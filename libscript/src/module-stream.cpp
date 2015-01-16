/*                                                                     -*-c++-*-
Copyright (C) 2015 Runtime Revolution Ltd.

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

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void
MCStreamExecWriteToStream(MCDataRef p_data,
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

extern "C" MC_DLLEXPORT MCDataRef
MCStreamExecReadFromStream (uindex_t p_amount,
                            MCStreamRef p_stream)
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


////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void
MCStreamExecGetStandardOutput (MCStreamRef & r_stream)
{
	MCStreamGetStandardOutput (r_stream);
}
