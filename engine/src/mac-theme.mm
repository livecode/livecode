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


#include "platform.h"

#include "osxprefix.h"

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

#import <AppKit/NSColor.h>
#import <AppKit/NSFont.h>
#import <AppKit/NSImageRep.h>
#import <CoreText/CoreText.h>

bool MCPlatformGetControlThemePropBool(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, bool& r_bool)
{
    return false;
}

bool MCPlatformGetControlThemePropInteger(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, int& r_int)
{
    bool t_found;
    t_found = false;
    
    switch (p_which)
    {
        case kMCPlatformThemePropertyTextSize:
        {
            // If in backwards-compatibility mode, all text is size 11
            if (p_state & kMCPlatformControlStateCompatibility)
                r_int = 11;
            else if (p_type == kMCPlatformControlTypeInputField)
                r_int = 11;
            else
                r_int = 13;
            t_found = true;
            break;
        }
        
        // Property is not known
        default:
            break;
    }
    
    return t_found;
}

bool MCPlatformGetControlThemePropColor(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCColor& r_color)
{
    bool t_found;
    t_found = false;
    
    NSColor *t_color;
    t_color = nil;
    
    bool t_is_pattern;
    t_is_pattern = false;
    
    switch (p_which)
    {
        case kMCPlatformThemePropertyTextColor:
        {
            t_found = true;
            if (p_state & kMCPlatformControlStateDisabled)
            {
                t_color = [NSColor disabledControlTextColor];
            }
            else
            {
                switch (p_type)
                {
                    case kMCPlatformControlTypeInputField:
                    {
                        if (p_state & kMCPlatformControlStateSelected)
                            t_color = [NSColor selectedTextColor];
                        else
                            t_color = [NSColor textColor];
                        break;
                    }
                        
                    case kMCPlatformControlTypeMenu:
                    case kMCPlatformControlTypeOptionMenu:
                    case kMCPlatformControlTypePopupMenu:
                    case kMCPlatformControlTypePulldownMenu:
                    case kMCPlatformControlTypeList:
                    {
                        if (p_state & kMCPlatformControlStateSelected
                            && p_state & kMCPlatformControlStateWindowActive)
                        {
                            if (p_type == kMCPlatformControlTypeList)
                                t_color = [NSColor alternateSelectedControlTextColor];
                            else
                                t_color = [NSColor selectedMenuItemTextColor];
                            break;
                        }
                        
                        /* FALLTHROUGH */
                    }
                        
                    default:
                        t_color = [NSColor controlTextColor];
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
                    case kMCPlatformControlTypeInputField:
                        t_color = [NSColor selectedTextBackgroundColor];
                        break;
                        
                    case kMCPlatformControlTypeMenu:
                    case kMCPlatformControlTypeOptionMenu:
                    case kMCPlatformControlTypePopupMenu:
                    case kMCPlatformControlTypePulldownMenu:
                        t_color = [NSColor selectedMenuItemColor];
                        break;
                        
                    case kMCPlatformControlTypeList:
                        if (p_state & kMCPlatformControlStateWindowActive)
                            t_color = [NSColor alternateSelectedControlColor];
                        else
                            t_color = [NSColor secondarySelectedControlColor];
                        break;
                        
                    default:
                        t_color = [NSColor selectedControlColor];
                        break;
                }
            }
            
            // Handle non-selected controls
            if (t_color == nil)
            {
                switch (p_type)
                {
                    case kMCPlatformControlTypeInputField:
                    case kMCPlatformControlTypeList:
                        t_color = [NSColor textBackgroundColor];
                        break;
                        
                    case kMCPlatformControlTypeWindow:
                        // In compatibility mode, handle window colour the old way
                        if (p_state & kMCPlatformControlStateCompatibility)
                        {
                            return false;
                        }
                        /* FALLTHROUGH */
                        
                    case kMCPlatformControlTypeMessageBox:
                        // windowBackgroundColor is a pattern
                        t_is_pattern = true;
                        t_color = [NSColor windowBackgroundColor];
                        break;
                        
                    default:
                        // controlColor is a pattern
                        t_is_pattern = true;
                        t_color = [NSColor controlColor];
                        break;
                }
            }
            
            break;
        }
        
        case kMCPlatformThemePropertyBorderColor:
        {
            break;
        }
            
        case kMCPlatformThemePropertyShadowColor:
        {
            t_found = true;
            if (p_type == kMCPlatformControlTypeWindow || p_type == kMCPlatformControlTypeMessageBox)
                t_color = [NSColor shadowColor];
            else
                t_color = [NSColor controlShadowColor];
            break;
        }
            
        case kMCPlatformThemePropertyFocusColor:
        {
            t_found = true;
            t_color = [NSColor keyboardFocusIndicatorColor];
            break;
        }
            
        // Property is not known
        default:
            break;
    }
    
    if (t_found && t_color != nil)
    {
        if (t_is_pattern)
        {
            // Patterns not supported at the moment
            t_color = [[NSColor controlHighlightColor] colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
        }
        else
        {
            t_color = [t_color colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
        }
        
        r_color.red = [t_color redComponent] * 65535;
        r_color.green = [t_color greenComponent] * 65535;
        r_color.blue = [t_color blueComponent] * 65535;
    }
    
    // Ensure the colour structure is set up correctly
    if (t_found)
        MCscreen->alloccolor(r_color);
    
    return t_found;
}

bool MCPlatformGetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCFontRef& r_font)
{
    // First of all, get the expected size of the font for the given property
    int t_font_size;
    if (!MCPlatformGetControlThemePropInteger(p_type, p_part, p_state, kMCPlatformThemePropertyTextSize, t_font_size))
        return false;
    
    // Only size 11 and 13 fonts are supported
    if (t_font_size != 11 && t_font_size != 13)
        return false;
    
    // Fonts are expensive to create so cache them
    static MCFontRef s_user_font_11;
    static MCFontRef s_user_font_13;
    static MCFontRef s_system_font_11;
    static MCFontRef s_system_font_13;
    
    MCFontRef* t_which_fontref = nil;
    
    NSFont* t_font = nil;
    switch (p_type)
    {
        case kMCPlatformControlTypeInputField:
            t_which_fontref = t_font_size == 11 ? &s_user_font_11 : &s_user_font_13;
            if (*t_which_fontref == nil)
                t_font = [[NSFont userFontOfSize: t_font_size] retain];
            break;
            
        default:
            t_which_fontref = t_font_size == 11 ? &s_system_font_11 : &s_system_font_13;
            if (*t_which_fontref == nil)
                t_font = [[NSFont systemFontOfSize: t_font_size] retain];
            break;
    }
    
    if (*t_which_fontref == nil && t_font == nil)
        return false;
    
    if (*t_which_fontref == nil)
    {
        // Create the in-engine font representation
        MCFontCreateWithHandle((MCSysFontHandle)t_font, *t_which_fontref);
    }

    r_font = MCFontRetain(*t_which_fontref);
    return true;
}
