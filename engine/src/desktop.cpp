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

#include "platform.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "objdefs.h"
#include "unicode.h"

#include "execpt.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"
#include "stack.h"
#include "card.h"
#include "debug.h"
#include "dispatch.h"
#include "control.h"
#include "field.h"
#include "graphics_util.h"
#include "redraw.h"
#include "player.h"
#include "aclip.h"
#include "stacklst.h"

#include "desktop-dc.h"

////////////////////////////////////////////////////////////////////////////////

bool X_init(int argc, char *argv[], char *envp[]);
void X_main_loop_iteration();
int X_close();

void X_main_loop(void)
{
	while(!MCquit)
		X_main_loop_iteration();
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformHandleApplicationStartup(int p_argc, char **p_argv, char **p_envp, int& r_error_code, char*& r_error_message)
{
	if (X_init(p_argc, p_argv, p_envp))
	{
		r_error_code = 0;
		r_error_message = nil;
		return;
	}
	
	r_error_code = -1;
	r_error_message = MCresult -> getvalue() . get_string() . clone();
}

void MCPlatformHandleApplicationShutdown(int& r_exit_code)
{
	r_exit_code = X_close();
}

void MCPlatformHandleApplicationShutdownRequest(bool& r_terminate)
{
	switch(MCdefaultstackptr->getcard()->message(MCM_shut_down_request))
	{
		case ES_PASS:
		case ES_NOT_HANDLED:
			MCdefaultstackptr->getcard()->message(MCM_shut_down);
			MCquit = True;
			MCexitall = True;
			MCtracestackptr = NULL;
			MCtraceabort = True;
			MCtracereturn = True;
			r_terminate = true;
			break;
		default:
			r_terminate = false;
			break;
	}
}

void MCPlatformHandleApplicationSuspend(void)
{
	MCdefaultstackptr -> getcard() -> message(MCM_suspend);
	MCappisactive = False;
}

void MCPlatformHandleApplicationResume(void)
{
	MCappisactive = True;
	MCdefaultstackptr -> getcard() -> message(MCM_resume);
}

void MCPlatformHandleApplicationRun(void)
{
	X_main_loop();
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformHandleScreenParametersChanged(void)
{
	// IM-2014-01-28: [[ HiDPI ]] Use updatedisplayinfo() method to update & compare display details
	bool t_changed;
	t_changed = false;
	MCscreen->updatedisplayinfo(t_changed);
	
	// Post a desktop changed message.
	MCscreen -> delaymessage(MCdefaultstackptr -> getcurcard(), MCM_desktop_changed);
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformHandleWindowCloseRequest(MCPlatformWindowRef p_window)
{
	MCdispatcher -> wclose(p_window);
}

void MCPlatformHandleWindowClose(MCPlatformWindowRef p_window)
{
	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(p_window);
	if (t_stack == nil)
		return;
	
	t_stack -> kunfocus();
	t_stack -> close();
	t_stack -> checkdestroy();
}

void MCPlatformHandleWindowReshape(MCPlatformWindowRef p_window)
{
	MCdispatcher -> wreshape(p_window);
}

void MCPlatformHandleWindowConstrain(MCPlatformWindowRef p_window, MCPoint p_proposed_size, MCPoint& r_wanted_size)
{
	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(p_window);
	if (t_stack == nil)
	{
		r_wanted_size = p_proposed_size;
		return;
	}
	
	t_stack -> constrain(p_proposed_size, r_wanted_size);
}

void MCPlatformHandleWindowRedraw(MCPlatformWindowRef p_window, MCPlatformSurfaceRef p_surface, MCRegionRef p_region)
{
	if (((MCScreenDC *)MCscreen) -> isbackdrop(p_window))
	{
		((MCScreenDC *)MCscreen) -> redrawbackdrop(p_surface, p_region);
		return;
	}
		
	MCdispatcher -> wredraw(p_window, p_surface, p_region);
}

void MCPlatformHandleWindowIconify(MCPlatformWindowRef p_window)
{
	MCdispatcher -> wiconify(p_window);
}

void MCPlatformHandleWindowUniconify(MCPlatformWindowRef p_window)
{
	MCdispatcher -> wuniconify(p_window);
}

void MCPlatformHandleWindowFocus(MCPlatformWindowRef p_window)
{
	MCdispatcher -> wkfocus(p_window);
}

void MCPlatformHandleWindowUnfocus(MCPlatformWindowRef p_window)
{
	MCdispatcher -> wkunfocus(p_window);
}

void MCPlatformHandleViewFocusSwitched(MCPlatformWindowRef p_window, uint32_t p_view_id)
{
	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(p_window);
	if (t_stack == nil)
		return;
	
	if (p_view_id == 0)
		t_stack -> getcard() -> kfocus();
	else
	{
		MCControl *t_control;
		char t_id[U4L];
		sprintf(t_id, "%d", p_view_id);
		t_control = t_stack -> getcard() -> getchild(CT_ID, t_id, CT_LAYER, CT_UNDEFINED);
		if (t_control != nil)
			t_stack -> kfocusset(t_control);
		else
			t_stack -> getcard() -> kunfocus();
	}
}

////////////////////////////////////////////////////////////////////////////////

extern Boolean tripleclick;

void MCPlatformHandleMouseEnter(MCPlatformWindowRef p_window)
{
	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(p_window);
	if (t_stack == nil)
		return;
	
	if (t_stack != MCmousestackptr)
	{
		MCmousestackptr = t_stack;
		MCmousestackptr -> enter();
	}
	
	MCObject *t_menu;
	t_menu = MCdispatcher -> getmenu();
	if (t_menu == nil)
		MCmousestackptr -> mfocus(MCmousex, MCmousey);
	else
		t_menu -> mfocus(MCmousex, MCmousey);
}

void MCPlatformHandleMouseLeave(MCPlatformWindowRef p_window)
{
	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(p_window);
	if (t_stack == nil)
		return;
	
	if (t_stack == MCmousestackptr)
	{
		MCmousestackptr -> munfocus();
		MCmousestackptr = nil;
	}
}

void MCPlatformHandleMouseMove(MCPlatformWindowRef p_window, MCPoint p_location)
{
	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(p_window);
	if (t_stack == nil)
		return;
	
	MCObject *t_menu;
	t_menu = MCdispatcher -> getmenu();
	
	if (MCmousestackptr == t_stack || t_menu != nil)
	{
		MCeventtime = MCPlatformGetEventTime();
		
		MCObject *t_target;
		t_target = t_menu != nil ? t_menu : MCmousestackptr;
		
		// IM-2014-02-12: [[ StackScale ]] mfocus will translate target stack to menu stack coords
		//   so in both cases we pass target stack coords.
		// IM-2014-02-14: [[ StackScale ]] Don't try to convert if target is null
		MCPoint t_mouseloc;
		if (t_stack != nil)
			t_mouseloc = t_stack->windowtostackloc(p_location);
		
		MCmousex = t_mouseloc.x;
		MCmousey = t_mouseloc.y;
		
		MCLog("MouseMove(%p, %d, %d)", t_target, t_mouseloc . x, t_mouseloc . y);
		
		t_target -> mfocus(t_mouseloc . x, t_mouseloc . y);		
	}
}

void MCPlatformHandleMouseDown(MCPlatformWindowRef p_window, uint32_t p_button, uint32_t p_count)
{
	if (((MCScreenDC *)MCscreen) -> isbackdrop(p_window))
	{
		((MCScreenDC *)MCscreen) -> mousedowninbackdrop(p_button, p_count);
		return;
	}
	
	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(p_window);
	if (t_stack == nil)
		return;
	
	MCObject *t_menu;
	t_menu = MCdispatcher -> getmenu();
	
	if (MCmousestackptr == t_stack || t_menu != nil)
	{
		MCbuttonstate |= (1 << p_button);
		
		MCeventtime = MCPlatformGetEventTime();
		MCclicklocx = MCmousex;
		MCclicklocy = MCmousey;
		MCclickstackptr = MCmousestackptr;
		
		MCObject *t_target;
		t_target = t_menu != nil ? t_menu : MCclickstackptr;
		
		tripleclick = p_count == 2;
		
		MCLog("MouseDown(%p, %d, %d)", t_target, p_button, p_count);
		
		if (p_count != 1)
		{
			if (p_count == 0 && !MCdispatcher -> isdragsource())
			{
				MCallowabledragactions = DRAG_ACTION_COPY;
				MCdragaction = DRAG_ACTION_NONE;
				MCdragimageid = 0;
				MCdragimageoffset . x = 0;
				MCdragimageoffset . y = 0;
				MCdragdata -> ResetSource();
			}
			
			t_target -> mdown(p_button + 1);
		}
		else
			t_target -> doubledown(p_button + 1);
	}
}

void MCPlatformHandleMouseUp(MCPlatformWindowRef p_window, uint32_t p_button, uint32_t p_count)
{
	if (((MCScreenDC *)MCscreen) -> isbackdrop(p_window))
	{
		((MCScreenDC *)MCscreen) -> mouseupinbackdrop(p_button, p_count);
		return;
	}
	
	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(p_window);
	if (t_stack == nil)
		return;
	
	MCObject *t_menu;
	t_menu = MCdispatcher -> getmenu();
	
	if (MCmousestackptr == t_stack || t_menu != nil)
	{
		MCbuttonstate &= ~(1 << p_button);
		
		MCeventtime = MCPlatformGetEventTime();
		
		MCObject *t_target;
		t_target = t_menu != nil ? t_menu : MCclickstackptr;
		
		MCLog("MouseUp(%p, %d, %d)", t_target, p_button, p_count);
		
		if (p_count != 1)
			t_target -> mup(p_button + 1);
		else
			t_target -> doubleup(p_button + 1);
	}
}

void MCPlatformHandleMouseDrag(MCPlatformWindowRef p_window, uint32_t p_button)
{
	MCdispatcher -> wmdrag(p_window);
}

void MCPlatformHandleMouseRelease(MCPlatformWindowRef p_window, uint32_t p_button)
{
	if (((MCScreenDC *)MCscreen) -> isbackdrop(p_window))
	{
		((MCScreenDC *)MCscreen) -> mousereleaseinbackdrop(p_button);
		return;
	}
	
	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(p_window);
	if (t_stack == nil)
		return;
	
	MCObject *t_menu;
	t_menu = MCdispatcher -> getmenu();
	
	if (MCmousestackptr == t_stack || t_menu != nil)
	{
		tripleclick = False;
		
		MCbuttonstate &= ~(1 << p_button);
		
		// If the press was 'released' i.e. cancelled then we stop messages, mup then
		// dispatch a mouseRelease message ourselves.
		
		// FG-2013-10-09 [[ Bugfix 11208 ]]
		// CS_NO_MESSAGES only applies to the target and not the controls it contains
		// so the mouse up message (on mouseUp) sets sent when it isn't desired
		// Hopefully nobody depends on the old behaviour...
		
		MCObject *t_target;
		t_target = t_menu != nil ? t_menu : MCclickstackptr;
		
		bool old_lock = MClockmessages;
		MClockmessages = true;
		t_target -> mup(p_button + 1);
		MClockmessages = old_lock;
		
		MCLog("MouseRelease(%p, %d)", t_target, p_button);
		
		t_target -> message_with_args(MCM_mouse_release, p_button + 1);
	}
}

void MCPlatformHandleMouseScroll(MCPlatformWindowRef p_window, int p_dx, int p_dy)
{
	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(p_window);
	if (t_stack == nil)
		return;
	
	if (MCmousestackptr != t_stack)
		return;
	
	MCObject *mfocused;
	
	mfocused = MCmousestackptr->getcard()->getmfocused();
	if (mfocused == NULL)
		mfocused = MCmousestackptr -> getcard();
	if (mfocused == NULL)
		mfocused = MCmousestackptr;
	
	if (p_dy != 0)
		mfocused -> kdown("", p_dy < 0 ? XK_WheelUp : XK_WheelDown);
	
	mfocused = MCmousestackptr->getcard()->getmfocused();
	if (mfocused == NULL)
		mfocused = MCmousestackptr -> getcard();
	if (mfocused == NULL)
		mfocused = MCmousestackptr;
	
	if (p_dx != 0)
		mfocused -> kdown("", p_dx < 0 ? XK_WheelLeft : XK_WheelRight);
}

//////////

static MCPlatformDragOperation dragaction_to_dragoperation(MCDragAction p_action)
{
	switch(p_action)
	{
		case DRAG_ACTION_NONE:
			return kMCPlatformDragOperationNone;
		case DRAG_ACTION_MOVE:
			return kMCPlatformDragOperationMove;
		case DRAG_ACTION_COPY:
			return kMCPlatformDragOperationCopy;
		case DRAG_ACTION_LINK:
			return kMCPlatformDragOperationLink;
	}
	
	assert(false);
	
	return kMCPlatformDragOperationNone;
}

void MCPlatformHandleDragEnter(MCPlatformWindowRef p_window, MCPlatformPasteboardRef p_pasteboard, MCPlatformDragOperation& r_operation)
{
	MCSystemPasteboard *t_pasteboard;
	t_pasteboard = new MCSystemPasteboard(p_pasteboard);
	MCdispatcher -> wmdragenter(p_window, t_pasteboard);
	t_pasteboard -> Release();
	
	r_operation = dragaction_to_dragoperation(MCdragaction);
}

void MCPlatformHandleDragMove(MCPlatformWindowRef p_window, MCPoint p_location, MCPlatformDragOperation& r_operation)
{
	MCdispatcher -> wmdragmove(p_window, p_location . x, p_location . y);
	
	r_operation = dragaction_to_dragoperation(MCdragaction);
}

void MCPlatformHandleDragLeave(MCPlatformWindowRef p_window)
{
	MCdispatcher -> wmdragleave(p_window);
}

void MCPlatformHandleDragDrop(MCPlatformWindowRef p_window, bool& r_accepted)
{
	MCdispatcher -> wmdragdrop(p_window);
	
	// PLATFORM-TODO: Should we do more than this? i.e. Should the dragDrop
	//   message be able to signal refusal?
	r_accepted = true;
}

////////////////////////////////////////////////////////////////////////////////

static MCPlatformKeyCode s_last_key_code = 0;

void MCPlatformHandleModifiersChanged(MCPlatformModifiers p_modifiers)
{
	MCmodifierstate = 0;
	if ((p_modifiers & kMCPlatformModifierShift) != 0)
		MCmodifierstate |= MS_SHIFT;
	if ((p_modifiers & kMCPlatformModifierControl) != 0)
		MCmodifierstate |= MS_CONTROL;
	if ((p_modifiers & kMCPlatformModifierAlt) != 0)
		MCmodifierstate |= MS_MOD1;
	if ((p_modifiers & kMCPlatformModifierMeta) != 0)
		MCmodifierstate |= MS_MOD2;
	if ((p_modifiers & kMCPlatformModifierCapsLock) != 0)
		MCmodifierstate |= MS_CAPS_LOCK;
}

// MW-2014-04-15: [[ Bug 12086 ]] This method is invoked to give us the last key
//   code that was passed to an IME session. This allows us to correctly synthesize
//   a keydown / keyup pair if a single character is produced.
void MCPlatformHandleRawKeyDown(MCPlatformWindowRef p_window, MCPlatformKeyCode p_key_code)
{
    s_last_key_code = p_key_code;
}

void MCPlatformHandleKeyDown(MCPlatformWindowRef p_window, MCPlatformKeyCode p_key_code, codepoint_t p_mapped_codepoint, codepoint_t p_unmapped_codepoint)
{	
	if (p_mapped_codepoint == 0xffffffffU)
	{
		MCdispatcher -> wkdown(p_window, MCnullstring, p_key_code);
		return;
	}
	
	if (p_mapped_codepoint <= 0xffff)
	{
		uint16_t t_unicode_char;
		t_unicode_char = p_mapped_codepoint & 0xffff;
		
		uint8_t t_native_char[2];
		if (MCUnicodeMapToNative(&t_unicode_char, 1, t_native_char[0]))
		{
			t_native_char[1] = '\0';
			MCdispatcher -> wkdown(p_window, (const char *)t_native_char, p_key_code);
			return;
		}
	}
	
	if (!MCdispatcher -> wkdown(p_window, MCnullstring, p_key_code))
		if (MCactivefield != nil)
		{
			// Handle unicode
		}
}

void MCPlatformHandleKeyUp(MCPlatformWindowRef p_window, MCPlatformKeyCode p_key_code, codepoint_t p_mapped_codepoint, codepoint_t p_unmapped_codepoint)
{	
	if (p_mapped_codepoint == 0xffffffffU)
	{
		MCdispatcher -> wkup(p_window, MCnullstring, p_key_code);
		return;
	}
	
	if (p_mapped_codepoint <= 0xffff)
	{
		uint16_t t_unicode_char;
		t_unicode_char = p_mapped_codepoint & 0xffff;
		
		uint8_t t_native_char[2];
		if (MCUnicodeMapToNative(&t_unicode_char, 1, t_native_char[0]))
		{
			t_native_char[1] = '\0';
			MCdispatcher -> wkup(p_window, (const char *)t_native_char, p_key_code);
			return;
		}
	}
	
	MCdispatcher -> wkup(p_window, MCnullstring, p_key_code);
}

void MCPlatformHandleTextInputQueryTextRanges(MCPlatformWindowRef p_window, MCRange& r_marked_range, MCRange& r_selected_range)
{
	if (MCactivefield == nil)
	{
		r_marked_range = MCRangeMake(UINDEX_MAX, 0);
		r_selected_range = MCRangeMake(UINDEX_MAX, 0);
		return;
	}
	
	int4 si, ei;
	MCactivefield -> selectedmark(False, si, ei, False, False);
	MCactivefield -> unresolvechars(0, si, ei);
	r_selected_range = MCRangeMake(si, ei - si);
	if (MCactivefield -> getcompositionrange(si, ei))
	{
		MCactivefield -> unresolvechars(0, si, ei);
		r_marked_range = MCRangeMake(si, ei - si);
	}
	else
		r_marked_range = MCRangeMake(UINDEX_MAX, 0);
}

void MCPlatformHandleTextInputQueryTextIndex(MCPlatformWindowRef p_window, MCPoint p_location, uindex_t& r_index)
{
	if (MCactivefield == nil)
	{
		r_index = 0;
		return;
	}
	
	MCPoint t_location;
	t_location = MCactivefield -> getstack() -> windowtostackloc(p_location);
	
	int32_t si, ei;
	if (MCactivefield -> locmarkpoint(p_location, False, False, False, False, si, ei))
		MCactivefield -> unresolvechars(0, si, ei);
	else
		si = 0;
	
	r_index = si;
}

void MCPlatformHandleTextInputQueryTextRect(MCPlatformWindowRef p_window, MCRange p_range, MCRectangle& r_first_line_rect, MCRange& r_actual_range)
{
	if (MCactivefield == nil)
	{
		r_first_line_rect = MCRectangleMake(0, 0, 0, 0);
		r_actual_range = MCRangeMake(UINDEX_MAX, 0);
		return;
	}
	
	int32_t t_si, t_ei;
	t_si = 0;
	t_ei = INT32_MAX;
	MCactivefield -> resolvechars(0, t_si, t_ei, p_range . offset, p_range . length);
	
	MCRectangle t_rect;
	t_rect = MCactivefield -> firstRectForCharacterRange(t_si, t_ei);
	
	MCactivefield -> unresolvechars(0, t_si, t_ei);
	
	MCPoint t_top_left, t_bottom_right;
	t_top_left = MCactivefield -> getstack() -> stacktowindowloc(MCPointMake(t_rect . x, t_rect . y));
	t_bottom_right = MCactivefield -> getstack() -> stacktowindowloc(MCPointMake(t_rect . x + t_rect . width, t_rect . y + t_rect . height));
	
	r_first_line_rect = MCRectangleMake(t_top_left . x, t_top_left . y, t_bottom_right . x - t_top_left . x, t_bottom_right . y - t_top_left . y);
	r_actual_range = MCRangeMake(t_si, t_ei - t_si);
}

void MCPlatformHandleTextInputQueryText(MCPlatformWindowRef p_window, MCRange p_range, unichar_t*& r_chars, uindex_t& r_char_count, MCRange& r_actual_range)
{
	// COCOA-TODO: Implement QueryText
	r_chars = nil;
	r_char_count = 0;
	r_actual_range = p_range;
}

void MCPlatformHandleTextInputInsertText(MCPlatformWindowRef p_window, unichar_t *p_chars, uindex_t p_char_count, MCRange p_replace_range, MCRange p_selection_range, bool p_mark)
{
	if (MCactivefield == nil)
		return;
	
	MCRedrawLockScreen();
	
	int32_t t_r_si, t_r_ei;
	t_r_si = 0;
	t_r_ei = INT32_MAX;
	MCactivefield -> resolvechars(0, t_r_si, t_r_ei, p_replace_range . offset, p_replace_range . length);
	
	if (!p_mark)
	{
		int4 si, ei;
		if (MCactivefield -> getcompositionrange(si, ei))
		{
			if (si < t_r_si)
				t_r_si -= MCMin(t_r_si - si, ei - si);
			if (si < t_r_ei)
				t_r_ei -= MCMin(t_r_ei - si, ei - si);
			
			MCactivefield -> stopcomposition(True, False);
		}
		
		// If the char count is 1 and the replacement range matches the current selection,
		// the char is native and the requested selection is after the char, then synthesis a
		// keydown/up pair.
		uint8_t t_char[2];
		if (p_char_count == 1 &&
			MCUnicodeMapToNative(p_chars, 1, t_char[0]) &&
			p_selection_range . offset == p_replace_range . offset + 1 &&
			p_selection_range . length == 0)
		{
			int32_t t_s_si, t_s_ei;
			MCactivefield -> selectedmark(False, t_s_si, t_s_ei, False, False);
			if (t_s_si == t_r_si &&
				t_s_ei == t_r_ei)
			{
				t_char[1] = '\0';
				
                // MW-2014-04-15: [[ Bug 12086 ]] Pass the keycode from the last event that was
                //   passed to the IME.
                MCdispatcher -> wkdown(p_window, (const char *)t_char, s_last_key_code);
				MCdispatcher -> wkup(p_window, (const char *)t_char, s_last_key_code);
				return;
			}
		}
	}
	else
	{
		if (p_char_count == 0)
			MCactivefield -> stopcomposition(True, False);
		else
		{
			int4 si, ei;
			if (MCactivefield -> getcompositionrange(si, ei))
			{
				if (si < t_r_si)
					t_r_si -= MCMin(t_r_si - si, ei - si);
				if (si < t_r_ei)
					t_r_ei -= MCMin(t_r_ei - si, ei - si);
				
				MCactivefield -> stopcomposition(True, False);
			}
		}
	}
	
	// Set the text.
	MCactivefield -> seltext(t_r_si, t_r_ei, False);
	
	if (p_mark)
		MCactivefield -> startcomposition();
	
	MCactivefield -> finsertnew(FT_IMEINSERT, MCString((char *)p_chars, p_char_count * 2), True, true);
	
	// And update the selection range.
	int32_t t_s_si, t_s_ei;
	t_s_si = 0;
	t_s_ei = INT32_MAX;
	MCactivefield -> resolvechars(0, t_s_si, t_s_ei, p_selection_range . offset, p_selection_range . length);
	MCactivefield -> setcompositioncursoroffset(t_s_si - t_r_si);
	MCactivefield -> seltext(t_s_si, t_s_ei, True);
	
	MCRedrawUnlockScreen();
}

static void synthesize_key_press(MCPlatformWindowRef p_window, char p_char, KeySym p_sym)
{
	char t_string[2];
	t_string[0] = p_char;
	t_string[1] = '\0';
	MCdispatcher -> wkdown(p_window, t_string, p_sym);
	MCdispatcher -> wkup(p_window, t_string, p_sym);
}

static void synthesize_move_with_shift(MCField *p_field, Field_translations p_action)
{
	uint2 t_modifier_state;
	t_modifier_state = MCmodifierstate;
	MCmodifierstate = MS_SHIFT;
	p_field -> fmove(p_action, nil, 0);
	MCmodifierstate = t_modifier_state;
}

// This is probably never called now as we catch keyDown's for IME input
// and dispatch them rather than go through actions.
void MCPlatformHandleTextInputAction(MCPlatformWindowRef p_window, MCPlatformTextInputAction p_action)
{
	if (MCactivefield == nil)
		return;
	
	switch(p_action)
	{
		case kMCPlatformTextInputActionCapitalizeWord:
			break;
		case kMCPlatformTextInputActionChangeCaseOfLetter:
			break;
		case kMCPlatformTextInputActionDeleteBackward:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fdel(FT_DELBCHAR, nil, 0);
			break;
		case kMCPlatformTextInputActionDeleteBackwardByDecomposingPreviousCharacter:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fdel(FT_DELBSUBCHAR, nil, 0);
			break;
		case kMCPlatformTextInputActionDeleteForward:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fdel(FT_DELFCHAR, nil, 0);
			break;
		case kMCPlatformTextInputActionDeleteToBeginningOfLine:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fdel(FT_DELBOL, nil, 0);
			break;
		case kMCPlatformTextInputActionDeleteToBeginningOfParagraph:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fdel(FT_DELBOP, nil, 0);
			break;
		case kMCPlatformTextInputActionDeleteToEndOfLine:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fdel(FT_DELEOL, nil, 0);
			break;
		case kMCPlatformTextInputActionDeleteToEndOfParagraph:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fdel(FT_DELEOP, nil, 0);
			break;
		case kMCPlatformTextInputActionDeleteWordBackward:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fdel(FT_DELBWORD, nil, 0);
			break;
		case kMCPlatformTextInputActionDeleteWordForward:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fdel(FT_DELFWORD, nil, 0);
			break;
		case kMCPlatformTextInputActionInsertBacktab:
			break;
		case kMCPlatformTextInputActionInsertContainerBreak:
			break;
		case kMCPlatformTextInputActionInsertLineBreak:
			synthesize_key_press(p_window, 11, 0);
			break;
		case kMCPlatformTextInputActionInsertNewline:
			synthesize_key_press(p_window, '\n', XK_Return);
			break;
		case kMCPlatformTextInputActionInsertParagraphSeparator:
			synthesize_key_press(p_window, '\n', XK_Return);
			break;
		case kMCPlatformTextInputActionInsertTab:
			synthesize_key_press(p_window, '\t', XK_Tab);
			break;
		case kMCPlatformTextInputActionLowercaseWord:
			break;
		case kMCPlatformTextInputActionMoveBackward:
			MCactivefield -> fmove(FT_BACKCHAR, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveBackwardAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_BACKCHAR);
			break;
		case kMCPlatformTextInputActionMoveParagraphForwardAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_FORWARDPARA);
			break;
		case kMCPlatformTextInputActionMoveParagraphBackwardAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_BACKPARA);
			break;
		case kMCPlatformTextInputActionMoveToBeginningOfDocumentAndModfySelection:
			synthesize_move_with_shift(MCactivefield, FT_BOF);
			break;
		case kMCPlatformTextInputActionMoveToEndOfDocumentAndModfySelection:
			synthesize_move_with_shift(MCactivefield, FT_EOF);
			break;
		case kMCPlatformTextInputActionMoveToBeginningOfLineAndModfySelection:
			synthesize_move_with_shift(MCactivefield, FT_BOL);
			break;
		case kMCPlatformTextInputActionMoveToEndOfLineAndModfySelection:
			synthesize_move_with_shift(MCactivefield, FT_EOL);
			break;
		case kMCPlatformTextInputActionMoveToBeginningOfParagraphAndModfySelection:
			synthesize_move_with_shift(MCactivefield, FT_BOP);
			break;
		case kMCPlatformTextInputActionMoveToEndOfParagraphAndModfySelection:
			synthesize_move_with_shift(MCactivefield, FT_EOP);
			break;
		case kMCPlatformTextInputActionMoveToLeftEndOfLine:
			MCactivefield -> fmove(FT_BOL, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveToLeftEndOfLineAndModfySelection:
			synthesize_move_with_shift(MCactivefield, FT_BOL);
			break;
		case kMCPlatformTextInputActionMoveToRightEndOfLine:
			MCactivefield -> fmove(FT_EOL, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveToRightEndOfLineAndModfySelection:
			synthesize_move_with_shift(MCactivefield, FT_EOL);
			break;
		case kMCPlatformTextInputActionMoveDown:
			MCactivefield -> fmove(FT_DOWN, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveDownAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_DOWN);
			break;
		case kMCPlatformTextInputActionMoveForward:
			MCactivefield -> fmove(FT_FORWARDCHAR, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveForwardAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_FORWARDCHAR);
			break;
		case kMCPlatformTextInputActionMoveLeft:
			MCactivefield -> fmove(FT_LEFTCHAR, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveLeftAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_LEFTCHAR);
			break;
		case kMCPlatformTextInputActionMoveRight:
			MCactivefield -> fmove(FT_RIGHTCHAR, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveRightAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_RIGHTCHAR);
			break;
		case kMCPlatformTextInputActionMoveToBeginningOfDocument:
			MCactivefield -> fmove(FT_BOF, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveToBeginningOfLine:
			MCactivefield -> fmove(FT_BOL, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveToBeginningOfParagraph:
			MCactivefield -> fmove(FT_BOP, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveToEndOfDocument:
			MCactivefield -> fmove(FT_EOF, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveToEndOfLine:
			MCactivefield -> fmove(FT_EOL, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveToEndOfParagraph:
			MCactivefield -> fmove(FT_EOF, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveUp:
			MCactivefield -> fmove(FT_UP, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveUpAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_UP);
			break;
		case kMCPlatformTextInputActionMoveWordBackward:
			MCactivefield -> fmove(FT_BACKWORD, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveWordBackwardAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_BACKWORD);
			break;
		case kMCPlatformTextInputActionMoveWordForward:
			MCactivefield -> fmove(FT_FORWARDWORD, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveWordForwardAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_FORWARDWORD);
			break;
		case kMCPlatformTextInputActionMoveWordLeft:
			MCactivefield -> fmove(FT_LEFTWORD, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveWordLeftAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_LEFTWORD);
			break;
		case kMCPlatformTextInputActionMoveWordRight:
			MCactivefield -> fmove(FT_RIGHTWORD, nil, 0);
			break;
		case kMCPlatformTextInputActionMoveWordRightAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_RIGHTWORD);
			break;
		case kMCPlatformTextInputActionPageUp:
			MCactivefield -> fmove(FT_PAGEUP, nil, 0);
			break;
		case kMCPlatformTextInputActionPageUpAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_PAGEUP);
			break;
		case kMCPlatformTextInputActionPageDown:
			MCactivefield -> fmove(FT_PAGEDOWN, nil, 0);
			break;
		case kMCPlatformTextInputActionPageDownAndModifySelection:
			synthesize_move_with_shift(MCactivefield, FT_PAGEDOWN);
			break;
		case kMCPlatformTextInputActionScrollToBeginningOfDocument:
			MCactivefield -> fscroll(FT_SCROLLTOP, nil, 0);
			break;
		case kMCPlatformTextInputActionScrollToEndOfDocument:
			MCactivefield -> fscroll(FT_SCROLLBOTTOM, nil, 0);
			break;
		case kMCPlatformTextInputActionScrollLineUp:
			MCactivefield -> fscroll(FT_SCROLLUP, nil, 0);
			break;
		case kMCPlatformTextInputActionScrollLineDown:
			MCactivefield -> fscroll(FT_SCROLLDOWN, nil, 0);
			break;
		case kMCPlatformTextInputActionScrollPageUp:
			MCactivefield -> fscroll(FT_SCROLLPAGEUP, nil, 0);
			break;
		case kMCPlatformTextInputActionScrollPageDown:
			MCactivefield -> fscroll(FT_SCROLLPAGEDOWN, nil, 0);
			break;
		case kMCPlatformTextInputActionSelectAll:
			break;
		case kMCPlatformTextInputActionSelectLine:
			break;
		case kMCPlatformTextInputActionSelectParagraph:
			break;
		case kMCPlatformTextInputActionSelectWord:
			break;
		case kMCPlatformTextInputActionTranspose:
			break;
		case kMCPlatformTextInputActionTransposeWords:
			break;
		case kMCPlatformTextInputActionUppercaseWord:
			break;
		case kMCPlatformTextInputActionYank:
			break;
		case kMCPlatformTextInputActionCut:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fcut(FT_CUT, nil, 0);
			break;
		case kMCPlatformTextInputActionCopy:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fcopy(FT_COPY, nil, 0);
			break;
		case kMCPlatformTextInputActionPaste:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fpaste(FT_PASTE, nil, 0);
			break;
		case kMCPlatformTextInputActionUndo:
			if (!MCactivefield -> getflag(F_LOCK_TEXT))
				MCactivefield -> fundo(FT_UNDO, nil, 0);
			break;
		case kMCPlatformTextInputActionRedo:
			break;
		case kMCPlatformTextInputActionDelete:
			MCactivefield -> deleteselection(False);
			break;
	};
}

////////////////////////////////////////////////////////////////////////////////

typedef bool (*pasteboard_resolve_callback_t)(MCPlatformPasteboardFlavor flavor, void*& r_data, size_t& r_data_size);

void MCPlatformHandlePasteboardResolve(MCPlatformPasteboardRef p_pasteboard, MCPlatformPasteboardFlavor p_flavor, void *p_handle, void *& r_data, size_t& r_data_size)
{
	void *t_data;
	size_t t_data_size;
	if (((pasteboard_resolve_callback_t)p_handle)(p_flavor, t_data, t_data_size))
	{
		r_data = t_data;
		r_data_size = t_data_size;
	}
	else
		r_data = nil, r_data_size = 0;
}

////////////////////////////////////////////////////////////////////////////////

static MCPlayer *find_player(MCPlatformPlayerRef p_player)
{
	for(MCPlayer *t_player = MCplayers; t_player != nil; t_player = t_player -> getnextplayer())
	{
		if (t_player -> getplatformplayer() == p_player)
            return t_player;
    }
    
    return nil;
}

void MCPlatformHandlePlayerFrameChanged(MCPlatformPlayerRef p_player)
{
    MCPlayer *t_player;
    t_player = find_player(p_player);
    if (t_player == nil)
        return;
    
    t_player -> layer_redrawall();
    MCPlatformBreakWait();
}

void MCPlatformHandlePlayerMarkerChanged(MCPlatformPlayerRef p_player, uint32_t p_time)
{
    MCPlayer *t_player;
    t_player = find_player(p_player);
    if (t_player == nil)
        return;
    
    t_player -> markerchanged(p_time);
}

void MCPlatformHandlePlayerCurrentTimeChanged(MCPlatformPlayerRef p_player)
{
    MCPlayer *t_player;
    t_player = find_player(p_player);
    if (t_player == nil)
        return;
    
    t_player -> timer(MCM_current_time_changed, nil);
}

void MCPlatformHandlePlayerSelectionChanged(MCPlatformPlayerRef p_player)
{
    MCPlayer *t_player;
    t_player = find_player(p_player);
    if (t_player == nil)
        return;
    
    // MW-2014-04-24: [[ Bug ]] Make sure we update the start/end time of the
    //   player.
    t_player -> selectionchanged();
}

void MCPlatformHandlePlayerStarted(MCPlatformPlayerRef p_player)
{
    MCPlayer *t_player;
    t_player = find_player(p_player);
    if (t_player == nil)
        return;
    
    t_player -> timer(MCM_play_started, nil);
}

void MCPlatformHandlePlayerPaused(MCPlatformPlayerRef p_player)
{
    MCPlayer *t_player;
    t_player = find_player(p_player);
    if (t_player == nil)
        return;
    
    t_player -> timer(MCM_play_paused, nil);
}

void MCPlatformHandlePlayerStopped(MCPlatformPlayerRef p_player)
{
    MCPlayer *t_player;
    t_player = find_player(p_player);
    if (t_player == nil)
        return;
    
    t_player -> timer(MCM_play_stopped, nil);
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformHandleSoundFinished(MCPlatformSoundRef p_sound)
{
    if (MCacptr != nil)
        MCscreen -> addtimer(MCacptr, MCM_internal, 0);
}

////////////////////////////////////////////////////////////////////////////////

#if 0
void MCPlatformProcess(MCPlatformCallback& p_callback)
{
	switch(p_callback . type)
	{
		case kMCPlatformCallbackApplicationStartup:
			MCPlatformHandleApplicationStartup(p_callb
			ack . application . startup . argc,
											   p_callback . application . startup . argv,
											   p_callback . application . startup . envp,
											   p_callback . application . startup . error_code,
											   p_callback . application . startup . error_message);
			break;
		case kMCPlatformCallbackApplicationShutdown:
			MCPlatformHandleApplicationShutdown(p_callback . application . shutdown . exit_code);
			break;
		case kMCPlatformCallbackApplicationShutdownRequest:
			MCPlatformHandleApplicationShutdownRequest(p_callback . application . shutdown_request . terminate);
			break;
		case kMCPlatformCallbackApplicationRun:
			MCPlatformHandleApplicationRun();
		default:
			break;
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////
