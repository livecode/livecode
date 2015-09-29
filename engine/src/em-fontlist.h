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

#ifndef __MC_EMSCRIPTEN_FONTLIST_H__
#define __MC_EMSCRIPTEN_FONTLIST_H__

#include <foundation.h>

#include "sysdefs.h"

class MCFontnode;

class MCFontlist
{
public:
	MCFontlist();
	~MCFontlist();
	MCFontStruct *getfont(MCNameRef p_name,
	                      uint16_t p_size,
	                      uint16_t p_style,
	                      Boolean p_for_printer);
	bool getfontnames(MCStringRef p_type, MCListRef & r_names);
	bool getfontsizes(MCStringRef p_fname, MCListRef & r_sizes);
	bool getfontstyles(MCStringRef p_fname,
	                   uint16_t p_size,
	                   MCListRef & r_styles);
	bool getfontstructinfo(MCNameRef & r_name,
	                       uint16_t & r_size,
	                       uint16_t & r_style,
	                       Boolean & r_for_printer,
	                       MCFontStruct * p_font);

protected:
	MCFontnode *m_font_list;
};

#endif /* ! __MC_EMSCRIPTEN_FONTLIST_H__ */
