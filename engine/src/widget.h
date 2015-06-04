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

#include "script.h"
#include "module-engine.h"

////////////////////////////////////////////////////////////////////////////////

typedef MCValueRef MCWidgetRef;

class MCWidgetCommon
{
public:
    MCWidgetCommon(void);
    virtual ~MCWidgetCommon(void);
    
    ////
    
    bool HasProperty(MCNameRef p_name);
    
    bool HandlesEvent(MCNameRef p_name);
    
    ////
    
    bool Create(MCNameRef kind);
    void Destroy(void);
    
    bool Load(MCValueRef rep);
    bool Save(MCValueRef& r_rep);
    
    bool Open(void);
    bool Close(void);
    
    bool Paint(MCGContextRef gcontext);
    
    bool ToolChanged(Tool p_tool);
    bool LayerChanged(void);
    
    ////
    
    // Property access - these expect / return non-script types.
    bool GetProperty(MCNameRef p_prop_name, MCValueRef& r_value);
    bool SetProperty(MCNameRef p_prop_name, MCValueRef p_value);
    
    bool CopyChildren(MCProperListRef& r_children);
    void PlaceWidget(MCWidgetRef p_widget, MCWidgetRef p_other_widget, bool p_is_below);
    void UnplaceWidget(MCWidgetRef p_widget);
    
    ////
    
    virtual void RedrawAll(void) = 0;
    virtual void RedrawRect(const MCGRectangle& area) = 0;
    virtual MCGRectangle GetRectangle(void) = 0;
    virtual void SetRectangle(const MCGRectangle& rectangle) = 0;
    virtual bool GetDisabled(void) = 0;
    virtual void SetDisabled(bool disabled) = 0;
    virtual void CopyFont(MCFontRef& r_font) = 0;
    virtual void SetFont(MCFontRef font) = 0;
    virtual void ScheduleTimerIn(double after) = 0;
    virtual void CancelTimer(void) = 0;
    virtual void CopyScriptObject(MCScriptObjectRef& r_script_object) = 0;
    virtual MCValueRef Send(bool p_is_function, MCStringRef message, MCProperListRef arguments) = 0;
    virtual void Post(MCStringRef message, MCProperListRef arguments) = 0;
    
    virtual MCWidget *GetHost(void) = 0;
    virtual MCGPoint MapPointFromGlobal(MCGPoint point) = 0;
    virtual MCGPoint MapPointToGlobal(MCGPoint point) = 0;
    
    ////
    
private:
    bool OnCreate(void);
    bool OnDestroy(void);
    
    // The instance of this widget.
    MCScriptInstanceRef m_instance;
    
    // The children of this widget (a mutable list - or nil if no children).
    MCProperListRef m_children;
};

class MCWidgetHost: public MCWidgetCommon
{
public:
    virtual void RedrawAll(void);
    virtual void RedrawRect(const MCGRectangle& area);
    virtual MCGRectangle GetRectangle(void);
    virtual void SetRectangle(const MCGRectangle& rectangle);
    virtual bool GetDisabled(void);
    virtual void SetDisabled(bool disabled);
    virtual void CopyFont(MCFontRef& r_font);
    virtual void SetFont(MCFontRef font);
    virtual void ScheduleTimerIn(double after);
    virtual void CancelTimer(void);
    virtual void CopyScriptObject(MCScriptObjectRef& r_script_object);
    virtual MCValueRef Send(bool p_is_function, MCStringRef message, MCProperListRef arguments);
    virtual void Post(MCStringRef message, MCProperListRef arguments);
    
    virtual MCWidget *GetHost(void);
    virtual MCGPoint MapPointFromGlobal(MCGPoint point);
    virtual MCGPoint MapPointToGlobal(MCGPoint point);
};

class MCWidgetChild: public MCWidgetCommon
{
public:
    virtual void RedrawAll(void);
    virtual void RedrawRect(const MCGRectangle& area);
    virtual MCGRectangle GetRectangle(void);
    virtual void SetRectangle(const MCGRectangle& rectangle);
    virtual bool GetDisabled(void);
    virtual void SetDisabled(bool disabled);
    virtual void CopyFont(MCFontRef& r_font);
    virtual void SetFont(MCFontRef font);
    virtual void ScheduleTimerIn(double after);
    virtual void CancelTimer(void);
    virtual void CopyScriptObject(MCScriptObjectRef& r_script_object);
    virtual MCValueRef Send(bool p_is_function, MCStringRef message, MCProperListRef arguments);
    virtual void Post(MCStringRef message, MCProperListRef arguments);
    
    virtual MCWidget *GetHost(void);
    virtual MCGPoint MapPointFromGlobal(MCGPoint point);
    virtual MCGPoint MapPointToGlobal(MCGPoint point);
};

bool MCWidgetHostCreate(MCWidgetRef& r_widget);
MCWidgetCommon *MCWidgetGetPtr(MCWidgetRef p_widget);

////////////////////////////////////////////////////////////////////////////////

#if 0
class MCCommonWidget
{
public:
    MCCommonWidget(void);
    ~MCCommonWidget(void);
    
    virtual MCCommonWidget *Retain(void) = 0;
    virtual void Release(void) = 0;
    
    virtual void RedrawAll(void) = 0;
    virtual void RedrawRect(const MCGRectangle& area) = 0;
    
    virtual MCGRectangle GetRectangle(void) = 0;
    virtual void SetRectangle(const MCGRectangle& rectangle) = 0;
    
    virtual bool GetDisabled(void) = 0;
    virtual void SetDisabled(bool disabled) = 0;
    
    virtual void CopyFont(MCFontRef& r_font) = 0;
    virtual void SetFont(MCFontRef font) = 0;
    
    virtual MCProperListRef GetChildren(void) = 0;
    
    virtual void ScheduleTimerIn(double after) = 0;
    virtual void CancelTimer(void) = 0;
    
    virtual void CopyScriptObject(MCScriptObjectRef& r_script_object) = 0;
    
    virtual MCValueRef Send(bool p_is_function, MCStringRef message, MCProperListRef arguments) = 0;
    virtual void Post(MCStringRef message, MCProperListRef arguments) = 0;
    
    ////
    
    virtual MCCommonWidget *GetParent(void) = 0;
    virtual MCWidget *GetHost(void) = 0;
    
    virtual MCGPoint MapPointFromGlobal(MCGPoint point) = 0;
    virtual MCGPoint MapPointToGlobal(MCGPoint point) = 0;
    
    virtual void Reparent(MCCommonWidget *parent) = 0;
    
    ////
    
    void PlaceWidget(MCWidgetRef p_widget, MCWidgetRef p_other_widget, bool p_is_below);
    void UnplaceWidget(MCWidgetRef p_widget);
    
    ////
    
    void OnCreate(void);
    void OnDestroy(void);
    
    void OnOpen(void);
    void OnClose(void);
    
    void OnPaint(MCGContextRef gcontext);
    
    void OnGeometryChanged(void);
    void OnParentPropChanged(void);
    
    ////
    
    bool CallHandler(MCScriptInstanceRef p_instance, MCNameRef p_name, MCValueRef* x_parameters, uindex_t p_param_count, MCValueRef* r_retval = nil);
    bool CallGetProp(MCScriptInstanceRef p_instance, MCExecContext& ctxt, MCNameRef p_property, MCNameRef p_key, MCValueRef& r_value);
    bool CallSetProp(MCScriptInstanceRef p_instance, MCExecContext& ctxt, MCNameRef p_property, MCNameRef p_key, MCValueRef p_value);
    
private:
    // The children of the widget
    MCProperListRef m_children;
};

class MCChildWidget: public MCCommonWidget
{
public:
    MCChildWidget(void);
    virtual ~MCChildWidget(void);
    
    virtual MCChildWidget *Retain(void);
    virtual void Release(void);
    
    virtual void RedrawAll(void);
    virtual void RedrawRect(const MCGRectangle& area);
    
    virtual MCGRectangle GetRectangle(void);
    virtual void SetRectangle(const MCGRectangle& rectangle);
    
    virtual bool GetDisabled(void);
    virtual void SetDisabled(bool disabled);
    
    virtual bool GetVisible(void);
    virtual void SetVisible(bool disabled);
    
    virtual void CopyFont(MCFontRef& r_font);
    virtual void SetFont(MCFontRef font);
    
    virtual MCProperListRef GetChildren(void);
    
    virtual void ScheduleTimerIn(double after);
    virtual void CancelTimer(void);
    
    virtual void CopyScriptObject(MCScriptObjectRef& r_script_object);
    
    virtual MCValueRef Send(bool p_is_function, MCStringRef message, MCProperListRef arguments);
    virtual void Post(MCStringRef message, MCProperListRef arguments);
    
    ////
    
    virtual MCCommonWidget *GetParent(void);
    virtual MCWidget *GetHost(void);
    
    virtual MCGPoint MapPointFromGlobal(MCGPoint point);
    virtual MCGPoint MapPointToGlobal(MCGPoint point);
    
    virtual void Reparent(MCCommonWidget *parent);
    
private:
    ////
    
    uindex_t m_references;
    MCNameRef m_kind;
    MCScriptInstanceRef m_instance;
    MCGRectangle m_rectangle;
    MCFontRef m_font;
    MCArrayRef m_annotations;
    MCCommonWidget *m_parent;
    bool m_disabled : 1;
    bool m_visible : 1;
};

class MCHostWidget: public MCCommonWidget
{
public:
    MCHostWidget(MCWidget *p_widget);
    
    virtual MCHostWidget *Retain(void);
    virtual void Release(void);
    
    virtual void RedrawAll(void);
    virtual void RedrawRect(const MCGRectangle& area);
    
    virtual MCGRectangle GetRectangle(void);
    virtual void SetRectangle(const MCGRectangle& rectangle);
    
    virtual bool GetDisabled(void);
    virtual void SetDisabled(bool disabled);
    
    virtual void CopyFont(MCFontRef& r_font);
    virtual void SetFont(MCFontRef font);
    
    virtual MCProperListRef GetChildren(void);
    virtual void SetChildren(MCProperListRef children);
    
    virtual void ScheduleTimerIn(double after);
    virtual void CancelTimer(void);
    
    virtual void CopyScriptObject(MCScriptObjectRef& r_script_object);
    
    virtual MCValueRef Send(bool p_is_function, MCStringRef message, MCProperListRef arguments);
    virtual void Post(MCStringRef message, MCProperListRef arguments);
    
    ////
    
    virtual MCCommonWidget *GetParent(void);
    virtual MCWidget *GetHost(void);
    
    virtual MCGPoint MapPointFromGlobal(MCGPoint point);
    virtual MCGPoint MapPointToGlobal(MCGPoint point);
    
    virtual void Reparent(MCCommonWidget *parent);
    
private:
    MCWidget *m_widget;
};
#endif

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
    
    // Returns true if the widget is in edit mode
    bool inEditMode();
    
    ////////// Functions used by the event manager for event processing
    bool handlesMouseDown() const;
    bool handlesMouseUp() const;
    bool handlesMouseCancel() const;
    bool handlesMouseScroll() const;
    bool handlesKeyPress() const;
    bool handlesActionKeyPress() const;
    bool handlesTouches() const;
    
    ////////// Functions used by the event manager for gesture processing
    bool wantsClicks() const;       // Does this widget want click events?
    bool wantsTouches() const;      // Does this widget want touch events?
    bool wantsDoubleClicks() const; // Does this widget want double-clicks?
    bool waitForDoubleClick() const;// Don't send click straight away
    bool isDragSource() const;      // Widget is source for drag-drop operations
    
    // Needed by the native layer code
    MCNativeLayer* getNativeLayer() const;
    
    // Bind a widget to a kind and rep.
    void bind(MCNameRef p_kind, MCValueRef p_rep);
    
protected:
    
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    
private:

    void CatchError(MCExecContext& ctxt);
    void SendError(void);
    
    //////////
    ////////// Widget messages
    //////////
    friend class MCWidgetEventManager;

    ////////// Lifecycle events
    
    void OnOpen();
    void OnClose();
    void OnAttach();
    void OnDetach();
    void OnPaint(MCGContextRef p_gcontext, const MCRectangle& p_dirty);
    void OnGeometryChanged(const MCRectangle& p_old_rect);
    void OnVisibilityChanged(bool p_visible);
    void OnHitTest(const MCRectangle& p_intersect, bool& r_inside);
    void OnBoundsTest(const MCRectangle& r_bounds_rect, bool& r_inside);
    void OnSave(MCValueRef& r_array);
    void OnLoad(MCValueRef array);
    void OnCreate();
    void OnDestroy();
    void OnParentPropChanged();
    void OnToolChanged(Tool p_new_tool);
    void OnLayerChanged();
    
    ////////// Simple timer events
    
    void OnTimer();
    
    ////////// Basic mouse events
    
    void OnMouseEnter();
    void OnMouseLeave();
    void OnMouseMove(coord_t p_x, coord_t p_y);
    void OnMouseCancel(uinteger_t p_button);
    void OnMouseDown(coord_t p_x, coord_t p_y, uinteger_t p_button);
    void OnMouseUp(coord_t p_x, coord_t p_y, uinteger_t p_button);
    void OnMouseScroll(coord_t p_delta_x, coord_t p_delta_y);
    
    ////////// Derived mouse events
    
    void OnMouseStillDown(uinteger_t p_button, real32_t p_duration);
    void OnMouseHover(coord_t p_x, coord_t p_y, real32_t p_duration);
    void OnMouseStillHover(coord_t p_x, coord_t p_y, real32_t p_duration);
    void OnMouseCancelHover(real32_t p_duration);
    
    ////////// Touch events
    
    void OnTouchStart(uinteger_t p_id, coord_t p_x, coord_t p_y, real32_t p_pressure, real32_t p_radius);
    void OnTouchMove(uinteger_t p_id, coord_t p_x, coord_t p_y, real32_t p_pressure, real32_t p_radius);
    void OnTouchEnter(uinteger_t p_id);
    void OnTouchLeave(uinteger_t p_id);
    void OnTouchFinish(uinteger_t p_id, coord_t p_x, coord_t p_y);
    void OnTouchCancel(uinteger_t p_id);
    
    ////////// Keyboard events
    
    void OnFocusEnter();
    void OnFocusLeave();
    void OnKeyPress(MCStringRef p_keytext);
    void OnModifiersChanged(uinteger_t p_modifier_mask);
    void OnActionKeyPress(MCStringRef p_keyname);
    
    ////////// Raw input events
    
    void OnRawKeyDown(uinteger_t p_keycode);
    void OnRawKeyUp(uinteger_t p_keycode);
    void OnRawMouseDown(uinteger_t p_button);
    void OnRawMouseUp(uinteger_t p_button);
    
    ////////// Drag-drop events
    
    // These take a drag context because touch controls can cause multiple drag
    // operations to be in effect simultaneously.
    //
    // Note: widget script should also have a way of triggering a drag and drop
    // operation itself, if desired; e.g. startDrag(in pTouchID as integer)
    void OnDragEnter(bool& r_accept);
    void OnDragLeave();
    void OnDragMove(coord_t p_x, coord_t p_y);
    void OnDragDrop();
    void OnDragStart(bool& r_accept);
    void OnDragFinish();
    
    ////////// Gestures
    
    // Mouse may have moved during/after the click so position is important
    void OnClick(coord_t p_x, coord_t p_y, uinteger_t p_button, uinteger_t p_count);
    void OnDoubleClick(coord_t p_x, coord_t p_y, uinteger_t p_button);

    ////////// IME
    
    // ????
    
    //////////
	//////////
    //////////
	
	bool CallHandler(MCNameRef p_name, MCValueRef* x_parameters, uindex_t p_param_count, MCValueRef* r_retval = NULL);
	bool CallGetProp(MCExecContext& ctxt, MCNameRef p_property_name, MCNameRef p_key, MCValueRef& r_value);
	bool CallSetProp(MCExecContext& ctxt, MCNameRef p_property_name, MCNameRef p_key, MCValueRef value);
    
    // The LCB Widget object.
    MCWidgetRef m_widget_imp;
    
    // The kind of the widget.
    MCNameRef m_kind;
    
    // The rep of the widget - this is non-nil if the widget kind is unresolved
    // after loading.
    MCValueRef m_rep;
    
    // The native layer(s) belonging to this widget
    MCNativeLayer* m_native_layer;
    
    // If this is true then the widget has scheduled a timer message, but it triggered
    // during edit mode.
    bool m_timer_deferred : 1;
    
    // Implemented by the platform-specific native layers: creates a new layer
    MCNativeLayer* createNativeLayer();
    
};

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT MCTypeInfoRef kMCWidgetNoCurrentWidgetErrorTypeInfo;
extern "C" MC_DLLEXPORT MCTypeInfoRef kMCWidgetSizeFormatErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

#endif
