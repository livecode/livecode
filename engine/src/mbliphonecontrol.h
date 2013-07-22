/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#ifndef __MC_MOBILE_IPHONE_CONTROL__
#define __MC_MOBILE_IPHONE_CONTROL__

#include "mblcontrol.h"

class MCiOSControl : public MCNativeControl
{
public:
    MCiOSControl(void);
    
    // overridden methods
    virtual bool Create(void);
    virtual void Delete(void);
#ifdef LEGACY_EXEC
    virtual Exec_stat Set(MCNativeControlProperty p_property, MCExecPoint &ep);
    virtual Exec_stat Get(MCNativeControlProperty p_property, MCExecPoint &ep);
#endif
    virtual Exec_stat Do(MCNativeControlAction p_action, MCParameter *_parameters);
    
    virtual void Set(MCExecContext& ctxt, MCNativeControlProperty p_property);
    virtual void Get(MCExecContext& ctxt, MCNativeControlProperty p_property);
    
    virtual void SetRect(MCExecContext& ctxt, MCRectangle p_rect);
    virtual void SetVisible(MCExecContext& ctxt, bool p_visible);
    virtual void SetOpaque(MCExecContext& ctxt, bool p_opaque);
    virtual void SetAlpha(MCExecContext& ctxt, uinteger_t p_alpha);
    virtual void SetBackgroundColor(MCExecContext& ctxt, const MCNativeControlColor& p_color);
    
    virtual void GetRect(MCExecContext& ctxt, MCRectangle& r_rect);
    virtual void GetVisible(MCExecContext& ctxt, bool& p_visible);
    virtual void GetOpaque(MCExecContext& ctxt, bool& p_opaque);
    virtual void GetAlpha(MCExecContext& ctxt, uinteger_t& p_alpha);
    virtual void GetBackgroundColor(MCExecContext& ctxt, MCNativeControlColor& p_color);
    
	// Get the native view of the instance.
	UIView *GetView(void);
	
	// Various helper functions
	static Exec_stat ParseColor(MCExecPoint& ep, UIColor*& r_color);
	static Exec_stat FormatColor(MCExecPoint& ep, UIColor *color);
    
    static bool ParseColor(const MCNativeControlColor& p_color, UIColor*& r_color);
	static bool FormatColor(const UIColor* p_color, MCNativeControlColor& r_color);
	static bool ParseString(MCExecPoint& ep, NSString*& r_string);
	static bool FormatString(MCExecPoint& ep, NSString *string);
	static bool ParseUnicodeString(MCExecPoint& ep, NSString*& r_string);
	static bool FormatUnicodeString(MCExecPoint& ep, NSString *string);
    
    static bool ParseRange(MCExecPoint &ep, NSRange &r_range);
    static bool FormatRange(MCExecPoint &ep, NSRange r_range);
	
protected:
	// Called by the base-class when it needs the view created
	virtual UIView *CreateView(void) = 0;
	// Called by the base-class when it needs the view destroyed
	virtual void DeleteView(UIView *view) = 0;

private:
	// The instance's view
	UIView *m_view;
};

float MCIPhoneGetNativeControlScale(void);
bool MCScrollViewGetContentOffset(UIScrollView *p_view, int32_t &r_x, int32_t &r_y);
#endif
