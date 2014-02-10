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

#include <Cocoa/Cocoa.h>

#include "core.h"
#include "globdefs.h"
#include "uidc.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

static MCPlatformPrintDialogResult s_dialog_result;

static void MCPlatformStartPrintInfo(void *p_session, void *p_settings, void *p_page_format, NSPrintInfo*& r_info)
{
	PMPrintSession t_session;
	PMPrintSettings t_settings;
	PMPageFormat t_page_format;
	t_session = (PMPrintSession)p_session;
	t_settings = (PMPrintSettings)p_settings;
	t_page_format = (PMPageFormat)p_page_format;
	
	PMPrinter t_printer;
	PMSessionGetCurrentPrinter(t_session, &t_printer);
	
	NSPrintInfo *t_info;
	t_info = [[NSPrintInfo alloc] initWithDictionary: [NSDictionary dictionary]];
	[t_info setPrinter: [NSPrinter printerWithName: (NSString *)PMPrinterGetName(t_printer)]];
	
	PMCopyPrintSettings(t_settings, (PMPrintSettings)[t_info PMPrintSettings]);
	PMCopyPageFormat(t_page_format, (PMPageFormat)[t_info PMPageFormat]);
	
	[t_info updateFromPMPrintSettings];
	[t_info updateFromPMPageFormat];
	
	r_info = t_info;
}

static void MCPlatformEndPrintInfo(NSPrintInfo *p_info, void *p_session, void *p_settings, void *p_page_format)
{
	PMPrintSession t_session;
	PMPrintSettings t_settings;
	PMPageFormat t_page_format;
	t_session = (PMPrintSession)p_session;
	t_settings = (PMPrintSettings)p_settings;
	t_page_format = (PMPageFormat)p_page_format;
	
	PMCopyPrintSettings((PMPrintSettings)[p_info PMPrintSettings], t_settings);
	PMCopyPageFormat((PMPageFormat)[p_info PMPageFormat], t_page_format);
	
	PMDestinationType t_type;
	
	CFURLRef t_output_location_url;
	CFStringRef t_output_format;
	t_output_location_url = NULL;
	t_output_format = NULL;
	if (PMSessionGetDestinationType((PMPrintSession)[p_info PMPrintSession], (PMPrintSettings)[p_info PMPrintSettings], &t_type) == noErr &&
		PMSessionCopyDestinationLocation((PMPrintSession)[p_info PMPrintSession], (PMPrintSettings)[p_info PMPrintSettings], &t_output_location_url) == noErr &&
		PMSessionCopyDestinationFormat((PMPrintSession)[p_info PMPrintSession], (PMPrintSettings)[p_info PMPrintSettings], &t_output_format) == noErr)
		PMSessionSetDestination(t_session, t_settings, t_type, t_output_format, t_output_location_url);
	if (t_output_format != NULL)
		CFRelease(t_output_format);
	if (t_output_location_url != NULL)
		CFRelease(t_output_location_url);
		
	PMPrinter t_printer;
	PMSessionGetCurrentPrinter((PMPrintSession)[p_info PMPrintSession], &t_printer);
	PMSessionSetCurrentPMPrinter(t_session, t_printer);
	
	[p_info release];
}

void MCPlatformBeginPageSetupDialog(MCPlatformWindowRef p_owner, void *p_session, void *p_settings, void *p_page_format)
{
	NSPrintInfo *t_info;
	MCPlatformStartPrintInfo(p_session, p_settings, p_page_format, t_info);
	
	//if (p_owner == nil)
	{
		s_dialog_result = kMCPlatformPrintDialogResultContinue;
		if ([[NSPageLayout pageLayout] runModalWithPrintInfo: t_info] == NSOKButton)
			s_dialog_result = kMCPlatformPrintDialogResultSuccess;
		else
			s_dialog_result = kMCPlatformPrintDialogResultCancel;
	}
	//else
	//{
	//}
	
	MCPlatformEndPrintInfo(t_info, p_session, p_settings, p_page_format);
}

void MCPlatformBeginPrintSettingsDialog(MCPlatformWindowRef p_owner, void *p_session, void *p_settings, void *p_page_format)
{
	NSPrintInfo *t_info;
	MCPlatformStartPrintInfo(p_session, p_settings, p_page_format, t_info);
	
	//if (p_owner == nil)
	{
		s_dialog_result = kMCPlatformPrintDialogResultContinue;
		if ([[NSPrintPanel printPanel] runModalWithPrintInfo: t_info] == NSOKButton)
			s_dialog_result = kMCPlatformPrintDialogResultSuccess;
		else
			s_dialog_result = kMCPlatformPrintDialogResultCancel;
	}
	//else
	//{
	//}
	
	MCPlatformEndPrintInfo(t_info, p_session, p_settings, p_page_format);
}

MCPlatformPrintDialogResult MCPlatformEndPrintDialog(void)
{
	return s_dialog_result;
}

////////////////////////////////////////////////////////////////////////////////
