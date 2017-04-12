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
#include "parsedef.h"
#include "objdefs.h"

#include "dispatch.h"
#include "globals.h"
#include "uidc.h"
#include "image.h"

#include "w32dc.h"

////////////////////////////////////////////////////////////////////////////////

enum MCCursorKind
{
	// The hidden cursor
	kMCCursorNone,

	// A standard 'theme' cursor
	kMCCursorStandard,

	// A custom cursor made from an image
	kMCCursorCustom
};

struct MCCursor
{
	MCCursorKind kind;
	union
	{
		LPCSTR standard;
		HCURSOR custom;
	};
};

/*
Edit Pointer	1
Pointer			29
Hand			28
Cross			37

Busy			16-23

Fill			5
Pencil			11
Brush			35
Dropper			12
Spray			34
Eraser			33

Object Dropper	38

Zoom In			39
Zoom Out		40

Resize Split V	30
Resize Split H	31
Resize

Drag-Add		32
Drag-Subtract	36
Drag-Text		24
Drag-Text-Add	25

*/

////////////////////////////////////////////////////////////////////////////////

#define IDC_ARROWA           MAKEINTRESOURCEA(32512)
#define IDC_IBEAMA           MAKEINTRESOURCEA(32513)
#define IDC_WAITA            MAKEINTRESOURCEA(32514)
#define IDC_CROSSA           MAKEINTRESOURCEA(32515)
#define IDC_UPARROWA         MAKEINTRESOURCEA(32516)
#define IDC_SIZEA            MAKEINTRESOURCEA(32640)  /* OBSOLETE: use IDC_SIZEALL */
#define IDC_ICONA            MAKEINTRESOURCEA(32641)  /* OBSOLETE: use IDC_ARROW */
#define IDC_SIZENWSEA        MAKEINTRESOURCEA(32642)
#define IDC_SIZENESWA        MAKEINTRESOURCEA(32643)
#define IDC_SIZEWEA          MAKEINTRESOURCEA(32644)
#define IDC_SIZENSA          MAKEINTRESOURCEA(32645)
#define IDC_SIZEALLA         MAKEINTRESOURCEA(32646)
#define IDC_NOA              MAKEINTRESOURCEA(32648)
#define IDC_HELPA            MAKEINTRESOURCEA(32651)

static LPCSTR kMCStandardWindowsCursors[] =
{
	nil, /* PI_NONE */ 
	IDC_ARROWA, /* PI_ARROW */ 
	IDC_ARROWA, /* PI_BRUSH */ 
	IDC_ARROWA, /* PI_SPRAY */ 
	IDC_ARROWA, /* PI_ERASER */
	IDC_ARROWA, /* PI_BUCKET */
	IDC_WAITA, /* PI_BUSY */
	IDC_CROSSA, /* PI_CROSS */
	IDC_ARROWA, /* PI_HAND */
	IDC_IBEAMA, /* PI_IBEAM */
	IDC_ARROWA, /* PI_LR */
	IDC_ARROWA, /* PI_PENCIL */
	IDC_ARROWA, /* PI_DROPPER */
	IDC_CROSSA, /* PI_PLUS */
	IDC_WAITA, /* PI_WATCH */
	IDC_HELPA, /* PI_HELP */
	IDC_WAITA, /* PI_BUSY1 */
	IDC_WAITA, /* PI_BUSY2 */
	IDC_WAITA, /* PI_BUSY3 */
	IDC_WAITA, /* PI_BUSY4 */
	IDC_WAITA, /* PI_BUSY5 */
	IDC_WAITA, /* PI_BUSY6 */
	IDC_WAITA, /* PI_BUSY7 */
	IDC_WAITA, /* PI_BUSY8 */
	IDC_ARROWA, /* PI_DRAGTEXT */
	IDC_ARROWA, /* PI_DRAGCLONE */
	IDC_ARROWA, /* PI_DRAGREFUSE */
	IDC_SIZEA, /* PI_SIZEV */
	IDC_SIZEA, /* PI_SIZEH */
};

////////////////////////////////////////////////////////////////////////////////

static MCCursorRef create_standard_cursor(LPCSTR p_cursor)
{
	MCCursorRef t_cursor;
	t_cursor = new (nothrow) MCCursor;
	// IM-2013-07-17: [[ bug 9836 ]] set up the nil string as the empty 'none' cursor
	if (p_cursor == nil)
		t_cursor -> kind = kMCCursorNone;
	else
		t_cursor -> kind = kMCCursorStandard;
	t_cursor -> standard = p_cursor;
	return t_cursor;
}

static MCCursorRef create_custom_cursor(HCURSOR p_cursor)
{
	MCCursorRef t_cursor;
	t_cursor = new (nothrow) MCCursor;
	t_cursor -> kind = kMCCursorCustom;
	t_cursor -> custom = p_cursor;
	return t_cursor;
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::resetcursors(void)
{
	// MW-2010-09-10: Make sure no stacks reference one of the standard cursors
	MCdispatcher -> clearcursors();

	// XP and above allow alpha-blended cursors
	if (MCmajorosversion >= 0x0501)
	{
		MCcursormaxsize = 128;
		MCcursorcanbealpha = True;

		HDC winhdc = GetDC(NULL);
		if (GetDeviceCaps(winhdc, BITSPIXEL) < 24)
			MCcursorcanbealpha = False;
		else
			MCcursorcanbealpha = True;

		ReleaseDC(NULL, winhdc);
	}
	else
	{
		MCcursormaxsize = GetSystemMetrics(SM_CXCURSOR);
		MCcursorcanbealpha = False;
	}

	// Need to check: Windows 2000 for color cursors?
	if (MCmajorosversion >= 0x0500)
		MCcursorcanbecolor = True;
	else
		MCcursorbwonly = True;

	// IM-2013-07-17: [[ bug 9836 ]] ensure we create an empty cursor at index 0
	for(uint32_t i = 0; i < PI_NCURSORS; i++)
	{
		freecursor(MCcursors[i]);
		MCcursors[i] = nil;

		MCImage *t_image;
		t_image = (MCImage *)MCdispatcher -> getobjid(CT_IMAGE, i);
		if (t_image != nil)
			MCcursors[i] = t_image -> createcursor();
		else
			MCcursors[i] = create_standard_cursor(kMCStandardWindowsCursors[i]);
	}
}

void MCScreenDC::setcursor(Window p_window, MCCursorRef p_cursor)
{
	if (p_cursor == nil || p_cursor -> kind == kMCCursorNone)
		SetCursor(NULL);
	else if (p_cursor -> kind == kMCCursorStandard)
		SetCursor(LoadCursorA(nil, p_cursor -> standard));
	else
		SetCursor(p_cursor -> custom);
}

bool MCImageCreateIcon(MCImageBitmap *p_bitmap, uint32_t p_width, uint32_t p_height, bool p_cursor, uint32_t p_xhot, uint32_t p_yhot, HICON &r_icon);
MCCursorRef MCScreenDC::createcursor(MCImageBitmap *p_image, int2 p_xhot, int2 p_yhot)
{
	// PM-2016-03-21: [[ Bug 17042 ]] Make sure the image actually exists
	if (p_image == nil)
        return nil;
		
	HCURSOR t_alpha_cursor = nil;
	/* UNCHECKED */ MCImageCreateIcon(p_image, p_image->width, p_image->height, true, p_xhot, p_yhot, t_alpha_cursor);

	return create_custom_cursor(t_alpha_cursor);
}

void MCScreenDC::freecursor(MCCursorRef p_cursor)
{
	if (p_cursor == nil)
		return;

	if (p_cursor -> kind == kMCCursorCustom)
		DestroyIcon((HICON)p_cursor -> custom);

	delete p_cursor;
}

////////////////////////////////////////////////////////////////////////////////

