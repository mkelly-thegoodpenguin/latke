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
#include "platform.h"
#include "QueueOCL.h"
#include "EnqueueInfoOCL.h"


namespace ltk {


EnqueueInfoOCL::EnqueueInfoOCL(QueueOCL *myQueue):
		queue(myQueue),
		dimension(0),
		useOffset(false),
	    numWaitEvents(0),
		needsCompletionEvent(false),
		completionEvent(0)
{
	for (int i = 0; i < 3; ++i){
		local_work_size[i]=0;
		global_work_size[i]=0;
		global_work_offset[i]=0;
	}

}

// push a wait event into the wait events
bool EnqueueInfoOCL::pushWaitEvent(cl_event evt) {
    if (numWaitEvents < MAX_ENQUEUE_WAIT_EVENTS) {
        waitEvents[numWaitEvents++] = evt;
            return true;
    }
    return false;
}

// replace an existing wait event in the wait event array
// Note: index must be less than numWait
bool EnqueueInfoOCL::setWaitEvent(cl_event evt, size_t index) {
    if (index < numWaitEvents) {
        waitEvents[index] = evt;
        return true;
    }
    return false;
}


}
#endif
