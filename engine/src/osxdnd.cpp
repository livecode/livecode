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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "field.h"
#include "stack.h"
#include "image.h"

#include "dispatch.h"
#include "util.h"
//#include "execpt.h"

#include "globals.h"

#include "mctheme.h"
#include "context.h"

#include "osxdc.h"
#include "osxtransfer.h"

///////////////////////////////////////////////////////////////////////////////

MCMacOSXDragPasteboard::MCMacOSXDragPasteboard(DragRef p_drag)
{
	m_drag = p_drag;
	Resolve();
}

bool MCMacOSXDragPasteboard::QueryFlavors(ScrapFlavorType*& r_types, uint4& r_type_count)
{
	bool t_success;
	t_success = true;
	
	DragItemRef t_item;
	if (t_success)
	{
		if (GetDragItemReferenceNumber(m_drag, 1, &t_item) != noErr)
			t_success = false;
	}
	
	UInt16 t_type_count;
	if (t_success)
		if (CountDragItemFlavors(m_drag, t_item, &t_type_count) != noErr)
			t_success = false;
	
	ScrapFlavorType *t_types;
	t_types = NULL;
	if (t_success)
	{
		t_types = new ScrapFlavorType[t_type_count];
		if (t_types == NULL)
			t_success = false;
	}
	
	if (t_success)
		for(uint4 i = 1; i <= t_type_count && t_success; ++i)
			if (GetFlavorType(m_drag, t_item, i, &t_types[i - 1]) != noErr)
				t_success = false;
	
	if (t_success)
	{
		r_types = t_types;
		r_type_count = t_type_count;
	}
	else
		delete t_types;

	return t_success;
}

bool MCMacOSXDragPasteboard::FetchFlavor(ScrapFlavorType p_type, MCDataRef &r_data)
{
	bool t_success;
	t_success = true;

	if (p_type == flavorTypeHFS)
	{
		UInt16 t_item_count;
		if (t_success)
			if (CountDragItems(m_drag, &t_item_count) != noErr)
				t_success = false;
		
		void *t_data;
		t_data = NULL;
		if (t_success)
		{
			t_data = malloc(sizeof(HFSFlavor) * t_item_count);
			if (t_data == NULL)
				t_success = false;
		}
		
		uint4 t_count;
		t_count = 0;
		if (t_success)
		{
			for(uint4 i = 1; i <= t_item_count && t_success; ++i)
			{
				DragItemRef t_item;
				if (GetDragItemReferenceNumber(m_drag, i, &t_item) != noErr)
					t_success = false;
				
				Size t_size;
				t_size = sizeof(HFSFlavor);
				if (GetFlavorData(m_drag, t_item, flavorTypeHFS, ((char *)t_data) + sizeof(HFSFlavor) * t_count, &t_size, 0) == noErr)
					t_count += 1;
			}
		}
		
		if (t_success)
		{
			t_data = realloc(t_data, sizeof(HFSFlavor) * t_count);
			t_success = MCDataCreateWithBytesAndRelease((byte_t *)t_data, sizeof(HFSFlavor) * t_count, r_data);
		}
		
		if (!t_success)
		{
			if (t_data != NULL)
				free(t_data);
		}
		
		return t_success;
	}

	DragItemRef t_item;
	if (t_success)
		if (GetDragItemReferenceNumber(m_drag, 1, &t_item) != noErr)
			t_success = false;
			
	Size t_data_size;
	t_data_size = 0;
	if (t_success)
		if (GetFlavorDataSize(m_drag, t_item, p_type, &t_data_size) != noErr)
			t_success = false;
			
	void *t_data;
	t_data = NULL;
	if (t_success)
	{
		t_data = malloc(t_data_size);
		if (t_data == NULL)
			t_success = false;
	}
	
	if (t_success)
	{
		if (GetFlavorData(m_drag, t_item, p_type, t_data, &t_data_size, 0) != noErr)
			t_success = false;
	}

	if (t_success)
		t_success = MCDataCreateWithBytesAndRelease((char_t *)t_data, t_data_size, r_data);
	
	if (!t_success)
	{
		if (t_data != NULL)
			free(t_data);
	}

	return t_success;
}

///////////////////////////////////////////////////////////////////////////////

static bool PublishDragFlavor(MCMacOSXTransferData *p_data, ScrapFlavorType p_type, void *p_context)
{
	if (p_type == flavorTypeHFS)
	{
		MCAutoDataRef t_files;
		if (!p_data -> Subscribe(flavorTypeHFS, &t_files))
			return false;
		
		for(uint4 i = 0; i < MCDataGetLength(*t_files) / sizeof(HFSFlavor); ++i)
			AddDragItemFlavor((DragRef)p_context, (DragItemRef)(i + 1), p_type, &((HFSFlavor *)MCDataGetBytePtr(*t_files))[i], sizeof(HFSFlavor), 0L);
		
		return true;
	}
	else if (AddDragItemFlavor((DragRef)p_context, (DragItemRef)1, p_type, NULL, 0, 0L) != noErr)
		return false;

	return true;
}

static OSErr SubscribeDragFlavor(FlavorType p_type, void *p_context, DragItemRef p_item, DragRef p_drag)
{
	MCMacOSXTransferData *t_data;
	t_data = (MCMacOSXTransferData *)p_context;

	OSStatus t_status;
	t_status = noErr;

	MCAutoDataRef t_string;
	if (!t_data -> Subscribe(p_type, &t_string))
		t_status = cantGetFlavorErr;

	if (t_status == noErr)
		t_status = SetDragItemFlavorData(p_drag, p_item, p_type, MCDataGetBytePtr(*t_string), MCDataGetLength(*t_string), 0);
	
	return t_status;
}

///////////////////////////////////////////////////////////////////////////////

MCDragAction MCScreenDC::dodragdrop(MCPasteboard* p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint *p_image_offset)
{
	bool t_success;
	t_success = true;

	MCMacOSXTransferData *t_data;
	t_data = NULL;
	if (t_success)
	{
		t_data = new MCMacOSXTransferData;
		if (t_data == NULL)
			t_success = false;
	}

	if (t_success)
		t_success = t_data -> Publish(p_pasteboard);

	DragRef t_drag;
	t_drag = NULL;
	if (t_success)
	{
		if (NewDrag(&t_drag) != noErr)
			t_success = false;
	}

	if (t_success && m_drag_send_data_upp == NULL)
	{
		m_drag_send_data_upp = NewDragSendDataUPP(SubscribeDragFlavor);
		if (m_drag_send_data_upp == NULL)
			t_success = false;
	}

	if (t_success)
		if (SetDragSendProc(t_drag, m_drag_send_data_upp, t_data) != noErr)
			t_success = false;

	if (t_success && p_image != NULL)
	{
		p_image->openimage();
	
		CGImageRef t_cg_image;
		t_cg_image = NULL;

		HIPoint t_hi_offset;
		t_hi_offset . x = -p_image_offset -> x;
		t_hi_offset . y = -p_image_offset -> y;

		t_cg_image = p_image -> converttodragimage();
	
		if (t_cg_image != NULL)
		{
			SetDragImageWithCGImage(t_drag, t_cg_image, &t_hi_offset, kDragStandardTranslucency);
			CGImageRelease(t_cg_image);
		}
		
		p_image->closeimage();
	}

	if (t_success)
		t_success = t_data -> ForEachFlavor(PublishDragFlavor, t_drag);

	MCDragAction t_action;
	t_action = DRAG_ACTION_NONE;
	if (t_success)
	{
		DragActions t_allowed_actions;
		t_allowed_actions = 0;
		if ((p_allowed_actions & DRAG_ACTION_COPY) != 0)
			t_allowed_actions |= kDragActionCopy;
		if ((p_allowed_actions & DRAG_ACTION_MOVE) != 0)
			t_allowed_actions |= kDragActionMove;
		if ((p_allowed_actions & DRAG_ACTION_LINK) != 0)
			t_allowed_actions |= kDragActionAlias;
		SetDragAllowableActions(t_drag, t_allowed_actions, false);
		
		RgnHandle t_rgn;
		t_rgn = NewRgn();

		OSErr t_err;
		t_err = TrackDrag(t_drag, &m_drag_event, t_rgn);
		if (t_err == noErr)
		{
			DragActions t_drag_action;
			t_action = DRAG_ACTION_NONE;
			GetDragDropAction(t_drag, &t_drag_action);
			if ((t_drag_action & kDragActionMove) != 0)
				t_action = DRAG_ACTION_MOVE;
			else if ((t_drag_action & kDragActionAlias) != 0)
				t_action = DRAG_ACTION_LINK;
			else
				t_action = DRAG_ACTION_COPY;
		}
		
		SetThemeCursor(kThemeArrowCursor);
	}

	if (t_drag != NULL)
		DisposeDrag(t_drag);

	if (t_data != NULL)
		delete t_data;

	return t_action;
}

///////////////////////////////////////////////////////////////////////////////

static DragActions to_mac_drag_action(MCDragAction p_action)
{
	switch(p_action)
	{
	case DRAG_ACTION_COPY:
		return kDragActionCopy;
	break;

	case DRAG_ACTION_MOVE:
		return kDragActionMove;
	break;

	case DRAG_ACTION_LINK:
		return kDragActionAlias;
	break;
	}

	return kDragActionNothing;
}

static MCDragAction from_mac_drag_action(DragActions p_action)
{
	if (p_action == 0)
		return DRAG_ACTION_NONE;

	if ((p_action & kDragActionMove) != 0)
		return DRAG_ACTION_MOVE;

	if ((p_action & kDragActionAlias) != 0)
		return DRAG_ACTION_LINK;

	return DRAG_ACTION_COPY;
}

static uint4 keystate_to_modifierstate(SInt16 p_keys)
{
	uint4 t_modifiers;
	t_modifiers = 0;
	if (p_keys & shiftKey)
		t_modifiers |= MS_SHIFT;
	if (p_keys & cmdKey)
		t_modifiers |= MS_CONTROL;
	if (p_keys & optionKey)
		t_modifiers |= MS_MOD1;
	return t_modifiers;
}

pascal OSErr MCScreenDC::DragTrackingHandler(DragTrackingMessage p_message, WindowRef p_window, void *p_context, DragRef p_drag)
{
	_Drawable _dw;
	_dw.type = DC_WINDOW;
	_dw.handle.window = (MCSysWindowHandle)p_window;
	MCStack *t_stack = MCdispatcher -> findstackd(&_dw);

	if (t_stack == NULL)
		return dragNotAcceptedErr;

	uint4 t_old_modifier_state;
	t_old_modifier_state = MCmodifierstate;

	SInt16 t_modifiers;
	GetDragModifiers(p_drag, &t_modifiers, NULL, NULL);
	MCmodifierstate = keystate_to_modifierstate(t_modifiers);

	MCDragAction t_action;
	t_action = DRAG_ACTION_NONE;

	// MW-2013-01-22: [[ Bug 10638 ]] Use &_dw to pass to the dispatcher
	//   functions to ensure they find the same stack.

	switch(p_message)
	{
	case kDragTrackingEnterWindow:
	{
		MCPasteboard *t_pasteboard;
		t_pasteboard = new MCMacOSXDragPasteboard(p_drag);
		MCdispatcher -> wmdragenter(&_dw, t_pasteboard);
		t_pasteboard -> Release();
	}
	break;

	case kDragTrackingInWindow:
	{
		Point t_point;
		GetDragMouse(p_drag, &t_point, NULL);
		GlobalToLocal(&t_point);
		
		// IM-2013-12-09: [[ Bug 11563 ]] Remove scroll adjustment as this is now taken care of in MCDispatch::wmdragmove

		t_action = MCdispatcher -> wmdragmove(&_dw, t_point . h, t_point . v);
		switch(t_action)
		{
		case DRAG_ACTION_NONE:
			SetThemeCursor(kThemeNotAllowedCursor);
		break;
		
		case DRAG_ACTION_COPY:
			SetThemeCursor(kThemeCopyArrowCursor);
		break;
		
		case DRAG_ACTION_LINK:
			SetThemeCursor(kThemeAliasArrowCursor);
		break;
		
		default:
			SetThemeCursor(kThemeArrowCursor);
		break;
		}
	}
	break;

	case kDragTrackingLeaveWindow:
	{
		MCdispatcher -> wmdragleave(&_dw);
	}
	break;
	}

	MCmodifierstate = t_old_modifier_state;

	DragActions t_mac_action;
	t_mac_action = to_mac_drag_action(t_action);
	SetDragDropAction(p_drag, t_mac_action);
	
	return t_mac_action == kDragActionNothing ? (OSErr)dragNotAcceptedErr : (OSErr)noErr;
}

pascal OSErr MCScreenDC::DragReceiveHandler(WindowPtr p_window, void *p_context, DragRef p_drag)
{
	_Drawable _dw;
	_dw.type = DC_WINDOW;
	_dw.handle.window = (MCSysWindowHandle)p_window;
	MCStack *t_stack = MCdispatcher -> findstackd(&_dw);

	if (t_stack == NULL)
		return dragNotAcceptedErr;

	uint4 t_old_modifier_state;
	t_old_modifier_state = MCmodifierstate;

	SInt16 t_modifiers;
	GetDragModifiers(p_drag, NULL, NULL, &t_modifiers);
	MCmodifierstate = keystate_to_modifierstate(t_modifiers);
	
	Point t_point;
	GetDragMouse(p_drag, &t_point, NULL);
	GlobalToLocal(&t_point);
	
	// IM-2013-12-09: [[ Bug 11563 ]] Remove scroll adjustment as this is now taken care of in MCDispatch::wmdragmove

	SetThemeCursor(kThemeArrowCursor);

	MCDragAction t_action;
	t_action = MCdispatcher -> wmdragmove(t_stack -> getw(), t_point . h, t_point . v);

	DragActions t_mac_action;
	t_mac_action = kDragActionNothing;
	if (t_action != DRAG_ACTION_NONE)
	{
		t_action = MCdispatcher -> wmdragdrop(t_stack -> getw());
		t_mac_action = to_mac_drag_action(t_action);
	}

	MCmodifierstate = t_old_modifier_state;

	// MW-2010-10-12: [[ Bug 8991 ]] Use this to break out of WNE
	PostEvent(mouseUp, 0);
	
	return t_mac_action == kDragActionNothing ? dragNotAcceptedErr : noErr;
}
