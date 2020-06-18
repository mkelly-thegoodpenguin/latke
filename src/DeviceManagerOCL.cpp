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
#include "DeviceManagerOCL.h"
#include "UtilOCL.h"
#include "ArchFactory.h"
#include <cstring>

namespace ltk {

DeviceManagerOCL::DeviceManagerOCL(bool singleCtxt) :
        singleContext(singleCtxt), context(0) {
}

DeviceManagerOCL::~DeviceManagerOCL(void) {
    for (auto &dev : devices)
        delete dev;

    if (context) {
        auto status = clReleaseContext(context);
        CHECK_OPENCL_ERROR_NO_RETURN(status, "clReleaseContext failed.");
    }
}

DeviceOCL* DeviceManagerOCL::getDevice(size_t deviceNumber) {
    if (deviceNumber >= devices.size())
        return nullptr;
    return devices[deviceNumber];
}

int DeviceManagerOCL::init(int32_t platformId, eDeviceType type, int32_t deviceNumber, bool verbose) {
    bool isCpu = type == CPU;
    cl_device_type dType;
    switch(type){
		case CPU:
			dType = CL_DEVICE_TYPE_CPU;
			break;
		case GPU:
			dType = CL_DEVICE_TYPE_GPU;
			break;
		case ACCELERATOR:
			dType = CL_DEVICE_TYPE_ACCELERATOR;
			break;
		case CUSTOM:
			dType = CL_DEVICE_TYPE_CUSTOM;
			break;
		default:
			dType = CL_DEVICE_TYPE_DEFAULT;
			break;
    }

    // Get platform
    cl_platform_id platform = NULL;
    int retValue = getPlatform(platform, platformId, false, verbose);
    CHECK_ERROR(retValue, SUCCESS, "getPlatform() failed");

    // Display available devices.
    if (verbose) {
        retValue = displayDevices(platform, dType);
        CHECK_ERROR(retValue, SUCCESS, "displayDevices() failed");
    }

    // If we can our platform, use it. Otherwise just use available platform.
    cl_context_properties cps[3] = {
    CL_CONTEXT_PLATFORM, (cl_context_properties) platform, 0 };

    cl_int status = 0;
    context = clCreateContextFromType(cps, dType,
    NULL,
    NULL, &status);
    CHECK_OPENCL_ERROR(status, "clCreateContextFromType failed.");

    cl_device_id *deviceIds = nullptr;
    size_t numDevices;
    status = getDevices(context, &deviceIds, &numDevices);
    CHECK_ERROR(status, SUCCESS, "getDevices() failed");

    size_t firstDevice = 0;
    size_t lastDevicePlusOne = numDevices;
    // force usage of single context if there is only one device
    if (numDevices == 1) {
        singleContext = true;
    } else {
        if (deviceNumber >= 0 && deviceNumber < numDevices) {
            firstDevice = deviceNumber;
            lastDevicePlusOne = firstDevice + 1;
        }
    }

    for (auto i = firstDevice; i < lastDevicePlusOne; ++i) {
        auto deviceContext = context;
        // create unique context per device, if specified
        if (!singleContext) {
            deviceContext = clCreateContext(cps, 1, deviceIds + i,
            NULL,
            NULL, &status);
            CHECK_OPENCL_ERROR(status, "clCreateContext failed.");

        }
        //Set device info of given cl_device_id
        DeviceInfo *deviceInfo = new DeviceInfo();
        status = deviceInfo->setDeviceInfo(deviceIds[i]);
        CHECK_ERROR(status, SUCCESS, "DeviceInfo::setDeviceInfo() failed");
        auto arch = ArchFactory::getArchitecture(deviceInfo->venderId);
        if (!arch) {
        	if (verbose)
        		std::cout << "Unrecognized vendor id: " << deviceInfo->venderId << std::endl;
        	continue;
        }
        devices.push_back(
                new DeviceOCL(deviceContext, !singleContext, deviceIds[i],
                        deviceInfo, arch));
    }
    // clean up all-devices context if we are not using single context
    // for all devices
    if (!singleContext) {
        if (context) {
            auto status = clReleaseContext(context);
            CHECK_OPENCL_ERROR_NO_RETURN(status, "clReleaseContext failed.");
            context = 0;
        }
    }
    delete[] deviceIds;
    return DeviceSuccess;
}
size_t DeviceManagerOCL::getNumDevices() {
    return devices.size();
}
}
#endif
