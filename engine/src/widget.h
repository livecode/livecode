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

#ifndef __MC_WIDGET__
#define __MC_WIDGET__

#include "control.h"
#include "native-layer.h"

#include "libscript/script.h"
#include "module-engine.h"

////////////////////////////////////////////////////////////////////////////////

typedef MCValueRef MCWidgetRef;

bool MCWidgetCreateRoot(MCWidget *host, MCNameRef kind, MCWidgetRef& r_widget);
bool MCWidgetCreateChild(MCNameRef kind, MCWidgetRef& r_widget);

MCNameRef MCWidgetGetKind(MCWidgetRef widget);

bool MCWidgetIsRoot(MCWidgetRef widget);
MCWidget *MCWidgetGetHost(MCWidgetRef widget);
MCWidgetRef MCWidgetGetOwner(MCWidgetRef widget);

MCGRectangle MCWidgetGetFrame(MCWidgetRef widget);
bool MCWidgetGetDisabled(MCWidgetRef widget);
bool MCWidgetCopyFont(MCWidgetRef widget, MCFontRef& r_font);

bool MCWidgetHasProperty(MCWidgetRef widget, MCNameRef property);
bool MCWidgetHasHandler(MCWidgetRef widget, MCNameRef handler);

bool MCWidgetQueryProperty(MCWidgetRef widget, MCNameRef property, MCTypeInfoRef& r_get_type, MCTypeInfoRef& r_set_type);

bool MCWidgetSetProperty(MCWidgetRef widget, MCNameRef property, MCValueRef value);
bool MCWidgetGetProperty(MCWidgetRef widget, MCNameRef property, MCValueRef& r_value);

bool MCWidgetOnLoad(MCWidgetRef widget, MCValueRef rep);
bool MCWidgetOnSave(MCWidgetRef widget, MCValueRef& r_rep);
bool MCWidgetOnOpen(MCWidgetRef widget);
bool MCWidgetOnClose(MCWidgetRef widget);
bool MCWidgetOnTimer(MCWidgetRef widget);
bool MCWidgetOnPaint(MCWidgetRef widget, MCGContextRef gcontext);
bool MCWidgetOnHitTest(MCWidgetRef widget, MCGPoint location, MCWidgetRef& r_target);

bool MCWidgetOnMouseEnter(MCWidgetRef widget, bool& r_bubble);
bool MCWidgetOnMouseLeave(MCWidgetRef widget, bool& r_bubble);
bool MCWidgetOnMouseMove(MCWidgetRef widget, bool& r_bubble);
bool MCWidgetOnMouseDown(MCWidgetRef widget, bool& r_bubble);
bool MCWidgetOnMouseUp(MCWidgetRef widget, bool& r_bubble);
bool MCWidgetOnMouseCancel(MCWidgetRef widget, bool& r_bubble);
bool MCWidgetOnClick(MCWidgetRef widget, bool& r_bubble);

bool MCWidgetOnMouseScroll(MCWidgetRef widget, real32_t delta_x, real32_t delta_y, bool& r_bubble);

bool MCWidgetOnGeometryChanged(MCWidgetRef widget);
bool MCWidgetOnParentPropertyChanged(MCWidgetRef widget);
bool MCWidgetOnToolChanged(MCWidgetRef widget, Tool p_tool);

bool MCWidgetCopyAnnotation(MCWidgetRef widget, MCNameRef annotation, MCValueRef& r_value);
bool MCWidgetSetAnnotation(MCWidgetRef widget, MCNameRef annotation, MCValueRef value);

bool MCWidgetPost(MCWidgetRef widget, MCNameRef event, MCProperListRef args);

void MCWidgetRedrawAll(MCWidgetRef widget);
void MCWidgetScheduleTimerIn(MCWidgetRef widget, double timeout);
void MCWidgetCancelTimer(MCWidgetRef widget);

void MCWidgetCopyChildren(MCWidgetRef widget, MCProperListRef& r_children);
void MCWidgetPlaceWidget(MCWidgetRef widget, MCWidgetRef child, MCWidgetRef relative_to, bool put_below);
void MCWidgetUnplaceWidget(MCWidgetRef widget,  MCWidgetRef child);

MCGPoint MCWidgetMapPointToGlobal(MCWidgetRef widget, MCGPoint point);
MCGPoint MCWidgetMapPointFromGlobal(MCWidgetRef widget, MCGPoint point);

MCGRectangle MCWidgetMapRectToGlobal(MCWidgetRef widget, MCGRectangle point);
MCGRectangle MCWidgetMapRectFromGlobal(MCWidgetRef widget, MCGRectangle point);

////

bool MCChildWidgetSetFrame(MCWidgetRef widget, MCGRectangle frame);
bool MCChildWidgetSetDisabled(MCWidgetRef widget, bool disabled);

////////////////////////////////////////////////////////////////////////////////

class MCWidget: public MCControl
{
public:
	MCWidget(void);
	MCWidget(const MCWidget& p_other);
	virtual ~MCWidget(void);

	virtual Chunk_term gettype(void) const;
	virtual const char *gettypestring(void);

	virtual const MCObjectPropertyTable *getpropertytable(void) const;
    
	virtual bool visit_self(MCObjectVisitor *p_visitor);
	
	virtual void open(void);
	virtual void close(void);

	virtual void kfocus(void);
	virtual void kunfocus(void);
	virtual Boolean kdown(MCStringRef p_key_string, KeySym p_key);
	virtual Boolean kup(MCStringRef p_key_string, KeySym p_key);

	virtual Boolean mdown(uint2 p_which);
	virtual Boolean mup(uint2 p_which, bool p_release);
	virtual Boolean mfocus(int2 p_x, int2 p_y);
	virtual void munfocus(void);

    virtual void mdrag(void);
    
	virtual Boolean doubledown(uint2 p_which);
	virtual Boolean doubleup(uint2 p_which);
	
    virtual MCObject* hittest(int32_t x, int32_t y);
    
	virtual void timer(MCNameRef p_message, MCParameter *p_parameters);

	virtual void setrect(const MCRectangle& p_rectangle);
	virtual void recompute(void);

	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *, MCObject *pass_from);

	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	virtual IO_stat load(IO_handle stream, uint32_t p_version);

	virtual MCControl *clone(Boolean p_attach, Object_pos p_position, bool invisible);

	virtual void draw(MCDC *p_dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite);
	virtual Boolean maskrect(const MCRectangle& p_rect);
	
    virtual bool getprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue& r_value);
	virtual bool setprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue p_value);
    virtual bool getcustomprop(MCExecContext& ctxt, MCNameRef set_name, MCNameRef prop_name, MCExecValue& r_value);
	virtual bool setcustomprop(MCExecContext& ctxt, MCNameRef set_name, MCNameRef prop_name, MCExecValue p_value);
    
    virtual void toolchanged(Tool p_new_tool);
    
    virtual void layerchanged();
    
    virtual void SetDisabled(MCExecContext& ctxt, uint32_t part, bool flag);
    
    void GetKind(MCExecContext& ctxt, MCNameRef& r_kind);
    
    // Bind a widget to a kind and rep.
    void bind(MCNameRef p_kind, MCValueRef p_rep);
    
    MCWidgetRef getwidget(void) const;
    
    void CatchError(MCExecContext& ctxt);
    void SendError(void);
    
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    
private:
    // The kind of the widget.
    MCNameRef m_kind;
    
    // The rep of the widget - this is non-nil if the widget kind is unresolved
    // after loading.
    MCValueRef m_rep;
    
    // The LCB Widget object.
    MCWidgetRef m_widget;
};

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT MCTypeInfoRef kMCWidgetNoCurrentWidgetErrorTypeInfo;
extern "C" MC_DLLEXPORT MCTypeInfoRef kMCWidgetSizeFormatErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

#endif
