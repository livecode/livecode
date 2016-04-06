/*                                                                     -*-c++-*-
Copyright (C) 2015 LiveCode Ltd.

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
 * C stdio-based streams
 * ================================================================ */

#include <stdio.h>

// Standard streams
MC_DLLEXPORT bool MCSStreamGetStandardOutput(MCStreamRef & r_stdout);
MC_DLLEXPORT bool MCSStreamGetStandardInput(MCStreamRef & r_stdin);
MC_DLLEXPORT bool MCSStreamGetStandardError(MCStreamRef & r_stderr);

#ifdef __MCS_INTERNAL_API__

bool __MCSStreamCreateWithStdio (FILE *, MCStreamRef & r_stream);

#endif

/* ================================================================
 * Initialization
 * ================================================================ */

#ifdef __MCS_INTERNAL_API__

bool __MCSStreamInitialize (void);
void __MCSStreamFinalize (void);

#endif
