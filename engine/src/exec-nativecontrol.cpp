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

static void MCNativeControlColorParse(MCExecContext& ctxt, MCStringRef p_input, MCNativeControlColor& r_output)
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

static void MCNativeControlColorFormat(MCExecContext& ctxt, const MCNativeControlColor& p_input, MCStringRef& r_output)
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

static void MCNativeControlColorFree(MCExecContext& ctxt, MCNativeControlColor& p_input)
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

void MCNativeControlDecelerationRateFree(MCExecContext& ctxt, MCNativeControlColor& p_input)
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

void MCNativeControlIndicatorInsetsFree(MCExecContext& ctxt, MCNativeControlColor& p_input)
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

////////////////////////////////////////////////////////////////////////////////

MCExecCustomTypeInfo *kMCNativeControlColorTypeInfo = &_kMCNativeControlColorTypeInfo;
MCExecCustomTypeInfo *kMCNativeControlDecelerationRateTypeInfo = &_kMCNativeControlDecelerationRateTypeInfo;
MCExecCustomTypeInfo *kMCNativeControlIndicatorInsetsTypeInfor = &_kMCNativeControlIndicatorInsetsTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlIndicatorStyleTypeInfo = &_kMCNativeControlIndicatorStyleTypeInfo;
MCExecEnumTypeInfo *kMCNativeControlPlaybackStateTypeInfo = &_kMCNativeControlPlaybackStateTypeInfo;
MCExecSetTypeInfo *kMCNativeControlLoadStateTypeInfo = &_kMCNativeControlLoadStateTypeInfo;

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
    
    if (t_control != nil)
        t_control -> Delete();
}

void MCNativeControlExecDeleteControl(MCExecContext& ctxt, MCStringRef p_control_name)
{
    MCNativeControl *t_control;
    if (!MCNativeControl::FindByNameOrId(MCStringGetCString(p_control_name), t_control))
        return;
    
    t_control -> Delete();
    t_control -> Release();
}

void MCNativeControlExecSetProperty(MCExecContext& ctxt, MCStringRef p_property, MCValueRef p_value)
{
    ctxt . Unimplemented();
}

void MCNativeControlExecGetProperty(MCExecContext& ctxt, MCStringRef p_property, MCValueRef& r_value)
{
    ctxt . Unimplemented();
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
