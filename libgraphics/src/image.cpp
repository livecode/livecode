#include "graphics.h"
#include "graphics-internal.h"

////////////////////////////////////////////////////////////////////////////////

static void MCGImageDestroy(MCGImageRef self)
{
	if (self != NULL)
	{
		if (self -> bitmap != NULL)
			delete self -> bitmap;
		MCMemoryDelete(self);
	}
}

bool MCGImageCreateWithSkBitmap(const SkBitmap &p_bitmap, MCGImageRef &r_image)
{
	bool t_success;
	t_success = true;
	
	__MCGImage *t_image;
	t_image = nil;
	if (t_success)
		t_success = MCMemoryNew(t_image);
	
	SkBitmap *t_bitmap;
	t_bitmap = nil;
	if (t_success)
	{
		t_bitmap = new SkBitmap(p_bitmap);
		t_success = nil != t_bitmap;
	}
	
	if (t_success)
	{
		t_image -> bitmap = t_bitmap;
		t_image -> is_valid = true;
		t_image -> references = 1;
		r_image = t_image;
	}
	else
	{
		if (t_bitmap != nil)
			delete t_bitmap;
		MCMemoryDelete(t_image);
	}
	
	return t_success;
}

bool MCGImageCreateWithRaster(const MCGRaster &p_raster, MCGPixelOwnershipType p_ownership, MCGImageRef &r_image)
{
	bool t_success;
	t_success = true;
	
	__MCGImage *t_image;
	t_image = nil;
	
	SkBitmap t_bitmap;
	
	if (t_success)
		t_success = MCGRasterToSkBitmap(p_raster, p_ownership, t_bitmap);
	
	if (t_success)
		t_success = MCGImageCreateWithSkBitmap(t_bitmap, t_image);
	
	if (t_success)
		r_image = t_image;
	else
		MCMemoryDelete(t_image);
	
	return t_success;	
}

bool MCGImageCreateWithRasterAndRelease(const MCGRaster &p_raster, MCGImageRef &r_image)
{
	return MCGImageCreateWithRaster(p_raster, kMCGPixelOwnershipTypeTake, r_image);
}

bool MCGImageCreateWithRasterNoCopy(const MCGRaster& p_raster, MCGImageRef& r_image)
{
	return MCGImageCreateWithRaster(p_raster, kMCGPixelOwnershipTypeBorrow, r_image);
}

bool MCGImageCreateWithRaster(const MCGRaster& p_raster, MCGImageRef& r_image)
{
	return MCGImageCreateWithRaster(p_raster, kMCGPixelOwnershipTypeCopy, r_image);
}

bool MCGImageGetRaster(MCGImageRef p_image, MCGRaster &r_raster)
{
	if (p_image == nil)
		return false;

	__MCGImage *t_image = (__MCGImage*)p_image;
	if (!t_image->is_valid || t_image->bitmap == nil)
		return false;

	// IM-2013-06-18: lock pixels here to force skia to update the pixel ptr
	// from the bitmap's pixel ref
	t_image->bitmap->lockPixels();
	
	r_raster.format = MCGRasterFormatFromSkBitmapConfig(t_image->bitmap->config());
	r_raster.width = t_image->bitmap->width();
	r_raster.height = t_image->bitmap->height();
	r_raster.pixels = t_image->bitmap->getPixels();
	r_raster.stride =t_image->bitmap->rowBytes();

	t_image->bitmap->unlockPixels();
	
	return true;
}

bool MCGImageCreateWithData(const void *p_bytes, uindex_t p_byte_count, MCGImageRef& r_image)
{
	// TODO: Implement
	return false;
}

bool MCGImageCreateWithFilename(const char *p_filename, MCGImageRef& r_image)
{
	// TODO: Implement
	return false;
}

////////////////////////////////////////////////////////////////////////////////

MCGImageRef MCGImageRetain(MCGImageRef self)
{
	if (self != NULL)
		self -> references++;
	return self;	
}

void MCGImageRelease(MCGImageRef self)
{
	if (self != NULL)
	{
		self -> references--;
		if (self -> references <= 0)
			MCGImageDestroy(self);
	}	
}

////////////////////////////////////////////////////////////////////////////////

bool MCGImageIsValid(MCGImageRef self)
{
	return self != NULL && 	self -> is_valid;
}

////////////////////////////////////////////////////////////////////////////////

int32_t MCGImageGetWidth(MCGImageRef self)
{
	return self -> bitmap -> width();
}

int32_t MCGImageGetHeight(MCGImageRef self)
{
	return self -> bitmap -> height();
}

MCGSize MCGImageGetSize(MCGImageRef self)
{
	MCGSize t_size;
	t_size . width = 0;
	t_size . height = 0;
	
	if (!MCGImageIsValid(self))
		return t_size;
	
	t_size . width = (MCGFloat) self -> bitmap -> width();
	t_size . height = (MCGFloat) self -> bitmap -> height();	
	return t_size;
}

////////////////////////////////////////////////////////////////////////////////