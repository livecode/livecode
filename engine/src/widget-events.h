/* Copyright (C) 2015 LiveCode Ltd.
 
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
    void event_visibilitychanged(MCWidget*, bool);
    
    // Non-MCControl event for handling touches
    bool event_touch(MCWidget*, uint32_t p_id, MCEventTouchPhase, int2 p_x, int2 p_y);
    void event_cancel_touches(MCWidgetRef widget);
    
    // Non-MCControl events called by platform-specific gesture recognition
    void event_gesture_begin(MCWidget*);    // Suppress touch events until end
    void event_gesture_end(MCWidget*);      // Unlock touch events
    void event_gesture_magnify(MCWidget*, real32_t p_factor);
    void event_gesture_swipe(MCWidget*, real32_t p_delta_x, real32_t p_delta_y);
    void event_gesture_rotate(MCWidget*, real32_t p_radians);
    
    // Non-MCControl events for drag-and-drop handling
    void event_dnd_end(MCWidget*);
    void event_dnd_drop(MCWidget*);
    
    // (Child) Widget appear / disappear notifications (used to ensure the widget
    // event manager syncs events and internal state appropriately).
    void widget_appearing(MCWidgetRef widget);
    void widget_disappearing(MCWidgetRef widget);
    
    // Tell the event manager to sync widget state.
    void widget_sync(void);
    
    MCWidgetRef GetGrabbedWidget(void) const;
    MCWidgetRef GetTargetWidget(void) const;
    MCWidgetRef SetTargetWidget(MCWidgetRef target);
    
    // Returns the synchronous mouse/click coordinates
    void GetSynchronousMousePosition(coord_t& r_x, coord_t& r_y) const;
    void GetSynchronousClickPosition(coord_t& r_x, coord_t& r_y) const;
    void GetSynchronousClickButton(unsigned int& r_button) const;
    void GetSynchronousClickCount(unsigned int& r_count) const;
    
    // Returns the asynchronous ("current") mouse/click coordinates
    void GetAsynchronousMousePosition(coord_t& r_x, coord_t& r_y) const;
    void GetAsynchronousClickPosition(coord_t& r_x, coord_t& r_y) const;
    
    // Returns touch state
    uindex_t GetTouchCount(void);
    bool GetActiveTouch(integer_t& r_index);
    bool GetTouchPosition(integer_t p_index, MCPoint& r_point);
    bool GetTouchIDs(MCProperListRef& r_touch_ids);
    
private:
    void TriggerEvent(MCWidgetRef widget, MCWidgetEventTriggerType type, MCNameRef event);
    
    // State of the input devices at the last point an event was received
    coord_t     m_mouse_x, m_mouse_y;
    coord_t     m_click_x, m_click_y;
    uint32_t    m_click_time;
    uinteger_t  m_click_count;
    uinteger_t  m_click_button;
    uinteger_t  m_mouse_buttons;
    MCWidgetRef m_mouse_focus;
    MCWidgetRef m_mouse_grab;
    
    uinteger_t  m_keycode;
    uinteger_t  m_modifiers;
    MCStringRef m_keystring;
    MCWidgetRef  m_keyboard_focus;
    
    MCWidgetRef m_drag_target;
    
    MCWidgetRef m_target;
    
    // Parameters for controlling double-click time and position deltas
    uint32_t    m_doubleclick_time;
    coord_t     m_doubleclick_distance;
    
    // When set to true, widget-bound state (mouse focus etc.) will be recomputed
    // after the current event has finished.
    bool        m_check_mouse_focus : 1;
    
    // State for touch events
    struct MCWidgetTouchEvent;
    MCAutoArray<MCWidgetTouchEvent> m_touches;
    uindex_t m_touch_count;
    uindex_t m_touch_sequence;
    MCWidgetRef m_touched_widget;
    uindex_t m_touch_id;
    
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
    
    // Utility functions for managing touch events
    uinteger_t allocateTouchSlot();
    bool findTouchSlotById(uinteger_t p_id, uinteger_t& r_which);
    bool findTouchSlotByIndex(uinteger_t p_index, uinteger_t& r_which);
    void freeTouchSlot(uinteger_t p_which);
    
    // Indicates whether the given widget is in run mode or not
    bool widgetIsInRunMode(MCWidget *);
    
    // Bubble an event up from the target. This is used for things such as mouse
    // button presses and key presses. Each target has the ability to allow the
    // message to be bubbled by returning true in their event handlers.
    bool bubbleEvent(MCWidgetRef target, bool (*action)(MCWidgetRef, bool&));
    
    // Always bubble an event up from the target. This is used for things such as
    // mouseEnter/Leave events. Each target is called with the event and they do
    // not have a chance to block the bubbling.
    bool alwaysBubbleEvent(MCWidgetRef target, bool (*action)(MCWidgetRef, bool&));
    
    // The common implementation of event bubbling.
    bool doBubbleEvent(bool always_bubble, MCWidgetRef target, bool (*action)(void *context, MCWidgetRef widget, bool&), void *context);
	
	// A method to check for the problem when mouse_focus is not in-sync with
	// the script object's side of mouse_focus.
	bool check_mouse_focus(MCWidget *widget, const char *event);
};

#endif // ifndef __MC_WIDGET_EVENTS__

