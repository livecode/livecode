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

//
// MCParagraphs within a field
//
#ifndef	PARAGRAPH_H
#define	PARAGRAPH_H

#include "dllst.h"

#ifndef __MC_FONT__
#include "font.h"
#endif

#define MIXED_NONE   0x00
#define MIXED_NAMES  (1L << 0)
#define MIXED_SIZES  (1L << 1)
#define MIXED_STYLES (1L << 2)
#define MIXED_COLORS (1L << 3)
#define MIXED_SHIFT  (1L << 4)
#define MIXED_ALIGN  (1L << 5)
#define MIXED_LIST	 (1L << 6)
#define MIXED_SPEC_STYLE (1L << 7)

#define PG_MAX_INDEX_SIZE 32

#define PG_PAD 32
#define PG_MASK 0xFFE0

#define PA_HAS_TEXT_ALIGN (1 << 0)
#define PA_HAS_LIST_STYLE (1 << 1)
#define PA_HAS_FIRST_INDENT (1 << 2)
#define PA_HAS_LEFT_INDENT (1 << 3)
#define PA_HAS_RIGHT_INDENT (1 << 4)
#define PA_HAS_SPACE_ABOVE (1 << 5)
#define PA_HAS_SPACE_BELOW (1 << 6)
#define PA_HAS_TABS (1 << 7)
#define PA_HAS_BACKGROUND_COLOR (1 << 8)
#define PA_HAS_BORDER_WIDTH (1 << 9)
#define PA_HAS_LIST_INDENT (1 << 10)
#define PA_HAS_HGRID (1 << 11)
#define PA_HAS_VGRID (1 << 12)
#define PA_HAS_BORDER_COLOR (1 << 13)
#define PA_HAS_DONT_WRAP (1 << 14)
#define PA_HAS_PADDING (1 << 15)
// MW-2012-03-05: [[ HiddenText ]] This flag is set if the hidden flag is
//   true. This doesn't strictly need a flag, but it makes it easier to see
//   if there are any attrs set if it does.
#define PA_HAS_HIDDEN (1 << 16)
// MW-2012-11-13: [[ ParaMetadata ]] This flag is set if the paragraph has
//   metadata set.
#define PA_HAS_METADATA (1 << 17)
// MW-2012-11-13: [[ ParaListIndex ]] This flag is set if the paragraph has
//   a list index attribute set.
#define PA_HAS_LIST_INDEX (1 << 18)

enum
{
	kMCParagraphTextAlignLeft,
	kMCParagraphTextAlignCenter,
	kMCParagraphTextAlignRight,
	kMCParagraphTextAlignJustify
};

enum
{
	kMCParagraphListStyleNone,
	kMCParagraphListStyleDisc,
	kMCParagraphListStyleCircle,
	kMCParagraphListStyleSquare,
	kMCParagraphListStyleNumeric,
	kMCParagraphListStyleLowerCase,
	kMCParagraphListStyleUpperCase,
	kMCParagraphListStyleLowerRoman,
	kMCParagraphListStyleUpperRoman,
	kMCParagraphListStyleSkip,
};

// MW-2012-01-25: [[ ParaStyles ]] A collection of paragraph attributes.
struct MCParagraphAttrs
{
	unsigned flags : 19;
	unsigned text_align : 2;
	unsigned list_style : 4;
	unsigned list_depth : 4;
	bool vgrid : 1;
	bool hgrid : 1;
	bool dont_wrap : 1;
	// MW-2012-03-05: [[ HiddenText ]] True if the paragraph isn't currently
	//   being displayed.
	bool hidden : 1;
	uint8_t border_width;
	uint8_t padding;
	int16_t first_indent;
	int16_t left_indent;
	int16_t right_indent;
	int16_t space_above;
	int16_t space_below;
	uint16_t tab_count;
	uint16_t *tabs;
	uint16_t list_index;
	uint32_t background_color;
	uint32_t border_color;
	MCNameRef metadata;

	MCParagraphAttrs(void)
	{
		memset(this, 0, sizeof(MCParagraphAttrs));
	}
};

class MCParagraph : public MCDLlist
{
	friend class MCField;

	MCField *parent;
	char *text;
	uint2 buffersize;
	uint2 textsize;
	MCBlock *blocks;
	MCLine *lines;
	uint2 focusedindex;
	uint2 startindex, endindex, originalindex;
	uint2 opened;
	uint1 state;
	// MW-2012-01-25: [[ ParaStyles ]] This paragraphs collection of attrs.
	MCParagraphAttrs *attrs;

	static uint2 cursorwidth;

public:
	MCParagraph();
	MCParagraph(const MCParagraph &pref);
	~MCParagraph();

	bool visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor* p_visitor);

	// MW-2012-03-04: [[ StackFile5500 ]] If 'is_ext' is true then this paragraph
	//   has an attribute extension.
	IO_stat load(IO_handle stream, const char *version, bool is_ext);
	IO_stat save(IO_handle stream, uint4 p_part);
	
	// MW-2012-02-14: [[ FontRefs ]] Now takes the parent fontref so it can compute
	//   block's fontrefs.
	void open(MCFontRef parent_font);
	void close();

	MCField *getparent()
	{
		return parent;
	}
	void setparent(MCField *newparent);

	// MW-2012-02-14: [[ FontRefs ]] Invoked to recompute the block's fontrefs based
	//   on a new parent fontref.
	bool recomputefonts(MCFontRef parent_font);

	// Iterate backwards starting at p_index and return the first index in the
	// paragraph before which a word-break can occur.
	uint2 findwordbreakbefore(MCBlock *p_block, uint2 p_index);

	// Iterate forwards starting at p_index and return the first index in the
	// paragraph after which a word-break can occur.
	uint2 findwordbreakafter(MCBlock *p_block, uint2 p_index);

	// Make sure there are no empty blocks in the paragraph:
	//   - called by MCField::deleteselection
	//   - called by MCField::fdel
	//   - called by MCField::freturn
	//   - called by methods in MCParagraph
	Boolean clearzeros();
	
	// Return the list of blocks:
	//   - called by MCField::getparagraphmacstyles
	MCBlock *getblocks()
	{
		return blocks;
	}

	// Return the list of lines in the paragraph, if any.
	MCLine *getlines(void) const
	{
		return lines;
	}

	// Make sure style-runs that are next to each other don't have
	// the same attributes.
	// This is called in a number of places after style mutation has
	// been applied.
	void defrag();

	// Return the block containing the given index - if forinsert is
	// true, then it splits the block at that point.
	// Called by:
	//   MCField::finsert - to check the charset of the target block
	//   MCField::verifyindex - to check the integrity of an index
	//   MCField::getlinkdata - to check if the given block is a link
	MCBlock *indextoblock(uint2 tindex, Boolean forinsert);

	// Join this paragraph with the next paragraph or split the current
	// paragraph at the focused index.
	// Called by:
	//   MCField::joinparagraphs
	//   MCField::settextindex
	//   MCField::gettextatts (to fetch partial parts of a paragraph)
	//   MCField::settextatts (to insert partial parts of a paragraph)
	//   MCField::insertparagraph
	//   
	void join();
	void split();

	// Delete the text from si to ei in the paragraph.
	// Called by:
	//   MCField::deletecomposition
	//   MCField::settextindex
	void deletestring(uint2 si, uint2 ei);

	// Delete the current selection in the paragraph.
	// Called by:
	//   MCField::deleteselection
	void deleteselection();

	// Cut the paragraph from the field.
	// Called by:
	//   MCField::fcutline
	MCParagraph *cutline();

	// Return a copy of the current selected portion of the paragraph as a new
	// paragraph.
	// Called by
	//   MCField::cloneselection
	MCParagraph *copyselection();

	// Insert the given string into the paragraph at the focusedindex.
	// Called by any client that does text insertion.

	void finsertnobreak(const MCString& text, bool is_unicode);
	Boolean finsertnew(const MCString& text, bool is_unicode);

	// Delete the specified chunk at the focusedindex and return the deleted
	// string in undopgptr.
	int2 fdelete(Field_translations type, MCParagraph *&undopgptr);

	// Move the focused index to the place specified by type.
	// Called by:
	//   MCField::fmove
	uint1 fmovefocus(Field_translations type);

	// Initialize the text buffer
	// Called by:
	//   MCField::getparagraphmacstyles
	//   MCField::getparagraphshasunicode
	void inittext();

	// Reset the dirtywidths of each line in the paragraph.
	// Called by:
	//   MCField::updateparagraph
	void clean();

	// If there is no text buffer then initialize it and return the length of the
	// paragraph in bytes.
	uint2 gettextsize()
	{
		if (blocks != NULL)
			return textsize;
			
		inittext();
		return textsize;
	}

	// Same as gettextsize, except adjust by one for the CR character.
	uint2 gettextsizecr()
	{
		if (blocks != NULL)
			return textsize + 1;
			
		inittext();
		return textsize + 1;
	}

	// Return the length of the paragraph in codepoints *not* bytes.
	// Called by:
	//   MCField::getparagraphmacstyles
	//   MCField::getparagraphtext
	//   MCField::indextocharacter
	//   MCField::charactertoindex
	//   MCIdeScriptColorize::exec
	uint2 gettextlength();

	// 'gettext()' returns a direct pointer to the backstore
	// Called by:
	//   MCField::getparagraphtext
	//   MCField::getparagraphtextrtf
	//   MCField::sort
	//   MCField::find
	//   MCField::gettext
	//   MCField::selectedtext
	//   MCIdeScriptColorize::exec
	const char *gettext();

	// Return the text as HTML formatted string.
	// Called by:
	//   MCField::gethtmltext
	void gethtmltext(MCExecPoint &ep);

	// Clear everything in the current paragraph and set the text to the
	// given string.
	// Called by:
	//   MCField::settext
	//   MCHctext::settext
	void settext(char *tptr, uint2 length, bool p_is_unicode);

	// Set the backing store to the given text and length, then adjust
	// the final block (creating one if necessary) so it finishes at the
	// end of the paragraph.
	// Called by:
	//   MCField::texttoparagraphs
	//   MCField::htmltoparagraphs
	void resettext(char *tptr, uint2 length);

	// Calculate the maximum of the width, ascender and descender of all
	// lines that make up the paragraph.
	// Called by:
	//   MCField::recompute
	void getmaxline(uint2 &width, uint2 &aheight, uint2 &dheight);

	// Calculate the height of the paragraph, using the given fixedheight
	// if non-zero.
	// Called in a great many places!!
	uint2 getheight(uint2 fixedheight) const;

	// Calculate the width of the paragraph (which is the maximum of all
	// line widths).
	// Called by:
	//   MCField::updateparagraph
	//   MCField::fdel
	uint2 getwidth() const;

	// Returns true if any part of the line is selected.
	// Called by:
	//   MCField::kfocus
	//   MCField::kunfocus
	//   MCField::mfocus
	//   MCField::positioncursor
	//   MCField::startselection
	//   MCField::endselection
	//   MCField::unselect
	//   MCField::deleteselection
	//   MCField::fmove
	//   MCField::selectedmark
	//   MCField::cloneselection
	//   MCField::copytext
	Boolean isselection();

	// Return the current selected range in the paragraph
	// Called by:
	//   MCField::typetext
	//   MCField::selectedmark
	//   MCField::cloneselection
	void getselectionindex(uint2 &si, uint2 &ei);

	// Set the selected range of the paragraph
	// Called by:
	//   MCField::undo (front == back == False)
	//   MCField::unselect (removes selection)
	//   MCField::freturn (removes selection)
	//   MCField::settextindex (removes selection, sets to a single index)
	//   MCField::gettextatts (used to mark for split/join)
	//   MCField::settextatts (used to mark for split/join)
	//   MCField::seltext (removes selection, selects range/front/back)
	void setselectionindex(uint2 si, uint2 ei, Boolean front, Boolean back);

	// Reverses the 'originalindex' position in the paragraph
	// Called by:
	//   MCField::reverse
	void reverseselection();

	// Computes the height of the field (using fixedheight if non-zero) up
	// to and including the line containing tindex.
	// Called by:
	//   MCField::getlinkdata
	//   MCField::gettextatts
	uint2 getyextent(int4 tindex, uint2 fixedheight);

	// Set the hilite flag of the paragraph (overrides selected range)
	// Called by:
	//   MCField::kfocus (list mode)
	//   MCField::setfocus (list mode)
	//   MCField::clearhilites
	//   MCField::seltext (list mode)
	//   MCField::sethilitedlines (list mode)
	//   MCField::hiliteline (list mode)
	void sethilite(Boolean newhilite);

	// Returns true if the paragraph is 'list' hilited.
	// Called by:
	//   MCField::setfocus (list mode)
	//   MCField::clearhilites
	//   MCField::hilitedline(s)
	//   MCField::hiliteline
	//   MCField::selectedtext (list mode)
	//   MCField::selectedmark
	//   MCField::cloneselection
	Boolean gethilite();

	// Get the link text (if any) in the block containing index si
	// Called by:
	//   MCField::gettextatts
	const char *getlinktext(uint2 si);

	// Get the image source (if any) in the block contaning index si
	// Called by:
	//   MCField::gettextatts
	const char *getimagesource(uint2 si);

	// Get the metadata (if any) in the block containing index si
	const char *getmetadataatindex(uint2 si);

	// Return true if the link (if any) in the block containing index si
	// has been visited.
	// Called by:
	//   MCField::gettextatts
	Boolean getvisited(uint2 si);

	// Set the visited status of the blocks between si and ei to v.
	// Called by:
	//   MCField::settextatts
	void setvisited(uint2 si, uint2 ei, Boolean v);
	
	// Returns the state of a given block flag in the specified range.
	bool getflagstate(uint32_t flag, uint2 si, uint2 ei, bool& r_state);

	// MW-2012-02-08: [[ FlaggedRanges ]] Get the ranges of indices which have the
	//   flagged status set to true - these are then adjusted by delta.
	// MW-2012-02-24: [[ FieldChars ]] Pass in the part_id so the paragraph can map
	//   field indices to char indices.
	// MW-2013-07-31: [[ Bug 10957 ]] Pass in the start of the paragraph as a byte
	//   offset so that the correct char offset can be calculated.
	void getflaggedranges(uint32_t p_part_id, MCExecPoint& ep, uint2 si, uint2 ei, int32_t p_paragraph_start);

	// Return true if the paragraph completely fits in theight. Otherwise, return
	// false and set lastline to the line that would be clipped.
	// Called by:
	//   MCField::getprop
	Boolean pageheight(uint2 fixedheight, uint2 &theight, MCLine *&lastline);

    // JS-2013-05-15: [[ PageRanges ]] pagerange as variant of pageheight
	// Return true if the paragraph completely fits in theight. Otherwise, return
	// false and set lastline to the line that would be clipped.
	// Called by:
	//   MCField::getprop
	Boolean pagerange(uint2 fixedheight, uint2 &theight, uint2 &tend, MCLine *&lastline);

	// Returns true if any of the paragraph attributes are non-default.
	bool hasattrs(void);
	// Sets the given paragraph attribute to the value in ep.
	Exec_stat setparagraphattr(Properties which, MCExecPoint& ep);
	// Gets the given paragraph attribute into the given ep.
	Exec_stat getparagraphattr(Properties which, MCExecPoint& ep, Boolean effective);
	// Copies the given attribute from the given paragraph.
	void copysingleattr(Properties which, MCParagraph *other);
	// Copies all the attributes from the given paragraph.
	void copyattrs(const MCParagraph& other);
	// Stores the paragraph attributes into the dst array.
	void storeattrs(MCVariableValue *dst);
	// Fetches the paragraph attributes from the src array.
	void fetchattrs(MCVariableValue *src);
	// Clears the paragraph attributes.
	void clearattrs(void);
	// Unserializes the paragraph attributes from stream.
	IO_stat loadattrs(IO_handle stream);
	// Serializes the paragraph attributes into stream.
	IO_stat saveattrs(IO_handle stream);
	// MW-2012-02-21: [[ FieldExport ]] Fills in the appropriate members of the
	//   field export struct.
	void exportattrs(MCFieldParagraphStyle& x_style);
	// MW-2012-03-04: [[ FieldImport ]] Set the attributes of the paragraph to those described
	//  by the style.
	void importattrs(const MCFieldParagraphStyle& x_style);
	// MW-2012-03-03: [[ StackFile5500 ]] Computes the size of the attrs when serialized.
	uint32_t measureattrs(void);

	// MW-2012-03-05: [[ HiddenText ]] Get whether the paragraph is hidden or not.
	bool gethidden(void) const;

	// Get the (effective) listStyle.
	uint32_t getliststyle(void) const;
	// Set the liststyle property directly.
	void setliststyle(uint32_t new_list_style);

	// Returns true if the paragraph has listIndex set.
	bool haslistindex(void) const;
	// Gets the index of the pargraph, if set.
	uint32_t getlistindex(void) const;
	// Sets the index of the paragraph.
	void setlistindex(uint32_t new_list_index);

	// Get the (effective) listDepth
	uint32_t getlistdepth(void) const;
	// Get the (effective) textAlign.
	uint32_t gettextalign(void) const;
	// Get the (effective) spaceBefore.
	int32_t getspaceabove(void) const;
	// Get the (effective) spaceAfter.
	int32_t getspacebelow(void) const;
	// Get the (effective) borderWidth.
	int32_t getborderwidth(void) const;
	// Get the (effective) padding.
	int32_t getpadding(void) const;
	// Get the (effective) horizontal padding (adjusted for vgrid/hgrid mode)
	int32_t gethpadding(void) const;
	// Get the (effective) vertical padding (adjusted for vgrid/hgrid mode)
	int32_t getvpadding(void) const;
	// Get the (effective) firstIndent
	int32_t getfirstindent(void) const;
	// Get the (effective) leftIndent
	int32_t getleftindent(void) const;
	// Get the (effective) rightIndent
	int32_t getrightindent(void) const;

	// Get the (effective) listIndent
	int32_t getlistindent(void) const;
	// Get the (effective) width of any list label.
	int32_t getlistlabelwidth(void) const;

	// Fetch the (effective) tabs for the paragraph.
	void gettabs(uint16_t*& r_tabs, uint16_t& r_tab_count, Boolean& r_fixed) const;
	// Get the (effective) hGrid.
	bool gethgrid(void) const;
	// Get the (effective) vGrid.
	bool getvgrid(void) const;
	// Get the (effective) dontWrap.
	bool getdontwrap(void) const;
	// Get the (effective) table width (or zero if table mode is not on).
	int32_t gettablewidth(void) const;

	// Returns the metadata for the current paragraph.
	MCNameRef getmetadata(void) const;
	// Set the metadata property for the current paragraph.
	void setmetadata(const char *metadata);

	// Returns true if the top border will be elided due to hGrid
	bool elidetopborder(void) const;
	// Returns true if the bottom border will be elided due to hGrid
	bool elidebottomborder(void) const;

	// Computes the width of the top border.
	int32_t computetopborder(void) const;
	// Computes the width of the bottom border.
	int32_t computebottomborder(void) const;

	// Computes the maximum length of a normal and first line for the current paragraph.
	void computelayoutwidths(int32_t& r_normal, int32_t& r_first) const;
	// Computes the left margin (distance to left of text).
	int32_t computeleftmargin(void) const;
	// Computes the right margin (distance to right of text).
	int32_t computerightmargin(void) const;
	// Computes the top margin (distance to top of text).
	int32_t computetopmargin(void) const;
	// Computes the bottom margin (distance to bottom of text).
	int32_t computebottommargin(void) const;
	// Computes the paragraph x offset and width.
	void computeparaoffsetandwidth(int32_t& r_offset, int32_t& r_width) const;
	// Computes the x offset and width of the paragraph box.
	void computeboxoffsetandwidth(int32_t& r_offset, int32_t& r_width) const;
	// Computes the offset of the given line from the left margin of the field.
	int32_t computelineoffset(MCLine *line) const;
	// Computes the offset of the given line's text (taking into account lists) when fit into the given width.
	int32_t computelineinneroffset(int32_t p_layout_width, MCLine *p_line) const;
	// Computes the inner (inside border) and outer (outside border) rects for the paragraph.
	void computerects(int32_t x, int32_t y, int32_t layout_width, uint2 pg_width, uint2 pg_height, MCRectangle& r_outer, MCRectangle& r_inner) const;
	// Adjust paragraph rects to take into account fixed width table setting
	void adjustrectsfortable(MCRectangle& x_inner_rect, MCRectangle& x_outer_rect);

	// Force the paragraph to re-flow itself depending on its setting of dontWrap.
	void layout(void);
	
	// MW-2012-01-27: [[ UnicodeChunks ]] Returns the content of the field in a native
	//   form such that indices match that of the original content. If ASCII-only is
	//   set, then it preserves only ASCII unicode chars. 'p_data' must point to a
	//   at least gettextsize() bytes.
	bool nativizetext(bool p_ascii_only, char *p_data, uint32_t& x_length);
	
	// Draw the paragraph
	void draw(MCDC *dc, int2 x, int2 y,
	          uint2 fa, uint2 fd,
			  uint2 fstart, uint2 fend,
	          uint2 compstart,uint2 compend, uint1 compconvstart, uint1 compconvend,
			  uint2 textwidth, uint2 pgheight, uint2 sx, uint2 swidth,
	          uint2 pstyle);

	// Set the focus in the paragraph subject to various options...
	// Called by:
	//   MCField::setfocus
	int2 setfocus(int4 x, int4 y, uint2 fixedheight,
	              Boolean extend, Boolean extendwords, Boolean extendlines,
	              int2 direction, Boolean first, Boolean last, Boolean deselect);

	// Return the dirty region of the paragraph
	// Called by:
	//   MCField::updateparagraph
	//   MCField::seltext
	MCRectangle getdirty(uint2 fixedheight);

	// Compute the location of the cursor in the paragraph if the focused index were fi
	// Called by:
	//   MCField::replacecursor
	//   MCField::dragtext
	//   MCField::unselect
	//   MCField::fcenter
	//   MCField::fmove
	//   MCField::getcompositionrect
	MCRectangle getcursorrect(int4 fi, uint2 fixedheight, bool include_space);

	// Compute the (x, y) location of the given index in the paragraph
	// Called by:
	//   MCField::centerfound
	//   MCField::getlinkdata
	//   MCField::gettextatts
	//   MCField::returnloc
	//   MCField::insertparagraph
	void indextoloc(uint2 tindex, uint2 fixedheight, int2 &x, int2 &y);

	// Compute the left and right hand side of the range of indices (si..ei)
	// Called by:
	//   MCField::getlinkdata
	//   MCField::gettextatts
	void getxextents(int4 &si, int4 &ei, int2 &minx, int2 &maxx);

	// Compute the indices of a click at (x, y).
	// If x is outside the bounds of the line containing y then:
	//   if <chunk> then return si = ei = 0
	//   else return whole line
	// If not wholeword then return single char at index
	// If char at loc is part of a link then return link bounds
	// Return start and end of word at loc (needs more refinement)
	// Called by:
	//   MCField::locmark
	void getclickindex(int2 x, int2 y, uint2 fixedheight, uint2 &si, uint2 &ei, Boolean ww, Boolean chunk);

	// Return the attributes set on the given range. Returns true if there
	// are any set, otherwise false. In the case of some of the attrs not
	// being homogeneous, mixed is set to a bit mask indicating which ones
	// have several values.
	// Called by:
	//   MCField::finsert (for charset purposes)
	//   MCField::gettextatts
	Boolean getatts(uint2 si, uint2 ei, Font_textstyle spec_style, const char *&fname, uint2 &size,
	                uint2 &style, const MCColor *&color,
	                const MCColor *&backcolor, int2 &shift, bool& specstyle, uint2 &mixed);

	// Set the attributes on the given range.
	// Called by:
	//   MCField::finsert (for charset change purposes)
	//   MCField::texttoparagraphs
	//   MCField::htmltoparagraphs
	//   MCField::settextatts
	//   MCHcfield::buildf
	void setatts(uint2 si, uint2 ei, Properties which, void *value, bool from_html = false);

	uint2 getopened()
	{
		return opened;
	}
	MCParagraph *next()
	{
		return (MCParagraph *)MCDLlist::next();
	}
	MCParagraph *prev()
	{
		return (MCParagraph *)MCDLlist::prev();
	}
	const MCParagraph *next() const
	{
		return (const MCParagraph *)MCDLlist::next();
	}
	const MCParagraph *prev() const
	{
		return (MCParagraph *)MCDLlist::prev();
	}
	void totop(MCParagraph *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCParagraph *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCParagraph *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCParagraph *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCParagraph *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCParagraph *remove(MCParagraph *&list)
	{
		return (MCParagraph *)MCDLlist::remove((MCDLlist *&)list);
	}

	MCParagraph *copystring(uint2 si, uint2 ei);

	static uint32_t getliststylebullet(uint32_t p_list_style, bool as_unicode);
	static void formatliststyleindex(uint32_t p_list_style, uint32_t p_index, char r_index_buffer[PG_MAX_INDEX_SIZE], const char*& r_string, uint32_t& r_length);

	bool imagechanged(MCImage *p_image, bool p_deleting);

private:
	// Flow the paragraph using the given parent font. This is called
	// in many places after the text/styles have changed to reflow
	// the paragraph wrapped to the field width.
	void flow(void);

	// Flow the paragraph for a single line using the given parent font.
	void noflow(void);

	// Delete the lines computed for flow:
	//   - only called by MCParagraph
	void deletelines();

	// Remove all the style-run blocks:
	//   - only called by MCParagraph
	void deleteblocks();

	// Only called internally
	void fillselect(MCDC *dc, MCLine *lptr, int2 x, int2 y, uint2 height, int2 sx, uint2 swidth);
	void drawcomposition(MCDC *dc, MCLine *lptr, int2 x, int2 y, uint2 height, uint2 compstart, uint2 compend, uint1 compconvstart, uint1 compconvend);
	void drawfound(MCDC *dc, MCLine *lptr, int2 x, int2 y, uint2 height, uint2 fstart, uint2 fend);

	MCLine *indextoline(uint2 tindex);

	// Searches forward for the end of a link, returning the last index in si
	// and returning the block containing it.
	MCBlock *extendup(MCBlock *bptr, uint2 &si);
	
	// Searches backward for the start of a link, returning the first index in ei
	// and returning the block containing it.
	MCBlock *extenddown(MCBlock *bptr, uint2 &ei);

	int2 getx(uint2 tindex, MCLine *lptr);

	// Mark all the lines in the given range as dirty
	void marklines(uint2 si, uint2 ei);

	// Compute the x-offsets for liststyle settings
	void getliststyleoffsets(int32_t& r_label_offset, int32_t& r_content_offset);

	void computelistindex(MCParagraph *sentinal, uint32_t& r_index);
	void computeliststyleindex(MCParagraph *startpara, char r_buffer[PG_MAX_INDEX_SIZE], const char*& r_string, uint32_t& r_length);
};

#endif
