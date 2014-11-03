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
    if (MCFiltersCompress(x_target, &t_decompressed))
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

void MCEncodingExecEncodeUsingBinary(MCStringRef p_target, MCStringRef p_format, MCDataRef& r_output)
{
    //  TODO: Move binary encode/decode to foundation
    //  Does this take a list?
    //    ctxt . Throw();
}

void MCEncodingExecDecodeUsingBinary(MCDataRef p_target, MCStringRef p_format, MCStringRef& r_output)
{
    //  TODO: Move binary encode/decode to foundation
    //  Does this take a list?
    //    ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

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