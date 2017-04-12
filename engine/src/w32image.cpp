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
#include "region.h"
#include "image.h"

#include "w32dc.h"

void surface_extract_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);

MCWindowShape *MCImage::makewindowshape(const MCGIntegerSize &p_size)
{
	bool t_success;
	t_success = true;

	MCWindowShape *t_mask;
	if (!MCMemoryNew(t_mask))
		return nil;
	
	MCImageBitmap *t_bitmap = nil;
	bool t_has_mask = false, t_has_alpha = false;
	
	t_success = lockbitmap(true, true, &p_size, t_bitmap);

	if (t_success)
	{
		// Get the width / height.
		t_mask -> width = t_bitmap -> width;
		t_mask -> height = t_bitmap -> height;
		
		t_has_mask = MCImageBitmapHasTransparency(t_bitmap, t_has_alpha);
		if (t_has_alpha)
		{
			// The mask is not sharp.
			t_mask -> is_sharp = false;
			
			// Set the stride;
			t_mask -> stride = (t_mask -> width + 3) & ~3;
			
			// Allocate.
			t_success = nil != (t_mask -> data = (char *)malloc(t_mask -> stride * t_mask -> height));

			if (t_success)
				surface_extract_alpha(t_bitmap->data, t_bitmap->stride, t_mask->data, t_mask->stride, t_mask->width, t_mask->height);
		}
		else if (t_has_mask)
		{
			// The mask is sharp.
			t_mask -> is_sharp = true;

			// Stride and data are zero.
			t_mask -> stride = 0;
			t_mask -> data = nil;

			// Handle is region.
			t_success = nil != (t_mask -> handle = ((MCScreenDC *)MCscreen) -> BitmapToRegion(t_bitmap));
		}
		else
			t_success = false;
	}
	unlockbitmap(t_bitmap);

	if (t_success)
		return t_mask;

	MCMemoryDelete(t_mask);

	return nil;
}

///////////////////////////////////////////////////////////////////////////////

static inline uint32_t packed_bilinear_bounded(uint32_t x, uint8_t a, uint32_t y, uint8_t b)
{
	uint32_t u, v;

	u = (x & 0xff00ff) * a + (y & 0xff00ff) * b + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

// It seems that the only DIB format that is guaranteed to be accepted by apps such as
// WordPad and Paint is 3-byte per pixel RGB...
//
bool MCImageBitmapToDIB(MCImageBitmap *p_bitmap, MCWinSysHandle &r_dib)
{
	HGLOBAL t_handle = nil;

	// 3 bytes per pixel aligned to 4 bytes
	uindex_t t_stride = ((p_bitmap->width * 3) + 3) & ~3;

	t_handle = GlobalAlloc(GMEM_FIXED, sizeof(BITMAPINFOHEADER) + p_bitmap->height * t_stride);

	if (t_handle == nil)
		return false;

	BITMAPINFOHEADER *t_header = (BITMAPINFOHEADER*)t_handle;
	MCMemoryClear(t_header, sizeof(BITMAPINFOHEADER));
	t_header -> biSize = sizeof(BITMAPINFOHEADER);
	t_header -> biWidth = p_bitmap->width;
	t_header -> biHeight = p_bitmap->height;
	t_header -> biPlanes = 1;
	t_header -> biBitCount = 24;
	t_header -> biCompression = BI_RGB;
	t_header -> biSizeImage = 0;
	t_header -> biXPelsPerMeter = 0;
	t_header -> biYPelsPerMeter = 0;
	t_header -> biClrUsed = 0;
	t_header -> biClrImportant = 0;

	uint8_t *t_dst_ptr = (uint8_t*)t_handle + sizeof(BITMAPINFOHEADER);
	uint8_t *t_src_ptr = (uint8_t*)p_bitmap->data + (p_bitmap->height - 1) * p_bitmap->stride;

	// blend pixels against black background
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		for(uindex_t x = 0; x < p_bitmap->width; ++x)
		{
			uint4 t_pixel;
			uint8_t t_alpha = ((uint4 *)t_src_ptr)[x] >> 24;
			t_pixel = packed_bilinear_bounded(((uint4 *)t_src_ptr)[x], t_alpha, 0xFFFFFF, 255 - t_alpha);
			memcpy(t_dst_ptr + x * 3, &t_pixel, 3);
		}
		t_dst_ptr += t_stride;
		t_src_ptr -= p_bitmap->stride;
	}

	r_dib = (MCWinSysHandle)t_handle;

	return true;
}

bool MCImageBitmapToV5DIB(MCImageBitmap *p_bitmap, MCWinSysHandle &r_dib)
{
	HGLOBAL t_handle = nil;

	uindex_t t_stride = p_bitmap->width * 4;

	t_handle = GlobalAlloc(GMEM_FIXED, sizeof(BITMAPV5HEADER) + p_bitmap->height * t_stride);

	if (t_handle == nil)
		return false;

	BITMAPV5HEADER *t_header = (BITMAPV5HEADER*)t_handle;
	MCMemoryClear(t_header, sizeof(BITMAPV5HEADER));
	t_header -> bV5Size = sizeof(BITMAPV5HEADER);
	t_header -> bV5Width = p_bitmap->width;
	t_header -> bV5Height = p_bitmap->height;
	t_header -> bV5Planes = 1;
	t_header -> bV5BitCount = 32;
	t_header -> bV5Compression = BI_RGB; // BI_BITFIELDS;
	t_header -> bV5SizeImage = 0; // t_stride * p_bitmap->height;
	t_header -> bV5XPelsPerMeter = 0;
	t_header -> bV5YPelsPerMeter = 0;
	t_header -> bV5ClrUsed = 0;
	t_header -> bV5ClrImportant = 0;
	t_header -> bV5AlphaMask = 0xFF000000;
	t_header -> bV5RedMask =   0x00FF0000;
	t_header -> bV5GreenMask = 0x0000FF00;
	t_header -> bV5BlueMask =  0x000000FF;
	t_header -> bV5CSType = LCS_WINDOWS_COLOR_SPACE;

	uint8_t *t_dst_ptr = (uint8_t*)t_handle + sizeof(BITMAPV5HEADER);
	uint8_t *t_src_ptr = (uint8_t*)p_bitmap->data + (p_bitmap->height - 1) * p_bitmap->stride;

	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		MCMemoryCopy(t_dst_ptr, t_src_ptr, t_stride);
		t_dst_ptr += t_stride;
		t_src_ptr -= p_bitmap->stride;
	}

	r_dib = (MCWinSysHandle)t_handle;

	return true;
}

extern bool create_temporary_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits);
bool MCImageBitmapToEnhancedMetafile(MCImageBitmap *p_bitmap, MCWinSysEnhMetafileHandle &r_enh_metafile)
{
	bool t_success = true;

	HDC t_src_dc;
	HDC t_dst_dc;

	t_success = nil != (t_src_dc = CreateCompatibleDC(t_dst_dc));
	if (t_success)
		t_success = nil != (t_dst_dc = CreateEnhMetaFile(NULL, NULL, NULL, NULL));

	HBITMAP t_hbitmap = nil;
	void *t_bits = nil;

	if (t_success)
		t_success = create_temporary_dib(t_src_dc, p_bitmap->width, p_bitmap->height, t_hbitmap, t_bits);
	if (t_success)
		MCMemoryCopy(t_bits, p_bitmap->data, p_bitmap->stride * p_bitmap->height);

	if (t_success)
	{
		SetWindowOrgEx(t_dst_dc, 0, 0, NULL);
		SetWindowExtEx(t_dst_dc, p_bitmap->width , p_bitmap->height, NULL);
		SaveDC(t_dst_dc);

		BLENDFUNCTION t_blend;
		t_blend.BlendOp = AC_SRC_OVER;
		t_blend.BlendFlags = 0;
		t_blend.SourceConstantAlpha = 255;
		t_blend.AlphaFormat = AC_SRC_ALPHA;

		HGDIOBJ t_old_obj;
		t_old_obj = SelectObject(t_src_dc, t_hbitmap);
		t_success = AlphaBlend(t_dst_dc, 0, 0, p_bitmap->width, p_bitmap->height, t_src_dc, 0, 0, p_bitmap->width, p_bitmap->height, t_blend);
		if (!t_success)
			t_success = StretchBlt(t_dst_dc, 0, 0, p_bitmap->width, p_bitmap->height, t_src_dc, 0, 0, p_bitmap->width, p_bitmap->height, SRCCOPY);
		SelectObject(t_src_dc, t_old_obj);

		RestoreDC(t_dst_dc, -1);
	}

	HENHMETAFILE t_emetafile = nil;
	if (t_dst_dc != NULL)
		t_emetafile = CloseEnhMetaFile(t_dst_dc);

	if (t_success)
		t_success = t_emetafile != nil;

	if (t_hbitmap != nil)
		DeleteObject(t_hbitmap);

	if (t_src_dc != nil)
		DeleteDC(t_src_dc);

	if (t_success)
		r_enh_metafile = (MCWinSysEnhMetafileHandle)t_emetafile;
	else
	{
		if (t_emetafile != nil)
			DeleteEnhMetaFile(t_emetafile);
	}

	return t_success;
}

// Note that this is pretty much the best we can do for MetaFile
// conversion. Most apps don't seem to like it if you use the AlphaBlend
// GDI call - and this is not supported on NT4.0 anyway.
//
// In a metafile that would display from WordPad the following 
//   SetWindowOrg(0, 0)
//   SetWindowExt(0x085f, 0x085f)
//   SetBkMode(TRANSPARENT)
//   SetROP(R2_COPY_PEN)
//   CreatePenIndirect(0, 0, 0, 0, 0)
//   SelectObject(0)
//   CreateBrushIndirect(0, WHITE)
//   SelectObject(1)
//   CreateFontIndirect(...)
//   SelectObject(2)
//   SetTextAlign(0x18)
//   SetTextColor(WHITE)
//   SetStretchBltMode(3)
//   SaveDC
//   SetROP2(R2_NOT)
//   CreatePenIndirect(5, 0, WHITE)
//   SelectObject(3)
//   DeleteObject(0)
//   Rectangle(0, 0, 0x08d2, 0x08d2)
//   StretchBlt
//     8600ee000000
//     4000
//     4000
//     0000
//     0000
//     d208
//     d208
//     0000
//     0000
//   RestoreDC
//

extern bool MCImageBitmapSplitHBITMAPWithMask(HDC p_dc, MCImageBitmap *p_bitmap, HBITMAP &r_bitmap, HBITMAP &r_mask);
bool MCImageBitmapToMetafile(MCImageBitmap *p_bitmap, MCWinSysMetafileHandle &r_metafile)
{
	bool t_success = true;

	HDC t_src_dc;
	t_src_dc = ((MCScreenDC *)MCscreen) -> getsrchdc();

	HDC t_dst_dc;
	t_success = nil != (t_dst_dc = CreateMetaFileA(NULL));

	HBITMAP t_hbitmap = nil;
	HBITMAP t_hmask = nil;

	if (t_success)
		t_success = MCImageBitmapSplitHBITMAPWithMask(t_src_dc, p_bitmap, t_hbitmap, t_hmask);

	if (t_success)
	{
		uint4 t_mf_width;
		t_mf_width = p_bitmap->width;

		uint4 t_mf_height;
		t_mf_height = p_bitmap->height;

		SetWindowOrgEx(t_dst_dc, 0, 0, NULL);
		SetWindowExtEx(t_dst_dc, t_mf_width , t_mf_height, NULL);
		SetBkMode(t_dst_dc, TRANSPARENT);
		SetStretchBltMode(t_dst_dc, COLORONCOLOR);
		SaveDC(t_dst_dc);

		if (t_hmask != NULL)
		{
			HGDIOBJ t_old_object;

			// First ensure all affected pixels are black in the destination
			t_old_object = SelectObject(t_src_dc, t_hmask);
			StretchBlt(t_dst_dc, 0, 0, t_mf_width, t_mf_height, t_src_dc, 0, 0, p_bitmap->width, p_bitmap->height, SRCAND);

			// Now OR in the source color pixels
			SelectObject(t_src_dc, t_hbitmap);
			StretchBlt(t_dst_dc, 0, 0, t_mf_width, t_mf_height, t_src_dc, 0, 0, p_bitmap->width, p_bitmap->height, SRCPAINT);
			
			SelectObject(t_src_dc, t_old_object);
		}
		else
		{
			HGDIOBJ t_old_object;
			t_old_object = SelectObject(t_src_dc, t_hbitmap);

			StretchBlt(t_dst_dc, 0, 0, p_bitmap->width, p_bitmap->height, t_src_dc, 0, 0, p_bitmap->width, p_bitmap->height, SRCCOPY);

			SelectObject(t_src_dc, t_old_object);
		}

		RestoreDC(t_dst_dc, -1);
	}

	HMETAFILE t_metafile = nil;
	if (t_dst_dc != nil)
		t_metafile = CloseMetaFile(t_dst_dc);

	if (t_success)
		t_success = t_metafile != nil;

	HMETAFILEPICT t_pict = nil;
	if (t_success)
	{
		t_pict = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof(METAFILEPICT));
		t_success = t_pict != nil;
	}

	if (t_success)
	{
		METAFILEPICT *t_pict_ptr;
		t_pict_ptr = (METAFILEPICT *)GlobalLock(t_pict);
		t_pict_ptr -> hMF = t_metafile;
		t_pict_ptr -> xExt = p_bitmap->width * 2540 / 96;
		t_pict_ptr -> yExt = p_bitmap->height * 2540 / 96;
		t_pict_ptr -> mm = MM_ANISOTROPIC;
		GlobalUnlock(t_pict);
	}

	if (t_hbitmap != nil)
		DeleteObject(t_hbitmap);
	if (t_hmask != nil)
		DeleteObject(t_hmask);

	if (t_success)
		r_metafile = (MCWinSysMetafileHandle)t_pict;
	else
	{
		if (t_metafile != nil)
			DeleteMetaFile(t_metafile);
	}

	return t_success;
}

bool MCImageBitmapToDragImage(MCImageBitmap *p_bitmap, MCDataRef &r_dragimage)
{
	uint32_t t_header_size;
	if (MCmajorosversion <= 0x0500)
		t_header_size = sizeof(BITMAPINFOHEADER);
	else
		t_header_size = sizeof(BITMAPV4HEADER);

	uint32_t t_image_size;
	t_image_size = p_bitmap->stride * p_bitmap->height;

	uint32_t t_data_size;
	t_data_size = t_header_size + t_image_size;

	char *t_data = nil;
	MCAutoByteArray t_buffer;
	if (!t_buffer.New(t_data_size))
		return false;

	t_data = (char*)t_buffer.Bytes();

	// Location of the bits in the DIB data we are producing
	char *t_out_bits;
	t_out_bits = t_data + t_header_size;

	// First we copy the bits of the image into our DIB
	MCMemoryCopy(t_out_bits, p_bitmap->data, t_image_size);

	// For pre-Vista versions, we composite each pixel against white then set
	// all transparent pixels to a color key value. This value at the moment
	// is a fixed, arbitrarily chosen value.
	if (MCmajorosversion <= 0x0500)
	{
		char *t_ptr;
		t_ptr = t_out_bits;
		for(uint4 y = 0; y < p_bitmap->height; ++y, t_ptr += p_bitmap->stride)
			for(uint4 x = 0; x < p_bitmap->width; ++x)
			{
				uint4 t_pixel;
				t_pixel = ((uint4 *)t_ptr)[x];
				
				uint1 t_alpha;
				t_alpha = t_pixel >> 24;

				if (t_alpha == 0)
					t_pixel = 0x010101;
				else
				{
					t_pixel = 0xFFFFFF & (t_pixel + (0x010101 * (255 - t_alpha)));
					//t_pixel = packed_bilinear_bounded(t_pixel & 0xFFFFFF, t_alpha, 0xFFFFFF, 255 - t_alpha);
					if (t_pixel == 0x010101)
						t_pixel = 0x020202;
				}

				((uint4 *)t_ptr)[x] = t_pixel;
			}

		BITMAPINFOHEADER *t_header;
		t_header = (BITMAPINFOHEADER *)t_data;
		memset(t_header, 0, sizeof(BITMAPINFOHEADER));
		t_header -> biSize = sizeof(BITMAPINFOHEADER);
		t_header -> biWidth = p_bitmap->width;
		t_header -> biHeight = -(int32_t)p_bitmap->height;
		t_header -> biPlanes = 1;
		t_header -> biCompression = BI_RGB;
		t_header -> biBitCount = 32;
	}
	else
	{
		BITMAPV4HEADER *t_header;
		t_header = (BITMAPV4HEADER *)t_data;
		MCMemoryClear(t_header, sizeof(BITMAPV4HEADER));
		t_header -> bV4Size = sizeof(BITMAPV4HEADER);
		t_header -> bV4Width = p_bitmap->width;
		t_header -> bV4Height = -(int32_t)p_bitmap->height;
		t_header -> bV4Planes = 1;
		t_header -> bV4BitCount = 32;
		t_header -> bV4V4Compression = BI_RGB; // BI_BITFIELDS;
		t_header -> bV4SizeImage = 0;
		t_header -> bV4XPelsPerMeter = 0;
		t_header -> bV4YPelsPerMeter = 0;
		t_header -> bV4ClrUsed = 0;
		t_header -> bV4ClrImportant = 0;
		t_header -> bV4RedMask = 0x00FF0000;
		t_header -> bV4GreenMask = 0x0000FF00;
		t_header -> bV4BlueMask = 0x000000FF;
		t_header -> bV4AlphaMask = 0xFF000000;
		t_header -> bV4CSType = LCS_WINDOWS_COLOR_SPACE;
	}

	return t_buffer.CreateDataAndRelease(r_dragimage);
}

void MCImage::converttodragimage(MCDataRef& r_output)
{
	MCImageBitmap *t_bitmap = nil;
    
	if (!lockbitmap(t_bitmap, true))
		return;

	/* UNCHECKED */ MCImageBitmapToDragImage(t_bitmap, r_output);

	unlockbitmap(t_bitmap);
}
