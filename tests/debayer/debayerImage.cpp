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
#include "debayer.cpp"

Debayer<DualImageOCL, ImageAllocater> debayer;

void CL_CALLBACK HostToDeviceMappedCallback(cl_event event,
		cl_int cmd_exec_status, void *user_data) {

	assert(user_data);

	auto info = (JobInfo<DualImageOCL>*) user_data;

	// push mapped image into queue
	debayer.mappedHostToDeviceQueue.push(info);
}

void CL_CALLBACK DeviceToHostMappedCallback(cl_event event,
		cl_int cmd_exec_status, void *user_data) {

	assert(user_data);
	// handle processed data
	auto info = (JobInfo<DualImageOCL>*) user_data;

	// push mapped image into queue
	debayer.mappedDeviceToHostQueue.push(info);
}

int main(int argc, char *argv[]) {
	return debayer.debayer(argc, argv,
			(pfn_event_notify) HostToDeviceMappedCallback,
			(pfn_event_notify) DeviceToHostMappedCallback, "debayerImage.cl");
}

