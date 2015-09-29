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

extern "C" MC_DLLEXPORT_DEF void MCArrayEvalKeysOf(MCArrayRef p_target, MCProperListRef& r_output)
{
    MCProperListRef t_list;
    if (MCProperListCreateMutable(t_list) &&
        MCArrayApply(p_target, list_array_keys, t_list) &&
        MCProperListCopyAndRelease(t_list, r_output))
        return;
}

extern "C" MC_DLLEXPORT_DEF void MCArrayEvalElementsOf(MCArrayRef p_target, MCProperListRef& r_output)
{
    MCProperListRef t_list;
    if (MCProperListCreateMutable(t_list) &&
        MCArrayApply(p_target, list_array_elements, t_list) &&
        MCProperListCopyAndRelease(t_list, r_output))
        return;
}

extern "C" MC_DLLEXPORT_DEF void MCArrayEvalNumberOfElementsIn(MCArrayRef p_target, uindex_t& r_output)
{
    r_output = MCArrayGetCount(p_target);
}

extern "C" MC_DLLEXPORT_DEF void MCArrayEvalIsAmongTheElementsOf(MCValueRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    MCValueRef t_value;
    t_value = p_needle != nil ? p_needle : kMCNull;
    r_output = !MCArrayApply(p_target, is_not_among_the_elements_of, t_value);
}

extern "C" MC_DLLEXPORT_DEF void MCArrayEvalIsAmongTheKeysOfCaseless(MCStringRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    MCNewAutoNameRef t_key;
    if (!create_key_for_array(p_needle, p_target, &t_key))
        return;
    
    MCValueRef t_value;
    t_value = nil;
    
    r_output = MCArrayFetchValue(p_target, MCArrayIsCaseSensitive(p_target), *t_key, t_value);
    
    if (p_is_not)
        r_output = !r_output;
}

extern "C" MC_DLLEXPORT_DEF void MCArrayFetchElementOfCaseless(MCArrayRef p_target, MCStringRef p_key, MCValueRef& r_output)
{
    MCNewAutoNameRef t_key;
    
    if (!create_key_for_array(p_key, p_target, &t_key))
        return;
    
    MCValueRef t_value;
    t_value = nil;
    if (!MCArrayFetchValue(p_target, MCArrayIsCaseSensitive(p_target), *t_key, t_value))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("array key does not exist"), nil);
        return;
    }
    
    r_output = MCValueRetain(t_value);
}

extern "C" MC_DLLEXPORT_DEF void MCArrayStoreElementOfCaseless(MCValueRef p_value, MCArrayRef& x_target, MCStringRef p_key)
{
    MCNewAutoNameRef t_key;
    MCAutoArrayRef t_array;
    MCArrayMutableCopy(x_target, &t_array);
    
    MCValueRef t_value;
    t_value = p_value != nil ? p_value : kMCNull;
    
    if (!create_key_for_array(p_key, x_target, &t_key) ||
        !MCArrayStoreValue(*t_array, MCArrayIsCaseSensitive(*t_array), *t_key, t_value))
        return;
    
    MCAutoArrayRef t_new_array;
    if (!MCArrayCopy(*t_array, &t_new_array))
        return;
    
    MCValueAssign(x_target, *t_new_array);
}

extern "C" MC_DLLEXPORT_DEF void MCArrayDeleteElementOfCaseless(MCArrayRef& x_target, MCStringRef p_key)
{
    MCNewAutoNameRef t_key;
    MCAutoArrayRef t_array;
    MCArrayMutableCopy(x_target, &t_array);
    
    if (!create_key_for_array(p_key, x_target, &t_key) ||
        !MCArrayRemoveValue(*t_array, MCArrayIsCaseSensitive(*t_array), *t_key))
        return;
    
    MCAutoArrayRef t_new_array;
    if (!MCArrayCopy(*t_array, &t_new_array))
        return;
    
    MCValueAssign(x_target, *t_new_array);
}

extern "C" MC_DLLEXPORT_DEF void MCArrayEvalEmpty(MCArrayRef& r_output)
{
    r_output = MCValueRetain(kMCEmptyArray);
}

extern "C" bool MC_DLLEXPORT_DEF MCArrayRepeatForEachElement(void*& x_iterator, MCValueRef& r_iterand, MCArrayRef p_array)
{
    MCValueRef t_value;
    // If this is a numerical array, do it in order
/*    if (MCArrayIsSequence(p_array))
    {
        uindex_t t_offset;
        t_offset = (uindex_t)x_iterator;
        
        if (t_offset == MCArrayGetCount(p_array))
            return false;
        
        if (!MCArrayFetchValueAtIndex(p_array, t_offset, t_value))
            return false;
    }
    else */
    {
        MCNameRef t_key;
        
        uintptr_t t_ptr;
        t_ptr = (uintptr_t)x_iterator;
        
        if (!MCArrayIterate(p_array, t_ptr, t_key, t_value))
            return false;
        
        x_iterator = (void *)(t_ptr);
    }
    
    r_iterand = MCValueRetain(t_value);
    return true;
}

extern "C" bool MC_DLLEXPORT_DEF MCArrayRepeatForEachKey(void*& x_iterator, MCStringRef& r_iterand, MCArrayRef p_array)
{
    MCNameRef t_key;
    MCValueRef t_value;
    
    uintptr_t t_offset;
    t_offset = (uintptr_t)x_iterator;
    
    if (!MCArrayIterate(p_array, t_offset, t_key, t_value))
        return false;
    
    r_iterand = MCValueRetain(MCNameGetString(t_key));
    
    x_iterator = (void *)(t_offset);
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_array_Initialize(void)
{
    return true;
}

extern "C" void com_livecode_array_Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
