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
#include "native-layer.h"

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

bool MCWidgetThrowNotAChildOfThisWidgetError(void)
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
        return MCWidgetThrowNotSupportedInChildWidgetError();
    
    return true;
}

bool MCWidgetEnsureCanManipulateWidget(MCWidgetRef p_other_widget)
{
    MCWidgetRef t_owner;
    t_owner = MCWidgetGetOwner(p_other_widget);
    
    if (t_owner != nil &&
        t_owner != MCcurrentwidget)
        return MCWidgetThrowNotAChildOfThisWidgetError();
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCWidgetExecRedrawAll(void)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetRedrawAll(MCcurrentwidget);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetExecScheduleTimerIn(double p_after)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetScheduleTimerIn(MCcurrentwidget, p_after);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetExecCancelTimer(void)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetCancelTimer(MCcurrentwidget);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetEvalInEditMode(bool& r_in_edit_mode)
{
    r_in_edit_mode = MCcurtool != T_BROWSE;
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetExecTriggerAll(void)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetTriggerAll(MCcurrentwidget);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetExecTriggerAllInWidget(MCWidgetRef p_widget)
{
	MCWidgetTriggerAll(p_widget);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyScriptObject(MCScriptObjectRef& r_script_object)
{
    if (!MCWidgetEnsureCurrentWidgetIsRoot())
        return;
    
    if (!MCEngineScriptObjectCreate(MCWidgetGetHost(MCcurrentwidget), 0, r_script_object))
        return;
}

// This should only be called internally when MCcurrentwidget is known
// to be a non-root widget.
void MCWidgetExecPostToParentWithArguments(MCStringRef p_message, MCProperListRef p_arguments)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCAssert(!MCWidgetIsRoot(MCcurrentwidget));
    
    MCAutoStringRef t_modified_name;
    if (!MCStringFormat(&t_modified_name, "On%@", p_message))
        return;
    
    MCNewAutoNameRef t_message;
    if (!MCNameCreate(*t_modified_name, &t_message))
        return;
    
    // Note: At the moment 'post' isn't really a post in this case - it calls
    //   the handler immediately.
    MCWidgetRef t_old_target;
    t_old_target = MCwidgeteventmanager -> SetTargetWidget(MCcurrentwidget);
    MCWidgetPost(MCWidgetGetOwner(MCcurrentwidget), *t_message, p_arguments);
    MCwidgeteventmanager -> SetTargetWidget(t_old_target);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyName(MCStringRef& r_name)
{
	if (!MCWidgetEnsureCurrentWidget())
		return;
	
	r_name = MCValueRetain(MCNameGetString(MCWidgetGetHost(MCcurrentwidget) -> getname()));
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyBounds(MCCanvasRectangleRef& r_rect)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCGRectangle t_frame;
    t_frame = MCWidgetGetFrame(MCcurrentwidget);
    MCCanvasRectangleCreateWithMCGRectangle(MCGRectangleMake(0.0f, 0.0f, t_frame . size . width, t_frame . size . height), r_rect);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyWidth(MCNumberRef& r_width)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCNumberCreateWithReal(MCWidgetGetFrame(MCcurrentwidget) . size . width, r_width);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyHeight(MCNumberRef& r_height)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCNumberCreateWithReal(MCWidgetGetFrame(MCcurrentwidget) . size . height, r_height);
}

// AL-2015-08-27: [[ Bug 15773 ]] Reinstate MCWidgetGetMyRectangle
extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyRectangle(MCCanvasRectangleRef& r_rect)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCCanvasRectangleCreateWithMCGRectangle(MCWidgetGetFrame(MCcurrentwidget), r_rect);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyFont(MCCanvasFontRef& r_canvas_font)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCAutoCustomPointer<struct MCFont, MCFontRelease> t_font;
    if (!MCWidgetCopyFont(MCcurrentwidget, &t_font))
        return;
    
    if (!MCCanvasFontCreateWithMCFont(*t_font, r_canvas_font))
        return;
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyEnabled(bool& r_enabled)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    r_enabled = !MCWidgetGetDisabled(MCcurrentwidget);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyDisabled(bool& r_disabled)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    r_disabled = MCWidgetGetDisabled(MCcurrentwidget);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyPaint(uinteger_t p_type, MCCanvasPaintRef& r_paint)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidget *t_host;
    t_host = MCWidgetGetHost(MCcurrentwidget);
    
    // If getforecolor() returns true then we have a solid color.
    MCColor t_color;
    MCPatternRef t_pattern = nil;
    int2 x, y;
    if (t_host -> getforecolor((uint2)p_type,
                               False,
                               False,
                               t_color,
                               t_pattern,
                               x, y,
                               CONTEXT_TYPE_SCREEN,
                               t_host,
                               False))
    {
        MCAutoValueRefBase<MCCanvasColorRef> t_canvas_color;
        if (!MCCanvasColorCreateWithRGBA(t_color . red / 65535.0f,
                                         t_color . green / 65535.0f,
                                         t_color . blue / 65535.0f,
                                         1.0f,
                                         &t_canvas_color))
            return;
        
        MCCanvasSolidPaintMakeWithColor(*t_canvas_color,
                                        (MCCanvasSolidPaintRef&)r_paint);
        return;
    }
    
    // If t_pattern is not nil we have a pattern, offset by
    // (x, y).
    if (t_pattern != nil)
    {
        // We have to adjust the x, y offset here as it is in card
        // co-ordinates, but canvas's operate in object co-ordinates.
        MCAutoValueRefBase<MCCanvasTransformRef> t_canvas_transform;
        if (!MCCanvasTransformCreateWithMCGAffineTransform(MCGAffineTransformPreTranslate(MCPatternGetTransform(t_pattern),
                                                                                          x - t_host -> getrect() . x,
                                                                                          y - t_host -> getrect() . y),
                                                           &t_canvas_transform))
            return;
        
        MCAutoValueRefBase<MCCanvasImageRef> t_canvas_image;
        if (!MCCanvasImageCreateWithImageRep(MCPatternGetSource(t_pattern),
                                             &t_canvas_image))
            return;
        
        MCCanvasPatternCreateWithImage(*t_canvas_image,
                                       *t_canvas_transform,
                                       (MCCanvasPatternRef&)r_paint);
        return;
    }

    // In all other cases just return solid black.
    MCCanvasSolidPaintMakeWithColor(kMCCanvasColorBlack, (MCCanvasSolidPaintRef&)r_paint);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyPixelScale(MCCanvasFloat& r_pixel_scale)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    r_pixel_scale = MCWidgetGetHost(MCcurrentwidget)->getstack()->view_getbackingscale();
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMousePosition(bool p_current, MCCanvasPointRef& r_point)
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

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetClickPosition(bool p_current, MCCanvasPointRef& r_point)
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

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetClickButton(bool p_current, unsigned int& r_button)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    // TODO: Implement asynchronous version.
    if (!p_current)
        MCwidgeteventmanager -> GetSynchronousClickButton(r_button);
    else
        MCErrorThrowGeneric(MCSTR("'the current click button' is not implemented yet"));
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetClickCount(bool p_current, unsigned int& r_count)
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

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetTouchId(MCValueRef& r_id)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    integer_t t_id;
    if (!MCwidgeteventmanager->GetActiveTouch(t_id))
    {
        r_id = MCValueRetain(kMCNull);
        return;
    }
    
    MCNumberCreateWithInteger(t_id, (MCNumberRef&)r_id);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetTouchPosition(MCValueRef& r_point)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    integer_t t_id;
    MCPoint t_position;
    if (!MCwidgeteventmanager->GetActiveTouch(t_id) ||
        !MCwidgeteventmanager->GetTouchPosition(t_id, t_position))
    {
        r_point = MCValueRetain(kMCNull);
        return;
    }

    /* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(MCWidgetMapPointFromGlobal(MCcurrentwidget, MCPointToMCGPoint(t_position)), (MCCanvasPointRef&)r_point);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetNumberOfTouches(uinteger_t& r_count)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    r_count = MCwidgeteventmanager->GetTouchCount();
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetPositionOfTouch(integer_t p_id, MCValueRef& r_point)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCPoint t_position;
    if (!MCwidgeteventmanager->GetTouchPosition(p_id, t_position))
    {
        r_point = MCValueRetain(kMCNull);
        return;
    }
    
    /* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(MCWidgetMapPointFromGlobal(MCcurrentwidget, MCPointToMCGPoint(t_position)), (MCCanvasPointRef&)r_point);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetTouchIDs(MCValueRef& r_touch_ids)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCAutoProperListRef t_touch_ids;
    if (!MCwidgeteventmanager->GetTouchIDs(&t_touch_ids) ||
        MCProperListIsEmpty(*t_touch_ids))
    {
        r_touch_ids = MCValueRetain(kMCNull);
        return;
    }
    
    r_touch_ids = t_touch_ids.Take();
}

////////////////////////////////////////////////////////////////////////////////

typedef struct __MCPressedState* MCPressedStateRef;
MCTypeInfoRef kMCPressedState;

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMouseButtonState(uinteger_t p_index, MCPressedStateRef r_state)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    // TODO: implement
    MCAssert(false);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCWidgetEvalThisWidget(MCWidgetRef& r_widget)
{
	if (!MCWidgetEnsureCurrentWidget())
		return;
	
	r_widget = MCValueRetain(MCcurrentwidget);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetEvalTheTarget(MCWidgetRef& r_widget)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetRef t_target;
    t_target = MCwidgeteventmanager -> GetTargetWidget();
    if (t_target != nil)
        MCValueRetain(t_target);
    r_widget = t_target;
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetEvalMyChildren(MCProperListRef& r_children)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetCopyChildren(MCcurrentwidget, r_children);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetEvalANewWidget(MCStringRef p_kind, MCWidgetRef& r_widget)
{
    MCNewAutoNameRef t_kind;
    if (!MCNameCreate(p_kind, &t_kind))
        return;
    MCWidgetCreateChild(*t_kind, r_widget);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetExecPlaceWidget(MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetPlaceWidget(MCcurrentwidget, p_widget, nil, false);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetExecPlaceWidgetAt(MCWidgetRef p_widget, bool p_at_bottom)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetPlaceWidget(MCcurrentwidget, p_widget, nil, p_at_bottom);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetExecPlaceWidgetRelative(MCWidgetRef p_widget, bool p_is_below, MCWidgetRef p_other_widget)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetPlaceWidget(MCcurrentwidget, p_widget, p_other_widget, p_is_below);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetExecUnplaceWidget(MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
    
    MCWidgetUnplaceWidget(MCcurrentwidget, p_widget);
}

//////////

#if WIDGET_LCB_SNAPSHOT
extern "C" bool MCProperListFetchAsArrayOfFloat(MCProperListRef p_list, uindex_t p_size, float32_t *x_floats);

static MCCanvasImageRef MCWidgetExecSnapshotWidgetAtSize(MCWidgetRef p_widget, MCGSize p_size)
{
    MCGContextRef t_gcontext;
    if (!MCGContextCreate(ceilf(p_size . width), ceilf(p_size . height), true, t_gcontext))
        return nil;
    
    MCGRectangle t_frame;
    t_frame = MCWidgetGetFrame(p_widget);
    
    MCGContextScaleCTM(t_gcontext, p_size . width / t_frame . size . width, p_size . height / t_frame . size . height);
    MCGContextTranslateCTM(t_gcontext, -t_frame . origin . x, -t_frame . origin . y);
    
    MCGImageRef t_gimage;
    MCImageRep *t_imagerep;
    MCCanvasImageRef t_image;
    t_gimage = nil;
    t_imagerep = nil;
    if (!MCWidgetOnPaint(p_widget, t_gcontext) ||
        !MCGContextCopyImage(t_gcontext, t_gimage) ||
        !MCImageRepCreateWithGImage(t_gimage, t_imagerep) ||
        !MCCanvasImageCreateWithImageRep(t_imagerep, t_image))
        t_image = nil;
    
    MCGContextRelease(t_gcontext);
    if (t_imagerep != nil)
        t_imagerep -> Release();
    if (t_gimage != nil)
        MCGImageRelease(t_gimage);

    return t_image;
}

extern "C" MC_DLLEXPORT_DEF MCCanvasImageRef MCWidgetExecSnapshotWidgetAtSizeAsList(MCWidgetRef p_widget, MCProperListRef p_size)
{
    float32_t t_size[2];
    if (!MCProperListFetchAsArrayOfFloat(p_size, 2, t_size))
	{
		MCCanvasThrowError(kMCCanvasImageSizeListFormatErrorTypeInfo);
		return nil;
	}
    
    return MCWidgetExecSnapshotWidgetAtSize(p_widget, MCGSizeMake(t_size[0], t_size[1]));
}

extern "C" MC_DLLEXPORT_DEF MCCanvasImageRef MCWidgetExecSnapshotWidget(MCWidgetRef p_widget)
{
    return MCWidgetExecSnapshotWidgetAtSize(p_widget, MCWidgetGetFrame(p_widget) . size);
}
#endif

//////////

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetPropertyOfWidget(MCStringRef p_property, MCWidgetRef p_widget, MCValueRef& r_value)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCNewAutoNameRef t_property;
    if (!MCNameCreate(p_property, &t_property))
        return;
    
    MCWidgetGetProperty(p_widget, *t_property, r_value);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetPropertyOfWidget(MCValueRef p_value, MCStringRef p_property, MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCNewAutoNameRef t_property;
    if (!MCNameCreate(p_property, &t_property))
        return;
    
    MCWidgetSetProperty(p_widget, *t_property, p_value);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetRectangleOfWidget(MCWidgetRef p_widget, MCCanvasRectangleRef& r_value)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCCanvasRectangleCreateWithMCGRectangle(MCWidgetGetFrame(p_widget), r_value);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetRectangleOfWidget(MCCanvasRectangleRef p_rect, MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCGRectangle t_frame;
    MCCanvasRectangleGetMCGRectangle(p_rect, t_frame);
    MCChildWidgetSetFrame(p_widget, t_frame);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetWidthOfWidget(MCWidgetRef p_widget, MCCanvasFloat& r_width)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    r_width = MCWidgetGetFrame(p_widget) . size . width;
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetWidthOfWidget(MCCanvasFloat p_width, MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCGRectangle t_frame;
    t_frame = MCWidgetGetFrame(p_widget);
    t_frame . origin . x = t_frame . origin . x + t_frame . size . width / 2.0f - p_width / 2.0f;
    t_frame . size . width = p_width;
    MCChildWidgetSetFrame(p_widget, t_frame);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetHeightOfWidget(MCWidgetRef p_widget, MCCanvasFloat& r_height)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    r_height = MCWidgetGetFrame(p_widget) . size . height;
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetHeightOfWidget(MCCanvasFloat p_height, MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCGRectangle t_frame;
    t_frame = MCWidgetGetFrame(p_widget);
    t_frame . origin . x = t_frame . origin . y + t_frame . size . height / 2.0f - p_height / 2.0f;
    t_frame . size . height = p_height;
    MCChildWidgetSetFrame(p_widget, t_frame);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetLocationOfWidget(MCWidgetRef p_widget, MCCanvasPointRef& r_location)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCCanvasPointCreateWithMCGPoint(MCGRectangleGetCenter(MCWidgetGetFrame(p_widget)), r_location);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetLocationOfWidget(MCCanvasPointRef p_location, MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCGPoint t_location;
    MCCanvasPointGetMCGPoint(p_location, t_location);
    
    MCGRectangle t_frame;
    t_frame = MCWidgetGetFrame(p_widget);
    t_frame . origin . x = t_location . x - t_frame . size . width / 2.0f;
    t_frame . origin . y = t_location . y - t_frame . size . height / 2.0f;
    MCChildWidgetSetFrame(p_widget, t_frame);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetEnabledOfWidget(MCWidgetRef p_widget, bool& r_enabled)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    r_enabled = !MCWidgetGetDisabled(p_widget);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetEnabledOfWidget(bool p_enabled, MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCChildWidgetSetDisabled(p_widget, !p_enabled);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetDisabledOfWidget(MCWidgetRef p_widget, bool& r_disabled)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    r_disabled = MCWidgetGetDisabled(p_widget);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetDisabledOfWidget(bool p_disabled, MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCChildWidgetSetDisabled(p_widget, p_disabled);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetAnnotationOfWidget(MCStringRef p_annotation, MCWidgetRef p_widget, MCValueRef& r_value)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCNewAutoNameRef t_annotation;
    if (!MCNameCreate(p_annotation, &t_annotation))
        return;
    
    MCWidgetCopyAnnotation(p_widget, *t_annotation, r_value);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetAnnotationOfWidget(MCValueRef p_value, MCStringRef p_annotation, MCWidgetRef p_widget)
{
    if (!MCWidgetEnsureCanManipulateWidget(p_widget))
        return;
    
    MCNewAutoNameRef t_annotation;
    if (!MCNameCreate(p_annotation, &t_annotation))
        return;
    
    MCWidgetSetAnnotation(p_widget, *t_annotation, p_value);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCWidgetEvalIsPointWithinRect(MCCanvasPointRef p_point, MCCanvasRectangleRef p_rect, bool& r_within)
{
    MCGPoint t_p;
    MCGRectangle t_r;
    MCCanvasPointGetMCGPoint(p_point, t_p);
    MCCanvasRectangleGetMCGRectangle(p_rect, t_r);
    
    r_within = (t_r.origin.x <= t_p.x && t_p.x < t_r.origin.x+t_r.size.width)
    && (t_r.origin.y <= t_p.y && t_p.y < t_r.origin.y+t_r.size.height);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetEvalIsPointNotWithinRect(MCCanvasPointRef p_point, MCCanvasRectangleRef p_rect, bool& r_not_within)
{
    bool t_within;
    MCWidgetEvalIsPointWithinRect(p_point, p_rect, t_within);
    r_not_within = !t_within;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetNativeLayerOfWidget(MCWidgetRef p_widget, void *&r_native_layer)
{
	if (!MCWidgetEnsureCanManipulateWidget(p_widget))
		return;
	
	/* UNCHECKED */
	MCWidgetGetHost(p_widget)->GetNativeView(r_native_layer);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetNativeLayerOfWidget(void *p_native_layer, MCWidgetRef p_widget)
{
	if (!MCWidgetEnsureCanManipulateWidget(p_widget))
		return;
	
	/* UNCHECKED */
	MCWidgetGetHost(p_widget)->SetNativeView(p_native_layer);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetNativeLayerCanRenderToContext(MCWidgetRef p_widget, bool &r_can_render)
{
	if (!MCWidgetEnsureCanManipulateWidget(p_widget))
		return;
	
	if (MCWidgetGetHost(p_widget)->getNativeLayer() == nil)
	{
		// TODO - throw error: no native layer
		return;
	}
	
	r_can_render = MCWidgetGetHost(p_widget)->getNativeLayer()->GetCanRenderToContext();
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetNativeLayerCanRenderToContext(bool p_can_render, MCWidgetRef p_widget)
{
	if (!MCWidgetEnsureCanManipulateWidget(p_widget))
		return;
	
	if (MCWidgetGetHost(p_widget)->getNativeLayer() == nil)
	{
		// TODO - throw error: no native layer
		return;
	}
	
	MCWidgetGetHost(p_widget)->getNativeLayer()->SetCanRenderToContext(p_can_render);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetStackNativeViewOfWidget(MCWidgetRef p_widget, void *&r_native_view)
{
	if (!MCWidgetEnsureCanManipulateWidget(p_widget))
		return;
	
	/* UNCHECKED */
	r_native_view = MCscreen->GetNativeWindowHandle(MCWidgetGetHost(p_widget)->getw());
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetStackNativeDisplayOfWidget(MCWidgetRef p_widget, void *&r_display)
{
	if (!MCWidgetEnsureCanManipulateWidget(p_widget))
		return;
	
	/* UNCHECKED */
	if (!MCscreen->platform_get_display_handle(r_display))
	{
		// TODO - throw error
		return;
	}
}

//////////

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyNativeLayer(void *&r_native_layer)
{
	if (!MCWidgetEnsureCurrentWidget())
		return;
	
	MCWidgetGetNativeLayerOfWidget(MCcurrentwidget, r_native_layer);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetMyNativeLayer(void *p_native_layer)
{
	if (!MCWidgetEnsureCurrentWidget())
		return;
	
	MCWidgetSetNativeLayerOfWidget(p_native_layer, MCcurrentwidget);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyNativeLayerCanRenderToContext(bool &r_can_render)
{
	if (!MCWidgetEnsureCurrentWidget())
		return;
	
	MCWidgetGetNativeLayerCanRenderToContext(MCcurrentwidget, r_can_render);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetSetMyNativeLayerCanRenderToContext(bool p_can_render)
{
	if (!MCWidgetEnsureCurrentWidget())
		return;
	
	MCWidgetSetNativeLayerCanRenderToContext(p_can_render, MCcurrentwidget);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyStackNativeView(void *&r_view)
{
	if (!MCWidgetEnsureCurrentWidget())
		return;
	
	MCWidgetGetStackNativeViewOfWidget(MCcurrentwidget, r_view);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetGetMyStackNativeDisplay(void *&r_display)
{
	if (!MCWidgetEnsureCurrentWidget())
		return;
	
	MCWidgetGetStackNativeDisplayOfWidget(MCcurrentwidget, r_display);
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
    nil,
    nil,
};

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef kMCWidgetNoCurrentWidgetErrorTypeInfo = nil;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCWidgetSizeFormatErrorTypeInfo = nil;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCWidgetTypeInfo = nil;

extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCWidgetTypeInfo()
{ return kMCWidgetTypeInfo; }

bool com_livecode_widget_InitializePopups(void);
void com_livecode_widget_FinalizePopups(void);

extern "C" bool com_livecode_widget_Initialize(void)
{
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.widget.NoCurrentWidgetError"), MCNAME("widget"), MCSTR("No current widget."), kMCWidgetNoCurrentWidgetErrorTypeInfo))
		return false;
	
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.widget.WidgetSizeFormatError"), MCNAME("widget"), MCSTR("Size must be a list of two numbers"), kMCWidgetSizeFormatErrorTypeInfo))
		return false;
	
    if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.widget.Widget"), kMCNullTypeInfo, &kMCWidgetCustomValueCallbacks, kMCWidgetTypeInfo))
        return false;

    com_livecode_widget_InitializePopups();
    
	return true;
}

extern "C" void com_livecode_widget_Finalize(void)
{
    MCValueRelease(kMCWidgetTypeInfo);
    kMCWidgetTypeInfo = nil;
    
	MCValueRelease(kMCWidgetNoCurrentWidgetErrorTypeInfo);
	kMCWidgetNoCurrentWidgetErrorTypeInfo = nil;
	
	MCValueRelease(kMCWidgetSizeFormatErrorTypeInfo);
	kMCWidgetSizeFormatErrorTypeInfo = nil;
    
    com_livecode_widget_FinalizePopups();
}

////////////////////////////////////////////////////////////////////////////////

