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
    
    if (MCArrayIsFormSensitive(p_target) && !MCStringIsNative(p_string))
    {
        t_success = MCStringCreateWithBytes((const byte_t *)MCStringGetCharPtr(p_string), MCStringGetLength(p_string) * 2, kMCStringEncodingNative, false, &t_key_string);
    }
    else
        t_key_string = p_string;
    
    
    if (t_success)
        t_success = MCNameCreate(*t_key_string, r_key);
    
    return t_success;
}

static bool create_key_for_matrix(MCProperListRef p_list, MCNameRef*& r_path, uindex_t& r_length)
{
    bool t_success;
    t_success = true;
    
    MCAutoArray<MCNameRef> t_path;
    for (uindex_t i = 0; t_success && i < MCProperListGetLength(p_list); i++)
    {
        MCValueRef t_list_element;
        t_list_element = MCProperListFetchElementAtIndex(p_list, i);
        
        t_success = MCValueGetTypeCode(t_list_element) == kMCValueTypeCodeNumber
        && MCNumberIsInteger((MCNumberRef)t_list_element);
        
        MCAutoStringRef t_string;
        MCNameRef t_key;
        t_key = nil;
        if (t_success)
            t_success = MCStringFormat(&t_string, "%d", MCNumberFetchAsInteger((MCNumberRef)t_list_element))
            && MCNameCreate(*t_string, t_key);
        
        if (t_success)
            t_success = t_path . Push(t_key);
        
        if (!t_success)
            MCValueRelease(t_key);
    }
    
    t_path . Take(r_path, r_length);
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
    return MCProperListPushElement(t_list, p_key);
}

static bool list_array_elements(void *context, MCArrayRef p_target, MCNameRef p_key, MCValueRef p_value)
{
    MCProperListRef t_list = (MCProperListRef)context;
    return MCProperListPushElement(t_list, p_value);
}

void MCMapEvalKeysOf(MCArrayRef p_target, MCProperListRef& r_output)
{
    MCProperListRef t_list;
    if (MCProperListCreateMutable(t_list) &&
        MCArrayApply(p_target, list_array_keys, t_list) &&
        MCProperListCopyAndRelease(t_list, r_output))
        return;

    // ctxt . Throw()
}

void MCMapEvalElementsOf(MCArrayRef p_target, MCProperListRef& r_output)
{
    MCProperListRef t_list;
    if (MCProperListCreateMutable(t_list) &&
        MCArrayApply(p_target, list_array_elements, t_list) &&
        MCProperListCopyAndRelease(t_list, r_output))
        return;
    
    // ctxt . Throw()
}

void MCMapEvalNumberOfElementsIn(MCArrayRef p_target, uindex_t& r_output)
{
    r_output = MCArrayGetCount(p_target);
}

void MCMapEvalIsAmongTheElementsOf(MCValueRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    r_output = !MCArrayApply(p_target, is_not_among_the_elements_of, p_needle);
}

void MCMapEvalIsAmongTheKeysOf(MCStringRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    MCNewAutoNameRef t_key;
    create_key_for_array(p_needle, p_target, &t_key);
    
    MCValueRef t_value;
    t_value = nil;
    
    r_output = MCArrayFetchValue(p_target, MCArrayIsCaseSensitive(p_target), *t_key, t_value);
}

void MCMapEvalIsAmongTheKeysOfNumeric(integer_t p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    MCAutoStringRef t_string;
    if (MCStringFormat(&t_string, "%d", p_needle))
    {
        MCMapEvalIsAmongTheKeysOf(*t_string, p_is_not, p_target, r_output);
        return;
    }
    
    // ctxt . Throw
}

void MCMapEvalIsAmongTheKeysOfMatrix(MCProperListRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    bool t_output;
    t_output = false;

    MCNameRef *t_path;
    uindex_t t_path_length;
    if (create_key_for_matrix(p_needle, t_path, t_path_length))
    {
        MCValueRef t_value;
        t_value = nil;
        t_output = MCArrayFetchValueOnPath(p_target, true, t_path, t_path_length, t_value);
    }
    
    for (uindex_t i = 0; i < t_path_length; i++)
        MCValueRelease(t_path[i]);
    
    MCMemoryDeleteArray(t_path);
    
    r_output = t_output;
}

void MCMapFetchElementOfBinary(MCArrayRef p_target, MCDataRef p_key, MCValueRef& r_output)
{
    MCAutoStringRef t_key_string;
    MCNewAutoNameRef t_key;
    
    if (MCStringCreateWithBytes(MCDataGetBytePtr(p_key), MCDataGetLength(p_key), kMCStringEncodingNative, false, &t_key_string) &&
        MCNameCreate(*t_key_string, &t_key) &&
        MCArrayFetchValue(p_target, false, *t_key, r_output))
    {
        MCValueRetain(r_output);
        return;
    }
}

void MCMapStoreElementOfBinary(MCValueRef p_value, MCArrayRef& x_target, MCDataRef p_key)
{
    MCAutoStringRef t_key_string;
    MCNewAutoNameRef t_key;
    
    if (MCStringCreateWithBytes(MCDataGetBytePtr(p_key), MCDataGetLength(p_key), kMCStringEncodingNative, false, &t_key_string) &&
        MCNameCreate(*t_key_string, &t_key) &&
        MCArrayStoreValue(x_target, false, *t_key, p_value))
        return;
    
    // ctxt . Throw()
}

void MCMapFetchElementOf(MCArrayRef p_target, MCStringRef p_key, MCValueRef& r_output)
{
    MCNewAutoNameRef t_key;
    
    if (create_key_for_array(p_key, p_target, &t_key) &&
        MCArrayFetchValue(p_target, MCArrayIsCaseSensitive(p_target), *t_key, r_output))
    {
        MCValueRetain(r_output);
        return;
        
    }

    // ctxt . Throw();
}

void MCMapStoreElementOf(MCValueRef p_value, MCArrayRef& x_target, MCStringRef p_key)
{
    MCNewAutoNameRef t_key;
    
    if (create_key_for_array(p_key, x_target, &t_key) &&
        MCArrayStoreValue(x_target, MCArrayIsCaseSensitive(x_target), *t_key, p_value))
        return;
    
    // ctxt . Throw();
}

void MCMapFetchElementOfNumeric(MCArrayRef p_target, integer_t p_key, MCValueRef& r_output)
{
    MCAutoStringRef t_string;
    if (MCStringFormat(&t_string, "%d", p_key))
    {
        MCMapFetchElementOf(p_target, *t_string, r_output);
        return;
    }
    
    // ctxt . Throw
}

void MCMapStoreElementOfNumeric(MCValueRef p_value, MCArrayRef& x_target, integer_t p_key)
{
    MCAutoStringRef t_string;
    if (MCStringFormat(&t_string, "%d", p_key))
    {
        MCMapStoreElementOf(p_value, x_target, *t_string);
        return;
    }
    
    // ctxt . Throw
}

void MCMapFetchElementOfMatrix(MCArrayRef p_target, MCProperListRef p_key, MCValueRef& r_output)
{
    bool t_found;
    
    MCNameRef *t_path;
    uindex_t t_path_length;
    t_found = create_key_for_matrix(p_key, t_path, t_path_length) &&
              MCArrayFetchValueOnPath(p_target, true, t_path, t_path_length, r_output);

    for (uindex_t i = 0; i < t_path_length; i++)
        MCValueRelease(t_path[i]);
    
    MCMemoryDeleteArray(t_path);
    
   if (t_found)
   {
       MCValueRetain(r_output);
       return;
   }
    
    // ctxt . Throw()
}

void MCMapStoreElementOfMatrix(MCValueRef p_value, MCArrayRef& x_target, MCProperListRef p_key)
{
    bool t_success;
    
    MCNameRef *t_path;
    uindex_t t_path_length;
    t_success = create_key_for_matrix(p_key, t_path, t_path_length) &&
                MCArrayStoreValueOnPath(x_target, true, t_path, t_path_length, p_value);

    for (uindex_t i = 0; i < t_path_length; i++)
        MCValueRelease(t_path[i]);
    
    MCMemoryDeleteArray(t_path);
    
    if (t_success)
        return;
    
    // ctxt . Throw()
}