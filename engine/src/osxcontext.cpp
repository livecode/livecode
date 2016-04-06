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

#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"

#include "globals.h"
#include "mctheme.h"
#include "osxcontext.h"
#include "path.h"
#include "paint.h"
#include "bitmapeffect.h"
#include "region.h"

#include "osxdc.h"

void surface_merge(void *p_pixels, uint4 p_pixel_stride, void *p_src, uint4 p_src_stride, uint4 p_width, uint4 p_height);
void surface_merge_with_mask(void *p_pixels, uint4 p_pixel_stride, void *p_src, uint4 p_src_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);
void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_src, uint4 p_src_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
void surface_merge_with_alpha_and_mask(void *p_pixels, uint4 p_pixel_stride, void *p_src, uint4 p_src_stride, void *p_alpha, uint4 p_alpha_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);

void surface_merge_with_alpha(void *p_src, uint4 p_src_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
void surface_extract_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
void surface_unmerge_pre(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height);

extern void surface_combine_blendSrcOver_masked(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity);
extern void surface_combine_blendSrcOver_solid(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity);

typedef void (*surface_combiner_t)(void *p_dst, int32_t p_dst_stride, const void *p_src, uint4 p_src_stride, uint4 p_width, uint4 p_height, uint1 p_opacity);
extern surface_combiner_t s_surface_combiners[];
extern surface_combiner_t s_surface_combiners_nda[];
uint4 g_current_background_colour = 0x000000;

static uint1 s_dash_pattern[] =
{
	0x11, 0x22, 0x44, 0x88,
	0x11, 0x22, 0x44, 0x88,
	0x11, 0x22, 0x44, 0x88,
	0x11, 0x22, 0x44, 0x88
};

Pattern g_stipple_pattern = { 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22 };

CGrafPtr MCQuickDrawContext::s_current_port = NULL;

inline void MCRect2MacRect(const MCRectangle& p_rect, Rect& r_rect)
{
	r_rect . left = p_rect . x;
	r_rect . top = p_rect . y;
	r_rect . right = p_rect . x + p_rect . width;
	r_rect . bottom = p_rect . y + p_rect . height;
}


// MW-2007-03-15: [[ Bug 4492 ]] - srcOr not correct on OS X, so disabling use of native inks for now
#if 1
static uint1 s_is_native_function[] =
{
  0, /* GXclear */
	0, /* GXand */
	0, /* GXandReverse */
	1, /* GXcopy */
	0, /* GXandInverted */
	0, /* GXnoop */
	1, /* GXxor */
	0, /* GXor */
	0, /* GXnor */
	0, /* GXequiv */
	0, /* GXinvert */
	0, /* GXorReverse */
	0, /* GXcopyInverted */
	0, /* GXorInverted */
	0, /* GXnand */
	0, /* GXset */
	0, /* GXsrcBic */
	0, /* GXnotSrcBic */
	1, /* GXblend */
	1, /* GXaddPin */
	1, /* GXaddOver */
	1, /* GXsubPin */
	1, /* GXtransparent */
	1, /* GXaddMax */
	1, /* GXsubOver */
	1, /* GXaddMin */
};
#else
static uint1 s_is_native_function[] =
{
  0, /* GXclear */
	1, /* GXand */
	0, /* GXandReverse */
	1, /* GXcopy */
	1, /* GXandInverted */
	0, /* GXnoop */
	1, /* GXxor */
	1, /* GXor */
	0, /* GXnor */
	1, /* GXequiv */
	0, /* GXinvert */
	0, /* GXorReverse */
	1, /* GXcopyInverted */
	1, /* GXorInverted */
	0, /* GXnand */
	0, /* GXset */
	1, /* GXsrcBic */
	1, /* GXnotSrcBic */
	1, /* GXblend */
	1, /* GXaddPin */
	1, /* GXaddOver */
	1, /* GXsubPin */
	1, /* GXtransparent */
	1, /* GXaddMax */
	1, /* GXsubOver */
	1, /* GXaddMin */
};
#endif

static uint4 s_native_function_map[] =
{
  0,					/* GXclear */
	notSrcBic,	/* GXand */
	0,					/* GXandReverse */
	srcCopy,		/* GXcopy */
	srcBic,			/* GXandInverted */
	0,					/* GXnoop */
	srcXor,			/* GXxor */
	srcOr,			/* GXor */
	0,					/* GXnor */
	notSrcXor,	/* GXequiv */
	0,					/* GXinvert */
	0,					/* GXorReverse */
	notSrcCopy,	/* GXcopyInverted */
	srcOr,			/* GXorInverted */
	0,					/* GXnand */
	0,          /* GXset */
	srcBic,			/* GXsrcBic */
	notSrcBic,	/* GXnotSrcBic */
	blend,			/* GXblend */
	addPin,			/* GXaddPin */
	addOver,		/* GXaddOver */
	subPin,			/* GXsubPin */
	transparent,/* GXtransparent */
	addMax,			/* GXaddMax */
	subOver,		/* GXsubOver */
	adMin,			/* GXaddMin */
};

#define LAST_NATIVE_FUNCTION 0x19

MCQuickDrawContext::MCQuickDrawContext(void)
{
	memset(((uint1 *)this) + sizeof(MCContext), 0, sizeof(MCQuickDrawContext) - sizeof(MCContext));
	f_fill . colour = MCscreen -> black_pixel;
	f_background = MCscreen -> white_pixel;
}

MCQuickDrawContext::~MCQuickDrawContext(void)
{
	if (f_external_alpha != NULL)
	{
		void *t_dst_ptr;
		uint4 t_dst_stride;
		layer_lock(f_layers, t_dst_ptr, t_dst_stride);
		surface_extract_alpha(t_dst_ptr, t_dst_stride, f_external_alpha -> data, f_external_alpha -> bytes_per_line, f_layers -> width, f_layers -> height);
		surface_unmerge_pre(t_dst_ptr, t_dst_stride, f_layers -> width, f_layers -> height);
		layer_unlock(f_layers, t_dst_ptr, t_dst_stride);
	}

	while(f_layers != NULL)
		f_layers = layer_destroy(f_layers);
	
	if (f_stroke . dash . data != NULL)
		delete[] f_stroke . dash . data;

	if (f_pattern_port != NULL)
	{
		UnlockPixels((*f_pattern) -> patMap);
		DisposeGWorld(f_pattern_port);
		(*f_pattern) -> patMap = f_old_pattern;
		(*f_pattern) -> patData = f_old_pattern_data;
	}
	
	if (f_pattern != NULL)
		DisposePixPat(f_pattern);
}

MCContextType MCQuickDrawContext::gettype(void) const
{
	if (getflags(FLAG_IS_PRINTER))
		return CONTEXT_TYPE_PRINTER;
	if (getflags(FLAG_IS_TRANSIENT))
		return CONTEXT_TYPE_OFFSCREEN;
	return CONTEXT_TYPE_SCREEN;
}

bool MCQuickDrawContext::changeopaque(bool p_new_value)
{
	bool t_old_value;
	t_old_value = getflags(FLAG_IS_OPAQUE);
	setflags(FLAG_IS_OPAQUE, p_new_value);
	return t_old_value;
}

void MCQuickDrawContext::setprintmode(void)
{
	f_layers -> flags |= FLAG_IS_PRINTER;
}

MCQuickDrawContext::Layer *MCQuickDrawContext::layer_create_with_port(CGrafPtr p_port, bool p_transient, bool p_needs_alpha)
{
	Layer *t_layer;
	t_layer = new Layer;
	if (t_layer == NULL)
		return NULL;
	
	memset(t_layer, 0, sizeof(Layer));
	
	Rect t_bounds;
	GetPortBounds(p_port, &t_bounds);
	
	t_layer -> flags = FLAG_FILL_CHANGED | FLAG_STROKE_CHANGED | FLAG_FUNCTION_CHANGED | FLAG_FONT_CHANGED | FLAG_BACKGROUND_CHANGED | FLAG_FOREGROUND_CHANGED | FLAG_CLIP_CHANGED | FLAG_ORIGIN_CHANGED;
	if (p_transient)
		t_layer -> flags |= FLAG_IS_TRANSIENT;
	if (p_needs_alpha)
		t_layer -> flags |= FLAG_IS_ALPHA;
	if (IsPortOffscreen(p_port))
		t_layer -> flags |= FLAG_IS_OFFSCREEN;
		
	t_layer -> scroll = t_bounds . top;
	t_layer -> origin . x = t_bounds . left;
	t_layer -> origin . y = t_bounds . top;
	t_layer -> width = t_bounds . right - t_bounds . left;
	t_layer -> height = t_bounds . bottom - t_bounds . top;
	t_layer -> clip . x = t_bounds . left;
	t_layer -> clip . y = t_bounds . top;
	t_layer -> clip . width = t_layer -> width;
	t_layer -> clip . height = t_layer -> height;
	t_layer -> function = GXcopy;
	t_layer -> opacity = 255;
	t_layer -> nesting = 0;
	t_layer -> port = p_port;
	t_layer -> parent = NULL;
	
	if ((t_layer -> flags & FLAG_IS_OFFSCREEN) != 0)
	{
		t_layer -> pixels = GetGWorldPixMap(t_layer -> port);
		LockPixels(t_layer -> pixels);
		HLock((Handle)(t_layer -> pixels));
	}
	else
		t_layer -> pixels = GetPortPixMap(p_port);
	
	return t_layer;
}

MCQuickDrawContext::Layer *MCQuickDrawContext::layer_create_with_parameters(uint4 p_width, uint4 p_height, bool p_transient, bool p_needs_alpha)
{
	Layer *t_layer;
	t_layer = new Layer;
	if (t_layer == NULL)
		return NULL;
	
	memset(t_layer, 0, sizeof(Layer));
	
	t_layer -> parent = NULL;
	t_layer -> flags = FLAG_IS_MINE | FLAG_IS_OFFSCREEN;
	t_layer -> flags |= FLAG_FILL_CHANGED | FLAG_STROKE_CHANGED | FLAG_FUNCTION_CHANGED | FLAG_FONT_CHANGED | FLAG_BACKGROUND_CHANGED | FLAG_FOREGROUND_CHANGED | FLAG_CLIP_CHANGED | FLAG_ORIGIN_CHANGED;
	if (p_transient)
	  t_layer -> flags |= FLAG_IS_TRANSIENT;
	if (p_needs_alpha)
		t_layer -> flags |= FLAG_IS_ALPHA;
	t_layer -> origin . x = 0;
	t_layer -> origin . y = 0;
	t_layer -> clip . x = 0;
	t_layer -> clip . y = 0;
	t_layer -> clip . width = p_width;
	t_layer -> clip . height = p_height;
	t_layer -> width = p_width;
	t_layer -> height = p_height;
	t_layer -> function = GXcopy;
	t_layer -> opacity = 255;
	t_layer -> nesting = 0;
	t_layer -> scroll = 0;
	
	Rect t_bounds;
	t_bounds . left = 0;
	t_bounds . top = 0;
	t_bounds . right = t_layer -> width;
	t_bounds . bottom = t_layer -> height;
	if (NewGWorld(&t_layer -> port, 32, &t_bounds, NULL, NULL, MCmajorosversion >= 0x1040 ? kNativeEndianPixMap : 0) != noErr)
	{
	  delete t_layer;
		return NULL;
	}
	
	t_layer -> pixels = GetGWorldPixMap(t_layer -> port);
	LockPixels(t_layer -> pixels);
	HLock((Handle)(t_layer -> pixels));
		
	memset(GetPixBaseAddr(t_layer -> pixels), 0, GetPixRowBytes(t_layer -> pixels) * p_height);
	
	return t_layer;
}

// MW-2009-06-11: [[ Bitmap Effects ]] Layers may now need to be larger than the parent layer's
//   clip to allow blurs to work correctly - the clip required by the layer is passed as 'new_clip'.
MCQuickDrawContext::Layer *MCQuickDrawContext::layer_create(const MCRectangle& p_new_clip)
{
	assert(f_layers != NULL);
	
	Layer *t_layer;
	t_layer = layer_create_with_parameters(p_new_clip . width, p_new_clip . height, true, true);
		
	if (t_layer != NULL)
	{
		t_layer -> parent = f_layers;
		t_layer -> flags |= FLAG_CLIP_CHANGED | FLAG_ORIGIN_CHANGED;
		if (getflags(FLAG_IS_PRINTER))
			t_layer -> flags |= FLAG_IS_PRINTER;
		t_layer -> origin . x = p_new_clip . x;
		t_layer -> origin . y = p_new_clip . y;
		t_layer -> clip = p_new_clip;
		t_layer -> scroll = 0;
			
		// MW-2009-06-14: [[ Bitmap Effects ]] Layers, by default, have no
		//   effects to apply.
		t_layer -> effects = NULL;
	}
	
	return t_layer;
}

MCQuickDrawContext *MCQuickDrawContext::create_with_port(CGrafPtr p_port, bool p_transient, bool p_needs_alpha)
{
	MCQuickDrawContext *t_context;
	t_context = new MCQuickDrawContext;
	t_context -> f_layers = layer_create_with_port(p_port, p_transient, p_needs_alpha);
	return t_context;
}

MCQuickDrawContext *MCQuickDrawContext::create_with_parameters(uint4 p_width, uint4 p_height, bool p_transient, bool p_needs_alpha)
{
	MCQuickDrawContext *t_context;
	t_context = new MCQuickDrawContext;
	t_context -> f_layers = layer_create_with_parameters(p_width, p_height, p_transient, p_needs_alpha);
	return t_context;
}

void MCQuickDrawContext::layer_lock(Layer *p_layer, void*& r_ptr, uint4& r_stride)
{
  if ((p_layer -> flags & FLAG_IS_OFFSCREEN) == 0)
	{
	  LockPortBits(p_layer -> port);
		p_layer -> pixels = GetPortPixMap(p_layer -> port);
		LockPixels(p_layer -> pixels);
		HLock((Handle)(p_layer -> pixels));

		r_ptr = GetPixBaseAddr(p_layer -> pixels);
		r_stride = GetPixRowBytes(p_layer -> pixels);
	
		Rect t_bounds;
		GetPixBounds(p_layer -> pixels, &t_bounds);
		r_ptr = (uint1 *)r_ptr + r_stride * (p_layer -> scroll - t_bounds . top) + 4 * -t_bounds . left;
	}
	else
	{
		r_ptr = GetPixBaseAddr(p_layer -> pixels);
		r_stride = GetPixRowBytes(p_layer -> pixels);
	}
}

void MCQuickDrawContext::layer_unlock(Layer *p_layer, void *p_ptr, uint4 p_stride)
{
  if ((p_layer -> flags & FLAG_IS_OFFSCREEN) == 0)
	{
		HUnlock((Handle)(p_layer -> pixels));
	  UnlockPixels(p_layer -> pixels);
	  UnlockPortBits(p_layer -> port);
	}
}

MCQuickDrawContext::Layer *MCQuickDrawContext::layer_destroy(Layer *p_layer)
{
  Layer *t_parent;
	t_parent = p_layer -> parent;
	
	if ((p_layer -> flags & FLAG_IS_OFFSCREEN) != 0)
	{
	  HUnlock((Handle)(p_layer -> pixels));
	  UnlockPixels(p_layer -> pixels);

		if (s_current_port != p_layer -> port)
		{
			SetGWorld(p_layer -> port, NULL);
			s_current_port = p_layer -> port;
		}
		
		SetOrigin(0, 0);
		
		Rect t_rect;
		GetPortBounds(p_layer -> port, &t_rect);
		ClipRect(&t_rect);
	}
	else
	{
		if (s_current_port != p_layer -> port)
		{
			SetGWorld(p_layer -> port, GetMainDevice());
			s_current_port = p_layer -> port;
		}
		
		Rect t_rect;
		t_rect . left = t_rect . top = -32768;
		t_rect . right = t_rect . bottom = 32767;
		ClipRect(&t_rect);
	}
	
	if (s_current_port == p_layer -> port)
	{
		CGrafPtr t_port = GetWindowPort(((MCScreenDC *)MCscreen) -> getinvisiblewin());
		
		SetGWorld(t_port, GetMainDevice());
		s_current_port = t_port;
	}
		
	if ((p_layer -> flags & FLAG_IS_MINE) != 0)
	  DisposeGWorld(p_layer -> port);
		
	delete p_layer;
	
	return t_parent;
}


void MCQuickDrawContext::begin(bool p_overlap)
{
// *** OPTIMIZATION - high quality and blendSrcOver or GXcopy are compatible!
	if (!p_overlap && f_layers -> opacity == 255 && f_quality == QUALITY_DEFAULT && f_layers -> function <= LAST_NATIVE_FUNCTION && s_is_native_function[f_layers -> function])
	{
		f_layers -> nesting += 1;
		return;
	}
	
	Layer *t_new_layer;
	t_new_layer = layer_create(f_layers -> clip);
	if (t_new_layer == NULL)
	{
		f_layers -> nesting += 1;
		return;
	}
	f_layers = t_new_layer;
}

// MW-2009-06-11: [[ Bitmap Effects ]] OS X implementation of layer with effects.
bool MCQuickDrawContext::begin_with_effects(MCBitmapEffectsRef p_effects, const MCRectangle& p_shape)
{
	// First compute what region of the shape is required to correctly render
	// the full clip of the current layer.
	MCRectangle t_layer_clip;
	MCBitmapEffectsComputeClip(p_effects, p_shape, getclip(), t_layer_clip);
	
	if (t_layer_clip . width == 0 || t_layer_clip . height == 0)
		return false;
	
	// Create the new layer
	Layer *t_new_layer;
	t_new_layer = layer_create(t_layer_clip);
	if (t_new_layer == NULL)
	{
		f_layers -> nesting += 1;
		return true;
	}
	
	// Set the effect parameters
	t_new_layer -> effects = p_effects;
	t_new_layer -> effects_shape = p_shape;
	
	// Link it into the chain
	f_layers = t_new_layer;

	return true;
}

void MCQuickDrawContext::end(void)
{
  Layer *t_src_layer, *t_dst_layer;
	
	t_src_layer = f_layers;
	t_dst_layer = t_src_layer -> parent;
	
	if (t_src_layer -> nesting > 0)
	{
		t_src_layer -> nesting -= 1;
		return;
	}
	
	void *t_src_ptr;
	uint4 t_src_stride;
	layer_lock(t_src_layer, t_src_ptr, t_src_stride);
	
	void *t_dst_ptr;
	uint4 t_dst_stride;
	layer_lock(t_dst_layer, t_dst_ptr, t_dst_stride);
	
	// MW-2009-06-11: [[ Bitmap Effects ]] If we have effects to apply, hand off layer
	//   compositing.
	if (t_src_layer -> effects == NULL)
	{
		t_dst_ptr = (uint1 *)t_dst_ptr + (t_src_layer -> origin . y - t_dst_layer -> origin . y) * t_dst_stride + (t_src_layer -> origin . x - t_dst_layer -> origin . x) * 4;

		surface_combiner_t t_combiner;
		t_combiner = (t_dst_layer -> flags & FLAG_IS_ALPHA) ? s_surface_combiners[t_dst_layer -> function] : s_surface_combiners_nda[t_dst_layer -> function];
		t_combiner(t_dst_ptr, t_dst_stride, t_src_ptr, t_src_stride, t_src_layer -> width, t_src_layer -> height, t_dst_layer -> opacity);
	}
	else
	{
		// For now, if opacity is not 100%, or we have a non-srcOver function then we must
		// use a temporary buffer.
		void *t_tmp_bits;
		uint32_t t_tmp_stride;
		if (t_dst_layer -> opacity != 255 || (t_dst_layer -> function != GXblendSrcOver && t_dst_layer -> function != GXcopy))
		{
			t_tmp_bits = malloc(t_dst_layer -> clip . width * t_dst_layer -> clip . height * sizeof(uint4));
			t_tmp_stride = t_dst_layer -> clip . width * sizeof(uint4);
			memset(t_tmp_bits, 0, t_dst_layer -> clip . width * t_dst_layer -> clip . height * sizeof(uint4));
		}
		else
			t_tmp_bits = NULL, t_tmp_stride = 0;
		
		MCBitmapEffectLayer t_dst;
		t_dst . bounds = t_dst_layer -> clip;
		if (t_tmp_bits == NULL)
		{
			t_dst . stride = t_dst_stride;
			t_dst . bits = (uint1 *)t_dst_ptr + t_dst_stride * (t_dst_layer -> clip . y - t_dst_layer -> origin . y) + (t_dst_layer -> clip . x - t_dst_layer -> origin . x) * 4;
			t_dst . has_alpha = (t_dst_layer -> flags & FLAG_IS_ALPHA);
		}
		else
		{
			t_dst . stride = t_tmp_stride;
			t_dst . bits = t_tmp_bits;
			t_dst . has_alpha = true;
		}
		
		MCBitmapEffectLayer t_src;
		MCU_set_rect(t_src . bounds, t_src_layer -> origin . x, t_src_layer -> origin . y, t_src_layer -> width, t_src_layer -> height);
		t_src . stride = t_src_stride;
		t_src . bits = t_src_ptr;
		t_src . has_alpha = true;
		
		MCBitmapEffectsRender(t_src_layer -> effects, t_src_layer -> effects_shape, t_dst, t_src);
		
		if (t_tmp_bits != NULL)
		{
			surface_combiner_t t_combiner;
			t_combiner = (t_dst_layer -> flags & FLAG_IS_ALPHA) ? s_surface_combiners[t_dst_layer -> function] : s_surface_combiners_nda[t_dst_layer -> function];
			t_combiner(
					   (uint1 *)t_dst_ptr + t_dst_stride * (t_dst_layer -> clip . y - t_dst_layer -> origin . y) + (t_dst_layer -> clip . x - t_dst_layer -> origin . x) * 4, t_dst_stride,
					   t_tmp_bits, t_tmp_stride, t_dst_layer -> clip . width, t_dst_layer -> clip . height, t_dst_layer -> opacity);
			free(t_tmp_bits);
		}		
	}
	
	layer_unlock(t_dst_layer, t_dst_ptr, t_dst_stride);
	layer_unlock(t_src_layer, t_src_ptr, t_src_stride);
	
	f_layers = layer_destroy(t_src_layer);
	
	setflags(FLAG_FOREGROUND_CHANGED | FLAG_BACKGROUND_CHANGED | FLAG_FONT_CHANGED | FLAG_STROKE_CHANGED | FLAG_FILL_CHANGED, true);
}

void MCQuickDrawContext::setexternalalpha(MCBitmap* p_bitmap)
{
	f_external_alpha = p_bitmap;
	f_layers -> flags |= FLAG_IS_ALPHA;
	
	void *t_dst_ptr;
	uint4 t_dst_stride;
	layer_lock(f_layers, t_dst_ptr, t_dst_stride);
	
	surface_merge_with_alpha(t_dst_ptr, t_dst_stride, p_bitmap -> data, p_bitmap -> bytes_per_line, f_layers -> width, f_layers -> height);
	
	layer_unlock(f_layers, t_dst_ptr, t_dst_stride);
}

void MCQuickDrawContext::setclip(const MCRectangle& p_clip)
{
	if (!MCU_equal_rect(p_clip, f_layers -> clip))
	{
		f_layers -> clip = p_clip;
		setflags(FLAG_CLIP_CHANGED, true);
	}
}

const MCRectangle& MCQuickDrawContext::getclip(void) const
{
  return f_layers -> clip;
}

void MCQuickDrawContext::clearclip(void)
{
  MCRectangle t_bounds;
	t_bounds . x = f_layers -> origin . x;
	t_bounds . y = f_layers -> origin . y;
	t_bounds . width = f_layers -> width;
	t_bounds . height = f_layers -> height;
	
	if (!MCU_equal_rect(t_bounds, f_layers -> clip))
		setclip(t_bounds);
}


void MCQuickDrawContext::setorigin(int2 x, int2 y)
{
	if (f_layers -> origin . x != x || f_layers -> origin . y != y)
	{
		f_layers -> origin . x = x;
		f_layers -> origin . y = y;
		setflags(FLAG_ORIGIN_CHANGED, true);
	}
}

void MCQuickDrawContext::clearorigin(void)
{
  assert( false );
}


void MCQuickDrawContext::setquality(uint1 p_quality)
{
	f_quality = p_quality;
}

void MCQuickDrawContext::setfunction(uint1 p_function)
{
  if (p_function != f_layers -> function)
	{
	  f_layers -> function = p_function;
		setflags(FLAG_FUNCTION_CHANGED, true);
	}
}

uint1 MCQuickDrawContext::getfunction(void)
{
	return f_layers -> function;
}

void MCQuickDrawContext::setopacity(uint1 p_opacity)
{
	f_layers -> opacity = p_opacity;
}

uint1 MCQuickDrawContext::getopacity(void)
{
	return f_layers -> opacity;
}

void MCQuickDrawContext::setforeground(const MCColor& c)
{
	if (c . red != f_fill . colour . red || c . green != f_fill . colour . green || c . blue != f_fill . colour . blue)
	{
		f_fill . colour = c;
		setflags(FLAG_FOREGROUND_CHANGED | FLAG_STROKE_CHANGED | FLAG_FILL_CHANGED, true);
	}
}

void MCQuickDrawContext::setbackground(const MCColor& c)
{
  if (c . red != f_background . red || c . green != f_background . green || c . blue != f_background . blue)
	{
	  f_background = c;
		setflags(FLAG_BACKGROUND_CHANGED, true);
	}
}

void MCQuickDrawContext::setdashes(uint2 p_offset, const uint1 *p_data, uint2 p_length)
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

void MCQuickDrawContext::setfillstyle(uint2 style, Pixmap p, int2 x, int2 y)
{
  if (style != f_fill . style || p != f_fill . pattern || x != f_fill . origin . x || y != f_fill . origin . y)
	{
		if (style != FillTiled || p != NULL)
		{
			f_fill . style = style;
			f_fill . pattern = p;
			f_fill . origin . x = x;
			f_fill . origin . y = y;
		}
		else
		{
			f_fill . style = FillSolid;
			f_fill . pattern = p;
			f_fill . origin . x = 0;
			f_fill . origin . y = 0;
		}

		setflags(FLAG_FILL_CHANGED, true);
	}
}

void MCQuickDrawContext::setgradient(MCGradientFill *p_gradient)
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

void MCQuickDrawContext::getfillstyle(uint2& style, Pixmap& p, int2& x, int2& y)
{
	style = f_fill . style;
	p = f_fill . pattern;
	x = f_fill . origin . x;
	y = f_fill . origin . y;
}

void MCQuickDrawContext::setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle)
{
  if (getflags(FLAG_DASHES_CHANGED) || linesize != f_stroke . width || linestyle != f_stroke . style || capstyle != f_stroke . cap || joinstyle != f_stroke . join)
	{
	  f_stroke . style = linestyle;
		f_stroke . width = linesize;
		f_stroke . cap = capstyle;
		f_stroke . join = joinstyle;
		setflags(FLAG_STROKE_CHANGED, true);
	}
}

void MCQuickDrawContext::setmiterlimit(real8 p_limit)
{
	if (getflags(FLAG_DASHES_CHANGED) || p_limit != f_stroke.miter_limit)
	{
		f_stroke.miter_limit = p_limit;
		setflags(FLAG_STROKE_CHANGED | FLAG_DASHES_CHANGED, true);
	}
}

void MCQuickDrawContext::drawline(int2 x1, int2 y1, int2 x2, int2 y2)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		int2 t_adjust;
		t_adjust = f_stroke . width >> 1;
		
		qd_stroke_begin();
		MoveTo(x1 - t_adjust, y1 - t_adjust);
		LineTo(x2 - t_adjust, y2 - t_adjust);
		qd_stroke_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_line(x1, y1, x2, y2, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCQuickDrawContext::drawlines(MCPoint *points, uint2 npoints, bool p_closed)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		int2 t_adjust;
		t_adjust = f_stroke . width >> 1;
		
		qd_stroke_begin();
		MoveTo(points[0] . x - t_adjust, points[0] . y - t_adjust);
		for(uint2 i = 1; i < npoints; ++i)
			LineTo(points[i] . x - t_adjust, points[i] . y - t_adjust);
		qd_stroke_end();
	}
	else
	{
		MCPath *t_path;
		if (p_closed)
			t_path = MCPath::create_polygon(points, npoints, true);
		else
			t_path = MCPath::create_polyline(points, npoints, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCQuickDrawContext::drawsegments(MCSegment *segments, uint2 nsegs)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		int2 t_adjust;
		t_adjust = f_stroke . width >> 1;
		
		qd_stroke_begin();
		for(uint2 i = 0; i < nsegs; ++i)
		{
			MoveTo(segments[i] . x1 - t_adjust, segments[i] . y1 - t_adjust);
			LineTo(segments[i] . x2 - t_adjust, segments[i] . y2 - t_adjust);
		}
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_polypolyline(segments, nsegs, true);
		drawpath(t_path);
		t_path -> release();
	}
}

int4 OSX_DrawUnicodeText(int2 x, int2 y, const void *p_text, uint4 p_text_byte_length, MCFontStruct *f, bool p_fill_background, bool p_measure_only = false)
{	
	OSStatus t_err;
	ATSUTextLayout t_layout;
	ATSUStyle t_style;
	
	ATSUFontID t_font_id;
	Fixed t_font_size;
	ATSLineLayoutOptions t_layout_options;
	
	ATSUAttributeTag t_tags[] =
	{
		kATSUFontTag,
		kATSUSizeTag,
	};
	ByteCount t_sizes[] =
	{
		sizeof(ATSUFontID),
		sizeof(Fixed),
	};
	ATSUAttributeValuePtr t_attrs[] =
	{
		&t_font_id,
		&t_font_size,
	};
	
	ATSUAttributeTag t_layout_tags[] =
	{
		kATSULineLayoutOptionsTag,
	};
	ByteCount t_layout_sizes[] =
	{
		sizeof(ATSLineLayoutOptions)
	};
	ATSUAttributeValuePtr t_layout_attrs[] =
	{
		&t_layout_options
	};
	
	UniCharCount t_run = p_text_byte_length / 2;

	t_err = ATSUFONDtoFontID((short)(intptr_t)f -> fid, f -> style, &t_font_id);
	
	t_font_size = f -> size << 16;
	
	t_err = ATSUCreateStyle(&t_style);
	t_err = ATSUSetAttributes(t_style, sizeof(t_tags) / sizeof(ATSUAttributeTag), t_tags, t_sizes, t_attrs);
	
	t_err = ATSUCreateTextLayoutWithTextPtr((const UniChar *)p_text, 0, p_text_byte_length / 2, p_text_byte_length / 2, 1, &t_run, &t_style, &t_layout);
	t_err = ATSUSetTransientFontMatching(t_layout, true);
	
	t_layout_options = kATSLineUseDeviceMetrics | kATSLineFractDisable;
	t_err = ATSUSetLayoutControls(t_layout, sizeof(t_layout_tags) / sizeof(ATSUAttributeTag), t_layout_tags, t_layout_sizes, t_layout_attrs);
	
	int4 t_result;
	t_result = 0;
	
	if (p_fill_background || p_measure_only)
	{
		ATSUTextMeasurement t_before, t_after, t_ascent, t_descent;
		ATSUGetUnjustifiedBounds(t_layout, 0, p_text_byte_length / 2, &t_before, &t_after, &t_ascent, &t_descent);
		
		t_ascent = (t_ascent + 0xffff) >> 16;
		t_descent = (t_descent + 0xffff) >> 16;
		t_after = (t_after + 0xffff) >> 16;
		
		Rect t_bounds;
		t_bounds . left = x;
		t_bounds . top = y - f -> ascent;
		t_bounds . right = x + t_after;
		t_bounds . bottom = y - f -> ascent + (t_descent + t_ascent);
		
		if (p_fill_background)
			EraseRect(&t_bounds);
		else
			t_result = t_after;
	}
	
	if (!p_measure_only)
		t_err = ATSUDrawText(t_layout, 0, p_text_byte_length / 2, kATSUUseGrafPortPenLoc, kATSUUseGrafPortPenLoc);
	
	t_err = ATSUDisposeTextLayout(t_layout);
	t_err = ATSUDisposeStyle(t_style);
	
	return t_result;
}
	
void MCQuickDrawContext::drawtext(int2 x, int2 y, const char *s, uint2 length, MCFontStruct *f, Boolean image, bool p_unicode_override)
{	
	// MW-2012-02-17: [[ FontRefs ]] If the font being passed is different from the one
	//   that was last used, schedule a change.
	if (f != f_font)
	{
		f_font = f;
		setflags(FLAG_FONT_CHANGED, true);
	}
	
	qd_text_begin();

	MoveTo(x, y);
	
	TextMode(srcOr);
		
	// MW-2012-02-20: [[ Bug ]] Make sure we check the fontstruct and the override flag.
	if (f -> unicode || p_unicode_override)
	{
		if (MCmajorosversion >= 0x1050)
		{
			// MW-2012-03-15: [[ Bug ]] Some fonts (like Helvetica Neue) seem to cause issues
			//   when QD and ATSUI are used to render them - after using QD, the font is
			//   rendered slightly differently (lighter, in fact).
			DrawText(" ", 0, 1);
			MoveTo(x, y);
			OSX_DrawUnicodeText(x, y, s, length, f, image);
		}
		else
		{
			char *tempbuffer = NULL;

			if (length)
			{
				uint2 *testchar = (uint2 *)s;
				if (testchar[(length - 2 )>> 1] == 12398)
				{
					tempbuffer = new char[length+2];
					memcpy(tempbuffer,s,length);
					uint2 *tchar = (uint2 *)&tempbuffer[length];
					*tchar = 0;
				}
			}
			
			
			CFStringRef cfstring;
			cfstring = CFStringCreateWithCharactersNoCopy(NULL,
					   (UniChar *)(tempbuffer != NULL? tempbuffer: s),
					   (tempbuffer != NULL? length+=2:length) >> 1,kCFAllocatorNull);
			Rect bounds;
			Point dimensions;
			SInt16 baseline;
			
			GetThemeTextDimensions(cfstring, kThemeCurrentPortFont, kThemeStateActive, false, &dimensions, &baseline);
			
			bounds.left = x;
			bounds.bottom = y - f -> ascent + dimensions.v;
			bounds.right = x + dimensions.h;
			bounds.top = y - f->ascent;
			if (image)
				EraseRect(&bounds);
				
			DrawThemeTextBox(cfstring, kThemeCurrentPortFont, kThemeStateActive, false,
							 &bounds, teFlushDefault, NULL);

			CFRelease(cfstring);
			if (tempbuffer)
				delete tempbuffer;
		}
	}
	else if (length > 0 && s != NULL)
	{
		// MW-2009-11-02: [[ Bug 8293 ]] We don't need to turn off anti-aliasing if the context is
		//   current 'opaque'.
		bool t_disable_aa;
		t_disable_aa = !image && getflags(FLAG_IS_ALPHA) && !getflags(FLAG_IS_OPAQUE) && MCantialiasedtextworkaround;
		if (t_disable_aa)
			QDSwapTextFlags(kQDUseTrueTypeScalerGlyphs);
		
		if (image)
		{
			Rect t_rect;
			SetRect(&t_rect, x, y - f -> ascent, x + textwidth(f, s, length), y + f -> descent);
			EraseRect(&t_rect);
		}
		
		// MW-2008-03-31: [[ Bug 6276 ]] Maximum length of line displayed is 32767.
		DrawText(s, 0, MCU_min(length, 32767));
		
		if (t_disable_aa)
			QDSwapTextFlags(kQDSupportedFlags);
	}
		
	qd_text_end();
}

void MCQuickDrawContext::drawrect(const MCRectangle& rect)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		MCRectangle t_rect;
		t_rect = MCU_reduce_rect(rect, -(f_stroke . width / 2));
		
		Rect t_mac_rect;
		MCRect2MacRect(t_rect, t_mac_rect);
	
	    qd_stroke_begin();
		FrameRect(&t_mac_rect);
		qd_stroke_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rectangle(rect, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCQuickDrawContext::dofillrect(const MCRectangle& rect, int p_function)
{
	Rect t_mac_rect;
	MCRect2MacRect(rect, t_mac_rect);
	
	qd_fill_begin();
	if (f_fill . style == FillTiled && getflags(FLAG_IS_IRREGULAR_PATTERN))
	{
		RgnHandle t_rgn;
		Rect t_pattern_bounds;
		
		PixMapHandle t_pixmap;
		t_pixmap = GetGWorldPixMap((CGrafPtr)f_fill . pattern -> handle . pixmap);
		
		ForeColor(blackColor);
		BackColor(whiteColor);
		
		LockPixels(t_pixmap);
		
		if (!getflags(FLAG_IS_OFFSCREEN))
		{
			LockPortBits(f_layers -> port);
			f_layers -> pixels = GetPortPixMap(f_layers -> port);
			LockPixels(f_layers -> pixels);
		}
		
		GetPixBounds(t_pixmap, &t_pattern_bounds);
		
		int2 t_ox, t_oy;
		t_ox = (f_fill . origin . x - t_mac_rect . left) % t_pattern_bounds . right;
		t_oy = (f_fill . origin . y - t_mac_rect . top) % t_pattern_bounds . bottom;
		if (t_ox < 0)
			t_ox += t_pattern_bounds . right;
		if (t_oy < 0)
			t_oy += t_pattern_bounds . bottom;
		
		t_rgn = NewRgn();
		
		RectRgn(t_rgn, &t_mac_rect);
		
		for(int2 y = t_mac_rect . top - (t_pattern_bounds . bottom - t_oy); y < t_mac_rect . bottom; y += t_pattern_bounds . bottom)
			for(int2 x = t_mac_rect . left - (t_pattern_bounds . right - t_ox); x < t_mac_rect . right; x += t_pattern_bounds . right)
			{
				Rect t_dst_rect;
				SetRect(&t_dst_rect, x, y, x + t_pattern_bounds . right, y + t_pattern_bounds . bottom);
				CopyBits((BitMap *)*t_pixmap, (BitMap *)*(f_layers -> pixels), &t_pattern_bounds, &t_dst_rect, p_function == -1 ? srcCopy : p_function, t_rgn);
			}
				
		UnlockPixels(t_pixmap);
		
		setflags(FLAG_FOREGROUND_CHANGED | FLAG_BACKGROUND_CHANGED, true);
		
		if (!getflags(FLAG_IS_OFFSCREEN))
		{
			UnlockPixels(f_layers -> pixels);
			UnlockPortBits(f_layers -> port);
		}
		
		DisposeRgn(t_rgn);
	}
	else
	{
		if (p_function != -1)
			PenMode(p_function);
		PaintRect(&t_mac_rect);
	}
		
	qd_fill_end();
}

void MCQuickDrawContext::fillrect_with_native_function(const MCRectangle& rect, int func)
{
	dofillrect(rect, func);
}

void MCQuickDrawContext::fillrect(const MCRectangle& rect)
{
	if (f_quality == QUALITY_DEFAULT)
		dofillrect(rect, -1);
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rectangle(rect, false);
		fillpath(t_path);
		t_path -> release();
	}
}

void MCQuickDrawContext::fillrects(MCRectangle *rects, uint2 nrects)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		Rect t_mac_rect;
		qd_fill_begin();
		for(uint2 i = 0; i < nrects; ++i)
		{
			MCRect2MacRect(rects[i], t_mac_rect);
			PaintRect(&t_mac_rect);
		}
		qd_fill_end();
	}
	else
	{
		for(uint4 t_rectangle = 0; t_rectangle < nrects; ++t_rectangle)
			fillrect(rects[t_rectangle]);
	}
}

void MCQuickDrawContext::fillpolygon(MCPoint *points, uint2 npoints)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		PolyHandle t_handle;
		
		qd_fill_begin();
		t_handle = OpenPoly();
		MoveTo(points[0] . x, points[0] . y);
		for(uint2 i = 1; i < npoints; ++i)
			LineTo(points[i] . x, points[i] . y);
		ClosePoly();
		PaintPoly(t_handle);
		KillPoly(t_handle);
		qd_fill_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_polygon(points, npoints, true);
		fillpath(t_path);
		t_path -> release();
	}
}

void MCQuickDrawContext::drawroundrect(const MCRectangle& rect, uint2 radius)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		MCRectangle t_rect;
		t_rect = MCU_reduce_rect(rect, -(f_stroke . width / 2));
		
		Rect t_mac_rect;
		MCRect2MacRect(t_rect, t_mac_rect);
	
	  qd_stroke_begin();
		FrameRoundRect(&t_mac_rect, radius + 1, radius + 1);
		qd_stroke_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rounded_rectangle(rect, radius, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCQuickDrawContext::fillroundrect(const MCRectangle& rect, uint2 radius)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		Rect t_mac_rect;
		MCRect2MacRect(rect, t_mac_rect);
	
	  qd_fill_begin();
		PaintRoundRect(&t_mac_rect, radius + 1, radius + 1);
		qd_fill_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rounded_rectangle(rect, radius, false);
		fillpath(t_path);
		t_path -> release();
	}
}

void MCQuickDrawContext::drawarc(const MCRectangle& rect, uint2 start, uint2 angle)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		MCRectangle t_rect;
		t_rect = MCU_reduce_rect(rect, -(f_stroke . width / 2));
		
		Rect t_mac_rect;
		MCRect2MacRect(t_rect, t_mac_rect);
	
	  qd_stroke_begin();
		FrameArc(&t_mac_rect, (450 - start) % 360, -angle);
		qd_stroke_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_arc(rect, start, angle, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCQuickDrawContext::drawsegment(const MCRectangle& p_rectangle, uint2 p_start, uint2 p_angle)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		MCRectangle t_rect;
		t_rect = MCU_reduce_rect(p_rectangle, -(f_stroke . width / 2));
		
		Rect t_mac_rect;
		MCRect2MacRect(t_rect, t_mac_rect);
	
	  qd_stroke_begin();
		FrameArc(&t_mac_rect, (450 - p_start) % 360, -p_angle);

		int2 cx = p_rectangle.x + (p_rectangle.width >> 1);
		int2 cy = p_rectangle.y + (p_rectangle.height >> 1);
		real8 torad = M_PI * 2.0 / 360.0;
		real8 tw = (real8)p_rectangle.width;
		real8 th = (real8)p_rectangle.height;
		real8 sa = (real8)p_start * torad;
		
		int2 dx = cx + (int2)(cos(sa) * tw / 2.0);
		int2 dy = cy - (int2)(sin(sa) * th / 2.0);
		MoveTo(dx, dy);
		LineTo(cx, cy);

		sa = (real8)(p_start + p_angle) * torad;
		dx = cx + (int2)(cos(sa) * tw / 2.0);
		dy = cy - (int2)(sin(sa) * th / 2.0);
		LineTo(dx, dy);
		
		qd_stroke_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_segment(p_rectangle, p_start, p_angle, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCQuickDrawContext::fillarc(const MCRectangle& rect, uint2 start, uint2 angle)
{
	if (f_quality == QUALITY_DEFAULT)
	{
		Rect t_mac_rect;
		MCRect2MacRect(rect, t_mac_rect);
	
	  qd_fill_begin();
		PaintArc(&t_mac_rect, (450 - start) % 360, -angle);
    qd_fill_end();
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_segment(rect, start, angle, false);
		fillpath(t_path);
		t_path -> release();
	}
}

void MCQuickDrawContext::drawpath(MCPath *path)
{
	MCCombiner *t_combiner = combiner_lock();

	if (t_combiner == NULL)
		return;

	path -> stroke(t_combiner, f_layers -> clip, &f_stroke);
	combiner_unlock(t_combiner);
}

void MCQuickDrawContext::fillpath(MCPath *path, bool p_evenodd)
{
	MCCombiner *t_combiner = combiner_lock();

	if (t_combiner == NULL)
		return;

	path -> fill(t_combiner, f_layers -> clip, p_evenodd);
	combiner_unlock(t_combiner);
}

void MCQuickDrawContext::draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty, const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length, const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect)
{
}

static uint1 *s_drawpict_pic_data;
static pascal void drawpict_get_pict_callback(void *dataPtr, short bc)
{
	memcpy(dataPtr, s_drawpict_pic_data, bc);
	s_drawpict_pic_data += bc;
}

void qd_draw_pict(void *data, const MCRectangle& drect, const MCRectangle& crect)
{
	RgnHandle oldclip = NewRgn();
	GetClip(oldclip);
	
	RgnHandle newclip = NewRgn();
	SetRectRgn(newclip, crect.x, crect.y, crect.x + crect.width, crect.y + crect.height);
	SectRgn(oldclip, newclip, newclip);
	SetClip(newclip);
	DisposeRgn(newclip);
	
	Rect destRect;
	SetRect(&destRect, drect.x, drect.y, drect.x + drect.width, drect.y + drect.height);
	
	CQDProcs myprocs;
	SetStdCProcs(&myprocs);
	myprocs.getPicProc = NewQDGetPicUPP(drawpict_get_pict_callback);

	CQDProcsPtr savedprocs = GetPortGrafProcs(GetQDGlobalsThePort());
	SetPortGrafProcs(GetQDGlobalsThePort(), &myprocs);
	PicHandle mypic = (PicHandle)NewHandle(sizeof(Picture));
	HLock((Handle)mypic);
	memcpy(*mypic, data, sizeof(Picture));
	HUnlock((Handle)mypic);
	s_drawpict_pic_data = (uint1 *)data + sizeof(Picture);
	DrawPicture(mypic, &destRect);
	DisposeHandle((Handle)mypic);

	DisposeQDGetPicUPP(myprocs.getPicProc);

	SetPortGrafProcs(GetQDGlobalsThePort(), savedprocs);
	SetClip(oldclip);
	DisposeRgn(oldclip);
}

void MCQuickDrawContext::drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect)
{
	qd_common_begin();
	
	qd_draw_pict(data, drect, crect);
	
	qd_common_end();
}

void MCQuickDrawContext::drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy)
{
	MCImageBitmap *t_bits;
	t_bits = p_image . bitmap;

	if (t_bits == NULL)
		return;

	MCRectangle t_clip, t_dr;
	t_clip = f_layers -> clip;
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
	
	
	void *t_dst_ptr;
	uint32_t t_dst_stride;
	
	layer_lock(f_layers, t_dst_ptr, t_dst_stride);
	t_dst_ptr = (uint8_t*)t_dst_ptr + (dy - f_layers->origin.y) * t_dst_stride + (dx - f_layers->origin.x) * 4;
	
	// MW-2011-09-22: Special case for alpha-blended/non-masked image with GXcopy/GXblendSrcOver.
	if (f_layers -> function == GXcopy || f_layers -> function == GXblendSrcOver)
	{
		void *t_src_ptr;
		uint4 t_src_stride;
		t_src_stride = t_bits->stride;
		t_src_ptr = (uint8_t*)t_bits->data + sy * t_src_stride + sx * 4;
		
		if (MCImageBitmapHasTransparency(t_bits))
			surface_combine_blendSrcOver_masked(t_dst_ptr, t_dst_stride, t_src_ptr, t_src_stride, sw, sh, f_layers->opacity);
		else
			surface_combine_blendSrcOver_solid(t_dst_ptr, t_dst_stride, t_src_ptr, t_src_stride, sw, sh, f_layers->opacity);

		layer_unlock(f_layers, t_src_ptr, t_src_stride);
		return;
	}

	void *t_pixel_ptr;
	uint4 t_pixel_stride;
	
	t_pixel_stride = sw * 4;
	t_pixel_ptr = malloc(sh * t_pixel_stride);
	if (t_pixel_ptr == NULL)
		return;
		
	MCImageBitmapPremultiplyRegion(t_bits, sx, sy, sw, sh, t_pixel_stride, (uint32_t *)t_pixel_ptr);
		
	surface_combiner_t t_combiner;
	t_combiner = getflags(FLAG_IS_ALPHA) ? s_surface_combiners[f_layers -> function] : s_surface_combiners_nda[f_layers -> function];
	
	t_combiner(t_dst_ptr, t_dst_stride, t_pixel_ptr, t_pixel_stride, sw, sh, f_layers -> opacity);

	layer_unlock(f_layers, t_dst_ptr, t_dst_stride);

	free(t_pixel_ptr);
}

void MCQuickDrawContext::drawlink(MCStringRef p_name, const MCRectangle& p_area)
{
}

int4 MCQuickDrawContext::textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override)
{
	return MCscreen -> textwidth(f, s, l, p_unicode_override);
}

void MCQuickDrawContext::applywindowshape(MCWindowShape *p_mask, unsigned int p_update_width, unsigned int p_update_height)
{
	MCRectangle t_clip;
	t_clip = f_layers -> clip;

	MCRectangle t_mask_clip;
	MCU_set_rect(t_mask_clip, 0, 0, p_mask -> width, p_mask -> height);
	t_clip = MCU_intersect_rect(t_clip, t_mask_clip);
	
	p_update_width = MCU_min(p_update_width, f_layers -> width);
	p_update_height = MCU_min(p_update_height, f_layers -> height);

	void *t_original_dst_ptr;
	uint4 t_dst_stride;
	
	void *t_src_ptr;

	layer_lock(f_layers, t_original_dst_ptr, t_dst_stride);

	uint1 *t_dst_ptr;
	t_dst_ptr = ((uint1 *)t_original_dst_ptr) + (t_clip . y - f_layers -> origin . y) * t_dst_stride + (t_clip . x - f_layers -> origin . x) * 4;
	
	t_src_ptr = p_mask -> data + t_clip . y * p_mask -> stride + t_clip . x;

	surface_merge_with_alpha(t_dst_ptr, t_dst_stride, t_dst_ptr, t_dst_stride, t_src_ptr, p_mask -> stride, t_clip . width, t_clip . height);
	
	if (p_update_width > p_mask -> width)
	{
		for(uint4 t_height = t_clip . height; t_height > 0; --t_height, t_dst_ptr += t_dst_stride)
			memset((uint1 *)t_dst_ptr + p_mask -> width * 4, 0, (p_update_width - p_mask -> width) * 4);
	}
	else if (p_update_height > p_mask -> height)
		t_dst_ptr += t_dst_stride * p_mask -> height;
	
	if (p_update_height > p_mask -> height)
	{
		for(; p_update_height > p_mask -> height; --p_update_height, t_dst_ptr += t_dst_stride)
			memset(t_dst_ptr, 0, p_update_width * 4);
	}
	
	layer_unlock(f_layers, t_dst_ptr, t_dst_stride);
}

void MCQuickDrawContext::clear(const MCRectangle *rect)
{
	void *t_bits;
	uint1 *t_dst_ptr;
	uint4 t_dst_stride;
	
	layer_lock(f_layers, t_bits, t_dst_stride);
	t_dst_ptr = (uint1*)t_bits;
	for(uint4 y = f_layers -> height; y > 0; --y, t_dst_ptr += t_dst_stride)
		memset(t_dst_ptr, 0, t_dst_stride);
	layer_unlock(f_layers, t_dst_ptr, t_dst_stride);
}

MCRegionRef MCQuickDrawContext::computemaskregion(void)
{
	uint4 t_stride;
	void *t_bits;
	
	t_stride = ((f_layers -> width + 31) & ~31) / 8;
	t_bits = malloc(f_layers -> height * t_stride);
	if (t_bits == NULL)
		return NULL;
	
	memset(t_bits, 0, t_stride * f_layers -> height);
	
	void *t_src_bits;
	uint1 *t_src_ptr;
	uint4 t_src_stride;
	layer_lock(f_layers, t_src_bits, t_src_stride);
	t_src_ptr = (uint1 *)t_src_bits;
	
	uint1 *t_bits_ptr;
	t_bits_ptr = (uint1 *)t_bits;
	
	for(uint4 y = f_layers -> height; y > 0; --y, t_bits_ptr += t_stride, t_src_ptr += t_src_stride)
	{
		uint1 t_mask = 0x80;
		for(uint4 x = 0; x < f_layers -> width; ++x)
		{
			if ((((uint4 *)t_src_ptr)[x] >> 24) != 0)
				t_bits_ptr[x >> 3] |= t_mask;
			t_mask = t_mask >> 1;
			if (t_mask == 0)
				t_mask = 0x80;
		}
	}
	
	layer_unlock(f_layers, t_src_ptr, t_src_stride);
	
	BitMap t_bitmap;
	t_bitmap . baseAddr = (char *)t_bits;
	t_bitmap . rowBytes = t_stride;
	t_bitmap . bounds . left = f_layers -> origin . x;
	t_bitmap . bounds . top = f_layers -> origin . y;
	t_bitmap . bounds . right = f_layers -> origin . x + f_layers -> width;
	t_bitmap . bounds . bottom = f_layers -> origin . y + f_layers -> height;
	
	MCRegionRef t_region;
	t_region = nil;
	if (MCRegionCreate(t_region))
		BitMapToRegion((RgnHandle)t_region, &t_bitmap);
		
	free(t_bits);
	
	return t_region;
}

MCBitmap *MCQuickDrawContext::lock(void)
{
	MCBitmap *t_bitmap;
	t_bitmap = new MCBitmap;
	t_bitmap -> width = f_layers -> width;
	t_bitmap -> height = f_layers -> height;
	t_bitmap -> format = ZPixmap;
	t_bitmap -> bitmap_unit = 32;
	t_bitmap -> byte_order = MSBFirst;
	t_bitmap -> bitmap_pad = 32;
	t_bitmap -> bitmap_bit_order = MSBFirst;
	t_bitmap -> depth = 32;
	t_bitmap -> bits_per_pixel = 32;
	t_bitmap -> red_mask = 0xFF;
	t_bitmap -> green_mask = 0xFF;
	t_bitmap -> blue_mask = 0xFF;
	
	void *t_data;
	uint4 t_stride;
	
	layer_lock(f_layers, t_data, t_stride);
	t_bitmap -> data = (char *)t_data;
	t_bitmap -> bytes_per_line = t_stride;
	
	return t_bitmap;
}

void MCQuickDrawContext::unlock(MCBitmap *t_bitmap)
{
	layer_unlock(f_layers, t_bitmap -> data, t_bitmap -> bytes_per_line);
	delete t_bitmap;
}

uint2 MCQuickDrawContext::getdepth(void) const
{
	return 32;
}

const MCColor& MCQuickDrawContext::getblack(void) const
{
	return MCscreen -> black_pixel;
}

const MCColor& MCQuickDrawContext::getwhite(void) const
{
	return MCscreen -> white_pixel;
}

const MCColor& MCQuickDrawContext::getgray(void) const
{
	return MCscreen -> gray_pixel;
}

const MCColor& MCQuickDrawContext::getbg(void) const
{
	return MCscreen -> background_pixel;
}


void MCQuickDrawContext::qd_stroke_begin(void)
{
	qd_fill_begin();
	
	if (getflags(FLAG_STROKE_CHANGED))
	{
		uint2 t_line_size;
		t_line_size = f_stroke . width == 0 ? 1 : f_stroke . width;
		PenSize(t_line_size, t_line_size);
		
		if (f_stroke . style == LineOnOffDash || f_stroke . style == LineDoubleDash)
			PenPat((const Pattern *)s_dash_pattern);
	}
}

void MCQuickDrawContext::qd_stroke_end(void)
{
	qd_fill_end();
}

static int4 powerof2(int4 start)
{
	//1 restrict the pixel pattern's width and height to the power of 2
	//this routine is used to figure the the largest power of 2 number that is
	//less than or equal to the parameter passed in
	int4 s = 8;
	while (s <= start)
		s <<= 1;
	return s >> 1;
}

bool qd_begin_pattern(Drawable p_pattern, const MCPoint& p_origin, PixPatHandle p_pix_pat, GWorldPtr& r_pattern_port, Ptr& r_pattern_address)
{
	PixMapHandle t_src;
	t_src = GetGWorldPixMap((CGrafPtr)p_pattern -> handle . pixmap);
	LockPixels(t_src);

	int4 t_width, t_height;
	t_width = powerof2((*t_src) -> bounds . right);
	t_height = powerof2((*t_src) -> bounds . bottom);
	
	Rect r;
	SetRect(&r, 0, 0, t_width, t_height);
	
	OSErr err;
	err = NewGWorld(&r_pattern_port, 32, &r, NULL, NULL, MCmajorosversion >= 0x1040 ? kNativeEndianPixMap : 0);
	if (r_pattern_port == NULL || err != noErr)
	{
		UnlockPixels(t_src);
		r_pattern_port = NULL;
		return false;
	}

	//draw the tile 4 times with different x,y calculated in order to produce
	//the result of the patter/tile starts at the 0,0 of the object as the
	//objce moves.
	int2 x = p_origin . x;
	int2 y = p_origin . y;
	int2 w = (*t_src)->bounds.right - (*t_src)->bounds.left; //get tile w and h
	int2 h = (*t_src)->bounds.bottom - (*t_src)->bounds.top;
	x %= w;
	y %= h;
	
	GDHandle t_old_device;
	GWorldPtr t_old_gworld;
	GetGWorld(&t_old_gworld, &t_old_device);
	
	SetGWorld(r_pattern_port, NULL);
	
	PixMapHandle t_dst = GetGWorldPixMap(r_pattern_port);
	LockPixels(t_dst); //lock destination pixmap
	ForeColor(blackColor);
	BackColor(whiteColor);
	
	//first draw pattern at 9-12 o'clock, r contains p's rect
	r.top = (y - h) % h;
	r.bottom = r.top + h;
	r.left = (x - w) % w;
	r.right = r.left + w;
	CopyBits((BitMap *)*t_src, (BitMap *)*t_dst, &((*t_src)->bounds),
					 &r, srcCopy, NULL);
	//2nd shift tile at 12-3 o'clock, r.left and r.right is unchanged
	r.left += w;
	r.right = r.left + w;
	CopyBits((BitMap *)*t_src, (BitMap *)*t_dst, &((*t_src)->bounds),
					 &r, srcCopy, NULL);
	//3rd shift at 6-9 o'clock, r.left and r.right is unchanged
	r.top += h;
	r.bottom = r.top + h;
	r.left = (x - w) % w;
	r.right = r.left + w;
	CopyBits((BitMap *)*t_src, (BitMap *)*t_dst, &((*t_src)->bounds),
					 &r, srcCopy, NULL);
	//4th shiftat 3-6 o'clock, r.left and r.right is unchanged
	r.left += w;
	r.right = r.left + w;
	CopyBits((BitMap *)*t_src, (BitMap *)*t_dst, &((*t_src)->bounds),
					 &r, srcCopy, NULL);
	UnlockPixels(t_src); //unlock source pixmap
	
	SetGWorld(t_old_gworld, t_old_device);
	
	uint4 t_row_bytes = GetPixRowBytes(t_dst);
	r_pattern_address = GetPixBaseAddr(t_dst); //get addr of destination pixmap(our new pixmap)
	(*p_pix_pat)->patData = (char **)&r_pattern_address;
	(*p_pix_pat)->patMap = t_dst;
	
  // MW-2006-05-03: [[ Bug 3505 ]] - We need to byte-swap QD back patterns if running on OSX pre-Leopard
#ifdef __LITTLE_ENDIAN__
	uint4 *t_ptr;
	if (MCmajorosversion < 0x1050)
	{
		t_ptr = (uint4 *)r_pattern_address;
		for(uint4 t_row = 0; t_row < t_height; ++t_row, t_ptr += t_row_bytes / 4)
			for(uint4 t_column = 0; t_column < t_width; ++t_column)
				t_ptr[t_column] = MCSwapInt32NetworkToHost(t_ptr[t_column]);
	}
#endif
	
	PenPixPat(p_pix_pat);
	
	PixPatChanged(p_pix_pat);

	return t_width != (*t_src) -> bounds . right || t_height != (*t_src) -> bounds . bottom;
}

void qd_end_pattern(PixPatHandle p_pix_pat, GWorldPtr p_port)
{
	UnlockPixels((*p_pix_pat) -> patMap);
	DisposeGWorld(p_port);
}

void MCQuickDrawContext::qd_fill_begin(void)
{
	qd_common_begin();

	if (!getflags(FLAG_FILL_CHANGED))
		return;

	if (f_pattern_port != NULL)
	{	
		qd_end_pattern(f_pattern, f_pattern_port);
		f_pattern_port = NULL;
			
		(*f_pattern) -> patData = f_old_pattern_data;
		(*f_pattern) -> patMap = f_old_pattern;
	}
	
	switch(f_fill . style == FillTiled && f_fill . pattern == NULL ? FillSolid : f_fill . style)
	{
		case FillSolid:
		{
			Pattern temp;
			PenPat(GetQDGlobalsBlack(&temp));
		}
		break;
		
		case FillStippled:
		case FillOpaqueStippled:
		{
			Pattern temp;
			if (f_fill . pattern != NULL)
				BackPat(&g_stipple_pattern);
			else
				PenPat(GetQDGlobalsGray(&temp));
		}
		break;
	
		case FillTiled:
		{
			OSErr err;
		
			if (f_pattern == NULL)
			{
				f_pattern = NewPixPat();
				f_old_pattern = (*f_pattern) -> patMap;
				f_old_pattern_data = (*f_pattern) -> patData;
				(*f_pattern) -> patType = 1;
				if (f_pattern == NULL)
					return;
			}

			bool t_is_irregular;
			t_is_irregular = qd_begin_pattern(f_fill . pattern, f_fill . origin, f_pattern, f_pattern_port, f_pattern_base_address);
			setflags(FLAG_IS_IRREGULAR_PATTERN, t_is_irregular);

			setflags(FLAG_FOREGROUND_CHANGED | FLAG_BACKGROUND_CHANGED, true);
		}
		break;
	}
}

void MCQuickDrawContext::qd_fill_end(void)
{
  qd_common_end();
}

void MCQuickDrawContext::qd_text_begin(void)
{
	qd_common_begin();

	if (getflags(FLAG_FONT_CHANGED))
	{
		TextFont((short)(intptr_t)f_font -> fid);
		TextSize(f_font -> size);
		TextFace(f_font -> style);
		setflags(FLAG_FONT_CHANGED, false);
	}
}

void MCQuickDrawContext::qd_text_end(void)
{
	qd_common_end();
}

inline bool operator == (const RGBColor& a, const MCColor& b)
{
	return a . red == b . red && a . green == b . green && a . blue == b . blue;
}

void MCQuickDrawContext::qd_common_begin(void)
{
	uint4 t_changed_flags;
	
	t_changed_flags = 0;
	
	if (s_current_port != f_layers -> port)
	{
		SetGWorld(f_layers -> port, getflags(FLAG_IS_OFFSCREEN) ? NULL : GetMainDevice());
		s_current_port = f_layers -> port;
	}
	
	if (getflags(FLAG_CLIP_CHANGED))
	{
		Rect t_clip;
		t_clip . left = f_layers -> clip . x; // + f_layers -> origin . x;
		t_clip . top = f_layers -> clip . y; // + f_layers -> origin . y;
		t_clip . right = t_clip . left + f_layers -> clip . width;
		t_clip . bottom = t_clip . top + f_layers -> clip . height;
		ClipRect(&t_clip);
		t_changed_flags |= FLAG_CLIP_CHANGED;
	}
	
	if (getflags(FLAG_ORIGIN_CHANGED))
	{
		SetOrigin(f_layers -> origin . x, f_layers -> origin . y);
		t_changed_flags |= FLAG_ORIGIN_CHANGED;
	}

	if (getflags(FLAG_BACKGROUND_CHANGED))
	{
		RGBColor t_colour;
		t_colour . red = f_background . red;
		t_colour . green = f_background . green;
		t_colour . blue = f_background . blue;
		RGBBackColor(&t_colour);
		t_changed_flags |= FLAG_BACKGROUND_CHANGED;
	}

	if (getflags(FLAG_FOREGROUND_CHANGED))
	{
		RGBColor t_colour;
		t_colour . red = f_fill . colour . red;
		t_colour . green = f_fill . colour . green;
		t_colour . blue = f_fill . colour . blue;
		RGBForeColor(&t_colour);
		t_changed_flags |= FLAG_FOREGROUND_CHANGED;
	}
	
	if (getflags(FLAG_FUNCTION_CHANGED))
	{
		if (f_layers -> function <= LAST_NATIVE_FUNCTION && s_is_native_function[f_layers -> function])
		{
			uint4 t_mode;
			
			t_mode = s_native_function_map[f_layers -> function];
			if (t_mode < patCopy)
				t_mode += patCopy;
			PenMode(t_mode);
			
			RGBColor c;
			c.red = c.green = c.blue = 0x7FFF;
			OpColor(&c);
		}
		else
			PenMode(patCopy);
		
		t_changed_flags |= FLAG_FUNCTION_CHANGED;
	}
	
	setflags(t_changed_flags, false);
}

void MCQuickDrawContext::qd_common_end(void)
{
}

extern CGColorSpaceRef OSX_CGColorSpaceCreateGenericRGB(void);

CGContextRef MCQuickDrawContext::lock_as_cg(void)
{
	CGContextRef t_context = NULL;
	
	if (getflags(FLAG_IS_OFFSCREEN))
	{
		CGColorSpaceRef t_color_space;
		t_color_space = OSX_CGColorSpaceCreateGenericRGB();
		
		CGBitmapInfo t_bitmap_info;
		t_bitmap_info = kCGImageAlphaPremultipliedFirst;
		if (MCmajorosversion >= 0x1040)
			t_bitmap_info |= kCGBitmapByteOrder32Host;
		t_context = CGBitmapContextCreate(GetPixBaseAddr(f_layers -> pixels), f_layers -> width, f_layers -> height, 8, GetPixRowBytes(f_layers -> pixels), t_color_space, t_bitmap_info);
		CGColorSpaceRelease(t_color_space);
		
		CGContextTranslateCTM(t_context, 0, (float)f_layers -> height);
		CGContextScaleCTM(t_context, 1.0, -1.0);
		CGContextTranslateCTM(t_context, -f_layers -> origin . x, -f_layers -> origin . y);
		CGContextClipToRect(t_context, CGRectMake(f_layers -> clip . x, f_layers -> clip . y, f_layers -> clip . width, f_layers -> clip . height));
	}
	else
	{
		QDBeginCGContext(f_layers -> port, &t_context);
		CGContextTranslateCTM(t_context, 0, (float)f_layers -> height);
		CGContextScaleCTM(t_context, 1.0, -1.0);
		CGContextClipToRect(t_context, CGRectMake(f_layers -> clip . x, f_layers -> clip . y, f_layers -> clip . width, f_layers -> clip . height));
	}
	
	return t_context;
}

void MCQuickDrawContext::unlock_as_cg(CGContextRef p_context)
{
	if (getflags(FLAG_IS_OFFSCREEN))
		CGContextRelease(p_context);
	else
		QDEndCGContext(f_layers -> port, &p_context);
}


//-----------------------------------------------------------------------------
//  Theme Drawing Routines
//

// There are three modes for rendering of themes:
//   1) Using HITheme (10.4 or 10.3 if not a tab)
//   2) Using HIView and offscreen control (if offscreen buffer needed or default push button or 10.3 tab)
//   3) Using Appearence Manager
//
void MCQuickDrawContext::drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_info)
{
	extern void MCMacDrawTheme(MCThemeDrawType p_type, MCThemeDrawInfo& p_info, CGContextRef p_context);
	
		CGContextRef t_context;
		t_context = lock_as_cg();
		if (t_context != NULL)
		MCMacDrawTheme(p_type, *p_info, t_context);
		unlock_as_cg(t_context);
	
	setflags(FLAG_FONT_CHANGED | FLAG_FOREGROUND_CHANGED | FLAG_BACKGROUND_CHANGED | FLAG_ORIGIN_CHANGED | FLAG_CLIP_CHANGED | FLAG_STROKE_CHANGED | FLAG_FILL_CHANGED | FLAG_FUNCTION_CHANGED, true);
}

void MCQuickDrawContext::copyarea(Drawable p_src, uint4 p_dx, uint4 p_dy, uint4 p_sx, uint4 p_sy, uint4 p_sw, uint4 p_sh)
{
	PixMapHandle p_pixmap;
	Rect t_src_rect;
	Rect t_dst_rect;
	
	qd_common_begin();
	
	assert(p_src -> type == DC_BITMAP);
	
	p_pixmap = GetGWorldPixMap((CGrafPtr)p_src -> handle . pixmap);
	LockPixels(p_pixmap);
	ForeColor(blackColor);
	BackColor(whiteColor);
	SetRect(&t_src_rect, p_sx, p_sy, p_sx + p_sw, p_sy + p_sh);
	SetRect(&t_dst_rect, p_dx, p_dy, p_dx + p_sw, p_dy + p_sh);
	CopyBits((BitMap *)*p_pixmap, (BitMap *)*(f_layers -> pixels), &t_src_rect, &t_dst_rect, srcCopy, NULL);
	UnlockPixels(p_pixmap);
	
	qd_common_end();

	setflags(FLAG_FOREGROUND_CHANGED | FLAG_BACKGROUND_CHANGED, true);
}

void MCQuickDrawContext::combine(Pixmap data, int4 dx, int4 dy, int4 sx, int4 sy, uint4 sw, uint4 sh)
{
	MCRectangle t_clip, t_dr;
	t_clip = f_layers -> clip;
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
		
	PixMapHandle t_src_pixmap;
	void *t_src_ptr;
	uint4 t_src_stride;
	
	t_src_pixmap = GetGWorldPixMap((CGrafPtr)data -> handle . pixmap);
	LockPixels(t_src_pixmap);
	HLock((Handle)t_src_pixmap);
	
	t_src_ptr = GetPixBaseAddr(t_src_pixmap);
	t_src_stride = GetPixRowBytes(t_src_pixmap);
	
	t_src_ptr = (uint1 *)t_src_ptr + sy * t_src_stride + sx * 4;
	
	void *t_dst_ptr;
	uint4 t_dst_stride;
	layer_lock(f_layers, t_dst_ptr, t_dst_stride);
	
	t_dst_ptr = (uint1 *)t_dst_ptr + (dy - f_layers -> origin . y) * t_dst_stride + (dx - f_layers -> origin . x) * 4;
	
	surface_combiner_t t_combiner;
	t_combiner = getflags(FLAG_IS_ALPHA) ? s_surface_combiners[f_layers -> function] : s_surface_combiners_nda[f_layers -> function];
	
	t_combiner(t_dst_ptr, t_dst_stride, t_src_ptr, t_src_stride, sw, sh, f_layers -> opacity);

	layer_unlock(f_layers, t_dst_ptr, t_dst_stride);
	
	HUnlock((Handle)t_src_pixmap);
	UnlockPixels(t_src_pixmap);
}

//-----------------------------------------------------------------------------
//  Fill Combiners
//

#define INLINE inline

// r_i = (x_i * a) / 255
static INLINE uint4 packed_scale_bounded(uint4 x, uint1 a)
{
	uint4 u, v;

	u = ((x & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

// r_i = (x_i * a + y_i * b) / 255
static INLINE uint4 packed_bilinear_bounded(uint4 x, uint1 a, uint4 y, uint1 b)
{
	uint4 u, v;

	u = (x & 0xff00ff) * a + (y & 0xff00ff) * b + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

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
	else
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
	uint4 s;

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

MCCombiner *MCQuickDrawContext::combiner_lock(void)
{
	static bool s_solid_combiner_initialised = false;
	static MCSolidCombiner s_solid_combiner;
	static bool s_pattern_combiner_initialised = false;
	static MCPatternCombiner s_pattern_combiner;

	MCSurfaceCombiner *t_combiner;
	t_combiner = NULL;
	if (f_gradient_fill != NULL && f_gradient_fill->kind != kMCGradientKindNone && f_gradient_fill->ramp_length > 1)
	{
		t_combiner = MCGradientFillCreateCombiner(f_gradient_fill, f_layers -> clip);
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

		s_solid_combiner . pixel = 0xff000000 | (f_fill . colour . pixel & 0xffffff); //((f_fill . colour . pixel & 0xff) << 16) | (f_fill . colour . pixel & 0xff00) | ((f_fill . colour . pixel & 0xff0000) >> 16);
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
		
		PixMapHandle t_pattern;
		t_pattern = GetGWorldPixMap((CGrafPtr)f_fill . pattern -> handle . pixmap);
		LockPixels(t_pattern);
		
		Rect t_pattern_bounds;
		GetPixBounds(t_pattern, &t_pattern_bounds);

		s_pattern_combiner . pattern_bits = (uint4 *)GetPixBaseAddr(t_pattern);
		s_pattern_combiner . pattern_stride = GetPixRowBytes(t_pattern) >> 2;
		s_pattern_combiner . width = t_pattern_bounds . right - t_pattern_bounds . left;
		s_pattern_combiner . height = t_pattern_bounds . bottom - t_pattern_bounds . top;
		s_pattern_combiner . origin_x = f_fill . origin . x;
		s_pattern_combiner . origin_y = f_fill . origin . y;

		if (s_pattern_combiner . origin_x < 0)
			s_pattern_combiner . origin_x += s_pattern_combiner . width;
		if (s_pattern_combiner . origin_y < 0)
			s_pattern_combiner . origin_y += s_pattern_combiner . height;

		t_combiner = &s_pattern_combiner;
	}

	if (t_combiner != NULL)
	{
		void *t_dst_ptr;
		uint4 t_dst_stride;
		
		layer_lock(f_layers, t_dst_ptr, t_dst_stride);
		
		t_combiner -> bits = (uint4 *)t_dst_ptr - f_layers -> origin . y * t_dst_stride / 4 - f_layers -> origin . x;
		t_combiner -> stride = t_dst_stride / 4;
	}

	return t_combiner;
}

void MCQuickDrawContext::combiner_unlock(MCCombiner *p_combiner)
{
	layer_unlock(f_layers, NULL, 0);

	if (f_fill . style == FillTiled)
		UnlockPixels(GetGWorldPixMap((CGrafPtr)f_fill . pattern -> handle . pixmap));
}

//---------------------------------------------------------------------------------

void surface_merge(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;
	
	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride)
		for(uint4 x = 0; x < p_width; ++x)
			t_pixel_ptr[x] |= 0xFF000000;
}

void surface_unmerge(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride)
		for(uint4 x = 0; x < p_width; ++x)
			t_pixel_ptr[x] &= 0xFFFFFF;
}


void surface_merge(void *p_pixels, uint4 p_pixel_stride, void *p_src, uint4 p_src_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride / 4;

	uint4 *t_src_ptr;
	uint4 t_src_stride;
	
	t_src_ptr = (uint4 *)p_src;
	t_src_stride = p_src_stride / 4;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_src_ptr += t_src_stride)
		for(uint4 x = 0; x < p_width; ++x)
			t_pixel_ptr[x] = t_src_ptr[x] | 0xFF000000;
}

void surface_merge_with_mask(void *p_pixels, uint4 p_pixel_stride, void *p_src, uint4 p_src_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;
	
	uint4 *t_src_ptr;
	uint4 t_src_stride;

	uint1 *t_mask_ptr;
	uint4 t_mask_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	t_src_ptr = (uint4 *)p_src;
	t_src_stride = p_src_stride >> 2;

	t_mask_ptr = (uint1 *)p_mask;
	t_mask_stride = p_mask_stride;
	
	//return;
	
	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_src_ptr += t_src_stride, t_mask_ptr += t_mask_stride)
	{
		uint4 t_byte, t_bit;
		uint1 *t_bytes;

		t_bytes = t_mask_ptr;
		t_bit = 0x80 >> p_offset;
		t_byte = *t_bytes++;

		for(uint4 x = 0; x < p_width; ++x)
		{
			if ((t_byte & t_bit) != 0)
				t_pixel_ptr[x] = 0xFF000000 | (t_src_ptr[x] & 0xFFFFFF);
			else
				t_pixel_ptr[x] = 0;

			t_bit >>= 1;
			if (!t_bit && x < p_width - 1)
				t_bit = 0x80, t_byte = *t_bytes++;
		}
	}
}

void surface_merge_with_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint1 *t_mask_ptr;
	uint4 t_mask_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	t_mask_ptr = (uint1 *)p_mask;
	t_mask_stride = p_mask_stride;
	
	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_mask_ptr += t_mask_stride)
	{
		uint4 t_byte, t_bit;
		uint1 *t_bytes;

		t_bytes = t_mask_ptr;
		t_bit = 0x80 >> p_offset;
		t_byte = *t_bytes++;

		for(uint4 x = 0; x < p_width; ++x)
		{
			if ((t_byte & t_bit) != 0)
				t_pixel_ptr[x] = 0xFF000000 | (t_pixel_ptr[x] & 0xFFFFFF);
			else
				t_pixel_ptr[x] = 0;

			t_bit >>= 1;
			if (!t_bit && x < p_width - 1)
				t_bit = 0x80, t_byte = *t_bytes++;
		}
	}
}

void surface_extract_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_width, uint4 p_height, uint1 p_threshold)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint1 *t_mask_ptr;
	uint4 t_mask_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	t_mask_ptr = (uint1 *)p_mask;
	t_mask_stride = p_mask_stride;
	
	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_mask_ptr += t_mask_stride)
	{
		uint4 t_byte, t_bit;
		uint1 *t_bytes;

		t_bytes = t_mask_ptr;
		t_bit = 0x80;
		t_byte = 0;

		for(uint4 x = 0; x < p_width; ++x)
		{
			if ((t_pixel_ptr[x] >> 24) > p_threshold)
				t_byte |= t_bit;

			t_bit >>= 1;
			if (!t_bit)
				t_bit = 0x80, *t_bytes++ = t_byte, t_byte = 0;
		}
		if (t_bit != 0x80)
			*t_bytes = t_byte;
	}
}

void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_src, uint4 p_src_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint4 *t_src_ptr;
	uint4 t_src_stride;

	uint1 *t_alpha_ptr;
	uint4 t_alpha_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	t_src_ptr = (uint4 *)p_src;
	t_src_stride = p_src_stride >> 2;

	t_alpha_ptr = (uint1 *)p_alpha;
	t_alpha_stride = p_alpha_stride;

	//return;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_src_ptr += t_src_stride, t_alpha_ptr += t_alpha_stride)
		for(uint4 x = 0; x < p_width; ++x)
		{
			uint4 s = t_src_ptr[x];
			uint1 a = t_alpha_ptr[x];
			if (a == 0)
				t_pixel_ptr[x] = 0;
			else if (a == 255)
				t_pixel_ptr[x] = (s & 0xFFFFFF) | 0xFF000000;
			else
			{
				uint4 u, v;
				u = ((s & 0xff00ff) * a) + 0x800080;
				u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
				v = ((s & 0x00ff00) * a) + 0x8000;
				v = ((v + ((v >> 8) & 0x00ff00)) >> 8) & 0x00ff00;
				t_pixel_ptr[x] = (u + v) | (a << 24);
			}
		}
}

void surface_merge_with_alpha(void *p_src, uint4 p_src_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_src_ptr;
	uint4 t_src_stride;

	uint1 *t_alpha_ptr;
	uint4 t_alpha_stride;

	t_src_ptr = (uint4 *)p_src;
	t_src_stride = p_src_stride >> 2;

	t_alpha_ptr = (uint1 *)p_alpha;
	t_alpha_stride = p_alpha_stride;

	//return;

	for(uint4 y = p_height; y > 0; --y, t_src_ptr += t_src_stride, t_alpha_ptr += t_alpha_stride)
		for(uint4 x = 0; x < p_width; ++x)
		{
			uint4 s = t_src_ptr[x];
			uint1 a = t_alpha_ptr[x];
			if (a == 0)
				t_src_ptr[x] = 0;
			else if (a == 255)
				t_src_ptr[x] = (s & 0xFFFFFF) | 0xFF000000;
			else
			{
				uint4 u, v;
				u = ((s & 0xff00ff) * a) + 0x800080;
				u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
				v = ((s & 0x00ff00) * a) + 0x8000;
				v = ((v + ((v >> 8) & 0x00ff00)) >> 8) & 0x00ff00;
				t_src_ptr[x] = (u + v) | (a << 24);
			}
		}
}

void surface_merge_with_alpha_non_pre(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint1 *t_alpha_ptr;
	uint4 t_alpha_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	t_alpha_ptr = (uint1 *)p_alpha;
	t_alpha_stride = p_alpha_stride;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_alpha_ptr += t_alpha_stride)
		for(uint4 x = 0; x < p_width; ++x)
			t_pixel_ptr[x] = (t_pixel_ptr[x] & 0xFFFFFF) | (t_alpha_ptr[x] << 24);
}

void surface_extract_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint1 *t_alpha_ptr;
	uint4 t_alpha_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	t_alpha_ptr = (uint1 *)p_alpha;
	t_alpha_stride = p_alpha_stride;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_alpha_ptr += t_alpha_stride)
		for(uint4 x = 0; x < p_width; ++x)
			t_alpha_ptr[x] = t_pixel_ptr[x] >> 24;
}

void surface_merge_with_alpha_and_mask(void *p_pixels, uint4 p_pixel_stride, void *p_src, uint4 p_src_stride, void *p_alpha, uint4 p_alpha_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint4 *t_src_ptr;
	uint4 t_src_stride;
	
	uint1 *t_mask_ptr;
	uint4 t_mask_stride;

	uint1 *t_alpha_ptr;
	uint4 t_alpha_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	t_src_ptr = (uint4 *)p_src;
	t_src_stride = p_src_stride >> 2;

	t_mask_ptr = (uint1 *)p_mask;
	t_mask_stride = p_mask_stride;

	t_alpha_ptr = (uint1 *)p_alpha;
	t_alpha_stride = p_alpha_stride;

	//return;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_src_ptr += t_src_stride, t_alpha_ptr += t_alpha_stride, t_mask_ptr += t_mask_stride)
	{
		uint4 t_byte, t_bit;
		uint1 *t_bytes;

		t_bytes = t_mask_ptr;
		t_bit = 0x80 >> p_offset;
		t_byte = *t_bytes++;

		for(uint4 x = 0; x < p_width; ++x)
		{
			if ((t_byte & t_bit) != 0)
			{
				uint4 s = t_src_ptr[x];
				uint1 a = t_alpha_ptr[x];
				uint4 u, v;

				if (a == 0)
					t_pixel_ptr[x] = 0;
				else if (a == 255)
					t_pixel_ptr[x] = (s & 0xffffff) | 0xff000000;
				else
				{
					u = ((s & 0xff00ff) * a) + 0x800080;
					u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
					v = ((s & 0x00ff00) * a) + 0x8000;
					v = ((v + ((v >> 8) & 0x00ff00)) >> 8) & 0x00ff00;
					t_pixel_ptr[x] = (u + v) | (a << 24);
				}
			}
			else
				t_pixel_ptr[x] = 0;
			t_bit >>= 1;
			if (!t_bit && x < p_width - 1)
				t_bit = 0x80, t_byte = *t_bytes++;
		}
	}
}

static inline uint32_t packed_divide_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v, w;
	u = ((((x & 0xff0000) << 8) - (x & 0xff0000)) / a) & 0xff0000;
	v = ((((x & 0x00ff00) << 8) - (x & 0x00ff00)) / a) & 0x00ff00;
	w = ((((x & 0x0000ff) << 8) - (x & 0x0000ff)) / a) & 0x0000ff;
	return u | v | w;
}

void surface_unmerge_pre(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride)
		for(uint4 x = 0; x < p_width; ++x)
		{
			uint1 t_alpha;
			t_alpha = t_pixel_ptr[x] >> 24;
			if (t_alpha == 0)
				t_pixel_ptr[x] = 0;
			else if (t_alpha != 255)
				t_pixel_ptr[x] = packed_divide_bounded(t_pixel_ptr[x], t_alpha);
		}
}

void surface_unmerge_pre_checking(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride)
		for(uint4 x = 0; x < p_width; ++x)
		{
			uint1 t_alpha;
			t_alpha = t_pixel_ptr[x] >> 24;
			if (t_alpha == 0)
				t_pixel_ptr[x] = 0;
			else if (t_alpha != 255)
			{
				uint4 t_brokenbits = 0;
				if ((t_pixel_ptr[x] & 0xFF) > t_alpha)
					t_brokenbits |= 0xFF;
				if (((t_pixel_ptr[x] >> 8) & 0xFF) > t_alpha)
					t_brokenbits |= 0xFF00;
				if (((t_pixel_ptr[x] >> 16) & 0xFF) > t_alpha)
					t_brokenbits |= 0xFF0000;
				t_pixel_ptr[x] = packed_divide_bounded(t_pixel_ptr[x], t_alpha) | t_brokenbits;
			}
		}
}

void surface_mask_flush_to_alpha_base(void *p_alpha_ptr, uint4 p_alpha_stride, void * p_mask_ptr, uint4 p_mask_stride, uint4 sw, uint4 sh, uint1 p_value)
{
	
	uint4 t_pixel_stride ;
	uint4 t_mask_stride ;
	
	uint1 * t_pixel_ptr ;
	uint1 * t_mask_ptr ;
	
	uint4 p_w, p_h ;
	
	t_pixel_stride = p_alpha_stride ;
	t_mask_stride = p_mask_stride ;

	p_w = sw ;
	p_h = sh ;

	t_pixel_ptr = (uint1 *)p_alpha_ptr ;
	t_mask_ptr = (uint1 *)p_mask_ptr ;
	
	for(uint4 y = p_h; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_mask_ptr += t_mask_stride)
	{
		uint4 x;

		uint1 *t_pixels = t_pixel_ptr;
		uint1 *t_mskels = t_mask_ptr;

		for(x = p_w; x >= 8; x -= 8)
		{
			uint1 b = *t_mskels;
			*t_mskels++ = 0;

			if ((b & (1 << 7)) != 0) t_pixels[0] = p_value;
			if ((b & (1 << 6)) != 0) t_pixels[1] = p_value;
			if ((b & (1 << 5)) != 0) t_pixels[2] = p_value;
			if ((b & (1 << 4)) != 0) t_pixels[3] = p_value;
			if ((b & (1 << 3)) != 0) t_pixels[4] = p_value;
			if ((b & (1 << 2)) != 0) t_pixels[5] = p_value;
			if ((b & (1 << 1)) != 0) t_pixels[6] = p_value;
			if ((b & (1 << 0)) != 0) t_pixels[7] = p_value;

			t_pixels += 8;
		}

		if (x == 0)
			continue;

		uint1 b = *t_mskels;
		*t_mskels = 0;

		switch(7 - x)
		{
		case 0: if ((b & (1 << 1)) != 0) t_pixels[6] = p_value;
		case 1: if ((b & (1 << 2)) != 0) t_pixels[5] = p_value;
		case 2:	if ((b & (1 << 3)) != 0) t_pixels[4] = p_value;
		case 3: if ((b & (1 << 4)) != 0) t_pixels[3] = p_value;
		case 4: if ((b & (1 << 5)) != 0) t_pixels[2] = p_value;
		case 5: if ((b & (1 << 6)) != 0) t_pixels[1] = p_value;
		case 6: if ((b & (1 << 7)) != 0) t_pixels[0] = p_value;
		default:
			break;
		}
	}
	
	
}

void surface_mask_flush_to_alpha(void *p_alpha_ptr, uint4 p_alpha_stride, void * p_mask_ptr, uint4 p_mask_stride, uint4 sw, uint4 sh)
{
	surface_mask_flush_to_alpha_base( p_alpha_ptr, p_alpha_stride, p_mask_ptr, p_mask_stride, sw, sh, 0xff );
}

void surface_mask_flush_to_alpha_inverted(void *p_alpha_ptr, uint4 p_alpha_stride, void * p_mask_ptr, uint4 p_mask_stride, uint4 sw, uint4 sh)
{
	surface_mask_flush_to_alpha_base( p_alpha_ptr, p_alpha_stride, p_mask_ptr, p_mask_stride, sw, sh, 0x00 );
}
