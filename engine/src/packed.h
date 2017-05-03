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

#ifndef __MC_PACKED__
#define __MC_PACKED__

#ifdef __VISUALC__
#define PACKED_INLINE __forceinline
#else
#define PACKED_INLINE inline
#endif

// r_i = (x_i * a) / 255
PACKED_INLINE uint4 packed_scale_bounded(uint4 x, uint1 a)
{
	uint4 u, v;

	u = ((x & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

// r_i = (x_i * a + y_i * b) / 255
PACKED_INLINE uint4 packed_bilinear_bounded(uint4 x, uint1 a, uint4 y, uint1 b)
{
	uint4 u, v;

	u = (x & 0xff00ff) * a + (y & 0xff00ff) * b + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

PACKED_INLINE uint32_t _combine(uint32_t u, uint32_t v)
{
	u += 0x800080;
	v += 0x800080;
	return (((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff) + (((v + ((v >> 8) & 0xff00ff))) & 0xff00ff00);
}

PACKED_INLINE uint32_t _multiply_low(uint32_t x, uint32_t y)
{
	return ((x & 0xff) * (y & 0xff)) | ((x & 0xff0000) * ((y >> 16) & 0xff));
}

PACKED_INLINE uint32_t _multiply_high(uint32_t x, uint32_t y)
{
	x = x >> 8;
	return ((x & 0xff) * ((y >> 8) & 0xff)) | ((x & 0xff0000) * (y >> 24));
}

// r_i = x_i * y_i / 255;
PACKED_INLINE uint32_t packed_multiply_bounded(uint32_t x, uint32_t y)
{
	return _combine(_multiply_low(x, y), _multiply_high(x, y));
}

// r_i = (x_i + y_i) / 2
PACKED_INLINE uint4 packed_avg(uint4 x, uint4 y)
{
	uint4 u, v;
	u = (((x & 0xff00ff) + (y & 0xff00ff)) >> 1) & 0xff00ff;
	v = ((((x >> 8) & 0xff00ff) + ((y >> 8) & 0xff00ff)) << 7) & 0xff00ff00;

	return u | v;
}

#endif
