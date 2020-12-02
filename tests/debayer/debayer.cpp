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
#include "common.h"
#include <cmath>

// template struct to handle debayer to either image or buffer
template<typename M, typename A> struct Debayer {
	int debayer(int argc, char *argv[],
			pfn_event_notify HostToDeviceMappedCallback,
			pfn_event_notify DeviceToHostMappedCallback,
			std::string kernelFile);
	BlockingQueue<JobInfo<M>*> mappedHostToDeviceQueue;
	BlockingQueue<JobInfo<M>*> mappedDeviceToHostQueue;
};

enum pattern_t {
	RGGB = 0, GRBG = 1, GBRG = 2, BGGR = 3
};

const int numCLBuffers = 4;
const int numPostProcBuffers = 16;
const int tile_rows = 5;
const int tile_columns = 32;
const int platformId = 0;
const eDeviceType deviceType = GPU;
const int deviceNum = 0;

inline char separator()
{
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

template<typename M, typename A> int Debayer<M, A>::debayer(int argc,
		char *argv[], pfn_event_notify HostToDeviceMappedCallback,
		pfn_event_notify DeviceToHostMappedCallback, std::string kernelFile) {


	CmdLine cmd("debayer command line", ' ',"v1.0");

	ValueArg<std::string> inputDirArg("i", "input-dir", "Input Image Directory", false,
			"", "string", cmd);

	ValueArg<std::string> outputDirArg("o", "output-dir", "Output Image Directory", false,
			"", "string", cmd);

	ValueArg<std::string> patternArg("p", "pattern", "Bayer Pattern", false,
			"", "string", cmd);

	cmd.parse(argc, argv);


	if (!inputDirArg.isSet()) {
		std::cerr << "Required image directory missing";
		return -1;
	}

	std::string inputDir = inputDirArg.getValue();
	std::string outputDir = inputDir;
	if (outputDirArg.isSet())
		outputDir = outputDirArg.getValue();

	// set up directory iterator
	auto dir = opendir(inputDir.c_str());
	if (!dir) {
		std::cerr << "Unable to open image directory " << inputDir;
		return -1;
	}
	struct dirent *content = nullptr;
	std::string inputFile;
	BlockingQueue<std::string> imageQueue;
	while ((content = readdir(dir)) != nullptr) {
		if (strcmp(".", content->d_name) == 0
				|| strcmp("..", content->d_name) == 0)
			continue;
		inputFile = content->d_name;
		imageQueue.push(content->d_name);
	}
	closedir(dir);
	uint32_t numImages = (imageQueue.size()/numCLBuffers) * numCLBuffers;
	int numBatches = numImages / numCLBuffers;

	// read first image in to get image dimensions
	int width = 0, height = 0, channels = 0;
	std::string inputFileFull = inputDir + separator() + inputFile.c_str();
	auto image = stbi_load(inputFileFull.c_str(), &width, &height, &channels,
			STBI_default);
	if (!image) {
		std::cerr << "Failed to read image file " << inputFile;
		return -1;
	}
	stbi_image_free(image);

	uint32_t bufferWidth = width;
	uint32_t bufferHeight = height;

	int bayer_pattern = RGGB;

	if (patternArg.isSet()) {
		std::string patt = patternArg.getValue();
		if (patt == "GRBG")
			bayer_pattern = GRBG;
		else if (patt == "GBRG")
			bayer_pattern = GBRG;
		else if (patt == "BGGR")
			bayer_pattern = BGGR;
		else
			std::cout << "Unrecognized bayer pattern " << patt << ". Using RGGB." << std::endl;
	}

	uint32_t bps_out = 4;
	uint32_t bufferPitch = bufferWidth;
	uint32_t frameSize = bufferPitch * bufferHeight;
	uint32_t bufferPitchOut = bufferWidth * bps_out;
	uint32_t frameSizeOut = bufferPitchOut * bufferHeight;

	uint8_t *postProcBuffers[numPostProcBuffers];
	BlockingQueue<uint8_t*> availableBuffers;
	for (int i = 0; i < numPostProcBuffers; ++i) {
		postProcBuffers[i] = new uint8_t[frameSizeOut];
		availableBuffers.push(postProcBuffers[i]);
	}
  cl_command_queue_properties queue_props = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;

	// 1. create device manager
	auto deviceManager = std::make_shared<DeviceManagerOCL>(true);
	auto success = deviceManager->init(platformId, deviceType, deviceNum, true, queue_props);
	if (success != DeviceSuccess) {
		std::cerr << "Failed to initialize OpenCL device";
		return -1;
	}

	auto dev = deviceManager->getDevice(deviceNum);

	auto arch = ArchFactory::getArchitecture(dev->deviceInfo->venderId);
	if (!arch){
	    std::cerr << "Unsupported OpenCL vendor ID " << dev->deviceInfo->venderId;
	    return -1;
	}

	std::shared_ptr<M> hostToDevice[numCLBuffers];
	std::shared_ptr<M> deviceToHost[numCLBuffers];
	std::shared_ptr<QueueOCL> kernelQueue[numCLBuffers];
	JobInfo<M> *currentJobInfo[numCLBuffers];
	JobInfo<M> *prevJobInfo[numCLBuffers];

	std::stringstream buildOptions;
	buildOptions << " -I ./ ";
	buildOptions << " -D TILE_ROWS=" << tile_rows;
	buildOptions << " -D TILE_COLS=" << tile_columns;
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
	buildOptions << " -D OUTPUT_CHANNELS=" << bps_out;
	buildOptions << arch->getBuildOptions();
	//buildOptions << " -D DEBUG";

	KernelInitInfoBase initInfoBase(dev, buildOptions.str(), "",
	BUILD_BINARY_IN_MEMORY);
	KernelInitInfo initInfo(initInfoBase, kernelFile, "debayer",
			"malvar_he_cutler_demosaic");
	std::shared_ptr<KernelOCL> kernel;
	try {
		kernel = std::make_unique<KernelOCL>(initInfo);
	} catch (std::runtime_error &re) {
		std::cerr << "Unable to build kernel. Exiting" << std::endl;
		return -1;
	}

	A allocator(dev, bufferWidth, bufferHeight, 1, CL_UNSIGNED_INT8, queue_props);
	A allocatorOut(dev, bufferWidth, bufferHeight, 4, CL_UNSIGNED_INT8, queue_props);
	for (int i = 0; i < numCLBuffers; ++i) {
		hostToDevice[i] = allocator.allocate(true);
		deviceToHost[i] = allocatorOut.allocate(false);
		kernelQueue[i] = std::make_unique<QueueOCL>(dev,queue_props);
		currentJobInfo[i] = nullptr;
		prevJobInfo[i] = nullptr;
	}

	// queue all kernel runs
	for (int j = 0; j < numBatches; j++) {
		for (int i = 0; i < numCLBuffers; ++i) {
			bool lastBatch = j == numBatches - 1;
			auto prev = currentJobInfo[i];
			currentJobInfo[i] = new JobInfo<M>(dev, hostToDevice[i],
												deviceToHost[i], prevJobInfo[i]);
			prevJobInfo[i] = prev;

			// map
			// (wait for previous kernel to complete)
			cl_event hostToDeviceMapped;
			if (!hostToDevice[i]->map(prev ? 1 : 0,
										prev ? &prev->kernelCompleted : nullptr,
										&hostToDeviceMapped, false)) {
				return -1;
			}
			// set callback, which will add this image
			// info to host-side queue of mapped buffers
			auto error_code = clSetEventCallback(hostToDeviceMapped,
			CL_COMPLETE, HostToDeviceMappedCallback, currentJobInfo[i]);
			if (DeviceSuccess != error_code) {
				Util::LogError("Error: clSetEventCallback returned %s.\n",
						Util::TranslateOpenCLError(error_code));
				return -1;
			}
			Util::ReleaseEvent(hostToDeviceMapped);

			// unmap
			if (!hostToDevice[i]->unmap(1,
										&currentJobInfo[i]->hostToDevice->triggerMemUnmap,
										&currentJobInfo[i]->hostToDevice->memUnmapped)) {
				return -1;
			}

			kernel->pushArg<cl_uint>(&bufferHeight);
			kernel->pushArg<cl_uint>(&bufferWidth);
			kernel->pushArg<cl_mem>(hostToDevice[i]->getDeviceMem());
			kernel->pushArg<cl_uint>(&bufferPitch);
			kernel->pushArg<cl_mem>(deviceToHost[i]->getDeviceMem());
			kernel->pushArg<cl_uint>(&bufferPitchOut);
			kernel->pushArg<cl_int>(&bayer_pattern);

			EnqueueInfoOCL info(kernelQueue[i].get());
			info.dimension = 2;
			info.local_work_size[0] = tile_columns;
			info.local_work_size[1] = tile_rows;
			info.global_work_size[0] = (size_t) std::ceil(
					bufferWidth / (double) tile_columns)
					* info.local_work_size[0];
			info.global_work_size[1] = (size_t) std::ceil(
					bufferHeight / (double) tile_rows)
					* info.local_work_size[1];
			info.needsCompletionEvent = true;
			info.pushWaitEvent(currentJobInfo[i]->hostToDevice->memUnmapped);
			// wait for unmapping of previous deviceToHost
			if (prev)
				info.pushWaitEvent(prev->hostToDevice->memUnmapped);
			try {
				kernel->enqueue(info);
			} catch (std::exception &ex) {
				// todo: handle exception
			}
			currentJobInfo[i]->kernelCompleted = info.completionEvent;

			// map
			cl_event deviceToHostMapped;
			if (!deviceToHost[i]->map(1, &currentJobInfo[i]->kernelCompleted,
					&deviceToHostMapped, false)) {
				return -1;
			}
			// set callback on mapping
			error_code = clSetEventCallback(deviceToHostMapped,
											CL_COMPLETE,
											DeviceToHostMappedCallback,
											currentJobInfo[i]);
			if (DeviceSuccess != error_code) {
				Util::LogError("Error: clSetEventCallback returned %s.\n",
						Util::TranslateOpenCLError(error_code));
				return -1;
			}
			Util::ReleaseEvent(deviceToHostMapped);

			// unmap (except last batch)
			if (!lastBatch) {
				if (!deviceToHost[i]->unmap(1,
											&currentJobInfo[i]->deviceToHost->triggerMemUnmap,
											&currentJobInfo[i]->deviceToHost->memUnmapped)) {
					return -1;
				}
			}
		}
	}

	// wait for cl memory objects from queue, fill them, and trigger unmap event
	auto start = std::chrono::high_resolution_clock::now();
	std::thread pushImages([this, frameSize, numImages, &imageQueue, inputDir]() {
		JobInfo<M> *info = nullptr;
		int count = 0;
		while (mappedHostToDeviceQueue.waitAndPop(info)) {
			std::string fname;
			imageQueue.waitAndPop(fname);
			info->fileName = fname;
			int width = 0, height = 0, channels = 0;
			fname = inputDir + separator() + fname;
			auto image = stbi_load(fname.c_str(), &width, &height, &channels,	STBI_default);
			memcpy(info->hostToDevice->mem->getHostBuffer(), image, frameSize);
			stbi_image_free(image);
			// trigger unmap, allowing current kernel to proceed
			Util::SetEventComplete(info->hostToDevice->triggerMemUnmap);
			if (++count == numImages)
				break;
		}
	});

	// wait for processed images from queue, handle them,
	// and trigger unmap event
	std::mutex postMutex;
	std::condition_variable postCondition;
	auto postProcPool = new ThreadPool(std::thread::hardware_concurrency());
	std::thread pullImages([this, frameSizeOut, &postProcPool, bufferWidth, bufferHeight,
							bps_out, &availableBuffers, outputDir, numImages,
							&postCondition, &postMutex]() {
		JobInfo<M> *info = nullptr;
		int pullCount;
		std::atomic<int> postCount(0);
		while (mappedDeviceToHostQueue.waitAndPop(info)) {
			uint8_t *buf;
			if (availableBuffers.waitAndPop(buf)) {
				memcpy(buf, info->deviceToHost->mem->getHostBuffer(),frameSizeOut);
				auto evt = [buf, bufferWidth, bufferHeight,
							bps_out, &availableBuffers, info, outputDir, numImages,
							&postCondition, &postMutex, &postCount] {
					std::stringstream f;
					f << outputDir << separator() << info->fileName << ".png";
					stbi_write_png(f.str().c_str(), bufferWidth, bufferHeight, bps_out,buf, bufferWidth*bps_out);
					availableBuffers.push(buf);
					if (++postCount == numImages){
						std::lock_guard<std::mutex> lk(postMutex);
						postCondition.notify_one();
					}
				};
				postProcPool->enqueue(evt);
			} else {
				std::cout << "mappedDeviceToHostQueue failed" << std::endl;
			}
			// trigger unmap, allowing next kernel to proceed
			Util::SetEventComplete(info->deviceToHost->triggerMemUnmap);

			// cleanup
			delete info->prev;
			info->prev = nullptr;
			if (++pullCount == numImages){
				break;
			}
		}
	});
	std::unique_lock<std::mutex> lk(postMutex);
	postCondition.wait(lk);
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;

	// cleanup
	pushImages.join();
	pullImages.join();
	delete postProcPool;
	for (int i = 0; i < numPostProcBuffers; ++i)
		delete[] postProcBuffers[i];
	for (int i = 0; i < numCLBuffers; ++i) {
		deviceToHost[i]->unmap(0, nullptr, nullptr);
		delete currentJobInfo[i]->prev;
		delete currentJobInfo[i];
	}
	delete arch;
	fprintf(stdout, "opencl processing time per image = %f ms\n",
			(elapsed.count() * 1000) / (double) numImages);

	return 0;
}
