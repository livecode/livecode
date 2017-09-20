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

#ifndef __RTF__
#define __RTF__

//

enum RTFStatus
{
	kRTFStatusSuccess,
	kRTFStatusNoMemory,
	kRTFStatusUnderflow,
	kRTFStatusOverflow,
	kRTFStatusInvalidHex,
	kRTFStatusIncompleteKeyword,
	kRTFStatusKeywordTooLong,
	kRTFStatusIncompleteParameter,
	kRTFStatusParameterTooLong
};

enum RTFInputState
{
	kRTFInputStateNormal,
	kRTFInputStateBinary,
	kRTFInputStateHex
};

typedef uint4 RTFToken;
enum
{
	kRTFTokenUnknown,

	kRTFTokenEnd,
	kRTFTokenCharacter,
	kRTFTokenHex,
	kRTFTokenBin,
	kRTFTokenBeginGroup,
	kRTFTokenEndGroup,

	kRTFTokenSkipDestination,

	kRTFTokenNewLine,
	kRTFTokenTab,
	kRTFTokenUnicode,
	kRTFTokenParagraph,
	kRTFTokenResetParagraphStyle,
	kRTFTokenUnicodeSkip,

	kRTFTokenDefaultFont,

	kRTFTokenFontTable,
	kRTFTokenFont,
	kRTFTokenFontCharset,
	kRTFTokenFontSize,
	kRTFTokenFontCodepage,
	kRTFTokenFontName,

	kRTFTokenColorTable,
	kRTFTokenRed,
	kRTFTokenBlue,
	kRTFTokenGreen,

	kRTFTokenBold,
	kRTFTokenSubscript,
	kRTFTokenItalic,
	kRTFTokenPlain,
	kRTFTokenStrikethrough,
	kRTFTokenUnderline,
	kRTFTokenNoUnderline,
	kRTFTokenSuperscript,
	kRTFTokenNoSuperSub,

	kRTFTokenColor,
	kRTFTokenHighlight,

	kRTFTokenAnsi,
	kRTFTokenMac,
	kRTFTokenPC,
	kRTFTokenPCA,
	kRTFTokenAnsiCodepage,

	kRTFTokenCell,
	kRTFTokenRow,

	kRTFTokenBullet,

	kRTFTokenLeftQuote,
	kRTFTokenRightQuote,

	kRTFTokenLeftDoubleQuote,
	kRTFTokenRightDoubleQuote,

	kRTFTokenLegacyList,
	kRTFTokenLegacyListText,
	kRTFTokenLegacyListLevel,
	kRTFTokenLegacyListBulletLevel,
	kRTFTokenLegacyListBodyLevel,
	kRTFTokenLegacyListDecimalNumbering,
	kRTFTokenLegacyListLowercaseAlphabeticNumbering,
	kRTFTokenLegacyListLowercaseRomanNumbering,
	kRTFTokenLegacyListUppercaseAlphabeticNumbering,
	kRTFTokenLegacyListUppercaseRomanNumbering,
	kRTFTokenLegacyListPrefixText,
	kRTFTokenLegacyListSuffixText,

	kRTFTokenListTable,
	kRTFTokenList,
	kRTFTokenListId,
	kRTFTokenListLevel,
	kRTFTokenListText,
	kRTFTokenLevelFormat,
	kRTFTokenLevelText,
	
	kRTFTokenListOverrideTable,
	kRTFTokenListOverride,
	kRTFTokenListSelect,
	kRTFTokenListSelectLevel,
	
	kRTFTokenParagraphBackgroundColor,
	kRTFTokenParagraphBorderWidth,
	kRTFTokenParagraphBorderColor,
	kRTFTokenParagraphPadding,
	kRTFTokenFirstIndent,
	kRTFTokenLeftIndent,
	kRTFTokenRightIndent,
	kRTFTokenSpaceAbove,
	kRTFTokenSpaceBelow,
	kRTFTokenLeftJustify,
	kRTFTokenCenterJustify,
	kRTFTokenRightJustify,
	kRTFTokenTabStop,

	kRTFTokenField,
	kRTFTokenFldInst,
	
	kRTFTokenHasParameter = 1U << 31,
	kRTFTokenMask = 0xff
};

enum RTFDestination
{
	kRTFDestinationSkip,
	kRTFDestinationNormal,
	kRTFDestinationFontTable,
	kRTFDestinationColorTable,

	kRTFDestinationLegacyList,
	kRTFDestinationLegacyListPrefix,
	kRTFDestinationLegacyListSuffix,
	
	kRTFDestinationListTable,
	kRTFDestinationListTableLevelText,
	kRTFDestinationListOverrideTable,

	kRTFDestinationListText,

	kRTFDestinationField,
	kRTFDestinationFldInst,
};

typedef uint4 RTFFontStyle;
enum
{
	kRTFFontStyleNone = 0,
	kRTFFontStyleItalic = 1 << 0,
	kRTFFontStyleBold = 1 << 1,
	kRTFFontStyleUnderline = 1 << 2,
	kRTFFontStyleSuperscript = 1 << 3,
	kRTFFontStyleSubscript = 1 << 4,
	kRTFFontStyleStrikethrough = 1 << 5,

	kRTFFontStyleLink = 1 << 6,
};

struct RTFKeyword
{
	const char *name;
	RTFToken token;
};

//

class RTFState
{
public:
	RTFState(void);
	~RTFState(void);

	RTFStatus Save(void);
	RTFStatus Restore(void);
	
	void SetDestination(RTFDestination p_destination);
	RTFDestination GetDestination(void) const;
	
	void SetTextEncoding(MCTextEncoding p_encoding);
	MCTextEncoding GetTextEncoding(void) const;

	void SetUnicodeSkip(uint4 p_skip);
	uint4 GetUnicodeSkip(void) const;

	void SetFontName(const char *p_name);
	const char *GetFontName(void) const;
	
	void SetFontStyle(RTFFontStyle p_style);
	RTFFontStyle GetFontStyle(void) const;
	
	void SetFontSize(uint4 p_size);
	uint4 GetFontSize(void) const;
	
	void SetForegroundColor(uint4 p_color);
	uint4 GetForegroundColor(void) const;
	
	void SetBackgroundColor(uint4 p_color);
	uint4 GetBackgroundColor(void) const;

	void SetListLevel(uint32_t p_level);
	uint32_t GetListLevel(void) const;
	void SetListIndex(uint32_t p_index);
	uint32_t GetListIndex(void) const;
	void SetListStyle(MCTextListStyle p_liststyle);
	MCTextListStyle GetListStyle(void) const;

	void SetHyperlink(MCNameRef p_link);
	MCNameRef GetHyperlink(void);
	
    void SetMetadata(MCStringRef p_metadata);
    MCStringRef GetMetadata(void);

    void SetParagraphMetadata(MCStringRef p_metadata);
    MCStringRef GetParagraphMetadata(void);

	// MW-2012-03-14: [[ RtfParaStyles ]] The collection of setters and getters for
	//   the paragraph styles.
	void SetTextAlign(MCTextTextAlign p_align);
	MCTextTextAlign GetTextAlign(void) const;
	void SetBorderWidth(uint32_t p_width);
	uint32_t GetBorderWidth(void) const;
	void SetPadding(uint32_t p_padding);
	uint32_t GetPadding(void) const;
	void SetFirstIndent(int32_t p_first);
	int32_t GetFirstIndent(void) const;
	void SetLeftIndent(int32_t p_left);
	int32_t GetLeftIndent(void) const;
	void SetRightIndent(int32_t p_right);
	int32_t GetRightIndent(void) const;
	void SetSpaceAbove(int32_t p_space_above);
	int32_t GetSpaceAbove(void) const;
	void SetSpaceBelow(int32_t p_space_below);
	int32_t GetSpaceBelow(void) const;
	void SetParagraphBackgroundColor(uint32_t p_bg_color);
	uint32_t GetParagraphBackgroundColor(void) const;
	void SetBorderColor(uint32_t p_color);
	uint32_t GetBorderColor(void) const;
	
	bool HasParagraphChanged(void) const;

private:
	struct Entry
	{
		Entry *previous;
		RTFDestination destination;

		MCTextEncoding text_encoding;
		uint4 unicode_skip;

		const char *font_name;
		RTFFontStyle font_style;
		uint4 font_size;
		uint4 foreground_color;
		uint4 background_color;

		uint32_t list_level;
		uint32_t list_index;
		MCTextListStyle list_style;

		// MW-2012-03-14: [[ RtfParaStyles ]] The paragraph styles state.
		MCTextTextAlign text_align;
		uint32_t border_width;
		uint32_t padding;
		int32_t first_indent;
		int32_t left_indent;
		int32_t right_indent;
		int32_t space_above;
		int32_t space_below;
		uint32_t paragraph_background_color;
		uint32_t border_color;

        MCStringRef metadata;
        MCStringRef paragraph_metadata;
		MCNameRef hyperlink;
	};

	Entry *m_entries;
};

inline void RTFState::SetDestination(RTFDestination p_destination)
{
	if (m_entries == NULL)
		return;
		
	m_entries -> destination = p_destination;
}

inline RTFDestination RTFState::GetDestination(void) const
{
	if (m_entries == NULL)
		return kRTFDestinationNormal;
		
	return m_entries -> destination;
}

inline MCTextEncoding RTFState::GetTextEncoding(void) const
{
	if (m_entries == NULL)
		return kMCTextEncodingUndefined;
		
	return m_entries -> text_encoding;
}

inline void RTFState::SetTextEncoding(MCTextEncoding p_text_encoding)
{
	if (m_entries == NULL)
		return;
		
	m_entries -> text_encoding = p_text_encoding;
}

inline void RTFState::SetUnicodeSkip(uint4 p_skip)
{
	if (m_entries == NULL)
		return;

	m_entries -> unicode_skip = p_skip;
}

inline uint4 RTFState::GetUnicodeSkip(void) const
{
	if (m_entries == NULL)
		return 1;

	return m_entries -> unicode_skip;
}

inline void RTFState::SetFontName(const char *p_name)
{
	if (m_entries == NULL)
		return;

	m_entries -> font_name = p_name;
}

inline const char *RTFState::GetFontName(void) const
{
	if (m_entries == NULL)
		return NULL;

	return m_entries -> font_name;
}

inline void RTFState::SetFontStyle(RTFFontStyle p_style)
{
	if (m_entries == NULL)
		return;

	m_entries -> font_style = p_style;
}

inline RTFFontStyle RTFState::GetFontStyle(void) const
{
	if (m_entries == NULL)
		return kRTFFontStyleNone;

	return m_entries -> font_style;
}

inline void RTFState::SetFontSize(uint4 p_size)
{
	if (m_entries == NULL)
		return;

	m_entries -> font_size = p_size;
}

inline uint4 RTFState::GetFontSize(void) const
{
	if (m_entries == NULL)
		return 0;

	return m_entries -> font_size;
}

inline void RTFState::SetForegroundColor(uint4 p_color)
{
	if (m_entries == NULL)
		return;

	m_entries -> foreground_color = p_color;
}

inline uint4 RTFState::GetForegroundColor(void) const
{
	if (m_entries == NULL)
		return 0xffffffff;

	return m_entries -> foreground_color;
}

inline void RTFState::SetBackgroundColor(uint4 p_color)
{
	if (m_entries == NULL)
		return;

	m_entries -> background_color = p_color;
}

inline uint4 RTFState::GetBackgroundColor(void) const
{
	if (m_entries == NULL)
		return 0xffffffff;

	return m_entries -> background_color;
}

inline void RTFState::SetListLevel(uint32_t p_level)
{
	if (m_entries == NULL)
		return;

	m_entries -> list_level = p_level;
}

inline uint32_t RTFState::GetListLevel(void) const
{
	if (m_entries == NULL)
		return 0;

	return m_entries -> list_level;
}

inline void RTFState::SetListIndex(uint32_t p_level)
{
	if (m_entries == NULL)
		return;

	m_entries -> list_index = p_level;
}

inline uint32_t RTFState::GetListIndex(void) const
{
	if (m_entries == NULL)
		return 0;

	return m_entries -> list_index;
}

inline void RTFState::SetListStyle(MCTextListStyle p_style)
{
	if (m_entries == NULL)
		return;

	m_entries -> list_style = p_style;
}

inline MCTextListStyle RTFState::GetListStyle(void) const
{
	if (m_entries == NULL)
		return kMCTextListStyleNone;

	return m_entries -> list_style;
}


inline void RTFState::SetTextAlign(MCTextTextAlign p_align)
{
	if (m_entries == NULL)
		return;
	m_entries -> text_align = p_align;
}

inline MCTextTextAlign RTFState::GetTextAlign(void) const
{
	if (m_entries == nil)
		return kMCTextTextAlignLeft;
	return m_entries -> text_align;
}

inline void RTFState::SetBorderWidth(uint32_t p_width)
{
	if (m_entries == NULL)
		return;
	m_entries -> border_width = p_width;
}

inline uint32_t RTFState::GetBorderWidth(void) const
{
	if (m_entries == nil)
		return 0;
	return m_entries -> border_width;
}

inline void RTFState::SetPadding(uint32_t p_value)
{
	if (m_entries == NULL)
		return;
	m_entries -> padding = p_value;
}

inline uint32_t RTFState::GetPadding(void) const
{
	if (m_entries == nil)
		return 0;
	return m_entries -> padding;
}

inline void RTFState::SetFirstIndent(int32_t p_value)
{
	if (m_entries == NULL)
		return;
	m_entries -> first_indent = p_value;
}

inline int32_t RTFState::GetFirstIndent(void) const
{
	if (m_entries == nil)
		return 0;
	return m_entries -> first_indent;
}

inline void RTFState::SetLeftIndent(int32_t p_value)
{
	if (m_entries == NULL)
		return;
	m_entries -> left_indent = p_value;
}

inline int32_t RTFState::GetLeftIndent(void) const
{
	if (m_entries == nil)
		return 0;
	return m_entries -> left_indent;
}

inline void RTFState::SetRightIndent(int32_t p_value)
{
	if (m_entries == NULL)
		return;
	m_entries -> right_indent = p_value;
}

inline int32_t RTFState::GetRightIndent(void) const
{
	if (m_entries == nil)
		return 0;
	return m_entries -> right_indent;
}

inline void RTFState::SetSpaceAbove(int32_t p_value)
{
	if (m_entries == NULL)
		return;
	m_entries -> space_above = p_value;
}

inline int32_t RTFState::GetSpaceAbove(void) const
{
	if (m_entries == nil)
		return 0;
	return m_entries -> space_above;
}

inline void RTFState::SetSpaceBelow(int32_t p_value)
{
	if (m_entries == NULL)
		return;
	m_entries -> space_below = p_value;
}

inline int32_t RTFState::GetSpaceBelow(void) const
{
	if (m_entries == nil)
		return 0;
	return m_entries -> space_below;
}

inline void RTFState::SetParagraphBackgroundColor(uint32_t p_value)
{
	if (m_entries == NULL)
		return;
	m_entries -> paragraph_background_color = p_value;
}

inline uint32_t RTFState::GetParagraphBackgroundColor(void) const
{
	if (m_entries == nil)
		return 0xffffffff;
	return m_entries -> paragraph_background_color;
}

inline void RTFState::SetBorderColor(uint32_t p_value)
{
	if (m_entries == NULL)
		return;
	m_entries -> border_color = p_value;
}

inline uint32_t RTFState::GetBorderColor(void) const
{
	if (m_entries == nil)
		return 0xffffffff;
	return m_entries -> border_color;
}

inline void RTFState::SetMetadata(MCStringRef p_metadata)
{
	if (m_entries == NULL)
		return;
    /* UNCHECKED */ MCStringCopy(p_metadata, m_entries -> metadata);
}

inline MCStringRef RTFState::GetMetadata(void)
{
	if (m_entries == NULL)
        return kMCEmptyString;
	return m_entries -> metadata;
}

inline void RTFState::SetParagraphMetadata(MCStringRef p_metadata)
{
	if (m_entries == NULL)
		return;
    /* UNCHECKED */ MCStringCopy(p_metadata, m_entries -> paragraph_metadata);
}

inline MCStringRef RTFState::GetParagraphMetadata(void)
{
	if (m_entries == NULL)
        return kMCEmptyString;
	return m_entries -> paragraph_metadata;
}

inline void RTFState::SetHyperlink(MCNameRef p_hyperlink)
{
	if (m_entries == NULL)
		return;
    m_entries->hyperlink = MCValueRetain(p_hyperlink);
}

inline MCNameRef RTFState::GetHyperlink(void)
{
	if (m_entries == NULL)
		return kMCEmptyName;
	return m_entries -> hyperlink;
}

//

class RTFFontTable
{
public:
	RTFFontTable(void);
	~RTFFontTable(void);
	
	RTFStatus Define(uint4 p_index, char *p_name, uint4 p_charset);
	
	const char *GetName(uint4 p_index) const;
	uint4 GetCharset(uint4 p_index) const;
	
private:
	bool Extend(void);
	bool Find(uint4 p_index, uint4& r_location) const;

	struct Entry
	{
		uint4 index;
		char *name;
		uint4 charset;
	};
	
	Entry *m_entries;
	uint4 m_entry_count;
};

inline const char *RTFFontTable::GetName(uint4 p_index) const
{
	uint4 t_location;
	if (!Find(p_index, t_location))
		return NULL;
		
	return m_entries[t_location] . name;
}

inline uint4 RTFFontTable::GetCharset(uint4 p_index) const
{
	uint4 t_location;
	if (!Find(p_index, t_location))
		return 0;
		
	return m_entries[t_location] . charset;
}

//

class RTFColorTable
{
public:
	RTFColorTable(void);
	~RTFColorTable(void);
	
	RTFStatus Define(uint4 p_color);
	
	uint4 Get(uint4 p_index) const;
	
private:
	bool Extend(void);
	
	uint4 *m_colors;
	uint4 m_color_count;
};

inline uint4 RTFColorTable::Get(uint4 p_index) const
{
	if (p_index >= m_color_count)
		return 0xffffffff;
	
	return m_colors[p_index];
}

//

class RTFListTable
{
public:
	RTFListTable(void);
	~RTFListTable(void);
	
	uint32_t Count(void) {return m_list_count;}
	
	RTFStatus Define(void);
	void SetId(int4 list_id);
	void SetStyle(uint32_t level, MCTextListStyle list_style);
	
	bool Get(int4 list_id, uint32_t level, MCTextListStyle& r_style);
	
private:
	struct Entry
	{
		int4 list_id;
		MCTextListStyle list_styles[10];
	};
	
	bool Extend(void);

	uint32_t m_list_count;
	Entry *m_lists;
};

class RTFListOverrideTable
{
public:
	RTFListOverrideTable(void);
	~RTFListOverrideTable(void);
	
	RTFStatus Define(void);
	void SetListId(int4 list_id);
	void SetOverrideId(int4 override_id);
	
	bool Get(int4 override_id, int4& r_list_id);
	
private:
	struct Entry
	{
		int4 override_id;
		int4 list_id;
	};
	
	bool Extend(void);
	
	uint32_t m_override_count;
	Entry *m_overrides;
};

//

class RTFBuffer
{
public:
	RTFBuffer(void)
	{
		m_base = NULL;
		m_frontier = 0;
		m_capacity = 0;
	}

	~RTFBuffer(void)
	{
		if (m_base != NULL)
			free(m_base);
	}

	void Clear(void)
	{
		m_frontier = 0;
	}

	//

	bool AppendByte(uint1 p_byte)
	{
		if (!Ensure(1))
			return false;

		((uint1 *)m_base)[m_frontier] = p_byte;
		m_frontier += 1;

		return true;
	}

	bool AppendShort(uint2 p_short)
	{
		if (!Ensure(2))
			return false;

		((uint2 *)m_base)[m_frontier / 2] = p_short;
		m_frontier += 2;

		return true;
	}

	//

	bool Ensure(uint4 p_amount)
	{
		if (m_capacity - m_frontier < p_amount)
			Extend(p_amount);

		return true;
	}

	void Borrow(void*& r_frontier, uint4& r_available)
	{
		r_frontier = (char *)m_base + m_frontier;
		r_available = m_capacity - m_frontier;
	}

	void Advance(uint4 p_used)
	{
		m_frontier += p_used;
	}

	//

	const void *GetBase(void) const
	{
		return m_base;
	}

	uint4 GetLength(void) const
	{
		return m_frontier;
	}

private:
	bool Extend(uint4 p_amount)
	{
		uint4 t_new_capacity;
		t_new_capacity = (m_frontier + p_amount + 4095) & ~4095;
		
		void *t_new_base;
		t_new_base = realloc(m_base, t_new_capacity);
		if (t_new_base == NULL)
			return false;

		m_base = t_new_base;
		m_capacity = t_new_capacity;

		return true;
	}

	void *m_base;
	uint4 m_frontier;
	uint4 m_capacity;
};

//

class RTFText
{
public:
	RTFText(void);
	~RTFText(void);
	
	void Clear(void);
	
	RTFStatus Output(int p_character, MCTextEncoding p_encoding);

	RTFStatus Borrow(const uint2*& r_buffer, uint4& r_length);
	
private:
	bool Flush(void);

	MCTextEncoding m_input_encoding;
	RTFBuffer m_input;

	RTFBuffer m_output;
};

inline RTFText::RTFText(void)
{
	m_input_encoding = kMCTextEncodingUndefined;
}

inline RTFText::~RTFText(void)
{
}

inline void RTFText::Clear(void)
{
	m_input . Clear();
	m_output . Clear();
}

inline RTFStatus RTFText::Output(int p_character, MCTextEncoding p_encoding)
{
	if (m_input_encoding != p_encoding && m_input . GetLength() > 0)
		if (!Flush())
			return kRTFStatusNoMemory;

	switch(p_encoding)
	{
	case kMCTextEncodingASCII:
		if (!m_output . AppendShort(p_character & 0xFF))
			return kRTFStatusNoMemory;
	break;

	case kMCTextEncodingUTF16:
		if (!m_output . AppendShort(p_character & 0xFFFF))
			return kRTFStatusNoMemory;
	break;

	default:
		m_input_encoding = p_encoding;
		if (!m_input . AppendByte(p_character & 0xFF))
			return kRTFStatusNoMemory;
	break;
	}

	return kRTFStatusSuccess;
}

inline RTFStatus RTFText::Borrow(const uint2*& r_buffer, uint4& r_length)
{
	if (m_input . GetLength() > 0)
		if (!Flush())
			return kRTFStatusNoMemory;

	r_buffer = (const uint2 *)m_output . GetBase();
	r_length = m_output . GetLength();

	return kRTFStatusSuccess;
}

//

class RTFReader
{
public:
	RTFReader(void);
	~RTFReader(void);
	
	void SetConverter(MCTextConvertCallback p_converter, void *p_context);

	RTFStatus Process(const char *p_rtf, uint4 p_length);
	
private:
	RTFStatus Parse(void);
	RTFStatus ParseToken(RTFToken& r_token, int4& r_value);
	RTFStatus ParseDocument(RTFToken p_token, int4 p_value);
	RTFStatus ParseFontTable(RTFToken p_token, int4 p_value);
	RTFStatus ParseColorTable(RTFToken p_token, int4 p_value);

	RTFStatus ParseLegacyList(RTFToken p_token, int4 p_value);
	RTFStatus ParseLegacyListPrefix(RTFToken p_token, int4 p_value);
	RTFStatus ParseListTable(RTFToken p_token, int4 p_value);
	RTFStatus ParseListTableLevelText(RTFToken p_token, int4 p_value);
	RTFStatus ParseListOverrideTable(RTFToken p_token, int4 p_value);
	RTFStatus ParseListText(RTFToken p_token, int4 p_value);
	RTFStatus ParseField(RTFToken p_token, int4 p_value);
	RTFStatus ParseFldInst(RTFToken p_token, int4 p_value);
	
	void ProcessField(void);

	RTFStatus Flush(bool p_force = false);
	RTFStatus Paragraph(void);

	RTFText m_text;
	RTFState m_state;
	RTFFontTable m_fonts;
	RTFColorTable m_colors;
	RTFListTable m_lists;
	RTFListOverrideTable m_overrides;

	MCTextConvertCallback m_converter;
	void *m_converter_context;

	const char *m_input;
	const char *m_input_end;

	int4 m_input_binary_count;
	uint4 m_input_skip_count;
	RTFInputState m_input_state;

	bool m_color_started;
	uint1 m_color_red;
	uint1 m_color_green;
	uint1 m_color_blue;
	
	int32_t m_listtable_level_count;
	
	bool m_font_skip;
	bool m_font_started;
	char *m_font_name;
	int4 m_font_index;
	int4 m_font_charset;

	bool m_table_cell;

	int4 m_default_font;
	MCTextEncoding m_default_text_encoding;

	bool m_attributes_changed;
	MCTextBlock m_attributes;

	bool m_needs_paragraph;

	MCTextListStyle m_list_style;
	uint32_t m_list_level;
	bool m_list_skip;

	char *m_field_inst;

	static RTFStatus LookupKeyword(const char *p_keyword, int4 p_keyword_length, RTFToken& r_token);
	static RTFStatus LookupNibble(int p_char, int& r_nibble);
};

#endif
