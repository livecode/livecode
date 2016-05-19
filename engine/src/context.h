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

#ifndef CONTEXT_H
#define CONTEXT_H

#include "imagebitmap.h"
#include "graphics.h"

typedef real4 MCScalar;

struct MCTransform
{
	MCScalar sx, ry;
	MCScalar rx, sy;
	MCScalar tx, ty;
};

enum MCQuality
{
	QUALITY_DEFAULT,
	QUALITY_SMOOTH
};

enum MCContextType
{
	CONTEXT_TYPE_SCREEN,
	CONTEXT_TYPE_OFFSCREEN,
	CONTEXT_TYPE_PRINTER
};

struct MCStrokeStyle
{
	uint2 style;
	uint2 width;
	uint2 cap;
	uint2 join;
	real8 miter_limit;
	struct
	{
		uint2 length;
		uint4 *data;
	} dash;
};

class MCPath;
struct MCGradientFill;

struct MCBitmapEffects;
typedef MCBitmapEffects *MCBitmapEffectsRef;

enum MCImageDataType
{
	kMCImageDataNone,
	kMCImageDataPNG,
	kMCImageDataGIF,
	kMCImageDataJPEG
};

struct MCImageDescriptor
{
	bool has_transform : 1;
    bool has_center : 1;
    
	MCGAffineTransform transform;
	MCGRectangle center;
    
    MCGImageFilter filter;
	
	// IM-2013-07-19: [[ ResIndependence ]] add scale factor field for scaled images
	// IM-2014-08-07: [[ Bug 13021 ]] Split scale into x / y components
	MCGFloat x_scale;
	MCGFloat y_scale;

	// IM-2014-06-12: [[ ImageRepUpdate ]] Update image to be an MCGImage
	MCGImageRef image;

	// The image source data
	MCImageDataType data_type;
	void *data_bits;
	uint32_t data_size;
};

enum MCThemeDrawType
{
	THEME_DRAW_TYPE_SLIDER,
	THEME_DRAW_TYPE_SCROLLBAR,
	THEME_DRAW_TYPE_PROGRESS,
	THEME_DRAW_TYPE_BUTTON,
	THEME_DRAW_TYPE_FRAME,
	THEME_DRAW_TYPE_GROUP,
	THEME_DRAW_TYPE_TAB,
	THEME_DRAW_TYPE_TAB_PANE,
	THEME_DRAW_TYPE_FOCUS_RECT,
	THEME_DRAW_TYPE_BACKGROUND,
	THEME_DRAW_TYPE_MENU,
	THEME_DRAW_TYPE_GTK
};

enum MCDrawTextDirection
{
    kMCDrawTextDirectionLTR = 0,
    kMCDrawTextDirectionRTL
};

enum MCDrawTextBreaking
{
    kMCDrawTextNoBreak = 0,
    kMCDrawTextBreak
};

bool MCThemeDraw(MCGContextRef p_context, MCThemeDrawType p_type, MCThemeDrawInfo *p_info_ptr);

class MCContext
{
public:
	virtual ~MCContext(void) { };

	// MW-2009-06-10: [[ Bitmap Effects ]] Added new virtual method to 'begin' a
	//   layer with effects. The current clip is taken to be the destination
	//   scissor region, and the actual source region should be passed as shape.
	//   On exit, the clip will be the region of the source that is needed to
	//   render the layer with effects. If the clip is empty, begin_with_effects
	//   returns false, and the layer should not be rendered.
	virtual void begin(bool p_group) = 0;
	virtual bool begin_with_effects(MCBitmapEffectsRef effects, const MCRectangle& shape) = 0;
	virtual void end(void) = 0;

	virtual MCContextType gettype(void) const = 0;

	// MW-2009-06-14: This methods should be used to change the 'opaqueness'
	//   flag of the current layer. It should be used when it is known that
	//   everything inside the current clip is already opaque. This knowledge
	//   is currently used to improve text rendering. Note that changes to
	//   this must be stacked manually.
	virtual bool changeopaque(bool p_value) = 0;

	virtual void setprintmode(void) = 0;

	// IM-2014-06-03: [[ GraphicsPerformance ]] Save the current state of the graphics context
	virtual void save() = 0;
	// IM-2014-06-03: [[ GraphicsPerformance ]] Restore the previously saved graphics context state
	virtual void restore() = 0;
	
	// IM-2014-06-03: [[ GraphicsPerformance ]] Reduce the clipping region by intersecting with the given rect
	virtual void cliprect(const MCRectangle &p_rect) = 0;
	
	virtual void setclip(const MCRectangle& rect) = 0;
	virtual MCRectangle getclip(void) const = 0;
	virtual void clearclip(void) = 0;

	virtual void setorigin(int2 x, int2 y) = 0;
	virtual void clearorigin(void) = 0;

	virtual void setquality(uint1 quality) = 0;
	virtual void setfunction(uint1 function) = 0;
	virtual uint1 getfunction(void) = 0;
	virtual void setopacity(uint1 opacity) = 0;
	virtual uint1 getopacity(void) = 0;
	virtual void setforeground(const MCColor& c) = 0;
	virtual void setbackground(const MCColor& c) = 0;
	virtual void setdashes(uint2 offset, const uint1 *dashes, uint2 ndashes) = 0;
	virtual void setfillstyle(uint2 style, MCPatternRef p, int2 x, int2 y) = 0;
	virtual void getfillstyle(uint2& style, MCPatternRef &p, int2& x, int2& y) = 0;
	virtual void setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle) = 0;
	virtual void getlineatts(uint2 &r_linesize, uint2 &r_linestyle, uint2 &r_capstyle, uint2 &r_joinstyle) = 0;
	virtual void setmiterlimit(real8 p_limit) = 0;
	virtual void getmiterlimit(real8 &r_limit) = 0;
	virtual void setgradient(MCGradientFill *p_gradient) = 0;

	virtual void drawline(int2 x1, int2 y1, int2 x2, int2 y2) = 0;
	virtual void drawlines(MCPoint *points, uint2 npoints, bool p_closed = false) = 0;
	virtual void drawsegments(MCLineSegment *segments, uint2 nsegs) = 0;
	virtual void drawtext(coord_t x, int2 y, MCStringRef p_string, MCFontRef p_font, Boolean image, MCDrawTextBreaking = kMCDrawTextBreak, MCDrawTextDirection = kMCDrawTextDirectionLTR) = 0;
    virtual void drawtext_substring(coord_t x, int2 y, MCStringRef p_string, MCRange p_range, MCFontRef p_font, Boolean image, MCDrawTextBreaking = kMCDrawTextBreak, MCDrawTextDirection = kMCDrawTextDirectionLTR) = 0;
	virtual void drawrect(const MCRectangle& rect, bool inside = false) = 0;
	virtual void fillrect(const MCRectangle& rect, bool inside = false) = 0;
	virtual void fillrects(MCRectangle *rects, uint2 nrects) = 0;
	virtual void fillpolygon(MCPoint *points, uint2 npoints) = 0;
	virtual void drawroundrect(const MCRectangle& rect, uint2 radius, bool inside = false) = 0;
	virtual void fillroundrect(const MCRectangle& rect, uint2 radius, bool inside = false) = 0;
	virtual void drawarc(const MCRectangle& rect, uint2 start, uint2 angle, bool inside = false) = 0;
	virtual void drawsegment(const MCRectangle& rect, uint2 start, uint2 angle, bool inside = false) = 0;
	virtual void fillarc(const MCRectangle& rect, uint2 start, uint2 angle, bool inside = false) = 0;

	virtual void drawpath(MCPath *path) = 0;
	virtual void fillpath(MCPath *path, bool p_evenodd = true) = 0;

	virtual void drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect) = 0;
	virtual void draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty,
		                   const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length,
											 const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect) = 0;
	
	virtual void drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy) = 0;

	virtual void drawlink(MCStringRef link, const MCRectangle& region) = 0;

	virtual void applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height) = 0;

	virtual void drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_parameters) = 0;
	
	virtual bool lockgcontext(MCGContextRef& r_ctxt) = 0;
	virtual void unlockgcontext(MCGContextRef ctxt) = 0;
	
	// IM-2016-04-22: [[ WindowsPlayer ]] Returns the transform from user-space to the underlying device surface
	virtual MCGAffineTransform getdevicetransform(void) = 0;
	
	virtual MCRegionRef computemaskregion(void) = 0;
	virtual void clear(const MCRectangle* rect) = 0;

	virtual uint2 getdepth(void) const = 0;

	virtual const MCColor& getblack(void) const = 0;
	virtual const MCColor& getwhite(void) const = 0;
	virtual const MCColor& getgray(void) const = 0;
	virtual const MCColor& getbg(void) const = 0;
};

#endif
