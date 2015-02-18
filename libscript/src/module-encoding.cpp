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
#include "foundation-auto.h"
#include "foundation-filters.h"

#if 0
void MCEncodingEvalEncodedOfValue(MCValueRef p_target, MCDataRef& r_output)
{
    
}

void MCEncodingEvalDecodedOfValue(MCDataRef p_target, MCValueRef& r_output)
{
    
}

void MCEncodingExecCompress(MCDataRef& x_target)
{
    MCAutoDataRef t_compressed;
    if (MCFiltersCompress(x_target, &t_compressed))
    {
        MCValueAssign(x_target, *t_compressed);
        return;
    }
    
    //    ctxt . Throw();
}

void MCEncodingExecDecompress(MCDataRef& x_target)
{
    MCAutoDataRef t_decompressed;
    if (MCFiltersDecompress(x_target, &t_decompressed))
    {
        MCValueAssign(x_target, *t_decompressed);
        return;
    }
    
    //    ctxt . Throw();
}

void MCEncodingExecEncodeUsingBase64(MCDataRef p_target, MCStringRef& r_output)
{
    if (MCFiltersBase64Encode(p_target, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecDecodeUsingBase64(MCStringRef p_target, MCDataRef& r_output)
{
    if (MCFiltersBase64Decode(p_target, r_output))
        return;
    
    //    ctxt . Throw();
}
#endif

////////

// Format string:
//   8-bit host / network : signed / unsigned - [n|N]b|B
//   16-bit host / network : signed / unsigned - [n|N]s|S
//   32-bit host / network : signed / unsigned - [n|N]i|I
//   64-bit host / network : signed / unsigned - [n|N]l|L
//

template<typename T> static bool __EncodeNumber(MCDataRef p_data, MCValueRef p_number)
{
    T t_value;
    t_value = (T)MCNumberFetchAsReal((MCNumberRef)p_number);
    if (!MCDataAppendBytes(p_data, (const byte_t *)&t_value, sizeof(T)))
        return false;
    return true;
}

extern "C" MC_DLLEXPORT MCDataRef MCEncodingExecEncodeUsingBinary(MCProperListRef p_target, MCStringRef p_format)
{
    MCAutoDataRef t_data;
    if (!MCDataCreateMutable(0, &t_data))
        return nil;
    
    const unichar_t *t_format_ptr, *t_format_limit;
    t_format_ptr = MCStringGetCharPtr(p_format);
    t_format_limit = t_format_ptr + MCStringGetLength(p_format);
    
    uindex_t t_index;
    t_index = 0;
    while(t_format_ptr != t_format_limit)
    {
        if (t_index >= MCProperListGetLength(p_target))
        {
            MCErrorThrowGeneric(MCSTR("not enough elements to satisfy format string"));
            return nil;
        }
        
        MCValueRef t_value;
        t_value = MCProperListFetchElementAtIndex(p_target, t_index);
        
        if (MCValueGetTypeInfo(t_value) != kMCNumberTypeInfo)
        {
            MCErrorThrowGeneric(MCSTR("non-numeric value passed to binary encode"));
            return nil;
        }
        
        switch(*t_format_ptr)
        {
            case 'b': if (!__EncodeNumber<int8_t>(*t_data, t_value)) return nil; break;
            case 'B': if (!__EncodeNumber<uint8_t>(*t_data, t_value)) return nil; break;
            case 's': if (!__EncodeNumber<int16_t>(*t_data, t_value)) return nil; break;
            case 'S': if (!__EncodeNumber<uint16_t>(*t_data, t_value)) return nil; break;
            case 'i': if (!__EncodeNumber<int32_t>(*t_data, t_value)) return nil; break;
            case 'I': if (!__EncodeNumber<uint32_t>(*t_data, t_value)) return nil; break;
            case 'l': if (!__EncodeNumber<int64_t>(*t_data, t_value)) return nil; break;
            case 'L': if (!__EncodeNumber<uint64_t>(*t_data, t_value)) return nil; break;
            default:
                MCErrorThrowGeneric(MCSTR("invalid binary format string"));
                return nil;
        }
        
        t_format_ptr += 1;
        t_index += 1;
    }
    
    return t_data . Take();
}

extern "C" MC_DLLEXPORT MCProperListRef MCEncodingExecDecodeUsingBinary(MCDataRef p_target, MCStringRef p_format)
{
    MCAutoProperListRef t_list;
    if (!MCProperListCreateMutable(&t_list))
        return nil;
    
    const unichar_t *t_format_ptr, *t_format_limit;
    t_format_ptr = MCStringGetCharPtr(p_format);
    t_format_limit = t_format_ptr + MCStringGetLength(p_format);
    
    const byte_t *t_byte_ptr, *t_byte_limit;
    t_byte_ptr = MCDataGetBytePtr(p_target);
    t_byte_limit = t_byte_ptr + MCDataGetLength(p_target);
    
    while(t_format_ptr != t_format_limit)
    {
        MCAutoNumberRef t_value;
        switch(*t_format_ptr)
        {
            case 'b':
                if (t_byte_limit - t_byte_ptr < 1)
                    goto not_enough_bytes_error;
                if (!MCNumberCreateWithInteger((int8_t)*t_byte_ptr, &t_value))
                    return nil;
                t_byte_ptr += 1;
                break;
                
            case 'B':
                if (t_byte_limit - t_byte_ptr < 1)
                    goto not_enough_bytes_error;
                if (!MCNumberCreateWithUnsignedInteger(*(uint8_t *)t_byte_ptr, &t_value))
                    return nil;
                t_byte_ptr += 1;
                break;
                
            case 's':
                if (t_byte_limit - t_byte_ptr < 2)
                    goto not_enough_bytes_error;
                if (!MCNumberCreateWithInteger(*(int16_t *)t_byte_ptr, &t_value))
                    return nil;
                t_byte_ptr += 2;
                break;
                
            case 'S':
                if (t_byte_limit - t_byte_ptr < 1)
                    goto not_enough_bytes_error;
                if (!MCNumberCreateWithUnsignedInteger(*(uint16_t *)t_byte_ptr, &t_value))
                    return nil;
                t_byte_ptr += 2;
                break;
                
            case 'i':
                if (t_byte_limit - t_byte_ptr < 4)
                    goto not_enough_bytes_error;
                if (!MCNumberCreateWithInteger(*(int32_t *)t_byte_ptr, &t_value))
                    return nil;
                t_byte_ptr += 4;
                break;
                
            case 'I':
                if (t_byte_limit - t_byte_ptr < 4)
                    goto not_enough_bytes_error;
                if (!MCNumberCreateWithUnsignedInteger(*(uint32_t *)t_byte_ptr, &t_value))
                    return nil;
                t_byte_ptr += 4;
                break;
                
            case 'l':
                if (t_byte_limit - t_byte_ptr < 8)
                    goto not_enough_bytes_error;
                if (!MCNumberCreateWithInteger(*(int64_t *)t_byte_ptr, &t_value))
                    return nil;
                t_byte_ptr += 8;
                break;
                
            case 'L':
                if (t_byte_limit - t_byte_ptr < 8)
                    goto not_enough_bytes_error;
                if (!MCNumberCreateWithUnsignedInteger(*(uint64_t *)t_byte_ptr, &t_value))
                    return nil;
                t_byte_ptr += 8;
                break;
                
            default:
                MCErrorThrowGeneric(MCSTR("invalid binary format string"));
                return nil;
        }
        
        if (!MCProperListPushElementOntoBack(*t_list, *t_value))
            return nil;
        
        t_format_ptr += 1;
        
        continue;
        
    not_enough_bytes_error:
        MCErrorThrowGeneric(MCSTR("not enough bytes to satisfy format string"));
        return nil;
    }
    
    return t_list . Take();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
void MCEncodingExecEncodeUsingUTF8(MCStringRef p_target, MCDataRef& r_output)
{
    if (MCStringEncode(p_target, kMCStringEncodingUTF8, false, r_output))
        return;
    
//    ctxt . Throw();
}

void MCEncodingExecDecodeUsingUTF8(MCDataRef p_target, MCStringRef& r_output)
{
    if (MCStringDecode(p_target, kMCStringEncodingUTF8, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecEncodeUsingUTF16(MCStringRef p_target, MCDataRef& r_output)
{
    if (MCStringEncode(p_target, kMCStringEncodingUTF16, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecDecodeUsingUTF16(MCDataRef p_target, MCStringRef& r_output)
{
    if (MCStringDecode(p_target, kMCStringEncodingUTF16, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecEncodeUsingUTF32(MCStringRef p_target, MCDataRef& r_output)
{
    if (MCStringEncode(p_target, kMCStringEncodingUTF32, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecDecodeUsingUTF32(MCDataRef p_target, MCStringRef& r_output)
{
    if (MCStringDecode(p_target, kMCStringEncodingUTF32, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecEncodeUsingASCII(MCStringRef p_target, MCDataRef& r_output)
{
    if (MCStringEncode(p_target, kMCStringEncodingASCII, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingExecDecodeUsingASCII(MCDataRef p_target, MCStringRef& r_output)
{
    if (MCStringDecode(p_target, kMCStringEncodingASCII, false, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingEvalURLEncoded(MCStringRef p_target, MCStringRef& r_output)
{
    if (MCFiltersUrlEncode(p_target, r_output))
        return;
    
    //    ctxt . Throw();
}

void MCEncodingEvalURLDecoded(MCStringRef p_target, MCStringRef& r_output)
{
    if (MCFiltersUrlDecode(p_target, r_output))
        return;
    
    //    ctxt . Throw();
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _TEST
extern void log(const char *module, const char *test, bool result);
#define log_result(test, result) log("ENCODING MODULE", test, result)
void MCEncodingRunTests()
{
/*
 void MCEncodingEvalEncodedOfValue(MCValueRef p_target, MCDataRef& r_output)
 void MCEncodingEvalDecodedOfValue(MCDataRef p_target, MCValueRef& r_output)
 void MCEncodingExecEncodeUsingBase64(MCDataRef p_target, MCStringRef& r_output)
 void MCEncodingExecDecodeUsingBase64(MCStringRef p_target, MCDataRef& r_output)
 void MCEncodingExecEncodeUsingBinary(MCStringRef p_target, MCStringRef p_format, MCDataRef& r_output)
 void MCEncodingExecDecodeUsingBinary(MCDataRef p_target, MCStringRef p_format, MCStringRef& r_output)
 void MCEncodingExecEncodeUsingUTF8(MCStringRef p_target, MCDataRef& r_output)
 void MCEncodingExecDecodeUsingUTF8(MCDataRef p_target, MCStringRef& r_output)
 void MCEncodingExecEncodeUsingUTF16(MCStringRef p_target, MCDataRef& r_output)
 void MCEncodingExecDecodeUsingUTF16(MCDataRef p_target, MCStringRef& r_output)
 void MCEncodingExecEncodeUsingUTF32(MCStringRef p_target, MCDataRef& r_output)
 void MCEncodingExecDecodeUsingUTF32(MCDataRef p_target, MCStringRef& r_output)
 void MCEncodingExecEncodeUsingASCII(MCStringRef p_target, MCDataRef& r_output)
 void MCEncodingExecDecodeUsingASCII(MCDataRef p_target, MCStringRef& r_output)
 */
 
/*
void MCEncodingExecCompress(MCDataRef& x_target)
void MCEncodingExecDecompress(MCDataRef& x_target)
*/
    MCAutoDataRef t_data;
    MCDataCreateWithBytes((const byte_t *)"hello world", 11, &t_data);
    
    MCDataRef t_to_compress;
    MCDataMutableCopy(*t_data, t_to_compress);
    MCEncodingExecCompress(t_to_compress);

    log_result("compress changes data", !MCDataIsEqualTo(*t_data, t_to_compress));
    
    MCEncodingExecDecompress(t_to_compress);
    
    log_result("compress/decompress round trip", MCDataIsEqualTo(*t_data, t_to_compress));
    
    MCValueRelease(t_to_compress);
    
    MCDataRef t_empty;
    MCDataMutableCopy(kMCEmptyData, t_empty);
    
    MCEncodingExecCompress(t_empty);
    MCEncodingExecDecompress(t_empty);
    
    log_result("compress/decompress empty", MCDataIsEqualTo(t_empty, kMCEmptyData));
    
    MCValueRelease(t_empty);
 /*
    void MCEncodingEvalURLEncoded(MCStringRef p_target, MCStringRef& r_output)
    void MCEncodingEvalURLDecoded(MCStringRef p_target, MCStringRef& r_output)
*/
    MCStringRef t_test_a, t_test_b;
    MCStringRef t_test_c, t_test_d;
    
    t_test_a = MCSTR(" ");
    t_test_b = MCSTR("+");
    
    t_test_c = MCSTR("?");
    t_test_d = MCSTR("%3F");
    
    MCAutoStringRef t_encoded, t_decoded;
    MCAutoStringRef t_encoded2, t_decoded2;
    
    MCEncodingEvalURLEncoded(t_test_a, &t_encoded);
    
    log_result("url encode space", MCStringIsEqualTo(*t_encoded, t_test_b, kMCStringOptionCompareCaseless));
    
    MCEncodingEvalURLEncoded(t_test_c, &t_encoded2);
    
    log_result("url encode ?", MCStringIsEqualTo(*t_encoded2, t_test_d, kMCStringOptionCompareCaseless));
    
    MCEncodingEvalURLDecoded(t_test_b, &t_decoded);
    
    log_result("url decode +", MCStringIsEqualTo(*t_decoded, t_test_a, kMCStringOptionCompareCaseless));
    
    MCEncodingEvalURLDecoded(t_test_d, &t_decoded2);
    
    log_result("url decode %3F", MCStringIsEqualTo(*t_decoded2, t_test_c, kMCStringOptionCompareCaseless));
    
}
#endif
