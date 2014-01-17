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

#ifndef W32CONTEXT_H
#define W32CONTEXT_H

#ifndef CONTEXT_H
#include "context.h"
#endif

#ifndef PATH_H
#include "path.h"
#endif

struct MCGDISurface
{
	HBITMAP handle;
	void *data;
	uint4 width;
	uint4 height;
	uint4 stride;
	uint1 depth;
	uint1 alpha;
};

class MCGDIContext: public MCContext
{
	enum
	{
		FLAG_FILL_CHANGED = 1 << 0,
		FLAG_STROKE_CHANGED = 1 << 1,
		FLAG_FUNCTION_CHANGED = 1 << 2,
		FLAG_CLIP_CHANGED = 1 << 3,
		FLAG_FONT_CHANGED = 1 << 4,
		FLAG_BACKGROUND_CHANGED = 1 << 5,
		FLAG_FOREGROUND_CHANGED = 1 << 6,
		FLAG_DASHES_CHANGED = 1 << 7,

		// This flag is set when the current layer has been marked completely
		// opaque.
		FLAG_IS_OPAQUE = 1 << 14,
		FLAG_OPAQUE_TEXT = 1 << 15,

		FLAG_MASK_CHANGED = 1 << 16,
		FLAG_MASK_OPAQUE = 1 << 17,
		FLAG_MASK_ACTIVE = 1 << 18,
		FLAG_FILL_READY = 1 << 19,
		FLAG_STROKE_READY = 1 << 20,

		FLAG_IS_PRINTER = 1 << 25,
		FLAG_IS_TRANSIENT = 1 << 26,
		FLAG_IS_MEMORY = 1 << 27,
		FLAG_IS_WINDOW = 1 << 28,
		FLAG_IS_ALPHA = 1 << 29,

		FLAG_HAS_CLIP = 1 << 30,
		FLAG_HAS_MASK = 1U << 31,

		FLAGMASK_LAYER_SPECIFIC = FLAG_MASK_CHANGED | FLAG_MASK_OPAQUE | FLAG_MASK_ACTIVE | FLAG_IS_WINDOW | FLAG_IS_ALPHA | FLAG_HAS_MASK | FLAG_IS_OPAQUE
	};

	struct Layer
	{
		Layer *parent;
		uint4 flags;
		MCRectangle clip;
		MCPoint origin;
		HBITMAP destination;
		HBITMAP mask;
		void *destination_bits;
		void *mask_bits;
		uint4 width;
		uint4 height;
		uint1 function;
		uint1 opacity;
		uint4 nesting;
		MCBitmapEffectsRef effects;
		MCRectangle effects_shape;
	};

public:
	MCGDIContext(void);
	~MCGDIContext(void);

	HDC gethdc(void);
	HDC getmaskhdc(void);

	// MW-2009-06-10: [[ Bitmap Effects ]]
	void begin(bool p_group);
	bool begin_with_effects(MCBitmapEffectsRef effects, const MCRectangle& shape);
	void end(void);

	MCContextType gettype(void) const;

	bool changeopaque(bool p_new_value);
	void setprintmode(void);

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
	void setfillstyle(uint2 style, Pixmap p, int2 x, int2 y);
	void getfillstyle(uint2& style, Pixmap& p, int2& x, int2& y);
	void setgradient(MCGradientFill *p_gradient);
	void setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle);
	void setmiterlimit(real8 p_limit);

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

	void drawimage(const MCImageDescriptor& info, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy);

	void drawlink(const char *link, const MCRectangle& region);

	int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false);

	void applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height);

	void drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_info);
	void copyarea(Drawable p_src, uint4 p_dx, uint4 p_dy, uint4 p_sx, uint4 p_sy, uint4 p_sw, uint4 p_sh);

	void combine(Pixmap p_src, int4 p_dx, int4 p_dy, int4 p_sx, int4 p_sy, uint4 p_sw, uint4 p_sh);

	MCBitmap *lock(void);
	void unlock(MCBitmap *);

	MCRegionRef computemaskregion(void);
	void clear(const MCRectangle *rect);

	uint2 getdepth(void) const;
	const MCColor& getblack(void) const;
	const MCColor& getwhite(void) const;
	const MCColor& getgray(void) const;
	const MCColor& getbg(void) const;
	
	void setexternalalpha(MCBitmap *p_alpha);

	static MCGDIContext *create_with_window(HWND p_window);
	static MCGDIContext *create_with_bitmap(HBITMAP p_bitmap, bool p_transient = false);
	static MCGDIContext *create_with_parameters(uint4 p_width, uint4 p_height, bool p_alpha, bool p_transient = false);

private:
	// MW-2009-06-10: [[ Bitmap Effects ]]
	void begin_common(const MCRectangle& clip);

	bool getflags(uint4 p_flag) const;
	void setflags(uint4 p_flag, bool p_state);
	void changeflags(uint4 p_mask, uint4 p_value);

	void gdi_polyline(POINT *p_points, uint4 p_count);
	void gdi_polylines(POINT *p_points, uint4 *p_counts, uint4 p_count);
	void gdi_polygon(POINT *p_points, uint4 p_count);
	void gdi_rectangle(int4 p_left, int4 p_top, int4 p_right, int4 p_bottom);
	void gdi_round_rectangle(int4 p_left, int4 p_top, int4 p_right, int4 p_bottom, int4 p_radius);
	void gdi_arc(int4 p_left, int4 p_top, int4 p_right, int4 p_bottom, int4 p_start, int4 p_end);
	void gdi_text(int4 p_x, int4 p_y, const char *p_string, uint4 p_length, bool p_opaque, bool p_unicode_override = false);
	void gdi_image(Pixmap mask, Pixmap data, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy);

	void gdi_stroke_begin(void);
	void gdi_stroke_end(void);

	void gdi_fill_begin(void);
	void gdi_fill_end(void);

	void gdi_synchronize_pen(void);
	void gdi_synchronize_brush(void);

	MCCombiner *combiner_lock(void);
	void combiner_unlock(MCCombiner *);

public:
	void flush_mask(void);

private:
	uint4 f_flags;

	struct
	{
		uint2 style;
		uint4 colour;
		Pixmap pattern;
		MCPoint origin;
	} f_fill;

	MCStrokeStyle f_stroke;

	MCFontStruct *f_font;

	uint4 f_nesting;
	uint2 f_width;
	uint2 f_height;
	MCRectangle f_clip;
	MCPoint f_origin;

	uint4 f_background;
	uint1 f_quality;
	uint1 f_opacity;
	uint1 f_function;
	uint1 f_depth;

	HDC f_destination_dc;
	HDC f_mask_dc;

	HBRUSH f_current_brush;

	HPEN f_current_pen;
	HPEN f_current_mask_pen;

	union
	{
		HWND window;
		HBITMAP bitmap;
	} f_destination_surface;

	HBITMAP f_mask_surface;
	void *f_destination_bits;
	void *f_mask_bits;

	Layer *f_layers;

	MCBitmap *f_external_alpha;

	MCGradientFill *f_gradient_fill;

	// MW-2009-06-10: [[ Bitmap Effects ]]
	MCBitmapEffectsRef f_effects;
	MCRectangle f_effects_shape;
};

inline bool MCGDIContext::getflags(uint4 p_flags) const
{
	return ((f_flags & p_flags) != 0);
}

inline void MCGDIContext::setflags(uint4 p_flags, bool p_state)
{
	changeflags(p_flags, p_state ? p_flags : 0);
}

inline void MCGDIContext::changeflags(uint4 p_mask, uint4 p_value)
{
	f_flags = (f_flags & ~p_mask) | p_value;
}

#endif
