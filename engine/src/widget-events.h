/* Copyright (C) 2014 Runtime Revolution Ltd.
 
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

#ifndef __MC_WIDGET_EVENTS__
#define __MC_WIDGET_EVENTS__

#include "widget.h"
#include "eventqueue.h"

class MCWidgetEventManager;
extern MCWidgetEventManager* MCwidgeteventmanager;

class MCWidgetEventManager
{
public:
    
    MCWidgetEventManager();
    
    // MCWidget re-directs all events to these functions
    void event_open(MCWidget*);
    void event_close(MCWidget*);
    void event_kfocus(MCWidget*);
    void event_kunfocus(MCWidget*);
    Boolean event_kdown(MCWidget*, MCStringRef p_key_string, KeySym p_key);
    Boolean event_kup(MCWidget*, MCStringRef p_key_string, KeySym p_key);
    Boolean event_mdown(MCWidget*, uint2 p_which);
    Boolean event_mup(MCWidget*, uint2 p_which, bool p_release);
    Boolean event_mfocus(MCWidget*, int2 p_x, int2 p_y);
    void event_munfocus(MCWidget*);
    Boolean event_doubledown(MCWidget*, uint2 p_which);
    Boolean event_doubleup(MCWidget*, uint2 p_which);
    void event_timer(MCWidget*, MCNameRef p_message, MCParameter *p_parameters);
    void event_setrect(MCWidget*, const MCRectangle& p_rectangle);
    void event_recompute(MCWidget*);
    void event_draw(MCWidget*, MCDC *p_dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite);
    void event_mdrag(MCWidget*);
    
    // Non-MCControl event for handling touches
    void event_touch(MCWidget*, uint32_t p_id, MCEventTouchPhase, int2 p_x, int2 p_y);
    
    // Non-MCControl events called by platform-specific gesture recognition
    void event_gesture_begin(MCWidget*);    // Suppress touch events until end
    void event_gesture_end(MCWidget*);      // Unlock touch events
    void event_gesture_magnify(MCWidget*, real32_t p_factor);
    void event_gesture_swipe(MCWidget*, real32_t p_delta_x, real32_t p_delta_y);
    void event_gesture_rotate(MCWidget*, real32_t p_radians);
    
    // Non-MCControl events for drag-and-drop handling
    void event_dnd_end(MCWidget*);
    void event_dnd_drop(MCWidget*);
    
    // Returns a bitmask of the mouse buttons that are pressed
    uinteger_t GetMouseButtonState() const;
    
    // Returns the widget that the mouse is currently focused on
    MCWidget* GetMouseWidget() const;
    
    // Returns the synchronous mouse/click coordinates
    void GetSynchronousMousePosition(coord_t& r_x, coord_t& r_y) const;
    void GetSynchronousClickPosition(coord_t& r_x, coord_t& r_y) const;
    
    // Returns the asynchronous ("current") mouse/click coordinates
    void GetAsynchronousMousePosition(coord_t& r_x, coord_t& r_y) const;
    void GetAsynchronousClickPosition(coord_t& r_x, coord_t& r_y) const;
    
private:
    
    // State of the input devices at the last point an event was received
    coord_t     m_mouse_x, m_mouse_y;
    coord_t     m_click_x, m_click_y;
    real64_t    m_click_time;
    uinteger_t  m_click_count;
    uinteger_t  m_click_button;
    uinteger_t  m_mouse_buttons;
    uinteger_t  m_keycode;
    uinteger_t  m_modifiers;
    MCStringRef m_keystring;
    MCWidget*   m_mouse_focus;
    MCWidget*   m_keyboard_focus;
    
    // Parameters for controlling double-click time and position deltas
    real64_t    m_doubleclick_time;
    coord_t     m_doubleclick_distance;
    
    // State for touch events
    struct MCWidgetTouchEvent;
    MCAutoArray<MCWidgetTouchEvent> m_touches;
    
    
    // Common functions for mouse gesture processing
    void mouseMove(MCWidget*, coord_t p_x, coord_t p_y);
    void mouseEnter(MCWidget*);
    void mouseLeave(MCWidget*);
    bool mouseDown(MCWidget*, uinteger_t p_which);
    bool mouseUp(MCWidget*, uinteger_t p_which);
    void mouseClick(MCWidget*, uinteger_t p_which);
    bool mouseRelease(MCWidget*, uinteger_t p_which);
    bool mouseScroll(MCWidget*, real32_t p_delta_x, real32_t p_delta_y);
    
    // Common functions for keyboard gesture processing
    bool keyDown(MCWidget*, MCStringRef, KeySym);
    bool keyUp(MCWidget*, MCStringRef, KeySym);
    
    // Common functions for touch gesture processing
    void touchBegin(MCWidget*, uinteger_t p_id, coord_t p_x, coord_t p_y);
    void touchMove(MCWidget*, uinteger_t p_id, coord_t p_x, coord_t p_y);
    void touchEnd(MCWidget*, uinteger_t p_id, coord_t p_x, coord_t p_y);
    void touchCancel(MCWidget*, uinteger_t p_id, coord_t p_x, coord_t p_y);
    void touchEnter(MCWidget*, uinteger_t p_id);
    void touchLeave(MCWidget*, uinteger_t p_id);
    
    // Utility functions for managing touch events
    uinteger_t allocateTouchSlot();
    bool findTouchSlot(uinteger_t p_id, uinteger_t& r_which);
    void freeTouchSlot(uinteger_t p_which);
    
    // Indicates whether the given widget is in run mode or not
    bool widgetIsInRunMode(MCWidget*);
};

#endif // ifndef __MC_WIDGET_EVENTS__

