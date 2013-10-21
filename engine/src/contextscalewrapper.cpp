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

#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "flst.h"
#include "globals.h"
#include "mctheme.h"
#include "context.h"
#include "contextscalewrapper.h"
#include "gradient.h"
#include "path.h"
#include "bitmapeffect.h"

#if defined(_LINUX)
#include "lnximagecache.h"
#endif

MCContextScaleWrapper::MCContextScaleWrapper(MCContext *p_context, int4 p_scale)
{
	m_context = p_context;
	scale = p_scale;
	pixmap_cache = new MCLinkedList<pixmap_data>;
	gradient_cache = new MCLinkedList<MCGradientFill*>;
	
	// MW-2009-06-14: [[ Bitmap Effects ]] Create effect cache
	effects_cache = new MCLinkedList<MCBitmapEffectsRef>;

	m_font = NULL;
}

MCContextScaleWrapper::~MCContextScaleWrapper()
{
	MCLinkedListNode<pixmap_data> *t_node = pixmap_cache->first();
	while (t_node != NULL)
	{
		Pixmap t_pixmap = t_node->data().scaled;
		MCscreen->freepixmap(t_pixmap);
		t_node = t_node->next();
	}
	delete pixmap_cache;

	MCLinkedListNode<MCGradientFill*> *t_gnode = gradient_cache->first();
	while (t_gnode != NULL)
	{
		MCGradientFill *t_gradient = t_gnode->data();
		MCGradientFillFree(t_gradient);
		t_gnode = t_gnode->next();
	}
	delete gradient_cache;

	// MW-2009-06-14: [[ Bitmap Effects ]] Destroy effect cache
	MCLinkedListNode<MCBitmapEffectsRef> *t_enode = effects_cache->first();
	while(t_enode != NULL)
	{
		MCBitmapEffectsFinalize(t_enode -> data());
		t_enode = t_enode -> next();
	}
	delete effects_cache;
}

MCContext *MCContextScaleWrapper::getcontext()
{
	return m_context;
}

void MCContextScaleWrapper::begin(bool p_group)
{
	m_context->begin(p_group);
}

bool MCContextScaleWrapper::begin_with_effects(MCBitmapEffectsRef p_effects, const MCRectangle& shape)
{
	MCBitmapEffectsRef t_scaled_effects;
	MCBitmapEffectsInitialize(t_scaled_effects);
	
	if (!MCBitmapEffectsAssign(t_scaled_effects, p_effects))
		return false;
	
	if (!MCBitmapEffectsScale(t_scaled_effects, scale))
	{
		MCBitmapEffectsFinalize(t_scaled_effects);
		return false;
	}

	effects_cache -> append(t_scaled_effects);

	return m_context -> begin_with_effects(t_scaled_effects, MCU_scale_rect(shape, scale));
}

void MCContextScaleWrapper::end(void)
{
	m_context->end();
}

MCContextType MCContextScaleWrapper::gettype(void) const
{
	return m_context->gettype();
}

bool MCContextScaleWrapper::changeopaque(bool p_new_value)
{
	return m_context -> changeopaque(p_new_value);
}

void MCContextScaleWrapper::setprintmode(void)
{
}

void MCContextScaleWrapper::setclip(const MCRectangle& rect)
{
	MCRectangle t_rect = MCU_scale_rect(rect, scale);
	m_context->setclip(t_rect);
}

const MCRectangle& MCContextScaleWrapper::getclip(void) const
{
	m_cliprect = MCU_scale_rect(m_context->getclip(), -scale);
	return m_cliprect;
}

void MCContextScaleWrapper::clearclip(void)
{
	m_context->clearclip();
}

void MCContextScaleWrapper::setorigin(int2 x, int2 y)
{
	m_context->setorigin(x * scale, y * scale);
}

void MCContextScaleWrapper::clearorigin(void)
{
	m_context->clearorigin();
}

void MCContextScaleWrapper::setquality(uint1 quality)
{
	m_context->setquality(quality);
}

void MCContextScaleWrapper::setfunction(uint1 function)
{
	m_context->setfunction(function);
}

uint1 MCContextScaleWrapper::getfunction(void)
{
	return m_context->getfunction();
}

void MCContextScaleWrapper::setopacity(uint1 opacity)
{
	m_context->setopacity(opacity);
}

uint1 MCContextScaleWrapper::getopacity(void)
{
	return m_context->getopacity();
}

void MCContextScaleWrapper::setforeground(const MCColor& c)
{
	m_context->setforeground(c);
}

void MCContextScaleWrapper::setbackground(const MCColor& c)
{
	m_context->setbackground(c);
}

void MCContextScaleWrapper::setdashes(uint2 offset, const uint1 *dashes, uint2 ndashes)
{
	uint1 *t_dashes = new uint1[ndashes];
	for (int i = 0; i < ndashes; i++)
		t_dashes[i] = dashes[i] * scale;

	m_context->setdashes(offset, t_dashes, ndashes);

	delete t_dashes;
}

void MCContextScaleWrapper::setfillstyle(uint2 style, Pixmap p, int2 x, int2 y)
{
	Pixmap t_pixmap;
	if (p == NULL)
		t_pixmap = NULL;
	else
	{
		uint2 t_w, t_h, t_d;
		MCscreen->getpixmapgeometry(p, t_w, t_h, t_d);
		MCBitmap *t_srcbmp = MCscreen->getimage(p, 0, 0, t_w, t_h);

		t_w *= scale;
		t_h *= scale;

		MCBitmap *t_dstbmp = MCscreen->createimage(t_d, t_w, t_h, False, 0x0, False, False);
		MCscreen->scaleimage(t_srcbmp, t_dstbmp);

		t_pixmap = MCscreen->createpixmap(t_w, t_h, t_d, False);
		MCscreen->putimage(t_pixmap, t_dstbmp, 0, 0, 0, 0, t_w, t_h);

		MCscreen->destroyimage(t_srcbmp);
		MCscreen->destroyimage(t_dstbmp);

		pixmap_data t_pxdata;
		t_pxdata.original = p;
		t_pxdata.scaled = t_pixmap;

		pixmap_cache->append(t_pxdata);
	}
	m_context->setfillstyle(style, t_pixmap, x * scale, y * scale);
}

void MCContextScaleWrapper::getfillstyle(uint2& style, Pixmap& p, int2& x, int2& y)
{
	int2 t_x, t_y;
	Pixmap t_p;

	m_context->getfillstyle(style, t_p, t_x, t_y);
	x = t_x / scale;
	y = t_y / scale;

	MCLinkedListNode<pixmap_data> *t_node = pixmap_cache->first();
	while (t_node != NULL)
	{
		if (t_p == t_node->data().scaled)
		{
			p = t_node->data().original;
			break;
		}
		t_node = t_node->next();
	}
}

void MCContextScaleWrapper::setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle)
{
	m_context->setlineatts(linesize * scale, linestyle, capstyle, joinstyle);
}

void MCContextScaleWrapper::setmiterlimit(real8 p_limit)
{
	m_context->setmiterlimit(p_limit);
}

void MCContextScaleWrapper::setgradient(MCGradientFill *p_gradient)
{
	MCGradientFill *t_gradient;
	if (p_gradient == NULL)
		t_gradient = NULL;
	else
	{
		t_gradient = MCGradientFillCopy(p_gradient);
		t_gradient->origin.x *= scale;
		t_gradient->origin.y *= scale;
		t_gradient->primary.x *= scale;
		t_gradient->primary.y *= scale;
		t_gradient->secondary.x *= scale;
		t_gradient->secondary.y *= scale;

		gradient_cache->append(t_gradient);
	}
	m_context->setgradient(t_gradient);
}

void MCContextScaleWrapper::drawline(int2 x1, int2 y1, int2 x2, int2 y2)
{
	m_context->drawline(x1 * scale, y1 * scale, x2 * scale, y2 * scale);
}

void MCContextScaleWrapper::drawlines(MCPoint *points, uint2 npoints, bool p_closed)
{
	MCPoint *t_points = new MCPoint[npoints];
	for (int i = 0; i < npoints; i++)
	{
		t_points[i].x = points[i].x * scale;
		t_points[i].y = points[i].y * scale;
	}

	m_context->drawlines(t_points, npoints, p_closed);

	delete t_points;
}

void MCContextScaleWrapper::drawsegments(MCSegment *segments, uint2 nsegs)
{
	MCSegment *t_segments = new MCSegment[nsegs];
	for (int i = 0; i < nsegs; i++)
	{
		t_segments[i].x1 = segments[i].x1 * scale;
		t_segments[i].y1 = segments[i].y1 * scale;
		t_segments[i].x2 = segments[i].x2 * scale;
		t_segments[i].y2 = segments[i].y2 * scale;
	}

	m_context->drawsegments(t_segments, nsegs);

	delete t_segments;
}

void MCContextScaleWrapper::drawtext(int2 x, int2 y, const char *s, uint2 length, MCFontStruct *f, Boolean image, bool p_unicode_override)
{
	// MW-2012-02-16: [[ FontRefs ]] If the previous font is not the same as the new one,
	//   set the current one to the scaled version of f.
	if (f != m_font)
	{
		// Scale the font struct appropriately.
		MCNameRef t_fontname;
		uint2 t_fontsize, t_fontstyle;
		Boolean t_printer;
		if (MCdispatcher->getfontlist()->getfontstructinfo(t_fontname, t_fontsize, t_fontstyle, t_printer, f))
		{
			// MW-2009-04-23: [[ Bug ]] If we are in formatForPrinting mode, we *do* want to create a printer
			//   font.
			t_fontsize *= scale;
			m_font = MCdispatcher->loadfont(t_fontname, t_fontsize, t_fontstyle, t_printer);
		}
	}

	m_context->drawtext(x * scale, y * scale, s, length, m_font, image, p_unicode_override);
}

void MCContextScaleWrapper::drawrect(const MCRectangle& rect)
{
	m_context->drawrect(MCU_scale_rect(rect, scale));
}

void MCContextScaleWrapper::fillrect(const MCRectangle& rect)
{
	m_context->fillrect(MCU_scale_rect(rect, scale));
}

void MCContextScaleWrapper::fillrects(MCRectangle *rects, uint2 nrects)
{
	MCRectangle *t_rects = new MCRectangle[nrects];
	for (int i = 0; i < nrects; i++)
	{
		t_rects[i] = MCU_scale_rect(rects[i], scale);
	}

	m_context->fillrects(t_rects, nrects);

	delete t_rects;
}

void MCContextScaleWrapper::fillpolygon(MCPoint *points, uint2 npoints)
{
	MCPoint *t_points = new MCPoint[npoints];
	for (int i = 0; i < npoints; i++)
	{
		t_points[i].x = points[i].x * scale;
		t_points[i].y = points[i].y * scale;
	}

	m_context->fillpolygon(t_points, npoints);
	
	delete t_points;
}

void MCContextScaleWrapper::drawroundrect(const MCRectangle& rect, uint2 radius)
{
	m_context->drawroundrect(MCU_scale_rect(rect, scale), radius * scale);
}

void MCContextScaleWrapper::fillroundrect(const MCRectangle& rect, uint2 radius)
{
	m_context->fillroundrect(MCU_scale_rect(rect, scale), radius * scale);
}

void MCContextScaleWrapper::drawarc(const MCRectangle& rect, uint2 start, uint2 angle)
{
	m_context->drawarc(MCU_scale_rect(rect, scale), start, angle);
}

void MCContextScaleWrapper::drawsegment(const MCRectangle& rect, uint2 start, uint2 angle)
{
	m_context->drawsegment(MCU_scale_rect(rect, scale), start, angle);
}

void MCContextScaleWrapper::fillarc(const MCRectangle& rect, uint2 start, uint2 angle)
{
	m_context->fillarc(MCU_scale_rect(rect, scale), start, angle);
}

void MCContextScaleWrapper::drawpath(MCPath *path)
{
	MCPath *t_path = path->copy_scaled(scale);
	m_context->drawpath(t_path);

	delete t_path;
}

void MCContextScaleWrapper::fillpath(MCPath *path, bool p_evenodd)
{
	MCPath *t_path = path->copy_scaled(scale);
	m_context->fillpath(t_path, p_evenodd);

	delete t_path;
}

void MCContextScaleWrapper::drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect)
{
	m_context->drawpict(data, length, embed, MCU_scale_rect(drect, scale), MCU_scale_rect(crect, scale));
}

void MCContextScaleWrapper::draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty,
							MCStringRef prolog, MCStringRef psprolog, uint4 psprologlength, MCStringRef ps, uint4 length,
							MCStringRef fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect)
{
	m_context->draweps(sx, sy, angle, xscale, yscale, tx, ty, prolog, psprolog, psprologlength, ps, length,
						 fontname, fontsize, fontstyle, font, trect);
}

void MCContextScaleWrapper::drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy)
{
	MCImageBitmap *t_scaled = nil;

	uindex_t t_width, t_height;
	t_width = p_image.bitmap->width;
	t_height = p_image.bitmap->height;

	t_width *= scale;
	t_height *= scale;

	/* UNCHECKED */ MCImageScaleBitmap(p_image . bitmap, t_width, t_height, INTERPOLATION_NEAREST, t_scaled);

	MCImageDescriptor t_image;
	memset(&t_image, 0, sizeof(MCImageDescriptor));
	t_image . bitmap = t_scaled;
	m_context -> drawimage(t_image, sx * scale, sy * scale, sw * scale, sh * scale, dx * scale, dy * scale);

	MCImageFreeBitmap(t_scaled);
}

void MCContextScaleWrapper::drawlink(MCStringRef link, const MCRectangle& region)
{
	// This is a no-op as links are irrelevant to rasterized portions. The print
	// system will iterate through any links and render them directly.
}

int4 MCContextScaleWrapper::textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override)
{
	return m_context->textwidth(f, s, l, p_unicode_override);
}

void MCContextScaleWrapper::applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height)
{
	m_context->applywindowshape(p_mask, p_u_width, p_u_height);
}

#ifndef _DESKTOP
void MCContextScaleWrapper::drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_parameters)
{
}
#endif

void MCContextScaleWrapper::copyarea(Drawable p_src, uint4 p_dx, uint4 p_dy, uint4 p_sx, uint4 p_sy, uint4 p_sw, uint4 p_sh)
{
	m_context->copyarea(p_src, p_dx, p_dy, p_sx, p_sy, p_sw, p_sh);
}

void MCContextScaleWrapper::combine(Pixmap p_src, int4 p_dx, int4 p_dy, int4 p_sx, int4 p_sy, uint4 p_sw, uint4 p_sh)
{
	m_context->combine(p_src, p_dx, p_dy, p_sx, p_sy, p_sw, p_sh);
}

MCBitmap *MCContextScaleWrapper::lock(void)
{
	return m_context->lock();
}

void MCContextScaleWrapper::unlock(MCBitmap *p_bitmap)
{
	m_context->unlock(p_bitmap);
}

MCRegionRef MCContextScaleWrapper::computemaskregion(void)
{
	return m_context->computemaskregion();
}

void MCContextScaleWrapper::clear(const MCRectangle* rect)
{
	m_context->clear(nil);
}

uint2 MCContextScaleWrapper::getdepth(void) const
{
	return m_context->getdepth();
}

const MCColor& MCContextScaleWrapper::getblack(void) const
{
	return m_context->getblack();
}
const MCColor& MCContextScaleWrapper::getwhite(void) const
{
	return m_context->getwhite();
}
const MCColor& MCContextScaleWrapper::getgray(void) const
{
	return m_context->getgray();
}
const MCColor& MCContextScaleWrapper::getbg(void) const
{
	return m_context->getbg();
}
