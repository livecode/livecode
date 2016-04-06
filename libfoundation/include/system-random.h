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
 * Random number generation
 * ================================================================ */

/* Generate p_length bytes of random data. */
MC_DLLEXPORT bool MCSRandomData (uindex_t p_length, MCDataRef & r_data);

/* Generate a random real number on the interval [0,1).  If any random
 * number generation failure occurs, returns a quiet NaN. */
MC_DLLEXPORT real64_t MCSRandomReal (void);

#ifdef __MCS_INTERNAL_API__

/* Populate x_buffer with p_buffer_length uniformly-distributed random
 * bytes. */
bool __MCSRandomBytes (void *x_buffer, size_t p_buffer_length);

#endif
