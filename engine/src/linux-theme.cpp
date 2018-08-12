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


#include <gtk/gtk.h>

#define GTK_MAGIC_FONT_SCALE_FACTOR 96/72

// Cached styles for various widget types
static GtkStyle* s_styles[kMCPlatformControlTypeMessageBox+1];

// Cached widgets (for style updates)
static GtkWidget* s_widgets[kMCPlatformControlTypeMessageBox+1];

// Container for widgets
static GtkWidget* s_widget_container = NULL;

extern "C" int initialise_weak_link_gtk(void);
extern "C" int initialise_weak_link_X11(void);

// Creates a GtkWidget corresponding to the requested control type
static GtkWidget* getWidgetForControlType(MCPlatformControlType p_type, MCPlatformControlPart p_part)
{
    // Do nothing if running in no-UI mode
    if (MCnoui)
        return NULL;
    
    // Ensure that our container widget exists
    if (s_widget_container == NULL)
    {
        if (!MCscreen -> hasfeature(PLATFORM_FEATURE_NATIVE_THEMES))
            return NULL;
        
        gtk_init(NULL, NULL);
        
        // Create a new window
        GtkWidget* t_window;
        t_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        s_widgets[kMCPlatformControlTypeGeneric] = t_window;
        
        // Ensure it actually exists
        gtk_widget_realize(t_window);
        
        // Create a container to store our widgets and put it in the window
        s_widget_container = gtk_fixed_new();
        gtk_container_add(GTK_CONTAINER(t_window), GTK_WIDGET(s_widget_container));
        gtk_widget_realize(GTK_WIDGET(s_widget_container));
    }
    
    // Return the existing widget if possible
    if (s_widgets[p_type] != NULL)
    {
        g_object_ref(s_widgets[p_type]);
        return s_widgets[p_type];
    }
    
    GtkWidget* t_the_widget;
    t_the_widget = NULL;
    
    bool t_suppress_add;
    t_suppress_add = false;
    
    switch (p_type)
    {
        case kMCPlatformControlTypeGeneric:
            t_the_widget = s_widgets[kMCPlatformControlTypeGeneric];
            t_suppress_add = true;
            break;
            
        case kMCPlatformControlTypeButton:
            t_the_widget = gtk_button_new();
            break;
            
        case kMCPlatformControlTypeCheckbox:
            t_the_widget = gtk_check_button_new();
            break;
            
        case kMCPlatformControlTypeRadioButton:
            t_the_widget = gtk_radio_button_new(NULL);
            break;
            
        case kMCPlatformControlTypeTabButton:
        case kMCPlatformControlTypeTabPane:
            t_the_widget = gtk_notebook_new();
            break;
            
        case kMCPlatformControlTypeLabel:
            t_the_widget = gtk_label_new("LiveCode");
            break;
            
        case kMCPlatformControlTypeInputField:
            t_the_widget = gtk_entry_new();
            break;
            
        case kMCPlatformControlTypeList:
            t_the_widget = gtk_tree_view_new();
            break;
            
        case kMCPlatformControlTypeMenu:
            t_the_widget = gtk_menu_item_new();
            break;
            
        case kMCPlatformControlTypeMenuItem:
            t_the_widget = gtk_menu_item_new();
            break;
            
        case kMCPlatformControlTypeOptionMenu:
            t_the_widget = gtk_combo_box_new();
            break;
            
        case kMCPlatformControlTypePulldownMenu:
            t_the_widget = gtk_menu_item_new();
            break;
            
        case kMCPlatformControlTypeComboBox:
            t_the_widget = gtk_combo_box_new_with_entry();
            break;
            
        case kMCPlatformControlTypePopupMenu:
            t_the_widget = gtk_menu_item_new();
            break;
            
        case kMCPlatformControlTypeProgressBar:
            t_the_widget = gtk_progress_bar_new();
            break;
            
        case kMCPlatformControlTypeScrollBar:
            t_the_widget = gtk_vscrollbar_new(NULL);
            break;
            
        case kMCPlatformControlTypeSlider:
            break;
            
        case kMCPlatformControlTypeSpinArrows:
            break;
            
        case kMCPlatformControlTypeWindow:
            t_the_widget = s_widgets[kMCPlatformControlTypeGeneric];
            g_object_ref(t_the_widget);
            t_suppress_add = true;
            break;
            
        case kMCPlatformControlTypeMessageBox:
            break;
    }
    
    if (t_the_widget == NULL)
        return NULL;
    
    s_widgets[p_type] = t_the_widget;
    
    // Add to the container and realize so that styles get set up correctly
    if (!t_suppress_add)
        gtk_fixed_put(GTK_FIXED(s_widget_container), t_the_widget, 0, 0);
    gtk_widget_realize(t_the_widget);
    
    g_object_ref(t_the_widget);
    return t_the_widget;
}

// Gets the style for the given control type
static GtkStyle* getStyleForControlType(MCPlatformControlType p_type, MCPlatformControlPart p_part)
{
    if (s_styles[p_type] == NULL)
    {
        GtkWidget* t_widget;
        t_widget = getWidgetForControlType(p_type, p_part);
        
        if (t_widget != NULL)
        {
            s_styles[p_type] = gtk_widget_get_style(t_widget);
            g_object_unref(t_widget);
        }
    }
    
    return s_styles[p_type];
}


bool MCPlatformGetControlThemePropInteger(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_prop, int& r_int)
{
    GtkStyle* t_style;
    t_style = getStyleForControlType(p_type, p_part);
    if (t_style == NULL)
        return false;
    
    bool t_found;
    t_found = false;
    
    switch (p_prop)
    {
        case kMCPlatformThemePropertyTextSize:
        {
            t_found = true;
            
            // We use 12-point Helvetica on Linux traditionally
            if (p_state & kMCPlatformControlStateCompatibility)
            {
                r_int = 12;
                break;
            }
            
            r_int = (pango_font_description_get_size(t_style->font_desc) *
                     GTK_MAGIC_FONT_SCALE_FACTOR / PANGO_SCALE);
            break;
        }
            
        default:
            break;
    }
    
    return t_found;
}

bool MCPlatformGetControlThemePropColor(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_prop, MCColor& r_color)
{
    GtkStyle* t_style;
    t_style = getStyleForControlType(p_type, p_part);
    if (t_style == NULL)
        return false;
    
    bool t_found;
    t_found = false;
    
    GtkStateType t_gtk_state;
    if (p_state & kMCPlatformControlStateDisabled)
        t_gtk_state = GTK_STATE_INSENSITIVE;
    else if (p_state & kMCPlatformControlStateSelected)
        t_gtk_state = GTK_STATE_SELECTED;
    else if (p_state & kMCPlatformControlStatePressed)
        t_gtk_state = GTK_STATE_ACTIVE;
    else if (p_state & kMCPlatformControlStateMouseOver)
        t_gtk_state = GTK_STATE_PRELIGHT;
    else
        t_gtk_state = GTK_STATE_NORMAL;
    
    GdkColor t_color;
    
    switch (p_prop)
    {
        case kMCPlatformThemePropertyTextColor:
            t_found = true;
            t_color = t_style->text[t_gtk_state];
            break;
            
        case kMCPlatformThemePropertyBackgroundColor:
        {
            t_found = true;
            switch (p_type)
            {
                // We want the base colour, not background, for fields
                case kMCPlatformControlTypeInputField:
                case kMCPlatformControlTypeList:
                    t_color = t_style->base[t_gtk_state];
                    break;
                    
                // Suppress the disabled state to avoid some weird-looking menus
                case kMCPlatformControlTypeMenu:
                case kMCPlatformControlTypeOptionMenu:
                case kMCPlatformControlTypePopupMenu:
                case kMCPlatformControlTypeMenuItem:
                    if (t_gtk_state == GTK_STATE_INSENSITIVE)
                        t_gtk_state = GTK_STATE_NORMAL;
                    /* FALLTHROUGH */
                
                default:
                    t_color = t_style->bg[t_gtk_state];
                    break;
            }
            break;
        }
            
        case kMCPlatformThemePropertyShadowColor:
            t_found =  true;
            t_color = t_style->dark[t_gtk_state];
            break;
            
        case kMCPlatformThemePropertyBorderColor:
            t_found = true;
            t_color = t_style->dark[t_gtk_state];
            break;
            
        case kMCPlatformThemePropertyFocusColor:
            break;
            
        case kMCPlatformThemePropertyTopEdgeColor:
        case kMCPlatformThemePropertyLeftEdgeColor:
            t_found = true;
            t_color = t_style->light[t_gtk_state];
            break;
        
        case kMCPlatformThemePropertyBottomEdgeColor:
        case kMCPlatformThemePropertyRightEdgeColor:
            t_found = true;
            t_color = t_style->dark[t_gtk_state];
            break;
    }
    
    if (t_found)
    {
        r_color.red = t_color.red;
        r_color.green = t_color.green;
        r_color.blue = t_color.blue;
    }
    
    return t_found;
}

// Utility function needed by the Linux font code. Gets the family name of the
// font for the given control type.
bool MCPlatformGetControlThemePropString(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState, MCPlatformThemeProperty p_prop, MCStringRef& r_string)
{
    if (p_prop != kMCPlatformThemePropertyTextFont)
        return false;
    
    GtkStyle* t_style;
    t_style = getStyleForControlType(p_type, p_part);
    if (t_style == NULL)
        return false;
    
    const PangoFontDescription* t_pango = t_style->font_desc;
    return MCStringCreateWithCString(pango_font_description_get_family(t_pango), r_string);
}

bool MCPlatformGetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_prop, MCFontRef& r_font)
{
    GtkStyle* t_style;
    t_style = getStyleForControlType(p_type, p_part);
    if (t_style == NULL)
        return MCFontCreate(MCNAME(DEFAULT_TEXT_FONT), 0, 12, r_font);
    
    bool t_found;
    t_found = false;
    
    const PangoFontDescription* t_pango;
    t_pango = NULL;
    
    switch (p_prop)
    {
        case kMCPlatformThemePropertyTextFont:
            t_found = true;
            t_pango = t_style->font_desc;
            break;
            
        default:
            break;
    }
    
    if (!t_found)
        return false;
    
    MCFontRef t_font_ref;
    MCNameRef t_font_name;
    int t_font_size;
    
    // We use 12-point Helvetica on Linux, traditionally
    const char *t_font_name_cstr = nullptr;
    if (p_state & kMCPlatformControlStateCompatibility)
    {
        t_font_name_cstr = DEFAULT_TEXT_FONT;
        t_font_size = 12;
    }
    else
    {
        t_font_name_cstr = pango_font_description_get_family(t_pango);
        t_font_size = pango_font_description_get_size(t_pango)/PANGO_SCALE;
        /* UNCHECKED */ MCPlatformGetControlThemePropInteger(p_type, p_part, p_state, kMCPlatformThemePropertyTextSize, t_font_size);
    }
    MCNameCreateWithNativeChars((const char_t*)t_font_name_cstr, strlen(t_font_name_cstr), t_font_name);
    
    if (t_found)
        t_found = MCFontCreate(t_font_name, 0, t_font_size, t_font_ref);
    if (t_found)
        r_font = t_font_ref;
    MCValueRelease(t_font_name);
    
    return t_found;
}
