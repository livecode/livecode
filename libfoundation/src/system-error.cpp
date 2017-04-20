/*                                                                     -*-c++-*-
Copyright (C) 2016 LiveCode Ltd.

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

#include "system-private.h"

#include <foundation-auto.h>

#if defined(__WINDOWS__)
/* ================================================================
 * Windows implementations
 * ================================================================ */
#	include <windows.h>

MC_DLLEXPORT_DEF void
MCSErrorReset ()
{
	SetLastError (0);
}

MC_DLLEXPORT_DEF uint32_t
MCSErrorGetCode ()
{
	return GetLastError ();
}

MC_DLLEXPORT_DEF bool
MCSErrorGetDescription (uint32_t p_code,
                        MCStringRef& r_description)
{
	DWORD t_code = GetLastError ();

	/* No error has been recorded */
	if (0 == t_code)
	{
		return MCStringCopy (kMCEmptyString, r_description);
	}

	LPWSTR t_message_buffer = nil;

	DWORD t_message_len = FormatMessageW (
		(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		 FORMAT_MESSAGE_FROM_SYSTEM |
		 FORMAT_MESSAGE_IGNORE_INSERTS), /* dwFlags */
		nil,    /* lpSource */
		t_code, /* dwMessageId */
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* dwLanguageId */
		reinterpret_cast<LPWSTR>(&t_message_buffer), /* lpBuffer */
		0,      /* nSize */
		nil);   /* arguments... */

	bool t_success = true;
	MCAutoStringRef t_description;

	if (t_success)
		t_success = MCStringCreateWithChars(t_message_buffer,
		                                    t_message_len,
		                                    &t_description);

	LocalFree(t_message_buffer);

	return MCStringCopy(*t_description, r_description);
}

#else /* !__WINDOWS__ */
/* ================================================================
 * POSIX implementations
 * ================================================================ */

#include <errno.h>

MC_DLLEXPORT_DEF void
MCSErrorReset ()
{
	errno = 0;
}

MC_DLLEXPORT_DEF uint32_t
MCSErrorGetCode ()
{
	return errno;
}

MC_DLLEXPORT_DEF bool
MCSErrorGetDescription (uint32_t p_code,
                        MCStringRef& r_description)
{
	return MCStringCreateWithCString(strerror(p_code),
	                                 r_description);
}

#endif /* !__WINDOWS__ */
