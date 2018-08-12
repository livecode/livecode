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

#import <UIKit/UIColor.h>
#import <UIKit/UIFont.h>
#import <CoreText/CoreText.h>


// The following methods are documented in the UIFont class documentation but
// aren't declared in the header so we extend the UIFont interface here to
// ensure the compiler knows about them.
@interface UIFont ()
+ (CGFloat)labelFontSize;
+ (CGFloat)buttonFontSize;
+ (CGFloat)systemFontSize;
@end


// Returns the name of the legacy font
static NSString* get_legacy_font_name()
{
    return @"Helvetica";
}

// Returns the correct font for a control of the given type
static UIFont* font_for_control(MCPlatformControlType p_type, MCPlatformControlState p_state, MCNameRef* r_name = nil)
{
    // Always return the same font regardless of control type in legacy mode
    if (p_state & kMCPlatformControlStateCompatibility)
    {
        static UIFont* s_legacy_font = nil;
        if (nil == s_legacy_font)
            s_legacy_font = [[UIFont fontWithName:get_legacy_font_name() size:13] retain];
        if (nil == s_legacy_font)
            s_legacy_font = [[UIFont systemFontOfSize:13] retain];

        MCAssert(nil != s_legacy_font);

        if (r_name)
            *r_name = nil;
        return s_legacy_font;
    }
    
    switch (p_type)
    {
        case kMCPlatformControlTypeMenu:
        case kMCPlatformControlTypeMenuItem:
        case kMCPlatformControlTypePopupMenu:
        case kMCPlatformControlTypeOptionMenu:
        case kMCPlatformControlTypePulldownMenu:
        {
            static UIFont* s_label_font = [[UIFont systemFontOfSize: [UIFont labelFontSize]] retain];
            if (r_name)
                *r_name = MCValueRetain(MCN_font_system);
            return s_label_font;
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
            static UIFont* s_button_font = [[UIFont systemFontOfSize: [UIFont buttonFontSize]] retain];
            if (r_name)
                *r_name = MCValueRetain(MCN_font_system);
            return s_button_font;
        }
            
        case kMCPlatformControlTypeInputField:
        case kMCPlatformControlTypeComboBox:
        case kMCPlatformControlTypeTooltip:
        case kMCPlatformControlTypeRichText:
        default:
        {
            static UIFont* s_system_font = [[UIFont systemFontOfSize: [UIFont systemFontSize]] retain];
            if (r_name)
                *r_name = MCValueRetain(MCN_font_system);
            return s_system_font;
        }
    }
    
    if (r_name)
        *r_name = nil;
    return nil;
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
            // If in backwards-compatibility mode, all text is size 13
            if (p_state & kMCPlatformControlStateCompatibility)
                r_int = 13;
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
    return false;
}

bool MCPlatformGetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCFontRef& r_font)
{
    // Get the font for the given control type
	MCNewAutoNameRef t_font_name;
	UIFont* t_font = font_for_control(p_type, p_state, &(&t_font_name));
    if (t_font == nil)
        return false;
    
    // Ensure the font is registered and return it
    return MCFontCreateWithHandle((MCSysFontHandle)t_font, *t_font_name, r_font);
}
