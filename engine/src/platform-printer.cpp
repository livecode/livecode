/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

#include "platform.h"
#include "platform-internal.h"
#include "mac-extern.h"

////////////////////////////////////////////////////////////////////////////////

void MCPlatformBeginPageSetupDialog(MCPlatformWindowRef p_owner, void *p_session, void *p_settings, void *p_page_format, MCPlatformPrintDialogSessionRef &r_dialog_session)
{
    MCPlatform::PrintDialogSessionRef t_dialog_session = MCMacPlatformCreatePrintDialogSession();
    t_dialog_session -> BeginPageSetup(p_owner, p_session, p_settings, p_page_format);
    
    r_dialog_session = t_dialog_session.unsafeTake();
}

void MCPlatformBeginPrintSettingsDialog(MCPlatformWindowRef p_owner, void *p_session, void *p_settings, void *p_page_format, MCPlatformPrintDialogSessionRef &r_dialog_session)
{
    MCPlatform::PrintDialogSessionRef t_dialog_session = MCMacPlatformCreatePrintDialogSession();
    t_dialog_session -> BeginSettings(p_owner, p_session, p_settings, p_page_format);
    
    r_dialog_session = t_dialog_session.unsafeTake();
}

MCPlatformPrintDialogResult MCPlatformPrintDialogSessionResult(MCPlatformPrintDialogSessionRef p_dialog_session)
{
    return p_dialog_session -> GetResult();
}

void MCPlatformPrintDialogSessionCopyInfo(MCPlatformPrintDialogSessionRef p_dialog_session, void *&x_session, void *&x_settings, void *&x_page_format)
{
    p_dialog_session -> CopyInfo(x_session, x_settings, x_page_format);
}

void MCPlatformPrintDialogSessionRelease(MCPlatformPrintDialogSessionRef p_dialog_session)
{
    p_dialog_session -> Release();
}

////////////////////////////////////////////////////////////////////////////////
