/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "em-dc.h"
#include "em-javascript.h"
#include "em-view.h"
#include "em-async.h"
#include "em-event.h"
#include "em-util.h"
#include "em-liburl.h"

#include "osspec.h"
#include "eventqueue.h"
#include "redraw.h"
#include "dispatch.h"
#include "globals.h"
#include "graphics_util.h"

/* ================================================================
 * Helper Functions
 * ================================================================ */

MCRectangle MCEmscriptenGetWindowRect(uint32_t p_window_id)
{
	uint32_t t_left, t_top, t_right, t_bottom;
	MCEmscriptenGetWindowRect(p_window_id, &t_left, &t_top, &t_right, &t_bottom);
	
	return MCRectangleMake(t_left, t_top, t_right - t_left, t_bottom - t_top);
}

void MCEmscriptenSetWindowRect(uint32_t p_window_id, const MCRectangle &p_rect)
{
	MCEmscriptenSetWindowRect(p_window_id, p_rect.x, p_rect.y, p_rect.x + p_rect.width, p_rect.y + p_rect.height);
}

MCRectangle MCEmscriptenGetDisplayRect()
{
	uint32_t t_left, t_top, t_right, t_bottom;
	MCEmscriptenGetDisplayRect(&t_left, &t_top, &t_right, &t_bottom);
	
	return MCRectangleMake(t_left, t_top, t_right - t_left, t_bottom - t_top);
}

/* ================================================================
 * Initialization / Finalization
 * ================================================================ */

extern "C" bool MCEmscriptenDCInitializeJS();
bool MCEmscriptenDCInitialize()
{
	return MCEmscriptenDCInitializeJS();
}

extern "C" void MCEmscriptenDCFinalizeJS();
void MCEmscriptenDCFinalize()
{
	MCEmscriptenDCFinalizeJS();
}

/* ================================================================
 * Construction/Destruction
 * ================================================================ */

MCUIDC *
MCCreateScreenDC()
{
	return new MCScreenDC;
}

MC_DLLEXPORT_DEF MCStack *
MCEmscriptenGetStackForWindow(Window p_window)
{
	if (MCnoui) return nil;
	
	MCStack *t_stack = MCdispatcher->findstackd(p_window);
	
	return t_stack;
}

MC_DLLEXPORT_DEF
bool MCEmscriptenHandleMousePress(MCStack *p_stack, uint32_t p_time, uint32_t p_modifiers, MCMousePressState p_state, int32_t p_button)
{
	if (MCnoui) return false;
	
	MCScreenDC *t_dc = static_cast<MCScreenDC *>(MCscreen);
	t_dc->update_mouse_press_state(p_state, p_button);
	
	MCEventQueuePostMousePress(p_stack, p_time, p_modifiers, p_state, p_button);
	
	return true;
}

static inline MCPoint MCEmscriptenWindowToGlobalLoc(MCStack *p_stack, const MCPoint &p_loc)
{
	MCRectangle t_window_rect = p_stack->view_getrect();
	return MCPointOffset(p_loc, t_window_rect.x, t_window_rect.y);
}

MC_DLLEXPORT_DEF
bool MCEmscriptenHandleMousePosition(MCStack *p_stack, uint32_t p_time, uint32_t p_modifiers, int32_t p_x, int32_t p_y)
{
	if (MCnoui) return false;
	
	MCScreenDC *t_dc = static_cast<MCScreenDC *>(MCscreen);
	
	MCPoint t_position = MCPointMake(p_x, p_y);
	if (p_stack)
		t_position = MCEmscriptenWindowToGlobalLoc(p_stack, t_position);
	if (t_dc->update_mouse_position(t_position))
		MCEventQueuePostMousePosition(p_stack, p_time, p_modifiers, p_x, p_y);
	
	return true;
}

MCScreenDC::MCScreenDC()
	: m_main_window(nil), m_mouse_button_state(0)
{
	m_mouse_position = MCPointMake(-1,-1);
}

MCScreenDC::~MCScreenDC()
{
}

Boolean
MCScreenDC::open()
{
	return
		MCEmscriptenJSInitialize() &&
		MCEmscriptenEventInitialize() &&
		MCEmscriptenViewInitialize() &&
        MCEmscriptenLibUrlInitialize() &&
        MCEmscriptenDCInitialize();
}


Boolean
MCScreenDC::close(Boolean force)
{
	MCEmscriptenDCFinalize();
	MCEmscriptenViewFinalize();
	MCEmscriptenEventFinalize();
    MCEmscriptenLibUrlFinalize();
    MCEmscriptenJSFinalize();

	return true;
}

/* ================================================================
 * Window management
 * ================================================================ */

void
MCScreenDC::openwindow(Window p_window,
                       Boolean override)
{
	uint32_t t_window = reinterpret_cast<uint32_t>(p_window);
	MCLog("set window visible");
	MCEmscriptenSetWindowVisible(t_window, true);

	MCLog("find stack");
	MCStack *t_stack = MCdispatcher->findstackd(p_window);

	/* Enable drawing */
	MCLog("activate tilecache");
	t_stack->view_activatetilecache();

	t_stack->setextendedstate(false, ECS_DONTDRAW);

	/* Set up view to match window, as far as possible */
	/* FIXME Implement HiDPI support */

	MCEmscriptenSetWindowRect(t_window, t_stack->view_getrect());

	t_stack->view_configure(true);
	t_stack->dirtyall();
}

void
MCScreenDC::closewindow(Window p_window)
{
	MCAssert(p_window);

	uint32_t t_window = reinterpret_cast<uint32_t>(p_window);
	MCEmscriptenSetWindowVisible(t_window, false);
}

void
MCScreenDC::destroywindow(Window & x_window)
{
	uint32_t t_window = reinterpret_cast<uint32_t>(x_window);
	MCEmscriptenDestroyWindow(t_window);
	
	x_window = nil;
}

void
MCScreenDC::raisewindow(Window p_window)
{
	uint32_t t_window = reinterpret_cast<uint32_t>(p_window);
	MCEmscriptenRaiseWindow(t_window);
}

uintptr_t
MCScreenDC::dtouint(Drawable p_window)
{
	return reinterpret_cast<uint32_t>(p_window);
}

Boolean
MCScreenDC::uinttowindow(uintptr_t p_uint, Window &r_window)
{
	r_window = reinterpret_cast<Window>(p_uint);
	return True;
}

void *MCScreenDC::GetNativeWindowHandle(Window p_window)
{
    return (void*)dtouint(p_window);
}

bool
MCScreenDC::platform_getwindowgeometry(Window p_window,
                                       MCRectangle & r_rect)
{
	/* FIXME Implement HiDPI support */

	uint32_t t_window = reinterpret_cast<uint32_t>(p_window);
	r_rect = MCEmscriptenGetWindowRect(t_window);
	return true;
}

/* ================================================================
 * Display management
 * ================================================================ */

bool MCScreenDC::platform_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count)
{
	MCDisplay *t_display = nil;
	if (!MCMemoryNew(t_display))
		return false;
	
	t_display->viewport = t_display->workarea = MCEmscriptenGetDisplayRect();
	
	r_displays = t_display;
	r_count = 1;
	return true;
}

/* ================================================================
 * Event loop
 * ================================================================ */

// Important: This function is on the emterpreter whitelist. If its
// signature function changes, the mangled name must be updated in
// em-whitelist.json

/* Returns true if quit is requested, or from any inner main loop. */
Boolean
MCScreenDC::wait(real64_t p_duration,
                 Boolean p_allow_dispatch,
                 Boolean p_accept_any_event)
{
	/* Don't permit inner main loops.  They cause amusing "-12" assertion
	 * failures from Emterpreter. */
	if (0 < int(MCwaitdepth))
	{
		return true;
	}

	p_duration = MCMax(p_duration, 0.0);

	/* We allow p_duration to be infinite, but only if
	 * p_accept_any_event is set. */
	MCAssert(isfinite(p_duration) || p_accept_any_event);

	MCDeletedObjectsEnterWait(p_allow_dispatch);
	++MCwaitdepth;

	real64_t t_current_time = MCS_time();
	real64_t t_exit_time = t_current_time + p_duration;
	real64_t t_event_time = t_exit_time;

	bool t_first = true;
	bool t_done = false;

	while (!t_done && !MCquit)
	{
		/* If more than one iteration through the wait loop is needed
		 * (i.e. doing the work that was already queued didn't take
		 * long enough), wait for a timeout. */
		if (!t_first)
		{
			/* Update estimate of time remaining */
			t_current_time = MCS_time();

			real64_t t_sleep_time = MCMin(t_event_time - t_current_time,
			                              t_exit_time - t_current_time);

			/* Check if the requested wait duration has already
			 * elapsed */
			if (!t_done)
			{
				t_done = (t_sleep_time <= 0);
			}

			/* There's still some waiting required, so give up running the
			 * engine for now and wait for an event or timeout from the
			 * browser */
			if (!t_done &&
			    MCEmscriptenAsyncYield(t_sleep_time))
			{
				t_done = p_accept_any_event;
			}

			/* Update the times based on the result of the yield */
			t_current_time = MCS_time();
		}

		if (!t_done)
		{
			DoRunloopActions();
		}

		/* Dispatch pending engine messages */
		t_event_time = t_exit_time;

		if (!t_done &&
		    handlepending(t_current_time, t_event_time, p_allow_dispatch))
		{
			t_done = p_accept_any_event;
		}

		/* Dispatch queued incoming system events */
		if (!t_done &&
		    p_allow_dispatch && MCEventQueueDispatch())
		{
			t_done = p_accept_any_event;
		}

		/* Redraw the screen */
		MCRedrawUpdateScreen();

		t_first = false;
	}

	--MCwaitdepth;
	MCDeletedObjectsLeaveWait(p_allow_dispatch);

	return MCquit;
}

/* ================================================================
 * Ask/answer
 * ================================================================ */

// These functions are implemented in javascript
extern "C" int32_t MCEmscriptenDialogShowAlert(const unichar_t* p_message, size_t p_message_length);
extern "C" bool MCEmscriptenDialogShowConfirm(const unichar_t* p_message, size_t p_message_length);
extern "C" int32_t MCEmscriptenDialogShowPrompt(const unichar_t* p_message, size_t p_message_length, const unichar_t* p_default, size_t p_default_length, unichar_t** r_result, size_t* r_result_length);

int32_t
MCScreenDC::popupanswerdialog(MCStringRef *p_buttons, uint32_t p_button_count, uint32_t p_type, MCStringRef p_title, MCStringRef p_message, bool p_blocking)
{
    // Default to returning an unsuccessful result
    int32_t t_result = -1;
    
    // We need to have a UTF-16 string pointer for the message string. The other
    // parameters are not supported by the JavaScript built-in dialogues.
    MCAutoStringRefAsUTF16String t_message_u16;
    t_message_u16.Lock(p_message);
    
    switch (p_button_count)
    {
		case 0:
			// If no buttons specified, assume that an "OK" button is fine
        case 1:
            // Only one button - treat it as an "OK" button
            t_result = MCEmscriptenDialogShowAlert(t_message_u16.Ptr(), t_message_u16.Size());
            break;
            
        case 2:
            // Two buttons - treat it as an "OK"/"Cancel" button pair
            {
                int32_t t_ok_button = 0, t_cancel_button = 1;
                // check order of ok/cancel
                if (MCStringIsEqualToCString(p_buttons[1], "ok", kMCStringOptionCompareCaseless) ||
                    MCStringIsEqualToCString(p_buttons[0], "cancel", kMCStringOptionCompareCaseless))
                {
                    t_ok_button = 1;
                    t_cancel_button = 0;
                }
                
                if (MCEmscriptenDialogShowConfirm(t_message_u16.Ptr(), t_message_u16.Size()))
                    t_result = t_ok_button;
                else
                    t_result = t_cancel_button;
            }
            break;
            
        default:
            // Not supported
            break;
    }
    
    return t_result;
}

bool
MCScreenDC::popupaskdialog(uint32_t p_type, MCStringRef p_title, MCStringRef p_message, MCStringRef p_initial, bool p_hint, MCStringRef& r_result)
{
    MCAutoStringRefAsUTF16String t_message_u16;
    MCAutoStringRefAsUTF16String t_default_u16;
    t_message_u16.Lock(p_message);
    t_default_u16.Lock(p_initial);
    
    unichar_t* t_result;
    size_t t_result_length;
    if (!MCEmscriptenDialogShowPrompt(t_message_u16.Ptr(), t_message_u16.Size(), t_default_u16.Ptr(), t_default_u16.Size(), &t_result, &t_result_length))
    {
        return false;
    }
    
    MCStringCreateWithBytesAndRelease((byte_t*)t_result, t_result_length, kMCStringEncodingUTF16, false, r_result);
    return true;
}


void
MCScreenDC::update_mouse_press_state(MCMousePressState p_state, int32_t p_button)
{
	// track mouse button pressed state
	/* NOTE - assumes there are no more than 32 mouse buttons */
	if (p_button < 32)
	{
		if (p_state == kMCMousePressStateDown)
			m_mouse_button_state |= 1UL << p_button;
		else if (p_state == kMCMousePressStateUp)
			m_mouse_button_state &= ~(1UL << p_button);
	}
}

bool
MCScreenDC::update_mouse_position(const MCPoint &p_position)
{
	if (MCPointIsEqual(m_mouse_position, p_position))
		return false;
	
	m_mouse_position = p_position;
	
	return true;
}

Boolean
MCScreenDC::getmouse(uint2 p_button, Boolean& r_abort)
{
	// return recorded mouse button state
	if (p_button < 32)
		return (m_mouse_button_state & (1 << p_button)) != 0;
		
	return false;
}

void
MCScreenDC::platform_querymouse(int16_t& r_x, int16_t& r_y)
{
    // There is no asynchronous mouse position in Emscripten; just whatever the
    // browser has told us about.
    r_x = m_mouse_position.x;
    r_y = m_mouse_position.y;
}
