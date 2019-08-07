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
#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"
#include "stack.h"
#include "card.h"
#include "field.h"
#include "mode.h"
#include "dispatch.h"
#include "eventqueue.h"
#include "debug.h"
#include "group.h"
#include "widget-events.h"

#include "resolution.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

extern Boolean tripleclick;

#ifdef _MOBILE
static void handle_touch(MCStack *p_stack, MCEventTouchPhase p_phase, uint32_t p_id, uint32_t p_taps, int32_t x, int32_t y);
#endif

////////////////////////////////////////////////////////////////////////////////

enum MCEventType
{
	kMCEventTypeNotify,
	
	kMCEventTypeQuitApp,
	kMCEventTypeSuspendApp,
	kMCEventTypeResumeApp,
	
	kMCEventTypeUpdateMenu,
	kMCEventTypeMenuPick,
	
	kMCEventTypeWindowReshape,

	kMCEventTypeMouseFocus,
	kMCEventTypeMousePress,
	kMCEventTypeMouseWheel,
	kMCEventTypeMousePosition,

	kMCEventTypeKeyFocus,
	kMCEventTypeKeyPress,

	kMCEventTypeImeCompose,

	kMCEventTypeTouch,
	kMCEventTypeMotion,
	kMCEventTypeAcceleration,
	kMCEventTypeOrientation,
	kMCEventTypeLocation,
	kMCEventTypeHeading,
	
	kMCEventTypeCustom,
};

// Pointers to objects inside this structure shouldn't really be a raw
// MCObjectProxy* but we can't embed the MCObjectHandle RAII class here as it
// cannot be placed inside a union (without some C++11 magic).
// [[ C++11 ]] Refactor this to store the handles directly
struct MCEvent
{
	MCEvent *next;
	MCEventType type;
	union
	{
		struct
		{
			MCEventQueueNotifyCallback callback;
			void *state;
		} notify;

		struct
		{
			MCObjectProxy<MCStack>* stack;
			MCGFloat scale;
		} window;
		
		struct
		{
            
            MCObjectProxy<>* target;
			union 
			{
				struct
				{
                    // SN-2014-06-23: pick updated to StringRef
					MCStringRef string;
				} pick;
			};
		} menu;
		
		struct
		{
			uint32_t time;
			MCObjectProxy<MCStack>* stack;
			union
			{
				struct
				{
					bool inside;
				} focus;
				struct
				{
					uint32_t modifiers;
					MCMousePressState state;
					int32_t button;
				} press;
				struct
				{
					uint32_t modifiers;
					int32_t dh, dv;
				} wheel;
				struct
				{
					uint32_t modifiers;
					int32_t x, y;
				} position;
			};
		} mouse;

		struct
		{
			MCObjectProxy<MCStack>* stack;
			union
			{
				struct
				{
					bool owner;
				} focus;
				struct
				{
					uint32_t modifiers;
					uint32_t key_code;
					uint32_t char_code;
				} press;
			};
		} key;

		struct
		{
			MCObjectProxy<MCStack>* stack;
			union
			{
				struct
				{
					bool enabled;
					uint32_t offset;
					uint32_t char_count;
					uint16_t *chars;
				} compose;
			};
		} ime;
		
		struct
		{
			MCObjectProxy<MCStack>* stack;
			MCEventTouchPhase phase;
			uint32_t id;
			uint32_t taps;
			int32_t x;
			int32_t y;
		} touch;
		
		struct
		{
			MCObjectProxy<MCStack>* stack;
			MCEventMotionType type;
		} motion;
		
		struct
		{
			double x, y, z, t;
		} acceleration;
		
		struct
		{
			const char *error;
		} location;
		
		struct
		{
			MCCustomEvent *event;
		} custom;
	};
};

typedef bool (*MCEventQueueFilterCallback)(void *context, MCEvent *event);

static MCEvent *s_first_event = nil;
static MCEvent *s_last_event = nil;

static uint32_t s_click_time = 0;
static uint32_t s_click_count = 0;

////////////////////////////////////////////////////////////////////////////////

bool MCEventQueueInitialize(void)
{
	s_first_event = nil;
	s_last_event = nil;
	return true;
}

void MCEventQueueFinalize(void)
{
	while(s_first_event != nil)
	{
		MCEvent *t_event;
		t_event = s_first_event;
		s_first_event = s_first_event -> next;

		switch(t_event -> type)
		{
		case kMCEventTypeNotify:
			t_event -> notify . callback(t_event -> notify . state, false);
			break;
		default:
			break;
		}

		if (t_event -> type == kMCEventTypeImeCompose)
			MCMemoryDeleteArray(t_event -> ime . compose . chars);

		MCMemoryDelete(t_event);
	}

	s_first_event = nil;
	s_last_event = nil;
}

////////////////////////////////////////////////////////////////////////////////

// Important: This function is on the emterpreter whitelist. If its
// signature function changes, the mangled name must be updated in
// em-whitelist.json
static void MCEventQueueDispatchEvent(MCEvent *p_event)
{
	MCEvent *t_event;
	t_event = p_event;

	MCObject *t_menu;
	t_menu = MCdispatcher -> getmenu();

	switch(t_event -> type)
	{
	case kMCEventTypeNotify:
		t_event -> notify . callback(t_event -> notify . state, true);
		break;
			
	case kMCEventTypeQuitApp:
	{
		switch(MCdefaultstackptr->getcard()->message(MCM_shut_down_request))
		{
			case ES_PASS:
			case ES_NOT_HANDLED:
				MCdefaultstackptr->getcard()->message(MCM_shut_down);
				MCquit = True;
				MCexitall = True;
				MCtracestackptr = nil;
				MCtraceabort = True;
				MCtracereturn = True;
				break;
			default:
				break;
		}
	}
	break;

	case kMCEventTypeSuspendApp:
		MCdefaultstackptr->getcard()->message(MCM_suspend);
		break;
		
	case kMCEventTypeResumeApp:
		MCdefaultstackptr->getcard()->message(MCM_resume);
		break;
			
	case kMCEventTypeUpdateMenu:
	{
		MCObjectHandle t_target = t_event->menu.target;
		if (t_target.IsValid())
			t_target->message_with_valueref_args(MCM_mouse_down, kMCEmptyString);
	}
	break;

	case kMCEventTypeMenuPick:
	{
		MCObjectHandle t_target = t_event->menu.target;
		if (t_target.IsValid())
            // SN-2014-06-23: pick updated to StringRef
			t_target->message_with_valueref_args(MCM_menu_pick, t_event -> menu . pick . string);
	}
	break;
			
	case kMCEventTypeWindowReshape:
    {
		MCStackHandle t_stack = t_event->window.stack;
        if (t_stack.IsValid())
        {
            t_stack->view_setbackingscale(t_event->window.scale);
            t_stack->view_configure(true);
        }
		break;
    }
			
	case kMCEventTypeMouseFocus:
		if (t_event -> mouse . focus . inside)
		{
			if (MCmousestackptr != t_event -> mouse . stack)
			{
				MCmousestackptr = t_event -> mouse . stack;
				MCmousestackptr -> enter();
			}

			if (t_menu == nil)
				MCmousestackptr -> mfocus(MCmousex, MCmousey);
			else
				t_menu -> mfocus(MCmousex, MCmousey);
		}
		else if (MCmousestackptr == t_event -> mouse . stack)
		{
			MCmousestackptr -> munfocus();
			MCmousestackptr = nil;
		}
		break;

	case kMCEventTypeMousePress:
		if (MCmousestackptr == t_event -> mouse . stack || t_menu != nil)
		{
			if (t_event -> mouse . press . state == kMCMousePressStateDown)
				MCbuttonstate |= (1 << t_event -> mouse . press . button);
			else
				MCbuttonstate &= ~(1 << t_event -> mouse . press . button);

			if (t_event -> mouse . press . state == kMCMousePressStateDown)
			{
				if (t_event -> mouse . time - s_click_time < MCdoubletime &&
					MCU_abs(MCclicklocx - MCmousex) < MCdoubledelta &&
					MCU_abs(MCclicklocy - MCmousey) < MCdoubledelta)
					s_click_count += 1;
				else
					s_click_count = 0;
                
                MCclicklocx = MCmousex;
                MCclicklocy = MCmousey;
			}
			else
				s_click_time = t_event -> mouse . time;

			MCeventtime = t_event -> mouse . time;
			MCmodifierstate = t_event -> mouse . press . modifiers;
			MCclickstackptr = MCmousestackptr;

			MCObject *t_target;
			t_target = t_menu != nil ? t_menu : MCclickstackptr;

			if (t_event -> mouse . press . state == kMCMousePressStateDown)
			{
				tripleclick = s_click_count == 2;

				if (s_click_count != 1)
					t_target -> mdown(t_event -> mouse . press . button + 1);
				else
					t_target -> doubledown(t_event -> mouse . press . button + 1);
			}
			else if (t_event -> mouse . press . state == kMCMousePressStateUp)
			{
				if (s_click_count != 1)
					t_target -> mup(t_event -> mouse . press . button + 1, false);
				else
					t_target -> doubleup(t_event -> mouse . press . button + 1);
			}
			else
			{
				s_click_count = 0;
				tripleclick = False;
			
				// If the press was 'released' i.e. cancelled then we stop messages, mup then
				// dispatch a mouseRelease message ourselves.
                
                // FG-2013-10-09 [[ Bugfix 11208 ]]
                // CS_NO_MESSAGES only applies to the target and not the controls it contains
                // so the mouse up message (on mouseUp) sets sent when it isn't desired
                // Hopefully nobody depends on the old behaviour...
            
                //t_target -> setstate(True, CS_NO_MESSAGES);
				//t_target -> mup(t_event -> mouse . press . button + 1);
				//t_target -> setstate(False, CS_NO_MESSAGES);
                
                bool old_lock = MClockmessages;
                MClockmessages = true;
                t_target -> mup(t_event -> mouse . press . button + 1, false);
                MClockmessages = old_lock;
				
				t_target -> message_with_args(MCM_mouse_release, t_event -> mouse . press . button + 1);
			}
		}
		break;

	case kMCEventTypeMouseWheel:
		// Notice that we recompute mfocused twice - this is because calling the key handler
		// could invalidate mfocused in between.
		if (MCmousestackptr == t_event -> mouse . stack)
		{
			MCObject *mfocused;
			
			mfocused = MCmousestackptr->getcard()->getmfocused();
			if (mfocused == NULL)
				mfocused = MCmousestackptr -> getcard();
			if (mfocused == NULL)
				mfocused = MCmousestackptr;
			
			MCeventtime = t_event -> mouse . time;
			MCmodifierstate = t_event -> mouse . wheel . modifiers;
			if (t_event -> mouse . wheel . dv != 0)
				mfocused -> kdown(kMCEmptyString, t_event -> mouse . wheel . dv < 0 ? XK_WheelUp : XK_WheelDown);
			
			mfocused = MCmousestackptr->getcard()->getmfocused();
			if (mfocused == NULL)
				mfocused = MCmousestackptr -> getcard();
			if (mfocused == NULL)
				mfocused = MCmousestackptr;
			
			if (t_event -> mouse . wheel . dh != 0)
				mfocused -> kdown(kMCEmptyString, t_event -> mouse . wheel . dh < 0 ? XK_WheelLeft : XK_WheelRight);
		}
		break;

	case kMCEventTypeMousePosition:
		if (MCmousestackptr == t_event -> mouse . stack || t_menu != nil)
		{
			MCeventtime = t_event -> mouse . time;
			MCmodifierstate = t_event -> mouse . position . modifiers;

			MCObject *t_target;
			t_target = t_menu != nil ? t_menu : MCmousestackptr;
			
			// IM-2013-09-30: [[ FullscreenMode ]] Translate mouse location to stack coords
			MCPoint t_mouseloc;
			t_mouseloc = MCPointMake(t_event->mouse.position.x, t_event->mouse.position.y);
			
			// IM-2013-10-03: [[ FullscreenMode ]] Transform mouseloc based on the mousestack
			t_mouseloc = MCmousestackptr->windowtostackloc(t_mouseloc);
			
			MCmousex = t_mouseloc.x;
			MCmousey = t_mouseloc.y;

			t_target -> mfocus(t_mouseloc . x, t_mouseloc . y);
		}
		break;

	case kMCEventTypeKeyFocus:
    {
		MCStackHandle t_stack = t_event->key.stack;
		
		if (t_stack.IsValid())
		{
			if (t_event -> key . focus . owner)
				t_stack->kfocus();
			else
				t_stack->kunfocus();
		}
		break;
    }

	case kMCEventTypeKeyPress:
		{
			MCStackHandle t_stack = t_event->key.stack;
            
            MCObject *t_target = t_menu != nil ? t_menu : t_stack;

			MCmodifierstate = t_event -> key . press . modifiers;

			// If 'char_code' is 0, then this key press has not generated a
			// character.
			if (t_event -> key . press . char_code == 0)
			{
				t_target -> kdown(kMCEmptyString, t_event -> key . press . key_code);
				t_target -> kup(kMCEmptyString, t_event -> key . press . key_code);
				break;
			}

			// Otherwise 'char_code' is the unicode codepoint, so first map to
			// UTF-16 codeunits
			unichar_t t_unichars[2];
            uindex_t t_length = 1;
            if (MCUnicodeCodepointToSurrogates(t_event->key.press.char_code, t_unichars[0], t_unichars[1]))
                t_length = 2;

			// Now the string is created with the appropriate unicode-capable function
			MCAutoStringRef t_buffer;
            MCStringCreateWithChars(t_unichars, t_length, &t_buffer);
			t_target -> kdown(*t_buffer, t_event -> key . press . key_code);
			t_target -> kup(*t_buffer, t_event -> key . press . key_code);
		}
		break;

	case kMCEventTypeImeCompose:
		{
			if (!MCactivefield)
				break;

			if (t_event -> ime . compose . enabled)
				MCactivefield -> startcomposition();
			else
				MCactivefield -> stopcomposition(True, False);

			MCactivefield -> setcompositioncursoroffset(t_event -> ime . compose . offset * 2);

			MCAutoStringRef t_unichars;
			MCStringCreateWithChars((const unichar_t *)t_event->ime.compose.chars, t_event->ime.compose.char_count, &t_unichars);
			
			// MW-2012-02-13: [[ Block Unicode ]] Use the new 'finsert' method in
			//   unicode mode.
			MCactivefield -> finsertnew(FT_IMEINSERT, *t_unichars, LCH_UNICODE);
			if (t_event -> ime . compose . enabled)
			{
				MCRectangle r;
				MCactivefield -> getcompositionrect(r, -1);
				MCModeConfigureIme(MCactivefield -> getstack(), true, r . x, r . y + r . height);
			}
		}
        break;
            
    case kMCEventTypeCustom:
        t_event -> custom . event -> Dispatch();
        break;
		
	case kMCEventTypeTouch:

	{
#ifdef _MOBILE
		MCStackHandle t_stack(t_event->touch.stack);
        if (t_stack)
        {
            handle_touch(t_stack, t_event->touch.phase, t_event->touch.id, t_event->touch.taps, t_event->touch.x, t_event->touch.y);
        }
#else
        MCUnreachable();
#endif

		break;
	}
		
	case kMCEventTypeMotion:
#ifdef _MOBILE
		{
			MCNameRef t_message;
			MCStringRef t_motion;
			switch(t_event -> motion . type)
			{
				case kMCEventMotionShakeBegan:
					t_motion = MCSTR("shake");
					t_message = MCM_motion_start;
					break;
				case kMCEventMotionShakeEnded:
					t_motion = MCSTR("shake");
					t_message = MCM_motion_end;
					break;
				case kMCEventMotionShakeCancelled:
					t_motion = MCSTR("shake");
					t_message = MCM_motion_release;
					break;
			}
			
			MCdefaultstackptr -> getcurcard() -> message_with_valueref_args(t_message, t_motion);
		}
#else
		MCUnreachable();
#endif
		break;
		
	case kMCEventTypeAcceleration:
#ifdef _MOBILE
		{
			MCAutoStringRef t_value;
            /* UNCHECKED */ MCStringFormat(&t_value, "%.6f,%.6f,%.6f,%f", t_event -> acceleration . x, t_event -> acceleration . y, t_event -> acceleration . z, t_event -> acceleration . t);
			MCdefaultstackptr -> getcurcard() -> message_with_valueref_args(MCM_acceleration_changed, *t_value);
		}
#else
		MCUnreachable();
#endif
		break;
		
	case kMCEventTypeOrientation:
#ifdef _MOBILE
		MCdefaultstackptr -> getcurcard() -> message(MCM_orientation_changed);
#else
		MCUnreachable();
#endif
		break;
		
	case kMCEventTypeLocation:
#ifdef _MOBILE
		MCdefaultstackptr -> getcurcard() -> message(t_event -> location . error == nil ? MCM_location_changed : MCM_location_error);
#else
		MCUnreachable();
#endif
		break;
		
	case kMCEventTypeHeading:
#ifdef _MOBILE
		MCdefaultstackptr -> getcurcard() -> message(t_event -> location . error == nil ? MCM_heading_changed : MCM_heading_error);
#else
		MCUnreachable();
#endif
		break;
	}
}

static void MCEventQueueRemoveEvent(MCEvent *p_event)
{
	if (s_first_event == p_event)
	{
		s_first_event = p_event -> next;
		if (s_first_event == nil)
			s_last_event = nil;
	}
	else
	{
		MCEvent *t_previous;
		for(t_previous = s_first_event; t_previous -> next != p_event; t_previous = t_previous -> next)
			;
		t_previous -> next = p_event -> next;
		if (s_last_event == p_event)
			s_last_event = t_previous;
	}
}

static MCObjectProxy<MCStack>* MCEventQueueGetEventStack(MCEvent *p_event)
{
	switch(p_event -> type)
	{
		case kMCEventTypeWindowReshape:
			return p_event -> window . stack;
			
		case kMCEventTypeMouseFocus:
		case kMCEventTypeMousePress:
		case kMCEventTypeMouseWheel:
		case kMCEventTypeMousePosition:
			return p_event -> mouse . stack;
			
		case kMCEventTypeKeyFocus:
		case kMCEventTypeKeyPress:
			return p_event -> key . stack;
			
		case kMCEventTypeImeCompose:
			return p_event -> ime . stack;
			
		default:
			break;
	}
	
	return nil;
}

static void MCEventQueueDestroyEvent(MCEvent *p_event)
{
    if (p_event -> type == kMCEventTypeImeCompose)
        MCMemoryDeleteArray(p_event -> ime . compose . chars);
    else if (p_event -> type == kMCEventTypeUpdateMenu)
    {
		MCObjectHandle(p_event->menu.target).ExternalRelease();
    }
    else if (p_event -> type == kMCEventTypeMenuPick)
    {
		MCObjectHandle(p_event->menu.target).ExternalRelease();
        MCValueRelease(p_event -> menu . pick . string);
    }
    else if (MCEventQueueGetEventStack(p_event) != nil)
    {
        MCStackHandle(MCEventQueueGetEventStack(p_event)).ExternalRelease();
    }
    else if (p_event -> type == kMCEventTypeCustom)
    {
        p_event -> custom . event -> Destroy();
    }
    
    MCMemoryDelete(p_event);
}

bool MCEventQueueDispatch(void)
{
	if (s_first_event == nil)
		return false;

	MCEvent *t_event;
	t_event = s_first_event;
	if (t_event -> next == nil)
		s_first_event = s_last_event = nil;
	else
		s_first_event = s_first_event -> next;

	MCEventQueueDispatchEvent(t_event);
	
	MCEventQueueDestroyEvent(t_event);
	
	return true;
}

void MCEventQueueFlush(MCStack *p_stack)
{
	bool t_changed;
	do
	{
		t_changed = false;
		for(MCEvent *t_event = s_first_event; t_event != nil; t_event = t_event -> next)
        {
			MCStackHandle t_stack = MCEventQueueGetEventStack(t_event);
            
            // Remove events referencing this stack or any dead stack
            if (t_stack.IsBound() && (!t_stack.IsValid() || t_stack.UnsafeGet() == p_stack))
			{
				MCEventQueueRemoveEvent(t_event);
				MCEventQueueDestroyEvent(t_event);
				t_changed = true;
				break;
			}
        }
	}
	while(t_changed);
}

void MCEventQueueFilter(MCEventQueueFilterCallback p_callback, void *p_context)
{
	MCEvent *t_new_first_event, *t_new_last_event;
	t_new_first_event = nil;
	t_new_last_event = nil;
	
	MCEvent *t_event;
	t_event = s_first_event;
	while(t_event != nil)
	{
		MCEvent *t_next_event;
		t_next_event = t_event -> next;
		
		if (p_callback(p_context, t_event))
		{
			if (t_new_last_event == nil)
				t_new_first_event = t_event;
			else
				t_new_last_event -> next = t_event;
			t_new_last_event = t_event;
		}
		else
			MCEventQueueDestroyEvent(t_event);
		
		t_event = t_next_event;
	}
	
	s_first_event = t_new_first_event;
	s_last_event = t_new_last_event;
	
	if (s_last_event != nil)
		s_last_event -> next = nil;
}

// Search through the eventqueue for a mouse click (mousedown followed by mouseup)
// and remove it from the queue. This will also remove all but the last mousemove.
bool MCEventQueueGetMouseClick(uint32_t p_button)
{
	// Look for the first mouse down event in the queue
	MCEvent *t_mouse_down, *t_mouse_move;
	t_mouse_down = nil;
    t_mouse_move = nil;
	for(MCEvent *t_event = s_first_event; t_event != nil; t_event = t_event -> next)
	{
		if (t_event -> type == kMCEventTypeMousePosition)
			t_mouse_move = t_event;
		if (t_event -> type == kMCEventTypeMousePress &&
			t_event -> mouse . press . state == kMCMousePressStateDown &&
		    (p_button == 0 || (uint32_t) t_event -> mouse . press . button == p_button))
		{
			t_mouse_down = t_event;
			break;
		}
	}
	
	if (t_mouse_down == nil)
		return false;
	
	// Look for a subsequent mouse up event in the queue
	MCEvent *t_mouse_up;
	t_mouse_up = nil;
	for(MCEvent *t_event = t_mouse_down -> next; t_event != nil; t_event = t_event -> next)
		if (t_event -> type == kMCEventTypeMousePress &&
			t_event -> mouse . press . state == kMCMousePressStateUp &&
		    (p_button == 0 || (uint32_t) t_event -> mouse . press . button == p_button))
		{
			t_mouse_up = t_event;
			break;
		}
	
	if (t_mouse_up == nil)
		return false;
	
	MCmodifierstate = t_mouse_up -> mouse . press . modifiers;
	MCclickstackptr = MCmousestackptr;
    
    // If there is a mouse-move event then update the clickloc with that position
    // otherwise use MCmousex/y.
    if (t_mouse_move != nil)
    {
        // Take into account fullscreenmode.
        MCPoint t_mouseloc;
        t_mouseloc = MCPointMake(t_mouse_move->mouse.position.x, t_mouse_move->mouse.position.y);
        t_mouseloc = MCmousestackptr->windowtostackloc(t_mouseloc);
        
        MCclicklocx = t_mouseloc . x;
        MCclicklocy = t_mouseloc . y;
	}
    else
    {
        MCclicklocx = MCmousex;
        MCclicklocy = MCmousey;
    }
    
	// Now remove *all* mouse events from the queue up to and including the
	// mouse up.
	MCEvent *t_event;
	t_event = s_first_event;
	for(;;)
	{
		MCEvent *t_next;
		t_next = t_event -> next;
		
		if (t_event -> type == kMCEventTypeMouseFocus ||
			t_event -> type == kMCEventTypeMousePosition ||
			t_event -> type == kMCEventTypeMousePress ||
			t_event -> type == kMCEventTypeMouseWheel)
		{
			MCEventQueueRemoveEvent(t_event);
			MCEventQueueDestroyEvent(t_event);
		}
		
		if (t_event == t_mouse_up)
			break;
		
		t_event = t_next;
	}
	
	return true;
}

static bool MCEventQueueEventIsMouse(MCEvent *p_event)
{
	return p_event -> type == kMCEventTypeMouseFocus ||
		p_event -> type == kMCEventTypeMousePosition ||
		p_event -> type == kMCEventTypeMousePress ||
		p_event -> type == kMCEventTypeMouseWheel;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCEventQueuePost(MCEventType p_type, MCEvent*& r_event)
{
	MCEvent *t_event;
	if (!MCMemoryNew(t_event))
		return false;

	if (s_last_event == nil)
		s_first_event = s_last_event = t_event;
	else
	{
		s_last_event -> next = t_event;
		s_last_event = t_event;
	}

	t_event -> type = p_type;
	
	// MW-2011-08-16: [[ Wait ]] Ping any running wait loop to make sure it
	//   picks up the new event if it wants to.
	MCscreen -> pingwait();
	
	r_event = t_event;

	return true;
}

static bool MCEventQueuePostAtFront(MCEventType p_type, MCEvent*& r_event)
{
	MCEvent *t_event;
	if (!MCMemoryNew(t_event))
		return false;
	
	if (s_first_event == nil)
		s_first_event = s_last_event = t_event;
	else
	{
		t_event -> next = s_first_event;
		s_first_event = t_event;
	}
	
	t_event -> type = p_type;
	
	// MW-2011-08-16: [[ Wait ]] Ping any running wait loop to make sure it
	//   picks up the new event if it wants to.
	MCscreen -> pingwait();
	
	r_event = t_event;
	
	return true;
}

//////////

bool MCEventQueuePostNotify(MCEventQueueNotifyCallback p_callback, void *p_state)
{
	MCEvent *t_event;
	if (!MCEventQueuePost(kMCEventTypeNotify, t_event))
		return false;

	t_event -> notify . callback = p_callback;
	t_event -> notify . state = p_state;

	return true;
}

//////////

// IM-2014-02-14: [[ HiDPI ]] Post backing scale changes with window reshape message
MC_DLLEXPORT_DEF
bool MCEventQueuePostWindowReshape(MCStack *p_stack, MCGFloat p_backing_scale)
{
	// We look through the current event queue, looking for the last window
	// reshape event for this stack and coalesce the event.
	MCEvent *t_event;
	t_event = nil;
	for(MCEvent *t_new_event = s_first_event; t_new_event != nil; t_new_event = t_new_event -> next)
    {
        if (t_new_event -> type == kMCEventTypeWindowReshape)
        {
            MCStackHandle t_stack = t_new_event->window.stack;
            if (t_stack.IsValid() && t_stack == p_stack)
            {
                t_event = t_new_event;
            }
        }
    }

	// If we found an event, remove it since we are about to replace it
	// with a more recent mouse position event.
	if (t_event != nil)
	{
		MCEventQueueRemoveEvent(t_event);
		MCEventQueueDestroyEvent(t_event);
	}

	if (!MCEventQueuePost(kMCEventTypeWindowReshape, t_event))
		return false;
	
	t_event -> window . stack = p_stack->GetHandle().ExternalRetain();
	t_event -> window . scale = p_backing_scale;
	
	return true;
}

//////////

static bool MCEventQueuePostMouse(MCEventType p_type, MCStack *p_stack, uint32_t p_time, MCEvent*& r_event)
{
	if (!MCEventQueuePost(p_type, r_event))
		return false;

	r_event -> mouse . stack = p_stack->GetHandle().ExternalRetain();
	r_event -> mouse . time = p_time;

	return true;
}

MC_DLLEXPORT_DEF
bool MCEventQueuePostMouseFocus(MCStack *p_stack, uint32_t p_time, bool p_inside)
{
	MCEvent *t_event;
	if (!MCEventQueuePostMouse(kMCEventTypeMouseFocus, p_stack, p_time, t_event))
		return false;
	
	t_event -> mouse . focus . inside = p_inside;

	//MCLog("MouseFocus(%p, %d, %d)", p_stack, p_time, p_inside);
	
	return true;
}

MC_DLLEXPORT_DEF
bool MCEventQueuePostMousePress(MCStack *p_stack, uint32_t p_time, uint32_t p_modifiers, MCMousePressState p_state, int32_t p_button)
{
	MCEvent *t_event;
	if (!MCEventQueuePostMouse(kMCEventTypeMousePress, p_stack, p_time, t_event))
		return false;

	t_event -> mouse . press . modifiers = p_modifiers;
	t_event -> mouse . press . state = p_state;
	t_event -> mouse . press . button = p_button;
	
	//MCLog("MousePress(%p, %d, %d, %d, %d)", p_stack, p_time, p_modifiers, p_state, p_button);
	
	return true;
}

bool MCEventQueuePostMouseWheel(MCStack *p_stack, uint32_t p_time, uint32_t p_modifiers, int32_t p_dh, int32_t p_dv)
{
	MCEvent *t_event;
	if (!MCEventQueuePostMouse(kMCEventTypeMouseWheel, p_stack, p_time, t_event))
		return false;

	t_event -> mouse . wheel . modifiers = p_modifiers;
	t_event -> mouse . wheel . dh = p_dh;
	t_event -> mouse . wheel . dv = p_dv;

	return true;
}

MC_DLLEXPORT_DEF
bool MCEventQueuePostMousePosition(MCStack *p_stack, uint32_t p_time, uint32_t p_modifiers, int32_t p_x, int32_t p_y)
{
	// We look through the current event queue, looking for the last mouse
	// position event for this stack. If we encounter a press, wheel or focus
	// event, we reset our search.
	MCEvent *t_event;
	t_event = nil;
	for(MCEvent *t_new_event = s_first_event; t_new_event != nil; t_new_event = t_new_event -> next)
		if (MCEventQueueEventIsMouse(t_new_event) &&
			MCStackHandle(t_new_event->mouse.stack) == p_stack)
		{
			if (t_new_event -> type == kMCEventTypeMousePosition)
				t_event = t_new_event;
			else
				t_event = nil;
		}
	
	// If we found an event, remove it since we are about to replace it
	// with a more recent mouse position event.
	if (t_event != nil)
	{
		MCEventQueueRemoveEvent(t_event);
		MCEventQueueDestroyEvent(t_event);
	}
		
	if (!MCEventQueuePostMouse(kMCEventTypeMousePosition, p_stack, p_time, t_event))
		return false;

	t_event -> mouse . position . modifiers = p_modifiers;
	t_event -> mouse . position . x = p_x;
	t_event -> mouse . position . y = p_y;
	
	//MCLog("MousePosition(%p, %d, %d, %d, %d)", p_stack, p_time, p_modifiers, p_x, p_y);
	
	return true;
}

MC_DLLEXPORT_DEF
bool MCEventQueuePostKeyFocus(MCStack *p_stack, bool p_owner)
{
	MCEvent *t_event;
	if (!MCEventQueuePost(kMCEventTypeKeyFocus, t_event))
		return false;

	t_event -> key . stack = p_stack->GetHandle().ExternalRetain();
	t_event -> key . focus . owner = p_owner;

	return true;
}

MC_DLLEXPORT_DEF
bool MCEventQueuePostKeyPress(MCStack *p_stack, uint32_t p_modifiers, uint32_t p_char_code, uint32_t p_key_code)
{
	MCEvent *t_event;
	if (!MCEventQueuePost(kMCEventTypeKeyPress, t_event))
		return false;

	t_event -> key . stack = p_stack->GetHandle().ExternalRetain();
	t_event -> key . press . modifiers = p_modifiers;
	t_event -> key . press . char_code = p_char_code;
	t_event -> key . press . key_code = p_key_code;

	return true;
}

MC_DLLEXPORT_DEF
bool MCEventQueuePostImeCompose(MCStack *p_stack, bool p_enabled, uint32_t p_offset, const uint16_t *p_chars, uint32_t p_char_count)
{
	uint16_t *t_new_chars;
	if (!MCMemoryNewArray(p_char_count, t_new_chars))
		return false;

	MCEvent *t_event;
	if (!MCEventQueuePost(kMCEventTypeImeCompose, t_event))
	{
		MCMemoryDeleteArray(t_new_chars);
		return false;
	}

	t_event -> ime . stack = p_stack->GetHandle().ExternalRetain();
	t_event -> ime . compose . enabled = p_enabled;
	t_event -> ime . compose . offset = p_offset;
	t_event -> ime . compose . chars = t_new_chars;
	t_event -> ime . compose . char_count = p_char_count;

	MCMemoryCopy(t_new_chars, p_chars, sizeof(uint16_t) * p_char_count);

	return true;
}

bool MCEventQueuePostQuitApp(void)
{
	MCEvent *t_event;
	return MCEventQueuePost(kMCEventTypeQuitApp, t_event);
}

bool MCEventQueuePostSuspendApp(void)
{
	MCEvent *t_event;
	return MCEventQueuePost(kMCEventTypeSuspendApp, t_event);
}

bool MCEventQueuePostResumeApp(void)
{
	MCEvent *t_event;
	return MCEventQueuePost(kMCEventTypeResumeApp, t_event);
}

bool MCEventQueuePostUpdateMenu(MCObjectHandle p_target)
{
	MCEvent *t_event;
	if (!MCEventQueuePost(kMCEventTypeUpdateMenu, t_event))
		return false;
    
	t_event -> menu . target = p_target.ExternalRetain();
    
	return true;
}

bool MCEventQueuePostMenuPick(MCObjectHandle p_target, MCStringRef p_string)
{
	MCEvent *t_event;
	if (!MCEventQueuePost(kMCEventTypeMenuPick, t_event))
		return false;

	t_event -> menu . target = p_target.ExternalRetain();
    
    // SN-2014-06-23: pick updated to StringRef
	return MCStringCopy(p_string, t_event -> menu . pick . string);
}

bool MCEventQueuePostCustom(MCCustomEvent *p_event)
{
	bool t_success;
	t_success = true;
	
	MCEvent *t_event;
	t_event = nil;
	if (t_success)
		t_success = MCEventQueuePost(kMCEventTypeCustom, t_event);
    
	if (t_success)
		t_event -> custom . event = p_event;
	
	return t_success;
}

bool MCEventQueuePostCustomAtFront(MCCustomEvent *p_event)
{
	bool t_success;
	t_success = true;
	
	MCEvent *t_event;
	t_event = nil;
	if (t_success)
		t_success = MCEventQueuePostAtFront(kMCEventTypeCustom, t_event);
	
	if (t_success)
		t_event -> custom . event = p_event;
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef _MOBILE

struct MCTouch
{
	MCTouch *next;
	uint32_t id;
	int32_t x, y;
	MCObjectHandle target;
};

static MCTouch *s_touches = nil;
static bool s_touches_for_widget = false;

static void handle_touch(MCStack *p_stack, MCEventTouchPhase p_phase, uint32_t p_id, uint32_t p_taps, int32_t x, int32_t y)
{
    /* Touch messages might flow to a widget, or to a control.
     *
     * If a widget handles the first touch in a sequence, then the sequence is
     * considered grabbed by the widget event manager and should receive all
     * subsequent touch events in the sequence.
     *
     * If a widget does not handle the first touch in a sequence, then the
     * sequence is considered not grabbed by the widget event manager and no
     * touch events should flow into the widget event manager. */

	MCTouch *t_previous_touch;
	t_previous_touch = nil;
	
	MCTouch *t_touch;
	t_touch = nil;
	
	MCObject *t_target;
	t_target = nil;
	
	// IM-2013-09-30: [[ FullscreenMode ]] Translate touch location to stack coords
	MCPoint t_touch_loc;
	t_touch_loc = p_stack->view_viewtostackloc(MCPointMake(x, y));

    /* If true, then the event should be passed to the widget event manager
     * first. If the touches are already grabbed by a widget then the event
     * must be passed to the widget event manager. */
    bool t_widget_first = s_touches_for_widget;

    if (p_phase != kMCEventTouchPhaseBegan)
    {
        for(t_touch = s_touches; t_touch != nil; t_previous_touch = t_touch, t_touch = t_touch -> next)
            if (t_touch -> id == p_id)
                break;
        
        /* If there is a touch, fetch the target and delete the touch record if it
         * is ending or cancelling a touch. */
        if (t_touch != nil)
        {
            t_target = t_touch -> target;
            
            // MW-2011-09-05: [[ Bug 9683 ]] Make sure we remove (and delete the touch) here if
            //   it is 'end' or 'cancelled' so that a cleartouches inside an invoked handler
            //   doesn't cause a crash.			
            if (p_phase == kMCEventTouchPhaseEnded || p_phase == kMCEventTouchPhaseCancelled)
            {
                if (t_previous_touch == nil)
                    s_touches = t_touch -> next;
                else
                    t_previous_touch -> next = t_touch -> next;
                
                delete t_touch;
            }
        }
    }
    else
    {
        t_touch = new (nothrow) MCTouch;
        t_touch -> next = s_touches;
        t_touch -> id = p_id;

        /* If the touch sequence isn't for widgets, then we must find a control
         * to send the event to. In this case, if this is the first touch in 
         * a sequence *and* the target is a widget, we must allow the widget
         * event manager to have a look at it. */
        if (!s_touches_for_widget)
        {
            t_target = p_stack -> getcurcard() -> hittest(t_touch_loc.x, t_touch_loc.y);
            if (t_target != nullptr)
            {
                if (t_touch->next == nullptr &&
                    t_target->gettype() == CT_WIDGET)
                {
                    t_widget_first = true;
                }
            
                t_touch -> target = t_target -> GetHandle();
            }
        }
        else
        {
            t_target = s_touches -> target;
            t_touch -> target = t_target -> GetHandle();
        }
        
        s_touches = t_touch;
    }
    
	if (t_target != nil)
	{
        /* If the touch must be passed to the widget event manager then do that
         * first. */
        if (t_widget_first)
        {
            /* If the widget handled the touch, or a widget has grabbed all
             * touches in a sequence (s_touches_for_widget == true), then
             * make sure all future touches are handled by the widget em and
             * don't fall through to the engine. */
            if (MCwidgeteventmanager->event_touch(MCObjectCast<MCWidget>(t_target),
                                                  p_id, p_phase, t_touch_loc.x, t_touch_loc.y) ||
                s_touches_for_widget)
            {
                /* If this was touch event caused the sequence to end, then
                 * make sure future touch events reconsider the target. */
                s_touches_for_widget = s_touches != nullptr;
                return;
            }
        }
        
        switch(p_phase)
        {
            case kMCEventTouchPhaseBegan:
                t_target -> message_with_args(MCM_touch_start, p_id);
                break;
            case kMCEventTouchPhaseMoved:
                t_target -> message_with_args(MCM_touch_move, p_id, t_touch_loc.x, t_touch_loc.y);
                break;
            case kMCEventTouchPhaseEnded:
                t_target -> message_with_args(MCM_touch_end, p_id);
                break;
            case kMCEventTouchPhaseCancelled:
                t_target -> message_with_args(MCM_touch_release, p_id);
                break;
        }
	}
}

// Clear all touch state.
static void clear_touches(void)
{
	while(s_touches != nil)
	{
		MCTouch *t_touch;
		t_touch = s_touches;
		s_touches = s_touches -> next;
		
		delete t_touch;
	}
}

bool MCEventQueuePostTouch(MCStack *p_stack, MCEventTouchPhase p_phase, uint32_t p_id, uint32_t p_taps, int32_t p_x, int32_t p_y)
{
	MCEvent *t_event = nil;
	
	// If there is an existing unhandled touch move event in the queue, this new
	// one can be coalesced with it.
	bool t_existing = false;
	if (p_phase == kMCEventTouchPhaseMoved)
	{
		for(MCEvent *t_new_event = s_first_event; t_new_event != nil; t_new_event = t_new_event->next)
		{
			if (t_new_event -> type == kMCEventTypeTouch &&
				MCStackHandle(t_new_event->touch.stack) == p_stack &&
				t_new_event->touch.id == p_id &&
				t_new_event->touch.phase == p_phase)
			{
				t_event = t_new_event;
				t_existing = true;
			}
		}
	}
	
	// If there was no existing event, create and add a new one
	if (t_event == nil &&
		!MCEventQueuePost(kMCEventTypeTouch, t_event))
		return false;
	
	// Store a reference to the stack if this is a newly-created event
	if (!t_existing)
		t_event->touch.stack = p_stack->GetHandle().ExternalRetain();
	
	// Set the touch event parameters
	t_event -> touch . phase = p_phase;
	t_event -> touch . id = p_id;
	t_event -> touch . taps = p_taps;
	t_event -> touch . x = p_x;
	t_event -> touch . y = p_y;
	
	return true;
}

bool MCEventQueuePostMotion(MCStack *p_stack, MCEventMotionType p_type, uint32_t p_timestamp)
{
	MCEvent *t_event;
	if (!MCEventQueuePost(kMCEventTypeMotion, t_event))
		return false;
	
	t_event -> motion . stack = p_stack->GetHandle().ExternalRetain();
	t_event -> motion . type = p_type;
	
	return true;	
}

bool MCEventQueuePostAccelerationChanged(double x, double y, double z, double t)
{
	MCEvent *t_event;
	t_event = nil;
	for(MCEvent *t_new_event = s_first_event; t_new_event != nil; t_new_event = t_new_event -> next)
		if (t_new_event -> type == kMCEventTypeAcceleration)
		{
			t_event = t_new_event;
			break;
		}
	
	if (t_event == nil &&
		!MCEventQueuePost(kMCEventTypeAcceleration, t_event))
		return false;
	
	t_event -> acceleration . x = x;
	t_event -> acceleration . y = y;
	t_event -> acceleration . z = z;
	t_event -> acceleration . t = t;
	
	return true;	
}

bool MCEventQueuePostOrientationChanged(void)
{
	MCEvent *t_event;
	t_event = nil;
	for(MCEvent *t_new_event = s_first_event; t_new_event != nil; t_new_event = t_new_event -> next)
		if (t_new_event -> type == kMCEventTypeOrientation)
		{
			t_event = t_new_event;
			break;
		}
	
	if (t_event == nil &&
		!MCEventQueuePost(kMCEventTypeOrientation, t_event))
		return false;
	
	return true;
}

bool MCEventQueuePostLocationChanged(void)
{
	MCEvent *t_event;
	t_event = nil;
	for(MCEvent *t_new_event = s_first_event; t_new_event != nil; t_new_event = t_new_event -> next)
		if (t_new_event -> type == kMCEventTypeLocation)
		{
			t_event = t_new_event;
			break;
		}
	
	if (t_event == nil &&
		!MCEventQueuePost(kMCEventTypeLocation, t_event))
		return false;
	
	t_event -> location . error = nil;
	
	return true;
}

bool MCEventQueuePostLocationError(void)
{
	MCEvent *t_event;
	t_event = nil;
	for(MCEvent *t_new_event = s_first_event; t_new_event != nil; t_new_event = t_new_event -> next)
		if (t_new_event -> type == kMCEventTypeLocation)
		{
			t_event = t_new_event;
			break;
		}
	
	if (t_event == nil &&
		!MCEventQueuePost(kMCEventTypeLocation, t_event))
		return false;
	
	t_event -> location . error = "";
	
	return true;
}

bool MCEventQueuePostHeadingChanged(void)
{
	MCEvent *t_event;
	t_event = nil;
	for(MCEvent *t_new_event = s_first_event; t_new_event != nil; t_new_event = t_new_event -> next)
		if (t_new_event -> type == kMCEventTypeHeading)
		{
			t_event = t_new_event;
			break;
		}
	
	if (t_event == nil &&
		!MCEventQueuePost(kMCEventTypeHeading, t_event))
		return false;
	
	t_event -> location . error = nil;
	
	return true;
}

bool MCEventQueuePostHeadingError(void)
{
	MCEvent *t_event;
	t_event = nil;
	for(MCEvent *t_new_event = s_first_event; t_new_event != nil; t_new_event = t_new_event -> next)
		if (t_new_event -> type == kMCEventTypeHeading)
		{
			t_event = t_new_event;
			break;
		}
	
	if (t_event == nil &&
		!MCEventQueuePost(kMCEventTypeHeading, t_event))
		return false;
	
	t_event -> location . error = "";
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool clear_touches_filter(void *p_context, MCEvent *p_event)
{
	// MW-2011-03-30: [[ Bug ]] Don't clear the 'MouseFocus' messages as this
	//   stops later mouse downs from working!
	if (p_event -> type == kMCEventTypeMousePosition ||
		p_event -> type == kMCEventTypeMousePress ||
		p_event -> type == kMCEventTypeMouseWheel ||
		p_event -> type == kMCEventTypeTouch)
		return false;
	
	return true;
}

void MCEventQueueClearTouches(void)
{
	// Clear the static state we have tracking touches.
	clear_touches();
	
	// And now clear anything touch-related in the message queue
	MCEventQueueFilter(clear_touches_filter, nil);
}

#endif
