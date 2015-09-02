/* Copyright (C) 2009-2015 LiveCode Ltd.

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

#ifndef __MC_SSERIALIZE_LNX__
#define __MC_SSERIALIZE_LNX__

#ifndef __MC_CORE__
#include "core.h"
#endif

////////////////////////////////////////////////////////////////////////////////

struct MCLinuxPageSetup
{
	int32_t paper_width;
	int32_t paper_height;
	int32_t left_margin, top_margin, right_margin, bottom_margin;
	uint32_t orientation;
};

bool MCLinuxPageSetupEncode(const MCLinuxPageSetup& setup, void*& r_data, uint32_t& r_data_size);
bool MCLinuxPageSetupDecode(const void *data, uint32_t data_size, MCLinuxPageSetup& setup);

////////////////////////////////////////////////////////////////////////////////

struct MCLinuxPrintSetup
{
};

bool MCLinuxPrintSetupEncode(const MCLinuxPrintSetup& setup, void*& r_data, uint32_t& r_data_size);
bool MCLinuxPrintSetupDecode(const void *data, uint32_t data_size, MCLinuxPrintSetup& setup);

////////////////////////////////////////////////////////////////////////////////

#endif
