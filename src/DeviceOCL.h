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

#include <string>
#include "CL/cl.h"

#include "IArch.h"
#include "UtilOCL.h"

namespace ltk {

struct DeviceOCL {
	DeviceOCL(cl_context my_context, bool ownsCtxt, cl_device_id my_device,
			DeviceInfo *deviceInfo, IArch *architecture);
	~DeviceOCL();

	bool ownsContext;
	cl_context context;           // hold the context handler
	cl_device_id device;            // hold the selected device handler
	cl_command_queue commandQueue;      // hold the commands-queue handler
	DeviceInfo *deviceInfo;
	IArch *arch;
};

}
