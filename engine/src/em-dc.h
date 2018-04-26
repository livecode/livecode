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

#ifndef __MC_EMSCRIPTEN_DC_H__
#define __MC_EMSCRIPTEN_DC_H__

#include <foundation.h>

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "sysdefs.h"
#include "stack.h"
#include "uidc.h"
#include "eventqueue.h"

MCUIDC *MCCreateScreenDC(void);

extern bool MCEmscriptenDCInitialize();
extern void MCEmscriptenDCFinalize();

extern "C" void MCEmscriptenGetDisplayRect(uint32_t *r_left, uint32_t *r_top, uint32_t *r_right, uint32_t *r_bottom);

extern "C" MCStack *MCEmscriptenGetStackForWindow(Window p_window);
extern "C" bool MCEmscriptenHandleMousePress(MCStack *p_stack, uint32_t p_time, uint32_t p_modifiers, MCMousePressState p_state, int32_t p_button);
extern "C" bool MCEmscriptenHandleMousePosition(MCStack *p_stack, uint32_t p_time, uint32_t p_modifiers, int32_t p_x, int32_t p_y);

extern "C" uint32_t MCEmscriptenCreateWindow();
extern "C" void MCEmscriptenDestroyWindow(uint32_t p_window_id);
extern "C" void MCEmscriptenRaiseWindow(uint32_t p_window_id);
extern "C" void MCEmscriptenSetWindowRect(uint32_t p_window_id, uint32_t p_left, uint32_t p_top, uint32_t p_right, uint32_t p_bottom);
extern "C" void MCEmscriptenGetWindowRect(uint32_t p_window_id, uint32_t *r_left, uint32_t *r_top, uint32_t *r_right, uint32_t *r_bottom);
extern "C" void MCEmscriptenSetWindowVisible(uint32_t p_window_id, bool p_visible);
extern "C" bool MCEmscriptenGetWindowVisible(uint32_t p_window_id);

extern "C" void MCEmscriptenSyncCanvasSize(uint32_t p_window_id, uint32_t p_width, uint32_t p_height);

/* ---------------------------------------------------------------- */

MCRectangle MCEmscriptenGetWindowRect(uint32_t p_window_id);
void MCEmscriptenSetWindowRect(uint32_t p_window_id, const MCRectangle &p_rect);

/* ---------------------------------------------------------------- */

class MCScreenDC: public MCUIDC
{
public:
	/* ---------- Construction/destruction */
	MCScreenDC();
	virtual ~MCScreenDC();
	Boolean open();
	Boolean close(Boolean force);

	/* ---------- Window management */
	virtual void openwindow(Window p_window, Boolean override);
	virtual void closewindow(Window p_window);
	virtual void destroywindow(Window & x_window);
	virtual void raisewindow(Window p_window);
	virtual bool platform_getwindowgeometry(Window p_window,
	                                        MCRectangle & r_rect);

	uintptr_t dtouint(Drawable d);
	Boolean uinttowindow(uintptr_t, Window &w);
    void* GetNativeWindowHandle(Window p_window);

	/* ---------- Display management */

	virtual bool platform_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count);

	/* ---------- Event loop */
	virtual Boolean wait(real64_t p_duration,
	                     Boolean p_allow_dispatch,
	                     Boolean p_allow_anyevent);

	/* ---------- Extra stuff */

    /* Pops up simple ask/answer dialogues */
    virtual int32_t popupanswerdialog(MCStringRef *p_buttons, uint32_t p_button_count, uint32_t p_type, MCStringRef p_title, MCStringRef p_message, bool p_blocking);
    virtual bool popupaskdialog(uint32_t p_type, MCStringRef p_title, MCStringRef p_message, MCStringRef p_initial, bool p_hint, MCStringRef& r_result);

    /* Mouse management */
	void update_mouse_press_state(MCMousePressState p_state, int32_t p_button);
	bool update_mouse_position(const MCPoint &p_position); 

	virtual Boolean getmouse(uint2 button, Boolean& r_abort);
    virtual void platform_querymouse(int16_t& r_x, int16_t& r_y);
    
protected:
	void FitWindow();

private:
	Window m_main_window;
	uint32_t m_mouse_button_state;
	MCPoint m_mouse_position;
};

#endif /* ! __MC_EMSCRIPTEN_DC_H__ */
