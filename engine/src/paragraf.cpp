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

#include "stack.h"
#include "MCBlock.h"
#include "segment.h"
#include "line.h"
#include "field.h"
#include "paragraf.h"

#include "util.h"
#include "mcerror.h"
#include "text.h"

#include "globals.h"

#include "mctheme.h"

#include "context.h"
#include "exec-interface.h"

#include "stackfileformat.h"

const char *CHARSET_HTML[] =
    {
        "en",
        "en",
        "ja",
        "zh-TW",
        "ko",
        "ar",
        "he",
        "ru",
        "tr",
        "bg",
        "uk",
        "po",
        "el",
        "zh-CN",
        "th",
		"vn",
		"li",
        "en-UC",
        "en-UC"
    };

uint2 MCParagraph::cursorwidth = 1;

MCParagraph::MCParagraph()
{
	parent = NULL;
	/* UNCHECKED */ MCStringCreateMutable(0, &m_text);
	blocks = NULL;
    segments = NULL;
	lines = NULL;
	focusedindex = 0;
	opened = 0;
	startindex = endindex = originalindex = PARAGRAPH_MAX_LEN;
	state = 0;
    moving_left = false;
    moving_forward = true;
	
	// MP-2013-09-02: [[ FasterField ]] Paragraphs start off needing layout.
	needs_layout = true;

	// MW-2012-01-25: [[ ParaStyles ]] All attributes are unset to begin with.
	attrs = nil;
    base_direction = kMCTextDirectionAuto;
}

MCParagraph::MCParagraph(const MCParagraph &pref) : MCDLlist(pref)
{
	parent = pref.parent;
	/* UNCHECKED */ MCStringMutableCopy(*pref.m_text, &m_text);
	
	blocks = NULL;
	if (pref.blocks != NULL)
	{
		MCBlock *bptr = pref.blocks;
		do
		{
			MCBlock *tbptr = new (nothrow) MCBlock(*bptr);
			tbptr->appendto(blocks);
			tbptr->setparent(this);
			bptr = bptr->next();
		}
		while (bptr != pref.blocks);
	}

	// MW-2012-01-25: [[ ParaStyles ]] Make sure our attrs are nil, then copy
	//   the attrs from the other paragraph.
	attrs = nil;
	copyattrs(pref);

    segments = NULL;
	lines = NULL;
	focusedindex = 0;
	startindex = endindex = originalindex = PARAGRAPH_MAX_LEN;
	opened = 0;
	state = 0;
    moving_left = false;
    moving_forward = true;
	
	// MP-2013-09-02: [[ FasterField ]] Paragraphs start off needing layout.
	needs_layout = true;
    base_direction = pref.base_direction;
}

MCParagraph::~MCParagraph()
{
	while (opened)
		close();
	
	deleteblocks();

	// MW-2006-04-13: Memory leak caused by 'lines' not being freed. This happens if the field is open
	//   but the paragraph is not. Which occurs when a paragraph is duplicated to the clipboard and then
	//   asked to render itself as RTF.
	deletelines();

	clearattrs();
}

MCBlock* MCParagraph::AppendText(MCStringRef p_string)
{
    // Ensure the block list has been set up
    if (blocks == nil)
        inittext();
    
	// Is the last block empty or does a new one need to be created?
	MCBlock *t_block = blocks->prev();
	if (t_block->GetLength() > 0)
	{
		// Block already contains data, create a new one
		MCBlock *t_newblock = new (nothrow) MCBlock;
		t_newblock->setparent(this);
		t_block->append(t_newblock);
		t_block = t_newblock;
	}
	
	// Does the requested text fit or is truncation required?
	uindex_t t_new_length = MCStringGetLength(p_string);
	if (gettextlength() + t_new_length >= PARAGRAPH_MAX_LEN - 1)
		t_new_length = PARAGRAPH_MAX_LEN - gettextlength() - 1;
	
	// Append the text as requested
	// TODO: trunctation
	findex_t t_cur_len = gettextlength();
	/* UNCHECKED */ MCStringAppend(*m_text, p_string);
	
	// Set the indices for the block containing this text
	t_block->SetRange(t_cur_len, t_new_length);
	return t_block;
}

findex_t MCParagraph::NextChar(findex_t p_in)
{
    uindex_t t_index;
    t_index = MCStringGraphemeBreakIteratorAdvance(*m_text, p_in);
    return (t_index == kMCLocaleBreakIteratorDone) ? MCStringGetLength(*m_text) : t_index;
}

findex_t MCParagraph::PrevChar(findex_t p_in)
{
    uindex_t t_index;
    t_index = MCStringGraphemeBreakIteratorRetreat(*m_text, p_in);
    return (t_index == kMCLocaleBreakIteratorDone) ? 0 : t_index;
}

findex_t MCParagraph::NextWord(findex_t p_in)
{
    MCBreakIteratorRef t_iter;
    /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCLocaleBasic, kMCBreakIteratorTypeWord, t_iter);
    /* UNCHECKED */ MCLocaleBreakIteratorSetText(t_iter, *m_text);
    uindex_t t_index;
    t_index = MCLocaleBreakIteratorAfter(t_iter, p_in);
    MCLocaleBreakIteratorRelease(t_iter);
    return (t_index == kMCLocaleBreakIteratorDone) ? MCStringGetLength(*m_text) : t_index;
}

findex_t MCParagraph::PrevWord(findex_t p_in)
{
    MCBreakIteratorRef t_iter;
    /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCLocaleBasic, kMCBreakIteratorTypeWord, t_iter);
    /* UNCHECKED */ MCLocaleBreakIteratorSetText(t_iter, *m_text);
    uindex_t t_index;
    t_index = MCLocaleBreakIteratorBefore(t_iter, p_in);
    MCLocaleBreakIteratorRelease(t_iter);
    return (t_index == kMCLocaleBreakIteratorDone) ? 0 : t_index;
}

bool MCParagraph::TextIsWordBreak(codepoint_t p_codepoint)
{
	return MCUnicodeGetBinaryProperty(p_codepoint, kMCUnicodePropertyWhiteSpace);
}

bool MCParagraph::TextIsLineBreak(codepoint_t p_codepoint)
{
	return p_codepoint == '\v' || p_codepoint == 0x2028;    // Line Separator (LSEP)
}

bool MCParagraph::TextIsSentenceBreak(codepoint_t p_codepoint)
{
	return MCUnicodeGetBinaryProperty(p_codepoint, kMCUnicodePropertySentenceBreak);
}

bool MCParagraph::TextIsParagraphBreak(codepoint_t p_codepoint)
{
	return p_codepoint == '\n' || p_codepoint == 0x2029;    // Paragraps Separator (PSEP)
}

bool MCParagraph::TextIsPunctuation(codepoint_t p_codepoint)
{
	return MCUnicodeIsPunctuation(p_codepoint);
}

bool MCParagraph::TextFindNextParagraph(MCStringRef p_string, findex_t p_after, findex_t &r_next)
{
	uindex_t t_length = MCStringGetLength(p_string);
	while (p_after < t_length)
	{
		codepoint_t t_char =
			MCStringGetCodepointAtIndex(p_string, p_after);
		
		if (TextIsParagraphBreak(t_char))
			break;
		
		p_after += MCUnicodeCodepointGetCodeunitLength(t_char);
	}
	
	if (p_after == t_length)
		return false;
	
	r_next = p_after + 1;
	return true;
}

void MCParagraph::SetBlockDirectionLevel(findex_t si, findex_t ei, uint8_t level)
{
    findex_t t_block_index, t_block_length;
    MCBlock *bptr = indextoblock(si, False);
    do
    {
        bptr->GetRange(t_block_index, t_block_length);
        if (t_block_index < si)
        {
            // Starts part of the way through this block
            MCBlock *tbptr = new (nothrow) MCBlock(*bptr);
            bptr->append(tbptr);
            bptr->SetRange(t_block_index, si - t_block_index);
            tbptr->SetRange(si, t_block_length - (si - t_block_index));
            bptr = bptr->next();
        }
        else
        {
            bptr->close();
        }
        
        if (t_block_index + t_block_length > ei)
        {
            // Ends part of the way through this block
            MCBlock *tbptr = new (nothrow) MCBlock(*bptr);
            if (getopened())
                tbptr->open(getparent()->getfontref());
            bptr->append(tbptr);
            bptr->SetRange(t_block_index, ei - t_block_index);
            tbptr->SetRange(ei, t_block_length - ei + t_block_index);
        }
        
        if (getopened())
            bptr->open(getparent()->getfontref());
        bptr->SetDirectionLevel(level);
        bptr = bptr->next();
    }
	// PM-2016-01-19: [[ Bug 16741]] If bptr == bptr->next() then LC never exits this endless loop and hangs
    while (t_block_index + t_block_length < gettextlength()
           && t_block_index + t_block_length < ei && bptr != bptr->next());
}

void MCParagraph::resolvetextdirections()
{ 
    // Use the field text direction, if specified
    uint8_t t_base_level;
    if (parent->gettextdirection() == kMCTextDirectionLTR)
        t_base_level = 0;
    else if (parent->gettextdirection() == kMCTextDirectionRTL)
        t_base_level = 1;
    else // == kMCTextDirectionAuto
        t_base_level = MCBidiFirstStrongIsolate(*m_text, 0);
    
    MCAutoArray<uint8_t> t_levels;
   
    // SN-2014-04-03 [[ Bug 12078 ]] Text direction resolving relocated in foundation-bidi.h
    /* UNCHECKED */ MCBidiResolveTextDirection(*m_text, t_base_level, t_levels . PtrRef(), t_levels . SizeRef());
    
    // Using the calculated levels, do the appropriate block creation
    uindex_t i = 0;
    uindex_t t_length = t_levels . Size();
    
    while (i < t_length)
    {
        // Scan forward for the next change in direction level
        uint8_t t_cur_level;
        uindex_t t_run_length;
        t_cur_level = t_levels[i];
        t_run_length = 1;
        while (i + t_run_length < t_length && t_levels[i + t_run_length] == t_cur_level)
            t_run_length++;
        
        // Set the direction level attribute for this run
        SetBlockDirectionLevel(i, i + t_run_length, t_cur_level);
        
        // Next block
        i += t_run_length;
    }
}

uint8_t MCParagraph::firststrongisolate(uindex_t p_offset) const
{
    // From TR9:
    //  P1. Split the text into separate paragraphs. A paragraph separator is
    //      kept with the previous paragraph. Within each paragraph, apply all
    //      the other rules of this algorithm. (Already done by this stage)
    //
    //  P2. In each paragraph, find the first character of type L, AL, or R
    //      while skipping over any characters between an isolate initiator and
    //      its matching PDI or, if it has no matching PDI, the end of the
    //      paragraph.
    //
    //  P3. If a character is found in P2 and it is of type AL or R, then set
    //      the paragraph embedding level to one; otherwise, set it to zero
    
    bool t_found = false;
    uindex_t t_depth = 0;
    uint8_t t_level = 0;
    while (!t_found && p_offset < MCStringGetLength(*m_text))
    {
        codepoint_t t_char;
        t_char = MCStringGetCharAtIndex(*m_text, p_offset);
        
        // Get the surrogate pair, if required
        uindex_t t_increment = 1;
        codepoint_t t_low;
        if (MCUnicodeCodepointIsHighSurrogate(t_char) &&
            MCUnicodeCodepointIsLowSurrogate(t_low = MCStringGetCharAtIndex(*m_text, p_offset + 1)))
        {
            t_char = MCUnicodeSurrogatesToCodepoint(t_char, t_low);
            t_increment = 2;
        }
        
        // Get the directional category for this codepoint
        int32_t t_dir;
        t_dir = MCUnicodeGetIntegerProperty(t_char, kMCUnicodePropertyBidiClass);
        
        // Is this an isolate initiator?
        if (t_dir == kMCUnicodeDirectionLeftToRightIsolate
            || t_dir == kMCUnicodeDirectionRightToLeftIsolate)
        {
            t_depth++;
        }
        
        // Is this an isolate terminator?
        if (t_dir == kMCUnicodeDirectionPopDirectionalIsolate && t_depth > 0)
        {
            t_depth--;
        }
        
        // Is this a codepoint with a strong direction?
        if (t_depth == 0 && t_dir == kMCUnicodeDirectionLeftToRight)
        {
            t_level = 0;
            t_found = true;
        }
        else if (t_depth == 0 && (t_dir == kMCUnicodeDirectionRightToLeft
                    || t_dir == kMCUnicodeDirectionRightToLeftArabic))
        {
            t_level = 1;
            t_found = true;
        }
        
        p_offset += t_increment;
    }
    
    return t_level;
}

bool MCParagraph::visit(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor* p_visitor)
{
	bool t_continue;
	t_continue = true;

	if (MCObjectVisitorIsDepthLast(p_options))
		t_continue = p_visitor -> OnParagraph(this);
	
	if (t_continue && blocks != NULL)
	{
		MCBlock *bptr = blocks;
		do
		{
			t_continue = bptr -> visit(p_options, p_part, p_visitor);
			bptr = bptr->next();
		}
		while(t_continue && bptr != blocks);
	}

	if (t_continue && MCObjectVisitorIsDepthFirst(p_options))
		t_continue = p_visitor -> OnParagraph(this);

	return t_continue;
}

uint32_t MCParagraph::getminimumstackfileversion(void)
{
	return kMCStackFileFormatMinimumExportVersion;
}

// **** mutate blocks
IO_stat MCParagraph::load(IO_handle stream, uint32_t version, bool is_ext)
{
	IO_stat stat;
	uint1 type;

    // The constructor-created string of the paragraph must be reset
    m_text.Reset();

	// MW-2013-11-20: [[ UnicodeFileFormat ]] Prior to 7.0, paragraphs were mixed runs
	//   of UTF-16 and native text. 7.0 plus they are just a stringref.
	if (version < kMCStackFileFormatVersion_7_0)
	{
		uint32_t t_length;
		MCAutoCustomPointer<char,MCMemoryDeleteArray> t_text_data;
        
		// This string can contain a mixture of Unicode and native - t_length is the number
        // of bytes.
        if ((stat = IO_read_string_legacy_full(&t_text_data, t_length, stream, 2, true, false)) != IO_NORMAL)
			return checkloadstat(stat);

        if (!MCStringCreateMutable(0, &m_text))
			return checkloadstat(IO_ERROR);

        // MW-2012-03-04: [[ StackFile5500 ]] If this is an extended paragraph then
        //   load in the attribute extension record.
        if (is_ext)
            if ((stat = loadattrs(stream, version)) != IO_NORMAL)
                return checkloadstat(IO_ERROR);
		
		// If the whole text isn't covered by the saved blocks, the index of the
		// portion not covered needs to be retained so that it can be added to
		// the paragraph text at the end of loading.
		uindex_t t_last_added = 0;
		while (True)
		{
			if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
				return checkloadstat(stat);
			switch (type)
			{
				// MW-2012-03-04: [[ StackFile5500 ]] Handle either a normal block, or
				//   an extended block.
				case OT_BLOCK:
				case OT_BLOCK_EXT:
				{
					/* UNCHECKED */ MCAutoPointer<MCBlock> newblock =
						new (nothrow) MCBlock;
					newblock->setparent(this);
					
					// MW-2012-03-04: [[ StackFile5500 ]] If the tag was actually an
					//   extended block, then pass in 'true' for is_ext.
					if ((stat = newblock->load(stream, version, type == OT_BLOCK_EXT)) != IO_NORMAL)
					{
						return checkloadstat(IO_ERROR);
					}
					
					// The indices returned here are *wrong* from the point of view
					// of the refactored Unicode paragraph - the block as loaded
					// stores byte indices, not UTF-16 value indices. These wrong
					// values are needed to ensure the paragraph text is loaded 
					// using the correct encoding and get fixed up below.
					findex_t index, len;
					newblock->GetRange(index, len);
                    t_last_added = index+len;

                    // SN-2014-10-31: [[ Bug 13881 ]] Ensure that the block hasn't been corrupted.
                    //  (leads to a potential crash, in case the corrupted stack ends up to be valid).
                    if (index > t_length)
                        return checkloadstat(IO_ERROR);
                    
                    // Some stacks seem to be saved with invalid blocks that
                    // exceed the length of the paragraph character data
                    // SN-2014-09-29: [[ Bug 13552 ]] Clamp the length appropriately
                    if (len + index > t_length)
                    {
                        // MW-2014-09-29: [[ Bug 13552 ]] Make sure we only recalculate if the length
                        //   is not 0.
                        if (len != 0)
                            len = t_length - index;
                    }
                    
                    uindex_t t_index;
                    t_index = MCStringGetLength(*m_text);

					if (newblock->IsSavedAsUnicode())
					{
						//len >>= 1;
						if (len > 0 && t_length > 0)
						{
							// Copy to a new buffer to ensure alignment
							MCAutoArray<unichar_t> t_unicode_buffer;
							if (!t_unicode_buffer.New(len / sizeof(unichar_t)))
							{
								return checkloadstat(IO_ERROR);
							}

							uindex_t t_buffer_len = t_unicode_buffer.Size() * sizeof(unichar_t);
							MCMemoryCopy(t_unicode_buffer.Ptr(),
							             *t_text_data + index, t_buffer_len);
                            
							// Byte swap, if required
							for (uindex_t i = 0; i < t_unicode_buffer.Size(); ++i)
							{
								uint2 t_char = uint2(t_unicode_buffer[i]);
								swap_uint2(&t_char);
								t_unicode_buffer[i] = t_char;
							}

                            // Append to the paragraph text
							if (!MCStringAppendChars(*m_text, t_unicode_buffer.Ptr(),
							                         t_unicode_buffer.Size()))
							{
                                return checkloadstat(IO_ERROR);
							}

							// Take into account possible trailing junk
							uindex_t t_unicode_count = t_unicode_buffer.Size();
							for (uindex_t i = t_buffer_len; i < uindex_t(len); ++i)
							{
								unichar_t t_trailing = (*t_text_data)[index + i];
								if (!MCStringAppendChar(*m_text, t_trailing))
								{
									return checkloadstat(IO_ERROR);
								}
								++t_unicode_count;
							}

							// The indices used by the block are incorrect and need
							// to be updated (offsets into the stored string and
							// the string held by the paragraph will differ if any
							// portion of the stored string was non-UTF-16)
                            newblock->SetRange(t_index, t_unicode_count);
						}
                        // SN-2014-09-29: [[ Bug 13552 ]] Update the block range, even if its length is 0
                        else
                            newblock->SetRange(t_index, 0);
					}
					else
					{
						if (MCtranslatechars && len && t_length > 0)
#ifdef __MACROMAN__
							IO_iso_to_mac(*t_text_data + index, len);
#else
						IO_mac_to_iso(*t_text_data + index, len);
#endif

						// String is in native format. Append to paragraph text
                        if (!MCStringAppendNativeChars(*m_text, (const char_t*)(*t_text_data + index), len))
                            return checkloadstat(IO_ERROR);

                        // Fix the indices used by the block
                        newblock->SetRange(t_index, len);
					}
					newblock.Release()->appendto(blocks);
				}
					break;
				default:
					if (blocks == NULL && MCtranslatechars && t_length && *t_text_data != nil)
					{
#ifdef __MACROMAN__
						IO_iso_to_mac(*t_text_data, t_length);
#else
						IO_mac_to_iso(*t_text_data, t_length);
#endif
					}
					
					// If we have no blocks, add all the text to the paragraph. Because
					// there were no blocks to say it was unicode, it must be native.
					if (t_last_added == 0)
					{
                        if (!MCStringAppendNativeChars(*m_text, (const char_t*)*t_text_data, t_length))
							return checkloadstat(IO_ERROR);
						t_last_added = t_length;
					}
					
					// Ensure that all the text was covered
					//if (t_last_added != t_length)
					//	return IO_ERROR;
					
                    MCS_seek_cur(stream, -1);
					return IO_NORMAL;
			}
		}		
	}
	else
	{
		// MW-2013-11-20: [[ UnicodeFileFormat ]] The text is just a stringref, so no
		//   magical swizzling to be done.
        MCAutoStringRef t_read_text;
		if ((stat = IO_read_stringref_new(&t_read_text, stream, true)) != IO_NORMAL)
			return checkloadstat(stat);
        
        // The paragraph text *must* be mutable
        /* UNCHECKED */ MCStringMutableCopyAndRelease(t_read_text.Take(), &m_text);

        // MW-2012-03-04: [[ StackFile5500 ]] If this is an extended paragraph then
        //   load in the attribute extension record.
        if (is_ext)
            if ((stat = loadattrs(stream, version)) != IO_NORMAL)
                return checkloadstat(stat);
		
		while (True)
		{
			if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
				return checkloadstat(stat);
			switch (type)
			{
					// MW-2012-03-04: [[ StackFile5500 ]] Handle either a normal block, or
					//   an extended block.
				case OT_BLOCK:
				case OT_BLOCK_EXT:
				{
					MCBlock *newblock = new (nothrow) MCBlock;
					newblock->setparent(this);
					
					// MW-2012-03-04: [[ StackFile5500 ]] If the tag was actually an
					//   extended block, then pass in 'true' for is_ext.
					if ((stat = newblock->load(stream, version, type == OT_BLOCK_EXT)) != IO_NORMAL)
					{
						delete newblock;
						return checkloadstat(stat);
					}
					
                    // De-(plitter about with) the block indices (the saving code doubles them because
                    // the 7.0 blocks are always Unicode).
                    newblock->SetRange(newblock->GetOffset()/2, newblock->GetLength()/2);
                    
					newblock->appendto(blocks);
				}
				break;
				default:
					MCS_seek_cur(stream, -1);
					return IO_NORMAL;
			}
		}		
	}

	// This point shouldn't be reached
	assert(false);
	
	return IO_NORMAL;
}

// **** require blocks
IO_stat MCParagraph::save(IO_handle stream, uint4 p_part, uint32_t p_version)
{
	IO_stat stat;
	defrag();
	
	// MW-2012-03-04: [[ StackFile5500 ]] If the paragraph has attributes and 5.5
	//   stackfile format has been requested, then output an extended paragraph.
	bool t_is_ext;
	if (p_version >= kMCStackFileFormatVersion_5_5 && attrs != nil)
		t_is_ext = true;
	else
		t_is_ext = false;
	
	if ((stat = IO_write_uint1(t_is_ext ? OT_PARAGRAPH_EXT : OT_PARAGRAPH, stream)) != IO_NORMAL)
		return stat;
	
	if (p_version < kMCStackFileFormatVersion_7_0)
	{
		// The string data that will get written out. It can't be just done as a
		// StringRef without breaking file format compatibility.
		uindex_t t_data_len;
		const char *t_data;
		if (MCStringIsNative(*m_text))
		{
			t_data_len = MCStringGetLength(*m_text);
			t_data = (const char *)MCStringGetNativeCharPtr(*m_text);
		}
		else
		{
			// The paragraph is not native. If it does not contain any blocks,
			// one will have to be created to ensure Unicodeness is preserved.
			if (blocks == nil)
				inittext();
			
			t_data_len = MCStringGetLength(*m_text) * sizeof(unichar_t);
			t_data = (const char *)MCStringGetCharPtr(*m_text);
		}
		
		if (!MCStringIsNative(*m_text))
		{
			// For file format compatibility, swap_uint2 must be called on each 
			// character in the UTF-16 string (Unicodeness is now a paragraph
			// property, not a block property, so it is done for all the text)
			unichar_t *t_swapped_data = new (nothrow) unichar_t[t_data_len/sizeof(unichar_t)];
			memcpy(t_swapped_data, t_data, t_data_len);
			for (uindex_t i = 0; i < MCStringGetLength(*m_text); i++)
			{
				swap_uint2((uint2*)&t_swapped_data[i]);
			}
			t_data = (char *)t_swapped_data;
		}

        // AL-2014-07-15: [[ Bug 12672 ]] If the data is larger than MAXUINT2,
        //  make sure appropriate size is passed to IO_write_string_legacy_full
		if ((stat = IO_write_string_legacy_full(MCString(t_data, t_data_len), stream, t_data_len > MAXUINT2 ? 4 : 2, true)) != IO_NORMAL)
			return stat;

		// If the string had to be byte swapped, delete the allocated data
		if (!MCStringIsNative(*m_text))
			delete[] t_data;
	}
	else
	{
		// MW-2013-11-20: [[ UnicodeFileFormat ]] The text is just a stringref, so no
		//   magical swizzling to be done.
		if ((stat = IO_write_stringref_new(*m_text, stream, true)) != IO_NORMAL)
			return stat;
	}
		
	// MW-2012-03-04: [[ StackFile5500 ]] If this is an extended paragraph then
	//   write out the attribtues.
	if (t_is_ext)
		if ((stat = saveattrs(stream, p_version)) != IO_NORMAL)
			return IO_ERROR;
 
	// Write out the blocks
	if (blocks != NULL)
	{
		MCBlock *tptr = blocks;
		do
		{
			if ((stat = tptr->save(stream, p_part, p_version)) != IO_NORMAL)
				return stat;

			tptr = tptr->next();
		}
		while (tptr != blocks);
	}
	
	return IO_NORMAL;
}

// MW-2012-02-14: [[ FontRefs ]] Now takes the parent fontref so that the block fontrefs
//   can be computed.
void MCParagraph::open(MCFontRef p_parent_font)
{
	if (opened++ == 0)
	{
		state = 0;
		if (blocks != NULL)
		{
			MCBlock *bptr = blocks;
			do
			{
				bptr->open(p_parent_font);
				bptr = bptr->next();
			}
			while (bptr != blocks);
		}
		else
			inittext();
	}
}

void MCParagraph::close()
{
	if (opened != 0 && --opened == 0)
	{
		defrag();
		deletelines();
		startindex = endindex = originalindex = PARAGRAPH_MAX_LEN;
		if (blocks != NULL)
		{
			MCBlock *bptr = blocks;
			do
			{
				bptr->close();
				bptr = bptr->next();
			}
			while (bptr != blocks);
		}
	}
}

void MCParagraph::setparent(MCField *newparent)
{
	parent = newparent;
}

// MW-2012-02-14: [[ FontRefs ]] Recalculate the block's fontrefs using the new parent fontref.
bool MCParagraph::recomputefonts(MCFontRef p_parent_font)
{
	if (blocks == nil)
		return false;

	bool t_changed;
	t_changed = false;
	
	MCBlock *t_block;
	t_block = blocks;
	do
	{
		if (t_block -> recomputefonts(p_parent_font))
			t_changed = true;
		t_block = t_block -> next();
	}
	while(t_block != blocks);
	
	// MP-2013-09-02: [[ FasterField ]] If any of the blocks have changed, layout is required.
	if (t_changed)
		needs_layout = true;
	
	return t_changed;
}

//clear blocks with zero length

// **** mutate blocks
Boolean MCParagraph::clearzeros(MCBlock *p_start_from)
{
	if (p_start_from == nil)
		p_start_from = blocks;

	Boolean reflow = False;
	if (blocks != NULL && blocks->next() != blocks)
	{
		MCBlock *bptr = p_start_from;
		do
		{
			findex_t i, l;
			bptr->GetRange(i, l);
			if (l == 0)
			{
				MCBlock *tbptr = bptr;
				bptr = bptr->next();
				tbptr->remove(blocks);
				delete tbptr;
				reflow = True;
			}
			else
				bptr = bptr->next();
		}
		while (bptr != blocks);
	}

	if (reflow)
	{
		state |= PS_LINES_NOT_SYNCHED;
	}

	return reflow;
}

// **** mutate lines
void MCParagraph::deletelines()
{
	while (segments != NULL)
    {
        MCSegment *sptr = segments->remove(segments);
        delete sptr;
    }
    
    while (lines != NULL)
	{
		MCLine *lptr = lines->remove(lines);
		delete lptr;
	}
	
    segments = NULL;
    lines = NULL;
    
	// MP-2013-09-02: [[ FasterField ]] Deleting the lines means layout is needed.
	needs_layout = true;
}

// **** mutate blocks
void MCParagraph::deleteblocks()
{
	while (blocks != NULL)
	{
		MCBlock *bptr = blocks->remove(blocks);
		delete bptr;
	}

	state |= PS_LINES_NOT_SYNCHED;
	
	// MP-2013-09-02: [[ FasterField ]] Deleting the blocks means layout is needed.
	needs_layout = true;
}

//clear blocks with the same attributes
// **** mutate blocks
void MCParagraph::defrag()
{
	bool t_blocks_changed;
	t_blocks_changed = false;

	if (blocks != NULL)
	{
		MCBlock *bptr = blocks;
		Boolean frag;
		do
		{
			if (bptr->next() != blocks
			        && bptr->sameatts(bptr->next(), false))
			{
				MCBlock *tbptr = bptr->next()->remove(blocks);
				findex_t i1, l1, i2, l2;
				bptr->GetRange(i1, l1);
				tbptr->GetRange(i2, l2);
				bptr->SetRange(i1, l1 + l2);
				delete tbptr;
				frag = True;
			
				t_blocks_changed = true;
			}
			else
			{
				bptr = bptr->next();
				frag = False;
			}
		}
		while (frag || bptr != blocks);
	}

	if (t_blocks_changed)
	{
		state |= PS_LINES_NOT_SYNCHED;
		
		// MP-2013-09-02: [[ FasterField ]] If we've changed the blocks, the lines need recomputed.
		needs_layout = true;
	}
}

// MW-2012-01-25: [[ ParaStyles ]] This method causes a reflow of the paragraph depending
//   on the setting of 'dontWrap'.
// AL-2014-09-22: [[ Bug 11817 ]] Added cascade parameter to enable conditional r
//  of subsequent paragraphs if the number of lines changes
bool MCParagraph::layout(bool p_force, bool p_check_redraw)
{
	// MP-2013-09-02: [[ FasterField ]] If we don't need layout, and layout isn't being forced,
	//   do nothing.
	if (!needs_layout && !p_force)
		return false;

    uindex_t t_count = 0;
    if (p_check_redraw)
        t_count = countlines();
    
    // Update the text direction properties of the paragraph
    resolvetextdirections();
    
	if (getdontwrap())
		noflow();
	else
		flow();
    
	// MP-2013-09-02: [[ FasterField ]] We've layed out the paragraph, so it doesn't need to
	//   be again until mutated.
	needs_layout = false;
    
    if (p_check_redraw)
        return t_count != countlines();
    
    return false;
}

uindex_t MCParagraph::countlines()
{
    MCLine *t_line = lines;
    uindex_t t_count = 0;
    do
    {
        t_count++;
        t_line = t_line -> next();
    }
    while (t_line != lines);
    return t_count;
}

//reflow paragraph with wrapping
void MCParagraph::flow(void)
{
	// MW-2008-03-24: [[ Bug 6194 ]] Make sure we clean the paragraph of broken blocks and empty
	//   blocks before we reflow - failing to do this causes strange effects when wrapping unicode
	//   text.
	defrag();

	// MW-2012-01-25: [[ ParaStyles ]] Compute the normal and first line layout width for
	//   wrapping purposes.
	int32_t pwidth, twidth;
    computelayoutwidths(pwidth, twidth);

    // SN-2015-01-21: [[ Bug 14229 ]] We want to keep the former lines, to be able
    //  to update the dirtywidth of a newly empty line with the former line length.
    MCLine *t_old_line;
    t_old_line = lines;
    lines = NULL;
    
    // Delete all existing lines and segments
    deletelines();
    
    // Initially, add all of the blocks to the one line (this segments them)
    MCLine *lptr = new (nothrow) MCLine(this);
    lptr->appendall(blocks, true);
    
    // Do the line wrapping
    do
    {
        // Add this line to the list of lines in this paragraph
        lptr->appendto(lines);
        
        // Do block fitting on this line and get back a line containing the
        // left-overs that would not fit into the line
        
        // SN-2015-01-21: [[ Bug 14229 ]] We update the dirtywidth of the
        // the new line with the former line.
        MCLine* t_leftover;
        t_leftover = lptr->Fit(twidth);
        
        if (t_old_line != NULL)
        {
            MCLine *t_line_to_remove;
            t_line_to_remove = t_old_line->remove(t_old_line);
            lptr -> takewidth(t_line_to_remove);
            delete t_line_to_remove;
        }
        
        lptr = t_leftover;
        
        // MW-2008-06-12: [[ Bug 6482 ]] Make sure we only take the firstIndent into account
		//   on the first line of the paragraph.
		twidth = pwidth;
    }
    while (lptr != NULL);
    
	state &= ~PS_LINES_NOT_SYNCHED;
}

//flow paragraph and don't wrap
void MCParagraph::noflow(void)
{
	// MW-2008-04-01: [[ Bug ]] Calling clearzeros meant styling of empty 
	// selections didn't work.
	defrag();
	deletelines();
    lines = new (nothrow) MCLine(this);
	lines->appendall(blocks, false);
    lines->NoFlowLayout();

	// MW-2012-02-10: [[ FixedTable ]] If there is a non-zero table width then
	//   use that as the line width.
	int32_t t_table_width;
	t_table_width = gettablewidth();
	if (t_table_width != 0)
		lines -> setwidth(t_table_width);

	state &= ~PS_LINES_NOT_SYNCHED;
}

// MW-2008-04-02: [[ Bug 6259 ]] Make sure front and back hilites are only
//   drawn if appropriate.
void MCParagraph::fillselect(MCDC *dc, MCLine *lptr, int2 x, int2 y, uint2 height, int2 sx, uint2 swidth)
{
	findex_t i, l;
	lptr->GetRange(i, l);
	if ((state & PS_FRONT && lptr == lines)
	        || (state & PS_BACK && lptr == lines->prev())
	        || (endindex >= i && startindex < i + l))
	{
        MCRectangle srect;
        
        bool t_show_front;
		t_show_front = (state & PS_FRONT) != 0 && (parent -> getflag(F_LIST_BEHAVIOR) || this != parent -> getparagraphs() || lptr != lines);

		bool t_show_back;
		t_show_back = (state & PS_BACK) != 0 && (parent -> getflag(F_LIST_BEHAVIOR) || this != parent -> getparagraphs() -> prev() || lptr != lines -> prev());

        bool t_show_all;
        t_show_all = t_show_front && t_show_back;
        
        // Do some adjustment of the start/end indices for block drawing purposes
        // (This is needed so that we have valid indices when highlighting comboboxes)
        findex_t startindex_draw, endindex_draw;
        startindex_draw = t_show_front ? i : startindex;
        endindex_draw = t_show_back ? i + l : endindex;
		
		// MW-2012-03-19: [[ Bug 10069 ]] The padding to use vertically depends on
		//   the vgrid property and such.
		int32_t t_padding;
		t_padding = getvpadding();
		
		// MW-2012-03-16: [[ Bug ]] Make sure padding is taken into account for
		//   the first and last lines in a paragraph.
		if (lptr == lines)
		{
			y -= t_padding;
			height += t_padding;
		}
		
		if (lptr == lines -> prev())
			height += t_padding;

		srect.y = y;
		srect.height = height;

		// MW-2012-09-04: [[ Bug 9759 ]] Adjust any pattern origin to scroll with text.
		parent->setforeground(dc, DI_HILITE, False, True);
		// MW-2012-11-22: [[ Bug 9759 ]] In listBehavior fields make hilite pattern origin
		//   top left of line.
		parent->adjustpixmapoffset(dc, DI_HILITE, parent -> getflag(F_LIST_BEHAVIOR) ? y + parent -> gettexty() - parent -> getcontenty() - TEXT_Y_OFFSET : 0);

		// MW-2012-01-08: [[ Paragraph Spacing ]] If we are the first line and
		//   the 'front' selection is being shown, then render the hilite over
		//   the space above the paragraph.
		if (t_show_front && lptr == lines)
		{
			// MW-2012-03-16: [[ Bug ]] The topmargin includes padding, which we
			//   don't want.
			int32_t t_space_height;
			t_space_height = computetopmargin() - t_padding;

			MCRectangle t_rect;
			MCU_set_rect(t_rect, sx, y - t_space_height, swidth, t_space_height);
			dc -> fillrect(t_rect);
		}
        
        // MW-2012-03-15: [[ Bug ]] If we have an implicit grid line, then don't fill over it!
		if (computetopborder() == 0 && gethgrid())
		{
			srect . y += 1;
			srect . height -= 1;
		}
        
        // Iterate over the blocks in the line
        MCBlock *firstblock, *lastblock;
        lptr->getblocks(firstblock, lastblock);
        MCBlock *bptr = firstblock;
        MCSegment *sgptr = segments;
        
        // AL-2014-08-13: [[ Bug 13108 ]] Use left and right of segments for boundaries of fill rect if necessary
        bool t_segment_front, t_segment_back, t_whole_segment;
        
        do
        {
            t_segment_front = false;
            t_segment_back = false;
            t_whole_segment = false;
            
            // Is part of this block selected?
            findex_t bi, bl;
            bptr->GetRange(bi, bl);
            
            // AL-2014-07-29: [[ Bug 12951 ]] Selection rect should include whitespace between tabbed cells
            // If this is the first block of a segment, check if the selection covers the front of the segment.
            // SN-2014-08-22: [[ Bug 13249 ]] startindex and endindex are not set when hiliting the whole line of a field.
            if (bptr == sgptr -> GetFirstBlock() && endindex > bi && (startindex < bi || (t_show_front && (startindex == bi || startindex == INT32_MAX))))
            {
                t_segment_front = true;
                findex_t ei, el;
                MCBlock *t_seg_last = sgptr -> GetLastBlock();
                t_seg_last -> GetRange(ei, el);
                
                if (endindex > ei + el || (t_show_back && endindex == bi + bl))
                    t_whole_segment = true;
            }
            
            // If this is the last block of a segment, check if the selection covers the back of the segment.
            // SN-2014-08-22: [[ Bug 13249 ]] startindex and endindex are not set when hiliting the whole line of a field.
            if (!t_whole_segment && bptr == sgptr -> GetLastBlock() && startindex < bi + bl &&
                ((sgptr -> next() != segments && endindex > bi + bl) || (t_show_back && (endindex == bi + bl || endindex == INT32_MAX))))
                t_segment_back = true;

             // If selection covers the whole segment, we can fill it and skip to the first block of the next segment.           
            if (t_whole_segment)
            {
                srect . x  = x + sgptr -> GetLeft();
                srect . width = sgptr -> GetWidth();
                dc->fillrect(srect);
                
                bptr = sgptr -> GetLastBlock();
            }
            else if (t_show_all || (startindex <= bi + bl && endindex >= bi))
            {
                findex_t si, ei;
                if (startindex_draw > bi)
                    si = startindex_draw;
                else
                    si = bi;
                if (endindex_draw < bi + bl)
                    ei = endindex_draw;
                else
                    ei = bi + bl;
                
                // Get the X coordinates for the selection
                coord_t bix, bex;
                // SN-2014-08-14: [[ Bug 13106 ]] GetCursorX includes the cell padding, which we don't want
                bix = bptr->GetCursorX(si) - sgptr -> GetPadding();
                bex = bptr->GetCursorX(ei) - sgptr -> GetPadding();
                
                // AL-2014-07-17: [[ Bug 12823 ]] Include segment offset in the block coordinate calculation
                // Re-ordering will be required if the block is RTL
                if (bix > bex)
                {
                    coord_t t_temp;
                    t_temp = bix;
                    bix  = bex;
                    bex = t_temp;
                }
                
                coord_t temp_x,temp_width;

                if (t_segment_front)
                    temp_x = x + sgptr -> GetLeft();
                else
                    temp_x = x + sgptr -> GetLeftEdge() + bix;
                
                // AL-2014-07-29: [[ Bug 12951 ]] If selection traverses a segment boundary, include the boundary in the fill rect.
                
                srect.x = MCClamp(temp_x, INT16_MIN, INT16_MAX);
                
                if (t_segment_back)
                    temp_width = x + sgptr -> GetRight() - srect . x;
                else
                    temp_width = x + sgptr -> GetLeftEdge() + bex - srect . x;
                
                srect.width = MCClamp(temp_width, INT16_MIN, INT16_MAX);
                
                // Draw this block
                dc->fillrect(srect);
            }
            
            // AL-2014-03-28: [[ Bug 11960 ]] Break out of the loop if we have dealt with the last block
            //  in this line, otherwise GetCursorX will not be happy with the index passed to it.
            if (bptr == lastblock)
                break;
            
            bptr = bptr->next();
            
            // AL-2014-07-17: [[ Bug 12823 ]] Block origin is relative to the segment, so keep track
            //  of the current segment to get correct selection coordinates.
            // Advance to the next segment, if necessary
            if (sgptr->next()->GetFirstBlock() == bptr)
                sgptr = sgptr->next();
        }
        while (bptr != firstblock);
        
        // Draw the left-hand side, if required
        if (t_show_front || startindex < i)
        {
            // AL-2014-07-30: [[ Bug 12924 ]] Get first visual block in the line for front selection fill
            MCBlock *t_first_visual = lptr -> GetFirstSegment() -> GetFirstVisualBlock();
            
            srect.x = sx;
            // AL-2014-07-17: [[ Bug 12951 ]] Include segment offset in the block coordinate calculation
            srect.width = x + lptr -> GetFirstSegment() -> GetLeftEdge() + t_first_visual->getorigin() - sx;
            dc->fillrect(srect);
        }
        
        // Draw the right-hand side, if required
        if (t_show_back || endindex > i + l)
        {
            // AL-2014-07-17: [[ Bug 12951 ]] Include segment offset in the block coordinate calculation
            // SN-2014-09-11: [[ Bug 13407 ]] Include the part not drawn in case of text overflow
            srect.x = x + lptr -> GetLastSegment() -> GetRightEdge();
            srect.width = swidth - (srect.x - sx);
            dc->fillrect(srect);
        }
        

		//dc->fillrect(srect);

		// MW-2012-01-08: [[ Paragraph Spacing ]] If we are the last line and
		//   the 'back' selection is being shown, then render the hilite over
		//   the space above the paragraph.
		if (t_show_back && lptr -> next() == lines)
		{
			// MW-2012-03-16: [[ Bug ]] The bottommargin includes padding, which
			//   we don't want.
			int32_t t_space_height;
			t_space_height = computebottommargin() - t_padding;

			MCRectangle t_rect;
			MCU_set_rect(t_rect, sx, y + height, swidth, t_space_height);
			dc -> fillrect(t_rect);
		}
		parent->setforeground(dc, DI_FORE, False, True);
		parent->draw3dhilite(dc, srect);
	}
}

//draw box around found text
void MCParagraph::drawcomposition(MCDC *dc, MCLine *lptr, int2 x, int2 y, uint2 height, findex_t compstart, findex_t compend, findex_t compconvstart, findex_t compconvend)
{
	findex_t i, l;
	lptr->GetRange(i, l);
	if (compstart >= i || compend >= i)
	{
		MCRectangle srect;
		srect.x = x;
		srect.y = y;
		srect.height = height;
		if (compstart > i)
			srect.x += lptr->GetCursorXPrimary(compstart, moving_forward);
		if (compend > i + l)
			srect.width = lptr->getwidth() - (srect.x - x);
		else
			srect.width = x + lptr->GetCursorXPrimary(compend, moving_forward) - srect.x;
		dc->setforeground(dc->getgray());
		dc->drawline(srect.x, srect.y + srect.height - 1, srect.x + srect.width,
		             srect.y + srect.height - 1);
		dc->setforeground(dc->getblack());
		dc->setlineatts(2, LineSolid, CapButt, JoinBevel);
		if (compconvstart != compconvend)
		{
			compconvstart += compstart;
			compconvend += compstart;
			srect.x = x;
			srect.y = y;
			srect.height = height;
			if (compconvstart > i)
				srect.x += lptr->GetCursorXPrimary(compconvstart, moving_forward);
			if (compconvend > i + l)
				srect.width = lptr->getwidth() - (srect.x - x);
			else
				srect.width = x + lptr->GetCursorXPrimary(compconvend, moving_forward) - srect.x;

			dc->drawline(srect.x, srect.y + srect.height - 1, srect.x + srect.width,
			             srect.y + srect.height - 1);
		}
		else
			if (IsMacLFAM())
				dc->drawline(srect.x, srect.y + srect.height - 1, srect.x + srect.width,
				             srect.y + srect.height - 1);
		dc->setlineatts(0, LineSolid, CapButt, JoinBevel);
		parent->setforeground(dc, DI_FORE, False, True);
	}
}

// MW-2007-07-05: [[ Bug 110 ]] - Make sure the find box is continued over multiple lines
void MCParagraph::drawfound(MCDC *dc, MCLine *lptr, int2 x, int2 y, uint2 height, findex_t fstart, findex_t fend)
{
	findex_t i, l;
	lptr->GetRange(i, l);
	if (fstart < i + l && fend > i)
	{
		MCRectangle srect;
		srect.x = x;
		srect.y = y;
		srect.height = height;

		bool t_has_start;
		t_has_start = fstart >= i;
		if (fstart > i)
			srect.x += lptr->GetCursorXPrimary(fstart, moving_forward);

		bool t_has_end;
		t_has_end = fend <= i + l;
		if (fend > i + l)
			srect.width = lptr->getwidth() - (srect.x - x);
		else
			srect.width = x + lptr->GetCursorXPrimary(fend, moving_forward) - srect.x;

		parent->setforeground(dc, DI_FORE, False, True);
		if (t_has_start && t_has_end)
			dc->drawrect(srect);
		else
		{
			dc ->drawline(srect . x, srect . y, srect . x + srect . width - 1, srect . y);
			dc ->drawline(srect . x, srect . y + srect . height - 1, srect . x + srect . width - 1, srect . y + srect . height - 1);
			if (t_has_start)
				dc -> drawline(srect . x, srect . y, srect . x, srect . y + srect . height - 1);
			if (t_has_end)
				dc -> drawline(srect . x + srect . width - 1, srect . y, srect . x + srect . width - 1, srect . y + srect . height - 1);
		}
	}
}

//draw text of paragraph
void MCParagraph::draw(MCDC *dc, int2 x, int2 y, uint2 fixeda,
                       uint2 fixedd, findex_t fstart, findex_t fend, findex_t compstart,
                       findex_t compend, findex_t compconvstart, findex_t compconvend,
                       uint2 textwidth, uint2 pgheight, uint2 sx, uint2 swidth, uint2 pstyle)
{
	if (lines == NULL)
		return;
		
	// MW-2012-03-05: [[ HiddenText ]] If the paragraph is not visible, then don't draw
	//   anything.
	if (gethidden())
		return;

	MCRectangle t_clip;
	t_clip = dc -> getclip();
	
	dc->save();

	coord_t ascent, descent, leading, linespace;
	ascent = fixeda;
	descent = fixedd;
    leading = 0;

	// MW-2012-03-16: [[ Bug 10001 ]] Compute the paragraph offset (from leftmargin) and minimal width.
	int32_t t_paragraph_offset, t_paragraph_width;
	computeparaoffsetandwidth(t_paragraph_offset, t_paragraph_width);

	// MW-2012-03-15: [[ Bug 10001 ]] Compute the selection offset and width. Notice that
	//   the width is at least the textwidth.
	int32_t t_select_x, t_select_width;
	t_select_x = x;
	t_select_width = MCMax(t_paragraph_width, textwidth);

	// If the field is in listbehavior mode the selection fill also covers the left and
	// right margins.
	if (parent -> getflag(F_LIST_BEHAVIOR))
	{
		t_select_x -= parent -> getleftmargin() + DEFAULT_BORDER - parent -> getborderwidth();
		t_select_width += parent -> getleftmargin() + parent -> getrightmargin() + 2 * DEFAULT_BORDER - 2 * parent -> getborderwidth();
	}

	// MW-2012-01-25: [[ ParaStyles ]] Compute the inner and outer rects of the
	//   paragraph.
	MCRectangle t_outer_rect(kMCEmptyRectangle);
	MCRectangle t_inner_rect(kMCEmptyRectangle);
	computerects(x, y, textwidth, t_paragraph_width, pgheight, t_outer_rect, t_inner_rect); 

	// MW-2012-02-09: [[ ParaStyles ]] Compute the inner rect excluding padding (for
	//   border and background rendering).
	// MW-2012-03-19: [[ Bug 10069 ]] Make sure the appropriate h/v padding is used to
	//   adjust the rect.
	MCRectangle t_inner_border_rect;
	t_inner_border_rect . x = t_inner_rect . x - gethpadding();
	t_inner_border_rect . width = t_inner_rect . width + 2 * gethpadding();
	t_inner_border_rect . y = t_inner_rect . y - getvpadding();
	t_inner_border_rect . height = t_inner_rect . height + 2 * getvpadding();

	// MW-2012-02-10: [[ FixedTable ]] Adjust the rects to take into account any
	//   fixed width table mode.
	adjustrectsfortable(t_inner_border_rect, t_outer_rect);

	// MW-2012-01-25: [[ ParaStyles ]] If the paragraph has a background color
	//   then fill it in.
	if (attrs != nil && (attrs -> flags & PA_HAS_BACKGROUND_COLOR) != 0)
	{
		MCColor t_color;
		MCColorSetPixel(t_color, attrs -> background_color);
		dc -> setforeground(t_color);
		dc -> fillrect(t_inner_border_rect);
		parent->setforeground(dc, DI_FORE, False, True);
	}

	// Compute the top of the first line.
	int32_t t_current_y;
	t_current_y = t_inner_rect . y;

	findex_t si, ei;
	getselectionindex(si, ei);

	MCLine *lptr;
	lptr = lines;
	do
	{
		if (fixeda == 0)
		{
			ascent = lptr -> GetAscent();
			descent = lptr -> GetDescent();
            leading = lptr -> GetLeading();
		}
        
        linespace = ascent + descent + leading;

		if (t_current_y < t_clip . y + t_clip . height && t_current_y + ascent + descent > t_clip . y)
		{
			int32_t t_current_x;
			t_current_x = t_inner_rect . x + computelineinneroffset(t_inner_rect . width, lptr);

			if (startindex != endindex || state & PS_FRONT || state & PS_BACK)
				fillselect(dc, lptr, t_current_x, t_current_y, ceilf(linespace), t_select_x, t_select_width);

			uint32_t t_list_style;
			t_list_style = getliststyle();
			if (t_list_style != kMCParagraphListStyleNone && t_list_style != kMCParagraphListStyleSkip && lptr == lines)
			{
				char t_index_buffer[PG_MAX_INDEX_SIZE];
				const char *t_string;
				uint32_t t_string_length;
				bool t_is_unicode;
				if (t_list_style < kMCParagraphListStyleNumeric)
				{
					switch(t_list_style)
					{
					case kMCParagraphListStyleDisc:
						t_string = (const char *)L"\u2022";
						break;
					case kMCParagraphListStyleCircle:
						t_string = (const char *)L"\u25E6";
						break;
					case kMCParagraphListStyleSquare:
						t_string = (const char *)L"\u25AA";
						break;
					}
					t_string_length = 2;
					t_is_unicode = true;
				}
				else
				{
					computeliststyleindex(parent -> getparagraphs() -> prev(), t_index_buffer, t_string, t_string_length);
					t_is_unicode = false;
				}

				// MW-2012-02-06: [[ Bug ]] If the front part of the paragraph is selected,
				//   then make sure we change color to background, draw text, then reset to
				//   foreground.
				if ((state & PS_FRONT) != 0 && this != parent -> getparagraphs())
				{
					// MW-2012-03-07: [[ Bug 10059 ]] Set the text color appropriately for the
					//   list labels.
					if (IsMacLF() && !parent->isautoarm())
					{
						MCPatternRef t_pattern;
						int2 t_x, t_y;
						MCColor fc, hc;
						parent->getforecolor(DI_FORE, False, True, fc, t_pattern, t_x, t_y, dc -> gettype(), parent);
						parent->getforecolor(DI_HILITE, False, True, hc, t_pattern, t_x, t_y, dc -> gettype(), parent);
						if (MCColorGetPixel(hc) == MCColorGetPixel(fc))
							parent->setforeground(dc, DI_BACK, False, True);
					}
					else
						parent->setforeground(dc, DI_BACK, False, True);
				}
                
                MCAutoStringRef t_stringref;
                if (t_is_unicode)
                    /* UNCHECKED */ MCStringCreateWithChars((const unichar_t*)t_string, t_string_length, &t_stringref);
                else
                    /* UNCHECKED */ MCStringCreateWithNativeChars((const char_t*)t_string, t_string_length, &t_stringref);
                
                dc -> drawtext(t_current_x - getlistlabelwidth(), ceilf(t_current_y + ascent - 1), *t_stringref, parent->getfontref(), false);
                
				if ((state & PS_FRONT) != 0 && this != parent -> getparagraphs())
					parent -> setforeground(dc, DI_FORE, False, True);
			}

			lptr->draw(dc, t_current_x, ceilf(t_current_y + ascent - 1), si, ei, *m_text, pstyle);
			if (fstart != fend)
				drawfound(dc, lptr, t_current_x, t_current_y, ceilf(linespace), fstart, fend);
			if (compstart != compend)
				drawcomposition(dc, lptr, t_current_x, t_current_y, ceilf(linespace), compstart, compend, compconvstart, compconvend);
		}

		t_current_y += ceilf(linespace);

		lptr = lptr->next();
	}
	while (lptr != lines);

	dc->restore();
	
	// MW-2012-01-08: [[ Paragraph Border ]] Render the paragraph's border (if
	//   any).
	if (!MCU_equal_rect(t_inner_border_rect, t_outer_rect) || gethgrid() || getvgrid())
	{
		if (attrs != nil && (attrs -> flags & PA_HAS_BORDER_COLOR) != 0)
		{
			MCColor t_color;
			MCColorSetPixel(t_color, attrs -> border_color);
			dc -> setforeground(t_color);
		}
		else
		{
			// MW-2012-09-04: [[ Bug 9759 ]] Adjust any pattern origin to scroll with text.
			parent->setforeground(dc, DI_BORDER, False, True);
			parent->adjustpixmapoffset(dc, DI_BORDER);
		}

		// MW-2012-03-15: [[ Bug 10069 ]] If we have no border, and hgrid is set on the parent but not
		//   this paragraph we are in compatibility mode. This means:
		//     1) No top line is rendered.
		//     2) The hgrid lines extend full width of field.
		bool t_compat_hgrid;
		t_compat_hgrid = (attrs == nil || !(attrs -> flags & PA_HAS_HGRID)) && (parent -> getflags() & F_HGRID) != 0 && getborderwidth() == 0;

		// Fill the top border - slightly complicated by the need to handle the
		// implicit 'grid' case.
		// MW-2012-03-15: [[ Bug 10069 ]] Skip rendering the top border if in hgrid compat mode.
		if (!t_compat_hgrid)
		{
			if (!gethgrid() || !elidetopborder() || this == parent -> getparagraphs())
				dc -> fillrect(MCU_make_rect(t_outer_rect . x, t_outer_rect . y, t_outer_rect . width, MCMax(gethgrid() ? 1 : 0, t_inner_border_rect . y - t_outer_rect . y)));
			else
			{
				MCRectangle t_prev_inner(kMCEmptyRectangle);
				MCRectangle t_prev_outer(kMCEmptyRectangle);
				prev() -> computerects(x, y, textwidth, prev() -> getwidth(), pgheight, t_prev_outer, t_prev_inner);
				
				// MW-2012-02-10: [[ FixedTable ]] The adjustrects method uses both rects so make
				//   sure we adjust the prev inner rect for padding.
				// MW-2012-03-19: [[ Bug 10069 ]] Make sure the appropriate h/v padding is used to
				//   adjust the rect.
				// MW-2013-08-08: [[ Bug 10616 ]] Previously was making t_prev_inner equal to t_inner_rect
				//   adjusted for padding, causing incorrect length of hline.
				t_prev_inner . x = t_prev_inner . x - prev() -> gethpadding();
				t_prev_inner . width = t_prev_inner . width + 2 * prev() -> gethpadding();
				t_prev_inner . y = t_prev_inner . y - prev() -> getvpadding();
				t_prev_inner . height = t_prev_inner . height + 2 * prev() -> getvpadding();
				
				// MW-2012-02-10: [[ FixedTable ]] Adjust the outer rect to take into account any
				//   fixed width table mode.
				prev() -> adjustrectsfortable(t_prev_inner, t_prev_outer);

				int32_t t_left, t_right;
				t_left = MCMin(t_outer_rect . x, t_prev_outer . x);
				t_right = MCMax(t_outer_rect . x + t_outer_rect . width, t_prev_outer . x + t_prev_outer . width);
				dc -> fillrect(MCU_make_rect(t_left, t_outer_rect . y, t_right - t_left, MCMax(1, t_inner_border_rect . y - t_outer_rect . y)));
			}
		}

		// Fill the left border.
		dc -> fillrect(MCU_make_rect(t_outer_rect . x, t_inner_border_rect . y, t_inner_border_rect . x - t_outer_rect . x, t_inner_border_rect . height));

		// Fill the right border.
		dc -> fillrect(MCU_make_rect(t_inner_border_rect . x + t_inner_border_rect . width, t_inner_border_rect . y, t_outer_rect . x + t_outer_rect . width - (t_inner_border_rect . x + t_inner_border_rect . width), t_inner_border_rect . height));
		
		// Fill the bottom border - slightly complicated by the need to handle the
		// implicit 'grid' case.
		// MW-2012-03-15: [[ Bug 10069 ]] If in hgrid compat mode, render a compatibility border.
		if (!t_compat_hgrid)
		{
			int32_t t_bottom_border;
			t_bottom_border = t_outer_rect . y + t_outer_rect . height - (t_inner_border_rect . y + t_inner_border_rect . height);
			if (gethgrid() && t_bottom_border == 0 && !elidebottomborder())
				t_bottom_border = 1;
			dc -> fillrect(MCU_make_rect(t_outer_rect . x, t_inner_border_rect . y + t_inner_border_rect . height, t_outer_rect . width, t_bottom_border));
		}
		else
			dc -> fillrect(MCU_make_rect(parent -> getrect() . x, t_inner_border_rect . y + t_inner_border_rect . height, parent -> getrect() . width, 1));
		
		// Render the cell boundaries (if vGrid set).
		if (getvgrid())
		{
			uint2 *t;
			uint2 nt;
			Boolean fixed;
			gettabs(t, nt, fixed);

			// MW-2012-02-10: Compute the delta between tab and field offset.
			int32_t t_delta;
			t_delta = t_inner_border_rect . x + computelineinneroffset(t_inner_border_rect . width, lines) - 1;

			// MW-2012-03-15: [[ Bug 10069 ]] Continue the vgrid lines all the way across.
			int32_t t_limit;
			t_limit = parent -> getrect() . x + parent -> getrect() . width;

			uint2 ct = 0; 
			int4 t_x = t[0] + t_delta;
			while (t_x <= t_limit)
			{
				dc->drawline(t_x, t_inner_border_rect . y, t_x, t_inner_border_rect . y + t_inner_border_rect . height);
				
				if (ct < nt - 1)
					t_x = t_delta + t[++ct];
				else if (nt == 1)
					t_x += t[0];
				else
					t_x += t[nt - 1] - t[nt - 2];

				// MW-2012-02-10: [[ FixedTable ]] If we have reached the final tab in fixed
				//   table mode, we are done.
				// MW-2013-05-20: [[ Bug 10878 ]] Tweaked conditions to work for min two tabStops
				//   rather than 3.
                // MW-2015-05-28: [[ Bug 12341 ]] Only stop rendering lines if in 'fixed width table'
                //   mode - indicated by the last two tabstops being the same.
				if (nt >= 2 && t[nt - 1] == t[nt - 2] && ct == nt - 1)
					break;
			}
		}

		parent->setforeground(dc, DI_FORE, False, True);
	}
}

// MW-2008-03-27: [[ Bug 5093 ]] Rewritten to more correctly insert blocks around
//   imagesource characters.
MCBlock *MCParagraph::indextoblock(findex_t tindex, Boolean forinsert, bool for_navigation)
{
	if (blocks == NULL)
		inittext();
	
	if (forinsert)
	{
		MCBlock *t_block;
		t_block = blocks;
		findex_t i, l;
		do
		{
			MCBlock *t_next_block;
			t_next_block = t_block -> next() != blocks ? t_block -> next() : NULL;
			
			bool t_next_block_is_null;
			t_next_block_is_null = t_next_block != NULL && t_next_block -> GetLength() == 0;
			
			t_block -> GetRange(i, l);
			// If we are at the end of a block, and the next block is empty then
			// we want to insert into that block.
			if (tindex >= i && (tindex < i + l || (tindex == i + l && !t_next_block_is_null)))
			{
				// If this block hasn't got an image source, then its the
				// one we want.
				if (t_block -> getimagesource() == NULL)
					return t_block;

				if (tindex == i)
				{
					// If we are inserting at the beginning of the image source block
					// then we may need to insert a new block before it if we are the
					// first
					if (t_block == blocks)
					{
                        MCExecContext ctxt(nil, nil, nil);
						MCBlock *t_new_block;
						t_new_block = new (nothrow) MCBlock(*t_block);
                        t_new_block -> SetImageSource(ctxt, kMCEmptyString);

						// MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
						//   fontref so it can compute its.
						// MW-2012-03-13: [[ Bug 10082 ]] Make sure we are open before opening the
						//   block.
						if (opened)
							t_new_block -> open(parent -> getfontref());

						t_new_block -> SetRange(i, 0);
						t_new_block -> insertto(blocks);
						return t_new_block;
					}

					// This should never happen as the previous block would have been
					// processed and returned.
					assert(false);
					return t_block -> prev();
				}
				else
				{
					// If we are inserting at the end of the image source block,
					// then we may need to insert a new block if there are no more.
					if (t_block -> next() == blocks)
					{
                        MCExecContext ctxt(nil, nil, nil);
						MCBlock *t_new_block;
						t_new_block = new (nothrow) MCBlock(*t_block);
                        t_new_block -> SetImageSource(ctxt, kMCEmptyString);

						// MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
						//   fontref so it can compute its.
						// MW-2012-03-13: [[ Bug 10082 ]] Make sure we are open before opening the
						//   block.
						if (opened)
							t_new_block -> open(parent -> getfontref());

						t_new_block -> SetRange(i + l, 0);
						t_block -> append(t_new_block);
						return t_new_block;
					}

					// Otherwise just insert into the next block
					return t_block -> next();
				}
			}
			t_block = t_block -> next();
		}
		while(t_block != blocks);

		// In theory we shouldn't be able to get here since the entire range of
		// characters should be covered by blocks. Return the last block just in
		// case though.
		assert(false);
		return blocks -> prev();
	}

	if (tindex == PARAGRAPH_MAX_LEN)
		tindex = 0;

	MCBlock *bptr = blocks;
	findex_t i, l;
	do
	{
		bptr->GetRange(i, l);
		if (tindex >= i && tindex <= i + l)
		{
			if (tindex == i + l && (!for_navigation || !moving_forward) && bptr->next() != blocks)
				bptr = bptr->next();
			return bptr;
		}
		bptr = bptr->next();
	}
	while (bptr != blocks);
	return blocks->prev();
}

MCLine *MCParagraph::indextoline(findex_t tindex)
{
	MCLine *lptr = lines;
	findex_t i, l;
	do
	{
		lptr->GetRange(i, l);
		if (tindex >= i && tindex < i + l)
			return lptr;
		lptr = lptr->next();
	}
	while (lptr != lines);
	return lines->prev();
}

// MW-2014-05-28: [[ Bug 12303 ]] Special-case added for when setting 'text' of field chunks. If
//   'preserve_if_zero' is true, then 'this' paragraph's styles are preserved even if it has no
//   text. This is used in the case of 'set the text of <chunk>' to ensure that paragraph properties
//   of the first paragraph the text is set in do not get clobbered.
void MCParagraph::join(bool p_preserve_zero_length_styles_if_zero)
{
	if (blocks == NULL)
		inittext();

	MCParagraph *pgptr = next();
	
	// MW-2012-01-07: If the current paragraph is empty and has no attrs, then copy
	//   the next paragraphs attrs.
	// MW-2012-08-31: [[ Bug 10344 ]] If the textsize is 0 then always take the next
	//   paragraphs attrs.
	if (!p_preserve_zero_length_styles_if_zero && gettextlength() == 0)
	// MW-2014-05-28: [[ Bug 12303 ]] If the textsize is 0 and we don't want to preserve the style
    //   changes, then copy the next paragraph's.
		copyattrs(*pgptr);

	// MW-2006-04-13: If the total new text size is greater than 65536 - 34 we just delete the next paragraph
	uint4 t_new_size;
	t_new_size = gettextlengthcr() + pgptr -> gettextlength();;
	if (t_new_size > PARAGRAPH_MAX_LEN - PG_PAD - 1)
	{
		delete pgptr;
		return;
	}

	focusedindex = MCStringGetLength(*m_text);
	/* UNCHECKED */ MCStringAppend(*m_text, *pgptr->m_text);

	MCBlock *bptr = blocks->prev();
	bptr->append(pgptr->blocks);
	bptr = pgptr->blocks;
	do
	{
		bptr->MoveRange(focusedindex, 0);
		bptr->setparent(this);
		bptr = bptr->next();
	}
	while (bptr != blocks);
	
	pgptr->blocks = NULL;
	delete pgptr;
	clearzeros();
	deletelines();
	
	// MP-2013-09-02: [[ FasterField ]] Joining two paragraphs requires layout.
	needs_layout = true;
}

void MCParagraph::replacetextwithparagraphs(findex_t p_start, findex_t p_finish, MCParagraph *p_pglist)
{
    // Remove characters to replace
    deletestring(p_start, p_finish);

    // Split the paragraph if needed
    if (p_start < MCStringGetLength(*m_text))
        split(p_start);

    // Append the right part of the split to the end
    // of p_pglist if it exists
    if (next() != this)
    {
        // p_pglist must be inserted between this and next()
        // t_last_paragraph is meant to point to the last
        // paragraph of p_pglist
        MCParagraph *t_last_paragraph = p_pglist;
        for (; t_last_paragraph -> next() != p_pglist; t_last_paragraph = t_last_paragraph -> prev())
            ;
        t_last_paragraph -> append(next());
    }

    // Append p_pglist to this if this is not the only, empty paragraph
    if (MCStringIsEmpty(*m_text) && next() == this)
    {
        MCParagraph *t_old = this;
        delete t_old;
    }
    else
        append(p_pglist);
}

void MCParagraph::split() //split paragraphs on return
{
    split(focusedindex);
}

void MCParagraph::split(findex_t p_position)
{
    MCBlock *bptr = indextoblock(p_position, False);
	findex_t skip = 0;
	
    // Reinstate original check for the presence of '\n' after the split.
    // We believe this check was there as previously when importing text
    // into a field, it would be set in a paragraph and then the paragraph
    // would be iteratively split - which entailed removing the \n's.
    // The 'm_text' field of a paragraph should never contain '\n' now so
    // whilst we leave this check in (to be on the safe side) it should never
    // trigger - hence the assert.
    if (p_position < MCStringGetLength(*m_text) && GetCodepointAtIndex(p_position) == '\n')
    {
        MCAssert(false);
        skip = IncrementIndex(p_position) - p_position;
    }
    
	MCParagraph *pgptr = new (nothrow) MCParagraph;
	pgptr->parent = parent;

	// MW-2012-01-25: [[ ParaStyles ]] Copy the attributes from the first para.
	pgptr -> copyattrs(*this);
	// MW-2012-11-20: [[ ParaListIndex]] When splitting a paragraph we don't copy the
	//   list index.
	pgptr -> setlistindex(0);

    pgptr->m_text.Reset();
	if (!MCStringIsEmpty(*m_text))
	{
        MCRange t_range = MCRangeMakeMinMax(p_position, MCStringGetLength(*m_text));
		/* UNCHECKED */ MCStringMutableCopySubstring(*m_text, t_range, &pgptr->m_text);
        /* UNCHECKED */ MCStringSubstring(*m_text, MCRangeMake(0, p_position));
	}
	else
		/* UNCHECKED */ MCStringCreateMutable(0, &pgptr->m_text);

	// Trim the block containing the split so that it ends at the split point
    bptr = indextoblock(p_position, False);
	findex_t i, l;
	bptr->GetRange(i, l);
    bptr->MoveRange(0, p_position - (i + l));
	
	// Create a new block to cover the range from the split point to the end
	// of the original block.
	MCBlock *tbptr = new (nothrow) MCBlock(*bptr);
	bptr->append(tbptr);
	blocks->splitat(tbptr);
	pgptr->blocks = tbptr;
	tbptr->setparent(pgptr);
    tbptr->SetRange(0, (i + l) - p_position - skip);
	tbptr = tbptr->next();
	
	// Adjust the blocks after the split as they now belong to the new
	// paragraph (and therefore have different indices, too).
	while (tbptr != pgptr->blocks)
	{
		tbptr->setparent(pgptr);
        tbptr->MoveRange(-(p_position + skip), 0);
		tbptr->setparent(pgptr);
		tbptr = tbptr->next();
	}

    // Set the focusedindex at the right position
    if (focusedindex >= MCStringGetLength(*m_text))
    {
        pgptr -> focusedindex = focusedindex - MCStringGetLength(*m_text);
        focusedindex = 0;
    }
	
	// MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
	//   fontref so it can compute its.
	if (opened)
		pgptr->open(parent -> getfontref());
	append(pgptr);
	deletelines();
	
	// MP-2013-09-02: [[ FasterField ]] Splitting a paragraph requires layout.
	needs_layout = true;
}

void MCParagraph::deletestring(findex_t si, findex_t ei, MCFieldStylingMode p_styling_mode)
{
	MCBlock *sbptr = indextoblock(si, False);
	MCBlock *ebptr = indextoblock(ei, False);
	
	// Don't try to remove text beyond the end of the paragraph
	if (ei > gettextlength())
		return;
	
	findex_t length = ei - si;
	if (focusedindex >= ei)
		focusedindex -= length;
	else
		if (focusedindex > si)
			focusedindex = si;
	startindex = endindex = originalindex = focusedindex;
	
	// If the styling mode is 'from after' then we must ensure that at si
	// the style is the same as the first char in the deleted string. To
	// acheive this we insert a zero length block with the same style as
	// the char after si. If si is within a block (not at the start) then
	// we don't need to do anything.
	if (p_styling_mode == kMCFieldStylingFromAfter)
	{
		if (si == sbptr -> GetOffset())
		{
			sbptr -> split(si);
			if (ebptr == sbptr)
				ebptr = sbptr -> next();
			sbptr = sbptr -> next();
		}
	}
	
	// If the styling mode is 'none' then we must ensure that at si
	// there is a block with no style. This involves splitting sbptr at
	// si (if it is not at the start) and then inserting a plain MCBlock.
	if (p_styling_mode == kMCFieldStylingNone)
	{
		if (si != sbptr -> GetOffset())
		{
			sbptr -> split(si);
			if (ebptr == sbptr)
				ebptr = sbptr -> next();
			sbptr = sbptr -> next();
		}
		MCBlock *t_empty_block;
		t_empty_block = new (nothrow) MCBlock;
		t_empty_block -> setparent(this);
		t_empty_block -> SetRange(si, 0);
		if (sbptr == blocks)
			t_empty_block -> insertto(blocks);
		else
			sbptr -> prev() -> append(t_empty_block);
		if (opened)
			t_empty_block -> open(getparent() -> getfontref());
	}
	
	// We want to make sure we clear all zero blocks *apart* from one
	// inserted to enforce the styling mode - the place we want to
	// remove them from is always sbptr at this point.
	MCBlock *t_clear_zeros_from;
	t_clear_zeros_from = sbptr;
	
	if (sbptr == ebptr)
	{
		// If the start block and the end block are the same block, the range
		// adjustment is nearly trivial.
		sbptr->MoveRange(0, -length);
		sbptr = sbptr->next();
	}
	else
	{
		// A range of blocks is affected
		findex_t i, l;
		sbptr->GetRange(i, l);
		findex_t ld = 0;
		
		if (i != si)
		{
			// If the range to be removed does not start on a block boundary,
			// the first affected block needs to be shortened.
			ld = l - (si - i);
			sbptr->MoveRange(0, -ld);
			sbptr = sbptr->next();
		}
		else
			t_clear_zeros_from = ebptr;
		
		sbptr->GetRange(i, l);
		while (sbptr != ebptr)
		{
			// These blocks are entirely convered by the range to be removed
			// and can be dropped with no adjustment required.
			ld += l;
			if (sbptr == blocks)
			{
				// Removing the first block of this paragraph so the head block
				// pointer needs to be adjusted.
				MCBlock *tsbptr = blocks->remove(blocks);
				delete tsbptr;
				sbptr = blocks;
			}
			else
			{
				// Not the first block in the block list
				MCBlock *tsbptr = sbptr->remove(sbptr);
				delete tsbptr;
			}
			sbptr->GetRange(i, l);
		}
		
		// We are now at the last affected block.
		sbptr->MoveRange(-ld, ld - length);
		sbptr = sbptr->next();
	}
	
	// All blocks after the deleted text also require updates
	while (sbptr != blocks)
	{
		sbptr->MoveRange(-length, 0);
		sbptr = sbptr->next();
	}
	
	// Excise the deleted range from the paragraph text
	/* UNCHECKED */ MCStringRemove(*m_text, MCRangeMakeMinMax(si, ei));

	// Eliminate any zero length blocks *after* one we might have ensured
	// is present for styling purposes.
	clearzeros(t_clear_zeros_from);
	
	state |= PS_LINES_NOT_SYNCHED;
	
	// MP-2013-09-02: [[ FasterField ]] Deleting a string requires layout.
	needs_layout = true;
}

MCParagraph *MCParagraph::copystring(findex_t si, findex_t ei)
{
	// The string is copied by duplicating this paragraph and then removing all
	// text outside of the range [si, ei). This preserves all attributes, etc
	MCParagraph *pgptr = new (nothrow) MCParagraph(*this);
	
	if (ei != MCStringGetLength(*m_text))
	{
		// Discard any text after the desired end index,
		pgptr->focusedindex = ei;
		pgptr->split();
		MCParagraph *tptr = pgptr->next()->remove(pgptr);
		delete tptr;
	}
	
	if (si != 0)
	{
		// Discard any text before the desired start index
		pgptr->focusedindex = si;
		pgptr->split();
		pgptr = pgptr->next();
		MCParagraph *tptr = pgptr->prev()->remove(pgptr);
		delete tptr;
	}
	
	return pgptr;
}

//delete selectedtext in paragraph
void MCParagraph::deleteselection()
{
	if (startindex != endindex)
	{
		focusedindex = startindex;
		deletestring(startindex, endindex);
	}
	startindex = endindex = originalindex = PARAGRAPH_MAX_LEN;
	state &= ~PS_HILITED;
}

MCParagraph *MCParagraph::cutline()
{
	if (focusedindex == gettextlength())
		return NULL;
	split();
	MCParagraph *dummy = NULL;
	return next()->remove(dummy);
}

//copy selectedtext
MCParagraph *MCParagraph::copyselection()
{
	MCParagraph *t_paragraph;
	t_paragraph = copystring(startindex, endindex);
	// MW-2012-01-25: [[ ParaStyles ]] If the beginning of the para was not
	//   copied, then clear the attrs.
	// MW-2012-02-21: [[ Bug 10685 ]] Changing previous design decision, since paragraph
	//   attributes are elided in an appropriate way on paste, it doesn't make sense to
	//   strip them on copy.
	//if (startindex != 0)
	//	t_paragraph -> clearattrs();
	return t_paragraph;
}

// MW-2012-02-13: [[ Unicode Block ]] This variant of finsert assumes that the incoming
//   text contains no line-breaks.
void MCParagraph::finsertnobreak(MCStringRef p_string, MCRange t_range)
{
	// If the byte length exceeds the space we have, truncate it.
	uindex_t t_new_length, t_cur_length;
	t_new_length = t_range.length;
	t_cur_length = MCStringGetLength(*m_text);
	if (t_new_length >= PARAGRAPH_MAX_LEN - t_cur_length - 1)
	{
		t_new_length = PARAGRAPH_MAX_LEN - t_cur_length - 1;
	}

	if (t_new_length > 0)
	{
		// Get the block we want to insert into.
		MCBlock *t_block;
		t_block = indextoblock(focusedindex, True);
		
		// Insert the new text into the appropriate spot of the paragraph text
		// TODO: truncation if the paragraph would be too long
		/* UNCHECKED */ MCStringInsertSubstring(*m_text, focusedindex, p_string, t_range);

		// The block containing the insert and subsequent blocks need to have
		// their indices updated to account for the new text.
		t_block -> MoveRange(0, t_new_length);
		t_block = t_block -> next();
		while(t_block != blocks)
		{
			t_block -> MoveRange(t_new_length, 0);
			t_block = t_block -> next();
		}

		// Move the focusedindex to the end of the insert.
        focusedindex += t_new_length;
    }
    
    // New text so a re-layout is necessary
    needs_layout = true;
}

// MW-2012-02-13: [[ Block Unicode ]] New implementation of finsert which understands unicodeness.
Boolean MCParagraph::finsertnew(MCStringRef p_string)
{
	Boolean t_need_recompute;
	t_need_recompute = False;

	MCParagraph *t_paragraph;
	t_paragraph = this;

	// Loop through the string, inserting each line into a separate paragraph.
	uindex_t t_length;
	t_length = MCStringGetLength(p_string);
	findex_t t_index = 0;
	while(t_index < t_length)
	{
		findex_t t_nextpara;
		if (TextFindNextParagraph(p_string, t_index, t_nextpara))
		{
            // We found a line-break, so insert it into the current paragraph and then split at
			// the end.
			MCRange t_range = MCRangeMakeMinMax(t_index, t_nextpara - 1);
            t_paragraph -> finsertnobreak(p_string, t_range);
			t_paragraph -> split();
			t_paragraph = t_paragraph -> next();

			// Advance beyond the paragraph break codepoint
			t_index = t_nextpara;
			
			t_need_recompute = True;
		}
		else
		{
			// We didn't find a line-break, so insert the string into the current paragraph and
			// we must be done.
			t_paragraph -> finsertnobreak(p_string, MCRangeMakeMinMax(t_index, t_length));
			t_index = t_length;
		}
	}

	// MW-2012-02-27: [[ Bug 10028 ]] When editing text that falls before a tab-stop, no
	//   redraw will occur as the length of the line won't change in most circumstances.
	if (lines != nil)
	{
		// Make sure we mark the lines of the initial paragraph as zero width so they
		// redraw on reflow.
		MCLine *t_line;
		t_line = lines;
		do
		{
			// If the range of the line touches [focusedindex, focusedindex+length)
			// then set the line's width to 0 to force it to be redrawn
			findex_t i, l;
			t_line -> GetRange(i, l);
			if (i < focusedindex + t_length && i + l >= focusedindex)
				t_line -> setwidth(0);
			
			t_line = t_line -> next();
		}
		while(t_line != lines);
	}
	
	// Make sure the last paragraph we created has its focusedindex set to the
	// end.
	t_paragraph -> startindex = t_paragraph -> endindex = t_paragraph -> originalindex = t_paragraph -> focusedindex;

	// If we added new paragraphs then we must recompute, in this case reset the
	// selection of the first paragraph to unset.
	if (t_need_recompute)
	{
		startindex = endindex = originalindex = PARAGRAPH_MAX_LEN;
		focusedindex = 0;
	}

	state &= PS_HILITED;

	return t_need_recompute;
}

int2 MCParagraph::fdelete(Field_translations type, MCParagraph *&undopgptr)
{
	findex_t si = focusedindex;
	findex_t ei = focusedindex;
	MCBlock *bptr = indextoblock(focusedindex, False);
	findex_t bindex, blength;
	bptr->GetRange(bindex, blength);
	switch (type)
	{
    case FT_DELBSUBCHAR:
    {
		if (focusedindex == 0)
			return -1;
            
        // Because we are deleting a subchar, we need to decompose the current char
        findex_t t_charstart, t_charend;
        t_charstart = PrevChar(focusedindex);
        t_charend = focusedindex;
        
        // Get the bit of text we need to decompose and do so
        MCAutoStringRef t_composed, t_decomposed;
        MCRange t_range;
        t_range = MCRangeMakeMinMax(t_charstart, t_charend);
        /* UNCHECKED */ MCStringCopySubstring(*m_text, t_range, &t_composed);
        /* UNCHECKED */ MCStringNormalizedCopyNFD(*t_composed, &t_decomposed);
        
        // Replace the character with its decomposed form. This requires adjusting
        // all the blocks to alter their indices.
        /* UNCHECKED */ MCStringReplace(*m_text, t_range, *t_decomposed);
        
        findex_t t_delta = MCStringGetLength(*t_decomposed) - MCStringGetLength(*t_composed);
        MCBlock *t_bptr = indextoblock(t_charstart, False);
        t_bptr->MoveRange(0, t_delta);
        while ((t_bptr = t_bptr->next()) != blocks)
        {
            t_bptr->MoveRange(t_delta, 0);
        }
        
        focusedindex += t_delta;
        startindex += t_delta;
        endindex += t_delta;
        originalindex += t_delta;
        si += t_delta;
        ei += t_delta;
        
        // After all that has been done, we want to delete a single codepoint
		si = DecrementIndex(focusedindex);
		break;
    }
    case FT_DELBCHAR:
        if (focusedindex == 0)
            return -1;
        si = PrevChar(focusedindex);
        break;
	case FT_DELBWORD:
		if (focusedindex == 0)
			return -1;
		si = DecrementIndex(focusedindex);
		
        // TODO: find out if ICU break iterator makes this redundant
        while (si && TextIsWordBreak(GetCodepointAtIndex(si)))
			si = DecrementIndex(si);

        si = PrevWord(si);
		break;
	case FT_DELFCHAR:
		if (focusedindex == gettextlength())
			return 1;
		ei = NextChar(focusedindex);
		break;
	case FT_DELFWORD:
		if (focusedindex == gettextlength())
			return 1;
		ei = IncrementIndex(focusedindex);
            
        // TODO: find out if ICU break iterator makes this redundant
		while (ei < gettextlength() && TextIsWordBreak(GetCodepointAtIndex(ei)))
			ei = IncrementIndex(ei);

        ei = NextWord(ei);
		break;
	case FT_DELBOL:
		{
			// MW-2012-09-19: [[ Bug 9500 ]] Cmd-Backspace on Mac should delete to
			//   the beginning of line.
			
			// Get the line containing the focusedindex.
			MCLine *t_line;
			t_line = indextoline(focusedindex);
			
			// Get the line's start index and length.
			findex_t i, l;
			t_line -> GetRange(i, l);
			
			// Set the first char to delete to the line's start index.
			si = i;
		}
		break;
	case FT_DELEOP:
		ei = gettextlength();
		break;
	default:
		break;
	}
	undopgptr = copystring(si,ei);
	deletestring(si, ei);
	focusedindex = si;
	if ((state & PS_LINES_NOT_SYNCHED) == 0)
	{
		MCLine *lptr = indextoline(focusedindex);
		lptr->makedirty();
	}
	return 0;
}

uint1 MCParagraph::fmovefocus_visual(Field_translations type)
{
    // Get the current block and its text direction
    MCBlock *sbptr = indextoblock(focusedindex, false, true);
    MCBlock *ebptr = nil;
    bool t_is_rtl = sbptr->is_rtl();
    bool t_done = false;
  
    findex_t i, l;
    sbptr->GetRange(i, l);
    switch (type)
    {
        case FT_LEFTCHAR:
        case FT_LEFTWORD:
            moving_left = true;
            if ((t_is_rtl && focusedindex == i + l) || (!t_is_rtl && focusedindex == i))
            {
                ebptr = sbptr->GetPrevBlockVisualOrder();
                // Shortcut for the character moving
                if (ebptr != nil && type == FT_LEFTCHAR)
                {
                    t_done = true;
                    moving_forward = !ebptr->is_rtl();
                    if (ebptr->is_rtl())
                        focusedindex = NextChar(ebptr->GetOffset());
                    else
                        focusedindex = PrevChar(ebptr->GetOffset() + ebptr->GetLength());
                }
            }
            if (!t_done)
            {
                if (type == FT_LEFTCHAR)
                {
                    type = t_is_rtl ? FT_FORWARDCHAR : FT_BACKCHAR;
                }
                else
                {
                    type = t_is_rtl ? FT_FORWARDWORD : FT_BACKWORD;
                }
            }
            break;
            
        case FT_RIGHTCHAR:
        case FT_RIGHTWORD:
            moving_left = false;
            if ((t_is_rtl && focusedindex == i) || (!t_is_rtl && focusedindex == i + l))
            {
                ebptr = sbptr->GetNextBlockVisualOrder();
                // Shortcut for the character moving
                if (ebptr != nil && type == FT_RIGHTCHAR)
                {
                    t_done = true;
                    moving_forward = ebptr->is_rtl();
                    if (ebptr->is_rtl())
                        focusedindex = PrevChar(ebptr->GetOffset() + ebptr->GetLength());
                    else
                        focusedindex = NextChar(ebptr->GetOffset());
                }
            }
            if (!t_done)
            {
                if (type == FT_RIGHTCHAR)
                {
                    type = t_is_rtl ? FT_BACKCHAR : FT_FORWARDCHAR;
                }
                else
                {
                    type = t_is_rtl ? FT_BACKWORD : FT_FORWARDWORD;
                }
            }
            break;
            
        default:
            break;
    }
    
    if (t_done)
        return FT_UNDEFINED;
    else
        return fmovefocus(type, true);
}

uint1 MCParagraph::fmovefocus(Field_translations type, bool p_force_logical)
{
    // Get the cursor movement style of the parent field
    bool t_visual_movement;
    t_visual_movement = parent->IsCursorMovementVisual();
    if (!p_force_logical && t_visual_movement)
       return fmovefocus_visual(type);

    // Using logical ordering so translate the type
    MCBlock *bptr = indextoblock(focusedindex, false, true);
    switch (type)
    {
        case FT_LEFTCHAR:
            moving_left = !bptr -> is_rtl();
            type = FT_BACKCHAR;
            break;
            
        case FT_LEFTWORD:
            moving_left = !bptr -> is_rtl();
            type = FT_BACKWORD;
            break;
            
        case FT_RIGHTCHAR:
            moving_left = bptr -> is_rtl();;
            type = FT_FORWARDCHAR;
            break;
            
        case FT_RIGHTWORD:
            moving_left = bptr -> is_rtl();
            type = FT_FORWARDWORD;
            break;

		default:
			/* Field translations that don't require RTL fix-ups */
			break;
    }

    findex_t oldfocused = focusedindex;
    uindex_t t_length = gettextlength();
	switch (type)
	{
	case FT_BACKCHAR:
        moving_forward = false;
		if (focusedindex == 0)
			return FT_BACKCHAR;
		focusedindex = PrevChar(focusedindex);
		break;
	case FT_BACKWORD:
		// MW-2012-11-14: [[ Bug 10504 ]] Corrected loop to ensure the right chars
		//   are accessed when dealing with Unicode blocks.
        moving_forward = false;
		if (focusedindex == 0)
			return FT_BACKCHAR;
        focusedindex = PrevChar(focusedindex);

        // TODO: is this necessary with the ICU break iterator?
        while (focusedindex && TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
            focusedindex = PrevChar(focusedindex);

        focusedindex = PrevWord(focusedindex);
		break;
	case FT_FORWARDCHAR:
        moving_forward = true;
        if (focusedindex == t_length)
			return FT_FORWARDCHAR;
		focusedindex = NextChar(focusedindex);
		break;
	case FT_FORWARDWORD:
        moving_forward = true;
        if (focusedindex == t_length)
			return FT_FORWARDCHAR;
        focusedindex = NextChar(focusedindex);

        // TODO: is this necessary with the ICU break iterator?
        while (focusedindex < t_length && TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
            focusedindex = NextChar(focusedindex);
            
        focusedindex = NextWord(focusedindex);
		break;
	case FT_BOS:
        if (focusedindex)
            focusedindex = DecrementIndex(focusedindex);

        // Skip all the word delimiters that might be before a sentence delimiter
        while (focusedindex && TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
            focusedindex = DecrementIndex(focusedindex);

        // Skip all the sequence of sentence delimiters
        while (focusedindex && TextIsSentenceBreak(GetCodepointAtIndex(focusedindex)))
            focusedindex = DecrementIndex(focusedindex);

        for(;;)
        {
            if (focusedindex == 0)
                break;

            findex_t t_previous_focusedindex;
            t_previous_focusedindex = focusedindex;
            focusedindex = DecrementIndex(focusedindex);
            if (TextIsSentenceBreak(GetCodepointAtIndex(focusedindex)))
            {
                focusedindex = t_previous_focusedindex;
                break;
            }
        }

        // Skip all the word delimiters in the beginning of the sentence
        while (focusedindex < t_length && TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
            focusedindex = IncrementIndex(focusedindex);

		if (focusedindex == oldfocused)
			return FT_BOS;
		break;
	case FT_EOS:
        if (focusedindex < t_length)
            focusedindex = IncrementIndex(focusedindex);

        while (focusedindex < t_length && TextIsSentenceBreak(GetCodepointAtIndex(focusedindex)))
            focusedindex = IncrementIndex(focusedindex);

        while (focusedindex < t_length && !TextIsSentenceBreak(GetCodepointAtIndex(focusedindex)))
            focusedindex = IncrementIndex(focusedindex);

		if (focusedindex == oldfocused)
			return FT_EOS;
		break;
	case FT_BOP:
		focusedindex = 0;
		break;
	case FT_LEFTPARA:
		if (focusedindex == 0)
			return FT_LEFTPARA;
		focusedindex = 0;
		break;
	case FT_EOP:
        focusedindex = t_length;
		break;
	case FT_RIGHTPARA:
        if (focusedindex == t_length)
			return FT_RIGHTPARA;
        focusedindex = t_length;
		break;
	default:
		break;
	}
	return FT_UNDEFINED;
}

int2 MCParagraph::setfocus(int4 x, int4 y, uint2 fixedheight,
                           Boolean extend, Boolean extendwords,
                           Boolean extendlines, int2 direction, Boolean first,
                           Boolean last, Boolean deselect)
{
	MCBlock *bptr;
	findex_t bindex, blength;
	if (y < 0)
	{
		if (extend)
		{
			if (state & PS_FRONT)
			{
				if (first)
					return 0;
				state &= ~PS_HILITED;
				marklines(startindex, endindex);
				startindex = endindex = originalindex = PARAGRAPH_MAX_LEN;
			}
			else
			{
				if (originalindex == PARAGRAPH_MAX_LEN)
				{
					state |= PS_BACK;
					originalindex = gettextlength();
				}
				else
					if (originalindex != gettextlength())
					{
						state &= ~PS_BACK;
						if (extendlines)
							originalindex = gettextlength();
						else if (extendwords && focusedindex >= originalindex)
						{
							// This case is invoked when navigate to a previous paragraph upwards,
							// thus selecting everything before the original index in this paragraph.
							// This loop rounds up the original index to the next word boundary
							bptr = indextoblock(originalindex, False);
							if (originalindex < gettextlength() && !TextIsWordBreak(GetCodepointAtIndex(originalindex)))
								originalindex = findwordbreakafter(bptr, originalindex);
						}
					}
				state |= PS_FRONT;
				focusedindex = startindex = 0;
				endindex = originalindex;
				marklines(startindex, endindex);
			}
		}
		else
		{
			if (deselect)
			{
				state &= ~PS_HILITED;
				if (startindex != endindex)
					marklines(startindex, endindex);
				startindex = endindex = originalindex = PARAGRAPH_MAX_LEN;
			}
			if (first)
			{
				focusedindex = 0;
				return 0;
			}
		}
		return -1;
	}
	else
	{
		int2 height = getheight(fixedheight);
		if (y >= height)
		{
			if (extend)
			{
				if (state & PS_BACK)
				{
					if (last)
						return 0;
					state &= ~PS_HILITED;
					marklines(startindex, endindex);
					startindex = endindex = originalindex = PARAGRAPH_MAX_LEN;
				}
				else
				{
					if (originalindex == PARAGRAPH_MAX_LEN)
					{
						state |= PS_FRONT;
						originalindex = 0;
					}
					else
						if (originalindex != 0)
						{
							state &= ~PS_FRONT;
							if (extendlines)
								originalindex = 0;
							else if (extendwords && focusedindex < originalindex)
							{
								// This clause is invoked when we navigate to the next paragraph, having
								// already selected words before the original. It rounds originalindex down
								// to the beginning of a word.
								bptr = indextoblock(originalindex, False);
								originalindex = findwordbreakbefore(bptr, originalindex);
							}
						}
					state |= PS_BACK;
					focusedindex = endindex = gettextlength();
					startindex = originalindex;
					marklines(startindex, endindex);
				}
			}
			else
			{
				if (deselect)
				{
					state &= ~PS_HILITED;
					if (startindex != endindex)
						marklines(startindex, endindex);
					if (last)
						originalindex = startindex = endindex = focusedindex = gettextlength();
					else
						startindex = endindex = originalindex = PARAGRAPH_MAX_LEN;
				}
				if (last)
					return 0;
			}
			return 1;
		}
	}

	// MW-2012-01-08: [[ ParaStyles ]] Adjust the start of processing to
	//   after any spacing.
	uint2 ty;
	ty = computetopmargin();

	MCLine *lptr = lines;
	uint16_t theight;
	if (fixedheight == 0)
		theight = ceilf(lptr->GetHeight());
	else
		theight = fixedheight;
	
	// MW-2012-02-27: [[ Bug ]] We count the pixel 'ty + theight' to be part of
	//   the next line, so loop until >= rather than >.
	while (y >= ty + theight && lptr->next() != lines)
	{
		ty += theight;
		lptr = lptr->next();
		if (fixedheight == 0)
            theight = ceilf(lptr->GetHeight());
	};

	// MW-2012-01-08: [[ ParaStyles ]] Adjust the end of processing to
	//   after any spacing.
	ty += computebottommargin();

	// MW-2012-01-08: [[ ParaStyles ]] Adjust the x start taking into account
	//   indents, list indents and alignment. (Field to Paragraph so -ve)
    // SN-2014-08-14: [[ Bug 13106 ]] Having a Vgrid discards the line offsets
    if (!getvgrid())
        x -= computelineoffset(lptr);

	focusedindex = lptr->GetCursorIndex(MCU_max(x, 0), False, moving_forward);
	if (extend)
	{
		if (originalindex == PARAGRAPH_MAX_LEN)
		{
			if (direction < 0)
			{
				state |= PS_BACK;
				startindex = focusedindex;
				originalindex = endindex = gettextlength();
			}
			else
				if (direction > 0)
				{
					state |= PS_FRONT;
					originalindex = startindex = 0;
					endindex = focusedindex;
				}
		}
		else
		{
			marklines(startindex, endindex);
			if (focusedindex < originalindex)
			{
				bptr = indextoblock(focusedindex, False);
				bptr->GetRange(bindex, blength);
				if (extendwords)
				{
					// This clause is invoked when we are moving backwards across
					// words in the same paragraph before the originalindex point.
					// It rounds originalindex up to the end of the word it is in.
					// It then rounds focusedindex down to the beginning of the
					// previous word.
                    // The first time we are moving backwards, originalindex is
                    // at the beginning of the word and endindex at the end.
                    // We simply move originalindex to endindex
                    if (originalindex < endindex)
                        originalindex = endindex;

					bptr = indextoblock(focusedindex, False);
					focusedindex = findwordbreakbefore(bptr, focusedindex);
				}
				if (direction > 0 || first)
					state &= ~PS_FRONT;
				if (extendlines)
				{
					startindex = focusedindex = 0;
					endindex = originalindex = gettextlength();
				}
				else
				{
					startindex = focusedindex;
					if (originalindex > endindex || !(MCmodifierstate & MS_SHIFT))
						endindex = originalindex;
				}
				if (endindex != gettextlength() || direction < 0)
					state &= ~PS_BACK;
			}
			else
			{
				if (extendwords)
				{
					// This clause is invoked when we are moving forwards across
					// words in the same paragraph after the originalindex point.
					// It rounds focusedindex up to the end of the word it is in.
					// It then rounds originalindex down to the beginning of the
					// word it is in.
					bptr = indextoblock(focusedindex, False);
					if (focusedindex < gettextlength() && !TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
						focusedindex = findwordbreakafter(bptr, focusedindex);

					bptr = indextoblock(originalindex, False);
					if (originalindex != startindex)
                        originalindex = findwordbreakbefore(bptr, originalindex);
				}
				if (direction < 0 || last)
					state &= ~PS_BACK;
				if (extendlines)
				{
					startindex = focusedindex = 0;
					endindex = gettextlength();
				}
				else
				{
					if (originalindex < startindex || !(MCmodifierstate & MS_SHIFT))
					{
						startindex = originalindex;
						if (extendwords && state & PS_BACK)
							originalindex = endindex;
					}
					endindex = focusedindex;
				}
				if (startindex > endindex)
					startindex = endindex;
				if (startindex != 0 || direction > 0)
					state &= ~PS_FRONT;
			}
		}
		marklines(startindex, endindex);
	}
	else
	{
		if (startindex != endindex)
			marklines(startindex, endindex);
		originalindex = startindex = endindex = focusedindex;
		if (extendlines)
		{
			originalindex = startindex = 0;
			endindex = gettextlength();
		}
		else
			if (extendwords)
			{
				// Find the block containing startindex - decrementing the index if either:
				//   - the index is past the end of the last block
				//   - the index is a space and non-zero
				bptr = indextoblock(startindex, False);
				if (startindex && (startindex >= gettextlength() || TextIsWordBreak(GetCodepointAtIndex(startindex))))
					bptr = bptr -> RetreatIndex(startindex);

				// Move startindex back to an index that can be broken before.
				startindex = findwordbreakbefore(bptr, startindex);

				// We don't break before spaces, so increment again if we end up pointing at one
				bptr = indextoblock(startindex, False);
				if (startindex < gettextlength() && TextIsWordBreak(GetCodepointAtIndex(startindex)))
					bptr -> AdvanceIndex(startindex);

				originalindex = startindex;

				// Find the block containing focusedindex and then advance it to an index that
				// can be broken after.
				bptr = indextoblock(focusedindex, False);
				focusedindex = findwordbreakafter(bptr, focusedindex);

				// If the last index is not pointing at a space, when we increment to ensure we
				// get a non-empty selection.
				bptr = indextoblock(focusedindex, False);
					
				endindex = focusedindex;

				// MW-2008-08-26: [[ Bug 7023 ]] Make sure we never get an incorrect range.
				if (startindex > endindex)
					startindex = endindex;
					
				// If we've got a non-NULL selection then mark it.
				if (startindex != endindex)
					marklines(startindex, endindex);
			}
	}
	return 0;
}

MCRectangle MCParagraph::getdirty(uint2 fixedheight)
{
	MCRectangle dirty;

	dirty.x = 0;
	dirty.y = 0;
    // SN-2014-08-25: [[ Bug 13263 ]] We want to have a height, even if the line is empty - that ensures
    //  that a line is redrawn when the last remaining char is deleted
	dirty.width = 0;
    dirty.height = fixedheight;

	// MW-2012-01-08: [[ ParaStyles ]] Compute spacing top and bottom.
	int32_t t_space_above, t_space_below;
	t_space_above = computetopmargin();
	t_space_below = computebottommargin();

	// MW-2012-01-08: [[ ParaStyles ]] Text starts after the spacing.
	int2 y;
	y = t_space_above;
	
	bool t_found_dirty;
	t_found_dirty = false;

	int2 t_dirty_top;
	t_dirty_top = y;

	int2 t_dirty_left, t_dirty_right;
	t_dirty_left = INT16_MAX;
	t_dirty_right = INT16_MIN;

	// MW-2012-03-16: [[ Bug 10001 ]] Compute the paragraph box offset and width.
	int32_t t_box_offset, t_box_width;
	computeboxoffsetandwidth(t_box_offset, t_box_width);

    // If we don't have any lines, do a layout
    if (lines == nil)
        layout(false);
    
	uint2 height = fixedheight;
	MCLine *lptr = lines;
	do
	{
		// MW-2013-01-28: [[ Bug 10652 ]] When fixedheight is non-zero, the line height
		//   can differ from the line height we use - so fetch the actual height so we
		//   can adjust the dirty rect.
		int32_t t_line_height;
        t_line_height = ceilf(lptr->GetHeight());
		if (fixedheight == 0)
			height = t_line_height;

		uint2 lwidth = lptr->getdirtywidth();
		if (lwidth)
		{
			if (!t_found_dirty)
			{
				t_found_dirty = true;

				dirty.y = y;
				// MW-2012-01-08: [[ ParaStyles ]] If on the first line, adjust for spacing before.
                // MW-2014-06-10: [[ Bug 11809 ]] Make sure we adjust the top of the dirty rect if on
                //   the first line and there is space above.
				if (lptr == lines)
					t_dirty_top -= t_space_above;
			}

			int32_t t_new_dirty_left, t_new_dirty_right;
			t_new_dirty_left = t_box_offset + computelineinneroffset(t_box_width, lptr);
			t_new_dirty_right = t_new_dirty_left + lwidth;

			if (t_new_dirty_left < t_dirty_left)
				t_dirty_left = t_new_dirty_left;
			if (t_new_dirty_right > t_dirty_right)
				t_dirty_right = t_new_dirty_right;

			dirty.height = y - dirty.y + height;
			
			// MW-2013-01-28: [[ Bug 10652 ]] In fixedLineHeight mode the top of the dirty rect
			//   for a line must take into account the actual line height (otherwise imageSources
			//   in particular might not redraw correctly as they sit above the fixedLineHeight).
			if (fixedheight != 0 && t_line_height > fixedheight)
				t_dirty_top = MCU_min(t_dirty_top, t_dirty_top - (t_line_height - fixedheight));

			// MW-2012-01-08: [[ Paragraph Spacing ]] If on the last line, adjust for spacing after.
			if (lptr -> next() == lines)
				dirty . height += t_space_below;
		}
		y += height;
		lptr = lptr->next();
	}
	while (lptr != lines);

	dirty . x = t_dirty_left;
	dirty . width = t_dirty_right - t_dirty_left;
	
	// MW-2013-01-28: [[ Bug 10652 ]] Adjust the height/y of the dirty rectangle to take into
	//   account any vertical overlap due to imageSources and such.
	dirty . height += dirty . y - t_dirty_top;
	dirty . y = t_dirty_top;

	return dirty;
}

void MCParagraph::inittext()
{
	deletelines();
	deleteblocks();
	blocks = new (nothrow) MCBlock;
	blocks->setparent(this);
	blocks->SetRange(0, gettextlength());
	state |= PS_LINES_NOT_SYNCHED;
	// MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
	//   fontref so it can compute its.
	if (opened)
		blocks->open(parent -> getfontref());
}

void MCParagraph::clean()
{
	MCLine *lptr = lines;
	do
	{
		lptr->clean();
		lptr = lptr->next();
	}
	while (lptr != lines);
}

void MCParagraph::marklines(findex_t si, findex_t ei)
{
	if (lines == NULL || si == PARAGRAPH_MAX_LEN || ei == PARAGRAPH_MAX_LEN)
		return;

	if ((state & PS_LINES_NOT_SYNCHED) != 0)
		return;

	MCLine *slptr = indextoline(si);
	MCLine *elptr = indextoline(ei);
	while (slptr != elptr)
	{
		slptr->makedirty();
		slptr = slptr->next();
	}
	slptr->makedirty();
}

// MW-2012-01-25: [[ ParaStyles ]] The 'include_space' parameter, if true, means that
//   the returned rect will take into account space before and after.
MCRectangle MCParagraph::getcursorrect(findex_t fi, uint2 fixedheight, bool p_include_space, MCParagraphCursorType p_type)
{
	if (fi < 0)
		fi = focusedindex;

	// MW-2005-08-31: If we get here even though we have no lines, 
	//   noflow to make up for it.
	if (lines == NULL)
		noflow();

	// MW-2012-01-08: [[ ParaStyles ]] Compute spacing before and after.
	int32_t t_space_above, t_space_below;
	t_space_above = computetopmargin();
	t_space_below = computebottommargin();

	// MW-2012-01-08: [[ ParaStyles ]] Top of text starts after spacing above.
	MCRectangle drect;
	drect.y = int2(1 + t_space_above);

	MCLine *lptr;
	findex_t i, l;
	bool t_first_line;
	lptr = lines;
	lptr->GetRange(i, l);
	t_first_line = true;
	while (fi >= i + l && lptr->next() != lines)
	{
		if (fixedheight == 0)
			drect.y += ceilf(lptr->GetHeight());
		else
			drect.y += fixedheight;
		lptr = lptr->next();
		lptr->GetRange(i, l);
		t_first_line = false;
	};
	if (fixedheight == 0)
		drect.height = uint2(ceilf(lptr->GetHeight()) - 2);
	else
		drect.height = fixedheight - 2;
    if (p_type == kMCParagraphCursorTypeFull ||
        p_type == kMCParagraphCursorTypePrimary)
    {
        drect.x = int2(lptr->GetCursorXPrimary(fi, moving_forward));
	}
    else
    {
        drect.x = int2(lptr->GetCursorXSecondary(fi, moving_forward));
    }
    
	// MW-2012-01-08: [[ ParaStyles ]] If we want the 'full height' of the
	//   cursor (inc space), adjust appropriately depending on which line we are
	//   on.
	if (p_include_space)
	{
		if (t_first_line)
		{
			drect.y -= t_space_above;
			drect.height += t_space_above;
		}
		if (lptr -> next() == lines)
			drect.height += t_space_below;
	}
    
    // SN-2014-08-14: [[ Bug 13106 ]] Having a Vgrid discards the line offsets
    if (!getvgrid())
        drect.x += computelineoffset(lptr);

	drect.width = cursorwidth;

    // Adjust the height - if primary or secondary cursor is requested.
    if (p_type == kMCParagraphCursorTypePrimary)
    {
        drect.height /= 2;
    }
    else if (p_type == kMCParagraphCursorTypeSecondary)
    {
        drect.y += drect.height/2;
        drect.height /= 2;
    }

	return drect;
}

bool MCParagraph::copytextasstringref(MCStringRef& r_string)
{
	return MCStringCopy(*m_text, r_string);
}

void MCParagraph::settext(MCStringRef p_string)
{
	deletelines();
	deleteblocks();
	
	m_text.Reset();
	/* UNCHECKED */ MCStringMutableCopy(p_string, &m_text);
	
	blocks = new (nothrow) MCBlock;
	blocks->setparent(this);
	blocks->SetRange(0, MCStringGetLength(*m_text));
}

void MCParagraph::resettext(MCStringRef p_string)
{
    m_text.Reset();
	/* UNCHECKED */ MCStringMutableCopy(p_string, &m_text);
	findex_t i, l;
	if (blocks == NULL)
	{
		blocks = new (nothrow) MCBlock;
		blocks->setparent(this);

		state |= PS_LINES_NOT_SYNCHED;
	}
	
	// Trim the last block so that it does not extend past the paragraph
	blocks->prev()->GetRange(i, l);
	blocks->prev()->SetRange(i, MCU_max(gettextlength() - i, 0));
}

void MCParagraph::getmaxline(uint2 &width, uint2 &aheight, uint2 &dheight)
{
	width = aheight = dheight = 0;
	if (lines != NULL)
	{
		int32_t t_first_indent;
		t_first_indent = getfirstindent();

		MCLine *lptr = lines;
		do
		{
			uint2 t_line_width;
			t_line_width = lptr -> getwidth();

			// MW-2012-03-16: [[ Bug 10001 ]] Adjust the line width to take into
			//   account any extra width added by first indent (-ve first indent
			//   widens non-first lines; +ve first indent widens first line).
			if (lines == lptr)
				t_line_width += MCMax(0, t_first_indent);
			else
				t_line_width -= MCMin(0, t_first_indent);

			width = MCU_max(width, t_line_width);
			aheight = MCU_max(aheight, uint16_t(ceilf(lptr->GetAscent() + lptr->GetLeading())));
			dheight = MCU_max(dheight, uint16_t(ceilf(lptr->GetDescent())));
			lptr = lptr->next();
		}
		while (lptr != lines);
	}

	width += computeleftmargin() + computerightmargin();
}

uint2 MCParagraph::getwidth() const
{
	int32_t t_width = 0;

	if (lines != NULL)
	{
		int32_t t_first_indent;
		t_first_indent = getfirstindent();

		MCLine *lptr = lines;
		do
		{
			int32_t t_line_width;
			t_line_width = lptr -> getwidth();

			// MW-2012-03-16: [[ Bug 10001 ]] Adjust the line width to take into
			//   account any extra width added by first indent (-ve first indent
			//   widens non-first lines; +ve first indent widens first line).
			if (lines == lptr)
				t_line_width += MCMax(0, t_first_indent);
			else
				t_line_width -= MCMin(0, t_first_indent);

			t_width = MCU_max(t_width, t_line_width);
			lptr = lptr->next();
		}
		while (lptr != lines);
	}

	t_width += computeleftmargin() + computerightmargin();

	return t_width;
}

uint2 MCParagraph::getheight(uint2 fixedheight) const
{
	coord_t height = 0;

	// MW-2012-03-05: [[ HiddenText ]] If the paragraph is currently hidden, then it
	//   is of height 0.
	if (gethidden())
		return 0;

	// MW-2012-01-08: [[ ParaStyles ]] Height of paragraph includes spacing
	//   before.
	height += computetopmargin();

	if (lines != NULL)
	{
		MCLine *lptr = lines;
		do
		{
			if (fixedheight == 0)
				height += ceilf(lptr->GetHeight());
			else
				height += fixedheight;
			lptr = lptr->next();
		}
		while (lptr != lines);
	}

	// MW-2012-01-08: [[ ParaStyles ]] Height of paragraph includes spacing
	//   after.
	height += computebottommargin();

	return height;
}

Boolean MCParagraph::isselection()
{
	return startindex != endindex || state & (PS_FRONT | PS_BACK);
}

void MCParagraph::getselectionindex(findex_t &si, findex_t &ei)
{
	if (state & PS_FRONT && state & PS_BACK)
	{
		si = 0;
		ei = gettextlength();
	}
	else
		if (isselection())
		{
			si = startindex;
			ei = endindex;
		}
		else
			si = ei = focusedindex;
}

void MCParagraph::setselectionindex(findex_t si, findex_t ei, Boolean front, Boolean back)
{
	marklines(startindex, endindex);
	originalindex = startindex = si;
	endindex = ei;
	if (endindex < PARAGRAPH_MAX_LEN)
		focusedindex = endindex;
	if (front)
		state |= PS_FRONT;
	else
		state &= ~PS_FRONT;
	if (back)
		state |= PS_BACK;
	else
		state &= ~PS_BACK;
	marklines(startindex, endindex);
}

void MCParagraph::reverseselection()
{
	if (originalindex == startindex)
		originalindex = endindex;
	else
		originalindex = startindex;
}

void MCParagraph::indextoloc(findex_t tindex, uint2 fixedheight, coord_t &x, coord_t &y)
{
	// MW-2012-01-08: [[ ParaStyles ]] Text starts after spacing above.
	y = computetopmargin();
	
	MCLine *lptr = lines;
	while (True)
	{
		findex_t i, l;
		lptr->GetRange(i, l);
		if (i + l > tindex || lptr->next() == lines)
		{
			x = getx(tindex, lptr);
			break;
		}
		if (fixedheight == 0)
            y += ceilf(lptr->GetHeight());
		else
			y += fixedheight;
		lptr = lptr->next();
	}
}

uint2 MCParagraph::getyextent(findex_t tindex, uint2 fixedheight)
{
	uint2 y;
	MCLine *lptr = lines;
	findex_t i, l;

	// MW-2012-01-08: [[ ParaStyles ]] Text starts after spacing above.
	y = computetopmargin();
	do
	{
		if (fixedheight == 0)
            y += ceilf(lptr->GetHeight());
		else
			y += fixedheight;
		lptr->GetRange(i, l);
		lptr = lptr->next();
	}
	while (lptr != lines && i + l < tindex);

	// MW-2012-01-08: [[ ParaStyles ]] If we finish on the last line,
	//   adjust for space below.
	if (lptr == lines && tindex > i + l)
		y += computebottommargin();

	return y;
}

coord_t MCParagraph::getx(findex_t tindex, MCLine *lptr)
{
	coord_t x = lptr->GetCursorXPrimary(tindex, moving_forward);

	// MW-2012-01-08: [[ ParaStyles ]] Adjust the x start taking into account
	//   indents, list indents and alignment. (Paragraph to Field so +ve)
    // SN-2014-08-14: [[ Bug 13106 ]] Having a Vgrid discards the line offsets
    if (!getvgrid())
        x += computelineoffset(lptr);

	return x;
}

void MCParagraph::getxextents(findex_t &si, findex_t &ei, coord_t &minx, coord_t &maxx)
{
	if (lines == NULL)
	{
		minx = maxx = 0;
		return;
	}

	bool t_is_list;
	t_is_list = getliststyle() != kMCParagraphListStyleNone;
	
	MCLine *lptr = lines;
	findex_t i, l;
	do
	{
		coord_t newx;
		lptr->GetRange(i, l);
		if (i + l > si)
		{
			if (si >= i)
			{
				minx = maxx = getx(si, lptr);
				if (si == 0 && t_is_list)
					minx -= getlistlabelwidth();
			}
			else
			{
				newx = getx(i, lptr);
				if (i == 0 && t_is_list)
					newx -= getlistlabelwidth();
				if (newx < minx)
					minx = newx;
			}
			if (ei <= i + l)
				newx = getx(ei, lptr);
			else
			{
				findex_t end = i + l;
				findex_t bindex, blength;

				MCBlock *bptr = indextoblock(end, False);
				bptr->GetRange(bindex, blength);
				while (end && TextIsWordBreak(GetCodepointAtIndex(DecrementIndex(end))))
				{
					end = DecrementIndex(end);
					if (end < bindex)
					{
						bptr = bptr->prev();
						bptr->GetRange(bindex, blength);
					}
				}
				newx = getx(end, lptr);
			}
			if (newx > maxx)
				maxx = newx;
		}
		lptr = lptr->next();
	}
	while (i + l < ei && lptr != lines);
	
	si -= gettextlengthcr();
	ei -= gettextlengthcr();
}

// MW-2013-05-21: [[ Bug 10794 ]] Changed signature to return the block the search
//   ends up in.
MCBlock *MCParagraph::extendup(MCBlock *bptr, findex_t &si)
{
	Boolean isgroup = True;
	Boolean found = False;
	
	// MW-2008-09-04: [[ Bug 7085 ]] Extending clicked links upwards should terminate
	//   when we get to a block with different linkText.
	MCStringRef t_link_text;
	t_link_text = bptr -> getlinktext();
	
	while (bptr != blocks && isgroup)
	{
		if (bptr == NULL)
			bptr = blocks->prev();
		else
			bptr = bptr->prev();
		// MW-2012-02-17: [[ SplitTextAttrs ]] Use the 'islink()' predicate rather than
		//   checking directly.
		isgroup = bptr -> islink() && bptr -> getlinktext() == t_link_text;
		found |= isgroup;
	}
	if (!isgroup)
		bptr = bptr->next();
	findex_t l;
	bptr->GetRange(si, l);
	return bptr;
}

// MW-2013-05-21: [[ Bug 10794 ]] Changed signature to return the block the search
//   ends up in.
MCBlock *MCParagraph::extenddown(MCBlock *bptr, findex_t &ei)
{
	Boolean isgroup = True;
	Boolean found = False;
	
	// MW-2008-09-04: [[ Bug 7085 ]] Extending clicked links downwards should terminate
	//   when we get to a block with different linkText.
	MCStringRef t_link_text;
	t_link_text = bptr -> getlinktext();
	
	while (bptr == NULL || (bptr->next() != blocks && isgroup))
	{
		if (bptr == NULL)
			bptr = blocks;
		else
			bptr = bptr->next();
		// MW-2012-02-17: [[ SplitTextAttrs ]] Use the 'islink()' predicate rather than
		//   checking directly.
		isgroup = bptr -> islink() && bptr -> getlinktext() == t_link_text;
		found |= isgroup;
	}
	if (!isgroup)
		bptr = bptr->prev();
	findex_t l;
	bptr->GetRange(ei, l);
	ei += l;
	return bptr;
}

void MCParagraph::getclickindex(int2 x, int2 y,
                                   uint2 fixedheight, findex_t &si, findex_t &ei,
                                   Boolean wholeword, Boolean chunk)
{
	uint2 theight;
	if (fixedheight == 0)
        theight = ceilf(lines->GetHeight());
	else
		theight = fixedheight;

	// MW-2012-01-08: [[ ParaStyles ]] Text starts after spacing above.
	uint2 ty = computetopmargin();

	MCLine *lptr = lines;
	while (y > ty + theight && lptr->next() != lines)
	{
		ty += theight;
		lptr = lptr->next();
		if (fixedheight == 0)
            theight = ceilf(lptr->GetHeight());
	};

	// MW-2012-01-08: [[ ParaStyles ]] Text finishes before spacing below.
	ty += computebottommargin();

	// MW-2012-01-08: [[ Paragraph Align ]] Adjust the x start taking into account
	//   indents, list indents and alignment. (Field to Paragraph so -ve)
    // SN-2014-08-14: [[ Bug 13106 ]] Having a Vgrid discards the line offsets
    if (!getvgrid())
        x -= computelineoffset(lptr);

	si = lptr->GetCursorIndex(x, chunk, true);
	int4 lwidth = lptr->getwidth();
	if (x < 0 || x >= lwidth)
	{
		if (!chunk && x >= lwidth)
		{
			findex_t i, l;
			lptr->GetRange(i, l);
			ei = i + l;
		}
		else
			si = ei = 0;
		return;
	}

	MCBlock *bptr = NULL;
	bptr = indextoblock(si, False);
	if (!wholeword)
	{
		ei = NextChar(si);
		return;
	}

	// MW-2012-02-17: [[ SplitTextAttrs ]] If the block is a link, then act
	//   accordingly.
	if (bptr -> islink())
	{
		extendup(bptr, si);
		extenddown(bptr, ei);
		return;
	}
	else
	{
		// If we are looking at a space - we return a single character.
		if (TextIsWordBreak(GetCodepointAtIndex(si)))
		{
			ei = si;
			return;
		}
        
        // SN-2014-05-16 [[ Bug 12432 ]]
        // If si is now a wordbreak, then it was a worbreak boundary beforehand - and nothing should have been done
        uindex_t t_si;
		t_si = findwordbreakbefore(bptr, si);
        if (!TextIsWordBreak(GetCodepointAtIndex(t_si)))
            si = t_si;
        
		ei = si;
		bptr = indextoblock(ei, False);
		ei = findwordbreakafter(bptr, ei);

		bptr = indextoblock(ei, False);
		// AL-2014-04-07: [[ Bug 12143 ]] Advancing the index here causes the mouseChunk to report incorrect end index
        // bptr -> AdvanceIndex(ei);
		
		return;
	}
}

findex_t MCParagraph::findwordbreakbefore(MCBlock *p_block, findex_t p_index)
{    
	// Create the word break iterator
    MCBreakIteratorRef t_breaker;
    MCLocaleBreakIteratorCreate(kMCBasicLocale, kMCBreakIteratorTypeWord, t_breaker);
    MCLocaleBreakIteratorSetText(t_breaker, *m_text);
    
    // Find the preceding word break
    findex_t t_break;
    t_break = MCLocaleBreakIteratorBefore(t_breaker, p_index);
    MCLocaleBreakIteratorRelease(t_breaker);
    
    return (t_break == kMCLocaleBreakIteratorDone) ? 0 : t_break;
}

findex_t MCParagraph::findwordbreakafter(MCBlock *p_block, findex_t p_index)
{
	// Create the word break iterator
    MCBreakIteratorRef t_breaker;
    MCLocaleBreakIteratorCreate(kMCBasicLocale, kMCBreakIteratorTypeWord, t_breaker);
    MCLocaleBreakIteratorSetText(t_breaker, *m_text);
    
    // Find the succeeding word break
    findex_t t_break;
    t_break = MCLocaleBreakIteratorAfter(t_breaker, p_index);
    MCLocaleBreakIteratorRelease(t_breaker);
    
    return (t_break == kMCLocaleBreakIteratorDone) ? MCStringGetLength(*m_text) : t_break;
}

void MCParagraph::sethilite(Boolean newstate)
{
	uint4 oldstate = state;
	if (newstate)
		state |= PS_HILITED;
	else
		state &= ~PS_HILITED;
	startindex = endindex = PARAGRAPH_MAX_LEN;
	if (state != oldstate)
		lines->makedirty();
}

Boolean MCParagraph::gethilite()
{
	return (state & PS_HILITED) != 0;
}

Boolean MCParagraph::getvisited(findex_t si)
{
	MCBlock *bptr = indextoblock(si, False);
	return bptr->getvisited();
}

MCStringRef MCParagraph::getlinktext(findex_t si)
{
	MCBlock *bptr = indextoblock(si, False);
	return bptr->getlinktext();
}

MCStringRef MCParagraph::getimagesource(findex_t si)
{
	MCBlock *bptr = indextoblock(si, False);
	return bptr->getimagesource();
}

MCStringRef MCParagraph::getmetadataatindex(findex_t si)
{
	MCBlock *bptr = indextoblock(si, False);
	return bptr->getmetadata();
}

// This method returns the state of a given block flag in the specified range.
// If the flag is the same on all the range, the state is returned in 'state'
// and true is the result. Otherwise, false is the result.
bool MCParagraph::getflagstate(uint32_t flag, findex_t si, findex_t ei, bool& r_state)
{
	// Clamp the upper index to the textsize.
	if (ei > gettextlength())
		ei = gettextlength();
	
	// Get the block and make appropriate adjustments to ensure we are looking
	// at the right one.
	MCBlock *bptr = indextoblock(si, False);
	findex_t i, l;
	bptr->GetRange(i, l);
	if (si == ei && si == i && l != 0 && bptr != blocks)
	{
		bptr = bptr -> prev();
		bptr -> GetRange(i, l);
	}
	
	// Now loop through all the blocks until we reach the end, checking to see
	// if the states are all equal.
	bool t_state;
	t_state = bptr -> getflag(flag);
	while(i + l < ei)
	{
		bptr = bptr -> next();
		
		// If the block is of zero size, we ignore it.
		if (bptr -> GetLength() != 0 && bptr -> getflag(flag) != t_state)
			return false;

		bptr->GetRange(i, l);
	}
	
	r_state = t_state;
	return true;
}

// This method accumulates the ranges of the paragraph that have 'flagged' set
// to true. The output is placed in ep as a return-delimited list, with indices
// This method accumulates the ranges of the paragraph that have 'flagged' set
// to true. The output is placed in the uinteger_t array, with indices
// adjusted by the 'delta'.
void MCParagraph::getflaggedranges(uint32_t p_part_id, findex_t si, findex_t ei, int32_t p_delta, MCInterfaceFieldRanges& r_ranges)
{
	// If the paragraph is empty, there is nothing to do.
	if (gettextlength() == 0)
    {
        r_ranges . ranges = nil;
        r_ranges . count = 0;
        return;
    }

	if (ei > gettextlength())
		ei = gettextlength();
    
	// Get the block and make appropriate adjustments to ensure we are looking
	// at the right one.
	MCBlock *bptr = indextoblock(si, False);
	findex_t i, l;
	bptr->GetRange(i, l);
	if (si == ei && si == i && l != 0 && bptr != blocks)
	{
		bptr = bptr -> prev();
		bptr -> GetRange(i, l);
	}
    
    MCAutoArray<MCInterfaceFieldRange> t_ranges;
    
	// Now loop through all the blocks until we reach the end.
	int32_t t_flagged_start, t_flagged_end;
	t_flagged_start = -1;
	t_flagged_end = -1;
	for(;;)
	{
        MCInterfaceFieldRange t_range;
		// Ignore any blocks of zero width;
		if (bptr -> GetLength() != 0)
		{
			// If this block is flagged, update the start/end.
			if (bptr -> getflagged())
			{
				// If we don't have a start, take the start of this block.
				if (t_flagged_start == -1)
					t_flagged_start = MCMax(si, i);
                
				// Always extend to the end.
				t_flagged_end = MCMin(ei, i + l);
			}
            
			// If we have a flagged range and we are reaching the end or have a block
			// which is not flagged then append the range.
			if (t_flagged_start != -1 && (!bptr -> getflagged() || i + l >= ei))
			{				
				// MW-2012-02-24: [[ FieldChars ]] Map the field indices back to char indices.
				findex_t t_start, t_end;
				t_start = p_delta + t_flagged_start;
				t_end = p_delta + t_flagged_end;
				parent -> unresolvechars(p_part_id, t_start, t_end);
                t_range . start = t_start + 1;
                t_range . end = t_end;
                t_ranges . Push(t_range);
                
				t_flagged_start = t_flagged_end = -1;
			}
            
			// If we have reached the end, break.
			if (i + l >= ei)
				break;
		}
		
		// Advance to the end of the block.
		bptr = bptr -> next();
		bptr -> GetRange(i, l);
	}
    t_ranges . Take(r_ranges . ranges, r_ranges . count);
}

void MCParagraph::setvisited(findex_t si, findex_t ei, Boolean v)
{
	MCBlock *bptr = indextoblock(si, False);
	findex_t i, l;
	do
	{
		if (v)
			bptr->setvisited();
		else
			bptr->clearvisited();
		bptr->GetRange(i, l);
		bptr = bptr->next();
	}
	while (i + l < ei);
}

Boolean MCParagraph::pageheight(uint2 fixedheight, uint2 &theight,
                                MCLine *&lptr)
{
	if (lptr == NULL)
		lptr = lines;
    
    // FG-2014-12-03: [[ Bug 11688 ]] Hidden paragraphs have a zero height
    if (gethidden())
    {
        lptr = NULL;
        return True;
    }
    
    // SN-2014-09-17: [[ Bug 13462 ]] Added the space above and below each paragraph
    // FG-2014-11-03: [[ Bug 11688 ]] Take all of the top margin into account
    if (attrs != nil)
        theight = MCU_max(((int32_t)theight) - computetopmargin(), 0);
    
	do
	{
        uint2 lheight = fixedheight == 0 ? ceilf(lptr->GetHeight()) : fixedheight;
		if (lheight > theight)
			return False;
		theight -= lheight;
		lptr = lptr->next();
	}
	while (lptr != lines);
	lptr = NULL;
    
    // SN-2014-09-17: [[ Bug 13462 ]] Added the space above and below each paragraph.
    // There is no failure for this paragraph if only the space below does not fit in the field
    // FG-2014-12-03: [[ Bug 11688 ]] Take all of the bottom margin into account
    if (attrs != nil)
        theight = MCU_max(((int32_t)theight) - computebottommargin(), 0);
    
	return True;
}

// JS-2013-05-15: [[ PageRanges ]] pagerange as variant of pageheight
// MW-2014-04-11: [[ Bug 12182 ]] Make sure we use uint4 for field indicies.
Boolean MCParagraph::pagerange(uint2 fixedheight, uint2 &theight,
                               uint4 &tend, MCLine *&lptr)
{
	if (lptr == NULL)
		lptr = lines;
    
    // SN-2014-09-17: [[ Bug 13462 ]] Added the space above and below each paragraph
    // FG-2014-12-03: [[ Bug 11688 ]] Hidden paragraphs have a zero height
    if (gethidden())
    {
        lptr = NULL;
        return True;
    }
    
    // FG-2014-11-03: [[ Bug 11688 ]] Take all of the top margin into account
    if (attrs != nil)
        theight = MCU_max(((int32_t)theight) - computetopmargin(), 0);
    
	do
	{
		uint2 lheight = fixedheight == 0 ? ceilf(lptr->GetHeight()) : fixedheight;
		if (lheight > theight)
			return False;
		theight -= lheight;
        findex_t li, ll;
        lptr->GetRange(li, ll);
        tend += ll;
		lptr = lptr->next();
	}
	while (lptr != lines);
	lptr = NULL;
    
    // SN-2014-09-17: [[ Bug 13462 ]] Added the space above and below each paragraph.
    // There is no failure for this paragraph if only the space below does not fit in the field
    // FG-2014-12-03: [[ Bug 11688 ]] Take all of the bottom margin into account
    if (attrs != nil)
        theight = MCU_max(((int32_t)theight) - computebottommargin(), 0);
    
	return True;
}

bool MCParagraph::imagechanged(MCImage *p_image, bool p_deleting)
{
	bool t_used = false;
	MCBlock *t_block = blocks;

	do
	{
		t_used = t_block->imagechanged(p_image, p_deleting) || t_used;
		t_block = t_block->next();
	}
	while (t_block != blocks);

	return t_used;
}

void MCParagraph::restricttoline(findex_t& si, findex_t& ei)
{
	MCLine *t_line;
	t_line = lines;
	do
	{
		findex_t i, l;
		t_line -> GetRange(i, l);
		if (i >= si && si < (i + l))
		{
			si = i;
			ei = i + l;
			return;
		}
		t_line = t_line -> next();
	}
	while(t_line != lines);

	si = ei = 0;
}

uint2 MCParagraph::heightoflinewithindex(findex_t si, uint2 fixedheight)
{
	MCLine *t_line;
	t_line = lines;
	do
	{
		findex_t i, l;
		t_line -> GetRange(i, l);
		if (i >= si && si < (i + l))
			return fixedheight == 0 ? ceilf(t_line->GetHeight()) : fixedheight;
		t_line = t_line -> next();
	}
	while(t_line != lines);
	return 0;
}
