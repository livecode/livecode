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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "osspec.h"

#include "securemode.h"
#include "exec.h"
#include "util.h"
#include "uidc.h"

#include "system.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

uinteger_t MCFileStreamType_Measure(void)
{
    return sizeof(IO_handle);
}

void MCFileStreamType_Copy(const IO_handle p_source, IO_handle p_dest)
{
    if (!MCMemoryAllocateCopy(p_source, sizeof(IO_handle), p_dest))
    {
        // Throw
    }
}

void MCFileStreamType_Finalize(IO_handle p_handle)
{
    p_handle -> Close();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IO_handle MCFileExecOpenFileForRead(MCStringRef p_filename)
{
    return MCS_open(p_filename, kMCOpenFileModeRead, true, false, 0);
}

IO_handle MCFileExecOpenFileForWrite(MCStringRef p_filename)
{
    return MCS_open(p_filename, kMCOpenFileModeWrite, false, false, 0);
}

IO_handle MCFileExecOpenFileForUpdate(MCStringRef p_filename)
{
    return MCS_open(p_filename, kMCOpenFileModeUpdate, false, false, 0);
}

IO_handle MCFileExecOpenNewFile(MCStringRef p_filename)
{
    return MCS_open(p_filename, kMCOpenFileModeCreate, false, false, 0);
}

MCStringRef MCFileExecReadFromStream(IO_handle p_stream, uindex_t p_amount)
{
    byte_t *bytes;
    MCMemoryAllocate(p_amount, bytes);
    
    uindex_t t_read;
    MCStringRef t_result;
    
    if (p_stream -> Read(bytes, p_amount, t_read) &&
        MCStringCreateWithBytesAndRelease(bytes, t_read, kMCStringEncodingNative, false, t_result))
        return t_result;
    
    // Throw
    MCMemoryDeallocate(bytes);
    
    return MCValueRetain(kMCEmptyString);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern void log(const char *module, const char *test, bool result);
#define log_result(test, result) log("FILE MODULE", test, result)
void MCFileRunTests()
{
    MCStringRef t_string = MCSTR("/Users/alilloyd/Desktop/test.txt");
    
    IO_handle t_stream;
    t_stream = MCFileExecOpenFileForRead(t_string);
    
    MCStringRef t_first, t_second;
    
    t_first = MCFileExecReadFromStream(t_stream, 13);
    
    log_result("read test", MCStringIsEqualToCString(t_first, "abcdefghijklm", kMCStringOptionCompareCaseless));
    
    MCValueRelease(t_first);
    
    t_second = MCFileExecReadFromStream(t_stream, 13);
    
    log_result("stream test", MCStringIsEqualToCString(t_second, "nopqrstuvwxyz", kMCStringOptionCompareCaseless));
    
    MCValueRelease(t_second);
}


