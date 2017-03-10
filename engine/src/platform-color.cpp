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

void MCPlatformCreateColorTransform(const MCColorSpaceInfo& p_info, MCPlatformColorTransformRef& r_transform)
{
    MCPlatformColorTransformRef t_transform = MCplatform -> CreateColorTransform();
    if (t_transform &&
        t_transform->CreateWithColorSpace(p_info))
    {
        r_transform = t_transform;
    }
    else
    {
        t_transform -> Release();
        r_transform = nullptr;
    }
}

void MCPlatformRetainColorTransform(MCPlatformColorTransformRef p_transform)
{
    p_transform->Retain();
}

void MCPlatformReleaseColorTransform(MCPlatformColorTransformRef p_transform)
{
    p_transform->Release();
}

bool MCPlatformApplyColorTransform(MCPlatformColorTransformRef p_transform, MCImageBitmap *p_image)
{
    if (p_transform == nil)
        return false;
    
    return p_transform->Apply(p_image);
}

////////////////////////////////////////////////////////////////////////////////
