/* Copyright (C) 2016 LiveCode Ltd.
 
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


// Typographic information for Android: https://www.google.com/design/spec/style/typography.html#typography-styles
// Android design cheatsheet: http://petrnohejl.github.io/Android-Cheatsheet-For-Graphic-Designers/
// Android theming values: https://github.com/android/platform_frameworks_base/tree/master/core/res/res/values

bool MCPlatformGetControlThemePropBool(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, bool&)
{
    return false;
}

bool MCPlatformGetControlThemePropInteger(MCPlatformControlType p_type, MCPlatformControlPart, MCPlatformControlState p_state, MCPlatformThemeProperty p_prop, int& r_int)
{
    // The only integer property currently implemented for Android is text size
    if (p_prop != kMCPlatformThemePropertyTextSize)
        return false;
    
    // For legacy theming, the font is always 12-point Arial
    if (p_state & kMCPlatformControlStateCompatibility)
    {
        r_int = 12;
        return true;
    }
    
    // What type of control are we looking at?
    switch (p_type)
    {
        // Text input areas use 14-point Roboto
        case kMCPlatformControlTypeInputField:
        case kMCPlatformControlTypeComboBox:
        case kMCPlatformControlTypeTooltip:
        case kMCPlatformControlTypeRichText:
            r_int = 14;
            return true;
            
        // Everything else uses 18-point Roboto
        case kMCPlatformControlTypeMenu:
        case kMCPlatformControlTypeMenuItem:
        case kMCPlatformControlTypePopupMenu:
        case kMCPlatformControlTypeOptionMenu:
        case kMCPlatformControlTypePulldownMenu:
        case kMCPlatformControlTypeButton:
        case kMCPlatformControlTypeCheckbox:
        case kMCPlatformControlTypeLabel:
        case kMCPlatformControlTypeRadioButton:
        case kMCPlatformControlTypeList:
        case kMCPlatformControlTypeMessageBox:
        case kMCPlatformControlTypeTabButton:
        case kMCPlatformControlTypeTabPane:
        default:
            r_int = 18;
            return true;
    }
    
    return false;
}

bool MCPlatformGetControlThemePropColor(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, MCColor&)
{
    return false;
}

bool MCPlatformGetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_prop, MCFontRef& r_font)
{
    // The only supported font property is the text font
    if (p_prop != kMCPlatformThemePropertyTextFont)
        return false;
    
    // For legacy theming, the font is always 12-point Arial
    if (p_state & kMCPlatformControlStateCompatibility)
        return MCFontCreate(MCNAME("Arial"), 0, 12, r_font);
    
    // Get the text size for this control type
    int t_text_size;
    if (!MCPlatformGetControlThemePropInteger(p_type, p_part, p_state, kMCPlatformThemePropertyTextSize, t_text_size))
        return false;
    
    // The default font is Roboto of the appropriate size
    return MCFontCreate(MCNAME("Roboto"), 0, t_text_size, r_font);
}
