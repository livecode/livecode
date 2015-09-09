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

#include "w32prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "transfer.h"
//#include "execpt.h"
#include "image.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"

#include "w32dc.h"
#include "w32transfer.h"

void MCScreenDC::flushclipboard(void)
{
	if (ownsclipboard())
	{
		OleFlushClipboard();
		if (m_clipboard != NULL)
		{
			m_clipboard -> Release();
			m_clipboard = NULL;
		}
	}
}

bool MCScreenDC::ownsclipboard(void)
{
	if (m_clipboard == NULL)
		return false;

	bool t_is_current;
	t_is_current = OleIsCurrentClipboard(m_clipboard) == S_OK;
	if (!t_is_current)
	{
		m_clipboard -> Release();
		m_clipboard = NULL;
	}

	return t_is_current;
}

bool MCScreenDC::setclipboard(MCPasteboard *p_pasteboard)
{
	// If p_pasteboard is NULL then we are relinquishing the clipboard
	// So check if we still own it and if so reset it to nothing.
	if (p_pasteboard == NULL)
	{
		if (ownsclipboard())
			OleSetClipboard(NULL);
		return true;
	}

	bool t_success;
	t_success = true;

	// Attempt to create the new data object (an instance of TransferData)
	TransferData *t_new_clipboard;
	t_new_clipboard = NULL;
	if (t_success)
	{
		t_new_clipboard = new TransferData;
		if (t_new_clipboard != NULL)
			t_new_clipboard -> AddRef();
		else
			t_success = false;
	}

	// Attempt to add the formats to the data object
	if (t_success)
		t_success = t_new_clipboard -> Publish(p_pasteboard);

	// Attempt to actually change the system clipboard
	if (t_success)
		t_success = OleSetClipboard(t_new_clipboard) == S_OK;

	// If everything succeeded, then update the internal reference to the
	// clipboard. Note that m_clipboard only ever retains a copy of the
	// clipboard data object if we own it.
	if (t_success)
	{
		if (m_clipboard != NULL)
		{
			m_clipboard -> Release();
			m_clipboard = NULL;
		}

		m_clipboard = t_new_clipboard;
	}
	else
	{
		if (t_new_clipboard != NULL)
			t_new_clipboard -> Release();
	}

	return t_success;
}

MCPasteboard *MCScreenDC::getclipboard(void)
{
	// Ask OLE for a data object representing the system clipboard
	IDataObject *t_clipboard;
	if (OleGetClipboard(&t_clipboard) != S_OK)
		return NULL;

	// Wrap the OLE data object with our pasteboard abstraction.
	// Note that the constructore for MCWindowsPasteboard copies the passed
	// data object, so we must release our local reference to ensure that
	// the IDataObject is released when the pasteboard is released.
	MCPasteboard *t_pasteboard;
	t_pasteboard = new MCWindowsPasteboard(t_clipboard);

	t_clipboard -> Release();

	return t_pasteboard;
}
