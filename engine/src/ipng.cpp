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

#include "prefix.h"

#include "globdefs.h"
#include "png.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "uidc.h"
#include "util.h"
#include "image.h"

#include "globals.h"

#include "imageloader.h"

#define NATIVE_ALPHA_BEFORE ((kMCGPixelFormatNative & kMCGPixelAlphaPositionFirst) == kMCGPixelAlphaPositionFirst)

#define NATIVE_ORDER_BGR ((kMCGPixelFormatNative & kMCGPixelOrderRGB) == 0)

#if NATIVE_ALPHA_BEFORE
#define MCPNG_FILLER_POSITION PNG_FILLER_BEFORE
#else
#define MCPNG_FILLER_POSITION PNG_FILLER_AFTER
#endif

extern "C" void fakeread(png_structp png_ptr, png_bytep data, png_size_t length)
{
	uint8_t **t_data_ptr = (uint8_t**)png_get_io_ptr(png_ptr);
	memcpy(data, *t_data_ptr, length);
	*t_data_ptr += length;
}

extern "C" void fakeflush(png_structp png_ptr)
{}

bool MCImageValidatePng(const void *p_data, uint32_t p_length, uint16_t& r_width, uint16_t& r_height)
{
	png_structp png_ptr;
	png_ptr = png_create_read_struct((char *)PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	png_infop info_ptr;
	info_ptr = png_create_info_struct(png_ptr);

	png_infop end_info_ptr;
	end_info_ptr = png_create_info_struct(png_ptr);

	// If we return to this point, its an error.
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info_ptr);
		return false;
	}

	png_set_read_fn(png_ptr, &p_data, fakeread);
	png_read_info(png_ptr, info_ptr);

	png_uint_32 width, height;
	int interlace_type, compression_type, filter_type, bit_depth, color_type;
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, &compression_type, &filter_type);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info_ptr);

	r_width = (uint16_t)width;
	r_height = (uint16_t)height;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" void stream_read(png_structp png_ptr, png_bytep data, png_size_t length)
{
	IO_handle t_stream = (IO_handle)png_get_io_ptr(png_ptr);

	uint4 t_length;
	t_length = length;
	if (IO_read(data, length, t_stream) != IO_NORMAL)
		png_error(png_ptr, (char *)"pnglib read error");
}

static void MCPNGSetNativePixelFormat(png_structp p_png)
{
#if NATIVE_ORDER_BGR
	png_set_bgr(p_png);
#endif
	
#if NATIVE_ALPHA_BEFORE
	png_set_swap_alpha(p_png);
#endif
}

class MCPNGImageLoader : public MCImageLoader
{
public:
	MCPNGImageLoader(IO_handle p_stream);
	virtual ~MCPNGImageLoader();
	
	virtual MCImageLoaderFormat GetFormat() { return kMCImageFormatPNG; }
	
protected:
	virtual bool LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata);
	virtual bool LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count);
	
private:
	png_structp m_png;
	png_infop m_info;
	png_infop m_end_info;
	
	int m_bit_depth;
	int m_color_type;
};

MCPNGImageLoader::MCPNGImageLoader(IO_handle p_stream) : MCImageLoader(p_stream)
{
	m_png = nil;
	m_info = nil;
	m_end_info = nil;
}

MCPNGImageLoader::~MCPNGImageLoader()
{
	if (m_png != nil)
		png_destroy_read_struct(&m_png, &m_info, &m_end_info);
}

bool MCPNGImageLoader::LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata)
{
	bool t_success = true;
	
	t_success = nil != (m_png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL));

	if (t_success)
		t_success = nil != (m_info = png_create_info_struct(m_png));

	if (t_success)
		t_success = nil != (m_end_info = png_create_info_struct(m_png));

	if (t_success)
	{
		if (setjmp(png_jmpbuf(m_png)))
		{
			t_success = false;
		}
	}

	if (t_success)
	{
		png_set_read_fn(m_png, GetStream(), stream_read);
		png_read_info(m_png, m_info);
	}

	png_uint_32 t_width, t_height;
	int t_interlace_method, t_compression_method, t_filter_method;

	if (t_success)
	{
		png_get_IHDR(m_png, m_info, &t_width, &t_height,
			&m_bit_depth, &m_color_type,
			&t_interlace_method, &t_compression_method, &t_filter_method);
	}
    
    // MERG-2014-09-12: [[ ImageMetadata ]] load image metatadata
    if (t_success)
    {
        uint32_t t_X;
        uint32_t t_Y;
        int t_units;
        
        if (png_get_pHYs(m_png, m_info, &t_X, &t_Y, &t_units) && t_units != PNG_RESOLUTION_UNKNOWN)
        {
            MCImageMetadata t_metadata;
            MCMemoryClear(&t_metadata, sizeof(t_metadata));
            t_metadata.has_density = true;
            t_metadata.density = floor(t_X * 0.0254 + 0.5);
            
            r_metadata = t_metadata;
        }
        
    }

	if (t_success)
	{
		r_width = t_width;
		r_height = t_height;
		
		r_xhot = r_yhot = 0;
		r_name = MCValueRetain(kMCEmptyString);
		r_frame_count = 1;
	}

	return t_success;
}

bool MCPNGImageLoader::LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count)
{
	bool t_success = true;
	
	MCBitmapFrame *t_frame;
	t_frame = nil;
	
	MCColorTransformRef t_color_xform;
	t_color_xform = nil;
	
	if (setjmp(png_jmpbuf(m_png)))
	{
		t_success = false;
	}

	int t_interlace_passes;

	uint32_t t_width, t_height;

	if (t_success)
		t_success = GetGeometry(t_width, t_height);
	
	if (t_success)
		t_success = MCMemoryNew(t_frame);

	if (t_success)
		t_success = MCImageBitmapCreate(t_width, t_height, t_frame->image);

	if (t_success)
	{
		bool t_need_alpha = false;

		t_interlace_passes = png_set_interlace_handling(m_png);

		if (m_color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(m_png);
		if (m_color_type == PNG_COLOR_TYPE_GRAY || m_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(m_png);

		if (png_get_valid(m_png, m_info, PNG_INFO_tRNS))
		{
			png_set_tRNS_to_alpha(m_png);
			t_need_alpha = true;
			/* OVERHAUL - REVISIT - assume image has transparent pixels if tRNS is present */
			t_frame->image->has_transparency = true;
		}

		if (m_color_type & PNG_COLOR_MASK_ALPHA)
		{
			t_need_alpha = true;
			/* OVERHAUL - REVISIT - assume image has alpha if color type allows it */
			t_frame->image->has_alpha = t_frame->image->has_transparency = true;
		}
		else if (!t_need_alpha)
			png_set_add_alpha(m_png, 0xFF, MCPNG_FILLER_POSITION);

		if (m_bit_depth == 16)
			png_set_strip_16(m_png);

		MCPNGSetNativePixelFormat(m_png);
	}

	// MW-2009-12-10: Support for color profiles
	// Try to get an embedded ICC profile...
	if (t_success && t_color_xform == nil && png_get_valid(m_png, m_info, PNG_INFO_iCCP))
	{
		png_charp t_ccp_name;
		png_bytep t_ccp_profile;
		int t_ccp_compression_type;
		png_uint_32 t_ccp_profile_length;
		png_get_iCCP(m_png, m_info, &t_ccp_name, &t_ccp_compression_type, &t_ccp_profile, &t_ccp_profile_length);
		
		MCColorSpaceInfo t_csinfo;
		t_csinfo . type = kMCColorSpaceEmbedded;
		t_csinfo . embedded . data = t_ccp_profile;
		t_csinfo . embedded . data_size = t_ccp_profile_length;
		t_color_xform = MCscreen -> createcolortransform(t_csinfo);
	}

	// Next try an sRGB style profile...
	if (t_success && t_color_xform == nil && png_get_valid(m_png, m_info, PNG_INFO_sRGB))
	{
		int t_intent;
		png_get_sRGB(m_png, m_info, &t_intent);

		MCColorSpaceInfo t_csinfo;
		t_csinfo . type = kMCColorSpaceStandardRGB;
		t_csinfo . standard . intent = (MCColorSpaceIntent)t_intent;
		t_color_xform = MCscreen -> createcolortransform(t_csinfo);
	}

	// Finally try for cHRM + gAMA...
	if (t_success && t_color_xform == nil && png_get_valid(m_png, m_info, PNG_INFO_cHRM) &&
		png_get_valid(m_png, m_info, PNG_INFO_gAMA))
	{
		MCColorSpaceInfo t_csinfo;
		t_csinfo . type = kMCColorSpaceCalibratedRGB;
		png_get_cHRM(m_png, m_info,
			&t_csinfo . calibrated . white_x, &t_csinfo . calibrated . white_y,
			&t_csinfo . calibrated . red_x, &t_csinfo . calibrated . red_y,
			&t_csinfo . calibrated . green_x, &t_csinfo . calibrated . green_y,
			&t_csinfo . calibrated . blue_x, &t_csinfo . calibrated . blue_y);
		png_get_gAMA(m_png, m_info, &t_csinfo . calibrated . gamma);
		t_color_xform = MCscreen -> createcolortransform(t_csinfo);
	}

	// Could not create any kind, so fallback to gamma transform.
	if (t_success && t_color_xform == nil)
	{
		double image_gamma;
		if (png_get_gAMA(m_png, m_info, &image_gamma))
			png_set_gamma(m_png, MCgamma, image_gamma);
		else
			png_set_gamma(m_png, MCgamma, 0.45);
	}

	if (t_success)
	{
		for (uindex_t t_pass = 0; t_pass < t_interlace_passes; t_pass++)
		{
			png_bytep t_data_ptr = (png_bytep)t_frame->image->data;
			for (uindex_t i = 0; i < t_height; i++)
			{
				png_read_row(m_png, t_data_ptr, nil);
				t_data_ptr += t_frame->image->stride;
			}
		}
	}

	if (t_success)
		png_read_end(m_png, m_end_info);

	// transform colours using extracted colour profile
	if (t_success && t_color_xform != nil)
		MCImageBitmapApplyColorTransform(t_frame->image,  t_color_xform);

	if (t_color_xform != nil)
		MCscreen -> destroycolortransform(t_color_xform);

	if (t_success)
	{
		r_frames = t_frame;
		r_count = 1;
	}
	else
		MCImageFreeFrames(t_frame, 1);

	return t_success;
}

bool MCImageLoaderCreateForPNGStream(IO_handle p_stream, MCImageLoader *&r_loader)
{
	MCPNGImageLoader *t_loader;
	t_loader = new (nothrow) MCPNGImageLoader(p_stream);
	
	if (t_loader == nil)
		return false;
	
	r_loader = t_loader;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// embed stream within wrapper struct containing byte count
// so we can update byte_count as we go
struct MCPNGWriteContext
{
	IO_handle stream;
	uindex_t byte_count;
};

extern "C" void fakewrite(png_structp png_ptr, png_bytep data, png_size_t length)
{
	MCPNGWriteContext *t_context = (MCPNGWriteContext*)png_get_io_ptr(png_ptr);
	if (IO_write(data, sizeof(uint1), length, t_context->stream) != IO_NORMAL)
		png_error(png_ptr, (char *)"pnglib write error");
	t_context->byte_count += length;
}

// MERG-2014-07-16: [[ ImageMetadata ]] Parse the metadata array
static void parsemetadata(png_structp png_ptr, png_infop info_ptr, MCImageMetadata *p_metadata)
{
    if (p_metadata == nil)
        return;
    
    if (p_metadata -> has_density)
    {
        real64_t t_ppi = p_metadata -> density;
        if (t_ppi > 0)
        {
            // Convert to pixels per metre from pixels per inch
            png_set_pHYs(png_ptr, info_ptr, t_ppi / 0.0254, t_ppi / 0.0254, PNG_RESOLUTION_METER);
        }
    }
}

bool MCImageEncodePNG(MCImageIndexedBitmap *p_indexed, MCImageMetadata *p_metadata, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;

	MCPNGWriteContext t_context;
	t_context.stream = p_stream;
	t_context.byte_count = 0;

	png_structp t_png_ptr = nil;
	png_infop t_info_ptr = nil;
	png_color *t_png_palette = nil;
	png_byte *t_png_transparency = nil;

	png_bytep t_data_ptr = nil;
	uindex_t t_stride = 0;

	/*init png stuff*/
	if (t_success)
	{
		t_success = nil != (t_png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
			(png_voidp)NULL, (png_error_ptr)NULL,
			(png_error_ptr)NULL));
	}

	if (t_success)
		t_success = nil != (t_info_ptr = png_create_info_struct(t_png_ptr));

	/*in case of png error*/
	if (setjmp(png_jmpbuf(t_png_ptr)))
		t_success = false;

	if (t_success)
		png_set_write_fn(t_png_ptr,(png_voidp)&t_context,fakewrite,fakeflush);

	if (t_success)
	{
		png_set_IHDR(t_png_ptr, t_info_ptr, p_indexed->width, p_indexed->height, 8,
					 PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
					 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_set_gAMA(t_png_ptr, t_info_ptr, 1/MCgamma);
	}

    // MERG-2014-07-16: [[ ImageMetadata ]] Parse the metadata array
    if (t_success)
        parsemetadata(t_png_ptr, t_info_ptr, p_metadata);
    
	if (t_success)
		t_success = MCMemoryNewArray(p_indexed->palette_size, t_png_palette);

	/*create palette for 8 bit*/
	if (t_success)
	{
		for (uindex_t i = 0; i < p_indexed->palette_size ; i++)
		{
			t_png_palette[i].red   = p_indexed->palette[i].red >> 8;
			t_png_palette[i].green = p_indexed->palette[i].green >> 8;
			t_png_palette[i].blue  = p_indexed->palette[i].blue >> 8;
		}

		png_set_PLTE(t_png_ptr, t_info_ptr, t_png_palette, p_indexed->palette_size);
	}

	if (MCImageIndexedBitmapHasTransparency(p_indexed))
	{
		if (t_success)
			t_success = MCMemoryAllocate(p_indexed->palette_size, t_png_transparency);
		if (t_success)
		{
			memset(t_png_transparency, 0xFF, p_indexed->palette_size);
			t_png_transparency[p_indexed->transparent_index] = 0x00;
			png_set_tRNS(t_png_ptr, t_info_ptr, t_png_transparency, p_indexed->palette_size, NULL);
		}
	}
	if (t_success)
		png_write_info(t_png_ptr, t_info_ptr);

	if (t_success)
	{
		t_data_ptr = (png_bytep)p_indexed->data;
		t_stride = p_indexed->stride;
	}

	if (t_success)
	{
		for (uindex_t i = 0; i < p_indexed->height; i++)
		{
			png_write_row(t_png_ptr, t_data_ptr);
			t_data_ptr += t_stride;
		}
	}

	if (t_success)
		png_write_end(t_png_ptr, t_info_ptr);

	if (t_png_ptr != nil)
		png_destroy_write_struct(&t_png_ptr, &t_info_ptr);
	if (t_png_palette != nil)
		MCMemoryDeleteArray(t_png_palette);
	if (t_png_transparency != nil)
		MCMemoryDeallocate(t_png_transparency);

	if (t_success)
		r_bytes_written = t_context.byte_count;

	return t_success;
}

bool MCImageEncodePNG(MCImageBitmap *p_bitmap, MCImageMetadata *p_metadata, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;

	MCPNGWriteContext t_context;
	t_context.stream = p_stream;
	t_context.byte_count = 0;

	png_structp t_png_ptr = nil;
	png_infop t_info_ptr = nil;
	png_color *t_png_palette = nil;
	png_byte *t_png_transparency = nil;

	png_bytep t_data_ptr = nil;
	uindex_t t_stride = 0;

	MCImageIndexedBitmap *t_indexed = nil;
	if (MCImageConvertBitmapToIndexed(p_bitmap, false, t_indexed))
	{
		t_success = MCImageEncodePNG(t_indexed, p_metadata, p_stream, r_bytes_written);
		MCImageFreeIndexedBitmap(t_indexed);
		return t_success;
	}

	/*init png stuff*/
	if (t_success)
	{
		t_success = nil != (t_png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
			(png_voidp)NULL, (png_error_ptr)NULL,
			(png_error_ptr)NULL));
	}

	if (t_success)
		t_success = nil != (t_info_ptr = png_create_info_struct(t_png_ptr));

	/*in case of png error*/
	if (setjmp(png_jmpbuf(t_png_ptr)))
		t_success = false;

	if (t_success)
		png_set_write_fn(t_png_ptr,(png_voidp)&t_context,fakewrite,fakeflush);

	bool t_fully_opaque = true;
	if (t_success)
	{
		t_fully_opaque = !MCImageBitmapHasTransparency(p_bitmap);

		png_set_IHDR(t_png_ptr, t_info_ptr, p_bitmap->width, p_bitmap->height, 8,
			t_fully_opaque ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_set_gAMA(t_png_ptr, t_info_ptr, 1/MCgamma);
	}
    
    // MERG-2014-07-16: [[ ImageMetadata ]] Parse the metadata array
    if (t_success)
        parsemetadata(t_png_ptr, t_info_ptr, p_metadata);

	if (t_success)
	{
		png_write_info(t_png_ptr, t_info_ptr);

		if (t_fully_opaque)
			png_set_filler(t_png_ptr, 0, MCPNG_FILLER_POSITION);
		
		MCPNGSetNativePixelFormat(t_png_ptr);
	}

	if (t_success)
	{
		t_data_ptr = (png_bytep)p_bitmap->data;
		t_stride = p_bitmap->stride;
	}

	if (t_success)
	{
		for (uindex_t i = 0; i < p_bitmap->height; i++)
		{
			png_write_row(t_png_ptr, t_data_ptr);
			t_data_ptr += t_stride;
		}
	}

	if (t_success)
		png_write_end(t_png_ptr, t_info_ptr);

	if (t_png_ptr != nil)
		png_destroy_write_struct(&t_png_ptr, &t_info_ptr);
	if (t_png_palette != nil)
		MCMemoryDeleteArray(t_png_palette);
	if (t_png_transparency != nil)
		MCMemoryDeallocate(t_png_transparency);

	if (t_success)
		r_bytes_written = t_context.byte_count;

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
