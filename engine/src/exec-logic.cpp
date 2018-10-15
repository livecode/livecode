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

#include "globals.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

static bool MCLogicIsEqualTo(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result)
{    
	// If the two value ptrs are the same, we are done.
	if (p_left == p_right)
	{
		r_result = true;
		return true;
	}
    
	bool t_left_array, t_right_array;
	t_left_array = MCValueGetTypeCode(p_left) == kMCValueTypeCodeArray && !MCArrayIsEmpty((MCArrayRef)p_left);
	t_right_array = MCValueGetTypeCode(p_right) == kMCValueTypeCodeArray &&	!MCArrayIsEmpty((MCArrayRef)p_right);
	
	// MW-2012-12-11: [[ ArrayComp ]] If both are arrays and non-empty then
	//   compare as arrays; otherwise if either is an array they become empty
	//   in string context and as such are less (or more) than whatever the
	//   otherside has.
	if (t_left_array != t_right_array)
	{
		r_result = false;
		return true;
	}
	
	// If both are arrays, then compare them as that.
	if (t_left_array)
	{
		if (MCArrayGetCount((MCArrayRef)p_left) != MCArrayGetCount((MCArrayRef)p_right))
		{
			r_result = false;
			return true;
		}

		MCNameRef t_key;
		MCValueRef t_value;
		uintptr_t t_iterator;
		t_iterator = 0;
		while(MCArrayIterate((MCArrayRef)p_left, t_iterator, t_key, t_value))
		{
			MCValueRef t_other_value;
			if (MCArrayFetchValue((MCArrayRef)p_right, ctxt . GetCaseSensitive(), t_key, t_other_value))
			{
				bool t_result;
				if (MCLogicIsEqualTo(ctxt, t_value, t_other_value, t_result))
				{
					if (t_result)
						continue;
				}
			}
            
            r_result = false;
            return true;
		}

		r_result = true;
		return true;
	}
    
	// MW-2014-01-28: Only compare as numbers if both values are non-empty.
	bool t_left_is_empty, t_right_is_empty;
	t_left_is_empty = MCValueIsEmpty(p_left);
	t_right_is_empty = MCValueIsEmpty(p_right);
    
    bool t_left_converted;
    real64_t t_left_num;
    if (p_left == kMCNull || !t_left_is_empty)
    {
        if (!ctxt . TryToConvertToReal(p_left, t_left_converted, t_left_num))
            return false;
    }
    else
        t_left_converted = false;
    
    bool t_right_converted;
    real64_t t_right_num;
    if (p_right == kMCNull || !t_right_is_empty)
    {
        if (!ctxt . TryToConvertToReal(p_right, t_right_converted, t_right_num))
            return false;
    }
    else
        t_right_converted = false;
    
    if (t_left_converted && t_right_converted)
    {
        if (t_left_num == t_right_num)
        {
            r_result = true;
            return true;
        }
        
        real64_t t_dleft, t_dright;
        t_dleft = fabs(t_left_num);
        t_dright = fabs(t_right_num);
        
        real64_t t_min;
        t_min = MCMin(t_dleft, t_dright);
        
        if (t_min < MC_EPSILON)
            r_result = fabs(t_left_num - t_right_num) < MC_EPSILON;
        else
            r_result = fabs(t_left_num - t_right_num) / t_min < MC_EPSILON;
        
        return true;
    }
    
    // AL-2014-06-12: [[ Bug 12195 ]] If one is empty and the other not,
    //  by the time we're here, then we don't have equality.
    if (t_left_is_empty != t_right_is_empty)
    {
        r_result = false;
        return true;
    }
    
    // AL-2014-06-12: [[ Bug 12195 ]] If left and right are data, compare as data
    if (MCValueGetTypeCode(p_left) == kMCValueTypeCodeData &&
        MCValueGetTypeCode(p_right) == kMCValueTypeCodeData)
    {
        r_result = MCDataIsEqualTo((MCDataRef)p_left, (MCDataRef)p_right);
        return true;
    }
    
	// Otherwise, convert both to strings and compare.
	MCAutoStringRef t_left_str, t_right_str;
	if (ctxt . ForceToString(p_left, &t_left_str) &&
		ctxt . ForceToString(p_right, &t_right_str))
	{
		r_result = MCStringIsEqualTo(*t_left_str, *t_right_str, ctxt . GetStringComparisonType());
		return true;
	}

	return false;
}

static bool MCLogicCompareTo(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, compare_t& r_result)
{
	// If both can be numbers, then compare them as that.
	bool t_left_converted, t_right_converted;
	real64_t t_left_num, t_right_num;
	if (!ctxt . TryToConvertToReal(p_left, t_left_converted, t_left_num) ||
		!ctxt . TryToConvertToReal(p_right, t_right_converted, t_right_num))
		return false;
		
	if (t_left_converted && t_right_converted)
	{
        if (t_left_num == t_right_num)
        {
            r_result = 0;
            return true;
        }
        
		real64_t t_dleft, t_dright;
		t_dleft = fabs(t_left_num);
		t_dright = fabs(t_right_num);

		real64_t t_min;
		t_min = MCMin(t_dleft, t_dright);

		r_result = 1;
		if (t_min < MC_EPSILON)
		{
			if (fabs(t_left_num - t_right_num) < MC_EPSILON)
				r_result = 0;
		}
		else
			if (fabs(t_left_num - t_right_num) / t_min < MC_EPSILON)
				r_result = 0;

		if (r_result)
			if (t_left_num < t_right_num)
				r_result = -1;

		return true;
	}

	// Otherwise, convert both to strings and compare.
	MCAutoStringRef t_left_str, t_right_str;
	if (ctxt . ForceToString(p_left, &t_left_str) &&
		ctxt . ForceToString(p_right, &t_right_str))
	{
		r_result = MCStringCompareTo(*t_left_str, *t_right_str, ctxt . GetStringComparisonType());
		return true;
	}

	return false;
}

void MCLogicEvalIsEqualTo(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result)
{
	if (MCLogicIsEqualTo(ctxt, p_left, p_right, r_result))
		return;

	ctxt . Throw();
}

void MCLogicEvalIsNotEqualTo(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result)
{
	if (MCLogicIsEqualTo(ctxt, p_left, p_right, r_result))
	{
		r_result = !r_result;
		return;
	}

	ctxt . Throw();
}

void MCLogicEvalIsGreaterThan(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result)
{
	compare_t t_order;
	if (MCLogicCompareTo(ctxt, p_left, p_right, t_order))
	{
		r_result = t_order > 0;
		return;
	}

	ctxt . Throw();
}

void MCLogicEvalIsGreaterThanOrEqualTo(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result)
{
	compare_t t_order;
	if (MCLogicCompareTo(ctxt, p_left, p_right, t_order))
	{
		r_result = t_order >= 0;
		return;
	}

	ctxt . Throw();
}

void MCLogicEvalIsLessThan(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result)
{
	compare_t t_order;
	if (MCLogicCompareTo(ctxt, p_left, p_right, t_order))
	{
		r_result = t_order < 0;
		return;
	}

	ctxt . Throw();
}

void MCLogicEvalIsLessThanOrEqualTo(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result)
{
	compare_t t_order;
	if (MCLogicCompareTo(ctxt, p_left, p_right, t_order))
	{
		r_result = t_order <= 0;
		return;
	}

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCLogicEvalAnd(MCExecContext& ctxt, bool p_a, bool p_b, bool& r_result)
{
	r_result = p_a && p_b;
}

void MCLogicEvalOr(MCExecContext& ctxt, bool p_a, bool p_b, bool& r_result)
{
	r_result = p_a || p_b;
}

void MCLogicEvalNot(MCExecContext& ctxt, bool p_bool, bool& r_result)
{
	r_result = !p_bool;
}

////////////////////////////////////////////////////////////////////////////////

void MCLogicEvalIsABoolean(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCAutoBooleanRef t_boolean;
	r_result = ctxt.ConvertToBoolean(p_value, &t_boolean);
}

void MCLogicEvalIsNotABoolean(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCLogicEvalIsABoolean(ctxt, p_value, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////
