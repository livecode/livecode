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
#include "button.h"
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
#include "mcio.h"
#include "system.h"
#include "globals.h"
#include "context.h"

#include "widgetimp.h"
#include "widget-events.h"

#include "module-canvas.h"

#include "module-engine.h"

#include "dispatch.h"
#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

MCWidgetCommon::MCWidgetCommon(void)
{
    m_instance = nil;
    m_children = nil;
}

MCWidgetCommon::~MCWidgetCommon(void)
{
    // Nothing to do here as 'Destroy()' must be called first.
}

// The Create() method attempts to setup a new instance of the given kind and
// then trys to call the create method.
bool MCWidgetCommon::Create(MCNameRef p_kind)
{
    MCAssert(m_instance == nil);
    
    // Attempt to find the module and build an instance (lookup module is a get
    // whereas creating an instance is a copy).
    MCScriptModuleRef t_module;
    MCScriptInstanceRef t_instance;
    if (!MCScriptLookupModule(p_kind, t_module) ||
        !MCScriptEnsureModuleIsUsable(t_module) ||
        !MCScriptCreateInstanceOfModule(t_module, t_instance))
        return false;
    
    // We have an instance, so assign the necessary fields so we can create.
    m_instance = t_instance;
    
    // If creation fails, then we destroy and return false. We assume that if
    // 'Create()' fails, then 'Destroy()' mustn't be called. This does mean that
    // a widget developer must be careful to either ensure Create() does not fail,
    // or if it does there is nothing to free which would be missed by just freeing
    // the children and the instance.
    if (!OnCreate())
    {
        MCValueRelease(m_children);
        m_children = nil;
        
        MCScriptReleaseInstance(m_instance);
        m_instance = nil;
        
        return false;
    }
    
    return true;
}

// The Destroy() method tears down a previously Created widget.
void MCWidgetCommon::Destroy(void)
{
    // If there is no instance, there is nothing to do.
    if (m_instance == nil)
        return;
    
    // Invoke the widget's destroy handler.
    OnDestroy();
    
    // Now free our fields.
    MCValueRelease(m_children);
    m_children = nil;
    
    MCScriptReleaseInstance(m_instance);
    m_instance = nil;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWidgetCreateHost(MCWidgetRef& r_widget)
{
    if (!MCValueCreateCustom(kMCWidgetTypeInfo, sizeof(MCWidgetHost), r_widget))
        return false;
    
    MCWidgetHost *t_widget_host;
    t_widget_host = (MCWidgetHost *)MCValueGetExtraBytesPtr(r_widget);
    
    new (t_widget_host) MCWidgetHost;
    
    return true;
}

bool MCWidgetCreateChild(MCWidgetRef& r_widget)
{
    if (!MCValueCreateCustom(kMCWidgetTypeInfo, sizeof(MCWidgetChild), r_widget))
        return false;
    
    MCWidgetChild *t_widget_host;
    t_widget_host = (MCWidgetChild *)MCValueGetExtraBytesPtr(r_widget);
    
    new (t_widget_host) MCWidgetChild;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////
