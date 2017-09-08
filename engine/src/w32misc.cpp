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
#include "stacklst.h"
#include "stack.h"
#include "text.h"

#include "w32dc.h"
#include "w32theme.h"

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM STACKLST.CPP
//

void MCStacklist::hidepalettes(Boolean hide)
{
	active = !hide;
	if (stacks == NULL)
		return;
	MCStacknode *tptr = stacks;
	// only hide palettes if a non-palette is open
	Boolean dohide = False;
	do
	{
		MCStack *sptr = tptr->getstack();
		if (sptr->getrealmode() < WM_PALETTE)
		{
			dohide = True;
			break;
		}
		tptr = tptr->next();
	}
	while (tptr != stacks);
	if (!dohide)
		return;

	restart = False;
	tptr = stacks;
	do
	{
		MCStack *sptr = tptr->getstack();
		if (sptr->getrealmode() == WM_PALETTE && sptr->getflag(F_VISIBLE))
			if (MChidepalettes)
			{
				// Show the window non-active (to avoid messing with the focus),
				// then send a synthetic activate event to force a title-bar redraw
				ShowWindow((HWND)sptr->getw()->handle.window, hide ? SW_HIDE : SW_SHOWNA);
				PostMessageA((HWND)sptr->getw()->handle.window, WM_NCACTIVATE, 1, 0);
				// When closing or opening a window, Win32 delivers messages that can
				// change stack list, and we need to start over if this happened
				if (restart)
				{
					hidepalettes(hide);
					return;
				}

			}
			else if (sptr->getw() != NULL)
			{
				// OK-2007-04-19: Bug 4728, When hiding a palette with a windowShape
				// sptr->getw() can return null, causing crash here.
				PostMessageA((HWND)sptr->getw()->handle.window, WM_NCACTIVATE, !hide, 0);
			}
				
		tptr = tptr->next();
	}
	while (tptr != stacks);
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM CUSTOMPRINTER.CPP
//

int32_t MCCustomPrinterComputeFontSize(double p_size, void *p_font)
{
	// MW-2013-11-07: [[ Bug 10508 ]] We now do layout at 256px scale, so must
	//   scale down the font sizes.
	LOGFONTA t_logfont;
	GetObjectA((HFONT)p_font, sizeof(t_logfont), &t_logfont);

	return (-t_logfont . lfHeight) * p_size / 256.0;
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM TEXT.CPP
//

//bool MCSTextConvertToUnicode(MCTextEncoding p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
//{
//	if (p_input_length == 0)
//	{
//		r_used = 0;
//		return true;
//	}
//
//	UINT t_codepage;
//	if (p_input_encoding >= kMCTextEncodingWindowsNative)
//		t_codepage = p_input_encoding - kMCTextEncodingWindowsNative;
//	else if (p_input_encoding >= kMCTextEncodingMacNative)
//		t_codepage = 10000 + p_input_encoding - kMCTextEncodingMacNative;
//	else
//	{
//		r_used = 0;
//		return true;
//	}
//
//	// MW-2009-08-27: It is possible for t_codepage == 65001 which means UTF-8. In this case we can't
//	//   use the precomposed flag...
//
//	int t_required_size;
//	t_required_size = MultiByteToWideChar(t_codepage, t_codepage == 65001 ? 0 : MB_PRECOMPOSED, (LPCSTR)p_input, p_input_length, NULL, 0);
//	if (t_required_size > (int)p_output_length / 2)
//	{
//		r_used = t_required_size * 2;
//		return false;
//	}
//
//	int t_used;
//	t_used = MultiByteToWideChar(t_codepage, t_codepage == 65001 ? 0 : MB_PRECOMPOSED, (LPCSTR)p_input, p_input_length, (LPWSTR)p_output, p_output_length);
//	r_used = t_used * 2;
//
//	return true;
//}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM W32CONTEXT.CPP
//

bool create_temporary_mono_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits)
{
	char t_info_data[sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD)];
	BITMAPINFO *t_info = (BITMAPINFO *)t_info_data;
	HBITMAP t_bitmap;
	memset(t_info, 0, sizeof(BITMAPINFOHEADER));
	t_info -> bmiHeader . biSize = sizeof(BITMAPINFOHEADER);
	t_info -> bmiHeader . biCompression = BI_RGB;
	t_info -> bmiColors[0] . rgbRed = t_info -> bmiColors[0] . rgbGreen = t_info -> bmiColors[0] . rgbBlue = 0;
	t_info -> bmiColors[1] . rgbRed = t_info -> bmiColors[1] . rgbGreen = t_info -> bmiColors[1] . rgbBlue = 0xFF;
	t_info -> bmiColors[0] . rgbReserved = 0;
	t_info -> bmiColors[1] . rgbReserved = 0;
	t_info -> bmiHeader . biWidth = p_width;
	t_info -> bmiHeader . biHeight = -(int4)p_height;
	t_info -> bmiHeader . biPlanes = 1;
	t_info -> bmiHeader . biBitCount = 1;
	t_info -> bmiHeader . biSizeImage = p_height * ((p_width + 31) & ~31) / 8;
	t_bitmap = CreateDIBSection(p_dc, t_info, DIB_RGB_COLORS, (void **)&r_bits, NULL, 0);
	if (t_bitmap == NULL)
		return false;

	r_bitmap = t_bitmap;
	return true;
}

// MW-2006-05-26: [ Bug 3613 ] - It seems allocating a V4/V5 DIB Section causes an OS resource problem
//   So we have reverted this to use regular dib sections.
bool create_temporary_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits)
{
	HBITMAP t_bitmap;
	BITMAPV4HEADER t_bitmap_info;
	memset(&t_bitmap_info, 0, sizeof(BITMAPV4HEADER));
	t_bitmap_info . bV4Size = sizeof(BITMAPV4HEADER);
	t_bitmap_info . bV4Width = p_width;
	t_bitmap_info . bV4Height = -(int4)p_height;
	t_bitmap_info . bV4Planes = 1;
	t_bitmap_info . bV4BitCount = 32;
	// MW-2006-04-21: [[ Purify ]] Given that we are using the bV4*Mask members, this should be BI_RGB;
	t_bitmap_info . bV4V4Compression = BI_RGB;
	t_bitmap_info . bV4SizeImage = p_width * p_height * 4;
	t_bitmap_info . bV4AlphaMask = 0xFF << 24;
	t_bitmap_info . bV4RedMask = 0xFF << 16;
	t_bitmap_info . bV4GreenMask = 0xFF << 8;
	t_bitmap_info . bV4BlueMask = 0xFF;
	t_bitmap = CreateDIBSection(p_dc, (const BITMAPINFO *)&t_bitmap_info, DIB_RGB_COLORS, (void **)&r_bits, NULL, 0);
	if (t_bitmap == NULL)
		return false;

	r_bitmap = t_bitmap;
	return true;
}

static int CALLBACK RenderEnhMetaFileMask(HDC p_dc, HANDLETABLE *p_handles, const ENHMETARECORD *p_record, int p_handle_count, LPARAM p_data)
{
	SetROP2(p_dc, R2_WHITE);
	PlayEnhMetaFileRecord(p_dc, p_handles, p_record, p_handle_count);
	return 1;
}

void Windows_RenderMetaFile(HDC p_color_dc, HDC p_mask_dc, uint1 *p_data, uint4 p_length, const MCRectangle& p_dst_rect)
{
	HENHMETAFILE t_metafile;
	if (memcmp(p_data, "\xD7\xCD\xC6\x9A", 4) == 0)
	{
		int2 *t_bounds;
		t_bounds = (int2 *)&p_data[6];

		METAFILEPICT t_pict;
		t_pict . hMF = NULL;
		t_pict . mm = MM_ANISOTROPIC;
		//t_pict . xExt = (t_bounds[2] - t_bounds[0]) * 1440 / t_bounds[4];
		//t_pict . yExt = (t_bounds[3] - t_bounds[1]) * 1440 / t_bounds[4];
		t_pict . xExt = (((t_bounds[2] - t_bounds[0]) * 72 + t_bounds[4] - 1) / t_bounds[4]) * 2540 / 96;
		t_pict . yExt = (((t_bounds[3] - t_bounds[1]) * 72 + t_bounds[4] - 1) / t_bounds[4]) * 2540 / 96;
		t_metafile = SetWinMetaFileBits(p_length, p_data + 22, NULL, &t_pict);
	}
	else if (memcmp(&p_data[40], "\x20\x45\x4D\x46", 4) == 0)
		t_metafile = SetEnhMetaFileBits(p_length, p_data);

	if (t_metafile != NULL)
	{
		RECT t_dst_rect;
		t_dst_rect . left = p_dst_rect . x;
		t_dst_rect . top = p_dst_rect . y;
		t_dst_rect . right = p_dst_rect . x + p_dst_rect . width;
		t_dst_rect . bottom = p_dst_rect . y + p_dst_rect . height;

		PlayEnhMetaFile(p_color_dc, t_metafile, &t_dst_rect);
		if (p_mask_dc != NULL)
		{
			SaveDC(p_mask_dc);
			EnumEnhMetaFile(p_mask_dc, t_metafile, RenderEnhMetaFileMask, NULL, &t_dst_rect);
			RestoreDC(p_mask_dc, 1);
		}

		DeleteEnhMetaFile(t_metafile);
	}
}

void gdi_do_arc(HDC p_dc, HDC p_mask_dc, bool p_fill, int4 p_left, int4 p_top, int4 p_right, int4 p_bottom, int4 p_start, int4 p_end)
{
	if (p_start == p_end)
		return;

	if (p_start == p_end + 360)
	{
		Ellipse(p_dc, p_left, p_top, p_right, p_bottom);
		if (p_mask_dc != NULL)
			Ellipse(p_mask_dc, p_left, p_top, p_right, p_bottom);
	}
	else
	{
		real8 t_start_angle, t_end_angle;
		int4 t_x_radius, t_y_radius;
		int4 t_sx, t_sy;
		int4 t_ex, t_ey;
		int4 t_cx, t_cy;

		if (p_fill)
			p_right += 1, p_bottom += 1;

		t_start_angle = p_start * 2.0 * M_PI / 360.0;
		t_end_angle = p_end * 2.0 * M_PI / 360.0;

		t_x_radius = p_right - p_left;
		t_y_radius = p_bottom - p_top;

		t_cx = (p_left + p_right) / 2;
		t_cy = (p_top + p_bottom) / 2;

		t_sx = t_cx + int4(t_x_radius * cos(t_start_angle));
		t_sy = t_cy - int4(t_y_radius * sin(t_start_angle));
		t_ex = t_cx + int4(t_x_radius * cos(t_end_angle));
		t_ey = t_cy - int4(t_y_radius * sin(t_end_angle));

		if (t_cx == t_ex && t_cy == t_ey)
			t_ey += 1;

		if (p_fill)
		{
			Pie(p_dc, p_left, p_top, p_right, p_bottom, t_sx, t_sy, t_ex, t_ey);
			if (p_mask_dc != NULL)
				Pie(p_mask_dc, p_left, p_top, p_right, p_bottom, t_sx, t_sy, t_ex, t_ey);
		}
		else
		{
			Arc(p_dc, p_left, p_top, p_right, p_bottom, t_sx, t_sy, t_ex, t_ey);
			if (p_mask_dc != NULL)
				Arc(p_mask_dc, p_left, p_top, p_right, p_bottom, t_sx, t_sy, t_ex, t_ey);
		}
	}
}

static bool s_theme_dc_initialized = false;
static HDC s_theme_dc = NULL;

static void MCThemeDCIntialize()
{
	if (s_theme_dc_initialized)
		return;

	s_theme_dc_initialized = true;
	s_theme_dc = CreateCompatibleDC(NULL);
}

static void MCThemeDCFinalize()
{
	if (!s_theme_dc_initialized)
		return;

	DeleteDC(s_theme_dc);
	s_theme_dc_initialized = false;
}

typedef void (*MCGDIDrawFunc)(HDC p_hdc, void *p_context);

bool MCGDIDrawAlpha(uint32_t p_width, uint32_t p_height, MCGDIDrawFunc p_draw, void *p_context, MCImageBitmap *&r_bitmap)
{
	bool t_success = true;

	MCThemeDCIntialize();

	HDC t_dc;
	t_dc = s_theme_dc;
	t_success = t_dc != nil;

	bool t_alpha = false;
	bool t_trans = false;

	void *t_black_bits = nil;
	HBITMAP t_black_bitmap = NULL;
	if (t_success)
		t_success = create_temporary_dib(t_dc, p_width, p_height, t_black_bitmap, t_black_bits);

	void *t_white_bits = nil;
	HBITMAP t_white_bitmap = NULL;
	if (t_success)
		t_success = create_temporary_dib(t_dc, p_width, p_height, t_white_bitmap, t_white_bits);

	MCImageBitmap *t_bitmap = nil;
	if (t_success)
		t_success = MCImageBitmapCreate(p_width, p_height, t_bitmap);

	if (t_success)
	{
		RECT t_rect;
		SetRect(&t_rect, 0, 0, p_width, p_height);

		SelectObject(t_dc, t_black_bitmap);
		FillRect(t_dc, &t_rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
		p_draw(t_dc, p_context);

		SelectObject(t_dc, t_white_bitmap);
		FillRect(t_dc, &t_rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
		p_draw(t_dc, p_context);

		GdiFlush();

		uint32_t *t_white_ptr = (uint32_t*)t_white_bits;
		uint32_t *t_black_ptr = (uint32_t*)t_black_bits;
		uint32_t *t_dst_ptr = (uint32_t*)t_bitmap->data;
		for(uint4 y = 0; y < p_height; ++y)
		{
			for(uint4 x = 0; x < p_width; ++x)
			{
				uint32_t t_white_pixel = *t_white_ptr++;
				uint32_t t_black_pixel = *t_black_ptr++;

				uint8_t t_ar, t_ag, t_ab;
				t_ar = 255 - ((t_white_pixel >> 16) & 0xFF) + ((t_black_pixel >> 16) & 0xFF);
				//t_ag = 255 - ((t_white_pixel >> 8) & 0xFF) + ((t_black_pixel >> 8) & 0xFF);
				//t_ab = 255 - ((t_white_pixel >> 0) & 0xFF) + ((t_black_pixel >> 0) & 0xFF);
				//MCAssert(t_ar == t_ag && t_ar == t_ab);

				t_trans = t_trans || t_ar != 0xFF;
				t_alpha = t_alpha || (t_ar > 0 && t_ar < 255);

				*t_dst_ptr++ = t_black_pixel & 0xFFFFFF | (t_ar << 24);
			}
		}
	}

	DWORD t_err;
	if (!t_success)
		t_err = GetLastError();

	//if (t_dc != nil)
	//	DeleteDC(t_dc);
	if (t_black_bitmap != nil)
		DeleteObject(t_black_bitmap);
	if (t_white_bitmap != nil)
		DeleteObject(t_white_bitmap);

	if (t_success)
	{
		r_bitmap = t_bitmap;
		t_bitmap->has_transparency = t_trans;
		t_bitmap->has_transparency = t_alpha;
	}
	else
		MCImageFreeBitmap(t_bitmap);

	return t_success;
}

bool create_temporary_mask_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits)
{
	char t_info_data[sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD)];
	BITMAPINFO *t_info = (BITMAPINFO *)t_info_data;
	HBITMAP t_bitmap;
	memset(t_info, 0, sizeof(BITMAPINFOHEADER));
	t_info -> bmiHeader . biSize = sizeof(BITMAPINFOHEADER);
	t_info -> bmiHeader . biCompression = BI_RGB;
	// GDI mask bitmaps should be white == transparent, black == opaque so we can easily "erase" the occupied pixels
	// in the destination context with the SRCAND operation.
	// Internally, we use 0 to represent transparent & 1 to represent opaque, so the color table should map
	// 0 to white & 1 to black
	t_info -> bmiColors[0] . rgbRed = t_info -> bmiColors[0] . rgbGreen = t_info -> bmiColors[0] . rgbBlue = 0xFF;
	t_info -> bmiColors[1] . rgbRed = t_info -> bmiColors[1] . rgbGreen = t_info -> bmiColors[1] . rgbBlue = 0x0;
	t_info -> bmiColors[0] . rgbReserved = 0;
	t_info -> bmiColors[1] . rgbReserved = 0;
	t_info -> bmiHeader . biWidth = p_width;
	t_info -> bmiHeader . biHeight = -(int4)p_height;
	t_info -> bmiHeader . biPlanes = 1;
	t_info -> bmiHeader . biBitCount = 1;
	t_info -> bmiHeader . biSizeImage = p_height * ((p_width + 31) & ~31) / 8;
	t_bitmap = CreateDIBSection(p_dc, t_info, DIB_RGB_COLORS, (void **)&r_bits, NULL, 0);
	if (t_bitmap == NULL)
		return false;

	r_bitmap = t_bitmap;
	return true;
}

bool MCImageBitmapSplitHBITMAPWithMask(HDC p_dc, MCImageBitmap *p_bitmap, HBITMAP &r_bitmap, HBITMAP &r_mask)
{
	bool t_success = true;

	HBITMAP t_bitmap = nil;
	HBITMAP t_mask = nil;
	void *t_bitmap_bits = nil;
	void *t_mask_bits = nil;
	uint32_t t_mask_stride;
	uint32_t t_bitmap_stride = p_bitmap->width * sizeof(uint32_t);

	bool t_trans, t_alpha;

	t_mask_stride = (((p_bitmap->width + 7) / 8) + 3) & ~3;
	t_trans = MCImageBitmapHasTransparency(p_bitmap, t_alpha);

	t_success = create_temporary_dib(p_dc, p_bitmap->width, p_bitmap->height, t_bitmap, t_bitmap_bits);
	if (t_success && t_trans)
		t_success = create_temporary_mask_dib(p_dc, p_bitmap->width, p_bitmap->height, t_mask, t_mask_bits);

	if (t_success)
	{
		MCMemoryCopy(t_bitmap_bits, p_bitmap->data, p_bitmap->stride * p_bitmap->height);
		if (t_trans)
			MCImageBitmapExtractMask(p_bitmap, t_mask_bits, t_mask_stride, 0);

		// If there's an alpha-channel we composite the color bits against a white
		// backdrop as this gives a slightly more pleasant effect generally than
		// black...
		if (t_alpha)
		{
			uint8_t *t_src_ptr = (uint8_t*)t_bitmap_bits;
			for(uint32_t y = 0; y < p_bitmap->height; y++)
			{
				uint32_t *t_src_row = (uint32_t*)t_src_ptr;
				for(uint32_t x = 0; x < p_bitmap->width; x++)
				{
					uint8_t t_alpha = *t_src_row >> 24;
					if (t_alpha == 0)
						*t_src_row = 0xFFFFFFFF;
					else if (t_alpha != 255)
						*t_src_row = 0xFF000000 | (*t_src_row + 0x010101 * (255 - t_alpha));
					t_src_row++;
				}
				t_src_ptr += t_bitmap_stride;
			}
		}

		r_bitmap = t_bitmap;
		r_mask = t_mask;

		return true;
	}

	if (t_bitmap != nil)
		DeleteObject(t_bitmap);
	if (t_mask != nil)
		DeleteObject(t_mask);

	return false;
}

bool MCGImageSplitHBITMAPWithMask(HDC p_dc, MCGImageRef p_image, HBITMAP &r_bitmap, HBITMAP &r_mask)
{
	MCGRaster t_raster;
	if (!MCGImageGetRaster(p_image, t_raster))
		return false;

	MCImageBitmap t_bitmap;
	t_bitmap = MCImageBitmapFromMCGRaster(t_raster);

	MCImageBitmapCheckTransparency(&t_bitmap);

	return MCImageBitmapSplitHBITMAPWithMask(p_dc, &t_bitmap, r_bitmap, r_mask);
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM GLOBALS.CPP
//

MCUIDC *MCCreateScreenDC(void)
{
	return new MCScreenDC;
}
