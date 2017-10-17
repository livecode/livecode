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

#include <SkPaint.h>
#include <SkTypeface.h>

#include "em-fontlist.h"
#include "em-util.h"

#include "objdefs.h"
#include "dllst.h"

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


// Forward declarations
static sk_sp<SkTypeface> emscripten_get_font_by_name(MCNameRef p_name, uint16_t p_style);
static sk_sp<SkTypeface> emscripten_get_font_from_file(MCStringRef p_file);


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
    // Load the font as requested
    auto t_typeface = emscripten_get_font_by_name(p_name, p_style);

    // Calculate the metrics for this typeface and size
    SkPaint t_paint;
    t_paint.setTypeface(t_typeface);
    t_paint.setTextSize(p_size);
        
    SkPaint::FontMetrics t_metrics;
        
    t_paint.getFontMetrics(&t_metrics);
        
    // SkPaint::FontMetrics gives the ascent value as a negative offset from the baseline, where we expect the (positive) distance.
    m_font_info.m_ascent = -t_metrics.fAscent;
    m_font_info.m_descent = t_metrics.fDescent;
    m_font_info.m_leading = t_metrics.fLeading;
    m_font_info.m_xheight = t_metrics.fXHeight;
    m_font_info.fid = reinterpret_cast<MCSysFontHandle>(t_typeface.release());
    m_font_info.size = p_size;
}

MCFontnode::~MCFontnode()
{
	SkTypeface *t_typeface = reinterpret_cast<SkTypeface *>(m_font_info.fid);
	t_typeface->unref();
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
	MCEmscriptenNotImplemented();
	return false;
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
	MCEmscriptenNotImplemented();
	return false;
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


sk_sp<SkTypeface> emscripten_get_font_by_name(MCNameRef p_name,
                                              uint16_t p_style)
{
	/* Decode style */
	bool t_italic = (0 != (p_style & FA_ITALIC));
	bool t_bold = (0x05 < (p_style & FA_WEIGHT));

    SkFontStyle::Weight t_weight;
    SkFontStyle::Width t_width;
    SkFontStyle::Slant t_slant;
    
	if (t_bold)
        t_weight = SkFontStyle::kBold_Weight;
    else
        t_weight = SkFontStyle::kNormal_Weight;
    
    t_width = SkFontStyle::kNormal_Width;
    
    if (t_italic)
        t_slant = SkFontStyle::kItalic_Slant;
    else
        t_slant = SkFontStyle::kUpright_Slant;
	
    SkFontStyle t_style(t_weight, t_width, t_slant);
    
    MCAutoStringRefAsSysString t_sys_name;
	/* UNCHECKED */ t_sys_name.Lock(MCNameGetString(p_name));

	sk_sp<SkTypeface> t_typeface = SkTypeface::MakeFromName(*t_sys_name, t_style);
	
	return t_typeface;       
}

sk_sp<SkTypeface> emscripten_get_font_from_file(MCStringRef p_file)
{
    MCAutoStringRefAsSysString t_sys_path;
    /* UNCHECKED */ t_sys_path.Lock(p_file);

    sk_sp<SkTypeface> t_typeface = SkTypeface::MakeFromFile(*t_sys_path);
    MCAssert(t_typeface != nil);
    return t_typeface;
}

