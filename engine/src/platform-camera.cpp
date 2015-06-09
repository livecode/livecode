/* Copyright (C) 2003-2015 Runtime Revolution Ltd.
 
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

#include "prefix.h"

#include "platform.h"
#include "platform-internal.h"
#include "platform-camera-internal.h"

////////////////////////////////////////////////////////////////////////////////

MCPlatformCamera::MCPlatformCamera(void)
{
    m_references = 1;
}

MCPlatformCamera::~MCPlatformCamera(void)
{
}

void MCPlatformCamera::Retain(void)
{
    m_references += 1;
}

void MCPlatformCamera::Release(void)
{
    m_references -= 1;
    if (m_references == 0)
        delete this;
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCameraRetain(MCPlatformCameraRef camera)
{
    if (camera != nil)
        ((MCPlatformCamera*)camera) -> Retain();
}

void MCPlatformCameraRelease(MCPlatformCameraRef camera)
{
    if (camera != nil)
        ((MCPlatformCamera*)camera) -> Release();
}

void MCPlatformCameraAttach(MCPlatformCameraRef camera, void *p_target)
{
    if (camera != nil)
        ((MCPlatformCamera*)camera) -> Attach(p_target);
}

void MCPlatformCameraDetach(MCPlatformCameraRef camera)
{
    if (camera != nil)
        ((MCPlatformCamera*)camera) -> Detach();
}

void MCPlatformCameraOpen(MCPlatformCameraRef camera)
{
    if (camera != nil)
        ((MCPlatformCamera*)camera) -> Open();
}

void MCPlatformCameraClose(MCPlatformCameraRef camera)
{
    if (camera != nil)
        ((MCPlatformCamera*)camera) -> Close();
}

bool MCPlatformCameraGetNativeView(MCPlatformCameraRef camera, void *&r_view)
{
	if (camera != nil)
		return ((MCPlatformCamera*)camera)->GetNativeView(r_view);
	return false;
}

bool MCPlatformCameraSetProperty(MCPlatformCameraRef camera, MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value)
{
    if (camera != nil)
        return ((MCPlatformCamera*)camera) -> SetProperty(property, type, value);
    return true;
}

bool MCPlatformCameraGetProperty(MCPlatformCameraRef camera, MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value)
{
    if (camera != nil)
        return ((MCPlatformCamera*)camera) -> GetProperty(property, type, value);
    return true;
}

bool MCPlatformCameraStartRecording(MCPlatformCameraRef camera, MCStringRef filename)
{
    if (camera != nil)
        return ((MCPlatformCamera*)camera) -> StartRecording(filename);
    return false;
}

bool MCPlatformCameraStopRecording(MCPlatformCameraRef camera)
{
    if (camera != nil)
        return ((MCPlatformCamera*)camera) -> StopRecording();
    return false;
}

bool MCPlatformCameraTakePicture(MCPlatformCameraRef camera, MCDataRef& r_image_data)
{
    if (camera != nil)
        return ((MCPlatformCamera*)camera) -> TakePicture(r_image_data);
    return false;
}

////////////////////////////////////////////////////////////////////////////////
