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

#if !defined(__MCS_SYSTEM_H_INSIDE__)
#	error "Only <foundation-system.h> can be included directly"
#endif

/* ================================================================
 * System error handling
 * ================================================================ */

/* The MCSError library provides an API for accessing and resetting
 * the system error status.  This is to support standardized use of
 * the POSIX and Win32 APIs.
 */

/* Reset or clear the current system error state. */
MC_DLLEXPORT void MCSErrorReset (void);

/* Get the platform-dependent numeric code corresponding to the
 * current system error. */
MC_DLLEXPORT uint32_t MCSErrorGetCode (void);

/* Get a platform-dependent string describing a system error code. */
MC_DLLEXPORT bool MCSErrorGetDescription (uint32_t p_code, MCStringRef& r_description);
