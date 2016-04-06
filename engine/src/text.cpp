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

#include "text.h"

// p_input - pointer to UTF-16 codepoints
// p_input_length - number of codepoints pointed to by <p_input>
// p_output - pointer to buffer to hold output
// r_used - returns amount of input consumed
// r_made - returns amount of output used
void MCTextRunnify(const unichar_t *p_input, uint4 p_input_length, char_t *p_output, uint4& r_used, uint4& r_made)
{
	uint4 t_index;
	t_index = 0;

	uint4 t_length;
	t_length = p_input_length;

	uint4 t_native_length;
	t_native_length = 0;

	while(t_index < t_length)
	{
		uint4 t_start;
		t_start = t_index;

		uint4 t_codepoint;
		t_codepoint = MCUnicodeCodepointAdvance((const unichar_t*)p_input, p_input_length, t_index);

		while(t_index < t_length)
		{
			uint4 t_old_index;
			t_old_index = t_index;

			t_codepoint = MCUnicodeCodepointAdvance((const unichar_t*)p_input, p_input_length, t_index);

			if (MCUnicodeCodepointIsBase(t_codepoint))
			{
				t_index = t_old_index;
				break;
			}
		}

		if (MCUnicodeMapToNative((const unichar_t*)p_input + t_start, t_index - t_start, p_output[t_native_length]))
		{
			if (t_start > 0 && t_native_length == 0)
			{
				t_index = t_start;
				break;
			}

			t_native_length += 1;
		}
		else
		{
			if (t_native_length > 0)
			{
				t_index = t_start;
				break;
			}
		}
	}

	r_used = t_index;
	r_made = t_native_length;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Text Conversion to Unicode
//

bool MCSTextConvertToUnicode(MCTextEncoding p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used);

bool MCTextEncodeToUnicode(MCTextEncoding p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
{
	bool t_converted;
	t_converted = false;

	uint4 t_used;
	t_used = 0;

	switch(p_input_encoding)
	{
	case kMCTextEncodingUndefined:
		t_converted = true;
	break;

	case kMCTextEncodingASCII:
		if (p_input_length >= p_output_length)
		{
			for(uint4 i = 0; i < p_input_length; ++i)
				((uint2 *)p_output)[i] = ((uint1 *)p_input)[i];
			t_converted = true;
		}
		t_used = p_input_length * 2;
	break;

	default:
		t_converted = MCSTextConvertToUnicode(p_input_encoding, p_input, p_input_length, p_output, p_output_length, t_used);
	break;
	}

	r_used = t_used;
	return t_converted;
}

///////////////////////////////////////////////////////////////////////////////
