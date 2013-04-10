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

//
// List of patterns
//
#ifndef	PIXMAPLIST_H
#define	PIXMAPLIST_H

#include "dllst.h"

class MCPixmapnode : public MCDLlist
{
	MCImage *image;
	Pixmap pixmap;
	MCColor *colors;
	uint2 ncolors;
	uint2 refcount;
public:
	MCPixmapnode(MCImage *isource);
	~MCPixmapnode();
	
	// MW-2009-02-02: [[ Improved image search ]]
	// Previously, the MCPixmapnodes were keyed on image id, however they are now
	// keyed on MCImage*'s since ids are not necessarily unique.
	Boolean allocpixmap(MCImage* image, Pixmap &opat);
	Boolean freepixmap(Pixmap ipat);
	Boolean unreferenced();
	
	MCPixmapnode *next()
	{
		return (MCPixmapnode *)MCDLlist::next();
	}
	MCPixmapnode *prev()
	{
		return (MCPixmapnode *)MCDLlist::prev();
	}
	void totop(MCPixmapnode *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCPixmapnode *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCPixmapnode *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCPixmapnode *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCPixmapnode *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCPixmapnode *remove(MCPixmapnode *&list)
	{
		return (MCPixmapnode *)MCDLlist::remove((MCDLlist *&)list);
	}
};

class MCPixmaplist
{
	MCPixmapnode *pixmaps;
public:
	MCPixmaplist();
	~MCPixmaplist();
	Pixmap allocpat(uint4 id, MCObject *optr);
	void freepat(Pixmap &pat);
};
#endif
