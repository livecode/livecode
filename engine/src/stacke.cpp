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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "stack.h"
#include "aclip.h"
#include "card.h"
#include "control.h"
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
#include "execpt.h"

#define ZOOM_RECTS 6
#define DISSOLVE_SIZE 16
#define DISSOLVE_MASK 15

static uint2 Checkersize = 64;
static uint2 Venetiansize = 64;
static uint2 Zoomsize = 16;

static int2 dissolve_array[DISSOLVE_SIZE] = { 13, 2, 10, 7, 14, 8, 11, 3, 0, 5, 12, 9, 1, 6, 4, 15};

static Boolean barneffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean checkerboardeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean dissolveeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean iriseffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean pusheffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean revealeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean scrolleffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean shrinkeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean stretcheffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean venetianeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration);
static Boolean wipeeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration);

extern bool MCQTEffectBegin(Visual_effects p_type, const char *p_name, Visual_effects p_direction, Drawable p_target, Drawable p_start, Drawable p_end, const MCRectangle& p_area);
extern bool MCQTEffectStep(uint4 p_delta, uint4 p_duration);
extern void MCQTEffectEnd(void);

extern bool MCCoreImageEffectBegin(const char *p_name, Drawable p_target, Drawable p_source_a, Drawable p_source_b, const MCRectangle& p_rect, MCEffectArgument *p_arguments);
extern bool MCCoreImageEffectStep(float p_time);
extern void MCCoreImageEffectEnd(void);

extern void MCMacDisableScreenUpdates(void);
extern void MCMacEnableScreenUpdates(void);

void MCStack::effectrect(const MCRectangle& p_area, Boolean& r_abort)
{
#ifdef LIBGRAPHICS_BROKEN
	// Get the list of effects.
	MCEffectList *t_effects = MCcur_effects;
	MCcur_effects = NULL;

	// If the window isn't opened or hasn't been attached (plugin) or if we have no
	// snapshot to use, this is a no-op.
	if (!opened || !mode_haswindow() || m_snapshot == nil)
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
	
	// Make a region of the effect area
	MCRegionRef t_effect_rgn;
	MCRegionCreate(t_effect_rgn);
	MCRegionSetRect(t_effect_rgn, t_effect_area);

#if defined(FEATURE_QUICKTIME)
	// MW-2010-07-07: Make sure QT is only loaded if we actually are doing an effect
	if (t_effects != nil)
		if (!MCdontuseQTeffects)
			if (!MCtemplateplayer -> isQTinitted())
				MCtemplateplayer -> initqt();
#endif	

	// Lock the screen to prevent any updates occuring until we want them.
	MCRedrawLockScreen();

	// By default, we have not aborted.
	r_abort = False;
	
	// If we don't have a (soft) windowshape we can directly target the window (on
	// desktop).
	bool t_direct_dst;
#ifndef _MOBILE
	t_direct_dst = true;
	if (m_window_shape != nil && !m_window_shape -> is_sharp)
		t_direct_dst = false;
#else
	t_direct_dst = false;
#endif

	// Loop through the effects until we are done.
	Pixmap t_start_image, t_end_image, t_dst_image;
	t_start_image = nil;
	t_end_image = nil;
	t_dst_image = nil;
	while(t_effects != nil)
	{
		// The target of the effects.
		Drawable t_dst_drawable;
		
		// The effect area from the point of view of the effects is based at
		// (0, 0) for temp dsts.
		MCRectangle t_dst_effect_area;
		if (t_direct_dst)
		{
			t_dst_effect_area = t_effect_area;
			
			// MW-2011-10-18: [[ Bug 9822 ]] In direct mode, make sure we take into account the
			//   scroll.
			t_dst_effect_area . y -= getscroll();
		}
		else
			t_dst_effect_area = MCU_offset_rect(t_effect_area, -t_effect_area . x, -t_effect_area . y);
		
		// If this isn't a plain effect, then we must fetch first and last images.
		if (t_effects -> type != VE_PLAIN)
		{
			// Get the starting image for the effect.
			t_start_image = m_snapshot;
			m_snapshot = nil;

			// Allocate a pixmap for the final image.
			t_end_image = MCscreen -> createpixmap(t_effect_area . width, t_effect_area . height, 32, False);
			if (t_end_image == nil)
				break;

			// Allocate a destination image - if we are not direct.
			if (!t_direct_dst)
			{
				t_dst_image = MCscreen -> createpixmap(t_effect_area . width, t_effect_area . height, 32, False);
				if (t_dst_image == nil)
					break;
			}
			
			// Compute the target drawable - this is the window if direct mode is possible.
			if (t_direct_dst)
				t_dst_drawable = window;
			else
				t_dst_drawable = t_dst_image;
			
			// MW-2011-10-20: [[ Bug 9824 ]] Make sure dst point is correct.
			// Initialize the destination with the start image.
			MCscreen -> copyarea(t_start_image, t_dst_drawable, 32, 0, 0, t_effect_area . width, t_effect_area . height, t_dst_effect_area . x, t_dst_effect_area . y, GXcopy);

			// Render the final image.
			MCContext *t_context;
			t_context = MCscreen -> createcontext(t_end_image, true, true);
			if (t_context == nil)
				break;

			// Configure the context.
			t_context -> setorigin(t_effect_area . x, t_effect_area . y);
			t_context -> setclip(t_effect_area);
			
			// Render an appropriate image
			switch(t_effects -> image)
			{
				case VE_INVERSE:
					t_context -> setfunction(GXinvert);
					t_context -> begin(true);
					curcard -> draw(t_context, t_effect_area, false);
					t_context -> end();
				break;
				
				case VE_BLACK:
					t_context -> setforeground(MCscreen -> getblack());
					t_context -> fillrect(t_effect_area);
				break;
				
				case VE_WHITE:
					t_context -> setforeground(MCscreen -> getwhite());
					t_context -> fillrect(t_effect_area);
				break;
				
				case VE_GRAY:
					t_context -> setforeground(MCscreen -> getgray());
					t_context -> fillrect(t_effect_area);
				break;
				
				default:
					curcard -> draw(t_context, t_effect_area, false);
				break;
			}
			
			// Apply the window mask (if any).
			if (m_window_shape != nil && !m_window_shape -> is_sharp)
				t_context -> applywindowshape(m_window_shape, t_effect_area . width, t_effect_area . height);

			MCscreen -> freecontext(t_context);
		}

		// If there is a sound, then start playing it.
		if (t_effects -> sound != NULL)
		{
			MCAudioClip *acptr;
			if ((acptr = (MCAudioClip *)getobjname(CT_AUDIO_CLIP, t_effects->sound)) == NULL)
			{
				IO_handle stream;
				if ((stream = MCS_open(t_effects->sound, IO_READ_MODE, True, False, 0)) != NULL)
				{
					acptr = new MCAudioClip;
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
				if (MCacptr != NULL)
					MCscreen->addtimer(MCacptr, MCM_internal, PLAY_RATE);
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
#ifdef _MAC_DESKTOP
			if (t_effects -> type == VE_UNDEFINED && MCCoreImageEffectBegin(t_effects -> name, t_dst_drawable, t_start_image, t_end_image, t_dst_effect_area, t_effects -> arguments))
				t_effects -> type = VE_CIEFFECT;
			else
#endif
#ifdef FEATURE_QUICKTIME
			if (MCQTEffectBegin(t_effects -> type, t_effects -> name, t_effects -> direction, t_dst_drawable, t_start_image, t_end_image, t_dst_effect_area))
				t_effects -> type = VE_QTEFFECT;
#endif
		}

		// Now perform the effect loop, but only if there is something to do.
		if (t_effects -> type != VE_PLAIN || old_blendlevel != blendlevel)
		{
			// Calculate timing parameters.
			double t_start_time;
			t_start_time = 0.0;

			uint32_t t_duration;
			t_duration = MCU_max(1, MCeffectrate / (t_effects -> speed - VE_VERY));
			if (t_effects -> type == VE_DISSOLVE)
				t_duration *= 2;

			uint32_t t_delta;
			t_delta = 0;
			for(;;)
			{
				Boolean t_drawn;

				// Render the effect into the dst image buffer.
				switch(t_effects -> type)
				{
					case VE_BARN:
						t_drawn = barneffect_step(t_dst_effect_area, t_dst_drawable, t_start_image, t_end_image, t_effects->direction, t_delta, t_duration);
					break;
					
					case VE_CHECKERBOARD:
						t_drawn = checkerboardeffect_step(t_dst_effect_area, t_dst_drawable, t_start_image, t_end_image, t_effects->direction, t_delta, t_duration);
					break;
					
					case VE_DISSOLVE:
						t_drawn = dissolveeffect_step(t_dst_effect_area, t_dst_drawable, t_start_image, t_end_image, t_effects->direction, t_delta, t_duration);
					break;
				
					case VE_IRIS:
						t_drawn = iriseffect_step(t_dst_effect_area, t_dst_drawable, t_start_image, t_end_image, t_effects->direction, t_delta, t_duration);
					break;
					
					case VE_PUSH:
						t_drawn = pusheffect_step(t_dst_effect_area, t_dst_drawable, t_start_image, t_end_image, t_effects->direction, t_delta, t_duration);
					break;
					
					case VE_REVEAL:
						t_drawn = revealeffect_step(t_dst_effect_area, t_dst_drawable, t_start_image, t_end_image, t_effects->direction, t_delta, t_duration);
					break;
					
					case VE_SCROLL:
						t_drawn = scrolleffect_step(t_dst_effect_area, t_dst_drawable, t_start_image, t_end_image, t_effects->direction, t_delta, t_duration);
					break;
					
					case VE_SHRINK:
						t_drawn = shrinkeffect_step(t_dst_effect_area, t_dst_drawable, t_start_image, t_end_image, t_effects->direction, t_delta, t_duration);
					break;
					
					case VE_STRETCH:
						t_drawn = stretcheffect_step(t_dst_effect_area, t_dst_drawable, t_start_image, t_end_image, t_effects->direction, t_delta, t_duration);
					break;
					
					case VE_VENETIAN:
						t_drawn = venetianeffect_step(t_dst_effect_area, t_dst_drawable, t_start_image, t_end_image, t_effects->direction, t_delta, t_duration);
					break;
					
					case VE_WIPE:
						t_drawn = wipeeffect_step(t_dst_effect_area, t_dst_drawable, t_start_image, t_end_image, t_effects->direction, t_delta, t_duration);
					break;
					
#ifdef _MAC_DESKTOP
					case VE_CIEFFECT:
						t_drawn = MCCoreImageEffectStep((float)t_delta / t_duration);
					break;
#endif

#ifdef FEATURE_QUICKTIME
					case VE_QTEFFECT:
						t_drawn = MCQTEffectStep(t_delta, t_duration);
					break;
#endif
					
					default:
					break;
				}

				// Now redraw the window with the new image.
				if (t_drawn)
				{
					if (t_direct_dst)
						MCscreen -> sync(getw());
					else
					{
						if (m_window_shape != nil && !m_window_shape -> is_sharp)
							setextendedstate(True, ECS_MASK_CHANGED);
						updatewindowwithbuffer(t_dst_image, t_effect_rgn);
					}
				}
				
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
#ifdef FEATURE_QUICKTIME
		if (t_effects -> type == VE_QTEFFECT)
			MCQTEffectEnd();
#endif

		// Free the start image - this will be the snapshot first time
		// round.
		MCscreen -> freepixmap(t_start_image);
		
		// Free the end image.
		MCscreen -> freepixmap(t_dst_image);

		// The snapshot now becomes the end image.
		m_snapshot = t_end_image;
		t_end_image = nil;

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
		MCscreen -> freepixmap(t_start_image);
		MCscreen -> freepixmap(t_end_image);
		MCscreen -> freepixmap(t_dst_image);

		while(t_effects != NULL)
		{
			MCEffectList *t_current_effect;
			t_current_effect = t_effects;
			t_effects = t_effects -> next;
			delete t_current_effect;
		}
	}

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
#endif
}

Boolean barneffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	static int2 x;
	if (delta == 0)
		x = 0;
	int2 newx = (drect.width >> 1) * delta / duration;
	if (newx != x)
	{
		uint2 width = newx - x;
		if (dir == VE_CLOSE)
		{
			MCscreen->copyarea(p_end, p_target, 0, x, 0,
			                   width, drect.height, drect.x + x, drect.y, GXcopy);
			MCscreen->copyarea(p_end, p_target, 0, drect.width - x - width,
			                   0, width, drect.height,
			                   drect.x + drect.width - x - width, drect.y, GXcopy);
		}
		else
		{
			uint2 mid = drect.x + (drect.width >> 1);
			MCscreen->copyarea(p_end, p_target, 0, mid - x - width - drect . x, 0,
			                   width, drect.height, mid - x - width, drect.y, GXcopy);
			MCscreen->copyarea(p_end, p_target, 0, mid + x - drect . x, 0,
			                   width, drect.height, mid + x, drect.y, GXcopy);
		}
		x = newx;
		return True;
	}
	return False;
}

Boolean checkerboardeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	static int2 oldy;
	Boolean done = False;
	
	if (delta == 0)
	{
		oldy = drect . y;
		done = True;
	}
	
	int2 newy = drect . y + (Checkersize << 1) * delta / duration;
	if (newy != oldy)
	{
		for(int2 y = oldy; y < newy; y++)
		{
			MCRectangle trect;
			trect . height = 1;
			trect . width = Checkersize;
			
			Boolean odd = True;
			for(trect . y = -Checkersize; trect . y < drect . height; trect . y += Checkersize)
			{
				odd = !odd;
				Boolean draw = odd;
				for(trect . x = 0; trect . x < drect . width; trect . x += Checkersize)
				{
					if (draw)
						MCscreen -> copyarea(p_end, p_target, 32, trect . x, trect . y + y - drect . y, trect . width, trect . height, trect . x + drect . x, trect . y + y, GXcopy);
					draw = !draw;
				}
			}
		}
		oldy = newy;
		done = True;
	}
	
	return done;
}

Boolean dissolveeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	static uint2 oldindex;
	uint2 y;
	Boolean done = False;

	void *t_result_ptr;
	uint4 t_result_stride;

	void *t_end_ptr;
	uint4 t_end_stride;
	
#ifdef LIBGRAPHICS_BROKEN
#ifndef TARGET_PLATFORM_LINUX
	MCscreen -> lockpixmap(p_start, t_result_ptr, t_result_stride);
	MCscreen -> lockpixmap(p_end, t_end_ptr, t_end_stride);
#endif

	if (delta == 0)
		oldindex = 0;
		
	uint2 index = DISSOLVE_SIZE * delta / duration;
	
	if (index != oldindex)
	{
#ifdef TARGET_PLATFORM_LINUX
		MCBitmap *t_start_image;
		t_start_image = MCscreen -> getimage(p_start, 0, 0, drect . width, drect . height, False);
		t_result_ptr = t_start_image -> data;
		t_result_stride = t_start_image -> bytes_per_line;

		MCBitmap *t_end_image;
		t_end_image = MCscreen -> getimage(p_end, 0, 0, drect . width, drect . height, False);
		t_end_ptr = t_end_image -> data;
		t_end_stride = t_end_image -> bytes_per_line;
#endif

		while(index > oldindex)
		{
			for (y = 0 ; y < drect . height ; y++)
			{
				uint2 offset = dissolve_array[y + dissolve_array[oldindex] & DISSOLVE_MASK];
				uint4 *sptr = (uint4 *)&((uint1 *)t_end_ptr)[y * t_end_stride];
				sptr += offset;
				uint4 *dptr = (uint4 *)&((uint1 *)t_result_ptr)[y * t_result_stride];
				dptr += offset;
				uint4 *eptr = (uint4 *)&((uint1 *)t_result_ptr)[(y + 1) * t_result_stride];
				while (dptr < eptr)
				{
					*dptr = *sptr;
					sptr += DISSOLVE_SIZE;
					dptr += DISSOLVE_SIZE;
				}
			}
			oldindex++;
		}

#ifdef TARGET_PLATFORM_LINUX
		MCscreen -> putimage(p_start, t_start_image, 0, 0, 0, 0, drect . width, drect . height);
		MCscreen -> destroyimage(t_start_image);
		MCscreen -> destroyimage(t_end_image);
#endif

		MCscreen -> copyarea(p_start, p_target, 0, 0, 0, drect . width, drect . height, drect . x, drect . y, GXcopy);
		done = True;
	}
	
#ifndef TARGET_PLATFORM_LINUX
	MCscreen -> unlockpixmap(p_end, t_end_ptr, t_end_stride);
	MCscreen -> unlockpixmap(p_start, t_result_ptr, t_result_stride);
#endif
#endif
	
	return done;
}

Boolean iriseffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	static MCRectangle orect;
	
	if (delta == 0)
		orect . width = orect . height = 0;
	
	MCRectangle trect;
	Boolean done = False;
	
	if (dir == VE_OPEN)
	{
		trect.width =	drect.width * delta / duration;
		trect.height = drect.height * delta / duration;
		if (trect.width != orect.width || trect.height != orect.height)
		{
			trect = MCU_center_rect(drect, trect);
			MCscreen -> copyarea(p_end, p_target, 0, orect . x - drect . x, orect . y - drect . y, orect . width, orect . height, orect . x, orect . y, GXcopy);
			done = True;
		}
	}
	else
	{
		trect.width = drect.width - drect.width * delta / duration;
		trect.height = drect.height - drect.height * delta / duration;
		if (trect.width != orect.width || trect.height != orect.height)
		{
			trect = MCU_center_rect(drect, trect);
			
			uint4 oleft, otop, oright, obottom;
			oleft = orect . x - drect . x;
			otop = orect . y - drect . y;
			oright = oleft + orect . width;
			obottom = otop + orect . height;

			uint4 ileft, itop, iright, ibottom;
			ileft = trect . x - drect . x;
			itop = trect . y - drect . y;
			iright = ileft + trect . width;
			ibottom = itop + trect . height;

			MCscreen -> copyarea(p_end, p_target, 0, oleft, otop, ileft - oleft, obottom - otop, orect . x, orect . y, GXcopy);
			MCscreen -> copyarea(p_end, p_target, 0, iright, otop, oright - iright, obottom - otop, orect . x + iright - oleft, orect . y, GXcopy);
			MCscreen -> copyarea(p_end, p_target, 0, ileft, otop, iright - ileft, itop - otop, orect . x + ileft - oleft, orect . y, GXcopy);
			MCscreen -> copyarea(p_end, p_target, 0, ileft, ibottom, iright - ileft, obottom - ibottom, orect . x + ileft - oleft, orect . y + ibottom - otop, GXcopy);

			MCscreen -> copyarea(p_start, p_target, 0, trect . x - drect . x, trect . y - drect . y, trect . width, trect . height, trect . x, trect . y, GXcopy);
			done = True;
		}
	}
	
	if (done)
		orect = trect;
	
	return done;
}

Boolean pusheffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	static uint2 oldsize;
	if (delta == 0)
		oldsize = 0;

	if (dir == VE_LEFT || dir == VE_RIGHT)
	{
		uint2 size = drect.width * delta / duration;
		if (size != oldsize)
		{
			if (dir == VE_LEFT)
			{
				MCscreen->copyarea(p_start, p_target, 0, size, 0, drect.width - size, drect.height, drect.x, drect.y, GXcopy);
				MCscreen->copyarea(p_end, p_target, 0, 0, 0, size, drect.height, drect.x + drect.width - size, drect.y, GXcopy);
			}
			else
			{
				MCscreen->copyarea(p_start, p_target, 0, 0, 0, drect.width - size, drect.height, drect.x + size, drect.y, GXcopy);
				MCscreen->copyarea(p_end, p_target, 0, drect.width - size, 0, size, drect.height, drect.x, drect.y, GXcopy);
			}
			oldsize = size;
			return True;
		}
	}
	else
	{
		uint2 size = drect.height * delta / duration;
		if (size != oldsize)
		{
			if (dir == VE_UP)
			{
				MCscreen->copyarea(p_start, p_target, 0, 0, size, drect.width, drect.height - size, drect.x, drect.y, GXcopy);
				MCscreen->copyarea(p_end, p_target, 0, 0, 0, drect.width, size, drect.x, drect.y + drect.height - size, GXcopy);
			}
			else
			{
				MCscreen->copyarea(p_start, p_target, 0, 0, 0, drect.width, drect.height - size, drect.x, drect.y + size, GXcopy);
				MCscreen->copyarea(p_end, p_target, 0, 0, drect.height - size, drect.width, size, drect.x, drect.y, GXcopy);
			}
			oldsize = size;
			return True;
		}
	}
	return False;
}

Boolean revealeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	static uint2 oldsize;
	if (delta == 0)
		oldsize = 0;

	if (dir == VE_LEFT || dir == VE_RIGHT)
	{
		uint2 size = drect.width * delta / duration;
		if (size != oldsize)
		{
			if (dir == VE_LEFT)
			{
				MCscreen->copyarea(p_start, p_target, 0, size, 0, drect.width - size, drect.height, drect.x, drect.y, GXcopy);
				MCscreen->copyarea(p_end, p_target, 0, drect.width - size, 0, size, drect.height, drect.x + drect.width - size, drect.y, GXcopy);
			}
			else
			{
				MCscreen->copyarea(p_start, p_target, 0, 0, 0, drect.width - size, drect.height, drect.x + size, drect.y, GXcopy);
				MCscreen->copyarea(p_end, p_target, 0, 0, 0, size, drect.height, drect.x, drect.y, GXcopy);
			}
			oldsize = size;
			return True;
		}
	}
	else
	{
		uint2 size = drect.height * delta / duration;
		if (size != oldsize)
		{
			if (dir == VE_UP)
			{
				MCscreen->copyarea(p_start, p_target, 0, 0, size, drect.width, drect.height - size, drect.x, drect.y, GXcopy);
				MCscreen->copyarea(p_end, p_target, 0, 0, drect.height - size, drect.width, size, drect.x, drect.y + drect.height - size, GXcopy);
			}
			else
			{
				MCscreen->copyarea(p_start, p_target, 0, 0, 0, drect.width, drect.height - size, drect.x, drect.y + size, GXcopy);
				MCscreen->copyarea(p_end, p_target, 0, 0, 0, drect.width, size, drect.x, drect.y, GXcopy);
			}
			oldsize = size;
			return True;
		}
	}
	return False;
}

Boolean scrolleffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	static uint2 oldsize;
	if (delta == 0)
		oldsize = 0;

	if (dir == VE_LEFT || dir == VE_RIGHT)
	{
		uint2 width = drect.width * delta / duration;
		if (width != oldsize)
		{
			if (dir == VE_LEFT)
				MCscreen->copyarea(p_end, p_target, 0, 0, 0, width, drect.height, drect.x + drect.width - width, drect.y, GXcopy);
			else
				MCscreen->copyarea(p_end, p_target, 0, drect.width - width, 0, width, drect.height, drect.x, drect.y, GXcopy);
			oldsize = width;
			return True;
		}
	}
	else
	{
		uint2 height = drect.height * delta / duration;
		if (height != oldsize)
		{
			if (dir == VE_UP)
				MCscreen->copyarea(p_end, p_target, 0, 0, 0, drect.width, height, drect.x, drect.y + drect.height - height, GXcopy);
			else
				MCscreen->copyarea(p_end, p_target, 0, 0, drect.height - height, drect.width, height, drect.x, drect.y, GXcopy);
			oldsize = height;
			return True;
		}
	}
	return False;
}

Boolean shrinkeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	static Drawable result;
	static uint2 oldheight;

	Boolean done = False;
#ifdef LIBGRAPHICS_BROKEN
	if (delta == 0)
	{
		result = MCscreen->createpixmap(drect.width, drect.height, 32, False);
		oldheight = drect.height;
	}
	uint2 height = drect.height - drect.height * delta / duration;
	if (height != 0 && height != oldheight)
	{
		uint2 offset = 0;
		switch (dir)
		{
		case VE_BOTTOM:
			offset = drect.height - height;
			break;
		case VE_CENTER:
			offset = (drect.height - height) >> 1;
			break;
		default:
			break;
		}
		
		void *t_result_ptr;
		uint4 t_result_stride;
		void *t_start_ptr;
		uint4 t_start_stride;
				
#ifdef TARGET_PLATFORM_LINUX
		MCBitmap *t_result_image;
		t_result_image = MCscreen -> getimage(result, 0, 0, drect . width, drect . height, False);
		t_result_ptr = t_result_image -> data;
		t_result_stride = t_result_image -> bytes_per_line;

		MCBitmap *t_start_image;
		t_start_image = MCscreen -> getimage(p_start, 0, 0, drect . width, drect . height, False);
		t_start_ptr = t_start_image -> data;
		t_start_stride = t_start_image -> bytes_per_line;
#else
		MCscreen -> lockpixmap(result, t_result_ptr, t_result_stride);
		MCscreen -> lockpixmap(p_start, t_start_ptr, t_start_stride);
#endif

		uint2 dy;
		for (dy = 0 ; dy < height ; dy++)
		{
			uint2 sy = dy * drect.height / height;
			memcpy(&((uint1 *)t_result_ptr)[dy * t_result_stride], &((uint1 *)t_start_ptr)[sy * t_start_stride], drect . width * 4);
		}
		
#ifdef TARGET_PLATFORM_LINUX
		MCscreen -> destroyimage(t_start_image);

		MCscreen -> putimage(result, t_result_image, 0, 0, 0, 0, drect . width, drect . height);
		MCscreen -> destroyimage(t_result_image);
#else
		MCscreen -> unlockpixmap(p_start, t_start_ptr, t_start_stride);
		MCscreen -> unlockpixmap(result, t_result_ptr, t_result_stride);
#endif

		MCscreen->copyarea(result, p_target, 0, 0, 0, drect . width, height, drect.x, drect.y + offset, GXcopy);
		uint2 halfheight;
		switch (dir)
		{
		case VE_BOTTOM:
			MCscreen->copyarea(p_end, p_target, 0, 0, drect.height - oldheight, drect.width, oldheight - height, drect.x, drect.y + drect.height - oldheight, GXcopy);
			break;
		case VE_CENTER:
			halfheight = ((oldheight - height) >> 1) + 1;
			MCscreen->copyarea(p_end, p_target, 0, 0, ((drect.height - oldheight) >> 1), drect.width, halfheight, drect.x, drect.y + ((drect.height - oldheight) >> 1), GXcopy);
			MCscreen->copyarea(p_end, p_target, 0, 0, ((drect.height + height) >> 1), drect.width, halfheight, drect.x, drect.y + ((drect.height + height) >> 1), GXcopy);
			break;
		default:
			MCscreen->copyarea(p_end, p_target, 0, 0, height, drect.width, oldheight - height, drect.x, drect.y + height, GXcopy);
			break;
		}
		
		oldheight = height;
		done = True;
	}
	if (delta == duration)
		MCscreen->freepixmap(result);
#endif
	return done;
}

Boolean stretcheffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	static Drawable result;
	static uint2 oldheight;

	Boolean done = False;
#ifdef LIBGRAPHICS_BROKEN
	if (delta == 0)
	{
		result = MCscreen->createpixmap(drect.width, drect.height, 32, False);
		oldheight = 0;
	}

	uint2 height = drect.height * delta / duration;
	if (height != oldheight)
	{
		uint2 offset;
		switch (dir)
		{
		case VE_BOTTOM:
			offset = drect.height - height;
			break;
		case VE_CENTER:
			offset = (drect.height - height) >> 1;
			break;
		default:
			offset = 0;
			break;
		}
		
		void *t_result_ptr;
		uint4 t_result_stride;
		void *t_end_ptr;
		uint4 t_end_stride;
		
#ifdef TARGET_PLATFORM_LINUX
		MCBitmap *t_result_image;
		t_result_image = MCscreen -> getimage(result, 0, 0, drect . width, drect . height, False);
		t_result_ptr = t_result_image -> data;
		t_result_stride = t_result_image -> bytes_per_line;

		MCBitmap *t_end_image;
		t_end_image = MCscreen -> getimage(p_end, 0, 0, drect . width, drect . height, False);
		t_end_ptr = t_end_image -> data;
		t_end_stride = t_end_image -> bytes_per_line;
#else
		MCscreen -> lockpixmap(result, t_result_ptr, t_result_stride);
		MCscreen -> lockpixmap(p_end, t_end_ptr, t_end_stride);
#endif
		
		for (uint2 dy = 0 ; dy < height ; dy++)
		{
			uint2 sy = dy * drect.height / height;
			memcpy(&((uint1 *)t_result_ptr)[dy * t_result_stride], &((uint1 *)t_end_ptr)[sy * t_end_stride], drect . width * 4);
		}
		
#ifdef TARGET_PLATFORM_LINUX
		MCscreen -> destroyimage(t_end_image);

		MCscreen -> putimage(result, t_result_image, 0, 0, 0, 0, drect . width, drect . height);
		MCscreen -> destroyimage(t_result_image);
#else
		MCscreen -> unlockpixmap(p_start, t_end_ptr, t_end_stride);
		MCscreen -> unlockpixmap(result, t_result_ptr, t_result_stride);
#endif
		
		MCscreen->copyarea(result, p_target, 0, 0, 0, drect . width, height, drect.x, drect.y + offset, GXcopy);

		oldheight = height;
		done = True;
	}
	
	if (delta == duration)
		MCscreen->freepixmap(result);
#endif
		
	return done;
}

Boolean venetianeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	static int2 oldy;
	if (delta == 0)
		oldy = 0;

	int2 newy = Venetiansize * delta / duration;
	if (newy != oldy)
	{
		uint2 height = newy - oldy;
		int2 y = oldy;
		while (y <= drect.height - height)
		{
			MCscreen->copyarea(p_end, p_target, 0, 0, y,
			                   drect.width, height, drect.x, drect.y + y, GXcopy);
			y += Venetiansize;
		}
		oldy = newy;
		return True;
	}
	return False;
}

Boolean wipeeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
{
	static int2 old;
	if (delta == 0)
		old = 0;

	if (dir == VE_LEFT || dir == VE_RIGHT)
	{
		int2 newx = drect.width * delta / duration;
		if (newx != old)
		{
			uint2 width = newx - old;
			int2 x;
			if (dir == VE_LEFT)
				x = drect.width - newx;
			else
				x = old;
			MCscreen->copyarea(p_end, p_target, 0, x, 0,
			                   width, drect.height, drect.x + x, drect.y, GXcopy);
			old = newx;
			return True;
		}
	}
	else
	{
		int2 newy = drect.height * delta / duration;
		if (newy != old)
		{
			uint2 height = newy - old;
			int2 y;
			if (dir == VE_DOWN)
				y = old;
			else
				y = drect.height - newy;
			MCscreen->copyarea(p_end, p_target, 0, 0, y,
			                   drect.width, height, drect.x, drect.y + y, GXcopy);
			old = newy;
			return True;
		}
	}
	return False;
}

#if 0
Boolean zoomeffect_step(const MCRectangle &drect, Drawable p_target, Drawable p_start, Drawable p_end, Visual_effects dir, uint4 delta, uint4 duration)
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
