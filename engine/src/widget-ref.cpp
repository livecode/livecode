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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"


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

#include "widget-ref.h"
#include "widget-events.h"

#include "module-canvas.h"
#include "module-engine.h"

#include "dispatch.h"
#include "graphics_util.h"

#include "native-layer.h"

////////////////////////////////////////////////////////////////////////////////

MCWidgetBase::MCWidgetBase(void)
{
    m_instance = nil;
    m_children = nil;
    m_annotations = nil;
    m_has_timer = false;
    m_timer_deferred = false;
}

MCWidgetBase::~MCWidgetBase(void)
{
    // Nothing to do here as 'Destroy()' must be called first.
}

bool MCWidgetBase::Create(MCNameRef p_kind)
{
    MCScriptModuleRef t_module;
    if (!MCScriptLookupModule(p_kind, t_module))
        return MCErrorThrowGenericWithMessage(MCSTR("unknown module '%{module}'"), "module", p_kind, nil);
    
    if (!MCScriptEnsureModuleIsUsable(t_module))
        return false;
    
    if (!MCScriptCreateInstanceOfModule(t_module, m_instance))
        return false;
    
    MCScriptSetInstanceHostPtr(m_instance, AsWidget());
    
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
    
    if (m_has_timer)
        CancelTimer();
    
    if (m_children != nil)
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
    
    MCValueRelease(m_annotations);
    m_annotations = nil;
}

MCWidgetRef MCWidgetBase::AsWidget(void)
{
    return (MCWidgetRef)(((uint8_t *)this) - kMCValueCustomHeaderSize);
}

MCNameRef MCWidgetBase::GetKind(void) const
{
    return MCScriptGetNameOfModule(MCScriptGetModuleOfInstance(m_instance));
}

//////////

bool MCWidgetBase::QueryProperty(MCNameRef p_property, MCTypeInfoRef& r_getter, MCTypeInfoRef& r_setter)
{
    return MCScriptQueryPropertyOfModule(MCScriptGetModuleOfInstance(m_instance), p_property, r_getter, r_setter);
}

bool MCWidgetBase::HasProperty(MCNameRef p_property)
{
    MCTypeInfoRef t_setter, t_getter;
    return MCScriptQueryPropertyOfModule(MCScriptGetModuleOfInstance(m_instance), p_property, t_getter, t_setter);
}

bool MCWidgetBase::HasHandler(MCNameRef p_handler)
{
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerSignatureOfModule(MCScriptGetModuleOfInstance(m_instance), p_handler, t_signature);
}

bool MCWidgetBase::SetProperty(MCNameRef p_property, MCValueRef p_value)
{
    return MCScriptSetPropertyInInstance(m_instance, p_property, p_value);
}

bool MCWidgetBase::GetProperty(MCNameRef p_property, MCValueRef& r_value)
{
    return MCScriptGetPropertyInInstance(m_instance, p_property, r_value);
}

static bool chunk_property_handler(MCNameRef p_property, MCNameRef p_chunk_name, bool p_is_get_operation, MCNameRef& r_handler)
{
    const char *t_handler_prefix;
    if (p_is_get_operation)
        t_handler_prefix = "Get";
    else
        t_handler_prefix = "Set";
    
    MCAutoStringRef t_handler_string;
    if (!MCStringFormat(&t_handler_string, "%s%@Of%@", t_handler_prefix, p_property, p_chunk_name))
        return false;
    
    return MCNameCreate(*t_handler_string, r_handler);
}

bool MCWidgetBase::HasPropertyOfChunk(MCNameRef p_property, MCNameRef p_chunk_name, bool p_is_getter)
{
    MCTypeInfoRef t_typeinfo;
    return QueryPropertyOfChunk(p_property, p_chunk_name, p_is_getter, t_typeinfo);
}

bool MCWidgetBase::QueryPropertyOfChunk(MCNameRef p_property, MCNameRef p_chunk_name, bool p_is_getter, MCTypeInfoRef& r_type_info)
{
    MCNewAutoNameRef t_handler;
    if (!chunk_property_handler(p_property, p_chunk_name, p_is_getter, &t_handler))
        return false;
    
    MCTypeInfoRef t_handler_typeinfo;
    if (!MCScriptQueryHandlerSignatureOfModule(MCScriptGetModuleOfInstance(m_instance), *t_handler, t_handler_typeinfo))
        return false;

    if (p_is_getter)
    {
        if (MCHandlerTypeInfoGetParameterCount(t_handler_typeinfo) != 1)
            return false;
        
        r_type_info = MCHandlerTypeInfoGetReturnType(t_handler_typeinfo);
    }
    else
    {
        // Chunk property setter needs two parameters,
        if (MCHandlerTypeInfoGetParameterCount(t_handler_typeinfo) != 2)
            return false;
        
        r_type_info = MCHandlerTypeInfoGetParameterType(t_handler_typeinfo, 1);
    }

    return true;
}

bool MCWidgetBase::SetPropertyOfChunk(MCNameRef p_property, MCNameRef p_chunk_name, MCProperListRef p_path, MCValueRef p_value)
{
    MCAutoValueRefArray t_args;
    if (!t_args . New(2))
        return false;
    
    t_args[0] = MCValueRetain(p_path);
    t_args[1] = MCValueRetain(p_value);
    
    MCNewAutoNameRef t_handler;
    if (!chunk_property_handler(p_property, p_chunk_name, false, &t_handler))
        return false;
    
    return DispatchRestricted(*t_handler, t_args . Ptr(), t_args . Count());
}

bool MCWidgetBase::GetPropertyOfChunk(MCNameRef p_property, MCNameRef p_chunk_name, MCProperListRef p_path, MCValueRef& r_value)
{
    MCAutoValueRefArray t_args;
    if (!t_args . New(1))
        return false;
    
    t_args[0] = MCValueRetain(p_path);
    
    MCNewAutoNameRef t_handler;
    if (!chunk_property_handler(p_property, p_chunk_name, true, &t_handler))
        return false;
    
    return DispatchRestricted(*t_handler, t_args . Ptr(), t_args . Count(), &r_value);
}

bool MCWidgetBase::OnLoad(MCValueRef p_rep)
{
    MCAutoValueRefArray t_args;
    if (!t_args . New(1))
        return false;
    
    t_args[0] = MCValueRetain(p_rep);
    
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
    if (!DispatchRecursive(kDispatchOrderBeforeBottomUp, MCNAME("OnOpen")))
		return false;

	return OnAttach();
}

bool MCWidgetBase::OnClose(void)
{
	if (!OnDetach())
		return false;
	
    bool t_success;
    t_success = true;
    
    if (m_children != nil)
    {
        for(uindex_t i = MCProperListGetLength(m_children); i > 0; i--)
        {
            MCWidgetRef t_child;
            t_child = MCProperListFetchElementAtIndex(m_children, i - 1);
            if (!MCWidgetOnClose(t_child))
                t_success = false;
        }
    }
    
    if (!Dispatch(MCNAME("OnClose")))
        t_success = false;
    
    if (m_has_timer)
        CancelTimer();
    
    return t_success;
}

bool MCWidgetBase::OnAttach()
{
    if (!Dispatch(MCNAME("OnAttach")))
		return false;

	if (GetHost()->getNativeLayer())
		GetHost()->getNativeLayer()->OnAttach();
	
	return true;
}

bool MCWidgetBase::OnDetach()
{
    if (GetHost()->getNativeLayer())
        GetHost()->getNativeLayer()->OnDetach();
    
    return Dispatch(MCNAME("OnDetach"));
}

bool MCWidgetBase::OnTimer(void)
{
    // We no longer have a timer.
    m_has_timer = false;
    
    // If we aren't in run mode, then defer the timer.
    if (GetHost() -> getstack() -> gettool(GetHost()) != T_BROWSE)
    {
        m_timer_deferred = true;
        return true;
    }
    
    // Dispatch the event.
    return Dispatch(MCNAME("OnTimer"));
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
    
    MCWidget *t_widget;
    t_widget = GetHost();
    
    bool t_view_rendered;
    t_view_rendered = false;
    
	if (t_widget->getNativeLayer() != nil)
	{
		// If the widget is not in edit mode, we trust it to paint itself
		if (t_widget->isInRunMode())
			t_view_rendered = true;
		else if (t_widget->getNativeLayer()->GetCanRenderToContext())
			t_success = t_view_rendered = t_widget->getNativeLayer()->OnPaint(p_gcontext);
	}
	
	if (t_success && !t_view_rendered)
		t_success = DispatchRestricted(MCNAME("OnPaint"));
    
    if (m_children != nil)
    {
        for(uindex_t i = 0; i < MCProperListGetLength(m_children); i++)
        {
            MCWidgetRef t_child;
            t_child = MCProperListFetchElementAtIndex(m_children, i);
            if (!MCWidgetOnPaint(t_child, p_gcontext))
                t_success = false;
        }
    }
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
        for(uindex_t i = MCProperListGetLength(m_children); i > 0; i--)
        {
            MCWidgetRef t_child;
            t_child = MCProperListFetchElementAtIndex(m_children, i - 1);
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

bool MCWidgetBase::OnMouseEnter(bool& r_bubble)
{
    return DispatchBubbly(MCNAME("OnMouseEnter"), nil, 0, r_bubble);
}

bool MCWidgetBase::OnMouseLeave(bool& r_bubble)
{
    return DispatchBubbly(MCNAME("OnMouseLeave"), nil, 0, r_bubble);
}

bool MCWidgetBase::OnMouseMove(bool& r_bubble)
{
    return DispatchBubbly(MCNAME("OnMouseMove"), nil, 0, r_bubble);
}

bool MCWidgetBase::OnMouseDown(bool& r_bubble)
{
    return DispatchBubbly(MCNAME("OnMouseDown"), nil, 0, r_bubble);
}

bool MCWidgetBase::OnMouseUp(bool& r_bubble)
{
    return DispatchBubbly(MCNAME("OnMouseUp"), nil, 0, r_bubble);
}

bool MCWidgetBase::OnMouseCancel(bool& r_bubble)
{
    return DispatchBubbly(MCNAME("OnMouseCancel"), nil, 0, r_bubble);
}

bool MCWidgetBase::OnClick(bool& r_bubble)
{
    return DispatchBubbly(MCNAME("OnClick"), nil, 0, r_bubble);
}

bool MCWidgetBase::OnMouseScroll(coord_t p_delta_x, coord_t p_delta_y, bool& r_bubble)
{
    MCAutoValueRefArray t_args;
    if (!t_args . New(2))
        return false;
    
    if (!MCNumberCreateWithReal(p_delta_x, (MCNumberRef&)t_args[0]))
        return false;
    
    if (!MCNumberCreateWithReal(p_delta_y, (MCNumberRef&)t_args[1]))
        return false;
    
    return DispatchBubbly(MCNAME("OnMouseScroll"), t_args . Ptr(), t_args . Count(), r_bubble);
}

bool MCWidgetBase::HandlesTouchEvents(void)
{
    if (HasHandler(MCNAME("OnTouchStart")) ||
        HasHandler(MCNAME("OnTouchMove")) ||
        HasHandler(MCNAME("OnTouchFinish")) ||
        HasHandler(MCNAME("OnTouchCancel")))
    {
        return true;
    }
    
    MCWidgetRef t_owner = GetOwner();
    if (t_owner != nullptr)
    {
        return MCWidgetHandlesTouchEvents(t_owner);
    }
    
    return false;
}

bool MCWidgetBase::OnTouchStart(bool& r_bubble)
{
    return DispatchBubbly(MCNAME("OnTouchStart"), nil, 0, r_bubble);
}

bool MCWidgetBase::OnTouchMove(bool& r_bubble)
{
    return DispatchBubbly(MCNAME("OnTouchMove"), nil, 0, r_bubble);
}

bool MCWidgetBase::OnTouchFinish(bool& r_bubble)
{
    return DispatchBubbly(MCNAME("OnTouchFinish"), nil, 0, r_bubble);
}

bool MCWidgetBase::OnTouchCancel(bool& r_bubble)
{
    return DispatchBubbly(MCNAME("OnTouchCancel"), nil, 0, r_bubble);
}

bool MCWidgetBase::OnGeometryChanged(void)
{
    return Dispatch(MCNAME("OnGeometryChanged"));
}

bool MCWidgetBase::OnLayerChanged()
{
    return Dispatch(MCNAME("OnLayerChanged"));
}

bool MCWidgetBase::OnVisibilityChanged(bool p_visible)
{
    MCAutoValueRefArray t_args;
    if (!t_args . New(1))
        return false;
    
    if (p_visible)
        (MCBooleanRef&)t_args[0] = MCValueRetain(kMCTrue);
    else
        (MCBooleanRef&)t_args[0] = MCValueRetain(kMCFalse);
    
    return Dispatch(MCNAME("OnVisibilityChanged"), t_args . Ptr(), t_args . Count());
}

bool MCWidgetBase::OnParentPropertyChanged(void)
{
    return DispatchRecursive(kDispatchOrderBeforeBottomUp, MCNAME("OnParentPropertyChanged"));
}

bool MCWidgetBase::OnToolChanged(Tool p_tool)
{
    bool t_success;
    t_success = true;
    if (p_tool == T_BROWSE)
    {
        if (m_timer_deferred)
        {
            m_timer_deferred = false;
            MCscreen -> addsubtimer(GetHost(), AsWidget(), MCM_internal, 0);
        }
        
        if (m_children != nil)
        {
            for(uindex_t i = 0; i < MCProperListGetLength(m_children); i++)
            {
                MCWidgetRef t_child;
                t_child = MCProperListFetchElementAtIndex(m_children, i);
                if (!MCWidgetOnToolChanged(t_child, p_tool))
                    t_success = false;
            }
        }
    }
    else if (p_tool != T_BROWSE)
    {
        if (!DispatchRecursive(kDispatchOrderBeforeBottomUp, MCNAME("OnStartEditing")))
            t_success = false;
    }
    
    return t_success;
}

bool MCWidgetBase::CopyAnnotation(MCNameRef p_annotation, MCValueRef& r_value)
{
    MCValueRef t_value;
    if (m_annotations == nil ||
        !MCArrayFetchValue(m_annotations, true, p_annotation, t_value))
    {
        r_value = MCValueRetain(kMCNull);
        return true;
    }
    
    r_value = MCValueRetain(t_value);
    
    return true;
}

bool MCWidgetBase::SetAnnotation(MCNameRef p_annotation, MCValueRef p_value)
{
    if (m_annotations == nil &&
        !MCArrayCreateMutable(m_annotations))
        return false;
    
    if (!MCArrayStoreValue(m_annotations, true, p_annotation, p_value))
        return false;
    
    return true;
}

bool MCWidgetBase::Post(MCNameRef p_event, MCProperListRef p_args)
{
    MCAutoValueRefArray t_args;
    if (!t_args . New(MCProperListGetLength(p_args)))
        return false;
    
    for(uindex_t i = 0; i < MCProperListGetLength(p_args); i++)
        t_args[i] = MCValueRetain(MCProperListFetchElementAtIndex(p_args, i));
    
    return Dispatch(p_event, t_args . Ptr(), t_args . Count(), nil);
}

void MCWidgetBase::ScheduleTimerIn(double p_timeout)
{
    MCWidget *t_host;
    t_host = GetHost();
    
    // Do nothing if we aren't embedded.
    if (t_host == nil)
        return;
    
    // Do nothing if the host isn't open.
    if (t_host -> getopened() == 0)
        return;
    
    MCscreen -> addsubtimer(GetHost(), AsWidget(), MCM_internal, (uint4)(p_timeout * 1000));
    
    m_has_timer = true;
    m_timer_deferred = false;
}

void MCWidgetBase::CancelTimer(void)
{
    MCWidget *t_host;
    t_host = GetHost();
    
    // Do nothing if we aren't embedded.
    if (t_host == nil)
        return;
    
    // Do nothing if the host isn't open.
    if (t_host -> getopened() == 0)
        return;
    
    // Do nothing if we don't have a timer.
    if (!m_has_timer)
        return;
    
    MCscreen -> cancelsubtimer(GetHost(), MCM_internal, AsWidget());
    
    m_has_timer = false;
    m_timer_deferred = false;
}

void MCWidgetBase::RedrawRect(MCGRectangle *p_area)
{
    MCGRectangle t_frame;
    t_frame = GetFrame();
    
    MCGRectangle t_area;
    if (p_area == nil)
        t_area = t_frame;
    else
    {
        t_area = MCGRectangleTranslate(*p_area, t_frame . origin . x, t_frame . origin . y);
        t_area = MCGRectangleIntersection(*p_area, t_frame);
    }
    
    if (IsRoot())
    {
        GetHost() -> layer_redrawrect(MCGRectangleGetIntegerExterior(t_area));
        return;
    }
    
    MCWidgetRef t_owner;
    t_owner = GetOwner();
    if (t_owner == nil)
        return;
    
    MCWidgetAsBase(t_owner) -> RedrawRect(p_area);
}

void MCWidgetBase::TriggerAll()
{
    if (IsRoot())
    {
        GetHost() -> signallisteners(P_CUSTOM);
        return;
    }
    
    MCWidgetRef t_owner;
    t_owner = GetOwner();
    if (t_owner == nil)
        return;
    
    MCWidgetAsBase(t_owner) -> TriggerAll();
}

bool MCWidgetBase::CopyChildren(MCProperListRef& r_children)
{
    if (m_children == nil)
    {
        r_children = MCValueRetain(kMCEmptyProperList);
        return true;
    }
    
    return MCProperListCopy(m_children, r_children);
}

void MCWidgetBase::PlaceWidget(MCWidgetRef p_child, MCWidgetRef p_other_widget, bool p_is_below)
{
    // Make sure we have a children list (we create on demand).
    if (m_children == nil &&
        !MCProperListCreateMutable(m_children))
        return;
    
    // Find out where the widget is going.
    uindex_t t_target_offset;
    if (p_other_widget == nil)
    {
        if (!p_is_below)
            t_target_offset = MCProperListGetLength(m_children);
        else
            t_target_offset = 0;
    }
    else
    {
        if (!MCProperListFirstIndexOfElement(m_children, p_other_widget, 0, t_target_offset))
        {
            MCErrorThrowGeneric(MCSTR("Relative widget is not a child of this widget"));
            return;
        }
        
        if (!p_is_below)
            t_target_offset += 1;
    }
    
    // Remove the widget from its current location if necessary.
    MCWidgetChild *t_child;
    t_child = MCWidgetAsChild(p_child);
    if (t_child -> GetOwner() != nil)
    {
        if (t_child -> GetOwner() != AsWidget())
        {
            MCErrorThrowGeneric(MCSTR("Widget is already placed inside another widget"));
            return;
        }
        
        // Nothing to do if we are placing back in the same place.
        if (p_child == p_other_widget)
            return;
        
        uindex_t t_current_offset;
        MCProperListFirstIndexOfElement(m_children, p_child, 0, t_current_offset);
        if (!MCProperListRemoveElement(m_children, t_current_offset))
            return;
        
        if (t_current_offset < t_target_offset)
            t_target_offset -= 1;
    }
    
    // Put the widget in the child list.
    if (!MCProperListInsertElement(m_children, p_child, t_target_offset))
        return;
    
    // Reparent the widget if it does not already have a parent.
    if (t_child -> GetOwner() == nil)
        t_child -> SetOwner(AsWidget());
    
    // Make sure the new child is opened, if the host is.
    if (GetHost() != nil &&
        GetHost() -> getopened() != 0)
        t_child -> OnOpen();
    
    // Tell the eventmanager that the child widget has been placed.
    MCwidgeteventmanager -> widget_appearing(p_child);
    
    // Force a redraw.
    MCWidgetRedrawAll(p_child);
}

void MCWidgetBase::UnplaceWidget(MCWidgetRef p_child)
{
    MCWidgetChild *t_child;
    t_child = MCWidgetAsChild(p_child);
    
    // Make sure the widget is closed, if the host is open.
    if (GetHost() != nil &&
        GetHost() -> getopened() != 0)
        t_child -> OnClose();
    
    // Find out where the widget is.
    uindex_t t_current_offset;
    if (m_children == nil ||
        !MCProperListFirstIndexOfElement(m_children, p_child, 0, t_current_offset))
    {
        MCErrorThrowGeneric(MCSTR("Widget is not a child of this widget"));
        return;
    }
    
    // Remove the widget.
    if (!MCProperListRemoveElement(m_children, t_current_offset))
        return;
    
    // Tell the eventmanager that the child widget has been unplaced.
    MCwidgeteventmanager -> widget_disappearing(p_child);
    
    // Reparent the widget.
    MCWidgetRedrawAll(p_child);
    t_child -> SetOwner(nil);
}

MCGPoint MCWidgetBase::MapPointToGlobal(MCGPoint p_point)
{
    MCGRectangle t_frame;
    t_frame = GetFrame();
    
    p_point . x += t_frame . origin . x;
    p_point . y += t_frame . origin . y;
    
    MCWidgetRef t_owner;
    t_owner = GetOwner();
    if (t_owner == nil)
        return p_point;
    
    return MCWidgetAsBase(t_owner) -> MapPointToGlobal(p_point);
}

MCGPoint MCWidgetBase::MapPointFromGlobal(MCGPoint p_point)
{
    MCGRectangle t_frame;
    t_frame = GetFrame();
    
    p_point . x -= t_frame . origin . x;
    p_point . y -= t_frame . origin . y;
    
    MCWidgetRef t_owner;
    t_owner = GetOwner();
    if (t_owner == nil)
        return p_point;
    
    return MCWidgetAsBase(t_owner) -> MapPointFromGlobal(p_point);
}

MCGRectangle MCWidgetBase::MapRectToGlobal(MCGRectangle rect)
{
    abort();
    return MCGRectangle();
}

MCGRectangle MCWidgetBase::MapRectFromGlobal(MCGRectangle rect)
{
    abort();
    return MCGRectangle();
}

//////////

bool MCWidgetBase::DoDispatch(MCNameRef p_event, MCValueRef *x_args, uindex_t p_arg_count, MCValueRef *r_result)
{
	bool t_success;
	MCAutoValueRef t_retval;
	t_success = MCScriptCallHandlerInInstanceIfFound(m_instance, p_event, x_args, p_arg_count, &t_retval);
	
	if (t_success)
	{
		if (r_result != NULL)
			*r_result = t_retval . Take();
	}
	
	return t_success;
	
}

bool MCWidgetBase::Dispatch(MCNameRef p_event, MCValueRef *x_args, uindex_t p_arg_count, MCValueRef *r_result)
{
	MCStack *t_this_stack;
	MCStackHandle t_old_defaultstack = MCdefaultstackptr;
	
	// Preserve the host ptr we get across the dispatch so that
	// we definitely return things to the way they were.
	MCWidget *t_host;
	t_host = GetHost();
	
    MCObjectPartHandle t_old_target;
    if (t_host)
    {
        swap(t_old_target, MCtargetptr);
        MCtargetptr = MCObjectPartHandle(t_host);

        t_this_stack = MCtargetptr -> getstack();
        MCdefaultstackptr = t_this_stack;
    }
    else
        MCEngineScriptObjectPreventAccess();
	
    // Invoke event handler.
    bool t_success;
	t_success = DoDispatch(p_event, x_args, p_arg_count, r_result);
	
	if (t_host != nil)
	{
        swap(MCtargetptr, t_old_target);
        if (MCdefaultstackptr == t_this_stack && t_old_defaultstack.IsValid())
            MCdefaultstackptr = t_old_defaultstack;
	}
	else
		MCEngineScriptObjectAllowAccess();
	
    if (!t_success)
	{
		// Refetch the host pointer *just in case* something has happened
		// to it.
		t_host = GetHost();
		if (t_host != nil)
			t_host -> SendError();
	}
	
	return t_success;
}

bool MCWidgetBase::DispatchRestricted(MCNameRef p_event, MCValueRef *x_args, uindex_t p_arg_count, MCValueRef *r_result)
{
    MCEngineScriptObjectPreventAccess();
    
	// Invoke event handler.
	bool t_success;
	t_success = DoDispatch(p_event, x_args, p_arg_count, r_result);
	
    MCEngineScriptObjectAllowAccess();
	
	if (!t_success)
	{
		MCWidget *t_host;
		t_host = GetHost();
		if (t_host != nil)
			t_host -> SendError();
		else
		{
			MCAutoErrorRef t_error;
			MCErrorCatch(&t_error);
		}
	}
	
	return t_success;
}

void MCWidgetBase::DispatchRestrictedNoThrow(MCNameRef p_event, MCValueRef *x_args, uindex_t p_arg_count, MCValueRef *r_result)
{
	MCEngineScriptObjectPreventAccess();
	
	// Invoke event handler.
	bool t_success;
	t_success = DoDispatch(p_event, x_args, p_arg_count, r_result);
	
	MCEngineScriptObjectAllowAccess();
	
	if (!t_success)
	{
		MCAutoErrorRef t_error;
		MCErrorCatch(&t_error);
	}
}

bool MCWidgetBase::DispatchRecursive(DispatchOrder p_order, MCNameRef p_event, MCValueRef *x_args, uindex_t p_arg_count, MCValueRef *r_result)
{
    bool t_success;
    t_success = true;
    if (p_order == kDispatchOrderBeforeBottomUp ||
        p_order == kDispatchOrderBeforeTopDown)
    {
        if (!Dispatch(p_event, x_args, p_arg_count, r_result))
            t_success = false;
    }
    
    if (m_children != nil)
    {
        if (p_order == kDispatchOrderBeforeBottomUp ||
            p_order == kDispatchOrderBottomUpAfter ||
            p_order == kDispatchOrderBottomUp)
        {
            for(uindex_t i = 0; i < MCProperListGetLength(m_children); i++)
            {
                MCWidgetRef t_child;
                t_child = MCProperListFetchElementAtIndex(m_children, i);
                if (!MCWidgetAsBase(t_child) -> DispatchRecursive(p_order, p_event, x_args, p_arg_count, r_result))
                    t_success = false;
            }
        }
        else if (p_order == kDispatchOrderBeforeTopDown ||
                 p_order == kDispatchOrderTopDownAfter ||
                 p_order == kDispatchOrderTopDown)
        {
            for(uindex_t i = MCProperListGetLength(m_children); i > 0; i--)
            {
                MCWidgetRef t_child;
                t_child = MCProperListFetchElementAtIndex(m_children, i - 1);
                if (!MCWidgetAsBase(t_child) -> DispatchRecursive(p_order, p_event, x_args, p_arg_count, r_result))
                    t_success = false;
            }
        }
    }
    
    if (p_order == kDispatchOrderBottomUpAfter ||
        p_order == kDispatchOrderTopDownAfter)
    {
        if (!Dispatch(p_event, x_args, p_arg_count, r_result))
            t_success = false;
    }
    
    return t_success;
}

bool MCWidgetBase::DispatchBubbly(MCNameRef p_event, MCValueRef *x_args, uindex_t p_arg_count, bool& r_bubble)
{
    if (!HasHandler(p_event))
    {
        r_bubble = true;
        return true;
    }
    
    MCAutoValueRef t_result;
    if (!Dispatch(p_event, x_args, p_arg_count, &(&t_result)))
        return false;
    
    r_bubble = (*t_result == kMCTrue);
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MCWidgetRoot::MCWidgetRoot(MCWidget *p_host)
{
    m_host = p_host;
}

MCWidgetRoot::~MCWidgetRoot(void)
{
}

bool MCWidgetRoot::IsRoot(void) const
{
    return true;
}

MCWidget *MCWidgetRoot::GetHost(void) const
{
	if (!m_host.IsValid())
	{
		return nullptr;
	}

    return m_host;
}

MCWidgetRef MCWidgetRoot::GetOwner(void) const
{
    return nil;
}

MCGRectangle MCWidgetRoot::GetFrame(void) const
{
    return MCRectangleToMCGRectangle(m_host -> getrect());
}

bool MCWidgetRoot::GetDisabled(void) const
{
    return m_host -> isdisabled();
}

bool MCWidgetRoot::CopyFont(MCFontRef& r_font)
{
    MCFontRef t_font;
    m_host -> copyfont(t_font);
    if (t_font == nil)
        return false;
    r_font = t_font;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MCWidgetChild::MCWidgetChild(void)
{
    m_frame = MCGRectangleMake(0.0f, 0.0f, 0.0f, 0.0f);
    m_owner = nil;
    m_disabled = false;
}

MCWidgetChild::~MCWidgetChild(void)
{
}

void MCWidgetChild::SetOwner(MCWidgetRef p_owner)
{
    if (p_owner == m_owner)
        return;
    
    m_owner = p_owner;
}

bool MCWidgetChild::SetFrame(MCGRectangle p_frame)
{
    if (MCGRectangleIsEqual(p_frame, m_frame))
        return true;
    
    m_frame = p_frame;
    
    return OnGeometryChanged();
}

bool MCWidgetChild::SetDisabled(bool p_disabled)
{
    if (m_disabled == p_disabled)
        return true;
    
    m_disabled = p_disabled;
    return OnParentPropertyChanged();
}

bool MCWidgetChild::IsRoot(void) const
{
    return false;
}

MCWidget *MCWidgetChild::GetHost(void) const
{
    if (m_owner != nil)
        return MCWidgetGetHost(m_owner);
    return nil;
}

MCWidgetRef MCWidgetChild::GetOwner(void) const
{
    return m_owner;
}

MCGRectangle MCWidgetChild::GetFrame(void) const
{
    return m_frame;
}

bool MCWidgetChild::GetDisabled(void) const
{
    return m_disabled;
}

bool MCWidgetChild::CopyFont(MCFontRef& r_font)
{
    return false;
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
    if (!MCValueCreateCustom(kMCWidgetTypeInfo, sizeof(MCWidgetChild), t_widget))
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

MCNameRef MCWidgetGetKind(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> GetKind();
}

bool MCWidgetIsRoot(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> IsRoot();
}

bool MCWidgetIsAncestorOf(MCWidgetRef self, MCWidgetRef p_child)
{
    for(;;)
    {
        if (self == p_child)
            return true;
        
        p_child = MCWidgetGetOwner(p_child);
        if (p_child == nil)
            break;
    }
    
    return false;
}

MCWidget *MCWidgetGetHost(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> GetHost();
}

MCWidgetRef MCWidgetGetOwner(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> GetOwner();
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

bool MCWidgetQueryProperty(MCWidgetRef self, MCNameRef p_property, MCTypeInfoRef& r_getter, MCTypeInfoRef& r_setter)
{
    return MCWidgetAsBase(self) -> QueryProperty(p_property, r_getter, r_setter);
}

bool MCWidgetSetProperty(MCWidgetRef self, MCNameRef p_property, MCValueRef p_value)
{
    return MCWidgetAsBase(self) -> SetProperty(p_property, p_value);
}

bool MCWidgetGetProperty(MCWidgetRef self, MCNameRef p_property, MCValueRef& r_value)
{
    return MCWidgetAsBase(self) -> GetProperty(p_property, r_value);
}

bool MCWidgetHasPropertyOfChunk(MCWidgetRef self, MCNameRef p_property, MCNameRef p_chunk_name, bool p_getter)
{
    return MCWidgetAsBase(self) -> HasPropertyOfChunk(p_property, p_chunk_name, p_getter);
}

bool MCWidgetQueryPropertyOfChunk(MCWidgetRef self, MCNameRef p_property, MCNameRef p_chunk_name, bool p_getter, MCTypeInfoRef& r_type_info)
{
    MCNewAutoNameRef t_handler;
    if (!chunk_property_handler(p_property, p_chunk_name, p_getter, &t_handler))
        return false;
    
    return MCWidgetAsBase(self) -> QueryPropertyOfChunk(p_property, p_chunk_name, p_getter, r_type_info);
}

bool MCWidgetSetPropertyOfChunk(MCWidgetRef self, MCNameRef p_property, MCNameRef p_chunk_name, MCProperListRef p_path, MCValueRef p_value)
{
    return MCWidgetAsBase(self) -> SetPropertyOfChunk(p_property, p_chunk_name, p_path, p_value);
}

bool MCWidgetGetPropertyOfChunk(MCWidgetRef self, MCNameRef p_property, MCNameRef p_chunk_name, MCProperListRef p_path, MCValueRef& r_value)
{
    return MCWidgetAsBase(self) -> GetPropertyOfChunk(p_property, p_chunk_name, p_path, r_value);
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

bool MCWidgetOnTimer(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnTimer();
}

bool MCWidgetOnPaint(MCWidgetRef self, MCGContextRef p_gcontext)
{
    return MCWidgetAsBase(self) -> OnPaint(p_gcontext);
}

bool MCWidgetOnHitTest(MCWidgetRef self, MCGPoint p_location, MCWidgetRef& r_target)
{
    return MCWidgetAsBase(self) -> OnHitTest(p_location, r_target);
}

bool MCWidgetOnMouseEnter(MCWidgetRef self, bool& r_bubble)
{
    return MCWidgetAsBase(self) -> OnMouseEnter(r_bubble);
}

bool MCWidgetOnMouseLeave(MCWidgetRef self, bool& r_bubble)
{
    return MCWidgetAsBase(self) -> OnMouseLeave(r_bubble);
}

bool MCWidgetOnMouseMove(MCWidgetRef self, bool& r_bubble)
{
    return MCWidgetAsBase(self) -> OnMouseMove(r_bubble);
}

bool MCWidgetOnMouseDown(MCWidgetRef self, bool& r_bubble)
{
    return MCWidgetAsBase(self) -> OnMouseDown(r_bubble);
}

bool MCWidgetOnMouseUp(MCWidgetRef self, bool& r_bubble)
{
    return MCWidgetAsBase(self) -> OnMouseUp(r_bubble);
}

bool MCWidgetOnMouseCancel(MCWidgetRef self, bool& r_bubble)
{
    return MCWidgetAsBase(self) -> OnMouseCancel(r_bubble);
}

bool MCWidgetOnClick(MCWidgetRef self, bool& r_bubble)
{
    return MCWidgetAsBase(self) -> OnClick(r_bubble);
}

bool MCWidgetOnMouseScroll(MCWidgetRef self, real32_t p_delta_x, real32_t p_delta_y, bool& r_bubble)
{
    return MCWidgetAsBase(self) -> OnMouseScroll(p_delta_x, p_delta_y, r_bubble);
}

/* TOUCH MESSAGE METHODS */

bool MCWidgetHandlesTouchEvents(MCWidgetRef self)
{
    return MCWidgetAsBase(self)->HandlesTouchEvents();
}

bool MCWidgetOnTouchStart(MCWidgetRef self, bool& r_bubble)
{
    return MCWidgetAsBase(self)->OnTouchStart(r_bubble);
}

bool MCWidgetOnTouchMove(MCWidgetRef self, bool& r_bubble)
{
    return MCWidgetAsBase(self)->OnTouchMove(r_bubble);
}

bool MCWidgetOnTouchFinish(MCWidgetRef self, bool& r_bubble)
{
    return MCWidgetAsBase(self)->OnTouchFinish(r_bubble);
}

bool MCWidgetOnTouchCancel(MCWidgetRef self, bool& r_bubble)
{
    return MCWidgetAsBase(self)->OnTouchCancel(r_bubble);
}

bool MCWidgetOnGeometryChanged(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnGeometryChanged();
}

bool MCWidgetOnLayerChanged(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnLayerChanged();
}

bool MCWidgetOnVisibilityChanged(MCWidgetRef self, bool p_visible)
{
    return MCWidgetAsBase(self) -> OnVisibilityChanged(p_visible);
}

bool MCWidgetOnParentPropertyChanged(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> OnParentPropertyChanged();
}

bool MCWidgetOnToolChanged(MCWidgetRef self, Tool p_tool)
{
    return MCWidgetAsBase(self) -> OnToolChanged(p_tool);
}

bool MCWidgetCopyAnnotation(MCWidgetRef self, MCNameRef p_annotation, MCValueRef& r_value)
{
    return MCWidgetAsBase(self) -> CopyAnnotation(p_annotation, r_value);
}

bool MCWidgetSetAnnotation(MCWidgetRef self, MCNameRef p_annotation, MCValueRef p_value)
{
    return MCWidgetAsBase(self) -> SetAnnotation(p_annotation, p_value);
}

void MCWidgetRedrawAll(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> RedrawRect(nil);
}

void MCWidgetTriggerAll(MCWidgetRef self)
{
    return MCWidgetAsBase(self) -> TriggerAll();
}

bool MCWidgetPost(MCWidgetRef self, MCNameRef p_event, MCProperListRef p_args)
{
    return MCWidgetAsBase(self) -> Post(p_event, p_args);
}

void MCWidgetScheduleTimerIn(MCWidgetRef self, double p_timeout)
{
    MCWidgetAsBase(self) -> ScheduleTimerIn(p_timeout);
}

void MCWidgetCancelTimer(MCWidgetRef self)
{
    MCWidgetAsBase(self) -> CancelTimer();
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

//////////

bool MCChildWidgetSetFrame(MCWidgetRef p_widget, MCGRectangle p_frame)
{
    return MCWidgetAsChild(p_widget) -> SetFrame(p_frame);
}

bool MCChildWidgetSetDisabled(MCWidgetRef p_widget, bool p_disabled)
{
    return MCWidgetAsChild(p_widget) -> SetDisabled(p_disabled);
}

////////////////////////////////////////////////////////////////////////////////

MCWidgetBase *MCWidgetAsBase(MCWidgetRef self)
{
    MCAssert(self != nil);
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

void *MCWidgetEnter(MCScriptInstanceRef p_instance, void *p_host_ptr)
{
    void *t_cookie = MCcurrentwidget;
    
    auto t_widget = static_cast<MCWidgetRef>(p_host_ptr);
    
	// Make sure the host object's 'scriptdepth' is incremented and
	// decremented appropriately. This prevents script from deleting
	// a widget which has its LCB code on the stack (just as it would if
	// it had its LCS code on the stack).
    MCWidget *t_host = MCWidgetAsBase(t_widget)->GetHost();
    if (t_host != nullptr)
        t_host->lockforexecution();
    
    // Update the current widget context
    MCcurrentwidget = t_widget;
    
    return t_cookie;
}

void MCWidgetLeave(MCScriptInstanceRef p_instance, void *p_host_ptr, void *p_cookie)
{
    auto t_old_widget = static_cast<MCWidgetRef>(p_cookie);
    MCcurrentwidget = t_old_widget;

    auto t_widget = static_cast<MCWidgetRef>(p_host_ptr);
    
    MCWidget *t_host = MCWidgetAsBase(t_widget)->GetHost();
    if (t_host != nullptr)
        t_host->unlockforexecution();
}

////////////////////////////////////////////////////////////////////////////////
