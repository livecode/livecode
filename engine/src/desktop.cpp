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

void MCPlatformHandleApplicationRun(void)
{
	X_main_loop();
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformHandleScreenParametersChanged(void)
{
	// Force a refresh of the cached display info.
	free(MCScreenDC::s_monitor_displays);
	MCScreenDC::s_monitor_count = 0;
	
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
		
		// IM-2013-09-30: [[ FullscreenMode ]] Translate mouse location to stack coords		
		// IM-2013-10-03: [[ FullscreenMode ]] Transform mouseloc based on the mousestack
		MCPoint t_mouseloc;
		t_mouseloc = MCmousestackptr->windowtostackloc(p_location);
		
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
			t_target -> mdown(p_button + 1);
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
