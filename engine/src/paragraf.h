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

//
// MCParagraphs within a field
//
#ifndef	PARAGRAPH_H
#define	PARAGRAPH_H

#include "dllst.h"
#include "field.h"
#include "exec.h"

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
// FG-2014-05-06: [[ TabAlignments ]] This flag is set if the paragraph has
//   tab alignments set.
#define PA_HAS_TAB_ALIGNMENTS (1 << 19)

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
	unsigned flags : 20;
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
    MCStringRef metadata;
    
    uint16_t alignments_count;
    intenum_t *alignments;

	MCParagraphAttrs(void)
	{
		memset(this, 0, sizeof(MCParagraphAttrs));
	}
};

class MCSegment;

/* MCParagraphCursorType describes the type of cursor (caret) which can be
 * requested when using 'getcursorrect'. */
enum MCParagraphCursorType
{
    /* The full cursor rect, ignoring RTL split cursors. */
    kMCParagraphCursorTypeFull,
    
    /* The primary part of an RTL cursor. */
    kMCParagraphCursorTypePrimary,
    
    /* The secondary part of an RTL cursor. */
    kMCParagraphCursorTypeSecondary,
};

// Don't change this until everything dealing with fields, paragraphs, etc
// is capable of dealing with 32-bit offsets or things will break!
#define PARAGRAPH_MAX_LEN	INT32_MAX

class MCParagraph : public MCDLlist
{
	MCField *parent;
	MCAutoStringRef m_text;
	MCBlock *blocks;
    MCSegment *segments;
	MCLine *lines;
	findex_t focusedindex;
	findex_t startindex, endindex, originalindex;
    bool moving_left, moving_forward;        // Need to know direction for BiDi support
	uint2 opened;
	uint1 state;
	// MP-2013-09-02: [[ FasterField ]] If true, it means the paragraph needs layout.
	bool needs_layout : 1;
	// MW-2012-01-25: [[ ParaStyles ]] This paragraphs collection of attrs.
	MCParagraphAttrs *attrs;
    MCTextDirection base_direction;

    static uint2 cursorwidth;
    
    // Dirty hack until we have a proper styled text object...
    friend class MCSegment;
    friend class MCLine;

public:
	MCParagraph();
	MCParagraph(const MCParagraph &pref); 
	~MCParagraph();

	//////////
	
	// This returns the codepoint at the given *unichar* index (not a codepoint
	// index). All increments and decrements should use the utility functions
	// below to ensure surrogate pairs are handled properly.
	codepoint_t GetCodepointAtIndex(findex_t p_index)
	{
		MCAssert(p_index >= 0);
		// This assumes that the input string is valid UTF-16 and all surrogate
		// pairs are matched correctly.
		unichar_t t_lead, t_tail;
		t_lead = MCStringGetCharAtIndex(*m_text, p_index);
		if (MCStringIsValidSurrogatePair(*m_text, p_index))
		{
            t_tail = MCStringGetCharAtIndex(*m_text, p_index + 1);
			return MCStringSurrogatesToCodepoint(t_lead, t_tail);
		}
		return t_lead;
	}
	
	// Increments the index pointer to the next character, accounting for
	// surrogate pairs when it does so.
	findex_t IncrementIndex(findex_t p_in)
	{
		if (p_in < 0)
			return 0;
		unichar_t t_char = MCStringGetCharAtIndex(*m_text, p_in);
        // SN-2015-09-08: [[ Bug 15895 ]] A field can end with half of a
        //  surrogate pair - in which case the index only increments by 1.
		if (0xD800 <= t_char && t_char < 0xDC00)
            return (findex_t)MCU_min((uindex_t)(p_in + 2), MCStringGetLength(*m_text));
		return p_in + 1;
	}
	
	// Decrements the index pointer to the previous character, accounting for
	// surrogate pairs when it does so.
	findex_t DecrementIndex(findex_t p_in)
	{
		if (p_in <= 0)
            return 0;
        unichar_t t_char = MCStringGetCharAtIndex(*m_text, p_in - 1);
		if (0xDC00 <= t_char && t_char < 0xE000)
			return p_in - 2;
		return p_in - 1;
	}
    
    // Scans from the given index to the next word break
    findex_t NextWord(findex_t);
    
    // Scans from the given index to the previous word break
    findex_t PrevWord(findex_t);
    
    // Scans from the given index to the next character break
    findex_t NextChar(findex_t);
    
    // Scans from the given index to the previous charcter break
    findex_t PrevChar(findex_t);
	
	// Returns true if the given character is a word break (e.g. space)
	static bool TextIsWordBreak(codepoint_t);
	
	// Returns true if the given character is a line break (e.g. '\n')
	static bool TextIsLineBreak(codepoint_t);
	
	// Returns true if the given character is a sentence break
	static bool TextIsSentenceBreak(codepoint_t);
	
	// Returns true if the given character is a paragraph break
	static bool TextIsParagraphBreak(codepoint_t);
	
	// Returns true if the given character is punctuation
	static bool TextIsPunctuation(codepoint_t);
	
	// Scans the given string for the next paragraph break character and
	// returns the index of the character following the break
	static bool TextFindNextParagraph(MCStringRef p_string, findex_t p_after, findex_t &r_next_para);
	
	// Creates a new block at the end of this paragraph
	MCBlock* AppendText(MCStringRef p_text);
	
	// Returns true if the paragraph is empty
	bool IsEmpty()
	{
		return gettextlength() == 0;
	}
	
	// Returns the mutable stringref that is used internally by hte paragraph.
	// Note that this string ref is volatile across any calls that modify the
	// paragraph in any way and should not be retained.
	MCStringRef GetInternalStringRef() const
	{
		return *m_text;
	}
	
	////////// BIDIRECTIONAL SUPPORT
    
    MCTextDirection getbasetextdirection() const
    {
        if (base_direction == kMCTextDirectionAuto)
            return parent->getbasetextdirection();
        return base_direction;
    }
    
    void SetBlockDirectionLevel(findex_t si, findex_t ei, uint8_t level);
    
    void resolvetextdirections();
    
    uint8_t firststrongisolate(uindex_t p_offset) const;
    
    //////////
	
	bool visit(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor* p_visitor);

	uint32_t getminimumstackfileversion(void);
	
	// MW-2012-03-04: [[ StackFile5500 ]] If 'is_ext' is true then this paragraph
	//   has an attribute extension.
	IO_stat load(IO_handle stream, uint32_t version, bool is_ext);
	IO_stat save(IO_handle stream, uint4 p_part, uint32_t p_version);
	
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

	// Copies the current contents of the paragraph
	bool copytextasstringref(MCStringRef& r_string);

	// Iterate backwards starting at p_index and return the first index in the
	// paragraph before which a word-break can occur.
	findex_t findwordbreakbefore(MCBlock *p_block, findex_t p_index);

	// Iterate forwards starting at p_index and return the first index in the
	// paragraph after which a word-break can occur.
	findex_t findwordbreakafter(MCBlock *p_block, findex_t p_index);

	// Remove all empty blocks in the paragraph starting at
	// the specified block (inclusive).
	Boolean clearzeros(MCBlock *start_from = nil);
	
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
    
    // Return the list of segments in the paragraph, if any.
	MCSegment *getsegments(void) const
	{
		return segments;
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
	MCBlock *indextoblock(findex_t tindex, Boolean forinsert, bool for_navigation = false);

	// Join this paragraph with the next paragraph or split the current
	// paragraph at the focused index.
	// Called by:
	//   MCField::joinparagraphs
	//   MCField::settextindex
	//   MCField::gettextatts (to fetch partial parts of a paragraph)
	//   MCField::settextatts (to insert partial parts of a paragraph)
	//   MCField::insertparagraph
	// MW-2014-05-28: [[ Bug 12303 ]] If 'preserve' is true, then the paragraph styles
    //   of 'this' paragraph are never changed (used when setting 'text' of a chunk).
	void join(bool p_preserve_styles_if_zero_length = false);
    void split();
    void split(findex_t p_position);

    // Replace the text from p_start to p_finish
    // with the paragraphs list provided.
    // Called by:
    //   MCField::SetRtfTextOfCharChunk
    //   MCField::SetHtmlTextOfCharChunk
    //   MCField::SetStyledTextOfCharChunk
    void replacetextwithparagraphs(findex_t p_start, findex_t p_finish, MCParagraph *p_pglist);

	// Delete the text from si to ei in the paragraph.
	// The 'styling_mode' determines what block is left at si.
	// If it is 'frombefore' then no block is left so any text inserted at si will
	// take styles from the preceeding block.
	// If it is 'fromafter' then a zero length block with the same style as the first
	// char in the deleted string is left so any text inserted at si will take
	// styles from the first char in the deleted string.
	// If it is 'none' then a zero length block with no styles is left so any text
	// inserted at si will have no style.
	void deletestring(findex_t si, findex_t ei, MCFieldStylingMode p_preserve_first_style = kMCFieldStylingFromBefore);

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
	void finsertnobreak(MCStringRef p_string, MCRange p_range);
    Boolean finsertnew(MCStringRef p_string);

	// Delete the specified chunk at the focusedindex and return the deleted
	// string in undopgptr.
	int2 fdelete(Field_translations type, MCParagraph *&undopgptr);

	// Move the focused index to the place specified by type.
	// Called by:
	//   MCField::fmove
	uint1 fmovefocus(Field_translations type, bool force_logical = false);
    uint1 fmovefocus_visual(Field_translations type);

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
    // SN-2014-04-04 [[ CombiningChars ]] We want to be able to get the numbers of actual characters of a paragraph
    // not the numbers of codeunits.
	findex_t gettextlength(bool p_char_indices = false)
	{
		if (blocks == NULL)
			inittext();
			
        if (p_char_indices)
        {
            MCRange t_cu_range = {0,MCStringGetLength(*m_text)};
            MCRange t_char_range;
            MCStringUnmapIndices(*m_text, kMCCharChunkTypeGrapheme, t_cu_range, t_char_range);
            return t_char_range . length;
        }
        else
            return MCStringGetLength(*m_text);
	}

	// Same as gettextsize, except adjust by one for the CR character.
	findex_t gettextlengthcr(bool p_char_indices = false)
	{
		return gettextlength(p_char_indices) + 1;
	}

	// 'gettext()' returns a direct pointer to the backstore
	// Called by:
	//   MCField::getparagraphtext
	//   MCField::getparagraphtextrtf
	//   MCField::sort
	//   MCField::find
	//   MCField::gettext
	//   MCField::selectedtext
	//   MCIdeScriptColorize::exec
	//const char *gettext_raw();

	// Return the text as HTML formatted string.
	// Called by:
	//   MCField::gethtmltext
	// Clear everything in the current paragraph and set the text to the
	// given string.
	// Called by:
	//   MCField::settext
	//   MCHctext::settext
	void settext(MCStringRef p_string);

	// Set the backing store to the given text and length, then adjust
	// the final block (creating one if necessary) so it finishes at the
	// end of the paragraph.
	// Called by:
	//   MCField::texttoparagraphs
	//   MCField::htmltoparagraphs
	void resettext(MCStringRef p_string);

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
	void getselectionindex(findex_t &si, findex_t &ei);

	// Set the selected range of the paragraph
	// Called by:
	//   MCField::undo (front == back == False)
	//   MCField::unselect (removes selection)
	//   MCField::freturn (removes selection)
	//   MCField::settextindex (removes selection, sets to a single index)
	//   MCField::gettextatts (used to mark for split/join)
	//   MCField::settextatts (used to mark for split/join)
	//   MCField::seltext (removes selection, selects range/front/back)
	void setselectionindex(findex_t si, findex_t ei, Boolean front, Boolean back);

	// Reverses the 'originalindex' position in the paragraph
	// Called by:
	//   MCField::reverse
	void reverseselection();

	// Computes the height of the field (using fixedheight if non-zero) up
	// to and including the line containing tindex.
	// Called by:
	//   MCField::getlinkdata
	//   MCField::gettextatts
	uint2 getyextent(findex_t tindex, uint2 fixedheight);

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
	MCStringRef getlinktext(findex_t si);

	// Get the image source (if any) in the block contaning index si
	// Called by:
	//   MCField::gettextatts
	MCStringRef getimagesource(findex_t si);

	// Get the metadata (if any) in the block containing index si
	MCStringRef getmetadataatindex(findex_t si);

	// Return true if the link (if any) in the block containing index si
	// has been visited.
	// Called by:
	//   MCField::gettextatts
	Boolean getvisited(findex_t si);

	// Set the visited status of the blocks between si and ei to v.
	// Called by:
	//   MCField::settextatts
	void setvisited(findex_t si, findex_t ei, Boolean v);
	
	// Returns the state of a given block flag in the specified range.
	bool getflagstate(uint32_t flag, findex_t si, findex_t ei, bool& r_state);

	// MW-2012-02-08: [[ FlaggedRanges ]] Get the ranges of indices which have the
	//   flagged status set to true - these are then adjusted by delta.
	// MW-2012-02-24: [[ FieldChars ]] Pass in the part_id so the paragraph can map
	//   field indices to char indices.
    // MW-2013-07-31: [[ Bug 10957 ]] Pass in the start of the paragraph as a byte
	//   offset so that the correct char offset can be calculated.
    void getflaggedranges(uint32_t p_part_id, findex_t si, findex_t ei, int32_t p_delta, MCInterfaceFieldRanges& r_ranges);
    
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
    // MW-2014-04-11: [[ Bug 12182 ]] Make sure we use uint4 for field indicies.
    Boolean pagerange(uint2 fixedheight, uint2 &theight, uint4 &tend, MCLine *&lastline);

	// Returns true if any of the paragraph attributes are non-default.
	bool hasattrs(void);
    
    // Clear attrs if they are not needed
	void cleanattrs(void);
    
	// Sets the given paragraph attribute to the value in ep.
	// Gets the given paragraph attribute into the given ep.
	// Copies the given attribute from the given paragraph.
	void copysingleattr(Properties which, MCParagraph *other);
	// Copies all the attributes from the given paragraph.
	void copyattrs(const MCParagraph& other);
	// Fetches the paragraph attributes from the src array.
    void fetchattrs(MCArrayRef src);
	// Clears the paragraph attributes.
	void clearattrs(void);
	// Unserializes the paragraph attributes from stream.
	IO_stat loadattrs(IO_handle stream, uint32_t version);
	// Serializes the paragraph attributes into stream.
	IO_stat saveattrs(IO_handle stream, uint32_t p_version);
	// MW-2012-02-21: [[ FieldExport ]] Fills in the appropriate members of the
	//   field export struct.
	void exportattrs(MCFieldParagraphStyle& x_style);
	// MW-2012-03-04: [[ FieldImport ]] Set the attributes of the paragraph to those described
	//  by the style.
	void importattrs(const MCFieldParagraphStyle& x_style);
	// MW-2012-03-03: [[ StackFile5500 ]] Computes the size of the attrs when serialized.
	uint32_t measureattrs(uint32_t p_version);
    // SN-2015-05-01: [[ Bug 15175 ]] Make easier to find out whether we need an extra flag
    bool hasextraflag(void);

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
    void gettabaligns(intenum_t*& r_tabaligns, uint16_t& r_tab_count) const;
	// Get the (effective) hGrid.
	bool gethgrid(void) const;
	// Get the (effective) vGrid.
	bool getvgrid(void) const;
	// Get the (effective) dontWrap.
	bool getdontwrap(void) const;
	// Get the (effective) table width (or zero if table mode is not on).
	int32_t gettablewidth(void) const;

	// Returns the metadata for the current paragraph.
    MCStringRef getmetadata(void) const;
	// Set the metadata property for the current paragraph.
    void setmetadata(MCStringRef metadata);

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
    // AL-2014-09-22: [[ Bug 11817 ]] If p_check_redraw is true, returns true if the number of lines changes under this new layout
	bool layout(bool p_force, bool p_check_redraw = false);
    uindex_t countlines();
	
	// Draw the paragraph
	void draw(MCDC *dc, int2 x, int2 y,
	          uint2 fa, uint2 fd,
			  findex_t fstart, findex_t fend,
	          findex_t compstart, findex_t compend, findex_t compconvstart, findex_t compconvend,
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
	MCRectangle getcursorrect(findex_t fi, uint2 fixedheight, bool include_space, MCParagraphCursorType type = kMCParagraphCursorTypeFull);

	// Compute the (x, y) location of the given index in the paragraph
	// Called by:
	//   MCField::centerfound
	//   MCField::getlinkdata
	//   MCField::gettextatts
	//   MCField::returnloc
	//   MCField::insertparagraph
	void indextoloc(findex_t tindex, uint2 fixedheight, coord_t &x, coord_t &y);

	// Compute the left and right hand side of the range of indices (si..ei)
	// Called by:
	//   MCField::getlinkdata
	//   MCField::gettextatts
	void getxextents(findex_t &si, findex_t &ei, coord_t &minx, coord_t &maxx);

	// Compute the indices of a click at (x, y).
	// If x is outside the bounds of the line containing y then:
	//   if <chunk> then return si = ei = 0
	//   else return whole line
	// If not wholeword then return single char at index
	// If char at loc is part of a link then return link bounds
	// Return start and end of word at loc (needs more refinement)
	// Called by:
	//   MCField::locmark
	void getclickindex(int2 x, int2 y, uint2 fixedheight, findex_t &si, findex_t &ei, Boolean ww, Boolean chunk);

	// Return the attributes set on the given range. Returns true if there
	// are any set, otherwise false. In the case of some of the attrs not
	// being homogeneous, mixed is set to a bit mask indicating which ones
	// have several values.
	// Called by:
	//   MCField::finsert (for charset purposes)
	//   MCField::gettextatts
	// Set the attributes on the given range.
	// Called by:
	//   MCField::finsert (for charset change purposes)
	//   MCField::texttoparagraphs
	//   MCField::htmltoparagraphs
	//   MCField::settextatts
	//   MCHcfield::buildf
	void restricttoline(findex_t& si, findex_t& ei);
	uint2 heightoflinewithindex(findex_t si, uint2 fixedheight);
	
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

	MCParagraph *copystring(findex_t si, findex_t ei);

	// TODO: this should probably return a StringRef or a codepoint...
	static uint32_t getliststylebullet(uint32_t p_list_style, bool as_unicode);
	
	static void formatliststyleindex(uint32_t p_list_style, uint32_t p_index, char r_index_buffer[PG_MAX_INDEX_SIZE], const char*& r_string, uint32_t& r_length);

	bool imagechanged(MCImage *p_image, bool p_deleting);

    MCBlock* extendup(MCBlock *bptr, findex_t &si);
    MCBlock* extenddown(MCBlock *bptr, findex_t &ei);

    // Set the flag Lines not synched
    void setDirty() { state |= PS_LINES_NOT_SYNCHED; }

    void layoutchanged() { needs_layout = true; }
    bool getneedslayout() { return needs_layout; }
    
    //////////

    void GetEncoding(MCExecContext &ctxt, intenum_t& r_encoding);

    void GetTextAlign(MCExecContext& ctxt, intenum_t*& r_value);
    void GetEffectiveTextAlign(MCExecContext& ctxt, intenum_t& r_value);
    void SetTextAlign(MCExecContext& ctxt, intenum_t *p_value);

    void GetListStyle(MCExecContext& ctxt, intenum_t &r_style);
    void SetListStyle(MCExecContext& ctxt, intenum_t p_style);

    void GetListDepth(MCExecContext& ctxt, uinteger_t*& r_depth);
    void GetEffectiveListDepth(MCExecContext& ctxt, uinteger_t& r_depth);
    void SetListDepth(MCExecContext &ctxt, uinteger_t *p_depth);

    void GetListIndent(MCExecContext& ctxt, integer_t *&r_indent);
    void GetEffectiveListIndent(MCExecContext& ctxt, integer_t& r_indent);
    void SetListIndent(MCExecContext& ctxt, integer_t *p_indent);

    void GetListIndex(MCExecContext& ctxt, uinteger_t*& r_list_index);
    void GetEffectiveListIndex(MCExecContext& ctxt, uinteger_t& r_list_index);
    void SetListIndex(MCExecContext& ctxt, uinteger_t *p_list_index);

    void GetFirstIndent(MCExecContext& ctxt, integer_t*& r_indent);
    void GetEffectiveFirstIndent(MCExecContext& ctxt, integer_t& r_indent);
    void SetFirstIndent(MCExecContext& ctxt, integer_t *p_indent);

    void GetLeftIndent(MCExecContext& ctxt, integer_t*& r_indent);
    void GetEffectiveLeftIndent(MCExecContext& ctxt, integer_t& r_indent);
    void SetLeftIndent(MCExecContext& ctxt, integer_t *p_indent);

    void GetRightIndent(MCExecContext& ctxt, integer_t*& r_indent);
    void GetEffectiveRightIndent(MCExecContext& ctxt, integer_t& r_indent);
    void SetRightIndent(MCExecContext& ctxt, integer_t *p_indent);

    void GetSpaceAbove(MCExecContext& ctxt, uinteger_t *&r_space);
    void GetEffectiveSpaceAbove(MCExecContext& ctxt, uinteger_t& r_space);
    void SetSpaceAbove(MCExecContext& ctxt, uinteger_t *p_space);

    void GetSpaceBelow(MCExecContext& ctxt, uinteger_t*& r_space);
    void GetEffectiveSpaceBelow(MCExecContext& ctxt, uinteger_t& r_space);
    void SetSpaceBelow(MCExecContext& ctxt, uinteger_t *p_space);

    void DoSetTabStops(MCExecContext &ctxt, bool p_is_relative, const vector_t<uinteger_t> &p_tabs);
    void DoGetTabStops(MCExecContext &ctxt, bool p_is_relative, vector_t<uinteger_t> &r_tabs);

    void GetTabWidths(MCExecContext& ctxt, vector_t<uinteger_t> &r_tabs);
    void GetEffectiveTabWidths(MCExecContext& ctxt, vector_t<uinteger_t> &r_tabs);
    void SetTabWidths(MCExecContext& ctxt, const vector_t<uinteger_t> &p_tabs);

    void GetTabStops(MCExecContext& ctxt, vector_t<uinteger_t> &r_tabs);
    void GetEffectiveTabStops(MCExecContext& ctxt, vector_t<uinteger_t> &r_tabs);
    void SetTabStops(MCExecContext& ctxt, const vector_t<uinteger_t>& p_tabs);

    void GetTabAlignments(MCExecContext& ctxt, MCInterfaceFieldTabAlignments &r_alignments);
    void GetEffectiveTabAlignments(MCExecContext& ctxt, MCInterfaceFieldTabAlignments &r_alignments);
    void SetTabAlignments(MCExecContext& ctxt, const MCInterfaceFieldTabAlignments &p_alignments);
    
    void GetBackColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color);
    void GetEffectiveBackColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color);
    void SetBackColor(MCExecContext& ctxt, const MCInterfaceNamedColor &p_color);

    void GetBorderColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
    void GetEffectiveBorderColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color);
    void SetBorderColor(MCExecContext& ctxt, const MCInterfaceNamedColor &p_color);

    void GetBorderWidth(MCExecContext& ctxt, uinteger_t*& r_width);
    void GetEffectiveBorderWidth(MCExecContext& ctxt, uinteger_t& r_width);
    void SetBorderWidth(MCExecContext& ctxt, uinteger_t *p_width);

    void GetPadding(MCExecContext& ctxt, uinteger_t*& r_padding);
    void GetEffectivePadding(MCExecContext& ctxt, uinteger_t& r_padding);
    void SetPadding(MCExecContext& ctxt, uinteger_t *p_padding);

    void GetHGrid(MCExecContext& ctxt, bool*& r_has_hgrid);
    void GetEffectiveHGrid(MCExecContext& ctxt, bool& r_has_hgrid);
    void SetHGrid(MCExecContext& ctxt, bool *p_has_hgrid);

    void GetVGrid(MCExecContext& ctxt, bool*& r_has_vgrid);
    void GetEffectiveVGrid(MCExecContext& ctxt, bool& r_has_vgrid);
    void SetVGrid(MCExecContext& ctxt, bool *p_has_vrid);

    void GetDontWrap(MCExecContext& ctxt, bool*& r_dont_wrap);
    void GetEffectiveDontWrap(MCExecContext& ctxt, bool& r_dont_wrap);
    void SetDontWrap(MCExecContext& ctxt, bool *p_dont_wrap);

    void GetInvisible(MCExecContext& ctxt, bool *&r_invisible);
    void GetEffectiveInvisible(MCExecContext& ctxt, bool &r_invisible);
    void SetInvisible(MCExecContext& ctxt, bool p_invisible);

    void GetMetadata(MCExecContext& ctxt, MCStringRef &r_metadata);
    void GetEffectiveMetadata(MCExecContext& ctxt, MCStringRef &r_metadata);
    void SetMetadata(MCExecContext& ctxt, MCStringRef p_metadata);


    //////////////////////////////////////////////////
    // IDE-related functions

    void SetForeColorOfCharChunk(MCExecContext &ctxt, findex_t si, findex_t ei, const MCInterfaceNamedColor &p_color);
    void SetTextStyleOfCharChunk(MCExecContext &ctxt, findex_t si, findex_t ei, const MCInterfaceTextStyle &p_text);
    void SetTextFontOfCharChunk(MCExecContext &ctxt, findex_t si, findex_t ei, MCStringRef p_fontname);
    void SetTextSizeOfCharChunk(MCExecContext &ctxt, findex_t si, findex_t ei, uinteger_t *p_size);

private:
	// Flow the paragraph using the given parent font. This is called
	// in many places after the text/styles have changed to reflow
	// the paragraph wrapped to the field width.
	void flow(void);

	// Flow the paragraph for a single line using the given parent font.
	void noflow(void);

	// Delete the lines and segments computed for flow:
	//   - only called by MCParagraph
	void deletelines();

	// Remove all the style-run blocks:
	//   - only called by MCParagraph
	void deleteblocks();

	// Only called internally
	void fillselect(MCDC *dc, MCLine *lptr, int2 x, int2 y, uint2 height, int2 sx, uint2 swidth);
	void drawcomposition(MCDC *dc, MCLine *lptr, int2 x, int2 y, uint2 height, findex_t compstart, findex_t compend, findex_t compconvstart, findex_t compconvend);
	void drawfound(MCDC *dc, MCLine *lptr, int2 x, int2 y, uint2 height, findex_t fstart, findex_t fend);

	MCLine *indextoline(findex_t tindex);

	coord_t getx(findex_t tindex, MCLine *lptr);

	// Mark all the lines in the given range as dirty
	void marklines(findex_t si, findex_t ei);

	// Compute the x-offsets for liststyle settings
	void getliststyleoffsets(int32_t& r_label_offset, int32_t& r_content_offset);

	void computelistindex(MCParagraph *sentinal, uint32_t& r_index);
	void computeliststyleindex(MCParagraph *startpara, char r_buffer[PG_MAX_INDEX_SIZE], const char*& r_string, uint32_t& r_length);
};

#endif
