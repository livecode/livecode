/* Copyright (C) 2003-2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"

#include "uidc.h"

#include "imagebitmap.h"

// Universal double comparison threshold
#define EPS 1e-8

#ifndef __VISUALC__
#define __forceinline inline
#endif

// ---------------
// Resizing 32-32
// ---------------
static void resize_seq_32_32(	const uint1* const src_data, const int src_n, const int src_step,
                              uint1*       const dst_data, const int dst_n, const int dst_step,
                              const uint1 low_bit,
                              real8* const aa, real8* const bb)
{
#define GETINT(offs) ( (*(const uint4*)(curs + (offs)) >> low_bit) & 0xFF )
#define GET(offs)    ( (real8) GETINT(offs) / 0xFF )
#define SETINT(tv)   ( \
	*(uint4*)curd |= (tv) << low_bit )
#define SET(tv)   ( SETINT((uint4)((tv) * 0xFF + 0.5)) )

	int i;

	// Current source and destination pointers
	const uint1* curs = src_data;
	uint1*       curd = dst_data;

	if(src_n == dst_n) // copy case
	{
		for(i = src_n ; i ; i--)
			SETINT(GETINT(0)), curs += src_step, curd += dst_step;

		return;
	}

	if(src_n == 1) // fill case
	{
		const real8 tv = GET(0);

		for(i = dst_n ; i ; i--)
			SET(tv), curd += dst_step;

		return;
	}

	if(dst_n == 1) // median case
	{
		real8 tv = 0.0;

		for(i = src_n ; i ; i--)
			tv += GET(0), curs += src_step;

		tv /= src_n;

		SET(tv);

		return;
	}

	if(src_n == 2) // linear case
	{
		const real8 v1 = (real8)GET(0);
		const real8 v2 = (real8)GET(src_step);

		const real8 k = (v2-v1)/(dst_n-1);

		for(i=0 ; i<dst_n ; i++)
			SET(v1 + k*i + 0.5), curd += dst_step;

		return;
	}

	// Evaluating derivatives gaussian coefs
	curs = src_data + (src_n - 1) * src_step;

	aa[src_n-1] = 2.0, bb[src_n-1] = 3.0*(GET(0) - GET(-src_step)), curs -= src_step;
	for(i=src_n-2 ; i>0 ; i--, curs -= src_step)
	{
		aa[i] = 4.0 - 1.0/aa[i+1]; bb[i] = 3.0*(GET(src_step) - GET(-src_step)) - bb[i+1]/aa[i+1];
	}
	aa[0] = 2.0 - 1.0/aa[1], bb[0] = 3.0*(GET(src_step) - GET(0)) - bb[1]/aa[1];

	real8 csx = 0.0; // current src x
	int    cdx = 0;   // current dst x

	real8 CD, ND = bb[0] / aa[0]; // initial derivative values

	const real8 k = (real8)(src_n-1) / dst_n;
	const real8 _1_k = 1.0 / k;

	real8 v1 = 0.0;          // current value,
  real8 v2 = 0.0;          // next value
	real8 a, b, c, d; // polynom coefs
  a = b = c = d = 0.0;
	real8 t = 0.0;          // polynom argument

	curs = src_data, curd = dst_data;

	if(src_n < dst_n) // enlarging
	{
		for(i=0 ; cdx < dst_n ; i++, curs += src_step)
		{
			if(i < src_n-1) // permission to move forwards
			{
				// Updating current and next derivative values
				CD = ND, ND = (bb[i+1] - CD) / aa[i+1];

				v1 = GET(0), v2 = GET(src_step);

				// Updating polynom coefs
				a = v1;
				b = CD;
				c = 3.0*(v2 - v1) - 2.0*CD - ND;
				d = 2.0*(v1 - v2) + CD + ND;
			}

			t = csx - i;

			// Filling output pixels with corresponding source segment interpolated values
			while(cdx < dst_n && (i == src_n-1 || t < 1.0 - EPS))
			{
				// Evaluating output color
				real8 tv = a+t*(b+t*(c+t*d));

				// Bounding output color
				if(tv < 0)
					tv = 0;
				else if(tv > 1.0)
					tv = 1.0;

				// Setting output color
				SET(tv), curd += dst_step;

				// Advancing to next destination pixel
				cdx++, t+=k;
			}

			csx = i + t;
		}
	}
	else // shrinking
	{
		// Locking onto first segment
		t = 0.0;

		i = 0;
		CD = ND, ND = (bb[i+1] - CD) / aa[i+1];
		v1 = GET(0), v2 = GET(src_step);
		a = v1;
		b = CD * 0.5;
		c = (v2 - v1) - (2.0*CD - ND) * 0.33333333333333;
		d = 0.5*(v1 - v2) + (CD + ND) * 0.25;

		for( ; cdx < dst_n ; cdx++)
		{
			real8 tv = 0.0; // total integral sum for this output pixel
			real8 cs = 0.0; // total source length run

			if(t > EPS) // past segment start
			{
				// Adding current segment left-over
				tv += a+b+c+d - t*(a+t*(b+t*(c+t*d))), cs += 1.0 - t, t = 0.0;

				// Moving to the next segment
				if(i<src_n-2)
				{
					i++, curs += src_step;
					CD = ND, ND = (bb[i+1] - CD) / aa[i+1];
					v1 = GET(0), v2 = GET(src_step);
					a = v1;
					b = CD * 0.5;
					c = (v2 - v1) - (2.0*CD - ND) * 0.3333333333;
					d = 0.5*(v1 - v2) + (CD + ND) * 0.25;
				}
			}

			// Adding completely covered segments
			while(cs + 1.0 < k + EPS)
			{
				tv += a+b+c+d, cs += 1.0;

				if(i<src_n-2)
				{
					i++, curs += src_step;
					CD = ND, ND = (bb[i+1] - CD) / aa[i+1];
					v1 = GET(0), v2 = GET(src_step);
					a = v1;
					b = CD * 0.5;
					c = (v2 - v1) - (2.0*CD - ND) * 0.3333333333;
					d = 0.5*(v1 - v2) + (CD + ND) * 0.25;
				}
			}

			// Adding first part of the last segment
			if(cs < k - EPS)
				t = k - cs, tv += t*(a+t*(b+t*(c+t*d)));

			// Normalizing output color
			tv *= _1_k;

			// Bounding output color
			if(tv < 0.0)
				tv = 0.0;
			else if(tv > 1.0)
				tv = 1.0;

			// Setting output color
			SET(tv), curd += dst_step;
		}
	}

#undef SET
#undef GET
}

#define ALPHA_OFFSET 24
#define RED_OFFSET 16
#define GREEN_OFFSET 8
#define BLUE_OFFSET 0

// Sizing

static void scaleimage_nearest(void *p_src_ptr, uint4 p_src_stride, void *p_dst_ptr, uint4 p_dst_stride, uint4 p_src_width, uint4 p_src_height, uint4 p_dst_width, uint4 p_dst_height)
{
	unsigned int t_ix, t_iy;
	t_ix = (65536 * p_src_width) / p_dst_width;
	t_iy = (65536 * p_src_height) / p_dst_height;

	unsigned int t_sy;
	t_sy = 0;

	unsigned int t_src_stride, t_dst_stride;
	t_src_stride = p_src_stride >> 2;
	t_dst_stride = p_dst_stride >> 2;

	unsigned int *t_src_pixel, *t_dst_pixel;
	t_src_pixel = (unsigned int *)p_src_ptr;
	t_dst_pixel = (unsigned int *)p_dst_ptr;

	unsigned int t_dy;
	t_dy = p_dst_height;
	while(t_dy-- > 0)
	{
		unsigned int *t_src_pixel_current;
		t_src_pixel_current = t_src_pixel + (t_sy >> 16) * t_src_stride;

		unsigned int *t_dst_pixel_current;
		t_dst_pixel_current = t_dst_pixel;

		unsigned int t_sx;
		t_sx = 0;

		unsigned int t_dx;
		t_dx = p_dst_width;

		while(t_dx-- > 0)
		{
			*t_dst_pixel_current++ = t_src_pixel_current[t_sx >> 16];
			t_sx += t_ix;
		}

		t_sy += t_iy;
		t_dst_pixel += t_dst_stride;
	}
}

static void scaleimage_box(void *p_src_ptr, uint4 p_src_stride, void *p_dst_ptr, uint4 p_dst_stride, uint4 p_src_width, uint4 p_src_height, uint4 p_dst_width, uint4 p_dst_height, bool p_has_alpha)
{
	scaleimage_nearest(p_src_ptr, p_src_stride, p_dst_ptr, p_dst_stride, p_src_width, p_src_height, p_dst_width, p_dst_height);
}

__forceinline unsigned int packed_bilinear_bounded_4(unsigned int x, unsigned char a, unsigned int y, unsigned char b, unsigned z, unsigned char c, unsigned int w, unsigned char d)
{
	unsigned int u, v;

	u = (x & 0xff00ff) * a + (y & 0xff00ff) * b + (z & 0xff00ff) * c + (w & 0xff00ff) * d + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b + ((z >> 8) & 0xff00ff) * c + ((w >> 8) & 0xff00ff) * d + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

__forceinline unsigned int packed_bilinear_combine_4(unsigned char x, unsigned char y, unsigned int f00, unsigned int f10, unsigned int f01, unsigned int f11)
{
	unsigned char x_y, ix_y, x_iy, ix_iy;

	unsigned int u;
	u = x * y + 0x80;
	u = (u + (u >> 8)) >> 8;

	x_y = u ;
	ix_y = y - x_y;
	x_iy = x - x_y;
	ix_iy = (255 - y) - x_iy;

	return packed_bilinear_bounded_4(f00, ix_iy, f10, x_iy, f01, ix_y, f11, x_y);
}

static void scaleimage_bilinear(void *p_src_ptr, uint4 p_src_stride, void *p_dst_ptr, uint4 p_dst_stride, uint4 p_src_width, uint4 p_src_height, uint4 p_dst_width, uint4 p_dst_height)
{
	unsigned int t_dst_stride;
	t_dst_stride = p_dst_stride / 4;
	unsigned int *t_dst_pixel;
	t_dst_pixel = (unsigned int *)p_dst_ptr;

	unsigned int t_src_stride;
	t_src_stride = p_src_stride / 4;
	unsigned int *t_src_pixel;
	t_src_pixel = (unsigned int *)p_src_ptr;

	unsigned int t_ax;
	t_ax = p_src_width * 256;
	int t_bx;
	t_bx = p_dst_width;
	unsigned int t_sx;
	t_sx = t_ax / t_bx;
	unsigned int t_tx;
	t_tx = t_ax % t_bx;

	unsigned int t_ay;
	t_ay = p_src_height * 256;
	int t_by;
	t_by = p_dst_height;
	unsigned int t_sy;
	t_sy = t_ay / t_by;
	unsigned int t_ty;
	t_ty = t_ay % t_by;

	unsigned int t_qy;
	t_qy = 0;
	
	int t_ry;
	t_ry = -t_by;

	unsigned int t_y;
	t_y = p_dst_height;
	while(t_y-- > 0)
	{
		unsigned int t_qx;
		t_qx = 0;
		
		int t_rx;
		t_rx = -t_bx;	
		
		unsigned int t_iy, t_fy;
		t_iy = t_qy / 256;
		t_fy = t_qy & 0xFF;

		unsigned int *t_src_pixel_0, *t_src_pixel_1;
		t_src_pixel_0 = t_src_pixel + t_iy * t_src_stride;
		t_src_pixel_1 = t_src_pixel_0;
		if (t_iy < p_src_height - 1)
			t_src_pixel_1 += t_src_stride;

		unsigned int t_x;
		t_x = p_dst_width;

		unsigned int *t_dst_pixel_current;
		t_dst_pixel_current = t_dst_pixel;

		while(t_x-- > 0)
		{
			unsigned int t_ix, t_fx;
			t_ix = t_qx / 256;
			t_fx = t_qx & 0xFF;

			unsigned int t_p00, t_p10, t_p01, t_p11;
			t_p00 = t_src_pixel_0[t_ix];
			t_p01 = t_src_pixel_1[t_ix];
			if (t_ix < p_src_width - 1)
			{
				t_p10 = t_src_pixel_0[t_ix + 1];
				t_p11 = t_src_pixel_1[t_ix + 1];
			}
			else
			{
				t_p10 = t_p00;
				t_p11 = t_p01;
			}

			*t_dst_pixel_current = packed_bilinear_combine_4(t_fx, t_fy, t_p00, t_p10, t_p01, t_p11);
			t_dst_pixel_current++;

			t_qx += t_sx;
			t_rx += t_tx;
			if (t_rx >= 0)
			{
				t_qx++;
				t_rx -= t_bx;
			}
		}

		t_dst_pixel += t_dst_stride;

		t_qy += t_sy;
		t_ry += t_ty;
		if (t_ry >= 0)
		{
			t_qy++;
			t_ry -= t_by;
		}
	}
}

static void scaleimage_bicubic(void *p_src_ptr, uint4 p_src_stride, void *p_dst_ptr, uint4 p_dst_stride, uint4 p_src_width, uint4 p_src_height, uint4 p_dst_width, uint4 p_dst_height)
{
	int i;

	// Temporary resizes rows data
	const int tmp_bytes_per_line = p_dst_width << 2;

	uint1* const tmp_data = new (nothrow) uint1[tmp_bytes_per_line * p_src_height];

	// Clearing temp and destination data
	memset(tmp_data, 0, tmp_bytes_per_line * p_src_height);
	memset(p_dst_ptr, 0, p_dst_stride * p_dst_height);

	// Temporary derivative arrays
	real8* aa;
	real8* bb;

	if(p_src_width > p_src_height)
		aa = new (nothrow) real8[p_src_width], bb = new (nothrow) real8[p_src_width];
	else
		aa = new (nothrow) real8[p_src_height], bb = new (nothrow) real8[p_src_height];

	// resizing rows
	for(i=0 ; i< (signed)p_src_height ; i++)
	{
		// R
		resize_seq_32_32(	(const uint1*)p_src_ptr + i * p_src_stride, p_src_width, 4,
		                  (uint1*)tmp_data + i * tmp_bytes_per_line, p_dst_width, 4,
		                  RED_OFFSET,
		                  aa, bb);

		// G
		resize_seq_32_32(	(const uint1*)p_src_ptr + i * p_src_stride, p_src_width, 4,
		                  (uint1*)tmp_data + i * tmp_bytes_per_line, p_dst_width, 4,
		                  GREEN_OFFSET,
		                  aa, bb);

		// B
		resize_seq_32_32(	(const uint1*)p_src_ptr + i * p_src_stride, p_src_width, 4,
		                  (uint1*)tmp_data + i * tmp_bytes_per_line, p_dst_width, 4,
		                  BLUE_OFFSET,
		                  aa, bb);

		// A
		resize_seq_32_32(	(const uint1*)p_src_ptr + i * p_src_stride, p_src_width, 4,
		                  (uint1*)tmp_data + i * tmp_bytes_per_line, p_dst_width, 4,
		                  ALPHA_OFFSET,
		                  aa, bb);
	}

	// resizing cols
	for(i=0 ; i< (signed)p_dst_width ; i++)
	{
		// R
		resize_seq_32_32(	(const uint1*)tmp_data + (i << 2), p_src_height, tmp_bytes_per_line,
		                  (uint1*)p_dst_ptr + (i << 2), p_dst_height, p_dst_stride,
		                  RED_OFFSET,
		                  aa, bb);

		// G
		resize_seq_32_32(	(const uint1*)tmp_data + (i << 2), p_src_height, tmp_bytes_per_line,
		                  (uint1*)p_dst_ptr + (i << 2), p_dst_height, p_dst_stride,
		                  GREEN_OFFSET,
		                  aa, bb);

		// B
		resize_seq_32_32(	(const uint1*)tmp_data + (i << 2), p_src_height, tmp_bytes_per_line,
		                  (uint1*)p_dst_ptr + (i << 2), p_dst_height, p_dst_stride,
		                  BLUE_OFFSET,
		                  aa, bb);

		// A
		resize_seq_32_32(	(const uint1*)tmp_data + (i << 2), p_src_height, tmp_bytes_per_line,
		                  (uint1*)p_dst_ptr + (i << 2), p_dst_height, p_dst_stride,
		                  ALPHA_OFFSET,
		                  aa, bb);
	}	

	delete[] bb;
	delete[] aa;
	delete[] tmp_data;
}

inline double maxd(double a, double b)
{
	if (a > b)
		return a;
	return b;
}

bool MCImageScaleBitmap(MCImageBitmap *p_src_bitmap, uindex_t p_width, uindex_t p_height, uint8_t p_quality, MCImageBitmap *&r_scaled)
{
	if (!MCImageBitmapCreate(p_width, p_height, r_scaled))
		return false;

	/* If the target bitmap has 0 pixels, then no scaling is required. */
	if (0 == p_width || 0 == p_height)
	{
		return true;
	}

	uindex_t owidth = p_src_bitmap->width;
	uindex_t oheight = p_src_bitmap->height;

	uint32_t *t_src_ptr = p_src_bitmap->data;
	uindex_t t_src_stride = p_src_bitmap->stride;
	uint32_t *t_dst_ptr = r_scaled->data;
	uindex_t t_dst_stride = r_scaled->stride;

	/* OVERHAUL - REVISIT - keep transparency flags of source image, unless BILINEAR or BICUBIC in which case assume
	 * scaled image has alpha if source has transparency */
	r_scaled->has_transparency = p_src_bitmap->has_transparency;
	r_scaled->has_alpha = p_src_bitmap->has_alpha;
	if (p_quality == INTERPOLATION_BILINEAR || p_quality == INTERPOLATION_BICUBIC)
		r_scaled->has_alpha = p_src_bitmap->has_transparency;
	
	// MW-2013-04-05: [[ Bug 10812 ]] Make sure we pass whether the image has transparency through to the box
	//   filter - otherwise nearest is used.
	if (p_quality == INTERPOLATION_NEAREST)
		scaleimage_nearest(t_src_ptr, t_src_stride, t_dst_ptr, t_dst_stride, owidth, oheight, p_width, p_height);
	else if (p_quality == INTERPOLATION_BOX)
		scaleimage_box(t_src_ptr, t_src_stride, t_dst_ptr, t_dst_stride, owidth, oheight, p_width, p_height, p_src_bitmap -> has_transparency);
	else if (p_quality == INTERPOLATION_BILINEAR)
		scaleimage_bilinear(t_src_ptr, t_src_stride, t_dst_ptr, t_dst_stride, owidth, oheight, p_width, p_height);
	else if (p_quality == INTERPOLATION_BICUBIC)
		scaleimage_bicubic(t_src_ptr, t_src_stride, t_dst_ptr, t_dst_stride, owidth, oheight, p_width, p_height);

	return true;
}

static inline void MCSwap(uint32_t &x_a, uint32_t &x_b)
{
	uint32_t t_tmp;
	t_tmp = x_a;
	x_a = x_b;
	x_b = t_tmp;
}

void MCImageFlipBitmapInPlaceVertical(MCImageBitmap *p_bitmap)
{
	if (p_bitmap == nil)
		return;

	uint8_t *t_src_ptr, *t_dst_ptr;
	t_src_ptr = t_dst_ptr = (uint8_t*)p_bitmap->data;
	t_dst_ptr += (p_bitmap->height - 1) * (p_bitmap->stride);

	for (uint32_t y = 0; y < p_bitmap->height / 2; y++)
	{
		uint32_t *t_src_pixel, *t_dst_pixel;
		t_src_pixel = (uint32_t*)t_src_ptr;
		t_dst_pixel = (uint32_t*)t_dst_ptr;

		for (uint32_t x = 0; x < p_bitmap->width; x++)
		{
			MCSwap(*t_dst_pixel++, *t_src_pixel++);
		}

		t_src_ptr += p_bitmap->stride;
		t_dst_ptr -= p_bitmap->stride;
	}
}

void MCImageFlipBitmapInPlaceHorizontal(MCImageBitmap *p_bitmap)
{
	if (p_bitmap == nil)
		return;

	uint8_t *t_src_ptr, *t_dst_ptr;
	t_src_ptr = t_dst_ptr = (uint8_t*)p_bitmap->data;
	t_dst_ptr += (p_bitmap->width - 1) * sizeof(uint32_t);

	for (uint32_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_src_pixel, *t_dst_pixel;
		t_src_pixel = (uint32_t*)t_src_ptr;
		t_dst_pixel = (uint32_t*)t_dst_ptr;

		for (uint32_t x = 0; x < p_bitmap->width / 2; x++)
			MCSwap(*t_dst_pixel--, *t_src_pixel++);

		t_src_ptr += p_bitmap->stride;
		t_dst_ptr += p_bitmap->stride;
	}
}

void MCImageFlipBitmapInPlaceHV(MCImageBitmap *p_bitmap)
{
	if (p_bitmap == nil)
		return;

	uint8_t *t_src_ptr, *t_dst_ptr;
	t_src_ptr = t_dst_ptr = (uint8_t*)p_bitmap->data;
	t_dst_ptr += (p_bitmap->height - 1) * p_bitmap->stride + (p_bitmap->width - 1) * sizeof(uint32_t);

	for (uint32_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_src_pixel, *t_dst_pixel;
		t_src_pixel = (uint32_t*)t_src_ptr;
		t_dst_pixel = (uint32_t*)t_dst_ptr;

		for (uint32_t x = 0; x < p_bitmap->width / 2; x++)
			MCSwap(*t_dst_pixel--, *t_src_pixel++);

		t_src_ptr += p_bitmap->stride;
		t_dst_ptr -= p_bitmap->stride;
	}
}

void MCImageFlipBitmapInPlace(MCImageBitmap *p_bitmap, bool p_horizontal, bool p_vertical)
{
	if (p_bitmap == nil)
		return;

	if (p_horizontal && p_vertical)
		MCImageFlipBitmapInPlaceHV(p_bitmap);
	else if (p_horizontal)
		MCImageFlipBitmapInPlaceHorizontal(p_bitmap);
	else if (p_vertical)
		MCImageFlipBitmapInPlaceVertical(p_bitmap);
}
