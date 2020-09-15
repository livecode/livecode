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
		MCValueRelease(m_result);
		m_result = nil;
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
		
		uint32_t t_width = 0;
		uint32_t t_height = 0;
		getwidgetgeometry(t_width, t_height);
		
		if (MCErrorIsPending())
			return false;
		
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
		
		m_widget = new (nothrow) MCWidget();
		if (m_widget == nil)
			return MCErrorThrowOutOfMemory();
		
        m_widget->setparent(this);
		m_widget->bind(p_kind, nil);
		m_widget->attach(OP_NONE, false);
        
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
		
		if (MCValueIsEmpty(*t_value))
			return false;
		
		if (!WidgetGeometryFromLCBList(*t_value, r_width, r_height))
			return MCErrorCreateAndThrow(kMCWidgetSizeFormatErrorTypeInfo, nil);
		
		return true;
	}
	
	void getwidgetgeometry(uint32_t &r_width, uint32_t &r_height)
	{
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

extern "C" MC_DLLEXPORT_DEF MCStringRef MCWidgetExecPopupMenuAtLocation(MCStringRef p_menu, MCCanvasPointRef p_at)
{
    if (!MCWidgetEnsureCurrentWidget())
        return nil;
	
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
		
		t_at = MCGPointToMCPoint(MCWidgetMapPointToGlobal(MCcurrentwidget, t_point));
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
    MCerrorlock++;
    if (t_button->del(false))
        t_button->scheduledelete();
    MCerrorlock--;
    
	MCAutoStringRef t_string;
	
	if (t_handler.GetPick() != nil)
		ctxt.ConvertToString(t_handler.GetPick(), &t_string);
	
	return t_string.Take();
}

////////////////////////////////////////////////////////////////////////////////

static MCWidgetPopup *s_widget_popup = nil;

bool MCWidgetPopupAtLocationWithProperties(MCNameRef p_kind, const MCPoint &p_at, MCArrayRef p_properties, MCValueRef &r_result)
{
	MCWidgetPopup *t_popup;
	t_popup = nil;
	
	t_popup = new (nothrow) MCWidgetPopup();
	if (t_popup == nil)
	{
		// TODO - throw memory error
		return false;
	}
	
	MCWidgetPopup *t_old_popup;
	t_old_popup = s_widget_popup;
	s_widget_popup = t_popup;

    t_popup -> setparent(MCdispatcher);
	MCdispatcher -> add_transient_stack(t_popup);
	
	if (!t_popup->openpopup(p_kind, p_at, p_properties))
	{
		t_popup->close();
		delete t_popup;
		s_widget_popup = t_old_popup;
		return false;
	}
	
	while (t_popup->getopened() && !MCquit)
	{
		MCU_resetprops(True);
		// MW-2011-09-08: [[ Redraw ]] Make sure we flush any updates.
		MCRedrawUpdateScreen();
		MCscreen->siguser();
		MCscreen->wait(REFRESH_INTERVAL, True, True);
	}
	
	MCValueRef t_result;
	t_result = MCValueRetain(t_popup->getpopupresult());
	
    MCerrorlock++;
    if (t_popup->del(false))
        t_popup->scheduledelete();
    MCerrorlock--;
    
	s_widget_popup = t_old_popup;
	
	r_result = t_result;
	
	return true;
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCWidgetExecPopupAtLocationWithProperties(MCStringRef p_kind, MCCanvasPointRef p_at, MCArrayRef p_properties)
{
    if (!MCWidgetEnsureCurrentWidget())
        return nil;
	
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(p_at, t_point);
	
    MCWidget *t_host;
    t_host = MCWidgetGetHost(MCcurrentwidget);
    
    if (!t_host->getstack()->getopened() || !t_host->getstack()->isvisible())
    {
        return nil;
    }
    
    MCPoint t_at;
	t_at = t_host->getstack()->stacktogloballoc(MCGPointToMCPoint(MCWidgetMapPointToGlobal(MCcurrentwidget, t_point)));
    
	MCNewAutoNameRef t_kind;
	/* UNCHECKED */ MCNameCreate(p_kind, &t_kind);
	
	MCValueRef t_result;
	if (MCWidgetPopupAtLocationWithProperties(*t_kind, t_at, p_properties, t_result))
		return t_result;
	else
		return nil;
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCWidgetExecPopupAtLocation(MCStringRef p_kind, MCCanvasPointRef p_at)
{
	return MCWidgetExecPopupAtLocationWithProperties(p_kind, p_at, kMCEmptyArray);
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetEvalIsPopup(bool &r_popup)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
	
	r_popup = s_widget_popup != nil && MCWidgetGetHost(MCcurrentwidget) == s_widget_popup->getpopupwidget();
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetExecClosePopupWithResult(MCValueRef p_result)
{
    if (!MCWidgetEnsureCurrentWidget())
        return;
	
	bool t_is_popup = false;
	MCWidgetEvalIsPopup(t_is_popup);
	
	if (!t_is_popup)
	{
		// TODO - throw error
		return;
	}
	
	s_widget_popup->setpopupresult(p_result);
	s_widget_popup->close();
}

extern "C" MC_DLLEXPORT_DEF void MCWidgetExecClosePopup()
{
	MCWidgetExecClosePopupWithResult(kMCNull);
}

////////////////////////////////////////////////////////////////////////////////

bool com_livecode_widget_InitializePopups(void)
{
    return true;
}

void com_livecode_widget_FinalizePopups(void)
{
    
}

////////////////////////////////////////////////////////////////////////////////
