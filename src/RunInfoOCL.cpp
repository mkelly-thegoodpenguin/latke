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
#include "DeviceOCL.h"
#include "DeviceManagerOCL.h"
#include "DualBufferOCL.h"
#include "DualImageOCL.h"
#include "platform.h"
#include "QueueOCL.h"
#include "UtilOCL.h"
#include "KernelOCL.h"
#include "RunInfoOCL.h"


namespace ltk {

RunInfoOCL::RunInfoOCL(	QueueOCL* myQueue) :
    queue(myQueue),
    numWaitEvents(0),
	needsCompletionEvent(false),
	completionEvent(0),
	tileId(0),
	channel(0),
	numChannels(0)
{}

// push a wait event into the wait events array
bool RunInfoOCL::pushWaitEvent(cl_event evt) {
    if (numWaitEvents < MAX_EVENTS) {
        waitEvents[numWaitEvents++] = evt;
            return true;
    }
    return false;
}

// replace an existing wait event in the wait event array
// Note: index must be less than numWaitEvents
bool RunInfoOCL::setWaitEvent(cl_event evt, size_t index) {
    if (index < numWaitEvents) {
        waitEvents[index] = evt;
        return true;
    }
    return false;
}

void RunInfoOCL::copyInto(EnqueueInfo* info) {
	info->queue = queue->getQueueImpl();
	info->num_events_in_wait_list = numWaitEvents;
	info->event_wait_list = numWaitEvents ? waitEvents : NULL;
	info->needsCompletionEvent = needsCompletionEvent;
}

}
#endif
