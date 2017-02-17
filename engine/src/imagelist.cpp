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

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

// IM-2013-08-14: [[ ResIndependence ]] MCPattern struct which associates an image with a scale
struct MCPattern
{
	MCImageRep *source;
	MCGAffineTransform transform;
	MCGImageFilter filter;
	
	MCGImageRef image;
	
	struct
	{
		MCGImageRef image;
		MCGAffineTransform transform;
		MCGFloat x_scale;
		MCGFloat y_scale;
	} cache;
	
	uint32_t references;
};

// IM-2013-08-14: [[ ResIndependence ]] pattern create / retain / release functions

bool MCPatternCreate(const MCGRaster &p_raster, MCGFloat p_scale, MCGImageFilter p_filter, MCPatternRef &r_pattern)
{
	bool t_success;
	t_success = true;
	
	MCPatternRef t_pattern;
	t_pattern = nil;
	
	t_success = MCMemoryNew(t_pattern);
	
	MCGImageRef t_image;
	t_image = nil;
	
	if (t_success)
		t_success = MCGImageCreateWithRaster(p_raster, t_image);
	
	if (t_success)
	{
		t_pattern -> image = MCGImageRetain(t_image);
		t_pattern -> transform = MCGAffineTransformMakeScale(p_scale, p_scale);
		t_pattern -> filter = p_filter;
		t_pattern -> references = 1;
		
		r_pattern = t_pattern;
	}
	else
	{
		MCMemoryDelete(t_pattern);
	}
	
	MCGImageRelease(t_image);
	
	return t_success;
}

bool MCPatternCreate(MCImageRep *p_source, MCGAffineTransform p_transform, MCGImageFilter p_filter, MCPatternRef &r_pattern)
{
	bool t_success;
	t_success = true;
	
	MCPatternRef t_pattern;
	t_pattern = nil;
	
	t_success = MCMemoryNew(t_pattern);
	
	if (t_success)
	{
		t_pattern->source = p_source->Retain();
		t_pattern->transform = p_transform;
		t_pattern->filter = p_filter;
		t_pattern->references = 1;
		
		r_pattern = t_pattern;
	}
	
	return t_success;
}

MCPatternRef MCPatternRetain(MCPatternRef p_pattern)
{
	if (p_pattern == nil)
		return nil;
	
    p_pattern -> references += 1;
    
	return p_pattern;
}

void MCPatternRelease(MCPatternRef p_pattern)
{
	if (p_pattern == nil)
		return;
	
    p_pattern -> references -= 1;
    if (p_pattern -> references == 0)
    {
		MCGImageRelease(p_pattern->image);
		MCGImageRelease(p_pattern->cache.image);
		if (p_pattern->source != nil)
			p_pattern->source->Release();
		MCMemoryDelete(p_pattern);
	}
}

bool MCPatternIsOpaque(MCPatternRef p_pattern)
{
	if (p_pattern == nil)
		return false;
	
	if (p_pattern->image != nil)
		return MCGImageIsOpaque(p_pattern->image);
	
	/* TODO - determine if source rep is opaque */
	return false;
}

MCGAffineTransform MCPatternGetTransform(MCPatternRef p_pattern)
{
    if (p_pattern == nil)
        return MCGAffineTransformMakeIdentity();
    
    return p_pattern->transform;
}

MCImageRep *MCPatternGetSource(MCPatternRef p_pattern)
{
    if (p_pattern == nil)
        return nil;
    
    return p_pattern->source;
}

bool MCPatternGetGeometry(MCPatternRef p_pattern, uint32_t &r_width, uint32_t &r_height)
{
	if (p_pattern == nil)
		return false;
	
	MCGAffineTransform t_transform;
	t_transform = p_pattern->transform;
	
	uindex_t t_src_width, t_src_height;
	
	if (p_pattern->image != nil)
	{
		t_src_width = (uindex_t)MCGImageGetWidth(p_pattern->image);
		t_src_height = (uindex_t)MCGImageGetHeight(p_pattern->image);
	}
	else
	{
		if (!p_pattern->source->GetGeometry(t_src_width, t_src_height))
			return false;
	}
	
	
	MCGRectangle t_rect;
	t_rect = MCGRectangleApplyAffineTransform(MCGRectangleMake(0, 0, t_src_width, t_src_height), t_transform);

	r_width = (uint32_t)ceilf(t_rect.size.width);
	r_height = (uint32_t)ceilf(t_rect.size.height);
	
	return true;
}

bool MCPatternGetFilter(MCPatternRef p_pattern, MCGImageFilter &r_filter)
{
	if (p_pattern == nil)
		return false;

	r_filter = p_pattern->filter;

	return true;
}

bool MCPatternLockForContextTransform(MCPatternRef p_pattern, const MCGAffineTransform &p_transform, MCGImageRef &r_image, MCGAffineTransform &r_pattern_transform)
{
	if (p_pattern == nil)
		return false;
	
	MCGImageRef t_image;
	t_image = nil;
	
	MCGImageFrame t_frame;
	
	MCGAffineTransform t_transform;
	
	bool t_success = true;
	
	if (p_pattern->image != nil)
	{
		t_image = MCGImageRetain(p_pattern->image);
		t_transform = p_pattern->transform;
	}
	else
	{
		MCGAffineTransform t_combined;
		t_combined = MCGAffineTransformConcat(p_pattern->transform, p_transform);
		
		MCGFloat t_scale;
		t_scale = MCGAffineTransformGetEffectiveScale(t_combined);
		
        bool t_locked;
		t_locked = p_pattern->source->LockImageFrame(0, t_scale, t_frame);
		
		t_success = t_locked;
		
		if (t_success)
		{
			t_transform = MCGAffineTransformMakeScale(1.0f / t_frame.x_scale, 1.0f / t_frame.y_scale);
			
			if (!MCGAffineTransformIsRectangular(p_pattern->transform))
			{
				if (p_pattern->cache.x_scale != t_frame.x_scale || p_pattern->cache.y_scale != t_frame.y_scale)
				{
					MCGImageRelease(p_pattern->cache.image);
					p_pattern->cache.image = nil;
				}
				if (p_pattern->cache.image == nil)
				{
					MCImageBitmap *t_bitmap;
					t_bitmap = nil;
					
					MCGAffineTransform t_copy_transform;
					t_copy_transform = MCGAffineTransformConcat(p_pattern->transform, t_transform);
					t_copy_transform = MCGAffineTransformConcat(MCGAffineTransformInvert(t_transform), t_copy_transform);
					
					t_success = MCImageBitmapCreateWithTransformedMCGImage(t_frame.image, t_copy_transform, p_pattern->filter, nil, t_bitmap);
					
					if (t_success)
						t_success = MCImageBitmapCopyAsMCGImageAndRelease(t_bitmap, true, t_image);
					
					if (t_success)
					{
						p_pattern->cache.image = t_image;
						p_pattern->cache.transform = t_transform;
						p_pattern->cache.x_scale = t_frame.x_scale;
						p_pattern->cache.y_scale = t_frame.y_scale;
					}
					
					MCImageFreeBitmap(t_bitmap);
				}
				
				if (t_success)
				{
					t_image = MCGImageRetain(p_pattern->cache.image);
					t_transform = p_pattern->cache.transform;
				}
			}
			else
			{
				// return image & transform scaled for image density
				t_transform = MCGAffineTransformConcat(p_pattern->transform, t_transform);
				
				t_image = MCGImageRetain(t_frame.image);
			}
            if (t_locked)
                p_pattern->source->UnlockImageFrame(0, t_frame);
		}
	}
	
	if (t_success)
	{
		r_image = t_image;
		r_pattern_transform = t_transform;
	}
	
	return t_success;
}

void MCPatternUnlock(MCPatternRef p_pattern, MCGImageRef p_locked_image)
{
	if (p_pattern == nil)
		return;
	
    MCGImageRelease(p_locked_image);
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
	
	MCPatternRef pat = nullptr;
	MCImageListNode *tptr = images;
	if (tptr != nil)
		do
		{
			if (tptr->allocimage(newim, pat))
				return pat;
			tptr = tptr->next();
		}
	while (tptr != images);
	
	/* UNCHECKED */ tptr = new (nothrow) MCImageListNode(newim);
	tptr->appendto(images);
	/* UNCHECKED */ tptr->allocimage(newim, pat);
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
