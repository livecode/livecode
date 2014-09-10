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
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"
#include "mcio.h"

#include "object.h"
#include "cdata.h"
#include "field.h"
#include "paragraf.h"

MCCdata::MCCdata()
{
	id = 0;
	data = NULL;
}

MCCdata::MCCdata(const MCCdata &cref) : MCDLlist(cref)
{
	id = cref.id;
	if (cref.data != NULL && cref.data != (void *)1)
	{
		if (id & COMPACT_PARAGRAPHS)
			data = strclone((char *)cref.data);
		else
		{
			MCParagraph *paragraphs = NULL;
			MCParagraph *tptr = (MCParagraph *)cref.data;
			do
			{
				MCParagraph *newparagraph = new MCParagraph(*tptr);
				newparagraph->appendto(paragraphs);
				tptr = (MCParagraph *)tptr->next();
			}
			while (tptr != cref.data);
			data = paragraphs;
		}
	}
	else
		data = cref.data;
}

MCCdata::MCCdata(uint4 newid)
{
	id = newid;
	data = NULL;
}

MCCdata::~MCCdata()
{
	if (data != NULL && data != (void *)1)
	{
		if (id & COMPACT_PARAGRAPHS)
			delete (char *)data;
		else
		{
			MCParagraph *paragraphs = (MCParagraph *)data;
			while (paragraphs != NULL)
			{
				MCParagraph *pptr = (MCParagraph *)paragraphs->remove
				                    (paragraphs);
				delete pptr;
			}
		}
	}
}

IO_stat MCCdata::load(IO_handle stream, MCObject *parent, uint32_t version)
{
	IO_stat stat;

	if ((stat = IO_read_uint4(&id, stream)) != IO_NORMAL)
		return stat;
	if (parent->gettype() == CT_BUTTON)
	{
		uint1 set;
		stat = IO_read_uint1(&set, stream);
		data = (void *)(set ? 1 : 0);
		return stat;
	}
	else
	{
		if (id & COMPACT_PARAGRAPHS)
		{
			// MW-2013-11-19: [[ UnicodeFileFormat ]] This flag is never set by newer engines
			//   so is just legacy.
			char *string;
			if ((stat = IO_read_cstring_legacy(string, stream, sizeof(uint1))) != IO_NORMAL)
				return stat;
			data = string;
		}
		else
		{
			MCParagraph *paragraphs = NULL;
			while (True)
			{
				uint1 type;
				if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
					return stat;
				switch (type)
				{
				// MW-2012-03-04: [[ StackFile5500 ]] Handle either the paragraph or extended
				//   paragraph tag.
				case OT_PARAGRAPH:
				case OT_PARAGRAPH_EXT:
					{
						MCParagraph *newpar = new MCParagraph;
						newpar->setparent((MCField *)parent);
						
						// MW-2012-03-04: [[ StackFile5500 ]] If the paragraph tab was the extended
						//   variant, then pass the correct is_ext parameter.
						if ((stat = newpar->load(stream, version, type == OT_PARAGRAPH_EXT)) != IO_NORMAL)
						{
							delete newpar;
							return stat;
						}
						newpar->appendto(paragraphs);
					}
					break;
				default:
					data = paragraphs;
					MCS_seek_cur(stream, -1);
					return IO_NORMAL;
				}
			}
		}
	}
	return IO_NORMAL;
}

IO_stat MCCdata::save(IO_handle stream, Object_type type, uint4 p_part)
{
	IO_stat stat;

	// If p_part is non-zero it means we only want to save data specific
	// to a given card. In this case, we simply don't save the rest.
	if (p_part != 0 && id != p_part)
		return IO_NORMAL;

	if ((stat = IO_write_uint1(type, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint4(p_part != 0 ? 0 : id, stream)) != IO_NORMAL)
		return stat;
	if (type == OT_BDATA)
	{
		uint1 set = data ? 1 : 0;
		return IO_write_uint1(set, stream);
	}
	else
		if (id & COMPACT_PARAGRAPHS)
		{
			// MW-2013-11-19: [[ UnicodeFileFormat ]] This flag is never set by newer engines
			//   so is just legacy. (Indeed, this codepath should never be hit!).
			return IO_write_cstring_legacy((char *)data, stream, sizeof(uint1));
		}
		else
		{
			MCParagraph *tptr = (MCParagraph *)data;
			if (tptr != NULL)
				do
				{
					if ((stat = tptr->save(stream, p_part)) != IO_NORMAL)
						return stat;
					tptr = (MCParagraph *)tptr->next();
				}
				while (tptr != data);
		}
	return IO_NORMAL;
}

uint4 MCCdata::getid()
{
	return (id & ~COMPACT_PARAGRAPHS);
}

void MCCdata::setid(uint4 newid)
{
	id = newid | (id & COMPACT_PARAGRAPHS);
}

MCParagraph *MCCdata::getparagraphs()
{
	MCParagraph *paragraphs;
	if (id & COMPACT_PARAGRAPHS)
	{
		paragraphs = NULL;
		char *eptr = (char *)data;
		while ((eptr = strtok(eptr, "\n")) != NULL)
		{
			MCParagraph *tpgptr = new MCParagraph;
			tpgptr->appendto(paragraphs);
			uint2 l = strlen(eptr) + 1;
			char *sptr = new char[l];
			memcpy(sptr, eptr, l);
			MCAutoStringRef t_string;
			/* UNCHECKED */ MCStringCreateWithNativeChars((const char_t*)sptr, l, &t_string);
			tpgptr->settext(*t_string);
			eptr = NULL;
		}
		delete (char *)data;
		data = paragraphs;
		id &= ~COMPACT_PARAGRAPHS;
	}
	if (data == NULL)
		data = paragraphs = new MCParagraph;
	else
		paragraphs = (MCParagraph *)data;
	return paragraphs;
}

void MCCdata::setparagraphs(MCParagraph *&newpar)
{
	id &= ~COMPACT_PARAGRAPHS;
	data = newpar;
}

Boolean MCCdata::getset()
{
	return data != NULL;
}

void MCCdata::setset(Boolean newset)
{
	data = (void *)(newset != 0);
}
