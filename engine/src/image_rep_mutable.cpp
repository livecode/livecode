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
#include "mcio.h"

#include "execpt.h"
#include "util.h"
#include "undolst.h"
#include "sellst.h"
#include "image.h"
#include "button.h"
#include "stack.h"
#include "card.h"
#include "mcerror.h"
#include "objectstream.h"

#include "context.h"

#include "globals.h"

////////////////////////////////////////////////////////////////////////////////

extern void surface_mask_flush_to_alpha(void *p_alpha_ptr, uint4 p_alpha_stride, void * p_mask_ptr, uint4 p_mask_stride, uint4 sw, uint4 sh);
extern void surface_mask_flush_to_alpha_inverted(void *p_alpha_ptr, uint4 p_alpha_stride, void * p_mask_ptr, uint4 p_mask_stride, uint4 sw, uint4 sh) ;

extern void surface_merge_with_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);
extern void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);

extern void surface_extract_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_width, uint4 p_height, uint1 p_threshold);
extern void surface_extract_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);

extern void surface_unmerge_pre(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height);

void surface_combine_blendSrcOver_masked(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity);

////////////////////////////////////////////////////////////////////////////////

#define MIN_SELRECT 3

#define PEMPTY MAXINT2
#define PSTACKSIZE 4096

////////////////////////////////////////////////////////////////////////////////

Boolean MCMutableImageRep::erasing;

Pixmap MCMutableImageRep::editmask;
Pixmap MCMutableImageRep::editdata;
MCBitmap *MCMutableImageRep::editimage;
MCBitmap *MCMutableImageRep::editmaskimage;
MCBitmap *MCMutableImageRep::editmaskimagealpha;

Tool MCMutableImageRep::oldtool = T_UNDEFINED;
MCRectangle MCMutableImageRep::newrect;

MCPoint *MCMutableImageRep::points = NULL;
uint2 MCMutableImageRep::npoints = 0;
uint2 MCMutableImageRep::polypoints;

////////////////////////////////////////////////////////////////////////////////

bool MCMutableImageRep::LockImageFrame(uindex_t p_frame, MCImageFrame *&r_frame)
{
	if (p_frame > 0)
		return false;

	m_frame.image = m_bitmap;

	Retain();

	r_frame = &m_frame;

	return true;
}

void MCMutableImageRep::UnlockImageFrame(uindex_t p_index, MCImageFrame *p_frame)
{
	if (p_index > 0 || p_frame != &m_frame)
		return;

	m_frame.image = nil;

	Release();
}

uindex_t MCMutableImageRep::GetFrameCount()
{
	return 1;
}

bool MCMutableImageRep::GetGeometry(uindex_t &r_width, uindex_t &r_height)
{
	r_width = m_bitmap->width;
	r_height = m_bitmap->height;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCMutableImageRep::MCMutableImageRep(MCImage *p_owner, MCImageBitmap *p_bitmap)
{
	m_owner = p_owner;
	rect = m_owner->getrect();

	/* UNCHECKED */ MCImageCopyBitmap(p_bitmap, m_bitmap);
	m_selection_image = nil;
	m_undo_image = nil;
	m_rub_image = nil;

	mx = my = 0;
	state = 0;

	m_frame.image = nil;
	m_frame.duration = 0;
}

MCMutableImageRep::~MCMutableImageRep()
{
	MCImageFreeBitmap(m_bitmap);
	MCImageFreeBitmap(m_selection_image);
	MCImageFreeBitmap(m_undo_image);
	MCImageFreeBitmap(m_rub_image);

	MCscreen->freepixmap(editmask);
	MCscreen->freepixmap(editdata);
	if (editimage != nil)
		MCscreen->destroyimage(editimage);
	if (editmaskimage != nil)
		MCscreen->destroyimage(editmaskimage);
	if (editmaskimagealpha != nil)
		MCscreen->destroyimage(editmaskimagealpha);

	editmask = editdata = nil;
	editimage = editmaskimage = editmaskimagealpha = nil;
}

////////////////////////////////////////////////////////////////////////////////

bool MCMutableImageRep::copy_selection(MCImageBitmap *&r_bitmap)
{
	if (!has_selection())
		return MCImageCopyBitmap(m_bitmap, r_bitmap);
	else if (state & CS_BEEN_MOVED)
		return MCImageCopyBitmap(m_selection_image, r_bitmap);
	else
		return MCImageCopyBitmapRegion(m_bitmap, selrect, r_bitmap);
}

////////////////////////////////////////////////////////////////////////////////

bool MCMutableImageRep::has_selection()
{
	return state & CS_OWN_SELECTION;
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCMutableImageRep::image_mfocus(int2 x, int2 y)
{
	mx = x; my = y;

	if (state & (CS_DRAW | CS_DRAG))
	{
		continuedraw();
		return True;
	}

	if (state & CS_OWN_SELECTION)
		if (m_owner->getstack() -> gettool(m_owner) == T_SELECT)
			return True;
		else
			endsel();
			
	return False;
}

Boolean MCMutableImageRep::image_mdown(uint2 which)
{
	switch(which)
	{
	case Button3:
		if (!(state & CS_DRAW))
			erasing = True;
	case Button1:
		if (state & CS_DRAW)
		{
			if (mx == points[0].x && my == points[0].y
			        || which == Button1 && erasing
			        || which == Button3 && !erasing)
			{
				if (mx == points[0].x && my == points[0].y)
					points[polypoints++] = points[0];
				enddraw();
				erasing = False;
			}
			else
				if (polypoints < MCscreen->getmaxpoints() - 1)
				{
					points[polypoints].x = mx;
					points[polypoints].y = my;
					polypoints++;

					MCImageFreeBitmap(m_rub_image);
					/* UNCHECKED */ MCImageCopyBitmap(m_bitmap, m_rub_image);

					startx = mx;
					starty = my;
				}
			return True;
		}
		else
		{
			switch (m_owner->getstack()->gettool(m_owner))
			{
			case T_POLYGON:
				MCscreen->grabpointer(m_owner->getw());
			default:
				startdraw();
				break;
			}
			return True;
		}
		default:
			break;
	}
	
	return False;
}

Boolean MCMutableImageRep::image_doubledown(uint2 which)
{
	if (state & CS_DRAW && (which == Button1 || which == Button3 && erasing)
	        && m_owner->getstack() -> gettool(m_owner) == T_POLYGON)
		return True;
	return False;
}

Boolean MCMutableImageRep::image_doubleup(uint2 which)
{
	if (state & CS_DRAW && (which == Button1 || which == Button3 && erasing)
	        && m_owner->getstack() -> gettool(m_owner) == T_POLYGON)
	{
		enddraw();
		erasing = False;
		return True;
	}
	return False;
}

Boolean MCMutableImageRep::image_mup(uint2 which)
{
	switch(m_owner->getstack() -> gettool(m_owner))
	{
	case T_BROWSE:
	case T_IMAGE:
	case T_POINTER:
	case T_HELP:
		return False;
	case T_POLYGON:
		return True;
	default:
		if (state & (CS_DRAW | CS_DRAG | CS_MAG_DRAG))
			enddraw();
		erasing = False;
		return True;
	}
	return False;
}

///////////////////////////////////////////////////////////////////////////////

void MCMutableImageRep::drawsel(MCDC *dc)
{
	if (state & CS_OWN_SELECTION)
	{
		if (state & CS_BEEN_MOVED)
		{
			MCImageDescriptor t_image;
			memset(&t_image, 0, sizeof(MCImageDescriptor));
			t_image . bitmap = m_selection_image;
			dc -> drawimage(t_image, 0, 0, selrect . width, selrect . height, selrect . x + rect . x, selrect . y + rect . y);
		}
		drawselrect(dc);
	}
}

void MCMutableImageRep::drawselrect(MCDC *dc)
{
	if (MCdragging)
		return;
	dc->setlineatts(0, LineDoubleDash, CapButt, JoinBevel);
	dc->setforeground(MCscreen->getblack());
	dc->setbackground(MCscreen->getwhite());

	uint16_t t_dashcount;
	uint8_t *t_dashlist;
	int8_t t_dashoffset;
	t_dashcount = m_owner->getdashes(t_dashlist, t_dashoffset);

	dc->setdashes(t_dashoffset, t_dashlist, t_dashcount);
	MCRectangle trect = selrect;
	trect.x += rect.x;
	trect.y += rect.y;
	dc->drawrect(trect);
	dc->setlineatts(0, LineSolid, CapButt, JoinBevel);
	dc->setbackground(MCzerocolor);
}

///////////////////////////////////////////////////////////////////////////////

bool MCImageCreateOpaqueDrawmask(uindex_t p_width, uindex_t p_height, Pixmap &r_drawmask)
{
	bool t_success = true;

	MCBitmap *t_mask = nil;
	Pixmap t_drawmask = nil;
	t_success = nil != (t_mask = MCscreen->createimage(1, p_width, p_height, True, 0xFF, False, False));
	if (t_success)
		t_success = nil != (t_drawmask = MCscreen->createpixmap(p_width, p_height, 1, False));
	if (t_success)
		MCscreen->putimage(t_drawmask, t_mask, 0, 0, p_width, p_height, 0, 0);

	if (t_success)
		r_drawmask = t_drawmask;

	if (t_mask != nil)
		MCscreen->destroyimage(t_mask);

	return t_success;
}

//////////

void MCMutableImageRep::startdraw()
{
	MCeditingimage = m_owner;

	Tool t_tool = m_owner->getstack()->gettool(m_owner);

	startx = mx;
	starty = my;
	MCRectangle brect;
	brect.width = brect.height = 0;
	if (MCactiveimage != NULL && MCactiveimage != m_owner
	        && t_tool == T_SELECT)
		MCactiveimage->endsel();

	state |= CS_EDITED;
	if (!MCscreen->getlockmods())
	{
		MCundos->freestate();
		Ustruct *us = new Ustruct;
		us->type = UT_PAINT;
		MCundos->savestate(m_owner, us);
		MCImageFreeBitmap(m_undo_image);
		m_undo_image = nil;
		/* UNCHECKED */ MCImageCopyBitmap(m_bitmap, m_undo_image);
	}
	switch (t_tool)
	{
	case T_BRUSH:
	case T_SPRAY:
	case T_ERASER:
		// Note that in brush/spray/eraser mode, we accumulate the mask that has
		// been affected - the alpha channel has no influence on this and so no
		// editmaskalphaimage need be created.
		editmask = MCscreen->createpixmap(rect.width, rect.height, 1, False);
		editdata = MCscreen->createpixmap(rect.width, rect.height, 0, False);
		editimage = MCscreen->createimage(1, rect.width, rect.height, True, 0x00, True, False);
		brect = drawbrush(t_tool);
		state |= CS_DRAW;
		break;
	case T_BUCKET:
		if (MCU_point_in_rect(rect, mx, my))
			drawbucket();
		brect = rect;
		break;
	case T_POLYGON:
		startrub();
		brect = drawline(True);
	case T_CURVE:
		if (points != NULL)
			delete points;
		points = new MCPoint[MCscreen->getmaxpoints()];
		npoints = MCscreen->getmaxpoints();
		points[0].x = mx;
		points[0].y = my;
		polypoints = 1;
		state |= CS_DRAW;
		break;
	case T_LASSO:
		break;
	case T_LINE:
		startrub();
		brect = drawline(True);
		break;
	case T_OVAL:
		startrub();
		brect = drawoval();
		break;
	case T_PENCIL:
		state |= CS_DRAW;
		brect = drawpencil();
		break;
	case T_RECTANGLE:
		startrub();
		brect = drawrectangle();
		break;
	case T_REGULAR_POLYGON:
		startrub();
		brect = drawreg();
		break;
	case T_ROUND_RECT:
		startrub();
		brect = drawroundrect();
		break;
	case T_SELECT:
		if (MCactiveimage != NULL && MCactiveimage != m_owner)
			MCactiveimage->endsel();
		if (state & CS_OWN_SELECTION)
		{
			if (MCU_point_in_rect(selrect, mx - rect.x, my - rect.y))
			{
				// MW-2007-12-05: [[ Bug 1247 ]] Grid spacing not respected by select tool
				MCU_snap(mx);
				MCU_snap(my);
				startx = mx;
				starty = my;
				startseldrag();
				break;
			}
			else
			{
				if (oldtool != T_UNDEFINED)
				{
					MCcurtool = oldtool;
					endsel();
					return;
				}
				endsel();
			}
		}
		// MW-2007-12-05: [[ Bug 1247 ]] Grid spacing not respected by select tool
		MCU_snap(mx);
		MCU_snap(my);
		startx = mx;
		starty = my;
		selrect.x = mx - rect.x;
		selrect.y = my - rect.y;
		selrect.width = selrect.height = 1;
		brect.width = brect.height = 1;
		selrect = MCU_bound_rect(selrect, rect.x, rect.y, rect.width, rect.height);
		state |= CS_DRAW | CS_OWN_SELECTION;

		MCactiveimage = m_owner;
		MCscreen->addtimer(m_owner, MCM_internal, MCmovespeed);
		break;
	case T_TEXT:
		break;
	default:
		break;
	}
	if (brect.width != 0 && brect.height != 0)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the brush rect.
		m_owner->invalidate_rep(brect);
	}
}

void MCMutableImageRep::continuedraw()
{
	MCRectangle brect;
	Boolean all = False;
	brect.width = brect.height = 0;

	switch(m_owner->getstack()->gettool(m_owner))
	{
	case T_BRUSH:
		brect = drawbrush(T_BRUSH);
		startx = mx;
		starty = my;
		break;
	case T_SPRAY:
		brect = drawbrush(T_SPRAY);
		startx = mx;
		starty = my;
		break;
	case T_DROPPER:
		break;
	case T_CURVE:
		if (polypoints < MCscreen->getmaxpoints() - 1)
		{
			points[polypoints].x = mx;
			points[polypoints].y = my;

			MCRectangle trect = MCU_compute_rect(mx - rect.x, my - rect.y,
			                                     points[polypoints - 1].x - rect.x,
			                                     points[polypoints - 1].y - rect.y);
			MCRectangle t_clip;
			t_clip = MCU_reduce_rect(trect, -((MClinesize >> 1) + 1));

			uint16_t pixwidth, pixheight;
			pixwidth = m_bitmap->width; pixheight = m_bitmap->height;
			Pixmap drawdata = nil, drawmask = nil;
			MCBitmap *maskimagealpha = nil;
			/* UNCHECKED */ MCImageSplitPixmaps(m_bitmap, drawdata, drawmask, maskimagealpha);
			if (drawmask == nil && erasing)
				/* UNCHECKED */ MCImageCreateOpaqueDrawmask(pixwidth, pixheight, drawmask);
				
			MCContext *t_context;
			if (drawmask != nil)
			{
				t_context = MCscreen -> createcontext(drawmask);
				t_context->setforeground(fixmaskcolor(MConecolor));
				t_context->setclip(t_clip);
				pattson(t_context, False, 1);
				t_context->drawline(mx - rect.x, my - rect.y,
								   points[polypoints - 1].x - rect.x,
								   points[polypoints - 1].y - rect.y);
				pattsoff(t_context, 1);
				MCscreen -> freecontext(t_context);
			}

			if ( maskimagealpha != NULL ) 
				t_context = MCscreen -> createcontext ( drawdata, maskimagealpha );
			else 
				t_context = MCscreen -> createcontext(drawdata);

			
			t_context -> setclip(t_clip);
			pattson(t_context, False, 0);
			t_context->drawline(mx - rect.x, my - rect.y,
			                   points[polypoints - 1].x - rect.x,
			                   points[polypoints - 1].y - rect.y);
			pattsoff(t_context, 0);
			MCscreen -> freecontext(t_context);

			MCImageFreeBitmap(m_bitmap);
			/* UNCHECKED */ MCImageMergePixmaps(pixwidth, pixheight, drawdata, drawmask, maskimagealpha, m_bitmap);
			MCscreen->freepixmap(drawdata);
			MCscreen->freepixmap(drawmask);
			if (maskimagealpha != nil)
				MCscreen->destroyimage(maskimagealpha);

			brect = MCU_compute_rect(points[polypoints-1].x,
			                         points[polypoints-1].y, mx, my);
			brect = MCU_reduce_rect(brect, -((MClinesize >> 1) + 1));
			polypoints++;
		}
		else
			brect.width = brect.height = 0;
		break;
	case T_ERASER:
		brect = drawbrush(T_ERASER);
		startx = mx;
		starty = my;
		all = True;
		break;
	case T_LASSO:
		break;
	case T_LINE:
		brect = drawline(True);
		break;
	case T_POLYGON:
		brect = drawline(False);
		break;
	case T_OVAL:
		brect = drawoval();
		break;
	case T_PENCIL:
		brect = drawpencil();
		startx = mx;
		starty = my;
		break;
	case T_RECTANGLE:
		brect = drawrectangle();
		break;
	case T_REGULAR_POLYGON:
		brect = drawreg();
		break;
	case T_ROUND_RECT:
		brect = drawroundrect();
		break;
	case T_SELECT:
		MCU_snap(mx);
		MCU_snap(my);
		if (state & CS_DRAG)
		{
			brect = selrect;
			selrect.x += mx - startx;
			selrect.y += my - starty;
			int2 oldx = selrect.x;
			int2 oldy = selrect.y;
			selrect = MCU_bound_rect(selrect, 0, 0, rect.width, rect.height);
			brect = MCU_union_rect(brect, selrect);
			brect.x += rect.x;
			brect.y += rect.y;
			startx = mx + (selrect.x - oldx);
			starty = my + (selrect.y - oldy);
			all = True;
		}
		else
		{
			int2 oldx = startx;
			int2 oldy = starty;
			if (MCmodifierstate & MS_SHIFT)
				if (MCU_abs(oldx - mx) > MCU_abs(oldy - my))
					if (oldx > mx)
						mx = oldx - MCU_abs(my - oldy);
					else
						mx = oldx + MCU_abs(my - oldy);
				else
					if (oldy > my)
						my = oldy - MCU_abs(mx - oldx);
					else
						my = oldy + MCU_abs(mx - oldx);
			if (MCcentered)
			{
				oldx -= mx - oldx;
				oldy -= my - oldy;
			}
			brect = selrect;
			brect.x += rect.x;
			brect.y += rect.y;
			selrect = MCU_compute_rect(oldx, oldy, mx, my);
			selrect = MCU_clip_rect(selrect, rect.x, rect.y,
			                        rect.width, rect.height);
			brect = MCU_union_rect(brect, selrect);
			selrect.x -= rect.x;
			selrect.y -= rect.y;
		}
		break;
	default:
		break;
	}

	if (brect.width != 0 && brect.height != 0)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the brush rect.
		m_owner->invalidate_rep(brect);
	}
}

void MCMutableImageRep::enddraw()
{
	state &= ~CS_DRAW;
	Tool t_tool = m_owner->getstack()->gettool(m_owner);
	switch (t_tool)
	{
	case T_BRUSH:
	case T_SPRAY:
	case T_ERASER:
		if (editimage != nil)
			MCscreen->destroyimage(editimage);
		editimage = nil;
		MCscreen->freepixmap(editmask);
		MCscreen->freepixmap(editdata);
		break;
	case T_POLYGON:
		endrub();
		MCscreen->ungrabpointer();
	case T_CURVE:
		{
			MCU_offset_points(points, polypoints, -rect.x, -rect.y);

			uint16_t pixwidth, pixheight;
			pixwidth = m_bitmap->width; pixheight = m_bitmap->height;
			Pixmap drawdata = nil, drawmask = nil;
			MCBitmap *maskimagealpha = nil;
			/* UNCHECKED */ MCImageSplitPixmaps(m_bitmap, drawdata, drawmask, maskimagealpha);

			if (MCfilled && polypoints > 2)
			{
				if (points[polypoints - 1].x != points[0].x
						|| points[polypoints - 1].y != points[0].y)
					points[polypoints++] = points[0];

				MCContext *t_context;

				if (drawmask != nil)
				{
					t_context = MCscreen -> createcontext(drawmask);
					t_context->setforeground(fixmaskcolor(MConecolor));
					battson(t_context, 1);
					t_context->fillpolygon(points, polypoints);
					battsoff(t_context, 1);
					MCscreen -> freecontext(t_context);
				}

				if ( maskimagealpha != NULL ) 
					t_context = MCscreen -> createcontext ( drawdata, maskimagealpha );
				else 
					t_context = MCscreen -> createcontext(drawdata);

				battson(t_context, 0);
				t_context->fillpolygon(points, polypoints);
				battsoff(t_context, 0);
				MCscreen -> freecontext(t_context);
			}
			if (polypoints > 1)
			{
				MCContext *t_context;

				if (drawmask != nil)
				{
					t_context = MCscreen -> createcontext(drawmask);
					t_context->setforeground(fixmaskcolor(MConecolor));
					pattson(t_context, False, 1);
					t_context->drawlines(points, polypoints);
					pattsoff(t_context, 1);
					MCscreen -> freecontext(t_context);
				}

				if ( maskimagealpha != NULL ) 
					t_context = MCscreen -> createcontext ( drawdata, maskimagealpha );
				else 
					t_context = MCscreen -> createcontext(drawdata);

				pattson(t_context, False, 0);
				t_context->drawlines(points, polypoints);
				pattsoff(t_context, 0);
				MCscreen -> freecontext(t_context);
			}

			MCImageFreeBitmap(m_bitmap);
			/* UNCHECKED */ MCImageMergePixmaps(pixwidth, pixheight, drawdata, drawmask, maskimagealpha, m_bitmap);
			MCscreen->freepixmap(drawdata);
			MCscreen->freepixmap(drawmask);
			if (maskimagealpha != nil)
				MCscreen->destroyimage(maskimagealpha);

			delete points;
			points = NULL;
			npoints = 0;

			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			m_owner->invalidate_rep(rect);
		}
		break;
	case T_LASSO:
		break;
	case T_LINE:
		if (!erasing)
			drawline(true);
		endrub();
		break;
	case T_OVAL:
		drawoval();
		endrub();
		break;
	case T_RECTANGLE:
		drawrectangle();
		endrub();
		break;
	case T_REGULAR_POLYGON:
		drawreg();
		endrub();
		break;
	case T_ROUND_RECT:
		drawroundrect();
		endrub();
		break;
	case T_SELECT:
		if (state & CS_DRAG)
			state &= ~CS_DRAG;
		else
		{
			MCImageFreeBitmap(m_selection_image);
			m_selection_image = nil;
		}
		if (selrect.width < MIN_SELRECT || selrect.height < MIN_SELRECT)
			endsel();
		break;
	case T_TEXT:
		break;
	default:
		break;
	}
}

void MCMutableImageRep::canceldraw()
{
	if ((state & CS_DRAW) != 0)
		enddraw();
}

////////////////////////////////////////////////////////////////////////////////

void MCMutableImageRep::startrub()
{
	state |= CS_DRAW;
	MCU_snap(mx);
	MCU_snap(my);
	startx = newrect.x = mx;
	starty = newrect.y = my;
	newrect.width = newrect.height = 1;
	newrect.x -= rect.x;
	newrect.y -= rect.y;
	if (MCdragging)
		return;

	MCAssert(m_rub_image == nil);
	/* UNCHECKED */ MCImageCopyBitmap(m_bitmap, m_rub_image);
}

MCRectangle MCMutableImageRep::continuerub(Boolean line)
{
	int2 oldx = startx;
	int2 oldy = starty;

	MCU_snap(mx);
	MCU_snap(my);
	MCRectangle brect = newrect;
	if (MCmodifierstate & MS_SHIFT)
		if (line)
		{
			real8 dx = (real8)(mx - oldx);
			real8 dy = (real8)(my - oldy);
			real8 length = sqrt(dx * dx + dy * dy);
			real8 angle = atan2(dy, dx);
			real8 quanta = M_PI * 2.0 / (real8)MCslices;
			angle = floor((angle + quanta / 2.0) / quanta) * quanta;
			mx = oldx + (int2)(cos(angle) * length);
			my = oldy + (int2)(sin(angle) * length);
			newrect = MCU_compute_rect(oldx, oldy, mx, my);
		}
		else
			if (MCU_abs(oldx - mx) > MCU_abs(oldy - my))
				if (oldx > mx)
					mx = oldx - MCU_abs(my - oldy);
				else
					mx = oldx + MCU_abs(my - oldy);
			else
				if (oldy > my)
					my = oldy - MCU_abs(mx - oldx);
				else
					my = oldy + MCU_abs(mx - oldx);
	if (MCcentered)
	{
		oldx -= mx - oldx;
		oldy -= my - oldy;
	}
	newrect = MCU_compute_rect(oldx, oldy, mx, my);
	newrect.x -= rect.x;
	newrect.y -= rect.y;
	brect = MCU_union_rect(brect, newrect);
	brect = MCU_reduce_rect(brect, -((MClinesize >> 1) + 1));
	brect.width++;
	brect.height++;

	MCPoint t_dst;
	t_dst.x = brect.x; t_dst.y = brect.y;
	MCImageBitmapCopyRegionToBitmap(m_bitmap, m_rub_image, t_dst, brect);

	brect.x += rect.x;
	brect.y += rect.y;
	return brect;
}

void MCMutableImageRep::endrub()
{
	if (MCdragging)
		return;

	MCImageFreeBitmap(m_rub_image);
	m_rub_image = nil;
}

////////////////////////////////////////////////////////////////////////////////

void MCMutableImageRep::startseldrag()
{
	MCLog("MCMutableImageRep<%p>::startseldrag()", this);
	state |= CS_DRAG;
	if (state & CS_BEEN_MOVED)
	{
		if (MCmodifierstate & MS_CONTROL)
			stampsel();
	}
	else
		getsel(!(MCmodifierstate & MS_CONTROL));
}

void MCMutableImageRep::endsel()
{
	MCLog("MCMutableImageRep<%p>::endsel()", this);
	if (state & CS_BEEN_MOVED)
	{
		stampsel();
		MCImageFreeBitmap(m_selection_image);
		m_selection_image = nil;
	}
	state &= ~(CS_BEEN_MOVED | CS_OWN_SELECTION);
	MCactiveimage = NULL;
	selrect.x += rect.x;
	selrect.y += rect.y;
	// MW-2011-08-18: [[ Layers ]] Invalidate the selected rect.
	m_owner->invalidate_rep(selrect);

	oldtool = T_UNDEFINED;
}

////////////////////////////////////////////////////////////////////////////////

void MCMutableImageRep::pattson(MCContext *p_context, Boolean miter, uint2 depth)
{
	int4 lstyle, lends, jstyle;
	if (MCdashes)
	{
		lstyle = LineOnOffDash;
		p_context->setdashes(0, MCdashes, MCndashes);
	}
	else
		lstyle = LineSolid;
	if (MCroundends)
	{
		lends = CapRound;
		jstyle = JoinRound;
	}
	else
	{
		lends = CapProjecting;
		if (miter)
			jstyle = JoinMiter;
		else
			jstyle = JoinBevel;
	}
	p_context->setlineatts(MClinesize == 0 ? 1 : MClinesize, lstyle, lends, jstyle);
	if (erasing)
	{
		p_context->setforeground(fixmaskcolor(MCzerocolor));
		p_context->setfillstyle(FillSolid, DNULL, 0, 0);
	}
	else
		if (depth != 1)
			if (MCmodifierstate & MS_MOD1)
				battson(p_context, depth);
			else
				if (MCpenpm == DNULL)
				{
					p_context->setforeground(MCpencolor);
					p_context->setfillstyle(FillSolid, DNULL, 0, 0);
				}
				else
					p_context->setfillstyle(FillTiled, MCpenpm, 0, 0);
}

void MCMutableImageRep::pattsoff(MCContext *p_context, uint2 depth)
{
	p_context->setlineatts(0, LineSolid, CapButt, JoinBevel);
	if (erasing)
		p_context->setforeground(fixmaskcolor(MConecolor));
	else
		if (depth != 1)
			if (MCmodifierstate & MS_MOD1)
				battsoff(p_context, depth);
}

void MCMutableImageRep::battson(MCContext *p_context, uint2 depth)
{
	if (erasing)
		p_context->setforeground(fixmaskcolor(MCzerocolor));
	else
		if (depth != 1)
			if (MCbrushpm == DNULL)
			{
				p_context->setforeground(MCbrushcolor);
				p_context->setfillstyle(FillSolid, DNULL, 0, 0);
			}
			else
				p_context->setfillstyle(FillTiled, MCbrushpm, 0, 0);
}

void MCMutableImageRep::battsoff(MCContext *p_context, uint2 depth)
{
	if (erasing)
		p_context->setforeground(fixmaskcolor(MConecolor));
	else
		if (depth != 1)
			if (MCbrushpm != DNULL)
				p_context->setfillstyle(FillSolid, DNULL, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////

void MCMutableImageRep::put_brush(MCBitmap *image, int2 x, int2 y,
                        uint2 maxwidth, uint2 maxheight, MCbrushmask *bmptr)
{
	uint4 sx, sy ;
	uint4 sw, sh ;
	uint2 bwidth = bmptr->width;
	uint2 bheight = bmptr->height;
	int2 mw = (int2)maxwidth;
	int2 mh = (int2)maxheight;

	x -= bmptr->xhot - 1;
	y -= bmptr->yhot - 1;
	int2 bx, by;
	if (x < 0)
	{
		if (x <= -bwidth)
			return;
		bx = -x;
		bwidth -= bx;
		x = 0;
	}
	else
	{
		bx = 0;
		if (x + bwidth > maxwidth)
		{
			if (x >= mw)
				return;
			bwidth = maxwidth - x;
		}
	}
	if (y < 0)
	{
		if (y <= -bheight)
			return;
		by = -y;
		bheight -= by;
		y = 0;
	}
	else
	{
		by = 0;
		if (y + bheight > maxheight)
		{
			if (y >= mh)
				return;
			bheight = maxheight - y;
		}
	}
	
	// Take a copy of these values before we start messing around with em
	sx = x ;
	sy = y ;
	sw = bmptr -> width ;
	sh = bmptr -> height ;
	
	uint1 *dptr ;
	uint1 *sptr ;
	
	
	dptr = (uint1 *)&image->data[y * image->bytes_per_line + (x >> 3)];
	sptr = &bmptr->data[by * bmptr->bytes + (bx >> 3)];
	
	if (bx == 0)
	{
		x &= 0x07;
		while (bheight--)
		{
			uint1 *tdptr = dptr;
			uint1 *tsptr = sptr;
			int2 width = bwidth - (8 - x);
			*tdptr++ |= *tsptr >> x;
			while (width >= 8)
			{
				*tdptr++ |= *tsptr << (8 - x) | *(tsptr + 1) >> x;
				width -= 8;
				tsptr++;
			}
			if (width)
				*tdptr |= *tsptr << (8 - x);
			dptr += image->bytes_per_line;
			sptr += bmptr->bytes;
		}
	}
	else
	{
		bx &= 0x07;
		while (bheight--)
		{
			uint1 *tdptr = dptr;
			uint1 *tsptr = sptr;
			int2 width = bwidth;
			while (width > 8)
			{
				*tdptr++ |= *tsptr << bx | *(tsptr + 1) >> (8 - bx);
				width -= 8;
				tsptr++;
			}
			*tdptr |= *tsptr << bx;
			dptr += image->bytes_per_line;
			sptr += bmptr->bytes;
		}
	}
}

MCRectangle MCMutableImageRep::drawbrush(Tool which)
{
	MCbrushmask *bmptr = NULL;

	bmptr = MCImage::getbrush(which);

	MCscreen->flipimage(editimage, MSBFirst, MSBFirst);
	if (which == T_SPRAY)
	{
		newrect = MCU_compute_rect(mx, my, mx, my);
		newrect.x -= rect.x;
		newrect.y -= rect.y;
		put_brush(editimage, newrect.x, newrect.y,
		          rect.width, rect.height, bmptr);
	}
	else
	{
		int2 yinc;
		int2 x, y;
		int2 dx, dy;
		int2 e, e1, e2;

		newrect = MCU_compute_rect(startx, starty, mx, my);
		if (newrect.x == startx)
			if (newrect.y == starty)
				yinc = 1;
			else
				yinc = -1;
		else
			if (newrect.y == my)
				yinc = 1;
			else
				yinc = -1;
		newrect.x -= rect.x;
		newrect.y -= rect.y;
		x = newrect.x;
		if (yinc == 1)
			y = newrect.y;
		else
			y = newrect.y + newrect.height - 1;
		dx = newrect.width - 1;
		dy = newrect.height - 1;
		if (dx >= dy)
		{
			e2 = dy << 1;
			e = e2 - dx;
			e1 = e - dx;
			while (dx-- >= 0)
			{
				put_brush(editimage, x++, y, rect.width, rect.height, bmptr);
				if (e > 0)
				{
					y += yinc;
					e += e1;
				}
				else
					e += e2;
			}
		}
		else
		{
			e2 = dx << 1;
			e = e2 - dy;
			e1 = e - dy;
			while (dy-- >= 0)
			{
				put_brush(editimage, x, y, rect.width, rect.height, bmptr);
				y += yinc;
				if (e > 0)
				{
					x++;
					e += e1;
				}
				else
					e += e2;
			}
		}
	}
	newrect.x -= bmptr->xhot;
	newrect.y -= bmptr->yhot;
	newrect.width += bmptr->width;
	newrect.height += bmptr->height;
	newrect = MCU_clip_rect(newrect, 0, 0, rect.width, rect.height);
	if (which == T_ERASER || erasing)
		eraseimage(newrect);
	else
		fillimage(newrect);
	newrect.x += rect.x;
	newrect.y += rect.y;
	return newrect;
}

void MCMutableImageRep::fill_line(MCBitmap *plane, int2 left, int2 right, int2 y)
{
	uint1 *sptr = (uint1 *)&plane->data[y * plane->bytes_per_line + (left>>3)];
	uint1 *eptr = (uint1 *)&plane->data[y * plane->bytes_per_line + (right>>3)];

	if (sptr == eptr)
		*sptr |= MCleftmasks[left & 0x07] & MCrightmasks[right & 0x07];
	else
	{
		*sptr++ |= MCleftmasks[left & 0x07];
		while (sptr < eptr)
			*sptr++ |= 0xFF;
		*sptr |= MCrightmasks[right & 0x07];
	}
}

bool MCMutableImageRep::bucket_line(MCImageBitmap *p_src, uint4 color,
                             int2 x, int2 y, int2 &l, int2 &r)
{
	l = r = x;
	if (y < 0 || y >= p_src->height || x < 0 || x >= p_src->width)
		return false;

	uint32_t *t_src_row = (uint32_t*)((uint8_t*)p_src->data + y * p_src->stride) + x ;
	if (*t_src_row != color)
		return false;
	if (l)
	{
		uint32_t *t_left_ptr = t_src_row;
		while (l-- && *--t_left_ptr == color)
			;
		l++;
	}
	if (r < p_src->width - 1)
	{
		uint32_t *t_right_ptr = t_src_row;
		while (r++ < p_src->width - 1 && *++t_right_ptr == color)
			;
		r--;
	}

	return true;
}

bool MCMutableImageRep::bucket_point(MCImageBitmap *p_src, uint4 color, MCBitmap *dimage,
                              MCstacktype pstack[], uint2 &pstacktop,
                              uint2 &pstackptr, int2 xin, int2 yin,
                              int2 direction, int2 &xleftout, int2 &xrightout,
                              bool &collide)
{
	collide = false;
	uint2 pstacktmp;
	uint2 pstackhole;

	if (bucket_line(p_src, color, xin, yin, xleftout, xrightout))
	{
		for (pstacktmp = 0, pstackhole = PEMPTY ;
		        pstacktmp < pstacktop ; pstacktmp++)
		{
			if (pstack[pstacktmp].lx == xleftout
			        && pstack[pstacktmp].y == yin)
			{
				fill_line(dimage, xleftout, xrightout, yin);
				pstack[pstacktmp].y = PEMPTY;
				if ((pstacktmp + 1) == pstacktop)
				{
					while (pstacktop && pstack[pstacktop - 1].y == PEMPTY)
						pstacktop--;
					if (pstackptr >= pstacktop)
						pstackptr = 0;
				}
				collide = true;
				return false;
			}
			if (pstack[pstacktmp].y == PEMPTY && pstackhole == PEMPTY)
				pstackhole = pstacktmp;
		}

		if (pstackhole == PEMPTY)
		{
			if (++pstacktop >= PSTACKSIZE)
				return false;
		}
		else
			pstacktmp = pstackhole;
		pstack[pstacktmp].y = yin;
		pstack[pstacktmp].lx = xleftout;
		pstack[pstacktmp].rx = xrightout;
		pstack[pstacktmp].direction = direction;
		return true;
	}
	else
		return false;
}

void MCMutableImageRep::bucket_fill(MCImageBitmap *p_src, uint4 scolor, MCBitmap *dimage,
                          int2 xleft, int2 oldy)
{
	int2 oldxleft;
	int2 oldxright;

	bool gotpoint = bucket_line(p_src, scolor, xleft, oldy, oldxleft, oldxright);
	if (!gotpoint)
		return;

	MCstacktype *pstack = new MCstacktype[PSTACKSIZE];
	uint2 pstackptr = 0;
	uint2 pstacktop = 1;
	bool collision;

	pstack[0].y = oldy;
	pstack[0].lx = oldxleft;
	pstack[0].rx = oldxright;
	pstack[0].direction = 1;
	int2 direction = -1;

	bool getpoint = false;

	do
	{
		if (getpoint)
		{
			while (pstack[pstackptr].y == PEMPTY)
			{
				pstackptr++;
				if (pstackptr == pstacktop)
					pstackptr = 0;
			}

			oldy = pstack[pstackptr].y;
			direction = pstack[pstackptr].direction;
			pstack[pstackptr].y = PEMPTY;
			oldxleft = pstack[pstackptr].lx;
			oldxright = pstack[pstackptr++].rx;
			if (pstackptr == pstacktop)
			{
				while (pstacktop && pstack[pstacktop - 1].y == PEMPTY)
					pstacktop--;
				if (pstackptr >= pstacktop)
					pstackptr = 0;
			}
			fill_line(dimage, oldxleft, oldxright, oldy);
		}
		getpoint = True;
		int2 newxleft;
		int2 newxright;
		int2 xright;
		gotpoint = bucket_point(p_src, scolor, dimage, pstack,
			pstacktop, pstackptr,
			oldxleft, oldy + direction, direction,
			newxleft, newxright, collision);

		int2 leftdirection = direction;
		bool leftpoint = gotpoint;
		bool leftcollision = collision;
		int2 leftoldy = oldy;
		while (newxleft < (oldxleft - 1) && (leftpoint || leftcollision))
		{
			xleft = oldxleft;
			while (xleft > newxleft)
				leftpoint = bucket_point(p_src, scolor, dimage, pstack, pstacktop,
				pstackptr, --xleft, leftoldy, -leftdirection,
				xleft, xright, leftcollision);
			oldxleft = newxleft;
			if ((xleft < (newxleft - 1)) && (leftpoint || leftcollision))
			{
				newxleft = xleft;
				leftoldy += leftdirection;
				leftdirection = -leftdirection;
			}
		}
		while (newxright < oldxright)
		{
			gotpoint = bucket_point(p_src, scolor, dimage, pstack, pstacktop,
				pstackptr, ++newxright, oldy + direction,
				direction, xleft, newxright, collision);
		}
		while ((newxright > (oldxright + 1)) && (gotpoint || collision))
		{
			xright = oldxright;
			while (xright < newxright)
			{
				gotpoint = bucket_point(p_src, scolor, dimage, pstack, pstacktop,
					pstackptr, ++xright, oldy, -direction,
					xleft, xright, collision);
			}
			oldxright = newxright;
			if ((xright > (newxright + 1)) && (gotpoint || collision))
			{
				newxright = xright;
				oldy += direction;
				direction = -direction;
			}
		}
	}
	while (pstacktop);
	delete pstack;
}

void MCMutableImageRep::drawbucket()
{
	MCU_watchcursor(m_owner->getstack(), True);
	uint32_t t_color = MCImageBitmapGetPixel(m_bitmap, mx - rect.x, my - rect.y);
	MCRectangle trect;
	MCU_set_rect(trect, 0, 0, rect.width, rect.height);
	editmask = MCscreen->createpixmap(rect.width, rect.height, 1, False);
	editdata = MCscreen->createpixmap(rect.width, rect.height, 0, False);
	editimage = MCscreen->createimage(1, rect.width, rect.height,
	                                  True, 0x0, True, False);
	bucket_fill(m_bitmap, t_color, editimage, mx - rect.x, my - rect.y);
	if (erasing)
		eraseimage(trect);
	else
		fillimage(trect);
	MCscreen->destroyimage(editimage);
	editimage = nil;
	MCscreen->freepixmap(editmask);
	MCscreen->freepixmap(editdata);
	MCU_unwatchcursor(m_owner->getstack(), True);
}

MCRectangle MCMutableImageRep::drawline(Boolean cancenter)
{
		
	MCRectangle brect = continuerub(True);
	int2 oldx = startx;
	int2 oldy = starty;
	if (cancenter && MCcentered)
	{
		oldx -= mx - startx;
		oldy -= my - starty;
	}

	uint16_t pixwidth, pixheight;
	pixwidth = m_bitmap->width; pixheight = m_bitmap->height;
	Pixmap drawdata = nil, drawmask = nil;
	MCBitmap *maskimagealpha = nil;
	/* UNCHECKED */ MCImageSplitPixmaps(m_bitmap, drawdata, drawmask, maskimagealpha);
	if (drawmask == nil && erasing)
		/* UNCHECKED */ MCImageCreateOpaqueDrawmask(pixwidth, pixheight, drawmask);

	MCContext *t_context;

	if (drawmask != nil)
	{
		t_context = MCscreen -> createcontext(drawmask);
		t_context->setforeground(fixmaskcolor(MConecolor));
		pattson(t_context, False, 1);
		t_context->drawline(oldx - rect.x, oldy - rect.y, mx - rect.x, my - rect.y);
		pattsoff(t_context, 1);
		MCscreen -> freecontext(t_context);
	}

	if ( maskimagealpha != NULL ) 
		t_context = MCscreen -> createcontext ( drawdata, maskimagealpha );
	else 
		t_context = MCscreen -> createcontext(drawdata);

	pattson(t_context, False, 0);
	t_context->drawline(oldx - rect.x, oldy - rect.y, mx - rect.x, my - rect.y);
	pattsoff(t_context, 0);
	MCscreen -> freecontext(t_context);

	MCImageFreeBitmap(m_bitmap);
	/* UNCHECKED */ MCImageMergePixmaps(pixwidth, pixheight, drawdata, drawmask, maskimagealpha, m_bitmap);
	MCscreen->freepixmap(drawdata);
	MCscreen->freepixmap(drawmask);
	if (maskimagealpha != nil)
		MCscreen->destroyimage(maskimagealpha);

	return brect;
}

MCRectangle MCMutableImageRep::drawreg()
{
	real8 dx = (real8)(mx - startx);
	real8 dy = (real8)(my - starty);
	real8 length = sqrt(dx * dx + dy * dy);
	real8 angle;
	if (dx == 0 && dy == 0)
		angle = 0;
	else
		angle = atan2(dy, dx);
	if (MCmodifierstate & MS_SHIFT)
	{
		real8 quanta = M_PI * 2.0 / (real8)MCslices;
		angle = floor((angle + quanta / 2.0) / quanta) * quanta;
		mx = startx + (int2)(cos(angle) * length);
		my = starty + (int2)(sin(angle) * length);
	}
	if (npoints <= MCpolysides)
	{
		npoints = MCpolysides + 1;
		if (points != NULL)
			delete points;
		points = new MCPoint[npoints];
	}
	points[0].x = points[MCpolysides].x = mx;
	points[0].y = points[MCpolysides].y = my;
	int2 minx = mx;
	int2 miny = my;
	int2 maxx = mx;
	int2 maxy = my;
	real8 factor = 2.0 * M_PI / (real8)MCpolysides;
	uint2 i;
	for (i = 1 ; i < MCpolysides ; i++)
	{
		real8 iangle = angle + (real8)i * factor;
		points[i].x = startx + (int2)(cos(iangle) * length);
		points[i].y = starty + (int2)(sin(iangle) * length);
		minx = MCU_min(minx, points[i].x);
		miny = MCU_min(miny, points[i].y);
		maxx = MCU_max(maxx, points[i].x);
		maxy = MCU_max(maxy, points[i].y);
	}
	MCU_offset_points(points, MCpolysides + 1, -rect.x, -rect.y);

	MCRectangle brect = newrect;
	newrect = MCU_compute_rect(minx, miny, maxx, maxy);
	newrect.x -= rect.x;
	newrect.y -= rect.y;
	brect = MCU_union_rect(brect, newrect);
	brect = MCU_reduce_rect(brect, -((MClinesize >> 1) + 1));

	// 2013-01-22-IM - restore the image data to the previous state before drawing
	// - the other drawing tools do the same thing by calling continuerub, but
	// reg poly can't use that due to its different way of dragging out the shape.

	MCPoint t_dst;
	t_dst.x = brect.x;
	t_dst.y = brect.y;
	MCImageBitmapCopyRegionToBitmap(m_bitmap, m_rub_image, t_dst, brect);

	uint16_t pixwidth, pixheight;
	pixwidth = m_bitmap->width; pixheight = m_bitmap->height;
	Pixmap drawdata = nil, drawmask = nil;
	MCBitmap *maskimagealpha = nil;
	/* UNCHECKED */ MCImageSplitPixmaps(m_bitmap, drawdata, drawmask, maskimagealpha);
	if (drawmask == nil && erasing)
		/* UNCHECKED */ MCImageCreateOpaqueDrawmask(pixwidth, pixheight, drawmask);

	brect.x += rect.x;
	brect.y += rect.y;

	MCContext *t_context;

	if (drawmask != nil)
	{
		t_context = MCscreen -> createcontext(drawmask);
		t_context->setforeground(fixmaskcolor(MConecolor));
		if (MCfilled)
		{
			battson(t_context, 1);
			t_context->fillpolygon(points, MCpolysides + 1);
			battsoff(t_context, 1);
		}
		if (MClinesize != 0)
		{
			pattson(t_context, True, 1);
			t_context->drawlines(points, MCpolysides + 1);
			pattsoff(t_context, 1);
		}
		MCscreen -> freecontext(t_context);
	}

	// Make sure we target the alpha channel if present.
	if (maskimagealpha != NULL) 
		t_context = MCscreen -> createcontext(drawdata, maskimagealpha);
	else 
		t_context = MCscreen -> createcontext(drawdata);

	if (MCfilled)
	{
		battson(t_context, 0);
		t_context->fillpolygon(points, MCpolysides + 1);
		battsoff(t_context, 0);
	}
	if (MClinesize != 0)
	{
		pattson(t_context, True, 0);
		t_context->drawlines(points, MCpolysides + 1);
		pattsoff(t_context, 0);
	}
	MCscreen -> freecontext(t_context);

	MCImageFreeBitmap(m_bitmap);
	/* UNCHECKED */ MCImageMergePixmaps(pixwidth, pixheight, drawdata, drawmask, maskimagealpha, m_bitmap);
	MCscreen->freepixmap(drawdata);
	MCscreen->freepixmap(drawmask);
	if (maskimagealpha != nil)
		MCscreen->destroyimage(maskimagealpha);

	return brect;
}

MCRectangle MCMutableImageRep::drawroundrect()
{
	MCRectangle brect = continuerub(False);
	MCU_roundrect(points, npoints, newrect, MCroundradius);

	uint16_t pixwidth, pixheight;
	pixwidth = m_bitmap->width; pixheight = m_bitmap->height;
	Pixmap drawdata = nil, drawmask = nil;
	MCBitmap *maskimagealpha = nil;
	/* UNCHECKED */ MCImageSplitPixmaps(m_bitmap, drawdata, drawmask, maskimagealpha);
	if (drawmask == nil && erasing)
		/* UNCHECKED */ MCImageCreateOpaqueDrawmask(pixwidth, pixheight, drawmask);

	MCContext *t_context;
	if (drawmask != nil)
	{
		t_context = MCscreen -> createcontext(drawmask);
		t_context->setforeground(fixmaskcolor(MConecolor));
		if (MCfilled)
		{
			battson(t_context, 1);
			t_context->fillpolygon(points, npoints);
			battsoff(t_context, 1);
		}
		if (MClinesize != 0)
		{
			pattson(t_context, True, 1);
			t_context->drawlines(points, npoints);
			pattsoff(t_context, 1);
		}
		MCscreen -> freecontext(t_context);
	}

	// Make sure we target the alpha channel if present.
	if ( maskimagealpha != NULL ) 
		t_context = MCscreen -> createcontext ( drawdata, maskimagealpha );
	else 
		t_context = MCscreen -> createcontext(drawdata);
	
	
	if (MCfilled)
	{
		battson(t_context, 0);
		t_context->fillpolygon(points, npoints);
		battsoff(t_context, 0);
	}
	if (MClinesize != 0)
	{
		pattson(t_context, True, 0);
		t_context->drawlines(points, npoints);
		pattsoff(t_context, 0);
	}
	MCscreen -> freecontext(t_context);

	MCImageFreeBitmap(m_bitmap);
	/* UNCHECKED */ MCImageMergePixmaps(pixwidth, pixheight, drawdata, drawmask, maskimagealpha, m_bitmap);
	MCscreen->freepixmap(drawdata);
	MCscreen->freepixmap(drawmask);
	if (maskimagealpha != nil)
		MCscreen->destroyimage(maskimagealpha);

	return brect;
}

MCRectangle MCMutableImageRep::drawoval()
{
	MCContext *t_context;

	MCRectangle brect = continuerub(False);
	
	uint16_t pixwidth, pixheight;
	pixwidth = m_bitmap->width; pixheight = m_bitmap->height;
	Pixmap drawdata = nil, drawmask = nil;
	MCBitmap *maskimagealpha = nil;
	/* UNCHECKED */ MCImageSplitPixmaps(m_bitmap, drawdata, drawmask, maskimagealpha);
	if (drawmask == nil && erasing)
		/* UNCHECKED */ MCImageCreateOpaqueDrawmask(pixwidth, pixheight, drawmask);

	if (drawmask != nil)
	{
		t_context = MCscreen -> createcontext(drawmask);
		t_context -> setforeground(fixmaskcolor(MConecolor));
		if (MCfilled)
		{
			battson(t_context, 1);
			t_context -> fillarc(newrect, MCstartangle, MCarcangle);
			battsoff(t_context, 1);
		}
		if (MClinesize != 0)
		{
			pattson(t_context, False, 1);
			t_context -> drawarc(newrect, MCstartangle, MCarcangle);
			pattsoff(t_context, 1);
		}
		MCscreen -> freecontext(t_context);
	}

	if ( maskimagealpha != NULL ) 
		t_context = MCscreen -> createcontext ( drawdata, maskimagealpha );
	else 
		t_context = MCscreen -> createcontext(drawdata);
	
	if (MCfilled)
	{
		battson(t_context, 0);
		t_context->fillarc(newrect, MCstartangle, MCarcangle);
		battsoff(t_context, 0);
	}
	if (MClinesize != 0)
	{
		pattson(t_context, False, 0);
		t_context->drawarc(newrect, MCstartangle, MCarcangle);
		pattsoff(t_context, 0);
	}
	MCscreen -> freecontext(t_context);

	MCImageFreeBitmap(m_bitmap);
	/* UNCHECKED */ MCImageMergePixmaps(pixwidth, pixheight, drawdata, drawmask, maskimagealpha, m_bitmap);
	MCscreen->freepixmap(drawdata);
	MCscreen->freepixmap(drawmask);
	if (maskimagealpha != nil)
		MCscreen->destroyimage(maskimagealpha);

	return brect;
}

MCRectangle MCMutableImageRep::drawpencil()
{
	MCContext *t_context;

	uint16_t pixwidth, pixheight;
	pixwidth = m_bitmap->width; pixheight = m_bitmap->height;
	Pixmap drawdata = nil, drawmask = nil;
	MCBitmap *maskimagealpha = nil;
	/* UNCHECKED */ MCImageSplitPixmaps(m_bitmap, drawdata, drawmask, maskimagealpha);
	if (drawmask == nil && erasing)
		/* UNCHECKED */ MCImageCreateOpaqueDrawmask(pixwidth, pixheight, drawmask);

	if (drawmask != nil)
	{
		t_context = MCscreen -> createcontext(drawmask);
		t_context -> setforeground(erasing ? fixmaskcolor(MCzerocolor) : fixmaskcolor(MConecolor));
		t_context -> drawline(startx - rect . x, starty - rect . y, mx - rect . x, my - rect . y);
		MCscreen -> freecontext(t_context);
	}
	
	if ( maskimagealpha != NULL ) 
		t_context = MCscreen -> createcontext ( drawdata, maskimagealpha);
	else 
		t_context = MCscreen -> createcontext(drawdata);
	t_context -> setforeground(erasing ? fixmaskcolor(MCzerocolor) : MCpencolor );
	t_context -> drawline(startx - rect . x, starty - rect . y, mx - rect . x, my - rect . y);
	
	MCscreen -> freecontext(t_context);

	if ( maskimagealpha != NULL && erasing )
	{
		Drawable t_mask;
		t_mask = MCscreen -> createpixmap ( rect . width, rect . height, 1 , true ) ;
		
		t_context = MCscreen -> createcontext(t_mask);
		t_context -> setforeground(fixmaskcolor(MCzerocolor));

		MCRectangle t_rect;
		MCU_set_rect(t_rect, 0, 0, rect . width, rect . height);
		t_context -> fillrect(t_rect);

		t_context -> setforeground(fixmaskcolor(MConecolor));
		t_context -> drawline(startx - rect . x, starty - rect . y, mx - rect . x, my - rect . y);
		MCscreen -> freecontext(t_context);
		
		MCBitmap *t_image;
		t_image = MCscreen -> getimage(t_mask, 0, 0, rect . width, rect . height, False );
		
		surface_mask_flush_to_alpha_inverted(maskimagealpha -> data, maskimagealpha -> bytes_per_line, t_image->data, t_image -> bytes_per_line, t_image -> width, t_image -> height);
		if (t_image != NULL)
			MCscreen -> destroyimage(t_image);

		if (t_mask != DNULL)
			MCscreen -> freepixmap(t_mask);
	}

	MCImageFreeBitmap(m_bitmap);
	/* UNCHECKED */ MCImageMergePixmaps(pixwidth, pixheight, drawdata, drawmask, maskimagealpha, m_bitmap);
	MCscreen->freepixmap(drawdata);
	MCscreen->freepixmap(drawmask);
	if (maskimagealpha != nil)
		MCscreen->destroyimage(maskimagealpha);

	return MCU_compute_rect(startx, starty, mx, my);
}

MCRectangle MCMutableImageRep::drawrectangle()
{
	MCContext *t_context;

	MCRectangle brect = continuerub(False);

	uint16_t pixwidth, pixheight;
	pixwidth = m_bitmap->width; pixheight = m_bitmap->height;
	Pixmap drawdata = nil, drawmask = nil;
	MCBitmap *maskimagealpha = nil;
	/* UNCHECKED */ MCImageSplitPixmaps(m_bitmap, drawdata, drawmask, maskimagealpha);
	if (drawmask == nil && erasing)
		/* UNCHECKED */ MCImageCreateOpaqueDrawmask(pixwidth, pixheight, drawmask);

	if (drawmask != nil)
	{
		t_context = MCscreen -> createcontext(drawmask);
		t_context->setforeground(fixmaskcolor(MConecolor));
		if (MCfilled)
		{
			battson(t_context, 1);
			t_context->fillrect(newrect);
			battsoff(t_context, 1);
		}
		if (MClinesize != 0)
		{
			pattson(t_context, False, 1);
			t_context->drawrect(newrect);
			pattsoff(t_context, 1);
		}
		MCscreen -> freecontext(t_context);
	}

	if ( maskimagealpha != NULL ) 
		t_context = MCscreen -> createcontext ( drawdata, maskimagealpha );
	else 
		t_context = MCscreen -> createcontext(drawdata);

	if (MCfilled)
	{
		battson(t_context, 0);
		t_context->fillrect(newrect);
		battsoff(t_context, 0);
	}
	if (MClinesize != 0)
	{
		pattson(t_context, False, 0);
		t_context->drawrect(newrect);
		pattsoff(t_context, 0);
	}
	MCscreen -> freecontext(t_context);

	MCImageFreeBitmap(m_bitmap);
	/* UNCHECKED */ MCImageMergePixmaps(pixwidth, pixheight, drawdata, drawmask, maskimagealpha, m_bitmap);
	MCscreen->freepixmap(drawdata);
	MCscreen->freepixmap(drawmask);
	if (maskimagealpha != nil)
		MCscreen->destroyimage(maskimagealpha);

	return brect;
}

void MCMutableImageRep::fillimage(const MCRectangle &drect)
{
	uint16_t pixwidth, pixheight;
	pixwidth = m_bitmap->width; pixheight = m_bitmap->height;
	Pixmap drawdata = nil, drawmask = nil;
	MCBitmap *maskimagealpha = nil;
	/* UNCHECKED */ MCImageSplitPixmaps(m_bitmap, drawdata, drawmask, maskimagealpha);

	// Render the 1 bit mask pixmap into the 1-bit editmask IMAGE
	MCscreen->putimage(editmask, editimage, drect.x, drect.y,
					   drect.x, drect.y, drect.width, drect.height);
		
	if (drawmask != nil)
	{
		// Copy the editmask into the drawmask. We do this with a logical OR operation
		// as we are merging the filled mask with that which is already present.
		MCscreen->copyarea(editmask, drawmask, 1, drect.x, drect.y,
						   drect.width, drect.height,
						   drect.x, drect.y, fixmaskrop(GXor));
	}
		
	// Copy the editmask into the editdata, expanding 1's to 0xFFFFFFFF and
	// 0's to 0x00000000.
#ifdef _MACOSX
	MCscreen->copyplane(editmask, editdata, drect.x, drect.y, drect.width,
						drect.height, drect.x, drect.y, GXcopy,
						(uint4)0 );//needs to be 0x000000 on Mac
#else
	MCscreen->copyplane(editmask, editdata, drect.x, drect.y, drect.width,
						drect.height, drect.x, drect.y, GXcopy,
						(uint4)AllPlanes );//needs to be 0x000000 on Mac
#endif
	
	// Copy the editdata into the drawdata using a logical inverted and. This
	// has the effect of setting all pixels inside the mask (i.e. where it is 1)
	// to 0 and leaving the rest unaffected. After this operation, the target
	// color data has a black space where the new filled shape is to go.
	MCscreen->copyarea(editdata, drawdata, 0, drect.x, drect.y,
	                   drect.width, drect.height,
	                   drect.x, drect.y, GXandInverted);

	// Now fill the editdata with the fill pattern. This is done by using the
	// logical and operation. This results in the appropriate pattern appearing
	// in the editdata but only inside the mask.
	MCContext *t_context;
	t_context = MCscreen -> createcontext(editdata);
	battson(t_context, 0);
	
	// MW-2011-09-15: [[ Bug 9724 ]] Reinstate per-platform hooks - seems the
	//   AND operation doesn't quite work right except on Windows...
#if defined(_MACOSX)
	extern void MCMacApplyPatternToFillImage(MCContext *, const MCRectangle&);
	MCMacApplyPatternToFillImage(t_context, drect);
#elif defined(_LINUX)
	extern void MCLinuxApplyPatternToFillImage(MCContext *, const MCRectangle&);
	MCLinuxApplyPatternToFillImage(t_context, drect);
#else
	t_context->setfunction(GXand);
	t_context -> fillrect(drect);
#endif
	
	battsoff(t_context, 0);
	MCscreen -> freecontext(t_context);

	// Now logically OR the color pattern in editdata with the colors in drawdata.
	// As drawdata contains 0's where the new filled shape is to go, and editdata
	// only contains non-0's inside the filled shape this works as expected.
#ifdef _MACOSX 
	MCscreen->copyplane(editdata, drawdata, drect.x, drect.y, drect.width,
	                    drect.height, drect.x, drect.y, GXor,
	                    MCbrushpm == DNULL ? MCbrushcolor.pixel : (uint4)AllPlanes);
#else
	//Finally, copy the editdata back to the drawdata again to finish.
	MCscreen->copyarea(editdata, drawdata, 0, drect.x, drect.y,
	                   drect.width, drect.height, drect.x, drect.y, GXor);
#endif

	// Now, if the target image has an alpha channel we ensure that any pixels that
	// were just filled by the above sequence are maked as fully opaque in the alpha.
	if (maskimagealpha != NULL)
		surface_mask_flush_to_alpha(maskimagealpha -> data, maskimagealpha->bytes_per_line, editimage->data, editimage->bytes_per_line, editimage->width, editimage->height);	

	MCImageFreeBitmap(m_bitmap);
	/* UNCHECKED */ MCImageMergePixmaps(pixwidth, pixheight, drawdata, drawmask, maskimagealpha, m_bitmap);
	MCscreen->freepixmap(drawdata);
	MCscreen->freepixmap(drawmask);
	if (maskimagealpha != nil)
		MCscreen->destroyimage(maskimagealpha);
}

void MCMutableImageRep::eraseimage(const MCRectangle &drect)
{
	uint16_t pixwidth, pixheight;
	pixwidth = m_bitmap->width; pixheight = m_bitmap->height;
	Pixmap drawdata = nil, drawmask = nil;
	MCBitmap *maskimagealpha = nil;
	/* UNCHECKED */ MCImageSplitPixmaps(m_bitmap, drawdata, drawmask, maskimagealpha);
	if (drawmask == nil)
		/* UNCHECKED */ MCImageCreateOpaqueDrawmask(pixwidth, pixheight, drawmask);

	MCscreen->putimage(editmask, editimage, drect.x, drect.y,
	                   drect.x, drect.y, drect.width, drect.height);
	MCscreen->copyarea(editmask, drawmask, 1, drect.x, drect.y,
	                   drect.width, drect.height,
	                   drect.x, drect.y, fixmaskrop(GXandInverted));
	MCscreen->copyplane(editmask, editdata, drect.x, drect.y, drect.width,
	                    drect.height, drect.x, drect.y, GXcopy,
	                    (uint4)AllPlanes);
	MCscreen->copyarea(editdata, drawdata, 0, drect.x, drect.y,
	                   drect.width, drect.height,
	                   drect.x, drect.y, GXandInverted);
	
	if ( maskimagealpha != NULL)
		surface_mask_flush_to_alpha_inverted ( maskimagealpha -> data, maskimagealpha->bytes_per_line, editimage->data, editimage->bytes_per_line, editimage->width, editimage->height);

	MCImageFreeBitmap(m_bitmap);
	/* UNCHECKED */ MCImageMergePixmaps(pixwidth, pixheight, drawdata, drawmask, maskimagealpha, m_bitmap);
	MCscreen->freepixmap(drawdata);
	MCscreen->freepixmap(drawmask);
	if (maskimagealpha != nil)
		MCscreen->destroyimage(maskimagealpha);
}

////////////////////////////////////////////////////////////////////////////////

bool opaque_scan_horizontal(MCImageBitmap *p_bitmap, uindex_t p_x, uindex_t p_y, uindex_t p_length, uint8_t p_threshold)
{
	uint32_t *t_src_ptr = (uint32_t*)((uint8_t*)p_bitmap->data + p_y * p_bitmap->stride) + p_x;

	while (p_length--)
		if (((*t_src_ptr++) >> 24) > p_threshold)
			return true;

	return false;
}

bool opaque_scan_vertical(MCImageBitmap *p_bitmap, uindex_t p_x, uindex_t p_y, uindex_t p_length, uint8_t p_threshold)
{
	uint8_t *t_src_ptr = (uint8_t*)p_bitmap->data + p_y * p_bitmap->stride + p_x * sizeof(uint32_t);

	while (p_length--)
	{
		if (((*(uint32_t*)t_src_ptr) >> 24) > p_threshold)
			return true;
		t_src_ptr += p_bitmap->stride;
	}

	return false;
}

MCRectangle MCImageBitmapGetOpaqueBounds(MCImageBitmap *p_bitmap, uint8_t p_threshold)
{
	if (!MCImageBitmapHasTransparency(p_bitmap))
		return MCU_make_rect(0, 0, p_bitmap->width, p_bitmap->height);

	uindex_t t_top, t_bottom, t_left, t_right;
	bool t_have_top, t_have_bottom, t_have_left, t_have_right;

	t_top = 0;
	t_bottom = p_bitmap->height - 1;
	t_left = 0;
	t_right = p_bitmap->width - 1;

	t_have_top = t_have_bottom = t_have_left = t_have_right = false;

	while (!(t_have_top && t_have_bottom && t_have_left && t_have_right))
	{
		if (t_top > t_bottom || t_left > t_right)
			return MCU_make_rect(0, 0, 0, 0);

		// find topmost opaque pixel
		if (!t_have_top)
		{
			t_have_top = opaque_scan_horizontal(p_bitmap, t_left, t_top, t_right - t_left + 1, p_threshold);
			if (!t_have_top)
				t_top++;
		}
		// find bottommost opaque pixel
		if (!t_have_bottom)
		{
			t_have_bottom = opaque_scan_horizontal(p_bitmap, t_left, t_bottom, t_right - t_left + 1, p_threshold);
			if (!t_have_bottom)
				t_bottom--;
		}
		// find leftmost opaque pixel
		if (!t_have_left)
		{
			t_have_left = opaque_scan_vertical(p_bitmap, t_left, t_top, t_bottom - t_top + 1, p_threshold);
			if (!t_have_left)
				t_left++;
		}
		// find rightmost opaque pixel
		if (!t_have_right)
		{
			t_have_right = opaque_scan_vertical(p_bitmap, t_right, t_top, t_bottom - t_top + 1, p_threshold);
			if (!t_have_right)
				t_right--;
		}
	}

	return MCU_make_rect(t_left, t_top, t_right - t_left + 1, t_bottom - t_top + 1);
}

MCRectangle MCMutableImageRep::getopaqueregion(uint1 p_threshold)
{
	MCRectangle t_rect = MCImageBitmapGetOpaqueBounds(m_bitmap, p_threshold);
	t_rect.x += rect.x;
	t_rect.y += rect.y;
	return t_rect;
}

void MCMutableImageRep::croptoopaque()
{
	MCImageBitmap *t_cropped = nil;

	MCRectangle t_opaque_rect = MCImageBitmapGetOpaqueBounds(m_bitmap, 0);
	/* UNCHECKED */ MCImageCopyBitmapRegion(m_bitmap, t_opaque_rect, t_cropped);
	MCImageFreeBitmap(m_bitmap);
	m_bitmap = t_cropped;

	rect.x += t_opaque_rect.x;
	rect.y += t_opaque_rect.y;
	rect.width = t_opaque_rect.width;
	rect.height = t_opaque_rect.height;
	m_owner->sourcerectchanged(rect);
}

////////////////////////////////////////////////////////////////////////////////

void MCMutableImageRep::selimage()
{
	MCLog("MCMutableImageRep<%p>::selimage()", this);
	if (MCcurtool != T_SELECT)
	{
		oldtool = MCcurtool;
		MCcurtool = T_SELECT;
	}

	if (state & CS_OWN_SELECTION)
		endsel();

	state |= CS_EDITED | CS_OWN_SELECTION;
	MCU_set_rect(selrect, 0, 0, rect.width, rect.height);
	MCscreen->addtimer(m_owner, MCM_internal, MCrepeatrate);
	MCactiveimage = m_owner;
	MCeditingimage = m_owner;
	state |= CS_EDITED;
	MCundos->freestate();
	Ustruct *us = new Ustruct;
	us->type = UT_PAINT;

	MCundos->savestate(m_owner, us);
	if (m_undo_image != nil)
		MCImageFreeBitmap(m_undo_image);
	m_undo_image = nil;
	/* UNCHECKED */ MCImageCopyBitmap(m_bitmap, m_undo_image);

	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	m_owner->invalidate_rep(rect);
}

void MCMutableImageRep::getsel(Boolean cut)
{
	MCLog("MCMutableImageRep<%p>::getsel()", this);
	/* UNCHECKED */ MCImageCopyBitmapRegion(m_bitmap, selrect, m_selection_image);
	
	if (cut)
		cutoutsel();

	state |= CS_BEEN_MOVED;
}

void MCMutableImageRep::cutoutsel()
{
	MCLog("MCMutableImageRep<%p>::cutoutsel()", this);
	if (state & CS_BEEN_MOVED)
		MCImageBitmapClear(m_selection_image);
	else
		MCImageBitmapClearRegion(m_bitmap, selrect);
}

void MCImageBitmapMerge(MCImageBitmap *p_dst, MCImageBitmap *p_src, uindex_t p_dst_x, uindex_t p_dst_y, uindex_t p_src_x, uindex_t p_src_y, uindex_t p_width, uindex_t p_height)
{
	MCAssert(p_src_x >= 0 && p_src_y >= 0);
	MCAssert(p_dst_x >= 0 && p_dst_y >= 0);
	MCAssert(p_src_x + p_width <= p_src->width && p_src_y + p_height <= p_src->height);
	MCAssert(p_dst_x + p_width <= p_dst->width && p_dst_y + p_height <= p_dst->height);
	MCImageBitmapPremultiply(p_dst);
	surface_combine_blendSrcOver_masked((uint8_t*)p_dst->data + p_dst_y * p_dst->stride + p_dst_x * sizeof(uint32_t), p_dst->stride, (uint8_t*)p_src->data + p_src_y * p_src->stride + p_src_x * sizeof(uint32_t), p_src->stride, p_width, p_height, 0xFF);
	MCImageBitmapUnpremultiply(p_dst);
}

void MCMutableImageRep::stampsel()
{
	MCLog("MCMutableImageRep<%p>::stampsel()", this);
	// merge m_selection_image(0, 0) onto m_bitmap(selrect) srcOver
	MCImageBitmapMerge(m_bitmap, m_selection_image, selrect.x, selrect.y, 0, 0, selrect.width, selrect.height);
	
	state |= CS_EDITED;
}

void MCMutableImageRep::rotatesel(int2 angle)
{
	MCImageBitmap *t_dst_image = nil;
	MCImageBitmap *t_cutout = nil;
	MCImageBitmap *t_rotated = nil;
	MCRectangle trect;
	int2 xoffset, yoffset;
	xoffset = yoffset = 0;

	bool clearundo = false;

	if (!has_selection())
		selrect = MCU_make_rect(0, 0, rect.width, rect.height);

	if (has_selection() && ((state & CS_BEEN_MOVED) == 0))
		getsel(True);

	/* UNCHECKED */ copy_selection(t_cutout);

	if (state & CS_BEEN_MOVED)
	{
		t_dst_image = m_selection_image;
		MCU_set_rect(trect, 0, 0, selrect.width, selrect.height);
	}
	else
	{
		t_dst_image = m_bitmap;
		trect = selrect;
	}

	cutoutsel();

	MCImageBitmapPremultiply(t_cutout);
	/* UNCHECKED */ MCImageRotateBitmap(t_cutout, angle, m_owner->getresizequality(), 0x00, t_rotated);
	MCImageBitmapUnpremultiply(t_rotated);

	MCImageFreeBitmap(t_cutout);

	MCRectangle t_opaque = MCImageBitmapGetOpaqueBounds(t_rotated, 0);

	if (t_opaque.width == 0 || t_opaque.height == 0)
	{
		// no opaque pixels, leave things as they are
		MCImageFreeBitmap(t_rotated);
		return;
	}

	xoffset = t_opaque.x;
	yoffset = t_opaque.y;

	if (m_owner->getflag(F_LOCK_LOCATION) || state & CS_BEEN_MOVED)
	{
		int32_t t_new_x, t_new_y;
		t_new_x = selrect.x + ((int32_t)selrect.width - (int32_t)t_opaque.width) / 2;
		t_new_y = selrect.y + ((int32_t)selrect.height - (int32_t)t_opaque.height) / 2;

		// ensure new selrect fits within rect
		selrect.x = MCU_max(MCU_min(t_new_x, (int32_t)rect.width - (int32_t)t_opaque.width), 0);
		selrect.y = MCU_max(MCU_min(t_new_y, (int32_t)rect.height - (int32_t)t_opaque.height), 0);
		selrect.width = MCU_min(t_opaque.width, rect.width);
		selrect.height = MCU_min(t_opaque.height, rect.height);

		if (selrect.width != t_opaque.width)
			xoffset += (t_opaque.width - selrect.width) / 2;
		if (selrect.height != t_opaque.height)
			yoffset += (t_opaque.height - selrect.height) / 2;
	}
	else
	{
		rect.x = rect.x + (((int32_t)rect.width - t_opaque.width) / 2);
		rect.y = rect.y + (((int32_t)rect.height - t_opaque.height) / 2);
		rect.width = t_opaque.width;
		rect.height = t_opaque.height;

		if (m_bitmap->width != rect.width || m_bitmap->height != rect.height)
		{
			MCImageBitmap *t_new_image = nil;
			/* UNCHECKED */ MCImageBitmapCreate(rect.width, rect.height, t_new_image);
			MCImageBitmapClear(t_new_image);

			MCImageFreeBitmap(m_bitmap);
			m_bitmap = t_new_image;
			t_dst_image = m_bitmap;

			MCImageFreeBitmap(m_undo_image);
			m_undo_image = nil;
			clearundo = true;
		}
		trect.x = selrect.x = 0;
		trect.y = selrect.y = 0;
		selrect.width = rect.width;
		selrect.height = rect.height;
	}
	if (state & CS_BEEN_MOVED && (selrect.width > trect.width
	                              || selrect.height > trect.height))
	{
		MCImageFreeBitmap(m_selection_image);
		m_selection_image = nil;
		/* UNCHECKED */ MCImageBitmapCreate(selrect.width, selrect.height, m_selection_image);
		MCImageBitmapClear(m_selection_image);
		t_dst_image = m_selection_image;
	}

	MCPoint t_dst_offset;
	t_dst_offset.x = trect.x;
	t_dst_offset.y = trect.y;
	MCRectangle t_src_rect;
	t_src_rect = MCU_make_rect(xoffset, yoffset, selrect.width, selrect.height);
	MCImageBitmapCopyRegionToBitmap(t_dst_image, t_rotated, t_dst_offset, t_src_rect);

	MCImageFreeBitmap(t_rotated);

	if (clearundo)
	{
		MCundos->freestate();
		Ustruct *us = new Ustruct;
		us->type = UT_PAINT;
		MCundos->savestate(m_owner, us);
		MCImageFreeBitmap(m_undo_image);
		m_undo_image = nil;
		/* UNCHECKED */ MCImageCopyBitmap(m_bitmap, m_undo_image);
	}

	m_owner->sourcerectchanged(rect);
}

bool MCImageBitmapFlipVertical(MCImageBitmap *p_src, MCImageBitmap *&r_flipped)
{
	if (!MCImageBitmapCreate(p_src->width, p_src->height, r_flipped))
		return false;

	uint8_t *t_src_ptr = (uint8_t*)p_src->data;
	uint8_t *t_dst_ptr = (uint8_t*)r_flipped->data + r_flipped->stride * (r_flipped->height - 1);
	for (uindex_t y = 0; y < p_src->height; y++)
	{
		MCMemoryCopy(t_dst_ptr, t_src_ptr, r_flipped->stride);
		t_src_ptr += p_src->stride;
		t_dst_ptr -= r_flipped->stride;
	}

	r_flipped->has_transparency = p_src->has_transparency;
	r_flipped->has_alpha = p_src->has_alpha;

	return true;
}

bool MCImageBitmapFlipHorizontal(MCImageBitmap *p_src, MCImageBitmap *&r_flipped)
{
	if (!MCImageBitmapCreate(p_src->width, p_src->height, r_flipped))
		return false;

	uint8_t *t_src_ptr = (uint8_t*)p_src->data;
	uint8_t *t_dst_ptr = (uint8_t*)r_flipped->data + (r_flipped->width - 1) * sizeof(uint32_t);
	for (uindex_t y = 0; y < p_src->height; y++)
	{
		uint32_t *t_src_row = (uint32_t*)t_src_ptr;
		uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
		for (uindex_t x = 0; x < p_src->width; x++)
			*t_dst_row-- = *t_src_row++;
		t_src_ptr += p_src->stride;
		t_dst_ptr += r_flipped->stride;
	}

	r_flipped->has_transparency = p_src->has_transparency;
	r_flipped->has_alpha = p_src->has_alpha;

	return true;
}

bool MCImageBitmapFlip(MCImageBitmap *p_src, bool p_horizontal, MCImageBitmap *&r_flipped)
{
	if (p_horizontal)
		return MCImageBitmapFlipHorizontal(p_src, r_flipped);
	else
		return MCImageBitmapFlipVertical(p_src, r_flipped);
}

// MW-2007-09-22: [[ Bug 4512 ]] Add support for flipping the alpha channel
void MCMutableImageRep::flipsel(Boolean ishorizontal)
{
	MCImageBitmap *t_dst_image = m_bitmap;
	MCImageBitmap *t_flipped = nil;
	MCRectangle trect;

	if (state & CS_BEEN_MOVED)
	{
		/* UNCHECKED */ MCImageBitmapFlip(m_selection_image, ishorizontal, t_flipped);

		t_dst_image = m_selection_image;
		MCU_set_rect(trect, 0, 0, selrect.width, selrect.height);
	}
	else
	{
		MCImageBitmap *t_cutout = nil;
		/* UNCHECKED */ MCImageCopyBitmapRegion(m_bitmap, selrect, t_cutout);
		/* UNCHECKED */ MCImageBitmapFlip(t_cutout, ishorizontal, t_flipped);
		MCImageFreeBitmap(t_cutout);
		trect = selrect;
	}

	MCPoint t_dst_point;
	t_dst_point.x = trect.x;
	t_dst_point.y = trect.y;
	MCRectangle t_src_rect = MCU_make_rect(0, 0, trect.width, trect.height);
	MCImageBitmapCopyRegionToBitmap(t_dst_image, t_flipped, t_dst_point, t_src_rect);

	MCImageFreeBitmap(t_flipped);

	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	m_owner->invalidate_rep(rect);
}

////////////////////////////////////////////////////////////////////////////////

void MCMutableImageRep::pasteimage(MCImageBitmap *p_bitmap)
{
	MCLog("MCMutableImageRep<%p>::pasteimage()", this);
	if (MCcurtool != T_SELECT)
	{
		oldtool = MCcurtool;
		MCcurtool = T_SELECT;
	}
	
	// MW-2007-12-05: [[ Bug 1931 ]] If we currently own the selection, then make sure
	//   we paste at the centre of the current selection location
	MCRectangle t_target_rect;
	if (state & CS_OWN_SELECTION)
	{
		t_target_rect = selrect;
		endsel();
	}
	else
	{
		t_target_rect = rect;
		t_target_rect . x = 0;
		t_target_rect . y = 0;
	}
	
	state |= CS_OWN_SELECTION | CS_BEEN_MOVED;
	
	selrect . width = p_bitmap->width;
	selrect . height = p_bitmap->height;
	selrect = MCU_center_rect(t_target_rect, selrect);
	
	/* UNCHECKED */ MCImageCopyBitmap(p_bitmap, m_selection_image);

	MCactiveimage = m_owner;
	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	m_owner->invalidate_rep(rect);
}

////////////////////////////////////////////////////////////////////////////////

void MCMutableImageRep::image_undo(Ustruct *us)
{
	if (state & CS_DRAW)
		return;

	MCImageFreeBitmap(m_bitmap);
	m_bitmap = m_undo_image;
	m_undo_image = nil;

	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	m_owner->invalidate_rep(rect);
}

void MCMutableImageRep::image_freeundo(Ustruct *us)
{
	MCLog("MCMutableImage::image_free_undo(%p) - freeing image %p", us, m_undo_image);
	MCImageFreeBitmap(m_undo_image);
	m_undo_image = nil;
}

////////////////////////////////////////////////////////////////////////////////

void MCMutableImageRep::init()
{
}

void MCMutableImageRep::shutdown()
{
	if (points != NULL)
		delete points;
}

////////////////////////////////////////////////////////////////////////////////
