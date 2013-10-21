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

#ifndef MBLANDROIDCONTEXT_H
#define MBLANDROIDCONTEXT_H

#ifndef CONTEXT_H
#include "context.h"
#endif

#include "gradient.h"

#include <SkCanvas.h>
#include <SkShader.h>
#include <SkTypeface.h>

struct MCAndroidFont
{
	uint32_t size;
	SkTypeface *sk_typeface;
};

class MCAndroidContext : public MCContext
{
public:
	MCAndroidContext(uint32_t p_width, uint32_t p_height, bool p_alpha);
	MCAndroidContext(uint32_t p_width, uint32_t p_height, uint32_t p_stride, void *p_pixels, bool p_alpha);
	~MCAndroidContext();

	void begin(bool p_group);
	bool begin_with_effects(MCBitmapEffectsRef effects, const MCRectangle &shape);
	void end(void);

	MCContextType gettype(void) const;

	bool changeopaque(bool p_value);

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
		                   MCStringRef prolog, MCStringRef psprolog, uint4 psprologlength, MCStringRef ps, uint4 length,
											 MCStringRef fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect);
	void drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy);
	
	void drawlink(MCStringRef link, const MCRectangle& region);

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
	
	void setprintmode(void);

	void setexternalalpha(MCBitmap *p_alpha);


private:
	struct Layer
	{
		Layer *				parent;
		MCRectangle			clip;
		MCPoint				origin;
		uint32_t				width;
		uint32_t				height;
		uint32_t				stride;
		uint8_t				function;
		uint8_t				opacity;
		uint32_t				nesting;
		MCBitmapEffectsRef	effects;
		MCRectangle			effects_shape;
		SkCanvas *			canvas;
		bool				update_geometry_state;
	};

	struct Paint
	{
		uint16_t	style;
		MCColor		color;
		Pixmap		pattern;
		MCPoint		origin;
	};

	void init();

	Layer *layer_push_bitmap(SkBitmap &p_bitmap, int32_t p_x_origin, int32_t p_y_origin, bool p_opaque);
	Layer *layer_push_new(MCRectangle p_clip, bool p_opaque);
	Layer *layer_pop(bool p_destroy);
	void layer_destroy(Layer *p_layer);


	void path_begin_fill(void);
	void path_end_fill(void);
	void path_begin_stroke(void);
	void path_end_stroke(void);
	
	void path_move_to(int32_t x, int32_t y);
	void path_line_to(int32_t x, int32_t y);
	void path_rect(const MCRectangle& r, bool p_outline);
	void path_round_rect(const MCRectangle& r, uint32_t radius, bool p_outline);
	void path_segment(const MCRectangle& r, uint32_t start, uint32_t angle, bool p_outline);
	void path_arc(const MCRectangle& r, uint32_t start, uint32_t angle, bool p_outline);
	void path_stroke(MCPath *path);
	void path_fill(MCPath *path, bool p_even_odd);

	void sync_layer_geometry_state(Layer *p_layer);
	void sync_geometry_state(void);
	void sync_layer_blend_state(SkPaint *p_paint, Layer *p_layer);
	void sync_fill_state(void);
	void sync_stroke_state(void);
	void sync_text_state(void);
	Layer *		m_layers;

	SkCanvas *			m_canvas;

	Paint 				m_fill;
	MCStrokeStyle 		m_stroke;
	MCColor 			m_background;
	bool 				m_antialias;
	MCGradientFill *	m_gradient_fill;

	SkPaint				m_paint;
	SkPath				m_path;

	bool 	m_update_fill_state;
	bool 	m_update_stroke_state;
	bool	m_update_text_state;

	SkTypeface *	m_typeface;
	SkScalar		m_textsize;
	
	// MW-2012-02-21: [[ FontRefs ]] Store the last set fontstruct so we know whether we need
	//   to change fonts on drawtext.
	MCFontStruct *	m_fontstruct;

	struct
	{
		void * data;
		uint32_t width, height, stride;
	} m_external_bitmap;
};


class MCCombinerXfermode : public SkXfermode
{
public:
	MCCombinerXfermode(uint32_t p_function);

    virtual void xfer32(SkPMColor dst[], const SkPMColor src[], int count, const SkAlpha aa[]);
    virtual void xfer16(uint16_t dst[], const SkPMColor src[], int count, const SkAlpha aa[]);
    virtual void xfer4444(uint16_t dst[], const SkPMColor src[], int count, const SkAlpha aa[]);
    virtual void xferA8(SkAlpha dst[], const SkPMColor src[], int count, const SkAlpha aa[]);

	virtual bool asCoeff(Coeff* src, Coeff* dst);

    virtual Factory getFactory() { return CreateProc; }
    virtual void    flatten(SkFlattenableWriteBuffer&);

protected:
    MCCombinerXfermode(SkFlattenableReadBuffer& rb);

private:
	uint32_t m_function;

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) {
    return SkNEW_ARGS(MCCombinerXfermode, (buffer)); }
};

class MCGradientShader : public SkShader
{
public:
	MCGradientShader(MCGradientFill *p_gradient, MCRectangle &p_clip);
	~MCGradientShader();

    // overrides from SkShader
    virtual bool setContext(const SkBitmap&, const SkPaint&, const SkMatrix&);
    virtual uint32_t getFlags();
    virtual void shadeSpan(int x, int y, SkPMColor dstC[], int count);
    virtual void shadeSpan16(int x, int y, uint16_t dstC[], int count);
    virtual void beginSession();
    virtual void endSession();
    virtual bool asABitmap(SkBitmap*, SkMatrix*, TileMode*);

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) { 
        return SkNEW_ARGS(MCGradientShader, (buffer));
    }

    // override from flattenable
    //virtual bool toDumpString(SkString* str) const;

protected:
    MCGradientShader(SkFlattenableReadBuffer& );
    virtual void flatten(SkFlattenableWriteBuffer& );
    virtual Factory getFactory() { return CreateProc; }

	int32_t					m_y;
	uint8_t *				m_mask;
	MCRectangle				m_clip;
	MCGradientFill *		m_gradient;
	MCGradientCombiner *	m_gradient_combiner;

private:    
    typedef SkShader INHERITED;
};

#endif //MBLANDROIDCONTEXT_H

