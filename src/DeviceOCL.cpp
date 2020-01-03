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
#include "DeviceOCL.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include "time.h"
#include "stdarg.h"
#endif
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "UtilOCL.h"
namespace ltk {

DeviceOCL::DeviceOCL(cl_context my_context, bool ownsCtxt,
		cl_device_id my_device, DeviceInfo *deviceInfo, IArch *architecture) :
		ownsContext(ownsCtxt), context(my_context), device(my_device), commandQueue(
				NULL), deviceInfo(deviceInfo), arch(architecture) {

	cl_int status = 0;
#ifdef CL_VERSION_2_0
	// Create command queue
	cl_queue_properties prop[] = { 0 };
	commandQueue = clCreateCommandQueueWithProperties(context, device, prop,
			&status);
	CHECK_OPENCL_ERROR_NO_RETURN(status,
			"clCreateCommandQueueWithProperties failed.");
#else
	queue = clCreateCommandQueue(context,
		arg->device,
		0,
		NULL);
	CHECK_OPENCL_ERROR_NO_RETURN(status, "clCreateCommandQueue failed.");
#endif
	// todo: throw exception if create command queue failed
}

DeviceOCL::~DeviceOCL() {
	delete arch;
	delete deviceInfo;
	cl_int errorCode = CL_SUCCESS;
	if (commandQueue) {
		errorCode = clReleaseCommandQueue(commandQueue);
		if (errorCode != CL_SUCCESS) {
			Util::LogError("Error: clCreateCommandQueue() returned %s.\n",
					Util::TranslateOpenCLError(errorCode));
		}
	}
	if (device) {
		errorCode = clReleaseDevice(device);
		if (errorCode != CL_SUCCESS) {
			Util::LogError("Error: clCreateCommandQueue() returned %s.\n",
					Util::TranslateOpenCLError(errorCode));
		}
	}

	if (context && ownsContext) {
		auto status = clReleaseContext(context);
		CHECK_OPENCL_ERROR_NO_RETURN(status, "clReleaseContext failed.");
	}
}
}
#endif
