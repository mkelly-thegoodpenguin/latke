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
#include "QueueOCL.h"
#include "IDualMemOCL.h"
namespace ltk {


class DualBufferOCL: public IDualMemOCL {

public:
	DualBufferOCL(DeviceOCL *device, size_t len, bool hostToDevice, cl_command_queue_properties queue_props);
	~DualBufferOCL();

	bool map(cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
			cl_event *completionEvent, bool synchronous);
	bool unmap(cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
			cl_event *completionEvent);

	bool map(QueueOCL *mapQueue, cl_uint num_events_in_wait_list,
			const cl_event *event_wait_list, cl_event *completionEvent,
			bool synchronous);
	bool unmap(QueueOCL *mapQueue, cl_uint num_events_in_wait_list,
			const cl_event *event_wait_list, cl_event *completionEvent);

	unsigned char* getHostBuffer() const;
	cl_mem* getDeviceMem() const;
	size_t getSize() const;
	QueueOCL* getQueue() const;
private:
	void cleanup();
	bool hostToDevice;
	QueueOCL *queue;
	unsigned char *hostBuffer;
	cl_mem deviceBuffer;
	size_t numBytes;
};
}
#endif
