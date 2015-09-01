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

#include <Cocoa/Cocoa.h>

#include "globdefs.h"
#include "uidc.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCPrintDialogDelegate: NSObject

- (void)printDialogDidEnd:(id)printDialog returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo;

@end

@compatibility_alias MCPrintDialogDelegate com_runrev_livecode_MCPrintDialogDelegate;

////////////////////////////////////////////////////////////////////////////////

struct MCMacPlatformPrintDialogNest
{
	MCMacPlatformPrintDialogNest *next;
	MCPlatformPrintDialogResult result;
	MCPlatformWindowRef owner;
    
    // MW-2014-07-29: [[ Bug 12961 ]] Make sure we store the objects we need to
    //   apply settings as sheets end after the end of BeginDialog.
    NSPrintInfo *info;
    void *session;
    void *settings;
    void *page_format;
};

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCPrintDialogDelegate

- (void)printDialogDidEnd:(id)printDialog returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
	MCMacPlatformPrintDialogNest *t_nest;
	t_nest = (MCMacPlatformPrintDialogNest *)contextInfo;
	
	if (returnCode == NSOKButton)
		t_nest -> result = kMCPlatformPrintDialogResultSuccess;
	else
		t_nest -> result = kMCPlatformPrintDialogResultCancel;
}

@end

////////////////////////////////////////////////////////////////////////////////

static MCMacPlatformPrintDialogNest *s_print_dialog_nesting = nil;
static MCPrintDialogDelegate *s_print_dialog_delegate = nil;

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

static void MCPlatformBeginPrintDialog(MCPlatformWindowRef p_owner, void *p_session, void *p_settings, void *p_page_format, id p_dialog)
{
	NSPrintInfo *t_info;
	MCPlatformStartPrintInfo(p_session, p_settings, p_page_format, t_info);
	
	if (s_print_dialog_delegate == nil)
		s_print_dialog_delegate = [[MCPrintDialogDelegate alloc] init];
	
	MCMacPlatformPrintDialogNest *t_nest;
	/* UNCHECKED */ MCMemoryNew(t_nest);
    
    t_nest -> info = t_info;
    t_nest -> session = p_session;
    t_nest -> settings = p_settings;
    t_nest -> page_format = p_page_format;
    
	t_nest -> next = s_print_dialog_nesting;
	t_nest -> result = kMCPlatformPrintDialogResultContinue;
	s_print_dialog_nesting = t_nest;
	
	if (p_owner == nil)
	{
		t_nest -> owner = nil;
		
		if ([p_dialog runModalWithPrintInfo: t_info] == NSOKButton)
			t_nest -> result = kMCPlatformPrintDialogResultSuccess;
		else
			t_nest -> result = kMCPlatformPrintDialogResultCancel;
	}
	else
	{
		// If we've already tried to sheet a dialog against the given window, then
		// we can't do so again, so we implicitly 'cancel' it.
		
		bool t_exists;
		t_exists = false;
		for(MCMacPlatformPrintDialogNest *t_other_nest = t_nest; t_other_nest != nil; t_other_nest = t_other_nest -> next)
			if (t_other_nest -> owner == p_owner)
				t_exists = true;
		
		if (!t_exists)
		{
			t_nest -> owner = p_owner;
			MCPlatformRetainWindow(p_owner);
		
			[p_dialog beginSheetWithPrintInfo:t_info
							   modalForWindow:((MCMacPlatformWindow *)p_owner) -> GetHandle()
									 delegate:s_print_dialog_delegate
							   didEndSelector:@selector(printDialogDidEnd:returnCode:contextInfo:)
								  contextInfo:t_nest];
		}
		else
			t_nest -> result = kMCPlatformPrintDialogResultCancel;
	}
}

void MCPlatformBeginPageSetupDialog(MCPlatformWindowRef p_owner, void *p_session, void *p_settings, void *p_page_format)
{
	MCPlatformBeginPrintDialog(p_owner, p_session, p_settings, p_page_format, [NSPageLayout pageLayout]);
}

void MCPlatformBeginPrintSettingsDialog(MCPlatformWindowRef p_owner, void *p_session, void *p_settings, void *p_page_format)
{
	MCPlatformBeginPrintDialog(p_owner, p_session, p_settings, p_page_format, [NSPrintPanel printPanel]);
}

MCPlatformPrintDialogResult MCPlatformEndPrintDialog(void)
{
	MCMacPlatformPrintDialogNest *t_nest;
	t_nest = s_print_dialog_nesting;

	if (t_nest -> result == kMCPlatformPrintDialogResultContinue)
		return kMCPlatformPrintDialogResultContinue;

	s_print_dialog_nesting = s_print_dialog_nesting -> next;
	
	MCPlatformPrintDialogResult t_result;
	t_result = t_nest -> result;
	
	if (t_nest -> owner != nil)
		MCPlatformReleaseWindow(t_nest -> owner);
	
    // MW-2014-07-29: [[ Bug 12961 ]] Write back the print settings.
	MCPlatformEndPrintInfo(t_nest -> info, t_nest -> session, t_nest -> settings, t_nest -> page_format);
    
	MCMemoryDelete(t_nest);
	
	return t_result;
}

////////////////////////////////////////////////////////////////////////////////
