/* Copyright (C) 2016 LiveCode Ltd.

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

#include "gtest/gtest.h"

#include "prefix.h"
#include "sysdefs.h"
#include "util.h"
#include "util.cpp"

static const char** expected_string(Chunk_term p_chunk)
{
	switch (p_chunk)
	{
		case CT_TOOLTIP:
		case CT_MARKED:
		case CT_LAYER:
		case CT_MENU:
			return &MCnullstring;
		case CT_STACK:
			return &MCstackstring;
		case CT_VIDEO_CLIP:
			return &MCvideostring;
		case CT_AUDIO_CLIP:
			return &MCaudiostring;
		case CT_BACKGROUND:
			return &MCbackgroundstring;
		case CT_CARD:
			return &MCcardstring;
		case CT_GROUP:
			return &MCgroupstring;
		case CT_BUTTON:
			return &MCbuttonstring;
		case CT_SCROLLBAR:
			return &MCscrollbarstring;
		case CT_IMAGE:
			return &MCimagestring;		
		case CT_GRAPHIC:
			return &MCgraphicstring;
		case CT_EPS:
			return &MCepsstring;
		case CT_MAGNIFY:
			return &MCmagnifierstring;
		case CT_COLOR_PALETTE:
			return &MCcolorstring;
		case CT_WIDGET:
			return &MCwidgetstring;
		case CT_FIELD:
			return &MCfieldstring;
	}
	
	return nil;
}

static bool is_correct(Chunk_term p_chunk, const char **p_string)
{
	return p_string == expected_string(p_chunk);
}

TEST(chunk, type_table)
//
// Checks that the entries of the name table used for MCU_matchname
// correspond to the chunk terms in the Chunk_term enum
//
{
	uint4 type_table_size;
	type_table_size = sizeof(nametable) / sizeof(nametable[0]);

	ASSERT_GE(type_table_size, (unsigned)1);
	
	uint4 expected_size;
	expected_size = CT_LAST_CONTROL - CT_STACK + 1;

	EXPECT_EQ(expected_size, type_table_size);

	for(uint4 i = 0; i < type_table_size - 1; i++) {
		EXPECT_TRUE(is_correct((Chunk_term)(CT_STACK + i), nametable[i]))
			<< "\"" << *nametable[i] << "\""
			<< " name table match";
	}
}
