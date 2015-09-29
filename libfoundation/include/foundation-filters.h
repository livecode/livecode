/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

#ifndef __MC_FOUNDATION_FILTERS__
#define __MC_FOUNDATION_FILTERS__

#ifndef __MC_FOUNDATION__
#include <foundation.h>
#endif

////////////////////////////////////////////////////////////////////////////////

bool MCFiltersBase64Decode(MCStringRef p_src, MCDataRef& r_dst);
bool MCFiltersBase64Encode(MCDataRef p_src, MCStringRef& r_dst);
bool MCFiltersCompress(MCDataRef p_source, MCDataRef& r_result);
bool MCFiltersIsCompressed(MCDataRef p_source);
bool MCFiltersDecompress(MCDataRef p_source, MCDataRef& r_result);
bool MCFiltersIsoToMac(MCDataRef p_source, MCDataRef &r_result);
bool MCFiltersMacToIso(MCDataRef p_source, MCDataRef &r_result);
bool MCFiltersUrlEncode(MCStringRef p_source, MCStringRef& r_result);
bool MCFiltersUrlDecode(MCStringRef p_source, MCStringRef& r_result);

////////////////////////////////////////////////////////////////////////////////

#endif
