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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "util.h"

#include "mcerror.h"

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

static bool MCParseRGBA(MCStringRef p_data, bool p_require_alpha, uint1 &r_red, uint1 &r_green, uint1 &r_blue, uint1 &r_alpha)
{
	bool t_success = true;
	Boolean t_parsed;
	uint2 r, g, b, a;

    MCAutoStringRefAsCString t_data_str;
    if (t_success)
        t_success = t_data_str.Lock(p_data);
    const char *t_data = *t_data_str;
    uint32_t l = t_data_str.Size();

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
    if (MCParseRGBA(p_input, false, t_r8, t_g8, t_b8, t_a8))
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
    if (!p_input . has_insets)
    {
        r_output = MCValueRetain(kMCEmptyString);
        return;
    }
    
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

void MCNativeControlIdentifierParse(MCExecContext& ctxt, MCStringRef p_input, MCNativeControlIdentifier& r_output)
{
    if (MCU_stoui4(p_input, r_output . id))
    {
        r_output . type = kMCNativeControlIdentifierId;
        return;
    }
    
    r_output . type = kMCNativeControlIdentifierName;
    r_output . name = MCValueRetain(p_input);
}

void MCNativeControlIdentifierFormat(MCExecContext& ctxt, const MCNativeControlIdentifier& p_input, MCStringRef& r_output)
{
    if (p_input . type == kMCNativeControlIdentifierName)
    {
        if (p_input . name != nil)
            r_output = MCValueRetain(p_input . name);
        else
            r_output = MCValueRetain(kMCEmptyString);
        return;
    }
    
    if (MCStringFormat(r_output, "%d", p_input . id))
        return;
    
    ctxt . Throw();
}

void MCNativeControlIdentifierFree(MCExecContext& ctxt, MCNativeControlIdentifier& p_input)
{
    if (p_input . type == kMCNativeControlIdentifierName && p_input . name != nil)
        MCValueRelease(p_input . name);
}

static MCExecCustomTypeInfo _kMCNativeControlIdentifierTypeInfo =
{
	"NativeControl.Identifier",
	sizeof(MCNativeControlIdentifier),
	(void *)MCNativeControlIdentifierParse,
	(void *)MCNativeControlIdentifierFormat,
	(void *)MCNativeControlIdentifierFree
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlIndicatorStyleElementInfo[] =
{
    { "", kMCNativeControlIndicatorStyleEmpty, false },
    { "default", kMCNativeControlIndicatorStyleDefault, false },
    { "white", kMCNativeControlIndicatorStyleWhite, false },
    { "black", kMCNativeControlIndicatorStyleBlack, false }
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
    { "", kMCNativeControlPlaybackStateNone, false },
	{ "stopped", kMCNativeControlPlaybackStateStopped, false },
	{ "playing", kMCNativeControlPlaybackStatePlaying, false },
	{ "paused", kMCNativeControlPlaybackStatePaused, false },
	{ "interrupted", kMCNativeControlPlaybackStateInterrupted, false },
	{ "seeking forward", kMCNativeControlPlaybackStateSeekingForward, false },
	{ "seeking backward", kMCNativeControlPlaybackStateSeekingBackward, false },
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
    {"none", kMCNativeControlInputCapitalizeNone, false},
    {"words", kMCNativeControlInputCapitalizeWords, false},
    {"sentences", kMCNativeControlInputCapitalizeSentences, false},
    {"all characters", kMCNativeControlInputCapitalizeCharacters, false},
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
    {"default", kMCNativeControlInputAutocorrectionDefault, false},
    {"no", kMCNativeControlInputAutocorrectionNo, false},
    {"yes", kMCNativeControlInputAutocorrectionYes, false},
};

static MCExecEnumTypeInfo _kMCNativeControlInputAutocorrectionTypeTypeInfo =
{
    "NativeControl.InputAutocorrectionType",
    sizeof(_kMCNativeControlInputAutocorrectionTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlInputAutocorrectionTypeElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCNativeControlInputKeyboardTypeElementInfo[] =
{
    { "default", kMCNativeControlInputKeyboardTypeDefault, false},
    { "alphabet", kMCNativeControlInputKeyboardTypeAlphabet, false},
    { "numeric", kMCNativeControlInputKeyboardTypeNumeric, false},
    { "url", kMCNativeControlInputKeyboardTypeURL, false},
    { "number", kMCNativeControlInputKeyboardTypeNumber, false},
    { "phone", kMCNativeControlInputKeyboardTypePhone, false},
    { "contact", kMCNativeControlInputKeyboardTypeContact, false},
    { "email", kMCNativeControlInputKeyboardTypeEmail, false},
    { "decimal", kMCNativeControlInputKeyboardTypeDecimal, false},
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
    { "default", kMCNativeControlInputKeyboardStyleDefault, false},
    { "alert", kMCNativeControlInputKeyboardStyleAlert, false},
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
	{"default", kMCNativeControlInputReturnKeyTypeDefault, false},
	{"go", kMCNativeControlInputReturnKeyTypeGo, false},
	{"next", kMCNativeControlInputReturnKeyTypeNext, false},
	{"search", kMCNativeControlInputReturnKeyTypeSearch, false},
	{"send", kMCNativeControlInputReturnKeyTypeSend, false},
    {"done", kMCNativeControlInputReturnKeyTypeDone, false},
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
    { "plain", kMCNativeControlInputContentTypePlain, false},
    { "password", kMCNativeControlInputContentTypePassword, false},
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
    { "none", kMCNativeControlInputDataDetectorTypeNoneBit },
    { "link", kMCNativeControlInputDataDetectorTypeWebUrlBit },
    { "address", kMCNativeControlInputDataDetectorTypeMapAddressBit },
    { "phone number", kMCNativeControlInputDataDetectorTypePhoneNumberBit },
    { "email", kMCNativeControlInputDataDetectorTypeEmailAddressBit },
    { "calendar event", kMCNativeControlInputDataDetectorTypeCalendarEventBit },
    { "all", kMCNativeControlInputDataDetectorTypeAllBit },
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
    { "center", kMCNativeControlInputTextAlignCenter, false },
    { "left", kMCNativeControlInputTextAlignLeft, false },
    { "right", kMCNativeControlInputTextAlignRight, false },
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
    { "center", kMCNativeControlInputVerticalAlignCenter, false },
    { "top", kMCNativeControlInputVerticalAlignTop, false },
    { "bottom", kMCNativeControlInputVerticalAlignBottom, false },
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
	{"never", kMCNativeControlClearButtonModeNever, false},
	{"while editing", kMCNativeControlClearButtonModeWhileEditing, false},
	{"unless editing", kMCNativeControlClearButtonModeUnlessEditing, false},
	{"always", kMCNativeControlClearButtonModeAlways, false},
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
	{"none", kMCNativeControlBorderStyleNone, false},
	{"line", kMCNativeControlBorderStyleLine, false},
	{"bezel", kMCNativeControlBorderStyleBezel, false},
	{"rounded", kMCNativeControlBorderStyleRoundedRect, false},
};

static MCExecEnumTypeInfo _kMCNativeControlBorderStyleTypeInfo =
{
    "NativeControl.BorderStyle",
    sizeof(_kMCNativeControlBorderStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCNativeControlBorderStyleElementInfo
};


////////////////////////////////////////////////////////////////////////////////

MCExecCustomTypeInfo *kMCNativeControlColorTypeInfo = &_kMCNativeControlColorTypeInfo;
MCExecCustomTypeInfo *kMCNativeControlRangeTypeInfo = &_kMCNativeControlRangeTypeInfo;
MCExecCustomTypeInfo *kMCNativeControlIdentifierTypeInfo = &_kMCNativeControlIdentifierTypeInfo;

MCExecCustomTypeInfo *kMCNativeControlDecelerationRateTypeInfo = &_kMCNativeControlDecelerationRateTypeInfo;
MCExecCustomTypeInfo *kMCNativeControlIndicatorInsetsTypeInfo = &_kMCNativeControlIndicatorInsetsTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlIndicatorStyleTypeInfo = &_kMCNativeControlIndicatorStyleTypeInfo;

MCExecEnumTypeInfo *kMCNativeControlPlaybackStateTypeInfo = &_kMCNativeControlPlaybackStateTypeInfo;
MCExecSetTypeInfo *kMCNativeControlLoadStateTypeInfo = &_kMCNativeControlLoadStateTypeInfo;

MCExecEnumTypeInfo *kMCNativeControlInputCapitalizationTypeTypeInfo = &_kMCNativeControlInputCapitalizationTypeTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlInputAutocorrectionTypeTypeInfo = &_kMCNativeControlInputAutocorrectionTypeTypeInfo;
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
    
    if (p_control_name != nil)
    {
        // Make sure the name is valid.
        if (MCStringIsEqualTo(p_control_name, kMCEmptyString, kMCCompareCaseless))
            return;
        
        int2 t_integer;
        if (MCU_stoi2(p_control_name, t_integer))
            return;
    }
    
    if (p_control_name != nil)
    {
        MCNativeControl *t_control;
        // Make sure a control does not already exist with the name
        if (MCNativeControl::FindByNameOrId(p_control_name, t_control))
            return;
    }
    
    MCNativeControlType t_type;
    if (!MCLookupNativeControlType(p_type_name, (intenum_t&)t_type))
        return;
    
    MCNativeControl *t_new_control;
    t_new_control = nil;
    if (MCNativeControl::CreateWithType(t_type, t_new_control))
    {
        extern MCExecContext *MCECptr;
        t_new_control -> SetOwner(MCECptr -> GetObject());
        t_new_control -> SetName(p_control_name);
        ctxt . SetTheResultToNumber(t_new_control -> GetId());
        return;
    }
    else
    {
        if (t_new_control != nil)
            t_new_control -> Delete();
    
        ctxt . SetTheResultToEmpty();
    }
}

void MCNativeControlExecDeleteControl(MCExecContext& ctxt, MCStringRef p_control_name)
{
    MCNativeControl *t_control;
    if (!MCNativeControl::FindByNameOrId(p_control_name, t_control))
        return;
    
    t_control -> Delete();
    t_control -> Release();
}

void MCNativeControlGetTarget(MCExecContext& ctxt, MCNativeControlIdentifier& r_target)
{
	MCNativeControl *t_target;
	t_target = MCNativeControl::CurrentTarget();
	if (t_target != nil)
	{
        MCAutoStringRef t_name;
        t_target -> GetName(&t_name);
		if (!MCStringIsEmpty(*t_name))
        {
            r_target . type = kMCNativeControlIdentifierName;
			t_target -> GetName(ctxt, r_target . name);
        }
		else
        {
            r_target . type = kMCNativeControlIdentifierId;
            t_target -> GetId(ctxt, r_target . id);
        }
	}
	else
    {
        r_target . type = kMCNativeControlIdentifierName;
        // SN-2014-03-25: [[ Bug 11981 ]] calling mobileControlTarget () crashes the application
        r_target . name = nil;
    }
}

void MCNativeControlGetControlList(MCExecContext& ctxt, MCStringRef& r_list)
{
    if (MCNativeControl::GetControlList(r_list))
        return;
    
    ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

static MCPropertyInfo *lookup_control_property(const MCObjectPropertyTable *p_table, Properties p_which)
{
	for(uindex_t i = 0; i < p_table -> size; i++)
		if (p_table -> table[i] . property == p_which)
			return &p_table -> table[i];
	
	if (p_table -> parent != nil)
		return lookup_control_property(p_table -> parent, p_which);
	
	return nil;
}

void MCNativeControlExecGet(MCExecContext& ctxt, MCStringRef p_control_name, MCStringRef p_property_name, MCValueRef& r_result)
{
    MCNativeControl *t_native_control;
    if (!MCNativeControl::FindByNameOrId(p_control_name, t_native_control))
        return;
    
    Properties t_property;
    if (!MCLookupNativeControlProperty(p_property_name, (intenum_t&)t_property))
        return;

    MCPropertyInfo *t_info;
    t_info = lookup_control_property(t_native_control -> getpropertytable(), t_property);
    
    if (t_info != nil && t_info -> getter == nil)
    {
        ctxt . LegacyThrow(EE_OBJECT_GETNOPROP);
        return;
    }
    
    if (t_info != nil)
    {
        MCNativeControlPtr t_control;
        t_control . control = t_native_control;

		MCExecValue t_value;
        MCExecFetchProperty(ctxt, t_info, &t_control, t_value);
		MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeValueRef, &r_result);
    }
}

void MCNativeControlExecSet(MCExecContext& ctxt, MCStringRef p_control_name, MCStringRef p_property_name, MCValueRef p_value)
{
    MCNativeControl *t_native_control;
    if (!MCNativeControl::FindByNameOrId(p_control_name, t_native_control))
        return;
    
    Properties t_property;
    if (!MCLookupNativeControlProperty(p_property_name, (intenum_t&)t_property))
        return;
    
    MCPropertyInfo *t_info;
    t_info = lookup_control_property(t_native_control -> getpropertytable(), t_property);
    
    if (t_info != nil && t_info -> setter == nil)
    {
        ctxt . LegacyThrow(EE_OBJECT_SETNOPROP);
        return;
    }
    
    if (t_info != nil)
	{
        MCValueRetain(p_value);
		MCNativeControlPtr t_control;
		t_control . control = t_native_control;

		MCExecValue t_value;   
		MCExecValueTraits<MCValueRef>::set(t_value, p_value);

        MCExecStoreProperty(ctxt, t_info, &t_control, t_value);
    }		
}

static MCNativeControlActionInfo *lookup_control_action(const MCNativeControlActionTable *p_table, MCNativeControlAction p_which)
{
	for(uindex_t i = 0; i < p_table -> size; i++)
		if (p_table -> table[i] . action == p_which)
			return &p_table -> table[i];
	
	if (p_table -> parent != nil)
		return lookup_control_action(p_table -> parent, p_which);
	
	return nil;
}

void _MCNativeControlDo(MCExecContext &ctxt, MCNativeControlPtr *p_control, MCNativeControlActionInfo *p_info, MCValueRef *p_arguments, uindex_t p_argument_count)
{
	switch (p_info -> signature)
	{
		// no params
		case kMCNativeControlActionSignature_Void:
		{
			((void(*)(MCExecContext&, MCNativeControlPtr*))p_info -> exec_method)(ctxt, p_control);
			return;
		}
		
		// single string param
		case kMCNativeControlActionSignature_String:
		if (p_argument_count == 1)
		{
			// SN-2014-11-20: [[ Bug 14062 ]] Convert the argument (that could for example be a NameRef
			MCAutoStringRef t_string;
			if (!ctxt . ConvertToString(p_arguments[0], &t_string))
			break;
			
			((void(*)(MCExecContext&, MCNativeControlPtr*, MCStringRef))p_info -> exec_method)(ctxt, p_control, *t_string);
			return;
		}
		break;
            
        // optional integer param
		case kMCNativeControlActionSignature_OptInteger:
        {
            integer_t *t_value_ptr;
            integer_t t_value;
            if (p_argument_count == 1)
            {
                if (!ctxt . ConvertToInteger(p_arguments[0], t_value))
                    break;
                t_value_ptr = &t_value;
            }
            else
                t_value_ptr = nil;
                
            ((void(*)(MCExecContext&, MCNativeControlPtr*, integer_t *))p_info -> exec_method)(ctxt, p_control, t_value_ptr);
            return;
        }
        break;
            
		
		// double string param
		case kMCNativeControlActionSignature_String_String:
		if (p_argument_count == 2)
		{
			MCAutoStringRef t_url;
			MCAutoStringRef t_text;
			
			// SN-2014-11-20: [[ Bug 14062 ]] Convert the arguments (that could for example be a NameRef)
			if (!ctxt . ConvertToString(p_arguments[0], &t_url)
				|| !ctxt . ConvertToString(p_arguments[1], &t_text))
			break;
			
			((void(*)(MCExecContext&, MCNativeControlPtr*, MCStringRef, MCStringRef))p_info -> exec_method)(ctxt, p_control, *t_url, *t_text);
			return;
		}
		break;
		
		// double integer param
		case kMCNativeControlActionSignature_Integer_Integer:
		if (p_argument_count == 2)
		{
			integer_t t_start;
			integer_t t_end;
			// SN-2014-11-20: [[ Bug 14062 ]] Convert the argument (that could for example be a NameRef)
			if (!ctxt . ConvertToInteger(p_arguments[0], t_start)
				|| !ctxt . ConvertToInteger(p_arguments[1], t_end))
			break;
			
			((void(*)(MCExecContext&, MCNativeControlPtr*, integer_t, integer_t))p_info -> exec_method)(ctxt, p_control, t_start, t_end);
			return;
		}
		break;
		//other
		case kMCNativeControlActionSignature_Integer_OptInteger_OptInteger:
		{
			integer_t t_time;
			// SN-2014-11-20: [[ Bug 14062 ]] Convert the argument (that could for example be a NameRef)
			if (ctxt . ConvertToInteger(p_arguments[0], t_time))
			{
				if (p_argument_count == 1)
				{
					((void(*)(MCExecContext&, MCNativeControlPtr*, integer_t, integer_t*, integer_t*))p_info -> exec_method)(ctxt, p_control, t_time, nil, nil);
					return;
				}
				else if (p_argument_count == 3)
				{
					integer_t t_max_width;
					integer_t t_max_height;
					// SN-2014-11-20: [[ Bug 14062 ]] Convert the arguments (that could for example be a NameRef)
					if (ctxt . ConvertToInteger(p_arguments[1], t_max_width) && ctxt . ConvertToInteger(p_arguments[2], t_max_height))
					{
						((void(*)(MCExecContext&, MCNativeControlPtr*, integer_t, integer_t*, integer_t*))p_info -> exec_method)(ctxt, p_control, t_time, &t_max_width, &t_max_height);
						return;
					}
				}
			}
		}
		break;

        default:
            if (MCPerformNativeControlAction((intenum_t)p_info -> action, p_control, p_arguments, p_argument_count))
                return;
            break;
	}
	ctxt . Throw();
}

#ifdef TARGET_SUBPLATFORM_IPHONE
struct handle_context_t
{
	MCExecContext *ctxt;
	MCNativeControlPtr *control;
	MCNativeControlActionInfo *info;
	MCValueRef *arguments;
	uindex_t argument_count;
};

static void invoke_platform(void *p_context)
{
	handle_context_t *ctxt;
	ctxt = (handle_context_t *)p_context;
	
	_MCNativeControlDo(*(ctxt->ctxt), ctxt->control, ctxt->info, ctxt->arguments, ctxt->argument_count);
}

extern void MCIPhoneCallOnMainFiber(void (*)(void *), void *);

void MCNativeControlExecDo(MCExecContext& ctxt, MCStringRef p_control_name, MCStringRef p_action_name, MCValueRef *p_arguments, uindex_t p_argument_count)
{
	MCNativeControl *t_native_control;
	if (!MCNativeControl::FindByNameOrId(p_control_name, t_native_control))
		return;
	
	MCNativeControlAction t_action;
    if (!MCLookupNativeControlAction(p_action_name, (intenum_t&)t_action))
		return;
	
	MCNativeControlActionInfo *t_info;
	t_info = lookup_control_action(t_native_control -> getactiontable(), t_action);
	if (t_info != nil)
	{
		MCNativeControlPtr t_control;
		t_control . control = t_native_control;
		
		// MW-2012-07-31: [[ Fibers ]] If the method doesn't need script / wait, then
		//   jump to the main fiber for it.
		if (!t_info -> waitable)
		{
			handle_context_t t_context;
			t_context.ctxt = &ctxt;
			t_context.control = &t_control;
			t_context.info = t_info;
			t_context.arguments = p_arguments;
			t_context.argument_count = p_argument_count;
			
			MCIPhoneCallOnMainFiber(invoke_platform, &t_context);
			return;
		}
		
		// Execute the method as normal, in this case the method will have to jump
		// to the main fiber to do any system stuff.
		_MCNativeControlDo(ctxt, &t_control, t_info, p_arguments, p_argument_count);
	}
}
#else // Android
void MCNativeControlExecDo(MCExecContext& ctxt, MCStringRef p_control_name, MCStringRef p_action_name, MCValueRef *p_arguments, uindex_t p_argument_count)
{
	MCNativeControl *t_native_control;
	if (!MCNativeControl::FindByNameOrId(p_control_name, t_native_control))
		return;
	
	MCNativeControlAction t_action;
    if (!MCLookupNativeControlAction(p_action_name, (intenum_t&)t_action))
		return;
	
	MCNativeControlActionInfo *t_info;
	t_info = lookup_control_action(t_native_control -> getactiontable(), t_action);
	if (t_info != nil)
	{
		MCNativeControlPtr t_control;
		t_control . control = t_native_control;
		
		_MCNativeControlDo(ctxt, &t_control, t_info, p_arguments, p_argument_count);
	}
}
#endif
