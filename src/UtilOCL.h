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
 Copyright ©2013 Advanced Micro Devices, Inc. All rights reserved.
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 •   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 •   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************/
#pragma once
#include "latke_config.h"
#ifdef OPENCL_FOUND
#include <string>
#include <iostream>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#include <CL/opencl.h>
#endif

#include "IArch.h"

namespace ltk {

#define CHECK_OPENCL_ERROR_NO_RETURN(actual, msg) \
    if(checkVal(actual, CL_SUCCESS, msg)) \
    { \
        std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
    }

#define CHECK_OPENCL_ERROR(actual, msg) \
    if(checkVal(actual, CL_SUCCESS, msg)) \
    { \
        std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
        return FAILURE; \
    }

#define CHECK_OPENCL_ERROR_CLEANUP(actual, msg) \
    if(checkVal(actual, CL_SUCCESS, msg)) \
    { \
        std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
        goto CLEANUP; \
    }

/**
 * GLOBAL DEFINED Macros
 */
#define CL_CONTEXT_OFFLINE_DEVICES_AMD        0x403F

#define getPlatform(platform, platformId, isPlatformEnabled, verbose) getPlatformL((platform), (platformId), (isPlatformEnabled), (dType), (verbose))

const int SUCCESS = 0;
const int FAILURE = 1;
const int EXPECTED_FAILURE = 2;

#define CHECK_ALLOCATION(actual, msg) \
    if(actual == NULL) \
    { \
        error(msg); \
        std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
        return FAILURE; \
    }

#define CHECK_ALLOCATION_CLEANUP(actual, msg) \
    if(actual == NULL) \
    { \
        error(msg); \
        std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
        goto CLEANUP; \
    }

#define CHECK_ERROR(actual, reference, msg) \
    if(actual != reference) \
    { \
        error(msg); \
        std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
        return FAILURE; \
    }

/**
 * error
 * constant function, Prints error messages
 * @param errorMsg std::string message
 */
void error(std::string errorMsg);

/**
 * getPath
 * @return path of the current directory
 */
std::string getPath();

class KernelFile {
public:
	KernelFile() :
			source_("") {
	}
	~KernelFile() {
	}
	;

	/**
	 * Opens the CL program file
	 * @return true if success else false
	 */
	bool open(const char *fileName);

	/**
	 * writeBinaryToFile
	 * @param fileName Name of the file
	 * @param binary char binary array
	 * @param numBytes number of bytes
	 * @return true if success else false
	 */
	int writeBinaryToFile(const char *fileName, const char *binary,
			size_t numBytes);

	/**
	 * readBinaryToFile
	 * @param fileName name of file
	 * @return true if success else false
	 */
	int readBinaryFromFile(const char *fileName);

	/**
	 * Replaces Newline with spaces
	 */
	void replaceNewlineWithSpaces();

	/**
	 * source
	 * Returns a pointer to the string object with the source code
	 */
	const std::string& source() const;

private:
	/**
	 * Disable copy constructor
	 */
	KernelFile(const KernelFile&);

	/**
	 * Disable operator=
	 */
	KernelFile& operator=(const KernelFile&);

	std::string source_;    //!< source code of the CL program
};

/**
 * bifData
 * struct to generate/load binary functionality
 */
struct bifData {
	std::string programPath;
	std::string programFileName;
	std::string kernelName; /**< kernelName name of the kernel */
	std::string flagsFileName; /**< flagFileName flags file for the kernel */
	std::string flagsStr; /**< flagsStr flags string */
	std::string binaryName; /**< binaryName name of the binary */
	size_t numDevices;
	cl_device_id *devices; /**< devices array of device to build kernel for */

	/**
	 * Constructor
	 */
	bifData() :
			kernelName(""), flagsFileName(""), flagsStr(""), binaryName(""), numDevices(
					0), devices(nullptr) {
	}
};

/**
 * buildProgramData
 * struct to build the kernel
 */
struct buildProgramData {
	std::string programName; /**< name of the program */
	std::string programPath;
	std::string flagsFileName; /**< flagFileName name of the file of flags */
	std::string flagsStr; /**< flagsStr flags string */
	std::string binaryName; /**< binaryName name of the binary */
	cl_device_id device; /**< devices array of device to build kernel for */

	buildProgramData() :
			programName(""), programPath(""), flagsFileName(""), flagsStr(""), binaryName(
					""), device(0) {
	}
};

/**
 * getOpencLErrorCodeStr
 * global function to get corresponding string for a error code
 * @param input the error code
 * @return const char* the string for the error code
 */
const char* getOpenCLErrorCodeStr(std::string input);

template<typename T>
static const char* getOpenCLErrorCodeStr(T input) {
	int errorCode = (int) input;
	switch (errorCode) {
	case CL_DEVICE_NOT_FOUND:
		return "CL_DEVICE_NOT_FOUND";
	case CL_DEVICE_NOT_AVAILABLE:
		return "CL_DEVICE_NOT_AVAILABLE";
	case CL_COMPILER_NOT_AVAILABLE:
		return "CL_COMPILER_NOT_AVAILABLE";
	case CL_MEM_OBJECT_ALLOCATION_FAILURE:
		return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case CL_OUT_OF_RESOURCES:
		return "CL_OUT_OF_RESOURCES";
	case CL_OUT_OF_HOST_MEMORY:
		return "CL_OUT_OF_HOST_MEMORY";
	case CL_PROFILING_INFO_NOT_AVAILABLE:
		return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case CL_MEM_COPY_OVERLAP:
		return "CL_MEM_COPY_OVERLAP";
	case CL_IMAGE_FORMAT_MISMATCH:
		return "CL_IMAGE_FORMAT_MISMATCH";
	case CL_IMAGE_FORMAT_NOT_SUPPORTED:
		return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case CL_BUILD_PROGRAM_FAILURE:
		return "CL_BUILD_PROGRAM_FAILURE";
	case CL_MAP_FAILURE:
		return "CL_MAP_FAILURE";
	case CL_MISALIGNED_SUB_BUFFER_OFFSET:
		return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
	case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
		return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
	case CL_INVALID_VALUE:
		return "CL_INVALID_VALUE";
	case CL_INVALID_DEVICE_TYPE:
		return "CL_INVALID_DEVICE_TYPE";
	case CL_INVALID_PLATFORM:
		return "CL_INVALID_PLATFORM";
	case CL_INVALID_DEVICE:
		return "CL_INVALID_DEVICE";
	case CL_INVALID_CONTEXT:
		return "CL_INVALID_CONTEXT";
	case CL_INVALID_QUEUE_PROPERTIES:
		return "CL_INVALID_QUEUE_PROPERTIES";
	case CL_INVALID_COMMAND_QUEUE:
		return "CL_INVALID_COMMAND_QUEUE";
	case CL_INVALID_HOST_PTR:
		return "CL_INVALID_HOST_PTR";
	case CL_INVALID_MEM_OBJECT:
		return "CL_INVALID_MEM_OBJECT";
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
		return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case CL_INVALID_IMAGE_SIZE:
		return "CL_INVALID_IMAGE_SIZE";
	case CL_INVALID_SAMPLER:
		return "CL_INVALID_SAMPLER";
	case CL_INVALID_BINARY:
		return "CL_INVALID_BINARY";
	case CL_INVALID_BUILD_OPTIONS:
		return "CL_INVALID_BUILD_OPTIONS";
	case CL_INVALID_PROGRAM:
		return "CL_INVALID_PROGRAM";
	case CL_INVALID_PROGRAM_EXECUTABLE:
		return "CL_INVALID_PROGRAM_EXECUTABLE";
	case CL_INVALID_KERNEL_NAME:
		return "CL_INVALID_KERNEL_NAME";
	case CL_INVALID_KERNEL_DEFINITION:
		return "CL_INVALID_KERNEL_DEFINITION";
	case CL_INVALID_KERNEL:
		return "CL_INVALID_KERNEL";
	default:
		return "unknown error code";
	}
}

/**
 * checkVal
 * Set default(isAPIerror) parameter to false
 * if checkVaul is used to check other than OpenCL API error code
 */
template<typename T>
static int checkVal(T input, T reference, std::string message, bool isAPIerror =
		true) {
	if (input == reference) {
		return SUCCESS;
	} else {
		if (isAPIerror) {
			std::cout << "Error: " << message << ". Error code : ";
			std::cout << getOpenCLErrorCodeStr(input) << std::endl;
		} else {
			error(message);
		}
		return FAILURE;
	}
}
/**
 * display devices
 * displays the devices in a platform
 * @param platform cl_platform_id
 * @param deviceType deviceType
 * @return 0 if success else nonzero
 */
int displayDevices(cl_platform_id platform, cl_device_type deviceType);

/**
 * generateBinaryImage
 * geenrate Binary for a kernel
 * @param binaryData bifdata object
 * @return 0 if success else nonzero
 */
int generateBinaryImage(const bifData &binaryData);

/**
 * getDefaultPlatform
 * selects default platform
 * @param numPlatforms Number of Available platforms
 * @param platforms an array of cl_platform_id
 * @param platform cl_platform_id
 * @param dType cl_device_type
 * @return 0 if success else nonzero
 */
bool getDefaultPlatform(cl_uint numPlatforms, cl_platform_id *platforms,
		cl_platform_id &platform, cl_device_type dType, bool verbose);

/**
 * getPlatform
 * selects intended platform
 * @param platform cl_platform_id
 * @param platformId platform Number
 * @param platformIdEnabled if Platform option used
 * @return 0 if success else nonzero
 */
int getPlatformL(cl_platform_id &platform, int platformId,
		bool platformIdEnabled, cl_device_type dType, bool verbose);

/**
 * getDevices
 * selects intended device
 * @param context cl_context object
 * @param devices cl_device_id pointer to hold array of deviceIds
 * @param deviceId device Number
 * @param deviceIdEnabled if DeviceId option used
 * @return 0 if success else nonzero
 */
int getDevices(cl_context &context, cl_device_id **devices,
		size_t *deviceCount);

/**
 * buildOpenCLProgram
 * builds the OpenCL program
 * @param program program object
 * @param context cl_context object
 * @param buildData buildProgramData Object
 * @return 0 if success else nonzero
 */
int buildOpenCLProgram(cl_program &program, const cl_context &context,
		const buildProgramData &buildData);

/**
 * DeviceInfo
 * class implements the functionality to query
 * various Device related parameters
 */
class DeviceInfo {
public:
	cl_device_type dType; /**< dType device type*/
	cl_uint venderId; /**< vendorId VendorId of device*/
	cl_uint maxComputeUnits; /**< maxComputeUnits maxComputeUnits of device*/
	cl_uint maxWorkItemDims; /**< maxWorkItemDims maxWorkItemDimensions VendorId of device*/
	size_t *maxWorkItemSizes; /**< maxWorkItemSizes maxWorkItemSizes of device*/
	size_t maxWorkGroupSize; /**< maxWorkGroupSize max WorkGroup Size of device*/
	cl_uint preferredCharVecWidth; /**< preferredCharVecWidth preferred Char VecWidth of device*/
	cl_uint preferredShortVecWidth; /**< preferredShortVecWidth preferred Short VecWidth of device*/
	cl_uint preferredIntVecWidth; /**< preferredIntVecWidth preferred Int VecWidth of device*/
	cl_uint preferredLongVecWidth; /**< preferredLongVecWidth preferred Long VecWidth of device*/
	cl_uint preferredFloatVecWidth; /**< preferredFloatVecWidth preferredFloatVecWidth of device*/
	cl_uint preferredDoubleVecWidth; /**< preferredDoubleVecWidth preferred Double VecWidth of device*/
	cl_uint preferredHalfVecWidth; /**< preferredHalfVecWidth preferred Half VecWidth of device*/
	cl_uint nativeCharVecWidth; /**< nativeCharVecWidth native Char VecWidth of device*/
	cl_uint nativeShortVecWidth; /**< nativeShortVecWidth nativeShortVecWidth of device*/
	cl_uint nativeIntVecWidth; /**< nativeIntVecWidth nativeIntVecWidth of device*/
	cl_uint nativeLongVecWidth; /**< nativeLongVecWidth nativeLongVecWidth of device*/
	cl_uint nativeFloatVecWidth; /**< nativeFloatVecWidth native Float VecWidth of device*/
	cl_uint nativeDoubleVecWidth; /**< nativeDoubleVecWidth native Double VecWidth of device*/
	cl_uint nativeHalfVecWidth; /**< nativeHalfVecWidth native Half VecWidth of device*/
	cl_uint maxClockFrequency; /**< maxClockFrequency max Clock Frequency of device*/
	cl_uint addressBits; /**< addressBits address Bits of device*/
	cl_ulong maxMemAllocSize; /**< maxMemAllocSize max Mem Alloc Size of device*/
	cl_bool imageSupport; /**< imageSupport image Support of device*/
	cl_uint maxReadImageArgs; /**< maxReadImageArgs max ReadImage Args of device*/
	cl_uint maxWriteImageArgs; /**< maxWriteImageArgs max Write Image Args of device*/
	size_t image2dMaxWidth; /**< image2dMaxWidth image 2dMax Width of device*/
	size_t image2dMaxHeight; /**< image2dMaxHeight image 2dMax Height of device*/
	size_t image3dMaxWidth; /**< image3dMaxWidth image3d MaxWidth of device*/
	size_t image3dMaxHeight; /**< image3dMaxHeight image 3dMax Height of device*/
	size_t image3dMaxDepth; /**< image3dMaxDepth image 3dMax Depth of device*/
	size_t maxSamplers; /**< maxSamplers maxSamplers of device*/
	size_t maxParameterSize; /**< maxParameterSize maxParameterSize of device*/
	cl_uint memBaseAddressAlign; /**< memBaseAddressAlign memBase AddressAlign of device*/
	cl_uint minDataTypeAlignSize; /**< minDataTypeAlignSize minDataType AlignSize of device*/
	cl_device_fp_config singleFpConfig; /**< singleFpConfig singleFpConfig of device*/
	cl_device_fp_config doubleFpConfig; /**< doubleFpConfig doubleFpConfig of device*/
	cl_device_mem_cache_type globleMemCacheType; /**< globleMemCacheType globleMem CacheType of device*/
	cl_uint globalMemCachelineSize; /**< globalMemCachelineSize globalMem Cacheline Size of device*/
	cl_ulong globalMemCacheSize; /**< globalMemCacheSize globalMem CacheSize of device*/
	cl_ulong globalMemSize; /**< globalMemSize globalMem Size of device*/
	cl_ulong maxConstBufSize; /**< maxConstBufSize maxConst BufSize of device*/
	cl_uint maxConstArgs; /**< maxConstArgs max ConstArgs of device*/
	cl_device_local_mem_type localMemType;/**< localMemType local MemType of device*/
	cl_ulong localMemSize; /**< localMemSize localMem Size of device*/
	cl_bool errCorrectionSupport; /**< errCorrectionSupport errCorrectionSupport of device*/
	cl_bool hostUnifiedMem; /**< hostUnifiedMem hostUnifiedMem of device*/
	size_t timerResolution; /**< timerResolution timerResolution of device*/
	cl_bool endianLittle; /**< endianLittle endian Little of device*/
	cl_bool available; /**< available available of device*/
	cl_bool compilerAvailable; /**< compilerAvailable compilerAvailable of device*/
	cl_device_exec_capabilities execCapabilities;/**< execCapabilities exec Capabilities of device*/
	cl_command_queue_properties queueProperties;/**< queueProperties queueProperties of device*/
	cl_platform_id platform; /**< platform platform of device*/
	char *name; /**< name name of device*/
	char *vendorName; /**< venderName vender Name of device*/
	char *driverVersion; /**< driverVersion driver Version of device*/
	char *profileType; /**< profileType profile Type of device*/
	char *deviceVersion; /**< deviceVersion device Version of device*/
	char *openclCVersion; /**< openclCVersion opencl C Version of device*/
	char *extensions; /**< extensions extensions of device*/

#ifdef CL_VERSION_2_0
	cl_device_svm_capabilities svmcaps; /**< SVM Capabilities of device*/
	cl_uint maxQueueSize; /**< MAXIMUM QUEUE SIZE*/
#endif
	DeviceInfo();
	~DeviceInfo();

	/**
	 * setKernelWorkGroupInfo
	 * Set all information for a given device id
	 * @param deviceId deviceID
	 * @return 0 if success else nonzero
	 */
	int setDeviceInfo(cl_device_id deviceId);

	/**
	 * detectSVM
	 * Check if the device supports Shared virtual memory(SVM)
	 * @return bool
	 */
	bool detectSVM();

	/**
	 * detectOpenCL2_xCompatibility
	 * Check if the device supports OpenCL 2.x
	 * @return @bool
	 */
	bool checkOpenCL2_XCompatibility();

private:

	/**
	 * checkVal
	 * Templated FunctionCheck whether any error occurred
	 */
	template<typename T>
	int checkVal(T input, T reference, std::string message, bool isAPIerror =
			true) const {
		if (input == reference) {
			return 0;
		} else {
			if (isAPIerror) {
				std::cout << "Error: " << message << ". Error code : ";
				std::cout << getOpenCLErrorCodeStr(input) << std::endl;
			} else {
				std::cout << message;
			}
			return 1;
		}
	}
};

struct DataArgs {
	char *vendorName;                   // preferred OpenCL platform vendor name
	bool preferCpu;               // indicator to create context with CPU device
	bool preferGpu;               // indicator to create context with GPU device
};

class Util {

public:

	// Print useful information to the default output. Same usage as with printf
	static void LogInfo(const char *str, ...);

	// Print error notification to the default output. Same usage as with printf
	static void LogError(const char *str, ...);

	// Find an OpenCL platform from the preferredVendor with the preferred device(s)
	// One and only one of the flags is allowed to be set to true
	static cl_platform_id FindPlatformId(const char *preferredVendor = NULL,
			bool preferCpu = false, bool preferGpu = false, bool preferShared =
					false);

	// Create an OpenCL context using the preferred devices available on the OpenCL platform platformId
	static cl_context CreateContext(cl_platform_id platformId, bool preferCpu =
			false, bool preferGpu = false, bool preferShared = false);

	// Create an OpenCL context with the CPU device available on the OpenCL platform platformId
	static cl_context CreateCPUContext(cl_platform_id platformId);

	// Create an OpenCL context with the GPU device available on the OpenCL platform platformId
	static cl_context CreateGPUContext(cl_platform_id platformId);

	// Create an OpenCL shared context with both CPU and GPU devices available on the OpenCL platform platformId
	static cl_context CreateSharedContext(cl_platform_id platformId);

	// Translate OpenCL numeric error code (errorCode) to a meaningful error string
	static const char* TranslateOpenCLError(cl_int errorCode);

	static cl_int ReleaseMemory(cl_mem memory);

	static cl_event CreateUserEvent(cl_context ctxt);
	static cl_event RetainEvent(cl_event evt);
	static void ReleaseEvent(cl_event evt);
	static void SetEventComplete(cl_event evt);

	static cl_int mapImage(cl_command_queue queue, cl_mem img, bool synchronous,
			cl_map_flags flags, size_t width, size_t height,
			cl_uint numWaitEvents, const cl_event *waitEvents,
			cl_event *completionEvent, void **mappedPtr);

	static cl_int mapBuffer(cl_command_queue queue, cl_mem buffer,
			bool synchronous, cl_map_flags flags, size_t size,
			cl_uint numWaitEvents, const cl_event *waitEvents,
			cl_event *completionEvent, void **mappedPtr);

	static cl_int unmapMemory(cl_command_queue queue, cl_uint numWaitEvents,
			const cl_event *waitEvents, cl_event *completionEvent,
			cl_mem memory, void *mappedPtr);

	static cl_int getRefCount(cl_event evt);

private:

	static cl_int CheckPreferredVendorMatch(cl_platform_id platform,
			const char *preferredVendor, bool *match);

	static cl_int GetPlatformIds(cl_platform_id **platforms,
			cl_uint *numPlatforms);

	static cl_device_type TranslateDeviceType(bool preferCpu, bool preferGpu,
			bool preferShared);

};

}
#endif

