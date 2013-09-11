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

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"
#include "mcio.h"
#include "globals.h"

#include "stack.h"
#include "field.h"
#include "image.h"
#include "paragraf.h"
#include "line.h"
#include "block.h"
#include "util.h"
#include "context.h"
#include "unicode.h"
#include "mctheme.h"
#include "font.h"
#include "path.h"

// Default MCBlock constructor - makes a block with everything initialized
// to zero.
MCBlock::MCBlock(void)
{
	parent = NULL;
	flags = F_CLEAR;
	atts = NULL;
	index = size = 0;
	width = 0;
	opened = 0;

	// MW-2012-02-14: [[ FontRefs ]] The font for the block starts off nil.
	m_font = nil;
}

// MCBlock copy constructor - copy an existing block.
// Note that this copies everything except the linkage and opened state.
MCBlock::MCBlock(const MCBlock &bref) : MCDLlist(bref)
{
	parent = bref.parent;
	flags = bref.flags;
	if (flags & F_HAS_ATTS)
	{
		atts = new Blockatts;
		if (flags & F_HAS_COLOR)
		{
			atts->color = new MCColor;
			*atts->color = *bref.atts->color;
		}
		if (flags & F_HAS_BACK_COLOR)
		{
			atts->backcolor = new MCColor;
			*atts->backcolor = *bref.atts->backcolor;
		}

		// MW-2012-02-17: [[ SplitTextAttrs ]] Copy across the font attrs the other
		//   block has.
		if ((flags & F_HAS_FNAME) != 0)
			/* UNCHECKED */ MCNameClone(bref.atts->fontname, atts -> fontname);
		if ((flags & F_HAS_FSIZE) != 0)
			atts -> fontsize = bref . atts -> fontsize;
		if ((flags & F_HAS_FSTYLE) != 0)
			atts -> fontstyle = bref . atts -> fontstyle;

		atts->shift = bref.atts->shift;

		// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
		if (flags & F_HAS_LINK)
			/* UNCHECKED */ MCNameClone(bref.atts->linktext, atts->linktext);
		if (flags & F_HAS_IMAGE)
			/* UNCHECKED */ MCNameClone(bref.atts->imagesource, atts->imagesource);

		// MW-2012-01-6: [[ Block Metadata ]] Copy the metadata (if any).
		if (flags & F_HAS_METADATA)
			/* UNCHECKED */ MCNameClone(bref.atts->metadata, atts->metadata);
	}
	else
		atts = NULL;
	index = bref.index;
	size = bref.size;
	width = 0;
	opened = 0;

	// MW-2012-02-14: [[ FontRefs ]] The font for the block starts off nil.
	m_font = nil;
}

// MCBlock deconstructor - frees all associated data structures.
MCBlock::~MCBlock()
{
	while (opened)
		close();
	if (atts != NULL)
		freeatts();
}

bool MCBlock::visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor *p_visitor)
{
	return p_visitor -> OnBlock(this);
}

// MW-2012-03-04: [[ StackFile5500 ]] If 'is_ext' is true then the record is an extended
//   record.
IO_stat MCBlock::load(IO_handle stream, const char *version, bool is_ext)
{
	IO_stat stat;

	// MW-2012-03-04: [[ StackFile5500 ]] If this is an extended block, then work out
	//   where to skip to when all the attrs currently recognized have been read.
	int64_t t_attr_end;
	if (is_ext)
	{
		// Read the size.
		uint32_t t_size;
		if ((stat = IO_read_uint2or4(&t_size, stream)) != IO_NORMAL)
			return stat;
		
		// The end is the current stream position + the size of the attrs.
		t_attr_end = MCS_tell(stream) + t_size;
	}

	if ((stat = IO_read_uint4(&flags, stream)) != IO_NORMAL)
		return stat;

	// MW-2012-03-04: [[ StackFile5500 ]] If this isn't an extended block, then strip the
	//   metadata flag.
	if (!is_ext)
		flags &= ~F_HAS_METADATA;
	
	// MW-2012-01-26: [[ FlaggedField ]] Make sure we ignore the setting of FLAGGED flag.
	flags &= ~F_FLAGGED;

	if (atts == NULL)
		atts = new Blockatts;

	// MW-2012-02-17: [[ SplitTextAttrs ]] If the font flag is present, it means there
	//   is a font record to read.
	if (flags & F_FONT)
		if (strncmp(version, "1.3", 3) > 0)
		{
			uint2 t_font_index;
			if ((stat = IO_read_uint2(&t_font_index, stream)) != IO_NORMAL)
				return stat;

			// MW-2012-02-17: [[ LogFonts ]] Map the font index we have to the font attrs.
			//   Note that we ignore the 'unicode' tag since that is encoded in flags.
			MCNameRef t_fontname;
			uint2 t_fontstyle, t_fontsize;
			bool t_is_unicode;
			MCLogicalFontTableLookup(t_font_index, t_fontname, t_fontstyle, t_fontsize, t_is_unicode);

			// MW-2012-02-17: [[ SplitTextAttrs ]] Only set the font attrs if they are
			//   not inherited.
			if (!getflag(F_INHERIT_FNAME))
				MCNameClone(t_fontname, atts -> fontname);
			if (!getflag(F_INHERIT_FSIZE))
				atts -> fontsize = t_fontsize;
			if (!getflag(F_INHERIT_FSTYLE))
				atts -> fontstyle = t_fontstyle;
		}
		else
		{
			// MW_2012-02-17: [[ LogFonts ]] Read a nameref directly.
			if ((stat = IO_read_nameref(atts->fontname, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_read_uint2(&atts->fontsize, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_read_uint2(&atts->fontstyle, stream)) != IO_NORMAL)
				return stat;

			// MW-2012-02-17; [[ SplitTextAttrs ]] All the font attrs are set.
			flags |= F_FATTR_MASK;
		}
	if (flags & F_HAS_COLOR)
	{
		atts->color = new MCColor;
		if ((stat = IO_read_mccolor(*atts->color, stream)) != IO_NORMAL)
			return stat;
		if (flags & F_HAS_COLOR_NAME)
		{
			// MW-2012-01-06: [[ Block Changes ]] We no longer use the color name
			//   so load, delete and unset the flag.
			char *colorname;
			if ((stat = IO_read_string(colorname, stream)) != IO_NORMAL)
				return stat;
			delete colorname;
			flags &= ~F_HAS_COLOR_NAME;
		}
	}
	if (flags & F_HAS_BACK_COLOR)
	{
		atts->backcolor = new MCColor;
		if ((stat = IO_read_mccolor(*atts->backcolor, stream)) != IO_NORMAL)
			return stat;
		if (strncmp(version, "2.0", 3) < 0 || flags & F_HAS_BACK_COLOR_NAME)
		{
			// MW-2012-01-06: [[ Block Changes ]] We no longer use the backcolor name
			//   so load, delete and unset the flag.
			char *backcolorname;
			if ((stat = IO_read_string(backcolorname, stream)) != IO_NORMAL)
				return stat;
			delete backcolorname;
			flags &= ~F_HAS_BACK_COLOR_NAME;
		}
	}
	if (flags & F_HAS_SHIFT)
		if ((stat = IO_read_int2(&atts->shift, stream)) != IO_NORMAL)
			return stat;

	// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
	if (flags & F_HAS_LINK)
		if ((stat = IO_read_nameref(atts->linktext, stream)) != IO_NORMAL)
			return stat;
	if (flags & F_HAS_IMAGE)
		if ((stat = IO_read_nameref(atts->imagesource, stream)) != IO_NORMAL)
			return stat;

	// MW-2012-03-04: [[ StackFile5500 ]] If there is a metadata attr then read
	//   it in.
	if (flags & F_HAS_METADATA)
		if ((stat = IO_read_nameref(atts->metadata, stream)) != IO_NORMAL)
			return stat;
	
	// MW-2012-03-04: [[ StackFile5500 ]] If this is an extended block, then skip
	//   to the end of the attrs record.
	if (is_ext)
		if ((stat = MCS_seek_set(stream, t_attr_end)) != IO_NORMAL)
			return stat;
	
	if ((stat = IO_read_uint2(&index, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint2(&size, stream)) != IO_NORMAL)
		return stat;

	// MW-2012-02-17: [[ SplitTextAttrs ]] Adjust the flags to their in-memory
	//   representation. We ditch F_FONT because it is superceeded by the HAS_F*
	//   flags.
	if (getflag(F_FONT))
	{
		flags &= ~F_FONT;
		flags = (flags & ~F_FATTR_MASK) | (~(flags & F_FATTR_MASK) & F_FATTR_MASK);
	}
	else
		flags &= ~F_FATTR_MASK;

	// MW-2012-02-17: [[ SplitTextAttrs ]] If the block has no atts flags set
	//   then clear the atts struct.
	if ((flags & F_HAS_ATTS) == 0)
	{
		delete atts;
		atts = nil;
	}

	return IO_NORMAL;
}

IO_stat MCBlock::save(IO_handle stream, uint4 p_part)
{
	IO_stat stat;

	// MW-2012-03-04: [[ StackFile5500 ]] If the block has metadata and 5.5 stackfile
	//   format has been requested then this is an extended block.
	bool t_is_ext;
	if (MCstackfileversion >= 5500 && getflag(F_HAS_METADATA))
		t_is_ext = true;
	else
		t_is_ext = false;

	// MW-2012-03-04: [[ StackFile5500 ]] If this is an extended block then use the EXT
	//   tag.
	if ((stat = IO_write_uint1(t_is_ext ? OT_BLOCK_EXT : OT_BLOCK, stream)) != IO_NORMAL)
		return stat;
	
	// MW-2012-03-04: [[ StackFile5500 ]] If this is an extended block then write out
	//   the length of the attrs.
	if (t_is_ext)
		if ((stat = IO_write_uint2or4(measureattrs(), stream)) != IO_NORMAL)
			return stat;

	uint4 oldflags = flags;

	// MW-2012-01-06: [[ StackFile5500 ]] If this isn't an extended block, then remove
	//   the metadata flag.
	if (!t_is_ext)
		flags &= ~F_HAS_METADATA;
		
	flags &= ~F_VISITED;
	// MW-2012-01-26: [[ FlaggedField ]] Make sure we don't save the flagged flag.
	flags &= ~F_FLAGGED;

	// MW-2012-02-17: [[ SplitTextAttrs ]] If we have unicode, or one of the font attr are
	//   set then we must serialize a font.
	if ((flags & (F_FATTR_MASK | F_HAS_UNICODE)) != 0)
	{
		// Add in the font record flag.
		flags |= F_FONT;

		// Invert the font attr flags (from has to inherit).
		flags = (flags & ~F_FATTR_MASK) | (~(flags & F_FATTR_MASK) & F_FATTR_MASK);
	}

	if ((stat = IO_write_uint4(flags, stream)) != IO_NORMAL)
		return stat;
		
	flags = oldflags;

	// MW-2012-02-17: [[ SplitTextAttrs ]] If any one of the font attrs are set, or we
	//   are unicode we must serialize a font.
	if ((flags & (F_FATTR_MASK | F_HAS_UNICODE)) != 0)
	{
		// MW-2012-02-17: [[ SplitTextAttrs ]] Compute the attrs to write out.
		MCNameRef t_fontname;
		uint2 t_fontstyle, t_fontsize;
		getfontattrs(nil, t_fontname, t_fontsize, t_fontstyle);

		// MW-2012-02-17: [[ LogFonts ]] Map the font attrs to the appropriate font
		//   index and write.
		uint2 t_font_index;
		t_font_index = MCLogicalFontTableMap(t_fontname, t_fontstyle, t_fontsize, hasunicode());
		if ((stat = IO_write_uint2(t_font_index, stream)) != IO_NORMAL)
			return stat;
	}
	if (flags & F_HAS_COLOR)
		if ((stat = IO_write_mccolor(*atts->color, stream)) != IO_NORMAL)
			return stat;
	if (flags & F_HAS_BACK_COLOR)
		if ((stat = IO_write_mccolor(*atts->backcolor, stream)) != IO_NORMAL)
			return stat;
	if (flags & F_HAS_SHIFT)
		if ((stat = IO_write_int2(atts->shift, stream)) != IO_NORMAL)
			return stat;

	// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
	if (flags & F_HAS_LINK)
		if ((stat = IO_write_nameref(atts->linktext, stream)) != IO_NORMAL)
			return stat;
	if (flags & F_HAS_IMAGE)
		if ((stat = IO_write_nameref(atts->imagesource, stream)) != IO_NORMAL)
			return stat;
	
	// MW-2012-03-04: [[ StackFile5500 ]] If this is an extended block then emit the
	//   new attributes.
	if (t_is_ext)
	{
		if (flags & F_HAS_METADATA)
			if ((stat = IO_write_nameref(atts -> metadata, stream)) != IO_NORMAL)
				return stat;
	}
	
	if ((stat = IO_write_uint2(index, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(size, stream)) != IO_NORMAL)
		return stat;

	return IO_NORMAL;
}

void MCBlock::open(MCFontRef p_parent_font)
{
	if (opened++ != 0)
		return;

	// MW-2012-02-14: [[ FontRefs ]] Map the font for the block.
	mapfont(p_parent_font);

	if (flags & F_HAS_COLOR)
		MCscreen->alloccolor(*atts->color);
	if (flags & F_HAS_BACK_COLOR)
		MCscreen->alloccolor(*atts->backcolor);
	openimage();
	width = 0;
}

void MCBlock::close()
{
	if (opened == 0 || --opened != 0)
		return;
	closeimage();

	// MW-2012-02-14: [[ FontRefs ]] Unmap the font for the block.
	unmapfont();
}

// MW-2012-02-14: [[ FontRefs ]] This method computes the fontref for the block.
void MCBlock::mapfont(MCFontRef p_parent_font)
{
	// If we have no font attrs set, then we just copy the parent font.
	if (!hasfontattrs())
	{
		m_font = MCFontRetain(p_parent_font);
		return;
	}

	// MW-2012-02-17: [[ SplitTextAttrs ]] Compute the attrs to write out. If we don't
	//   have all of the attrs, fetch the inherited ones.
	MCNameRef t_textfont;
	uint2 t_textstyle, t_textsize;
	getfontattrs(nil, t_textfont, t_textsize, t_textstyle);

	// Compute the font style from the text style.
	MCFontStyle t_font_style;
	t_font_style = MCFontStyleFromTextStyle(t_textstyle);

	// If the parent font had printer metrics, then make sure we have them too.
	if (MCFontHasPrinterMetrics(p_parent_font))
		t_font_style |= kMCFontStylePrinterMetrics;

	// Create the fontref.
	/* UNCHECKED */ MCFontCreate(t_textfont, t_font_style, t_textsize, m_font);
}

// MW-2012-02-14: [[ FontRefs ]] This method delete the fontref associated with the block.
void MCBlock::unmapfont(void)
{
	MCFontRelease(m_font);
	m_font = nil;
}

// MW-2012-02-14: [[ FontRefs ]] This method updates the current fontref associated with the
//   block, returning 'true' if it has changed.
bool MCBlock::recomputefonts(MCFontRef p_parent_font)
{
	// If there are no font attributes, then we are updating to the parent font.
	if (!hasfontattrs())
	{
		// If the font is the same as the parent font already then there is nothing to do.
		if (m_font == p_parent_font)
			return false;

		// Otherwise change the current fontref for a copy of the parent fontref.
		MCFontRelease(m_font);
		m_font = MCFontRetain(p_parent_font);

		width = 0;

		// As we changed fontref, return 'true'.
		return true;
	}

	// Take a copy of the current font.
	MCFontRef t_current_font;
	t_current_font = MCFontRetain(m_font);

	// Unmap the current fontref, then map again.
	unmapfont();
	mapfont(p_parent_font);

	// We've only changed if the new font is different from the old fontref.
	bool t_changed;
	t_changed = m_font != t_current_font;

	// Get rid of the temporary copy of the old font we took.
	MCFontRelease(t_current_font);

	if (t_changed)
		width = 0;

	// Return the result of the operation.
	return t_changed;
}

// MW-2012-02-06: [[ Bug ]] The 'flagged' property was causing multiple (identical)
//   runs in the styledText format. To rectify this, the 'persistent_only' param
//   indicates whether the comparison is for persistence (styledText) or for in-
//   memory processing (defrag).
Boolean MCBlock::sameatts(MCBlock *bptr, bool p_persistent_only)
{
	// Check to see if a subset of the flags are the same, if they aren't the
	// blocks are not the same. Notice that in 'persistent' mode (used by
	// styledText generation) we exclude the transient flags. We need to keep
	// the latter in the case of 'defrag()' as technically they are different
	// runs from that point of view.

	// MW-2012-02-13: [[ Block Unicode ]] Make sure both blocks have the same
	//   'unicode' flag.
	// MW-2012-02-17: [[ SplitTextAttrs ]] Check for all the fattr rather than
	//   font.
	uint32_t t_flags;
	t_flags = F_HAS_ALL_FATTR | F_HAS_COLOR | F_HAS_METADATA | F_HAS_SHIFT | F_HAS_BACK_COLOR | F_HAS_LINK | F_HAS_UNICODE;
	if (!p_persistent_only)
		t_flags |= F_HILITED | F_FLAGGED | F_VISITED;

	// If the flags don't match, we can't be the same.
	if ((flags & t_flags) != (bptr -> flags & t_flags))
		return False;

	// If either block has an image, we treat them as different blocks.
	if (flags & F_HAS_IMAGE || bptr->flags & F_HAS_IMAGE)
		return False;

	// MW-2012-02-17: [[ SplitTextAttrs ]] CHeck to see if all the font attrs
	//   match in turn.
	if ((flags & F_HAS_FNAME) != 0 && (atts -> fontname != bptr -> atts -> fontname))
		return False;
	if ((flags & F_HAS_FSTYLE) != 0 && (atts -> fontstyle != bptr -> atts -> fontstyle))
		return False;
	if ((flags & F_HAS_FSIZE) != 0 && (atts -> fontsize != bptr -> atts -> fontsize))
		return False;

	// If the linkTexts are not the same, the blocks are different. Notice that
	// we do a pointer comparison - these are 'Names' and as such are identical
	// iff they are the same pointer.
	if (getlinktext() != bptr -> getlinktext())
		return False;
	
	// If the metadatas are not the same, the blocks are different. Notice that
	// we do a pointer comparison - these are 'Names' and as such are identical
	// iff they are the same pointer.
	if (getmetadata() != bptr -> getmetadata())
		return False;

	// If we have a color set, then make sure it matches the other block.
	if ((flags & F_HAS_COLOR) != 0 &&
			(bptr->atts->color->red != atts->color->red ||
				bptr->atts->color->green != atts->color->green ||
				bptr->atts->color->blue != atts->color->blue))
		return False;

	// If we have a backColor set, then make sure it matches the other block.
	if ((flags & F_HAS_BACK_COLOR) != 0 &&
			(bptr->atts->backcolor->red != atts->backcolor->red ||
				bptr->atts->backcolor->green != atts->backcolor->green ||
				bptr->atts->backcolor->blue != atts->backcolor->blue))
		return False;

	// If we have a shift set, then make sure it matches the other block.
	if ((flags & F_HAS_SHIFT) != 0 &&
			(bptr -> atts -> shift != atts -> shift))
		return False;

	// Everything matches, so these two blocks must be the same.
	return True;
}

static bool MCUnicodeCanBreakBetween(uint2 x, uint2 y)
{
	if (x < 256 && isspace(x))
		return true;

	bool t_xid;
	t_xid = MCUnicodeCodepointIsIdeographic(x);
	
	bool t_yid;
	t_yid = MCUnicodeCodepointIsIdeographic(y);
	
	if (!t_xid && !t_yid)
		return false;

	bool t_prohibit_break_after_x;
	t_prohibit_break_after_x = (MCUnicodeCodepointGetBreakClass(x) & 2) != 0;
	
	bool t_prohibit_break_before_y;
	t_prohibit_break_before_y = (MCUnicodeCodepointGetBreakClass(y) & 1) != 0;
	
	return !t_prohibit_break_after_x && !t_prohibit_break_before_y;
}

bool MCBlock::fit(int2 x, uint2 maxwidth, uint2& r_break_index, bool& r_break_fits)
{
	// If the block of zero length, then it can't be broken and must be kept with previous blocks.
	if (size == 0)
	{
		if (next() != parent -> getblocks())
			r_break_fits = false;
		else
			r_break_fits = true;
		r_break_index = index;
		return true;
	}
			
	if ((flags & F_HAS_IMAGE) && atts -> image != NULL)
	{
		r_break_index = index + size;
		r_break_fits = getwidth(NULL, x) <= maxwidth;
		return r_break_fits;
	}
	
	const char *text;
	text = parent -> gettext();

	// Fetch the first character of the next block
	int4 t_next_block_char;
	MCBlock *t_next_block;
	t_next_block = next();
	if (t_next_block != parent -> getblocks())
	{
		if (t_next_block -> getsize() == 0)
			t_next_block_char = -2;
		else if (t_next_block -> hasunicode())
			t_next_block_char = *(uint2 *)&text[t_next_block -> index];
		else
			t_next_block_char = MCUnicodeMapFromNative(text[t_next_block -> index]);
	}
	else
		t_next_block_char = -1;

	// If this is a text block and we fit inside maxwidth fully then check
	// for a break point at the end.
	if (getwidth(NULL, x) <= maxwidth)
	{
		uint2 t_last_char, t_break_index;
		if (hasunicode() && (size & 1) == 0)
			t_last_char = *(uint2 *)&text[index + size - 2], t_break_index = index + size - 2;
		else
			t_last_char = MCUnicodeMapFromNative(text[index + size - 1]), t_break_index = index + size - 1;

		// If this is the last block, or we can break between this and the
		// next block, return the end point.
		if (t_next_block_char != -2)
		{
			if (t_next_block_char == -1 || MCUnicodeCanBreakBetween(t_last_char, t_next_block_char))
			{
				r_break_index = index + size;
				r_break_fits = true;
				return true;
			}
		}
			
		// Compute the last possible break position in the block by looping
		// back over the characters;
		if (t_break_index > index)
		{
			do
			{
				uint2 i;
				i = t_break_index;
				
				uint2 t_prev_char;
				if (hasunicode())
				{
					i -= 2;
					t_prev_char = *(uint2 *)&text[i];
				}
				else
				{
					i -= 1;
					t_prev_char = MCUnicodeMapFromNative(text[i]);
				}
				
				if (MCUnicodeCanBreakBetween(t_prev_char, t_last_char))
					break;
				
				t_break_index = i;
				
				t_last_char = t_prev_char;
			}
			while(t_break_index > index);
		}
		
		r_break_index = t_break_index;
		r_break_fits = true;
		return true;
	}
	
	// We don't completely fit within maxwidth, so compute the last break point in
	// the block by measuring
	int4 twidth;
	twidth = 0;

	// MW-2009-04-23: [[ Bug ]] For printing, we measure complete runs of text otherwise we get
	//   positioning issues.
	int4 t_last_break_width;
	t_last_break_width = 0;
	uint2 t_last_break_i;
	t_last_break_i = index;
	
	uint2 i;
	i = index;
	
	int4 t_next_char;
	if (hasunicode())
		t_next_char = *(uint2 *)&text[i];
	else
		t_next_char = (uint2)MCUnicodeMapFromNative(text[i]);
	
	uint2 t_break_index;
	t_break_index = index;

	bool t_can_fit;
	t_can_fit = false;
	
	// MW-2013-08-01: [[ Bug 10932 ]] Optimized loop only measuring between potential break
	//   points, rather than char by char.
	while(i < index + size)
	{
		uint4 initial_i;
		initial_i = i;
		
		bool t_can_break;
		t_can_break = false;
		
		uint2 t_this_char;
		while(i < index + size)
		{
			t_this_char = (uint2)t_next_char;
			
			i += indexincrement(i);
			
			if (hasunicode())
			{
				if (i < index + size)
					t_next_char = *(uint2 *)&text[i];
				else
					t_next_char = t_next_block_char;
			}
			else
			{
				if (i < index + size)
					t_next_char = (uint2)MCUnicodeMapFromNative(text[i]);
				else
					t_next_char = t_next_block_char;
			}
			
			if (t_this_char == '\t' ||
				t_next_char == -1 ||
				MCUnicodeCanBreakBetween(t_this_char, t_next_char))
			{
				t_can_break = true;
				break;
			}
		}

		if (t_this_char == '\t')
		{
			twidth += gettabwidth(x + twidth, text, initial_i);

			t_last_break_width = twidth;
			t_last_break_i = i;
		}
		else
		{
#ifdef _IOS_MOBILE
			// MW-2012-02-01: [[ Bug 9982 ]] iOS uses sub-pixel positioning, so make sure we measure
			//   complete runs.
			twidth = t_last_break_width + MCFontMeasureText(m_font, &text[t_last_break_i], i - t_last_break_i, hasunicode());
#else
#ifdef TARGET_PLATFORM_WINDOWS
			// MW-2009-04-23: [[ Bug ]] For printing, we measure complete runs of text otherwise we get
			//   positioning issues.
			if (MCFontHasPrinterMetrics(m_font))
				twidth = t_last_break_width + MCFontMeasureText(m_font, &text[t_last_break_i], i - t_last_break_i, hasunicode());
			else
#endif
				twidth += MCFontMeasureText(m_font, &text[initial_i], i - initial_i, hasunicode());
#endif
		}

		if (t_can_fit && twidth > maxwidth)
			break;

		if (t_can_break)
		{
			t_break_index = i;

			if (twidth <= maxwidth)
				t_can_fit = true;

			if (twidth >= maxwidth)
				break;
		}
	}

	// We now have a suitable break point in t_break_index. This could be index if
	// there are none in the block. We now loop forward until we get to the end of
	// any suitable run of spaces.
	while(t_break_index < index + size && textisspace(&text[t_break_index]))
		t_break_index += indexincrement(t_break_index);
		
	r_break_fits = t_can_fit;
	r_break_index = t_break_index;
	
	return false;
}

void MCBlock::split(uint2 p_index)
{
	MCBlock *bptr = new MCBlock(*this);
	uint2 newlength = size - (p_index - index);
	bptr->setindex(parent -> gettext(), p_index, newlength);
	size -= newlength;
	width = 0;
	// MW-2012-02-13: [[ Block Unicode ]] Only open the new block if the original
	//   was already open.
	// MW-2012-02-14: [[ FontRefs ]] Pass in the parent fontref so the block can
	//   compute its fontref.
	if (opened)
		bptr->open(parent -> getparent() -> getfontref());
	append(bptr);
}

// Compute the distance between x and the next tab stop position.
int2 MCBlock::gettabwidth(int2 x, const char *text, uint2 i)
{
	uint2 *tabs;
	uint2 ntabs;
	Boolean fixed;

	// MW-2012-01-25: [[ ParaStyles ]] Fetch the tab-stops from the owning paragraph.
	parent->gettabs(tabs, ntabs, fixed);

	// The tabs are 'fixed' if VGRID is set - I presume this also prevents wrapping
	// but who can really say!
	if (fixed)
	{
		const char *cptr = text;
		uint2 ctab = 0;
		uint2 cindex = 0;

		// Count the number of preceeding tabs in the paragraph.
		MCBlock *t_block;
		t_block = parent -> getblocks();
		uint2 j;
		j = 0;
		while(j < i)
		{
			if (t_block -> getflag(F_HAS_TAB))
			{
				uint2 k;
				k = t_block -> getindex() + t_block -> getsize();
				while(j < k && j < i)
				{
					if (t_block -> textcomparechar(&cptr[j], '\t'))
						ctab++;
					j += t_block -> indexincrement(j);
				}
			}
			else
				j = t_block -> getindex() + t_block -> getsize();
			t_block = t_block -> next();
		}

		// At this point 'ctab' contains the number of tab characters before
		// the one gettabwidth was called with - i.e. the 0-based index
		// of the tab we started with.

		int2 newloc;
		if (ctab < ntabs)
		{		
			// The tab array stores absolute positions of each tab from the
			// left of the field.
			newloc = tabs[ctab];

		}
		else
		{
			// We have requested a tab beyond the end of the tab array, so
			// we use the length of the last tab as the length of every successive
			// tab.
			uint2 diff;
			if (ntabs == 1)
				diff = tabs[0];
			else
				diff = tabs[ntabs - 1] - tabs[ntabs - 2];
			newloc = tabs[ntabs - 1] + diff * (ctab - ntabs + 1);
		}

		// If the position of the current tab co-incides with the last x position then
		// adjust upward by one to ensure we don't overwrite anything (legacy behavior).
		if (newloc == x)
			return 1;

		// The width of the tab is absolute position of the tab minus the x position of
		// the left hand-side of the passed in tab character.
		return newloc - x;
	}
	else
	{
		int2 lasttab = 0;
		uint2 i = 0;

		// Loop through the tab array and find the first tab to the right of x.
		while (i < ntabs && x >= lasttab)
		{
			lasttab = tabs[i];
			i++;
		}

		if (x >= lasttab && ntabs != 0)
		{
			// If x is further right than (or is at) the last tab then the tab position
			// is the first multiple of the last difference after the last tab which is
			// beyond x.
			uint2 diff;
			if (ntabs == 1)
				diff = tabs[0];
			else
				diff = tabs[ntabs - 1] - tabs[ntabs - 2];

			// MW-2012-09-19: [[ Bug 10239 ]] The tab difference can now be zero, in
			//   the non-vGrid case, if this is the case then just take lasttab to be
			//   x.
			if (diff != 0)
				lasttab = tabs[ntabs - 1] + diff * ((x - tabs[ntabs - 1]) / diff + 1);
			else
				lasttab = x;
		}

		// Adjust by 1 if the last tab is x
		if (lasttab == x)
			return 1;

		// Return the difference between x and the last tab.
		return lasttab - x;
	}
}

void MCBlock::drawstring(MCDC *dc, int2 x, int2 cx, int2 y, uint2 start, uint2 length, Boolean image, uint32_t style)
{
	// MW-2012-02-16: [[ FontRefs ]] Fetch the font metrics we need to draw.
	int32_t t_ascent, t_descent;
	t_ascent = MCFontGetAscent(m_font);
	t_descent = MCFontGetDescent(m_font);
	
	// MW-2012-01-25: [[ ParaStyles ]] Fetch the vGrid setting from the owning paragraph.
	if (parent -> getvgrid())
	{
		const char *t_text;
		t_text = parent -> gettext();

		// MW-2012-02-09: [[ ParaStyles ]] Fetch the padding setting from the owning paragraph.
		// MW-2012-03-19: [[ Bug 10069 ]] Use the horiztonal padding value here.
		int32_t t_padding;
		t_padding = parent -> gethpadding();

		MCRectangle t_old_clip;
		t_old_clip = dc -> getclip();

		MCRectangle t_cell_clip;
		t_cell_clip = t_old_clip;

		int32_t t_delta;
		t_delta = cx - x;

		uint2 t_index;
		t_index = start;
		while(t_index < start + length)
		{
			const char *t_next_tab;
			t_next_tab = textstrchr(t_text + t_index, start + length - t_index, '\t');

			uint2 t_next_index;
			if (t_next_tab == nil)
				t_next_index = start + length;
			else
				t_next_index = t_next_tab - t_text;

			int2 t_tab_width;
			t_tab_width = gettabwidth(0, t_text, t_index);

			uint2 t_cell_right;
			t_cell_right = t_tab_width - t_delta;

			uint2 t_width;
			t_width = MCFontMeasureText(m_font, t_text + t_index, t_next_index - t_index, hasunicode());

			// MW-2012-02-09: [[ ParaStyles ]] Compute the cell clip, taking into account padding.
			t_cell_clip . x = x - 1;
			t_cell_clip . width = MCU_max(t_cell_right - x - t_padding * 2, 0);

			t_cell_clip = MCU_intersect_rect(t_cell_clip, t_old_clip);
			dc -> setclip(t_cell_clip);
			MCFontDrawText(m_font, t_text + t_index, t_next_index - t_index, hasunicode(), dc, x, y, image == True);

			// Only draw the various boxes/lines if there is any content.
			if (t_next_index - t_index > 0)
			{
				if ((style & FA_UNDERLINE) != 0)
					dc -> drawline(x, y + 1, x + t_width, y + 1);
				if ((style & FA_STRIKEOUT) != 0)
					dc -> drawline(x, y - (t_ascent >> 1), x + t_width, y - (t_ascent >> 1));
				if ((style & FA_BOX) != 0)
				{
					// MW-2012-09-04: [[ Bug 9759 ]] Adjust any pattern origin to scroll with text.
					parent -> getparent() -> setforeground(dc, DI_BORDER, False, True);
					parent -> getparent() -> adjustpixmapoffset(dc, DI_BORDER);
					MCRectangle trect = { x - 1, y - t_ascent, t_width + 3, t_ascent + t_descent };
					dc->drawrect(trect);
					parent -> getparent() -> setforeground(dc, DI_FORE, False, True);
				}
				else if ((style & FA_3D_BOX) != 0)
				{
					MCRectangle trect = { x - 1, y - t_ascent, t_width + 2, t_ascent + t_descent };
					parent -> getparent() -> draw3d(dc, trect, ETCH_RAISED, 1);
					parent -> getparent() -> setforeground(dc, DI_FORE, False, True);
				}
			}

			x += t_width;

			if (t_next_tab != nil)
			{
				x = t_cell_right;
				t_next_index += indexincrement(t_next_index);
			}

			t_index = t_next_index;
		}

		dc -> setclip(t_old_clip);
	}
	else
	{
		const char *sptr;
		uint2 size;
		sptr = parent -> gettext() + start;
		size = length;
		
		// MW-2012-02-21: [[ LineBreak ]] Trim the block slightly if there is an explicit line break
		//   at the end of the block.
		// MW-2013-02-12: [[ Bug 10662 ]] Make sure we take into account unicode chars.
		if (size > 0 && textcomparechar(sptr + size - getcharsize(), 11))
			size -= 1;
		
		// If we need an underline/strikeout then compute the start and width.
		int32_t t_line_width, t_line_x;
		t_line_width = 0;
		t_line_x = x;
		if ((style & (FA_UNDERLINE | FA_STRIKEOUT)) != 0)
			t_line_width = getsubwidth(dc, cx, start, size);
		
		if (flags & F_HAS_TAB)
		{
			const char *eptr;
			const char *tptr = parent->gettext();
			while ((eptr = textstrchr(sptr, size, '\t')) != NULL)
			{
				uint2 l = eptr - sptr;
				if (size < l)
					break;

				uint2 twidth;
				twidth = MCFontMeasureText(m_font, sptr, l, hasunicode());
				twidth += gettabwidth(cx + twidth, tptr, eptr - tptr);

				MCFontDrawText(m_font, sptr, l, hasunicode(), dc, x, y, image == True);

				cx += twidth;
				x += twidth;

				// Adjust for the tab character.
				l += indexincrement(eptr - tptr);

				sptr += l;
				size -= l;
			}
		}
		MCFontDrawText(m_font, sptr, size, hasunicode(), dc, x, y, image == True);

		// Apply strike/underline.
		if ((style & FA_UNDERLINE) != 0)
			dc -> drawline(t_line_x, y + 1, t_line_x + t_line_width, y + 1);
		if ((style & FA_STRIKEOUT) != 0)
			dc -> drawline(t_line_x, y - (t_ascent >> 1), t_line_x + t_line_width, y - (t_ascent >> 1));		
	}
}

void MCBlock::draw(MCDC *dc, int2 x, int2 cx, int2 y, uint2 si, uint2 ei, const char *tptr, uint2 pstyle, uint32_t p_border_flags)
{
	if (flags & F_HAS_SHIFT)
		y += atts->shift;

	if (flags & F_HAS_IMAGE && atts->image != NULL)
	{
		// store coordinates away so we can animate it (yeah, it's not a
		// good idea to store data in pointers, but it's better than
		// increasing the size of the atts structure...)
		atts->x = x;
		atts->y = y;
		atts->image->drawcentered(dc, x + (atts->image->getrect().width >> 1), y - (atts->image->getrect().height >> 1), False);
		return;
	}

	// MW-2012-02-16: [[ FontRefs ]] Fetch the font metrics we need to draw.
	int32_t t_ascent, t_descent;
	t_ascent = MCFontGetAscent(m_font);
	t_descent = MCFontGetDescent(m_font);

	// MW-2012-02-17: [[ SplitTextAttrs ]] If we have a font style, then use that; otherwise
	//   use the parent's.
	uint2 fontstyle;
	if (getflag(F_HAS_FSTYLE))
		fontstyle = atts->fontstyle;
	else
		fontstyle = pstyle;

	MCField *f = parent->getparent();
	Boolean ull = MClinkatts.underline;

	MCColor *t_foreground_color;
	t_foreground_color = NULL;

	// MW-2008-02-05: [[ Bug 5821 ]] Compute the appropriate foreground color here and use
	//   it to set the foreground either side of the selected run.
	if (fontstyle & FA_LINK)
	{
		Linkatts *a = f->getstack()->getlinkatts();
		if (flags & F_HILITED)
			t_foreground_color = &a -> hilitecolor;
		else
			if (flags & F_VISITED)
				t_foreground_color = &a -> visitedcolor;
			else
				t_foreground_color = &a -> color;
		ull = a->underline;
	}

	if (flags & F_HAS_COLOR && atts->color->pixel != MAXUINT4)
		t_foreground_color = atts -> color;

	if (flags & F_HAS_BACK_COLOR && atts->backcolor->pixel != MAXUINT4)
		dc->setbackground(*atts->backcolor);

	if (t_foreground_color != NULL)
		dc -> setforeground(*t_foreground_color);
	
	uint32_t t_style;
	t_style = 0;
	if (fontstyle & FA_UNDERLINE || fontstyle & FA_LINK && ull)
		t_style |= FA_UNDERLINE;
	if (fontstyle & FA_STRIKEOUT)
		t_style |= FA_STRIKEOUT;
	if (fontstyle & FA_3D_BOX)
		t_style |= FA_3D_BOX;
	if (fontstyle & FA_BOX)
		t_style |= FA_BOX;
	
	// If there is no selection, or the entire block is outside the selection, then
	// just draw normally. Otherwise use clipping to make change the hilite color of
	// the selected portion of text, thus stopping drawing the selection changing the
	// metrics of the text (due to sub-pixel positioning).
	if (ei == si || si >= index + size || ei <= index)
		drawstring(dc, x, cx, y, index, size, (flags & F_HAS_BACK_COLOR) != 0, t_style);
	else
	{
		// Save the current clip.
		MCRectangle t_old_clip;
		t_old_clip = dc -> getclip();
		
		// This will hold the clip for the selection.
		MCRectangle t_sel_clip;
		t_sel_clip = t_old_clip;
		
		// If there is some unselected text at the start of the block, then render it.
		if (si > index)
		{
			int2 t_start_dx;
			t_start_dx = getsubwidth(dc, cx, index, si - index);
			
			t_sel_clip . x = x + t_start_dx;
			t_sel_clip . width = (t_old_clip . x + t_old_clip . width) - t_sel_clip . x;
			
			MCRectangle t_clip;
			t_clip = t_old_clip;
			t_clip . width = (x + t_start_dx) - t_clip . x;
			dc -> setclip(t_clip);
			drawstring(dc, x, cx, y, index, size, (flags & F_HAS_BACK_COLOR) != 0, t_style);
		}

		// If there is some unselected text at the end of the block, then render it.
		if (ei < index + size)
		{
			int32_t t_end_dx;
			t_end_dx = getsubwidth(dc, cx, index, ei - index);
			
			t_sel_clip . width = (x + t_end_dx) - t_sel_clip . x;
			
			MCRectangle t_clip;
			t_clip = t_old_clip;
			t_clip . x = x + t_end_dx;
			t_clip . width = (t_old_clip . x + t_old_clip . width) - t_clip . x;
			dc -> setclip(t_clip);
			drawstring(dc, x, cx, y, index, size, (flags & F_HAS_BACK_COLOR) != 0, t_style);
		}
		
		// Now use the clip rect we've computed for the selected portion.
		dc -> setclip(t_sel_clip);
		
		// Change the hilite color (if necessary).
		if (!(flags & F_HAS_COLOR) || atts->color->pixel == MAXUINT4)
		{
			if (IsMacLF() && !f->isautoarm())
			{
				MCPatternRef t_pattern;
				int2 x, y;
				MCColor fc, hc;
				f->getforecolor(DI_FORE, False, True, fc, t_pattern, x, y, dc, f);
				f->getforecolor(DI_HILITE, False, True, hc, t_pattern, x, y, dc, f);
				if (hc.pixel == fc.pixel)
					f->setforeground(dc, DI_BACK, False, True);
			}
			else
				f->setforeground(dc, DI_BACK, False, True);
		}
		
		// Draw the selected text.
		drawstring(dc, x, cx, y, index, size, (flags & F_HAS_BACK_COLOR) != 0, t_style);
		
		// Revert to the previous clip and foreground color.
		if (t_foreground_color != NULL)
			dc->setforeground(*t_foreground_color);
		else if (!(flags & F_HAS_COLOR) || atts->color->pixel == MAXUINT4)
			f->setforeground(dc, DI_FORE, False, True);
		dc-> setclip(t_old_clip);
	}
	
	// MW-2012-01-25: [[ ParaStyles ]] Use the owning paragraph to test for vGrid-ness.
	if (!parent -> getvgrid() && (fontstyle & (FA_BOX | FA_3D_BOX)) != 0)
	{
		// MW-2012-02-27: [[ Bug 2939 ]] Setup a clipping rect so that we can clip out
		//   the left and/or right edge of the box style as determined by 'flags'.
		
		// Save the current clip setting.
		MCRectangle t_old_clip;
		t_old_clip = dc -> getclip();
		
		// Compute the width of the block.
		int32_t t_width;
		t_width = getwidth(dc, cx);
		
		// Start off with the clip being that which it was previously.
		MCRectangle t_clip;
		t_clip = t_old_clip;
		
		// If we aren't drawing the left edge, move it inwards.
		if ((p_border_flags & DBF_DRAW_LEFT_EDGE) == 0)
			t_clip . x = x, t_clip . width = t_old_clip . x + t_old_clip . width - x;
		
		// If we aren't drawing the right edge, move it inwards.
		if ((p_border_flags & DBF_DRAW_RIGHT_EDGE) == 0)
			t_clip . width = x + t_width - t_clip . x;
		
		// Set the clip temporarily.
		dc -> setclip(t_clip);
		
		if (fontstyle & FA_BOX)
		{
			// MW-2012-09-04: [[ Bug 9759 ]] Adjust any pattern origin to scroll with text.
			f->setforeground(dc, DI_BORDER, False, True);
			f->adjustpixmapoffset(dc, DI_BORDER);
			MCRectangle trect = { x - 1, y - t_ascent, t_width + 2, t_ascent + t_descent };
			dc->drawrect(trect);
			f->setforeground(dc, DI_FORE, False, True);
		}
		else if (fontstyle & FA_3D_BOX)
		{
			MCRectangle trect = { x - 1, y - t_ascent, t_width + 2, t_ascent + t_descent };
			f->draw3d(dc, trect, ETCH_RAISED, 1);
			f->setforeground(dc, DI_FORE, False, True);
		}
		
		// Revert the clip back to the previous setting.
		dc -> setclip(t_old_clip);
	}
	
	if (flags & F_HAS_BACK_COLOR && atts->backcolor->pixel != MAXUINT4)
		dc->setbackground(MCzerocolor);

	if (flags & F_HAS_COLOR && atts->color->pixel != MAXUINT4 || fontstyle & FA_LINK)
		f->setforeground(dc, DI_FORE, False, True);

	// MW-2010-01-06: If there is link text, then draw a link
	if ((fontstyle & FA_LINK) != 0 && getlinktext() != nil)
	{
		MCRectangle t_box;
		MCU_set_rect(t_box, x - 1, y - t_ascent, getwidth(dc, cx) + 3, t_ascent + t_descent);
		dc -> drawlink(getlinktext(), t_box);
	}
}

// MW-2012-02-17: [[ SplitTextAttrs ]] Fetch the block's effective font attrs, using the base attrs
//   in p_base_attrs (if non-nil). If p_base_attrs is nil, the parent's attrs are fetched and used.
void MCBlock::getfontattrs(MCObjectFontAttrs *p_base_attrs, MCNameRef& r_fname, uint2& r_fsize, uint2& r_fstyle)
{
	if ((flags & F_FATTR_MASK) != F_FATTR_MASK)
	{
		if (p_base_attrs == nil)
			parent -> getparent() -> getfontattsnew(r_fname, r_fsize, r_fstyle);
		else
		{
			r_fname = p_base_attrs -> name;
			r_fsize = p_base_attrs -> size;
			r_fstyle = p_base_attrs -> style;
		}
	}

	if (getflag(F_HAS_FNAME))
		r_fname = atts -> fontname;

	if (getflag(F_HAS_FSIZE))
		r_fsize = atts -> fontsize;

	if (getflag(F_HAS_FSTYLE))
		r_fstyle = atts -> fontstyle;
}

// MW-2012-02-17: [[ SplitTextAttrs ]] Fetch the textFont setting of the block, returning true
//   if there is one or false if there is not.
bool MCBlock::gettextfont(MCNameRef& r_textfont) const
{
	if (getflag(F_HAS_FNAME))
	{
		r_textfont = atts -> fontname;
		return true;
	}
	return false;
}

// MW-2012-02-17: [[ SplitTextAttrs ]] Fetch the textFont setting of the block, returning true
//   if there is one or false if there is not.
bool MCBlock::gettextfont(const char *& r_textfont) const
{
	if (getflag(F_HAS_FNAME))
	{
		r_textfont = MCNameGetCString(atts -> fontname);
		return true;
	}
	return false;
}

// MW-2012-02-17: [[ SplitTextAttrs ]] Fetch the textSize setting of the block, returning true
//   if there is one or false if there is not.
bool MCBlock::gettextsize(uint2& r_textsize) const
{
	if (getflag(F_HAS_FSIZE))
	{
		r_textsize = atts -> fontsize;
		return true;
	}
	return false;
}

// MW-2012-02-17: [[ SplitTextAttrs ]] Fetch the textStyle setting of the block, returning true
//   if there is one or false if there is not.
bool MCBlock::gettextstyle(uint2& r_textstyle) const
{
	if (getflag(F_HAS_FSTYLE))
	{
		r_textstyle = atts -> fontstyle;
		return true;
	}
	return false;
}

bool MCBlock::hasfontattrs(void) const
{
	return (flags & F_HAS_ALL_FATTR) != 0;
}

void MCBlock::setatts(Properties which, void *value)
{
		// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
	if (which == P_LINK_TEXT)
	{
		const char *t_text;
		t_text = (const char *)value;

		if (flags & F_HAS_LINK)
		{
			// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
			MCNameDelete(atts -> linktext);
			atts -> linktext = nil;
		}

		if (strlen(t_text) == 0)
			flags &= ~F_HAS_LINK;
		else
		{
			if (atts == NULL)
				atts = new Blockatts;

			// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
			/* UNCHECKED */ MCNameCreateWithCString(t_text, atts -> linktext);
			flags |= F_HAS_LINK;
		}
	}
	else if (which == P_IMAGE_SOURCE)
	{
		const char *t_image;
		t_image = (const char *)value;

		if (flags & F_HAS_IMAGE)
		{
			if (opened)
				closeimage();

			// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
			MCNameDelete(atts -> imagesource);
			atts -> imagesource = nil;
		}

		if (strlen(t_image) == 0)
			flags &= ~F_HAS_IMAGE;
		else
		{
			if (atts == NULL)
				atts = new Blockatts;

			// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
			/* UNCHECKED */ MCNameCreateWithCString(t_image, atts -> imagesource);
			atts->image = NULL;
			flags |= F_HAS_IMAGE;
		}
		if (opened)
			openimage();
	}
	else if (which == P_METADATA)
	{
		// MW-2012-01-06: [[ Block Metadata ]] Handle setting/unsetting the metadata
		//   property.
		const char *t_metadata;
		t_metadata = (const char *)value;

		if (flags & F_HAS_METADATA)
		{
			MCNameDelete(atts -> metadata);
			atts -> metadata = nil;
		}

		if (strlen(t_metadata) == 0)
			flags &= ~F_HAS_METADATA;
		else
		{
			if (atts == nil)
				atts = new Blockatts;

			/* UNCHECKED */ MCNameCreateWithCString(t_metadata, atts -> metadata);

			flags |= F_HAS_METADATA;
		}
	}
	else if (which == P_FLAGGED)
	{
		// MW-2012-01-26: [[ FlaggedField ]] Set the appropriate flag.
		if ((Boolean)(intptr_t)value == True)
			flags |= F_FLAGGED;
		else
			flags &= ~F_FLAGGED;
	}
	else
	{
		// MW-2012-02-17: [[ SplitTextAttrs ]] If the value is not nil then we
		//   must be setting an attr so make sure we have atts.
		if (value != nil)
			if (atts == nil)
				atts = new Blockatts;
		
		// MW-2012-02-17: [[ SplitTextAttrs ]] Update the appropriate text attr.
		switch(which)
		{
		case P_TEXT_FONT:
			if (value == nil || strlen((const char *)value) == 0)
			{
				flags &= ~F_HAS_FNAME;
				if (atts != nil)
				{
					MCNameDelete(atts -> fontname);
					atts -> fontname = nil;
				}
			}
			else
			{
				flags |= F_HAS_FNAME;
				/* UNCHECKED */ MCNameCreateWithCString((const char *)value, atts -> fontname);
			}
			break;

		case P_TEXT_SIZE:
			if (value == nil)
				flags &= ~F_HAS_FSIZE;
			else
			{
				flags |= F_HAS_FSIZE;
				atts -> fontsize = (uint2)(intptr_t)value;
			}
			break;

		case P_TEXT_STYLE:
			if (value == nil)
				flags &= ~F_HAS_FSTYLE;
			else
			{
				flags |= F_HAS_FSTYLE;
				atts -> fontstyle = (uint2)(intptr_t)value;
			}
			break;

		// MW-2011-11-23: [[ Array TextStyle ]] These pseudo-properties are used when
		//   adding or removing a specific textstyle.
		case P_TEXT_STYLE_ADD:
		case P_TEXT_STYLE_REMOVE:
			if (!getflag(F_HAS_FSTYLE))
				atts -> fontstyle = parent -> getparent() -> gettextstyle();
			flags |= F_HAS_FSTYLE;
			MCF_changetextstyle(atts -> fontstyle, (Font_textstyle)(intptr_t)value, which == P_TEXT_STYLE_ADD);
			break;
		}

		// MW-2012-02-13: [[ Block Unicode ]] If we are open then make sure the 'font' ref
		//   is up to date.
		// MW-2012-02-14: [[ FontRefs ]] We've updated the font attrs, so make sure we call
		//   recomputefonts to apply the changes to the fontref (only if opened though).
		if (opened)
			recomputefonts(parent -> getparent() -> getfontref());
	}

	// MW-2012-02-17: [[ SplitTextAttrs ]] If we no longer have any atts, delete the struct.
	if ((flags & F_HAS_ATTS) == 0)
	{
		delete atts;
		atts = nil;
	}
}

Boolean MCBlock::getshift(int2 &out)
{
	if (!(flags & F_HAS_SHIFT))
		return False;
	out = atts->shift;
	return True;
}

void MCBlock::setshift(int2 in)
{
	if (in == 0)
		flags &= ~F_HAS_SHIFT;
	else
	{
		if (atts == NULL)
			atts = new Blockatts;
		atts->shift = in;
		flags |= F_HAS_SHIFT;
	}
}

Boolean MCBlock::getcolor(const MCColor *&outcolor)
{
	if (!(flags & F_HAS_COLOR))
		return False;
	outcolor = atts->color;
	return True;
}

Boolean MCBlock::getbackcolor(const MCColor *&outcolor)
{
	if (!(flags & F_HAS_BACK_COLOR))
		return False;
	outcolor = atts->backcolor;
	return True;
}

void MCBlock::setcolor(const MCColor *newcolor)
{
	if (newcolor == NULL)
	{
		if (flags & F_HAS_COLOR)
		{
			delete atts->color;
			flags &= ~F_HAS_COLOR;
		}
	}
	else
	{
		if (atts == NULL)
			atts = new Blockatts;
		if (!(flags & F_HAS_COLOR))
			atts->color = new MCColor;
		*atts->color = *newcolor;
		flags |= F_HAS_COLOR;
	}
}

void MCBlock::setbackcolor(const MCColor *newcolor)
{
	if (newcolor == NULL)
	{
		if (flags & F_HAS_BACK_COLOR)
		{
			delete atts->backcolor;
			flags &= ~F_HAS_BACK_COLOR;
		}
	}
	else
	{
		if (atts == NULL)
			atts = new Blockatts;
		if (!(flags & F_HAS_BACK_COLOR))
			atts->backcolor = new MCColor;
		*atts->backcolor = *newcolor;
		flags |= F_HAS_BACK_COLOR;
	}
}

void MCBlock::setindex(const char *sptr, uint2 i, uint2 l)
{
	index = i;
	size = l;
	width = 0;
	if (textstrchr(&sptr[index], size, '\t') != NULL)
		flags |= F_HAS_TAB;
	else
		flags &= ~F_HAS_TAB;
	if (flags & F_HAS_IMAGE && size == 0)
		freeatts();
}

void MCBlock::moveindex(const char *sptr, int2 ioffset, int2 loffset)
{
	index += ioffset;
	if (loffset != 0)
	{
		size += loffset;
		width = 0;
		if (textstrchr(&sptr[index], size, '\t'))
			flags |= F_HAS_TAB;
		else
			flags &= ~F_HAS_TAB;
		if (flags & F_HAS_IMAGE && size == 0)
			freeatts();
	}
}

uint2 MCBlock::getcursorx(int2 x, uint2 fi)
{
	uint2 j = fi - index;
	if (j > size)
		j = size;
	return getsubwidth(NULL, x, index, j);
}

uint2 MCBlock::getcursorindex(int2 x, int2 cx, Boolean chunk, Boolean last)
{
	// MW-2007-07-05: [[ Bug 5099 ]] If we have an image and are unicode, the char
	//   we replace is two bytes long
	if (flags & F_HAS_IMAGE && atts->image != NULL)
		if (chunk || cx < atts->image->getrect().width >> 1)
			return index;
		else
			return index + (flags & F_HAS_UNICODE ? 2 : 1);

	uint2 i = index;
	const char *text = parent->gettext();
	int2 cwidth;
	uint2 tlen = 0;
	uint2 twidth = 0;
	uint2 toldwidth = 0;
#ifdef _IOS_MOBILE
	// MW-2012-02-01: [[ Bug 9982 ]] iOS uses sub-pixel positioning, so make sure we measure
	//   complete runs.
	int32_t t_last_width;
	t_last_width = 0;
	while(i < index + size)
	{		
		int32_t t_new_i;
		t_new_i = i + indexincrement(i);
		
		int32_t t_new_width;
		t_new_width = getcursorx(x, t_new_i);
		
		int32_t t_pos;
		if (chunk)
			t_pos = t_new_width;
		else
			t_pos = (t_last_width + t_new_width) / 2;
			
		if (cx < t_pos)
			break;
			
		t_last_width = t_new_width;
		
		i = t_new_i;
	}
#else
	while (i < index + size)
	{
		if (textcomparechar(&text[i],'\t'))
			cwidth = gettabwidth(x, text, i);
		else
#if defined(_MACOSX)
			if (hasunicode())
			{
				tlen += indexincrement(i);
				twidth = MCFontMeasureText(m_font, &text[index], tlen, hasunicode());
				cwidth = twidth - toldwidth;
				toldwidth = twidth;
			}
			else
#endif
				cwidth = MCFontMeasureText(m_font, &text[i], indexincrement(i), hasunicode());
		if (chunk)
		{
			if (cx < cwidth)
				break;
		}
		else
			if (cx < cwidth >> 1)
				break;
		cx -= cwidth;
		x += cwidth;
		i +=  indexincrement(i);
	}
#endif
	if (i == index + size && last && (index + size != parent->gettextsize()) && !hasunicode())
		return i - indexdecrement(i);
	else
		return i;
}

uint2 MCBlock::getsubwidth(MCDC *dc, int2 x, uint2 i, uint2 l)
{
	if (l == 0)
		return 0;
	if (flags & F_HAS_IMAGE && atts->image != NULL)
		return atts->image->getrect().width;
	else
	{
		const char *tptr = parent->gettext();
		const char *sptr = parent->gettext() + i;
		
		// MW-2012-02-12: [[ Bug 10662 ]] If the last char is a VTAB then ignore it.
		if (textcomparechar(sptr + l - getcharsize(), 11))
			l -= getcharsize();

		// MW-2012-08-29: [[ Bug 10325 ]] Use 32-bit int to compute the width, then clamp
		//   to 65535 - this means that we force wrapping when the line is too long.
		// MW-2013-08-08: [[ Bug 10654 ]] Make sure we use a signed integer here, otherwise
		//   we get incorrect clamping when converted to unsigned.
		int4 twidth = 0;
		if (flags & F_HAS_TAB)
		{
			const char *eptr;
			while ((eptr = textstrchr(sptr, l, '\t')) != NULL)
			{
				uint2 sl = eptr - sptr;
				if (l < sl)
					break;
				if (dc == NULL)
					twidth += MCFontMeasureText(m_font, sptr, sl, hasunicode());
				else
					twidth += MCFontMeasureText(m_font, sptr, sl, hasunicode());

				twidth += gettabwidth(x + twidth, tptr, eptr - tptr);

				// Adjust for the tab character.
				sl += indexincrement(eptr - tptr);

				sptr += sl;
				l -= sl;
			}
		}
		if (dc == NULL)
			return MCU_min(65535, twidth + MCFontMeasureText(m_font, sptr, l, hasunicode()));
		else
			return MCU_min(65535, twidth + MCFontMeasureText(m_font, sptr, l, hasunicode()));
	}
}

uint2 MCBlock::getwidth(MCDC *dc, int2 x)
{
	if (flags & F_HAS_IMAGE && atts->image != NULL)
		return atts->image->getrect().width;
	else if (dc != NULL && dc -> gettype() == CONTEXT_TYPE_PRINTER)
		return getsubwidth(dc, x, index, size);
	else if (width == 0 || flags & F_HAS_TAB)
		return width = getsubwidth(dc, x, index, size);
	else
		return width;
}

void MCBlock::reset()
{
	width = 0;
}

void MCBlock::getindex(uint2 &i, uint2 &l)
{
	i = index;
	l = size;
}

uint2 MCBlock::getascent(void)
{
	int2 shift = flags & F_HAS_SHIFT ? atts->shift : 0;
	// MW-2007-07-05: [[ Bug 1943 ]] - Images do not have correct ascent height *MIGHT NEED REVERSION*
	if (flags & F_HAS_IMAGE && atts->image != NULL)
		return MCU_max(0, atts->image->getrect().height - shift + 2);
	else
		return MCU_max(0, heightfromsize(MCFontGetAscent(m_font)) - MCFontGetDescent(m_font) - shift);
}

uint2 MCBlock::getdescent(void)
{
	int2 shift = flags & F_HAS_SHIFT ? atts->shift : 0;
	if (flags & F_HAS_IMAGE && atts->image != NULL)
		return MCU_max(0, shift);
	else
		return MCU_max(0, MCFontGetDescent(m_font) + shift);
}

void MCBlock::freeatts()
{
	freerefs();
	// MW-2012-02-17: [[ SplitTextAttrs ]] Free the fontname name if we have that attr.
	if (flags & F_HAS_FNAME)
		MCNameDelete(atts -> fontname);
	if (flags & F_HAS_COLOR)
		delete atts->color;
	if (flags & F_HAS_BACK_COLOR)
		delete atts->backcolor;
	delete atts;
	atts = NULL;
	flags &= ~F_HAS_ATTS;
}

void MCBlock::freerefs()
{
	// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
	if (flags & F_HAS_LINK)
	{
		MCNameDelete(atts -> linktext);
		atts -> linktext = nil;
	}

	if (flags & F_HAS_IMAGE)
	{
		if (opened)
			closeimage();

		// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
		MCNameDelete(atts -> imagesource);
		atts -> imagesource = nil;
	}

	// MW-2012-01-06: [[ Block Metadata ]] If we have metadata, delete it.
	if (flags & F_HAS_METADATA)
	{
		MCNameDelete(atts -> metadata);
		atts -> metadata = nil;
	}

	flags &= ~(F_HAS_LINK | F_HAS_IMAGE | F_HAS_METADATA);
}

void MCBlock::openimage()
{
	if (flags & F_HAS_IMAGE)
	{
		MCField *t_field;
		t_field = parent->getparent();
		// MW-2009-02-02: [[ Improved image search ]]
		// Search for the appropriate image object using the standard method - here we
		// use the field as the starting point.
		// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
		uint4 t_image_id;
		if (MCU_stoui4(MCNameGetOldString(atts->imagesource), t_image_id))
			atts -> image = t_field -> resolveimageid(t_image_id);
		else
			atts->image = (MCImage *)t_field->getstack()->getobjname(CT_IMAGE, MCNameGetOldString(atts->imagesource));

		if (atts->image != NULL)
		{
			Boolean ob = MCbufferimages;
			MCbufferimages = True;
			atts->image->open();
			MCbufferimages = ob;
			atts->image->addneed(t_field);
		}
	}
}

void MCBlock::closeimage()
{
	if (flags & F_HAS_IMAGE && atts->image != NULL)
	{
		atts->image->close();
		//atts->image->removeneed(this);
	}
}


const char *MCBlock::getlinktext()
{
	// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
	// MW-2012-10-01: [[ Bug 10178 ]] Added guard to ensure we don't get a null
	//   dereference.
	if (flags & F_HAS_LINK && atts != nil)
		return MCNameGetCString(atts->linktext);
	else
		return NULL;
}

const char *MCBlock::getimagesource()
{
	// MW-2012-01-06: [[ Block Changes ]] Change linktext/imagesource to be names.
	// MW-2012-10-01: [[ Bug 10178 ]] Added guard to ensure we don't get a null
	//   dereference.
	if (flags & F_HAS_IMAGE && atts != nil)
		return MCNameGetCString(atts->imagesource);
	else
		return NULL;
}

// MW-2012-01-06: [[ Block Metadata ]] This method returns the metadata of the
//   block if set, otherwise nil.
const char *MCBlock::getmetadata(void)
{
	// MW-2012-10-01: [[ Bug 10178 ]] Added guard to ensure we don't get a null
	//   dereference.
	if (flags & F_HAS_METADATA && atts != nil)
		return MCNameGetCString(atts -> metadata);

	return nil;
}

void MCBlock::sethilite(Boolean on)
{
	if (on)
		flags |= F_HILITED;
	else
		flags &= ~F_HILITED;
}

// Return how many bytes are required to increment an index by n characters.
uint2 MCBlock::indexincrement(uint2 tindex)
{
	// If no unicode flag, then chars == characters
	if ((flags & F_HAS_UNICODE) == 0)
		return 1;

	// Adjust index so it starts on a unicode char...
	uint2 t_this_index;
	t_this_index = tindex;
	if (((tindex - index) & 1) != 0)
		t_this_index -= 1;

	// If we only have a partial unicode char, then return 1
	if ((index + size - t_this_index) < 2)
		return 1;

	// Make sure we have the text pointer.
	const char *thetext;
	thetext = parent->gettext();

	// Fetch the char
	uint16_t *t_chars;
	t_chars = (uint16_t *)(thetext + t_this_index);

	// If it isn't a high surrogate, or this is the last char in
	// the block, or the next char is not a low surrogate, 
	// advance to end of this char.
	if (!MCUnicodeCodepointIsHighSurrogate(t_chars[0]) ||
		(index + size - t_this_index) < 4 ||
		!MCUnicodeCodepointIsLowSurrogate(t_chars[1]))
		return (t_this_index + 2) - tindex;

	// Otherwise advance to the end of the pair
	return (t_this_index + 4) - tindex;
}

//return how many bytes are required to increment an index by n characters
uint2 MCBlock::indexdecrement(uint2 tindex)
{
	// If the current index is at the start or before this block then
	// special case.
	if (tindex <= index)
	{
		// If we are at zero index, do nothing.
		if (index == 0)
			return 1;

		// Otherwise handle in the previous block
		MCBlock *t_prev;
		t_prev = prev();
		if (t_prev != this)
			return t_prev -> indexdecrement(tindex);

		return 1;
	}

	// If no unicode flag, then chars == characters
	if ((flags & F_HAS_UNICODE) == 0)
		return 1;

	// Adjust index so it starts on a unicode char before tindex
	uint2 t_this_index;
	t_this_index = tindex;
	if (((tindex - index) & 1) != 0)
		t_this_index -= 1;
	else
		t_this_index -= 2;

	// If we only have a partial unicode char, then return 1
	if ((index + size - t_this_index) < 2)
		return 1;

	// Make sure we have the text pointer.
	const char *thetext;
	thetext = parent->gettext();

	// Fetch the char pointer
	uint16_t *t_chars;
	t_chars = (uint16_t *)(thetext + t_this_index);

	// If this is a low surrogate, or this is the first char
	// in the block, or the previous char is not a high surrogate,
	// advance to the beginning of this card.
	if (!MCUnicodeCodepointIsLowSurrogate(t_chars[0]) ||
		(t_this_index - index) < 2 ||
		!MCUnicodeCodepointIsHighSurrogate(t_chars[-1]))
		return tindex - t_this_index;

	return tindex - (t_this_index - 2);
}

Boolean MCBlock::textcomparechar(const char *thetext, char thechar)
{
	if (!size)
		return False;
	return MCU_comparechar(thetext, thechar, (flags & F_HAS_UNICODE) != 0);
}

char *MCBlock::textstrchr(const char *sptr,  uint2 l, char target)
{
	const char *eptr = (char *)sptr;
	if (!sptr)
		return NULL;
	uint4 len = l;
	if (!l)
		return NULL;
	if (MCU_strchr(eptr, len, target, (flags & F_HAS_UNICODE) != 0))
		return (char *)eptr;
	return NULL;
}

Boolean MCBlock::textisspace(const char *textptr)
{
	if (!size)
		return False;
	if (flags & F_HAS_UNICODE)
#if defined(_WINDOWS)
		return iswspace(*(uint2 *)textptr) != 0;
#else
		return textcomparechar(textptr, ' ');
#endif
	else
		return isspace((uint1)*textptr) != 0;
}

Boolean MCBlock::textispunct(const char *textptr)
{
	if (!size)
		return False;
	if (flags & F_HAS_UNICODE)
	{
		const uint2 *uchar = (const uint2 *)textptr;
		if (*uchar < MAXUINT1)
		{
			return MCU_ispunct((uint1)*uchar);
		}
		else
#if defined(_WINDOWS)
			return iswpunct(*uchar) != 0;
#else
			return False;
#endif
	}
	else
		return MCU_ispunct((uint1)*textptr);
}

uint2 MCBlock::getcharsize()
{
	return MCU_charsize((flags & F_HAS_UNICODE) != 0);
}

uint2 MCBlock::indextocharacter(uint2 si)
{
	if (flags & F_HAS_UNICODE)
		return si >> 1;
	else
		return si + index;
}

uint2 MCBlock::verifyindex(uint2 si, bool p_is_end)
{
	si -= index;
	if (flags & F_HAS_UNICODE && si & 0x1)
	{
		if (p_is_end)
			return index + si + 1;
		else
			return index + si - 1;
	}
	else
		return index + si;
}

uint2 MCBlock::getlength()
{
	if (flags & F_HAS_UNICODE)
		return size >> 1;
	else
		return size;
}

uint4 MCBlock::getcharatindex(int4 p_index)
{
	MCBlock *t_block;
	t_block = this;

	while(p_index >= t_block -> index + t_block -> size)
	{
		t_block = t_block -> next();
		if (t_block == parent -> getblocks())
			return 0xffffffff;
	}

	while(p_index < t_block -> index)
	{
		t_block = t_block -> prev();
		if (t_block == parent -> getblocks() -> prev())
			return 0xffffffff;
	}
	
	
	// MW-2008-07-25: [[ Bug 6830 ]] The code to fetch the correct unicode character
	//   was previously completely wrong. I think its correct now...
	if (getflag(F_HAS_UNICODE))
	{
		if (t_block -> index + t_block -> size - p_index >= 2)
			return *((uint2 *)(parent -> gettext() + p_index));
		
		return parent -> gettext()[p_index];
	}

	// MW-2008-07-21: [[ Bug 6817 ]] Make sure we map the native encoded character
	//   to a Unicode codepoint.
	return MCUnicodeMapFromNative(parent -> gettext()[p_index]);
}

MCBlock *MCBlock::retreatindex(uint2& p_index)
{
	MCBlock *t_block;
	t_block = this;
	
	if (p_index == 0)
		return NULL;
	
	// MW-2012-08-29: [[ Bug 10322 ]] If we are at the start of the block, then we must
	//   move back a block before doing anything.
	if (p_index == index)
	{
		do
		{
			t_block = t_block -> prev();
		}
		while(t_block -> size == 0 && t_block -> prev() != parent -> getblocks() -> prev());
	}
		
	if (t_block -> getflag(F_HAS_UNICODE))
		p_index -= 2;
	else
		p_index -= 1;
	
	// MW-2012-03-10: [[ Bug ]] Loop while the block is empty, or the block doesn't
	//   contain the index.
	while(p_index < t_block -> index || t_block -> size == 0)
	{
		t_block = t_block -> prev();
		if (t_block == parent -> getblocks() -> prev())
			return NULL;
	}

	return t_block;
}

MCBlock *MCBlock::advanceindex(uint2& p_index)
{
	MCBlock *t_block;
	t_block = this;
	
	// MW-2012-08-29: [[ Bug 10322 ]] If we are at the end of the block, then we must
	//   move forward a block before doing anything.
	if (p_index == index + size)
	{
		do
		{
			t_block = t_block -> next();
		}
		while(t_block -> size == 0 && t_block -> next() != parent -> getblocks());
	}
	
	if (t_block -> getflag(F_HAS_UNICODE) && p_index < index + size - 1)
		p_index += 2;
	else
		p_index += 1;

	// MW-2012-03-10: [[ Bug ]] Loop while the block is empty, or the block doesn't
	//   contain the index.
	while(p_index >= t_block -> index + t_block -> size || t_block -> size == 0)
	{
		t_block = t_block -> next();
		if (t_block == parent -> getblocks())
			return NULL;
	}

	return t_block;
}

// MW-2012-02-21: [[ FieldExport ]] This method updates the given style struct with
//   settings from this block.
void MCBlock::exportattrs(MCFieldCharacterStyle& x_style)
{
	if (getflag(F_HAS_COLOR))
	{
		if (!opened)
			MCscreen -> alloccolor(*atts -> color);

		x_style . has_text_color = true;
		x_style . text_color = atts -> color -> pixel;
	}
	if (getflag(F_HAS_BACK_COLOR))
	{
		if (!opened)
			MCscreen -> alloccolor(*atts -> backcolor);

		x_style . has_background_color = true;
		x_style . background_color = atts -> backcolor -> pixel;
	}
	if (getflag(F_HAS_LINK))
	{
		x_style . has_link_text = true;
		x_style . link_text = atts -> linktext;
	}
	if (getflag(F_HAS_IMAGE))
	{
		x_style . has_image_source = true;
		x_style . image_source = atts -> imagesource;
	}
	if (getflag(F_HAS_METADATA))
	{
		x_style . has_metadata = true;
		x_style . metadata = atts -> metadata;
	}
	if (getflag(F_HAS_FNAME))
	{
		x_style . has_text_font = true;
		x_style . text_font = atts -> fontname;
	}
	if (getflag(F_HAS_FSTYLE))
	{
		x_style . has_text_style = true;
		x_style . text_style = atts -> fontstyle;
	}
	if (getflag(F_HAS_FSIZE))
	{
		x_style . has_text_size = true;
		x_style . text_size = atts -> fontsize;
	}
	if (getflag(F_HAS_SHIFT))
	{
		x_style . has_text_shift = true;
		x_style . text_shift = atts -> shift;
	}
}

// MW-2012-03-04: [[ FieldImport ]] This method updates the block's attributes to
//   those described by the style struct.
void MCBlock::importattrs(const MCFieldCharacterStyle& p_style)
{
	if (p_style . has_text_color)
	{
		MCColor t_color;
		t_color . pixel = p_style . text_color;
		MCscreen -> querycolor(t_color);
		setcolor(&t_color);
	}
	if (p_style . has_background_color)
	{
		MCColor t_color;
		t_color . pixel = p_style . background_color;
		MCscreen -> querycolor(t_color);
		setbackcolor(&t_color);
	}
	if (p_style . has_link_text)
		setatts(P_LINK_TEXT, (void *)MCNameGetCString(p_style . link_text));
	if (p_style . has_image_source)
		setatts(P_IMAGE_SOURCE, (void *)MCNameGetCString(p_style . image_source));
	if (p_style . has_metadata)
		setatts(P_METADATA, (void *)MCNameGetCString(p_style . metadata));
	if (p_style . has_text_font)
		setatts(P_TEXT_FONT, (void *)MCNameGetCString(p_style . text_font));
	if (p_style . has_text_style)
		setatts(P_TEXT_STYLE, (void *)p_style . text_style);
	if (p_style . has_text_size)
		setatts(P_TEXT_SIZE, (void *)p_style . text_size);
	// MW-2012-05-09: [[ Bug ]] Setting the textShift of a block is done with 'setshift'
	//   not 'setatts'.
	if (p_style . has_text_shift)
		setshift(p_style . text_shift);
}

// MW-2012-03-04: [[ StackFile5500 ]] Utility routine for computing the length of
//   a nameref when serialized to disk.
uint32_t measure_nameref(MCNameRef p_name)
{
	const char *t_cstring;
	t_cstring = MCNameGetCString(p_name);
	
	// If the cstring is nil, then it's just a size field.
	if (t_cstring == nil)
		return 2;
		
	// Otherwise its the length of it nul terminated plus the 2 byte size.
	return 2 + MCU_min(strlen(t_cstring) + 1, MAXUINT2);
}

// MW-2012-03-04: [[ StackFile5500 ]] Compute the number of bytes the attributes will
//   take up when serialized.
uint32_t MCBlock::measureattrs(void)
{
	// If there are no attrs, then the size is 0.
	if (!hasatts())
		return 0;

	uint32_t t_size;
	t_size = 0;
	
	// The flags field.
	t_size = 4;
	// The font index (if any)
	if ((flags & (F_FATTR_MASK | F_HAS_UNICODE)) != 0)
		t_size += 2;
	if ((flags & F_HAS_COLOR) != 0)
		t_size += 6;
	if ((flags & F_HAS_BACK_COLOR) != 0)
		t_size += 6;
	if ((flags & F_HAS_SHIFT) != 0)
		t_size += 2;
	if ((flags & F_HAS_LINK) != 0)
		t_size += measure_nameref(atts -> linktext);
	if ((flags & F_HAS_IMAGE) != 0)
		t_size += measure_nameref(atts -> imagesource);
	if ((flags & F_HAS_METADATA) != 0)
		t_size += measure_nameref(atts -> metadata);

	return t_size;
}

bool MCBlock::getfirstlinebreak(uint2& r_index)
{
	char *t_break;
	t_break = textstrchr(parent -> gettext() + index, size, '\x0B');
	if (t_break == nil)
		return false;

	r_index = t_break - parent -> gettext();
	advanceindex(r_index);
	
	return true;
}

bool MCBlock::imagechanged(MCImage *p_image, bool p_deleting)
{
	if ((flags & F_HAS_IMAGE) && atts->image == p_image)
	{
		if (p_deleting)
			atts->image = nil;
		return true;
	}

	return false;
}

