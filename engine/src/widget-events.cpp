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

#include "globals.h"
#include "context.h"

#include "widget-events.h"

////////////////////////////////////////////////////////////////////////////////

MCWidgetEventManager* MCwidgeteventmanager = nil;

////////////////////////////////////////////////////////////////////////////////


void MCWidgetEventManager::event_open(MCWidget* p_widget)
{
    p_widget->OnOpen();
}

void MCWidgetEventManager::event_close(MCWidget* p_widget)
{
    p_widget->OnClose();
}

void MCWidgetEventManager::event_kfocus(MCWidget* p_widget)
{
    // Keyboard focus has changed
    m_keyboard_focus = p_widget;
    
    p_widget->OnFocusEnter();
}

void MCWidgetEventManager::event_kunfocus(MCWidget* p_widget)
{
    // Keyboard focus has changed
    // TODO: does the unfocus *always* happen before the next focus?
    m_keyboard_focus = nil;
    
    p_widget->OnFocusLeave();
}

Boolean MCWidgetEventManager::event_kdown(MCWidget* p_widget, MCStringRef p_text, KeySym p_key)
{
    
}

Boolean MCWidgetEventManager::event_kup(MCWidget* p_widget, MCStringRef p_text, KeySym p_key)
{
    
}

Boolean MCWidgetEventManager::event_mdown(MCWidget* p_widget, uint2 p_which)
{
    return mouseDown(p_widget, p_which);
}

Boolean MCWidgetEventManager::event_mup(MCWidget* p_widget, uint2 p_which, bool p_release)
{
    if (p_release)
        return mouseRelease(p_widget, p_which);
    else
        return mouseUp(p_widget, p_which);
}

Boolean MCWidgetEventManager::event_mfocus(MCWidget* p_widget, int2 p_x, int2 p_y)
{
    // Cursor position has changed
    m_mouse_x = p_x;
    m_mouse_y = p_y;
    
    // Do a quick bounds test on the targeted widget. If this fails, the widget
    // wasn't the target of the mouse focus event.
    MCRectangle p_rect;
    p_rect = MCU_make_rect(p_x, p_y, 1, 1);
    bool t_within_bounds;
    p_widget->OnBoundsTest(p_rect, t_within_bounds);
    if (!t_within_bounds)
        return False;

    // Do a more thorough hit test
    bool t_hit;
    p_widget->OnHitTest(p_rect, t_hit);
    if (!t_hit)
        return False;
    
    // Has the focus state for this widget changed?
    if (p_widget != m_mouse_focus)
    {
        // Mouse has entered this widget
        m_mouse_focus = p_widget;
        mouseEnter(p_widget);
    }
    else
    {
        // Mouse has moved within this widget
        mouseMove(p_widget);
    }
    
    // This event was handled
    return True;
}

void MCWidgetEventManager::event_munfocus(MCWidget* p_widget)
{
    // TODO: cancel events for held mouse buttons
    
    // Widget has lost focus
    m_mouse_focus = nil;
}

Boolean MCWidgetEventManager::event_doubledown(MCWidget* p_widget, uint2 p_which)
{
    // Because we do gesture recognition ourselves, treat this as a normal click
    return event_mdown(p_widget, p_which);
}

Boolean MCWidgetEventManager::event_doubleup(MCWidget* p_widget, uint2 p_which)
{
    // Because we do gesture recognition ourselves, treat this as a normal click
    return event_mup(p_widget, p_which, false);
}

void MCWidgetEventManager::event_timer(MCWidget* p_widget, MCNameRef p_message, MCParameter* p_parameters)
{
    
}

void MCWidgetEventManager::event_setrect(MCWidget* p_widget, const MCRectangle& p_rectangle)
{
    p_widget->OnGeometryChanged(p_rectangle);
}

void MCWidgetEventManager::event_recompute(MCWidget* p_widget)
{
    // This gets called whenever certain parent (group, card, stack) properties
    // are changed. Unfortunately, we have no information as to *what* changed.
    p_widget->OnParentPropChanged();
}

void MCWidgetEventManager::event_draw(MCDC* p_dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite)
{
    // Ignored parameter: p_isolated
    // Ignored parameter: p_sprite
    
}

////////////////////////////////////////////////////////////////////////////////

uinteger_t MCWidgetEventManager::GetMouseButtonState() const
{
    return m_mouse_buttons;
}

MCWidget* MCWidgetEventManager::GetMouseWidget() const
{
    return m_mouse_focus;
}

////////////////////////////////////////////////////////////////////////////////

void MCWidgetEventManager::mouseMove(MCWidget* p_widget)
{
    
}

void MCWidgetEventManager::mouseEnter(MCWidget* p_widget)
{
    
}

void MCWidgetEventManager::mouseLeave(MCWidget* p_widget)
{
    
}

bool MCWidgetEventManager::mouseDown(MCWidget* p_widget, uinteger_t p_which)
{
    // Mouse button is down
    m_mouse_buttons |= (1 << p_which);
    
    // If the widget handles clicks or mouse down events, accept the event
    bool t_accepted;
    t_accepted = false;
    if (p_widget->wantsClicks())
    {
        t_accepted = true;
    }
    
    // Independent "if" because a widget could conceivably want both
    if (p_widget->handlesMouseDown())
    {
        t_accepted = true;
        p_widget->OnMouseDown(m_mouse_x, m_mouse_y, p_which);
    }
    
    return t_accepted;
}

bool MCWidgetEventManager::mouseUp(MCWidget* p_widget, uinteger_t p_which)
{
    // Mouse button is no longer down
    m_mouse_buttons &= ~(1 << p_which);
    
    // If the widget wants click gestures, process it as such
    bool t_accepted;
    t_accepted = false;
    if (p_widget->wantsClicks())
    {
        mouseClick(p_widget, p_which);
        t_accepted = true;
    }
    // If the widget handles mouse up, send it the event
    else if (p_widget->handlesMouseUp())
    {
        p_widget->OnMouseUp(m_mouse_x, m_mouse_y, p_which);
        t_accepted = true;
    }
    
    return t_accepted;
}

void MCWidgetEventManager::mouseClick(MCWidget* p_widget, uinteger_t p_which)
{
    // Send the mouse up event before the click recognition
    p_widget->OnMouseUp(m_mouse_x, m_mouse_y, p_which);
    
    // Basic gesture processing: we currently only look for double- (or other
    // multi-) click events. More can be added later if desired.
    
    // We only detect double-click events for button 1
    if (p_which == 1 && m_click_button == 1)
    {
        // Do the position change and time since the last click make this a double
        // (or triple or more...) click?
        if (m_click_time + m_doubleclick_time <= MCeventtime
            && fabs(m_mouse_x - m_click_x) <= m_doubleclick_distance
            && fabs(m_mouse_y - m_click_y) <= m_doubleclick_distance)
        {
            // Within the limits - this is a multiple-click event
            m_click_count++;
        }
        else
        {
            // Outside the limits. Only a single click.
            m_click_count = 1;
        }
        
        // Send a double click event to the widget, if appropriate. If the user
        // clicks multiple times in rapid sequence, the widget should receive
        // multiple double-clicks (one per two clicks) rather than only the one.
        if ((m_click_count & 1) == 0            // Click count is event
            && p_widget->wantsDoubleClicks())
        {
            p_widget->OnDoubleClick(m_mouse_x, m_mouse_y, p_which);
        }
        // If this is the first click in a sequence and the widget is happy to
        // wait for gesture processing, suppress the first click.
        else if (m_click_count == 1 && p_widget->waitForDoubleClick())
        {
            // TODO: set up a timer that expires after the double click interval
        }
        else
        {
            // Single or multiple click event
            p_widget->OnClick(m_mouse_x, m_mouse_y, p_which, m_click_count);
        }
    }
    else
    {
        // Not a double-click gesture; just send the click event
        p_widget->OnClick(m_mouse_x, m_mouse_y, p_which, 1);
    }
    
    // Update the time and location for the last mouse button event
    m_click_x = m_mouse_x;
    m_click_y = m_mouse_y;
    m_click_time = MCeventtime;
    m_click_button = p_which;
}

bool MCWidgetEventManager::mouseRelease(MCWidget* p_widget, uinteger_t p_which)
{
    // Mouse button is no longer down
    m_mouse_buttons &= ~(1 << p_which);
    
    // Send a mouse release event if the widget handles it
    bool t_accepted;
    t_accepted = false;
    if (p_widget->handlesMouseRelease())
    {
        p_widget->OnMouseCancel(p_which);
        t_accepted = true;
    }
    
    return t_accepted;
}

