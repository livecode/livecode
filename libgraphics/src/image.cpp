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

bool MCGImageCreateWithRaster(const MCGRaster& p_raster, MCGImageRef& r_image)
{
	bool t_success;
	t_success = true;
	
	__MCGImage *t_image;
	t_image = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_image);
	
	SkBitmap *t_bitmap;
	if (t_success)
	{
		t_bitmap = new SkBitmap();
		t_success = t_bitmap != NULL;
	}
	
	if (t_success)
		t_success = MCGRasterToSkBitmap(p_raster, true, *t_bitmap);
	
	if (t_success)
	{
		t_image -> bitmap = t_bitmap;
		t_image -> is_valid = true;
		t_image -> references = 1;
		r_image = t_image;
	}
	else
	{
		if (t_bitmap != NULL)
			delete t_bitmap;
		MCMemoryDelete(t_image);
	}	
	
	return t_success;	
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