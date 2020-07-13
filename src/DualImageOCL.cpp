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
#include "DualImageOCL.h"
#include "UtilOCL.h"

namespace ltk {
DualImageOCL::DualImageOCL(DeviceOCL *device, size_t dimX, size_t dimY,
		uint32_t channelOrder, uint32_t dataType, bool doHostToDevice, std::vector<uint64_t> queue_props) :
		    hostToDevice(doHostToDevice), queue(new QueueOCL(device, queue_props)),
		    hostBuffer(nullptr),
		    image(0),
		    dimX(dimX),
		    dimY(dimY),
		    channelOrder(channelOrder),
		    dataType(dataType) {
	if (dimX == 0 && dimY == 0)
		throw std::exception();

	cl_mem_flags flags = CL_MEM_ALLOC_HOST_PTR;
	flags |=
			hostToDevice ?
					(CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY) :
					(CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY);

	cl_int error_code = CL_SUCCESS;
	cl_image_desc desc;
	desc.image_type = CL_MEM_OBJECT_IMAGE2D;
	desc.image_width = dimX;
	desc.image_height = dimY;
	desc.image_depth = 0;
	desc.image_array_size = 0;
	desc.image_row_pitch = 0;
	desc.image_slice_pitch = 0;
	desc.num_mip_levels = 0;
	desc.num_samples = 0;
	desc.buffer = NULL;

	cl_image_format format;
	format.image_channel_order = channelOrder;
	format.image_channel_data_type = dataType;

	image = clCreateImage(device->context, flags, &format, &desc, hostBuffer,
			&error_code);
	if (CL_SUCCESS != error_code) {
		Util::LogError("Error: clCreateImage (CL_QUEUE_CONTEXT) returned %s.\n",
				Util::TranslateOpenCLError(error_code));
		cleanup();
		throw std::exception();
	}
}
DualImageOCL::~DualImageOCL() {
	cleanup();
}
unsigned char* DualImageOCL::getHostBuffer() const {
	return hostBuffer;
}
QueueOCL* DualImageOCL::getQueue() const {
	return queue;
}

cl_mem* DualImageOCL::getDeviceMem() const {
	return (cl_mem*) &image;
}

size_t DualImageOCL::getDimX() const {
	return dimX;
}
size_t DualImageOCL::getDimY() const {
	return dimY;
}
void DualImageOCL::cleanup() {
	delete queue;
	Util::ReleaseMemory(image);
}
size_t DualImageOCL::getNumBytes() const {
	return getNumBytes(dimX, dimY, channelOrder, dataType);
}
size_t DualImageOCL::getNumBytes(size_t dimX, size_t dimY,
		uint32_t channelOrder, uint32_t dataType) {
	size_t typeSize = 1;
	switch (dataType) {
	case CL_UNSIGNED_INT8:
	case CL_SIGNED_INT8:
		typeSize = 1;
		break;
	case CL_UNSIGNED_INT16:
	case CL_SIGNED_INT16:
		typeSize = 2;
		break;
	case CL_UNSIGNED_INT32:
	case CL_SIGNED_INT32:
		typeSize = 4;
		break;
	}

	size_t numChannels = 1;
	switch (channelOrder) {
	case CL_R:
		numChannels = 1;
		break;
	case CL_RGB:
		numChannels = 3;
		break;
	case CL_RGBA:
	case CL_BGRA:
		numChannels = 4;
		break;
	}

	return dimX * dimY * typeSize * numChannels;
}

bool DualImageOCL::map(cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list, cl_event *completionEvent,
		bool synchronous) {
	return map(queue, num_events_in_wait_list, event_wait_list, completionEvent,
			synchronous);
}
bool DualImageOCL::unmap(cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list, cl_event *completionEvent) {
	return unmap(queue, num_events_in_wait_list, event_wait_list,
			completionEvent);
}

bool DualImageOCL::map(QueueOCL *mapQueue, cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list, cl_event *completionEvent,
		bool synchronous) {

	cl_int error_code = Util::mapImage(mapQueue->getQueueImpl(), image,
			synchronous, hostToDevice ? CL_MAP_WRITE : CL_MAP_READ, dimX, dimY,
			num_events_in_wait_list, event_wait_list, completionEvent,
			(void**) &hostBuffer);
	if (CL_SUCCESS != error_code) {
		Util::LogError("Error: map (CL_QUEUE_CONTEXT) returned %s.\n",
				Util::TranslateOpenCLError(error_code));
		return false;
	}
	return true;
}
bool DualImageOCL::unmap(QueueOCL *mapQueue, cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list, cl_event *completionEvent) {
	cl_int error_code = Util::unmapMemory(mapQueue->getQueueImpl(),
			num_events_in_wait_list, event_wait_list, completionEvent, image,
			hostBuffer);
	if (CL_SUCCESS != error_code) {
		Util::LogError("Error: unmap (CL_QUEUE_CONTEXT) returned %s.\n",
				Util::TranslateOpenCLError(error_code));
	}
	return error_code == CL_SUCCESS;
}
}
#endif
