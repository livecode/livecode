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
#include "globals.h"

////////////////////////////////////////////////////////////////////////////////

bool MCPlatformInitialize(void)
{
    MCPlatform::CoreRef t_platform = MCMacPlatformCreateCore();
    MCplatform = t_platform.unsafeTake();
    
    return true;
}

void MCPlatformFinalize(void)
{
    MCplatform -> Release();
}

bool MCPlatformGetAbortKeyPressed(void)
{
    return MCplatform -> GetAbortKeyPressed();
}

void MCPlatformShowMessageDialog(MCStringRef p_title, MCStringRef p_message)
{
    MCplatform -> ShowMessageDialog(p_title, p_message);
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformBeginColorDialog(MCStringRef p_title, const MCColor& p_color)
{
    return MCplatform -> BeginColorDialog(p_title, p_color);
}

MCPlatformDialogResult MCPlatformEndColorDialog(MCColor& r_new_color)
{
    return MCplatform -> EndColorDialog(r_new_color);
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformGetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
    MCplatform -> GetSystemProperty(p_property, p_type, r_value);
}

void MCPlatformSetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
    MCplatform -> SetSystemProperty(p_property, p_type, p_value);
}

////////////////////////////////////////////////////////////////////////////////

int platform_main(int argc, char *argv[], char *envp[])
{
    if (MCPlatformInitialize())
        return MCplatform -> Run(argc, argv, envp);
    
    return -1;
}
