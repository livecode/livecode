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

#ifndef GRAPHICSCONTEXT_H
#define GRAPHICSCONTEXT_H

#include "context.h"
#include "graphics.h"

class MCGraphicsContext : public MCContext
{
public:
	MCGraphicsContext(MCGContextRef p_context);
	MCGraphicsContext(uint32_t p_width, uint32_t p_height, bool p_alpha);
	MCGraphicsContext(uint32_t p_width, uint32_t p_height, uint32_t p_stride, void *p_pixels, bool p_alpha);
	~MCGraphicsContext();
	
	void begin(bool p_group);
	bool begin_with_effects(MCBitmapEffectsRef effects, const MCRectangle &shape);
	void end(void);
	
	MCContextType gettype(void) const;
	
	bool changeopaque(bool p_value);
	
	void save();
	void restore();
	
	void cliprect(const MCRectangle &p_rect);
	
	void setclip(const MCRectangle& rect);
	MCRectangle getclip(void) const;
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
	void setfillstyle(uint2 style, MCPatternRef p, int2 x, int2 y);
	void getfillstyle(uint2& style, MCPatternRef& p, int2& x, int2& y);
	void setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle);
	void getlineatts(uint2 &r_linesize, uint2 &r_linestyle, uint2 &r_capstyle, uint2 &r_joinstyle);
	void setmiterlimit(real8 p_limit);
	void getmiterlimit(real8 &r_limit);
	void setgradient(MCGradientFill *p_gradient);
	
	void drawline(int2 x1, int2 y1, int2 x2, int2 y2);
	void drawlines(MCPoint *points, uint2 npoints, bool p_closed = false);
	void drawsegments(MCLineSegment *segments, uint2 nsegs);
	void drawtext(coord_t x, int2 y, MCStringRef p_string, MCFontRef p_font, Boolean image, MCDrawTextBreaking = kMCDrawTextBreak, MCDrawTextDirection = kMCDrawTextDirectionLTR);
    void drawtext_substring(coord_t x, int2 y, MCStringRef p_string, MCRange p_range, MCFontRef p_font, Boolean image, MCDrawTextBreaking = kMCDrawTextBreak, MCDrawTextDirection = kMCDrawTextDirectionLTR);
	void drawrect(const MCRectangle& rect, bool inside);
	void fillrect(const MCRectangle& rect, bool inside);
	void fillrects(MCRectangle *rects, uint2 nrects);
	void fillpolygon(MCPoint *points, uint2 npoints);
	void drawroundrect(const MCRectangle& rect, uint2 radius, bool inside);
	void fillroundrect(const MCRectangle& rect, uint2 radius, bool inside);
	void drawarc(const MCRectangle& rect, uint2 start, uint2 angle, bool inside);
	void drawsegment(const MCRectangle& rect, uint2 start, uint2 angle, bool inside);
	void fillarc(const MCRectangle& rect, uint2 start, uint2 angle, bool inside);
	
	void drawpath(MCPath *path);
	void fillpath(MCPath *path, bool p_evenodd = true);
	
	void drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect);
	void draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty,
				 const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length,
				 const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect);
	void drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy);

	void drawlink(MCStringRef link, const MCRectangle& region);
	//int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false);
	
	void applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height);
	
	void drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_parameters);
	
	MCRegionRef computemaskregion(void);
	void clear(void);
	void clear(const MCRectangle* rect);
		
	uint2 getdepth(void) const;
	
	const MCColor& getblack(void) const;
	const MCColor& getwhite(void) const;
	const MCColor& getgray(void) const;
	const MCColor& getbg(void) const;
	
	void setprintmode(void);
		
	// IM-2014-01-31: [[ HiDPI ]] Return the underlying MCGContextRef
	MCGContextRef getgcontextref(void) const;
	
	bool lockgcontext(MCGContextRef& r_gcontext);
	void unlockgcontext(MCGContextRef gcontext);
	
	MCGAffineTransform getdevicetransform(void);
	
private:
	void init(MCGContextRef p_context);

	MCGContextRef m_gcontext;
	MCColor m_background;
	uint8_t m_function;
	uint8_t m_opacity;
	int32_t m_pattern_x;
	int32_t m_pattern_y;
	MCPatternRef m_pattern;
	uint2 m_fill_style;

	uint16_t m_line_width;
	uint16_t m_line_style;
	uint16_t m_cap_style;
	uint16_t m_join_style;

	real64_t m_miter_limit;

	MCGFloat m_dash_phase;
	MCGFloat *m_dash_lengths;
	uint32_t m_dash_count;

	// IM-2014-06-19: [[ Bug 12557 ]] Override for the antialias setting
	bool m_force_antialiasing;
};

// MW-2014-01-07: [[ Bug 11632 ]] The player object distinguishes between 'screen' and 'offscreen' - in the
//   latter case it caches the bitmap and blits it rather than relying on the OS to overlay it. This derived
//   class handles this case by changing the 'type' appropriately.
class MCOffscreenGraphicsContext: public MCGraphicsContext
{
public:
	MCOffscreenGraphicsContext(MCGContextRef ctxt): MCGraphicsContext(ctxt) {}

	virtual MCContextType gettype(void) const { return CONTEXT_TYPE_OFFSCREEN; }
};

#endif
