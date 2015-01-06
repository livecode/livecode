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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "util.h"
#include "mcerror.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "widget.h"
#include "param.h"
#include "osspec.h"
#include "cmds.h"
#include "scriptpt.h"
#include "hndlrlst.h"
#include "debug.h"
#include "redraw.h"
#include "font.h"
#include "chunk.h"
#include "graphicscontext.h"
#include "objptr.h"

#include "globals.h"
#include "context.h"

#include "native-layer.h"

MCNativeLayer::MCNativeLayer() :
  m_attached(false)
{
    ;
}

MCNativeLayer::~MCNativeLayer()
{
    ;
}

bool MCNativeLayer::isAttached() const
{
    return m_attached;
}

MCWidget* MCNativeLayer::findNextLayerAbove(MCWidget* p_widget)
{
    MCWidget *t_before;
    MCControl *t_control;
    t_before = NULL;
    t_control = p_widget->next();
    while (t_control != p_widget && t_control != p_widget->getcard()->getobjptrs()->getref())
    {
        // We are only looking for widgets with a native layer
        if (t_control->gettype() == CT_WIDGET && reinterpret_cast<MCWidget*>(t_control)->getNativeLayer() != nil)
        {
            // Found what we are looking for
            t_before = reinterpret_cast<MCWidget*>(t_control);
            break;
        }
        
        // Next control
        t_control = t_control->next();
    }
    
    return t_before;
}

MCWidget* MCNativeLayer::findNextLayerBelow(MCWidget* p_widget)
{
    MCWidget *t_after;
    MCControl *t_control;
    t_after = NULL;
    t_control = p_widget->prev();
    while (t_control != p_widget && t_control != p_widget->getcard()->getobjptrs()->getref()->prev())
    {
        // We are only looking for widgets with a native layer
        if (t_control->gettype() == CT_WIDGET && reinterpret_cast<MCWidget*>(t_control)->getNativeLayer() != nil)
        {
            // Found what we are looking for
            t_after = reinterpret_cast<MCWidget*>(t_control);
            break;
        }
        
        // Next control
        t_control = t_control->next();
    }
    
    return t_after;
}
