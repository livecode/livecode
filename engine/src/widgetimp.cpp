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

MCWidgetBase::MCWidgetBase(void)
{
    m_instance = nil;
    m_children = nil;
}

MCWidgetBase::~MCWidgetBase(void)
{
    // Nothing to do here as 'Destroy()' must be called first.
}

bool MCWidgetBase::Create(MCNameRef p_kind)
{
    MCScriptModuleRef t_module;
    if (!MCScriptLookupModule(p_kind, t_module))
        return false;
    
    if (!MCScriptEnsureModuleIsUsable(t_module))
        return false;
    
    if (!MCScriptCreateInstanceOfModule(t_module, m_instance))
        return false;
    
    if (!DispatchRestricted(MCNAME("OnCreate")))
    {
        MCScriptReleaseInstance(m_instance);
        m_instance = nil;
        return false;
    }
    
    return true;
}

void MCWidgetBase::Destroy(void)
{
    if (m_instance == nil)
        return;
    
    for(uindex_t i = 0; i < MCProperListGetLength(m_children); i++)
    {
        MCWidgetRef t_child;
        t_child = (MCWidgetRef)MCProperListFetchElementAtIndex(m_children, i);
        
        MCWidgetAsChild(t_child) -> SetOwner(nil);
    }
    
    MCValueRelease(m_children);
    m_children = nil;
    
    DispatchRestrictedNoThrow(MCNAME("OnDestroy"));
    MCScriptReleaseInstance(m_instance);
    m_instance = nil;
}

//////////

bool MCWidgetBase::HasProperty(MCNameRef p_property)
{
    MCTypeInfoRef t_setter, t_getter;
    return MCScriptQueryPropertyOfModule(MCScriptGetModuleOfInstance(m_instance), p_property, t_getter, t_setter);
}

bool MCWidgetBase::HasHandler(MCNameRef p_handler)
{
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(MCScriptGetModuleOfInstance(m_instance), p_handler, t_signature);
}

bool MCWidgetBase::SetProperty(MCNameRef p_property, MCValueRef p_value)
{
    return MCScriptSetPropertyOfInstance(m_instance, p_property, p_value);
}

bool MCWidgetBase::GetProperty(MCNameRef p_property, MCValueRef& r_value)
{
    return MCScriptGetPropertyOfInstance(m_instance, p_property, r_value);
}

bool MCWidgetBase::OnLoad(MCValueRef p_rep)
{
    MCAutoValueRefArray t_args;
    if (!t_args . New(1))
        return false;
    
    t_args . Push(p_rep);
    
    return DispatchRestricted(MCNAME("OnLoad"), t_args . Ptr(), t_args . Count());
}

bool MCWidgetBase::OnSave(MCValueRef& r_rep)
{
    MCAutoValueRefArray t_args;
    if (!t_args . New(1))
        return false;
    
    if (!DispatchRestricted(MCNAME("OnSave"), t_args . Ptr(), t_args . Count()))
        return false;
    
    r_rep = t_args[0];
    t_args[0] = nil;
    
    return true;
}

bool MCWidgetBase::OnOpen(void)
{
    return DispatchRecursive(kDispatchOrderBeforeBottomUp, MCNAME("OnOpen"));
}

bool MCWidgetBase::OnClose(void)
{
    return DispatchRecursive(kDispatchOrderTopDownAfter, MCNAME("OnClose"));
}

bool MCWidgetBase::OnPaint(MCGContextRef p_gcontext)
{
    bool t_success;
    t_success = true;
    
    uintptr_t t_cookie;
    MCCanvasPush(p_gcontext, t_cookie);
    
    MCGRectangle t_frame;
    t_frame = GetFrame();
    
    MCGContextSave(p_gcontext);
    MCGContextClipToRect(p_gcontext, t_frame);
    MCGContextTranslateCTM(p_gcontext, t_frame . origin . x, t_frame . origin . y);
    if (!DispatchRecursive(kDispatchOrderBeforeBottomUp, MCNAME("OnPaint")))
        t_success = false;
    MCGContextRestore(p_gcontext);
    
    MCCanvasPop(t_cookie);
    
    return t_success;
}

bool MCWidgetBase::OnHitTest(MCGPoint p_location, MCWidgetRef& r_target)
{
    bool t_success;
    t_success = true;
    
    MCGRectangle t_frame;
    t_frame = GetFrame();
    
    if (!MCGPointInRectangle(p_location, t_frame))
    {
        r_target = nil;
        return true;
    }
    
    MCWidgetRef t_target;
    t_target = nil;
    if (m_children != nil)
    {
        p_location . x -= t_frame . origin . x;
        p_location . y -= t_frame . origin . y;
        for(uindex_t i = 0; i < MCProperListGetLength(m_children); i++)
        {
            MCWidgetRef t_child;
            t_child = MCProperListFetchElementAtIndex(m_children, i);
            if (!MCWidgetOnHitTest(t_child, p_location, t_target))
            {
                t_success = false;
                t_target = nil;
            }
            
            if (t_target != nil)
                break;
        }
    }
    
    if (t_target == nil)
        t_target = AsWidget();
    
    r_target = t_target;
    
    return t_success;
}

bool MCWidgetBase::OnMouseEnter(void)
{
    return Dispatch(MCNAME("OnMouseEnter"));
}

bool MCWidgetBase::OnMouseLeave(void)
{
    return Dispatch(MCNAME("OnMouseLeave"));
}

bool MCWidgetBase::OnMouseMove(void)
{
    return Dispatch(MCNAME("OnMouseLeave"));
}

bool MCWidgetBase::OnMouseDown(void)
{
    return Dispatch(MCNAME("OnMouseDown"));
}

bool MCWidgetBase::OnMouseUp(void)
{
    return Dispatch(MCNAME("OnMouseUp"));
}

bool MCWidgetBase::OnMouseCancel(void)
{
    return Dispatch(MCNAME("OnMouseCancel"));
}

bool MCWidgetBase::OnClick(void)
{
    return Dispatch(MCNAME("OnClick"));
}

bool MCWidgetBase::OnGeometryChanged(void)
{
    return Dispatch(MCNAME("OnGeometryChanged"));
}

bool MCWidgetBase::OnParentPropertyChanged(void)
{
    return DispatchRecursive(kDispatchOrderBeforeBottomUp, MCNAME("OnParentPropertyChanged"));
}

////////////////////////////////////////////////////////////////////////////////

bool MCWidgetCreateRoot(MCWidget *p_host, MCNameRef p_kind, MCWidgetRef& r_widget)
{
    MCWidgetRef t_widget;
    if (!MCValueCreateCustom(kMCWidgetTypeInfo, sizeof(MCWidgetRoot), t_widget))
        return false;
    
    MCWidgetRoot *t_widget_root;
    t_widget_root = (MCWidgetRoot *)MCValueGetExtraBytesPtr(t_widget);
    
    new (t_widget_root) MCWidgetRoot(p_host);
    
    if (!t_widget_root -> Create(p_kind))
    {
        MCValueRelease(t_widget);
        return false;
    }
    
    r_widget = t_widget;
    
    return true;
}

bool MCWidgetCreateChild(MCNameRef p_kind, MCWidgetRef& r_widget)
{
    MCWidgetRef t_widget;
    if (!MCValueCreateCustom(kMCWidgetTypeInfo, sizeof(MCWidgetRoot), t_widget))
        return false;
    
    MCWidgetChild *t_widget_child;
    t_widget_child = (MCWidgetChild *)MCValueGetExtraBytesPtr(t_widget);
    
    new (t_widget_child) MCWidgetChild;
    
    if (!t_widget_child -> Create(p_kind))
    {
        MCValueRelease(t_widget);
        return false;
    }
    
    r_widget = t_widget;
    
    return true;
}

bool MCWidgetIsRoot(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> IsRoot();
}

MCWidget *MCWidgetGetHost(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> GetHost();
}

MCGRectangle MCWidgetGetFrame(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> GetFrame();
}

bool MCWidgetGetDisabled(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> GetDisabled();
}

bool MCWidgetCopyFont(MCWidgetRef self, MCFontRef& r_font)
{
    return MCWidgetAsBase(self) -> CopyFont(r_font);
}

bool MCWidgetHasProperty(MCWidgetRef self, MCNameRef p_property)
{
    return MCWidgetAsBase(self) -> HasProperty(p_property);
}

bool MCWidgetHasHandler(MCWidgetRef self, MCNameRef p_handler)
{
    return MCWidgetAsBase(self) -> HasHandler(p_handler);
}

bool MCWidgetSetProperty(MCWidgetRef self, MCNameRef p_property, MCValueRef p_value)
{
    return MCWidgetAsBase(self) -> SetProperty(p_property, p_value);
}

bool MCWidgetGetProperty(MCWidgetRef self, MCNameRef p_property, MCValueRef& r_value)
{
    return MCWidgetAsBase(self) -> GetProperty(p_property, r_value);
}

bool MCWidgetOnLoad(MCWidgetRef self, MCValueRef p_rep)
{
    return MCWidgetAsBase(self) -> OnLoad(p_rep);
}

bool MCWidgetOnSave(MCWidgetRef self, MCValueRef& r_rep)
{
    return MCWidgetAsBase(self) -> OnSave(r_rep);
}

bool MCWidgetOnOpen(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnOpen();
}

bool MCWidgetOnClose(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnClose();
}

bool MCWidgetOnPaint(MCWidgetRef self, MCGContextRef p_gcontext)
{
    return MCWidgetAsBase(self) -> OnPaint(p_gcontext);
}

bool MCWidgetOnHitTest(MCWidgetRef self, MCGPoint p_location, MCWidgetRef& r_target)
{
    return MCWidgetAsBase(self) -> OnHitTest(p_location, r_target);
}

bool MCWidgetOnMouseEnter(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnMouseEnter();
}

bool MCWidgetOnMouseLeave(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnMouseLeave();
}

bool MCWidgetOnMouseMove(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnMouseMove();
}

bool MCWidgetOnMouseDown(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnMouseDown();
}

bool MCWidgetOnMouseUp(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnMouseUp();
}

bool MCWidgetOnMouseCancel(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnMouseCancel();
}

bool MCWidgetOnClick(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnClick();
}

bool MCWidgetOnGeometryChanged(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnGeometryChanged();
}

bool MCWidgetOnParentPropertyChanged(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnParentPropertyChanged();
}

void MCWidgetRedrawAll(MCWidgetRef self)
{
}

void MCWidgetScheduleTimerIn(MCWidgetRef self, double p_timeout)
{
}

void MCWidgetCancelTimer(MCWidgetRef self)
{
}

void MCWidgetCopyChildren(MCWidgetRef self, MCProperListRef& r_children)
{
    MCWidgetAsBase(self) -> CopyChildren(r_children);
}

void MCWidgetPlaceWidget(MCWidgetRef self, MCWidgetRef p_child, MCWidgetRef p_relative_to, bool p_put_below)
{
    MCWidgetAsBase(self) -> PlaceWidget(p_child, p_relative_to, p_put_below);
}

void MCWidgetUnplaceWidget(MCWidgetRef self,  MCWidgetRef p_child)
{
    MCWidgetAsBase(self) -> UnplaceWidget(p_child);
}

MCGPoint MCWidgetMapPointToGlobal(MCWidgetRef self, MCGPoint p_point)
{
    return MCWidgetAsBase(self) -> MapPointToGlobal(p_point);
}

MCGPoint MCWidgetMapPointFromGlobal(MCWidgetRef self, MCGPoint p_point)
{
    return MCWidgetAsBase(self) -> MapPointFromGlobal(p_point);
}

MCGRectangle MCWidgetMapRectToGlobal(MCWidgetRef self, MCGRectangle p_rect)
{
    return MCWidgetAsBase(self) -> MapRectToGlobal(p_rect);
}

MCGRectangle MCWidgetMapRectFromGlobal(MCWidgetRef self, MCGRectangle p_rect)
{
    return MCWidgetAsBase(self) -> MapRectFromGlobal(p_rect);
}

////////////////////////////////////////////////////////////////////////////////

MCWidgetBase *MCWidgetAsBase(MCWidgetRef self)
{
    return (MCWidgetBase *)MCValueGetExtraBytesPtr(self);
}

MCWidgetChild *MCWidgetAsChild(MCWidgetRef self)
{
    MCWidgetBase *t_base;
    t_base = (MCWidgetBase *)MCValueGetExtraBytesPtr(self);
    MCAssert(!t_base -> IsRoot());
    return static_cast<MCWidgetChild *>(t_base);
}

MCWidgetRoot *MCWidgetAsRoot(MCWidgetRef self)
{
    MCWidgetBase *t_base;
    t_base = (MCWidgetBase *)MCValueGetExtraBytesPtr(self);
    MCAssert(t_base -> IsRoot());
    return static_cast<MCWidgetRoot *>(t_base);
}

////////////////////////////////////////////////////////////////////////////////
