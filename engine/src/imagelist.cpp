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

////////////////////////////////////////////////////////////////////////////////

// IM-2013-08-14: [[ ResIndependence ]] pattern create / retain / release functions

bool MCPatternCreate(MCGImageRef p_image, MCGFloat p_scale, MCPatternRef &r_pattern)
{
	bool t_success;
	t_success = true;
	
	MCPatternRef t_pattern;
	t_pattern = nil;
	
	t_success = MCMemoryNew(t_pattern);
	
	if (t_success)
	{
		t_pattern -> image = MCGImageRetain(p_image);
		t_pattern -> scale = p_scale;
		t_pattern -> references = 1;
		
		r_pattern = t_pattern;
	}
	else
	{
		MCMemoryDelete(t_pattern);
	}
	
	return t_success;
}

MCPatternRef MCPatternRetain(MCPatternRef p_pattern)
{
	if (p_pattern == nil)
		return nil;
	
	p_pattern->references++;
	
	return p_pattern;
}

void MCPatternRelease(MCPatternRef p_pattern)
{
	if (p_pattern == nil)
		return;
	
	p_pattern->references--;
	if (p_pattern->references == 0)
	{
		MCGImageRelease(p_pattern->image);
		MCMemoryDelete(p_pattern);
	}
}

////////////////////////////////////////////////////////////////////////////////

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
	MCPatternRelease(image);
}

bool MCImageListNode::allocimage(MCImage* p_source, MCPatternRef &r_image)
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

bool MCImageListNode::freeimage(MCPatternRef p_image)
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

MCPatternRef MCImageList::allocpat(uint4 id, MCObject *optr)
{
	// MW-2009-02-02: [[ Improved image search ]]
	// Search for the appropriate image object using the standard method - here we
	// use the given object context as the starting point.
	MCImage *newim;
	newim = optr -> resolveimageid(id);
	if (newim == nil)
		return nil;
	
	MCPatternRef pat;
	MCImageListNode *tptr = images;
	if (tptr != nil)
		do
		{
			if (tptr->allocimage(newim, pat))
				return pat;
			tptr = tptr->next();
		}
	while (tptr != images);
	
	/* UNCHECKED */ tptr = new MCImageListNode(newim);
	tptr->appendto(images);
	tptr->allocimage(newim, pat);
	return pat;
}

void MCImageList::freepat(MCPatternRef &p_image)
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
