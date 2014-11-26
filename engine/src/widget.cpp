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

#include "globals.h"
#include "context.h"

#include "widget-events.h"


////////////////////////////////////////////////////////////////////////////////

MCGContextRef MCwidgetcontext;
MCWidget *MCwidgetobject;

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCWidget::kProperties[] =
{
	DEFINE_RW_OBJ_PROPERTY(P_KIND, Name, MCWidget, Kind)
};

MCObjectPropertyTable MCWidget::kPropertyTable =
{
	&MCControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0]
};

////////////////////////////////////////////////////////////////////////////////

MCWidget::MCWidget(void) :
  m_native_layer(nil)
{

}

MCWidget::MCWidget(const MCWidget& p_other) :
  MCControl(p_other),
  m_native_layer(nil)
{
    m_native_layer = createNativeLayer();
    m_native_layer->OnAttach();
}

MCWidget::~MCWidget(void)
{
}

Chunk_term MCWidget::gettype(void) const
{
	return CT_WIDGET;
}

const char *MCWidget::gettypestring(void)
{
	return MCwidgetstring;
}

const MCObjectPropertyTable *MCWidget::getpropertytable(void) const
{
	return &kPropertyTable;
}

void MCWidget::open(void)
{
	MCControl::open();
    MCwidgeteventmanager->event_open(this);
}

void MCWidget::close(void)
{
    MCwidgeteventmanager->event_close(this);
	MCControl::close();
}

void MCWidget::kfocus(void)
{
	MCControl::kfocus();
	if (getstate(CS_KFOCUSED))
        MCwidgeteventmanager->event_kfocus(this);
}

void MCWidget::kunfocus(void)
{
	if (getstate(CS_KFOCUSED))
        MCwidgeteventmanager->event_kunfocus(this);
	MCControl::kunfocus();
}

Boolean MCWidget::kdown(MCStringRef p_key_string, KeySym p_key)
{
	if (MCwidgeteventmanager->event_kdown(this, p_key_string, p_key))
		return True;

	return MCObject::kdown(p_key_string, p_key);
}

Boolean MCWidget::kup(MCStringRef p_key_string, KeySym p_key)
{
	if (MCwidgeteventmanager->event_kup(this, p_key_string, p_key))
        return True;
    
    return False;
}

Boolean MCWidget::mdown(uint2 p_which)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mdown(p_which);

	switch(getstack() -> gettool(this))
	{
	case T_BROWSE:
		setstate(True, CS_MFOCUSED);
        MCwidgeteventmanager->event_mdown(this, p_which);
		break;

	case T_POINTER:
		if (getstate(CS_MFOCUSED))
			return False;
		setstate(True, CS_MFOCUSED);
		if (p_which == Button1)
			start(True);
		break;

	default:
		break;	
	}

	return True;
}

Boolean MCWidget::mup(uint2 p_which, bool p_release)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(p_which, p_release);
	
	switch(getstack() -> gettool(this))
	{
	case T_BROWSE:
        MCwidgeteventmanager->event_mup(this, p_which, p_release);
		if (MCwidgeteventmanager->GetMouseButtonState() == 0)
			setstate(False, CS_MFOCUSED);
		break;

	case T_POINTER:
		if (!getstate(CS_MFOCUSED))
			return False;
		setstate(False, CS_MFOCUSED);
		if (p_which == Button1)
			end(true, p_release);
		break;
			
	default:
		break;	

	}

	return True;
}

Boolean MCWidget::mfocus(int2 p_x, int2 p_y)
{
	if (!(getflag(F_VISIBLE) || MCshowinvisibles) ||
		(getflag(F_DISABLED) && (getstack() -> gettool(this) == T_BROWSE)))
		return False;
	
	if (getstack() -> gettool(this) != T_BROWSE)
		return MCControl::mfocus(p_x, p_y);
	
	// Update the mouse loc.
	mx = p_x;
	my = p_y;
	
	// Get control local coords
	int32_t t_mouse_x, t_mouse_y;
	t_mouse_x = p_x - getrect() . x;
	t_mouse_y = p_y - getrect() . y;
	
    return MCwidgeteventmanager->event_mfocus(this, p_x, p_y);
}

void MCWidget::munfocus(void)
{
	if (getstack() -> gettool(this) != T_BROWSE ||
		(MCwidgeteventmanager->GetMouseWidget() != this
         && MCwidgeteventmanager->GetMouseButtonState() == 0))
	{
		MCControl::munfocus();
		return;
	}
	
    MCwidgeteventmanager->event_munfocus(this);
}

void MCWidget::mdrag(void)
{
    MCwidgeteventmanager->event_mdrag(this);
}

Boolean MCWidget::doubledown(uint2 p_which)
{
    return MCwidgeteventmanager->event_doubledown(this, p_which);
}

Boolean MCWidget::doubleup(uint2 p_which)
{
    return MCwidgeteventmanager->event_doubleup(this, p_which);
}

MCObject* MCWidget::hittest(int32_t x, int32_t y)
{
    bool t_inside = false;
    MCRectangle t_rect;
    t_rect = MCU_make_rect(x, y, 1, 1);
    
    // Start with a basic (fast-path) bounds test
    OnBoundsTest(t_rect, t_inside);
    
    // If within bounds, do a more thorough hit test
    if (t_inside)
        OnHitTest(t_rect, t_inside);
    
    return t_inside ? this : nil;
}

void MCWidget::timer(MCNameRef p_message, MCParameter *p_parameters)
{
    MCwidgeteventmanager->event_timer(this, p_message, p_parameters);
}

void MCWidget::setrect(const MCRectangle& p_rectangle)
{
	MCRectangle t_old_rect;
	t_old_rect = rect;
	
	rect = p_rectangle;
	
    MCwidgeteventmanager->event_setrect(this, t_old_rect);
}

void MCWidget::recompute(void)
{
    MCwidgeteventmanager->event_recompute(this);
}

static void lookup_name_for_prop(Properties p_which, MCNameRef& r_name)
{
    extern LT factor_table[];
    extern const uint4 factor_table_size;
    for(uindex_t i = 0; i < factor_table_size; i++)
        if (factor_table[i] . which == p_which)
        {
            /* UNCHECKED */ MCNameCreateWithCString(factor_table[i] . token, r_name);
            return;
        }
    
    assert(false);
}

bool MCWidget::getprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue& r_value)
{
	// If we are getting any of the reserved properties, then pass directly
	// to MCControl (and super-classes) to handle. Any changes in these will
	// be notified to us so we can take action - but widget's have no direct
	// control over them.
	switch(p_which)
	{
		case P_ID:
		case P_SHORT_ID:
		case P_LONG_ID:
		case P_ABBREV_ID:
		case P_NAME:
		case P_SHORT_NAME:
		case P_ABBREV_NAME:
		case P_LONG_NAME:
		case P_ALT_ID:
		case P_LAYER:
		case P_SCRIPT:
		case P_PARENT_SCRIPT:
		case P_NUMBER:
            /*		case P_FORE_PIXEL:
             case P_BACK_PIXEL:
             case P_HILITE_PIXEL:
             case P_BORDER_PIXEL:
             case P_TOP_PIXEL:
             case P_BOTTOM_PIXEL:
             case P_SHADOW_PIXEL:
             case P_FOCUS_PIXEL:
             case P_PEN_COLOR:
             case P_BRUSH_COLOR:
             case P_FORE_COLOR:
             case P_BACK_COLOR:
             case P_HILITE_COLOR:
             case P_BORDER_COLOR:
             case P_TOP_COLOR:
             case P_BOTTOM_COLOR:
             case P_SHADOW_COLOR:
             case P_FOCUS_COLOR:
             case P_COLORS:
             case P_FORE_PATTERN:
             case P_BACK_PATTERN:
             case P_HILITE_PATTERN:
             case P_BORDER_PATTERN:
             case P_TOP_PATTERN:
             case P_BOTTOM_PATTERN:
             case P_SHADOW_PATTERN:
             case P_FOCUS_PATTERN:
             case P_PATTERNS:
             case P_TEXT_HEIGHT:
             case P_TEXT_ALIGN:
             case P_TEXT_FONT:
             case P_TEXT_SIZE:
             case P_TEXT_STYLE:*/
		case P_LOCK_LOCATION:
		case P_VISIBLE:
		case P_INVISIBLE:
		case P_SELECTED:
		case P_TRAVERSAL_ON:
		case P_OWNER:
		case P_SHORT_OWNER:
		case P_ABBREV_OWNER:
		case P_LONG_OWNER:
		case P_PROPERTIES:
		case P_CUSTOM_PROPERTY_SET:
		case P_CUSTOM_PROPERTY_SETS:
		case P_INK:
		case P_CANT_SELECT:
		case P_BLEND_LEVEL:
		case P_LOCATION:
		case P_LEFT:
		case P_TOP:
		case P_RIGHT:
		case P_BOTTOM:
		case P_TOP_LEFT:
		case P_TOP_RIGHT:
		case P_BOTTOM_LEFT:
		case P_BOTTOM_RIGHT:
		case P_WIDTH:
		case P_HEIGHT:
		case P_RECTANGLE:
		case P_TOOL_TIP:
		case P_UNICODE_TOOL_TIP:
		case P_LAYER_MODE:
            
        case P_KIND:
			return MCControl::getprop(ctxt, p_part_id, p_which, p_index, p_effective, r_value);
            
        default:
            break;
    }

    MCNewAutoNameRef t_name_for_prop;
    /* UNCHECKED */ lookup_name_for_prop(p_which, &t_name_for_prop);
    
    // CallGetProp
    
    // No properties handled for now.
    return false;
}

bool MCWidget::setprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue p_value)
{
	// If we are getting any of the reserved properties, then pass directly
	// to MCControl (and super-classes) to handle. Any changes in these will
	// be notified to us so we can take action - but widget's have no direct
	// control over them.
	switch(p_which)
	{
		case P_ID:
		case P_SHORT_ID:
		case P_LONG_ID:
		case P_ABBREV_ID:
		case P_NAME:
		case P_SHORT_NAME:
		case P_ABBREV_NAME:
		case P_LONG_NAME:
		case P_ALT_ID:
		case P_LAYER:
		case P_SCRIPT:
		case P_PARENT_SCRIPT:
		case P_NUMBER:
            /*		case P_FORE_PIXEL:
             case P_BACK_PIXEL:
             case P_HILITE_PIXEL:
             case P_BORDER_PIXEL:
             case P_TOP_PIXEL:
             case P_BOTTOM_PIXEL:
             case P_SHADOW_PIXEL:
             case P_FOCUS_PIXEL:
             case P_PEN_COLOR:
             case P_BRUSH_COLOR:
             case P_FORE_COLOR:
             case P_BACK_COLOR:
             case P_HILITE_COLOR:
             case P_BORDER_COLOR:
             case P_TOP_COLOR:
             case P_BOTTOM_COLOR:
             case P_SHADOW_COLOR:
             case P_FOCUS_COLOR:
             case P_COLORS:
             case P_FORE_PATTERN:
             case P_BACK_PATTERN:
             case P_HILITE_PATTERN:
             case P_BORDER_PATTERN:
             case P_TOP_PATTERN:
             case P_BOTTOM_PATTERN:
             case P_SHADOW_PATTERN:
             case P_FOCUS_PATTERN:
             case P_PATTERNS:
             case P_TEXT_HEIGHT:
             case P_TEXT_ALIGN:
             case P_TEXT_FONT:
             case P_TEXT_SIZE:
             case P_TEXT_STYLE:*/
		case P_LOCK_LOCATION:
		case P_VISIBLE:
		case P_INVISIBLE:
		case P_SELECTED:
		case P_TRAVERSAL_ON:
		case P_OWNER:
		case P_SHORT_OWNER:
		case P_ABBREV_OWNER:
		case P_LONG_OWNER:
		case P_PROPERTIES:
		case P_CUSTOM_PROPERTY_SET:
		case P_CUSTOM_PROPERTY_SETS:
		case P_INK:
		case P_CANT_SELECT:
		case P_BLEND_LEVEL:
		case P_LOCATION:
		case P_LEFT:
		case P_TOP:
		case P_RIGHT:
		case P_BOTTOM:
		case P_TOP_LEFT:
		case P_TOP_RIGHT:
		case P_BOTTOM_LEFT:
		case P_BOTTOM_RIGHT:
		case P_WIDTH:
		case P_HEIGHT:
		case P_RECTANGLE:
		case P_TOOL_TIP:
		case P_UNICODE_TOOL_TIP:
		case P_LAYER_MODE:
            
        case P_KIND:
			return MCControl::setprop(ctxt, p_part_id, p_which, p_index, p_effective, p_value);
            
        default:
            break;
    }
    
    MCNewAutoNameRef t_name_for_prop;
    /* UNCHECKED */ lookup_name_for_prop(p_which, &t_name_for_prop);
    
    // CallSetProp
    
    // No properties handled for now.
    return false;
}

bool MCWidget::getcustomprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCExecValue& r_value)
{
    // CallGetProp(p_prop_name, nil, ...)
    
    // Not handled for now.
    return false;
}

bool MCWidget::setcustomprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCExecValue p_value)
{
    // CallSetProp(p_prop_name, nil, ...)
    
    // Not handled for now.
    return false;
}

void MCWidget::toolchanged(Tool p_new_tool)
{
    OnToolChanged(p_new_tool);
}

void MCWidget::layerchanged()
{
    OnLayerChanged();
}
	
Exec_stat MCWidget::handle(Handler_type p_type, MCNameRef p_method, MCParameter *p_parameters, MCObject *p_passing_object)
{
	return MCControl::handle(p_type, p_method, p_parameters, p_passing_object);
}

IO_stat MCWidget::load(IO_handle p_stream, const char *version)
{
	return IO_ERROR;
}

IO_stat MCWidget::save(IO_handle p_stream, uint4 p_part, bool p_force_ext)
{
	return IO_ERROR;
}

MCControl *MCWidget::clone(Boolean p_attach, Object_pos p_position, bool invisible)
{
	MCWidget *t_new_widget;
	t_new_widget = new MCWidget(*this);
	if (p_attach)
		t_new_widget -> attach(p_position, invisible);
	return t_new_widget;
}

void MCWidget::draw(MCDC *dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite)
{
	MCRectangle dirty;
	dirty = p_dirty;
	
	if (!p_isolated)
	{
		if (!p_sprite)
		{
			dc -> setopacity(blendlevel * 255 / 100);
			dc -> setfunction(ink);
		}
		
		if (m_bitmap_effects == nil)
			dc -> begin(true);
		else
		{
			if (!dc -> begin_with_effects(m_bitmap_effects, MCU_reduce_rect(rect, -gettransient())))
				return;
			dirty = dc -> getclip();
		}
	}

	MCwidgetcontext = ((MCGraphicsContext *)dc) -> getgcontextref();
	MCGContextSave(MCwidgetcontext);
	MCGContextSetShouldAntialias(MCwidgetcontext, true);
	MCGContextTranslateCTM(MCwidgetcontext, rect . x, rect . y);
    MCwidgeteventmanager->event_draw(this, dc, dirty, p_isolated, p_sprite);
	MCGContextRestore(MCwidgetcontext);
	MCwidgetcontext = nil;
	
	if (!p_isolated)
	{
		dc -> end();

		if (getstate(CS_SELECTED))
			drawselected(dc);
	}
}

Boolean MCWidget::maskrect(const MCRectangle& p_rect)
{
	if (!(getflag(F_VISIBLE) || MCshowinvisibles))
		return False;

	MCRectangle drect = MCU_intersect_rect(p_rect, rect);

	return drect.width != 0 && drect.height != 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWidget::inEditMode()
{
    Tool t_tool = getstack()->gettool(this);
    return t_tool != T_BROWSE && t_tool != T_HELP;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWidget::handlesMouseDown() const
{
    return true;
}

bool MCWidget::handlesMouseUp() const
{
    return true;
}

bool MCWidget::handlesMouseRelease() const
{
    return true;
}

bool MCWidget::handlesKeyPress() const
{
    return true;
}

bool MCWidget::handlesTouches() const
{
    return false;
}

bool MCWidget::wantsClicks() const
{
    return true;
}

bool MCWidget::wantsTouches() const
{
    return false;
}

bool MCWidget::wantsDoubleClicks() const
{
    return true;
}

bool MCWidget::waitForDoubleClick() const
{
    return false;
}

bool MCWidget::isDragSource() const
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer* MCWidget::getNativeLayer() const
{
    return m_native_layer;
}

////////////////////////////////////////////////////////////////////////////////

void MCWidget::OnOpen()
{
    if (m_native_layer)
        m_native_layer->OnOpen();
    
    fprintf(stderr, "MCWidget::OnOpen\n");
}

void MCWidget::OnClose()
{
    if (m_native_layer)
        m_native_layer->OnClose();
    
    fprintf(stderr, "MCWidget::OnClose\n");
}

void MCWidget::OnAttach()
{
    if (m_native_layer)
        m_native_layer->OnAttach();
    
    fprintf(stderr, "MCWidget::OnAttach\n");
}

void MCWidget::OnDetach()
{
    if (m_native_layer)
        m_native_layer->OnDetach();
    
    fprintf(stderr, "MCWidget::OnDetach\n");
}

void MCWidget::OnPaint(MCDC* p_dc, const MCRectangle& p_rect)
{
    if (m_native_layer)
        m_native_layer->OnPaint(p_dc, p_rect);
    
    fprintf(stderr, "MCWidget::OnPaint\n");
}

void MCWidget::OnGeometryChanged(const MCRectangle& p_old_rect)
{
    if (m_native_layer)
        m_native_layer->OnGeometryChanged(p_old_rect);
    
    fprintf(stderr, "MCWidget::OnGeometryChanged\n");
}

void MCWidget::OnVisibilityChanged(bool p_visible)
{
    if (m_native_layer)
        m_native_layer->OnVisibilityChanged(p_visible);
    
    fprintf(stderr, "MCWidget::OnVisibilityChanged\n");
}

void MCWidget::OnHitTest(const MCRectangle& p_intersect, bool& r_hit)
{
    fprintf(stderr, "MCWidget::OnHitTest\n");
    r_hit = maskrect(p_intersect);
}

void MCWidget::OnBoundsTest(const MCRectangle& p_intersect, bool& r_hit)
{
    fprintf(stderr, "MCWidget::OnBoundsTest\n");
    r_hit = maskrect(p_intersect);
}

void MCWidget::OnSave(class MCWidgetSerializer& p_stream)
{
    fprintf(stderr, "MCWidget::OnSave\n");
}

void MCWidget::OnLoad(class MCWidgetSerializer& p_stream)
{
    fprintf(stderr, "MCWidget::OnLoad\n");
}

void MCWidget::OnCreate()
{
    fprintf(stderr, "MCWidget::OnCreate\n");
}

void MCWidget::OnDestroy()
{
    fprintf(stderr, "MCWidget::OnDestroy\n");
}

void MCWidget::OnParentPropChanged()
{
    fprintf(stderr, "MCWidget::OnParentPropChanged\n");
}

void MCWidget::OnToolChanged(Tool p_new_tool)
{
    if (m_native_layer)
        m_native_layer->OnToolChanged(p_new_tool);
    
    fprintf(stderr, "MCWidget::OnToolChanged\n");
}

void MCWidget::OnLayerChanged()
{
    if (m_native_layer)
        m_native_layer->OnLayerChanged();
    
    fprintf(stderr, "MCWidget::OnLayerChanged\n");
}

void MCWidget::OnMouseEnter()
{
    fprintf(stderr, "MCWidget::OnMouseEnter\n");
}

void MCWidget::OnMouseLeave()
{
    fprintf(stderr, "MCWidget::OnMouseLeave\n");
}

void MCWidget::OnMouseMove(coord_t p_x, coord_t p_y)
{
    fprintf(stderr, "MCWidget::OnMouseMove\n");
}

void MCWidget::OnMouseCancel(uinteger_t p_button)
{
    fprintf(stderr, "MCWidget::OnMouseCancel\n");
}

void MCWidget::OnMouseDown(coord_t p_x, coord_t p_y , uinteger_t p_button)
{
    fprintf(stderr, "MCWidget::OnMouseDown\n");
}

void MCWidget::OnMouseUp(coord_t p_x, coord_t p_y, uinteger_t p_button)
{
    fprintf(stderr, "MCWidget::OnMouseUp\n");
}

void MCWidget::OnMouseScroll(coord_t p_delta_x, coord_t p_delta_y)
{
    fprintf(stderr, "MCWidget::OnMouseScroll\n");
}

void MCWidget::OnMouseStillDown(uinteger_t p_button, real32_t p_duration)
{
    fprintf(stderr, "MCWidget::OnMouseStillDown\n");
}

void MCWidget::OnMouseHover(coord_t p_x, coord_t p_y, real32_t p_duration)
{
    fprintf(stderr, "MCWidget::OnMouseHover\n");
}

void MCWidget::OnMouseStillHover(coord_t p_x, coord_t p_y, real32_t p_duration)
{
    fprintf(stderr, "MCWidget::OnMouseStillHover\n");
}

void MCWidget::OnMouseCancelHover(real32_t p_duration)
{
    fprintf(stderr, "MCWidget::OnMouseCancelHover\n");
}

void MCWidget::OnTouchStart(uinteger_t p_id, coord_t p_x, coord_t p_y, real32_t p_pressure, real32_t p_radius)
{
    fprintf(stderr, "MCWidget::OnTouchStart\n");
}

void MCWidget::OnTouchMove(uinteger_t p_id, coord_t p_x, coord_t p_y, real32_t p_pressure, real32_t p_radius)
{
    fprintf(stderr, "MCWidget::OnTouchMove\n");
}

void MCWidget::OnTouchEnter(uinteger_t p_id)
{
    fprintf(stderr, "MCWidget::OnTouchEnter\n");
}

void MCWidget::OnTouchLeave(uinteger_t p_id)
{
    fprintf(stderr, "MCWidget::OnTouchLeave\n");
}

void MCWidget::OnTouchFinish(uinteger_t p_id, coord_t p_x, coord_t p_y)
{
    fprintf(stderr, "MCWidget::OnTouchFinish\n");
}

void MCWidget::OnTouchCancel(uinteger_t p_id)
{
    fprintf(stderr, "MCWidget::OnTouchCancel\n");
}

void MCWidget::OnFocusEnter()
{
    fprintf(stderr, "MCWidget::OnFocusEnter\n");
}

void MCWidget::OnFocusLeave()
{
    fprintf(stderr, "MCWidget::OnFocusLEave\n");
}

void MCWidget::OnKeyPress(MCStringRef p_keytext)
{
    fprintf(stderr, "MCWidget::OnKeyPress\n");
}

void MCWidget::OnModifiersChanged(uinteger_t p_modifier_mask)
{
    fprintf(stderr, "MCWidget::OnModifiersChanged\n");
}

void MCWidget::OnActionKeyPress(MCStringRef p_keyname)
{
    fprintf(stderr, "MCWidget::OnActionKeyPress\n");
}

void MCWidget::OnDragEnter(bool& r_accept)
{
    fprintf(stderr, "MCWidget::OnDragEnter\n");
    r_accept = true;
}

void MCWidget::OnDragLeave()
{
    fprintf(stderr, "MCWidget::OnDragLeave\n");
}

void MCWidget::OnDragMove(coord_t p_x, coord_t p_y)
{
    fprintf(stderr, "MCWidget::OnDragMove\n");
}

void MCWidget::OnDragDrop()
{
    fprintf(stderr, "MCWidget::OnDragDrop\n");
}

void MCWidget::OnDragStart(bool& r_accept)
{
    fprintf(stderr, "MCWidget::OnDragStart\n");
    r_accept = true;
}

void MCWidget::OnDragFinish()
{
    fprintf(stderr, "MCWidget::OnDragFinish\n");
}

void MCWidget::OnClick(coord_t p_x, coord_t p_y, uinteger_t p_button, uinteger_t p_count)
{
    fprintf(stderr, "MCWidget::OnClick\n");
}

void MCWidget::OnDoubleClick(coord_t p_x, coord_t p_y, uinteger_t p_button)
{
    fprintf(stderr, "MCWidget::OnDoubleClick\n");
}

////////////////////////////////////////////////////////////////////////////////

bool MCWidget::CallHandler(MCNameRef p_name, MCParameter *p_parameters)
{
	MCWidget *t_old_widget_object;
	t_old_widget_object = MCwidgetobject;
	MCwidgetobject = this;
	
    // Invoke event handler.
    
	MCwidgetobject = t_old_widget_object;
    
	return true;
}

bool MCWidget::CallGetProp(MCNameRef p_property, MCNameRef p_key, MCValueRef& r_value)
{
	MCWidget *t_old_widget_object;
	t_old_widget_object = MCwidgetobject;
	MCwidgetobject = this;
	
    // Invoke event handler.
    
	MCwidgetobject = t_old_widget_object;
    
	return true;
}

bool MCWidget::CallSetProp(MCNameRef p_property, MCNameRef p_key, MCValueRef p_value)
{
	MCWidget *t_old_widget_object;
	t_old_widget_object = MCwidgetobject;
	MCwidgetobject = this;
	
    // Invoke event handler.
    
	MCwidgetobject = t_old_widget_object;
    
	return true;
}

void MCWidget::SetKind(MCExecContext& ctxt, MCNameRef p_new_kind)
{
    // Look for extension
    
	int t_opened;
	t_opened = opened;
	while(opened)
		close();
	
    // Replace current implementation with new implementation.
    
	while(t_opened > 0)
	{
		open();
		t_opened -= 1;
	}
	
    layer_redrawall();
}

void MCWidget::GetKind(MCExecContext& ctxt, MCNameRef& r_kind)
{
    r_kind = MCValueRetain(kMCEmptyName);
}

////////////////////////////////////////////////////////////////////////////////
