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

#include "widget-ref.h"
#include "widget-events.h"

#include "module-canvas.h"

#include "module-engine.h"

#include "dispatch.h"
#include "graphics_util.h"

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
        MCValueRelease(m_widget);
        m_widget = nil;
        
        if (p_rep != nil)
            m_rep = MCValueRetain(p_rep);
        
        SendError();
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
    if (m_widget != nil)
        MCwidgeteventmanager->event_open(this);
}

void MCWidget::close(void)
{
    if (m_widget != nil)
        MCwidgeteventmanager->event_close(this);
	MCControl::close();
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
    if (m_widget != nil)
        if (MCwidgeteventmanager->event_kdown(this, p_key_string, p_key))
            return True;

	return MCControl::kdown(p_key_string, p_key);
}

Boolean MCWidget::kup(MCStringRef p_key_string, KeySym p_key)
{
    if (m_widget != nil)
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
	if (!(getflag(F_VISIBLE) || MCshowinvisibles) ||
		(getflag(F_DISABLED) && (getstack() -> gettool(this) == T_BROWSE)))
		return False;
	
	if (getstack() -> gettool(this) != T_BROWSE)
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
	if (getstack() -> gettool(this) != T_BROWSE)
	{
		MCControl::munfocus();
		return;
	}
	
    if (m_widget != nil)
        MCwidgeteventmanager->event_munfocus(this);
}

void MCWidget::mdrag(void)
{
    if (m_widget != nil)
        MCwidgeteventmanager->event_mdrag(this);
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

void MCWidget::setrect(const MCRectangle& p_rectangle)
{
	MCRectangle t_old_rect;
	t_old_rect = rect;
	
	rect = p_rectangle;
	
    if (m_widget != nil)
        MCwidgeteventmanager->event_setrect(this, t_old_rect);
}

void MCWidget::recompute(void)
{
    if (m_widget != nil)
        MCwidgeteventmanager->event_recompute(this);
}

static void lookup_name_for_prop(Properties p_which, MCNameRef& r_name)
{
    extern LT factor_table[];
    extern const uint4 factor_table_size;
    for(uindex_t i = 0; i < factor_table_size; i++)
        if (factor_table[i] . type == TT_PROPERTY && factor_table[i] . which == p_which)
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
        case P_ENABLED:
        case P_DISABLED:
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
        case P_ENABLED:
        case P_DISABLED:
            
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
    if (m_widget == nil || !MCNameIsEmpty(p_set_name) || !MCWidgetHasProperty(m_widget, p_prop_name))
        return MCObject::getcustomprop(ctxt, p_set_name, p_prop_name, r_value);
    
    MCValueRef t_value;
    if (!MCWidgetGetProperty(m_widget, p_prop_name, t_value) ||
        !MCExtensionConvertToScriptType(ctxt, t_value))
    {
        MCValueRelease(t_value);
        CatchError(ctxt);
        return false;
    }
    
    r_value.valueref_value = t_value;
    r_value.type = kMCExecValueTypeValueRef;
    
    return true;
}

bool MCWidget::setcustomprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCExecValue p_value)
{
    // Treat as a normal custom property if not a widget property
    if (m_widget == nil || !MCNameIsEmpty(p_set_name) || !MCWidgetHasProperty(m_widget, p_prop_name))
        return MCObject::setcustomprop(ctxt, p_set_name, p_prop_name, p_value);
    
    MCAutoValueRef t_value;
    if (MCExecTypeIsValueRef(p_value . type))
        t_value = p_value . valueref_value;
    else
    {
        MCExecTypeConvertToValueRefAndReleaseAlways(ctxt, p_value.type, &p_value.valueref_value, Out(t_value));
        if (ctxt . HasError())
            return false;
    }
    
    MCTypeInfoRef t_get_type, t_set_type;
    if (!MCWidgetQueryProperty(m_widget, p_prop_name, t_get_type, t_set_type))
        return false;
    
    if (t_set_type != nil &&
        !MCExtensionConvertFromScriptType(ctxt, t_set_type, InOut(t_value)))
    {
        CatchError(ctxt);
        return false;
    }
    
    if (!MCWidgetSetProperty(m_widget, p_prop_name, In(t_value)))
    {
        CatchError(ctxt);
        return false;
    }
    
    return true;
}

void MCWidget::toolchanged(Tool p_new_tool)
{
    if (m_widget == nil)
        return;
    
    MCwidgeteventmanager -> event_toolchanged(this, p_new_tool);
}

void MCWidget::layerchanged()
{
    if (m_widget == nil)
        return;
    
    MCwidgeteventmanager -> event_layerchanged(this);
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
    
	if ((t_stat = loadpropsets(p_stream, p_version)) != IO_NORMAL)
		return t_stat;
    
    return t_stat;
}

IO_stat MCWidget::save(IO_handle p_stream, uint4 p_part, bool p_force_ext)
{
    // Make the widget generate a rep.
    MCAutoValueRef t_rep;
    if (m_widget != nil)
        MCWidgetOnSave(m_widget, &t_rep);
    
    // If the rep is nil, then an error must have been thrown, so we still
    // save, but without any state for this widget.
    if (*t_rep == nil)
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
    if ((t_stat = IO_write_valueref_new(*t_rep, p_stream)) != IO_NORMAL)
        return t_stat;
    
    if ((t_stat = savepropsets(p_stream)) != IO_NORMAL)
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
    if (MCerrorptr == NULL)
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

////////////////////////////////////////////////////////////////////////////////
