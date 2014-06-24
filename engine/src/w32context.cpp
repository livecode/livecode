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

#include "w32prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "globals.h"
#include "path.h"
#include "packed.h"
#include "paint.h"
#include "bitmapeffect.h"

#include "w32dc.h"
#include "w32context.h"
#include "w32theme.h"

static DWORD s_image_mode_map[] =
{
	BLACKNESS,	/* DDx		GXclear */
	SRCAND,			/* SDa		GXand */
	SRCERASE,		/* SDna		GXandReverse */
	SRCCOPY,		/* S			GXcopy */
	0x220326,		/* SnDa		GXandInverted */
	0xAA0029,		/* D			GXnoop */
	SRCINVERT,	/* SDx		GXxor */
	SRCPAINT,		/* SDo		GXor */
	NOTSRCERASE,/* SnDna	GXnor */
	0x990066,		/* SnDx		GXequiv */
	DSTINVERT,	/* Dn			GXinvert */
	0xDD0228,		/* SDno		GXorReverse */
	NOTSRCCOPY,	/* Sn			GXcopyInverted */
	MERGEPAINT, /* SnDo		GXorInverted */
	0x7700E6,		/* SnDno	GXnand */
	WHITENESS,	/* DDnx		GXset */
	SRCERASE,		/* SDna		GXsrcBic */
	SRCAND			/* SDa		GXnotSrcBic */
};


static DWORD s_masked_image_mode_map[] =
{
	0x0A0329,	/* 0x0A	GXclear */
	0x8A0E06,	/* 0x8A GXand */
	0x4A0789,	/* 0x4A GXandReverse */
	0xCA0749,	/* 0xCA GXcopy */
	0x2A0CC9,	/* 0x2A GXandInverted */
	0xAA0029,	/* 0xAA GXnoop */
	0x6A01E9,	/* 0x6A GXxor */
	0xEA02E9,	/* 0xEA GXor */
	0x1A06C5,	/* 0x1A GXnor */
	0x9A0709,	/* 0x9A GXequiv */
	0x5A0049,	/* 0x5A GXinvert */
	0xDA1CE9,	/* 0xDA GXorReverse */
	0x3A0644,	/* 0x3A GXcopyInverted */
	0xBA0B09, /* 0xBA GXorInverted */
	0x7A1E29,	/* 0x7A GXnand */
	0xFA0089,	/* 0xFA GXset */
	0x4A0789,	/* 0x4A GXsrcBic */
	0x8A0E06	/* 0x8A GXnotSrcBic */
};

static int s_function_mode_map[] =
{
	R2_BLACK,				/* GXclear */
	R2_MASKPEN,			/* GXand */
	R2_MASKPENNOT,	/* GXandReverse */
	R2_COPYPEN,			/* GXcopy */
	R2_MASKNOTPEN,	/* GXandInverted */
	R2_NOP,					/* GXnoop */
	R2_XORPEN,			/* GXxor */
	R2_MERGEPEN,		/* GXor */
	R2_NOTMERGEPEN,	/* GXnor */
	R2_NOTXORPEN,		/* GXequiv */
	R2_NOT,					/* GXinvert */
	R2_MERGEPENNOT,	/* GXorReverse */
	R2_NOTCOPYPEN,	/* GXcopyInverted */
	R2_MERGENOTPEN,	/* GXorInverted */
	R2_NOTMASKPEN,	/* GXnand */
	R2_WHITE,				/* GXset */
	R2_MASKPENNOT,	/* GXsrcBic */
	R2_MASKPEN			/* GXnotSrcBic */
};

static uint1 s_function_needs_alpha[NUM_INKS] =
{
	// Bitwise
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0,

	// Arithmetic
	0, 0, 0, 0, 0, 0, 0, 0,

	// Structural Blends
	1, 1, 1, 0, 1, 1, 1, 1,
	1, 0, 1, 1,

	// Imaging Blends
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

#define LAST_NATIVE_FUNCTION GXnotSrcBic


typedef HRESULT (WINAPI*DrawThemeBackgroundPtr)(HANDLE hTheme, HDC hdc, int iPartId,
        int iStateId, const RECT *pRect,
        const RECT* pClipRect);
typedef HRESULT (WINAPI *GetThemeBackgroundRegionPtr)(HANDLE hTheme, OPTIONAL HDC hdc, int iPartId,
				int iStateId, const RECT *pRect, HRGN *pRegion);
extern GetThemeBackgroundRegionPtr getThemeBackgroundRegion;
extern DrawThemeBackgroundPtr drawThemeBG;

typedef void (*surface_combiner_t)(void *p_dst, int32_t p_dst_stride, const void *p_src, uint4 p_src_stride, uint4 p_width, uint4 p_height, uint1 p_opacity);
extern surface_combiner_t s_surface_combiners[];
extern surface_combiner_t s_surface_combiners_nda[];
uint4 g_current_background_colour = 0x000000;

extern void surface_merge(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height);
extern void surface_merge_with_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);
extern void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
extern void surface_merge_with_alpha_non_pre(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
extern void surface_merge_with_alpha_and_mask(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);
extern void surface_extract_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
extern void surface_unmerge_pre(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height);

extern void surface_combine_blendSrcOver_masked(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity);
extern void surface_combine_blendSrcOver_solid(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity);

void gdi_blend(HDC p_dst, int p_dx, int p_dy, int p_dw, int p_dh, HDC p_src, int p_sx, int p_sy, int p_sw, int p_sh, int p_opacity);
bool create_temporary_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits);
bool create_temporary_mono_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits);

MCGDIContext::MCGDIContext(void)
{
	memset(((uint1 *)this) + sizeof(MCContext), 0, sizeof(MCGDIContext) - sizeof(MCContext));
}

MCGDIContext::~MCGDIContext(void)
{
	if (f_external_alpha != NULL)
	{
		if (getflags(FLAG_MASK_CHANGED))
			flush_mask();
	}

	if (getflags(FLAG_IS_WINDOW))
		ReleaseDC(f_destination_surface . window, f_destination_dc);
	else
		DeleteDC(f_destination_dc);

	if (getflags(FLAG_IS_MEMORY))
		DeleteObject(f_destination_surface . bitmap);

	if (f_mask_dc != NULL)
		DeleteDC(f_mask_dc);

	if (f_mask_surface != NULL)
		DeleteObject(f_mask_surface);

	if (f_current_brush != NULL)
		DeleteObject(f_current_brush);

	if (f_current_pen != NULL)
		DeleteObject(f_current_pen);

	if (f_current_mask_pen != NULL)
		DeleteObject(f_current_mask_pen);

	if (f_stroke . dash . data != NULL)
		delete[] f_stroke . dash . data;
}

MCContextType MCGDIContext::gettype(void) const
{
	if (getflags(FLAG_IS_PRINTER))
		return CONTEXT_TYPE_PRINTER;
	if (getflags(FLAG_IS_TRANSIENT))
		return CONTEXT_TYPE_OFFSCREEN;
	return CONTEXT_TYPE_SCREEN;
}

void MCGDIContext::setprintmode(void)
{
	setflags(FLAG_IS_PRINTER, true);
}

bool MCGDIContext::changeopaque(bool p_new_value)
{
	bool t_old_value;
	t_old_value = getflags(FLAG_IS_OPAQUE);
	setflags(FLAG_IS_OPAQUE, p_new_value);
	return t_old_value;
}

MCGDIContext *MCGDIContext::create_with_bitmap(HBITMAP p_bitmap, bool p_transient)
{
	MCGDIContext *t_context;
	BITMAP t_info;

	GetObjectA(p_bitmap, sizeof(BITMAP), &t_info);

	t_context = new MCGDIContext;
	t_context -> f_destination_dc = CreateCompatibleDC(NULL);
	t_context -> f_depth = (uint1)t_info . bmBitsPixel;
	t_context -> f_destination_surface . bitmap = p_bitmap;
	t_context -> f_destination_bits = t_info . bmBits;
	t_context -> f_flags = FLAG_FILL_CHANGED | FLAG_STROKE_CHANGED | FLAG_FOREGROUND_CHANGED | FLAG_BACKGROUND_CHANGED | FLAG_FONT_CHANGED | FLAG_IS_OPAQUE;
	t_context -> f_fill . style = FillSolid;
	t_context -> f_fill . colour = t_context -> getwhite() . pixel;
	t_context -> f_stroke . style = LineSolid;
	t_context -> f_stroke . cap = CapRound;
	t_context -> f_stroke . join = JoinRound;
	t_context -> f_background = t_context -> getblack() . pixel;
	t_context -> f_function = GXcopy;
	t_context -> f_width = (uint2)t_info . bmWidth;
	t_context -> f_height = (uint2)t_info . bmHeight;
	t_context -> f_origin . x = 0;
	t_context -> f_origin . y = 0;
	t_context -> f_opacity = 255;
	t_context -> f_effects = NULL;

	if (p_transient)
		t_context -> f_flags |= FLAG_IS_TRANSIENT;

	SetROP2(t_context -> f_destination_dc, R2_COPYPEN);
	SelectObject(t_context -> f_destination_dc, p_bitmap);
	SelectObject(t_context -> f_destination_dc, GetStockObject(NULL_PEN));
	SelectObject(t_context -> f_destination_dc, GetStockObject(NULL_BRUSH));
	SetTextAlign(t_context -> f_destination_dc, TA_LEFT | TA_BASELINE);
	SelectClipRgn(t_context -> f_destination_dc, NULL);
	SetViewportOrgEx(t_context -> f_destination_dc, 0, 0, NULL);

	return t_context; 
}

MCGDIContext *MCGDIContext::create_with_window(HWND p_window)
{
	MCGDIContext *t_context;
	t_context = new MCGDIContext;

	RECT t_window_rectangle;
	GetClientRect(p_window, &t_window_rectangle);

	t_context -> f_destination_dc = GetDC(p_window); // Released
	t_context -> f_destination_surface . window = p_window;

	// MW-2009-06-14: Windows buffers are always opaque.
	t_context -> f_flags = FLAG_IS_WINDOW | FLAG_FILL_CHANGED | FLAG_STROKE_CHANGED | FLAG_FOREGROUND_CHANGED | FLAG_BACKGROUND_CHANGED | FLAG_FONT_CHANGED | FLAG_IS_OPAQUE;

	t_context -> f_fill . style = FillSolid;
	t_context -> f_fill . colour = t_context -> getwhite() . pixel;
	t_context -> f_stroke . style = LineSolid;
	t_context -> f_stroke . cap = CapRound;
	t_context -> f_stroke . join = JoinRound;
	t_context -> f_background = t_context -> getblack() . pixel;
	t_context -> f_function = GXcopy;
	t_context -> f_width = uint2(t_window_rectangle . right - t_window_rectangle . left);
	t_context -> f_height = uint2(t_window_rectangle . bottom - t_window_rectangle . top);
	t_context -> f_origin . x = 0;
	t_context -> f_origin . y = 0;
	t_context -> f_depth = (uint1)MCscreen -> getdepth();
	t_context -> f_opacity = 255;
	t_context -> f_effects = NULL;

	SetROP2(t_context -> f_destination_dc, R2_COPYPEN);
	SelectObject(t_context -> f_destination_dc, GetStockObject(NULL_PEN));
	SelectObject(t_context -> f_destination_dc, GetStockObject(NULL_BRUSH));
	SetTextAlign(t_context -> f_destination_dc, TA_LEFT | TA_BASELINE);
	SelectClipRgn(t_context -> f_destination_dc, NULL);
	SetViewportOrgEx(t_context -> f_destination_dc, 0, 0, NULL);

	return t_context;
}

MCGDIContext *MCGDIContext::create_with_parameters(uint4 p_width, uint4 p_height, bool p_alpha, bool p_transient)
{
	MCGDIContext *t_context;

	t_context = new MCGDIContext;
	t_context -> f_destination_dc = CreateCompatibleDC(NULL);
	t_context -> f_depth = 32;
	t_context -> f_flags = FLAG_IS_MEMORY | FLAG_FILL_CHANGED | FLAG_STROKE_CHANGED | FLAG_FOREGROUND_CHANGED | FLAG_BACKGROUND_CHANGED | FLAG_FONT_CHANGED;
	t_context -> f_fill . style = FillSolid;
	t_context -> f_fill . colour = t_context -> getwhite() . pixel;
	t_context -> f_stroke . style = LineSolid;
	t_context -> f_stroke . cap = CapRound;
	t_context -> f_stroke . join = JoinRound;
	t_context -> f_background = t_context -> getblack() . pixel;
	t_context -> f_function = GXcopy;
	t_context -> f_width = p_width;
	t_context -> f_height = p_height;
	t_context -> f_origin . x = 0;
	t_context -> f_origin . y = 0;
	t_context -> f_opacity = 255;
	t_context -> f_effects = NULL;

	if (p_transient)
		t_context -> f_flags |= FLAG_IS_TRANSIENT;

	// MW-2009-06-14: An non-alpha buffer, is opaque by default
	if (!p_alpha)
		t_context -> f_flags |= FLAG_IS_OPAQUE;

	create_temporary_dib(t_context -> f_destination_dc, p_width, p_height, t_context -> f_destination_surface . bitmap, t_context -> f_destination_bits);

	SetROP2(t_context -> f_destination_dc, R2_COPYPEN);
	SelectObject(t_context -> f_destination_dc, t_context -> f_destination_surface . bitmap);
	SelectObject(t_context -> f_destination_dc, GetStockObject(NULL_PEN));
	SelectObject(t_context -> f_destination_dc, GetStockObject(NULL_BRUSH));
	SetTextAlign(t_context -> f_destination_dc, TA_LEFT | TA_BASELINE);
	SelectClipRgn(t_context -> f_destination_dc, NULL);
	SetViewportOrgEx(t_context -> f_destination_dc, 0, 0, NULL);

	HGDIOBJ t_old_brush;
	t_old_brush = SelectObject(t_context -> f_destination_dc, GetStockObject(BLACK_BRUSH));
	Rectangle(t_context -> f_destination_dc, 0, 0, p_width, p_height);
	SelectObject(t_context -> f_destination_dc, t_old_brush);

	if (p_alpha)
	{
		t_context -> f_mask_dc = CreateCompatibleDC(NULL);
		t_context -> f_flags |= FLAG_HAS_MASK | FLAG_IS_ALPHA | FLAG_MASK_ACTIVE;

		create_temporary_mono_dib(t_context -> f_mask_dc, p_width, p_height, t_context -> f_mask_surface, t_context -> f_mask_bits);

		SetROP2(t_context -> f_mask_dc, R2_COPYPEN);
		SelectObject(t_context -> f_mask_dc, t_context -> f_mask_surface);
		SelectObject(t_context -> f_mask_dc, GetStockObject(NULL_PEN));
		SelectObject(t_context -> f_mask_dc, GetStockObject(NULL_BRUSH));
		SetTextColor(t_context -> f_mask_dc, MCscreen -> getwhite() . pixel);
		SetBkColor(t_context -> f_mask_dc, MCscreen -> getwhite() . pixel);
		SetTextAlign(t_context -> f_mask_dc, TA_LEFT | TA_BASELINE);
		SelectClipRgn(t_context -> f_mask_dc, NULL);
		SetViewportOrgEx(t_context -> f_mask_dc, 0, 0, NULL);

		HGDIOBJ t_old_brush;
		t_old_brush = SelectObject(t_context -> f_mask_dc, GetStockObject(BLACK_BRUSH));
		Rectangle(t_context -> f_mask_dc, 0, 0, p_width, p_height);
		SelectObject(t_context -> f_mask_dc, t_old_brush);
	}

	return t_context; 
}

void MCGDIContext::setexternalalpha(MCBitmap *p_alpha)
{
	f_external_alpha = p_alpha;

	MCGDIContext *t_context;
	t_context = this;

	t_context -> f_mask_dc = CreateCompatibleDC(NULL);
	t_context -> f_flags |= FLAG_HAS_MASK | FLAG_IS_ALPHA | FLAG_MASK_ACTIVE;
	
	// MW-2009-06-14: When we attach an external alpha channel, we become non-opaque
	setflags(FLAG_IS_OPAQUE, false);

	create_temporary_mono_dib(t_context -> f_mask_dc, f_width, f_height, t_context -> f_mask_surface, t_context -> f_mask_bits);

	SetROP2(t_context -> f_mask_dc, R2_COPYPEN);
	SelectObject(t_context -> f_mask_dc, t_context -> f_mask_surface);
	SelectObject(t_context -> f_mask_dc, GetStockObject(NULL_PEN));
	SelectObject(t_context -> f_mask_dc, GetStockObject(NULL_BRUSH));
	SetTextColor(t_context -> f_mask_dc, MCscreen -> getwhite() . pixel);
	SetBkColor(t_context -> f_mask_dc, MCscreen -> getwhite() . pixel);
	SetTextAlign(t_context -> f_mask_dc, TA_LEFT | TA_BASELINE);
	SelectClipRgn(t_context -> f_mask_dc, NULL);
	SetViewportOrgEx(t_context -> f_mask_dc, 0, 0, NULL);

	HGDIOBJ t_old_brush;
	t_old_brush = SelectObject(t_context -> f_mask_dc, GetStockObject(BLACK_BRUSH));
	Rectangle(t_context -> f_mask_dc, 0, 0, f_width, f_height);
	SelectObject(t_context -> f_mask_dc, t_old_brush);
}

HDC MCGDIContext::gethdc(void)
{
	return f_destination_dc;
}

HDC MCGDIContext::getmaskhdc(void)
{
	if (f_mask_surface == NULL)
		return NULL;

	setflags(FLAG_MASK_CHANGED, true);

	return f_mask_dc;
}

// MW-2009-06-10: [[ Bitmap Effects ]] Factor out the common 'begin' implementation.
//   This method starts a new layer with the given clip rect.
void MCGDIContext::begin_common(const MCRectangle& p_clip)
{
	if (getflags(FLAG_MASK_CHANGED))
		flush_mask();

	// Create a new layer
	Layer *t_layer;
	t_layer = new Layer;

	// Push the current info into the layer structure
	memset(t_layer, 0, sizeof(Layer));
	t_layer -> parent = f_layers;
	t_layer -> flags = f_flags & FLAGMASK_LAYER_SPECIFIC;
	t_layer -> origin = f_origin;
	t_layer -> clip = getclip();
	t_layer -> destination = f_destination_surface . bitmap;
	t_layer -> mask = f_mask_surface;
	t_layer -> destination_bits = f_destination_bits;
	t_layer -> mask_bits = f_mask_bits;
	t_layer -> function = f_function;
	t_layer -> opacity = f_opacity;
	t_layer -> width = f_width;
	t_layer -> height = f_height;
	t_layer -> nesting = f_nesting;
	t_layer -> effects = f_effects;
	t_layer -> effects_shape = f_effects_shape;

	// Set the clip to the requested rect.
	f_clip = p_clip;
	f_origin . x = p_clip . x;
	f_origin . y = p_clip . y;
	f_width = p_clip . width;
	f_height = p_clip . height;

	// MW-2009-06-14: [[ Bitmap Effects ]] Layers, by default, have no
	//   effects to apply.
	f_effects = NULL;

	// Set the appropriate flags
	f_flags = (f_flags & ~(FLAGMASK_LAYER_SPECIFIC)) | FLAG_HAS_MASK | FLAG_MASK_ACTIVE | FLAG_IS_ALPHA | FLAG_HAS_CLIP;

	// Create new surfaces
	create_temporary_dib(f_destination_dc, f_width, f_height, f_destination_surface . bitmap, f_destination_bits);
	create_temporary_mono_dib(f_destination_dc, f_width, f_height, f_mask_surface, f_mask_bits);

	// Make sure we have appropriate DC's
	if (f_mask_dc == NULL)
	{
		f_mask_dc = CreateCompatibleDC(NULL);
		if (f_font != NULL)
			SelectObject(f_mask_dc, f_font -> fid);
		SelectObject(f_mask_dc, GetStockObject(NULL_PEN));
		SelectObject(f_mask_dc, GetStockObject(NULL_BRUSH));
		SetTextColor(f_mask_dc, getwhite() . pixel);
		SetBkColor(f_mask_dc, getwhite() . pixel);
		SetTextAlign(f_mask_dc, TA_LEFT | TA_BASELINE);
	}

	// Reset graphics state
	setfunction(GXcopy);
	setopacity(255);

	// Select appropriate objects into the DCs
	SelectObject(f_destination_dc, f_destination_surface . bitmap);
	SelectObject(f_mask_dc, f_mask_surface);

	SelectClipRgn(f_destination_dc, NULL);
	SelectClipRgn(f_mask_dc, NULL);
	SetViewportOrgEx(f_destination_dc, -f_origin . x, -f_origin . y, NULL);
	SetViewportOrgEx(f_mask_dc, -f_origin . x, -f_origin . y, NULL);

	SelectObject(f_mask_dc, GetStockObject(BLACK_BRUSH));
	Rectangle(f_mask_dc, f_origin . x, f_origin . y, f_origin . x + f_width, f_origin . y + f_height);
	SelectObject(f_mask_dc, GetStockObject(NULL_BRUSH));

	SelectObject(f_destination_dc, GetStockObject(BLACK_BRUSH));
	Rectangle(f_destination_dc, f_origin . x, f_origin . y, f_origin . x + f_width, f_origin . y + f_height);
	SelectObject(f_destination_dc, GetStockObject(NULL_BRUSH));

	// We have a new layer, so start nesting at 0
	f_nesting = 0;
	f_layers = t_layer;
}

void MCGDIContext::begin(bool p_overlap)
{
	assert(!getflags(FLAG_IS_WINDOW));

// *** OPTIMIZATION - high quality and blendSrcOver or GXcopy are compatible!
	if (!p_overlap && f_opacity == 255 &&
			((f_quality == QUALITY_DEFAULT && f_function <= LAST_NATIVE_FUNCTION) ||
			 (f_quality == QUALITY_SMOOTH && (f_function == GXblendSrcOver || f_function == GXcopy))))
	{
		f_nesting += 1;
		return;
	}

	begin_common(getclip());
}

bool MCGDIContext::begin_with_effects(MCBitmapEffectsRef p_effects, const MCRectangle& p_shape)
{
	assert(!getflags(FLAG_IS_WINDOW));

	// Compute the region of the shape required to correctly render the given
	// clip.
	MCRectangle t_layer_clip;
	MCBitmapEffectsComputeClip(p_effects, p_shape, getclip(), t_layer_clip);

	if (t_layer_clip . width == 0 || t_layer_clip . height == 0)
		return false;

	begin_common(t_layer_clip);

	f_effects = p_effects;
	f_effects_shape = p_shape;

	return true;
}

void MCGDIContext::end(void)
{
	// If the layer was implicit, reduce nesting and return.
	if (f_nesting > 0)
	{
		f_nesting -= 1;
		return;
	}

	// Pop the destination layer into t_layer.
	Layer *t_layer;
	t_layer = f_layers;

	assert(t_layer != NULL);

	// Ensure the mask is in sync.
	if (getflags(FLAG_MASK_CHANGED))
		flush_mask();

	// Now fetch the details of the source layer.
	HBITMAP t_layer_surface;
	void *t_layer_bits;
	MCRectangle t_layer_rect;
	MCBitmapEffectsRef t_layer_effects;
	MCRectangle t_layer_effects_shape;
	t_layer_surface = f_destination_surface . bitmap;
	t_layer_bits = f_destination_bits;
	MCU_set_rect(t_layer_rect, f_origin . x, f_origin . y, f_width, f_height);
	t_layer_effects = f_effects;
	t_layer_effects_shape = f_effects_shape;

	// Set the context parameters to the destination layer.
	SelectObject(f_destination_dc, t_layer -> destination);
	if (f_layers -> mask != NULL)
		SelectObject(f_mask_dc, t_layer -> mask);
	else
	{
		DeleteDC(f_mask_dc);
		f_mask_dc = NULL;
	}
	DeleteObject(f_mask_surface);

	f_destination_surface . bitmap = t_layer -> destination;
	f_destination_bits = t_layer -> destination_bits;
	f_mask_surface = t_layer -> mask;
	f_mask_bits = t_layer -> mask_bits;
	f_width = t_layer -> width;
	f_height = t_layer -> height;
	f_opacity = t_layer -> opacity;
	f_nesting = t_layer -> nesting;
	f_flags = t_layer -> flags | (f_flags & ~FLAGMASK_LAYER_SPECIFIC);
	f_effects = t_layer -> effects;
	f_effects_shape = t_layer -> effects_shape;

	setfunction(t_layer -> function);
	setorigin(t_layer -> origin . x, t_layer -> origin . y);

	setclip(t_layer -> clip);
	
	// Work out the combining function.
	uint4 t_function;
	if (!getflags(FLAG_IS_ALPHA) && s_function_needs_alpha[f_function])
		t_function = GXblendSrcOver;
	else
		t_function = f_function;

	if (t_layer_effects == NULL)
	{
		// MW-2009-06-10: [[ Bitmap Effects ]] The source layer is no longer guaranteed
		//   to be a sub-rect of the dest, so compute the required bounds.
		MCRectangle t_layer_src_rect;
		t_layer_src_rect = MCU_intersect_rect(t_layer_rect, f_clip);

		// Blend source into destination
		if ((t_function == GXcopy || t_function == GXblendSrcOver) && MCmajorosversion >= 0x0500)
		{
			HDC t_src_dc;
			t_src_dc = ((MCScreenDC *)MCscreen) -> getsrchdc();

			HGDIOBJ t_old_src;
			t_old_src = SelectObject(t_src_dc, t_layer_surface);

			gdi_blend(f_destination_dc, t_layer_src_rect . x, t_layer_src_rect . y, t_layer_src_rect . width, t_layer_src_rect . height, t_src_dc, 0, 0, t_layer_src_rect . width, t_layer_src_rect . height, f_opacity);

			SelectObject(t_src_dc, t_old_src);
		}
		else
		{
			uint4 *t_dst_ptr, *t_src_ptr;
			surface_combiner_t t_combiner;

			t_dst_ptr = (uint4 *)f_destination_bits + f_width * (t_layer_src_rect . y - f_origin . y) + (t_layer_src_rect . x - f_origin . x);
			t_src_ptr = (uint4 *)t_layer_bits + t_layer_rect . width * (t_layer_src_rect . y - t_layer_rect . y) + (t_layer_src_rect . x - t_layer_rect . x);
			t_combiner = getflags(FLAG_IS_ALPHA) ? s_surface_combiners[t_function] : s_surface_combiners_nda[t_function];
	 
			t_combiner(t_dst_ptr, f_width * 4, t_src_ptr, t_layer_rect . width * 4, t_layer_src_rect . width, t_layer_src_rect . height, f_opacity);
		}
	}
	else
	{
		// For now, if opacity is not 100%, or we have a non-srcOver function then we must
		// use a temporary buffer.
		void *t_tmp_bits;
		uint32_t t_tmp_stride;
		if (f_opacity != 255 || (t_function != GXblendSrcOver && t_function != GXcopy))
		{
			t_tmp_bits = malloc(f_clip . width * f_clip . height * sizeof(uint4));
			t_tmp_stride = f_clip . width * sizeof(uint4);
			memset(t_tmp_bits, 0, f_clip . width * f_clip . height * sizeof(uint4));
		}
		else
			t_tmp_bits = NULL, t_tmp_stride = 0;

		MCBitmapEffectLayer t_dst;
		t_dst . bounds = f_clip;
		if (t_tmp_bits == NULL)
		{
			t_dst . stride = f_width * sizeof(uint4);
			t_dst . bits = (uint4 *)f_destination_bits + f_width * (f_clip . y - f_origin . y) + (f_clip . x - f_origin . x);
			t_dst . has_alpha = getflags(FLAG_IS_ALPHA);
		}
		else
		{
			t_dst . stride = t_tmp_stride;
			t_dst . bits = t_tmp_bits;
			t_dst . has_alpha = true;
		}

		MCBitmapEffectLayer t_src;
		t_src . bounds = t_layer_rect;
		t_src . stride = t_layer_rect . width * sizeof(uint4);
		t_src . bits = t_layer_bits;
		t_src . has_alpha = true;

		MCBitmapEffectsRender(t_layer_effects, t_layer_effects_shape, t_dst, t_src);

		if (t_tmp_bits != NULL)
		{
			surface_combiner_t t_combiner;
			t_combiner = getflags(FLAG_IS_ALPHA) ? s_surface_combiners[t_function] : s_surface_combiners_nda[t_function];
			t_combiner(
				(uint4 *)f_destination_bits + f_width * (f_clip . y - f_origin . y) + (f_clip . x - f_origin . x), f_width * 4,
				t_tmp_bits, f_clip . width * sizeof(uint4), f_clip . width, f_clip . height, f_opacity);
			free(t_tmp_bits);
		}
	}

	// MW-2009-06-14: If the function needed an input alpha channel, it means
	//   that it could have made the layer transparent (partially) thus we
	//   must unset the opaque flag
	if (s_function_needs_alpha[f_function])
		setflags(FLAG_IS_OPAQUE, false);

	f_layers = t_layer -> parent;

	DeleteObject(t_layer_surface);
	delete t_layer;
}

void MCGDIContext::setclip(const MCRectangle& rect)
{
	f_clip = rect;

	HRGN t_region = CreateRectRgn(rect.x - f_origin . x, rect.y - f_origin . y, (rect.x + rect.width) - f_origin . x, (rect.y + rect.height) - f_origin . y);
	if (t_region != NULL)
	{
		SelectObject(f_destination_dc, t_region);
		if (getflags(FLAG_MASK_ACTIVE))
			SelectObject(f_mask_dc, t_region);

		DeleteObject(t_region);
	}

	setflags(FLAG_HAS_CLIP, true);
}

const MCRectangle& MCGDIContext::getclip(void) const
{
	if (getflags(FLAG_HAS_CLIP))
		return f_clip;

	static MCRectangle s_rect;
	MCU_set_rect(s_rect, f_origin . x, f_origin . y, f_width, f_height);
	
	return s_rect;
}

void MCGDIContext::clearclip(void)
{
	if (getflags(FLAG_HAS_CLIP))
	{
		SelectClipRgn(f_destination_dc, NULL);
		if (getflags(FLAG_MASK_ACTIVE))
			SelectClipRgn(f_mask_dc, NULL);

		setflags(FLAG_HAS_CLIP, false);
	}
}

void MCGDIContext::setorigin(int2 x, int2 y)
{
	f_origin . x = x;
	f_origin . y = y;

	SetViewportOrgEx(f_destination_dc, -x, -y, NULL);
	if (getflags(FLAG_MASK_ACTIVE))
		SetViewportOrgEx(f_mask_dc, -x, -y, NULL);
}

void MCGDIContext::clearorigin(void)
{
	setorigin(0, 0);
}

void MCGDIContext::setquality(uint1 quality)
{
	f_quality = quality;
}

void MCGDIContext::setfunction(uint1 function)
{
	if (function != f_function)
	{
		f_function = function;
		if (function <= LAST_NATIVE_FUNCTION)
			SetROP2(f_destination_dc, s_function_mode_map[f_function]);
		else
			SetROP2(f_destination_dc, s_function_mode_map[GXcopy]);
	}
}

uint1 MCGDIContext::getfunction(void)
{
	return f_function;
}

void MCGDIContext::setopacity(uint1 opacity)
{
	f_opacity = opacity;
}

uint1 MCGDIContext::getopacity(void)
{
	return f_opacity;
}

void MCGDIContext::setforeground(const MCColor& c)
{
	// MW-2011-09-22: [[ Bug ]] Compute the new pixel (as it will be).
	uint32_t t_new_pixel;
	t_new_pixel = (c . pixel & 0x00ff00) | ((c . pixel & 0x0000ff) << 16) | ((c . pixel & 0xff0000) >> 16);
	if (t_new_pixel != f_fill . colour)
	{
		// MW-2011-07-15: [[ COLOR ]] The 'pixel' field of MCColor is now 0xXXRRGGBB
		//   universally. As Win32 uses 0xXXBBGGRR, we must flip.
		f_fill . colour = t_new_pixel;
		setflags(FLAG_FILL_CHANGED | FLAG_STROKE_CHANGED | FLAG_FOREGROUND_CHANGED, true);
	}
}

void MCGDIContext::setbackground(const MCColor& c)
{
	if (c . pixel != f_background)
	{
		// MW-2011-07-15: [[ COLOR ]] The 'pixel' field of MCColor is now 0xXXRRGGBB
		//   universally. As Win32 uses 0xXXBBGGRR, we must flip.
		f_background = (c . pixel & 0x00ff00) | ((c . pixel & 0x0000ff) << 16) | ((c . pixel & 0xff0000) >> 16);
		setflags(FLAG_BACKGROUND_CHANGED, true);
	}
}

void MCGDIContext::setdashes(uint2 p_offset, const uint1 *p_data, uint2 p_length)
{
	delete[] f_stroke . dash . data;
	f_stroke . dash . data = new uint4[p_length + 2];

	bool t_on;
	uint2 t_start;

	t_start = 0;
	t_on = true;

	while(p_offset > 0 && p_offset >= p_data[t_start])
	{
		p_offset -= p_data[t_start++];
		t_start %= p_length;
		t_on = !t_on;
	}

	uint2 t_current;
	t_current = 0;

	f_stroke . dash . length = p_length;

	if (!t_on)
	{
		f_stroke . dash . data[t_current++] = 0;
		f_stroke . dash . length += 1;
	}

	f_stroke . dash . data[t_current++] = p_data[t_start++] - p_offset;

	for(uint4 t_index = 1; t_index < p_length; ++t_index)
	{
		f_stroke . dash . data[t_current++] = p_data[t_start++];
		t_start %= p_length;
	}

	if (p_offset != 0)
	{
		f_stroke . dash . data[t_current++] = p_offset;
		f_stroke . dash . length++;
	}

	setflags(FLAG_DASHES_CHANGED, true);
}

void MCGDIContext::setfillstyle(uint2 p_style, Pixmap p_pattern, int2 p_origin_x, int2 p_origin_y)
{
	bool t_changed = false;

	if (p_style != f_fill . style || p_pattern != f_fill . pattern || p_origin_x != f_fill . origin . x || p_origin_y != f_fill . origin . y)
	{
		if (p_style != FillTiled || p_pattern != NULL)
		{
			f_fill . style = p_style;
			f_fill . pattern = p_pattern;
			f_fill . origin . x = p_origin_x;
			f_fill . origin . y = p_origin_y;
		}
		else
		{
			f_fill . style = FillSolid;
			f_fill . pattern = p_pattern;
			f_fill . origin . x = 0;
			f_fill . origin . y = 0;
		}

		t_changed = true;
	}

	if(t_changed)
		setflags(FLAG_FILL_CHANGED | FLAG_STROKE_CHANGED, true);
}

void MCGDIContext::getfillstyle(uint2& style, Pixmap& p, int2& x, int2& y)
{
	style = f_fill . style;
	p = f_fill . pattern;
	x = f_fill . origin . x;
	y = f_fill . origin . y;
}

void MCGDIContext::setgradient(MCGradientFill *p_gradient)
{
	bool t_changed = false;
	if (f_gradient_fill == NULL)
	{
		if (p_gradient != NULL)
		{
			t_changed = true;
			f_gradient_fill = p_gradient;
		}
	}
	else
	{
		if (p_gradient == NULL)
		{
			t_changed = true;
			f_gradient_fill = p_gradient;
		}
		else
		{
			if (p_gradient->kind != f_gradient_fill->kind || p_gradient->ramp_length != f_gradient_fill->ramp_length)
			{
				t_changed = true;
				f_gradient_fill = p_gradient;
			}
			else
				for (uint4 i=0; i<p_gradient->ramp_length; i++)
					if (p_gradient->ramp[i].offset != f_gradient_fill->ramp[i].offset || p_gradient->ramp[i].color != f_gradient_fill->ramp[i].color)
					{
						t_changed = true;
						f_gradient_fill = p_gradient;
						break;
					}
		}
	}

	if (t_changed)
		setflags(FLAG_FILL_CHANGED | FLAG_STROKE_CHANGED, true);
}

void MCGDIContext::setlineatts(uint2 p_width, uint2 p_style, uint2 p_cap, uint2 p_join)
{
	if (getflags(FLAG_DASHES_CHANGED) || p_width != f_stroke . width || p_style != f_stroke . style || p_cap != f_stroke . cap || p_join != f_stroke . join)
	{
		f_stroke . width = p_width;
		f_stroke . style = p_style;
		f_stroke . cap = p_cap;
		f_stroke . join = p_join;
		changeflags(FLAG_STROKE_CHANGED | FLAG_DASHES_CHANGED, FLAG_STROKE_CHANGED);
	}
}

void MCGDIContext::setmiterlimit(real8 p_limit)
{
	if (getflags(FLAG_DASHES_CHANGED) || p_limit != f_stroke.miter_limit)
	{
		f_stroke.miter_limit = p_limit;
		changeflags(FLAG_STROKE_CHANGED | FLAG_DASHES_CHANGED, FLAG_STROKE_CHANGED);
	}
}

void MCGDIContext::drawline(int2 x1, int2 y1, int2 x2, int2 y2)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		POINT t_points[3];

		t_points[0] . x = x1;
		t_points[0] . y = y1;
		t_points[1] . x = x2;
		t_points[1] . y = y2;
		t_points[2] . x = x2 + 1;
		t_points[2] . y = y2;

		gdi_polyline(t_points, 2);
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_line(x1, y1, x2, y2, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCGDIContext::drawlines(MCPoint *p_points, uint2 p_count, bool p_closed)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		POINT t_static_points[5];
		POINT *t_points;

		if (p_count < 5)
			t_points = t_static_points;
		else
			t_points = new POINT[p_count + 1];

		if (t_points == NULL)
			return;

		for(uint4 t_point = 0; t_point < p_count; ++t_point)
		{
			t_points[t_point] . x = p_points[t_point] . x;
			t_points[t_point] . y = p_points[t_point] . y;
		}
		t_points[p_count] . x = t_points[p_count - 1] . x + 1;
		t_points[p_count] . y = t_points[p_count - 1] . y;

		gdi_polyline(t_points, p_count);

		if (p_count >= 5)
			delete[] t_points;
	}
	else
	{
		MCPath *t_path;
		if (p_closed)
			t_path = MCPath::create_polygon(p_points, p_count, true);
		else
			t_path = MCPath::create_polyline(p_points, p_count, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCGDIContext::drawsegments(MCSegment *p_segments, uint2 p_count)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		POINT *t_points = NULL;
		uint4 *t_counts = NULL;
		uint4 t_count;

		t_count = f_stroke . width == 0 ? 3 : 2;
		t_points = new POINT[p_count * t_count];
		t_counts = new uint4[p_count];

		if (t_points != NULL && t_counts != NULL)
		{
			for(uint4 t_segment = 0; t_segment < p_count; ++t_segment)
			{
				t_points[t_segment * t_count + 0] . x = p_segments[t_segment] . x1;
				t_points[t_segment * t_count + 0] . y = p_segments[t_segment] . y1;
				t_points[t_segment * t_count + 1] . x = p_segments[t_segment] . x2;
				t_points[t_segment * t_count + 1] . y = p_segments[t_segment] . y2;
				if (t_count == 3)
				{
					t_points[t_segment * t_count + 2] . x = p_segments[t_segment] . x2 + 1;
					t_points[t_segment * t_count + 2] . y = p_segments[t_segment] . y2;
				}
				t_counts[t_segment] = t_count;
			}

			gdi_polylines(t_points, t_counts, p_count);
		}

		delete[] t_points;
		delete[] t_counts;
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_polypolyline(p_segments, p_count, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCGDIContext::drawtext(int2 p_x, int2 p_y, const char *p_string, uint2 p_length, MCFontStruct *f, Boolean image, bool p_unicode_override)
{
	if (f != f_font)
	{
		f_font = f;
		setflags(FLAG_FONT_CHANGED, true);
	}

	gdi_text(p_x, p_y, p_string, p_length, image ? true : false, p_unicode_override);
}

void MCGDIContext::drawrect(const MCRectangle& rect)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		gdi_stroke_begin();
		gdi_rectangle(rect . x, rect . y, rect . x + rect . width, rect . y + rect . height);
		gdi_stroke_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rectangle(rect, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCGDIContext::fillrect(const MCRectangle& rect)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		gdi_fill_begin();
		gdi_rectangle(rect . x, rect . y, rect . x + rect . width + 1, rect . y + rect . height + 1);
		gdi_fill_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rectangle(rect, false);
		fillpath(t_path);
		t_path -> release();
	}
}

void MCGDIContext::fillrects(MCRectangle *p_rectangles, uint2 p_count)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		gdi_fill_begin();
		for(uint4 t_rectangle = 0; t_rectangle < p_count; ++t_rectangle)
			gdi_rectangle(p_rectangles[t_rectangle] . x, p_rectangles[t_rectangle] . y, p_rectangles[t_rectangle] . x + p_rectangles[t_rectangle] . width + 1, p_rectangles[t_rectangle] . y + p_rectangles[t_rectangle] . height + 1);
		gdi_fill_end();
	}
	else
	{
		for(uint4 t_rectangle = 0; t_rectangle < p_count; ++t_rectangle)
			fillrect(p_rectangles[t_rectangle]);
	}
}

void MCGDIContext::fillpolygon(MCPoint *p_points, uint2 p_count)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		POINT *t_points;
		t_points = new POINT[p_count];

		if (t_points != NULL)
		{
			for(uint4 t_point = 0; t_point < p_count; ++t_point)
				t_points[t_point] . x = p_points[t_point] . x, t_points[t_point] . y = p_points[t_point] . y;

			gdi_polygon(t_points, p_count);
		}

		delete[] t_points;
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_polygon(p_points, p_count, true);
		fillpath(t_path);
		t_path -> release();
	}
}

void MCGDIContext::drawroundrect(const MCRectangle& p_rectangle, uint2 p_radius)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		gdi_stroke_begin();
		gdi_round_rectangle(p_rectangle . x, p_rectangle . y, p_rectangle . x + p_rectangle . width, p_rectangle . y + p_rectangle . height, p_radius);
		gdi_stroke_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rounded_rectangle(p_rectangle, p_radius, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCGDIContext::fillroundrect(const MCRectangle& p_rectangle, uint2 p_radius)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		gdi_fill_begin();
		gdi_round_rectangle(p_rectangle . x, p_rectangle . y, p_rectangle . x + p_rectangle . width + 1, p_rectangle . y + p_rectangle . height + 1, p_radius);
		gdi_fill_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rounded_rectangle(p_rectangle, p_radius, false);
		fillpath(t_path);
		t_path -> release();
	}
}

void MCGDIContext::drawarc(const MCRectangle& p_rectangle, uint2 p_start, uint2 p_angle)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		gdi_stroke_begin();
		gdi_arc(p_rectangle . x, p_rectangle . y, p_rectangle . x + p_rectangle . width, p_rectangle . y + p_rectangle . height, p_start, p_start + p_angle);
		gdi_stroke_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_arc(p_rectangle, p_start, p_angle, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCGDIContext::drawsegment(const MCRectangle& p_rectangle, uint2 p_start, uint2 p_angle)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		gdi_stroke_begin();
		gdi_arc(p_rectangle . x, p_rectangle . y, p_rectangle . x + p_rectangle . width, p_rectangle . y + p_rectangle . height, p_start, p_start + p_angle);
		gdi_stroke_end();

		int2 cx = p_rectangle.x + (p_rectangle.width >> 1);
		int2 cy = p_rectangle.y + (p_rectangle.height >> 1);
		real8 torad = M_PI * 2.0 / 360.0;
		real8 tw = (real8)p_rectangle.width;
		real8 th = (real8)p_rectangle.height;
		real8 sa = (real8)p_start * torad;
		
		int2 dx = cx + (int2)(cos(sa) * tw / 2.0);
		int2 dy = cy - (int2)(sin(sa) * th / 2.0);
		drawline(cx, cy, dx, dy);

		sa = (real8)(p_start + p_angle) * torad;
		dx = cx + (int2)(cos(sa) * tw / 2.0);
		dy = cy - (int2)(sin(sa) * th / 2.0);
		drawline(cx, cy, dx, dy);
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_segment(p_rectangle, p_start, p_angle, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCGDIContext::fillarc(const MCRectangle& p_rectangle, uint2 p_start, uint2 p_angle)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		gdi_fill_begin();
		gdi_arc(p_rectangle . x, p_rectangle . y, p_rectangle . x + p_rectangle . width, p_rectangle . y + p_rectangle . height, p_start, p_start + p_angle);
		gdi_fill_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_segment(p_rectangle, p_start, p_angle, false);
		fillpath(t_path);
		t_path -> release();
	}
}

void MCGDIContext::drawpath(MCPath *path)
{
	MCCombiner *t_combiner = combiner_lock();

	if (t_combiner == NULL)
		return;

	MCRectangle t_clip;
	t_clip = getclip();
	uint2 t_width = f_stroke.width;
	if (t_width == 0)
		f_stroke.width = 1;
	path -> stroke(t_combiner, t_clip, &f_stroke);
	f_stroke.width = t_width;
	combiner_unlock(t_combiner);
}

void MCGDIContext::fillpath(MCPath *path, bool p_evenodd)
{
	MCCombiner *t_combiner = combiner_lock();

	if (t_combiner == NULL)
		return;

	MCRectangle t_clip;
	t_clip = getclip();
	path -> fill(t_combiner, t_clip, p_evenodd);
	combiner_unlock(t_combiner);
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

void MCGDIContext::drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect)
{
	HDC t_mask_dc;
	if (getflags(FLAG_MASK_ACTIVE))
	{
		t_mask_dc = f_mask_dc;
		setflags(FLAG_MASK_CHANGED, true);
	}
	else
		t_mask_dc = NULL;

	Windows_RenderMetaFile(f_destination_dc, t_mask_dc, data, length, drect);
}

void MCGDIContext::draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty,
												   const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length,
													 const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect)
{
}

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
#if 1
	char t_info_data[sizeof(BITMAPINFOHEADER) + 3 * sizeof(RGBQUAD)];
	BITMAPINFO *t_info = (BITMAPINFO *)t_info_data;
	memset(t_info, 0, sizeof(BITMAPINFOHEADER));
	t_info -> bmiHeader . biSize = sizeof(BITMAPINFOHEADER);
	t_info -> bmiHeader . biCompression = BI_BITFIELDS;
	*(DWORD *)&t_info -> bmiColors[0] = 0x00FF0000;
	*(DWORD *)&t_info -> bmiColors[1] = 0x0000FF00;
	*(DWORD *)&t_info -> bmiColors[2] = 0x000000FF;
	t_info -> bmiHeader . biWidth = p_width;
	t_info -> bmiHeader . biHeight = -(int4)p_height;
	t_info -> bmiHeader . biPlanes = 1;
	t_info -> bmiHeader . biBitCount = 32;
	t_info -> bmiHeader . biSizeImage = 0; //p_height * ((p_width + 31) & ~31) / 8;
	t_bitmap = CreateDIBSection(p_dc, t_info, DIB_RGB_COLORS, (void **)&r_bits, NULL, 0);
#else
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
#endif
	if (t_bitmap == NULL)
		return false;

	r_bitmap = t_bitmap;
	return true;
}

void MCGDIContext::drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy)
{
#ifdef LIBGRAPHICS_BROKEN
	HBITMAP t_bitmap;
	void *t_bits;
	HDC t_src_dc, t_dst_dc;

	MCRectangle t_clip, t_dr;
	t_clip = getclip();
	MCU_set_rect(t_dr, dx, dy, sw, sh);
	t_dr = MCU_intersect_rect(t_dr, t_clip);
	if (t_dr . width == 0 || t_dr . height == 0)
		return;

	sx += t_dr . x - dx;
	sy += t_dr . y - dy;
	sw = t_dr . width;
	sh = t_dr . height;
	dx = t_dr . x;
	dy = t_dr . y;

	if (getflags(FLAG_MASK_CHANGED))
		flush_mask();

	// MW-2011-09-22: Special case for alpha-blended/solid image with GXcopy/GXblendSrcOver.
	if (f_external_alpha == NULL && (f_function == GXcopy || f_function == GXblendSrcOver))
	{
		uint8_t *t_dst_ptr;
		uint4 t_dst_stride;
		t_dst_stride = f_width * 4;
		t_dst_ptr = (uint8_t *)f_destination_bits + t_dst_stride * (dy - f_origin . y) + (dx - f_origin . x) * sizeof(uint32_t);

		uint8_t *t_src_ptr;
		uint32_t t_src_stride;
		t_src_stride = p_image.bitmap->stride;
		t_src_ptr = (uint8_t*)p_image.bitmap->data + sy * t_src_stride + sx * 4;

		if (MCImageBitmapHasTransparency(p_image.bitmap))
			surface_combine_blendSrcOver_masked(t_dst_ptr, t_dst_stride, t_src_ptr, t_src_stride, sw, sh, f_opacity);
		else
			surface_combine_blendSrcOver_solid(t_dst_ptr, t_dst_stride, t_src_ptr, t_src_stride, sw, sh, f_opacity);

		return;
	}

	t_src_dc = ((MCScreenDC *)MCscreen) -> getsrchdc();
	t_dst_dc = ((MCScreenDC *)MCscreen) -> getdsthdc();

	if (!create_temporary_dib(t_dst_dc, sw, sh, t_bitmap, t_bits))
		return;


	HGDIOBJ t_old_dst;//, t_old_src;
	t_old_dst = SelectObject(t_dst_dc, t_bitmap);

	GdiFlush();

	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	t_pixel_stride = sw * 4;
	t_pixel_ptr = (uint4 *)t_bits;

	MCImageBitmapPremultiplyRegion(p_image.bitmap, sx, sy, sw, sh, t_pixel_stride, t_pixel_ptr);

	uint4 t_function;
	if (!getflags(FLAG_IS_ALPHA) && s_function_needs_alpha[f_function])
		t_function = GXblendSrcOver;
	else
		t_function = f_function;

	if (f_external_alpha == NULL && (t_function == GXcopy || t_function == GXblendSrcOver) && MCmajorosversion >= 0x0500)
		gdi_blend(f_destination_dc, dx, dy, sw, sh, t_dst_dc, 0, 0, sw, sh, f_opacity);
	else
	{
		uint4 *t_dst_ptr;
		surface_combiner_t t_combiner;

		t_dst_ptr = (uint4 *)f_destination_bits + f_width * (dy - f_origin . y) + (dx - f_origin . x);
		t_combiner = getflags(FLAG_IS_ALPHA) ? s_surface_combiners[t_function] : s_surface_combiners_nda[t_function];

		if (f_external_alpha != NULL)
			surface_merge_with_alpha(t_dst_ptr, f_width * 4, f_external_alpha -> data + (dy - f_origin . y) * f_external_alpha -> bytes_per_line + (dx - f_origin . x), f_external_alpha -> bytes_per_line, sw, sh);

		t_combiner(t_dst_ptr, f_width * 4, t_pixel_ptr, t_pixel_stride, sw, sh, f_opacity);

		if (f_external_alpha != NULL)
		{
			surface_extract_alpha(t_dst_ptr, f_width * 4, f_external_alpha -> data + (dy - f_origin . y) * f_external_alpha -> bytes_per_line + (dx - f_origin . x), f_external_alpha -> bytes_per_line, sw, sh);
			surface_unmerge_pre(t_dst_ptr, f_width * 4, sw, sh);
		}
	}

	SelectObject(t_dst_dc, t_old_dst);

	DeleteObject(t_bitmap);
#endif
}

int4 MCGDIContext::textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override)
{
	return MCscreen -> textwidth(f, s, l, p_unicode_override);
}

void MCGDIContext::applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height)
{
	if (p_mask -> is_sharp)
		return;

	if (getflags(FLAG_MASK_CHANGED))
		flush_mask();

	MCRectangle t_clip;
	t_clip = getclip();

	void *t_dst_ptr;
	void *t_src_ptr;
	t_dst_ptr = ((uint4 *)f_destination_bits) + (t_clip . y - f_origin . y) * f_width + (t_clip . x - f_origin . x);
	t_src_ptr = p_mask -> data + t_clip . y * p_mask -> stride + t_clip . x;
	
	// MW-2012-11-21: [[ Bug ]] Make sure we only merge up to the maximum size of the mask.
	surface_merge_with_alpha(t_dst_ptr, f_width * 4, t_src_ptr, p_mask -> stride, MCU_min(t_clip . width, p_mask -> width), MCU_min(t_clip . height, p_mask -> height));
}

MCBitmap *MCGDIContext::lock(void)
{
	MCBitmap *t_bitmap;
	t_bitmap = new MCBitmap;
	t_bitmap -> width = f_width;
	t_bitmap -> height = f_height;
	t_bitmap -> format = ZPixmap;
	t_bitmap -> bitmap_unit = 32;
	t_bitmap -> byte_order = MSBFirst;
	t_bitmap -> bitmap_pad = 32;
	t_bitmap -> bitmap_bit_order = MSBFirst;
	t_bitmap -> depth = 32;
	t_bitmap -> bytes_per_line = f_width * 4;
	t_bitmap -> bits_per_pixel = 32;
	t_bitmap -> red_mask = 0xFF;
	t_bitmap -> green_mask = 0xFF;
	t_bitmap -> blue_mask = 0xFF;
	t_bitmap -> data = (char *)f_destination_bits;
	t_bitmap -> bm = NULL;
	if (getflags(FLAG_MASK_CHANGED))
		flush_mask();
	return t_bitmap;
}

void MCGDIContext::unlock(MCBitmap *p_bitmap)
{
	delete p_bitmap;
}

uint2 MCGDIContext::getdepth(void) const
{
	return f_depth;
}

const MCColor& MCGDIContext::getblack(void) const
{
	return MCscreen -> black_pixel;
}

const MCColor& MCGDIContext::getwhite(void) const
{
	return MCscreen -> white_pixel;
}

const MCColor& MCGDIContext::getgray(void) const
{
	return MCscreen -> gray_pixel;
}

const MCColor& MCGDIContext::getbg(void) const
{
	return MCscreen -> background_pixel;
}

void MCGDIContext::clear(const MCRectangle *p_rect)
{
	MCRectangle t_rect;
	if (p_rect == nil)
		MCU_set_rect(t_rect, f_origin . x, f_origin . y, f_origin . x + f_width, f_origin . y + f_height);
	else
		t_rect = *p_rect;

	if (f_mask_dc != NULL)
	{
		HGDIOBJ t_old_brush;
		t_old_brush = SelectObject(f_mask_dc, GetStockObject(BLACK_BRUSH));
		Rectangle(f_mask_dc, t_rect . x, t_rect . y, t_rect . x + t_rect . width, t_rect . y + t_rect . height);
		SelectObject(f_mask_dc, t_old_brush);
		setflags(FLAG_MASK_CHANGED, false);
	}

	HGDIOBJ t_old_brush;
	t_old_brush = SelectObject(f_destination_dc, GetStockObject(BLACK_BRUSH));
	Rectangle(f_destination_dc, t_rect . x, t_rect . y, t_rect . x + t_rect . width, t_rect . y + t_rect . height);
	SelectObject(f_destination_dc, t_old_brush);
}

MCRegionRef MCGDIContext::computemaskregion(void)
{
	assert(f_mask_surface != NULL);

	if (getflags(FLAG_MASK_CHANGED))
		flush_mask();

	uint1 *t_alpha_ptr;
	t_alpha_ptr = (uint1 *)f_destination_bits + 3;

	uint4 t_rect_limit = 128;
	RGNDATA *t_data;

	t_data = (RGNDATA *)malloc(sizeof(RGNDATAHEADER) + sizeof(RECT) * t_rect_limit);
	if (t_data == NULL)
		return NULL;

	HRGN t_region;
	t_region = NULL;

	t_data -> rdh . dwSize = sizeof(RGNDATAHEADER);
	t_data -> rdh . iType = RDH_RECTANGLES;
	t_data -> rdh . nCount = 0;
	t_data -> rdh . nRgnSize = 0;
	SetRect(&t_data -> rdh . rcBound, MAXLONG, MAXLONG, 0, 0);

	for(int y = 0; y < f_height; y++)
	{
		for(int x = 0; x < f_width;)
		{
			while(x < f_width && *t_alpha_ptr == 0)
				t_alpha_ptr += 4, x++;

			int x0 = x;
			while(x < f_width && *t_alpha_ptr != 0)
				t_alpha_ptr += 4, x++;

			if (x > x0)
			{
				if (t_data -> rdh . nCount >= t_rect_limit)
				{
					RGNDATA *t_new_data;
					t_rect_limit += 128;
					t_new_data = (RGNDATA *)realloc(t_data, sizeof(RGNDATAHEADER) + sizeof(RECT) * t_rect_limit);
					if (t_new_data == NULL)
					{
						if (t_region != NULL)
							DeleteObject(t_region);
						free(t_data);
						return NULL;
					}
					t_data = t_new_data;
				}

				SetRect(&((RECT *)&t_data -> Buffer)[t_data -> rdh . nCount], f_origin . x + x0, f_origin . y + y, f_origin . x + x, f_origin .y + y + 1);
				if (x0 < t_data -> rdh . rcBound . left)
					t_data -> rdh . rcBound . left = f_origin . x + x0;
				if (y < t_data -> rdh . rcBound . top)
					t_data -> rdh . rcBound . top = f_origin . y + y;
				if (x > t_data -> rdh . rcBound . right)
					t_data -> rdh . rcBound . right = f_origin . x + x;
				if (y + 1 > t_data -> rdh . rcBound . bottom)
					t_data -> rdh . rcBound . bottom = f_origin . y + y + 1;
				t_data -> rdh . nCount += 1;

				if (t_data -> rdh . nCount == 2048)
				{
					HRGN t_new_region;
					t_new_region = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + sizeof(RECT) * t_rect_limit, t_data);
					if (t_region)
					{
						CombineRgn(t_region, t_region, t_new_region, RGN_OR);
						DeleteObject(t_new_region);
					}
					else
						t_region = t_new_region;
					t_data -> rdh . nCount = 0;
					SetRect(&t_data -> rdh . rcBound, MAXLONG, MAXLONG, 0, 0);
				}
			}
		}
	}
	
	HRGN t_new_region;
	t_new_region = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + sizeof(RECT) * t_rect_limit, t_data);
	if (t_region)
	{
		CombineRgn(t_region, t_region, t_new_region, RGN_OR);
		DeleteObject(t_new_region);
	}
	else
		t_region = t_new_region;

	free(t_data);

	return (MCRegionRef)t_region;
}

void MCGDIContext::copyarea(Drawable p_src, uint4 p_dx, uint4 p_dy, uint4 p_sx, uint4 p_sy, uint4 p_sw, uint4 p_sh)
{
	if (p_src -> type == DC_BITMAP)
		gdi_image(NULL, p_src, p_sx, p_sy, p_sw, p_sh, p_dx, p_dy);
	else if (p_src -> type == DC_BITMAP_WITH_DC)
	{
		BitBlt(f_destination_dc, p_dx, p_dy, p_sw, p_sh, (HDC)((_ExtendedDrawable *)p_src) -> hdc, p_sx, p_sy, SRCCOPY);
		if (getflags(FLAG_MASK_ACTIVE))
		{
			HGDIOBJ t_old_brush;
			t_old_brush = SelectObject(f_mask_dc, GetStockObject(WHITE_BRUSH));
			Rectangle(f_mask_dc, p_dx, p_dy, p_dx + p_sw + 1, p_dy + p_sh + 1);
			SelectObject(f_mask_dc, t_old_brush);
			setflags(FLAG_MASK_CHANGED, true);
		}
	}
}

void MCGDIContext::drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_info_ptr)
{
	MCThemeDrawInfo& p_info = *p_info_ptr;

	MCRectangle t_old_clip;
	HRGN t_clip_region = NULL;
	t_old_clip = getclip();

	if (p_info . clip_interior)
	{
		HRGN t_outside_region;
		t_outside_region = CreateRectRgn(t_old_clip . x - f_origin . x, t_old_clip . y - f_origin . y, t_old_clip . x + t_old_clip . width - f_origin . x, t_old_clip . y + t_old_clip . height - f_origin . y);

		HRGN t_inside_region;
		t_inside_region = CreateRectRgn(p_info . interior . x - f_origin . x, p_info . interior . y - f_origin . y, p_info . interior . x + p_info . interior . width - f_origin . x, p_info . interior . y + p_info . interior . height - f_origin . y);

		CombineRgn(t_inside_region, t_outside_region, t_inside_region, RGN_DIFF);
		DeleteObject(t_outside_region);

		t_clip_region = t_inside_region;
	}

	if (t_clip_region != NULL)
		SelectClipRgn(f_destination_dc, t_clip_region);

	RECT t_widget_rect;
	RECT t_clip_rect;
	SetRect(&t_widget_rect, p_info . bounds . x, p_info . bounds . y, p_info . bounds . x + p_info . bounds . width, p_info . bounds . y + p_info . bounds . height);
	SetRect(&t_clip_rect, p_info . clip . x, p_info . clip . y, p_info . clip . x + p_info . clip . width, p_info . clip . y + p_info . clip . height);
	drawThemeBG(p_info . theme, f_destination_dc, p_info . part, p_info . state, &t_widget_rect, &t_clip_rect);

	if (f_mask_dc != NULL)
	{
		HRGN t_region;

		clearclip();

		getThemeBackgroundRegion(p_info . theme, f_destination_dc, p_info . part, p_info . state, &t_widget_rect, &t_region);
		OffsetRgn(t_region, -f_origin . x, -f_origin . y);
		SelectClipRgn(f_mask_dc, t_region);
		if (t_clip_region != NULL)
			ExtSelectClipRgn(f_mask_dc, t_clip_region, RGN_AND);

		HGDIOBJ t_old_brush, t_old_pen;
		t_old_brush = SelectObject(f_mask_dc, GetStockObject(WHITE_BRUSH));
		t_old_pen = SelectObject(f_mask_dc, GetStockObject(NULL_PEN));
		Rectangle(f_mask_dc, p_info . clip . x, p_info . clip . y, p_info . clip . x + p_info . clip . width + 1, p_info . clip . y + p_info . clip . height + 1);
		SelectObject(f_mask_dc, t_old_pen);
		SelectObject(f_mask_dc, t_old_brush);

		DeleteObject(t_region);

		setflags(FLAG_MASK_CHANGED, true);
	}

	if (t_clip_region != NULL || f_mask_dc != NULL)
	{
		if (t_clip_region != NULL)
			DeleteObject(t_clip_region);
		setclip(t_old_clip);
	}
}

void MCGDIContext::drawlink(const char *p_link, const MCRectangle& p_region)
{
}

void MCGDIContext::combine(Pixmap p_src, int4 dx, int4 dy, int4 sx, int4 sy, uint4 sw, uint4 sh)
{
	HDC t_src_dc;

	MCRectangle t_clip, t_dr;
	t_clip = getclip();
	MCU_set_rect(t_dr, dx, dy, sw, sh);
	t_dr = MCU_intersect_rect(t_dr, t_clip);
	if (t_dr . width == 0 || t_dr . height == 0)
		return;

	sx += t_dr . x - dx;
	sy += t_dr . y - dy;
	sw = t_dr . width;
	sh = t_dr . height;
	dx = t_dr . x;
	dy = t_dr . y;

	void *t_pixel_ptr;
	uint4 t_pixel_stride;

	if (!MCscreen -> lockpixmap(p_src, t_pixel_ptr, t_pixel_stride))
		return;

	t_pixel_ptr = (uint4 *)t_pixel_ptr + sy * t_pixel_stride / 4 + sx;

	t_src_dc = ((MCScreenDC *)MCscreen) -> getsrchdc();

	HGDIOBJ t_old_src;
	t_old_src = SelectObject(t_src_dc, p_src -> handle . pixmap);

	GdiFlush();

	uint4 t_function;
	if (!getflags(FLAG_IS_ALPHA) && s_function_needs_alpha[f_function])
		t_function = GXblendSrcOver;
	else
		t_function = f_function;

	if ((t_function == GXcopy || t_function == GXblendSrcOver) && MCmajorosversion >= 0x0500)
		gdi_blend(f_destination_dc, dx, dy, sw, sh, t_src_dc, sx, sy, sw, sh, f_opacity);
	else
	{
		uint4 *t_dst_ptr;
		surface_combiner_t t_combiner;

		t_dst_ptr = (uint4 *)f_destination_bits + f_width * (dy - f_origin . y) + (dx - f_origin . x);
		t_combiner = getflags(FLAG_IS_ALPHA) ? s_surface_combiners[t_function] : s_surface_combiners_nda[t_function];

		t_combiner(t_dst_ptr, f_width * 4, t_pixel_ptr, t_pixel_stride, sw, sh, f_opacity);
	}

	SelectObject(t_src_dc, t_old_src);
}

// Mask Utilities

void MCGDIContext::flush_mask(void)
{
	GdiFlush();

	uint1 *t_mask_ptr;
	t_mask_ptr = (uint1 *)f_mask_bits;
	uint4 t_mask_stride;
	t_mask_stride = ((f_width + 31) & ~31) / 8;

	if (f_external_alpha == NULL)
	{
		uint1 *t_pixel_ptr;
		uint4 t_pixel_stride;

		t_pixel_stride = f_width * 4;

		t_pixel_ptr = (uint1 *)(f_destination_bits) + 3;
		
		for(uint4 y = f_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_mask_ptr += t_mask_stride)
		{
			uint4 x;

			uint1 *t_pixels = t_pixel_ptr;
			uint1 *t_mskels = t_mask_ptr;

			for(x = f_width; x >= 8; x -= 8)
			{
				uint1 b = *t_mskels;
				*t_mskels++ = 0;

				if ((b & (1 << 7)) != 0) t_pixels[0] = 0xFF;
				if ((b & (1 << 6)) != 0) t_pixels[4] = 0xFF;
				if ((b & (1 << 5)) != 0) t_pixels[8] = 0xFF;
				if ((b & (1 << 4)) != 0) t_pixels[12] = 0xFF;
				if ((b & (1 << 3)) != 0) t_pixels[16] = 0xFF;
				if ((b & (1 << 2)) != 0) t_pixels[20] = 0xFF;
				if ((b & (1 << 1)) != 0) t_pixels[24] = 0xFF;
				if ((b & (1 << 0)) != 0) t_pixels[28] = 0xFF;

				t_pixels += 32;
			}

			if (x == 0)
				continue;

			uint1 b = *t_mskels;
			*t_mskels = 0;

			switch(7 - x)
			{
			case 0: if ((b & (1 << 1)) != 0) t_pixels[24] = 0xFF;
			case 1: if ((b & (1 << 2)) != 0) t_pixels[20] = 0xFF;
			case 2:	if ((b & (1 << 3)) != 0) t_pixels[16] = 0xFF;
			case 3: if ((b & (1 << 4)) != 0) t_pixels[12] = 0xFF;
			case 4: if ((b & (1 << 5)) != 0) t_pixels[8] = 0xFF;
			case 5: if ((b & (1 << 6)) != 0) t_pixels[4] = 0xFF;
			case 6: if ((b & (1 << 7)) != 0) t_pixels[0] = 0xFF;
			default:
				break;
			}
		}
	}
	else
	{
		uint1 *t_pixel_ptr;
		uint4 t_pixel_stride;

		t_pixel_stride = f_external_alpha -> bytes_per_line;
		t_pixel_ptr = (uint1 *)f_external_alpha -> data;
		
		for(uint4 y = f_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_mask_ptr += t_mask_stride)
		{
			uint4 x;

			uint1 *t_pixels = t_pixel_ptr;
			uint1 *t_mskels = t_mask_ptr;

			for(x = f_width; x >= 8; x -= 8)
			{
				uint1 b = *t_mskels;
				*t_mskels++ = 0;

				if ((b & (1 << 7)) != 0) t_pixels[0] = 0xFF;
				if ((b & (1 << 6)) != 0) t_pixels[1] = 0xFF;
				if ((b & (1 << 5)) != 0) t_pixels[2] = 0xFF;
				if ((b & (1 << 4)) != 0) t_pixels[3] = 0xFF;
				if ((b & (1 << 3)) != 0) t_pixels[4] = 0xFF;
				if ((b & (1 << 2)) != 0) t_pixels[5] = 0xFF;
				if ((b & (1 << 1)) != 0) t_pixels[6] = 0xFF;
				if ((b & (1 << 0)) != 0) t_pixels[7] = 0xFF;

				t_pixels += 8;
			}

			if (x == 0)
				continue;

			uint1 b = *t_mskels;
			*t_mskels = 0;

			switch(7 - x)
			{
			case 0: if ((b & (1 << 1)) != 0) t_pixels[6] = 0xFF;
			case 1: if ((b & (1 << 2)) != 0) t_pixels[5] = 0xFF;
			case 2:	if ((b & (1 << 3)) != 0) t_pixels[5] = 0xFF;
			case 3: if ((b & (1 << 4)) != 0) t_pixels[3] = 0xFF;
			case 4: if ((b & (1 << 5)) != 0) t_pixels[2] = 0xFF;
			case 5: if ((b & (1 << 6)) != 0) t_pixels[1] = 0xFF;
			case 6: if ((b & (1 << 7)) != 0) t_pixels[0] = 0xFF;
			default:
				break;
			}
		}
	}

	setflags(FLAG_MASK_CHANGED, false);
}

// GDI Implementation

#define CAPSTYLE_IS_BUTT(a)			(((a) & CapMask) == CapButt)
#define CAPSTYLE_IS_ROUND(a)		(((a) & CapMask) == CapRound)
#define CAPSTYLE_IS_PROJECTING(a)	(((a) & CapMask) == CapProjecting)
#define CAPSTYLE_TO_GDI_FLAGS(a)	(CAPSTYLE_IS_ROUND(a) ? PS_ENDCAP_ROUND : (CAPSTYLE_IS_PROJECTING(a) ? PS_ENDCAP_SQUARE : PS_ENDCAP_FLAT))

#define JOINSTYLE_IS_ROUND(a)		((a) == JoinRound)
#define JOINSTYLE_IS_MITER(a)		((a) == JoinMiter)
#define JOINSTYLE_IS_BEVEL(a)		((a) == JoinBevel)
#define JOINSTYLE_TO_GDI_FLAGS(a)	(JOINSTYLE_IS_ROUND(a) ? PS_JOIN_ROUND : (JOINSTYLE_IS_MITER(a) ? PS_JOIN_MITER : PS_JOIN_BEVEL))

void MCGDIContext::gdi_synchronize_pen(void)
{
	assert(getflags(FLAG_STROKE_CHANGED));

	if (f_current_pen != NULL)
		DeleteObject(f_current_pen);

	if (f_current_mask_pen != NULL)
		DeleteObject(f_current_mask_pen);

	// MW-2006-03-20: Bug 3397 - if width is 1 and no special requirements, use a cosmetic pen (much faster!)
	if (f_stroke . width == 0 || (f_stroke . width == 1 && f_stroke . style == LineSolid && f_stroke . cap == 0 && f_stroke . join == 0))
	{
		int t_style;
		LOGBRUSH t_brush;

		t_style = PS_COSMETIC;
		if (f_stroke . style != LineSolid && f_stroke . dash . length > 0)
			t_style |= (f_stroke . dash . data[0] == 1 ? PS_ALTERNATE : PS_DOT);
		else
			t_style |= PS_SOLID;

		t_brush . lbStyle = BS_SOLID;
		t_brush . lbHatch = 0;
		t_brush . lbColor = f_fill . colour;

		f_current_pen = ExtCreatePen(t_style, 1, &t_brush, 0, NULL);
		if (getflags(FLAG_MASK_ACTIVE))
		{
			t_brush . lbColor = 0xffffff;
			f_current_mask_pen = ExtCreatePen(t_style, 1, &t_brush, 0, NULL);
		}
	}
	else
	{
		uint4 t_style;
		uint4 t_width;
		LOGBRUSH t_brush;
		uint4 t_dash_length;
		uint4 *t_dashes;

		t_style = JOINSTYLE_TO_GDI_FLAGS(f_stroke . join) | CAPSTYLE_TO_GDI_FLAGS(f_stroke . cap) | PS_GEOMETRIC;
		t_width = f_stroke . width;

		if (f_fill . style != FillTiled)
		{
			t_brush . lbStyle = BS_SOLID;
			t_brush . lbHatch = 0;
		}
		else
		{
			t_brush . lbStyle = BS_PATTERN;
			t_brush . lbHatch = (LONG)f_fill . pattern -> handle . pixmap;
		}

		if (f_stroke . style != LineSolid && f_stroke . dash . length != 0)
		{
			t_style |= PS_USERSTYLE;
			t_dash_length = f_stroke . dash . length;
			t_dashes = f_stroke . dash . data;
		}
		else
		{
			t_dash_length = 0;
			t_dashes = NULL;
		}

		t_brush . lbColor = f_fill . colour;

		f_current_pen = ExtCreatePen(t_style, t_width, &t_brush, t_dash_length, (DWORD *)t_dashes);
		if (getflags(FLAG_MASK_ACTIVE))
		{
			t_brush . lbColor = 0xffffff;
			f_current_mask_pen = ExtCreatePen(t_style, t_width, &t_brush, t_dash_length, (DWORD *)t_dashes);
		}
	}

	if (getflags(FLAG_BACKGROUND_CHANGED))
	{
		SetBkColor(f_destination_dc, f_background);
		setflags(FLAG_BACKGROUND_CHANGED, false);
	}

	setflags(FLAG_STROKE_CHANGED, false);
}

void MCGDIContext::gdi_synchronize_brush(void)
{
	assert(getflags(FLAG_FILL_CHANGED));

	if (f_current_brush != NULL)
		DeleteObject(f_current_brush);

#pragma message("Implement proper stippling brush")
	if (f_fill . style == FillSolid)
		f_current_brush = CreateSolidBrush(f_fill . colour);
	else if (f_fill . style == FillTiled)
		f_current_brush = CreatePatternBrush((HBITMAP)f_fill . pattern -> handle . pixmap);
	else
	{
		static HBITMAP s_stipple = NULL;
		static uint4 s_stipple_data[8] =
    {
        0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
        0x55555555, 0xAAAAAAAA, 0x55555555,
        0xAAAAAAAA, 0x55555555
    };
		if (s_stipple == NULL)
			s_stipple = CreateBitmap(32, 8, 1, 1, s_stipple_data);
		f_current_brush = CreatePatternBrush(s_stipple);
	}
		
	setflags(FLAG_FILL_CHANGED, false);
}

void MCGDIContext::gdi_stroke_begin(void)
{
	assert(!getflags(FLAG_STROKE_READY | FLAG_FILL_READY));

	if (getflags(FLAG_STROKE_CHANGED))
		gdi_synchronize_pen();

	if (f_current_pen != NULL)
	{
		SelectObject(f_destination_dc, f_current_pen);
		SetBkMode(f_destination_dc, f_stroke . style == LineOnOffDash ? TRANSPARENT : OPAQUE);
		if (getflags(FLAG_MASK_ACTIVE))
		{
			SelectObject(f_mask_dc, f_current_mask_pen);
			SetBkMode(f_mask_dc, f_stroke . style == LineOnOffDash ? TRANSPARENT : OPAQUE);
		}
	}

	setflags(FLAG_STROKE_READY, true);
}

void MCGDIContext::gdi_stroke_end(void)
{
	assert(getflags(FLAG_STROKE_READY));

	SelectObject(f_destination_dc, GetStockObject(NULL_PEN));
	if (getflags(FLAG_MASK_ACTIVE))
		SelectObject(f_mask_dc, GetStockObject(NULL_PEN));

	setflags(FLAG_STROKE_READY, false);
}

void MCGDIContext::gdi_fill_begin(void)
{
	assert(!getflags(FLAG_STROKE_READY | FLAG_FILL_READY));

	if (getflags(FLAG_FILL_CHANGED))
		gdi_synchronize_brush();

	if (f_current_brush != NULL)
	{
		if (f_fill . style == FillSolid)
			;
		else if (f_fill . style == FillTiled)
		{
			if (MCmajorosversion >= 0x0500)
			{
				SetBrushOrgEx(f_destination_dc, 0, 0, NULL);
				SetBrushOrgEx(f_destination_dc, f_fill . origin . x - f_origin . x, f_fill . origin . y - f_origin . y, NULL);
			}
			else
			{
				UnrealizeObject(f_current_brush);
				SetBrushOrgEx(f_destination_dc, f_fill . origin . x - f_origin . x, f_fill . origin . y - f_origin . y, NULL);
			}
		}
		else if (f_fill . style == FillStippled)
		{
			SetTextColor(f_destination_dc, f_fill . colour);
			SetBkColor(f_destination_dc, getgray() . pixel);
		}
		else if (f_fill . style == FillOpaqueStippled)
		{
			SetTextColor(f_destination_dc, f_fill . colour);
			SetBkColor(f_destination_dc, f_background);
		}

		SelectObject(f_destination_dc, f_current_brush);

		if (getflags(FLAG_MASK_ACTIVE))
			SelectObject(f_mask_dc, GetStockObject(WHITE_BRUSH));
	}

	setflags(FLAG_FILL_READY, true);
}

void MCGDIContext::gdi_fill_end(void)
{
	assert(getflags(FLAG_FILL_READY));

	SelectObject(f_destination_dc, GetStockObject(NULL_BRUSH));
	if (getflags(FLAG_MASK_ACTIVE))
		SelectObject(f_mask_dc, GetStockObject(NULL_BRUSH));

	setflags(FLAG_FILL_READY, false);
}

void MCGDIContext::gdi_polyline(POINT *p_points, uint4 p_count)
{
	if (f_stroke . width == 0)
		p_count += 1;

	gdi_stroke_begin();
	Polyline(f_destination_dc, p_points, p_count);
	if (getflags(FLAG_MASK_ACTIVE))
	{
		Polyline(f_mask_dc, p_points, p_count);
		setflags(FLAG_MASK_CHANGED, true);
	}
	gdi_stroke_end();
}

void MCGDIContext::gdi_polylines(POINT *p_points, uint4 *p_counts, uint4 p_count)
{
	gdi_stroke_begin();
	PolyPolyline(f_destination_dc, p_points, (DWORD *)p_counts, p_count);
	if (getflags(FLAG_MASK_ACTIVE))
	{
		PolyPolyline(f_mask_dc, p_points, (DWORD *)p_counts, p_count);
		setflags(FLAG_MASK_CHANGED, true);
	}
	gdi_stroke_end();
}

void MCGDIContext::gdi_polygon(POINT *p_points, uint4 p_count)
{
	gdi_fill_begin();
	Polygon(f_destination_dc, p_points, p_count);
	if (getflags(FLAG_MASK_ACTIVE))
	{
		Polygon(f_mask_dc, p_points, p_count);
		setflags(FLAG_MASK_CHANGED, true);
	}
	gdi_fill_end();
}

void MCGDIContext::gdi_rectangle(int4 p_left, int4 p_top, int4 p_right, int4 p_bottom)
{
	assert(getflags(FLAG_STROKE_READY | FLAG_FILL_READY));

	Rectangle(f_destination_dc, p_left, p_top, p_right, p_bottom);
	if (getflags(FLAG_MASK_ACTIVE))
	{
		Rectangle(f_mask_dc, p_left, p_top, p_right, p_bottom);
		setflags(FLAG_MASK_CHANGED, true);
	}
}

void MCGDIContext::gdi_round_rectangle(int4 p_left, int4 p_top, int4 p_right, int4 p_bottom, int4 p_radius)
{
	assert(getflags(FLAG_STROKE_READY | FLAG_FILL_READY));

	RoundRect(f_destination_dc, p_left, p_top, p_right, p_bottom, p_radius, p_radius);
	if (getflags(FLAG_MASK_ACTIVE))
	{
		RoundRect(f_mask_dc, p_left, p_top, p_right, p_bottom, p_radius, p_radius);
		setflags(FLAG_MASK_CHANGED, true);
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

void MCGDIContext::gdi_arc(int4 p_left, int4 p_top, int4 p_right, int4 p_bottom, int4 p_start, int4 p_end)
{
	assert(getflags(FLAG_STROKE_READY | FLAG_FILL_READY));

	gdi_do_arc(f_destination_dc, getflags(FLAG_MASK_ACTIVE) ? f_mask_dc : NULL, getflags(FLAG_FILL_READY), p_left, p_top, p_right, p_bottom, p_start, p_end);
	if (getflags(FLAG_MASK_ACTIVE))
		setflags(FLAG_MASK_CHANGED, true);
}

void MCGDIContext::gdi_text(int4 p_x, int4 p_y, const char *p_string, uint4 p_length, bool p_opaque, bool p_unicode_override)
{
	// GDI turns on and off anti-aliasing depending on whether it is rendering
	// into a suitable bitmap or not... This is somewhat unfortunate because it
	// means that the image produced in the case of non-'opaque' text rendered
	// into a 'masked' context is wrong. Thus we treat the latter case specially.
	bool t_unicode;
	t_unicode = p_unicode_override || f_font -> unicode;

	bool t_masked;
	t_masked = getflags(FLAG_MASK_ACTIVE);

	if (getflags(FLAG_FONT_CHANGED))
	{
		SelectObject(f_destination_dc, f_font -> fid);
		if (t_masked)
			SelectObject(f_mask_dc, f_font -> fid);

		setflags(FLAG_FONT_CHANGED, false);
	}

	char *t_buffer;
	if (t_unicode && ((long)p_string & 1))
	{
		t_buffer = new char[p_length];
		memcpy(t_buffer, p_string, p_length);
	}
	else
		t_buffer = (char *)p_string;

	// MW-2009-06-14: If we are rendering into a layer that has been marked as opaque
	//   then we don't need to capture alpha.
	if (getflags(FLAG_IS_OPAQUE) || p_opaque || !t_masked)
	{
		// This is the 'common' case - we want text to be rendered including a
		// background into a context which doesn't need the mask.

		// Of course, this case is still used in the case of masked but opaque
		// renderings which doesn't work right as the mask of the text rendered
		// into the mask DC doesn't get antialiased. So, in this case, we compute
		// a rough bounding rect, clip it and render that to the mask.

		if (getflags(FLAG_FOREGROUND_CHANGED))
		{
			SetTextColor(f_destination_dc, f_fill . colour);
			setflags(FLAG_FOREGROUND_CHANGED, false);
		}

		if (getflags(FLAG_BACKGROUND_CHANGED))
		{
			SetBkColor(f_destination_dc, f_background);
			setflags(FLAG_BACKGROUND_CHANGED, false);
		}

		SetBkMode(f_destination_dc, p_opaque ? OPAQUE : TRANSPARENT);

		if (t_unicode)
			TextOutW(f_destination_dc, p_x, p_y, (LPCWSTR)t_buffer, p_length >> 1);
		else
			TextOutA(f_destination_dc, p_x, p_y, t_buffer, p_length);

		// MW-2011-02-07: [[ Bug 9381 ]] Rejig of masked text rendering.
		if (t_masked)
		{
			SIZE t_size;
			if (t_unicode)
				GetTextExtentPoint32W(f_destination_dc, (LPCWSTR)t_buffer, p_length >> 1, &t_size);
			else
				GetTextExtentPoint32A(f_destination_dc, t_buffer, p_length, &t_size);
			
			TEXTMETRICA t_metrics;
			GetTextMetricsA(f_destination_dc, &t_metrics);

			MCRectangle t_bounds;
			t_bounds . x = (int2)(p_x - t_size . cy);
			t_bounds . y = (int2)(p_y - t_metrics . tmAscent);
			t_bounds . height = (uint2)t_size . cy;
			t_bounds . width = (uint2)(t_size . cx + t_size . cy * 2);
			t_bounds = MCU_intersect_rect(t_bounds, f_clip);
			
			RECT t_bounds_r;
			SetRect(&t_bounds_r,  t_bounds . x, t_bounds . y, t_bounds . x + t_bounds . width, t_bounds . y + t_bounds . height);
			FillRect(f_mask_dc, &t_bounds_r, (HBRUSH)GetStockObject(WHITE_BRUSH));

			setflags(FLAG_MASK_CHANGED, true);
		}
	}
	else
	{
		// This is the 'evil' case - the operation now becomes an 'alpha'
		// compositing operation. We first copy out the 'affected' region of
		// the destination into a temporary block of memory, and then render
		// white-on-black text into the destination. We then re-composite in
		// the original contents.

		// Note that like our alpha image rendering, we must flush the mask
		// first as we are dealing with the alpha-channel directly.
		if (getflags(FLAG_MASK_CHANGED))
			flush_mask();

		// Next we need to know how big a temporary store we need, so we measure
		// the text and compute a 'rough' bounding box (note that it doesn't
		// matter if we process a few too many pixels - but it *does* matter if
		// we process too few!).
		SIZE t_size;
		if (t_unicode)
			GetTextExtentPoint32W(f_destination_dc, (LPCWSTR)t_buffer, p_length >> 1, &t_size);
		else
			GetTextExtentPoint32A(f_destination_dc, t_buffer, p_length, &t_size);

		// For now we fudge the rectangle we consider. To do this properly would
		// require that we analyze the first and last char of the string, compute
		// their ABC widths and adjust the bounding box appropriately. Additionally
		// this is done differently whether the font is a TrueType font or not.

		// Our fudge will be to consider the 'height' of character extra space either
		// side - this should be enough for all reasonable fonts. The logic here is that
		// the overhang either side is some percentage of the size of the font...

		// We fetch the actual text metrics as the ascent value in the FontStruct is
		// already fudged...
		TEXTMETRICA t_metrics;
		GetTextMetricsA(f_destination_dc, &t_metrics);

		// Fudge the bounds...
		MCRectangle t_bounds;
		t_bounds . x = (int2)(p_x - t_size . cy);
		t_bounds . y = (int2)(p_y - t_metrics . tmAscent);
		t_bounds . height = (uint2)t_size . cy;
		t_bounds . width = (uint2)(t_size . cx + t_size . cy * 2);

		// Make sure our bounds are no bigger than the clip rect.
		t_bounds = MCU_intersect_rect(t_bounds, f_clip);

		// Allocate a temporary buffer to hold the original pixels.
		// [ Note that the 'panic' case is no-op :o) ]
		uint4 *t_tmp_bits;
		t_tmp_bits = new uint4[t_bounds . width * t_bounds . height];
		if (t_tmp_bits != NULL)
		{
			uint4 *t_dst_bits;
			t_dst_bits = (uint4 *)f_destination_bits + (t_bounds . y - f_origin . y) * f_width + (t_bounds . x - f_origin . x);

			// MW-2009-06-14: It seems that the output we get when rendering
			//   white on black is not exactly what windows blends - it has
			//   undergone some sort of correction/filtering. Indeed, the range
			//   of 'alpha' values we get is something like 104-255 for non-
			//   ClearType text. This needs a little more research before
			//   we can remap this. For now, however, significantly better
			//   results can be gained by rendering black on white or white on
			//   black depending on the luminosity of the source color.
			bool t_nearest_white;
			t_nearest_white = (0.3 * (f_fill . colour & 0xff) + 0.59 * ((f_fill . colour >> 8) & 0xff) + 0.11 * ((f_fill . colour >> 16) & 0xff)) >= 127.0;

			uint32_t t_foreground, t_background;
			t_foreground = t_nearest_white ? 0xffffff : 0x000000;
			t_background = t_nearest_white ? 0x000000 : 0xffffff;

			// Set the destination dc to do black text on white or white on black
			// depending on the source color.
			SetTextColor(f_destination_dc, t_foreground);
			SetBkColor(f_destination_dc, t_background);
			SetBkMode(f_destination_dc, OPAQUE);

			// Copy the current destination contents in the area of interest,
			// and set the destination to transparent black. Doing the latter
			// means that our 'fudge' doesn't cause any problems since the
			// extra space is just transparent.
			for(int4 y = 0; y < t_bounds . height; y++)
			{
				memcpy(t_tmp_bits + y * t_bounds . width, t_dst_bits + y * f_width, t_bounds . width * 4);
				memset(t_dst_bits + y * f_width, t_background & 0xff, t_bounds . width * 4);
			}

			// Render the text.
			if (t_unicode)
				TextOutW(f_destination_dc, p_x, p_y, (LPCWSTR)t_buffer, p_length >> 1);
			else
				TextOutA(f_destination_dc, p_x, p_y, t_buffer, p_length);

			// Ensure it has happened.
			GdiFlush();

			// We now have an alpha mask for the text in the 't_bounds' rectangle.
			// Due to cleartypeness, what we now have is a per-color channel alpha
			// mask - i.e. three alpha values per pixel!

			for(int4 y = 0; y < t_bounds . height; y++)
			{
				for(int4 x = 0; x < t_bounds . width; x++)
				{
					uint4 *t_dst;
					t_dst = t_dst_bits + y * f_width + x;

					uint4 *t_src;
					t_src = t_tmp_bits + y * t_bounds . width + x;

					uint4 t_mask;
					t_mask = *t_dst;

					// For now we just average the alpha value - when we have a bit
					// more time for testing, we need to manufacture a non-ClearType
					// using HFONT to use for the rendering.
					uint1 t_alpha;
					t_alpha = ((t_mask & 0xFF) + ((t_mask & 0xFF00) >> 8) + ((t_mask & 0xFF0000) >> 16)) / 3;
					
					// This mapping function can be enabled after we have done more
					// research on possible output ramps.
					// t_alpha = t_alpha != 0 ? (t_alpha - 95) * 255 / (255 - 95) : t_alpha;

					if (!t_nearest_white)
						t_alpha = 255 - t_alpha;

					// Do a simple composite of the original src with the foreground
					// (text) color.
					*t_dst = packed_bilinear_bounded(*t_src, 255 - t_alpha, 0xff000000 | ((f_fill . colour & 0xff) << 16) | (f_fill . colour & 0xff00) | ((f_fill . colour & 0xff0000) >> 16), t_alpha);
				}
			}
			
			// We reset the foreground and background colors of the field so
			// make sure we force a re-change when they are next used.
			setflags(FLAG_FOREGROUND_CHANGED | FLAG_BACKGROUND_CHANGED, true);

			// Get rid of our temporary array.
			delete t_tmp_bits;
		}
	}

	if (p_string != t_buffer)
		delete t_buffer;
}

void MCGDIContext::gdi_image(Pixmap mask, Pixmap data, int2 sx, int2 sy, uint2 sw, uint2 sh,
														 int2 dx, int2 dy)
{
	assert(f_function <= LAST_NATIVE_FUNCTION);

	if (data != NULL)
	{
		HDC t_src_dc;
		t_src_dc = ((MCScreenDC *)MCscreen) -> getsrchdc();
		
		HGDIOBJ t_old_src;
		t_old_src = SelectObject(t_src_dc, data -> handle . pixmap);

		if (mask == NULL)
		{
			BitBlt(f_destination_dc, dx, dy, sw, sh, t_src_dc, sx, sy, s_image_mode_map[f_function]);
			if (getflags(FLAG_MASK_ACTIVE))
			{
				HGDIOBJ t_old_brush;
				t_old_brush = SelectObject(f_mask_dc, GetStockObject(WHITE_BRUSH));
				Rectangle(f_mask_dc, dx, dy, dx + sw + 1, dy + sh + 1);
				SelectObject(f_mask_dc, t_old_brush);
				setflags(FLAG_MASK_CHANGED, true);
			}
		}
		else
		{
			MaskBlt(f_destination_dc, dx, dy, sw, sh, t_src_dc, sx, sy, (HBITMAP)mask -> handle . pixmap, sx, sy, MAKEROP4(s_image_mode_map[GXnoop], s_image_mode_map[f_function]));
			if (getflags(FLAG_MASK_ACTIVE))
			{
				SelectObject(t_src_dc, mask -> handle . pixmap);

				BITMAP t_bitmap;
				GetObjectA(mask -> handle . pixmap, sizeof(BITMAP), &t_bitmap);

				BitBlt(f_mask_dc, dx, dy, sw, sh, t_src_dc, sx, sy, s_image_mode_map[GXor]);
				setflags(FLAG_MASK_CHANGED, true);
			}
		}

		SelectObject(t_src_dc, t_old_src);
	}
}

void gdi_blend(HDC p_dst, int p_dx, int p_dy, int p_dw, int p_dh, HDC p_src, int p_sx, int p_sy, int p_sw, int p_sh, int p_opacity)
{
	BLENDFUNCTION t_blend;
	t_blend . BlendOp = AC_SRC_OVER;
	t_blend . BlendFlags = 0;
	t_blend . AlphaFormat = AC_SRC_ALPHA;
	t_blend . SourceConstantAlpha = p_opacity;
	BOOL t_result = AlphaBlend(p_dst, p_dx, p_dy, p_dw, p_dh, p_src, p_sx, p_sy, p_sw, p_sh, t_blend);
}

//-----------------------------------------------------------------------------
//  Fill Combiners
//

#ifdef _MSC_VER
#define INLINE __forceinline
#else
#define INLINE inline
#endif

static void solid_combiner_begin(MCCombiner *_self, int4 y)
{
	MCSolidCombiner *self = (MCSolidCombiner *)_self;
	self -> bits += y * self -> stride;
}

static void solid_combiner_advance(MCCombiner *_self, int4 dy)
{
	MCSolidCombiner *self = (MCSolidCombiner *)_self;
	self -> bits += dy * self -> stride;
}

static void solid_combiner_blend(MCCombiner *_self, int4 fx, int4 tx, uint1 alpha)
{
	MCSolidCombiner *self = (MCSolidCombiner *)_self;
	uint4 *d;
	uint4 s;

	d = self -> bits;

	if (alpha == 255)
	{
		s = self -> pixel;
		for(; fx < tx; ++fx)
			d[fx] = s;
	}
	else if (alpha != 0)
	{
		s = packed_scale_bounded(self -> pixel, alpha);
		for(; fx < tx; ++fx)
			d[fx] = packed_scale_bounded(d[fx], 255 - alpha) + s;
	}
}

static void solid_combiner_combine(MCCombiner *_self, int4 fx, int4 tx, uint1 *mask)
{
	MCSolidCombiner *self = (MCSolidCombiner *)_self;
	uint4 *d;

	d = self -> bits;

	for(; fx < tx; ++fx)
	{
		uint1 alpha;
		alpha = *mask++;
		d[fx] = packed_bilinear_bounded(d[fx], 255 - alpha, self -> pixel, alpha);
	}
}

static void solid_combiner_end(MCCombiner *_self)
{
}

static void pattern_combiner_begin(MCCombiner *_self, int4 y)
{
	MCPatternCombiner *self = (MCPatternCombiner *)_self;
	self -> pattern_offset = ((y - self -> origin_y) % self -> height) * self -> pattern_stride;
	self -> bits += y * self -> stride;
}

static void pattern_combiner_advance(MCCombiner *_self, int4 dy)
{
	MCPatternCombiner *self = (MCPatternCombiner *)_self;
	self -> pattern_offset += dy * self -> pattern_stride;
	self -> pattern_offset %= self -> height * self -> pattern_stride;
	self -> bits += dy * self -> stride;
}

static void pattern_combiner_blend(MCCombiner *_self, int4 fx, int4 tx, uint1 alpha)
{
	MCPatternCombiner *self = (MCPatternCombiner *)_self;
	uint4 x, w;
	uint4 *s, *d;

	d = self -> bits;
	s = self -> pattern_bits + self -> pattern_offset;

	w = self -> width;
	x = (fx - self -> origin_x) % w;
	
	for(; fx < tx; ++fx)
	{
		d[fx] = packed_bilinear_bounded(s[x] | 0xFF000000, alpha, d[fx], 255 - alpha);
		x++;
		if (x == w)
			x = 0;
	}
}

static void pattern_combiner_end(MCCombiner *_self)
{
	MCPatternCombiner *self = (MCPatternCombiner *)_self;
}

MCCombiner *MCGDIContext::combiner_lock(void)
{
	static bool s_solid_combiner_initialised = false;
	static MCSolidCombiner s_solid_combiner;
	static bool s_pattern_combiner_initialised = false;
	static MCPatternCombiner s_pattern_combiner;

	if (getflags(FLAG_MASK_CHANGED))
		flush_mask();

	MCSurfaceCombiner *t_combiner;
	t_combiner = NULL;
	if (f_gradient_fill != NULL && f_gradient_fill->kind != kMCGradientKindNone && f_gradient_fill->ramp_length > 0)
	{
		t_combiner = MCGradientFillCreateCombiner(f_gradient_fill, f_clip);
	}
	else if (f_fill . style == FillSolid)
	{
		if (!s_solid_combiner_initialised)
		{
			s_solid_combiner . begin = solid_combiner_begin;
			s_solid_combiner . advance = solid_combiner_advance;
			s_solid_combiner . blend = solid_combiner_blend;
			s_solid_combiner . end = solid_combiner_end;
			s_solid_combiner . combine = solid_combiner_combine;
			s_solid_combiner_initialised = true;
		}

		s_solid_combiner . pixel = 0xff000000 | ((f_fill . colour & 0xff) << 16) | (f_fill . colour & 0xff00) | ((f_fill . colour & 0xff0000) >> 16);
		t_combiner = &s_solid_combiner;
	}
	else if (f_fill . style == FillTiled)
	{
		if (!s_pattern_combiner_initialised)
		{
			s_pattern_combiner . begin = pattern_combiner_begin;
			s_pattern_combiner . advance = pattern_combiner_advance;
			s_pattern_combiner . blend = pattern_combiner_blend;
			s_pattern_combiner . end = pattern_combiner_end;
			s_pattern_combiner . combine = NULL;
			s_pattern_combiner_initialised = true;
		}

		BITMAP t_pattern;
		GetObjectA(f_fill . pattern -> handle . pixmap, sizeof(BITMAP), &t_pattern);

		s_pattern_combiner . pattern_bits = (uint4 *)t_pattern . bmBits;
		s_pattern_combiner . pattern_stride = t_pattern . bmWidth;
		s_pattern_combiner . width = t_pattern . bmWidth;
		s_pattern_combiner . height = t_pattern . bmHeight;
		s_pattern_combiner . origin_x = f_fill . origin . x;
		s_pattern_combiner . origin_y = f_fill . origin . y;

		t_combiner = &s_pattern_combiner;
	}

	if (t_combiner != NULL)
	{
		t_combiner -> bits = (uint4 *)f_destination_bits - f_origin . y * f_width - f_origin . x;
		t_combiner -> stride = f_width;
	}

	return t_combiner;
}

void MCGDIContext::combiner_unlock(MCCombiner *p_combiner)
{
}
