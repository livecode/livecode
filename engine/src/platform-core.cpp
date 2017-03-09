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

bool MCPlatformWaitForEvent(double p_duration, bool p_blocking)
{
    return MCplatform -> WaitForEvent(p_duration, p_blocking);
}

void MCPlatformBreakWait(void)
{
    MCplatform -> BreakWait();
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformScreenSnapshotOfUserArea(MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
    MCplatform -> ScreenSnapshotOfUserArea(p_size, r_bitmap);
}

void MCPlatformScreenSnapshot(MCRectangle p_area, MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
    MCplatform -> ScreenSnapshot(p_area, p_size, r_bitmap);
}

void MCPlatformScreenSnapshotOfWindow(uint32_t window_id, MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
    MCplatform -> ScreenSnapshotOfWindow(window_id, p_size, r_bitmap);
}

void MCPlatformScreenSnapshotOfWindowArea(uint32_t window_id, MCRectangle p_area, MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
    MCplatform -> ScreenSnapshotOfWindowArea(window_id, p_area, p_size, r_bitmap);
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformBeep(void)
{
    MCplatform -> Beep();
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformFlushEvents(MCPlatformEventMask p_mask)
{
    MCplatform -> FlushEvents(p_mask);
}

uint32_t MCPlatformGetEventTime(void)
{
    return MCplatform -> GetEventTime();
}

////////////////////////////////////////////////////////////////////////////////

bool MCPlatformGetMouseButtonState(uindex_t button)
{
    return MCplatform -> GetMouseButtonState(button);
}

bool MCPlatformGetKeyState(MCPlatformKeyCode*& r_codes, uindex_t& r_code_count)
{
    return MCplatform -> GetKeyState(r_codes, r_code_count);
}

MCPlatformModifiers MCPlatformGetModifiersState(void)
{
    return MCplatform -> GetModifiersState();
}

bool MCPlatformGetMouseClick(uindex_t button, MCPoint& r_location)
{
    return MCplatform -> GetMouseClick(button, r_location);
}

void MCPlatformGetMousePosition(MCPoint& r_location)
{
    MCplatform -> GetMousePosition(r_location);
}

void MCPlatformSetMousePosition(MCPoint location)
{
    MCplatform -> SetMousePosition(location);
}

void MCPlatformGetWindowAtPoint(MCPoint location, MCPlatformWindowRef& r_window)
{
    MCplatform -> GetWindowAtPoint(location, r_window);
}

void MCPlatformGrabPointer(MCPlatformWindowRef p_window)
{
    MCplatform -> GrabPointer(p_window);
}

void MCPlatformUngrabPointer(void)
{
    MCplatform -> UngrabPointer();
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformDoDragDrop(MCPlatformWindowRef p_window, MCPlatformAllowedDragOperations p_allowed_operations, MCImageBitmap *p_image, const MCPoint *p_image_loc, MCPlatformDragOperation& r_operation)
{
    MCplatform -> DoDragDrop(p_window, p_allowed_operations, p_image, p_image_loc, r_operation);
}

////////////////////////////////////////////////////////////////////////////////

int platform_main(int argc, char *argv[], char *envp[])
{
    if (MCPlatformInitialize())
        return MCplatform -> Run(argc, argv, envp);
    
    return -1;
}
