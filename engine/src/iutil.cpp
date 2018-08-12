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
#include "util.h"
#include "dispatch.h"
#include "stack.h"
#include "stacklst.h"
#include "sellst.h"
#include "undolst.h"
#include "image.h"
#include "uidc.h"

#include "mcio.h"

#include "globals.h"

#include "context.h"
#include "packed.h"

#include "iquantization.h"

void MCImage::init()
{
	brush.image = spray.image = eraser.image = nil;

	MCImage *im;
	if ((im = (MCImage *)MCdispatcher->getobjid(CT_IMAGE, 108)) != NULL)
		im->createbrush(P_BRUSH);
	else
		MCtemplateimage->createbrush(P_BRUSH);
	if ((im = (MCImage *)MCdispatcher->getobjid(CT_IMAGE, 102)) != NULL)
		im->createbrush(P_ERASER);
	else
		MCtemplateimage->createbrush(P_ERASER);
	if ((im = (MCImage *)MCdispatcher->getobjid(CT_IMAGE, 134)) != NULL)
		im->createbrush(P_SPRAY);
	else
		MCtemplateimage->createbrush(P_SPRAY);

	MCMutableImageRep::init();
}

void MCImage::shutdown()
{
	MCGImageRelease(brush.image);
	MCGImageRelease(spray.image);
	MCGImageRelease(eraser.image);

	MCMutableImageRep::shutdown();
}

////////////////////////////////////////////////////////////////////////////////

MCImageNeed::MCImageNeed(MCObject *p_object) :
  m_object(p_object->GetHandle()),
  m_prev(nil),
  m_next(nil)
{
}

MCImageNeed::~MCImageNeed()
{
}

void MCImageNeed::Add(MCImageNeed *&x_head)
{
	if (x_head != nil)
		x_head->m_prev = this;
	m_next = x_head;
	m_prev = nil;
	x_head = this;
}

void MCImageNeed::Remove(MCImageNeed *&x_head)
{
	if (m_next != nil)
		m_next->m_prev = m_prev;
	if (m_prev != nil)
		m_prev->m_next = m_next;

	if (this == x_head)
		x_head = m_next;

	m_next = m_prev = nil;
}

MCObject *MCImageNeed::GetObject()
{
	if (m_object.IsValid())
        return m_object;
    else
        return nil;
}

MCImageNeed *MCImageNeed::GetNext()
{
	return m_next;
}

//////////

void MCImage::addneed(MCObject *p_object)
{
	MCImageNeed *t_need = m_needs;

	while (t_need != nil)
	{
		MCImageNeed *t_next = t_need->GetNext();
		MCObject *t_obj = t_need->GetObject();
		if (t_obj == nil)
		{
			// remove from list
			t_need->Remove(m_needs);
			delete t_need;
		}
		else if (p_object == t_obj)
		{
			if (t_need != m_needs)
			{
				// move to head of list
				t_need->Remove(m_needs);
				t_need->Add(m_needs);
			}
			return;
		}

		t_need = t_next;
	}

	// not found - create new need & add to list
	/* UNCHECKED */ t_need = new (nothrow) MCImageNeed(p_object);
	t_need->Add(m_needs);
}

void MCImage::notifyneeds(bool p_deleting)
{
	MCImageNeed *t_need = m_needs;
	while (t_need != nil)
	{
		MCImageNeed *t_next = t_need->GetNext();
		MCObject *t_obj = t_need->GetObject();
		if (t_obj == nil || !t_obj->imagechanged(this, p_deleting) || p_deleting)
		{
			// remove from list
			t_need->Remove(m_needs);
			delete t_need;
		}
		t_need = t_next;
	}
}

//////////

MCGImageFilter MCImage::resizequalitytoimagefilter(uint8_t p_quality)
{
    // MM-2014-05-29: [[ Bug 12382 ]] Temporarily reverted the box filter back to none to improve perfromace on non-Mac platforms.
	switch (p_quality)
	{
        case INTERPOLATION_NEAREST:
            return kMCGImageFilterNone;

        case INTERPOLATION_BOX:
            return kMCGImageFilterNone;

		case INTERPOLATION_BILINEAR:
            return kMCGImageFilterMedium;

		case INTERPOLATION_BICUBIC:
            return kMCGImageFilterHigh;

		default:
			return kMCGImageFilterNone;
	}
}

//////////

// composite image against black, opaque = alpha > 0
void MCImageBitmapFlattenAlpha(MCImageBitmap *p_bitmap, bool p_preserve_mask)
{
	if (!p_bitmap->has_transparency || (!p_bitmap->has_alpha && p_preserve_mask))
		return;

	uint8_t *t_src_ptr = (uint8_t*)p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_src_row = (uint32_t*)t_src_ptr;
		for (uindex_t x = 0; x < p_bitmap->width; x++)
		{
			uint8_t t_alpha = *t_src_row >> 24;
			if (t_alpha == 0)
				*t_src_row = p_preserve_mask ? 0x00000000 : 0xFF000000;
			else if (t_alpha < 0xFF)
				*t_src_row = 0xFF000000 | packed_scale_bounded(*t_src_row, t_alpha);
			t_src_row++;
		}
		t_src_ptr += p_bitmap->stride;
	}

	p_bitmap->has_transparency = p_bitmap->has_alpha && p_preserve_mask;
	p_bitmap->has_alpha = false;
}

bool MCImageQuantizeColors(MCImageBitmap *p_bitmap, MCImagePaletteSettings *p_palette_settings, bool p_dither, bool p_transparency_index, MCImageIndexedBitmap *&r_indexed)
{
	bool t_success = true;

	bool t_has_transparency = false;
	uint32_t t_bits = 0;
	uint32_t t_maxcolours = 0;

	MCImageBitmap *t_flattened = nil;
	MCColor *t_colors = nil;
	uindex_t t_color_count = 0;

	t_success = MCImageCopyBitmap(p_bitmap, t_flattened);

	if (t_success)
	{
		MCImageBitmapFlattenAlpha(t_flattened, p_transparency_index);
		t_has_transparency = p_transparency_index && MCImageBitmapHasTransparency(t_flattened);

		if (p_palette_settings->ncolors <= 2)
			t_bits = 1;
		else if (p_palette_settings->ncolors <= 4)
			t_bits = 2;
		else if (p_palette_settings->ncolors <= 16)
			t_bits = 4;
		else
			t_bits = 8;

		t_maxcolours = 1 << t_bits;
		if (t_has_transparency)
			t_maxcolours -= 1;

		t_maxcolours = MCU_min(p_palette_settings->ncolors, t_maxcolours);

		switch (p_palette_settings->type)
		{
		case kMCImagePaletteTypeOptimal:
			t_color_count = t_maxcolours;
			t_success = MCImageGenerateOptimalPaletteWithWeightedPixels(t_flattened, t_color_count, t_colors);
			break;
		case kMCImagePaletteTypeWebSafe:
			t_success = MCImageGenerateWebsafePalette(t_color_count, t_colors);
			break;
		case kMCImagePaletteTypeCustom:
			t_color_count = t_maxcolours;
			t_colors = p_palette_settings->colors;
			break;
		default:
			t_success = false;
		}
	}

	if (t_success)
		t_success = MCImageQuantizeImageBitmap(t_flattened, t_colors, t_color_count, p_dither, t_has_transparency, r_indexed);

	MCImageFreeBitmap(t_flattened);
	if (p_palette_settings->type != kMCImagePaletteTypeCustom)
		MCMemoryDeallocate(t_colors);

	return t_success;
}

Boolean MCImage::noblack()
{
	uint4 i = ncolors;
	while (i--)
		if (colors[i].red == 0 && colors[i].green == 0 && colors[i].blue == 0)
			return False;
	return True;
}

void MCImage::cutimage()
{
	copyimage();
	if (isediting() || convert_to_mutable())
	{
		static_cast<MCMutableImageRep*>(m_rep)->cutoutsel();
	}
	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	layer_redrawall();
}

void MCImage::copyimage()
{
	bool t_success = true;
	
	MCAutoDataRef t_image_data;
	
	if (isediting())
	{
		MCImageBitmap *t_bitmap = nil;
		t_success = static_cast<MCMutableImageRep*>(m_rep)->copy_selection(t_bitmap);
		if (t_success)
			t_success = MCImageCreateClipboardData(t_bitmap, &t_image_data);
		MCImageFreeBitmap(t_bitmap);
	}
	else
		getclipboardtext(&t_image_data);
	
	if (*t_image_data != NULL)
    {
        // Clear the clipboard and add the image to it.
        MCclipboard->Clear();
        MCclipboard->AddImage(*t_image_data);
    }
}

void MCImage::delimage()
{
	if (isediting() || convert_to_mutable())
	{
		static_cast<MCMutableImageRep*>(m_rep)->cutoutsel();
	}
	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	layer_redrawall();
}

void MCImage::selimage()
{
	if (isediting() || convert_to_mutable())
	{
		static_cast<MCMutableImageRep*>(m_rep)->selimage();
	}
}

void MCImage::flipsel(Boolean p_horizontal)
{
	if (isediting() || convert_to_mutable())
	{
		static_cast<MCMutableImageRep*>(m_rep)->flipsel(p_horizontal);
	}
}

void MCImage::rotatesel(int16_t p_angle)
{
	if (isediting() || convert_to_mutable())
	{
		static_cast<MCMutableImageRep*>(m_rep)->rotatesel(p_angle);
	}
}

void MCImage::pasteimage(MCImage *clipimage)
{
	if (isediting() || convert_to_mutable())
	{
		MCImageBitmap *t_bitmap = nil;
		if (clipimage->lockbitmap(t_bitmap, false))
		{
			static_cast<MCMutableImageRep*>(m_rep)->pasteimage(t_bitmap);
		}
		clipimage->unlockbitmap(t_bitmap);
	}
}

void MCImage::compute_gravity(MCRectangle &trect, int2 &xorigin, int2 &yorigin)
{
	uint16_t t_width, t_height;
	t_width = m_current_width;
	t_height = m_current_height;

	trect = rect;
	xorigin = yorigin = 0;
	if (rect.width != t_width)
	{
		if (state & CS_SIZEL)
		{
			if (rect.width > t_width)
				trect.x = rect.x + rect.width - t_width;
			else
				xorigin = t_width - rect.width;
		}
		trect.width = MCU_min(t_width, rect.width);
	}
	if (rect.height != t_height)
	{
		if (state & CS_SIZET)
		{
			if (rect.height > t_height)
				trect.y = rect.y + rect.height - t_height;
			else
				yorigin = t_height - rect.height;
		}
		trect.height = MCU_min(t_height, rect.height);
	}

    //MCLog("compute gravity: rect(%d,%d,%d,%d) x(%d) y(%d)", trect.x, trect.y, trect.width, trect.height, xorigin, yorigin);
}

void MCImage::compute_offset(MCRectangle &p_rect, int16_t &r_xoffset, int16_t &r_yoffset)
{
	uint32_t t_pixwidth, t_pixheight;
	getgeometry(t_pixwidth, t_pixheight);
	if (state & CS_SIZE && state & CS_EDITED && (t_pixwidth != rect.width || t_pixheight != rect.height))
		compute_gravity(p_rect, r_xoffset, r_yoffset);
	else
	{
		r_xoffset = p_rect.x - rect.x;
		r_yoffset = p_rect.y - rect.y;
	}
}

void MCImage::crop(MCRectangle *newrect)
{
	MCRectangle oldrect;
	if (newrect != NULL)
	{
		oldrect = rect;
		rect.width = MCU_max(newrect->width, 1);
		rect.height = MCU_max(newrect->height, 1);
	}

	if (m_rep == nil || m_rep->GetType() == kMCImageRepVector)
		return;

	MCRectangle trect;
	int2 xorigin, yorigin;
	if (!newrect)
	{
		compute_gravity(trect, xorigin, yorigin);
		trect.x -= rect.x;
		trect.y -= rect.y;
	}
	else
	{
		trect.x = trect.y = xorigin = yorigin = 0;
		if (newrect->x > rect.x)
			xorigin = newrect->x - rect.x;
		else
			trect.x = rect.x - newrect->x;
		if (newrect->y > rect.y)
			yorigin = newrect->y - rect.y;
		else
			trect.y = rect.y - newrect->y;
		trect.width = newrect->width;
		trect.height = newrect->height;
	}

	MCImageBitmap *t_bitmap = nil;
	/* UNCHECKED */ lockbitmap(t_bitmap, false, false);

	MCImageBitmap *t_cropimage = nil;
	/* UNCHECKED */ MCImageBitmapCreate(rect.width, rect.height, t_cropimage);

	if (rect.width > t_bitmap->width || rect.height > t_bitmap->height || newrect)
		MCImageBitmapClear(t_cropimage);

	MCImageBitmapCopyRegionToBitmap(t_bitmap, t_cropimage, xorigin, yorigin, trect.x, trect.y, trect.width, trect.height);

	unlockbitmap(t_bitmap);

	/* UNCHECKED */ setbitmap(t_cropimage, 1.0);
	
	// PM-2015-07-13: [[ Bug 15590 ]] Fix memory leak
	MCImageFreeBitmap(t_cropimage);

	uint32_t t_pixwidth, t_pixheight;
	getgeometry(t_pixwidth, t_pixheight);
	if (xhot == (int32_t)t_pixwidth >> 1)
		xhot = rect.width >> 1;
	if (yhot == (int32_t)t_pixheight >> 1)
		yhot = rect.height >> 1;

	if (newrect)
	{
		rect.x = newrect->x;
		rect.y = newrect->y;
		// MW-2011-08-18: [[ Layers ]] Notify of changed rect and invalidate.
		layer_rectchanged(oldrect, true);
	}
}

// Rotate the image by the number of degrees stored in the 'angle' field
// of the object.
//
// Rotation should occur around the centre of the image. In particular, 
// 'the location' of the image should not change.
// If the image's lock location is set, then the rotated image should be
// cropped to the existing bounds.
//
// If the lock location of the image is not set, then the final bounds of
// the image should be small enough to completely cover the colored part
// of the image, but large enough to maintain the same pixel centre.
//

void MCCalculateRotatedGeometry(uint32_t p_width, uint32_t p_height, int32_t p_angle, uint32_t &r_width, uint32_t &r_height)
{
	real8 t_angle = p_angle * M_PI / 180.0;
	real8 t_cos = cos(t_angle);
	real8 t_sin = sin(t_angle);

	r_width = (uint32_t)ceil(p_width * fabs(t_cos) + p_height * fabs(t_sin));
	r_height = (uint32_t)ceil(p_width * fabs(t_sin) + p_height * fabs(t_cos));
}

void MCImage::rotate_transform(int32_t p_angle)
{
	uint32_t t_src_width = rect.width, t_src_height = rect.height;
	/* UNCHECKED */ getsourcegeometry(t_src_width, t_src_height);
	
	uint32_t t_trans_width = t_src_width;
	uint32_t t_trans_height = t_src_height;

	// IM-2013-04-22: [[ BZ 10858 ]] A transformed rep is needed if the angle is non-zero,
	// or the rect is locked and doesn't match the source dimensions.
	// MW-2013-10-25: [[ Bug 11300 ]] Additionally a transform is needed if flipped on
	//   either axis.
	if (p_angle == 0 && !(getflag(F_LOCK_LOCATION) && (t_src_width != rect.width || t_src_height != rect.height)) && !m_flip_x && !m_flip_y)
		m_has_transform = false;
	else
	{
		m_has_transform = true;

		MCCalculateRotatedGeometry(t_src_width, t_src_height, p_angle, t_trans_width, t_trans_height);

		MCGAffineTransform t_transform = MCGAffineTransformMakeTranslation(-(int32_t)t_src_width / 2.0, -(int32_t)t_src_height / 2.0);
		t_transform = MCGAffineTransformPreRotate(t_transform, -p_angle);
		
		// MW-2013-10-25: [[ Bug 11300 ]] If needed, flip the transform appropriately.
		if (m_flip_x || m_flip_y)
			t_transform = MCGAffineTransformPreScale(t_transform, m_flip_x ? -1.0f : 1.0f, m_flip_y ? -1.0f : 1.0f);
		
		t_transform = MCGAffineTransformPreTranslate(t_transform, t_trans_width / 2.0, t_trans_height / 2.0);
		
		if (getflag(F_LOCK_LOCATION))
		{
			t_transform = MCGAffineTransformPreScale(t_transform, rect.width / (MCGFloat)t_trans_width, rect.height / (MCGFloat)t_trans_height);
			t_trans_width = rect.width;
			t_trans_height = rect.height;
		}
		
		// MM-2013-09-16: [[ Bug 11179 ]] Make sure we store the transform.
		m_transform = t_transform;
	}

	if (t_trans_width != rect.width || t_trans_height != rect.height)
	{
		int2 diff = rect.width - t_trans_width;
		rect.x += (diff >> 1) + (diff & 0x1 ? rect.width & 1 : 0);
		diff = rect.height - t_trans_height;
		rect.y += (diff >> 1) + (diff & 0x1 ? rect.height & 1 : 0);
		rect.width = t_trans_width;
		rect.height = t_trans_height;
	}
}

void MCImage::resize_transform()
{
	uint32_t t_src_width = rect.width, t_src_height = rect.height;
	/* UNCHECKED */ getsourcegeometry(t_src_width, t_src_height);
	
	// MW-2013-10-25: [[ Bug 11300 ]] Additionally a transform is needed if flipped on
	//   either axis.
	if (rect.width == t_src_width && rect.height == t_src_height && !m_flip_x && !m_flip_y)
		m_has_transform = false;
	else
	{
		m_has_transform = true;
		
		MCGAffineTransform t_transform;

		// MW-2013-10-25: [[ Bug 11300 ]] If needed, flip the transform appropriately.
		if (m_flip_x || m_flip_y)
		{
			t_transform = MCGAffineTransformMakeTranslation(-(signed)t_src_width / 2.0f, -(signed)t_src_height / 2.0f);
			t_transform = MCGAffineTransformPreScale(t_transform, m_flip_x ? -1.0f : 1.0f, m_flip_y ? -1.0f : 1.0f);
			t_transform = MCGAffineTransformPreTranslate(t_transform, t_src_width / 2.0, t_src_height / 2.0);
		}
		else
			t_transform = MCGAffineTransformMakeIdentity();
		
		m_transform = MCGAffineTransformPreScale(t_transform, rect.width / (MCGFloat)t_src_width, rect.height / (MCGFloat)t_src_height);
	}
}

// MW-2013-05-25: [[ Bug 11300 ]] This applies the flip transform if needed.
void MCImage::flip_transform()
{
	uint32_t t_src_width = rect.width, t_src_height = rect.height;
	/* UNCHECKED */ getsourcegeometry(t_src_width, t_src_height);
	
	if (!m_flip_x && !m_flip_y)
		m_has_transform = false;
	else
	{
		m_has_transform = true;
		
		m_transform = MCGAffineTransformMakeTranslation(-(signed)t_src_width / 2.0, -(signed)t_src_height / 2.0);
		m_transform = MCGAffineTransformPreScale(m_transform, m_flip_x ? -1.0f : 1.0f, m_flip_y ? -1.0f : 1.0f);
		m_transform = MCGAffineTransformPreTranslate(m_transform, t_src_width / 2.0, t_src_height / 2.0);
	}	
}

void MCImage::createbrush(Properties which)
{
	MCGImageRef t_image = nil;
	/* UNCHECKED */ decompressbrush(t_image);
	MCBrush t_brush;
	t_brush.image = t_image;
	t_brush.xhot = xhot;
	t_brush.yhot = yhot;

	uint2 index;
	switch (which)
	{
	case P_BRUSH:
		MCGImageRelease(brush.image);
		brush = t_brush;
		index = PI_BRUSH;
		break;
	case P_SPRAY:
		MCGImageRelease(spray.image);
		spray = t_brush;
		index = PI_SPRAY;
		break;
	case P_ERASER:
		MCGImageRelease(eraser.image);
		eraser = t_brush;
		index = PI_ERASER;
		break;
	default:
		index = 0;
		break;
	}

	if (t_image)
	{
		MCscreen->freecursor(MCcursors[index]);
		MCcursors[index] = createcursor();
	}
}

MCBrush *MCImage::getbrush(Tool p_which)
{
	switch (p_which)
	{
	case T_BRUSH:
		return &brush;
	case T_SPRAY:
		return &spray;
	case T_ERASER:
		return &eraser;
	default:
		return nil;
	}
}

// This method makes the best cursor it can out of the image's current bits.
//
// First of all, if the image is larger than the maximum cursor size of the
// desktop, the it scales the image down to fit - preserving aspect ratio.
//
// If the desktop supports color, alpha-blended cursors it then uses the
// resulting image.
//
// If the desktop only supports bi-level color cursors, it computes an optimal
// two color palette and dithers the image down. If the desktop only supports
// black and white curors, it dithers the image to monochrome.
//
// Finally, any alpha mask is dithered to a 1-bit mask and the resulting image
// use as the cursor.
//
extern bool MCImageDitherAlphaInPlace(MCImageBitmap *p_bitmap);
MCCursorRef MCImage::createcursor()
{
	Boolean ob = MCbufferimages;
	MCbufferimages = True;
	MCCursorRef t_cursor;
	int32_t t_xhot, t_yhot;
	
	openimage();

	int32_t t_width, t_height;
	t_width = rect . width;
	t_height = rect . height;

	// if the cursor cannot have color or it cannot have alpha then we need to transform
	// the original color image
	bool t_premultiply = MCcursorcanbealpha && MCcursorcanbecolor;

	MCImageBitmap *t_bitmap = nil;
	MCImageBitmap *t_cursor_bitmap = nil;
	/* UNCHECKED */ lockbitmap(t_bitmap, t_premultiply);
	/* UNCHECKED */ MCImageCopyBitmap(t_bitmap, t_cursor_bitmap);
	unlockbitmap(t_bitmap);

	closeimage();

	MCbufferimages = ob;
	
	int32_t t_largest_side;
	t_largest_side = MCMax(t_width, t_height);

	if (t_largest_side > MCcursormaxsize)
	{
		t_width = MCcursormaxsize * t_width / t_largest_side;
		t_height = MCcursormaxsize * t_height / t_largest_side;
		t_xhot = MCcursormaxsize * xhot / t_largest_side;
		t_yhot = MCcursormaxsize * yhot / t_largest_side;

		MCImageBitmap *t_scaled = nil;
		/* UNCHECKED */ MCImageScaleBitmap(t_cursor_bitmap, t_width, t_height, INTERPOLATION_NEAREST, t_scaled);
		MCImageFreeBitmap(t_cursor_bitmap);
		t_cursor_bitmap = t_scaled;
	}
	else
	{
		t_xhot = xhot;
		t_yhot = yhot;
	}

	// IM-2013-11-11: [[ RefactorGraphics ]] Check if image has alpha before dithering
	bool t_has_mask, t_has_alpha;
	t_has_mask = MCImageBitmapHasTransparency(t_cursor_bitmap, t_has_alpha);

	// If the alpha mask is present, and cursors cannot have alpha then dither
	// the mask down.
	if (t_has_alpha && !MCcursorcanbealpha)
		// IM-2013-11-11: [[ RefactorGraphics ]] Replace copy operation with in-place version
		// as the bitmap has already been copied from the source image.
		/* UNCHECKED */ MCImageDitherAlphaInPlace(t_cursor_bitmap);

	// If the cursor cannot be color, then we must reduce the colors of the image.
	if (!MCcursorcanbecolor)
	{
		MCColor t_palette[2];
		MCColor *t_colors;
        // SN-2015-06-02: [[ CID 90611 ]] Initialise t_colors
        t_colors = NULL;
        
		if (!MCcursorbwonly)
			MCImageGenerateOptimalPaletteWithWeightedPixels(t_cursor_bitmap, 2, t_colors);
		else
		{
			t_colors = t_palette;
			t_palette[0] . red = t_palette[0] . green = t_palette[0] . blue = 0;
			t_palette[1] . red = t_palette[1] . green = t_palette[1] . blue = 0xffff;
		}

		MCImageIndexedBitmap *t_indexed = nil;
		/* UNCHECKED */ MCImageQuantizeImageBitmap(t_cursor_bitmap, t_colors, 2, true, true, t_indexed);
		MCImageFreeBitmap(t_cursor_bitmap);
		t_cursor_bitmap = nil;
		/* UNCHECKED */ MCImageConvertIndexedToBitmap(t_indexed, t_cursor_bitmap);
		MCImageFreeIndexedBitmap(t_indexed);

		if (t_colors != t_palette)
			MCMemoryDeleteArray(t_colors);
	}

	// Create the cursor
	t_cursor = MCscreen -> createcursor(t_cursor_bitmap, t_xhot, t_yhot);

	// Destroy the transient bitmaps we needed
	MCImageFreeBitmap(t_cursor_bitmap);

	return t_cursor;
}

MCCursorRef MCImage::getcursor(bool p_is_default)
{
	if (p_is_default)
	{
		if (defaultcursor == MCdefaultcursor)
			MCdefaultcursor = nil;

		if (defaultcursor != nil)
			MCscreen -> freecursor(defaultcursor);

		defaultcursor = createcursor();

		return defaultcursor;
	}

	if (cursor == MCcursor)
		MCcursor = nil;

	if (cursor != nil)
		MCscreen -> freecursor(cursor);

	cursor = createcursor();
	
	return cursor;
}

bool MCImage::createpattern(MCPatternRef &r_image)
{
	bool t_success = true;

	MCImageBitmap *t_bitmap = nil;
	MCImageBitmap *t_blank = nil;

	MCPatternRef t_pattern;
	t_pattern = nil;

	openimage();

	if (m_rep == nil)
	{
		// create blank bitmap;
		t_success = MCImageBitmapCreate(rect.width, rect.height, t_blank);
		if (t_success)
		{
			MCImageBitmapClear(t_blank);
			t_bitmap = t_blank;

			MCGRaster t_raster;
			t_raster = MCImageBitmapGetMCGRaster(t_bitmap, true);
			
			// IM-2013-08-14: [[ ResIndependence ]] Wrap image in MCPattern with scale factor
			t_success = MCPatternCreate(t_raster, 1.0, getimagefilter(), t_pattern);
		}
	}
	else
	{
		// IM-2014-05-13: [[ HiResPatterns ]] Rather than create a pattern with a static bitmap image,
		// we can now supply the source rep and the image transform to enable density-mapped patterns
		apply_transform();
		t_success = MCPatternCreate(m_rep, m_has_transform ? m_transform : MCGAffineTransformMakeIdentity(), getimagefilter(), t_pattern);
	}

	if (t_blank != nil)
		MCImageFreeBitmap(t_blank);

	closeimage();
	
	if (t_success)
		r_image = t_pattern;

	return t_success;
}

template <uint1 quality>
bool MCImageRotateRotate(MCImageBitmap *p_src, real64_t p_angle, uint32_t p_backing_color, MCImageBitmap *&r_rotated);

bool MCImageRotateBitmap(MCImageBitmap *p_src, real64_t p_angle, uint8_t p_quality, uint32_t p_backing_color, MCImageBitmap *&r_rotated)
{
	bool t_success = true;

	MCImageBitmap *t_midimage = nil;

	if (p_angle > 45.0 && p_angle <= 135.0)
	{
		p_angle -= 90.0;
		t_success = MCImageBitmapCreate(p_src->height, p_src->width, t_midimage);

		if (t_success)
		{
			for (uindex_t y = 0; y < p_src->height; y++)
				for (uindex_t x = 0; x < p_src->width; x++)
					MCImageBitmapSetPixel(t_midimage, y, p_src->width - x - 1,
					MCImageBitmapGetPixel(p_src, x, y));
		}
	}
	else if (p_angle > 135.0 && p_angle <= 225.0)
	{
		p_angle -= 180.0;
		t_success = MCImageBitmapCreate(p_src->width, p_src->height, t_midimage);

		if (t_success)
		{
			for (uindex_t y = 0; y < p_src->height; y++)
				for (uindex_t x = 0; x < p_src->width; x++)
					MCImageBitmapSetPixel(t_midimage, p_src->width - x - 1, p_src->height - y - 1,
					MCImageBitmapGetPixel(p_src, x, y));
		}
	}
	else if ((p_angle > 225.0) && (p_angle <= 315.0))
	{
		p_angle -= 270.0;
		t_success = MCImageBitmapCreate(p_src->height, p_src->width, t_midimage);

		if (t_success)
		{
			for (uindex_t y = 0; y < p_src->height; y++)
				for (uindex_t x = 0; x < p_src->width; x++)
					MCImageBitmapSetPixel(t_midimage, p_src-> height - y - 1, x,
					MCImageBitmapGetPixel(p_src, x, y));
		}
	}

	if (t_success && t_midimage != nil)
	{
		t_midimage->has_transparency = p_src->has_transparency;
		t_midimage->has_alpha = p_src->has_alpha;
		p_src = t_midimage;
	}

	if (t_success && p_angle == 0)
	{
		if (t_midimage == nil)
			return MCImageCopyBitmap(p_src, r_rotated);

		r_rotated = t_midimage;
		return true;
	}

	if (t_success)
	{
		if (p_quality == INTERPOLATION_BOX)
			t_success = MCImageRotateRotate<INTERPOLATION_BOX>(p_src, p_angle, p_backing_color, r_rotated);
		else
			t_success = MCImageRotateRotate<INTERPOLATION_BILINEAR>(p_src, p_angle, p_backing_color, r_rotated);
	}

	MCImageFreeBitmap(t_midimage);

	return t_success;
}

template <uint1 quality>
bool MCImageRotateRotate(MCImageBitmap *p_src, real64_t p_angle, uint32_t p_backing_color, MCImageBitmap *&r_rotated)
{
	if (p_angle == 0)
		return MCImageCopyBitmap(p_src, r_rotated);

	real8 t_angle = p_angle * M_PI / 180.0;
	real8 t_cos = cos(t_angle);
	real8 t_sin = sin(t_angle);

	uint2 t_width = (uint2)ceil(p_src->width * fabs(t_cos) + p_src->height * fabs(t_sin));
	uint2 t_height = (uint2)ceil(p_src->width * fabs(t_sin) + p_src->height * fabs(t_cos));

	if (!MCImageBitmapCreate(t_width, t_height, r_rotated))
		return false;

	int4 t_icos = (int4) (256 * t_cos);
	int4 t_isin = (int4) (256 * t_sin);

	int4 t_isinh = t_isin * - (t_height >> 1);
	int4 t_icosh = t_icos * - (t_height >> 1);
	int4 t_icosw = t_icos * - (t_width >> 1);
	int4 t_isinw = t_isin * - (t_width >> 1);

	t_icosw += -t_isinh + (p_src->width << 7);
	t_isinw += t_icosh + (p_src->height << 7);

	uint8_t *t_dst_ptr = (uint8_t*)r_rotated->data;
	for (uint2 t_y=0; t_y < t_height; t_y++)
	{
		int4 t_xinc = t_icosw;
		int4 t_yinc = t_isinw;

		uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
		t_dst_ptr += r_rotated->stride;
		for (uint2 t_x=0; t_x < t_width; t_x++)
		{
			int4 t_ix, t_iy;
			uint4 t_rem_x, t_rem_y;
			t_ix = t_xinc >> 8;  t_iy = t_yinc >> 8;
			t_rem_x = t_xinc & 0xFF;  t_rem_y = t_yinc & 0xFF;
			t_xinc += t_icos;
			t_yinc += t_isin;
			
			uint4 t_pixel;

			if (quality == INTERPOLATION_BOX)
			{
				if (t_ix < 0 || t_ix >= p_src->width || t_iy < 0 || t_iy >= p_src->height)
					t_pixel = p_backing_color;
				else
					t_pixel = MCImageBitmapGetPixel(p_src, t_ix, t_iy);
			}
			else
			{
				if (t_ix < -1 || t_ix >= p_src->width || t_iy < -1 || t_iy >= p_src->height)
					t_pixel = p_backing_color;
				else
				{
					uint4 t_p1, t_p2, t_p3, t_p4;
					uint4 t_pleft, t_pright;
					if (t_ix == -1)
					{
						t_pleft = p_backing_color;
						if (t_iy == -1)
						{
							t_p2 = p_backing_color;
							t_p4 = MCImageBitmapGetPixel(p_src, t_ix + 1, t_iy + 1);
						}
						else if (t_iy == p_src->height - 1)
						{
							t_p2 = MCImageBitmapGetPixel(p_src, t_ix + 1, t_iy);
							t_p4 = p_backing_color;
						}
						else
						{
							t_p2 = MCImageBitmapGetPixel(p_src, t_ix + 1, t_iy);
							t_p4 = MCImageBitmapGetPixel(p_src, t_ix + 1, t_iy + 1);
						}
						t_pright = packed_bilinear_bounded(t_p2, 255 - t_rem_y, t_p4, t_rem_y);
						t_pixel = packed_bilinear_bounded(t_pleft, 255 - t_rem_x, t_pright, t_rem_x);
					}
					else if (t_iy == -1)
					{
						t_pleft = p_backing_color;
						if (t_ix == p_src->width -1)
						{
							t_p3 = MCImageBitmapGetPixel(p_src, t_ix, t_iy + 1);
							t_p4 = p_backing_color;
						}
						else
						{
							t_p3 = MCImageBitmapGetPixel(p_src, t_ix, t_iy + 1);
							t_p4 = MCImageBitmapGetPixel(p_src, t_ix + 1, t_iy + 1);
						}
						t_pright = packed_bilinear_bounded(t_p3, 255 - t_rem_x, t_p4, t_rem_x);
						t_pixel = packed_bilinear_bounded(t_pleft, 255 - t_rem_y, t_pright, t_rem_y);
					}
					else if (t_ix == p_src->width - 1)
					{
						t_pright = p_backing_color;
						if (t_iy == p_src->height - 1)
						{
							t_p1 = MCImageBitmapGetPixel(p_src, t_ix, t_iy);
							t_p3 = p_backing_color;
						}
						else
						{
							t_p1 = MCImageBitmapGetPixel(p_src, t_ix, t_iy);
							t_p3 = MCImageBitmapGetPixel(p_src, t_ix, t_iy + 1);
						}
						t_pleft = packed_bilinear_bounded(t_p1, 255 - t_rem_y, t_p3, t_rem_y);
						t_pixel = packed_bilinear_bounded(t_pleft, 255 - t_rem_x, t_pright, t_rem_x);
					}
					else if (t_iy == p_src->height - 1)
					{
						t_pright = p_backing_color;
						t_p1 = MCImageBitmapGetPixel(p_src, t_ix, t_iy);
						t_p2 = MCImageBitmapGetPixel(p_src, t_ix + 1, t_iy);
						t_pleft = packed_bilinear_bounded(t_p1, 255 - t_rem_x, t_p2, t_rem_x);
						t_pixel = packed_bilinear_bounded(t_pleft, 255 - t_rem_y, t_pright, t_rem_y);
					}
					else
					{
						t_p1 = MCImageBitmapGetPixel(p_src, t_ix, t_iy);
						t_p2 = MCImageBitmapGetPixel(p_src, t_ix + 1, t_iy);
						t_p3 = MCImageBitmapGetPixel(p_src, t_ix, t_iy + 1);
						t_p4 = MCImageBitmapGetPixel(p_src, t_ix + 1, t_iy + 1);

						t_pleft = packed_bilinear_bounded(t_p1, 255 - t_rem_y, t_p3, t_rem_y);
						t_pright = packed_bilinear_bounded(t_p2, 255 - t_rem_y, t_p4, t_rem_y);
						t_pixel = packed_bilinear_bounded(t_pleft, 255 - t_rem_x, t_pright, t_rem_x);
					}
				}
			}
			*t_dst_row++ = t_pixel;
		}
		t_icosw -= t_isin;
		t_isinw += t_icos;
	}
	
	/* OVERHAUL - REVISIT - rotated images will have added transparency & alpha at the edges */
	r_rotated->has_transparency = r_rotated->has_alpha = true;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCImage::resetimage()
{
	if (m_rep != nil)
	{
		uint32_t t_width, t_height;
		getgeometry(t_width, t_height);
		MCRectangle t_old_rect;
		t_old_rect = rect;
		if (t_width != rect.width || t_height != rect.height)
		{
			if ((flags & F_LOCK_LOCATION) == 0)
			{
				rect.x += (rect.width - t_width) >> 1;
				rect.width = t_width;
				rect.y += (rect.height - t_height) >> 1;
				rect.height = t_height;
			}
		}

		// MW-2011-09-12: [[ Redraw ]] If the rect has changed then notify the layer.
		//   (note we have to check 'parent' as at the moment MCImage is used for
		//    the rb* icons which are unowned!).
		if (parent && !MCU_equal_rect(t_old_rect, rect))
			layer_rectchanged(t_old_rect, false);

		if (m_rep->GetFrameCount() > 1)
		{
			if ((flags & F_REPEAT_COUNT) == 0)
				repeatcount = -1;
			irepeatcount = repeatcount;
			state |= CS_DO_START;
		}
	}

	if (parent)
		layer_redrawall();
}

////////////////////////////////////////////////////////////////////////////////

void MCImageFreeFrames(MCBitmapFrame *p_frames, uindex_t p_count)
{
	if (p_frames != nil)
	{
		for (uindex_t i = 0; i < p_count; i++)
			MCImageFreeBitmap(p_frames[i].image);
		MCMemoryDeleteArray(p_frames);
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageBitmapApplyColorTransform(MCImageBitmap *p_bitmap, MCColorTransformRef p_transform)
{
	return MCscreen->transformimagecolors(p_transform, p_bitmap);
}

////////////////////////////////////////////////////////////////////////////////

// Legacy Functions

void MCImageBitmapSetAlphaValue(MCImageBitmap *p_bitmap, uint8_t p_alpha)
{
	uint8_t *t_dst_ptr = (uint8_t*)p_bitmap->data;
	uindex_t t_height = p_bitmap->height;
	while (t_height--)
	{
		uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
		uindex_t t_width = p_bitmap->width;
		while (t_width--)
		{
			*t_dst_row = (*t_dst_row & 0x00FFFFFF) | p_alpha << 24;
			t_dst_row++;
		}
		t_dst_ptr += p_bitmap->stride;
	}
	
	p_bitmap->has_transparency = p_alpha != 0xFF;
	p_bitmap->has_alpha = p_alpha != 0x00 && p_alpha != 0xFF;
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageCreateCompressedBitmap(uint32_t p_compression, MCImageCompressedBitmap *&r_compressed)
{
	if (!MCMemoryNew(r_compressed))
		return false;

	r_compressed->compression = p_compression;
	return true;
}

bool MCImageCopyCompressedBitmap(MCImageCompressedBitmap *p_src, MCImageCompressedBitmap *&r_dst)
{
	bool t_success = true;

	MCImageCompressedBitmap *t_copy = nil;

	t_success = MCImageCreateCompressedBitmap(p_src->compression, t_copy);
	if (t_success && p_src->data != nil)
	{
		t_copy->size = p_src->size;
		t_success = MCMemoryAllocateCopy(p_src->data, p_src->size, t_copy->data);
	}

	if (t_success && p_src->colors != nil)
	{
		t_copy->color_count = p_src->color_count;
		t_success = MCMemoryAllocateCopy(p_src->colors, sizeof(MCColor) * p_src->color_count, t_copy->colors);
	}

	if (t_success && p_src->mask != nil)
	{
		t_copy->mask_size = p_src->mask_size;
		t_success = MCMemoryAllocateCopy(p_src->mask, p_src->mask_size, t_copy->mask);
	}

	if (t_success && p_src->planes != nil)
	{
		t_success = MCMemoryNewArray(p_src->color_count, t_copy->planes) &&
			MCMemoryAllocateCopy(p_src->plane_sizes, sizeof(p_src->plane_sizes[0]) * p_src->color_count, t_copy->plane_sizes);

		for (uindex_t i = 0; t_success && i < p_src->color_count; i++)
			t_success = MCMemoryAllocateCopy(p_src->planes[i], p_src->plane_sizes[i], t_copy->planes[i]);
	}

	if (t_success)
	{
		t_copy->width = p_src->width;
		t_copy->height = p_src->height;

		r_dst = t_copy;
	}
	else
		MCImageFreeCompressedBitmap(t_copy);

	return t_success;
}

void MCImageFreeCompressedBitmap(MCImageCompressedBitmap *p_compressed)
{
	if (p_compressed != nil)
	{
		MCMemoryDeleteArray(p_compressed->data);

		if (p_compressed->planes != nil)
		{
			for (uindex_t i = 0; i < p_compressed->color_count; i++)
				MCMemoryDeleteArray(p_compressed->planes[i]);
		}
		MCMemoryDeleteArray(p_compressed->planes);
		MCMemoryDeleteArray(p_compressed->plane_sizes);

		MCMemoryDeleteArray(p_compressed->colors);
		MCMemoryDeleteArray(p_compressed->mask);

		MCMemoryDelete(p_compressed);
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageCreateClipboardData(MCImageBitmap *p_bitmap, MCDataRef &r_data)
{
	bool t_success = true;
	
	IO_handle t_stream = nil;
	
	void *t_bytes = nil;
	uindex_t t_byte_count = 0;
	
	t_success = nil != (t_stream = MCS_fakeopenwrite());
	
	if (t_success)
		t_success = MCImageEncodePNG(p_bitmap, nil, t_stream, t_byte_count);
	
	if (t_stream != nil && IO_NORMAL != MCS_closetakingbuffer_uint32(t_stream, t_bytes, t_byte_count))
		t_success = false;
	
	if (t_success)
		t_success = MCDataCreateWithBytesAndRelease((char_t*)t_bytes, t_byte_count, r_data);
	
	if (!t_success)
		MCMemoryDeallocate(t_bytes);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
