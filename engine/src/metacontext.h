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

#ifndef __META_CONTEXT_H
#define __META_CONTEXT_H

#ifndef __CONTEXT_H
#include "context.h"
#endif

// A meta-context is an implementation of the MCContext interface which records
// all drawing primitives allowing global analysis to be done before rendering.
// This is necessary for printing, since such target devices will not support the
// full range of graphics operations needed.
//
// The MCMetaContext base-class requires three virtual methods to be implemented.
//
// domark:
//   This should render the given mark to the target device. You can assume that
//   the mark trys to do nothing that the target device will not support
// begincomposite:
//   This should return a raster context covering the rectangle passed in. This
//   context is used to:
//     1) compute a clipping mask for final output to the target device
//     2) render all pixels affected by the mark
// endcomposite:
//   This should take the raster context and clip region and render the image
//   to the target device within the clip.
//
// At present, a platform specific implementation of the MCPrinter will create
// a derived meta-context when 'beginrendering' is called, executing the contents
// of the context on 'endrendering'.

struct MCMark;
struct MCMarkState;
struct MCMarkStroke;
struct MCMarkFill;

class heap_t
{
public:
	heap_t(void)
	{
		f_chunks = NULL;
	}
	
	~heap_t(void)
	{
		while(f_chunks != NULL)
		{
			chunk_t *t_previous = f_chunks -> previous;
			free(f_chunks);
			f_chunks = t_previous;
		}
	}
	
	template<typename T> T *allocate(void)
	{
		return allocate<T>(sizeof(T));
	}
	
	template<typename T> T *allocate_array(uint4 count)
	{
		return allocate<T>(count * sizeof(T));
	}
	
	template<typename T> T *allocate(uint4 size)
	{
		size = (size + 3) & ~3;
		
		if (f_chunks == NULL || f_chunks -> remaining < size)
			if (!extend(size))
				return NULL;
				
		void *t_data;
		f_chunks -> remaining -= size;
		t_data = f_chunks -> frontier;
		f_chunks -> frontier = (uint1 *)f_chunks -> frontier + size;
		
		return (T *)t_data;
	}
	
private:
	struct chunk_t
	{
		chunk_t *previous;
		void *frontier;
		uint4 remaining;
	};
	
	chunk_t *f_chunks;
	
	bool extend(uint4 p_size)
	{
		// MW-2008-03-10: [[ Bug 5469 ]] Hmmm... for some strange reason I got it into my
		//   head that 'realloc' wouldn't move a block if you resize it as smaller... I was
		//   wrong!!!!
		p_size = (p_size + sizeof(chunk_t) + 4095) & ~4095;
		
		chunk_t *t_chunk;
		t_chunk = (chunk_t *)malloc(p_size);
		if (t_chunk != NULL)
		{
			t_chunk -> previous = f_chunks;
			t_chunk -> frontier = t_chunk + 1;
			t_chunk -> remaining = p_size - sizeof(chunk_t);
			f_chunks = t_chunk;
		}
		else
			return false;
			
		return true;
	}
};

struct MCMarkImage;

class MCMetaContext: public MCContext
{
public:
	MCMetaContext(const MCRectangle& p_page);
	~MCMetaContext(void);

	// MW-2009-06-10: [[ Bitmap Effects ]]
	void begin(bool p_group);
	bool begin_with_effects(MCBitmapEffectsRef effects, const MCRectangle& shape);
	void end(void);
	
	MCContextType gettype(void) const;
	
	bool changeopaque(bool p_new_value);
	void setprintmode(void);

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
	void setfillstyle(uint2 style, MCPatternRef p_pattern, int2 x, int2 y);
	void getfillstyle(uint2& style, MCPatternRef& r_pattern, int2& x, int2& y);
	void setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle);
	void getlineatts(uint2& linesize, uint2& linestyle, uint2& capstyle, uint2& joinstyle);
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

	void draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty, const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length, const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect);
	void drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect);
	void drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy);

	void drawlink(MCStringRef link, const MCRectangle& region);

	void applywindowshape(MCWindowShape *p_mask, unsigned int p_update_width, unsigned int p_update_height);

	void drawtheme(MCThemeDrawType type, MCThemeDrawInfo* p_parameters);

	bool lockgcontext(MCGContextRef& r_ctxt);
	void unlockgcontext(MCGContextRef ctxt);
	
	MCGAffineTransform getdevicetransform(void) { return MCGAffineTransformMakeIdentity(); }

	void clear(const MCRectangle *rect);
	MCRegionRef computemaskregion(void);
	
	uint2 getdepth(void) const;
	const MCColor& getblack(void) const;
	const MCColor& getwhite(void) const;
	const MCColor& getgray(void) const;
	const MCColor& getbg(void) const;
	
	void execute(void);

protected:
	// Execute the contents of the given group mark - used by derived classes
	// when doing a group mark.
	void executegroup(MCMark *group_mark);

	// Return 'true' if the current group mark can be handled natively.
	virtual bool candomark(MCMark *p_mark) = 0;

	// Execute the given mark - this should result in the appropriate drawing
	// primitives being executed on the target device.
	virtual void domark(MCMark *p_mark) = 0;

	// Begin rendering into an offscreen buffer for a drawing operation not
	// supported by the target device.
	virtual bool begincomposite(const MCRectangle &p_region, MCGContextRef &r_context) = 0;

	// Render the offscreen buffer to the target device using the passed
	// clip region.
	virtual void endcomposite(MCRegionRef p_clip_region) = 0;

private:
	uint1 f_quality;
	uint1 f_function;
	uint1 f_opacity;
	MCRectangle f_clip;
	MCFontStruct *f_font;
	MCMarkStroke *f_stroke;
	MCMarkFill *f_fill_foreground;
	MCMarkFill *f_fill_background;
    // SN-2014-08-25: [[ Bug 13187 ]] MarkImage added to save the image to be drawn
    MCMarkImage *f_image;
	bool f_stroke_used;
	bool f_fill_foreground_used;
	bool f_fill_background_used;
	
	MCMarkState *f_state_stack;
	
	// IM-2014-06-03: [[ GraphicsPerformance ]] Minimal implementation of save() & restore()
	MCRectangle *m_clip_stack;
	uint32_t m_clip_stack_size;
	uint32_t m_clip_stack_index;
	
	heap_t f_heap;

	MCMark *new_mark(uint4 p_type, bool p_stroke, bool p_fill);
	template<typename T> T *new_array(uint4 p_size);
	void new_fill_foreground(void);
	void new_fill_background(void);
	void new_stroke(void);
	
	void rectangle_mark(bool p_stroke, bool p_fill, const MCRectangle& rect, bool inside);
	void round_rectangle_mark(bool p_stroke, bool p_fill, const MCRectangle& rect, uint2 radius, bool inside);
	void polygon_mark(bool p_stroke, bool p_fill, MCPoint *p_vertices, uint2 p_arity, bool p_closed);
	void arc_mark(bool p_stroke, bool p_fill, const MCRectangle& p_bounds, uint2 p_start, uint2 p_angle, bool p_complete, bool inside);
	void path_mark(bool stroke, bool fill, MCPath *path, bool evenodd);
};


enum MCMarkType
{
	MARK_TYPE_END,
	MARK_TYPE_LINE,
	MARK_TYPE_POLYGON,
	MARK_TYPE_TEXT,
	MARK_TYPE_RECTANGLE,
	MARK_TYPE_ROUND_RECTANGLE,
	MARK_TYPE_ARC,
	MARK_TYPE_IMAGE,
	MARK_TYPE_METAFILE,
	MARK_TYPE_EPS,
	MARK_TYPE_THEME,
	MARK_TYPE_GROUP,
	MARK_TYPE_LINK,
	MARK_TYPE_PATH
};

struct MCMarkLine
{
	MCPoint start;
	MCPoint end;
};

struct MCMarkPolygon
{
	MCPoint *vertices;
	uint2 count;
	bool closed;
};

struct MCMarkRectangle
{
	MCRectangle bounds;
    // MM-2014-04-23: [[ Bug 11884 ]] Store by how much we want to inset (rather than we just want to inset).
	uint2 inset;
};

struct MCMarkRoundRectangle
{
	MCRectangle bounds;
	uint2 radius;
    // MM-2014-04-23: [[ Bug 11884 ]] Store by how much we want to inset (rather than we just want to inset).
	uint2 inset;
};

struct MCMarkArc
{
	MCRectangle bounds;
	uint2 start, angle;
    bool complete : 1;
    // MM-2014-04-23: [[ Bug 11884 ]] Store by how much we want to inset (rather than we just want to inset).
	uint2 inset;
};

struct MCMarkImage
{
	// The details of the image to be rendered.
	MCImageDescriptor descriptor;

	// The part of the image to be rendered in src pixels *after*
	// rotation and scaling has been applied.
	int2 sx, sy;
	uint2 sw, sh;

	// The place to render the top-left of the src rect
	int2 dx, dy;
    
    // SN-2014-08-25: [[ Bug 13187 ]] Added to allow listing of MarkImages
    MCMarkImage *previous;
};

struct MCMarkMetafile
{
	bool embedded;
	void *data;
	uint4 data_length;
	MCRectangle src_area;
	MCRectangle dst_area;
};

struct MCMarkText
{
	MCMarkFill *background;
    MCFontRef font;
	MCPoint position;
	void *data;
	uint2 length;
	bool unicode_override;
};

struct MCMarkTheme
{
	MCThemeDrawType type;
	uint8_t info[0];
};

// MW-2009-06-14: [[ Bitmap Effects ]] Added reference to any effects that may
//   have been requested with the group.
struct MCMarkGroup
{
	MCMark *head;
	MCMark *tail;
	uint1 opacity;
	uint1 function;
	MCBitmapEffectsRef effects;
	MCRectangle effects_shape;
};

struct MCMarkStroke
{
	uint2 style;
	uint2 width;
	uint2 cap;
	uint2 join;
	struct
	{
		uint1 *data;
		uint2 length;
		uint2 offset;
	} dash;
	real8 miter_limit;
};

struct MCMarkFill
{
	uint2 style;
	MCColor colour;
	MCPatternRef pattern;
	MCPoint origin;
	MCGradientFill *gradient;

	MCMarkFill *previous;
};

struct MCMarkLink
{
	char *text;
	MCRectangle region;
};

struct MCMarkPath
{
	uint32_t command_count;
	uint1 *commands;
	uint32_t ordinate_count;
	int4 *ordinates;
	bool evenodd;
};

struct MCMarkHeader
{
	MCMarkType type;
	MCMark *next;
	MCMark *previous;
	MCRectangle clip;
	MCMarkStroke *stroke;
	MCMarkFill *fill;
};

struct MCMark
{
	MCMarkType type;
	MCMark *next;
	MCMark *previous;
	MCRectangle clip;
	MCMarkStroke *stroke;
	MCMarkFill *fill;
	union
	{
		MCMarkLine line;
		MCMarkPolygon polygon;
		MCMarkRectangle rectangle;
		MCMarkRoundRectangle round_rectangle;
		MCMarkArc arc;
		MCMarkImage image;
		MCMarkMetafile metafile;
		MCMarkText text;
		MCMarkTheme theme;
		MCMarkGroup group;
		MCMarkLink link;
		MCMarkPath path;
	};
};

struct MCMarkState
{
	MCMarkState *parent;
	uint4 nesting;
	MCMark *root;
};


#endif
