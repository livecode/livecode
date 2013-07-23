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

#include "w32prefix.h"

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
	/* PI_HELP */
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
	t_cursor = new MCCursor;
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
	t_cursor = new MCCursor;
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

MCCursorRef MCScreenDC::createcursor(MCImageBuffer *p_image, int2 p_xhot, int2 p_yhot)
{
	DWORD t_cursor_width;
	t_cursor_width = p_image -> width;

	DWORD t_cursor_height;
	t_cursor_height = p_image -> height;

	BITMAPV4HEADER t_bitmap_header;
	ZeroMemory(&t_bitmap_header, sizeof(BITMAPV4HEADER));
    t_bitmap_header.bV4Size = sizeof(BITMAPV4HEADER);
    t_bitmap_header.bV4Width = t_cursor_width;
    t_bitmap_header.bV4Height = -(signed)t_cursor_height;
    t_bitmap_header.bV4Planes = 1;
    t_bitmap_header.bV4BitCount = 32;
    t_bitmap_header.bV4V4Compression = BI_RGB;

    // The following mask specification specifies a supported 32 BPP
    // alpha format for Windows XP.
    t_bitmap_header.bV4RedMask   =  0x00FF0000;
    t_bitmap_header.bV4GreenMask =  0x0000FF00;
    t_bitmap_header.bV4BlueMask  =  0x000000FF;
    t_bitmap_header.bV4AlphaMask =  0xFF000000; 

	HDC t_hdc;
	t_hdc = GetDC(NULL);

	HBITMAP t_bitmap;
	t_bitmap = CreateDIBitmap(t_hdc, (BITMAPINFOHEADER *)&t_bitmap_header, CBM_INIT, p_image -> data, (BITMAPINFO *)&t_bitmap_header, DIB_RGB_COLORS);

    ReleaseDC(NULL, t_hdc);

	// Now build the mask bitmap
    HBITMAP t_mask_bitmap;
	uint32_t t_mask_stride;
	uint8_t *t_mask_bits;
	t_mask_bitmap = NULL;
	t_mask_stride = ((p_image -> width + 15) & ~15) / 8;
	t_mask_bits = (uint8_t *)malloc(t_mask_stride * p_image -> height);
	if (t_mask_bits != nil)
	{
		memset(t_mask_bits, 255, t_mask_stride * p_image -> height);
		for(int32_t y = 0; y < p_image -> height; y++)
			for(int32_t x = 0; x < p_image -> width; x++)
			{
				if ((((uint32_t *)p_image -> data)[y * p_image -> stride / 4 + x] >> 24) != 0)
					t_mask_bits[y * t_mask_stride + (x >> 3)] &= ~(1 << (7 - (x & 7)));
			}
		t_mask_bitmap = CreateBitmap(t_cursor_width, t_cursor_height, 1, 1, t_mask_bits);
		free(t_mask_bits);
	}

	ICONINFO t_icon_info;
	t_icon_info . fIcon = FALSE;  // Change fIcon to TRUE to create an alpha icon
    t_icon_info . xHotspot = p_xhot;
    t_icon_info . yHotspot = p_yhot;
    t_icon_info . hbmMask = t_mask_bitmap;
    t_icon_info . hbmColor = t_bitmap;

	 // Create the alpha cursor with the alpha DIB section.
    HCURSOR t_alpha_cursor;
	t_alpha_cursor = CreateIconIndirect(&t_icon_info);

    DeleteObject(t_bitmap);          
    DeleteObject(t_mask_bitmap); 

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

