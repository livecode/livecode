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


#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "field.h" 
#include "notify.h"
#include "statemnt.h"
#include "funcs.h"
#include "eventqueue.h"
#include "image.h"
#include "osspec.h"
#include "fiber.h"
#include "redraw.h"
#include "param.h"
#include "mbldc.h"

#include "font.h"

#include <objc/runtime.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <AudioToolbox/AudioToolbox.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <MediaPlayer/MPMoviePlayerViewController.h>

#include "mbliphoneapp.h"
#include "mbliphoneview.h"

#include "resolution.h"

#include <objc/message.h>

////////////////////////////////////////////////////////////////////////////////

extern void X_main_loop(void);
extern void send_startup_message(bool p_do_relaunch = true);
extern void setup_simulator_hooks(void);

@class com_runrev_livecode_MCIPhoneBreakWaitHelper;

extern CGBitmapInfo MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha);
extern bool MCImageGetCGColorSpace(CGColorSpaceRef &r_colorspace);

// SN-2015-02-16: [[ iOS Font mapping ]] We want to clean the generated font map
//   when closing the engine.
extern void ios_clear_font_mapping(void);

////////////////////////////////////////////////////////////////////////////////

Boolean tripleclick = False;
uint4 g_current_background_colour = 0;

////////////////////////////////////////////////////////////////////////////////

// These are used by the MCScreenDC 'beep' methods.
static SystemSoundID s_system_sound = 0;
static MCStringRef s_system_sound_name = nil;

// These control the mapping of LiveCode pixel values to iOS pixels.
static MCGFloat s_iphone_device_scale = 1;
// IM-2014-01-30: [[ HiDPI ]] Controls scaling of LiveCode coords to iOS logical coords
//   when dealing with native controls
static bool s_iphone_scale_controls = true;

// The main fiber on which all other code is executed.
static MCFiberRef s_main_fiber = nil;
// The fiber on which script and code calling script is executed.
static MCFiberRef s_script_fiber = nil;

// If this is true, then a request to break the current wait has already been
// made.
static bool s_break_wait_pending = false;
// The helper object used to break the main wait loop.
static com_runrev_livecode_MCIPhoneBreakWaitHelper *s_break_wait_helper = nil;
// The current depth of wait.
static uindex_t s_wait_depth = 0;

// The current height of the keyboard.
static float s_current_keyboard_height = 0.0f;

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-30: [[ HiDPI ]] The default value for pixel scaling on iOS is the main display density
MCGFloat MCResPlatformGetDefaultPixelScale()
{
	return s_iphone_device_scale;
}

// IM-2013-07-18: [[ ResIndependence ]] if using the device resolution
// then stack size == screen size, otherwise scale
float MCIPhoneGetResolutionScale(void)
{
	// IM-2014-01-30: [[ HiDPI ]] Use device resolution if pixel scaling is enabled
	return MCResGetUsePixelScaling() ? s_iphone_device_scale : 1.0;
}

// IM-2013-07-18: [[ ResIndependence ]] return the device scale
float MCIPhoneGetDeviceScale()
{
	return s_iphone_device_scale;
}

// IM-2014-03-14: [[ HiDPI ]] UI scale is resolution scale on iOS
MCGFloat MCResPlatformGetUIDeviceScale(void)
{
	return MCIPhoneGetResolutionScale();
}

////////////////////////////////////////////////////////////////////////////////

static bool MCIPhoneWait(double sleep);

static float iphone_font_measure_text(void *p_font, const char *p_text, uint32_t p_text_length, bool p_is_unicode);
static void iphone_font_draw_text(void *p_font, CGContextRef p_context, CGFloat x, CGFloat y, const char *p_text, uint32_t p_text_length, bool p_is_unicode);


////////////////////////////////////////////////////////////////////////////////

static inline MCRectangle MCRectangleFromCGRect(const CGRect &p_rect)
{
	return MCRectangleMake(p_rect.origin.x, p_rect.origin.y, p_rect.size.width, p_rect.size.height);
}

static inline MCGRectangle MCGRectangleFromCGRect(CGRect p_rect)
{
	return MCGRectangleMake(p_rect.origin.x, p_rect.origin.y, p_rect.size.width, p_rect.size.height);
}

static inline CGRect MCGRectangleToCGRect(MCGRectangle p_rect)
{
	return CGRectMake(p_rect.origin.x, p_rect.origin.y, p_rect.size.width, p_rect.size.height);
}

// IM-2013-07-18: [[ ResIndependence ]] rename these functions to more accurately describe their new purpose
MCRectangle MCDeviceRectFromLogicalCGRect(const CGRect p_cg_rect)
{
	return MCGRectangleGetIntegerBounds(MCGRectangleScale(MCGRectangleFromCGRect(p_cg_rect), s_iphone_device_scale));
}

CGRect MCUserRectToLogicalCGRect(const MCRectangle p_rect)
{
	// IM-2014-01-30: [[ HiDPI ]] Use logical->screen scale for conversion
	return MCGRectangleToCGRect(MCGRectangleScale(MCRectangleToMCGRectangle(p_rect), MCScreenDC::logicaltoscreenscale()));
}

// MW-2012-08-06: [[ Fibers ]] Primitive calls for executing selectors on
//   different fibers.

struct sel_ctxt_t
{
	id object;
	SEL selector;
	id argument;
};

static void sel_callback(void *p_context)
{
	sel_ctxt_t *ctxt;
	ctxt = (sel_ctxt_t *)p_context;
	[ctxt -> object performSelector: ctxt -> selector];
}

static void sel_callback_arg(void *p_context)
{
	sel_ctxt_t *ctxt;
	ctxt = (sel_ctxt_t *)p_context;
	[ctxt -> object performSelector: ctxt -> selector withObject: ctxt -> argument];
}

static void MCFiberCallSelector(MCFiberRef p_fiber, id object, SEL selector)
{
	sel_ctxt_t ctxt;
	ctxt . object = object;
	ctxt . selector = selector;
	MCFiberCall(p_fiber, sel_callback, &ctxt);
}

static void MCFiberCallSelectorWithObject(MCFiberRef p_fiber, id object, SEL selector, id arg)
{
	sel_ctxt_t ctxt;
	ctxt . object = object;
	ctxt . selector = selector;
	ctxt . argument = arg;
	MCFiberCall(p_fiber, sel_callback_arg, &ctxt);
}

void MCIPhoneCallOnMainFiber(void (*handler)(void *), void *context)
{
	MCFiberCall(s_main_fiber, handler, context);
}

bool MCIPhoneIsOnMainFiber(void)
{
    return MCFiberIsCurrentThread(s_main_fiber);
}

bool MCIPhoneIsOnScriptFiber(void)
{
    return MCFiberIsCurrentThread(s_script_fiber);
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCScreenDC::open(void)
{
	common_open();
	
	return True;
}

Boolean MCScreenDC::close(Boolean p_force)
{
    // SN-2015-02-16: [[ iOS Font Name ]] Clear the font mapping array.
    ios_clear_font_mapping();
	return True;
}

bool MCScreenDC::hasfeature(MCPlatformFeature p_feature)
{
	return false;
}

MCNameRef MCScreenDC::getdisplayname(void)
{
	return MCN_iphone;
}

MCNameRef MCScreenDC::getvendorname(void)
{
	return MCN_iphone;
}

uint2 MCScreenDC::device_getwidth()
{
	return 320 * MCIPhoneGetResolutionScale();
}

uint2 MCScreenDC::device_getheight()
{
	return 480 * MCIPhoneGetResolutionScale();
}

uint2 MCScreenDC::getwidthmm()
{
	return 32;
}

uint2 MCScreenDC::getheightmm()
{
	return 48;
}

uint2 MCScreenDC::getmaxpoints()
{
	return 4096;
}

uint2 MCScreenDC::getvclass()
{
	return TrueColor;
}

uint2 MCScreenDC::getdepth()
{
	return 32;
}

uint2 MCScreenDC::getrealdepth(void)
{
	return 32;
}

uint2 MCScreenDC::getpad()
{
	return 32;
}

Window MCScreenDC::getroot()
{
	return NULL;
}

// IM-2014-01-30: [[ HiDPI ]] Refactor to return in logical coordinates, with addition of screen pixel scale
bool MCScreenDC::platform_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count)
{
	bool t_success;
	t_success = true;
	
	MCDisplay *t_displays;
	t_displays = nil;
	
	uint32_t t_display_count;
	t_display_count = 0;
	
	t_success = MCMemoryNewArray(1, t_displays);
	
	if (t_success)
	{
		t_display_count = 1;
		
		t_displays[0].index = 0;
		t_displays[0].pixel_scale = MCIPhoneGetDeviceScale();
		
		t_displays[0].viewport = MCRectangleFromCGRect(MCIPhoneGetScreenBounds());
		t_displays[0].workarea = MCRectangleFromCGRect(MCIPhoneGetViewBounds());
		
		if (p_effective)
			t_displays[0].workarea.height -= s_current_keyboard_height;
		
		t_displays[0].viewport = screentologicalrect(t_displays[0].viewport);
		t_displays[0].workarea = screentologicalrect(t_displays[0].workarea);
	}
	
	if (t_success)
	{
		r_displays = t_displays;
		r_count = t_display_count;
	}
	else
		MCMemoryDeleteArray(t_displays);
	
	return t_success;
}

// IM-2014-01-29: [[ HiDPI ]] We receive notification of screen changes on iOS, so can safely cache display info
bool MCScreenDC::platform_displayinfocacheable(void)
{
	return true;
}

bool MCScreenDC::platform_getwindowgeometry(Window p_window, MCRectangle& r_rect)
{
	// IM-2014-01-30: [[ HiDPI ]] On iOS, the window rect will always be the bounds of the view
	r_rect = screentologicalrect(MCRectangleFromCGRect(MCIPhoneGetViewBounds()));
	
	return true;
}

void *MCScreenDC::GetNativeWindowHandle(Window p_window)
{
	if (p_window == nil)
		return nil;
	
	return MCIPhoneGetView();
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::beep(void)
{
	// MW-2012-08-06: [[ Fibers ]] Execute the system code on the main fiber.
	MCIPhoneRunBlockOnMainFiber(^(void) {
    
    // PM-2015-03-12: [[ Bug 14408 ]] On devices that don't support vibration, use AudioServicesPlaySystemSound, which does not lower the background volume, if audio is playing on the background
    if([[UIDevice currentDevice].model isEqualToString:@"iPhone"])
		AudioServicesPlayAlertSound(s_system_sound_name != nil ? s_system_sound : kSystemSoundID_Vibrate);
    else
        AudioServicesPlaySystemSound(s_system_sound_name != nil ? s_system_sound : kSystemSoundID_Vibrate);
	});
}

struct MCScreenDCDoSetBeepSoundEnv
{
	MCStringRef sound;
	bool result;
};

// MW-2012-08-06: [[ Fibers ]] Main fiber callback for system calls.
static void MCScreenDCDoSetBeepSound(void *p_env)
{
	MCScreenDCDoSetBeepSoundEnv *env;
	env = (MCScreenDCDoSetBeepSoundEnv *)p_env;
	
	if (env -> sound == nil || MCStringIsEmpty(env -> sound))
	{
		if (s_system_sound_name != nil)
		{
			AudioServicesDisposeSystemSoundID(s_system_sound);
			MCValueRelease(s_system_sound_name);
		}
		s_system_sound = 0;
		s_system_sound_name = nil;
		env -> result = true;
		return;
	}
	
	SystemSoundID t_new_sound;
	
	MCAutoStringRef t_sound_path;
	MCS_resolvepath(env -> sound, &t_sound_path);
	
	NSURL *t_url;
	t_url = [NSURL fileURLWithPath: MCStringConvertToAutoreleasedNSString(*t_sound_path)];
	
	OSStatus t_status;
	t_status = AudioServicesCreateSystemSoundID((CFURLRef)t_url, &t_new_sound);
	if (t_status == noErr)
	{
		if (s_system_sound_name != nil)
		{
			AudioServicesDisposeSystemSoundID(s_system_sound);
			MCValueRelease(s_system_sound_name);
		}
		s_system_sound = t_new_sound;
		s_system_sound_name = MCValueRetain(*t_sound_path);
	}
	
	env -> result = t_status == noErr;
}

bool MCScreenDC::setbeepsound(MCStringRef p_beep_sound)
{
	MCScreenDCDoSetBeepSoundEnv t_env;
	t_env . sound = p_beep_sound;

	// MW-2012-08-06: [[ Fibers ]] Execute the system code on the main fiber.
	/* REMOTE */ MCFiberCall(s_main_fiber, MCScreenDCDoSetBeepSound, &t_env);

	return t_env . result;
}

bool MCScreenDC::getbeepsound(MCStringRef& r_beep_sound)
{
	if (s_system_sound_name != nil)
		r_beep_sound = MCValueRetain(s_system_sound_name);
	else
		r_beep_sound = MCValueRetain(kMCEmptyString);
	return true;
}

void MCScreenDC::getbeep(uint4 property, int4& r_value)
{
}

void MCScreenDC::setbeep(uint4 property, int4 beep)
{
}

////////////////////////////////////////////////////////////////////////////////

struct MCScreenDCDoSnapshotEnv
{
	MCRectangle r;
	MCPoint size;
	uint4 window;
	MCStringRef displayname;
	MCImageBitmap *result;
};

// MW-2012-08-06: [[ Fibers ]] Main fiber callback for system calls.
// MW-2014-02-20: [[ Bug 11811 ]] Updated to scale snapshot to requested size.
static void MCScreenDCDoSnapshot(void *p_env)
{
	MCScreenDCDoSnapshotEnv *env;
	env = (MCScreenDCDoSnapshotEnv *)p_env;
	
	MCRectangle r;
	uint4 window;
	r = env -> r;
	window = env -> window;
	
	/////
	
	bool t_success = true;
	
	MCImageBitmap *t_bitmap = NULL;
	
	// Use the screenRect to clip the input rect
	MCRectangle t_screen_rect;
	const MCDisplay *t_displays;
	MCscreen -> getdisplays(t_displays, false);
	t_screen_rect = t_displays[0] . viewport;
	r = MCU_clip_rect(env -> r, t_screen_rect . x, t_screen_rect . y, t_screen_rect . width, t_screen_rect . height);
	
	uint32_t t_bitmap_width, t_bitmap_height;
	t_bitmap_width = env -> size . x;
	t_bitmap_height = env -> size . y;
	
	if (r.width != 0 && r.height != 0)
	{
		CGContextRef t_img_context = nil;
		CGColorSpaceRef t_colorspace = nil;
		uint8_t *t_pixel_buffer = nil;
		
		if (t_success)
			t_success = MCImageGetCGColorSpace(t_colorspace);
		
		if (t_success)
			t_success = MCImageBitmapCreate(t_bitmap_width, t_bitmap_height, t_bitmap);
		
		if (t_success)
		{
			MCImageBitmapClear(t_bitmap);
			// IM-2013-08-21: [[ RefactorGraphics ]] Refactor CGImage creation code to be pixel-format independent
			CGBitmapInfo t_bm_info;
			t_bm_info = MCGPixelFormatToCGBitmapInfo(kMCGPixelFormatNative, false);
			t_img_context = CGBitmapContextCreate(t_bitmap -> data, t_bitmap->width, t_bitmap->height, 8, t_bitmap->stride, t_colorspace, t_bm_info);
			t_success = t_img_context != nil;
		}
		
		if (t_success)
		{
			CGContextScaleCTM(t_img_context, 1.0, -1.0);
			CGContextTranslateCTM(t_img_context, 0, -(CGFloat)t_bitmap_height);
			
			// IM-2014-10-24: [[ Bug 13350 ]] Scale to the target bitmap size before further transformation.
			CGContextScaleCTM(t_img_context, (MCGFloat)t_bitmap_width / r . width , (MCGFloat)t_bitmap_height / r . height);

            // MW-2014-04-22: [[ Bug 12008 ]] Translate before scale.
			CGContextTranslateCTM(t_img_context, -(CGFloat)r.x, -(CGFloat)r.y);
            
            // MW-2014-04-22: [[ Bug 12008 ]] Make sure we take into account the logical to device screen
            //   scale (and invert appropriately!).
            float t_scale;
            t_scale = ((MCScreenDC *)MCscreen) -> logicaltoscreenscale();
			CGContextScaleCTM(t_img_context, 1.0 / t_scale, 1.0 / t_scale);
			
			bool t_is_rotated;
			t_is_rotated = UIInterfaceOrientationIsLandscape(MCIPhoneGetOrientation());
			
			if (MCmajorosversion >= 800)
			{
				// PM-2014-10-09: [[ Bug 13634 ]] No need to rotate the coords on iOS 8
				// IM-2014-11-05: [[ Bug 13949 ]] Avoid rotating entirely as faulty offset was being applied to snapshots in landscape orientation.
			}
			else
			{
				CGSize t_offset;
				CGFloat t_angle;
				
				switch(MCIPhoneGetOrientation())
				{
					case UIInterfaceOrientationPortrait:
						t_angle = 0.0;
						t_offset = CGSizeMake(t_screen_rect . width / 2, t_screen_rect . height / 2);
						break;
					case UIInterfaceOrientationPortraitUpsideDown:
						t_angle = M_PI;
						t_offset = CGSizeMake(t_screen_rect . width / 2, t_screen_rect . height / 2);
						break;
					case UIInterfaceOrientationLandscapeLeft:
						// MW-2011-10-17: [[ Bug 9816 ]] Angle caused upside-down image so inverted.
						t_angle = M_PI / 2;
						t_offset = CGSizeMake(t_screen_rect . height / 2, t_screen_rect . width / 2);
						break;
					case UIInterfaceOrientationLandscapeRight:
						// MW-2011-10-17: [[ Bug 9816 ]] Angle caused upside-down image so inverted.
						t_angle = -M_PI / 2;
						t_offset = CGSizeMake(t_screen_rect . height / 2, t_screen_rect . width / 2);
						break;
				}
				
				CGContextTranslateCTM(t_img_context, t_screen_rect . width / 2, t_screen_rect . height / 2);
				CGContextRotateCTM(t_img_context, t_angle);
				CGContextTranslateCTM(t_img_context, -t_offset . width, -t_offset . height);
			}
			
#ifndef USE_UNDOCUMENTED_METHODS
			NSArray *t_windows;
			t_windows = [[[UIApplication sharedApplication] windows] retain];
#else
			NSMutableArray *t_windows;
			t_windows = [[[UIApplication sharedApplication] windows] mutableCopy];
			if (![[UIApplication sharedApplication] isStatusBarHidden])
			{
				CGFloat t_statusbar_size;
				CGRect t_statusbar_frame;
				t_statusbar_frame = [[UIApplication sharedApplication] statusBarFrame];
				if (t_is_rotated)
					t_statusbar_size = t_statusbar_frame . size . width;
				else
					t_statusbar_size = t_statusbar_frame . size . height;
				
				MCRectangle t_statusbar_rect;
				MCU_set_rect(t_statusbar_rect, 0, 0, t_screen_rect . width, t_statusbar_size * t_scale);
				if (!MCU_empty_rect(MCU_intersect_rect(t_statusbar_rect, r)))
				{
					id t_statusbar_window;
					if (object_getInstanceVariable([UIApplication sharedApplication], "_statusBarWindow", (void **)&t_statusbar_window) != nil)
						[t_windows addObject: t_statusbar_window];
				}
			}
#endif
			
			for (UIWindow *window in t_windows) 
			{
				if ([window screen] == [UIScreen mainScreen])
				{
					// -renderInContext: renders in the coordinate space of the layer,
					// so we must first apply the layer's geometry to the graphics context
					CGContextSaveGState(t_img_context);
					// Center the context around the window's anchor point
					CGContextTranslateCTM(t_img_context, [window center].x, [window center].y);
					// Apply the window's transform about the anchor point
					CGContextConcatCTM(t_img_context, [window transform]);
					// Offset by the portion of the bounds left of and above the anchor point
					CGContextTranslateCTM(t_img_context,
										  -[window bounds].size.width * [[window layer] anchorPoint].x,
										  -[window bounds].size.height * [[window layer] anchorPoint].y);
					
					// Render the layer hierarchy to the current context
                    if ([window respondsToSelector: @selector(drawViewHierarchyInRect:afterScreenUpdates:)])
                    {
                        // This method is supported in iOS7+ and will capture many
                        // native view's content.
                        UIGraphicsPushContext(t_img_context);
                        [window drawViewHierarchyInRect:[window bounds] afterScreenUpdates:YES];
                        UIGraphicsPopContext();
                    }
                    else
                    {
                        [[window layer] renderInContext:t_img_context];
					}
                    
					// Restore the context
					CGContextRestoreGState(t_img_context);
				}
			}
			
			[t_windows release];
		}
		
		if (t_img_context)
			CGContextRelease(t_img_context);
		
		if (!t_success)
		{
			if (t_bitmap != NULL)
			{
				MCImageFreeBitmap(t_bitmap);
				t_bitmap = NULL;
			}
		}
	}
		
	env -> result = t_bitmap;
}

MCImageBitmap *MCScreenDC::snapshot(MCRectangle &r, uint4 window, MCStringRef displayname, MCPoint *size)
{
	MCScreenDCDoSnapshotEnv env;
	env . r = r;
	env . window = window;
	env . displayname = displayname == nil ? nil : MCValueRetain(displayname);
    
    // MW-2014-02-20: [[ Bug 11811 ]] Pass through the specified size, or the size of the rect.
	env . size = size != nil ? *size : MCPointMake(r . width, r . height);

	// MW-2012-08-06: [[ Fibers ]] Execute the system code on the main fiber.
	/* REMOTE */ MCFiberCall(s_main_fiber, MCScreenDCDoSnapshot, &env);

    if (env . displayname != nil)
        MCValueRelease(env . displayname);
	return env . result;
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCScreenDC::wait(real8 duration, Boolean dispatch, Boolean anyevent)
{
    MCDeletedObjectsEnterWait(dispatch);
    
	real8 curtime = MCS_time();
	
	if (duration < 0.0)
		duration = 0.0;
	
	real8 exittime = curtime + duration;
	
	Boolean abort = False;
	Boolean reset = False;
	Boolean done = False;
	
	MCwaitdepth++;
	
	do
	{
		// IM-2014-03-06: [[ revBrowserCEF ]] Call additional runloop callbacks
		DoRunloopActions();
		
		// MW-2013-08-18: [[ XPlatNotify ]] Handle any pending notifications
		if (MCNotifyDispatch(dispatch == True))
        {
            if (anyevent)
                break;
        }
		
		real8 eventtime = exittime;
		if (handlepending(curtime, eventtime, dispatch))
		{
			if (anyevent)
				done = True;
			
			if (MCquit)
			{
				abort = True;
				break;
			}
		}
		
		if (dispatch && MCEventQueueDispatch())
		{
			if (anyevent)
				done = True;
			
			if (MCquit)
			{
				abort = True;
				break;
			}
		}
		
		// MW-2012-09-19: [[ Bug 10218 ]] Make sure we update the screen in case
		//   any engine event handling methods need us to.
		MCRedrawUpdateScreen();

		// Get the time now
		curtime = MCS_time();
		
		// And work out how long to sleep for.
		real8 t_sleep;
		t_sleep = 0.0;
		if (curtime >= exittime)
			done = True;
		else if (!done && eventtime > curtime)
			t_sleep = MCMin(eventtime - curtime, exittime - curtime);

		// Switch to the main fiber and wait for at most t_sleep seconds. This
		// returns 'true' if the wait was broken rather than timed out.
		if (MCIPhoneWait(t_sleep) && anyevent)
			done = True;
		
		// If 'quit' has been set then we must have got a finalization request
		if (MCquit)
		{
			abort = True;
			break;
		}
	}
	while(!done);
	
	MCwaitdepth--;
	
	// MW-2012-09-19: [[ Bug 10218 ]] Make sure we update the screen in case
	//   any engine event handling methods need us to.
	MCRedrawUpdateScreen();
    
    MCDeletedObjectsLeaveWait(dispatch);
	
    return abort;
}

// MW-2011-08-16: [[ Wait ]] Break the OS event loop, causing a switch back to
//   the engine fiber, with control resuming within 'wait()'.
void MCScreenDC::pingwait(void)
{
	MCIPhoneBreakWait();
}

void MCScreenDC::openIME()
{
}

void MCScreenDC::activateIME(Boolean activate)
{
	// MW-2012-08-06: [[ Fibers ]] Execute the system code on the main fiber.
	MCIPhoneRunBlockOnMainFiber(^(void) {
	if (activate)
			MCIPhoneActivateKeyboard();
	else
			MCIPhoneDeactivateKeyboard();
	});
}

void MCScreenDC::closeIME()
{
	// MW-2012-08-06: [[ Fibers ]] Execute the system code on the main fiber.
	MCIPhoneRunBlockOnMainFiber(^(void) {
		MCIPhoneDeactivateKeyboard();
	});
}

void MCScreenDC::do_take_focus(void)
{
	// MW-2012-08-06: [[ Fibers ]] Execute the system code on the main fiber.
	MCFiberCallSelector(s_main_fiber, MCIPhoneGetView(), @selector(becomeFirstResponder));
}

void MCScreenDC::do_fit_window(bool p_immediate_resize, bool p_post_message)
{
	if (m_current_window == nil)
		return;
	
	MCRectangle t_view_bounds;
	t_view_bounds = MCDeviceRectFromLogicalCGRect(MCIPhoneGetViewBounds());
	
	// IM-2014-03-03: [[ Bug 11836 ]] Store window topleft in logical coords
	MCPoint t_topleft;
	t_topleft = MCPointMake(t_view_bounds.x, t_view_bounds.y);

	t_topleft = screentologicalpoint(t_topleft);

	m_window_left = t_topleft . x;
	m_window_top = t_topleft . y;
	
	if (p_post_message)
	{
		if (p_immediate_resize)
		{
			// IM-2014-01-30: [[ HiDPI ]] Ensure stack backing scale is set
			((MCStack *)m_current_window) -> view_setbackingscale(MCResGetPixelScale());
			((MCStack *)m_current_window) -> view_configure(true);
		}
		else
		{
			// IM-2014-02-14: [[ HiDPI ]] Post backing scale changes with window reshape message
			MCEventQueuePostWindowReshape((MCStack *)m_current_window, MCResGetPixelScale());
		}
	}
}

// MW-2012-03-05: [[ ViewStack]] Change the currently displayed stack to a new one.
void MCScreenDC::do_show_stack_in_window(MCStack *p_stack)
{
	[MCIPhoneGetRootView() setCurrentStack: p_stack];
}

Window MCScreenDC::get_current_window(void)
{
	return m_current_window;
}

void MCScreenDC::refresh_current_window(void)
{
    
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::getsystemappearance(MCSystemAppearance &r_appearance)
{
	/* userInterfaceStyle was introduced in iOS 12.0 */
	typedef int (*_userInterfaceStyle)(UITraitCollection *, SEL selector);
	UITraitCollection *t_traits = [MCIPhoneGetRootView() traitCollection];
	if ([t_traits respondsToSelector: @selector(userInterfaceStyle)] &&
			((_userInterfaceStyle)objc_msgSend)(t_traits, @selector(userInterfaceStyle)) == 2)
	{
		r_appearance = kMCSystemAppearanceDark;
	}
	else
	{
		r_appearance = kMCSystemAppearanceLight;
	}
}

////////////////////////////////////////////////////////////////////////////////

extern void *coretext_font_create_with_name_size_and_style(MCStringRef p_name, uint32_t p_size, bool p_bold, bool p_italic);
extern bool coretext_font_destroy(void *p_font);
extern bool coretext_font_get_metrics(void *p_font, float& r_ascent, float& r_descent, float& r_leading, float& r_xheight);

struct do_iphone_font_create_env
{
	MCStringRef name;
	uint32_t size;
	bool bold;
	bool italic;
	void *result;
};

// MW-2012-08-06: [[ Fibers ]] Main fiber callback for system calls.
static void do_iphone_font_create(void *p_env)
{
	do_iphone_font_create_env *env;
	env = (do_iphone_font_create_env *)p_env;
    
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
	env -> result = coretext_font_create_with_name_size_and_style(env -> name, env -> size, env -> bold, env -> italic);
}

static void do_iphone_font_destroy(void *p_font)
{
    coretext_font_destroy(p_font);
}

void *iphone_font_create(MCStringRef p_name, uint32_t p_size, bool p_bold, bool p_italic)
{
	do_iphone_font_create_env env;
	env . name = MCValueRetain(p_name);
	env . size = p_size;
	env . bold = p_bold;
	env . italic = p_italic;
	// MW-2012-08-06: [[ Fibers ]] Execute the system code on the main fiber.
	/* REMOTE */ MCFiberCall(s_main_fiber, do_iphone_font_create, &env);
    MCValueRelease(env . name);
	return env . result;
}

void iphone_font_get_metrics(void *p_font, float& r_ascent, float& r_descent, float& r_leading, float& r_xheight)
{
    coretext_font_get_metrics(p_font, r_ascent, r_descent, r_leading, r_xheight);
}

void iphone_font_destroy(void *p_font)
{
	// MW-2012-08-06: [[ Fibers ]] Execute the system code on the main fiber.
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
	/* REMOTE */ MCFiberCall(s_main_fiber, do_iphone_font_destroy, p_font);
}

////////////////////////////////////////////////////////////////////////////////

bool iphone_run_on_main_thread(void *p_callback, void *p_callback_state, int p_options);

typedef void (*MCExternalThreadOptionalCallback)(void *state);
typedef void (*MCExternalThreadRequiredCallback)(void *state, int flags);
enum
{
	// Post the callback and wait until the callback is invoked
	kMCExternalRunOnMainThreadSend = 0 << 0,
	// Post the callback and return immediately
	kMCExternalRunOnMainThreadPost = 1 << 0,
	// The callback does not have to be executed
	kMCExternalRunOnMainThreadOptional = 0 << 1,
	// The callback has to be executed (changes signature)
	kMCExternalRunOnMainThreadRequired = 1 << 1,
	// The callback should be invoked in a script-safe environment
	kMCExternalRunOnMainThreadSafe = 0 << 2,
	// The callback should can be invoked in a non-script-safe environment
	kMCExternalRunOnMainThreadUnsafe = 1 << 2,
	// The callback should be invoked as soon as possible
	kMCExternalRunOnMainThreadImmediate = 0 << 3,
	// The callback should be invoked synchronized to the event queue
	kMCExternalRunOnMainThreadDeferred = 1 << 3,
	// Call the callback on the UI thread.
	kMCExternalRunOnMainThreadJumpToUI = 1 << 4,
	// Call the callback on the Engine thread.
	kMCExternalRunOnMainThreadJumpToEngine = 2 << 4,
};

@interface com_runrev_livecode_MCRunOnMainThreadHelper : NSObject
{
	void *m_callback;
	void *m_callback_state;
	int m_options;
}

- (id)initWithCallback:(void*)callback state:(void *)callbackState options:(int)options;
- (void)run;

@end

@implementation com_runrev_livecode_MCRunOnMainThreadHelper

- (id)initWithCallback:(void*)p_callback state:(void *)p_callback_state options:(int)p_options
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_callback = p_callback;
	m_callback_state = p_callback_state;
	m_options = p_options;
	
	return self;
}

- (void)perform
{
	if ((m_options & kMCExternalRunOnMainThreadRequired) != 0)
		((MCExternalThreadRequiredCallback)m_callback)(m_callback_state, 0);
	else
		((MCExternalThreadOptionalCallback)m_callback)(m_callback_state);
}

- (void)run
{
	iphone_run_on_main_thread(m_callback, m_callback_state, m_options);
	[self release];
}

@end

class MCRunOnMainThreadEvent: public MCCustomEvent
{
public:
	MCRunOnMainThreadEvent(void *p_callback, void *p_callback_context, int p_options)
	{
		m_callback = p_callback;
		m_callback_context = p_callback_context;
		m_options = p_options;
		m_dispatched = false;
	}
	
	void Destroy(void)
	{
		if (!m_dispatched && (m_options & kMCExternalRunOnMainThreadRequired) != 0)
			((MCExternalThreadRequiredCallback)m_callback)(m_callback_context, 1);
		delete this;
	}
	
	void Dispatch(void)
	{
		m_dispatched = true;
		
		if ((m_options & kMCExternalRunOnMainThreadRequired) != 0)
			((MCExternalThreadRequiredCallback)m_callback)(m_callback_context, 0);
		else
			((MCExternalThreadOptionalCallback)m_callback)(m_callback_context);
	}
	
private:
	void *m_callback;
	void *m_callback_context;
	int m_options;
	bool m_dispatched;
};

// MW-2012-08-06: [[ Fibers ]] Updated implementation to understand 'jumps'
bool iphone_run_on_main_thread(void *p_callback, void *p_callback_state, int p_options)
{
	// Handle the jump to one or other fiber.
	if ((p_options & kMCExternalRunOnMainThreadJumpToUI) != 0)
	{
		if ((p_options & ~kMCExternalRunOnMainThreadJumpToUI) != 0)
			return false;
			
		MCFiberCall(s_main_fiber, (MCFiberCallback)p_callback, p_callback_state);
		
		return true;
	}
	else if ((p_options & kMCExternalRunOnMainThreadJumpToEngine) != 0)
	{
		if ((p_options & ~kMCExternalRunOnMainThreadJumpToEngine) != 0)
			return false;
			
		MCFiberCall(s_script_fiber, (MCFiberCallback)p_callback, p_callback_state);
		
		return true;
	}

	// MW-2013-06-18: [[ XPlatNotify ]] Make sure we check whether either fiber is the
	//   current thread, rather than is the current fiber (otherwise calls from aux. threads
	//   don't work!).
	// If we aren't on one of the fibers, then post a selector to the main fiber's
	// thread.
	if (!MCFiberIsCurrentThread(s_script_fiber) && !MCFiberIsCurrentThread(s_main_fiber))
	{
		com_runrev_livecode_MCRunOnMainThreadHelper *t_helper;
		t_helper = [[com_runrev_livecode_MCRunOnMainThreadHelper alloc] initWithCallback: p_callback state: p_callback_state options: p_options];
		
		SEL t_selector;
		if ((p_options & (kMCExternalRunOnMainThreadDeferred | kMCExternalRunOnMainThreadUnsafe)) == (kMCExternalRunOnMainThreadUnsafe | kMCExternalRunOnMainThreadImmediate))
			t_selector = @selector(perform);
		else
			t_selector = @selector(run);
		
		[t_helper performSelectorOnMainThread: t_selector withObject: nil waitUntilDone: (p_options & kMCExternalRunOnMainThreadPost) == 0];
		
		return true;
	}
	
	// Unsafe and immediate -> queue and perform
	if ((p_options & (kMCExternalRunOnMainThreadDeferred | kMCExternalRunOnMainThreadUnsafe)) == (kMCExternalRunOnMainThreadUnsafe | kMCExternalRunOnMainThreadImmediate))
	{
		if ((p_options & kMCExternalRunOnMainThreadPost) == 0)
		{
			if ((p_options & kMCExternalRunOnMainThreadRequired) != 0)
				((MCExternalThreadRequiredCallback)p_callback)(p_callback_state, 0);
			else
				((MCExternalThreadOptionalCallback)p_callback)(p_callback_state);
			return true;
		}
		
		com_runrev_livecode_MCRunOnMainThreadHelper *t_helper;
		t_helper = [[com_runrev_livecode_MCRunOnMainThreadHelper alloc] initWithCallback: p_callback state: p_callback_state options: p_options];
		[t_helper performSelector: @selector(perform) withObject: nil afterDelay: 0];
		return true;
	}
	
	// Safe and immediate -> post to front of event queue
	// Unsafe/Safe and deferred -> post to back of event queue
	MCRunOnMainThreadEvent *t_event;
	t_event = new MCRunOnMainThreadEvent(p_callback, p_callback_state, p_options);
	if ((p_options & kMCExternalRunOnMainThreadDeferred) != 0)
		MCEventQueuePostCustom(t_event);
	else
		MCEventQueuePostCustomAtFront(t_event);
		
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCUIDC *MCCreateScreenDC(void)
{
	return new MCScreenDC;
}

////////////////////////////////////////////////////////////////////////////////

// MW-2013-03-20: [[ Bug 10748 ]] Make sure we only switch display class when
//   absolutely necessary - the request to switch is deferred until the next
//   update request.

static bool s_ensure_opengl = false;
static bool s_is_opengl_display = false;

void MCIPhoneSwitchToOpenGL(void)
{
	s_ensure_opengl = true;
}

void MCIPhoneSwitchToUIKit(void)
{
	s_ensure_opengl = false;
}

void MCIPhoneSyncDisplayClass(void)
{
	if (s_ensure_opengl && !s_is_opengl_display)
	{
		s_is_opengl_display = true;
		MCIPhoneRunBlockOnMainFiber(^(void) {
			MCIPhoneSwitchViewToOpenGL();
			// IM-2014-01-30: [[ HiDPI ]] GL backing surface should be at the current resolution scale
			MCIPhoneConfigureContentScale(MCIPhoneGetResolutionScale());
		});
	}
	else if (!s_ensure_opengl && s_is_opengl_display)
	{
		s_is_opengl_display = false;
		// MW-2012-08-06: [[ Fibers ]] Execute the system code on the main fiber.
		MCIPhoneRunBlockOnMainFiber(^(void) {
			MCIPhoneSwitchViewToUIKit();
		});
	}
}

////////////////////////////////////////////////////////////////////////////////

float MCIPhoneGetNativeControlScale(void)
{
	// IM-2014-01-30: [[ HiDPI ]] Scale is from screen -> logical so return inverted if scaling
	return s_iphone_scale_controls ? 1.0f / MCScreenDC::logicaltoscreenscale() : 1.0f;
}

// Only called from mobile extra calls so on main thread.
// IM-2014-01-30: [[ HiDPI ]] Legacy function reimplemented to use MCResSetUsePixelScale
void MCIPhoneUseDeviceResolution(bool p_use, bool p_controls_too)
{
	s_iphone_scale_controls = p_controls_too;
	
	MCResSetUsePixelScaling(p_use);
	
	// IM-2013-12-10: [[ Bug 11571 ]] Update the pixelScale to the new device scale.
	MCResSetPixelScale(p_use ? 1 : MCIPhoneGetDeviceScale());
}

// IM-2014-01-30: [[ HiDPI ]] Revise per-platform handling of changes to pixelscale settings
void MCResPlatformHandleScaleChange(void)
{
	// GL backing surface should be at the current resolution scale
	MCIPhoneConfigureContentScale(MCIPhoneGetResolutionScale());
	
	// Refresh display info after any change to the pixelscale
	bool t_changed;
	MCscreen->updatedisplayinfo(t_changed);
	
	// This doesn't do an immediate resize, so is fine for the main thread. (no
	// script called).
	static_cast<MCScreenDC *>(MCscreen) -> do_fit_window(false, true);
	
	// MW-2012-03-21: [[ Bug ]] Make sure the screen is dirtied as chances are
	//   scaling has changed.
	MCRedrawDirtyScreen();
}

////////////////////////////////////////////////////////////////////////////////

// MW-2012-08-06: [[ Fibers ]] Primitives for executing code on the main fiber.

void MCIPhoneCallSelectorOnMainFiber(id p_object, SEL p_selector)
{
	MCFiberCallSelector(s_main_fiber, p_object, p_selector);
}

void MCIPhoneCallSelectorOnMainFiberWithObject(id p_object, SEL p_selector, id p_argument)
{
	MCFiberCallSelectorWithObject(s_main_fiber, p_object, p_selector, p_argument);
}

void MCIPhoneRunOnMainFiber(void (*p_callback)(void *), void *p_context)
{
	MCFiberCall(s_main_fiber, p_callback, p_context);
}

void MCIPhoneRunOnScriptFiber(void (*p_callback)(void *), void *p_context)
{
    MCFiberCall(s_script_fiber, p_callback, p_context);
}

static void invoke_block(void *p_context)
{
	void (^t_block)(void) = (void (^)(void))p_context;
	t_block();
}

void MCIPhoneRunBlockOnMainFiber(void (^block)(void))
{
	MCFiberCall(s_main_fiber, invoke_block, block);
}

void MCIPhoneRunBlockOnScriptFiber(void (^block)(void))
{
	MCFiberCall(s_script_fiber, invoke_block, block);
}

////////////////////////////////////////////////////////////////////////////////

// MW-2012-08-06: [[ Fibers ]] Updated entry point for didBecomeActive.
static void MCIPhoneDoDidBecomeActive(void *)
{
	// MW-2011-08-11: [[ Bug 9671 ]] Make sure we initialize MCstackbottom.
	int i;
	MCstackbottom = (char *)&i;
	
	NSAutoreleasePool *t_pool;
    t_pool = [[NSAutoreleasePool alloc] init];
	
	// Convert the arguments into stringrefs
	MCStringRef args[1];
	MCStringCreateWithCFStringRef((CFStringRef)[[[NSProcessInfo processInfo] arguments] objectAtIndex: 0], args[0]);

    // SN-2015-09-22: [[ Bug 15987 ]] Use NSProcessInfo to get the env variables
	// Convert the environment variables into stringrefs
	uindex_t envc = 0;
    MCAutoStringRefArray t_envp;

    envc = (uindex_t)[[[NSProcessInfo processInfo] environment] count];

    // last elem of env must be a NULL element, so we allocated envc + 1 elems
    // If the allocation fails, *t_envp will return NULL, so we are fine.
    if (t_envp . New(envc + 1))
    {
        NSDictionary *t_environment;
        NSEnumerator* t_key_enumerator;
        NSString* t_value;

        // NSProcessInfo::environment returns NSDictionary<NSString*,NSString*>
        t_environment = [[NSProcessInfo processInfo] environment];

        t_key_enumerator = [t_environment keyEnumerator];
        index_t t_index = 0;

        // Loop through all the keys in the environment array
        for (id t_key in t_environment)
        {
            t_value = [t_environment objectForKey:t_key];

            MCAutoStringRef t_value_str, t_variable_str;
            MCStringRef t_env_var;

            // We convert the CFStringRef that we got from the dictionary into
            //  StringRefs, and create a Bash-like environment variable
            if (MCStringCreateWithCFStringRef((CFStringRef)t_value, &t_value_str)
                    && MCStringCreateWithCFStringRef((CFStringRef)t_key, &t_variable_str)
                    && MCStringFormat(t_env_var, "%@=%@", *t_variable_str, *t_value_str))
                t_envp[t_index] = t_env_var;

            t_index++;
        }

        // Ensure t_envp array is NULL-terminated
        t_envp[envc] = NULL;
    }
	
	// Initialize the engine.
    struct X_init_options t_options;
    t_options.argc = 1;
    t_options.argv = args;
    t_options.envp = *t_envp;
    t_options.app_code_path = nullptr;
    
	Bool t_init_success;
    t_init_success = X_init(t_options);
	
    [t_pool release];
	
	if (!t_init_success)
	{
		
		if (MCValueGetTypeCode(MCresult -> getvalueref()) == kMCValueTypeCodeString)
		{
			NSLog(@"Startup error: %s\n", MCStringGetCString((MCStringRef)MCresult -> getvalueref()));
			abort();
			return;
		}
	}

	// MW-2012-08-31: [[ Bug 10340 ]] Now we've finished initializing, get the app to
	//   start preparing.
	[MCIPhoneGetApplication() performSelectorOnMainThread:@selector(startPreparing) withObject:nil waitUntilDone:NO];
}

// MW-2012-08-06: [[ Fibers ]] Updated entry point that triggers before the main
//   runloop is entered.
static void MCIPhoneDoDidStartPreparing(void *)
{
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	send_startup_message(false);
	if (!MCquit)
		MCdispatcher ->  gethome() -> open();
	[t_pool release];
	
	// MW-2012-08-31: [[ Bug 10340 ]] Now we've finished preparing, get the app to
	//   start executing.
	[MCIPhoneGetApplication() performSelectorOnMainThread:@selector(startExecuting) withObject:nil waitUntilDone:NO];
}

// MW-2012-08-06: [[ Fibers ]] Updated entry point for execution of the main
//   run loop.
static void MCIPhoneDoDidStartExecuting(void *)
{
	// Now run the event loop
	for(;;)
	{
		NSAutoreleasePool *t_pool;
		t_pool = [[NSAutoreleasePool alloc] init];
		
		bool t_continue;
		t_continue = X_main_loop_iteration();
		
		[t_pool release];
		
		if (!t_continue)
			break;
	}
	
	// MW-2013-01-13: [[ Bug 10633 ]] Make sure the app exits on quit.
	// MW-2013-04-01: [[ Bug 10799 ]] Make sure we only exit if the quit was explicit.
	if (MCquitisexplicit)
		exit(0);
}

// MW-2012-08-06: [[ Fibers ]] Updated entry point for willTerminate.
static void MCIPhoneDoWillTerminate(void *)
{
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	// Ensure shutdown is called.
	if (MCdefaultstackptr)
		MCdefaultstackptr->getcard()->message(MCM_shut_down, (MCParameter*)NULL, True, True);
	
	// Shutdown the engine
	X_close();
	
	[t_pool release];
	
}

// MW-2012-08-06: [[ Fibers ]] Updated entry point for when main view resizes.
static void MCIPhoneDoViewBoundsChanged(void *)
{
	if (MCscreen == nil)
		return;
	
	// IM-2014-01-30: [[ HiDPI ]] Update display info when view bounds change
	bool t_changed;
	MCscreen->updatedisplayinfo(t_changed);
	
	static_cast<MCScreenDC *>(MCscreen) -> do_fit_window(true, true);
}

////////////////////////////////////////////////////////////////////////////////

// MW-2012-08-06: [[ Fibers ]] Main (system) fiber side handler for didBecomeActive.
void MCIPhoneHandleDidBecomeActive(void)
{
	// Convert the current thread to the main fiber (system owned).
	MCFiberConvert(s_main_fiber);

    // Create our auxiliary script fiber.
	MCFiberCreate(256 * 1024, s_script_fiber);
	
	// Transfer control to the engine fiber.
	MCFiberCall(s_script_fiber, MCIPhoneDoDidBecomeActive, nil);
}

// MW-2012-08-06: [[ Fibers ]] Main (system) fiber side handler for didStartPreparing.
void MCIPhoneHandleDidStartPreparing(void)
{
	// Transfer control to the engine fiber.
	MCFiberCall(s_script_fiber, MCIPhoneDoDidStartPreparing, nil);
}

// MW-2012-08-06: [[ Fibers ]] Main (system) fiber side handler for didStartExecuting.
void MCIPhoneHandleDidStartExecuting(void)
{
	// Transfer control to the engine fiber.
	MCFiberCall(s_script_fiber, MCIPhoneDoDidStartExecuting, nil);
}

// MW-2012-08-06: [[ Fibers ]] Main (system) fiber side handler for willTerminate.
void MCIPhoneHandleWillTerminate(void)
{
	// Ensure we cause a complete exit.
	MCquit = True;
	MCexitall = True;
	
	// Switch to the script thread to allow the wait to exit.
	MCFiberMakeCurrent(s_script_fiber);
	
	// Now invoke will terminate.
	MCFiberCall(s_script_fiber, MCIPhoneDoWillTerminate, nil);
	
	// The app terminates after this routine exits. So make sure we
	// do the final clean up of the fibers.
	MCFiberDestroy(s_script_fiber);
	s_script_fiber = nil;
	MCFiberDestroy(s_main_fiber);
	s_main_fiber = nil;
}

// MW-2012-10-04: Main handler for when the app should suspend.
void MCIPhoneHandleSuspend(void)
{
}

// MW-2012-10-04: Main handler for when the app should resume.
void MCIPhoneHandleResume(void)
{
}

// MW-2012-08-06: [[ Fibers ]] Main (system) fiber side handler for didReceiveMemoryWarning.
void MCIPhoneHandleDidReceiveMemoryWarning(void)
{
	static_cast<MCScreenDC *>(MCscreen) -> compact_memory();
	MCCachedImageRep::FlushCache();
}

// MW-2012-08-06: [[ Fibers ]] Main (system) fiber side handler for viewBoundsChanged.
void MCIPhoneHandleViewBoundsChanged(void)
{
	MCFiberCall(s_script_fiber, MCIPhoneDoViewBoundsChanged, nil);
}

//////////

struct MCOrientationChangedEvent: public MCCustomEvent
{
	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
		MCdefaultstackptr -> getcurcard() -> message(MCM_orientation_changed);
		// MW-2012-08-06: [[ Fibers ]] Invoke the subsequent action on the main fiber.
		/* REMOTE */ MCFiberCallSelector(s_main_fiber, MCIPhoneGetApplication(), @selector(commitOrientation));
	}
};

void MCIPhoneHandleOrientationChanged(void)
{
	MCEventQueuePostCustom(new MCOrientationChangedEvent);
}

//////////

static NSInteger compare_touch_timestamps(id a, id b, void *context)
{
	double ta, tb;
	ta = [a timestamp];
	tb = [b timestamp];
	return ta < tb ? NSOrderedAscending : (ta > tb ? NSOrderedDescending : NSOrderedSame);
}

void MCIPhoneHandleTouches(UIView *p_view, NSSet *p_touches, UITouchPhase p_phase)
{
	NSArray *t_sorted_touches;
	t_sorted_touches = [[p_touches allObjects] sortedArrayUsingFunction: compare_touch_timestamps context: nil];
	
	MCEventTouchPhase t_phase;
	if (p_phase == UITouchPhaseBegan)
		t_phase = kMCEventTouchPhaseBegan;
	else if (p_phase == UITouchPhaseEnded)
		t_phase = kMCEventTouchPhaseEnded;
	else if (p_phase == UITouchPhaseMoved)
		t_phase = kMCEventTouchPhaseMoved;
	else if (p_phase == UITouchPhaseCancelled)
		t_phase = kMCEventTouchPhaseCancelled;
	else
		return;
	
	for(UITouch *t_touch in t_sorted_touches)
	{
		MCGFloat t_scale;
		t_scale = MCIPhoneGetDeviceScale();
		
		CGPoint t_location;
		t_location = [ t_touch locationInView: p_view ];
		
		MCPoint t_loc;
		t_loc = MCPointMake(t_location.x, t_location.y);
		
		// IM-2014-01-30: [[ HiDPI ]] Convert screen to logical coords
		t_loc = MCscreen -> screentologicalpoint(t_loc);
		
		static_cast<MCScreenDC *>(MCscreen) -> handle_touch(t_phase, t_touch, [t_touch timestamp] * 1000, t_loc . x, t_loc . y);
	}
}

//////////

class MCTextEditFinishedEvent: public MCCustomEvent
{
public:
	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
		static_cast<MCScreenDC *>(MCscreen) -> unfocus_current_window();
	}
};

void MCIPhoneHandleBeginTextInput(void)
{
}

void MCIPhoneHandleEndTextInput(void)
{
	MCEventQueuePostCustom(new MCTextEditFinishedEvent);
}

void MCIPhoneHandleProcessTextInput(uint32_t p_char_code, uint32_t p_key_code)
{
	if (!MCactivefield)
		return;

	static_cast<MCScreenDC *>(MCscreen) -> handle_key_press(0, p_char_code, p_key_code);
}

/////////

void MCIPhoneHandleMotionBegan(UIEventSubtype motion, NSTimeInterval timestamp)
{
	if (motion == UIEventSubtypeMotionShake)
		static_cast<MCScreenDC *>(MCscreen) -> handle_motion(kMCEventMotionShakeBegan, timestamp * 1000);
}

void MCIPhoneHandleMotionCancelled(UIEventSubtype motion, NSTimeInterval timestamp)
{
	if (motion == UIEventSubtypeMotionShake)
		static_cast<MCScreenDC *>(MCscreen) -> handle_motion(kMCEventMotionShakeCancelled, timestamp * 1000);
}

void MCIPhoneHandleMotionEnded(UIEventSubtype motion, NSTimeInterval timestamp)
{
	if (motion == UIEventSubtypeMotionShake)
		static_cast<MCScreenDC *>(MCscreen) -> handle_motion(kMCEventMotionShakeEnded, timestamp * 1000);
}

/////////

struct MCKeyboardActivatedEvent: public MCCustomEvent
{
	MCKeyboardActivatedEvent(float p_height)
	{
		m_height = p_height;
	}

	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
		s_current_keyboard_height = m_height;
        // MM-2014-10-08: [[ Bug 12464 ]] Clear the display info cache on keyboard activation/deactivation ensuring the effective screenrect returns the correct data.
        MCscreen -> cleardisplayinfocache();
		MCdefaultstackptr -> getcurcard() -> message(MCM_keyboard_activated);
	}
	
private:
	float m_height;
};

struct MCKeyboardDeactivatedEvent: public MCCustomEvent
{
	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
		s_current_keyboard_height = 0.0;
        // MM-2014-10-08: [[ Bug 12464 ]] Clear the display info cache on keyboard activation/deactivation ensuring the effective screenrect returns the correct data.
        MCscreen -> cleardisplayinfocache();
		MCdefaultstackptr -> getcurcard() -> message(MCM_keyboard_deactivated);
	}
};

void MCIPhoneHandleKeyboardWillActivate(float p_height)
{
	MCEventQueuePostCustom(new MCKeyboardActivatedEvent(p_height));
}

void MCIPhoneHandleKeyboardWillDeactivate(void)
{
	MCEventQueuePostCustom(new MCKeyboardDeactivatedEvent);
}

////////////////////////////////////////////////////////////////////////////////

void MCIPhoneHandlePerformRedraw(void)
{
	MCRedrawEnableScreenUpdates();
	MCRedrawUpdateScreen();
	MCRedrawDisableScreenUpdates();
}

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCIPhoneBreakWaitHelper : NSObject

- (void)breakWait;

@end

@implementation com_runrev_livecode_MCIPhoneBreakWaitHelper

- (void)breakWait
{
	// When the wait is broken, just jump to the script fiber.
	MCFiberMakeCurrent(s_script_fiber);
}

@end

static void MCIPhoneDoBreakWait(void *)
{
	[NSObject cancelPreviousPerformRequestsWithTarget: s_break_wait_helper];
	
	s_break_wait_pending = true;
	
	if (s_wait_depth > 0)
	{
		NSArray *t_modes;
		t_modes = [[NSArray alloc] initWithObjects: NSRunLoopCommonModes, nil];
		[s_break_wait_helper performSelector: @selector(breakWait) withObject: nil afterDelay: 0 inModes: t_modes];
		[t_modes release];
    }
}

static void MCIPhoneDoBreakWaitOnCorrectThread(void *context)
{
	MCFiberCall(s_main_fiber, MCIPhoneDoBreakWait, nil);
}

void MCIPhoneBreakWait(void)
{
	if (s_break_wait_pending)
		return;
	
	if (s_break_wait_helper == nil)
		s_break_wait_helper = [[com_runrev_livecode_MCIPhoneBreakWaitHelper alloc] init];

	iphone_run_on_main_thread((void *)MCIPhoneDoBreakWaitOnCorrectThread, nil, kMCExternalRunOnMainThreadUnsafe | kMCExternalRunOnMainThreadImmediate);
}

static void MCIPhoneDoScheduleWait(void *p_ctxt)
{
	double t_sleep;
	t_sleep = *(double *)p_ctxt;
	[s_break_wait_helper performSelector: @selector(breakWait) withObject: nil afterDelay: t_sleep inModes: [NSArray arrayWithObject: NSRunLoopCommonModes]];
}
	
static void MCIPhoneDoCancelWait(void *p_ctxt)
	{
	[NSObject cancelPreviousPerformRequestsWithTarget: s_break_wait_helper selector: @selector(breakWait) object: nil];
	}

static bool MCIPhoneWait(double p_sleep)
	{
	if (s_break_wait_pending)
		{
		MCFiberCall(s_main_fiber, MCIPhoneDoCancelWait, nil);
		s_break_wait_pending = false;
		return true;
		}
    
	if (s_break_wait_helper == nil)
		s_break_wait_helper = [[com_runrev_livecode_MCIPhoneBreakWaitHelper alloc] init];
	
	// Schedule the wait on the main fiber.
	MCFiberCall(s_main_fiber, MCIPhoneDoScheduleWait, &p_sleep);
	
	// Mark ourselves as waiting.
	s_wait_depth += 1;
	
	// Now switch back to the main fiber.
	MCFiberMakeCurrent(s_main_fiber);

	// Unmark ourselves as waiting.
	s_wait_depth -= 1;

	bool t_broken;
	t_broken = s_break_wait_pending;
	s_break_wait_pending = false;

	return t_broken;
	}
	
////////////////////////////////////////////////////////////////////////////////

void MCResPlatformInitPixelScaling(void)
{
	// IM-2014-08-18: [[ Bug 12372 ]] Move platform-specific setup to MCResPlatformInitPixelScaling.
	
	// IM-2013-07-18: [[ ResIndependence ]] store the device scale in our new static variable
	s_iphone_device_scale = [[UIScreen mainScreen] scale];
}

// IM-2014-01-30: [[ HiDPI ]] Pixel scaling supported on iOS
bool MCResPlatformSupportsPixelScaling(void)
{
	return true;
}

// IM-2014-01-30: [[ HiDPI  ]] Pixel scaling can be enabled / disable on iOS
bool MCResPlatformCanChangePixelScaling(void)
{
	return true;
}

// IM-2014-01-30: [[ HiDPI ]] Pixel scale can be modified on iOS
bool MCResPlatformCanSetPixelScale(void)
{
	return true;
}

// IM-2014-01-30: [[ HiDPI ]] Calculate logical to iOS screen coordinate scale
MCGFloat MCScreenDC::logicaltoscreenscale(void)
{
	// (logical coordinate to device coordinate scale) * (device coordinate to iOS screen coordinate scale)
	return MCResGetPixelScale() / MCIPhoneGetResolutionScale();
}

////////////////////////////////////////////////////////////////////////////////
