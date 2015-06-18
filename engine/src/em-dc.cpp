/*                                                                     -*-c++-*-

Copyright (C) 2003-2013 Runtime Revolution Ltd.

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
#include "em-stack.h"
#include "em-view.h"
#include "em-async.h"
#include "em-util.h"

#include "osspec.h"
#include "eventqueue.h"
#include "redraw.h"

/* ================================================================
 * Construction/Destruction
 * ================================================================ */

MCUIDC *
MCCreateScreenDC()
{
	return new MCScreenDC;
}

MCScreenDC::MCScreenDC()
	: m_main_window(nil)
{
}

MCScreenDC::~MCScreenDC()
{
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

	MCStack *t_stack = MCEmscriptenGetStackForWindow(p_window);

	/* Enable drawing */
	t_stack->view_activatetilecache();

	t_stack->setextendedstate(false, ECS_DONTDRAW);

	/* Set mouse & keyboard focus */
	UpdateFocus();

	/* Fit window to view */
	FitWindow();
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
	return MCEmscriptenGetStackForWindow(m_main_window);
}

void
MCScreenDC::FitWindow()
{
	/* FIXME Implement HiDPI support */

	MCAssert(nil != m_main_window);

	MCStack * t_stack = GetCurrentStack();
	t_stack->view_configure(true);
	t_stack->view_dirty_all();
}

/* ================================================================
 * Event loop
 * ================================================================ */

/* Returns true if quit is requested. */
Boolean
MCScreenDC::wait(real64_t p_duration,
                 Boolean p_allow_dispatch,
                 Boolean p_accept_any_event)
{
	p_duration = MCMax(p_duration, 0.0);

	/* We allow p_duration to be infinite, but only if
	 * p_accept_any_event is set. */
	MCAssert(isfinite(p_duration) || p_accept_any_event);

	real64_t t_current_time = MCS_time();
	real64_t t_exit_time = t_current_time + p_duration;
	real64_t t_event_time = t_exit_time;

	bool t_first = true;
	bool t_reset = false;
	bool t_done = false;

	++MCwaitdepth;

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
		}

		if (!t_done)
		{
			DoRunloopActions();
		}

		/* Dispatch pending engine messages */
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

	return MCquit;
}
