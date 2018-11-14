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

#include "stack.h"
#include "card.h"
#include "util.h"
#include "visual.h"
#include "globals.h"
#include "region.h"
#include "redraw.h"
#include "tilecache.h"

#include "mbldc.h"
#include "mbliphoneapp.h"

#include "resolution.h"

#include <UIKit/UIKit.h>
#include <QuartzCore/QuartzCore.h>

////////////////////////////////////////////////////////////////////////////////

extern UIView *MCIPhoneGetView(void);
extern float MCIPhoneGetDeviceScale();
extern float MCIPhoneGetResolutionScale(void);
extern bool MCIPhoneGrabUIViewSnapshot(UIView *p_view, CGRect &p_bounds, UIImage *&r_image);

////////////////////////////////////////////////////////////////////////////////

// This global tells RevMobileContainerView that its not UIWebView modifying us.
extern bool g_engine_manipulating_container;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCEffectDelegate : NSObject
{
	BOOL m_finished;
}

- (void) animationDidStart: (CAAnimation *)theAnimation;
- (void) animationDidStop: (CAAnimation *)theAnimation finished: (BOOL)flag;

- (void) blockAnimationDidStop: (NSString *)animationID finished: (NSNumber *)finished context: (void*)context;
@end

@implementation com_runrev_livecode_MCEffectDelegate
- (void) animationDidStart: (CAAnimation *)theAnimation
{
}

- (void) animationDidStop: (CAAnimation *)theAnimation finished: (BOOL)flag
{
	m_finished = YES;
}

- (void) blockAnimationDidStop: (NSString *)animationID finished: (NSNumber*)finished context: (void*)context
{
	m_finished = YES;
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

- (void) setFinished: (BOOL)finished
{
	m_finished = finished;
}

- (BOOL) finished
{
	return m_finished;
}
@end

NSString *transition_direction(uint32_t p_direction)
{
	switch (p_direction)
	{
		case VE_DOWN:
			return kCATransitionFromBottom;
		case VE_UP:
			return kCATransitionFromTop;
		case VE_LEFT:
			return kCATransitionFromRight;
		case VE_RIGHT:
			return kCATransitionFromLeft;
		default:
			return nil;
	}
}

bool need_current_image(uint32_t p_effect_type)
{
	switch (p_effect_type)
	{
		case VE_DISSOLVE:
			return true;
		case VE_SCROLL:
			return true;
		case VE_PUSH:
			return true;
		case VE_REVEAL:
			return true;
		case VE_CURL:
			return true;
		case VE_FLIP:
			return true;
	}
	
	return false;
}

bool need_updated_image(uint32_t p_effect_type)
{
	switch (p_effect_type)
	{
		case VE_DISSOLVE:
			return false;
		case VE_SCROLL:
			return true;
		case VE_PUSH:
			return true;
		case VE_REVEAL:
			return false;
		case VE_CURL:
			return true;
		case VE_FLIP:
			return true;
	}
	
	return false;
}

void setup_layer_transition(UIView *p_new_view, uint32_t p_effect_type, uint32_t p_effect_direction)
{
	bool t_bring_to_front = false;
	bool t_start_offscreen = false;
	switch (p_effect_type)
	{
		case VE_SCROLL:
			t_bring_to_front = true;
			t_start_offscreen = true;
			break;
		case VE_PUSH:
			t_bring_to_front = true;
			t_start_offscreen = true;
			break;
		case VE_REVEAL:
			t_bring_to_front = false;
			t_start_offscreen = false;
			break;
	}
	if (t_bring_to_front)
		[p_new_view.superview bringSubviewToFront: p_new_view];
	else
		[p_new_view.superview sendSubviewToBack: p_new_view];
	if (t_start_offscreen)
	{
		switch (p_effect_direction)
		{
			case VE_UP:
				p_new_view.frame = CGRectOffset(p_new_view.frame, 0, p_new_view.frame.size.height);
				break;
			case VE_DOWN:
				p_new_view.frame = CGRectOffset(p_new_view.frame, 0, -p_new_view.frame.size.height);
				break;
			case VE_LEFT:
				p_new_view.frame = CGRectOffset(p_new_view.frame, p_new_view.frame.size.width, 0);
				break;
			case VE_RIGHT:
				p_new_view.frame = CGRectOffset(p_new_view.frame, -p_new_view.frame.size.width, 0);
				break;
		}
	}

}

void layer_animation_changes(UIView *p_new_view, UIView *p_old_view, uint32_t p_effect_type, uint32_t p_effect_direction)
{
	UIView *t_moving_view = (p_effect_type == VE_REVEAL) ? p_old_view : p_new_view;
	int32_t t_offset_x, t_offset_y;
	switch (p_effect_direction)
	{
		case VE_UP:
			t_offset_x = 0;
			t_offset_y = -p_new_view.frame.size.height;
			break;
		case VE_DOWN:
			t_offset_x = 0;
			t_offset_y = p_new_view.frame.size.height;
			break;
		case VE_LEFT:
			t_offset_x = -p_new_view.frame.size.width;
			t_offset_y = 0;
			break;
		case VE_RIGHT:
			t_offset_x = p_new_view.frame.size.width;
			t_offset_y = 0;
			break;
	}
	switch (p_effect_type)
	{
		case VE_REVEAL:
			p_old_view.frame = CGRectOffset(p_old_view.frame, t_offset_x, t_offset_y);
			break;
		case VE_PUSH:
			p_new_view.frame = CGRectOffset(p_new_view.frame, t_offset_x, t_offset_y);
			p_old_view.frame = CGRectOffset(p_old_view.frame, t_offset_x, t_offset_y);
			break;
		default:
			p_new_view.frame = CGRectOffset(p_new_view.frame, t_offset_x, t_offset_y);
			break;
	}
}

/*
 *******************************************************************************
 *
 * in order to implement visual effects within a sub-region of the displayed
 * area, we need to do the following:
 * create a snapshot image of the screen region affected
 * redraw to the offscreen buffer
 * create a snapshot of the updated buffer region
 * create image views for each of these snapshots and add to the view heirarchy
 * set up the transition animation between these image views
 * wait for the animation to finish
 * remove and release the image views and snapshots
 *
 *******************************************************************************
*/
 
// available iPhone Transition Effects:
// CATransition Effects:
//    Fade
//    MoveIn (From Right/Left/Top/Bottom)       (new card slides in)
//    Push   (From Right/Left/Top/Bottom)       (new in, old out)
//    Reveal (From Right/Left/Top/Bottom)       (old card slides out)
// UIViewAnimationTransition:
//    FlipFromLeft
//    FlipFromRight
//    CurlUp
//    CurlDown
//
// LiveCode equivalents:
//    dissolve (Fade)
//    scroll   (MoveIn)
//    push     (Push)
//    reveal   (Reveal)

extern bool MCGImageToCGImage(MCGImageRef p_src, const MCGIntegerRectangle &p_src_rect, bool p_invert, CGImageRef &r_image);

// IM-2013-07-18: [[ ResIndependence ]] added scale parameter to support hi-res images
static bool MCGImageToUIImage(MCGImageRef p_image, bool p_copy, MCGFloat p_scale, UIImage *&r_uiimage)
{
   if (p_image == nil)
    {
        r_uiimage = nil;
        return false;
    }
    
	bool t_success = true;
	
	CGImageRef t_cg_image = nil;
	UIImage *t_image = nil;
	
	t_success = MCGImageToCGImage(p_image, MCGIntegerRectangleMake(0, 0, MCGImageGetWidth(p_image), MCGImageGetHeight(p_image)), false, t_cg_image);
	
	if (t_success)
		t_success = nil != (t_image = [UIImage imageWithCGImage:t_cg_image scale:p_scale orientation:/*0.0*/UIImageOrientationUp]);
	
	if (t_cg_image != nil)
		CGImageRelease(t_cg_image);
	
	if (t_success)
		r_uiimage = t_image;
	
	return t_success;
}

struct effectrect_t
{
	MCColor bg_color;
	MCGImageRef snapshot;
	MCEffectList *effect;
	MCRectangle effect_area;
	UIImage *current_image;
	UIImage *updated_image;
	UIView *current_image_view;
	UIView *updated_image_view;
	UIView *effect_view;
	UIView *main_view;
	UIView *composite_view;
	UIView *background_view;
	com_runrev_livecode_MCEffectDelegate *effect_delegate;
	real8 duration;
	UIViewAnimationTransition transition;
};

static void effectrect_phase_1(void *p_context)
{
	effectrect_t *ctxt;
	ctxt = (effectrect_t *)p_context;
	// IM-2014-01-30: [[ HiDPI ]] Use resolution scale for snapshots - this gives us the correct size in screen coords 
	/* UNCHECKED */ MCGImageToUIImage(ctxt->snapshot, true, MCIPhoneGetResolutionScale(), ctxt->current_image);
	[ctxt -> current_image retain];
}

static void effectrect_phase_2(void *p_context)
{
	effectrect_t *ctxt;
	ctxt = (effectrect_t *)p_context;
	// IM-2014-01-30: [[ HiDPI ]] Use resolution scale for snapshots - this gives us the correct size in screen coords 
	/* UNCHECKED */ MCGImageToUIImage(ctxt->snapshot, false, MCIPhoneGetResolutionScale(), ctxt->updated_image);
	[ctxt -> updated_image retain];
	
	CGSize t_img_size;
	t_img_size = [ctxt -> updated_image size];
	
	ctxt -> current_image_view = [[UIImageView alloc] initWithImage: ctxt -> current_image];
	[ctxt -> current_image release];
	ctxt -> updated_image_view = [[UIImageView alloc] initWithImage: ctxt -> updated_image];
	[ctxt -> updated_image release];
	
	// IM-2014-01-30: [[ HiDPI ]] Convert logical to screen coords
	float t_scale = MCScreenDC::logicaltoscreenscale();
	
	// MW-2011-09-27: [[ iOSApp ]] Adjust for content origin.
	// IM-2014-01-30: [[ HiDPI ]] Scale effect area from logical to screen coords
	CGRect t_bounds;
	t_bounds = CGRectMake(ctxt -> effect_area.x * t_scale + [ctxt -> main_view frame] . origin . x,
						  ctxt -> effect_area.y * t_scale + [ctxt -> main_view frame] . origin . y,
						  ctxt -> effect_area.width * t_scale,
						  ctxt -> effect_area.height * t_scale);
	ctxt -> effect_view = [[UIView alloc] initWithFrame: t_bounds];
	
	[ctxt -> effect_view setClipsToBounds: YES];
	[ctxt -> effect_view addSubview: ctxt -> updated_image_view];
	[ctxt -> effect_view addSubview: ctxt -> current_image_view];
	
	// IM-2013-07-18: [[ ResIndependence ]] we don't need to scale the bounds any more
	// as the UIImage scale is set appropriately
	
	ctxt -> duration = MCU_max(1, MCeffectrate / (ctxt -> effect->speed - VE_VERY)) / 1000.0;
	
	ctxt -> effect_delegate = [[com_runrev_livecode_MCEffectDelegate alloc] init];
	[ctxt -> effect_delegate setFinished:NO];
	
	ctxt -> composite_view = [ctxt -> main_view superview];
	
	[ctxt -> composite_view addSubview:ctxt -> effect_view];
	
	if (ctxt -> effect->type == VE_DISSOLVE)
	{
		[ctxt -> current_image_view setAlpha: 1.0];
		
		[UIView beginAnimations: nil context: nil];
		[UIView setAnimationDuration: ctxt -> duration];
		[UIView setAnimationDidStopSelector: @selector(blockAnimationDidStop:finished:context:)];
		[UIView setAnimationDelegate: ctxt -> effect_delegate];
		[ctxt -> current_image_view setAlpha: 0.0];
		[UIView commitAnimations];
	}
	else if (ctxt -> effect->type == VE_SCROLL || ctxt -> effect->type == VE_PUSH || ctxt -> effect->type == VE_REVEAL)
	{
		setup_layer_transition(ctxt -> updated_image_view, ctxt -> effect->type, ctxt -> effect->direction);
		
		[UIView beginAnimations: nil context: nil];
		[UIView setAnimationDuration: ctxt -> duration];
		[UIView setAnimationDidStopSelector: @selector(blockAnimationDidStop:finished:context:)];
		[UIView setAnimationDelegate: ctxt -> effect_delegate];
		layer_animation_changes(ctxt -> updated_image_view, ctxt -> current_image_view, ctxt -> effect->type, ctxt -> effect->direction);
		[UIView commitAnimations];
	}
	else if (ctxt -> effect->type == VE_WIPE)
	{
		// The initial bounds of the current image view are the full bounds.
		CGRect t_initial_bounds;
		t_initial_bounds = CGRectMake(0, 0, t_bounds . size . width, t_bounds . size . height);
		
		// The final bounds of the updated image view depends on the direction.
		CGRect t_final_clip_bounds, t_final_image_bounds;
		t_final_clip_bounds = t_initial_bounds;
		t_final_image_bounds = t_initial_bounds;
		switch(ctxt -> effect -> direction)
		{
			case VE_LEFT:
				t_final_clip_bounds . size . width = 0.0f;
				break;
			case VE_RIGHT:
				t_final_clip_bounds . origin . x += t_final_clip_bounds . size . width;
				t_final_clip_bounds . size . width = 0.0f;
				t_final_image_bounds . origin . x -= t_final_image_bounds . size . width;
				break;
			case VE_UP:
				t_final_clip_bounds . size . height = 0.0f;
				break;
			case VE_DOWN:
				t_final_clip_bounds . origin . y += t_final_clip_bounds . size . height;
				t_final_clip_bounds . size . height = 0.0f;
				t_final_image_bounds . origin . y -= t_final_image_bounds . size . height;
				break;
		}
		
		// Create a clipping container for the updated image view.
		UIView *t_clipping_view;
		t_clipping_view = [[UIView alloc] initWithFrame: t_initial_bounds];
		[t_clipping_view setClipsToBounds: YES];
		
		// Place the current image view within the clipping view within the effect view.
		[ctxt -> current_image_view removeFromSuperview];
		[t_clipping_view addSubview: ctxt -> current_image_view];
		[ctxt -> effect_view addSubview: t_clipping_view];
		
		// Prepare the animation.
		[UIView beginAnimations: nil context: nil];
		[UIView setAnimationDuration: ctxt -> duration];
		[UIView setAnimationDidStopSelector: @selector(blockAnimationDidStop:finished:context:)];
		[UIView setAnimationDelegate: ctxt -> effect_delegate];
		[t_clipping_view setFrame: t_final_clip_bounds];
		[ctxt -> current_image_view setFrame: t_final_image_bounds];
		[UIView commitAnimations];
		
		// Swap the image view for the clipping view.
		[ctxt -> current_image_view release];
		ctxt -> current_image_view = t_clipping_view;
	}
	else if (ctxt -> effect->type == VE_CURL || ctxt -> effect->type == VE_FLIP)
	{
		ctxt -> transition = UIViewAnimationTransitionNone;
		if (ctxt -> effect->type == VE_CURL)
		{
			if (ctxt -> effect->direction == VE_UP)
				ctxt -> transition = UIViewAnimationTransitionCurlUp;
			else if (ctxt -> effect->direction == VE_DOWN)
				ctxt -> transition = UIViewAnimationTransitionCurlDown;
		}
		else if (ctxt -> effect->type == VE_FLIP)
		{
			if (ctxt -> effect->direction == VE_RIGHT)
				ctxt -> transition = UIViewAnimationTransitionFlipFromLeft;
			else if (ctxt -> effect->direction == VE_LEFT)
				ctxt -> transition = UIViewAnimationTransitionFlipFromRight;
			
			ctxt -> background_view = [[UIView alloc] initWithFrame: t_bounds];
			
			uint16_t t_index;
			UIColor *t_bgcolor = nil;
			t_bgcolor = [UIColor colorWithRed: ctxt -> bg_color.red / 65535.0
										green: ctxt -> bg_color.green / 65535.0
										 blue: ctxt -> bg_color.blue / 65535.0
										alpha: 1.0];
			
			[ctxt -> background_view setBackgroundColor: t_bgcolor];
			[ctxt -> composite_view addSubview: ctxt -> background_view];
			[ctxt -> composite_view bringSubviewToFront: ctxt -> effect_view];
		}
		
		[ctxt -> current_image_view setHidden: NO];
		[ctxt -> updated_image_view setHidden: YES];
		[ctxt -> effect_view setClipsToBounds: NO];
	}
	else
		[ctxt -> effect_delegate setFinished:YES];
}

static void effectrect_phase_3(void *p_context)
{
	effectrect_t *ctxt;
	ctxt = (effectrect_t *)p_context;
	
	if (ctxt -> effect->type == VE_CURL || ctxt -> effect->type == VE_FLIP)
	{
		[UIView beginAnimations: nil context: nil];
	
		[UIView setAnimationTransition:ctxt -> transition forView:ctxt -> effect_view cache:YES];
		[UIView setAnimationDuration:ctxt -> duration];
		[UIView setAnimationDidStopSelector: @selector(blockAnimationDidStop:finished:context:)];
		[UIView setAnimationDelegate: ctxt -> effect_delegate];
		
		[ctxt -> current_image_view setHidden: YES];
		[ctxt -> updated_image_view setHidden: NO];
		
		[UIView commitAnimations];
	}
	
	// MW-2011-01-05: Add support for 'sound'.
	extern bool MCSystemPlaySound(MCStringRef, bool);
	if (ctxt -> effect -> sound != nil)
	{
		MCSystemPlaySound(ctxt->effect->sound, false);
	}
}

static void effectrect_phase_4(void *p_context)
{
	effectrect_t *ctxt;
	ctxt = (effectrect_t *)p_context;
	
	[ctxt -> background_view removeFromSuperview];
	[ctxt -> background_view release];
	[ctxt -> effect_view removeFromSuperview];
	[ctxt -> effect_view release];
	[ctxt -> current_image_view release];
	[ctxt -> updated_image_view release];
	
	[ctxt -> effect_delegate release];
}

void MCStack::effectrect(const MCRectangle& p_rect, Boolean& r_abort)
{
	MCEffectList *t_effects = MCcur_effects;
	MCcur_effects = NULL;
	
	// MW-2012-07-19: [[ Bug ]] Disable the redraw interval feature temporarily and
	// ensure screen updates are enabled.
	bool t_updates_were_enabled;
	t_updates_were_enabled = MCRedrawIsScreenUpdateEnabled();
	if (!t_updates_were_enabled)
		MCRedrawEnableScreenUpdates();
	[MCIPhoneGetApplication() disableRedrawInterval];

	// MW-2011-10-01: We only support a single effect on mobile at this time.
	// MW-2011-10-17: [[ Bug 9802 ]] Only perform an effect if we have a snapshot.
	if (t_effects != NULL && m_snapshot != NULL)
	{
		effectrect_t ctxt;
		memset(&ctxt, 0, sizeof(effectrect_t));
		ctxt . effect = t_effects;
		ctxt . current_image = nil;
		ctxt . updated_image = nil;
		ctxt . main_view = MCIPhoneGetView();
		
		uint2 t_index;
		if (getcindex(P_BACK_COLOR - P_FORE_COLOR, t_index))
			ctxt . bg_color = colors[t_index];
		else
			ctxt . bg_color = MCscreen -> background_pixel;

		
		// MW-2011-10-24: [[ Bug 9831 ]] Calculate the area of interest.
		// IM-2013-10-03: [[ FullscreenMode ]] transform effect area to logical screen coords
		MCRectangle t_effect_area;
		t_effect_area = curcard -> getrect();
		t_effect_area . y = getscroll();
		t_effect_area . height -= t_effect_area . y;
		t_effect_area = MCU_intersect_rect(t_effect_area, p_rect);
		
		ctxt . effect_area = MCRectangleGetTransformedBounds(t_effect_area, view_getviewtransform());
        
        // MW-2013-10-29: [[ Bug 11330 ]] Make sure the effect area is cropped to the visible
        //   area.
        ctxt . effect_area = MCU_intersect_rect(ctxt . effect_area, MCU_make_rect(0, 0, view_getrect() . width, view_getrect() . height));
		
		// MW-2011-09-24: [[ Effects ]] Get the current snapshot as a UIImage
		// MW-2011-11-18: [[ Bug ]] Make sure we grab a UIImage copy of the snapshot
		//   here, as 'wait' can trigger an update if SetRedrawInterval has been used.
		ctxt . snapshot = m_snapshot;
		
		MCIPhoneRunOnMainFiber(effectrect_phase_1, &ctxt);
		
		// MW-2012-09-26: [[ Bug ]] Make sure we lock the screen around the effects.
		MCRedrawLockScreen();
		
		// MW-2011-10-17: [[ Bug 9811 ]] In the case of moving between stacks with
		//   effects, something is pending on the iOS side which must be dealt with
		//   otherwise no effect happens.
		MCscreen->wait(0.0, False, True);
		
		// MW-2011-09-24: [[ Effects ]] Take another snapshot, and get as a UIImage.
		snapshotwindow(t_effect_area);
		ctxt . snapshot = m_snapshot;
		MCIPhoneRunOnMainFiber(effectrect_phase_2, &ctxt);
		
		if (ctxt . effect->type == VE_CURL || ctxt . effect->type == VE_FLIP)
			MCscreen->wait(0.0, False, True);

		MCIPhoneRunOnMainFiber(effectrect_phase_3, &ctxt);

		while (![ctxt . effect_delegate finished])
			MCscreen->wait(ctxt . duration, False, True);

		MCIPhoneRunOnMainFiber(effectrect_phase_4, &ctxt);
		
		// MW-2012-09-26: [[ Bug ]] Make sure we lock the screen around the effects.
		MCRedrawUnlockScreen();
	}
	
	while(t_effects != nil)
	{
		MCEffectList *t_effect;
		t_effect = t_effects;
		t_effects = t_effects -> next;
		delete t_effect;
	}
	
	dirtyrect(p_rect);
	
	// MW-2012-07-19: [[ Bug ]] Make sure we force an update of the screen so that
	//   everything is in sync.
	MCRedrawUpdateScreen();
	
	// MW-2012-07-19: [[ Bug ]] Enable the redraw interval feature temporarily.
	[MCIPhoneGetApplication() enableRedrawInterval];
	if (!t_updates_were_enabled)
		MCRedrawDisableScreenUpdates();
}

void MCStack::updatetilecache(void)
{
	if (!view_getacceleratedrendering())
		return;

	MCIPhoneRunBlockOnMainFiber(^(void) {
		view_updatetilecache();
	});
}

// MW-2013-06-26: [[ Bug 10990 ]] For iOS, we must run the MCTileCacheDeactivate
//   method on the system thread.
void MCStack::deactivatetilecache(void)
{
	if (!view_getacceleratedrendering())
		return;
	
	MCIPhoneRunBlockOnMainFiber(^(void) {
		view_deactivatetilecache();
	});
}

bool MCStack::snapshottilecache(MCRectangle p_area, MCGImageRef& r_image)
{
	if (!view_getacceleratedrendering())
		return false;
	
	__block bool t_result;
    __block MCGImageRef *t_image = &r_image;
	MCIPhoneRunBlockOnMainFiber(^(void) {
		t_result = view_snapshottilecache(p_area, *t_image);
	});
	
	return t_result;
}
