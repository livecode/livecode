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

//#include "execpt.h"
#include "exec.h"
#include "sellst.h"
#include "undolst.h"
#include "stack.h"
#include "card.h"
#include "cdata.h"
#include "field.h"
#include "MCBlock.h"
#include "paragraf.h"
#include "scrolbar.h"
#include "mcerror.h"
#include "util.h"
#include "date.h"
#include "font.h"
#include "styledtext.h"
#include "cmds.h"
#include "redraw.h"

#include "globals.h"

const char *MCliststylestrings[] =
{
	"",
	"disc",
	"circle",
	"square",
	"decimal",
	"lower latin",
	"upper latin",
	"lower roman",
	"upper roman",
	"skip",
	nil
};

Exec_stat MCField::sort(MCExecContext &ctxt, uint4 parid, Chunk_term type,
                        Sort_type dir, Sort_type form, MCExpression *by)
{
	if (flags & F_SHARED_TEXT)
        parid = 0;
    else if (parid == 0)
        parid = getcard()->getid();
    
    // SN-2014-09-17: [[ Bug 13461 ]] We might get CT_FIELD as a chunk type.
    if (type == CT_FIELD)
        type = CT_PARAGRAPH;

    // SN-2014-09-17: [[ Bug 13461 ]] CT_LINE and CT_PARAGRAPH are equivalent in
    // the way they don't modify the content of the paragraphs
	if (type != CT_PARAGRAPH && type != CT_LINE)
	{
        
        extern bool MCInterfaceExecSortContainer(MCExecContext &ctxt, MCStringRef p_data, int p_type, Sort_type p_direction, int p_form, MCExpression *p_by, MCStringRef &r_output);
		// MW-2012-02-21: [[ FieldExport ]] Use the new text export method.
        MCAutoStringRef t_text_string, t_sorted;
		exportastext(parid, 0, INT32_MAX, &t_text_string);
        if (!MCInterfaceExecSortContainer(ctxt, *t_text_string, type, dir, form, by, &t_sorted))
            return ES_ERROR;
        
        settext(parid, *t_sorted, False);
        return ES_NORMAL;
	}

    MCAutoArray<MCSortnode> items;
	uint4 nitems = 0;
	MCParagraph *pgptr;
    
    if (opened && parid == 0)
        pgptr = paragraphs;
    else
        pgptr = getcarddata(fdata, parid, True)->getparagraphs();
    
    // MW-2008-02-29: [[ Bug 5763 ]] The crash report seems to suggest a problem with
    //   a NULL-pointer access on a paragraph - this is the only place I can see this
    //   happening, so we will guard against it.
    if (pgptr != NULL)
    {
        MCParagraph *tpgptr = pgptr;
        do
        {
            nitems++;
            tpgptr = tpgptr->next();
        }
        while (tpgptr != pgptr);
        items.Extend(nitems);
        nitems = 0;
        
        extern void MCStringsSortAddItem(MCExecContext &ctxt, MCSortnode *items, uint4 &nitems, int form, MCValueRef p_input, MCExpression *by);
        
        do
        {
            if (tpgptr->next() != pgptr || !tpgptr->IsEmpty())
            {
                MCAutoStringRef t_string;
                /* UNCHECKED */ MCStringCopy(tpgptr->GetInternalStringRef(), &t_string);
                MCStringsSortAddItem(ctxt, items.Ptr(), nitems, form, *t_string, by);
                items[nitems - 1].data = (void *)tpgptr;
            }
            tpgptr = tpgptr->next();
        }
        while (tpgptr != pgptr);
    }
    
    extern void MCStringsSort(MCSortnode *p_items, uint4 nitems, Sort_type p_dir, Sort_type p_form, MCStringOptions p_options);

    MCStringsSort(items.Ptr(), nitems, dir, form, ctxt . GetStringComparisonType());
    
    if (nitems > 0)
	{
		MCParagraph *newparagraphs = NULL;
		uint4 i;
		for (i = 0 ; i < nitems ; i++)
		{
			MCParagraph *tpgptr = (MCParagraph *)items[i].data;
			tpgptr->remove(pgptr);
			tpgptr->appendto(newparagraphs);

		}
		if (pgptr != NULL)
			pgptr->appendto(newparagraphs);
		getcarddata(fdata, parid, True)->setparagraphs(newparagraphs);
		if (opened && (parid == 0 || parid == getcard()->getid()))
		{
			paragraphs = newparagraphs;
			resetparagraphs();
			do_recompute(true);

			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
		}
	}
    
	return ES_NORMAL;
}

Boolean MCField::find(MCExecContext &ctxt, uint4 cardid, Find_mode mode,
                      MCStringRef tofind, Boolean first)
{
	if (fdata == NULL || flags & F_F_DONT_SEARCH)
		return False;
	if (opened)
		fdata->setparagraphs(paragraphs);
    
	if (flags & F_SHARED_TEXT)
		cardid = 0;
	else if (cardid == 0)
        cardid = getcard()->getid();
    
	MCCdata *tptr = fdata;
	do
	{
		if (tptr->getid() == cardid)
		{
			MCParagraph *paragraphptr = tptr->getparagraphs();
			MCParagraph *tpgptr = paragraphptr;
			findex_t flength = MCStringGetLength(tofind);
			findex_t toffset, oldoffset;
			if (first && foundlength != 0)
			{
				findex_t junk = toffset = oldoffset = foundoffset + foundlength;
				tpgptr = indextoparagraph(tpgptr, oldoffset, junk);
				toffset -= oldoffset;
			}
			else
				toffset = oldoffset = 0;
			do
			{
				uindex_t t_length = MCStringGetLength(tpgptr->GetInternalStringRef());
				MCRange t_range, t_where;
				t_range = MCRangeMake(oldoffset, t_length - oldoffset);
				while (MCStringFind(tpgptr->GetInternalStringRef(), t_range, tofind, 
									ctxt.GetStringComparisonType(),
									&t_where))
				{
					switch (mode)
					{
					case FM_NORMAL:
						if (t_where.offset == 0 
							|| tpgptr->TextIsWordBreak(tpgptr->GetCodepointAtIndex(tpgptr->DecrementIndex(t_where.offset)))
							|| tpgptr->TextIsPunctuation(tpgptr->GetCodepointAtIndex(tpgptr->DecrementIndex(t_where.offset))))
						{
							if (first)
							{
								if (MCfoundfield != NULL && MCfoundfield != this)
									MCfoundfield->clearfound();
								foundoffset = toffset + t_where.offset;
								toffset = t_where.offset;
								t_where.offset = tpgptr->IncrementIndex(t_where.offset);
								while (t_where.offset < t_length
									   && !tpgptr->TextIsWordBreak(tpgptr->GetCodepointAtIndex(t_where.offset))
									   && !tpgptr->TextIsPunctuation(tpgptr->GetCodepointAtIndex(t_where.offset)))
								{
									t_where.offset = tpgptr->IncrementIndex(t_where.offset);
								}
								foundlength = t_where.offset - toffset;
								MCfoundfield = this;
							}
							return True;
						}
						t_where.offset = tpgptr->IncrementIndex(t_where.offset);
						break;
					case FM_WHOLE:
					case FM_WORD:
						if ((t_where.offset == 0 
								|| tpgptr->TextIsWordBreak(tpgptr->GetCodepointAtIndex(tpgptr->DecrementIndex(t_where.offset))))
							 && (t_where.offset + flength == t_length
								|| tpgptr->TextIsWordBreak(tpgptr->GetCodepointAtIndex(t_where.offset + flength))
								|| tpgptr->TextIsPunctuation(tpgptr->GetCodepointAtIndex(t_where.offset + flength))))

						{
							if (first)
							{
								if (MCfoundfield != NULL && MCfoundfield != this)
									MCfoundfield->clearfound();
								foundoffset = toffset + t_where.offset;
								foundlength = MCStringGetLength(tofind);
								MCfoundfield = this;
							}
							return True;
						}
						t_where.offset = tpgptr->IncrementIndex(t_where.offset);
						break;
					case FM_CHARACTERS:
					case FM_STRING:
						if (first)
						{
							if (MCfoundfield != NULL && MCfoundfield != this)
								MCfoundfield->clearfound();
							foundoffset = toffset + t_where.offset;
							foundlength = MCStringGetLength(tofind);
							MCfoundfield = this;
						}
						// Fall through...
					default:
						return True;
					}
					oldoffset = t_where.offset;
					t_range.offset = oldoffset;
					t_range.length = t_length - oldoffset;
				}
				toffset += tpgptr->gettextlengthcr();
				tpgptr = tpgptr->next();
				oldoffset = 0;
			}
			while (tpgptr != paragraphptr);
			return False;
		}
		tptr = tptr->next();
	}
	while (tptr != fdata);
	return False;
}

void MCField::verifyindex(MCParagraph *top, findex_t &si, bool p_is_end)
{
	// Does nothing at the moment
	// TODO: this will become useful again for surrogate pairs
}

// MW-2012-02-08: [[ Field Indices ]] If 'index' is non-nil then we return the
//   1-based index of the paragraph that si resides in.
MCParagraph *MCField::indextoparagraph(MCParagraph *top, findex_t &si, findex_t &ei, findex_t* index)
{
	int4 t_index;
	findex_t l = top->gettextlengthcr();
	MCParagraph *pgptr = top;
	t_index = 1;
	while (si >= l)
	{
		si -= l;
		ei -= l;
		t_index += 1;
		pgptr = pgptr->next();
		if (pgptr == top)
		{
			t_index -= 1;
			pgptr = pgptr->prev();
			si = ei = l - 1;
		}
		l = pgptr->gettextlengthcr();
	}
	if (index != nil)
		*index = t_index;
	return pgptr;
}

findex_t MCField::ytooffset(int4 y)
{
	findex_t si = 0;
	MCParagraph *tptr = paragraphs;
	while (True)
	{
		y -= tptr->getheight(fixedheight);
		if (y <= 0)
			break;
		si += tptr->gettextlengthcr();
		tptr = tptr->next();
	}
	return si;
}

int4 MCField::paragraphtoy(MCParagraph *target)
{
	int4 y = cury;
	MCParagraph *tptr = curparagraph;
	
	while (tptr != target)
	{
		y += tptr->getheight(fixedheight);
		tptr = tptr->next();
		if (tptr == paragraphs)
			break;
	}
	if (tptr != target)
	{
		y = cury;
		tptr = curparagraph;
		while (tptr != target && tptr != paragraphs)
		{
			tptr = tptr->prev();
			y -= tptr->getheight(fixedheight);
		}
	}
	return y;
}

#if TO_REMOVE
int32_t MCField::mapnativeindex(uint4 parid, int32_t p_native_index, bool p_is_end)
{
	// Resolve the correct collection of paragraphs.
	MCParagraph *t_paragraphs;
	t_paragraphs = resolveparagraphs(parid);
	if (t_paragraphs == nil)
		return p_native_index;
	
	int32_t t_index;
	t_index = 0;
	
	MCParagraph *t_paragraph;
	t_paragraph = t_paragraphs;
	do
	{
		int32_t t_native_length;
		t_native_length = t_paragraph -> gettextlength();
		if (p_native_index <= t_native_length)
		{
			t_index += t_paragraph -> mapnativeindex(p_native_index, p_is_end);
			break;
		}
		
		p_native_index -= t_native_length + 1;
		t_index += t_paragraph -> gettextsizecr();
		
		t_paragraph = t_paragraph -> next();
	}
	while(t_paragraph != t_paragraphs);
	
	return t_index;
}
#endif

findex_t MCField::getpgsize(MCParagraph *pgptr)
{
	if (pgptr == NULL)
		pgptr = paragraphs;
	MCParagraph *tpgptr = pgptr;
	findex_t length = 0;
	do
	{
		length += tpgptr->gettextlengthcr();
		tpgptr = tpgptr->next();
	}
	while (tpgptr != pgptr);
	return length;
}

// SN-2014-06-23: [[ Bug 12303 ]] Refactoring of the bugfix: new param added
void MCField::setparagraphs(MCParagraph *newpgptr, uint4 parid, bool p_preserve_zero_length_styles)
{
    setparagraphs(newpgptr, parid, INTEGER_MIN, INTEGER_MIN, p_preserve_zero_length_styles);
}

// SN-2014-06-23: [[ Bug 12303 ]] Refactoring of the bugfix: new param added
void MCField::setparagraphs(MCParagraph *newpgptr, uint4 parid, findex_t p_start, findex_t p_end, bool p_preserve_zero_length_styles)
{
	if (flags & F_SHARED_TEXT)
		parid = 0;
	else if (parid == 0)
        parid = getcard()->getid();
    
	MCCdata *fptr = getcarddata(fdata, parid, True);
	MCParagraph *pgptr = fptr->getparagraphs();
    MCParagraph *t_old_pgptr;

	if (opened && fptr == fdata)
		closeparagraphs(pgptr);
	if (pgptr == paragraphs)
		paragraphs = NULL;
	if (pgptr == oldparagraphs)
        oldparagraphs = NULL;

    // Save the current paragraphs and switch to the one to be processed
    t_old_pgptr = paragraphs;
    paragraphs = pgptr;

    // Must simply delete the whole lot of paragraphs (old execution)
    if (p_start == p_end && p_end == INTEGER_MIN)
    {
        while (paragraphs != NULL)
        {
            MCParagraph *tpgptr = paragraphs->remove(paragraphs);
            delete tpgptr;
        }
        paragraphs = newpgptr;

        fptr->setparagraphs(paragraphs);
    }
    else
    {
        // New execution, only deleting a part of the paragraphs
        uint4 oc = 0;

        uint4 oldstate = state;
        bool t_refocus;
        if (focused == this)
            t_refocus = true;
        else
            t_refocus = false;
        if (state & CS_KFOCUSED)
            kunfocus();

        // MW-2012-09-07: [[ 10374 ]] Make sure we preseve the drag* vars since
        //   closing and opening the field will clear them.
        bool t_was_dragdest, t_was_dragsource;
        t_was_dragdest = MCdragdest == this;
        t_was_dragsource = MCdragsource == this;
        while (opened)
        {
            close();
            oc++;
        }

        // delete the text to be replaced
        settextindex(parid, p_start, p_end, kMCEmptyString, False);
        
        MCParagraph *t_lastpgptr;
        MCParagraph *t_insert_paragraph;
        // Fetch the paragraph in which to insert the new paragraphs (and update the position)
        t_insert_paragraph = indextoparagraph(paragraphs, p_start, p_end);

        if (newpgptr -> prev() == newpgptr)
            t_lastpgptr = NULL;
        else
            t_lastpgptr = newpgptr -> prev();

        t_insert_paragraph->setselectionindex(p_start, p_start, False, False);
        t_insert_paragraph->split();
        t_insert_paragraph->append(newpgptr);
        // SN-2014-20-06: [[ Bug 12303 ]] Refactoring of the bugfix
        t_insert_paragraph->join(p_preserve_zero_length_styles);
        if (t_lastpgptr == NULL)
            t_lastpgptr = t_insert_paragraph;
        else
            t_insert_paragraph->defrag();
        
        // MW-2014-05-28: [[ Bug 10593 ]] When replacing a range of text with styles, paragraph styles from
        //   the new content should replace paragraph styles for the old content whenever the range touches
        //   the 0 index of a paragraph. Thus when joining the end of the range again, we want to preserve
        //   the new contents styles even if it is an empty paragraph.
        t_lastpgptr->join(true);
        t_lastpgptr->defrag();

        fptr->setparagraphs(paragraphs);

        while (oc--)
        {
            open();
        }

        // MW-2012-09-07: [[ 10374 ]] Make sure we preseve the drag* vars since
        //   closing and opening the field will clear them.
        if (t_was_dragsource)
            MCdragsource = this;
        if (t_was_dragdest)
            MCdragdest = this;

        if (oldstate & CS_KFOCUSED)
            kfocus();
        // MW-2008-03-25: Make sure we reset focused to this field - otherwise mouseMoves aren't
        //   sent to the field after doing a partial htmlText update.
        if (t_refocus)
            focused = this;
        state = oldstate;
    }


	// MW-2008-03-13: [[ Bug 5383 ]] Crash when saving after initiating a URL download
	//   Here it is important to 'setparagraphs' on the MCCdata object *before* opening. This
	//   is because it is possible for URL downloads to be initiated in openparagraphs, which 
	//   allow messages to be processed which allows a save to occur and its important that
	//   the field be in a consistent state.
	if (opened && fptr == fdata)
    {
		openparagraphs();
		do_recompute(true);

		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	else
    {
        paragraphs = t_old_pgptr;
    }
}

Exec_stat MCField::settext(uint4 parid, MCStringRef p_text, Boolean formatted)
{
	state &= ~CS_CHANGED;
	if (opened)
	{
		Ustruct *us = MCundos->getstate();
		if (us != NULL && MCundos->getobject() == this)
			MCundos->freestate();
		clearfound();
		fdata->setparagraphs(paragraphs);
		unselect(False, True);
    }

	MCParagraph *pgptr = NULL;
    if (MCStringGetLength(p_text))
    {
        uindex_t t_text_length;
        uindex_t t_start;

        t_text_length = MCStringGetLength(p_text);
        t_start = 0;

		while (True)
		{
            uindex_t t_pos;
            t_pos = t_start;

			if (formatted)
                while (t_pos < t_text_length)
				{
                    if (MCStringFirstIndexOfChar(p_text, '\n', t_pos, kMCStringOptionCompareExact, t_pos))
                    {
                        if (MCStringGetCharAtIndex(p_text, t_pos + 1) == '\n')
                            break;
                        else
                            ++t_pos;
                    }
                    else
                        t_pos = t_text_length;
				}
			else
            {
                if (!MCStringFirstIndexOfChar(p_text, '\n', t_start, kMCStringOptionCompareExact, t_pos))
                    t_pos = t_text_length;
            }

            if (t_pos > PARAGRAPH_MAX_LEN - 2)
                t_pos = PARAGRAPH_MAX_LEN - 2;

            MCStringRef t_paragraph_text;
            if (t_pos != t_start)
                MCStringCopySubstring(p_text, MCRangeMake(t_start, t_pos - t_start), t_paragraph_text);
            else
                t_paragraph_text = MCValueRetain(kMCEmptyString);

            if (formatted)
            {
                /* UNCHECKED */ MCStringMutableCopyAndRelease(t_paragraph_text, t_paragraph_text);
                MCStringFindAndReplaceChar(t_paragraph_text, '\n', ' ', kMCStringOptionCompareExact);
			}

			MCParagraph *tpgptr = new MCParagraph;
			tpgptr->setparent(this);
			tpgptr->appendto(pgptr);

			// MW-2012-02-16: [[ IntrinsicUnicode ]] Make sure we pass the correct setting for
			//   'unicode'.
            // SN-2014-01-17: [[ Unicodification ]] Now input is directly handled as a unicode input
            tpgptr->settext(t_paragraph_text);

            MCValueRelease(t_paragraph_text);

            if (t_pos < t_text_length)
			{
                t_start = t_pos + 1;

                if (formatted && t_pos < t_text_length)
                    ++t_start;
			}
			else
				break;
		}
	}
	else
	{
		pgptr = new MCParagraph;
		pgptr->setparent(this);
	}
	setparagraphs(pgptr, parid);
	return ES_NORMAL;
}



// On input (si, ei) are relative to p_top
// On output we return the paragraph in which si is placed, and (si, ei) is
// relative to it.
// We also verify that (si, ei) fall on character boundaries.
MCParagraph *MCField::verifyindices(MCParagraph *p_top, findex_t& si, findex_t& ei)
{
	MCParagraph *t_start_pg;
	t_start_pg = indextoparagraph(p_top, si, ei);

	return t_start_pg;
}

Exec_stat MCField::settextindex(uint4 parid, findex_t si, findex_t ei, MCStringRef p_text, Boolean undoing, MCFieldStylingMode p_styling_mode)
{
	state &= ~CS_CHANGED;
	if (!undoing)
		MCundos->freestate();
    
	if (flags & F_SHARED_TEXT)
		parid = 0;
	else if (parid == 0)
        parid = getcard()->getid();
    
	MCCdata *fptr = getcarddata(fdata, parid, True);
	if (opened && fptr == fdata && focusedparagraph != NULL)
	{
		clearfound();
		unselect(False, True);
        
        // SN-2014-05-12 [[ Bug 12365 ]]
        // There might be several paragraph to be inserted, the cursor must be removed
        removecursor();
		focusedparagraph->setselectionindex(PARAGRAPH_MAX_LEN, PARAGRAPH_MAX_LEN, False, False);
	}
    findex_t oldsi;

    MCParagraph *toppgptr = fptr->getparagraphs();

    // 'put after' puts at PARAGRAPH_MAX_LEN...
    if (si == PARAGRAPH_MAX_LEN)
    {
        MCParagraph* t_paragraph;
        t_paragraph = fptr -> getparagraphs();
        oldsi = 0;
        // Loop up to the penultimate paragraph
        while (t_paragraph -> next() != toppgptr)
        {
            oldsi += t_paragraph -> gettextlengthcr();
            t_paragraph = t_paragraph -> next();
        }
        // Add the length of the last paragraph
        oldsi += t_paragraph -> gettextlength();
    }
    else
        oldsi = si;

	MCParagraph *pgptr;
	pgptr = verifyindices(toppgptr, si, ei);


	// MW-2013-10-24: [[ FasterField ]] If affect_many is true then multiple
	//   paragraphs have been affected, so we need to redraw everything below
	//   the initial one. We also store the initial height of the paragraph
	//   so we can see if it has changed.
	bool t_affect_many;
	t_affect_many = false;
	
	MCParagraph *t_initial_pgptr;
	t_initial_pgptr = pgptr;
	
	int32_t t_initial_height;
	if (opened && fptr == fdata)
		t_initial_height = t_initial_pgptr -> getheight(fixedheight);
	
    if (si < ei)
	{
        // MW-2014-05-28: [[ Bug 11928 ]] Reworked code here so that it is the same as
        //   MCField::deleteselection (makes sure paragraph styles work the same way
        //   when deleting a paragraph break).
		MCParagraph *saveparagraph = pgptr;
		int4 savey = 0;
		if (opened && pgptr == paragraphs)
			savey = paragraphtoy(saveparagraph);
        
        // First delete the portion of the first paragraph in the range.
        int4 tei;
        tei = MCMin(ei, pgptr -> gettextlength());
		
		// Pass through the first style preservation flag. This will leave us with
		// a zero length block at si (which finsertnew will extend).
		pgptr->deletestring(si, tei, p_styling_mode);
        ei -= (tei - si);
        
		if (ei > pgptr -> gettextlength())
		{
            // End index is reduced by the amount we just deleted.
            ei -= si;
            
            // MW-2014-06-10: [[ Bug 11928 ]] Adjust for the CR that will be removed by the
            //   final join in this consequent.
            ei -= 1;
			pgptr = pgptr->next();
			while (ei >= pgptr->gettextlengthcr())
			{
				ei -= pgptr->gettextlengthcr();
				MCParagraph *tpgptr = pgptr->remove
				                      (pgptr);
				if (tpgptr == curparagraph)
				{
					curparagraph = saveparagraph;
					cury = savey;
				}
				if (tpgptr == focusedparagraph)
				{
					focusedparagraph = saveparagraph;
					focusedy = savey;
				}
				delete tpgptr;
				
				// MW-2013-10-24: [[ FasterField ]] Removing paragraphs affects multiple paragraphs.
				t_affect_many = true;
			}
			pgptr->deletestring(0, ei);
			if (pgptr == curparagraph)
			{
				curparagraph = saveparagraph;
				cury = savey;
			}
			if (pgptr == focusedparagraph)
			{
				focusedparagraph = saveparagraph;
				focusedy = savey;
			}
			if (pgptr != pgptr->prev())
			{
				pgptr = pgptr->prev();
				pgptr->join();
				
				// MW-2013-10-24: [[ FasterField ]] Join affects multiple paragraphs.
				t_affect_many = true;
			}
		}
	}
	pgptr->setparent(this);
	pgptr->setselectionindex(si, si, False, False);

	// MM-2014-04-09: [[ Bug 12088 ]] Get the width of the paragraph before insertion and layout.
	//  If as a result of the update the width of the field has changed, we need to recompute.
    // MW-2014-06-06: [[ Bug 12385 ]] Don't do anything layout related if not open.
	int2 t_initial_width;
    if (opened != 0)
        t_initial_width = pgptr -> getwidth();
	
	// MW-2012-02-13: [[ Block Unicode ]] Use the new finsert method in native mode.
	// MW-2012-02-23: [[ PutUnicode ]] Pass through the encoding to finsertnew.
	if (!MCStringIsEmpty(p_text))
	{
		// MW-2013-10-24: [[ FasterField ]] If finsertnew() returns true then multiple
		//   paragraphs were created, so we've affected many.
		if (pgptr->finsertnew(p_text))
			t_affect_many = true;
    }

	if (opened && fptr == fdata)
    {
        oldsi += MCStringGetLength(p_text);
        ei = oldsi;
        focusedparagraph = indextoparagraph(paragraphs, oldsi, ei);
		if (state & CS_KFOCUSED)
			focusedparagraph->setselectionindex(ei, ei, False, False);
		
		// If we haven't already affected many, then lay out the paragraph and see if the
		// height has changed. If it has we must do a recompute and need to redraw below.
		// MM-2014-04-09: [[ Bug 12088 ]] If the height hasn't changed, then check to see 
		//  if the width has changed and a re-compute is needed.
		if (!t_affect_many)
			t_initial_pgptr -> layout(false);
		if (t_affect_many || t_initial_pgptr -> getheight(fixedheight) != t_initial_height)
		{
				do_recompute(false);
				t_affect_many = true;
		}
        else if ((t_initial_width == textwidth && pgptr -> getwidth() != textwidth)
                 || pgptr -> getwidth() > textwidth)
			do_recompute(false);
		
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		// MW-2013-10-24: [[ FasterField ]] Tweak to minimize redraw.
		int32_t t_paragraph_y;
		t_paragraph_y = getcontenty() + paragraphtoy(t_initial_pgptr);
		MCRectangle drect;
		drect = getfrect();
		
		// If affecting many, redraw everything below y of the initial pg in the
		// field, otherwise just redraw the paragraph.
		if (t_affect_many)
			drect . height -= (t_paragraph_y - drect . y);
		else
			drect . height = t_initial_pgptr -> getheight(fixedheight);
		
		drect . y = t_paragraph_y;
		
		layer_redrawrect(drect);
		
		focusedy = paragraphtoy(focusedparagraph);
        
        // SN-2014-05-12 [[ Bug 12365 ]]
        // Redraw the cursor after the update
        // SN-2014-10-17: [[ Bug 13493 ]] Don't replace the cursor - unnecessary and causes
        //  unwanted scrolling
//        replacecursor(True, True);
	}

	return ES_NORMAL;
}

// MW-2011-03-13: [[ Bug 9447 ]] Make sure we update linksi/linkei with the appropriate
//   indices after stretching out the region.
void MCField::getlinkdata(MCRectangle &lrect, MCBlock *&sb, MCBlock *&eb)
{
	findex_t si, ei;
	si = linksi;
	ei = linkei;
	int2 maxx;
	MCParagraph *sptr = indextoparagraph(paragraphs, si, ei);
	linksi -= si;
	linkei -= si;
	ei = MCU_min(ei, sptr->gettextlengthcr());
	sb = sptr->indextoblock(si, False);
	if (!sb->islink())
	{
		sb = NULL;
		return;
	}
	eb = sptr->indextoblock(ei - 1, False);
	
	// MW-2011-02-26: [[ Bug 9416 ]] Make sure the linkrect and block extends to the
	//   extremities of the link.
	// MW-2013-05-21: [[ Bug 10794 ]] Make sure we update sb/eb with the actual blocks
	//   the indices are within.
	findex_t t_index;
	t_index = si;
	sb = sptr -> extendup(sb, t_index);
	si = t_index;
	t_index = (ei - 1);
	eb = sptr -> extenddown(eb, t_index);
	ei = t_index;
	
	linksi += si;
	linkei = linksi + (ei - si);

    coord_t t_x, t_y;
	sptr->indextoloc(si, fixedheight, t_x, t_y);
    maxx = t_x;
    lrect.y = t_y;

	// MW-2012-01-25: [[ FieldMetrics ]] Compute the y-offset in card coords.
	uint4 yoffset = getcontenty() + paragraphtoy(sptr);
	lrect.height = sptr->getyextent(ei, fixedheight);
    coord_t minxf, maxxf;
	sptr->getxextents(si, ei, minxf, maxxf);
    lrect.x = minxf;
    maxx = maxxf;
	// MW-2012-01-25: [[ FieldMetrics ]] Make sure the linkrect is in card coords.
	lrect.height -= lrect.y;
	lrect.y += yoffset;
	lrect.width = maxx - lrect.x;
	lrect.x += getcontentx();
}

#ifdef /* MCField::gettextatts */ LEGACY_EXEC
// MW-2012-01-25: [[ ParaStyles ]] The 'is_line_chunk' parameter is true when a text
//   attribute is set directly on a line (used to disambiguate backColor).
Exec_stat MCField::gettextatts(uint4 parid, Properties which, MCExecPoint &ep, MCNameRef index, Boolean effective, findex_t si, findex_t ei, bool is_line_chunk)
{
	// MW-2012-02-21: [[ FieldExport ]] For all text props, use then new export methods.
	if (which == P_UNICODE_TEXT || which == P_TEXT)
	{
		exportastext(parid, ep, si, ei, which == P_UNICODE_TEXT);
		return ES_NORMAL;
	}
	else if (which == P_UNICODE_PLAIN_TEXT || which == P_PLAIN_TEXT)
	{
		exportasplaintext(parid, ep, si, ei, which == P_UNICODE_PLAIN_TEXT);
		return ES_NORMAL;
	}
	else if (which == P_UNICODE_FORMATTED_TEXT || which == P_FORMATTED_TEXT)
	{
		exportasformattedtext(parid, ep, si, ei, which == P_UNICODE_FORMATTED_TEXT);
		return ES_NORMAL;
	}
	else if (which == P_RTF_TEXT)
	{
		exportasrtftext(parid, ep, si, ei);
		return ES_NORMAL;
	}
	else if (which == P_HTML_TEXT)
	{
		exportashtmltext(parid, ep, si, ei, effective == True);
		return ES_NORMAL;
	}
	else if (which == P_STYLED_TEXT || which == P_FORMATTED_STYLED_TEXT)
	{
		exportasstyledtext(parid, ep, si, ei, which == P_FORMATTED_STYLED_TEXT, effective == True);
		return ES_NORMAL;
	}
	
	// MW-2013-08-27: [[ Bug 11129 ]] Use 'resolveparagraphs()' so we get the same
	//   behavior as if exporting the content in various ways.
	MCParagraph *pgptr = resolveparagraphs(parid);

	// MW-2012-02-08: [[ Field Indices ]] If we pass a pointer to indextoparagraph
	//   then it computes the index of the paragraph (1-based).
	// MW-2012-02-24: [[ Bug ]] Compute the char index, by working out the amount
	//   left in si after navigating to the paragraph containing it.
	// MW-2012-03-19: [[ Bug 10094 ]] The charIndex is just si, after unresolving to
	//   chars (rather than codeunits).
	findex_t t_line_index, t_char_index;
	t_char_index = si;
	MCParagraph *sptr = indextoparagraph(pgptr, si, ei, &t_line_index);

	int2 x, y;
	switch (which)
	{
	// MW-2012-02-08: [[ Field Indices ]] The charIndex of a range is the first index
	//   (notice that we add one - internally indices are 0-based).
	case P_CHAR_INDEX:
		// MW-2012-02-24: [[ FieldChars ]] Convert the field indices to char indices.
		unresolvechars(parid, t_char_index, t_char_index);
		ep.setuint(t_char_index + 1);
		break;
	// MW-2012-02-08: [[ Field Indices ]] The lineIndex of a range is the index of
	//   the line containing the first index (notice that this index is already
	//   1-based).
	case P_LINE_INDEX:
		ep.setuint(t_line_index);
		break;
	case P_FORMATTED_TOP:
	  // MW-2005-07-16: [[Bug 2938]] We must check to see if the field is open, if not we cannot do this.
		if (opened)
		{
			sptr->indextoloc(si, fixedheight, x, y);
			// MW-2012-01-25: [[ FieldMetrics ]] Adjust the y-coord to be in card coords.
			ep.setnvalue(getcontenty() + paragraphtoy(sptr) + y);
		}
		else
			ep . setnvalue(0);
		break;
	case P_FORMATTED_LEFT:
	case P_FORMATTED_WIDTH:
		// MW-2005-07-16: [[Bug 2938]] We must check to see if the field is open, if not we cannot do this.
		if (opened)
		{
			coord_t minx, maxx;

			// MW-2008-07-08: [[ Bug 6331 ]] the formattedWidth can return gibberish for empty lines.
			//   This is because minx/maxx are uninitialized and it seems that they have to be for
			//   calls to getxextents() to make sense.
			minx = INFINITY; //MCinfinity
			maxx = -INFINITY; //MCinfinity

			do
			{
				sptr->getxextents(si, ei, minx, maxx);
				sptr = sptr->next();
			}
			while (ei > 0 && sptr != pgptr);
			
			// MW-2008-07-08: [[ Bug 6331 ]] the formattedWidth can return gibberish for empty lines.
			//   If minx > maxx then just assume bother are 0.
			if (minx > maxx)
				minx = maxx = 0;

			if (which == P_FORMATTED_LEFT)
				ep.setnvalue(int32_t(floorf(getcontentx() + minx)));
			else
				ep.setnvalue(int32_t(ceilf(maxx - minx)));
		}
		else
			ep . setnvalue(0);
		break;
	case P_FORMATTED_HEIGHT:
		// MW-2005-07-16: [[Bug 2938]] We must check to see if the field is open, if not we cannot do this.
		if (opened)
		{
			sptr->indextoloc(si, fixedheight, x, y);
			int4 maxy = 0;
			do
			{
                // FG-2014-12-03: [[ Bug 11688 ]] The margins get counted twice...
                //if (maxy != 0)
                //	maxy += sptr -> prev() -> computebottommargin() + sptr -> computetopmargin();
				maxy += sptr->getyextent(ei, fixedheight);
				ei -= sptr->gettextlengthcr();
				sptr = sptr->next();
			}
			while (ei > 0 && sptr != pgptr);
			ep.setnvalue(maxy - y);
		}
		else
			ep . setnvalue(0);
		break;
	case P_FORMATTED_RECT:
		// MW-2005-07-16: [[Bug 2938]] We must check to see if the field is open, if not we cannot do this.
		if (opened)
		{
			sptr->indextoloc(si, fixedheight, x, y);
			// MW-2012-01-25: [[ FieldMetrics ]] Compute the yoffset in card-coords.
			int4 yoffset = getcontenty() + paragraphtoy(sptr);
			coord_t minx, maxx;
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
            // AL-2014-10-28: [[ Bug 13829 ]] The left and right of the formattedRect should be
            // floorf'd and ceilf'd respectively, to give the correct integer bounds.
			ep.setrectangle(floorf(minx + getcontentx()), y + yoffset, ceilf(maxx + getcontentx()), (maxy - y) + yoffset);
		}
		else
			ep.setrectangle(0, 0, 0, 0);
		break;
	case P_LINK_TEXT:
		ep.setvalueref_nullable(sptr->getlinktext(si));
		break;
	case P_IMAGE_SOURCE:
		ep.setvalueref_nullable(sptr->getimagesource(si));
		break;
	// MW-2012-01-06: [[ Block Metadata ]] Handle fetching the metadata about a given
	//   index.
	case P_METADATA:
		if (is_line_chunk)
			ep.setvalueref_nullable(sptr -> getmetadata());
		else
			ep.setvalueref_nullable(sptr->getmetadataatindex(si));
		break;
	case P_VISITED:
		ep.setboolean(sptr->getvisited(si));
		break;
	// MW-2012-01-26: [[ FlaggedField ]] Fetch the flagged status of the range.
	// MW-2012-02-12: [[ Encoding ]] Fetch the unicode status of the range.
	case P_ENCODING:
	case P_FLAGGED:
		{
			bool t_first, t_state;
			t_first = true;
			t_state = false;
			do
			{
				// Fetch the flagg state of the current paragraph, breaking if
				// the state has changed and this is not the first one we look at.
				bool t_new_state;
				t_new_state = false;
				if (!sptr -> getflagstate(which == P_ENCODING ? F_HAS_UNICODE : F_FLAGGED, si, ei, t_new_state) ||
					(!t_first && t_state != t_new_state))
					break;
				
				// If we are the first para, then store the state.
				if (t_first)
				{
					t_state = t_new_state;
					t_first = false;
				}

				// Reduce ei until we get to zero, advancing through the paras.
				ei -= sptr->gettextlengthcr();
				sptr = sptr->next();
				
				// MW-2013-08-27: [[ Bug 11129 ]] If we reach the end of the paragraphs
				//   then set ei to 0 as we are done.
				if (sptr == pgptr)
					ei = 0;
			}
			while(ei > 0);
			
			if (ei <= 0)
			{
				if (which == P_ENCODING)
					ep.setstaticcstring(t_state ? MCunicodestring : MCnativestring);
				else
					ep.setboolean(t_state);
			}
			else
				ep.setstaticcstring(MCmixedstring);
		}
		break;
	// MW-2012-02-08: [[ FlaggedField ]] Fetch the ranges of flagged runs between si
	//   and ei - the ranges are measured relative to si.
	case P_FLAGGED_RANGES:
		{
			// Start with an empty result.
			ep . clear();

			// MW-2013-07-31: [[ Bug 10957 ]] Keep track of the byte index of the start
			//   of the paragraph.
			int32_t t_paragraph_offset;
			t_paragraph_offset = 0;
			
			// Loop through the paragraphs until the range is exhausted.
			do
			{
				// Fetch the flagged ranges into ep between si and ei (sptr relative)
				sptr -> getflaggedranges(parid, ep, si, ei, t_paragraph_offset);
				
				// MW-2013-07-31: [[ Bug 10957 ]] Update the paragraph (byte) offset.
				t_paragraph_offset += sptr -> gettextlengthcr();
				
				// Reduce ei until we get to zero, advancing through the paras.
				si = 0;
				ei -= sptr -> gettextlengthcr();
				sptr = sptr -> next();
				
				// MW-2013-08-27: [[ Bug 11129 ]] If we reach the end of the paragraphs
				//   then set ei to 0 as we are done.
				if (sptr == pgptr)
					ei = 0;
			}
			while(ei > 0);
		}
		break;
	// MW-2012-01-25: [[ ParaStyles ]] All these properties are handled as paragraph
	//   styles.
	case P_LIST_STYLE:
	case P_LIST_DEPTH:
	case P_LIST_INDENT:
	case P_LIST_INDEX:
	case P_FIRST_INDENT:
	case P_LEFT_INDENT:
	case P_RIGHT_INDENT:
	case P_TEXT_ALIGN:
	case P_SPACE_ABOVE:
	case P_SPACE_BELOW:
	case P_TAB_STOPS:
	case P_BORDER_WIDTH:
	case P_BACK_COLOR:
	case P_BORDER_COLOR:
	case P_HGRID:
	case P_VGRID:
	case P_DONT_WRAP:
	// MW-2012-02-09: [[ ParaStyles ]] Take account of the new 'padding' property.
	case P_PADDING:
	// MW-2012-02-11: [[ TabWidths ]] Take account of the new 'tabWidths' property.
	case P_TAB_WIDTHS:
	// MW-2012-03-05: [[ HiddenText ]] Take account of the new 'hidden' property.
	case P_INVISIBLE:
		if (which != P_BACK_COLOR || is_line_chunk)
		{
			// MW-2012-02-13: [[ ParaStyles ]] Added 'effective' adjective support to
			//   paragraph props.
			sptr -> getparagraphattr(which, ep, effective);

			MCExecPoint ep2(nil, nil, nil);
			do
			{
				// MW-2012-02-13: [[ ParaStyles ]] Added 'effective' adjective support to
				//   paragraph props.
				sptr -> getparagraphattr(which, ep2, effective);

				// MW-2012-01-25: [[ ParaStyles ]] If, at any point, we find a different
				//   value then the prop is 'mixed' for that range.
				if (ep . getsvalue() != ep2 . getsvalue())
				{
					ep . setstaticcstring(MCmixedstring);
					break;
				}

				ei -= sptr->gettextlengthcr();
				sptr = sptr->next();
				
				// MW-2013-08-27: [[ Bug 11129 ]] If we reach the end of the paragraphs
				//   then set ei to 0 as we are done.
				if (sptr == pgptr)
					ei = 0;
			}
			while(ei > 0);
			break;
		}
	default:
	{
		Boolean has = False;
		const char *pname;
		uint2 psize, pstyle;
		MCNameRef pname_name;
		getfontattsnew(pname_name, psize, pstyle);
        MCAutoPointer<char> temp;
        /* UNCHECKED */ MCStringConvertToCString(MCNameGetString(pname_name), &temp);
		pname = *temp;

		uint2 height;
		height = gettextheight();

		const char *fname = pname;
		uint2 size = psize;
		uint2 style = pstyle;
		const MCColor *color = NULL;
		const MCColor *bcolor = NULL;
		int2 shift = 0;
		uint2 mixed = MIXED_NONE;
		Font_textstyle t_text_style = FTS_UNKNOWN;
		bool pspecstyle = false, specstyle = false;

		// MW-2011-11-24: [[ Array TextStyle ]] If we are fetching textStyle and we have
		//   an index, then decode which specific style is being manipulated.
		if (which == P_TEXT_STYLE && !MCNameIsEmpty(index))
		{
			if (MCF_parsetextstyle(MCNameGetString(index), t_text_style) != ES_NORMAL)
				return ES_ERROR;

			pspecstyle = MCF_istextstyleset(pstyle, t_text_style);
			specstyle =  MCF_istextstyleset(style, t_text_style);
		}

		do
		{
			const char *tname = fname;
			uint2 tsize = size;
			uint2 tstyle = style;
			const MCColor *tcolor = NULL;
			const MCColor *tbcolor = NULL;
			int2 tshift = 0;
			uint2 tmixed = MIXED_NONE;
			bool tspecstyle = specstyle;

			// MW-2012-02-14: [[ FontRefs ]] Previously this process would open/close the
			//   paragraph, but this is unnecessary as it doesn't rely on anything active.
			if (sptr->getatts(si, ei, which, t_text_style, tname, tsize, tstyle, tcolor, tbcolor, tshift, tspecstyle, tmixed))
			{
				tspecstyle = MCF_istextstyleset(tstyle, t_text_style);

				if (has)
				{
					mixed |= tmixed;
					if (tname != fname)
						mixed |= MIXED_NAMES;
					if (tsize != size)
						mixed |= MIXED_SIZES;
					if (tstyle != style)
						mixed |= MIXED_STYLES;
					if (tcolor == NULL && color != NULL
					        || color == NULL && tcolor != NULL
					        || tcolor != NULL && color != NULL
					        && (tcolor->red != color->red || tcolor->green != color->green
					            || tcolor->blue != color->blue))
						mixed |= MIXED_COLORS;
					if (tbcolor == NULL && bcolor != NULL
					        || bcolor == NULL && tbcolor != NULL
					        || tbcolor != NULL && bcolor != NULL
					        && (tbcolor->red != bcolor->red || tbcolor->green != bcolor->green
					            || tbcolor->blue != bcolor->blue))
						mixed |= MIXED_COLORS;
					if (tshift != shift)
						mixed |= MIXED_SHIFT;
					if (tspecstyle != specstyle)
						mixed |= MIXED_SPEC_STYLE;
				}
				else
				{
					has = True;
					fname = tname;
					size = tsize;
					style = tstyle;
					color = tcolor;
					bcolor = tbcolor;
					shift = tshift;
					specstyle = tspecstyle;
					mixed = tmixed;
				}
			}
			else if (has)
			{
				if (fname != pname)
					mixed |= MIXED_NAMES;
				if (size != psize)
					mixed |= MIXED_SIZES;
				if (style != pstyle)
					mixed |= MIXED_STYLES;
				if (color != NULL || bcolor != NULL)
					mixed |= MIXED_COLORS;
				if (shift != 0)
					mixed |= MIXED_SHIFT;
				if (tspecstyle != pspecstyle)
					mixed |= MIXED_SPEC_STYLE;
			}
			ei -= sptr->gettextlengthcr();
			si = 0;
			sptr = sptr->next();
			
			// MW-2013-08-27: [[ Bug 11129 ]] If we reach the end of the paragraphs
			//   then set ei to 0 as we are done.
			if (sptr == pgptr)
				ei = 0;
		}
		while (ei > 0);
        
        if (!has)
        {
            if (effective)
            {
                // MW-2011-11-23: [[ Array TextStyle ]] Make sure we call the correct
                //   method for textStyle (its now an array property).
                if (which != P_TEXT_STYLE)
                    return getprop(parid, which, ep, effective);
                else
                    return getarrayprop(parid, which, ep, index, effective);
            }
            ep.clear();
            return ES_NORMAL;
        }

		Exec_stat stat = ES_NORMAL;
		switch (which)
		{
		case P_FORE_COLOR:
			if (color == NULL)
                ep.clear();
			else if (mixed & MIXED_COLORS)
				ep.setstaticcstring(MCmixedstring);
			else
				ep.setcolor(*color);
			break;
		case P_BACK_COLOR:
			if (bcolor == NULL)
				ep.clear();
			else if (mixed & MIXED_COLORS)
				ep.setstaticcstring(MCmixedstring);
			else
				ep.setcolor(*bcolor);
			break;
		case P_TEXT_FONT:
			if (mixed & MIXED_NAMES)
				ep.setstaticcstring(MCmixedstring);
			else
				stat = MCF_unparsetextatts(which, ep, flags, *t_fname, height, size, style);
			break;
		case P_TEXT_SIZE:
			if (mixed & MIXED_SIZES)
				ep.setstaticcstring(MCmixedstring);
			else
				stat = MCF_unparsetextatts(which, ep, flags, *t_fname, height, size, style);
			break;
		case P_TEXT_STYLE:
			// MW-2011-11-23: [[ Array TextStyle ]] If no textStyle has been specified
			//   then we are processing the whole set, otherwise process a specific one.
			if (t_text_style == FTS_UNKNOWN)
			{
				if (mixed & MIXED_STYLES)
					ep.setstaticcstring(MCmixedstring);
				else
					stat = MCF_unparsetextatts(which, ep, flags, *t_fname, height, size, style);
			}
			else
			{
				if (mixed & MIXED_SPEC_STYLE)
					ep.setstaticcstring(MCmixedstring);
				else
					ep.setboolean(specstyle);
			}
			break;
		case P_TEXT_SHIFT:
			if (mixed & MIXED_SHIFT)
				ep.setstaticcstring(MCmixedstring);
			else
				ep.setint(shift);
			break;
		default:
			MCeerror->add(EE_FIELD_BADTEXTATTS, 0, 0);
			stat = ES_ERROR;
		}
		return stat;
	}
	break;
	}
	
	return ES_NORMAL;
}
#endif /* MCField::gettextatts */

#ifdef /* MCField::settextatts */ LEGACY_EXEC
// MW-2011-12-08: [[ StyledText ]] We now take the execpoint directly so that array
//   values can be used.
// MW-2012-01-25: [[ ParaStyles ]] The 'is_line_chunk' parameter is true if the prop
//   is being set on a line directly.
Exec_stat MCField::settextatts(uint4 parid, Properties which, MCExecPoint& ep, MCNameRef index, findex_t si, findex_t ei, bool is_line_chunk, bool dont_layout)
{
	// Fetch the string value of the ep as 's' for compatibility with pre-ep taking
	// code.
	
	MCAutoStringRef s;
	ep . copyasstringref(&s);
	
	if (flags & F_SHARED_TEXT)
		parid = 0;
	else if (parid == 0)
        parid = getcard()->getid();

	// MW-2011-12-08: [[ StyledText ]] Handle the styledText case.
	if (which == P_HTML_TEXT || which == P_RTF_TEXT || which == P_STYLED_TEXT || which == P_UNICODE_TEXT || which == P_TEXT)
	{
		uint2 oc = 0;
		uint4 oldstate = state;
		bool t_refocus;
		if (focused == this)
			t_refocus = true;
		else
			t_refocus = false;
		if (state & CS_KFOCUSED)
			kunfocus();
		
		// MW-2012-09-07: [[ 10374 ]] Make sure we preseve the drag* vars since
		//   closing and opening the field will clear them.
		bool t_was_dragdest, t_was_dragsource;
		t_was_dragdest = MCdragdest == this;
		t_was_dragsource = MCdragsource == this;
			
		while (opened)
		{
			close();
			oc++;
		}
		settextindex(parid, si, ei, kMCEmptyString, False);
		MCCdata *fptr = getcarddata(fdata, parid, True);
		MCParagraph *oldparagraphs = fptr->getparagraphs();
		fptr->setset(0);
        
        // MW-2014-05-28: [[ Bug 12303 ]] If we are setting 'text' then we don't want to touch the paragraph
        //   styles of the first paragraph it is being put into. (In the other cases they are styled formats
        //   so if the first paragraph is empty, we replace styles - this is what you'd expect).
        bool t_preserve_zero_length_styles;
        t_preserve_zero_length_styles = false;
		switch (which)
		{
		case P_HTML_TEXT:
			sethtml(parid, *s);
			break;
		case P_RTF_TEXT:
			setrtf(parid, *s);
			break;
		// MW-2011-12-08: [[ StyledText ]] Import the styled text.
		case P_STYLED_TEXT:
			setstyledtext(parid, ep);
			break;
		case P_UNICODE_TEXT:
		case P_TEXT:
            t_preserve_zero_length_styles = true;
			setpartialtext(parid, MCStringGetOldString(*s), which == P_UNICODE_TEXT);
			break;
		default:
			break;
		}
		MCParagraph *newpgptr = fptr->getparagraphs();
		MCParagraph *lastpgptr = newpgptr->prev();
		if (lastpgptr == newpgptr)
			lastpgptr = NULL;
		MCParagraph *pgptr = indextoparagraph(oldparagraphs, si, ei);
		pgptr->setselectionindex(si, si, False, False);
		pgptr->split();
		pgptr->append(newpgptr);
		pgptr->join(t_preserve_zero_length_styles);
		if (lastpgptr == NULL)
			lastpgptr = pgptr;
		else
			pgptr->defrag();
        // MW-2014-05-28: [[ Bug 10593 ]] When replacing a range of text with styles, paragraph styles from
        //   the new content should replace paragraph styles for the old content whenever the range touches
        //   the 0 index of a paragraph. Thus when joining the end of the range again, we want to preserve
        //   the new contents styles even if it is an empty paragraph.
		lastpgptr->join(true);
		lastpgptr->defrag();
		fptr->setparagraphs(oldparagraphs);
		paragraphs = oldparagraphs;
		while (oc--)
			open();
			
		// MW-2012-09-07: [[ 10374 ]] Make sure we preseve the drag* vars since
		//   closing and opening the field will clear them.
		if (t_was_dragsource)
			MCdragsource = this;
		if (t_was_dragdest)
			MCdragdest = this;
			
		if (oldstate & CS_KFOCUSED)
			kfocus();
		// MW-2008-03-25: Make sure we reset focused to this field - otherwise mouseMoves aren't
		//   sent to the field after doing a partial htmlText update.
		if (t_refocus)
			focused = this;
		state = oldstate;
		if (opened && (parid == 0 || parid == getcard()->getid()))
		{
			resetparagraphs();
			do_recompute(true);
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
		}
		return ES_NORMAL;
	}
	else if (which == P_FLAGGED_RANGES)
	{
		// MW-2012-02-08: [[ FlaggedField ]] Special case the 'flaggedRanges' property.

		// Get the list string and length.
		uint32_t l = ep . getsvalue() . getlength();
		const char *sptr = ep . getsvalue() . getstring();

		// Use a temporary EP to set the flagged property to false / true.
		MCExecPoint ep2(nil, nil, nil);

		// First ensure the flagged property is false along the whole range.
		ep2 . setboolean(False);
		
		// MW-2013-08-01: [[ Bug 10932 ]] Don't layout this operation.
		settextatts(parid, P_FLAGGED, ep2, nil, si, ei, false, true);

		// All remaining ranges will have flagged set to true.
		ep2 . setboolean(True);

		// Loop while we haven't exhausted the string.
		while(l)
		{
			// Parse <n>,<m> and skip any non-number chars after it.
			Boolean done1, done2;
			int32_t i1, i2;
			i1 = MCU_strtol(sptr, l, ',', done1);
			i2 = MCU_strtol(sptr, l, ',', done2);
			while (l && !isdigit((uint1)*sptr) && *sptr != '-' && *sptr != '+')
			{
				l--;
				sptr++;
			}

			// If we successfully parsed two integers, set the flagged of the range.
			if (done1 && done2)
			{
				// MW-2012-02-24: [[ FieldChars ]] Convert char indices to field indices.
				findex_t t_range_start, t_range_end;
				t_range_start = si;
				t_range_end = ei;
				resolvechars(parid, t_range_start, t_range_end, i1 - 1, i2 - i1 + 1);
				
				// MW-2012-03-23: [[ Bug 10118 ]] Both range_start and range_end are already
				//   offset from the start of the field.
				// MW-2013-08-01: [[ Bug 10932 ]] Don't layout this operation.
				settextatts(parid, P_FLAGGED, ep2, nil, MCU_max(si, t_range_start), MCU_min(ei, t_range_end), false, true);
			}
		}
		// MW-2013-08-01: [[ Bug 10932 ]] Force a full relayout.
		settextatts(parid, P_UNDEFINED, ep2, nil, 0, 0, false, false);
		return ES_NORMAL;
	}

	// MW-2013-08-27: [[ Bug 11129 ]] Use 'resolveparagraphs()' so we get the same behavior
	//   as elsewhere.
	MCParagraph *pgptr = resolveparagraphs(parid);

	// MW-2013-03-20: [[ Bug 10764 ]] We only need to layout if the paragraphs
	//   are attached to the current card.
	bool t_need_layout;
	if (opened && !dont_layout)
		t_need_layout = pgptr == paragraphs;
	else
		t_need_layout = false;

	// MW-2007-07-05: [[ Bug 5099 ]] If this is an image source property we
	//   force to one character here to ensure unicode chars are rounded
	//   up and down correctly.
	if (which == P_IMAGE_SOURCE && si != ei)
		ei = si + 1;

	verifyindex(pgptr, si, false);
	verifyindex(pgptr, ei, true);

	pgptr = indextoparagraph(pgptr, si, ei);
	MCAutoStringRef fname;
	uint2 size = 0;
	uint2 style = FA_DEFAULT_STYLE;
	MCColor tcolor;
	tcolor.red = tcolor.green = tcolor.blue = 0;
	MCColor *color = &tcolor;
	int2 shift = 0;
	Boolean all = False;
	Boolean newstate;
	void *t_value;

	MCAutoStringRef t_string_value;

    bool t_redraw_field;
    t_redraw_field = false;

	// If true, it means process this prop set as a paragraph style.
	bool t_is_para_attr;
	t_is_para_attr = false;

	switch (which)
	{
	case P_UNDEFINED:
		all = True;
		break;
	case P_BACK_COLOR:
		// If we are a line chunk, then make sure we set the paragraph
		// level value.
		if (is_line_chunk)
		{
			t_is_para_attr = true;
			break;
		}
	case P_FORE_COLOR:
		if (!MCStringGetLength(*s))
			color = NULL;
		else if (!MCscreen->parsecolor(*s, tcolor, nil))
			return ES_ERROR;
		t_value = color;
		break;
	case P_TEXT_STYLE:
        // AL-2014-09-22: [[ Bug 11817 ]] Don't necessarily recompute the whole field when changing text styles
		// MW-2011-11-23: [[ Array TextStyle ]] If we have an index then change the prop
		//   to the pseudo add/remove ones. In this case 'value' is the textStyle to process.
		if (!MCNameIsEmpty(index))
		{
			Font_textstyle t_text_style;

			if (MCF_parsetextstyle(MCNameGetString(index), t_text_style) != ES_NORMAL)
				return ES_ERROR;

			Boolean t_new_state;
			if (!MCU_stob(MCStringGetOldString(*s), t_new_state))
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, *s);
				return ES_ERROR;
			}

			if (t_new_state)
				which = P_TEXT_STYLE_ADD;
			else
				which = P_TEXT_STYLE_REMOVE;

			t_value = (void *)t_text_style;
			break;
		}

		if (MCF_parsetextatts(which, s, flags, fname, fontheight, size, style) != ES_NORMAL)
            return ES_ERROR;
        t_value = (void *)style;
        break;
	case P_TEXT_FONT:
	case P_TEXT_SIZE:
		all = True;
		if (MCF_parsetextatts(which, s, flags, fname, fontheight, size, style) != ES_NORMAL)
			return ES_ERROR;
		if (which == P_TEXT_FONT)
			t_value = (void *)fname;
		else
			t_value = (void *)size;
        break;
	case P_TEXT_SHIFT:
		if (!MCU_stoi2(*s, shift))
		{
			MCeerror->add(EE_FIELD_SHIFTNAN, 0, 0);
			return ES_ERROR;
		}
		t_value = (void *)shift;
		all = True;
		break;
	case P_IMAGE_SOURCE:
		if (ei == si)
			return ES_NORMAL;
		all = True;
	// MW-2012-01-06: [[ Block Metadata ]] Handle setting the metadata of a run.
	case P_METADATA:
		if (is_line_chunk)
		{
			t_is_para_attr = true;
			break;
		}
	case P_LINK_TEXT:
		/* UNCHECKED */ ep . copyasstringref(&t_string_value);
		t_value = (void *)*t_string_value;
		break;
	// MW-2012-01-26: [[ FlaggedField ]] Set the flagged status of the range.
	case P_FLAGGED:
		if (!MCU_stob(MCStringGetOldString(*s), newstate))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, *s);
			return ES_ERROR;
		}
		t_value = (void *)(Boolean)newstate;
		t_value = (void *)newstate;
		break;
	case P_VISITED:
		if (!MCU_stob(MCStringGetOldString(*s), newstate))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, *s);
			return ES_ERROR;
		}
		pgptr->setvisited(si, MCU_min(ei, pgptr->gettextlength()), newstate);
		break;
	// MW-2012-01-25: [[ ParaStyles ]] Make sure we set 'is_para_attr' for all
	//   paragraph styles. Notice that they all cause a complete recompute.
	case P_LIST_STYLE:
	case P_LIST_DEPTH:
	case P_LIST_INDENT:
	case P_LIST_INDEX:
	case P_FIRST_INDENT:
	case P_LEFT_INDENT:
	case P_RIGHT_INDENT:
	case P_TEXT_ALIGN:
	case P_SPACE_ABOVE:
	case P_SPACE_BELOW:
	case P_TAB_STOPS:
	case P_BORDER_WIDTH:
	case P_BORDER_COLOR:
	case P_HGRID:
	case P_VGRID:
	case P_DONT_WRAP:
	// MW-2012-02-09: [[ ParaStyles ]] Take account of the new 'padding' property.
	case P_PADDING:
	// MW-2012-02-11: [[ TabWidths ]] Take account of the new 'tabWidths' property.
	case P_TAB_WIDTHS:
	// MW-2012-03-05: [[ HiddenText ]] Take account of the new 'hidden' property.
	case P_INVISIBLE:
		all = True;
		t_is_para_attr = true;
		break;
	default:
		MCeerror->add(EE_FIELD_BADTEXTATTS, 0, 0);
		return ES_ERROR;
	}
	MCRectangle drect = rect;
	findex_t ssi = 0;
	findex_t sei = 0;
	int4 savex = textx;
	int4 savey = texty;

	// MW-2008-07-09: [[ Bug 6353 ]] Improvements in 2.9 meant that the field was
	//   more careful about not doing anything if it wasn't the MCactivefield.
	//   However, the unselection/reselection code here breaks text input if the
	//   active field sets text properties of another field. Therefore we only
	//   get and then reset the selection if we are the active field.
	if (t_need_layout)
		if (all)
		{
			if (MCactivefield == this)
			{
				selectedmark(False, ssi, sei, False);
				unselect(False, True);
			}
			curparagraph = focusedparagraph = paragraphs;
			firstparagraph = lastparagraph = NULL;
			cury = focusedy = topmargin;
			textx = texty = 0;
		}
		else
		{
			// MW-2012-02-27: [[ Bug ]] Update rect slightly off, shows itself when
			//   setting the box style of the top line of a field.
			drect = getfrect();
			drect.y = getcontenty() + paragraphtoy(pgptr);
			drect.height = 0;
		}

	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	MCParagraph *t_first_pgptr;
	t_first_pgptr = pgptr;

	do
	{
		findex_t l = pgptr->gettextlengthcr();
		if (si < l)
		{
			pgptr->setparent(this);

			if (which != P_UNDEFINED)
			{
				if (!t_is_para_attr)
					pgptr->setatts(si, MCU_min(ei, pgptr->gettextlength()), which, t_value);
				else
				{
					// MW-2012-01-25: [[ ParaStyles ]] If we are a paragraph style then we
					//   set on the first para, then copy from the first on all subsequent.
					if (pgptr == t_first_pgptr)
						t_stat = pgptr->setparagraphattr(which, ep);
					else
						pgptr -> copysingleattr(which, t_first_pgptr);
				}
			}
				
			if (t_need_layout && !all)
			{
				// MW-2012-01-25: [[ ParaStyles ]] Ask the paragraph to reflow itself.
                // AL-2014-09-22: [[ Bug 11817 ]] If we changed the amount of lines of this paragraph
                //  then redraw the whole field.
				if (pgptr -> layout(all, true))
                    t_redraw_field = true;
                else
                    drect.height += pgptr->getheight(fixedheight);
			}
		}
		si = MCU_max(0, si - l);
		ei -= l;
		pgptr = pgptr->next();
		
		// MW-2013-08-27: [[ Bug 11129 ]] If we reach the end of the paragraphs
		//   then set ei to 0 as we are done.
		if (pgptr == t_first_pgptr)
			ei = 0;
	}
	while(ei > 0 && t_stat == ES_NORMAL);
	if (t_need_layout)
	{
		if (all)
		{
			do_recompute(true);
			hscroll(savex - textx, False);
			vscroll(savey - texty, False);
			resetscrollbars(True);
			if (MCactivefield == this)
				seltext(ssi, sei, False);
		}
		else
        {
			removecursor();
            // AL-2014-09-22: [[ Bug 11817 ]] If we are redrawing, then the dirty rect is the whole rect.
            if (t_redraw_field)
                drect = rect;
        }
		// MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
		layer_redrawrect(drect);
		if (!all)
			replacecursor(False, True);
	}
	return t_stat;
}
#endif /* MCField::settextatts */

// MW-2008-01-30: [[ Bug 5754 ]] If update is true the new selection will be
//   updated - this is used by MCDispatch::dodrop to ensure dropped text is
//   rendered selected.
Exec_stat MCField::seltext(findex_t si, findex_t ei, Boolean focus, Boolean update)
{
	if (!opened || !(flags & F_TRAVERSAL_ON))
		return ES_NORMAL;
	if (MCactivefield != NULL)
	{
		if (MCactivefield != this && focus && !(state & CS_KFOCUSED))
			MCactivefield->kunfocus();
		if (MCactivefield != NULL)
			MCactivefield->unselect(True, True);
		if (focusedparagraph != NULL)
			focusedparagraph->setselectionindex(PARAGRAPH_MAX_LEN, PARAGRAPH_MAX_LEN, False, False);
	}
	if (focus && !(state & CS_KFOCUSED))
	{
		getstack()->kfocusset(this);
		if (!(state & CS_KFOCUSED))
			return ES_NORMAL;
    }
    // SN-2014-12-08: [[ Bug 12784 ]] Only make this field the selectedfield
    //  if it is Focusable
	else if (flags & F_TRAVERSAL_ON)
		MCactivefield = this;
	removecursor();
	
	MCParagraph *pgptr;
	pgptr = verifyindices(paragraphs, si, ei);
	
	firstparagraph = pgptr;
	firsty = paragraphtoy(firstparagraph);
	MCRectangle drect = firstparagraph->getdirty(fixedheight);
	// MW-2012-01-25: [[ FieldMetrics ]] Make sure the dirty rect is in
	//   card coords.
	drect.y += getcontenty() + firsty;
	MCRectangle frect = getfrect();
	drect.x = frect.x;
	drect.width = frect.width;
	if (flags & F_LIST_BEHAVIOR)
	{
		sethilitedlines(NULL, 0);
		drect = rect;
	}
	uint2 l;
	do
	{
		l = pgptr->gettextlengthcr();
		pgptr->setselectionindex(si, MCU_min(ei, l-1),
		                         pgptr != firstparagraph, ei > l);
		if (flags & F_LIST_BEHAVIOR)
			pgptr->sethilite(True);
		si = 0;
		ei -= l;
		pgptr = pgptr->next();
	}
	while (ei >= 0 && pgptr != paragraphs);
	lastparagraph = pgptr->prev();
	focusedparagraph = lastparagraph;
	focusedy = paragraphtoy(focusedparagraph);
	drect.height += focusedy - firsty + lastparagraph->getheight(fixedheight);
	if (focus || update)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
		layer_redrawrect(drect);
		updateparagraph(False, False);
		if (focus)
			endselection();
		replacecursor(True, True);
	}
	return ES_NORMAL;
}

uint2 MCField::hilitedline()
{
	uint2 line = 0;
	MCParagraph *pgptr = paragraphs;
	do
	{
		line++;
		if (pgptr->gethilite())
			break;
		pgptr = pgptr->next();
	}
	while (pgptr != paragraphs);
	return line;
}

void MCField::hilitedlines(vector_t<uint32_t> &r_lines)
{
    if (r_lines.elements != nil)
        delete r_lines.elements;
    if (r_lines.count != 0)
        r_lines.count = 0;;
	if (!opened || !(flags & F_LIST_BEHAVIOR))
		return;
	uinteger_t line = 0;
    MCAutoArray<uint32_t> t_lines;

	MCParagraph *pgptr = paragraphs;
	do
	{
		line++;
		if (pgptr->gethilite())
		{
            /* UNCHECKED */ t_lines . Push(line);
		}
		pgptr = pgptr->next();
	}
	while (pgptr != paragraphs);
    
    t_lines . Take(r_lines . elements, r_lines . count);
}

#ifdef LEGACY_EXEC
Exec_stat MCField::sethilitedlines(const MCString &s, Boolean forcescroll)
{
	Exec_stat t_status = ES_NORMAL;

	if (flags & F_LIST_BEHAVIOR)
	{
		uint4 *lines = NULL;
		uint4 nlines = 0;
		uint4 l = s.getlength();
		const char *sptr = s.getstring();
		while (l)
		{
			Boolean done;
			uint4 i1 = MCU_strtol(sptr, l, ',', done);
			if (!done)
			{
				MCeerror->add
				(EE_FIELD_HILITEDNAN, 0, 0, s);
				delete lines;
				return ES_ERROR;
			}
			MCU_realloc((char **)&lines, nlines, nlines + 1, sizeof(int4));
			lines[nlines++] = i1;
		}

		t_status = sethilitedlines(lines, nlines, forcescroll);
		delete lines;
	}
	return t_status;
}
#endif

Exec_stat MCField::sethilitedlines(const uint32_t *p_lines, uint32_t p_line_count, Boolean forcescroll)
{
	if (flags & F_LIST_BEHAVIOR)
	{
		uint4 line = 0;
		MCParagraph *pgptr = paragraphs;
		Boolean done = False;
		do
		{
			line++;
			uint4 i;
			for (i = 0 ; i < p_line_count ; i++)
				if (p_lines[i] == line)
				{
					pgptr->sethilite(True);
					if (i == 0)
					{
						focusedparagraph = pgptr;
						focusedy = paragraphtoy(focusedparagraph);
						done = True;
					}
					break;
				}
			if (i == p_line_count)
				pgptr->sethilite(False);
			pgptr = pgptr->next();
		}
		while (pgptr != paragraphs);

		// TS-2005-01-06: Fix for bug 2381
		//   Don't scroll to first hilitedline unless hilitedlines was set by prop
		// MW-2008-08-14: [[ Bug ]] Setting the hilitedLine(s) of a list field when
		//   it doesn't have focus can mess up arrow movements because it resets
		//   'goalx'.
		if (done)
		{
			int2 t_old_goalx;
			t_old_goalx = goalx;
			replacecursor(forcescroll, True); // force scroll
			if (cursorfield != this)
				goalx = t_old_goalx;
		}
	}
	return ES_NORMAL;
}

void MCField::hiliteline(int2 x, int2 y)
{
	MCRectangle frect = getfrect();
	if (x < frect.x || x > frect.x + frect.width
	        || y < frect.y || y > frect.y + frect.height)
		return;
	uint2 cy = y - rect.y;
	MCParagraph *pgptr = paragraphs;
	while (pgptr != curparagraph)
	{
		pgptr->sethilite(False);
		pgptr = pgptr->next();
	}
	int4 ty = cury - TEXT_Y_OFFSET;
	MCRectangle drect = frect;
	while (pgptr->next() != paragraphs
	        && ty + pgptr->getheight(fixedheight) < cy)
	{
		if (pgptr->gethilite())
		{
			pgptr->sethilite(False);
			drect.y = ty + frect.y;
			drect.height = pgptr->getheight(fixedheight);
			// MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
			layer_redrawrect(drect);
		}
		ty += pgptr->getheight(fixedheight);
		pgptr = pgptr->next();
	}
	if (!pgptr->gethilite())
	{
		pgptr->sethilite(True);
		focusedparagraph = pgptr;
		focusedy = paragraphtoy(focusedparagraph);
		drect.y = ty + frect.y;
		drect.height = pgptr->getheight(fixedheight);
		// MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
		layer_redrawrect(drect);
	}
	while (pgptr->next() != paragraphs)
	{
		ty += pgptr->getheight(fixedheight);
		pgptr = pgptr->next();
		if (pgptr->gethilite())
		{
			pgptr->sethilite(False);
			drect.y = ty + frect.y;
			drect.height = pgptr->getheight(fixedheight);
			// MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
			layer_redrawrect(drect);
		}
	}
}

bool MCField::locchar(Boolean click, MCStringRef& r_string)
{
	findex_t si, ei;
	if (locmark(False, False, click, True, True, si, ei))
		return returntext(si, ei, r_string);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCField::loccharchunk(Boolean click, MCStringRef& r_string)
{
	findex_t si, ei;
	if (locmark(False, False, click, True, True, si, ei))
		return returnchunk(si, ei, r_string);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCField::locchunk(Boolean click, MCStringRef& r_string)
{
	findex_t si, ei;
	if (locmark(False, True, click, True, True, si, ei))
		return returnchunk(si, ei, r_string);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCField::locline(Boolean click, MCStringRef& r_string)
{
	findex_t si, ei;
	if (locmark(True, True, click, True, True, si, ei))
		return returnline(si, ei, r_string);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCField::loctext(Boolean click, MCStringRef& r_string)
{
	findex_t si, ei;
	if (locmark(False, True, click, True, True, si, ei))
		return returntext(si, ei, r_string);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}
				 
Boolean MCField::locmark(Boolean wholeline, Boolean wholeword,
                         Boolean click, Boolean chunk, Boolean inc_cr, findex_t &si, findex_t &ei)
{
	int4 cx, cy;
	
	// MW-2012-01-25: [[ FieldMetrics ]] Fetch which co-ords should be used.
	if (click)
	{
		cx = clickx;
		cy = clicky;
	}
	else
	{
		cx = MCmousex;
		cy = MCmousey;
	}

	MCPoint p;
	p . x = cx;
	p . y = cy;
	
	return locmarkpoint(p, wholeline, wholeword, chunk, inc_cr, si, ei);
}

Boolean MCField::locmarkpoint(MCPoint p, Boolean wholeline, Boolean wholeword, Boolean chunk, Boolean inc_cr, int4 &si, int4 &ei)
{
	int4 cx, cy;
	
	cx = p . x;
	cy = p . y;
	
	// MW-2012-01-25: [[ FieldMetrics ]] Convert them to field coords.
	cx -= getcontentx();
	cy -= getcontenty() + cury;
	
	MCParagraph *pgptr = paragraphs;
	si = 0;
	while (pgptr != curparagraph)
	{
		si += pgptr->gettextlengthcr();
		pgptr = pgptr->next();
	}
	int4 y = 0;
	uint2 pgheight = pgptr->getheight(fixedheight);
	while (pgptr->next() != paragraphs && y + pgheight <= cy)
	{
		y += pgheight;
		si += pgptr->gettextlengthcr();
		pgptr = pgptr->next();
		pgheight = pgptr->getheight(fixedheight);
	}
	cy -= y;
	if (chunk && cy > pgptr->getheight(fixedheight))
		return False;
	if (wholeline || (wholeword && flags & F_LIST_BEHAVIOR))
	{
		ei = si + pgptr->gettextlengthcr();
		if (!inc_cr || flags & F_LIST_BEHAVIOR || pgptr->next() == paragraphs)
			ei--;
	}
	else
	{
		findex_t ssi, sei;
		ei = si;
		pgptr->getclickindex(cx, cy, fixedheight, ssi, sei, wholeword, chunk);
		si += ssi;
		ei += sei;
	}
	if (ei <= si && !wholeline)
		return False;
	return True;
}

bool MCField::foundchunk(MCStringRef& r_string)
{
	findex_t si, ei;
	if (foundmark(False, True, si, ei))
		return returnchunk(si, ei, r_string);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCField::foundline(MCStringRef& r_string)
{
	findex_t si, ei;
	if (foundmark(False, True, si, ei))
		return returnline(si, ei, r_string);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCField::foundloc(MCStringRef& r_string)
{
	findex_t si, ei;
	if (foundmark(False, True, si, ei))
		return returnloc(si, r_string);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCField::foundtext(MCStringRef& r_string)
{
	findex_t si, ei;
	if (foundmark(False, True, si, ei))
		return returntext(si, ei, r_string);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}

Boolean MCField::foundmark(Boolean wholeline, Boolean inc_cr, findex_t &si, findex_t &ei)
{
	if (foundlength == 0)
		return False;
	if (wholeline)
	{
		MCParagraph *pgptr = paragraphs;
		si = 0;
		while (si + pgptr->gettextlengthcr() <= foundoffset)
		{
			si += pgptr->gettextlengthcr();
			pgptr = pgptr->next();
		}
		ei = si + pgptr->gettextlengthcr();
		if (!inc_cr || pgptr->next() == paragraphs)
			ei--;
	}
	else
	{
		si = foundoffset;
		ei = foundoffset + foundlength;
	}
	return True;
}

bool MCField::selectedchunk(MCStringRef& r_string)
{
	findex_t si, ei;
	if (selectedmark(False, si, ei, False, true))
		return returnchunk(si, ei, r_string, true);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCField::selectedline(MCStringRef& r_string)
{
	findex_t si, ei;
	if (selectedmark(False, si, ei, False))
		return returnline(si, ei, r_string);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCField::selectedloc(MCStringRef& r_string)
{
	findex_t si, ei;
	if (selectedmark(False, si, ei, False))
		return returnloc(si, r_string);
	r_string = MCValueRetain(kMCEmptyString);
	return true;
}

#ifdef LEGACY_EXEC
void MCField::selectedtext(MCExecPoint& ep)
{
	MCAutoStringRef t_string;
	if (selectedtext(&t_string))
		ep.setvalueref(*t_string);
	else
		ep.clear();
}
#endif

bool MCField::selectedtext(MCStringRef& r_string)
{
	if (paragraphs == NULL) // never been opened
	{
		r_string = MCValueRetain(kMCEmptyString);
		return true;
	}
	if (flags & F_LIST_BEHAVIOR)
	{
		MCParagraph *pgptr = paragraphs;
		MCAutoListRef t_list;
		if (!MCListCreateMutable('\n', &t_list))
			return false;
		do
		{
			if (pgptr->gethilite())
			{
				MCAutoStringRef t_string;
				if (!pgptr -> copytextasstringref(&t_string))
					return false;
				if (!MCListAppend(*t_list, *t_string))
					return false;
			}
			pgptr = pgptr->next();
		}
		while (pgptr != paragraphs);

		if (!MCListCopyAsString(*t_list, r_string))
			return false;

		return true;
	}
	else
	{
		findex_t si, ei;
		if (selectedmark(False, si, ei, False))
			return returntext(si, ei, r_string);
		r_string = MCValueRetain(kMCEmptyString);
		return true;
	}
}

// SN-2014-04-04 [[ CombiningChars ]] We need to pay attention whether we are execpted characters or codeunit
// indices as a return value.
// MW-2014-05-28: [[ Bug 11928 ]] The 'inc_cr' parameter is not necessary - this is determined
//   by 'whole' - i.e. if 'whole' is true then select the whole paragraph inc CR.
Boolean MCField::selectedmark(Boolean whole, int4 &si, int4 &ei, Boolean force, bool p_char_indices)
{
    // SN-2014-04-03 selectedchunk return codepoint range instead of char range
    MCRange t_cu_range;
    MCRange t_char_range;
    
	MCParagraph *pgptr = paragraphs;
	si = ei = 0;
	if (flags & F_LIST_BEHAVIOR)
	{
		// MW-2005-01-12: [[Crash]] If selectedLine of a field is accessed when the field is closed
		//   we get a crash
		if (pgptr == NULL)
			return False;
			
		while (!pgptr->gethilite())
		{
            si += pgptr->gettextlengthcr(p_char_indices);
            
			pgptr = pgptr->next();
			if (pgptr == paragraphs)
				return False;
		}
		ei = si;
		ei += pgptr->gettextlengthcr(p_char_indices);
		pgptr = pgptr->next();
		while (pgptr != paragraphs && pgptr->gethilite())
		{
			ei += pgptr->gettextlengthcr(p_char_indices);
			pgptr = pgptr->next();
		}
		ei--;
	}
	else
	{
		findex_t s, e;
		if (!force && MCactivefield != this)
			return False;
		if (firstparagraph == NULL)
		{
			firstparagraph = lastparagraph = focusedparagraph;
			firsty = focusedy;
		}
		if (!focusedparagraph->isselection() && firstparagraph == lastparagraph)
		{
			while (pgptr != focusedparagraph)
			{
                si += pgptr -> gettextlengthcr(p_char_indices);
				pgptr = pgptr->next();
			}
			if (whole)
            {
                ei = si + focusedparagraph->gettextlengthcr(p_char_indices);
            }
			else
			{
				focusedparagraph->getselectionindex(s, e);
                if (p_char_indices)
                {
                    t_cu_range . offset = 0;
                    t_cu_range . length = s;
                    MCStringUnmapIndices(focusedparagraph -> GetInternalStringRef(), kMCCharChunkTypeGrapheme, t_cu_range, t_char_range);
                    ei = si += t_char_range . length;
                }
                else
                    ei = si += s;
			}
		}
		else
		{
			while (pgptr != firstparagraph)
			{
                si += pgptr->gettextlengthcr(p_char_indices);                
				pgptr = pgptr->next();
			}
			ei = si;
			if (!whole)
			{
				pgptr->getselectionindex(s, e);
                if (p_char_indices)
                {
                    t_cu_range . offset = 0;
                    t_cu_range . length = s;
                    MCStringUnmapIndices(pgptr -> GetInternalStringRef(), kMCCharChunkTypeGrapheme, t_cu_range, t_char_range);
                    si += t_char_range . length;
                }
                else
                    si += s;
			}
			while (pgptr != lastparagraph)
			{
                ei += pgptr->gettextlengthcr(p_char_indices);                
				pgptr = pgptr->next();
			}
			if (whole)
			{
                ei += pgptr->gettextlengthcr(p_char_indices);
                
				if (pgptr->next() == paragraphs)
					ei--;
			}
			else
			{
				pgptr->getselectionindex(s, e);
                if (p_char_indices)
                {
                    t_cu_range . offset = 0;
                    t_cu_range . length = e;
                    MCStringUnmapIndices(pgptr -> GetInternalStringRef(), kMCCharChunkTypeGrapheme, t_cu_range, t_char_range);
                    ei += t_char_range . length;
                }
                else
                    ei += e;
			}
		}
//        MW-2014-05-28: [[ Bug 11928 ]] 
//		if (include_cr && pgptr != NULL && e == pgptr->gettextlength() && pgptr->next() != paragraphs)
//			ei++;
	}
    
	return True;
}

bool MCField::returnchunk(findex_t p_si, findex_t p_ei, MCStringRef& r_chunk, bool p_char_indices)
{
    MCExecContext ctxt(nil, nil, nil);
	uinteger_t t_number;
	GetNumber(ctxt, 0, t_number);

	// MW-2012-02-23: [[ CharChunk ]] Map the internal field indices (si, ei) to
	//   char indices.
    // SN-2014-02-11: [[ Unicodify ]] The functions calling returnchunk already have field indices.
    // SN-2014-05-16 [[ Bug 12432 ]] Re-establish unresolving of the chars indices
    if (!p_char_indices)
        unresolvechars(0, p_si, p_ei);
	
	const char *sptr = parent->gettype() == CT_CARD && getstack()->hcaddress()
										 ? "char %d to %d of card field %d" : "char %d to %d of field %d";
	return MCStringFormat(r_chunk, sptr, p_si + 1, p_ei, t_number);
}

bool MCField::returnline(findex_t si, findex_t ei, MCStringRef& r_string)
{
    MCExecContext ctxt(nil, nil, nil);
	uinteger_t t_number;
	GetNumber(ctxt, 0, t_number);

	uint4 line = 0;
	int4 offset = 0;
	MCParagraph *pgptr = paragraphs;
	do
	{
		line++;
		offset += pgptr->gettextlengthcr();
		pgptr = pgptr->next();
	}
	while (offset <= si);
	if (offset >= ei)
	{
		const char *sptr = parent->gettype() == CT_CARD && getstack()->hcaddress()
											 ? "line %d of card field %d" : "line %d of field %d";
		return MCStringFormat(r_string, sptr, line, t_number);
	}
	else
	{
		uint4 endline = line;
		do
		{
			endline++;
			offset += pgptr->gettextlengthcr();
			pgptr = pgptr->next();
		}
		while (offset < ei);
		const char *sptr = parent->gettype() == CT_CARD && getstack()->hcaddress()
											 ? "line %d to %d of card field %d" : "line %d to %d of field %d";
		return MCStringFormat(r_string, sptr, line, endline, t_number);
	}
}

//unicode fine
bool MCField::returnloc(findex_t si, MCStringRef& r_string)
{
	// MW-2006-04-26: If the field is not opened we cannot call indextoloc so must return empty.
	if (!opened)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return true;
	}

	findex_t ei = si;
	MCParagraph *pgptr = indextoparagraph(paragraphs, si, ei);
	coord_t x, y;
	pgptr->indextoloc(si, fixedheight, x, y);

	// MW-2012-01-25: [[ FieldMetrics ]] The x, y are in paragraph coords, so
	//   translate to field, then card.
	return MCStringFormat(r_string, "%d,%d", int32_t(x + getcontentx()), int32_t(y + paragraphtoy(pgptr) + getcontenty()));
}

#ifdef LEGACY_EXEC
void MCField::returntext(MCExecPoint &ep, findex_t si, findex_t ei)
{
	// MW-2012-02-21: [[ FieldExport ]] Use the new text export method.
	exportastext(0, ep, si, ei, false);
}
#endif

bool MCField::returntext(findex_t p_si, findex_t p_ei, MCStringRef& r_string)
{
	return exportastext(0, p_si, p_ei, r_string);
}

void MCField::charstoparagraphs(findex_t si, findex_t ei, MCParagraph*& rsp, MCParagraph*& rep, uint4& rsl, uint4& rel)
{
	MCParagraph *sp, *ep;
	uint4 sl, el;

	MCParagraph *pgptr = paragraphs;
	int4 offset = 0;

	si -= 1;

	sl = 0;
	do
	{
		sl++;
		sp = pgptr;
		offset += pgptr->gettextlengthcr();
		pgptr = pgptr->next();
	}
	while (offset < si);

	el = sl;
	ep = sp;
	if (offset < ei)
	{
		do
		{
			el++;
			ep = pgptr;
			offset += pgptr->gettextlengthcr();
			pgptr = pgptr->next();
		}
		while (offset < ei);
	}

	rsp = sp;
	rep = ep;

	rsl = sl;
	rel = el;
}

void MCField::linestoparagraphs(findex_t si, findex_t ei, MCParagraph*& rsp, MCParagraph*& rep)
{
	MCParagraph *sp, *ep;

	MCParagraph *pgptr = paragraphs;
	int4 offset = 0;

	do
	{
		offset++;
		sp = pgptr;
		pgptr = pgptr->next();
	}
	while (offset < si);

	ep = sp;
	if (offset < ei)
	{
		do
		{
			offset++;
			ep = pgptr;
			pgptr = pgptr->next();
		}
		while (offset < ei);
	}

	rsp = sp;
	rep = ep;
}

MCParagraph *MCField::cloneselection()
{
	MCParagraph *cutptr = NULL;
	if (flags & F_LIST_BEHAVIOR)
	{
		MCParagraph *pgptr = paragraphs;
		do
		{
			if (pgptr->gethilite())
			{
				MCParagraph *tpgptr = new MCParagraph(*pgptr);
				tpgptr->appendto(cutptr);
			}
			pgptr = pgptr->next();
		}
		while (pgptr != paragraphs);
	}
	else
	{
		if ((focusedparagraph == NULL || !focusedparagraph->isselection())
						&& firstparagraph == lastparagraph)
			return NULL;
		MCParagraph *pgptr = firstparagraph;
		if (pgptr == NULL)
			return NULL;
		cutptr = pgptr->copyselection();
		if (firstparagraph != lastparagraph)
		{
			do
			{
				pgptr = pgptr->next();
				pgptr->copyselection()->appendto(cutptr);
			}
			while (pgptr != lastparagraph);
		}
		// MW-2007-10-11: [[ Bug 1531 ]] - No check necessary here:
		//   if the user wishes to copy the line-ending at the end of a paragraph
		//   they can do so by selecting up until the beginning of the next
		//   paragraph
	}
	return cutptr;
}


bool MCField::pickleselection(MCDataRef& r_string)
{
	// MW-2012-02-17: [[ SplitTextAttrs ]] When pickling in the field, make the
	//   styledtext's parent the field so that font attr inheritence works.
	MCStyledText t_styled_text;
	t_styled_text . setparent(this);
	t_styled_text . setparagraphs(cloneselection());
	
	/* UNCHECKED */ MCObject::pickle(&t_styled_text, 0, r_string);
	return true;
}

// Will reflow
void MCField::cuttext()
{
	// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
	//   textChanged message by a lock screen pair.
	MCRedrawLockScreen();
	copytext();
	removecursor();
	deleteselection(True);
	
	// MW-2012-03-16: [[ Bug 3173 ]] Make sure the width is updated and such
	//   so that the caret repositions itself correctly.
	do_recompute(true);
	layer_redrawall();
	
	replacecursor(True, True);
	state |= CS_CHANGED;

	// MW-2012-09-21: [[ Bug 10401 ]] Make sure we unlock the screen!.
	MCRedrawUnlockScreen();
	
	// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
	//   was called as a result of a user action (cut key, cut command). 
	textchanged();
}

void MCField::copytext()
{
    // Do nothing if there is nothing to copy
    if (!focusedparagraph->isselection() && firstparagraph == lastparagraph)
		return;

    // Serialise the text. Failures are ignored here as there isn't really a
    // good way to alert the user that a copy-to-clipboard operation failed.
	MCAutoDataRef t_data;
	pickleselection(&t_data);
	if (*t_data != nil)
    {
        // Clear the clipboard and copy the selection to it
        MCclipboard->Clear();
        MCclipboard->AddLiveCodeStyledText(*t_data);
    }
}

void MCField::cuttextindex(uint4 parid, findex_t si, findex_t ei)
{
	if (!opened || getcard()->getid() != parid)
		return;
	seltext(si, ei, False);
	cuttext();
}

void MCField::copytextindex(uint4 parid, findex_t si, findex_t ei)
{
	if (!opened || getcard()->getid() != parid)
		return;
	seltext(si, ei, False);
	copytext();
}

// MW-2006-04-26: Only paste text into open fields. Also, we should honor dodel
//   regardless.
void MCField::pastetext(MCParagraph *newtext, Boolean dodel)
{
	if ((flags & F_LOCK_TEXT) == 0 && !getstack()->islocked() && newtext != NULL && opened)
	{
		MCundos->freestate();
		deleteselection(False);
		Ustruct *us = MCundos->getstate();
		if (us == NULL || MCundos->getobject() != this)
		{
			us = new Ustruct;
			findex_t si, ei;
			selectedmark(False, si, ei, False);
			us->ud.text.index = si;
			us->ud.text.newline = False;
			us->ud.text.data = NULL;
			MCundos->savestate(this, us);
		}
		us->type = UT_TYPE_TEXT;
		us->ud.text.newchars = 0;
		MCParagraph *tpgptr = newtext;
		do
		{
			us->ud.text.newchars += tpgptr->gettextlengthcr();
			tpgptr = tpgptr->next();
		}
		while (tpgptr != newtext);
		if (us->ud.text.newchars)
			us->ud.text.newchars--;
		insertparagraph(newtext);
		// MW-2012-03-16: [[ Bug ]] When pasting text ensure we replace the cursor
		//   forcing a scroll in view.
		replacecursor(True, True);
	}

	if (dodel)
		while (newtext != NULL)
		{
			MCParagraph *tpgptr;
			tpgptr = newtext->remove(newtext);
			delete tpgptr;
		}
}

void MCField::movetext(MCParagraph *newtext, findex_t p_to_index)
{
	if ((flags & F_LOCK_TEXT) == 0 && !getstack()->islocked() && opened)
	{
		findex_t si, ei;
		selectedmark(False, si, ei, False);
		if (si < p_to_index)
			p_to_index -= ei - si;

		MCundos->freestate();
		deleteselection(False);
		Ustruct *us = MCundos->getstate();
		if (us == NULL || MCundos->getobject() != this)
		{
			us = new Ustruct;
			us->ud.text.index = si;
			us->ud.text.newline = False;
			us->ud.text.data = NULL;
			MCundos->savestate(this, us);
		}
		us->type = UT_MOVE_TEXT;
		us->ud.text.newchars = 0;
		us->ud.text.old_index = us->ud.text.index;
		us->ud.text.index = p_to_index;
		MCParagraph *tpgptr = newtext;
		do
		{
			us->ud.text.newchars += tpgptr->gettextlengthcr();
			tpgptr = tpgptr->next();
		}
		while (tpgptr != newtext);
		if (us->ud.text.newchars)
			us->ud.text.newchars--;
		seltext(p_to_index, p_to_index, True);
		insertparagraph(newtext);
	}

	while (newtext != NULL)
	{
		MCParagraph *tpgptr;
		tpgptr = newtext->remove(newtext);
		delete tpgptr;
	}
}

void MCField::deletetext(findex_t si, findex_t ei)
{
	MCParagraph *t_deleted_text;
	t_deleted_text = clonetext(si, ei);
	if (t_deleted_text == NULL)
		return;

	Ustruct *us;
	us = new Ustruct;
	us->type = UT_DELETE_TEXT;
	us->ud.text.index=si;
	us->ud.text.data = t_deleted_text;
	us->ud.text.newline = False;
	MCundos->savestate(this, us);
	settextindex(0, si, ei, kMCEmptyString, True);
}

MCParagraph *MCField::clonetext(findex_t si, findex_t ei)
{
	MCParagraph *t_paragraph;
	t_paragraph = verifyindices(paragraphs, si, ei);

	MCParagraph *t_new_paragraphs;
	t_new_paragraphs = NULL;

	uint2 l;
	do
	{
		l = t_paragraph -> gettextlengthcr();
		
		MCParagraph *t_new_paragraph;
		t_new_paragraph = t_paragraph -> copystring(si, MCU_min(l - 1, ei));
		if (t_new_paragraphs == NULL)
			t_new_paragraphs = t_new_paragraph;
		else
			t_new_paragraph -> appendto(t_new_paragraphs);

		si = 0;
		ei -= l;
		t_paragraph = t_paragraph -> next();
	}
	while(ei >= 0 && t_paragraph != paragraphs);

	return t_new_paragraphs;
}

void MCField::insertparagraph(MCParagraph *newtext)
{
	uint4 oldflags = flags;
	flags &= ~F_VISIBLE;
	deleteselection(False);
    // MW-2014-06-10: [[ Bug 12589 ]] Make sure the paragraph's selection is unset.
    //   If we don't do this portions of the paragraph will still think they are
    //   selected causing strange behavior.
    focusedparagraph -> deleteselection();
	textheight -= focusedparagraph->getheight(fixedheight);
	focusedparagraph->split();
	MCParagraph *oldend = focusedparagraph->next();
	MCParagraph *pgptr = newtext;
	MCParagraph *tfptr = focusedparagraph;
	MCStack *oldstack = newtext->getparent()->getstack();
	if (oldstack == getstack())
		oldstack = NULL;
	do
	{
		MCParagraph *newptr = new MCParagraph(*pgptr);
		newptr->setparent(this);
		newptr->open(m_font);
		tfptr->append(newptr);
		pgptr = pgptr->next();
		tfptr = tfptr->next();
	}
	while (pgptr != newtext);
	focusedparagraph->join();
	updateparagraph(True, False, False);
	uint2 newheight;
	while (focusedparagraph->next() != oldend)
	{
		newheight = focusedparagraph->getheight(fixedheight);
		focusedy += newheight;
		focusedparagraph = focusedparagraph->next();
		updateparagraph(True, False, False);
	}
	textheight -= focusedparagraph->getheight(fixedheight);
	findex_t si = focusedparagraph->gettextlength();
	focusedparagraph->join();
	firstparagraph = lastparagraph = NULL;
	flags = oldflags;
	updateparagraph(True, True);
	coord_t x, y;
	focusedparagraph->indextoloc(si, fixedheight, x, y);

	// MW-2012-01-25: [[ FieldMetrics ]] Translate the para coords to field, then
	//   card.
	x += getcontentx();
	y += getcontenty() + focusedy;
	firstparagraph = lastparagraph = NULL;
	setfocus(x, y);
	state |= CS_CHANGED;
}

////////////////////////////////////////////////////////////////////////////////

MCRectangle MCField::firstRectForCharacterRange(int32_t& si, int32_t& ei)
{
	MCParagraph *pgptr = resolveparagraphs(0);
	
	// These will be paragraph relative (after indextoparagraph).
	int32_t t_si, t_ei;
	t_si = si;
	t_ei = ei;
	
	// Fetch the paragraph and indicies within it.
	int4 t_line_index;
	MCParagraph *sptr;
	sptr = indextoparagraph(pgptr, t_si, t_ei, &t_line_index);
	
	// Restrict the range to the line t_si starts on.
	sptr -> restricttoline(t_si, t_ei);
	
	// Now update the output range (the indices get munged in what follows)
	si = si + t_si;
	ei = si + t_ei;
	
	// Get the x, y of the initial index.
	coord_t x, y;
	sptr->indextoloc(t_si, fixedheight, x, y);
	
	// Get the offset for computing card coords.
	int4 yoffset = getcontenty() + paragraphtoy(sptr);
	
	// Get the extent of the range.
	coord_t minx, maxx;
	sptr -> getxextents(t_si, t_ei, minx, maxx);
	
	MCRectangle t_rect;
	t_rect . x = minx + getcontentx();
	t_rect . y = y + yoffset;
	t_rect . width = maxx - minx;
	t_rect . height = sptr -> heightoflinewithindex(t_si, fixedheight);
	
	return t_rect;
}

////////////////////////////////////////////////////////////////////////////////

