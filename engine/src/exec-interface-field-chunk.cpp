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
        self . name = nil;
    }

    static void input(MCInterfaceNamedColor p_value, MCInterfaceNamedColor& r_value)
    {
        assign(r_value, p_value);
    }

    static bool equal(MCInterfaceNamedColor a, MCInterfaceNamedColor b)
    {
        if (a . name != nil && b . name != nil)
            return MCStringIsEqualTo(a . name, b . name, kMCCompareExact);
        else if (a . name == nil || b . name == nil)
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
        assign(r_value, p_value);
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

    template<typename X> static void getter(MCExecContext ctxt, X *sptr, void (X::*p_getter)(MCExecContext& ctxt, return_type&), stack_type& r_value)
    {
        (sptr ->* p_getter)(ctxt, r_value . list);
    }

    template <typename X> static void setter(MCExecContext ctxt, X *sptr, void (X::*p_setter)(MCExecContext& ctxt, arg_type), arg_type p_value)
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
        if (a . list . count == 0 && a . list . count == 0)
            return true;
        else if (a . list . count != b . list . count)
            return false;
        else
        {
            for (unsigned int i = 0; i < a . list . count && i < b . list . count; ++i)
                if (a . list . elements[i] != b . list . elements[i])
                    return false;
        }
        return true;
    }

    static void output(stack_type a, return_type& r_value)
    {
        r_value . elements = a . list . elements;
        r_value . count = a . list . count;
        a . list . elements = nil;
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
    {
        t_first = true;
        T::init(t_value);
    }

    do
    {
        MCBlock *t_firstblock;
        MCBlock *t_block;

        t_firstblock = sptr -> getblocks();
        t_block = sptr -> indextoblock(si, False);

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

            // Stop if the next block is the first one - we are the last one
            if (t_block -> next() == t_firstblock)
                break;

            t_block = t_block -> next();
        }

        ei -= sptr->gettextlengthcr();
        sptr = sptr->next();
    }
    while(ei > 0);

    r_mixed = false;
    T::output(t_value, r_value);
}

template<typename T> void SetParagraphPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, bool all, uint32_t p_part_id, findex_t si, findex_t ei, void (MCParagraph::*p_setter)(MCExecContext&, typename T::arg_type), typename T::arg_type p_value)
{
    MCParagraph *t_paragraph;
    t_paragraph = p_field -> resolveparagraphs(p_part_id);

    // MW-2013-03-20: [[ Bug 10764 ]] We only need to layout if the paragraphs
    //   are attached to the current card.
    bool t_need_layout;
    if (p_field -> getopened())
        t_need_layout = t_paragraph == p_field -> getparagraphs();
    else
        t_need_layout = false;

    p_field -> verifyindex(t_paragraph, si, false);
    p_field -> verifyindex(t_paragraph, ei, true);

    findex_t t_line_index;
    MCParagraph *sptr = p_field -> indextoparagraph(t_paragraph, si, ei, &t_line_index);

    sptr -> defrag();

    MCRectangle drect = p_field -> getrect();
    findex_t ssi = 0;
    findex_t sei = 0;
    int4 savex = p_field -> textx;
    int4 savey = p_field -> texty;

    T::setter(ctxt, sptr, p_setter, p_value);

    if (t_need_layout)
    {
        if (all)
        {
            p_field -> recompute();
            p_field -> hscroll(savex - p_field -> textx, False);
            p_field -> vscroll(savey - p_field -> texty, False);
            p_field -> resetscrollbars(True);
            if (MCactivefield == p_field)
                p_field -> seltext(ssi, sei, False);
        }
        else
            p_field -> removecursor();
        // MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
        p_field -> layer_redrawrect(drect);
        if (!all)
            p_field -> replacecursor(False, True);
    }
}

// SN-28-11-13: Added specific function for the IDE which needs
// to set the property to a char chunk in a given paragraph.
template<typename T> void SetCharPropOfCharChunkOfParagraph(MCExecContext& ctxt, MCParagraph *p_paragraph, findex_t si, findex_t ei, void (MCBlock::*p_setter)(MCExecContext&, typename T::arg_type), typename T::arg_type p_value)
{
    MCField *t_field;
    t_field = p_paragraph -> getparent();

#ifdef NO_LAYOUT
    // MW-2013-03-20: [[ Bug 10764 ]] We only need to layout if the paragraphs
    //   are attached to the current card.
    bool t_need_layout;
    if (t_field -> getopened())
        t_need_layout = p_paragraph == t_field -> getparagraphs();
    else
        t_need_layout = false;

    MCRectangle drect = t_field -> getrect();
    findex_t ssi = 0;
    findex_t sei = 0;
    int4 savex = t_field -> textx;
    int4 savey = t_field -> texty;

    // MW-2008-07-09: [[ Bug 6353 ]] Improvements in 2.9 meant that the field was
    //   more careful about not doing anything if it wasn't the MCactivefield.
    //   However, the unselection/reselection code here breaks text input if the
    //   active field sets text properties of another field. Therefore we only
    //   get and then reset the selection if we are the active field.
    if (t_need_layout)
    {
        if (all)
        {
            // Same as this?
            if (MCactivefield == t_field)
            {
                t_field -> selectedmark(False, ssi, sei, False, False);
                t_field -> unselect(False, True);
            }
            t_field -> curparagraph = t_field -> focusedparagraph = p_field -> paragraphs;
            t_field -> firstparagraph = t_field -> lastparagraph = NULL;
            t_field -> cury = t_field -> focusedy = t_field -> topmargin;
            t_field -> textx = t_field -> texty = 0;
//            p_field -> resetparagraphs();
        }
        else
        {
            // MW-2012-02-27: [[ Bug ]] Update rect slightly off, shows itself when
            //   setting the box style of the top line of a field.
            drect = t_field -> getfrect();
            drect.y = t_field -> getcontenty() + t_field -> paragraphtoy(pgptr);
            drect.height = 0;
        }
    }
#endif

    // Sanity check for lengths
    uindex_t t_para_len;
    t_para_len = p_paragraph->gettextlength();
    if (si > t_para_len)
    {
        si = ei = t_para_len;
    }
    else if (ei > t_para_len)
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
            MCBlock *tbptr = new MCBlock(*bptr);
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
            MCBlock *tbptr = new MCBlock(*bptr);
            // MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
            //   fontref so it can compute its.
            if (p_paragraph -> getopened())
                tbptr->open(t_field -> getfontref());
            bptr->append(tbptr);
            bptr->SetRange(t_block_index, ei - t_block_index);
            tbptr->SetRange(ei, t_block_length - ei + t_block_index);
            t_blocks_changed = true;
        }

        //                  TODO: what to do with the image source property, as there is a need for p_from_html?
        //                                case P_IMAGE_SOURCE:
        //                    {
        //                                bptr->setatts(p, value);

        //                    // MW-2008-04-03: [[ Bug ]] Only add an extra block if this is coming from
        //                    //   html parsing.
        //                    if (p_from_html)
        //                    {
        //                        MCBlock *tbptr = new MCBlock(*bptr); // need a new empty block
        //                        tbptr->freerefs();                   // for HTML continuation
        //                        // MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
        //                        //   fontref so it can compute its.
        //                        if (opened)
        //                            tbptr->open(parent -> getfontref());
        //                        bptr->append(tbptr);
        //                        tbptr->SetRange(ei, 0);
        //                        t_blocks_changed = true;
        //                    }
        //                }

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

#ifdef NO_LAYOUT
    if (t_need_layout && !all)
    {
        // MW-2012-01-25: [[ ParaStyles ]] Ask the paragraph to reflow itself.
        p_paragraph -> layout(true);
        drect.height += p_paragraph -> getheight(t_field -> fixedheight);
    }
#else
    p_paragraph -> layout(true);
#endif
#ifdef NO_LAYOUT
    if (t_need_layout)
    {
        if (all)
        {
            t_field -> recompute();
            t_field -> hscroll(savex - t_field -> textx, False);
            t_field -> vscroll(savey - t_field -> texty, False);
            t_field -> resetscrollbars(True);
            if (MCactivefield == t_field)
                t_field -> seltext(ssi, sei, False);
        }
        else
            t_field -> removecursor();
        // MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
        t_field -> layer_redrawrect(drect);
        if (!all)
            t_field -> replacecursor(False, True);
    }
#endif
}

template<typename T> void SetCharPropOfCharChunk(MCExecContext& ctxt, MCField *p_field, bool all, uint32_t p_part_id, findex_t si, findex_t ei, void (MCBlock::*p_setter)(MCExecContext&, typename T::arg_type), typename T::arg_type p_value)
{
    if (p_field -> getflag(F_SHARED_TEXT))
        p_part_id = 0;

    // MW-2013-08-27: [[ Bug 11129 ]] Use 'resolveparagraphs()' so we get the same behavior
    //   as elsewhere.
    MCParagraph *pgptr = p_field -> resolveparagraphs(p_part_id);

    // MW-2013-03-20: [[ Bug 10764 ]] We only need to layout if the paragraphs
    //   are attached to the current card.
    bool t_need_layout;
    if (p_field -> getopened())
        t_need_layout = pgptr == p_field -> getparagraphs();
    else
        t_need_layout = false;

    p_field -> verifyindex(pgptr, si, false);
    p_field -> verifyindex(pgptr, ei, true);

    pgptr = p_field -> indextoparagraph(pgptr, si, ei);

    MCRectangle drect = p_field -> getrect();
    findex_t ssi = 0;
    findex_t sei = 0;
    int4 savex = p_field -> textx;
    int4 savey = p_field -> texty;

    // MW-2008-07-09: [[ Bug 6353 ]] Improvements in 2.9 meant that the field was
    //   more careful about not doing anything if it wasn't the MCactivefield.
    //   However, the unselection/reselection code here breaks text input if the
    //   active field sets text properties of another field. Therefore we only
    //   get and then reset the selection if we are the active field.
    if (t_need_layout)
    {
        if (all)
        {
            // Same as this?
            if (MCactivefield == p_field)
            {
                p_field -> selectedmark(False, ssi, sei, False, False);
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
            drect = p_field -> getfrect();
            drect.y = p_field -> getcontenty() + p_field -> paragraphtoy(pgptr);
            drect.height = 0;
        }
    }

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
                uindex_t t_ei;
                t_ei = MCU_min(ei, pgptr -> gettextlength());
                bool t_blocks_changed;
                t_blocks_changed = false;

                pgptr -> defrag();
                MCBlock *bptr = pgptr -> indextoblock(si, False);
                findex_t t_block_index, t_block_length;
                do
                {
                    bptr->GetRange(t_block_index, t_block_length);
                    if (t_block_index < si)
                    {
                        MCBlock *tbptr = new MCBlock(*bptr);
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
                        MCBlock *tbptr = new MCBlock(*bptr);
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

                    // MW-2012-02-14: [[ FontRefs ]] If the block is open, pass in the parent's
                    //   fontref so it can compute its.
                    if (pgptr -> getopened())
                        bptr->open(pgptr -> getparent() -> getfontref());
                    bptr = bptr->next();
                }
                while (t_block_index + t_block_length < (t_pg_length-1) // Length of paragraph without CR
                       && t_block_index + t_block_length < t_ei);

                if (t_blocks_changed)
                    pgptr -> setDirty();
            }
            // end of MCParagraph scope

            if (t_need_layout && !all)
            {
                // MW-2012-01-25: [[ ParaStyles ]] Ask the paragraph to reflow itself.
                pgptr -> layout(true);
                drect.height += pgptr->getheight(p_field -> fixedheight);
            }
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

    if (t_need_layout)
    {
        if (all)
        {
            p_field -> recompute();
            p_field -> hscroll(savex - p_field -> textx, False);
            p_field -> vscroll(savey - p_field -> texty, False);
            p_field -> resetscrollbars(True);
            if (MCactivefield == p_field)
                p_field -> seltext(ssi, sei, False);
        }
        else
            p_field -> removecursor();
        // MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
        p_field -> layer_redrawrect(drect);
        if (!all)
            p_field -> replacecursor(False, True);
    }
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

    // Make sure the pixel attribute has been generated
    MCscreen -> alloccolor(t_color);

    if (attrs == nil)
        attrs = new MCParagraphAttrs;

    attrs -> flags |= p_flag;
    ((uint32_t *)((char *)attrs + p_field_offset))[0] = t_color . pixel;
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
    uinteger_t *t_size;
    GetCharPropOfCharChunk<OptionalFieldPropType<PodFieldPropType<uinteger_t> > >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextSize, false, 0, r_mixed, t_size);

    if (r_mixed)
        return;

    if (t_size == nil)
        GetEffectiveTextSize(ctxt, r_value);
    else
        r_value = *t_size;
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
    /* UNCHECKED */ MCStringDecode(r_value, kMCStringEncodingUTF16, false, &t_string);
    /* UNCHECKED */ settextindex(p_part_id, p_start, p_finish, *t_string, false);
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

    resolveparagraphs(p_part_id) -> replacetextwithparagraphs(p_start, p_finish, rtftoparagraphs(value));

    state &= ~CS_NO_FILE;
}

void MCField::GetHtmlTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef& r_value)
{
    if (exportashtmltext(p_part_id, p_start, p_finish, false, r_value))
        return;

    ctxt . Throw();
}

void MCField::GetEffectiveHtmlTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef& r_value)
{
    if (exportashtmltext(p_part_id, p_start, p_finish, true, r_value))
        return;

    ctxt . Throw();
}

void MCField::SetHtmlTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef value)
{
    if (state & CS_NO_FILE)
    {
        ctxt . SetTheResultToStaticCString("can't set HTMLtext while images are loading");
        return;
    }
    state |= CS_NO_FILE; // prevent interactions while downloading images
    // MW-2012-03-08: [[ FieldImport ]] Use the new htmlText importer.
    resolveparagraphs(p_part_id) -> replacetextwithparagraphs(p_start, p_finish, importhtmltext(value));

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
        resolveparagraphs(p_part_id) -> deletestring(p_start , p_finish);
    else
        resolveparagraphs(p_part_id) -> replacetextwithparagraphs(p_start, p_finish, stpgptr);
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
        int2 x, y;
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
        int2 minx, maxx;
        findex_t t_si, t_ei; // needed to call MCParagraph::getextents

        // MW-2008-07-08: [[ Bug 6331 ]] the formattedWidth can return gibberish for empty lines.
        //   This is because minx/maxx are uninitialized and it seems that they have to be for
        //   calls to getxextents() to make sense.
        minx = MAXINT2;
        maxx = MININT2;

        do
        {
            sptr->getxextents(t_si, t_ei, minx, maxx);
            sptr = sptr->next();
        }
        while (ei > 0 && sptr != pgptr);

        // MW-2008-07-08: [[ Bug 6331 ]] the formattedWidth can return gibberish for empty lines.
        //   If minx > maxx then just assume both are 0.
        if (minx > maxx)
            minx = maxx = 0;

        r_value = getcontentx() + minx;
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
        int2 minx, maxx;

        // MW-2008-07-08: [[ Bug 6331 ]] the formattedWidth can return gibberish for empty lines.
        //   This is because minx/maxx are uninitialized and it seems that they have to be for
        //   calls to getxextents() to make sense.
        minx = MAXINT2;
        maxx = MININT2;

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

        r_value = maxx - minx;
    }
    else
        r_value = 0;
}

void MCField::GetFormattedHeightOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t& r_value)
{
    // MW-2005-07-16: [[Bug 2938]] We must check to see if the field is open, if not we cannot do this.
    if (opened)
    {
        int2 x, y;
        MCParagraph *pgptr = resolveparagraphs(p_part_id);
        MCParagraph *sptr = indextoparagraph(resolveparagraphs(p_part_id), si, ei, nil);
        sptr->indextoloc(si, fixedheight, x, y);
        int4 maxy = 0;
        do
        {
            if (maxy != 0)
                maxy += sptr -> prev() -> computebottommargin() + sptr -> computetopmargin();
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

void MCField::GetFormattedRectOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCRectangle& r_value)
{
    // MW-2005-07-16: [[Bug 2938]] We must check to see if the field is open, if not we cannot do this.
    if (opened)
    {
        int2 x, y;
        MCParagraph *pgptr = resolveparagraphs(p_part_id);
        MCParagraph *sptr = indextoparagraph(resolveparagraphs(p_part_id), si, ei, nil);
        sptr->indextoloc(si, fixedheight, x, y);
        // MW-2012-01-25: [[ FieldMetrics ]] Compute the yoffset in card-coords.
        int4 yoffset = getcontenty() + paragraphtoy(sptr);
        int2 minx, maxx;
        int4 maxy = y;
        minx = INT16_MAX;
        maxx = INT16_MIN;
        do
        {
            // MW-2012-01-25: [[ FieldMetrics ]] Increment the y-extent by the height of the
            //   paragraph up to ei.
            maxy += sptr->getyextent(ei, fixedheight);
            sptr->getxextents(si, ei, minx, maxx);
            sptr = sptr->next();
        }
        while (ei > 0 && sptr != pgptr);

        // MW-2012-01-25: [[ FieldMetrics ]] Make sure the rect we return is in card coords.
        r_value . height = (maxy - 2*y);
        r_value . width = maxx - minx;
        r_value . x = minx + getcontentx();
        r_value . y = y + yoffset;
    }
    else
        memset(&r_value, 0, sizeof(MCRectangle));
}


//////////

void MCField::GetLinkTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef& r_value)
{
    bool t_mixed;
    GetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetLinkText, false, (MCStringRef)nil, t_mixed, r_value);
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
        r_value = nil;
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
    GetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetMetadata, false, (MCStringRef)nil, t_mixed, r_value);
}

void MCField::SetMetadataOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef value)
{
    SetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetMetadata, value);
}

void MCField::GetImageSourceOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef& r_value)
{
    bool t_mixed;
    GetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetImageSource, false, (MCStringRef)nil, t_mixed, r_value);
}

void MCField::SetImageSourceOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef value)
{
    if (si == ei)
        return;

    // MW-2007-07-05: [[ Bug 5099 ]] If this is an image source property we
    //   force to one character here to ensure unicode chars are rounded
    //   up and down correctly.
    findex_t t_ei;
    t_ei = si + 1;

    SetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, true, p_part_id, si, t_ei, &MCBlock::SetImageSource, value);
}

void MCField::GetVisitedOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_value)
{
    bool t_mixed;
    GetCharPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetVisited, false, false, t_mixed, r_value);
}

void MCField::GetEncodingOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, intenum_t &r_encoding)
{
    intenum_t t_encoding;
    bool t_mixed;
    GetParagraphPropOfCharChunk< PodFieldPropType<intenum_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEncoding, t_mixed, t_encoding);

    if (!t_mixed)
        r_encoding = t_encoding;
    else
        r_encoding = 2;
}

void MCField::GetFlaggedOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& r_value)
{
    GetCharPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetFlagged, false, false, r_mixed, r_value);
}

void MCField::SetFlaggedOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool value)
{
    SetCharPropOfCharChunk< PodFieldPropType<bool> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetFlagged, value);
}

void MCField::GetFlaggedRangesOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCInterfaceFlaggedRanges& r_value)
{
    integer_t t_index_offset;
    t_index_offset = -countchars(p_part_id, 0, si);

    MCParagraph *pgptr = resolveparagraphs(p_part_id);
    MCParagraph *sptr = indextoparagraph(pgptr, si, ei, nil);

    MCAutoArray<MCInterfaceFlaggedRange> t_ranges;

    do
    {
        MCInterfaceFlaggedRanges t_paragraphRanges;
        sptr -> getflaggedranges(p_part_id, si, ei, t_index_offset, t_paragraphRanges);

        for (uindex_t i = 0; i < t_paragraphRanges . count; ++i)
            t_ranges . Push(t_paragraphRanges . ranges[i]);

        sptr = sptr -> next();
    }
    while (sptr -> gettextlengthcr() < ei);

    t_ranges . Take(r_value . ranges, r_value . count);
}

void MCField::SetFlaggedRangesOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceFlaggedRanges& value)
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
    MCInterfaceFlaggedRange t_next_range;
    t_next_range = value . ranges[t_range_index];
    t_next_range . start += t_range_offset;
    t_next_range . end += t_range_offset;

    // Loop while there is a range to flag and and we haven't gone further than ei
    while (t_range_index < value . count
           && t_next_range . start < ei
           && t_paragraph_offset < ei)
    {
        // if the next range doesn't cover this paragraph, we skip the paragraph
        if (t_next_range . start > t_paragraph_offset + sptr -> gettextlengthcr())
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
            if (t_next_range . start > t_block_offset + bptr -> GetLength())
            {
                t_block_offset += bptr -> GetLength();
                bptr = bptr -> next();
                continue;
            }

            // if the range doesn't start at the beginning of the block
            // we must split the block and skip the first part
            if (t_next_range . start > t_block_offset)
            {
                bptr -> split(t_next_range . start);
                t_block_offset += bptr -> GetLength();
                bptr = bptr -> next();
            }

            // if the range doesn't cover the block up to its end
            // we must split it
            if (t_next_range . end < t_block_offset + bptr -> GetLength())
                bptr -> split(t_next_range . end);

            // Flag the block
            bptr -> SetFlagged(ctxt, True);

            // if the range went further than the block
            // we must keep track of this and update the next range to be flagged
            if (t_next_range . end > t_block_offset + bptr -> GetLength())
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
    GetParagraphPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetLeftIndent, r_mixed, r_value);
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

    r_count = t_vector . count;
    r_values = t_vector . elements;
}

void MCField::GetEffectiveTabStopsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uindex_t& r_count, uinteger_t*& r_values)
{
    vector_t<uinteger_t> t_vector;
    GetParagraphPropOfCharChunk< VectorFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveTabStops, r_mixed, t_vector);

    r_count = t_vector.count;
    r_values = t_vector.elements;
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

    r_count = t_vector . count;
    r_values = t_vector . elements;
}

void MCField::GetEffectiveTabWidthsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uindex_t& r_count, uinteger_t*& r_values)
{
    vector_t<uinteger_t> t_vector;
    GetParagraphPropOfCharChunk< VectorFieldPropType<uinteger_t> >(ctxt, this, p_part_id, si, ei, &MCParagraph::GetEffectiveTabWidths, r_mixed, t_vector);

    r_count = t_vector.count;
    r_values = t_vector.elements;
}

void MCField::SetTabWidthsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uindex_t count, uinteger_t *values)
{
    vector_t<uinteger_t> t_vector;
    t_vector . count = count;
    t_vector . elements = values;

    SetParagraphPropOfCharChunk< VectorFieldPropType<uinteger_t> >(ctxt, this, true, p_part_id, si, ei, &MCParagraph::SetTabWidths, t_vector);
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
// Block color properties
//////////

void MCField::GetForeColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    GetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetForeColor, false, r_color, r_mixed, r_color);
}

void MCField::GetEffectiveForeColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    GetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetForeColor, false, r_color, r_mixed, r_color);

    if (r_mixed)
    {
        MCInterfaceNamedColorFree(ctxt, r_color);
        return;
    }

    // Color unset: must default
    if (MCStringIsEmpty(r_color . name))
    {
        MCInterfaceNamedColorFree(ctxt, r_color);
        GetForeColor(ctxt, r_color);
    }
}

void MCField::SetForeColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceNamedColor& color)
{
    SetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetForeColor, color);
}

void MCField::GetBackColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    GetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetBackColor, false, r_color, r_mixed, r_color);
}

void MCField::GetEffectiveBackColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color)
{
    GetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetBackColor, false, r_color, r_mixed, r_color);

    if (r_mixed)
    {
        MCInterfaceNamedColorFree(ctxt, r_color);
        return;
    }

    // No value returned: must default
    if (MCStringIsEmpty(r_color . name))
    {
        MCInterfaceNamedColorFree(ctxt, r_color);
        GetBackColor(ctxt, r_color);
    }
}

void MCField::SetBackColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceNamedColor& color)
{
    SetCharPropOfCharChunk< PodFieldPropType<MCInterfaceNamedColor> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetBackColor, color);
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
    GetCharPropOfCharChunk< PodFieldPropType<MCStringRef> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextFont, false, (MCStringRef)nil, r_mixed, &t_value);

    if (r_mixed)
        return;

    // Block value unset, must default to the parent's one
    if (*t_value == nil)
        GetTextFont(ctxt, r_value);
    else
        r_value = MCValueRetain(*t_value);
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
    GetCharPropOfCharChunk< PodFieldPropType<MCInterfaceTextStyle> >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextStyle, false, t_value, r_mixed, t_value);

    if (r_mixed)
        return;

    if (t_value . style == 0)
        GetTextStyle(ctxt, t_value);
    else
        r_value = t_value;
}

void MCField::SetTextStyleOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceTextStyle& p_value)
{
    SetCharPropOfCharChunk< PodFieldPropType<MCInterfaceTextStyle> >(ctxt, this, false, p_part_id, si, ei, &MCBlock::SetTextStyle, p_value);
}

void MCField::GetTextShiftOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t*& r_value)
{
    GetCharPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextShift, false, 0, r_mixed, r_value);
}

void MCField::GetEffectiveTextShiftOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t& r_value)
{
    integer_t *t_shift;
    GetCharPropOfCharChunk< OptionalFieldPropType<PodFieldPropType<integer_t> > >(ctxt, this, p_part_id, si, ei, &MCBlock::GetTextShift, false, 0, r_mixed, t_shift);

    if (r_mixed)
        return;

    if (t_shift == nil)
        r_value = 0;
    else
        r_value = *t_shift;
}

void MCField::SetTextShiftOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t* p_value)
{
    SetCharPropOfCharChunk< OptionalFieldPropType< PodFieldPropType<integer_t> > >(ctxt, this, true, p_part_id, si, ei, &MCBlock::SetTextShift, p_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCParagraph::GetEncoding(MCExecContext &ctxt, intenum_t &r_encoding)
{
    if (MCStringIsNative(m_text))
        r_encoding = 0; // nativestring
    else
        r_encoding = 1; // unicode
}

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
        *r_depth = getlistdepth();
}

void MCParagraph::GetEffectiveListDepth(MCExecContext& ctxt, uinteger_t& r_depth)
{
    r_depth = getlistdepth();
}

void MCParagraph::SetListDepth(MCExecContext& ctxt, uinteger_t* p_depth)
{
    uinteger_t t_depth;

    if (p_depth == nil)
        t_depth = 1;
    else
        t_depth = *p_depth;

    if (t_depth < 1 || t_depth > 16)
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
    setparagraphattr_int<uinteger_t, 0, 32767>(attrs, PA_HAS_SPACE_ABOVE, offsetof(MCParagraphAttrs, space_above), p_space);
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
    setparagraphattr_int<uinteger_t, 0, 32767>(attrs, PA_HAS_SPACE_BELOW, offsetof(MCParagraphAttrs, space_below), p_space);
}

void MCParagraph::DoSetTabStops(MCExecContext &ctxt, bool p_is_relative, const vector_t<uinteger_t>& p_tabs)
{
    MCAutoArray<uint2> t_new_tabs;

    uint2 *t_new = nil;
    uindex_t t_new_count = 0;

    uint2 t_previous_tab_stop;
    t_previous_tab_stop = 0;

    for (uindex_t i = 0; i < p_tabs . count; i++)
    {
        if (p_tabs . elements[i] > 65535)
        {
            ctxt . LegacyThrow(EE_PROPERTY_NAN);
            return;
        }

        if (p_is_relative)
        {
            t_new_tabs . Push(p_tabs . elements[i] + t_previous_tab_stop);
            t_previous_tab_stop = t_new_tabs[i];
        }
        else
            t_new_tabs . Push(p_tabs . elements[i]);
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

void MCParagraph::GetBackColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color)
{
    if (attrs == nil || (attrs -> flags & PA_HAS_BACKGROUND_COLOR) == 0)
        r_color . name = MCValueRetain(kMCEmptyString);
    else
    {
        MCColor t_color;
        t_color . pixel = attrs -> background_color;
        MCscreen -> querycolor(t_color);
        get_interface_color(t_color, nil, r_color);
    }
}

void MCParagraph::GetEffectiveBackColor(MCExecContext& ctxt, MCInterfaceNamedColor &r_color)
{
    if (attrs != nil && (attrs -> flags & PA_HAS_BACKGROUND_COLOR) != 0)
    {
        MCColor t_color;
        t_color . pixel = attrs -> background_color;
        MCscreen -> querycolor(t_color);
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
        t_color . pixel = attrs -> background_color;
        MCscreen -> querycolor(t_color);
        get_interface_color(t_color, nil, r_color);
    }
}

void MCParagraph::GetEffectiveBorderColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
    if (attrs != nil && (attrs -> flags & PA_HAS_BORDER_COLOR) != 0)
    {
        MCColor t_color;
        t_color . pixel = attrs -> background_color;
        MCscreen -> querycolor(t_color);
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
    if (attrs == nil || (attrs -> flags & PA_HAS_METADATA) == 0)
        r_metadata = nil;
    else
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
            attrs = new MCParagraphAttrs;

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
            atts = new Blockatts;

        /* UNCHECKED */ MCValueInter(p_linktext, atts -> linktext);

        flags |= F_HAS_LINK;
    }
}

void MCBlock::GetMetadata(MCExecContext& ctxt, MCStringRef& r_metadata)
{
    if (getmetadata())
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
            atts = new Blockatts;

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
            atts = new Blockatts;

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
            MCNameDelete(atts -> fontname);
            atts -> fontname = nil;
        }
    }
    else
    {
        if (atts == nil)
            atts = new Blockatts;
        
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
            atts = new Blockatts;
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
            atts = new Blockatts;
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
            atts = new Blockatts;
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
