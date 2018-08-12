/* Copyright (C) 2015 LiveCode Ltd.
 
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


// Returns the name of the legacy font
static NSString* get_legacy_font_name()
{
    if (MCmajorosversion < 0x10A0)
        return @"Lucida Grande";
    if (MCmajorosversion > 0x10B0)
        return @"San Francisco";
    else
        return @"Helvetica Neue";
}

// Returns the correct font for a control of the given type
static NSFont* font_for_control(MCPlatformControlType p_type, MCPlatformControlState p_state, MCNameRef* r_name = nil)
{
    // Always return the same font regardless of control type in legacy mode
    if (p_state & kMCPlatformControlStateCompatibility)
    {
        static NSFont* s_legacy_font = nil;
        if (nil == s_legacy_font)
            s_legacy_font = [[NSFont fontWithName:get_legacy_font_name() size:11] retain];
        if (nil == s_legacy_font)
            s_legacy_font = [[NSFont systemFontOfSize:11] retain];

        MCAssert(nil != s_legacy_font);

        if (r_name)
            *r_name = nil;
        return s_legacy_font;
    }
    
    switch (p_type)
    {
        case kMCPlatformControlTypeRichText:
        {
            static NSFont* s_user_font = [[NSFont userFontOfSize:-1.0] retain];
            if (r_name)
                *r_name = MCValueRetain(MCN_font_usertext);
            return s_user_font;
        }
            
        case kMCPlatformControlTypeMenu:
        case kMCPlatformControlTypeMenuItem:
        case kMCPlatformControlTypePopupMenu:
        case kMCPlatformControlTypeOptionMenu:
        case kMCPlatformControlTypePulldownMenu:
        {
            static NSFont* s_menu_font = [[NSFont menuFontOfSize:-1.0] retain];
            if (r_name)
                *r_name = MCValueRetain(MCN_font_menutext);
            return s_menu_font;
        }
            
        case kMCPlatformControlTypeInputField:
        case kMCPlatformControlTypeComboBox:
        {
            static NSFont* s_content_font = [[NSFont controlContentFontOfSize:-1.0] retain];
            if (r_name)
                *r_name = MCValueRetain(MCN_font_content);
            return s_content_font;
        }
            
        case kMCPlatformControlTypeButton:
        case kMCPlatformControlTypeCheckbox:
        case kMCPlatformControlTypeLabel:
        case kMCPlatformControlTypeRadioButton:
        case kMCPlatformControlTypeList:
        case kMCPlatformControlTypeMessageBox:
        case kMCPlatformControlTypeTabButton:
        case kMCPlatformControlTypeTabPane:
        {
            static NSFont* s_message_font = [[NSFont messageFontOfSize:-1.0] retain];
            if (r_name)
                *r_name = MCValueRetain(MCN_font_message);
            return s_message_font;
        }
            
        case kMCPlatformControlTypeTooltip:
        {
            static NSFont* s_tooltip_font = [[NSFont toolTipsFontOfSize:-1.0] retain];
            if (r_name)
                *r_name = MCValueRetain(MCN_font_tooltip);
            return s_tooltip_font;
        }
            
        default:
        {
            static NSFont* s_system_font = [[NSFont systemFontOfSize:[NSFont systemFontSize]] retain];
            if (r_name)
                *r_name = MCValueRetain(MCN_font_system);
            return s_system_font;
        }
    }
}


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
            else
                return [font_for_control(p_type, p_state) pointSize];
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
                        
                    case kMCPlatformControlTypeTabPane:
                    case kMCPlatformControlTypeTabButton:
                    {
                        // These really should update like the other menu types
                        // do when the window isn't active but we don't have
                        // access to the active-tab-but-inactive-window button
                        // appearance used for "real" tabbed controls.
                        if (p_state & kMCPlatformControlStateSelected)
                            t_color = [NSColor selectedMenuItemTextColor];
                        else
                            t_color = [NSColor controlTextColor];
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
                        
                    case kMCPlatformControlTypeTooltip:
                        // Undocumented but it works (and other mac apps use it)
                        t_color = [NSColor toolTipColor];
                        t_found = t_color != nil;
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
            t_found = true;
            t_color = [NSColor blackColor];
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
            
        case kMCPlatformThemePropertyTopEdgeColor:
        case kMCPlatformThemePropertyLeftEdgeColor:
        {
            t_found = true;
            t_color = [NSColor controlLightHighlightColor];
            break;
        }
            
        case kMCPlatformThemePropertyBottomEdgeColor:
        case kMCPlatformThemePropertyRightEdgeColor:
        {
            t_found = true;
            t_color = [NSColor controlShadowColor];
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
    
    return t_found;
}

bool MCPlatformGetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCFontRef& r_font)
{
    // Get the font for the given control type
	MCNewAutoNameRef t_font_name;
    NSFont* t_font = font_for_control(p_type, p_state, &(&t_font_name));
    if (t_font == nil)
        return false;
    
    // Ensure the font is registered and return it
    return MCFontCreateWithHandle((MCSysFontHandle)t_font, *t_font_name, r_font);
}
