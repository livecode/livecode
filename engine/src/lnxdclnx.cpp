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

//
// X ScreenDC display specific functions
//
#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"


#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "card.h"
#include "field.h"
#include "sellst.h"
#include "util.h"

#include "debug.h"
#include "osspec.h"
#include "stacklst.h"

#include "globals.h"

#include "lnxdc.h"
#include "lnxgtkthemedrawing.h"

#include "resolution.h"

#define XK_Window_L 0xFF6C
#define XK_Window_R 0xFF6D

#include <gdk/gdkkeysyms.h>


Boolean tripleclick = False;
static Boolean dragclick;


void MCScreenDC::setupcolors()
{
	ncolors = MCU_min(vis->colormap_size, MAX_CELLS);
	colors = new (nothrow) MCColor[ncolors];
    colornames = new (nothrow) MCStringRef[ncolors];
	allocs = new (nothrow) int2[ncolors];
	int2 i;
	for (i = 0 ; i < ncolors ; i++)
	{
        colornames[i] = MCValueRetain(kMCEmptyString);
		allocs[i] = 0;
	}
}

GdkScreen* MCScreenDC::getscreen()
{
	return gdk_visual_get_screen(vis);
}

GdkColormap* MCScreenDC::getcmap()
{
	return cmap;
}

GdkVisual* MCScreenDC::getvisual()
{
	return vis;
}

KeySym MCScreenDC::translatekeysym(KeySym sym, uint4 keycode)
{
	// Major assumption: GDK keysyms are the same as X11 keysyms
    switch (sym)
	{
	case 0:
		switch (keycode)
		{
		case 115:
			return XK_Window_L;
		case 116:
			return XK_Window_R;
		case 117:

			return XK_Menu;
		}
		break;
	case XK_L4:
	case XK_Undo:
		return XK_osfUndo;
	case XK_L10:
	case XK_apCut:
	case XK_SunCut:
		return XK_osfCut;
	case XK_L6:
	case XK_apCopy:
	case XK_SunCopy:
		return XK_osfCopy;
	case XK_L8:
	case XK_apPaste:
	case XK_SunPaste:
		return XK_osfPaste;
	case XK_Help:
		return XK_osfHelp;
	case XK_osfInsert:
	case XK_hpInsertChar:
		return XK_Insert;
	case XK_osfDelete:
	case XK_hpDeleteChar:
		return XK_Delete;
	case XK_ISO_Left_Tab: /* X11 shift-tab keysym */
	case 0x1000FF74: // HP shift-tab keysysm
		return XK_Tab;
	}
	return sym;
}

static Boolean isKeyPressed(char *km, uint1 keycode)
{
	return (km[keycode >> 3] >> (keycode & 7)) & 1;
}

bool MCScreenDC::getkeysdown(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable(',', &t_list))
		return false;

    // GDK keymap for mapping hardware keycodes to symbols
    GdkKeymap *t_keymap;
    t_keymap = gdk_keymap_get_for_display(dpy);
    
    // GDK does not provide a wrapper for XQueryKeymap so we have to use it
    // directly if we want to get the key state from X11.
	char km[32];
	MCmodifierstate = querymods();
    x11::XQueryKeymap(x11::gdk_x11_display_get_xdisplay(dpy), km);
    
    // Translate the engine modifiers to GDK modifiers
    gint t_mods = 0;
    if (MCmodifierstate & (MS_SHIFT|MS_CAPS_LOCK))
        t_mods |= GDK_SHIFT_MASK;
    if (MCmodifierstate & MS_CONTROL)
        t_mods |= GDK_CONTROL_MASK;
    if (MCmodifierstate & MS_MOD1)
        t_mods |= GDK_MOD1_MASK;
    if (MCmodifierstate & MS_MOD2)
        t_mods |= GDK_MOD2_MASK;
    if (MCmodifierstate & MS_MOD3)
        t_mods |= GDK_MOD3_MASK;
    if (MCmodifierstate & MS_MOD4)
        t_mods |= GDK_MOD4_MASK;
    if (MCmodifierstate & MS_MOD5)
        t_mods |= GDK_MOD5_MASK;
    
    // Loop through the list of hardware keys
    bool t_success = true;
	for (int i = 0; i < 256; i++)
	{
		if (isKeyPressed(km, i))
		{
            guint t_keyval;
            t_success = gdk_keymap_translate_keyboard_state(t_keymap, i, GdkModifierType(t_mods), 0, &t_keyval, NULL, NULL, NULL);
            
            if (t_success && t_keyval > 0)
				if (!MCListAppendInteger(*t_list, t_keyval))
					t_success = false;
            
            if (!t_success)
                break;
		}
	}

	return t_success && MCListCopy(*t_list, r_list);
}

void MCScreenDC::setmods(guint state, KeySym sym,
                         uint2 button, Boolean release)
{
	if (lockmods)
		return;

    // Set the button state
    uint2 t_buttons = 0;
    if (state & GDK_BUTTON1_MASK)
        t_buttons |= 1;
    if (state & GDK_BUTTON2_MASK)
        t_buttons |= 2;
    if (state & GDK_BUTTON3_MASK)
        t_buttons |= 4;
    if (state & GDK_BUTTON4_MASK)
        t_buttons |= 8;
    if (state & GDK_BUTTON5_MASK)
        t_buttons |= 16;
    
	if (button)
    {
        // Update the particular button
        if (release)
			MCbuttonstate = t_buttons & ~(1 << (button-1));
		else
			MCbuttonstate = t_buttons |  (1 << (button-1));
    }
	else
    {
		// Update all the buttons
        MCbuttonstate = t_buttons;
    }
    
    // Assumption: GDK keysyms and X11 keysyms have the same values
	if (sym >= XK_Shift_L && sym <= XK_Hyper_R)
		MCmodifierstate = querymods();
	else
	{
		// Convert the GDK modifier flags
        MCmodifierstate = 0;
        if (state & GDK_SHIFT_MASK)
            MCmodifierstate |= MS_SHIFT;
        if (state & GDK_LOCK_MASK)
            MCmodifierstate |= MS_CAPS_LOCK;
        if (state & GDK_CONTROL_MASK)
            MCmodifierstate |= MS_CONTROL;
        if (state & GDK_MOD1_MASK)
            MCmodifierstate |= MS_MOD1;
        if (state & GDK_MOD2_MASK)
            MCmodifierstate |= MS_MOD2;
        if (state & GDK_MOD3_MASK)
            MCmodifierstate |= MS_MOD3;
        if (state & GDK_MOD4_MASK)
            MCmodifierstate |= MS_MOD4;
        if (state & GDK_MOD5_MASK)
            MCmodifierstate |= MS_MOD5;
	}
}

extern "C"
{
void gtk_main_do_event(GdkEvent*);
}

static bool motion_event_filter_fn(GdkEvent *p_event, void*)
{
    return p_event->type == GDK_MOTION_NOTIFY;
}

Boolean MCScreenDC::handle(Boolean dispatch, Boolean anyevent, Boolean& abort, Boolean& reset)
{
    // Event object. Note that GDK requires these to be disposed of after handling
    GdkEvent *t_event = NULL;
    
    // Loop until both the pending event queue and GDK event queue are empty
    abort = reset = False;
    bool t_handled = false;
    while (true || dispatch || g_main_context_pending(NULL) || gdk_events_pending())
    {
        // Place all events onto the pending event queue
        EnqueueGdkEvents();
        
        bool t_queue = false;
        if (dispatch && pendingevents != NULL)
        {
            // Get the next event from the queue
            t_event = gdk_event_copy(pendingevents->event);
            MCEventnode *tptr = (MCEventnode *)pendingevents->remove(pendingevents);
            delete tptr;
        }
        
        if (t_event == NULL)
        {
            break;
        }

        // What type of event are we dealing with?
        switch (t_event->type)
        {
            case GDK_DELETE:
            {
                MCdispatcher->wclose(t_event->any.window);
                break;
            }
            
            case GDK_EXPOSE:
            case GDK_DAMAGE:
            {
                // Handled separately
                //fprintf(stderr, "GDK_EXPOSE (window %p)\n", t_event->expose.window);
                MCEventnode *t_node = new (nothrow) MCEventnode(gdk_event_copy(t_event));
                t_node->appendto(pendingevents);
                expose();
                break;
            }
                
            case GDK_FOCUS_CHANGE:
            {
                // Was focus gained or lost?
                if (t_event->focus_change.in)
                {
                    // Focus was gained. If we do not currently have focus, we
                    // can now assume that we do.
                    if (!m_application_has_focus)
                    {
                        m_application_has_focus = true;
                        hidebackdrop(true);
                        if (MCdefaultstackptr)
                            MCdefaultstackptr->getcard()->message(MCM_resume);
                        
                        MCstacks->hidepalettes(false);
                    }
                    
                    if (dispatch)
                    {
                        if (t_event->focus_change.window != MCtracewindow)
                            MCdispatcher->wkfocus(t_event->focus_change.window);
                    }
                    else
                        t_queue = true;
                }
                else
                {
                    // Focus was lost. Was it to another LiveCode window or to
                    // a different application?
                    bool t_lostfocus = false;
                    x11::Window t_return_window;
                    int t_return_revert_window;
                    
                    if (m_application_has_focus)
                    {
                        // GDK doesn't let us get the focus window so we have
                        // to use Xlib to get it. Sigh.
                        x11::XGetInputFocus(x11::gdk_x11_display_get_xdisplay(dpy), &t_return_window, &t_return_revert_window);
                        
                        // Look up the X11 window XID in GDK's window table. If
                        // it isn't found, it definitely isn't one of ours.
                        GdkWindow *t_window;
                        if ((t_window = x11::gdk_x11_window_lookup_for_display(dpy, t_return_window)) == NULL)
                            t_lostfocus = true;
                        
                        // Even if we found it, it may not be ours. This is very
                        // unlikely but could happen if we've created a GdkWindow
                        // for it in the past (e.g. in import snapshot)
                        if ((backdrop != NULL && t_window == backdrop) || MCdispatcher->findstackd(t_window) != NULL)
                            t_lostfocus = false;
                        
                        if (t_lostfocus)
                        {
                            // Another application gained focus
                            m_application_has_focus = false;
                            hidebackdrop(true);
                            if (MCdefaultstackptr)
                                MCdefaultstackptr->getcard()->message(MCM_suspend);
                            
                            MCstacks->hidepalettes(true);
                        }
                    }
                    
                    if (dispatch)
                    {
                        if (t_event->focus_change.window != MCtracewindow)
                            MCdispatcher->wkunfocus(t_event->focus_change.window);
                    }
                    else
                        t_queue = true;
                }
                
                t_handled = true;
                break;
            }
                
            case GDK_KEY_PRESS:
            case GDK_KEY_RELEASE:
            {
                // We also want the key symbol for non-character keys etc
                uint32_t t_keysym = translatekeysym(t_event->key.keyval, t_event->key.hardware_keycode);
                
                // Update the current modifier state
                setmods(t_event->key.state, t_keysym, 0, False);
                
                // Check for the interrupt command
                if (t_event->type == GDK_KEY_PRESS && MCmodifierstate & MS_CONTROL)
                {
                    if (t_keysym == XK_Break || t_keysym == '.')
                    {
                        if (MCallowinterrupts && !MCdefaultstackptr->cantabort())
                            abort = True;
                        else
                            MCinterrupt = true;
                    }
                }
                
                if (dispatch)
                {
                    if (t_event->key.window != MCtracewindow)
                    {
                        // Let the IME have the key event first
                        bool t_ignore = false;
                        if (dispatch && MCactivefield && m_im_context != nil)
                        {
                            t_ignore = gtk_im_context_filter_keypress(m_im_context, &t_event->key);
                        }
                        
                        // No further processing of the event if the IME ate it
                        if (t_ignore)
                        {
                            t_handled = true;
                            break;
                        }
                        
                        // Convert the key event into a Unicode character
                        codepoint_t t_codepoint = gdk_keyval_to_unicode(t_event->key.keyval);
                        MCAutoStringRef t_text;
                        if (t_codepoint != 0)
                            /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)&t_codepoint, sizeof(t_codepoint), kMCStringEncodingUTF32, false, &t_text);
                        else
                            t_text = MCValueRetain(kMCEmptyString);
                        
                        MCeventtime = t_event->key.time;
                        if (t_event->type == GDK_KEY_PRESS)
                            MCdispatcher->wkdown(t_event->key.window, *t_text, t_keysym);
                        else
                            MCdispatcher->wkup(t_event->key.window, *t_text, t_keysym);
                    }
                }
                else
                {
                    t_queue = true;
                }
                
                t_handled = true;
                break;
            }
                
            case GDK_ENTER_NOTIFY:
            case GDK_LEAVE_NOTIFY:
            {
                if (t_event->type == GDK_ENTER_NOTIFY)
                {
                    // Update which stack currently contains the mouse
                    MCmousestackptr = MCdispatcher->findstackd(t_event->crossing.window);
                    if (MCmousestackptr)
                        MCmousestackptr->resetcursor(True);
                }
                else
                {
                    // The mouse is not within any of our stacks
                    MCmousestackptr = nil;
                }
                if (dispatch)
                {
                    if (t_event->crossing.window != MCtracewindow)
                    {
                        if (t_event->type == GDK_ENTER_NOTIFY)
                        {
                            if (MCmousestackptr)
                            {
                                // Send a window focus event
                                MCdispatcher->enter(t_event->crossing.window);
                                MCdispatcher->wmfocus(t_event->crossing.window, t_event->crossing.x, t_event->crossing.y);
                            }
                        }
                        else
                        {
                            // Send a window unfocus event
                            MCdispatcher->wmunfocus(t_event->crossing.window);
                        }
                    }
                }
                else
                {
                    t_queue = true;
                }
                
                t_handled = true;
                break;
            }
                
            case GDK_MOTION_NOTIFY:
            {
                // Get the most up-to-date motion event
                GdkEvent *t_new_event;
                while (GetFilteredEvent(&motion_event_filter_fn, t_new_event, NULL))
                {
                    gdk_event_free(t_event);
                    t_event = t_new_event;
                }
                
                // Update the modifier keys flags
                setmods(t_event->motion.state, 0, 0, False);
                
                // IM-2013-08-12: [[ ResIndependence ]] Scale mouse coords to user space
                MCGFloat t_scale;
                t_scale = MCResGetPixelScale();
                
                MCPoint t_mouseloc;
                t_mouseloc = MCPointMake(t_event->motion.x / t_scale, t_event->motion.y / t_scale);
                
                MCStack *t_mousestack;
                t_mousestack = MCdispatcher->findstackd(t_event->motion.window);
                
                // In certain types of modal loops (e.g. drag-and-drop) we may
                // be receiving events from other windows - in that case, the
                // window won't be found and some massaging is required
                if (t_mousestack == NULL && MCmousestackptr)
                {
                    // Retain the current mouse stack and adjust the coordinates
                    // to be relative to it
                    t_mousestack = MCmousestackptr;
                    gint ox, oy;
                    gdk_window_get_origin(MCmousestackptr->getwindow(), &ox, &oy);
                    t_mouseloc = MCPointMake((t_event->motion.x_root - ox) / t_scale, (t_event->motion.y_root - oy) / t_scale);
                }
                
                // IM-2013-10-09: [[ FullscreenMode ]] Update mouseloc with MCscreen getters & setters
                MCscreen->setmouseloc(t_mousestack, t_mouseloc);
                
                // If this is a motion hint event, request the rest
                if (t_event->motion.is_hint)
                    gdk_event_request_motions(&t_event->motion);
                
                // Detect if we should start a drag
                if (!dragclick && (MCU_abs(MCmousex - MCclicklocx) > 4 || MCU_abs(MCmousey - MCclicklocy) > 4) && MCbuttonstate != 0)
                {
                    last_window = t_event->motion.window;
                    dragclick = true;
                    MCdispatcher->wmdrag(last_window);
                }
                
                if (dispatch)
                {
                    if (t_event->motion.window != MCtracewindow)
                    {
                        MCeventtime = t_event->motion.time;
                        MCdispatcher->wmfocus(t_event->motion.window, t_mouseloc.x, t_mouseloc.y);
                    }
                }
                else
                    t_queue = true;
                
                t_handled = true;
                break;
            }
             
            case GDK_SCROLL:
            case GDK_BUTTON_PRESS:
            {
                // We're not dragging
                dragclick = false;
                
                // Update the mouse button status
                if (t_event->type == GDK_BUTTON_PRESS)
                    setmods(t_event->button.state, 0, t_event->button.button, False);
                else
                    setmods(t_event->scroll.state, 0, 0, False);
                
                // IM-2013-08-12: [[ ResIndependence ]] Scale mouse coords to user space
                MCGFloat t_scale;
                t_scale = MCResGetPixelScale();
                
                // NOTE: this depends on the offsets for the x and y positions
                // of the event being in the same place in the GdkEventButton
                // and GdkEventScroll structures.
                MCPoint t_clickloc;
                t_clickloc = MCPointMake(t_event->button.x / t_scale, t_event->button.y / t_scale);
                
                MCStack *t_mousestack;
                t_mousestack = MCdispatcher->findstackd(t_event->motion.window);
                
                // IM-2013-10-09: [[ FullscreenMode ]] Update mouseloc with MCscreen getters & setters
                // FG-2014-09-22: [[ Bugfix 13225 ]] Update the mouse position before the click
                MCscreen->setmouseloc(t_mousestack, t_clickloc);
                
                MCStack *t_old_clickstack;
                MCPoint  t_old_clickloc;
                MCscreen->getclickloc(t_old_clickstack, t_old_clickloc);
                
                // IM-2013-10-09: [[ FullscreenMode ]] Update clicklock with MCscreen getters & setters
                MCscreen->setclickloc(MCmousestackptr, t_clickloc);
                
                // Used for measuring double clicks
                static guint32 clicktime = -1;
                
                if (dispatch)
                {
                    if (t_event->button.window != MCtracewindow)
                    {
                        MCeventtime = t_event->button.time;
                        
                        // Is this a mouse scroll event?
                        if (MCmousestackptr && t_event->type == GDK_SCROLL)
                        {
                            // Find the object that should receive the scroll
                            MCObject *mfocused = MCmousestackptr->getcard()->getmfocused();
                            if (mfocused == NULL)
                                mfocused = MCmousestackptr->getcard();
                            
                            if (mfocused != NULL)
                            {
                                switch (t_event->scroll.direction)
                                {
                                    // GDK events are named for the 'natural scrolling' version and interpreted according to system settings
                                    case GDK_SCROLL_UP:
                                        mfocused->kdown(kMCEmptyString, XK_WheelDown);
                                        break;
                                        
                                    case GDK_SCROLL_DOWN:
                                        mfocused->kdown(kMCEmptyString, XK_WheelUp);
                                        break;
                                        
                                    case GDK_SCROLL_LEFT:
                                        mfocused->kdown(kMCEmptyString, XK_WheelRight);
                                        break;
                                        
                                    case GDK_SCROLL_RIGHT:
                                        mfocused->kdown(kMCEmptyString, XK_WheelLeft);
                                        break;
                                }
                            }
                        }
                        else
                        {
                            // Not a scroll, actually a button press
                            uint16_t t_delay;
                            
                            if (t_event->button.time < clicktime) /* 32-bit wrap */
                                t_delay = (t_event->button.time + 10000) - (clicktime + 10000);
                            else
                                t_delay = t_event->button.time - clicktime;
                            
                            clicktime = t_event->button.time;
                            
                            // Was the click on the background window?
                            if (backdrop != DNULL && t_event->button.window == backdrop)
                                MCdefaultstackptr->getcard()->message_with_args(MCM_mouse_down_in_backdrop, t_event->button.button);
                            else
                            {
                                // MM-2013-09-16: [[ Bugfix 11176 ]] Make sure we calculate the y delta correctly.
                                if (t_delay < MCdoubletime
                                    && MCU_abs(t_old_clickloc.x - t_clickloc.x) < MCdoubledelta
                                    && MCU_abs(t_old_clickloc.y - t_clickloc.y) < MCdoubledelta)
                                {
                                    // If we've already detected a double-click,
                                    // this must be a treble-click event.
                                    if (doubleclick)
                                    {
                                        doubleclick = False;
                                        tripleclick = True;
                                        MCdispatcher->wmdown(t_event->button.window, t_event->button.button);
                                    }
                                    else
                                    {
                                        // This is a double-click event
                                        doubleclick = True;
                                        MCdispatcher->wdoubledown(t_event->button.window, t_event->button.button);
                                    }
                                    
                                    reset = True;
                                }
                                else
                                {
                                    doubleclick = tripleclick = false;
                                    MCdispatcher->wmfocus(t_event->button.window, t_clickloc.x, t_clickloc.y);
                                    MCdispatcher->wmdown(t_event->button.window, t_event->button.button);
                                }
                            }
                        }
                    }
                }
                else
                {
                    t_queue = true;
                }
                
                t_handled = true;
                break;
            }
                
            case GDK_BUTTON_RELEASE:
            {
                // No longer in a drag-and-drop situation
                dragclick = false;
                
                // Update the current button state
                setmods(t_event->button.state, 0, t_event->button.button, True);
                
                if (dispatch)
                {
                    if (backdrop != DNULL && t_event->button.window == backdrop)
                    {
                        // Don't send mouse events to the backdrop
                        MCdefaultstackptr->getcard()->message_with_args(MCM_mouse_up_in_backdrop, t_event->button.button);
                    }
                    else
                    {
                        if (t_event->button.window != MCtracewindow)
                        {
                            MCeventtime = t_event->button.time;
                            if (doubleclick)
                                MCdispatcher->wdoubleup(t_event->button.window, t_event->button.button);
                            else
                                MCdispatcher->wmup(t_event->button.window, t_event->button.button);
                            reset = True;
                        }
                    }
                }
                else
                {
                    t_queue = true;
                }
                
                t_handled = true;
                break;
            }
                
            // This replaces the need to check for state changes in the property
            // notify handler.
            case GDK_WINDOW_STATE:
            {
                // Which window underwent a state change?
                MCeventtime = gdk_event_get_time(t_event);
                MCStack *t_target = MCdispatcher->findstackd(t_event->window_state.window);
                if (t_target != NULL)
                {
                    // Which state flags changed?
                    if (t_event->window_state.changed_mask & GDK_WINDOW_STATE_ICONIFIED)
                    {
                        // Was the iconified flag set or cleared?
                        if (t_event->window_state.new_window_state & GDK_WINDOW_STATE_ICONIFIED)
                            t_target->iconify();
                        else
                            t_target->uniconify();
                    }
                }
                
                t_handled = true;
                break;
            }
                
            case GDK_PROPERTY_NOTIFY:
                // No longer required - only monitored for window state changes
                // which GDK provides more explicit events for.
                break;
                
            case GDK_CONFIGURE:
            {
                // Window geometry has changed
                // We may need to handle window geometry limits ourselves
                MCStack *t_stack = MCdispatcher->findstackd(t_event->configure.window);
                if (t_stack == nil)
                    break;
                
                GdkGeometry t_geom;
                gint t_new_width, t_new_height;
                guint t_flags = GDK_HINT_MIN_SIZE|GDK_HINT_MAX_SIZE;
 
                t_geom.min_width = t_stack->getminwidth();
                t_geom.max_width = t_stack->getmaxwidth();
                t_geom.min_height = t_stack->getminheight();
                t_geom.max_height = t_stack->getmaxheight();
                
                gdk_window_constrain_size(&t_geom, t_flags,
                                         t_event->configure.width, t_event->configure.height,
                                         &t_new_width, &t_new_height);
                
                if (t_new_width != t_event->configure.width || t_new_height != t_event->configure.height)
                {
                    gdk_window_unmaximize(t_event->configure.window);
                    gdk_window_resize(t_event->configure.window, t_new_width, t_new_height);
                }                        
                
                MCdispatcher->wreshape(t_event->configure.window);
                break;
            }
                
            case GDK_CLIENT_EVENT:
                // Hmm - do we still need to react to any of these?
                break;
                
            case GDK_SELECTION_CLEAR:
            {
                // Tell the appropriate clipboard that it lost ownership
                MCLinuxRawClipboard* t_clipboard = NULL;
                if (t_event->selection.selection == GDK_SELECTION_PRIMARY)
                    t_clipboard = static_cast<MCLinuxRawClipboard*> (MCselection->GetRawClipboard());
                else if (t_event->selection.selection == GDK_SELECTION_CLIPBOARD)
                    t_clipboard = static_cast<MCLinuxRawClipboard*> (MCclipboard->GetRawClipboard());
                else if (t_event->selection.selection == MCdndselectionatom)
                    t_clipboard = static_cast<MCLinuxRawClipboard*> (MCdragboard->GetRawClipboard());
                if (t_clipboard)
                    t_clipboard->LostSelection();
                
                if (t_event->selection.time != MCeventtime)
                {
                    // Clear the active selection
                    if (MCactivefield)
                        MCactivefield->unselect(False, False);
                }
                
                break;
            }
                
            case GDK_SELECTION_NOTIFY:
                // Handled as a drag-and-drop event
                DnDClientEvent(t_event);
                break;
                
            case GDK_SELECTION_REQUEST:
            {
                // Get the clipboard associated with the requested selection
                // Checking for ownership is unreliable in GDK so don't bother
                // -- we just fulfil the request anyway.
                MCLinuxRawClipboard* t_clipboard;
				if (t_event->selection.selection == GDK_SELECTION_PRIMARY)
                    t_clipboard = static_cast<MCLinuxRawClipboard*> (MCselection->GetRawClipboard());
				else if (t_event->selection.selection == GDK_SELECTION_CLIPBOARD)
                    t_clipboard = static_cast<MCLinuxRawClipboard*> (MCclipboard->GetRawClipboard());
                else if (t_event->selection.selection == MCdndselectionatom)
                    t_clipboard = static_cast<MCLinuxRawClipboard*> (MCdragboard->GetRawClipboard());
                else
                    t_clipboard = NULL;
                
                // Note: we don't use a secondary selection
                if (t_clipboard != NULL)
                {
                    // Convert the requestor window XID into a GdkWindow
                    GdkWindow *t_requestor;
                    t_requestor = x11::gdk_x11_window_foreign_new_for_display(dpy, t_event->selection.requestor);
                    
                    // There is a backwards-compatibility issue with the way the
                    // ICCCM deals with selections: older clients can request a
                    // selection but not supply a property name. In that case,
                    // the property set should be equal to the target name.
                    //
                    // The GDK manual does not say whether it works around this
                    // wrinkle so we might as well check ourselves.
                    GdkAtom t_property;
                    if (t_event->selection.property != GDK_NONE)
                        t_property = t_event->selection.property;
                    else
                        t_property = t_event->selection.target;
                    
                    // What type should the selection be converted to?
                    static GdkAtom s_targets = gdk_atom_intern_static_string("TARGETS");
                    static GdkAtom s_multiple = gdk_atom_intern_static_string("MULTIPLE");
                    static GdkAtom s_timestamp = gdk_atom_intern_static_string("TIMESTAMP");
                    if (t_event->selection.target == s_targets)
                    {
                        // Get the list of types we can convert to
                        MCAutoDataRef t_targets(t_clipboard->CopyTargets());
                        
                        if (*t_targets != NULL)
                        {
                            // Set a property on the requestor containing the
                            // list of targets we can convert to.
                            uindex_t t_atom_count = MCDataGetLength(*t_targets)/sizeof(gulong);
                            gdk_property_change(t_requestor, t_property,
                                                GDK_SELECTION_TYPE_ATOM,
                                                32,
                                                GDK_PROP_MODE_REPLACE,
                                                (const guchar*)MCDataGetBytePtr(*t_targets),
                                                t_atom_count);
                            
                            // Notify the requestor that we have replied
                            gdk_selection_send_notify(t_event->selection.requestor,
                                                      t_event->selection.selection,
                                                      t_event->selection.target,
                                                      t_property,
                                                      t_event->selection.time);
                        }
                        else
                        {
                            // We don't actually have anything to supply so
                            // reject the request without supplying any data
                            gdk_selection_send_notify(t_event->selection.requestor,
                                                      t_event->selection.selection,
                                                      t_event->selection.target,
                                                      GDK_NONE,
                                                      t_event->selection.time);
                        }
                    }
                    else if (t_event->selection.target == s_multiple)
                    {
                        // This should be handled by GDK
                        MCAssert(false);
                    }
                    else if (t_event->selection.target == s_timestamp)
                    {
                        // This should be handled by GDK
                        MCAssert(false);
                    }
                    else
                    {
                        // Turn the requested selection into a string
                        MCAutoStringRef t_atom_string(MCLinuxRawClipboard::CopyTypeForAtom(t_event->selection.target));
                        
                        // Get the requested representation of the data
                        const MCRawClipboardItemRep* t_rep = NULL;
                        MCAutoRefcounted<const MCLinuxRawClipboardItem> t_item = t_clipboard->GetSelectionItem();
                        if (t_item != NULL)
                            t_rep = t_item->FetchRepresentationByType(*t_atom_string);
                        
                        // Get the data in the requested form
                        MCAutoDataRef t_data;
                        if (t_rep != NULL)
                            t_data.Give(t_rep->CopyData());
                        
                        if (*t_data != NULL)
                        {
                            // Transfer the data to the requestor via the
                            // property that it specified
                            gdk_property_change(t_requestor, t_property,
                                                t_event->selection.target,
                                                8,
                                                GDK_PROP_MODE_REPLACE,
                                                (const guchar*)MCDataGetBytePtr(*t_data),
                                                MCDataGetLength(*t_data));
                            
                            // Notify the requestor that we have replied
                            gdk_selection_send_notify(t_event->selection.requestor,
                                                      t_event->selection.selection,
                                                      t_event->selection.target,
                                                      t_property,
                                                      t_event->selection.time);
                        }
                        else
                        {
                            // Could not convert the data to the format that was
                            // requested - reject the request.
                            gdk_selection_send_notify(t_event->selection.requestor,
                                                      t_event->selection.selection,
                                                      t_event->selection.target,
                                                      GDK_NONE,
                                                      t_event->selection.time);
                        }
                    }
                    
                    // We don't need the requestor window handle any longer
                    g_object_unref(t_requestor);
                }
                
                break;
            }
            
            case GDK_DRAG_ENTER:
            case GDK_DRAG_LEAVE:   
            case GDK_DRAG_MOTION:  
            case GDK_DRAG_STATUS:
            case GDK_DROP_START:
            case GDK_DROP_FINISHED:
                DnDClientEvent(t_event);
                break;

            default:
                // Any other event types are ignored
                break;
        }
        
        // Flush all pending messages to X11
        gdk_display_flush(dpy);
        
        // Queue the message if required. Otherwise, dispose of it
        if (t_queue)
        {
            MCEventnode *tptr = new (nothrow) MCEventnode(t_event);
            tptr->appendto(pendingevents);
            t_event = NULL;
        }
        else if (t_event != NULL)
        {
            gdk_event_free(t_event);
            t_event = NULL;
        }
    }
    
    return t_handled;
}

void MCScreenDC::waitmessage(GdkWindow* w, int event_type)
{
	// Does nothing
}


GdkAtom MCworkareaatom;
GdkAtom MCstrutpartialatom;
GdkAtom MCclientlistatom;
GdkAtom MCdndselectionatom;


void MCScreenDC::EnqueueGdkEvents(bool p_block)
{
    while (true)
    {
        // Run the GLib main loop. We only block for the first iteration.
        //gdk_threads_leave();
        while (g_main_context_iteration(NULL, p_block))
            p_block = false;
        //gdk_threads_enter();
        
        // Enqueue any further GDK events
        GdkEvent *t_event = gdk_event_get();
        if (t_event == NULL)
            break;
        
        // GTK hasn't had a chance at this event yet
        //gtk_main_do_event(t_event);
        
        MCEventnode *t_eventnode = new (nothrow) MCEventnode(t_event);
        t_eventnode->appendto(pendingevents);
    }
}

bool MCScreenDC::GetFilteredEvent(bool (*p_filterfn)(GdkEvent*, void*), GdkEvent* &r_event, void *p_context, bool p_may_block)
{
    // Gather all events into the pending events queue. Because we are looking
    // for a particular event, we can allow blocking until it arrives if the
    // caller desires it.
    EnqueueGdkEvents(p_may_block);
    
    MCEventnode *t_eventnode = pendingevents;
    while (t_eventnode != NULL)
    {
        if (p_filterfn(t_eventnode->event, p_context))
        {
            r_event = gdk_event_copy(t_eventnode->event);
            t_eventnode = t_eventnode->remove(pendingevents);
            delete t_eventnode;
            return true;
        }
        
        // Remember that the list is circular
        if (t_eventnode->next() == pendingevents)
            t_eventnode = NULL;
        else
            t_eventnode = t_eventnode->next();
    }
    
    return false;
}

void MCScreenDC::EnqueueEvent(GdkEvent* p_event)
{
    MCEventnode *t_node = new (nothrow) MCEventnode(p_event);
    t_node->appendto(pendingevents);
}

void MCScreenDC::IME_OnCommit(GtkIMContext*, gchar *p_utf8_string)
{
    MCAutoStringRef t_text;
    /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)p_utf8_string, strlen(p_utf8_string), kMCStringEncodingUTF8, false, &t_text);
    
    if (MCStringGetLength(*t_text) == 1)
    {
        if (MCStringIsNative(*t_text))
            MCdispatcher->wkdown(MCactivefield->getstack()->getwindow(), *t_text, MCStringGetCodepointAtIndex(*t_text, 0));
        else
            MCdispatcher->wkdown(MCactivefield->getstack()->getwindow(), *t_text, MCStringGetCodepointAtIndex(*t_text, 0)|XK_Class_codepoint);
    }
    else
    {
        // Insert the text from the IME into the active field
        MCactivefield->stopcomposition(True, False);
        MCactivefield->finsertnew(FT_IMEINSERT, *t_text, LCH_UNICODE);
    }
}

bool MCScreenDC::IME_OnDeleteSurrounding(GtkIMContext*, gint p_offset, gint p_length)
{
    return false;
}

void MCScreenDC::IME_OnPreeditChanged(GtkIMContext* p_context)
{
    if (!MCactivefield)
        return;
    
    // Get the string. We ignore the attributes list entirely.
    gchar *t_utf8_string;
    gint t_cursor_pos;
    gtk_im_context_get_preedit_string(p_context, &t_utf8_string, NULL, &t_cursor_pos);
    
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)t_utf8_string, strlen(t_utf8_string), kMCStringEncodingUTF8, false, &t_string);
    g_free(t_utf8_string);
    
    // Do the insert
    MCactivefield->startcomposition();
    MCactivefield->finsertnew(FT_IMEINSERT, *t_string, LCH_UNICODE);
    
    // Update the cursor position
    MCactivefield->setcompositioncursoroffset(t_cursor_pos);
}

void MCScreenDC::IME_OnPreeditEnd(GtkIMContext*)
{
    if (!MCactivefield)
        return;
    
    MCactivefield->stopcomposition(True, False);
}

void MCScreenDC::IME_OnPreeditStart(GtkIMContext*)
{
    if (!MCactivefield)
        return;
    
    MCactivefield->startcomposition();
}

void MCScreenDC::IME_OnRetrieveSurrounding(GtkIMContext*)
{
    ;
}

void MCScreenDC::clearIME(Window w)
{
    if (!m_has_gtk)
        return;
    
    gtk_im_context_reset(m_im_context);
}

void MCScreenDC::activateIME(Boolean activate)
{
    if (!m_has_gtk)
        return;
    
    // SN-2015-04-22: [[ Bug 14994 ]] Ensure that there is an activeField
    //  before starting the IME in it.
    if (activate && MCactivefield)
    {
        gtk_im_context_set_client_window(m_im_context, MCactivefield->getstack()->getwindow());
        gtk_im_context_focus_in(m_im_context);
        
        if (MCinlineinput)
            gtk_im_context_set_use_preedit(m_im_context, TRUE);
        else
            gtk_im_context_set_use_preedit(m_im_context, FALSE);
    }
    else
    {
        gtk_im_context_focus_out(m_im_context);
    }
}

void MCScreenDC::configureIME(int32_t x, int32_t y)
{
    if (!m_has_gtk)
        return;
    
    GdkRectangle t_cursor;
    t_cursor.x = x;
    t_cursor.y = y;
    t_cursor.width = t_cursor.height = 1;
    
    gtk_im_context_set_cursor_location(m_im_context, &t_cursor);
}

void init_xDnD()
{
    ;
}

void MCScreenDC::DnDClientEvent(GdkEvent* p_event)
{
    switch (p_event->type)
    {
        case GDK_EXPOSE:
        case GDK_DAMAGE:
        {
            // Handled separately
            //fprintf(stderr, "GDK_EXPOSE (window %p)\n", t_event->expose.window);
            MCEventnode *t_node = new (nothrow) MCEventnode(gdk_event_copy(p_event));
            t_node->appendto(pendingevents);
            expose();
            break;
        }
        
        case GDK_DRAG_ENTER:
        {
            //fprintf(stderr, "DND: drag enter\n");
            // Temporarily set the modifier state to the asynchronous state
            uint16_t t_old_modstate = MCmodifierstate;
            MCmodifierstate = MCscreen->querymods();
            
            // Ensure our dragboard ownership info is up-to-date
            MCLinuxRawClipboard* t_dragboard = static_cast<MCLinuxRawClipboard*>(MCdragboard->GetRawClipboard());
            if (!MCdispatcher->isdragsource())
               t_dragboard->LostSelection();
            t_dragboard->SetDragContext(p_event->dnd.context);
            
            // We use the destination window as the clipboard window for drag-
            // and-drop operations from outside LiveCode as some sources get
            // confused when the window requesting the data != the drag target
            // window.
            if (!MCdispatcher->isdragsource())
                t_dragboard->SetClipboardWindow(p_event->dnd.window);
            
            // Handle the event
            MCdispatcher->wmdragenter(p_event->dnd.window);
            
            // Also perform a motion so that we have some status to return. If
            // we don't do this, some drag sources will get confused.
            
            // Fall through to the GDK_DRAG_MOTION_CASE
        }
            
        case GDK_DRAG_MOTION:
        {
            //fprintf(stderr, "DND: drag motion\n");
            // Translate the position from root to relative coordinates
            uint32_t wx, wy;    // Window-relative coordinates
            gint ox, oy;        // Window origin in root coordinates
            gdk_window_get_origin(p_event->dnd.window, &ox, &oy);
            wx = p_event->dnd.x_root - ox;
            wy = p_event->dnd.y_root - oy;
            
            // Temporarily adopt the asynchronous modifier state
            uint16_t t_old_modstate = MCmodifierstate;
            MCmodifierstate = MCscreen->querymods();
            
            // Handle the event
            MCDragActionSet t_action;
            t_action = MCdispatcher->wmdragmove(p_event->dnd.window, wx, wy);
            
            // Restore modifier state
            MCmodifierstate = t_old_modstate;
            
            // Convert the selected drag action to the corresponding GDK value
            GdkDragAction t_gdk_action = GdkDragAction(0);
            if (t_action == DRAG_ACTION_COPY)
                t_gdk_action = GDK_ACTION_COPY;
            else if (t_action == DRAG_ACTION_MOVE)
                t_gdk_action = GDK_ACTION_MOVE;
            else if (t_action == DRAG_ACTION_LINK)
                t_gdk_action = GDK_ACTION_LINK;
            
            // Reply to the motion event
            gdk_drag_status(p_event->dnd.context, t_gdk_action, GDK_CURRENT_TIME);
            break;
        }
            
        case GDK_DRAG_LEAVE:
        {
            //fprintf(stderr, "DND: drag leave\n");
            // The drag is no longer relevant to us
            MCdispatcher->wmdragleave(p_event->dnd.window);
            static_cast<MCLinuxRawClipboard*>(MCdragboard->GetRawClipboard())->SetDragContext(NULL);
            MCdragboard->FlushData();
            break;
        }
            
        case GDK_DRAG_STATUS:
            // Only sent while we are in a D&D loop so shouldn't happen
            break;
            
        case GDK_DROP_START:
        {
            //fprintf(stderr, "DND: drop start\n");
            // Temporarily adopt the asynchronous modifier state
            uint16_t t_old_modstate = MCmodifierstate;
            MCmodifierstate = MCscreen->querymods();
            
            // Something was dropped on us
            MCdispatcher->wmdragdrop(p_event->dnd.window);
            
            // Restore the modifier state
            MCmodifierstate = t_old_modstate;
            
            // Tell the source that we are now finished with it
            gdk_drop_finish(p_event->dnd.context, TRUE, p_event->dnd.time);
            break;
        }
            
        case GDK_DROP_FINISHED:
            // Only sent while we are in a D&D loop so shouldn't happen
            break;
    }
}
