#pragma once


#include <iostream>
#include <sstream>
#include "latke.h"
#include "BlockingQueue.h"
#include <math.h>
#include <chrono>
#include <cassert>
#include "ArchFactory.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include <string>
#include "ThreadPool.h"

using namespace ltk;

enum pattern_t{
    RGGB = 0,
    GRBG = 1,
    GBRG = 2,
    BGGR = 3
};

template<typename M> struct JobInfo {
	JobInfo(DeviceOCL *dev, std::shared_ptr<M> hostToDev,
			std::shared_ptr<M> devToHost, JobInfo *previous) :
			hostToDevice(new MemMapEvents<M>(dev, hostToDev)), kernelCompleted(
					0), deviceToHost(new MemMapEvents<M>(dev, devToHost)), prev(
					previous) {
	}
	~JobInfo() {
		delete hostToDevice;
		Util::ReleaseEvent(kernelCompleted);
		delete deviceToHost;
	}

	MemMapEvents<M> *hostToDevice;
	cl_event kernelCompleted;
	MemMapEvents<M> *deviceToHost;

	JobInfo *prev;
};

const uint32_t numBuffers =16;
const int numImages = 4;
const int numBatches = numBuffers / numImages;
const int numPostProcBuffers = 16;

const int tile_rows = 5;
const int tile_columns = 32;

typedef void (CL_CALLBACK  *pfn_event_notify) (cl_event event, cl_int event_command_exec_status, void *user_data);

class BufferAllocater{
public:
	BufferAllocater(DeviceOCL *dev, size_t dimX, size_t dimY, size_t bps) :
		m_dev(dev), m_dimX(dimX), m_dimY(dimY), m_bps(bps){

	}
	std::unique_ptr<DualBufferOCL> allocate(bool hostToDevice){
		return std::make_unique<DualBufferOCL>(m_dev, m_dimX * m_dimY * m_bps, hostToDevice);
	}
private:
	DeviceOCL *m_dev;
	size_t m_dimX;
	size_t m_dimY;
	size_t m_bps;
};

class ImageAllocater{
public:
	ImageAllocater(DeviceOCL *dev, size_t dimX, size_t dimY, size_t bps) :
		m_dev(dev), m_dimX(dimX), m_dimY(dimY), m_bps(bps){

	}
	std::unique_ptr<DualImageOCL> allocate(bool hostToDevice){
		return std::make_unique<DualImageOCL>(m_dev, m_dimX, m_dimY, (m_bps == 1 ? CL_R : CL_RGBA), CL_UNSIGNED_INT8, hostToDevice);
	}
private:
	DeviceOCL *m_dev;
	size_t m_dimX;
	size_t m_dimY;
	size_t m_bps;
};


template<typename M, typename A> struct Debayer {
	int debayer(int argc, char *argv[],
			pfn_event_notify HostToDeviceMappedCallback,
			pfn_event_notify DeviceToHostMappedCallback,
			std::string kernelFile);
	BlockingQueue<JobInfo<M>*> mappedHostToDeviceQueue;
	BlockingQueue<JobInfo<M>*> mappedDeviceToHostQueue;
};


