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

#ifndef __W32DND__
#define __W32DND__

#include <shobjidl.h>
#include <shlobj.h>
#include <shlwapi.h>

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    DropSource
//
//  Type:
//    Private platform-specific utility class
//
//  Description:
//    The DropSource class provides the LiveCode-specific implementation of
//    the OLE IDropSource interface.
//
//    It is only intended to be used by the implementation of
//    MCScreenDC::dodragdrop.
//
//    Note that the initial reference count after construction is zero. An
//    AddRef call must be made before passing a reference on.
//
//  Sample Usage:
//    DropSource *t_drop_source;
//    t_drop_source = new DropSource;
//    if (t_drop_source != NULL)
//    {
//        t_drop_source -> AddRef();
//        <use t_drop_source to do drag-drop>
//        t_drop_souce -> Release();
//    }
//
//  Known Issues:
//    <none>
//
class DropSource: public IDropSource
{
public:
	DropSource(void);

	STDMETHOD(QueryInterface)(REFIID p_id, void** r_object);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	STDMETHOD(QueryContinueDrag)(BOOL p_escape_pressed, DWORD p_key_state);
	STDMETHOD(GiveFeedback)(DWORD p_effect);

private:
	ULONG m_references;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    CDropTarget
//
//  Type:
//    Private platform-specific utility class
//
//  Description:
//    The CDropTarget class is the LiveCode-specific implementation of the
//    OLE IDropTarget interface.
//
//    An instance of the object is created for each stack when running on
//    Windows and each instance references the stack pointer. This is used
//    to perform the appropriate callbacks to MCDispatch::wmdrag* when
//    the application is running as a drop-target.
//
//    In addition, the implementation automatically creates and uses the
//    IDropTargetHelper interface provided the Windows Shell on Windows ME
//    platforms and above. This allows drag-images to be displayed
//    correctly.
//
//    Note that the current implementation is designed to be used inline in
//    the containing stack. Attempting to use this class outside of that
//    context will cause problems.
//
//  Sample Usage:
//    CDropTarget *t
//
//  Known Issues:
//    <none>
//
class FAR CDropTarget : public IDropTarget
{
public:
	CDropTarget();
	~CDropTarget();
	
	void setstack(MCStack *whichstack);

	/* IUnknown methods */
	STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);
	/* IDropTarget methods */
	STDMETHOD(DragEnter)(LPDATAOBJECT pDataObj, DWORD grfKeyState,
	                     POINTL pt, LPDWORD pdwEffect);
	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
	STDMETHOD(DragLeave)();
	STDMETHOD(Drop)(LPDATAOBJECT pDataObj, DWORD grfKeyState,
	                POINTL pt, LPDWORD pdwEffect);
private:
	ULONG m_refs;
	MCStack *dropstack;

	static IDropTargetHelper *m_helper;
	static uint4 m_helper_references;
	static bool m_helper_created;
};

#endif
