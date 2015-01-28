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
#include <foundation-system.h>

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

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void
MCStreamExecGetStandardOutput (MCStreamRef & r_stream)
{
	MCSStreamGetStandardOutput (r_stream);
}
