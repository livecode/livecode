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
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "globals.h"
#include "printer.h"
#include "stack.h"
#include "card.h"
#include "tilecache.h"
#include "eventqueue.h"

#include "mbldc.h"


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
	m_main_windows = new MCWindowStack;
	
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
	black_pixel.pixel = 0;
	white_pixel.pixel = 0xFFFFFF;
	
	MCselectioncolor = MCpencolor = black_pixel;
	alloccolor(MCselectioncolor);
	alloccolor(MCpencolor);
	
	MConecolor = MCbrushcolor = white_pixel;
	alloccolor(MCbrushcolor);
	
	gray_pixel.red = gray_pixel.green = gray_pixel.blue = 0x8080;
	alloccolor(gray_pixel);
	
	MChilitecolor.red = MChilitecolor.green = 0x0000;
	MChilitecolor.blue = 0x8080;
	alloccolor(MChilitecolor);
	
	MCaccentcolor = MChilitecolor;
	alloccolor(MCaccentcolor);
	
	background_pixel.red = background_pixel.green = background_pixel.blue = 0xC0C0;
	alloccolor(background_pixel);
	
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
	if (t_stack -> gettilecache())
		MCTileCacheCompact(t_stack -> gettilecache());
}

void MCScreenDC::handle_mouse_press(uint32_t p_time, uint32_t p_modifiers, int32_t x, int32_t y, int32_t p_button, MCMousePressState p_state)
{
	if (m_current_window == nil)
		return;
	
	if (m_mouse_x != x || m_mouse_y != y)
	{
		m_mouse_x = x;
		m_mouse_y = y;
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
	MCEventQueuePostMousePosition((MCStack *)m_current_window, p_time, p_modifiers, x, y);
}

void MCScreenDC::handle_key_press(uint32_t p_modifiers, uint32_t p_char_code, uint32_t p_key_code)
{
	if (m_current_window == nil)
		return;
	
	MCStack *t_stack;
	t_stack = (MCStack *)m_current_window;
	
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
			
			t_touch = new MCActiveTouch;
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

void MCScreenDC::setname(Window p_window, const char *p_new_name)
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

void MCScreenDC::boundrect(MCRectangle &rect, Boolean title, Window_mode m)
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
		if (t_old_stack -> gettilecache() != nil)
			MCTileCacheDeactivate(t_old_stack -> gettilecache());
		
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
		if (t_new_stack -> gettilecache() != nil)
			MCTileCacheActivate(t_new_stack -> gettilecache());
		
		t_new_stack -> setextendedstate(false, ECS_DONTDRAW);
		
		if (!t_new_stack -> getstate(CS_MFOCUSED))
			MCEventQueuePostMouseFocus(t_new_stack, m_message_time, true);
		
		if (!m_current_focus)
			focus_window(p_window);
		
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
	t_bitmap = new MCMobileBitmap;
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

void MCScreenDC::querymouse(int2 &x, int2 &y)
{
	// These co-ords should be in screen co-ords, so adjust the view-based current co-ords
	// by the view top-left.
	x = m_mouse_x + m_window_left;
	y = m_mouse_y + m_window_top;
}

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

Pixmap MCScreenDC::createpixmap(uint2 width, uint2 height, uint2 depth, Boolean purge)
{
	_Drawable *t_pixmap;
	t_pixmap = new _Drawable;
	t_pixmap -> type = DC_BITMAP;
	t_pixmap -> handle . pixmap = (MCSysBitmapHandle)MCMobileBitmapCreate(width, height, depth == 1);
	return t_pixmap;
}

Pixmap MCScreenDC::createstipple(uint2 width, uint2 height, uint4 *bits)
{
	return NULL;
}

void MCScreenDC::freepixmap(Pixmap &pixmap)
{
	if (pixmap == NULL)
		return;
	
	MCMobileBitmap *t_bitmap;
	t_bitmap = (MCMobileBitmap *)pixmap -> handle . pixmap;
	free(t_bitmap -> data);
	delete t_bitmap;
	delete pixmap;
	
	pixmap = nil;
}

bool MCScreenDC::lockpixmap(Pixmap p_pixmap, void*& r_data, uint4& r_stride)
{
	if (p_pixmap == NULL)
		return false;
	
	MCMobileBitmap *t_bitmap;
	t_bitmap = (MCMobileBitmap *)p_pixmap -> handle . pixmap;
	r_data = t_bitmap -> data;
	r_stride = t_bitmap -> stride;
	return true;
}

void MCScreenDC::unlockpixmap(Pixmap p_pixmap, void *p_data, uint4 p_stride)
{
}

Boolean MCScreenDC::getpixmapgeometry(Pixmap p, uint2 &w, uint2 &h, uint2 &d)
{
	MCMobileBitmap *t_bitmap;
	t_bitmap = (MCMobileBitmap *)p -> handle . pixmap;
	w = t_bitmap -> width;
	h = t_bitmap -> height;
	d = t_bitmap -> is_mono ? 1 : 32;
	return True;
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

void MCScreenDC::copyplane(Drawable s, Drawable d, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy, uint4 rop, uint4 pixel)
{
	MCMobileBitmap *t_src_bitmap, *t_dst_bitmap;
	t_src_bitmap = (MCMobileBitmap *)s -> handle . pixmap;
	t_dst_bitmap = (MCMobileBitmap *)d -> handle . pixmap;
	
	assert(t_src_bitmap -> is_mono);
	assert(!t_dst_bitmap -> is_mono);
	
	uint8_t *t_src_ptr, *t_dst_ptr;
	t_src_ptr = (uint8_t *)t_src_bitmap -> data;
	t_dst_ptr = (uint8_t *)t_dst_bitmap -> data;
	
	uint32_t t_src_stride, t_dst_stride;
	t_src_stride = t_src_bitmap -> stride;
	t_dst_stride = t_dst_bitmap -> stride;
	
	t_src_ptr += sy * t_src_stride + sx / 8;
	t_dst_ptr += dy * t_dst_stride + dx * 4;
	
	uint32_t t_src_first_bit;
	t_src_first_bit = 0x80 >> (sx & 0x7);
	
	for (uint32_t y = 0; y < sh; y++)
	{
		uint8_t *t_src_bits;
		uint32_t *t_dst_bits;
		t_src_bits = t_src_ptr;
		t_dst_bits = (uint32_t *)t_dst_ptr;
		uint32_t t_bit;
		t_bit = t_src_first_bit;
		for (uint32_t x = 0; x < sw; x++)
		{
			if (*t_src_bits & t_bit)
			{
				switch (rop)
				{
					case GXcopy:
						*t_dst_bits = pixel;
						break;
					case GXand:
						*t_dst_bits &= pixel;
						break;
					case GXor:
						*t_dst_bits |= pixel;
						break;
				}
			}
			
			t_bit >>= 1;
			if (t_bit == 0)
			{
				t_bit = 0x80;
				t_src_bits++;
			}
			t_dst_bits++;
		}
		t_src_ptr += t_src_stride;
		t_dst_ptr += t_dst_stride;
	}
}

MCBitmap *MCScreenDC::createimage(uint2 depth, uint2 width, uint2 height, Boolean set, uint1 value, Boolean shm, Boolean forceZ)
{
	if (depth == 0)
		depth = 32;
	
	MCBitmap *image = new MCBitmap;
	image->width = width;
	image->height = height;
	image->format = ZPixmap;
	image->bitmap_unit = 32;
	image->byte_order = MSBFirst;
	image->bitmap_pad = 32;
	image->bitmap_bit_order = MSBFirst;
	image->depth = (uint1)depth;
	image->bytes_per_line = ((width * depth + 31) >> 3) & 0xFFFFFFFC;
	image->bits_per_pixel = (uint1)depth;
	image->red_mask = image->green_mask = image->blue_mask = (depth == 1 || depth == getdepth() ? 0x00 : 0xFF);
	image->data = new char[image->bytes_per_line * height];
	
	if (set)
	{
		uint4 bytes = image->bytes_per_line * height;
		memset(image->data, value, bytes);
	}
	
	return image;
}

void MCScreenDC::destroyimage(MCBitmap *image)
{
	if (image -> data != NULL)
		delete image -> data;
	delete image;
}

MCBitmap *MCScreenDC::copyimage(MCBitmap *source, Boolean invert)
{
	MCBitmap *image = createimage(source->depth, source->width, source->height, 0, 0, False, False);
	uint4 bytes = image->bytes_per_line * image->height;
	if (invert)
	{
		uint1 *sptr = (uint1 *)source->data;
		uint1 *dptr = (uint1 *)image->data;
		while (bytes--)
			*dptr++ = ~*sptr++;
	}
	else
		memcpy(image->data, source->data, bytes);
	return image;
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::putimage(Drawable dest, MCBitmap *source, int2 sx, int2 sy, int2 dx, int2 dy, uint2 w, uint2 h)
{
	uint8_t *t_src_ptr;
	uint32_t t_src_stride;
	t_src_ptr = (uint8_t *)source -> data + source -> bytes_per_line * sy + sx * source -> depth / 8;
	t_src_stride = source -> bytes_per_line;
	
	MCMobileBitmap *t_dst_bitmap;
	uint8_t *t_dst_ptr;
	uint32_t t_dst_stride;
	t_dst_bitmap = (MCMobileBitmap *)dest -> handle . pixmap;
	t_dst_ptr = (uint8_t *)t_dst_bitmap -> data + t_dst_bitmap -> stride * dy + dx * (t_dst_bitmap -> is_mono ? 1 : 32) / 8;
	t_dst_stride = t_dst_bitmap -> stride;
	
	if (!t_dst_bitmap -> is_mono)
		for(uint32_t i = 0; i < h; i++, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
			for(uint32_t j = 0; j < w; j++)
				((uint32_t *)t_dst_ptr)[j] = ((uint32_t *)t_src_ptr)[j];
	else
	{
		// MW-2012-06-27: [[ Bug ]] Round up number of bytes to copy to nearest byte
		//   (rather than down).
		for(uint32_t i = 0; i < h; i++)
			memcpy(t_dst_ptr + i * t_dst_stride, t_src_ptr + i * t_src_stride, (w * source -> depth + 7) / 8);
}
}

MCBitmap *MCScreenDC::getimage(Drawable pm, int2 x, int2 y, uint2 w, uint2 h, Boolean shm)
{
	// NULL input means get from screen - not supported at present.
	if (pm == NULL)
		return NULL;
	
	MCMobileBitmap *t_src_bitmap;
	uint8_t *t_src_ptr;
	uint32_t t_src_stride;
	t_src_bitmap = (MCMobileBitmap *)pm -> handle . pixmap;
	t_src_ptr = (uint8_t*)t_src_bitmap -> data + y * t_src_bitmap -> stride + x * (t_src_bitmap -> is_mono ? 1 : 32) / 8;
	t_src_stride = t_src_bitmap -> stride;
	
	MCBitmap *t_dst_bitmap;
	uint8_t *t_dst_ptr;
	uint32_t t_dst_stride;
	t_dst_bitmap = createimage(t_src_bitmap -> is_mono ? 1 : 32, w, h, False, 0, False, shm);
	t_dst_ptr = (uint8_t *)t_dst_bitmap -> data;
	t_dst_stride = t_dst_bitmap -> bytes_per_line;
	
	if (!t_src_bitmap -> is_mono)
		for(uint32_t i = 0; i < h; i++, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
			for(uint32_t j = 0; j < w; j++)
				((uint32_t *)t_dst_ptr)[j] = ((uint32_t *)t_src_ptr)[j];
	else
		// IM-2012-11-16: [[ Bug ]] Round up number of bytes to copy to nearest byte
		//   (rather than down).
		for(uint32_t i = 0; i < h; i++)
			memcpy(t_dst_ptr + i * t_dst_stride, t_src_ptr + i * t_src_stride, (w + 7) / 8);
	
	return t_dst_bitmap;
}

void MCScreenDC::flipimage(MCBitmap *image, int2 byte_order, int2 bit_order)
{
}

////////////////////////////////////////////////////////////////////////////////

MCCursorRef MCScreenDC::createcursor(MCImageBuffer *p_image, int2 p_hotspot_x, int2 p_hotspot_y)
{
	return NULL;
}

void MCScreenDC::freecursor(MCCursorRef c)
{
}

////////////////////////////////////////////////////////////////////////////////

uint4 MCScreenDC::dtouint4(Drawable d)
{
	if (d == DNULL)
		return 0;
	
	return (uint4)d -> handle . pixmap;
}

Boolean MCScreenDC::uint4topixmap(uint4 p_ptr, Pixmap &p)
{
	_Drawable *t_pixmap;
	t_pixmap = new _Drawable;
	t_pixmap -> type = DC_BITMAP;
	t_pixmap -> handle . pixmap = (MCSysBitmapHandle)p_ptr;
	p = t_pixmap;
	return True;
}

Boolean MCScreenDC::uint4towindow(uint4, Window &w)
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

void MCScreenDC::configurebackdrop(const MCColor& p_colour, Pixmap p_pattern, MCImage *p_badge)
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

void MCScreenDC::setmouse(int2 x, int2 y)
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
	
	bool DoReset(const char *name) { return false; }
	bool DoResetSettings(const MCString& settings) { return false; }
	void DoResync(void) {}
	
	const char *DoFetchName(void) { return NULL; }
	void DoFetchSettings(void*& r_buffer, uint32_t& r_length) { r_length = 0; r_buffer = NULL; }
	
	MCPrinterDialogResult DoPrinterSetup(bool p_window_modal, Window p_owner)  { return PRINTER_DIALOG_RESULT_ERROR; }
	MCPrinterDialogResult DoPageSetup(bool p_window_modal, Window p_owner) { return PRINTER_DIALOG_RESULT_ERROR; }
	MCPrinterResult DoBeginPrint(const char *p_document, MCPrinterDevice*& r_device) { return PRINTER_RESULT_ERROR; }
	MCPrinterResult DoEndPrint(MCPrinterDevice* p_device) { return PRINTER_RESULT_ERROR; }
};

MCPrinter *MCScreenDC::createprinter(void)
{
	return new MCDummyPrinter;
}

void MCScreenDC::listprinters(MCExecPoint& ep)
{
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

MCDragAction MCScreenDC::dodragdrop(MCPasteboard *p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint *p_image_offset)
{
	return DRAG_ACTION_NONE;
}

////////////////////////////////////////////////////////////////////////////////

MCScriptEnvironment *MCScreenDC::createscriptenvironment(const char *p_language)
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
