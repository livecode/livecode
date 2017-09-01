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

#ifndef	BUTTON_H
#define	BUTTON_H

#include "mccontrol.h"
#include "stack.h"
#include "mctristate.h"

#define AQUA_FUDGE 8



#define needfocus() (MClook == LF_MOTIF && state & CS_KFOCUSED \
		     && (flags & F_SHOW_BORDER || !(flags & F_AUTO_ARM)) \
		     && !(extraflags & EF_NO_FOCUS_BORDER) \
                     && (getstyleint(flags) != F_MENU \
			 || menumode != WM_TOP_LEVEL))


#define standardbtn() ((IsMacEmulatedLF() || MCcurtheme!=NULL) && flags & F_3D \
		  && !(flags & F_AUTO_ARM) && flags & F_SHOW_BORDER \
		  && ((IsMacEmulatedLF() && getstyleint(flags) == F_STANDARD) || \
			  (MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_PUSHBUTTON) && \
			   getstyleint(flags) == F_STANDARD) || \
			  (MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_BEVELBUTTON) && \
			   getstyleint(flags) == F_RECTANGLE)) \
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

struct MCInterfaceButtonIcon;

#define MENUCONTROL_NONE 0
#define MENUCONTROL_ITEM 1
#define MENUCONTROL_SEPARATOR 2

class MCButtonMenuHandler
{
public:
	virtual bool OnMenuPick(MCButton *p_button, MCValueRef p_pick, MCValueRef p_old_pick) = 0;
};

class ButtonMenuCallback;

typedef MCObjectProxy<MCButton>::Handle MCButtonHandle;

class MCButton : public MCControl, public MCMixinObjectHandle<MCButton>
{
public:
    
    enum { kObjectType = CT_BUTTON };
    using MCMixinObjectHandle<MCButton>::GetHandle;
    
private:
    
	friend class MCHcbutton;
	MCCdata *bdata;
	iconlist *icons;
	MCStringRef label;
	MCNameRef menuname;
	MCStringRef menustring;
	MCField *entry;
	MCStackHandle menu;
	MCStringRef acceltext;
	MCArrayRef tabs;
	MCPlatformMenuRef m_system_menu;
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
	static const Keynames button_keys[];
	static uint4 clicktime;
	static uint2 menubuttonheight;
	static Boolean starthilite;
	static uint2 starttab;
	static MCImage *macrb;
	static MCImage *macrbtrack;
	static MCImage *macrbhilite;
	static MCImage *macrbhilitetrack;

	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    
	MCButtonMenuHandler *m_menu_handler;
	
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

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }

	virtual bool visit_self(MCObjectVisitor *p_visitor);

	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual Boolean kfocusnext(Boolean top);
	virtual Boolean kfocusprev(Boolean bottom);
	virtual void kunfocus();
	virtual Boolean kdown(MCStringRef p_string, KeySym key);
	virtual Boolean kup(MCStringRef p_string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
#ifdef _MAC_DESKTOP
	virtual void timer(MCNameRef mptr, MCParameter *params);
#endif

	virtual uint2 gettransient() const;
	virtual void applyrect(const MCRectangle &nrect);

	virtual void closemenu(Boolean kfocus, Boolean disarm);
	
	// MW-2011-09-20: [[ Collision ]] Compute shape of button - will use mask of icon if possible.
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);

	// virtual functions from MCControl
	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	virtual IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);
	virtual IO_stat load(IO_handle stream, uint32_t version);
	virtual IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);

	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);
	virtual MCControl *findnum(Chunk_term type, uint2 &num);
    virtual MCControl *findname(Chunk_term type, MCNameRef p_name);
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
	void activate(Boolean notify, KeySym p_key);
	void clearmnemonic();
	void setupmnemonic();
	MCCdata *getbptr(uint4 cardid);
	uint2 getfamily();
	MCTristate gethilite(uint4 parid);
	void setdefault(Boolean def);
	Boolean sethilite(uint4 parid, MCTristate hilite);
	void resethilite(uint4 parid, MCTristate hilite);

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
	
	MCStringRef getlabeltext();

	MCStringRef getmenustring()
	{
		return menustring;
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
    MCStack *getmenu();
    
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
	
	void setmenuhandler(MCButtonMenuHandler *p_handler);
	Exec_stat handlemenupick(MCValueRef p_pick, MCValueRef p_old_pick);
	
	void openmenu(Boolean grab);
	void freemenu(Boolean force);
	MCRange getmenurange();
	void docascade(MCStringRef t_pick);
	void setupmenu();
	
	bool menuisopen();
	
	bool selectedchunk(MCStringRef& r_string);
	bool selectedline(MCStringRef& r_string);
	bool selectedtext(MCStringRef& r_string);
	bool resetlabel();
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
	void drawlabel(MCDC *dc, int2 sx, int sy, uint2 t, const MCRectangle &srect, MCStringRef p_label, uint2 fstyle, uindex_t p_mnemonic);
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
    static void getmacmenuitemtextfromaccelerator(MCPlatformMenuRef menu, KeySym key, uint1 mods, MCStringRef &r_string, bool issubmenu);
    
    bool macmenuisopen();
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

	////////// PROPERTY SUPPORT METHODS

	void GetIcon(MCExecContext& ctxt, Properties which, uinteger_t& r_icon);
	void SetIcon(MCExecContext& ctxt, Properties which, uinteger_t p_icon);
    void DoGetIcon(MCExecContext& ctxt, Current_icon which, MCInterfaceButtonIcon& r_icon);
    void DoSetIcon(MCExecContext& ctxt, Current_icon which, const MCInterfaceButtonIcon& p_icon);
    void SetChunkProp(MCExecContext& ctxt, uint32_t p_part, int32_t p_start, int32_t p_finish, Properties which, bool setting);
    
	void UpdateIconAndMenus(void);

	////////// PROPERTY ACCESSORS

	virtual void SetName(MCExecContext& ctxt, MCStringRef p_name);
	void GetStyle(MCExecContext& ctxt, intenum_t& r_style);
	void SetStyle(MCExecContext& ctxt, intenum_t p_style);
	void GetAutoArm(MCExecContext& ctxt, bool& r_setting);
	void SetAutoArm(MCExecContext& ctxt, bool setting);
	void GetAutoHilite(MCExecContext& ctxt, bool& r_setting);
	void SetAutoHilite(MCExecContext& ctxt, bool setting);
	void GetArmBorder(MCExecContext& ctxt, bool& r_setting);
	void SetArmBorder(MCExecContext& ctxt, bool setting);
	void GetArmFill(MCExecContext& ctxt, bool& r_setting);
	void SetArmFill(MCExecContext& ctxt, bool setting);
	void GetHiliteBorder(MCExecContext& ctxt, bool& r_setting);
	void SetHiliteBorder(MCExecContext& ctxt, bool setting);
	void GetHiliteFill(MCExecContext& ctxt, bool& r_setting);
	void SetHiliteFill(MCExecContext& ctxt, bool setting);
	void GetShowHilite(MCExecContext& ctxt, bool& r_setting);
	void SetShowHilite(MCExecContext& ctxt, bool setting);
	void GetArm(MCExecContext& ctxt, bool& r_setting);
	void SetArm(MCExecContext& ctxt, bool setting);
	void GetSharedHilite(MCExecContext& ctxt, bool& r_setting);
	void SetSharedHilite(MCExecContext& ctxt, bool setting);
	void GetShowIcon(MCExecContext& ctxt, bool& r_setting);
	void SetShowIcon(MCExecContext& ctxt, bool setting);
	void GetShowName(MCExecContext& ctxt, bool& r_setting);
	void SetShowName(MCExecContext& ctxt, bool setting);
	void GetLabel(MCExecContext& ctxt, MCStringRef& r_label);
	void SetLabel(MCExecContext& ctxt, MCStringRef p_label);
	void GetUnicodeLabel(MCExecContext& ctxt, MCDataRef& r_label);
	void SetUnicodeLabel(MCExecContext& ctxt, MCDataRef p_label);
	void GetEffectiveLabel(MCExecContext& ctxt, MCStringRef& r_label);
	void GetEffectiveUnicodeLabel(MCExecContext& ctxt, MCDataRef& r_label);
	void GetLabelWidth(MCExecContext& ctxt, uinteger_t& r_width);
	void SetLabelWidth(MCExecContext& ctxt, uinteger_t p_width);
	void GetFamily(MCExecContext& ctxt, uinteger_t& r_family);
	void SetFamily(MCExecContext& ctxt, uinteger_t p_family);
	void GetVisited(MCExecContext& ctxt, bool& r_setting);
	void SetVisited(MCExecContext& ctxt, bool setting);
	void GetMenuHistory(MCExecContext& ctxt, uinteger_t& r_history);
	void SetMenuHistory(MCExecContext& ctxt, uinteger_t p_history);
	void GetMenuLines(MCExecContext& ctxt, uinteger_t*& r_lines);
	void SetMenuLines(MCExecContext& ctxt, uinteger_t* p_lines);
	void GetMenuButton(MCExecContext& ctxt, uinteger_t& r_button);
	void SetMenuButton(MCExecContext& ctxt, uinteger_t p_button);
	void GetMenuMode(MCExecContext& ctxt, intenum_t& r_mode);
	void SetMenuMode(MCExecContext& ctxt, intenum_t p_mode);
	void GetMenuName(MCExecContext& ctxt, MCNameRef& r_name);
	void SetMenuName(MCExecContext& ctxt, MCNameRef p_name);
	virtual void SetShowBorder(MCExecContext& ctxt, bool setting);
	void GetAcceleratorText(MCExecContext& ctxt, MCStringRef& r_text);
	void SetAcceleratorText(MCExecContext& ctxt, MCStringRef p_text);
	void GetUnicodeAcceleratorText(MCExecContext& ctxt, MCDataRef& r_text);
	void GetAcceleratorKey(MCExecContext& ctxt, MCStringRef& r_text);
	void SetAcceleratorKey(MCExecContext& ctxt, MCStringRef p_text);
	void GetAcceleratorModifiers(MCExecContext& ctxt, intset_t& r_mods);
	void SetAcceleratorModifiers(MCExecContext& ctxt, intset_t p_mods);
	void GetMnemonic(MCExecContext& ctxt, uinteger_t& r_mnemonic);
	void SetMnemonic(MCExecContext& ctxt, uinteger_t p_mnemonic);
	void GetFormattedWidth(MCExecContext& ctxt, integer_t& r_width);
	void GetFormattedHeight(MCExecContext& ctxt, integer_t& r_height);
	void GetDefault(MCExecContext& ctxt, bool& r_setting);
	void SetDefault(MCExecContext& ctxt, bool setting);
	virtual void SetTextFont(MCExecContext& ctxt, MCStringRef p_font);
	virtual void SetTextHeight(MCExecContext& ctxt, uinteger_t* p_height);
	virtual void SetTextSize(MCExecContext& ctxt, uinteger_t* p_size);
	virtual void SetTextStyle(MCExecContext& ctxt, const MCInterfaceTextStyle& p_style);
	virtual void SetEnabled(MCExecContext& ctxt, uint32_t part, bool setting);
	virtual void SetDisabled(MCExecContext& ctxt, uint32_t part, bool setting);
	void GetText(MCExecContext& ctxt, MCStringRef& r_text);
	void SetText(MCExecContext& ctxt, MCStringRef p_text);
	void GetUnicodeText(MCExecContext& ctxt, MCDataRef& r_text);
	void SetUnicodeText(MCExecContext& ctxt, MCDataRef p_text);
	virtual void SetCantSelect(MCExecContext& ctxt, bool setting);
    void SetArmedIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon);
    void GetArmedIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon);
    void SetDisabledIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon);
    void GetDisabledIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon);
    void SetIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon);
    void GetIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon);
    // SN-2014-06-25 [[ IconGravity ]]
    void GetIconGravity(MCExecContext &ctxt, intenum_t &r_gravity);
    void SetIconGravity(MCExecContext &ctxt, intenum_t p_gravity);
    void SetHiliteIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon);
    void GetHiliteIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon);
    void SetVisitedIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon);
    void GetVisitedIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon);
    void SetHoverIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon);
    void GetHoverIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon);
    virtual void SetMargins(MCExecContext& ctxt, const MCInterfaceMargins& p_margins);
    void GetHilite(MCExecContext& ctxt, uint32_t p_part, MCInterfaceTriState& r_hilite);
    void SetHilite(MCExecContext& ctxt, uint32_t p_part, const MCInterfaceTriState& p_hilite);
    
    void SetDisabledOfCharChunk(MCExecContext& ctxt, uint32_t p_part, int32_t p_start, int32_t p_finish, bool setting);
    void SetEnabledOfCharChunk(MCExecContext& ctxt, uint32_t p_part, int32_t p_start, int32_t p_finish, bool setting);
    void SetHiliteOfCharChunk(MCExecContext& ctxt, uint32_t p_part, int32_t p_start, int32_t p_finish, bool setting);
    void SetUnhiliteOfCharChunk(MCExecContext& ctxt, uint32_t p_part, int32_t p_start, int32_t p_finish, bool setting);
    
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
    
    // Returns the size that check-marks should be drawn at
    int16_t GetCheckSize() const;
};
#endif
