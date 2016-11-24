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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"

#include "globals.h"

#include "mctheme.h"

#include "context.h"
#include "metacontext.h"
#include "bitmapeffect.h"
#include "path.h"

#include "graphics.h"
#include "graphics_util.h"
#include "graphicscontext.h"

inline bool operator != (const MCColor& a, const MCColor& b)
{
	return a . red != b . red || a . green != b . green || a . blue != b . blue;
}

MCMetaContext::MCMetaContext(const MCRectangle& p_page)
{
	f_quality = QUALITY_DEFAULT;
	f_function = GXcopy;
	f_opacity = 255;
	f_clip = p_page;
	f_font = NULL;
	f_stroke = NULL;
	f_fill_foreground = NULL;
	f_fill_background = NULL;
    // SN-2014-08-25: [[ Bug 13187 ]] Image marks added
    f_image = NULL;
	f_stroke_used = false;
	f_fill_foreground_used = false;
	f_fill_background_used = false;
	f_state_stack = NULL;
	
	m_clip_stack = nil;
	m_clip_stack_size = 0;
	m_clip_stack_index = 0;
    
	begin(true);
}

MCMetaContext::~MCMetaContext(void)
{
	while(f_state_stack != NULL)
		end();

	while (f_fill_foreground != nil)
	{
		MCPatternRelease(f_fill_foreground -> pattern);
		f_fill_foreground = f_fill_foreground -> previous;
	}

	while (f_fill_background != nil)
	{
		MCPatternRelease(f_fill_background -> pattern);
		f_fill_background = f_fill_background -> previous;
	}
    
    // SN-2014-08-25: [[ Bug 13187 ]] Image marks added
    while(f_image != nil)
    {
        MCGImageRelease(f_image -> descriptor . image);
        f_image = f_image -> previous;
    }
	
	MCMemoryDeleteArray(m_clip_stack);
}


void MCMetaContext::begin(bool p_overlap)
{
	if (!p_overlap && f_opacity == 255 && (f_function == GXcopy || f_function == GXblendSrcOver) && f_state_stack -> root -> group . effects == NULL)
	{
		f_state_stack -> nesting += 1;
		return;
	}

	MCMarkState *t_new_state;
	t_new_state = new (nothrow) MCMarkState;
	
	MCMark *t_root_mark;
	t_root_mark = NULL;
	if (t_new_state != NULL)
		t_root_mark = new_mark(MARK_TYPE_GROUP, false, false);
		
	if (t_root_mark != NULL)
	{
		t_root_mark -> group . head = NULL;
		t_root_mark -> group . tail = NULL;
		t_root_mark -> group . opacity = f_opacity;
		t_root_mark -> group . function = f_function;
		t_root_mark -> group . effects = NULL;
		
		t_new_state -> parent = f_state_stack;
		t_new_state -> nesting = 0;
		t_new_state -> root = t_root_mark;

		f_state_stack = t_new_state;
	}
	else
	{
		f_state_stack -> nesting += 1;
		
		if (t_new_state != NULL)
			delete t_new_state;
	}
}

bool MCMetaContext::begin_with_effects(MCBitmapEffectsRef p_effects, const MCRectangle& p_shape)
{
	// Compute the region of the shape required to correctly render the given
	// clip.
	MCRectangle t_layer_clip;
	MCBitmapEffectsComputeClip(p_effects, p_shape, getclip(), t_layer_clip);

	// If the clip is empty, return.
	if (t_layer_clip . width == 0 || t_layer_clip . height == 0)
		return false;

	// Begin a new layer - making sure it won't be nested.
	begin(true);

	// If we didn't manage to create a new layer, then we are done.
	if (f_state_stack -> nesting != 0)
		return true;

	// Add in the effects
	f_state_stack -> root -> group . effects = p_effects;
	f_state_stack -> root -> group . effects_shape = p_shape;

	// Set the clip to be used by the source - note that this is different
	// from the clip stored in the mask because that's the region of the
	// destination we want to render.
	setclip(t_layer_clip);

	return true;
}

void MCMetaContext::end(void)
{
	assert(f_state_stack != NULL);

	if (f_state_stack -> nesting > 0)
	{
		f_state_stack -> nesting -= 1;
		return;
	}

	MCMarkState *t_state;
	t_state = f_state_stack;
	f_state_stack = f_state_stack -> parent;
	delete t_state;
}

MCContextType MCMetaContext::gettype(void) const
{
	return CONTEXT_TYPE_PRINTER;
}

bool MCMetaContext::changeopaque(bool p_new_value)
{
	return false;
}

void MCMetaContext::setprintmode(void)
{
}

void MCMetaContext::save()
{
	if (m_clip_stack_index + 1 > m_clip_stack_size)
		/* UNCHECKED */ MCMemoryResizeArray(m_clip_stack_size + 1, m_clip_stack, m_clip_stack_size);
	m_clip_stack[m_clip_stack_index++] = f_clip;
}

void MCMetaContext::restore()
{
	if (m_clip_stack_index > 0)
		f_clip = m_clip_stack[--m_clip_stack_index];
}

void MCMetaContext::cliprect(const MCRectangle &p_rect)
{
	f_clip = MCU_intersect_rect(f_clip, p_rect);
}

void MCMetaContext::setclip(const MCRectangle& rect)
{
	f_clip = rect;
}

MCRectangle MCMetaContext::getclip(void) const
{
	return f_clip;
}

void MCMetaContext::clearclip(void)
{
	f_clip = f_state_stack -> root -> clip;
}

void MCMetaContext::setorigin(int2 x, int2 y)
{
	MCUnreachable();
}

void MCMetaContext::clearorigin(void)
{
	MCUnreachable();
}

void MCMetaContext::setquality(uint1 quality)
{
}

void MCMetaContext::setfunction(uint1 function)
{
	f_function = function;
}

uint1 MCMetaContext::getfunction(void)
{
	return f_function;
}

void MCMetaContext::setopacity(uint1 opacity)
{
	f_opacity = opacity;
}

uint1 MCMetaContext::getopacity(void)
{
	return f_opacity;
}

void MCMetaContext::setforeground(const MCColor& c)
{
	if (f_fill_foreground == NULL || c != f_fill_foreground -> colour)
	{
		new_fill_foreground();
		
		if (f_fill_foreground != NULL)
			f_fill_foreground -> colour = c;
	}
}

void MCMetaContext::setbackground(const MCColor& c)
{
	if (f_fill_background == NULL || c != f_fill_background -> colour)
	{
		new_fill_background();
	
		if (f_fill_background != NULL)
			f_fill_background -> colour = c;
	}
}

void MCMetaContext::setdashes(uint2 offset, const uint1 *dashes, uint2 ndashes)
{
	new_stroke();
	
	if (f_stroke != NULL)
	{
		f_stroke -> dash . offset = offset;
		f_stroke -> dash . length = ndashes;
		f_stroke -> dash . data = new_array<uint1>(ndashes);
		memcpy(f_stroke -> dash . data, dashes, ndashes);
	}
}

void MCMetaContext::setfillstyle(uint2 style, MCPatternRef p, int2 x, int2 y)
{
	if (f_fill_foreground == NULL || style != f_fill_foreground -> style || p != f_fill_foreground -> pattern || x != f_fill_foreground -> origin . x || y != f_fill_foreground -> origin . y)
	{
		new_fill_foreground();
	
		if (f_fill_foreground != NULL)
		{
			if (style != FillTiled || p != NULL)
			{
				f_fill_foreground -> style = style;
				MCPatternRelease(f_fill_foreground -> pattern);
				f_fill_foreground -> pattern = MCPatternRetain(p);
				f_fill_foreground -> origin . x = x;
				f_fill_foreground -> origin . y = y;
			}
			else
			{
				f_fill_foreground -> style = FillSolid;
				f_fill_foreground -> pattern = p;
				f_fill_foreground -> origin . x = 0;
				f_fill_foreground -> origin . y = 0;
			}
		}
	}
}

void MCMetaContext::getfillstyle(uint2& style, MCPatternRef& p, int2& x, int2& y)
{
	if (f_fill_foreground != NULL)
	{
		style = f_fill_foreground -> style;
		p = f_fill_foreground -> pattern;
		x = f_fill_foreground -> origin . x;
		y = f_fill_foreground -> origin . y;
	}
	else
	{
		style = FillSolid;
		p = NULL;
		x = 0;
		y = 0;
	}
}

void MCMetaContext::setgradient(MCGradientFill *p_gradient)
{
	if (f_fill_foreground == NULL || p_gradient != f_fill_foreground -> gradient)
	{
		new_fill_foreground();
		
		if (f_fill_foreground != NULL)
		{
			f_fill_foreground -> gradient = p_gradient;
		}
	}
}

void MCMetaContext::setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle)
{
	if (f_stroke == NULL || linesize != f_stroke -> width || linestyle != f_stroke -> style || capstyle != f_stroke -> cap || joinstyle != f_stroke -> join)
		new_stroke();
	
	if (f_stroke != NULL)
	{
		f_stroke -> width = linesize;
		f_stroke -> style = linestyle;
		f_stroke -> cap = capstyle;
		f_stroke -> join = joinstyle;
	}
}

void MCMetaContext::getlineatts(uint2& linesize, uint2& linestyle, uint2& capstyle, uint2& joinstyle)
{
	if (f_stroke != nil)
	{
		linesize = f_stroke->width;
		linestyle = f_stroke->style;
		capstyle = f_stroke->cap;
		joinstyle = f_stroke->join;
	}
	else
	{
		linesize = 0;
		linestyle = LineSolid;
		capstyle = CapButt;
		joinstyle = JoinBevel;
	}
}

void MCMetaContext::setmiterlimit(real8 p_limit)
{
	if (f_stroke == NULL || p_limit != f_stroke->miter_limit)
		new_stroke();

	if (f_stroke != NULL)
	{
		f_stroke->miter_limit = p_limit;
	}
}

void MCMetaContext::getmiterlimit(real8 &r_limit)
{
	if (f_stroke != nil)
		r_limit = f_stroke->miter_limit;
	else
		r_limit = 0.0;
}

void MCMetaContext::drawline(int2 x1, int2 y1, int2 x2, int2 y2)
{
	MCMark *t_mark;
	
	t_mark = new_mark(MARK_TYPE_LINE, true, false);
	if (t_mark != NULL)
	{
		t_mark -> line . start . x = x1;
		t_mark -> line . start . y = y1;
		t_mark -> line . end . x = x2;
		t_mark -> line . end . y = y2;
	}
}

void MCMetaContext::drawlines(MCPoint *points, uint2 npoints, bool p_closed)
{
	polygon_mark(true, false, points, npoints, p_closed);
}

void MCMetaContext::drawsegments(MCLineSegment *segments, uint2 nsegs)
{
	for(uint2 t_segment = 0; t_segment < nsegs; ++t_segment)
		drawline(segments[t_segment] . x1, segments[t_segment] . y1, segments[t_segment] . x2, segments[t_segment] . y2);
}

void MCMetaContext::drawtext(coord_t x, int2 y, MCStringRef p_string, MCFontRef p_font, Boolean image, MCDrawTextBreaking p_can_break, MCDrawTextDirection p_direction)
{
    MCRange t_range;
    t_range = MCRangeMake(0, MCStringGetLength(p_string));
    drawtext_substring(x, y, p_string, t_range, p_font, image, p_can_break, p_direction);
}

void MCMetaContext::drawtext_substring(coord_t x, int2 y, MCStringRef p_string, MCRange p_range, MCFontRef p_font, Boolean image, MCDrawTextBreaking p_can_break, MCDrawTextDirection p_direction)
{
	// MW-2009-12-22: Make sure we don't generate 0 length text mark records
	if (MCStringIsEmpty(p_string) || p_range.length == 0)
		return;
	
	MCMark *t_mark;
	t_mark = new_mark(MARK_TYPE_TEXT, false, true);
	if (t_mark != NULL)
	{
		// MW-2012-02-28: [[ Bug ]] Use the fontstruct from the parameter as 'setfont()' is no
		//   longer used.
		t_mark -> text . font = p_font;
		t_mark -> text . position . x = x;
		t_mark -> text . position . y = y;
		if (image && f_fill_background != NULL)
		{
			t_mark -> text . background = f_fill_background;
			f_fill_background_used = true;
		}
		else
			t_mark -> text . background = NULL;
        
        uindex_t t_length = p_range.length;
        if (MCStringIsNative(p_string))
        {
            t_mark -> text . data = new_array<char>(t_length);
            t_mark -> text . length = t_length;
            if (t_mark -> text . data != NULL)
                MCStringGetNativeChars(p_string, p_range, (char_t *)t_mark -> text . data);
            t_mark -> text . unicode_override = false;
        }
        else
        {
            t_mark -> text . data = new_array<unichar_t>(t_length);
            t_mark -> text . length = t_length;
            // SN-2014-06-17 [[ Bug 12595 ]] Printing to PDF does not yield all information
            if (t_mark -> text . data != NULL)
                MCStringGetChars(p_string, p_range, (unichar_t *)t_mark -> text . data);
            t_mark -> text . unicode_override = true;
        }
	}
}

void MCMetaContext::drawrect(const MCRectangle& rect, bool inside)
{
	rectangle_mark(true, false, rect, inside);
}
	
void MCMetaContext::fillrect(const MCRectangle& rect, bool inside)
{
    // MM-2014-04-23: [[ Bug 11884 ]] Make sure we store the inside param for fills. This ensures the fill path is the samde as the stroke.
	rectangle_mark(false, true, rect, inside); 
}

void MCMetaContext::fillrects(MCRectangle *rects, uint2 nrects)
{
	for(uint4 t_index = 0; t_index < nrects; ++t_index)
		rectangle_mark(false, true, rects[t_index], false);
}

void MCMetaContext::fillpolygon(MCPoint *points, uint2 npoints)
{
	polygon_mark(false, true, points, npoints, true);
}

void MCMetaContext::drawroundrect(const MCRectangle& rect, uint2 radius, bool inside)
{
	round_rectangle_mark(true, false, rect, radius, inside);
}

void MCMetaContext::fillroundrect(const MCRectangle& rect, uint2 radius, bool inside)
{
        // MM-2014-04-23: [[ Bug 11884 ]] Make sure we store the inside param for fills. This ensures the fill path is the samde as the stroke.
	round_rectangle_mark(false, true, rect, radius, inside);
}

void MCMetaContext::drawarc(const MCRectangle& rect, uint2 start, uint2 angle, bool inside)
{
	arc_mark(true, false, rect, start, angle, false, inside);
}

void MCMetaContext::drawsegment(const MCRectangle& rect, uint2 start, uint2 angle, bool inside)
{
	arc_mark(true, false, rect, start, angle, true, inside);
}

void MCMetaContext::fillarc(const MCRectangle& rect, uint2 start, uint2 angle, bool inside)
{
        // MM-2014-04-23: [[ Bug 11884 ]] Make sure we store the inside param for fills. This ensures the fill path is the samde as the stroke.
	arc_mark(false, true, rect, start, angle, true, inside);
}


void MCMetaContext::drawpath(MCPath *path)
{
	path_mark(false, true, path, false);
}

void MCMetaContext::fillpath(MCPath *path, bool p_evenodd)
{
	path_mark(true, false, path, p_evenodd);
}


void MCMetaContext::draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty, const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length, const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect)
{
}

void MCMetaContext::drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect)
{
	MCMark *t_mark;
	t_mark = new_mark(MARK_TYPE_METAFILE, false, false);
	if (t_mark != NULL)
	{
		if (embed)
		{
			uint1 *t_new_data;
			t_new_data = new_array<uint1>(length);
			memcpy(t_new_data, data, length);

			t_mark -> metafile . data = t_new_data;
			t_mark -> metafile . data_length = length;
			t_mark -> metafile . embedded = true;
		}
		else
		{
			t_mark -> metafile . data = data;
			t_mark -> metafile . data_length = length;
			t_mark -> metafile . embedded = false;
		}
		t_mark -> metafile . src_area = crect;
		t_mark -> metafile . dst_area = drect;
	}
}

void MCMetaContext::drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy)
{
	MCMark *t_mark;
	bool t_need_group;

	t_need_group = (!MCGImageIsOpaque(p_image.image) || f_function != GXcopy || f_opacity != 255);

	MCRectangle t_old_clip;
	t_old_clip = getclip();
	
	MCRectangle t_clip;
	MCU_set_rect(t_clip, dx, dy, sw, sh);
	t_clip = MCU_intersect_rect(t_clip, f_clip);
	setclip(t_clip);

	if (t_need_group)
		begin(true);
		
	t_mark = new_mark(MARK_TYPE_IMAGE, false, false);
	if (t_mark != NULL)
	{
        // SN-2014-08-25: [[ Bug 13187 ]] Image marks added
		t_mark -> image . descriptor = p_image;
        MCGImageRetain(t_mark -> image . descriptor . image);
		t_mark -> image . sx = sx;
		t_mark -> image . sy = sy;
		t_mark -> image . sw = sw;
		t_mark -> image . sh = sh;
		t_mark -> image . dx = dx;
		t_mark -> image . dy = dy;
        
        t_mark -> image . previous = f_image;
        f_image = &t_mark -> image;
	}
	
	if (t_need_group)
		end();

	setclip(t_old_clip);
}

void MCMetaContext::drawlink(MCStringRef p_link, const MCRectangle& p_region)
{
	MCMark *t_mark;
	t_mark = new_mark(MARK_TYPE_LINK, false, false);
	if (t_mark != NULL)
	{
		t_mark -> link . region = p_region;
		t_mark -> link . text = new_array<char>(MCStringGetLength(p_link) + 1);
		if (t_mark -> link . text != NULL)
        {
            char *t_link;
            /* UNCHECKED */ MCStringConvertToCString(p_link, t_link);
			t_mark -> link . text = t_link;
        }
	}
}

void MCMetaContext::applywindowshape(MCWindowShape *p_mask, unsigned int p_update_width, unsigned int p_update_height)
{
	MCUnreachable();
}


void MCMetaContext::drawtheme(MCThemeDrawType type, MCThemeDrawInfo* p_info)
{
	MCMark *t_mark;

	begin(true);
	
	t_mark = new_mark(MARK_TYPE_THEME, false, false);
	if (t_mark != NULL)
	{
		t_mark -> theme . type = type;

		// MW-2011-09-14: [[ Bug 9719 ]] Query the current MCTheme object for the MCThemeDrawInfo size.
		memcpy(&t_mark -> theme . info, p_info, MCcurtheme != nil ? MCcurtheme -> getthemedrawinfosize() : 0);
	}
	
	end();
}

void MCMetaContext::clear(const MCRectangle *rect)
{
	MCUnreachable();
}

MCRegionRef MCMetaContext::computemaskregion(void)
{
    MCUnreachableReturn(NULL);
}


uint2 MCMetaContext::getdepth(void) const
{
	return 32;
}

const MCColor& MCMetaContext::getblack(void) const
{
	return MCscreen -> black_pixel;
}

const MCColor& MCMetaContext::getwhite(void) const
{
	return MCscreen -> white_pixel;
}

const MCColor& MCMetaContext::getgray(void) const
{
	return MCscreen -> gray_pixel;
}

const MCColor& MCMetaContext::getbg(void) const
{
	if (getdepth() == 32)
		return MCscreen -> background_pixel;
	
	return MCscreen -> white_pixel;
}

bool MCMetaContext::lockgcontext(MCGContextRef& r_ctxt)
{
    // SN-2014-08-25: [[ Bug 13187 ]] Implementation of MCMetaContext::lockgcontext needed
    //  by the printers.    
    bool t_success;
    MCGContextRef t_context;
    
    save();

    t_success = MCGContextCreate(f_clip.width, f_clip.height, true, t_context);
    
    if (t_success)
    {
        // Set the origin appropriately
        MCGContextTranslateCTM(t_context, -1.0 * f_clip.x, -1.0*f_clip.y);
        r_ctxt = t_context;
    }
    
    return t_success;
}

void MCMetaContext::unlockgcontext(MCGContextRef ctxt)
{
    // SN-2014-08-25: [[ Bug 13187 ]] Implementation of MCMetaContext::unlockgcontext needed
    //  by the printers.
    MCGImageRef t_image;
    
    restore();
    
    // Get the image we want to get drawn
    MCGContextCopyImage(ctxt, t_image);
    MCGContextRelease(ctxt);
    
    MCImageDescriptor t_desc;
    t_desc . has_transform = false;
    t_desc . has_center = false;
    t_desc . filter = kMCGImageFilterNone;
    t_desc . x_scale = t_desc . y_scale = 1.0f;
    t_desc . image = t_image;
    t_desc . data_type = kMCImageDataNone;
    
    // Add the image in the MetaContext drawing queue.
    drawimage(t_desc, 0, 0, f_clip . width, f_clip . height, f_clip . x, f_clip . y);
    
    MCGImageRelease(t_image);
}

static bool mark_indirect(MCContext *p_context, MCMark *p_mark, MCMark *p_upto_mark, const MCRectangle& p_clip)
{
	MCRectangle t_clip;
	t_clip = MCU_intersect_rect(p_mark -> clip, p_clip);
	
	if (t_clip . width == 0 || t_clip . height == 0)
		return false;

	bool t_finished;
	t_finished = false;

	if (p_mark == p_upto_mark)
		t_finished = true;
		
	if (p_mark -> type == MARK_TYPE_GROUP)
	{
		p_context -> setopacity(p_mark -> group . opacity);
		p_context -> setfunction(p_mark -> group . function);
		p_context -> setclip(t_clip);

		bool t_have_layer;
		t_have_layer = true;

		if (p_mark -> group . effects == NULL)
			p_context -> begin(true);
		else
			t_have_layer = p_context -> begin_with_effects(p_mark -> group . effects, p_mark -> group . effects_shape);

		if (t_have_layer)
		{
			p_context -> setopacity(255);
			p_context -> setfunction(GXcopy);

			for(MCMark *t_group_mark = p_mark -> group . head; t_group_mark != NULL; t_group_mark = t_group_mark -> next)
			{
				t_finished = mark_indirect(p_context, t_group_mark, p_upto_mark, t_clip);
				if (t_finished)
					break;
			}
		
			p_context -> end();
		}
	}
	else
	{
		if (p_mark -> stroke != NULL)
		{
			if (p_mark -> stroke -> dash . length != 0)
				p_context -> setdashes(p_mark -> stroke -> dash . offset, p_mark -> stroke -> dash . data, p_mark -> stroke -> dash . length);
			p_context -> setlineatts(p_mark -> stroke -> width, p_mark -> stroke -> style, p_mark -> stroke -> cap, p_mark -> stroke -> join);
		}
		
		if (p_mark -> fill != NULL)
		{
			p_context -> setforeground(p_mark -> fill -> colour);

			p_context -> setfillstyle(p_mark -> fill -> style, p_mark -> fill -> pattern, p_mark -> fill -> origin . x, p_mark -> fill -> origin . y);

			p_context->setgradient(p_mark->fill->gradient);	
		}
		
		if (p_mark->fill != NULL && p_mark->fill->gradient != NULL)
			p_context->setquality(QUALITY_SMOOTH);
		else
			p_context->setquality(QUALITY_DEFAULT);

		p_context -> setclip(t_clip);
	
		switch(p_mark -> type)
		{
			case MARK_TYPE_LINE:
				p_context -> drawline(p_mark -> line . start . x, p_mark -> line . start . y, p_mark -> line . end . x, p_mark -> line . end . y);
			break;
			
			case MARK_TYPE_POLYGON:
				if (p_mark -> stroke != NULL)
					p_context -> drawlines(p_mark -> polygon . vertices, p_mark -> polygon . count);
				else
					p_context -> fillpolygon(p_mark -> polygon . vertices, p_mark -> polygon . count);
			break;
			
			case MARK_TYPE_TEXT:
            {
				if (p_mark -> text . background != NULL)
					p_context -> setbackground(p_mark -> text . background -> colour);
                
                MCAutoStringRef t_string;
                if (p_mark -> text . unicode_override)
                    /* UNCHECKED */ MCStringCreateWithChars((const unichar_t*)p_mark -> text . data, p_mark -> text . length, &t_string);
                else
                    /* UNCHECKED */ MCStringCreateWithNativeChars((const char_t*)p_mark -> text . data, p_mark -> text . length, &t_string);
                
                p_context -> drawtext(p_mark -> text . position . x, p_mark -> text . position . y, *t_string, p_mark -> text . font, p_mark -> text . background != NULL);

                break;
            }
			
			case MARK_TYPE_RECTANGLE:
				if (p_mark -> stroke != NULL)
					p_context -> drawrect(p_mark -> rectangle . bounds, p_mark -> rectangle . inset > 0);
				else
					p_context -> fillrect(p_mark -> rectangle . bounds);
			break;
			
			case MARK_TYPE_ROUND_RECTANGLE:
				if (p_mark -> stroke != NULL)
					p_context -> drawroundrect(p_mark -> round_rectangle . bounds, p_mark -> round_rectangle . radius, p_mark -> round_rectangle . inset > 0);
				else
					p_context -> fillroundrect(p_mark -> round_rectangle . bounds, p_mark -> round_rectangle . radius);
			break;
			
			case MARK_TYPE_ARC:
				if (p_mark -> stroke != NULL)
				{
					if (p_mark -> arc . complete)
						p_context -> drawsegment(p_mark -> arc . bounds, p_mark -> arc . start, p_mark -> arc . angle, p_mark -> arc . inset > 0);
					else
						p_context -> drawarc(p_mark -> arc . bounds, p_mark -> arc . start, p_mark -> arc . angle, p_mark -> arc . inset > 0);
				}
				else
					p_context -> fillarc(p_mark -> arc . bounds, p_mark -> arc . start, p_mark -> arc . angle);
			break;
			
			case MARK_TYPE_IMAGE:
				p_context -> drawimage(p_mark -> image . descriptor, p_mark -> image . sx, p_mark -> image . sy, p_mark -> image . sw, p_mark -> image . sh, p_mark -> image . dx, p_mark -> image . dy);
			break;
			
			case MARK_TYPE_METAFILE:
				p_context -> drawpict((uint1 *)p_mark -> metafile . data, p_mark -> metafile . data_length, false, p_mark -> metafile . dst_area, p_mark -> metafile . src_area);
			break;
			
			case MARK_TYPE_THEME:
				p_context -> drawtheme(p_mark -> theme . type, (MCThemeDrawInfo*)&p_mark -> theme . info);
			break;

			case MARK_TYPE_PATH:
			{
				MCPath *t_path;
				t_path = MCPath::create_path(p_mark -> path . commands, p_mark -> path . command_count, p_mark -> path . ordinates, p_mark -> path . ordinate_count);
				if (p_mark -> stroke != NULL)
					p_context -> drawpath(t_path);
				else
					p_context -> fillpath(t_path, p_mark -> path . evenodd);
				t_path -> release();
			}
			break;
				
			case MARK_TYPE_END:
			case MARK_TYPE_EPS:
			case MARK_TYPE_LINK:
			case MARK_TYPE_GROUP:
				break;
		}
	}

	return t_finished;
}

void MCMetaContext::execute(void)
{
	executegroup(f_state_stack -> root);
}

void MCMetaContext::executegroup(MCMark *p_group_mark)
{
	for(MCMark *t_mark = p_group_mark -> group . head; t_mark != NULL; t_mark = t_mark -> next)
	{
		if (!candomark(t_mark))
		{
			// Compute the region of the destination we need to rasterize.
			MCRectangle t_dst_clip;
			if (t_mark -> type != MARK_TYPE_GROUP || t_mark -> group . effects == NULL)
				t_dst_clip = t_mark -> clip;
			else
				MCBitmapEffectsComputeBounds(t_mark -> group . effects, t_mark -> group . effects_shape, t_dst_clip);

			// Get a raster context for the given clip - but only if there is something to
			// render.
			if (t_dst_clip . width != 0 && t_dst_clip . height != 0)
			{
				bool t_success;
				t_success = true;
				
				MCGContextRef t_context;
				t_context = nil;
				
				MCContext *t_gfx_context;
				t_gfx_context = nil;
				
				MCRegionRef t_clip_region;
				t_clip_region = nil;
				
				if (t_success)
					t_success = begincomposite(t_dst_clip, t_context);
				
				if (t_success)
					t_success = nil != (t_gfx_context = new (nothrow) MCGraphicsContext(t_context));
				
				if (t_success)
				{
					t_gfx_context -> setprintmode();
					
					// First render just the group we are interested in, so we can clip out any pixels not
					// affected by it.
					mark_indirect(t_gfx_context, t_mark, nil, t_dst_clip);
					
					// Compute the region touched by non-transparent pixels.
					t_clip_region = t_gfx_context -> computemaskregion();
					t_gfx_context -> clear(nil);
					
					// MW-2007-11-28: [[ Bug 4873 ]] Failure to reset the context state here means the first
					//   objects rendered up until a group are all wrong!
					t_gfx_context -> setfunction(GXcopy);
					t_gfx_context -> setopacity(255);
					t_gfx_context -> clearclip();
					
					// Render all marks from the bottom up to and including the current mark - clipped
					// by the dst bounds.
                    // PM-2014-11-25: [[ Bug 14093 ]] nil-check to prevent a crash
					for(MCMark *t_raster_mark = f_state_stack -> root -> group . head; t_raster_mark != t_mark -> next && t_raster_mark != NULL; t_raster_mark = t_raster_mark -> next)
						if (mark_indirect(t_gfx_context, t_raster_mark, t_mark, t_dst_clip))
							break;
				}
				
				if (t_gfx_context != nil)
					delete t_gfx_context;
				
				endcomposite(t_clip_region);
			}
		}
		else
			domark(t_mark);
	}
}

void MCMetaContext::new_fill_foreground(void)
{
	if (f_fill_foreground_used || f_fill_foreground == NULL)
	{
		MCMarkFill *t_new_fill = f_heap . allocate<MCMarkFill>();
		if (t_new_fill != NULL)
		{
			t_new_fill -> style = FillSolid;
			t_new_fill -> colour = getblack();
			t_new_fill -> pattern = NULL;
			t_new_fill -> origin . x = 0;
			t_new_fill -> origin . y = 0;
			t_new_fill -> gradient = NULL;

			t_new_fill -> previous = f_fill_foreground;
			f_fill_foreground = t_new_fill;
		}
		
		f_fill_foreground_used = false;
	}
}

void MCMetaContext::new_fill_background(void)
{
	if (f_fill_background_used || f_fill_background == NULL)
	{
		MCMarkFill *t_new_fill = f_heap . allocate<MCMarkFill>();
		if (t_new_fill != NULL)
		{
			t_new_fill -> style = FillSolid;
			t_new_fill -> colour = getwhite();
			t_new_fill -> pattern = NULL;
			t_new_fill -> origin . x = 0;
			t_new_fill -> origin . y = 0;
			t_new_fill -> gradient = NULL;

			t_new_fill -> previous = f_fill_background;
			f_fill_background = t_new_fill;
		}
		
		f_fill_background_used = false;
	}
}

void MCMetaContext::new_stroke(void)
{
	if (f_stroke_used || f_stroke == NULL)
	{
		f_stroke = f_heap . allocate<MCMarkStroke>();
		if (f_stroke != NULL)
		{
			f_stroke -> width = 0;
			f_stroke -> style = LineSolid;
			f_stroke -> cap = CapButt;
			f_stroke -> join = JoinBevel;
			f_stroke -> dash . data = NULL;
			f_stroke -> dash . length = 0;
			f_stroke -> dash . offset = 0;
			f_stroke -> miter_limit = 10.0;
		}
		
		f_stroke_used = false;
	}
}

template<typename T> T *MCMetaContext::new_array(uint4 p_size)
{
	return f_heap . heap_t::allocate_array<T>(p_size);
}

MCMark *MCMetaContext::new_mark(uint4 p_type, bool p_stroke, bool p_fill)
{
	static uint4 s_mark_sizes[] =
	{
		0,
		sizeof(MCMarkLine),
		sizeof(MCMarkPolygon),
		sizeof(MCMarkText),
		sizeof(MCMarkRectangle),
		sizeof(MCMarkRoundRectangle),
		sizeof(MCMarkArc),
		sizeof(MCMarkImage),
		sizeof(MCMarkMetafile),
		0,
		sizeof(MCMarkTheme),
		sizeof(MCMarkGroup),
		sizeof(MCMarkLink),
		sizeof(MCMarkPath),
	};
	
	// IM-2013-06-11: dynamically allocate sufficient memory for a
	// MCThemeDrawInfo struct (previous fixed size of 64bytes was too
	// small.
	uint32_t t_mark_size;
	t_mark_size = sizeof(MCMarkHeader) + s_mark_sizes[p_type];
	if (p_type == MARK_TYPE_THEME && MCcurtheme != nil)
		t_mark_size += MCcurtheme->getthemedrawinfosize();
		
	MCMark *t_mark;
	t_mark = f_heap . allocate<MCMark>(t_mark_size);
	if (t_mark != NULL)
	{
		if (p_stroke)
		{
			if (f_stroke == NULL)
				new_stroke();
		}
		
		if (p_stroke || p_fill)
		{
				if (f_fill_foreground == NULL)
					new_fill_foreground();
		}
		
		t_mark -> type = (MCMarkType)p_type;
		t_mark -> stroke = (p_stroke ? f_stroke : NULL);
		t_mark -> fill = (p_stroke || p_fill ? f_fill_foreground : NULL);
		
		if (p_stroke)
			f_stroke_used = true;
		
		if (p_fill || p_stroke)
			f_fill_foreground_used = true;
			
		t_mark -> clip = f_clip;
		
		if (f_state_stack != NULL)
		{
			assert(f_state_stack -> root != NULL && f_state_stack -> root -> type == MARK_TYPE_GROUP);

			t_mark -> previous = f_state_stack -> root -> group . tail;
			t_mark -> next = NULL;
			
			if (t_mark -> previous != NULL)
				t_mark -> previous -> next = t_mark;
			else
				f_state_stack -> root -> group . head = t_mark;
				
			f_state_stack -> root -> group . tail = t_mark;
		}
	}
	
	return t_mark;
}

void MCMetaContext::rectangle_mark(bool p_stroke, bool p_fill, const MCRectangle& rect, bool inside)
{
	MCMark *t_mark;
	t_mark = new_mark(MARK_TYPE_RECTANGLE, p_stroke, p_fill);
	if (t_mark != NULL)
	{
		t_mark -> rectangle . bounds = rect;
        // MM-2014-04-23: [[ Bug 11884 ]] Store by how much we want to inset (rather than we just want to inset).
        // SN-2014-08-25: [[ Bug 13187 ]] Makes sure that there is an f_stroke for this context
		t_mark -> rectangle . inset = (inside && f_stroke) ? f_stroke -> width : 0 ;
	}
}

void MCMetaContext::round_rectangle_mark(bool p_stroke, bool p_fill, const MCRectangle& rect, uint2 radius, bool inside)
{
	MCMark *t_mark;
	t_mark = new_mark(MARK_TYPE_ROUND_RECTANGLE, p_stroke, p_fill);
	if (t_mark != NULL)
	{
		t_mark -> round_rectangle . bounds = rect;
		t_mark -> round_rectangle . radius = radius;
        // MM-2014-04-23: [[ Bug 11884 ]] Store by how much we want to inset (rather than we just want to inset).
        // SN-2014-08-25: [[ Bug 13187 ]] Makes sure that there is an f_stroke for this context
		t_mark -> round_rectangle . inset = (inside && f_stroke) ? f_stroke -> width : 0 ;
	}
}

void MCMetaContext::polygon_mark(bool p_stroke, bool p_fill, MCPoint *p_vertices, uint2 p_arity, bool p_closed)
{
	MCMark *t_mark;
	
	t_mark = new_mark(MARK_TYPE_POLYGON, p_stroke, p_fill);
	if (t_mark != NULL)
	{
		t_mark -> polygon . count = p_arity;
		t_mark -> polygon . vertices = new_array<MCPoint>(p_arity);
		if (t_mark -> polygon . vertices != NULL)
			memcpy(t_mark -> polygon . vertices, p_vertices, sizeof(MCPoint) * p_arity);
		t_mark -> polygon . closed = p_closed;
	}
}

void MCMetaContext::arc_mark(bool p_stroke, bool p_fill, const MCRectangle& p_bounds, uint2 p_start, uint2 p_angle, bool p_complete, bool inside)
{
	MCMark *t_mark;
	
	t_mark = new_mark(MARK_TYPE_ARC, p_stroke, p_fill);
	if (t_mark != NULL)
	{
		t_mark -> arc . bounds = p_bounds;
		t_mark -> arc . start = p_start;
		t_mark -> arc . angle = p_angle;
		t_mark -> arc . complete = p_complete;
        // MM-2014-04-23: [[ Bug 11884 ]] Store by how much we want to inset (rather than we just want to inset).
        // SN-2014-08-25: [[ Bug 13187 ]] Makes sure that there is an f_stroke for this context
		t_mark -> arc . inset = (inside && f_stroke) ? f_stroke -> width : 0 ;
	}
}

void MCMetaContext::path_mark(bool p_stroke, bool p_fill, MCPath *p_path, bool p_evenodd)
{
	MCMark *t_mark;
	t_mark = new_mark(MARK_TYPE_PATH, p_stroke, p_fill);
	if (t_mark != NULL)
	{
		p_path -> get_lengths(t_mark -> path . command_count, t_mark -> path . ordinate_count);
		t_mark -> path . evenodd = p_evenodd;
		t_mark -> path . commands = new_array<uint1>(t_mark -> path . command_count);
		t_mark -> path . ordinates = new_array<int4>(t_mark -> path . ordinate_count);
		if (t_mark -> path . commands != NULL)
			memcpy(t_mark -> path . commands, p_path -> get_commands(), t_mark -> path . command_count);
		if (t_mark -> path . ordinates != NULL)
			memcpy(t_mark -> path . ordinates, p_path -> get_ordinates(), t_mark -> path . ordinate_count * sizeof(int4));
	}
}
