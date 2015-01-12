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

extern "C" MC_DLLEXPORT void
MCFileExecGetContents (MCStringRef p_path, MCDataRef & r_data)
{
	/* UNCHECKED */ MCFileGetContents (p_path, r_data);
}

extern "C" MC_DLLEXPORT void
MCFileExecSetContents (MCDataRef p_contents, MCStringRef p_path)
{
	/* UNCHECKED */ MCFileSetContents (p_path, p_contents);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void
MCStreamExecGetStandardOutput (MCStreamRef & r_stream)
{
	MCStreamGetStandardOutput (r_stream);
}

extern "C" MC_DLLEXPORT void
MCStreamExecGetStandardInput (MCStreamRef & r_stream)
{
	MCStreamGetStandardInput (r_stream);
}
extern "C" MC_DLLEXPORT void
MCStreamExecGetStandardError (MCStreamRef & r_stream)
{
	MCStreamGetStandardError (r_stream);
}
////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT MCStreamRef MCFileExecOpenFileForRead(MCStringRef p_filename)
{
    MCStreamRef t_stream;
    if (!MCFileCreateStream(p_filename, kMCOpenFileModeRead, t_stream))
        return nil;
    
    return t_stream;
}

extern "C" MC_DLLEXPORT MCStreamRef MCFileExecOpenFileForWrite(bool p_create, MCStringRef p_filename, MCStreamRef& r_stream)
{
    MCStreamRef t_stream;
    if (!MCFileCreateStream(p_filename, p_create ? kMCOpenFileModeCreate : kMCOpenFileModeWrite, t_stream))
        return nil;
    
    return t_stream;
}

extern "C" MC_DLLEXPORT MCStreamRef MCFileExecOpenFileForUpdate(bool p_create, MCStringRef p_filename, MCStreamRef& r_stream)
{
    MCStreamRef t_stream;
    if (!MCFileCreateStream(p_filename, p_create ? kMCOpenFileModeCreate : kMCOpenFileModeUpdate, t_stream))
        return nil;
    
    return t_stream;
}

extern "C" MC_DLLEXPORT MCDataRef MCFileExecReadFromStream(uindex_t p_amount, MCStreamRef p_stream)
{
	/* FIXME this should be handled by MCStreamRead */
    if (!MCStreamIsReadable(p_stream))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("stream is not readable"), nil);
        return MCValueRetain(kMCEmptyData);
    }
    
    byte_t t_buffer[p_amount];
    
    if (!MCStreamRead(p_stream, t_buffer, p_amount))
    {
		/* Error information should already be set */
        return MCValueRetain(kMCEmptyData);
    }
    
    MCDataRef t_data;
    if (!MCDataCreateWithBytes(t_buffer, p_amount, t_data))
        return MCValueRetain(kMCEmptyData);
    
    return t_data;
}

extern "C" MC_DLLEXPORT void
MCFileExecWriteToStream(MCDataRef p_data,
                        MCStreamRef p_stream)
{
	/* FIXME This check should be handled by MCStreamWrite */
	if (!MCStreamIsWritable (p_stream))
	{
		MCErrorCreateAndThrow (kMCGenericErrorTypeInfo, "reason", MCSTR("stream is not writable"), NULL);
		return;
	}

	if (!MCStreamWrite (p_stream,
	                    MCDataGetBytePtr (p_data), MCDataGetLength (p_data)))
	{
		return; /* Error should already have been set */
	}
}
