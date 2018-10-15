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

#include "globals.h"
#include "dispatch.h"
#include "flst.h"
#include "uidc.h"
#include "util.h"
#include "font.h"
#include "osspec.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

bool MCTextBaseFontName(MCStringRef p_font, MCStringRef& r_base_name)
{
	uindex_t t_index;
	if (MCStringFirstIndexOfChar(p_font, ',', 0, kMCStringOptionCompareExact, t_index))
		return MCStringCopySubstring(p_font, MCRangeMake(0, t_index), r_base_name);
	else
		return MCStringCopy(p_font, r_base_name);
}

////////////////////////////////////////////////////////////////////////////////

void MCTextEvalFontNames(MCExecContext& ctxt, MCStringRef p_type, MCStringRef& r_names)
{
	MCAutoListRef t_name_list;
    MCAutoStringRef t_names;
	if (MCdispatcher->getfontlist()->getfontnames(p_type, &t_name_list) &&
		MCListCopyAsString(*t_name_list, &t_names))
    {
        // Prepend the special UI font names
        if (MCStringFormat(r_names, "%@\n%@\n%@\n%@\n%@\n%@\n%@\n%@",
                           MCN_font_default, MCN_font_usertext, MCN_font_menutext,
                           MCN_font_content, MCN_font_message, MCN_font_tooltip,
                           MCN_font_system, *t_names))
        {
            return;
        }
    }

	ctxt . Throw();
}

//////////

void MCTextEvalFontLanguage(MCExecContext& ctxt, MCStringRef p_font, MCNameRef& r_lang)
{
	MCAutoStringRef t_fontname;
	if (!MCTextBaseFontName(p_font, &t_fontname))
	{
		ctxt . Throw();
		return;
	}
	uint1 charset = MCscreen->fontnametocharset(*t_fontname);
	r_lang = MCU_charsettolanguage(charset);
	MCValueRetain(r_lang);
}

//////////

void MCTextEvalFontSizes(MCExecContext& ctxt, MCStringRef p_font, MCStringRef& r_sizes)
{
	MCAutoStringRef t_fontname;
	MCAutoListRef t_size_list;
	if (MCTextBaseFontName(p_font, &t_fontname) &&
		MCdispatcher->getfontlist()->getfontsizes(*t_fontname, &t_size_list) &&
		MCListCopyAsString(*t_size_list, r_sizes))
		return;

	ctxt . Throw();
}

//////////

void MCTextEvalFontStyles(MCExecContext& ctxt, MCStringRef p_font, integer_t p_size, MCStringRef& r_styles)
{
	MCAutoStringRef t_fontname;
	MCAutoListRef t_style_list;
	if (MCTextBaseFontName(p_font, &t_fontname) &&
		MCdispatcher->getfontlist()->getfontstyles(*t_fontname, p_size, &t_style_list) &&
		MCListCopyAsString(*t_style_list, r_styles))
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCTextEvalMeasureText(MCExecContext& ctxt, MCObject *p_obj, MCStringRef p_text, MCStringRef p_mode, bool p_unicode, MCStringRef& r_result)
{
    MCRectangle t_bounds = p_obj -> measuretext(p_text, p_unicode);
    
    bool t_success;
    t_success = false;
    
    // AL-2014-10-27: [[ Bug 13809 ]] Can specify "width" to get width, as well as leaving parameter empty
    if (p_mode == nil || MCStringIsEqualTo(p_mode, MCSTR("width"), kMCStringOptionCompareCaseless))
        t_success = MCStringFormat(r_result, "%d", t_bounds . width);
    else if (MCStringIsEqualTo(p_mode, MCSTR("size"), kMCStringOptionCompareCaseless))
        t_success = MCStringFormat(r_result, "%d,%d", t_bounds . width, t_bounds . height);
    else if (MCStringIsEqualTo(p_mode, MCSTR("bounds"), kMCStringOptionCompareCaseless))
        t_success = MCStringFormat(r_result, "%d,%d,%d,%d", t_bounds . x, t_bounds . y, t_bounds . x + t_bounds . width, t_bounds . y + t_bounds . height);

    if (t_success)
        return;
    
    ctxt . Throw();
}

void MCTextGetFontfilesInUse(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_list)
{
    MCFontListLoaded(r_count, r_list);
}

void MCTextExecStartUsingFont(MCExecContext& ctxt, MCStringRef p_path, bool p_is_globally)
{
    // MERG-2013-08-14: [[ DynamicFonts ]] Refactored to use MCFontLoad
    MCAutoStringRef t_resolved_path;
    /* UNCHECKED */ MCS_resolvepath(p_path, &t_resolved_path);
    if (!MCFontLoad(*t_resolved_path , p_is_globally))
        ctxt . SetTheResultToStaticCString("can't load font file");
    else
        ctxt . SetTheResultToEmpty();
}
void MCTextExecStopUsingFont(MCExecContext& ctxt, MCStringRef p_path)
{
    // MERG-2013-08-14: [[ DynamicFonts ]] Refactored to use MCFontUnload
    MCAutoStringRef t_resolved_path;
    /* UNCHECKED */ MCS_resolvepath(p_path, &t_resolved_path);
    if (!MCFontUnload(*t_resolved_path))
        ctxt . SetTheResultToStaticCString("can't unload font file");
    else
        ctxt . SetTheResultToEmpty();
}

////////////////////////////////////////////////////////////////////////////////
