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
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;
    
public:
    MCiOSControl(void);
    
    // overridden methods
    virtual bool Create(void);
    virtual void Delete(void);
#ifdef LEGACY_EXEC
    virtual Exec_stat Set(MCNativeControlProperty p_property, MCExecPoint &ep);
    virtual Exec_stat Get(MCNativeControlProperty p_property, MCExecPoint &ep);
    virtual Exec_stat Do(MCNativeControlAction p_action, MCParameter *_parameters);
#endif
    
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
    void SetRect(MCExecContext& ctxt, MCRectangle p_rect);
    void SetVisible(MCExecContext& ctxt, bool p_visible);
    void SetOpaque(MCExecContext& ctxt, bool p_opaque);
    void SetAlpha(MCExecContext& ctxt, uinteger_t p_alpha);
    void SetBackgroundColor(MCExecContext& ctxt, const MCNativeControlColor& p_color);
    // SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]] Added functions for the ignoring voice over sensibility
    void SetIgnoreVoiceOverSensitivity(MCExecContext& ctxt, bool p_ignore_vos);
    
    void GetRect(MCExecContext& ctxt, MCRectangle& r_rect);
    void GetVisible(MCExecContext& ctxt, bool& p_visible);
    void GetOpaque(MCExecContext& ctxt, bool& p_opaque);
    void GetAlpha(MCExecContext& ctxt, uinteger_t& p_alpha);
    void GetBackgroundColor(MCExecContext& ctxt, MCNativeControlColor& p_color);
    // SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]] Added functions for the ignoring voice over sensibility
    void GetIgnoreVoiceOverSensitivity(MCExecContext& ctxt, bool& r_ignore_vos);
    
	// Get the native view of the instance.
	UIView *GetView(void);

	
	// Various helper functions
#ifdef LEGACY_EXEC
	static Exec_stat ParseColor(MCExecPoint& ep, UIColor*& r_color);
	static Exec_stat FormatColor(MCExecPoint& ep, UIColor *color);
#endif
    
    static bool ParseColor(const MCNativeControlColor& p_color, UIColor*& r_color);
	static bool FormatColor(const UIColor* p_color, MCNativeControlColor& r_color);
#ifdef LEGACY_EXEC
	static bool ParseString(MCExecPoint& ep, NSString*& r_string);
	static bool FormatString(MCExecPoint& ep, NSString *string);
	static bool ParseUnicodeString(MCExecPoint& ep, NSString*& r_string);
	static bool FormatUnicodeString(MCExecPoint& ep, NSString *string);
    static bool ParseRange(MCExecPoint &ep, NSRange &r_range);
    static bool FormatRange(MCExecPoint &ep, NSRange r_range);
#endif

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
