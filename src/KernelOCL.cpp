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

#include "latke_config.h"
#ifdef OPENCL_FOUND
#include "KernelOCL.h"
#include <stdio.h>
#include "UtilOCL.h"
#include <sstream>
#include <algorithm>

namespace ltk {


KernelOCL::KernelOCL(KernelInitInfo init) : KernelOCL(init, 0)
{}

KernelOCL::KernelOCL(KernelInitInfo init, cl_program prog) : initInfo(init),
											myKernel(0),
											device(init.device->device),
											context(init.device->context),
											argCount(0),
											program(prog) {
	bool verbose = true;
	if (!prog)
		program = generateProgram(init);

	// Create the required kernel
	cl_int error_code;
	myKernel = clCreateKernel(program, initInfo.kernelName.c_str(),
			&error_code);
	if (CL_SUCCESS != error_code) {
		Util::LogError("Error: clCreateKernel returned %s for kernel %s.\n",
				Util::TranslateOpenCLError(error_code),
				initInfo.kernelName.c_str());
		throw std::runtime_error(
				("Failed to create kernel " + initInfo.kernelName + "\n").c_str());
	}
	if (verbose)
		std::cout << "Created kernel " << initInfo.kernelName << std::endl;

	if (!prog)
		clReleaseProgram(program);
}

KernelOCL::~KernelOCL(void) {
	if (myKernel)
		clReleaseKernel(myKernel);
}

cl_program KernelOCL::generateProgram(KernelInitInfo init){
	cl_program program = 0;
	buildProgramData data = getProgramData(init);
	if (init.binaryBuildMethod == LOAD_BINARY ) {
		generateBinaryName(data);
	} else {
		data.binaryName = "";
	}

	cl_int error_code = buildOpenCLProgram(program, init.device->context, data);
	if (error_code != CL_SUCCESS) {
		Util::LogError("Error: buildOpenCLProgram returned %s.\n",
				Util::TranslateOpenCLError(error_code));
		throw std::runtime_error(
				("Failed to build program " + data.programName + "\n").c_str());
	}

	return program;
}

void KernelOCL::generateBinaryName(buildProgramData &data) {
	// try binary
	char deviceName[1024];
	auto status = clGetDeviceInfo(data.device,CL_DEVICE_NAME, sizeof(deviceName), deviceName,NULL);

	// remove white space from device name
	std::string deviceNameString(deviceName);
	deviceNameString.erase(	remove_if(deviceNameString.begin(), deviceNameString.end(),::isspace),
							deviceNameString.end());
	data.binaryName = data.binaryName + "." + deviceNameString;
}

buildProgramData KernelOCL::getProgramData(KernelInitInfo init) {
	buildProgramData data;
	data.device = init.device->device;
	data.programName = init.programName;
	data.binaryName = init.binaryName;
	data.programPath = init.directory;
	data.flagsStr = getBuildOptions(init) + init.buildOptions;
	return data;
}

void KernelOCL::generateBinary(KernelInitInfo init) {
	buildProgramData data = getProgramData(init);
	bifData biData;
	biData.programPath = data.programPath;
	biData.programFileName = data.programName;
	biData.binaryName = init.binaryName;
	biData.flagsStr = data.flagsStr;
	if (!(init.binaryBuildMethod & BUILD_BINARY_OFFLINE_ALL_DEVICES)) {
		biData.numDevices = 1;
		biData.devices = &init.device->device;
	}
	generateBinaryImage(biData);

}

std::string KernelOCL::getBuildOptions(KernelInitInfo init) {
	std::stringstream bldOptions;
	bldOptions << init.device->getBuildOptions();
	if (init.device->deviceInfo->checkOpenCL2_XCompatibility())
		bldOptions << "-cl-std=CL2.0 -D OPENCL_2_X";
	return bldOptions.str();
}

// Enqueue the command to asynchronously execute the kernel on the device
void KernelOCL::enqueue(EnqueueInfoOCL &info) {
	cl_int error_code = clEnqueueNDRangeKernel(info.queue->getQueueImpl(), myKernel,
			info.dimension, info.global_work_offset, info.global_work_size,
			info.local_work_size, info.numWaitEvents,
			info.numWaitEvents ? (cl_event*)info.waitEvents : NULL,
			info.needsCompletionEvent ? &info.completionEvent : NULL);
	if (CL_SUCCESS != error_code) {
		Util::LogError("Error: clEnqueueNDRangeKernel returned %s.\n",
				Util::TranslateOpenCLError(error_code));
		throw std::exception();
	}
	argCount = 0;
}
}
#endif
