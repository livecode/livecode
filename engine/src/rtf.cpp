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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "text.h"
#include "rtf.h"

//

#ifdef _MACOSX
#define IMPORT_RTF_PLATFORM_DPI 72
#else
#define IMPORT_RTF_PLATFORM_DPI 96
#endif

//

#define ANSI_CHARSET 0
#define DEFAULT_CHARSET 1
#define SYMBOL_CHARSET 2
#define MAC_CHARSET 77
#define SHIFTJIS_CHARSET 128
#define HANGEUL_CHARSET 129
#define JOHAB_CHARSET 130
#define GB2312_CHARSET 134
#define CHINESEBIG5_CHARSET 136
#define GREEK_CHARSET 161
#define TURKISH_CHARSET 162
#define VIETNAMESE_CHARSET 163
#define HEBREW_CHARSET 177
#define ARABIC_CHARSET 178
#define BALTIC_CHARSET 186
#define RUSSIAN_CHARSET 204
#define THAI_CHARSET 222
#define EASTEUROPE_CHARSET 238
#define PC437_CHARSET 254
#define OEM_CHARSET 255

//

static int4 StringToInteger(const char *p_string, int4 p_length)
{
	if (p_length == 0)
		return 0;
		
	bool t_negative;
	t_negative = false;
	if (*p_string == '-')
	{
		p_string++;
		p_length--;
		t_negative = true;
	}
		
	int4 t_result;
	t_result = 0;
	while(p_length > 0)
	{
		t_result *= 10;
		t_result += *p_string - '0';
		
		p_string++;
		p_length--;
	}
	
	if (t_negative)
		t_result = -t_result;
		
	return t_result;
}

static int4 StringCompare(const char *p_left, int4 p_left_length, const char *p_right, int4 p_right_length)
{
	if (p_left_length == -1)
		p_left_length = strlen(p_left);
	
	if (p_right_length == -1)
		p_right_length = strlen(p_right);
		
	int4 t_comparison;
	t_comparison = strncmp(p_left, p_right, MCU_min(p_left_length, p_right_length));
		
	if (p_left_length == p_right_length)
		return t_comparison;
	
	if (t_comparison == 0)
		return p_left_length - p_right_length;

	return t_comparison;
}

static char *CStringAppend(char *p_string, uint1 p_char)
{
	uint4 t_new_length;
	t_new_length = p_string == NULL ? 1 : strlen(p_string) + 1;
	
	char *t_new_string;
	t_new_string = (char *)realloc(p_string, t_new_length + 1);
	if (t_new_string == NULL)
		return NULL;
		
	t_new_string[t_new_length - 1] = p_char;
	t_new_string[t_new_length] = '\0';
	
	return t_new_string;
}

int CStringCompare(const char *a, const char *b)
{
	if (a == NULL)
		return b == NULL ? 0 : -1;
	if (b == NULL)
		return 1;
	return strcmp(a, b);
}

//

static MCTextListStyle bullet_char_to_liststyle(uint16_t p_codepoint)
{
	MCTextListStyle t_style;
	t_style = kMCTextListStyleDisc;

	switch(p_codepoint)
	{
	case 0x2022:
		t_style = kMCTextListStyleDisc;
		break;
	case 0x25E6:
		t_style = kMCTextListStyleCircle;
		break;
	case 0x25AA:
		t_style = kMCTextListStyleSquare;
		break;
	default:
		break;
	}

	return t_style;
}

//

RTFReader::RTFReader(void)
{
	m_converter = NULL;
	m_converter_context = NULL;

	m_input = NULL;
	m_input_end = NULL;
	m_input_binary_count = 0;
	m_input_state = kRTFInputStateNormal;
	m_input_skip_count = 0;
	
	m_font_started = false;
	m_font_name = NULL;
	m_font_index = 0;
	m_font_charset = 0;
	m_font_skip = false;
	
	m_color_started = false;
	m_color_red = 0;
	m_color_green = 0;
	m_color_blue = 0;

	m_table_cell = false;

	m_default_text_encoding = kMCTextEncodingWindows1252;
	m_default_font = -1;

	m_listtable_level_count = 0;
	
	m_list_style = kMCTextListStyleNone;
	m_list_level = 0;

	m_field_inst = nil;

	m_attributes_changed = false;
	m_attributes . foreground_color = 0xffffffff;
	m_attributes . background_color = 0xffffffff;
	m_attributes . font_name = NULL;
	m_attributes . font_style = 0;
	m_attributes . font_size = 0;
	m_attributes . text_link = NULL;
	m_attributes . text_shift = 0;
	m_attributes . text_metadata = NULL;

	m_attributes . string_native = false;
	m_attributes . string_buffer = NULL;
	m_attributes . string_length = 0;

	// MW-2012-03-14: [[ RtfParaStyles ]] Make sure we create a paragraph when
	//   the first text is output to ensure paragraph styles are applied
	//   correctly.
	m_needs_paragraph = true;
}

RTFReader::~RTFReader(void)
{
	MCValueRelease(m_attributes . text_link);
    MCValueRelease(m_attributes . text_metadata);

	if (m_font_name != NULL)
		free(m_font_name);
	if (m_field_inst != NULL)
		free(m_field_inst);
}

void RTFReader::SetConverter(MCTextConvertCallback p_writer, void *p_context)
{
	m_converter = p_writer;
	m_converter_context = p_context;
}

RTFStatus RTFReader::Process(const char *p_input, uint4 p_input_length)
{
	m_input = p_input;
	m_input_end = p_input + p_input_length;

	m_input_binary_count = 0;
	m_input_state = kRTFInputStateNormal;
	
	RTFStatus t_status;
	t_status = Parse();
	if (t_status == kRTFStatusSuccess)
		t_status = Flush(true);

	return t_status;
}

RTFStatus RTFReader::Parse(void)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;
			
	while(t_status == kRTFStatusSuccess)
	{
		RTFToken t_token;
		int4 t_value;
		
		if (m_input_skip_count > 0)
		{
			while(m_input_skip_count > 0)
			{
				t_status = ParseToken(t_token, t_value);
				if (t_status != kRTFStatusSuccess)
					break;

				if (t_token == kRTFTokenEnd || t_token == kRTFTokenBeginGroup || t_token == kRTFTokenEndGroup)
				{
					m_input_skip_count = 0;
					break;
				}

				if ((t_token & kRTFTokenMask) == kRTFTokenBin)
					m_input += t_value;

				m_input_skip_count -= 1;
			}
		}

		if (t_status == kRTFStatusSuccess)
			t_status = ParseToken(t_token, t_value);

		if (t_status != kRTFStatusSuccess)
			break;

		if (t_token == kRTFTokenEnd)
			break;
			
		if (t_token == kRTFTokenBeginGroup)
			t_status = m_state . Save();
		else if (t_token == kRTFTokenEndGroup)
		{
			// Take into account implementation of 'destinations'.
			bool t_was_list;
			t_was_list = m_state . GetDestination() == kRTFDestinationLegacyList;

			bool t_was_list_text;
			t_was_list_text = m_state . GetDestination() == kRTFDestinationListText;

			bool t_was_field;
			t_was_field = m_state . GetDestination() == kRTFDestinationFldInst;

			// MW-2014-01-08: [[ Bug 11627 ]] If the paragraph attributes have changed then
			//   force a flush so that a new paragraph with said attributes is created. [ This
			//   isn't 100% correct from my reading of the RTF Spec - really paragraph attrs
			//   should be set on the current paragraph as the paragraph is parsed, rather than
			//   before the first text is emitted - however due to the way LiveCode and Word Processors
			//   generate RTF, this at least makes things roundtrip ].
			if (m_state . HasParagraphChanged())
				Flush(true);

			t_status = m_state . Restore();

			if (t_was_list)
			{
				m_state . SetListStyle(m_list_style);
				m_state . SetListLevel(m_list_level);
			}
			else if (t_was_list_text)
			{
				if (m_list_skip)
					m_state . SetListStyle(kMCTextListStyleSkip);
			}
			else if (t_was_field && m_state . GetDestination() != kRTFDestinationFldInst)
			{
				ProcessField();
			}

			m_attributes_changed = true;
		}
		else if ((t_token & kRTFTokenMask) == kRTFTokenBin)
		{
			m_input_binary_count = t_value;
			m_input_state = kRTFInputStateBinary;
		}
		else switch(m_state . GetDestination())
		{
		case kRTFDestinationSkip:
			// If the skipped destination is in fact the 'list' destination then
			// handle it. We ignore 'pn' destinations if we have a listtable though.
			if (t_token == kRTFTokenLegacyList && m_lists . Count() == 0)
			{
				m_list_style = kMCTextListStyleNone;
				m_list_level = 0;
				m_state . SetDestination(kRTFDestinationLegacyList);
			}
			else if (t_token == kRTFTokenListTable)
				m_state . SetDestination(kRTFDestinationListTable);
			else if (t_token == kRTFTokenListOverrideTable)
				m_state . SetDestination(kRTFDestinationListOverrideTable);
			else if (t_token == kRTFTokenFldInst)
				m_state . SetDestination(kRTFDestinationFldInst);
		break;
		
		case kRTFDestinationNormal:
			t_status = ParseDocument(t_token, t_value);
		break;
		
		case kRTFDestinationFontTable:
			t_status = ParseFontTable(t_token, t_value);
		break;
		
		case kRTFDestinationColorTable:
			t_status = ParseColorTable(t_token, t_value);
		break;

		case kRTFDestinationLegacyList:
			t_status = ParseLegacyList(t_token, t_value);
		break;

		case kRTFDestinationLegacyListPrefix:
			t_status = ParseLegacyListPrefix(t_token, t_value);
		break;
				
		case kRTFDestinationLegacyListSuffix:
			break;
				
		case kRTFDestinationListTable:
			t_status = ParseListTable(t_token, t_value);
		break;
				
		case kRTFDestinationListTableLevelText:
			t_status = ParseListTableLevelText(t_token, t_value);
		break;
				
		case kRTFDestinationListOverrideTable:
			t_status = ParseListOverrideTable(t_token, t_value);
		break;

		case kRTFDestinationListText:
			t_status = ParseListText(t_token, t_value);
		break;

		case kRTFDestinationField:
		break;

		case kRTFDestinationFldInst:
			t_status = ParseFldInst(t_token, t_value);
		break;
		}
	}
	
	return t_status;
}

RTFStatus RTFReader::ParseToken(RTFToken& r_token, int4& r_parameter)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;
	
	while(m_input < m_input_end && (*m_input == '\n' || *m_input == '\r'))
		m_input++;
	
	switch(m_input_state)
	{
	case kRTFInputStateNormal:
	{
		// MW-2013-07-31: [[ Bug 10972 ]] Don't use '\0' to indicate end
		//   of RTF, just break directly (Adobe Reader 9 on Mac puts \0 in
		//   font names sometimes causing premature end of RTF parsing).
		if (m_input == m_input_end)
		{
			r_token = kRTFTokenEnd;
			break;
		}
		
		int t_char;
		t_char = *m_input;
		
		switch(t_char)
		{
		case '{':
			r_token = kRTFTokenBeginGroup;
			m_input++;
		break;
		
		case '}':
			r_token = kRTFTokenEndGroup;
			m_input++;
		break;
		
		case '\\':
		{
			m_input++;
			if (m_input == m_input_end)
				t_char = '\0';
			else
				t_char = *m_input;
			
			char t_keyword[32];
			int t_keyword_length;
			t_keyword_length = 0;
			
			char t_parameter[20];
			int t_parameter_length;
			t_parameter_length = 0;
		
			if (t_char == '\0')
			{
				// End of stream in a keyword is an error
				t_status = kRTFStatusIncompleteKeyword;
			}
			else if (t_char == '\'')
			{
				// Handle hex characters here (simplifies skipping)
				m_input++;
				if (m_input == m_input_end)
					t_char = '\0';
				else
					t_char = *m_input;
			
				int t_first_nibble;
				t_status = LookupNibble(t_char, t_first_nibble);
					
				int t_second_nibble;
				if (t_status == kRTFStatusSuccess)
				{
					m_input++;
					if (m_input == m_input_end)
						t_char = '\0';
					else
						t_char = *m_input;
					
					t_status = LookupNibble(t_char, t_second_nibble);
				}
				
				if (t_status == kRTFStatusSuccess)
				{
					m_input++;

					r_token = kRTFTokenCharacter | kRTFTokenHasParameter;
					r_parameter = (t_first_nibble << 4) | t_second_nibble;
				}
			}
			else if (t_char == '{' || t_char == '}' || t_char == '\\')
			{
				m_input++;

				r_token = kRTFTokenCharacter | kRTFTokenHasParameter;
				r_parameter = (uint1)t_char;
			}
			else
			{
				if ((t_char < 'a' || t_char > 'z' ) && (t_char < 'A' || t_char > 'Z'))
				{
					// Any non-letter is a control symbol
					t_keyword[0] = t_char;
					t_keyword_length = 1;
					
					m_input++;
				}
				else
				{
					// We have a letter so parse a control word
					while((t_char >= 'a' && t_char <= 'z') || (t_char >= 'A' && t_char <= 'Z'))
					{
						if (t_char == '\0')
						{
							t_status = kRTFStatusIncompleteKeyword;
							break;
						}
						else if (t_keyword_length == 31)
						{
							t_status = kRTFStatusKeywordTooLong;
							break;
						}
						else
							t_keyword[t_keyword_length++] = t_char;
						
						m_input++;
						if (m_input == m_input_end)
							t_char = '\0';
						else
							t_char = *m_input;
					}
					
					// Now we parse a parameter if the current char is '-' or a digit
					if (t_char == '-' || (t_char >= '0' && t_char <= '9'))
					{
						t_parameter[t_parameter_length++] = t_char;
						
						m_input++;
						if (m_input == m_input_end)
							t_char = '\0';
						else
							t_char = *m_input;
						
						while(t_char >= '0' && t_char <= '9')
						{
							if (t_char == '\0')
							{
								t_status = kRTFStatusIncompleteKeyword;
								break;
							}
							else if (t_parameter_length == 19)
							{
								t_status = kRTFStatusParameterTooLong;
								break;
							}
							else
								t_parameter[t_parameter_length++] = t_char;
								
							m_input++;
							if (m_input == m_input_end)
								t_char = '\0';
							else
								t_char = *m_input;
						}
					}
					
					// Now skip the delimiter if its a space
					if (t_char == ' ')
						m_input++;
				}
				
				if (t_status == kRTFStatusSuccess)
					t_status = LookupKeyword(t_keyword, t_keyword_length, r_token);
				
				if (t_status == kRTFStatusSuccess && t_parameter_length != 0)
				{
					r_token |= kRTFTokenHasParameter;
					r_parameter = StringToInteger(t_parameter, t_parameter_length);
				}
			}
		}
		break;
		
		default:
			m_input++;
			
			r_token = kRTFTokenCharacter | kRTFTokenHasParameter;
			r_parameter = (int)((unsigned char)t_char);
		break;
		}
	}
	break;
	
	case kRTFInputStateBinary:
	{
		int4 t_char;
		if (m_input != m_input_end)
		{
			t_char = *m_input;
			m_input++;
		}
        else
            t_char = '\0';
	
		m_input_binary_count -= 1;
		if (m_input_binary_count == 0)
			m_input_state = kRTFInputStateNormal;
			
		r_token = kRTFTokenCharacter | kRTFTokenHasParameter;
		r_parameter = (int)((unsigned char)t_char);
	}
	break;
	
	case kRTFInputStateHex:
	{
		int4 t_char;
		if (m_input != m_input_end)
			t_char = *m_input;
		else
			t_char = '\0';
		m_input++;
	
		int t_first_nibble;
		t_status = LookupNibble(t_char, t_first_nibble);
			
		int t_second_nibble;
		if (t_status == kRTFStatusSuccess)
		{
			if (m_input != m_input_end)
				t_char = *m_input;
			else
				t_char = '\0';
			m_input++;
			
			t_status = LookupNibble(t_char, t_second_nibble);
		}
		
		if (t_status == kRTFStatusSuccess)
		{
			r_token = kRTFTokenCharacter | kRTFTokenHasParameter;
			r_parameter = (t_first_nibble << 4) | t_second_nibble;
			
			m_input_state = kRTFInputStateNormal;
		}
	}
	break;
	}
	
	return t_status;
}

RTFStatus RTFReader::ParseDocument(RTFToken p_token, int4 p_value)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;

	RTFToken t_token;
	t_token = p_token & kRTFTokenMask;

	bool t_has_parameter;
	t_has_parameter = (p_token & kRTFTokenHasParameter) != 0;

	switch(t_token)
	{
	case kRTFTokenSkipDestination:
		m_state . SetDestination(kRTFDestinationSkip);
	break;
	
	case kRTFTokenFontTable:
		m_state . SetDestination(kRTFDestinationFontTable);
	break;
	
	case kRTFTokenColorTable:
		m_state . SetDestination(kRTFDestinationColorTable);
	break;
	
	case kRTFTokenListTable:
		m_state . SetDestination(kRTFDestinationListTable);
	break;

	case kRTFTokenListText:
		m_list_skip = true;
		m_state . SetDestination(kRTFDestinationListText);
	break;
			
	case kRTFTokenListOverrideTable:
		m_state . SetDestination(kRTFDestinationListOverrideTable);
	break;

	case kRTFTokenFldInst:
		m_state . SetDestination(kRTFDestinationFldInst);
	break;

	case kRTFTokenField:
	break;
			
	case kRTFTokenDefaultFont:
		if (t_has_parameter)
			m_default_font = p_value;
	break;

	case kRTFTokenAnsi:
		m_default_text_encoding = kMCTextEncodingWindows1252;
	break;

	case kRTFTokenMac:
		m_default_text_encoding = kMCTextEncodingMacRoman;
	break;

	case kRTFTokenPC:
		m_default_text_encoding = (MCTextEncoding)(kMCTextEncodingWindowsNative + 437);
	break;

	case kRTFTokenPCA:
		m_default_text_encoding = (MCTextEncoding)(kMCTextEncodingWindowsNative + 850);
	break;

	case kRTFTokenAnsiCodepage:
		if (t_has_parameter)
		{
			if (p_value < 0)
				p_value = 65536 + p_value;
			
			m_default_text_encoding = (MCTextEncoding)(kMCTextEncodingWindowsNative + p_value);
		}
	break;

	case kRTFTokenUnicodeSkip:
		if (t_has_parameter)
			m_state . SetUnicodeSkip(p_value);
	break;

	case kRTFTokenNewLine:
	case kRTFTokenParagraph:
		t_status = Flush(true);
		if (t_status == kRTFStatusSuccess)
		{
			if (m_needs_paragraph)
				Paragraph();
			m_needs_paragraph = true;
		}
	break;

	case kRTFTokenResetParagraphStyle:
		// MW-2012-03-14: [[ RtfParaStyles ]] Reset all the paragraph styles to defaults.
		m_state . SetListStyle(kMCTextListStyleNone);
		m_state . SetListLevel(0);
		m_state . SetListIndex(0);
		m_state . SetBorderWidth(0);
		m_state . SetPadding(0);
		m_state . SetFirstIndent(0);
		m_state . SetLeftIndent(0);
		m_state . SetRightIndent(0);
		m_state . SetSpaceAbove(0);
		m_state . SetSpaceBelow(0);
		m_state . SetParagraphBackgroundColor(0xffffffff);
		m_state . SetBorderColor(0xffffffff);
			
		// MW-2014-01-08: [[ Bug 11627 ]] Make sure the text alignment attribute is reset.
		m_state . SetTextAlign(kMCTextTextAlignLeft);
		break;

	case kRTFTokenRow:
		m_table_cell = false;

		t_status = Flush(true);
		if (t_status == kRTFStatusSuccess)
		{
			if (m_needs_paragraph)
				Paragraph();
			m_needs_paragraph = true;
		}
	break;

	case kRTFTokenCell:
		if (m_table_cell)
		{
			m_table_cell = false;
			t_status = m_text . Output(9, kMCTextEncodingUTF16);
		}
		if (t_status == kRTFStatusSuccess && m_attributes_changed)
			t_status = Flush();
		if (t_status == kRTFStatusSuccess)
			m_table_cell = true;
	break;

	case kRTFTokenColor:
		if (t_has_parameter)
		{
			m_state .  SetForegroundColor(m_colors . Get(p_value));
			m_attributes_changed = true;
		}
	break;

	case kRTFTokenHighlight:
		if (t_has_parameter)
		{
			m_state . SetBackgroundColor(m_colors . Get(p_value));
			m_attributes_changed = true;
		}
	break;

	case kRTFTokenPlain:
		m_state . SetFontName(nil);
		m_state . SetFontStyle(kRTFFontStyleNone);
		m_state . SetFontSize(0);
		m_attributes_changed = true;
	break;

	case kRTFTokenFont:
		if (t_has_parameter)
		{
			static struct { int charset; MCTextEncoding encoding; } s_charset_mapping[] =
			{
				{ ANSI_CHARSET, kMCTextEncodingWindows1252 },
				{ SYMBOL_CHARSET, kMCTextEncodingSymbol },
				{ MAC_CHARSET , kMCTextEncodingMacRoman },
				{ SHIFTJIS_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 932) },
				{ HANGEUL_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 949) },
				{ JOHAB_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 1361) },
				{ GB2312_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 936) },
				{ CHINESEBIG5_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 950) },
				{ GREEK_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 1253) },
				{ TURKISH_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 1254) },
				{ VIETNAMESE_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 1258) },
				{ HEBREW_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 1255) },
				{ ARABIC_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 1256) },
				{ BALTIC_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 1257) },
				{ RUSSIAN_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 1251) },
				{ THAI_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 874) },
				{ EASTEUROPE_CHARSET, MCTextEncoding(kMCTextEncodingWindowsNative + 1250) },
				{ /* PC437_CHARSET */ 254, MCTextEncoding(kMCTextEncodingWindowsNative + 437) },
				{ -1, kMCTextEncodingUndefined }
			};

			const char *t_new_font;
			t_new_font = m_fonts . GetName(p_value);

			uint4 t_new_charset;
			t_new_charset = m_fonts . GetCharset(p_value);

			MCTextEncoding t_new_encoding;
			if (t_new_charset == DEFAULT_CHARSET)
				t_new_encoding = m_default_text_encoding;
			else
			{
				uint4 t_charset_index;
				for(t_charset_index = 0; s_charset_mapping[t_charset_index] . charset != -1; ++t_charset_index)
					if (s_charset_mapping[t_charset_index] . charset == t_new_charset)
						break;
				t_new_encoding = s_charset_mapping[t_charset_index] . encoding;
			}

			m_state . SetTextEncoding(t_new_encoding);
			m_state . SetFontName(t_new_font);
			m_attributes_changed = true;
		}
	break;

	case kRTFTokenFontSize:
		if (t_has_parameter)
		{
			m_state . SetFontSize(p_value);
			m_attributes_changed = true;
		}
	break;

	case kRTFTokenBold:
	case kRTFTokenItalic:
	case kRTFTokenUnderline:
	case kRTFTokenNoUnderline:
	case kRTFTokenStrikethrough:
	case kRTFTokenSuperscript:
	case kRTFTokenSubscript:
	{
		bool t_turn_off;
		t_turn_off = t_has_parameter && p_value == 0;

		RTFFontStyle t_mask = 0;
		if (t_token == kRTFTokenBold)
			t_mask = kRTFFontStyleBold;
		else if (t_token == kRTFTokenItalic)
			t_mask = kRTFFontStyleItalic;
		else if (t_token == kRTFTokenUnderline)
			t_mask = kRTFFontStyleUnderline;
		else if (t_token == kRTFTokenNoUnderline)
		{
			t_mask = kRTFFontStyleUnderline;
			t_turn_off = true;
		}
		else if (t_token == kRTFTokenStrikethrough)
			t_mask = kRTFFontStyleStrikethrough;
		else if (t_token == kRTFTokenSuperscript)
			t_mask = kRTFFontStyleSuperscript;
		else if (t_token == kRTFTokenSubscript)
			t_mask = kRTFFontStyleSubscript;

		RTFFontStyle t_new_style;
		if (t_turn_off)
			t_new_style = m_state . GetFontStyle() & ~t_mask;
		else
			t_new_style = m_state . GetFontStyle() | t_mask;

		m_state . SetFontStyle(t_new_style);
		m_attributes_changed = true;
	}
	break;

	case kRTFTokenTab:
		if (t_status == kRTFStatusSuccess && m_table_cell)
		{
			m_table_cell = false;
			t_status = m_text . Output(9, kMCTextEncodingUTF16);
		}
		if (m_attributes_changed)
			t_status = Flush();
		if (t_status == kRTFStatusSuccess)
			t_status = m_text . Output(9, kMCTextEncodingUTF16);
	break;

	case kRTFTokenUnicode:
	{
		if (t_has_parameter)
		{
			if (t_status == kRTFStatusSuccess && m_table_cell)
			{
				m_table_cell = false;
				t_status = m_text . Output(9, kMCTextEncodingUTF16);
			}
			
			// MW-2014-03-14: [[ Bug 11771 ]] On Mac, HTML on the clipboard is translated
			//   to RTF with LINE SEPARATOR instead of BR. So map both LINE SEPARATOR and
			//   PARAGRAPH SEPARATOR to a new paragraph marker. (This is consistent with the
			//   handling of newline and other related markers in the RTF and means
			//   scripts won't get tripped up).
			if ((p_value & 0xFFFF) == 0x2028 || (p_value & 0xFFFF) == 0x2029)
			{	
				t_status = Flush(true);
				if (t_status == kRTFStatusSuccess)
				{
					if (m_needs_paragraph)
						Paragraph();
					m_needs_paragraph = true;
				}
			}
			else
			{
				if (m_attributes_changed)
					t_status = Flush();
				if (t_status == kRTFStatusSuccess)
					t_status = m_text . Output(p_value & 0xFFFF, kMCTextEncodingUTF16);
				if (t_status == kRTFStatusSuccess)
					m_input_skip_count = m_state . GetUnicodeSkip();
			}
		}
	}
	break;

	case kRTFTokenCharacter:
	{
		MCTextEncoding t_encoding;
		t_encoding = m_state . GetTextEncoding();
		if (t_encoding == kMCTextEncodingUndefined)
			t_encoding = m_default_text_encoding;
		if (t_status == kRTFStatusSuccess && m_table_cell)
		{
			m_table_cell = false;
			t_status = m_text . Output(9, kMCTextEncodingUTF16);
		}
		if (m_attributes_changed)
			t_status = Flush();
		if (t_status == kRTFStatusSuccess)
			t_status = m_text . Output(p_value, t_encoding);
	}
	break;

	// MW-2010-01-08: [[ Bug 8143 ]] Make sure we handle special chars and map then to UTF-16
	case kRTFTokenBullet:
		t_status = m_text . Output(0x2022, kMCTextEncodingUTF16);
		break;
	case kRTFTokenLeftQuote:
		t_status = m_text . Output(0x2018, kMCTextEncodingUTF16);
		break;
	case kRTFTokenRightQuote:
		t_status = m_text . Output(0x2019, kMCTextEncodingUTF16);
		break;
	case kRTFTokenLeftDoubleQuote:
		t_status = m_text . Output(0x201C, kMCTextEncodingUTF16);
		break;
	case kRTFTokenRightDoubleQuote:
		t_status = m_text . Output(0x201D, kMCTextEncodingUTF16);
		break;
			
	// Handle the 'listselect' style lists.
	case kRTFTokenListSelect:
		if (t_has_parameter)
			m_state . SetListIndex(p_value);
	break;

	// MW-2012-03-14: [[ RtfParaStyles ]] Handle the list level select.
	case kRTFTokenListSelectLevel:
		if (t_has_parameter && p_value >= 0 && p_value <= 8)
			m_state . SetListLevel(p_value);
		break;

	// MW-2012-03-14: [[ RtfParaStyles ]] Handle all the paragraph style tags.
	case kRTFTokenParagraphBackgroundColor:
		if (t_has_parameter)
			m_state . SetParagraphBackgroundColor(m_colors . Get(p_value));
		break;
	case kRTFTokenParagraphBorderWidth:
		if (t_has_parameter)
			m_state . SetBorderWidth(p_value);
		break;
	case kRTFTokenParagraphBorderColor:
		if (t_has_parameter)
			m_state . SetBorderColor(m_colors . Get(p_value));
		break;
	case kRTFTokenParagraphPadding:
		if (t_has_parameter)
			m_state . SetPadding(p_value);
		break;
	case kRTFTokenLeftIndent:
		if (t_has_parameter)
			m_state . SetLeftIndent(p_value);
		break;
	case kRTFTokenRightIndent:
		if (t_has_parameter)
			m_state . SetRightIndent(p_value);
		break;
	case kRTFTokenFirstIndent:
		if (t_has_parameter)
			m_state . SetFirstIndent(p_value);
		break;
	case kRTFTokenSpaceAbove:
		if (t_has_parameter)
			m_state . SetSpaceAbove(p_value);
		break;
	case kRTFTokenSpaceBelow:
		if (t_has_parameter)
			m_state . SetSpaceBelow(p_value);
		break;
	case kRTFTokenLeftJustify:
		m_state . SetTextAlign(kMCTextTextAlignLeft);
		break;
	case kRTFTokenRightJustify:
		m_state . SetTextAlign(kMCTextTextAlignRight);
		break;
	case kRTFTokenCenterJustify:
		m_state . SetTextAlign(kMCTextTextAlignCenter);
		break;
	}
	
	return t_status;
}

RTFStatus RTFReader::ParseFontTable(RTFToken p_token, int4 p_value)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;

	switch(p_token & kRTFTokenMask)
	{
		case kRTFTokenSkipDestination:
			m_font_skip = true;
		break;
		case kRTFTokenFont:
			m_font_skip = false;
			m_font_started = true;
			m_font_index = p_value;
		break;
		case kRTFTokenFontCharset:
			m_font_skip = false;
			m_font_charset = p_value;
		break;
		case kRTFTokenFontName:
			m_font_skip = false;
		break;
		case kRTFTokenCharacter:
			m_font_skip = false;
			if (m_font_started)
			{
				if (p_value == ';')
				{
					t_status = m_fonts . Define(m_font_index, m_font_name, m_font_charset);
					if (t_status == kRTFStatusSuccess)
					{
						m_font_started = false;
						m_font_name = NULL;
					}
				}
				else
				{
					m_font_name = CStringAppend(m_font_name, p_value & 0xff);
					if (m_font_name == NULL)
						t_status = kRTFStatusNoMemory;
				}
			}
		break;

		default:
			if (m_font_skip)
			{
				m_state . SetDestination(kRTFDestinationSkip);
				m_font_skip = false;
			}
		break;
	}
	return t_status;
}

RTFStatus RTFReader::ParseField(RTFToken p_token, int4 p_value)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;

	switch(p_token & kRTFTokenMask)
	{
		case kRTFTokenSkipDestination:
			break;

		case kRTFTokenFldInst:
			m_state . SetDestination(kRTFDestinationFldInst);
			break;
	}

	return t_status;
}

RTFStatus RTFReader::ParseFldInst(RTFToken p_token, int4 p_value)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;

	switch(p_token & kRTFTokenMask)
	{
		case kRTFTokenSkipDestination:
			m_state . SetDestination(kRTFDestinationSkip);
			break;

		case kRTFTokenCharacter:
			m_field_inst = CStringAppend(m_field_inst, p_value & 0xff);
			if (m_field_inst == nil)
				t_status = kRTFStatusNoMemory;
			break;
	}

	return t_status;
}

void RTFReader::ProcessField(void)
{
	if (m_field_inst == nil)
		return;

	char *t_type, *t_data;
	t_type = m_field_inst;
	while(isspace(*t_type))
		t_type++;
	t_data = strchr(t_type, ' ');
	if (t_data != nil)
	{
		char *t_type_end;
		t_type_end = t_data;

		while(isspace(*t_data))
			t_data++;
	
		*t_type_end = '\0';

		if (*t_data == '"')
		{
			char *t_data_end;
			t_data += 1;
			t_data_end = t_data;
			while(*t_data_end != '\0' && *t_data_end != '\"')
				t_data_end++;
			if (*t_data_end == '\"')
				*t_data_end = '\0';
			else
				t_data = nil;
		}
		else
			t_data = nil;		
	}

	if (t_type != nil && t_data != nil)
	{
		MCNameRef t_name;
		/* UNCHECKED */ MCNameCreateWithNativeChars((const char_t *)t_data, strlen(t_data), t_name);
        
        MCAutoStringRef t_string;
        /* UNCHECKED */ MCStringCreateWithCString(t_data, &t_string);
		if (MCU_strcasecmp(t_type, "HYPERLINK") == 0)
		{
            m_state . SetHyperlink(t_name);
			m_state . SetFontStyle(m_state . GetFontStyle() | kRTFFontStyleLink);
		}
		else if (MCU_strcasecmp(t_type, "LCANCHOR") == 0)
		{
			m_state . SetHyperlink(t_name);
		}
		else if (MCU_strcasecmp(t_type, "LCMETADATA") == 0)
        {
            m_state . SetMetadata(*t_string);
		}
		else if (MCU_strcasecmp(t_type, "LCLINEMETADATA") == 0)
        {
            m_state . SetParagraphMetadata(*t_string);
		}
		MCValueRelease(t_name);
	}

	free(m_field_inst);
	m_field_inst = nil;
}

RTFStatus RTFReader::ParseColorTable(RTFToken p_token, int4 p_value)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;

	switch(p_token & kRTFTokenMask)
	{
		case kRTFTokenSkipDestination:
			m_state . SetDestination(kRTFDestinationSkip);
		break;
	
		case kRTFTokenRed:
			m_color_started = true;
			m_color_red = p_value;
		break;
		case kRTFTokenGreen:
			m_color_started = true;
			m_color_green = p_value;
		break;
		case kRTFTokenBlue:
			m_color_started = true;
			m_color_blue = p_value;
		break;
		case kRTFTokenCharacter:
			if (p_value == ';')
			{
				if (m_color_started)
				{
					t_status = m_colors . Define(m_color_red | (m_color_green << 8) | (m_color_blue << 16));
					if (t_status == kRTFStatusSuccess)
						m_color_started = false;
				}
				else
					m_colors . Define(0xffffffff);
			}
		break;
	}
	return t_status;
}

RTFStatus RTFReader::ParseLegacyList(RTFToken p_token, int4 p_value)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;

	RTFToken t_token;
	t_token = p_token & kRTFTokenMask;

	bool t_has_parameter;
	t_has_parameter = (p_token & kRTFTokenHasParameter) != 0;

	switch(t_token)
	{
	case kRTFTokenLegacyListLevel:
		m_list_level = MCU_max(MCU_min(p_value, 9), 1);
		break;
	case kRTFTokenLegacyListBulletLevel:
		m_list_style = kMCTextListStyleDisc;
		m_list_level = 1;
		break;
	case kRTFTokenLegacyListBodyLevel:
		m_list_style = kMCTextListStyleDecimal;
		m_list_level = 1;
		break;
	case kRTFTokenLegacyListDecimalNumbering:
		m_list_style = kMCTextListStyleDecimal;
		break;
	case kRTFTokenLegacyListLowercaseAlphabeticNumbering:
		m_list_style = kMCTextListStyleLowerCaseLetter;
		break;
	case kRTFTokenLegacyListLowercaseRomanNumbering:
		m_list_style = kMCTextListStyleLowerCaseRoman;
		break;
	case kRTFTokenLegacyListUppercaseAlphabeticNumbering:
		m_list_style = kMCTextListStyleUpperCaseLetter;
		break;
	case kRTFTokenLegacyListUppercaseRomanNumbering:
		m_list_style = kMCTextListStyleUpperCaseRoman;
		break;
	case kRTFTokenLegacyListPrefixText:
		m_state . SetDestination(kRTFDestinationLegacyListPrefix);
		break;
	case kRTFTokenLegacyListSuffixText:
		m_state . SetDestination(kRTFDestinationSkip);
		break;
	}

	return t_status;
}

RTFStatus RTFReader::ParseLegacyListPrefix(RTFToken p_token, int4 p_value)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;

	bool t_has_parameter;
	t_has_parameter = (p_token & kRTFTokenHasParameter) != 0;

	uint16_t t_bullet_char;
	t_bullet_char = 0;

	switch(p_token & kRTFTokenMask)
	{
		case kRTFTokenSkipDestination:
			m_state . SetDestination(kRTFDestinationSkip);
		break;

		case kRTFTokenUnicode:
		{
			if (t_has_parameter)
			{
				t_bullet_char = p_value & 0xFFFF;
				m_input_skip_count = m_state . GetUnicodeSkip();
			}
		}
		break;

		case kRTFTokenCharacter:
		{
			MCTextEncoding t_encoding;
			t_encoding = m_state . GetTextEncoding();
			if (t_encoding == kMCTextEncodingUndefined)
				t_encoding = m_default_text_encoding;

			uint8_t t_input;
			t_input = (uint8_t)p_value;

			uint16_t t_output;
			uint32_t t_read;
			if (MCTextEncodeToUnicode(t_encoding, &t_input, 1, &t_output, 2, t_read) && t_read == 2)
				t_bullet_char = t_output;
		}
		break;
	}

	if (t_bullet_char != 0)
		m_list_style = bullet_char_to_liststyle(t_bullet_char);

	return t_status;
}

RTFStatus RTFReader::ParseListTable(RTFToken p_token, int4 p_value)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;
	
	bool t_has_parameter;
	t_has_parameter = (p_token & kRTFTokenHasParameter) != 0;
	
	switch(p_token & kRTFTokenMask)
	{
	case kRTFTokenList:
		m_listtable_level_count = 0;
		m_lists . Define();
		break;
	case kRTFTokenListId:
		m_lists . SetId(p_value);
		break;
	case kRTFTokenListLevel:
		m_listtable_level_count += 1;
		break;
	case kRTFTokenLevelFormat:
		if (m_listtable_level_count < 9)
		{
			MCTextListStyle t_style;
			switch(p_value)
			{
			// MW-2012-10-10: [[ Bug 10209 ]] Map 22 to decimal at the moment.
			case 22:
			case 0: t_style = kMCTextListStyleDecimal; break;
			
			case 1: t_style = kMCTextListStyleUpperCaseRoman; break;
			case 2: t_style = kMCTextListStyleLowerCaseRoman; break;
			case 3: t_style = kMCTextListStyleUpperCaseLetter; break;
			case 4: t_style = kMCTextListStyleLowerCaseLetter; break;
			case 23: t_style = kMCTextListStyleNone; break;
			default: t_style = kMCTextListStyleNone; break;
			}
			m_lists . SetStyle(m_listtable_level_count, t_style);
		}
		break;
	case kRTFTokenLevelText:
		m_state . SetDestination(kRTFDestinationListTableLevelText);
		break;
	}
	
	return t_status;
}

RTFStatus RTFReader::ParseListTableLevelText(RTFToken p_token, int4 p_value)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;
	
	bool t_has_parameter;
	t_has_parameter = (p_token & kRTFTokenHasParameter) != 0;
	
	int4 t_bullet_char;
	t_bullet_char = -1;
	switch(p_token & kRTFTokenMask)
	{
		case kRTFTokenSkipDestination:
			m_state . SetDestination(kRTFDestinationSkip);
			break;
			
		case kRTFTokenUnicode:
		{
			if (t_has_parameter)
			{
				t_bullet_char = p_value & 0xFFFF;
				m_input_skip_count = m_state . GetUnicodeSkip();
			}
		}
		break;
			
		case kRTFTokenCharacter:
		{
			MCTextEncoding t_encoding;
			t_encoding = m_state . GetTextEncoding();
			if (t_encoding == kMCTextEncodingUndefined)
				t_encoding = m_default_text_encoding;
			
			uint8_t t_input;
			t_input = (uint8_t)p_value;
			
			if (t_input != ';' && t_input >= 32)
			{
				uint16_t t_output;
				uint32_t t_read;
				if (MCTextEncodeToUnicode(t_encoding, &t_input, 1, &t_output, 2, t_read) && t_read == 2)
					t_bullet_char = t_output;
			}
		}
		break;
	}
	
	if (m_listtable_level_count <= 9 && t_bullet_char != -1)
		m_lists . SetStyle(m_listtable_level_count, bullet_char_to_liststyle(t_bullet_char));
	
	return t_status;
}

RTFStatus RTFReader::ParseListOverrideTable(RTFToken p_token, int4 p_value)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;
	
	bool t_has_parameter;
	t_has_parameter = (p_token & kRTFTokenHasParameter) != 0;
	
	switch(p_token & kRTFTokenMask)
	{
	case kRTFTokenListOverride:
		m_overrides . Define();
		break;
	case kRTFTokenListId:
		m_overrides . SetListId(p_value);
		break;
	case kRTFTokenListSelect:
		m_overrides . SetOverrideId(p_value);
		break;
	}
	
	return t_status;
}

RTFStatus RTFReader::ParseListText(RTFToken p_token, int4 p_value)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;

	bool t_has_parameter;
	t_has_parameter = (p_token & kRTFTokenHasParameter) != 0;

	uint16_t t_bullet_char;
	t_bullet_char = 0;

	switch(p_token & kRTFTokenMask)
	{
		case kRTFTokenSkipDestination:
			m_state . SetDestination(kRTFDestinationSkip);
		break;

		case kRTFTokenUnicode:
		{
			if (t_has_parameter)
			{
				if ((p_value & 0xFFFF) != ' ')
					m_list_skip = false;
				m_input_skip_count = m_state . GetUnicodeSkip();
			}
		}
		break;

		case kRTFTokenCharacter:
		{
			MCTextEncoding t_encoding;
			t_encoding = m_state . GetTextEncoding();
			if (t_encoding == kMCTextEncodingUndefined)
				t_encoding = m_default_text_encoding;

			uint8_t t_input;
			t_input = (uint8_t)p_value;

			uint16_t t_output;
			uint32_t t_read;
			if (MCTextEncodeToUnicode(t_encoding, &t_input, 1, &t_output, 2, t_read) && t_read == 2)
				if (t_output != ' ')
					m_list_skip = false;
		}
		break;
	}

	return t_status;
}


// Convert twentieth's of a point to pixels.
static int32_t twips_to_pixels(int32_t p_twips)
{
	return p_twips / 20;
}

// Convert half points to pixels.
static int32_t half_points_to_pixels(int32_t p_points)
{
	return (p_points * IMPORT_RTF_PLATFORM_DPI) / 144;
}

RTFStatus RTFReader::Flush(bool p_force)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;

	if (!m_attributes_changed && !p_force)
		return t_status;

	MCTextBlock t_block;
	bool t_changed;
	t_changed = false;
	if (m_attributes_changed)
	{
		t_block . foreground_color = m_state . GetForegroundColor();
		t_block . background_color = m_state . GetBackgroundColor();
		t_block . font_name = m_state . GetFontName();
		if (m_state . GetFontSize() != 0)
			t_block . font_size = half_points_to_pixels(m_state . GetFontSize());
		else
			t_block . font_size = 0;
		
		t_block . font_style = FA_DEFAULT_STYLE;
		if ((m_state . GetFontStyle() & kRTFFontStyleItalic) != 0)
			t_block . font_style |= FA_ITALIC;
		if ((m_state . GetFontStyle() & kRTFFontStyleBold) != 0)
			t_block . font_style |= FA_BOLD;
		if ((m_state . GetFontStyle() & kRTFFontStyleUnderline) != 0)
			t_block . font_style |= FA_UNDERLINE;
		if ((m_state . GetFontStyle() & kRTFFontStyleStrikethrough) != 0)
			t_block . font_style |= FA_STRIKEOUT;
		if ((m_state . GetFontStyle() & kRTFFontStyleLink) != 0)
			t_block . font_style |= FA_LINK;

		if ((m_state . GetFontStyle() & kRTFFontStyleSuperscript) != 0)
			t_block . text_shift = -4;
		else if ((m_state . GetFontStyle() & kRTFFontStyleSubscript) != 0)
			t_block . text_shift = 4;
		else
			t_block . text_shift = 0;
		
		if (m_state . GetHyperlink() != kMCEmptyName)
            t_block.text_link = MCValueRetain(m_state . GetHyperlink());
		else
			t_block . text_link = nil;

        if (m_state . GetMetadata() != kMCEmptyString)
            /* UNCHECKED */ MCStringCopy(m_state . GetMetadata(), t_block . text_metadata);
		else
			t_block . text_metadata = MCValueRetain(kMCEmptyString);

		t_block . string_native = false;
		t_block . string_buffer = NULL;
		t_block . string_length = 0;

		if (t_block . foreground_color != m_attributes . foreground_color ||
			t_block . background_color != m_attributes . background_color ||
			t_block . font_size != m_attributes . font_size ||
			t_block . font_style != m_attributes . font_style ||
			t_block . text_shift != m_attributes . text_shift ||
			CStringCompare(t_block . font_name, m_attributes . font_name) != 0 ||
			t_block . text_link != m_attributes . text_link ||
			t_block . text_metadata != m_attributes . text_metadata)
			t_changed = true;
	}

	const uint2 *t_string_buffer;
	uint4 t_string_length;
	if (t_status == kRTFStatusSuccess)
		t_status = m_text . Borrow(t_string_buffer, t_string_length);

	if (t_status == kRTFStatusSuccess && t_string_length > 0 && (t_changed || p_force))
	{
		if (m_needs_paragraph)
		{
			m_needs_paragraph = false;

			// MW-2012-03-14: [[ RtfParaStyles ]] Use the 'Paragraph()' method to ensure
			//   paragraph styles get applied.
			Paragraph();
		}

		m_attributes . string_native = false;
		m_attributes . string_buffer = t_string_buffer;
		m_attributes . string_length = t_string_length / 2;
		m_converter(m_converter_context, NULL, &m_attributes);
		m_text . Clear();
	}

	if (t_changed)
	{
        MCValueRelease(m_attributes . text_metadata);
        MCValueRelease(m_attributes . text_link);
		memcpy(&m_attributes, &t_block, sizeof(MCTextBlock));
		m_attributes_changed = false;
	}

	return t_status;
}

RTFStatus RTFReader::Paragraph(void)
{
	// MW-2012-03-14: [[ RtfParaStyles ]] Build the paragraph style record up
	//   to apply to the new paragraph.
	MCTextParagraph t_para;
	memset(&t_para, 0, sizeof(MCTextParagraph));
	t_para . text_align = m_state . GetTextAlign();
	t_para . border_width = twips_to_pixels(m_state . GetBorderWidth());
	t_para . padding = twips_to_pixels(m_state . GetPadding());
	t_para . first_indent = twips_to_pixels(m_state . GetFirstIndent());
	t_para . left_indent = twips_to_pixels(m_state . GetLeftIndent());
	t_para . right_indent = twips_to_pixels(m_state . GetRightIndent());
	t_para . space_above = twips_to_pixels(m_state . GetSpaceAbove());
	t_para . space_below = twips_to_pixels(m_state . GetSpaceBelow());
	t_para . background_color = m_state . GetParagraphBackgroundColor();
	t_para . border_color = m_state . GetBorderColor();
	t_para . metadata = m_state . GetParagraphMetadata();

	if (m_lists . Count() == 0 || m_state . GetListStyle() == kMCTextListStyleSkip)
	{
		t_para . list_style = m_state . GetListStyle();
		t_para . list_depth = m_state . GetListLevel() + 1;
	}
	else if (m_state . GetListIndex() != 0)
	{
		int4 t_list_id;
		if (m_overrides . Get(m_state . GetListIndex(), t_list_id))
		{
			uint32_t t_level;
			t_level = m_state . GetListLevel();

			MCTextListStyle t_style;
			if (m_lists . Get(t_list_id, t_level + 1, t_style))
			{
				t_para . list_style = t_style;
				t_para . list_depth = t_level + 1;
			}
		}
	}

	m_converter(m_converter_context, &t_para, NULL);
	
	return kRTFStatusSuccess;
}

RTFStatus RTFReader::LookupKeyword(const char *p_keyword, int4 p_keyword_length, RTFToken& r_token)
{
	static const RTFKeyword s_keywords[] =
	{
		{ "\n", kRTFTokenNewLine },
		{ "line", kRTFTokenNewLine },
		{ "bin", kRTFTokenBin },
		{ "tab", kRTFTokenTab },
		{ "u", kRTFTokenUnicode },
		{ "par", kRTFTokenParagraph },
		{ "pard", kRTFTokenResetParagraphStyle },
		{ "uc", kRTFTokenUnicodeSkip },
		
		{ "*", kRTFTokenSkipDestination },
		{ "header", kRTFTokenSkipDestination },
		{ "stylesheet", kRTFTokenSkipDestination },
		{ "info", kRTFTokenSkipDestination },
		{ "object", kRTFTokenSkipDestination },
		{ "pict", kRTFTokenSkipDestination },
		{ "stylesheet", kRTFTokenSkipDestination },
		
		{ "deff", kRTFTokenDefaultFont },

		{ "fonttbl", kRTFTokenFontTable },
		{ "f", kRTFTokenFont },
		{ "fcharset", kRTFTokenFontCharset },
		{ "fs", kRTFTokenFontSize },
		{ "cpg", kRTFTokenFontCodepage },
		{ "fname", kRTFTokenFontName },
		
		{ "colortbl", kRTFTokenColorTable },
		{ "red", kRTFTokenRed },
		{ "green", kRTFTokenGreen },
		{ "blue", kRTFTokenBlue },

		{ "b", kRTFTokenBold },
		{ "dn", kRTFTokenSubscript },
		{ "i", kRTFTokenItalic },
		{ "plain", kRTFTokenPlain },
		{ "strike", kRTFTokenStrikethrough },
		{ "ul", kRTFTokenUnderline },
		{ "ulnone", kRTFTokenNoUnderline },
		{ "up", kRTFTokenSuperscript },
		{ "super", kRTFTokenSuperscript },
		{ "sub", kRTFTokenSubscript },
		{ "nosupersub", kRTFTokenNoSuperSub },

		{ "cf", kRTFTokenColor },
		
		// MW-2012-03-04: [[ RtfImport ]] Accept any of cb, chcbpat or highlight
		//   as background color.
		{ "cb", kRTFTokenHighlight },
		{ "chcbpat", kRTFTokenHighlight },
		{ "highlight", kRTFTokenHighlight },

		{ "ansi", kRTFTokenAnsi },
		{ "mac", kRTFTokenMac },
		{ "pc", kRTFTokenPC },
		{ "pca", kRTFTokenPCA },
		{ "ansicpg", kRTFTokenAnsiCodepage },

		{ "cell", kRTFTokenCell },
		{ "row", kRTFTokenRow },

		{ "bullet", kRTFTokenBullet },
		{ "lquote", kRTFTokenLeftQuote },
		{ "rquote", kRTFTokenRightQuote },
		{ "ldblquote", kRTFTokenLeftDoubleQuote },
		{ "rdblquote", kRTFTokenRightDoubleQuote },

		{ "pn", kRTFTokenLegacyList },
		{ "pntext", kRTFTokenSkipDestination },
		{ "pnlvl", kRTFTokenLegacyListLevel },
		{ "pnlvlblt", kRTFTokenLegacyListBulletLevel },
		{ "pnlvlbody", kRTFTokenLegacyListBodyLevel },
		{ "pndec", kRTFTokenLegacyListDecimalNumbering },
		{ "pnlcltr", kRTFTokenLegacyListLowercaseAlphabeticNumbering },
		{ "pnlcrm", kRTFTokenLegacyListLowercaseRomanNumbering },
		{ "pnucltr", kRTFTokenLegacyListUppercaseAlphabeticNumbering },
		{ "pnucrm", kRTFTokenLegacyListUppercaseRomanNumbering },
		{ "pntxtb", kRTFTokenLegacyListPrefixText },
		{ "pntxta", kRTFTokenLegacyListSuffixText },

		{ "list", kRTFTokenList },
		{ "listtext", kRTFTokenListText },
		{ "listtable", kRTFTokenListTable },
		{ "listid", kRTFTokenListId },
		{ "listlevel", kRTFTokenListLevel },
		{ "levelnfc", kRTFTokenLevelFormat },
		{ "leveltext", kRTFTokenLevelText },
		
		{ "listoverridetable", kRTFTokenListOverrideTable },
		{ "listoverride", kRTFTokenListOverride },
		{ "ls", kRTFTokenListSelect },
		{ "ilvl", kRTFTokenListSelectLevel },
		
		// MW-2012-03-14: [[ RtfParaStyles ]] All the new paragraph style tags we now
		//   can recognize.
		{ "cbpat", kRTFTokenParagraphBackgroundColor },
		{ "brdrw", kRTFTokenParagraphBorderWidth },
		{ "brdrcf", kRTFTokenParagraphBorderColor },
		{ "brdsp", kRTFTokenParagraphPadding },
		{ "fi", kRTFTokenFirstIndent },
		{ "li", kRTFTokenLeftIndent },
		{ "ri", kRTFTokenRightIndent },
		{ "sb", kRTFTokenSpaceAbove },
		{ "sa", kRTFTokenSpaceBelow },
		{ "ql", kRTFTokenLeftJustify },
		{ "qc", kRTFTokenCenterJustify },
		{ "qr", kRTFTokenRightJustify },
		{ "tx", kRTFTokenTabStop },

		// MW-2012-11-20: Add support for HYPERLINK and LCMETADATA fields.
		{ "fldinst", kRTFTokenFldInst },
		{ "field", kRTFTokenField },
	};
	
	r_token = kRTFTokenUnknown;
	
	for(uint4 i = 0; i < sizeof(s_keywords) / sizeof(RTFKeyword); ++i)
		if (StringCompare(p_keyword, p_keyword_length, s_keywords[i] . name, -1) == 0)
		{
			r_token = s_keywords[i] . token;
			break;
		}
	
	return kRTFStatusSuccess;
}

RTFStatus RTFReader::LookupNibble(int4 p_char, int4& r_nibble)
{
	if (p_char >= '0' && p_char <= '9')
		r_nibble = p_char - '0';
	else if (p_char >= 'A' && p_char <= 'F')
		r_nibble = 10 + p_char - 'A';
	else if (p_char >= 'a' && p_char <= 'f')
		r_nibble = 10 + p_char - 'a';
	else
		return kRTFStatusInvalidHex;
	
	return kRTFStatusSuccess;
}

bool RTFRead(const char *p_rtf, uint4 p_rtf_length, MCTextConvertCallback p_writer, void *p_writer_context)
{
	RTFReader t_reader;
	t_reader . SetConverter(p_writer, p_writer_context);
	return t_reader . Process(p_rtf, p_rtf_length) == kRTFStatusSuccess;
}
