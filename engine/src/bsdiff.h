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

#ifndef __MC_BSDIFF__
#define __MC_BSDIFF__

struct MCBsDiffInputStream
{
	virtual bool Measure(uint32_t& r_size) = 0;
	virtual bool ReadBytes(void *buffer, uint32_t count) = 0;
	virtual bool ReadInt32(int32_t& r_value) = 0;
};

struct MCBsDiffOutputStream
{
	virtual bool Rewind(void) = 0;
	virtual bool WriteBytes(const void *buffer, uint32_t count) = 0;
	virtual bool WriteInt32(int32_t value) = 0;
};

bool MCBsDiffBuild(MCBsDiffInputStream *old_stream, MCBsDiffInputStream *new_stream, MCBsDiffOutputStream *patch_stream);
bool MCBsDiffApply(MCBsDiffInputStream *patch_stream, MCBsDiffInputStream *input_stream, MCBsDiffOutputStream *output_stream);

#endif
