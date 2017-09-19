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

/* ================================================================
 * Construction/Destruction
 * ================================================================ */

MCUIDC *
MCCreateScreenDC()
{
	return new MCScreenDC;
}

MC_DLLEXPORT_DEF MCStack *
MCEmscriptenGetCurrentStack()
{
	if (MCnoui) return nil;

	MCScreenDC *t_dc = static_cast<MCScreenDC *>(MCscreen);

	return t_dc->GetCurrentStack();
}

MCScreenDC::MCScreenDC()
	: m_main_window(nil)
{
}

MCScreenDC::~MCScreenDC()
{
}

Boolean
MCScreenDC::open()
{
	return
		MCEmscriptenEventInitialize() &&
		MCEmscriptenViewInitialize() &&
        MCEmscriptenLibUrlInitialize();
}


Boolean
MCScreenDC::close(Boolean force)
{
	MCEmscriptenViewFinalize();
	MCEmscriptenEventFinalize();
    MCEmscriptenLibUrlFinalize();

	return true;
}

/* ================================================================
 * Window management
 * ================================================================ */

void
MCScreenDC::openwindow(Window p_window,
                       Boolean override)
{
	/* FIXME Implement multiple windows */

	if (nil != m_main_window)
	{
		if (m_main_window == p_window) {
			return;
		}
		else
		{
			MCEmscriptenNotImplemented();
		}
	}

	m_main_window = p_window;

	MCStack *t_stack = MCdispatcher->findstackd(p_window);

	/* Enable drawing */
	t_stack->view_activatetilecache();

	t_stack->setextendedstate(false, ECS_DONTDRAW);

	/* Set mouse & keyboard focus */
	UpdateFocus();

	/* Set up view to match window, as far as possible */
	/* FIXME Implement HiDPI support */

	MCEmscriptenViewSetBounds(t_stack->view_getrect());

	t_stack->view_configure(true);
	t_stack->view_dirty_all();
}

void
MCScreenDC::closewindow(Window p_window)
{
	/* FIXME Implement multiple windows */

	MCAssert(p_window);

	if (p_window != m_main_window)
	{
		return;
	}

	m_main_window = nil;
}

void
MCScreenDC::destroywindow(Window & x_window)
{
	x_window = nil;
}

bool
MCScreenDC::platform_getwindowgeometry(Window p_window,
                                       MCRectangle & r_rect)
{
	/* FIXME Implement HiDPI support */

	r_rect = MCEmscriptenViewGetBounds();
	return true;
}

void
MCScreenDC::UpdateFocus()
{
	MCAssert(nil != m_main_window);

	MCStack *t_stack = GetCurrentStack();

	MCEventQueuePostMouseFocus(t_stack, 0, true);
	MCEventQueuePostKeyFocus(t_stack, true);
}

MCStack *
MCScreenDC::GetCurrentStack()
{
	return MCdispatcher->findstackd(m_main_window);
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
MCScreenDC::platform_querymouse(int16_t& r_x, int16_t& r_y)
{
    // There is no asynchronous mouse position in Emscripten; just whatever the
    // browser has told us about.
    r_x = MCmousex;
    r_y = MCmousey;
}
