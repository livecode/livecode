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

////////////////////////////////////////////////////////////////////////////////////////////////////

MCDataRef MCEncodingExecCompressUsingZlib(MCDataRef p_target)
{
    MCDataRef t_compressed;
//    if (!MCFiltersCompress(p_target, t_compressed))
        return nil;

    return t_compressed;
}

MCDataRef MCEncodingExecDecompressUsingZlib(MCDataRef p_target)
{
    MCDataRef t_decompressed;
//    if (!MCFiltersDecompress(p_target, t_decompressed))
        return nil;
    
    return t_decompressed;
}

void MCEncodingEvalCompressedUsingZlib(MCDataRef p_target, MCDataRef& r_compressed)
{
//    if (!MCFiltersCompress(p_target, r_compressed))
        return;
}

void MCEncodingEvalDecompressedUsingZlib(MCDataRef p_target, MCDataRef& r_decompressed)
{
//    if (!MCFiltersDecompress(p_target, r_decompressed))
        return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT MCStringRef MCEncodingExecEncodeUsingBase64(MCDataRef p_target)
{
    MCStringRef t_string;
    if (!MCFiltersBase64Encode(p_target, t_string))
        return nil;
    
    return t_string;
}

extern "C" MC_DLLEXPORT MCDataRef MCEncodingExecDecodeUsingBase64(MCStringRef p_target)
{
    MCDataRef t_data;
    if (!MCFiltersBase64Decode(p_target, t_data))
        return nil;
    
    return t_data;
}

extern "C" MC_DLLEXPORT void MCEncodingEvalEncodedUsingBase64(MCDataRef p_target, MCStringRef& r_output)
{
    if (!MCFiltersBase64Encode(p_target, r_output))
        return;
}

extern "C" MC_DLLEXPORT void MCEncodingEvalDecodedUsingBase64(MCStringRef p_target, MCDataRef& r_output)
{
    if (!MCFiltersBase64Decode(p_target, r_output))
        return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MCEncodingExecEncodeUsingBinary(MCStringRef p_target, MCStringRef p_format, MCDataRef& r_output)
{
    //  TODO: Move binary encode/decode to foundation
    //  Does this take a list?
}

void MCEncodingExecDecodeUsingBinary(MCDataRef p_target, MCStringRef p_format, MCStringRef& r_output)
{
    //  TODO: Move binary encode/decode to foundation
    //  Does this take a list?
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void MCEncodingEvaluateTextEncoding(integer_t p_type, integer_t& r_type)
{
    // TODO: Make this less ad-hoc
    // Currently relies on order of MCStringEncoding enum
    r_type = p_type;
}

extern "C" MC_DLLEXPORT MCDataRef MCEncodingExecEncodeTextUsingEncoding(MCStringRef p_target, integer_t p_encoding)
{
    MCDataRef t_data;
    if (!MCStringEncode(p_target, (MCStringEncoding)p_encoding, false, t_data))
        return nil;
    
    return t_data;
}

extern "C" MC_DLLEXPORT MCStringRef MCEncodingExecDecodeTextUsingEncoding(MCDataRef p_target, integer_t p_encoding)
{
    MCStringRef t_string;
    if (!MCStringDecode(p_target, (MCStringEncoding)p_encoding, false, t_string))
        return nil;
    
    return t_string;
}

extern "C" MC_DLLEXPORT MCDataRef MCEncodingExecEncodeTextUsingEncodingString(MCStringRef p_target, MCStringRef p_encoding)
{
    MCStringEncoding t_encoding;
    if (!MCStringEvalTextEncoding(p_encoding, t_encoding))
    {
        //MCErrorCreateAndThrow();
        return nil;
    }
    
    MCDataRef t_data;
    if (!MCStringEncode(p_target, t_encoding, false, t_data))
        return nil;
    
    return t_data;
}

extern "C" MC_DLLEXPORT MCStringRef MCEncodingExecDecodeTextUsingEncodingString(MCDataRef p_target, MCStringRef p_encoding)
{
    MCStringEncoding t_encoding;
    if (!MCStringEvalTextEncoding(p_encoding, t_encoding))
    {
        //MCErrorCreateAndThrow();
        return nil;
    }
    
    MCStringRef t_string;
    if (!MCStringDecode(p_target, t_encoding, false, t_string))
        return nil;
    
    return t_string;
}

extern "C" MC_DLLEXPORT void MCEncodingEvalTextEncodedUsingEncoding(MCStringRef p_target, integer_t p_encoding, MCDataRef& r_encoded)
{
    if (!MCStringEncode(p_target, (MCStringEncoding)p_encoding, false, r_encoded))
        return;
}

extern "C" MC_DLLEXPORT void MCEncodingEvalTextDecodedUsingEncoding(MCDataRef p_target, integer_t p_encoding, MCStringRef& r_decoded)
{
    if (!MCStringDecode(p_target, (MCStringEncoding)p_encoding, false, r_decoded))
        return;
}

extern "C" MC_DLLEXPORT void MCEncodingEvalTextEncodedUsingEncodingString(MCStringRef p_target, MCStringRef p_encoding, MCDataRef& r_encoded)
{
    MCStringEncoding t_encoding;
    if (!MCStringEvalTextEncoding(p_encoding, t_encoding))
    {
        //MCErrorCreateAndThrow();
        return;
    }
    
    if (!MCStringEncode(p_target, t_encoding, false, r_encoded))
        return;
}

extern "C" MC_DLLEXPORT void MCEncodingEvalTextDecodedUsingEncodingString(MCDataRef p_target, MCStringRef p_encoding, MCStringRef& r_decoded)
{
    MCStringEncoding t_encoding;
    if (!MCStringEvalTextEncoding(p_encoding, t_encoding))
    {
        //MCErrorCreateAndThrow();
        return;
    }
    
    if (!MCStringDecode(p_target, t_encoding, false, r_decoded))
        return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MCEncodingEvalURLEncoded(MCStringRef p_target, MCStringRef& r_output)
{
    if (!MCFiltersUrlEncode(p_target, r_output))
        return;
}

void MCEncodingEvalURLDecoded(MCStringRef p_target, MCStringRef& r_output)
{
    if (!MCFiltersUrlDecode(p_target, r_output))
        return;
}


void MCEncodingEvalEncodedOfValue(MCValueRef p_target, MCDataRef& r_output)
{
    
}

void MCEncodingEvalDecodedOfValue(MCDataRef p_target, MCValueRef& r_output)
{
    
}

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
