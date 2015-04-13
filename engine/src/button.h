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

#ifndef	BUTTON_H
#define	BUTTON_H

#include "control.h"


#define AQUA_FUDGE 8



#define needfocus() (MClook == LF_MOTIF && state & CS_KFOCUSED \
		     && (flags & F_SHOW_BORDER || !(flags & F_AUTO_ARM)) \
		     && !(extraflags & EF_NO_FOCUS_BORDER) \
                     && (getstyleint(flags) != F_MENU \
			 || menumode != WM_TOP_LEVEL))


#define standardbtn() ((IsMacEmulatedLF() || MCcurtheme!=NULL) && flags & F_3D \
		  && !(flags & F_AUTO_ARM) && flags & F_SHOW_BORDER \
	          && (IsMacEmulatedLF() && getstyleint(flags) == F_STANDARD|| \
						MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_PUSHBUTTON) && \
						getstyleint(flags) == F_STANDARD || MCcurtheme \
		      && MCcurtheme->iswidgetsupported(WTHEME_TYPE_BEVELBUTTON) && getstyleint(flags) == F_RECTANGLE) \
		  && !(flags & F_SHADOW) && flags & F_OPAQUE)

#define MENU_FLAGS (F_RECTANGLE | F_VISIBLE | F_SHOW_NAME \
		    | F_ALIGN_LEFT | F_3D | F_TRAVERSAL_ON \
		    | F_ARM_BORDER | F_OPAQUE | F_SHOW_HILITE)

#define MENU_ITEM_FLAGS (F_RECTANGLE | F_VISIBLE | F_SHOW_NAME \
			 | F_ALIGN_LEFT | F_3D | F_TRAVERSAL_ON \
			 | F_AUTO_ARM | F_ARM_BORDER | F_OPAQUE \
			 | F_SHARED_HILITE | F_SHOW_HILITE)

#define DIVIDER_FLAGS (F_RECTANGLE | F_VISIBLE | F_3D \
		       | F_SHOW_BORDER | F_DISABLED)


#define CHECK_SIZE 13
#define MOTIF_DEFAULT_WIDTH 4
#define WIN95_DEFAULT_WIDTH 1
#define MAX_CASCADE 5

#define THROB_RATE 80

#define MAX_SUBMENU_DEPTH 31
typedef struct
{
	MCButton *parent;
	MCButton *buttons;
	uint2 f;
	uint2 maxwidth;
	uint2 maxaccelwidth;
}
sublist;

enum Current_icon {
    CI_ARMED,
    CI_DISABLED,
    CI_HILITED,
    CI_DEFAULT,
    CI_VISITED,
		CI_FILE_NICONS,
		CI_HOVER = CI_FILE_NICONS,
    CI_NICONS
};

typedef struct
{
	MCImage *curicon;
	uint4 iconids[CI_NICONS];
}
iconlist;

#define MENUCONTROL_NONE 0
#define MENUCONTROL_ITEM 1
#define MENUCONTROL_SEPARATOR 2

class ButtonMenuCallback;

class MCButton : public MCControl
{
	friend class MCHcbutton;
	MCCdata *bdata;
	iconlist *icons;
	char *label;
	uint2 labelsize;
	uint2 menusize;
	char *menuname;
	char *menustring;
	MCField *entry;
	MCStack *menu;
	MCPlatformMenuRef m_system_menu;
	char *acceltext;
	uint2 acceltextsize;
	char *seltext;
	MCString *tabs;
	uint2 ntabs;
	uint2 menuhistory;
	uint2 menulines;
	uint2 accelkey;
	uint2 labelwidth;
	uint2 family;
	uint1 mymenudepth;
	uint1 menubutton;
	uint1 menumode;
	uint1 accelmods;
	uint1 mnemonic;
	uint1 menucontrol;
	// SN-2015-01-06: [[ Bug 14306 ]] The type of an enum is implementation-defined,
	// and forcing the size to 4 boils down to a 4-bit int, not a 4-byte int on Windows.
	// A 5-bit signed int is enough though to handle the 12 values of the MCGravity enum.
	MCGravity m_icon_gravity : 5;
	bool menuhasitemtags : 1;

	Boolean ishovering;
	static uint2 focusedtab;
	static uint2 mnemonicoffset;
	static MCRectangle optionrect;
	static Keynames button_keys[];
	static uint4 clicktime;
	static uint2 menubuttonheight;
	static Boolean starthilite;
	static uint2 starttab;
	static MCImage *macrb;
	static MCImage *macrbtrack;
	static MCImage *macrbhilite;
	static MCImage *macrbhilitetrack;
    
    // MM-2014-07-31: [[ ThreadedRendering ]] Used to ensure the default button animate message is only posted from a single thread.
    bool m_animate_posted : 1;

public:
	MCButton();
	MCButton(const MCButton &bref);
	// virtual functions from MCDLlist
	void removelink(MCObject *optr);
	bool imagechanged(MCImage *p_image, bool p_deleting);

	// virtual functions from MCObject
	virtual ~MCButton();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual bool visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor* p_visitor);

	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual Boolean kfocusnext(Boolean top);
	virtual Boolean kfocusprev(Boolean bottom);
	virtual void kunfocus();
	virtual Boolean kdown(const char *string, KeySym key);
	virtual Boolean kup(const char *string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
#ifdef _MACOSX
	virtual void timer(MCNameRef mptr, MCParameter *params);
#endif

	virtual uint2 gettransient() const;
	virtual void setrect(const MCRectangle &nrect);
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective, bool recursive = false);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual void closemenu(Boolean kfocus, Boolean disarm);
	
	// MW-2011-09-20: [[ Collision ]] Compute shape of button - will use mask of icon if possible.
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);

	// virtual functions from MCControl
	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	virtual IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);
	virtual IO_stat load(IO_handle stream, const char *version);
	virtual IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);

	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);
	virtual MCControl *findnum(Chunk_term type, uint2 &num);
	virtual MCControl *findname(Chunk_term type, const MCString &);
	virtual Boolean count(Chunk_term type, MCObject *stop, uint2 &num);
	virtual Boolean maskrect(const MCRectangle &srect);
	virtual void installaccels(MCStack *stack);
	virtual void removeaccels(MCStack *stack);
	virtual MCCdata *getdata(uint4 cardid, Boolean clone);
	virtual void replacedata(MCCdata *&data, uint4 newid);
	virtual void compactdata();
	virtual void resetfontindex(MCStack *oldstack);
	virtual void getwidgetthemeinfo(MCWidgetInfo &widgetinfo);
	
	// MCButton functions
	void activate(Boolean notify, uint2 key);
	void clearmnemonic();
	void setupmnemonic();
	MCCdata *getbptr(uint4 cardid);
	uint2 getfamily();
	Boolean gethilite(uint4 parid);
	void setdefault(Boolean def);
	Boolean sethilite(uint4 parid, Boolean hilite);
	void resethilite(uint4 parid, Boolean hilite);

	// MW-2011-09-30: [[ Redraw ]] This function conditionally does a redraw all
	//   if flags means it might need to be.
	void mayberedrawall(void);

	bool gethovering(void)
	{
		return ishovering == True;
	}

	int32_t getmenucontrol(void)
	{
		return menucontrol;
	}

	MCField *getentry()
	{
		return entry;
	}
	
	// MW-2012-02-16: [[ IntrinsicUnicode ]] 'unicode' parameter is true if 's' is
	//   UTF-16.
	void getlabeltext(MCString &s, bool& r_unicode);

	void getmenustring(MCString &s)
	{
		s.set(menustring, menusize);
	}
	uint1 getmenumode()
	{
		return menumode;
	}
	void setmenumode(uint1 newmode)
	{
		menumode = newmode;
	}
	bool getmenuhastags()
	{
		return menuhasitemtags;
	}
	void setmenuhasitemtags(bool p_hastags)
	{
		menuhasitemtags = p_hastags;
	}
	MCStack *getmenu()
	{
		return menu;
	}
	uint2 getaccelkey()
	{
		return accelkey;
	}
	uint1 getaccelmods()
	{
		return accelmods;
	}
	void getentrytext();
	void createentry();
	void deleteentry();
	void makemenu(sublist *bstack, int2 &stackdepth, uint2 menuflags, MCFontRef fontref);
	
	// MW-2011-02-08: [[ Bug 9384 ]] If 'just for accel' is true, then findmenu won't create
	//   a Mac menu as its assumed its not needed.
	Boolean findmenu(bool p_just_for_accel = false);
	
	void openmenu(Boolean grab);
	void freemenu(Boolean force);
	void docascade(MCString &pick);
	void getmenuptrs(const char *&sptr, const char *&eptr);
	void setupmenu();
	void selectedchunk(MCExecPoint &);
	void selectedline(MCExecPoint &);
	void selectedtext(MCExecPoint &);
	Boolean resetlabel();
	void reseticon();
	void radio();
	void setmenuhistory(int2 newline);
	void setmenuhistoryprop(int2 newline);
	uint2 getmousetab(int2 &curx);
	void allocicons();
	void freeicons();
	bool tabselectonmouseup();
	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);
	void drawlabel(MCDC *dc, int2 sx, int sy, uint2 t, const MCRectangle &srect, const MCString &lptr, bool isunicode, uint2 fstyle);
	void drawcheck(MCDC *dc, MCRectangle &srect, Boolean white);
	void drawradio(MCDC *dc, MCRectangle &srect, Boolean white);
	void drawoption(MCDC *dc, MCRectangle &srect, MCRectangle& r_content_rect);
	void drawpulldown(MCDC *dc, MCRectangle &srect);
	void drawcascade(MCDC *dc, MCRectangle &srect);
	void drawcombo(MCDC *dc, MCRectangle &srect);
	void drawtabs(MCDC *dc, MCRectangle &srect);
	void drawstandardbutton(MCDC *dc, MCRectangle &srect);
	void drawmacdefault(MCDC *dc, const MCRectangle &srect);
	void drawmacborder(MCDC *dc, MCRectangle &srect);
	void drawmacpopup(MCDC *dc, MCRectangle &srect);

#ifdef _MAC_DESKTOP
	Bool macfindmenu(bool p_just_for_accel);
	void macopenmenu(void);
	void macfreemenu(void);
	static void getmacmenuitemtextfromaccelerator(MCPlatformMenuRef menu, uint2 key, uint1 mods, MCString &s, bool isunicode, bool issubmenu);
#endif

	MCCdata *getcdata(void) {return bdata;}

	MCButton *next()
	{
		return (MCButton *)MCDLlist::next();
	}
	MCButton *prev()
	{
		return (MCButton *)MCDLlist::prev();
	}
	void totop(MCButton *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCButton *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCButton *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCButton *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCButton *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCButton *remove(MCButton *&list)
	{
		return (MCButton *)MCDLlist::remove((MCDLlist *&)list);
	}

private:
	int4 formattedtabwidth(void);

	// MW-2012-02-16: [[ IntrinsicUnicode]] Change the menustring, label and acceltext to unicode.
	void switchunicode(bool p_to_unicode);
	// MW-2012-02-16: [[ IntrinsicUnicode]] Attempt to change everything back to native if possible.
	void trytochangetonative(void);

	friend class ButtonMenuCallback;
    
protected:
    
    // FG-2014-11-11: [[ Better theming ]] Fetch the control type/state for theming purposes
    virtual MCPlatformControlType getcontroltype();
    virtual MCPlatformControlPart getcontrolsubpart();
    virtual MCPlatformControlState getcontrolstate();
};
#endif
