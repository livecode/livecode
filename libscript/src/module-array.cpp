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

#include <foundation.h>
#include <foundation-auto.h>

static bool create_key_for_array(MCStringRef p_string, MCArrayRef p_target, MCNameRef& r_key)
{
    MCAutoStringRef t_key_string;
    
    bool t_success;
    t_success = true;

    /*
    if (MCArrayIsFormSensitive(p_target) && !MCStringIsNative(p_string))
    {
        t_success = MCStringCreateWithBytes((const byte_t *)MCStringGetCharPtr(p_string), MCStringGetLength(p_string) * 2, kMCStringEncodingNative, false, &t_key_string);
    }
    else
     */
        t_key_string = p_string;
    
    
    if (t_success)
        t_success = MCNameCreate(*t_key_string, r_key);
    
    return t_success;
}

static bool is_not_among_the_elements_of(void *context, MCArrayRef p_target, MCNameRef p_key, MCValueRef p_value)
{
    MCValueRef t_needle = (MCValueRef)context;
    return !MCValueIsEqualTo(t_needle, p_value);
}

static bool list_array_keys(void *context, MCArrayRef p_target, MCNameRef p_key, MCValueRef p_value)
{
    MCProperListRef t_list = (MCProperListRef)context;
    return MCProperListPushElementOntoBack(t_list, MCNameGetString(p_key));
}

static bool list_array_elements(void *context, MCArrayRef p_target, MCNameRef p_key, MCValueRef p_value)
{
    MCProperListRef t_list = (MCProperListRef)context;
    return MCProperListPushElementOntoBack(t_list, p_value);
}

extern "C" void MCArrayEvalKeysOf(MCArrayRef p_target, MCProperListRef& r_output)
{
    MCProperListRef t_list;
    if (MCProperListCreateMutable(t_list) &&
        MCArrayApply(p_target, list_array_keys, t_list) &&
        MCProperListCopyAndRelease(t_list, r_output))
        return;

    // ctxt . Throw()
}

extern "C" void MCArrayEvalElementsOf(MCArrayRef p_target, MCProperListRef& r_output)
{
    MCProperListRef t_list;
    if (MCProperListCreateMutable(t_list) &&
        MCArrayApply(p_target, list_array_elements, t_list) &&
        MCProperListCopyAndRelease(t_list, r_output))
        return;
    
    // ctxt . Throw()
}

extern "C" void MCArrayEvalNumberOfElementsIn(MCArrayRef p_target, uindex_t& r_output)
{
    r_output = MCArrayGetCount(p_target);
}

extern "C" void MCArrayEvalIsAmongTheElementsOf(MCValueRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    r_output = !MCArrayApply(p_target, is_not_among_the_elements_of, p_needle);
}

extern "C" void MCArrayEvalIsAmongTheKeysOf(MCStringRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    MCNewAutoNameRef t_key;
    create_key_for_array(p_needle, p_target, &t_key);
    
    MCValueRef t_value;
    t_value = nil;
    
    r_output = MCArrayFetchValue(p_target, MCArrayIsCaseSensitive(p_target), *t_key, t_value);
    
    if (p_is_not)
        r_output = !r_output;
}

extern "C" void MCArrayFetchElementOf(MCArrayRef p_target, MCStringRef p_key, MCValueRef& r_output)
{
    MCNewAutoNameRef t_key;
    
    if (create_key_for_array(p_key, p_target, &t_key) &&
        MCArrayFetchValue(p_target, MCArrayIsCaseSensitive(p_target), *t_key, r_output))
    {
        MCValueRetain(r_output);
        return;
        
    }
}

extern "C" void MCArrayStoreElementOf(MCValueRef p_value, MCArrayRef& x_target, MCStringRef p_key)
{
    MCNewAutoNameRef t_key;
    MCAutoArrayRef t_array;
    MCArrayMutableCopy(x_target, &t_array);
    
    if (!create_key_for_array(p_key, x_target, &t_key) ||
        !MCArrayStoreValue(*t_array, MCArrayIsCaseSensitive(*t_array), *t_key, p_value))
        return;
    
    MCAutoArrayRef t_new_array;
    if (!MCArrayCopy(*t_array, &t_new_array))
        return;
    
    MCValueAssign(x_target, *t_new_array);
}

extern "C" void MCArrayEvalEmpty(MCArrayRef& r_output)
{
    r_output = MCValueRetain(kMCEmptyArray);
}