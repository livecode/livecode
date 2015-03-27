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

#include "widget-events.h"

#include "module-canvas.h"

#include "module-engine.h"

#include "dispatch.h"
#include "graphics_util.h"

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

MCWidget::MCWidget(void)
{
    m_kind = MCValueRetain(kMCEmptyName);
    m_instance = nil;
    m_native_layer = nil;
    m_rep = nil;
    m_timer_deferred = false;
}

MCWidget::MCWidget(const MCWidget& p_other) :
  MCControl(p_other)
{
    m_kind = MCValueRetain(kMCEmptyName);
    m_instance = nil;
    m_native_layer = nil;
    m_rep = nil;
    m_timer_deferred = false;
}

MCWidget::~MCWidget(void)
{
    OnDestroy();
    if (m_instance != nil)
        MCScriptReleaseInstance(m_instance);
    if (m_rep != nil)
        MCValueRelease(m_rep);
    MCValueRelease(m_kind);
}

void MCWidget::bind(MCNameRef p_kind, MCValueRef p_rep)
{
    // Assign the kind.
    MCValueAssign(m_kind, p_kind);
    
    // Attempt to find the module and build an instance (lookup module is a get
    // whereas creating an instance is a copy).
    MCScriptModuleRef t_module;
    MCScriptInstanceRef t_instance;
    if (MCScriptLookupModule(p_kind, t_module) &&
        MCScriptEnsureModuleIsUsable(t_module) &&
        MCScriptCreateInstanceOfModule(t_module, t_instance))
    {
        // Set the instance.
        m_instance = t_instance;
        
        // Now create.
        OnCreate();
        
        // Now load the rep.
        if (p_rep != nil)
            OnLoad(p_rep);
        
        // If we are open, then open.
        if (opened != 0)
        {
            OnOpen();
            if (MCcurtool != T_BROWSE)
                OnToolChanged(MCcurtool);
        }
    }
    else
    {
        // There is no module, or the module is not usable, so just store the
        // kind and the rep.
        if (p_rep != nil)
            m_rep = MCValueRetain(p_rep);
    }
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

bool MCWidget::visit_self(MCObjectVisitor* p_visitor)
{
    return p_visitor -> OnWidget(this);
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
    if (p_message == MCM_internal)
    {
        if (getstack() -> gettool(this) == T_BROWSE)
            OnTimer();
        else
            m_timer_deferred = true;
    }
    else
    {
        MCControl::timer(p_message, p_parameters);
        //MCwidgeteventmanager->event_timer(this, p_message, p_parameters);
    }
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
             case P_TEXT_ALIGN:*/
        case P_TEXT_FONT:
        case P_TEXT_SIZE:
        case P_TEXT_STYLE:
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
    return getcustomprop(ctxt, kMCEmptyName, *t_name_for_prop, r_value);
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
             case P_TEXT_ALIGN:*/
             case P_TEXT_FONT:
             case P_TEXT_SIZE:
             case P_TEXT_STYLE:
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
    return setcustomprop(ctxt, kMCEmptyName, *t_name_for_prop, p_value);
}

bool MCWidget::getcustomprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCExecValue& r_value)
{
    // Treat as a normal custom property if not a widget property
    MCTypeInfoRef t_getter, t_setter;
    if (m_instance == nil || !MCNameIsEmpty(p_set_name) || !MCScriptQueryPropertyOfModule(MCScriptGetModuleOfInstance(m_instance), p_prop_name, t_getter, t_setter))
        return MCObject::getcustomprop(ctxt, p_set_name, p_prop_name, r_value);
    
    if (CallGetProp(ctxt, p_prop_name, nil, r_value.valueref_value))
    {
        r_value.type = kMCExecValueTypeValueRef;
        return true;
    }
    
    MCExtensionCatchError(ctxt);
    
    return false;
}

bool MCWidget::setcustomprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCExecValue p_value)
{
    // Treat as a normal custom property if not a widget property
    MCTypeInfoRef t_getter, t_setter;
    if (m_instance == nil || !MCNameIsEmpty(p_set_name) || !MCScriptQueryPropertyOfModule(MCScriptGetModuleOfInstance(m_instance), p_prop_name, t_getter, t_setter))
        return MCObject::setcustomprop(ctxt, p_set_name, p_prop_name, p_value);
    
    MCAutoValueRef t_value;
    if (MCExecTypeIsValueRef(p_value . type))
        t_value = p_value . valueref_value;
    else
        MCExecTypeConvertToValueRefAndReleaseAlways(ctxt, p_value.type, &p_value.valueref_value, &t_value);
    if (!ctxt.HasError())
    {
        if (CallSetProp(ctxt, p_prop_name, nil, *t_value))
            return true;
        
        MCExtensionCatchError(ctxt);
    }
    
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

IO_stat MCWidget::load(IO_handle p_stream, uint32_t p_version)
{
	IO_stat t_stat;
    
	if ((t_stat = MCObject::load(p_stream, p_version)) != IO_NORMAL)
		return t_stat;
    
    MCNewAutoNameRef t_kind;
    if ((t_stat = IO_read_nameref_new(&t_kind, p_stream, true)) != IO_NORMAL)
        return t_stat;
    
    MCAutoValueRef t_rep;
    if ((t_stat = IO_read_valueref_new(&t_rep, p_stream)) != IO_NORMAL)
        return t_stat;
    
    if (t_stat == IO_NORMAL)
    {
        MCValueRef t_actual_rep;
        if (*t_rep != kMCNull)
            t_actual_rep = *t_rep;
        else
            t_actual_rep = nil;
        
        bind(*t_kind, t_actual_rep);
    }
    
    return t_stat;
}

IO_stat MCWidget::save(IO_handle p_stream, uint4 p_part, bool p_force_ext)
{
    // Make the widget generate a rep.
    MCValueRef t_rep;
    t_rep = nil;
    OnSave(t_rep);
    
    // If the rep is nil, then an error must have been thrown, so we still
    // save, but without any state for this widget.
    if (t_rep == nil)
        t_rep = MCValueRetain(kMCNull);
    
    // The state of the IO.
    IO_stat t_stat;
    
    // First the widget code.
    if ((t_stat = IO_write_uint1(OT_WIDGET, p_stream)) != IO_NORMAL)
        return t_stat;
    
    // Save the object state.
	if ((t_stat = MCObject::save(p_stream, p_part, p_force_ext)) != IO_NORMAL)
		return t_stat;
    
    // Now the widget kind.
    if ((t_stat = IO_write_nameref_new(m_kind, p_stream, true)) != IO_NORMAL)
        return t_stat;
    
    // Now the widget's rep.
    if ((t_stat = IO_write_valueref_new(t_rep, p_stream)) != IO_NORMAL)
        return t_stat;
    
    // We are done.
    return t_stat;
}

MCControl *MCWidget::clone(Boolean p_attach, Object_pos p_position, bool invisible)
{
	MCWidget *t_new_widget;
	t_new_widget = new MCWidget(*this);
	if (p_attach)
		t_new_widget -> attach(p_position, invisible);
    
    MCAutoValueRef t_rep;
    OnSave(&t_rep);
    if (*t_rep == nil)
        t_rep = kMCNull;
    
    t_new_widget -> bind(m_kind, *t_rep);
    
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

    if (m_instance != nil)
    {
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
    }
    else
    {
        setforeground(dc, DI_BACK, False);
        dc->setbackground(MCscreen->getwhite());
        dc->setfillstyle(FillOpaqueStippled, nil, 0, 0);
        dc->fillrect(dirty);
        dc->setbackground(MCzerocolor);
        dc->setfillstyle(FillSolid, nil, 0, 0);
    }
    
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
    if (m_instance == nil)
        return false;
    
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(MCScriptGetModuleOfInstance(m_instance), MCNAME("OnMouseDown"), t_signature);
}

bool MCWidget::handlesMouseUp() const
{
    if (m_instance == nil)
        return false;
    
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(MCScriptGetModuleOfInstance(m_instance), MCNAME("OnMouseUp"), t_signature);
}

bool MCWidget::handlesMouseCancel() const
{
    if (m_instance == nil)
        return false;
    
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(MCScriptGetModuleOfInstance(m_instance), MCNAME("OnMouseCancel"), t_signature);
}

bool MCWidget::handlesKeyPress() const
{
    if (m_instance == nil)
        return false;
    
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(MCScriptGetModuleOfInstance(m_instance), MCNAME("OnKeyPress"), t_signature);
}

bool MCWidget::handlesTouches() const
{
    if (m_instance == nil)
        return false;
    
    return false;
}

bool MCWidget::wantsClicks() const
{
    if (m_instance == nil)
        return false;
    
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(MCScriptGetModuleOfInstance(m_instance), MCNAME("OnClick"), t_signature);
}

bool MCWidget::wantsTouches() const
{
    if (m_instance == nil)
        return false;
    
    return false;
}

bool MCWidget::wantsDoubleClicks() const
{
    if (m_instance == nil)
        return false;
    
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(MCScriptGetModuleOfInstance(m_instance), MCNAME("OnDoubleClick"), t_signature);
}

bool MCWidget::waitForDoubleClick() const
{
    if (m_instance == nil)
        return false;
    
    return false;
}

bool MCWidget::isDragSource() const
{
    if (m_instance == nil)
        return false;
    
    MCTypeInfoRef t_signature;
    return MCScriptQueryHandlerOfModule(MCScriptGetModuleOfInstance(m_instance), MCNAME("OnDragStart"), t_signature);
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
    
    // OnAttach handlers mustn't mutate the world, or cause re-entrancy so no
    // script access is allowed.
    MCEngineScriptObjectPreventAccess();
    CallHandler(MCNAME("OnAttach"), nil, 0);
    MCEngineScriptObjectAllowAccess();
}

void MCWidget::OnDetach()
{
    if (m_native_layer)
        m_native_layer->OnDetach();
    
    // OnAttach handlers mustn't mutate the world, or cause re-entrancy so no
    // script access is allowed.
    MCEngineScriptObjectPreventAccess();
    CallHandler(MCNAME("OnDetach"), nil, 0);
    MCEngineScriptObjectAllowAccess();
}

void MCWidget::OnPaint(MCDC* p_dc, const MCRectangle& p_rect)
{
    if (m_native_layer)
        m_native_layer->OnPaint(p_dc, p_rect);
    
    // Re-entering into the draw chain is distinctly unwise, so no script access
    // for OnPaint() handlers.
    MCEngineScriptObjectPreventAccess();
    
    uintptr_t t_cookie;
    MCCanvasPush(((MCGraphicsContext*)p_dc)->getgcontextref(), t_cookie);
    CallHandler(MCNAME("OnPaint"), nil, 0);
    MCCanvasPop(t_cookie);
    
    MCEngineScriptObjectAllowAccess();
}

void MCWidget::OnGeometryChanged(const MCRectangle& p_old_rect)
{
    if (m_native_layer)
        m_native_layer->OnGeometryChanged(p_old_rect);
    
    MCAutoValueRefArray t_params;
    t_params.New(0);
    
    /*MCNumberCreateWithReal(rect.x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(rect.y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithReal(rect.width, reinterpret_cast<MCNumberRef&>(t_params[2]));
    MCNumberCreateWithReal(rect.height, reinterpret_cast<MCNumberRef&>(t_params[3]));*/
    
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
    
    // In theory this handler shouldn't allow script access.
}

void MCWidget::OnBoundsTest(const MCRectangle& p_intersect, bool& r_hit)
{
    fprintf(stderr, "MCWidget::OnBoundsTest\n");
    r_hit = maskrect(p_intersect);
    
    // In theory this handler shouldn't allow script access.
}

void MCWidget::OnSave(MCValueRef& r_array)
{
    fprintf(stderr, "MCWidget::OnSave\n");
    
    MCAutoValueRefArray t_params;
    t_params.New(1);
    t_params[0] = nil;
    
    // OnSave handlers mustn't mutate the world, or cause re-entrancy so no
    // script access is allowed.
    MCEngineScriptObjectPreventAccess();
    CallHandler(MCNAME("OnSave"), t_params.Ptr(), t_params.Size());
    MCEngineScriptObjectAllowAccess();

    r_array = t_params[0];
    t_params[0] = nil;
}

void MCWidget::OnLoad(MCValueRef p_array)
{
    fprintf(stderr, "MCWidget::OnLoad\n");
    
    MCAutoValueRefArray t_params;
    t_params.New(1);
    t_params[0] = MCValueRetain(p_array);
    
    // OnLoad handlers mustn't mutate the world, or cause re-entrancy so no
    // script access is allowed.
    MCEngineScriptObjectPreventAccess();
    CallHandler(MCNAME("OnLoad"), t_params.Ptr(), t_params.Size());
    MCEngineScriptObjectAllowAccess();
}

void MCWidget::OnCreate()
{
    // OnCreate handlers mustn't mutate the world, or cause re-entrancy so no
    // script access is allowed.
    MCEngineScriptObjectPreventAccess();
    CallHandler(MCNAME("OnCreate"), nil, 0);
    MCEngineScriptObjectAllowAccess();
}

void MCWidget::OnDestroy()
{
    // OnCreate handlers mustn't mutate the world, or cause re-entrancy so no
    // script access is allowed.
    MCEngineScriptObjectPreventAccess();
    CallHandler(MCNAME("OnDestroy"), nil, 0);
    MCEngineScriptObjectAllowAccess();
}

void MCWidget::OnParentPropChanged()
{
    CallHandler(MCNAME("OnParentPropertyChanged"), nil, 0);
}

void MCWidget::OnToolChanged(Tool p_new_tool)
{
    if (m_native_layer)
        m_native_layer->OnToolChanged(p_new_tool);
    
    // When the tool changes we don't want to allow script access to ensure
    // no re-entrancy issues occur.
    MCEngineScriptObjectPreventAccess();
    if (p_new_tool == T_BROWSE)
        CallHandler(MCNAME("OnStopEditing"), nil, 0);
    else if (p_new_tool != T_BROWSE)
        CallHandler(MCNAME("OnStartEditing"), nil, 0);
    MCEngineScriptObjectAllowAccess();
    
    if (p_new_tool == T_BROWSE && m_timer_deferred)
    {
        m_timer_deferred = false;
        MCscreen -> addtimer(this, MCM_internal, 0);
    }
    
    fprintf(stderr, "MCWidget::OnToolChanged\n");
}

void MCWidget::OnLayerChanged()
{
    if (m_native_layer)
        m_native_layer->OnLayerChanged();
    
    CallHandler(MCNAME("OnLayerChanged"), nil, 0);
}

void MCWidget::OnTimer()
{
    CallHandler(MCNAME("OnTimer"), nil, 0);
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
    t_params.New(0);
    
    //MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    //MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    
    CallHandler(MCNAME("OnMouseMove"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnMouseCancel(uinteger_t p_button)
{
    MCAutoValueRefArray t_params;
    t_params.New(0);
    
    //MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[0]));
    
    CallHandler(MCNAME("OnMouseCancel"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnMouseDown(coord_t p_x, coord_t p_y , uinteger_t p_button)
{
    MCAutoValueRefArray t_params;
    t_params.New(0);
    
    //MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    //MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    //MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[2]));
    
    CallHandler(MCNAME("OnMouseDown"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnMouseUp(coord_t p_x, coord_t p_y, uinteger_t p_button)
{
    MCAutoValueRefArray t_params;
    t_params.New(0);
    
    //MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    //MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    //MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[2]));
    
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
    t_params.New(0);
    
    /*MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[2]));
    MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[3]));*/
    
    CallHandler(MCNAME("OnClick"), t_params.Ptr(), t_params.Size());
}

void MCWidget::OnDoubleClick(coord_t p_x, coord_t p_y, uinteger_t p_button)
{
    MCAutoValueRefArray t_params;
    t_params.New(0);
    
    /*MCNumberCreateWithReal(p_x, reinterpret_cast<MCNumberRef&>(t_params[0]));
    MCNumberCreateWithReal(p_y, reinterpret_cast<MCNumberRef&>(t_params[1]));
    MCNumberCreateWithUnsignedInteger(p_button, reinterpret_cast<MCNumberRef&>(t_params[2]));*/
    
    CallHandler(MCNAME("OnDoubleClick"), t_params.Ptr(), t_params.Size());
}

////////////////////////////////////////////////////////////////////////////////

bool MCWidget::CallHandler(MCNameRef p_name, MCValueRef* x_parameters, uindex_t p_param_count, MCValueRef* r_retval)
{
    if (m_instance == nil)
        return true;
    
	MCWidget *t_old_widget_object;
	t_old_widget_object = MCwidgetobject;
	MCwidgetobject = this;
	
	MCStack *t_old_default_stack, *t_this_stack;
	t_old_default_stack = MCdefaultstackptr;
	
	MCObject *t_old_target;
	t_old_target = MCtargetptr;
	
	MCtargetptr = this;
	t_this_stack = this->getstack();
	MCdefaultstackptr = t_this_stack;
	
    // Invoke event handler.
    bool t_success;
    MCValueRef t_retval;
    t_success = MCScriptCallHandlerOfInstanceIfFound(m_instance, p_name, x_parameters, p_param_count, t_retval);
    
    if (t_success)
    {
        if (r_retval != NULL)
            *r_retval = t_retval;
        else
            MCValueRelease(t_retval);
    }
    else
    {
        MCExecContext ctxt(this, nil, nil);
        MCExtensionCatchError(ctxt);
        if (MCerrorptr == NULL)
            MCerrorptr = this;
        senderror();
    }
    
	MCwidgetobject = t_old_widget_object;
    
	MCtargetptr = t_old_target;
	if (MCdefaultstackptr == t_this_stack)
		MCdefaultstackptr = t_old_default_stack;
	
	return t_success;
}

bool MCWidget::CallGetProp(MCExecContext& ctxt, MCNameRef p_property, MCNameRef p_key, MCValueRef& r_value)
{
    if (m_instance == nil)
    {
        r_value = MCValueRetain(kMCNull);
        return true;
    }
    
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

	return t_success;
}

bool MCWidget::CallSetProp(MCExecContext& ctxt, MCNameRef p_property, MCNameRef p_key, MCValueRef p_value)
{
    if (m_instance == nil)
    {
        return true;
    }
    
	MCWidget *t_old_widget_object;
	t_old_widget_object = MCwidgetobject;
	MCwidgetobject = this;
    
    // Convert to the appropriate type
    MCTypeInfoRef t_gettype, t_settype;
    if (!MCScriptQueryPropertyOfModule(MCScriptGetModuleOfInstance(m_instance), p_property, t_gettype, t_settype))
        return false;
    
    // TODO: Fix this - we should really throw a read-only property error here, but
    //   instead we'll let MCScriptSetPropertyOfInstance do it.
    if (t_settype != nil &&
        !MCExtensionConvertFromScriptType(ctxt, t_settype, p_value))
        return false;
    
    // Invoke event handler.
    bool t_success;
    t_success = MCScriptSetPropertyOfInstance(m_instance, p_property, p_value);
    
	MCwidgetobject = t_old_widget_object;

	return t_success;
}

void MCWidget::GetKind(MCExecContext& ctxt, MCNameRef& r_kind)
{
    r_kind = MCValueRetain(m_kind);
}

////////////////////////////////////////////////////////////////////////////////

extern MCValueRef MCEngineGetPropertyOfObject(MCExecContext &ctxt, MCStringRef p_property, MCObject *p_object, uint32_t p_part_id);
extern void MCEngineSetPropertyOfObject(MCExecContext &ctxt, MCStringRef p_property, MCObject *p_object, uint32_t p_part_id, MCValueRef p_value);

class MCWidgetPopup: public MCStack
{
public:
	MCWidgetPopup()
	{
		setname_cstring("Popup Widget");
		state |= CS_NO_MESSAGES;
		
		curcard = cards = NULL;
		curcard = cards = MCtemplatecard->clone(False, False);
		cards->setparent(this);
		cards->setstate(True, CS_NO_MESSAGES);
		
		parent = nil;
		
		m_font = nil;
		
		m_widget = nil;
		m_result = MCValueRetain(kMCNull);
	}
	
	~MCWidgetPopup(void)
	{
	}
	
	// This will be called when the stack is closed, either directly
	// or indirectly if the popup is cancelled by clicking outside
	// or pressing escape.
	void close(void)
	{
		MCStack::close();
		MCdispatcher -> removemenu();
	}
	
	virtual bool isopaque()
	{
		// Allow widget popups to have transparency.
		return false;
	}
	
	// This is called to render the stack.
	void render(MCContext *dc, const MCRectangle& dirty)
	{
		// Clear the target rectangle
		MCGContextRef t_context;
		dc->lockgcontext(t_context);
		
		MCGContextSetBlendMode(t_context, kMCGBlendModeClear);
		MCGContextAddRectangle(t_context, MCRectangleToMCGRectangle(dirty));
		MCGContextFill(t_context);
		
		dc->unlockgcontext(t_context);
		
		// Draw the widget
		if (m_widget != nil)
			m_widget->draw(dc, dirty, true, false);
	}
	
	//////////
	
	Boolean mdown(uint2 which)
	{
		if (MCU_point_in_rect(m_widget->getrect(), MCmousex, MCmousey))
			return MCStack::mdown(which);
			
		close();
		return True;
	}
	
	//////////
	
	bool openpopup(MCNameRef p_kind, const MCPoint &p_at, MCArrayRef p_properties)
	{
		if (!createwidget(p_kind, p_properties))
			return false;
		
		uint32_t t_width, t_height;
		getwidgetgeometry(t_width, t_height);
		
		MCdispatcher -> addmenu(this);
		m_widget->setrect(MCRectangleMake(0, 0, t_width, t_height));
		
		return ES_NORMAL == openrect(MCRectangleMake(p_at.x, p_at.y, t_width, t_height), WM_POPUP, NULL, WP_ASRECT, OP_NONE);
	}
	
	const MCWidget *getpopupwidget() const
	{
		return m_widget;
	}
	
	void setpopupresult(MCValueRef p_result)
	{
		MCValueAssign(m_result, p_result);
	}
	
	MCValueRef getpopupresult() const
	{
		return m_result;
	}
	
private:
	bool createwidget(MCNameRef p_kind, MCArrayRef p_properties)
	{
		if (m_widget != nil)
			return true;
		
		m_widget = new MCWidget();
		if (m_widget == nil)
			return MCErrorThrowOutOfMemory();
		
		m_widget->bind(p_kind, nil);
		
		appendcontrol(m_widget);
		curcard->newcontrol(m_widget, False);
		
		MCExecContext ctxt(MCdefaultstackptr, nil, nil);
		uintptr_t t_iter;
		t_iter = 0;
		
		MCNameRef t_key;
		MCValueRef t_value;
		
		while (MCArrayIterate(p_properties, t_iter, t_key, t_value))
		{
			MCEngineSetPropertyOfObject(ctxt, MCNameGetString(t_key), m_widget, 0, t_value);
			if (MCErrorIsPending())
				return false;
		}
		
		return true;
	}
	
	static bool WidgetGeometryFromLCBList(MCValueRef p_list, uint32_t &r_width, uint32_t &r_height)
	{
		// MCProperList gets converted to a sequence array
		if (!MCValueIsArray(p_list))
			return false;
		
		MCArrayRef t_array;
		t_array = (MCArrayRef)p_list;
		
		if (!MCArrayIsSequence(t_array) || MCArrayGetCount(t_array) != 2)
			return false;
		
		uint32_t t_width, t_height;
		MCValueRef t_value;
		if (!MCArrayFetchValueAtIndex(t_array, 1, t_value) || MCValueGetTypeCode(t_value) != kMCValueTypeCodeNumber)
			return false;
		
		t_width = MCNumberFetchAsUnsignedInteger((MCNumberRef)t_value);
		
		if (!MCArrayFetchValueAtIndex(t_array, 2, t_value) || MCValueGetTypeCode(t_value) != kMCValueTypeCodeNumber)
			return false;
		
		t_height = MCNumberFetchAsUnsignedInteger((MCNumberRef)t_value);
		
		r_width = t_width;
		r_height = t_height;
		
		return true;
	}
	
	bool getwidgetpreferredsize(uint32_t &r_width, uint32_t &r_height)
	{
		MCExecContext ctxt(MCdefaultstackptr, nil, nil);
		MCAutoValueRef t_value;
		t_value = MCEngineGetPropertyOfObject(ctxt, MCSTR("preferredSize"), m_widget, 0);
		if (MCErrorIsPending())
			return false;
		
		if (!WidgetGeometryFromLCBList(*t_value, r_width, r_height))
			return MCErrorCreateAndThrow(kMCWidgetSizeFormatErrorTypeInfo, nil);
		
		return true;
	}
	
	void getwidgetgeometry(uint32_t &r_width, uint32_t &r_height)
	{
		MCPoint t_size;
		if (!getwidgetpreferredsize(r_width, r_height))
		{
			MCRectangle t_rect;
			t_rect = m_widget->getrect();
			
			r_width = t_rect.width;
			r_height = t_rect.height;
		}
	}
	
	MCWidget *m_widget;
	MCValueRef m_result;
};

////////////////////////////////////////////////////////////////////////////////

static bool MCWidgetThrowNoCurrentWidgetError(void)
{
	return MCErrorCreateAndThrow(kMCWidgetNoCurrentWidgetErrorTypeInfo, nil);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void MCWidgetExecRedrawAll(void)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    MCwidgetobject -> layer_redrawall();
}

extern "C" MC_DLLEXPORT void MCWidgetExecScheduleTimerIn(double p_after)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    MCscreen -> cancelmessageobject(MCwidgetobject, MCM_internal);
    MCscreen -> addtimer(MCwidgetobject, MCM_internal, (uint4)(p_after * 1000));
}

extern "C" MC_DLLEXPORT void MCWidgetExecCancelTimer(void)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    MCscreen -> cancelmessageobject(MCwidgetobject, MCM_internal);
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
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    if (!MCEngineScriptObjectCreate(MCwidgetobject, 0, r_script_object))
        return;
}

extern "C" MC_DLLEXPORT MCValueRef MCWidgetExecSend(bool p_is_function, MCStringRef p_message)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return nil;
    }
    
    return MCEngineDoSendToObjectWithArguments(p_is_function, p_message, MCwidgetobject, kMCEmptyProperList);
}

extern "C" MC_DLLEXPORT MCValueRef MCWidgetExecSendWithArguments(bool p_is_function, MCStringRef p_message, MCProperListRef p_arguments)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return nil;
    }
    
    return MCEngineDoSendToObjectWithArguments(p_is_function, p_message, MCwidgetobject, p_arguments);
}

extern "C" MC_DLLEXPORT void MCWidgetExecPost(MCStringRef p_message)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    MCEngineDoPostToObjectWithArguments(p_message, MCwidgetobject, kMCEmptyProperList);
}

extern "C" MC_DLLEXPORT void MCWidgetExecPostWithArguments(MCStringRef p_message, MCProperListRef p_arguments)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    MCEngineDoPostToObjectWithArguments(p_message, MCwidgetobject, p_arguments);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void MCWidgetGetRectangle(MCCanvasRectangleRef& r_rect)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
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
        MCWidgetThrowNoCurrentWidgetError();
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
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    MCRectangle t_rect;
    MCGRectangle t_grect;
    t_rect = MCwidgetobject -> getrect();
    
    // Only the size is wanted
    t_grect = MCGRectangleMake(0, 0, t_rect.width, t_rect.height);
    MCCanvasRectangleCreateWithMCGRectangle(t_grect, r_rect);
}

extern "C" MC_DLLEXPORT void MCWidgetGetWidth(MCNumberRef& r_width)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    MCNumberCreateWithReal(MCwidgetobject->getrect().width, r_width);
}

extern "C" MC_DLLEXPORT void MCWidgetGetHeight(MCNumberRef& r_height)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    MCNumberCreateWithReal(MCwidgetobject->getrect().height, r_height);
}

extern "C" MC_DLLEXPORT void MCWidgetSetWidth(MCNumberRef p_width)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    MCRectangle t_rect = MCwidgetobject->getrect();
    t_rect.width = MCNumberFetchAsReal(p_width);
    MCwidgetobject->setrect(t_rect);
}

extern "C" MC_DLLEXPORT void MCWidgetSetHeight(MCNumberRef p_height)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    MCRectangle t_rect = MCwidgetobject->getrect();
    t_rect.height = MCNumberFetchAsReal(p_height);
    MCwidgetobject->setrect(t_rect);
}

extern "C" MC_DLLEXPORT void MCWidgetGetFont(MCCanvasFontRef& r_canvas_font)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    MCAutoCustomPointer<struct MCFont, MCFontRelease> t_font;
    if (!MCwidgetobject -> copyfont(&t_font))
        return;
    
    if (!MCCanvasFontCreateWithMCFont(*t_font, r_canvas_font))
        return;
}

extern "C" MC_DLLEXPORT void MCWidgetGetMousePosition(bool p_current, MCCanvasPointRef& r_point)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
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

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void MCWidgetGetClickPosition(bool p_current, MCCanvasPointRef& r_point)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
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

extern "C" MC_DLLEXPORT void MCWidgetGetClickButton(bool p_current, unsigned int& r_button)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
    // TODO: Implement asynchronous version.
    if (!p_current)
        MCwidgeteventmanager -> GetSynchronousClickButton(r_button);
    else
        MCErrorThrowGeneric(MCSTR("'the current click button' is not implemented yet"));
}

extern "C" MC_DLLEXPORT void MCWidgetGetClickCount(bool p_current, unsigned int& r_count)
{
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
        return;
    }
    
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
    if (MCwidgetobject == nil)
    {
        MCWidgetThrowNoCurrentWidgetError();
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

////////////////////////////////////////////////////////////////////////////////

static inline MCPoint _MCWidgetToStackLoc(MCWidget *p_widget, const MCPoint &p_point)
{
	MCRectangle t_rect;
	t_rect = p_widget->getrect();
	return MCPointMake(p_point.x + t_rect.x, p_point.y + t_rect.y);
}

class MCPopupMenuHandler : public MCButtonMenuHandler
{
public:
	MCPopupMenuHandler()
	{
		m_pick = nil;
	}
	
	~MCPopupMenuHandler()
	{
		MCValueRelease(m_pick);
	}
	
	virtual bool OnMenuPick(MCButton *p_button, MCValueRef p_pick, MCValueRef p_old_pick)
	{
		MCValueAssign(m_pick, p_pick);
		return true;
	}
	
	MCValueRef GetPick()
	{
		return m_pick;
	}
	
private:
	MCValueRef m_pick;
};

extern "C" MC_DLLEXPORT MCStringRef MCWidgetExecPopupMenuAtLocation(MCStringRef p_menu, MCCanvasPointRef p_at)
{
	if (MCwidgetobject == nil)
	{
		MCWidgetThrowNoCurrentWidgetError();
		return nil;
	}
	
	MCButton *t_button;
	t_button = nil;
	
	t_button = (MCButton*)MCtemplatebutton->clone(True, OP_NONE, true);
	if (t_button == nil)
	{
		MCErrorThrowOutOfMemory();
		return nil;
	}
	
	MCPopupMenuHandler t_handler;
	
	MCExecContext ctxt;
	
	t_button->setmenuhandler(&t_handler);
	
	t_button->SetStyle(ctxt, F_MENU);
	t_button->SetMenuMode(ctxt, WM_POPUP);
	t_button->SetText(ctxt, p_menu);
	
	MCPoint t_at;
	MCPoint *t_at_ptr;
	t_at_ptr = nil;
	
	if (p_at != nil)
	{
		MCGPoint t_point;
		MCCanvasPointGetMCGPoint(p_at, t_point);
		
		t_at = _MCWidgetToStackLoc(MCwidgetobject, MCGPointToMCPoint(t_point));
		t_at_ptr = &t_at;
	}
	
	MCInterfaceExecPopupButton(ctxt, t_button, t_at_ptr);

	while (t_button->menuisopen() && !MCquit)
	{
		MCU_resetprops(True);
		// MW-2011-09-08: [[ Redraw ]] Make sure we flush any updates.
		MCRedrawUpdateScreen();
		MCscreen->siguser();
		MCscreen->wait(REFRESH_INTERVAL, True, True);
	}
	
	t_button->SetVisible(ctxt, 0, false);
	t_button->del();
	t_button->scheduledelete();
	
	MCAutoStringRef t_string;
	
	if (t_handler.GetPick() != nil)
		ctxt.ConvertToString(t_handler.GetPick(), &t_string);
	
	return t_string.Take();
}

////////////////////////////////////////////////////////////////////////////////

static MCWidgetPopup *s_widget_popup = nil;

MCValueRef MCWidgetPopupAtLocationWithProperties(MCNameRef p_kind, const MCPoint &p_at, MCArrayRef p_properties)
{
	MCWidgetPopup *t_old_popup;
	t_old_popup = s_widget_popup;
	
	s_widget_popup = new MCWidgetPopup();
	if (s_widget_popup == nil)
	{
		// TODO - throw memory error
		s_widget_popup = t_old_popup;
		return nil;
	}
	
	s_widget_popup -> setparent(MCdispatcher);
	MCdispatcher -> add_transient_stack(s_widget_popup);
	
	if (!s_widget_popup->openpopup(p_kind, p_at, p_properties))
	{
		s_widget_popup->scheduledelete();
		s_widget_popup = t_old_popup;
		return nil;
	}
	
	while (s_widget_popup->getopened() && !MCquit)
	{
		MCU_resetprops(True);
		// MW-2011-09-08: [[ Redraw ]] Make sure we flush any updates.
		MCRedrawUpdateScreen();
		MCscreen->siguser();
		MCscreen->wait(REFRESH_INTERVAL, True, True);
	}
	
	MCValueRef t_result;
	t_result = MCValueRetain(s_widget_popup->getpopupresult());
	
	s_widget_popup->del();
	s_widget_popup->scheduledelete();
	
	s_widget_popup = t_old_popup;
	
	return t_result;
}

extern "C" MC_DLLEXPORT MCValueRef MCWidgetExecPopupAtLocationWithProperties(MCStringRef p_kind, MCCanvasPointRef p_at, MCArrayRef p_properties)
{
	if (MCwidgetobject == nil)
	{
		MCWidgetThrowNoCurrentWidgetError();
		return nil;
	}
	
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(p_at, t_point);
	
	MCPoint t_at;
	t_at = MCGPointToMCPoint(t_point);
	
	MCNewAutoNameRef t_kind;
	/* UNCHECKED */ MCNameCreate(p_kind, &t_kind);
	
	t_at = MCPointMake(t_at.x + MCwidgetobject->getrect().x, t_at.y + MCwidgetobject->getrect().y);
	t_at = MCwidgetobject->getstack()->stacktogloballoc(t_at);
	
	return MCWidgetPopupAtLocationWithProperties(*t_kind, t_at, p_properties);
}

extern "C" MC_DLLEXPORT MCValueRef MCWidgetExecPopupAtLocation(MCStringRef p_kind, MCCanvasPointRef p_at)
{
	return MCWidgetExecPopupAtLocationWithProperties(p_kind, p_at, kMCEmptyArray);
}



extern "C" MC_DLLEXPORT void MCWidgetEvalIsPopup(bool &r_popup)
{
	if (MCwidgetobject == nil)
	{
		MCWidgetThrowNoCurrentWidgetError();
		return;
	}
	
	r_popup = s_widget_popup != nil && MCwidgetobject == s_widget_popup->getpopupwidget();
}

extern "C" MC_DLLEXPORT void MCWidgetExecClosePopupWithResult(MCValueRef p_result)
{
	if (MCwidgetobject == nil)
	{
		MCWidgetThrowNoCurrentWidgetError();
		return;
	}
	
	bool t_is_popup;
	MCWidgetEvalIsPopup(t_is_popup);
	
	if (!t_is_popup)
	{
		// TODO - throw error
		return;
	}
	
	s_widget_popup->setpopupresult(p_result);
	s_widget_popup->close();
}

extern "C" MC_DLLEXPORT void MCWidgetExecClosePopup(MCValueRef p_result)
{
	MCWidgetExecClosePopupWithResult(kMCNull);
}

////////////////////////////////////////////////////////////////////////////////

bool MCErrorCreateNamedTypeInfo(MCNameRef p_domain, MCNameRef p_name, MCStringRef p_message, MCTypeInfoRef &r_error_type)
{
	MCAutoTypeInfoRef t_type, t_named_type;
	
	if (!MCErrorTypeInfoCreate(p_domain, p_message, &t_type))
		return false;
	
	if (!MCNamedTypeInfoCreate(p_name, &t_named_type))
		return false;
	
	if (!MCNamedTypeInfoBind(*t_named_type, *t_type))
		return false;
	
	r_error_type = MCValueRetain(*t_named_type);
	return true;
}

MCTypeInfoRef kMCWidgetNoCurrentWidgetErrorTypeInfo = nil;
MCTypeInfoRef kMCWidgetSizeFormatErrorTypeInfo = nil;

bool MCWidgetModuleInitialize(void)
{
	if (!MCErrorCreateNamedTypeInfo(MCNAME("widget"), MCNAME("com.livecode.widget.NoCurrentWidgetError"), MCSTR("No current widget."), kMCWidgetNoCurrentWidgetErrorTypeInfo))
		return false;
	
	if (!MCErrorCreateNamedTypeInfo(MCNAME("widget"), MCNAME("com.livecode.widget.WidgetSizeFormatError"), MCSTR("Size must be a list of two numbers"), kMCWidgetSizeFormatErrorTypeInfo))
		return false;
	
	return true;
}

void MCWidgetModuleFinalize(void)
{
	MCValueRelease(kMCWidgetNoCurrentWidgetErrorTypeInfo);
	kMCWidgetNoCurrentWidgetErrorTypeInfo = nil;
	
	MCValueRelease(kMCWidgetSizeFormatErrorTypeInfo);
	kMCWidgetSizeFormatErrorTypeInfo = nil;
}

////////////////////////////////////////////////////////////////////////////////
