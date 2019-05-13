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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "sysdefs.h"

#include "globals.h"
#include "object.h"
#include "stack.h"
#include "cdata.h"
#include "objptr.h"
#include "field.h"
#include "object.h"
#include "button.h"
#include "card.h"
#include "exec.h"
#include "util.h"
#include "group.h"
#include "image.h"
#include "graphic.h"

#include "exec-interface.h"
#include "graphics_util.h"
#include "module-canvas.h"

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceImageResizeQualityElementInfo[] =
{
	{ "normal", INTERPOLATION_BOX, false },
	{ "good", INTERPOLATION_BILINEAR, false },
	{ "best", INTERPOLATION_BICUBIC, false },
};

static MCExecEnumTypeInfo _kMCInterfaceImageResizeQualityTypeInfo =
{
	"Interface.ImageResizeQuality",
	sizeof(_kMCInterfaceImageResizeQualityElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceImageResizeQualityElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceImagePaintCompressionElementInfo[] =
{
	{ "png", F_PNG, false },
	{ "jpeg", F_JPEG, false },
	{ "gif", F_GIF, false },
	{ "pict", F_PICT, false },
	{ "rle", F_RLE, false },
};

static MCExecEnumTypeInfo _kMCInterfaceImagePaintCompressionTypeInfo =
{
	"Interface.ImagePaintCompression",
	sizeof(_kMCInterfaceImagePaintCompressionElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceImagePaintCompressionElementInfo
};

//////////

MCExecEnumTypeInfo *kMCInterfaceImageResizeQualityTypeInfo = &_kMCInterfaceImageResizeQualityTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceImagePaintCompressionTypeInfo = &_kMCInterfaceImagePaintCompressionTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCImage::GetXHot(MCExecContext& ctxt, integer_t& r_x)
{
	r_x  = xhot;
}

void MCImage::SetXHot(MCExecContext& ctxt, integer_t p_x)
{
	uint32_t t_pixwidth, t_pixheight;
	getgeometry(t_pixwidth, t_pixheight);
	xhot = MCMax(1, MCMin(xhot, (int32_t)t_pixwidth));
}

void MCImage::GetYHot(MCExecContext& ctxt, integer_t& r_y)
{
	r_y  = yhot;
}

void MCImage::SetYHot(MCExecContext& ctxt, integer_t p_y)
{
	uint32_t t_pixwidth, t_pixheight;
	getgeometry(t_pixwidth, t_pixheight);
	yhot = MCMax(1, MCMin(yhot, (int32_t)t_pixheight));
}

void MCImage::GetHotSpot(MCExecContext& ctxt, MCPoint& r_spot)
{
	r_spot . x = xhot;
	r_spot . y = yhot;
}

void MCImage::SetHotSpot(MCExecContext& ctxt, MCPoint p_spot)
{
	uint32_t t_pixwidth, t_pixheight;
	getgeometry(t_pixwidth, t_pixheight);
    // SN-2014-10-27: [[ Bug 13821 ]] Set the values of the parameter, not xhot and yhot
	xhot = MCMax(1, MCMin(p_spot . x, (int32_t)t_pixwidth));
	yhot = MCMax(1, MCMin(p_spot . y, (int32_t)t_pixheight));
}

void MCImage::GetFileName(MCExecContext& ctxt, MCStringRef& r_name)
{
    if (getflag(F_HAS_FILENAME))
        r_name = MCValueRetain(filename);
    else
        r_name = MCValueRetain(kMCEmptyString);
}

void MCImage::SetFileName(MCExecContext& ctxt, MCStringRef p_name)
{
    if (m_rep && m_rep->IsLocked())
    {
        ctxt . LegacyThrow(EE_IMAGE_MUTABLELOCK);
        return;
    }
    // MW-2013-06-24: [[ Bug 10977 ]] If we are setting the filename to
    //   empty, and the filename is already empty, do nothing.
	if ((m_rep != nil && m_rep->GetType() == kMCImageRepReferenced &&
		MCStringIsEmpty(p_name)) || !MCStringIsEqualTo(p_name, filename, kMCCompareExact))
	{
		setfilename(p_name);
		resetimage();

        // MW-2013-06-25: [[ Bug 10980 ]] Only set the result to an error if we were
        //   attempting to set a non-empty filename.
		if (m_rep != nil || MCStringIsEmpty(p_name))
			ctxt . SetTheResultToEmpty();
		else
			ctxt . SetTheResultToStaticCString("could not open image");
	}
}

void MCImage::GetAlwaysBuffer(MCExecContext& ctxt, bool& r_setting)
{
	r_setting  = getflag(F_I_ALWAYS_BUFFER);
}

void MCImage::SetAlwaysBuffer(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_I_ALWAYS_BUFFER))
		Redraw();
}

void MCImage::GetImagePixmapId(MCExecContext& ctxt, uinteger_t*& r_id)
{
    r_id = nil;
}

void MCImage::SetImagePixmapId(MCExecContext& ctxt, uinteger_t* p_id)
{
}

void MCImage::GetMaskPixmapId(MCExecContext& ctxt, uinteger_t*& r_id)
{
    r_id = nil;
}

void MCImage::SetMaskPixmapId(MCExecContext& ctxt, uinteger_t* p_id)
{
}

void MCImage::GetDontDither(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_DONT_DITHER);
}

void MCImage::SetDontDither(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_DONT_DITHER);
}

void MCImage::GetMagnify(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getstate(CS_MAGNIFY);
}

void MCImage::SetMagnify(MCExecContext& ctxt, bool setting)
{
	if (changestate(setting, CS_MAGNIFY))
	{
		if (setting)
			startmag(rect.width >> 1, rect.height >> 1);
		else
			endmag(True);
		
		Redraw();
	}
}

void MCImage::GetSize(MCExecContext& ctxt, uinteger_t& r_size)
{
	void *t_data = nil;
	uinteger_t t_size = 0;
	MCImageCompressedBitmap *t_compressed = nil;
	
	if (m_rep != nil)
	{
		if (m_rep->GetType() == kMCImageRepResident)
			static_cast<MCResidentImageRep*>(m_rep)->GetData(t_data, t_size);
		else if (m_rep->GetType() == kMCImageRepVector)
			static_cast<MCVectorImageRep*>(m_rep)->GetData(t_data, t_size);
		else if (m_rep->GetType() == kMCImageRepCompressed)
		{
			t_compressed = static_cast<MCCompressedImageRep*>(m_rep)->GetCompressed();
			if (t_compressed->size != 0)
				t_size = t_compressed->size;
			else
			{
				for (uinteger_t i = 0; i < t_compressed->color_count; i++)
					t_size += t_compressed->plane_sizes[i];
			}
		}
	}
	
	r_size = t_size;
}

void MCImage::GetCurrentFrame(MCExecContext& ctxt, uinteger_t& r_frame)
{
	r_frame = currentframe + 1;
}

void MCImage::SetCurrentFrame(MCExecContext& ctxt, uinteger_t p_frame)
{
	setframe(p_frame - 1);
}

void MCImage::GetFrameCount(MCExecContext& ctxt, integer_t& r_count)
{
	r_count = m_rep == nil ? 0 : m_rep->GetFrameCount();
	if (r_count <= 1)
		r_count = 0;
}

void MCImage::GetPalindromeFrames(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_PALINDROME_FRAMES);
}

void MCImage::SetPalindromeFrames(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_PALINDROME_FRAMES))
		Redraw();
}

void MCImage::GetConstantMask(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_CONSTANT_MASK);
}

void MCImage::SetConstantMask(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_CONSTANT_MASK))
		Redraw();
}

void MCImage::GetRepeatCount(MCExecContext& ctxt, integer_t& r_count)
{
	r_count = repeatcount;
}

void MCImage::SetRepeatCount(MCExecContext& ctxt, integer_t p_count)
{
	if (p_count < 0)
		flags &= ~F_REPEAT_COUNT;
	else
		flags |= F_REPEAT_COUNT;
	irepeatcount = repeatcount = p_count;
	if (opened && m_rep != nil && m_rep->GetFrameCount() > 1 && repeatcount != 0)
	{
		setframe((uindex_t) currentframe == m_rep->GetFrameCount() - 1 ? 0 : currentframe + 1);
		
		// IM-2014-11-25: [[ ImageRep ]] Use ImageRep method to get frame duration
		uint32_t t_frame_duration;
		if (m_rep->GetFrameDuration(currentframe, t_frame_duration))
			MCscreen->addtimer(this, MCM_internal, t_frame_duration);
	}
}

void MCImage::GetFormattedHeight(MCExecContext& ctxt, integer_t& r_height)
{
	uindex_t t_width = 0, t_height = 0;
    /* UNCHECKED */ getsourcegeometry(t_width, t_height);
	
	r_height = t_height;
}

void MCImage::GetFormattedWidth(MCExecContext& ctxt, integer_t& r_width)
{
	uindex_t t_width = 0, t_height = 0;
    /* UNCHECKED */ getsourcegeometry(t_width, t_height);
	
	r_width = t_width;
}

void MCImage::GetText(MCExecContext& ctxt, MCDataRef& r_text)
{
    if (m_rep == nullptr || m_rep->GetType() == kMCImageRepReferenced)
    {
        r_text = MCValueRetain(kMCEmptyData);
        return;
    }
    
    bool t_success = false;
    
    MCImage* t_image_to_work_on = this;   
    if (m_rep && m_rep->IsLocked())
    {
        t_image_to_work_on = new MCImage(*this);
    }
    else
    {
        t_image_to_work_on->recompress();
    }
    
    void *t_data = nil;
	uindex_t t_size = 0;
	if (t_image_to_work_on->m_rep->GetType() == kMCImageRepResident)
		static_cast<MCResidentImageRep*>(t_image_to_work_on->m_rep)->GetData(t_data, t_size);
	else if (t_image_to_work_on->m_rep->GetType() == kMCImageRepVector)
		static_cast<MCVectorImageRep*>(t_image_to_work_on->m_rep)->GetData(t_data, t_size);
	else if (t_image_to_work_on->m_rep->GetType() == kMCImageRepCompressed)
	{
		MCImageCompressedBitmap *t_compressed = nil;
		t_compressed = static_cast<MCCompressedImageRep*>(t_image_to_work_on->m_rep)->GetCompressed();
		if (t_compressed->data != nil)
		{
			t_data = t_compressed->data;
			t_size = t_compressed->size;
		}
		else
		{
			t_data = t_compressed->planes[0];
			t_size = t_compressed->plane_sizes[0];
		}
	}
	
    if (MCDataCreateWithBytes((const byte_t *)t_data, t_size, r_text))
    {
        t_success = true;
    }

    if (t_image_to_work_on != this)
    {
        delete t_image_to_work_on;
    }
    if (!t_success)
    {
        ctxt . Throw();
        
    }
}


void MCImage::SetText(MCExecContext& ctxt, MCDataRef p_text)
{
    if (m_rep && m_rep->IsLocked())
    {
        ctxt . LegacyThrow(EE_IMAGE_MUTABLELOCK);
        return;
    }
	
    bool t_success = true;
	
	MCImageBitmap *t_bitmap = nil;
	MCImageCompressedBitmap *t_compressed = nil;
	MCPoint t_hotspot;
	MCStringRef t_name = nil;
	IO_handle t_stream = nil;
    MCAutoDataRef t_data;
	
	if (MCDataGetLength(p_text) == 0)
	{
        // MERG-2013-06-24: [[ Bug 10977 ]] If we have a filename then setting the
        //   text to empty shouldn't have an effect; otherwise we are unsetting the
        //   current text.
        if (!getflag(F_HAS_FILENAME))
        {
            // empty text - unset flags & set rep to nil;
            flags &= ~(F_COMPRESSION | F_TRUE_COLOR | F_HAS_FILENAME);
            setrep(nil);
        }
	}
	else
	{
		if (t_success)
            t_success = nil != (t_stream = MCS_fakeopen(MCDataGetBytePtr(p_text), MCDataGetLength(p_text)));
		if (t_success)
			t_success = MCImageImport(t_stream, nil, t_hotspot, t_name, t_compressed, t_bitmap);
		if (t_success)
		{
			if (t_compressed != nil)
				t_success = setcompressedbitmap(t_compressed);
			else if (t_bitmap != nil)
				t_success = setbitmap(t_bitmap, 1.0);
		}
		
		MCImageFreeBitmap(t_bitmap);
		MCImageFreeCompressedBitmap(t_compressed);
		MCValueRelease(t_name);
		if (t_stream != nil)
			MCS_close(t_stream);
	}
	
	if (t_success)
		resetimage();
}

void MCImage::GetImageData(MCExecContext& ctxt, MCDataRef& r_data)
{
    if (m_rep && m_rep->GetType() != kMCImageRepMutable && m_rep->IsLocked())
    {
        ctxt . LegacyThrow(EE_IMAGE_MUTABLELOCK);
        return;
    }
    
    // IM-2013-02-07: image data must return a block of data, even if the image is empty.
	// image data should always be for an image the size of the rect.
    uint32_t t_pixel_count = rect.width * rect.height;
    uint32_t t_data_size = t_pixel_count * sizeof(uint32_t);
    
    MCAutoByteArray t_buffer;
    
    bool t_success = true;
    
    t_success = t_buffer.New(t_data_size);
    
    if (t_success)
    {
        uint32_t *t_data_ptr = (uint32_t*)t_buffer.Bytes();
        if (m_rep == nil)
            MCMemoryClear(t_data_ptr, t_data_size);
        else
        {
            // SN-2014-01-31: [[ Bug 11462 ]] Opening an image to get its data should not
            // reset its size: F_LOCK_LOCATION ensures the size - and the location, which
            // doesn't matter here - are read as they are stored.
            bool t_tmp_locked;
            t_tmp_locked = false;
            
            MCImageBitmap *t_bitmap = nil;
            
            if (m_rep->GetType() == kMCImageRepMutable)
            {
                t_success = static_cast<MCMutableImageRep *>(m_rep)->LockBitmap(0, 0, t_bitmap);
            }
            else
            {
                if (!getflag(F_LOCK_LOCATION))
                {
                    setflag(true, F_LOCK_LOCATION);
                    t_tmp_locked = true;
                }
                openimage();
                t_success = lockbitmap(t_bitmap, false);
            }
            // IM-2014-09-02: [[ Bug 13295 ]] Call lockbitmap() insted of copybitmap() to avoid unnecessary copy
            
            if (t_success)
            {
                MCMemoryCopy(t_data_ptr, t_bitmap->data, t_data_size);
                // IM-2013-09-16: [[ RefactorGraphics ]] [[ Bug 11185 ]] Use correct pixel format (xrgb) for imagedata
#if (kMCGPixelFormatNative != kMCGPixelFormatARGB)
                while (t_pixel_count--)
                {
                    uint8_t t_r, t_g, t_b, t_a;
                    MCGPixelUnpackNative(*t_data_ptr, t_r, t_g, t_b, t_a);
                    *t_data_ptr++ = MCGPixelPack(kMCGPixelFormatARGB, t_r, t_g, t_b, t_a);
                }
#endif
            }
            
            if (m_rep->GetType() == kMCImageRepMutable)
            {
                static_cast<MCMutableImageRep *>(m_rep)->UnlockBitmap(0, t_bitmap);
            }
            else
            {
                unlockbitmap(t_bitmap);
                if (t_tmp_locked)
                    setflag(false, F_LOCK_LOCATION);
                closeimage();
            }
        }
    }
	if (t_success)
    {
        t_success = t_buffer.CreateDataAndRelease(r_data);
        return;
    }
    
    ctxt . Throw();
}

void MCImage::SetImageData(MCExecContext& ctxt, MCDataRef p_data)
{
    if (m_rep && m_rep->IsLocked())
    {
        ctxt . LegacyThrow(EE_IMAGE_MUTABLELOCK);
        return;
    }
	
    uindex_t t_length;
	t_length = MCDataGetLength(p_data);
	if (t_length != 0)
	{
		bool t_success = true;
		
		MCImageBitmap *t_copy = nil;
        if (m_rep != nil)
        {
            t_success = copybitmap(false, t_copy);
        }
		else
		{
			t_success = MCImageBitmapCreate(rect.width, rect.height, t_copy);
			if (t_success)
                MCImageBitmapSet(t_copy, MCGPixelPackNative(0, 0, 0, 255)); // set to opaque black
		}
		
		if (t_success)
		{
			uint32_t t_stride = MCMin(t_length / t_copy->height, t_copy->width * 4);
			uint32_t t_width = t_stride / 4;
			
			uint8_t *t_src_ptr = (uint8_t*)MCDataGetBytePtr(p_data);
			uint8_t *t_dst_ptr = (uint8_t*)t_copy->data;
			for (uindex_t y = 0; y < t_copy->height; y++)
			{
                uint32_t *t_src_row = (uint32_t*)t_src_ptr;
                uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
				for (uindex_t x = 0; x < t_width; x++)
				{
					uint8_t a, r, g, b;
                    MCGPixelUnpack(kMCGPixelFormatARGB, *t_src_row++, r, g, b, a);
                    
                    // IM-2013-10-25: [[ Bug 11314 ]] Preserve current alpha values when setting the imagedata
                    *t_dst_row = MCGPixelPackNative(r, g, b, MCGPixelGetNativeAlpha(*t_dst_row));
                    t_dst_row++;
				}
				t_src_ptr += t_stride;
				t_dst_ptr += t_copy->stride;
			}
			
			setbitmap(t_copy, 1.0);
		}
		
		MCImageFreeBitmap(t_copy);
		
		resetimage();
	}
}

void MCImage::GetTransparencyData(MCExecContext &ctxt, bool p_flatten, MCDataRef &r_data)
{
    if (m_rep && m_rep->GetType() != kMCImageRepMutable && m_rep->IsLocked())
    {
        ctxt . LegacyThrow(EE_IMAGE_MUTABLELOCK);
        return;
    }
    
    uint32_t t_pixel_count = rect.width * rect.height;
	uint32_t t_data_size = t_pixel_count;
    
	bool t_success = true;
    
	MCAutoByteArray t_buffer;
	
	t_success = t_buffer.New(t_data_size);
	
	if (t_success)
	{
		uint8_t *t_data_ptr = (uint8_t*)t_buffer.Bytes();
		if (m_rep == nil)
			MCMemoryClear(t_data_ptr, t_data_size);
		else
        {
            // SN-2014-07-09: [[ MERGE-6.7 ]] Apply Ian's fix in the exec file
            // IM-2014-06-18: [[ Bug 12646 ]] Apply Seb's fix here too.
            // SN-2014-01-31: [[ Bug 11462 ]] Opening an image to get its data should not
            // reset its size: F_LOCK_LOCATION ensures the size - and the location, which
            // doesn't matter here - are read as they are stored.
            bool t_tmp_locked;
            t_tmp_locked = false;
            
            MCImageBitmap *t_bitmap = nil;
            
            if (m_rep->GetType() == kMCImageRepMutable)
            {
                t_success = static_cast<MCMutableImageRep *>(m_rep)->LockBitmap(0, 0, t_bitmap);
            }
            else
            {
                if (!getflag(F_LOCK_LOCATION))
                {
                    setflag(true, F_LOCK_LOCATION);
                    t_tmp_locked = true;
                }
                openimage();
                t_success = lockbitmap(t_bitmap, true);
            }
            
            if (t_success)
			{
				uint8_t *t_src_ptr = (uint8_t*)t_bitmap->data;
				for (uindex_t y = 0; y < t_bitmap->height; y++)
				{
					uint32_t *t_src_row = (uint32_t*)t_src_ptr;
					for (uindex_t x = 0; x < t_bitmap->width; x++)
					{
                        uint8_t t_alpha;
                        t_alpha = MCGPixelGetNativeAlpha(*t_src_row++);
						if (p_flatten && t_alpha > 0)
							*t_data_ptr++ = 0xFF;
						else
							*t_data_ptr++ = t_alpha;
					}
					t_src_ptr += t_bitmap->stride;
				}
			}
            
            if (m_rep->GetType() == kMCImageRepMutable)
            {
                static_cast<MCMutableImageRep *>(m_rep)->UnlockBitmap(0, t_bitmap);
            }
            else
            {
                unlockbitmap(t_bitmap);
                if (t_tmp_locked)
                    setflag(false, F_LOCK_LOCATION);
                closeimage();
            }
		}
	}
	
	if (t_success)
    {
        t_success = t_buffer.CreateDataAndRelease(r_data);
        return;
    }
    
    ctxt . Throw();
}

extern void MCImageSetMask(MCImageBitmap *p_bitmap, uint8_t *p_mask_data, uindex_t p_mask_size, bool p_is_alpha);
void MCImage::SetTransparencyData(MCExecContext &ctxt, bool p_flatten, MCDataRef p_data)
{
    if (m_rep && m_rep->IsLocked())
    {
        ctxt . LegacyThrow(EE_IMAGE_MUTABLELOCK);
        return;
    }
	uindex_t t_length = MCDataGetLength(p_data);
	if (t_length != 0)
	{
		bool t_success = true;
		
		MCImageBitmap *t_copy = nil;
		if (m_rep != nil)
		{
            // PM-2015-02-09: [[ Bug 14483 ]] Reverted patch for bugfix 13938
            t_success = copybitmap(false, t_copy);
		}
		else
		{
			t_success = MCImageBitmapCreate(rect.width, rect.height, t_copy);
			if (t_success)
                MCImageBitmapSet(t_copy, MCGPixelPackNative(0, 0, 0, 255)); // set to opaque black
		}
		
		if (t_success)
		{
			MCImageSetMask(t_copy, (uint8_t*)MCDataGetBytePtr(p_data), t_length, !p_flatten);
            // PM-2015-02-09: [[ Bug 14483 ]] Reverted patch for bugfix 14347
            setbitmap(t_copy, 1.0);
		}
		
		MCImageFreeBitmap(t_copy);
		
		resetimage();
	}
}

void MCImage::GetMaskData(MCExecContext& ctxt, MCDataRef& r_data)
{
	GetTransparencyData(ctxt, true, r_data);
}

void MCImage::SetMaskData(MCExecContext& ctxt, MCDataRef p_data)
{
	SetTransparencyData(ctxt, true, p_data);
}

void MCImage::GetAlphaData(MCExecContext& ctxt, MCDataRef& r_data)
{
	GetTransparencyData(ctxt, false, r_data);
}

void MCImage::SetAlphaData(MCExecContext& ctxt, MCDataRef p_data)
{
	SetTransparencyData(ctxt, false, p_data);
    Redraw();
}

void MCImage::GetResizeQuality(MCExecContext& ctxt, intenum_t& r_quality)
{
	r_quality = (intenum_t)resizequality;
}

void MCImage::SetResizeQuality(MCExecContext& ctxt, intenum_t p_quality)
{
	resizequality = (uint1)p_quality;
}

void MCImage::GetPaintCompression(MCExecContext& ctxt, intenum_t& r_compression)
{
	uint4 pc;
	pc = getcompression();
	r_compression = (intenum_t)pc;
}

void MCImage::GetAngle(MCExecContext& ctxt, integer_t& r_angle)
{
	r_angle = angle;
}

void MCImage::SetAngle(MCExecContext& ctxt, integer_t p_angle)
{
	while (p_angle < 0)
		p_angle += 360;
	p_angle %= 360;

	if (p_angle != angle)
	{
		// MW-2010-11-25: [[ Bug 9195 ]] Make sure we have some image data to rotate, otherwise
		//   odd things happen with the rect.
		MCRectangle oldrect = rect;
        rotate_transform(p_angle);
		
		angle = p_angle;
		
		if (angle)
			flags |= F_ANGLE;
		else
			flags &= ~F_ANGLE;
		
		// MW-2011-08-18: [[ Layers ]] Notify of rect changed and invalidate.
		layer_rectchanged(oldrect, true);
		
		notifyneeds(false);
	}
}

// SN-2014-06-25: [[ MERGE-6.7 ]] P_CENTER_RECT property's getter added
// MW-2014-06-20: [[ ImageCenterRect ]] Setter for centerRect property.
void MCImage::SetCenterRectangle(MCExecContext& ctxt, MCRectangle *p_rectangle)
{
    if (p_rectangle == nil)
        m_center_rect = MCRectangleMake(INT16_MIN, INT16_MIN, UINT16_MAX, UINT16_MAX);
    else
    {
        m_center_rect . x = MCU_max(p_rectangle -> x, 0);
        m_center_rect . y = MCU_max(p_rectangle -> y, 0);
        m_center_rect . width = MCU_max(p_rectangle -> width, 0);
        m_center_rect . height = MCU_max(p_rectangle -> height, 0);
    }
    
    notifyneeds(false);
    
    if (opened)
        layer_redrawall();
}

// SN-2014-06-25: [[ MERGE-6.7 ]] P_CENTER_RECT property's setter added
// MW-2014-06-20: [[ ImageCenterRect ]] Getter for centerRect property.
void MCImage::GetCenterRectangle(MCExecContext& ctxt, MCRectangle *&r_rectangle)
{
    if (m_center_rect . x != INT16_MIN)
        // AL-2014-11-05: [[ Bug 13943 ]] Return center rect correctly
        *r_rectangle = m_center_rect;
    else
        r_rectangle = NULL;
}

void MCImage::SetBlendLevel(MCExecContext& ctxt, uinteger_t level)
{
    MCObject::SetBlendLevel(ctxt, level);
    notifyneeds(false);
}

void MCImage::SetInk(MCExecContext& ctxt, intenum_t ink)
{
    MCControl::SetInk(ctxt, ink);
    notifyneeds(false);
}

void MCImage::SetVisible(MCExecContext& ctxt, uinteger_t part, bool setting)
{
	bool wasvisible = isvisible();
	MCObject::SetVisible(ctxt, part, setting);
    if (!(MCbufferimages || flags & F_I_ALWAYS_BUFFER)
        && !isvisible() && m_rep != nil)
        closeimage();
    if (state & CS_IMAGE_PM && opened > 0)
    {
        // MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
        layer_redrawall();
    }
    if (isvisible() && !wasvisible && m_rep != nil && m_rep->GetFrameCount() > 1)
    {
		// IM-2014-11-25: [[ ImageRep ]] Use ImageRep method to get frame duration
		uint32_t t_frame_duration;
		if (m_rep->GetFrameDuration(currentframe, t_frame_duration))
        {
            MCscreen->addtimer(this, MCM_internal, t_frame_duration);
        }
    }
}

// MERG-2015-02-11: [[ ImageMetadata ]] Refactored image metadata property
void MCImage::GetMetadataProperty(MCExecContext& ctxt, MCNameRef p_prop, MCExecValue& r_value)
{
	// AL-2015-07-22: [[ Bug 15620 ]] If image rep is nil, don't try to fetch metadata
	bool t_stat;
	t_stat = m_rep != nil;

	MCAutoArrayRef t_metadata;
	if (t_stat)
		t_stat = MCImageRepGetMetadata(m_rep, &t_metadata);

	if (t_stat)
	{
		if (p_prop == nil || MCNameIsEmpty(p_prop))
		{
			r_value . arrayref_value = MCValueRetain(*t_metadata);
			r_value . type = kMCExecValueTypeArrayRef;
		}
		else
		{
			t_stat = MCArrayFetchValue(*t_metadata, true, p_prop, r_value.valueref_value);
			if (t_stat)
			{
				MCValueRetain(r_value.valueref_value);
				r_value.type = kMCExecValueTypeValueRef;
			}
		}
	}

	if (!t_stat)
	{
		r_value . arrayref_value = MCValueRetain(kMCEmptyArray);
		r_value . type = kMCExecValueTypeArrayRef;
	}
}
