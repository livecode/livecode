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
#include "undolst.h"

#include "widget-ref.h"
#include "widget-events.h"

#include "module-canvas.h"

#include "module-engine.h"

#include "dispatch.h"
#include "graphics_util.h"

#include "native-layer.h"

#include "stackfileformat.h"

////////////////////////////////////////////////////////////////////////////////

void MCCanvasPush(MCGContextRef gcontext, uintptr_t& r_cookie);
void MCCanvasPop(uintptr_t p_cookie);

////////////////////////////////////////////////////////////////////////////////

MCWidgetRef MCcurrentwidget;

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
    m_kind = nil;
    m_rep = nil;
    m_widget = nil;
}

MCWidget::MCWidget(const MCWidget& p_other) :
  MCControl(p_other)
{
    m_kind = nil;
    m_rep = nil;
    m_widget = nil;
}

MCWidget::~MCWidget(void)
{
    MCValueRelease(m_widget);
    MCValueRelease(m_kind);
    MCValueRelease(m_rep);
}

void MCWidget::bind(MCNameRef p_kind, MCValueRef p_rep)
{
    bool t_success;
    t_success = true;
    
    // Make sure we set the widget barrier callbacks - this should be done in
    // module init for 'extension' when we have such a mechanism.
    MCScriptSetWidgetBarrierCallbacks(MCWidgetEnter, MCWidgetLeave);
    
    // Create a new root widget.
    if (t_success)
        t_success = MCWidgetCreateRoot(this, p_kind, m_widget);
    
    // Load in a previously saved rep (if any)
    if (t_success && p_rep != nil)
        t_success = MCWidgetOnLoad(m_widget, p_rep);
    
    // Make sure it is in sync with the current state of this object.
    if (t_success && opened != 0)
        MCwidgeteventmanager -> event_open(this);
    
    // We always record the kind.
    m_kind = MCValueRetain(p_kind);
    
    // If we failed then store the rep and destroy the imp.
    if (!t_success)
    {
        // Make sure we swallow the error so that it doesn't affect
        // future execution.
        MCAutoErrorRef t_error;
        MCErrorCatch(&t_error);
        
        MCValueRelease(m_widget);
        m_widget = nil;
        
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

void MCWidget::kfocus(void)
{
	MCControl::kfocus();
	if (getstate(CS_KFOCUSED))
        if (m_widget != nil)
            MCwidgeteventmanager->event_kfocus(this);
}

void MCWidget::kunfocus(void)
{
	if (getstate(CS_KFOCUSED))
        if (m_widget != nil)
            MCwidgeteventmanager->event_kunfocus(this);
	MCControl::kunfocus();
}

Boolean MCWidget::kdown(MCStringRef p_key_string, KeySym p_key)
{
    // Only send the key down event to the widget if in browse mode
    if (m_widget != nil && getstack() -> gettool(this) == T_BROWSE)
        if (MCwidgeteventmanager->event_kdown(this, p_key_string, p_key))
            return True;

	return MCControl::kdown(p_key_string, p_key);
}

Boolean MCWidget::kup(MCStringRef p_key_string, KeySym p_key)
{
    // Only send the key up event to the widget if in browse mode
    if (m_widget != nil && getstack() -> gettool(this) == T_BROWSE)
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
        if (m_widget != nil)
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
        if (m_widget != nil)
            MCwidgeteventmanager->event_mup(this, p_which, p_release);
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
	if (!(getflag(F_VISIBLE) || showinvisible()) ||
		(getflag(F_DISABLED) && (getstack() -> gettool(this) == T_BROWSE)))
		return False;
	
	if (getstack() -> gettool(this) != T_BROWSE ||
#ifdef WIDGETS_HANDLE_DND
        false)
#else
        MCdispatcher -> isdragtarget())
#endif
		return MCControl::mfocus(p_x, p_y);
	
	// Update the mouse loc.
	mx = p_x;
	my = p_y;
    
    if (m_widget != nil)
        return MCwidgeteventmanager->event_mfocus(this, p_x, p_y);
    
    return False;
}

void MCWidget::munfocus(void)
{
	if (getstack() -> gettool(this) != T_BROWSE ||
#ifdef WIDGETS_HANDLE_DND
        false)
#else
        MCdispatcher -> isdragtarget())
#endif
	{
		MCControl::munfocus();
		return;
	}
	
    if (m_widget != nil)
        MCwidgeteventmanager->event_munfocus(this);
}

void MCWidget::mdrag(void)
{
#ifdef WIDGETS_HANDLE_DND
    if (m_widget != nil)
        MCwidgeteventmanager->event_mdrag(this);
#else
	MCControl::mdrag();
#endif
}

Boolean MCWidget::doubledown(uint2 p_which)
{
    if (m_widget != nil)
        return MCwidgeteventmanager->event_doubledown(this, p_which);
    return False;
}

Boolean MCWidget::doubleup(uint2 p_which)
{
    if (m_widget != nil)
        return MCwidgeteventmanager->event_doubleup(this, p_which);
    return False;
}

MCObject* MCWidget::hittest(int32_t x, int32_t y)
{
    if (!(flags & F_VISIBLE || showinvisible()) ||
        (flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE))
        return nil;
    
    if (m_widget != nil)
        return MCwidgeteventmanager->event_hittest(this, x, y);
    return nil;
}

void MCWidget::timer(MCNameRef p_message, MCParameter *p_parameters)
{
    if (p_message == MCM_internal)
    {
        if (m_widget != nil)
            MCwidgeteventmanager->event_timer(this, p_message, p_parameters);
    }
    else
    {
        MCControl::timer(p_message, p_parameters);
    }
}

void MCWidget::recompute(void)
{
    if (m_widget != nil)
        MCwidgeteventmanager->event_recompute(this);
}

static void lookup_name_for_prop(Properties p_which, MCNameRef& r_name)
{
    extern const LT factor_table[];
    extern const uint4 factor_table_size;
    for(uindex_t i = 0; i < factor_table_size; i++)
        if (factor_table[i] . type == TT_PROPERTY && factor_table[i] . which == p_which)
        {
            r_name = MCNAME(factor_table[i] . token);
            return;
        }
	
	extern bool lookup_property_override_name(uint16_t p_property, MCNameRef &r_name);
	if (lookup_property_override_name(p_which, r_name))
		return;

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
            		case P_FORE_PIXEL:
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
             /*case P_TEXT_HEIGHT:
             case P_TEXT_ALIGN:*/
        case P_TEXT_FONT:
        case P_TEXT_SIZE:
        case P_TEXT_STYLE:
		case P_LOCK_LOCATION:
		case P_VISIBLE:
		case P_INVISIBLE:
        case P_ENABLED:
        case P_DISABLED:
		case P_SELECTED:
		case P_TRAVERSAL_ON:
		case P_OWNER:
		case P_SHORT_OWNER:
		case P_ABBREV_OWNER:
		case P_LONG_OWNER:
		case P_PROPERTIES:
		case P_CUSTOM_PROPERTIES:
		case P_CUSTOM_PROPERTY_SET:
		case P_CUSTOM_PROPERTY_SETS:
        case P_CUSTOM_KEYS:
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
        case P_LAYER_CLIP_RECT:
            
        // Development mode only
        case P_REV_AVAILABLE_HANDLERS:
        case P_REV_AVAILABLE_VARIABLES:
    
        case P_KIND:
        case P_THEME_CONTROL_TYPE:
			return MCControl::getprop(ctxt, p_part_id, p_which, p_index, p_effective, r_value);
            
        default:
            break;
    }

    MCNewAutoNameRef t_name_for_prop;
    /* UNCHECKED */ lookup_name_for_prop(p_which, &t_name_for_prop);
    
    // Forward to the custom property handler
    return getcustomprop(ctxt, kMCEmptyName, *t_name_for_prop, nil, r_value);
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
            		case P_FORE_PIXEL:
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
             /*case P_TEXT_HEIGHT:
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
		case P_CUSTOM_PROPERTIES:
		case P_CUSTOM_PROPERTY_SET:
		case P_CUSTOM_PROPERTY_SETS:
        case P_CUSTOM_KEYS:
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
        case P_ENABLED:
        case P_DISABLED:
        case P_LAYER_CLIP_RECT:
            
        case P_KIND:
        case P_THEME_CONTROL_TYPE:
			return MCControl::setprop(ctxt, p_part_id, p_which, p_index, p_effective, p_value);
            
        default:
            break;
    }
    
    MCNewAutoNameRef t_name_for_prop;
    /* UNCHECKED */ lookup_name_for_prop(p_which, &t_name_for_prop);
    
    // Forward to the custom property handler
    return setcustomprop(ctxt, kMCEmptyName, *t_name_for_prop, nil, p_value);
}

static bool is_custom_prop(MCWidgetRef self, MCNameRef p_set_name, MCNameRef p_prop_name, MCProperListRef p_path, bool p_is_get_operation)
{
    if (self == nil)
        return true;
    
    if (!MCNameIsEmpty(p_set_name))
        return true;
    
    if (p_path == nil)
        return !MCWidgetHasProperty(self, p_prop_name);
    
    return !MCWidgetHasPropertyOfChunk(self, p_prop_name, MCNAME("Element"), p_is_get_operation);
}

bool MCWidget::getcustomprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCProperListRef p_path, MCExecValue& r_value)
{
    // Treat as a normal custom property if not a widget property
    if (is_custom_prop(m_widget, p_set_name, p_prop_name, p_path, true))
        return MCObject::getcustomprop(ctxt, p_set_name, p_prop_name, p_path, r_value);
    
    bool t_success;
    t_success = true;
    
    MCValueRef t_value;
    if (t_success)
    {
        if (p_path != nil)
        {
            t_success = MCWidgetGetPropertyOfChunk(m_widget, p_prop_name, MCNAME("Element"), p_path, t_value);
        }
        else
        {
            t_success = MCWidgetGetProperty(m_widget, p_prop_name, t_value);
        }
    }
    
    if (!t_success)
    {
        CatchError(ctxt);
        return false;
    }
    
    if (!MCExtensionConvertToScriptType(ctxt, t_value))
    {
        MCValueRelease(t_value);
        CatchError(ctxt);
        return false;
    }
    
    r_value.valueref_value = t_value;
    r_value.type = kMCExecValueTypeValueRef;
    
    return true;
}

bool MCWidget::setcustomprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCProperListRef p_path, MCExecValue p_value)
{
    // Treat as a normal custom property if not a widget property
    if (is_custom_prop(m_widget, p_set_name, p_prop_name, p_path, false))
        return MCObject::setcustomprop(ctxt, p_set_name, p_prop_name, p_path, p_value);
    
    MCAutoValueRef t_value;
    MCExecTypeConvertToValueRefAndReleaseAlways(ctxt, p_value.type, &p_value.valueref_value, Out(t_value));
    if (ctxt . HasError())
        return false;
    
    MCTypeInfoRef t_get_type, t_set_type;
    if (p_path != nil)
    {
        if (!MCWidgetQueryPropertyOfChunk(m_widget, p_prop_name, MCNAME("Element"), false, t_set_type))
            return false;
    }
    else
    {
        if (!MCWidgetQueryProperty(m_widget, p_prop_name, t_get_type, t_set_type))
            return false;
    }
    
    if (t_set_type != nil &&
        !MCExtensionConvertFromScriptType(ctxt, t_set_type, InOut(t_value)))
    {
        CatchError(ctxt);
        
        Exec_errors t_error;
        bool t_throw = false;
        
        MCResolvedTypeInfo t_resolved_type;
        if (!MCTypeInfoResolve(t_set_type, t_resolved_type))
            return false;
        
        if ( t_resolved_type . named_type == kMCBooleanTypeInfo)
        {
            t_error = EE_PROPERTY_NAB;
            t_throw = true;
        }
        else if (t_resolved_type . named_type == kMCNumberTypeInfo)
        {
            t_error = EE_PROPERTY_NAN;
            t_throw = true;
        }
        else if (t_resolved_type . named_type == kMCStringTypeInfo)
        {
            t_error = EE_PROPERTY_NAS;
            t_throw = true;
        }
        else if (t_resolved_type . named_type == kMCArrayTypeInfo || t_resolved_type . named_type == kMCProperListTypeInfo)
        {
            t_error = EE_PROPERTY_NOTANARRAY;
            t_throw = true;
        }
        else if (t_resolved_type . named_type == kMCDataTypeInfo)
        {
            t_error = EE_PROPERTY_NOTADATA;
            t_throw = true;
        }
        
        if (t_throw)
            ctxt . LegacyThrow(t_error);
        return false;
    }
    
    if (p_path != nil)
    {
        if (!MCWidgetSetPropertyOfChunk(m_widget, p_prop_name, MCNAME("Element"), p_path, In(t_value)))
        {
            CatchError(ctxt);
            return false;
        }
    }
    else
    {
        if (!MCWidgetSetProperty(m_widget, p_prop_name, In(t_value)))
        {
            CatchError(ctxt);
            return false;
        }
    }
    
    return true;
}

void MCWidget::toolchanged(Tool p_new_tool)
{
	MCControl::toolchanged(p_new_tool);

	if (m_widget != nil)
		MCwidgeteventmanager -> event_toolchanged(this, p_new_tool);
}

void MCWidget::layerchanged()
{
	MCControl::layerchanged();

	if (m_widget != nil)
		MCwidgeteventmanager -> event_layerchanged(this);
}

void MCWidget::visibilitychanged(bool p_visible)
{
	MCControl::visibilitychanged(p_visible);
	
	if (m_widget != nil)
		MCwidgeteventmanager -> event_visibilitychanged(this, p_visible);
}

void MCWidget::geometrychanged(const MCRectangle &p_rect)
{
	MCControl::geometrychanged(p_rect);

	if (m_widget != nil)
		MCwidgeteventmanager -> event_setrect(this, p_rect);
}

Boolean MCWidget::del(bool p_check_flag)
{
    // MCControl::del will check deletable
    if (!MCControl::del(p_check_flag))
        return False;

    // Make sure we release the widget ref here. Otherwise its
    // module cannot be released until the pending object pools
    // are drained.
    if (m_widget != nil)
    {
        MCValueRelease(m_widget);
        m_widget = nil;
    }
    
    return True;
}

void MCWidget::undo(Ustruct *us)
{
    MCControl::undo(us);

    if (us->type == UT_REPLACE)
    {
        MCAutoValueRef t_rep;
        t_rep.Give(m_rep);
        m_rep = nullptr;
        
        MCNewAutoNameRef t_kind;
        t_kind.Give(m_kind);
        m_kind = nullptr;
        
        bind(*t_kind, *t_rep);
    }
}

Boolean MCWidget::delforundo(bool p_check_flag)
{
    MCAutoValueRef t_rep;
    // Make the widget generate a rep.
    if (m_rep == nullptr)
    {
        if (m_widget != nullptr)
        {
            MCWidgetOnSave(m_widget, &t_rep);
        }
        
        if (!t_rep.IsSet())
        {
            t_rep = kMCNull;
        }
    }
    
    if (!del(p_check_flag))
    {
        return false;
    }
    
    if (t_rep.IsSet())
    {
        m_rep = t_rep.Take();
    }
    return true;
}


void MCWidget::OnOpen()
{
	if (m_widget != nil)
		MCwidgeteventmanager -> event_open(this);
	
	MCControl::OnOpen();
}

void MCWidget::OnClose()
{
	MCControl::OnClose();
	
	if (m_widget != nil)
		MCwidgeteventmanager -> event_close(this);
}

Exec_stat MCWidget::handle(Handler_type p_type, MCNameRef p_method, MCParameter *p_parameters, MCObject *p_passing_object)
{
	return MCControl::handle(p_type, p_method, p_parameters, p_passing_object);
}

uint32_t MCWidget::getminimumstackfileversion(void)
{
	return kMCStackFileFormatVersion_8_0;
}

IO_stat MCWidget::load(IO_handle p_stream, uint32_t p_version)
{
	IO_stat t_stat;
    
	if ((t_stat = MCObject::load(p_stream, p_version)) != IO_NORMAL)
		return checkloadstat(t_stat);
    
    MCNewAutoNameRef t_kind;
    if ((t_stat = IO_read_nameref_new(&t_kind, p_stream, true)) != IO_NORMAL)
        return checkloadstat(t_stat);
    
    MCAutoValueRef t_rep;
    if ((t_stat = IO_read_valueref_new(&t_rep, p_stream)) != IO_NORMAL)
        return checkloadstat(t_stat);
    
    if (t_stat == IO_NORMAL)
    {
        MCValueRef t_actual_rep;
        if (*t_rep != kMCNull)
            t_actual_rep = *t_rep;
        else
            t_actual_rep = nil;
        
        bind(*t_kind, t_actual_rep);
    }
    
	if ((t_stat = loadpropsets(p_stream, p_version)) != IO_NORMAL)
		return checkloadstat(t_stat);
    
    return checkloadstat(t_stat);
}

IO_stat MCWidget::save(IO_handle p_stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	/* If the file format doesn't support widgets, skip the widget */
	if (p_version < kMCStackFileFormatVersion_8_0)
	{
		return IO_NORMAL;
	}

    // Make the widget generate a rep.
	MCAutoValueRef t_rep;
	if (m_widget != nil)
    {
		MCWidgetOnSave(m_widget, &t_rep);
        // A widget might not have an OnSave handler (e.g. colorswatch widget)
        if (*t_rep == nil)
            t_rep = kMCNull;
    }
	else if (m_rep != nil)
		t_rep = m_rep;
	else
	{
		// If the rep is nil, then an error must have been thrown, so we still
		// save, but without any state for this widget.
		t_rep = kMCNull;
	}
	
    // The state of the IO.
    IO_stat t_stat;
    
    // First the widget code.
    if ((t_stat = IO_write_uint1(OT_WIDGET, p_stream)) != IO_NORMAL)
        return t_stat;
    
    // Save the object state.
    if ((t_stat = MCObject::save(p_stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
		return t_stat;
    
    // Now the widget kind.
    if ((t_stat = IO_write_nameref_new(m_kind, p_stream, true)) != IO_NORMAL)
        return t_stat;
    
    // Now the widget's rep.
    if ((t_stat = IO_write_valueref_new(*t_rep, p_stream)) != IO_NORMAL)
        return t_stat;
    
    if ((t_stat = savepropsets(p_stream, p_version)) != IO_NORMAL)
        return t_stat;
    
    // We are done.
    return t_stat;
}

MCControl *MCWidget::clone(Boolean p_attach, Object_pos p_position, bool invisible)
{
	MCWidget *t_new_widget;
	t_new_widget = new (nothrow) MCWidget(*this);
	if (p_attach)
		t_new_widget -> attach(p_position, invisible);
    
    MCAutoValueRef t_rep;
    // AL-2015-09-08: [[ Bug 15897 ]] Ensure m_widget is not nil before fetching save state
    if (m_widget != nil)
        MCWidgetOnSave(m_widget, &t_rep);
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

    if (m_widget != nil)
    {
        if (dc -> gettype() != CONTEXT_TYPE_PRINTER)
        {
            MCGContextRef t_gcontext;
            t_gcontext = ((MCGraphicsContext *)dc) -> getgcontextref();
            MCwidgeteventmanager->event_paint(this, t_gcontext);
        }
        else
        {
            bool t_success;
            t_success = true;
            
            // Create a raster to draw into.
            MCGRaster t_raster;
            t_raster . format = kMCGRasterFormat_ARGB;
            t_raster . width = dirty . width;
            t_raster . height = dirty . height;
            t_raster . stride = t_raster . width * sizeof(uint32_t);
            if (t_success)
                t_success = MCMemoryAllocate(t_raster . height * t_raster . stride, t_raster . pixels);
            
            MCGContextRef t_gcontext;
            t_gcontext = nil;
            if (t_success)
            {
                memset(t_raster . pixels, 0, t_raster . height * t_raster . stride);
                t_success = MCGContextCreateWithRaster(t_raster, t_gcontext);
            }
            
            MCGImageRef t_image;
            t_image = nil;
            if (t_success)
            {
                MCGContextTranslateCTM(t_gcontext, -dirty . x, -dirty . y);
                
                MCGContextSave(t_gcontext);
                MCwidgeteventmanager->event_paint(this, t_gcontext);
                MCGContextRestore(t_gcontext);
                
                t_success = MCGImageCreateWithRasterAndRelease(t_raster, t_image);
                if (t_success)
                    t_raster . pixels = NULL;
            }
            
            if (t_success)
            {
                MCImageDescriptor t_descriptor;
                memset(&t_descriptor, 0, sizeof(MCImageDescriptor));
                t_descriptor . image = t_image;
                t_descriptor . x_scale = t_descriptor . y_scale = 1.0;
                dc -> drawimage(t_descriptor, 0, 0, dirty . width, dirty . height, dirty . x, dirty . y);
            }
            
            MCGContextRelease(t_gcontext);
            MCGImageRelease(t_image);
            MCMemoryDeallocate(t_raster . pixels);
        }
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
	}
}

Boolean MCWidget::maskrect(const MCRectangle& p_rect)
{
	if (!(getflag(F_VISIBLE) || showinvisible()))
		return False;

	MCRectangle drect = MCU_intersect_rect(p_rect, rect);

	return drect.width != 0 && drect.height != 0;
}

void MCWidget::SetName(MCExecContext& ctxt, MCStringRef p_name)
{
    MCNewAutoNameRef t_old_name = getname();

    MCControl::SetName(ctxt, p_name);
    
    if (!MCNameIsEqualTo(*t_old_name, getname(), kMCStringOptionCompareExact))
    {
        recompute();
    }
}

void MCWidget::SetDisabled(MCExecContext& ctxt, uint32_t p_part_id, bool p_flag)
{
    bool t_is_disabled;
    t_is_disabled = getflag(F_DISABLED);
    
    MCControl::SetDisabled(ctxt, p_part_id, p_flag);
    
    if (t_is_disabled != getflag(F_DISABLED))
        recompute();
}

MCWidgetRef MCWidget::getwidget(void) const
{
    return m_widget;
}

void MCWidget::SendError(void)
{
    MCExecContext ctxt(this, nil, nil);
    MCExtensionCatchError(ctxt);
    if (!MCerrorptr)
        MCerrorptr = this;
    senderror();
}

void MCWidget::CatchError(MCExecContext& ctxt)
{
    MCExtensionCatchError(ctxt);
}

////////////////////////////////////////////////////////////////////////////////

void MCWidget::GetKind(MCExecContext& ctxt, MCNameRef& r_kind)
{
    r_kind = MCValueRetain(m_kind);
}

void MCWidget::GetState(MCExecContext& ctxt, MCArrayRef& r_state)
{
    MCAutoValueRef t_value;
    if (!m_widget && m_rep)
    {
        if (!MCValueCopy(m_rep, &t_value))
        {
            r_state = MCValueRetain(kMCEmptyArray);
            return;
        }
    }
    else
    {
        if (!MCWidgetOnSave(m_widget,
                            &t_value))
        {
            r_state = MCValueRetain(kMCEmptyArray);
            return;
        }
    }
    
    if (!MCExtensionConvertToScriptType(ctxt, InOut(t_value)))
    {
        CatchError(ctxt);
        return;
    }
    
    r_state = (MCArrayRef)t_value . Take();
}

////////////////////////////////////////////////////////////////////////////////

bool MCWidget::isInRunMode()
{
    Tool t_tool = getstack() -> gettool(this);
    return t_tool == T_BROWSE || t_tool == T_HELP;
}

////////////////////////////////////////////////////////////////////////////////

void MCWidget::SetFocused(bool p_setting)
{
    if (p_setting)
        focused = this;
    else if (focused.IsBoundTo(this))
        focused = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
