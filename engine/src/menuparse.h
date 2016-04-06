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
	MCStringRef label;
	bool is_disabled: 1;
	bool is_radio: 1;
	bool is_hilited: 1;
    // SN-2014-07-29: [[ Bug 12998 ]] has_tag member put back
    bool has_tag: 1;
	uint4 accelerator;
	MCStringRef accelerator_name;
	uint1 modifiers;
	uint4 mnemonic;
	MCStringRef tag;
	uint1 menumode;
	
	MCMenuItem()
	{
		depth = 0;
		label = MCValueRetain(kMCEmptyString);
		is_disabled = 0;
		is_radio = 0;
		is_hilited = 0;
		accelerator = 0;
		accelerator_name = MCValueRetain(kMCEmptyString);
		modifiers = 0;
		mnemonic = 0;
		tag = MCValueRetain(kMCEmptyString);
        // SN-2014-07-29: [[ Bug 12998 ]] has_tag member put back
        has_tag = false;
		menumode = 0;
	}
	
	~MCMenuItem()
	{
		MCValueRelease(label);
		MCValueRelease(accelerator_name);
		MCValueRelease(tag);
	}
	
	void assignFrom(MCMenuItem *p_from);
};

class IParseMenuCallback
{
public:
	virtual bool Start() {return false;}
	virtual bool ProcessItem(MCMenuItem *p_menuitem) = 0;
	virtual bool End(bool p_has_tags) {return false;}
};

extern void MCParseMenuString(MCStringRef p_string, IParseMenuCallback *p_callback, uint1 p_menumode);
extern uint4 MCLookupAcceleratorKeysym(MCStringRef p_name);
extern const char *MCLookupAcceleratorName(uint4 p_keysym);

#endif
