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

#ifndef __MC_MOBILE_DC__
#define __MC_MOBILE_DC__

#ifndef __MC_UIDC__
#include "uidc.h"
#endif

#ifndef __MC_MOBILE_FONTLIST__
#include "mblflst.h"
#endif

#ifndef __MC_EVENT_QUEUE__
#include "eventqueue.h"
#endif

struct MCMobileBitmap
{
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	// If true, then the data is for a 1-bit per pixel mask.
	bool is_mono : 1;
	// If true, then the data is in OpenGL (reversed) byte order.
	bool is_swapped : 1;
	// The data for the bitmap.
	void *data;
};

MCMobileBitmap *MCMobileBitmapCreate(uint32_t width, uint32_t height, bool mono);

class MCWindowStack;
class MCActiveTouch;

class MCScreenDC: public MCUIDC
{
public:
	MCScreenDC(void);
	virtual ~MCScreenDC(void);

	Boolean open(void);
	Boolean close(Boolean force);

	bool hasfeature(MCPlatformFeature feature);
	MCNameRef getdisplayname(void);
	MCNameRef getvendorname(void);
	uint2 getwidth();
	uint2 getheight();
	uint2 getwidthmm();
	uint2 getheightmm();
	uint2 getmaxpoints();
	uint2 getvclass();
	uint2 getdepth();
	uint2 getrealdepth(void);
	uint2 getpad();
	Window getroot();
	
	// IM-2014-01-23: [[ HiDPI ]] Change device methods to platform-specific logical-coordinate based methods
	
	virtual uint16_t platform_getwidth(void);
	virtual uint16_t platform_getheight(void);
	virtual bool platform_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count);
	virtual bool platform_displayinfocacheable(void);
	virtual bool platform_getwindowgeometry(Window w, MCRectangle &drect);
	virtual void platform_boundrect(MCRectangle &rect, Boolean title, Window_mode m, Boolean resizable);
	virtual void platform_querymouse(int16_t &r_x, int16_t &r_y);
	virtual void platform_setmouse(int16_t p_x, int16_t p_y);
	
	virtual void *GetNativeWindowHandle(Window p_window);
	
	// IM-2014-01-28: [[ HiDPI ]] Convenience methods to convert logical to screen coords and back
	
    static MCGFloat logicaltoscreenscale(void);
    MCPoint logicaltoscreenpoint(const MCPoint &p_point);
    MCPoint screentologicalpoint(const MCPoint &p_point);
    MCRectangle logicaltoscreenrect(const MCRectangle &p_rect);
    MCRectangle screentologicalrect(const MCRectangle &p_rect);
	
	uint16_t device_getwidth();
	uint16_t device_getheight();
	bool device_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count);
	bool device_getwindowgeometry(Window w, MCRectangle &drect);
	void device_boundrect(MCRectangle &rect, Boolean title, Window_mode m);
	void device_setmouse(int16_t p_x, int16_t p_y);

	bool fullscreenwindows(void);
	MCRectangle fullscreenrect(const MCDisplay *p_display);
	
	void resetcursors(void);
	void setcursor(Window w, MCCursorRef c);

	void grabpointer(Window w);
	void ungrabpointer(void);
	void openwindow(Window w, Boolean override);
	void closewindow(Window window);
	void destroywindow(Window &window);
	void raisewindow(Window window);
	void iconifywindow(Window window);
	void uniconifywindow(Window window);
	void setname(Window window, MCStringRef newname);
	void sync(Window w);
	void setinputfocus(Window window);

	void setcmap(MCStack *sptr);
	void setgraphicsexposures(Boolean on, MCStack *sptr);

	void copyarea(Drawable source, Drawable dest, int2 depth, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy, uint4 rop);

	MCCursorRef createcursor(MCImageBitmap *p_image, int2 p_hotspot_x, int2 p_hotspot_y);
	void freecursor(MCCursorRef c);

	uintptr_t dtouint(Drawable d);
	Boolean uinttowindow(uintptr_t, Window &w);

	void beep();
	bool setbeepsound(MCStringRef p_beep_sound);
	bool getbeepsound(MCStringRef& r_beep_sound);
	void getbeep(uint4 property, int4& r_value);
	void setbeep(uint4 property, int4 beep);

	MCImageBitmap *snapshot(MCRectangle &r, uint4 window, MCStringRef displayname, MCPoint *size);

	void enablebackdrop(bool p_hard = false);
	void disablebackdrop(bool p_hard = false);
	void configurebackdrop(const MCColor& p_colour, MCPatternRef p_pattern, MCImage *p_badge);
	void assignbackdrop(Window_mode p_mode, Window p_window);

	void hidemenu();
	void hidetaskbar();
	void showmenu();
	void showtaskbar();

	MCColor *getaccentcolors();

	void expose();
	Boolean abortkey();
	void waitconfigure(Window w);
	void waitreparent(Window w);
	void waitfocus();
	uint2 querymods();
	Boolean getmouse(uint2 button, Boolean& r_abort);
	Boolean getmouseclick(uint2 button, Boolean& r_abort);
	Boolean wait(real8 duration, Boolean dispatch, Boolean anyevent);
	
	// MW-2011-08-16: [[ Wait ]] Notifies wait it should recheck for events and/or break.
	void pingwait(void);
	
	void flushevents(uint2 e);
	void updatemenubar(Boolean force);
	Boolean istripleclick();
	bool getkeysdown(MCListRef& r_list);
	
	uint1 fontnametocharset(MCStringRef p_fontname);
//	char *charsettofontname(uint1 charset, const char *oldfontname);
	
	void clearIME(Window w);
	void openIME();
	void activateIME(Boolean activate);
	void closeIME();

	void enactraisewindows(void);
	
	//

	MCPrinter *createprinter(void);
	//

	bool ownsselection(void);
	bool setselection(MCPasteboard *p_pasteboard);
	MCPasteboard *getselection(void);
	void flushclipboard(void);
	bool ownsclipboard(void);
	bool setclipboard(MCPasteboard *p_pasteboard);
	MCPasteboard *getclipboard(void);

	//

	MCScriptEnvironment *createscriptenvironment(MCStringRef p_language);

	//
	
    int32_t popupanswerdialog(MCStringRef *p_buttons, uint32_t p_button_count, uint32_t p_type, MCStringRef p_title, MCStringRef p_message, bool p_blocking);
	bool popupaskdialog(uint32_t p_type, MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, bool p_hint, MCStringRef& r_result);
	
	////////// COMMON IMPLEMENTATION METHODS
	
	// Common portion of the open method.
	void common_open(void);
	
	// Called when we need to minimize memory.
	void compact_memory(void);
	
	// Common event handling methods.
	void handle_mouse_press(uint32_t p_time, uint32_t p_modifiers, int32_t x, int32_t y, int32_t p_button, MCMousePressState p_state);
	void handle_mouse_move(uint32_t p_time, uint32_t p_modifiers, int32_t x, int32_t y);
	void handle_key_press(uint32_t p_modifiers, uint32_t p_char_code, uint32_t p_key_code);
	void handle_key_focus(bool focused);
	void handle_redraw(const MCRectangle& p_dirty);
	void handle_motion(MCEventMotionType type, double timestamp);
	void handle_touch(MCEventTouchPhase phase, void *touch, int32_t p_timestamp, int32_t p_device_x, int32_t p_device_y);
	
	// Common window handling methods.
	void open_window(Window p_window);
	void close_window(Window p_window);
	void focus_window(Window p_window);
	void refresh_window(Window p_window);
	
	// Redraw the current main window in entirety
	void redraw_current_window(void);
	// Unfocus the current main window.
	void unfocus_current_window(void);
	
	// Do the leg work of processing a touch, including active touch tracking.
	void process_touch(MCEventTouchPhase phase, void *touch, int32_t p_timestamp, int32_t p_x, int32_t p_y);
	// Cancel all active touches (causes 'cancel' events to be sent).
	void cancel_touches(void);
	// Clear all active touches (causes no events to be sent).
	void clear_touches(void);
	
	// Called by common implementation when the current window should take focus.
	void do_take_focus(void);
	
	void do_fit_window(bool immediate_resize, bool post_message);
	
	// MW-2012-03-05: [[ ViewStack ]] Platform specific method for changing the visible
	//   stack in the app's view.
	void do_show_stack_in_window(MCStack *stack);
	
	// MW-2012-11-14: [[ Bug 10514 ]] Returns the current window on display.
	Window get_current_window(void);
	
    void refresh_current_window(void);
	
	void getsystemappearance(MCSystemAppearance &r_appearance);
private:
	// The top-left of the mobile 'window' in screen co-ordinates.
	int32_t m_window_left;
	int32_t m_window_top;
	
	// The current position of the mouse touch in window co-ordinates.
	int32_t m_mouse_x;
	int32_t m_mouse_y;
	
	// The time of the last event.
	uint32_t m_message_time;
	
	// The list of main windows currently open.
	MCWindowStack *m_main_windows;
	// The current window that is visible.
	Window m_current_window;
	
	// The main window has focus (or not)
	bool m_current_focus;
	
	// The touch that corresponds to the synthesized mouse (if any)
	void *m_mouse_touch;
	// The list of touches that are currently active
	MCActiveTouch *m_active_touches;
	// The last id to be assigned to a touch
	uint32_t m_last_touch_id;
};

#endif
