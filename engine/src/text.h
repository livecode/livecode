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

#ifndef __MC_TEXT__
#define __MC_TEXT__

enum MCTextEncoding
{
	kMCTextEncodingUndefined,
	kMCTextEncodingASCII,
	kMCTextEncodingNative,
	kMCTextEncodingUTF8,
	kMCTextEncodingUTF16,
	kMCTextEncodingUTF32,

	kMCTextEncodingSymbol,

	kMCTextEncodingMacNative = 1024,
	kMCTextEncodingMacRoman = kMCTextEncodingMacNative,

	kMCTextEncodingWindowsNative = 65536,
	kMCTextEncodingWindows1252 = kMCTextEncodingWindowsNative + 1252
};

enum MCTextListStyle
{
	kMCTextListStyleNone,
	kMCTextListStyleDisc,
	kMCTextListStyleCircle,
	kMCTextListStyleSquare,
	kMCTextListStyleDecimal,
	kMCTextListStyleLowerCaseLetter,
	kMCTextListStyleUpperCaseLetter,
	kMCTextListStyleLowerCaseRoman,
	kMCTextListStyleUpperCaseRoman,
	kMCTextListStyleSkip,
};

enum MCTextTextAlign
{
	kMCTextTextAlignLeft,
	kMCTextTextAlignCenter,
	kMCTextTextAlignRight,
	kMCTextTextAlignJustify
};

struct MCTextBlock
{
	uint4 foreground_color;
	uint4 background_color;
	const char *font_name;
	uint4 font_size;
	uint4 font_style;
	int4 text_shift;

	MCNameRef text_link;
    MCStringRef text_metadata;

	bool string_native;
	const uint2 *string_buffer;
	uint4 string_length;
};

// MW-2012-03-14: [[ RtfParaStyles ]] The paragraph style record used to apply attrs to a
//   new paragraph.
struct MCTextParagraph
{
	MCTextTextAlign text_align;
	MCTextListStyle list_style;
	uint32_t list_depth;

	uint32_t border_width;
	uint32_t padding;
	int32_t first_indent;
	int32_t left_indent;
	int32_t right_indent;
	int32_t space_above;
	int32_t space_below;
	uint32_t tab_count;
	uint16_t *tabs;
	uint32_t background_color;
	uint32_t border_color;
	
    MCStringRef metadata;
};

// MW-2012-03-14: [[ RtfParaStyles ]] The convert callback now takes a 'paragraph' style record
//   which is applied when creating a new paragraph.
typedef bool (*MCTextConvertCallback)(void *p_context, const MCTextParagraph *p_paragraph, const MCTextBlock *p_block);

bool MCTextEncodeToUnicode(MCTextEncoding p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used);

void MCTextRunnify(const unichar_t *p_unicode, uint4 p_unicode_length, char_t *p_native, uint4& r_unicode_used, uint4& r_native_made);

void MCTextSetDefaultEncoding(uint4 p_encoding);
uint4 MCTextGetDefaultEncoding(void);


#endif
