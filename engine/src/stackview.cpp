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

//#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "stack.h"

#include "util.h"
#include "region.h"

#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

struct MCStackFullscreenModeNames
{
	const char *name;
	MCStackFullscreenMode mode;
};

static MCStackFullscreenModeNames s_fullscreenmode_names[] = {
	{"", kMCStackFullscreenResize},
	{"exact fit", kMCStackFullscreenExactFit},
	{"show all", kMCStackFullscreenShowAll},
	{"no border", kMCStackFullscreenNoBorder},
	{"no scale", kMCStackFullscreenNoScale},

	{nil, kMCStackFullscreenModeNone},
};

const char *MCStackFullscreenModeToString(MCStackFullscreenMode p_mode)
{
	for (uint32_t i = 0; s_fullscreenmode_names[i].name != nil; i++)
	{
		if (p_mode == s_fullscreenmode_names[i].mode)
			return s_fullscreenmode_names[i].name;
	}

	return nil;
}

bool MCStackFullscreenModeFromString(const char *p_string, MCStackFullscreenMode &r_mode)
{
	for (uint32_t i = 0; s_fullscreenmode_names[i].name != nil; i++)
	{
		if (MCCStringEqualCaseless(p_string, s_fullscreenmode_names[i].name))
		{
			r_mode = s_fullscreenmode_names[i].mode;
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

struct MCRegionTransformContext
{
	MCRegionRef region;
	MCGAffineTransform transform;
};

bool MCRegionTransformCallback(void *p_context, const MCRectangle &p_rect)
{
	MCRegionTransformContext *t_context;
	t_context = static_cast<MCRegionTransformContext*>(p_context);

	MCRectangle t_transformed_rect;
	t_transformed_rect = MCRectangleGetTransformedBounds(p_rect, t_context->transform);

	return MCRegionIncludeRect(t_context->region, t_transformed_rect);
}

bool MCRegionTransform(MCRegionRef p_region, const MCGAffineTransform &p_transform, MCRegionRef &r_transformed_region)
{
	bool t_success;
	t_success = true;

	MCRegionRef t_new_region;
	t_new_region = nil;

	t_success = MCRegionCreate(t_new_region);

	if (t_success)
	{
		MCRegionTransformContext t_context;
		t_context.region = t_new_region;
		t_context.transform = p_transform;

		t_success = MCRegionForEachRect(p_region, MCRegionTransformCallback, &t_context);
	}

	if (t_success)
		r_transformed_region = t_new_region;
	else
		MCRegionDestroy(t_new_region);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCStack::view_init(void)
{
	m_view_fullscreen = false;
	m_view_fullscreenmode = kMCStackFullscreenModeDefault;
	m_view_redraw = false;
}

void MCStack::view_copy(const MCStack &p_view)
{
	m_view_fullscreen = p_view.m_view_fullscreen;
	m_view_fullscreenmode = p_view.m_view_fullscreenmode;
	m_view_redraw = false;
}

void MCStack::view_setfullscreen(bool p_fullscreen)
{
	m_view_fullscreen = p_fullscreen;
}

bool MCStack::view_getfullscreen(void)
{
	// IM-2013-09-30: [[ FullscreenMode ]] return true if windows are always fullscreen on this display
	return m_view_fullscreen || (MCscreen != nil && MCscreen->fullscreenwindows());
}

void MCStack::view_setfullscreenmode(MCStackFullscreenMode p_mode)
{
	m_view_fullscreenmode = p_mode;
}

MCStackFullscreenMode MCStack::view_getfullscreenmode(void)
{
	return m_view_fullscreenmode;
}

MCGAffineTransform MCStack::view_getviewtransform(void) const
{
	return m_view_transform;
}

MCRectangle MCStack::view_getrect(void)
{
	return m_view_screen_rect;
}

MCRectangle MCStack::view_setstackrect(MCRectangle p_rect)
{
	// MW-2012-10-04: [[ Bug 10436 ]] Make sure we constrain stack size to screen size
	//   if in fullscreen mode.
	MCRectangle t_new_rect;
	if (view_getfullscreen())
	{
		const MCDisplay *t_display;
		t_display = MCscreen -> getnearestdisplay(p_rect);

		switch (m_view_fullscreenmode)
		{
		case kMCStackFullscreenResize:
			// resize stack to fullscreen rect
			t_new_rect = MCscreen->fullscreenrect(t_display);
			break;

		case kMCStackFullscreenNoScale:
			// center rect on screen
			t_new_rect = MCU_center_rect(MCscreen->fullscreenrect(t_display), p_rect);
			break;

		case kMCStackFullscreenExactFit:
		case kMCStackFullscreenShowAll:
		case kMCStackFullscreenNoBorder:
			// scaling modes should return the requested stack rect
			t_new_rect = p_rect;
			break;

		default:
			MCAssert(false);
		}
	}
	else
		t_new_rect = p_rect;

	return t_new_rect;
}

MCGAffineTransform view_get_stack_transform(MCStackFullscreenMode p_mode, MCRectangle p_stack_rect, MCRectangle p_screen_rect)
{
	MCGFloat t_scale;
	MCGAffineTransform t_transform;

	switch (p_mode)
	{
	case kMCStackFullscreenResize:
		return MCGAffineTransformMakeIdentity();

	case kMCStackFullscreenExactFit:
		return MCGAffineTransformMakeScale((MCGFloat)p_screen_rect.width / (MCGFloat)p_stack_rect.width, (MCGFloat)p_screen_rect.height / (MCGFloat)p_stack_rect.height);

	case kMCStackFullscreenShowAll:
		t_scale = MCMin((MCGFloat)p_screen_rect.width / (MCGFloat)p_stack_rect.width, (MCGFloat)p_screen_rect.height / (MCGFloat)p_stack_rect.height);
		t_transform = MCGAffineTransformMakeTranslation(-(MCGFloat)p_stack_rect.width / 2.0, -(MCGFloat)p_stack_rect.height / 2.0);
		t_transform = MCGAffineTransformScale(t_transform, t_scale, t_scale);
		t_transform = MCGAffineTransformTranslate(t_transform, (MCGFloat)p_screen_rect.width / 2.0, (MCGFloat)p_screen_rect.height / 2.0);

		return t_transform;

	case kMCStackFullscreenNoBorder:
		t_scale = MCMax((MCGFloat)p_screen_rect.width / (MCGFloat)p_stack_rect.width, (MCGFloat)p_screen_rect.height / (MCGFloat)p_stack_rect.height);
		t_transform = MCGAffineTransformMakeTranslation(-(MCGFloat)p_stack_rect.width / 2.0, -(MCGFloat)p_stack_rect.height / 2.0);
		t_transform = MCGAffineTransformScale(t_transform, t_scale, t_scale);
		t_transform = MCGAffineTransformTranslate(t_transform, (MCGFloat)p_screen_rect.width / 2.0, (MCGFloat)p_screen_rect.height / 2.0);

		return t_transform;

	case kMCStackFullscreenNoScale:
		// offset so stack is centered in screen
		MCRectangle t_rect;
		t_rect = MCU_center_rect(p_screen_rect, p_stack_rect);
		return MCGAffineTransformMakeTranslation(t_rect.x, t_rect.y);
	}
}

MCRectangle MCStack::view_setgeom(const MCRectangle &p_rect)
{
	MCRectangle t_old_stack_rect;

	t_old_stack_rect = m_view_stack_rect;

	if (view_getfullscreen())
	{
		const MCDisplay *t_display;
		t_display = MCscreen -> getnearestdisplay(p_rect);

		m_view_screen_rect = MCscreen->fullscreenrect(t_display);
		m_view_screen_rect . x = m_view_screen_rect . y = 0;
		
		m_view_stack_rect = view_setstackrect(p_rect);
		m_view_transform = view_get_stack_transform(m_view_fullscreenmode, m_view_stack_rect, m_view_screen_rect);

		m_view_redraw = true;
	}
	else
	{
		m_view_screen_rect = p_rect;

		m_view_stack_rect = p_rect;
		m_view_transform = MCGAffineTransformMakeIdentity();
	}
	
	MCRectangle t_device_rect, t_old_device_rect;
	t_device_rect = MCGRectangleGetIntegerInterior(MCResUserToDeviceRect(m_view_screen_rect));

	t_old_device_rect = device_setgeom(t_device_rect);

	return t_old_stack_rect;
}

void MCStack::view_render(MCGContextRef p_target, MCRectangle p_rect)
{
	// redraw borders if visible

	// scale & position stack redraw rect
	// update stack region
	if (view_getfullscreen())
	{
		MCRectangle t_update_rect;
		t_update_rect = MCGRectangleGetIntegerBounds(MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(p_rect), MCGAffineTransformInvert(m_view_transform)));
		
		MCGContextSave(p_target);
		MCGContextConcatCTM(p_target, m_view_transform);
		
		if (m_view_redraw)
		{
			// IM-2013-09: [[ FullscreenMode ]] draw the view backdrop
			/* OVERHAUL - REVISIT: currently just draws black behind the stack area */
			MCGContextAddRectangle(p_target, MCRectangleToMCGRectangle(t_update_rect));
			MCGContextSetFillRGBAColor(p_target, 0.0, 0.0, 0.0, 1.0);
			MCGContextFill(p_target);
			m_view_redraw = false;
		}

		t_update_rect = MCU_intersect_rect(t_update_rect, MCRectangleMake(0, 0, m_view_stack_rect.width, m_view_stack_rect.height));
		MCGContextClipToRect(p_target, MCRectangleToMCGRectangle(t_update_rect));
		render(p_target, t_update_rect);
		MCGContextRestore(p_target);
	}
	else
	{
		render(p_target, p_rect);
	}

}

void MCStack::view_updatestack(MCRegionRef p_region)
{
	MCGAffineTransform t_transform;
	t_transform = getdevicetransform();

	// transform stack region to device region
	MCRegionRef t_view_region;
	t_view_region = nil;

	/* UNCHECKED */ MCRegionTransform(p_region, t_transform, t_view_region);
	
	// IM-2013-09-30: [[ FullscreenMode ]] If view background needs redrawn, add view rect
	// (in device coords) to redraw region
	if (view_getfullscreen() && m_view_redraw)
		MCRegionIncludeRect(t_view_region, MCGRectangleGetIntegerBounds(MCResUserToDeviceRect(m_view_screen_rect)));
	
	device_updatewindow(t_view_region);

	MCRegionDestroy(t_view_region);
}

MCPoint MCStack::view_viewtostackloc(const MCPoint &p_loc)
{
	if (!view_getfullscreen() || m_view_fullscreenmode == kMCStackFullscreenResize)
		return p_loc;

	MCGAffineTransform t_transform = MCGAffineTransformInvert(m_view_transform);
	return MCGPointToMCPoint(MCGPointApplyAffineTransform(MCPointToMCGPoint(p_loc), t_transform));
}

MCPoint MCStack::view_stacktoviewloc(const MCPoint &p_loc)
{
	if (!view_getfullscreen() || m_view_fullscreenmode == kMCStackFullscreenResize)
		return p_loc;

	return MCGPointToMCPoint(MCGPointApplyAffineTransform(MCPointToMCGPoint(p_loc), m_view_transform));
}