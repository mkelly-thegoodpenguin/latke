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

/**********************************************************************
 Copyright 2012 Advanced Micro Devices, Inc. All rights reserved.
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 ?Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 ?Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************/

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
		cl_device_id my_device, DeviceInfo *deviceInfo, IArch *architecture, std::vector<uint64_t> queue_props) :
		ownsContext(ownsCtxt),
		context(my_context),
		device(my_device),
		commandQueue(NULL),
		deviceInfo(deviceInfo),
		arch(architecture) {

    cl_int errorCode;

  #ifdef CL_VERSION_2_0
    // Create command queue
    if (deviceInfo->checkOpenCL2_XCompatibility()) {
       cl_command_queue_properties *props = nullptr;
       if (!queue_props.empty())
         props = &queue_props[0];
       commandQueue = clCreateCommandQueueWithProperties(context, device,
               props, &errorCode);
      if (errorCode != CL_SUCCESS)
        Util::LogError(
            "Error: clCreateCommandQueueWithProperties() returned %s.\n",
            Util::TranslateOpenCLError(errorCode));
    }

  #endif
    if (!commandQueue) {
      cl_command_queue_properties properties = 0;
      for (uint64_t bf : queue_props)
          properties = properties | bf;
      commandQueue = clCreateCommandQueue(context, device, properties, &errorCode);
      if (errorCode != CL_SUCCESS)
        Util::LogError("Error: clCreateCommandQueue() returned %s.\n", Util::TranslateOpenCLError(errorCode));
    }
    if (!commandQueue)
      throw std::runtime_error("Failed to create command queue");
}

DeviceOCL::~DeviceOCL() {
	delete arch;
	delete deviceInfo;
	cl_int errorCode = CL_SUCCESS;
	if (commandQueue) {
		errorCode = clReleaseCommandQueue(commandQueue);
		if (errorCode != CL_SUCCESS) {
			Util::LogError("Error: clReleaseCommandQueue() returned %s.\n",
					Util::TranslateOpenCLError(errorCode));
		}
	}
	if (device) {
		errorCode = clReleaseDevice(device);
		if (errorCode != CL_SUCCESS) {
			Util::LogError("Error: clReleaseCommandQueue() returned %s.\n",
					Util::TranslateOpenCLError(errorCode));
		}
	}

	if (context && ownsContext) {
		auto status = clReleaseContext(context);
		CHECK_OPENCL_ERROR_NO_RETURN(status, "clReleaseContext failed.");
	}
}

std::string DeviceOCL::getBuildOptions(){
	return arch->getBuildOptions();
}


}
#endif
