/*
 * Copyright 2016-2020 Grok Image Compression Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <iostream>
#include <memory>
#include <sstream>
#include "latke_test.h"
#include "BlockingQueue.h"
#include <math.h>

using namespace ltk;

struct JobInfo{
	JobInfo(DeviceOCL *dev,
			std::unique_ptr<DualImageOCL> *hostToDev,
			std::unique_ptr<DualImageOCL> *devToHost,
			JobInfo *previous):
				hostToDevice(hostToDev),
				hostToDeviceMapped(0),
				triggerHostToDeviceUnmap(Util::CreateUserEvent(dev->context)),
				hostToDeviceUnmapped(0),
				kernelCompleted(0),
				deviceToHost(devToHost),
				deviceToHostMapped(0),
				triggerDeviceToHostUnmap(Util::CreateUserEvent(dev->context)),
				deviceToHostUnmapped(0),
				prev(previous)
	{}
	~JobInfo(){
		Util::ReleaseEvent(triggerHostToDeviceUnmap);
		Util::ReleaseEvent(triggerDeviceToHostUnmap);
	}

	std::unique_ptr<DualImageOCL> *hostToDevice;
	cl_event hostToDeviceMapped;
	cl_event triggerHostToDeviceUnmap;
	cl_event hostToDeviceUnmapped;

	cl_event kernelCompleted;

	std::unique_ptr<DualImageOCL> *deviceToHost;
	cl_event deviceToHostMapped;
	cl_event triggerDeviceToHostUnmap;
	cl_event deviceToHostUnmapped;

	JobInfo *prev;
};

BlockingQueue< JobInfo* > mappedImageQueue;

const int numBuffers = 20;
const int bufferWidth = 2048;
const int bufferHeight = 1920;
auto buffers = std::make_unique<uint8_t[]>(bufferWidth*bufferHeight*numBuffers);

void CL_CALLBACK HostToDeviceMappedCallback(cl_event event, cl_int cmd_exec_status, void *user_data) {

	// push mapped image into queue
	mappedImageQueue.push((JobInfo*)user_data);

	// cleanup
	Util::ReleaseEvent(event);
}

void CL_CALLBACK DeviceToHostMappedCallback(cl_event event, cl_int cmd_exec_status, void *user_data) {

	// handle processed data
	auto info = (JobInfo*)user_data;

	// trigger unmap
	Util::SetEventComplete(info->triggerDeviceToHostUnmap);

	// cleanup
	Util::ReleaseEvent(event);
}

int main()
{
  // 1. create device manager
  auto deviceManager = std::make_unique<DeviceManagerOCL>(true);
  auto rc = deviceManager->init(0, true);
  if (rc != DeviceSuccess) {
	  std::cout << "Failed to initialize OpenCL device";
	  exit(0);
  }

  auto dev = deviceManager->getDevice(0);

  const int numImages = 4;
  const int numBatches = numBuffers/numImages;

  std::unique_ptr<DualImageOCL> hostToDevice[numImages];
  std::unique_ptr<DualImageOCL> deviceToHost[numImages];
  std::unique_ptr<QueueOCL> kernelQueue[numImages];
  JobInfo* currentJobInfo[numImages];
  JobInfo* prevJobInfo[numImages];

  const int kernel_dim_x = 32;
  const int kernel_dim_y = 32;
  std::stringstream buildOptions;
  buildOptions << " -I ./ ";
  buildOptions << " -D KERNEL_DIM_X=" << kernel_dim_x;
  buildOptions << " -D KERNEL_DIM_Y=" << kernel_dim_y;
  KernelInitInfoBase initInfoBase(dev,buildOptions.str(),"",BUILD_BINARY_IN_MEMORY);
  KernelInitInfo initInfo(initInfoBase,"latke_test.cl", "latke_test", "process");
  std::unique_ptr<KernelOCL> kernel =  std::make_unique<KernelOCL>(initInfo);

  for (int i=0; i < numImages; ++i){
	  hostToDevice[i] = std::make_unique<DualImageOCL>(dev,bufferWidth, bufferHeight, CL_R, CL_UNSIGNED_INT8, true );
	  deviceToHost[i] = std::make_unique<DualImageOCL>(dev,bufferWidth, bufferHeight, CL_R, CL_UNSIGNED_INT8, false );
	  kernelQueue[i] = std::make_unique<QueueOCL>(dev);
	  currentJobInfo[i] = nullptr;
	  prevJobInfo[i] = nullptr;
  }

  // queue all kernel runs
  for (int j=0; j < numBatches; j++){
     for (int i=0; i < numImages; ++i){
    	auto prev = currentJobInfo[i];
    	currentJobInfo[i] = new JobInfo(dev, &hostToDevice[i], &deviceToHost[i], prevJobInfo[i]);
    	prevJobInfo[i] = prev;

    	// map hostToDevice image
    	// (wait for previous kernel to complete)
		if (!hostToDevice[i]->map(prev ? 1 : 0,
								prev  ? &prev->kernelCompleted : nullptr,
								&currentJobInfo[i]->hostToDeviceMapped,
								false)) {
			return -1;
		}
		// set callback on hostToDevice mapping, which will add this image
		// info to host-side queue of mapped buffers
		auto error_code = clSetEventCallback(currentJobInfo[i]->hostToDeviceMapped,
			CL_COMPLETE,
			&HostToDeviceMappedCallback,
			currentJobInfo[i]);
		if (DeviceSuccess != error_code) {
			Util::LogError("Error: clSetEventCallback returned %s.\n", Util::TranslateOpenCLError(error_code));
			return -1;
		}
		Util::ReleaseEvent(currentJobInfo[i]->hostToDeviceMapped);

		// unmap hostToDeviceImage
		if (!hostToDevice[i]->unmap(1,
								&currentJobInfo[i]->triggerHostToDeviceUnmap,
								&currentJobInfo[i]->hostToDeviceUnmapped)) {
			return -1;
		}

		// queue kernel
		kernel->pushArg<cl_mem>(hostToDevice[i]->getDeviceMem() );
		kernel->pushArg<cl_mem>(deviceToHost[i]->getDeviceMem() );

	    EnqueueInfo info;
	    info.dimension = 2;
	    info.local_work_size[0] = kernel_dim_x;
	    info.local_work_size[1] = kernel_dim_y;
		info.global_work_size[0] = (size_t)std::ceil(bufferWidth/(double)kernel_dim_x)*info.local_work_size[0];
		info.global_work_size[1] = (size_t)std::ceil(bufferHeight/(double)kernel_dim_y)*info.local_work_size[1];
		info.queue = kernelQueue[i]->getQueueImpl();
		info.needsCompletionEvent = true;
		cl_event wait_events[2] = {currentJobInfo[i]->hostToDeviceUnmapped, 0};
		info.num_events_in_wait_list = 1;
		info.event_wait_list = wait_events;
		// wait for unmapping of previous deviceToHost
		if (prev){
			info.num_events_in_wait_list++;
			wait_events[1] = prev->deviceToHostUnmapped;
		}
		kernel->enqueue(info);
		currentJobInfo[i]->kernelCompleted = info.completionEvent;


    	// map deviceToHost image
		if (!deviceToHost[i]->map(1,
								&currentJobInfo[i]->kernelCompleted,
								&currentJobInfo[i]->deviceToHostMapped,
								false)) {
			return -1;
		}
		// set callback on deviceToHostMapped mapping
		error_code = clSetEventCallback(currentJobInfo[i]->deviceToHostMapped,
			CL_COMPLETE,
			&DeviceToHostMappedCallback,
			currentJobInfo[i]);
		if (DeviceSuccess != error_code) {
			Util::LogError("Error: clSetEventCallback returned %s.\n", Util::TranslateOpenCLError(error_code));
			return -1;
		}
		Util::ReleaseEvent(currentJobInfo[i]->deviceToHostMapped);

		// unmap deviceToHostImage
		if (!deviceToHost[i]->unmap(1,
								&currentJobInfo[i]->triggerDeviceToHostUnmap,
								&currentJobInfo[i]->deviceToHostUnmapped)) {
			return -1;
		}
     }
  }

  // wait for images from queue, fill them, and trigger unmap event
  JobInfo *info=nullptr;
  int count = 0;
  while (mappedImageQueue.waitAndPop(info)){
	  /*
	   * fill image
	   */
	  // trigger unmap, which will allow kernel to proceed
	  Util::SetEventComplete(info->triggerHostToDeviceUnmap);
	  count++;
	  if (count == numBuffers)
		  break;
  }

  // cleanup
  fprintf(stdout, "completed\n");
  return 0;
}

