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
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "sysdefs.h"

#include "globals.h"
#include "object.h"
#include "stack.h"
#include "cdata.h"
#include "objptr.h"
#include "field.h"
#include "object.h"
#include "button.h"
#include "card.h"
#include "exec.h"
#include "util.h"
#include "group.h"
#include "image.h"
#include "menuparse.h"
#include "stacklst.h"
#include "font.h"
#include "mode.h"
#include "scrolbar.h"
#include "paragraf.h"
#include "block.h"

#include <stddef.h> // offsetof

#include "exec-interface.h"

////////////////////////////////////////////////////////////////////////////////

void MCField::GetUnicodeTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCDataRef& r_value)
{
	MCAutoStringRef t_value;
    if (exportastext(p_part_id, p_start, p_finish, &t_value) &&
		MCStringEncode(*t_value, kMCStringEncodingUTF16, false, r_value))
		return;
	
    ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

template<typename T> struct PodFieldPropType
{
    typedef T value_type;
    typedef T stack_type;
    typedef T return_type;

    template<typename X> static void getter(MCExecContext& ctxt, X *sptr, void (X::*getter)(MCExecContext& ctxt, T&), T& r_value)
    {
        (sptr ->* getter)(ctxt, r_value);
    }

    static void input(T p_value, T& r_value)
    {
        r_value = p_value;
    }

    static bool equal(T a, T b)
    {
        return a == b;
    }

    static void assign(T& x, T y)
    {
        x = y;
    }

    static void output(T p_value, T& r_value)
    {
        r_value = p_value;
    }
};

template<typename T> struct OptionalFieldPropType
{
    typedef typename T::value_type value_type;
    struct stack_type
    {
        typename T::value_type value;
        typename T::return_type *value_ptr;
    };
    typedef typename T::return_type *return_type;

    template<typename X> static void getter(MCExecContext& ctxt, X *sptr, void (X::*getter)(MCExecContext& ctxt, typename T::value_type*&), stack_type& r_value)
    {
        r_value . value_ptr = &r_value . value;
        (sptr ->* getter)(ctxt, r_value . value_ptr);
    }

    static void input(typename T::value_type p_value, stack_type& r_value)
    {
        T::assign(r_value . value, p_value);
    }

    static bool equal(stack_type& a, stack_type& b)
    {
        if (a . value_ptr == nil && b . value_ptr == nil)
            return true;
        if (a . value_ptr == nil || b . value_ptr == nil)
            return false;
        return T::equal(a . value, b . value);
    }

    static void output(stack_type& p_value, typename T::value_type*& r_value)
    {
        if (p_value . value_ptr == nil)
            r_value = nil;
        else
            T::assign(*r_value, p_value . value);
    }
};

template<typename T> void GetParagraphPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, uint32_t p_part_id, findex_t si, findex_t ei, void (MCParagraph::*p_getter)(MCExecContext& ctxt, typename T::return_type&), bool& r_mixed, typename T::return_type& r_value)
{
    MCParagraph *t_paragraph;
    t_paragraph = p_field -> resolveparagraphs(p_part_id);

    findex_t t_line_index;
    MCParagraph *sptr = p_field -> indextoparagraph(t_paragraph, si, ei, &t_line_index);

    typename T::stack_type t_value;
    T::getter(ctxt, sptr, p_getter, t_value);
    if (ctxt . HasError())
        return;

    do
    {
        typename T::stack_type t_new_value;
        T::getter(ctxt, sptr, p_getter, t_new_value);
        if (ctxt . HasError())
            return;

        if (!T::equal(t_value, t_new_value))
        {
            r_mixed = true;
            return;
        }

        ei -= sptr->gettextlengthcr();
        sptr = sptr->next();
    }
    while(ei > 0);

    r_mixed = false;
    T::output(t_value, r_value);
}

template<typename T> void GetCharPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, uint32_t p_part_id, findex_t si, findex_t ei, void (MCBlock::*p_getter)(MCExecContext& ctxt, typename T::return_type&), bool is_effective, typename T::value_type parent_value, bool& r_mixed, typename T::return_type& r_value)
{
    MCParagraph *t_paragraph;
    t_paragraph = p_field -> resolveparagraphs(p_part_id);

    findex_t t_line_index;
    MCParagraph *sptr = p_field -> indextoparagraph(t_paragraph, si, ei, &t_line_index);

    bool t_first;
    typename T::stack_type t_value;

    if (is_effective)
    {
        T::input(parent_value, t_value);
        t_first = false;
    }
    else
        t_first = true;

    do
    {
        MCBlock *t_block;
        t_block = sptr -> getblocks();

        for(;;)
        {
            if (t_block -> GetOffset() <= si)
                break;
            t_block = t_block -> next();
        }

        for(;;)
        {
            if (t_block -> GetOffset() >= ei)
                break;

            if (t_block -> GetLength() != 0)
            {
                if (t_first)
                {
                    T::getter(ctxt, t_block, p_getter, t_value);
                    if (ctxt . HasError())
                        return;

                    t_first = false;
                }
                else
                {
                    typename T::stack_type t_new_value;
                    T::getter(ctxt, t_block, p_getter, t_new_value);
                    if (ctxt . HasError())
                        return;

                    if (!T::equal(t_value, t_new_value))
                    {
                        r_mixed = true;
                        return;
                    }
                }
            }

            t_block = t_block -> next();
        }

        ei -= sptr->gettextlengthcr();
        sptr = sptr->next();
    }
    while(ei > 0);

    r_mixed = false;
    T::output(t_value, r_value);
}

//////////

template<typename T, int Min, int Max> static void setparagraphattr_int(MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset, T *p_value)
{
    if (p_value == nil)
    {
        if (attrs != nil)
        {
            ((T *)((char *)attrs + p_field_offset))[0] = 0;
            attrs -> flags &= ~p_flag;
        }
    }
    else
    {
        T t_clamped_field;
        t_clamped_field = MCMin(MCMax((int)*p_value, Min), Max);

        if (attrs == nil)
            attrs = new MCParagraphAttrs;

        attrs -> flags |= p_flag;
        ((T *)((char *)attrs + p_field_offset))[0] = t_clamped_field;
    }
}

static void setparagraphattr_uint8(MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset, uinteger_t *p_value)
{
    setparagraphattr_int<uinteger_t, 0, 255>(attrs, p_flag, p_field_offset, p_value);
}

static void setparagraphattr_int16(MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset, integer_t *p_value)
{
    setparagraphattr_int<integer_t, INT16_MIN, INT16_MAX>(attrs, p_flag, p_field_offset, p_value);
}

static void setparagraphattr_color(MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset, MCColor *p_color)
{
    if (p_color == nil)
    {
        if (attrs != nil)
        {
            ((uint32_t *)((char *)attrs + p_field_offset))[0] = 0;
            attrs -> flags &= ~p_flag;
        }
    }
    else
    {
        MCscreen -> alloccolor(*p_color);

        if (attrs == nil)
            attrs = new MCParagraphAttrs;

        attrs -> flags |= p_flag;
        ((uint32_t *)((char *)attrs + p_field_offset))[0] = p_color -> pixel;
    }
}

static void setparagraphattr_bool(MCParagraphAttrs*& attrs, uint32_t p_flag, bool *p_value, bool &r_new_value)
{
    if (p_value == nil)
    {
        if (attrs != nil)
        {
            r_new_value = false;
            attrs -> flags &= ~p_flag;
        }
    }
    else
    {
        if (attrs == nil)
            attrs = new MCParagraphAttrs;

        attrs -> flags |= p_flag;
        r_new_value = *p_value;
    }
}

//////////

void MCField::GetCharIndexOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t& r_value)
{
    MCParagraph *t_paragraph;
    t_paragraph = resolveparagraphs(p_part_id);

    findex_t t_line_index, t_char_index;
    findex_t t_si = si;
    findex_t t_ei = ei;
    t_char_index = si;
    MCParagraph *sptr = indextoparagraph(t_paragraph, t_si, t_ei, &t_line_index);

    unresolvechars(p_part_id, t_char_index, t_char_index);

    r_value = t_char_index + 1;
}

void MCField::GetTextAlignOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, intenum_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<intenum_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetTextAlign, r_mixed, r_value);
}

void MCField::GetEffectiveTextAlignOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, intenum_t& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<intenum_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveTextAlign, r_mixed, r_value);
}

void MCField::GetTextSizeOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value)
{
    GetCharPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextSize, false, 0, r_mixed, r_value);
}

void MCField::SetTextSizeOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t* p_value)
{

}

void MCField::GetEffectiveTextSizeOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value)
{
    uinteger_t t_default;
    GetEffectiveTextSize(ctxt, t_default);
    //GetCharPropOfCharChunk< PodFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextSize, true, t_default, r_mixed, r_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCParagraph::GetTextAlign(MCExecContext& ctxt, intenum_t*& r_value)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_TEXT_ALIGN) == 0)
        r_value = nil;
    else
        *r_value = gettextalign();
}

void MCParagraph::GetEffectiveTextAlign(MCExecContext& ctxt, intenum_t& r_value)
{
    r_value = gettextalign();
}

void MCParagraph::SetTextAlign(MCExecContext& ctxt, intenum_t* p_value)
{
    if (p_value == nil)
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
        attrs -> text_align = *p_value;
    }
}

void MCParagraph::GetListStyle(MCExecContext& ctxt, intenum_t*& r_style)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_LIST_STYLE) == 0)
        r_style = nil;
    else
        *r_style = getliststyle();
}

void MCParagraph::GetEffectiveListStyle(MCExecContext& ctxt, intenum_t& r_style)
{
    r_style = getliststyle();
}

void MCParagraph::SetListStyle(MCExecContext& ctxt, intenum_t p_style)
{
    setliststyle(p_style);
}

void MCParagraph::GetListDepth(MCExecContext& ctxt, uinteger_t*& r_depth)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_LIST_STYLE) == 0)
        r_depth = nil;
    else
        *r_depth = getlistdepth();
}

void MCParagraph::GetEffectiveListDepth(MCExecContext& ctxt, uinteger_t& r_depth)
{
    r_depth = getlistdepth();
}

void MCParagraph::SetListDepth(MCExecContext& ctxt, uinteger_t p_depth)
{
    if (p_depth < 1 || p_depth > 16)
    {
        ctxt . Throw();
        return;
    }

    if (attrs == nil)
        attrs = new MCParagraphAttrs;

    if ((attrs -> flags & PA_HAS_LIST_STYLE) == 0)
    {
        attrs -> flags |= PA_HAS_LIST_STYLE;
        attrs -> list_style = kMCParagraphListStyleDisc;
    }

    attrs -> list_depth = p_depth - 1;
}

void MCParagraph::GetListIndent(MCExecContext& ctxt, integer_t*& r_indent)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_LIST_INDENT) == 0)
        r_indent = nil;
    else
        *r_indent = getlistindent();
}

void MCParagraph::GetEffectiveListIndent(MCExecContext& ctxt, integer_t &r_indent)
{
    r_indent = getlistindent();
}

void MCParagraph::SetListIndent(MCExecContext& ctxt, integer_t* p_indent)
{
    setparagraphattr_int16(attrs, PA_HAS_LIST_INDENT, offsetof(MCParagraphAttrs, first_indent), p_indent);

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
}

void MCParagraph::GetListIndex(MCExecContext& ctxt, uinteger_t*& r_list_index)
{
    if (attrs == nil || !(attrs -> flags & PA_HAS_LIST_INDEX))
        r_list_index = nil;
    else
        *r_list_index = getlistindex();
}

void MCParagraph::GetEffectiveListIndex(MCExecContext& ctxt, uinteger_t& r_list_index)
{
    r_list_index = getlistindex();
}

void MCParagraph::SetListIndex(MCExecContext& ctxt, uinteger_t *p_list_index)
{
    setparagraphattr_int<uinteger_t, 1, 65535>(attrs, PA_HAS_LIST_INDEX, offsetof(MCParagraphAttrs, list_index), p_list_index);
}

void MCParagraph::GetFirstIndent(MCExecContext& ctxt, integer_t*& r_indent)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_FIRST_INDENT) == 0)
        r_indent = nil;
    else
        *r_indent = getfirstindent();
}

void MCParagraph::GetEffectiveFirstIndent(MCExecContext& ctxt, integer_t& r_indent)
{
    r_indent = getfirstindent();
}

void MCParagraph::SetFirstIndent(MCExecContext& ctxt, integer_t *p_indent)
{
    setparagraphattr_int16(attrs, PA_HAS_FIRST_INDENT, offsetof(MCParagraphAttrs, first_indent), p_indent);
}

void MCParagraph::GetLeftIndent(MCExecContext& ctxt, integer_t*& r_indent)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_LEFT_INDENT) == 0)
        r_indent = nil;
    else
        *r_indent = getleftindent();
}

void MCParagraph::GetEffectiveLeftIndent(MCExecContext& ctxt, integer_t& r_indent)
{
    r_indent = getleftindent();
}

void MCParagraph::SetLeftIndent(MCExecContext& ctxt, integer_t *p_indent)
{
    setparagraphattr_int16(attrs, PA_HAS_LEFT_INDENT, offsetof(MCParagraphAttrs, left_indent), p_indent);
}

void MCParagraph::GetRightIndent(MCExecContext& ctxt, integer_t*& r_indent)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_RIGHT_INDENT) == 0)
        r_indent = nil;
    else
        *r_indent = getrightindent();
}

void MCParagraph::GetEffectiveRightIndent(MCExecContext& ctxt, integer_t& r_indent)
{
    r_indent = getrightindent();
}

void MCParagraph::SetRightIndent(MCExecContext& ctxt, integer_t *p_indent)
{
    setparagraphattr_int16(attrs, PA_HAS_RIGHT_INDENT, offsetof(MCParagraphAttrs, right_indent), p_indent);
}

void MCParagraph::GetSpaceAbove(MCExecContext& ctxt, integer_t*& r_space)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_SPACE_ABOVE) == 0)
        r_space = nil;
    else
        *r_space = getspaceabove();
}

void MCParagraph::GetEffectiveSpaceAbove(MCExecContext& ctxt, integer_t& r_space)
{
    r_space = getspaceabove();
}

void MCParagraph::SetSpaceAbove(MCExecContext& ctxt, integer_t* p_space)
{
    setparagraphattr_int<integer_t, 0, 32767>(attrs, PA_HAS_SPACE_ABOVE, offsetof(MCParagraphAttrs, space_above), p_space);
}

void MCParagraph::GetSpaceBelow(MCExecContext& ctxt, integer_t*& r_space)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_SPACE_BELOW) == 0)
        r_space = nil;
    else
        *r_space = getspacebelow();
}

void MCParagraph::GetEffectiveSpaceBelow(MCExecContext& ctxt, integer_t& r_space)
{
    r_space = getspacebelow();
}

void MCParagraph::SetSpaceBelow(MCExecContext& ctxt, integer_t* p_space)
{
    setparagraphattr_int<integer_t, 0, 32767>(attrs, PA_HAS_SPACE_BELOW, offsetof(MCParagraphAttrs, space_below), p_space);
}

void MCParagraph::DoSetTabStops(MCExecContext &ctxt, bool p_is_relative, uindex_t p_count, uinteger_t* p_tabs)
{
    MCAutoArray<uint2> t_new_tabs;

    uint2 *t_new = nil;
    uindex_t t_new_count = 0;

    uint2 t_previous_tab_stop;
    t_previous_tab_stop = 0;

    for (uindex_t i = 0; i < p_count; i++)
    {
        if (p_tabs[i] > 65535)
        {
            ctxt . LegacyThrow(EE_PROPERTY_NAN);
            return;
        }

        if (p_is_relative)
        {
            t_new_tabs . Push(p_tabs[i] + t_previous_tab_stop);
            t_previous_tab_stop = t_new_tabs[i];
        }
        else
            t_new_tabs . Push(p_tabs[i]);
    }

    t_new_tabs . Take(t_new, t_new_count);

    if (attrs == nil)
        attrs = new MCParagraphAttrs;
    else
        delete attrs -> tabs;

    if (t_new != nil)
    {
        attrs -> tabs = t_new;
        attrs -> tab_count = t_new_count;
        attrs -> flags |= PA_HAS_TABS;
    }
    else
    {
        attrs -> tabs = nil;
        attrs -> tab_count = 0;
        attrs -> flags &= ~PA_HAS_TABS;
    }
}

void MCParagraph::DoGetTabStops(MCExecContext &ctxt, bool p_is_relative, uindex_t &r_count, uinteger_t *&r_tabs)
{
    uint16_t t_count;
    uint16_t *t_tabs;
    MCAutoArray<uinteger_t> t_tabs_formatted;
    Boolean t_fixed;

    gettabs(t_tabs, t_count, t_fixed);

    if (p_is_relative)
    {
        uinteger_t t_previous_tab;
        t_previous_tab = 0;

        for (uint16_t i = 0; i < t_count; i++)
        {
            t_tabs_formatted . Push((uinteger_t)t_tabs[i] - t_previous_tab);
            t_previous_tab = (uinteger_t)t_tabs[i];
        }
    }
    else
    {
        for (uint16_t i = 0; i < t_count; i++)
            t_tabs_formatted . Push((uinteger_t)t_tabs[i]);
    }

    t_tabs_formatted . Take(r_tabs, r_count);
}

void MCParagraph::GetTabStops(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tabs)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_TABS) == 0)
    {
        r_count = 0;
        r_tabs = nil;
    }
    else
        DoGetTabStops(ctxt, false, r_count, r_tabs);
}

void MCParagraph::GetEffectiveTabStops(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tabs)
{
    DoGetTabStops(ctxt, false, r_count, r_tabs);
}

void MCParagraph::SetTabStops(MCExecContext& ctxt, uindex_t p_count, uinteger_t* p_stops)
{
    DoSetTabStops(ctxt, false, p_count, p_stops);
}

void MCParagraph::GetTabWidths(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tabs)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_TABS) == 0)
    {
        r_count = 0;
        r_tabs = nil;
    }
    else
        DoGetTabStops(ctxt, true, r_count, r_tabs);
}

void MCParagraph::GetEffectiveTabWidths(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tabs)
{
    DoGetTabStops(ctxt, true, r_count, r_tabs);
}

void MCParagraph::SetTabWidths(MCExecContext& ctxt, uindex_t p_count, uinteger_t* p_tabs)
{
    DoSetTabStops(ctxt, true, p_count, p_tabs);
}

void MCParagraph::GetBackColor(MCExecContext& ctxt, MCColor *&r_color)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_BACKGROUND_COLOR) == 0)
        r_color = nil;
    else
        r_color -> pixel = attrs -> background_color;
}

void MCParagraph::GetEffectiveBackColor(MCExecContext& ctxt, MCColor &r_color)
{
    if (attrs != nil && (attrs -> flags & PA_HAS_BACKGROUND_COLOR) != 0)
        r_color . pixel = attrs -> background_color;
}

void MCParagraph::SetBackColor(MCExecContext& ctxt, MCColor *p_color)
{
    setparagraphattr_color(attrs, PA_HAS_BACKGROUND_COLOR, offsetof(MCParagraphAttrs, background_color), p_color);
}

void MCParagraph::GetBorderColor(MCExecContext& ctxt, MCColor*& r_color)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_BORDER_COLOR) == 0)
        r_color = nil;
    else
        r_color -> pixel = attrs -> border_color;
}

void MCParagraph::GetEffectiveBorderColor(MCExecContext& ctxt, MCColor& r_color)
{
    if (attrs != nil && (attrs -> flags & PA_HAS_BORDER_COLOR) != 0)
        r_color . pixel = attrs -> border_color;
}

void MCParagraph::SetBorderColor(MCExecContext& ctxt, MCColor* p_color)
{
    setparagraphattr_color(attrs, PA_HAS_BORDER_COLOR, offsetof(MCParagraphAttrs, border_color), p_color);
}

void MCParagraph::GetBorderWidth(MCExecContext& ctxt, uinteger_t*& r_width)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_BORDER_WIDTH) == 0)
        r_width = nil;
    else
        *r_width = getborderwidth();
}

void MCParagraph::GetEffectiveBorderWidth(MCExecContext& ctxt, uinteger_t& r_width)
{
    r_width = getborderwidth();
}

void MCParagraph::SetBorderWidth(MCExecContext& ctxt, uinteger_t* p_width)
{
    setparagraphattr_uint8(attrs, PA_HAS_BORDER_WIDTH, offsetof(MCParagraphAttrs, border_width), p_width);
}

void MCParagraph::GetPadding(MCExecContext& ctxt, uinteger_t*& r_padding)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_PADDING) == 0)
        r_padding = nil;
    else
        *r_padding = getpadding();
}

void MCParagraph::GetEffectivePadding(MCExecContext& ctxt, uinteger_t& r_padding)
{
    r_padding = getpadding();
}

void MCParagraph::SetPadding(MCExecContext& ctxt, uinteger_t* p_padding)
{
    setparagraphattr_uint8(attrs, PA_HAS_PADDING, offsetof(MCParagraphAttrs, padding), p_padding);
}

void MCParagraph::GetHGrid(MCExecContext& ctxt, bool*& r_has_hgrid)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_HGRID) == 0)
        r_has_hgrid = nil;
    else
        *r_has_hgrid = gethgrid();
}

void MCParagraph::GetEffectiveHGrid(MCExecContext& ctxt, bool& r_has_hgrid)
{
    r_has_hgrid = gethgrid();
}

void MCParagraph::SetHGrid(MCExecContext& ctxt, bool* p_has_hgrid)
{
    bool t_new_value;
    setparagraphattr_bool(attrs, PA_HAS_HGRID, p_has_hgrid, t_new_value);

    if (attrs != nil)
        attrs -> hgrid = t_new_value;
}

void MCParagraph::GetVGrid(MCExecContext& ctxt, bool*& r_has_vgrid)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_VGRID) == 0)
        r_has_vgrid = nil;
    else
        *r_has_vgrid = getvgrid();
}

void MCParagraph::GetEffectiveVGrid(MCExecContext& ctxt, bool& r_has_vgrid)
{
    r_has_vgrid = getvgrid();
}

void MCParagraph::SetVGrid(MCExecContext& ctxt, bool* p_has_vrid)
{
    bool t_new_value;
    setparagraphattr_bool(attrs, PA_HAS_VGRID, p_has_vrid, t_new_value);

    if (attrs != nil)
        attrs -> vgrid = t_new_value;
}

void MCParagraph::GetDontWrap(MCExecContext& ctxt, bool*& r_dont_wrap)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_DONT_WRAP) == 0)
        r_dont_wrap = nil;
    else
        *r_dont_wrap = getdontwrap();
}

void MCParagraph::GetEffectiveDontWrap(MCExecContext& ctxt, bool& r_dont_wrap)
{
    r_dont_wrap = getdontwrap();
}

void MCParagraph::SetDontWrap(MCExecContext& ctxt, bool* p_dont_wrap)
{
    bool t_new_value;
    setparagraphattr_bool(attrs, PA_HAS_DONT_WRAP, p_dont_wrap, t_new_value);

    if (attrs != nil)
        attrs -> dont_wrap = t_new_value;
}

void MCParagraph::GetInvisible(MCExecContext &ctxt, bool *&r_invisible)
{
    if (attrs == nil)
        r_invisible = nil;
    else
        *r_invisible = attrs -> hidden;
}

void MCParagraph::GetEffectiveInvisible(MCExecContext &ctxt, bool &r_invisible)
{
    r_invisible = (attrs != nil) ? attrs -> hidden : false;
}

// MW-2012-03-05: [[ HiddenText ]] Set the 'hidden' property. Notice that if the
//   setting becomes false, we unset the 'has_hidden' flag thus allowing the attrs
//   to be freed if its the only setting.
void MCParagraph::SetInvisible(MCExecContext &ctxt, bool *p_invisible)
{
    bool t_new_value;
    setparagraphattr_bool(attrs, PA_HAS_HIDDEN, p_invisible, t_new_value);

    if (attrs != nil)
    {
        attrs -> hidden = t_new_value;
        if (!t_new_value)
            attrs -> flags &= ~PA_HAS_HIDDEN;
    }
}

void MCParagraph::GetMetadata(MCExecContext& ctxt, MCNameRef *&r_metadata)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_METADATA) == 0)
        r_metadata = nil;
    else
        *r_metadata = MCValueRetain(getmetadata());
}

void MCParagraph::GetEffectiveMetadata(MCExecContext& ctxt, MCNameRef &r_metadata)
{
    r_metadata = MCValueRetain(getmetadata());
}

// MW-2012-11-13: [[ ParaMetadata ]] Set the metadata attribute.
void MCParagraph::SetMetadata(MCExecContext& ctxt, MCNameRef *p_metadata)
{
    if (p_metadata == nil && attrs != nil)
    {
        attrs -> flags &= ~PA_HAS_METADATA;
        MCNameDelete(attrs -> metadata);
        attrs -> metadata = nil;
    }
    else
    {
        if (attrs == nil)
            attrs = new MCParagraphAttrs;

        attrs -> flags |= PA_HAS_METADATA;
        MCNameDelete(attrs -> metadata);
        attrs -> metadata = MCValueRetain(*p_metadata);
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCBlock::GetTextSize(MCExecContext& ctxt, uinteger_t*& r_value)
{
   uint2 t_text_size;
   if (gettextsize(t_text_size))
   {
       *r_value = t_text_size;
       return;
   }

   r_value = nil;
}

////////////////////////////////////////////////////////////////////////////////

/*

template<typename T> void GetOptionalParagraphPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, uint32_t p_part_id, uint32_t si, uint32_t ei, void (MCParagraph::*p_getter)(MCExecContext& ctxt, T*&), bool& r_mixed, T*& r_value)
{
    MCParagraph *t_paragraph;
    t_paragraph = p_field -> resolveparagraphs(p_part_id);

    int4 t_line_index, t_char_index;
    t_char_index = si;
    MCParagraph *sptr = p_field -> indextoparagraph(t_paragraph, si, ei, &t_line_index);

    T t_value;
    T *t_value_ptr;
    t_value_ptr = &t_value;
    sptr -> p_getter(ctxt, t_value_ptr);
    if (ctxt . HasError())
        return;

    do
    {
        T t_new_value;
        T *t_new_value_ptr;
        t_new_value_ptr = &t_new_value;
        sptr -> p_getter(ctxt, t_new_value_ptr);
        if (ctxt . HasError())
            return;

        if (t_value_ptr == nil && t_new_value_ptr != nil ||
            t_value_ptr != nil && t_new_value_ptr == nil ||
            !EqualParagraphProps(t_value, t_new_value))
        {
            r_mixed = true;
            return;
        }

        ei -= sptr->gettextsizecr();
        sptr = sptr->next();
    }
    while(ei > 0);

    r_mixed = false;
    if (t_value_ptr != nil)
        *r_value = t_value;
    else
        r_value = nil;
}
*/

/*struct IntEnumParagraphPropType
{
    struct stack_type
    {
        intenum_t value;
        intenum_t *value_ptr;
    };
    typedef intenum_t *return_type;

    void getter(MCExecContext& ctxt, MCParagraph *sptr, void (MCParagraph::*getter)(MCExecContext& ctxt, intenum_t*&), stack_type& r_value)
    {
        r_value . value_ptr = &r_value . value;
        sptr -> getter(ctxt, r_value . value_ptr);
    }

    bool equal(stack_type& a, stack_type& b)
    {
        if (a . value_ptr == nil && b . value_ptr == nil)
            return true;
        if (a . value_ptr == nil || b . value_ptr == nil)
            return false;
        return a . value == b . value;
    }

    void output(stack_type& p_value, intenum_t*& r_value)
    {
        if (p_value . value_ptr == nil)
            r_value = nil;
        else
            *r_value = p_value . value;
    }
};*/
