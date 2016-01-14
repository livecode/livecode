/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "prefix.h"

#include "parsedef.h"

#include "globals.h"
#include "system.h"

/* ----------------------------------------------------------------
 * Functions implemented in em-url.js
 * ---------------------------------------------------------------- */

extern "C" bool MCEmscriptenUrlLoad(const unichar_t *p_url,
                                    uint32_t p_url_length,
                                    const unichar_t *p_headers,
                                    uint32_t p_headers_length,
                                    uint32_t p_timeout,
                                    MCSystemUrlCallback p_callback,
                                    void *p_context);

/* ----------------------------------------------------------------
 * Wrappers (with C++ linkage)
 * ---------------------------------------------------------------- */

bool
MCSystemLoadUrl(MCStringRef p_url,
                MCSystemUrlCallback p_callback,
                void *p_context)
{
	MCAutoStringRefAsUTF16String t_url_u16;
	if (!t_url_u16.Lock(p_url))
	{
		return false;
	}

	MCAutoStringRefAsUTF16String t_headers_u16;
	if (!t_headers_u16.Lock(MChttpheaders))
	{
		return false;
	}

	return MCEmscriptenUrlLoad(t_url_u16.Ptr(), t_url_u16.Size(),
	                           t_headers_u16.Ptr(), t_headers_u16.Size(),
	                           MCsockettimeout,
	                           p_callback, p_context);
}

/* ---------------------------------------------------------------- */

extern "C" MC_DLLEXPORT_DEF void
MCEmscriptenUrlCallbackStarted(MCSystemUrlCallback p_callback,
                               void *p_context)
{
	p_callback(p_context, kMCSystemUrlStatusStarted, nil);
}

extern "C" MC_DLLEXPORT_DEF void
MCEmscriptenUrlCallbackProgress(uint32_t p_loaded_length,
                                int32_t p_total_length,
                                MCSystemUrlCallback p_callback,
                                void *p_context)
{
	/* Dispatch the callback.  We need to send two messages: one with
	 * the total length of the data being transferred, and one with
	 * the length transferred so far. */
	p_callback(p_context, kMCSystemUrlStatusNegotiated, &p_total_length);
	p_callback(p_context, kMCSystemUrlStatusLoadingProgress, &p_loaded_length);
}

extern "C" MC_DLLEXPORT_DEF void
MCEmscriptenUrlCallbackFinished(const byte_t *p_data,
                                uint32_t p_data_length,
                                MCSystemUrlCallback p_callback,
                                void *p_context)
{
	/* Assemble the request data */
	MCAutoDataRef t_data;
	if (!MCDataCreateWithBytes(p_data, p_data_length, &t_data))
	{
		t_data = kMCEmptyData;
	}

	/* Dispatch the callback. We need to send two messages: a progress
	 * message with the data received, and a finished message. */
	p_callback(p_context, kMCSystemUrlStatusLoading, *t_data);
	p_callback(p_context, kMCSystemUrlStatusFinished, nil);
}

extern "C" MC_DLLEXPORT_DEF void
MCEmscriptenUrlCallbackError(const unichar_t *p_error,
                             uint32_t p_error_length,
                             MCSystemUrlCallback p_callback,
                             void *p_context)
{
	/* Assemble the error information string */
	MCAutoStringRef t_error;
	if (!MCStringCreateWithChars(p_error, p_error_length, &t_error))
	{
		t_error = kMCEmptyString;
	}

	/* Dispatch the callback */
	p_callback(p_context, kMCSystemUrlStatusError, *t_error);
}
