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

//
// HyperMCCard import functions
//

#define HC_HEADER_SIZE 2048

#define HC_MESSAGE_LENGTH 256

enum HC_File_type {
    HC_RAW,
    HC_MACBIN,
    HC_BINHEX
};

#define HC_STAK 0x5354414b
#define HC_MAST 0x4d415354
#define HC_LIST 0x4c495354
#define HC_PAGE 0x50414745
#define HC_BKGD 0x424b4744
#define HC_CARD 0x43415244
#define HC_BMAP 0x424d4150
#define HC_STBL 0x5354424c
#define HC_FTBL 0x4654424c
#define HC_FREE 0x46524545
#define HC_PRNT 0x50524e54
#define HC_PRFT 0x50524654
#define HC_PRST 0x50525354
#define HC_TAIL 0x5441494c
#define HC_BUGS 0xFFFF5354

#define HC_ICON 0x49434f4e
#define HC_CURS 0x43555253
#define HC_SND  0x736e6420

#define HC_F_MULTIPLE       0x0010
#define HC_F_MARGINS        0x0020
#define HC_F_SHOW_LINES     0x0040
#define HC_F_AUTO_SELECT    0x0080
#define HC_F_LOCK_TEXT      0x0100
#define HC_F_AUTO_TAB       0x0200
#define HC_F_UNFIXED_HEIGHT 0x0400
#define HC_F_SHARED_TEXT    0x0800
#define HC_F_DONT_SEARCH    0x1000
#define HC_F_DONT_WRAP      0x2000
#define HC_F_INVISIBLE      0x8000

#define HC_FSTYLE_TRANS     0
#define HC_FSTYLE_OPAQUE    1
#define HC_FSTYLE_RECT      2
#define HC_FSTYLE_SHADOW    4
#define HC_FSTYLE_SCROLL    7

#define HC_B_NOT_SHARED     0x0010
#define HC_B_AUTO_HILITE    0x0020
#define HC_B_HILITED        0x0040
#define HC_B_SHOW_NAME      0x0080
#define HC_B_DISABLED       0x0100
#define HC_B_INVISIBLE      0x8000

#define HC_BSTYLE_TRANS     0
#define HC_BSTYLE_OPAQUE    1
#define HC_BSTYLE_RECT      2
#define HC_BSTYLE_ROUND     3
#define HC_BSTYLE_SHADOW    4
#define HC_BSTYLE_CHECK     5
#define HC_BSTYLE_RADIO     6
#define HC_BSTYLE_STANDARD  8
#define HC_BSTYLE_DEFAULT   9
#define HC_BSTYLE_OVAL      10
#define HC_BSTYLE_POPUP     11

#define HC_BC_DONT_SEARCH   0x08
#define HC_BC_HIDE_BMAP     0x20
#define HC_BC_CANT_DELETE   0x40

#define HC_OTYPE_BUTTON     0x01
#define HC_OTYPE_FIELD      0x02

#define HC_TSTYLE_BOLD       0x0100
#define HC_TSTYLE_ITALIC     0x0200
#define HC_TSTYLE_UNDERLINE  0x0400
#define HC_TSTYLE_OUTLINE    0x0800
#define HC_TSTYLE_SHADOW     0x1000
#define HC_TSTYLE_CONDENSE   0x2000
#define HC_TSTYLE_EXTEND     0x4000
#define HC_TSTYLE_GROUP      0x8000

#define HC_TALIGN_LEFT       0x00
#define HC_TALIGN_CENTER     0x01
#define HC_TALIGN_RIGHT      0xFF

#define HC_DEFAULT_TEXT_FONT "helvetica"
#define HC_DEFAULT_TEXT_SIZE 12

#define HC_NICONS            192
#define HC_DEFAULT_ICON      330

class MCHcstak;
class MCCdata;

struct Hcfont
{
	uint2 id;
	const char *name;
};

struct Hcatts
{
	uint2 id;
	uint2 fid;
	uint2 size;
	uint2 style;
};

class MCHcsnd : public MCDLlist
{
public:
	MCNameRef m_name;
	int1 *data;
	uint4 size;
	uint2 rate;
	uint2 id;
	MCHcsnd();
	~MCHcsnd();
	Boolean import(uint4 inid, MCNameRef inname, char *sptr);
	MCAudioClip *build();
	MCHcsnd *next()
	{
		return (MCHcsnd *)MCDLlist::next();
	}
	void appendto(MCHcsnd *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCHcsnd *remove
	(MCHcsnd *&list)
	{
		return (MCHcsnd *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

class MCHctext : public MCDLlist
{
public:
	uint2 *atts;
	char *string;
	Boolean card;
	uint2 id;
	uint4 cid;
	MCHctext();
	~MCHctext();
	IO_stat parse(char *sptr);
	MCCdata *buildf(MCHcstak *hcsptr, MCField *parent);
	MCCdata *buildb();
	char *buildm();
	MCHctext *next()
	{
		return (MCHctext *)MCDLlist::next();
	}
	void appendto(MCHctext *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCHctext *remove
	(MCHctext *&list)
	{
		return (MCHctext *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

class MCHcfield : public MCDLlist
{
public:
    char *name;
    char *script;
	MCRectangle rect;
	uint2 id;
	uint2 atts;
	uint2 style;
	uint2 tfont;
	uint2 tsize;
	uint2 tstyle;
	uint2 talign;
	uint2 theight;
	uint2 hctstyle;
	MCHctext *text;
	MCHcfield();
	~MCHcfield();
	IO_stat parse(char *sptr);
	MCControl *build(MCHcstak *hcsptr, MCStack *sptr);
	MCHcfield *next()
	{
		return (MCHcfield *)MCDLlist::next();
	}
	void appendto(MCHcfield *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCHcfield *remove
	(MCHcfield *&list)
	{
		return (MCHcfield *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

class MCHcbutton : public MCDLlist
{
public:
    char *name;
    char *script;
	MCRectangle rect;
	uint2 titlewidth;
	uint2 icon;
	uint2 id;
	uint2 atts;
	uint2 style;
	uint2 tfont;
	uint2 tsize;
	uint2 tstyle;
	uint2 talign;
	uint2 hctstyle;
	MCHctext *text;
	MCHcbutton();
	~MCHcbutton();
	IO_stat parse(char *sptr);
	MCControl *build(MCHcstak *hcsptr, MCStack *sptr);
	MCHcbutton *next()
	{
		return (MCHcbutton *)MCDLlist::next();
	}
	void appendto(MCHcbutton *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCHcbutton *remove
	(MCHcbutton *&list)
	{
		return (MCHcbutton *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

class MCHcbmap : public MCDLlist
{
public:
	uint4 id;
	MCRectangle rect;
	MCRectangle mrect;
	Boolean visible;
	// IM-2014-04-08: [[ Bug 12101 ]] Buffers to hold 1-bit bitmaps - dimensions stored in rect (data), mrect (mask)
	uint8_t *mask;
	uint8_t *data;
    MCNameRef m_name;
	uint2 xhot;
	uint2 yhot;
	MCHcbmap();
	~MCHcbmap();
	void setvisible(Boolean newvis);
	void icon(uint4 inid, MCNameRef inname, char *sptr);
	void cursor(uint4 inid, MCNameRef inname, char *sptr);
	IO_stat parse(char *sptr);
	MCControl *build();
	MCHcbmap *next()
	{
		return (MCHcbmap *)MCDLlist::next();
	}
	void appendto(MCHcbmap *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCHcbmap *remove
	(MCHcbmap *&list)
	{
		return (MCHcbmap *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

class MCHccard : public MCDLlist
{
public:
	char *name;
	char *script;
	uint2 nobjects;
	uint2 *objects;
	MCHcbutton *hcbuttons;
	MCHcfield *hcfields;
	MCHctext *hctexts;
	uint4 bkgdid;
	uint4 bmapid;
	uint1 atts;
	uint4 id;
	MCHccard();
	~MCHccard();
	IO_stat parse(char *sptr);
	MCCard *build(MCHcstak* hcsptr, MCStack *sptr);
	MCHccard *next()
	{
		return (MCHccard *)MCDLlist::next();
	}
	void appendto(MCHccard *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCHccard *remove
	(MCHccard *&list)
	{
		return (MCHccard *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

class MCHcbkgd : public MCDLlist
{
public:
	char *name;
	char *script;
	uint2 nobjects;
	uint2 *objects;
	MCHcbutton *hcbuttons;
	MCHcfield *hcfields;
	MCHctext *hctexts;
	uint4 id;
	uint4 bkgdid;
	uint4 bmapid;
	uint1 atts;
	MCHcbkgd();
	~MCHcbkgd();
	IO_stat parse(char *sptr);
	MCGroup *build(MCHcstak* hcsptr, MCStack *sptr);
	MCHcbkgd *next()
	{
		return (MCHcbkgd *)MCDLlist::next();
	}
	void appendto(MCHcbkgd *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCHcbkgd *remove
	(MCHcbkgd *&list)
	{
		return (MCHcbkgd *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

class MCHcstak
{
public:
	MCRectangle rect;
	char *name;
	char *script;
	char *fullbuffer;
	char *buffer;
	uint4 *pages;
	uint1 *marks;
	uint4 npages;
	struct Hcfont *fonts;
	struct Hcatts *atts;
	uint2 nfonts;
	uint2 natts;
	uint2 pagesize;
	uint4 npbuffers;
	uint2 *pbuffersizes;
	char **pbuffers;
	MCHcbkgd *hcbkgds;
	MCHccard *hccards;
	MCHcbmap *hcbmaps;
	MCHcbmap *icons;
	MCHcbmap *cursors;
	MCHcsnd *snds;
	MCHcstak(char *inname);
	~MCHcstak();
	uint4 geticon(uint2 iid);
	MCHcbmap *getbmap(uint4 bid);
	MCHcbmap *removebmap(uint4 bid);
	MCHcbkgd *getbkgd(uint4 bid);
	const char *getfont(uint2 fid);
	void getatts(uint2 aid, const char *&font, uint2 &size, uint2 &style);
	IO_stat read(IO_handle stream);
	MCStack *build();
	
#ifdef _MAC_DESKTOP
	IO_stat macreadresources(void);
#endif
};

extern IO_stat hc_import(MCStringRef p_name, IO_handle p_stream, MCStack *&p_sptr);

