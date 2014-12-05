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

#include "text-api.h"

#include "text-pane.h"


void MCTextCellSetMaxSize(MCTextCellRef p_cell, coord_t p_width, coord_t p_height)
{
    p_cell->setMaxSize(p_width, p_height);
}

// Creates a new, empty text pane. The specified stack is used for scaling and
// theming only - the pane isn't a LiveCode control and isn't a child of the
// stack.
bool MCTextPaneCreate(class MCStack* p_on_stack, MCTextPaneRef& r_pane)
{
    r_pane = new MCTextPane;
    return true;
}

// Deletes the given text pane
bool MCTextPaneDelete(MCTextPaneRef p_pane)
{
    delete p_pane;
    return true;
}

// Sets the contents of the text pane to be the given string. Other than certain
// inline control characters (tabs, newlines, BiDi controls, etc), the string
// is unformatted.
bool MCTextPaneSetContentsPlain(MCTextPaneRef p_pane, MCStringRef p_contents)
{
    p_pane->setContentsPlain(p_contents);
    return true;
}

static MCTextPaneRef s_pane;
MCTextPaneRef MCTextPaneGet()
{
    return s_pane;
}

void MCTextPaneSet(MCTextPaneRef p_pane)
{
    s_pane = p_pane;
}

extern MCDC* g_widget_paint_dc;
void MCTextPanePaintShim(MCTextPaneRef p_pane)
{
    p_pane->draw(g_widget_paint_dc);
}
