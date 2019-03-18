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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "stack.h"
#include "aclip.h"
#include "card.h"
#include "mccontrol.h"
#include "player.h"
#include "sellst.h"
#include "visual.h"
#include "util.h"
#include "mode.h"
#include "osspec.h"
#include "redraw.h"
#include "region.h"
#include "globals.h"
#include "context.h"


#include "graphicscontext.h"

#include "graphics.h"
#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

#define ZOOM_RECTS 6
#define DISSOLVE_SIZE 16
#define DISSOLVE_MASK 15

static uint2 Checkersize = 64;
static uint2 Venetiansize = 64;
#if INCLUDE_ZOOM_EFFECT
static uint2 Zoomsize = 16;
#endif

static Boolean barneffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean checkerboardeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean dissolveeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean iriseffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean pusheffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean revealeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean scrolleffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean shrinkeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean stretcheffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean venetianeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean wipeeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean zoomeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration);

extern bool MCQTEffectBegin(Visual_effects p_type, const char *p_name, Visual_effects p_direction, MCGImageRef p_start, MCGImageRef p_end, const MCRectangle& p_area);
extern bool MCQTEffectStep(const MCRectangle &drect, MCStackSurface *p_target, uint4 p_delta, uint4 p_duration);
extern void MCQTEffectEnd(void);

extern bool MCCoreImageEffectBegin(const char *p_name, MCGImageRef p_source_a, MCGImageRef p_source_b, const MCRectangle& p_rect, MCGFloat p_surface_height, MCEffectArgument *p_arguments);
extern bool MCCoreImageEffectStep(MCStackSurface *p_target, float p_time);
extern void MCCoreImageEffectEnd(void);

extern void MCMacDisableScreenUpdates(void);
extern void MCMacEnableScreenUpdates(void);

////////////////////////////////////////////////////////////////////////////////

struct MCStackEffectContext
{
	MCGImageRef initial_image;
	MCGImageRef final_image;
	
	MCEffectList *effect;
	MCRectangle effect_area;
	
	uint32_t delta;
	uint32_t duration;
};

bool MCStackRenderInitial(MCStackSurface *p_target, MCRegionRef p_region, void *p_context)
{
	MCStackEffectContext *context = static_cast<MCStackEffectContext*>(p_context);
	
	p_target->Composite(MCRectangleToMCGRectangle(context->effect_area), context->initial_image, MCGRectangleMake(0, 0, context->effect_area.width, context->effect_area.height), 1.0, kMCGBlendModeCopy);
	
	return true;
}

bool MCStackRenderEffect(MCStackSurface *p_target, MCRegionRef p_region, void *p_context)
{
	bool t_drawn = false;
	MCStackEffectContext *context = static_cast<MCStackEffectContext*>(p_context);
	
	// Render the effect into the dst image buffer.
	switch(context->effect->type)
	{
		case VE_BARN:
			t_drawn = barneffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
		case VE_CHECKERBOARD:
			t_drawn = checkerboardeffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
		case VE_DISSOLVE:
			t_drawn = dissolveeffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
		case VE_IRIS:
			t_drawn = iriseffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
		case VE_PUSH:
			t_drawn = pusheffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
		case VE_REVEAL:
			t_drawn = revealeffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
		case VE_SCROLL:
			t_drawn = scrolleffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
		case VE_SHRINK:
			t_drawn = shrinkeffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
		case VE_STRETCH:
			t_drawn = stretcheffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
		case VE_VENETIAN:
			t_drawn = venetianeffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
		case VE_WIPE:
			t_drawn = wipeeffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
		case VE_ZOOM:
			t_drawn = zoomeffect_step(context->effect_area, p_target, context->initial_image, context->final_image, context->effect->direction, context->delta, context->duration);
			break;
			
#ifdef _MAC_DESKTOP
		case VE_CIEFFECT:
			t_drawn = MCCoreImageEffectStep(p_target, (float)context->delta / context->duration);
			break;
#endif
			
#ifdef FEATURE_QUICKTIME_EFFECTS
		case VE_QTEFFECT:
			t_drawn = MCQTEffectStep(context->effect_area, p_target, context->delta, context->duration);
			break;
#endif
			
		default:
			break;
	}
	
	return t_drawn;
}

//////////

void MCStack::effectrect(const MCRectangle& p_area, Boolean& r_abort)
{
	// Get the list of effects.
	MCEffectList *t_effects = MCcur_effects;
	MCcur_effects = NULL;

	// If the window isn't opened or hasn't been attached (plugin) or if we have no
	// snapshot to use, this is a no-op.
	if (!opened || !haswindow() || m_snapshot == nil)
	{
		while(t_effects != NULL)
		{
			MCEffectList *t_effect;
			t_effect = t_effects;
			t_effects = t_effects -> next;
			delete t_effect;
		}
		return;
	}

	// Mark the stack as being in an effect.
	state |= CS_EFFECT;

	// Lock messages while the effect is happening.
	Boolean t_old_lockmessages;
	t_old_lockmessages = MClockmessages;
	MClockmessages = True;

	// Calculate the area of interest.
	MCRectangle t_effect_area;
	t_effect_area = curcard -> getrect();
	t_effect_area . y = getscroll();
	t_effect_area . height -= t_effect_area . y;
	t_effect_area = MCU_intersect_rect(t_effect_area, p_area);
	
	// IM-2013-08-21: [[ ResIndependence ]] Scale effect area to device coords
	// Align snapshot rect to device pixels
	// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
	MCGAffineTransform t_transform;
	t_transform = getdevicetransform();
	
    // MW-2013-10-29: [[ Bug 11330 ]] Make sure the effect area is cropped to the visible
    //   area.
    t_effect_area = MCRectangleGetTransformedBounds(t_effect_area, getviewtransform());
    t_effect_area = MCU_intersect_rect(t_effect_area, MCU_make_rect(0, 0, view_getrect() . width, view_getrect() . height));
	
	// IM-2014-01-24: [[ HiDPI ]] scale effect region to backing surface coords
	MCGFloat t_scale;
	t_scale = view_getbackingscale();
	
    MCRectangle t_device_rect, t_user_rect;
	t_device_rect = MCRectangleGetScaledBounds(t_effect_area, t_scale);
	t_user_rect = MCRectangleGetTransformedBounds(t_device_rect, MCGAffineTransformInvert(t_transform));
	
#ifdef _MAC_DESKTOP
	// IM-2013-08-29: [[ RefactorGraphics ]] get device height for CoreImage effects
	// IM-2013-09-30: [[ FullscreenMode ]] Use view rect to get device height
	uint32_t t_device_height;
	t_device_height = floor(view_getrect().height * t_scale);
#endif /* _MAC_DESKTOP */
	
	// Make a region of the effect area
	// IM-2013-08-29: [[ ResIndependence ]] scale effect region to device coords
	MCRegionRef t_effect_region;
	t_effect_region = nil;
	/* UNCHECKED */ MCRegionCreate(t_effect_region);
	/* UNCHECKED */ MCRegionSetRect(t_effect_region, t_effect_area);
	
	// Lock the screen to prevent any updates occuring until we want them.
	MCRedrawLockScreen();

	// By default, we have not aborted.
	r_abort = False;
	
	MCGImageRef t_initial_image;
	t_initial_image = MCGImageRetain(m_snapshot);
	
	while(t_effects != nil)
	{
		uint32_t t_duration;
		t_duration = MCU_max(1, MCeffectrate / (t_effects -> speed - VE_VERY));
		if (t_effects -> type == VE_DISSOLVE)
			t_duration *= 2;
		
		uint32_t t_delta;
		t_delta = 0;
		
		// Create surface at effect_area size.
		// Render into surface based on t_effects -> image
		MCGImageRef t_final_image = nil;
		
		// If this isn't a plain effect, then we must fetch first and last images.
		if (t_effects -> type != VE_PLAIN)
		{
			// Render the final image.
			MCGContextRef t_context = nil;
			
			// IM-2014-05-20: [[ GraphicsPerformance ]] Create opaque context for snapshot
			/* UNCHECKED */ MCGContextCreate(t_device_rect.width, t_device_rect.height, false, t_context);
			
			MCGContextTranslateCTM(t_context, -t_device_rect.x, -t_device_rect.y);
			
			// IM-2013-10-03: [[ FullscreenMode ]] Apply device transform to context
			MCGContextConcatCTM(t_context, t_transform);
			
			// Configure the context.
			MCGContextClipToRect(t_context, MCRectangleToMCGRectangle(t_user_rect));
			
			// Render an appropriate image
			switch(t_effects -> image)
			{
				case VE_INVERSE:
					{
						MCContext *t_old_context = nil;
						/* UNCHECKED */ t_old_context = new (nothrow) MCGraphicsContext(t_context);
						curcard->draw(t_old_context, t_user_rect, false);
						delete t_old_context;
						
						MCGContextSetFillRGBAColor(t_context, 1.0, 1.0, 1.0, 1.0);
						MCGContextSetBlendMode(t_context, kMCGBlendModeDifference);
						MCGContextAddRectangle(t_context, MCRectangleToMCGRectangle(t_user_rect));
						MCGContextFill(t_context);
					}
					break;
					
				case VE_BLACK:
					MCGContextSetFillRGBAColor(t_context, 0.0, 0.0, 0.0, 1.0);
					MCGContextAddRectangle(t_context, MCRectangleToMCGRectangle(t_user_rect));
					MCGContextFill(t_context);
					break;
					
				case VE_WHITE:
					MCGContextSetFillRGBAColor(t_context, 1.0, 1.0, 1.0, 1.0);
					MCGContextAddRectangle(t_context, MCRectangleToMCGRectangle(t_user_rect));
					MCGContextFill(t_context);
					break;
					
				case VE_GRAY:
					MCGContextSetFillRGBAColor(t_context, 0.5, 0.5, 0.5, 1.0);
					MCGContextAddRectangle(t_context, MCRectangleToMCGRectangle(t_user_rect));
					MCGContextFill(t_context);
					break;
					
				default:
				{
					MCContext *t_old_context = nil;
					/* UNCHECKED */ t_old_context = new (nothrow) MCGraphicsContext(t_context);
					curcard->draw(t_old_context, t_user_rect, false);
					delete t_old_context;
				}
			}
			
			/* UNCHECKED */ MCGContextCopyImage(t_context, t_final_image);
			MCGContextRelease(t_context);
		}
		
		MCStackEffectContext t_context;
		t_context.delta = t_delta;
		t_context.duration = t_duration;
		t_context.effect = t_effects;
		t_context.effect_area = t_device_rect;
		t_context.initial_image = t_initial_image;
		t_context.final_image = t_final_image;
		
		// MW-2011-10-20: [[ Bug 9824 ]] Make sure dst point is correct.
		// Initialize the destination with the start image.
		view_platform_updatewindowwithcallback(t_effect_region, MCStackRenderInitial, &t_context);
		
		// If there is a sound, then start playing it.
		if (t_effects -> sound != NULL)
		{
			MCAudioClip *acptr;
            MCNewAutoNameRef t_sound;
            /* UNCHECKED */ MCNameCreate(t_effects->sound, &t_sound);
			if ((acptr = (MCAudioClip *)getobjname(CT_AUDIO_CLIP, *t_sound)) == NULL)
			{
				IO_handle stream;
				if ((stream = MCS_open(t_effects->sound, kMCOpenFileModeRead, True, False, 0)) != NULL)
				{
					acptr = new (nothrow) MCAudioClip;
					acptr->setdisposable();
					if (!acptr->import(t_effects->sound, stream))
					{
						delete acptr;
						acptr = NULL;
					}
					MCS_close(stream);
				}
			}
			
			if (acptr != NULL)
			{
				MCU_play_stop();
				MCacptr = acptr;
				MCU_play();
#ifndef FEATURE_PLATFORM_AUDIO
				if (MCacptr)
					MCscreen->addtimer(MCacptr, MCM_internal, PLAY_RATE);
#endif
			}
			
			if (MCscreen->wait((real8)MCsyncrate / 1000.0, False, True))
			{
				r_abort = True;
				break;
			}
		}
		
		// Initialize CoreImage of QTEffects if needed.
		if (t_effects -> type != VE_PLAIN)
		{
            MCAutoPointer<char> t_name;
            /* UNCHECKED */ MCStringConvertToCString(t_effects -> name, &t_name);
#ifdef _MAC_DESKTOP
			// IM-2013-08-29: [[ ResIndependence ]] use scaled effect rect for CI effects
			if (t_effects -> type == VE_UNDEFINED && MCCoreImageEffectBegin(*t_name, t_initial_image, t_final_image, t_device_rect, t_device_height, t_effects -> arguments))
				t_effects -> type = VE_CIEFFECT;
			else
#endif
#ifdef FEATURE_QUICKTIME_EFFECTS
				// IM-2013-08-29: [[ ResIndependence ]] use scaled effect rect for QT effects
				if (t_effects -> type == VE_UNDEFINED && MCQTEffectBegin(t_effects -> type, *t_name, t_effects -> direction, t_initial_image, t_final_image, t_device_rect))
					t_effects -> type = VE_QTEFFECT;
#else
				;
#endif
		}
		
		// Run effect
		// Now perform the effect loop, but only if there is something to do.
		if (t_effects -> type != VE_PLAIN || old_blendlevel != blendlevel)
		{
			// Calculate timing parameters.
			double t_start_time;
			t_start_time = 0.0;
			
			for(;;)
			{
				t_context.delta = t_delta;
				
				view_platform_updatewindowwithcallback(t_effect_region, MCStackRenderEffect, &t_context);
				
                MCscreen -> sync(getw());
				
				// Update the window's blendlevel (if needed)
				if (old_blendlevel != blendlevel)
				{
					float t_fraction = float(t_delta) / t_duration;
					setopacity(uint1((old_blendlevel * 255 + (float(blendlevel) - old_blendlevel) * 255 * t_fraction) / 100));
				}
				
				// If the start time is zero, then start counting from here.
				if (t_start_time == 0.0)
					t_start_time = MCS_time();
				
				// If we've reached the end of the transition, we are done.
				if (t_delta == t_duration)
				{
#ifdef _ANDROID_MOBILE
					// MW-2011-12-12: [[ Bug 9907 ]] Make sure we let the screen sync at this point
					MCscreen -> wait(0.01, False, False);
#endif
					break;
				}
				
				// Get the time now.
				double t_now;
				t_now = MCS_time();
				
				// Compute the new delta value.
				uint32_t t_new_delta;
				t_new_delta = (uint32_t)ceil((t_now - t_start_time) * 1000.0);
				
				// If the new value is same as the old, then advance one step.
				if (t_new_delta == t_delta)
					t_delta = t_new_delta + 1;
				else
					t_delta = t_new_delta;
				
				// If the new delta is beyond the end point, set it to the end.
				if (t_delta > t_duration)
					t_delta = t_duration;
				
				// Wait until the next boundary, making sure we break for no reason
				// other than abort.
				if (MCscreen -> wait((t_start_time + (t_delta / 1000.0)) - t_now, False, False))
					r_abort = True;
				
				// If we aborted, we render the final step and are thus done.
				if (r_abort)
					t_delta = t_duration;
			}
		}		
#ifdef _MAC_DESKTOP
		if (t_effects -> type == VE_CIEFFECT)
			MCCoreImageEffectEnd();
		else
#endif
#ifdef FEATURE_QUICKTIME_EFFECTS
			if (t_effects -> type == VE_QTEFFECT)
				MCQTEffectEnd();
#endif
		
		// Free initial surface.
		MCGImageRelease(t_initial_image);
		
		// initial surface becomes final surface.
		t_initial_image = t_final_image;
		t_final_image = nil;
		
		// Move to the next effect.
		MCEffectList *t_current_effect;
		t_current_effect = t_effects;
		t_effects = t_effects -> next;
		delete t_current_effect;
	}

	// Make sure the pixmaps are freed and any dangling effects
	// are cleaned up.
	if (t_effects != NULL)
	{
		/* OVERHAUL - REVISIT: error cleanup needs revised */
		MCGImageRelease(t_initial_image);
//		MCGSurfaceRelease(t_final_image);

		while(t_effects != NULL)
		{
			MCEffectList *t_current_effect;
			t_current_effect = t_effects;
			t_effects = t_effects -> next;
			delete t_current_effect;
		}
	}

	MCRegionDestroy(t_effect_region);
	
	MCGImageRelease(m_snapshot);
	m_snapshot = nil;
	
	m_snapshot = t_initial_image;
	
	// Unlock the screen.
	MCRedrawUnlockScreen();
	
	// Unlock messages.
	MClockmessages = t_old_lockmessages;

	// Turn off effect mode.
	state &= ~CS_EFFECT;
	
	// The stack's blendlevel is now the new one.
	old_blendlevel = blendlevel;
	
	// Finally, mark the affected area of the stack for a redraw.
	dirtyrect(p_area);
}


Boolean barneffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	uint32_t width, x;
	MCGImageRef t_top, t_bottom;
	MCGRectangle t_top_src, t_bottom_src;
	
	if (dir == VE_CLOSE)
	{
		width = (drect.width) * (duration - delta) / duration;
		x = (drect.width / 2) * delta / duration;
		
		t_top = p_start;
		t_bottom = p_end;

	}
	else
	{
		width = (drect.width) * delta / duration;
		x = (drect.width / 2) * (duration - delta) / duration;
		
		t_top = p_end;
		t_bottom = p_start;
	}
	
	t_bottom_src = MCGRectangleMake(0, 0, drect.width, drect.height);
	t_top_src = MCGRectangleMake(x, 0, width, drect.height);
	
	p_target->Composite(MCGRectangleTranslate(t_bottom_src, drect.x, drect.y), t_bottom, t_bottom_src, 1.0, kMCGBlendModeCopy);
	p_target->Composite(MCGRectangleTranslate(t_top_src, drect.x, drect.y), t_top, t_top_src, 1.0, kMCGBlendModeCopy);

	return True;
}

Boolean checkerboardeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	int2 newy = (Checkersize << 1) * delta / duration;

	MCGRectangle t_src_rect, t_dst_rect;
	
	t_src_rect = MCGRectangleMake(0, 0, drect.width, drect.height);
	t_dst_rect = MCGRectangleTranslate(t_src_rect, drect.x, drect.y);
	
	p_target->Composite(t_dst_rect, p_start, t_src_rect, 1.0, kMCGBlendModeCopy);
	
	bool t_odd = false;
	for (uint32_t x = 0; x < drect.width; x += Checkersize)
	{
		for (int32_t y = (t_odd ? -Checkersize : 0); y < drect.height; y += Checkersize * 2)
		{
			t_src_rect = MCGRectangleMake(x, y, Checkersize, newy);
			t_dst_rect = MCGRectangleTranslate(t_src_rect, drect.x, drect.y);
			p_target->Composite(t_dst_rect, p_end, t_src_rect, 1.0, kMCGBlendModeCopy);
		}
		t_odd = !t_odd;
	}
	
	return True;
}

Boolean dissolveeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	MCGFloat t_alpha;
	t_alpha = (MCGFloat)delta / (MCGFloat)duration;
	
	MCGRectangle t_src_rect, t_dst_rect;
	t_dst_rect = MCRectangleToMCGRectangle(drect);
	t_src_rect = MCGRectangleMake(0.0, 0.0, t_dst_rect.size.width, t_dst_rect.size.height);
	
	p_target->Composite(t_dst_rect, p_start, t_src_rect, 1.0, kMCGBlendModeSourceOver);
	p_target->Composite(t_dst_rect, p_end, t_src_rect, t_alpha, kMCGBlendModeSourceOver);
	
	return True;
}

Boolean iriseffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	MCGFloat t_position = (MCGFloat) delta / (MCGFloat) duration;
	uint32_t width, height, x, y;
	MCGImageRef t_top, t_bottom;
	MCGRectangle t_top_src, t_bottom_src;
	
	if (dir == VE_CLOSE)
	{
		width = drect.width * (1.0 - t_position);
		height = drect.height * (1.0 - t_position);
		x = (drect.width / 2) * t_position;
		y = (drect.height / 2) * t_position;
		
		t_top = p_start;
		t_bottom = p_end;
		
	}
	else
	{
		width = drect.width * t_position;
		height = drect.height * t_position;
		x = (drect.width / 2) * (1.0 - t_position);
		y = (drect.height / 2) * (1.0 - t_position);
		
		t_top = p_end;
		t_bottom = p_start;
	}
	
	t_bottom_src = MCGRectangleMake(0, 0, drect.width, drect.height);
	t_top_src = MCGRectangleMake(x, y, width, height);
	
	p_target->Composite(MCGRectangleTranslate(t_bottom_src, drect.x, drect.y), t_bottom, t_bottom_src, 1.0, kMCGBlendModeCopy);
	p_target->Composite(MCGRectangleTranslate(t_top_src, drect.x, drect.y), t_top, t_top_src, 1.0, kMCGBlendModeCopy);
	
	return True;
}

Boolean pusheffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	uint2 size;
	
	if (dir == VE_LEFT || dir == VE_RIGHT)
		size = drect.width * delta / duration;
	else
		size = drect.height * delta / duration;
	
	MCGRectangle t_src_rect, t_dst_rect;
	t_src_rect = MCGRectangleMake(0, 0, drect.width, drect.height);
	t_dst_rect = MCRectangleToMCGRectangle(drect);
	MCGRectangle t_start_dst, t_end_dst;
	switch (dir)
	{
		case VE_LEFT:
			t_start_dst = MCGRectangleTranslate(t_dst_rect, -(MCGFloat)size, 0.0);
			t_end_dst = MCGRectangleTranslate(t_start_dst, drect.width, 0.0);
			break;
			
		case VE_RIGHT:
			t_start_dst = MCGRectangleTranslate(t_dst_rect, size, 0.0);
			t_end_dst = MCGRectangleTranslate(t_start_dst, -(MCGFloat)drect.width, 0.0);
			break;
			
		case VE_UP:
			t_start_dst = MCGRectangleTranslate(t_dst_rect, 0.0, -(MCGFloat)size);
			t_end_dst = MCGRectangleTranslate(t_start_dst, 0.0, drect.height);
			break;
			
		case VE_DOWN:
			t_start_dst = MCGRectangleTranslate(t_dst_rect, 0.0, size);
			t_end_dst = MCGRectangleTranslate(t_start_dst, 0.0, -(MCGFloat)drect.height);
			break;
			
		default:
			MCUnreachable();
			break;
	}
	
	p_target->Composite(t_start_dst, p_start, t_src_rect, 1.0, kMCGBlendModeCopy);
	p_target->Composite(t_end_dst, p_end, t_src_rect, 1.0, kMCGBlendModeCopy);
	
	return True;
}

Boolean revealeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	uint2 size;
	if (dir == VE_LEFT || dir == VE_RIGHT)
		size = drect.width * delta / duration;
	else
		size = drect.height * delta / duration;
	
	MCGRectangle t_start_src, t_end_src, t_dst_rect;
	MCGRectangle t_start_dst, t_end_dst;
	t_dst_rect = MCRectangleToMCGRectangle(drect);
	
	t_start_src = MCGRectangleMake(0, 0, drect.width, drect.height);
	switch (dir)
	{
		case VE_LEFT:
			t_start_dst = MCGRectangleTranslate(t_dst_rect, -(MCGFloat)size, 0.0);
			t_end_src = MCGRectangleMake(t_dst_rect.size.width - size, 0.0, size, t_dst_rect.size.height);
			break;
			
		case VE_RIGHT:
			t_start_dst = MCGRectangleTranslate(t_dst_rect, size, 0.0);
			t_end_src = MCGRectangleMake(0.0, 0.0, size, t_dst_rect.size.height);
			break;
			
		case VE_UP:
			t_start_dst = MCGRectangleTranslate(t_dst_rect, 0.0, -(MCGFloat)size);
			t_end_src = MCGRectangleMake(0.0, t_dst_rect.size.height - size, t_dst_rect.size.width, size);
			break;
			
		case VE_DOWN:
			t_start_dst = MCGRectangleTranslate(t_dst_rect, 0.0, size);
			t_end_src = MCGRectangleMake(0.0, 0.0, t_dst_rect.size.width, size);
			break;
			
		default:
			MCUnreachable();
			break;
	}
	
	t_end_dst = MCGRectangleTranslate(t_end_src, t_dst_rect.origin.x, t_dst_rect.origin.y);
	p_target->Composite(t_start_dst, p_start, t_start_src, 1.0, kMCGBlendModeCopy);
	p_target->Composite(t_end_dst, p_end, t_end_src, 1.0, kMCGBlendModeCopy);
	
	return True;
}

Boolean scrolleffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	MCGFloat t_position = (MCGFloat)delta / (MCGFloat)duration;
	
	MCGRectangle t_end_src, t_end_dst;
	MCGRectangle t_start_src, t_start_dst;
	t_start_dst = MCRectangleToMCGRectangle(drect);
	
	t_start_src = t_end_src = MCGRectangleMake(0.0, 0.0, drect.width, drect.height);
	
	uint32_t width, height;
	
	switch (dir)
	{
		case VE_LEFT:
			width = drect.width * t_position;
			t_end_dst = MCGRectangleTranslate(t_end_src, t_end_src.size.width - width, 0.0);
			break;
			
		case VE_RIGHT:
			width = drect.width * t_position;
			t_end_dst = MCGRectangleTranslate(t_end_src, -t_end_src.size.width + width, 0.0);
			break;
			
		case VE_UP:
			height = drect.height * t_position;
			t_end_dst = MCGRectangleTranslate(t_end_src, 0.0, t_end_src.size.height - height);
			break;
			
		case VE_DOWN:
			height = drect.height * t_position;
			t_end_dst = MCGRectangleTranslate(t_end_src, 0.0, -t_end_src.size.height + height);
			break;
			
		default:
			MCUnreachable();
			break;
	}
	
	t_end_dst.origin.x += drect.x;
	t_end_dst.origin.y += drect.y;
    
	p_target->Composite(t_start_dst, p_start, t_start_src, 1.0, kMCGBlendModeCopy);
	p_target->Composite(t_end_dst, p_end, t_end_src, 1.0, kMCGBlendModeCopy);
	
	return True;
}

Boolean shrinkeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	uint2 height = drect.height - drect.height * delta / duration;
	
	uint32_t t_top, t_bottom;
	
	switch (dir)
	{
		case VE_TOP:
			t_top = 0;
			break;
			
		case VE_CENTER:
			t_top = (drect.height - height) / 2;
			break;
			
		case VE_BOTTOM:
			t_top = drect.height - height;
			break;
			
		default:
			MCUnreachable();
			break;
	}
	t_bottom = t_top + height;
	
	MCGRectangle t_start_src, t_start_dst;
	t_start_src = MCGRectangleMake(0.0, 0.0, drect.width, drect.height);
	t_start_dst = MCGRectangleMake(drect.x, drect.y + t_top, drect.width, height);
	
	p_target->Composite(t_start_dst, p_start, t_start_src, 1.0, kMCGBlendModeCopy);
	
	MCGRectangle t_end_src, t_end_dst;
	if (t_top > 0)
	{
		t_end_src = MCGRectangleMake(0, 0, drect.width, t_top);
		t_end_dst = MCGRectangleTranslate(t_end_src, drect.x, drect.y);
		p_target->Composite(t_end_dst, p_end, t_end_src, 1.0, kMCGBlendModeCopy);
	}
	
	if (t_bottom < drect.height)
	{
		t_end_src = MCGRectangleMake(0, t_bottom, drect.width, drect.height - t_bottom);
		t_end_dst = MCGRectangleTranslate(t_end_src, drect.x, drect.y);
		p_target->Composite(t_end_dst, p_end, t_end_src, 1.0, kMCGBlendModeCopy);
	}
	
	return True;
}

Boolean stretcheffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	uint2 height = drect.height * delta / duration;
	
	uint32_t t_top;
	
	switch (dir)
	{
		case VE_TOP:
			t_top = 0;
			break;
			
		case VE_CENTER:
			t_top = (drect.height - height) / 2;
			break;
			
		case VE_BOTTOM:
			t_top = drect.height - height;
			break;
			
		default:
			MCUnreachable();
			break;
	}
	
	MCGRectangle t_start_src, t_start_dst;
	MCGRectangle t_end_src, t_end_dst;
	t_start_src = MCGRectangleMake(0.0, 0.0, drect.width, drect.height);
	t_start_dst = MCGRectangleTranslate(t_start_src, drect.x, drect.y);
	t_end_src = MCGRectangleMake(0.0, 0.0, drect.width, drect.height);
	t_end_dst = MCGRectangleMake(drect.x, drect.y + t_top, drect.width, height);
	
	p_target->Composite(t_start_dst, p_start, t_start_src, 1.0, kMCGBlendModeCopy);
	p_target->Composite(t_end_dst, p_end, t_end_src, 1.0, kMCGBlendModeCopy);
	
	return True;
}

Boolean venetianeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	int2 newy = Venetiansize * delta / duration;
	
	MCGRectangle t_src_rect, t_dst_rect;
	
	t_src_rect = MCGRectangleMake(0, 0, drect.width, drect.height);
	t_dst_rect = MCGRectangleTranslate(t_src_rect, drect.x, drect.y);
	
	p_target->Composite(t_dst_rect, p_start, t_src_rect, 1.0, kMCGBlendModeCopy);
	
	uint2 height = newy;
	int2 y = 0;
	while (y <= drect.height)
	{
		MCGRectangle t_end_src, t_end_dst;
		t_end_src = MCGRectangleMake(0, y, drect.width, height);
		t_end_dst = MCGRectangleTranslate(t_end_src, drect.x, drect.y);
		p_target->Composite(t_end_dst, p_end, t_end_src, 1.0, kMCGBlendModeCopy);
		y += Venetiansize;
	}
	
	return True;
}

Boolean wipeeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	int2 size;
	if (dir == VE_LEFT || dir == VE_RIGHT)
		size = drect.width * delta / duration;
	else
		size = drect.height * delta / duration;
	
	MCGRectangle t_start_src, t_start_dst;
	MCGRectangle t_end_src, t_end_dst;
	switch (dir)
	{
		case VE_LEFT:
			t_start_src = MCGRectangleMake(0.0, 0.0, drect.width - size, drect.height);
			t_end_src = MCGRectangleMake(drect.width - size, 0.0, size, drect.height);
			break;
			
		case VE_RIGHT:
			t_start_src = MCGRectangleMake(size, 0.0, drect.width - size, drect.height);
			t_end_src = MCGRectangleMake(0.0, 0.0, size, drect.height);
			break;
			
		case VE_UP:
			t_start_src = MCGRectangleMake(0.0, 0.0, drect.width, drect.height - size);
			t_end_src = MCGRectangleMake(0.0, drect.height - size, drect.width, size);
			break;
			
		case VE_DOWN:
			t_start_src = MCGRectangleMake(0.0, size, drect.width, drect.height - size);
			t_end_src = MCGRectangleMake(0.0, 0.0, drect.width, size);
			break;
			
		default:
			MCUnreachable();
			break;
	}
	
	t_start_dst = MCGRectangleTranslate(t_start_src, drect.x, drect.y);
	t_end_dst = MCGRectangleTranslate(t_end_src, drect.x, drect.y);
	p_target->Composite(t_start_dst, p_start, t_start_src, 1.0, kMCGBlendModeCopy);
	p_target->Composite(t_end_dst, p_end, t_end_src, 1.0, kMCGBlendModeCopy);
	
	return True;
}

Boolean zoomeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint32_t p_delta, uint32_t p_duration)
{
	MCGFloat t_position = (MCGFloat)p_delta / (MCGFloat)p_duration;
	uint32_t t_width, t_height;
	
	MCGImageRef t_top, t_bottom;
	if (dir == VE_OUT || dir == VE_OPEN)
	{
		t_bottom = p_start;
		t_top = p_end;
		
		t_width = drect.width * t_position;
		t_height = drect.height * t_position;
	}
	else
	{
		t_bottom = p_end;
		t_top = p_start;
		
		t_width = drect.width * (1.0 - t_position);
		t_height = drect.height * (1.0 - t_position);
	}
	
	MCGRectangle t_src, t_dst, t_top_dst;
	t_src = MCGRectangleMake(0.0, 0.0, drect.width, drect.height);
	t_dst = MCGRectangleTranslate(t_src, drect.x, drect.y);
	t_top_dst = MCGRectangleMake(drect.x + (drect.width - t_width) / 2, drect.y + (drect.height - t_height) / 2, t_width, t_height);
	
	p_target->Composite(t_dst, t_bottom, t_src, 1.0, kMCGBlendModeCopy);
	p_target->Composite(t_top_dst, t_top, t_src, 1.0, kMCGBlendModeCopy);
	
	return True;
}

#if INCLUDE_ZOOM_EFFECT
Boolean zoomeffect_step(const MCRectangle &drect, MCStackSurface *p_target, MCGImageRef p_start, MCGImageRef p_end, Visual_effects dir, uint4 delta, uint4 duration)
{

	static MCRectangle trects[ZOOM_RECTS];
	static MCRectangle trect;
	static uint2 index;
	static uint2 pass;
	static Pixmap tpm;

	if (delta == 0)
	{
		MCscreen->setlineatts(0, LineDoubleDash, CapButt, JoinBevel);
		MCscreen->setforeground(MCscreen->getblack());
		MCscreen->setbackground(MCscreen->getwhite());
		MCscreen->setdashes(0, dashlist, 2);
		index = ZOOM_RECTS - 1;
		pass = 0;
		trects[index].width = trects[index].height = 0;
		int2 newy = getscroll();
		tpm = MCscreen->createpixmap(rect.width, rect.height + newy, 0, False);
		scrollpm(tpm, newy);
		MCscreen->copyarea(window, tpm, 0, 0, 0, rect.width, rect.height,
		                   0, 0, GXcopy);
		delta = 1;
		trect = MCU_reduce_rect(drect, Zoomsize >> 1);
		startx = MCmousex - trect.x;
		starty = MCmousey - trect.y;
	}
	uint2 newwidth;
	uint2 newheight;

	if (dir == VE_OUT || dir == VE_OPEN)
	{
		newwidth = (trect.width * delta / duration + Zoomsize)
		           & ~(Zoomsize - 1);
		newheight = (trect.height * delta / duration + Zoomsize)
		            & ~(Zoomsize - 1);
	}
	else
	{
		newwidth = (trect.width - trect.width * delta / duration + Zoomsize)
		           & ~(Zoomsize - 1);
		newheight = (trect.height - trect.height * delta / duration + Zoomsize)
		            & ~(Zoomsize - 1);
	}
	Boolean done = False;
	if (newwidth != trects[index].width && newheight != trects[index].height)
	{
		index = (index + 1) % ZOOM_RECTS;
		trects[index].width = newwidth;
		trects[index].height = newheight;
		trects[index].x = trect.x + startx
		                  - trects[index].width * startx / trect.width;
		trects[index].y = trect.y + starty
		                  - trects[index].height * starty / trect.height;
		MCscreen->drawrect(trects[index]);
		done = True;
		pass++;
		if (pass >= ZOOM_RECTS)
		{
			uint2 erase = pass % ZOOM_RECTS;
			MCSysRegionHandle oreg = MCscreen->createregion();
			MCSysRegionHandle ireg = MCscreen->createregion();
			MCscreen->unionrectwithregion(trects[erase], oreg, oreg);
			trects[erase] = MCU_reduce_rect(trects[erase], 1);
			MCscreen->unionrectwithregion(trects[erase], ireg, ireg);
			MCscreen->subtractregion(oreg, ireg, oreg);
			MCscreen->setregion(oreg);
			MCscreen->copyarea(tpm, window, 0, 0, 0, rect.width, rect.height,
			                   0, 0, GXcopy);
			MCscreen->clearclip();
			MCscreen->destroyregion(oreg);
			MCscreen->destroyregion(ireg);
		}
	}
	if (delta == duration)
	{
		MCscreen->freepixmap(tpm);
		MCscreen->setlineatts(0, LineSolid, CapButt, JoinBevel);
		MCscreen->setbackground(MCzerocolor);
		MCscreen->copyarea(pixmap, window, 0, 0, 0, rect.width, rect.height,
		                   0, 0, GXcopy);
	}
	return done;
}
#endif
