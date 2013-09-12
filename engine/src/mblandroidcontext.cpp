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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "uidc.h"
#include "util.h"
#include "objectstream.h"
#include "bitmapeffect.h"
#include "globals.h"
#include "execpt.h"
#include "paint.h"

#include "mbldc.h"

#include "mblandroidcontext.h"

#include <SkBitmap.h>
#include <SkDevice.h>
#include <SkTypeface.h>
#include <SkDashPathEffect.h>
#include <SkShader.h>

#define SKIA_LAYERING 0

void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
void surface_merge_with_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);

#define SWAP_R_B(p) (((p) & 0xFF00FF00) | (((p) & 0x00FF0000) >> 16) | (((p) & 0x000000FF) << 16))
#define SWAP_R_B_MERGE_ALPHA(p, a) (((a) << 24) | (((p) & 0x00FF0000) >> 16) | ((p) & 0x0000FF00) | (((p) & 0x000000FF) << 16))

static inline uint32_t swap_r_b_merge_alpha(uint32_t s, uint8_t a)
{
	uint4 u, v;
	u = ((s & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
	v = ((s & 0x00ff00) * a) + 0x8000;
	v = ((v + ((v >> 8) & 0x00ff00)) >> 8) & 0x00ff00;
	return (u + v) | (a << 24);
}


MCAndroidContext::MCAndroidContext(uint32_t p_width, uint32_t p_height, bool p_alpha)
{
	init();
	SkBitmap t_bitmap;
	t_bitmap.setConfig(SkBitmap::kARGB_8888_Config, p_width, p_height);
	t_bitmap.setIsOpaque(!p_alpha);
	t_bitmap.allocPixels();
	if (p_alpha)
		t_bitmap.eraseARGB(0x00, 0x00, 0x00, 0x00);
	
	layer_push_bitmap(t_bitmap, 0, 0, !p_alpha);
}

////////////////////////////////////////////////////////////////////////////////

MCAndroidContext::MCAndroidContext(uint32_t p_width, uint32_t p_height, uint32_t p_stride, void *p_pixels, bool p_alpha)
{
	init();
	m_external_bitmap.data = p_pixels;
	m_external_bitmap.width = p_width;
	m_external_bitmap.height = p_height;
	m_external_bitmap.stride = p_stride;

	SkBitmap t_bitmap;
	t_bitmap.setConfig(SkBitmap::kARGB_8888_Config, p_width, p_height, p_stride);
	t_bitmap.setIsOpaque(!p_alpha);
	t_bitmap.setPixels(p_pixels);

	layer_push_bitmap(t_bitmap, 0, 0, !p_alpha);
}

MCAndroidContext::~MCAndroidContext()
{
	while (m_layers != nil)
		layer_pop(true);

	if (m_stroke . dash . data != nil)
		delete [] m_stroke . dash . data;

	if (m_typeface)
		m_typeface->unref();
}

void MCAndroidContext::init()
{
	m_layers = nil;
	m_canvas = nil;

	m_fill . style = FillSolid;
	m_fill . color = getwhite();
	m_fill . pattern = nil;

	m_background = getblack();

	m_gradient_fill = nil;

	m_antialias = false;

	m_update_fill_state = false;
	m_update_stroke_state = false;
	m_update_text_state = false;

	m_stroke . style = LineSolid;
	m_stroke . width = 0;
	m_stroke . cap = CapButt;
	m_stroke . join = JoinMiter;
	m_stroke . miter_limit = 10.0;
	m_stroke . dash . length = 0;
	m_stroke . dash . data = 0;

	m_external_bitmap.data = NULL;
	m_paint.setAntiAlias(true);

	m_typeface = NULL;

	// MW-2012-02-21: [[ FontRefs ]] Initially nil, this stores the font last used in drawtext.
	m_fontstruct = nil;
}

MCContextType MCAndroidContext::gettype() const
{
	return CONTEXT_TYPE_SCREEN;
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidContext::begin(bool p_group)
{
	if (!p_group && m_layers -> opacity == 255 &&
		(m_layers->function == GXblendSrcOver || m_layers->function == GXcopy))
	{
		m_layers -> nesting += 1;
		return;
	}

#if SKIA_LAYERING
	sync_geometry_state();
	sync_fill_state();
	MCRectangle &t_clip = m_layers->clip;
	SkRect t_rect;
	t_rect.iset(t_clip.x, t_clip.y, t_clip.x + t_clip.width, t_clip.y + t_clip.height);
	m_layers->canvas->saveLayerAlpha(&t_rect, m_layers->opacity);
	m_layers->canvas->saveLayer(&t_rect, &m_paint);
#else
	Layer *t_new_layer;
	t_new_layer = layer_push_new(m_layers -> clip, false);
	if (t_new_layer == nil)
	{
		m_layers -> nesting += 1;
		return;
	}
#endif
}

bool MCAndroidContext::begin_with_effects(MCBitmapEffectsRef p_effects, const MCRectangle &p_shape)
{
	MCRectangle t_layer_clip;
	MCBitmapEffectsComputeClip(p_effects, p_shape, getclip(), t_layer_clip);

	if (t_layer_clip . width == 0 || t_layer_clip . height == 0)
		return false;

	Layer *t_new_layer;
	t_new_layer = layer_push_new(t_layer_clip, false);
	if (t_new_layer == nil)
	{
		m_layers -> nesting += 1;
		return true;
	}

	t_new_layer -> effects = p_effects;
	t_new_layer -> effects_shape = p_shape;

	return true;
}

void MCAndroidContext::end()
{
	Layer *t_src_layer, *t_dst_layer;

	t_src_layer = m_layers;
	t_dst_layer = t_src_layer->parent;

	if (t_src_layer->nesting > 0)
	{
		t_src_layer->nesting -= 1;
		return;
	}

	if (t_src_layer->effects == NULL)
	{
#if SKIA_LAYERING
		t_src_layer->canvas->restore();
		t_src_layer->canvas->restore();
		m_update_fill_state = true;
		return;
#else
		sync_layer_geometry_state(t_dst_layer);
		SkPaint t_paint;
		sync_layer_blend_state(&t_paint, t_dst_layer);
		t_dst_layer->canvas->drawBitmap(t_src_layer->canvas->getDevice()->accessBitmap(false), t_src_layer -> origin . x, t_src_layer -> origin . y, &t_paint);
#endif
	}
	else
	{
		// MW-2011-10-04: [[ Bug 9771 ]] Graphic effect applied to bitmap effects not working
		//   correctly. If suitable, make sure we render direct to ensure colorDodge et al.
		//   work right on the bitmap effects.
		
		void *t_tmp_bits = NULL;
		uint32_t t_tmp_stride = 0;

		if (t_dst_layer -> opacity != 255 || (t_dst_layer -> function != GXblendSrcOver && t_dst_layer -> function != GXcopy))
		{
			t_tmp_stride = t_dst_layer->clip.width * 4;
			t_tmp_bits = malloc(t_dst_layer->clip.height * t_tmp_stride);
			memset(t_tmp_bits, 0, t_tmp_stride * t_dst_layer->clip.height);
		}
		else
			t_tmp_bits = NULL, t_tmp_stride = 0;

		MCBitmapEffectLayer t_dst;
		t_dst.bounds = t_dst_layer->clip;
		if (t_tmp_bits == NULL)
		{
			void *t_dst_bits;
			uint32_t t_dst_stride;
			const SkBitmap &t_dst_bitmap = t_dst_layer->canvas->getDevice()->accessBitmap(false);
			t_dst_stride = t_dst_bitmap.rowBytes();
			t_dst_bits = t_dst_bitmap.getPixels();
			t_dst . stride = t_dst_stride;
			t_dst . bits = (uint1 *)t_dst_bits + t_dst_stride * (t_dst_layer -> clip . y - t_dst_layer -> origin . y) + (t_dst_layer -> clip . x - t_dst_layer -> origin . x) * 4;
			t_dst . has_alpha = (t_dst_layer -> parent != nil || !t_dst_bitmap . isOpaque());
		}
		else
		{
			t_dst.bits = t_tmp_bits;
			t_dst.stride = t_tmp_stride;
			t_dst.has_alpha = true;
		}
		
		MCBitmapEffectLayer t_src;
		MCU_set_rect(t_src.bounds, t_src_layer->origin.x, t_src_layer->origin.y, t_src_layer->width, t_src_layer->height);
		const SkBitmap &t_src_bitmap = t_src_layer->canvas->getDevice()->accessBitmap(false);
		t_src.stride = t_src_bitmap.rowBytes();
		t_src.bits = t_src_bitmap.getPixels();

		MCBitmapEffectsRender(t_src_layer->effects, t_src_layer->effects_shape, t_dst, t_src);

		if (t_tmp_bits != nil)
		{
			SkBitmap t_effect_bitmap;
			t_effect_bitmap.setConfig(SkBitmap::kARGB_8888_Config, t_dst_layer->clip.width, t_dst_layer->clip.height, t_tmp_stride);
			t_effect_bitmap.setPixels(t_tmp_bits);

			sync_layer_geometry_state(t_dst_layer);
			SkPaint t_paint;
			sync_layer_blend_state(&t_paint, t_dst_layer);
			t_dst_layer->canvas->drawBitmap(t_effect_bitmap, t_dst_layer->clip.x, t_dst_layer->clip.y, &t_paint);

			free(t_tmp_bits);
		}
	}

	layer_pop(true);

	m_update_fill_state = true;
	m_update_stroke_state = true;
}

////////////////////////////////////////////////////////////////////////////////

MCAndroidContext::Layer *MCAndroidContext::layer_push_bitmap(SkBitmap &p_bitmap, int32_t p_x_origin, int32_t p_y_origin, bool p_opaque)
{
	Layer *t_layer;
	t_layer = new Layer;
	t_layer -> parent = m_layers;
	MCU_set_rect(t_layer -> clip, p_x_origin, p_y_origin, p_bitmap . width(), p_bitmap . height());
	t_layer -> origin . x = p_x_origin;
	t_layer -> origin . y = p_y_origin;
	t_layer -> width = p_bitmap . width();
	t_layer -> height = p_bitmap . height();
	t_layer -> stride = p_bitmap . rowBytes();
	t_layer -> function = GXcopy;
	t_layer -> opacity = 255;
	t_layer -> nesting = 0;
	t_layer -> effects = nil;
	t_layer -> update_geometry_state = true;
	m_layers = t_layer;

	p_bitmap.setIsOpaque(p_opaque);
	t_layer -> canvas = new SkCanvas(p_bitmap);

	m_update_fill_state = true;
	m_update_stroke_state = true;

	return t_layer;
}

MCAndroidContext::Layer *MCAndroidContext::layer_push_new(MCRectangle p_clip, bool p_opaque)
{
	SkBitmap t_bitmap;
	t_bitmap.setConfig(SkBitmap::kARGB_8888_Config, p_clip.width, p_clip.height);
	t_bitmap.setIsOpaque(p_opaque);
	t_bitmap.allocPixels();
	t_bitmap.eraseARGB(0x00, 0x00, 0x00, 0x00);
	return layer_push_bitmap(t_bitmap, p_clip.x, p_clip.y, p_opaque);
}

MCAndroidContext::Layer *MCAndroidContext::layer_pop(bool p_destroy)
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

void MCAndroidContext::layer_destroy(Layer *p_layer)
{
	delete p_layer -> canvas;
	delete p_layer;
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidContext::changeopaque(bool p_value)
{
	return true;
}

void MCAndroidContext::setclip(const MCRectangle& p_clip)
{
	if (!MCU_equal_rect(p_clip, m_layers -> clip))
	{
		m_layers -> clip = p_clip;
		m_layers -> update_geometry_state = true;
	}
}

const MCRectangle& MCAndroidContext::getclip(void) const
{
	return m_layers -> clip;
}

void MCAndroidContext::clearclip(void)
{
	m_layers -> update_geometry_state = true;
	MCU_set_rect(m_layers -> clip, m_layers -> origin . x, m_layers -> origin . y, m_layers -> width, m_layers -> height);
}

void MCAndroidContext::setorigin(int2 x, int2 y)
{
	m_layers -> update_geometry_state = true;
	m_layers -> origin . x = x;
	m_layers -> origin . y = y;
}

void MCAndroidContext::clearorigin(void)
{
}

void MCAndroidContext::setquality(uint1 p_quality)
{
	m_antialias = (p_quality == QUALITY_SMOOTH);
}

void MCAndroidContext::setfunction(uint1 p_function)
{
	m_layers -> function = p_function;
	m_update_fill_state = true;
}

uint1 MCAndroidContext::getfunction(void)
{
	return m_layers -> function;
}

void MCAndroidContext::setopacity(uint1 p_opacity)
{
	m_layers -> opacity = p_opacity;
	m_update_fill_state = true;
}

uint1 MCAndroidContext::getopacity(void)
{
	return m_layers -> opacity;
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidContext::setforeground(const MCColor& c)
{
	m_fill . color = c;
	m_update_fill_state = true;
}

void MCAndroidContext::setbackground(const MCColor& c)
{
	m_background = c;
	m_update_fill_state = true;
}

void MCAndroidContext::setdashes(uint16_t p_offset, const uint8_t *p_dashes, uint16_t p_length)
{
	delete [] m_stroke . dash . data;
	m_stroke . dash . data = new uint32_t[p_length + 2];

	bool t_on;
	uint16_t t_start;

	t_start = 0;
	t_on = true;

	while (p_offset > 0 && p_offset >= p_dashes[t_start])
	{
		p_offset -= p_dashes[t_start++];
		t_start %= p_length;
		t_on = !t_on;
	}

	uint16_t t_current;
	t_current = 0;

	m_stroke . dash . length = p_length;

	if (!t_on)
	{
		m_stroke . dash . data[t_current++] = 0;
		m_stroke . dash . length += 1;
	}

	m_stroke . dash . data[t_current++] = p_dashes[t_start++] - p_offset;

	for (uint32_t t_index = 1; t_index < p_length; t_index++)
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

void MCAndroidContext::setfillstyle(uint2 style, Pixmap p, int2 x, int2 y)
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

void MCAndroidContext::getfillstyle(uint2& style, Pixmap& p, int2& x, int2& y)
{
	style = m_fill . style;
	p = m_fill . pattern;
	x = m_fill . origin . x;
	y = m_fill . origin . y;
}

void MCAndroidContext::setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle)
{
	m_stroke . style = linestyle;
	m_stroke . width = linesize;
	m_stroke . cap = capstyle;
	m_stroke . join = joinstyle;

	m_update_stroke_state = true;
}

void MCAndroidContext::setmiterlimit(real8 p_limit)
{
	m_stroke . miter_limit = p_limit;

	m_update_stroke_state = true;
}

void MCAndroidContext::setgradient(MCGradientFill *p_gradient)
{
	m_gradient_fill = p_gradient;
	m_update_fill_state = true;
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidContext::drawline(int2 x1, int2 y1, int2 x2, int2 y2)
{
	path_begin_stroke();
	path_move_to(x1, y1);
	path_line_to(x2, y2);
	path_end_stroke();
}

// TODO - p_closed
void MCAndroidContext::drawlines(MCPoint *points, uint2 npoints, bool p_closed)
{
	path_begin_stroke();
	path_move_to(points[0].x, points[0].y);
	for (uint32_t i = 1; i < npoints; i++)
		path_line_to(points[i].x, points[i].y);
	path_end_stroke();
}

void MCAndroidContext::drawsegments(MCSegment *segments, uint2 nsegs)
{
	path_begin_stroke();
	for (uint32_t i = 0; i < nsegs; i++)
	{
		path_move_to(segments[i].x1, segments[i].y1);
		path_line_to(segments[i].x2, segments[i].y2);
	}
	path_end_stroke();
}

void MCAndroidContext::drawrect(const MCRectangle& rect)
{
	path_begin_stroke();
	path_rect(rect, true);
	path_end_stroke();
}

void MCAndroidContext::fillrect(const MCRectangle& rect)
{
	path_begin_fill();
	path_rect(rect, false);
	path_end_fill();
}

void MCAndroidContext::fillrects(MCRectangle *rects, uint2 nrects)
{
	path_begin_fill();
	for (uint32_t i = 0; i < nrects; i++)
		path_rect(rects[i], false);
	path_end_fill();
}

void MCAndroidContext::fillpolygon(MCPoint *points, uint2 npoints)
{
	path_begin_fill();
	path_move_to(points[0].x, points[0].y);
	for (uint32_t i = 1; i < npoints; i++)
		path_line_to(points[i].x, points[i].y);
	path_end_fill();
}

void MCAndroidContext::drawroundrect(const MCRectangle& rect, uint2 radius)
{
	path_begin_stroke();
	path_round_rect(rect, radius, true);
	path_end_stroke();
}

void MCAndroidContext::fillroundrect(const MCRectangle& rect, uint2 radius)
{
	path_begin_fill();
	path_round_rect(rect, radius, false);
	path_end_fill();
}

void MCAndroidContext::drawarc(const MCRectangle& rect, uint2 start, uint2 angle)
{
	path_begin_stroke();
	path_arc(rect, start, angle, true);
	path_end_stroke();
}

void MCAndroidContext::drawsegment(const MCRectangle& rect, uint2 start, uint2 angle)
{
	path_begin_stroke();
	path_segment(rect, start, angle, true);
	path_end_stroke();
}

void MCAndroidContext::fillarc(const MCRectangle& rect, uint2 start, uint2 angle)
{
	path_begin_fill();
	path_segment(rect, start, angle, false);
	path_end_fill();
}

void MCAndroidContext::drawpath(MCPath *path)
{
	path_stroke(path);
}

void MCAndroidContext::fillpath(MCPath *path, bool p_evenodd)
{
	path_fill(path, p_evenodd);
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidContext::drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect)
{
}

void MCAndroidContext::draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty,
							   const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length,
							   const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidContext::drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy)
{
	MCImageBitmap *t_bits;
	t_bits = p_image . bitmap;
	
	if (t_bits == nil)
		return;
	
	bool t_opaque;
	t_opaque = !MCImageBitmapHasTransparency(t_bits);
	
	void *t_argb_pixels;
	uint32_t t_argb_stride;
	
	t_argb_stride = sw * 4;
	/* UNCHECKED */ MCMemoryAllocate(t_argb_stride * sh, t_argb_pixels);
	
	if (t_opaque)
		MCImageBitmapCopyRegionToBuffer(t_bits, sx, sy, sw, sh, t_argb_stride, (uint8_t*)t_argb_pixels);
	else
		MCImageBitmapPremultiplyRegion(t_bits, sx, sy, sw, sh, t_argb_stride, (uint32_t*)t_argb_pixels);
	
	// translate to skia byte packing (ARGB -> ABGR)
	uint8_t *t_src_row = (uint8_t*)t_argb_pixels;
	uint32_t t_height = sh;
	while (t_height--)
	{
		uint32_t *t_src_pixel = (uint32_t*)t_src_row;
		uint32_t t_width = sw;
		while (t_width--)
		{
			*t_src_pixel = SWAP_R_B(*t_src_pixel);
			t_src_pixel++;
		}
		t_src_row += t_argb_stride;
	}
	
	SkBitmap t_src_bitmap;
	t_src_bitmap.setConfig(SkBitmap::kARGB_8888_Config, sw, sh, t_argb_stride);
	t_src_bitmap.setPixels(t_argb_pixels);
	t_src_bitmap.setIsOpaque(t_opaque);

	SkPaint t_paint(m_paint);
	sync_geometry_state();
	sync_layer_blend_state(&t_paint, m_layers);

	m_layers->canvas->drawBitmap(t_src_bitmap, dx, dy, &t_paint);

	MCMemoryDeallocate(t_argb_pixels);
}

void MCAndroidContext::drawlink(MCStringRef link, const MCRectangle& region)
{
	// This is a no-op as links are irrelevant to rasterized portions. The print
	// system will iterate through any links and render them directly.
}

////////////////////////////////////////////////////////////////////////////////

MCBitmap *MCAndroidContext::lock(void)
{
	MCBitmap *t_bitmap = NULL;
	MCMemoryNew(t_bitmap);

	t_bitmap->width = m_layers->width;
	t_bitmap->height = m_layers->height;
	t_bitmap->depth = 32;
	t_bitmap->bits_per_pixel = 32;
	t_bitmap->bytes_per_line = m_layers->stride;

	t_bitmap->data = (char*)m_layers->canvas->getDevice()->accessBitmap(false).getPixels();

	return t_bitmap;
}

void MCAndroidContext::unlock(MCBitmap *p_bitmap)
{
	MCMemoryDelete(p_bitmap);
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidContext::setexternalalpha(MCBitmap *p_alpha)
{
	p_alpha = p_alpha;
}

void MCAndroidContext::applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height)
{
	p_mask = p_mask;
}

void MCAndroidContext::copyarea(Drawable p_src, uint4 p_dx, uint4 p_dy, uint4 p_sx, uint4 p_sy, uint4 p_sw, uint4 p_sh)
{
}

void MCAndroidContext::combine(Pixmap p_src, int4 p_dx, int4 p_dy, int4 p_sx, int4 p_sy, uint4 p_sw, uint4 p_sh)
{
}

void MCAndroidContext::drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_info)
{
}

MCRegionRef MCAndroidContext::computemaskregion(void)
{
	return NULL;
}

void MCAndroidContext::clear(const MCRectangle *rect)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidContext::drawtext(int2 x, int2 y, const char *s, uint2 length, MCFontStruct *f, Boolean image, bool p_unicode_override)
{
	// MW-2012-02-21: [[ FontRefs ]] If the font doesn't match the one we used last then make
	//   sure we mark for text state update.
	if (f != m_fontstruct)
	{
		m_fontstruct = f;
		m_update_text_state = true;
	}
	
	sync_geometry_state();
	sync_fill_state();
	sync_text_state();
	m_paint.setStyle(SkPaint::kFill_Style);

	MCExecPoint ep;
	ep.setsvalue(MCString(s, length));

	// MW-2012-02-29: [[ Bug 10038 ]] Source text is in UTF-16 if unicode_override is true.
	if (f->unicode || p_unicode_override)
		ep.utf16toutf8();
	else
		ep.nativetoutf8();

	const MCString &t_utf_string = ep.getsvalue();
	m_layers->canvas->drawText(t_utf_string.getstring(), t_utf_string.getlength(), x, y, m_paint);
}

int4 MCAndroidContext::textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override)
{
	return MCscreen -> textwidth(f, s, l, p_unicode_override);
}

////////////////////////////////////////////////////////////////////////////////

uint2 MCAndroidContext::getdepth(void) const
{
	return 32;
}

const MCColor& MCAndroidContext::getblack(void) const
{
	return MCscreen->black_pixel;
}

const MCColor& MCAndroidContext::getwhite(void) const
{
	return MCscreen->white_pixel;
}

const MCColor& MCAndroidContext::getgray(void) const
{
	return MCscreen->gray_pixel;
}

const MCColor& MCAndroidContext::getbg(void) const
{
	return MCscreen->background_pixel;
}

void MCAndroidContext::setprintmode(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidContext::path_begin_fill(void)
{
	m_path.reset();
	sync_geometry_state();
	sync_fill_state();
	m_paint.setStyle(SkPaint::kFill_Style);
}

void MCAndroidContext::path_end_fill(void)
{
	m_layers->canvas->drawPath(m_path, m_paint);
}

void MCAndroidContext::path_begin_stroke(void)
{
	m_path.reset();
	sync_geometry_state();
	sync_fill_state();
	sync_stroke_state();
	m_paint.setStyle(SkPaint::kStroke_Style);
}

void MCAndroidContext::path_end_stroke(void)
{
	m_layers->canvas->drawPath(m_path, m_paint);
}
	
void MCAndroidContext::path_move_to(int32_t x, int32_t y)
{
	m_path.moveTo(SkIntToScalar(x) + 0.5, SkIntToScalar(y) + 0.5);
}

void MCAndroidContext::path_line_to(int32_t x, int32_t y)
{
	m_path.lineTo(SkIntToScalar(x) + 0.5, SkIntToScalar(y) + 0.5);
}

void MCAndroidContext::path_rect(const MCRectangle& r, bool p_outline)
{
	SkRect t_rect;
	t_rect.iset(r.x, r.y, r.x + r.width, r.y + r.height);
	if (p_outline)
		t_rect.inset(0.5, 0.5);
	m_path.addRect(t_rect);
}

void MCAndroidContext::path_round_rect(const MCRectangle& r, uint32_t radius, bool p_outline)
{
	SkRect t_rect;
	t_rect.iset(r.x, r.y, r.x + r.width, r.y + r.height);
	if (p_outline)
		t_rect.inset(0.5, 0.5);
	// IM-2011-07-18 [[ bz 9623 ]] never assume that a property called 'roundRadius', whose value is referred to
	// everywhere else in the engine as 'radius' actually is the radius and not the diameter.
	m_path.addRoundRect(t_rect, SkIntToScalar(radius) / 2, SkIntToScalar(radius) / 2);
}

void MCAndroidContext::path_segment(const MCRectangle& r, uint32_t start, uint32_t angle, bool p_outline)
{
	path_arc(r, start, angle, p_outline);
	if (!p_outline)
		path_line_to(r.x + r.width / 2.0, r.y + r.height / 2.0);
	else
		path_line_to(r.x + r.width / 2.0 + 0.5, r.y + r.height / 2.0 + 0.5);
	m_path.close();
}

void MCAndroidContext::path_arc(const MCRectangle& r, uint32_t start, uint32_t angle, bool p_outline)
{
	SkRect t_rect;
	t_rect.iset(r.x, r.y, r.x + r.width, r.y + r.height);
	if (p_outline)
		t_rect.inset(0.5, 0.5);
	// invert start angle - counter-clockwise in rev, clockwise in android context
	SkScalar t_start = -SkIntToScalar(start + angle);
	SkScalar t_angle = SkIntToScalar(angle);
	m_path.addArc(t_rect, t_start, t_angle);
}

// TODO - implement path fill/stroke
void MCAndroidContext::path_fill(MCPath *path, bool p_even_odd)
{
}

void MCAndroidContext::path_stroke(MCPath *path)
{
}


////////////////////////////////////////////////////////////////////////////////

void MCAndroidContext::sync_layer_geometry_state(Layer *p_layer)
{
	if (!p_layer->update_geometry_state)
		return;

	p_layer->canvas->resetMatrix();
	p_layer->canvas->translate(SkIntToScalar(-p_layer->origin.x), SkIntToScalar(-p_layer->origin.y));

	SkRect t_rect;
	t_rect.iset(p_layer->clip.x, p_layer->clip.y, p_layer->clip.x + p_layer->clip.width, p_layer->clip.y + p_layer->clip.height);
	p_layer->canvas->clipRect(t_rect, SkRegion::kReplace_Op);

	p_layer->update_geometry_state = false;
}

void MCAndroidContext::sync_geometry_state(void)
{
	sync_layer_geometry_state(m_layers);
}


void MCAndroidContext::sync_layer_blend_state(SkPaint *p_paint, Layer *p_layer)
{
	uint32_t t_first_mode = GXblendClear;
	uint32_t t_last_mode = GXblendExclusion;
	SkXfermode *t_xfermode;

	if (p_layer->function == GXcopy || p_layer->function == GXblendSrcOver)
		t_xfermode = NULL;
	else
		t_xfermode = new MCCombinerXfermode(p_layer->function);

	p_paint->setAlpha(p_layer->opacity);
	p_paint->setXfermode(t_xfermode);
	if (t_xfermode != NULL)
		t_xfermode->unref();
}

void MCAndroidContext::sync_fill_state(void)
{
	if (!m_update_fill_state)
		return;

#if SKIA_LAYERING
	sync_layer_blend_state(&m_paint, m_layers);
#endif

	m_paint.setShader(NULL);
	if (m_gradient_fill != nil)
	{
		SkShader *t_gradient_shader = new MCGradientShader(m_gradient_fill, m_layers->clip);
		m_paint.setShader(t_gradient_shader);
		t_gradient_shader->unref();

	}
	else if (m_fill.pattern != nil)
	{
		uint32_t t_pixstride;
		uint16_t t_pixwidth, t_pixheight, t_pixdepth;
		void *t_pixdata;
		MCscreen->getpixmapgeometry(m_fill.pattern, t_pixwidth, t_pixheight, t_pixdepth);

		MCscreen->lockpixmap(m_fill.pattern, t_pixdata, t_pixstride);
		SkBitmap t_bitmap;
		t_bitmap.setConfig(SkBitmap::kARGB_8888_Config, t_pixwidth, t_pixheight, t_pixstride);
		t_bitmap.allocPixels();
		t_bitmap.setIsOpaque(true);
		for (uint32_t y = 0; y < t_pixheight; y++)
		{
			uint32_t *t_src_ptr = (uint32_t*)((uint8_t*)t_pixdata + t_pixstride * y);
			uint32_t *t_dst_ptr = t_bitmap.getAddr32(0, y);
			for (uint32_t x = 0; x < t_pixwidth; x++)
			{
				*t_dst_ptr++ = SWAP_R_B_MERGE_ALPHA(*t_src_ptr, 0xFF);
				t_src_ptr++;
			}
		}
		MCscreen->unlockpixmap(m_fill.pattern, t_pixdata, t_pixstride);

		SkShader *t_pattern_shader;
		t_pattern_shader = SkShader::CreateBitmapShader(t_bitmap, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
		SkMatrix t_translation;
		t_translation.setTranslate(m_fill.origin.x, m_fill.origin.y);
		t_pattern_shader->setLocalMatrix(t_translation);
		m_paint.setShader(t_pattern_shader);
		t_pattern_shader->unref();
	}
	else
	{
		m_paint.setARGB(0xFF, m_fill.color.red >> 8, m_fill.color.green >> 8, m_fill.color.blue >> 8);
	}

	m_update_fill_state = false;
}

void MCAndroidContext::sync_text_state(void)
{
	if (!m_update_text_state)
		return;

	// Release any typeface we currently have.
	if (m_typeface != nil)
	{
		m_typeface -> unref();
		m_typeface = nil;
	}
	
	// Now fetch the updated details from the fontstuct.
	MCAndroidFont *t_font = (MCAndroidFont*)m_fontstruct->fid;
	m_typeface = t_font -> sk_typeface;
	m_typeface -> ref();
	m_textsize = t_font -> size;
	
	m_paint.setPathEffect(NULL);
	m_paint.setTypeface(m_typeface);
	m_paint.setTextSize(m_textsize);
	m_paint.setAntiAlias(true);
	m_paint.setLCDRenderText(true);

	m_update_text_state = false;
	m_update_stroke_state = true;
}

void MCAndroidContext::sync_stroke_state(void)
{
	if (!m_update_stroke_state)
		return;

	SkScalar t_width;
	SkPaint::Cap t_cap;
	SkPaint::Join t_join;

	if (m_stroke.width == 0)
	{
		t_width = 1.0;
		t_cap = SkPaint::kButt_Cap;
		t_join = SkPaint::kMiter_Join;
	}
	else
	{
		t_width = m_stroke.width;
		switch (m_stroke.cap)
		{
		case CapButt:
			t_cap = SkPaint::kButt_Cap;
			break;
		case CapRound:
			t_cap = SkPaint::kRound_Cap;
			break;
		case CapProjecting:
			t_cap = SkPaint::kSquare_Cap;
			break;
		}
		switch (m_stroke.join)
		{
		case JoinRound:
			t_join = SkPaint::kRound_Join;
			break;
		case JoinMiter:
			t_join = SkPaint::kMiter_Join;
			break;
		case JoinBevel:
			t_join = SkPaint::kBevel_Join;
			break;
		}
	}

	m_paint.setStrokeWidth(t_width);
	m_paint.setStrokeCap(t_cap);
	m_paint.setStrokeJoin(t_join);

	if (m_stroke.style != LineSolid && m_stroke.dash.length > 0)
	{
		SkScalar *t_dashes = NULL;
		MCMemoryNewArray(m_stroke.dash.length, t_dashes);
		if (t_dashes != NULL)
		{
			for (uint32_t i = 0; i < m_stroke.dash.length; i++)
				t_dashes[i] = SkIntToScalar(m_stroke.dash.data[i]);
			SkDashPathEffect *t_dash_effect;
			t_dash_effect = new SkDashPathEffect(t_dashes, m_stroke.dash.length, 0, false);
			m_paint.setPathEffect(t_dash_effect);
			t_dash_effect->unref();
			MCMemoryDeallocate(t_dashes);
		}
	}
	else
	{
		m_paint.setPathEffect(NULL);
	}

	m_paint.setAntiAlias(m_antialias);

	m_update_stroke_state = false;
	m_update_text_state = true;
}

////////////////////////////////////////////////////////////////////////////////

typedef void (*surface_combiner_t)(void *p_dst, int4 p_dst_stride, const void *p_src, uint4 p_src_stride, uint4 p_width, uint4 p_height, uint1 p_opacity);
extern surface_combiner_t s_surface_combiners[];

MCCombinerXfermode::MCCombinerXfermode(uint32_t p_function)
{
	m_function = p_function;
}

bool MCCombinerXfermode::asCoeff(SkXfermode::Coeff *src, SkXfermode::Coeff *dst)
{
	return false;
}

void MCCombinerXfermode::xfer32(SkPMColor dst[], const SkPMColor src[], int count, const SkAlpha aa[])
{
	s_surface_combiners[m_function](dst, count * 4, src, count * 4, count, 1, 0xFF);
}

void MCCombinerXfermode::xfer4444(uint16_t dst[], const SkPMColor src[], int count, const SkAlpha aa[])
{
}

void MCCombinerXfermode::xfer16(uint16_t dst[], const SkPMColor src[], int count, const SkAlpha aa[])
{
}

void MCCombinerXfermode::xferA8(SkAlpha dst[], const SkPMColor src[], int count, const SkAlpha aa[])
{
}

MCCombinerXfermode::MCCombinerXfermode(SkFlattenableReadBuffer& buffer)
        : SkXfermode(buffer)
{
	m_function = buffer.readU32();
}

void MCCombinerXfermode::flatten(SkFlattenableWriteBuffer& buffer)
{
	buffer.write32(m_function);
}

////////////////////////////////////////////////////////////////////////////////

MCGradientShader::MCGradientShader(MCGradientFill *p_gradient, MCRectangle &p_clip)
{
	m_gradient = MCGradientFillCopy(p_gradient);
	m_clip = p_clip;
	m_gradient_combiner = NULL;
	MCMemoryAllocate(p_clip.width, m_mask);
	memset(m_mask, 0xFF, p_clip.width);
}

MCGradientShader::~MCGradientShader()
{
	MCMemoryDeallocate(m_mask);
	if (m_gradient != NULL)
	{
		MCMemoryDeallocate(m_gradient->ramp);
		MCMemoryDeallocate(m_gradient);
	}
}

bool MCGradientShader::setContext(const SkBitmap& p_bitmap, const SkPaint& p_paint, const SkMatrix& p_matrix)
{
	// may need to apply matrix to gradient points

	int32_t tx = SkScalarFloor(p_matrix.getTranslateX());
	int32_t ty = SkScalarFloor(p_matrix.getTranslateY());
	m_clip.x += tx;
	m_clip.y += ty;
	m_gradient->origin.x += tx;
	m_gradient->origin.y += ty;
	m_gradient->primary.x += tx;
	m_gradient->primary.y += ty;
	m_gradient->secondary.x += tx;
	m_gradient->secondary.y += ty;
	return true;
}

uint32_t MCGradientShader::getFlags() { return 0; }

bool MCGradientShader::asABitmap(SkBitmap*, SkMatrix*, TileMode*) { return false; }
void MCGradientShader::shadeSpan16(int x, int y, uint16_t dstC[], int count) { ; }

void MCGradientShader::shadeSpan(int x, int y, SkPMColor dstC[], int count)
{
	int32_t t_dy = y - m_y;
	m_gradient_combiner->advance(m_gradient_combiner, t_dy);
	m_y = y;
	memset(dstC, 0x00, count * sizeof(SkPMColor));
	m_gradient_combiner->bits = dstC - x;
	m_gradient_combiner->combine(m_gradient_combiner, x, x + count, m_mask);
}

// initialize combiner
void MCGradientShader::beginSession()
{
	m_gradient_combiner = MCGradientFillCreateCombiner(m_gradient, m_clip);
	m_gradient_combiner->begin(m_gradient_combiner, m_clip.y);
	m_y = m_clip.y;
}


void MCGradientShader::endSession() { ; }


MCGradientShader::MCGradientShader(SkFlattenableReadBuffer &p_read_buffer)
{
	uint32_t t_length, t_buffer_size;
	void *t_buffer;
	t_length = p_read_buffer.readU32();
	t_buffer_size = (t_length + 3) & ~0x03;
	t_buffer = malloc(t_length);
	p_read_buffer.read(t_buffer, t_buffer_size);

    MCAutoDataRef t_data;
    /* UNCHECKED */ MCDataCreateWithBytes((byte_t*)t_buffer, t_length, &t_data);
    
	IO_handle t_fake_io = MCS_fakeopen(*t_data);
	MCObjectInputStream t_input(t_fake_io, t_length);
	m_gradient = new MCGradientFill;
	MCGradientFillUnserialize(m_gradient, t_input);

	MCS_close(t_fake_io);
	free(t_buffer);
}

void MCGradientShader::flatten(SkFlattenableWriteBuffer &p_write_buffer)
{
	IO_handle t_fake_io = MCS_fakeopenwrite();
	MCObjectOutputStream t_output(t_fake_io);
	MCGradientFillSerialize(m_gradient, t_output);

	uint32_t t_bytes_written;
	t_bytes_written = MCS_faketell(t_fake_io);

	uint32_t t_padding = t_bytes_written & 0x03;
	// pad to multiple of 4 bytes
	while (t_padding != 0)
	{
		t_output.WriteU8(0);
		t_padding -= 1;
	}

	uint32_t t_length = 0;
	char *t_buffer = NULL;
	MCS_fakeclosewrite(t_fake_io, t_buffer, t_length);

	p_write_buffer.write32(t_bytes_written);
	p_write_buffer.writeMul4(t_buffer, t_length);

	free(t_buffer);
}

////////////////////////////////////////////////////////////////////////////////
