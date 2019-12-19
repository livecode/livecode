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
#include "sysdefs.h"

#include "exec.h"
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
#include "MCBlock.h"
#include "variable.h"

#include <stddef.h> // offsetof

#include "exec-interface.h"

////////////////////////////////////////////////////////////////////////////////

template<typename T> struct PodFieldPropType
{
    typedef T value_type;
    typedef T stack_type;
    typedef T return_type;
    typedef T arg_type;

    template<typename X> static void getter(MCExecContext& ctxt, X *sptr, void (X::*getter)(MCExecContext& ctxt, T&), T& r_value)
    {
        (sptr ->* getter)(ctxt, r_value);
    }

    template<typename X> static void setter(MCExecContext &ctxt, X *sptr, void (X::*p_setter)(MCExecContext& ctxt, T), T p_value)
    {
        (sptr ->* p_setter)(ctxt, p_value);
    }

    static void init(T& self)
    {
        self = (T)0;
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
    
    static bool need_layout()
    {
        return true;
    }
    
    static bool is_set(T p_value)
    {
        return true;
    }
};

template<typename T> struct PodFieldArrayPropType
{
    typedef T value_type;
    typedef T stack_type;
    typedef T return_type;
    typedef T arg_type;
    
    template<typename X> static void getter(MCExecContext& ctxt, X *sptr, MCNameRef index, void (X::*getter)(MCExecContext& ctxt, MCNameRef, T&), T& r_value)
    {
        (sptr ->* getter)(ctxt, index, r_value);
    }
    
    template<typename X> static void setter(MCExecContext &ctxt, X *sptr, MCNameRef index, void (X::*p_setter)(MCExecContext& ctxt, MCNameRef, T), T p_value)
    {
        (sptr ->* p_setter)(ctxt, index, p_value);
    }
    
    static void init(T& self)
    {
        self = (T)0;
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
    
    static bool need_layout()
    {
        return true;
    }
    
    static bool is_set(T p_value)
    {
        return true;
    }
};

template <>
struct PodFieldPropType<MCInterfaceNamedColor>
{
    typedef MCInterfaceNamedColor value_type;
    typedef MCInterfaceNamedColor stack_type;
    typedef MCInterfaceNamedColor return_type;
    typedef const MCInterfaceNamedColor& arg_type;

    template<typename X> static void getter(MCExecContext& ctxt, X *sptr, void (X::*p_getter)(MCExecContext& ctxt, return_type&), return_type& r_value)
    {
        (sptr ->* p_getter)(ctxt, r_value);
    }

    template<typename X> static void setter(MCExecContext &ctxt, X *sptr, void (X::*p_setter)(MCExecContext& ctxt, arg_type), arg_type p_value)
    {
        (sptr ->* p_setter)(ctxt, p_value);
    }

    static void init(MCInterfaceNamedColor& self)
    {
        self . name = MCValueRetain(kMCEmptyString);
        self . color . red = 0;
        self . color . green = 0;
        self . color . blue = 0;
    }

    static void input(MCInterfaceNamedColor p_value, MCInterfaceNamedColor& r_value)
    {
        assign(r_value, p_value);
    }

    static bool equal(MCInterfaceNamedColor a, MCInterfaceNamedColor b)
    {
        if (a . name != nil && b . name != nil)
            return MCStringIsEqualTo(a . name, b . name, kMCCompareExact);
        // SN-2014-11-03: [[ Bug 13925 ]] It's false if one has a name, and not the other.
        else if (a . name != nil || b . name != nil)
            return false;
        else
            return (a . color . blue == b . color . blue
                    && a . color . red == b . color . red
                    && a . color . green == b . color . green);
    }

    static void assign(MCInterfaceNamedColor& x, MCInterfaceNamedColor y)
    {
        if (y . name != nil)
            x . name = MCValueRetain(y . name);
        else
        {
            x . color = y . color;
            x . name = nil;
        }
    }

    static void output(MCInterfaceNamedColor p_value, MCInterfaceNamedColor& r_value)
    {
        if (p_value . name != nil)
            r_value . name = p_value . name;
        else
        {
            r_value . color = p_value . color;
            r_value . name = nil;
        }
    }
    
    // don't need to relayout paragraph if block colour changes
    static bool need_layout()
    {
        return false;
    }
    
    static bool is_set(MCInterfaceNamedColor p_value)
    {
        return p_value . name == nil || !MCStringIsEmpty(p_value . name);
    }
};

template <>
struct PodFieldPropType<MCInterfaceTextStyle>
{
    typedef MCInterfaceTextStyle value_type;
    typedef MCInterfaceTextStyle stack_type;
    typedef MCInterfaceTextStyle return_type;
    typedef const MCInterfaceTextStyle& arg_type;

    template<typename X> static void getter(MCExecContext& ctxt, X *sptr, void (X::*p_getter)(MCExecContext& ctxt, return_type&), return_type& r_value)
    {
        (sptr ->* p_getter)(ctxt, r_value);
    }

    template<typename X> static void setter(MCExecContext &ctxt, X *sptr, void (X::*p_setter)(MCExecContext& ctxt, arg_type), arg_type p_value)
    {
        (sptr ->* p_setter)(ctxt, p_value);
    }

    static void init(MCInterfaceTextStyle& self)
    {
        self . style = 0;
    }

    static void input(value_type p_value, stack_type& r_value)
    {
        r_value . style = p_value . style;
    }

    static bool equal(const stack_type& a, const stack_type& b)
    {
        return a . style == b . style;
    }

    static void assign(stack_type& x, stack_type y)
    {
         x . style = y . style;
    }

    static void output(stack_type p_value, return_type& r_value)
    {
        r_value . style = p_value . style;
    }
    
    static bool need_layout()
    {
        return true;
    }
    
    static bool is_set(MCInterfaceTextStyle p_style)
    {
        return p_style . style != 0;
    }
};

template <>
struct PodFieldPropType<MCInterfaceFieldTabAlignments>
{
    typedef MCInterfaceFieldTabAlignments value_type;
    struct stack_type
    {
        MCInterfaceFieldTabAlignments alignments;
        ~stack_type()
        {
            if (alignments.m_alignments != nil)
                MCMemoryDeallocate(alignments.m_alignments);
        }
    };
    typedef MCInterfaceFieldTabAlignments return_type;
    typedef const MCInterfaceFieldTabAlignments& arg_type;
    
    template<typename X> static void getter(MCExecContext& ctxt, X *sptr, void (X::*p_getter)(MCExecContext& ctxt, return_type&), stack_type& r_value)
    {
        (sptr ->* p_getter)(ctxt, r_value.alignments);
    }
    
    template<typename X> static void setter(MCExecContext &ctxt, X *sptr, void (X::*p_setter)(MCExecContext& ctxt, arg_type), arg_type p_value)
    {
        (sptr ->* p_setter)(ctxt, p_value);
    }
    
    static void init(stack_type& self)
    {
        self.alignments.m_count = 0;
        self.alignments.m_alignments = 0;
    }
    
    static void input(value_type p_value, stack_type& r_value)
    {
        r_value.alignments = p_value;
    }
    
    static bool equal(const stack_type& a, const stack_type& b)
    {
        return (a.alignments.m_count == b.alignments.m_count
                && MCMemoryCompare(a.alignments.m_alignments, b.alignments.m_alignments, a.alignments.m_count * sizeof(intenum_t)) == 0);
    }
    
    //static void assign(stack_type& x, stack_type y)
    //{
    //    x = y;
    //}
    
    static void output(stack_type& p_value, return_type& r_value)
    {
        r_value = p_value.alignments;
        p_value.alignments.m_alignments = nil;
    }
    
    static bool need_layout()
    {
        return true;
    }
    
    static bool is_set(stack_type& p_alignments)
    {
        return p_alignments.alignments.m_alignments != nil;
    }
};

template<typename T> struct VectorFieldPropType
{
    typedef vector_t<T> value_type;
    struct stack_type
    {
        vector_t<T> list;
        ~stack_type()
        {
            if (list.elements != nil)
                delete list.elements;
        }
    };
    typedef vector_t<T> return_type;
    typedef const vector_t<T>& arg_type;
    
    // SN-2014-07-25: [[ Bug 12945 ]] While fixing, let's avoid a copy of the ExecContext
    template<typename X> static void getter(MCExecContext& ctxt, X *sptr, void (X::*p_getter)(MCExecContext& ctxt, return_type&), stack_type& r_value)
    {
        (sptr ->* p_getter)(ctxt, r_value . list);
    }
    
    // SN-2014-07-25: [[ Bug 12945 ]] While fixing, let's avoid a copy of the ExecContext
    template <typename X> static void setter(MCExecContext& ctxt, X *sptr, void (X::*p_setter)(MCExecContext& ctxt, arg_type), arg_type p_value)
    {
        (sptr ->* p_setter)(ctxt, p_value);
    }

    static void init(stack_type& r_value)
    {
        r_value . list . count = 0;
        r_value . list . elements = nil;
    }

    static void input(const value_type& p_value, stack_type& r_value)
    {
        r_value . list = p_value;
    }

    static bool equal(const stack_type& a, const stack_type& b)
    {
        if (a . list . count != b . list . count)
            return false;
        else
        {
            for (unsigned int i = 0; i < a . list . count; ++i)
                if (a . list . elements[i] != b . list . elements[i])
                    return false;
        }
        return true;
    }
    
    // SN-2014-07-25: [[ Bug 12945 ]] Making the stack_type a reference argument might allow to set
    //  the elements to nil
    static void output(stack_type& a, return_type& r_value)
    {
        r_value . elements = a . list . elements;
        r_value . count = a . list . count;
        a . list . elements = nil;
    }
    
    static bool need_layout()
    {
        return true;
    }
    
    static bool is_set(stack_type a)
    {
        return true;
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
    typedef typename T::value_type *arg_type;

    template<typename X> static void getter(MCExecContext& ctxt, X *sptr, void (X::*getter)(MCExecContext& ctxt, typename T::value_type*&), stack_type& r_value)
    {
        r_value . value_ptr = &r_value . value;
        (sptr ->* getter)(ctxt, r_value . value_ptr);
    }

    template<typename X> static void setter(MCExecContext& ctxt, X *sptr, void (X::*p_setter)(MCExecContext&, arg_type), arg_type p_value)
    {
        (sptr ->* p_setter)(ctxt, p_value);
    }

    static void input(typename T::value_type p_value, stack_type& r_value)
    {
        T::assign(r_value . value, p_value);
        r_value . value_ptr = &r_value . value;
    }

    static void init(stack_type& self)
    {
        self . value_ptr = nil;
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
    
    static bool need_layout()
    {
        return true;
    }
    
    static bool is_set(stack_type p_value)
    {
        return p_value . value_ptr != nil;
    }
};

template<typename T> struct OptionalFieldArrayPropType
{
    typedef typename T::value_type value_type;
    struct stack_type
    {
        typename T::value_type value;
        typename T::return_type *value_ptr;
    };
    typedef typename T::return_type *return_type;
    typedef typename T::value_type *arg_type;
    
    template<typename X> static void getter(MCExecContext& ctxt, X *sptr, MCNameRef index, void (X::*getter)(MCExecContext& ctxt, MCNameRef index, typename T::value_type*&), stack_type& r_value)
    {
        r_value . value_ptr = &r_value . value;
        (sptr ->* getter)(ctxt, index, r_value . value_ptr);
    }
    
    template<typename X> static void setter(MCExecContext& ctxt, X *sptr, MCNameRef index, void (X::*p_setter)(MCExecContext&, MCNameRef, arg_type), arg_type p_value)
    {
        (sptr ->* p_setter)(ctxt, index, p_value);
    }
    
    static void input(typename T::value_type p_value, stack_type& r_value)
    {
        T::assign(r_value . value, p_value);
        r_value . value_ptr = &r_value . value;
    }
    
    static void init(stack_type& self)
    {
        self . value_ptr = nil;
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
    
    static bool need_layout()
    {
        return true;
    }
    
    static bool is_set(stack_type p_value)
    {
        return p_value . value_ptr != nil;
    }
};

template<typename T> void GetParagraphPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, uint32_t p_part_id, findex_t si, findex_t ei, void (MCParagraph::*p_getter)(MCExecContext& ctxt, typename T::return_type&), bool& r_mixed, typename T::return_type& r_value)
{
    MCParagraph *t_paragraph;
    t_paragraph = p_field -> resolveparagraphs(p_part_id);

    findex_t t_line_index;
    MCParagraph *sptr = p_field -> indextoparagraph(t_paragraph, si, ei, &t_line_index);

    typename T::stack_type t_value;

    T::init(t_value);
    T::getter(ctxt, sptr, p_getter, t_value);
    if (ctxt . HasError())
        return;

    do
    {
        typename T::stack_type t_new_value;
        T::init(t_new_value);
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
    t_first = true;
    
    typename T::stack_type t_value;
    typename T::stack_type t_default_value;
    
    T::init(t_value);
    T::init(t_default_value);
    
    if (is_effective)
    {
        T::input(parent_value, t_default_value);
    }
    
    bool t_mixed;
    t_mixed = false;
    
    bool t_first_set;
    t_first_set = false;
    
    do
    {
        MCBlock *t_firstblock;
        MCBlock *t_block;
        
        t_firstblock = sptr -> getblocks();
        t_block = sptr -> indextoblock(si, False);
        
        while(t_block->next() != t_firstblock && t_block->GetLength() == 0)
        {
            t_block = t_block->next();
        }
        
        for(;;)
        {
            if (t_first)
            {
                T::getter(ctxt, t_block, p_getter, t_value);
                if (ctxt . HasError())
                    return;
                
                t_first = false;
                
                // If the first value is default, we don't need to compare to future unset values.
                t_first_set = T::is_set(t_value);
            }
            else
            {
                typename T::stack_type t_new_value;
                T::getter(ctxt, t_block, p_getter, t_new_value);
                if (ctxt . HasError())
                    return;
                
                if (T::is_set(t_new_value) != t_first_set)
                {
                    // if one is set and the other is unset, then the result is mixed
                    // unless we are effective, in which case check for equality
                    if (!is_effective)
                        t_mixed = true;
                    else if (t_first_set)
                        t_mixed = !T::equal(t_value, t_default_value);
                    else
                        t_mixed = !T::equal(t_new_value, t_default_value);
                }
                else
                {
                    // if they are both set, then test equality between them
                    t_mixed = !T::equal(t_value, t_new_value);
                }
                
                // otherwise they are both unset, and therefore equal,
                // so leave t_mixed alone and return if the result is 'mixed'
                if (t_mixed)
                {
                    r_mixed = true;
                    return;
                }
            }
        
            // Stop if the next block is the first one - we are the last one
            if (t_block -> next() == t_firstblock)
                break;
            
            // Stop if the next block index will exceed the end index
            if (t_block -> next() -> GetOffset() >= ei)
                break;
            
            t_block = t_block -> next();
        }
        
        ei -= sptr->gettextlengthcr();
        sptr = sptr->next();
    }
    while(ei > 0);
    
    r_mixed = false;
    
    if (t_first_set)
        T::output(t_value, r_value);
    else
        T::output(t_default_value, r_value);
}

template<typename T> void GetArrayCharPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, uint32_t p_part_id, findex_t si, findex_t ei, MCNameRef p_index, void (MCBlock::*p_getter)(MCExecContext& ctxt, MCNameRef index, typename T::return_type&), bool is_effective, typename T::value_type parent_value, bool& r_mixed, typename T::return_type& r_value)
{
    MCParagraph *t_paragraph;
    t_paragraph = p_field -> resolveparagraphs(p_part_id);
    
    findex_t t_line_index;
    MCParagraph *sptr = p_field -> indextoparagraph(t_paragraph, si, ei, &t_line_index);
    
    bool t_first;
    t_first = true;
    
    typename T::stack_type t_value;
    typename T::stack_type t_default_value;
    
    T::init(t_value);
    T::init(t_default_value);
    
    if (is_effective)
        T::input(parent_value, t_default_value);
    
    bool t_mixed;
    t_mixed = false;
    
    bool t_first_set;
    t_first_set = false;
    
    do
    {
        MCBlock *t_firstblock;
        MCBlock *t_block;
        
        t_firstblock = sptr -> getblocks();
        t_block = sptr -> indextoblock(si, False);
        
        for(;;)
        {
            if (t_first)
            {
                T::getter(ctxt, t_block, p_index, p_getter, t_value);
                if (ctxt . HasError())
                    return;
                
                t_first = false;
                
                // If the first value is default, we don't need to compare to future unset values.
                t_first_set = T::is_set(t_value);
            }
            else
            {
                typename T::stack_type t_new_value;
                T::getter(ctxt, t_block, p_index, p_getter, t_new_value);
                if (ctxt . HasError())
                    return;
                
                if (T::is_set(t_new_value) != t_first_set)
                {
                    // if one is set and the other is unset, then the result is mixed
                    // unless we are effective, in which case check for equality
                    if (!is_effective)
                        t_mixed = true;
                    else if (t_first_set)
                        t_mixed = !T::equal(t_value, t_default_value);
                    else
                        t_mixed = !T::equal(t_new_value, t_default_value);
                }
                else
                {
                    // if they are both set, then test equality between them
                    t_mixed = !T::equal(t_value, t_new_value);
                }
                
                // otherwise they are both unset, and therefore equal,
                // so leave t_mixed alone and return if the result is 'mixed'
                if (t_mixed)
                {
                    r_mixed = true;
                    return;
                }
            }
            
            // Stop if the next block is the first one - we are the last one
            if (t_block -> next() == t_firstblock)
                break;
            
            // Stop if the next block index will exceed the end index
            if (t_block -> next() -> GetOffset() >= ei)
                break;
            
            t_block = t_block -> next();
        }
        
        ei -= sptr->gettextlengthcr();
        sptr = sptr->next();
    }
    while(ei > 0);
    
    r_mixed = false;
    
    if (t_first_set)
        T::output(t_value, r_value);
    else
        T::output(t_default_value, r_value);
}


// SN-2014-11-04: [[ Bug 13934 ]] Laying out a field refactored
struct MCFieldLayoutSettings
{
    uint32_t savex;
    uint32_t savey;
    findex_t ssi;
    findex_t sei;
    MCRectangle drect;
    bool redraw_field;

    bool need_layout;
    bool all;
    MCField* field;
};

typedef struct MCFieldLayoutSettings MCFieldLayoutSettings;

// SN-2014-11-04: [[ Bug 13934 ]] Stores all the needed settings and data and return
//  the first paragraph to be used.
//  This should be called before any change is brought to the field.
MCParagraph* PrepareLayoutSettings(bool all, MCField *p_field, uint32_t p_part_id, findex_t &si, findex_t &ei, MCFieldLayoutSettings &r_layout_settings)
{
    MCFieldLayoutSettings t_layout_settings;

    t_layout_settings . savex = p_field -> textx;
    t_layout_settings . savey = p_field -> texty;
    t_layout_settings . ssi = 0;
    t_layout_settings . sei = 0;
    t_layout_settings . drect = p_field -> getrect();
    t_layout_settings . redraw_field = false;
    t_layout_settings . all = all;
    t_layout_settings . field = p_field;

    MCParagraph *t_top_paragraph;

    t_top_paragraph = p_field -> resolveparagraphs(p_part_id);

    p_field -> verifyindex(t_top_paragraph, si, false);
    p_field -> verifyindex(t_top_paragraph, ei, true);

    // MW-2013-03-20: [[ Bug 10764 ]] We only need to layout if the paragraphs
    //   are attached to the current card.
    if (p_field -> getopened())
        t_layout_settings . need_layout = t_top_paragraph == p_field -> getparagraphs();
    else
        t_layout_settings . need_layout = false;

    // Get the first paragraph having the indices.
    MCParagraph* t_pgptr;
    t_pgptr = p_field -> indextoparagraph(t_top_paragraph, si, ei);

    // MW-2008-07-09: [[ Bug 6353 ]] Improvements in 2.9 meant that the field was
    //   more careful about not doing anything if it wasn't the MCactivefield.
    //   However, the unselection/reselection code here breaks text input if the
    //   active field sets text properties of another field. Therefore we only
    //   get and then reset the selection if we are the active field.
    if (t_layout_settings . need_layout)
    {
        if (all)
        {
            // Same as this?
            if (MCactivefield == p_field)
            {
                p_field -> selectedmark(False, t_layout_settings . ssi, t_layout_settings . sei, False);
                p_field -> unselect(False, True);
            }
            p_field -> curparagraph = p_field -> focusedparagraph = p_field -> paragraphs;
            p_field -> firstparagraph = p_field -> lastparagraph = NULL;
            p_field -> cury = p_field -> focusedy = p_field -> topmargin;
            p_field -> textx = p_field -> texty = 0;
            //            p_field -> resetparagraphs();
        }
        else
        {
            // MW-2012-02-27: [[ Bug ]] Update rect slightly off, shows itself when
            //   setting the box style of the top line of a field.
            t_layout_settings . drect = p_field -> getfrect();
            t_layout_settings . drect.y = p_field -> getcontenty() + p_field -> paragraphtoy(t_pgptr);
            t_layout_settings . drect.height = 0;
        }
    }

    r_layout_settings = t_layout_settings;
    return t_pgptr;
}

// SN-2014-11-04: [[ Bug 13934 ]] Update the area of the field to redraw,
//  depending on the paragraph settings.
// SN-2014-12-18: [[ Bug 14161 ]] Add a parameter to force the re-layout of a paragraph
void LayoutParagraph(MCParagraph* p_paragraph, MCFieldLayoutSettings &x_layout_settings, bool p_force)
{
    // AL-2014-07-14: [[ Bug 12789 ]] Defragging can cause paragraph to need layout, do make sure we relayout
    //  if it did. Otherwise setting properties that avoid relayout can cause crashes.
    // SN-2014-12-18: [[ Bug 14161 ]] The relayout can be forced
    if (p_force || (p_paragraph -> getneedslayout() && !x_layout_settings . all && p_paragraph->getopened()))
    {
        // MW-2012-01-25: [[ ParaStyles ]] Ask the paragraph to reflow itself.
        // AL-2014-09-22: [[ Bug 11817 ]] If we changed the amount of lines of this paragraph
        //  then redraw the whole field.
        if (p_paragraph -> layout(x_layout_settings . all, true))
            x_layout_settings . redraw_field = true;
        else
            x_layout_settings.drect.height += p_paragraph->getheight(x_layout_settings . field -> fixedheight);
    }
}

// SN-2014-11-04: [[ Bug 13934 ]] Called once the field has been updated,
//  to ask for the appropriate area of the field to be redrawn.
void FinishLayout(MCFieldLayoutSettings &x_layout_settings)
{
    if (x_layout_settings . need_layout)
    {
        if (x_layout_settings . all)
        {
            x_layout_settings . field -> Relayout(false, x_layout_settings .savex - x_layout_settings . field -> textx, x_layout_settings . savey - x_layout_settings . field -> texty);
            if (MCactivefield == x_layout_settings . field)
                x_layout_settings . field -> seltext(x_layout_settings . ssi, x_layout_settings . sei, False);
        }
        else
        {
            x_layout_settings . field -> removecursor();
            // AL-2014-09-22: [[ Bug 11817 ]] If we are redrawing, then the dirty rect is the whole rect.
            if (x_layout_settings . redraw_field)
                x_layout_settings . drect = x_layout_settings . field -> getrect();
        }
        // MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
        x_layout_settings . field -> layer_redrawrect(x_layout_settings . drect);
        if (!x_layout_settings . all)
            x_layout_settings . field -> replacecursor(False, True);
    }
}

template<typename T> void SetParagraphPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, bool all, uint32_t p_part_id, findex_t si, findex_t ei, void (MCParagraph::*p_setter)(MCExecContext&, typename T::arg_type), typename T::arg_type p_value)
{
    // AL-2014-09-01: [[ Bug 13316 ]] Setting of line chunk props should cycle through paragraphs
    // SN-2014-11-04: [[ Bug 13934 ]] Laying out a field refactored.
    MCFieldLayoutSettings t_layout_settings;
    MCParagraph *sptr;

    sptr = PrepareLayoutSettings(all, p_field, p_part_id, si, ei, t_layout_settings);
    
    do
    {
        // AL-2014-09-24: [[ Bug 13529 ] Ensure all necessary cleanups are applied to each paragraph
        //  affected by the property change
        sptr -> defrag();
        
        T::setter(ctxt, sptr, p_setter, p_value);
        if (ctxt . HasError())
            return;
        
        sptr -> cleanattrs();

        // SN-2014-11-04: [[ Bug 13934 ]] Laying out a field refactored.
        // SN-2014-12-18: [[ Bug 14161 ]] Forces the re-layout of this paragraph which has been changed
        // SN-2015-01-22: [[ Bug 14428 ]] Ensure that the field is open, before forcing a re-layout
        //  (MCParagraph::lines is NULL otherwise, and crashes in MCParagraph::countlines).
        LayoutParagraph(sptr, t_layout_settings, sptr -> getopened());
        
        ei -= sptr->gettextlengthcr();
        sptr = sptr->next();
    }
    while(ei >= 0);

    // SN-2014-11-04: [[ Bug 13934 ]] Laying out a field refactored.
    FinishLayout(t_layout_settings);
}

// SN-28-11-13: Added specific function for the IDE which needs
// to set the property to a char chunk in a given paragraph.
template<typename T> void SetCharPropOfCharChunkOfParagraph(MCExecContext& ctxt, MCParagraph *p_paragraph, findex_t si, findex_t ei, void (MCBlock::*p_setter)(MCExecContext&, typename T::arg_type), typename T::arg_type p_value)
{
    MCField *t_field;
    t_field = p_paragraph -> getparent();

    // Sanity check for lengths
    uindex_t t_para_len;
    t_para_len = p_paragraph->gettextlength();
    if (si > 0 && (uindex_t) si > t_para_len)
    {
        si = ei = t_para_len;
    }
    else if (ei > 0 && (uindex_t) ei > t_para_len)
    {
        ei = t_para_len;
    }
    
    bool t_blocks_changed;
    t_blocks_changed = false;

    p_paragraph -> defrag();
    MCBlock *bptr = p_paragraph -> indextoblock(si, False);
    findex_t t_block_index, t_block_length;
    do
    {
        bptr->GetRange(t_block_index, t_block_length);
        if (t_block_index < si)
        {
            MCBlock *tbptr = new (nothrow) MCBlock(*bptr);
            bptr->append(tbptr);
            bptr->SetRange(t_block_index, si - t_block_index);
            tbptr->SetRange(si, t_block_length - (si - t_block_index));
            bptr = bptr->next();
            bptr->GetRange(t_block_index, t_block_length);
            t_blocks_changed = true;
        }
        else
            bptr->close();
        if (t_block_index + t_block_length > ei)
        {
            MCBlock *tbptr = new (nothrow) MCBlock(*bptr);
            // MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
            //   fontref so it can compute its.
            if (p_paragraph -> getopened())
                tbptr->open(t_field -> getfontref());
            bptr->append(tbptr);
            bptr->SetRange(t_block_index, ei - t_block_index);
            tbptr->SetRange(ei, t_block_length - ei + t_block_index);
            t_blocks_changed = true;
        }

        T::setter(ctxt, bptr, p_setter, p_value);

        // MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
        //   fontref so it can compute its.
        if (p_paragraph -> getopened())
            bptr->open(t_field -> getfontref());

        bptr = bptr->next();
    }
    while (t_block_index + t_block_length < ei);

    if (t_blocks_changed)
        p_paragraph -> setDirty();

    if (T::need_layout() || t_blocks_changed)
        p_paragraph -> layoutchanged();
}

template<typename T> void SetCharPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, bool all, uint32_t p_part_id, findex_t si, findex_t ei, void (MCBlock::*p_setter)(MCExecContext&, typename T::arg_type), typename T::arg_type p_value)
{
    if (p_field -> getflag(F_SHARED_TEXT))
        p_part_id = 0;

    // SN-2014-11-04: [[ Bug 13934 ]] Laying out a field refactored.
    MCFieldLayoutSettings t_layout_settings;
    MCParagraph *pgptr;

    pgptr = PrepareLayoutSettings(all, p_field, p_part_id, si, ei, t_layout_settings);

    MCParagraph *t_first_pgptr;
    t_first_pgptr = pgptr;
    
    do
    {
        findex_t t_pg_length = pgptr->gettextlengthcr();
        if (si < t_pg_length)
        {
            pgptr->setparent(p_field);

            // MCParagraph scope
            {
                findex_t t_ei;
                t_ei = MCU_min(ei, pgptr -> gettextlength());
                bool t_blocks_changed;
                bool t_need_layout;
                t_blocks_changed = false;

                pgptr -> defrag();
                MCBlock *bptr = pgptr -> indextoblock(si, False);
                findex_t t_block_index, t_block_length;
                do
                {
                    bptr->GetRange(t_block_index, t_block_length);
                    if (t_block_index < si)
                    {
                        MCBlock *tbptr = new (nothrow) MCBlock(*bptr);
                        bptr->append(tbptr);
                        bptr->SetRange(t_block_index, si - t_block_index);
                        tbptr->SetRange(si, t_block_length - (si - t_block_index));
                        bptr = bptr->next();
                        bptr->GetRange(t_block_index, t_block_length);
                        t_blocks_changed = true;
                    }
                    else
                        bptr->close();
                    if (t_block_index + t_block_length > t_ei)
                    {
                        MCBlock *tbptr = new (nothrow) MCBlock(*bptr);
                        // MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
                        //   fontref so it can compute its.
                        if (pgptr -> getopened())
                            tbptr->open(pgptr -> getparent() -> getfontref());
                        bptr->append(tbptr);
                        bptr->SetRange(t_block_index, t_ei - t_block_index);
                        tbptr->SetRange(t_ei, t_block_length - t_ei + t_block_index);
                        t_blocks_changed = true;
                    }
                    
                    T::setter(ctxt, bptr, p_setter, p_value);
                    
                    // AL-2014-09-23 [[ Bug 13509 ]] Delete the atts struct if this block has no atts
                    bptr -> cleanatts();
                    
                    // MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
                    //   fontref so it can compute its.
                    if (pgptr -> getopened())
                        bptr->open(pgptr -> getparent() -> getfontref());
                    bptr = bptr->next();
                }
                while (t_block_index + t_block_length < (t_pg_length-1) // Length of paragraph without CR
                       && t_block_index + t_block_length < t_ei);

                // avoid relayout for certain block attributes
                t_need_layout = T::need_layout();
                
                // MP-2013-09-02: [[ FasterField ]] If attributes on existing blocks needing layout changed,
                //   or the blocks themselves changed, we need layout.
                if (t_blocks_changed)
                    pgptr -> setDirty();
                
                if (t_need_layout || t_blocks_changed)
                    pgptr -> layoutchanged();
            }
            // end of MCParagraph scope

            // SN-2014-11-04: [[ Bug 13934 ]] Laying out a field refactored.
            // SN-2014-12-18: [[ Bug 14161 ]] Add a parameter to force the re-layout of a paragraph
            LayoutParagraph(pgptr, t_layout_settings, false);
        }

        si = MCU_max(0, si - t_pg_length);
        ei -= t_pg_length;
        pgptr = pgptr->next();

        // MW-2013-08-27: [[ Bug 11129 ]] If we reach the end of the paragraphs
        //   then set ei to 0 as we are done.
        if (pgptr == t_first_pgptr)
            ei = 0;

        // Stop in case of an error
        if (ctxt . HasError())
            ei = 0;
    }
    while(ei > 0);

    // SN-2014-11-04: [[ Bug 13934 ]] Laying out a field refactored.
    FinishLayout(t_layout_settings);
}

template<typename T> void SetArrayCharPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, bool all, uint32_t p_part_id, findex_t si, findex_t ei, MCNameRef p_index, void (MCBlock::*p_setter)(MCExecContext&, MCNameRef, typename T::arg_type), typename T::arg_type p_value)
{
    if (p_field -> getflag(F_SHARED_TEXT))
        p_part_id = 0;

    // SN-2014-11-04: [[ Bug 13934 ]] Laying out a field refactored.
    MCFieldLayoutSettings t_layout_settings;
    MCParagraph *pgptr;

    pgptr = PrepareLayoutSettings(all, p_field, p_part_id, si, ei, t_layout_settings);
    
    MCParagraph *t_first_pgptr;
    t_first_pgptr = pgptr;
    
    do
    {
        findex_t t_pg_length = pgptr->gettextlengthcr();
        if (si < t_pg_length)
        {
            pgptr->setparent(p_field);
            
            // MCParagraph scope
            {
                findex_t t_ei;
                t_ei = MCU_min(ei, pgptr -> gettextlength());
                bool t_blocks_changed;
                bool t_need_layout;
                t_blocks_changed = false;
                
                pgptr -> defrag();
                MCBlock *bptr = pgptr -> indextoblock(si, False);
                findex_t t_block_index, t_block_length;
                do
                {
                    bptr->GetRange(t_block_index, t_block_length);
                    if (t_block_index < si)
                    {
                        MCBlock *tbptr = new (nothrow) MCBlock(*bptr);
                        bptr->append(tbptr);
                        bptr->SetRange(t_block_index, si - t_block_index);
                        tbptr->SetRange(si, t_block_length - (si - t_block_index));
                        bptr = bptr->next();
                        bptr->GetRange(t_block_index, t_block_length);
                        t_blocks_changed = true;
                    }
                    else
                        bptr->close();
                    if (t_block_index + t_block_length > t_ei)
                    {
                        MCBlock *tbptr = new (nothrow) MCBlock(*bptr);
                        // MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
                        //   fontref so it can compute its.
                        if (pgptr -> getopened())
                            tbptr->open(pgptr -> getparent() -> getfontref());
                        bptr->append(tbptr);
                        bptr->SetRange(t_block_index, t_ei - t_block_index);
                        tbptr->SetRange(t_ei, t_block_length - t_ei + t_block_index);
                        t_blocks_changed = true;
                    }
                    
                    T::setter(ctxt, bptr, p_index, p_setter, p_value);
                    
                    // AL-2014-09-23 [[ Bug 13509 ]] Delete the atts struct if this block has no atts
                    bptr -> cleanatts();
                    
                    // MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
                    //   fontref so it can compute its.
                    if (pgptr -> getopened())
                        bptr->open(pgptr -> getparent() -> getfontref());
                    bptr = bptr->next();
                }
                while (t_block_index + t_block_length < (t_pg_length-1) // Length of paragraph without CR
                       && t_block_index + t_block_length < t_ei);
                
                // avoid relayout for certain block attributes
                t_need_layout = T::need_layout();
                
                // MP-2013-09-02: [[ FasterField ]] If attributes on existing blocks needing layout changed,
                //   or the blocks themselves changed, we need layout.
                if (t_blocks_changed)
                    pgptr -> setDirty();
                
                if (t_need_layout || t_blocks_changed)
                    pgptr -> layoutchanged();
            }
            // end of MCParagraph scope

            // SN-2014-11-04: [[ Bug 13934 ]] Laying out a field refactored.
            // SN-2014-12-18: [[ Bug 14161 ]] Add a parameter to force the re-layout of a paragraph
            LayoutParagraph(pgptr, t_layout_settings, false);
        }
        
        si = MCU_max(0, si - t_pg_length);
        ei -= t_pg_length;
        pgptr = pgptr->next();
        
        // MW-2013-08-27: [[ Bug 11129 ]] If we reach the end of the paragraphs
        //   then set ei to 0 as we are done.
        if (pgptr == t_first_pgptr)
            ei = 0;
        
        // Stop in case of an error
        if (ctxt . HasError())
            ei = 0;
    }
    while(ei > 0);

    // SN-2014-11-04: [[ Bug 13934 ]] Laying out a field refactored.
    FinishLayout(t_layout_settings);
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
            attrs = new (nothrow) MCParagraphAttrs;

        attrs -> flags |= p_flag;
        ((T *)((char *)attrs + p_field_offset))[0] = t_clamped_field;
    }
}

// AL-2014-09-30: [[ Bug 13559 ]] Make sure these functions use the correct type for the template
//  otherwise dereferencing and setting can overwrite other parapraph attrs.
static void setparagraphattr_uint8(MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset, uinteger_t *p_value)
{
    setparagraphattr_int<uint8_t, 0, 255>(attrs, p_flag, p_field_offset, (uint8_t *)p_value);
}

static void setparagraphattr_int16(MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset, integer_t *p_value)
{
    setparagraphattr_int<int16_t, INT16_MIN, INT16_MAX>(attrs, p_flag, p_field_offset, (int16_t *)p_value);
}

static void setparagraphattr_uint16(MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset, uinteger_t *p_value)
{
    setparagraphattr_int<uint16_t, 0, UINT16_MAX>(attrs, p_flag, p_field_offset, (uint16_t *)p_value);
}

static void setparagraphattr_color(MCParagraphAttrs*& attrs, uint32_t p_flag, size_t p_field_offset, const MCInterfaceNamedColor& p_color)
{
    MCColor t_color;
    if (p_color . name != nil) // name no null: must interpret the string
    {
        if (MCStringIsEmpty(p_color . name)) // Empty color name: no color set
        {
            if (attrs != nil)
            {
                ((uint32_t *)((char *)attrs + p_field_offset))[0] = 0;
                attrs -> flags &= ~p_flag;
            }
            return;
        }

        MCscreen -> parsecolor(p_color . name, t_color, nil);
    }
    else // name null: must interpret the MCColor
        t_color = p_color . color;

    if (attrs == nil)
        attrs = new (nothrow) MCParagraphAttrs;

    attrs -> flags |= p_flag;
    ((uint32_t *)((char *)attrs + p_field_offset))[0] = MCColorGetPixel(t_color);
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
            attrs = new (nothrow) MCParagraphAttrs;

        attrs -> flags |= p_flag;
        r_new_value = *p_value;
    }
}

//////////

void MCField::GetTextAlignOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, intenum_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<intenum_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetTextAlign, r_mixed, r_value);
}

void MCField::GetEffectiveTextAlignOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, intenum_t& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<intenum_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveTextAlign, r_mixed, r_value);
}

void MCField::SetTextAlignOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, intenum_t* value)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<intenum_t> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetTextAlign, value);
}

void MCField::GetTextSizeOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value)
{
    GetCharPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextSize, false, 0, r_mixed, r_value);
}

void MCField::GetEffectiveTextSizeOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value)
{
    uinteger_t t_size;
    uinteger_t *t_size_ptr = &t_size;
    GetCharPropOfCharChunk<OptionalFieldPropType<PodFieldPropType<uinteger_t> > >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextSize, false, 0, r_mixed, t_size_ptr);

    if (r_mixed)
        return;

    if (t_size_ptr == nil)
        GetEffectiveTextSize(ctxt, r_value);
    else
        r_value = t_size;
}

void MCField::SetTextSizeOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t* p_value)
{
    SetCharPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, true, p_part_id, si, ei, &MCBlock::SetTextSize, p_value);
}

void MCField::GetTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef& r_value)
{
    if (exportastext(p_part_id, p_start, p_finish, r_value))
        return;

    ctxt . Throw();
}

void MCField::SetTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef value)
{
    settextindex(p_part_id, p_start, p_finish, value, false);
}

void MCField::GetUnicodeTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCDataRef& r_value)
{
    MCAutoStringRef t_value;
    if (exportastext(p_part_id, p_start, p_finish, &t_value) &&
        MCStringEncode(*t_value, kMCStringEncodingUTF16, false, r_value))
        return;

    ctxt . Throw();
}

void MCField::SetUnicodeTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCDataRef r_value)
{
    MCAutoStringRef t_string;
	if (MCStringDecode(r_value, kMCStringEncodingUTF16, false, &t_string) &&
		settextindex(p_part_id, p_start, p_finish, *t_string, false))
		return;
	
	ctxt . Throw();
}

void MCField::GetPlainTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef& r_value)
{
    if (exportasplaintext(p_part_id, p_start, p_finish, r_value))
        return;

    ctxt . Throw();
}

void MCField::GetUnicodePlainTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCDataRef& r_value)
{
    MCAutoStringRef t_text;
    GetPlainTextOfCharChunk(ctxt, p_part_id, p_start, p_finish, &t_text);
    if (!ctxt . HasError() &&
        MCStringEncode(*t_text, kMCStringEncodingUTF16, false, r_value))
        return;

    ctxt . Throw();
}

void MCField::GetFormattedTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef& r_value)
{
    if (exportasformattedtext(p_part_id, p_start, p_finish, r_value))
        return;

    ctxt . Throw();
}

void MCField::GetUnicodeFormattedTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCDataRef& r_value)
{
    MCAutoStringRef t_string;
    if (exportasformattedtext(p_part_id, p_start, p_finish, &t_string)
            && MCStringEncode(*t_string, kMCStringEncodingUTF16, false, r_value))
        return;

    ctxt . Throw();
}

void MCField::GetRtfTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef& r_value)
{
    if (exportasrtftext(p_part_id, p_start, p_finish, r_value))
        return;

    ctxt . Throw();
}

void MCField::SetRtfTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef value)
{
    state |= CS_NO_FILE; // prevent interactions while downloading images

    setparagraphs(rtftoparagraphs(value), p_part_id, p_start, p_finish);

    state &= ~CS_NO_FILE;
}

void MCField::GetHtmlTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCValueRef& r_value)
{
    if (exportashtmltext(p_part_id, p_start, p_finish, false, (MCDataRef&)r_value))
        return;

    ctxt . Throw();
}

void MCField::GetEffectiveHtmlTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCValueRef& r_value)
{
    if (exportashtmltext(p_part_id, p_start, p_finish, true, (MCDataRef&)r_value))
        return;

    ctxt . Throw();
}

void MCField::SetHtmlTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCValueRef value)
{
    if (state & CS_NO_FILE)
    {
        ctxt . SetTheResultToStaticCString("can't set HTMLtext while images are loading");
        return;
    }
    state |= CS_NO_FILE; // prevent interactions while downloading images
    // MW-2012-03-08: [[ FieldImport ]] Use the new htmlText importer.
    setparagraphs(importhtmltext(value), p_part_id, p_start, p_finish);

    state &= ~CS_NO_FILE;
}

void MCField::GetStyledTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCArrayRef& r_value)
{
    if (exportasstyledtext(p_part_id, p_start, p_finish, false, false, r_value))
        return;

    ctxt . Throw();
}

void MCField::GetEffectiveStyledTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCArrayRef& r_value)
{
    if (exportasstyledtext(p_part_id, p_start, p_finish, false, true, r_value))
        return;

    ctxt . Throw();
}

void MCField::SetStyledTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCArrayRef value)
{
    state |= CS_NO_FILE; // prevent interactions while downloading images
    MCParagraph *stpgptr = styledtexttoparagraphs(value);

    if (stpgptr == nil)
        stpgptr = texttoparagraphs(kMCEmptyString);
        
    setparagraphs(stpgptr, p_part_id, p_start, p_finish);
    
    state &= ~CS_NO_FILE;
}

void MCField::GetFormattedStyledTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCArrayRef& r_value)
{
    if (exportasstyledtext(p_part_id, p_start, p_finish, true, false, r_value))
        return;

    ctxt . Throw();
}

void MCField::GetEffectiveFormattedStyledTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCArrayRef& r_value)
{
    if (exportasstyledtext(p_part_id, p_start, p_finish, true, true, r_value))
        return;

    ctxt . Throw();
}


//////////

// AL-2014-05-27: [[ Bug 12511 ]] charIndex is a char chunk property
void MCField::GetCharIndexOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t& r_value)
{
    findex_t t_char_index = si;
    unresolvechars(p_part_id, t_char_index, t_char_index);

    r_value = t_char_index + 1;
}

void MCField::GetLineIndexOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t& r_value)
{
    findex_t t_value;
    findex_t t_si = si;
    findex_t t_ei = ei;
    indextoparagraph(resolveparagraphs(p_part_id), t_si, t_ei, &t_value);
    r_value = t_value;
}

void MCField::GetFormattedTopOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t& r_value)
{
    if (opened)
    {
        coord_t x, y;
        MCParagraph *pgptr = resolveparagraphs(p_part_id);
        MCParagraph *sptr = indextoparagraph(pgptr, si, ei, nil);
        sptr -> indextoloc(si, fixedheight, x, y);
        r_value = getcontenty() + paragraphtoy(sptr) + y;
    }
    else
        r_value = 0;
}

void MCField::GetFormattedLeftOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t& r_value)
{
    // MW-2005-07-16: [[Bug 2938]] We must check to see if the field is open, if not we cannot do this.
    if (opened)
    {
        MCParagraph *pgptr = resolveparagraphs(p_part_id);
        MCParagraph *sptr = indextoparagraph(pgptr, si, ei, nil);
        coord_t minx, maxx;

        // MW-2008-07-08: [[ Bug 6331 ]] the formattedWidth can return gibberish for empty lines.
        //   This is because minx/maxx are uninitialized and it seems that they have to be for
        //   calls to getxextents() to make sense.
        minx = MCinfinity;
        maxx = -MCinfinity;

        do
        {
            // AL-2014-10-31: [[ Bug 13897 ]] Pass in correct values to getextents
            sptr->getxextents(si, ei, minx, maxx);
            sptr = sptr->next();
        }
        while (ei > 0 && sptr != pgptr);

        // MW-2008-07-08: [[ Bug 6331 ]] the formattedWidth can return gibberish for empty lines.
        //   If minx > maxx then just assume both are 0.
        if (minx > maxx)
            minx = maxx = 0;
        
        // AL-2014-10-28: [[ Bug 13829 ]] The formattedLeft should be floorf'd to give the correct integer value.
        r_value = floorf(getcontentx() + minx);
    }
    else
        r_value = 0;
}

void MCField::GetFormattedWidthOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t& r_value)
{
    if (opened)
    {
        MCParagraph *pgptr = resolveparagraphs(p_part_id);
        MCParagraph *sptr = indextoparagraph(pgptr, si, ei, nil);
        coord_t minx, maxx;

        // MW-2008-07-08: [[ Bug 6331 ]] the formattedWidth can return gibberish for empty lines.
        //   This is because minx/maxx are uninitialized and it seems that they have to be for
        //   calls to getxextents() to make sense.
        minx = MCinfinity;
        maxx = -MCinfinity;

        do
        {
            sptr->getxextents(si, ei, minx, maxx);
            sptr = sptr->next();
        }
        while (ei > 0 && sptr != pgptr);

        // MW-2008-07-08: [[ Bug 6331 ]] the formattedWidth can return gibberish for empty lines.
        //   If minx > maxx then just assume both are 0.
        if (minx > maxx)
            minx = maxx = 0;

        // AL-2014-10-28: [[ Bug 13829 ]] The formattedWidth should be ceilf'd to give the correct integer value.
        r_value = ceilf(maxx - minx);
    }
    else
        r_value = 0;
}

void MCField::GetFormattedHeightOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t& r_value)
{
    // MW-2005-07-16: [[Bug 2938]] We must check to see if the field is open, if not we cannot do this.
    if (opened)
    {
        coord_t x, y;
        MCParagraph *pgptr = resolveparagraphs(p_part_id);
        MCParagraph *sptr = indextoparagraph(resolveparagraphs(p_part_id), si, ei, nil);
        sptr->indextoloc(si, fixedheight, x, y);
        int4 maxy = 0;
        do
        {
            // FG-2014-12-03: [[ Bug 11688 ]] The margins get counted twice...
            //if (maxy != 0)
            //    maxy += sptr -> prev() -> computebottommargin() + sptr -> computetopmargin();
            maxy += sptr->getyextent(ei, fixedheight);
            ei -= sptr->gettextlengthcr();
            sptr = sptr->next();
        }
        while (ei > 0 && sptr != pgptr);

        r_value = maxy - y;
    }
    else
        r_value = 0;
}

void MCField::GetFormattedRectOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCRectangle32& r_value)
{
    // MW-2005-07-16: [[Bug 2938]] We must check to see if the field is open, if not we cannot do this.
    if (opened)
    {
        coord_t x, y;
        MCParagraph *pgptr = resolveparagraphs(p_part_id);
        MCParagraph *sptr = indextoparagraph(resolveparagraphs(p_part_id), si, ei, nil);
        sptr->indextoloc(si, fixedheight, x, y);
        // MW-2012-01-25: [[ FieldMetrics ]] Compute the yoffset in card-coords.
        coord_t yoffset = getcontenty() + paragraphtoy(sptr);
        coord_t minx, maxx;
        coord_t maxy = y;
        minx = FLT_MAX;
        maxx = FLT_MIN;
        do
        {
            // MW-2012-01-25: [[ FieldMetrics ]] Increment the y-extent by the height of the
            //   paragraph up to ei.
            maxy += sptr->getyextent(ei, fixedheight);
            sptr->getxextents(si, ei, minx, maxx);
            sptr = sptr->next();
        }
        while (ei > 0 && sptr != pgptr);

        /* Make sure minx is sensible if it is not set in getxextents() and there is only one
         * line. */
        if (minx > maxx)
            minx = maxx;
        
        // MW-2012-01-25: [[ FieldMetrics ]] Make sure the rect we return is in card coords.
        r_value . height = maxy - 2*y;
        r_value . width = ceilf(maxx - minx);
        r_value . x = floorf(minx) + getcontentx();
        r_value . y = y + yoffset;
    }
    else
        r_value = MCRectangle32{};
}


//////////

void MCField::GetLinkTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef& r_value)
{
    bool t_mixed;
    GetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, p_part_id, si, si, &MCBlock::GetLinkText, false, (MCStringRef)nil, t_mixed, r_value);
}

void MCField::SetLinkTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef value)
{
    SetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetLinktext, value);
}

void MCField::GetMetadataOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef& r_value)
{
    bool t_mixed;
    MCAutoStringRef t_metadata;
    GetParagraphPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetMetadata, t_mixed, &t_metadata);

    if (*t_metadata == nil)
        r_value = MCValueRetain(kMCEmptyString);
    else
        r_value = MCValueRetain(*t_metadata);
}

void MCField::SetMetadataOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef value)
{
    SetParagraphPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, false, p_part_id, si, ei, &MCParagraph::SetMetadata, value);
}

void MCField::GetMetadataOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef& r_value)
{
    bool t_mixed;
    GetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, p_part_id, si, si, &MCBlock::GetMetadata, false, (MCStringRef)nil, t_mixed, r_value);
}

void MCField::SetMetadataOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef value)
{
    SetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetMetadata, value);
}

void MCField::GetImageSourceOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef& r_value)
{
    bool t_mixed;
    GetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, p_part_id, si, si, &MCBlock::GetImageSource, false, (MCStringRef)nil, t_mixed, r_value);
}

void MCField::SetImageSourceOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef value)
{
    if (si == ei)
        return;

    SetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, true, p_part_id, si, ei, &MCBlock::SetImageSource, value);
}

void MCField::GetVisitedOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_value)
{
    bool t_mixed;
    GetCharPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetVisited, false, false, t_mixed, r_value);
}

// PM-2015-07-06: [[ Bug 15577 ]] Allow setting of the "visited" property of a block
void MCField::SetVisitedOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool p_value)
{
    SetCharPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetVisited,p_value);
}

void MCField::GetEncodingOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, intenum_t& r_encoding)
{
    MCAutoStringRef t_value;
    if (!exportastext(p_part_id, p_start, p_finish, &t_value))
    {
        ctxt.Throw();
        return;
    }
     
    if (MCStringCanBeNative(*t_value))
    {
        r_encoding = 0;
    }
    else
    {
        r_encoding = 1;
    }
}

void MCField::GetFlaggedOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& r_value)
{
    GetCharPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetFlagged, false, false, r_mixed, r_value);
}

void MCField::SetFlaggedOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool value)
{
    SetCharPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetFlagged, value);
}

void MCField::GetFlaggedRangesOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCInterfaceFieldRanges& r_value)
{
    integer_t t_index_offset;
    t_index_offset = -countchars(p_part_id, 0, si);

    MCParagraph *pgptr = resolveparagraphs(p_part_id);
    MCParagraph *sptr = indextoparagraph(pgptr, si, ei, nil);

    MCAutoArray<MCInterfaceFieldRange> t_ranges;

    do
    {
        MCInterfaceFieldRanges t_paragraphRanges;
        sptr -> getflaggedranges(p_part_id, si, ei, t_index_offset, t_paragraphRanges);
		// PM-2016-01-08: [[ Bug 16666 ]] Update the offset to be relative to the beginning of the text
		t_index_offset += sptr -> gettextlengthcr();

        for (uindex_t i = 0; i < t_paragraphRanges . count; ++i)
            t_ranges . Push(t_paragraphRanges . ranges[i]);

        sptr = sptr -> next();
    }
    while (sptr -> gettextlengthcr() < ei && sptr != pgptr);

    t_ranges . Take(r_value . ranges, r_value . count);
}

void MCField::SetFlaggedRangesOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceFieldRanges& value)
{
    MCParagraph *pgptr = resolveparagraphs(p_part_id);
    MCParagraph *sptr;

    // ----------------------------
    // Unflag the range [si;ei]
    // ----------------------------
    findex_t t_si = si;
    findex_t t_ei = ei;
    sptr = indextoparagraph(pgptr, t_si, t_ei, nil);
    MCBlock *t_block = sptr -> getblocks();

    // skip the blocks outside [si;ei]
    while (t_si + t_block -> GetLength() < si)
    {
        t_si += t_block -> GetLength();
        t_block = t_block -> next();
    }

    // block flagged and we're not at its exact beginning: must split it
    // and skip the first part
    if (t_block -> getflagged()
            && t_si != si)
    {
        t_block -> split(si);
        t_si += t_block -> GetLength();
        t_block = t_block -> next();
    }

    // Unflag all the blocks within [si;ei[
    while (t_si  + t_block -> GetLength() < ei)
    {
        t_block  -> SetFlagged(ctxt, false);
        t_si += t_block -> GetLength();
        t_block = t_block -> next();
    }

    // block flagged and we're not at its exact end: must split it
    // and unflag the first part
    if (t_block ->getflagged()
            && t_si != ei)
    {
        t_block -> split(ei);
        t_block -> SetFlagged(ctxt, false);
    }

    // ---------------------------------
    // Flag the appropriate ranges
    // ---------------------------------

    // 'set the flaggedRanges of char 10 to 20 to "1,4"' affects char 10 to 14
    uindex_t t_range_offset = si;

    // The position of the beginning of the current paragraph.
    findex_t t_paragraph_offset = si;

    // get the first paragraph within the bounds given and update the position in the text
    sptr = indextoparagraph(pgptr, t_paragraph_offset, ei, nil);

    // The index of the range currently considered
    uindex_t t_range_index;
    t_range_index = 0;

    // Contains the remaining range to flag in case a range
    // covers more than one paragraph
    MCInterfaceFieldRange t_next_range;
    t_next_range = value . ranges[t_range_index];
    t_next_range . start += t_range_offset;
    t_next_range . end += t_range_offset;

    // Loop while there is a range to flag and and we haven't gone further than ei
    while (t_range_index < value . count
           && (findex_t) t_next_range . start < ei
           && t_paragraph_offset < ei)
    {
        // if the next range doesn't cover this paragraph, we skip the paragraph
        if ((findex_t) t_next_range . start > t_paragraph_offset + sptr -> gettextlengthcr())
        {
            t_paragraph_offset += sptr -> gettextlengthcr();
            sptr = sptr -> next();
            continue;
        }

        MCBlock *bptr = sptr -> getblocks();

        // t_block_offset keeps the position in the text
        integer_t t_block_offset = t_paragraph_offset;

        // while there is a range to flaf and we haven't gone further than ei
        // and there are blocks to be checked
        while (t_range_index < value . count
               && t_block_offset < sptr -> gettextlengthcr()
               && t_block_offset < ei)
        {
            // skip block if it's not covered by the next range
            if ((findex_t) t_next_range . start > t_block_offset + bptr -> GetLength())
            {
                t_block_offset += bptr -> GetLength();
                bptr = bptr -> next();
                continue;
            }

            // if the range doesn't start at the beginning of the block
            // we must split the block and skip the first part
            if ((findex_t) t_next_range . start > t_block_offset)
            {
                bptr -> split(t_next_range . start);
                t_block_offset += bptr -> GetLength();
                bptr = bptr -> next();
            }

            // if the range doesn't cover the block up to its end
            // we must split it
            if ((findex_t) t_next_range . end < t_block_offset + bptr -> GetLength())
                bptr -> split(t_next_range . end);

            // Flag the block
            bptr -> SetFlagged(ctxt, True);

            // if the range went further than the block
            // we must keep track of this and update the next range to be flagged
            if ((findex_t) t_next_range . end > t_block_offset + bptr -> GetLength())
                t_next_range . start = t_block_offset + bptr -> GetLength();
            // otherwise we set the next range to the appropriate value
            else if (t_range_index < value . count)
            {
                t_next_range = value . ranges[t_range_index++];
                t_next_range . start += t_range_offset;
                t_next_range . end += t_range_offset;
            }

            // update the position in the text
            // and switch to the next block
            t_block_offset += bptr -> GetLength();
            bptr = bptr -> next();
        }

        // update the paragraph offset
        t_paragraph_offset += sptr -> gettextlengthcr();
    }

}

//////////
// Paragraph list properties
//////////

void MCField::GetListStyleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, intenum_t& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<intenum_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetListStyle, r_mixed, r_value);
}

void MCField::SetListStyleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, intenum_t value)
{
    SetParagraphPropOfCharChunk< PodFieldPropType<intenum_t> >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetListStyle, value);
}

void MCField::GetListDepthOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetListDepth, r_mixed, r_value);
}

void MCField::SetListDepthOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *value)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetListDepth, value);
}

void MCField::GetListIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetListIndent, r_mixed, r_value);
}

void MCField::SetListIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t *value)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetListIndent, value);
}

void MCField::GetListIndexOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetListIndex, r_mixed, r_value);
}

void MCField::GetEffectiveListIndexOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveListIndex, r_mixed, r_value);
}
void MCField::SetListIndexOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *value)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetListIndex, value);
}

//////////
// Paragraph indent properties
//////////

void MCField::GetFirstIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetFirstIndent, r_mixed, r_value);
}

void MCField::GetEffectiveFirstIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<integer_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveFirstIndent, r_mixed, r_value);
}

void MCField::SetFirstIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t *p_indent)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetFirstIndent, p_indent);
}

void MCField::GetLeftIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetLeftIndent, r_mixed, r_value);
}

void MCField::GetEffectiveLeftIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<integer_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveLeftIndent, r_mixed, r_value);
}

void MCField::SetLeftIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t *p_indent)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetLeftIndent, p_indent);
}

void MCField::GetRightIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetRightIndent, r_mixed, r_value);
}

void MCField::GetEffectiveRightIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<integer_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveRightIndent, r_mixed, r_value);
}

void MCField::SetRightIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t *p_indent)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetRightIndent, p_indent);
}

void MCField::GetSpaceAboveOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetSpaceAbove, r_mixed, r_value);
}

void MCField::GetEffectiveSpaceAboveOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveSpaceAbove, r_mixed, r_value);
}

void MCField::SetSpaceAboveOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *p_space)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetSpaceAbove, p_space);
}

void MCField::GetSpaceBelowOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetSpaceBelow, r_mixed, r_value);
}

void MCField::GetEffectiveSpaceBelowOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveSpaceBelow, r_mixed, r_value);
}

void MCField::SetSpaceBelowOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *p_space)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetSpaceBelow, p_space);
}

//////////
// Paragraph tabs Properties
//////////

void MCField::GetTabStopsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uindex_t& r_count, uinteger_t*& r_values)
{
    vector_t<uinteger_t> t_vector;
    GetParagraphPropOfCharChunk< VectorFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetTabStops, r_mixed, t_vector);

    // SN-2015-04-22: [[ Bug 15243 ]] Do not use t_vector if the result is mixed
    //  as it will be uninitialised.
    if (!r_mixed)
    {
        r_count = t_vector . count;
        r_values = t_vector . elements;
    }
}

void MCField::GetEffectiveTabStopsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uindex_t& r_count, uinteger_t*& r_values)
{
    vector_t<uinteger_t> t_vector;
    GetParagraphPropOfCharChunk< VectorFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveTabStops, r_mixed, t_vector);
    
    // SN-2015-04-22: [[ Bug 15243 ]] Do not use t_vector if the result is mixed
    //  as it will be uninitialised.
    if (!r_mixed)
    {
        r_count = t_vector.count;
        r_values = t_vector.elements;
    }
}

void MCField::SetTabStopsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uindex_t count, uinteger_t *values)
{
    vector_t<uinteger_t> t_vector;
    t_vector . count = count;
    t_vector . elements = values;

    SetParagraphPropOfCharChunk< VectorFieldPropType<uinteger_t > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetTabStops, t_vector);
}

void MCField::GetTabWidthsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uindex_t& r_count, uinteger_t*& r_values)
{
    vector_t<uinteger_t> t_vector;
    GetParagraphPropOfCharChunk< VectorFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetTabWidths, r_mixed, t_vector);
    
    // SN-2015-04-22: [[ Bug 15243 ]] Do not use t_vector if the result is mixed
    //  as it will be uninitialised.
    if (!r_mixed)
    {
        r_count = t_vector . count;
        r_values = t_vector . elements;
    }
}

void MCField::GetEffectiveTabWidthsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uindex_t& r_count, uinteger_t*& r_values)
{
    vector_t<uinteger_t> t_vector;
    GetParagraphPropOfCharChunk< VectorFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveTabWidths, r_mixed, t_vector);
    
    // SN-2015-04-22: [[ Bug 15243 ]] Do not use t_vector if the result is mixed
    //  as it will be uninitialised.
    if (!r_mixed)
    {
        r_count = t_vector.count;
        r_values = t_vector.elements;
    }
}

void MCField::SetTabWidthsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uindex_t count, uinteger_t *values)
{
    vector_t<uinteger_t> t_vector;
    t_vector . count = count;
    t_vector . elements = values;

    SetParagraphPropOfCharChunk< VectorFieldPropType<uinteger_t> >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetTabWidths, t_vector);
}

void MCField::GetTabAlignmentsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceFieldTabAlignments &r_values)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<MCInterfaceFieldTabAlignments> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetTabAlignments, r_mixed, r_values);
}

void MCField::SetTabAlignmentsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceFieldTabAlignments &p_values)
{
    SetParagraphPropOfCharChunk< PodFieldPropType<MCInterfaceFieldTabAlignments> >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetTabAlignments, p_values);
}

void MCField::GetEffectiveTabAlignmentsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceFieldTabAlignments &r_values)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<MCInterfaceFieldTabAlignments> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveTabAlignments, r_mixed, r_values);
}

//////////
// Paragraph border properties
//////////

void MCField::GetBorderWidthOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetBorderWidth, r_mixed, r_value);
}

void MCField::GetEffectiveBorderWidthOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveBorderWidth, r_mixed, r_value);
}

void MCField::SetBorderWidthOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *p_width)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetBorderWidth, p_width);
}

//////////
// Paragraph color properties
//////////

void MCField::GetBackColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetBackColor, r_mixed, r_color);
}

void MCField::GetEffectiveBackColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveBackColor, r_mixed, r_color);
}

void MCField::SetBackColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceNamedColor& p_color)
{
    SetParagraphPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, false, p_part_id, si, ei, &MCParagraph::SetBackColor, p_color);

    // AL-2014-11-18: [[ Bug 14049 ]] Redraw without relayout after changing paragraph color
    MCObject::Redraw();
}

void MCField::GetBorderColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetBorderColor, r_mixed, r_color);
}

void MCField::GetEffectiveBorderColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveBorderColor, r_mixed, r_color);
}

void MCField::SetBorderColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceNamedColor& p_color)
{
    SetParagraphPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetBorderColor, p_color);
}

//////////
// Paragraph grid properties
//////////

void MCField::GetHGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<bool> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetHGrid, r_mixed, r_value);
}

void MCField::GetEffectiveHGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveHGrid, r_mixed, r_value);
}

void MCField::SetHGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool *p_has_hgrid)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<bool> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetHGrid, p_has_hgrid);
}

void MCField::GetVGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<bool> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetVGrid, r_mixed, r_value);
}

void MCField::GetEffectiveVGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveVGrid, r_mixed, value);
}

void MCField::SetVGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool *p_has_vgrid)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<bool> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetVGrid, p_has_vgrid);
}

void MCField::GetDontWrapOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<bool> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetDontWrap, r_mixed, r_value);
}

void MCField::GetEffectiveDontWrapOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveDontWrap, r_mixed, value);
}

void MCField::SetDontWrapOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool *p_has_dont_wrap)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<bool> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetDontWrap, p_has_dont_wrap);
}

//////////
// Paragraph padding properties
//////////

void MCField::GetPaddingOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value)
{
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetPadding, r_mixed, r_value);
}

void MCField::GetEffectivePaddingOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectivePadding, r_mixed, r_value);
}

void MCField::SetPaddingOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *p_padding)
{
    SetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<uinteger_t> > >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetPadding, p_padding);
}

//////////
// Paragraph invisible properties
//////////

void MCField::GetInvisibleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& r_value)
{
    GetParagraphPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveInvisible, r_mixed, r_value);
}

void MCField::SetInvisibleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool p_invisible)
{
    SetParagraphPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetInvisible, p_invisible);
}

//////////
// AB : 2017-06-07 Fix bug 18407
//////////

void MCField::GetVisibleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& r_value)
{
    GetInvisibleOfLineChunk(ctxt, p_part_id, si, ei, r_mixed, r_value);
    r_value = !r_value;
}
void MCField::SetVisibleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool p_visible)
{
    SetInvisibleOfLineChunk(ctxt, p_part_id, si, ei, !p_visible);
}

//////////
// Block color properties
//////////

void MCField::GetForeColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    GetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetForeColor, false, r_color, r_mixed, r_color);
}

void MCField::GetEffectiveForeColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    MCInterfaceNamedColor t_default_color;
    GetEffectiveForeColor(ctxt, t_default_color);
    
    GetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetForeColor, true, t_default_color, r_mixed, r_color);

    if (r_mixed)
    {
        MCInterfaceNamedColorFree(ctxt, t_default_color);
        return;
    }
}

void MCField::SetForeColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceNamedColor& color)
{
    SetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetForeColor, color);
    
    // AL-2014-08-04: [[ Bug 13076 ]] Redraw without relayout after changing block color
    MCObject::Redraw();
}

void MCField::GetBackColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    GetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetBackColor, false, r_color, r_mixed, r_color);
}

void MCField::GetEffectiveBackColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    MCInterfaceNamedColor t_default_color;
    GetEffectiveBackColor(ctxt, t_default_color);
    
    GetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetBackColor, true, t_default_color, r_mixed, r_color);

    if (r_mixed)
    {
        MCInterfaceNamedColorFree(ctxt, r_color);
        return;
    }
}

void MCField::SetBackColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceNamedColor& color)
{
    SetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetBackColor, color);
    
    // AL-2014-08-04: [[ Bug 13076 ]] Redraw without relayout after changing block color
    MCObject::Redraw();
}

//////////
// Block text properties
//////////

void MCField::GetTextFontOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCStringRef& r_value)
{
    GetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextFont, false, (MCStringRef)nil, r_mixed, r_value);
}

void MCField::GetEffectiveTextFontOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCStringRef& r_value)
{
    MCAutoStringRef t_value;
    MCAutoStringRef t_default;
    GetEffectiveTextFont(ctxt, r_value);
    
    GetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextFont, true, *t_default, r_mixed, &t_value);

    if (r_mixed)
        return;
}

void MCField::SetTextFontOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef p_value)
{
    SetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, true, p_part_id, si, ei, &MCBlock::SetTextFont, p_value);
}

void MCField::GetTextStyleOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceTextStyle& r_value)
{
    GetCharPropOfCharChunk< PodFieldPropType<MCInterfaceTextStyle> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextStyle, false, r_value, r_mixed, r_value);
}

void MCField::GetEffectiveTextStyleOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceTextStyle& r_value)
{
    MCInterfaceTextStyle t_value;
    GetEffectiveTextStyle(ctxt, t_value);
    GetCharPropOfCharChunk< PodFieldPropType<MCInterfaceTextStyle> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextStyle, true, t_value, r_mixed, r_value);

    if (r_mixed)
        return;
}

void MCField::SetTextStyleOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceTextStyle& p_value)
{
    // AL-2014-09-22: [[ Bug 11817 ]] Don't necessarily recompute the whole field when changing text styles
    SetCharPropOfCharChunk< PodFieldPropType<MCInterfaceTextStyle> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetTextStyle, p_value);
}

void MCField::GetTextShiftOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t*& r_value)
{
    GetCharPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextShift, false, 0, r_mixed, r_value);
}

void MCField::GetEffectiveTextShiftOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t& r_value)
{
    integer_t t_value = 0;
    integer_t *t_value_ptr = &t_value;
    GetCharPropOfCharChunk< OptionalFieldPropType<PodFieldPropType<integer_t> > >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextShift, true, 0, r_mixed, t_value_ptr);

    if (r_mixed)
        return;

    r_value = t_value;
}

void MCField::SetTextShiftOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t* p_value)
{
    SetCharPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, true, p_part_id, si, ei, &MCBlock::SetTextShift, p_value);
}

void MCField::GetTextStyleElementOfCharChunk(MCExecContext& ctxt, MCNameRef p_index, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool*& r_value)
{
    GetArrayCharPropOfCharChunk< OptionalFieldArrayPropType< PodFieldArrayPropType<bool> > >(ctxt, this, p_part_id, si, ei, p_index, &MCBlock::GetTextStyleElement, false, false, r_mixed, r_value);
}

void MCField::GetEffectiveTextStyleElementOfCharChunk(MCExecContext& ctxt, MCNameRef p_index, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& r_value)
{
    // SN-2015-03-25: [[ Bug 15030 ]] Initialise t_value_ptr to a value, since
    //  it will be dereferenced in MCBlock::GetTextStyleElement
    bool t_default, t_value;
    bool *t_value_ptr = &t_value;
    GetTextStyleElement(ctxt, p_index, t_default);
    GetArrayCharPropOfCharChunk< OptionalFieldArrayPropType< PodFieldArrayPropType<bool> > >(ctxt, this, p_part_id, si, ei, p_index, &MCBlock::GetTextStyleElement, true, t_default, r_mixed, t_value_ptr);
    
    r_value = *t_value_ptr;
}

void MCField::SetTextStyleElementOfCharChunk(MCExecContext& ctxt, MCNameRef p_index, uint32_t p_part_id, int32_t si, int32_t ei, bool *p_value)
{
    bool t_value;
    if (p_value == nil)
        t_value = false;
    else
        t_value = *p_value;
    
    // AL-2014-09-22: [[ Bug 11817 ]] Don't necessarily recompute the whole field when changing text styles
    SetArrayCharPropOfCharChunk< PodFieldArrayPropType<bool> >(ctxt, this, false, p_part_id, si, ei, p_index, &MCBlock::SetTextStyleElement, t_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCParagraph::GetEncoding(MCExecContext &ctxt, intenum_t &r_encoding)
{
    if (MCStringIsNative(*m_text))
        r_encoding = 0; // nativestring
    else
        r_encoding = 1; // unicode
}

void MCParagraph::GetTextAlign(MCExecContext& ctxt, intenum_t*& r_value)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_TEXT_ALIGN) == 0)
        r_value = nil;
    else
        // SN-2014-07-28: [[ Bug 12925 ]] The output value needs to be translated as well
        *r_value = gettextalign() << F_ALIGNMENT_SHIFT;
}

void MCParagraph::GetEffectiveTextAlign(MCExecContext& ctxt, intenum_t& r_value)
{
    // SN-2014-07-28: [[ Bug 12925 ]] The output value needs to be translated as well
    r_value = gettextalign() << F_ALIGNMENT_SHIFT;
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
        // The incoming values need to be translated
        intenum_t t_value;
        t_value = *p_value >> F_ALIGNMENT_SHIFT;
        
        if (attrs == nil)
            attrs = new (nothrow) MCParagraphAttrs;
        attrs -> flags |= PA_HAS_TEXT_ALIGN;
        attrs -> text_align = t_value;
    }
}

void MCParagraph::GetListStyle(MCExecContext& ctxt, intenum_t& r_style)
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
		// PM-2016-01-19: [[ Bug 16742 ]] Default listDepth should be 1 (to match the LC 6.x behavior)
        *r_depth = getlistdepth() + 1;
}

void MCParagraph::GetEffectiveListDepth(MCExecContext& ctxt, uinteger_t& r_depth)
{
    r_depth = getlistdepth();
}

void MCParagraph::SetListDepth(MCExecContext& ctxt, uinteger_t* p_depth)
{
    uinteger_t t_depth;

    // Do not make any changes if they aren't desired
    if (p_depth == nil)
        return;
    
    t_depth = *p_depth;

    if (t_depth < 1 || t_depth > 16)
    {
        ctxt . Throw();
        return;
    }

    if (attrs == nil)
        attrs = new (nothrow) MCParagraphAttrs;

    if ((attrs -> flags & PA_HAS_LIST_STYLE) == 0)
    {
        attrs -> flags |= PA_HAS_LIST_STYLE;
        attrs -> list_style = kMCParagraphListStyleDisc;
    }

    attrs -> list_depth = t_depth - 1;
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

void MCParagraph::GetSpaceAbove(MCExecContext& ctxt, uinteger_t*& r_space)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_SPACE_ABOVE) == 0)
        r_space = nil;
    else
        *r_space = (uinteger_t)getspaceabove();
}

void MCParagraph::GetEffectiveSpaceAbove(MCExecContext& ctxt, uinteger_t& r_space)
{
    r_space = (uinteger_t)getspaceabove();
}

void MCParagraph::SetSpaceAbove(MCExecContext& ctxt, uinteger_t *p_space)
{
    setparagraphattr_uint16(attrs, PA_HAS_SPACE_ABOVE, offsetof(MCParagraphAttrs, space_above), p_space);
}

void MCParagraph::GetSpaceBelow(MCExecContext& ctxt, uinteger_t *&r_space)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_SPACE_BELOW) == 0)
        r_space = nil;
    else
        *r_space = (uinteger_t)getspacebelow();
}

void MCParagraph::GetEffectiveSpaceBelow(MCExecContext& ctxt, uinteger_t& r_space)
{
    r_space = (uinteger_t)getspacebelow();
}

void MCParagraph::SetSpaceBelow(MCExecContext& ctxt, uinteger_t *p_space)
{
    setparagraphattr_uint16(attrs, PA_HAS_SPACE_BELOW, offsetof(MCParagraphAttrs, space_below), p_space);
}

void MCParagraph::DoSetTabStops(MCExecContext &ctxt, bool p_is_relative, const vector_t<uinteger_t>& p_tabs)
{
    uint2 *t_new_tabs = nil;
    uindex_t t_new_count = 0;
    
    MCInterfaceTabStopsParse(ctxt, p_is_relative, p_tabs . elements, p_tabs . count, t_new_tabs, t_new_count);

    if (attrs == nil)
        attrs = new (nothrow) MCParagraphAttrs;
    else
        delete attrs -> tabs;

    if (t_new_tabs != nil)
    {
        attrs -> tabs = t_new_tabs;
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

void MCParagraph::DoGetTabStops(MCExecContext &ctxt, bool p_is_relative, vector_t<uinteger_t>& r_tabs)
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

    t_tabs_formatted . Take(r_tabs . elements, r_tabs . count);
}

void MCParagraph::GetTabStops(MCExecContext& ctxt, vector_t<uinteger_t> &r_tabs)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_TABS) == 0)
    {
        r_tabs . count = 0;
        r_tabs . elements = nil;
    }
    else
        DoGetTabStops(ctxt, false, r_tabs);
}

void MCParagraph::GetEffectiveTabStops(MCExecContext& ctxt, vector_t<uinteger_t> &r_tabs)
{
    DoGetTabStops(ctxt, false, r_tabs);
}

void MCParagraph::SetTabStops(MCExecContext& ctxt, const vector_t<uinteger_t>& p_tabs)
{
    DoSetTabStops(ctxt, false, p_tabs);
}

void MCParagraph::GetTabWidths(MCExecContext& ctxt, vector_t<uinteger_t> &r_tabs)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_TABS) == 0)
    {
        r_tabs . count = 0;
        r_tabs . elements = nil;
    }
    else
        DoGetTabStops(ctxt, true, r_tabs);
}

void MCParagraph::GetEffectiveTabWidths(MCExecContext& ctxt, vector_t<uinteger_t> &r_tabs)
{
    DoGetTabStops(ctxt, true, r_tabs);
}

void MCParagraph::SetTabWidths(MCExecContext& ctxt, const vector_t<uinteger_t>& p_tabs)
{
    DoSetTabStops(ctxt, true, p_tabs);
}

void MCParagraph::GetTabAlignments(MCExecContext& ctxt, MCInterfaceFieldTabAlignments& r_alignments)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_TAB_ALIGNMENTS) == 0)
    {
        r_alignments . m_count = 0;
        r_alignments . m_alignments = nil;
    }
    else
    {
        GetEffectiveTabAlignments(ctxt, r_alignments);
    }
}

void MCParagraph::GetEffectiveTabAlignments(MCExecContext& ctxt, MCInterfaceFieldTabAlignments& r_alignments)
{
    if (attrs != nil && (attrs -> flags & PA_HAS_TAB_ALIGNMENTS))
    {
        MCMemoryAllocateCopy(attrs -> alignments, attrs -> alignments_count * sizeof(intenum_t), r_alignments . m_alignments);
        r_alignments . m_count = attrs -> alignments_count;
    }
    else
    {
        parent->GetTabAlignments(ctxt, r_alignments);
    }
}

void MCParagraph::SetTabAlignments(MCExecContext& ctxt, const MCInterfaceFieldTabAlignments& p_alignments)
{
    if (attrs == nil)
        attrs = new (nothrow) MCParagraphAttrs;
    else
        delete attrs -> alignments;
    
    MCMemoryAllocateCopy(p_alignments . m_alignments, p_alignments . m_count * sizeof(intenum_t), attrs -> alignments);
    attrs -> alignments_count = p_alignments . m_count;
    
    if (attrs -> alignments != nil)
        attrs -> flags |= PA_HAS_TAB_ALIGNMENTS;
    else
        attrs -> flags &= ~PA_HAS_TAB_ALIGNMENTS;
}

void MCParagraph::GetBackColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_BACKGROUND_COLOR) == 0)
        r_color . name = MCValueRetain(kMCEmptyString);
    else
    {
        MCColor t_color;
		MCColorSetPixel(t_color, attrs -> background_color);
        get_interface_color(t_color, nil, r_color);
    }
}

void MCParagraph::GetEffectiveBackColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color)
{
    if (attrs != nil && (attrs -> flags & PA_HAS_BACKGROUND_COLOR) != 0)
    {
        MCColor t_color;
		MCColorSetPixel(t_color, attrs -> background_color);
        get_interface_color(t_color, nil, r_color);
    }
}

void MCParagraph::SetBackColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color)
{
    setparagraphattr_color(attrs, PA_HAS_BACKGROUND_COLOR, offsetof(MCParagraphAttrs, background_color), p_color);
}

void MCParagraph::GetBorderColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_BORDER_COLOR) == 0)
        r_color . name = MCValueRetain(kMCEmptyString); // an empty name is an empty MCInterfaceNamedColor
    else
    {
        MCColor t_color;
		MCColorSetPixel(t_color, attrs -> border_color);
        get_interface_color(t_color, nil, r_color);
    }
}

void MCParagraph::GetEffectiveBorderColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
    if (attrs != nil && (attrs -> flags & PA_HAS_BORDER_COLOR) != 0)
    {
        MCColor t_color;
		MCColorSetPixel(t_color, attrs -> border_color);
        get_interface_color(t_color, nil, r_color);
    }
}

void MCParagraph::SetBorderColor(MCExecContext& ctxt, const MCInterfaceNamedColor &p_color)
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
void MCParagraph::SetInvisible(MCExecContext &ctxt, bool p_invisible)
{
    bool t_new_value;
    setparagraphattr_bool(attrs, PA_HAS_HIDDEN, &p_invisible, t_new_value);

    if (attrs != nil)
    {
        attrs -> hidden = t_new_value;
        if (!t_new_value)
            attrs -> flags &= ~PA_HAS_HIDDEN;
    }
}

void MCParagraph::GetMetadata(MCExecContext& ctxt, MCStringRef &r_metadata)
{
    r_metadata = MCValueRetain(getmetadata());
}

void MCParagraph::GetEffectiveMetadata(MCExecContext& ctxt, MCStringRef &r_metadata)
{
    r_metadata = MCValueRetain(getmetadata());
}

// MW-2012-11-13: [[ ParaMetadata ]] Set the metadata attribute.
void MCParagraph::SetMetadata(MCExecContext& ctxt, MCStringRef p_metadata)
{
    if (p_metadata == nil)
    {
        if (attrs != nil)
        {
            attrs -> flags &= ~PA_HAS_METADATA;
            MCValueRelease(attrs -> metadata);
            attrs -> metadata = nil;
        }
    }
    else
    {
        if (attrs == nil)
            attrs = new (nothrow) MCParagraphAttrs;

        attrs -> flags |= PA_HAS_METADATA;
        MCValueInter(p_metadata, attrs -> metadata);
    }
}

// SN-28-11-13: The IDE needs to set char chunk properties for a specific paragraph
void MCParagraph::SetForeColorOfCharChunk(MCExecContext &ctxt, findex_t si, findex_t ei, const MCInterfaceNamedColor &p_color)
{
    SetCharPropOfCharChunkOfParagraph<PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, si, ei, &MCBlock::SetForeColor, p_color);
}

void MCParagraph::SetTextStyleOfCharChunk(MCExecContext &ctxt, findex_t si, findex_t ei, const MCInterfaceTextStyle &p_text)
{
    SetCharPropOfCharChunkOfParagraph<PodFieldPropType<MCInterfaceTextStyle> >(ctxt, this, si, ei, &MCBlock::SetTextStyle, p_text);
}

void MCParagraph::SetTextFontOfCharChunk(MCExecContext &ctxt, findex_t si, findex_t ei, MCStringRef p_fontname)
{
    SetCharPropOfCharChunkOfParagraph<PodFieldPropType<MCStringRef> >(ctxt, this, si, ei, &MCBlock::SetTextFont, p_fontname);
}

void MCParagraph::SetTextSizeOfCharChunk(MCExecContext &ctxt, findex_t si, findex_t ei, uinteger_t *p_size)
{
    SetCharPropOfCharChunkOfParagraph<OptionalFieldPropType<PodFieldPropType<uinteger_t> > >(ctxt, this, si, ei, &MCBlock::SetTextSize, p_size);
}

///////////////////////////////////////////////////////////////////////////////

void MCBlock::GetLinkText(MCExecContext& ctxt, MCStringRef& r_linktext)
{
    if (getlinktext())
        r_linktext = MCValueRetain(getlinktext());
    else
        r_linktext = MCValueRetain(kMCEmptyString);
}

void MCBlock::SetLinktext(MCExecContext& ctxt, MCStringRef p_linktext)
{
    if (flags & F_HAS_LINK)
    {
        MCValueRelease(atts -> linktext);
        atts -> linktext = nil;
    }

    if (MCStringIsEmpty(p_linktext))
        flags &= ~F_HAS_LINK;
    else
    {
        if (atts == nil)
            atts = new (nothrow) Blockatts;

        /* UNCHECKED */ MCValueInter(p_linktext, atts -> linktext);

        flags |= F_HAS_LINK;
    }
}

void MCBlock::GetMetadata(MCExecContext& ctxt, MCStringRef& r_metadata)
{
    r_metadata = MCValueRetain(getmetadata());
}

void MCBlock::SetMetadata(MCExecContext& ctxt, MCStringRef p_metadata)
{
    // MW-2012-01-06: [[ Block Metadata ]] Handle setting/unsetting the metadata
    //   property.
    if (flags & F_HAS_METADATA)
    {
        MCValueRelease(atts -> metadata);
        atts -> metadata = nil;
    }

    if (MCStringIsEmpty(p_metadata))
        flags &= ~F_HAS_METADATA;
    else
    {
        if (atts == nil)
            atts = new (nothrow) Blockatts;

        /* UNCHECKED */ MCValueInter((MCStringRef)p_metadata, atts -> metadata);

        flags |= F_HAS_METADATA;
    }
}

void MCBlock::GetImageSource(MCExecContext& ctxt, MCStringRef& r_image_source)
{
    if (getimagesource())
        r_image_source = MCValueRetain(getimagesource());
    else
        r_image_source = MCValueRetain(kMCEmptyString);
}

void MCBlock::SetImageSource(MCExecContext& ctxt, MCStringRef p_image_source)
{
    if (flags & F_HAS_IMAGE)
    {
        if (opened)
            closeimage();

        MCValueRelease(atts -> imagesource);
        atts -> imagesource = nil;
    }

    if (MCStringIsEmpty(p_image_source))
        flags &= ~F_HAS_IMAGE;
    else
    {
        if (atts == NULL)
            atts = new (nothrow) Blockatts;

        /* UNCHECKED */ MCValueInter(p_image_source, atts -> imagesource);

        atts->image = NULL;
        flags |= F_HAS_IMAGE;
    }
    if (opened)
        openimage();
}

void MCBlock::GetVisited(MCExecContext& ctxt, bool& r_value)
{
    r_value = getvisited() == True;
}

void MCBlock::SetVisited(MCExecContext& ctxt, bool p_value)
{
    if (p_value)
		setvisited();
	else
		clearvisited();
}

void MCBlock::GetFlagged(MCExecContext& ctxt, bool &r_value)
{
    r_value = getflagged();
}

void MCBlock::SetFlagged(MCExecContext& ctxt, bool p_value)
{
    // MW-2012-01-26: [[ FlaggedField ]] Set the appropriate flag.
    if (p_value)
        flags |= F_FLAGGED;
    else
        flags &= ~F_FLAGGED;
}

void MCBlock::GetTextFont(MCExecContext& ctxt, MCStringRef &r_fontname)
{
    // Note: gettextfont does not do a ValueRetain
    MCNameRef t_fontname;
    if (!gettextfont(t_fontname))
        r_fontname = nil;
    else
        r_fontname = MCValueRetain(MCNameGetString(t_fontname));
}

void MCBlock::SetTextFont(MCExecContext& ctxt, MCStringRef p_fontname)
{
    if (p_fontname == nil || MCStringIsEmpty(p_fontname))
    {
        flags &= ~F_HAS_FNAME;
        if (atts != nil)
        {
            MCValueRelease(atts -> fontname);
            atts -> fontname = nil;
        }
    }
    else
    {
        if (atts == nil)
            atts = new (nothrow) Blockatts;
        
        flags |= F_HAS_FNAME;
        /* UNCHECKED */ MCNameCreate(p_fontname, atts -> fontname);
    }
}

void MCBlock::GetTextSize(MCExecContext& ctxt, uinteger_t*& r_size)
{
    uint2 t_size;
    if (!gettextsize(t_size))
        r_size = nil;
    else
        *r_size = t_size;
}

void MCBlock::SetTextSize(MCExecContext& ctxt, uinteger_t* p_size)
{
    if (p_size == nil)
        flags &= ~F_HAS_FSIZE;
    else
    {
        if (atts == NULL)
            atts = new (nothrow) Blockatts;
        flags |= F_HAS_FSIZE;
        atts -> fontsize = *p_size;
    }
}

void MCBlock::GetTextStyle(MCExecContext& ctxt, MCInterfaceTextStyle& r_style)
{
    if (!gettextstyle(r_style . style))
        // Set the style to unset
        r_style . style = 0;
}

void MCBlock::SetTextStyle(MCExecContext& ctxt, const MCInterfaceTextStyle& p_style)
{
    if (p_style . style == 0)
        flags &= ~F_HAS_FSTYLE;
    else
    {
        if (atts == NULL)
            atts = new (nothrow) Blockatts;
        flags |= F_HAS_FSTYLE;
        atts -> fontstyle = p_style . style;
    }
}

void MCBlock::GetTextShift(MCExecContext& ctxt, integer_t*& r_shift)
{
    int2 t_shift;
    if (getshift(t_shift) != True)
        r_shift = nil;
    else
        *r_shift = t_shift;
}

void MCBlock::SetTextShift(MCExecContext& ctxt, integer_t* p_shift)
{
    if (p_shift == nil)
        flags &= ~F_HAS_SHIFT;
    else
    {
        if (atts == NULL)
            atts = new (nothrow) Blockatts;
        atts->shift = *p_shift;
        flags |= F_HAS_SHIFT;
    }
}

void MCBlock::GetForeColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color)
{
    const MCColor *t_color_ptr;
    if (getcolor(t_color_ptr))
        get_interface_color(*t_color_ptr, nil, r_color);
    else
        r_color . name = MCValueRetain(kMCEmptyString);
}

void MCBlock::SetForeColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color)
{
    MCColor t_color;
    if (p_color . name != nil)
    {
        if (MCStringIsEmpty(p_color . name)) // no color set
        {
            if (flags & F_HAS_COLOR)
            {
                delete atts -> color;
                flags &= ~F_HAS_COLOR;
            }
            return;
        }
        MCscreen -> parsecolor(p_color . name, t_color, nil);
    }
    else
        t_color = p_color . color;

    setcolor(&t_color);
}

void MCBlock::GetBackColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color)
{
    const MCColor *t_color_ptr;
    if (getbackcolor(t_color_ptr))
        get_interface_color(*t_color_ptr, nil, r_color);
    else
        r_color . name = MCValueRetain(kMCEmptyString);
}

void MCBlock::SetBackColor(MCExecContext& ctxt, const MCInterfaceNamedColor &p_color)
{
    MCColor t_color;
    if (p_color . name != nil)
    {
        if (MCStringIsEmpty(p_color . name)) // no color set
        {
            setbackcolor(nil);
            return;
        }
        MCscreen -> parsecolor(p_color . name, t_color, nil);
    }
    else
        t_color = p_color . color;

    setbackcolor(&t_color);
}

void MCBlock::GetTextStyleElement(MCExecContext& ctxt, MCNameRef p_index, bool*& r_value)
{
    Font_textstyle t_text_style;
    if (MCF_parsetextstyle(MCNameGetString(p_index), t_text_style) == ES_NORMAL)
    {
        uint2 t_cur_styles;
        if (gettextstyle(t_cur_styles))
            *r_value = MCF_istextstyleset(t_cur_styles, t_text_style);
        else
            r_value = nil;
        
        return;
    }
    ctxt . Throw();
}

void MCBlock::SetTextStyleElement(MCExecContext& ctxt, MCNameRef p_index, bool p_setting)
{
    Font_textstyle t_text_style;
    if (MCF_parsetextstyle(MCNameGetString(p_index), t_text_style) == ES_NORMAL)
    {
        if (atts == NULL)
            atts = new (nothrow) Blockatts;
        
        // AL-2014-09-23 [[ Bug 13509 ]] Check F_HAS_FSTYLE when adding block attribute
        if (!getflag(F_HAS_FSTYLE))
            atts -> fontstyle = parent -> getparent() -> gettextstyle();
        
        flags |= F_HAS_FSTYLE;
        MCF_changetextstyle(atts -> fontstyle, t_text_style, p_setting);
        return;
    }
    ctxt . Throw();
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
