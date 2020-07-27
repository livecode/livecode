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
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"


#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "cdata.h"
#include "scrolbar.h"
#include "field.h"
#include "MCBlock.h"
#include "paragraf.h"
#include "mcerror.h"
#include "util.h"
#include "undolst.h"
#include "debug.h"
#include "mctheme.h"
#include "stacklst.h"
#include "dispatch.h"
#include "mode.h"
#include "globals.h"
#include "context.h"
#include "redraw.h"
#include "objectstream.h"

#include "exec.h"
#include "exec-interface.h"

#include "stackfileformat.h"

int2 MCField::clickx;
int2 MCField::clicky;
int2 MCField::goalx;

MCRectangle MCField::cursorrectp;
MCRectangle MCField::cursorrects;
Boolean MCField::cursoron = False;
MCField *MCField::cursorfield = NULL;

Boolean MCField::extend;
Boolean MCField::extendwords;
Boolean MCField::extendlines;
Boolean MCField::contiguous = True;
int2 MCField::widemargin = 14;
int2 MCField::narrowmargin = 8;
MCRectangle MCField::linkrect;
MCBlock *MCField::linkstart;
MCBlock *MCField::linkend;
findex_t MCField::linksi;
findex_t MCField::linkei;
findex_t MCField::composeoffset = 0;
findex_t MCField::composelength = 0;
findex_t MCField::composeconvertingsi = 0;
findex_t MCField::composeconvertingei = 0;
Boolean MCField::composing = False;
findex_t MCField::composecursorindex = 0;

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCField::kProperties[] =
{
	DEFINE_RW_OBJ_PROPERTY(P_AUTO_TAB, Bool, MCField, AutoTab)
	DEFINE_RW_OBJ_PROPERTY(P_DONT_SEARCH, Bool, MCField, DontSearch)
	DEFINE_RW_OBJ_PROPERTY(P_DONT_WRAP, Bool, MCField, DontWrap)
	DEFINE_RW_OBJ_PROPERTY(P_FIXED_HEIGHT, Bool, MCField, FixedHeight)
	DEFINE_RW_OBJ_PROPERTY(P_LOCK_TEXT, Bool, MCField, LockText)
	DEFINE_RW_OBJ_PROPERTY(P_SHARED_TEXT, Bool, MCField, SharedText)
	DEFINE_RW_OBJ_PROPERTY(P_SHOW_LINES, Bool, MCField, ShowLines)
	DEFINE_RW_OBJ_PROPERTY(P_HGRID, Bool, MCField, HGrid)
	DEFINE_RW_OBJ_PROPERTY(P_VGRID, Bool, MCField, VGrid)
	DEFINE_RW_OBJ_ENUM_PROPERTY(P_STYLE, InterfaceFieldStyle, MCField, Style)
	DEFINE_RW_OBJ_PROPERTY(P_AUTO_HILITE, Bool, MCField, AutoHilite)
	DEFINE_RW_OBJ_PROPERTY(P_AUTO_ARM, Bool, MCField, AutoArm)
	DEFINE_RW_OBJ_PROPERTY(P_FIRST_INDENT, Int16, MCField, FirstIndent)
	DEFINE_RW_OBJ_PROPERTY(P_WIDE_MARGINS, Bool, MCField, WideMargins)
	DEFINE_RW_OBJ_PROPERTY(P_HSCROLL, Int32, MCField, HScroll)
	DEFINE_RW_OBJ_PROPERTY(P_VSCROLL, Int32, MCField, VScroll)
	DEFINE_RW_OBJ_PROPERTY(P_HSCROLLBAR, Bool, MCField, HScrollbar)
	DEFINE_RW_OBJ_PROPERTY(P_VSCROLLBAR, Bool, MCField, VScrollbar)
	DEFINE_RW_OBJ_PROPERTY(P_SCROLLBAR_WIDTH, UInt16, MCField, ScrollbarWidth)
	DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_WIDTH, Int16, MCField, FormattedWidth)
	DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_HEIGHT, Int16, MCField, FormattedHeight)
	DEFINE_RW_OBJ_PROPERTY(P_LIST_BEHAVIOR, Bool, MCField, ListBehavior)
	DEFINE_RW_OBJ_PROPERTY(P_MULTIPLE_HILITES, Bool, MCField, MultipleHilites)
	DEFINE_RW_OBJ_PROPERTY(P_NONCONTIGUOUS_HILITES, Bool, MCField, NoncontiguousHilites)
    DEFINE_RW_OBJ_ENUM_PROPERTY(P_CURSORMOVEMENT, InterfaceFieldCursorMovement, MCField, CursorMovement)
    DEFINE_RW_OBJ_ENUM_PROPERTY(P_TEXTDIRECTION, InterfaceTextDirection, MCField, TextDirection)
	DEFINE_RW_OBJ_PART_PROPERTY(P_TEXT, String, MCField, Text)
	DEFINE_RW_OBJ_PART_PROPERTY(P_UNICODE_TEXT, BinaryString, MCField, UnicodeText)
	DEFINE_RW_OBJ_PART_NON_EFFECTIVE_PROPERTY(P_HTML_TEXT, Any, MCField, HtmlText)
	DEFINE_RO_OBJ_PART_EFFECTIVE_PROPERTY(P_HTML_TEXT, Any, MCField, HtmlText)
	DEFINE_RW_OBJ_PART_PROPERTY(P_RTF_TEXT, String, MCField, RtfText)
	DEFINE_RW_OBJ_PART_NON_EFFECTIVE_PROPERTY(P_STYLED_TEXT, Array, MCField, StyledText)
	DEFINE_RO_OBJ_PART_EFFECTIVE_PROPERTY(P_STYLED_TEXT, Array, MCField, StyledText)
	DEFINE_RO_OBJ_PART_NON_EFFECTIVE_PROPERTY(P_FORMATTED_STYLED_TEXT, Array, MCField, FormattedStyledText)
	DEFINE_RO_OBJ_PART_EFFECTIVE_PROPERTY(P_FORMATTED_STYLED_TEXT, Array, MCField, FormattedStyledText)
	DEFINE_RO_OBJ_PART_PROPERTY(P_PLAIN_TEXT, String, MCField, PlainText)
	DEFINE_RO_OBJ_PART_PROPERTY(P_UNICODE_PLAIN_TEXT, BinaryString, MCField, UnicodePlainText)
	DEFINE_RW_OBJ_PART_PROPERTY(P_FORMATTED_TEXT, String, MCField, FormattedText)
	DEFINE_RW_OBJ_PART_PROPERTY(P_UNICODE_FORMATTED_TEXT, BinaryString, MCField, UnicodeFormattedText)
	DEFINE_RW_OBJ_PROPERTY(P_LABEL, String, MCField, Label)
	DEFINE_RW_OBJ_PROPERTY(P_TOGGLE_HILITE, Bool, MCField, ToggleHilite)
	DEFINE_RW_OBJ_PROPERTY(P_3D_HILITE, Bool, MCField, ThreeDHilite)
	DEFINE_RO_OBJ_PART_ENUM_PROPERTY(P_ENCODING, InterfaceEncoding, MCField, Encoding)
	DEFINE_RW_OBJ_LIST_PROPERTY(P_HILITED_LINES, ItemsOfLooseUInt, MCField, HilitedLines)
    DEFINE_RW_OBJ_PART_CUSTOM_PROPERTY(P_FLAGGED_RANGES, InterfaceFieldRanges, MCField, FlaggedRanges)
    DEFINE_RW_OBJ_LIST_PROPERTY(P_TAB_STOPS, ItemsOfLooseUInt, MCField, TabStops)
    DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_TAB_ALIGN, InterfaceFieldTabAlignments, MCField, TabAlignments)
    DEFINE_RW_OBJ_LIST_PROPERTY(P_TAB_WIDTHS, ItemsOfLooseUInt, MCField, TabWidths)
    DEFINE_RO_OBJ_LIST_PROPERTY(P_PAGE_HEIGHTS, LinesOfLooseUInt, MCField, PageHeights)
    DEFINE_RO_OBJ_CUSTOM_PROPERTY(P_PAGE_RANGES, InterfaceFieldRanges, MCField, PageRanges)
    
    DEFINE_RW_OBJ_ENUM_PROPERTY(P_KEYBOARD_TYPE, InterfaceKeyboardType, MCField, KeyboardType)
    DEFINE_RW_OBJ_ENUM_PROPERTY(P_RETURN_KEY_TYPE, InterfaceReturnKeyType, MCField, ReturnKeyType)

    DEFINE_RW_OBJ_NON_EFFECTIVE_OPTIONAL_ENUM_PROPERTY(P_TEXT_ALIGN, InterfaceTextAlign, MCField, TextAlign)
	DEFINE_RO_OBJ_EFFECTIVE_ENUM_PROPERTY(P_TEXT_ALIGN, InterfaceTextAlign, MCField, TextAlign)
    
	DEFINE_RW_OBJ_CHAR_CHUNK_PROPERTY(P_TEXT, String, MCField, Text)
	DEFINE_RW_OBJ_CHAR_CHUNK_PROPERTY(P_UNICODE_TEXT, BinaryString, MCField, UnicodeText)
	DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(P_PLAIN_TEXT, String, MCField, PlainText)
	DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(P_UNICODE_PLAIN_TEXT, BinaryString, MCField, UnicodePlainText)
	DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(P_FORMATTED_TEXT, String, MCField, FormattedText)
	DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(P_UNICODE_FORMATTED_TEXT, BinaryString, MCField, UnicodeFormattedText)
	DEFINE_RW_OBJ_CHAR_CHUNK_PROPERTY(P_RTF_TEXT, String, MCField, RtfText)
	DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_PROPERTY(P_HTML_TEXT, Any, MCField, HtmlText)
	DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_PROPERTY(P_HTML_TEXT, Any, MCField, EffectiveHtmlText)
	DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_PROPERTY(P_STYLED_TEXT, Array, MCField, StyledText)
	DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_PROPERTY(P_STYLED_TEXT, Array, MCField, EffectiveStyledText)
	DEFINE_RO_OBJ_CHAR_CHUNK_NON_EFFECTIVE_PROPERTY(P_FORMATTED_STYLED_TEXT, Array, MCField, FormattedStyledText)
	DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_PROPERTY(P_FORMATTED_STYLED_TEXT, Array, MCField, EffectiveFormattedStyledText)

    // AL-2014-05-27: [[ Bug 12511 ]] charIndex is a char chunk property
	DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(P_CHAR_INDEX, UInt32, MCField, CharIndex)
	DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(P_LINE_INDEX, UInt32, MCField, LineIndex)
	DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(P_FORMATTED_TOP, Int32, MCField, FormattedTop)
	DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(P_FORMATTED_LEFT, Int32, MCField, FormattedLeft)
	DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(P_FORMATTED_WIDTH, Int32, MCField, FormattedWidth)
	DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(P_FORMATTED_HEIGHT, Int32, MCField, FormattedHeight)
	DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(P_FORMATTED_RECT, Rectangle32, MCField, FormattedRect)
	DEFINE_RW_OBJ_CHAR_CHUNK_PROPERTY(P_LINK_TEXT, String, MCField, LinkText)
	DEFINE_RW_OBJ_CHAR_CHUNK_PROPERTY(P_IMAGE_SOURCE, String, MCField, ImageSource)
	DEFINE_RW_OBJ_CHAR_CHUNK_PROPERTY(P_METADATA, String, MCField, Metadata)
	DEFINE_RW_OBJ_LINE_CHUNK_PROPERTY(P_METADATA, String, MCField, Metadata)
	// PM-2015-07-06: [[ Bug 15577 ]] "visited" property should be RW
	DEFINE_RW_OBJ_CHAR_CHUNK_PROPERTY(P_VISITED, Bool, MCField, Visited)
	DEFINE_RO_OBJ_CHAR_CHUNK_ENUM_PROPERTY(P_ENCODING, InterfaceEncoding, MCField, Encoding)
	DEFINE_RW_OBJ_CHAR_CHUNK_MIXED_PROPERTY(P_FLAGGED, Bool, MCField, Flagged)
	DEFINE_RW_OBJ_CHAR_CHUNK_CUSTOM_PROPERTY(P_FLAGGED_RANGES, InterfaceFieldRanges, MCField, FlaggedRanges)
	DEFINE_RW_OBJ_LINE_CHUNK_MIXED_ENUM_PROPERTY(P_LIST_STYLE, InterfaceListStyle, MCField, ListStyle)
	DEFINE_RW_OBJ_LINE_CHUNK_MIXED_PROPERTY(P_LIST_DEPTH, OptionalUInt16, MCField, ListDepth)
	DEFINE_RW_OBJ_LINE_CHUNK_MIXED_PROPERTY(P_LIST_INDENT, OptionalInt16, MCField, ListIndent)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_LIST_INDEX, OptionalUInt16, MCField, ListIndex)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_LIST_INDEX, UInt16, MCField, ListIndex)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_FIRST_INDENT, OptionalInt16, MCField, FirstIndent)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_FIRST_INDENT, Int16, MCField, FirstIndent)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_LEFT_INDENT, OptionalInt16, MCField, LeftIndent)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_LEFT_INDENT, Int16, MCField, LeftIndent)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_RIGHT_INDENT, OptionalInt16, MCField, RightIndent)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_RIGHT_INDENT, Int16, MCField, RightIndent)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_OPTIONAL_ENUM_PROPERTY(P_TEXT_ALIGN, InterfaceTextAlign, MCField, TextAlign)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_ENUM_PROPERTY(P_TEXT_ALIGN, InterfaceTextAlign, MCField, TextAlign)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_SPACE_ABOVE, OptionalUInt16, MCField, SpaceAbove)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_SPACE_ABOVE, UInt16, MCField, SpaceAbove)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_SPACE_BELOW, OptionalUInt16, MCField, SpaceBelow)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_SPACE_BELOW, UInt16, MCField, SpaceBelow)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_LIST_PROPERTY(P_TAB_STOPS, ItemsOfLooseUInt, MCField, TabStops)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_LIST_PROPERTY(P_TAB_STOPS, ItemsOfLooseUInt, MCField, TabStops)
    DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_TAB_ALIGN, InterfaceFieldTabAlignments, MCField, TabAlignments)
    DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_TAB_ALIGN, InterfaceFieldTabAlignments, MCField, TabAlignments)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_BORDER_WIDTH, OptionalUInt8, MCField, BorderWidth)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_BORDER_WIDTH, UInt8, MCField, BorderWidth)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_BACK_COLOR, InterfaceNamedColor, MCField, BackColor)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_BACK_COLOR, InterfaceNamedColor, MCField, BackColor)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_BORDER_COLOR, InterfaceNamedColor, MCField, BorderColor)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_BORDER_COLOR, InterfaceNamedColor, MCField, BorderColor)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_HGRID, OptionalBool, MCField, HGrid)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_HGRID, Bool, MCField, HGrid)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_VGRID, OptionalBool, MCField, VGrid)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_VGRID, Bool, MCField, VGrid)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_DONT_WRAP, OptionalBool, MCField, DontWrap)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_DONT_WRAP, Bool, MCField, DontWrap)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_PADDING, OptionalUInt8, MCField, Padding)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_PADDING, UInt8, MCField, Padding)
	DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_LIST_PROPERTY(P_TAB_WIDTHS, ItemsOfLooseUInt, MCField, TabWidths)
	DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_LIST_PROPERTY(P_TAB_WIDTHS, ItemsOfLooseUInt, MCField, TabWidths)
	DEFINE_RW_OBJ_LINE_CHUNK_MIXED_PROPERTY(P_INVISIBLE, Bool, MCField, Invisible)
    DEFINE_RW_OBJ_LINE_CHUNK_MIXED_PROPERTY(P_VISIBLE, Bool, MCField, Visible)
    
	DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_FORE_COLOR, InterfaceNamedColor, MCField, ForeColor)
    DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_FORE_COLOR, InterfaceNamedColor, MCField, ForeColor)
    DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_BACK_COLOR, InterfaceNamedColor, MCField, BackColor)
    DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_BACK_COLOR, InterfaceNamedColor, MCField, BackColor)
    DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_TEXT_FONT, OptionalString, MCField, TextFont)
    DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_TEXT_FONT, OptionalString, MCField, TextFont)
    DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_TEXT_SIZE, OptionalUInt16, MCField, TextSize)
    DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_TEXT_SIZE, UInt16, MCField, TextSize)
	DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_TEXT_STYLE, InterfaceTextStyle, MCField, TextStyle)
	DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_MIXED_CUSTOM_PROPERTY(P_TEXT_STYLE, InterfaceTextStyle, MCField, TextStyle)
	DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(P_TEXT_SHIFT, OptionalInt16, MCField, TextShift)
	DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_MIXED_PROPERTY(P_TEXT_SHIFT, Int16, MCField, TextShift)
    
    DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_MIXED_ARRAY_PROPERTY(P_TEXT_STYLE, OptionalBool, MCField, TextStyleElement)
	DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_MIXED_ARRAY_PROPERTY(P_TEXT_STYLE, Bool, MCField, TextStyleElement)
};

MCObjectPropertyTable MCField::kPropertyTable =
{
	&MCControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCField::MCField()
{
	flags |= F_FIXED_HEIGHT | F_TRAVERSAL_ON;
	fdata = oldfdata = scrolls = NULL;
	oldparagraphs = paragraphs = NULL;
	curparagraph = focusedparagraph = NULL;
	textx = texty = 0;
	textheight = textwidth = 0;
	fixeda = fixedd = fixedheight = 0;
	leftmargin = rightmargin = topmargin = bottommargin = narrowmargin;
	indent = 0;
	cury = focusedy = firsty = topmargin;
	firstparagraph = lastparagraph = NULL;
	foundlength = 0;
	vscrollbar = hscrollbar = NULL;
	scrollbarwidth = MCscrollbarwidth;
	tabs = NULL;
	ntabs = 0;
    alignments = NULL;
    nalignments = 0;
    cursor_movement = kMCFieldCursorMovementDefault;
    text_direction = kMCTextDirectionAuto;
	label = MCValueRetain(kMCEmptyString);
    
    // MM-2014-08-11: [[ Bug 13149 ]] Used to flag if a recompute is required during the next draw.
    m_recompute = false;
    
    keyboard_type = kMCInterfaceKeyboardTypeNone;
    return_key_type = kMCInterfaceReturnKeyTypeNone;
}

MCField::MCField(const MCField &fref) : MCControl(fref)
{
	fdata = oldfdata = scrolls = NULL;
	oldparagraphs = paragraphs = NULL;
	curparagraph = focusedparagraph = NULL;
	textx = texty = 0;
	textheight = textwidth = 0;
	fixeda = fixedd = fixedheight = 0;
	indent = fref.indent;
	cury = focusedy = firsty = topmargin;
	firstparagraph = lastparagraph = NULL;
	foundlength = 0;
    cursor_movement = fref.cursor_movement;
    text_direction= fref.text_direction;
    keyboard_type = fref.keyboard_type;
    return_key_type = fref.return_key_type;
    
	if (fref.vscrollbar != NULL)
	{
		vscrollbar = new (nothrow) MCScrollbar(*fref.vscrollbar);
		vscrollbar->setparent(this);
		vscrollbar->allowmessages(False);
		vscrollbar->setflag(flags & F_3D, F_3D);
		vscrollbar->setflag(flags & F_OPAQUE, F_OPAQUE);
		vscrollbar->setflag(flags & F_DISABLED, F_DISABLED);
		vscrollbar->setembedded();
	}
	else
		vscrollbar = NULL;
	if (fref.hscrollbar != NULL)
	{
		hscrollbar = new (nothrow) MCScrollbar(*fref.hscrollbar);
		hscrollbar->setparent(this);
		hscrollbar->allowmessages(False);
		hscrollbar->setflag(flags & F_3D, F_3D);
		hscrollbar->setflag(flags & F_OPAQUE, F_OPAQUE);
		hscrollbar->setflag(flags & F_DISABLED, F_DISABLED);
		hscrollbar->setembedded();
	}
	else
		hscrollbar = NULL;
	scrollbarwidth = fref.scrollbarwidth;
	ntabs = fref.ntabs;
	if (ntabs)
	{
		tabs = new (nothrow) uint2[ntabs];
		uint2 i;
		for (i = 0 ; i < ntabs ; i++)
			tabs[i] = fref.tabs[i];
	}
	else
		tabs = NULL;
    nalignments = fref.nalignments;
    if (nalignments)
    {
        /* UNCHECKED */ alignments = new (nothrow) intenum_t[nalignments];
        uint2 i;
        for (i = 0; i < nalignments; i++)
            alignments[i] = fref.alignments[i];
    }
    else
        alignments = NULL;
	
	if (fref.fdata != NULL)
	{
		MCCdata *fptr = fref.fdata;
		do
		{
			MCCdata *newfdata = new (nothrow) MCCdata(*fptr, this);
			newfdata->appendto(fdata);
			fptr = fptr->next();
		}
		while (fptr != fref.fdata);
	}
	MCValueRetain(fref.label);
	label = fref.label;
	state &= ~CS_KFOCUSED;
    
    // MM-2014-08-11: [[ Bug 13149 ]] Used to flag if a recompute is required during the next draw.
    m_recompute = false;
}

MCField::~MCField()
{
	// OK-2009-04-30: [[Bug 7517]] - Ensure the field is actually closed before deletion, otherwise dangling references may still exist,
	// particuarly if the field had image source characters.
	while (opened)
		close();
	
	if (opened && paragraphs != NULL)
	{
		closeparagraphs(paragraphs);
		fdata->setparagraphs(paragraphs);
	}
	while (fdata != NULL)
	{
		MCCdata *fptr = fdata->remove
		                (fdata);
		delete fptr;
	}
	while (scrolls != NULL)
	{
		MCCdata *fptr = scrolls->remove
		                (scrolls);
		fptr->setdata(0);
		delete fptr;
	}

	if (getstate(CS_FOREIGN_HSCROLLBAR))
		hscrollbar -> link(NULL);
	else
		delete hscrollbar;

	if (getstate(CS_FOREIGN_VSCROLLBAR))
		vscrollbar -> link(NULL);
	else
		delete vscrollbar;

	delete[] tabs; /* Allocated with new[] */
    
    MCMemoryDeallocate(alignments);

	MCValueRelease(label);
}

Chunk_term MCField::gettype() const
{
	return CT_FIELD;
}

const char *MCField::gettypestring()
{
	return MCfieldstring;
}

bool MCField::visit_self(MCObjectVisitor *p_visitor)
{
	return p_visitor -> OnField(this);
}

bool MCField::visit_children(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor *p_visitor)
{
	bool t_continue;
	t_continue = true;

	if (t_continue && fdata != NULL)
	{
		// Loop through all the field data attached to this field.
		MCCdata *t_fdata;
		t_fdata = fdata;

		do
		{
			// We include this field data in the following cases:
			//   1) it is shared and its id is 0
			//   2) the requested part is non-zero and the fdata's id matches it
			//   3) the requested part is zero.
			bool t_include;
			if (getflag(F_SHARED_TEXT))
				t_include = fdata -> getid() == 0;
			else if (p_part != 0)
				t_include = fdata -> getid() == p_part;
			else
				t_include = true;

			// If we are including this field data, then loop through its
			// paragraphs.
			if (t_include)
			{
				// MW-2012-11-22: [[ Bug 10558 ]] Make sure we visit the correct fdata!
				MCParagraph *pgptr = t_fdata -> getparagraphs();

				MCParagraph *tpgptr = pgptr;
				do
				{
					t_continue = tpgptr -> visit(p_options, p_part, p_visitor);
					tpgptr = tpgptr->next();
				}
				while(t_continue && tpgptr != pgptr);
			}

			t_fdata = t_fdata -> next();
		}
		while(t_fdata != fdata);
	}

	return t_continue;
}

void MCField::open()
{
	// MW-2008-03-14: [[ Bug 5750 ]] Fix to focus border problem in fields used as menu lists in
	//   (for example) option menus. Make sure the CS_MENUFIELD state persists.
	bool t_is_menu_field;
	t_is_menu_field = getstate(CS_MENUFIELD) == True;
	MCControl::open();
	if (t_is_menu_field)
		setstate(True, CS_MENUFIELD);

	if (hscrollbar != NULL && !getstate(CS_FOREIGN_HSCROLLBAR))
		hscrollbar->open();
	if (vscrollbar != NULL && !getstate(CS_FOREIGN_VSCROLLBAR))
		vscrollbar->open();
	if (opened > 1)
	{
		oldparagraphs = paragraphs;
		oldfdata = fdata;
		if (vscrollbar != NULL)
		{
			MCCdata *scrollptr = getcarddata(scrolls, fdata->getid(), False);
			if (scrollptr != NULL)
				scrollptr->setdata(texty);
			else
				if (texty != 0)
					getcarddata(scrolls, fdata->getid(), True)->setdata(texty);
		}
		if (this == MCfoundfield)
		{
			foundoffset = 0;
			foundlength = 0;
			MCfoundfield = nil;
		}
	}
	else
	{
		oldparagraphs = NULL;
		setsbrects();
	}
	paragraphs = NULL;

	uint4 parentid;
	if (flags & F_SHARED_TEXT)
		parentid = 0;
	else
		parentid = getcard()->getid();
	MCCdata *foundptr = getcarddata(fdata, parentid, True);
	paragraphs = foundptr->getparagraphs();
	foundptr->totop(fdata);
	if (paragraphs == oldparagraphs)
		oldparagraphs = NULL;
	else
	{
		openparagraphs();
		do_recompute(false);
		if (vscrollbar != NULL)
		{
			MCCdata *scrollptr = getcarddata(scrolls, parentid, False);
			if (scrollptr != NULL)
			{
				texty = scrollptr->getdata();
				cury = focusedy = topmargin - texty;
				resetscrollbars(True);
			}
		}
	}
}

void MCField::close()
{
	MCControl::close();
	if (oldparagraphs != NULL)
		MCundos->freeobject(this);
	if (opened)
	{
		if (oldparagraphs != NULL)
		{
			closeparagraphs(oldparagraphs);
			oldfdata->setparagraphs(oldparagraphs);
		}
	}
	else
	{
		// MW-2007-12-18: [[ Bug 5497 ]] If a field is not truly closed, then we
		//   shouldn't reset all the dependent pointers. Only when it is closed
		//   once and for all do we reset click and active field pointers.
		
		// MW-2005-12-01: [[Bug]] I-Beam cursor does not reset on field close
		
		if ((state & CS_IBEAM) != 0)
			getstack() -> clearibeam();
		if (MCclickfield == this)
			MCclickfield = nil;
		if (MCactivefield == this)
			MCactivefield = nil;
		if (MCfoundfield == this)
		{
			foundoffset = 0;
			foundlength = 0;
			MCfoundfield = nil;
		}
		if (paragraphs != NULL)
		{
			closeparagraphs(paragraphs);
			fdata->setparagraphs(paragraphs);
			if (vscrollbar != NULL)
			{
				MCCdata *scrollptr = getcarddata(scrolls, fdata->getid(), False);
				if (scrollptr != NULL)
					scrollptr->setdata(texty);
				else
					if (texty != 0)
						getcarddata(scrolls, fdata->getid(), True)->setdata(texty);
			}
		}
		curparagraph = focusedparagraph = NULL;
	}
	if (vscrollbar != NULL && vscrollbar->getopened() > opened)
		vscrollbar->close();
	if (hscrollbar != NULL && hscrollbar->getopened() > opened)
		hscrollbar->close();
}

void MCField::kfocus()
{
	if (opened && !(state & CS_KFOCUSED) && flags & F_TRAVERSAL_ON)
	{
		uint2 t_old_trans;
		t_old_trans = gettransient();
		state |= CS_KFOCUSED;
        
        if (MCactivefield && MCactivefield != this)
            MCactivefield->unselect(True, True);
        MCactivefield = this;
        clearfound();
        
		if (flags & F_LIST_BEHAVIOR)
		{
			if (!(flags & F_TOGGLE_HILITE))
				if ((!focusedparagraph->isselection()
				     && !focusedparagraph->IsEmpty())
				        || focusedparagraph->next() != focusedparagraph)
				{
					focusedparagraph->sethilite(True);
					updateparagraph(False, False);
				}
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
			//   possible change in transient.
			layer_transientchangedandredrawall(t_old_trans);
			replacecursor(False, False);
			message(MCM_focus_in);
		}
		else
		{
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
			//   possible change in transient.
			layer_transientchangedandredrawall(t_old_trans);
			
			MCscreen->addtimer(this, MCM_internal, MCblinkrate);
			if (!(state & CS_MFOCUSED) && flags & F_AUTO_TAB)
				seltext(0, getpgsize(paragraphs), True);
			if (flags & F_TRAVERSAL_ON && !(flags & F_LOCK_TEXT))
				message(MCM_open_field);
			else
				message(MCM_focus_in);
                
            // PM-2015-03-05: [[ Bug 14664 ]] Avoid flickering and draw cursor in the correct position after focussing the field
            replacecursor(False, False);
		}
		if (!(flags & F_LOCK_TEXT))
			MCModeActivateIme(getstack(), true);

		MCstacks -> ensureinputfocus(getstack() -> getwindow());
	}
}

void MCField::kunfocus()
{
	if (state & CS_KFOCUSED)
	{
		getstack() -> resetcursor(True);
	
		stopcomposition(False, True);
		MCModeActivateIme(getstack(), false);

		uint2 t_old_trans;
		t_old_trans = gettransient();

		state &= ~CS_KFOCUSED;

		if (flags & F_LIST_BEHAVIOR)
		{
			if (MCactivefield == this)
				unselect(True, True);
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
			//   possible change in transient.
			layer_transientchangedandredrawall(t_old_trans);
			removecursor();
			message(MCM_focus_out);
		}
		else
		{
			MCscreen->cancelmessageobject(this, MCM_internal);
			removecursor();
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
			//   possible change in transient.
			layer_transientchangedandredrawall(t_old_trans);
			if (flags & F_LOCK_TEXT)
				message(MCM_focus_out);
			else
				if (state & CS_CHANGED)
				{
					message(MCM_close_field);
					if (!(state & CS_KFOCUSED))
						state &= ~CS_CHANGED;
				}
				else
					message(MCM_exit_field);
			if (!(state & CS_KFOCUSED) && MCactivefield == this
			        && !focusedparagraph->isselection()
			        && firstparagraph == lastparagraph)
				MCactivefield = nil;
		}
	}
    
    // MM-2014-08-11: [[ Bug 13149 ]] Flag that a recompute is potentially required at the next draw.
    if (state & CS_SELECTED)
        m_recompute = false;
}

Boolean MCField::kdown(MCStringRef p_string, KeySym key)
{
	if (state & CS_NO_FILE)
		return False;
	if (key == XK_Return || key == XK_KP_Enter || key == XK_Tab)
	{
		MCAutoStringRef t_string;
		/* UNCHECKED */ MCStringFormat(&t_string, "%d", key);
		if (message_with_valueref_args(MCM_raw_key_down, *t_string) == ES_NORMAL)
			return True;
	}
	Exec_stat stat;
	switch (key)
	{
	case XK_Return:
		stat = message(MCM_return_in_field);
		if (stat == ES_NORMAL || stat == ES_ERROR)
			return True;
		break;
	case XK_KP_Enter:
		stat = message(MCM_enter_in_field);
		if (stat == ES_NORMAL || stat == ES_ERROR)
			return True;
		break;
	case XK_Tab:
		break;
	case XK_Up:
	case XK_Down:
	case XK_Left:
	case XK_Right:
		if (!MCtextarrows)
			return MCObject::kdown(p_string, key);
		// Fall through...
	default:
		if (MCObject::kdown(p_string, key))
			return True;
		break;
	}

	// MW-2008-01-11: [[ Bug 4360 ]] Hilite remains after removing focus in arrowKey handler.
	//   We must not only check that the focused paragraph is NULL, but also whether the
	//   activefield is us....
	// MW-2008-02-13: ... unless we are doing a mousewheely thing - in which case the currently
	//   non-active field can scroll
	// MW-2008-03-14: [[ Bug 5750 ]] ... or we are a menufield
	// MW-2008-03-18: [[ Bug 6156 ]] ... or a field with list behaviour set
	if (key != XK_WheelDown && key != XK_WheelUp && key != XK_WheelLeft && key != XK_WheelRight && (focusedparagraph == NULL || MCactivefield != this) && !getstate(CS_MENUFIELD) && !getflag(F_LIST_BEHAVIOR))
		return False;

	//windows likes to echo keydowns

#ifdef _MACOSX
	/*uint2 mods = (MCmodifierstate & MS_CONTROL) >> 1 & 0x3;
	Field_translations function;
	if (MCmodifierstate & MS_MAC_CONTROL)
	{
		mods |= MS_CONTROL >> 1;
		function = trans_lookup(emacs_keys, key, mods);
	}
	else
		function = trans_lookup(std_keys, key, mods);*/
	Field_translations function;
	function = lookup_mac_keybinding(key, MCmodifierstate);
	if (function == FT_UNDEFINED && MCmodifierstate & (MS_CONTROL | MS_MAC_CONTROL))
		return False;
#else

	uint2 mods = (MCmodifierstate >> 1) & 0x3;

	Field_translations function;
	if (key == XK_Insert && (MCmodifierstate & MS_SHIFT) != 0)
		function = FT_PASTE;
	else
		function = trans_lookup(MCemacskeys ? emacs_keys : std_keys, key, mods);
	if (function == FT_UNDEFINED && MCmodifierstate & (MS_CONTROL | MS_MOD1))
		return False;
	if (function != FT_UNDEFINED || !MCStringIsEmpty(p_string))
		stopcomposition(False, True);
#endif

	// MW-2008-02-26: [[ Bug 5939 ]] 'Escape' key inserts characters into fields
	if (key == XK_Escape)
		return False;
	
	switch (function)
	{
	case FT_UNDEFINED:
		if (!getflag(F_LOCK_TEXT))
		{
			if (MCStringIsEmpty(p_string))
				return False;
            
            // MW-2014-04-25: [[ Bug 5545 ]] This method will do the appropriate behavior
            //   based on platform.
            MCscreen -> hidecursoruntilmousemoves();

			finsertnew(function, p_string, key);
		}
		break;
	case FT_SELECTALL:
		seltext(0, getpgsize(nil), True);
		break;
	case FT_DELBCHAR:
    case FT_DELBSUBCHAR:
	case FT_DELBWORD:
	case FT_DELFCHAR:
	case FT_DELFWORD:
	case FT_DELBOL: // Not implemented yet
	case FT_DELEOL: // Not implemented yet
	case FT_DELBOP: // Not implemented yet
	case FT_DELEOP:
		if (!getflag(F_LOCK_TEXT))
			fdel(function, p_string, key);
		break;
	case FT_HELP:
		fhelp(function, p_string, key);
		break;
	case FT_UNDO:
		if (!getflag(F_LOCK_TEXT))
		fundo(function, p_string, key);
		break;
	case FT_CUT:
		if (!getflag(F_LOCK_TEXT))
			fcut(function, p_string, key);
		break;
	case FT_CUTLINE:
		if (!getflag(F_LOCK_TEXT))
			fcutline(function, p_string, key);
		break;
	case FT_COPY:
		if (!getflag(F_LOCK_TEXT))
			fcopy(function, p_string, key);
		break;
	case FT_PASTE:
		if (!getflag(F_LOCK_TEXT))
			fpaste(function, p_string, key);
		break;
	case FT_TAB:
		ftab(function, p_string, key);
		break;
	case FT_FOCUSFIRST:
	case FT_FOCUSLAST:
	case FT_FOCUSNEXT:
	case FT_FOCUSPREV:
		ffocus(function, p_string, key);
		break;
	case FT_PARAGRAPHAFTER:
		if (!getflag(F_LOCK_TEXT))
		{
			freturn(function, p_string, key);
			fmove(FT_LEFTCHAR, p_string, key);
		}
		break;
	case FT_PARAGRAPH:
		if (!getflag(F_LOCK_TEXT))
			freturn(function, p_string, key);
		break;
	case FT_CENTER:
		fcenter(function, p_string, key);
		break;
	case FT_SCROLLUP:
	case FT_SCROLLDOWN:
	case FT_SCROLLLEFT:
	case FT_SCROLLRIGHT:
	case FT_SCROLLPAGEUP:
	case FT_SCROLLPAGEDOWN:
	case FT_SCROLLTOP:
	case FT_SCROLLBOTTOM:
		fscroll(function, p_string, key);
		break;
	default:
		fmove(function, p_string, key);
		break;
	}
	return True;
}

Boolean MCField::mfocus(int2 x, int2 y)
{
	Tool tool = getstack()->gettool(this);
	if (!(flags & F_VISIBLE || showinvisible())
	    || (flags & F_DISABLED && tool == T_BROWSE) || state & CS_NO_FILE)
		return False;
	if (sbfocus(x, y, hscrollbar, vscrollbar))
	{
		if (state & CS_IBEAM)
		{
			state &= ~CS_IBEAM;
			getstack()->clearibeam();
		}
		return True;
	}
	if (flags & F_F_AUTO_ARM && flags & F_LOCK_TEXT && flags & F_LIST_BEHAVIOR)
	{
		if (x != mx || y != my)
			hiliteline(x, y);
		return MCControl::mfocus(x, y);
	}
	if (state & CS_SELECTING)
	{
		if (x != mx || y != my)
		{
			// MW-2012-01-25: [[ FieldMetrics ]] Co-ordinates are now card-based.
			extendselection(x, y);
			mx = x;
			my = y;
		}
		if (flags & F_LOCK_TEXT)
			return MCControl::mfocus(x, y);
		return True;
	}
	Boolean oldstate = True;
	if (linkstart != NULL)
		oldstate = MCU_point_in_rect(linkrect, mx, my);
	Boolean getfocus = MCControl::mfocus(x, y);
	if (getfocus && tool == T_BROWSE && hscrollbar != NULL && vscrollbar != NULL
	        && x > rect.x + rect.width - scrollbarwidth
	        && y > rect.y + rect.height - scrollbarwidth)
		getfocus = False;
	if (!(flags & F_LOCK_TEXT))
	{
		// Some control paths here do not explicitly set the cursor, so we take
		// note of this and reset the stack's cursor if nothing else does. This
		// is needed to ensure that a hidden cursor doesn't stick in some difficult
		// to locate circumstances...
		bool t_cursor_set;
		t_cursor_set = false;
		
		// MW-2008-03-18: [[ Bug 6106 ]] Cursor changes to i-beam in focus border and
		//   border of fields - fixed by making sure we ignore focus messages that
		//   land in the boder of the field.
		bool t_show_ibeam;
		if (MCU_point_in_rect(getfrect(), x, y))
			t_show_ibeam = true;
		else
			t_show_ibeam = false;
		
		if (getfocus && t_show_ibeam && !(state & CS_IBEAM) && tool == T_BROWSE)
		{
			state |= CS_IBEAM;
			getstack()->setibeam();
			t_cursor_set = true;
		}
		else if (!getfocus && !t_show_ibeam && state & CS_IBEAM)
		{
			state &= ~CS_IBEAM;
			getstack()->clearibeam();
			t_cursor_set = true;
		}
		
		if (state & CS_DRAG_TEXT)
			dragtext();
		else if (getfocus && MCactivefield == this && (focusedparagraph->isselection() || firstparagraph != lastparagraph))
		{
			computedrag();
			t_cursor_set = true;
		}

		if (!t_cursor_set)
			getstack() -> resetcursor(False);
	}
	else
		if (linkstart != NULL)
		{
			Boolean newstate = MCU_point_in_rect(linkrect, mx, my);
			if (newstate != oldstate)
			{
				MCBlock *bptr = linkstart;
				do
				{
					bptr->sethilite(newstate);
					bptr = bptr->next();
				}
				while (bptr != linkend->next());
				// MW-2011-08-18: [[ Layers ]] Invalidate the links part of the field.
				layer_redrawrect(linkrect);
			}
		}
	return getfocus;
}

void MCField::munfocus()
{
	if (state & CS_IBEAM)
		getstack()->clearibeam();
	state &= ~(CS_HSCROLL | CS_VSCROLL | CS_SELECTING | CS_IBEAM);
	extend = extendwords = False;
	linkstart = linkend = NULL;

	MCControl::munfocus();
}

void MCField::mdrag(void)
{
	// Don't act as a field drag if the cursor is within a scrollbar
	if (getstate(CS_HSCROLL) || getstate(CS_VSCROLL))
		return;

	// We can do an automatic (engine-based) drag from a field if the following
    // conditions are true:
    //  - the drag start message was not handled (or failed, or passed)
    //  - the field contains draggable text
    //
	bool t_auto_start;
	if (getflag(F_LOCK_TEXT) || getstate(CS_SOURCE_TEXT))
		t_auto_start = message(MCM_drag_start) != ES_NORMAL;
	else
		t_auto_start = false;

    // This field is the object being dragged
	MCdragtargetptr = this;

    // If we're not using engine-based dragging, there isn't anything more to
    // do for the moment.
	if (!t_auto_start)
		return;

    // If there is nothing to drag, there are no contents to put on the drag
    // board.
	if (!getstate(CS_SOURCE_TEXT))
		return;

    // Serialise the current selection and place it on the drag board.
	MCAutoDataRef t_data;
	pickleselection(&t_data);
	if (*t_data != nil)
	{
		MCallowabledragactions = DRAG_ACTION_MOVE | DRAG_ACTION_COPY;
		if (!MCdragboard->AddLiveCodeStyledText(*t_data))
        {
            // If we failed to put the data on the drag board, prohibit the drag
            MCallowabledragactions = 0;
        }
	}
}

Boolean MCField::mdown(uint2 which)
{
	if (state & CS_MFOCUSED)
		return False;
	if (state & CS_MENU_ATTACHED)
		return MCObject::mdown(which);
	state |= CS_MFOCUSED;
	state &= ~CS_SOURCE_TEXT;
	Tool tool = getstack()->gettool(this);
	if (tool == T_BROWSE
	        && sbdown(which, hscrollbar, vscrollbar))
		return True;
	switch (which)
	{
	case Button1:
		switch (tool)
		{
		case T_BROWSE:
		{
			// MW-2011-02-26: [[ Bug 9417 ]] When clicking on a link in a list we
			//   still do any autoHilite behavior.
			bool t_is_link_in_list;
			t_is_link_in_list = false;
			
			MCclickfield = this;
			clickx = mx;
			clicky = my;
			// MW-2011-02-26: [[ Bug 9417 ]] We now get a single char click loc
			//   which then gets extended to the full link in 'getlinkdata()'.
			if ((flags & F_LOCK_TEXT || MCmodifierstate & MS_CONTROL)
			        && locmark(False, False, False, True, True, linksi, linkei))
			{
				getlinkdata(linkrect, linkstart, linkend);
				if (linkstart != NULL)
				{
					MCBlock *bptr = linkstart;
					do
					{
						bptr->sethilite(True);
						bptr = bptr->next();
					}
					while (bptr != linkend->next());
					// MW-2011-08-18: [[ Layers ]] Invalidate the links part of the field.
					layer_redrawrect(linkrect);
					if (!getflag(F_LIST_BEHAVIOR))
					{
						message_with_valueref_args(MCM_mouse_down, MCSTR("1"));
						return True;
					}
					
					t_is_link_in_list = true;
				}
			}
			if (flags & F_TRAVERSAL_ON ||
			    ((flags & F_LOCK_TEXT || MCmodifierstate & MS_CONTROL) && flags & F_LIST_BEHAVIOR))
			{
				if (flags & F_TRAVERSAL_ON && !(state & CS_KFOCUSED)
				        && !(flags & F_NO_AUTO_HILITE))
				{
					// MW-2010-10-04: [[ Bug 9027 ]] Keep track of whether the field is still being
					//   'mouseDowned'. If there is no longer a mouse down in effect after the openField
					//    we don't do any of the selection stuff.
					state |= CS_MOUSEDOWN;
					getstack()->kfocusset(this);
					if (!(state & CS_KFOCUSED))
					{
						state &= ~CS_MFOCUSED;
						return False;
					}
					if (!(state & CS_MOUSEDOWN))
						return True;
				}
				if ((paragraphs != paragraphs->next() || !paragraphs->IsEmpty())
				        && !(flags & F_LIST_BEHAVIOR
				             && (my - rect.y > (int4)(textheight + topmargin - texty))))
				{
					// MW-2012-01-25: [[ FieldMetrics ]] Co-ordinates are now card-based.
					startselection(mx, my, False);
					if (flags & F_LOCK_TEXT || flags & F_LIST_BEHAVIOR)
						message_with_valueref_args(MCM_mouse_down, MCSTR("1"));
					if (t_is_link_in_list)
						endselection();
				}
				else
					if (flags & F_LOCK_TEXT || MCmodifierstate & MS_CONTROL)
						message_with_valueref_args(MCM_mouse_down, MCSTR("1"));
			}
			else
				message_with_valueref_args(MCM_mouse_down, MCSTR("1"));
		}
		break;
		case T_FIELD:
		case T_POINTER:
			if (MCactivefield == this)
				unselect(True, True);
			else
				if (flags & F_LIST_BEHAVIOR)
					sethilitedlines(NULL, 0);
			start(True);
			break;
		case T_HELP:
			break;
		default:
			return False;
		}
		break;
	case Button2:
		if (flags & F_LOCK_TEXT || getstack()->gettool(this) != T_BROWSE)
		{
			MCclickfield = this;
			clickx = mx;
			clicky = my;
			message_with_valueref_args(MCM_mouse_down, MCSTR("2"));
		}
		break;
	case Button3:
		MCclickfield = this;
		clickx = mx;
		clicky = my;
		message_with_valueref_args(MCM_mouse_down, MCSTR("3"));
		break;
	}
	return True;
}

Boolean MCField::mup(uint2 which, bool p_release)
{
	if (!(state & (CS_MFOCUSED | CS_DRAG_TEXT)))
		return False;
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which, p_release);
	state &= ~(CS_MFOCUSED | CS_MOUSEDOWN);
	if (state & CS_GRAB)
	{
		ungrab(which);
		return True;
	}
	Tool tool = getstack()->gettool(this);
	if (tool == T_BROWSE
	        && sbup(which, hscrollbar, vscrollbar))
	{
		if (flags & F_F_AUTO_ARM && flags & F_LOCK_TEXT
		        && flags & F_LIST_BEHAVIOR)
		{
			findex_t index = ytooffset(texty);
			findex_t ei = index;
			focusedparagraph = indextoparagraph(paragraphs, index, ei)->next();
			focusedy = paragraphtoy(focusedparagraph);
		}
		return True;
	}
	switch (which)
	{
	case Button1:
		switch (tool)
		{
		case T_BROWSE:
			if (state & CS_SOURCE_TEXT)
			{
				state &= ~CS_SOURCE_TEXT;

				// MW-2007-11-23: [[ Bug 5546 ]] Make sure we don't reset the activefield when unselecting
				//   (i.e. pass 'False' as the clear parameter to unselect)
				unselect(False, True);

				state |= CS_SELECTING;

				// MW-2012-01-25: [[ FieldMetrics ]] Co-ordinates are now card-based.
				setfocus(clickx, clicky);
			}
			if (state & CS_SELECTING)
			{
				endselection();
				MCundos->freestate();
				signallisteners(P_HILITED_LINES);
				message(MCM_selection_changed);
			}
			if (!(state & CS_DRAG_TEXT))
				if ((flags & F_LOCK_TEXT || MCmodifierstate & MS_CONTROL))
                {
					if (!p_release && MCU_point_in_rect(rect, mx, my))
                    {
                        if (flags & F_LIST_BEHAVIOR
                                && (my - rect.y > (int4)(textheight + topmargin - texty)
                                    || (paragraphs == paragraphs->next()
                                        && paragraphs->IsEmpty())))
                            message_with_valueref_args(MCM_mouse_release, MCSTR("1"));
						else
						{
							if (linkstart != NULL)
                            {
								if (linkstart->gethilite())
								{
									MCBlock *bptr = linkstart;
									do
									{
										bptr->sethilite(False);
										bptr->setvisited();
										bptr = bptr->next();
									}
									while (bptr != linkend->next());
									// MW-2011-08-18: [[ Layers ]] Invalidate the links part of the field.
									layer_redrawrect(linkrect);
                            
                                    MCAutoStringRef t_string;
									if (linkstart->getlinktext() == NULL)
                                    {
										returntext(linksi, linkei, &t_string);
                                    }
									else
										t_string = MCValueRetain(linkstart->getlinktext());
									linkstart = linkend = NULL;
									if (message_with_valueref_args(MCM_link_clicked, *t_string) == ES_NORMAL)
										return True;
								}
								else
									linkstart = linkend = NULL;
                            }
							
                            message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
						}
                    }
					else
						message_with_valueref_args(MCM_mouse_release, MCSTR("1"));
                }
			break;
		case T_FIELD:
		case T_POINTER:
			end(true, p_release);
			break;
		case T_HELP:
			help();
			break;
		default:
			return False;
		}
		break;
	case Button2:
		if (flags & F_LOCK_TEXT || getstack()->gettool(this) != T_BROWSE)
		{
			if (!p_release && MCU_point_in_rect(rect, mx, my))
				message_with_valueref_args(MCM_mouse_up, MCSTR("2"));
			else
				message_with_valueref_args(MCM_mouse_release, MCSTR("2"));
		}
		else if (MCscreen -> hasfeature(PLATFORM_FEATURE_TRANSIENT_SELECTION))
		{
            // Lock the selection
            MCselection->Lock();
            
            // Pasting styled text with middle-click isn't currently supported.
            // On the only platform where we support selection pasting (Linux),
            // a large number of apps use HTML to supply styled text but our
            // HTML parsing code doesn't really understand HTML so trying to
            // paste styled text here just causes a mess.
            //
            // Strangely, this doesn't seem to affect normal pasting as the
            // affected apps also supply RTF there, which we can parse fairly
            // well.
            if (MCselection->HasText())
            {
                MCAutoStringRef t_text;
                if (MCselection->CopyAsText(&t_text))
                {
                    // Set the focus position based on the current mouse
                    // position and enter the text as if it came from the
                    // keyboard.
                    extend = extendwords = False;
                    setfocus(mx, my);
                    typetext(*t_text);
                }
            }
            
            // Unlock the selection
            MCselection->Unlock();
		}
		break;
	case Button3:
		if (!p_release && MCU_point_in_rect(rect, mx, my))
			message_with_valueref_args(MCM_mouse_up, MCSTR("3"));
		else
			message_with_valueref_args(MCM_mouse_release, MCSTR("3"));
		break;
	}
	return True;
}

// MW-2011-01-31: [[ Bug 9284 ]] Given you can click-drag-select and triple-click-select
//   in locked fields, you should probably be able to double-click select also!
// MW-2011-02-04: [[ Bug 9377 ]] Reverting 9284 - needs further investigation before
//   fixing.
Boolean MCField::doubledown(uint2 which)
{
	if (which == Button1 && getstack()->gettool(this) == T_BROWSE)
	{
		if (sbdoubledown(which, hscrollbar, vscrollbar))
			return True;
		MCclickfield = this;
		// MW-2012-10-02: [[ Bug 10290 ]] Only start a selection in a traversalOn field
		//   if it is the active field. This stops oddness when double-click gets interleaved
		//   with tab.
		if (flags & F_LIST_BEHAVIOR && (flags & F_TOGGLE_HILITE))
			mdown(which);
		else if (!(flags & F_LOCK_TEXT || MCmodifierstate & MS_CONTROL) &&
				 (!getflag(F_TRAVERSAL_ON) || MCactivefield == this))
		{
			state |= CS_MFOCUSED;
			// MW-2012-01-25: [[ FieldMetrics ]] Co-ordinates are now card-based.
			startselection(mx, my, True);
			return True;
		}
	}
	return MCControl::doubledown(which);
}

Boolean MCField::doubleup(uint2 which)
{
	if (which == Button1 && getstack()->gettool(this) == T_BROWSE)
	{
		if (state & CS_SELECTING)
		{
			state &= ~CS_MFOCUSED;
			endselection();
			MCundos->freestate();
			signallisteners(P_HILITED_LINES);
			message(MCM_selection_changed);
			return True;
		}
		if (state & CS_DRAG_TEXT)
		{
			removecursor();
			state &= ~(CS_DRAG_TEXT | CS_MFOCUSED);
			getstack()->resetcursor(True);
		}
		if (sbdoubleup(which, hscrollbar, vscrollbar))
			return True;
		if (flags & F_LIST_BEHAVIOR && (flags & F_TOGGLE_HILITE))
			mup(which, false);
	}
	return MCControl::doubleup(which);
}

void MCField::timer(MCNameRef mptr, MCParameter *params)
{
	if (MCNameIsEqualToCaseless(mptr, MCM_internal))
	{
		if (opened && (state & CS_KFOCUSED)
		        && !(state & CS_DRAG_TEXT))
		{
			if (cursoron)
				removecursor();
			else
				replacecursor(False, False);
			MCscreen->addtimer(this, MCM_internal, MCblinkrate);
		}
	}
	else if (MCNameIsEqualToCaseless(mptr, MCM_internal2))
		{
			if (opened)
			{
				if (state & CS_SELECTING)
				{
					// MW-2012-01-25: [[ FieldMetrics ]] Co-ordinates are now card-based.
					if (!MCU_point_in_rect(getfrect(), mx, my))
						extendselection(mx, my);
					MCscreen->addtimer(this, MCM_internal2, MCsyncrate);
				}
				else
				{
					if (state & CS_DRAG_TEXT)
					{
						if (!MCU_point_in_rect(getfrect(), mx, my))
							dragtext();
						MCscreen->addtimer(this, MCM_internal2, MCsyncrate);
					}
				}
			}
		}
		else
			MCControl::timer(mptr, params);
}

void MCField::select()
{
	if (MCactivefield == this)
		unselect(True, True);
	MCControl::select();
}

uint2 MCField::gettransient() const
{
	if (state & CS_KFOCUSED && MClook != LF_WIN95
	        && borderwidth != 0 && !(extraflags & EF_NO_FOCUS_BORDER))
		return MCfocuswidth;
	return 0;
}

void MCField::applyrect(const MCRectangle &nrect)
{
	// The contents only need to be laid out if the size changes. In particular,
    // it is the width that is important; the height does not affect layout.
    bool t_resized = false;
    if (nrect.width != rect.width)
        t_resized = true;
    
    rect = nrect;
	setsbrects();

	// MW-2007-07-05: [[ Bug 2435 ]] - 'Caret' doesn't move with the field when its rect changes
	if (cursoron)
		replacecursor(False, False);
    
    if (t_resized)
        do_recompute(true);
    
    // MM-2014-08-11: [[ Bug 13149 ]] Flag that a recompute is potentially required at the next draw.
    if (state & CS_SIZE)
        m_recompute = true;
}

// MW-2012-01-25: [[ ParaStyles ]] Parse the given string as a list of tab-stops.
// MW-2012-02-11: [[ TabWidths ]] The 'which' parameter determines what style of tabStops to
//   parse - widths or stops.
bool MCField::parsetabstops(Properties which, MCStringRef data, uint16_t*& r_tabs, uint16_t& r_tab_count)
{
	uint2 *newtabs;
	uint2 newntabs;
	newtabs = nil;
	newntabs = 0;

	uint4 l = MCStringGetLength(data);
    MCAutoPointer<char> t_data;
    /* UNCHECKED */ MCStringConvertToCString(data, &t_data);
	const char *sptr = *t_data;
	while (l)
	{
		int32_t i1;
		Boolean done;
		i1 = MCU_strtol(sptr, l, ',', done);
		// MW-2012-02-10: [[ HiddenTabs ]] Allow a zero-width tab, now used to indicate
		//   invisibility of the column in vGrid mode.
		if (!done || i1 < 0)
		{
			MCeerror->add(EE_FIELD_TABSNAN, 0, 0, data);
			delete newtabs;
			return false;
		}
		MCU_realloc((char **)&newtabs, newntabs, newntabs + 1, sizeof(uint2));
		// MW-2012-02-11: [[ TabWidths ]] Assume all entries are relative if in tabWidths
		//   mode.
		// MW-2013-01-15: [[ Bug 10614 ]] If the previous tab is the same as the current one
		//   then make sure we record a zero width.
		if (newntabs > 0 && (which == P_TAB_WIDTHS || i1 < (int2)newtabs[newntabs - 1]))
			i1 += (int2)newtabs[newntabs - 1];
		newtabs[newntabs++] = i1;
	}

	r_tabs = newtabs;
	r_tab_count = newntabs;

	return true;
}

// MW-2012-02-11: [[ TabWidths ]] This method formats the tabStops in either stops or
//   widths style depending on the value of which.
void MCField::formattabstops(Properties which, uint16_t *tabs, uint16_t tab_count, MCStringRef &r_result)
{
	if (r_result != nil)
        MCValueRelease(r_result);
    MCAutoListRef t_list;
    /* UNCHECKED */ MCListCreateMutable(',', &t_list);
	if (which == P_TAB_STOPS)
	{
		for(uint32_t i = 0 ; i < tab_count ; i++)
            MCListAppendInteger(*t_list, tabs[i]);
	}
	else
	{
		int32_t t_previous_tab;
		t_previous_tab = 0;
		for(uint32_t i = 0; i < tab_count; i++)
		{
            MCListAppendInteger(*t_list, tabs[i]- t_previous_tab);
			t_previous_tab = tabs[i];
		}
	}
    /* UNCHECKED */ MCListCopyAsString(*t_list, r_result);
}

bool MCField::parsetabalignments(MCStringRef p_data, intenum_t *&r_alignments, uindex_t &r_alignment_count)
{
    MCAutoProperListRef t_list;
    if (!MCStringSplitByDelimiter(p_data, MCSTR(","), kMCStringOptionCompareExact, &t_list))
		return false;

    uindex_t t_count;
	t_count = MCProperListGetLength(*t_list);
    
    MCAutoArray<intenum_t> t_alignments;
	if (!t_alignments.Extend(t_count))
		return false;
    
    for (uindex_t i = 0; i < t_count; i++)
    {
        MCValueRef t_item;
		t_item = MCProperListFetchElementAtIndex(*t_list, i);

        if (MCStringIsEqualToCString((MCStringRef)t_item, "left", kMCStringOptionCompareCaseless))
        {
            t_alignments[i] = kMCParagraphTextAlignLeft;
        }
        else if (MCStringIsEqualToCString((MCStringRef)t_item, "right", kMCStringOptionCompareCaseless))
        {
            t_alignments[i] = kMCParagraphTextAlignRight;
        }
        else if (MCStringIsEqualToCString((MCStringRef)t_item, "center", kMCStringOptionCompareCaseless))
        {
            t_alignments[i] = kMCParagraphTextAlignCenter;
        }
        else
        {
            return false;
        }
    }
    
    t_alignments . Take(r_alignments, r_alignment_count);
	return true;
}

bool MCField::formattabalignments(const intenum_t *p_alignments, uindex_t p_alignment_count, MCStringRef &r_result)
{
    if (p_alignment_count == 0)
    {
        r_result = MCValueRetain(kMCEmptyString);
        return true;
    }
    
    MCAutoListRef t_list;
    if (!MCListCreateMutable(',', &t_list))
		return false;
    
    for (uindex_t i = 0; i < p_alignment_count; i++)
    {
        switch (p_alignments[i])
        {
            case kMCParagraphTextAlignLeft:
                if (!MCListAppendCString(*t_list, "left"))
					return false;
                break;
                
            case kMCParagraphTextAlignRight:
                if (!MCListAppendCString(*t_list, "right"))
					return false;
                break;
                
            case kMCParagraphTextAlignCenter:
                if (!MCListAppendCString(*t_list, "center"))
					return false;
                break;
                
            case kMCParagraphTextAlignJustify:
                if (!MCListAppendCString(*t_list, "justify"))
					return false;
                break;
                
            default:
				MCUnreachableReturn(false);
        }
    }
    
    return MCListCopyAsString(*t_list, r_result);
}

void MCField::undo(Ustruct *us)
{
	findex_t si, ei;
	MCParagraph *pgptr = NULL;

	// MW-2005-02-20: Fix Bug 2537 by assuming if the field is closed then
	//   the undo record is no longer valid. In the future, we need to
	//   ensure the undo is ignored if the contents of the field has changed
	//   through some other means (not by user interaction).
	if (!opened)
		return;
	
	// MW-2012-02-27: [[ Bug ]] Lock the screen before the undo operation so
	//   we can issue textchanged.
	MCRedrawLockScreen();
	
	switch (us->type)
	{
	case UT_TYPE_TEXT:
		if (!(state & CS_KFOCUSED))
			getstack()->kfocusset(this);

		// MW-UNDO-FIX: Use the start and end indices that were used when the
		//   record was created.
		si = us->ud.text.index;
		ei = us->ud.text.index+us->ud.text.newchars;

		seltext(ei - us->ud.text.newchars, ei, False);
		pgptr = cloneselection();
        settextindex(0, ei - us->ud.text.newchars, ei, kMCEmptyString, True);
		ei -= us->ud.text.newchars;
		us->ud.text.newchars = 0;
		if (us->ud.text.data != NULL)
		{
			MCParagraph *tpgptr = us->ud.text.data;
			do
			{
				us->ud.text.newchars += tpgptr->gettextlengthcr();
				tpgptr = tpgptr->next();
			}
			while (tpgptr != us->ud.text.data);
			if (us->ud.text.newchars)
				us->ud.text.newchars--;
			insertparagraph(us->ud.text.data);
			tpgptr = us->ud.text.data;
			while (tpgptr != NULL)
			{
				MCParagraph *tipgptr = tpgptr->remove
				                       (tpgptr);
				delete tipgptr;
			}
			seltext(ei, ei + us->ud.text.newchars, False);
		}
		us->ud.text.data = pgptr;
		updateparagraph(True, True);
		break;
	case UT_MOVE_TEXT:
	{
		if (!(state & CS_KFOCUSED))
			getstack()->kfocusset(this);

		// index is valid
		// old_index is valid after [index..index + newchars) has been deleted

		si = us -> ud . text . index;
		ei = si + us -> ud . text . newchars;
		seltext(si, ei, False);
		pgptr = cloneselection();
		settextindex(0, si, ei, kMCEmptyString, True);

		us->ud.text.newchars = 0;
		if (us -> ud . text . data != NULL)
		{
			MCParagraph *tpgptr = us->ud.text.data;
			do
			{
				us->ud.text.newchars += tpgptr->gettextlengthcr();
				tpgptr = tpgptr->next();
			}
			while (tpgptr != us->ud.text.data);
			if (us->ud.text.newchars)
				us->ud.text.newchars--;

			seltext(us->ud.text.old_index, us->ud.text.old_index, True);
			insertparagraph(us->ud.text.data);

			tpgptr = us->ud.text.data;
			while (tpgptr != NULL)
			{
				MCParagraph *tipgptr = tpgptr->remove(tpgptr);
				delete tipgptr;
			}
			seltext(us->ud.text.old_index, us->ud.text.old_index + us->ud.text.newchars, False);
		}

		int4 t_old_index;
		t_old_index = us->ud.text.old_index;
		us->ud.text.data = pgptr;
		us->ud.text.old_index = us->ud.text.index;
		us->ud.text.index = t_old_index;
		updateparagraph(True, True);
	}
	break;
	case UT_DELETE_TEXT:
	case UT_REPLACE_TEXT:
		if (!(state & CS_KFOCUSED))
			getstack()->kfocusset(this);
		si = us->ud.text.index;
		ei = si;
		if (us->type == UT_DELETE_TEXT)
		{
			pgptr = indextoparagraph(paragraphs, si, ei);
			pgptr->setselectionindex(si, si, False, False);
			focusedparagraph = pgptr;
			if (us->ud.text.data != NULL)
			{
				insertparagraph(us->ud.text.data);
				selectedmark(False, si, ei, False);
				seltext(us->ud.text.index, si, True);
				us->type = UT_REPLACE_TEXT;
			}
			else
				if (us->ud.text.newline)
				{
					pgptr->split();
					pgptr->setselectionindex(PARAGRAPH_MAX_LEN, PARAGRAPH_MAX_LEN, False, False);
					pgptr = pgptr->next();
					pgptr->setselectionindex(0, 0, False, False);
					flags &= ~F_VISIBLE;
					do_recompute(true);
					focusedparagraph = pgptr;
					updateparagraph(True, False);
					flags |= F_VISIBLE;
					seltext(us->ud.text.index, us->ud.text.index + 1, True);
					state |= CS_CHANGED;
				}
		}
		else
		{
			pgptr = us->ud.text.data;
			do
			{
				ei += pgptr->gettextlengthcr();
				pgptr = pgptr->next();
			}
			while (pgptr != us->ud.text.data);
			settextindex(0, si, ei - 1, kMCEmptyString, True);
			us->type = UT_DELETE_TEXT;
		}
		updateparagraph(True, True);
		break;
	default:
		MCControl::undo(us);
		break;
	}
	
	// MW-2012-02-27: [[ Bug ]]  Unlock the screen before issuing the textChanged message. 
	MCRedrawUnlockScreen();
	
	// MW-2012-02-27: [[ Bug ]] Make sure we invoke textChanged.
	textchanged();
}

MCControl *MCField::clone(Boolean attach, Object_pos p, bool invisible)
{
	if (opened && fdata != NULL)
		fdata->setparagraphs(paragraphs);
	MCField *newfield = new (nothrow) MCField(*this);
	if (attach)
		newfield->attach(p, invisible);
	return newfield;
}

void MCField::getwidgetthemeinfo(MCWidgetInfo &widgetinfo)
{
	uint4 wstate = 0;
	MCControl::getwidgetthemeinfo(widgetinfo);
	if (state & CS_HILITED)
		wstate |= WTHEME_STATE_HILITED;
	if (!(state & CS_SELECTED) && MCU_point_in_rect(rect, mx, my))
		wstate |= WTHEME_STATE_HOVER;
	// MW-2012-10-02: [[ Bug 10287 ]] Only show the focus border if focused and
	//   showFocusBorder is true.
	if (state & CS_KFOCUSED && !(extraflags & EF_NO_FOCUS_BORDER)) //has keyboard focus
		wstate |= WTHEME_STATE_HASFOCUS;
	if (flags & F_LOCK_TEXT)
		wstate |= WTHEME_STATE_READONLY;
	widgetinfo.state |= wstate;
}

MCParagraph *MCField::resolveparagraphs(uint4 parid)
{
	MCParagraph *pgptr = NULL;

	if (flags & F_SHARED_TEXT)
		parid = 0;
		
	if (opened && parid == 0)
		pgptr = paragraphs;
	else
	{
		if (parid == 0 && !(flags & F_SHARED_TEXT))
			parid = getcard()->getid();
		pgptr = getcarddata(fdata, parid, True)->getparagraphs();
	}
	
	return pgptr;
}

MCCdata *MCField::getdata(uint4 cardid, Boolean clone)
{
	if (fdata != NULL)
	{
		if (flags & F_SHARED_TEXT)
			cardid = 0;
		MCCdata *fptr = fdata;
		do
		{
			if (fptr->getid() == cardid)
			{
				if (clone)
					return new MCCdata(*fptr);
				if (cardid == 0)
					break;
				if (opened && fptr == fdata)
					paragraphs = curparagraph = NULL;
				return fptr->remove
				       (fdata);
			}
			fptr = fptr->next();
		}
		while (fptr != fdata);
	}
	MCCdata *newptr = new (nothrow) MCCdata(cardid);
	return newptr;
}

void MCField::replacedata(MCCdata *&data, uint4 newid)
{
	if (data == NULL)
		return;
	if (flags & F_SHARED_TEXT)
	{
		if (data->getid() == 0)
		{
			MCCdata *fptr = data->remove
			                (data);
			delete fptr;
			return;
		}
		newid = 0;
	}
	if (opened)
	{
		closeparagraphs(paragraphs);
		curparagraph = focusedparagraph = NULL;
	}
	MCCdata *foundptr = getcarddata(fdata, newid, True);
	foundptr->remove
	(fdata);
	delete foundptr;
	MCCdata *fptr = data->remove
	                (data);
	fptr->setid(newid);
	fptr->insertto(fdata);
	if (opened)
	{
		paragraphs = fdata->getparagraphs();
		openparagraphs();
		do_recompute(true);

		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
}

void MCField::compactdata()
{
	if (fdata != NULL)
	{
		MCStack *sptr = getstack();
		MCCdata *fptr = fdata;
		Boolean check;
		MCObject *tparent = this;
		while (tparent->getparent()->gettype() == CT_GROUP)
			tparent = tparent->getparent();
		uint4 tid = tparent->getid();
		do
		{
			check = False;
			MCCdata *nfptr = fptr->next();
			uint4 cid = fptr->getid();
			if (cid != 0 && (flags & F_SHARED_TEXT || !sptr->checkid(cid, tid)))
			{
				fptr->remove(fdata);
				delete fptr;
				if (fdata == NULL)
					break;
				check = True;
			}
			fptr = nfptr;
		}
		while (check || fptr != fdata);
	}
}

void MCField::resetfontindex(MCStack *oldstack)
{
	if (fdata != NULL)
	{
		MCCdata *fptr = fdata;
		MCCdata *tptr = fptr->remove(fptr);
		
		MCParagraph *pgptr = tptr->getparagraphs();
		MCParagraph *tpgptr = pgptr;
		do
		{
			tpgptr->setparent(this);
			tpgptr = tpgptr->next();
		}
		while (tpgptr != pgptr);
		
		if (!(flags & F_SHARED_TEXT))
			tptr->setid(getcard()->getid());

		fdata = tptr;
		while (fptr != NULL)
		{
			tptr = fptr->remove(fptr);
			delete tptr;
		}
	}
	MCControl::resetfontindex(oldstack);
	if (opened)
	{
		resetparagraphs();
		do_recompute(true);
	}
}

Exec_stat MCField::hscroll(int4 offset, Boolean doredraw)
{
	int4 oldx = textx;
	textx += offset;
	MCRectangle drect = getfrect();
	if (!borderwidth)
	{
		drect.width--;
		drect.height--;
	}
	else
		if (borderwidth > 1 && !(flags & F_SHOW_BORDER))
		{
			drect.x -= DEFAULT_BORDER;
			drect.width +=  flags & F_VSCROLLBAR ? DEFAULT_BORDER
			                : DEFAULT_BORDER * 2;
			drect.y -= DEFAULT_BORDER;
			drect.height += flags & F_HSCROLLBAR ? DEFAULT_BORDER
			                : DEFAULT_BORDER * 2;
		}
	uint4 twidth = textwidth + leftmargin + rightmargin - DEFAULT_BORDER * 2;
	if (textx < 0 || twidth < (uint4)drect.width)
		textx = 0;
	else
		if ((uint4)textx > twidth - drect.width)
			textx = twidth - drect.width;
	offset = textx - oldx;
	if (offset == 0)
		return ES_NORMAL;

	// MW-2006-03-21: Bug 3344/3377 - Fix the focus border lingering
	if (cursorfield == this)
		replacecursor(False, False);

	if (opened && doredraw)
	{
		// MW-2011-08-18: [[ Layers ]] Redraw the content area.
		layer_redrawrect(drect);
	}

	signallisteners(P_HSCROLL);
	
	return message_with_args(MCM_scrollbar_drag, textx);
}

Exec_stat MCField::vscroll(int4 offset, Boolean doredraw)
{
	int4 oldy = texty;
	texty += offset;
	MCRectangle drect = getfrect();
	if (!borderwidth)
	{
		drect.width--;
		drect.height--;
	}
	else
		if (borderwidth > 1 && !(flags & F_SHOW_BORDER))
		{
			drect.x -= DEFAULT_BORDER;
			drect.width += flags & F_VSCROLLBAR ? DEFAULT_BORDER : DEFAULT_BORDER * 2;
			drect.y -= DEFAULT_BORDER;
			drect.height += flags & F_HSCROLLBAR ? DEFAULT_BORDER : DEFAULT_BORDER * 2;
			if (state & CS_KFOCUSED && !(extraflags & EF_NO_FOCUS_BORDER)
			    && (IsMacEmulatedLF() || (IsMacLFAM() && !MCaqua)))
				drect = MCU_reduce_rect(drect, 1);
		}
	//to-do change for drawing xp text fields
	if (IsMacLFAM() && MCaqua && borderwidth == DEFAULT_BORDER)
	{
		drect.y++;
		drect.height--;
	}
	uint4 theight = textheight + topmargin + bottommargin - TEXT_Y_OFFSET * 2;
	if (texty < 0 || theight < (uint4)drect.height)
		texty = 0;
	else
		if ((uint4)texty > theight - drect.height)
			texty = theight - drect.height;
	offset = texty - oldy;
	if (offset == 0)
		return ES_NORMAL;
	focusedy -= offset;
	cury -= offset;
	firsty -= offset;
	
	// MW-2006-03-21: Bug 3344/3377 - Fix the focus border lingering
	if (cursorfield == this)
		replacecursor(False, False);

	if (opened && doredraw)
	{
		// MW-2011-08-18: [[ Layers ]] Redraw the content area.
		layer_redrawrect(drect);
	}
	
	signallisteners(P_VSCROLL);
	
	return message_with_args(MCM_scrollbar_drag, texty);
}

void MCField::readscrollbars()
{
	real8 pos;
	removecursor();
	if (flags & F_HSCROLLBAR || getstate(CS_FOREIGN_HSCROLLBAR))
	{
		hscrollbar->getthumb(pos);
		hscroll((int4)pos - textx, True);
	}
	if (flags & F_VSCROLLBAR || getstate(CS_FOREIGN_VSCROLLBAR))
	{
		vscrollbar->getthumb(pos);
		vscroll((int4)pos - texty, True);
	}
	replacecursor(False, True);
}

void MCField::resetscrollbars(Boolean move)
{
	if (!(flags & F_SCROLLBAR) && !getstate(CS_FOREIGN_SCROLLBAR))
		return;
	real8 pos, size;
	real8 linc = 0.0;
	MCRectangle textrect = getfrect();

	uint2 fheight;
	fheight = gettextheight();

	if (flags & F_HSCROLLBAR || getstate(CS_FOREIGN_HSCROLLBAR))
	{
		hscroll(0, True);
		
		// MW-2008-06-12: This can result in a negative thumbsize - this is
		// probably not good, so we clamp.
		real8 tw = MCU_fmax(0.0, textwidth + leftmargin + rightmargin - DEFAULT_BORDER * 4);
		if (tw < textrect.width)
		{
			pos = 0.0;
			size = tw;
		}
		else
		{
			pos = textx;
			size = textrect.width;
			linc = fheight;
		}
		if (move)
			hscrollbar->movethumb(pos);
		else
			hscrollbar->setthumb(pos, size, linc, tw);

		if (getstate(CS_FOREIGN_HSCROLLBAR))
		{
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole scrollbar.
			hscrollbar -> layer_redrawall();
		}
	}
	if (flags & F_VSCROLLBAR || getstate(CS_FOREIGN_VSCROLLBAR))
	{
		vscroll(0, True);
		
		// MW-2008-06-12: This can result in a negative thumbsize - this is
		// probably not good, so we clamp.
		real8 th = MCU_fmax(0.0, textheight + topmargin + bottommargin - TEXT_Y_OFFSET * 2);
		if (th <= textrect.height)
		{
			pos = 0.0;
			size = th;
		}
		else
		{
			pos = texty;
			size = textrect.height;
			linc = fheight;
		}
		if (move)
			vscrollbar->movethumb(pos);
		else
			vscrollbar->setthumb(pos, size, linc, th);

		if (getstate(CS_FOREIGN_VSCROLLBAR))
		{
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole scrollbar.
			vscrollbar -> layer_redrawall();
		}
	}
}

// MW-2011-02-28: [[ Bug 9395 ]] Make sure scrollbars don't get large widths instead of 0.
void MCField::setsbrects()
{
	if (flags & F_HSCROLLBAR)
	{
		hscrollbar->setborderwidth(MClook == LF_WIN95 ? 0 : DEFAULT_BORDER);
		if ((scrollbarwidth == DEFAULT_SB_WIDTH || scrollbarwidth == MAC_SB_WIDTH)
		        && (hscrollbar->getrect().height == DEFAULT_SB_WIDTH
		            || hscrollbar->getrect().height == MAC_SB_WIDTH))
		{
			if (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEGTK)
				scrollbarwidth = MCcurtheme->getmetric(WTHEME_METRIC_TRACKSIZE);
			else
				scrollbarwidth = hscrollbar->getrect().height;
		}
		MCRectangle trect = MCU_reduce_rect(rect, borderwidth ? borderwidth - 1 : 0);
		trect.y += trect.height - scrollbarwidth;
		trect.height = scrollbarwidth;
		if (flags & F_SHADOW)
		{
			if (shadowoffset > 0)
				trect.y -= shadowoffset;
			else
				trect.x += shadowoffset;
			trect.width = MCU_max(trect . width - shadowoffset, 0);
		}
		if (flags & F_VSCROLLBAR)
			trect.width = MCU_max(trect . width - (scrollbarwidth - 1), 0);
		hscrollbar->setrect(trect);
	}
	if (flags & F_VSCROLLBAR)
	{
		vscrollbar->setborderwidth(MClook == LF_WIN95 ? 0 : DEFAULT_BORDER);
		if ((scrollbarwidth == DEFAULT_SB_WIDTH || scrollbarwidth == MAC_SB_WIDTH)
		        && (vscrollbar->getrect().width == DEFAULT_SB_WIDTH
		            || vscrollbar->getrect().width == MAC_SB_WIDTH))
		{
			if (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEGTK)
				scrollbarwidth = MCcurtheme->getmetric(WTHEME_METRIC_TRACKSIZE);
			else
				scrollbarwidth = vscrollbar->getrect().width;
		}
		MCRectangle trect = MCU_reduce_rect(rect, borderwidth ? borderwidth - 1 : 0);
		trect.x += trect.width - scrollbarwidth;
		trect.width = scrollbarwidth;
		if (flags & F_SHADOW)
		{
			if (shadowoffset > 0)
				trect.x -= shadowoffset;
			else
				trect.y += shadowoffset;
			trect.height = MCU_max(trect . height - shadowoffset, 0);
		}
		if (flags & F_HSCROLLBAR)
			trect.height = MCU_max(trect . height - (scrollbarwidth - 1), 0);
		vscrollbar->setrect(trect);
	}
	resetscrollbars(False);
}

void MCField::recompute()
{
    do_recompute(false);
}

void MCField::do_recompute(bool p_force_layout)
{
	if (!opened)
		return;

	uint2 fheight;
	fheight = gettextheight();

	MCParagraph *pgptr = paragraphs;
	fixeda = fixedd = 0;
	textwidth = 0;
	do
	{
		// MW-2012-01-25: [[ ParaStyles ]] Whether to flow or noflow is decided on a
		//   per-paragraph basis.
		pgptr -> layout(p_force_layout);

		uint2 ascent, descent, width;
		pgptr->getmaxline(width, ascent, descent);
		if (ascent > fixeda)
			fixeda = ascent;
		if (descent > fixedd)
			fixedd = descent;
		if (width > textwidth)
			textwidth = width;
		pgptr = pgptr->next();
	}
	while (pgptr != paragraphs);
	if (flags & F_FIXED_HEIGHT)
	{
		fixedheight = fheight;
		fixeda = fixedheight - fixedd;
	}
	else
		fixedheight = 0;
	textheight = 0;
	do
	{
		textheight += pgptr->getheight(fixedheight);
		pgptr = pgptr->next();
	}
	while (pgptr != paragraphs);
	resetscrollbars(False);
	if (MCclickfield == this)
		MCclickfield = nil;

	// MW-2006-03-21: Bug 3344/3377 - Fix the focus border lingering
	if (cursorfield == this)
		replacecursor(False, False);
}

void MCField::textchanged(void)
{
	if (getstate(CS_IN_TEXTCHANGED))
		return;

	signallisteners(P_TEXT);

	setstate(True, CS_IN_TEXTCHANGED);
	message(MCM_text_changed);
	setstate(False, CS_IN_TEXTCHANGED);
}

// MW-2012-02-14: [[ FontRefs ]] Update the field's fontref and all its blocks
//   based on having the given parent fontref.
bool MCField::recomputefonts(MCFontRef p_parent_font, bool p_force)
{
	// First update our font ref (if opened etc.), doing nothing further if it
	// hasn't changed.
	if (!MCObject::recomputefonts(p_parent_font, p_force))
		return false;

	// Now loop through all paragraphs, keeping track if anything changed so
	// we know whether to call recompute or not (the field's font has no effect
	// if all paragraphs have their own).
	bool t_changed;
	t_changed = false;
	if (paragraphs != nil)
	{
		MCParagraph *t_paragraph;
		t_paragraph = paragraphs;
		do
		{
			if (t_paragraph -> recomputefonts(m_font))
				t_changed = true;
			t_paragraph = t_paragraph -> next();
		}
		while(t_paragraph != paragraphs);
	}

	return t_changed;
}


// MW-2012-02-23: [[ FieldChars ]] Count the number of characters between si and
//   ei in the paragraphs corresponding to part_id.
findex_t MCField::countchars(uint32_t p_part_id, findex_t si, findex_t ei)
{
	// Get the first paragraph for this instance of the field
    MCParagraph *t_pg, *t_first_pg;
    bool t_stop;
    t_first_pg = t_pg = resolveparagraphs(p_part_id);
    t_stop = false;

    
    // Loop through the paragraphs until we've gone si code units in
    while (t_pg->gettextlength() < si && !t_stop)
    {
        // Move on to the next paragraph
        si -= t_pg->gettextlengthcr();
        ei -= t_pg->gettextlengthcr();
        t_pg = t_pg->next();
        t_stop = t_pg == t_first_pg;
    }

    // Loop until we reach the end index, counting chars as we go
    findex_t t_count;
    t_count = 0;
    while (t_pg->gettextlength() < ei && !t_stop)
    {
        // Count the number of chars in this paragraph. The only paragraph
        // with a non-zero si valus is the first paragraph.
        MCRange t_cu_range, t_char_range;
        t_cu_range = MCRangeMakeMinMax(si, t_pg->gettextlength());
        // SN-2014-09-11: [[ Bug 13361 ]] We need to remove the codeunits we skipped in the paragraph before counting.
        ei -= si;
        si = 0;
        /* UNCHECKED */ MCStringUnmapIndices(t_pg->GetInternalStringRef(), kMCCharChunkTypeGrapheme, t_cu_range, t_char_range);
        ++t_cu_range.length; // implicit paragraph break
        // SN-2014-05-20 [[ Bug 12432 ]] Add the paragraph break to the number of chars
        ++t_char_range.length; // implicit paragraph break

        t_count += t_char_range.length;
        
        // Move on to the next paragraph
        ei -= t_cu_range.length;
        t_pg = t_pg->next();
        t_stop = t_pg == t_first_pg;
    }
    
    // Count the number of chars in the final paragraph
    if (!t_stop)
    {
        MCRange t_char_range, t_cu_range;
        t_cu_range = MCRangeMakeMinMax(si, ei);
        /* UNCHECKED */ MCStringUnmapIndices(t_pg->GetInternalStringRef(), kMCCharChunkTypeGrapheme, t_cu_range, t_char_range);
        t_count += t_char_range.length;
    }
    
    // Return the number of chars that we encountered
    return t_count;
}

// MW-2012-02-23: [[ FieldChars ]] Adjust field indices (si, ei) to cover the start chars
//   in, ending count chars later.
// SN-2014-04-04: [[ CombiningChars ]] x_si and x_ei are codepoint indices, p_start and p_count are char indices
// We need to take this in consideration in the whole process
void MCField::resolvechars(uint32_t p_part_id, findex_t& x_si, findex_t& x_ei, findex_t p_start, findex_t p_count)
{
    // Get the first paragraph for this instance of the field
    MCParagraph *t_pg, *t_top_para;
    t_top_para = t_pg = resolveparagraphs(p_part_id);
    
    // Loop through the paragraphs until we've gone x_si code units in
    findex_t t_index = 0;
    while ((t_index + t_pg->gettextlengthcr()) < x_si)
    {
        t_index += t_pg->gettextlengthcr();
        t_pg = t_pg->next();
    }
    
    // We now need to calculate how many chars into the paragraph we are
    MCRange t_char_range, t_cu_range;
    t_cu_range = MCRangeMake(0, x_si - t_index);
    /* UNCHECKED */ MCStringMapIndices(t_pg->GetInternalStringRef(), kMCCharChunkTypeGrapheme, t_cu_range, t_char_range);

    // Because we measure chars from the beginning of the paragraph,
    // increase the number of chars we want to skip to account for this.
    p_start += t_char_range.length;
    
    // Loop until we get to the starting paragraph (or reach the end of the field)
    findex_t t_pg_char_length = t_pg -> gettextlengthcr(true);
    while (t_pg_char_length <= p_start)
    {
        // Move to the next paragraph
        p_start -= t_pg_char_length;
        x_si += t_pg -> gettextlengthcr();
        t_pg = t_pg->next();

        // Count the number of chars in the next paragraph
        t_pg_char_length = t_pg -> gettextlengthcr(true);
        
        // If we've reached end of the last paragraph, end the loop
        if (t_pg == t_top_para)
        {
            if (p_start > t_pg_char_length)
            {
                // The start index is at or beyond the end of the field. Clamp it
                p_start = t_pg_char_length;
                break;
            }
        }
    }
    
    // We know the char offset into the paragraph and need to convert
    // this back into a code unit offset.
    t_char_range = MCRangeMake(0, p_start);
    /* UNCHECKED */ MCStringMapIndices(t_pg->GetInternalStringRef(), kMCCharChunkTypeGrapheme, t_char_range, t_cu_range);
    x_si  += t_cu_range . length;
    x_ei = x_si - t_cu_range.length;
    

    // Now we need to do it again but measuring ahead p_count chars. Again,
    // start at the beginning of the current paragraph.
    p_count += p_start;


    // Loop until we get to the final paragraph
    // Note that t_pg_length already contains the measurement for the current pg
    while (t_pg_char_length <= p_count)
    {
        // Move to the next paragraph
        p_count -= t_pg_char_length;
        x_ei += t_pg -> gettextlengthcr();
        t_pg = t_pg->next();
        
        // Have we reached the first paragraph of the field again?
        if (t_pg == t_top_para)
        {
            // End index was too large. Clamp it and stop looping.
            p_count = 0;
            break;
        }
        
        // Count the number of chars in the next paragraph
        t_pg_char_length = t_pg -> gettextlengthcr(true);
    }
    
    // We know the char offset into the paragraph and need to convert this
    // back into a code unit offset.
    t_char_range = MCRangeMake(0, p_count);
    /* UNCHECKED */ MCStringMapIndices(t_pg->GetInternalStringRef(), kMCCharChunkTypeGrapheme, t_char_range, t_cu_range);
    x_ei += t_cu_range.length;
}

// MW-2012-02-23: [[ FieldChars ]] Convert field indices (si, ei) back to char indices.
void MCField::unresolvechars(uint32_t p_part_id, findex_t& x_si, findex_t& x_ei)
{
	// Count the number of chars from the beginning of the field to the
    // starting index, giving us x_si
    uindex_t t_si = countchars(p_part_id, 0, x_si);
    
    // Count the number of chars from x_si to x_ei
    uindex_t t_count = countchars(p_part_id, x_si, x_ei);
    
    x_si = t_si;
    x_ei = t_count + t_si;
}

//-----------------------------------------------------------------------------
//  Redraw Management

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCField::draw(MCDC *dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite)
{
	MCRectangle dirty;
	dirty = p_dirty;

	if (!p_isolated)
	{
		// MW-2011-09-06: [[ Redraw ]] If rendering as a sprite, don't change opacity or ink.
		if (!p_sprite)
		{
			dc -> setopacity(blendlevel * 255 / 100);
			dc -> setfunction(ink);
		}

		// MW-2009-06-10: [[ Bitmap Effects ]]
		if (m_bitmap_effects == NULL)
			dc -> begin(false);
		else
		{
			if (!dc -> begin_with_effects(m_bitmap_effects, MCU_reduce_rect(rect, -gettransient())))
				return;
			dirty = dc -> getclip();
		}
	}

	if ((state & CS_SIZE || state & CS_MOVE) && flags & F_SCROLLBAR)
		setsbrects();

    // MM-2014-08-11: [[ Bug 13149 ]] Make sure only the first thread calls do_recompute.
    if (m_recompute)
    {
        // MM-2014-08-05: [[ Bug 13012 ]] Put locks around recompute to prevent threading issues.
        if (m_recompute)
        {
            if (state & CS_SIZE)
            {
                resetparagraphs();
                do_recompute(true);
            }
            else if (state & CS_SELECTED && (texty != 0 || textx != 0))
            {
                resetparagraphs();
                do_recompute(false);
            }
            m_recompute = false;
        }
    }

	MCRectangle frect = getfrect();
	
	dc->save();
	dc->cliprect(dirty);
	
	MCRectangle trect = frect;
	if (flags & F_SHOW_BORDER && borderwidth)
	{
		trect = MCU_reduce_rect(trect, -borderwidth);
		int2 offset = MClook == LF_MOTIF ? 0 : -1;
		if (flags & F_HSCROLLBAR && trect.height > scrollbarwidth)
			trect.height += scrollbarwidth + offset;
		if (flags & F_VSCROLLBAR && trect.width > scrollbarwidth)
			trect.width += scrollbarwidth + offset;
		if (flags & F_3D)
		{
			if (MCcurtheme  && borderwidth == DEFAULT_BORDER && MCcurtheme->iswidgetsupported(WTHEME_TYPE_TEXTFIELD_FRAME))
			{
				MCWidgetInfo winfo;
				winfo.type = WTHEME_TYPE_TEXTFIELD_FRAME;
				if (MCcurtheme->iswidgetsupported(WTHEME_TYPE_LISTBOX_FRAME)
				        && flags & F_LIST_BEHAVIOR)
					winfo.type = WTHEME_TYPE_LISTBOX_FRAME;
				getwidgetthemeinfo(winfo);
				MCcurtheme->drawwidget(dc, winfo, trect);
			}
			else
				draw3d(dc, trect, ETCH_SUNKEN, borderwidth);
		}
		else
			drawborder(dc, trect, borderwidth);
	}
	
	dc->restore();

	// MW-2009-06-14: If the field is opaque, then render the contents with that
	//   marked.
	bool t_was_opaque = false;
	if (getflag(F_OPAQUE))
		t_was_opaque = dc -> changeopaque(true);
	drawrect(dc, dirty);
	if (getflag(F_OPAQUE))
		dc -> changeopaque(t_was_opaque);

	frect = getfrect();
	if (flags & F_SHADOW)
	{
		drawshadow(dc, rect, shadowoffset);
	}

	if (state & CS_KFOCUSED && borderwidth != 0
	        && !(extraflags & EF_NO_FOCUS_BORDER) &&
	        !(MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEGTK) )
		drawfocus(dc, p_dirty);

	if (!p_isolated)
	{
		dc -> end();
	}
}

//  Redraw Management
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

#define FIELD_EXTRA_TEXTDIRECTION   (1 << 0)
// SN-2015-04-30: [[ Bug 15175 ]] TabAlignment flag added
#define FIELD_EXTRA_TABALIGN        (1 << 1)
#define FIELD_EXTRA_KEYBOARDTYPE    (1 << 2)
#define FIELD_EXTRA_RETURNKEYTYPE    (1 << 3)

IO_stat MCField::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	uint32_t t_size, t_flags;
	t_size = 0;
	t_flags = 0;
    
    // MW-2014-06-20: [[ Bug 13315 ]] Save the textDirection of the field
    if (text_direction != kMCTextDirectionAuto)
    {
        t_flags |= FIELD_EXTRA_TEXTDIRECTION;
        t_size += sizeof(uint8_t);
    }
    
    // SN-2015-04-30: [[ Bug 15175 ]] Save the tabalign property of the field
    if (nalignments != 0)
    {
        t_flags |= FIELD_EXTRA_TABALIGN;
        // Save number of tab alignments, and then each of them.
        t_size += sizeof(uint16_t);
        t_size += sizeof(int8_t) * nalignments;
    }
    
    if (keyboard_type != kMCInterfaceKeyboardTypeNone)
    {
        t_flags |= FIELD_EXTRA_KEYBOARDTYPE;
        t_size += sizeof(int8_t);
    }
    
    if (return_key_type != kMCInterfaceReturnKeyTypeNone)
    {
        t_flags |= FIELD_EXTRA_RETURNKEYTYPE;
        t_size += sizeof(int8_t);
    }

	IO_stat t_stat;
	t_stat = p_stream . WriteTag(t_flags, t_size);

    if (t_stat == IO_NORMAL && (t_flags & FIELD_EXTRA_TEXTDIRECTION))
        t_stat = p_stream . WriteU8(text_direction);

    if (t_stat == IO_NORMAL && (t_flags & FIELD_EXTRA_TABALIGN))
    {
        t_stat = p_stream . WriteU16(nalignments);

        for (uint2 i = 0; i < nalignments && t_stat == IO_NORMAL; ++i)
            t_stat = p_stream . WriteS8((int8_t)alignments[i]);
    }
    
    if (t_stat == IO_NORMAL && (t_flags & FIELD_EXTRA_KEYBOARDTYPE))
    {
        t_stat = p_stream . WriteS8((int8_t)keyboard_type);
    }
    
    if (t_stat == IO_NORMAL && (t_flags & FIELD_EXTRA_RETURNKEYTYPE))
    {
        t_stat = p_stream . WriteS8((int8_t)return_key_type);
    }
    
    if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedsave(p_stream, p_part, p_version);
    
	return t_stat;
}

IO_stat MCField::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_length)
{
    IO_stat t_stat;
	t_stat = IO_NORMAL;
    
    if (p_length > 0)
    {
		uint4 t_flags, t_length, t_header_length;
		t_stat = checkloadstat(p_stream . ReadTag(t_flags, t_length, t_header_length));
        
		if (t_stat == IO_NORMAL)
			t_stat = checkloadstat(p_stream . Mark());
        
        // MW-2014-06-20: [[ 13315 ]] Load the textDirection of the field.
        if (t_stat == IO_NORMAL && (t_flags & FIELD_EXTRA_TEXTDIRECTION) != 0)
        {
            uint8_t t_value;
            t_stat = checkloadstat(p_stream . ReadU8(t_value));
            if (t_stat == IO_NORMAL)
                text_direction = (MCTextDirection)t_value;
        }

        // SN-2015-04-30: [[ Bug 15175 ]] Read the tablAlign property, if saved
        if (t_stat == IO_NORMAL && (t_flags & FIELD_EXTRA_TABALIGN) != 0)
        {
            uint16_t t_nalign;
            intenum_t *t_alignments;
            t_alignments = NULL;

            t_stat = checkloadstat(p_stream . ReadU16(t_nalign));

            if (t_stat == IO_NORMAL && t_nalign != 0)
            {
                if (!MCMemoryAllocate(sizeof(intenum_t) * t_nalign, t_alignments))
                    t_stat = checkloadstat(IO_ERROR);

                for (uint2 i = 0; i < t_nalign && t_stat == IO_NORMAL; ++i)
                {
                    int8_t t_align;
                    if ((t_stat = checkloadstat(p_stream . ReadS8(t_align))) == IO_NORMAL)
                        t_alignments[i] = t_align;
                }

                if (t_stat == IO_NORMAL)
                {
                    nalignments = t_nalign;
                    alignments = t_alignments;
                }
                else
                    MCMemoryDelete(t_alignments);
            }
        }
        
        if (t_stat == IO_NORMAL && (t_flags & FIELD_EXTRA_KEYBOARDTYPE) != 0)
        {
            int8_t t_value;
            t_stat = checkloadstat(p_stream . ReadS8(t_value));
            if (t_stat == IO_NORMAL)
            {
                keyboard_type = static_cast<MCInterfaceKeyboardType>(t_value);
            }
        }
        
        if (t_stat == IO_NORMAL && (t_flags & FIELD_EXTRA_RETURNKEYTYPE) != 0)
        {
            int8_t t_value;
            t_stat = checkloadstat(p_stream . ReadS8(t_value));
            if (t_stat == IO_NORMAL)
            {
                return_key_type =  static_cast<MCInterfaceReturnKeyType>(t_value);
            }
        }
        
        if (t_stat == IO_NORMAL)
            t_stat = checkloadstat(p_stream . Skip(t_length));
        
        if (t_stat == IO_NORMAL)
            p_length -= t_length + t_header_length;
    }
    
	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedload(p_stream, p_version, p_length);
    
	return t_stat;
}

IO_stat MCField::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;
	int4 savex = textx;
	int4 savey = texty;

	if ((stat = IO_write_uint1(OT_FIELD, stream)) != IO_NORMAL)
		return stat;

    // AL-2014-09-12: [[ Bug 13315 ]] Force an extension if field has explicit textDirection.
    // SN-2015-04-30: [[ Bug 15175 ]] Force an extension if field has tabalign.
    bool t_has_extension;
    t_has_extension = text_direction != kMCTextDirectionAuto ||
                      nalignments != 0 ||
                      keyboard_type != kMCInterfaceKeyboardTypeNone ||
                      return_key_type != kMCInterfaceReturnKeyTypeNone;
    
    if ((stat = MCObject::save(stream, p_part, t_has_extension || p_force_ext, p_version)) != IO_NORMAL)
		return stat;

	if ((stat = IO_write_int2(leftmargin, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_int2(rightmargin, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_int2(topmargin, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_int2(bottommargin, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_int2(indent, stream)) != IO_NORMAL)
		return stat;
	if (flags & F_TABS)
	{
		if ((stat = IO_write_uint2(ntabs, stream)) != IO_NORMAL)
			return stat;
		uint2 i;
		for (i = 0 ; i < ntabs ; i++)
			if ((stat = IO_write_uint2(tabs[i], stream)) != IO_NORMAL)
				return stat;
	}
	if ((stat = savepropsets(stream, p_version)) != IO_NORMAL)
		return stat;
	if (fdata != NULL)
	{
		if (opened)
		{
			resetparagraphs();
			fdata->setparagraphs(paragraphs);
		}

		// If p_part != 0, and the field is shared, we only want to save the '0' part.
		if (getflag(F_SHARED_TEXT))
		{
			MCCdata *tptr;
			tptr = getcarddata(fdata, 0, False);
			if (tptr != NULL)
				if ((stat = tptr -> save(stream, OT_FDATA, 0, this, p_version)) != IO_NORMAL)
					return stat;
		}
		else
		{
			MCCdata *tptr = fdata;
			do
			{
				if ((stat = tptr->save(stream, OT_FDATA, p_part, this, p_version)) != IO_NORMAL)
					return stat;
				tptr = tptr->next();
			}
			while (tptr != fdata);
		}
	}
	if (vscrollbar != NULL)
		if ((stat = vscrollbar->save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
			return stat;
	if (hscrollbar != NULL)
		if ((stat = hscrollbar->save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
			return stat;
	if (opened)
	{
		// OK-2009-01-27: [[Bug 6884]] - Prevent messages being sent here as code execution while saving
		// is a bad idea.
		setstate(True, CS_NO_MESSAGES);

		if (fdata == NULL)
			resetparagraphs();
		do_recompute(false);
		hscroll(savex - textx, False);
		vscroll(savey - texty, False);
		resetscrollbars(True);

		setstate(False, CS_NO_MESSAGES);
	}
	return IO_NORMAL;
}

IO_stat MCField::load(IO_handle stream, uint32_t version)
{
	IO_stat stat;

	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_int2(&leftmargin, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_int2(&rightmargin, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_int2(&topmargin, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_int2(&bottommargin, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_int2(&indent, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if (flags & F_TABS)
	{
		if ((stat = IO_read_uint2(&ntabs, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		tabs = new (nothrow) uint2[ntabs];
		uint2 i;
		for (i = 0 ; i < ntabs ; i++)
			if ((stat = IO_read_uint2(&tabs[i], stream)) != IO_NORMAL)
				return checkloadstat(stat);
	}
	if (version <= kMCStackFileFormatVersion_2_0)
	{
		rect = MCU_reduce_rect(rect, MCfocuswidth);
		if (flags & F_LIST_BEHAVIOR)
			flags |= F_TRAVERSAL_ON;
	}
	if (flags & F_VGRID)
		flags |= F_DONT_WRAP;
	if ((stat = loadpropsets(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);
	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if (type == OT_FDATA)
		{
			MCCdata *newfdata = new (nothrow) MCCdata;
			if ((stat = newfdata->load(stream, this, version)) != IO_NORMAL)
			{
				delete newfdata;
				return checkloadstat(stat);
			}
			newfdata->appendto(fdata);
		}
		else
			if (type == OT_SCROLLBAR)
			{
				if (flags & F_VSCROLLBAR && vscrollbar == NULL)
				{
					vscrollbar = new (nothrow) MCScrollbar;
					vscrollbar->setparent(this);
					if ((stat = vscrollbar->load(stream, version)) != IO_NORMAL)
						return checkloadstat(stat);
					vscrollbar->setflag(getflag(F_DISABLED), F_DISABLED);
					vscrollbar->allowmessages(False);
					vscrollbar->setembedded();
					scrollbarwidth = vscrollbar->getrect().width;
				}
				else
					if (flags & F_HSCROLLBAR && hscrollbar == NULL)
					{
						hscrollbar = new (nothrow) MCScrollbar;
						hscrollbar->setparent(this);
						if ((stat = hscrollbar->load(stream, version)) != IO_NORMAL)
							return checkloadstat(stat);
						hscrollbar->setflag(getflag(F_DISABLED), F_DISABLED);
						hscrollbar->allowmessages(False);
						hscrollbar->setembedded();
						if (!(flags & F_VSCROLLBAR))
							scrollbarwidth = hscrollbar->getrect().height;
					}
					else
					{
						MCS_seek_cur(stream, -1);
						break;
					}
			}
			else
			{
				MCS_seek_cur(stream, -1);
				break;
			}
	}
	return IO_NORMAL;
}

///////////////////////////////////////////////////////////////////////////////

void MCField::unlink(MCControl *p_control)
{
	if (hscrollbar == (MCScrollbar *)p_control)
	{
		setstate(False, CS_FOREIGN_HSCROLLBAR);
		hscrollbar = NULL;
	}

	if (vscrollbar == (MCScrollbar *)p_control)
	{
		setstate(False, CS_FOREIGN_VSCROLLBAR);
		vscrollbar = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////

bool MCField::imagechanged(MCImage *p_image, bool p_deleting)
{
	bool t_used = false;
	MCParagraph *t_para = paragraphs;

	if (t_para != NULL)
	{
		do
		{
			t_used = t_para->imagechanged(p_image, p_deleting) || t_used;
			t_para = t_para->next();
		}
		while (t_para != paragraphs);
		
		if (t_used)
		{
			do_recompute(true);
			layer_redrawall();
		}
	}

	return t_used;
}

///////////////////////////////////////////////////////////////////////////////

bool MCField::IsCursorMovementVisual()
{
    if (cursor_movement == kMCFieldCursorMovementLogical)
        return false;
    else if (cursor_movement == kMCFieldCursorMovementVisual)
        return true;
    else
    {
#ifdef _WIN32
        return false;
#else
        return true;
#endif
    }
}

////////////////////////////////////////////////////////////////////////////////

MCPlatformControlType MCField::getcontroltype()
{
    MCPlatformControlType t_type;
    t_type = MCObject::getcontroltype();
    
    if (t_type != kMCPlatformControlTypeGeneric)
        return t_type;
    else
        t_type = kMCPlatformControlTypeInputField;
    
    if (flags & F_LIST_BEHAVIOR)
        t_type = kMCPlatformControlTypeList;
    else if ((flags & F_LOCK_TEXT) && !(flags & F_OPAQUE))
        t_type = kMCPlatformControlTypeLabel;
    
    return t_type;
}

MCPlatformControlPart MCField::getcontrolsubpart()
{
    return kMCPlatformControlPartNone;
}

MCPlatformControlState MCField::getcontrolstate()
{
    int t_state;
    t_state = MCControl::getcontrolstate();
    
    return MCPlatformControlState(t_state);
}

////////////////////////////////////////////////////////////////////////////////

void MCField::layer_redrawrect(const MCRectangle& m_dirty_rect)
{
    // If the parent of this field is a button, this is the entry field for
    // a combo-box. In that case, the text is vertically centred(-ish)
    // within the field so the y offsets we calculate here are wrong.
    // Calculating the correct position is fairly complicated so the simpler
    // solution is to just invalidate the whole field in that case.
    //
    // This doesn't make much of a difference performance-wise; the number
    // of pixels to be rasterised is higher but only by 3 or 4 rows in most
    // cases.
    if (getparent()->gettype() != CT_BUTTON)
        MCControl::layer_redrawrect(m_dirty_rect);
    else
        MCControl::layer_redrawrect(rect);
}
