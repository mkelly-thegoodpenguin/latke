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

#ifdef OPENCL_FOUND
#include "platform.h"

namespace ltk {


class QueueOCL
{
public:
    QueueOCL(QueueOCL& rhs);
    QueueOCL(cl_command_queue cmdQueue);
    QueueOCL(DeviceOCL* device);
    ~QueueOCL(void);
    tDeviceRC finish();
    tDeviceRC flush();

	static tDeviceRC flush(QueueOCL* queue);
	static tDeviceRC flush(cl_command_queue commandQueue);

    static tDeviceRC finish(QueueOCL* queue);
    static tDeviceRC finish(cl_command_queue commandQueue);
    cl_command_queue getQueueImpl() {
        return queue;
    }
private:
    cl_command_queue queue;
    bool ownsQueue;
};

}
#endif
