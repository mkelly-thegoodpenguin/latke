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
#include "latke_config.h"
#ifdef OPENCL_FOUND
#include <vector>
#include "QueueOCL.h"
#include "IDualMemOCL.h"

namespace ltk {

class DualImageOCL: public IDualMemOCL {

public:
	DualImageOCL(DeviceOCL *device, size_t dimX, size_t dimY,
			uint32_t channelOrder, uint32_t dataType, bool hostToDevice, cl_command_queue_properties queue_props);
	~DualImageOCL();

	bool map(cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
			cl_event *completionEvent, bool synchronous);
	bool unmap(cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
			cl_event *completionEvent);

	bool map(QueueOCL *mapQueue, cl_uint num_events_in_wait_list,
			const cl_event *event_wait_list, cl_event *completionEvent,
			bool synchronous);
	bool unmap(QueueOCL *mapQueue, cl_uint num_events_in_wait_list,
			const cl_event *event_wait_list, cl_event *completionEvent);

	size_t getNumBytes() const;
	static size_t getNumBytes(size_t dimX, size_t dimY, uint32_t channelOrder,
			uint32_t dataType);
	unsigned char* getHostBuffer() const;
	cl_mem* getDeviceMem() const;
	size_t getDimX() const;
	size_t getDimY() const;
	QueueOCL* getQueue() const;
private:
	void cleanup();
	bool hostToDevice;
	QueueOCL *queue;
	unsigned char *hostBuffer;
	cl_mem image;
	size_t dimX;
	size_t dimY;
	uint32_t channelOrder;
	uint32_t dataType;
};
}
#endif
