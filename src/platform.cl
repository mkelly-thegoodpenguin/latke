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

#if defined(OPENCL_FOUND)

#define CONSTANT constant
#define GLOBAL global
#define LOCAL local
#define KERNEL kernel

#define REQUIRED_WORK_GROUP_SIZE(dimX,dimY) __attribute__((reqd_work_group_size(dimX,dimY, 1)))

#define READ_ONLY_IMAGE2D read_only image2d_t
#define WRITE_ONLY_IMAGE2D write_only image2d_t

static inline size_t getGlobalIdX(void) {
	return get_global_id(0);
}
static inline size_t getGlobalIdY(void) {
	return get_global_id(1);
}

static inline size_t getGroupIdX(void) {
	return get_group_id(0);
}
static inline size_t getGroupIdY(void) {
	return get_group_id(1);
}

static inline size_t getLocalIdX(void) {
	return get_local_id(0);
}
static inline size_t getLocalIdY(void) {
	return get_local_id(1);
}

static inline void barrierWithLocalFence() {
	barrier(CLK_LOCAL_MEM_FENCE);
}

#endif