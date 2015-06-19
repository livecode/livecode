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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "util.h"
#include "mcerror.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "widget.h"
#include "param.h"
#include "osspec.h"
#include "cmds.h"
#include "scriptpt.h"
#include "hndlrlst.h"
#include "debug.h"
#include "redraw.h"
#include "font.h"
#include "chunk.h"
#include "graphicscontext.h"
#include "dispatch.h"

#include "globals.h"
#include "context.h"

#include "widgetimp.h"
#include "widget-events.h"

////////////////////////////////////////////////////////////////////////////////

MCWidgetEventManager* MCwidgeteventmanager = nil;

////////////////////////////////////////////////////////////////////////////////

struct MCWidgetEventManager::MCWidgetTouchEvent
{
    uinteger_t  m_id;
    MCWidgetRef m_widget;
    coord_t     m_x;
    coord_t     m_y;
};

////////////////////////////////////////////////////////////////////////////////

MCWidgetEventManager::MCWidgetEventManager() :
  m_mouse_x(0), m_mouse_y(0),
  m_click_x(0), m_click_y(0),
  m_click_time(0),
  m_click_count(0),
  m_click_button(0),
  m_mouse_buttons(0),
  m_keycode(0),
  m_modifiers(0),
  m_keystring(nil),
  m_mouse_focus(nil),
  m_mouse_grab(nil),
  m_keyboard_focus(nil),
  m_doubleclick_time(MCdoubletime),
  m_doubleclick_distance(MCdoubledelta),
  m_touches()
{
    
}

////////////////////////////////////////////////////////////////////////////////

// These methods are the bridge between the 'old' MCControl world and the new
// Widget world.

void MCWidgetEventManager::event_open(MCWidget* p_widget)
{
    MCWidgetOnOpen(p_widget -> getwidget());
    if (MCcurtool != T_BROWSE)
        event_toolchanged(p_widget, MCcurtool);
}

void MCWidgetEventManager::event_close(MCWidget* p_widget)
{
    MCWidgetOnClose(p_widget -> getwidget());
}

void MCWidgetEventManager::event_kfocus(MCWidget* p_widget)
{
    // TODO: Keyboard events
    
#if 0
    // Keyboard focus has changed
    m_keyboard_focus = p_widget;
    
    p_widget->OnFocusEnter();
#endif
}

void MCWidgetEventManager::event_kunfocus(MCWidget* p_widget)
{
    // TODO: Keyboard events
    
#if 0
    // Keyboard focus has changed
    // TODO: does the unfocus *always* happen before the next focus?
    m_keyboard_focus = nil;
    
    p_widget->OnFocusLeave();
#endif
}

Boolean MCWidgetEventManager::event_kdown(MCWidget* p_widget, MCStringRef p_text, KeySym p_key)
{
    // Mouse scroll events are sent as key events
    switch (p_key)
    {
        case XK_WheelUp:
            return mouseScroll(p_widget, 0.0, +1.0);
            
        case XK_WheelDown:
            return mouseScroll(p_widget, 0.0, -1.0);
            
        case XK_WheelLeft:
            return mouseScroll(p_widget, +1.0, 0.0);
            
        case XK_WheelRight:
            return mouseScroll(p_widget, -1.0, 0.0);
            
        default:
            break;
    }
    
    // TODO: Keyboard events
#if 0
    return keyDown(p_widget, p_text, p_key);
#else
    return False;
#endif
}

Boolean MCWidgetEventManager::event_kup(MCWidget* p_widget, MCStringRef p_text, KeySym p_key)
{
#if 0
    return keyUp(p_widget, p_text, p_key);
#else
    return False;
#endif
}

Boolean MCWidgetEventManager::event_mdown(MCWidget* p_widget, uint2 p_which)
{
    // Prevent the IDE from breaking
    if (!widgetIsInRunMode(p_widget))
        return p_widget->MCControl::mdown(p_which);
    
    return mouseDown(p_widget -> getwidget(), p_which);
}

Boolean MCWidgetEventManager::event_mup(MCWidget* p_widget, uint2 p_which, bool p_release)
{
    // Prevent the IDE from breaking
    if (!widgetIsInRunMode(p_widget))
        return p_widget->MCControl::mup(p_which, p_release);
    
    if (p_release)
        return mouseCancel(p_widget -> getwidget(), p_which);

    return mouseUp(p_widget -> getwidget(), p_which);
}

Boolean MCWidgetEventManager::event_mfocus(MCWidget* p_widget, int2 p_x, int2 p_y)
{
    if (!widgetIsInRunMode(p_widget))
        return False;
    
    // Check to see if the mouse position has changed.
    bool t_pos_changed;
    t_pos_changed = !(p_x == m_mouse_x && p_y == m_mouse_y);

    // Compute the new focused widget.
    MCWidgetRef t_focused_widget;
    t_focused_widget = hitTest(p_widget -> getwidget(), p_x, p_y);

    // Check to see if the focused widget changed.
    bool t_focused_changed;
    t_focused_changed = !(m_mouse_focus == t_focused_widget);
    
    // Update the current mouse position so that it is correct in mouseEnter and
    // mouseLeave events.
    m_mouse_x = p_x;
    m_mouse_y = p_y;
    
    // Now what we do depends on whether the mouse is grabbed.
    if (m_mouse_grab != nil)
    {
        // If a widget is handling a mouse press event then we want to inform
        // the grabbed widget about enter / leave, but it keeps getting all the
        // move events.
        if (t_focused_widget != m_mouse_grab)
        {
            mouseLeave(m_mouse_grab);
            
            // We want to keep track of the focused widget we computed above,
            // but don't want to send it enter / leave in this case.
            MCValueAssign(m_mouse_focus, t_focused_widget);
        }
        else if (m_mouse_focus != m_mouse_grab)
            mouseEnter(m_mouse_grab);
        
        // We only post a mouse-move message if the position changed.
        if (t_pos_changed)
            mouseMove(m_mouse_grab);
    }
    else
    {
        if (t_focused_changed)
        {
            mouseLeave(m_mouse_focus);
        
            MCValueAssign(m_mouse_focus, t_focused_widget);
            
            mouseEnter(m_mouse_focus);
        }
        
        if (t_pos_changed)
            mouseMove(m_mouse_focus);
    }
    
    // This event was handled
    return True;
}

void MCWidgetEventManager::event_munfocus(MCWidget* p_widget)
{
    // If a widget is currently grabbed, do nothing.
    if (m_mouse_grab != nil)
        return;
    
    // If the unfocused widget is the currently focused widget, then
    // leave it.
    if (p_widget -> getwidget() == m_mouse_focus)
    {
        mouseLeave(m_mouse_focus);
        MCValueRelease(m_mouse_focus);
        m_mouse_focus = nil;
    }
}

void MCWidgetEventManager::event_mdrag(MCWidget* p_widget)
{
#if 0
    // If this widget is not a drag source, reject the drag attempt
    bool t_drag_accepted;
    t_drag_accepted = p_widget->isDragSource();
    if (t_drag_accepted)
        p_widget->OnDragStart(t_drag_accepted);
    
    if (t_drag_accepted)
        MCdragtargetptr = p_widget;
    else
        MCdragtargetptr = nil;
#endif
}

Boolean MCWidgetEventManager::event_doubledown(MCWidget* p_widget, uint2 p_which)
{
    // Prevent the IDE from breaking
    if (!widgetIsInRunMode(p_widget))
        return p_widget->MCControl::doubledown(p_which);
    
    // Because we do gesture recognition ourselves, treat this as a normal click
    return event_mdown(p_widget, p_which);
}

Boolean MCWidgetEventManager::event_doubleup(MCWidget* p_widget, uint2 p_which)
{
    // Prevent the IDE from breaking
    if (!widgetIsInRunMode(p_widget))
        return p_widget->MCControl::doubleup(p_which);
    
    // Because we do gesture recognition ourselves, treat this as a normal click
    return event_mup(p_widget, p_which, false);
}

void MCWidgetEventManager::event_timer(MCWidget* p_widget, MCNameRef p_message, MCParameter* p_parameters)
{
    
}

void MCWidgetEventManager::event_setrect(MCWidget* p_widget, const MCRectangle& p_rectangle)
{
    MCWidgetOnGeometryChanged(p_widget -> getwidget());
}

void MCWidgetEventManager::event_recompute(MCWidget* p_widget)
{
    // This gets called whenever certain parent (group, card, stack) properties
    // are changed. Unfortunately, we have no information as to *what* changed.
    MCWidgetOnParentPropertyChanged(p_widget -> getwidget());
}

void MCWidgetEventManager::event_paint(MCWidget *p_widget, MCGContextRef p_gcontext)
{
    MCWidgetOnPaint(p_widget -> getwidget(), p_gcontext);
}

void MCWidgetEventManager::event_touch(MCWidget* p_widget, uint32_t p_id, MCEventTouchPhase p_phase, int2 p_x, int2 p_y)
{
    switch (p_phase)
    {
        case kMCEventTouchPhaseBegan:
            touchBegin(p_widget, p_id, p_x, p_y);
            break;
            
        case kMCEventTouchPhaseMoved:
            touchMove(p_widget, p_id, p_x, p_y);
            break;
            
        case kMCEventTouchPhaseEnded:
            touchEnd(p_widget, p_id, p_x, p_y);
            break;
            
        case kMCEventTouchPhaseCancelled:
            touchCancel(p_widget, p_id, p_x, p_y);
            break;
    }
}

void MCWidgetEventManager::event_gesture_begin(MCWidget* p_widget)
{
    // Not implemented
}

void MCWidgetEventManager::event_gesture_end(MCWidget* p_widget)
{
    // Not implemented
}

void MCWidgetEventManager::event_gesture_magnify(MCWidget* p_widget, real32_t p_factor)
{
    // Not implemented
}

void MCWidgetEventManager::event_gesture_rotate(MCWidget* p_widget, real32_t p_radians)
{
    // Not implemented
}

void MCWidgetEventManager::event_gesture_swipe(MCWidget* p_widget, real32_t p_delta_x, real32_t p_delta_y)
{
    // Not implemented
}

void MCWidgetEventManager::event_dnd_drop(MCWidget* p_widget)
{
    //p_widget->OnDragDrop();
}

void MCWidgetEventManager::event_dnd_end(MCWidget* p_widget)
{
    //p_widget->OnDragFinish();
}

////////////////////////////////////////////////////////////////////////////////

MCWidgetRef MCWidgetEventManager::GetGrabbedWidget() const
{
    return m_mouse_grab;
}

void MCWidgetEventManager::GetSynchronousMousePosition(coord_t& r_x, coord_t& r_y) const
{
    r_x = m_mouse_x;
    r_y = m_mouse_y;
}

void MCWidgetEventManager::GetSynchronousClickPosition(coord_t& r_x, coord_t& r_y) const
{
    r_x = m_click_x;
    r_y = m_click_y;
}

void MCWidgetEventManager::GetSynchronousClickButton(unsigned int& r_button) const
{
    r_button = m_click_button;
}

void MCWidgetEventManager::GetSynchronousClickCount(unsigned int& r_count) const
{
    r_count = m_click_count;
}

void MCWidgetEventManager::GetAsynchronousMousePosition(coord_t& r_x, coord_t& r_y) const
{
    r_x = MCmousex;
    r_y = MCmousey;
}

void MCWidgetEventManager::GetAsynchronousClickPosition(coord_t& r_x, coord_t& r_y) const
{
    r_x = MCclicklocx;
    r_y = MCclicklocy;
}

////////////////////////////////////////////////////////////////////////////////

MCWidgetRef MCWidgetEventManager::hitTest(MCWidgetRef p_widget, coord_t x, coord_t y)
{
    // If an error is thrown whilst hit-testing, then we ignore the widget.
    MCWidgetRef t_target;
    if (!MCWidgetOnHitTest(p_widget, MCGPointMake(x, y), t_target))
        t_target = nil;
    return t_target;
}

void MCWidgetEventManager::mouseMove(MCWidgetRef p_widget)
{
#if 0
    // Update the mouse coordinates
    m_mouse_x = p_x;
    m_mouse_y = p_y;
    
    if (MCdispatcher->isdragtarget())
        p_widget->OnDragMove(p_x, p_y);
    else
        p_widget->OnMouseMove(p_x, p_y);
#endif
    
    MCWidgetOnMouseMove(p_widget);
}

void MCWidgetEventManager::mouseEnter(MCWidgetRef p_widget)
{
#if 0
    if (MCdispatcher->isdragtarget())
    {
        // Set this widget as the drag target if it would accept the drop
        bool t_accepted;
        MCdragaction = DRAG_ACTION_NONE;
        p_widget->OnDragEnter(t_accepted);
        if (t_accepted)
            MCdragdest = p_widget;
        else
            MCdragdest = nil;
    }
    else
        p_widget->OnMouseEnter();
#endif
    
    MCWidgetOnMouseEnter(p_widget);
}

void MCWidgetEventManager::mouseLeave(MCWidgetRef p_widget)
{
#if 0
    if (MCdispatcher->isdragtarget())
    {
        p_widget->OnDragLeave();
        MCdragdest = nil;
        MCdragaction = DRAG_ACTION_NONE;
    }
    else
        p_widget->OnMouseLeave();
#endif
    
    MCWidgetOnMouseLeave(p_widget);
}

bool MCWidgetEventManager::mouseDown(MCWidgetRef p_widget, uinteger_t p_which)
{
    if (m_mouse_buttons == 0)
        MCValueAssign(m_mouse_grab, p_widget);
    
    // Mouse button is down
    m_mouse_buttons |= (1 << p_which);
    
    if (!widgetIsInRunMode(p_widget))
        return false;
    
    // Do the position change and time since the last click make this a double
    // (or triple or more...) click?
    if ((MCeventtime <= m_click_time + m_doubleclick_time) &&
        (fabsf(m_mouse_x - m_click_x) <= m_doubleclick_distance) &&
        (fabsf(m_mouse_y - m_click_y) <= m_doubleclick_distance))
    {
        // Within the limits - this is a multiple-click event
        m_click_count += 1;
    }
    else
    {
        // Outside the limits. Only a single click.
        m_click_count = 1;
    }
    
    // The click position is updated regardless of what happens.
    m_click_x = m_mouse_x;
    m_click_y = m_mouse_y;
    m_click_time = MCeventtime;
    m_click_button = p_which;
    
    MCWidgetOnMouseDown(p_widget);
    
    return True;
}

bool MCWidgetEventManager::mouseUp(MCWidgetRef p_widget, uinteger_t p_which)
{
    // Mouse button is no longer down
    m_mouse_buttons &= ~(1 << p_which);
    
    if (m_mouse_buttons == 0)
    {
        MCValueRelease(m_mouse_grab);
        m_mouse_grab = nil;
    }
        
    if (!widgetIsInRunMode(p_widget))
        return false;
    
    mouseClick(p_widget, p_which);
    
    return True;
}

void MCWidgetEventManager::mouseClick(MCWidgetRef p_widget, uinteger_t p_which)
{
    // Send the mouse up event before the click recognition
    MCWidgetOnMouseUp(p_widget);
    
    MCWidgetOnClick(p_widget);
}

bool MCWidgetEventManager::mouseCancel(MCWidgetRef p_widget, uinteger_t p_which)
{
    // Mouse button is no longer down
    m_mouse_buttons &= ~(1 << p_which);
    
    if (m_mouse_buttons == 0)
    {
        MCValueRelease(m_mouse_grab);
        m_mouse_grab = nil;
    }
	
    if (!widgetIsInRunMode(p_widget))
        return false;
    
    // Send a mouse release event if the widget handles it
    MCWidgetOnMouseCancel(p_widget);
    
    // Release implies loss of mouse focus
    MCValueRelease(m_mouse_focus);
    m_mouse_focus = nil;
    
    return True;
}

bool MCWidgetEventManager::mouseScroll(MCWidgetRef p_widget, real32_t p_delta_x, real32_t p_delta_y)
{
    if (!widgetIsInRunMode(p_widget))
        return false;
    
#if 0
    // Only send a mouseScroll if the widget handles it, otherwise we pass
    // it on.
    if (p_widget -> handlesMouseScroll())
    {
        p_widget->OnMouseScroll(p_delta_x, p_delta_y);
        return true;
    }
#endif
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWidgetEventManager::keyDown(MCWidgetRef p_widget, MCStringRef p_string, KeySym p_key)
{
#if 0
    // Todo: key gesture (shortcuts, accelerators, etc) processing

    // Has there been a change of modifiers?
    if (MCmodifierstate != m_modifiers)
    {
        m_modifiers = MCmodifierstate;
        p_widget->OnModifiersChanged(m_modifiers);
    }
    
    // Ignore if send to the wrong widget
    if (p_widget != m_keyboard_focus)
        return false;
    
    if (!widgetIsInRunMode(p_widget))
        return false;
    
    // Does the widget want key press events?
    bool t_handled = false;
    if (p_widget->handlesKeyPress() && !MCStringIsEmpty(p_string))
    {
        p_widget->OnKeyPress(p_string);
        t_handled = true;
    }
    
    return t_handled;
#else
    return false;
#endif
}

bool MCWidgetEventManager::keyUp(MCWidgetRef p_widget, MCStringRef p_string, KeySym p_key)
{
#if 0
    // Todo: key gesture (shortcuts, accelerators, etc) processing
    
    // Has there been a change of modifiers?
    if (MCmodifierstate != m_modifiers)
    {
        m_modifiers = MCmodifierstate;
        p_widget->OnModifiersChanged(m_modifiers);
    }
    
    // Ignore if send to the wrong widget
    if (p_widget != m_keyboard_focus)
        return false;
    
    if (!widgetIsInRunMode(p_widget))
        return false;
    
    // If the widget handles key press events, treat this as handled (even
    // though we don't send another message to say the key has been released)
    return p_widget->handlesKeyPress();
#else
    return false;
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MCWidgetEventManager::touchBegin(MCWidgetRef p_widget, uinteger_t p_id, coord_t p_x, coord_t p_y)
{
#if 0
    // Check whether this touch already exists within our touch event list
    uinteger_t t_slot;
    if (findTouchSlot(p_id, t_slot))
    {
        // Already exists. This ought not to happen... ignore the event.
        return;
    }
    
    // Allocate a slot to store the event details
    t_slot = allocateTouchSlot();
    m_touches[t_slot].m_id = p_id;
    m_touches[t_slot].m_widget = p_widget;
    m_touches[t_slot].m_x = p_x;
    m_touches[t_slot].m_y = p_y;
    
    if (!widgetIsInRunMode(p_widget))
        return;
    
    // Send an event to the widget
    if (p_widget->handlesTouches())
        p_widget->OnTouchStart(p_id, p_x, p_y, 0.0, 0.0);
    else if (p_id == 1)
    {
        mouseMove(p_widget, p_x, p_y);
        mouseDown(p_widget, 1);
    }
#endif
}

void MCWidgetEventManager::touchMove(MCWidgetRef p_widget, uinteger_t p_id, coord_t p_x, coord_t p_y)
{
#if 0
    // Does this touch event exist in the list yet? If not, create it. (This
    // might happen if a touch starts on a non-widget MCControl and later moves
    // onto a widget).
    uinteger_t t_slot;
    if (!findTouchSlot(p_id, t_slot))
    {
        touchBegin(p_widget, p_id, p_x, p_y);
        /* UNCHECKED */ findTouchSlot(p_id, t_slot);
    }
    else
    {
        // Pre-existing touch. Check for enter/leave events.
        if (m_touches[t_slot].m_widget != p_widget)
        {
            touchLeave(m_touches[t_slot].m_widget, p_id);
            m_touches[t_slot].m_widget = p_widget;
            touchEnter(p_widget, p_id);
        }
        
        // Update the touch status
        m_touches[t_slot].m_x = p_x;
        m_touches[t_slot].m_y = p_y;
        
        if (!widgetIsInRunMode(p_widget))
            return;
        
        // Send a move event
        if (p_widget->handlesTouches())
            p_widget->OnTouchMove(p_id, p_x, p_y, 0, 0);
        else if (p_id == 1)
        {
            mouseMove(p_widget, p_x, p_y);
        }
    }
#endif
}

void MCWidgetEventManager::touchEnd(MCWidgetRef p_widget, uinteger_t p_id, coord_t p_x, coord_t p_y)
{
#if 0
    // Ignore the event if this touch has not been registered
    uinteger_t t_slot;
    if (!findTouchSlot(p_id, t_slot))
        return;
    
    // Update touch status
    m_touches[t_slot].m_x = p_x;
    m_touches[t_slot].m_y = p_y;
    
    if (!widgetIsInRunMode(p_widget))
        return;
    
    // Send a touch finish event
    if (p_widget->handlesTouches())
        p_widget->OnTouchFinish(p_id, p_x, p_y);
    else if (p_id == 1)
    {
        mouseMove(p_widget, p_x, p_y);
        mouseUp(p_widget, 1);
    }
    
    // Delete the event
    freeTouchSlot(t_slot);
#endif
}

void MCWidgetEventManager::touchCancel(MCWidgetRef p_widget, uinteger_t p_id, coord_t p_x, coord_t p_y)
{
#if 0
    // Ignore the event if this touch has not been registered
    uinteger_t t_slot;
    if (!findTouchSlot(p_id, t_slot))
        return;
    
    // Update touch status
    m_touches[t_slot].m_x = p_x;
    m_touches[t_slot].m_y = p_y;
    
    if (!widgetIsInRunMode(p_widget))
        return;
    
    // Send a touch cancel event
    if (p_widget->handlesTouches())
        p_widget->OnTouchCancel(p_id);
    else if (p_id == 1)
    {
        mouseMove(p_widget, p_x, p_y);
        mouseRelease(p_widget, 1);
    }
    
    // Delete the event
    freeTouchSlot(t_slot);
#endif
}

void MCWidgetEventManager::touchEnter(MCWidgetRef p_widget, uinteger_t p_id)
{
#if 0
    if (!widgetIsInRunMode(p_widget))
        return;
    
    if (p_widget->handlesTouches())
        p_widget->OnTouchEnter(p_id);
    else if (p_id == 1)
    {
        mouseEnter(p_widget);
    }
#endif
}

void MCWidgetEventManager::touchLeave(MCWidgetRef p_widget, uinteger_t p_id)
{
#if 0
    if (!widgetIsInRunMode(p_widget))
        return;
    
    if (p_widget->handlesTouches())
        p_widget->OnTouchLeave(p_id);
    else if (p_id == 1)
    {
        mouseLeave(p_widget);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////

uinteger_t MCWidgetEventManager::allocateTouchSlot()
{
    // Search the list for an empty slot
    for (uinteger_t i = 0; i < m_touches.Size(); i++)
    {
        // Look for a slot with no widget
        if (m_touches[i].m_widget == nil)
        {
            return i;
        }
    }
    
    // No empty slots found. Extend the array.
    /* UNCHECKED */ m_touches.Extend(m_touches.Size() + 1);
    return m_touches.Size() - 1;
}

bool MCWidgetEventManager::findTouchSlot(uinteger_t p_id, uinteger_t& r_which)
{
    // Search the list for a touch event with the given ID
    for (uinteger_t i = 0; i < m_touches.Size(); i++)
    {
        if (m_touches[i].m_id == p_id)
        {
            r_which = i;
            return true;
        }
    }
    
    // Could not find any touch event with that ID
    return false;
}

void MCWidgetEventManager::freeTouchSlot(uinteger_t p_which)
{
    // Mark as free by clearing the widget pointer for the event
    m_touches[p_which].m_widget = nil;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWidgetEventManager::widgetIsInRunMode(MCWidgetRef p_widget)
{
    MCWidget *t_host;
    t_host = MCWidgetGetHost(p_widget);
    Tool t_tool = t_host -> getstack() -> gettool(t_host);
    return t_tool == T_BROWSE || t_tool == T_HELP;
}
