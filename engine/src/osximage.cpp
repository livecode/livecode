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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "globals.h"
#include "image.h"

////////////////////////////////////////////////////////////////////////////////

extern void surface_extract_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
extern bool MCImageBitmapToCGImage(MCImageBitmap *p_bitmap, bool p_copy, bool p_invert, CGImageRef &r_image);

////////////////////////////////////////////////////////////////////////////////

static bool MCAlphaToCGImage(uindex_t p_width, uindex_t p_height, uint8_t* p_data, uindex_t p_stride, CGImageRef &r_image)
{
	bool t_success = true;
	
	CGImageRef t_image = nil;
	CGColorSpaceRef t_colorspace = nil;
	CFDataRef t_data = nil;
	CGDataProviderRef t_dp = nil;
	
	if (t_success)
		t_success = nil != (t_data = CFDataCreate(kCFAllocatorDefault, (uint8_t*)p_data, p_stride * p_height));
	
	if (t_success)
		t_success = nil != (t_dp = CGDataProviderCreateWithCFData(t_data));
	
	if (t_success)
		t_success = nil != (t_colorspace = CGColorSpaceCreateDeviceGray());
	
	if (t_success)
		t_success = nil != (t_image = CGImageCreate(p_width, p_height, 8, 8, p_stride, t_colorspace, kCGImageAlphaNone, t_dp, nil, false, kCGRenderingIntentDefault));
	
	CGColorSpaceRelease(t_colorspace);
	CGDataProviderRelease(t_dp);
	CFRelease(t_data);
	
	if (t_success)
		r_image = t_image;
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

CGImageRef MCImage::makeicon(uint4 p_width, uint4 p_height)
{
	CGImageRef t_icon = NULL;

	MCImageBitmap *t_bitmap = nil;
	
	if (lockbitmap(t_bitmap, false))
	{
		MCImageBitmap *t_scaled = nil;
		if (p_width != t_bitmap->width || p_height != t_bitmap->height)
			/* UNCHECKED */ MCImageScaleBitmap(t_bitmap, p_width, p_height, resizequality, t_scaled);
		/* UNCHECKED */ MCImageBitmapToCGImage(t_scaled != nil ? t_scaled : t_bitmap, true, false, t_icon);
		MCImageFreeBitmap(t_scaled);
		unlockbitmap(t_bitmap);
	}
	
	return t_icon;
}

bool MCImageBitmapToPICT(MCImageBitmap *p_bitmap, MCMacSysPictHandle &r_pict)
{
#ifdef LIBGRAPHICS_BROKEN
	bool t_success = true;
	
	Pixmap drawdata = nil, drawmask = nil;
	MCBitmap *maskimagealpha = nil;
	
	t_success = MCImageSplitPixmaps(p_bitmap, drawdata, drawmask, maskimagealpha);
	
	if (!t_success)
		return false;

	Rect t_rect;
	SetRect(&t_rect, 0, 0, p_bitmap->width, p_bitmap->height);
	
	GWorldPtr t_old_gworld;
	GDHandle t_old_gdevice;
	GetGWorld(&t_old_gworld, &t_old_gdevice);

	PixMapHandle t_draw_pixmap;
	t_draw_pixmap = GetGWorldPixMap((CGrafPtr)drawdata -> handle . pixmap);
	
	GWorldPtr t_img_gworld;
	t_img_gworld = NULL;
	if (t_success)
	{
		QDErr t_err;
		t_err = NewGWorld(&t_img_gworld, 32, &t_rect, NULL, NULL, 0);
		if (t_err != noErr)
			t_success = false;
	}
	
	if (t_success)
	{
		SetGWorld(t_img_gworld, GetGDevice());
		
		PenMode(srcCopy);
		ForeColor(blackColor);
		BackColor(whiteColor);
		
		if (maskimagealpha != NULL)
		{
			GWorldPtr t_alpha_gworld;
			if (NewGWorldFromPtr(&t_alpha_gworld, 8, &t_rect, GetCTable(40), NULL, 0, maskimagealpha -> data, maskimagealpha -> bytes_per_line) == noErr)
			{
				const BitMap *t_dst_bits;
				t_dst_bits = GetPortBitMapForCopyBits(t_img_gworld);
				
				const BitMap *t_src_bits;
				t_src_bits = GetPortBitMapForCopyBits((CGrafPtr)drawdata -> handle . pixmap);
				
				const BitMap *t_mask_bits;
				t_mask_bits = GetPortBitMapForCopyBits(t_alpha_gworld);
				
				EraseRect(&t_rect);
				
				CopyDeepMask(t_src_bits, t_mask_bits, t_dst_bits, &t_rect, &t_rect, &t_rect, srcCopy, NULL);
			}
		}
		else if (drawmask != NULL)
		{
			PixMapHandle t_mask_pixmap;
			t_mask_pixmap = GetGWorldPixMap((CGrafPtr)drawmask -> handle . pixmap);
			
			EraseRect(&t_rect);
			
			const BitMap *t_dst_bits;
			t_dst_bits = GetPortBitMapForCopyBits(t_img_gworld);
			
			const BitMap *t_src_bits;
			LockPixels(t_draw_pixmap);
			t_src_bits = (BitMap *)*t_draw_pixmap;
			
			const BitMap *t_mask_bits;
			LockPixels(t_mask_pixmap);
			t_mask_bits = (BitMap *)*t_mask_pixmap;
			
			CopyMask(t_src_bits, t_mask_bits, t_dst_bits, &t_rect, &t_rect, &t_rect);
			
			UnlockPixels(t_mask_pixmap);
			
			UnlockPixels(t_draw_pixmap);
		}
		else
		{
			const BitMap *t_dst_bits;
			t_dst_bits = GetPortBitMapForCopyBits(t_img_gworld);
			
			const BitMap *t_src_bits;
			LockPixels(t_draw_pixmap);
			t_src_bits = (BitMap *)*t_draw_pixmap;
			CopyBits(t_src_bits, t_dst_bits, &t_rect, &t_rect, srcCopy, NULL);
			UnlockPixels(t_draw_pixmap);
		}
	}
	
	PicHandle t_handle;
	t_handle = NULL;
	if (t_success)
	{
		OpenCPicParams t_params;
		t_params . srcRect = t_rect;
		t_params . hRes = 72 << 16;
		t_params . vRes = 72 << 16;
		t_params . version = -2;
		t_params . reserved1 = 0;
		t_params . reserved2 = 0;
		t_handle = OpenCPicture(&t_params);
		if (t_handle == NULL)
			t_success = false;
	}

	if (t_success)
	{
		GWorldPtr t_pict_gworld;
		GDHandle t_pict_gdevice;
		GetGWorld(&t_pict_gworld, &t_pict_gdevice);
		
		PenMode(srcCopy);
		ForeColor(blackColor);
		BackColor(whiteColor);
		
		const BitMap *t_dst_bits;
		t_dst_bits = GetPortBitMapForCopyBits(t_pict_gworld);

		const BitMap *t_src_bits;
		t_src_bits = GetPortBitMapForCopyBits(t_img_gworld);
		CopyBits(t_src_bits, t_dst_bits, &t_rect, &t_rect, srcCopy, NULL);
		
		ClosePicture();
	}
	
	if (t_img_gworld != NULL)
		DisposeGWorld(t_img_gworld);
	
	SetGWorld(t_old_gworld, t_old_gdevice);

	MCscreen->freepixmap(drawdata);
	MCscreen->freepixmap(drawmask);
	if (maskimagealpha != nil)
		MCscreen->destroyimage(maskimagealpha);
	
	if (t_success)
		r_pict = (MCMacSysPictHandle)t_handle;
	
	return t_success;
#else
	return false;
#endif
}

