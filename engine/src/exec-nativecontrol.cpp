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

static MCExecEnumTypeInfo _kMCNativeControlInputAutocorrectionTypeTypeInfo =
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
        if (MCNativeControl::FindByNameOrId(MCStringGetCString(p_control_name), t_control))
            return;
    }
    
    MCNativeControlType t_type;
    if (!MCNativeControl::LookupType(MCStringGetCString(p_type_name), t_type))
        return;
    
    MCNativeControl *t_new_control;
    t_new_control = nil;
    if (MCNativeControl::CreateWithType(t_type, t_new_control))
    {
        extern MCExecPoint *MCEPptr;
        t_new_control -> SetOwner(MCEPptr -> getobj());
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
    if (!MCNativeControl::FindByNameOrId(MCStringGetCString(p_control_name), t_control))
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
		if (t_target -> GetName() != nil)
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
    }
}

void MCNativeControlGetControlList(MCExecContext& ctxt, MCStringRef& r_list)
{
    if (MCNativeControl::GetControlList(r_list))
        return;
    
    ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

static MCNativeControlPropertyInfo *lookup_control_property(const MCNativeControlPropertyTable *p_table, MCNativeControlProperty p_which)
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
    if (!MCNativeControl::FindByNameOrId(MCStringGetCString(p_control_name), t_native_control))
        return;
    
    MCNativeControlProperty t_property;
    if (!MCNativeControl::LookupProperty(MCStringGetCString(p_property_name), t_property))
        return;

    MCNativeControlPropertyInfo *t_info;
    t_info = lookup_control_property(t_native_control -> getpropertytable(), t_property);
    
    if (t_info != nil && t_info -> getter == nil)
    {
        MCeerror -> add(EE_OBJECT_GETNOPROP, 0, 0);
        return;
    }
    
    MCExecPoint& ep = ctxt . GetEP();
    
    if (t_info != nil)
    {
        MCNativeControlPtr t_control;
        t_control . control = t_native_control;
        
        switch(t_info -> type)
        {
            case kMCNativeControlPropertyTypeAny:
            {
                MCAutoValueRef t_any;
                ((void(*)(MCExecContext&, MCNativeControlPtr, MCValueRef&))t_info -> getter)(ctxt, t_control, &t_any);
                if (!ctxt . HasError())
                    ep . setvalueref(*t_any);
            }
                break;
                
            case kMCNativeControlPropertyTypeBool:
            {
                bool t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, bool&))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                    ep . setboolean(t_value ? True : False);
            }
                break;
                
            case kMCNativeControlPropertyTypeInt16:
            case kMCNativeControlPropertyTypeInt32:
            {
                integer_t t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, integer_t&))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                    ep . setint(t_value);
            }
                break;
                
            case kMCNativeControlPropertyTypeUInt16:
            case kMCNativeControlPropertyTypeUInt32:
            {
                uinteger_t t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, uinteger_t&))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                    ep . setuint(t_value);
            }
                break;
                
            case kMCNativeControlPropertyTypeDouble:
            {
                double t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, double&))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                    ep . setnvalue(t_value);
            }
                break;
                
            case kMCNativeControlPropertyTypeString:
            case kMCNativeControlPropertyTypeBinaryString:
            {
                MCAutoStringRef t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, MCStringRef&))t_info -> getter)(ctxt, t_control, &t_value);
                if (!ctxt . HasError())
                    ep . setvalueref(*t_value);
            }
                break;
                
            case kMCNativeControlPropertyTypeRectangle:
            {
                MCRectangle t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, MCRectangle&))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                    ep . setrectangle(t_value);
            }
                break;
                
            case kMCNativeControlPropertyTypePoint:
            {
                MCPoint t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, MCPoint&))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                    ep . setpoint(t_value);
            }
                break;
                
            case kMCNativeControlPropertyTypeInt16X2:
            case kMCNativeControlPropertyTypeInt32X2:
            {
                integer_t t_value[2];
                ((void(*)(MCExecContext&, MCNativeControlPtr, integer_t[2]))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                    ep . setstringf("%d,%d", t_value[0], t_value[1]);
            }
                break;
                
            case kMCNativeControlPropertyTypeInt16X4:
            case kMCNativeControlPropertyTypeInt32X4:
            {
                integer_t t_value[4];
                ((void(*)(MCExecContext&, MCNativeControlPtr, integer_t[4]))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                    ep . setstringf("%d,%d,%d,%d", t_value[0], t_value[1], t_value[2], t_value[3]);
            }
                break;

            case kMCNativeControlPropertyTypeUInt32X4:
            {
                uinteger_t t_value[4];
                ((void(*)(MCExecContext&, MCNativeControlPtr, uinteger_t[4]))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                    ep . setstringf("%d,%d,%d,%d", t_value[0], t_value[1], t_value[2], t_value[3]);
            }
                break;

            case kMCNativeControlPropertyTypeEnum:
            {
                int t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, int&))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                {
                    MCExecEnumTypeInfo *t_enum_info;
                    t_enum_info = (MCExecEnumTypeInfo *)(t_info -> type_info);
                    for(uindex_t i = 0; i < t_enum_info -> count; i++)
                        if (t_enum_info -> elements[i] . value == t_value)
                        {
                            ep . setcstring(t_enum_info -> elements[i] . tag);
                            break;
                        }
                    
                    // THIS MEANS A METHOD HAS RETURNED AN ILLEGAL VALUE
                    MCAssert(false);
                    return;
                }
            }
                break;
                
            case kMCNativeControlPropertyTypeOptionalEnum:
            {
                int t_value;
                int *t_value_ptr;
                t_value_ptr = &t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, int*&))t_info -> getter)(ctxt, t_control, t_value_ptr);
                if (!ctxt . HasError())
                {
                    if (t_value_ptr == nil)
                        ep . clear();
                    else
                    {
                        MCExecEnumTypeInfo *t_enum_info;
                        t_enum_info = (MCExecEnumTypeInfo *)(t_info -> type_info);
                        for(uindex_t i = 0; i < t_enum_info -> count; i++)
                            if (t_enum_info -> elements[i] . value == t_value)
                            {
                                ep . setcstring(t_enum_info -> elements[i] . tag);
                                break;
                            }
                        
                        // THIS MEANS A METHOD HAS RETURNED AN ILLEGAL VALUE
                        MCAssert(false);
                        return;
                    }
                    return;
                }
            }
                break;
                
            case kMCNativeControlPropertyTypeSet:
            {
                unsigned int t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, unsigned int&))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                {
                    MCExecSetTypeInfo *t_set_info;
                    t_set_info = (MCExecSetTypeInfo *)(t_info -> type_info);
                    
                    bool t_first;
                    t_first = true;
                    
                    ep . clear();
                    for(uindex_t i = 0; i < t_set_info -> count; i++)
                        if (((1 << t_set_info -> elements[i] . bit) & t_value) != 0)
                        {
                            ep . concatcstring(t_set_info -> elements[i] . tag, EC_COMMA, t_first);
                            t_first = false;
                        }
                }
            }
                break;
                
            case kMCNativeControlPropertyTypeCustom:
            {
                MCExecCustomTypeInfo *t_custom_info;
                t_custom_info = (MCExecCustomTypeInfo *)(t_info -> type_info);
                
                MCAssert(t_custom_info -> size <= 64);
                
                char t_value[64];
                ((void(*)(MCExecContext&, MCNativeControlPtr, void *))t_info -> getter)(ctxt, t_control, t_value);
                if (!ctxt . HasError())
                {
                    MCAutoStringRef t_value_ref;
                    ((MCExecCustomTypeFormatProc)t_custom_info -> format)(ctxt, t_value, &t_value_ref);
                    ((MCExecCustomTypeFreeProc)t_custom_info -> free)(ctxt, t_value);
                    if (!ctxt . HasError())
                        ep . setvalueref(*t_value_ref);
                }
                
            }
                break;
                
            case kMCNativeControlPropertyTypeOptionalInt16:
            {
                integer_t t_value;
                integer_t *t_value_ptr;
                t_value_ptr = &t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, integer_t*&))t_info -> getter)(ctxt, t_control, t_value_ptr);
                if (!ctxt . HasError())
                {
                    if (t_value_ptr == nil)
                        ep . clear();
                    else
                        ep . setint(t_value);
                }
            }
                break;
                
            case kMCNativeControlPropertyTypeOptionalUInt16:
            case kMCNativeControlPropertyTypeOptionalUInt32:
            {
                uinteger_t t_value;
                uinteger_t *t_value_ptr;
                t_value_ptr = &t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, uinteger_t*&))t_info -> getter)(ctxt, t_control, t_value_ptr);
                if (!ctxt . HasError())
                {
                    if (t_value_ptr == nil)
                        ep . clear();
                    else
                        ep . setint(t_value);
                }
            }
                break;
                
            case kMCNativeControlPropertyTypeOptionalString:
            {
                MCAutoStringRef t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, MCStringRef&))t_info -> getter)(ctxt, t_control, &t_value);
                if (!ctxt . HasError())
                {
                    if (*t_value == nil)
                        ep . clear();
                    else
                        ep . setvalueref(*t_value);
                }
                
            }
                break;
                
            case kMCNativeControlPropertyTypeOptionalRectangle:
            {
                MCRectangle t_value;
                MCRectangle *t_value_ptr;
                t_value_ptr = &t_value;
                ((void(*)(MCExecContext&, MCNativeControlPtr, MCRectangle*&))t_info -> getter)(ctxt, t_control, t_value_ptr);
                if (!ctxt . HasError())
                {
                    if (t_value_ptr == nil)
                        ep . clear();
                    else
                        ep . setrectangle(t_value);
                }
            }
                break;
                
        }
    }
    
    if (!ctxt . HasError())
        r_result = MCValueRetain(ep . getvalueref());
}

void MCNativeControlExecSet(MCExecContext& ctxt, MCStringRef p_control_name, MCStringRef p_property_name, MCValueRef p_value)
{
    MCNativeControl *t_native_control;
    if (!MCNativeControl::FindByNameOrId(MCStringGetCString(p_control_name), t_native_control))
        return;
    
    MCNativeControlProperty t_property;
    if (!MCNativeControl::LookupProperty(MCStringGetCString(p_property_name), t_property))
        return;
    
    MCNativeControlPropertyInfo *t_info;
    t_info = lookup_control_property(t_native_control -> getpropertytable(), t_property);
    
    if (t_info != nil && t_info -> setter == nil)
    {
        MCeerror -> add(EE_OBJECT_SETNOPROP, 0, 0);
        return;
    }
    
    MCExecPoint& ep = ctxt . GetEP();
    /* UNCHECKED */ ep . setvalueref(p_value);
    
    if (t_info != nil)
	{
		MCExecContext ctxt(ep);
		
		MCNativeControlPtr t_control;
		t_control . control = t_native_control;
		
		switch(t_info -> type)
		{
			case kMCNativeControlPropertyTypeAny:
			{
				MCAutoValueRef t_value;
				/* UNCHECKED */ ep . copyasvalueref(&t_value);
				((void(*)(MCExecContext&, MCNativeControlPtr, MCValueRef))t_info -> setter)(ctxt, t_control, *t_value);
			}
                break;
				
			case kMCNativeControlPropertyTypeBool:
			{
				bool t_value;
				if (!ep . copyasbool(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAB);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, bool))t_info -> setter)(ctxt, t_control, t_value);
			}
                break;
                
			case kMCNativeControlPropertyTypeInt16:
			{
				integer_t t_value;
				if (!ep . copyasint(t_value) ||
					t_value < -32768 || t_value > 32767)
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, integer_t))t_info -> setter)(ctxt, t_control, t_value);
			}
                break;
				
			case kMCNativeControlPropertyTypeInt32:
			{
				integer_t t_value;
				if (!ep . copyasint(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, integer_t))t_info -> setter)(ctxt, t_control, t_value);
			}
                break;
                
			case kMCNativeControlPropertyTypeUInt16:
			{
				uinteger_t t_value;
				if (!ep . copyasuint(t_value) ||
					t_value < 0 || t_value > 65535)
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, uinteger_t))t_info -> setter)(ctxt, t_control, t_value);
			}
                break;
				
			case kMCNativeControlPropertyTypeUInt32:
			{
				uinteger_t t_value;
				if (!ep . copyasuint(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, uinteger_t))t_info -> setter)(ctxt, t_control, t_value);
			}
                break;
				
			case kMCNativeControlPropertyTypeDouble:
			{
				double t_value;
				if (!ep . copyasdouble(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, double))t_info -> setter)(ctxt, t_control, t_value);
			}
                break;
				
			case kMCNativeControlPropertyTypeString:
			case kMCNativeControlPropertyTypeBinaryString:
			{
				MCAutoStringRef t_value;
				if (!ep . copyasstringref(&t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAC);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, MCStringRef))t_info -> setter)(ctxt, t_control, *t_value);
			}
                break;
                
			case kMCNativeControlPropertyTypeRectangle:
			{
				MCRectangle t_value;
				if (!ep . copyaslegacyrectangle(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NOTARECT);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, MCRectangle))t_info -> setter)(ctxt, t_control, t_value);
			}
                break;
				
			case kMCNativeControlPropertyTypePoint:
			{
				MCPoint t_value;
				if (!ep . copyaslegacypoint(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NOTAPOINT);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, MCPoint))t_info -> setter)(ctxt, t_control, t_value);
			}
                break;
                
			case kMCNativeControlPropertyTypeInt16X2:
            case kMCNativeControlPropertyTypeInt32X2:
			{
				int2 a, b;
				if (!MCU_stoi2x2(ep . getsvalue(), a, b))
					ctxt . LegacyThrow(EE_PROPERTY_NOTAINTPAIR);
				if (!ctxt . HasError())
				{
					integer_t t_value[2];
					t_value[0] = a;
					t_value[1] = b;
					((void(*)(MCExecContext&, MCNativeControlPtr, integer_t[2]))t_info -> setter)(ctxt, t_control, t_value);
				}
			}
                break;
                
			case kMCNativeControlPropertyTypeInt16X4:
            case kMCNativeControlPropertyTypeInt32X4:
			{
				int2 a, b, c, d;
				if (!MCU_stoi2x4(ep . getsvalue(), a, b, c, d))
					ctxt . LegacyThrow(EE_PROPERTY_NOTAINTQUAD);
				if (!ctxt . HasError())
				{
					integer_t t_value[4];
					t_value[0] = a;
					t_value[1] = b;
					t_value[2] = c;
					t_value[3] = d;
					((void(*)(MCExecContext&, MCNativeControlPtr, integer_t[4]))t_info -> setter)(ctxt, t_control, t_value);
				}
			}
                break;
                
            case kMCNativeControlPropertyTypeUInt32X4:
			{
				int2 a, b, c, d;
				if (!MCU_stoi2x4(ep . getsvalue(), a, b, c, d))
					ctxt . LegacyThrow(EE_PROPERTY_NOTAINTQUAD);
				if (!ctxt . HasError())
				{
					uinteger_t t_value[4];
					t_value[0] = a;
					t_value[1] = b;
					t_value[2] = c;
					t_value[3] = d;
					((void(*)(MCExecContext&, MCNativeControlPtr, uinteger_t[4]))t_info -> setter)(ctxt, t_control, t_value);
				}
			}
                break;
                
			case kMCNativeControlPropertyTypeEnum:
			{
				MCExecEnumTypeInfo *t_enum_info;
				t_enum_info = (MCExecEnumTypeInfo *)t_info -> type_info;
				
				bool t_found;
				t_found = false;
				intenum_t t_value;
				for(uindex_t i = 0; i < t_enum_info -> count; i++)
					if (!t_enum_info -> elements[i] . read_only &&
						MCU_strcasecmp(ep . getcstring(), t_enum_info -> elements[i] . tag) == 0)
					{
						t_found = true;
						t_value = t_enum_info -> elements[i] . value;
					}
                
				if (!t_found)
					ctxt . LegacyThrow(EE_PROPERTY_BADENUMVALUE);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, int))t_info -> setter)(ctxt, t_control, t_value);
			}
                break;
                
			case kMCNativeControlPropertyTypeOptionalEnum:
			{
				MCExecEnumTypeInfo *t_enum_info;
				t_enum_info = (MCExecEnumTypeInfo *)t_info -> type_info;
				
				intenum_t t_value;
				intenum_t* t_value_ptr;
				if (ep . isempty())
					t_value_ptr = nil;
				else
				{
					t_value_ptr = &t_value;
					bool t_found;
					t_found = false;
					for(uindex_t i = 0; i < t_enum_info -> count; i++)
						if (!t_enum_info -> elements[i] . read_only &&
							MCU_strcasecmp(ep . getcstring(), t_enum_info -> elements[i] . tag) == 0)
						{
							t_found = true;
							t_value = t_enum_info -> elements[i] . value;
						}
                    
					if (!t_found)
						ctxt . LegacyThrow(EE_PROPERTY_BADENUMVALUE);
				}
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, int*))t_info -> setter)(ctxt, t_control, t_value_ptr);
			}
                break;
                
			case kMCNativeControlPropertyTypeSet:
			{
				MCExecSetTypeInfo *t_set_info;
				t_set_info = (MCExecSetTypeInfo *)(t_info -> type_info);
				
				intset_t t_value = 0;
				char **t_elements;
				uindex_t t_element_count;
				MCCStringSplit(ep . getcstring(), ',', t_elements, t_element_count);
                
				for (uindex_t i = 0; i < t_element_count; i++)
				{
					for (uindex_t j = 0; j < t_set_info -> count; j++)
					{
						if (MCU_strcasecmp(t_elements[i], t_set_info -> elements[j] . tag) == 0)
						{
							t_value |= 1 << t_set_info -> elements[j] . bit;
							break;
						}
					}
				}
                
				MCCStringArrayFree(t_elements, t_element_count);
				((void(*)(MCExecContext&, MCNativeControlPtr, unsigned int))t_info -> setter)(ctxt, t_control, t_value);
			}
                break;
				
			case kMCNativeControlPropertyTypeCustom:
			{
				MCExecCustomTypeInfo *t_custom_info;
				t_custom_info = (MCExecCustomTypeInfo *)(t_info -> type_info);
				
				MCAssert(t_custom_info -> size <= 64);
                
				MCAutoStringRef t_input_value;
				/* UNCHECKED */ ep . copyasstringref(&t_input_value);
				
				char t_value[64];
				((MCExecCustomTypeParseProc)t_custom_info -> parse)(ctxt, *t_input_value, t_value);
				if (!ctxt . HasError())
				{
					((void(*)(MCExecContext&, MCNativeControlPtr, void *))t_info -> setter)(ctxt, t_control, t_value);
					((MCExecCustomTypeFreeProc)t_custom_info -> free)(ctxt, t_value);
				}
			}
                break;
                
			case kMCNativeControlPropertyTypeOptionalInt16:
			{
				integer_t t_value;
				integer_t *t_value_ptr;
				if (ep . isempty())
					t_value_ptr = nil;
				else
				{
					t_value_ptr = &t_value;
					if (!ep . copyasint(t_value) ||
						t_value < -32768 || t_value > 32767)
						ctxt . LegacyThrow(EE_PROPERTY_NAN);
				}
				
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, integer_t*))t_info -> setter)(ctxt, t_control, t_value_ptr);
			}
                break;
                
			case kMCNativeControlPropertyTypeOptionalUInt16:
			{
				uinteger_t t_value;
				uinteger_t *t_value_ptr;
				if (ep . isempty())
					t_value_ptr = nil;
				else
				{
					t_value_ptr = &t_value;
					if (!ep . copyasuint(t_value) ||
						t_value < 0 || t_value > 65535)
						ctxt . LegacyThrow(EE_PROPERTY_NAN);
				}
				
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, uinteger_t*))t_info -> setter)(ctxt, t_control, t_value_ptr);
			}
                break;
                
			case kMCNativeControlPropertyTypeOptionalUInt32:
			{
				uinteger_t t_value;
				uinteger_t *t_value_ptr;
				if (ep . isempty())
					t_value_ptr = nil;
				else
				{
					t_value_ptr = &t_value;
					if (!ep . copyasuint(t_value))
						ctxt . LegacyThrow(EE_PROPERTY_NAN);
				}
				
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, uinteger_t*))t_info -> setter)(ctxt, t_control, t_value_ptr);
			}
                break;
                
			case kMCNativeControlPropertyTypeOptionalString:
			{
				MCAutoStringRef t_value;
				if (!ep . isempty())
				{
					if (!ep . copyasstringref(&t_value))
						ctxt . LegacyThrow(EE_PROPERTY_NAS);
				}
				
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, MCStringRef))t_info -> setter)(ctxt, t_control, *t_value);
			}
                break;
                
			case kMCNativeControlPropertyTypeOptionalRectangle:
			{
				MCRectangle t_value;
				MCRectangle *t_value_ptr;
				if (ep . isempty())
					t_value_ptr = nil;
				else
				{
					t_value_ptr = &t_value;
					if (!ep . copyaslegacyrectangle(t_value))
						ctxt . LegacyThrow(EE_PROPERTY_NOTARECT);
				}
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCNativeControlPtr, MCRectangle*))t_info -> setter)(ctxt, t_control, t_value_ptr);	
			}
                break;	
                
			default:
				ctxt . Unimplemented();
				break;
		}
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

void MCNativeControlExecDo(MCExecContext& ctxt, MCStringRef p_control_name, MCStringRef p_action_name, MCValueRef *p_arguments, uindex_t p_argument_count)
{
	MCNativeControl *t_native_control;
    if (!MCNativeControl::FindByNameOrId(MCStringGetCString(p_control_name), t_native_control))
        return;

	MCNativeControlAction t_action;
    if (!MCNativeControl::LookupAction(MCStringGetCString(p_action_name), t_action))
        return;
    
    MCNativeControlActionInfo *t_info;
    t_info = lookup_control_action(t_native_control -> getactiontable(), t_action);
    if (t_info != nil)
    {
        MCNativeControlPtr t_control;
        t_control . control = t_native_control;
        
        switch (t_info -> action)
        {
            // no params
            case kMCNativeControlActionAdvance:
            case kMCNativeControlActionRetreat:
            case kMCNativeControlActionReload:
            case kMCNativeControlActionStop:
            case kMCNativeControlActionFlashScrollIndicators:
            case kMCNativeControlActionPlay:
            case kMCNativeControlActionPause:
            case kMCNativeControlActionPrepareToPlay:
            case kMCNativeControlActionBeginSeekingForward:
            case kMCNativeControlActionBeginSeekingBackward:
            case kMCNativeControlActionEndSeeking:
            case kMCNativeControlActionFocus:
            {
                ((void(*)(MCExecContext&, MCNativeControlPtr))t_info -> exec_method)(ctxt, t_control);
                return;
            }
            
            // single string param
            case kMCNativeControlActionExecute:
                if (p_argument_count == 1)
                {
                    MCAutoStringRef t_string;
                    &t_string = MCValueRetain((MCStringRef)p_arguments[0]);
                    ((void(*)(MCExecContext&, MCNativeControlPtr, MCStringRef))t_info -> exec_method)(ctxt, t_control, *t_string);
                    return;
                }
                break;
                
            // double string param
            case kMCNativeControlActionLoad:
                if (p_argument_count == 2)
                {
                    MCAutoStringRef t_url;
                    MCAutoStringRef t_text;
                    &t_url = MCValueRetain((MCStringRef)p_arguments[0]);
                    &t_text = MCValueRetain((MCStringRef)p_arguments[1]);
                    ((void(*)(MCExecContext&, MCNativeControlPtr, MCStringRef, MCStringRef))t_info -> exec_method)(ctxt, t_control, *t_url, *t_text);
                    return;
                }
                break;
            
            // double integer param
            case kMCNativeControlActionScrollRangeToVisible:
                if (p_argument_count == 2)
                {
                    integer_t t_start;
                    integer_t t_end;
                    if (MCU_stoi4((MCStringRef)p_arguments[0], t_start) && MCU_stoi4((MCStringRef)p_arguments[1], t_end))
                    {
                        ((void(*)(MCExecContext&, MCNativeControlPtr, integer_t, integer_t))t_info -> exec_method)(ctxt, t_control, t_start, t_end);
                        return;
                    }
                }
                    break;
            //other
            case kMCNativeControlActionSnapshot:
            case kMCNativeControlActionSnapshotExactly:
            {
                integer_t t_time;
                if (MCU_stoi4((MCStringRef)p_arguments[0], t_time))
                {
                    if (p_argument_count == 1)
                    {
                        ((void(*)(MCExecContext&, MCNativeControlPtr, integer_t, integer_t*, integer_t*))t_info -> exec_method)(ctxt, t_control, t_time, nil, nil);
                        return;
                    }
                    else if (p_argument_count == 3)
                    {
                        integer_t t_max_width;
                        integer_t t_max_height;
                        if (MCU_stoi4((MCStringRef)p_arguments[1], t_max_width) && MCU_stoi4((MCStringRef)p_arguments[2], t_max_height))
                        {
                            ((void(*)(MCExecContext&, MCNativeControlPtr, integer_t, integer_t*, integer_t*))t_info -> exec_method)(ctxt, t_control, t_time, &t_max_width, &t_max_height);
                            return;
                        }
                    }
                }
            }
                break;
    
            case kMCNativeControlActionUnknown:
            default:
                break;
        }
        ctxt . Throw();
    }
}