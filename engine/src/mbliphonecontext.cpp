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

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "image.h"
#include "util.h"
#include "dispatch.h"
#include "paint.h"
#include "path.h"
#include "pathprivate.h"
#include "bitmapeffect.h"
#include "globals.h"
#include "region.h"

#include <CoreGraphics/CoreGraphics.h>

#include "mbldc.h"

#include "mbliphonecontext.h"

////////////////////////////////////////////////////////////////////////////////

extern void surface_merge(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height);
extern void surface_merge_with_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);
extern void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
extern void surface_merge_with_alpha_non_pre(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
extern void surface_merge_with_alpha_and_mask(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);
extern void surface_extract_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
extern void surface_unmerge_pre(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height);

extern void surface_combine_blendSrcOver_masked(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity);
extern void surface_combine_blendSrcOver_solid(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity);

typedef void (*surface_combiner_t)(void *p_dst, int4 p_dst_stride, const void *p_src, uint4 p_src_stride, uint4 p_width, uint4 p_height, uint1 p_opacity);
extern surface_combiner_t s_surface_combiners[];
extern surface_combiner_t s_surface_combiners_nda[];

extern void iphone_font_draw_text(void *font, CGContextRef context, CGFloat x, CGFloat y, const char *s, uint4 l, bool p_is_unicode);

////////////////////////////////////////////////////////////////////////////////

MCIPhoneContext::MCIPhoneContext(MCMobileBitmap *p_target, MCBitmap *p_external_mask, bool p_foreign, bool p_alpha)
{
	m_layers = nil;
	m_fill . style = FillSolid;
	m_fill . color = getwhite();
	m_fill . pattern = nil;
	m_background = getblack();
	m_font = nil;
	m_quality = QUALITY_DEFAULT;
	m_gradient_fill = nil;
	
	m_target = p_target;
	m_target_mask = p_external_mask;
	m_target_foreign = p_foreign;
	m_target_opaque = !p_alpha;
	m_target_mono = p_target -> is_mono;
	
	m_update_fill_state = false;
	m_update_stroke_state = false;
	
	m_path_is_native = false;
	
	m_stroke . style = LineSolid;
	m_stroke . width = 0;
	m_stroke . cap = CapButt;
	m_stroke . join = JoinMiter;
	m_stroke . miter_limit = 10.0;
	m_stroke . dash . length = 0;
	m_stroke . dash . data = nil;
	
	m_path_commands = nil;
	m_path_command_count = 0;
	m_path_command_capacity = 0;
	m_path_data = nil;
	m_path_data_count = 0;
	m_path_data_capacity = 0;
	
	layer_push_bitmap(p_target, !p_alpha);
}

MCIPhoneContext::~MCIPhoneContext(void)
{
	while(m_layers != nil)
		layer_pop(true);
	
	MCMemoryDeleteArray(m_path_commands);
	MCMemoryDeleteArray(m_path_data);
	
	if (m_stroke . dash . data != nil)
		delete[] m_stroke . dash . data;
			   
	if (!m_target_foreign)
	{
		free(m_target -> data);
		delete m_target;
	}
}

MCContextType MCIPhoneContext::gettype(void) const
{
	return CONTEXT_TYPE_SCREEN;
}

////////////////////////////////////////////////////////////////////////////////

void MCIPhoneContext::begin(bool p_overlap)
{
	// MW-2012-09-18: [[ Bug ]] Make sure we create a new layer if an ink is set
	//   on a layer.
	if (!p_overlap && m_layers -> opacity == 255 && m_quality == QUALITY_DEFAULT &&
			(m_layers -> function == GXcopy || m_layers -> function == GXblendSrcOver))
	{
		m_layers -> nesting += 1;
		return;
	}
	
	Layer *t_new_layer;
	t_new_layer = layer_push_new(m_layers -> clip, false);
	if (t_new_layer == nil)
	{
		m_layers -> nesting += 1;
		return;
	}
}

bool MCIPhoneContext::begin_with_effects(MCBitmapEffectsRef p_effects, const MCRectangle& p_shape)
{
	// First compute what region of the shape is required to correctly render
	// the full clip of the current layer.
	MCRectangle t_layer_clip;
	MCBitmapEffectsComputeClip(p_effects, p_shape, getclip(), t_layer_clip);
	
	if (t_layer_clip . width == 0 || t_layer_clip . height == 0)
		return false;
	
	// Create the new layer
	Layer *t_new_layer;
	t_new_layer = layer_push_new(t_layer_clip, false);
	if (t_new_layer == nil)
	{
		m_layers -> nesting += 1;
		return true;
	}
	
	// Set the effect parameters
	t_new_layer -> effects = p_effects;
	t_new_layer -> effects_shape = p_shape;
	
	return true;
}

void MCIPhoneContext::end(void)
{
	Layer *t_src_layer, *t_dst_layer;
	
	t_src_layer = m_layers;
	t_dst_layer = t_src_layer -> parent;
	
	if (t_src_layer -> nesting > 0)
	{
		t_src_layer -> nesting -= 1;
		return;
	}
	
	void *t_src_ptr;
	uint4 t_src_stride;
	t_src_ptr = (char *)t_src_layer -> data;
	t_src_stride = t_src_layer -> stride;
	
	void *t_dst_ptr;
	uint4 t_dst_stride;
	t_dst_ptr = t_dst_layer -> data;
	t_dst_stride = t_dst_layer -> stride;
	
	// MW-2009-06-11: [[ Bitmap Effects ]] If we have effects to apply, hand off layer
	//   compositing.
	if (t_src_layer -> effects == NULL)
	{
		t_dst_ptr = (uint1 *)t_dst_ptr + (t_src_layer -> origin . y - t_dst_layer -> origin . y) * t_dst_stride + (t_src_layer -> origin . x - t_dst_layer -> origin . x) * 4;
		
		surface_combiner_t t_combiner;
		t_combiner = (t_dst_layer -> parent != nil || !m_target_opaque) ? s_surface_combiners[t_dst_layer -> function] : s_surface_combiners_nda[t_dst_layer -> function];
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
			t_dst . has_alpha = (t_dst_layer -> parent != nil || !m_target_opaque);
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
			t_combiner = (t_dst_layer -> parent != nil || !m_target_opaque) ? s_surface_combiners[t_dst_layer -> function] : s_surface_combiners_nda[t_dst_layer -> function];
			t_combiner(
					   (uint1 *)t_dst_ptr + t_dst_stride * (t_dst_layer -> clip . y - t_dst_layer -> origin . y) + (t_dst_layer -> clip . x - t_dst_layer -> origin . x) * 4, t_dst_stride,
					   t_tmp_bits, t_tmp_stride, t_dst_layer -> clip . width, t_dst_layer -> clip . height, t_dst_layer -> opacity);
			free(t_tmp_bits);
		}		
	}
	
	layer_pop(true);
	
	m_update_fill_state = true;
	m_update_stroke_state = true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCIPhoneContext::changeopaque(bool p_new_value)
{
	return true;
}

void MCIPhoneContext::setclip(const MCRectangle& p_clip)
{
	if (!MCU_equal_rect(p_clip, m_layers -> clip))
	{
		m_layers -> clip = p_clip;
		m_layers -> update_geometry_state = true;
	}
}

const MCRectangle& MCIPhoneContext::getclip(void) const
{
	return m_layers -> clip;
}

void MCIPhoneContext::clearclip(void)
{
	m_layers -> update_geometry_state = true;
	MCU_set_rect(m_layers -> clip, m_layers -> origin . x, m_layers -> origin . y, m_layers -> width, m_layers -> height);
}

void MCIPhoneContext::setorigin(int2 x, int2 y)
{
	m_layers -> origin . x = x;
	m_layers -> origin . y = y;
	m_layers -> update_geometry_state = true;
}

void MCIPhoneContext::clearorigin(void)
{
}

void MCIPhoneContext::setquality(uint1 p_quality)
{
	m_quality = p_quality;
}

void MCIPhoneContext::setgradient(MCGradientFill* p_fill)
{
	m_gradient_fill = p_fill;
	m_update_fill_state = true;
}

void MCIPhoneContext::setfunction(uint1 p_function)
{
	m_layers -> function = p_function;
}

uint1 MCIPhoneContext::getfunction(void)
{
	return m_layers -> function;
}

void MCIPhoneContext::setopacity(uint1 p_opacity)
{
	m_layers -> opacity = p_opacity;
}

uint1 MCIPhoneContext::getopacity(void)
{
	return m_layers -> opacity;
}

////////////////////////////////////////////////////////////////////////////////

void MCIPhoneContext::setforeground(const MCColor& c)
{
	m_fill . color = c;
	m_update_fill_state = true;
}

void MCIPhoneContext::setbackground(const MCColor& c)
{
	m_background = c;
	m_update_fill_state = true;
}

void MCIPhoneContext::setdashes(uint2 p_offset, const uint1 *p_dashes, uint2 p_length)
{
	delete[] m_stroke . dash . data;
	m_stroke . dash . data = new uint4[p_length + 2];
	
	bool t_on;
	uint2 t_start;
	
	t_start = 0;
	t_on = true;
	
	while(p_offset > 0 && p_offset >= p_dashes[t_start])
	{
		p_offset -= p_dashes[t_start++];
		t_start %= p_length;
		t_on = !t_on;
	}
	
	uint2 t_current;
	t_current = 0;
	
	m_stroke . dash . length = p_length;
	
	if (!t_on)
	{
		m_stroke . dash . data[t_current++] = 0;
		m_stroke . dash . length += 1;
	}
	
	m_stroke . dash . data[t_current++] = p_dashes[t_start++] - p_offset;
	
	for(uint4 t_index = 1; t_index < p_length; ++t_index)
	{
		m_stroke . dash . data[t_current++] = p_dashes[t_start++];
		t_start %= p_length;
	}
	
	if (p_offset != 0)
	{
		m_stroke . dash . data[t_current++] = p_offset;
		m_stroke . dash . length++;
	}
	
	m_update_stroke_state = true;
}

void MCIPhoneContext::setfillstyle(uint2 style, Pixmap p, int2 x, int2 y)
{
	if (style != FillTiled || p != nil)
	{
		m_fill . style = style;
		m_fill . pattern = p;
		m_fill . origin . x = x;
		m_fill . origin . y = y;
	}
	else
	{
		m_fill . style = FillSolid;
		m_fill . pattern = p;
		m_fill . origin . x = 0;
		m_fill . origin . y = 0;
	}
	
	m_update_fill_state = true;
}

void MCIPhoneContext::getfillstyle(uint2& style, Pixmap& p, int2& x, int2& y)
{
	style = m_fill . style;
	p = m_fill . pattern;
	x = m_fill . origin . x;
	y = m_fill . origin . y;
}

void MCIPhoneContext::setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle)
{
	m_stroke . style = linestyle;
	m_stroke . width = linesize;
	m_stroke . cap = capstyle;
	m_stroke . join = joinstyle;
	
	m_update_fill_state = true;
}

void MCIPhoneContext::setmiterlimit(real8 p_limit)
{
	m_stroke . miter_limit = p_limit;
	
	m_update_stroke_state = true;
}

////////////////////////////////////////////////////////////////////////////////

void MCIPhoneContext::drawline(int2 x1, int2 y1, int2 x2, int2 y2)
{
	path_begin_stroke();
	path_move_to(x1, y1);
	path_line_to(x2, y2);
	path_end_stroke();
}

void MCIPhoneContext::drawlines(MCPoint *points, uint2 npoints, bool p_closed)
{
	path_begin_stroke();
	path_move_to(points[0] . x, points[0] . y);
	for(uint32_t i = 1; i < npoints; i++)
		path_line_to(points[i] . x, points[i] . y);
	path_end_stroke();
}

void MCIPhoneContext::drawsegments(MCSegment *segments, uint2 nsegs)
{
	path_begin_stroke();
	for(uint32_t i = 0; i < nsegs; i++)
	{
		path_move_to(segments[i] . x1, segments[i] . y1);
		path_line_to(segments[i] . x2, segments[i] . y2);
	}
	path_end_stroke();
}

void MCIPhoneContext::drawtext(int2 x, int2 y, const char *s, uint2 length, MCFontStruct *f, Boolean image, bool p_unicode_override)
{
	sync_geometry_state();
	CGContextSetShouldAntialias(m_layers -> context, true);
	
	if (image)
	{
		int32_t t_width;
		t_width = textwidth(f, s, length, p_unicode_override);
		
		CGContextSetRGBFillColor(m_layers -> context, m_background . red / 65536.0, m_background . green / 65536.0, m_background . blue / 65536.0, 1.0);
		CGContextFillRect(m_layers -> context, CGRectMake(x, y - f -> ascent, t_width, f -> ascent + f -> descent));
		m_update_fill_state = true;
	}
	
	sync_fill_state();
	CGContextSetTextDrawingMode(m_layers -> context, kCGTextFill);
	
	// MW-2011-09-29: [[ Bug 9760 ]] Previously y was adjusted by +1, this doesn't seem to be necessary
	//   any more... Probably due to changes in transform of context...
	iphone_font_draw_text(f -> fid, m_layers -> context, x, y, s, length, p_unicode_override || f -> unicode);
	
	CGContextSetShouldAntialias(m_layers -> context, false);
}

void MCIPhoneContext::drawrect(const MCRectangle& rect)
{
	path_begin_stroke();
	path_rect(rect, true);
	path_end_stroke();
}

void MCIPhoneContext::fillrect(const MCRectangle& rect)
{
	path_begin_fill();
	path_rect(rect, false);
	path_end_fill();
}

void MCIPhoneContext::fillrects(MCRectangle *rects, uint2 nrects)
{
	path_begin_fill();
	for(uint32_t i = 0; i < nrects; i++)
		path_rect(rects[i], false);
	path_end_fill();
}

void MCIPhoneContext::fillpolygon(MCPoint *points, uint2 npoints)
{
	path_begin_fill();
	path_move_to(points[0] . x, points[0] . y);
	for(uint32_t i = 1; i < npoints; i++)
		path_line_to(points[i] . x, points[i] . y);
	path_end_fill();
}

void MCIPhoneContext::drawroundrect(const MCRectangle& rect, uint2 radius)
{
	path_begin_stroke();
	path_round_rect(rect, radius, true);
	path_end_stroke();
}

void MCIPhoneContext::fillroundrect(const MCRectangle& rect, uint2 radius)
{
	path_begin_fill();
	path_round_rect(rect, radius, false);
	path_end_fill();
}

void MCIPhoneContext::drawarc(const MCRectangle& rect, uint2 start, uint2 angle)
{
	path_begin_stroke();
	path_arc(rect, start, angle, true);
	path_end_stroke();
}

void MCIPhoneContext::drawsegment(const MCRectangle& rect, uint2 start, uint2 angle)
{
	path_begin_stroke();
	path_segment(rect, start, angle, true);
	path_end_stroke();
}

void MCIPhoneContext::fillarc(const MCRectangle& rect, uint2 start, uint2 angle)
{
	path_begin_fill();
	path_segment(rect, start, angle, false);
	path_end_fill();
}

void MCIPhoneContext::drawpath(MCPath *path)
{
	path_stroke(path);
}

void MCIPhoneContext::fillpath(MCPath *path, bool p_evenodd)
{
	path_fill(path, p_evenodd);
}

void MCIPhoneContext::drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect)
{
}

void MCIPhoneContext::draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty,
			 const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length,
			 const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect)
{
}

void MCIPhoneContext::drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy)
{
	MCImageBitmap *t_bits;
	t_bits = p_image . bitmap;
	
	if (t_bits == NULL)
		return;
	
	MCRectangle t_clip, t_dr;
	t_clip = m_layers -> clip;
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
	int32_t t_dst_stride;
	t_dst_ptr = (char *)m_layers -> data;
	t_dst_stride = m_layers -> stride;
	t_dst_ptr = (uint4 *)t_dst_ptr + t_dst_stride * (dy - m_layers -> origin . y) / 4 + (dx - m_layers -> origin . x);
	
	// MW-2011-09-22: Special case for alpha-blended/non-masked image with GXcopy/GXblendSrcOver.
	if (m_layers -> function == GXcopy || m_layers -> function == GXblendSrcOver)
	{
		void *t_src_ptr;
		uint4 t_src_stride;
		t_src_stride = t_bits -> stride;
		t_src_ptr = (uint1 *)(t_bits -> data) + sy * t_src_stride + sx * 4;
		
		if (MCImageBitmapHasTransparency(t_bits))
		{
			surface_combine_blendSrcOver_masked(t_dst_ptr, t_dst_stride, t_src_ptr, t_src_stride, sw, sh, m_layers -> opacity);
			return;
		}
		
		surface_combine_blendSrcOver_solid(t_dst_ptr, t_dst_stride, t_src_ptr, t_src_stride, sw, sh, m_layers -> opacity);		
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
	t_combiner = m_layers -> parent != nil || !m_target_opaque ? s_surface_combiners[m_layers -> function] : s_surface_combiners_nda[m_layers -> function];
	
	t_combiner(t_dst_ptr, t_dst_stride, t_pixel_ptr, t_pixel_stride, sw, sh, m_layers -> opacity);
	
	free(t_pixel_ptr);
}

void MCIPhoneContext::drawlink(const char *link, const MCRectangle& region)
{
	// This is a no-op as links are irrelevant to rasterized portions. The print
	// system will iterate through any links and render them directly.
}

int4 MCIPhoneContext::textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override)
{
	return MCscreen -> textwidth(f, s, l, p_unicode_override);
}

void MCIPhoneContext::applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height)
{
}

void MCIPhoneContext::drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_info)
{
}

void MCIPhoneContext::copyarea(Drawable p_src, uint4 p_dx, uint4 p_dy, uint4 p_sx, uint4 p_sy, uint4 p_sw, uint4 p_sh)
{
}

void MCIPhoneContext::combine(Pixmap p_src, int4 p_dx, int4 p_dy, int4 p_sx, int4 p_sy, uint4 p_sw, uint4 p_sh)
{
}

MCBitmap *MCIPhoneContext::lock(void)
{
	MCBitmap *t_bitmap;
	t_bitmap = new MCBitmap;
	memset(t_bitmap, 0, sizeof(MCBitmap));
	t_bitmap -> width = m_layers -> width;
	t_bitmap -> height = m_layers -> height;
	t_bitmap -> depth = 32;
	t_bitmap -> bits_per_pixel = 32;
	t_bitmap -> bytes_per_line = m_layers -> stride;
	t_bitmap -> data = (char *)m_layers -> data;
	return t_bitmap;
	
}

void MCIPhoneContext::unlock(MCBitmap *p_bitmap)
{
	delete p_bitmap;
}

MCRegionRef MCIPhoneContext::computemaskregion(void)
{
	MCRegionRef t_region;
	MCRegionCreate(t_region);
	MCRegionSetRect(t_region, getclip());
	return t_region;
}

void MCIPhoneContext::clear(const MCRectangle *rect)
{
}

uint2 MCIPhoneContext::getdepth(void) const
{
	return m_target_mono ? 1 : 32;
}

const MCColor& MCIPhoneContext::getblack(void) const
{
	return MCscreen -> black_pixel;
}

const MCColor& MCIPhoneContext::getwhite(void) const
{
	return MCscreen -> white_pixel;
}

const MCColor& MCIPhoneContext::getgray(void) const
{
	return MCscreen -> gray_pixel;
}

const MCColor& MCIPhoneContext::getbg(void) const
{
	return MCscreen -> background_pixel;
}

void MCIPhoneContext::setprintmode(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCIPhoneContext::path_begin_fill(void)
{
	if (m_quality == QUALITY_DEFAULT)
		m_path_is_native = true;
	else
		m_path_is_native = false;
	
	if (m_path_is_native)
	{
		sync_geometry_state();
		sync_fill_state();
	}
}

void MCIPhoneContext::path_end_fill(void)
{
	if (m_path_is_native)
		CGContextFillPath(m_layers -> context);
	else
	{
		if (path_do_alloc(1, 0))
		{	
			m_path_commands[m_path_command_count++] = PATH_COMMAND_END;
			
			MCCombiner *t_combiner = combiner_lock();
			if (t_combiner != NULL)
			{
				MCPath::fill(m_path_commands, m_path_data, t_combiner, m_layers -> clip, false);
				combiner_unlock(t_combiner);
			}
		}
		
		m_path_command_count = 0;
		m_path_data_count = 0;
	}
}

void MCIPhoneContext::path_begin_stroke(void)
{
	if (m_quality == QUALITY_DEFAULT)
		m_path_is_native = true;
	else
		m_path_is_native = false;
	
	if (m_path_is_native)
	{
		sync_geometry_state();
		sync_fill_state();
		sync_stroke_state();
	}
}

void MCIPhoneContext::path_end_stroke(void)
{
	if (m_path_is_native)
		CGContextStrokePath(m_layers -> context);
	else
	{
		if (path_do_alloc(1, 0))
		{	
			m_path_commands[m_path_command_count++] = PATH_COMMAND_END;
			
			MCCombiner *t_combiner = combiner_lock();
			if (t_combiner != NULL)
			{
				uint32_t t_old_width;
				t_old_width = m_stroke . width;
				m_stroke . width *= 256;
				if (m_stroke . width == 0)
					m_stroke . width = 256;
				MCPath::stroke(m_path_commands, m_path_data, t_combiner, m_layers -> clip, &m_stroke);
				m_stroke. width = t_old_width;
				combiner_unlock(t_combiner);
			}
		}
		
		m_path_command_count = 0;
		m_path_data_count = 0;
	}
}

//////////

void MCIPhoneContext::path_move_to(int32_t x, int32_t y)
{
	path_do_move_to(x + 0.5, y + 0.5);
}

void MCIPhoneContext::path_line_to(int32_t x, int32_t y)
{
	path_do_line_to(x + 0.5, y + 0.5);
}

void MCIPhoneContext::path_rect(const MCRectangle& r, bool p_outline)
{
	CGFloat x, y, w, h;
	x = r . x;
	y = r . y;
	w = r . width;
	h = r . height;
	if (p_outline)
		x += 0.5, y += 0.5, w -= 1.0, h -= 1.0;
	
	path_do_move_to(x, y);
	path_do_line_to(x + w, y);
	path_do_line_to(x + w, y + h);
	path_do_line_to(x, y + h);
	path_do_close();
}

void MCIPhoneContext::path_round_rect(const MCRectangle& p_rect, uint32_t p_radius, bool p_outline)
{
	const CGFloat k = 0.552284749830793453f;
	
	CGFloat l, r, t, b;
	l = p_rect . x;
	t = p_rect . y;
	r = p_rect . x + p_rect . width;
	b = p_rect . y + p_rect . height;
	if (p_outline)
		l += 0.5, t += 0.5, r -= 0.5, b -= 0.5;
	
	// MW-2010-12-18: [[ Bug 9249 ]] Radius is actually diameter.
	CGFloat hr, vr;
	hr = MCMin((CGFloat)((r - l) / 2.0), (CGFloat)p_radius / 2);
	vr = MCMin((CGFloat)((b - t) / 2.0), (CGFloat)p_radius / 2);
	
	CGFloat hk, vk;
	hk = hr * k;
	vk = vr * k;
	
	if (hk == 0.0 && vk == 0.0)
	{
		path_rect(p_rect, p_outline);
		return;
	}
	
	l += hr;
	t += vr;
	r -= hr;
	b -= vr;
	
	path_do_move_to(r, t - vr);
	path_do_cubic_to(r + hk, t - vr, r + hr, t - vk, r + hr, t);
	path_do_line_to(r + hr, b);
	path_do_cubic_to(r + hr, b + vk, r + hk, b + vr, r, b + vr);
	path_do_line_to(l, b + vr);
	path_do_cubic_to(l - hk, b + vr, l - hr, b + vk, l - hr, b);
	path_do_line_to(l - hr, t);
	path_do_cubic_to(l - hr, t - vk, l - hk, t - vr, l, t - vr);
	path_do_close();
}

void MCIPhoneContext::path_segment(const MCRectangle& r, uint32_t p_start, uint32_t p_angle, bool p_outline)
{
	path_arc(r, p_start, p_angle, p_outline);
	if (!p_outline)
		path_do_line_to(r . x + r . width / 2.0, r . y + r . height / 2.0);
	else
		path_do_line_to(r . x + r . width / 2.0 + 0.5, r . y + r . height / 2.0 + 0.5);
	path_do_close();
}

void MCIPhoneContext::path_arc(const MCRectangle& p_rect, uint32_t p_start, uint32_t p_angle, bool p_outline)
{
	CGFloat l, r, t, b;
	l = p_rect . x;
	t = p_rect . y;
	r = p_rect . x + p_rect . width;
	b = p_rect . y + p_rect . height;
	if (p_outline)
		l += 0.5, t += 0.5, r -= 0.5, b -= 0.5;
	
	if (p_angle > 360)
		p_angle = 360;
	
	p_start = p_start % 360;
	
	bool t_first;
	t_first = true;
	while(p_angle > 0)
	{
		int32_t t_delta;
		t_delta = MCMin(90 - p_start % 90, p_angle);
		p_angle -= t_delta;
		path_do_arc((r + l) / 2.0, (t + b) / 2.0, (r - l) / 2.0, (b - t) / 2.0, p_start, p_start + t_delta, t_first);
		p_start += t_delta;
		t_first = false;
	}
}

void MCIPhoneContext::path_stroke(MCPath *path)
{
}

void MCIPhoneContext::path_fill(MCPath *path, bool p_even_odd)
{
	MCCombiner *t_combiner = combiner_lock();
	if (t_combiner != NULL)
	{
		path -> fill(t_combiner, m_layers -> clip, p_even_odd);
		combiner_unlock(t_combiner);
	}	
}

//////////

bool MCIPhoneContext::path_do_alloc(uint32_t p_command_count, uint32_t p_data_count)
{
	if (m_path_command_count + p_command_count > m_path_command_capacity)
	{
		p_command_count = (m_path_command_count + p_command_count + 16) & ~15;
		if (!MCMemoryResizeArray(p_command_count, m_path_commands, m_path_command_capacity))
			return false;
	}
	
	if (m_path_data_count + p_data_count > m_path_data_capacity)
	{
		p_data_count = (m_path_data_count + p_data_count + 16) & ~15;
		if (!MCMemoryResizeArray(p_data_count, m_path_data, m_path_data_capacity))
			return false;
	}
	
	return true;
}

void MCIPhoneContext::path_do_move_to(CGFloat x, CGFloat y)
{
	if (m_path_is_native)
		CGContextMoveToPoint(m_layers -> context, x, y);
	else
	{
		if (!path_do_alloc(1, 2))
			return;
		
		m_path_commands[m_path_command_count++] = PATH_COMMAND_MOVE_TO;
		m_path_data[m_path_data_count++] = (int32_t)(x * 256.0);
		m_path_data[m_path_data_count++] = (int32_t)(y * 256.0);
	}
}

void MCIPhoneContext::path_do_line_to(CGFloat x, CGFloat y)
{
	if (m_path_is_native)
		CGContextAddLineToPoint(m_layers -> context, x, y);
	else
	{
		if (!path_do_alloc(1, 2))
			return;
		
		m_path_commands[m_path_command_count++] = PATH_COMMAND_LINE_TO;
		m_path_data[m_path_data_count++] = (int32_t)(x * 256.0);
		m_path_data[m_path_data_count++] = (int32_t)(y * 256.0);
	}
}

void MCIPhoneContext::path_do_cubic_to(CGFloat ax, CGFloat ay, CGFloat bx, CGFloat by, CGFloat ex, CGFloat ey)
{
	if (m_path_is_native)
		CGContextAddCurveToPoint(m_layers -> context, ax, ay, bx, by, ex, ey);
	else
	{
		if (!path_do_alloc(1, 6))
			return;
		
		m_path_commands[m_path_command_count++] = PATH_COMMAND_CUBIC_TO;
		m_path_data[m_path_data_count++] = (int32_t)(ax * 256.0);
		m_path_data[m_path_data_count++] = (int32_t)(ay * 256.0);
		m_path_data[m_path_data_count++] = (int32_t)(bx * 256.0);
		m_path_data[m_path_data_count++] = (int32_t)(by * 256.0);
		m_path_data[m_path_data_count++] = (int32_t)(ex * 256.0);
		m_path_data[m_path_data_count++] = (int32_t)(ey * 256.0);
		
	}
}

void MCIPhoneContext::path_do_arc(CGFloat cx, CGFloat cy, CGFloat hr, CGFloat vr, uint32_t s, uint32_t e, bool first)
{
	CGFloat hk, vk;
	
	if (e - s == 90)
	{
		const CGFloat k = 0.552284749830793453f;
		hk = hr * k;
		vk = vr * k;
	}
	else
	{
		CGFloat h = tanf(M_PI * (e - s) / 720.0);
		hk = 4.0 * h * hr / 3.0;
		vk = 4.0 * h * vr / 3.0;
	}
	
	CGFloat ca, sa;
	CGFloat cb, sb;
	
	ca = cosf(M_PI * s / 180.0);
	sa = sinf(M_PI * s / 180.0);
	
	cb = cosf(M_PI * e / 180.0);
	sb = sinf(M_PI * e / 180.0);
	
	if (first)
		path_do_move_to(cx + hr * ca, cy - vr * sa);

	path_do_cubic_to(cx + hr * ca - hk * sa, cy - vr * sa - vk * ca,
					 cx + hr * cb + hk * sb, cy - vr * sb + vk * cb,
					 cx + hr * cb, cy - vr * sb);
	
}

void MCIPhoneContext::path_do_close(void)
{
	if (m_path_is_native)
		CGContextClosePath(m_layers -> context);
	else
	{
		if (!path_do_alloc(1, 0))
			return;
		
		m_path_commands[m_path_command_count++] = PATH_COMMAND_CLOSE;
	}
}

/////////

void MCIPhoneContext::sync_geometry_state(void)
{
	if (!m_layers -> update_geometry_state)
		return;
	
	CGContextRestoreGState(m_layers -> context);
	CGContextSaveGState(m_layers -> context);
	CGContextTranslateCTM(m_layers -> context, -m_layers -> origin . x, -m_layers -> origin . y);
	CGContextClipToRect(m_layers -> context, CGRectMake(m_layers -> clip . x, m_layers -> clip . y, m_layers -> clip . width, m_layers -> clip . height));
	CGContextSetShouldAntialias(m_layers -> context, false);
	
	m_layers -> update_geometry_state = false;
	m_update_fill_state = true;
	m_update_stroke_state = true;
}

static void FreeData(void *info, const void *data, size_t size)
{
	free(info);
}

// MW-2011-09-11: Not uses CGBitmapContextCreateImage() to make the CGImage which
//   is more efficient as it does a vm_copy of the bits.
static CGImageRef bitmap_to_cgimage(MCMobileBitmap *p_bitmap)
{
	CGColorSpaceRef t_colorspace;
	t_colorspace = CGColorSpaceCreateDeviceRGB();

	CGImageRef t_image;
	t_image = nil;
	
	CGContextRef t_context;
	t_context = CGBitmapContextCreate(p_bitmap -> data, p_bitmap -> width, p_bitmap -> height, 8, p_bitmap -> stride, t_colorspace, kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst);
	if (t_context != nil)
	{
		t_image = CGBitmapContextCreateImage(t_context);
		CGContextRelease(t_context);
	}
		
	CGColorSpaceRelease(t_colorspace);

	return t_image;
}

// MW-2011-09-11: As we no longer invert the bitmap, we must transform the pattern
//   context.
static void CGImagePatternDrawCallback(void *p_info, CGContextRef p_context)
{
	CGImageRef t_image;
	t_image = (CGImageRef)p_info;
	CGContextSaveGState(p_context);
	CGContextTranslateCTM(p_context, 0.0, CGImageGetHeight(t_image));
	CGContextScaleCTM(p_context, 1.0, -1.0);
	CGContextDrawImage(p_context, CGRectMake(0, 0, CGImageGetWidth(t_image), CGImageGetHeight(t_image)), t_image);
	CGContextRestoreGState(p_context);
}

void MCIPhoneContext::sync_fill_state(void)
{
	if (!m_update_fill_state)
		return;
	
	if (m_fill . style != FillTiled)
	{
		CGContextSetRGBFillColor(m_layers -> context, m_fill . color . red / 65536.0, m_fill . color . green / 65536.0, m_fill . color . blue / 65536.0, 1.0);
		CGContextSetRGBStrokeColor(m_layers -> context, m_fill . color . red / 65536.0, m_fill . color . green / 65536.0, m_fill . color . blue / 65536.0, 1.0);
	}
	else
	{
		static CGPatternCallbacks s_pattern_callbacks =
		{
			0,
			CGImagePatternDrawCallback,
			(CGPatternReleaseInfoCallback)CGImageRelease
		};
		
		CGImageRef t_tile_image;
		t_tile_image = bitmap_to_cgimage((MCMobileBitmap *)(m_fill . pattern -> handle . pixmap));
		
		CGAffineTransform t_transform;
		t_transform = CGAffineTransformConcat(CGAffineTransformMakeTranslation(m_fill . origin . x, m_fill . origin . y), CGContextGetCTM(m_layers -> context));
		
		CGPatternRef t_pattern;
		t_pattern = CGPatternCreate(
									t_tile_image,
									CGRectMake(0, 0, CGImageGetWidth(t_tile_image), CGImageGetHeight(t_tile_image)),
									t_transform,
									CGImageGetWidth(t_tile_image), CGImageGetHeight(t_tile_image),
									kCGPatternTilingNoDistortion,
									true,
									&s_pattern_callbacks);
		
		CGColorSpaceRef t_pattern_space;
		t_pattern_space = CGColorSpaceCreatePattern(NULL);
		
		float t_components[1];
		t_components[0] = 1.0f;		
		CGContextSetStrokeColorSpace(m_layers -> context, t_pattern_space);
		CGContextSetStrokePattern(m_layers -> context, t_pattern, t_components);
		CGContextSetFillColorSpace(m_layers -> context, t_pattern_space);
		CGContextSetFillPattern(m_layers -> context, t_pattern, t_components);
		
		CGColorSpaceRelease(t_pattern_space);
		CGPatternRelease(t_pattern);
	}
	
	m_update_fill_state = false;
}

void MCIPhoneContext::sync_stroke_state(void)
{
	if (!m_update_stroke_state)
		return;
	
	CGFloat t_width;
	CGLineCap t_cap;
	CGLineJoin t_join;
	if (m_stroke . width == 0)
	{
		t_width = 1.0;
		t_cap = kCGLineCapButt;
		t_join = kCGLineJoinMiter;
	}
	else
	{
		t_width = m_stroke . width;
		t_cap = m_stroke . cap == CapButt ? kCGLineCapButt :
					m_stroke . cap == CapRound ? kCGLineCapRound :
						kCGLineCapSquare;
		t_join = m_stroke . join == JoinRound ? kCGLineJoinRound :
					m_stroke . join == JoinMiter ? kCGLineJoinMiter :
						 kCGLineJoinBevel;
	}
	
	CGContextSetLineWidth(m_layers -> context, t_width);
	CGContextSetLineCap(m_layers -> context, t_cap);
	CGContextSetLineJoin(m_layers -> context, t_join);
	
	m_update_stroke_state = false;
}

/////////

void MCIPhoneContext::layer_push_bitmap(MCMobileBitmap *p_bitmap, bool p_opaque)
{	
	Layer *t_layer;
	t_layer = new Layer;
	t_layer -> parent = m_layers;
	MCU_set_rect(t_layer -> clip, 0, 0, p_bitmap -> width, p_bitmap -> height);
	t_layer -> origin . x = 0;
	t_layer -> origin . y = 0;
	t_layer -> width = p_bitmap -> width;
	t_layer -> height = p_bitmap -> height;
	t_layer -> stride = p_bitmap -> stride;
	t_layer -> function = GXcopy;
	t_layer -> opacity = 255;
	t_layer -> nesting = 0;
	t_layer -> effects = nil;
	t_layer -> data = p_bitmap -> data;
	t_layer -> foreign_data = true;
	t_layer -> update_geometry_state = false;
	m_layers = t_layer;
											   
	CGColorSpaceRef t_colorspace;
	t_colorspace = CGColorSpaceCreateDeviceRGB();
	t_layer -> context = CGBitmapContextCreate(p_bitmap -> data, p_bitmap -> width, p_bitmap -> height, 8, p_bitmap -> stride, t_colorspace, p_opaque ? kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst : kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
	CGColorSpaceRelease(t_colorspace);
	
	CGContextTranslateCTM(t_layer -> context, 0.0, t_layer -> height);
	CGContextScaleCTM(t_layer -> context, 1.0, -1.0);
	
	CGContextSaveGState(t_layer -> context);
	m_update_fill_state = true;
	m_update_stroke_state = true;
}

MCIPhoneContext::Layer *MCIPhoneContext::layer_push_new(const MCRectangle& p_rect, bool p_opaque)
{
	Layer *t_layer;
	t_layer = new Layer;
	t_layer -> parent = m_layers;
	t_layer -> clip = p_rect;
	t_layer -> origin . x = p_rect . x;
	t_layer -> origin . y = p_rect . y;
	t_layer -> width = p_rect . width;
	t_layer -> height = p_rect . height;
	t_layer -> stride = p_rect . width * sizeof(uint32_t);
	t_layer -> function = GXcopy;
	t_layer -> opacity = 255;
	t_layer -> nesting = 0;
	t_layer -> effects = nil;
	t_layer -> data = malloc(p_rect . width * p_rect . height * sizeof(uint32_t));
	memset(t_layer -> data, 0, p_rect . width * p_rect . height * sizeof(uint32_t));
	t_layer -> foreign_data = false;
	t_layer -> update_geometry_state = true;
	m_layers = t_layer;
	
	CGColorSpaceRef t_colorspace;
	t_colorspace = CGColorSpaceCreateDeviceRGB();
	t_layer -> context = CGBitmapContextCreate(t_layer -> data, t_layer -> width, t_layer -> height, 8, t_layer -> stride, t_colorspace, p_opaque ? kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst : kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
	CGColorSpaceRelease(t_colorspace);
	
	CGContextTranslateCTM(t_layer -> context, 0.0, t_layer -> height);
	CGContextScaleCTM(t_layer -> context, 1.0, -1.0);
	
	CGContextSaveGState(t_layer -> context);
	m_update_fill_state = true;
	m_update_stroke_state = true;
	
	return m_layers;
}


MCIPhoneContext::Layer *MCIPhoneContext::layer_pop(bool p_destroy)
{
	Layer *t_layer;
	t_layer = m_layers;
	
	if (m_layers != nil)
		m_layers = m_layers -> parent;

	if (t_layer != nil && p_destroy)
	{
		layer_destroy(t_layer);
		t_layer = nil;
	}
	
	m_update_fill_state = true;
	m_update_stroke_state = true;
	
	return t_layer;
}

void MCIPhoneContext::layer_destroy(Layer *p_layer)
{
	CGContextRelease(p_layer -> context);
	if (!p_layer -> foreign_data)
		free(p_layer -> data);
	delete p_layer;
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
	
	//return;
	
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
	
	//return;
	
	d = self -> bits;
	s = self -> pattern_bits + self -> pattern_offset;
	
	w = self -> width;
	x = (fx - self -> origin_x) % w;
	
	for(; fx < tx; ++fx)
	{
		d[fx] = packed_bilinear_bounded(s[x] | 0xFF000000, alpha, d[fx], 255 - alpha);
		x++;
		if (x != w)
			continue;
		else
			x = 0;
	}
}

static void pattern_combiner_end(MCCombiner *_self)
{
	MCPatternCombiner *self = (MCPatternCombiner *)_self;
}

MCCombiner *MCIPhoneContext::combiner_lock(void)
{
	static bool s_solid_combiner_initialised = false;
	static MCSolidCombiner s_solid_combiner;
	static bool s_pattern_combiner_initialised = false;
	static MCPatternCombiner s_pattern_combiner;
	
	MCSurfaceCombiner *t_combiner;
	t_combiner = NULL;
	if (m_gradient_fill != NULL && m_gradient_fill->kind != kMCGradientKindNone && m_gradient_fill->ramp_length > 1)
	{
		t_combiner = MCGradientFillCreateCombiner(m_gradient_fill, m_layers -> clip);
	}
	else if (m_fill . style == FillSolid)
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
		
		s_solid_combiner . pixel = 0xff000000 | (m_fill . color . pixel & 0xffffff);
		t_combiner = &s_solid_combiner;
	}
	else if (m_fill . style == FillTiled)
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

		MCMobileBitmap *t_bitmap;
		t_bitmap = (MCMobileBitmap *)(m_fill . pattern -> handle . pixmap);
		
		s_pattern_combiner . pattern_bits = (uint4 *)t_bitmap -> data;
		s_pattern_combiner . pattern_stride = t_bitmap -> stride / 4;
		s_pattern_combiner . width = t_bitmap -> width;
		s_pattern_combiner . height = t_bitmap -> height;
		s_pattern_combiner . origin_x = m_fill . origin . x;
		s_pattern_combiner . origin_y = m_fill . origin . y;
		
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
		t_dst_ptr = m_layers -> data;
		t_dst_stride = m_layers -> stride;
		
		t_combiner -> bits = (uint4 *)t_dst_ptr - m_layers -> origin . y * t_dst_stride / 4 - m_layers -> origin . x;
		t_combiner -> stride = t_dst_stride / 4;
	}
	
	return t_combiner;
}

void MCIPhoneContext::combiner_unlock(MCCombiner *p_combiner)
{
}

//---------------------------------------------------------------------------------