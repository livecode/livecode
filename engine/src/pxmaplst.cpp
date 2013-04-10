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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "pxmaplst.h"

#include "globals.h"

// MW-2009-02-02: [[ Improved image search ]]
// The pixmap list provides a reference counted wrapper around Pixmaps that have been
// derived from resolved images. In order to provide the improved image search, we
// need to key on the actual image object rather than the id, since we may have patterns
// set to the same id which resolve to different image objects.

// At some point we could replace this entire thing with a reference counted Pattern
// object and use a cache to do the mapping.

MCPixmapnode::MCPixmapnode(MCImage *iimage)
{
	image = iimage;
	refcount = 0;
}

MCPixmapnode::~MCPixmapnode()
{
	MCscreen->freepixmap(pixmap);
	delete colors;
}

Boolean MCPixmapnode::allocpixmap(MCImage* p_image, Pixmap &opixmap)
{
	if (p_image == image)
	{
		if (refcount == 0)
			image->createpat(pixmap, colors, ncolors);
		refcount++;
		opixmap = pixmap;
		return True;
	}
	else
		return False;
}

Boolean MCPixmapnode::freepixmap(Pixmap ipixmap)
{
	if (pixmap == ipixmap)
	{
		refcount--;
		return True;
	}
	else
		return False;
}

Boolean MCPixmapnode::unreferenced()
{
	if (refcount == 0)
		return True;
	else
		return False;
}

MCPixmaplist::MCPixmaplist()
{
	pixmaps = NULL;
}

MCPixmaplist::~MCPixmaplist()
{
	while (pixmaps != NULL)
	{
		MCPixmapnode *tptr = pixmaps->remove(pixmaps);
		delete tptr;
	}
}

Pixmap MCPixmaplist::allocpat(uint4 id, MCObject *optr)
{
	// MW-2009-02-02: [[ Improved image search ]]
	// Search for the appropriate image object using the standard method - here we
	// use the given object context as the starting point.
	MCImage *newim;
	newim = optr -> resolveimageid(id);
	if (newim == NULL)
		return DNULL;
	
	Pixmap pat;
	MCPixmapnode *tptr = pixmaps;
	if (tptr != NULL)
		do
		{
			if (tptr->allocpixmap(newim, pat))
				return pat;
			tptr = tptr->next();
		}
	while (tptr != pixmaps);
	
	tptr = new MCPixmapnode(newim);
	tptr->appendto(pixmaps);
	tptr->allocpixmap(newim, pat);
	return pat;
}

void MCPixmaplist::freepat(Pixmap &oldpm)
{
	if (oldpm == DNULL)
		return;
	MCPixmapnode *tptr = pixmaps;
	if (tptr != NULL)
		do
		{
			if (tptr->freepixmap(oldpm))
			{
				oldpm = DNULL;
				if (tptr->unreferenced())
				{
					tptr->remove(pixmaps);
					delete tptr;
				}
				return;
			}
			tptr = tptr->next();
		}
		while (tptr != pixmaps);
}
