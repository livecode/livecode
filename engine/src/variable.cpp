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
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"
#include "mcio.h"

#include "stack.h"
#include "card.h"
#include "field.h"
#include "handler.h"
#include "hndlrlst.h"

#include "scriptpt.h"
#include "mcerror.h"
#include "util.h"
#include "debug.h"
#include "globals.h"
#include "objectstream.h"
#include "parentscript.h"
#include "osspec.h"
#include "syntax.h"
#include "variable.h"

////////////////////////////////////////////////////////////////////////////////

bool MCVariable::create(MCVariable*& r_var)
{
	MCVariable *self;
	self = new (nothrow) MCVariable;
	if (self == nil)
		return false;

	r_var = self;
	
	return true;
}

bool MCVariable::createwithname(MCNameRef p_name, MCVariable*& r_var)
{
	MCVariable *self;
	if (!create(self))
		return false;

    self->name.Reset(p_name);

	r_var = self;

	return true;
}

// This is only called by MCObject to create copies of prop sets.
bool MCVariable::createcopy(MCVariable& p_var, MCVariable*& r_new_var)
{
	MCVariable *self = nullptr;
	if (!create(self))
        return false;

    self->name.Reset(*p_var.name);
    p_var . value . valueref_value = self -> value . valueref_value;
    p_var . value . type = self -> value . type;

    r_new_var = self;

    return true;
}

bool MCVariable::encode(void *&r_buffer, uindex_t& r_size)
{
    IO_handle t_stream;
    t_stream = MCS_fakeopenwrite();
    if (t_stream == NULL)
        return false;

    IO_stat t_stat;
    t_stat = IO_NORMAL;

    if (value . type == kMCExecValueTypeArrayRef)
    {
        t_stat = IO_write_uint1(kMCEncodedValueTypeLegacyArray, t_stream);

        if (t_stat == IO_NORMAL)
            t_stat = MCArraySaveToHandleLegacy(value . arrayref_value, t_stream);
    }
    else if (value . type == kMCExecValueTypeDataRef)
    {
        t_stat = IO_write_uint1(kMCEncodedValueTypeString, t_stream);
        if (t_stat == IO_NORMAL)
            t_stat = IO_write(MCDataGetBytePtr(value . dataref_value), 1, MCDataGetLength(value . dataref_value), t_stream);
    }
    else if (value . type == kMCExecValueTypeStringRef)
    {
        t_stat = IO_write_uint1(kMCEncodedValueTypeString, t_stream);
        if (t_stat == IO_NORMAL)
            t_stat = IO_write_stringref_legacy(value . stringref_value, t_stream, false);
    }
    else if (value . type == kMCExecValueTypeNone)
    {
        t_stat = IO_write_uint1((uint1)kMCEncodedValueTypeEmpty, t_stream);
    }
    else if (MCExecTypeIsNumber(value . type))
    {
        double t_value;
        if (value . type == kMCExecValueTypeUInt)
            t_value = (double)value . uint_value;
        else if (value . type == kMCExecValueTypeUInt)
            t_value = (double) value . int_value;
        else if (value . type == kMCExecValueTypeUInt)
            t_value = (double) value . float_value;
        else if (value . type == kMCExecValueTypeUInt)
            t_value = value . double_value;

        t_stat = IO_write_uint1((uint1)kMCEncodedValueTypeNumber, t_stream);

        if (t_stat == IO_NORMAL)
            t_stat = IO_write_real8(t_value, t_stream);
    }
    else
        t_stat = IO_ERROR;

    if (t_stat == IO_NORMAL)
    {
        char *t_buffer;
        size_t t_length;
        t_stat = MCS_closetakingbuffer(t_stream, (void*&)t_buffer, t_length);

        if (t_stat == IO_NORMAL)
        {
            r_buffer = t_buffer;
            r_size = t_length;
        }
        else
            delete t_buffer;
    }
    else
        MCS_close(t_stream);
    
    return t_stat == IO_NORMAL;
}

bool MCVariable::decode(void *p_buffer, uindex_t p_size)
{
    IO_handle t_stream;
    t_stream = MCS_fakeopen(p_buffer, p_size);
    if (t_stream == NULL)
        return false;

    IO_stat t_stat;
    t_stat = IO_NORMAL;

    uint8_t t_type;
    t_stat = IO_read_uint1((uint1*)&t_type, t_stream);
    if (t_stat == IO_NORMAL)
    {
        switch(t_type)
        {
        case kMCEncodedValueTypeEmpty:
            clear();
        break;
        case kMCEncodedValueTypeString:
        {
            MCAutoStringRef t_value;
            t_stat = IO_read_stringref_legacy(&t_value, t_stream, false);

            if (t_stat == IO_NORMAL)
                /* UNCHECKED */ setvalueref(*t_value);
        }
        break;
        case kMCEncodedValueTypeNumber:
        {
            real8 t_value;
            t_stat = IO_read_real8(&t_value, t_stream);

            if (t_stat == IO_NORMAL)
                setnvalue(t_value);
        }
        break;
        case kMCEncodedValueTypeLegacyArray:
        {
            MCAutoArrayRef t_array;
			if (!MCArrayCreateMutable(&t_array))
				t_stat = IO_ERROR;
				
			if (t_stat == IO_NORMAL)
            	t_stat = MCArrayLoadFromHandleLegacy(*t_array, t_stream);

            if (t_stat == IO_NORMAL && !setvalueref(*t_array))
                t_stat = IO_ERROR;
        }
		break;
        default:
            t_stat = IO_ERROR;
        break;
        }
    }

    MCS_close(t_stream);
    return t_stat == IO_NORMAL;
}

MCVariable::~MCVariable(void)
{
    MCExecTypeRelease(value);
}

////////////////////////////////////////////////////////////////////////////////

bool MCVariable::isuql(void) const
{
	return is_uql;
}

void MCVariable::clearuql(void)
{    
	if (!is_uql)
		return;
    
    // SN-2014-04-09 [[ Bug 12160 ]] Put after/before on an uninitialised, by-reference parameter inserts the variable's name in it
    // The content of a UQL value was not cleared when needed
    if (value . type == kMCExecValueTypeNameRef && MCNameIsEqualTo(value . nameref_value, *name))
        clear();
    
	is_uql = false;
}

Boolean MCVariable::isclear(void) const
{
    return value . type == kMCExecValueTypeNone;
}

Boolean MCVariable::isfree(void) const
{
	return value . type == kMCExecValueTypeNone;
}

Boolean MCVariable::isarray(void) const
{
	return value . type == kMCExecValueTypeArrayRef;
}

Boolean MCVariable::isempty(void) const
{
	if (value . nameref_value== kMCEmptyName)
		return True;
	return value . type == kMCExecValueTypeStringRef && MCValueIsEmpty(value . stringref_value);
}

void MCVariable::clear(Boolean p_delete_buffer)
{
    MCExecTypeRelease(value);
    value . type = kMCExecValueTypeNone;
}

Exec_stat MCVariable::sets(const MCString& p_string)
{
	copysvalue(p_string);
	return ES_NORMAL;
}

void MCVariable::copysvalue(const MCString& p_string)
{
	MCStringRef t_string;
	if (MCStringCreateWithNativeChars((const char_t *)p_string . getstring(), p_string . getlength(), t_string))
	{
        MCExecTypeRelease(value);
        value . type = kMCExecValueTypeStringRef;
		value . stringref_value = t_string;
		return;
	}
}

void MCVariable::grab(char *p_buffer, uint32_t p_length)
{
	copysvalue(MCString(p_buffer, p_length));
	free(p_buffer);
}

void MCVariable::setnvalue(real8 p_number)
{
	MCExecTypeRelease(value);
	value . type = kMCExecValueTypeDouble;
    value . double_value = p_number;
}

bool MCVariable::setvalueref(MCValueRef p_value)
{
	return setvalueref(nil, 0, false, p_value);
}

bool MCVariable::setvalueref(MCNameRef *p_path, uindex_t p_length, bool p_case_sensitive, MCValueRef p_value)
{
	if (p_length == 0)
	{
		MCValueRef t_new_value;
		if (!MCValueCopy(p_value, t_new_value))
			return false;
        
		MCExecTypeRelease(value);
		MCExecTypeSetValueRef(value, t_new_value);
		return true;
	}

	if (!converttomutablearray())
		return false;

    MCAutoValueRef t_copied_value;
    if (!MCValueCopy(p_value, &t_copied_value))
        return false;

    if (MCArrayStoreValueOnPath(value . arrayref_value, p_case_sensitive, p_path, p_length, *t_copied_value))
		return true;

	return false;
}

MCValueRef MCVariable::getvalueref(void)
{
	if (!is_uql)
    {
        if (!MCExecTypeIsValueRef(value . type))
        {
            MCExecContext ctxt(nil, nil, nil);
            /* UNCHECKED */ MCExecTypeConvertAndReleaseAlways(ctxt, value . type, &value, kMCExecValueTypeValueRef, &value);
            // SN-2014-07-28: [[ Bug 12937 ]] The value stored is now a valueRef
            value . type = kMCExecValueTypeValueRef;
        }
        
        return value . valueref_value;
    }
	return *name;
}

MCValueRef MCVariable::getvalueref(MCNameRef *p_path, uindex_t p_length, bool p_case_sensitive)
{
	if (p_length == 0)
		return getvalueref();

	MCValueRef t_value;
	if (value . type == kMCExecValueTypeArrayRef &&
            MCArrayFetchValueOnPath(value . arrayref_value, p_case_sensitive, p_path, p_length, t_value))
		return t_value;

	return kMCEmptyString;
}

bool MCVariable::copyasexecvalue(MCExecValue& r_value)
{
    MCExecTypeCopy(value, r_value);
    return true;
}

MCExecValue MCVariable::getexecvalue(void)
{
    return value;
}

bool MCVariable::copyasvalueref(MCValueRef& r_value)
{
    return copyasvalueref(nil, 0, false, r_value);
}

bool MCVariable::copyasvalueref(MCNameRef *p_path, uindex_t p_length, bool p_case_sensitive, MCValueRef& r_value)
{
    return MCValueCopy(getvalueref(p_path, p_length, p_case_sensitive), r_value);
}

bool MCVariable::eval(MCExecContext& ctxt, MCValueRef& r_value)
{
    return eval(ctxt, nil, 0, r_value);
}

bool MCVariable::eval(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_length, MCValueRef& r_value)
{
    return copyasvalueref(p_path, p_length, ctxt . GetCaseSensitive(), r_value);
}

bool MCVariable::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    MCExecTypeCopy(value, r_value);
    return true;
}

bool MCVariable::eval_ctxt(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_length, MCExecValue& r_value)
{
    if (p_length == 0)
        return eval_ctxt(ctxt, r_value);
    
    if (copyasvalueref(p_path, p_length, ctxt . GetCaseSensitive(), r_value . valueref_value))
    {
        r_value . type = kMCExecValueTypeValueRef;
        return true;
    }
    return false;
}

bool MCVariable::set(MCExecContext& ctxt, MCValueRef p_value, MCVariableSettingStyle p_setting)
{
    return set(ctxt, p_value, nil, 0, p_setting);
}

bool MCVariable::set(MCExecContext& ctxt, MCValueRef p_value, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting)
{
    if (p_setting != kMCVariableSetInto)
        return modify(ctxt, p_value, p_path, p_length, p_setting);
    
    if (setvalueref(p_path, p_length, ctxt . GetCaseSensitive(), p_value))
    {
        synchronize(ctxt, true);
        return true;
    }
    
    return false;
}

bool MCVariable::give_value(MCExecContext& ctxt, MCExecValue p_value, MCVariableSettingStyle p_setting)
{
    if (p_setting != kMCVariableSetInto)
        return modify_ctxt(ctxt, p_value, p_setting);
    
    if (MCExecTypeIsValueRef(p_value . type))
    {
        setvalueref(p_value . valueref_value);
        MCValueRelease(p_value . valueref_value);
    }
    else
    {
        MCExecTypeRelease(value);
        value = p_value;
    }
    
    // SN-2014-09-18 [[ Bug 13453 ]] give_value should notify the debugger about it
    synchronize(ctxt, true);
    return true;
}

bool MCVariable::give_value(MCExecContext& ctxt, MCExecValue p_value, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting)
{
    if (p_setting != kMCVariableSetInto)
        return modify_ctxt(ctxt, p_value, p_path, p_length, p_setting);
    
    if (p_length == 0)
        return give_value(ctxt, p_value, p_setting);
    
    MCAutoValueRef t_value;
    MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeValueRef, &(&t_value));
    
    if (!ctxt . HasError() && setvalueref(p_path, p_length, ctxt . GetCaseSensitive(), *t_value))
    {
        synchronize(ctxt, true);
        return true;
    }
    
    return false;
}

bool MCVariable::can_become_data(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_length)
{
    MCValueRef t_current_value;
    t_current_value = nil;
 
    if (p_length == 0)
    {
        // If we are already data, then we can stay data
        if (value . type == kMCExecValueTypeDataRef)
            return true;
        
        // If we are anything other than a value, we can always convert to data safely
        if (!MCExecTypeIsValueRef(value . type))
            return true;
        
        // Otherwise, check the value ref
        t_current_value = value . valueref_value;
    }
    else
        t_current_value = getvalueref(p_path, p_length, ctxt . GetCaseSensitive());
    
    // The only values that cannot convert losslessly to data are strings or names that contain non-native characters
    if (MCValueGetTypeCode(t_current_value) == kMCValueTypeCodeString)
        return MCStringIsNative((MCStringRef)t_current_value);
    
    if (MCValueGetTypeCode(t_current_value) == kMCValueTypeCodeName)
        return MCStringIsNative(MCNameGetString((MCNameRef)t_current_value));
    
    return true;
}

bool MCVariable::modify(MCExecContext& ctxt, MCValueRef p_value, MCVariableSettingStyle p_setting)
{
	return modify(ctxt, p_value, nil, 0, p_setting);
}

bool MCVariable::modify_data(MCExecContext& ctxt, MCDataRef p_data, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting)
{    
	if (p_length == 0)
	{
        if (!converttomutabledata(ctxt))
            return false;
        
        bool t_success = false;
        // SN-2014-04-11 [[ FasterVariable ]] now chose between appending or prepending
        if (p_setting == kMCVariableSetAfter)
            t_success = MCDataAppend(value . dataref_value, p_data);
        else if (p_setting == kMCVariableSetBefore)
            t_success = MCDataPrepend(value . dataref_value, p_data);
        
        if (!t_success)
			return false;
        
        synchronize(ctxt, true);
        
		return true;
	}
    
	MCValueRef t_current_value;
	t_current_value = getvalueref(p_path, p_length, ctxt . GetCaseSensitive());

    MCDataRef t_value_as_data;
    t_value_as_data = nil;
    // SN-2014-04-11 [[ FasterVariable ]] now chose between appending or prepending
	if (ctxt . ConvertToData(t_current_value, t_value_as_data) &&
	    MCDataMutableCopyAndRelease(t_value_as_data, t_value_as_data) &&
		((p_setting == kMCVariableSetAfter && MCDataAppend(t_value_as_data, p_data)) ||
         (p_setting == kMCVariableSetBefore && MCDataPrepend(t_value_as_data, p_data))) &&
		setvalueref(p_path, p_length, ctxt . GetCaseSensitive(), t_value_as_data))
	{
		MCValueRelease(t_value_as_data);
        synchronize(ctxt, true);
		return true;
	}
    
	MCValueRelease(t_value_as_data);
	return false;
}

bool MCVariable::modify_string(MCExecContext& ctxt, MCStringRef p_value, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting)
{
    if (p_length == 0)
    {
        if (!converttomutablestring(ctxt))
            return false;
        
        bool t_success = false;
        // SN-2014-04-11 [[ FasterVariable ]] now chose between appending or prepending
        // The value is now a stringref
        if (p_setting == kMCVariableSetAfter)
            t_success = MCStringAppend(value . stringref_value, p_value);
        else if (p_setting == kMCVariableSetBefore)
            t_success = MCStringPrepend(value . stringref_value, p_value);
        
        if (!t_success)
            return false;
        
        synchronize(ctxt, true);
        
        return true;
    }
    
    MCValueRef t_current_value;
    t_current_value = getvalueref(p_path, p_length, ctxt . GetCaseSensitive());
    
    MCStringRef t_current_value_as_string;
    t_current_value_as_string = nil;
    // SN-2014-04-11 [[ FasterVariable ]] now chose between appending or prepending
    if (ctxt . ConvertToString(t_current_value, t_current_value_as_string) &&
        MCStringMutableCopyAndRelease(t_current_value_as_string, t_current_value_as_string) &&
        ((p_setting == kMCVariableSetAfter && MCStringAppend(t_current_value_as_string, p_value)) ||
         (p_setting == kMCVariableSetBefore && MCStringPrepend(t_current_value_as_string, p_value))) &&
        setvalueref(p_path, p_length, ctxt . GetCaseSensitive(), t_current_value_as_string))
    {
        MCValueRelease(t_current_value_as_string);
        synchronize(ctxt, true);
        return true;
    }
    
    MCValueRelease(t_current_value_as_string);
    return false;
}

bool MCVariable::modify(MCExecContext& ctxt, MCValueRef p_value, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting)
{
    if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeData && can_become_data(ctxt, p_path, p_length))
        return modify_data(ctxt, (MCDataRef)p_value, p_path, p_length, p_setting);

    MCAutoStringRef t_value;
    if (!ctxt . ConvertToString(p_value, &t_value))
        return false;
    
    return modify_string(ctxt, *t_value, p_path, p_length, p_setting);
}

bool MCVariable::modify_ctxt(MCExecContext& ctxt, MCExecValue p_value, MCVariableSettingStyle p_setting)
{
	return modify_ctxt(ctxt, p_value, nil, 0, p_setting);
}

bool MCVariable::modify_ctxt(MCExecContext& ctxt, MCExecValue p_value, MCNameRef *p_path, uindex_t p_length, MCVariableSettingStyle p_setting)
{
    if (p_value . type == kMCExecValueTypeDataRef)
    {
        if (can_become_data(ctxt, p_path, p_length))
        {
            // AL-2014-11-20: In this codepath, we are taking the value rather than retaining, so make sure the DataRef is released.
            MCAutoDataRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeDataRef, &(&t_value));
            return modify_data(ctxt, *t_value, p_path, p_length, p_setting);
        }
    }
    
    MCAutoStringRef t_value;
    MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_value));
    
    if (ctxt . HasError())
        return ctxt . IgnoreLastError(), false;
    
    return modify_string(ctxt, *t_value, p_path, p_length, p_setting);
}

bool MCVariable::replace(MCExecContext& ctxt, MCValueRef p_replacement, MCRange p_range)
{
    return replace(ctxt, p_replacement, p_range, nil, 0);
}

bool MCVariable::replace_data(MCExecContext& ctxt, MCDataRef p_replacement, MCRange p_range, MCNameRef *p_path, uindex_t p_length)
{
    if (p_length == 0)
    {
        if (!converttomutabledata(ctxt))
            return false;
            
        // We are now sure to have a dataref in our ExecValue
        MCDataReplace(value . dataref_value, p_range, (MCDataRef)p_replacement);
        
        synchronize(ctxt, true);
        
        return true;
    }
    
    MCValueRef t_current_value;
    t_current_value = getvalueref(p_path, p_length, ctxt . GetCaseSensitive());
    
    MCDataRef t_current_value_as_data;
    t_current_value_as_data = nil;
        
    if (ctxt . ConvertToData(t_current_value, t_current_value_as_data) &&
        MCDataMutableCopyAndRelease(t_current_value_as_data, t_current_value_as_data) &&
        MCDataReplace(t_current_value_as_data, p_range, (MCDataRef)p_replacement) &&
        setvalueref(p_path, p_length, ctxt . GetCaseSensitive(), t_current_value_as_data))
    {
        MCValueRelease(t_current_value_as_data);
        synchronize(ctxt, true);
        return true;
    }
        
    MCValueRelease(t_current_value_as_data);
    return false;
}

bool MCVariable::replace_string(MCExecContext& ctxt, MCStringRef p_replacement, MCRange p_range, MCNameRef *p_path, uindex_t p_length)
{
    if (p_length == 0)
    {
        if (!converttomutablestring(ctxt))
            return false;
        
        // We are now sure to have a stringref in our ExecValue
        MCStringReplace(value . stringref_value, p_range, p_replacement);
        
        synchronize(ctxt, true);
        
        return true;
    }
    
    MCValueRef t_current_value;
    t_current_value = getvalueref(p_path, p_length, ctxt . GetCaseSensitive());
    
    MCStringRef t_current_value_as_string;
    t_current_value_as_string = nil;
    // SN-2014-04-11 [[ FasterVariable ]] now chose between appending or prepending
    if (ctxt . ConvertToString(t_current_value, t_current_value_as_string) &&
        MCStringMutableCopyAndRelease(t_current_value_as_string, t_current_value_as_string) &&
        MCStringReplace(t_current_value_as_string, p_range, p_replacement) &&
        setvalueref(p_path, p_length, ctxt . GetCaseSensitive(), t_current_value_as_string))
    {
        MCValueRelease(t_current_value_as_string);
        synchronize(ctxt, true);
        return true;
    }
    
    MCValueRelease(t_current_value_as_string);
    return false;
}

bool MCVariable::replace(MCExecContext& ctxt, MCValueRef p_replacement, MCRange p_range, MCNameRef *p_path, uindex_t p_length)
{
    if (MCValueGetTypeCode(p_replacement) == kMCValueTypeCodeData && can_become_data(ctxt, p_path, p_length))
        return replace_data(ctxt, (MCDataRef)p_replacement, p_range, p_path, p_length);
    
    MCAutoStringRef t_replacement;
    if (!ctxt . ConvertToString(p_replacement, &t_replacement))
        return false;
    
    return replace_string(ctxt, *t_replacement, p_range, p_path, p_length);
}

bool MCVariable::deleterange(MCExecContext& ctxt, MCRange p_range)
{
    if (value . type == kMCExecValueTypeDataRef)
        return replace_data(ctxt, kMCEmptyData, p_range, nil, 0);
    
    return replace_string(ctxt, kMCEmptyString, p_range, nil, 0);
}

bool MCVariable::deleterange(MCExecContext& ctxt, MCRange p_range, MCNameRef *p_path, uindex_t p_length)
{
    if (MCValueGetTypeCode(getvalueref(p_path, p_length, ctxt . GetCaseSensitive())) == kMCValueTypeCodeData)
        return replace_data(ctxt, kMCEmptyData, p_range, p_path, p_length);
    
    return replace_string(ctxt, kMCEmptyString, p_range, p_path, p_length);
}

bool MCVariable::remove(MCExecContext& ctxt)
{
	return remove(ctxt, nil, 0);
}

bool MCVariable::remove(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_length)
{
	if (p_length == 0)
	{
		clear();
		
		if (is_env)
		{
			if (!isdigit(MCNameGetCharAtIndex(*name, 1)) && MCNameGetCharAtIndex(*name, 1) != '#')
			{
				MCAutoStringRef t_env;
				/* UNCHECKED */ MCStringCopySubstring(MCNameGetString(*name), MCRangeMake(1, MCStringGetLength(MCNameGetString(*name))), &t_env);
				MCS_unsetenv(*t_env);
			}
		}
	}
    
	if (value . type != kMCExecValueTypeArrayRef)
		return true;
    
	if (!converttomutablearray())
		return false;
    
	MCArrayRemoveValueOnPath(value . arrayref_value, ctxt . GetCaseSensitive(), p_path, p_length);
    
	return true;
    
}


bool MCVariable::converttomutablearray(void)
{
	if (value . type == kMCExecValueTypeArrayRef)
	{
        if (!MCArrayIsMutable(value . arrayref_value))
        {
            MCArrayRef t_array;
            if (!MCArrayMutableCopyAndRelease(value . arrayref_value, t_array))
                return false;
            value . arrayref_value = t_array;
        }
	}
	else
	{
		MCArrayRef t_array;
		if (!MCArrayCreateMutable(t_array))
			return false;
		MCExecTypeRelease(value);
		MCExecTypeSetValueRef(value, t_array);
	}

	return true;
}

bool MCVariable::converttoarrayofstrings(MCExecContext& ctxt)
{
	return false;
}

bool MCVariable::converttomutablestring(MCExecContext& ctxt)
{
	if (value . type != kMCExecValueTypeStringRef)
	{
		MCStringRef t_string = nil;
        
        // If we have nothing stored, we don't try to convert - but we may need to release kMCNull in case it has been stored
        if (value . type != kMCExecValueTypeNone)
            MCExecTypeConvertAndReleaseAlways(ctxt, value . type, &value, kMCExecValueTypeStringRef, &t_string);
        else
            MCExecTypeRelease(value);
        
        if (t_string == nil || ctxt . HasError())
        {
            MCStringRef t_mutable_string;
            ctxt . IgnoreLastError();
            if (MCStringCreateMutable(0, t_mutable_string))
            {
                MCExecTypeRelease(value);
                MCExecTypeSetValueRef(value, t_mutable_string);
                return true;
            }
            
            return false;
        }
        else
        {
            MCExecTypeSetValueRef(value, t_string);
        }
	}
    
    if (!MCStringIsMutable(value . stringref_value))
    {
        MCStringRef t_mutable_string;
        if (MCStringMutableCopyAndRelease(value . stringref_value, t_mutable_string))
        {
            value . stringref_value = t_mutable_string;
            return true;
        }
        return false;
    }
    return true;
}

bool MCVariable::converttomutabledata(MCExecContext& ctxt)
{
	if (value . type != kMCExecValueTypeDataRef)
	{
		MCAutoStringRef t_string;
        
        // If we have nothing stored, we don't try to convert - but we may need to release kMCNull in case it has been stored
        if (value . type != kMCExecValueTypeNone)
            MCExecTypeConvertAndReleaseAlways(ctxt, value . type, &value, kMCExecValueTypeStringRef, &(&t_string));
        else
            MCExecTypeRelease(value);
        
        if (*t_string == nil || ctxt . HasError())
        {
            MCDataRef t_mutable_data;
            ctxt . IgnoreLastError();
            if (MCDataCreateMutable(0, t_mutable_data))
            {
                MCExecTypeRelease(value);
                MCExecTypeSetValueRef(value, t_mutable_data);
                return true;
            }
            
            return false;
        }
        else
        {
            MCDataRef t_data;
            if (!ctxt . ConvertToData(*t_string, t_data))
                return false;
            
            MCExecTypeSetValueRef(value, t_data);
        }
	}
    
    if (!MCDataIsMutable(value . dataref_value))
    {
        MCDataRef t_mutable_data;
        if (MCDataMutableCopyAndRelease(value . dataref_value, t_mutable_data))
        {
            value . dataref_value = t_mutable_data;
            return true;
        }
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MCVariable *MCVariable::lookupglobal_cstring(const char *p_name)
{
	// If we can't find an existing name, then there can be no global with
	// name 'p_name'.
	MCNameRef t_name;
	t_name = MCNameLookupWithCStringCaseless(p_name);
	if (t_name == nil)
		return nil;

	// The name is in use, so check to see if there is a global using it.
	return lookupglobal(t_name);
}

MCVariable *MCVariable::lookupglobal(MCNameRef p_name)
{
	// See if the global already exists.
	for(MCVariable *t_var = MCglobals; t_var != nil; t_var = t_var -> next)
		if (t_var -> hasname(p_name))
			return t_var;

	return nil;
}

bool MCVariable::ensureglobal(MCNameRef p_name, MCVariable*& r_var)
{
	// First check to see if the global variable already exists
	MCVariable *t_var;
	t_var = lookupglobal(p_name);
	if (t_var != nil)
	{
		r_var = t_var;
		return true;
	}

	// Didn't find one, so create a shiny new one!
	MCVariable *t_new_global;
	if (!createwithname(p_name, t_new_global))
		return false;

	if (MCNameGetCharAtIndex(p_name, 0) == '$')
  {
    MCAutoStringRef t_env;
    /* UNCHECKED */ MCStringCopySubstring(
      MCNameGetString(p_name),
      MCRangeMake(1, MCStringGetLength(MCNameGetString(p_name))), 
      &t_env
    );
        
    MCAutoStringRef t_value;
    if (MCS_getenv(*t_env, &t_value))
      t_new_global -> setvalueref(*t_value);

    t_new_global -> is_env = true;
  }

	t_new_global -> is_global = true;

	t_new_global -> next = MCglobals;
	MCglobals = t_new_global;

	r_var = t_new_global;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

#if 0
void MCVariable::synchronize(MCExecPoint& ep, Boolean notify)
{
	MCExecContext ctxt(ep);
	if (is_env)
	{
		if (!isdigit(MCNameGetCharAtIndex(*name, 1)) && MCNameGetCharAtIndex(*name, 1) != '#')
		{
			MCAutoStringRef t_string;
			if (ep . copyasstringref(&t_string))
			{
				MCAutoStringRef t_env;
				/* UNCHECKED */ MCStringCopySubstring(MCNameGetString(*name), MCRangeMake(1, MCStringGetLength(MCNameGetString(*name))), &t_env);
				MCS_setenv(*t_env, *t_string);
			}
		}
	}
	else if (is_msg)
	{
		eval(ep);
		MCAutoStringRef t_string;
		/* UNCHECPED */ ep.copyasstringref(&t_string);
		MCB_setmsg(ctxt, *t_string);
	}

	if (notify && MCnwatchedvars)
	{
		uint2 i;
		for (i = 0 ; i < MCnwatchedvars ; i++)
		{
			if ((MCwatchedvars[i].object == NULL || MCwatchedvars[i].object == ep.getobj()) &&
				(MCwatchedvars[i].handlername == NULL || ep.gethandler()->hasname(MCwatchedvars[i].handlername)) &&
				hasname(MCwatchedvars[i].varname))
			{
				// If this is a global watch (object == handlername == nil) then
				// check that this var is a global - if not carry on the search.
				if (MCwatchedvars[i] . object == NULL &&
					MCwatchedvars[i] . handlername == NULL &&
					!is_global)
					continue;

				// Otherwise, trigger the setvar message.
				eval(ep);
				MCAutoStringRef t_string;
				/* UNCHECKED */ ep.copyasstringref(&t_string);
				if (MCwatchedvars[i].expression != nil && !MCStringIsEmpty(MCwatchedvars[i].expression))
				{
					MCExecPoint ep2(ep);
					MCExecContext ctxt(ep2);
					MCAutoValueRef t_val;
					ctxt.GetHandler()->eval(ctxt, MCwatchedvars[i].expression, &t_val);
					
					MCAutoBooleanRef t_bool;
					if (!ctxt.HasError() && ctxt.ConvertToBoolean(*t_val, &t_bool) && *t_bool == kMCTrue)
						MCB_setvar(ctxt, *t_string, name);
				}
				else
					MCB_setvar(ctxt, *t_string, name);

				break;
			}
		}
	}
}
#endif

void MCVariable::synchronize(MCExecContext& ctxt, bool p_notify)
{
    MCExecValue t_value;
    MCAutoStringRef t_stringref_value;
	if (is_env)
	{
		if (!isdigit(MCNameGetCharAtIndex(*name, 1)) && MCNameGetCharAtIndex(*name, 1) != '#')
		{
            MCExecTypeCopy(value, t_value);
            MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeStringRef, &(&t_stringref_value));
            if (!ctxt . HasError())
            {
                MCAutoStringRef t_env;
				/* UNCHECKED */ MCStringCopySubstring(MCNameGetString(*name), MCRangeMake(1, MCStringGetLength(MCNameGetString(*name))), &t_env);
				MCS_setenv(*t_env, *t_stringref_value);
            }
		}
	}
	else if (is_msg)
    {
        MCExecTypeCopy(value, t_value);
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeStringRef, &(&t_stringref_value));
        if (!ctxt . HasError())
            MCB_setmsg(ctxt, *t_stringref_value);
	}
    
	if (p_notify && MCnwatchedvars)
	{
		uint2 i;
		for (i = 0 ; i < MCnwatchedvars ; i++)
		{
			if ((!MCwatchedvars[i].object.IsBound() || MCwatchedvars[i].object == ctxt.GetObject()) 
				&& (!MCwatchedvars[i].handlername.IsSet() 
					|| (ctxt.GetHandler() != nil && ctxt.GetHandler()->hasname(*MCwatchedvars[i].handlername)))
				&& hasname(*MCwatchedvars[i].varname))
			{
				// If this is a global watch (object == handlername == nil) then
				// check that this var is a global - if not carry on the search.
				if (!MCwatchedvars[i].object.IsBound() &&
					!MCwatchedvars[i].handlername.IsSet() &&
					!is_global)
					continue;

				if (MCwatchedvars[i].expression.IsSet() && !MCStringIsEmpty(*MCwatchedvars[i].expression))
				{
                    MCAutoValueRef t_val;
                    ctxt.eval(ctxt, *MCwatchedvars[i].expression, &t_val);
                    
					MCAutoBooleanRef t_bool;
					if (!ctxt.HasError() && ctxt.ConvertToBoolean(*t_val, &t_bool) && *t_bool == kMCTrue)
						MCB_setvalue(ctxt, value, *name);
				}
				else
                {
                    MCB_setvalue(ctxt, value, *name);
                }
                
				break;
			}
		}
	}
}

#if 0
Exec_stat MCVariable::remove(MCExecPoint& ep, Boolean notify)
{
	value . clear();
	
	if (is_env)
	{
		if (!isdigit(MCNameGetCharAtIndex(*name, 1)) && MCNameGetCharAtIndex(*name, 1) != '#')
			MCS_unsetenv(MCNameGetCString(*name) + 1);
	}

	return ES_NORMAL;
}
#endif

MCVarref *MCVariable::newvarref(void)
{
	if (!is_deferred)
		return new MCVarref(this);

	return new MCDeferredVarref(this);
}

////////////////////////////////////////////////////////////////////////////////

static MCAutoPointer<MCNameRef[]>
__join_paths(MCSpan<MCNameRef> p_base,
             MCSpan<MCNameRef> p_extra)
{
    MCAutoPointer<MCNameRef[]> t_result =
            new (nothrow) MCNameRef[p_base.size() + p_extra.size()];
    if (t_result)
    {
        for(int i = 0; i < p_base.size(); i++)
            t_result[i] = p_base[i];
    
        for(int i = 0; i < p_extra.size(); i++)
            t_result[i + p_base.size()] = p_extra[i];
    }
    
    return std::move(t_result);
}

MCContainer::~MCContainer(void)
{
    // AL-2014-09-17: [[ Bug 13465 ]] Delete path array regardless of length
	for(uindex_t i = 0; i < m_length; i++)
		MCValueRelease(m_path[i]);
	MCMemoryDeleteArray(m_path);
}

bool MCContainer::eval(MCExecContext& ctxt, MCValueRef& r_value)
{
    return m_variable -> eval(ctxt, m_path, m_length, r_value);
}

bool MCContainer::eval_on_path(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_path_length, MCValueRef& r_value)
{
    MCAutoPointer<MCNameRef[]> t_full_path =
            __join_paths(MCMakeSpan(m_path, m_length),
                         MCMakeSpan(p_path, p_path_length));
    
    if (!t_full_path)
        return false;
    
    return m_variable->eval(ctxt, *t_full_path, p_path_length + m_length, r_value);
}

bool MCContainer::set(MCExecContext& ctxt, MCValueRef p_value, MCVariableSettingStyle p_setting)
{
	return m_variable -> set(ctxt, p_value, m_path, m_length, p_setting);
}

bool MCContainer::set_on_path(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_path_length, MCValueRef p_value)
{
    MCAutoPointer<MCNameRef[]> t_full_path =
            __join_paths(MCMakeSpan(m_path, m_length),
                         MCMakeSpan(p_path, p_path_length));
    
    if (!t_full_path)
        return false;
    
    return m_variable->set(ctxt, p_value, *t_full_path, p_path_length + m_length);
}

bool MCContainer::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    return m_variable -> eval_ctxt(ctxt, m_path, m_length, r_value);
}

bool MCContainer::give_value(MCExecContext& ctxt, MCExecValue p_value, MCVariableSettingStyle p_setting)
{
	return m_variable -> give_value(ctxt, p_value, m_path, m_length, p_setting);
}

bool MCContainer::replace(MCExecContext &ctxt, MCValueRef p_replacement, MCRange p_range)
{
    return m_variable -> replace(ctxt, p_replacement, p_range, m_path, m_length);
}

bool MCContainer::deleterange(MCExecContext &ctxt, MCRange p_range)
{
    return m_variable -> deleterange(ctxt, p_range, m_path, m_length);
}

bool MCContainer::remove(MCExecContext& ctxt)
{
	return m_variable -> remove(ctxt, m_path, m_length);
}

bool MCContainer::set_valueref(MCValueRef p_value)
{
    m_variable->clearuql();
	return m_variable -> setvalueref(m_path, m_length, m_case_sensitive, p_value);
}

MCValueRef MCContainer::get_valueref()
{
	return m_variable -> getvalueref(m_path, m_length, m_case_sensitive);
}

bool MCContainer::clear(void)
{
	return set_valueref(kMCEmptyString);
}

bool MCContainer::set_real(double p_real)
{
    if (m_length == 0)
    {
        m_variable -> setnvalue(p_real);
        return true;
    }
    
	MCAutoNumberRef t_number;
	if (!MCNumberCreateWithReal(p_real, &t_number))
		return false;
	return set_valueref(*t_number);
}

bool MCContainer::createwithvariable(MCVariable *p_var, MCContainer& r_container)
{
	r_container.m_variable = p_var;
	r_container.m_length = 0;
    r_container.m_path = nil;
	r_container.m_case_sensitive = false;
	return true;
}

bool MCContainer::createwithpath(MCVariable *p_var, MCNameRef *p_path, uindex_t p_length, MCContainer& r_container)
{
	r_container.m_variable = p_var;
	r_container.m_path = p_path;
	r_container.m_length = p_length;
	r_container.m_case_sensitive = false;
	return true;
}

bool MCContainer::copywithpath(MCContainer *p_container, MCNameRef *p_path, uindex_t p_length, MCContainer& r_container)
{
	return createwithpath(p_container -> m_variable, p_path, p_length, r_container);
}

////////////////////////////////////////////////////////////////////////////////

MCVarref::~MCVarref()
{
    if (dimensions <= 1)
        delete exp;
    else
    {
        for(uint4 i = 0; i < dimensions; ++i)
            delete exps[i];
        MCMemoryDeleteArray(exps); /* Allocated with MCMemoryNewArray() */
    }
}

MCVariable *MCVarref::fetchvar(MCExecContext& ctxt)
{
	// MW-2009-01-28: [[ Inherited parentScripts ]]
	// If we are in parentScript context, then fetch the script local from there,
	// otherwise use the information stored in this.
	MCParentScriptUse *t_parentscript;
	t_parentscript = ctxt . GetParentScript();
	if (!isscriptlocal || t_parentscript == NULL)
	{
		if (ref != NULL)
			return ref;
        
		return handler -> getvar(index, isparam);
	}
	
	return t_parentscript -> GetVariable(index);
}

MCContainer *MCVarref::fetchcontainer(MCExecContext& ctxt)
{
	// MW-2009-01-28: [[ Inherited parentScripts ]]
	// If we are in parentScript context, then fetch the script local from there,
	// otherwise use the information stored in this.
	MCParentScriptUse *t_parentscript;
	t_parentscript = ctxt . GetParentScript();
	if (!isscriptlocal || t_parentscript == NULL)
		return handler -> getcontainer(index, isparam);
    
    return nil;
}

bool MCVarref::evalcontainer(MCExecContext& ctxt, MCContainer& r_container)
{
    return resolve(ctxt, r_container);
}

void MCVarref::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCVarref::compile_in(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCVarref::compile_out(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCVarref::compile_inout(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

MCVarref *MCVarref::getrootvarref(void)
{
	return this;
}

Parse_stat MCVarref::parsearray(MCScriptPoint &sp)
{
	Symbol_type type;

	for(;;)
	{
		if (sp.next(type) != PS_NORMAL)
			return PS_NORMAL;

		if (type != ST_LB)
		{
			sp.backup();
			return PS_NORMAL;
		}

		MCExpression *t_new_dimension;
		if (dimensions == 255 || sp.parseexp(False, True, &t_new_dimension) != PS_NORMAL)
		{
			MCperror->add(PE_VARIABLE_BADINDEX, sp);
			return PS_ERROR;
		}

		if (sp.next(type) != PS_NORMAL || type != ST_RB)
		{
			MCperror->add(PE_VARIABLE_NORBRACE, sp);
			return PS_ERROR;
		}

        if (dimensions == 0)
		{
			exp = t_new_dimension;
			dimensions = 1;
		}
		else if (dimensions == 1)
		{
			MCExpression *t_current_exp = exp;
			uindex_t t_dimensions = dimensions;
			if (!MCMemoryNewArray(2, exps, t_dimensions))
				return PS_ERROR;
			exps[0] = t_current_exp;
			exps[1] = t_new_dimension;
			dimensions = t_dimensions;
		}
		else
		{
			uindex_t t_dimensions = dimensions;
			if (!MCMemoryResizeArray(t_dimensions + 1, exps, t_dimensions))
				return PS_ERROR;
			exps[dimensions] = t_new_dimension;
			dimensions = t_dimensions;
		}
        
        // If we are pure, but the new index expression is not pure, then
        // we are not pure.
        if (m_is_pure &&
            !t_new_dimension->is_pure())
        {
            m_is_pure = false;
        }
	}

	return PS_NORMAL;
}

Exec_stat MCVarref::sets(const MCString &s)
{
	if (ref != NULL)
		return ref->sets(s);
	return handler->getvar(index, isparam)->sets(s);
}

void MCVarref::clear()
{
	if (ref != NULL)
		ref->clear(True);
	else
		handler->getvar(index, isparam)->clear(True);
}

void MCVarref::clearuql()
{
	if (ref != NULL)
		ref->clearuql();
	else if (!isparam)
		handler->getvar(index, isparam)->clearuql();
    // SN-2015-02-25: [[ Bug 14536 ]] Reinstate the UQL unsetting
    //  for the container as well.
    else
        handler->getcontainer(index, isparam)->getvar()->clearuql();
}

bool MCVarref::replace(MCExecContext &ctxt, MCValueRef p_replacement, MCRange p_range)
{
    MCContainer t_container;
    if (!evalcontainer(ctxt, t_container))
        return false;
    
    return t_container.replace(ctxt, p_replacement, p_range);
}

bool MCVarref::deleterange(MCExecContext &ctxt, MCRange p_range)
{
    MCContainer t_container;
    if (!evalcontainer(ctxt, t_container))
        return false;
    
    return t_container.deleterange(ctxt, p_range);
}

void MCVarref::getpath(MCExecContext& ctxt, MCNameRef*& r_path, uindex_t& r_length)
{
    if (!isparam)
    {
        r_path = nil;
        r_length = 0;
        return;
    }
    
    fetchcontainer(ctxt) -> getpath(r_path, r_length);
}

////////////////////////////////////////////////////////////////////////////////

template<typename Actor>
static bool
__MCVarrefDimensionAction(MCExecContext& ctxt,
                          Actor& p_actor,
                          MCValueRef p_key)
{
    if (MCValueGetTypeCode(p_key) == kMCValueTypeCodeArray)
    {
        MCArrayRef t_array_key =
        (MCArrayRef)p_key;
        
        // If the array is not a sequence, it is an error.
        if (!MCArrayIsSequence(t_array_key))
        {
            ctxt.LegacyThrow(EE_VARIABLE_BADINDEX);
            return false;
        }
        
        // Get the number of keys.
        uindex_t t_array_key_count =
        MCArrayGetCount(t_array_key);
        
        // Hint at the number of dimensions
        if (!p_actor.Hint(ctxt,
                          t_array_key_count - 1))
        {
            return false;
        }
        
        // Now iterate and recurse on each element of the sequence.
        for(uindex_t i = 1; i <= MCArrayGetCount(t_array_key); i++)
        {
            MCValueRef t_sub_key = nullptr;
            /* CANNOT_FAIL */ MCArrayFetchValueAtIndex(t_array_key,
                                                       i,
                                                       t_sub_key);
            
            if (!__MCVarrefDimensionAction(ctxt,
                                           p_actor,
                                           t_sub_key))
            {
                return false;
            }
        }
        
        return true;
    }
    
    MCNewAutoNameRef t_name_key;
    if (!ctxt.ConvertToName(p_key,
                            &t_name_key))
    {
        ctxt.LegacyThrow(EE_VARIABLE_BADINDEX);
        return false;
    }
    
    if (!p_actor.Continue(ctxt,
                          *t_name_key))
    {
        return false;
    }
    
    return true;
}

template<typename Actor>
void MCVarref::action(MCExecContext& ctxt,
                      Actor& p_actor)
{
    MCVariable *t_var = nullptr;
    MCNameRef *t_existing_path = nullptr;
    uindex_t t_existing_path_length = 0;
    if (!isparam)
    {
        t_var = fetchvar(ctxt);
    }
    else
    {
        MCContainer *t_container =
        fetchcontainer(ctxt);
        
        t_var = t_container->getvar();
        t_container->getpath(t_existing_path,
                             t_existing_path_length);
    }
    
    // Start the action
    if (!p_actor.Begin(ctxt,
                       t_var))
    {
        return;
    }
    
    // Hint at the number of upcoming dimensions.
    if (!p_actor.Hint(ctxt,
                      t_existing_path_length + dimensions))
    {
        return;
    }
    
    // Iterate over the existing path
    for(uindex_t i = 0; i < t_existing_path_length; i++)
    {
        if (!p_actor.Continue(ctxt,
                              t_existing_path[i]))
        {
            return;
        }
    }
    
    // Get the dimension array (which is a union tagged by dimensions).
    if (dimensions > 0)
    {
        MCExpression **t_dimensions =
        dimensions == 1 ? &exp : exps;
        
        // Iterate through each provided dimension.
        for(uindex_t i = 0; i < dimensions; i++)
        {
            // First evaluate the dimension expression to get the next key.
            MCAutoValueRef t_key;
            if (!ctxt.EvalExprAsValueRef(t_dimensions[i],
                                         EE_VARIABLE_BADINDEX,
                                         &t_key))
            {
                return;
            }
            
            // Now visit the key. As the key could be an array, it is possible for a
            // single dimension to give rise to multiple keys.
            if (!__MCVarrefDimensionAction(ctxt,
                                           p_actor,
                                           *t_key))
            {
                return;
            }
        }
    }
    
    // Complete the operation.
    if (!p_actor.End(ctxt))
    {
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////

class __MCVarrefPureFetchActor
{
public:
    __MCVarrefPureFetchActor(void)
        : m_value(nil)
    {
    }
    
    bool Hint(MCExecContext& ctxt,
              uindex_t p_extra_dimensions)
    {
        return true;
    }
    
    bool Begin(MCExecContext& ctxt,
               MCVariable* p_var)
    {
        m_value = p_var->getvalueref();
        return true;
    }
    
    bool Continue(MCExecContext& ctxt,
                  MCNameRef p_key)
    {
        // If m_value is not an array, or the array key is not present in it as
        // an array then we return null.
        if (MCValueGetTypeCode(m_value) != kMCValueTypeCodeArray ||
            !MCArrayFetchValue((MCArrayRef)m_value,
                               ctxt.GetCaseSensitive(),
                               p_key,
                               m_value))
        {
            m_value = kMCNull;
            return false;
        }
        
        return true;
    }
    
    bool End(MCExecContext& ctxt)
    {
        return true;
    }
    
    ////////
    
    bool Copy(MCValueRef& r_value)
    {
        return MCValueCopy(m_value,
                           r_value);
    }
    
private:
    MCValueRef m_value;
};

void MCVarref::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    if (m_is_pure)
    {
        __MCVarrefPureFetchActor t_actor;
        
        action(ctxt,
               t_actor);
        
        if (ctxt.HasError())
        {
            return;
        }
        
        if (!t_actor.Copy(r_value.valueref_value))
        {
            ctxt.Throw();
            return;
        }
        
        r_value.type = kMCExecValueTypeValueRef;
    }
    else
    {
        MCContainer t_container;
        if (evalcontainer(ctxt, t_container) &&
            t_container.eval_ctxt(ctxt, r_value))
            return;
        
        ctxt.Throw();
    }
}

////////////////////////////////////////////////////////////////////////////////

class __MCVarrefPureMutateActor
{
public:
    __MCVarrefPureMutateActor(void)
        : m_variable(nullptr),
          m_array(nullptr)
    {
    }
    
    bool Hint(MCExecContext& ctxt,
              uindex_t p_extra_dimensions)
    {
        return true;
    }
    
    bool Begin(MCExecContext& ctxt,
               MCVariable* p_var)
    {
        if (!p_var->converttomutablearray())
        {
            ctxt.Throw();
            return false;
        }
        
        m_variable = p_var;
        m_array = (MCArrayRef)p_var->getvalueref();
        
        return true;
    }
    
    bool Continue(MCExecContext& ctxt,
                  MCNameRef p_key)
    {
        if (*m_key == nullptr)
        {
            m_key.Reset(p_key);
            return true;
        }
        
        MCValueRef *t_slot_ptr;
        if (!MCArrayMutateValue(m_array,
                                ctxt.GetCaseSensitive(),
                                *m_key,
                                t_slot_ptr))
        {
            ctxt.Throw();
            return false;
        }
        
        MCArrayRef t_new_array = nullptr;
        if (MCValueGetTypeCode(*t_slot_ptr) == kMCValueTypeCodeArray)
        {
            MCArrayRef& t_array_slot = (MCArrayRef&)*t_slot_ptr;
            if (!MCArrayIsMutable(t_array_slot))
            {
                if (!MCArrayMutableCopyAndRelease(t_array_slot,
                                                  t_array_slot))
                {
                    ctxt.Throw();
                    return false;
                }
            }
            
            t_new_array = t_array_slot;
        }
        else
        {
            if (!MCArrayCreateMutable(t_new_array))
            {
                ctxt.Throw();
                return false;
            }
            
            MCValueAssignAndRelease(*t_slot_ptr,
                                    (MCValueRef)t_new_array);
        }
        
        m_array = t_new_array;
        m_key.Reset(p_key);
        
        return true;
    }
    
    bool End(MCExecContext& ctxt)
    {
        m_variable->synchronize(ctxt,
                                true);
        return true;
    }
    
protected:
    MCVariable *m_variable;
    MCArrayRef m_array;
    MCNewAutoNameRef m_key;
};

////////////////////////////////////////////////////////////////////////////////

class __MCVarrefPureStoreActor: public __MCVarrefPureMutateActor
{
public:
    __MCVarrefPureStoreActor(MCValueRef p_value)
        : m_value(p_value)
    {
    }
    
    bool End(MCExecContext& ctxt)
    {
        if (!MCArrayStoreValue(m_array,
                               ctxt.GetCaseSensitive(),
                               *m_key,
                               m_value))
        {
            ctxt.Throw();
            return false;
        }
        
        return __MCVarrefPureMutateActor::End(ctxt);
    }
    
private:
    MCValueRef m_value;
};

bool MCVarref::set(MCExecContext& ctxt, MCValueRef p_value, MCVariableSettingStyle p_setting)
{
    if (!isparam &&
        dimensions > 0 &&
        p_setting == kMCVariableSetInto &&
        m_is_pure)
    {
        __MCVarrefPureStoreActor t_actor(p_value);
        
        action(ctxt,
               t_actor);
        
        return ctxt.HasError();
    }
    else
    {
        MCContainer t_container;
        if (!evalcontainer(ctxt, t_container))
            return false;
        
        return t_container.set(ctxt, p_value, p_setting);
    }
}

bool MCVarref::give_value(MCExecContext& ctxt, MCExecValue p_value, MCVariableSettingStyle p_setting)
{
    if (!isparam &&
        dimensions > 0 &&
        p_setting == kMCVariableSetInto &&
        m_is_pure)
    {
        MCAutoValueRef t_boxed_value;
        MCExecTypeConvertAndReleaseAlways(ctxt,
                                          p_value.type,
                                          &p_value,
                                          kMCExecValueTypeValueRef,
                                          &(&t_boxed_value));
        if (ctxt.HasError())
        {
            return false;
        }

        __MCVarrefPureStoreActor t_actor(*t_boxed_value);
        
        action(ctxt,
               t_actor);
        
        return ctxt.HasError();
    }
    else
    {
        MCContainer t_container;
        if (!evalcontainer(ctxt, t_container))
            return false;
        
        return t_container.give_value(ctxt, p_value, p_setting);
    }
}

////////////////////////////////////////////////////////////////////////////////

class __MCVarrefPureRemoveActor: public __MCVarrefPureMutateActor
{
public:
    bool End(MCExecContext& ctxt)
    {
        if (!MCArrayRemoveValue(m_array,
                                ctxt.GetCaseSensitive(),
                                *m_key))
        {
            ctxt.Throw();
            return false;
        }
        
        return __MCVarrefPureMutateActor::End(ctxt);
    }
};

// MW-2008-08-18: [[ Bug 6945 ]] Cannot delete a nested array key.
bool MCVarref::dofree(MCExecContext& ctxt)
{
    /*if (!isparam &&
        dimensions > 0 &&
        m_is_pure)
    {
        __MCVarrefPureRemoveActor t_actor;
        
        action(ctxt,
               t_actor);
        
        return ctxt.HasError();
    }
    else*/
    {
        MCContainer t_container;
        if (!resolve(ctxt, t_container))
            return false;
    
        return t_container.remove(ctxt);
    }
}

////////////////////////////////////////////////////////////////////////////////

class __MCVarrefResolveActor
{
public:
    bool Hint(MCExecContext& ctxt,
              uindex_t p_extra_dimensions)
    {
        return m_path.Ensure(p_extra_dimensions);
    }
    
    bool Begin(MCExecContext& ctxt,
               MCVariable* p_var)
    {
        m_variable = p_var;
        return true;
    }
    
    bool Continue(MCExecContext& ctxt,
                  MCNameRef p_key)
    {
        m_path.Push(p_key);
        return true;
    }
    
    bool End(MCExecContext& ctxt)
    {
        return true;
    }
    
    //////////
    
    bool Defer(MCExecContext& ctxt,
               MCContainer& r_container)
    {
        MCNameRef *t_path = nullptr;
        uindex_t t_path_length = 0;
        m_path.Take(t_path,
                    t_path_length);
        
        MCContainer::createwithpath(m_variable,
                                    t_path,
                                    t_path_length,
                                    r_container);
        
        m_variable = nullptr;
        
        return true;
    }
    
private:
    MCVariable *m_variable;
    MCAutoNameRefArray m_path;
};

bool MCVarref::resolve(MCExecContext& ctxt,
                       MCContainer& r_container)
{
    __MCVarrefResolveActor t_actor;
    action(ctxt,
           t_actor);
    if (ctxt.HasError())
    {
        return false;
    }
    return t_actor.Defer(ctxt,
                         r_container);
}

////////////////////////////////////////////////////////////////////////////////

bool MCDeferredVariable::createwithname(MCNameRef p_name, MCDeferredVariableComputeCallback p_callback, void *p_context, MCVariable*& r_var)
{
	MCDeferredVariable *self;
	self = new (nothrow) MCDeferredVariable;
	if (self == nil)
		return false;

	self -> next = nil;
	self -> name = MCValueRetain(p_name);

	self -> value . type = kMCExecValueTypeNone;
    self -> value . valueref_value = nil;
	self -> is_msg = false;
	self -> is_env = false;
	self -> is_global = false;
	self -> is_deferred = true;
	self -> is_uql = false;

	self -> m_callback = p_callback;
	self -> m_context = p_context;

	r_var = self;

	return true;
}

bool MCDeferredVariable::compute(void)
{
	// Compute can only be called once. By setting deferred to false here, any
	// future access of the variable via an MCDeferredVarref will result in this
	// not being called.
	is_deferred = false;

    return m_callback(m_context, this);
}

void MCDeferredVarref::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    bool t_error;
    if (ref -> isdeferred())
        t_error = static_cast<MCDeferredVariable *>(ref) -> compute() != ES_NORMAL;
    else
        t_error = false;

    if (!t_error)
        MCVarref::eval_ctxt(ctxt, r_value);
    else
        ctxt . Throw();
}

bool MCDeferredVarref::evalcontainer(MCExecContext &ctxt, MCContainer& r_container)
{
    bool t_error = false;
    if (ref -> isdeferred())
        t_error = static_cast<MCDeferredVariable *>(ref) -> compute() != ES_NORMAL;

    if (!t_error)
        t_error = MCVarref::evalcontainer(ctxt, r_container);

    return t_error;
}

////////////////////////////////////////////////////////////////////////////////
