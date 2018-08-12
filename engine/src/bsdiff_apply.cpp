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

#include "prefix.h"
#include "bsdiff.h"

////////////////////////////////////////////////////////////////////////////////

static bool bspatchmain(MCBsDiffInputStream *patch_stream, MCBsDiffInputStream *input_stream, MCBsDiffOutputStream *output_stream);

bool MCBsDiffApply(MCBsDiffInputStream *p_patch_stream, MCBsDiffInputStream *p_input_stream, MCBsDiffOutputStream *p_output_stream)
{
	return bspatchmain(p_patch_stream, p_input_stream, p_output_stream);
}

////////////////////////////////////////////////////////////////////////////////

/*-
 * Copyright 2003-2005 Colin Percival
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


static bool bspatchmain(MCBsDiffInputStream *p_patch_stream, MCBsDiffInputStream *p_input_stream, MCBsDiffOutputStream *p_output_stream)
{
	bool t_success;
	t_success = true;

	// Read in the sizes of all the data arrays

	int32_t t_control_byte_size, t_diff_size, t_extra_size;
	int32_t t_new_size = 0;
	if (t_success)
		t_success =
			p_patch_stream -> ReadInt32(t_control_byte_size) &&
			p_patch_stream -> ReadInt32(t_diff_size) &&
			p_patch_stream -> ReadInt32(t_extra_size) &&
			p_patch_stream -> ReadInt32(t_new_size);

	uindex_t t_old_size;
	if (t_success)
		t_success = p_input_stream -> Measure(t_old_size);

	// Allocate the data arrays

	uint8_t *t_old_data, *t_new_data, *t_diff_data, *t_extra_data;
	int32_t *t_control_data;
	t_old_data = nil;
	if (t_success)
		t_success =
			MCMemoryNewArray(t_old_size, t_old_data) &&
			MCMemoryNewArray(t_new_size, t_new_data) &&
			MCMemoryNewArray(t_control_byte_size / 4, t_control_data) &&
			MCMemoryNewArray(t_diff_size, t_diff_data) &&
			MCMemoryNewArray(t_extra_size, t_extra_data);

	// Read in the data

	if (t_success)
		t_success = p_input_stream -> ReadBytes(t_old_data, t_old_size);

	if (t_success)
		for(int32_t i = 0; i < t_control_byte_size / 4; i++)
			t_success = p_patch_stream -> ReadInt32(t_control_data[i]);

	if (t_success)
		t_success =
			p_patch_stream -> ReadBytes(t_diff_data, t_diff_size) &&
			p_patch_stream -> ReadBytes(t_extra_data, t_extra_size);

	// Now do the processing
	int32_t t_new_pos, t_old_pos, t_ctrl_pos, t_diff_pos, t_extra_pos;
	t_new_pos = 0;
	t_old_pos = 0;
	t_ctrl_pos = 0;
	t_diff_pos = 0;
	t_extra_pos = 0;
	while(t_new_pos < t_new_size && t_success)
	{
		// Extract the control info for the next update
		int32_t t_ctrl_diff, t_ctrl_extra, t_ctrl_old;
		if (t_ctrl_pos + 3 <= t_control_byte_size / 4)
		{
			t_ctrl_diff = t_control_data[t_ctrl_pos + 0];
			t_ctrl_extra = t_control_data[t_ctrl_pos + 1];
			t_ctrl_old = t_control_data[t_ctrl_pos + 2];
			t_ctrl_pos += 3;
		}
		else
			t_success = false;

		// Copy over the diff'd data and add in the old data
		if (t_success &&
			t_new_pos + t_ctrl_diff <= t_new_size &&
			t_diff_pos + t_ctrl_diff <= t_diff_size)
		{
			MCMemoryCopy(t_new_data + t_new_pos, t_diff_data + t_diff_pos, t_ctrl_diff);
			for(int32_t i = 0; i < t_ctrl_diff; i++)
				if (t_old_pos + i >= 0 && t_old_pos + i < (signed)t_old_size)
					t_new_data[t_new_pos + i] += t_old_data[t_old_pos + i];

			t_new_pos += t_ctrl_diff;
			t_old_pos += t_ctrl_diff;
			t_diff_pos += t_ctrl_diff;
		}
		else
			t_success = false;

		// Copy across the extra data
		if (t_success &&
			t_new_pos + t_ctrl_extra <= t_new_size &&
			t_extra_pos + t_ctrl_extra <= t_extra_size)
		{
			MCMemoryCopy(t_new_data + t_new_pos, t_extra_data + t_extra_pos, t_ctrl_extra);

			t_new_pos += t_ctrl_extra;
			t_extra_pos += t_ctrl_extra;
			t_old_pos += t_ctrl_old;
		}
	}

	// Write the diff'd file
	if (t_success)
		t_success = p_output_stream -> WriteBytes(t_new_data, t_new_size);

	MCMemoryDeleteArray(t_extra_data);
	MCMemoryDeleteArray(t_diff_data);
	MCMemoryDeleteArray(t_control_data);
	MCMemoryDeleteArray(t_new_data);
	MCMemoryDeleteArray(t_old_data);

	return t_success;
}
