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
#include "MCBlock.h"
#include "line.h"
#include "field.h"
#include "paragraf.h"
//#include "execpt.h"
#include "util.h"
#include "mcerror.h"
#include "segment.h"

#include "globals.h"

#include "exec-interface.h"

#include <stddef.h>

////////////////////////////////////////////////////////////////////////////////

static struct {const char *name; Properties prop; } kMCParagraphAttrsProps[] =
{
	{"textAlign", P_TEXT_ALIGN},
	{"listStyle", P_LIST_STYLE},
	{"listDepth", P_LIST_DEPTH},
	{"listIndent", P_LIST_INDENT},
	{"firstIndent", P_FIRST_INDENT},
	{"leftIndent", P_LEFT_INDENT},
	{"rightIndent", P_RIGHT_INDENT},
	{"spaceAbove", P_SPACE_ABOVE},
	{"spaceBelow", P_SPACE_BELOW},
	{"tabStops", P_TAB_STOPS},
	{"backgroundColor", P_BACK_COLOR},
	{"borderWidth", P_BORDER_WIDTH},
	{"borderColor", P_BORDER_COLOR},
	{"hGrid", P_HGRID},
	{"vGrid", P_VGRID},
	{"dontWrap", P_DONT_WRAP},
	{"padding", P_PADDING},
    {"listIndex", P_LIST_INDEX},
	{nil},
};

bool MCParagraph::hasattrs(void)
{
	return attrs != nil;
}

#ifdef LEGACY_EXEC
void MCParagraph::storeattrs(MCArrayRef dst)
{
	MCExecPoint ep(nil, nil, nil);
	for(uint32_t i = 0; kMCParagraphAttrsProps[i] . name != nil; i++)
	{
		getparagraphattr(kMCParagraphAttrsProps[i] . prop, ep, False);
		if (!ep . isempty())
			/* UNCHECKED */ ep . storearrayelement_cstring(dst, kMCParagraphAttrsProps[i] . name);
	}
}
#endif

void MCParagraph::fetchattrs(MCArrayRef src)
{
    intenum_t t_intenum_value;
    uinteger_t t_uint_value;
    integer_t t_integer_value;
    MCStringRef t_stringref_value;
    MCBooleanRef t_boolean_value;
    MCInterfaceNamedColor t_color;
    MCExecContext ctxt(nil, nil, nil);
    
    t_color . name = nil;

    if (ctxt . CopyElementAsInteger(src, MCNAME("textAlign"), false, t_intenum_value))
        SetTextAlign(ctxt, &t_intenum_value);
    else
        SetTextAlign(ctxt, nil);

    if (ctxt . CopyElementAsInteger(src, MCNAME("listStyle"), false, t_intenum_value))
        SetListStyle(ctxt, t_intenum_value);

    if (ctxt . CopyElementAsUnsignedInteger(src, MCNAME("listDepth"), false, t_uint_value))
        SetListDepth(ctxt, &t_uint_value);
    else
        SetListDepth(ctxt, nil);

    if (ctxt . CopyElementAsInteger(src, MCNAME("listIndent"), false, t_integer_value))
        SetListIndent(ctxt, &t_integer_value);
    else
        SetListIndent(ctxt, nil);

    if (ctxt . CopyElementAsInteger(src, MCNAME("firstIndent"), false, t_integer_value))
        SetFirstIndent(ctxt, &t_integer_value);
    else
        SetFirstIndent(ctxt, nil);

    if (ctxt . CopyElementAsInteger(src, MCNAME("leftIndent"), false, t_integer_value))
        SetLeftIndent(ctxt, &t_integer_value);
    else
        SetLeftIndent(ctxt, nil);

    if (ctxt . CopyElementAsInteger(src, MCNAME("rightIndent"), false, t_integer_value))
        SetRightIndent(ctxt, &t_integer_value);
    else
        SetRightIndent(ctxt, nil);

    if (ctxt . CopyElementAsUnsignedInteger(src, MCNAME("spaceAbove"), false, t_uint_value))
        SetSpaceAbove(ctxt, &t_uint_value);
    else
        SetSpaceAbove(ctxt, nil);

    if (ctxt . CopyElementAsUnsignedInteger(src, MCNAME("spaceBelow"), false, t_uint_value))
        SetSpaceBelow(ctxt, &t_uint_value);
    else
        SetSpaceBelow(ctxt, nil);

    if (ctxt . CopyElementAsString(src, MCNAME("tabStops"), false, t_stringref_value))
    {
        uint16_t t_count;
        vector_t<uinteger_t> t_tabstops;
        uint16_t *t_uint16_tabs;

        if (MCField::parsetabstops(P_TAB_STOPS, t_stringref_value, t_uint16_tabs, t_count))
        {
            /* UNCHECKED */ MCMemoryAllocate(t_count, t_tabstops . elements);
            for (uint16_t i = 0; i < t_count; ++i)
                t_tabstops . elements[i] = t_uint16_tabs[i];

            SetTabStops(ctxt, t_tabstops);
            MCMemoryDeallocate(t_tabstops . elements);
            MCMemoryDeallocate(t_uint16_tabs);
        }

        MCValueRelease(t_stringref_value);
    }

    if (ctxt . CopyElementAsString(src, MCNAME("backgroundColor"), false, t_stringref_value))
    {
        MCInterfaceNamedColorParse(ctxt, t_stringref_value, t_color);
        SetBackColor(ctxt, t_color);
        
        MCValueRelease(t_stringref_value);
    }

    if (ctxt . CopyElementAsUnsignedInteger(src, MCNAME("borderWidth"), false, t_uint_value))
        SetBorderWidth(ctxt, &t_uint_value);
    else
        SetBorderWidth(ctxt, nil);


    if (ctxt . CopyElementAsString(src, MCNAME("borderColor"), false, t_stringref_value))
    {
        MCInterfaceNamedColorParse(ctxt, t_stringref_value, t_color);
        SetBorderColor(ctxt, t_color);
        
        MCValueRelease(t_stringref_value);
    }

    if (ctxt . CopyElementAsBoolean(src, MCNAME("hGrid"), false, t_boolean_value))
    {
        bool t_bool;
        t_bool = t_boolean_value == kMCTrue;
        SetHGrid(ctxt, &t_bool);
        MCValueRelease(t_boolean_value);
    }
    else
        SetHGrid(ctxt, nil);

    if (ctxt . CopyElementAsBoolean(src, MCNAME("vGrid"), false, t_boolean_value))
    {
        bool t_bool;
        t_bool = t_boolean_value == kMCTrue;
        SetVGrid(ctxt, &t_bool);
        MCValueRelease(t_boolean_value);
    }
    else
        SetVGrid(ctxt, nil);

    if (ctxt . CopyElementAsBoolean(src, MCNAME("dontWrap"), false, t_boolean_value))
    {
        bool t_bool;
        t_bool = t_boolean_value == kMCTrue;
        SetDontWrap(ctxt, &t_bool);
    }
    else
        SetDontWrap(ctxt, nil);

    if (ctxt . CopyElementAsUnsignedInteger(src, MCNAME("padding"), false, t_uint_value))
        SetPadding(ctxt, &t_uint_value);
    else
        SetPadding(ctxt, nil);

    if (ctxt . CopyElementAsInteger(src, MCNAME("listIndex"), false, t_integer_value))
        SetListIndent(ctxt, &t_integer_value);
    else
        SetListIndent(ctxt, nil);
}

IO_stat MCParagraph::loadattrs(IO_handle stream, uint32_t version)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	// Make sure the paragraph has no attrs.
	clearattrs();

	// Read in the size of the attrs.
	uint32_t t_size;
	if (t_stat == IO_NORMAL)
		t_stat = IO_read_uint2or4(&t_size, stream);
	
	// Fetch the stream position at this point and use it to compute
	// the end of the record.
	int64_t t_attr_end;
	if (t_stat == IO_NORMAL)
		t_attr_end = MCS_tell(stream) + t_size;

	// Read the (first) flags word
	uint2 t_flags;
	if (t_stat == IO_NORMAL)
		t_stat = IO_read_uint2(&t_flags, stream);
		
	// Initialize the paragraph attrs to default values.
	attrs = new MCParagraphAttrs;
	attrs -> flags = t_flags;

	// Now read each attribute in turn.
	if (t_stat == IO_NORMAL && (attrs -> flags & (PA_HAS_TEXT_ALIGN | PA_HAS_LIST_STYLE | PA_HAS_VGRID | PA_HAS_HGRID | PA_HAS_DONT_WRAP)) != 0)
	{
		uint2 t_value;
		t_stat = IO_read_uint2(&t_value, stream);
		if (t_stat == IO_NORMAL)
		{
			attrs -> text_align = (t_value & 0x03);
			attrs -> list_style = ((t_value >> 2) & 0x0f);
			attrs -> list_depth = (t_value >> 6) & 0x0f;
			attrs -> vgrid = ((t_value >> 10) & 0x01) != 0;
			attrs -> hgrid = ((t_value >> 11) & 0x01) != 0;
			attrs -> dont_wrap = ((t_value >> 12) & 0x01) != 0;
		}
	}
	if (t_stat == IO_NORMAL && (attrs -> flags & (PA_HAS_BORDER_WIDTH)) != 0)
		t_stat = IO_read_uint1(&attrs -> border_width, stream);
	// MW-2012-02-29: [[ Bug ]] Read the first indent field if either has first
	//   or has list indent.
	if (t_stat == IO_NORMAL && ((attrs -> flags & PA_HAS_FIRST_INDENT) != 0 || (attrs -> flags & PA_HAS_LIST_INDENT) != 0))
		t_stat = IO_read_int2(&attrs -> first_indent, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_LEFT_INDENT) != 0)
		t_stat = IO_read_int2(&attrs -> left_indent, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_RIGHT_INDENT) != 0)
		t_stat = IO_read_int2(&attrs -> right_indent, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_SPACE_ABOVE) != 0)
		t_stat = IO_read_int2(&attrs -> space_above, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_SPACE_BELOW) != 0)
		t_stat = IO_read_int2(&attrs -> space_below, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_TABS) != 0)
	{
		t_stat = IO_read_uint2(&attrs -> tab_count, stream);
		if (t_stat == IO_NORMAL)
		{
			attrs -> tabs = new uint16_t[attrs -> tab_count];
			for(uint32_t i = 0; i < attrs -> tab_count && t_stat == IO_NORMAL; i++)
				t_stat = IO_read_uint2(&attrs -> tabs[i], stream);
		}
	}
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_BACKGROUND_COLOR) != 0)
		t_stat = IO_read_uint4(&attrs -> background_color, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_BORDER_COLOR) != 0)
		t_stat = IO_read_uint4(&attrs -> border_color, stream);
	// MW-2012-02-22: [[ Bug ]] Load the padding setting from the stream.
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_PADDING) != 0)
		t_stat = IO_read_uint1(&attrs -> padding, stream);

	// MW-2012-03-19: [[ HiddenText ]] If there is still more to read, we must have
	//   a second flags word.
	if (t_stat == IO_NORMAL && MCS_tell(stream) + 2 <= t_attr_end)
	{
		uint2 t_flags_2;
		if (t_stat == IO_NORMAL)
			t_stat = IO_read_uint2(&t_flags_2, stream);
		if (t_stat == IO_NORMAL)
		{
				attrs -> flags |= t_flags_2 << 16;
		
			// When saving, the 'hidden' attr is only saved as a flag, so set its value.
			if ((attrs -> flags & PA_HAS_HIDDEN) != 0)
				attrs -> hidden = true;
		}
		
		// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
		if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_METADATA) != 0)
            t_stat = IO_read_stringref_new(attrs -> metadata, stream, version >= 7000);

		if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_LIST_INDEX) != 0)
			t_stat = IO_read_uint2(&attrs -> list_index, stream);
	}
	
	// Finally skip to the end of the record for future modifications.
	if (t_stat == IO_NORMAL)
		t_stat = MCS_seek_set(stream, t_attr_end);

	return t_stat;
}

IO_stat MCParagraph::saveattrs(IO_handle stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	// First we work out the size of the attrs.
	uint32_t t_attr_size;
	t_attr_size = measureattrs();
	
	// Write out the size as either a 2 byte or 4 byte size.
	if (t_stat == IO_NORMAL)
		t_stat = IO_write_uint2or4(t_attr_size, stream);

	// Write out the (first) flags field.
	if (t_stat == IO_NORMAL)
		t_stat = IO_write_uint2(attrs -> flags, stream);
		
	// Now write out all the attrs.
	
	if (t_stat == IO_NORMAL && (attrs -> flags & (PA_HAS_TEXT_ALIGN | PA_HAS_LIST_STYLE | PA_HAS_VGRID | PA_HAS_HGRID | PA_HAS_DONT_WRAP)) != 0)
		t_stat = IO_write_uint2(attrs -> text_align | ((attrs -> list_style << 2) | (attrs -> list_depth << 6) | (attrs -> vgrid << 10) | (attrs -> hgrid << 11) | (attrs -> dont_wrap << 12)), stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & (PA_HAS_BORDER_WIDTH)) != 0)
		t_stat = IO_write_uint1(attrs -> border_width, stream);
	// MW-2012-02-29: [[ Bug ]] Write the first indent field if either has first
	//   or has list indent.
	if (t_stat == IO_NORMAL && ((attrs -> flags & PA_HAS_FIRST_INDENT) != 0 || (attrs -> flags & PA_HAS_LIST_INDENT) != 0))
		t_stat = IO_write_int2(attrs -> first_indent, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_LEFT_INDENT) != 0)
		t_stat = IO_write_int2(attrs -> left_indent, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_RIGHT_INDENT) != 0)
		t_stat = IO_write_int2(attrs -> right_indent, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_SPACE_ABOVE) != 0)
		t_stat = IO_write_int2(attrs -> space_above, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_SPACE_BELOW) != 0)
		t_stat = IO_write_int2(attrs -> space_below, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_TABS) != 0)
	{
		t_stat = IO_write_uint2(attrs -> tab_count, stream);
		for(uint32_t i = 0; i < attrs -> tab_count && t_stat == IO_NORMAL; i++)
			t_stat = IO_write_uint2(attrs -> tabs[i], stream);
	}
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_BACKGROUND_COLOR) != 0)
		t_stat = IO_write_uint4(attrs -> background_color, stream);
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_BORDER_COLOR) != 0)
		t_stat = IO_write_uint4(attrs -> border_color, stream);
	// MW-2012-02-22: [[ Bug ]] Save the padding setting out to the stream.
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_PADDING) != 0)
		t_stat = IO_write_uint1(attrs -> padding, stream);

	// MW-2012-03-19: [[ HiddenText ]] If the hidden prop is set, then save out the second flags
	//   word. Note that PA_HAS_HIDDEN is only set if the prop is true, so there's no need to
	//   add any more data.
	// MW-2012-11-13: [[ ParaMetadata ]] Write out the extended flags word if we have metadata.
	// MW-2012-11-13: [[ ParaListIndex ]] Write out the extended flags word if we have list index.
	if (t_stat == IO_NORMAL && (attrs -> flags & (PA_HAS_HIDDEN | PA_HAS_METADATA | PA_HAS_LIST_INDEX)) != 0)
		t_stat = IO_write_uint2(attrs -> flags >> 16, stream);

	// MW-2012-11-13: [[ ParaMetadata ]] Write out the metadata, if any.
	// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_METADATA) != 0)
        t_stat = IO_write_stringref_new(attrs -> metadata, stream, MCstackfileversion >= 7000);

	// MW-2012-11-13: [[ ParaListIndex ]] Write out the list index, if any.
	if (t_stat == IO_NORMAL && (attrs -> flags & PA_HAS_LIST_INDEX) != 0)
		t_stat = IO_write_uint2(attrs -> list_index, stream);
	
	return t_stat;
}

uint32_t MCParagraph::measureattrs(void)
{
	// If there are no attrs, then the size is 0.
	if (attrs == nil)
		return 0;

	uint32_t t_size;
	t_size = 0;
	
	// The flags field.
	t_size = 2;
	// The textAlign / listStyle / listDepth / vGrid / hGrid / dontWrap field.
	if ((attrs -> flags & (PA_HAS_TEXT_ALIGN | PA_HAS_LIST_STYLE | PA_HAS_VGRID | PA_HAS_HGRID | PA_HAS_DONT_WRAP)) != 0)
		t_size += 2;
	// The borderWidth field.
	if ((attrs -> flags & PA_HAS_BORDER_WIDTH) != 0)
		t_size += 1;
	// The firstIndent / listIndent field.
	if ((attrs -> flags & PA_HAS_FIRST_INDENT) != 0 || (attrs -> flags & PA_HAS_LIST_INDENT) != 0)
		t_size += 2;
	// The leftIndent field.
	if ((attrs -> flags & PA_HAS_LEFT_INDENT) != 0)
		t_size += 2;
	// The rightIndent field.
	if ((attrs -> flags & PA_HAS_RIGHT_INDENT) != 0)
		t_size += 2;
	// The spaceAbove field.
	if ((attrs -> flags & PA_HAS_SPACE_ABOVE) != 0)
		t_size += 2;
	// The spaceBelow field.
	if ((attrs -> flags & PA_HAS_SPACE_BELOW) != 0)
		t_size += 2;
	// The tabStops field.
	if ((attrs -> flags & PA_HAS_TABS) != 0)
		t_size += 2 + attrs -> tab_count * 2;
	// The backgroundColor field.
	if ((attrs -> flags & PA_HAS_BACKGROUND_COLOR) != 0)
		t_size += 4;
	// The borderColor field.
	if ((attrs -> flags & PA_HAS_BORDER_COLOR) != 0)
		t_size += 4;
	// The padding field.
	if ((attrs -> flags & PA_HAS_PADDING) != 0)
		t_size += 1;
	
	// MW-2012-03-19: [[ HiddenText ]] If the hidden prop is set, then we need a second flags
	//   word.
	if ((attrs -> flags & (PA_HAS_HIDDEN | PA_HAS_METADATA | PA_HAS_LIST_INDEX)) != 0)
		t_size += 2;
	
	// MW-2012-11-13: [[ ParaMetadata ]] If the paragraph has metadata then add that on.
    extern uint32_t measure_stringref(MCStringRef string);
	if ((attrs -> flags & PA_HAS_METADATA) != 0)
        t_size += measure_stringref(attrs -> metadata);

	// MW-2012-11-13: [[ ParaListIndex ]] If the paragraph has a list index, then add that on.
	if ((attrs -> flags & PA_HAS_LIST_INDEX) != 0)
		t_size += 2;

	return t_size;
}

#ifdef LEGACY_EXEC
template<typename T, int Min, int Max> static bool setparagraphattr_int(MCExecPoint& ep, MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset, Exec_errors p_error)
{
	if (ep . isempty())
	{
		if (attrs == nil)
			return true;

		((T *)((char *)attrs + p_field_offset))[0] = 0;
		attrs -> flags &= ~p_flag;
		return true;
	}

	int4 t_field;
	if (!MCU_stoi4(ep . getsvalue(), t_field))
	{
		MCeerror -> add(p_error, 0, 0, ep . getsvalue());
		return false;
	}

	T t_clamped_field;
	t_clamped_field = MCMin(MCMax(t_field, Min), Max);

	if (attrs == nil)
		attrs = new MCParagraphAttrs;

	attrs -> flags |= p_flag;
	((T *)((char *)attrs + p_field_offset))[0] = t_clamped_field;

	return true;
}

static bool setparagraphattr_uint8(MCExecPoint& ep, MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset, Exec_errors p_error)
{
	return setparagraphattr_int<uint8_t, 0, 255>(ep, attrs, p_flag, p_field_offset, p_error);
}

static bool setparagraphattr_int16(MCExecPoint& ep, MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset, Exec_errors p_error)
{
	return setparagraphattr_int<int16_t, INT16_MIN, INT16_MAX>(ep, attrs, p_flag, p_field_offset, p_error);
}

static bool setparagraphattr_color(MCExecPoint& ep, MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset)
{
	if (ep . isempty())
	{
		if (attrs == nil)
			return true;

		((uint32_t *)((char *)attrs + p_field_offset))[0] = 0;
		attrs -> flags &= ~p_flag;
		return true;
	}

	MCColor t_color;
	MCAutoStringRef t_value;
	ep.copyasstringref(&t_value);
	if (!MCscreen -> parsecolor(*t_value, t_color, nil))
	{
		MCeerror->add(EE_OBJECT_BADCOLOR, 0, 0, *t_value);
		return false;
	}

	if (attrs == nil)
		attrs = new MCParagraphAttrs;

	MCscreen -> alloccolor(t_color);

	attrs -> flags |= p_flag;
	((uint32_t *)((char *)attrs + p_field_offset))[0] = t_color . pixel;

	return true;
}

static bool setparagraphattr_bool(MCExecPoint& ep, MCParagraphAttrs*& attrs, uint32_t p_flag, bool& r_new_value)
{
	if (ep . isempty())
	{
		if (attrs == nil)
			return true;

		r_new_value = false;
		attrs -> flags &= ~p_flag;
		return true;
	}

	Boolean t_new_value;
	if (ep . getboolean(t_new_value, 0, 0, EE_PROPERTY_NAB) != ES_NORMAL)
		return false;

	if (attrs == nil)
		attrs = new MCParagraphAttrs;

	attrs -> flags |= p_flag;
	r_new_value = t_new_value == True;

	return true;
}
#endif

#ifdef LEGACY_EXEC
Exec_stat MCParagraph::setparagraphattr(Properties which, MCExecPoint& ep)
{
	switch(which)
	{
#ifdef OLD_EXEC
	case P_TEXT_ALIGN:
		{
			if (ep . isempty())
			{
				if (attrs == nil)
					break;

				attrs -> flags &= ~PA_HAS_TEXT_ALIGN;
				attrs -> text_align = kMCParagraphTextAlignLeft;
				break;
			}
			
			uint4 myflags;
			uint2 fontheight, size, style;
			MCAutoStringRef fname;
            MCAutoStringRef t_value;
            ep . copyasstringref(&t_value);
			if (MCF_parsetextatts(which, *t_value, myflags, &fname, fontheight, size, style) != ES_NORMAL)
				return ES_ERROR;

			if (attrs == nil)
				attrs = new MCParagraphAttrs;
			attrs -> flags |= PA_HAS_TEXT_ALIGN;
			attrs -> text_align = ((myflags & F_ALIGNMENT) >> F_ALIGNMENT_SHIFT);
		}
		break;
	case P_LIST_STYLE:
		{
			int32_t t_new_list_style;
			t_new_list_style = -1;
			for(uint32_t i = 0; MCliststylestrings[i] != nil; i++)
				if (ep . getsvalue() == MCliststylestrings[i])
				{
					t_new_list_style = i;
					break;
				}

			if (t_new_list_style == -1)
			{
				MCeerror -> add(EE_FIELD_BADLISTSTYLE, 0, 0);
				return ES_ERROR;
			}

			setliststyle(t_new_list_style);
		}
		break;
	case P_LIST_DEPTH:
		{
			uint16_t t_depth;
			if (!MCU_stoui2(ep . getsvalue(), t_depth) || t_depth < 1 || t_depth > 16)
			{
				MCeerror -> add(EE_FIELD_BADLISTDEPTH, 0, 0);
				return ES_ERROR;
			}

			if (attrs == nil)
				attrs = new MCParagraphAttrs;

			if ((attrs -> flags & PA_HAS_LIST_STYLE) == 0)
			{
				attrs -> flags |= PA_HAS_LIST_STYLE;
				attrs -> list_style = kMCParagraphListStyleDisc;
			}

			attrs -> list_depth = t_depth - 1;
		}
		break;
	case P_LIST_INDENT:
		if (!setparagraphattr_int16(ep, attrs, PA_HAS_LIST_INDENT, offsetof(MCParagraphAttrs, first_indent), EE_FIELD_LISTINDENTNAN))
			return ES_ERROR;

		if (attrs != nil && (attrs -> flags & PA_HAS_LIST_INDENT) != 0)
		{
			attrs -> flags &= ~PA_HAS_FIRST_INDENT;

			if ((attrs -> flags & PA_HAS_LIST_STYLE) == 0)
			{
				attrs -> flags |= PA_HAS_LIST_STYLE;
				attrs -> list_style = kMCParagraphListStyleDisc;
				attrs -> list_depth = 0;
			}
		}
		break;
	// MW-2012-11-13: [[ ParaListIndex ]] Set the listIndex property.
	case P_LIST_INDEX:
		if (!setparagraphattr_int<uint16_t, 1, 65535>(ep, attrs, PA_HAS_LIST_INDEX, offsetof(MCParagraphAttrs, list_index), EE_FIELD_LISTINDEXNAN))
			return ES_ERROR;
		break;
	case P_FIRST_INDENT:
		if (!setparagraphattr_int16(ep, attrs, PA_HAS_FIRST_INDENT, offsetof(MCParagraphAttrs, first_indent), EE_FIELD_FIRSTINDENTNAN))
			return ES_ERROR;

		if (attrs != nil && (attrs -> flags & PA_HAS_FIRST_INDENT) != 0)
			attrs -> flags &= ~PA_HAS_LIST_INDENT;
		break;
	case P_LEFT_INDENT:
		if (!setparagraphattr_int16(ep, attrs, PA_HAS_LEFT_INDENT, offsetof(MCParagraphAttrs, left_indent), EE_FIELD_LEFTINDENTNAN))
			return ES_ERROR;
		break;
	case P_RIGHT_INDENT:
		if (!setparagraphattr_int16(ep, attrs, PA_HAS_RIGHT_INDENT, offsetof(MCParagraphAttrs, right_indent), EE_FIELD_RIGHTINDENTNAN))
			return ES_ERROR;
		break;
	case P_SPACE_ABOVE:
		if (!setparagraphattr_int<int16_t, 0, 32767>(ep, attrs, PA_HAS_SPACE_ABOVE, offsetof(MCParagraphAttrs, space_above), EE_FIELD_SPACEBEFORENAN))
			return ES_ERROR;
		break;
	case P_SPACE_BELOW:
		if (!setparagraphattr_int<int16_t, 0, 32767>(ep, attrs, PA_HAS_SPACE_BELOW, offsetof(MCParagraphAttrs, space_below), EE_FIELD_SPACEAFTERNAN))
			return ES_ERROR;
		break;
	// MW-2012-02-11: [[ TabWidths ]] Handle the new tabWidths property.
	case P_TAB_WIDTHS:
	case P_TAB_STOPS:
		{
			if (ep . isempty())
			{
				if (attrs == nil)
					break;

				attrs -> flags &= ~PA_HAS_TABS;
				attrs -> tab_count = 0;
				delete attrs -> tabs;
				break;
			}

			uint16_t *t_new_tabs;
			uint16_t t_new_tab_count;
            MCAutoStringRef t_value;
            ep . copyasstringref(&t_value);
			if (!MCField::parsetabstops(which, *t_value, t_new_tabs, t_new_tab_count))
				return ES_ERROR;

			if (attrs == nil)
				attrs = new MCParagraphAttrs;

			attrs -> flags |= PA_HAS_TABS;
			delete attrs -> tabs;
			attrs -> tab_count = t_new_tab_count;
			attrs -> tabs = t_new_tabs;
		}
		break;
	case P_BACK_COLOR:
		if (!setparagraphattr_color(ep, attrs, PA_HAS_BACKGROUND_COLOR, offsetof(MCParagraphAttrs, background_color)))
			return ES_ERROR;
		break;
	case P_BORDER_COLOR:
		if (!setparagraphattr_color(ep, attrs, PA_HAS_BORDER_COLOR, offsetof(MCParagraphAttrs, border_color)))
			return ES_ERROR;
		break;
	case P_BORDER_WIDTH:
		if (!setparagraphattr_uint8(ep, attrs, PA_HAS_BORDER_WIDTH, offsetof(MCParagraphAttrs, border_width), EE_FIELD_BORDERWIDTHNAN))
			return ES_ERROR;
		break;
	case P_PADDING:
		if (!setparagraphattr_uint8(ep, attrs, PA_HAS_PADDING, offsetof(MCParagraphAttrs, padding), EE_FIELD_PADDINGNAN))
			return ES_ERROR;
		break;
	case P_HGRID:
		{
			bool t_new_value;
			if (!setparagraphattr_bool(ep, attrs, PA_HAS_HGRID, t_new_value))
				return ES_ERROR;
			attrs -> hgrid = t_new_value;
		}
		break;
	case P_VGRID:
		{
			bool t_new_value;
			if (!setparagraphattr_bool(ep, attrs, PA_HAS_VGRID, t_new_value))
				return ES_ERROR;
			attrs -> vgrid = t_new_value;
		}
		break;
	case P_DONT_WRAP:
		{
			bool t_new_value;
			if (!setparagraphattr_bool(ep, attrs, PA_HAS_DONT_WRAP, t_new_value))
				return ES_ERROR;
			attrs -> dont_wrap = t_new_value;
		}
		break;
	// MW-2012-03-05: [[ HiddenText ]] Set the 'hidden' property. Notice that if the
	//   setting becomes false, we unset the 'has_hidden' flag thus allowing the attrs
	//   to be freed if its the only setting.
	case P_INVISIBLE:
		{
			bool t_new_value;
			if (!setparagraphattr_bool(ep, attrs, PA_HAS_HIDDEN, t_new_value))
				return ES_ERROR;
			attrs -> hidden = t_new_value;
			if (!t_new_value)
				attrs -> flags &= ~PA_HAS_HIDDEN;
		}
		break;
	// MW-2012-11-13: [[ ParaMetadata ]] Set the metadata attribute.
	case P_METADATA:
		if (ep . isempty())
		{
			if (attrs == nil)
				break;

			attrs -> flags &= ~PA_HAS_METADATA;
			MCNameDelete(attrs -> metadata);
			attrs -> metadata = nil;
		}

		if (attrs == nil)
			attrs = new MCParagraphAttrs;

		attrs -> flags |= PA_HAS_METADATA;
		MCNameDelete(attrs -> metadata);
		/* UNCHECKED */ ep . copyasnameref(attrs -> metadata);
		break;
#endif
	}

	if (attrs != nil && attrs -> flags == 0)
		clearattrs();

	return ES_NORMAL;
}
#endif

#ifdef LEGACY_EXEC
Exec_stat MCParagraph::getparagraphattr(Properties which, MCExecPoint& ep, Boolean effective)
{
	ep . clear();


	switch(which)
	{
	case P_TEXT_ALIGN:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_TEXT_ALIGN) == 0))
			return ES_NORMAL;
		MCF_unparsetextatts(P_TEXT_ALIGN, ep, gettextalign() << F_ALIGNMENT_SHIFT, nil, 0, 0, 0);
		break;
	case P_LIST_STYLE:
		if (attrs == nil || (attrs -> flags & PA_HAS_LIST_STYLE) == 0)
			return ES_NORMAL;
		ep . setstaticcstring(MCliststylestrings[getliststyle()]);
		break;
	case P_LIST_DEPTH:
		if (attrs == nil || (attrs -> flags & PA_HAS_LIST_STYLE) == 0)
			return ES_NORMAL;
		ep . setuint(attrs -> list_depth + 1);
		break;
	case P_LIST_INDENT:
		// MW-2012-02-27: [[ Bug ]] Crash when evaluating listIndent if its not been set.
		if (attrs == nil || (attrs -> flags & PA_HAS_LIST_INDENT) == 0)
			return ES_NORMAL;
		ep . setint(attrs -> first_indent);
		break;
	case P_LIST_INDEX:
		// MW-2012-11-13: [[ ParaListIndex ]] Fetch the list index of the paragraph.
		if (!effective)
		{
			if (attrs == nil || (attrs -> flags & PA_HAS_LIST_INDEX) == 0)
				return ES_NORMAL;
			ep . setint(attrs -> list_index);
		}
		else
		{
			uint32_t t_index;
			computelistindex(getparent() -> getparagraphs() -> prev(), t_index);
			ep . setint(t_index);
		}
		break;
	case P_FIRST_INDENT:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_FIRST_INDENT) == 0))
			return ES_NORMAL;
		ep . setint(getfirstindent());
		break;
	case P_LEFT_INDENT:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_LEFT_INDENT) == 0))
			return ES_NORMAL;
		ep . setint(getleftindent());
		break;
	case P_RIGHT_INDENT:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_RIGHT_INDENT) == 0))
			return ES_NORMAL;
		ep . setint(getrightindent());
		break;
	case P_SPACE_ABOVE:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_SPACE_ABOVE) == 0))
			return ES_NORMAL;
		ep . setint(getspaceabove());
		break;
	case P_SPACE_BELOW:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_SPACE_BELOW) == 0))
			return ES_NORMAL;
		ep . setint(getspacebelow());
		break;
	case P_TAB_WIDTHS:
	case P_TAB_STOPS:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_TABS) == 0))
			return ES_NORMAL;
		{
			uint16_t *t_tabs;
			uint16_t t_tab_count;
			Boolean t_fixed;
			gettabs(t_tabs, t_tab_count, t_fixed);
			MCField::formattabstops(which, ep, t_tabs, t_tab_count);
		}
		break;
	case P_BACK_COLOR:
		if (attrs == nil || (attrs -> flags & PA_HAS_BACKGROUND_COLOR) == 0)
			return ES_NORMAL;
		ep . setpixel(attrs -> background_color);
		break;
	case P_BORDER_COLOR:
		if (attrs == nil || (attrs -> flags & PA_HAS_BORDER_COLOR) == 0)
			return ES_NORMAL;
		ep . setpixel(attrs -> border_color);
		break;
	case P_BORDER_WIDTH:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_BORDER_WIDTH) == 0))
			return ES_NORMAL;
		ep . setuint(getborderwidth());
		break;
	case P_PADDING:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_PADDING) == 0))
			return ES_NORMAL;
		ep . setuint(getpadding());
		break;
	case P_HGRID:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective - notice
		//   that we use the attr value in non-effective mode, as 'gethgrid()' takes
		//   into account other propeties.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_HGRID) == 0))
			return ES_NORMAL;
		ep . setboolean(effective ? gethgrid() : attrs -> hgrid);
		break;
	case P_VGRID:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective - notice
		//   that we use the attr value in non-effective mode, as 'getvgrid()' takes
		//   into account other propeties.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_VGRID) == 0))
			return ES_NORMAL;
		ep . setboolean(effective ? getvgrid() : attrs -> vgrid);
		break;
	case P_DONT_WRAP:
		// MW-2012-02-13: [[ ParaStyles ]] Added support for effective adjective - notice
		//   that we use the attr value in non-effective mode, as 'getdontwrap()' takes
		//   into account other propeties.
		if (!effective && (attrs == nil || (attrs -> flags & PA_HAS_DONT_WRAP) == 0))
			return ES_NORMAL;
		ep . setboolean(effective ? getdontwrap() : attrs -> dont_wrap);
		break;
	// MW-2012-03-05: [[ HiddenText ]] Add support for the 'hidden' property. If there are no
	//   attrs, the hidden is false.
	case P_INVISIBLE:
		ep . setboolean(attrs != nil ? attrs -> hidden : false);
		break;
	// MW-2012-11-13: [[ ParaMetadata ]] Add support for the 'metadata' property.
	case P_METADATA:
		if (attrs != nil && (attrs -> flags & PA_HAS_METADATA) != 0)
			ep . setvalueref_nullable(attrs -> metadata);
		else
			ep . clear();
		break;
	}

	return ES_NORMAL;
}
#endif

template<typename T> static void copysingleattr_int(MCParagraphAttrs *other_attrs, MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset)
{
	if (other_attrs == nil || (other_attrs -> flags & p_flag) == 0)
	{
		if (attrs == nil)
			return;

		attrs -> flags &= ~p_flag;
		((T *)((char *)attrs + p_field_offset))[0] = 0;
	}
	else
	{
		if (attrs == nil)
			attrs = new MCParagraphAttrs;
		attrs -> flags |= p_flag;
		((T *)((char *)attrs + p_field_offset))[0] = ((T *)((char *)other_attrs + p_field_offset))[0];
	}
}

static bool copysingleattr_bool(MCParagraphAttrs *other_attrs, MCParagraphAttrs*& attrs, uint32_t p_flag)
{
	if (other_attrs == nil || (other_attrs -> flags & p_flag) == 0)
	{
		if (attrs == nil)
			return false;

		attrs -> flags &= ~p_flag;
		return false;
	}

	if (attrs == nil)
		attrs = new MCParagraphAttrs;
	attrs -> flags |= p_flag;
	return true;
}

static void copysingleattr_int8(MCParagraphAttrs *other_attrs, MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset)
{
	copysingleattr_int<int8_t>(other_attrs, attrs, p_flag, p_field_offset);
}

static void copysingleattr_int16(MCParagraphAttrs *other_attrs, MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset)
{
	copysingleattr_int<int16_t>(other_attrs, attrs, p_flag, p_field_offset);
}

static void copysingleattr_uint16(MCParagraphAttrs *other_attrs, MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset)
{
	copysingleattr_int<uint16_t>(other_attrs, attrs, p_flag, p_field_offset);
}

static void copysingleattr_int32(MCParagraphAttrs *other_attrs, MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset)
{
	copysingleattr_int<int32_t>(other_attrs, attrs, p_flag, p_field_offset);
}

void MCParagraph::copysingleattr(Properties which, MCParagraph *other)
{
	switch(which)
	{
#ifdef OLD_EXEC
	case P_TEXT_ALIGN:
		if (other -> attrs == nil || (other -> attrs -> flags & PA_HAS_TEXT_ALIGN) == 0)
		{
			if (attrs != nil)
			{
				attrs -> flags &= ~PA_HAS_TEXT_ALIGN;
				attrs -> text_align = kMCParagraphTextAlignLeft;
			}
		}
		else
		{
			if (attrs == nil)
				attrs = new MCParagraphAttrs;
			attrs -> flags |= PA_HAS_TEXT_ALIGN;
			attrs -> text_align = other -> attrs -> text_align;
		}
		break;
	case P_LIST_STYLE:
		if (other -> attrs == nil || (other -> attrs -> flags & PA_HAS_LIST_STYLE) == 0)
		{
			if (attrs != nil)
			{
				attrs -> flags &= ~PA_HAS_LIST_STYLE;
				attrs -> list_style = kMCParagraphListStyleNone;
				attrs -> list_depth = 0;
			}
		}
		else
		{
			if (attrs == nil)
				attrs = new MCParagraphAttrs;
			attrs -> flags |= PA_HAS_LIST_STYLE;
			attrs -> list_style = other -> attrs -> list_style;
		}
		break;
	case P_LIST_DEPTH:
		if (other -> attrs == nil || (other -> attrs -> flags & PA_HAS_LIST_STYLE) == 0)
		{
			if (attrs != nil)
			{
				attrs -> flags &= ~PA_HAS_LIST_STYLE;
				attrs -> list_style = kMCParagraphListStyleNone;
				attrs -> list_depth = 0;
			}
		}
		else
		{
			if (attrs == nil)
				attrs = new MCParagraphAttrs;
			if ((attrs -> flags & PA_HAS_LIST_STYLE) == 0)
			{
				attrs -> flags |= PA_HAS_LIST_STYLE;
				attrs -> list_style = kMCParagraphListStyleDisc;
			}
			attrs -> list_depth = other -> attrs -> list_depth;
		}
		break;
	case P_LIST_INDENT:
		if (other -> attrs == nil || (other -> attrs -> flags & PA_HAS_LIST_INDENT) == 0)
		{
			if (attrs != nil)
			{
				attrs -> flags &= ~PA_HAS_LIST_INDENT;
				if ((attrs -> flags & PA_HAS_FIRST_INDENT) == 0)
					attrs -> first_indent = 0;
			}
		}
		else
		{
			if (attrs == nil)
				attrs = new MCParagraphAttrs;
			attrs -> flags |= PA_HAS_LIST_INDENT;
			attrs -> flags &= ~PA_HAS_FIRST_INDENT;
			attrs -> first_indent = other -> attrs -> first_indent;
		}
		break;
	// MW-2012-11-13: [[ ParaListIndex ]] Make sure we copy the list index.
	case P_LIST_INDEX:
		copysingleattr_uint16(other -> attrs, attrs, PA_HAS_LIST_INDEX, offsetof(MCParagraphAttrs, list_index));
		break;
	case P_FIRST_INDENT:
		copysingleattr_int16(other -> attrs, attrs, PA_HAS_FIRST_INDENT, offsetof(MCParagraphAttrs, first_indent));
		break;
	case P_LEFT_INDENT:
		copysingleattr_int16(other -> attrs, attrs, PA_HAS_LEFT_INDENT, offsetof(MCParagraphAttrs, left_indent));
		break;
	case P_RIGHT_INDENT:
		copysingleattr_int16(other -> attrs, attrs, PA_HAS_RIGHT_INDENT, offsetof(MCParagraphAttrs, right_indent));
		break;
	case P_SPACE_ABOVE:
		copysingleattr_int16(other -> attrs, attrs, PA_HAS_SPACE_ABOVE, offsetof(MCParagraphAttrs, space_above));
		break;
	case P_SPACE_BELOW:
		copysingleattr_int16(other -> attrs, attrs, PA_HAS_SPACE_BELOW, offsetof(MCParagraphAttrs, space_below));
		break;
	// MW-2012-02-11: [[ TabWidths ]] There's no difference between copying tabWidths or
	//   tabStops as tabWidths is synthetic (derived from tabStops).
	case P_TAB_WIDTHS:
	case P_TAB_STOPS:
		{
			if (attrs != nil && (attrs -> flags & PA_HAS_TABS) != 0)
			{
				delete attrs -> tabs;
				attrs -> tabs = nil;
				attrs -> tab_count = 0;
				attrs -> flags &= ~PA_HAS_TABS;
			}
			
			if (other -> attrs != nil && (other -> attrs -> flags & PA_HAS_TABS) != 0)
			{
				if (attrs == nil)
					attrs = new MCParagraphAttrs;
				
				attrs -> tabs = new uint2[other -> attrs -> tab_count];
				memcpy(attrs -> tabs, other -> attrs -> tabs, other -> attrs -> tab_count * sizeof(uint2));
				attrs -> tab_count = other -> attrs -> tab_count;
				attrs -> flags |= PA_HAS_TABS;
			}
		}
		break;
	case P_BACK_COLOR:
		copysingleattr_int32(other -> attrs, attrs, PA_HAS_BACKGROUND_COLOR, offsetof(MCParagraphAttrs, background_color));
		break;
	case P_BORDER_COLOR:
		copysingleattr_int32(other -> attrs, attrs, PA_HAS_BORDER_COLOR, offsetof(MCParagraphAttrs, border_color));
		break;
	case P_BORDER_WIDTH:
		copysingleattr_int8(other -> attrs, attrs, PA_HAS_BORDER_WIDTH, offsetof(MCParagraphAttrs, border_width));
		break;
	case P_PADDING:
		copysingleattr_int8(other -> attrs, attrs, PA_HAS_PADDING, offsetof(MCParagraphAttrs, padding));
		break;
	case P_HGRID:
		{
			if (copysingleattr_bool(other -> attrs, attrs, PA_HAS_HGRID))
				attrs -> hgrid = other -> attrs -> hgrid;
			else if (attrs != nil)
				attrs -> hgrid = false;
		}
		break;
	case P_VGRID:
		{
			if (copysingleattr_bool(other -> attrs, attrs, PA_HAS_VGRID))
				attrs -> vgrid = other -> attrs -> vgrid;
			else if (attrs != nil)
				attrs -> vgrid = false;
		}
		break;
	case P_DONT_WRAP:
		{
			if (copysingleattr_bool(other -> attrs, attrs, PA_HAS_DONT_WRAP))
				attrs -> dont_wrap = other -> attrs -> dont_wrap;
			else if (attrs != nil)
				attrs -> dont_wrap = false;
		}
		break;
	// MW-2012-03-05: [[ HiddenText ]] Copy across the hidden property. Notice that we unset
	//   the flag if the value ends up being false.
	case P_INVISIBLE:
		{
			if (copysingleattr_bool(other -> attrs, attrs, PA_HAS_HIDDEN))
				attrs -> hidden = other -> attrs -> hidden;
			else if (attrs != nil)
				attrs -> hidden = false;
			// MW-2013-08-20: [[ Bug 11108 ]] If we don't have attributes, then don't tweak
			//   the flags.
			if (attrs != nil && !attrs -> hidden)
				attrs -> flags &= ~PA_HAS_HIDDEN;
		}
		break;
	// MW-2012-11-13: [[ ParaMetadata ]] Make sure we copy the metadata.
	case P_METADATA:
		if (attrs != nil && (attrs -> flags & PA_HAS_METADATA) != 0)
		{
			MCNameDelete(attrs -> metadata);
			attrs -> metadata = nil;
			attrs -> flags &= ~PA_HAS_METADATA;
		}
		
		if (other -> attrs != nil && (other -> attrs -> flags & PA_HAS_METADATA) != 0)
		{
			if (attrs == nil)
				attrs = new MCParagraphAttrs;
			
			MCNameClone(other -> attrs -> metadata, attrs -> metadata);
			attrs -> flags |= PA_HAS_METADATA;
		}
		break;
#endif
	}

	if (attrs == nil)
		return;

	if (attrs -> flags == 0)
		clearattrs();
}

void MCParagraph::copyattrs(const MCParagraph& other)
{
	// Get rid of any attrs we have.
	clearattrs();

	// If the other side has no attrs, we are done.
	if (other . attrs == nil)
		return;

	// Make a new attrs structure.
	attrs = new MCParagraphAttrs;
	
	// Copy across all the fields byte-wise.
	memcpy(attrs, other . attrs, sizeof(MCParagraphAttrs));

	// If the struct has tabs, then copy them properly.
	if ((other . attrs -> flags & PA_HAS_TABS) != 0)
	{
		attrs -> tabs = new uint16_t[other . attrs -> tab_count];
		memcpy(attrs -> tabs, other . attrs -> tabs, sizeof(uint16_t) * other . attrs -> tab_count);
	}

	// MW-2012-12-04: [[ Bug 10577 ]] If the struct has metadata, then copy it properly.
	if ((other . attrs -> flags & PA_HAS_METADATA) != 0)
        /* UNCHECKED */ MCStringCopy(other . attrs -> metadata, attrs -> metadata);
}

void MCParagraph::clearattrs(void)
{
	// If we have no attrs, we are done.
	if (attrs == nil)
		return;

	// If we have tabs, then delete them.
	if ((attrs -> flags & PA_HAS_TABS) != 0)
		delete attrs -> tabs;

	// MW-2012-11-13: [[ ParaMetadata ]] If we have metadata, delete it.
	if ((attrs -> flags & PA_HAS_METADATA) != 0)
        MCValueRelease(attrs -> metadata);

	// Delete the structure.
	delete attrs;

	// Remove dangling reference.
	attrs = nil;
}

// MW-2012-02-21: [[ FieldExport ]] Extract the current paragraph styles of this
//   object into the given struct.
void MCParagraph::exportattrs(MCFieldParagraphStyle& x_style)
{
	if (attrs == nil)
		return;

	if ((attrs -> flags & PA_HAS_TEXT_ALIGN) != 0)
	{
		x_style . has_text_align = true;
		x_style . text_align = attrs -> text_align;
	}
	if ((attrs -> flags & PA_HAS_LIST_STYLE) != 0)
	{
		x_style . has_list_style = true;
		x_style . list_style = attrs -> list_style;
		x_style . list_depth = attrs -> list_depth;
	}
	if ((attrs -> flags & PA_HAS_FIRST_INDENT) != 0)
	{
		x_style . has_first_indent = true;
		x_style . first_indent = attrs -> first_indent;
	}
	if ((attrs -> flags & PA_HAS_LEFT_INDENT) != 0)
	{
		x_style . has_left_indent = true;
		x_style . left_indent = attrs -> left_indent;
	}
	if ((attrs -> flags & PA_HAS_RIGHT_INDENT) != 0)
	{
		x_style . has_right_indent = true;
		x_style . right_indent = attrs -> right_indent;
	}
	if ((attrs -> flags & PA_HAS_SPACE_ABOVE) != 0)
	{
		x_style . has_space_above = true;
		x_style . space_above = attrs -> space_above;
	}
	if ((attrs -> flags & PA_HAS_SPACE_BELOW) != 0)
	{
		x_style . has_space_below = true;
		x_style . space_below = attrs -> space_below;
	}
	if ((attrs -> flags & PA_HAS_TABS) != 0)
	{
		x_style . has_tabs = true;	
		x_style . tabs = attrs -> tabs;
		x_style . tab_count = attrs -> tab_count;
	}
	if ((attrs -> flags & PA_HAS_BACKGROUND_COLOR) != 0)
	{
		x_style . has_background_color = true;
		x_style . background_color = attrs -> background_color;
	}
	if ((attrs -> flags & PA_HAS_BORDER_WIDTH) != 0)
	{
		x_style . has_border_width = true;
		x_style . border_width = attrs -> border_width;
	}
	if ((attrs -> flags & PA_HAS_LIST_INDENT) != 0)
	{
		x_style . has_list_indent = true;
		x_style . list_indent = MCAbs(attrs -> first_indent);
	}
	if ((attrs -> flags & PA_HAS_HGRID) != 0)
	{
		x_style . has_hgrid = true;
		x_style . hgrid = attrs -> hgrid;
	}
	if ((attrs -> flags & PA_HAS_VGRID) != 0)
	{
		x_style . has_vgrid = true;
		x_style . vgrid = attrs -> vgrid;
	}
	if ((attrs -> flags & PA_HAS_BORDER_COLOR) != 0)
	{
		x_style . has_border_color = true;
		x_style . border_color = attrs -> border_color;
	}
	if ((attrs -> flags & PA_HAS_DONT_WRAP) != 0)
	{
		x_style . has_dont_wrap = true;
		x_style . dont_wrap = attrs -> dont_wrap;
	}
	if ((attrs -> flags & PA_HAS_PADDING) != 0)
	{
		x_style . has_padding = true;
		x_style . padding = attrs -> padding;
	}
	// MW-2012-03-07: [[ HiddenText ]] Make sure the 'hidden' attribute is set.
	if ((attrs -> flags & PA_HAS_HIDDEN) != 0)
		x_style . hidden = attrs -> hidden;
	else
		x_style . hidden = false;
	// MW-2012-11-13: [[ ParaMetadata ]] Fetch the metadata attr.
	if ((attrs -> flags & PA_HAS_METADATA) != 0)
	{
		x_style . has_metadata = true;
		x_style . metadata = attrs -> metadata;
	}
	// MW-2012-11-13: [[ ParaListIndex ]] Fetch the list index.
	if ((attrs -> flags & PA_HAS_LIST_INDEX) != 0)
	{
		x_style . has_list_index = true;
		x_style . list_index = attrs -> list_index;
	}
}

// MW-2012-03-04: [[ FieldImport ]] Set the attributes of the paragraph to those
//   described by the style.
void MCParagraph::importattrs(const MCFieldParagraphStyle& p_style)
{
	clearattrs();
	
	attrs = new MCParagraphAttrs;
	if (p_style . has_text_align)
	{
		attrs -> flags |= PA_HAS_TEXT_ALIGN;
		attrs -> text_align = p_style . text_align;
	}
	if (p_style . has_list_style)
	{
		attrs -> flags |= PA_HAS_LIST_STYLE;
		attrs -> list_style = p_style . list_style;
		attrs -> list_depth = p_style . list_depth;
	}
	if (p_style . has_first_indent)
	{
		attrs -> flags |= PA_HAS_FIRST_INDENT;
		attrs -> first_indent = p_style . first_indent;
	}
	if (p_style . has_left_indent)
	{
		attrs -> flags |= PA_HAS_LEFT_INDENT;
		attrs -> left_indent = p_style . left_indent;
	}
	if (p_style . has_right_indent)
	{
		attrs -> flags |= PA_HAS_RIGHT_INDENT;
		attrs -> right_indent = p_style . right_indent;
	}
	if (p_style . has_space_above)
	{
		attrs -> flags |= PA_HAS_SPACE_ABOVE;
		attrs -> space_above = p_style . space_above;
	}
	if (p_style . has_space_below)
	{
		attrs -> flags |= PA_HAS_SPACE_BELOW;
		attrs -> space_below = p_style . space_below;
	}
	if (p_style . has_tabs)
	{
		attrs -> flags |= PA_HAS_TABS;
		attrs -> tab_count = p_style . tab_count;
		attrs -> tabs = new uint16_t[p_style . tab_count];
		memcpy(attrs -> tabs, p_style . tabs, sizeof(uint16_t) * p_style . tab_count);
	}
	if (p_style . has_background_color)
	{
		attrs -> flags |= PA_HAS_BACKGROUND_COLOR;
		attrs -> background_color = p_style . background_color;
	}
	if (p_style . has_border_width)
	{
		attrs -> flags |= PA_HAS_BORDER_WIDTH;
		attrs -> border_width = p_style . border_width;
	}
	if (p_style . has_list_indent)
	{
		attrs -> flags |= PA_HAS_LIST_INDENT;
		attrs -> first_indent = p_style . list_indent;
	}
	if (p_style . has_hgrid)
	{
		attrs -> flags |= PA_HAS_HGRID;
		attrs -> hgrid = p_style . hgrid;
	}
	if (p_style . has_vgrid)
	{
		attrs -> flags |= PA_HAS_VGRID;
		attrs -> vgrid = p_style . vgrid;
	}
	if (p_style . has_border_color)
	{
		attrs -> flags |= PA_HAS_BORDER_COLOR;
		attrs -> border_color = p_style . border_color;
	}
	if (p_style . has_dont_wrap)
	{
		attrs -> flags |= PA_HAS_DONT_WRAP;
		attrs -> dont_wrap = p_style . dont_wrap;
	}
	if (p_style . has_padding)
	{
		attrs -> flags |= PA_HAS_PADDING;
		attrs -> padding = p_style . padding;
	}
	if (p_style . hidden)
	{
		attrs -> flags |= PA_HAS_HIDDEN;
		attrs -> hidden = true;
	}
	// MW-2012-11-13: [[ ParaMetadata ]] Import the metadata setting.
	if (p_style . has_metadata)
	{
		attrs -> flags |= PA_HAS_METADATA;
        /* UNCHECKED */ MCStringCopy(p_style . metadata, attrs -> metadata);
	}
	// MW-2012-11-13: [[ ParaListIndex ]] Import the listIndex setting.
	if (p_style . has_list_index)
	{
		attrs -> flags |= PA_HAS_LIST_INDEX;
		attrs -> list_index = p_style . list_index;
	}
	
	if (attrs -> flags == 0)
	{
		delete attrs;
		attrs = nil;
	}
}

void MCParagraph::setliststyle(uint32_t p_new_list_style)
{
	if (p_new_list_style == kMCParagraphListStyleNone)
	{
		if (attrs == nil)
			return;

		attrs -> flags &= ~PA_HAS_LIST_STYLE;
		attrs -> list_style = kMCParagraphListStyleNone;
		attrs -> list_depth = 0;
	}
	else
	{
		if (attrs == nil)
			attrs = new MCParagraphAttrs;
		if ((attrs -> flags & PA_HAS_LIST_STYLE) == 0)
		{
			attrs -> flags |= PA_HAS_LIST_STYLE;
			attrs -> list_depth = 0;
		}
		attrs -> list_style = p_new_list_style;
	}
}

void MCParagraph::setmetadata(MCStringRef p_metadata)
{
	if (attrs != nil && (attrs -> flags & PA_HAS_METADATA) != 0)
	{
		attrs -> flags &= ~PA_HAS_METADATA;
        MCValueRelease(attrs -> metadata);
		attrs -> metadata = nil;
	}

    if (p_metadata == nil || MCStringIsEmpty(p_metadata))
		return;

	if (attrs == nil)
		attrs = new MCParagraphAttrs;
    attrs -> flags |= PA_HAS_METADATA;
    /* UNCHECKED */ MCValueInter(p_metadata, attrs -> metadata);
}

void MCParagraph::setlistindex(uint32_t p_new_list_index)
{
	if (p_new_list_index == 0)
	{
		if (attrs == nil)
			return;

		attrs -> flags &= ~PA_HAS_LIST_INDEX;
		attrs -> list_index = 0;
	}
	else
	{
		if (attrs == nil)
			attrs = new MCParagraphAttrs;

		attrs -> flags |= PA_HAS_LIST_INDEX;
		attrs -> list_index = p_new_list_index;
	}
}

////////////////////////////////////////////////////////////////////////////////

uint32_t MCParagraph::gettextalign(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_TEXT_ALIGN) != 0)
		return attrs -> text_align;
	if (parent != nil)
		return (parent -> getflags() & F_ALIGNMENT) >> F_ALIGNMENT_SHIFT;
	return kMCParagraphTextAlignLeft;
}

uint32_t MCParagraph::getliststyle(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_LIST_STYLE) != 0)
		return attrs -> list_style;
	return kMCParagraphListStyleNone;
}

bool MCParagraph::haslistindex(void) const
{
	return attrs != nil && (attrs -> flags & PA_HAS_LIST_INDEX) != 0;
}

uint32_t MCParagraph::getlistindex(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_LIST_INDEX) != 0)
		return attrs -> list_index;
	return 0;
}

uint32_t MCParagraph::getlistdepth(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_LIST_STYLE) != 0)
		return attrs -> list_depth;
	return 0;
}

int32_t MCParagraph::getspaceabove(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_SPACE_ABOVE) != 0)
		return attrs -> space_above;
	return 0;
}

int32_t MCParagraph::getspacebelow(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_SPACE_BELOW) != 0)
		return attrs -> space_below;
	return 0;
}

int32_t MCParagraph::getborderwidth(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_BORDER_WIDTH) != 0)
		return attrs -> border_width;
	return 0;
}

int32_t MCParagraph::getpadding(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_PADDING) != 0)
		return attrs -> padding;
	return 0;
}

// MW-2012-03-19: [[ Bug 10069 ]] The horizontal padding is a function of the leftMargin
//   of the field if the padding and vgrid props are not set on the paragraph but vgrid
//   is set on the field.
int32_t MCParagraph::gethpadding(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_PADDING) != 0)
		return attrs -> padding;
	if ((attrs == nil || (attrs -> flags & PA_HAS_VGRID) == 0) && parent -> getflag(F_VGRID))
		return MCU_max(0, parent -> getleftmargin() - 3);
	return 0;
}

// MW-2012-03-19: [[ Bug 10069 ]] The vertical padding always defaults to zero.
int32_t MCParagraph::getvpadding(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_PADDING) != 0)
		return attrs -> padding;
	return 0;
}

int32_t MCParagraph::getfirstindent(void) const
{
	if (getvgrid())
		return 0;
	if (attrs != nil && (attrs -> flags & PA_HAS_FIRST_INDENT) != 0)
		return attrs -> first_indent;
	return parent -> getfirstindent();
}

int32_t MCParagraph::getleftindent(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_LEFT_INDENT) != 0)
		return attrs -> left_indent;
	return 0;
}

int32_t MCParagraph::getrightindent(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_RIGHT_INDENT) != 0)
		return attrs -> right_indent;
	return 0;
}

int32_t MCParagraph::getlistindent(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_LIST_INDENT) != 0)
		return MCAbs(attrs -> first_indent);

    // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
    return 8 * MCFontMeasureText(parent -> getfontref(), MCSTR(" "), parent -> getstack() -> getdevicetransform());
}

void MCParagraph::gettabs(uint16_t*& r_tabs, uint16_t& r_tab_count, Boolean& r_fixed) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_TABS) != 0)
	{
		r_tabs = attrs -> tabs;
		r_tab_count = attrs -> tab_count;
	}
	else
		parent -> gettabs(r_tabs, r_tab_count, r_fixed);

	r_fixed = getvgrid();
}

void MCParagraph::gettabaligns(intenum_t*& r_tab_aligns, uint16_t& r_tab_count) const
{
    if (attrs != nil && (attrs -> flags & PA_HAS_TAB_ALIGNMENTS) != 0)
    {
        r_tab_aligns = attrs -> alignments;
        r_tab_count = attrs -> alignments_count;
    }
    else
        parent -> gettabaligns(r_tab_aligns, r_tab_count);
}

bool MCParagraph::getvgrid(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_LIST_STYLE) != 0)
		return false;
	if (attrs != nil && (attrs -> flags & PA_HAS_VGRID) != 0)
		return attrs -> vgrid;
	return (parent -> getflags() & F_VGRID) != 0;
}

bool MCParagraph::gethgrid(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_HGRID) != 0)
		return attrs -> hgrid;
	return (parent -> getflags() & F_HGRID) != 0;
}

bool MCParagraph::getdontwrap(void) const
{
	if (getvgrid())
		return true;

	if (attrs != nil && (attrs -> flags & PA_HAS_DONT_WRAP) != 0)
		return attrs -> dont_wrap;

	return (parent -> getflags() & F_DONT_WRAP) != 0;
}

// MW-2012-03-05: [[ HiddenText ]] Compute whether the paragraph is hidden or not.
//   If there are no attributes, its not hidden; otherwise take the value in the
//   attrs struct.
bool MCParagraph::gethidden(void) const
{
	if (attrs == nil)
		return false;
	return attrs -> hidden;
}

// MW-2012-02-10: [[ FixedTable ]] Compute the fixed table width (if there is one).
int32_t MCParagraph::gettablewidth(void) const
{
	if (getvgrid())
	{
		Boolean t_fixed;
		uint16_t *t_tabs;
		uint16_t t_tab_count;
		gettabs(t_tabs, t_tab_count, t_fixed);
		if (t_tab_count > 1 && t_tabs[t_tab_count - 1] == t_tabs[t_tab_count - 2])
			return t_tabs[t_tab_count - 1] - 1;
	}

	return 0;
}

MCStringRef MCParagraph::getmetadata(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_METADATA) && attrs -> metadata != nil)
		return attrs -> metadata;
    return kMCEmptyString;
}

////////////////////////////////////////////////////////////////////////////////

int32_t MCParagraph::getlistlabelwidth(void) const
{
	if (attrs != nil && (attrs -> flags & PA_HAS_FIRST_INDENT) != 0)
		return MCMax(-attrs -> first_indent, 0);

	return getlistindent();
}

void MCParagraph::computelayoutwidths(int32_t& r_normal_line, int32_t& r_first_line) const
{
	int32_t t_left_margin, t_right_margin;
	t_left_margin = computeleftmargin();
	t_right_margin = computerightmargin();
	if (getliststyle() != kMCParagraphListStyleNone)
	{
		r_normal_line = MCMax(parent -> getlayoutwidth() - t_left_margin - t_right_margin - getlistlabelwidth(), 1);
		r_first_line = r_normal_line;
	}
	else
	{
		int32_t t_first_indent;
		t_first_indent = getfirstindent();

		r_first_line = MCMax(parent -> getlayoutwidth() - t_left_margin - t_right_margin - MCMax(t_first_indent, 0), 1);
		r_normal_line = MCMax(parent -> getlayoutwidth() - t_left_margin - t_right_margin + MCMin(t_first_indent, 0), 1);
	}
}

int32_t MCParagraph::computeleftmargin(void) const
{
	int32_t t_left_margin;
	if (getliststyle() != kMCParagraphListStyleNone)
	{
		int32_t t_list_offset;
		if (attrs != nil && ((attrs -> flags & PA_HAS_FIRST_INDENT) != 0))
			t_list_offset = MCMin(attrs -> first_indent, 0);
		else
			t_list_offset = getlistindent() * getlistdepth();

		t_left_margin = getleftindent() + t_list_offset;
	}
	else
		t_left_margin = MCMin(getleftindent(), getleftindent() + getfirstindent());

	// MW-2012-02-09: [[ ParaStyles ]] Adjust the margin for any padding.
	// MW-2012-03-19: [[ Bug 10069 ]] Use the horizontal padding here.
	t_left_margin += getborderwidth() + gethpadding();

	return t_left_margin;
}

int32_t MCParagraph::computerightmargin(void) const
{
	// MW-2012-02-09: [[ ParaStyles ]] Adjust the margin for any padding.
	// MW-2012-03-19: [[ Bug 10069 ]] Use the horizontal padding here.
	return getrightindent() + getborderwidth() + gethpadding();
}

int32_t MCParagraph::computetopmargin(void) const
{
	// MW-2012-02-09: [[ ParaStyles ]] Adjust the margin for any padding.
	// MW-2012-03-19: [[ Bug 10069 ]] Use the vertical padding here.
	return getspaceabove() + computetopborder() + getvpadding();
}

int32_t MCParagraph::computebottommargin(void) const
{
	// MW-2012-02-09: [[ ParaStyles ]] Adjust the margin for any padding.
	// MW-2012-03-19: [[ Bug 10069 ]] Use the vertical padding here.
	return computebottomborder() + getspacebelow() + getvpadding();
}

bool MCParagraph::elidetopborder(void) const
{
	if (getspaceabove() == 0 &&
		this != parent -> getparagraphs() &&
		prev() -> getspacebelow() == 0 &&
		prev() -> getborderwidth() == getborderwidth() &&
		prev() -> gethgrid())
		return true;

	return false;
}

bool MCParagraph::elidebottomborder(void) const
{
	// MW-2012-03-15: [[ Bug 10069 ]] If the next para has hgrid and the parent
	//   doesn't then we must not elide as otherwise the grid line will not draw.
	if (getspacebelow() == 0 &&
		this != parent -> getparagraphs() -> prev() &&
		next() -> getspaceabove() == 0 &&
		next() -> getborderwidth() == getborderwidth() &&
		next() -> gethgrid() && parent -> getflag(F_HGRID) == False)
		return true;

	return false;
}

int32_t MCParagraph::computetopborder(void) const
{
	int32_t t_border;
	t_border = getborderwidth();
	if (gethgrid())
	{
		if (elidetopborder())
		{
			if (!parent -> getflag(F_FIXED_HEIGHT))
				return 1;
			return 0;
		}

		if (!parent -> getflag(F_FIXED_HEIGHT))
			t_border = MCMax(t_border, 1);
	}

	return t_border;
}

int32_t MCParagraph::computebottomborder(void) const
{
	int32_t t_border;
	t_border = getborderwidth();
	if (gethgrid())
	{
		if (elidebottomborder())
			return 0;

		if (!parent -> getflag(F_FIXED_HEIGHT))
			t_border = MCMax(t_border, 1);
	}

	return t_border;
}

void MCParagraph::computerects(int32_t x, int32_t y, int32_t p_layout_width, uint2 p_pg_width, uint2 p_pg_height, MCRectangle& r_outer, MCRectangle& r_inner) const
{
	int32_t t_left_margin, t_right_margin, t_top_margin, t_bottom_margin;
	t_left_margin = computeleftmargin();
	t_right_margin = computerightmargin();
	t_top_margin = computetopmargin();
	t_bottom_margin = computebottommargin();

	r_inner . x = x + t_left_margin;
	r_inner . width = MCMax(p_layout_width, p_pg_width) - t_left_margin - t_right_margin;
	r_inner . y = y + t_top_margin;
	r_inner . height = p_pg_height - t_top_margin - t_bottom_margin;

	r_outer = r_inner;
	r_outer . x -= getborderwidth();
	r_outer . width += 2 * getborderwidth();

	int32_t t_top_border, t_bottom_border;
	t_top_border = computetopborder();
	t_bottom_border = computebottomborder();

	// MW-2012-02-09: [[ ParaStyles ]] The inner rect includes padding, so remove it to
	//   compute outer rect.
	// MW-2012-03-19: [[ Bug 10069 ]] Use the appropriate h/v padding to reduce the rect.
	r_outer . x -= gethpadding();
	r_outer . width += 2 * gethpadding();
	r_outer . y -= getvpadding();
	r_outer . height += 2 * getvpadding();

	r_outer . y -= t_top_border;
	r_outer . height += t_top_border;
	r_outer . height += t_bottom_border;

	// MW-2012-02-16: [[ Bug 10001 ]] Adjust for alignment in the case that the paragraph
	//   is too wide.
	int32_t t_total_width;
	t_total_width = r_inner . width + t_left_margin + t_right_margin;
	if (t_total_width > p_layout_width)
	{
		int32_t t_offset;
		switch(gettextalign())
		{
		case kMCParagraphTextAlignLeft:
		case kMCParagraphTextAlignJustify:
			t_offset = 0;
			break;
		case kMCParagraphTextAlignCenter:
			t_offset = (p_layout_width - t_total_width) / 2;
			break;
		case kMCParagraphTextAlignRight:
			t_offset = p_layout_width - t_total_width;
			break;
		}
		r_inner . x += t_offset;
		r_outer . x += t_offset;
	}
}

void MCParagraph::adjustrectsfortable(MCRectangle& x_inner_rect, MCRectangle& x_outer_rect)
{
	// MW-2012-02-10: [[ FixedTable ]] Check to see if this paragraph is a fixed
	//   width table.
	int32_t t_table_width;
	t_table_width = gettablewidth();
	
	// MW-2012-02-10: [[ FixedTable ]] If we are in fixed table mode then adjust the
	//   rects appropriately to take this into account.
	if (t_table_width != 0)
	{
		int32_t t_offset;
		t_offset = computelineinneroffset(x_inner_rect . width, lines);
		x_outer_rect . width = t_table_width + (x_outer_rect . width - x_inner_rect . width);
		x_outer_rect . x += t_offset;
		x_inner_rect . x += t_offset;
		x_inner_rect . width = t_table_width;
	}
}

// This method computes the offset from the left-hand margin of the field to the left of
// the paragraph. It also returns the (minimal) textwidth of the paragraph.
void MCParagraph::computeparaoffsetandwidth(int32_t& r_offset, int32_t& r_width) const
{
	int32_t t_layout_width, t_para_width;
	t_layout_width = parent -> getlayoutwidth();
	t_para_width = getwidth();

    int32_t t_offset;
	if (getdontwrap())
	{
		switch(gettextalign())
		{
		case kMCParagraphTextAlignLeft:
		case kMCParagraphTextAlignJustify:
			t_offset = 0;
			break;
		case kMCParagraphTextAlignCenter:
			t_offset = (t_layout_width - t_para_width) / 2;
			break;
		case kMCParagraphTextAlignRight:
			t_offset = t_layout_width - t_para_width;
			break;
		}
	}
	else
		t_offset = 0;
    
	r_offset = t_offset;
	r_width = t_para_width;
}

// This method computes the offset from the left-hand margin of the field to the
// left of the inner paragraph box. It also returns the width of the paragraph box.
void MCParagraph::computeboxoffsetandwidth(int32_t& r_offset, int32_t& r_width) const
{
	MCRectangle t_outer, t_inner;
	computerects(0, 0, parent -> getlayoutwidth(), getwidth(), 0, t_outer, t_inner);
	r_offset = t_inner . x;
	r_width = t_inner . width;
}

// This method computes the offset from the leftmargin of the given line in this
// paragraph.
int32_t MCParagraph::computelineoffset(MCLine *p_line) const
{
	int32_t t_box_offset, t_box_width;
	computeboxoffsetandwidth(t_box_offset, t_box_width);
	return t_box_offset + computelineinneroffset(t_box_width, p_line);
}

// This method computes the offset of the given line from the left (inside) edge
// of the paragrahp box, assuming the box is layout_width wide.
int32_t MCParagraph::computelineinneroffset(int32_t p_layout_width, MCLine *p_line) const
{
	int32_t t_offset;
	t_offset = 0;

	int32_t t_line_width;
	t_line_width = p_line -> getwidth();
	if (getliststyle() != kMCParagraphListStyleNone)
	{
		t_offset += getlistlabelwidth();
		p_layout_width -= getlistlabelwidth();
	}
	else
	{
		int32_t t_first_indent;
		t_first_indent = getfirstindent();

		if (t_first_indent < 0)
		{
			if (p_line != lines)
			{
				p_layout_width -= -t_first_indent;
				t_offset += -t_first_indent;
			}
		}
		else if (t_first_indent > 0)
		{
			if (p_line == lines)
			{
				p_layout_width -= t_first_indent;
				t_offset += t_first_indent;
			}
		}
	}


    // FG-2014-05-06: [[ TabAlignments ]]
    // Lines now handle offsets themselves unless they are non-flowed
    if (getdontwrap())
    {
        if (p_layout_width > t_line_width)
            switch(gettextalign())
            {
            case kMCParagraphTextAlignLeft:
            case kMCParagraphTextAlignJustify:
                break;
            case kMCParagraphTextAlignCenter:
                t_offset += (p_layout_width - t_line_width) / 2;
                break;
            case kMCParagraphTextAlignRight:
                t_offset += p_layout_width - t_line_width;
                break;
            }
    }
    else
    {
           t_offset += p_line->GetLineOffset(); 
    }

	return t_offset;
}

void MCParagraph::computelistindex(MCParagraph *sentinal, uint32_t& r_index)
{
	// MW-2012-11-13: [[ ParaListIndex ]] If we have a list index, then use that.
	if (haslistindex())
	{
		r_index = getlistindex();
		return;
	}

	uint32_t t_list_style, t_list_depth;
	t_list_style = getliststyle();
	t_list_depth = getlistdepth();

	uint32_t t_index;
	t_index = 1;
	for(MCParagraph *t_para = prev(); t_para != sentinal; t_para = t_para -> prev())
	{
		uint32_t t_other_list_style, t_other_list_depth;
		t_other_list_style = t_para -> getliststyle();
		t_other_list_depth = t_para -> getlistdepth();

		// If the other paragraph has no liststyle we are done.
		if (t_other_list_style == kMCParagraphListStyleNone)
			break;

		// If the other list style is skip and of the same depth, continue.
		if (t_other_list_style == kMCParagraphListStyleSkip && t_other_list_depth == t_list_depth)
			continue;

		// If other list depth is greater, we skip over.
		if (t_other_list_depth > t_list_depth)
			continue;

		// If other list style is not equal to this or has lesser depth, we are done.
		if (t_other_list_style != t_list_style || t_other_list_depth < t_list_depth)
			break;

		// MW-2012-11-13: [[ ParaListIndex ]] If the other paragraph has a list index then
		//   take that + 1.
		if (t_para -> haslistindex())
		{
			t_index += t_para -> getlistindex();
			break;
		}

		// Otherwise its a list of same depth and type, so increment index.
		t_index += 1;
	}

	r_index = t_index;
}

void MCParagraph::computeliststyleindex(MCParagraph *sentinal, char r_index_buffer[PG_MAX_INDEX_SIZE], const char*& r_string, uint32_t& r_length)
{
	uint32_t t_list_style;
	t_list_style = getliststyle();

	uint32_t t_index;
	computelistindex(sentinal, t_index);

	// MW-2012-02-21: [[ FieldExport ]] Use a common method to format the list label.
	formatliststyleindex(t_list_style, t_index, r_index_buffer, r_string, r_length);
}

// MW-2012-02-21: [[ FieldExport ]] This method returns the appropriate bullet char
//   for the given style, taking into account whether the target is unicode or not.
uint32_t MCParagraph::getliststylebullet(uint32_t p_list_style, bool p_as_unicode)
{
	if (p_as_unicode)
		return (p_list_style == kMCParagraphListStyleDisc ? 0x2022 : (p_list_style == kMCParagraphListStyleCircle ? 0x25E6 : 0x25AA));
	
#ifdef _MACOSX
	return 0xA5;
#else
	return 0x95;
#endif
}

// MW-2012-02-21: [[ FieldExport ]] This method formats a paragarph number based on
//   the given list style.
void MCParagraph::formatliststyleindex(uint32_t p_list_style, uint32_t p_index, char r_index_buffer[PG_MAX_INDEX_SIZE], const char*& r_string, uint32_t& r_length)
{
	if (p_list_style == kMCParagraphListStyleNumeric)
	{
		sprintf(r_index_buffer, "%d.", p_index);
		r_string = r_index_buffer;
	}
	else if (p_list_style == kMCParagraphListStyleLowerCase || p_list_style == kMCParagraphListStyleUpperCase)
	{
		// Base character depends on case
		char t_base;
		t_base = (p_list_style == kMCParagraphListStyleLowerCase) ? 'a' : 'A';

		// Start at the end of the buffer and work backwards.
		char *t_buffer;
		t_buffer = r_index_buffer + U4L;
		t_buffer[1] = '\0';
		*t_buffer = '.';
		while(p_index != 0)
		{
			t_buffer -= 1;
			*t_buffer = t_base + ((p_index - 1) % 26);
			p_index /= 26;
		}

		r_string = t_buffer;
	}
	else
	{
		// Significant values have specific numeral sequences. As roman numerals
		// are additive we simply subtract each in turn until we reach zero.
		static uint32_t s_values[] = { 1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1 };
		static const char *s_ucase_numerals[] = { "M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I" };
		static const char *s_lcase_numerals[] = { "m", "cm", "d", "cd", "c", "xc", "l", "xl", "x", "ix", "v", "iv", "i" };
		
		// Maximum value allowed is 4000
		if (p_index > 4000)
			p_index = 4000;

		// Choose the correct case
		const char **t_numerals;
		t_numerals = (p_list_style == kMCParagraphListStyleLowerRoman ? s_lcase_numerals : s_ucase_numerals);

		// Now build the string
		char *t_buffer;
		t_buffer = r_index_buffer;
		for(uint32_t i = 0; i < 13; i++)
		{
			while(p_index >= s_values[i])
			{
				p_index -= s_values[i];
				strcpy(t_buffer, t_numerals[i]);
				t_buffer += strlen(t_buffer);
			}
		}

		t_buffer[0] = '.';
		t_buffer[1] = '\0';
		r_string = r_index_buffer;
	}

	r_length = strlen(r_string);
}

////////////////////////////////////////////////////////////////////////////////
