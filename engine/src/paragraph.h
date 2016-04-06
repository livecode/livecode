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

#ifndef __MC_PARAGRAPH__
#define __MC_PARAGRAPH__

#if 0
enum
{
	kMCParagraphCharStyleBoldInherit = 0,
	kMCParagraphCharStyleBoldOn = 1,
	kMCParagraphCharStyleBoldOff = 2,

	kMCParagraphCharStyleItalicInherit = 0,
	kMCParagraphCharStyleItalicOn = 1,
	kMCParagraphCharStyleItalicOff = 2,

	kMCParagraphCharStyleBoxInherit = 0,
	kMCParagraphCharStyleBoxFlat = 1,
	kMCParagraphCharStyleBoxRaised = 2,
	kMCParagraphCharStyleBoxNone = 3,

	kMCParagraphCharStyleLinkInherit = 0,
	kMCParagraphCharStyleLinkNone = 1,
	kMCParagraphCharStyleLinkNormal = 2,

	kMCParagraphCharStyleUnderlineInherit = 0,
	kMCParagraphCharStyleUnderlineNone = 1,
	kMCParagraphCharStyleUnderlineSingle = 2,

	kMCParagraphCharStyleStrikeoutInherit = 0,
	kMCParagraphCharStyleStrikeoutNone = 1,
	kMCParagraphCharStyleStrikeoutSingle = 2,

	kMCParagraphCharStyleExpandInherit = 0,
	kMCParagraphCharStyleExpandNone = 1,
	kMCParagraphCharStyleExpandExtend = 2,
	kMCParagraphCharStyleExpandCondense = 3
};
#endif

struct MCParagraphCharStyle
{
#if 0
	unsigned bold : 2;
	unsigned italic : 2;
	unsigned box : 2;
	unsigned underline : 2;
	unsigned strikeout : 2;
	unsigned expand : 2;
	unsigned link : 2;
#endif
	bool is_unicode : 1;

	bool has_foreground : 1;
	bool has_background : 1;
	bool has_font : 1;
	bool has_size : 1;
	bool has_attrs : 1;
	bool has_shift : 1;
	bool has_link : 1;
	bool has_image : 1;

	uint4 foreground;
	uint4 background;

	uint2 font;
	uint2 size;
	uint2 attrs;
	int2 shift;

	uint2 link;
	uint2 image;
};

enum MCParagraphCursorMove
{
	kMCParagraphCursorMoveCharacterBoundary,
	kMCParagraphCursorMoveCharacterBegin,
	kMCParagraphCursorMoveCharacterEnd,
	kMCParagraphCursorMoveParagraphBoundary,
	kMCParagraphCursorMoveParagraphBegin,
	kMCParagraphCursorMoveParagraphEnd,
	kMCParagraphCursorMoveWordBoundary,
	kMCParagraphCursorMoveWordBegin,
	kMCParagraphCursorMoveWordEnd,
	kMCParagraphCursorMoveSentenceBoundary,
	kMCParagraphCursorMoveSentenceBegin,
	kMCParagraphCursorMoveSentenceEnd
};

///////////////////////////////////////////////////////////////////////////////

class MCParagraphPosition
{
public:
	MCParagraphPosition(MCParagraph *p_owner);
	MCParagraphPosition(MCParagraph *p_owner, uint4 p_offset);
	MCParagraphPosition(const MCParagraphPosition& p_other);

	MCParagraphPosition& operator = (const MCParagraphPosition& p_other);

	uint4 GetOffset(void) const;

	bool Retreat(void);
	bool Advance(void);

	uint4 GetNativeCodepoint(void) const;
	uint4 GetUTF16Codepoint(void) const;
	uint4 GetCodepoint(void) const;

private:
	MCParagraph *m_target;
	bool m_unicode;
	uint2 m_entry;
	uint2 m_offset;
};

inline uint4 MCParagraphPosition::GetOffset(void) const
{
	return m_offset;
}

///////////////////////////////////////////////////////////////////////////////

class MCParagraphCursor
{
public:
	MCParagraphCursor(MCParagraph *p_field);
	MCParagraphCursor(MCParagraph *p_field, uint4 p_index);
	MCParagraphCursor(MCParagraph *p_field, uint4 p_start, uint4 p_finish);

	void Collapse(int4 p_delta);

	bool Move(MCParagraphCursorMove p_movement, int4 p_delta);
	bool MoveStart(MCParagraphCursorMove p_movement, int4 p_delta);
	bool MoveFinish(MCParagraphCursorMove p_movement, int4 p_delta);

	MCParagraph *Clone(void);
	void Delete(void);
	void Replace(const MCParagraphCursor& p_other);

	uint4 GetOffset(void) const;
	uint4 GetStartOffset(void) const;
	uint4 GetFinishOffset(void) const;

private:
	static bool BoundaryMatch(MCParagraphCursorMove p_movement, uint4 p_left, uint4 p_right);

	//

	bool MoveIndex(MCParagraphPosition& x_cursor, MCParagraphCursorMove p_movement, int4 p_delta);

	//

	MCParagraph *m_target;
	MCParagraphPosition m_start;
	MCParagraphPosition m_finish;
};

inline uint4 MCParagraphCursor::GetOffset(void) const
{
	return m_start . GetOffset();
}

inline uint4 MCParagraphCursor::GetStartOffset(void) const
{
	return m_start . GetOffset();
}

inline uint4 MCParagraphCursor::GetFinishOffset(void) const
{
	return m_finish . GetOffset();
}

///////////////////////////////////////////////////////////////////////////////

class MCParagraph: public MCDLlist
{
public:
	MCParagraph(void);
	MCParagraph(const MCParagraph& p_other);
	MCParagraph(const MCParagraph& p_original, uint4 p_first, uint4 p_last);
	~MCParagraph(void);

	// Old Interface
	
	IO_stat load_block(IO_handle stream, const char *version, uint4& r_block_offset, uint4& r_block_length, uint4& r_style);
	IO_stat load(IO_handle stream, const char *version);
	IO_stat save(IO_handle stream, uint4 p_part);

	void open(void);
	void close(void);
	uint2 getopened(void);

	MCField *getparent(void);
	void setparent(MCField *p_new_parent);

	void flow(MCFontStruct *p_font);
	void noflow(MCFontStruct *p_font);

	void join(void);
	void split(void);

	void deletestring(uint2 ui, uint2 ei);

	void deleteselection(void);
	MCParagraph *copyselection(void);

	Boolean finsert(const MCString& p_string, bool p_dont_break);
	int2 fdelete(Field_translations p_type, MCParagraph*& r_undo);
	uint1 fmovefocus(Field_translations p_type);

	void clean(void);

	uint2 gettextsize(void);
	uint2 gettextsizecr(void);
	uint2 gettextlength(void);
	const char *gettext(void);
	void gettext(char *p_dest, bool p_unicode);
	char *getformattedtext(uint2& p_length);
	void gethtmltext(MCExecPoint& ep);

	void settext(char *p_text, uint2 p_length);
	void resettext(char *p_text, uint2 p_length);

	void getmaxline(uint2 &width, uint2 &aheight, uint2 &dheight);
	uint2 getheight(uint2 fixedheight);
	uint2 getwidth();
	uint2 getyextent(int4 tindex, uint2 fixedheight);
	void getxextents(int4 &si, int4 &ei, int2 &minx, int2 &maxx,
	                 MCFontStruct *pfont, uint4 align);

	Boolean isselection(void);
	void getselectionindex(uint2& si, uint2& ei);
	void setselectionindex(uint2 si, uint2 ei, Boolean front, Boolean back);
	void reverseselection(void);

	void sethilite(Boolean p_new_hilite);
	Boolean gethilite(void);

	const char *getlinktext(uint2 si);
	const char *getimagesource(uint2 si);

	Boolean getvisited(uint2 si);
	void setvisited(uint2 si, uint2 ei, Boolean v);

	// New Interface
	void draw(MCContext *gc,
		int2 x, int2 y, uint4 align,
		uint2 fa, uint2 fd, uint2 fstart, uint2 fend,
		uint2 compstart, uint2 compend, uint1 compconvstart, uint1 compconvend,
		int2 sx, uint2 swidth,
		const char *pname, uint2 psize, uint2 pstyle, MCFontStruct *pfont);

	int2 setfocus(int4 x, int4 y, uint4 align, uint2 fixedheight,
		Boolean extend, Boolean extendwords, Boolean extendlines,
		int2 direction, Boolean first, Boolean last, Boolean deselect,
		MCFontStruct *pfont);

	MCRectangle getdirty(uint4 align, uint2 fixedheight);
	MCRectangle getcursorrect(int4 fi, uint4 align, uint2 fixedheight, MCFontStruct *pfont);

	void indextoloc(uint2 tindex, uint4 align, uint2 fixedheight, int2& x, int2& y, MCFontStruct *pfont);
	void getclickindex(int2 x, int2 y, uint4 align, uint2 fixedheight, uint2& si, uint2& ei, MCFontStruct *pfont, Boolean wholewords, Boolean chunk);

	Boolean getatts(uint2 si, uint2 ei, const char*& fname, uint2& size, uint2& style,
		const MCColor*& color, const MCColor*& backcolor, int2& shift, uint2& mixed);
	void setatts(uint2 si, uint2 ei, Properties which, const char *fname, uint2 size, uint2 style,
		const MCColor* color, const char *cname, int2 shift);
	
	MCParagraph *next(void) {return (MCParagraph *)MCDLlist::next();}
	MCParagraph *prev(void) {return (MCParagraph *)MCDLlist::prev();}
	void totop(MCParagraph *&list) {MCDLlist::totop((MCDLlist *&)list);}
	void insertto(MCParagraph *&list) {MCDLlist::insertto((MCDLlist *&)list);}
	void appendto(MCParagraph *&list) {MCDLlist::appendto((MCDLlist *&)list);}
	void append(MCParagraph *node) {MCDLlist::append((MCDLlist *)node);}
	void splitat(MCParagraph *node) {MCDLlist::splitat((MCDLlist *)node);}
	MCParagraph *remove(MCParagraph *&list) {return (MCParagraph *)MCDLlist::remove((MCDLlist *&)list);}

private:
	//

	enum
	{
		kStateReflow = 1 << 0,
		kStateHilited = 1 << 12,
		kStateTextAvailable = 1 << 13,
		kStateStylesAvailable = 1 << 14,
		kStateLinesAvailable = 1 << 15
	};

	//

	void Delete(uint4 p_start, uint4 p_finish);
	void Replace(uint4 p_start, uint4 p_finish, const MCParagraph *p_other, uint4 p_other_start, uint4 p_other_end);
	void Compact(void);

	//

	void CloneText(void*& r_text, uint2& r_text_size) const;
	void CloneStyles(uint4*& r_styles, uint2& r_styles_size) const;

	void CopyText(uint4 p_start, uint4 p_finish, void*& r_text, uint2& r_text_size) const;
	void CopyStyles(uint4 p_start, uint4 p_finish, uint4*& r_styles, uint2& r_styles_size) const;

	void DestroyText(void);
	void DestroyStyles(void);
	void DestroyLines(void);

	void ReplaceText(uint4 p_start, uint4 p_finish, const void *p_other_text, uint4 p_other_start, uint4 p_other_finish);
	void ReplaceStyles(uint4 p_start, uint4 p_finish, uint4 p_style);

	void DeleteStyles(uint4 p_start, uint4 p_finish);

	static void DoSearchStyleEntries(const uint4* p_styles, uint4 p_start, uint4 p_finish, uint4& r_lower, uint4& r_higher);

	//

	void SearchStyleEntries(uint4 p_start, uint4& p_entry) const;
	void SearchStyleEntries(uint4 p_start, uint4 p_finish, uint4& r_lower_index, uint4& r_higher_index) const;

	void DeleteStyleEntries(uint4 p_from, uint4 p_count);
	bool InsertStyleEntries(uint4 p_next, uint4 p_count);

	uint4 FetchStylesCapacity(void);
	void StoreStylesCapacity(uint4 p_new_capacity);

	uint4 FetchTextCapacity(void);
	void StoreTextCapacity(uint4 p_new_capacity);

	//

	uint4 AcquireCharStyle(const MCParagraphCharStyle& p_style);
	const MCParagraphCharStyle *FetchCharStyle(uint4 p_index);
	void RetainCharStyle(uint4 p_index);
	void ReleaseCharStyle(uint4 p_index);

	uint4 AcquireLink(const char *p_text);
	void ReleaseLink(uint4 p_index);

	uint4 AcquireImage(const char *p_text);
	void ReleaseImage(uint4 p_index);

	uint4 AcquireFont(const char *p_font_name);
	void ReleaseFont(uint4 p_index);

	//

	static uint4 GetStyleEntryIndex(uint4 p_style);
	static uint4 GetStyleEntryOffset(uint4 p_style);
	static uint4 ShiftStyleEntry(uint4 p_entry, int4 p_delta);
	static uint4 MakeStyleEntry(uint4 p_offset, uint4 p_style);

	//

	uint2 m_state;
	uint2 m_opened;

	uint2 m_style_table;
	uint2 m_object_table;

	uint2 m_focused_index;
	uint2 m_start_index;
	uint2 m_end_index;
	uint2 m_original_index;

	uint2 m_text_size;
	uint2 m_styles_size;
	uint2 m_lines_size;

	void *m_text;
	uint4 *m_styles;
	uint2 *m_lines;

	MCField *m_parent;

	friend MCParagraphCursor;
	friend MCParagraphPosition;
};

inline uint4 MCParagraph::GetStyleEntryIndex(uint4 p_style)
{
	return p_style & 0xffff;
}

inline uint4 MCParagraph::GetStyleEntryOffset(uint4 p_style)
{
	return p_style >> 16;
}

inline uint4 MCParagraph::MakeStyleEntry(uint4 p_offset, uint4 p_style)
{
	return (p_style & 0xffff) | (p_offset << 16);
}

inline uint4 MCParagraph::ShiftStyleEntry(uint4 p_entry, int4 p_delta)
{
	return p_entry + (p_delta << 16);
}

#endif
