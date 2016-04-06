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

#include <stdio.h>

#include "Position.h"

////////////////////////////////////////////////////////////////////////////////

Position PositionMake(uint32_t p_row, uint32_t p_column)
{
	return MCMin(1023U, p_column) | (p_row << 10);
}

const char *PositionDescribe(Position p_position)
{
	static char s_buffer[64];
	sprintf(s_buffer, "%04d:%04d", PositionGetRow(p_position) + 1, PositionGetColumn(p_position) + 1);
	return s_buffer;
}

uint32_t PositionGetRow(Position p_position)
{
	return 1 + p_position / 1024;
}

uint32_t PositionGetColumn(Position p_position)
{
	return 1 + p_position % 1024;
}

////////////////////////////////////////////////////////////////////////////////
