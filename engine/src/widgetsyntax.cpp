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

bool MCWidgetThrowNoCurrentWidgetError(void)
{
	return MCErrorCreateAndThrow(kMCWidgetNoCurrentWidgetErrorTypeInfo, nil);
}

bool MCWidgetThrowNotSupportedInChildWidgetError(void)
{
	return MCErrorCreateAndThrow(kMCWidgetNoCurrentWidgetErrorTypeInfo, nil);
}

bool MCWidgetEnsureCurrentWidget(void)
{
    if (MCcurrentwidget == nil)
        return MCWidgetThrowNoCurrentWidgetError();
    
    return true;
}

bool MCWidgetEnsureCurrentWidgetIsRoot(void)
{
    if (!MCWidgetEnsureCurrentWidget())
        return false;
    
    if (!MCWidgetIsRoot(MCcurrentwidget))
        return MCWidgetThrowNotSupportedInChildWidgetError;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void MCWidgetExecRedrawAll(void)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetRedrawAll(MCcurrentwidget);
}

extern "C" MC_DLLEXPORT void MCWidgetExecScheduleTimerIn(double p_after)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetScheduleTimerIn(MCcurrentwidget, p_after);
}

extern "C" MC_DLLEXPORT void MCWidgetExecCancelTimer(void)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetCancelTimer(MCcurrentwidget);
}

extern "C" MC_DLLEXPORT void MCWidgetEvalInEditMode(bool& r_in_edit_mode)
{
    r_in_edit_mode = MCcurtool != T_BROWSE;
}

////////////////////////////////////////////////////////////////////////////////

extern MCValueRef MCEngineDoSendToObjectWithArguments(bool p_is_function, MCStringRef p_message, MCObject *p_object, MCProperListRef p_arguments);
extern void MCEngineDoPostToObjectWithArguments(MCStringRef p_message, MCObject *p_object, MCProperListRef p_arguments);

extern "C" MC_DLLEXPORT void MCWidgetGetScriptObject(MCScriptObjectRef& r_script_object)
{
    if (!MCWidgetEnsureCurrentWidgetIsRoot())
        return;
    
    if (!MCEngineScriptObjectCreate(MCWidgetGetHost(MCcurrentwidget), 0, r_script_object))
        return;
}

extern "C" MC_DLLEXPORT MCValueRef MCWidgetExecSendWithArguments(bool p_is_function, MCStringRef p_message, MCProperListRef p_arguments)
{
    if (!MCWidgetEnsureCurrentWidgetIsRoot())
        return nil;
    
    return MCEngineDoSendToObjectWithArguments(p_is_function, p_message, MCWidgetGetHost(MCcurrentwidget), p_arguments);
}

extern "C" MC_DLLEXPORT MCValueRef MCWidgetExecSend(bool p_is_function, MCStringRef p_message)
{
    return MCWidgetExecSendWithArguments(p_is_function, p_message, kMCEmptyProperList);
}

extern "C" MC_DLLEXPORT void MCWidgetExecPostWithArguments(MCStringRef p_message, MCProperListRef p_arguments)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCEngineDoPostToObjectWithArguments(p_message, MCWidgetGetHost(MCcurrentwidget), p_arguments);
}

extern "C" MC_DLLEXPORT void MCWidgetExecPost(MCStringRef p_message)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetExecPostWithArguments(p_message, kMCEmptyProperList);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void MCWidgetGetBounds(MCCanvasRectangleRef& r_rect)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCGRectangle t_frame;
    t_frame = MCWidgetGetFrame(MCcurrentwidget);
    MCCanvasRectangleCreateWithMCGRectangle(MCGRectangleMake(0.0f, 0.0f, t_frame . size . width, t_frame . size . height), r_rect);
}

extern "C" MC_DLLEXPORT void MCWidgetGetMyWidth(MCNumberRef& r_width)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCNumberCreateWithReal(MCWidgetGetFrame(MCcurrentwidget) . size . width, r_width);
}

extern "C" MC_DLLEXPORT void MCWidgetGetMyHeight(MCNumberRef& r_height)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCNumberCreateWithReal(MCWidgetGetFrame(MCcurrentwidget) . size . width, r_height);
}

extern "C" MC_DLLEXPORT void MCWidgetGetMyFont(MCCanvasFontRef& r_canvas_font)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCAutoCustomPointer<struct MCFont, MCFontRelease> t_font;
    if (!MCWidgetCopyFont(MCcurrentwidget, &t_font))
        return;
    
    if (!MCCanvasFontCreateWithMCFont(*t_font, r_canvas_font))
        return;
}

extern "C" MC_DLLEXPORT void MCWidgetGetMyEnabled(bool& r_enabled)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    r_enabled = !MCWidgetGetDisabled(MCcurrentwidget);
}

extern "C" MC_DLLEXPORT void MCWidgetGetDisabled(bool& r_disabled)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    r_disabled = MCWidgetGetDisabled(MCcurrentwidget);
}

extern "C" MC_DLLEXPORT void MCWidgetGetMousePosition(bool p_current, MCCanvasPointRef& r_point)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    // TODO - coordinate transform
    
    coord_t t_x, t_y;
    if (p_current)
        MCwidgeteventmanager->GetAsynchronousMousePosition(t_x, t_y);
    else
        MCwidgeteventmanager->GetSynchronousMousePosition(t_x, t_y);
    
    MCGPoint t_gpoint;
    t_gpoint = MCGPointMake(t_x, t_y);
    /* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(MCWidgetMapPointFromGlobal(MCcurrentwidget, t_gpoint), r_point);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void MCWidgetGetClickPosition(bool p_current, MCCanvasPointRef& r_point)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    // TODO - coordinate transforms
    
    coord_t t_x, t_y;
    if (p_current)
        MCwidgeteventmanager->GetAsynchronousClickPosition(t_x, t_y);
    else
        MCwidgeteventmanager->GetSynchronousClickPosition(t_x, t_y);
    
    MCGPoint t_gpoint;
    t_gpoint = MCGPointMake(t_x, t_y);
    /* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(MCWidgetMapPointFromGlobal(MCcurrentwidget, t_gpoint), r_point);
}

extern "C" MC_DLLEXPORT void MCWidgetGetClickButton(bool p_current, unsigned int& r_button)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    // TODO: Implement asynchronous version.
    if (!p_current)
        MCwidgeteventmanager -> GetSynchronousClickButton(r_button);
    else
        MCErrorThrowGeneric(MCSTR("'the current click button' is not implemented yet"));
}

extern "C" MC_DLLEXPORT void MCWidgetGetClickCount(bool p_current, unsigned int& r_count)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    // TODO: Implement asynchronous version.
    if (!p_current)
        MCwidgeteventmanager -> GetSynchronousClickCount(r_count);
    else
        MCErrorThrowGeneric(MCSTR("'the current click count' is not implemented yet"));
}

////////////////////////////////////////////////////////////////////////////////

typedef struct __MCPressedState* MCPressedStateRef;
MCTypeInfoRef kMCPressedState;

extern "C" MC_DLLEXPORT void MCWidgetGetMouseButtonState(uinteger_t p_index, MCPressedStateRef r_state)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    // TODO: implement
    MCAssert(false);
}

////////////////////////////////////////////////////////////////////////////////

void MCWidgetEvalMyChildren(MCProperListRef& r_children)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetCopyChildren(MCcurrentwidget, r_children);
}

void MCWidgetEvalANewWidget(MCStringRef p_kind, MCWidgetRef& r_widget)
{
    
}

void MCWidgetExecPlaceWidget(MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetPlaceWidget(MCcurrentwidget, p_widget, nil, false);
}

void MCWidgetExecPlaceWidgetAt(MCWidgetRef p_widget, bool p_at_bottom)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetPlaceWidget(MCcurrentwidget, p_widget, nil, p_at_bottom);
}

void MCWidgetExecPlaceWidgetRelative(MCWidgetRef p_widget, bool p_is_below, MCWidgetRef p_other_widget)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetPlaceWidget(MCcurrentwidget, p_widget, p_other_widget, p_is_below);
}

void MCWidgetExecUnplaceWidget(MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetUnplaceWidget(MCcurrentwidget, p_widget);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void MCWidgetEvalIsPointWithinRect(MCCanvasPointRef p_point, MCCanvasRectangleRef p_rect, bool& r_within)
{
    MCGPoint t_p;
    MCGRectangle t_r;
    MCCanvasPointGetMCGPoint(p_point, t_p);
    MCCanvasRectangleGetMCGRectangle(p_rect, t_r);
    
    r_within = (t_r.origin.x <= t_p.x && t_p.x < t_r.origin.x+t_r.size.width)
    && (t_r.origin.y <= t_p.y && t_p.y < t_r.origin.y+t_r.size.height);
}

extern "C" MC_DLLEXPORT void MCWidgetEvalIsPointNotWithinRect(MCCanvasPointRef p_point, MCCanvasRectangleRef p_rect, bool& r_not_within)
{
    bool t_within;
    MCWidgetEvalIsPointWithinRect(p_point, p_rect, t_within);
    r_not_within = !t_within;
}

////////////////////////////////////////////////////////////////////////////////

static void __MCWidgetDestroy(MCValueRef p_value)
{
    MCWidgetBase *t_widget;
    t_widget = (MCWidgetBase *)MCValueGetExtraBytesPtr(p_value);
    t_widget -> Destroy();
    t_widget -> ~MCWidgetBase();
}

static bool __MCWidgetCopy(MCValueRef p_value, bool p_release, MCValueRef& r_new_value)
{
    if (p_release)
        r_new_value = p_value;
    else
        r_new_value = MCValueRetain(p_value);
    
    return true;
}

static bool __MCWidgetEqual(MCValueRef p_left, MCValueRef p_right)
{
    if (p_left != p_right)
        return false;
    return true;
}

static hash_t __MCWidgetHash(MCValueRef p_value)
{
    return MCHashPointer(p_value);
}

static bool __MCWidgetDescribe(MCValueRef p_value, MCStringRef& r_desc)
{
    return MCStringFormat(r_desc, "<widget>");
}

static MCValueCustomCallbacks kMCWidgetCustomValueCallbacks =
{
    false,
    __MCWidgetDestroy,
    __MCWidgetCopy,
    __MCWidgetEqual,
    __MCWidgetHash,
    __MCWidgetDescribe,
};

////////////////////////////////////////////////////////////////////////////////

MCTypeInfoRef kMCWidgetNoCurrentWidgetErrorTypeInfo = nil;
MCTypeInfoRef kMCWidgetSizeFormatErrorTypeInfo = nil;
MCTypeInfoRef kMCWidgetTypeInfo = nil;

bool MCWidgetModuleInitialize(void)
{
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.widget.NoCurrentWidgetError"), MCNAME("widget"), MCSTR("No current widget."), kMCWidgetNoCurrentWidgetErrorTypeInfo))
		return false;
	
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.widget.WidgetSizeFormatError"), MCNAME("widget"), MCSTR("Size must be a list of two numbers"), kMCWidgetSizeFormatErrorTypeInfo))
		return false;
	
    if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.widget.Widget"), kMCNullTypeInfo, &kMCWidgetCustomValueCallbacks, kMCWidgetTypeInfo))
        return false;
    
	return true;
}

void MCWidgetModuleFinalize(void)
{
    MCValueRelease(kMCWidgetTypeInfo);
    kMCWidgetTypeInfo = nil;
    
	MCValueRelease(kMCWidgetNoCurrentWidgetErrorTypeInfo);
	kMCWidgetNoCurrentWidgetErrorTypeInfo = nil;
	
	MCValueRelease(kMCWidgetSizeFormatErrorTypeInfo);
	kMCWidgetSizeFormatErrorTypeInfo = nil;
}

////////////////////////////////////////////////////////////////////////////////

