#ifndef __MC_WIDGET_EVENTS__
#define __MC_WIDGET_EVENTS__

#include "widget.h"

class MCWidgetEventManager;
extern MCWidgetEventManager* MCwidgeteventmanager;

class MCWidgetEventManager
{
public:
    
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
    void event_draw(MCDC *p_dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite);
    
    // Returns a bitmask of the mouse buttons that are pressed
    uinteger_t GetMouseButtonState() const;
    
    // Returns the widget that the mouse is currently focused on
    MCWidget* GetMouseWidget() const;
    
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
    
    
    void mouseMove(MCWidget*);
    void mouseEnter(MCWidget*);
    void mouseLeave(MCWidget*);
    bool mouseDown(MCWidget*, uinteger_t p_which);
    bool mouseUp(MCWidget*, uinteger_t p_which);
    void mouseClick(MCWidget*, uinteger_t p_which);
    bool mouseRelease(MCWidget*, uinteger_t p_which);
    
    // Returns whether the given widget is operating in run (browse) mode or not
    bool inRunMode(MCWidget*);
};

#endif // ifndef __MC_WIDGET_EVENTS__

