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


#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "mcerror.h"
#include "globals.h"
#include "objectstream.h"
#include "variable.h"

#include "path.h"

#include "meta.h"

#include "gradient.h"
#include "packed.h"

typedef struct _GradientPropList
{
	const char *token;
	MCGradientFillProperty value;
}
GradientPropList;

static GradientPropList gradientprops[] =
{
	{"type", P_GRADIENT_FILL_TYPE},
	{"ramp", P_GRADIENT_FILL_RAMP},
	{"from", P_GRADIENT_FILL_ORIGIN},
	{"to", P_GRADIENT_FILL_PRIMARY_POINT},
	{"via", P_GRADIENT_FILL_SECONDARY_POINT},
	{"quality", P_GRADIENT_FILL_QUALITY},
	{"mirror", P_GRADIENT_FILL_MIRROR},
	{"repeat", P_GRADIENT_FILL_REPEAT},
	{"wrap", P_GRADIENT_FILL_WRAP},
};

static Exec_stat MCGradientFillLookupProperty(MCNameRef p_token, MCGradientFillProperty& r_prop)
{
	uint4 tablesize = ELEMENTS(gradientprops);
	while (tablesize--)
	{
		if (MCStringIsEqualToCString(MCNameGetString(p_token), gradientprops[tablesize].token, kMCCompareCaseless))
		{
			r_prop = gradientprops[tablesize].value;
			return ES_NORMAL;
		}
	}
	
	MCeerror->add(EE_GRAPHIC_BADGRADIENTKEY, 0, 0, p_token);
	return ES_ERROR;
}

void MCGradientFillInit(MCGradientFill *&r_gradient, MCRectangle p_rect)
{
	r_gradient = new (nothrow) MCGradientFill;
	r_gradient->kind = kMCGradientKindLinear;
	r_gradient->quality = kMCGradientQualityNormal;
	r_gradient->mirror = false;
	r_gradient->wrap = false;
	r_gradient->repeat = 1;
	r_gradient->ramp = new (nothrow) MCGradientFillStop[2];
	r_gradient->ramp[0].offset = 0;
	r_gradient->ramp[0].color = MCGPixelPack(kMCGPixelFormatBGRA, 0, 0, 0, 255); // black
	r_gradient->ramp[0].hw_color = MCGPixelPackNative(0, 0, 0, 255);
	r_gradient->ramp[1].offset = STOP_INT_MAX;
	r_gradient->ramp[0].color = MCGPixelPack(kMCGPixelFormatBGRA, 255, 255, 255, 255); // white
	r_gradient->ramp[0].hw_color = MCGPixelPackNative(255, 255, 255, 255);
	r_gradient->ramp[0].difference = STOP_DIFF_MULT / STOP_INT_MAX;
	r_gradient->ramp_length = 2;
	r_gradient->origin.x = p_rect.x + p_rect.width / 2;
	r_gradient->origin.y = p_rect.y + p_rect.height / 2;
	r_gradient->primary.x = p_rect.x + p_rect.width * 7 / 8;
	r_gradient->primary.y = p_rect.y + p_rect.height / 2;
	r_gradient->secondary.x = p_rect.x + p_rect.width / 2;
	r_gradient->secondary.y = p_rect.y + p_rect.height * 7 / 8;
	r_gradient->old_origin.x = MININT2;
	r_gradient->old_origin.y = MININT2;
}

void MCGradientFillFree(MCGradientFill *p_gradient)
{
	delete[] p_gradient -> ramp;
	delete p_gradient;
}

MCGradientFill *MCGradientFillCopy(const MCGradientFill *p_gradient)
{
	if (p_gradient == NULL)
		return NULL;

	MCGradientFill *t_gradient = new (nothrow) MCGradientFill;
	t_gradient->kind = p_gradient->kind;
	
	t_gradient->ramp_length = p_gradient->ramp_length;
	t_gradient->ramp = new (nothrow) MCGradientFillStop[t_gradient->ramp_length];

	memcpy(t_gradient->ramp, p_gradient->ramp, sizeof(MCGradientFillStop)*t_gradient->ramp_length);

	t_gradient->origin = p_gradient->origin;
	t_gradient->primary = p_gradient->primary;
	t_gradient->secondary = p_gradient->secondary;
	t_gradient->old_origin.x = MININT2;
	t_gradient->old_origin.y = MININT2;

	t_gradient->quality = p_gradient->quality;
	t_gradient->mirror = p_gradient->mirror;
	t_gradient->repeat = p_gradient->repeat;
	t_gradient->wrap = p_gradient->wrap;

	return t_gradient;
}

static bool MCGradientFillFetchProperty(MCExecContext& ctxt, MCGradientFill* p_gradient, MCGradientFillProperty p_property, MCExecValue& r_value)
{
	if (p_gradient == nil)
	{
		r_value . stringref_value = MCValueRetain(kMCEmptyString);
        r_value . type = kMCExecValueTypeStringRef;
        return true;
	}
    
	switch (p_property)
	{
        case P_GRADIENT_FILL_TYPE:
        {
            uint1 t_gradient_kind;
            t_gradient_kind = p_gradient->kind;
            
            MCExecFormatEnum(ctxt, kMCInterfaceGradientFillKindTypeInfo, (intenum_t)t_gradient_kind, r_value);
			if (ctxt.HasError())
				return false;
        }
            break;
        case P_GRADIENT_FILL_RAMP:
        {
            MCAutoStringRef t_data;
			if (!MCGradientFillRampUnparse(p_gradient->ramp,p_gradient->ramp_length, &t_data))
				return false;
			
            r_value . stringref_value = MCValueRetain(*t_data);
            r_value . type = kMCExecValueTypeStringRef;
        }
            break;
        case P_GRADIENT_FILL_ORIGIN:
        {
            r_value . point_value . x = p_gradient->origin.x;
            r_value . point_value . y = p_gradient->origin.y;
            r_value . type = kMCExecValueTypePoint;
        }
            break;
        case P_GRADIENT_FILL_PRIMARY_POINT:
        {
            r_value . point_value . x = p_gradient->primary.x;
            r_value . point_value . y = p_gradient->primary.y;
            r_value . type = kMCExecValueTypePoint;
        }
            break;
        case P_GRADIENT_FILL_SECONDARY_POINT:
        {
            r_value . point_value . x = p_gradient->secondary.x;
            r_value . point_value . y = p_gradient->secondary.y;
            r_value . type = kMCExecValueTypePoint;
        }
            break;
        case P_GRADIENT_FILL_QUALITY:
		{
			uint1 t_gradient_quality;
            t_gradient_quality = p_gradient->quality;
                
            MCExecFormatEnum(ctxt, kMCInterfaceGradientFillQualityTypeInfo, (intenum_t)t_gradient_quality, r_value);
			if (ctxt.HasError())
				return false;
		}
            break;
        case P_GRADIENT_FILL_MIRROR:
            r_value . bool_value = p_gradient -> mirror;
            r_value . type = kMCExecValueTypeBool;
            break;
        case P_GRADIENT_FILL_REPEAT:
            r_value . uint_value = p_gradient -> repeat;
            r_value . type = kMCExecValueTypeUInt;
            break;
        case P_GRADIENT_FILL_WRAP:
            r_value . bool_value = p_gradient -> wrap;
            r_value . type = kMCExecValueTypeBool;
            break;
	}
	
	return true;
}

bool MCGradientFillGetElement(MCExecContext& ctxt, MCGradientFill* p_gradient, MCNameRef p_prop, MCExecValue& r_value)
{
    MCGradientFillProperty t_property;
    if (MCGradientFillLookupProperty(p_prop, t_property) != ES_NORMAL)
        return false;
    
    return MCGradientFillFetchProperty(ctxt, p_gradient, t_property, r_value);
}

bool MCGradientFillGetProperties(MCExecContext& ctxt, MCGradientFill* p_gradient, MCExecValue& r_value)
{
	if (p_gradient == nil)
	{
        r_value . arrayref_value = MCValueRetain(kMCEmptyArray);
        r_value . type = kMCExecValueTypeArrayRef;
        return true;
	}
	uint4 tablesize = ELEMENTS(gradientprops);
    
    MCAutoArrayRef v;
    if (!MCArrayCreateMutable(&v))
        return false;
    
    MCerrorlock++;
    
    bool t_success;
    t_success = true;
    
    while (t_success && tablesize--)
    {
        MCValueRef t_prop_value;
        
        MCExecValue t_value;
        t_success = MCGradientFillFetchProperty(ctxt, p_gradient, gradientprops[tablesize].value, t_value);
        if (t_success)
        {
            MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypeValueRef, &t_prop_value);
            t_success = !ctxt . HasError();
        }
        if (t_success)
            t_success = MCArrayStoreValue(*v, false, MCNAME(gradientprops[tablesize].token), t_prop_value);
    }
    
    MCerrorlock--;
    
    if (t_success)
    {
        r_value . arrayref_value = MCValueRetain(*v);
        r_value . type = kMCExecValueTypeArrayRef;
        return true;
    }
    
    return false;
}

static void MCGradientFillSetEnumProperty(MCExecContext& ctxt, MCGradientFill*& p_gradient, MCGradientFillProperty which, MCExecValue p_value, bool& x_dirty)
{
    intenum_t t_value;
    switch (which)
    {
        case P_GRADIENT_FILL_TYPE:
        {
            MCExecParseEnum(ctxt, kMCInterfaceGradientFillKindTypeInfo, p_value, t_value);
            if (!ctxt . HasError())
            {
                MCGradientFillKind t_new_gradient_kind;
                t_new_gradient_kind = (MCGradientFillKind)t_value;
                if (t_new_gradient_kind != p_gradient->kind)
                {
                    p_gradient->kind = t_new_gradient_kind;
                    x_dirty = true;
                }
            }
        }
            break;
            
        case P_GRADIENT_FILL_QUALITY:
        {
            MCExecParseEnum(ctxt, kMCInterfaceGradientFillQualityTypeInfo, p_value, t_value);
            if (!ctxt . HasError())
            {
                MCGradientFillQuality t_new_gradient_quality;
                t_new_gradient_quality = (MCGradientFillQuality)t_value;
                if (t_new_gradient_quality != p_gradient->quality)
                {
                    p_gradient->quality = t_new_gradient_quality;
                    x_dirty = true;
                }
            }
        }
            break;
        default:
            break;
    }
}

static void MCGradientFillSetPointProperty(MCGradientFill*& p_gradient, MCGradientFillProperty which, MCPoint p_value, bool& x_dirty)
{
    switch (which)
    {
        case P_GRADIENT_FILL_ORIGIN:
            p_gradient->origin.x = p_value . x;
            p_gradient->origin.y = p_value . y;
            x_dirty = true;
            break;
        case P_GRADIENT_FILL_PRIMARY_POINT:
            p_gradient->primary.x = p_value . x;
            p_gradient->primary.y = p_value . y;
            x_dirty = true;
            break;
        case P_GRADIENT_FILL_SECONDARY_POINT:
            p_gradient->secondary.x = p_value . x;
            p_gradient->secondary.y = p_value . y;
            x_dirty = true;
            break;
            
        default:
            break;
    }
}

static void MCGradientFillSetBoolProperty(MCGradientFill*& p_gradient, MCGradientFillProperty which, bool p_value, bool& x_dirty)
{
    switch (which)
    {
        case P_GRADIENT_FILL_MIRROR:
            if (p_gradient->mirror != p_value)
            {
                p_gradient->mirror = p_value;
                x_dirty = true;
            }
            break;
        case P_GRADIENT_FILL_WRAP:
            if (p_gradient->wrap != p_value)
            {
                p_gradient->wrap = p_value;
                x_dirty = true;
            }
            break;
            
        default:
            break;
    }
}

static void MCGradientFillSetUIntProperty(MCGradientFill*& p_gradient, MCGradientFillProperty which, uinteger_t p_uint, bool& x_dirty)
{
    switch (which)
    {
        case P_GRADIENT_FILL_REPEAT:
        {
            uint4 t_new_repeat;
            t_new_repeat = MCU_max(1U, MCU_min(p_uint, 255U));
            if (p_gradient->repeat != t_new_repeat)
            {
                p_gradient->repeat = (uint1)t_new_repeat;
                x_dirty = true;
            }
        }
            break;
            
        default:
            break;
    }
}

static bool MCGradientFillStoreProperty(MCExecContext& ctxt, MCGradientFill*& p_gradient, MCGradientFillProperty which, MCRectangle rect, MCExecValue p_value, bool &dirty)
{
	MCGradientFill *t_gradient = nil;
	bool t_success = true;
    
	if (p_gradient == nil)
    {
		MCGradientFillInit(t_gradient, rect);
        dirty = true;
    }
	else
		t_gradient = p_gradient;
       
	switch (which)
	{
        case P_GRADIENT_FILL_TYPE:
        case P_GRADIENT_FILL_QUALITY:
            MCGradientFillSetEnumProperty(ctxt, t_gradient, which, p_value, dirty);
            break;
        case P_GRADIENT_FILL_RAMP:
        {
            MCAutoStringRef t_data;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value , kMCExecValueTypeStringRef, &(&t_data));
            if (!MCGradientFillRampParse(t_gradient->ramp, t_gradient->ramp_length, *t_data))
            {
                ctxt . LegacyThrow(EE_GRAPHIC_BADGRADIENTRAMP);
                t_success = false;
            }
            dirty = true;
        }
            break;
        case P_GRADIENT_FILL_ORIGIN:
        case P_GRADIENT_FILL_PRIMARY_POINT:
        case P_GRADIENT_FILL_SECONDARY_POINT:
        {
            MCPoint t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value , kMCExecValueTypePoint, &t_value);
            if (!ctxt . HasError())
                MCGradientFillSetPointProperty(t_gradient, which, t_value, dirty);
            else
            {
                p_gradient->old_origin.x = MININT2;
                p_gradient->old_origin.y = MININT2;
                dirty = true;
            }
        }
            break;
        case P_GRADIENT_FILL_MIRROR:
        case P_GRADIENT_FILL_WRAP:
        {
            bool t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value , kMCExecValueTypeBool, &t_value);
            MCGradientFillSetBoolProperty(t_gradient, which, t_value, dirty);
        }
            break;
        case P_GRADIENT_FILL_REPEAT:
        {
            uinteger_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value , kMCExecValueTypeUInt, &t_value);
            MCGradientFillSetUIntProperty(t_gradient, which, t_value, dirty);
        }
            break;
            
        default:
            break;
	}
	if (!t_success)
	{
		if (p_gradient == nil)
			MCGradientFillFree(t_gradient);
		return false;
	}
    
	if (dirty)
	{
		if (p_gradient == nil)
			p_gradient = t_gradient;
	}
	else
	{
		if (p_gradient == nil)
			MCGradientFillFree(t_gradient);
	}
	return t_success;
}

bool MCGradientFillSetElement(MCExecContext& ctxt, MCGradientFill*& x_gradient, MCNameRef p_prop, MCRectangle rect, MCExecValue p_setting, bool &dirty)
{
    MCGradientFillProperty t_property;
    if (MCGradientFillLookupProperty(p_prop, t_property) != ES_NORMAL)
        return false;
    
    return MCGradientFillStoreProperty(ctxt, x_gradient, t_property, rect, p_setting, dirty);
}

bool MCGradientFillSetProperties(MCExecContext& ctxt, MCGradientFill*& x_gradient, MCRectangle rect, MCExecValue p_value, bool& r_dirty)
{
	MCGradientFill *t_gradient = nil;
	
	if (x_gradient == nil)
	{
		MCGradientFillInit(t_gradient, rect);
		r_dirty = true;
	}
	else
		t_gradient = x_gradient;
    
	uint4 tablesize = ELEMENTS(gradientprops);

    // AL-2014-05-21: [[ Bug 12459 ]] Ensure setting gradient property to anything
    //  that isn't an array does the same as setting it to 'empty'.
    if (p_value . type == kMCExecValueTypeValueRef &&
        (MCValueIsEmpty(p_value . valueref_value) ||
         (MCValueGetTypeCode(p_value . valueref_value) != kMCValueTypeCodeArray)))
    {
        delete t_gradient;
        t_gradient = nil;
        x_gradient = nil;
        r_dirty = true;
    }
    else
    {
        MCAutoArrayRef t_array;
        MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value , kMCExecValueTypeArrayRef, &(&t_array));
        if (ctxt . HasError())
        {
            delete t_gradient;
            return false;
        }
        
        if (MCArrayIsEmpty(*t_array))
        {
            delete t_gradient;
            t_gradient = nil;
            x_gradient = nil;
            r_dirty = true;
        }
        else
        {
            MCerrorlock++;
            while (tablesize--)
            {
                MCValueRef t_prop_value;
                
                if (MCArrayFetchValue(*t_array, false, MCNAME(gradientprops[tablesize].token), t_prop_value))
                {
                    MCExecValue t_value;
                    t_value . valueref_value = MCValueRetain(t_prop_value);
                    t_value . type = kMCExecValueTypeValueRef;
                    MCGradientFillStoreProperty(ctxt, t_gradient, gradientprops[tablesize].value, rect, t_value, r_dirty);
                    ctxt . IgnoreLastError();
                }
            }
            MCerrorlock--;
        }
    }
    
	if (r_dirty)
	{
		if (x_gradient == nil)
			x_gradient = t_gradient;
	}
	else
	{
		if (x_gradient == nil)
			MCGradientFillFree(t_gradient);
	}
    
    return true;
}


int cmp_gradient_fill_stops(const void *a, const void *b)
{
	return ((MCGradientFillStop*)a)->offset - ((MCGradientFillStop*)b)->offset;
}

Boolean MCGradientFillRampParse(MCGradientFillStop* &r_stops, uint1 &r_stop_count, MCStringRef p_data)
{
	Boolean allvalid = True;
	bool ordered = true;
	uint4 t_nstops = 0;
    uint4 l = MCStringGetLength(p_data);

	MCAutoPointer<char>t_data;
    /* UNCHECKED */ MCStringConvertToCString(p_data, &t_data);
    const char *sptr = *t_data;
    // avoid overflow in the case of extremely long ramps
	while(t_nstops < 255 && (l != 0))
	{
		Boolean done1, done2;
		uint4 offset;
        offset = (uint4) (STOP_INT_MAX * (GRADIENT_ROUND_EPSILON + MCU_fmin(1.0,MCU_fmax(0.0,MCU_strtor8(sptr, l, ',', done1)))));
		uint1 r, g, b, a;
		r = MCU_max(0, MCU_min(255, MCU_strtol(sptr, l, ',', done2)));
		done1 &= done2;	
		g = MCU_max(0, MCU_min(255, MCU_strtol(sptr, l, ',', done2)));
		done1 &= done2;
		b = MCU_max(0, MCU_min(255, MCU_strtol(sptr, l, ',', done2)));
		done1 &= done2;

		if (l == 0 || *(sptr-1) == '\n')
			a = 255;
		else
			a = MCU_max(0, MCU_min(255, MCU_strtol(sptr, l, ',', done2)));
		
		while (l && !isdigit((uint1)*sptr) && *sptr != '-' && *sptr != '+')
		{
			l--;
			sptr++;
		}
		if (!done1 || !done2)
			allvalid = False;
		if (t_nstops > 0 && offset < r_stops[t_nstops-1].offset)
			ordered = false;
		
		if (t_nstops+1 > r_stop_count)
			MCU_realloc((char **)&r_stops, t_nstops, t_nstops + 1, sizeof(MCGradientFillStop));
		r_stops[t_nstops].offset = offset;
		if (t_nstops > 0 && offset != r_stops[t_nstops-1].offset)
			r_stops[t_nstops-1].difference = STOP_DIFF_MULT / (offset - r_stops[t_nstops-1].offset);
		r_stops[t_nstops].color =  MCGPixelPack(kMCGPixelFormatBGRA, r, g, b, a);
		r_stops[t_nstops++].hw_color = MCGPixelPackNative(r, g, b, a);
	}
	r_stop_count = t_nstops;
	if (!ordered)
	{
		qsort(r_stops, r_stop_count, sizeof(MCGradientFillStop), cmp_gradient_fill_stops);
		for (uint4 i = 1; i < r_stop_count; i++)
			if (r_stops[i].offset != r_stops[i - 1].offset)
				r_stops[i - 1].difference = STOP_DIFF_MULT / (r_stops[i].offset - r_stops[i - 1].offset);
	}
	return allvalid;
}

bool MCGradientFillRampUnparse(MCGradientFillStop* p_stops, uint1 p_stop_count, MCStringRef& r_data)
{
	uint4 i;

    MCAutoListRef t_ramp;
    MCListCreateMutable('\n', &t_ramp);
    
	for (i=0; i<p_stop_count; i++)
	{
        MCAutoStringRef t_string;
		uint1 r, g, b, a;
		
		a = (p_stops[i].color >> 24) & 0xFF;
		r = (p_stops[i].color >> 16) & 0xFF;
		g = (p_stops[i].color >> 8) & 0xFF;
		b = (p_stops[i].color) & 0xFF;
		
		// MW-2008-08-07: Adjusting the precision of the stop value since we have at most 65535 values
		//   which therefore need no more than 100000 decimal values.
        const char *t_format;
        t_format = a == 255 ? "%.5f,%d,%d,%d" : "%.5f,%d,%d,%d,%d";
		if (!MCStringFormat(&t_string, t_format, (p_stops[i].offset * (1.0 / STOP_INT_MAX) + GRADIENT_ROUND_EPSILON), r, g, b, a))
			return false;
		
		if (!MCListAppend(*t_ramp, *t_string))
			return false;
	}

	return MCListCopyAsString(*t_ramp, r_data);
}

uint4 MCGradientFillMeasure(MCGradientFill *p_gradient)
{
	return 3 + 4 * 3 + p_gradient -> ramp_length * 6;
}

IO_stat MCGradientFillSerialize(MCGradientFill *p_gradient, MCObjectOutputStream& p_stream)
{
	IO_stat t_stat;

	t_stat = p_stream . WriteU8((p_gradient->kind << 4) | (p_gradient->quality << 2) | (p_gradient->mirror << 1) | (p_gradient->wrap));
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU8(p_gradient -> repeat);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU8(p_gradient -> ramp_length);

    if (t_stat == IO_NORMAL)
        t_stat = p_stream.WritePoint(p_gradient->origin);
    if (t_stat == IO_NORMAL)
        t_stat = p_stream.WritePoint(p_gradient->primary);
    if (t_stat == IO_NORMAL)
        t_stat = p_stream.WritePoint(p_gradient->secondary);

	for(int i = 0; i < p_gradient -> ramp_length; ++i)
	{
		t_stat = p_stream . WriteU16((uint16_t)p_gradient -> ramp[i] . offset);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteU32(p_gradient -> ramp[i] . color);
	}

	return t_stat;
}

IO_stat MCGradientFillUnserialize(MCGradientFill *p_gradient, MCObjectInputStream& p_stream)
{
	IO_stat t_stat;

	uint1 t_packed;
	t_stat = p_stream . ReadU8(t_packed);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadU8(p_gradient -> repeat);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadU8(p_gradient -> ramp_length);

	MCPoint *t_points;
	t_points = &p_gradient -> origin;
	for(int i = 0; i < 3 && t_stat == IO_NORMAL; ++i)
	{
		t_stat = p_stream . ReadS16(t_points[i] . x);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . ReadS16(t_points[i] . y);
	}

	if (t_stat == IO_NORMAL)
	{
		if (p_gradient != NULL)
			delete[] p_gradient -> ramp; /* Allocated with new[] */
		p_gradient -> ramp = new (nothrow) MCGradientFillStop[p_gradient -> ramp_length];
		for(int i = 0; i < p_gradient -> ramp_length && t_stat == IO_NORMAL; ++i)
		{
			uint16_t t_offset;
			t_stat = p_stream . ReadU16(t_offset);
			if (t_stat == IO_NORMAL)
			{
				p_gradient -> ramp[i] . offset = t_offset;
				
				uint32_t t_color;
				t_stat = p_stream . ReadU32(t_color);
				if (t_stat == IO_NORMAL)
				{
					p_gradient -> ramp[i] . color = t_color;
					
					uint8_t r, g, b, a;
					MCGPixelUnpack(kMCGPixelFormatBGRA, t_color, r, g, b, a);
					p_gradient -> ramp[i] . hw_color = MCGPixelPackNative(r, g, b, a);
				}
			}
		}
	}

	if (t_stat == IO_NORMAL)
	{
		p_gradient->kind = t_packed >> 4;
		p_gradient->quality = (t_packed >> 2) & 0x03;
		p_gradient->mirror = (t_packed >> 1) & 0x01;
		p_gradient->wrap = t_packed & 0x01;

		for (uint4 i = 1; i < p_gradient->ramp_length; i++)
			if (p_gradient->ramp[i].offset != p_gradient->ramp[i - 1].offset)
				p_gradient->ramp[i - 1].difference = STOP_DIFF_MULT / (p_gradient->ramp[i].offset - p_gradient->ramp[i - 1].offset);
	}

	return t_stat;
}

template <typename IntType>
static IntType
MCGradientFillUnserializeInt(MCSpan<byte_t>::const_iterator& p_data)
{
	IntType t_value;
    const byte_t &t_data = *p_data;
    p_data += sizeof(t_value);
	MCMemoryCopy(&t_value, &t_data, sizeof(t_value));
	return MCSwapIntHostToNetwork(t_value);
}

static MCPoint
MCGradientFillUnserializePoint(MCSpan<byte_t>::const_iterator& p_data)
{
	MCPoint t_point;
	t_point.x = MCGradientFillUnserializeInt<int16_t>(p_data);
	t_point.y = MCGradientFillUnserializeInt<int16_t>(p_data);
	return t_point;
}

void MCGradientFillUnserialize(MCGradientFill *p_gradient, uint1 *p_data, uint4 &r_length)
{
    MCSpan<byte_t> t_span(p_data, r_length);
    MCSpan<byte_t>::const_iterator t_ptr = t_span.cbegin();

	uint1 t_packed = *t_ptr++;
	p_gradient->kind = t_packed >> 4;
	p_gradient->quality = (t_packed >> 2) & 0x03;
	p_gradient->mirror = (t_packed >> 1) & 0x01;
	p_gradient->wrap = t_packed & 0x01;

	p_gradient->repeat = *t_ptr++;
	p_gradient->ramp_length = *t_ptr++;
	p_gradient->origin = MCGradientFillUnserializePoint(t_ptr);
	p_gradient->primary = MCGradientFillUnserializePoint(t_ptr);
	p_gradient->secondary = MCGradientFillUnserializePoint(t_ptr);

	if (p_gradient->ramp != NULL)
		delete p_gradient->ramp;
	p_gradient->ramp = new (nothrow) MCGradientFillStop[p_gradient->ramp_length];

	for (uint4 i = 0; i < p_gradient->ramp_length; i++)
	{
		p_gradient->ramp[i].offset = MCGradientFillUnserializeInt<uint16_t>(t_ptr);
		p_gradient->ramp[i].color = MCGradientFillUnserializeInt<uint32_t>(t_ptr);
	}
	for (uint4 i = 1; i < p_gradient->ramp_length; i++)
		if (p_gradient->ramp[i].offset != p_gradient->ramp[i - 1].offset)
			p_gradient->ramp[i - 1].difference = STOP_DIFF_MULT / (p_gradient->ramp[i].offset - p_gradient->ramp[i - 1].offset);

	r_length = t_span.cend() - t_ptr;
}
