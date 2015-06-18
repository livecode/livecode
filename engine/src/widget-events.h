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

enum MCWidgetEventTriggerType
{
    kMCWidgetEventTriggerTargetOnly,
    kMCWidgetEventTriggerTargetFirst,
    kMCWidgetEventTriggerTargetLast,
};

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
    void event_paint(MCWidget*, MCGContextRef);
    void event_mdrag(MCWidget*);
    
    MCObject *event_hittest(MCWidget*, int32_t x, int32_t y);
    void event_toolchanged(MCWidget*, Tool);
    void event_layerchanged(MCWidget*);
    
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
    
    MCWidgetRef GetGrabbedWidget(void) const;
    
    // Returns the synchronous mouse/click coordinates
    void GetSynchronousMousePosition(coord_t& r_x, coord_t& r_y) const;
    void GetSynchronousClickPosition(coord_t& r_x, coord_t& r_y) const;
    void GetSynchronousClickButton(unsigned int& r_button) const;
    void GetSynchronousClickCount(unsigned int& r_count) const;
    
    // Returns the asynchronous ("current") mouse/click coordinates
    void GetAsynchronousMousePosition(coord_t& r_x, coord_t& r_y) const;
    void GetAsynchronousClickPosition(coord_t& r_x, coord_t& r_y) const;
    
private:
    void TriggerEvent(MCWidgetRef widget, MCWidgetEventTriggerType type, MCNameRef event);
    
    // State of the input devices at the last point an event was received
    coord_t     m_mouse_x, m_mouse_y;
    coord_t     m_click_x, m_click_y;
    uint32_t    m_click_time;
    uinteger_t  m_click_count;
    uinteger_t  m_click_button;
    uinteger_t  m_mouse_buttons;
    uinteger_t  m_keycode;
    uinteger_t  m_modifiers;
    MCStringRef m_keystring;
    MCWidgetRef m_mouse_focus;
    MCWidgetRef m_mouse_grab;
    MCWidgetRef  m_keyboard_focus;
    
    // Parameters for controlling double-click time and position deltas
    uint32_t    m_doubleclick_time;
    coord_t     m_doubleclick_distance;
    
    // State for touch events
    struct MCWidgetTouchEvent;
    MCAutoArray<MCWidgetTouchEvent> m_touches;
    
    // Common functions for mouse gesture processing
    MCWidgetRef hitTest(MCWidgetRef, coord_t x, coord_t y);
    void mouseMove(MCWidgetRef);
    void mouseEnter(MCWidgetRef);
    void mouseLeave(MCWidgetRef);
    bool mouseDown(MCWidgetRef, uinteger_t p_which);
    bool mouseUp(MCWidgetRef, uinteger_t p_which);
    void mouseClick(MCWidgetRef, uinteger_t p_which);
    bool mouseCancel(MCWidgetRef, uinteger_t p_which);
    bool mouseScroll(MCWidgetRef, real32_t p_delta_x, real32_t p_delta_y);
    
    // Common functions for keyboard gesture processing
    bool keyDown(MCWidgetRef, MCStringRef, KeySym);
    bool keyUp(MCWidgetRef, MCStringRef, KeySym);
    
    // Common functions for touch gesture processing
    void touchBegin(MCWidgetRef, uinteger_t p_id, coord_t p_x, coord_t p_y);
    void touchMove(MCWidgetRef, uinteger_t p_id, coord_t p_x, coord_t p_y);
    void touchEnd(MCWidgetRef, uinteger_t p_id, coord_t p_x, coord_t p_y);
    void touchCancel(MCWidgetRef, uinteger_t p_id, coord_t p_x, coord_t p_y);
    void touchEnter(MCWidgetRef, uinteger_t p_id);
    void touchLeave(MCWidgetRef, uinteger_t p_id);
    
    // Utility functions for managing touch events
    uinteger_t allocateTouchSlot();
    bool findTouchSlot(uinteger_t p_id, uinteger_t& r_which);
    void freeTouchSlot(uinteger_t p_which);
    
    // Indicates whether the given widget is in run mode or not
    bool widgetIsInRunMode(MCWidgetRef);
};

#endif // ifndef __MC_WIDGET_EVENTS__

