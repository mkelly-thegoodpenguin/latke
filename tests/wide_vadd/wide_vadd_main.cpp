/**********
Copyright (c) 2019, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

#include <array>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <chrono>

#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1

#include <CL/cl2.hpp>
#include <CL/cl_ext_xilinx.h>
#include "latke.h"
using namespace ltk;

// OpenMP
#include <omp.h>


const uint32_t num_concurrent_kernels = 4;
const uint32_t bufferSize = (1024 * 1024 * 2);
const uint32_t numSubBuffers = 1;
std::string kernelName = "wide_vadd";


void vadd_sw(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t size){
#pragma omp parallel for
    for (int i = 0; i < size; i++) {
        c[i] = a[i] + b[i];
    }
}

int subdivide_buffer(std::vector<cl::Buffer> &divided, cl::Buffer buf_in, cl_mem_flags flags, int num_divisions){
    // Get the size of the buffer
    size_t size;
    size = buf_in.getInfo<CL_MEM_SIZE>();

    if (num_divisions == 1){
    	divided.push_back(buf_in);
    	return 0;
    }

    if (size / num_divisions <= 4096) {
        return -1;
    }

    cl_buffer_region region;

    int err;
    region.origin = 0;
    region.size   = size / num_divisions;

    // Round region size up to nearest 4k for efficient burst behavior
    if (region.size % 4096 != 0) {
        region.size += (4096 - (region.size % 4096));
    }

    for (int i = 0; i < num_divisions; i++) {
        if (i == num_divisions - 1) {
            if ((region.origin + region.size) > size) {
                region.size = size - region.origin;
            }
        }
        cl::Buffer buf = buf_in.createSubBuffer(flags, CL_BUFFER_CREATE_TYPE_REGION, &region, &err);
        if (err != CL_SUCCESS) {
            return err;
        }
        divided.push_back(buf);
        region.origin += region.size;
    }

    return 0;
}

int enqueue_subbuf_vadd(cl::CommandQueue &q, cl::Kernel &krnl, cl::Event *event, cl::Buffer a, cl::Buffer b, cl::Buffer c)
{
    // Get the size of the buffer
    cl::Event k_event, m_event;
    std::vector<cl::Event> krnl_events;

    static std::vector<cl::Event> tx_events, rx_events;

    std::vector<cl::Memory> c_vec;
    size_t size;
    size = a.getInfo<CL_MEM_SIZE>();

    std::vector<cl::Memory> in_vec;
    in_vec.push_back(a);
    in_vec.push_back(b);
    q.enqueueMigrateMemObjects(in_vec, 0, &tx_events, &m_event);
    krnl_events.push_back(m_event);
    tx_events.push_back(m_event);
    if (tx_events.size() > 1) {
        tx_events[0] = tx_events[1];
        tx_events.pop_back();
    }

    krnl.setArg(0, a);
    krnl.setArg(1, b);
    krnl.setArg(2, c);
    krnl.setArg(3, (uint32_t)(size / sizeof(uint32_t)));

    q.enqueueTask(krnl, &krnl_events, &k_event);
    krnl_events.push_back(k_event);
    if (rx_events.size() == 1) {
        krnl_events.push_back(rx_events[0]);
        rx_events.pop_back();
    }
    c_vec.push_back(c);
    q.enqueueMigrateMemObjects(c_vec, CL_MIGRATE_MEM_OBJECT_HOST, &krnl_events, event);
    rx_events.push_back(*event);

    return 0;
}

struct KernelObjects {
	KernelObjects() : a(nullptr),b(nullptr),c(nullptr)
	{}
    cl::Buffer a_buf;
    cl::Buffer b_buf;
    cl::Buffer c_buf;
    uint32_t *a, *b, *c;
	std::vector<cl::Buffer> a_bufs;
	std::vector<cl::Buffer> b_bufs;
	std::vector<cl::Buffer> c_bufs;

	cl_mem_ext_ptr_t in_bank_ext;
	cl::Kernel krnl;
	std::array<cl::Event, numSubBuffers> kernel_events;
};


int main(int argc, char *argv[])
{
  	// 1. create device manager
    const int platformId = 0;
    const eDeviceType deviceType = ACCELERATOR;
    const int deviceNum = 0;

  	auto deviceManager = std::make_shared<DeviceManagerOCL>(true);
    cl_command_queue_properties queue_props = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
  	auto rc = deviceManager->init(platformId, deviceType, deviceNum, true, queue_props);
  	if (rc != DeviceSuccess) {
  		std::cerr << "Failed to initialize OpenCL device";
  		return -1;
  	}

	auto dev = deviceManager->getDevice(deviceNum);
	auto arch = ArchFactory::getArchitecture(dev->deviceInfo->venderId);
	if (!arch){
	    std::cerr << "Unsupported OpenCL vendor ID " << dev->deviceInfo->venderId;
	    return -1;
	}
    cl::Context ctx = cl::Context(dev->context);
    cl::CommandQueue q = cl::CommandQueue(dev->queue);

    std::cout << std::endl << std::endl;
    std::cout << "Running kernel test with XRT-allocated contiguous buffers" << std::endl;
    std::cout << "and wide VADD (16 values/clock)" << std::endl;

	std::stringstream buildOptions;
	buildOptions << " -I ./ ";
	switch (arch->getVendorId()) {
		case vendorIdAMD:
			buildOptions << " -D AMD_GPU_ARCH";
			break;
		case vendorIdNVD:
			buildOptions << " -D NVIDIA_ARCH";
			break;
		case vendorIdXILINX:
			buildOptions << "";
			break;
    case vendorIdINTL:
      buildOptions << "";
      break;
		default:
			return -1;

	}
	buildOptions << arch->getBuildOptions();
	//buildOptions << " -D DEBUG";

	KernelInitInfoBase initInfoBase(dev, buildOptions.str(), "",LOAD_BINARY);
	KernelInitInfo initInfo(initInfoBase, "", "wide_vadd_250HW","");
	cl_program program = KernelOCL::generateProgram(initInfo);


    // Map our user-allocated buffers as OpenCL buffers using a shared
    // host pointer
    KernelObjects objects[num_concurrent_kernels];
    for (uint32_t i = 0; i < num_concurrent_kernels; ++i) {
    	auto obj = objects + i;
		try {

	    	std::stringstream ss;
	    	ss << kernelName << ":{" << kernelName << "_" << (i + 1) << "}";
	    	initInfo.kernelName = ss.str();

	    	KernelOCL *kernel = nullptr;
	    	try {
	    		kernel = new KernelOCL(initInfo,program);
	    	} catch (std::runtime_error &re) {
	    		std::cerr << "Unable to build kernel. Exiting" << std::endl;
	    		return -1;
	    	}

	        obj->krnl = cl::Kernel(kernel->getKernel());

	    	obj->in_bank_ext.flags = i | XCL_MEM_TOPOLOGY;
			obj->in_bank_ext.obj   = NULL;
			obj->in_bank_ext.param = 0;

			obj->a_buf = cl::Buffer(ctx,
							 static_cast<cl_mem_flags>(CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX),
							 bufferSize * sizeof(uint32_t),
							 &obj->in_bank_ext,
							 NULL);
			obj->b_buf = cl::Buffer(ctx,
							 static_cast<cl_mem_flags>(CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX),
							 bufferSize * sizeof(uint32_t),
							 &obj->in_bank_ext,
							 NULL);
			obj->c_buf = cl::Buffer(ctx,
							 static_cast<cl_mem_flags>(CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_EXT_PTR_XILINX),
							 bufferSize * sizeof(uint32_t),
							 &obj->in_bank_ext,
							 NULL);

			// Although we'll change these later, we'll set the buffers as kernel
			// arguments prior to mapping so that XRT can resolve the physical memory
			// in which they need to be allocated
			obj->krnl.setArg(0, obj->a_buf);
			obj->krnl.setArg(1, obj->b_buf);
			obj->krnl.setArg(2, obj->c_buf);

			obj->a = (uint32_t *)q.enqueueMapBuffer(obj->a_buf,
														 CL_TRUE,
														 CL_MAP_WRITE_INVALIDATE_REGION,
														 0,
														 bufferSize * sizeof(uint32_t));
			obj->b = (uint32_t *)q.enqueueMapBuffer(obj->b_buf,
														 CL_TRUE,
														 CL_MAP_WRITE_INVALIDATE_REGION,
														 0,
														 bufferSize * sizeof(uint32_t));
			for (int i = 0; i < bufferSize; i++) {
				obj->a[i] = i;
				obj->b[i] = 2 * i;
			}
		}
		catch (cl::Error &e) {
			std::cout << "ERROR: " << e.what() << std::endl;
			return EXIT_FAILURE;
		}
    }

	auto start = std::chrono::high_resolution_clock::now();
	std::array<cl::Event, numSubBuffers * num_concurrent_kernels > globalKernelEvents;

    for (uint32_t i = 0; i < num_concurrent_kernels; ++i) {
    	auto obj = objects + i;
		try {
			// Send the buffers down to the Alveo card
			q.enqueueUnmapMemObject(obj->a_buf, obj->a);
			q.enqueueUnmapMemObject(obj->b_buf, obj->b);

			subdivide_buffer(obj->a_bufs, obj->a_buf, CL_MEM_READ_ONLY| CL_MEM_HOST_WRITE_ONLY, numSubBuffers);
			subdivide_buffer(obj->b_bufs, obj->b_buf, CL_MEM_READ_ONLY| CL_MEM_HOST_WRITE_ONLY, numSubBuffers);
			subdivide_buffer(obj->c_bufs, obj->c_buf, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, numSubBuffers);

			for (int i = 0; i < numSubBuffers; i++)
				enqueue_subbuf_vadd(q, obj->krnl, &obj->kernel_events[i], obj->a_bufs[i], obj->b_bufs[i], obj->c_bufs[i]);

		}
		catch (cl::Error &e) {
			std::cout << "ERROR: " << e.what() << std::endl;
			return EXIT_FAILURE;
		}
    }

    for (uint32_t i = 0; i < num_concurrent_kernels; ++i) {
    	auto obj = objects + i;
    	 for (uint32_t j = 0; j < numSubBuffers; ++j) {
    		 globalKernelEvents[j + i * numSubBuffers] = obj->kernel_events[j].get();
    	 }
    }

	clWaitForEvents(numSubBuffers, (const cl_event *)&globalKernelEvents);

    for (uint32_t i = 0; i < num_concurrent_kernels; ++i) {
    	auto obj = objects + i;

    	// 1. map output
    	obj->c = (uint32_t *)q.enqueueMapBuffer(obj->c_buf,
													 CL_TRUE,
													 CL_MAP_READ,
													 0,
													 bufferSize * sizeof(uint32_t));
		// 2. process output

		// 3. unmap output
		q.enqueueUnmapMemObject(obj->c_buf, obj->c);
    }
    q.finish();

	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::cout << "Execution time " <<  (elapsed.count() * 1000.0) << " ms per buffer" << std::endl;

    if (program)
    	clReleaseProgram(program);
}
