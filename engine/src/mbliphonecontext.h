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

#ifndef __MC_MOBILE_IPHONE_CONTEXT__
#define __MC_MOBILE_IPHONE_CONTEXT__

#ifndef CONTEXT_H
#include "context.h"
#endif

#ifndef PATH_H
#include "path.h"
#endif

struct MCMobileBitmap;

class MCIPhoneContext: public MCContext
{
public:
	// Create a context around the given drawable, with optional external
	// mask. If <foreign> is true, then the drawable and mask are not deleted with
	// the context. If <opaque> is true, the drawable is assumed to be alpha-less.
	MCIPhoneContext(MCMobileBitmap *p_target, MCBitmap *p_external_mask, bool p_foreign, bool p_alpha);
	~MCIPhoneContext(void);

	void begin(bool p_group);
	bool begin_with_effects(MCBitmapEffectsRef effects, const MCRectangle& shape);
	void end(void);

	MCContextType gettype(void) const;

	bool changeopaque(bool p_new_value);

	void setclip(const MCRectangle& rect);
	const MCRectangle getclip(void) const;
	void clearclip(void);

	void setorigin(int2 x, int2 y);
	void clearorigin(void);

	void setquality(uint1 quality);
	void setgradient(MCGradientFill* fill);

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

	void drawline(int2 x1, int2 y1, int2 x2, int2 y2);
	void drawlines(MCPoint *points, uint2 npoints, bool p_closed = false);
	void drawsegments(MCLineSegment *segments, uint2 nsegs);
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
	
	void drawlink(MCStringRef link, const MCRectangle& region);
	
	int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false);

	void applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height);

	void drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo *p_info);
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
	
	void setprintmode(void);
	void setexternalalpha(MCBitmap *p_alpha);

private:
	struct Layer;
	
	void layer_push_bitmap(MCMobileBitmap *bitmap, bool opaque);
	Layer *layer_push_new(const MCRectangle& rect, bool opaque);
	Layer *layer_pop(bool destroy);
	void layer_destroy(Layer *layer);
	
	void path_begin_fill(void);
	void path_end_fill(void);
	void path_begin_stroke(void);
	void path_end_stroke(void);
	
	void path_move_to(int32_t x, int32_t y);
	void path_line_to(int32_t x, int32_t y);
	void path_rect(const MCRectangle& r, bool outline);
	void path_round_rect(const MCRectangle& r, uint32_t radius, bool outline);
	void path_segment(const MCRectangle& r, uint32_t start, uint32_t angle,  bool outline);
	void path_arc(const MCRectangle& r, uint32_t start, uint32_t angle, bool outline);
	void path_stroke(MCPath *path);
	void path_fill(MCPath *path, bool p_even_odd);
	
	void path_do_move_to(CGFloat x, CGFloat y);
	void path_do_line_to(CGFloat x, CGFloat y);
	void path_do_cubic_to(CGFloat ax, CGFloat ay, CGFloat bx, CGFloat by, CGFloat ex, CGFloat ey);
	void path_do_arc(CGFloat cx, CGFloat cy, CGFloat rh, CGFloat rv, uint32_t sa, uint32_t ea, bool first);
	void path_do_close(void);
	bool path_do_alloc(uint32_t command_count, uint32_t data_count);
	
	void sync_geometry_state(void);
	void sync_fill_state(void);
	void sync_stroke_state(void);
	
	MCCombiner *combiner_lock(void);
	void combiner_unlock(MCCombiner *combiner);
	
	struct Layer
	{
		Layer *parent;
		MCRectangle clip;
		MCPoint origin;
		uint32_t width;
		uint32_t height;
		uint32_t stride;
		uint8_t function;
		uint8_t opacity;
		uint32_t nesting;
		MCBitmapEffectsRef effects;
		MCRectangle effects_shape;
		CGContextRef context;
		void *data;
		bool foreign_data;
		bool update_geometry_state;
	};
	
	Layer *m_layers;

	struct
	{
		uint2 style;
		MCColor color;
		Pixmap pattern;
		MCPoint origin;
	} m_fill;
	MCStrokeStyle m_stroke;

	MCColor m_background;
	MCFontStruct *m_font;
	uint1 m_quality;
	
	MCGradientFill *m_gradient_fill;
	
	MCMobileBitmap *m_target;
	MCBitmap *m_target_mask;
	bool m_target_foreign;
	bool m_target_opaque;
	bool m_target_mono;
	
	bool m_update_fill_state;
	bool m_update_stroke_state;
	
	// This is true when we are accumulating a native (CG) path.
	bool m_path_is_native;
	
	// Non-native path data
	uint8_t *m_path_commands;
	uint32_t m_path_command_count;
	uint32_t m_path_command_capacity;
	int32_t *m_path_data;
	uint32_t m_path_data_count;
	uint32_t m_path_data_capacity;
};

#endif
