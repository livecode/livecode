#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "imagelist.h"

#include "globals.h"

// MW-2009-02-02: [[ Improved image search ]]
// The pixmap list provides a reference counted wrapper around Pixmaps that have been
// derived from resolved images. In order to provide the improved image search, we
// need to key on the actual image object rather than the id, since we may have patterns
// set to the same id which resolve to different image objects.

// At some point we could replace this entire thing with a reference counted Pattern
// object and use a cache to do the mapping.

MCImageListNode::MCImageListNode(MCImage *p_source)
{
	source = p_source;
	image = nil;
	refcount = 0;
}

MCImageListNode::~MCImageListNode()
{
	MCGImageRelease(image);
}

bool MCImageListNode::allocimage(MCImage* p_source, MCGImageRef &r_image)
{
	if (p_source == source)
	{
		if (refcount == 0)
			/* UNCHECKED */ source->createpattern(image);
		refcount++;
		r_image = image;
		return true;
	}
	else
		return false;
}

bool MCImageListNode::freeimage(MCGImageRef p_image)
{
	if (image == p_image)
	{
		refcount--;
		return true;
	}
	else
		return false;
}

bool MCImageListNode::unreferenced()
{
	return refcount == 0;
}

MCImageList::MCImageList()
{
	images = nil;
}

MCImageList::~MCImageList()
{
	while (images != NULL)
	{
		MCImageListNode *t_ptr = images->remove(images);
		delete t_ptr;
	}
}

MCGImageRef MCImageList::allocpat(uint4 id, MCObject *optr)
{
	// MW-2009-02-02: [[ Improved image search ]]
	// Search for the appropriate image object using the standard method - here we
	// use the given object context as the starting point.
	MCImage *newim;
	newim = optr -> resolveimageid(id);
	if (newim == nil)
		return nil;
	
	MCGImageRef pat;
	MCImageListNode *tptr = images;
	if (tptr != nil)
		do
		{
			if (tptr->allocimage(newim, pat))
				return pat;
			tptr = tptr->next();
		}
	while (tptr != images);
	
	tptr = new MCImageListNode(newim);
	tptr->appendto(images);
	tptr->allocimage(newim, pat);
	return pat;
}

void MCImageList::freepat(MCGImageRef &p_image)
{
	if (p_image == nil)
		return;
	MCImageListNode *tptr = images;
	if (tptr != nil)
		do
		{
			if (tptr->freeimage(p_image))
			{
				p_image = nil;
				if (tptr->unreferenced())
				{
					tptr->remove(images);
					delete tptr;
				}
				return;
			}
			tptr = tptr->next();
		}
		while (tptr != images);
}
