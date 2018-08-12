/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#ifndef __MC_MOBILE_ANDROID_CONTROL__
#define __MC_MOBILE_ANDROID_CONTROL__

#include "mblcontrol.h"

class MCAndroidControl : public MCNativeControl
{
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;

public:
    MCAndroidControl(void);
    
    // overridden methods
    virtual bool Create(void);
    virtual void Delete(void);
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
    void SetRect(MCExecContext& ctxt, MCRectangle p_rect);
    void SetVisible(MCExecContext& ctxt, bool p_visible);
    void SetAlpha(MCExecContext& ctxt, uinteger_t p_alpha);
    void SetBackgroundColor(MCExecContext& ctxt, const MCNativeControlColor& p_color);
    
    void GetRect(MCExecContext& ctxt, MCRectangle& r_rect);
    void GetVisible(MCExecContext& ctxt, bool& r_visible);
    void GetAlpha(MCExecContext& ctxt, uinteger_t& r_alpha);
    void GetBackgroundColor(MCExecContext& ctxt, MCNativeControlColor& r_color);
    
    // standard event handling methods
    void PostNotifyEvent(MCNameRef p_message);
    virtual void HandleNotifyEvent(MCNameRef p_message);
    
	// Get the native view of the instance.
	jobject GetView(void);
	
    // Various helper functions
    static bool FindByView(jobject p_view, MCAndroidControl *&r_control);
    static bool GetViewRect(jobject p_view, int16_t &r_left, int16_t &r_top, int16_t &r_right, int16_t &r_bottom);
    static bool GetViewBackgroundColor(jobject p_view, uint16_t &r_red, uint16_t &r_green, uint16_t &r_blue, uint16_t &r_alpha);
    
protected:
	// Called by the base-class when it needs the view created
	virtual jobject CreateView(void) = 0;
	// Called by the base-class when it needs the view destroyed
	virtual void DeleteView(jobject view) = 0;
    
private:
	// The instance's view
	jobject m_view;
};

#endif //__MC_MOBILE_ANDROID_CONTROL__
