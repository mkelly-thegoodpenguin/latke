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
#include "latke.h"
#include "BlockingQueue.h"
#include <math.h>
#include <chrono>
#include <cassert>
#include <thread>

using namespace ltk;

template<typename M> struct JobInfo {
	JobInfo(DeviceOCL *dev, std::unique_ptr<M> *hostToDev,
			std::unique_ptr<M> *devToHost, JobInfo *previous) :
			hostToDevice(new MemInfo<M>(dev, hostToDev)), kernelCompleted(
					0), deviceToHost(new MemInfo<M>(dev, devToHost)), prev(
					previous) {
	}
	~JobInfo() {
		delete hostToDevice;
		Util::ReleaseEvent(kernelCompleted);
		delete deviceToHost;
	}

	MemInfo<M> *hostToDevice;
	cl_event kernelCompleted;
	MemInfo<M> *deviceToHost;

	JobInfo *prev;
};

BlockingQueue<JobInfo<DualImageOCL>*> mappedHostToDeviceQueue;
BlockingQueue<JobInfo<DualImageOCL>*> mappedDeviceToHostQueue;

const int numBuffers = 100;
const int bufferWidth = 2048;
const int bufferHeight = 1920;

void CL_CALLBACK HostToDeviceMappedCallback(cl_event event,
		cl_int cmd_exec_status, void *user_data) {

	assert(user_data);

	auto info = (JobInfo<DualImageOCL>*) user_data;

	// push mapped image into queue
	mappedHostToDeviceQueue.push(info);
}

void CL_CALLBACK DeviceToHostMappedCallback(cl_event event,
		cl_int cmd_exec_status, void *user_data) {

	assert(user_data);
	// handle processed data
	auto info = (JobInfo<DualImageOCL>*) user_data;

	// push mapped image into queue
	mappedDeviceToHostQueue.push(info);
}

int main() {
	// 1. create device manager
	auto deviceManager = std::make_unique<DeviceManagerOCL>(true);
	auto rc = deviceManager->init(0, true);
	if (rc != DeviceSuccess) {
		std::cout << "Failed to initialize OpenCL device";
		exit(-1);
	}

	auto dev = deviceManager->getDevice(0);

	const int numImages = 4;
	const int numBatches = numBuffers / numImages;

	std::unique_ptr<DualImageOCL> hostToDevice[numImages];
	std::unique_ptr<DualImageOCL> deviceToHost[numImages];
	std::unique_ptr<QueueOCL> kernelQueue[numImages];
	JobInfo<DualImageOCL> *currentJobInfo[numImages];
	JobInfo<DualImageOCL> *prevJobInfo[numImages];

	const int kernel_dim_x = 32;
	const int kernel_dim_y = 32;
	std::stringstream buildOptions;
	buildOptions << " -I ./ ";
	buildOptions << " -D KERNEL_DIM_X=" << kernel_dim_x;
	buildOptions << " -D KERNEL_DIM_Y=" << kernel_dim_y;
	KernelInitInfoBase initInfoBase(dev, buildOptions.str(), "",
	BUILD_BINARY_IN_MEMORY);
	KernelInitInfo initInfo(initInfoBase, "simple.cl", "simple",
			"process");
	std::unique_ptr<KernelOCL> kernel = std::make_unique<KernelOCL>(initInfo);

	for (int i = 0; i < numImages; ++i) {
		hostToDevice[i] = std::make_unique<DualImageOCL>(dev, bufferWidth,
				bufferHeight, CL_R, CL_UNSIGNED_INT8, true);
		deviceToHost[i] = std::make_unique<DualImageOCL>(dev, bufferWidth,
				bufferHeight, CL_R, CL_UNSIGNED_INT8, false);
		kernelQueue[i] = std::make_unique<QueueOCL>(dev);
		currentJobInfo[i] = nullptr;
		prevJobInfo[i] = nullptr;
	}

	// queue all kernel runs
	for (int j = 0; j < numBatches; j++) {
		for (int i = 0; i < numImages; ++i) {
			bool lastBatch = j == numBatches - 1;
			auto prev = currentJobInfo[i];
			currentJobInfo[i] = new JobInfo<DualImageOCL>(dev, &hostToDevice[i],
					&deviceToHost[i], prevJobInfo[i]);
			prevJobInfo[i] = prev;

			// map
			// (wait for previous kernel to complete)
			cl_event hostToDeviceMapped;
			if (!hostToDevice[i]->map(prev ? 1 : 0,
					prev ? &prev->kernelCompleted : nullptr,
					&hostToDeviceMapped, false)) {
				return -1;
			}
			// set callback, which will add this image
			// info to host-side queue of mapped buffers
			auto error_code = clSetEventCallback(hostToDeviceMapped,
			CL_COMPLETE, &HostToDeviceMappedCallback, currentJobInfo[i]);
			if (DeviceSuccess != error_code) {
				Util::LogError("Error: clSetEventCallback returned %s.\n",
						Util::TranslateOpenCLError(error_code));
				return -1;
			}
			Util::ReleaseEvent(hostToDeviceMapped);

			// unmap
			if (!hostToDevice[i]->unmap(1,
					&currentJobInfo[i]->hostToDevice->triggerMemUnmap,
					&currentJobInfo[i]->hostToDevice->memUnmapped)) {
				return -1;
			}

			// queue kernel
			kernel->pushArg<cl_mem>(hostToDevice[i]->getDeviceMem());
			kernel->pushArg<cl_mem>(deviceToHost[i]->getDeviceMem());

			EnqueueInfo info;
			info.dimension = 2;
			info.local_work_size[0] = kernel_dim_x;
			info.local_work_size[1] = kernel_dim_y;
			info.global_work_size[0] = (size_t) std::ceil(
					bufferWidth / (double) kernel_dim_x)
					* info.local_work_size[0];
			info.global_work_size[1] = (size_t) std::ceil(
					bufferHeight / (double) kernel_dim_y)
					* info.local_work_size[1];
			info.queue = kernelQueue[i]->getQueueImpl();
			info.needsCompletionEvent = true;
			cl_event wait_events[2] = {
					currentJobInfo[i]->hostToDevice->memUnmapped, 0 };
			info.num_events_in_wait_list = 1;
			// wait for unmapping of previous deviceToHost
			if (prev) {
				wait_events[info.num_events_in_wait_list++] =
						prev->deviceToHost->memUnmapped;
			}
			info.event_wait_list = wait_events;
			try {
				kernel->enqueue(info);
			} catch (std::exception &ex) {
				// todo: handle exception
			}
			currentJobInfo[i]->kernelCompleted = info.completionEvent;

			// map
			cl_event deviceToHostMapped;
			if (!deviceToHost[i]->map(1, &currentJobInfo[i]->kernelCompleted,
					&deviceToHostMapped, false)) {
				return -1;
			}
			// set callback on mapping
			error_code = clSetEventCallback(deviceToHostMapped,
			CL_COMPLETE, &DeviceToHostMappedCallback, currentJobInfo[i]);
			if (DeviceSuccess != error_code) {
				Util::LogError("Error: clSetEventCallback returned %s.\n",
						Util::TranslateOpenCLError(error_code));
				return -1;
			}
			Util::ReleaseEvent(deviceToHostMapped);

			// unmap (except last batch)
			if (!lastBatch) {
				if (!deviceToHost[i]->unmap(1,
						&currentJobInfo[i]->deviceToHost->triggerMemUnmap,
						&currentJobInfo[i]->deviceToHost->memUnmapped)) {
					return -1;
				}
			}
		}
	}

	auto start = std::chrono::high_resolution_clock::now();

	// wait for images from queue, fill them, and trigger unmap event
	std::thread pushImages([]() {
		JobInfo<DualImageOCL> *info = nullptr;
		int count = 0;
		while (mappedHostToDeviceQueue.waitAndPop(info)) {
			/*
			 * fill unprocessed image memory
			 */
			// trigger unmap, allowing current kernel to proceed
			Util::SetEventComplete(info->hostToDevice->triggerMemUnmap);
			count++;
			if (count == numBuffers)
				break;
		}
	});

	// wait for processed images from queue, handle them,
	// and trigger unmap event
	std::thread pullImages([]() {
		JobInfo<DualImageOCL> *info = nullptr;
		int count = 0;
		while (mappedDeviceToHostQueue.waitAndPop(info)) {

			/*
			 *
			 * handle processed image memory
			 *
			 */

			// trigger unmap, allowing next kernel to proceed
			Util::SetEventComplete(info->deviceToHost->triggerMemUnmap);

			// cleanup
			delete info->prev;
			info->prev = nullptr;
			count++;
			if (count == numBuffers)
				break;
		}
	});

	pushImages.join();
	pullImages.join();

	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;

	// cleanup
	for (int i = 0; i < numImages; ++i) {
		deviceToHost[i]->unmap(0, nullptr, nullptr);
		delete currentJobInfo[i]->prev;
		delete currentJobInfo[i];
	}
	fprintf(stdout, "time per image = %f ms\n",
			(elapsed.count() * 1000) / (double) numBuffers);
	return 0;
}
