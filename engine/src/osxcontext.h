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

#ifndef MACCONTEXT_H
#define MACCONTEXT_H

#ifndef CONTEXT_H
#include "context.h"
#endif

#ifndef PATH_H
#include "path.h"
#endif

#ifndef __MC_BITMAP_EFFECT__
#include "bitmapeffect.h"
#endif

struct MCCombiner;

class MCQuickDrawContext: public MCContext
{
	enum
	{
		FLAG_FILL_CHANGED = 1 << 0,
		FLAG_STROKE_CHANGED = 1 << 1,
		FLAG_FUNCTION_CHANGED = 1 << 2,
		FLAG_ORIGIN_CHANGED = 1 << 3,
		FLAG_CLIP_CHANGED = 1 << 4,
		FLAG_FONT_CHANGED = 1 << 5,
		FLAG_BACKGROUND_CHANGED = 1 << 6,
		FLAG_FOREGROUND_CHANGED = 1 << 7,
		FLAG_DASHES_CHANGED = 1 << 8,
		
		FLAG_IS_IRREGULAR_PATTERN = 1 << 16,
		
		FLAG_IS_OPAQUE = 1 << 26,
		FLAG_IS_PRINTER = 1 << 27,
		FLAG_IS_ALPHA = 1 << 28,
		FLAG_IS_MINE = 1 << 29,
		FLAG_IS_TRANSIENT = 1 << 30,
		FLAG_IS_OFFSCREEN = 1U << 31
	};
	
	struct Layer
	{
		Layer *parent;
		uint4 flags;
		MCRectangle clip;
		MCPoint origin;
		uint4 width;
		uint4 height;
		uint1 function;
		uint1 opacity;
		uint4 scroll;
		uint4 nesting;
		CGrafPtr port;
		PixMapHandle pixels;
		
		// MW-2009-06-11: [[ Bitmap Effects ]]
		MCBitmapEffectsRef effects;
		MCRectangle effects_shape;
	};

public:
  MCQuickDrawContext(void);
	~MCQuickDrawContext(void);
	
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
	void setgradient(MCGradientFill *p_gradient);

	void drawline(int2 x1, int2 y1, int2 x2, int2 y2);
	void drawlines(MCPoint *points, uint2 npoints, bool p_closed = false);
	void drawsegments(MCSegment *segments, uint2 nsegs);
	void drawtext(int2 x, int2 y, const char *s, uint2 length, MCFontStruct *f, Boolean image, bool p_unicode_override = false);
	void drawrect(const MCRectangle& rect);
	void fillrect(const MCRectangle& rect);
	
	void fillrect_with_native_function(const MCRectangle& p_rect, int p_mode);
	
	void fillrects(MCRectangle *rects, uint2 nrects);
	void fillpolygon(MCPoint *points, uint2 npoints);
	void drawroundrect(const MCRectangle& rect, uint2 radius);
	void fillroundrect(const MCRectangle& rect, uint2 radius);
	void drawarc(const MCRectangle& rect, uint2 start, uint2 angle);
	void drawsegment(const MCRectangle& rect, uint2 start, uint2 angle);
	void fillarc(const MCRectangle& rect, uint2 start, uint2 angle);

	void setmiterlimit(real8 p_limit);

	void drawpath(MCPath *path);
	void fillpath(MCPath *path, bool p_evenodd = true);

	void draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty, const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length, const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect);
	void drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect);
	void drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy);

	void drawlink(MCStringRef link, const MCRectangle& region);

	int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false);

	void applywindowshape(MCWindowShape *p_mask, unsigned int p_update_width, unsigned int p_update_height);

	void drawtheme(MCThemeDrawType type, MCThemeDrawInfo* p_parameters);
	void copyarea(Drawable p_src, uint4 p_dx, uint4 p_dy, uint4 p_sx, uint4 p_sy, uint4 p_sw, uint4 p_sh);
	void combine(Pixmap p_src, int4 p_dx, int4 p_dy, int4 p_sx, int4 p_sy, uint4 p_sw, uint4 p_sh);

	void clear(const MCRectangle *rect);
	MCRegionRef computemaskregion(void);
	
	MCBitmap *lock(void);
	void unlock(MCBitmap *);

	uint2 getdepth(void) const;
	const MCColor& getblack(void) const;
	const MCColor& getwhite(void) const;
	const MCColor& getgray(void) const;
	const MCColor& getbg(void) const;
	
	void setexternalalpha(MCBitmap *p_alpha);
	
// Non-Virtuals

	CGContextRef lock_as_cg(void);
	void unlock_as_cg(CGContextRef p_context);
	
	PixMapHandle qd_get_pixmap(void)
	{
		return f_layers -> pixels;
	}
	
	CGrafPtr qd_get_port(void)
	{
		return f_layers -> port;
	}
	
	bool istransient(void)
	{
		MCContextType t_type;
		t_type = gettype();
		return t_type == CONTEXT_TYPE_OFFSCREEN || t_type == CONTEXT_TYPE_PRINTER;
	}
	
	void qd_common_begin(void);
	void qd_common_end(void);
	
	static MCQuickDrawContext *create_with_port(CGrafPtr p_port, bool p_transient = false, bool p_needs_alpha = true);
	static MCQuickDrawContext *create_with_parameters(uint4 p_width, uint4 p_height, bool p_transient = false, bool p_needs_alpha = true);

private:
	struct
	{
		uint2 style;
		MCColor colour;
		Pixmap pattern;
		MCPoint origin;
	} f_fill;
	
	MCStrokeStyle f_stroke;
	
	MCColor f_background;
	MCFontStruct *f_font;
	uint1 f_quality;
	
	PixMapHandle f_old_pattern;
	Handle f_old_pattern_data;
	PixPatHandle f_pattern;
	CGrafPtr f_pattern_port;
	Ptr f_pattern_base_address;
	
	MCGradientFill *f_gradient_fill;
	
	Layer *f_layers;
	
	// Allow external alpha channel for image-editing
	MCBitmap *f_external_alpha;
	
	static CGrafPtr s_current_port;
	
	MCCombiner *combiner_lock(void);
	void combiner_unlock(MCCombiner *);
	
	static Layer *layer_create_with_port(CGrafPtr p_port, bool p_transient, bool p_needs_alpha);
	static Layer *layer_create_with_parameters(uint4 p_width, uint4 p_height, bool p_transient, bool p_needs_alpha);
	Layer *layer_create(const MCRectangle& p_clip);
	Layer *layer_destroy(Layer *);
	
	static void layer_lock(Layer *, void*&, uint4&);
	static void layer_unlock(Layer *, void*, uint4);
	
	void qd_stroke_begin(void);
	void qd_stroke_end(void);
	
	void qd_fill_begin(void);
	void qd_fill_end(void);
	
	void qd_text_begin(void);
	void qd_text_end(void);
	
	void dofillrect(const MCRectangle& p_rect, int p_function);
	
	void setflags(uint4 p_affect, bool p_value)
	{
		if (p_value)
			f_layers -> flags |= p_affect;
		else
			f_layers -> flags &= ~p_affect;
	}
	
	bool getflags(uint4 p_flags) const
	{
		return (f_layers -> flags & p_flags) != 0;
	}

};

bool qd_begin_pattern(Drawable p_pattern, const MCPoint& p_origin, PixPatHandle p_pix_pat, GWorldPtr& r_pattern_port, Ptr& r_pattern_address);
void qd_draw_pict(void *data, const MCRectangle& drect, const MCRectangle& crect);
void qd_end_pattern(PixPatHandle p_pix_pat, GWorldPtr p_port);


#endif
