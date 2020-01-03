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
#include "QueueOCL.h"
#include "DeviceOCL.h"
#include "UtilOCL.h"

namespace ltk {

QueueOCL::QueueOCL(QueueOCL &rhs) :
		queue(rhs.queue), ownsQueue(rhs.ownsQueue) {
}
QueueOCL::QueueOCL(cl_command_queue cmdQueue) :
		queue(cmdQueue), ownsQueue(false) {
}

QueueOCL::QueueOCL(DeviceOCL *device) :
		queue(0), ownsQueue(true) {
	cl_int errorCode;

#ifdef CL_VERSION_2_0
	// Create command queue
	cl_queue_properties prop[] = { 0 };
	queue = clCreateCommandQueueWithProperties(device->context, device->device,
			prop, &errorCode);
	if (errorCode != CL_SUCCESS) {
		Util::LogError(
				"Error: clCreateCommandQueueWithProperties() returned %s.\n",
				Util::TranslateOpenCLError(errorCode));
	}

#else
	cl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;
	queue = clCreateCommandQueue(device->context, device->device, properties, &errorCode);
	if (errorCode != CL_SUCCESS)
	{
		Util::LogError("Error: clCreateCommandQueue() returned %s.\n", Util::TranslateOpenCLError(errorCode));
	}
#endif
}

QueueOCL::~QueueOCL(void) {
	if (queue && ownsQueue) {
		cl_int errorCode = clReleaseCommandQueue(queue);
		if (errorCode != CL_SUCCESS) {
			Util::LogError("Error: clCreateCommandQueue() returned %s.\n",
					Util::TranslateOpenCLError(errorCode));
		}
	}
}

tDeviceRC QueueOCL::finish(QueueOCL *queue) {
	return finish(queue->getQueueImpl());
}
tDeviceRC QueueOCL::finish(cl_command_queue commandQueue) {
	// Wait until the end of the execution
	cl_int error_code = clFinish(commandQueue);
	if (CL_SUCCESS != error_code) {
		Util::LogError("Error: clFinish returned %s.\n",
				Util::TranslateOpenCLError(error_code));
		return error_code;
	}
	return CL_SUCCESS;
}

tDeviceRC QueueOCL::finish(void) {
	return QueueOCL::finish(queue);
}

tDeviceRC QueueOCL::flush(QueueOCL *queue) {
	return flush(queue->getQueueImpl());
}
tDeviceRC QueueOCL::flush(cl_command_queue commandQueue) {
	// Wait until the end of the execution
	cl_int error_code = clFlush(commandQueue);
	if (CL_SUCCESS != error_code) {
		Util::LogError("Error: clFinish returned %s.\n",
				Util::TranslateOpenCLError(error_code));
		return error_code;
	}
	return CL_SUCCESS;
}

tDeviceRC QueueOCL::flush(void) {
	return QueueOCL::flush(queue);
}
}
#endif
