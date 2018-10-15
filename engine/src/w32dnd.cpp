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

#include "image.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"

#include "w32dc.h"
#include "w32dnd.h"
#include "w32-clipboard.h"

#include "graphics_util.h"

///////////////////////////////////////////////////////////////////////////////

static inline MCPoint MCPointFromWin32POINT(const POINT &p_point)
{
	return MCPointMake(p_point.x, p_point.y);
}

static inline MCPoint MCPointFromWin32POINT(const POINTL &p_point)
{
	return MCPointMake(p_point.x, p_point.y);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Drag-Drop Implementation
//

DropSource::DropSource(void)
{
	m_references = 0;
}

HRESULT DropSource::QueryInterface(REFIID p_iid, void** r_interface)
{
	if (IsEqualIID(p_iid, IID_IUnknown) || IsEqualIID(p_iid, IID_IDropSource))
	{
		*r_interface = this;
		AddRef();
		return S_OK;
	}
	
	*r_interface = NULL;
	return E_NOINTERFACE;
}

ULONG DropSource::AddRef(void)
{
	return ++m_references;
}

ULONG DropSource::Release(void)
{
	if (--m_references == 0)
	{
		delete this;
		return 0;
	}

	return m_references;
}

HRESULT DropSource::QueryContinueDrag(BOOL p_escape_pressed, DWORD p_key_state)
{
	if (p_escape_pressed)
		return DRAGDROP_S_CANCEL;

	if ((p_key_state & MK_LBUTTON) == 0)
		return DRAGDROP_S_DROP;

	return S_OK;
}

HRESULT DropSource::GiveFeedback(DWORD p_effect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

// SN-2014-07-11: [[ Bug 12769 ]] Update the signature - the non-implemented UIDC dodragdrop was called otherwise
MCDragAction MCScreenDC::dodragdrop(Window w, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint* p_image_offset)
{
	bool t_success;
	t_success = true;

	DropSource *t_drop_source;
	t_drop_source = NULL;
	if (t_success)
	{
		t_drop_source = new (nothrow) DropSource;
		if (t_drop_source != NULL)
			t_drop_source -> AddRef();
		else
			t_success = false;
	}
	
	// Get the IDataObject from the dragboard. Because we need one for the
	// COM drag-and-drop API, we call the method that forces one to be
	// created if it does not yet exist.
	MCRawClipboard* t_dragboard = MCdragboard->GetRawClipboard();
	IDataObject* t_transfer_data = (static_cast<MCWin32RawClipboardItem*>(t_dragboard->GetItemAtIndex(0)))->GetDataObject();

	IDragSourceHelper *t_source_helper;
	t_source_helper = NULL;
	if (t_success && p_image != NULL)
	{
		p_image -> openimage();

		if (CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDragSourceHelper), (void **)&t_source_helper) == S_OK)
		{
			MCAutoDataRef t_dib;
			p_image -> converttodragimage(&t_dib);
			if (*t_dib != nil)
			{
				const void *t_data;
				t_data = (const void *)MCDataGetBytePtr(*t_dib);
				
				HDC t_dc;
				t_dc = GetDC(static_cast<MCScreenDC *>(MCscreen) -> getinvisiblewindow());

				HBITMAP t_drag_bitmap;
				t_drag_bitmap = CreateDIBitmap(t_dc, (BITMAPINFOHEADER *)t_data, CBM_INIT, ((char *)t_data) + ((BITMAPINFOHEADER *)t_data) -> biSize, (BITMAPINFO *)t_data, DIB_RGB_COLORS);

				ReleaseDC(static_cast<MCScreenDC *>(MCscreen) -> getinvisiblewindow(), t_dc);

				if (t_drag_bitmap != NULL)
				{
					SHDRAGIMAGE t_drag_image;
					t_drag_image . sizeDragImage . cx = ((BITMAPINFOHEADER *)t_data) -> biWidth;
					t_drag_image . sizeDragImage . cy = -((BITMAPINFOHEADER *)t_data) -> biHeight; 
					t_drag_image . ptOffset . x = p_image_offset -> x;
					t_drag_image . ptOffset . y = p_image_offset -> y;
					t_drag_image . hbmpDragImage = t_drag_bitmap;
					t_drag_image . crColorKey = 0x010101;

					HRESULT t_err;
					t_err = t_source_helper -> InitializeFromBitmap(&t_drag_image, t_transfer_data);

					DeleteObject(t_drag_bitmap);
				}
			}
		}

		p_image -> closeimage();
	}

	MCDragAction t_action;
	t_action = DRAG_ACTION_NONE;
	if (t_success)
	{
		DWORD t_allowed_effects;
		t_allowed_effects = 0;
		if ((p_allowed_actions & DRAG_ACTION_COPY) != 0)
			t_allowed_effects |= DROPEFFECT_COPY;
		if ((p_allowed_actions & DRAG_ACTION_MOVE) != 0)
			t_allowed_effects |= DROPEFFECT_MOVE;
		if ((p_allowed_actions & DRAG_ACTION_LINK) != 0)
			t_allowed_effects |= DROPEFFECT_LINK;

		DWORD t_effect;
		if (DoDragDrop(t_transfer_data, t_drop_source, t_allowed_effects, &t_effect) == DRAGDROP_S_DROP)
		{
			if ((t_effect & DRAG_ACTION_COPY) != 0)
				t_action = DRAG_ACTION_COPY;
			else if ((t_effect & DRAG_ACTION_MOVE) != 0)
				t_action = DRAG_ACTION_MOVE;
			else if ((t_effect & DRAG_ACTION_LINK) != 0)
				t_action = DRAG_ACTION_LINK;
		}
	}

	if (t_source_helper != NULL)
		t_source_helper -> Release();

	if (t_transfer_data != NULL)
		t_transfer_data -> Release();

	if (t_drop_source != NULL)
		t_drop_source -> Release();

	return t_action;
}

///////////////////////////////////////////////////////////////////////////////

bool CDropTarget::m_helper_created = false;
uint4 CDropTarget::m_helper_references = 0;
IDropTargetHelper *CDropTarget::m_helper = NULL;

/*initialize drop target*/
CDropTarget::CDropTarget()
{
	m_refs = 1;
	dropstack = NULL;

	m_helper_references += 1;
}

void CDropTarget::setstack(MCStack *whichstack)
{
	dropstack = whichstack;
}

CDropTarget::~CDropTarget()
{
	m_helper_references -= 1;
	if (m_helper != NULL && m_helper_references == 0)
	{
		m_helper -> Release();
		m_helper = NULL;
		m_helper_created = false;
	}
}

/*query interface*/
STDMETHODIMP
CDropTarget::QueryInterface(REFIID iid, void FAR* FAR* ppv)
{
	if (iid == IID_IUnknown || iid == IID_IDropTarget)
	{
		*ppv = this;
		AddRef();
		return NOERROR;
	}
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CDropTarget::AddRef(void)
{
	return ++m_refs;
}

STDMETHODIMP_(ULONG) CDropTarget::Release(void)
{
	if (--m_refs == 0)
	{
		delete this;
		return 0;
	}
	return m_refs;
}

static uint4 keystate_to_modifierstate(DWORD p_key_state)
{
	uint4 t_modifier_state;
	t_modifier_state = 0;

	if ((p_key_state & MK_CONTROL) != 0)
		t_modifier_state |= MS_CONTROL;
	if ((p_key_state & MK_SHIFT) != 0)
		t_modifier_state |= MS_SHIFT;
	if ((p_key_state & MK_ALT) != 0)
		t_modifier_state |= MS_MOD1;

	return t_modifier_state;
}

STDMETHODIMP CDropTarget::DragEnter(LPDATAOBJECT pDataObj, DWORD grfKeyState,
                                    POINTL pt, LPDWORD pdwEffect)
{
	if (!m_helper_created && m_helper == NULL)
	{
		if (CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IDropTargetHelper, (void **)&m_helper) != S_OK)
			m_helper = NULL;

		m_helper_created = true;
	}

	if (m_helper != NULL)
		m_helper -> DragEnter((HWND)dropstack -> getw() -> handle . window, pDataObj, (POINT *)&pt, *pdwEffect);

	// Place the data object onto the dragboard
	MCWin32RawClipboardCommon* t_raw_clipboard = static_cast<MCWin32RawClipboardCommon*>(MCdragboard->GetRawClipboard());
	t_raw_clipboard->SetToIDataObject(pDataObj);

	uint4 t_old_modifierstate;
	t_old_modifierstate = MCmodifierstate;

	MCmodifierstate = keystate_to_modifierstate(grfKeyState);

	MCdispatcher -> wmdragenter(dropstack -> getw());
	*pdwEffect = 0;

	MCmodifierstate = t_old_modifierstate;

	return S_OK;
}

STDMETHODIMP CDropTarget::DragOver(DWORD grfKeyState, POINTL pt,
                                   LPDWORD pdwEffect)
{
	if (m_helper != NULL)
		m_helper -> DragOver((POINT *)&pt, *pdwEffect);

	ScreenToClient((HWND)dropstack -> getw() -> handle . window, (POINT *)&pt);

	uint4 t_old_modifierstate;
	t_old_modifierstate = MCmodifierstate;

	MCmodifierstate = keystate_to_modifierstate(grfKeyState);

	// IM-2014-05-06: [[ Bug 12319 ]] Convert screen to logical coords
	MCPoint t_mouseloc;
	t_mouseloc = MCscreen->screentologicalpoint(MCPointFromWin32POINT(pt));

	MCDragAction t_action;
	t_action = MCdispatcher -> wmdragmove(dropstack -> getw(), t_mouseloc . x, t_mouseloc . y);
	switch(t_action)
	{
	case DRAG_ACTION_NONE:
		*pdwEffect = DROPEFFECT_NONE;
	break;
	case DRAG_ACTION_COPY:
		*pdwEffect = DROPEFFECT_COPY;
	break;
	case DRAG_ACTION_MOVE:
		if ((*pdwEffect & DROPEFFECT_MOVE) != 0)
			*pdwEffect = DROPEFFECT_MOVE;
		else
			*pdwEffect = DROPEFFECT_COPY;
	break;
	case DRAG_ACTION_LINK:
		if ((*pdwEffect & DROPEFFECT_LINK) != 0)
			*pdwEffect = DROPEFFECT_LINK;
		else
			*pdwEffect = DROPEFFECT_COPY;
	break;
	}

	MCmodifierstate = t_old_modifierstate;

	return S_OK;
}

STDMETHODIMP CDropTarget::DragLeave()
{
	if (m_helper != NULL)
		m_helper -> DragLeave();

	MCdispatcher -> wmdragleave(dropstack -> getw());

	// Remove the data object from the dragboard
	MCWin32RawClipboardCommon* t_raw_clipboard = static_cast<MCWin32RawClipboardCommon*>(MCdragboard->GetRawClipboard());
	t_raw_clipboard->SetToIDataObject(NULL);

	return S_OK;
}

STDMETHODIMP CDropTarget::Drop(LPDATAOBJECT pDataObj, DWORD grfKeyState,
                               POINTL pt, LPDWORD pdwEffect)
{
	if (m_helper != NULL)
		m_helper -> Drop(pDataObj, (POINT *)&pt, *pdwEffect);

	ScreenToClient((HWND)dropstack -> getw() -> handle . window, (POINT *)&pt);

	// IM-2014-05-06: [[ Bug 12319 ]] Convert screen to logical coords
	MCPoint t_mouseloc;
	t_mouseloc = MCscreen->screentologicalpoint(MCPointFromWin32POINT(pt));

	uint4 t_old_modifierstate;
	t_old_modifierstate = MCmodifierstate;

	MCmodifierstate = keystate_to_modifierstate(grfKeyState);

	MCdispatcher -> wmdragmove(dropstack -> getw(), t_mouseloc . x, t_mouseloc . y);
	
	MCDragAction t_action;
	t_action = MCdispatcher -> wmdragdrop(dropstack -> getw());

	switch(t_action)
	{
	case DRAG_ACTION_NONE:
		*pdwEffect = DROPEFFECT_NONE;
	break;
	case DRAG_ACTION_COPY:
		*pdwEffect = DROPEFFECT_COPY;
	break;
	case DRAG_ACTION_MOVE:
		*pdwEffect = DROPEFFECT_MOVE;
	break;
	case DRAG_ACTION_LINK:
		*pdwEffect = DROPEFFECT_LINK;
	break;
	}

	MCmodifierstate = t_old_modifierstate;

	return S_OK;
}
