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

#ifndef __MC_WIDGET_IMP__
#define __MC_WIDGET_IMP__

class MCWidgetCommon
{
public:
    MCWidgetCommon(void);
    virtual ~MCWidgetCommon(void);
    
    ////
    
    bool HasProperty(MCNameRef p_name);
    
    bool HandlesEvent(MCNameRef p_name);
    
    //// Non-event interactions
    
    bool Create(MCNameRef kind);
    void Destroy(void);
    
    bool Load(MCValueRef rep);
    bool Save(MCValueRef& r_rep);
    
    bool Open(void);
    bool Close(void);
    
    bool Paint(MCGContextRef gcontext);
    
    bool ToolChanged(Tool p_tool);
    bool LayerChanged(void);
    
    //// Event interactions
    
    //// Universal syntax bindings
    bool GetProperty(MCNameRef p_prop_name, MCValueRef& r_value);
    bool SetProperty(MCNameRef p_prop_name, MCValueRef p_value);
    
    bool CopyChildren(MCProperListRef& r_children);
    void PlaceWidget(MCWidgetRef p_widget, MCWidgetRef p_other_widget, bool p_is_below);
    void UnplaceWidget(MCWidgetRef p_widget);
    
    //// Overridable syntax bindings
    
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
    
    //// Overridable utilities
    
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

extern MCWidgetRef MCcurrentwidget;
extern MCTypeInfoRef kMCWidgetTypeInfo;

bool MCWidgetThrowNoCurrentWidgetError(void);
bool MCWidgetThrowNotSupportedInChildWidgetError(void);
bool MCWidgetEnsureCurrentWidget(void);

#endif
