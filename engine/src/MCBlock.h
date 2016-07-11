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

#ifndef	BLOCK_H
#define	BLOCK_H

#include "dllst.h"
#include "field.h"

class MCParagraph;
class MCField;
class MCLine;
class MCSegment;
struct MCFieldCharacterStyle;

typedef struct Blockatts
{
	MCColor *color;
	MCColor *backcolor;
	// MW-2012-05-04: [[ Values ]] linkText, imageSource and metaData are all
	//   better represented as uniqued strings, rather than names.
	MCStringRef linktext;
	MCStringRef imagesource;
	MCStringRef metadata;
	MCImage *image;
	// MW-2012-01-06: [[ Block Changes ]] These are the x, y location of the
	//   image source (previously unioned with colornames).
	int2 x, y;
	// MW-2012-02-14: [[ FontRefs ]] The block's textFont attr is now stored as
	//   a name.
	MCNameRef fontname;
	uint2 fontsize;
	uint2 fontstyle;
	int2 shift;

	// MW-2006-04-21: [[ Purify ]] Ensure this is initialised to empty
	Blockatts(void)
	{
		memset(this, 0, sizeof(Blockatts));
	}
}
Blockatts;

class MCBlock : public MCDLlist
{

protected:
	MCParagraph *parent;
	uint4 flags;
	Blockatts *atts;
	findex_t m_index, m_size;
	coord_t width;
    coord_t origin;
	uint2 opened;
    coord_t tabpos;         // Pixel offset to use when calculating tabstops
    uint2 visual_index;     // Visual ordering index from left to right
    uint8_t direction_level;

    // Store pointer to containing segment for convenience
    MCSegment *segment;
	
    // MW-2012-02-14: [[ FontRefs ]] The concrete font to use for the block.
	//   (only valid when the block is open).
	MCFontRef m_font;
public:
	MCBlock();
	MCBlock(const MCBlock &bref);
	~MCBlock();

	bool visit(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor* p_visitor);

	// MCBlock functions
	void copy(MCBlock *bptr);
	
	// IM-2016-07-06: [[ Bug 17690 ]] Test if block sizes or offsets require 32bit
	//   values to store (stack file format v8.1).
	uint32_t getminimumstackfileversion(void);
	
	// MW-2012-03-04: [[ StackFile5500 ]] If 'is_ext' is true then this block has
	//   an extension style attribute section.
	IO_stat load(IO_handle stream, uint32_t version, bool is_ext);
	IO_stat save(IO_handle stream, uint4 p_part, uint32_t p_version);

	// MW-2012-02-14: [[ FontRefs ]] To open a block, we need the parent's fontref.
	void open(MCFontRef parent_font);
	void close();

	// MW-2012-02-14: [[ FontRefs ]] Compute a concrete font for the block using the given
	//   parent font.
	void mapfont(MCFontRef p_parent_font);
	// MW-2012-02-14: [[ FontRefs ]] Free the block's concrete font.
	void unmapfont(void);
	// MW-2012-02-14: [[ FontRefs ]] Recompute the concrete font for the block using the
	//   given parent font. Returns 'true' if something changed.
	bool recomputefonts(MCFontRef p_parent_font);

	// MW-2012-02-06: The 'persistent_only' param determines whether things like
	//   'flagged' are excluded from the sameness check (used by styledText).
	Boolean sameatts(MCBlock *bptr, bool p_persistent_only);

	bool fit(coord_t x, coord_t width, findex_t& r_break_index, bool& r_break_fits);
	void split(findex_t p_index);
    // SN-2014-08-13: [[ Bug 13016 ]] Added a parameter for the left of the cell
	void drawstring(MCDC *dc, coord_t x, coord_t lx, coord_t cx, int2 y, findex_t start, findex_t length, Boolean image, uint32_t style);
	
	// MW-2012-02-27: [[ Bug 2939 ]] The 'flags' parameter indicates whether the left and/or
	//   right edge of any box or 3d-box should be rendered.
    // SN-2014-08-13: [[ Bug 13016 ]] Added a parameter for the left of the cell
	void draw(MCDC *dc, coord_t x, coord_t p_left_cell, coord_t cx, int2 y, findex_t si, findex_t ei, MCStringRef p_text, uint2 pstyle, uint32_t flags);

	// MW-2012-02-17: [[ SplitTextAttrs ]] Returns the effective font attrs of the block.
	//   If 'base_attrs' is non-nil, it uses that to derive the attrs.
	void getfontattrs(MCObjectFontAttrs *base_attrs, MCNameRef& fname, uint2& fsize, uint2& fstyle);

	// MW-2012-02-17: [[ SplitTextAttrs ]] Fetches the textFont property of the block, or returns
	//   false if none is set.
	bool gettextfont(MCNameRef& r_textfont) const;
	// MW-2012-02-17: [[ SplitTextAttrs ]] Fetches the textFont property of the block, or returns
	//   false if none is set.
	bool gettextfont(const char*& r_textfont) const;
	// MW-2012-02-17: [[ SplitTextAttrs ]] Fetches the textSize property of the block, or returns
	//   false if none is set.
	bool gettextsize(uint2& r_textsize) const;
	// MW-2012-02-17: [[ SplitTextAttrs ]] Fetches the textStyle property of the block, or returns
	//   false if none is set.
	bool gettextstyle(uint2& r_textstyle) const;

	// MW-2012-02-17: [[ SplitTextAttrs ]] Returns true if the block has any font attrs set, or
	//   false otherwise.
	bool hasfontattrs(void) const;

	// MW-2012-02-21: [[ FieldExport ]] Update the fields in the field style export struct to
	//   reflect changes made by this block.
	void exportattrs(MCFieldCharacterStyle& x_syle);
	// MW-2012-03-04: [[ FieldImport ]] Set the attributes of the block to those described by
	//  the style.
	void importattrs(const MCFieldCharacterStyle& x_style);
	
	// MW-2012-03-04: [[ StackFile5500 ]] Measure the size of the serialized attributes.
	uint32_t measureattrs(uint32_t p_version);

	Boolean getshift(int2 &out);
	void setshift(int2 in);
	Boolean getcolor(const MCColor *&color);
	Boolean getbackcolor(const MCColor *&color);
	void setcolor(const MCColor *color);
	void setbackcolor(const MCColor *color);
	MCParagraph *getparent()
	{
		return parent;
	}
	void setparent(MCParagraph *pgptr)
	{
		parent = pgptr;
	}

	coord_t getsubwidth(MCDC *dc, coord_t x, findex_t i, findex_t l);
	coord_t getwidth(MCDC *dc, coord_t x);
	void reset();
    coord_t GetAscent() const;
    coord_t GetDescent() const;
    coord_t GetLeading() const;
	void freeatts();
	void freerefs();
	void openimage();
	void closeimage();

	// MW-2012-05-04: [[ Values ]] linkText / imageSource / metaData are now uniqued
	//   strings.
	MCStringRef getlinktext();
	MCStringRef getimagesource();
	MCStringRef getmetadata(void);

	void sethilite(Boolean on);
	
	// MW-2012-01-26: [[ FlaggedField ]] Returns whether the given block has the
	//   'flagged' status set.
	bool getflagged(void) const
	{
		return (flags & F_FLAGGED) != 0;
	}
	
	Boolean gethilite() const
	{
		return (flags & F_HILITED) != 0;
	}
	Boolean getvisited() const
	{
		return (flags & F_VISITED) != 0;
	}
	void setvisited()
	{
		flags |= F_VISITED;
	}
	void clearvisited()
	{
		flags &= ~F_VISITED;
	}

	Boolean islink() const
	{
		// MW-2012-02-17: [[ SplitTextAttrs ]] We are a link if we have a font
		//   style and the FA_LINK attr is set.
		return flags & F_HAS_FSTYLE
		       && atts != NULL && (atts->fontstyle & FA_LINK) != 0;
	}

	bool hasatts() const
	{
		// MW-2012-02-17: [[ SplitTextAttrs ]] We have atts if one of the flags is
		//   set.
		return (flags & F_HAS_ATTS) != 0;
	}
    
    void cleanatts()
	{
        // MW-2012-02-17: [[ SplitTextAttrs ]] If we no longer have any atts, delete the struct.
        if ((flags & F_HAS_ATTS) == 0)
        {
            delete atts;
            atts = nil;
        }
	}

	bool getflag(unsigned int f) const
	{
		return (flags & f) != 0;
	}

	MCBlock *next()
	{
		return (MCBlock *)MCDLlist::next();
	}
	MCBlock *prev()
	{
		return (MCBlock *)MCDLlist::prev();
	}
	void totop(MCBlock *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCBlock *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCBlock *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCBlock *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCBlock *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCBlock *remove(MCBlock *&list)
	{
		return (MCBlock *)MCDLlist::remove((MCDLlist *&)list);
	}

	bool imagechanged(MCImage *p_image, bool p_deleting);
    
    ////////// BIDIRECTIONAL SUPPORT
    
    coord_t getorigin() const
    {
        return origin;
    }
    
    void setorigin(coord_t o)
    {
        origin = o;
    }
    
    uint2 GetVisualIndex() const
    {
        return visual_index;
    }
    
    void SetVisualIndex(uint2 i)
    {
        visual_index = i;
    }
    
    uint2 GetDirectionLevel() const
    {
        return direction_level;
    }
    
    void SetDirectionLevel(uint8_t l)
    {
        direction_level = l;
    }
    
    bool is_rtl() const
    {
        // If odd, text is right-to-left, otherwise left-to-right
        return GetDirectionLevel() & 1;
    }
    
    coord_t getwidth(MCDC *dc = NULL)
    {
        if (is_rtl())
            return getwidth(dc, origin - width);
        else
            return getwidth(dc, origin);
    }
    
    void settabpos(coord_t offset)
    {
        tabpos = offset;
    }
    
    // Returns the next/previous block in visual order (or nil if none)
    MCBlock *GetNextBlockVisualOrder();
    MCBlock *GetPrevBlockVisualOrder();
	
    void SetSegment(MCSegment *p_segment)
    {
        segment = p_segment;
    }
    
    MCSegment *GetSegment(void)
    {
        return segment;
    }
    
	////////////////////
	
	// Returns only the "index" component of the range
	findex_t GetOffset();
	
	// Returns only the "length" component of the range
	findex_t GetLength();
	
	// Returns the range of text covered by the block
	void GetRange(findex_t &r_index, findex_t &r_length);
	
	// Sets the indices correctly using the parent paragraph's stringref
	void SetRange(findex_t t_index, findex_t t_length);
	
	// Moves the index by the specified number of character positions
	void MoveRange(findex_t t_index_offset, findex_t t_length_offset); 
	
	// Translates from a pixel position to a cursor index
	findex_t GetCursorIndex(coord_t x, Boolean chunk, Boolean last, bool moving_forward);
	
	// Returns the x coordinate of the cursor
	coord_t GetCursorX(findex_t fi);
	
	// Moves the index forwards by one codepoint, possibly changing block
	MCBlock *AdvanceIndex(findex_t &x_index);
	
	// Moves the index backwards by one codepoint, possibly changing block
	MCBlock *RetreatIndex(findex_t &x_index);
	
	// Returns the codepoint at the given index into the block
	codepoint_t GetCodepointAtIndex(findex_t p_index) const;
	
	// Finds the first linebreak character in the block
	bool GetFirstLineBreak(findex_t &r_index);
	
	// Returns true if the block's text was stored in Unicode format (required
	// when reading the existing (non-Unicode) stack file format)
	bool IsSavedAsUnicode() const
	{
		return flags & F_HAS_UNICODE;
	}
    
    // Returns true if the block has a trailing tab character.
    bool HasTrailingTab(void) const
    {
        return m_size != 0 && GetCodepointAtIndex(m_size - 1) == '\t';
    }
    
    //////////

    void GetLinkText(MCExecContext& ctxt, MCStringRef& r_linktext);
    void SetLinktext(MCExecContext& ctxt, MCStringRef p_linktext);
    void GetMetadata(MCExecContext& ctxt, MCStringRef& r_metadata);
    void SetMetadata(MCExecContext& ctxt, MCStringRef p_metadata);
    void GetImageSource(MCExecContext& ctxt, MCStringRef &r_image_source);
    void SetImageSource(MCExecContext& ctxt, MCStringRef p_image_source);
    void GetVisited(MCExecContext& ctxt, bool& r_value);
	// PM-2015-07-06: [[ Bug 15577 ]] "visited" property should be RW
	void SetVisited(MCExecContext& ctxt, bool p_value);
    void GetFlagged(MCExecContext& ctxt, bool &r_value);
    void SetFlagged(MCExecContext& ctxt, bool p_value);

    void GetTextFont(MCExecContext& ctxt, MCStringRef &r_fontname);
    void SetTextFont(MCExecContext& ctxt, MCStringRef p_fontname);
    void GetTextStyle(MCExecContext& ctxt, MCInterfaceTextStyle &r_style);
    void SetTextStyle(MCExecContext& ctxt, const MCInterfaceTextStyle& p_style);
    void GetTextSize(MCExecContext& ctxt, uinteger_t*& r_size);
    void SetTextSize(MCExecContext& ctxt, uinteger_t* p_size);
    void GetTextShift(MCExecContext& ctxt, integer_t*& r_shift);
    void GetEffectiveTextShift(MCExecContext& ctxt, integer_t& r_shift);
    void SetTextShift(MCExecContext& ctxt, integer_t* p_shift);

    void GetForeColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color);
    void GetEffectiveForeColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color);
    void SetForeColor(MCExecContext& ctxt, const MCInterfaceNamedColor &p_color);
    void GetBackColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color);
    void SetBackColor(MCExecContext& ctxt, const MCInterfaceNamedColor &p_color);

    void GetTextStyleElement(MCExecContext& ctxt, MCNameRef p_index, bool*& r_value);
    void SetTextStyleElement(MCExecContext& ctxt, MCNameRef p_index, bool p_value);

    //////////

    // FG-2014-11-11: [[ Better theming ]]
    // Sets up the colours on the DC for the given type of drawing
    void setcolorfornormaltext(MCDC*, MCColor*);
    void setcolorforhilite(MCDC*);
    void setcolorforselectedtext(MCDC*, MCColor*);
};
#endif
