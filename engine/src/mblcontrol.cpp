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

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"
#include "execpt.h"
#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"

#include "mblcontrol.h"

////////////////////////////////////////////////////////////////////////////////

static MCNativeControl *s_native_controls = nil;
static uint32_t s_last_native_control_id = 0;

static MCNativeControl *s_native_control_target = nil;

// MM-2012-02-22: Added module initialisation and clean up so static variables are managed correctly on Android
void MCNativeControlInitialize(void)
{
    s_native_controls = nil;
    s_last_native_control_id = 0;
    s_native_control_target = nil;
}

void MCNativeControlFinalize(void)
{
    MCNativeControl::Finalize();
    s_native_controls = nil;
    s_last_native_control_id = 0;
    s_native_control_target = nil;
}

////////////////////////////////////////////////////////////////////////////////


MCNativeControl::MCNativeControl(void)
{
	m_references = 1;
	m_id = ++s_last_native_control_id;
	m_name = nil;
	m_object = nil;
	m_next = nil;
    m_deleted = false;
}

MCNativeControl::~MCNativeControl(void)
{
	if (m_object != nil)
	{
		m_object -> Release();
		m_object = nil;
	}
	
	if (m_name != nil)
	{
		MCCStringFree(m_name);
		m_name = nil;
	}
    
	if (s_native_controls == this)
		s_native_controls = m_next;
	else
		for(MCNativeControl *t_previous = s_native_controls; t_previous != nil; t_previous = t_previous -> m_next)
			if (t_previous -> m_next == this)
			{
				t_previous -> m_next = m_next;
				break;
			}
}

void MCNativeControl::Retain(void)
{
	m_references += 1;
}

void MCNativeControl::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
    {
        // IM 2012-01-27
        // need to move Delete() call here as virtual functions cannot be called from the destructor
        // (nor should they be called from non-virtual functions called by the destructor)
        Delete();
		delete this;
    }
}

uint32_t MCNativeControl::GetId(void)
{
	return m_id;
}

const char *MCNativeControl::GetName(void)
{
	return m_name;
}

MCObject *MCNativeControl::GetOwner(void)
{
	return m_object != nil ? m_object -> Get() : nil;
}

void MCNativeControl::SetOwner(MCObject *p_owner)
{
	if (m_object != nil)
		m_object -> Release();
	m_object = p_owner -> gethandle();
}

bool MCNativeControl::SetName(const char *p_name)
{
	if (m_name != nil)
	{
		MCCStringFree(m_name);
		m_name = nil;
	}
	
	if (p_name != nil)
		return MCCStringClone(p_name, m_name);
	
	return true;
}

MCNativeControl *MCNativeControl::ChangeTarget(MCNativeControl *p_new_target)
{
	MCNativeControl *t_last_target;
	t_last_target = s_native_control_target;
	s_native_control_target = p_new_target;
	return t_last_target;
}

MCNativeControl *MCNativeControl::CurrentTarget(void)
{
	return s_native_control_target;
}

////////////////////////////////////////////////////////////////////////////////

static struct {const char *name; MCNativeControlType type;} s_native_control_types[] =
{
	{"browser", kMCNativeControlTypeBrowser},
	{"scroller", kMCNativeControlTypeScroller},
	{"player", kMCNativeControlTypePlayer},
	{"input", kMCNativeControlTypeInput},
	{"multiline", kMCNativeControlTypeMultiLineInput},
	{nil, kMCNativeControlTypeUnknown}
};

static struct {const char *name; MCNativeControlProperty property;} s_native_control_properties[] =
{
	{"id", kMCNativeControlPropertyId},
	{"name", kMCNativeControlPropertyName},
	
	{"rect", kMCNativeControlPropertyRectangle},
	{"rectangle", kMCNativeControlPropertyRectangle},
	{"visible", kMCNativeControlPropertyVisible},
	{"opaque", kMCNativeControlPropertyOpaque},
	{"alpha", kMCNativeControlPropertyAlpha},
	{"backgroundColor", kMCNativeControlPropertyBackgroundColor},
	
	{"dataDetectorTypes", kMCNativeControlPropertyDataDetectorTypes},
	
	{"url", kMCNativeControlPropertyUrl},
	{"canadvance", kMCNativeControlPropertyCanAdvance},
	{"canretreat", kMCNativeControlPropertyCanRetreat},
	{"autofit", kMCNativeControlPropertyAutoFit},
	{"delayrequests", kMCNativeControlPropertyDelayRequests},
	{"allowsInlineMediaPlayback", kMCNativeControlPropertyAllowsInlineMediaPlayback},
	{"mediaPlaybackRequiresUserAction", kMCNativeControlPropertyMediaPlaybackRequiresUserAction},
	
	{"contentrect", kMCNativeControlPropertyContentRectangle},
	{"contentrectangle", kMCNativeControlPropertyContentRectangle},
	{"canbounce", kMCNativeControlPropertyCanBounce},
	{"vscroll", kMCNativeControlPropertyVScroll},
	{"hscroll", kMCNativeControlPropertyHScroll},
	{"canscrolltotop", kMCNativeControlPropertyCanScrollToTop},
	{"cancanceltouches", kMCNativeControlPropertyCanCancelTouches},
	{"delaytouches", kMCNativeControlPropertyDelayTouches},
	{"decelerationrate", kMCNativeControlPropertyDecelerationRate},
	{"indicatorstyle", kMCNativeControlPropertyIndicatorStyle},
	{"indicatorinsets", kMCNativeControlPropertyIndicatorInsets},
	{"pagingenabled", kMCNativeControlPropertyPagingEnabled},
	{"scrollingenabled", kMCNativeControlPropertyScrollingEnabled},
	{"hIndicator", kMCNativeControlPropertyShowHorizontalIndicator},
	{"vIndicator", kMCNativeControlPropertyShowVerticalIndicator},
	{"lockdirection", kMCNativeControlPropertyLockDirection},
	{"tracking", kMCNativeControlPropertyTracking},
	{"dragging", kMCNativeControlPropertyDragging},
	{"decelerating", kMCNativeControlPropertyDecelerating},
	
	{"filename", kMCNativeControlPropertyContent},
	{"naturalsize", kMCNativeControlPropertyNaturalSize},
	{"fullscreen", kMCNativeControlPropertyFullscreen},
	{"preserveaspect", kMCNativeControlPropertyPreserveAspect},
	{"showcontroller", kMCNativeControlPropertyShowController},
	{"duration", kMCNativeControlPropertyDuration},
	{"playableduration", kMCNativeControlPropertyPlayableDuration},
	{"starttime", kMCNativeControlPropertyStartTime},
	{"endtime", kMCNativeControlPropertyEndTime},
	{"currenttime", kMCNativeControlPropertyCurrentTime},
	{"autoplay", kMCNativeControlPropertyShouldAutoplay},
	{"looping", kMCNativeControlPropertyLooping},
	
	{"playbackstate", kMCNativeControlPropertyPlaybackState},
    
    // MM-2013-02-21: [[ Bug 10632 ]] Added playRate property for native player.
    {"playrate", kMCNativeControlPropertyPlayRate},
    
	{"loadstate", kMCNativeControlPropertyLoadState},
	{"useapplicationaudiosession", kMCNativeControlPropertyUseApplicationAudioSession},
	{"allowsairplay", kMCNativeControlPropertyAllowsAirPlay},
	
	{"enabled", kMCNativeControlPropertyEnabled},
	
	{"text", kMCNativeControlPropertyText},
	{"unicodetext", kMCNativeControlPropertyUnicodeText},
	{"textcolor", kMCNativeControlPropertyTextColor},
	{"textalign", kMCNativeControlPropertyTextAlign},
	{"fontname", kMCNativeControlPropertyFontName},
	{"fontsize", kMCNativeControlPropertyFontSize},
	{"editing", kMCNativeControlPropertyEditing},
    
	{"minimumfontsize", kMCNativeControlPropertyMinimumFontSize},
	{"autoclear", kMCNativeControlPropertyAutoClear},
	{"clearbuttonmode", kMCNativeControlPropertyClearButtonMode},
	{"borderstyle", kMCNativeControlPropertyBorderStyle},
	{"verticaltextalign", kMCNativeControlPropertyVerticalTextAlign},
	
	{"editable", kMCNativeControlPropertyEditable},
	{"selectedrange", kMCNativeControlPropertySelectedRange},
	
	{"autocapitalizationtype", kMCNativeControlPropertyAutoCapitalizationType},
	{"autocorrectiontype", kMCNativeControlPropertyAutoCorrectionType},
	{"managereturnkey", kMCNativeControlPropertyManageReturnKey},
	{"keyboardtype", kMCNativeControlPropertyKeyboardType},
	{"keyboardstyle", kMCNativeControlPropertyKeyboardStyle},
	{"returnkeytype", kMCNativeControlPropertyReturnKeyType},
	{"contenttype", kMCNativeControlPropertyContentType},
	
    {"multiline", kMCNativeControlPropertyMultiLine},
    
	{nil, kMCNativeControlPropertyUnknown}
};

static struct {const char *name; MCNativeControlAction action;} s_native_control_actions[] =
{
	{"advance", kMCNativeControlActionAdvance},
	{"retreat", kMCNativeControlActionRetreat},
	{"reload", kMCNativeControlActionReload},
	{"stop", kMCNativeControlActionStop},
	{"execute", kMCNativeControlActionExecute},
	{"load", kMCNativeControlActionLoad},
	
	{"flashscrollindicators", kMCNativeControlActionFlashScrollIndicators},
	
	{"play", kMCNativeControlActionPlay},
	{"pause", kMCNativeControlActionPause},
	{"prepare", kMCNativeControlActionPrepareToPlay},
	{"begin seeking backward", kMCNativeControlActionBeginSeekingBackward},
	{"begin seeking forward", kMCNativeControlActionBeginSeekingForward},
	{"end seeking", kMCNativeControlActionEndSeeking},
	{"snapshot", kMCNativeControlActionSnapshot},
	{"snapshot exactly", kMCNativeControlActionSnapshotExactly},
	
	{"focus", kMCNativeControlActionFocus},
	
	{"scrollrangetovisible", kMCNativeControlActionScrollRangeToVisible},
	
	{nil, kMCNativeControlActionUnknown}
};

bool MCNativeControl::LookupProperty(const char *p_property, MCNativeControlProperty& r_prop)
{
	for(uint32_t i = 0; s_native_control_properties[i] . name != nil; i++)
		if (MCCStringEqualCaseless(p_property, s_native_control_properties[i] . name))
		{
			r_prop = s_native_control_properties[i] . property;
			return true;
		}
    
	return false;
}

bool MCNativeControl::LookupAction(const char *p_action, MCNativeControlAction& r_action)
{
	for(uint32_t i = 0; s_native_control_actions[i] . name != nil; i++)
		if (MCCStringEqualCaseless(p_action, s_native_control_actions[i] . name))
		{
			r_action = s_native_control_actions[i] . action;
			return true;
		}
	return false;
}

bool MCNativeControl::LookupType(const char *p_type, MCNativeControlType& r_type)
{
	for(uint32_t i = 0; s_native_control_types[i] . name != nil; i++)
		if (MCCStringEqualCaseless(p_type, s_native_control_types[i] . name))
		{
			r_type = s_native_control_types[i] . type;
			return true;
		}
	return false;
}

bool MCNativeControl::FindByNameOrId(const char *p_name, MCNativeControl*& r_control)
{
	char *t_id_end;
	uint32_t t_id;
	t_id = strtoul(p_name, &t_id_end, 10);
	if (t_id_end != p_name)
		return FindById(t_id, r_control);
	
	for(MCNativeControl *t_control = s_native_controls; t_control != nil; t_control = t_control -> m_next)
		if (!t_control -> m_deleted && t_control -> GetName() != nil && MCCStringEqualCaseless(t_control -> GetName(), p_name))
		{
			r_control = t_control;
			return true;
		}
	
	return false;
}

bool MCNativeControl::FindById(uint32_t p_id, MCNativeControl*& r_control)
{
	for(MCNativeControl *t_control = s_native_controls; t_control != nil; t_control = t_control -> m_next)
		if (!t_control -> m_deleted && t_control -> GetId() == p_id)
		{
			r_control = t_control;
			return true;
		}
	
	return false;
}

void MCNativeControl::Finalize(void)
{
    for(MCNativeControl *t_control = s_native_controls; t_control != nil;)
    {
        MCNativeControl *t_next_control;
        t_next_control = t_control->m_next;
        delete t_control;
        t_control = t_next_control;
    }    
}

////////////////////////////////////////////////////////////////////////////////

bool MCNativeControl::List(MCNativeControlListCallback p_callback, void *p_context)
{
	// MERG-2013-06-10: [[ Bug 10945 ]] Make sure we only list controls that haven't
	//   been deleted.
	for(MCNativeControl *t_control = s_native_controls; t_control != nil; t_control = t_control -> m_next)
		if (!t_control -> m_deleted)
            if (!p_callback(p_context, t_control))
                return false;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

extern bool MCNativeBrowserControlCreate(MCNativeControl*& r_control);
extern bool MCNativeScrollerControlCreate(MCNativeControl *&r_control);
extern bool MCNativePlayerControlCreate(MCNativeControl *&r_control);
extern bool MCNativeInputControlCreate(MCNativeControl *&r_control);
extern bool MCNativeMultiLineInputControlCreate(MCNativeControl *&r_control);

bool MCNativeControl::CreateWithType(MCNativeControlType p_type, MCNativeControl*& r_control)
{
    bool t_success = true;
    MCNativeControl *t_control = nil;
	switch(p_type)
	{
		case kMCNativeControlTypeBrowser:
			t_success = MCNativeBrowserControlCreate(t_control);
            break;
		case kMCNativeControlTypeScroller:
			t_success = MCNativeScrollerControlCreate(t_control);
            break;
		case kMCNativeControlTypePlayer:
			t_success = MCNativePlayerControlCreate(t_control);
            break;
		case kMCNativeControlTypeInput:
			t_success = MCNativeInputControlCreate(t_control);
            break;
		case kMCNativeControlTypeMultiLineInput:
			t_success = MCNativeMultiLineInputControlCreate(t_control);
			break;
            
		default:
            t_success = false;
			break;
	}
    
    if (t_success)
        t_success = t_control->Create();
    
    if (t_success)
    {
        t_control->m_next = s_native_controls;
        s_native_controls = t_control;
        
        r_control = t_control;
    }
    else if (t_control != nil)
        t_control->Release();
    
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecPointSetRect(MCExecPoint &ep, int2 p_left, int2 p_top, int2 p_right, int2 p_bottom)
{
	char *t_buffer = nil;
	if (!MCCStringFormat(t_buffer, "%d,%d,%d,%d", p_left, p_top, p_right, p_bottom))
		return false;
	
	ep.grabbuffer(t_buffer, MCCStringLength(t_buffer));
	return true;
}

static bool MCParseRGBA(const MCString &p_data, bool p_require_alpha, uint1 &r_red, uint1 &r_green, uint1 &r_blue, uint1 &r_alpha)
{
	bool t_success = true;
	Boolean t_parsed;
	uint2 r, g, b, a;
	const char *t_data = p_data.getstring();
	uint32_t l = p_data.getlength();
	if (t_success)
	{
		r = MCU_max(0, MCU_min(255, MCU_strtol(t_data, l, ',', t_parsed)));
		t_success = t_parsed;
	}
	if (t_success)
	{
		g = MCU_max(0, MCU_min(255, MCU_strtol(t_data, l, ',', t_parsed)));
		t_success = t_parsed;
	}
	if (t_success)
	{
		b = MCU_max(0, MCU_min(255, MCU_strtol(t_data, l, ',', t_parsed)));
		t_success = t_parsed;
	}
	if (t_success)
	{
		a = MCU_max(0, MCU_min(255, MCU_strtol(t_data, l, ',', t_parsed)));
		if (!t_parsed)
		{
			if (p_require_alpha)
				t_success = false;
			else
				a = 255;
		}
	}
	
	if (t_success)
	{
		r_red = r;
		r_green = g;
		r_blue = b;
		r_alpha = a;
	}
	return t_success;
}

bool MCNativeControl::ParseColor(MCExecPoint &ep, uint16_t &r_red, uint16_t &r_green, uint16_t &r_blue, uint16_t &r_alpha)
{
    uint8_t t_r8, t_g8, t_b8, t_a8;
    MCColor t_color;
    char *t_name = nil;
    if (MCParseRGBA(ep.getsvalue(), false, t_r8, t_g8, t_b8, t_a8))
    {
        r_red = (t_r8 << 8) | t_r8;
        r_green = (t_g8 << 8) | t_g8;
        r_blue = (t_b8 << 8) | t_b8;
        r_alpha = (t_a8 << 8) | t_a8;
        return true;
    }
    else if (MCscreen->parsecolor(ep.getsvalue(), &t_color, &t_name))
    {
        delete t_name;
        r_red = t_color.red;
        r_green = t_color.green;
        r_blue = t_color.blue;
        r_alpha = 0xFFFF;
        return true;
    }
    else
        return false;
}

bool MCNativeControl::FormatColor(MCExecPoint& ep, uint16_t p_red, uint16_t p_green, uint16_t p_blue, uint16_t p_alpha)
{
    char *t_colorstring = nil;

    p_red >>= 8;
    p_green >>= 8;
    p_blue >>= 8;
    p_alpha >>= 8;
    
    if (p_alpha != 255)
        MCCStringFormat(t_colorstring, "%u,%u,%u,%u", p_red, p_green, p_blue, p_alpha);
    else
        MCCStringFormat(t_colorstring, "%u,%u,%u", p_red, p_green, p_blue);
    ep.grabbuffer(t_colorstring, MCCStringLength(t_colorstring));
	
	return true;
}

bool MCNativeControl::ParseBoolean(MCExecPoint& ep, bool& r_value)
{
	Boolean t_bool;
	if (!MCU_stob(ep.getsvalue(), t_bool))
	{
		MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
		return false;
	}
	r_value = t_bool == True;
	return true;
}

bool MCNativeControl::FormatBoolean(MCExecPoint& ep, bool p_value)
{
	ep . setsvalue(MCU_btos(p_value));
	return true;
}

bool MCNativeControl::ParseInteger(MCExecPoint& ep, int32_t& r_value)
{
	if (!MCU_stoi4(ep . getsvalue(), r_value))
	{
		MCeerror->add(EE_OBJECT_NAN, 0, 0, ep.getsvalue());
		return false;
	}
	return true;
}

bool MCNativeControl::FormatInteger(MCExecPoint& ep, int32_t p_value)
{
	ep . setnvalue(p_value);
	return true;
}

bool MCNativeControl::ParseUnsignedInteger(MCExecPoint& ep, uint32_t& r_value)
{
	if (!MCU_stoui4(ep . getsvalue(), r_value))
	{
		MCeerror->add(EE_OBJECT_NAN, 0, 0, ep.getsvalue());
		return false;
	}
	return true;
}

bool MCNativeControl::ParseReal(MCExecPoint& ep, double& r_value)
{
	if (!MCU_stor8(ep . getsvalue(), r_value))
	{
		MCeerror->add(EE_OBJECT_NAN, 0, 0, ep.getsvalue());
		return false;
	}
	return true;
}

bool MCNativeControl::FormatReal(MCExecPoint& ep, double p_value)
{
	ep . setnvalue(p_value);
	return true;
}

bool MCNativeControl::ParseEnum(MCExecPoint& ep, MCNativeControlEnumEntry *p_entries, int32_t& r_value)
{
	for(uint32_t i = 0; p_entries[i] . key != nil; i++)
		if (MCCStringEqualCaseless(p_entries[i] . key, ep . getcstring()))
		{
			r_value = p_entries[i] . value;
			return true;
		}
	
	MCeerror->add(EE_OBJECT_BADSTYLE, 0, 0, ep.getsvalue());
	return false;
}

bool MCNativeControl::FormatEnum(MCExecPoint& ep, MCNativeControlEnumEntry *p_entries, int32_t p_value)
{
	for(uint32_t i = 0; p_entries[i] . key != nil; i++)
		if (p_entries[i] . value == p_value)
		{
			ep . setsvalue(p_entries[i] . key);
			return true;
		}
	
	ep . clear();
	return true;
}

bool MCNativeControl::ParseSet(MCExecPoint& ep, MCNativeControlEnumEntry *p_entries, int32_t& r_value)
{
    bool t_success = true;
    
	char **t_members_array;
	uint32_t t_members_count;
	t_members_array = nil;
	t_members_count = 0;
	if (t_success)
		t_success = MCCStringSplit(ep.getcstring(), ',', t_members_array, t_members_count);
	
	int32_t t_members_set;
	t_members_set = 0;
	if (t_success)
		for(uint32_t i = 0; t_success && i < t_members_count; i++)
        {
            bool t_found = false;
			for(uint32_t j = 0; !t_found && p_entries[j].key != nil; j++)
            {
				if (MCCStringEqualCaseless(t_members_array[i], p_entries[j].key))
                {
					t_members_set |= p_entries[j].value;
                    t_found = true;
                }
            }
            if (!t_found)
                t_success = false;
        }
	
	for(uint32_t i = 0; i < t_members_count; i++)
		MCCStringFree(t_members_array[i]);
	MCMemoryDeleteArray(t_members_array);
    
    if (t_success)
        r_value = t_members_set;
    
	return t_success;
}

bool MCNativeControl::FormatSet(MCExecPoint& ep, MCNativeControlEnumEntry *p_entries, int32_t p_value)
{
	bool t_first;
	t_first = true;
	
	ep . clear();
	
	for(uint32_t i = 0; p_entries[i] . key != nil; i++)
		if ((p_value & p_entries[i] . value) != 0)
		{
			ep . concatcstring(p_entries[i] . key, EC_COMMA, t_first);
			t_first = false;
		}
	
	return true;
}

bool MCNativeControl::ParseRectangle(MCExecPoint& ep, MCRectangle& r_rect)
{
	int16_t t_left, t_top, t_right, t_bottom;
	if (!MCU_stoi2x4(ep . getsvalue(), t_left, t_top, t_right, t_bottom))
	{
		MCeerror->add(EE_OBJECT_NAR, 0, 0, ep.getsvalue());
		return false;
	}
	
	MCU_set_rect(r_rect, t_left, t_top, t_right - t_left, t_bottom - t_top);
	return true;
}

bool MCNativeControl::ParseRectangle32(MCExecPoint& ep, MCRectangle32& r_rect)
{
	int32_t t_left, t_top, t_right, t_bottom;
	if (!MCU_stoi4x4(ep . getsvalue(), t_left, t_top, t_right, t_bottom))
	{
		MCeerror->add(EE_OBJECT_NAR, 0, 0, ep.getsvalue());
		return false;
	}
	
	MCU_set_rect(r_rect, t_left, t_top, t_right - t_left, t_bottom - t_top);
	return true;
}

bool MCNativeControl::ParseRange(MCExecPoint &ep, uint32_t &r_start, uint32_t &r_length)
{
	const char *sptr = ep.getsvalue().getstring();
	uint4 l = ep.getsvalue().getlength();
	uint32_t d1, d2;
	Boolean done;
	d1 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return false;
	d2 = MCU_strtol(sptr, l, '\0', done, True, False);
	if (!done || l != 0)
		return false;
    
    r_start = d1;
    r_length = d2;
    
    return true;
}

bool MCNativeControl::FormatRange(MCExecPoint &ep, uint32_t p_start, uint32_t p_length)
{
    ep.setnvalue(p_start);
	ep.concatint(p_length, EC_COMMA, false);
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

Exec_stat MCHandleControlCreate(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleControlCreate */ LEGACY_EXEC
	bool t_success;
	t_success = true;
	
	char *t_type_name;
	t_type_name = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_type_name);
	
	char *t_control_name;
	t_control_name = nil;
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "s", &t_control_name);
	
	// Make sure the name is valid.
	if (t_success && t_control_name != nil)
	{
		if (MCCStringIsEmpty(t_control_name))
		{
			delete t_control_name;
			t_control_name = nil;
		}
		else
			t_success = !MCCStringIsInteger(t_control_name);
	}
	
	// Make sure a control does not already exist with the name
	if (t_success && t_control_name != nil)
	{
		MCNativeControl *t_control;
		t_success = !MCNativeControl::FindByNameOrId(t_control_name, t_control);
	}
	
	MCNativeControlType t_type;
	if (t_success)
		t_success = MCNativeControl::LookupType(t_type_name, t_type);
	
	MCNativeControl *t_control;
	t_control = nil;
	if (t_success)
		t_success = MCNativeControl::CreateWithType(t_type, t_control);
	
	if (t_success)
	{
		extern MCExecPoint *MCEPptr;
		t_control -> SetOwner(MCEPptr -> getobj());
		t_control -> SetName(t_control_name);
		MCresult -> setnvalue(t_control -> GetId());
	}
	else
	{
		if (t_control != nil)
			t_control -> Delete();
		
		MCresult -> clear();
	}
	
	delete t_control_name;
	delete t_type_name;
	
	return ES_NORMAL;
#endif /* MCHandleControlCreate */
}

Exec_stat MCHandleControlDelete(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleControlDelete */ LEGACY_EXEC
	bool t_success;
	t_success = true;
	
	char *t_control_name;
	t_control_name = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_control_name);
	
	MCNativeControl *t_control;
	if (t_success)
		t_success = MCNativeControl::FindByNameOrId(t_control_name, t_control);
	
	if (t_success)
	{
		t_control -> Delete();
		t_control -> Release();
	}
	
	delete t_control_name;
	
	return ES_NORMAL;
#endif /* MCHandleControlDelete */
}

Exec_stat MCHandleControlSet(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleControlSet */ LEGACY_EXEC
	bool t_success;
	t_success = true;
	
	char *t_control_name;
	char *t_prop_name;
	t_control_name = nil;
	t_prop_name = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ss", &t_control_name, &t_prop_name);
	
	MCNativeControl *t_control;
	MCNativeControlProperty t_property;
	if (t_success)
		t_success =
		MCNativeControl::FindByNameOrId(t_control_name, t_control) &&
		MCNativeControl::LookupProperty(t_prop_name, t_property);
	
	MCExecPoint ep(nil, nil, nil);
	if (t_success && p_parameters != nil)
		t_success = p_parameters -> eval(ep);
	
	if (t_success)
		t_success = t_control -> Set(t_property, ep) == ES_NORMAL;
	
	delete t_prop_name;
	delete t_control_name;
	
	return ES_NORMAL;
#endif /* MCHandleControlSet */
}

Exec_stat MCHandleControlGet(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleControlGet */ LEGACY_EXEC
	bool t_success;
	t_success = true;
	
	char *t_control_name;
	char *t_prop_name;
	t_control_name = nil;
	t_prop_name = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ss", &t_control_name, &t_prop_name);
	
	MCNativeControl *t_control;
	MCNativeControlProperty t_property;
	if (t_success)
		t_success =
		MCNativeControl::FindByNameOrId(t_control_name, t_control) &&
		MCNativeControl::LookupProperty(t_prop_name, t_property);
	
	MCExecPoint ep(nil, nil, nil);
	if (t_success)
		t_success = t_control -> Get(t_property, ep) == ES_NORMAL;
	
	if (t_success)
		MCresult -> store(ep, True);
	else
		MCresult -> clear();
	
	delete t_prop_name;
	delete t_control_name;
	
	return ES_NORMAL;
	
#endif /* MCHandleControlGet */
}

Exec_stat MCHandleControlDo(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleControlDo */ LEGACY_EXEC
	bool t_success;
	t_success = true;
	
	char *t_control_name;
	char *t_action_name;
	t_control_name = nil;
	t_action_name = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ss", &t_control_name, &t_action_name);
	
	MCNativeControl *t_control;
	MCNativeControlAction t_action;
	if (t_success)
		t_success =
		MCNativeControl::FindByNameOrId(t_control_name, t_control) &&
		MCNativeControl::LookupAction(t_action_name, t_action);
	
	if (t_success)
		t_success = t_control -> Do(t_action, p_parameters) == ES_NORMAL;
	
	delete t_action_name;
	delete t_control_name;
	
	return ES_NORMAL;
#endif /* MCHandleControlDo */
}

Exec_stat MCHandleControlTarget(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleControlTarget */ LEGACY_EXEC
	MCNativeControl *t_target;
	t_target = MCNativeControl::CurrentTarget();
	if (t_target != nil)
	{
		if (t_target -> GetName() != nil)
			MCresult -> copysvalue(t_target -> GetName());
		else
			MCresult -> setnvalue(t_target -> GetId());
	}
	else
		MCresult -> clear();
	
	return ES_NORMAL;
#endif /* MCHandleControlTarget */
}

bool list_native_controls(void *context, MCNativeControl* p_control)
{
#ifdef /* list_native_controls */ LEGACY_EXEC
	MCExecPoint *ep;
	ep = (MCExecPoint *)context;
	
	if (p_control -> GetName() != nil)
		ep -> concatcstring(p_control -> GetName(), EC_RETURN, ep -> isempty());
	else
		ep -> concatuint(p_control -> GetId(), EC_RETURN, ep -> isempty());
	
	return true;
#endif /* list_native_controls */
}

Exec_stat MCHandleControlList(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleControlList */ LEGACY_EXEC
	MCExecPoint ep(nil, nil, nil);
	MCNativeControl::List(list_native_controls, &ep);
	MCresult -> store(ep, False);
	return ES_NORMAL;
#endif /* MCHandleControlList */
}

////////////////////////////////////////////////////////////////////////////////
