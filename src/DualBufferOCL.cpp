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
#include "DualBufferOCL.h"
#include "UtilOCL.h"
#include <cassert>


namespace ltk {

DualBufferOCL::DualBufferOCL(DeviceOCL *device, size_t len,DualBufferType type, cl_command_queue_properties queue_props) :
		m_type(type),
		queue(new QueueOCL(device, queue_props)),
		hostBuffer(nullptr),
		deviceBuffer(0),
		numBytes(len){
	if (numBytes == 0)
		throw std::exception();
  cl_mem_flags flags = CL_MEM_ALLOC_HOST_PTR;
  if (type == HostToDeviceBuffer){
	  flags |= (CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY);
  } else if (type == DeviceToHostBuffer){
	  flags |= (CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY);
  }
	cl_int error_code = CL_SUCCESS;
	deviceBuffer = clCreateBuffer(device->context, flags, numBytes, hostBuffer,
			&error_code);
	if (CL_SUCCESS != error_code) {
		Util::LogError(
				"Error: clCreateBuffer (CL_QUEUE_CONTEXT) returned %s.\n",
				Util::TranslateOpenCLError(error_code));
		cleanup();
		throw std::exception();
	}
}
DualBufferOCL::~DualBufferOCL() {
	cleanup();
}
unsigned char* DualBufferOCL::getHostBuffer() const {
	return hostBuffer;
}
cl_mem* DualBufferOCL::getDeviceMem() const {
	return (cl_mem*) &deviceBuffer;
}
QueueOCL* DualBufferOCL::getQueue() const {
	return queue;
}
size_t DualBufferOCL::getSize() const {
	return numBytes;
}
void DualBufferOCL::cleanup() {
	delete queue;
	Util::ReleaseMemory(deviceBuffer);
}

bool DualBufferOCL::map(cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list, cl_event *completionEvent,
		bool synchronous) {
	return map(queue, num_events_in_wait_list, event_wait_list, completionEvent,
			synchronous);
}
bool DualBufferOCL::unmap(cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list, cl_event *completionEvent) {

	return unmap(queue, num_events_in_wait_list, event_wait_list,
			completionEvent);
}

bool DualBufferOCL::map(QueueOCL *mapQueue, cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list, cl_event *completionEvent,
		bool synchronous, cl_map_flags flags) {
	cl_int error_code = Util::mapBuffer(mapQueue->getQueueImpl(), deviceBuffer,
			synchronous, flags, numBytes,
			num_events_in_wait_list, event_wait_list, completionEvent,
			(void**) &hostBuffer);
	if (CL_SUCCESS != error_code) {
		Util::LogError(
				"Error: mapDeviceToHost (CL_QUEUE_CONTEXT) returned %s.\n",
				Util::TranslateOpenCLError(error_code));
		return false;
	}
	return true;
}

bool DualBufferOCL::map(QueueOCL *mapQueue, cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list, cl_event *completionEvent,
		bool synchronous) {
	    assert(m_type == HostToDeviceBuffer || m_type == DeviceToHostBuffer);
	    cl_map_flags f = m_type ==  HostToDeviceBuffer ? CL_MAP_WRITE : CL_MAP_READ;
			return map(mapQueue, num_events_in_wait_list,event_wait_list,
					completionEvent, synchronous,f);
}
bool DualBufferOCL::unmap(QueueOCL *mapQueue, cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list, cl_event *completionEvent) {

	cl_int error_code = Util::unmapMemory(mapQueue->getQueueImpl(),
			num_events_in_wait_list, event_wait_list, completionEvent,
			deviceBuffer, hostBuffer);
	if (CL_SUCCESS != error_code) {
		Util::LogError("Error: unmap (CL_QUEUE_CONTEXT) returned %s.\n",
				Util::TranslateOpenCLError(error_code));
	}
	return error_code == CL_SUCCESS;
}
}
#endif
