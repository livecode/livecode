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
#include "objdefs.h"
#include "parsedef.h"

#include "globals.h"
#include "stacklst.h"
#include "stack.h"
#include "contextscalewrapper.h"
#include "text.h"

#include "w32dc.h"
#include "w32context.h"
#include "w32theme.h"

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM WIDGET.CPP
//

void *MCWidgetContextLockNative(MCContext *p_context)
{
	HDC t_dc;
	t_dc = static_cast<MCGDIContext *>(p_context) -> gethdc();
	SaveDC(t_dc);
	return t_dc;
}

void MCWidgetContextUnlockNative(MCContext *p_context)
{
	HDC t_dc;
	t_dc = static_cast<MCGDIContext *>(p_context) -> gethdc();
	RestoreDC(t_dc, -1);
}

void MCWidgetContextBeginOffscreen(MCContext *p_context, void*& r_dc)
{
	RECT t_rect;
	t_rect . left = 0;
	t_rect . top = 0;
	t_rect . right = (p_context -> getclip() . width * 2540 + 95) / 96;
	t_rect . bottom = (p_context -> getclip() . height * 2540 + 95) / 96;

	HDC t_metafile_dc;
	t_metafile_dc = CreateEnhMetaFileA(NULL, NULL, &t_rect, NULL);

	SetWindowOrgEx(t_metafile_dc, p_context -> getclip() . x, p_context -> getclip() . y, NULL);

	r_dc = t_metafile_dc;
}

void MCWidgetContextEndOffscreen(MCContext *p_context, void *p_dc)
{
	HDC t_metafile_dc;
	t_metafile_dc = (HDC)p_dc;

	HENHMETAFILE t_metafile;
	t_metafile = CloseEnhMetaFile(t_metafile_dc);

	UINT t_size;
	t_size = GetEnhMetaFileBits(t_metafile, 0, NULL);

	void *t_buffer;
	t_buffer = malloc(t_size);
	if (t_buffer != NULL)
	{
		GetEnhMetaFileBits(t_metafile, t_size, (LPBYTE)t_buffer);
		p_context -> drawpict((uint1 *)t_buffer, t_size, true, p_context -> getclip(), p_context -> getclip());
		free(t_buffer);
	}

	DeleteEnhMetaFile(t_metafile);
}

void MCWidgetInvalidateRect(Window p_window, const MCRectangle& p_rect)
{
	RECT t_area;
	t_area . left = p_rect . x;
	t_area . top = p_rect . y;
	t_area . right = p_rect . x + p_rect . width;
	t_area . bottom = p_rect . y + p_rect . height;
	InvalidateRect((HWND)p_window -> handle . window, &t_area, FALSE);
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM STACKLST.CPP
//

void MCStacklist::hidepalettes(Boolean hide)
{
	active = !hide;
	if (stacks == NULL)
		return;
	MCStacknode *tptr = stacks;
	// only hide palettes if a non-palette is open
	Boolean dohide = False;
	do
	{
		MCStack *sptr = tptr->getstack();
		if (sptr->getrealmode() < WM_PALETTE)
		{
			dohide = True;
			break;
		}
		tptr = tptr->next();
	}
	while (tptr != stacks);
	if (!dohide)
		return;

	restart = False;
	tptr = stacks;
	do
	{
		MCStack *sptr = tptr->getstack();
		if (sptr->getrealmode() == WM_PALETTE && sptr->getflag(F_VISIBLE))
			if (MChidepalettes)
			{
				// Show the window non-active (to avoid messing with the focus),
				// then send a synthetic activate event to force a title-bar redraw
				ShowWindow((HWND)sptr->getw()->handle.window, hide ? SW_HIDE : SW_SHOWNA);
				PostMessageA((HWND)sptr->getw()->handle.window, WM_NCACTIVATE, 1, 0);
				// When closing or opening a window, Win32 delivers messages that can
				// change stack list, and we need to start over if this happened
				if (restart)
				{
					hidepalettes(hide);
					return;
				}

			}
			else if (sptr->getw() != NULL)
			{
				// OK-2007-04-19: Bug 4728, When hiding a palette with a windowShape
				// sptr->getw() can return null, causing crash here.
				PostMessageA((HWND)sptr->getw()->handle.window, WM_NCACTIVATE, !hide, 0);
			}
				
		tptr = tptr->next();
	}
	while (tptr != stacks);
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM CONTEXTSCALEWRAPPER.CPP
//

void MCContextScaleWrapper::drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_parameters)
{
	MCRectangle t_bounds = p_parameters -> bounds;
	MCContext *t_context = MCscreen->creatememorycontext(t_bounds.width, t_bounds.height, true, true);
	t_context->setorigin(t_bounds.x, t_bounds.y);
	t_context->clearclip();
	t_context->drawtheme(p_type, p_parameters);
	((MCGDIContext*)t_context)->flush_mask();

	HDC t_dsthdc = ((MCGDIContext*)m_context)->gethdc();
	HDC t_srchdc = ((MCGDIContext*)t_context)->gethdc();

	StretchBlt(t_dsthdc, t_bounds.x * scale, t_bounds.y * scale, t_bounds.width * scale, t_bounds.height * scale, t_srchdc, t_bounds.x, t_bounds.y, t_bounds.width, t_bounds.height, SRCCOPY);

	MCscreen->freecontext(t_context);
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM CUSTOMPRINTER.CPP
//

int32_t MCCustomPrinterComputeFontSize(void *p_font)
{
	HDC t_dc;
	t_dc = static_cast<MCScreenDC *>(MCscreen) -> getsrchdc();

	HGDIOBJ t_old_font;
	t_old_font = SelectObject(t_dc, p_font);

	TEXTMETRICA t_metrics;
	GetTextMetricsA(t_dc, &t_metrics);

	int32_t t_font_size;
	t_font_size = t_metrics . tmHeight - t_metrics . tmInternalLeading;

	SelectObject(t_dc, t_old_font);

	return t_font_size;
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM TEXT.CPP
//

bool MCSTextConvertToUnicode(MCTextEncoding p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
{
	if (p_input_length == 0)
	{
		r_used = 0;
		return true;
	}

	UINT t_codepage;
	if (p_input_encoding >= kMCTextEncodingWindowsNative)
		t_codepage = p_input_encoding - kMCTextEncodingWindowsNative;
	else if (p_input_encoding >= kMCTextEncodingMacNative)
		t_codepage = 10000 + p_input_encoding - kMCTextEncodingMacNative;
	else
	{
		r_used = 0;
		return true;
	}

	// MW-2009-08-27: It is possible for t_codepage == 65001 which means UTF-8. In this case we can't
	//   use the precomposed flag...

	int t_required_size;
	t_required_size = MultiByteToWideChar(t_codepage, t_codepage == 65001 ? 0 : MB_PRECOMPOSED, (LPCSTR)p_input, p_input_length, NULL, 0);
	if (t_required_size > (int)p_output_length / 2)
	{
		r_used = t_required_size * 2;
		return false;
	}

	int t_used;
	t_used = MultiByteToWideChar(t_codepage, t_codepage == 65001 ? 0 : MB_PRECOMPOSED, (LPCSTR)p_input, p_input_length, (LPWSTR)p_output, p_output_length);
	r_used = t_used * 2;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM GLOBALS.CPP
//

MCUIDC *MCCreateScreenDC(void)
{
	return new MCScreenDC;
}
