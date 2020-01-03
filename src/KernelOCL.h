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

#pragma once
#include "latke_config.h"
#ifdef OPENCL_FOUND
#include "platform.h"
#include <string>
#include "QueueOCL.h"
#include "UtilOCL.h"

namespace ltk {

#define LOAD_BINARY							0
#define BUILD_BINARY_IN_MEMORY				1
#define BUILD_BINARY_OFFLINE				2
#define BUILD_BINARY_OFFLINE_ALL_DEVICES	4

struct EnqueueInfo {
	EnqueueInfo();
	cl_command_queue queue;
	int dimension;
	size_t global_work_size[3];
	size_t global_work_offset[3];
	bool useOffset;
	size_t local_work_size[3];
	cl_uint num_events_in_wait_list;
	const cl_event *event_wait_list;
	bool needsCompletionEvent;
	cl_event completionEvent;
};

struct KernelInitInfoBase {
	KernelInitInfoBase(DeviceOCL *dev, std::string bldOptions, std::string directory,
			uint32_t binaryBuildMethod) :
			device(dev), buildOptions(bldOptions), directory(directory), binaryBuildMethod(
					binaryBuildMethod) {
	}
	KernelInitInfoBase(const KernelInitInfoBase &other) :
			device(other.device), buildOptions(other.buildOptions), directory(
					other.directory), binaryBuildMethod(other.binaryBuildMethod) {
	}
	DeviceOCL *device;
	string buildOptions;
	std::string directory;
	uint32_t binaryBuildMethod;
};

struct KernelInitInfo: KernelInitInfoBase {
	KernelInitInfo(KernelInitInfoBase initInfo, string progName,
			string binaryName, string knlName) :
			KernelInitInfoBase(initInfo), programName(progName), binaryName(
					binaryName), kernelName(knlName) {
	}
	string programName;
	string binaryName;
	string kernelName;
};

class KernelOCL {
public:
	KernelOCL(KernelInitInfo initInfo);
	virtual ~KernelOCL(void);
	cl_kernel getKernel() {
		return myKernel;
	}
	cl_device_id getDevice() {
		return device;
	}
	void enqueue(EnqueueInfo &info);
	virtual void generateBinary();

	template<typename T> void pushArg(T *val) {
		auto error_code = clSetKernelArg(myKernel, argCount++, sizeof(T), val);
		if (DeviceSuccess != error_code) {
			Util::LogError("Error: setKernelArgs returned %s.\n",
					Util::TranslateOpenCLError(error_code));
			throw std::exception();
		}
	}
protected:
	cl_program loadBinary(bool verbose);
	buildProgramData getProgramData();
	std::string getBuildOptions();
	KernelInitInfo initInfo;
	cl_kernel myKernel;
	cl_ulong localMemorySize;
	cl_device_id device;
	cl_context context;
	uint32_t argCount;
};
}
#endif
