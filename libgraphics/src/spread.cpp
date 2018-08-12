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

#include "graphics.h"
#include "graphics-internal.h"

#include <assert.h>

#include <SkMath.h>

// Dilate the input by using a distance transform. A pixel in the new mask
// is taken to be in the dilated mask if it is within the ellipse with radii
// xradius and yradius of a set pixel in the original mask. The maximum
// radii is 254 pixels.
void dilateDistanceXY(const uint8_t *src, uint8_t *dst, int xradius, int yradius, int width, int height, int& r_new_width, int& r_new_height)
{
/*
    int new_width = width + 2 * xradius;
	int new_height = height + 2 * yradius;
    
	// Compute the x distance of each pixel from the nearest set pixel.
	uint8_t *xd;
	xd = new (nothrow) uint8_t[new_width * height];
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
*/
    int new_width = width + 2 * xradius;
	int new_height = height + 2 * yradius;

    // Allocate memory for the loop
    size_t t_mem_size = new_width * new_height;
	uint8_t *buffer;
	/* UNCHECKED */ buffer = (uint8_t*)malloc(t_mem_size);

    // All the destination pixels are set to the infinite distance initially
    memset(buffer, 255, t_mem_size);
    
    // Only this part of the buffer can have non-infinite x distances
    uint8_t *xd = buffer + new_width * yradius;
    
	// Compute the x distance of each pixel from the nearest set pixel
	for(int y = 0; y < height; y++)
	{
		uint8_t *xdptr;
		xdptr = xd + new_width * y;
		
		const uint8_t *sptr;
		sptr = src + width * y;
        
        int x = 0;
        int next = 0;
        while (x < new_width)
        {
            // Scan along the line for the next pixel that is set in the source mask
            while (next < width && sptr[next] == 0)
                next++;
            
            // If no more set pixels in the source mask, go to the end of the line
            int dnext;
            if (next >= width)
                dnext = new_width;
            else
                dnext = next + xradius;    // Account for position shift between src and dst
            
            // If we're starting at the left edge, there isn't a mask point there
            int distance = dnext - x;
            int halfway = distance / 2;
            if (x == 0)
                halfway = 0;
            
            // If we're ending at the right edge, there isn't a mask point there
            if (dnext >= new_width)
                halfway = distance;
            
            // Ensure that we do nothing for empty lines
            if (next >= width && x == 0)
                halfway = distance = 0;
            
            int i;
            for (i = 0; i < halfway; i++)
                xdptr[x + i] = SkMin32(i, 255);
            for (; i < distance; i++)
                xdptr[x + i] = SkMin32(distance - i, 255);
            
            // Move on to the next pixel
            //
            // Optimisation: scan for the next clear pixel in the mask
            //      Avoids excessive processing for rows of non-zero pixels
            x = dnext;
            next = next + 1;
            
            // MW-2014-06-24: [[ Bug ]] Only loop up to (width - 1), otherwise we get
            //   an access overflow.
            while (next < (width - 1) && sptr[next + 1] != 0)
            {
                xdptr[x] = 0;
                x++, next++;
            }
            
            // MW-2014-08-27: [[ Bug 13221 ]] If we reached the edge of the source, then we
            //   assume the next pixel is clear.
            if (next == width - 1)
            {
                xdptr[x] = 0;
                x++, next++;
            }
        }
	}
    
    // Destination pointer stride
    int ydstride = new_width;
	
    // a-squared, b-squared and a-squared*b-squared
	uint32_t aa, bb, aabb;
    aa = xradius * xradius;
    bb = yradius * yradius;
    aabb = aa*bb;
    
    // Look-up tables for pre-calculated multiplications
    uint32_t t_xlut[256], t_ylut[256];
    for (int i = 0; i < 256; i++)
        t_xlut[i] = bb*i*i, t_ylut[i] = aa*i*i;
    
    // Ensure that the "infinite" distance never compares < anything else
    t_xlut[255] = t_ylut[255] = 0xFFFFFFFF;
	
	memset(dst, 0, new_width * new_height);
    
    // X distance array
    const uint8_t *xdist = buffer;
	
    // Be careful:
    //
    //  Minor, seemingly inconsequential, changes to this loop can slow it down
    //  considerably. On the other hand, they could speed it up...
    int offset = 0;
    for (int y = 0; y < new_height; y++)
    {
        for (int x = 0; x < new_width; x++, offset++)
        {
            // Starting at this position, scan down for a pixel that is within
            // the dilation radius.
            int ny = y;
            int noffset = offset;
            int max = SkMin32(new_height, y + yradius);
            do
            {
                uint8_t xval, yval;
                xval = xdist[noffset];
                yval = ny - y;
                
                // Note that care is needed to avoid overflow errors
                uint64_t t_dist64 = uint64_t(t_xlut[xval]) + uint64_t(t_ylut[yval]);
                if (t_dist64 == uint32_t(t_dist64) && uint32_t(t_dist64) < aabb)
                {
                    dst[offset] = 255;
                    break;
                }
                
                ny += 1;
                noffset += ydstride;
            }
            while (ny < max);
            
            // Early escape to avoid processing another loop
            if (dst[offset])
                continue;
            
            // Starting at this position, scan up for a pixel that is within
            // the dilation radius
            ny = y - 1;
            noffset = offset - ydstride;
            int min = SkMax32(0, y - yradius);
            while (ny >= min)
            {
                uint8_t xval, yval;
                xval = xdist[noffset];
                yval = y - ny;
                
                // Note that care is needed to avoid overflow errors
                uint64_t t_dist64 = uint64_t(t_xlut[xval]) + uint64_t(t_ylut[yval]);
                if (t_dist64 == uint32_t(t_dist64) && uint32_t(t_dist64) < aabb)
                {
                    dst[offset] = 255;
                    break;
                }
                
                ny -= 1;
                noffset -= ydstride;
            }
        }
    }
	
    free(buffer);
    
	r_new_width = new_width;
	r_new_height = new_height;
}
