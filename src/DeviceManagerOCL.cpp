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
#include "ArchAMD.h"
#include <cstring>

namespace ltk {

// deviceId in {0,1,2.....} specifies that single GPU device with this id should be used by plugin
// deviceId == ALL_GPU_DEVICES specifies that all GPU devices should be used by plugin
// deviceId == FALLBACK_TO_HOST specifies that plugin is not used

const int32_t ALL_GPU_DEVICES = -1;
const int32_t FALLBACK_TO_HOST = -2;

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

DeviceOCL* DeviceManagerOCL::getDevice(size_t deviceId) {
    if (deviceId >= devices.size())
        return nullptr;
    return devices[deviceId];
}

int DeviceManagerOCL::init(int32_t deviceId, bool verbose) {
    eDeviceType firstTry =
            (deviceId >= 0 || deviceId == ALL_GPU_DEVICES) ? GPU : CPU;
    if (verbose)
        std::cout << "Initializing " << ((firstTry == GPU) ? "GPU." : "CPU.")
                << std::endl;
    int rc = init(firstTry, deviceId, verbose);
    if (rc != DeviceSuccess && firstTry != CPU) {
        if (verbose)
            std::cout << "Initializing " << "CPU." << std::endl;
        rc = init(CPU, deviceId, verbose);
    }
    return rc;
}

int DeviceManagerOCL::init(eDeviceType type, int32_t deviceId, bool verbose) {
    bool isCpu = type == CPU;
    cl_device_type dType =
            (type == CPU) ? CL_DEVICE_TYPE_CPU : CL_DEVICE_TYPE_GPU;

    // Get platform
    cl_platform_id platform = NULL;
    int retValue = getPlatform(platform, 0, false, verbose);
    CHECK_ERROR(retValue, SUCCESS, "getPlatform() failed");

    // Display available devices.
    if (verbose) {
        retValue = displayDevices(platform, dType);
        CHECK_ERROR(retValue, SUCCESS, "displayDevices() failed");
    }

    // If we could find our platform, use it. Otherwise use just available platform.
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
        if (deviceId >= 0 && deviceId < numDevices) {
            firstDevice = deviceId;
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
        DeviceInfo *deviceInfo = new DeviceInfo();
        //Set device info of given cl_device_id
        status = deviceInfo->setDeviceInfo(deviceIds[i]);
        CHECK_ERROR(status, SUCCESS, "DeviceInfo::setDeviceInfo() failed");
        char buildOption[4096] = "";

        bool isOCL2_x = deviceInfo->checkOpenCL2_XCompatibility();
        if (deviceInfo->checkOpenCL2_XCompatibility()) {
            if (deviceInfo->detectSVM()) {
                strcat(buildOption, "-cl-std=CL2.0 ");
            }
        }
        devices.push_back(
                new DeviceOCL(deviceContext, !singleContext, deviceIds[i],
                        deviceInfo, new ArchAMD()));
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
