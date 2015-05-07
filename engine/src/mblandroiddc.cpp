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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

//#include "execpt.h"
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
#include "mode.h"
#include "osspec.h"
#include "redraw.h"
#include "region.h"
#include "font.h"

#include "mbldc.h"

#include "mblandroidutil.h"
#include "mblandroidjava.h"

#include "graphics.h"
#include "resolution.h"

#include <jni.h>
#include <pthread.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <GLES/gl.h>
#include <unistd.h>

#include "stacktile.cpp"

#include "script.h"

extern bool MCModulesInitialize();

////////////////////////////////////////////////////////////////////////////////

// Various globals depended on by other parts of the engine.

Boolean tripleclick = False;
uint4 g_current_background_colour = 0;

typedef void (*co_yield_callback_t)(void *);

////////////////////////////////////////////////////////////////////////////////

// These hold the current state of events and messages. revMobile uses a
// 'single-window' model. There is only ever one window displayed at a time,
// and the display history of open stacks is stored to ensure appropriate ones
// get revealed at the point of the top-most stack closing.

// The android port uses two threads in a co-operative fashion to handle the
// fact we don't have direct access to the run-loop. The Android UI runs on one
// thread, while the engine runs on its own.

static pthread_t s_android_ui_thread;
static pthread_t s_android_engine_thread;

static pthread_t s_coroutine_thread;
static pthread_mutex_t s_coroutine_mutex;
static pthread_cond_t s_coroutine_condition;

// Resolved method ids
static jmethodID s_schedule_wakeup_method = 0;
static jmethodID s_invalidate_method = 0;
static jmethodID s_openglview_start_method = 0;
static jmethodID s_openglview_finish_method = 0;
static jmethodID s_openglview_swap_method = 0;
static jmethodID s_openglview_configure_method = 0;

// The Java VM that we are bound to. This is set in the JNI_OnLoad startup
// function.
static JavaVM *s_java_vm = nil;
// The JNIEnv for the android UI thread.
static JNIEnv *s_android_ui_env;
// The JNI environment pointer *for our auxiliary thread*. This only has lifetime
// as long as 'mobile_main' is running.
static JNIEnv *s_java_env = nil;

// Wakeup vars - used to determine whether to post a wakeup message on yield
// to android.
static bool s_schedule_wakeup = false;
static uint32_t s_schedule_wakeup_timeout = 0;
static bool s_schedule_wakeup_breakable = false;
static bool s_schedule_wakeup_was_broken = false;

static co_yield_callback_t s_yield_callback = nil;
static void *s_yield_callback_context = nil;

// The bitmap containing the current visible state of the view
static jobject s_android_bitmap = nil;
static int s_android_bitmap_width = 0;
static int s_android_bitmap_height = 0;
static int s_android_bitmap_stride = 0;
static pthread_mutex_t s_android_bitmap_mutex;

// The dirty region of the bitmap
static MCGRegionRef s_android_bitmap_dirty_region = nil;
static MCRectangle s_android_bitmap_dirty;

// If non-nil, then we are in opengl mode.
static bool s_android_opengl_enabled = false;
static bool s_android_opengl_visible = false;
static jobject s_android_opengl_view = nil;

// This is the JNI reference to our display/view instance.
static jobject s_android_activity = nil;
static jobject s_android_container = nil;
static jobject s_android_view = nil;
static jobject s_android_view_class = nil;

// If this is false, then it means the engine broke somehow.
static bool s_engine_running = false;

int32_t g_android_keyboard_type = 1;

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-31: [[ HiDPI ]] Refactor view_platform_updatewindowwithcallback to use
//   view_platform_updatewindow method
static MCStackUpdateCallback s_updatewindow_callback = nil;
static void *s_updatewindow_context = nil;

////////////////////////////////////////////////////////////////////////////////

static void android_process(void);

static MCRectangle android_view_get_bounds(void);

void MCAndroidCustomFontsLoad();
float android_font_measure_text(void *p_font, const char *p_text, uint32_t p_text_length, bool p_is_unicode);


static void co_enter_engine(void);
static void co_leave_engine(void);
static void co_yield_to_engine(void);
static void co_yield_to_android(void);
static bool co_yield_to_android_and_wait(double sleep, bool wake_on_event);
static void co_yield_to_android_and_call(co_yield_callback_t callback, void *context);

static void revandroid_scheduleWakeUp(JNIEnv *env, jobject object, int32_t timeout, bool breakable);
static void revandroid_invalidate(JNIEnv *env, jobject object, int32_t left, int32_t top, int32_t right, int32_t bottom);
static bool revandroid_getAssetOffsetAndLength(JNIEnv *env, jobject object, const char *p_filename, int32_t& r_offset, int32_t& r_length);

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-31: [[ HiDPI ]] Return the screen pixel density
MCGFloat MCAndroidGetSystemScale(void)
{
	MCGFloat t_scale;
	MCAndroidEngineCall("getPixelDensity", "f", &t_scale);
	
	return t_scale;
}

////////////////////////////////////////////////////////////////////////////////

void MCResPlatformInitPixelScaling(void)
{
}

// IM-2014-01-31: [[ HiDPI ]] Pixel scaling supported on android
bool MCResPlatformSupportsPixelScaling(void)
{
	return true;
}

// IM-2014-01-31: [[ HiDPI ]] Pixel scaling cannot be disabled on Android
bool MCResPlatformCanChangePixelScaling(void)
{
	return false;
}

// IM-2014-01-31: [[ HiDPI ]] The pixelScale can be set on Android
bool MCResPlatformCanSetPixelScale(void)
{
	return true;
}

//////////

// IM-2014-01-31: [[ HiDPI ]] The default pixel scale on android is the display density
MCGFloat MCResPlatformGetDefaultPixelScale(void)
{
	return MCAndroidGetSystemScale();
}

// IM-2014-03-14: [[ HiDPI ]] UI scale is 1.0 on Android
MCGFloat MCResPlatformGetUIDeviceScale(void)
{
	return 1.0;
}

// IM-2014-01-31: [[ HiDPI ]] On Android use the configured pixelScale
MCGFloat MCScreenDC::logicaltoscreenscale(void)
{
	return MCResGetPixelScale();
}

//////////

void MCResPlatformHandleScaleChange(void)
{
	// IM-2014-01-31: [[ HiDPI ]] Update the main stack geometry
	static_cast<MCScreenDC *>(MCscreen) -> do_fit_window(false, true);
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidInvalidateBitmapRect(const MCGIntegerRectangle &p_rect)
{
	s_android_ui_env -> CallVoidMethod(s_android_view, s_invalidate_method,
									   p_rect.origin.x, p_rect.origin.y,
									   p_rect.origin.x + p_rect.size.width,
									   p_rect.origin.y + p_rect.size.height);
}

bool MCAndroidInvalidateBitmapCallback(void *p_context, const MCGIntegerRectangle &p_rect)
{
	MCAndroidInvalidateBitmapRect(p_rect);

	return true;
}

// IM-2014-06-11: [[ GraphicsPerformance ]] Call invalidate method on Java bitmap view
void MCAndroidInvalidateBitmapRegion(MCGRegionRef p_region)
{
	MCGRegionIterate(p_region, MCAndroidInvalidateBitmapCallback, nil);
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCScreenDC::open(void)
{
	common_open();

	// We don't need to do anything to initialize the view, as that is done
	// by the Java wrapper.

	// IM-2014-01-31: [[ HiDPI ]] Initialise updatewindow callback to nil
	s_updatewindow_callback = nil;
	s_updatewindow_context = nil;
	
	return True;
}

Boolean MCScreenDC::close(Boolean p_force)
{
	return True;
}

//////////

bool MCScreenDC::hasfeature(MCPlatformFeature p_feature)
{
	return false;
}

MCNameRef MCScreenDC::getdisplayname(void)
{
	return MCN_android;
}

MCNameRef MCScreenDC::getvendorname(void)
{
	return MCN_android;
}

uint2 MCScreenDC::device_getwidth()
{
	return 320;
}

uint2 MCScreenDC::device_getheight()
{
	return 480;
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

// IM-2014-01-31: [[ HiDPI ]] Refactor to return display rects in logical coords and include pixel scale
bool MCScreenDC::platform_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count)
{
	bool t_success;
	t_success = true;
	
	MCDisplay *t_displays;
	t_displays = nil;
	
	uint32_t t_count;
	t_count = 0;
	
	t_success = MCMemoryNewArray(1, t_displays);
	
	if (!t_success)
		return false;
	
	t_count = 1;
	
	MCRectangle t_viewport, t_workarea;

	MCAutoStringRef t_rect_string;
	int2 t_left, t_top, t_right, t_bottom;

	// The workarea is the rect of the screen
	// not covered by any OS furniture, the viewport the whole area of the sreen.

	// IM-2013-11-15: [[ Bug 10485 ]] Use appropriate java method to get (effective) working screenrect
	if (p_effective)
		MCAndroidEngineCall("getEffectiveWorkareaAsString", "x", &(&t_rect_string));
	else
        MCAndroidEngineCall("getWorkareaAsString", "x", &(&t_rect_string));
	MCU_stoi2x4(*t_rect_string, t_left, t_top, t_right, t_bottom);

	t_workarea = MCRectangleMake(t_left, t_top, t_right - t_left, t_bottom - t_top);

    MCAutoStringRef t_viewport_string;
	MCAndroidEngineCall("getViewportAsString", "x", &(&t_viewport_string));
	MCU_stoi2x4(*t_viewport_string, t_left, t_top, t_right, t_bottom);

	t_viewport = MCRectangleMake(t_left, t_top, t_right - t_left, t_bottom - t_top);

	// IM-2014-01-31: [[ HiDPI ]] Convert screen to logical coords
	t_viewport = MCScreenDC::screentologicalrect(t_viewport);
	t_workarea = MCScreenDC::screentologicalrect(t_workarea);

	MCLog("getdisplays(effective=%s): workarea(%d,%d,%d,%d) viewport(%d,%d,%d,%d)", p_effective?"true":"false",
		  t_workarea.x, t_workarea.y, t_workarea.width, t_workarea.height,
		  t_viewport.x, t_viewport.y, t_viewport.width, t_viewport.height);

	t_displays[0].index = 0;
	t_displays[0].pixel_scale = MCAndroidGetSystemScale();
	t_displays[0].viewport = t_viewport;
	t_displays[0].workarea = t_workarea;
	
	r_displays = t_displays;
	r_count = t_count;
	
	return true;
}

// IM-2014-01-31: [[ HiDPI ]] Display info updating not yet implemented on Android
bool MCScreenDC::platform_displayinfocacheable(void)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCScreenDC::platform_getwindowgeometry(Window w, MCRectangle &r_rect)
{
	MCRectangle t_rect;
	if (!device_getwindowgeometry(w, t_rect))
		return false;
	
	// IM-2014-01-31: [[ HiDPI ]] Convert screen to logical coords
	r_rect = screentologicalrect(t_rect);
	
	return true;
}

bool MCScreenDC::device_getwindowgeometry(Window w, MCRectangle &drect)
{
	drect = android_view_get_bounds();
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::beep(void)
{
	MCAndroidEngineRemoteCall("doBeep", "vi", nil, 1);
}

bool MCScreenDC::setbeepsound(MCStringRef p_beep_sound)
{
	return false;
}

bool MCScreenDC::getbeepsound(MCStringRef& r_beep_sound)
{
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

MCImageBitmap *MCScreenDC::snapshot(MCRectangle &r, uint4 window, MCStringRef displayname, MCPoint *size)
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCScreenDC::wait(real8 duration, Boolean dispatch, Boolean anyevent)
{
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
			{
				done = True;
				break;
			}

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

		// At this point we yield to android, requesting that we get control back
		// within the specified time.
		bool t_broken;
		//MCLog("Yielding to android for %lf (anyevent = %d)", t_sleep, anyevent);
		t_broken = co_yield_to_android_and_wait(t_sleep, anyevent);
		//MCLog("Control returned to engine (broken = %d)", t_broken);

		// If the wait was broken before timeout and we are qutting on any
		// event, we are done.
		if (t_broken && anyevent)
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

	return abort;
}

// MW-2011-08-16: [[ Wait ]] This is a no-op on Android at the moment.
void MCScreenDC::pingwait(void)
{
	MCAndroidEngineRemoteCall("wakeEngineThread", "v", nil);
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::openIME()
{
}

void MCScreenDC::activateIME(Boolean activate)
{
	MCAndroidEngineRemoteCall("setTextInputVisible", "vb", nil, activate);
}

void MCScreenDC::closeIME()
{
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::do_take_focus(void)
{
}

void MCScreenDC::do_fit_window(bool p_immediate_resize, bool p_post_message)
{
	if (m_current_window == nil)
		return;

	// Make sure view is configured to be the right size...
	MCRectangle drect;
	drect = android_view_get_bounds();

	// IM-2014-03-03: [[ Bug 11836 ]] Store window topleft in logical coords
	MCPoint t_topleft;
	t_topleft = MCPointMake(drect.x, drect.y);

	t_topleft = screentologicalpoint(t_topleft);

	m_window_left = t_topleft . x;
	m_window_top = t_topleft . y;

	if (p_post_message)
	{
		if (p_immediate_resize)
		{
			// IM-2014-01-31: [[ HiDPI ]] Ensure stack view is updated with the current pixel scale
			((MCStack*)m_current_window)->view_setbackingscale(MCResGetPixelScale());
			((MCStack *)m_current_window) -> view_configure(true);
		}
		else
		{
			// IM-2014-02-14: [[ HiDPI ]] Post backing scale changes with window reshape message
			MCEventQueuePostWindowReshape((MCStack *)m_current_window, MCResGetPixelScale());
		}

		// When we get a resize from android, we need to redraw the whole thing
		// as the buffer will have changed!
		((MCStack *)m_current_window) -> dirtyrect(((MCStack *)m_current_window) -> getcurcard() -> getrect());
	}
}

Window MCScreenDC::get_current_window(void)
{
	return m_current_window;
}

////////////////////////////////////////////////////////////////////////////////

static MCRectangle android_view_get_bounds(void)
{
	MCRectangle r;
	MCU_set_rect(r, 0, 0, s_android_bitmap_width, s_android_bitmap_height);
	return r;
}

////////////////////////////////////////////////////////////////////////////////

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use the new stack surface API.
class MCAndroidStackSurface: public MCStackSurface
{
	MCGRegionRef m_region;
	void *m_pixels;

public:
	MCAndroidStackSurface(MCGRegionRef p_region)
	{
		m_region = p_region;
		m_pixels = nil;
    }

	bool Lock(void)
	{
		if (s_android_bitmap == nil)
			return false;

		if (m_pixels != nil)
			return false;

		if (AndroidBitmap_lockPixels(s_java_env, s_android_bitmap, &m_pixels) < 0)
			return false;

		return true;
	}
	
	void Unlock(void)
	{
		if (m_pixels == nil)
			return;
		
		AndroidBitmap_unlockPixels(s_java_env, s_android_bitmap);
		m_pixels = nil;
	}
    
    bool LockGraphics(MCGIntegerRectangle p_area, MCGContextRef &r_context, MCGRaster &r_raster)
	{
		MCGRaster t_raster;
		MCGIntegerRectangle t_locked_area;
		if (LockPixels(p_area, t_raster, t_locked_area))
		{
            MCGContextRef t_context;
            if (MCGContextCreateWithRaster(t_raster, t_context))
			{
				// Set origin
                MCGContextTranslateCTM(t_context, -t_locked_area . origin . x, -t_locked_area . origin . y);
                
				// Set clipping rect
                MCGContextClipToRegion(t_context, m_region);
				MCGContextClipToRect(t_context, MCGIntegerRectangleToMCGRectangle(t_locked_area));
				
				r_context = t_context;
                r_raster = t_raster;
				
				return true;
			}
			
			UnlockPixels(t_locked_area, t_raster, false);
		}
		
		return false;
	}

	void UnlockGraphics(MCGIntegerRectangle p_area, MCGContextRef p_context, MCGRaster &p_raster)
	{
		if (p_context == nil)
			return;
		
		MCGContextRelease(p_context);
		UnlockPixels(p_area, p_raster, false);
		
		// IM-2014-06-11: [[ Graphics Performance ]] Mark the locked region for redraw
		MCGRegionAddRegion(s_android_bitmap_dirty_region, m_region);
	}
    
    bool LockPixels(MCGIntegerRectangle p_area, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area)
    {
        MCGIntegerRectangle t_actual_area;
        t_actual_area = MCGIntegerRectangleIntersection(p_area, MCGRegionGetBounds(m_region));
        
        if (MCGIntegerRectangleIsEmpty(t_actual_area))
            return false;
        
        r_raster . width = t_actual_area . size . width ;
        r_raster . height = t_actual_area . size . height;
        r_raster . stride = s_android_bitmap_stride;
        r_raster . format = kMCGRasterFormat_xRGB;
		r_raster.pixels = (uint8_t*)m_pixels + t_actual_area . origin . y * s_android_bitmap_stride + t_actual_area . origin . x * sizeof(uint32_t);

		r_locked_area = t_actual_area;

        return true;
    }

	void UnlockPixels(MCGIntegerRectangle p_area, MCGRaster& p_raster)
	{
		UnlockPixels(p_area, p_raster, true);
	}
	
	void UnlockPixels(MCGIntegerRectangle p_area, MCGRaster& p_raster, bool p_update)
	{
		if (p_raster . pixels == nil)
			return;
        
		if (p_update)
			MCGRegionAddRect(s_android_bitmap_dirty_region, p_area);
	}

	bool LockTarget(MCStackSurfaceTargetType p_type, void*& r_target)
	{
        return false;
	}

	void UnlockTarget(void)
	{
	}

    bool Composite(MCGRectangle p_dst_rect, MCGImageRef p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend)
	{
		bool t_success = true;
        
        MCGIntegerRectangle t_bounds;
        MCGContextRef t_context = nil;
        MCGRaster t_raster;
		if (t_success)
        {
            t_bounds = MCGRectangleGetBounds(p_dst_rect);
            t_success = LockGraphics(t_bounds, t_context, t_raster);
        }
		
		if (t_success)
		{
            MCGContextSetBlendMode(t_context, p_blend);
			MCGContextSetOpacity(t_context, p_alpha);

            // MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types (was nearest).
			MCGContextDrawRectOfImage(t_context, p_src, p_src_rect, p_dst_rect, kMCGImageFilterNone);
		}
		
		UnlockGraphics(t_bounds, t_context, t_raster);
		
		return t_success;
	}
};

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use the new stack surface API.
class MCOpenGLStackSurface: public MCStackSurface
{
	void *m_buffer_pixels;
	uint32_t m_buffer_stride;
	
	bool m_locked;
	bool m_update;

public:
	MCOpenGLStackSurface(void)
	{
		m_buffer_pixels = nil;
		m_locked = false;
		m_update = false;
	}

	bool Lock(void)
	{
		if (m_buffer_pixels != nil)
			return false;

		m_buffer_stride = s_android_bitmap_width * sizeof(uint32_t);
		if (!MCMemoryAllocate(s_android_bitmap_height * m_buffer_stride, m_buffer_pixels))
			return false;

		return true;
	}

	void Unlock()
	{
		if (m_buffer_pixels == nil)
			return;
		
		if (m_update)
			FlushBits(m_buffer_pixels, m_buffer_stride);
		
		MCMemoryDeallocate(m_buffer_pixels);
		m_buffer_pixels = nil;
	}
    
    bool LockGraphics(MCGIntegerRectangle p_area, MCGContextRef &r_context, MCGRaster &r_raster)
	{
		MCGRaster t_raster;
		MCGIntegerRectangle t_locked_area;
		if (LockPixels(p_area, t_raster, t_locked_area))
		{
            MCGContextRef t_context;
            if (MCGContextCreateWithRaster(t_raster, t_context))
			{
                r_raster = t_raster;
				return true;
			}
			UnlockPixels(t_locked_area, t_raster, false);
		}
		return false;
	}
    
	void UnlockGraphics(MCGIntegerRectangle p_area, MCGContextRef p_context, MCGRaster &p_raster)
	{
		if (p_context == nil)
			return;
		
		MCGContextRelease(p_context);
		UnlockPixels(p_area, p_raster);
	}

    bool LockPixels(MCGIntegerRectangle p_area, MCGRaster &r_raster, MCGIntegerRectangle &r_locked_area)
	{
		if (m_locked)
			return false;
        
		r_raster.width = s_android_bitmap_width;
		r_raster.height = s_android_bitmap_height;
		r_raster.stride = m_buffer_stride;
		r_raster.pixels = m_buffer_pixels;
		r_raster.format = kMCGRasterFormat_xRGB;
        
		r_locked_area = p_area;

		m_locked = true;
        
		return true;
	}
    
	void UnlockPixels(MCGIntegerRectangle p_area, MCGRaster& p_raster)
	{
		UnlockPixels(p_area, p_raster, true);
	}

	void UnlockPixels(MCGIntegerRectangle p_area, MCGRaster& p_raster, bool p_update)
	{
		if (!m_locked)
			return;
		
		if (p_update)
			m_update = true;
		
		m_locked = false;
	}
    
	bool LockTarget(MCStackSurfaceTargetType p_type, void*& r_context)
	{
		if (p_type != kMCStackSurfaceTargetEAGLContext)
			return false;

		return true;
	}

	void UnlockTarget(void)
	{
	}
    
    bool Composite(MCGRectangle p_dst_rect, MCGImageRef p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend)
	{
		bool t_success = true;
        
        MCGIntegerRectangle t_bounds;
        MCGContextRef t_context = nil;
        MCGRaster t_raster;
		if (t_success)
        {
            t_bounds = MCGRectangleGetBounds(p_dst_rect);
            t_success = LockGraphics(t_bounds, t_context, t_raster);
        }
		
		if (t_success)
		{
            MCGContextSetBlendMode(t_context, p_blend);
			MCGContextSetOpacity(t_context, p_alpha);
            
            // MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types (was nearest).
			MCGContextDrawRectOfImage(t_context, p_src, p_src_rect, p_dst_rect, kMCGImageFilterNone);
		}
		
		UnlockGraphics(t_bounds, t_context, t_raster);
		
		return t_success;
	}

protected:
	static void FlushBits(void *p_bits, uint32_t p_stride)
	{
		GLuint t_texture;
		glGenTextures(1, &t_texture);
		glBindTexture(GL_TEXTURE_2D, t_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // MW-2011-10-07: iOS 5 doesn't like an inconsistency in input format between
        //   TexImage2D and TexSubImage2D.
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		GLfloat t_vertices[8];

		GLfloat t_coords[8] =
		{
			0, 0,
			1.0, 0.0,
			0.0, 1.0,
			1.0, 1.0
		};

		glVertexPointer(2, GL_FLOAT, 0, t_vertices);
		glTexCoordPointer(2, GL_FLOAT, 0, t_coords);

		for(int32_t y = 0; y < (s_android_bitmap_height + 255) / 256; y++)
			for(int32_t x = 0; x < (s_android_bitmap_width + 255) / 256; x++)
			{
				int32_t t_tw, t_th;
				t_tw = MCMin(256, s_android_bitmap_width - x * 256);
				t_th = MCMin(256, s_android_bitmap_height - y * 256);

				// Fill the texture scanline by scanline
				for(int32_t s = 0; s < t_th; s++)
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, s, t_tw, 1, GL_RGBA, GL_UNSIGNED_BYTE, (uint8_t *)p_bits + (y * 256 + s) * p_stride + x * 256 * sizeof(uint32_t));

				int32_t t_px, t_py;
				t_px = x * 256;
				t_py = s_android_bitmap_height - y * 256 - 256;

				// Setup co-ords.
				t_vertices[0] = t_px, t_vertices[1] = t_py + 256;
				t_vertices[2] = t_px + 256, t_vertices[3] = t_py + 256;
				t_vertices[4] = t_px, t_vertices[5] = t_py;
				t_vertices[6] = t_px + 256, t_vertices[7] = t_py;

				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}

		glDeleteTextures(1, &t_texture);
	}
};

void MCStack::view_device_updatewindow(MCRegionRef p_region)
{
	// IM-2014-01-31: [[ HiDPI ]] If using a callback, render to the Android bitmap view
	if (s_updatewindow_callback != nil || !s_android_opengl_enabled)
	{
		// MW-2011-10-01: [[ Bug 9772 ]] At the moment, dirtyrect() calls that
		//   occur prior to 'configure()' being called for the first time will
		//   result in an update region being too big. Thus we must restrict.

		MCGRegionRef t_region;
		t_region = (MCGRegionRef)p_region;
		
		MCGRegionIntersectRect(t_region, MCGIntegerRectangleMake(0, 0, s_android_bitmap_width, s_android_bitmap_height));

        MCAndroidStackSurface t_surface(t_region);
        if (t_surface.Lock())
        {
            // IM-2014-01-31: [[ HiDPI ]] If a callback is given then use it to render to the surface
            if (s_updatewindow_callback != nil)
                s_updatewindow_callback(&t_surface, (MCRegionRef)t_region, s_updatewindow_context);
            else
                view_surface_redrawwindow(&t_surface, t_region);
            
            t_surface.Unlock();
        }
        
		// MW-2012-09-04: [[ Bug 10333 ]] Make sure we cause a screen flush.
		co_yield_to_android_and_wait(0.0, true);
	}
	else
	{
		MCOpenGLStackSurface t_surface;

		glClear(GL_COLOR_BUFFER_BIT);

		glViewport(0, 0, s_android_bitmap_width, s_android_bitmap_height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrthof(0, (GLfloat)s_android_bitmap_width, 0, (GLfloat)s_android_bitmap_height, 0, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDisable(GL_DEPTH_TEST);
		glDisableClientState(GL_COLOR_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		MCGRegionRef t_dirty_rgn;
		MCGRegionCreate(t_dirty_rgn);
		MCGRegionSetRect(t_dirty_rgn, MCGIntegerRectangleMake(0, 0, s_android_bitmap_width, s_android_bitmap_height));
		
		if (t_surface.Lock())
		{
			view_surface_redrawwindow(&t_surface, t_dirty_rgn);
			t_surface.Unlock();
		}
		
		s_java_env -> CallVoidMethod(s_android_opengl_view, s_openglview_swap_method, nil);
		MCGRegionDestroy(t_dirty_rgn);

		// If the OpenGL view is not visible, then hide the bitmap view
		// to reveal it.
		if (!s_android_opengl_visible)
		{
			s_android_opengl_visible = true;

			// MW-2011-12-12: [[ Bug 9908 ]] Make sure both front and back buffers hold the same image
			//   to prevent a flicker back to an old frame when making the opengl layer visible.
			view_device_updatewindow(p_region);

			MCAndroidEngineRemoteCall("hideBitmapView", "v", nil);
		}
	}

}

// IM-2014-01-31: [[ HiDPI ]] Refactor view_platform_updatewindowwithcallback to use
//   view_platform_updatewindow method
void MCStack::view_platform_updatewindowwithcallback(MCRegionRef p_region, MCStackUpdateCallback p_callback, void *p_context)
{
	s_updatewindow_callback = p_callback;
	s_updatewindow_context = p_context;

	view_platform_updatewindow(p_region);

	s_updatewindow_callback = nil;
	s_updatewindow_context = nil;

	// If we are in OpenGL mode, then show the bitmap view.
	if (s_android_opengl_enabled)
	{
		s_android_opengl_visible = false;
		MCAndroidEngineRemoteCall("showBitmapView", "v", nil);
	}
}

void MCStack::preservescreenforvisualeffect(const MCRectangle& p_rect)
{
	// If we are not in OpenGL mode, then there's nothing to sync.
	if (!s_android_opengl_enabled || !s_android_opengl_visible)
		return;

	// IM-2013-10-03: [[ FullscreenMode ]] get region in device coords for comparison
	MCRectangle t_device_rect;
	t_device_rect = MCRectangleGetTransformedBounds(p_rect, getdevicetransform());
	
	// If we are doing a full screen effect, we don't need to ensure the rest
	// of the bitmap is in sync.
	if (t_device_rect . width == s_android_bitmap_width && t_device_rect . height == s_android_bitmap_height)
		return;

	MCGIntegerRectangle t_rect;
	t_rect = MCGIntegerRectangleMake(0, 0, s_android_bitmap_width, s_android_bitmap_height);
	
	MCGRegionRef t_actual_region;
	MCGRegionCreate(t_actual_region);
	MCGRegionSetRect(t_actual_region, t_rect);

	MCAndroidStackSurface t_surface(t_actual_region);

	if (t_surface . Lock())
	{
		MCGRaster t_raster;
		MCGIntegerRectangle t_locked_area;
		// Lock the whole surface of the bitmap.
		if (t_surface . LockPixels(t_rect, t_raster, t_locked_area))
		{
			// We need the contents of the last presented framebuffer. To ensure
			// we get that, force an (OpenGL) update before reading the pixels.
			view_device_updatewindow((MCRegionRef)t_actual_region);

			// Fetch the contents of the framebuffer.
			glReadPixels(0, 0, s_android_bitmap_width, s_android_bitmap_height, GL_RGBA, GL_UNSIGNED_BYTE, t_raster . pixels);

			// glReadPixels gives us the bitmap the wrong way up, so swap it round.
			void *t_scanline;
			/* UNCHECKED */ t_scanline = malloc(t_raster . stride);
			for(int y = 0; y < s_android_bitmap_height / 2; y++)
			{
				memcpy(t_scanline, (char *)t_raster . pixels + y * t_raster . stride, t_raster . stride);
				memcpy((char *)t_raster . pixels + y * t_raster . stride, (char *)t_raster . pixels + (s_android_bitmap_height - y - 1) * t_raster . stride, t_raster . stride);
				memcpy((char *)t_raster . pixels + (s_android_bitmap_height - y - 1) * t_raster . stride, t_scanline, t_raster . stride);
			}
			free(t_scanline);
		}
		t_surface . Unlock();
	}

	MCGRegionDestroy(t_actual_region);
}

////////////////////////////////////////////////////////////////////////////////

Bool X_init(int argc, MCStringRef argv[], int envc, MCStringRef envp[]);
bool X_main_loop_iteration(void);
int X_close(void);
void send_startup_message(bool p_do_relaunch = true);
extern void MCQuit(void);

IO_handle android_get_mainstack_stream(void)
{
	// I.M.  01/06/2011
	// open main stack through asset path rather than apk file + offset

	MCAutoStringRef t_asset_filename;

	if (!MCStringFormat(&t_asset_filename, "%@/revandroidmain.rev", MCcmd))
		return nil;
    
    return MCS_open(*t_asset_filename, kMCOpenFileModeRead, False, False, 0);
}

static void empty_signal_handler(int)
{
}

static void *mobile_main(void *arg)
{
	co_enter_engine();

	// We are now running on a nice, shiny, native thread. However, Dalvik is
	// completely unaware that we exist. This is not good, since we will want
	// to call into Dalvik via the JNI from this thread. So we need to bind
	// our current thread to the VM.
    
    //MCInitialize();
    
	MCLog("Attaching thread to VM %p", s_java_vm);

	// Attach ourselves to the JVM - if we fail, we just return.
	if (s_java_vm -> AttachCurrentThread(&s_java_env, nil) < 0)
	{
		co_leave_engine();
		return (void *)1;
	}

    // PM-2015-02-19: [[ Bug 14489 ]] Init statics on restart of an app
    if (!MCJavaInitialize(s_java_env))
    {
		co_leave_engine();
		return (void *)1;
	}
    
	// MW-2011-08-11: [[ Bug 9671 ]] Make sure we initialize MCstackbottom.
	int i;
	MCstackbottom = (char *)&i;

	// IM-2014-06-11: [[ GraphicsPerformance ]] Create initially empty redraw region
	s_android_bitmap_dirty_region = nil;
	/* UNCHECKED */ MCGRegionCreate(s_android_bitmap_dirty_region);
	
	// Make sure when a 'SIGINT' is sent to this thread, it causes any system
	// calls to be interrupted (this thread will spend much of its time in a
	// 'select' loop).
	struct sigaction t_sig_action;
	t_sig_action . sa_handler = empty_signal_handler;
	t_sig_action . sa_flags = 0;
	sigaction(SIGINT, &t_sig_action, nil);

	// We don't care too much about args and env vars at the moment.
	// (The only argument is the name and there are no env vars)
	MCStringRef t_args[1], t_env[1];
	int argc = 1;
	int envc = 0;
	MCAndroidEngineCall("getPackagePath", "x", &t_args[0]);
	t_env[0] = nil;

	MCLog("args[0] = %@", t_args[0]);

	// Make sure MCquit is false before we start running things
	MCquit = False;
	MCquitisexplicit = False;
	MCexitall = False;

	// Initialize and the run the main loop

	MCLog("Calling X_init", 0);

	if (!X_init(argc, t_args, envc, t_env))
	{
		MCLog("X_init failed", 0);

		// IM-2013-05-01: [[ BZ 10586 ]] signal java ui thread to exit
		// finish LiveCodeActivity
		MCAndroidEngineRemoteCall("finishActivity", "v", nil);
		
		// Yield for now as we don't detect error states correctly.
		co_yield_to_android();

        // Free global refs
        MCJavaFinalize(s_java_env);
        
		// Now detach (will be called as a result of doDestroy)
		s_java_vm -> DetachCurrentThread();
		co_leave_engine();

		return (void *)1;
	}

	MCLog("Calling mode initialize", 0);

	// Load device-specific configuration
	MCAndroidLoadDeviceConfiguration();
    
    // MM-2012-03-05: Load any custom fonts included in the package
    MCAndroidCustomFontsLoad();

	// MW-2011-10-01: [[ Bug 9772 ]] Switch to the android thread until we get
	//   a bitmap to render into!
	while(s_android_bitmap == nil)
		co_yield_to_android();

	MCLog("Starting up project", 0);
	send_startup_message(false);
    
    // PM-2015-02-02: [[ Bug 14456 ]] Make sure the billing provider is properly initialized before a preopenstack/openstack message is sent
    MCAndroidInitEngine();
    
	if (!MCquit)
		MCdispatcher -> gethome() -> open();
    
	MCLog("Hiding splash screen", 0);
	MCAndroidEngineRemoteCall("hideSplashScreen", "v", nil);

	while(s_engine_running)
	{
		if (!X_main_loop_iteration())
			break;
	}

	MCLog("Shutting down project", 0);
	
	MCLog("Calling X_close", 0);
	X_close();

	// IM-2013-05-01: [[ BZ 10586 ]] signal java ui thread
	// and wait for it to exit
	MCAndroidEngineRemoteCall("finishActivity", "v", nil);
	
	while (s_engine_running)
		co_yield_to_android();
	
	// Free arguments and environment vars
	for (int i = 0; i < argc; i++)
		MCValueRelease(t_args[i]);
	for (int i = 0; i < envc; i++)
		MCValueRelease(t_env[i]);

    // Free global refs
    MCJavaFinalize(s_java_env);
	// We have finished with the engine now, so detach from the thread
	s_java_vm -> DetachCurrentThread();

	co_leave_engine();

	return (void *)0;
}

////////////////////////////////////////////////////////////////////////////////

static void co_yield(pthread_t p_yieldee)
{
	pthread_mutex_lock(&s_coroutine_mutex);
	s_coroutine_thread = p_yieldee;
	pthread_mutex_unlock(&s_coroutine_mutex);

	pthread_cond_signal(&s_coroutine_condition);

	pthread_mutex_lock(&s_coroutine_mutex);
	while(s_coroutine_thread != pthread_self())
		pthread_cond_wait(&s_coroutine_condition, &s_coroutine_mutex);
	pthread_mutex_unlock(&s_coroutine_mutex);
}

static void co_enter_engine(void)
{
	pthread_mutex_lock(&s_coroutine_mutex);
	while(s_coroutine_thread != pthread_self())
		pthread_cond_wait(&s_coroutine_condition, &s_coroutine_mutex);
	pthread_mutex_unlock(&s_coroutine_mutex);
}

static void co_leave_engine(void)
{
	pthread_mutex_lock(&s_coroutine_mutex);
	s_coroutine_thread = s_android_ui_thread;
	pthread_mutex_unlock(&s_coroutine_mutex);

	pthread_cond_signal(&s_coroutine_condition);
}

static void co_yield_to_engine(void)
{
	co_yield(s_android_engine_thread);

	// Service callbacks while we have them.
	while(s_yield_callback != nil)
	{
		co_yield_callback_t t_callback;
		void *t_context;

		t_callback = s_yield_callback;
		t_context = s_yield_callback_context;

		s_yield_callback = nil;
		s_yield_callback_context = nil;

		t_callback(t_context);

		co_yield(s_android_engine_thread);
	}

	// If a wakeup should be scheduled, inform the engine view
	if (s_schedule_wakeup)
	{
		s_schedule_wakeup = false;
		s_android_ui_env -> CallVoidMethod(s_android_view, s_schedule_wakeup_method, s_schedule_wakeup_timeout, s_schedule_wakeup_breakable);
	}

	// If the screen needs updating, post an invalidate event.
	if (!MCGRegionIsEmpty(s_android_bitmap_dirty_region))
	{
		MCAndroidInvalidateBitmapRegion(s_android_bitmap_dirty_region);
		MCGRegionSetEmpty(s_android_bitmap_dirty_region);
	}
}

static void co_yield_to_engine_and_call(co_yield_callback_t callback, void *context)
{
	void *t_stack;
	s_yield_callback = callback;
	s_yield_callback_context = context;
	co_yield_to_engine();;
}

static void co_yield_to_android(void)
{
	co_yield(s_android_ui_thread);

	// Service callbacks while we have them.
	while(s_yield_callback != nil)
	{
		co_yield_callback_t t_callback;
		void *t_context;

		t_callback = s_yield_callback;
		t_context = s_yield_callback_context;

		s_yield_callback = nil;
		s_yield_callback_context = nil;

		t_callback(t_context);

		co_yield(s_android_ui_thread);
	}
}

static bool co_yield_to_android_and_wait(double p_sleep, bool p_wake_on_event)
{
	s_schedule_wakeup = true;
	s_schedule_wakeup_timeout = (uint32_t)(p_sleep * 1000.0);
	s_schedule_wakeup_breakable = p_wake_on_event;
	co_yield_to_android();
	s_schedule_wakeup = false;
	return s_schedule_wakeup_was_broken;
}

static void co_yield_to_android_and_call(co_yield_callback_t callback, void *context)
{
	void *t_stack;
	s_schedule_wakeup = false;
	s_yield_callback = callback;
	s_yield_callback_context = context;
	co_yield_to_android();
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidBreakWait(void)
{
	s_schedule_wakeup_was_broken = true;
	co_yield_to_engine();
}

struct MCAndroidEngineCallThreadContext
{
	const char *method;
	void *return_value;

    jobject object;
    MCJavaMethodParams *params;
};


static void MCAndroidEngineCallThreadCallback(void *p_context)
{
	MCAndroidEngineCallThreadContext *context;
	context = (MCAndroidEngineCallThreadContext *)p_context;

	bool t_success = true;
	bool t_exception_thrown = false;

	bool t_cleanup_java_refs = true;

    JNIEnv *t_env;
    t_env = MCJavaGetThreadEnv();

    MCJavaMethodParams *t_params = nil;

    t_params = context->params;

    jmethodID t_method_id;
	t_method_id = 0;

    jclass t_class = nil;

    t_class = t_env->GetObjectClass(context->object);
    if (t_success)
	{
		t_method_id = t_env -> GetMethodID(t_class, context -> method, t_params->signature);
		if (t_method_id == 0)
			t_success = false;
	}

	if (t_success)
	{
		switch(t_params->return_type)
		{
		case kMCJavaTypeVoid:
            t_env -> CallVoidMethodA(context->object, t_method_id, t_params->params);
			if (t_cleanup_java_refs && t_env -> ExceptionCheck())
			{
				t_exception_thrown = true;
				t_success = false;
			}
			break;
		case kMCJavaTypeInt:
			*((int32_t *)(context -> return_value)) = t_env -> CallIntMethodA(context->object, t_method_id, t_params->params);
			if (t_cleanup_java_refs && t_env -> ExceptionCheck())
			{
				t_exception_thrown = true;
				t_success = false;
			}
			break;
        case kMCJavaTypeLong:
            *((int64_t *)(context -> return_value)) = t_env -> CallLongMethodA(context->object, t_method_id, t_params->params);
            if (t_cleanup_java_refs && t_env -> ExceptionCheck())
            {
                t_exception_thrown = true;
                t_success = false;
            }
            break;
        case kMCJavaTypeFloat:
            *((float*)(context -> return_value)) = t_env -> CallFloatMethodA(context->object, t_method_id, t_params->params);
            if (t_cleanup_java_refs && t_env -> ExceptionCheck())
            {
                t_exception_thrown = true;
                t_success = false;
            }
            break;
        case kMCJavaTypeDouble:
            *((double *)(context -> return_value)) = t_env -> CallDoubleMethodA(context->object, t_method_id, t_params->params);
            if (t_cleanup_java_refs && t_env -> ExceptionCheck())
            {
                t_exception_thrown = true;
                t_success = false;
            }
            break;
		case kMCJavaTypeBoolean:
			*((bool *)(context -> return_value)) = JNI_TRUE == t_env -> CallBooleanMethodA(context->object, t_method_id, t_params->params);
			if (t_cleanup_java_refs && t_env -> ExceptionCheck())
			{
				t_exception_thrown = true;
				t_success = false;
			}
			break;
		case kMCJavaTypeCString:
		{
			jstring t_java_string;
			t_java_string = (jstring)t_env -> CallObjectMethodA(context->object, t_method_id, t_params->params);
			if (t_cleanup_java_refs && t_env -> ExceptionCheck())
			{
				t_exception_thrown = true;
				t_success = false;
			}

            char *t_cstring = nil;
			if (t_success)
                t_success = MCJavaStringToNative(t_env, t_java_string, t_cstring);
            if (t_success)
                *(char **)(context -> return_value) = t_cstring;

			t_env -> DeleteLocalRef(t_java_string);
		}
		break;
		case kMCJavaTypeMCString:
			{
				jstring t_java_string;
				t_java_string = (jstring)t_env -> CallObjectMethodA(context->object, t_method_id, t_params->params);
				if (t_cleanup_java_refs && t_env -> ExceptionCheck())
				{
					t_exception_thrown = true;
					t_success = false;
				}

                char *t_cstring;
				if (t_success)
                    t_success = MCJavaStringToNative(t_env, t_java_string, t_cstring);
                if (t_success)
                    ((MCString*)context -> return_value) -> set(t_cstring, MCCStringLength(t_cstring));

				t_env -> DeleteLocalRef(t_java_string);
			}
			break;
		case kMCJavaTypeMCStringUnicode:
			{
				jstring t_java_string;
				t_java_string = (jstring)t_env -> CallObjectMethodA(context->object, t_method_id, t_params->params);
				if (t_cleanup_java_refs && t_env -> ExceptionCheck())
				{
					t_exception_thrown = true;
					t_success = false;
				}

                unichar_t *t_unicode_string = nil;
                uint32_t t_unicode_length = 0;
				if (t_success)
                    t_success = MCJavaStringToUnicode(t_env, t_java_string, t_unicode_string, t_unicode_length);
                if (t_success)
                    ((MCString*)context -> return_value) -> set((char*)t_unicode_string, t_unicode_length * 2);

				t_env -> DeleteLocalRef(t_java_string);
			}
			break;
		case kMCJavaTypeMCStringRef:
			{
				jstring t_java_string;
				t_java_string = (jstring)t_env -> CallObjectMethodA(context->object, t_method_id, t_params->params);
				if (t_cleanup_java_refs && t_env -> ExceptionCheck())
				{
					t_exception_thrown = true;
					t_success = false;
				}
            
                MCStringRef t_string;
                if (t_success)
                    t_success = MCJavaStringToStringRef(t_env, t_java_string, t_string);
                
                if (t_success)
					*((MCStringRef *)context -> return_value) = t_string;

				t_env -> DeleteLocalRef(t_java_string);
			}
			break;
		case kMCJavaTypeByteArray:
			{
				jbyteArray t_byte_array;
				t_byte_array = (jbyteArray)t_env -> CallObjectMethodA(context->object, t_method_id, t_params->params);
				if (t_cleanup_java_refs && t_env -> ExceptionCheck())
				{
					t_exception_thrown = true;
					t_success = false;
				}

                MCDataRef t_data;
				if (t_success)
                    t_success = MCJavaByteArrayToDataRef(t_env, t_byte_array, t_data);
                if (t_success)
					*((MCDataRef *)context -> return_value) = t_data;

				t_env -> DeleteLocalRef(t_byte_array);
			}
			break;
        case kMCJavaTypeObject:
		case kMCJavaTypeMap:
            {
                jobject t_object;
                t_object = (jobject)t_env->CallObjectMethodA(context->object, t_method_id, t_params->params);
                if (t_cleanup_java_refs && t_env->ExceptionCheck())
                {
                    t_exception_thrown = true;
                    t_success = false;
                }

                // All object return values need to be passed through as global refs as we may be
                // switching threads on return
                if (t_success)
                    *((jobject*)(context->return_value)) = t_env->NewGlobalRef(t_object);

                t_env->DeleteLocalRef(t_object);
            }
		}
	}

	if (t_exception_thrown)
	{
		MCLog("unhandled exception in %s", context -> method);
#ifdef _DEBUG
		t_env -> ExceptionDescribe();
#endif
		t_env -> ExceptionClear();
	}

    t_env->DeleteLocalRef(t_class);
}

void MCAndroidJavaMethodCall(jobject p_object, const char *p_method, void *p_return_value, MCJavaMethodParams *p_params, bool p_on_engine_thread)
{
	MCAndroidEngineCallThreadContext t_context;
    t_context . object = p_object;
	t_context . method = p_method;
	t_context . return_value = p_return_value;
    t_context . params = p_params;

    if (p_on_engine_thread)
    {
        MCAndroidEngineCallThreadCallback(&t_context);
    }
    else
    {
        co_yield_to_android_and_call(MCAndroidEngineCallThreadCallback, &t_context);
    }
}

void MCAndroidObjectCall(jobject p_object, const char *p_method, const char *p_signature, void *p_return_value, ...)
{
    bool t_success = true;
	va_list args;

    MCJavaMethodParams *t_params = nil;

	va_start(args, p_return_value);
    t_success = MCJavaConvertParameters(s_java_env, p_signature, args, t_params);
	va_end(args);

    if (t_success)
        MCAndroidJavaMethodCall(p_object, p_method, p_return_value, t_params, true);

    MCJavaMethodParamsFree(s_java_env, t_params);
}

void MCAndroidObjectRemoteCall(jobject p_object, const char *p_method, const char *p_signature, void *p_return_value, ...)
{
    MCLog("MCAndroidObjectRemoteCall <%p>.%s(%s)", p_object, p_method, p_signature);
    bool t_success = true;
	va_list args;

    MCJavaMethodParams *t_params = nil;

	va_start(args, p_return_value);
    t_success = MCJavaConvertParameters(s_java_env, p_signature, args, t_params, true);
	va_end(args);

    if (t_success)
        MCAndroidJavaMethodCall(p_object, p_method, p_return_value, t_params, false);

    MCJavaMethodParamsFree(s_java_env, t_params, true);
}

void MCAndroidEngineCall(const char *p_method, const char *p_signature, void *p_return_value, ...)
{
    bool t_success = true;
	va_list args;

    MCJavaMethodParams *t_params = nil;

	va_start(args, p_return_value);
    t_success = MCJavaConvertParameters(s_java_env, p_signature, args, t_params);
	va_end(args);

    if (t_success)
        MCAndroidJavaMethodCall(s_android_view, p_method, p_return_value, t_params, true);

    MCJavaMethodParamsFree(s_java_env, t_params);
}

void MCAndroidEngineRemoteCall(const char *p_method, const char *p_signature, void *p_return_value, ...)
{
    bool t_success = true;
	va_list args;

    MCJavaMethodParams *t_params = nil;

	va_start(args, p_return_value);
    t_success = MCJavaConvertParameters(s_java_env, p_signature, args, t_params, true);
	va_end(args);

    if (t_success)
        MCAndroidJavaMethodCall(s_android_view, p_method, p_return_value, t_params, false);

    MCJavaMethodParamsFree(s_java_env, t_params, true);
}

void *MCAndroidGetActivity(void)
{
	return (void *)s_android_activity;
}

void *MCAndroidGetContainer(void)
{
	return (void *)s_android_container;
}

// MW-2013-06-14: [[ ExternalsApiV5 ]] Return the JavaEnv of the Android system
//   thread.
void *MCAndroidGetSystemJavaEnv(void)
{
	return s_android_ui_env;
}

// MW-2013-06-14: [[ ExternalsApiV5 ]] Return the JavaEnv of the engine's script
//   thread.
void *MCAndroidGetScriptJavaEnv(void)
{
	return s_java_env;
}

// MW-2013-07-25: [[ ExternalsApiV5 ]] Return the engine object (EngineApi really)
void *MCAndroidGetEngine(void)
{
	return s_android_view;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreate(JNIEnv *env, jobject object, jobject activity, jobject container, jobject view) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doDestroy(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRestart(JNIEnv *env, jobject object, jobject view) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doStart(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doStop(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPause(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doResume(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doLowMemory(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doProcess(JNIEnv *env, jobject object, bool timedout) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doWait(JNIEnv *env, jobject object, double time, bool dispatch, bool anyevent) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doReconfigure(JNIEnv *env, jobject object, int w, int h, jobject bitmap) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doTouch(JNIEnv *env, jobject object, int action, int id, int timestamp, int x, int y) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doKeyPress(JNIEnv *env, jobject object, int modifiers, int char_code, int key_code) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShake(JNIEnv *env, jobject object, int action, jlong timestamp) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPhotoPickerCanceled(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPhotoPickerDone(JNIEnv *env, jobject object, jbyteArray data, jint size) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPhotoPickerError(JNIEnv *env, jobject object, jstring error) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMailDone(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMailCanceled(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doBackPressed(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMenuKey(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doSearchKey(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doOrientationChanged(JNIEnv *env, jobject object, jint orientation) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doTextDone(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doTextCanceled(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMediaDone(JNIEnv *env, jobject object, jstring p_media_content) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMediaCanceled(JNIEnv *env, jobject object) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doKeyboardShown(JNIEnv *env, jobject object, int height) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doKeyboardHidden(JNIEnv *env, jobject object) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreate(JNIEnv *env, jobject object, jobject activity, jobject container, jobject view)
{
    MCInitialize();
    MCSInitialize();
    MCModulesInitialize();
    MCScriptInitialize();
    
	MCLog("doCreate called", 0);

	// Make sure the engine isn't running
	s_engine_running = false;

	// The android ui thread is this one
	s_android_ui_thread = pthread_self();

	// Initialize our mutex, condition and initial coroutine
	pthread_mutex_init(&s_coroutine_mutex, NULL);
	pthread_cond_init(&s_coroutine_condition, NULL);
	s_coroutine_thread = s_android_ui_thread;

	// Now we must create the engine thread, it will immediately yield.
	s_android_engine_thread = nil;
	if (pthread_create(&s_android_engine_thread, nil, mobile_main, nil) != 0)
	{
		s_engine_running = false;
		return;
	}

	// We now have an engine thread, and are running.
	s_engine_running = true;

	// The android activity - make sure we hold a global ref.
	s_android_activity = env -> NewGlobalRef(activity);
	MCLog("Got global android activity: %p\n", s_android_activity);
	
	// The android container - make sure we hold a global ref.
	s_android_container = env -> NewGlobalRef(container);
	MCLog("Got global android activity: %p\n", s_android_container);
	
	// The android view - make sure we hold a global ref.
	s_android_view = env -> NewGlobalRef(view);
	MCLog("Got global android view: %p\n", s_android_view);

	s_android_view_class = env -> NewGlobalRef( env -> GetObjectClass(view) );
	MCLog("Got global android view class reference: %p\n", s_android_view_class);

	// Now get the schedule wakeup method ID
	s_schedule_wakeup_method = env -> GetMethodID(env -> GetObjectClass(view), "scheduleWakeUp", "(IZ)V");
	MCLog("Got scheduleWakeUp method id: %p\n", s_schedule_wakeup_method);

	// Get the invalidate method ID
	s_invalidate_method = env -> GetMethodID(env -> GetObjectClass(view), "invalidateBitmap", "(IIII)V");
	MCLog("Got invalidate method id: %p\n", s_invalidate_method);

	// Clear the bitmap properties
	s_android_bitmap = nil;
	s_android_bitmap_width = 0;
	s_android_bitmap_height = 0;

	// Initialize bitmap mutex
	pthread_mutex_init(&s_android_bitmap_mutex, NULL);

	// Next we yield to engine which will run until the creation phase is done.
	MCLog("Yielding to engine thread to perform initialization phase", 0);
	co_yield_to_engine();
	MCLog("Engine has initialized", 0);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doDestroy(JNIEnv *env, jobject object)
{
	MCLog("doDestroy called", 0);

	if (!s_engine_running)
		return;

	s_engine_running = false;

	// IM-2013-05-01: [[ BZ 10586 ]] we should now only be called when the
	// engine thread is about to exit, so set s_engine_running to false & yield
	// so it can finish terminating
	MCLog("Yielding to engine thread to perform finalization phase", 0);
	co_yield_to_engine();

	// Finalize bitmap mutex
	pthread_mutex_destroy(&s_android_bitmap_mutex);

	// Free the global bitmap ref (if any)
	if (s_android_bitmap != nil)
	{
		env -> DeleteGlobalRef(s_android_bitmap);
		s_android_bitmap = nil;
	}

	// Free the global ref
	env -> DeleteGlobalRef(s_android_view);
	s_android_view = nil;
	env -> DeleteGlobalRef(s_android_view_class);
	s_android_view_class = nil;
	env -> DeleteGlobalRef(s_android_container);
	s_android_container = nil;
	env -> DeleteGlobalRef(s_android_activity);
	s_android_activity = nil;

	void *t_result;
	MCLog("Engine has finalized", 0);
	pthread_join(s_android_engine_thread, &t_result);
	s_android_engine_thread = nil;

	pthread_cond_destroy(&s_coroutine_condition);
	pthread_mutex_destroy(&s_coroutine_mutex);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRestart(JNIEnv *env, jobject object, jobject view)
{
	MCLog("doRestart called", 0);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doStart(JNIEnv *env, jobject object)
{
	MCLog("doStart called", 0);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doStop(JNIEnv *env, jobject object)
{
	MCLog("doStop called", 0);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPause(JNIEnv *env, jobject object)
{
	MCLog("doPause called", 0);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doResume(JNIEnv *env, jobject object)
{
	MCLog("doResume called", 0);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doLowMemory(JNIEnv *env, jobject object)
{
	MCLog("doLowMemory called", 0);
	static_cast<MCScreenDC *>(MCscreen) -> compact_memory();
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doProcess(JNIEnv *env, jobject object, bool timedout)
{
	// MW-2012-10-04: [[ Bug 10439 ]] If the engine thread isn't running, then do nothing.
	if (!s_engine_running)
		return;

	s_schedule_wakeup_was_broken = !timedout;
	co_yield_to_engine();
}

// MW-2013-08-07: [[ ExternalsApiV5 ]] Native implementation of the Engine 'doWait'
//   method - just calls MCScreenDC::wait().
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doWait(JNIEnv *env, jobject object, double time, bool dispatch, bool anyevent)
{
	if (!s_engine_running)
		return;
	
	MCscreen -> wait(time, dispatch, anyevent);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doReconfigure(JNIEnv *env, jobject object, int w, int h, jobject bitmap)
{
	MCLog("doReconfigure(%d, %d, %p)", w, h, bitmap);

	bool t_resizing_bitmap;
	t_resizing_bitmap = (s_android_bitmap != nil);

	if (s_android_bitmap != nil)
	{
		env -> DeleteGlobalRef(s_android_bitmap);
		s_android_bitmap = nil;
	}

	s_android_bitmap = env -> NewGlobalRef(bitmap);

	AndroidBitmapInfo t_info;
	AndroidBitmap_getInfo(env, bitmap, &t_info);
	s_android_bitmap_width = t_info . width;
	s_android_bitmap_height = t_info . height;
	s_android_bitmap_stride = t_info . stride;

	// MW-2011-10-01: [[ Bug 9772 ]] If we are resizing, we do a 'fit window', else
	//   we yield to engine.
	if (t_resizing_bitmap)
		static_cast<MCScreenDC *>(MCscreen) -> do_fit_window(false, true);
	else
		co_yield_to_engine();
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doTouch(JNIEnv *env, jobject object, int action, int id, int timestamp, int x, int y)
{
	MCEventTouchPhase t_phase;
	switch(action)
	{
		case 0: // DOWN
		case 5: // NON-PRIMARY DOWN
			t_phase = kMCEventTouchPhaseBegan;
			break;
		case 1: // UP
		case 6: // NON-PRIMARY UP
			t_phase = kMCEventTouchPhaseEnded;
			break;
		case 2: // MOVE
			t_phase = kMCEventTouchPhaseMoved;
			break;
		case 3: // CANCEL
			t_phase = kMCEventTouchPhaseCancelled;
			break;
		default:
			return;
	}

	MCPoint t_loc;
	t_loc = MCPointMake(x, y);
	
	// IM-2014-01-31: [[ HiDPI ]] Convert screen to logical coords
	t_loc = MCscreen -> screentologicalpoint(t_loc);
	
	// MW-2014-01-06: [[ Bug 11641 ]] Make sure we use 'id + 1' for the id as it needs to be non-zero
	//   (non-nil) for 'getmouse()'. (Android touch ids are 0 based).
	static_cast<MCScreenDC *>(MCscreen) -> handle_touch(t_phase, (void *)(id + 1), timestamp, t_loc.x, t_loc.y);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doKeyPress(JNIEnv *env, jobject object, int modifiers, int char_code, int key_code)
{
	//MCLog("doTouch(%d, %d, %d, %d, %d)", action, id, timestamp, x, y);
	static_cast<MCScreenDC *>(MCscreen) -> handle_key_press(modifiers, char_code, key_code);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShake(JNIEnv *env, jobject object, int action, jlong timestamp)
{
	MCLog("doShake(%d, %d)", action, timestamp);
	static_cast<MCScreenDC *>(MCscreen) -> handle_motion((MCEventMotionType)action, timestamp);
}

void MCAndroidPhotoPickCanceled();
void MCAndroidPhotoPickDone(const char *p_data, uint32_t p_size);
void MCAndroidPhotoPickError(const char *p_error);

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPhotoPickerCanceled(JNIEnv *env, jobject object)
{
	MCAndroidPhotoPickCanceled();
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPhotoPickerDone(JNIEnv *env, jobject object, jbyteArray data, jint size)
{
	jbyte *t_bytes = env->GetByteArrayElements(data, nil);
	MCAndroidPhotoPickDone((char *)t_bytes, size);
	env->ReleaseByteArrayElements(data, t_bytes, 0);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPhotoPickerError(JNIEnv *env, jobject object, jstring error)
{
	const char *t_err_str = nil;
	t_err_str = env->GetStringUTFChars(error, nil);
	MCAndroidPhotoPickError(t_err_str);
	env->ReleaseStringUTFChars(error, t_err_str);
}

void MCAndroidMailDone();
void MCAndroidMailCanceled();

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMailDone(JNIEnv *env, jobject object)
{
	MCAndroidMailDone();
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMailCanceled(JNIEnv *env, jobject object)
{
	MCAndroidMailCanceled();
}

void MCAndroidBackPressed();

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doBackPressed(JNIEnv *env, jobject object)
{
	MCAndroidBackPressed();
}

void MCAndroidMenuKey();

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMenuKey(JNIEnv *env, jobject object)
{
	MCAndroidMenuKey();
}

void MCAndroidSearchKey();

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doSearchKey(JNIEnv *env, jobject object)
{
	MCAndroidSearchKey();
}

void MCAndroidOrientationChanged(int orientation);

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doOrientationChanged(JNIEnv *env, jobject object, jint orientation)
{
	MCAndroidOrientationChanged(orientation);
}

//////////

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
		MCdefaultstackptr -> getcurcard() -> message(MCM_keyboard_deactivated);
	}
};


JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doKeyboardShown(JNIEnv *env, jobject object, int height)
{
	MCEventQueuePostCustom(new MCKeyboardActivatedEvent(height));
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doKeyboardHidden(JNIEnv *env, jobject object)
{
	MCEventQueuePostCustom(new MCKeyboardDeactivatedEvent);
}

//////////

void MCAndroidTextDone();
void MCAndroidTextCanceled();

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doTextDone(JNIEnv *env, jobject object)
{
	MCAndroidTextDone();
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doTextCanceled(JNIEnv *env, jobject object)
{
	MCAndroidTextCanceled();
}

void MCAndroidMediaDone(MCStringRef s_media_content);
void MCAndroidMediaCanceled();

static MCStringRef s_media_content = nil;

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMediaDone(JNIEnv *env, jobject object, jstring p_media_content)
{
	MCLog("doMediaDone called - passing arg", 0);

    if (s_media_content != nil)
        MCValueRelease(s_media_content);
    s_media_content = nil;
    
    if (p_media_content != nil)
	{
		MCJavaStringToStringRef(env, p_media_content, s_media_content);
	}

	MCAndroidMediaDone(s_media_content);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMediaCanceled(JNIEnv *env, jobject object)
{
	MCAndroidMediaCanceled();
}

void MCNotificationPostUrlWakeUp(MCStringRef t_url);

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doLaunchFromUrl(JNIEnv *env, jobject object, jstring url) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doLaunchFromUrl(JNIEnv *env, jobject object, jstring url)
{
	MCAutoStringRef t_url_str;
    if (MCJavaStringToStringRef(env, url, &t_url_str))
        MCNotificationPostUrlWakeUp(*t_url_str);
}
////////////////////////////////////////////////////////////////////////////////

static jmethodID s_get_asset_info_method = 0;

bool revandroid_getAssetOffsetAndLength(JNIEnv *env, jobject object, const char *p_filename, int32_t& r_offset, int32_t& r_length)
{
	if (s_get_asset_info_method == 0)
		s_get_asset_info_method = env -> GetMethodID(env -> GetObjectClass(object), "getAssetInfo", "(Ljava/lang/String;I)I");

	jstring t_filename_string;
	t_filename_string = env -> NewStringUTF(p_filename);

	int32_t t_offset, t_length;
	t_offset = env -> CallIntMethod(object, s_get_asset_info_method, t_filename_string, 0);
	t_length = env -> CallIntMethod(object, s_get_asset_info_method, t_filename_string, 1);

	env -> DeleteLocalRef(t_filename_string);

	if (t_offset == -1 || t_length == -1)
		return false;

	r_offset = t_offset;
	r_length = t_length;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool revandroid_loadExternalLibrary(MCStringRef p_external, MCStringRef &r_path)
{
	MCAndroidEngineRemoteCall("loadExternalLibrary", "xx", &r_path, p_external);
	return r_path != nil;
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidGetBuildInfo(MCStringRef p_key, MCStringRef& r_value)
{
	MCAndroidEngineCall("getBuildInfo", "xx", &r_value, p_key);

	if (r_value == nil)
		return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	kMCBuildInfoKeyManufacturer,
	kMCBuildInfoKeyModel,
	kMCBuildInfoKeyDevice,
	kMCBuildInfoKeyVersionRelease,
	kMCBuildInfoKeyVersionIncremental,

	kMCBuildInfoKeyCount
} MCAndroidBuildInfoKey;

static const char *s_build_keys[] = {
	"MANUFACTURER",
	"MODEL",
	"DEVICE",
	"VERSION.RELEASE",
	"VERSION.INCREMENTAL",
};

static MCStringRef *s_build_info = NULL;

MCAndroidDeviceConfiguration s_device_configuration = {
	false,
	{0,0,0,0}
};

bool MCAndroidInitBuildInfo()
{
	if (s_build_info != NULL)
		return true;

	bool t_success = true;
	MCStringRef *t_build_info = NULL;

	uint32_t t_key_count;
	t_key_count = kMCBuildInfoKeyCount;

	if (t_success)
		t_success = MCMemoryNewArray(t_key_count, t_build_info);

	for (uint32_t i = 0; i < t_key_count && t_success; i++)
	{
		t_success = MCAndroidGetBuildInfo(MCSTR(s_build_keys[i]), t_build_info[i]);
	}

	if (t_success)
		s_build_info = t_build_info;
	else
	{
		if (t_build_info != NULL)
		{
			for (uint32_t i = 0; i < t_key_count; i++)
			{
				MCValueRelease(t_build_info[i]);
			}
			MCMemoryDeleteArray(t_build_info);
		}
	}

	return t_success;
}

void MCAndroidFinalizeBuildInfo()
{
    if (s_build_info != NULL)
    {
        uint32_t t_count;
        t_count = kMCBuildInfoKeyCount;
        for (uint32_t i = 0; i < t_count; i++)
        {
            MCValueRelease(s_build_info[i]);
        }
        MCMemoryDeleteArray(s_build_info);
        // AL-2014-10-08: [[ Bug 13626 ]] Set s_build_info back to NULL when finalizing
        s_build_info = NULL;
    }
}

bool MCAndroidSignatureMatch(const char *p_signature)
{
    MCAutoStringRef t_signature;
    MCAutoArrayRef t_signature_array;
    /* UNCHECKED */ MCStringCreateWithCString(p_signature, &t_signature);
    /* UNCHECKED */ MCStringSplit(*t_signature, MCSTR("|"), nil, kMCCompareExact, &t_signature_array);
    uindex_t t_count;
	t_count = MCArrayGetCount(*t_signature_array);
	for (uindex_t i = 0; i < t_count; i++)
	{
		MCValueRef t_val;
        /* UNCHECKED */ MCArrayFetchValueAtIndex(*t_signature_array, i + 1, t_val);
        MCStringRef t_val_str = (MCStringRef)t_val;
        MCLog("testing component (%@)", t_val_str);
        if (!MCStringIsEqualTo(t_val_str, s_build_info[i], kMCCompareCaseless))
            return false;
    }
    return true;
}

bool MCAndroidSetOrientationMap(int p_map[4], const char *p_mapping)
{
	MCLog("MCAndroidSetOrientationMap(%s)", p_mapping);
	bool t_success = true;

	int16_t t_portrait, t_landscape;

	t_success = MCU_stoi2x2(MCString(p_mapping), t_portrait, t_landscape);

	if (t_success)
		t_success = (t_portrait == 0) || (t_portrait == 90) || (t_portrait == 180) || (t_portrait == 270);
	if (t_success)
		t_success = (t_landscape == 0) || (t_landscape == 90) || (t_landscape == 180) || (t_landscape == 270);

	if (t_success)
		t_success = (t_landscape % 180) != (t_portrait % 180);

	if (t_success)
	{
		p_map[0] = t_portrait;
		p_map[1] = (t_landscape + 180) % 360;
		p_map[2] = (t_portrait + 180) % 360;
		p_map[3] = t_landscape;
	}
	return t_success;
}

// called on init, check for existance of device configuration file and read in
// device-specific orientation mappings
//
// identify device / android version by:
//    MANUFACTURER|MODEL|DEVICE|VERSION.RELEASE|VERSION.INCREMENTAL

extern MCStringRef MCcmd;

bool MCAndroidLoadDeviceConfiguration()
{
	bool t_success = true;
	bool t_this_device = false;

	MCAndroidDeviceConfiguration *t_configuration = NULL;

	MCAutoStringRef t_config_file_path;

	uint32_t t_filesize;
	t_filesize = 0;

	IO_handle t_filehandle = NULL;
	char *t_file_buffer = NULL;

	char ** t_lines = NULL;
	uint32_t t_line_count = 0;

	if (t_success)
		t_success = MCAndroidInitBuildInfo();

	if (t_success)
		t_success = MCStringFormat(&t_config_file_path, "%@/lc_device_config.txt", MCcmd);

	if (t_success)
	{
        t_filehandle = MCS_open(*t_config_file_path, kMCOpenFileModeRead, false, false, 0);
		t_success = t_filehandle != NULL;
	}

	if (t_success)
	{
		t_filesize = MCS_fsize(t_filehandle);
		// +1 for terminating null byte
		t_success = MCMemoryAllocate(t_filesize + 1, t_file_buffer);
	}

	if (t_success)
        if (MCS_readfixed(t_file_buffer, t_filesize, t_filehandle) != IO_NORMAL)
            t_success = false;

	if (t_success)
	{
		t_file_buffer[t_filesize] = '\0';
		t_success = MCCStringSplit(t_file_buffer, '\n', t_lines, t_line_count);
	}

	if (t_success)
	{
		for (uint32_t i = 0; i < t_line_count; i++)
		{
			// check for CRLF line endings
			uint32_t t_line_length = MCCStringLength(t_lines[i]);
			if (t_line_length > 0 && t_lines[i][t_line_length - 1] == '\r')
				t_lines[i][t_line_length - 1] = '\0';

			if (MCCStringBeginsWith(t_lines[i], "device="))
			{
				t_this_device = (MCAndroidSignatureMatch(t_lines[i] + 7));
			}
			else if (t_this_device)
			{
				if (MCCStringBeginsWith(t_lines[i], "orientation_map="))
				{
					if (MCAndroidSetOrientationMap(s_device_configuration.orientation_map, t_lines[i] + 16))
						s_device_configuration.have_orientation_map = true;
				}
			}
		}
	}

	if (t_lines != NULL)
	{
		for (uint32_t i = 0; i < t_line_count; i++)
			MCCStringFree(t_lines[i]);
		MCMemoryDeallocate(t_lines);
	}
	if (t_filehandle != NULL)
		MCS_close(t_filehandle);
	if (t_file_buffer != NULL)
		MCMemoryDeallocate(t_file_buffer);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_OpenGLView_doSurfaceCreated(JNIEnv *env, jobject object, jobject view) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_OpenGLView_doSurfaceDestroyed(JNIEnv *env, jobject object, jobject view) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_OpenGLView_doSurfaceChanged(JNIEnv *env, jobject object, jobject view) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_OpenGLView_doSurfaceCreated(JNIEnv *env, jobject object, jobject p_view)
{
	MCLog("doSurfaceCreated called", 0);

	// Get the openglview methods
	if (s_openglview_start_method == nil)
	{
		jclass t_view_class;
		t_view_class = env -> GetObjectClass(p_view);
		s_openglview_start_method = env -> GetMethodID(t_view_class, "start", "()V");
		s_openglview_finish_method = env -> GetMethodID(t_view_class, "finish", "()V");
		s_openglview_configure_method = env -> GetMethodID(t_view_class, "configure", "()V");
		s_openglview_swap_method = env -> GetMethodID(t_view_class, "swap", "()V");
	}
}

static void doSurfaceDestroyedCallback(void *)
{
	// Make sure we do a full redraw when we get the surface back. We must do this
	// before we finish the OpenGL context as we could make OpenGL calls at this
	// point.
	if (s_android_opengl_enabled)
		MCRedrawDirtyScreen();

	// Discard all the OpenGL state.
	s_java_env -> CallVoidMethod(s_android_opengl_view, s_openglview_finish_method);

	// We have no surface at the moment, so disable screen updates.
	if (s_android_opengl_visible)
		MCRedrawDisableScreenUpdates();
}

JNIEXPORT void JNICALL Java_com_runrev_android_OpenGLView_doSurfaceDestroyed(JNIEnv *env, jobject object, jobject p_view)
{
	MCLog("doSurfaceDestroyed called", 0);

	co_yield_to_engine_and_call(doSurfaceDestroyedCallback, nil);

	env -> DeleteGlobalRef(s_android_opengl_view);
	s_android_opengl_view = nil;
}

static void doSurfaceChangedCallback(void *p_is_init)
{
	bool t_is_init;
	t_is_init = (bool)p_is_init;

	// If the view is nil, then this must be an initializing change.
	if (t_is_init)
		s_java_env -> CallVoidMethod(s_android_opengl_view, s_openglview_start_method);

	// Make sure we have a valid surface.
	s_java_env -> CallVoidMethod(s_android_opengl_view, s_openglview_configure_method);

	// We can now re-enable screen updates.
	MCRedrawEnableScreenUpdates();

	// Force a screen redraw
	MCRedrawUpdateScreen();
}

JNIEXPORT void JNICALL Java_com_runrev_android_OpenGLView_doSurfaceChanged(JNIEnv *env, jobject object, jobject p_view)
{
	MCLog("doSurfaceChanged called", 0);

	bool t_is_init;
	t_is_init = false;

	if (s_android_opengl_view == nil)
	{
		t_is_init = true;
		s_android_opengl_view = env -> NewGlobalRef(p_view);
	}

	co_yield_to_engine_and_call(doSurfaceChangedCallback, (void *)t_is_init);
}

void MCAndroidEnableOpenGLMode(void)
{
	if (s_android_opengl_enabled)
		return;

	MCRedrawDisableScreenUpdates();

	MCAndroidEngineRemoteCall("enableOpenGLView", "v", nil);

	s_android_opengl_enabled = true;
	s_android_opengl_visible = false;
}

void MCAndroidDisableOpenGLMode(void)
{
	if (!s_android_opengl_enabled)
		return;

	s_android_opengl_enabled = false;
	s_android_opengl_visible = false;

	MCAndroidEngineRemoteCall("disableOpenGLView", "v", nil);

	MCRedrawEnableScreenUpdates();
}

////////////////////////////////////////////////////////////////////////////////

bool android_run_on_main_thread(void *p_callback, void *p_callback_state, int p_options);

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
	// Call the callback on the UI thread (Android only at the moment).
	kMCExternalRunOnMainThreadJumpToUI = 1 << 4,
	// Call the callback on the Engine thread (Android only at the moment).
	kMCExternalRunOnMainThreadJumpToEngine = 2 << 4,
};

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

class MCRunOnMainThreadHelper
{
public:
	MCRunOnMainThreadHelper(void *p_callback, void *p_callback_context, int p_options)
	{
		m_callback = p_callback;
		m_callback_context = p_callback_context;
		m_options = p_options;
	}
	
	void Dispatch(void)
	{
		android_run_on_main_thread(m_callback, m_callback_context, m_options);
	}
	
	static void DispatchThunk(void *self)
	{
		((MCRunOnMainThreadHelper *)self) -> Dispatch();
		delete ((MCRunOnMainThreadHelper *)self);
	}
	
private:
	void *m_callback;
	void *m_callback_context;
	int m_options;
};

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doNativeNotify(JNIEnv *env, jobject object, int p_callback, int p_context) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doNativeNotify(JNIEnv *env, jobject object, int p_callback, int p_context)
{
	co_yield_to_engine_and_call((co_yield_callback_t)p_callback, (void *)p_context);
}

bool android_run_on_main_thread(void *p_callback, void *p_callback_state, int p_options)
{
	pthread_t t_current_thread;
	t_current_thread = pthread_self();
	
	// If this is a jump, then handle things differently.
	if ((p_options & (kMCExternalRunOnMainThreadJumpToUI | kMCExternalRunOnMainThreadJumpToEngine)) != 0)
	{
		if ((p_options & ~(kMCExternalRunOnMainThreadJumpToUI | kMCExternalRunOnMainThreadJumpToEngine)) != 0)
			return false;
		
		if ((p_options & kMCExternalRunOnMainThreadJumpToUI) != 0)
		{
			if (t_current_thread == s_android_ui_thread)
				((co_yield_callback_t)p_callback)(p_callback_state);
			else
				co_yield_to_android_and_call((co_yield_callback_t)p_callback, p_callback_state);
		}
		else
		{
			if (t_current_thread == s_android_engine_thread)
				((co_yield_callback_t)p_callback)(p_callback_state);
			else
				co_yield_to_engine_and_call((co_yield_callback_t)p_callback, p_callback_state);
		}
		
		return true;
	}
	
	// If the current thread is not the engine thread, then we must poke
	// the main thread (i.e. we are on a non-main thread).
	if (t_current_thread != s_android_engine_thread && t_current_thread != s_android_ui_thread)
	{
		if ((p_options & kMCExternalRunOnMainThreadPost) == 0)
		{
			__android_log_print(ANDROID_LOG_INFO, "LiveCode", "RunOnMainThread send from non-main thread not implemented.");
			abort();
			return false;
		}
		
		MCRunOnMainThreadHelper *t_helper;
		t_helper = new MCRunOnMainThreadHelper(p_callback, p_callback_state, p_options);
		MCAndroidEngineCall("nativeNotify", "(II)V", nil, MCRunOnMainThreadHelper::DispatchThunk, t_helper);
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
		
		MCRunOnMainThreadHelper *t_helper;
		t_helper = new MCRunOnMainThreadHelper(p_callback, p_callback_state, p_options & ~kMCExternalRunOnMainThreadPost);
		MCAndroidEngineCall("nativeNotify", "vii", nil, MCRunOnMainThreadHelper::DispatchThunk, t_helper);
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

extern "C" JNIEXPORT jstring JNICALL Java_com_runrev_android_Engine_doGetCustomPropertyValue(JNIEnv *env, jobject object, jstring set, jstring property) __attribute__((visibility("default")));
JNIEXPORT jstring JNICALL Java_com_runrev_android_Engine_doGetCustomPropertyValue(JNIEnv *env, jobject object, jstring set, jstring property)
{
    bool t_success = true;

    jstring t_js = nil;

    MCAutoStringRef t_property, t_set;

    t_success = MCJavaStringToStringRef(env, set, &t_set) && MCJavaStringToStringRef(env, property, &t_property);

    MCNewAutoNameRef t_set_name, t_prop_name;
    if (t_success)
    {
        MCNameCreate(*t_set, &t_set_name);
        MCNameCreate(*t_property, &t_prop_name);
    }

    MCExecValue t_value;
    MCExecContext ctxt(nil, nil, nil);
    
    if (MCdefaultstackptr -> getcustomprop(ctxt, *t_set_name, *t_prop_name, t_value))
    {
        MCAutoStringRef t_string_value;
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypeStringRef, &(&t_string_value));

        if (!ctxt . HasError())
            t_success = MCJavaStringFromStringRef(env, *t_string_value, t_js);
    }

    return t_js;
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidInitEngine()
{
    MCAndroidEngineCall("onAppLaunched", "v", nil);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) __attribute__((visibility("default")));

// This is called when our native bundle is loaded. We use it to cache the JavaVM
// pointer.
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
	s_java_vm = vm;
	vm -> GetEnv((void **)&s_android_ui_env, JNI_VERSION_1_2);

	return JNI_VERSION_1_2;
}

JNIEnv *MCJavaGetThreadEnv()
{
    JNIEnv *t_env = nil;
    s_java_vm->GetEnv((void**)&t_env, JNI_VERSION_1_2);
    return t_env;
}

////////////////////////////////////////////////////////////////////////////////

// No theming for mobile platforms yet
bool MCPlatformGetControlThemePropBool(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, bool&)
{
    return false;
}

bool MCPlatformGetControlThemePropInteger(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, int&)
{
    return false;
}

bool MCPlatformGetControlThemePropColor(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, MCColor&)
{
    return false;
}

bool MCPlatformGetControlThemePropFont(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, MCFontRef& r_font)
{
    return MCFontCreate(MCNAME("Arial"), 0, 12, r_font);
}
