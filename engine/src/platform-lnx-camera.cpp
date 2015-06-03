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

#include "platform.h"

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCameraCreate(MCPlatformCameraRef& r_camera)
{
	r_camera = nil;
}

void MCPlatformCameraRetain(MCPlatformCameraRef camera)
{
}

void MCPlatformCameraRelease(MCPlatformCameraRef camera)
{
}

void MCPlatformCameraAttach(MCPlatformCameraRef camera, void *target)
{
}

void MCPlatformCameraDetach(MCPlatformCameraRef camera)
{
}

void MCPlatformCameraOpen(MCPlatformCameraRef camera)
{
}

void MCPlatformCameraClose(MCPlatformCameraRef camera)
{
}

bool MCPlatformCameraGetNativeView(MCPlatformCameraRef camera, void *&r_view)
{
	return false;
}

bool MCPlatformCameraSetProperty(MCPlatformCameraRef camera, MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value)
{
	return false;
}

bool MCPlatformCameraGetProperty(MCPlatformCameraRef sound, MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value)
{
	return false;
}

bool MCPlatformCameraStartRecording(MCPlatformCameraRef camera, MCStringRef filename)
{
	return false;
}

bool MCPlatformCameraStopRecording(MCPlatformCameraRef camera)
{
	return false;
}

bool MCPlatformCameraTakePicture(MCPlatformCameraRef camera, MCDataRef& r_image_data)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////
