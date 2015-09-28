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
    return MCProperListPushElementOntoBack(t_list, MCNameGetString(p_key));
}

static bool list_array_elements(void *context, MCArrayRef p_target, MCNameRef p_key, MCValueRef p_value)
{
    MCProperListRef t_list = (MCProperListRef)context;
    return MCProperListPushElementOntoBack(t_list, p_value);
}

extern "C" void MCMapEvalKeysOf(MCArrayRef p_target, MCProperListRef& r_output)
{
    MCProperListRef t_list;
    if (MCProperListCreateMutable(t_list) &&
        MCArrayApply(p_target, list_array_keys, t_list) &&
        MCProperListCopyAndRelease(t_list, r_output))
        return;

    // ctxt . Throw()
}

extern "C" void MCMapEvalElementsOf(MCArrayRef p_target, MCProperListRef& r_output)
{
    MCProperListRef t_list;
    if (MCProperListCreateMutable(t_list) &&
        MCArrayApply(p_target, list_array_elements, t_list) &&
        MCProperListCopyAndRelease(t_list, r_output))
        return;
    
    // ctxt . Throw()
}

extern "C" void MCMapEvalNumberOfElementsIn(MCArrayRef p_target, uindex_t& r_output)
{
    r_output = MCArrayGetCount(p_target);
}

extern "C" void MCMapEvalIsAmongTheElementsOf(MCValueRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    r_output = !MCArrayApply(p_target, is_not_among_the_elements_of, p_needle);
}

extern "C" void MCMapEvalIsAmongTheKeysOf(MCStringRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    MCNewAutoNameRef t_key;
    create_key_for_array(p_needle, p_target, &t_key);
    
    MCValueRef t_value;
    t_value = nil;
    
    r_output = MCArrayFetchValue(p_target, MCArrayIsCaseSensitive(p_target), *t_key, t_value);
    
    if (p_is_not)
        r_output = !r_output;
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
    
    if (p_is_not)
        t_output = !t_output;
    
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
    
    MCAutoArrayRef t_array;
    MCArrayMutableCopy(x_target, &t_array);
    
    if (!MCStringCreateWithBytes(MCDataGetBytePtr(p_key), MCDataGetLength(p_key), kMCStringEncodingNative, false, &t_key_string) ||
        !MCNameCreate(*t_key_string, &t_key) ||
        !MCArrayStoreValue(*t_array, false, *t_key, p_value))
        return;
    
    MCAutoArrayRef t_new_array;
    if (!MCArrayCopy(*t_array, &t_new_array))
        return;
    
    MCValueAssign(x_target, *t_new_array);
}

extern "C" void MCMapFetchElementOf(MCArrayRef p_target, MCStringRef p_key, MCValueRef& r_output)
{
    MCNewAutoNameRef t_key;
    
    if (create_key_for_array(p_key, p_target, &t_key) &&
        MCArrayFetchValue(p_target, MCArrayIsCaseSensitive(p_target), *t_key, r_output))
    {
        MCValueRetain(r_output);
        return;
        
    }
}

extern "C" void MCMapStoreElementOf(MCValueRef p_value, MCArrayRef& x_target, MCStringRef p_key)
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
    
    MCNewAutoNameRef t_key;
    MCAutoArrayRef t_array;
    MCArrayMutableCopy(x_target, &t_array);
    
    MCNameRef *t_path;
    uindex_t t_path_length;
    t_success = create_key_for_matrix(p_key, t_path, t_path_length) &&
                MCArrayStoreValueOnPath(*t_array, true, t_path, t_path_length, p_value);

    for (uindex_t i = 0; i < t_path_length; i++)
        MCValueRelease(t_path[i]);
    
    MCMemoryDeleteArray(t_path);
    
    if (!t_success)
        return;
    
    MCAutoArrayRef t_new_array;
    if (!MCArrayCopy(*t_array, &t_new_array))
        return;
    
    MCValueAssign(x_target, *t_new_array);
}

#ifdef _TEST
extern void log(const char *module, const char *test, bool result);
#define log_result(test, result) log("MAP MODULE", test, result)
void MCMapRunTests()
{
/*
    MCMapFetchElementOfBinary(MCArrayRef p_target, MCDataRef p_key, MCValueRef& r_output)
    MCMapStoreElementOfBinary(MCValueRef p_value, MCArrayRef& x_target, MCDataRef p_key)
    MCMapFetchElementOf(MCArrayRef p_target, MCStringRef p_key, MCValueRef& r_output)
    MCMapStoreElementOf(MCValueRef p_value, MCArrayRef& x_target, MCStringRef p_key)
    MCMapFetchElementOfNumeric(MCArrayRef p_target, integer_t p_key, MCValueRef& r_output)
    MCMapStoreElementOfNumeric(MCValueRef p_value, MCArrayRef& x_target, integer_t p_key)
 */
    
    MCAutoArrayRef t_array, t_array_for_keys;
    MCAutoArrayRef t_caseless, t_cs_array, t_cs_fs_array, t_fs_array;
    MCArrayCreateMutable(&t_array);
    
    MCNewAutoNameRef t_1, t_2, t_3;
    MCNameCreateWithNativeChars((const char_t *)"1", 1, &t_1);
    MCNameCreateWithNativeChars((const char_t *)"2", 1, &t_2);
    MCNameCreateWithNativeChars((const char_t *)"3", 1, &t_3);
    
    MCAutoNumberRef t_1n, t_2n, t_3n;
    MCNumberCreateWithInteger(1, &t_1n);
    MCNumberCreateWithInteger(2, &t_2n);
    MCNumberCreateWithInteger(3, &t_3n);
    
    MCAutoArray<MCNameRef> t_path;
    t_path . Push(*t_1);
    t_path . Push(*t_2);
    t_path . Push(*t_3);
    
    MCAutoArray<MCValueRef> t_list_elts;
    t_list_elts . Push(*t_1n);
    t_list_elts . Push(*t_2n);
    t_list_elts . Push(*t_3n);
    
    unichar_t t_chars[6] = {0xE1, 'a', 0x301, 0xC1, 'A', 0x301};
    
    MCAutoStringRef t_lc_norm_str, t_uc_norm_str, t_lc_decomp_str, t_uc_decomp_str;
    MCStringCreateWithChars((const unichar_t *)t_chars, 1, &t_lc_norm_str);
    MCStringCreateWithChars((const unichar_t *)&t_chars[1], 2, &t_lc_decomp_str);
    MCStringCreateWithChars((const unichar_t *)&t_chars[3], 1, &t_uc_norm_str);
    MCStringCreateWithChars((const unichar_t *)&t_chars[4], 2, &t_uc_decomp_str);
    
    MCNewAutoNameRef t_lc_norm, t_uc_norm, t_lc_decomp, t_uc_decomp;
    MCNameCreate(*t_lc_norm_str, &t_lc_norm);
    MCNameCreate(*t_uc_norm_str, &t_uc_norm);
    MCNameCreate(*t_lc_decomp_str, &t_lc_decomp);
    MCNameCreate(*t_uc_decomp_str, &t_uc_decomp);
    
    MCAutoArray<MCNameRef> t_keys;
    t_keys . Push(*t_lc_norm);
    t_keys . Push(*t_lc_decomp);
    t_keys . Push(*t_uc_norm);
    t_keys . Push(*t_uc_decomp);
    
    MCAutoArray<MCStringRef> t_strings;
    t_strings . Push(*t_lc_norm_str);
    t_strings . Push(*t_lc_decomp_str);
    t_strings . Push(*t_uc_norm_str);
    t_strings . Push(*t_uc_decomp_str);
    
    MCNameRef t_key = *t_lc_norm;
    MCArrayCreateWithOptions(false, false, &t_key, t_list_elts . Ptr(), 1, &t_caseless);
    t_key = *t_lc_decomp;
    MCArrayCreateWithOptions(false, true, &t_key, t_list_elts . Ptr(), 1, &t_fs_array);
    t_key = *t_uc_norm;
    MCArrayCreateWithOptions(true, false, &t_key, t_list_elts . Ptr(), 1, &t_cs_array);
    t_key = *t_uc_decomp;
    MCArrayCreateWithOptions(true, true, &t_key, t_list_elts . Ptr(), 1, &t_cs_fs_array);
    
    
    /* MCMapEvalIsAmongTheKeysOf(MCStringRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output) */
    bool t_none, t_cs, t_fs, t_cs_fs;
    MCMapEvalIsAmongTheKeysOf(*t_lc_norm_str, false, *t_caseless, t_none);
    MCMapEvalIsAmongTheKeysOf(*t_uc_norm_str, false, *t_caseless, t_cs);
    MCMapEvalIsAmongTheKeysOf(*t_lc_decomp_str, false, *t_caseless, t_fs);
    MCMapEvalIsAmongTheKeysOf(*t_uc_decomp_str, false, *t_caseless, t_cs_fs);
    
    log_result("caseless map, uncased string", t_none);
    log_result("caseless map, cased string", t_cs);
    log_result("caseless map, uncased decomp string", t_fs);
    log_result("caseless map, cased decomp string", t_cs_fs);
    
    MCMapEvalIsAmongTheKeysOf(*t_lc_norm_str, true, *t_cs_array, t_none);
    MCMapEvalIsAmongTheKeysOf(*t_uc_norm_str, false, *t_cs_array, t_cs);
    MCMapEvalIsAmongTheKeysOf(*t_lc_decomp_str, true, *t_cs_array, t_fs);
    MCMapEvalIsAmongTheKeysOf(*t_uc_decomp_str, false, *t_cs_array, t_cs_fs);
    
    log_result("case sensitive map, uncased string", t_none);
    log_result("case sensitive map, cased string", t_cs);
    log_result("case sensitive map, uncased decomp string", t_fs);
    log_result("case sensitive map, cased decomp string", t_cs_fs);
    
    MCMapEvalIsAmongTheKeysOf(*t_lc_norm_str, true, *t_fs_array, t_none);
    MCMapEvalIsAmongTheKeysOf(*t_uc_norm_str, true, *t_fs_array, t_cs);
    MCMapEvalIsAmongTheKeysOf(*t_lc_decomp_str, false, *t_fs_array, t_fs);
    MCMapEvalIsAmongTheKeysOf(*t_uc_decomp_str, false, *t_fs_array, t_cs_fs);
    
    log_result("form sensitive map, uncased string", t_none);
    log_result("form sensitive map, cased string", t_cs);
    log_result("form sensitive map, uncased decomp string", t_fs);
    log_result("form sensitive map, cased decomp string", t_cs_fs);
    
    MCMapEvalIsAmongTheKeysOf(*t_lc_norm_str, true, *t_cs_fs_array, t_none);
    MCMapEvalIsAmongTheKeysOf(*t_uc_norm_str, true, *t_cs_fs_array, t_cs);
    MCMapEvalIsAmongTheKeysOf(*t_lc_decomp_str, true, *t_cs_fs_array, t_fs);
    MCMapEvalIsAmongTheKeysOf(*t_uc_decomp_str, false, *t_cs_fs_array, t_cs_fs);
    
    log_result("case and form sensitive map, uncased string", t_none);
    log_result("case and form sensitive map map, cased string", t_cs);
    log_result("case and form sensitive map map, uncased decomp string", t_fs);
    log_result("case and form sensitive map map, cased decomp string", t_cs_fs);
    
    /*MCMapEvalKeysOf(MCArrayRef p_target, MCProperListRef& r_output)*/
    MCAutoProperListRef t_keys_list;
    MCArrayCreateWithOptions(true, true, t_keys . Ptr(), (MCValueRef *)t_strings . Ptr(), t_keys . Size(), &t_array_for_keys);
    MCMapEvalKeysOf(*t_array_for_keys, &t_keys_list);
    bool t_result;
    t_result = MCProperListGetLength(*t_keys_list) == 4;
    
    uindex_t t_offset;
    if (t_result)
        t_result = MCProperListFirstIndexOfElement(*t_keys_list, *t_uc_decomp_str, 0, t_offset);

    if (t_result)
        t_result = MCProperListFirstIndexOfElement(*t_keys_list, *t_lc_decomp_str, 0, t_offset);
    
    if (t_result)
        t_result = MCProperListFirstIndexOfElement(*t_keys_list, *t_uc_norm_str, 0, t_offset);
    
    if (t_result)
        t_result = MCProperListFirstIndexOfElement(*t_keys_list, *t_lc_norm_str, 0, t_offset);
    
    log_result("the keys of", t_result);
    
    /*MCMapEvalElementsOf(MCArrayRef p_target, MCProperListRef& r_output)*/
    
    MCAutoProperListRef t_elts_list;
    MCMapEvalElementsOf(*t_array_for_keys, &t_elts_list);
    t_result = MCProperListGetLength(*t_elts_list) == 4;

    if (t_result)
        t_result = MCProperListFirstIndexOfElement(*t_elts_list, *t_uc_decomp_str, 0, t_offset);
    
    if (t_result)
        t_result = MCProperListFirstIndexOfElement(*t_elts_list, *t_lc_decomp_str, 0, t_offset);
    
    if (t_result)
        t_result = MCProperListFirstIndexOfElement(*t_elts_list, *t_uc_norm_str, 0, t_offset);
    
    if (t_result)
        t_result = MCProperListFirstIndexOfElement(*t_elts_list, *t_lc_norm_str, 0, t_offset);
    
    log_result("the elements of", t_result);
    
    /*MCMapEvalNumberOfElementsIn(MCArrayRef p_target, uindex_t& r_output)*/

    uindex_t t_number;
    MCMapEvalNumberOfElementsIn(*t_array_for_keys, t_number);
    log_result("the number of elements in", t_number == 4);
    
    /*MCMapEvalIsAmongTheElementsOf(MCValueRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)*/
    
    t_result = true;
    if (t_result)
        MCMapEvalIsAmongTheElementsOf(*t_uc_decomp_str, false, *t_array_for_keys, t_result);
    
    if (t_result)
        MCMapEvalIsAmongTheElementsOf(*t_lc_decomp_str, false, *t_array_for_keys, t_result);
    
    if (t_result)
        MCMapEvalIsAmongTheElementsOf(*t_uc_norm_str, false, *t_array_for_keys, t_result);
    
    if (t_result)
        MCMapEvalIsAmongTheElementsOf(*t_lc_norm_str, false, *t_array_for_keys, t_result);
    
    log_result("is among the elements of", t_result);
    
    /*MCMapEvalIsAmongTheKeysOf(MCStringRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
     MCMapEvalIsAmongTheKeysOfNumeric(integer_t p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
     MCMapEvalIsAmongTheKeysOfMatrix(MCProperListRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)*/
    
    MCArrayStoreValueOnPath(*t_array, true, t_path . Ptr(), t_path . Size(), kMCTrue);
    
    MCAutoProperListRef t_list;
    MCProperListCreateMutable(&t_list);
    MCProperListPushElementsOntoBack(*t_list, t_list_elts .Ptr(), t_list_elts . Size() - 1);
    bool t_is_among;
    MCMapEvalIsAmongTheKeysOfMatrix(*t_list, false, *t_array, t_is_among);
    
    log_result("is (partial) among the keys of matrix", t_is_among);
    
    MCProperListPushElementOntoBack(*t_list, *t_3n);
    
    MCMapEvalIsAmongTheKeysOfMatrix(*t_list, false, *t_array, t_is_among);
    
    log_result("is among the keys of matrix", t_is_among);
    
    /*MCMapStoreElementOfMatrix(MCValueRef p_value, MCArrayRef& x_target, MCProperListRef p_key)*/
    MCAutoNumberRef t_4n;
    MCNumberCreateWithInteger(4, &t_4n);
    MCProperListPushElementOntoBack(*t_list, *t_4n);
    
    MCArrayRef t_matrix;
    MCArrayCreateMutable(t_matrix);
    MCMapStoreElementOfMatrix(kMCTrue, t_matrix, *t_list);
    
    /*MCMapFetchElementOfMatrix(MCArrayRef p_target, MCProperListRef p_key, MCValueRef& r_output)*/
    MCValueRef t_value;
    MCMapFetchElementOfMatrix(t_matrix, *t_list, t_value);
    
    MCValueRelease(t_matrix);
    
    log_result("store/fetch element of matrix", t_value == kMCTrue);
}
#endif
