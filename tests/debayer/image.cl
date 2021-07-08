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
/*
 * Copyright 2015 Jason Newton <nevion@gmail.com>
 *
 *Permission is hereby granted, free of charge, to any person obtaining a copy
 *of this software and associated documentation files (the "Software"), to deal
 *in the Software without restriction, including without limitation the rights
 *to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *copies of the Software, and to permit persons to whom the Software is
 *furnished to do so, subject to the following conditions:
 *
 *The above copyright notice and this permission notice shall be included in all
 *copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *SOFTWARE.
 */

#ifndef CLCOMMONS_IMAGE_H
#define CLCOMMONS_IMAGE_H

#include "common.cl"

enum {
	ADDRESS_CLAMP = 0, //repeat border
	ADDRESS_ZERO = 1, //returns 0
	ADDRESS_REFLECT_BORDER_EXCLUSIVE = 2, //reflects at boundary and will not duplicate boundary elements
	ADDRESS_REFLECT_BORDER_INCLUSIVE = 3, //reflects at boundary and will duplicate boundary elements,
	ADDRESS_NOOP = 4 //programmer guarantees no reflection necessary
};

//coordinate is c, r for compatibility with climage and CUDA
INLINE uint2 tex2D(const int rows, const int cols, const int _c, const int _r,
		const uint sample_method) {
	int c = _c;
	int r = _r;
	if (sample_method == ADDRESS_REFLECT_BORDER_EXCLUSIVE) {
		c = c < 0 ? -c : c;
		c = c >= cols ? cols - (c - cols) - 2 : c;
		r = r < 0 ? -r : r;
		r = r >= rows ? rows - (r - rows) - 2 : r;
	} else if (sample_method == ADDRESS_CLAMP) {
		c = c < 0 ? 0 : c;
		c = c > cols - 1 ? cols - 1 : c;
		r = r < 0 ? 0 : r;
		r = r > rows - 1 ? rows - 1 : r;
	} else if (sample_method == ADDRESS_REFLECT_BORDER_INCLUSIVE) {
		c = c < 0 ? -c - 1 : c;
		c = c >= cols ? cols - (c - cols) - 1 : c;
		r = r < 0 ? -r - 1 : r;
		r = r >= rows ? rows - (r - rows) - 1 : r;
	} else if (sample_method == ADDRESS_ZERO) {
	} else if (sample_method == ADDRESS_NOOP) {
	} else {
		assert(false);
	}
	assert_val(r >= 0 && r < rows, r);
	assert_val(c >= 0 && c < cols, c);
	return (uint2)(r, c);
}

INLINE __global uchar* image_line_at_(__global uchar *im_p, const uint im_rows, const uint im_cols, const uint image_pitch_p, const uint r) {
	assert_val(r >= 0 && r < im_rows, r);
	(void) im_cols;
	return im_p + r * image_pitch_p;
}
#define image_line_at(PixelT, im_p, im_rows, im_cols, image_pitch, r) ((__global PixelT *) image_line_at_((__global uchar *) (im_p), (im_rows), (im_cols), (image_pitch), (r)))

INLINE __global uchar* image_pixel_at_(__global uchar *im_p, const uint im_rows, const uint im_cols, const uint image_pitch_p, const uint r, const uint c, const uint sizeof_pixel) {
	assert_val(r >= 0 && r < im_rows, r);
	assert_val(c >= 0 && c < im_cols, c);
	return im_p + (r * image_pitch_p) + c * sizeof_pixel;
}
#define image_pixel_at(PixelT, im_p, im_rows, im_cols, image_pitch, r, c) (*((__global PixelT *) image_pixel_at_((__global uchar *)(im_p), (im_rows), (im_cols), (image_pitch), (r), (c), sizeof(PixelT))))

INLINE __global uchar* image_tex2D_(__global uchar *im_p, const uint im_rows, const uint im_cols, const uint image_pitch, const int r, const int c, const uint sizeof_pixel, const uint sample_method) {
	const uint2 p2 = tex2D((int) im_rows, (int) im_cols, c, r, sample_method);
	return image_pixel_at_(im_p, im_rows, im_cols, image_pitch, p2.s0, p2.s1, sizeof_pixel);
}
#define image_tex2D(PixelT, im_p, im_rows, im_cols, image_pitch, r, c, sample_method) \
  (((sample_method) == ADDRESS_ZERO) & (((r) < 0) | ((r) >= (im_rows)) | ((c) < 0) | ((c) >= (im_cols))) ? 0 : \
  *(__global PixelT *) image_tex2D_((__global uchar *)(im_p), (im_rows), (im_cols), (image_pitch), (r), (c), sizeof(PixelT), (sample_method)))

#endif
