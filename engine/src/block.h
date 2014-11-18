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

#ifndef	BLOCK_H
#define	BLOCK_H

#include "dllst.h"

class MCParagraph;
class MCField;
class MCLine;
struct MCFieldCharacterStyle;

typedef struct Blockatts
{
	MCColor *color;
	MCColor *backcolor;
	// MW-2012-01-06: [[ Block Changes ]] Change the linktext and imagesource
	//   strings to be interred names.
	MCNameRef linktext;
	MCNameRef imagesource;
	// MW-2012-01-06: [[ Block Metadata ]] An arbitrary (text) string that can
	//   be attached to a run of text.
	MCNameRef metadata;
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
	friend class MCField;
	friend class MCParagraph;

protected:
	MCParagraph *parent;
	uint4 flags;
	Blockatts *atts;
	uint2 index, size;
	coord_t width;
	uint2 opened;

	// MW-2012-02-14: [[ FontRefs ]] The concrete font to use for the block.
	//   (only valid when the block is open).
	MCFontRef m_font;
public:
	MCBlock();
	MCBlock(const MCBlock &bref);
	~MCBlock();

	bool visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor *p_visitor);

	// MCBlock functions
	void copy(MCBlock *bptr);
	
	// MW-2012-03-04: [[ StackFile5500 ]] If 'is_ext' is true then this block has
	//   an extension style attribute section.
	IO_stat load(IO_handle stream, const char *version, bool is_ext);
	IO_stat save(IO_handle stream, uint4 p_part);

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
	Boolean breakat(int2 x, uint2 width, Boolean first,
	                MCFontStruct *pfont, Boolean &broken);
	bool fit(coord_t x, coord_t width, uint2& r_break_index, bool& r_break_fits);
	void split(uint2 p_index);
	coord_t gettabwidth(coord_t x, const char *text, uint2 index);
	void drawstring(MCDC *dc, coord_t x, coord_t cx, int2 y, uint2 start, uint2 length, Boolean image, uint32_t style);
	
	// MW-2012-02-27: [[ Bug 2939 ]] The 'flags' parameter indicates whether the left and/or
	//   right edge of any box or 3d-box should be rendered.
	void draw(MCDC *dc, coord_t x, coord_t cx, int2 y, uint2 si, uint2 ei, const char *tptr, uint2 pstyle, uint32_t flags);

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
	uint32_t measureattrs(void);

	void setatts(Properties which, void *value);
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
	void setindex(const char *sptr, uint2 i, uint2 l);
	void moveindex(const char *sptr, int2 ioffset, int2 loffset);
	coord_t getcursorx(int2 x, uint2 fi);
	uint2 getcursorindex(coord_t x, coord_t cx, Boolean chunk, Boolean last);
	coord_t getsubwidth(MCDC *dc, coord_t x, uint2 i, uint2 l);
	coord_t getwidth(MCDC *dc, coord_t x);
	void reset();
	void getindex(uint2 &i, uint2 &l);
	uint2 getascent(void);
	uint2 getdescent(void);
    coord_t GetAscent() const;
    coord_t GetDescent() const;
    coord_t GetLeading() const;
	void freeatts();
	void freerefs();
	void openimage();
	void closeimage();
	const char *getlinktext();
	const char *getimagesource();
	// MW-2012-01-06: [[ Block Metadata ]] Returns the metadata of the block as
	//   a cstring if the property has been set; otherwise it returns nil.
	const char *getmetadata(void);
	void sethilite(Boolean on);
	
	bool getfirstlinebreak(uint2& index);
	
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
	bool hasunicode() const
	{
		return (flags & F_HAS_UNICODE) != 0;
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

	bool getflag(unsigned int f) const
	{
		return (flags & f) != 0;
	}

	void sethasunicode(bool p_has_unicode)
	{
		if (p_has_unicode)
			flags |= F_HAS_UNICODE;
		else
			flags &= ~F_HAS_UNICODE;
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
	uint2 indexdecrement(uint2 index);
	uint2 indexincrement(uint2 index);
	Boolean textcomparechar(const char *textptr,char whichchar);
	char *textstrchr(const char *sptr,  uint2 l, char target);
	Boolean textisspace(const char *textptr);
	Boolean textispunct(const char *textptr);
	uint2 getcharsize();
	uint2 getlength();
	uint2 indextocharacter(uint2 si);
	uint2 verifyindex(uint2 si, bool p_is_end);
	MCBlock *remove(MCBlock *&list)
	{
		return (MCBlock *)MCDLlist::remove((MCDLlist *&)list);
	}

	uint2 getindex(void) const
	{
		return index;
	}
	
	uint2 getsize(void) const
	{
		return size;
	}

	
	uint4 getcharatindex(int4 p_index);
	MCBlock *retreatindex(uint2& p_index);
	MCBlock *advanceindex(uint2& p_index);

	bool imagechanged(MCImage *p_image, bool p_deleting);
    
    // FG-2014-11-11: [[ Better theming ]]
    // Sets up the colours on the DC for the given type of drawing
    void setcolorfornormaltext(MCDC*, MCColor*);
    void setcolorforhilite(MCDC*);
    void setcolorforselectedtext(MCDC*, MCColor*);
};
#endif
