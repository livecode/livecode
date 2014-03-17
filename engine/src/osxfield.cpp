/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "osxprefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "field.h"
#include "paragraf.h"
#include "cdata.h"
#include "mcerror.h"
#include "execpt.h"
#include "util.h"
#include "block.h"

#include "globals.h"
#include "unicode.h"

#include "text.h"

MCParagraph *MCField::macunicodestyletexttoparagraphs(const MCString& p_text, const MCString& p_style_data)
{
	bool t_success;
	t_success = true;
	
	ItemCount t_run_count;
	ItemCount t_style_count;
	
	if (t_success)
	{
		if (ATSUUnflattenStyleRunsFromStream(
											 kATSUDataStreamUnicodeStyledText, 0,
											 p_style_data . getlength(), p_style_data . getstring(),
											 0, 0, NULL, NULL,
											 &t_run_count, &t_style_count) != noErr)
			t_success = false;
	}
	
	ATSUStyleRunInfo *t_runs;
	t_runs = NULL;
	if (t_success)
	{
		t_runs = new ATSUStyleRunInfo[t_run_count];
		if (t_runs == NULL)
			t_success = false;
	}
	
	ATSUStyle *t_styles;
	t_styles = NULL;
	if (t_success)
	{
		t_styles = new ATSUStyle[t_style_count];
		if (t_styles == NULL)
			t_success = false;
	}
	
	if (t_success)
	{
		
		if (ATSUUnflattenStyleRunsFromStream(
											 kATSUDataStreamUnicodeStyledText, 0,
											 p_style_data . getlength(), p_style_data . getstring(),
											 t_run_count, t_style_count, t_runs, t_styles,
											 &t_run_count, &t_style_count) != noErr)
			t_success = false;
	}
	
	char t_font_name[256];
	
	MCParagraph *t_paragraphs;
	t_paragraphs = NULL;
	if (t_success)
	{
		t_paragraphs = new MCParagraph;
		t_paragraphs -> state |= PS_LINES_NOT_SYNCHED;
		t_paragraphs -> setparent(this);
		t_paragraphs -> blocks = new MCBlock;
		t_paragraphs -> blocks -> setparent(t_paragraphs);
		t_paragraphs -> blocks -> index = 0;
		t_paragraphs -> blocks -> size = 0;
		
		const UniChar *t_text;
		t_text = (UniChar *)p_text . getstring();
		
		uint4 t_text_length;
		t_text_length = p_text . getlength();
		
		int4 t_run_start;
		t_run_start = 0;
		for(uint4 i = 0; i < t_run_count; ++i)
		{
			MCTextBlock t_block;
			memset(&t_block, 0, sizeof(MCTextBlock));
			if (i >= 0)
			{
				t_block . background_color = 0xffffffff;
				
				RGBColor t_color;
				if (ATSUGetAttribute(t_styles[t_runs[i] . styleObjectIndex], kATSUColorTag, sizeof(RGBColor), &t_color, NULL) != kATSUNotSetErr)
				{
					t_block . foreground_color =
					(t_color . red >> 8) |
					((t_color . green >> 8) << 8) |
					((t_color . blue >> 8) << 16);
				}
				else
					t_block . foreground_color = 0xffffffff;
				
				Fixed t_font_size;
				if (ATSUGetAttribute(t_styles[t_runs[i] . styleObjectIndex], kATSUSizeTag, sizeof(Fixed), &t_font_size, NULL) != kATSUNotSetErr)
					t_block . font_size = t_font_size >> 16;
				
				ATSUFontID t_font_id;
				if (ATSUGetAttribute(t_styles[t_runs[i] . styleObjectIndex], kATSUFontTag, sizeof(ATSUFontID), &t_font_id, NULL) != kATSUNotSetErr)
				{
					ByteCount t_font_name_length;
					if (ATSUFindFontName(t_font_id, kFontFamilyName, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage, 255, t_font_name, &t_font_name_length, NULL) == noErr)
					{
						t_font_name[t_font_name_length] = '\0';
						t_block . font_name = t_font_name;
					}
					else
						t_block . font_name = NULL;
					
					ByteCount t_style_name_length;
					char t_style_name[64];
					if (ATSUFindFontName(t_font_id, kFontStyleName, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage, 63, t_style_name, &t_style_name_length, NULL) == noErr)
					{
						t_style_name[t_style_name_length] = '\0';
						if (strcasecmp(t_style_name, "Regular") == 0)
							t_block . font_style = FA_DEFAULT_STYLE;
						else if (strcasecmp(t_style_name, "Bold") == 0)
							t_block . font_style = FA_DEFAULT_STYLE | FA_BOLD;
						else if (strcasecmp(t_style_name, "Italic") == 0)
							t_block . font_style = FA_DEFAULT_STYLE | FA_ITALIC;
						else if (strcasecmp(t_style_name, "Bold Italic") == 0)
							t_block . font_style = FA_DEFAULT_STYLE | FA_BOLD | FA_ITALIC;
						else if (ATSUFindFontName(t_font_id, kFontFullName, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage, 255, t_font_name, &t_font_name_length, NULL) == noErr)
							t_font_name[t_font_name_length] = '\0';
					}
				}
				else
					t_block . font_name = NULL;
				
				Boolean t_underline;
				if (ATSUGetAttribute(t_styles[t_runs[i] . styleObjectIndex], kATSUQDUnderlineTag, sizeof(Boolean), &t_underline, NULL) != kATSUNotSetErr)
				{
					if (t_underline)
						t_block . font_style |= FA_UNDERLINE;
				}
			}
			else
			{
				t_block . foreground_color = 0xffffffff;
				t_block . background_color = 0xffffffff;
			}
			
			uint4 t_run_end;
			t_run_end = t_run_start + t_runs[i] . runLength;
			
			while(t_run_start < t_run_end)
			{
				uint4 t_block_end;
				for(t_block_end = t_run_start; t_block_end < t_run_end; ++t_block_end)
					if (t_text[t_block_end] == 13)
						break;
				
				if (t_block_end > t_run_start)
				{
					t_block . string_native = false;
					t_block . string_buffer = (uint2 *)(t_text + t_run_start);
					t_block . string_length = t_block_end - t_run_start;
					converttoparagraphs(t_paragraphs, nil, &t_block);
				}
				
				while(t_block_end < t_run_end && t_text[t_block_end] == 13)
				{
					converttoparagraphs(t_paragraphs, NULL, NULL);
					t_block_end += 1;
				}
				
				t_run_start = t_block_end;
			}
		}
	}
	
	if (t_styles != NULL)
	{
		for(uint4 i = 0; i < t_style_count; ++i)
			ATSUDisposeStyle(t_styles[i]);
		
		delete t_styles;
	}
	
	delete t_runs;
	
	return t_paragraphs;
}

MCParagraph *MCField::macstyletexttoparagraphs(const MCString& p_text, const MCString& p_style_data, Boolean p_unicode)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = new MCParagraph;
	t_paragraphs -> state |= PS_LINES_NOT_SYNCHED;
	t_paragraphs -> setparent(this);
	t_paragraphs -> blocks = new MCBlock;
	t_paragraphs -> blocks -> setparent(t_paragraphs);
	t_paragraphs -> blocks -> index = 0;
	t_paragraphs -> blocks -> size = 0;
	
	StScrpRec *t_styles;
	t_styles = (StScrpRec *)p_style_data . getstring();
	
	const char *t_text;
	t_text = p_text . getstring();
	
	uint4 t_text_length;
	t_text_length = p_text . getlength();
	
	uint4 t_run_start;
	t_run_start = 0;
	
	char t_font_name[256];
	int4 t_font_index;
	t_font_index = -1;
	
	for(int4 i = -1; i < t_styles -> scrpNStyles; ++i)
	{
		MCTextBlock t_block;
		memset(&t_block, 0, sizeof(MCTextBlock));
		if (i >= 0)
		{
			t_block . foreground_color =
			(t_styles -> scrpStyleTab[i] . scrpColor . red >> 8) |
			((t_styles -> scrpStyleTab[i] . scrpColor . green >> 8) << 8) |
			((t_styles -> scrpStyleTab[i] . scrpColor . blue >> 8) << 16);
			
			t_block . background_color = 0xffffffff;
			
			t_block . string_length = t_styles -> scrpStyleTab[i] . scrpSize;
			
			t_block . font_style = FA_DEFAULT_STYLE;
			if ((t_styles -> scrpStyleTab[i] . scrpFace & italic) != 0)
				t_block . font_style |= FA_ITALIC;
			if ((t_styles -> scrpStyleTab[i] . scrpFace & bold) != 0)
				t_block . font_style |= FA_BOLD;
			if ((t_styles -> scrpStyleTab[i] . scrpFace & condense) != 0)
				t_block . font_style |= FA_CONDENSED;
			if ((t_styles -> scrpStyleTab[i] . scrpFace & extend) != 0)
				t_block . font_style |= FA_EXPANDED;
			if ((t_styles -> scrpStyleTab[i] . scrpFace & underline) != 0)
				t_block . font_style |= FA_UNDERLINE;
			
			if (t_styles -> scrpStyleTab[i] . scrpFont != t_font_index)
			{
				GetFontName(t_styles ->scrpStyleTab[i] . scrpFont, (unsigned char *)t_font_name);
				p2cstr((unsigned char *)t_font_name);
				t_font_index = t_styles -> scrpStyleTab[i] . scrpFont;
			}
			t_block . font_name = t_font_name;
		}
		else
		{
			t_block . foreground_color = 0xffffffff;
			t_block . background_color = 0xffffffff;
		}
		
		uint4 t_run_end;
		if (i + 1 < t_styles -> scrpNStyles)
			t_run_end = t_styles -> scrpStyleTab[i + 1] . scrpStartChar;
		else
			t_run_end = t_text_length;
		
		while(t_run_start < t_run_end)
		{
			uint4 t_block_end;
			for(t_block_end = t_run_start; t_block_end < t_run_end; ++t_block_end)
				if (t_text[t_block_end] == 13)
					break;
			
			if (t_block_end > t_run_start)
			{
				t_block . string_native = true;
				t_block . string_buffer = (uint2 *)(t_text + t_run_start);
				t_block . string_length = t_block_end - t_run_start;
				converttoparagraphs(t_paragraphs, NULL, &t_block);
			}
			
			while(t_block_end < t_run_end && t_text[t_block_end] == 13)
			{
				converttoparagraphs(t_paragraphs, NULL, NULL);
				t_block_end += 1;
			}
			
			t_run_start = t_block_end;
		}
	}
	
	return t_paragraphs;
}

class StylesBuffer
{
public:
	StylesBuffer(void)
	{
		m_runs = NULL;
		m_run_count = 0;
		m_run_capacity = 0;
		
		m_styles = NULL;
		m_style_infos = NULL;
		m_style_count = 0;
		m_style_capacity = 0;
	}
	
	~StylesBuffer(void)
	{
		if (m_runs != NULL)
			free(m_runs);
		
		if (m_styles != NULL)
		{
			for(uint4 i = 0; i < m_style_count; ++i)
				ATSUDisposeStyle(m_styles[i]);
			free(m_styles);
			free(m_style_infos);
		}
	}
	
	void Append(const char *p_font, uint2 p_size, uint2 p_style, uint4 p_color, uint4 p_length)
	{
		uint4 t_style_index;
		t_style_index = FindStyle(p_font, p_size, p_style, p_color);
		
		if (m_run_count > 0 && m_runs[m_run_count - 1] . styleObjectIndex == t_style_index)
		{
			m_runs[m_run_count - 1] . runLength += p_length;
			return;
		}
		
		if (m_run_count == m_run_capacity)
		{
			uint4 t_new_capacity;
			t_new_capacity = m_run_capacity + 128;
			
			ATSUStyleRunInfo *t_new_runs;
			t_new_runs = (ATSUStyleRunInfo *)realloc(m_runs, sizeof(ATSUStyleRunInfo) * t_new_capacity);
			if (t_new_runs == NULL)
			{
				m_runs[m_run_count - 1] . runLength += p_length;
				return;
			}
			
			m_runs = t_new_runs;
			m_run_capacity = t_new_capacity;
		}
		
		m_runs[m_run_count] . runLength = p_length;
		m_runs[m_run_count] . styleObjectIndex = t_style_index;
		m_run_count += 1;
	}
	
	void BorrowRuns(ATSUStyleRunInfo*& r_runs, ItemCount& r_run_count)
	{
		r_runs = m_runs;
		r_run_count = m_run_count;
	}
	
	void BorrowStyles(ATSUStyle*& r_styles, ItemCount& r_style_count)
	{
		r_styles = m_styles;
		r_style_count = m_style_count;
	}
	
private:
	struct StyleInfo
	{
		const char *font_name;
		uint2 font_style;
		uint2 font_size;
		uint4 color;
	};
	
	uint4 FindStyle(const char *p_font, uint2 p_size, uint2 p_style, uint4 p_color)
	{
		for(uint4 i = 0; i < m_style_count; ++i)
		{
			if (p_style == m_style_infos[i] . font_style &&
				p_size == m_style_infos[i] . font_size &&
				p_color == m_style_infos[i] . color)
			{
				if (p_font == m_style_infos[i] . font_name)
					return i;
				if (p_font == NULL || m_style_infos[i] . font_name == NULL)
					continue;
				if (strcasecmp(p_font, m_style_infos[i] . font_name) == 0)
					return i;
			}
		}
		
		if (m_style_count == m_style_capacity)
		{
			uint4 t_new_capacity;
			t_new_capacity = m_style_capacity + 16;
			
			ATSUStyle *t_new_styles;
			t_new_styles = (ATSUStyle *)realloc(m_styles, sizeof(ATSUStyle *) * t_new_capacity);
			if (t_new_styles != NULL)
				m_styles = t_new_styles;
			
			StyleInfo *t_new_style_infos;
			t_new_style_infos = (StyleInfo *)realloc(m_style_infos, sizeof(StyleInfo) * t_new_capacity);
			if (t_new_style_infos != NULL)
				m_style_infos = t_new_style_infos;
			
			if (t_new_styles == NULL || t_new_style_infos == NULL)
				return 0;
			
			m_style_capacity = t_new_capacity;
		}
		
		ATSUStyle t_style;
		if (ATSUCreateStyle(&t_style) != noErr)
			return 0;
		
		if (p_font != NULL)
		{
			bool t_is_bold;
			if ((p_style & FA_WEIGHT) == (FA_BOLD & FA_WEIGHT))
				t_is_bold = true;
			else
				t_is_bold = false;
			
			bool t_is_italic;
			if ((p_style & FA_ITALIC) != 0)
				t_is_italic = true;
			else
				t_is_italic = false;
			
			const char *t_font_style;
			if (t_is_bold && t_is_italic)
				t_font_style = "Bold Italic";
			else if (t_is_bold)
				t_font_style = "Bold";
			else if (t_is_italic)
				t_font_style = "Italic";
			else
				t_font_style = NULL;
			
			char t_font_name[256];
			strcpy(t_font_name, p_font);
			if (t_font_style != NULL)
			{
				strcat(t_font_name, " ");
				strcat(t_font_name, t_font_style);
			}
			
			ATSUAttributeTag t_tag;
			ByteCount t_value_length;
			ATSUAttributeValuePtr t_value_ptr;
			
			ATSUFontID t_font_id;
			if (ATSUFindFontFromName(t_font_name, strlen(t_font_name), kFontFullName, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) != noErr)
			{
				if (ATSUFindFontFromName(p_font, strlen(p_font), kFontFullName, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) != noErr)
					if (ATSUFindFontFromName(p_font, strlen(p_font), kFontFamilyName, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) != noErr)
						return 0;
				
				// We've failed to find font + variant, so apply QD styles instead
				if (t_is_bold)
				{
					Boolean t_bold;
					t_bold = True;
					t_tag = kATSUQDBoldfaceTag;
					t_value_length = sizeof(Boolean);
					t_value_ptr = &t_bold;
					ATSUSetAttributes(t_style, 1, &t_tag, &t_value_length, &t_value_ptr);
				}
				
				if (t_is_italic)
				{
					Boolean t_italic;
					t_italic = True;
					t_tag = kATSUQDItalicTag;
					t_value_length = sizeof(Boolean);
					t_value_ptr = &t_italic;
					ATSUSetAttributes(t_style, 1, &t_tag, &t_value_length, &t_value_ptr);
				}
			}
			
			
			t_tag = kATSUFontTag;
			t_value_length = sizeof(ATSUFontID);
			t_value_ptr = &t_font_id;
			ATSUSetAttributes(t_style, 1, &t_tag, &t_value_length, &t_value_ptr);
			
			Fixed t_font_size;
			t_font_size = p_size << 16;
			
			t_tag = kATSUSizeTag;
			t_value_length = sizeof(Fixed);
			t_value_ptr = &t_font_size;
			ATSUSetAttributes(t_style, 1, &t_tag, &t_value_length, &t_value_ptr);
			
			if ((p_style & FA_UNDERLINE) != 0)
			{
				Boolean t_underline;
				t_underline = True;
				t_tag = kATSUQDUnderlineTag;
				t_value_length = sizeof(Boolean);
				t_value_ptr = &t_underline;
				ATSUSetAttributes(t_style, 1, &t_tag, &t_value_length, &t_value_ptr);
			}
		}
		
		if (p_color != 0xffffffff)
		{
			ATSUAttributeTag t_tag;
			ByteCount t_value_length;
			ATSUAttributeValuePtr t_value_ptr;
			
			RGBColor t_color;
			t_color . red = ((p_color & 0xff) << 8) | (p_color & 0xff);
			t_color . green = (p_color & 0xff00) | ((p_color & 0xff00) >> 8);
			t_color . blue = ((p_color & 0xff0000) >> 8) | ((p_color & 0xff0000) >> 16);
			
			t_tag = kATSUColorTag;
			t_value_length = sizeof(RGBColor);
			t_value_ptr = &t_color;
			ATSUSetAttributes(t_style, 1, &t_tag, &t_value_length, &t_value_ptr);
		}
		
		
		m_styles[m_style_count] = t_style;
		m_style_infos[m_style_count] . font_name = p_font;
		m_style_infos[m_style_count] . font_style = p_style;
		m_style_infos[m_style_count] . font_size = p_size;
		m_style_infos[m_style_count] . color = p_color;
		m_style_count += 1;
		
		return m_style_count - 1;
	}
	
	ATSUStyleRunInfo *m_runs;
	uint4 m_run_count;
	uint4 m_run_capacity;
	
	ATSUStyle *m_styles;
	StyleInfo *m_style_infos;
	uint4 m_style_count;
	uint4 m_style_capacity;
};

Exec_stat MCField::getparagraphmacunicodestyles(MCExecPoint& ep, MCParagraph *p_start, MCParagraph *p_end)
{
	const char *origname;
	uint2 origsize;
	uint2 origstyle;
	getfontattsnew(origname, origsize, origstyle);
	
	const char *fontname;
	uint2 fontsize, fontstyle;
	getfontattsnew(fontname, fontsize, fontstyle);
	
	uint4 t_offset;
	t_offset = 0;
	
	StylesBuffer t_buffer;
	
	MCParagraph *t_paragraph;
	t_paragraph = p_start;
	do
	{
		t_paragraph->setparent(this);
		t_paragraph->defrag();
		
		// MW-2011-01-17: Make sure we have no zero-length blocks in the paragraph as these
		//   mess up style generation.
		t_paragraph->clearzeros();
		
		// MW-2012-02-18: [[ Bug ]] Make sure the blocks are initialized prior to attempting
		//   to recurse over them.
		MCBlock *t_blocks;
		t_blocks = t_paragraph -> getblocks();
		if (t_blocks == NULL)
		{
			t_paragraph->inittext();
			t_blocks = t_paragraph -> getblocks();
		}
		
		MCBlock *t_block;
		t_block = t_blocks;
		do
		{
			uint2 t_block_length;
			const char *t_font_name;
			uint2 t_font_style;
			uint2 t_font_size;
			uint4 t_color;
			
			MCBlock *t_next_block;
			if (t_block == NULL)
				t_next_block = NULL;
			else
			{
				t_next_block = t_block -> next();
				while(t_next_block != t_blocks && t_next_block -> getsize() == 0)
					t_next_block = t_next_block -> next();
			}
			
			if (t_block != NULL)
			{
				uint2 t_block_offset;
				t_block -> getindex(t_block_offset, t_block_length);
				if (t_block -> hasunicode())
					t_block_length = t_block_length / 2;
				if (t_next_block == t_blocks && t_paragraph -> next() != p_end -> next())
					t_block_length += 1;
				
				// MW-2012-02-17: [[ SplitTextAttrs ]] Get any font attrs the block has.
				if (!t_block -> gettextfont(t_font_name))
					t_font_name = origname;
				if (!t_block -> gettextsize(t_font_size))
					t_font_size = origsize;
				if (!t_block -> gettextstyle(t_font_style))
					t_font_style = origstyle;
				
				const MCColor *t_mc_color;
				if (t_block -> getcolor(t_mc_color))
					t_color = (t_mc_color -> red >> 8) | (t_mc_color -> green & 0xFF00) | ((t_mc_color -> blue & 0xFF00) << 8);
				else
					t_color = 0xffffffff;
				
				t_block = t_block -> next();
			}
			else
			{
				if (t_paragraph -> next() != p_end -> next())
					t_block_length = 1;
				t_font_name = NULL;
				t_font_style = 0;
				t_font_size = 0;
				t_color = 0xffffffff;
			}
			
			t_buffer . Append(t_font_name, t_font_size, t_font_style, t_color, t_block_length);
			
		}
		while(t_block != t_blocks);

		// MW-2011-01-25: [[ ParaStyles ]] Ask the paragraph to reflow itself.
		if (opened)
			t_paragraph -> layout(false);
		
		t_paragraph = t_paragraph -> next();
	}
	while(t_paragraph != p_end -> next());
	
	ATSUStyleRunInfo *t_runs;
	ItemCount t_run_count;
	t_buffer . BorrowRuns(t_runs, t_run_count);
	
	ATSUStyle *t_styles;
	ItemCount t_style_count;
	t_buffer . BorrowStyles(t_styles, t_style_count);
	
	ByteCount t_stream_size;
	if (ATSUFlattenStyleRunsToStream(kATSUDataStreamUnicodeStyledText, 0, t_run_count, t_runs, t_style_count, t_styles, 0, NULL, &t_stream_size) == noErr)
	{
		void *t_stream;
		t_stream = ep . getbuffer(t_stream_size);
		if (t_stream != NULL)
		{
			if (ATSUFlattenStyleRunsToStream(kATSUDataStreamUnicodeStyledText, 0, t_run_count, t_runs, t_style_count, t_styles, t_stream_size, t_stream, &t_stream_size) != noErr)
				t_stream_size = 0;
			ep . setlength(t_stream_size);
		}
	}
	
	return ES_NORMAL;
}

bool MCField::macmatchfontname(const char *p_font_name, char p_derived_font_name[])
{
	ATSUFontID t_font_id;
	if (ATSUFindFontFromName(p_font_name, strlen(p_font_name), kFontFullName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) == noErr ||
		ATSUFindFontFromName(p_font_name, strlen(p_font_name), kFontUniqueName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) == noErr ||
		ATSUFindFontFromName(p_font_name, strlen(p_font_name), kFontFamilyName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) == noErr ||
		ATSUFindFontFromName(p_font_name, strlen(p_font_name), kFontNoName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) == noErr)
	{
		// Fetch the style name
		char t_style_name[64];
		ByteCount t_style_name_length;
		t_style_name_length = 0;
		ATSUFindFontName(t_font_id, kFontStyleName, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage, 63, t_style_name, &t_style_name_length, NULL);
		t_style_name[t_style_name_length] = '\0';
		
		// Fetch the full name
		char t_full_name[256];
		ByteCount t_full_name_length;
		t_full_name_length = 0;
		ATSUFindFontName(t_font_id, kFontFullName, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage, 255, t_full_name, &t_full_name_length, NULL);
		t_full_name[t_full_name_length] = '\0';
		
		// MW-2011-09-02: Make sure we don't do anything at all if style is regular
		//   (output name should be fullname!)
		if (MCCStringEqualCaseless(t_style_name, "Regular"))
			p_font_name = p_font_name; // Do nothing
		else if (MCCStringEndsWithCaseless(t_full_name, "Bold Italic"))
			t_full_name[t_full_name_length - 12] = '\0';
		else if (MCCStringEndsWithCaseless(t_full_name, "Bold"))
			t_full_name[t_full_name_length - 5] = '\0';
		else if (MCCStringEndsWithCaseless(t_full_name, "Italic"))
			t_full_name[t_full_name_length - 7] = '\0';
		
		strcpy(p_derived_font_name, t_full_name);
		
		return true;
	}
	
	return false;
}
