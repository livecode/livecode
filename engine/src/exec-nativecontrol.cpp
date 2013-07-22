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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "util.h"

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
#include "exec.h"

#include "mblsyntax.h"
#include "mblcontrol.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EXEC_METHOD(NativeControl, CreateControl, 2)
MC_EXEC_DEFINE_EXEC_METHOD(NativeControl, DeleteControl, 1)
MC_EXEC_DEFINE_EXEC_METHOD(NativeControl, SetProperty, 2)
MC_EXEC_DEFINE_EXEC_METHOD(NativeControl, GetProperty, 2)
MC_EXEC_DEFINE_EXEC_METHOD(NativeControl, Do, 0)
MC_EXEC_DEFINE_GET_METHOD(NativeControl, Target, 1)
MC_EXEC_DEFINE_GET_METHOD(NativeControl, ControlList, 1)

//////////

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

void MCNativeControlColorParse(MCExecContext& ctxt, MCStringRef p_input, MCNativeControlColor& r_output)
{
    uint8_t t_r8, t_g8, t_b8, t_a8;
    MCColor t_color;
    if (MCParseRGBA(MCStringGetOldString(p_input), false, t_r8, t_g8, t_b8, t_a8))
    {
        r_output . r = (t_r8 << 8) | t_r8;
        r_output . g = (t_g8 << 8) | t_g8;
        r_output . b = (t_b8 << 8) | t_b8;
        r_output . a = (t_a8 << 8) | t_a8;
        return;
    }
    
    if (MCscreen->parsecolor(p_input, t_color))
    {
        r_output . r = t_color.red;
        r_output . g = t_color.green;
        r_output . b = t_color.blue;
        r_output . a = 0xFFFF;
        return;
    }

    ctxt . LegacyThrow(EE_OBJECT_BADCOLOR);
}

void MCNativeControlColorFormat(MCExecContext& ctxt, const MCNativeControlColor& p_input, MCStringRef& r_output)
{
    uint16_t t_r, t_g, t_b, t_a;
    t_r = p_input . r >> 8;
    t_g = p_input . g >> 8;
    t_b = p_input . b >> 8;
    t_a = p_input . a >> 8;
    
    if (t_a != 255 && MCStringFormat(r_output, "%u,%u,%u,%u", t_r, t_g, t_b, t_a))
        return;
    
    if (MCStringFormat(r_output, "%u,%u,%u", t_r, t_g, t_b))
        return;
    
	ctxt . Throw();
}

void MCNativeControlColorFree(MCExecContext& ctxt, MCNativeControlColor& p_input)
{
}

static MCExecCustomTypeInfo _kMCNativeControlColorTypeInfo =
{
	"NativeControl.Color",
	sizeof(MCNativeControlColor),
	(void *)MCNativeControlColorParse,
	(void *)MCNativeControlColorFormat,
	(void *)MCNativeControlColorFree
};

//////////

void MCNativeControlDecelerationRateParse(MCExecContext& ctxt, MCStringRef p_input, MCNativeControlDecelerationRate& r_output)
{
    if (MCStringIsEqualToCString(p_input, "normal", kMCCompareCaseless))
    {
        r_output . type  = kMCNativeControlDecelerationRateNormal;
        return;
    }
    else if (MCStringIsEqualToCString(p_input, "fast", kMCCompareCaseless))
    {
        r_output . type  = kMCNativeControlDecelerationRateFast;
        return;
    }
    else if (MCU_stor8(p_input, r_output . rate))
    {
        r_output . type = kMCNativeControlDecelerationRateCustom;
        return;
    }
    
    ctxt . LegacyThrow(EE_OBJECT_NAN);
}

void MCNativeControlDecelerationRateFormat(MCExecContext& ctxt, const MCNativeControlDecelerationRate& p_input, MCStringRef& r_output)
{
    if (MCStringFormat(r_output, "%f", p_input . rate))
        return;
    
    ctxt . Throw();
        
}

void MCNativeControlDecelerationRateFree(MCExecContext& ctxt, MCNativeControlDecelerationRate& p_input)
{
}

static MCExecCustomTypeInfo _kMCNativeControlDecelerationRateTypeInfo =
{
	"NativeControl.DecelerationRate",
	sizeof(MCNativeControlDecelerationRate),
	(void *)MCNativeControlDecelerationRateParse,
	(void *)MCNativeControlDecelerationRateFormat,
	(void *)MCNativeControlDecelerationRateFree
};

//////////

void MCNativeControlIndicatorInsetsParse(MCExecContext& ctxt, MCStringRef p_input, MCNativeControlIndicatorInsets& r_output)
{
    if (MCU_stoi2x4(p_input, r_output . top, r_output . left, r_output . right, r_output . bottom))
        return;
    
    ctxt . LegacyThrow(EE_OBJECT_NAN);
}

void MCNativeControlIndicatorInsetsFormat(MCExecContext& ctxt, const MCNativeControlIndicatorInsets& p_input, MCStringRef& r_output)
{
    if (MCStringFormat(r_output, "%d,%d,%d,%d", p_input . top, p_input . left, p_input . right, p_input . bottom))
        return;
    
    ctxt . Throw();
    
}

void MCNativeControlIndicatorInsetsFree(MCExecContext& ctxt, MCNativeControlIndicatorInsets& p_input)
{
}

static MCExecCustomTypeInfo _kMCNativeControlIndicatorInsetsTypeInfo =
{
	"NativeControl.IndicatorInsets",
	sizeof(MCNativeControlIndicatorInsets),
	(void *)MCNativeControlIndicatorInsetsParse,
	(void *)MCNativeControlIndicatorInsetsFormat,
	(void *)MCNativeControlIndicatorInsetsFree
};

//////////

void MCNativeControlRangeParse(MCExecContext& ctxt, MCStringRef p_input, MCNativeControlRange& r_output)
{
    if (MCU_stoui4x2(p_input, r_output . start, r_output . length))
        return;
    
    ctxt . LegacyThrow(EE_OBJECT_NAN);
}

void MCNativeControlRangeFormat(MCExecContext& ctxt, const MCNativeControlRange& p_input, MCStringRef& r_output)
{
    if (MCStringFormat(r_output, "%d,%d", p_input . start, p_input . length))
        return;
    
    ctxt . Throw();
    
}

void MCNativeControlRangeFree(MCExecContext& ctxt, MCNativeControlRange& p_input)
{
}

static MCExecCustomTypeInfo _kMCNativeControlRangeTypeInfo =
{
	"NativeControl.Range",
	sizeof(MCNativeControlRange),
	(void *)MCNativeControlRangeParse,
	(void *)MCNativeControlRangeFormat,
	(void *)MCNativeControlRangeFree
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlIndicatorStyleElementInfo[] =
{
    { "", kMCNativeControlIndicatorStyleEmpty },
    { "default", kMCNativeControlIndicatorStyleDefault },
    { "white", kMCNativeControlIndicatorStyleWhite },
    { "black", kMCNativeControlIndicatorStyleBlack }
};

static MCExecEnumTypeInfo _kMCNativeControlIndicatorStyleTypeInfo =
{
    "NativeControl.IndicatorStyle",
    sizeof(_kMCNativeControlIndicatorStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlIndicatorStyleElementInfo
};

//////////

static MCExecSetTypeElementInfo _kMCNativeControlLoadStateElementInfo[] =
{
    { "", kMCNativeControlLoadStateNone },
	{ "playable", kMCNativeControlLoadStatePlayable },
	{ "playthrough", kMCNativeControlLoadStatePlaythroughOK },
	{ "stalled", kMCNativeControlLoadStateStalled },
};

static MCExecSetTypeInfo _kMCNativeControlLoadStateTypeInfo =
{
    "NativeControl.LoadState",
    sizeof(_kMCNativeControlLoadStateElementInfo) / sizeof(MCExecSetTypeElementInfo),
    _kMCNativeControlLoadStateElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlPlaybackStateElementInfo[] =
{
    { "", kMCNativeControlPlaybackStateNone },
	{ "stopped", kMCNativeControlPlaybackStateStopped },
	{ "playing", kMCNativeControlPlaybackStatePlaying },
	{ "paused", kMCNativeControlPlaybackStatePaused },
	{ "interrupted", kMCNativeControlPlaybackStateInterrupted },
	{ "seeking forward", kMCNativeControlPlaybackStateSeekingForward },
	{ "seeking backward", kMCNativeControlPlaybackStateSeekingBackward },
};

static MCExecEnumTypeInfo _kMCNativeControlPlaybackStateTypeInfo =
{
    "NativeControl.PlaybackState",
    sizeof(_kMCNativeControlPlaybackStateElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlPlaybackStateElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlInputCapitalizationTypeElementInfo[] =
{
    {"none", kMCNativeControlInputCapitalizeNone},
    {"words", kMCNativeControlInputCapitalizeWords},
    {"sentences", kMCNativeControlInputCapitalizeSentences},
    {"all characters", kMCNativeControlInputCapitalizeCharacters},
};

static MCExecEnumTypeInfo _kMCNativeControlInputCapitalizationTypeTypeInfo =
{
    "NativeControl.InputCapitalizationType",
    sizeof(_kMCNativeControlInputCapitalizationTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlInputCapitalizationTypeElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlInputAutocorrectionTypeElementInfo[] =
{
    {"default", kMCNativeControlInputAutocorrectionDefault},
    {"no", kMCNativeControlInputAutocorrectionNo},
    {"yes", kMCNativeControlInputAutocorrectionYes},
};

static MCExecEnumTypeInfo _kMCNativeControlInputAutocorrectiontionTypeTypeInfo =
{
    "NativeControl.InputAutocorrectionType",
    sizeof(_kMCNativeControlInputAutocorrectionTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlInputAutocorrectionTypeElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlInputKeyboardTypeElementInfo[] =
{
    { "default", kMCNativeControlInputKeyboardTypeDefault},
    { "alphabet", kMCNativeControlInputKeyboardTypeAlphabet},
    { "numeric", kMCNativeControlInputKeyboardTypeNumeric},
    { "url", kMCNativeControlInputKeyboardTypeURL},
    { "number", kMCNativeControlInputKeyboardTypeNumber},
    { "phone", kMCNativeControlInputKeyboardTypePhone},
    { "contact", kMCNativeControlInputKeyboardTypeContact},
    { "email", kMCNativeControlInputKeyboardTypeEmail},
    { "decimal", kMCNativeControlInputKeyboardTypeDecimal},
};

static MCExecEnumTypeInfo _kMCNativeControlInputKeyboardTypeTypeInfo =
{
    "NativeControl.InputKeyboardType",
    sizeof(_kMCNativeControlInputKeyboardTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlInputKeyboardTypeElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlInputKeyboardStyleElementInfo[] =
{
    { "default", kMCNativeControlInputKeyboardStyleDefault},
    { "alert", kMCNativeControlInputKeyboardStyleAlert},
};

static MCExecEnumTypeInfo _kMCNativeControlInputKeyboardStyleTypeInfo =
{
    "NativeControl.InputKeyboardStyle",
    sizeof(_kMCNativeControlInputKeyboardStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlInputKeyboardStyleElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlInputReturnKeyTypeElementInfo[] =
{
	{"default", kMCNativeControlInputReturnKeyTypeDefault},
	{"go", kMCNativeControlInputReturnKeyTypeGo},
	{"next", kMCNativeControlInputReturnKeyTypeNext},
	{"search", kMCNativeControlInputReturnKeyTypeSearch},
	{"send", kMCNativeControlInputReturnKeyTypeSend},
    {"done", kMCNativeControlInputReturnKeyTypeDone},
#if defined(TARGET_SUBPLATFORM_IPHONE)
    {"route", kMCNativeControlInputReturnKeyTypeRoute},
	{"yahoo", kMCNativeControlInputReturnKeyTypeYahoo},
    {"google", kMCNativeControlInputReturnKeyTypeGoogle},
	{"join", kMCNativeControlInputReturnKeyTypeJoin},
	{"emergency call", kMCNativeControlInputReturnKeyTypeEmergencyCall},
#endif
};

static MCExecEnumTypeInfo _kMCNativeControlInputReturnKeyTypeTypeInfo =
{
    "NativeControl.InputReturnKeyType",
    sizeof(_kMCNativeControlInputReturnKeyTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlInputReturnKeyTypeElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlInputContentTypeElementInfo[] =
{
    { "plain", kMCNativeControlInputContentTypePlain},
    { "password", kMCNativeControlInputContentTypePassword},
};

static MCExecEnumTypeInfo _kMCNativeControlInputContentTypeTypeInfo =
{
    "NativeControl.InputContentType",
    sizeof(_kMCNativeControlInputContentTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlInputContentTypeElementInfo
};

//////////

static MCExecSetTypeElementInfo _kMCNativeControlInputDataDetectorTypeElementInfo[] =
{
    { "", kMCNativeControlInputDataDetectorTypeNone },
    { "none", kMCNativeControlInputDataDetectorTypeNone },
    { "link", kMCNativeControlInputDataDetectorTypeWebUrl },
    { "address", kMCNativeControlInputDataDetectorTypeMapAddress },
    { "phone number", kMCNativeControlInputDataDetectorTypePhoneNumber },
    { "email", kMCNativeControlInputDataDetectorTypeEmailAddress },
    { "calendar event", kMCNativeControlInputDataDetectorTypeCalendarEvent },
    { "all", kMCNativeControlInputDataDetectorTypeAll },
};

static MCExecSetTypeInfo _kMCNativeControlInputDataDetectorTypeTypeInfo =
{
    "NativeControl.InputDataDetectorType",
    sizeof(_kMCNativeControlInputDataDetectorTypeElementInfo) / sizeof(MCExecSetTypeElementInfo),
    _kMCNativeControlInputDataDetectorTypeElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlInputTextAlignElementInfo[] =
{
    { "center", kMCNativeControlInputTextAlignCenter },
    { "left", kMCNativeControlInputTextAlignLeft },
    { "right", kMCNativeControlInputTextAlignRight },
};

static MCExecEnumTypeInfo _kMCNativeControlInputTextAlignTypeInfo =
{
    "NativeControl.InputTextAlign",
    sizeof(_kMCNativeControlInputTextAlignElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlInputTextAlignElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlInputVerticalAlignElementInfo[] =
{
    { "center", kMCNativeControlInputVerticalAlignCenter },
    { "top", kMCNativeControlInputVerticalAlignTop },
    { "bottom", kMCNativeControlInputVerticalAlignBottom },
};

static MCExecEnumTypeInfo _kMCNativeControlInputVerticalAlignTypeInfo =
{
    "NativeControl.InputVerticalAlign",
    sizeof(_kMCNativeControlInputVerticalAlignElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlInputVerticalAlignElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlClearButtonModeElementInfo[] =
{
	{"never", kMCNativeControlClearButtonModeNever},
	{"while editing", kMCNativeControlClearButtonModeWhileEditing},
	{"unless editing", kMCNativeControlClearButtonModeUnlessEditing},
	{"always", kMCNativeControlClearButtonModeAlways},
};

static MCExecEnumTypeInfo _kMCNativeControlClearButtonModeTypeInfo =
{
    "NativeControl.ClearButtonMode",
    sizeof(_kMCNativeControlClearButtonModeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlClearButtonModeElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlBorderStyleElementInfo[] =
{
	{"none", kMCNativeControlBorderStyleNone},
	{"line", kMCNativeControlBorderStyleLine},
	{"bezel", kMCNativeControlBorderStyleBezel},
	{"rounded", kMCNativeControlBorderStyleRoundedRect},
};

static MCExecEnumTypeInfo _kMCNativeControlBorderStyleTypeInfo =
{
    "NativeControl.BorderStyle",
    sizeof(_kMCNativeControlBorderStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlBorderStyleElementInfo
};


////////////////////////////////////////////////////////////////////////////////

MCExecCustomTypeInfo *kMCNativeControlColorTypeInfo = &_kMCNativeControlColorTypeInfo;

MCExecCustomTypeInfo *kMCNativeControlDecelerationRateTypeInfo = &_kMCNativeControlDecelerationRateTypeInfo;
MCExecCustomTypeInfo *kMCNativeControlIndicatorInsetsTypeInfo = &_kMCNativeControlIndicatorInsetsTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlIndicatorStyleTypeInfo = &_kMCNativeControlIndicatorStyleTypeInfo;

MCExecEnumTypeInfo *kMCNativeControlPlaybackStateTypeInfo = &_kMCNativeControlPlaybackStateTypeInfo;
MCExecSetTypeInfo *kMCNativeControlLoadStateTypeInfo = &_kMCNativeControlLoadStateTypeInfo;

MCExecEnumTypeInfo *kMCNativeControlInputCapitalizationTypeTypeInfo = &_kMCNativeControlInputCapitalizationTypeTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlInputAutocorrectiontionTypeTypeInfo = &_kMCNativeControlInputAutocorrectiontionTypeTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlInputKeyboardTypeTypeInfo = &_kMCNativeControlInputKeyboardTypeTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlInputKeyboardStyleTypeInfo = &_kMCNativeControlInputKeyboardStyleTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlInputReturnKeyTypeTypeInfo = &_kMCNativeControlInputReturnKeyTypeTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlInputContentTypeTypeInfo = &_kMCNativeControlInputContentTypeTypeInfo;
MCExecSetTypeInfo *kMCNativeControlInputDataDetectorTypeTypeInfo = &_kMCNativeControlInputDataDetectorTypeTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlInputTextAlignTypeInfo = &_kMCNativeControlInputTextAlignTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlInputVerticalAlignTypeInfo = &_kMCNativeControlInputVerticalAlignTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlClearButtonModeTypeInfo = &_kMCNativeControlClearButtonModeTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlBorderStyleTypeInfo = &_kMCNativeControlBorderStyleTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCNativeControlExecCreateControl(MCExecContext& ctxt, MCStringRef p_type_name, MCStringRef p_control_name)
{
    ctxt . SetTheResultToEmpty();
    
    // Make sure the name is valid.
    if (MCStringIsEqualTo(p_control_name, kMCEmptyString, kMCCompareCaseless))
        return;
    
    int2 t_integer;
    if (MCU_stoi2(p_control_name, t_integer))
        return;
    
    // Make sure a control does not already exist with the name
    MCNativeControl *t_control;
    if (MCNativeControl::FindByNameOrId(MCStringGetCString(p_control_name), t_control))
        return;
    
    MCNativeControlType t_type;
    if (!MCNativeControl::LookupType(MCStringGetCString(p_type_name), t_type))
        return;
    
    MCNativeControl *t_new_control;
    t_new_control = nil;
    if (MCNativeControl::CreateWithType(t_type, t_new_control))
    {
        extern MCExecPoint *MCEPptr;
        t_control -> SetOwner(MCEPptr -> getobj());
        t_control -> SetName(p_control_name);
        ctxt . SetTheResultToNumber(t_new_control -> GetId());
        return;
    }
    else
    {
        if (t_control != nil)
            t_control -> Delete();
    
        ctxt . SetTheResultToEmpty();
    }
}

void MCNativeControlExecDeleteControl(MCExecContext& ctxt, MCStringRef p_control_name)
{
    MCNativeControl *t_control;
    if (!MCNativeControl::FindByNameOrId(MCStringGetCString(p_control_name), t_control))
        return;
    
    t_control -> Delete();
    t_control -> Release();
}

void MCNativeControlExecDo(MCExecContext& ctxt)
{
    ctxt . Unimplemented();
}

void MCNativeControlGetTarget(MCExecContext& ctxt, MCStringRef& r_target)
{
    // UNION TYPE!
    ctxt . Unimplemented();
}

void MCNativeControlGetControlList(MCExecContext& ctxt, MCStringRef& r_list)
{
    if (MCNativeControl::GetControlList(r_list))
        return;
    
    ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCNativeControl::Set(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
    switch (p_property)
    {
        case kMCNativeControlPropertyName:
        {
            MCAutoStringRef t_name;
            /* UNCHECKED */ ep . copyasstringref(&t_name);
            SetName(ctxt, &t_name);
            return;
        }
        default:
            break;
    }
    ctxt . LegacyThrow(EE_OBJECT_SETNOPROP);
}

void MCNativeControl::Get(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
    switch (p_property)
    {
        case kMCNativeControlPropertyId:
        {
            uint32_t t_id;
            GetId(ctxt, t_id);
            ep.setnvalue(t_id);
            return;
        }
            
        case kMCNativeControlPropertyName:
        {
            MCAutoStringRef t_name;
            GetName(ctxt, &t_name);
            
            if (!ctxt . HasError())
                ep.setvalueref(*t_name);
            return;
        }
        default:
            break;
    }
    ctxt . LegacyThrow(EE_OBJECT_SETNOPROP);
}

void MCNativeControl::GetId(MCExecContext& ctxt, uinteger_t& r_id)
{
    r_id = m_id;
}

void MCNativeControl::GetName(MCExecContext& ctxt, MCStringRef& r_name)
{
    if (m_name != nil)
    {
        if (MCStringCreateWithCString(m_name, r_name))
            return;
    }
    else
    {
        r_name = MCValueRetain(kMCEmptyString);
        return;
    }
    ctxt . Throw();
}

void MCNativeControl::SetName(MCExecContext& ctxt, MCStringRef p_name)
{
    if (m_name != nil)
	{
		MCCStringFree(m_name);
		m_name = nil;
	}
	
	if (p_name != nil)
    {
        if (MCCStringClone(MCStringGetCString(p_name), m_name))
            return;
	}
    else
        return;
	
    ctxt . Throw();
}

void ctxt_to_enum(MCExecContext& ctxt, MCExecEnumTypeInfo* p_enum_info, intenum_t& r_enum)
{
    bool t_found;
    t_found = false;
    for(uindex_t i = 0; i < p_enum_info -> count; i++)
        if (!p_enum_info -> elements[i] . read_only &&
            MCU_strcasecmp(ctxt . GetEP() . getcstring(), p_enum_info -> elements[i] . tag) == 0)
        {
            t_found = true;
            r_enum = p_enum_info -> elements[i] . value;
        }
    
    if (!t_found)
        ctxt . LegacyThrow(EE_PROPERTY_BADENUMVALUE);
}
    
void ctxt_to_set(MCExecContext& ctxt, MCExecSetTypeInfo* p_set_info, intset_t& r_set)
{
    r_set = 0;
    char **t_elements;
    uindex_t t_element_count;
    MCCStringSplit(ctxt . GetEP() . getcstring(), ',', t_elements, t_element_count);
    
    for (uindex_t i = 0; i < t_element_count; i++)
    {
        for (uindex_t j = 0; j < p_set_info -> count; j++)
        {
            if (MCU_strcasecmp(t_elements[i], p_set_info -> elements[j] . tag) == 0)
            {
                r_set |= 1 << p_set_info -> elements[j] . bit;
                break;
            }
        }
    }
    
    MCCStringArrayFree(t_elements, t_element_count);
}

void enum_to_ctxt(MCExecContext& ctxt, MCExecEnumTypeInfo* p_enum_info, intenum_t p_enum)
{
    for(uindex_t i = 0; i < p_enum_info -> count; i++)
        if (p_enum_info -> elements[i] . value == p_enum)
        {
            ctxt . GetEP() . setcstring(p_enum_info -> elements[i] . tag);
            return;
        }
    
    // THIS MEANS A METHOD HAS RETURNED AN ILLEGAL VALUE
    MCAssert(false);
}

void set_to_ctxt(MCExecContext& ctxt, MCExecSetTypeInfo* p_set_info, intset_t p_set)
{
    bool t_first;
    t_first = true;
    
    ctxt . GetEP() . clear();
    for(uindex_t i = 0; i < p_set_info -> count; i++)
        if (((1 << p_set_info -> elements[i] . bit) & p_set) != 0)
        {
            ctxt . GetEP() . concatcstring(p_set_info -> elements[i] . tag, EC_COMMA, t_first);
            t_first = false;
        }
}
