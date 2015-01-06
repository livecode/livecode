/* Copyright (C) 2014 Runtime Revolution Ltd.
 
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

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcstring.h"
#include "globals.h"
#include "mctheme.h"
#include "util.h"
#include "object.h"
#include "stack.h"
#include "font.h"

#include <windows.h>


bool MCPlatformGetControlThemePropBool(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, bool&)
{
    return false;
}

bool MCPlatformGetControlThemePropInteger(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_prop, int& r_int)
{
    bool t_found;
    t_found = false;
    
    switch (p_prop)
    {
        case kMCPlatformThemePropertyTextSize:
            t_found = true;
            r_int = MCmajorosversion >= 0x0600 ? 12 : 11;
            break;
    }
    
    return t_found;
}

bool MCPlatformGetControlThemePropColor(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_prop, MCColor& r_color)
{
    bool t_found;
    t_found = false;
    
    int t_color;
    
    switch (p_prop)
    {
        case kMCPlatformThemePropertyTextColor:
        {
            t_found = true;
            if (p_state & kMCPlatformControlStateDisabled)
            {
                t_color = COLOR_GRAYTEXT;
            }
            else if (p_state & kMCPlatformControlStateSelected)
            {
                t_color = COLOR_HIGHLIGHTTEXT;
            }
            else
            {
                switch (p_type)
                {
                    case kMCPlatformControlTypeMenu:
                    case kMCPlatformControlTypeMenuItem:
                    case kMCPlatformControlTypeOptionMenu:
                    case kMCPlatformControlTypePulldownMenu:
                    case kMCPlatformControlTypeComboBox:
                        t_color = COLOR_MENUTEXT;
                        break;
                        
                    case kMCPlatformControlTypeButton:
                        t_color = COLOR_BTNTEXT;
                        break;
                        
                    default:
                        t_color = COLOR_WINDOWTEXT;
                        break;
                }
            }
            
            break;
        }
            
        case kMCPlatformThemePropertyBackgroundColor:
        {
            t_found = true;
            if (p_state & kMCPlatformControlStateSelected)
            {
                switch (p_type)
                {
                    case kMCPlatformControlTypeMenu:
                    case kMCPlatformControlTypeMenuItem:
                    case kMCPlatformControlTypeOptionMenu:
                    case kMCPlatformControlTypePulldownMenu:
                    case kMCPlatformControlTypeComboBox:
                        t_color = COLOR_MENUHILIGHT;
                        break;
                        
                    default:
                        t_color = COLOR_HIGHLIGHT;
                        break;
                }
            }
            else
            {
                switch (p_type)
                {
                    case kMCPlatformControlTypeInputField:
                    case kMCPlatformControlTypeList:
                    case kMCPlatformControlTypeComboBox:
                    case kMCPlatformControlTypeOptionMenu:
                        // Doesn't seem to have a colour index - use white
                        r_color.red = r_color.green = r_color.blue = 65535;
                        MCscreen->alloccolor(r_color);
                        return true;
                        
                    case kMCPlatformControlTypeMenuItem:
                        t_color = COLOR_MENU;
                        break;
                        
                    case kMCPlatformControlTypeWindow:
                        // Use the control colour instead of the window colour
                        //t_color = COLOR_WINDOW;
                        //break;
                        
                    default:
                        // Message boxes are this colour instead of the window colour
                        t_color = COLOR_3DFACE;
                        break;
                }
            }
            
            break;
        }
            
        case kMCPlatformThemePropertyShadowColor:
            break;
            
        case kMCPlatformThemePropertyBorderColor:
            break;
            
        case kMCPlatformThemePropertyFocusColor:
            break;
            
        case kMCPlatformThemePropertyTopEdgeColor:
        case kMCPlatformThemePropertyLeftEdgeColor:
            t_found = true;
            t_color = COLOR_3DHILIGHT;
            break;
            
        case kMCPlatformThemePropertyBottomEdgeColor:
        case kMCPlatformThemePropertyRightEdgeColor:
            t_found = true;
            t_color = COLOR_3DSHADOW;
            break;
    }
    
    if (t_found)
    {
        // Colour << 8 + Colour == Colour*257
        DWORD t_colorref;
        t_colorref = GetSysColor(t_color);
        r_color.red = 257*GetRValue(t_colorref);
        r_color.green = 257*GetGValue(t_colorref);
        r_color.blue = 257*GetBValue(t_colorref);
        
        MCscreen->alloccolor(r_color);
    }
    
    return t_found;
}

bool MCPlatformGetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_prop, MCFontRef& r_font)
{
    bool t_found;
    t_found = false;
    
    const char* t_fontname;
    int t_fontsize;
    
    switch (p_prop)
    {
        case kMCPlatformThemePropertyTextFont:
            t_found = true;
            t_fontname = MCmajorosversion >= 0x0600 ? "Segoe UI" : "Tahoma";
            MCPlatformGetControlThemePropInteger(p_type, p_part, p_state, kMCPlatformThemePropertyTextSize, t_fontsize);
            break;
    }
    
    if (t_found)
    {
        MCNameRef t_name;
        MCNameCreateWithCString(t_fontname, t_name);
        MCFontCreate(t_name, 0, t_fontsize, r_font);
        MCNameDelete(t_name);
    }
    
    return t_found;
}
