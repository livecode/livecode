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

@implementation com_runrev_livecode_MCPrintDialogDelegate

- (void)printDialogDidEnd:(id)printDialog returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
    MCMacPlatformPrintDialogSession *t_session;
    t_session = (MCMacPlatformPrintDialogSession *)contextInfo;
    
    if (returnCode == NSOKButton)
        t_session -> SetResult(kMCPlatformPrintDialogResultSuccess);
    else
        t_session -> SetResult(kMCPlatformPrintDialogResultCancel);
}

@end

////////////////////////////////////////////////////////////////////////////////


MCMacPlatformPrintDialogSession::MCMacPlatformPrintDialogSession()
: m_result(kMCPlatformPrintDialogResultContinue)
, m_owner(nil)
, m_info(nil)
, m_session(nil)
, m_settings(nil)
, m_page_format(nil)
{
    
}

MCMacPlatformPrintDialogSession::~MCMacPlatformPrintDialogSession(void)
{
    if (m_owner != nil)
        m_owner -> Release();
    
    if (m_info != nil)
        [m_info release];
    
    if (m_session != nil)
        PMRelease(m_session);
    
    if (m_settings != nil)
        PMRelease(m_settings);
    
    if (m_page_format != nil)
        PMRelease(m_page_format);
}

void MCMacPlatformPrintDialogSession::CopyInfo(void *&x_session, void *&x_settings, void *&x_page_format)
{
    PMCopyPrintSettings((PMPrintSettings)[m_info PMPrintSettings], m_settings);
    PMCopyPageFormat((PMPageFormat)[m_info PMPageFormat], m_page_format);
    
    PMDestinationType t_type;
    
    CFURLRef t_output_location_url;
    CFStringRef t_output_format;
    t_output_location_url = NULL;
    t_output_format = NULL;
    if (PMSessionGetDestinationType((PMPrintSession)[m_info PMPrintSession], (PMPrintSettings)[m_info PMPrintSettings], &t_type) == noErr &&
        PMSessionCopyDestinationLocation((PMPrintSession)[m_info PMPrintSession], (PMPrintSettings)[m_info PMPrintSettings], &t_output_location_url) == noErr &&
        PMSessionCopyDestinationFormat((PMPrintSession)[m_info PMPrintSession], (PMPrintSettings)[m_info PMPrintSettings], &t_output_format) == noErr)
        PMSessionSetDestination(m_session, m_settings, t_type, t_output_format, t_output_location_url);
    if (t_output_format != NULL)
        CFRelease(t_output_format);
    if (t_output_location_url != NULL)
        CFRelease(t_output_location_url);
    
    PMPrinter t_printer;
    PMSessionGetCurrentPrinter((PMPrintSession)[m_info PMPrintSession], &t_printer);
    PMSessionSetCurrentPMPrinter(m_session, t_printer);
    
    PMRelease(x_session);
    x_session = m_session;
    PMRetain(x_session);
    
    PMRelease(x_settings);
    x_settings = m_settings;
    PMRetain(x_settings);
    
    PMRelease(x_page_format);
    x_page_format = m_page_format;
    PMRetain(x_page_format);
    
}

void MCMacPlatformPrintDialogSession::BeginSettings(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format)
{
    Begin(p_window, p_session, p_settings, p_page_format, [NSPrintPanel printPanel]);
}

void MCMacPlatformPrintDialogSession::BeginPageSetup(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format)
{
    Begin(p_window, p_session, p_settings, p_page_format, [NSPageLayout pageLayout]);
}


void MCMacPlatformPrintDialogSession::Begin(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format, id p_panel)
{
    m_session = (PMPrintSession) p_session;
    PMRetain(m_session);
    
    m_settings = (PMPrintSettings) p_settings;
    PMRetain(m_settings);
    
    m_page_format = (PMPageFormat) p_page_format;
    PMRetain(m_page_format);
    
    NSPrintInfo *t_info;
    
    PMPrinter t_printer;
    PMSessionGetCurrentPrinter(m_session, &t_printer);
    
    t_info = [[NSPrintInfo alloc] initWithDictionary: [NSDictionary dictionary]];
    [t_info setPrinter: [NSPrinter printerWithName: (NSString *)PMPrinterGetName(t_printer)]];
    
    PMCopyPrintSettings(m_settings, (PMPrintSettings)[t_info PMPrintSettings]);
    PMCopyPageFormat(m_page_format, (PMPageFormat)[t_info PMPageFormat]);
    
    [t_info updateFromPMPrintSettings];
    [t_info updateFromPMPageFormat];
    
    
    MCPrintDialogDelegate * t_delegate = [[[MCPrintDialogDelegate alloc] init] autorelease];
    
    m_info = t_info;
    
    bool t_should_sheet;
    t_should_sheet = p_window != nil && [((MCMacPlatformWindow *)p_window) -> GetHandle() attachedSheet] == nil;
    
    if (!t_should_sheet)
    {
        if ([p_panel runModalWithPrintInfo: t_info] == NSOKButton)
            m_result = kMCPlatformPrintDialogResultSuccess;
        else
            m_result = kMCPlatformPrintDialogResultCancel;
    }
    else
    {
        m_owner = p_window;
        m_owner -> Retain();
        
        [p_panel beginSheetWithPrintInfo:t_info
                           modalForWindow:((MCMacPlatformWindow *)m_owner) -> GetHandle()
                                 delegate:t_delegate
                           didEndSelector:@selector(printDialogDidEnd:returnCode:contextInfo:)
                              contextInfo:this];
    }
}

void MCMacPlatformPrintDialogSession::SetResult(MCPlatformPrintDialogResult p_result)
{
    m_result = p_result;
}

MCPlatformPrintDialogResult MCMacPlatformPrintDialogSession::GetResult(void)
{
    return m_result;
}

////////////////////////////////////////////////////////////////////////////////

MCPlatform::Ref<MCPlatformPrintDialogSession> MCMacPlatformCreatePrintDialogSession()
{
    return MCPlatform::makeRef<MCMacPlatformPrintDialogSession>();
}
