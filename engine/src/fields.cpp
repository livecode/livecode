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

#include "execpt.h"
#include "sellst.h"
#include "undolst.h"
#include "stack.h"
#include "card.h"
#include "cdata.h"
#include "field.h"
#include "block.h"
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

Exec_stat MCField::sort(MCExecPoint &ep, uint4 parid, Chunk_term type,
                        Sort_type dir, Sort_type form, MCExpression *by)
{
	MCSortnode *items = NULL;
	uint4 nitems = 0;
	uint4 itemsize = 0;
	char *itemtext = NULL;
	MCParagraph *pgptr;
	MCString s;

	if (flags & F_SHARED_TEXT)
		parid = 0;

	if (type == CT_ITEM)
	{
		if (ep.getitemdel() == '\0')
			return ES_NORMAL; //can't sort field by null bytes
		// MW-2012-02-21: [[ FieldExport ]] Use the new text export method.
		exportastext(parid, ep, 0, INT32_MAX, false);
		itemsize = ep.getsvalue().getlength();
		itemtext = ep.getsvalue().clone();
		char *sptr = itemtext;
		char *eptr;
		while ((eptr = strchr(sptr, ep.getitemdel())) != NULL)
		{
			nitems++;
			sptr = eptr + 1;
		}
		items = new MCSortnode[nitems + 1];
		nitems = 0;
		sptr = itemtext;
		do
		{
			if ((eptr = strchr(sptr, ep.getitemdel())) != NULL)
			{
				*eptr++ = '\0';
				s.set(sptr, eptr - sptr - 1);
			}
			else
				s.set(sptr, strlen(sptr));
			MCSort::additem(ep, items, nitems, form, s, by);
			items[nitems - 1].data = (void *)sptr;
			sptr = eptr;
		}
		while (eptr != NULL);
	}
	else
	{
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
			items = new MCSortnode[nitems];
			nitems = 0;
			do
			{
				s.set(tpgptr->gettext(), tpgptr->gettextsize());
				if (tpgptr->next() != pgptr || tpgptr->gettextsize())
				{
					MCSort::additem(ep, items, nitems, form, s, by);
					items[nitems - 1].data = (void *)tpgptr;
				}
				tpgptr = tpgptr->next();
			}
			while (tpgptr != pgptr);
		}
	}
	MCU_sort(items, nitems, dir, form);
	if (type == CT_ITEM)
	{
		char *newtext = new char[itemsize + 1];
		*newtext = '\0';
		uint4 i;
		uint4 tlength = 0;
		for (i = 0 ; i < nitems ; i++)
		{
			uint4 length = strlen((const char *)items[i].data);
			strncpy(&newtext[tlength], (const char *)items[i].data, length);
			tlength += length;
			if ((form == ST_INTERNATIONAL || form == ST_TEXT)
			        && (!ep.getcasesensitive() || by != NULL))
				delete items[i].svalue;
			if (i < nitems - 1)
				newtext[tlength++] = ep.getitemdel();
			newtext[tlength] = '\0';
		}
		delete itemtext;
		settext(parid, newtext, False);
		delete newtext;
	}
	else if (nitems > 0)
	{
		MCParagraph *newparagraphs = NULL;
		uint4 i;
		for (i = 0 ; i < nitems ; i++)
		{
			MCParagraph *tpgptr = (MCParagraph *)items[i].data;
			tpgptr->remove(pgptr);
			tpgptr->appendto(newparagraphs);
			if ((form == ST_INTERNATIONAL || form == ST_TEXT)
			        && (!ep.getcasesensitive() || by != NULL))
				delete items[i].svalue;
		}
		if (pgptr != NULL)
			pgptr->appendto(newparagraphs);
		getcarddata(fdata, parid, True)->setparagraphs(newparagraphs);
		if (opened && (parid == 0 || parid == getcard()->getid()))
		{
			paragraphs = newparagraphs;
			resetparagraphs();
			recompute();

			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
		}
	}
	delete items;
	return ES_NORMAL;
}

Boolean MCField::find(MCExecPoint &ep, uint4 cardid, Find_mode mode,
                      const MCString &tofind, Boolean first)
{
	if (fdata == NULL || flags & F_F_DONT_SEARCH)
		return False;
	if (opened)
		fdata->setparagraphs(paragraphs);
	if (flags & F_SHARED_TEXT)
		cardid = 0;
	MCCdata *tptr = fdata;
	do
	{
		if (tptr->getid() == cardid)
		{
			MCParagraph *paragraphptr = tptr->getparagraphs();
			MCParagraph *tpgptr = paragraphptr;
			uint2 flength = tofind.getlength();
			int4 toffset, oldoffset;
			if (first && foundlength != 0)
			{
				int4 junk = toffset = oldoffset = foundoffset + foundlength;
				tpgptr = indextoparagraph(tpgptr, oldoffset, junk);
				toffset -= oldoffset;
			}
			else
				toffset = oldoffset = 0;
			do
			{
				const char *text = tpgptr->gettext();
				uint2 length = tpgptr->gettextsize();
				uint4 offset;
				MCString tosearch(&text[oldoffset], length - oldoffset);
				while (MCU_offset(tofind, tosearch, offset, ep.getcasesensitive()))
				{
					offset += oldoffset;
					switch (mode)
					{
					case FM_NORMAL:
						if (offset == 0 || isspace((uint1)text[offset - 1])
						        || ispunct((uint1)text[offset - 1]))
						{
							if (first)
							{
								if (MCfoundfield != NULL && MCfoundfield != this)
									MCfoundfield->clearfound();
								foundoffset = toffset + offset;
								toffset = offset++;
								while (offset < length && !isspace((uint1)text[offset])
								        && !ispunct((uint1)text[offset]))
									offset++;
								foundlength = offset - toffset;
								MCfoundfield = this;
							}
							return True;
						}
						offset++;
						break;
					case FM_WHOLE:
					case FM_WORD:
						if ((offset == 0 || isspace((uint1)text[offset - 1])
						        || ispunct((uint1)text[offset - 1]))
						        && (offset + flength == length
						            || isspace((uint1)text[offset + flength])
						            || ispunct((uint1)text[offset + flength])))
						{
							if (first)
							{
								if (MCfoundfield != NULL && MCfoundfield != this)
									MCfoundfield->clearfound();
								foundoffset = toffset + offset;
								foundlength = tofind.getlength();
								MCfoundfield = this;
							}
							return True;
						}
						offset++;
						break;
					case FM_CHARACTERS:
					case FM_STRING:
						if (first)
						{
							if (MCfoundfield != NULL && MCfoundfield != this)
								MCfoundfield->clearfound();
							foundoffset = toffset + offset;
							foundlength = tofind.getlength();
							MCfoundfield = this;
						}
					default:
						return True;
					}
					oldoffset = offset;
					tosearch.set(&text[oldoffset], length - oldoffset);
				}
				toffset += tpgptr->gettextsizecr();
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

void MCField::verifyindex(MCParagraph *top, int4 &si, bool p_is_end)
{
	int4 oldindex = si;
	int4 junk = si;
	MCParagraph *pgptr = indextoparagraph(top, si, junk);
	MCBlock *bptr = pgptr->indextoblock(si, False);
	si = (oldindex - si) + bptr->verifyindex(si, p_is_end);
}

// MW-2012-02-08: [[ Field Indices ]] If 'index' is non-nil then we return the
//   1-based index of the paragraph that si resides in.
MCParagraph *MCField::indextoparagraph(MCParagraph *top, int4 &si, int4 &ei, int4* index)
{
	int4 t_index;
	uint2 l = top->gettextsizecr();
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
		l = pgptr->gettextsizecr();
	}
	if (index != nil)
		*index = t_index;
	return pgptr;
}

uint4 MCField::ytooffset(int4 y)
{
	uint4 si = 0;
	MCParagraph *tptr = paragraphs;
	while (True)
	{
		y -= tptr->getheight(fixedheight);
		if (y <= 0)
			break;
		si += tptr->gettextsizecr();
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

bool MCField::nativizetext(uint4 parid, MCExecPoint& ep, bool p_ascii_only)
{
	bool t_has_unicode;
	t_has_unicode = false;
	
	// Resolve the correct collection of paragraphs.
	MCParagraph *t_paragraphs;
	t_paragraphs = resolveparagraphs(parid);
	if (t_paragraphs == nil)
	{
		ep . clear();
		return t_has_unicode;
	}

	// Ensure there is room in ep for the data. The size of the data is the
	// same as the original, so just use getpgsize().
	char *t_data;
	t_data = ep . getbuffer(getpgsize(t_paragraphs));
	
	// Keep track of how much of the buffer we've used.
	uint32_t t_length;
	t_length = 0;
	
	// Now loop through the paragraphs, converting as appropriate.
	MCParagraph *t_paragraph;
	t_paragraph = t_paragraphs;
	do
	{
		if (t_paragraph -> nativizetext(p_ascii_only, t_data, t_length))
			t_has_unicode = true;
		t_paragraph = t_paragraph -> next();
		if (t_paragraph != t_paragraphs)
			t_data[t_length++] = '\n';
	}
	while(t_paragraph != t_paragraphs);
	
	// Update the length of the ep.
	ep . setlength(t_length);
	
	return t_has_unicode;
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

uint4 MCField::getpgsize(MCParagraph *pgptr)
{
	if (pgptr == NULL)
		pgptr = paragraphs;
	MCParagraph *tpgptr = pgptr;
	uint4 length = 0;
	do
	{
		length += tpgptr->gettextsizecr();
		tpgptr = tpgptr->next();
	}
	while (tpgptr != pgptr);
	return length;
}

void MCField::setparagraphs(MCParagraph *newpgptr, uint4 parid)
{
	if (flags & F_SHARED_TEXT)
		parid = 0;
	else
		if (parid == 0)
			parid = getcard()->getid();
	MCCdata *fptr = getcarddata(fdata, parid, True);
	MCParagraph *pgptr = fptr->getparagraphs();
	if (opened && fptr == fdata)
		closeparagraphs(pgptr);
	if (pgptr == paragraphs)
		paragraphs = NULL;
	if (pgptr == oldparagraphs)
		oldparagraphs = NULL;
	while (pgptr != NULL)
	{
		MCParagraph *tpgptr = pgptr->remove
		                      (pgptr);
		delete tpgptr;
	}
	
	// MW-2008-03-13: [[ Bug 5383 ]] Crash when saving after initiating a URL download
	//   Here it is important to 'setparagraphs' on the MCCdata object *before* opening. This
	//   is because it is possible for URL downloads to be initiated in openparagraphs, which 
	//   allow messages to be processed which allows a save to occur and its important that
	//   the field be in a consistent state.
	if (opened && fptr == fdata)
	{
		paragraphs = newpgptr;
		fptr->setparagraphs(newpgptr);
		openparagraphs();
		recompute();

		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	else
		fptr->setparagraphs(newpgptr);
}

Exec_stat MCField::settext(uint4 parid, const MCString &s, Boolean formatted, Boolean isunicode)
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
	if (s.getlength())
	{
		const char *sptr = s.getstring();
		uint4 l = s.getlength();
		while (True)
		{
			const char *eptr = sptr;
			if (formatted)
				while (l)
				{
					if (MCU_strchr(eptr, l, '\n', isunicode) && l)
					{
						if (l && MCU_comparechar(eptr + MCU_charsize(isunicode), '\n',
						                         isunicode))
							break;
						else
						{
							eptr += MCU_charsize(isunicode);
							l -= MCU_charsize(isunicode);
						}
					}
					else
					{
						eptr += l;
						l = 0;
					}
				}
			else
				if (!MCU_strchr(eptr, l, '\n', isunicode))
				{
					eptr += l;
					l = 0;
				}
			uint4 length = eptr - sptr;
			if (length > MAXUINT2 - 2)
			{
				length = MAXUINT2 - 2;
				eptr = sptr + length;
			}
			char *pgtext = new char[length ? length : 1];
			memcpy(pgtext, sptr, length);
			if (formatted)
			{
				char *tptr = pgtext;
				while (*tptr)
				{
					if (*tptr == '\n')
						*tptr = ' ';
					tptr++;
				}
			}
			MCParagraph *tpgptr = new MCParagraph;
			tpgptr->setparent(this);
			tpgptr->appendto(pgptr);

			// MW-2012-02-16: [[ IntrinsicUnicode ]] Make sure we pass the correct setting for
			//   'unicode'.
			tpgptr->settext(pgtext, length, isunicode == True);
			if (l)
			{
				sptr = eptr + MCU_charsize(isunicode);
				l -= MCU_charsize(isunicode);
				if (formatted && l)
				{
					sptr+=MCU_charsize(isunicode);
					l-=MCU_charsize(isunicode);
				}
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
MCParagraph *MCField::verifyindices(MCParagraph *p_top, int4& si, int4& ei)
{
	MCParagraph *t_start_pg;
	t_start_pg = indextoparagraph(p_top, si, ei);
	
	MCBlock *t_start_block;
	t_start_block = t_start_pg -> indextoblock(si, False);
	si = t_start_block -> verifyindex(si, false);
	
	int4 t_new_ei, t_junk;
	MCParagraph *t_end_pg;
	t_new_ei = ei;
	t_junk = ei;
	t_end_pg = indextoparagraph(t_start_pg, t_new_ei, t_junk);
	
	MCBlock *t_end_block;
	t_end_block = t_end_pg -> indextoblock(t_new_ei, False);
	ei = (ei - t_new_ei) + t_end_block -> verifyindex(t_new_ei, true);
	
	return t_start_pg;
}

Exec_stat MCField::settextindex(uint4 parid, int4 si, int4 ei, const MCString &s, Boolean undoing, bool p_as_unicode)
{
	state &= ~CS_CHANGED;
	if (!undoing)
		MCundos->freestate();
	if (flags & F_SHARED_TEXT)
		parid = 0;
	else
		if (parid == 0)
			parid = getcard()->getid();
	MCCdata *fptr = getcarddata(fdata, parid, True);
	if (opened && fptr == fdata && focusedparagraph != NULL)
	{
		clearfound();
		unselect(False, True);
		focusedparagraph->setselectionindex(MAXUINT2, MAXUINT2, False, False);
	}
	int4 oldsi = si;
	
	MCParagraph *toppgptr = fptr->getparagraphs();

	MCParagraph *pgptr;
	pgptr = verifyindices(toppgptr, si, ei);
	
	if (si != ei)
	{
		int4 tei;
		if (ei >= pgptr->gettextsizecr())
		{
			tei = pgptr->gettextsize();
			ei--;
			if (ei == tei && pgptr->next() != toppgptr)
				pgptr->join();
		}
		else
			tei = ei;
		ei -= tei;
		MCParagraph *saveparagraph = pgptr;
		int4 savey = 0;
		if (opened && pgptr == paragraphs)
			savey = paragraphtoy(saveparagraph);
		pgptr->deletestring(si, tei);
		if (ei > 0)
		{
			pgptr = pgptr->next();
			while (ei >= pgptr->gettextsizecr())
			{
				ei -= pgptr->gettextsizecr();
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
			}
		}
	}
	pgptr->setparent(this);
	pgptr->setselectionindex(si, si, False, False);
	
	Boolean t_need_recompute = False;

	// MW-2012-02-13: [[ Block Unicode ]] Use the new finsert method in native mode.
	// MW-2012-02-23: [[ PutUnicode ]] Pass through the encoding to finsertnew.
	if (s.getlength())
		t_need_recompute = pgptr->finsertnew(s, p_as_unicode);

	if (opened && fptr == fdata)
	{
		oldsi += s.getlength();
		ei = oldsi;
		focusedparagraph = indextoparagraph(paragraphs, oldsi, ei);
		if (state & CS_KFOCUSED)
			focusedparagraph->setselectionindex(ei, ei, False, False);

		recompute();
		
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
		
		focusedy = paragraphtoy(focusedparagraph);
	}

	return ES_NORMAL;
}

// MW-2011-03-13: [[ Bug 9447 ]] Make sure we update linksi/linkei with the appropriate
//   indices after stretching out the region.
void MCField::getlinkdata(MCRectangle &lrect, MCBlock *&sb, MCBlock *&eb)
{
	int32_t si, ei;
	si = linksi;
	ei = linkei;
	int2 maxx;
	MCParagraph *sptr = indextoparagraph(paragraphs, si, ei);
	linksi -= si;
	linkei -= si;
	ei = MCU_min(ei, sptr->gettextsizecr());
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
	uint2 t_index;
	t_index = (uint2)si;
	sb = sptr -> extendup(sb, t_index);
	si = t_index;
	t_index = (uint2)(ei - 1);
	eb = sptr -> extenddown(eb, t_index);
	ei = t_index;
	
	linksi += si;
	linkei = linksi + (ei - si);

	sptr->indextoloc(si, fixedheight, maxx, lrect.y);

	// MW-2012-01-25: [[ FieldMetrics ]] Compute the y-offset in card coords.
	uint4 yoffset = getcontenty() + paragraphtoy(sptr);
	lrect.height = sptr->getyextent(ei, fixedheight);
	sptr->getxextents(si, ei, lrect.x, maxx);
	// MW-2012-01-25: [[ FieldMetrics ]] Make sure the linkrect is in card coords.
	lrect.height -= lrect.y;
	lrect.y += yoffset;
	lrect.width = maxx - lrect.x;
	lrect.x += getcontentx();
}

// MW-2012-01-25: [[ ParaStyles ]] The 'is_line_chunk' parameter is true when a text
//   attribute is set directly on a line (used to disambiguate backColor).
Exec_stat MCField::gettextatts(uint4 parid, Properties which, MCExecPoint &ep, MCNameRef index, Boolean effective, int4 si, int4 ei, bool is_line_chunk)
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
	int4 t_line_index, t_char_index;
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
			//   If minx > maxx then just assume bother are 0.
			if (minx > maxx)
				minx = maxx = 0;

			if (which == P_FORMATTED_LEFT)
				ep.setnvalue(getcontentx() + minx);
			else
				ep.setnvalue(maxx - minx);
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
				if (maxy != 0)
					maxy += sptr -> prev() -> computebottommargin() + sptr -> computetopmargin();
				maxy += sptr->getyextent(ei, fixedheight);
				ei -= sptr->gettextsizecr();
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
			ep.setrectangle(minx + getcontentx(), y + yoffset, maxx + getcontentx(), (maxy - y) + yoffset);
		}
		else
			ep.setrectangle(0, 0, 0, 0);
		break;
	case P_LINK_TEXT:
		ep.setsvalue(sptr->getlinktext(si));
		break;
	case P_IMAGE_SOURCE:
		ep.setsvalue(sptr->getimagesource(si));
		break;
	// MW-2012-01-06: [[ Block Metadata ]] Handle fetching the metadata about a given
	//   index.
	case P_METADATA:
		if (is_line_chunk)
			ep.setnameref_unsafe(sptr -> getmetadata());
		else
			ep.setsvalue(sptr->getmetadataatindex(si));
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
				ei -= sptr->gettextsizecr();
				
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
				// making sure the ranges are adjusted to the start of the range.
				sptr -> getflaggedranges(parid, ep, si, ei, t_paragraph_offset);
				
				// MW-2013-07-31: [[ Bug 10957 ]] Update the paragraph (byte) offset.
				t_paragraph_offset += sptr -> gettextsizecr();
				
				// Reduce ei until we get to zero, advancing through the paras.
				si = 0;
				ei -= sptr -> gettextsizecr();
				
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

				ei -= sptr->gettextsizecr();
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
		pname = MCNameGetCString(pname_name);

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
			if (MCF_parsetextstyle(MCNameGetOldString(index), t_text_style) != ES_NORMAL)
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
			if (sptr->getatts(si, ei, t_text_style, tname, tsize, tstyle, tcolor, tbcolor, tshift, tspecstyle, tmixed))
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
			ei -= sptr->gettextsizecr();
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
				stat = MCF_unparsetextatts(which, ep, flags, fname, height, size, style);
			break;
		case P_TEXT_SIZE:
			if (mixed & MIXED_SIZES)
				ep.setstaticcstring(MCmixedstring);
			else
				stat = MCF_unparsetextatts(which, ep, flags, fname, height, size, style);
			break;
		case P_TEXT_STYLE:
			// MW-2011-11-23: [[ Array TextStyle ]] If no textStyle has been specified
			//   then we are processing the whole set, otherwise process a specific one.
			if (t_text_style == FTS_UNKNOWN)
			{
				if (mixed & MIXED_STYLES)
					ep.setstaticcstring(MCmixedstring);
				else
					stat = MCF_unparsetextatts(which, ep, flags, fname, height, size, style);
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

// MW-2011-12-08: [[ StyledText ]] We now take the execpoint directly so that array
//   values can be used.
// MW-2012-01-25: [[ ParaStyles ]] The 'is_line_chunk' parameter is true if the prop
//   is being set on a line directly.
Exec_stat MCField::settextatts(uint4 parid, Properties which, MCExecPoint& ep, MCNameRef index, int4 si, int4 ei, bool is_line_chunk, bool dont_layout)
{
	// Fetch the string value of the ep as 's' for compatibility with pre-ep taking
	// code.
	const MCString& s = ep . getsvalue();
	
	if (flags & F_SHARED_TEXT)
		parid = 0;

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
		settextindex(parid, si, ei, MCnullmcstring, False);
		MCCdata *fptr = getcarddata(fdata, parid, True);
		MCParagraph *oldparagraphs = fptr->getparagraphs();
		fptr->setset(0);
		switch (which)
		{
		case P_HTML_TEXT:
			sethtml(parid, s);
			break;
		case P_RTF_TEXT:
			setrtf(parid, s);
			break;
		// MW-2011-12-08: [[ StyledText ]] Import the styled text.
		case P_STYLED_TEXT:
			setstyledtext(parid, ep);
			break;
		case P_UNICODE_TEXT:
		case P_TEXT:
			setpartialtext(parid, s, which == P_UNICODE_TEXT);
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
		pgptr->join();
		if (lastpgptr == NULL)
			lastpgptr = pgptr;
		else
			pgptr->defrag();
		lastpgptr->join();
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
			recompute();
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
				int32_t t_range_start, t_range_end;
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
	char *fname = NULL;
	uint2 size = 0;
	uint2 style = FA_DEFAULT_STYLE;
	MCColor tcolor;
	tcolor.red = tcolor.green = tcolor.blue = 0;
	MCColor *color = &tcolor;
	char *cname = NULL;
	int2 shift = 0;
	Boolean all = False;
	Boolean newstate;
	void *t_value;

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
		if (!s.getlength())
			color = NULL;
		else if (!MCscreen->parsecolor(s, &tcolor, &cname))
			return ES_ERROR;
		t_value = color;
		break;
	case P_TEXT_STYLE:
		// MW-2011-11-23: [[ Array TextStyle ]] If we have an index then change the prop
		//   to the pseudo add/remove ones. In this case 'value' is the textStyle to process.
		if (!MCNameIsEmpty(index))
		{
			Font_textstyle t_text_style;

			if (MCF_parsetextstyle(MCNameGetOldString(index), t_text_style) != ES_NORMAL)
				return ES_ERROR;

			Boolean t_new_state;
			if (!MCU_stob(s, t_new_state))
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, s);
				return ES_ERROR;
			}

			if (t_new_state)
				which = P_TEXT_STYLE_ADD;
			else
				which = P_TEXT_STYLE_REMOVE;

			t_value = (void *)t_text_style;
			break;
		}
		// Fall through for default (non-array) handling.
	case P_TEXT_FONT:
	case P_TEXT_SIZE:
		if (MCF_parsetextatts(which, s, flags, fname, fontheight, size, style) != ES_NORMAL)
			return ES_ERROR;
		all = True;
		if (which == P_TEXT_FONT)
			t_value = (void *)fname;
		else if (which == P_TEXT_SIZE)
			t_value = (void *)size;
		else
			t_value = (void *)style;
		break;
	case P_TEXT_SHIFT:
		if (!MCU_stoi2(s, shift))
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
		fname = s.clone();
		t_value = (void *)fname;
		break;
	// MW-2012-01-26: [[ FlaggedField ]] Set the flagged status of the range.
	case P_FLAGGED:
		if (!MCU_stob(s, newstate))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, s);
			return ES_ERROR;
		}
		t_value = (void *)(Boolean)newstate;
		t_value = (void *)newstate;
		break;
	case P_VISITED:
		if (!MCU_stob(s, newstate))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, s);
			return ES_ERROR;
		}
		pgptr->setvisited(si, MCU_min(ei, pgptr->gettextsize()), newstate);
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
	int4 ssi = 0;
	int4 sei = 0;
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
				selectedmark(False, ssi, sei, False, False);
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
		uint2 l = pgptr->gettextsizecr();
		if (si < l)
		{
			pgptr->setparent(this);

			if (which != P_UNDEFINED)
			{
				if (!t_is_para_attr)
					pgptr->setatts(si, MCU_min(ei, pgptr->gettextsize()), which, t_value);
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
				pgptr -> layout();
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
	delete cname;
	delete fname;
	if (t_need_layout)
	{
		if (all)
		{
			recompute();
			hscroll(savex - textx, False);
			vscroll(savey - texty, False);
			resetscrollbars(True);
			if (MCactivefield == this)
				seltext(ssi, sei, False);
		}
		else
			removecursor();
		// MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
		layer_redrawrect(drect);
		if (!all)
			replacecursor(False, True);
	}
	return t_stat;
}

// MW-2008-01-30: [[ Bug 5754 ]] If update is true the new selection will be
//   updated - this is used by MCDispatch::dodrop to ensure dropped text is
//   rendered selected.
Exec_stat MCField::seltext(int4 si, int4 ei, Boolean focus, Boolean update)
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
			focusedparagraph->setselectionindex(MAXUINT2, MAXUINT2, False, False);
	}
	if (focus && !(state & CS_KFOCUSED))
	{
		getstack()->kfocusset(this);
		if (!(state & CS_KFOCUSED))
			return ES_NORMAL;
	}
	else
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
		sethilitedlines(MCnullmcstring);
		drect = rect;
	}
	uint2 l;
	do
	{
		l = pgptr->gettextsizecr();
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

void MCField::hilitedlines(MCExecPoint &ep)
{
	ep.clear();
	if (!opened || !(flags & F_LIST_BEHAVIOR))
		return;
	uint4 line = 0;
	MCParagraph *pgptr = paragraphs;
	bool first = true;
	do
	{
		line++;
		if (pgptr->gethilite())
		{
			ep.concatuint(line, EC_COMMA, first);
			first = false;
		}
		pgptr = pgptr->next();
	}
	while (pgptr != paragraphs);
}

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

void MCField::locchar(MCExecPoint &ep, Boolean click)
{
	int4 si, ei;
	if (locmark(False, False, click, True, True, si, ei))
		returntext(ep, si, ei);
	else
		ep.clear();
}
void MCField::loccharchunk(MCExecPoint &ep, Boolean click)
{
	int4 si, ei;
	if (locmark(False, False, click, True, True, si, ei))
		returnchunk(ep, si, ei);
	else
		ep.clear();
}

void MCField::locchunk(MCExecPoint &ep, Boolean click)
{
	int4 si, ei;
	if (locmark(False, True, click, True, True, si, ei))
		returnchunk(ep, si, ei);
	else
		ep.clear();
}

void MCField::locline(MCExecPoint &ep, Boolean click)
{
	int4 si, ei;
	if (locmark(True, True, click, True, True, si, ei))
		returnline(ep, si, ei);
	else
		ep.clear();
}

void MCField::loctext(MCExecPoint &ep, Boolean click)
{
	int4 si, ei;
	if (locmark(False, True, click, True, True, si, ei))
		returntext(ep, si, ei);
	else
		ep.clear();
}

Boolean MCField::locmark(Boolean wholeline, Boolean wholeword,
                         Boolean click, Boolean chunk, Boolean inc_cr, int4 &si, int4 &ei)
{
	MCRectangle frect = getfrect();
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

	// MW-2012-01-25: [[ FieldMetrics ]] Convert them to field coords.
	cx -= getcontentx();
	cy -= getcontenty() + cury;

	MCParagraph *pgptr = paragraphs;
	si = 0;
	while (pgptr != curparagraph)
	{
		si += pgptr->gettextsizecr();
		pgptr = pgptr->next();
	}
	int4 y = 0;
	uint2 pgheight = pgptr->getheight(fixedheight);
	while (pgptr->next() != paragraphs && y + pgheight <= cy)
	{
		y += pgheight;
		si += pgptr->gettextsizecr();
		pgptr = pgptr->next();
		pgheight = pgptr->getheight(fixedheight);
	}
	cy -= y;
	if (chunk && cy > pgptr->getheight(fixedheight))
		return False;
	if (wholeline || wholeword && flags & F_LIST_BEHAVIOR)
	{
		ei = si + pgptr->gettextsizecr();
		if (!inc_cr || flags & F_LIST_BEHAVIOR || pgptr->next() == paragraphs)
			ei--;
	}
	else
	{
		uint2 ssi, sei;
		ei = si;
		pgptr->getclickindex(cx, cy, fixedheight, ssi, sei, wholeword, chunk);
		si += ssi;
		ei += sei;
	}
	if (ei <= si && !wholeline)
		return False;
	return True;
}

void MCField::foundchunk(MCExecPoint &ep)
{
	int4 si, ei;
	if (foundmark(False, True, si, ei))
		returnchunk(ep, si, ei);
	else
		ep.clear();
}

void MCField::foundline(MCExecPoint &ep)
{
	int4 si, ei;
	if (foundmark(False, True, si, ei))
		returnline(ep, si, ei);
	else
		ep.clear();
}

void MCField::foundloc(MCExecPoint &ep)
{
	int4 si, ei;
	if (foundmark(False, True, si, ei))
		returnloc(ep, si);
	else
		ep.clear();
}

void MCField::foundtext(MCExecPoint &ep)
{
	int4 si, ei;
	if (foundmark(False, True, si, ei))
		returntext(ep, si, ei);
	else
		ep.clear();
}

Boolean MCField::foundmark(Boolean wholeline, Boolean inc_cr, int4 &si, int4 &ei)
{
	if (foundlength == 0)
		return False;
	if (wholeline)
	{
		MCParagraph *pgptr = paragraphs;
		si = 0;
		while (si + pgptr->gettextsizecr() <= foundoffset)
		{
			si += pgptr->gettextsizecr();
			pgptr = pgptr->next();
		}
		ei = si + pgptr->gettextsizecr();
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

void MCField::selectedchunk(MCExecPoint &ep)
{
	int4 si, ei;
	if (selectedmark(False, si, ei, False, False))
		returnchunk(ep, si, ei);
	else
		ep.clear();
}

void MCField::selectedline(MCExecPoint &ep)
{
	int4 si, ei;
	if (selectedmark(False, si, ei, False, False))
		returnline(ep, si, ei);
	else
		ep.clear();
}

void MCField::selectedloc(MCExecPoint &ep)
{
	int4 si, ei;
	if (selectedmark(False, si, ei, False, False))
		returnloc(ep, si);
	else
		ep.clear();
}

void MCField::selectedtext(MCExecPoint &ep)
{
	ep.clear();
	if (paragraphs == NULL) // never been opened
		return;
	if (flags & F_LIST_BEHAVIOR)
	{
		bool first = true;
		MCParagraph *pgptr = paragraphs;
		do
		{
			if (pgptr->gethilite())
			{
				ep.concatchars(pgptr->gettext(), pgptr->gettextsize(), EC_RETURN, first);
				first = false;
			}
			pgptr = pgptr->next();
		}
		while (pgptr != paragraphs);
	}
	else
	{
		int4 si, ei;
		if (selectedmark(False, si, ei, False, False))
			returntext(ep, si, ei);
	}
}

Boolean MCField::selectedmark(Boolean whole, int4 &si, int4 &ei,
                              Boolean force, Boolean include_cr)
{
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
			si += pgptr->gettextsizecr();
			pgptr = pgptr->next();
			if (pgptr == paragraphs)
				return False;
		}
		ei = si;
		ei += pgptr->gettextsizecr();
		pgptr = pgptr->next();
		while (pgptr != paragraphs && pgptr->gethilite())
		{
			ei += pgptr->gettextsizecr();
			pgptr = pgptr->next();
		}
		ei--;
	}
	else
	{
		uint2 s, e;
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
				si += pgptr->gettextsizecr();
				pgptr = pgptr->next();
			}
			if (whole)
				ei = si + focusedparagraph->gettextsizecr();
			else
			{
				focusedparagraph->getselectionindex(s, e);
				ei = si += s;
			}
		}
		else
		{
			while (pgptr != firstparagraph)
			{
				si += pgptr->gettextsizecr();
				pgptr = pgptr->next();
			}
			ei = si;
			if (!whole)
			{
				pgptr->getselectionindex(s, e);
				si += s;
			}
			while (pgptr != lastparagraph)
			{
				ei += pgptr->gettextsizecr();
				pgptr = pgptr->next();
			}
			if (whole)
			{
				ei += pgptr->gettextsizecr();
				if (pgptr->next() == paragraphs)
					ei--;
			}
			else
			{
				pgptr->getselectionindex(s, e);
				ei += e;
			}
		}
		if (include_cr && pgptr != NULL && e == pgptr->gettextsize() && pgptr->next() != paragraphs)
			ei++;
	}
	return True;
}

void MCField::returnchunk(MCExecPoint &ep, int4 si, int4 ei)
{
	getprop(0, P_NUMBER, ep, False);
	ep.ton();
	uint2 number = ep.getuint2();
	
	// MW-2012-02-23: [[ CharChunk ]] Map the internal field indices (si, ei) to
	//   char indices.
	unresolvechars(0, si, ei);
	
	const char *sptr = parent->gettype() == CT_CARD && getstack()->hcaddress()
										 ? "char %d to %d of card field %d" : "char %d to %d of field %d";
	ep.setstringf(sptr, si + 1, ei, number);
}

void MCField::returnline(MCExecPoint &ep, int4 si, int4 ei)
{
	getprop(0, P_NUMBER, ep, False);
	ep.ton();
	uint2 number = ep.getuint2();
	uint4 line = 0;
	int4 offset = 0;
	MCParagraph *pgptr = paragraphs;
	do
	{
		line++;
		offset += pgptr->gettextsizecr();
		pgptr = pgptr->next();
	}
	while (offset <= si);
	if (offset >= ei)
	{
		const char *sptr = parent->gettype() == CT_CARD && getstack()->hcaddress()
											 ? "line %d of card field %d" : "line %d of field %d";
		ep.setstringf(sptr, line, number);
	}
	else
	{
		uint4 endline = line;
		do
		{
			endline++;
			offset += pgptr->gettextsizecr();
			pgptr = pgptr->next();
		}
		while (offset < ei);
		const char *sptr = parent->gettype() == CT_CARD && getstack()->hcaddress()
											 ? "line %d to %d of card field %d" : "line %d to %d of field %d";
		ep.setstringf(sptr, line, endline, number);
	}
}

//unicode fine
void MCField::returnloc(MCExecPoint &ep, int4 si)
{
	// MW-2006-04-26: If the field is not opened we cannot call indextoloc so must return empty.
	if (!opened)
	{
		ep . clear();
		return;
	}

	int4 ei = si;
	MCParagraph *pgptr = indextoparagraph(paragraphs, si, ei);
	int2 x, y;
	pgptr->indextoloc(si, fixedheight, x, y);

	// MW-2012-01-25: [[ FieldMetrics ]] The x, y are in paragraph coords, so
	//   translate to field, then card.
	ep.setpoint(x + getcontentx(), y + paragraphtoy(pgptr) + getcontenty());
}

void MCField::returntext(MCExecPoint &ep, int4 si, int4 ei)
{
	// MW-2012-02-21: [[ FieldExport ]] Use the new text export method.
	exportastext(0, ep, si, ei, false);
}

void MCField::charstoparagraphs(int4 si, int4 ei, MCParagraph*& rsp, MCParagraph*& rep, uint4& rsl, uint4& rel)
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
		offset += pgptr->gettextsizecr();
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
			offset += pgptr->gettextsizecr();
			pgptr = pgptr->next();
		}
		while (offset < ei);
	}

	rsp = sp;
	rep = ep;

	rsl = sl;
	rel = el;
}

void MCField::linestoparagraphs(int4 si, int4 ei, MCParagraph*& rsp, MCParagraph*& rep)
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

MCSharedString *MCField::pickleselection(void)
{
	// MW-2012-02-17: [[ SplitTextAttrs ]] When pickling in the field, make the
	//   styledtext's parent the field so that font attr inheritence works.
	MCStyledText t_styled_text;
	t_styled_text . setparent(this);
	t_styled_text . setparagraphs(cloneselection());
	return MCObject::pickle(&t_styled_text, 0);
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
	recompute();
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
	if (!focusedparagraph->isselection() && firstparagraph == lastparagraph)
		return;

	MCSharedString *t_data;
	t_data = pickleselection();
	if (t_data != NULL)
	{
		MCclipboarddata -> Store(TRANSFER_TYPE_STYLED_TEXT, t_data);
		t_data -> Release();
	}
}

void MCField::cuttextindex(uint4 parid, int4 si, int4 ei)
{
	if (!opened || getcard()->getid() != parid)
		return;
	seltext(si, ei, False);
	cuttext();
}

void MCField::copytextindex(uint4 parid, int4 si, int4 ei)
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
			int4 si, ei;
			selectedmark(False, si, ei, False, False);
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
			us->ud.text.newchars += tpgptr->gettextsizecr();
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

void MCField::movetext(MCParagraph *newtext, int4 p_to_index)
{
	if ((flags & F_LOCK_TEXT) == 0 && !getstack()->islocked() && opened)
	{
		int4 si, ei;
		selectedmark(False, si, ei, False, False);
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
			us->ud.text.newchars += tpgptr->gettextsizecr();
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

void MCField::deletetext(int4 si, int4 ei)
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
	settextindex(0, si, ei, MCnullmcstring, True);
}

MCParagraph *MCField::clonetext(int4 si, int4 ei)
{
	MCParagraph *t_paragraph;
	t_paragraph = verifyindices(paragraphs, si, ei);

	MCParagraph *t_new_paragraphs;
	t_new_paragraphs = NULL;

	uint2 l;
	do
	{
		l = t_paragraph -> gettextsizecr();
		
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
	uint2 si = focusedparagraph->gettextsize();
	focusedparagraph->join();
	firstparagraph = lastparagraph = NULL;
	flags = oldflags;
	updateparagraph(True, True);
	int2 x, y;
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

