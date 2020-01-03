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

namespace ltk {

const size_t MAX_EVENTS = 20;

class QueueOCL;
struct EnqueueInfo;

struct RunInfoOCL {

	RunInfoOCL(QueueOCL* myQueue);

    // push a wait event into the wait events array
	bool pushWaitEvent(cl_event evt);

    // replace an existing wait event in the wait event array
    // Note: index must be less than numWaitEvents
	bool setWaitEvent(cl_event evt, size_t index);
	void copyInto(EnqueueInfo* info);

    QueueOCL* queue;
    cl_uint numWaitEvents;
    cl_event waitEvents[MAX_EVENTS];
	bool needsCompletionEvent;
    cl_event completionEvent;
	uint8_t tileId;
	uint8_t channel;
	uint8_t numChannels;
};

}
#endif
