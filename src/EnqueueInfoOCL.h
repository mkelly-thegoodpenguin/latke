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

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "QueueOCL.h"


namespace ltk {


const size_t MAX_ENQUEUE_WAIT_EVENTS = 64;

struct EnqueueInfoOCL {
	EnqueueInfoOCL(QueueOCL *myQueue);

    // push a wait event into the wait events array
	bool pushWaitEvent(cl_event evt);

    // replace an existing wait event in the wait event array
    // Note: index must be less than numWaitEvents
	bool setWaitEvent(cl_event evt, size_t index);

	QueueOCL *queue;
	int dimension;
	size_t global_work_size[3];
	size_t global_work_offset[3];
	bool useOffset;
	size_t local_work_size[3];
	cl_uint numWaitEvents;
	cl_event waitEvents[MAX_ENQUEUE_WAIT_EVENTS];
	bool needsCompletionEvent;
	cl_event completionEvent;
};

}
#endif
