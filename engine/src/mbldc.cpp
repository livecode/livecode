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
#include "parsedef.h"
#include "objdefs.h"

#include "globals.h"
#include "printer.h"
#include "stack.h"
#include "card.h"
#include "tilecache.h"
#include "eventqueue.h"
#include "notify.h"

#include "mbldc.h"

#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

// A utility class used to manage a stack of windows.
class MCWindowStack
{
public:
	MCWindowStack(void)
	{
		m_windows = nil;
		m_window_count = 0;
	}
	
	~MCWindowStack(void)
	{
		free(m_windows);
	}
	
	bool Empty(void)
	{
		return m_window_count == 0;
	}
	
	Window Top(void)
	{
		if (m_window_count != 0)
			return m_windows[m_window_count - 1];
		return nil;
	}
	
	void Add(Window p_window)
	{
		Window *t_new_windows;
		t_new_windows = (Window *)realloc(m_windows, sizeof(Window) * (m_window_count + 1));
		if (t_new_windows != nil)
		{
			t_new_windows[m_window_count] = p_window;
			
			m_windows = t_new_windows;
			m_window_count += 1;
		}
	}
	
	void Remove(Window p_window)
	{
		for(uint32_t i = 0; i < m_window_count; i++)
			if (m_windows[i] == p_window)
			{
				memmove(m_windows + i, m_windows + i + 1, (m_window_count - i - 1) * sizeof(Window));
				m_window_count -= 1;
				return;
			}
	}
	
private:
	Window *m_windows;
	uint32_t m_window_count;
};

// The record that maps a touch to an id for all currently active touches.
struct MCActiveTouch
{
	MCActiveTouch *next;
	uint32_t ident;
	void *touch;
	int32_t x, y;
	uint32_t timestamp;
};

////////////////////////////////////////////////////////////////////////////////

MCScreenDC::MCScreenDC(void)
{
	// Initialize the window stacks.
	m_main_windows = new (nothrow) MCWindowStack;
	
	// Initialize the list of active touches.
	m_active_touches = nil;
}

MCScreenDC::~MCScreenDC(void)
{
	// Delete any lingering active touches (clear_touches does just the job)
	clear_touches();
	
	// Delete the main windows stack.
	delete m_main_windows;
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::common_open(void)
{
	// Initialize all the color related fields.
	redbits = greenbits = bluebits = 8;
	redshift = 16;
	greenshift = 8;
	blueshift = 0;
	
	black_pixel.red = black_pixel.green = black_pixel.blue = 0;
	white_pixel.red = white_pixel.green = white_pixel.blue = 0xFFFF;
	
	MCselectioncolor = MCpencolor = black_pixel;
	
	MConecolor = MCbrushcolor = white_pixel;
	
	gray_pixel.red = gray_pixel.green = gray_pixel.blue = 0x8080;
	
	MChilitecolor.red = MChilitecolor.green = 0x0000;
	MChilitecolor.blue = 0x8080;
	
	MCaccentcolor = MChilitecolor;
	
	background_pixel.red = background_pixel.green = background_pixel.blue = 0xC0C0;
	
	// Initialize the common vars.
	m_window_left = 0;
	m_window_top = 0;
	m_mouse_x = -100000;
	m_mouse_y = -100000;
	m_current_window = nil;
	m_current_focus = false;
	m_message_time = 0;
	m_last_touch_id = 0;
	m_mouse_touch = nil;
}

void MCScreenDC::compact_memory(void)
{
	if (m_current_window == nil)
		return;
	
	MCStack *t_stack;
	t_stack = (MCStack *)m_current_window;
	t_stack -> view_compacttilecache();
}

void MCScreenDC::handle_mouse_press(uint32_t p_time, uint32_t p_modifiers, int32_t x, int32_t y, int32_t p_button, MCMousePressState p_state)
{
	if (m_current_window == nil)
		return;
	
	if (m_mouse_x != x || m_mouse_y != y)
	{
		m_mouse_x = x;
		m_mouse_y = y;

		// IM-2014-01-30: [[ HiDPI ]] Mouse position should now be given in logical coords
		MCEventQueuePostMousePosition((MCStack *)m_current_window, p_time, p_modifiers, x, y);
	}
	
	MCEventQueuePostMousePress((MCStack *)m_current_window, p_time, p_modifiers, p_state, p_button);
}

void MCScreenDC::handle_mouse_move(uint32_t p_time, uint32_t p_modifiers, int32_t x, int32_t y)
{
	if (m_current_window == nil)
		return;
	
	if (m_mouse_x == x && m_mouse_y == y)
		return;
	
	m_mouse_x = x;
	m_mouse_y = y;

	// IM-2014-01-30: [[ HiDPI ]] Mouse position should now be given in logical coords
	MCEventQueuePostMousePosition((MCStack *)m_current_window, p_time, p_modifiers, x, y);
}

void MCScreenDC::handle_key_press(uint32_t p_modifiers, uint32_t p_char_code, uint32_t p_key_code)
{
	if (m_current_window == nil)
		return;
	
	MCStack *t_stack;
	t_stack = (MCStack *)m_current_window;
	
	// MW-2013-10-01: [[ Bug 11199 ]] If the char code is ASCII, then make the keycode
	//   match.
	if (p_char_code >= 32 && p_char_code < 128)
		p_key_code = p_char_code;
	
	MCEventQueuePostKeyPress(t_stack, p_modifiers, p_char_code, p_key_code);
}

void MCScreenDC::handle_key_focus(bool p_gain_focus)
{
	if (m_current_window == nil)
		return;
	
	if (m_current_focus == p_gain_focus)
		return;
	
	MCStack *t_stack;
	t_stack = (MCStack *)m_current_window;
	
	m_current_focus = p_gain_focus;
	MCEventQueuePostKeyFocus(t_stack, p_gain_focus);
}

void MCScreenDC::handle_motion(MCEventMotionType p_type, double p_timestamp)
{
	MCEventQueuePostMotion((MCStack *)m_current_window, p_type, (uint32_t)p_timestamp);
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::process_touch(MCEventTouchPhase p_phase, void *p_touch_handle, int32_t p_timestamp, int32_t p_x, int32_t p_y)
{
	/* It is possible for the engine to receive touch messages between initial startup and
	 * there being a stack shown, so ignore the touch if there is no current window. */
	if (m_current_window == nil)
	{
		return;
	}

	MCActiveTouch *t_touch, *t_previous_touch;
	t_previous_touch = nil;
	for(t_touch = m_active_touches; t_touch != nil; t_previous_touch = t_touch, t_touch = t_touch -> next)
		if (t_touch -> touch == p_touch_handle)
			break;
	
	if (p_phase == kMCEventTouchPhaseBegan)
	{
		if (t_touch == nil)
		{
			uint32_t t_touch_id;
			t_touch_id = ++m_last_touch_id;
			
			t_touch = new (nothrow) MCActiveTouch;
			t_touch -> ident = t_touch_id;
			t_touch -> touch = p_touch_handle;
			t_touch -> next = m_active_touches;
			m_active_touches = t_touch;
		}
		else
			return;
	}
	
	if (t_touch == nil)
		return;
	
	t_touch -> x = p_x;
	t_touch -> y = p_y;
	t_touch -> timestamp = p_timestamp;
	
	// IM-2014-01-30: [[ HiDPI ]] Touch position should now be given in logical coords
	MCEventQueuePostTouch((MCStack *)m_current_window, p_phase, t_touch -> ident, 1, p_x, p_y);
	
	if (p_phase == kMCEventTouchPhaseEnded || p_phase == kMCEventTouchPhaseCancelled)
	{
		if (t_previous_touch != nil)
			t_previous_touch -> next = t_touch -> next;
		else
			m_active_touches = t_touch -> next;
		
		delete t_touch;
	}
	
}

void MCScreenDC::cancel_touches(void)
{
	while(m_active_touches != nil)
		process_touch(kMCEventTouchPhaseCancelled, m_active_touches -> touch, m_active_touches -> timestamp, m_active_touches -> x, m_active_touches -> y);
}

void MCScreenDC::clear_touches(void)
{
	while(m_active_touches != nil)
	{
		MCActiveTouch *t_touch;
		t_touch = m_active_touches;
		m_active_touches = m_active_touches -> next;
		
		delete t_touch;
	}
	
	if (m_mouse_touch != nil)
		m_mouse_touch = nil;
}

void MCScreenDC::handle_touch(MCEventTouchPhase p_phase, void *p_touch, int32_t p_timestamp, int32_t p_x, int32_t p_y)
{
	switch(p_phase)
	{
		case kMCEventTouchPhaseBegan:
			if (m_mouse_touch == nil)
			{
				m_mouse_touch = p_touch;
				handle_mouse_press(p_timestamp, 0, p_x, p_y, 0, kMCMousePressStateDown);
			}
				
			process_touch(kMCEventTouchPhaseBegan, p_touch, p_timestamp, p_x, p_y);
			
			p_phase = kMCEventTouchPhaseMoved;
			break;
			
		case kMCEventTouchPhaseEnded:
			if (m_mouse_touch == p_touch)
			{
				handle_mouse_press(p_timestamp, 0, p_x, p_y, 0, kMCMousePressStateUp);
				m_mouse_touch = nil;
			}
			break;
	
		case kMCEventTouchPhaseMoved:
			if (m_mouse_touch == p_touch)
				handle_mouse_move(p_timestamp, 0, p_x, p_y);
			
			process_touch(kMCEventTouchPhaseMoved, p_touch, p_timestamp, p_x, p_y);
			break;
			
		case kMCEventTouchPhaseCancelled:
			if (m_mouse_touch == p_touch)
			{
				handle_mouse_press(p_timestamp, 0, p_x, p_y, 0, kMCMousePressStateRelease);
				m_mouse_touch = nil;
			}
			break;
			
		default:
			break;
	}
	
	process_touch(p_phase, p_touch, p_timestamp, p_x, p_y);
}

////////////////////////////////////////////////////////////////////////////////

bool MCScreenDC::fullscreenwindows(void)
{
	return true;
}

MCRectangle MCScreenDC::fullscreenrect(const MCDisplay *p_display)
{
	return p_display->workarea;
}

void MCScreenDC::openwindow(Window p_window, Boolean override)
{
	if (p_window == nil)
		return;
	
	open_window(p_window);
}

void MCScreenDC::closewindow(Window p_window)
{
	if (p_window == nil)
		return;
	
	close_window(p_window);
}

void MCScreenDC::destroywindow(Window &p_window)
{
	p_window = nil;
}

void MCScreenDC::raisewindow(Window p_window)
{
	if (p_window == nil)
		return;
	
	open_window(p_window);
}

void MCScreenDC::setname(Window p_window, MCStringRef p_new_name)
{
}

void MCScreenDC::iconifywindow(Window window)
{
}

void MCScreenDC::uniconifywindow(Window window)
{
}

void MCScreenDC::sync(Window w)
{
}

void MCScreenDC::setinputfocus(Window p_window)
{
	if (p_window != nil)
		focus_window(p_window);
}

void MCScreenDC::device_boundrect(MCRectangle &rect, Boolean title, Window_mode m)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::open_window(Window p_window)
{
	// If the window is already top, we are done
	if (m_main_windows -> Top() == p_window)
		return;
	
	// Add the entry to the top
	m_main_windows -> Remove(p_window);
	m_main_windows -> Add(p_window);
	
	refresh_window(p_window);
}

void MCScreenDC::close_window(Window p_window)
{
	bool t_is_top;
	t_is_top = m_main_windows -> Top() == p_window;
	
	m_main_windows -> Remove(p_window);
	
	if (t_is_top)
		refresh_window(m_main_windows -> Top());
}

void MCScreenDC::focus_window(Window p_window)
{
	if (m_current_window != p_window)
		return;
	
	do_take_focus();
	
	if (!m_current_focus)
		handle_key_focus(true);
}

void MCScreenDC::refresh_window(Window p_window)
{
	if (p_window != m_main_windows -> Top())
		return;
	
	if (p_window != m_current_window &&
		m_current_window != nil)
	{
		MCStack *t_old_stack;
		t_old_stack = (MCStack *)m_current_window;
		
		if (m_current_focus)
			MCEventQueuePostKeyFocus(t_old_stack, false);
		
		if (t_old_stack -> getstate(CS_MFOCUSED))
			MCEventQueuePostMouseFocus(t_old_stack, m_message_time, false);
		
		t_old_stack -> setextendedstate(true, ECS_DONTDRAW);
		
		// MW-2011-09-13: [[ TileCache ]] Deactivate old stack's tilecache.
		// MW-2013-06-26: [[ Bug 10990 ]] Method now implemented differently on iOS, due to
		//   the need to only do OpenGL calls on the main thread.
        t_old_stack -> deactivatetilecache();
		
		// IM-2016-08-17: [[ Bug 18100 ]] Detach old stack from window
		t_old_stack->OnDetach();
		
		m_current_window = nil;
		m_current_focus = false;
		m_mouse_x = -100000;
		m_mouse_y = -100000;
	}
	
	if (p_window != nil)
	{
		bool t_need_redraw;
		t_need_redraw = m_current_window != p_window;
		
		m_current_window = p_window;
		
		MCStack *t_new_stack;
		t_new_stack = (MCStack *)p_window;
		
		// MW-2011-09-13: [[ TileCache ]] Activate old stack's tilecache.
		t_new_stack -> view_activatetilecache();
		
		t_new_stack -> setextendedstate(false, ECS_DONTDRAW);
		
		if (!t_new_stack -> getstate(CS_MFOCUSED))
			MCEventQueuePostMouseFocus(t_new_stack, m_message_time, true);
		
		if (!m_current_focus)
			focus_window(p_window);
		
		// IM-2016-08-17: [[ Bug 18100 ]] Attach new stack to window
		t_new_stack->OnAttach();
		
#ifdef _IOS_MOBILE
		// MW-2012-03-05: [[ ViewStack ]] Make sure we tell the app's view
		//   which stack to use.
		do_show_stack_in_window(t_new_stack);
#endif
		
		do_fit_window(false, true);
		
		if (t_need_redraw)
			t_new_stack -> dirtyall();
	}
}

void MCScreenDC::redraw_current_window(void)
{
	MCStack *t_stack;
	t_stack = (MCStack *)m_current_window;
	if (t_stack != nil)
		t_stack -> dirtyall();
}

void MCScreenDC::unfocus_current_window(void)
{
	MCStack *t_stack;
	t_stack = (MCStack *)m_current_window;
	if (t_stack != nil)
		t_stack -> getcard() -> kunfocus();
}

////////////////////////////////////////////////////////////////////////////////

MCMobileBitmap *MCMobileBitmapCreate(uint32_t width, uint32_t height, bool mono)
{
	MCMobileBitmap *t_bitmap;
	t_bitmap = new (nothrow) MCMobileBitmap;
	t_bitmap -> width = width;
	t_bitmap -> height = height;
	if (mono)
		t_bitmap -> stride = ((width + 31) & ~31) / 8;
	else
		t_bitmap -> stride = width * 4;
	t_bitmap -> is_mono = mono;
	t_bitmap -> is_swapped = false;
	t_bitmap -> data = malloc(height * t_bitmap -> stride);
	memset(t_bitmap -> data, 0, height * t_bitmap -> stride);
	return t_bitmap;
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCScreenDC::getmouse(uint2 button, Boolean& r_abort)
{
	// Wait for 0 seconds to give a chance to collect system events (notice we
	// don't dispatch and break on any event).
	r_abort = wait(0.0, False, True);
	
	// Now whether there the 'mouse' is down or not depends on whether we have
	// a mouse touch.
	return m_mouse_touch != nil;
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::resetcursors(void)
{
}

void MCScreenDC::setcursor(Window p_window, MCCursorRef p_cursor)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::grabpointer(Window w)
{
}

void MCScreenDC::ungrabpointer(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::setcmap(MCStack *sptr)
{
}

void MCScreenDC::setgraphicsexposures(Boolean on, MCStack *sptr)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::copyarea(Drawable source, Drawable dest, int2 depth, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy, uint4 rop)
{	
	MCMobileBitmap *t_src_bitmap, *t_dst_bitmap;
	t_src_bitmap = (MCMobileBitmap *)source -> handle . pixmap;
	t_dst_bitmap = (MCMobileBitmap *)dest -> handle . pixmap;
	
    int32_t t_src_x = sx, t_src_y = sy;
    int32_t t_width = sw, t_height = sh;
    int32_t t_dst_x = dx, t_dst_y = dy;
    
    int32_t t_min_x = MCMin(t_src_x, t_dst_x);
    int32_t t_min_y = MCMin(t_src_y, t_dst_y);
    if (t_min_x < 0)
    {
        t_width += t_min_x;
        t_src_x -= t_min_x;
        t_dst_x -= t_min_x;
    }
    if (t_min_y < 0)
    {
        t_height += t_min_y;
        t_src_y -= t_min_y;
        t_dst_y -= t_min_y;
    }
    
    t_width = MCMin(t_width, MCMin((int32_t)t_src_bitmap->width - t_src_x, (int32_t)t_dst_bitmap->width - t_dst_x));
    t_height = MCMin(t_height, MCMin((int32_t)t_src_bitmap->height - t_src_y, (int32_t)t_dst_bitmap->height - t_dst_y));
    
    if (t_width <= 0 || t_height <= 0)
        return;
    
	assert(t_src_bitmap -> is_mono == t_dst_bitmap -> is_mono);
	assert(!t_dst_bitmap -> is_mono || (t_dst_x & 7) == 0 && (t_width & 7) == 0 && (t_src_x & 7) == 0);
	
	uint8_t *t_src_ptr, *t_dst_ptr;
	t_src_ptr = (uint8_t *)t_src_bitmap -> data;
	t_dst_ptr = (uint8_t *)t_dst_bitmap -> data;
	
	uint32_t t_src_stride, t_dst_stride;
	t_src_stride = t_src_bitmap -> stride;
	t_dst_stride = t_dst_bitmap -> stride;
	
	uint32_t t_src_depth, t_dst_depth;
	t_src_depth = t_src_bitmap -> is_mono ? 1 : 32;
	t_dst_depth = t_dst_bitmap -> is_mono ? 1 : 32;
	
	t_src_ptr += t_src_y * t_src_stride + t_src_x * t_src_depth / 8;
	t_dst_ptr += t_dst_y * t_dst_stride + t_dst_x * t_src_depth / 8;
	
	for(uint32_t i = 0; i < t_height; i++)
	{
		memcpy(t_dst_ptr, t_src_ptr, t_width * t_src_depth / 8);
		t_src_ptr += t_src_stride;
		t_dst_ptr += t_dst_stride;
	}
}

////////////////////////////////////////////////////////////////////////////////

MCCursorRef MCScreenDC::createcursor(MCImageBitmap *p_image, int2 p_hotspot_x, int2 p_hotspot_y)
{
	return NULL;
}

void MCScreenDC::freecursor(MCCursorRef c)
{
}

////////////////////////////////////////////////////////////////////////////////

uintptr_t MCScreenDC::dtouint(Drawable d)
{
	if (d == DNULL)
		return 0;
	
	return (uintptr_t)d -> handle . pixmap;
}

Boolean MCScreenDC::uinttowindow(uintptr_t, Window &w)
{
	return False;
}


////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::enablebackdrop(bool p_hard)
{
}

void MCScreenDC::disablebackdrop(bool p_hard)
{
}

void MCScreenDC::configurebackdrop(const MCColor& p_colour, MCPatternRef p_pattern, MCImage *p_badge)
{
}

void MCScreenDC::assignbackdrop(Window_mode p_mode, Window p_window)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::hidemenu()
{
}

void MCScreenDC::hidetaskbar()
{
}

void MCScreenDC::showmenu()
{
}

void MCScreenDC::showtaskbar()
{
}

////////////////////////////////////////////////////////////////////////////////

MCColor *MCScreenDC::getaccentcolors()
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::expose()
{
}

Boolean MCScreenDC::abortkey()
{
	return False;
}

void MCScreenDC::waitconfigure(Window w)
{
}

void MCScreenDC::waitreparent(Window w)
{
}

void MCScreenDC::waitfocus()
{
}

uint2 MCScreenDC::querymods()
{
	return 0;
}

void MCScreenDC::device_setmouse(int2 x, int2 y)
{
}

Boolean MCScreenDC::getmouseclick(uint2 button, Boolean& r_abort)
{
	return False;
}

void MCScreenDC::flushevents(uint2 e)
{
}

Boolean MCScreenDC::istripleclick()
{
	return False;
}

bool MCScreenDC::getkeysdown(MCListRef& r_list)
{
	r_list = MCValueRetain(kMCEmptyList);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

uint1 MCScreenDC::fontnametocharset(MCStringRef p_fontname)
{
	return 0;
}

/*
char *MCScreenDC::charsettofontname(uint1 charset, const char *oldfontname)
{
	const char *t_charset;
	t_charset = strchr(oldfontname, ',');
	if (t_charset != NULL)
	{
		char *t_result;
		t_result = strclone(oldfontname);
		t_result[t_charset - oldfontname] = '\0';
		return t_result;
	}
	return strclone(oldfontname);
}
*/

void MCScreenDC::clearIME(Window w)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::enactraisewindows(void)
{
}

void MCScreenDC::updatemenubar(Boolean force)
{
}

////////////////////////////////////////////////////////////////////////////////

class MCDummyPrinter: public MCPrinter
{
protected:
	void DoInitialize(void) { }
	void DoFinalize(void) { }
	
	bool DoReset(MCStringRef name) { return false; }
	bool DoResetSettings(MCDataRef settings) { return false; }
	void DoResync(void) {}
	
	const char *DoFetchName(void) { return NULL; }
	void DoFetchSettings(void*& r_buffer, uint32_t& r_length) { r_length = 0; r_buffer = NULL; }
	
	MCPrinterDialogResult DoPrinterSetup(bool p_window_modal, Window p_owner)  { return PRINTER_DIALOG_RESULT_ERROR; }
	MCPrinterDialogResult DoPageSetup(bool p_window_modal, Window p_owner) { return PRINTER_DIALOG_RESULT_ERROR; }
	MCPrinterResult DoBeginPrint(MCStringRef p_document, MCPrinterDevice*& r_device) { return PRINTER_RESULT_ERROR; }
	MCPrinterResult DoEndPrint(MCPrinterDevice* p_device) { return PRINTER_RESULT_ERROR; }
};

MCPrinter *MCScreenDC::createprinter(void)
{
	return new MCDummyPrinter;
}

////////////////////////////////////////////////////////////////////////////////

bool MCScreenDC::ownsselection(void)
{
	return false;
}

bool MCScreenDC::setselection(MCPasteboard *p_pasteboard)
{
	return false;
}

MCPasteboard *MCScreenDC::getselection(void)
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::flushclipboard(void)
{
}

bool MCScreenDC::ownsclipboard(void)
{
	return false;
}

bool MCScreenDC::setclipboard(MCPasteboard *p_pasteboard)
{
	return false;
}

MCPasteboard *MCScreenDC::getclipboard(void)
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

MCScriptEnvironment *MCScreenDC::createscriptenvironment(MCStringRef p_language)
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

uint16_t MCScreenDC::platform_getwidth()
{
	// IM-2014-01-30: [[ HiDPI ]] Convert screen to logical size
	return device_getwidth() / logicaltoscreenscale();
}

uint16_t MCScreenDC::platform_getheight()
{
	// IM-2014-01-30: [[ HiDPI ]] Convert screen to logical size
	return device_getheight() / logicaltoscreenscale();
}

void MCScreenDC::platform_querymouse(int16_t &r_x, int16_t &r_y)
{
	// IM-2014-03-03: [[ Bug 11836 ]] Mouse loc and window position now stored in logical coords
	r_x = m_mouse_x + m_window_left;
	r_y = m_mouse_y + m_window_top;
}

void MCScreenDC::platform_setmouse(int16_t p_x, int16_t p_y)
{
	MCPoint t_loc;
	t_loc = MCPointMake(p_x, p_y);
	
	// IM-2014-01-30: [[ HiDPI ]] Convert logical to screen coords
	t_loc = logicaltoscreenpoint(t_loc);
	
	device_setmouse(t_loc.x, t_loc.y);
}

void MCScreenDC::platform_boundrect(MCRectangle &rect, Boolean title, Window_mode m, Boolean resizable)
{
	MCRectangle t_rect;
	t_rect = rect;

	// IM-2014-01-30: [[ HiDPI ]] Convert logical to screen coords
	t_rect = logicaltoscreenrect(t_rect);
	
	device_boundrect(t_rect, title, m);
	
	// IM-2014-01-30: [[ HiDPI ]] Convert screen to logical coords
	t_rect = screentologicalrect(t_rect);
	
	rect = t_rect;
}

////////////////////////////////////////////////////////////////////////////////

MCPoint MCScreenDC::logicaltoscreenpoint(const MCPoint &p_point)
{
	MCGFloat t_scale;
	t_scale = logicaltoscreenscale();
	return MCPointTransform(p_point, MCGAffineTransformMakeScale(t_scale, t_scale));
}

MCPoint MCScreenDC::screentologicalpoint(const MCPoint &p_point)
{
	MCGFloat t_scale;
	t_scale = 1 / logicaltoscreenscale();
	return MCPointTransform(p_point, MCGAffineTransformMakeScale(t_scale, t_scale));
}

MCRectangle MCScreenDC::logicaltoscreenrect(const MCRectangle &p_rect)
{
	return MCRectangleGetScaledInterior(p_rect, logicaltoscreenscale());
}

MCRectangle MCScreenDC::screentologicalrect(const MCRectangle &p_rect)
{
	return MCRectangleGetScaledBounds(p_rect, 1 / logicaltoscreenscale());
}

////////////////////////////////////////////////////////////////////////////////
