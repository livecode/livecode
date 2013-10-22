/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "graphics.h"
#include "graphics-internal.h"

#include <assert.h>

#include <SkMath.h>

static int dilateMask(const uint8_t *src, int src_y_stride, uint8_t *dst, int radius, int width, int height, bool transpose)
{
	int new_width = width + radius * 2;
    int dst_x_stride = transpose ? height : 1;
    int dst_y_stride = transpose ? 1 : new_width;
	for(int y = 0; y < height; ++y)
	{
		uint8_t *dptr;
		dptr = dst + y * dst_y_stride;
		const uint8_t *sptr;
		sptr = src + y * src_y_stride;
		for(int x = -radius; x < width + radius; x++)
		{
			uint8_t v;
			v = 0;
			for(int z = x - radius; z < x + radius; z++)
			{
				if (z < 0)
					continue;
				if (z >= width)
					continue;
				if (sptr[z] > v)
					v = sptr[z];
			}
			*dptr = v;
			dptr += dst_x_stride;
		}
	}
	return new_width;
}

// Dilate the input by using a distance transform. A pixel in the new mask
// is taken to be in the dilated mask if it is within the ellipse with radii
// xradius and yradius of a set pixel in the original mask. The maximum
// radii is 254 pixels.
void dilateDistanceXY(const uint8_t *src, uint8_t *dst, int xradius, int yradius, int width, int height, int& r_new_width, int& r_new_height)
{
	int new_width = width + 2 * xradius;
	int new_height = height + 2 * yradius;

	// Compute the x distance of each pixel from the nearest set pixel.
	uint8_t *xd;
	xd = new uint8_t[new_width * height];
	for(int y = 0; y < height; y++)
	{
		uint8_t *xdptr;
		xdptr = xd + new_width * y;
		
		const uint8_t *sptr;
		sptr = src + width * y;
		for(int x = 0; x < width; x++)
		{
			// If there is a value at x, take it
			if (sptr[x] != 0)
			{
				xdptr[x + xradius] = 0;
				continue;
			}
			
			// Otherwise search back...
			int db;
			db = 255;
			for(int z = x; z >= 0 && (x - z) < 255; z--)
			{
				if (sptr[z] != 0)
				{
					db = x - z;
					break;
				}
			}
			
			// Now search forwards...
			int df;
			df = 255;
			for(int z = x; (z < width) && (z - x) < 255; z++)
			{
				if (sptr[z] != 0)
				{
					df = z - x;
					break;
				}
			}
			
			// Distance is the minimum.
			int d;
			d = SkMin32(db, df);
			xdptr[x + xradius] = d;
		}
		
		// Now expand the fringes.
		for(int x = 0; x < xradius; x++)
		{
			if (xdptr[xradius] + (xradius - x) < 255)
				xdptr[x] = ((xradius - x) + xdptr[xradius]);
			else
				xdptr[x] = 255;

			if (xdptr[width + xradius - 1] + x + 1 < 255)
				xdptr[width + xradius + x] = xdptr[width + xradius - 1] + x + 1;
			else
				xdptr[width + xradius + x] = 255;
		}
	}
	
	unsigned int rf;
	rf = xradius * xradius * yradius * yradius;	
	
	memset(dst, 0, new_width * new_height);
	
	// Now use xd to compute the spread mask.
	for(int x = 0; x < new_width; x++)
	{
		for(int y = 0; y < new_height; y++)
		{
			uint8_t *dptr = dst + y * new_width + x;
			
			// If the distance at x, y is 0 then we are done.
			if (y >= yradius && y < (new_height - yradius) && xd[(y - yradius) * new_width + x] == 0)
			{
				*dptr = 255;
				continue;
			}
			
			// Otherwise, search up.
			*dptr = 0;
			for(int z = y; z >= 0; z--)
			{
				unsigned int yf;
				yf = (y - z) * xradius;
				yf *= yf;
				if (yf >= rf)
					break;
				
				unsigned int xf;
				if (z >= yradius && z < (new_height - yradius))
					xf = yradius * xd[(z - yradius) * new_width + x];
				else
					xf = yradius * 255;
				xf *= xf;
				if (xf < rf - yf)
				{
					*dptr = 255;
					break;
				}
			}
			
			if (*dptr == 255)
				continue;
			
			// Search downwards
			for(int z = y; z < new_height; z++)
			{
				unsigned int yf;
				yf = (z - y) * xradius;
				yf *= yf;
				if (yf >= rf)
					break;
				
				unsigned int xf;
				if (z >= yradius && z < (new_height - yradius))
					xf = yradius * xd[(z - yradius) * new_width + x];
				else
					xf = yradius * 255;
				xf *= xf;
				if (xf < rf - yf)
				{
					*dptr = 255;
					break;
				}
			}
		}
	}
	
	r_new_width = new_width;
	r_new_height = new_height;
}
