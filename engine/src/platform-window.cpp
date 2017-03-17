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

#include "platform.h"
#include "platform-internal.h"
#include "mac-extern.h"

#include "region.h"

#include "graphics_util.h"

#include "globals.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Platform Window Procedural Wrappers
//

void MCPlatformCreateWindow(MCPlatformWindowRef& r_window)
{
    r_window = MCplatform -> CreateWindow();
}

void MCPlatformRetainWindow(MCPlatformWindowRef p_window)
{
	p_window -> Retain();
}

void MCPlatformReleaseWindow(MCPlatformWindowRef p_window)
{
	p_window -> Release();
}

void MCPlatformUpdateWindow(MCPlatformWindowRef p_window)
{
	p_window -> Update();
}

void MCPlatformInvalidateWindow(MCPlatformWindowRef p_window, MCGRegionRef p_region)
{
	p_window -> Invalidate(p_region);
}

void MCPlatformShowWindow(MCPlatformWindowRef p_window)
{
	p_window -> Show();
}

void MCPlatformShowWindowAsSheet(MCPlatformWindowRef p_window, MCPlatformWindowRef p_parent_window)
{
	p_window -> ShowAsSheet(p_parent_window);
}

void MCPlatformHideWindow(MCPlatformWindowRef p_window)
{
	p_window -> Hide();
}

void MCPlatformFocusWindow(MCPlatformWindowRef p_window)
{
	p_window -> Focus();
}

void MCPlatformRaiseWindow(MCPlatformWindowRef p_window)
{
	p_window -> Raise();
}

void MCPlatformIconifyWindow(MCPlatformWindowRef p_window)
{
	p_window -> Iconify();
}

void MCPlatformUniconifyWindow(MCPlatformWindowRef p_window)
{
	p_window -> Uniconify();
}

void MCPlatformConfigureTextInputInWindow(MCPlatformWindowRef p_window, bool p_activate)
{
	p_window -> ConfigureTextInput(p_activate);
}

void MCPlatformResetTextInputInWindow(MCPlatformWindowRef p_window)
{
	p_window -> ResetTextInput();
}

bool MCPlatformIsWindowVisible(MCPlatformWindowRef p_window)
{
	return p_window -> IsVisible();
}

//////////

void MCPlatformMapPointFromScreenToWindow(MCPlatformWindowRef p_window, MCPoint p_screen_point, MCPoint& r_window_point)
{
	p_window -> MapPointFromScreenToWindow(p_screen_point, r_window_point);
}

void MCPlatformMapPointFromWindowToScreen(MCPlatformWindowRef p_window, MCPoint p_window_point, MCPoint& r_screen_point)
{
	p_window -> MapPointFromWindowToScreen(p_window_point, r_screen_point);
}

//////////

void MCPlatformSetWindowProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, const void *p_value)
{
	p_window -> SetProperty(p_property, p_type, p_value);
}

void MCPlatformGetWindowProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	p_window -> GetProperty(p_property, p_type, r_value);
}

void MCPlatformSetWindowContentRect(MCPlatformWindowRef p_window, MCRectangle p_content_rect)
{
	p_window -> SetProperty(kMCPlatformWindowPropertyContentRect, kMCPlatformPropertyTypeRectangle, &p_content_rect);
}

void MCPlatformGetWindowContentRect(MCPlatformWindowRef p_window, MCRectangle& r_content_rect)
{
	p_window -> GetProperty(kMCPlatformWindowPropertyContentRect, kMCPlatformPropertyTypeRectangle, &r_content_rect);
}

void MCPlatformSetWindowFrameRect(MCPlatformWindowRef p_window, MCRectangle p_frame_rect)
{
	p_window -> SetProperty(kMCPlatformWindowPropertyFrameRect, kMCPlatformPropertyTypeRectangle, &p_frame_rect);
}

void MCPlatformGetWindowFrameRect(MCPlatformWindowRef p_window, MCRectangle& r_frame_rect)
{
	p_window -> GetProperty(kMCPlatformWindowPropertyFrameRect, kMCPlatformPropertyTypeRectangle, &r_frame_rect);
}

void MCPlatformSetWindowBoolProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, bool p_value)
{
	p_window -> SetProperty(p_property, kMCPlatformPropertyTypeBool, &p_value);
}

void MCPlatformSetWindowFloatProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, float p_value)
{
	p_window -> SetProperty(p_property, kMCPlatformPropertyTypeFloat, &p_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformSwitchFocusToView(MCPlatformWindowRef p_window, uint32_t p_id)
{
    p_window -> SwitchFocusToView(p_id);
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformWindowDeathGrip(MCPlatformWindowRef p_window)
{
    MCplatform -> DeathGrip(p_window);
}

bool MCPlatformGetWindowWithId(uint32_t p_id, MCPlatformWindowRef& r_window)
{
    return MCplatform -> GetWindowWithId(p_id, r_window);
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformWindowMaskCreateWithAlphaAndRelease(int32_t width, int32_t height, int32_t stride, void *bits, MCPlatformWindowMaskRef& r_mask)
{
    MCPlatformWindowMaskRef t_mask = MCplatform -> CreateWindowMask();
    if (t_mask && !t_mask->CreateWithAlphaAndRelease(width, height, stride, bits))
    {
        t_mask -> Release();
        t_mask = nil;
    }
    r_mask = t_mask;
}

void MCPlatformWindowMaskRetain(MCPlatformWindowMaskRef p_mask)
{
    p_mask->Retain();
}

void MCPlatformWindowMaskRelease(MCPlatformWindowMaskRef p_mask)
{
    p_mask->Release();
}

////////////////////////////////////////////////////////////////////////////////



