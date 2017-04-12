///* Copyright (C) 2003-2015 LiveCode Ltd.
//
//This file is part of LiveCode.
//
//LiveCode is free software; you can redistribute it and/or modify it under
//the terms of the GNU General Public License v3 as published by the Free
//Software Foundation.
//
//LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
//WARRANTY; without even the implied warranty of MERCHANTABILITY or
//FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
//for more details.
//
//You should have received a copy of the GNU General Public License
//along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */
//
////
//// MCFontlist class functions
////
//#include "lnxprefix.h"
//
//#include "globdefs.h"
//#include "filedefs.h"
//#include "parsedef.h"
//#include "objdefs.h"
//
//
//#include "font.h"
//#include "util.h"
//
//#include "globals.h"
//
//#include "lnxdc.h"
//#include "lnxflst.h"
//
//////////////////////////////////////////////////////////////////////////////////
//
//#define XLFD_LENGTH 256
//
//struct MCOldFontStruct: public MCFontStruct
//{
//	XFontStruct *fstruct;
//	MCNameRef reqname ;
//	uint2 reqsize ;
//	uint2 reqstyle;
//	uint4 max_byte1;
//};
//
//class MCOldFonttablenode
//{
//public:
//	char *charset;
//	char *bstring;
//	uint2 *psizes;
//	uint2 *bsizes;
//	uint2 *isizes;
//	uint2 *bisizes;
//	uint2 nplain;
//	uint2 nbold;
//	uint2 nitalic;
//	uint2 nbolditalic;
//	MCOldFonttablenode *next;
//	char* name;
//	MCOldFonttablenode();
//	~MCOldFonttablenode();
//	void registersize(uint2 size, uint2 *&sizes, uint2 &nsizes);
//	uint2 matchsize(uint2 size, uint2 *sizes, uint2 nsizes);
//	void addfont();
//	XFontStruct *loadfont(uint2 size, uint2 style);
//	
//#ifdef LIST
//
//	void list();
//#endif
//};
//
//class MCOldFontnode : public MCDLlist
//{
//	MCNameRef reqname;
//	uint2 reqsize;
//	uint2 reqstyle;
//	MCOldFontStruct font;
//	static MCOldFonttablenode *table;
//public:
//	MCOldFontnode(MCNameRef fname, uint2 &size, uint2 style);
//	MCOldFontnode();
//	~MCOldFontnode();
//	void buildtable();
//	void freetable();
//	MCOldFonttablenode *findtablenode(MCStringRef name);
//	MCOldFonttablenode *gettable()
//	{
//		return table;
//	}
//	XFontStruct *lookup(MCStringRef name, uint2 size, uint2 style);
//	
//	MCFontStruct *getfont(MCNameRef fname, uint2 size, uint2 style);
//	MCFontStruct *getfontstruct()
//	{
//		return &font;
//	}
//	MCNameRef getname()
//	{
//		return reqname;
//	}
//	uint2 getsize()
//	{
//		return reqsize;
//	}
//	uint2 getstyle()
//	{
//		return reqstyle;
//	}
//	MCOldFontnode *next()
//	{
//		return (MCOldFontnode *)MCDLlist::next();
//	}
//	MCOldFontnode *prev()
//	{
//		return (MCOldFontnode *)MCDLlist::prev();
//	}
//	void totop(MCOldFontnode *&list)
//	{
//		MCDLlist::totop((MCDLlist *&)list);
//	}
//	void insertto(MCOldFontnode *&list)
//	{
//		MCDLlist::insertto((MCDLlist *&)list);
//	}
//	void appendto(MCOldFontnode *&list)
//	{
//		MCDLlist::appendto((MCDLlist *&)list);
//	}
//	void append(MCOldFontnode *node)
//	{
//		MCDLlist::append((MCDLlist *)node);
//	}
//	void splitat(MCOldFontnode *node)
//	{
//		MCDLlist::splitat((MCDLlist *)node) ;
//	}
//	MCOldFontnode *remove
//	(MCOldFontnode *&list)
//	{
//		return (MCOldFontnode *)MCDLlist::remove
//			       ((MCDLlist *&)list);
//	}
//};
//
//class MCOldFontlist: public MCFontlist
//{
//	MCOldFontnode *fonts;
//
//public:
//	MCOldFontlist();
//	~MCOldFontlist();
//
//	virtual void destroy(void);
//
//	virtual MCFontStruct *getfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer);
//	virtual bool getfontnames(MCStringRef p_type, MCListRef& r_names);
//	virtual bool getfontsizes(MCStringRef p_fname, MCListRef& r_sizes);
//	virtual bool getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles);
//	virtual bool getfontstructinfo(MCNameRef& r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font);
//	virtual void getfontreqs(MCFontStruct *f, MCNameRef& r_name, uint2& r_size, uint2& r_style);
//
//	virtual int4 ctxt_textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override);
//	virtual bool ctxt_layouttext(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font, MCTextLayoutCallback p_callback, void *p_context);
//};
//
//////////////////////////////////////////////////////////////////////////////////
//
//static Atom pixelsize = 0;
//
//MCOldFonttablenode::MCOldFonttablenode()
//{
//	charset = NULL;
//	bstring = MCU_empty();
//	psizes = NULL;
//	bsizes = NULL;
//	isizes = NULL;
//	bisizes = NULL;
//	nplain = 0;
//	nbold = 0;
//	nitalic = 0;
//	nbolditalic = 0;
//	next = NULL;
//	name = NULL;
//}
//
//MCOldFonttablenode::~MCOldFonttablenode()
//{
//	delete charset;
//	delete bstring;
//	delete psizes;
//	delete bsizes;
//	delete isizes;
//	delete bisizes;
//	delete name;
//}
//
//void MCOldFonttablenode::registersize(uint2 size, uint2 *&sizes, uint2 &nsizes)
//{
//	uint2 i;
//	for (i = 0 ; i < nsizes ; i++)
//		if (sizes[i] == size)
//			return;
//	MCU_realloc((char **)&sizes, nsizes, nsizes + 1, sizeof(uint2));
//	sizes[nsizes++] = size;
//}
//
//uint2 MCOldFonttablenode::matchsize(uint2 size, uint2 *sizes, uint2 nsizes)
//{
//	uint2 i;
//	uint2 bestmatch = 0;
//	for (i = 0 ; i < nsizes ; i++)
//	{
//		if (sizes[i] == 0 || sizes[i] == size)
//			return size;
//		if (sizes[i] < size && sizes[i] > bestmatch)
//			bestmatch = sizes[i];
//	}
//	return bestmatch;
//}
//
//void MCOldFonttablenode::addfont()
//{
//	Boolean bold = False;
//	Boolean italic = False;
//	char *sptr;
//
//	if ((sptr = MCU_strtok(NULL, "-")) == NULL)
//		return;
//	if (strequal(sptr, "bold") || strequal(sptr, "demibold")
//	        || strequal(sptr, "demi"))
//	{
//		bold = True;
//		if (bstring[0] == '\0')
//		{
//			delete bstring;
//			bstring = strclone(sptr);
//		}
//	}
//
//	if ((sptr = MCU_strtok(NULL, "-")) == NULL)
//		return;
//	if (strequal(sptr, "i") || strequal(sptr, "o"))
//		italic = True;
//	if (MCU_strtok(NULL, "-") == NULL || (sptr = MCU_strtok(NULL, "-")) == NULL
//	        || (sptr = MCU_strtok(NULL, "-")) == NULL)
//		return;
//	uint2 size = strtol(sptr, NULL, 10);
//	if (bold)
//		if (italic)
//			registersize(size, bisizes, nbolditalic);
//		else
//			registersize(size, bsizes, nbold);
//	else
//		if (italic)
//			registersize(size, isizes, nitalic);
//		else
//			registersize(size, psizes, nplain);
//	if (MCU_strtok(NULL, "-") == NULL
//	        || MCU_strtok(NULL, "-") == NULL
//	        || MCU_strtok(NULL, "-") == NULL
//	        || MCU_strtok(NULL, "-") == NULL
//	        || (sptr = MCU_strtok(NULL, "-")) == NULL)
//		return;
//	sptr += strlen(sptr) + 1;
//	if (charset == NULL)
//		charset = strclone(sptr);
//	else
//		if (strequal(sptr, "iso8859-1") && !strequal(charset, "iso8859-1"))
//		{
//			delete charset;
//			charset = strclone(sptr);
//		}
//}
//
//XFontStruct *MCOldFonttablenode::loadfont(uint2 size, uint2 style)
//{
//	char fontname[XLFD_LENGTH];
//	const char *wptr = "medium";
//	const char *iptr = "r";
//	Boolean bold = False;
//	Boolean italic = False;
//
//	if (MCF_getweightint(style) > (FA_DEFAULT_STYLE & FA_WEIGHT))
//	{
//		wptr = bstring;
//		bold = True;
//	}
//	if (style & FA_ITALIC)
//	{
//		iptr = "i";
//		italic = True;
//	}
//	if (bold)
//		if (italic)
//			size = matchsize(size, bisizes, nbolditalic);
//		else
//			size = matchsize(size, bsizes, nbold);
//	else
//		if (italic)
//			size = matchsize(size, isizes, nitalic);
//		else
//			size = matchsize(size, psizes, nplain);
//	sprintf(fontname, "-*-%s-%s-%s-normal-*-%d-*-%s",
//	        name, wptr, iptr, size, charset);
//	XFontStruct *fsptr = XLoadQueryFont(MCdpy, fontname);
//	if (fsptr == NULL)
//	{
//		sprintf(fontname, "-*-%s-%s-%s-normal-*-%d-0-0-0-*-0-%s",
//		        name, wptr, iptr, size, charset);
//		fsptr = XLoadQueryFont(MCdpy, fontname);
//	}
//	if (fsptr == NULL && italic)
//	{
//		iptr = "o";
//		sprintf(fontname, "-*-%s-%s-%s-normal-*-%d-*-%s",
//		        name, wptr, iptr, size, charset);
//		fsptr = XLoadQueryFont(MCdpy, fontname);
//	}
//	if (fsptr == NULL)
//	{
//		sprintf(fontname, "-*-%s-%s-%s-normal-*-%d-*", name, wptr, iptr, size);
//		fsptr = XLoadQueryFont(MCdpy, fontname);
//		if (fsptr == NULL && italic)
//		{
//			iptr = "i";
//			sprintf(fontname, "-*-%s-%s-%s-normal-*-%d-*", name, wptr, iptr, size);
//			fsptr = XLoadQueryFont(MCdpy, fontname);
//		}
//	}
//	if (fsptr == NULL)
//	{
//		sprintf(fontname, "-*-%s-%s-%s-*-%d-*-%s",
//		        name, wptr, iptr, size, charset);
//		fsptr = XLoadQueryFont(MCdpy, fontname);
//		if (fsptr == NULL && italic)
//		{
//			iptr = "o";
//			sprintf(fontname, "-*-%s-%s-%s-*-%d-*-%s",
//			        name, wptr, iptr, size, charset);
//			fsptr = XLoadQueryFont(MCdpy, fontname);
//		}
//	}
//	if (fsptr == NULL)
//	{
//		sprintf(fontname, "-*-%s-%s-%s-*-%d-*", name, wptr, iptr, size);
//		fsptr = XLoadQueryFont(MCdpy, fontname);
//		if (fsptr == NULL && italic)
//		{
//			iptr = "i";
//			sprintf(fontname, "-*-%s-%s-%s-*-%d-*", name, wptr, iptr, size);
//			fsptr = XLoadQueryFont(MCdpy, fontname);
//		}
//	}
//	if (fsptr == NULL)
//	{
//		sprintf(fontname, "-*-%s-*--%d-*", name, size);
//		fsptr = XLoadQueryFont(MCdpy, fontname);
//	}
//		
//	return fsptr;
//}
//
//MCOldFonttablenode *MCOldFontnode::table = NULL;
//
//MCOldFontnode::MCOldFontnode(MCNameRef fname_n, uint2 &size, uint2 style)
//{
//	char fontname[XLFD_LENGTH];
//
//	//MCString fname;
//	//fname = MCStringGetCString(MCNameGetString(fname_n));
//
//	reqname = MCValueRetain(fname_n);
//	reqsize = size;
//	reqstyle = style;
//	
//	uint2 t_original_size;
//	t_original_size = size;
//	
//	XFontStruct *fs = NULL;
//	
//	memset(&font, 0, sizeof(MCFontStruct));
//	
//	if (MCnoui)
//		return;
//
//	// MW-2005-02-08: We aren't going to use XMBTEXT for now, instead we will
//	//   search for an appropriate ISO10646 font if in 'encoding mode'.
//	char *temp;
//	/* UNCHECKED */ MCStringConvertToCString(MCNameGetString(reqname), temp);
//	sprintf(fontname, "-*-%.*s-%s-%s-%s--%d-*-*-*-*-*-iso8859-%d",
//		        strchr(temp, ',') - temp, temp, MCF_getweightstring(style),
//		        MCF_getslantshortstring(style),
//		        MCF_getexpandstring(style), size, MCcharset);
//	
//	if ((fs = XLoadQueryFont(MCdpy, fontname)) == NULL)
//		fs = lookup(MCNameGetString(reqname), size, style);
//
//	if (fs == NULL)
//		if ((fs = XLoadQueryFont(MCdpy, temp)) != NULL)
//		{
//			if (pixelsize == 0)
//				pixelsize = XInternAtom(MCdpy, "PIXEL_SIZE", True);
//			uint2 i = fs->n_properties;
//			while (i--)
//				if (fs->properties[i].name == pixelsize)
//				{
//					size = reqsize = fs->properties[i].card32;
//					break;
//				}
//			size = reqsize = fs->ascent + fs->descent - 2;
//		}
//
//	if (fs == NULL)
//		fs = lookup(MCSTR(DEFAULT_TEXT_FONT), size, style);
//	if (fs == NULL)
//		fs = XLoadQueryFont(MCdpy, "fixed");
//
//
//	font.reqname = MCValueRetain(fname_n);
//	font.reqsize = reqsize ;
//	font.reqstyle = reqstyle ;
//
//	font.fstruct = fs;
//	font.max_byte1 = fs -> max_byte1;
//
//	font.ascent = fs -> ascent;
//	font.descent = fs -> descent;
//}
//
//MCOldFontnode::MCOldFontnode()
//{
//	reqname = NULL;
//}
//
//MCOldFontnode::~MCOldFontnode()
//{
//	if (reqname != NULL)
//	{
//		if (!MCnoui)
//			XFreeFont(MCdpy, font.fstruct);
//		MCValueRelease(font.reqname);
//		MCValueRelease(reqname);
//	}
//}
//
//void MCOldFontnode::buildtable()
//{
//	if (table != NULL || MCnoui)
//		return;
//	int nnames;
//	char **allnames = XListFonts(MCdpy, "-*-iso8859-1", MAXUINT2, &nnames);
//	int i;
//	for (i = 0 ; i < nnames ; i++)
//	{
//		char *sptr = allnames[i];
//		sptr++;
//		if (MCU_strtok(sptr, "-") == NULL)
//			continue;
//		if ((sptr = MCU_strtok(NULL, "-")) == NULL)
//			continue;
//		MCAutoStringRef t_node;
//        /* UNCHECKED */ MCStringCreateWithSysString(sptr, &t_node);
//		MCOldFonttablenode *fnptr = findtablenode(*t_node);
//		if (fnptr == NULL)
//		{
//			fnptr = new (nothrow) MCOldFonttablenode;
//			char *temp;
//			/* UNCHECKED */ MCStringConvertToCString(*t_node, temp);
//			fnptr->name = strclone(temp);
//			delete temp;
//			fnptr->next = table;
//			table = fnptr;
//		}
//		fnptr->addfont();
//	}
//	XFreeFontNames(allnames);
//}
//
//void MCOldFontnode::freetable()
//{
//	while (table != NULL)
//	{
//		MCOldFonttablenode *fptr = table;
//		table = table->next;
//		delete fptr;
//	}
//}
//
//MCOldFonttablenode *MCOldFontnode::findtablenode(MCStringRef name)
//{
//	if (table != NULL)
//	{
//		MCOldFonttablenode *fnptr = table;
//		do
//		{
//			if (MCStringIsEqualToCString(name, fnptr->name, kMCCompareExact))
//				return fnptr;
//			fnptr = fnptr->next;
//		}
//		while (fnptr != NULL);
//	}
//	return NULL;
//}
//
//XFontStruct *MCOldFontnode::lookup(MCStringRef name, uint2 size, uint2 style)
//{
//	
//	buildtable();
//	MCOldFonttablenode *fnptr = findtablenode(name);
//	if (fnptr == NULL || MCnoui)
//		return NULL;
//	
//	return fnptr->loadfont(size, style);
//}
//
//MCFontStruct *MCOldFontnode::getfont(MCNameRef fname, uint2 size, uint2 style)
//{
//	if (!MCNameIsEqualTo(fname, reqname))
//		return NULL;
//	if (size == 0)
//		return &font;
//	if (style != reqstyle)
//		return NULL;
//	if (size != reqsize)
//		return NULL;
//	return &font;
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//MCOldFontlist::MCOldFontlist()
//{
//	fonts = NULL;
//}
//
//MCOldFontlist::~MCOldFontlist()
//{
//	if ( MCnoui )
//		return ;
//	if (fonts != NULL)
//		fonts->freetable();
//	while (fonts != NULL)
//	{
//		MCOldFontnode *fptr = fonts->remove
//		                   (fonts);
//		delete fptr;
//	}
//}
//
//void MCOldFontlist::destroy(void)
//{
//	delete this;
//}
//
//MCFontStruct *MCOldFontlist::getfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printing)
//{
//	MCOldFontnode *tmp = fonts;
//	if (tmp != NULL)
//		do
//		{
//			MCFontStruct *font = tmp->getfont(fname, size, style);
//			if (font != NULL)
//				return font;
//			tmp = tmp->next();
//		}
//		while (tmp != fonts);
//	tmp = new (nothrow) MCOldFontnode(fname, size, style);
//	tmp->appendto(fonts);
//	return tmp->getfont(fname, size, style);
//}
//
//bool MCOldFontlist::getfontnames(MCStringRef p_type, MCListRef& r_names)
//{
//	if (MCnoui)
//	{
//		r_names = MCValueRetain(kMCEmptyList);
//		return true;
//	}
//
//	MCAutoListRef t_list;
//	if (!MCListCreateMutable('\n', &t_list))
//		return false;
//
//	MCOldFontnode dummynode;
//	dummynode.buildtable();
//	MCOldFonttablenode *table = dummynode.gettable();
//	MCOldFonttablenode *tmp = table;
//	do
//	{
//		if (!MCListAppendCString(*t_list, tmp->name))
//			return false;
//		tmp = tmp->next;
//	}
//	while (tmp != NULL);
//
//	return MCListCopy(*t_list, r_names);
//}
//
//bool MCOldFontlist::getfontsizes(MCStringRef p_fname, MCListRef& r_sizes)
//{
//	if (MCnoui)
//	{
//		r_sizes = MCValueRetain(kMCEmptyList);
//		return true;
//	}
//
//	MCAutoListRef t_list;
//	if (!MCListCreateMutable('\n', &t_list))
//		return false;
//
//	MCOldFontnode dummynode;
//	dummynode.buildtable();
//	MCOldFonttablenode *fnptr = dummynode.findtablenode(p_fname);
//	if (fnptr != NULL)
//		for (uint2 i = 0 ; i < fnptr->nplain ; i++)
//			if (!MCListAppendInteger(*t_list, fnptr -> psizes[i]))
//				return false;
//
//	return MCListCopy(*t_list, r_sizes);
//}
//
//bool MCOldFontlist::getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles)
//{
//	if (MCnoui)
//	{
//		r_styles = MCValueRetain(kMCEmptyList);
//		return true;
//	}
//
//	MCAutoListRef t_list;
//	if (!MCListCreateMutable('\n', &t_list))
//		return false;
//
//	MCOldFontnode dummynode;
//	dummynode.buildtable();
//	MCOldFonttablenode *fnptr = dummynode.findtablenode(p_fname);
//	if (fnptr != NULL)
//	{
//		uint2 i;
//		for (i = 0 ; i < fnptr->nplain ; i++)
//		{
//			if (fnptr->psizes[i] == 0 || fnptr->psizes[i] == fsize)
//			{
//				if (!MCListAppend(*t_list, MCN_plain))
//					return false;
//				if (fnptr->nbold > i && (fnptr->bsizes[i] == 0 || fnptr->bsizes[i] == fsize))
//					if (!MCListAppend(*t_list, MCN_bold))
//						return false;
//				if (fnptr->nitalic > i && (fnptr->isizes[i] == 0 || fnptr->isizes[i] == fsize))
//					if (!MCListAppend(*t_list, MCN_italic))
//						return false;
//				if (fnptr->nbolditalic > i && (fnptr->bisizes[i] == 0 || fnptr->bisizes[i] == fsize))
//					if (!MCListAppend(*t_list, MCN_bold_italic))
//						return false;
//				break;
//			}
//		}
//	}
//	return MCListCopy(*t_list, r_styles);
//}
//
//bool MCOldFontlist::getfontstructinfo(MCNameRef& r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font)
//{
//	MCOldFontnode *t_font = fonts;
//	while (t_font != NULL)
//	{
//		if (t_font->getfontstruct() == p_font)
//		{
//			r_name = t_font->getname();
//			r_size = t_font->getsize();
//			r_style = t_font->getstyle();
//			return true;
//		}
//		t_font = t_font->next();
//	}
//	return false;
//}
//
//void MCOldFontlist::getfontreqs(MCFontStruct *f, MCNameRef& r_name, uint2& r_size, uint2& r_style)
//{
//	MCOldFontStruct *t_font;
//	t_font = static_cast<MCOldFontStruct *>(f);
//	r_name = t_font -> reqname;
//	r_size = t_font -> reqsize;
//	r_style = t_font -> reqstyle;
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//int4 MCOldFontlist::ctxt_textwidth(MCFontStruct *of, const char *s, uint2 l, bool p_unicode_override)
//{
//	MCOldFontStruct *f;
//	f = static_cast<MCOldFontStruct *>(of);
//
//	bool useUnicode = f->max_byte1 > 0 || p_unicode_override;
//
//	if ( useUnicode )
//	{
//		uint2 x_l ;
//		XChar2b x_s[l / 2];
//		
//		x_l = (l + 1) / 2;
//		for(int i = 0; i < x_l; ++i)
//			x_s[i] . byte1 = s[i * 2 + 1], x_s[i] . byte2 = s[i * 2];
//
//		return XTextWidth16(f->fstruct, (const XChar2b *)x_s, x_l);
//	}
//	
//	return XTextWidth(f->fstruct, s, l);
//}
//
//bool MCOldFontlist::ctxt_layouttext(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font, MCTextLayoutCallback p_callback, void *p_context)
//{
//	return false;
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//MCFontlist *MCFontlistCreateOld(void)
//{
//	return new MCOldFontlist;
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
