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

#ifndef CONTEXTSCALEWRAPPER_H
#define CONTEXTSCALEWRAPPER_H

#include "context.h"
#include "datastructures.h"

struct pixmap_data
{
	Pixmap original;
	Pixmap scaled;
};

class MCContextScaleWrapper: public MCContext
{
private:
	MCContext *m_context;
	int4 scale;
	MCLinkedList<pixmap_data> *pixmap_cache;
	MCLinkedList<MCGradientFill*> *gradient_cache;

	// MW-2009-06-14: [[ Bitmap Effects ]] Cache to store scaled versions.
	MCLinkedList<MCBitmapEffectsRef> *effects_cache;

	MCFontStruct *m_font;
	mutable MCRectangle m_cliprect;

public:
	MCContextScaleWrapper(MCContext *p_context, int4 p_scale);
	~MCContextScaleWrapper();
	MCContext *getcontext(void);

	// MW-2009-06-10: [[ Bitmap Effects ]]
	void begin(bool p_group);
	bool begin_with_effects(MCBitmapEffectsRef effects, const MCRectangle& shape);
	void end(void);

	MCContextType gettype(void) const;

	bool changeopaque(bool p_new_value);
	void setprintmode(void);

	void setclip(const MCRectangle& rect);
	const MCRectangle& getclip(void) const;
	void clearclip(void);

	void setorigin(int2 x, int2 y);
	void clearorigin(void);

	void setquality(uint1 quality);
	void setfunction(uint1 function);
	uint1 getfunction(void);
	void setopacity(uint1 opacity);
	uint1 getopacity(void);

	void setforeground(const MCColor& c);
	void setbackground(const MCColor& c);
	void setdashes(uint2 offset, const uint1 *dashes, uint2 ndashes);
	void setfillstyle(uint2 style, Pixmap p, int2 x, int2 y);
	void getfillstyle(uint2& style, Pixmap& p, int2& x, int2& y);
	void setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle);
	void setmiterlimit(real8 p_limit);
	void setgradient(MCGradientFill *p_gradient);

	void drawline(int2 x1, int2 y1, int2 x2, int2 y2);
	void drawlines(MCPoint *points, uint2 npoints, bool p_closed = false);
	void drawsegments(MCSegment *segments, uint2 nsegs);
	void drawtext(int2 x, int2 y, const char *s, uint2 length, MCFontStruct *f, Boolean image, bool p_unicode_override = false);
	void drawrect(const MCRectangle& rect);
	void fillrect(const MCRectangle& rect);
	void fillrects(MCRectangle *rects, uint2 nrects);
	void fillpolygon(MCPoint *points, uint2 npoints);
	void drawroundrect(const MCRectangle& rect, uint2 radius);
	void fillroundrect(const MCRectangle& rect, uint2 radius);
	void drawarc(const MCRectangle& rect, uint2 start, uint2 angle);
	void drawsegment(const MCRectangle& rect, uint2 start, uint2 angle);
	void fillarc(const MCRectangle& rect, uint2 start, uint2 angle);

	void drawpath(MCPath *path);
	void fillpath(MCPath *path, bool p_evenodd = true);

	void drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect);
	void draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty,
		                   const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length,
											 const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect);
	void drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy);

	void drawlink(const char *link, const MCRectangle& region);

	int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false);

	void applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height);

	void drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_parameters);
	void copyarea(Drawable p_src, uint4 p_dx, uint4 p_dy, uint4 p_sx, uint4 p_sy, uint4 p_sw, uint4 p_sh);

	void combine(Pixmap p_src, int4 p_dx, int4 p_dy, int4 p_sx, int4 p_sy, uint4 p_sw, uint4 p_sh);

	MCBitmap *lock(void);
	void unlock(MCBitmap *p_bitmap);
	
	MCRegionRef computemaskregion(void);
	void clear(const MCRectangle *rect);

	uint2 getdepth(void) const;

	const MCColor& getblack(void) const;
	const MCColor& getwhite(void) const;
	const MCColor& getgray(void) const;
	const MCColor& getbg(void) const;
};

#endif

