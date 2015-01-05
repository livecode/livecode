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

#include "foundation-file.h"

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void MCFileExecOpenFileForRead(MCStringRef p_filename, MCStreamRef& r_stream)
{
    if (!MCFileCreateStreamForFile(p_filename, kMCOpenFileModeRead, r_stream))
        return;
}

extern "C" MC_DLLEXPORT void MCFileExecOpenFileForWrite(MCStringRef p_filename, MCStreamRef& r_stream)
{
    if (!MCFileCreateStreamForFile(p_filename, kMCOpenFileModeWrite, r_stream))
        return;
}

extern "C" MC_DLLEXPORT void MCFileExecOpenFileForUpdate(MCStringRef p_filename, MCStreamRef& r_stream)
{
    if (!MCFileCreateStreamForFile(p_filename, kMCOpenFileModeUpdate, r_stream))
        return;
}

extern "C" MC_DLLEXPORT void MCFileExecOpenNewFile(MCStringRef p_filename, MCStreamRef& r_stream)
{
    if (!MCFileCreateStreamForFile(p_filename, kMCOpenFileModeCreate, r_stream))
        return;
}

extern "C" MC_DLLEXPORT MCStringRef MCFileExecReadFromStream(uindex_t p_amount, MCStreamRef p_stream)
{
    if (!MCStreamIsReadable(p_stream))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("stream is not readable"), nil);
        return;
    }
    
    char t_buffer[p_amount];
    
    if (!MCStreamRead(p_stream, t_buffer, p_amount))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("could not read from stream"), nil);
        return;
    }
    
    MCStringCreateWithBytes((const byte_t *)t_buffer, p_amount, kMCStringEncodingUTF8, false, r_result);
}


