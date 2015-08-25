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

#ifndef MENUITEM_H
#define MENUITEM_H


struct MCMenuItem
{
	int4 depth;
	MCString label;
	bool is_unicode;
	bool is_disabled;
	bool is_radio;
	bool is_hilited;
	uint4 accelerator;
	const char *accelerator_name;
	uint1 modifiers;
	uint4 mnemonic;
	MCString tag;
    bool has_tag;
	uint1 menumode;
	
	void assignFrom(MCMenuItem *p_from)
	{
		memcpy(this, p_from, sizeof(MCMenuItem));
	}
};

class IParseMenuCallback
{
public:
	virtual bool Start() {return false;}
	virtual bool ProcessItem(MCMenuItem *p_menuitem) = 0;
	virtual bool End(bool p_has_tags) {return false;}
};

extern bool MCParseMenuString(MCString &r_string, IParseMenuCallback *p_callback, bool isunicode, uint1 p_menumode);
extern uint4 MCLookupAcceleratorKeysym(MCString &p_name);
extern const char *MCLookupAcceleratorName(uint4 p_keysym);

#endif
