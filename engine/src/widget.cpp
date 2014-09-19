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

MCWidget::MCWidget(void)
{
	m_mouse_over = false;
	m_mouse_x = INT32_MIN;
	m_mouse_y = INT32_MAX;
	m_button_state = 0;
	m_modifier_state = 0;
}

MCWidget::MCWidget(const MCWidget& p_other)
	: MCControl(p_other)
{
	m_mouse_over = false;
	m_mouse_x = INT32_MIN;
	m_mouse_y = INT32_MAX;
	m_button_state = 0;
	m_modifier_state = 0;
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
    OnOpen();
}

void MCWidget::close(void)
{
    OnClose();
	MCControl::close();
}

void MCWidget::kfocus(void)
{
	MCControl::kfocus();
	if (getstate(CS_KFOCUSED))
		OnFocus();
}

void MCWidget::kunfocus(void)
{
	if (getstate(CS_KFOCUSED))
		OnUnfocus();
	MCControl::kunfocus();
}

Boolean MCWidget::kdown(MCStringRef p_key_string, KeySym p_key)
{
	uint32_t t_modifiers;
	t_modifiers = 0;
	/*if ((MCmodifierstate & MS_CONTROL) != 0)
		t_modifiers |= kMCWidgetModifierKeyControl;
	if ((MCmodifierstate & MS_MOD1) != 0)
		t_modifiers |= kMCWidgetModifierKeyOption;
	if ((MCmodifierstate & MS_SHIFT) != 0)
		t_modifiers |= kMCWidgetModifierKeyShift;*/

	if (OnKeyPress(p_key_string, t_modifiers))
		return True;

	return MCObject::kdown(p_key_string, p_key);
}

Boolean MCWidget::kup(MCStringRef p_key_string, KeySym p_key)
{
	return False;
}

Boolean MCWidget::mdown(uint2 p_which)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mdown(p_which);
	
	if ((m_button_state & (1 << p_which)) != 0)
		return True;

	switch(getstack() -> gettool(this))
	{
	case T_BROWSE:
		setstate(True, CS_MFOCUSED);
		m_button_state |= 1 << p_which;
		OnMouseDown(p_which);
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
		m_button_state &= ~(1 << p_which);
		if (m_button_state == 0)
			setstate(False, CS_MFOCUSED);
		if (!p_release)
			OnMouseUp(p_which);
		else
			OnMouseRelease(p_which);
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
	
	if (m_button_state == 0 && !maskrect(MCU_make_rect(p_x, p_y, 1, 1)))
		return False;
	
	// Update the mouse loc.
	mx = p_x;
	my = p_y;
	
	// Get control local coords
	int32_t t_mouse_x, t_mouse_y;
	t_mouse_x = p_x - getrect() . x;
	t_mouse_y = p_y - getrect() . y;
	
	// Check to see if pos has changed
	bool t_pos_changed;
	t_pos_changed = false;
	if (t_mouse_x != m_mouse_x || t_mouse_y != m_mouse_y)
	{
		m_mouse_x = t_mouse_x;
		m_mouse_y = t_mouse_y;
		t_pos_changed = true;
	}
		
	// If we weren't previously under the mouse, we are now.
	if (!m_mouse_over)
	{
		m_mouse_over = true;
		OnMouseEnter();
	}
	
	// Dispatch a position update if needed.
	if (t_pos_changed)
		OnMouseMove(t_mouse_x, t_mouse_y);
	
	return True;
}

void MCWidget::munfocus(void)
{
	if (getstack() -> gettool(this) != T_BROWSE ||
		(!m_mouse_over && m_button_state == 0))
	{
		MCControl::munfocus();
		return;
	}
	
	if (m_button_state != 0)
	{
		for(int32_t i = 0; i < 3; i++)
			if ((m_button_state & (1 << i)) != 0)
			{
				m_button_state &= ~(1 << i);
				OnMouseRelease(i);
			}
	}
	
	m_mouse_over = false;
	OnMouseLeave();
}

Boolean MCWidget::doubledown(uint2 p_which)
{
	return False;
}

Boolean MCWidget::doubleup(uint2 p_which)
{
	return False;
}

void MCWidget::timer(MCNameRef p_message, MCParameter *p_parameters)
{
}

void MCWidget::setrect(const MCRectangle& p_rectangle)
{
	MCRectangle t_old_rect;
	t_old_rect = rect;
	
	rect = p_rectangle;
	
	OnReshape(t_old_rect);
}

void MCWidget::recompute(void)
{
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
	OnPaint();
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

void MCWidget::OnOpen(void)
{
    CallEvent("open", nil);
}

void MCWidget::OnClose(void)
{
    CallEvent("close", nil);
}

void MCWidget::OnReshape(const MCRectangle& p_old_rect)
{
	MCParameter t_param;
	t_param . setrect_argument(p_old_rect);
	
	CallEvent("reshape", &t_param);
}

void MCWidget::OnFocus(void)
{
}

void MCWidget::OnUnfocus(void)
{
}

void MCWidget::OnMouseEnter(void)
{
	CallEvent("mouseEnter", nil);
}

void MCWidget::OnMouseMove(int32_t x, int32_t y)
{
	MCParameter t_param_1, t_param_2;
	t_param_1 . setn_argument(x);
	t_param_2 . setn_argument(y);
	t_param_1 . setnext(&t_param_2);
	CallEvent("mouseMove", &t_param_1);
}

void MCWidget::OnMouseLeave(void)
{
	CallEvent("mouseLeave", nil);
}

void MCWidget::OnMouseDown(uint32_t p_button)
{
	MCParameter t_param;
	t_param . setn_argument(p_button);
	CallEvent("mouseDown", &t_param);
}

void MCWidget::OnMouseUp(uint32_t p_button)
{
	MCParameter t_param;
	t_param . setn_argument(p_button);
	CallEvent("mouseUp", &t_param);
}

void MCWidget::OnMouseRelease(uint32_t p_button)
{
	MCParameter t_param;
	t_param . setn_argument(p_button);
	CallEvent("mouseRelease", &t_param);
}

bool MCWidget::OnKeyPress(MCStringRef key, uint32_t modifiers)
{
	return false;
}

bool MCWidget::OnHitTest(const MCRectangle& region)
{
	return false;
}

void MCWidget::OnPaint(void)
{
	CallEvent("paint", nil);
}

////////////////////////////////////////////////////////////////////////////////

bool MCWidget::CallEvent(const char *p_name, MCParameter *p_parameters)
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
