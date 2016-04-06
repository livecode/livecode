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

MCUIDC *MCCreateScreenDC(void);

extern "C" MCStack *MCEmscriptenGetCurrentStack(void);

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
	virtual bool platform_getwindowgeometry(Window p_window,
	                                        MCRectangle & r_rect);

	/* ---------- Event loop */
	virtual Boolean wait(real64_t p_duration,
	                     Boolean p_allow_dispatch,
	                     Boolean p_allow_anyevent);

	/* ---------- Extra stuff */

	/* Get the stack being shown in the default canvas. */
	MCStack *GetCurrentStack();
    
    /* Pops up simple ask/answer dialogues */
    virtual int32_t popupanswerdialog(MCStringRef *p_buttons, uint32_t p_button_count, uint32_t p_type, MCStringRef p_title, MCStringRef p_message, bool p_blocking);
    virtual bool popupaskdialog(uint32_t p_type, MCStringRef p_title, MCStringRef p_message, MCStringRef p_initial, bool p_hint, MCStringRef& r_result);

    /* Mouse management */
    virtual void platform_querymouse(int16_t& r_x, int16_t& r_y);
    
protected:
	void UpdateFocus();
	void FitWindow();

private:
	Window m_main_window;
};

#endif /* ! __MC_EMSCRIPTEN_DC_H__ */
