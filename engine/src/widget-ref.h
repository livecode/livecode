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

#ifndef __MC_WIDGET_IMP__
#define __MC_WIDGET_IMP__

////////////////////////////////////////////////////////////////////////////////

class MCWidgetBase
{
public:
    MCWidgetBase(void);
    virtual ~MCWidgetBase(void);
    
    bool Create(MCNameRef kind);
    void Destroy(void);
    
    MCWidgetRef AsWidget(void);
    
    MCNameRef GetKind(void) const;
    
    //// NORMAL METHODS
    
    bool HasProperty(MCNameRef property);
    bool HasHandler(MCNameRef handler);
    
    bool QueryProperty(MCNameRef property, MCTypeInfoRef& r_get_type, MCTypeInfoRef& r_set_type);
    bool QueryHandler(MCNameRef handler, MCTypeInfoRef& r_signature);
    
    bool SetProperty(MCNameRef property, MCValueRef value);
    bool GetProperty(MCNameRef property, MCValueRef& r_value);

    bool HasPropertyOfChunk(MCNameRef p_property, MCNameRef p_chunk_name, bool p_is_getter);
    
    bool QueryPropertyOfChunk(MCNameRef p_property, MCNameRef p_chunk_name, bool p_is_getter, MCTypeInfoRef& r_type_info);
    bool SetPropertyOfChunk(MCNameRef property, MCNameRef p_chunk_name, MCProperListRef p_path, MCValueRef value);
    bool GetPropertyOfChunk(MCNameRef property, MCNameRef p_chunk_name, MCProperListRef p_path, MCValueRef& r_value);

    bool OnLoad(MCValueRef rep);
    bool OnSave(MCValueRef& r_rep);
    
    bool OnOpen(void);
    bool OnClose(void);
    
    bool OnAttach();
    bool OnDetach();
    
    bool OnTimer(void);
    
    bool OnPaint(MCGContextRef gcontext);
    bool OnHitTest(MCGPoint location, MCWidgetRef& r_target);
    
    bool OnMouseEnter(bool& r_bubble);
    bool OnMouseLeave(bool& r_bubble);
    bool OnMouseMove(bool& r_bubble);
    
    bool OnMouseDown(bool& r_bubble);
    bool OnMouseUp(bool& r_bubble);
    bool OnMouseCancel(bool& r_bubble);
    
    bool OnMouseScroll(coord_t delta_x, coord_t delta_y, bool& r_bubble);
    
    bool OnClick(bool& r_bubble);
    
    bool HandlesTouchEvents(void);
    bool OnTouchStart(bool& r_bubble);
    bool OnTouchMove(bool& r_bubble);
    bool OnTouchFinish(bool& r_bubble);
    bool OnTouchCancel(bool& r_bubble);
    
    bool OnGeometryChanged(void);
    bool OnLayerChanged();
    bool OnParentPropertyChanged(void);
    bool OnToolChanged(Tool tool);
    bool OnVisibilityChanged(bool p_visible);
    
    bool CopyAnnotation(MCNameRef annotation, MCValueRef& r_value);
    bool SetAnnotation(MCNameRef annotation, MCValueRef value);
    
    bool Post(MCNameRef event, MCProperListRef args);
    
    void ScheduleTimerIn(double timeout);
    void CancelTimer(void);
    void RedrawRect(MCGRectangle *area);
    void TriggerAll();
    
    bool CopyChildren(MCProperListRef& r_children);
    void PlaceWidget(MCWidgetRef child, MCWidgetRef relative_to, bool put_below);
    void UnplaceWidget(MCWidgetRef child);
    
    MCGPoint MapPointToGlobal(MCGPoint point);
    MCGPoint MapPointFromGlobal(MCGPoint point);
    
    MCGRectangle MapRectToGlobal(MCGRectangle rect);
    MCGRectangle MapRectFromGlobal(MCGRectangle rect);
    
    //// VIRTUAL METHODS
    
    // Returns true if the widget is a root widget.
    virtual bool IsRoot(void) const = 0;
    
    // Returns the widget's host MCControl.
    virtual MCWidget *GetHost(void) const = 0;
    
    // Returns the widget's owning widget (if any).
    virtual MCWidgetRef GetOwner(void) const = 0;
    
    // Returns the frame of the widget in the owner's coord system.
    virtual MCGRectangle GetFrame(void) const = 0;
    
    // Returns the enabled state of the widget.
    virtual bool GetDisabled(void) const = 0;
    
    // Copies the widget's font.
    virtual bool CopyFont(MCFontRef& r_font) = 0;
    
private:
    enum DispatchOrder
    {
        kDispatchOrderBeforeBottomUp,
        kDispatchOrderBottomUpAfter,
        kDispatchOrderBeforeTopDown,
        kDispatchOrderTopDownAfter,
        kDispatchOrderBottomUp,
        kDispatchOrderTopDown,
	};
	
	// Inner method for dispatching an event.
	bool DoDispatch(MCNameRef event, MCValueRef *x_args = nil, uindex_t arg_count = 0, MCValueRef *r_result = nil);
	
    // Dispatch an event to the widget allowing script access.
	// Any errors are sent to the host (if any).
    bool Dispatch(MCNameRef event, MCValueRef *x_args = nil, uindex_t arg_count = 0, MCValueRef *r_result = nil);
    
	// Dispatch an event to the widget but don't allow script access.
	// Any errors are sent to the host (if any).
    bool DispatchRestricted(MCNameRef event, MCValueRef *args = nil, uindex_t arg_count = 0, MCValueRef *r_result = nil);
    
    // Dispatch an event to the widget, don't allow script access and swallow
    // any errors.
    void DispatchRestrictedNoThrow(MCNameRef event, MCValueRef *args = nil, uindex_t arg_count = 0, MCValueRef *r_result = nil);
    
    // Disatpch to the widget and/or its children in the given order.
    bool DispatchRecursive(DispatchOrder order, MCNameRef event, MCValueRef *args = nil, uindex_t arg_count = 0, MCValueRef *r_result = nil);
    
    // Dispatch a potential bubbling event to the widget. Bubbling events can return
    // a boolean value to indicate whether the event should be passed up the owner
    // chain.
    bool DispatchBubbly(MCNameRef event, MCValueRef *args, uindex_t arg_count, bool& r_bubble);
    
    // The instance of this widget.
    MCScriptInstanceRef m_instance;
    
    // The children of this widget (a mutable list - or nil if no children).
    MCProperListRef m_children;
    
    // The annotations of this widget (a mutable array - or nil if none).
    MCArrayRef m_annotations;
    
    // If true, then the widget has an active timer that should be cancelled.
    bool m_has_timer : 1;
    
    // If true, then the widget has deferred a timer until browse mode is entered.
    bool m_timer_deferred : 1;
};

class MCWidgetRoot: public MCWidgetBase
{
public:
    MCWidgetRoot(MCWidget *p_host);
    virtual ~MCWidgetRoot(void);
    
    virtual bool IsRoot(void) const;
    virtual MCWidget *GetHost(void) const;
    virtual MCWidgetRef GetOwner(void) const;
    virtual MCGRectangle GetFrame(void) const;
    virtual bool GetDisabled(void) const;
    virtual bool CopyFont(MCFontRef& r_font);
    
private:
    MCWidgetHandle m_host;
};

class MCWidgetChild: public MCWidgetBase
{
public:
    MCWidgetChild(void);
    virtual ~MCWidgetChild(void);
    
    void SetOwner(MCWidgetRef owner);
    bool SetFrame(MCGRectangle frame);
    bool SetDisabled(bool disabled);
    
    virtual bool IsRoot(void) const;
    virtual MCWidget *GetHost(void) const;
    virtual MCWidgetRef GetOwner(void) const;
    virtual MCGRectangle GetFrame(void) const;
    virtual bool GetDisabled(void) const;
    virtual bool CopyFont(MCFontRef& r_font);
    
private:
    MCGRectangle m_frame;
    MCWidgetRef m_owner;
    bool m_disabled : 1;
};

MCWidgetBase *MCWidgetAsBase(MCWidgetRef widget);
MCWidgetRoot *MCWidgetAsRoot(MCWidgetRef widget);
MCWidgetChild *MCWidgetAsChild(MCWidgetRef widget);

void *MCWidgetEnter(MCScriptInstanceRef instance, void *host_ptr);
void MCWidgetLeave(MCScriptInstanceRef instance, void *host_ptr, void *cookie);

////////////////////////////////////////////////////////////////////////////////

extern MCWidgetRef MCcurrentwidget;

extern "C"
{
extern MC_DLLEXPORT MCTypeInfoRef kMCWidgetTypeInfo;
}

bool MCWidgetThrowNoCurrentWidgetError(void);
bool MCWidgetThrowNotSupportedInChildWidgetError(void);
bool MCWidgetThrowNotAChildOfThisWidgetError(void);
bool MCWidgetEnsureCurrentWidget(void);

#endif
