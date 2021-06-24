/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "dllst.h"

#include <SkPaint.h>
#include <SkTypeface.h>


#include "skiatypeface.h"

#include "em-util.h"
#include "em-font.h"
#include "em-fontlist.h"

class MCFontnode : public MCDLlist
{
public:
	MCFontnode(MCNameRef p_name, uint16_t p_size, uint16_t p_style);
	virtual ~MCFontnode();

	virtual MCFontStruct *getfont(MCNameRef p_name,
	                              uint16_t p_size,
	                              uint16_t p_style);

	virtual MCFontStruct *getfontstruct() { return &m_font_info; }

	virtual MCNameRef getname() { return *m_requested_name;  }
	virtual uint16_t getsize()  { return m_requested_size;  }
	virtual uint16_t getstyle() { return m_requested_style; }

	virtual MCFontnode *next();
	virtual void appendto(MCFontnode *& list);
	virtual MCFontnode *remove(MCFontnode *& list);

protected:
	MCNewAutoNameRef m_requested_name;
	uint16_t m_requested_size;
	uint16_t m_requested_style;
	MCFontStruct m_font_info;
};


/* ================================================================
 * MCFontnode implementation for Emscripten
 * ================================================================ */

MCFontnode::MCFontnode(MCNameRef p_name,
                       uint16_t p_size,
                       uint16_t p_style)
	: m_requested_name(p_name),
	  m_requested_size(p_size),
	  m_requested_style(p_style)
{
	MCAutoStringRef t_name;
	t_name = MCNameGetString(p_name);

	uindex_t t_comma;
    MCAutoStringRef t_before_comma;
    if (MCStringFirstIndexOfChar(*t_name, ',', 0, kMCCompareExact, t_comma))
    {
        /* UNCHECKED */ MCStringCopySubstring(*t_name, MCRangeMake(0, t_comma - 1), &t_before_comma);
    }
    else
    {
        t_before_comma = *t_name;
    }
    
    m_font_info.fid = (MCSysFontHandle)MCEmscriptenFontCreate(*t_before_comma, p_size, (p_style & FA_WEIGHT) > 0x05, (p_style & FA_ITALIC) != 0);

 	MCEmscriptenFontGetMetrics(m_font_info.fid,  m_font_info.m_ascent, m_font_info.m_descent, m_font_info.m_leading, m_font_info.m_xheight);
}

MCFontnode::~MCFontnode()
{
	MCEmscriptenFontDestroy(m_font_info.fid);
}

MCFontStruct *
MCFontnode::getfont(MCNameRef p_name,
                    uint16_t p_size,
                    uint16_t p_style)
{
	if (!MCNameIsEqualToCaseless(p_name, *m_requested_name))
	{
		return NULL;
	}

	if ((p_size != 0 && p_size != m_requested_size) ||
	    p_style != m_requested_style)
	{
		return NULL;
	}

	return &m_font_info;
}

/* ----------------------------------------------------------------
 * MCDLlist stuff
 * ---------------------------------------------------------------- */

MCFontnode *
MCFontnode::next()
{
	return static_cast<MCFontnode *>(MCDLlist::next());
}

void
MCFontnode::appendto(MCFontnode *& list)
{
	MCDLlist::appendto(reinterpret_cast<MCDLlist *&>(list));
}

MCFontnode *
MCFontnode::remove(MCFontnode *& list)
{
	return static_cast<MCFontnode *>(MCDLlist::remove(reinterpret_cast<MCDLlist *&>(list)));
}

/* ================================================================
 * MCFontlist implementation for Emscripten
 * ================================================================ */

MCFontlist::MCFontlist() : m_font_list(nil)
{
}

MCFontlist::~MCFontlist()
{
	while (m_font_list != NULL)
	{
		MCFontnode *fptr = m_font_list->remove(m_font_list);
		delete fptr;
	}
}

MCFontStruct *
MCFontlist::getfont(MCNameRef p_name,
                    uint16_t p_size,
                    uint16_t p_style,
                    Boolean p_for_printer) /* ignored */
{
	MCFontnode *t_node = m_font_list;

	/* Check if we already know about the requested font */
	if (t_node != NULL)
	{
		do
		{
			MCFontStruct *t_font = t_node->getfont(p_name, p_size, p_style);

			if (t_font != NULL)
				return t_font;

			t_node = t_node->next();
		}
		while (t_node != m_font_list);
	}

	/* No match; create a new font with the requested parameters */
	t_node = new (nothrow) MCFontnode(p_name, p_size, p_style);

	t_node->appendto(m_font_list);

	return t_node->getfont(p_name, p_size, p_style);
}

bool
MCFontlist::getfontnames(MCStringRef p_type,
                         MCListRef & r_names)
{
	return MCEmscriptenListFontFamilies(r_names);
}

/* All fonts are scalable on Emscripten, so return 0 */
bool
MCFontlist::getfontsizes(MCStringRef p_fname,
                         MCListRef & r_sizes)
{
	MCAutoListRef t_list;
	if (MCListCreateMutable('\n', &t_list))
		return false;

	if (!MCListAppendInteger(*t_list, 0))
		return false;

	return MCListCopy(*t_list, r_sizes);
}

bool
MCFontlist::getfontstyles(MCStringRef p_name,
                          uint16_t p_size,
                          MCListRef & r_styles)
{
	return MCEmscriptenListFontsForFamily(p_name, p_size, r_styles);
}

bool
MCFontlist::getfontstructinfo(MCNameRef & r_name,
                              uint16_t & r_size,
                              uint16_t & r_style,
                              Boolean & r_for_printer,
                              MCFontStruct *p_font)
{
	/* Iterate over the list of fonts, looking for the correct font
	 * structure */
	MCFontnode *t_node = m_font_list;
	if (t_node != NULL)
	{
		do
		{
			if (t_node->getfontstruct() != p_font)
			{
				t_node = t_node->next();
				continue;
			}

			/* Found a match */
			r_name = t_node->getname();
			r_size = t_node->getsize();
			r_style = t_node->getstyle();

			/* Don't support printer fonts on Emscripten */
			r_for_printer = false;

			return true;
		}
		while (t_node != m_font_list);
	}
	return false;
}
