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

#ifndef __MC_FOUNDATION_BIDI__
#define __MC_FOUNDATION_BIDI__

#include "foundation.h"

// The values assigned to these constants are significant: they represent the
// starting values in the Unicode BiDi algorithm and should not be changed.
enum MCTextDirection
{
    kMCTextDirectionAuto = -1,     // Detect based on contents
    kMCTextDirectionLTR = 0,       // Force left-to-right direction
    kMCTextDirectionRTL = 1,       // Force right-to-left direction
};

// Significant characters for BiDi processing
#define kMCBidiLRE      0x202A      // Left-to-right embedding
#define kMCBidiRLE      0x202B      // Right-to-left embedding
#define kMCBidiLRO      0x202D      // Left-to-right override
#define kMCBidiRLO      0x202E      // Right-to-left override
#define kMCBidiPDF      0x202C      // Pop directional formatting
#define kMCBidiLRI      0x2066      // Left-to-right isolate
#define kMCBidiRLI      0x2067      // Right-to-left isolate
#define kMCBidiFSI      0x2068      // First strong isolate
#define kMCBidiPDI      0x2069      // Pop directional isolate
#define kMCBidiLRM      0x200E      // Left-to-right mark
#define kMCBidiRLM      0x200F      // Right-to-left mark
#define kMCBidiALM      0x061C      // Arabic letter mark


bool MCBidiResolveTextDirection(MCStringRef p_string, intenum_t p_base_level, uint8_t *&r_levels, uindex_t& r_level_size);
uint8_t MCBidiFirstStrongIsolate(MCStringRef p_string, uindex_t p_offset);

#endif // __MC_FOUNDATION_BIDI__
