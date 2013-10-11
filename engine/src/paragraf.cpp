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

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"
#include "mcio.h"

#include "stack.h"
#include "block.h"
#include "line.h"
#include "field.h"
#include "paragraf.h"
#include "execpt.h"
#include "util.h"
#include "mcerror.h"
#include "unicode.h"
#include "text.h"

#include "globals.h"

#include "mctheme.h"

#include "context.h"
#include "exec-interface.h"

const char *ER_reverse[256] =
    {
        "&#0;", "&#1;", "&#2;", "&#3;", "&#4;",
        "&#5;", "&#6;", "&#7;", "&#8;", "&#9;", "&#10;", "&#11;", "&#12;",
        "&#13;", "&#14;", "&#15;", "&#16;", "&#17;", "&#18;", "&#19;",
        "&#20;", "&#21;", "&#22;", "&#23;", "&#24;", "&#25;", "&#26;",
        "&#27;", "&#28;", "&#29;", "&#30;", "&#31;", NULL, NULL, "&quot;", NULL,
        NULL, NULL, "&amp;", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, "&lt;", NULL, "&gt;", NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, "&#127;", "&#128;", "&#129;", "&#130;", "&#131;",
        "&#132;", "&#133;", "&#134;", "&#135;", "&#136;", "&#137;", "&#138;",
        "&#139;", "&#140;", "&#141;", "&#142;", "&#143;", "&#144;", "&#145;",
        "&#146;", "&#147;", "&#148;", "&#149;", "&#150;", "&#151;", "&#152;",
        "&#153;", "&#154;", "&#155;", "&#156;", "&#157;", "&#158;", "&#159;",
        "&nbsp;", "&iexcl;", "&cent;", "&pound;", "&curren;", "&yen;",
        "&brvbar;", "&sect;", "&uml;", "&copy;", "&ordf;", "&laquo;", "&not;",
        "&shy;", "&reg;", "&macr;", "&deg;", "&plusmn;", "&sup2;", "&sup3;",
        "&acute;", "&micro;", "&para;", "&middot;", "&cedil;", "&sup1;",
        "&ordm;", "&raquo;", "&frac14;", "&frac12;", "&frac34;", "&iquest;",
        "&Agrave;", "&Aacute;", "&Acirc;", "&Atilde;", "&Auml;", "&Aring;",
        "&AElig;", "&Ccedil;", "&Egrave;", "&Eacute;", "&Ecirc;", "&Euml;",
        "&Igrave;", "&Iacute;", "&Icirc;", "&Iuml;", "&ETH;", "&Ntilde;",
        "&Ograve;", "&Oacute;", "&Ocirc;", "&Otilde;", "&Ouml;", "&times;",
        "&Oslash;", "&Ugrave;", "&Uacute;", "&Ucirc;", "&Uuml;", "&Yacute;",
        "&THORN;", "&szlig;", "&agrave;", "&aacute;", "&acirc;", "&atilde;",
        "&auml;", "&aring;", "&aelig;", "&ccedil;", "&egrave;", "&eacute;",
        "&ecirc;", "&euml;", "&igrave;", "&iacute;", "&icirc;", "&iuml;",
        "&eth;", "&ntilde;", "&ograve;", "&oacute;", "&ocirc;", "&otilde;",
        "&ouml;", "&divide;", "&oslash;", "&ugrave;", "&uacute;", "&ucirc;",
        "&uuml;", "&yacute;", "&thorn;", "&yuml;"
    };

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
	/* UNCHECKED */ MCStringCreateMutable(0, m_text);
	blocks = NULL;
	lines = NULL;
	focusedindex = 0;
	opened = 0;
	startindex = endindex = originalindex = PARAGRAPH_MAX_LEN;
	state = 0;

	// MW-2012-01-25: [[ ParaStyles ]] All attributes are unset to begin with.
	attrs = nil;
}

MCParagraph::MCParagraph(const MCParagraph &pref) : MCDLlist(pref)
{
	parent = pref.parent;
	/* UNCHECKED */ MCStringMutableCopy(pref.m_text, m_text);
	
	blocks = NULL;
	if (pref.blocks != NULL)
	{
		MCBlock *bptr = pref.blocks;
		do
		{
			MCBlock *tbptr = new MCBlock(*bptr);
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

	lines = NULL;
	focusedindex = 0;
	startindex = endindex = originalindex = PARAGRAPH_MAX_LEN;
	opened = 0;
	state = 0;
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
	
	// Don't let the text go away until anything referencing it is gone
	MCValueRelease(m_text);
}

MCBlock* MCParagraph::AppendText(MCStringRef p_string)
{
	// Ensure the block list has been set up
	inittext();
	
	// Is the last block empty or does a new one need to be created?
	MCBlock *t_block = blocks->prev();
	if (t_block->GetLength() > 0)
	{
		// Block already contains data, create a new one
		MCBlock *t_newblock = new MCBlock;
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
	/* UNCHECKED */ MCStringAppend(m_text, p_string);
	
	// Set the indices for the block containing this text
	t_block->SetRange(t_cur_len, t_new_length);
	return t_block;
}

bool MCParagraph::TextIsWordBreak(codepoint_t p_codepoint)
{
	return p_codepoint == ' ';
}

bool MCParagraph::TextIsLineBreak(codepoint_t p_codepoint)
{
	return p_codepoint == '\v';
}

bool MCParagraph::TextIsSentenceBreak(codepoint_t p_codepoint)
{
	return p_codepoint == '.';
}

bool MCParagraph::TextIsParagraphBreak(codepoint_t p_codepoint)
{
	return p_codepoint == '\n';
}

bool MCParagraph::TextIsPunctuation(codepoint_t p_codepoint)
{
	return ispunct(p_codepoint);
}

bool MCParagraph::TextFindNextParagraph(MCStringRef p_string, findex_t p_after, findex_t &r_next)
{
	codepoint_t t_cp;
	uindex_t t_length = MCStringGetLength(p_string);
	while (p_after < t_length && !TextIsParagraphBreak(MCStringGetCodepointAtIndex(p_string, p_after)))
		p_after++;
	
	if (p_after == t_length)
		return false;
	
	r_next = p_after + 1;
	return true;
}

bool MCParagraph::visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor* p_visitor)
{
	bool t_continue;
	t_continue = true;

	if (p_style == VISIT_STYLE_DEPTH_LAST)
		t_continue = p_visitor -> OnParagraph(this);
	
	if (t_continue && blocks != NULL)
	{
		MCBlock *bptr = blocks;
		do
		{
			t_continue = bptr -> visit(p_style, p_part, p_visitor);
			bptr = bptr->next();
		}
		while(t_continue && bptr != blocks);
	}

	if (t_continue && p_style == VISIT_STYLE_DEPTH_FIRST)
		t_continue = p_visitor -> OnParagraph(this);

	return t_continue;
}

// **** mutate blocks
IO_stat MCParagraph::load(IO_handle stream, const char *version, bool is_ext)
{
	IO_stat stat;
	uint1 type;

	// This string can contain a mixture of Unicode and native...
	uint32_t t_length;
	MCAutoPointer<char> t_text_data;
	if ((stat = IO_read_string(&t_text_data, t_length, stream, 2, true, false)) != IO_NORMAL)
		return stat;
	
	// The paragraph's text is accumulated into here as it can change between
	// native and UTF-16 encodings on a block-by-block basis.
	MCAutoStringRef t_text;
	if (!MCStringCreateMutable(0, &t_text))
		return IO_ERROR;

	// MW-2012-03-04: [[ StackFile5500 ]] If this is an extended paragraph then
	//   load in the attribute extension record.
	if (is_ext)
		if ((stat = loadattrs(stream)) != IO_NORMAL)
			return IO_ERROR;
	
	// If the whole text isn't covered by the saved blocks, the index of the
	// portion not covered needs to be retained so that it can be added to
	// the paragraph text at the end of loading.
	uindex_t t_last_added = 0;
	
	while (True)
	{
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return stat;
		switch (type)
		{
		// MW-2012-03-04: [[ StackFile5500 ]] Handle either a normal block, or
		//   an extended block.
		case OT_BLOCK:
		case OT_BLOCK_EXT:
			{
				MCBlock *newblock = new MCBlock;
				newblock->setparent(this);
				
				// MW-2012-03-04: [[ StackFile5500 ]] If the tag was actually an
				//   extended block, then pass in 'true' for is_ext.
				if ((stat = newblock->load(stream, version, type == OT_BLOCK_EXT)) != IO_NORMAL)
				{
					delete newblock;
					return stat;
				}
				
				// The indices returned here are *wrong* from the point of view
				// of the refactored Unicode paragraph - the block as loaded
				// stores byte indices, not UTF-16 value indices. These wrong
				// values are needed to ensure the paragraph text is loaded 
				// using the correct encoding and get fixed up below.
				findex_t index, len;
				newblock->GetRange(index, len);
				t_last_added = index+len;
				if (newblock->IsSavedAsUnicode())
				{
					len >>= 1;
					if (len && t_length > 0)
					{
						uint2 *dptr = (uint2*)(*t_text_data + index);
						
						// Byte swap, if required
						uindex_t t_len = len;
						while (len--)
							swap_uint2(dptr++);
						
						// The indices used by the block are incorrect and need
						// to be updated (offsets into the stored string and
						// the string held by the paragraph will differ if any
						// portion of the stored string was non-UTF-16)
						newblock->SetRange(MCStringGetLength(*t_text), t_len);
						
						// Append to the paragraph text
						if (!MCStringAppendChars(*t_text, dptr - t_len, t_len))
							return IO_ERROR;
					}
				}
				else
				{
					if (MCtranslatechars && len && t_length > 0)
#ifdef __MACROMAN__
						IO_iso_to_mac(*t_text_data + index, len);
#else
						IO_mac_to_iso(*t_text_data + index, len);
#endif

					// Fix the indices used by the block
					newblock->SetRange(MCStringGetLength(*t_text), len);
					
					// String is in native format. Append to paragraph text
					if (!MCStringAppendNativeChars(*t_text, (const char_t*)(*t_text_data + index), len))
						return IO_ERROR;
				}
				newblock->appendto(blocks);
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
				if (!MCStringAppendNativeChars(*t_text, (const char_t*)*t_text_data, t_length))
					return IO_ERROR;
				t_last_added = t_length;
			}
				
			// Ensure that all the text was covered
			if (t_last_added != t_length)
				return IO_ERROR;

			MCS_seek_cur(stream, -1);
			MCValueAssign(m_text, *t_text);
			return IO_NORMAL;
		}
	}

	// This point shouldn't be reached
	assert(false);
	return IO_NORMAL;
}

// **** require blocks
IO_stat MCParagraph::save(IO_handle stream, uint4 p_part)
{
	IO_stat stat;
	defrag();
	
	// The string data that will get written out. It can't be just done as a
	// StringRef without breaking file format compatibility.
	uindex_t t_data_len;
	const char *t_data;
	if (MCStringIsNative(m_text))
	{
		t_data_len = MCStringGetLength(m_text);
		t_data = (const char *)MCStringGetNativeCharPtr(m_text);
	}
	else
	{
		t_data_len = MCStringGetLength(m_text) * sizeof(unichar_t);
		t_data = (const char *)MCStringGetCharPtr(m_text);
	}
	
	if (!MCStringIsNative(m_text))
	{
		// For file format compatibility, swap_uint2 must be called on each 
		// character in the UTF-16 string (Unicodeness is now a paragraph
		// property, not a block property, so it is done for all the text)
		unichar_t *t_swapped_data = new unichar_t[t_data_len/sizeof(unichar_t)];
		memcpy(t_swapped_data, t_data, t_data_len);
		for (uindex_t i = 0; i < MCStringGetLength(m_text); i++)
		{
			swap_uint2(&t_swapped_data[i]);
		}
		t_data = (char *)t_swapped_data;
	}
	
	// MW-2012-03-04: [[ StackFile5500 ]] If the paragraph has attributes and 5.5
	//   stackfile format has been requested, then output an extended paragraph.
	bool t_is_ext;
	if (MCstackfileversion >= 5500 && attrs != nil)
		t_is_ext = true;
	else
		t_is_ext = false;
	
	if ((stat = IO_write_uint1(t_is_ext ? OT_PARAGRAPH_EXT : OT_PARAGRAPH, stream)) != IO_NORMAL)
		return stat;

	if ((stat = IO_write_string(MCString(t_data, t_data_len), stream, 2, true)) != IO_NORMAL)
		return stat;

	// If the string had to be byte swapped, delete the allocated data
	if (!MCStringIsNative(m_text))
		delete[] t_data;
	
	// MW-2012-03-04: [[ StackFile5500 ]] If this is an extended paragraph then
	//   write out the attribtues.
	if (t_is_ext)
		if ((stat = saveattrs(stream)) != IO_NORMAL)
			return IO_ERROR;
 
	// Write out the blocks
	if (blocks != NULL)
	{
		MCBlock *tptr = blocks;
		do
		{
			if ((stat = tptr->save(stream, p_part)) != IO_NORMAL)
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

	return t_changed;
}

//clear blocks with zero length

// **** mutate blocks
Boolean MCParagraph::clearzeros()
{
	Boolean reflow = False;
	if (blocks != NULL && blocks->next() != blocks)
	{
		MCBlock *bptr = blocks;
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
	while (lines != NULL)
	{
		MCLine *lptr = lines->remove(lines);
		delete lptr;
	}
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
	}
}

// MW-2012-01-25: [[ ParaStyles ]] This method causes a reflow of the paragraph depending
//   on the setting of 'dontWrap'.
void MCParagraph::layout()
{
	if (getdontwrap())
		noflow();
	else
		flow();
}

//reflow paragraph with wrapping
void MCParagraph::flow(void)
{
	// MW-2008-03-24: [[ Bug 6194 ]] Make sure we clean the paragraph of broken blocks and empty
	//   blocks before we reflow - failing to do this causes strange effects when wrapping unicode
	//   text.
	defrag();

	MCBlock *bptr = blocks;
	MCLine *lptr = new MCLine(this);
	MCLine *olptr = lines;

	// MW-2012-01-25: [[ ParaStyles ]] Compute the normal and first line layout width for
	//   wrapping purposes.
	int32_t pwidth, twidth;
	computelayoutwidths(pwidth, twidth);

	do
	{
		bptr = lptr -> fitblocks(bptr, blocks, twidth);
		if (bptr != blocks)
		{
			if (olptr != NULL)
			{
				olptr->takebreaks(lptr);
				olptr = olptr->next();
				if (olptr == lines)
					olptr = NULL;
			}
			else
			{
				lptr->appendto(lines);
				lptr = new MCLine(this);
			}
		}
		
		// MW-2008-06-12: [[ Bug 6482 ]] Make sure we only take the firstIndent into account
		//   on the first line of the paragraph.
		twidth = pwidth;
	}
	while (bptr != blocks);
	if (olptr != NULL)
	{
		olptr->takebreaks(lptr);
		delete lptr;
		olptr = olptr->next();
		while (olptr != lines)
		{
			lptr = olptr->remove(olptr);
			delete lptr;
		}
	}
	else
	{
		lptr->appendto(lines);
		lptr->makedirty();
	}

	state &= ~PS_LINES_NOT_SYNCHED;
}

//flow paragraph and don't wrap
void MCParagraph::noflow(void)
{
	// MW-2008-04-01: [[ Bug ]] Calling clearzeros meant styling of empty 
	// selections didn't work.
	defrag();
	if (lines == NULL)
		lines = new MCLine(this);
	else
		while (lines->next() != lines)
		{
			MCLine *lptr = lines->remove(lines);
			delete lptr;
		}
	MCBlock *bptr = blocks;
	do
	{
		bptr->reset();
		bptr = bptr->next();
	}
	while (bptr != blocks);
	lines->appendall(blocks);

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
	if (state & PS_FRONT && lptr == lines
	        || state & PS_BACK && lptr == lines->prev()
	        || endindex >= i && startindex < i + l)
	{
		bool t_show_front;
		t_show_front = (state & PS_FRONT) != 0 && (parent -> getflag(F_LIST_BEHAVIOR) || this != parent -> getparagraphs() || lptr != lines);
	
		MCRectangle srect;
		if (t_show_front || startindex < i)
			srect.x = sx;
		else
		{
			srect.x = x;
			if (startindex > i)
				srect.x += lptr->GetCursorX(MCU_min(gettextlength(), startindex));
		}
		
		bool t_show_back;
		t_show_back = (state & PS_BACK) != 0 && (parent -> getflag(F_LIST_BEHAVIOR) || this != parent -> getparagraphs() -> prev() || lptr != lines -> prev());
		if (t_show_back || endindex > i + l)
			srect.width = swidth - (srect.x - sx);
		else
		{
			int2 sx = x + lptr->GetCursorX(MCU_min(gettextlength(), endindex));
			if (sx > srect.x)
				srect.width = sx - srect.x;
			else
				return;
		}
		
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

		dc->fillrect(srect);

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
			srect.x += lptr->GetCursorX(compstart);
		if (compend > i + l)
			srect.width = lptr->getwidth() - (srect.x - x);
		else
			srect.width = x + lptr->GetCursorX(compend) - srect.x;
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
				srect.x += lptr->GetCursorX(compconvstart);
			if (compconvend > i + l)
				srect.width = lptr->getwidth() - (srect.x - x);
			else
				srect.width = x + lptr->GetCursorX(compconvend) - srect.x;

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
			srect.x += lptr->GetCursorX(fstart);

		bool t_has_end;
		t_has_end = fend <= i + l;
		if (fend > i + l)
			srect.width = lptr->getwidth() - (srect.x - x);
		else
			srect.width = x + lptr->GetCursorX(fend) - srect.x;

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

	uint2 ascent, descent;
	ascent = fixeda;
	descent = fixedd;

	// MW-2012-03-16: [[ Bug 10001 ]] Compute the paragraph offset (from leftmargin) and minimal width.
	int32_t t_paragraph_offset, t_paragraph_width;
	computeparaoffsetandwidth(t_paragraph_offset, t_paragraph_width);

	// MW-2012-03-15: [[ Bug 10001 ]] Compute the selection offset and width. Notice that
	//   the width is at least the textwidth.
	int32_t t_select_x, t_select_width;
	t_select_x = x + t_paragraph_offset;
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
	MCRectangle t_outer_rect, t_inner_rect;
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
		t_color . pixel = attrs -> background_color;
		MCscreen -> querycolor(t_color);
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
			ascent = lptr -> getascent();
			descent = lptr -> getdescent();
		}

		if (t_current_y < t_clip . y + t_clip . height && t_current_y + ascent + descent > t_clip . y)
		{
			int32_t t_current_x;
			t_current_x = t_inner_rect . x + computelineinneroffset(t_inner_rect . width, lptr);

			if (startindex != endindex || state & PS_FRONT || state & PS_BACK)
				fillselect(dc, lptr, t_current_x, t_current_y, ascent + descent, t_select_x, t_select_width);

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
						Pixmap p;
						int2 x, y;
						MCColor fc, hc;
						parent->getforecolor(DI_FORE, False, True, fc, p, x, y, dc, parent);
						parent->getforecolor(DI_HILITE, False, True, hc, p, x, y, dc, parent);
						if (hc.pixel == fc.pixel)
							parent->setforeground(dc, DI_BACK, False, True);
					}
					else
						parent->setforeground(dc, DI_BACK, False, True);
				}
				MCFontDrawText(parent -> getfontref(), t_string, t_string_length, t_is_unicode, dc, t_current_x - getlistlabelwidth(), t_current_y + ascent - 1, false);
				if ((state & PS_FRONT) != 0 && this != parent -> getparagraphs())
					parent -> setforeground(dc, DI_FORE, False, True);
			}

			lptr->draw(dc, t_current_x, t_current_y + ascent - 1, si, ei, m_text, pstyle);
			if (fstart != fend)
				drawfound(dc, lptr, t_current_x, t_current_y, ascent + descent, fstart, fend);
			if (compstart != compend)
				drawcomposition(dc, lptr, t_current_x, t_current_y, ascent + descent, compstart, compend, compconvstart, compconvend);
		}

		t_current_y += ascent + descent;

		lptr = lptr->next();
	}
	while (lptr != lines);

	dc -> setclip(t_clip);
	
	// MW-2012-01-08: [[ Paragraph Border ]] Render the paragraph's border (if
	//   any).
	if (!MCU_equal_rect(t_inner_border_rect, t_outer_rect) || gethgrid() || getvgrid())
	{
		if (attrs != nil && (attrs -> flags & PA_HAS_BORDER_COLOR) != 0)
		{
			MCColor t_color;
			t_color . pixel = attrs -> border_color;
			MCscreen -> querycolor(t_color);
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
				MCRectangle t_prev_inner, t_prev_outer;
				prev() -> computerects(x, y, textwidth, prev() -> getwidth(), pgheight, t_prev_outer, t_prev_inner);
				
				// MW-2012-02-10: [[ FixedTable ]] The adjustrects method uses both rects so make
				//   sure we adjust the prev inner rect for padding.
				// MW-2012-03-19: [[ Bug 10069 ]] Make sure the appropriate h/v padding is used to
				//   adjust the rect.
				t_prev_inner . x = t_inner_rect . x - prev() -> gethpadding();
				t_prev_inner . width = t_inner_rect . width + 2 * prev() -> gethpadding();
				t_prev_inner . y = t_inner_rect . y - prev() -> getvpadding();
				t_prev_inner . height = t_inner_rect . height + 2 * prev() -> getvpadding();
				
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
			int4 x = t[0] + t_delta;
			while (x <= t_limit)
			{
				dc->drawline(x, t_inner_border_rect . y, x, t_inner_border_rect . y + t_inner_border_rect . height);
				
				if (ct < nt - 1)
					x = t_delta + t[++ct];
				else if (nt == 1)
					x += t[0];
				else
					x += t[nt - 1] - t[nt - 2];

				// MW-2012-02-10: [[ FixedTable ]] If we have reached the final tab in fixed
				//   table mode, we are done.
				if (ct == nt - 2 && t[nt - 2] == t[nt - 1])
					break;
			}
		}

		parent->setforeground(dc, DI_FORE, False, True);
	}
}

Boolean MCParagraph::getatts(findex_t si, findex_t ei, Font_textstyle textstyle, const char *&fname,
                             uint2 &size, uint2 &fstyle, const MCColor *&color,
                             const MCColor *&backcolor, int2 &shift, bool& specstyle,
                             uint2 &mixed)
{
	Boolean ahas = False;
	Boolean chas = False;
	Boolean bchas = False;
	Boolean shas = False;
	const char *defname = fname;
	uint2 defsize = size;
	uint2 defstyle = fstyle;
	bool defspecstyle = specstyle;
	if (ei > gettextlength())
		ei = gettextlength();
	MCBlock *startptr = indextoblock(si, False);
	MCBlock *bptr = startptr;
	findex_t i, l;
	bptr->GetRange(i, l);
	
	// MW-2009-01-16: [[ Bug 7548 ]] The problem here is that indextoblock() isn't
	//   returning quite what we need. When inserting text, the block used for styling
	//   is (in order):
	//     - the empty block at the index (if any)
	//     - the block to the immediate left (if any)
	//     - the block to the immediate right.
	//   In comparison, indextoblock returns the block after the first one containing
	//   the index. Which is the following (in order):
	//     - the empty block at the index (if any)
	//     - the block to the immediate right (if any)
	//     - the block to the immediate left.
	//   Thus we need to adjust the output of indextoblock in the case of a [si, ei)
	//   being empty.
	if (si == ei && si == i && l != 0 && bptr != blocks)
	{
		bptr = bptr -> prev();
		bptr -> GetRange(i, l);
		startptr = bptr;
	}
	
	do
	{
		const char *tname;
		uint2 tsize;
		uint2 tstyle;
		bool tspecstyle;

		// MW-2012-02-17: [[ SplitTextAttrs ]] Get any font attrs the block has.
		if (bptr -> gettextfont(tname))
		{
			if (bptr == startptr)
				fname = tname;
			ahas = True;
		}
		else
			tname = defname;

		if (bptr -> gettextsize(tsize))
		{
			if (bptr == startptr)
				size = tsize;
			ahas = True;
		}
		else
			tsize = defsize;

		if (bptr -> gettextstyle(tstyle))
		{
			tspecstyle = MCF_istextstyleset(tstyle, textstyle);
			if (bptr == startptr)
			{
				fstyle = tstyle;
				specstyle = tspecstyle;
			}
			ahas = True;
		}
		else
			tstyle = defstyle, tspecstyle = defspecstyle;

		if (ahas)
		{
			if (fname != tname)
				mixed |= MIXED_NAMES;
			if (tsize != size)
				mixed |= MIXED_SIZES;
			if (tstyle != fstyle)
				mixed |= MIXED_STYLES;
			if (tspecstyle != specstyle)
				mixed |= MIXED_SPEC_STYLE;
		}
		const MCColor *tcolor = color;
		if (bptr->getcolor(tcolor))
			if (chas)
			{
				if (tcolor->red != color->red || tcolor->green != color->green
				        || tcolor->blue != color->blue)
					mixed |= MIXED_COLORS;
			}
			else
			{
				if (bptr != startptr)
					mixed |= MIXED_COLORS;
				chas = True;
				color = tcolor;
			}
		else
			if (chas)
				mixed |= MIXED_COLORS;

		const MCColor *btcolor = color;
		if (bptr->getbackcolor(btcolor))
			if (bchas)
			{
				if (btcolor->red != backcolor->red
				        || btcolor->green != backcolor->green
				        || btcolor->blue != backcolor->blue)
					mixed |= MIXED_COLORS;
			}
			else
			{
				if (bptr != startptr)
					mixed |= MIXED_COLORS;
				bchas = True;
				backcolor = btcolor;
			}
		else
			if (bchas)
				mixed |= MIXED_COLORS;
		int2 tshift;
		if (bptr->getshift(tshift))
			if (shas)
			{
				if (tshift != shift)
					mixed |= MIXED_SHIFT;
			}
			else
			{
				if (bptr != startptr)
					mixed |= MIXED_SHIFT;
				shas = True;
				shift = tshift;
			}
		else
			if (shas)
				mixed |= MIXED_SHIFT;
		bptr->GetRange(i, l);
		bptr = bptr->next();
	}
	while (i + l < ei);
	
	return ahas || chas || bchas || shas;
}

void MCParagraph::setatts(findex_t si, findex_t ei, Properties p, void *value, bool p_from_html)
{
	bool t_blocks_changed;
	t_blocks_changed = false;

	defrag();
	MCBlock *bptr = indextoblock(si, False);
	findex_t i, l;
	do
	{
		bptr->GetRange(i, l);
		if (i < si)
		{
			MCBlock *tbptr = new MCBlock(*bptr);
			bptr->append(tbptr);
			bptr->SetRange(i, si - i);
			tbptr->SetRange(si, l - (si - i));
			bptr = bptr->next();
			bptr->GetRange(i, l);
			t_blocks_changed = true;
		}
		else
			bptr->close();
		if (i + l > ei)
		{
			MCBlock *tbptr = new MCBlock(*bptr);
			// MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
			//   fontref so it can compute its.
			if (opened)
				tbptr->open(parent -> getfontref());
			bptr->append(tbptr);
			bptr->SetRange(i, ei - i);
			tbptr->SetRange(ei, l - ei + i);
			t_blocks_changed = true;
		}
		switch (p)
		{
		case P_FORE_COLOR:
			bptr->setcolor((MCColor *)value);
			break;
		case P_BACK_COLOR:
			bptr->setbackcolor((MCColor *)value);
			break;
		case P_TEXT_SHIFT:
			bptr->setshift((uint4)(intptr_t)value);
			break;
		case P_IMAGE_SOURCE:
			{
				bptr->setatts(p, value);
				
				// MW-2008-04-03: [[ Bug ]] Only add an extra block if this is coming from
				//   html parsing.
				if (p_from_html)
				{
					MCBlock *tbptr = new MCBlock(*bptr); // need a new empty block
					tbptr->freerefs();                   // for HTML continuation
					// MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
					//   fontref so it can compute its.
					if (opened)
						tbptr->open(parent -> getfontref());
					bptr->append(tbptr);
					tbptr->SetRange(ei, 0);
					t_blocks_changed = true;
				}
			}
			break;
		default:
			bptr->setatts(p, value);
			break;
		}
		// MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
		//   fontref so it can compute its.
		if (opened)
			bptr->open(parent -> getfontref());
		bptr = bptr->next();
	}
	while (i + l < ei);

	if (t_blocks_changed)
	{
		state |= PS_LINES_NOT_SYNCHED;
	}
}

// MW-2008-03-27: [[ Bug 5093 ]] Rewritten to more correctly insert blocks around
//   imagesource characters.
MCBlock *MCParagraph::indextoblock(findex_t tindex, Boolean forinsert)
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
						MCBlock *t_new_block;
						t_new_block = new MCBlock(*t_block);
						t_new_block -> setatts(P_IMAGE_SOURCE, (void *)kMCEmptyString);

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
						MCBlock *t_new_block;
						t_new_block = new MCBlock(*t_block);
						t_new_block -> setatts(P_IMAGE_SOURCE, (void *)kMCEmptyString);

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
			if (tindex == i + l && bptr->next() != blocks)
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

void MCParagraph::join()
{
	if (blocks == NULL)
		inittext();

	MCParagraph *pgptr = next();
	
	// MW-2012-01-07: If the current paragraph is empty and has no attrs, then copy
	//   the next paragraphs attrs.
	// MW-2012-08-31: [[ Bug 10344 ]] If the textsize is 0 then always take the next
	//   paragraphs attrs.
	if (gettextlength() == 0)
		copyattrs(*pgptr);

	// MW-2006-04-13: If the total new text size is greater than 65536 - 34 we just delete the next paragraph
	uint4 t_new_size;
	t_new_size = gettextlengthcr() + pgptr -> gettextlength();;
	if (t_new_size > PARAGRAPH_MAX_LEN - PG_PAD - 1)
	{
		delete pgptr;
		return;
	}

	focusedindex = MCStringGetLength(m_text);
	/* UNCHECKED */ MCStringAppend(m_text, pgptr->m_text);

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
}

void MCParagraph::split() //split paragraphs on return
{
	MCBlock *bptr = indextoblock(focusedindex, False);
	findex_t skip = 0;
	
	if (focusedindex < MCStringGetLength(m_text) && TextIsLineBreak(GetCodepointAtIndex(focusedindex)))
		skip = IncrementIndex(focusedindex) - focusedindex;
	
	MCParagraph *pgptr = new MCParagraph;
	pgptr->parent = parent;

	// MW-2012-01-25: [[ ParaStyles ]] Copy the attributes from the first para.
	pgptr -> copyattrs(*this);
	// MW-2012-11-20: [[ ParaListIndex]] When splitting a paragraph we don't copy the
	//   list index.
	pgptr -> setlistindex(0);

	if (!MCStringIsEmpty(m_text))
	{
		MCRange t_range = MCRangeMake(focusedindex, MCStringGetLength(m_text) - focusedindex);
		/* UNCHECKED */ MCStringMutableCopySubstring(m_text, t_range, pgptr->m_text);
		/* UNCHECKED */ MCStringSubstring(m_text, MCRangeMake(0, focusedindex));
	}
	else
		/* UNCHECKED */ MCStringCreateMutable(0, pgptr->m_text);

	// Trim the block containing the split so that it ends at the split point
	bptr = indextoblock(focusedindex, False);
	findex_t i, l;
	bptr->GetRange(i, l);
	bptr->MoveRange(0, focusedindex - (i + l));
	
	// Create a new block to cover the range from the split point to the end
	// of the original block.
	MCBlock *tbptr = new MCBlock(*bptr);
	bptr->append(tbptr);
	blocks->splitat(tbptr);
	pgptr->blocks = tbptr;
	tbptr->setparent(pgptr);
	tbptr->SetRange(0, (i + l) - focusedindex - skip);
	tbptr = tbptr->next();
	
	// Adjust the blocks after the split as they now belong to the new
	// paragraph (and therefore have different indices, too).
	while (tbptr != pgptr->blocks)
	{
		tbptr->setparent(pgptr);
		tbptr->MoveRange(-(focusedindex + skip), 0);
		tbptr->setparent(pgptr);
		tbptr = tbptr->next();
	}
	
	// MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
	//   fontref so it can compute its.
	if (opened)
		pgptr->open(parent -> getfontref());
	append(pgptr);
	deletelines();
}

void MCParagraph::deletestring(findex_t si, findex_t ei)
{
	MCBlock *sbptr = indextoblock(si, False);
	MCBlock *ebptr = indextoblock(ei, False);
	
	// Don't try to remove text beyond the end of the paragraph
	if (ei > MCStringGetLength(m_text))
		return;
	
	findex_t length = ei - si;
	if (focusedindex >= ei)
		focusedindex -= length;
	else
		if (focusedindex > si)
			focusedindex = si;
	startindex = endindex = originalindex = focusedindex;
	
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
	uindex_t t_length = MCStringGetLength(m_text);
	/* UNCHECKED */ MCStringRemove(m_text, MCRangeMake(si, ei-si));

	clearzeros();
	state |= PS_LINES_NOT_SYNCHED;
}

MCParagraph *MCParagraph::copystring(findex_t si, findex_t ei)
{
	// The string is copied by duplicating this paragraph and then removing all
	// text outside of the range [si, ei). This preserves all attributes, etc
	MCParagraph *pgptr = new MCParagraph(*this);
	
	if (ei != MCStringGetLength(m_text))
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
	t_cur_length = MCStringGetLength(m_text);
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
		/* UNCHECKED */ MCStringInsertSubstring(m_text, focusedindex, p_string, t_range);

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
			MCRange t_range = MCRangeMake(t_index, t_nextpara - t_index - 1);
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
			t_paragraph -> finsertnobreak(p_string, MCRangeMake(t_index, t_length - t_index));
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
	case FT_DELBCHAR:
		if (focusedindex == 0)
			return -1;
		si = DecrementIndex(focusedindex);
		break;
	case FT_DELBWORD:
		if (focusedindex == 0)
			return -1;
		si = DecrementIndex(focusedindex);
		while (si && TextIsWordBreak(GetCodepointAtIndex(si)))
		{
			si = DecrementIndex(si);
			if (si < bindex)
			{
				bptr = bptr->prev();
				bptr->GetRange(bindex, blength);
			}
		}
		while (si && !TextIsWordBreak(GetCodepointAtIndex(DecrementIndex(si))))
		{
			si = DecrementIndex(si);
			if (si < bindex)
			{
				bptr = bptr->prev();
				bptr->GetRange(bindex, blength);
			}
		}
		break;
	case FT_DELFCHAR:
		if (focusedindex == gettextlength())
			return 1;
		ei = IncrementIndex(focusedindex);
		break;
	case FT_DELFWORD:
		if (focusedindex == gettextlength())
			return 1;
		ei = IncrementIndex(focusedindex);
		while (ei < gettextlength() && TextIsWordBreak(GetCodepointAtIndex(ei)))
		{
			ei = IncrementIndex(ei);
			if (ei >= bindex + blength)
			{
				bptr = bptr->next();
				bptr->GetRange(bindex, blength);
			}
		}
		while (ei < gettextlength() && !TextIsWordBreak(GetCodepointAtIndex(ei)))
		{
			ei = IncrementIndex(ei);
			if (ei >= bindex + blength)
			{
				bptr = bptr->next();
				bptr->GetRange(bindex, blength);
			}
		}
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

uint1 MCParagraph::fmovefocus(Field_translations type)
{
	findex_t oldfocused = focusedindex;
	findex_t bindex, blength;
	MCBlock *bptr = indextoblock(focusedindex, False);
	bptr->GetRange(bindex, blength);
	switch (type)
	{
	case FT_LEFTCHAR:
		if (focusedindex == 0)
			return FT_LEFTCHAR;
		focusedindex = DecrementIndex(focusedindex);
		break;
	case FT_LEFTWORD:
		// MW-2012-11-14: [[ Bug 10504 ]] Corrected loop to ensure the right chars
		//   are accessed when dealing with Unicode blocks.
		if (focusedindex == 0)
			return FT_LEFTCHAR;
		focusedindex = DecrementIndex(focusedindex);
		if (focusedindex < bindex)
		{
			bptr = bptr -> prev();
			bptr -> GetRange(bindex, blength);
		}
		while (focusedindex && TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
		{
			focusedindex = DecrementIndex(focusedindex);
			if (focusedindex < bindex)
			{
				bptr = bptr->prev();
				bptr->GetRange(bindex, blength);
			}
		}
		for(;;)
		{
			if (focusedindex == 0)
				break;
	
			// MW-2012-12-04: [[ Bug 10578 ]] Make sure we navigate to the beginning of the word
			//   so save current index, then restore if it points to a space.
			findex_t t_previous_focusedindex;
			t_previous_focusedindex = focusedindex;
			focusedindex = DecrementIndex(focusedindex);
			if (focusedindex < bindex)
			{
				bptr = bptr->prev();
				bptr->GetRange(bindex, blength);
			}
			if (TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
			{
				focusedindex = t_previous_focusedindex;
				break;
			}
		}
		break;
	case FT_RIGHTCHAR:
		if (focusedindex == gettextlength())
			return FT_RIGHTCHAR;
		focusedindex = IncrementIndex(focusedindex);
		break;
	case FT_RIGHTWORD:
		if (focusedindex == gettextlength())
			return FT_RIGHTCHAR;
		focusedindex += IncrementIndex(focusedindex);
		if (focusedindex >= bindex + blength)
		{
			bptr = bptr->next();
			bptr->GetRange(bindex, blength);
		}
		while (focusedindex < gettextlength() && TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
		{
			focusedindex = IncrementIndex(focusedindex);
			if (focusedindex >= bindex + blength)
			{
				bptr = bptr->next();
				bptr->GetRange(bindex, blength);
			}
		}
		while (focusedindex < gettextlength() && TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
		{
			focusedindex = IncrementIndex(focusedindex);
			if (focusedindex >= bindex + blength)
			{
				bptr = bptr->next();
				bptr->GetRange(bindex, blength);
			}
		}
		break;
	case FT_BOS:
		while (focusedindex < gettextlength() && TextIsSentenceBreak(GetCodepointAtIndex(focusedindex)))
		{
			focusedindex = DecrementIndex(focusedindex);
			if (focusedindex < bindex)
			{
				bptr = bptr->prev();
				bptr->GetRange(bindex, blength);
			}
		}
		if (focusedindex)
			focusedindex = IncrementIndex(focusedindex);
		while (focusedindex < gettextlength() && TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
		{
			focusedindex = IncrementIndex(focusedindex);
			if (focusedindex >= bindex + blength)
			{
				bptr = bptr->next();
				bptr->GetRange(bindex, blength);
			}
		}
		if (focusedindex == oldfocused)
			return FT_BOS;
		break;
	case FT_EOS:
		while (focusedindex < gettextlength() && TextIsSentenceBreak(GetCodepointAtIndex(focusedindex)))
		{
			focusedindex = IncrementIndex(focusedindex);
			if (focusedindex >= bindex + blength)
			{
				bptr = bptr->next();
				bptr->GetRange(bindex, blength);
			}
		}
		if (focusedindex < gettextlength())
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
		focusedindex = gettextlength();
		break;
	case FT_RIGHTPARA:
		if (focusedindex == gettextlength())
			return FT_RIGHTPARA;
		focusedindex = gettextlength();
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
							if (originalindex < gettextlength() && TextIsWordBreak(GetCodepointAtIndex(originalindex)))
							{
								originalindex = findwordbreakafter(bptr, originalindex);
								bptr = indextoblock(originalindex, False);
								bptr -> AdvanceIndex(originalindex);
							}
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
	uint2 theight;
	if (fixedheight == 0)
		theight = lptr->getheight();
	else
		theight = fixedheight;
	
	// MW-2012-02-27: [[ Bug ]] We count the pixel 'ty + theight' to be part of
	//   the next line, so loop until >= rather than >.
	while (y >= ty + theight && lptr->next() != lines)
	{
		ty += theight;
		lptr = lptr->next();
		if (fixedheight == 0)
			theight = lptr->getheight();
	};

	// MW-2012-01-08: [[ ParaStyles ]] Adjust the end of processing to
	//   after any spacing.
	ty += computebottommargin();

	// MW-2012-01-08: [[ ParaStyles ]] Adjust the x start taking into account
	//   indents, list indents and alignment. (Field to Paragraph so -ve)
	x -= computelineoffset(lptr);

	focusedindex = lptr->GetCursorIndex(x, False);
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
					bptr = indextoblock(originalindex, False);
					originalindex = findwordbreakafter(bptr, originalindex);
					if (originalindex < gettextlength() && !TextIsWordBreak(GetCodepointAtIndex(originalindex)))
					{
						originalindex = findwordbreakafter(bptr, originalindex);
						bptr = indextoblock(originalindex, False);
						bptr -> AdvanceIndex(originalindex);
					}

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
					focusedindex = findwordbreakafter(bptr, focusedindex);
					if (focusedindex < gettextlength() && !TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
					{
						focusedindex = findwordbreakafter(bptr, focusedindex);
						bptr = indextoblock(focusedindex, False);
						bptr -> AdvanceIndex(focusedindex);
					}

					bptr = indextoblock(originalindex, False);
					if (originalindex != startindex)
						originalindex = DecrementIndex(originalindex);
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
				if (focusedindex < gettextlength() && !TextIsWordBreak(GetCodepointAtIndex(focusedindex)))
					bptr -> AdvanceIndex(focusedindex);
					
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
	dirty.width = dirty.height = 0;

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

	uint2 height = fixedheight;
	MCLine *lptr = lines;
	do
	{
		// MW-2013-01-28: [[ Bug 10652 ]] When fixedheight is non-zero, the line height
		//   can differ from the line height we use - so fetch the actual height so we
		//   can adjust the dirty rect.
		int32_t t_line_height;
		t_line_height = lptr -> getheight();
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
				if (lptr == lines)
					dirty.y -= t_space_above;
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
	blocks = new MCBlock;
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
MCRectangle MCParagraph::getcursorrect(findex_t fi, uint2 fixedheight, bool p_include_space)
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
	drect.y = 1 + t_space_above;

	MCLine *lptr;
	findex_t i, l;
	bool t_first_line;
	lptr = lines;
	lptr->GetRange(i, l);
	t_first_line = true;
	while (fi >= i + l && lptr->next() != lines)
	{
		if (fixedheight == 0)
			drect.y += lptr->getheight();
		else
			drect.y += fixedheight;
		lptr = lptr->next();
		lptr->GetRange(i, l);
		t_first_line = false;
	};
	if (fixedheight == 0)
		drect.height = lptr->getheight() - 2;
	else
		drect.height = fixedheight - 2;
	drect.x = lptr->GetCursorX(fi);
	
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

	drect.x += computelineoffset(lptr);

	drect.width = cursorwidth;

	return drect;
}

bool MCParagraph::copytextasstringref(MCStringRef& r_string)
{
	return MCStringCopy(m_text, r_string);
}

void MCParagraph::settext(MCStringRef p_string)
{
	deletelines();
	deleteblocks();
	
	MCValueRelease(m_text);
	/* UNCHECKED */ MCStringMutableCopy(p_string, m_text);
	
	blocks = new MCBlock;
	blocks->setparent(this);
	blocks->SetRange(0, MCStringGetLength(m_text));
}

void MCParagraph::resettext(MCStringRef p_string)
{
	MCValueRelease(m_text);
	/* UNCHECKED */ MCStringMutableCopy(p_string, m_text);
	findex_t i, l;
	if (blocks == NULL)
	{
		blocks = new MCBlock;
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
			aheight = MCU_max(aheight, lptr->getascent());
			dheight = MCU_max(dheight, lptr->getdescent());
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
	uint2 height = 0;

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
				height += lptr->getheight();
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
	if (endindex != PARAGRAPH_MAX_LEN)
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

void MCParagraph::indextoloc(findex_t tindex, uint2 fixedheight, int2 &x, int2 &y)
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
			y += lptr->getheight();
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
			y += lptr->getheight();
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

int2 MCParagraph::getx(findex_t tindex, MCLine *lptr)
{
	int2 x = lptr->GetCursorX(tindex);

	// MW-2012-01-08: [[ ParaStyles ]] Adjust the x start taking into account
	//   indents, list indents and alignment. (Paragraph to Field so +ve)
	x += computelineoffset(lptr);

	return x;
}

void MCParagraph::getxextents(findex_t &si, findex_t &ei, int2 &minx, int2 &maxx)
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
		int2 newx;
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

Boolean MCParagraph::extendup(MCBlock *bptr, findex_t &si)
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
	return found;
}

Boolean MCParagraph::extenddown(MCBlock *bptr, findex_t &ei)
{
	Boolean isgroup = True;
	Boolean found = False;
	
	// MW-2008-09-04: [[ Bug 7085 ]] Extending clicked links downwards should terminate
	//   when we get to a block with different linkText.
	MCStringRef t_link_text;
	t_link_text = bptr -> getlinktext();
	
	while (bptr == NULL || bptr->next() != blocks && isgroup)
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
	return found;
}

void MCParagraph::getclickindex(int2 x, int2 y,
                                   uint2 fixedheight, findex_t &si, findex_t &ei,
                                   Boolean wholeword, Boolean chunk)
{
	uint2 theight;
	if (fixedheight == 0)
		theight = lines->getheight();
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
			theight = lptr->getheight();
	};

	// MW-2012-01-08: [[ ParaStyles ]] Text finishes before spacing below.
	ty += computebottommargin();

	// MW-2012-01-08: [[ Paragraph Align ]] Adjust the x start taking into account
	//   indents, list indents and alignment. (Field to Paragraph so -ve)
	x -= computelineoffset(lptr);

	si = lptr->GetCursorIndex(x, chunk);
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
		ei = IncrementIndex(si);
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

		si = findwordbreakbefore(bptr, si);
		ei = si;
		bptr = indextoblock(ei, False);
		ei = findwordbreakafter(bptr, ei);

		bptr = indextoblock(ei, False);
		bptr -> AdvanceIndex(ei);
		
		return;
	}
}

// MW-2008-07-25: [[ Bug 6830 ]] Make sure we use the block retreat/advance
//   methods to navigate relatively to an index, otherwise Unicodiness isn't
//   taken into account.
static codepoint_t GetCodepointAtRelativeIndex(MCBlock *p_block, findex_t p_index, findex_t p_delta)
{
	while(p_block != NULL && p_delta < 0)
	{
		p_block = p_block -> RetreatIndex(p_index);
		p_delta += 1;
	}
	
	while(p_block != NULL && p_delta > 0)
	{
		p_block = p_block -> AdvanceIndex(p_index);
		p_delta -= 1;
	}
	
	if (p_block == NULL)
		return 0xFFFFFFFF;
	
	return p_block -> GetCodepointAtIndex(p_index);
}

findex_t MCParagraph::findwordbreakbefore(MCBlock *p_block, findex_t p_index)
{
	codepoint_t xc, x, y, yc;
	xc = GetCodepointAtRelativeIndex(p_block, p_index, -2);
	x = GetCodepointAtRelativeIndex(p_block, p_index, -1);
	y = GetCodepointAtRelativeIndex(p_block, p_index, 0);
	yc = GetCodepointAtRelativeIndex(p_block, p_index, 1);

	while(p_block != NULL)
	{
		if (MCUnicodeCanBreakWordBetween(xc, x, y, yc))
			return p_index;

		p_block = p_block -> RetreatIndex(p_index);

		yc = y;
		y = x;
		x = xc;
		xc = GetCodepointAtRelativeIndex(p_block, p_index, -2);
	}

	return p_index;
}

findex_t MCParagraph::findwordbreakafter(MCBlock *p_block, findex_t p_index)
{
	uint4 xc, x, y, yc;
	xc = GetCodepointAtRelativeIndex(p_block, p_index, -1);
	x = GetCodepointAtRelativeIndex(p_block, p_index, 0);
	y = GetCodepointAtRelativeIndex(p_block, p_index, 1);
	yc = GetCodepointAtRelativeIndex(p_block, p_index, 2);

	while(p_block != NULL)
	{
		if (MCUnicodeCanBreakWordBetween(xc, x, y, yc))
			return p_index;

		p_block = p_block -> AdvanceIndex(p_index);

		xc = x;
		x = y;
		y = yc;
		yc = GetCodepointAtRelativeIndex(p_block, p_index, 2);
	}

	return p_index;
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
// adjusted by the 'delta'.
void MCParagraph::getflaggedranges(uint32_t p_part_id, MCExecPoint& ep, findex_t si, findex_t ei, int32_t p_delta)
{
	// If the paragraph is empty, there is nothing to do.
	if (gettextlength() == 0)
		return;
	
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

	// Now loop through all the blocks until we reach the end.
	int32_t t_flagged_start, t_flagged_end;
	t_flagged_start = -1;
	t_flagged_end = -1;
	for(;;)
	{
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
				if (!ep . isempty())
					ep . appendnewline();
				
				// MW-2012-02-24: [[ FieldChars ]] Map the field indices back to char indices.
				int32_t t_start, t_end;
				t_start = t_flagged_start;
				t_end = t_flagged_end;
				parent -> unresolvechars(p_part_id, t_start, t_end);
				ep.appendstringf("%d,%d", t_start + p_delta + 1, t_end + p_delta);

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
}

// This method accumulates the ranges of the paragraph that have 'flagged' set
// to true. The output is placed in the uinteger_t array, with indices
// adjusted by the 'delta'.
void MCParagraph::getflaggedranges(uint32_t p_part_id, findex_t si, findex_t ei, int32_t p_delta, MCInterfaceFlaggedRanges& r_ranges)
{
	// If the paragraph is empty, there is nothing to do.
	if (gettextlength() == 0)
		return;
	
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
    
    MCAutoArray<MCInterfaceFlaggedRange> t_ranges;
    
	// Now loop through all the blocks until we reach the end.
	int32_t t_flagged_start, t_flagged_end;
	t_flagged_start = -1;
	t_flagged_end = -1;
	for(;;)
	{
        MCInterfaceFlaggedRange t_range;
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
				int32_t t_start, t_end;
				t_start = t_flagged_start;
				t_end = t_flagged_end;
				parent -> unresolvechars(p_part_id, t_start, t_end);
                t_range . start = t_start + p_delta + 1;
                t_range . end = t_end + p_delta;
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
	do
	{
		uint2 lheight = fixedheight == 0 ? lptr->getheight() : fixedheight;
		if (lheight > theight)
			return False;
		theight -= lheight;
		lptr = lptr->next();
	}
	while (lptr != lines);
	lptr = NULL;
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
