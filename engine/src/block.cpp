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
#include "parsedef.h"
#include "objdefs.h"
#include "mcio.h"
#include "globals.h"

#include "stack.h"
#include "field.h"
#include "image.h"
#include "paragraf.h"
#include "line.h"
#include "MCBlock.h"
#include "util.h"
#include "context.h"
#include "mctheme.h"
#include "font.h"
#include "path.h"
#include "segment.h"

#include "exec-interface.h"

#include "stackfileformat.h"

#define IMAGE_BLOCK_LEADING  2
static MCRectangle
MCBlockMakeRectangle(double x, double y,
                     double width, double height)
{
	MCRectangle p_result;
	p_result.x = int2(x);
	p_result.y = int2(y);
	p_result.width = uint2(width);
	p_result.height = uint2(height);
	return p_result;
}

// Default MCBlock constructor - makes a block with everything initialized
// to zero.
MCBlock::MCBlock(void)
{
	parent = NULL;
	flags = F_CLEAR|F_HAS_UNICODE;
	atts = NULL;
	m_index = m_size = 0;
	width = 0;
	opened = 0;
    origin = 0;
    tabpos = 0;
    direction_level = 0;

    // SN-2014-12-16: [[ Bug 14227 ]] Make sure to initialise the segment to nil.
    segment = nil;

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
		atts = new (nothrow) Blockatts;
		if (flags & F_HAS_COLOR)
		{
			atts->color = new (nothrow) MCColor;
			*atts->color = *bref.atts->color;
		}
		if (flags & F_HAS_BACK_COLOR)
		{
			atts->backcolor = new (nothrow) MCColor;
			*atts->backcolor = *bref.atts->backcolor;
		}

		// MW-2012-02-17: [[ SplitTextAttrs ]] Copy across the font attrs the other
		//   block has.
		if ((flags & F_HAS_FNAME) != 0)
            atts->fontname = MCValueRetain(bref.atts->fontname);
		if ((flags & F_HAS_FSIZE) != 0)
			atts -> fontsize = bref . atts -> fontsize;
		if ((flags & F_HAS_FSTYLE) != 0)
			atts -> fontstyle = bref . atts -> fontstyle;

		atts->shift = bref.atts->shift;

		// MW-2012-05-04: [[ Values ]] linkText / imageSource / metaData are now uniqued
		//   strings.
		if (flags & F_HAS_LINK)
			atts -> linktext = MCValueRetain(bref . atts -> linktext);
		if (flags & F_HAS_IMAGE)
			atts -> imagesource = MCValueRetain(bref . atts -> imagesource);
		if (flags & F_HAS_METADATA)
			atts -> metadata = MCValueRetain(bref . atts -> metadata);
	}
	else
		atts = NULL;
	m_index = bref.m_index;
	m_size = bref.m_size;
	width = 0;
	opened = 0;
    origin = 0;
    tabpos = 0;
    direction_level = bref.direction_level;

    // SN-2014-12-16: [[ Bug 14227 ]] Make sure to initialise the segment to nil.
    segment = nil;

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

bool MCBlock::visit(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor *p_visitor)
{
	return p_visitor -> OnBlock(this);
}

// IM-2016-07-06: [[ Bug 17690 ]] Test if block sizes or offsets require 32bit
//   values to store (stack file format v8.1).
uint32_t MCBlock::getminimumstackfileversion(void)
{
	// paragraph text is always unicode when saving as version 7.0 or greater.
	//    since we can't know which version will be used at this point, the
	//    best we can do is assume unicode text.
	uint32_t t_index_size;
	t_index_size = sizeof(unichar_t);
	
	if (m_index * t_index_size > UINT16_MAX || m_size * t_index_size > UINT16_MAX)
		return kMCStackFileFormatVersion_8_1;
	else
		return kMCStackFileFormatMinimumExportVersion;
}

// MW-2012-03-04: [[ StackFile5500 ]] If 'is_ext' is true then the record is an extended
//   record.
IO_stat MCBlock::load(IO_handle stream, uint32_t version, bool is_ext)
{
	IO_stat stat;

	// MW-2012-03-04: [[ StackFile5500 ]] If this is an extended block, then work out
	//   where to skip to when all the attrs currently recognized have been read.
	int64_t t_attr_end = 0;
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
		return checkloadstat(stat);

	// MW-2012-03-04: [[ StackFile5500 ]] If this isn't an extended block, then strip the
	//   metadata flag.
	if (!is_ext)
		flags &= ~F_HAS_METADATA;
	
	// MW-2012-01-26: [[ FlaggedField ]] Make sure we ignore the setting of FLAGGED flag.
	flags &= ~F_FLAGGED;

	if (atts == NULL)
		atts = new (nothrow) Blockatts;

	// MW-2012-02-17: [[ SplitTextAttrs ]] If the font flag is present, it means there
	//   is a font record to read.
	if (flags & F_FONT)
    {
		if (version > kMCStackFileFormatVersion_1_3)
		{
			uint2 t_font_index;
			if ((stat = IO_read_uint2(&t_font_index, stream)) != IO_NORMAL)
				return checkloadstat(stat);

			// MW-2012-02-17: [[ LogFonts ]] Map the font index we have to the font attrs.
			//   Note that we ignore the 'unicode' tag since that is encoded in flags.
			MCNameRef t_fontname;
			uint2 t_fontstyle, t_fontsize;
			bool t_is_unicode;
			MCLogicalFontTableLookup(t_font_index, t_fontname, t_fontstyle, t_fontsize, t_is_unicode);

			// MW-2012-02-17: [[ SplitTextAttrs ]] Only set the font attrs if they are
			//   not inherited.
			if (!getflag(F_INHERIT_FNAME))
                atts->fontname = MCValueRetain(t_fontname);
			if (!getflag(F_INHERIT_FSIZE))
				atts -> fontsize = t_fontsize;
			if (!getflag(F_INHERIT_FSTYLE))
				atts -> fontstyle = t_fontstyle;
		}
		else
		{
			// MW-2012-02-17: [[ LogFonts ]] Read a nameref directly.
			// MW-2013-11-19: [[ UnicodeFileFormat ]] This path only happens sfv < 1300
			//   so is legacy.
			if ((stat = IO_read_nameref_legacy(atts->fontname, stream, false)) != IO_NORMAL)
				return checkloadstat(stat);
			if ((stat = IO_read_uint2(&atts->fontsize, stream)) != IO_NORMAL)
				return checkloadstat(stat);
			if ((stat = IO_read_uint2(&atts->fontstyle, stream)) != IO_NORMAL)
				return checkloadstat(stat);

			// MW-2012-02-17; [[ SplitTextAttrs ]] All the font attrs are set.
			flags |= F_FATTR_MASK;
		}
    }
	if (flags & F_HAS_COLOR)
	{
		atts->color = new (nothrow) MCColor;
		if ((stat = IO_read_mccolor(*atts->color, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if (flags & F_HAS_COLOR_NAME)
		{
			// MW-2012-01-06: [[ Block Changes ]] We no longer use the color name
			//   so load, delete and unset the flag.
			// MW-2013-11-19: [[ UnicodeFileFormat ]] The storage of this is ignored,
			//   so is legacy,
			char *colorname;
			if ((stat = IO_read_cstring_legacy(colorname, stream, 2)) != IO_NORMAL)
				return checkloadstat(stat);
			MCCStringFree(colorname);
			flags &= ~F_HAS_COLOR_NAME;
		}
	}
	if (flags & F_HAS_BACK_COLOR)
	{
		atts->backcolor = new (nothrow) MCColor;
		if ((stat = IO_read_mccolor(*atts->backcolor, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if (version < kMCStackFileFormatVersion_2_0 || flags & F_HAS_BACK_COLOR_NAME)
		{
			// MW-2012-01-06: [[ Block Changes ]] We no longer use the backcolor name
			//   so load, delete and unset the flag.
			// MW-2013-11-19: [[ UnicodeFileFormat ]] The storage of this is ignored,
			//   so is legacy,
			if ((stat = IO_discard_cstring_legacy(stream, 2)) != IO_NORMAL)
				return checkloadstat(stat);
			flags &= ~F_HAS_BACK_COLOR_NAME;
		}
	}
	if (flags & F_HAS_SHIFT)
		if ((stat = IO_read_int2(&atts->shift, stream)) != IO_NORMAL)
			return checkloadstat(stat);

	// MW-2012-05-04: [[ Values ]] linkText / imageSource / metaData are now uniqued
	//   strings.
	if (flags & F_HAS_LINK)
	{
		// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
		if ((stat = IO_read_stringref_new(atts->linktext, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return checkloadstat(stat);
		/* UNCHECKED */ MCValueInterAndRelease(atts -> linktext, atts -> linktext);
	}

	if (flags & F_HAS_IMAGE)
	{
		// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
		if ((stat = IO_read_stringref_new(atts->imagesource, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return checkloadstat(stat);
		/* UNCHECKED */ MCValueInterAndRelease(atts -> imagesource, atts -> imagesource);
	}

	// MW-2012-03-04: [[ StackFile5500 ]] If there is a metadata attr then read
	//   it in.
	if (flags & F_HAS_METADATA)
	{
		// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
		if ((stat = IO_read_stringref_new(atts->metadata, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return checkloadstat(stat);
		/* UNCHECKED */ MCValueInterAndRelease(atts -> metadata, atts -> metadata);
	}

	// MW-2012-03-04: [[ StackFile5500 ]] If this is an extended block, then skip
	//   to the end of the attrs record.
	if (is_ext)
		if ((stat = MCS_seek_set(stream, t_attr_end)) != IO_NORMAL)
			return checkloadstat(stat);
	
	// ***** IMPORTANT *****
	// The "index" and "size" values loaded below are byte indices into the
	// parent paragraph's text buffer and are almost certainly wrong if the
	// paragraph contains any Unicode text.
	//
	// Things are this way to maintain file format compatibility with engines
	// that do not support the full Unicode refactor.
	//
	// Helpfully, the paragraph loading code makes a SetRanges call to inform
	// the block of the correct offsets as soon as it knows them.

	// IM-2016-07-11: [[ Bug 17690 ]] change storage format for index & size from
	//   16bit to 32bit in stack file format v8.1.
	if (version >= kMCStackFileFormatVersion_8_1)
	{
		uint32_t t_index, t_size;
		stat = IO_read_uint4(&t_index, stream);
		if (stat != IO_NORMAL)
			return checkloadstat(stat);
		
		stat = IO_read_uint4(&t_size, stream);
		if (stat != IO_NORMAL)
			return checkloadstat(stat);
		
		m_index = t_index;
		m_size = t_size;
	}
	else
	{
		uint2 index, size;
		if ((stat = IO_read_uint2(&index, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if ((stat = IO_read_uint2(&size, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		m_index = index;
		m_size = size;
	}

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

IO_stat MCBlock::save(IO_handle stream, uint4 p_part, uint32_t p_version)
{
	IO_stat stat;

	// MW-2012-03-04: [[ StackFile5500 ]] If the block has metadata and 5.5 stackfile
	//   format has been requested then this is an extended block.
	bool t_is_ext;
	if (p_version >= kMCStackFileFormatVersion_5_5 && getflag(F_HAS_METADATA))
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
		if ((stat = IO_write_uint2or4(measureattrs(p_version), stream)) != IO_NORMAL)
			return stat;

	uint4 oldflags = flags;

	// MW-2012-01-06: [[ StackFile5500 ]] If this isn't an extended block, then remove
	//   the metadata flag.
	if (!t_is_ext)
		flags &= ~F_HAS_METADATA;
		
	flags &= ~F_VISITED;
	// MW-2012-01-26: [[ FlaggedField ]] Make sure we don't save the flagged flag.
	flags &= ~F_FLAGGED;
	
    // The "has unicode" flag depends on whether the paragraph is native
	bool t_is_unicode;
    if (p_version < kMCStackFileFormatVersion_7_0 && MCStringIsNative(parent->GetInternalStringRef()))
	{
		t_is_unicode = false;
        flags &= ~F_HAS_UNICODE;
	}
    else
	{
		t_is_unicode = true;
        flags |= F_HAS_UNICODE;
	}

    // SN-2014-12-04: [[ Bug 14149 ]] Add the F_HAS_TAB flag, for legacy saving
    if (p_version < kMCStackFileFormatVersion_7_0)
    {
        if (segment && segment != segment -> next())
            flags |= F_HAS_TAB;
    }
	
	// MW-2012-02-17: [[ SplitTextAttrs ]] If we have unicode, or one of the font attr are
	//   set then we must serialize a font.
	bool t_need_font;
	if ((flags & (F_FATTR_MASK | F_HAS_UNICODE)) != 0)
	{
		// Add in the font record flag.
		flags |= F_FONT;

		// Invert the font attr flags (from has to inherit).
		flags = (flags & ~F_FATTR_MASK) | (~(flags & F_FATTR_MASK) & F_FATTR_MASK);
		
		t_need_font = true;
	}
	else
		t_need_font = false;
    
	if ((stat = IO_write_uint4(flags, stream)) != IO_NORMAL)
		return stat;
		
	flags = oldflags;

	// MW-2012-02-17: [[ SplitTextAttrs ]] If any one of the font attrs are set, or we
	//   are unicode we must serialize a font.
	if (t_need_font)
	{
		// MW-2012-02-17: [[ SplitTextAttrs ]] Compute the attrs to write out.
		MCNameRef t_fontname;
		uint2 t_fontstyle, t_fontsize;
		getfontattrs(nil, t_fontname, t_fontsize, t_fontstyle);

		// MW-2012-02-17: [[ LogFonts ]] Map the font attrs to the appropriate font
		//   index and write.
		uint2 t_font_index;
		t_font_index = MCLogicalFontTableMap(t_fontname, t_fontstyle, t_fontsize, true);
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

	// MW-2012-05-04: [[ Values ]] linkText / imageSource / metaData are now uniqued
	//   strings.
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
    if (flags & F_HAS_LINK)
        if ((stat = IO_write_stringref_new(atts->linktext, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return stat;
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
    if (flags & F_HAS_IMAGE)
        if ((stat = IO_write_stringref_new(atts->imagesource, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return stat;
	
	// MW-2012-03-04: [[ StackFile5500 ]] If this is an extended block then emit the
	//   new attributes.
	if (t_is_ext)
	{
		// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
        if (flags & F_HAS_METADATA)
            if ((stat = IO_write_stringref_new(atts -> metadata, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
				return stat;
	}
	
	uint32_t t_index_size;
	t_index_size = t_is_unicode ? sizeof(unichar_t) : sizeof(char_t);
	
	// IM-2016-07-11: [[ Bug 17690 ]] change storage format for index & size from
	//   16bit to 32bit in stack file format v8.1.
	if (p_version >= kMCStackFileFormatVersion_8_1)
	{
		stat = IO_write_uint4(m_index * t_index_size, stream);
		if (stat != IO_NORMAL)
			return checkloadstat(stat);
		
		stat = IO_write_uint4(m_size * t_index_size, stream);
		if (stat != IO_NORMAL)
			return checkloadstat(stat);
	}
	else
	{
		if ((stat = IO_write_uint2(m_index * t_index_size, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint2(m_size * t_index_size, stream)) != IO_NORMAL)
			return stat;
	}

	return IO_NORMAL;
}

void MCBlock::open(MCFontRef p_parent_font)
{
	if (opened++ != 0)
		return;

	// MW-2012-02-14: [[ FontRefs ]] Map the font for the block.
	mapfont(p_parent_font);

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
	// we do a pointer comparison - the metadata fields are unique values so this
	// is sufficient to check for equality.
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

    // Ensure that the direction level matches
    if (direction_level != bptr -> direction_level)
        return False;
    
	// Everything matches, so these two blocks must be the same.
	return True;
}

static bool MCUnicodeCanBreakBetween(uint2 x, uint2 y)
{
	// MW-2013-12-19: [[ Bug 11606 ]] We only check for breaks between chars and spaces
	//   where the space follows the char. This is because a break will consume all space
	//   chars after it thus we want to measure up to but not including the spaces.
	bool t_x_isspace, t_y_isspace;
    // AL-2014-09-03: [[ Bug 13332 ]] Don't break before a non-breaking space
    t_x_isspace = MCUnicodeIsWhitespace(x);
    t_y_isspace = y != 0x00A0 && MCUnicodeIsWhitespace(y);

	if (t_x_isspace && t_y_isspace)
		return false;
	if (t_y_isspace)
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

bool MCBlock::fit(coord_t x, coord_t maxwidth, findex_t& r_break_index, bool& r_break_fits)
{
	// If the block of zero length, then it can't be broken and must be kept with previous blocks.
	if (m_size == 0)
	{
		if (next() != parent -> getblocks())
			r_break_fits = false;
		else
			r_break_fits = true;
		r_break_index = m_index;
		return true;
	}
			
	if ((flags & F_HAS_IMAGE) && atts -> image != NULL)
	{
		r_break_index = m_index + m_size;
		r_break_fits = getwidth(NULL, x) <= maxwidth;
		return r_break_fits;
	}

	// Fetch the first character of the next block
	codepoint_t t_next_block_char;
	MCBlock *t_next_block;
	t_next_block = next();
	if (t_next_block != parent -> getblocks())
	{
		if (t_next_block -> GetLength() == 0)
			t_next_block_char = CODEPOINT_NONE-1;
		else
			t_next_block_char = parent->GetCodepointAtIndex(t_next_block -> m_index);
	}
	else
		t_next_block_char = CODEPOINT_NONE;

    // FG-2013-10-21 [[ Field speedups ]]
    // Previously, we used to calculate the length of the entire block here in order
    // to determine if splitting was required. Unfortunately, this tends to bypass
    // the text layout cache resulting in very slow laying out now that the "proper"
    // Unicode text layout APIs are in use.
    //
    // Now, all text is laid out word-at-a-time.
	
	// We don't completely fit within maxwidth, so compute the last break point in
	// the block by measuring
	// MW-2013-12-19: [[ Bug 11606 ]] Track the width of the text within the block as a float
	//   but use the integer width to break. This ensures measure(a & b) == measure(a) + measure(b)
	//   (otherwise you get drift as the accumulated width the block calculates is different
	//    from the width of the text that is drawn). 

	coord_t t_width_float;
	t_width_float = 0;

	findex_t i;
	i = m_index;
	
	codepoint_t t_next_char;
	t_next_char = parent->GetCodepointAtIndex(i);
	
	findex_t t_break_index;
	t_break_index = m_index;

	bool t_can_fit;
	t_can_fit = false;
	
	// MW-2013-08-01: [[ Bug 10932 ]] Optimized loop only measuring between potential break
	//   points, rather than char by char.
    bool t_whole_block;
    t_whole_block = false;
	while(i < m_index + m_size)
	{
		findex_t initial_i;
		initial_i = i;
		
		bool t_can_break;
		t_can_break = false;
		
		codepoint_t t_this_char;
        bool t_end_of_block;
        t_end_of_block = false;
        
        // If this is the first block of a segment that was created by a tab,
        // we can use the first position in the block as a break position,
        // unless this is the first segment on a line.
        if (!t_can_fit
            && this == segment->GetFirstBlock()
            && i == m_index
            && segment->GetParent()->GetFirstSegment() != segment
            && parent->GetCodepointAtIndex(i - 1) == '\t')
        {
            t_can_fit = t_can_break = true;
        }
        else
        {
            while(i < m_index + m_size)
            {
                t_this_char = t_next_char;
                
                i = parent->IncrementIndex(i);
            
                if (i < m_index + m_size)
                    t_next_char = parent->GetCodepointAtIndex(i);
                else
                {
                    t_next_char = t_next_block_char;
                    t_end_of_block = true;
                }
                
                if (t_next_char == CODEPOINT_NONE ||
                    MCUnicodeCanBreakBetween(t_this_char, t_next_char))
                {
                    t_can_break = true;
                    break;
                }
            }
		}

		// MW-2013-11-07: [[ Bug 11393 ]] Previous per-platform implementations all fold into the optimized
		//   case now (previously iOS / Windows printer were measuring break by break, which is what we do
		//   generally now).
        // FG-2014-04-30: [[ TabAlignments ]] Blocks no longer contain tabs
		/*if (t_this_char == '\t')
		{
			twidth += gettabwidth(x + twidth, initial_i);
			twidth_float = (MCGFloat)twidth;

			t_last_break_width = twidth;
			t_last_break_i = i;
		}
		else*/
        {
            MCRange t_range;
            t_range = MCRangeMakeMinMax(initial_i, i);
            // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
            t_width_float += MCFontMeasureTextSubstringFloat(m_font,  parent->GetInternalStringRef(), t_range, parent -> getparent() -> getstack() -> getdevicetransform());
        }
        
		if (t_can_fit && t_width_float > maxwidth)
			break;

		if (t_can_break)
			t_break_index = i;

        if (t_width_float <= maxwidth)
        {
            t_can_fit = true;
            t_whole_block = t_end_of_block;
        }

		if (t_width_float >= maxwidth)
			break;
	}

	// We now have a suitable break point in t_break_index. This could be index if
	// there are none in the block. We now loop forward until we get to the end of
	// any suitable run of spaces.
	while(t_break_index < m_index + m_size && parent->TextIsWordBreak(parent->GetCodepointAtIndex(t_break_index)))
		t_break_index = parent->IncrementIndex(t_break_index);

	r_break_fits = t_can_fit;
	r_break_index = t_break_index;
	
    // If we found a break, was it before the end of the block?
    return t_whole_block;
}

void MCBlock::split(findex_t p_index)
{
	MCBlock *bptr = new (nothrow) MCBlock(*this);
	findex_t newlength = m_size - (p_index - m_index);
	bptr->SetRange(p_index, newlength);
	m_size -= newlength;
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
// FG-2014-04-30: [[ TabAlignments ]] Blocks no longer contain tabs
/*coord_t MCBlock::gettabwidth(coord_t x, findex_t i)
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
		findex_t ctab = 0;
		findex_t cindex = 0;

		// Count the number of preceeding tabs in the paragraph.
		MCBlock *t_block;
		t_block = parent -> getblocks();
		findex_t j;
		j = 0;
		while(j < i)
		{
            if (t_block -> getflag(F_HAS_TAB))
			{
				findex_t k;
				k = t_block -> GetOffset() + t_block -> GetLength();
				while(j < k && j < i)
				{
					if (parent->GetCodepointAtIndex(j) == '\t')
						ctab++;
					j = parent->IncrementIndex(j);
				}
			}
			else
				j = t_block -> GetOffset() + t_block -> GetLength();
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
            // MW-2014-07-29: [[ Bug 12944 ] Make sure we round x to an int, otherwise the
            //   rounding up to nearest tab code becomes a little bit broken.
			if (diff != 0)
				lasttab = tabs[ntabs - 1] + diff * (((int)ceilf(x) - tabs[ntabs - 1]) / diff + 1);
			else
				lasttab = x;
		}

		// Adjust by 1 if the last tab is x
		if (lasttab == x)
			return 1;

		// Return the difference between x and the last tab.
		return lasttab - x;
	}
}*/

// SN-2014-08-13: [[ Bug 13016 ]] Added a parameter for the left of the cell
void MCBlock::drawstring(MCDC *dc, coord_t x, coord_t p_cell_left, coord_t p_cell_right, int2 y, findex_t start, findex_t length, Boolean image, uint32_t style)
{
	// MW-2012-02-16: [[ FontRefs ]] Fetch the font metrics we need to draw.
	coord_t t_ascent, t_descent, t_leading, t_xheight;
	t_ascent = MCFontGetAscent(m_font);
	t_descent = MCFontGetDescent(m_font);
    t_leading = MCFontGetLeading(m_font);
    t_xheight = MCFontGetXHeight(m_font);
    
    // Width for strike-through/underline lines. Factor is arbitrary...
    coord_t t_strikewidth;
    t_strikewidth = ceilf(MCFontGetAscent(m_font)/16);
	
	// MW-2012-01-25: [[ ParaStyles ]] Fetch the vGrid setting from the owning paragraph.
	if (parent -> getvgrid())
	{
        MCRectangle t_cell_clip;
        t_cell_clip = dc->getclip();
        dc -> save();

		findex_t t_index;
		t_index = start;
		while(t_index < start + length)
		{
			uindex_t t_next_tab;
            // FG-2014-04-30: [[ TabAlignments ]] Blocks no longer contain tabs
			/*if (MCStringFirstIndexOfChar(parent->GetInternalStringRef(), '\t', t_index, kMCStringOptionCompareExact, t_next_tab))
            {
                if (t_next_tab >= m_index + m_size)
					t_next_tab = -1;
            }
			else*/
				t_next_tab = UINDEX_MAX;

			findex_t t_next_index;
			if (t_next_tab == UINDEX_MAX)
				t_next_index = start + length;
			else
				t_next_index = t_next_tab;

			//int2 t_tab_width;
			//t_tab_width = 64; //gettabwidth(0, t_index);

			// MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
            // FG-2014-07-16: [[ Bug 12539 ]] Make sure not to draw tab characters
			coord_t t_width;
			MCRange t_range;
			t_range = MCRangeMakeMinMax(t_index, t_next_index);
            if (length > 0 && parent->GetCodepointAtIndex(t_next_index - 1) == '\t')
                t_range.length--;
            t_width = MCFontMeasureTextSubstringFloat(m_font, parent->GetInternalStringRef(), t_range, parent -> getparent() -> getstack() -> getdevicetransform());

			// MW-2012-02-09: [[ ParaStyles ]] Compute the cell clip, taking into account padding.
            // SN-2014-08-14: [[ Bug 13106 ]] Fix for the cell clipping and update to GetInnerWidth
            // SN-2015-09-07: [[ Bug 15273 ]] We clip from the origin of the block
            t_cell_clip . x = p_cell_left + origin - 1;
            // AL-2014-07-29: [[ Bug 12952 ]] Clip to segment boundaries
			t_cell_clip . width = MCU_max(segment -> GetInnerWidth() - origin, 0.0f);

            dc -> cliprect(t_cell_clip);
            dc -> drawtext_substring(x, y, parent->GetInternalStringRef(), t_range, m_font, image == True, kMCDrawTextBreak, is_rtl() ? kMCDrawTextDirectionRTL : kMCDrawTextDirectionLTR);

			// Only draw the various boxes/lines if there is any content.
			if (t_next_index - t_index > 0)
			{
				if ((style & FA_UNDERLINE) != 0)
                {
                    MCRectangle t_underlinerect;
                    t_underlinerect = MCU_make_rect(x, y + t_strikewidth, t_width, t_strikewidth);
                    dc -> fillrect(t_underlinerect);
                }
				if ((style & FA_STRIKEOUT) != 0)
                {
                    MCRectangle t_strikerect;
                    t_strikerect = MCU_make_rect(x, y - (t_xheight / 2) - (t_strikewidth / 2), t_width, t_strikewidth);
                    dc -> fillrect(t_strikerect);
                }
				if ((style & FA_BOX) != 0)
				{
					// MW-2012-09-04: [[ Bug 9759 ]] Adjust any pattern origin to scroll with text.
					parent -> getparent() -> setforeground(dc, DI_BORDER, False, True);
					parent -> getparent() -> adjustpixmapoffset(dc, DI_BORDER);
					MCRectangle trect = MCBlockMakeRectangle(x - 1, y - t_ascent, t_width + 3, t_ascent + t_descent);
					dc->drawrect(trect);
					parent -> getparent() -> setforeground(dc, DI_FORE, False, True);
				}
				else if ((style & FA_3D_BOX) != 0)
				{
					MCRectangle trect = MCBlockMakeRectangle(x - 1, y - t_ascent, t_width + 2, t_ascent + t_descent);
					parent -> getparent() -> draw3d(dc, trect, ETCH_RAISED, 1);
					parent -> getparent() -> setforeground(dc, DI_FORE, False, True);
				}
			}

			x += t_width;

			if (t_next_tab != UINDEX_MAX)
			{
				x = p_cell_right;
				t_next_index = parent->IncrementIndex(t_next_index);
			}

            t_index = t_next_index;
		}

        dc -> restore();
	}
	else
	{
		findex_t sptr;
		findex_t size;
		sptr = start;
		size = length;
		
		// MW-2012-02-21: [[ LineBreak ]] Trim the block slightly if there is an explicit line break
		//   at the end of the block.
		// MW-2013-02-12: [[ Bug 10662 ]] Make sure we take into account unicode chars.
		if (size > 0 && parent->TextIsLineBreak(parent->GetCodepointAtIndex(sptr + size - 1)))
			size -= 1;
		
		// If we need an underline/strikeout then compute the start and width.
		int32_t t_line_width, t_line_x;
		t_line_width = 0;
		t_line_x = x;
		if ((style & (FA_UNDERLINE | FA_STRIKEOUT)) != 0)
			t_line_width = getsubwidth(dc, 0, start, size);
		
        // FG-2014-04-30: [[ TabAlignments ]] Blocks no longer contain tabs
		/*if (flags & F_HAS_TAB)
		{
			uindex_t eptr;
			while (MCStringFirstIndexOfChar(parent->GetInternalStringRef(), '\t', sptr, kMCStringOptionCompareExact, eptr))
			{
				// If beyond this block, ignore
				findex_t l = eptr - sptr;
				if (l >= size)
                    break;

                // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
                coord_t twidth;
				MCRange t_range;
				t_range = MCRangeMake(sptr, l);
                twidth = MCFontMeasureTextSubstringFloat(m_font, parent->GetInternalStringRef(), t_range, parent -> getparent() -> getstack() -> getdevicetransform());
				twidth += gettabwidth(cx + twidth, eptr);

                dc -> drawtext_substring(x, y, parent->GetInternalStringRef(), t_range, m_font, image == True, kMCDrawTextBreak, is_rtl() ? kMCDrawTextDirectionRTL : kMCDrawTextDirectionLTR);

				cx += twidth;
				x += twidth;
// FRAGILE ???
                // Adjust for the tab character.
                eptr = parent->IncrementIndex(eptr);
                findex_t sl = eptr - sptr;

                sptr += sl;
                size -= sl;

                // Adjust for the tab character.
//				l = parent->IncrementIndex(eptr);

//				sptr = l;
//                size = length - l;
			}
		}*/

        // FG-2014-07-16: [[ Bug 12539 ]] Make sure not to draw tab characters
        MCRange t_range;
        t_range = MCRangeMake(sptr, size);
        if (size > 0 && parent->GetCodepointAtIndex(sptr + size - 1) == '\t')
            t_range.length--;
        dc -> drawtext_substring(x, y, parent->GetInternalStringRef(), t_range, m_font, image == True, kMCDrawTextBreak, is_rtl() ? kMCDrawTextDirectionRTL : kMCDrawTextDirectionLTR);

		// Apply strike/underline.
		if ((style & FA_UNDERLINE) != 0)
        {
            MCRectangle t_underlinerect;
            t_underlinerect = MCU_make_rect(t_line_x, y + t_strikewidth, t_line_width, t_strikewidth);
            dc -> fillrect(t_underlinerect);
        }
		if ((style & FA_STRIKEOUT) != 0)
        {
            MCRectangle t_strikerect;
            t_strikerect = MCU_make_rect(t_line_x, y - (t_xheight / 2) - (t_strikewidth / 2), t_line_width, t_strikewidth);
            dc -> fillrect(t_strikerect);
        }
	}
}


// SN-2014-08-13: [[ Bug 13016 ]] Added a parameter for the left of the cell
void MCBlock::draw(MCDC *dc, coord_t x, coord_t lx, coord_t cx, int2 y, findex_t si, findex_t ei, MCStringRef p_string, uint2 pstyle, uint32_t p_border_flags)
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

	// MM-2013-11-05: [[ Bug 11547 ]] We now pack alpha values into pixels meaning we shouldn't check against MAXUNIT4. Not sure why this check was here previously.
	if (flags & F_HAS_COLOR)
		t_foreground_color = atts -> color;

	// MM-2013-11-05: [[ Bug 11547 ]] We now pack alpha values into pixels meaning we shouldn't check against MAXUNIT4. Not sure why this check was here previously.
	if (flags & F_HAS_BACK_COLOR)
		dc->setbackground(*atts->backcolor);

    setcolorfornormaltext(dc, t_foreground_color);
	
	uint32_t t_style;
	t_style = 0;
	if (fontstyle & FA_UNDERLINE || (fontstyle & FA_LINK && ull))
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
	if (ei == si || si >= m_index + m_size || ei <= m_index)
        // SN-2014-08-13: [[ Bug 13016 ]] Added a parameter for the left of the cell
		drawstring(dc, x, lx, cx, y, m_index, m_size, (flags & F_HAS_BACK_COLOR) != 0, t_style);
	else
	{
        // Save the current clip.
        // SN-2014-07-09: [[ MERGE-6.7 ]] Update to the new clipping methods
        dc->save();

        // This will hold the clip for the selection.
        MCRectangle t_sel_clip, t_old_clip;
        t_sel_clip = dc->getclip();
        t_old_clip = t_sel_clip;
		
		// If there is some unselected text at the start of the block, then render it.
		if (si > m_index)
		{
			int2 t_start_dx;
			t_start_dx = getsubwidth(dc, cx, m_index, si - m_index);
			
            MCRectangle t_unsel_clip;
            t_unsel_clip = t_old_clip;

            if (is_rtl())
            {
                t_unsel_clip . x = x + width - t_start_dx;
                t_unsel_clip . width = t_start_dx;
                t_sel_clip . x = x;
                t_sel_clip . width = t_unsel_clip . x - t_sel_clip . x;
            }
            else
            {
                t_unsel_clip . x = x;
                t_unsel_clip . width = t_start_dx;
                t_sel_clip . x = t_unsel_clip . x + t_unsel_clip . width;
                t_sel_clip . width = width - t_start_dx;
            }

            // SN-2014-07-09: [[ MERGE-6.7 ]] Update to the new clipping methods
            dc -> cliprect(t_unsel_clip);
            // SN-2014-08-13: [[ Bug 13016 ]] Added a parameter for the left of the cell
			drawstring(dc, x, lx, cx, y, m_index, m_size, (flags & F_HAS_BACK_COLOR) != 0, t_style);
        }

        // SN-2014-07-09: [[ MERGE-6.7 ]] Switch back to the initial clip
        dc -> restore();
        dc -> save();

		// If there is some unselected text at the end of the block, then render it.
		if (ei < m_index + m_size)
		{
			coord_t t_end_dx;
			t_end_dx = getsubwidth(dc, cx, m_index, ei - m_index);
			
            MCRectangle t_unsel_clip;
            t_unsel_clip = dc -> getclip();
            
            if (is_rtl())
            {
                t_unsel_clip . x = x;
                t_unsel_clip . width = width - t_end_dx;
                t_sel_clip . x = t_unsel_clip . x + t_unsel_clip . width;
                t_sel_clip . width = t_sel_clip . width - (width - t_end_dx);
            }
            else
            {
                t_unsel_clip . x = x + t_end_dx;
                t_unsel_clip . width = width - t_end_dx;
                // Unchanged: t_sel_clip . x
                t_sel_clip . width = x + t_end_dx - t_sel_clip . x;
            }

            dc -> cliprect(t_unsel_clip);
            // SN-2014-08-13: [[ Bug 13016 ]] Added a parameter for the left of the cell
			drawstring(dc, x, lx, cx, y, m_index, m_size, (flags & F_HAS_BACK_COLOR) != 0, t_style);
        }

        // SN-2014-07-09: [[ MERGE-6.7 ]] Switch back to the initial clip
        dc -> restore();
        dc -> save();

        // Now use the clip rect we've computed for the selected portion.
        dc -> cliprect(t_sel_clip);
		
		// Change the hilite color (if necessary).
		// MM-2013-11-05: [[ Bug 11547 ]] We now pack alpha values into pixels meaning we shouldn't check against MAXUNIT4. Not sure why this check was here previously.
		if (!(flags & F_HAS_COLOR))
		{
			if (IsMacLF() && !f->isautoarm())
			{
				MCPatternRef t_pattern;
				int2 t_x, t_y;
				MCColor fc, hc;
				f->getforecolor(DI_FORE, False, True, fc, t_pattern, t_x, t_y, dc -> gettype(), f);
				f->getforecolor(DI_HILITE, False, True, hc, t_pattern, t_x, t_y, dc -> gettype(), f);
				if (MCColorGetPixel(hc) == MCColorGetPixel(fc))
					f->setforeground(dc, DI_BACK, False, True);
                else
                    setcolorforselectedtext(dc, nil);
			}
			else
                setcolorforselectedtext(dc, t_foreground_color);
		}

		// Draw the selected text.
        // SN-2014-08-13: [[ Bug 13016 ]] Added a parameter for the left of the cell
		drawstring(dc, x, lx, cx, y, m_index, m_size, (flags & F_HAS_BACK_COLOR) != 0, t_style);
		
		// MM-2013-11-05: [[ Bug 11547 ]] We now pack alpha values into pixels meaning we shouldn't check against MAXUNIT4. Not sure why this check was here previously.
		// Revert to the previous clip and foreground color.
		if (t_foreground_color != NULL)
			dc->setforeground(*t_foreground_color);
		else if (!(flags & F_HAS_COLOR))
			f->setforeground(dc, DI_FORE, False, True);
		
		dc->restore();
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
		coord_t t_width;
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
		dc->save();
        dc->setclip(t_clip);
		
		if (fontstyle & FA_BOX)
		{
			// MW-2012-09-04: [[ Bug 9759 ]] Adjust any pattern origin to scroll with text.
			f->setforeground(dc, DI_BORDER, False, True);
			f->adjustpixmapoffset(dc, DI_BORDER);
			MCRectangle trect = MCBlockMakeRectangle(x - 1, y - t_ascent, t_width + 2, t_ascent + t_descent);
			
			// MW-2014-03-11: [[ Bug 11882 ]] Ensure we use miter join for drawing the border.
			dc->setlineatts(1, LineSolid, CapButt, JoinMiter);
			dc->drawrect(trect, true);
			dc->setlineatts(0, LineSolid, CapButt, JoinBevel);
			
			f->setforeground(dc, DI_FORE, False, True);
		}
		else if (fontstyle & FA_3D_BOX)
		{
			MCRectangle trect = MCBlockMakeRectangle(x - 1, y - t_ascent, t_width + 2, t_ascent + t_descent);
			f->draw3d(dc, trect, ETCH_RAISED, 1);
			f->setforeground(dc, DI_FORE, False, True);
		}
		
		// Revert the clip back to the previous setting.
		dc->restore();
	}
	
	// MM-2013-11-05: [[ Bug 11547 ]] We now pack alpha values into pixels meaning we shouldn't check against MAXUNIT4. Not sure why this check was here previously.
	if (flags & F_HAS_BACK_COLOR)
		dc->setbackground(MCzerocolor);

	// MM-2013-11-05: [[ Bug 11547 ]] We now pack alpha values into pixels meaning we shouldn't check against MAXUNIT4. Not sure why this check was here previously.
	if (flags & F_HAS_COLOR || fontstyle & FA_LINK)
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
        char *t_font;
        /* UNCHECKED */ MCStringConvertToCString(MCNameGetString(atts -> fontname), t_font);
		r_textfont = t_font;
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
			atts = new (nothrow) Blockatts;
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
			atts = new (nothrow) Blockatts;
		if (!(flags & F_HAS_COLOR))
			atts->color = new (nothrow) MCColor;
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
			atts = new (nothrow) Blockatts;
		if (!(flags & F_HAS_BACK_COLOR))
			atts->backcolor = new (nothrow) MCColor;
		*atts->backcolor = *newcolor;
		flags |= F_HAS_BACK_COLOR;
	}
}

coord_t MCBlock::GetCursorX(findex_t fi)
{
	findex_t j = fi - m_index;
	if (j > m_size)
		j = m_size;
    
    // AL-2014-07-29: [[ Bug 12896 ]] Include tab width in block cursor calculation
    // SN-2014-08-08: [[ Bug 13124 ]] Ignore the last tab width if we are the empty, last block
    //  of a TAB-terminated paragraph
    if (m_size && j == m_size && fi > 0 && parent -> GetCodepointAtIndex(fi - 1) == '\t')
    {
        if (segment -> GetLastBlock() == this)
            return segment -> GetWidth() - origin;
    }
    
    // [[ BiDi Support ]]
    // If the block is RTL, x decreases as fi increases
    if (is_rtl())
        return origin + width - getsubwidth(NULL, tabpos, m_index, j);
    else
        return origin + getsubwidth(NULL, tabpos, m_index, j);
}

findex_t MCBlock::GetCursorIndex(coord_t x, Boolean chunk, Boolean last, bool moving_forward)
{
    // The x coordinate is relative to the segment, not ourselves
    x -= getorigin();
    
    // MW-2007-07-05: [[ Bug 5099 ]] If we have an image and are unicode, the char
	//   we replace is two bytes long
	if (flags & F_HAS_IMAGE && atts->image != NULL)
    {
		if (chunk || x < atts->image->getrect().width >> 1)
			return m_index;
		else
			return m_index + 1;
    }

	findex_t i = m_index;
	
	// MW-2012-02-01: [[ Bug 9982 ]] iOS uses sub-pixel positioning, so make sure we measure
	//   complete runs.
	// MW-2013-11-07: [[ Bug 11393 ]] We only want to measure complete runs now regardless of
	//   platform.
	coord_t t_last_width;
	t_last_width = is_rtl() ? width : 0;
    
    coord_t t_pos = t_last_width;
    while(i < m_index + m_size)
    {
        findex_t t_new_i;
        
        // SN-2014-03-26 [[ CombiningChars ]] We need to find the size of a char, starting from a given codepoint
        t_new_i = parent -> NextChar(i);
        
        coord_t t_new_width;
        t_new_width = GetCursorX(t_new_i) - origin;
        
        if (chunk)
            t_pos = t_new_width;
        else if (t_last_width == t_new_width)
            // Don't update the position if this character was zero-width
            ;
        else
            t_pos = (t_last_width + t_new_width) / 2;
        
        if ((is_rtl() && x >= t_pos) || (!is_rtl() && x < t_pos))
        {
            // FG-2014-07-16: [[ Bugfix 12166 ]] Make sure that we don't return
            // an index pointing at a zero-width character.
            while (moving_forward && i < m_index + m_size && (GetCursorX(i) - origin) == t_new_width)
                i++;
            while (!moving_forward && i > m_index && (GetCursorX(i-1) - origin) == t_last_width)
                i--;

            break;
        }
        
        t_last_width = t_new_width;
        i = t_new_i;
    }

	if (i == m_index + m_size && last && (m_index + m_size != parent->gettextlength()))
        return parent -> PrevChar(i);
	else
		return i;
}

coord_t MCBlock::getsubwidth(MCDC *dc, coord_t x /* IGNORED */, findex_t i, findex_t l)
{
	if (l == 0)
		return 0;
	if (flags & F_HAS_IMAGE && atts->image != NULL)
		return atts->image->getrect().width;
	else
	{
		findex_t sptr = i;
        
		// MW-2012-02-12: [[ Bug 10662 ]] If the last char is a VTAB then ignore it.
        if (parent->TextIsLineBreak(parent->GetCodepointAtIndex(sptr + l - 1)))
			l--;
        
        // AL-2014-07-18: [[ Bug 12828 ]] If the last char is a tab character then ignore it.
        // SN-2014-07-24: [[ Bug 12948 ]] Fix for the crash (negative length possible)
        if (l && parent->GetCodepointAtIndex(sptr + l - 1) == '\t')
			l--;

		// MW-2012-08-29: [[ Bug 10325 ]] Use 32-bit int to compute the width, then clamp
		//   to 65535 - this means that we force wrapping when the line is too long.
		// MW-2013-08-08: [[ Bug 10654 ]] Make sure we use a signed integer here, otherwise
		//   we get incorrect clamping when converted to unsigned.
        // FG-2014-04-30: [[ TabAlignments ]] Blocks no longer contain tabs
		coord_t twidth = 0;
		/*if (flags & F_HAS_TAB)
		{
			uindex_t eptr;
			while (MCStringFirstIndexOfChar(parent->GetInternalStringRef(), '\t', sptr, kMCStringOptionCompareExact, eptr))
			{
				// Break if we've gone past the end of this block
                if (eptr >= i + t_length)
					break;
				
				MCRange t_range;
				t_range = MCRangeMakeMinMax(sptr, eptr);
                // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
                twidth += MCFontMeasureTextSubstringFloat(m_font, parent->GetInternalStringRef(), t_range, parent -> getparent() -> getstack() -> getdevicetransform());

				twidth += gettabwidth(x + twidth, eptr);

				// Adjust for the tab character.
				eptr = parent->IncrementIndex(eptr);
				findex_t sl = eptr - sptr;

				sptr += sl;
				l -= sl;
			}
		}*/
		MCRange t_range;
		t_range = MCRangeMake(sptr, l);
        // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
        return twidth + MCFontMeasureTextSubstringFloat(m_font, parent->GetInternalStringRef(), t_range, parent -> getparent() -> getstack() -> getdevicetransform());
	}
}

coord_t MCBlock::getwidth(MCDC *dc, coord_t x)
{
	if (flags & F_HAS_IMAGE && atts->image != NULL)
		return atts->image->getrect().width;
	else if (dc != NULL && dc -> gettype() == CONTEXT_TYPE_PRINTER)
		return getsubwidth(dc, x, m_index, m_size);
    // FG-2014-04-30 [[ TabAlignments ]] Blocks no longer contain tabs
	else if (width == 0 /*|| flags & F_HAS_TAB*/) 
		return width = getsubwidth(dc, x, m_index, m_size);
	else
		return width;
}

void MCBlock::reset()
{
	width = 0;
}

coord_t MCBlock::GetAscent() const
{
   	int2 shift = flags & F_HAS_SHIFT ? atts->shift : 0;
    // MW-2007-07-05: [[ Bug 1943 ]] - Images do not have correct ascent height *MIGHT NEED REVERSION*
    if (flags & F_HAS_IMAGE && atts->image != NULL)
        return MCU_max(0, atts->image->getrect().height - shift + 2);
    else
        return MCU_max(0.0f, MCFontGetAscent(m_font) - shift);
}

coord_t MCBlock::GetDescent() const
{
    int2 shift = flags & F_HAS_SHIFT ? atts->shift : 0;
    if (flags & F_HAS_IMAGE && atts->image != NULL)
        return MCU_max(0, shift);
    else
        return MCU_max(0.0f, MCFontGetDescent(m_font) + shift);
}

coord_t MCBlock::GetLeading() const
{
	// PM-2016-02-15: [[ Bug 16754 ]] Use same value for leading as in pre-LC 8 when an image is present 
    if (flags & F_HAS_IMAGE && atts->image != NULL)
        return IMAGE_BLOCK_LEADING;
    else
        return MCFontGetLeading(m_font);
}

void MCBlock::freeatts()
{
	freerefs();
	// MW-2012-02-17: [[ SplitTextAttrs ]] Free the fontname name if we have that attr.
	if (flags & F_HAS_FNAME)
		MCValueRelease(atts -> fontname);
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
	// MW-2012-05-04: [[ Values ]] linkText / imageSource / metaData are now uniqued
	//   strings.

	if (flags & F_HAS_LINK)
	{
		MCValueRelease(atts -> linktext);
		atts -> linktext = nil;
	}

	if (flags & F_HAS_IMAGE)
	{
		if (opened)
			closeimage();

		MCValueRelease(atts -> imagesource);
		atts -> imagesource = nil;
	}

	if (flags & F_HAS_METADATA)
	{
		MCValueRelease(atts -> metadata);
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
		uint4 t_image_id;
		if (MCU_stoui4(atts->imagesource, t_image_id))
			atts -> image = t_field -> resolveimageid(t_image_id);
		else
		{
			MCNewAutoNameRef t_name;
			/* UNCHECKED */ MCNameCreate(atts->imagesource, &t_name);
			atts->image = (MCImage *)t_field->getstack()->getobjname(CT_IMAGE, *t_name);
		}

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

MCStringRef MCBlock::getlinktext()
{
	// MW-2012-10-01: [[ Bug 10178 ]] Added guard to ensure we don't get a null
	//   dereference.
	if (flags & F_HAS_LINK && atts != nil)
		return atts->linktext;

	return NULL;
}

MCStringRef MCBlock::getimagesource()
{
	// MW-2012-10-01: [[ Bug 10178 ]] Added guard to ensure we don't get a null
	//   dereference.
	if (flags & F_HAS_IMAGE && atts != nil)
		return atts->imagesource;

	return NULL;
}

MCStringRef MCBlock::getmetadata(void)
{
	// MW-2012-10-01: [[ Bug 10178 ]] Added guard to ensure we don't get a null
	//   dereference.
	if (flags & F_HAS_METADATA && atts != nil && atts -> metadata != nil)
		return atts -> metadata;

	return kMCEmptyString;
}

void MCBlock::sethilite(Boolean on)
{
	if (on)
		flags |= F_HILITED;
	else
		flags &= ~F_HILITED;
}

MCBlock *MCBlock::RetreatIndex(findex_t& x_index)
{
	MCBlock *t_block;
	t_block = this;
	
	if (x_index == 0)
		return NULL;
	
	// MW-2012-08-29: [[ Bug 10322 ]] If we are at the start of the block, then we must
	//   move back a block before doing anything.
	if (x_index == m_index)
	{
		do
		{
			t_block = t_block -> prev();
		}
		while(t_block -> m_size == 0 && t_block -> prev() != parent -> getblocks() -> prev());
	}
		
	x_index = parent->DecrementIndex(x_index);

	// MW-2012-03-10: [[ Bug ]] Loop while the block is empty, or the block doesn't
	//   contain the index.
	while(x_index < t_block -> m_index || t_block -> m_size == 0)
	{
		t_block = t_block -> prev();
		if (t_block == parent -> getblocks() -> prev())
			return NULL;
	}

	return t_block;
}

MCBlock *MCBlock::AdvanceIndex(findex_t& x_index)
{
	MCBlock *t_block;
	t_block = this;
	
	// MW-2012-08-29: [[ Bug 10322 ]] If we are at the end of the block, then we must
	//   move forward a block before doing anything.
	if (x_index == m_index + m_size)
	{
		do
		{
			t_block = t_block -> next();
		}
		while(t_block -> m_size == 0 && t_block -> next() != parent -> getblocks());
	}
	
	x_index = parent->NextChar(x_index);

	// MW-2012-03-10: [[ Bug ]] Loop while the block is empty, or the block doesn't
	//   contain the index.
	while(x_index >= t_block -> m_index + t_block -> m_size || t_block -> m_size == 0)
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
		x_style . has_text_color = true;
		x_style . text_color = MCColorGetPixel(*(atts -> color));
	}
	if (getflag(F_HAS_BACK_COLOR))
	{
		x_style . has_background_color = true;
		x_style . background_color = MCColorGetPixel(*(atts -> backcolor));
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
    MCExecContext ctxt(nil, nil, nil);
	if (p_style . has_text_color)
	{
		MCColor t_color;
		MCColorSetPixel(t_color, p_style . text_color);
		setcolor(&t_color);
	}
	if (p_style . has_background_color)
	{
		MCColor t_color;
		MCColorSetPixel(t_color, p_style . background_color);
		setbackcolor(&t_color);
	}
	if (p_style . has_link_text)
        SetLinktext(ctxt, p_style . link_text);
	if (p_style . has_image_source)
        SetImageSource(ctxt, p_style . image_source);
    if (p_style . has_metadata)
        SetMetadata(ctxt, p_style . metadata);
    if (p_style . has_text_font)
        SetTextFont(ctxt, MCNameGetString(p_style . text_font));
	if (p_style . has_text_style)
    {
        MCInterfaceTextStyle t_style;
        t_style . style = p_style . text_style;
        SetTextStyle(ctxt, t_style);
    }
    if (p_style . has_text_size)
    {
        uinteger_t t_size = p_style . text_size;
        SetTextSize(ctxt, &t_size);
    }
	// MW-2012-05-09: [[ Bug ]] Setting the textShift of a block is done with 'setshift'
	//   not 'setatts'.
	if (p_style . has_text_shift)
		setshift(p_style . text_shift);
}

// SN-2014-10-31: [[ Bug 13879 ]] Update the way the string is measured.
uint32_t measure_stringref(MCStringRef p_string, uint32_t p_version)
{
    MCStringEncoding t_encoding;
    uint32_t t_additional_bytes = 0;
    

    if (p_version < kMCStackFileFormatVersion_7_0)
        t_encoding = kMCStringEncodingNative;
    else
        t_encoding = kMCStringEncodingUTF8;
   
    // Encode the string to get the right length
    MCAutoDataRef t_data;
    /* UNCHECKED */ MCStringEncode(p_string, t_encoding, false, &t_data);
    uint32_t t_length;
    t_length = MCDataGetLength(*t_data);
    
    if (p_version < kMCStackFileFormatVersion_7_0)
    {
        // Full string is written in 5.5 format:
        //  - length is written as a uint2
        //  - NULL char is included
        t_additional_bytes = 2 + 1;
    }
    else
    {
        // 7.0 format may write the length as a uint4
        if (t_length < 16384)
            t_additional_bytes = 2;
        else
            t_additional_bytes = 4;
    }
    
    return t_length + t_additional_bytes;
}

// MW-2012-03-04: [[ StackFile5500 ]] Utility routine for computing the length of
//   a nameref when serialized to disk.
uint32_t measure_nameref(MCNameRef p_name, uint32_t p_version)
{
	return measure_stringref(MCNameGetString(p_name), p_version);
}

// MW-2012-03-04: [[ StackFile5500 ]] Compute the number of bytes the attributes will
//   take up when serialized.
uint32_t MCBlock::measureattrs(uint32_t p_version)
{
	// If there are no attrs, then the size is 0.
	if (!hasatts())
		return 0;

	uint32_t t_size;
	t_size = 0;
	
	// The flags field.
	t_size = 4;
	// The font index (if any)
	//if ((flags & (F_FATTR_MASK | F_HAS_UNICODE)) != 0)
	if (true)
		t_size += 2;
	if ((flags & F_HAS_COLOR) != 0)
		t_size += 6;
	if ((flags & F_HAS_BACK_COLOR) != 0)
		t_size += 6;
	if ((flags & F_HAS_SHIFT) != 0)
		t_size += 2;

	// MW-2012-05-04: [[ Values ]] linkText / imageSource / metaData are now uniqued
	//   strings.
	if ((flags & F_HAS_LINK) != 0)
		t_size += measure_stringref(atts -> linktext, p_version);
	if ((flags & F_HAS_IMAGE) != 0)
		t_size += measure_stringref(atts -> imagesource, p_version);
	if ((flags & F_HAS_METADATA) != 0)
		t_size += measure_stringref((MCStringRef)atts -> metadata, p_version);

	return t_size;
}

bool MCBlock::GetFirstLineBreak(findex_t& r_index)
{
	uindex_t t_offset;
    // AL-2014-08-21: [[ Bug 13247 ]] Don't repeatedly search to the end of the paragraph to find line break.
    //  The search should only be in the range of the block.
    if (!MCStringFirstIndexOfCharInRange(parent->GetInternalStringRef(), '\v', MCRangeMake(m_index, m_size), kMCStringOptionCompareExact, t_offset))
        return false;
    
    // SN-2014-03-20: [[ bug 11947 ]] ensure the index is incremented to avoid an infinite loop...
	r_index = parent -> IncrementIndex(t_offset);
    
    if (r_index > m_index + m_size)
        return false;

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

void MCBlock::GetRange(findex_t &r_index, findex_t &r_length)
{
	r_index = m_index;
	r_length = m_size;
}

findex_t MCBlock::GetOffset()
{
	return m_index;
}

findex_t MCBlock::GetLength()
{
	return m_size;
}

void MCBlock::SetRange(findex_t p_index, findex_t p_length)
{
	m_index = p_index;
	m_size = p_length;
	width = 0;
    
    // AL-2014-08-04: [[ Bug 13082 ]] Make sure imagesource is deleted in zero-width blocks
    if (flags & F_HAS_IMAGE && m_size == 0)
        freeatts();
    
	// Update the 'has tabs' flag
    // FG-2014-04-30 [[ TabAlignments ]] Blocks no longer contain tabs
	/*uindex_t t_where;
	if (MCStringFirstIndexOfChar(parent->GetInternalStringRef(), '\t', m_index, kMCStringOptionCompareExact, t_where)
		&& t_where < m_index + m_size)
		flags |= F_HAS_TAB;
	else
		flags &= ~F_HAS_TAB;*/
}

void MCBlock::MoveRange(findex_t p_index, findex_t p_length)
{
	m_index += p_index;
	m_size += p_length;
    if (p_length)
    {
        width = 0;
        // AL-2014-08-04: [[ Bug 13082 ]] Make sure imagesource is deleted in zero-width blocks
        if (flags & F_HAS_IMAGE && m_size == 0)
			freeatts();
    }
    
	// Update the 'has tabs' flag
    // FG-2014-04-30 [[ TabAlignments ]] Blocks no longer contain tabs
	/*uindex_t t_where;
	if (MCStringFirstIndexOfChar(parent->GetInternalStringRef(), '\t', m_index, kMCStringOptionCompareExact, t_where)
		&& t_where < m_index + m_size)
		flags |= F_HAS_TAB;
	else
		flags &= ~F_HAS_TAB;*/
}

codepoint_t MCBlock::GetCodepointAtIndex(findex_t p_index) const
{
	return parent->GetCodepointAtIndex(m_index + p_index);
}

// AL-2014-07-29: [[ Bug 12896 ]] The next and previous blocks in the visual order may go over segment boundaries
// AL-2014-07-30: [[ Bug 12924 ]] Calculate next and previous blocks more efficiently (and correctly).
MCBlock *MCBlock::GetNextBlockVisualOrder()
{
    MCBlock *bptr = GetSegment() -> GetFirstBlock();
    while (bptr != GetSegment() -> GetLastBlock())
    {
        // SN-2014-08-08: [[ Bug 13124 ]] Make sure we ignore the last empty block
        //  of the TAB-terminated paragraphs
        if (bptr->visual_index == visual_index + 1 && bptr -> m_size)
            return bptr;
        bptr = bptr->next();
    }
    
    MCSegment *last_segment = parent -> getsegments() -> prev();
    // SN-2014-08-08: [[ Bug 13124 ]] Make sure we ignore the last empty block
    //  of the TAB-terminated paragraphs
    if (segment != last_segment
            && segment -> next() -> GetFirstVisualBlock() -> m_size
                && !last_segment -> GetFirstVisualBlock() -> m_size)
            return segment -> next() -> GetFirstVisualBlock();
    
    return nil;
}

MCBlock *MCBlock::GetPrevBlockVisualOrder()
{
    MCBlock *bptr = GetSegment() -> GetFirstBlock();
    while (bptr != GetSegment() -> GetLastBlock())
    {
        if (bptr->visual_index == visual_index - 1)
            return bptr;
        bptr = bptr->next();
    }
    
    MCSegment *first_segment = parent -> getsegments();
    if (segment != first_segment)
        return segment -> prev() -> GetLastVisualBlock();
    
    return nil;
}

void MCBlock::setcolorfornormaltext(MCDC* dc, MCColor* p_color)
{
    MCField* f = parent->getparent();
    
    if (p_color != nil)
        dc->setforeground(*p_color);
    else if (flags & F_HAS_COLOR)
        dc->setforeground(*atts -> color);
    else
        f->setforeground(dc, DI_PSEUDO_TEXT_COLOR, False, True);
}

void MCBlock::setcolorforhilite(MCDC* dc)
{
    MCField* f = parent->getparent();
    
    f->setforeground(dc, DI_PSEUDO_TEXT_BACKGROUND_SEL, False, True);
}

void MCBlock::setcolorforselectedtext(MCDC* dc, MCColor* p_color)
{
    MCField* f = parent->getparent();
    
    if (p_color != nil)
        dc->setforeground(*p_color);
    else if (flags & F_HAS_COLOR)
        dc->setforeground(*atts -> color);
    else if (!IsMacLF()) // TODO: if platform reverses selected text
        f->setforeground(dc, DI_PSEUDO_TEXT_COLOR_SEL_BACK, False, True, true);
    else
        f->setforeground(dc, DI_PSEUDO_TEXT_COLOR_SEL_FORE, False, True, true);
}
