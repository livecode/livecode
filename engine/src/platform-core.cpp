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

////////////////////////////////////////////////////////////////////////////////

int platform_main(int argc, char *argv[], char *envp[])
{
    if (MCPlatformInitialize())
        return MCplatform -> Run(argc, argv, envp);
    
    return -1;
}
