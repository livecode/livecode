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

extern MCPlatformColorTransformRef MCMacPlatformCreateColorTransform(const MCColorSpaceInfo& p_info);

////////////////////////////////////////////////////////////////////////////////

MCPlatformColorTransform::MCPlatformColorTransform(const MCColorSpaceInfo& p_info)
: m_references(1)
{
}

MCPlatformColorTransform::~MCPlatformColorTransform(void)
{
}

void MCPlatformColorTransform::Retain(void)
{
    m_references += 1;
}

void MCPlatformColorTransform::Release(void)
{
    m_references -= 1;
    if (m_references == 0)
    {
        delete this;
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCreateColorTransform(const MCColorSpaceInfo& p_info, MCPlatformColorTransformRef& r_transform)
{
    MCPlatformColorTransformRef t_colorxform = MCMacPlatformCreateColorTransform(p_info);
    
    r_transform = t_colorxform;
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

bool MCPlatformInitializeColorTransform(void)
{
    return true;
}

void MCPlatformFinalizeColorTransform(void)
{
}

////////////////////////////////////////////////////////////////////////////////
