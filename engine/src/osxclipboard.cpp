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

#include "dispatch.h"
#include "util.h"
//#include "execpt.h"

#include "globals.h"

#include "mctheme.h"
#include "context.h"

#include "osxdc.h"
#include "osxtransfer.h"

///////////////////////////////////////////////////////////////////////////////

MCMacOSXScrapPasteboard::MCMacOSXScrapPasteboard(ScrapRef p_scrap)
{
	m_scrap = p_scrap;
	Resolve();
}

bool MCMacOSXScrapPasteboard::QueryFlavors(ScrapFlavorType*& r_types, uint4& r_type_count)
{
	bool t_success;
	t_success = true;

	UInt32 t_count;
	t_count = 0;
	if (t_success)
		if (GetScrapFlavorCount(m_scrap, &t_count) != noErr)
			t_success = false;

	ScrapFlavorInfo *t_info_array;
	t_info_array = NULL;
	if (t_success)
	{
		t_info_array = new ScrapFlavorInfo[t_count];
		if (t_info_array == NULL)
			t_success = false;
	}

	if (t_success)
		if (GetScrapFlavorInfoList(m_scrap, &t_count, t_info_array) != noErr)
			t_success = false;

	ScrapFlavorType *t_types;
	t_types = NULL;
	if (t_success)
	{
		t_types = new ScrapFlavorType[t_count];
		if (t_types == NULL)
			t_success = false;
	}

	if (t_success)
	{
		for(uint4 i = 0; i < t_count; ++i)
			t_types[i] = t_info_array[i] . flavorType;
			
		r_types = t_types;
		r_type_count = t_count;
	}
	else
		delete t_types;

	delete t_info_array;

	return t_success;
}

bool MCMacOSXScrapPasteboard::FetchFlavor(ScrapFlavorType p_type, MCDataRef& r_data)
{
	bool t_success;
	t_success = true;

	Size t_size;
	t_size = 0;
	if (t_success)
		if (GetScrapFlavorSize(m_scrap, p_type, &t_size) != noErr)
			t_success = false;

	void *t_buffer;
	t_buffer = NULL;
	if (t_success)
	{
		t_buffer = malloc(t_size);
		if (t_buffer == NULL)
			t_success = false;
	}

	if (t_success)
		if (GetScrapFlavorData(m_scrap, p_type, &t_size, t_buffer) != noErr)
			t_success = false;

	if (t_success)
		t_success = MCDataCreateWithBytesAndRelease((char_t *)t_buffer, t_size, r_data);
	
	if (!t_success)
	{
		if (t_buffer != NULL)
			free(t_buffer);
	}

	return t_success;
}

///////////////////////////////////////////////////////////////////////////////

void MCScreenDC::flushclipboard(void)
{
	// If we own the clipboard, then make sure we have provided all our flavors.
	if (ownsclipboard())
		CallInScrapPromises();
}

bool MCScreenDC::ownsclipboard(void)
{
	// If m_current_scrap is NULL, then we haven't set the clipboard ourselves
	if (m_current_scrap == NULL)
		return false;

	// Fetch the current scrap references
	ScrapRef t_new_scrap;
	if (GetCurrentScrap(&t_new_scrap) != noErr)
		t_new_scrap = NULL;

	// If its changed, then we no longer own the clipboard so reset our
	// internal data
	if (t_new_scrap != m_current_scrap)
	{
		m_current_scrap = NULL;
		if (m_current_scrap_data != NULL)
		{
			delete m_current_scrap_data;
			m_current_scrap_data = NULL;
		}

		return false;
	}

	return true;
}

static bool PublishScrapFlavor(MCMacOSXTransferData *p_data, ScrapFlavorType p_type, void *p_context)
{
	if (PutScrapFlavor((ScrapRef)p_context, p_type, kScrapFlavorMaskNone, kScrapFlavorSizeUnknown, NULL) != noErr)
		return false;

	return true;
}

static OSStatus SubscribeScrapFlavor(ScrapRef p_scrap, ScrapFlavorType p_type, void *p_context)
{
	MCMacOSXTransferData *t_data;
	t_data = (MCMacOSXTransferData *)p_context;

	OSStatus t_status;
	t_status = noErr;

	MCAutoDataRef t_string;
	if (!t_data -> Subscribe(p_type, &t_string))
		t_status = internalScrapErr;

	if (t_status == noErr)
		t_status = PutScrapFlavor(p_scrap, p_type, kScrapFlavorMaskNone, MCDataGetLength(*t_string), MCDataGetBytePtr(*t_string));

	return t_status;
}

bool MCScreenDC::setclipboard(MCPasteboard *p_pasteboard)
{
	// First we try and clear the current scrap
	if (ClearCurrentScrap() == noErr)
		m_current_scrap = NULL;
	else
		return false;

	// Free current clipboard data if any
	if (m_current_scrap_data != NULL)
	{
		delete m_current_scrap_data;
		m_current_scrap_data = NULL;
	}

	// If the pasteboard is NULL then we are just clearing the clipboard
	if (p_pasteboard == NULL)
		return true;


	bool t_success;
	t_success = true;

	// Fetch the new scrap reference
	ScrapRef t_new_scrap;
	t_new_scrap = NULL;
	if (GetCurrentScrap(&t_new_scrap) != noErr)
		t_success = false;

	// Create the new scrap data object
	MCMacOSXTransferData *t_new_scrap_data;
	t_new_scrap_data = NULL;
	if (t_success)
	{
		t_new_scrap_data = new MCMacOSXTransferData;
		if (t_new_scrap_data == NULL)
			t_success = false;
	}

	// Publish the pasteboard, this will construct a list of flavor
	// types and converters inside the scrap data.
	if (t_success)
		t_success = t_new_scrap_data -> Publish(p_pasteboard);

	// Iterate over each scrap flavor and put the item onto the scrap
	// using the promise callback mechanism.
	if (t_success)
		t_success = t_new_scrap_data -> ForEachFlavor(PublishScrapFlavor, t_new_scrap);

	// Make sure we set the promise keeper callback - this involves
	// creating a UPP if one hasn't already been made
	if (t_success && m_scrap_promise_keeper_upp == NULL)
	{
		m_scrap_promise_keeper_upp = NewScrapPromiseKeeperUPP(SubscribeScrapFlavor);
		if (m_scrap_promise_keeper_upp == NULL)
			t_success = false;
	}

	if (t_success)
		if (SetScrapPromiseKeeper(t_new_scrap, m_scrap_promise_keeper_upp, t_new_scrap_data) != noErr)
			t_success = false;

	// If we succeeded then update internal state
	if (t_success)
	{
		m_current_scrap = t_new_scrap;
		m_current_scrap_data = t_new_scrap_data;
	}
	else
	{
		ClearCurrentScrap();
		delete t_new_scrap_data;
	}

	return t_success;
}

MCPasteboard *MCScreenDC::getclipboard(void)
{
	ScrapRef t_scrap;
	if (GetCurrentScrap(&t_scrap) != noErr)
		return NULL;

	MCPasteboard *t_pasteboard;
	t_pasteboard = new MCMacOSXScrapPasteboard(t_scrap);
	
	return t_pasteboard;
}
