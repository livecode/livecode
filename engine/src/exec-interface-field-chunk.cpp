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

#include "exec-interface.h"

////////////////////////////////////////////////////////////////////////////////

void MCField::GetUnicodeTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef& r_value)
{
    if (exportastext(p_part_id, p_start, p_finish, r_value))
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

template<typename T> void GetParagraphPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, uint32_t p_part_id, int32_t si, int32_t ei, void (MCParagraph::*p_getter)(MCExecContext& ctxt, typename T::return_type&), bool& r_mixed, typename T::return_type& r_value)
{
    MCParagraph *t_paragraph;
    t_paragraph = p_field -> resolveparagraphs(p_part_id);

    int4 t_line_index;
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

        ei -= sptr->gettextsizecr();
        sptr = sptr->next();
    }
    while(ei > 0);

    r_mixed = false;
    T::output(t_value, r_value);
}

template<typename T> void GetCharPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, uint32_t p_part_id, int32_t si, int32_t ei, void (MCBlock::*p_getter)(MCExecContext& ctxt, typename T::return_type&), bool is_effective, typename T::value_type parent_value, bool& r_mixed, typename T::return_type& r_value)
{
    MCParagraph *t_paragraph;
    t_paragraph = p_field -> resolveparagraphs(p_part_id);

    int4 t_line_index;
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
            if (t_block -> getindex() <= si)
                break;
            t_block = t_block -> next();
        }

        for(;;)
        {
            if (t_block -> getindex() >= ei)
                break;

            if (t_block -> getsize() != 0)
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

        ei -= sptr->gettextsizecr();
        sptr = sptr->next();
    }
    while(ei > 0);

    r_mixed = false;
    T::output(t_value, r_value);
}

//////////

void MCField::GetCharIndexOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t& r_value)
{
    MCParagraph *t_paragraph;
    t_paragraph = resolveparagraphs(p_part_id);

    int4 t_line_index, t_char_index;
    t_char_index = si;
    MCParagraph *sptr = indextoparagraph(t_paragraph, si, ei, &t_line_index);

    unresolvechars(p_part_id, t_char_index, t_char_index);

    r_value = t_char_index + 1;
}

void MCField::GetTextAlignOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, intenum_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<intenum_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetTextAlign, r_mixed, r_value);
}

void MCField::GetEffectiveTextAlignOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, intenum_t& r_value)
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

void MCField::GetEffectiveTextSizeOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value)
{
    uinteger_t t_default;
    GetEffectiveTextSize(ctxt, t_default);
    //GetCharPropOfCharChunk< PodFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextSize, true, t_default, r_mixed, r_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCParagraph::GetTextAlign(MCExecContext& ctxt, intenum_t*& r_value)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_TEXT_ALIGN) == 0)
    {
        r_value = nil;
        return;
    }

    *r_value = gettextalign();
}

void MCParagraph::GetEffectiveTextAlign(MCExecContext& ctxt, intenum_t& r_value)
{
    r_value = gettextalign();
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
