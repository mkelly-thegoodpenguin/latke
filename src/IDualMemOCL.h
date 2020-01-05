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
#include <memory>
#include "latke_config.h"
#ifdef OPENCL_FOUND
#include "platform.h"
namespace ltk {

class IDualMemOCL {

public:
	virtual ~IDualMemOCL() {
	}
	virtual bool map(cl_uint num_events_in_wait_list,
			const cl_event *event_wait_list, cl_event *completionEvent,
			bool synchronous)=0;
	virtual bool unmap(cl_uint num_events_in_wait_list,
			const cl_event *event_wait_list, cl_event *completionEvent)=0;

	virtual bool map(QueueOCL *mapQueue, cl_uint num_events_in_wait_list,
			const cl_event *event_wait_list, cl_event *completionEvent,
			bool synchronous) = 0;
	virtual bool unmap(QueueOCL *mapQueue, cl_uint num_events_in_wait_list,
			const cl_event *event_wait_list, cl_event *completionEvent) = 0;

	virtual unsigned char* getHostBuffer() const =0;
	virtual cl_mem* getDeviceMem() const =0;
	virtual QueueOCL* getQueue() const=0;

};

template<typename M> struct MemMapEvents {
	MemMapEvents(DeviceOCL *dev, std::unique_ptr<M> *image) :
			mem(image), triggerMemUnmap(Util::CreateUserEvent(dev->context)), memUnmapped(
					0) {
	}
	~MemMapEvents() {
		Util::ReleaseEvent(triggerMemUnmap);
		Util::ReleaseEvent(memUnmapped);
	}

	std::unique_ptr<M> *mem;
	cl_event triggerMemUnmap;
	cl_event memUnmapped;
};



}
#endif
