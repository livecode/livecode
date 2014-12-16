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

#include "module-canvas.h"

#include "module-engine.h"

////////////////////////////////////////////////////////////////////////////////

void MCCanvasPush(MCGContextRef gcontext, uintptr_t& r_cookie);
void MCCanvasPop(uintptr_t p_cookie);

////////////////////////////////////////////////////////////////////////////////

MCWidget *MCwidgetobject;

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCWidget::kProperties[] =
{
	DEFINE_RO_OBJ_PROPERTY(P_KIND, Name, MCWidget, Kind)
};

MCObjectPropertyTable MCWidget::kPropertyTable =
{
	&MCControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0]
};

////////////////////////////////////////////////////////////////////////////////

MCWidget::MCWidget(MCNameRef p_kind) :
  m_kind(p_kind),
  m_module(nil),
  m_instance(nil),
  m_native_layer(nil)
{
    ;
}

MCWidget::MCWidget(const MCWidget& p_other) :
  MCControl(p_other),
  m_kind(MCValueRetain(p_other.m_kind)),
  m_module(nil),
  m_instance(nil),
  m_native_layer(nil)
{
    ;
}

MCWidget::~MCWidget(void)
{
    ;
}

#include "mcio.h"
#include "system.h"

MCWidget* MCWidget::createInstanceOfKind(MCNameRef p_kind)
{
    // Attempt to look-up the module for the given kind and ensure that all of
    // its dependencies have been satisfied.
    MCScriptModuleRef t_module;
    if (!MCScriptLookupModule(p_kind, t_module) || !MCScriptEnsureModuleIsUsable(t_module))
        return nil;
    
    // Create a new instance of the given module
    MCScriptInstanceRef t_instance;
    if (!MCScriptCreateInstanceOfModule(t_module, t_instance))
        t_instance = nil;
    
    MCScriptReleaseModule(t_module);
    
    if (t_instance == nil)
        return nil;
    
    // Create the widget and set its instance
    MCWidget* t_widget;
    t_widget = new MCWidget(p_kind);
    t_widget->m_module = t_module;
    t_widget->m_instance = t_instance;
    
    // Widget has now been created
    t_widget->OnCreate();
    
    return t_widget;
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

	return MCControl::kdown(p_key_string, p_key);
}

Boolean MCWidget::kup(MCStringRef p_key_string, KeySym p_key)
{
	if (MCwidgeteventmanager->event_kup(this, p_key_string, p_key))
        return True;
    
    return MCControl::kup(p_key_string, p_key);
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
        else
            message_with_args(MCM_mouse_down, p_which);
		break;

	default:
        message_with_args(MCM_mouse_down, p_which);
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
            
        // Development mode only
        case P_REV_AVAILABLE_HANDLERS:
        case P_REV_AVAILABLE_VARIABLES:
    
        case P_KIND:
			return MCControl::getprop(ctxt, p_part_id, p_which, p_index, p_effective, r_value);
            
        default:
            break;
    }

    MCNewAutoNameRef t_name_for_prop;
    /* UNCHECKED */ lookup_name_for_prop(p_which, &t_name_for_prop);
    
    // Forward to the custom property handler
    return getcustomprop(ctxt, nil, *t_name_for_prop, r_value);
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
    
    // Forward to the custom property handler
    return setcustomprop(ctxt, nil, *t_name_for_prop, p_value);
}

bool MCWidget::getcustomprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCExecValue& r_value)
{
    // Treat as a normal custom property if not a widget property
    MCTypeInfoRef t_getter, t_setter;
    if (p_set_name != nil || !MCScriptQueryPropertyOfModule(m_module, p_prop_name, t_getter, t_setter))
        return MCObject::getcustomprop(ctxt, p_set_name, p_prop_name, r_value);
    
    if (CallGetProp(ctxt, p_prop_name, nil, r_value.valueref_value))
    {
        r_value.type = kMCExecValueTypeValueRef;
        return true;
    }
    
    // Property handler failed
    return false;
}

bool MCWidget::setcustomprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCExecValue p_value)
{
    // Treat as a normal custom property if not a widget property
    MCTypeInfoRef t_getter, t_setter;
    if (p_set_name != nil || !MCScriptQueryPropertyOfModule(m_module, p_prop_name, t_getter, t_setter))
        return MCObject::setcustomprop(ctxt, p_set_name, p_prop_name, p_value);
    
    MCAutoValueRef t_value;
    MCExecTypeConvertToValueRefAndReleaseAlways(ctxt, p_value.type, &p_value.valueref_value, &t_value);
    if (!ctxt.HasError())
    {
        if (CallSetProp(ctxt, p_prop_name, nil, *t_value))
            return true;
    }
    
    // Property handler failed
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

    MCGContextRef t_gcontext;
    t_gcontext = ((MCGraphicsContext *)dc) -> getgcontextref();
    
	MCGContextSave(t_gcontext);
	MCGContextSetShouldAntialias(t_gcontext, true);
	MCGContextTranslateCTM(t_gcontext, rect . x, rect . y);
    
    uintptr_t t_cookie;
    MCCanvasPush(t_gcontext, t_cookie);
    MCwidgeteventmanager->event_draw(this, dc, dirty, p_isolated, p_sprite);
    MCCanvasPop(t_cookie);
    
	MCGContextRestore(t_gcontext);
	
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

// TODO: all of these should be cached

bool MCWidget::handlesMouseDown() const
{
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(m_module, MCNAME("OnMouseDown"), t_signature);
}

bool MCWidget::handlesMouseUp() const
{
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(m_module, MCNAME("OnMouseUp"), t_signature);
}

bool MCWidget::handlesMouseRelease() const
{
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(m_module, MCNAME("OnMouseRelease"), t_signature);
}

bool MCWidget::handlesKeyPress() const
{
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(m_module, MCNAME("OnKeyPress"), t_signature);
}

bool MCWidget::handlesTouches() const
{
    return false;
}

bool MCWidget::wantsClicks() const
{
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(m_module, MCNAME("OnClick"), t_signature);
}

bool MCWidget::wantsTouches() const
{
    return false;
}

bool MCWidget::wantsDoubleClicks() const
{
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(m_module, MCNAME("OnDoubleClick"), t_signature);
}

bool MCWidget::waitForDoubleClick() const
{
    return false;
}

bool MCWidget::isDragSource() const
{
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(m_module, MCNAME("OnDragStart"), t_signature);
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
    
    CallHandler(MCNAME("OnOpen"), nil, 0);
}

void MCWidget::OnClose()
{
    if (m_native_layer)
        m_native_layer->OnClose();
    
    CallHandler(MCNAME("OnClose"), nil, 0);
}

void MCWidget::OnAttach()
{
    if (m_native_layer)
        m_native_layer->OnAttach();
    
    CallHandler(MCNAME("OnAttach"), nil, 0);
}

void MCWidget::OnDetach()
{
    if (m_native_layer)
        m_native_layer->OnDetach();
    
    CallHandler(MCNAME("OnDetach"), nil, 0);
}

void MCWidget::OnPaint(MCDC* p_dc, const MCRectangle& p_rect)
{
    if (m_native_layer)
        m_native_layer->OnPaint(p_dc, p_rect);

    uintptr_t t_cookie;
    MCCanvasPush(((MCGraphicsContext*)p_dc)->getgcontextref(), t_cookie);
    CallHandler(MCNAME("OnPaint"), nil, 0);
    MCCanvasPop(t_cookie);
}

void MCWidget::OnGeometryChanged(const MCRectangle& p_old_rect)
{
    if (m_native_layer)
        m_native_layer->OnGeometryChanged(p_old_rect);
    
    MCAutoValueRefArray t_params;
    t_params.New(4);
    
    MCNumberCreateWithReal(rect.x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(rect.y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithReal(rect.width, reinterpret_cast<MCNumberRef&>(t_params[2]));
    MCNumberCreateWithReal(rect.height, reinterpret_cast<MCNumberRef&>(t_params[3]));
    
    CallHandler(MCNAME("OnGeometryChanged"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnVisibilityChanged(bool p_visible)
{
    if (m_native_layer)
        m_native_layer->OnVisibilityChanged(p_visible);
    
    MCAutoValueRefArray t_params;
    t_params.New(1);
    
    MCBooleanCreateWithBool(p_visible, reinterpret_cast<MCBooleanRef&>(t_params[0]));
    
    CallHandler(MCNAME("OnVisibilityChanged"), nil, 0);
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
    CallHandler(MCNAME("OnCreate"), nil, 0);
}

void MCWidget::OnDestroy()
{
    CallHandler(MCNAME("OnDestroy"), nil, 0);
}

void MCWidget::OnParentPropChanged()
{
    CallHandler(MCNAME("OnParentPropertyChanged"), nil, 0);
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
    
    CallHandler(MCNAME("OnLayerChanged"), nil, 0);
}

void MCWidget::OnMouseEnter()
{
    CallHandler(MCNAME("OnMouseEnter"), nil, 0);
}

void MCWidget::OnMouseLeave()
{
    CallHandler(MCNAME("OnMouseLeave"), nil, 0);
}

void MCWidget::OnMouseMove(coord_t p_x, coord_t p_y)
{
    MCAutoValueRefArray t_params;
    t_params.New(2);
    
    MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    
    CallHandler(MCNAME("OnMouseMove"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnMouseCancel(uinteger_t p_button)
{
    MCAutoValueRefArray t_params;
    t_params.New(1);
    
    MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[0]));
    
    CallHandler(MCNAME("OnMouseCancel"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnMouseDown(coord_t p_x, coord_t p_y , uinteger_t p_button)
{
    MCAutoValueRefArray t_params;
    t_params.New(3);
    
    MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[2]));
    
    CallHandler(MCNAME("OnMouseDown"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnMouseUp(coord_t p_x, coord_t p_y, uinteger_t p_button)
{
    MCAutoValueRefArray t_params;
    t_params.New(3);
    
    MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[2]));
    
    CallHandler(MCNAME("OnMouseUp"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnMouseScroll(coord_t p_delta_x, coord_t p_delta_y)
{
    MCAutoValueRefArray t_params;
    t_params.New(2);
    
    MCNumberCreateWithReal(p_delta_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_delta_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    
    CallHandler(MCNAME("OnMouseScroll"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnMouseStillDown(uinteger_t p_button, real32_t p_duration)
{
    MCAutoValueRefArray t_params;
    t_params.New(2);
    
    MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_duration, reinterpret_cast<MCNumberRef&>(t_params[1]));
    
    CallHandler(MCNAME("OnMouseStillDown"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnMouseHover(coord_t p_x, coord_t p_y, real32_t p_duration)
{
    MCAutoValueRefArray t_params;
    t_params.New(3);
    
    MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithReal(p_duration, reinterpret_cast<MCNumberRef&>(t_params[2]));
    
    CallHandler(MCNAME("OnMouseHover"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnMouseStillHover(coord_t p_x, coord_t p_y, real32_t p_duration)
{
    MCAutoValueRefArray t_params;
    t_params.New(3);
    
    MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithReal(p_duration, reinterpret_cast<MCNumberRef&>(t_params[2]));
    
    CallHandler(MCNAME("OnMouseStillHover"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnMouseCancelHover(real32_t p_duration)
{
    MCAutoValueRefArray t_params;
    t_params.New(1);
    
    MCNumberCreateWithReal(p_duration, reinterpret_cast<MCNumberRef&>(t_params[0]));
    
    CallHandler(MCNAME("OnMouseCancelHover"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnTouchStart(uinteger_t p_id, coord_t p_x, coord_t p_y, real32_t p_pressure, real32_t p_radius)
{
    MCAutoValueRefArray t_params;
    t_params.New(5);
    
    MCNumberCreateWithUnsignedInteger(p_id, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[2]));
    MCNumberCreateWithReal(p_pressure, reinterpret_cast<MCNumberRef&>(t_params[3]));
    MCNumberCreateWithReal(p_radius, reinterpret_cast<MCNumberRef&>(t_params[4]));
    
    CallHandler(MCNAME("OnTouchStart"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnTouchMove(uinteger_t p_id, coord_t p_x, coord_t p_y, real32_t p_pressure, real32_t p_radius)
{
    MCAutoValueRefArray t_params;
    t_params.New(5);
    
    MCNumberCreateWithUnsignedInteger(p_id, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[2]));
    MCNumberCreateWithReal(p_pressure, reinterpret_cast<MCNumberRef&>(t_params[3]));
    MCNumberCreateWithReal(p_radius, reinterpret_cast<MCNumberRef&>(t_params[4]));
    
    CallHandler(MCNAME("OnTouchMove"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnTouchEnter(uinteger_t p_id)
{
    MCAutoValueRefArray t_params;
    t_params.New(1);
    
    MCNumberCreateWithUnsignedInteger(p_id, reinterpret_cast<MCNumberRef&>(t_params[0]));
    
    CallHandler(MCNAME("OnTouchEnter"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnTouchLeave(uinteger_t p_id)
{
    MCAutoValueRefArray t_params;
    t_params.New(1);
    
    MCNumberCreateWithUnsignedInteger(p_id, reinterpret_cast<MCNumberRef&>(t_params[0]));
    
    CallHandler(MCNAME("OnTouchLeave"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnTouchFinish(uinteger_t p_id, coord_t p_x, coord_t p_y)
{
    MCAutoValueRefArray t_params;
    t_params.New(3);
    
    MCNumberCreateWithUnsignedInteger(p_id, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[2]));
    
    CallHandler(MCNAME("OnTouchFinish"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnTouchCancel(uinteger_t p_id)
{
    MCAutoValueRefArray t_params;
    t_params.New(1);
    
    MCNumberCreateWithUnsignedInteger(p_id, reinterpret_cast<MCNumberRef&>(t_params[0]));
    
    CallHandler(MCNAME("OnTouchCancel"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnFocusEnter()
{
    CallHandler(MCNAME("OnFocusEnter"), nil, 0);
}

void MCWidget::OnFocusLeave()
{
    CallHandler(MCNAME("OnFocusLeave"), nil, 0);
}

void MCWidget::OnKeyPress(MCStringRef p_keytext)
{
    MCAutoValueRefArray t_params;
    t_params.New(1);
    
    t_params[0] = MCValueRetain(p_keytext);
    
    CallHandler(MCNAME("OnKeyPress"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnModifiersChanged(uinteger_t p_modifier_mask)
{
    MCAutoValueRefArray t_params;
    t_params.New(1);
    
    MCNumberCreateWithUnsignedInteger(p_modifier_mask, reinterpret_cast<MCNumberRef&>(t_params[0]));
    
    CallHandler(MCNAME("OnModifiersChanged"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnActionKeyPress(MCStringRef p_keyname)
{
    MCAutoValueRefArray t_params;
    t_params.New(1);
    
    t_params[0] = MCValueRetain(p_keyname);
    
    CallHandler(MCNAME("OnActionKeyPress"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnDragEnter(bool& r_accept)
{
    MCValueRef t_retval;
    if (CallHandler(MCNAME("OnDragEnter"), nil, 0, &t_retval))
    {
        MCExecContext t_ctxt;
        if (!t_ctxt.ConvertToBool(t_retval, r_accept))
            r_accept = false;
        MCValueRelease(t_retval);
    }
    else
    {
        // Call failed
        r_accept = false;
    }
}

void MCWidget::OnDragLeave()
{
    CallHandler(MCNAME("OnDragLeave"), nil, 0);
}

void MCWidget::OnDragMove(coord_t p_x, coord_t p_y)
{
    MCAutoValueRefArray t_params;
    t_params.New(2);
    
    MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    
    CallHandler(MCNAME("OnDragMove"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnDragDrop()
{
    CallHandler(MCNAME("OnDragDrop"), nil, 0);
}

void MCWidget::OnDragStart(bool& r_accept)
{
    MCValueRef t_retval;
    if (CallHandler(MCNAME("OnDragStart"), nil, 0, &t_retval))
    {
        MCExecContext t_ctxt;
        if (!t_ctxt.ConvertToBool(t_retval, r_accept))
            r_accept = false;
        MCValueRelease(t_retval);
    }
    else
    {
        // Call failed
        r_accept = false;
    }
}

void MCWidget::OnDragFinish()
{
    CallHandler(MCNAME("OnDragFinish"), nil, 0);
}

void MCWidget::OnClick(coord_t p_x, coord_t p_y, uinteger_t p_button, uinteger_t p_count)
{
    MCAutoValueRefArray t_params;
    t_params.New(4);
    
    MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[2]));
    MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[3]));
    
    CallHandler(MCNAME("OnClick"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnDoubleClick(coord_t p_x, coord_t p_y, uinteger_t p_button)
{
    MCAutoValueRefArray t_params;
    t_params.New(3);
    
    MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[2]));
    
    CallHandler(MCNAME("OnDoubleClick"), t_params.Ptr(), t_params.Size());
}

////////////////////////////////////////////////////////////////////////////////

bool MCWidget::CallHandler(MCNameRef p_name, MCValueRef* x_parameters, uindex_t p_param_count, MCValueRef* r_retval)
{
	MCWidget *t_old_widget_object;
	t_old_widget_object = MCwidgetobject;
	MCwidgetobject = this;
	
    // Invoke event handler.
    bool t_success;
    MCValueRef t_retval;
    t_success = MCScriptCallHandlerOfInstance(m_instance, p_name, x_parameters, p_param_count, t_retval);
    
    if (t_success)
    {
        if (r_retval != NULL)
            *r_retval = t_retval;
        else
            MCValueRelease(t_retval);
    }
    else
    {
        MCErrorRef t_error;
        
        // TODO: handle
        if (MCErrorCatch(t_error))
        {
            MCLog(MCStringGetCString(MCErrorGetMessage(t_error)), 0);
            MCValueRelease(t_error);
        }
        else
            MCLog("Failed to execute handler %@", p_name);
    }
    
	MCwidgetobject = t_old_widget_object;
    
	return t_success;
}

bool MCWidget::CallGetProp(MCExecContext& ctxt, MCNameRef p_property, MCNameRef p_key, MCValueRef& r_value)
{
	MCWidget *t_old_widget_object;
	t_old_widget_object = MCwidgetobject;
	MCwidgetobject = this;

    // Invoke event handler.
    bool t_success;
    t_success = MCScriptGetPropertyOfInstance(m_instance, p_property, r_value);
    
	MCwidgetobject = t_old_widget_object;
    
    if (t_success)
    {
        // Convert to a script type
        t_success = MCExtensionConvertToScriptType(ctxt, r_value);
    }
    
    if (!t_success)
    {
        MCErrorRef t_error;
        
        // TODO: handle
        if (MCErrorCatch(t_error))
        {
            MCLog(MCStringGetCString(MCErrorGetMessage(t_error)), 0);
            MCValueRelease(t_error);
        }
        else
            MCLog("Failed to execute property getter for %@", p_property);
    }
    
	return t_success;
}

bool MCWidget::CallSetProp(MCExecContext& ctxt, MCNameRef p_property, MCNameRef p_key, MCValueRef p_value)
{
	MCWidget *t_old_widget_object;
	t_old_widget_object = MCwidgetobject;
	MCwidgetobject = this;
    
    // Convert to the appropriate type
    MCTypeInfoRef t_gettype, t_settype;
    if (!MCScriptQueryPropertyOfModule(m_module, p_property, t_gettype, t_settype))
        return false;
    
    if (!MCExtensionConvertFromScriptType(ctxt, t_settype, p_value))
        return false;
    
    // Invoke event handler.
    bool t_success;
    t_success = MCScriptSetPropertyOfInstance(m_instance, p_property, p_value);
    
	MCwidgetobject = t_old_widget_object;
    
    if (!t_success)
    {
        MCErrorRef t_error;
        
        // TODO: handle
        if (MCErrorCatch(t_error))
        {
            MCLog(MCStringGetCString(MCErrorGetMessage(t_error)), 0);
            MCValueRelease(t_error);
        }
        else
            MCLog("Failed to execute property setter for %@", p_property);
    }
    
	return t_success;
}

void MCWidget::GetKind(MCExecContext& ctxt, MCNameRef& r_kind)
{
    r_kind = MCValueRetain(m_kind);
}

////////////////////////////////////////////////////////////////////////////////

typedef struct __MCPressedState* MCPressedStateRef;
MCTypeInfoRef kMCPressedState;

extern "C" MC_DLLEXPORT void MCWidgetExecRedrawAll(void)
{
    if (MCwidgetobject == nil)
    {
        // TODO - throw an error
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("no current widget"), nil);
        return;
    }
    
    MCwidgetobject -> layer_redrawall();
}

extern "C" MC_DLLEXPORT void MCWidgetGetScriptObject(MCScriptObjectRef& r_script_object)
{
    if (MCwidgetobject == nil)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("no current widget"), nil);
        return;
    }
    
    if (!MCScriptObjectCreate(MCwidgetobject, 0, r_script_object))
        return;
}

extern "C" MC_DLLEXPORT void MCWidgetGetRectangle(MCCanvasRectangleRef& r_rect)
{
    if (MCwidgetobject == nil)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("no current widget"), nil);
        return;
    }
    
    MCRectangle t_rect;
    MCGRectangle t_grect;
    t_rect = MCwidgetobject -> getrect();
    
    // Absolute rectangle
    t_grect = MCGRectangleMake(t_rect.x, t_rect.y, t_rect.width, t_rect.height);
    MCCanvasRectangleCreateWithMCGRectangle(t_grect, r_rect);
}

extern "C" MC_DLLEXPORT void MCWidgetGetFrame(MCCanvasRectangleRef& r_rect)
{
    if (MCwidgetobject == nil)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("no current widget"), nil);
        return;
    }
    
    MCRectangle t_rect;
    MCGRectangle t_grect;
    t_rect = MCwidgetobject -> getrect();
    
    // Adjust for the parent's rect
    MCRectangle t_parent_rect;
    t_parent_rect = MCwidgetobject->getparent()->getrect();
    t_rect.x -= t_parent_rect.x;
    t_rect.y -= t_parent_rect.y;
    
    t_grect = MCGRectangleMake(t_rect.x, t_rect.y, t_rect.width, t_rect.height);
    MCCanvasRectangleCreateWithMCGRectangle(t_grect, r_rect);
}

extern "C" MC_DLLEXPORT void MCWidgetGetBounds(MCCanvasRectangleRef& r_rect)
{
    if (MCwidgetobject == nil)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("no current widget"), nil);
        return;
    }
    
    MCRectangle t_rect;
    MCGRectangle t_grect;
    t_rect = MCwidgetobject -> getrect();
    
    // Only the size is wanted
    t_grect = MCGRectangleMake(0, 0, t_rect.width, t_rect.height);
    MCCanvasRectangleCreateWithMCGRectangle(t_grect, r_rect);
}

extern "C" MC_DLLEXPORT void MCWidgetGetMousePosition(bool p_current, MCCanvasPointRef& r_point)
{
    if (MCwidgetobject == nil)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("no current widget"), nil);
        return;
    }
    
    // TODO - coordinate transform
    
    coord_t t_x, t_y;
    if (p_current)
        MCwidgeteventmanager->GetAsynchronousMousePosition(t_x, t_y);
    else
        MCwidgeteventmanager->GetSynchronousMousePosition(t_x, t_y);
    
    MCRectangle t_rect = MCwidgetobject->getrect();
    t_x -= t_rect.x;
    t_y -= t_rect.y;
    
    MCGPoint t_gpoint;
    t_gpoint = MCGPointMake(t_x, t_y);
    /* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(t_gpoint, r_point);
}

extern "C" MC_DLLEXPORT void MCWidgetGetClickPosition(bool p_current, MCCanvasPointRef& r_point)
{
    if (MCwidgetobject == nil)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("no current widget"), nil);
        return;
    }
    
    // TODO - coordinate transforms
    
    coord_t t_x, t_y;
    if (p_current)
        MCwidgeteventmanager->GetAsynchronousClickPosition(t_x, t_y);
    else
        MCwidgeteventmanager->GetSynchronousClickPosition(t_x, t_y);
    
    MCRectangle t_rect = MCwidgetobject->getrect();
    t_x -= t_rect.x;
    t_y -= t_rect.y;
    
    MCGPoint t_gpoint;
    t_gpoint = MCGPointMake(t_x, t_y);
    /* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(t_gpoint, r_point);
}

extern "C" MC_DLLEXPORT void MCWidgetGetMouseButtonState(uinteger_t p_index, MCPressedStateRef r_state)
{
    if (MCwidgetobject == nil)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("no current widget"), nil);
        return;
    }
    
    // TODO: implement
    MCAssert(false);
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
