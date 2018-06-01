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


#include "util.h"
#include "undolst.h"
#include "sellst.h"
#include "image.h"
#include "button.h"
#include "stack.h"
#include "card.h"
#include "mcerror.h"
#include "objectstream.h"
#include "osspec.h"

#include "context.h"
#include "exec.h"

#include "globals.h"

#include "resolution.h"

#include "stackfileformat.h"

////////////////////////////////////////////////////////////////////////////////

#define IMAGE_EXTRA_CONTROLCOLORS_DEAD (1 << 0) // Due to a bug, this cannot be used.
#define IMAGE_EXTRA_CONTROLPIXMAPS_DEAD (1 << 1) // Due to a bug, this cannot be used.

#define IMAGE_EXTRA_CONTROLCOLORS (1 << 2)
#define IMAGE_EXTRA_CENTERRECT (1 << 3)

////////////////////////////////////////////////////////////////////////////////

int2 MCImage::magmx;
int2 MCImage::magmy;
MCRectangle MCImage::magrect;
MCObject *MCImage::magtoredraw;
Boolean MCImage::filledborder;
MCBrush MCImage::brush;
MCBrush MCImage::spray;
MCBrush MCImage::eraser;
MCCursorRef MCImage::cursor;
MCCursorRef MCImage::defaultcursor;
uint2 MCImage::cmasks[MAX_CMASK + 1] = {0x00, 0x01, 0x03, 0x07,
                                        0x0F, 0x1F, 0x3F, 0x7F};

bool MCImage::s_have_control_colors;
uint16_t MCImage::s_control_color_count;
MCColor *MCImage::s_control_colors;
MCStringRef *MCImage::s_control_color_names;
uint16_t MCImage::s_control_pixmap_count;
MCPatternInfo *MCImage::s_control_pixmapids;
uint16_t MCImage::s_control_color_flags;

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCImage::kProperties[] =
{
	DEFINE_RW_OBJ_PROPERTY(P_XHOT, Int16, MCImage, XHot)
	DEFINE_RW_OBJ_PROPERTY(P_YHOT, Int16, MCImage, YHot)
	DEFINE_RW_OBJ_PROPERTY(P_HOT_SPOT, Point, MCImage, HotSpot)
	DEFINE_RW_OBJ_PROPERTY(P_FILE_NAME, OptionalString, MCImage, FileName)
	DEFINE_RW_OBJ_PROPERTY(P_ALWAYS_BUFFER, Bool, MCImage, AlwaysBuffer)
	DEFINE_RW_OBJ_PROPERTY(P_IMAGE_PIXMAP_ID, OptionalUInt16, MCImage, ImagePixmapId)
	DEFINE_RW_OBJ_PROPERTY(P_MASK_PIXMAP_ID, OptionalUInt16, MCImage, MaskPixmapId)
	DEFINE_RW_OBJ_PROPERTY(P_DONT_DITHER, Bool, MCImage, DontDither)
	DEFINE_RW_OBJ_PROPERTY(P_MAGNIFY, Bool, MCImage, Magnify)
	DEFINE_RO_OBJ_PROPERTY(P_SIZE, UInt16, MCImage, Size)
	DEFINE_RW_OBJ_PROPERTY(P_CURRENT_FRAME, UInt16, MCImage, CurrentFrame)
	DEFINE_RO_OBJ_PROPERTY(P_FRAME_COUNT, Int16, MCImage, FrameCount)
	DEFINE_RW_OBJ_PROPERTY(P_PALINDROME_FRAMES, Bool, MCImage, PalindromeFrames)
	DEFINE_RW_OBJ_PROPERTY(P_CONSTANT_MASK, Bool, MCImage, ConstantMask)
	DEFINE_RW_OBJ_PROPERTY(P_REPEAT_COUNT, Int16, MCImage, RepeatCount)
	DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_HEIGHT, Int16, MCImage, FormattedHeight)
	DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_WIDTH, Int16, MCImage, FormattedWidth)
	DEFINE_RW_OBJ_PROPERTY(P_TEXT, BinaryString, MCImage, Text)
	DEFINE_RW_OBJ_PROPERTY(P_IMAGE_DATA, BinaryString, MCImage, ImageData)
	DEFINE_RW_OBJ_PROPERTY(P_MASK_DATA, BinaryString, MCImage, MaskData)
	DEFINE_RW_OBJ_PROPERTY(P_ALPHA_DATA, BinaryString, MCImage, AlphaData)
	DEFINE_RW_OBJ_ENUM_PROPERTY(P_RESIZE_QUALITY, InterfaceImageResizeQuality, MCImage, ResizeQuality)
	DEFINE_RO_OBJ_ENUM_PROPERTY(P_PAINT_COMPRESSION, InterfaceImagePaintCompression, MCImage, PaintCompression)
	DEFINE_RW_OBJ_PROPERTY(P_ANGLE, Int16, MCImage, Angle)
    DEFINE_RW_OBJ_PROPERTY(P_CENTER_RECTANGLE, OptionalRectangle, MCImage, CenterRectangle)
    DEFINE_RO_OBJ_RECORD_PROPERTY(P_METADATA, MCImage, Metadata)

};

MCObjectPropertyTable MCImage::kPropertyTable =
{
	&MCControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCImage::MCImage()
{
	angle = 0;
	flags &= ~(F_SHOW_BORDER | F_TRAVERSAL_ON);
    
	m_rep = nil;
	m_resampled_rep = nil;
	m_image_opened = false;
	m_has_transform = false;

	// MW-2013-10-25: [[ Bug 11300 ]] Images start off unflipped.
	m_flip_x = false;
	m_flip_y = false;

	m_locked_rep = nil;
	m_locked_image = nil;
	m_locked_bitmap = nil;

	m_needs = nil;

	filename = MCValueRetain(kMCEmptyString);

	xhot = yhot = 1;
	currentframe = 0;
	repeatcount = 0;
	resizequality = INTERPOLATION_BOX;
    
    m_center_rect = MCRectangleMake(INT16_MIN, INT16_MIN, UINT16_MAX, UINT16_MAX);
    
    // MM-2014-07-31: [[ ThreadedRendering ]] Used to ensure the image animate message is only posted from a single thread.
    m_animate_posted = false;
    
}

MCImage::MCImage(const MCImage &iref) : MCControl(iref)
{
	m_rep = nil;
	m_resampled_rep = nil;
	m_image_opened = false;
	m_has_transform = false;

	// MW-2013-10-25: [[ Bug 11300 ]]  Images start off unflipped.
	m_flip_x = false;
	m_flip_y = false;
	
	m_locked_rep = nil;
	m_locked_image = nil;
	m_locked_bitmap = nil;
	m_needs = nil;
    
    filename = MCValueRetain(iref.filename);
	
    if (iref.isediting())
	{
		MCImageBitmap *t_bitmap = nil;
		/* UNCHECKED */static_cast<MCMutableImageRep*>(iref.m_rep)->copy_selection(t_bitmap);
		setbitmap(t_bitmap, 1.0);
		if (static_cast<MCMutableImageRep*>(iref.m_rep)->has_selection())
		{
			xhot = t_bitmap->width >> 1;
			yhot = t_bitmap->height >> 1;
		}
		MCImageFreeBitmap(t_bitmap);
	}
	else
	{
		xhot = iref.xhot;
		yhot = iref.yhot;
		if (iref . m_rep != nil)
			m_rep = iref . m_rep->Retain();
	}

	angle = iref.angle;
	currentframe = 0;
	repeatcount = iref.repeatcount;
	resizequality = iref.resizequality;
    
    m_center_rect = iref.m_center_rect;
    
    // MM-2014-07-31: [[ ThreadedRendering ]] Used to ensure the image animate message is only posted from a single thread.
    m_animate_posted = false;
}

MCImage::~MCImage()
{
	while (opened)
		close();

	if (m_needs != nil)
		notifyneeds(true);

	if (m_rep != nil)
	{
		m_rep->Release();
		m_rep = nil;
	}

	if (m_resampled_rep != nil)
	{
		m_resampled_rep->Release();
		m_resampled_rep = nil;
	}
	
	MCValueRelease(filename);
}

Chunk_term MCImage::gettype() const
{
	return CT_IMAGE;
}

const char *MCImage::gettypestring()
{
	return MCimagestring;
}

bool MCImage::visit_self(MCObjectVisitor* p_visitor)
{
    return p_visitor -> OnImage(this);
}

void MCImage::open()
{
	MCControl::open();
	
	// MW-2012-08-27: [[ Bug ]] Force image opening when there is no drawdata
	//   and buffer image is set.
	if ((opened == 1) && (MCbufferimages || flags & F_I_ALWAYS_BUFFER))
		openimage();
}

void MCImage::close()
{
	if (state & CS_OWN_SELECTION)
		endsel();
	if (state & CS_MAGNIFY)
		endmag(True);
	
	// IM-2014-06-26: [[ Bug 12702 ]] Make sure editing is finished when closing.
	recompress();
	
	if (opened == 1 && m_image_opened)
		closeimage();
	MCControl::close();
}

Boolean MCImage::mfocus(int2 x, int2 y)
{
	if (!(flags & F_VISIBLE || showinvisible())
	    || (flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE))
		return False;
	
	mx = x;
	my = y;
	
	if (state & CS_MAGNIFY && state & CS_MAG_DRAG)
	{
		MCRectangle brect;
		brect = MCU_reduce_rect(magrect, -MAG_WIDTH);
		magrect.x += mx - startx;
		magrect.y += my - starty;
		int2 oldx = magrect.x;
		int2 oldy = magrect.y;
		MCRectangle trect = MCU_intersect_rect(rect, getcard()->getrect());
		magrect = MCU_bound_rect(magrect, trect.x - rect.x, trect.y - rect.y,
		                         trect.width, trect.height);
		brect = MCU_union_rect(brect, MCU_reduce_rect(magrect, -MAG_WIDTH));
		startx = mx + (magrect.x - oldx);
		starty = my + (magrect.y - oldy);
		brect.x += rect.x;
		brect.y += rect.y;

		layer_redrawrect(brect);
		magredrawdest(brect);

		return True;
	}

	if (isediting() &&
		static_cast<MCMutableImageRep *>(m_rep) -> image_mfocus(x, y))
		return True;
	

	switch(getstack() -> gettool(this))
	{
	case T_BRUSH:
	case T_BUCKET:
	case T_CURVE:
	case T_ERASER:
	case T_LASSO:
	case T_LINE:
	case T_OVAL:
	case T_PENCIL:
	case T_POLYGON:
	case T_RECTANGLE:
	case T_REGULAR_POLYGON:
	case T_ROUND_RECT:
	case T_SELECT:
	case T_SPRAY:
	case T_TEXT:
		if (!iseditable() || !MCU_point_in_rect(rect, x, y))
			return False;
		message_with_args(MCM_mouse_move, x, y);
		break;
	case T_BROWSE:
	case T_POINTER:
	case T_IMAGE:
	case T_HELP:
		return MCControl::mfocus(x, y);
	default:
		return False;
	}
	return True;

}

Boolean MCImage::mdown(uint2 which)
{
	if (state & CS_MFOCUSED)
		return False;
		
	if (state & CS_MENU_ATTACHED)
		return MCObject::mdown(which);
	
	state |= CS_MFOCUSED;
	
	switch (which)
	{
	case Button3:
	case Button1:
			switch (getstack()->gettool(this))
			{
			case T_BROWSE:
				message_with_args(MCM_mouse_down, which);
				break;
			case T_POINTER:
			case T_IMAGE:
				if (which != Button1)
				{
					message_with_args(MCM_mouse_down, which);
					break;
				}
				if (state & CS_MAGNIFY)
					endmag(True);
				finishediting();
				start(True);
				if (state & CS_SIZE)
				{
					if (MCmodifierstate & MS_CONTROL)
					{ //cropping
						state |= CS_EDITED;
						m_current_width = rect.width;
						m_current_height = rect.height;
					}
					if (state & CS_IMAGE_PM)
					{
						state &= ~CS_SIZE;
						state |= CS_MOVE;
						MCselected->startmove(mx, my, False);
					}
				}
				break;
			case T_HELP:
				break;
			default:
				if (state & CS_MAGNIFY
						&& !MCU_point_in_rect(magrect, mx - rect.x, my - rect.y)
						&& MCU_point_in_rect(MCU_reduce_rect(magrect, -MAG_WIDTH),
											 mx - rect.x, my - rect.y))
				{
					startx = mx;
					starty = my;
					MCRectangle brect;
					state &= ~CS_MAGNIFY;
					brect = MCU_reduce_rect(magrect, -MAG_WIDTH);
					brect.x += rect.x;
					brect.y += rect.y;
					brect.width = (brect.width + (brect.x & 0x07) + 0x07) & ~0x07;
					brect.x &= ~0x07;
					// MW-2011-08-18: [[ Layers ]] Invalidate the brush rect.
					layer_redrawrect(brect);
					state |= CS_MAGNIFY | CS_MAG_DRAG;

					if (state & CS_MAGNIFY)
						magredrawdest(brect);
				}
				else
				{
					switch (getstack()->gettool(this))
					{
					case T_BRUSH:
					case T_SPRAY:
					case T_ERASER:
					case T_PENCIL:
						if (MCmodifierstate & MS_CONTROL)
						{
							if (state & CS_MAGNIFY)
								endmag(True);
							else
								startmag(mx - rect.x, my - rect.y);

							return True;
						}
					default:
						break;
					}

					if (isediting() &&
						static_cast<MCMutableImageRep *>(m_rep) -> image_mdown(which))
						return True;

					startediting(which);
				}

				break;
			}
		break;
	case Button2:
		message_with_valueref_args(MCM_mouse_down, MCSTR("2"));
		break;
	}
	return True;
}

Boolean MCImage::mup(uint2 which, bool p_release)
{
	if (!(state & CS_MFOCUSED))
		return False;

	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which, p_release);
		
	state &= ~CS_MFOCUSED;
	if (state & CS_GRAB)
	{
		ungrab(which);
		return True;
	}
	
	if (state & CS_MAGNIFY && state & CS_MAG_DRAG)
	{
		state &= ~CS_MAG_DRAG;
		return True;
	}

	if (isediting() &&
		static_cast<MCMutableImageRep *>(m_rep) -> image_mup(which))
		return True;

	switch(getstack() -> gettool(this))
	{
	case T_BROWSE:
		MCRectangle srect;
		MCU_set_rect(srect, mx, my, 1, 1);
		if (!p_release && maskrect(srect))
			message_with_args(MCM_mouse_up, which);
		else
			message_with_args(MCM_mouse_release, which);
		break;
	case T_IMAGE:
	case T_POINTER:
		{
			if (which != Button1)
			{
                if (p_release)
                    message_with_args(MCM_mouse_release, which);
                else
                    message_with_args(MCM_mouse_up, which);
				break;
			}
			uint32_t t_pixwidth, t_pixheight;
			getgeometry(t_pixwidth, t_pixheight);

			if ((t_pixwidth != rect.width || t_pixheight != rect.height)
					&& state & CS_SIZE && state & CS_EDITED)
			{
				crop(NULL);
			}
			// MM-2014-04-09: [[ Bug 11689 ]] Let end take care of unsetting the CS_SIZE flag.
			//  Doing so here prevents resize control being sent.
			state &= ~CS_EDITED;
			end(false, p_release);
			if (state & CS_MAGNIFY)
				magredrawdest(rect);
			if (!p_release && maskrect(srect))
				message_with_args(MCM_mouse_up, which);
            else
                message_with_args(MCM_mouse_release, which);
		}
		break;
	case T_HELP:
		help();
		break;
	default:
		break;
	}
	return True;
}

Boolean MCImage::doubledown(uint2 which)
{
	if (isediting() && static_cast<MCMutableImageRep*>(m_rep)->image_doubledown(which) == True)
		return True;
	return MCControl::doubledown(which);
}

Boolean MCImage::doubleup(uint2 which)
{
	if (isediting() && static_cast<MCMutableImageRep*>(m_rep)->image_doubleup(which) == True)
		return True;
	return MCControl::doubleup(which);
}

void MCImage::timer(MCNameRef mptr, MCParameter *params)
{
	if (MCNameIsEqualToCaseless(mptr, MCM_internal))
	{
		if (state & CS_OWN_SELECTION)
		{
			dashoffset++;
			dashoffset &= 0x07;
			MCRectangle trect = selrect;
			trect.x += rect.x;
			trect.y += rect.y;
			// MW-2011-08-18: [[ Layers ]] Redraw the affected rect.
			layer_redrawrect(trect);
			if (state & CS_MAGNIFY)
				magredrawdest(trect);
			MCscreen->addtimer(this, MCM_internal, MCmovespeed);
		}
		else
			if ((isvisible() || m_needs) && irepeatcount && m_rep != nil && m_rep->GetFrameCount() > 1)
			{
                // MM-2014-07-31: [[ ThreadedRendering ]] Flag that there is no longer an image animation message pending.
                m_animate_posted = false;
                
				advanceframe();
				if (irepeatcount)
				{
					// IM-2014-11-25: [[ ImageRep ]] Use ImageRep method to get frame duration
					uint32_t t_frame_duration;
					if (m_rep->GetFrameDuration(currentframe, t_frame_duration))
						MCscreen->addtimer(this, MCM_internal, t_frame_duration);
				}
			}
	}
	else if (MCNameIsEqualToCaseless(mptr, MCM_internal2))
		{
			if (state & CS_MAGNIFY)
			{
				switch (getstack()->gettool(this))
				{
				case T_BROWSE:
				case T_POINTER:
				case T_IMAGE:
				case T_BRUSH:
				case T_BUCKET:
				case T_CURVE:
				case T_DROPPER:
				case T_ERASER:
				case T_LASSO:
				case T_LINE:
				case T_OVAL:
				case T_PENCIL:
				case T_POLYGON:
				case T_RECTANGLE:
				case T_REGULAR_POLYGON:
				case T_ROUND_RECT:
				case T_SELECT:
				case T_SPRAY:
				case T_TEXT:
					{
						if (!(state & CS_OWN_SELECTION))
						{
							dashoffset++;
							dashoffset &= 0x07;
						}
						MCRectangle trect = MCU_reduce_rect(magrect, -MAG_WIDTH);
						trect.x += rect.x;
						trect.y += rect.y;
						// MW-2011-08-18: [[ Layers ]] Redraw the affected rect.
						layer_redrawrect(trect);
						MCscreen->addtimer(this, MCM_internal2, MCmovespeed);
					}
					break;
				default:
					endmag(True);
					break;
				}
			}
		}
		else
			MCControl::timer(mptr, params);
}

void MCImage::applyrect(const MCRectangle &nrect)
{
	MCRectangle orect = rect;
	rect = nrect;

	// IM-2013-12-17: [[ Bug 11604 ]] Notify mutable image rep of change in image rect
	if (m_rep != nil && m_rep->GetType() == kMCImageRepMutable)
		static_cast<MCMutableImageRep*>(m_rep)->owner_rect_changed(rect);
	
	if (!(state & CS_SIZE) || !(state & CS_EDITED))
	{
		// IM-2013-04-15: [[ BZ 10827 ]] if the image has rotation then apply_transform()
		// will reset the rect otherwise it will stay as set, in which case we can avoid
		// the call to apply_transform() and any costly image loading that might cause
		if (angle != 0)
		apply_transform();
		if ((rect.width != orect.width || rect.height != orect.height) && m_rep != nil)
		{
			layer_rectchanged(orect, true);
			notifyneeds(false);
		}
	}
}

void MCImageSetMask(MCImageBitmap *p_bitmap, uint8_t *p_mask_data, uindex_t p_mask_size, bool p_is_alpha)
{
	uint32_t t_mask_stride = MCMin(p_mask_size / p_bitmap->height, p_bitmap->width);
	uint32_t t_width = t_mask_stride;

	uint8_t *t_src_ptr = p_mask_data;
	uint8_t *t_dst_ptr = (uint8_t*)p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint8_t *t_src_row = t_src_ptr;
		uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
		for (uindex_t x = 0; x < t_width; x++)
		{
			uint8_t t_r, t_g, t_b, t_alpha;
			MCGPixelUnpackNative(*t_dst_row, t_r, t_g, t_b, t_alpha);
			
			t_alpha = *t_src_row++;
			
			// with maskdata, nonzero is fully opaque
			if (!p_is_alpha && t_alpha > 0)
				t_alpha = 0xFF;
			*t_dst_row++ = MCGPixelPackNative(t_r, t_g, t_b, t_alpha);
		}
		t_src_ptr += t_mask_stride;
		t_dst_ptr += p_bitmap->stride;
	}

	MCImageBitmapCheckTransparency(p_bitmap);
}

void MCImage::select()
{
	MCControl::select();
	if (state & CS_MAGNIFY)
		magredrawdest(rect);
}

void MCImage::deselect()
{
	MCControl::deselect();
	if (state & CS_MAGNIFY)
		magredrawdest(rect);
}

void MCImage::undo(Ustruct *us)
{
	switch (us->type)
	{
	case UT_PAINT:
		static_cast<MCMutableImageRep*>(m_rep)->image_undo(us);
		break;
	default:
		MCControl::undo(us);
	}
}

void MCImage::freeundo(Ustruct *us)
{
	switch (us->type)
	{
	case UT_PAINT:
		if (m_rep != nil && m_rep->GetType() == kMCImageRepMutable)
			static_cast<MCMutableImageRep*>(m_rep)->image_freeundo(us);
		break;
	default:
		MCControl::freeundo(us);
		break;
	}
}

MCControl *MCImage::clone(Boolean attach, Object_pos p, bool invisible)
{
	recompress();
	MCImage *newiptr = new (nothrow) MCImage(*this);
	if (attach)
		newiptr->attach(p, invisible);
	return newiptr;
}

Boolean MCImage::maskrect(const MCRectangle &srect)
{
	if (!(flags & F_VISIBLE || showinvisible()))
		return False;
	MCRectangle drect = MCU_intersect_rect(srect, rect);
	if (drect.width == 0 || drect.height == 0)
		return False;
	if (srect.width != 1 || srect.height != 1)
		return True; // selection rect is not masked
	if (state & CS_IMAGE_PM && !(state & CS_MASK_PM))
		return True;

	// MW-2007-09-11: [[ Bug 5177 ]] If the object is currently selected, make its mask the whole rectangle
	MCGImageFrame t_frame;
	if (!getstate(CS_SELECTED) && m_rep != nil && m_rep->LockImageFrame(currentframe, getdevicescale(), t_frame))
	{
		int32_t t_x = srect.x - rect.x;
		int32_t t_y = srect.y - rect.y;
		if (m_has_transform)
		{
			MCGAffineTransform t_inverted = MCGAffineTransformInvert(m_transform);
			MCGPoint t_src_point = MCGPointApplyAffineTransform(MCGPointMake(t_x, t_y), t_inverted);
			t_x = t_src_point.x;
			t_y = t_src_point.y;
		}
		
		// IM-2013-10-30: [[ FullscreenMode ]] Account for image density when locating pixel position
		// IM-2014-08-07: [[ Bug 13021 ]] Split density into x / y scale components
		t_x = t_x * t_frame.x_scale;
		t_y = t_y * t_frame.y_scale;
		
		uint32_t t_width, t_height;
		t_width = MCGImageGetWidth(t_frame.image);
		t_height = MCGImageGetHeight(t_frame.image);

		uint32_t t_pixel = 0;
		if (t_x >= 0 && t_y >= 0 && t_x < t_width && t_y < t_height)
			MCGImageGetPixel(t_frame.image, t_x, t_y, t_pixel);

		m_rep->UnlockImageFrame(currentframe, t_frame);
		return MCGPixelGetNativeAlpha(t_pixel) != 0;
	}
	else
		return True;
}

// MW-2011-09-20: [[ Collision ]] The image's shape depends on its properties.
bool MCImage::lockshape(MCObjectShape& r_shape)
{
	// Make sure we consider the case where only the image bits are rendered.
	if (getflag(F_SHOW_BORDER) ||
	    (getstate(CS_KFOCUSED) && (extraflags & EF_NO_FOCUS_BORDER) == 0) ||
		getcompression() == F_PICT || m_rep == nil)
	{
		r_shape . type = kMCObjectShapeComplex;
		r_shape . bounds = getrect();
		return true;
	}
	
	MCRectangle t_bounds;
	t_bounds = getrect();
	
	MCPoint t_origin;
	t_origin = MCPointMake(t_bounds.x, t_bounds.y);

	return lockbitmapshape(t_bounds, t_origin, r_shape);
}

// IM-2013-10:16: [[ ResIndependence ]] Split out image bitmap shape functionality for use
// when locking button icon shapes
bool MCImage::lockbitmapshape(const MCRectangle &p_bounds, const MCPoint &p_origin, MCObjectShape &r_shape)
{
	bool t_mask, t_alpha;
	MCImageBitmap *t_bitmap = nil;
	
	if (!lockbitmap(t_bitmap, true))
		return false;
	
	t_mask = MCImageBitmapHasTransparency(t_bitmap, t_alpha);
	
	// If the image has no mask, then it is a solid rectangle.
	if (!t_mask)
	{
		MCRectangle t_rect;
		t_rect = getrect();
		
		r_shape . type = kMCObjectShapeRectangle;
		r_shape . bounds = p_bounds;
		r_shape . rectangle = MCRectangleMake(p_origin . x, p_origin . y, t_rect . width, t_rect . height);
		unlockbitmap(t_bitmap);
		return true;
	}
	
	// Otherwise we have a nice mask to pass back!
	// IM-2014-08-01: [[ Bug 13021 ]] lockbitmap() always returns an image with 1.0 scale
	r_shape . type = kMCObjectShapeMask;
	r_shape . bounds = p_bounds;
	r_shape . mask . origin . x = p_origin . x;
	r_shape . mask . origin . y = p_origin . y;
	r_shape . mask . bits = t_bitmap;
	r_shape . mask . scale = 1.0;
	return true;
}

void MCImage::unlockshape(MCObjectShape& p_shape)
{
	if (p_shape . type == kMCObjectShapeMask)
		unlockbitmap(p_shape . mask . bits);
}

//-----------------------------------------------------------------------------
//  Redraw Management

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCImage::draw(MCDC *dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite)
{
	MCRectangle dirty;
	dirty = p_dirty;

	/* OVERHAUL - REVISIT - not sure setting F_OPAQUE has any effect here */
	//if (maskimagealpha != NULL)
	//	flags &= ~F_OPAQUE;
	//else
	//	flags |= F_OPAQUE;
	
	if (!p_isolated)
	{
		// MW-2011-09-06: [[ Redraw ]] If rendering as a sprite, don't change opacity or ink.
		if (!p_sprite)
		{
			dc -> setfunction(ink);
			dc -> setopacity(blendlevel * 255 / 100);
		}
	}

	bool t_need_group = false;
	if (!p_isolated)
	{
		// MW-2009-06-10: [[ Bitmap Effects ]]
		t_need_group = getflag(F_SHOW_BORDER) || getstate(CS_KFOCUSED) || getcompression() == F_PICT || m_bitmap_effects != NULL;
		if (t_need_group)
		{
			if (m_bitmap_effects == NULL)
				dc -> begin(false);
			else
			{
				if (!dc -> begin_with_effects(m_bitmap_effects, rect))
					return;
				dirty = dc -> getclip();
			}
		}
	}

	int2 xorigin;
	int2 yorigin;

	MCRectangle trect;
	trect = MCU_intersect_rect(dirty, getrect());
	uint32_t t_pixwidth, t_pixheight;
	getgeometry(t_pixwidth, t_pixheight);
	if (state & CS_SIZE && state & CS_EDITED && (t_pixwidth != rect.width || t_pixheight != rect.height))
		compute_gravity(trect, xorigin, yorigin);
	else
	{
		xorigin = trect.x - rect.x;
		yorigin = trect.y - rect.y;
	}
	drawme(dc, xorigin, yorigin, trect.width, trect.height, trect.x, trect.y, trect.width, trect.height);

	if (getflag(F_SHOW_BORDER))
	{
		if (getflag(F_3D))
			draw3d(dc, rect, ETCH_RAISED, borderwidth);
		else
			drawborder(dc, rect, borderwidth);
	}

	if (getstate(CS_KFOCUSED))
		drawfocus(dc, p_dirty);

	if (!p_isolated)
	{
		if (t_need_group)
			dc -> end();

		if (isediting())
			static_cast<MCMutableImageRep*>(m_rep)->drawsel(dc);

		if (getstate(CS_MAGNIFY))
			drawmagrect(dc);
	}
}

//  Redraw Management
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

IO_stat MCImage::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	// Extended data area for an image consists of:
	//   uint1 resize_quality;
	// Extended data area for an image consists of:
	//   tag image_extensions
	//   if (tag & IMAGE_EXTRA_CONTROLCOLORS)
	//     uint2 ncolors
	//     uint2 dflags
	//     color[ncolors] colors
	//     char *[ncolors] colornames
	//
	//   MCObject::extensions
	IO_stat t_stat;
	t_stat = p_stream . WriteU8(resizequality);

	uint4 t_flags;
	t_flags = 0;

	uint4 t_length;
	t_length = 0;

	// MW-2013-09-05: [[ Bug 11127 ]] If we have control colors then write them out
	//   in two sections (first the colors, if any, then the patterns, if any).
	if (s_have_control_colors)
	{
		t_flags |= IMAGE_EXTRA_CONTROLCOLORS;
        // increase t_length to accommodate s_control_color_count and s_control_color_flags
		t_length += sizeof(uint16_t) + sizeof(uint16_t);
        // increase t_length to accommodate the color
        t_length += s_control_color_count * 3 * sizeof(uint16_t);
        
		for (uint16_t i = 0; i < s_control_color_count; i++)
			if (s_control_color_names[i] != nil)
            {
                // FG-2014-10-17: [[ Bugfix 13706 ]]
                // Calculate the correct size for 7.0+ style strings
                // SN-2014-10-27: [[ Bug 13554 ]] String length calculation refactored
                t_length += p_stream . MeasureStringRefNew(s_control_color_names[i], p_version >= kMCStackFileFormatVersion_7_0);
            }
			else
                // AL-2014-11-07: [[ Bug 13851 ]] Measure empty string if the color name is nil
                t_length += p_stream . MeasureStringRefNew(kMCEmptyString, p_version >= kMCStackFileFormatVersion_7_0);
	
		t_length += sizeof(uint16_t);
		t_length += s_control_pixmap_count * sizeof(uint4);
	}
    
    if (m_center_rect . x != INT16_MIN)
    {
        t_flags |= IMAGE_EXTRA_CENTERRECT;
        t_length += sizeof(MCRectangle);
    }

	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteTag(t_flags, t_length);
	
	if (t_stat == IO_NORMAL && (t_flags & IMAGE_EXTRA_CONTROLCOLORS) != 0)
	{
		t_stat = p_stream . WriteU16(s_control_color_count);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteU16(s_control_color_flags);

		for (uint16_t i = 0; t_stat == IO_NORMAL && i < s_control_color_count; i++)
			t_stat = p_stream . WriteColor(s_control_colors[i]);
		// MW-2013-12-05: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
		for (uint16_t i = 0; t_stat == IO_NORMAL && i < s_control_color_count; i++)
			t_stat = p_stream . WriteStringRefNew(s_control_color_names[i] != nil ? s_control_color_names[i] : kMCEmptyString, p_version >= kMCStackFileFormatVersion_7_0);
		
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteU16(s_control_pixmap_count);
		
		if (t_stat == IO_NORMAL)
			for(int i = 0; t_stat == IO_NORMAL && i < s_control_pixmap_count; i++)
				t_stat = p_stream . WriteU32(s_control_pixmapids[i] . id);
	}

    if (t_stat == IO_NORMAL && (t_flags & IMAGE_EXTRA_CENTERRECT) != 0)
    {
        t_stat = p_stream . WriteS16(m_center_rect . x);
		if (t_stat == IO_NORMAL)
            t_stat = p_stream . WriteS16(m_center_rect . y);
		if (t_stat == IO_NORMAL)
            t_stat = p_stream . WriteU16(m_center_rect . width);
		if (t_stat == IO_NORMAL)
            t_stat = p_stream . WriteU16(m_center_rect . height);
    }
    
	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedsave(p_stream, p_part, p_version);

	return t_stat;
}

IO_stat MCImage::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_remaining)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;
    
	if (p_remaining >= 1)
	{
		t_stat = checkloadstat(p_stream . ReadU8(resizequality));
		
		if (t_stat == IO_NORMAL)
			p_remaining -= 1;
	}

	if (p_remaining > 0)
	{
		uint4 t_flags, t_length, t_header_length;
		t_stat = checkloadstat(p_stream . ReadTag(t_flags, t_length, t_header_length));
        
		if (t_stat == IO_NORMAL)
			t_stat = checkloadstat(p_stream . Mark());
		
		// MW-2013-09-05: [[ Bug 11127 ]] If we have control colors then read them in
		//   (first do colors, then pixmapids - if any).
		if (t_stat == IO_NORMAL && (t_flags & IMAGE_EXTRA_CONTROLCOLORS) != 0)
		{
			s_have_control_colors = true;
			t_stat = checkloadstat(p_stream . ReadU16(s_control_color_count));
			t_stat = checkloadstat(p_stream . ReadU16(s_control_color_flags));

            if (t_stat == IO_NORMAL)
			{
				s_control_colors = new (nothrow) MCColor[s_control_color_count];
				s_control_color_names = new (nothrow) MCStringRef[s_control_color_count];
				if (nil == s_control_colors || nil == s_control_color_names)
					t_stat = checkloadstat(IO_ERROR);
			}

			for (uint32_t i = 0; t_stat == IO_NORMAL && i < s_control_color_count; i++)
				t_stat = checkloadstat(p_stream . ReadColor(s_control_colors[i]));
			for (uint32_t i = 0; t_stat == IO_NORMAL && i < s_control_color_count; i++)
			{
				// MW-2013-12-05: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
				t_stat = checkloadstat(p_stream . ReadStringRefNew(s_control_color_names[i], p_version >= kMCStackFileFormatVersion_7_0));
				if (t_stat == IO_NORMAL && MCStringIsEmpty(s_control_color_names[i]))
				{
					MCValueRelease(s_control_color_names[i]);
					s_control_color_names[i] = nil;
				}
			}
            if (t_stat == IO_NORMAL)
				t_stat = checkloadstat(p_stream . ReadU16(s_control_pixmap_count));
			
			if (t_stat == IO_NORMAL && 
				!MCMemoryNewArray(s_control_pixmap_count, s_control_pixmapids))
				t_stat = checkloadstat(IO_ERROR);
			for(uint32_t i = 0; t_stat == IO_NORMAL && i < s_control_pixmap_count; i++)
				t_stat = checkloadstat(p_stream . ReadU32(s_control_pixmapids[i] . id));
		}
        
        if (t_stat == IO_NORMAL && (t_flags & IMAGE_EXTRA_CENTERRECT) != 0)
        {
            t_stat = checkloadstat(p_stream . ReadS16(m_center_rect . x));
            if (t_stat == IO_NORMAL)
                t_stat = checkloadstat(p_stream . ReadS16(m_center_rect . y));
            if (t_stat == IO_NORMAL)
                t_stat = checkloadstat(p_stream . ReadU16(m_center_rect . width));
            if (t_stat == IO_NORMAL)
                t_stat = checkloadstat(p_stream . ReadU16(m_center_rect . height));
        }

		if (t_stat == IO_NORMAL)
			t_stat = checkloadstat(p_stream . Skip(t_length));

		if (t_stat == IO_NORMAL)
			p_remaining -= t_length + t_header_length;
	}

	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedload(p_stream, p_version, p_remaining);

	if (t_stat != IO_NORMAL)
	{
		delete[] s_control_colors;
		delete[] s_control_color_names;
	}

	return t_stat;
}

IO_stat MCImage::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;

	recompress();
	if ((stat = IO_write_uint1(OT_IMAGE, stream)) != IO_NORMAL)
		return stat;
	
	// MW-2013-09-05: [[ Bug 11127 ]] The object colors/pixmaps pertain to an RLE
	//   compressed image (if this is one); whereas the control colors are stored
	//   in an extended record. When in memory the object stores the control colors
	//   in colors/pixmapids so we temporarily switch these here. So the object
	//   writes out the image colors; and the image does an extended record with the
	//   control colors.
	s_have_control_colors = false;
	if (ncolors != 0 || npatterns != 0 ||
		(m_rep != nil && m_rep -> GetType() == kMCImageRepCompressed))
	{
		s_have_control_colors = true;
		s_control_color_count = ncolors;
		s_control_colors = colors;
		s_control_color_names = colornames;
		s_control_color_flags = dflags;
		s_control_pixmap_count = npatterns;
		s_control_pixmapids = patterns;

		if (m_rep != nil && m_rep -> GetType() == kMCImageRepCompressed)
		{
			MCImageCompressedBitmap *t_compressed;
			t_compressed = static_cast<MCCompressedImageRep*>(m_rep)->GetCompressed();
			
			ncolors = t_compressed->color_count;
			colors = t_compressed->colors;
			dflags = MCImage::cmasks[MCMin(ncolors, MAX_CMASK)];
			if (!MCMemoryNewArray(ncolors, colornames))
				return IO_ERROR;
		}
		else
		{
			ncolors = 0;
			colors = nil;
			colornames = nil;
			dflags = 0;
		}
		
		npatterns = 0;
		patterns = 0;
	}
	
	uint32_t t_pixwidth, t_pixheight;
	t_pixwidth = t_pixheight = 0;
	
	// IM-2013-12-05: [[ Bug 11551 ]] We only need to get the geometry of an image rep if it's an rle-compressed image
	if (m_rep != nil && m_rep->GetType() == kMCImageRepCompressed)
	{
		m_rep->GetGeometry(t_pixwidth, t_pixheight);
		if (rect.width != t_pixwidth || rect.height != t_pixheight)
			flags |= F_SAVE_SIZE;
	}

	uint1 t_old_ink = ink;

//--- pre-2.7 Conversion
	if (p_version < kMCStackFileFormatVersion_2_7)
	{
		if (ink == GXblendSrcOver)
		{
			if (blendlevel != 50)
				ink = (100 - blendlevel) | 0x80;
			else
				ink = GXblend;
		}
	}
//----

	bool t_has_extension = false;
	if (resizequality != INTERPOLATION_BOX)
		t_has_extension = true;
	if (s_have_control_colors)
		t_has_extension = true;
    if (m_center_rect . x != INT16_MIN)
        t_has_extension = true;
    
	uint4 oldflags = flags;
	if (flags & F_HAS_FILENAME)
		flags &= ~(F_TRUE_COLOR | F_COMPRESSION | F_NEED_FIXING);
	stat = MCControl::save(stream, p_part, t_has_extension || p_force_ext, p_version);

	flags = oldflags;

	ink = t_old_ink;
	
	// MW-2013-09-05: [[ Bug 11127 ]] Now we've written out the control colors and
	//   object colors, reset the in-memory references to the control colors.
	if (s_have_control_colors)
	{
		MCMemoryDeleteArray(colornames);

		ncolors = s_control_color_count;
		colors = s_control_colors;
		colornames = s_control_color_names;
		npatterns = s_control_pixmap_count;
		patterns = s_control_pixmapids;
		dflags = s_control_color_flags;

		s_control_colors = nil;
		s_control_color_names = nil;
		s_control_color_count = 0;
		s_control_pixmap_count = 0;
		s_control_pixmapids = nil;

		s_have_control_colors = false;
	}

	if (stat != IO_NORMAL)
		return stat;

	
	if (flags & F_HAS_FILENAME)
    {
		// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
        if ((stat = IO_write_stringref_new(filename, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return stat;
	}
	else
	{
		if (m_rep != nil)
		{
			MCImageRepType t_type = m_rep->GetType();
			void *t_data = nil;
			uindex_t t_size = 0;
			void *t_mask_data = nil;
			uindex_t t_mask_size = 0;
			MCImageCompressedBitmap *t_compressed = nil;

			if (t_type == kMCImageRepResident)
				static_cast<MCResidentImageRep*>(m_rep)->GetData(t_data, t_size);
			else if (t_type == kMCImageRepVector)
				static_cast<MCVectorImageRep*>(m_rep)->GetData(t_data, t_size);
			else if (t_type == kMCImageRepCompressed)
			{
				t_compressed = static_cast<MCCompressedImageRep*>(m_rep)->GetCompressed();
				if (t_compressed->size > 0)
				{
					t_data = t_compressed->data;
					t_size = t_compressed->size;
				}
			}

			if (t_size > 0)
			{
				if (flags & F_REPEAT_COUNT)
					if ((stat = IO_write_int2(repeatcount, stream)) != IO_NORMAL)
						return stat;
				if ((stat = IO_write_uint4(t_size, stream)) != IO_NORMAL)
					return stat;
				if ((stat = IO_write(t_data, sizeof(uint1), t_size, stream)) != IO_NORMAL)
					return stat;
			}
			else if (t_compressed != nil)
			{
				uint2 i;
				for (i = 0 ; i < t_compressed->color_count ; i++)
				{
					if ((stat = IO_write_uint4(t_compressed->plane_sizes[i], stream)) != IO_NORMAL)
						return stat;
					if ((stat = IO_write(t_compressed->planes[i], sizeof(uint1),
					                     t_compressed->plane_sizes[i], stream)) != IO_NORMAL)
						return stat;
				}
			}
			// IM-2013-07-29: [[ Bugfix 11073 ]] If compressed data has a mask make sure we write it out
			if (t_compressed != nil)
			{
				t_mask_data = t_compressed->mask;
				t_mask_size = t_compressed->mask_size;
			}
			if ((stat = IO_write_uint4(t_mask_size, stream)) != IO_NORMAL)
				return stat;

			if (t_mask_size != 0)
				if ((stat = IO_write(t_mask_data, sizeof(uint1), t_mask_size, stream)) != IO_NORMAL)
					return stat;
			if (flags & F_SAVE_SIZE)
			{
				if ((stat = IO_write_uint2(t_pixwidth, stream)) != IO_NORMAL)
					return stat;
				if ((stat = IO_write_uint2(t_pixheight, stream)) != IO_NORMAL)
					return stat;
			}
		}
	}
	if ((stat = IO_write_uint2(xhot, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(yhot, stream)) != IO_NORMAL)
		return stat;
	if (flags & F_ANGLE)
		if ((stat = IO_write_uint2(angle, stream)) != IO_NORMAL)
			return stat;
	return savepropsets(stream, p_version);
}

IO_stat MCImage::load(IO_handle stream, uint32_t version)
{
	IO_stat stat = IO_NORMAL;

	resizequality = INTERPOLATION_BOX;
	
	// MW-2013-09-05: [[ Bug 11127 ]] Make sure the control color statics are reset.
	MCMemoryDeleteArray(s_control_colors);
	s_control_colors = nil;
	MCMemoryDeleteArray(s_control_color_names);
	s_control_color_names = nil;
	s_control_color_count = 0;
	s_control_pixmap_count = 0;
	MCMemoryDeleteArray(s_control_pixmapids);
	s_control_pixmapids = nil;
	s_control_color_flags = 0;
	s_have_control_colors = false;
	
	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return stat;

//---- Conversion from pre-2.7 behaviour to new behaviour
	if ((ink & 0x80) != 0 && version < kMCStackFileFormatVersion_2_7)
	{
		blendlevel = 100 - (ink & 0x7F);
		ink = GXblendSrcOver;
	}
	
	if (flags & F_HAS_FILENAME)
	{
		// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
		MCAutoStringRef t_filename;
		if ((stat = IO_read_stringref_new(&t_filename, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return stat;

		/* UNCHECKED */ setfilename(*t_filename);
	}
	else
		if (ncolors || flags & F_COMPRESSION || flags & F_TRUE_COLOR)
		{
			MCImageCompressedBitmap *t_compressed = nil;
			if (!MCImageCreateCompressedBitmap(flags & F_COMPRESSION, t_compressed))
				return IO_ERROR;
			
			if (ncolors > MAX_PLANES || flags & F_COMPRESSION
			        || flags & F_TRUE_COLOR)
			{
				if (IO_NORMAL == stat)
				{
					// IM-2013-04-12: [[ BZ 10843 ]] Initialize to -1 to indicate no repeat count has been set
					repeatcount = -1;
					if (flags & F_REPEAT_COUNT)
						stat = IO_read_int2(&repeatcount, stream);
				}
				
				if (IO_NORMAL == stat)
					stat = IO_read_uint4(&t_compressed->size, stream);
					 
				if (IO_NORMAL == stat)
				{
					 if (!MCMemoryAllocate(t_compressed->size, t_compressed->data))
						stat = IO_ERROR;
				}
				
				if (IO_NORMAL == stat)
					stat = IO_read(t_compressed->data, t_compressed->size, stream);
				
				if (IO_NORMAL == stat)
				{
					if (version == kMCStackFileFormatVersion_1_4)
					{
						if ((ncolors == 16 || ncolors == 256) && noblack())
							flags |= F_NEED_FIXING;
						if (MCMemoryReallocate(colors, sizeof(MCColor) * (ncolors + 1), colors))
							MCColorSetPixel(colors[ncolors], 0);
					}
				}
			}
			else
			{
				if (IO_NORMAL == stat)
				{
					t_compressed->color_count = ncolors;
					if (!MCMemoryNewArray(ncolors, t_compressed->planes) ||
						!MCMemoryNewArray(ncolors, t_compressed->plane_sizes))
						stat = IO_ERROR;
				}

				if (IO_NORMAL == stat)
				{
					uint2 i;
					for (i = 0 ; i < ncolors ; i++)
					{
						if (IO_NORMAL == stat)
							stat = IO_read_uint4(&t_compressed->plane_sizes[i], stream);
						
						
						if (IO_NORMAL == stat && t_compressed->plane_sizes[i] != 0)
						{
							if (!MCMemoryAllocate(t_compressed->plane_sizes[i], t_compressed->planes[i]))
								stat = IO_ERROR;
							
							if (IO_NORMAL == stat)
								stat = IO_read(t_compressed->planes[i], t_compressed->plane_sizes[i], stream);
						}
						
						if (IO_NORMAL != stat)
							break;
					}
				}
			}
			if (IO_NORMAL == stat && t_compressed->compression == F_RLE && ncolors != 0 && (flags & F_TRUE_COLOR) == 0)
			{
				t_compressed->color_count = ncolors;
				if (!MCMemoryAllocateCopy(colors, sizeof(MCColor) * ncolors, t_compressed->colors))
					stat = IO_ERROR;
			}

			if (IO_NORMAL == stat)
				stat = IO_read_uint4(&t_compressed->mask_size, stream);
			
			if (IO_NORMAL == stat && t_compressed->mask_size != 0)
			{
				if (!MCMemoryAllocate(t_compressed->mask_size, t_compressed->mask))
					stat = IO_ERROR;
					
				if (IO_NORMAL == stat)
					stat = IO_read(t_compressed->mask, t_compressed->mask_size, stream);
			}

			if (IO_NORMAL == stat)
			{
				uint16_t t_pixwidth, t_pixheight;
				t_pixwidth = rect.width;
				t_pixheight = rect.height;
			
				if (flags & F_SAVE_SIZE)
				{
					stat = IO_read_uint2(&t_pixwidth, stream);
					
					if (IO_NORMAL == stat)
						stat = IO_read_uint2(&t_pixheight, stream);
				}
				
				if (IO_NORMAL == stat)
				{
					t_compressed->width = t_pixwidth;
					t_compressed->height = t_pixheight;
				}
			}
			
			if (IO_NORMAL == stat && !setcompressedbitmap(t_compressed))
				stat = IO_ERROR;
			
			MCImageFreeCompressedBitmap(t_compressed);
			
			if (IO_NORMAL != stat)
				return stat;
		}
	if ((stat = IO_read_int2(&xhot, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_int2(&yhot, stream)) != IO_NORMAL)
		return stat;
	if (flags & F_ANGLE)
		if ((stat = IO_read_uint2(&angle, stream)) != IO_NORMAL)
			return stat;
	
	// MW-2013-09-05: [[ Bug 11127 ]] At this point the color/pixmap fields in the object
	//   will pertain to the image colors. This isn't what we want anymore, so free them
	//   (an RLE compressed rep will already have extracted the info it needs).
	delete[] colors; /* Allocated with new[] */
	for (uint32_t i = 0; i < ncolors; i++)
		MCValueRelease(colornames[i]);
	delete[] colornames; /* Allocated with new[] */
	MCMemoryDeleteArray(patterns);
	ncolors = 0;
	npatterns = 0;
	dflags = 0;
	colornames = nil;
	patterns = nil;
	colors = nil;
	
	// MW-2013-09-05: [[ Bug 11127 ]] If we had an extended control color record, then
	//   take those as the in-memory fields for colors and pixmaps as they are used
	//   by the control rendering (the rep has already taken its colors if it needed them).
	if (s_have_control_colors)
	{
		colors = s_control_colors;
		colornames = s_control_color_names;
		ncolors = s_control_color_count;
		patterns = s_control_pixmapids;
		npatterns = s_control_pixmap_count;
		dflags = s_control_color_flags;
		if (npatterns != 0 &&
			!MCMemoryNewArray(npatterns, patterns))
			return IO_ERROR;

		s_control_colors = nil;
		s_control_color_names = nil;
		s_control_color_count = 0;
		s_control_pixmapids = nil;
		s_control_pixmap_count = 0;
		s_have_control_colors = false;
	}

	return loadpropsets(stream, version);
}

// MW-2012-03-28: [[ Bug 10130 ]] This is a no-op as the image object has no
//   font.
bool MCImage::recomputefonts(MCFontRef p_parent_font, bool p_force)
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool MCImage::getclipboardtext(MCDataRef& r_data)
{
	bool t_success = true;
	
	recompress();
	if (getcompression() == F_RLE)
	{
		MCImageBitmap *t_bitmap = nil;
		t_success = lockbitmap(t_bitmap, false);

		if (t_success)
			t_success = MCImageCreateClipboardData(t_bitmap, r_data);
		unlockbitmap(t_bitmap);
	}
	else if (m_rep != nil)
	{
		MCImageRepType t_type = m_rep->GetType();
		void *t_bytes = nil;
		uindex_t t_size = 0;
		if (t_type == kMCImageRepResident)
			static_cast<MCResidentImageRep*>(m_rep)->GetData(t_bytes, t_size);
		else if (t_type == kMCImageRepVector)
			static_cast<MCVectorImageRep*>(m_rep)->GetData(t_bytes, t_size);
		else
			t_success = false;
		
		if (t_success)
			t_success = MCDataCreateWithBytes((const char_t *)t_bytes, t_size, r_data);
	}
	else
		t_success = false;
	
	return t_success;
}

///////////////////////////////////////////////////////////////////////////////

void MCImage::flip(bool p_horz)
{
	// Invert the flip states.
	if (p_horz)
		m_flip_x = !m_flip_x;
	else
		m_flip_y = !m_flip_y;
	
	// Update the transform.
	apply_transform();

	// Ensure we redraw.
	layer_redrawall();
	notifyneeds(false);
}

void MCImage::apply_transform()
{
	uindex_t t_width = rect.width;
	uindex_t t_height = rect.height;
	/* UNCHECKED */ getsourcegeometry(t_width, t_height);

	// MW-2013-10-25: [[ Bug 11300 ]] Make sure we apply a flip transform if
	//   required and there are no other transforms to apply.
	if (angle != 0)
		rotate_transform(angle);
	else if (rect.width != t_width || rect.height != t_height)
		resize_transform();
	else if (m_flip_x || m_flip_y)
		flip_transform();
	else
		m_has_transform = false;
	
	if (m_resampled_rep != nil && !(m_has_transform && MCGAffineTransformIsRectangular(m_transform) && m_resampled_rep->Matches(rect.width, rect.height, m_transform.a == -1.0, m_transform.d == -1.0, m_rep)))
	{
		m_resampled_rep->Release();
		m_resampled_rep = nil;
	}
}

///////////////////////////////////////////////////////////////////////////////

void MCImage::sourcerectchanged(MCRectangle p_new_rect)
{
	MCRectangle t_old_rect = rect;
	rect = p_new_rect;
	layer_rectchanged(t_old_rect, true);
}

void MCImage::invalidate_rep(MCRectangle &p_rect)
{
	layer_redrawrect(p_rect);

	if (state & CS_MAGNIFY)
		magredrawdest(p_rect);
}

bool MCImage::isediting() const
{
	return m_rep != nil && m_rep->GetType() == kMCImageRepMutable;
}

bool MCImage::convert_to_mutable()
{
	// referenced images cannot be edited
	if (getflag(F_HAS_FILENAME))
		return false;

	if (m_rep != nil && m_rep->GetType() == kMCImageRepMutable)
		return true;

	bool t_success = true;

	MCMutableImageRep *t_rep = nil;
	MCImageBitmap *t_bitmap = nil;
	if (m_rep != nil)
	{
		t_success = lockbitmap(t_bitmap, true);
		if (t_success)
			t_success = nil != (t_rep = new (nothrow) MCMutableImageRep(this, t_bitmap));
		unlockbitmap(t_bitmap);
	}
	else
	{
		t_success = MCImageBitmapCreate(rect.width, rect.height, t_bitmap);
		if (t_success)
		{
			MCImageBitmapClear(t_bitmap);
			t_success = nil != (t_rep = new (nothrow) MCMutableImageRep(this, t_bitmap));
		}
		MCImageFreeBitmap(t_bitmap);
	}

	if (t_success)
	{
		setrep(t_rep);
		// MW-2010-12-14: [[ Bug 9240 ]] Make sure the angle is reset to 0 (as we've
		//   ditched the 'original' data now).
		angle = 0;
	}

	return t_success;
}

void MCImage::startediting(uint16_t p_which)
{
	bool t_success = convert_to_mutable();

	if (t_success)
	{
		static_cast<MCMutableImageRep*>(m_rep)->image_mfocus(mx, my);
		static_cast<MCMutableImageRep*>(m_rep)->image_mdown(p_which);
	}

	/* UNCHECKED */ MCAssert(t_success);
}

void MCImage::finishediting()
{
	if (!isediting())
		return;

	if (MCeditingimage == this)
		MCeditingimage = nil;

	if (MCactiveimage == this)
		MCactiveimage = nil;

	bool t_success = true;

	MCImageRep *t_rep = m_rep;
	MCImageBitmap *t_bitmap = nil;

	t_success = t_rep->LockBitmap(0, 1.0, t_bitmap);
	if (t_success)
		t_success = setbitmap(t_bitmap, 1.0);
	t_rep->UnlockBitmap(0, t_bitmap);

	/* UNCHECKED */ MCAssert(t_success);
}

///////////////////////////////////////////////////////////////////////////////

void MCImage::setrep(MCImageRep *p_rep)
{
	if (p_rep == m_rep)
		return;

	MCImageRep *t_rep = nil;
	if (p_rep != nil)
		t_rep = p_rep->Retain();

	if (m_rep != nil)
		m_rep->Release();

	m_rep = t_rep;

	m_has_transform = false;
	if (m_resampled_rep != nil)
	{
		m_resampled_rep->Release();
		m_resampled_rep = nil;
	}
	
	// IM-2013-03-11: [[ BZ 10723 ]] If we have a new image, ensure that the current frame falls within the new framecount
	// IM-2013-04-15: [[ BZ 10827 ]] Skip this check if the currentframe is 0 (preventing unnecessary image loading)
	if (currentframe != 0)
		setframe(currentframe);

	notifyneeds(false);
}

bool MCImage::setfilename(MCStringRef p_filename)
{
	bool t_success = true;

	if (MCStringIsEmpty(p_filename))
	{
		setrep(nil);
		flags &= ~(F_COMPRESSION | F_TRUE_COLOR | F_NEED_FIXING);
		flags &= ~F_HAS_FILENAME;
		
		MCValueAssign(filename, p_filename);
		
		return true;
	}
    
	MCImageRep *t_rep = nil;

	if (t_success)
	{
		t_success = MCImageGetRepForReferenceWithStackContext(p_filename, getstack(), t_rep);

		// MM-2013-11-27: [[ Bug 11522 ]] If we can't get the image rep, make sure we still store the filename.
		if (t_success)
		{
			setrep(t_rep);
			if (t_rep != nil)
				t_rep->Release();
			
			flags &= ~(F_COMPRESSION | F_TRUE_COLOR | F_NEED_FIXING);
			flags |= F_HAS_FILENAME;
			
            MCValueAssign(filename, p_filename);
		}
		else
		{
			setrep(nil);
			flags &= ~(F_COMPRESSION | F_TRUE_COLOR | F_NEED_FIXING);
			flags |= F_HAS_FILENAME;
			
            MCValueAssign(filename, p_filename);
		}		
	}

	return t_success;	
}

/* Special case used by set_gif() */
bool MCImage::setdata(void *p_data, uindex_t p_size)
{
	bool t_success = true;

	MCImageRep *t_rep = nil;

	t_success = MCImageRepGetResident(p_data, p_size, t_rep);

	if (t_success)
	{
		// MW-2013-09-05: [[ UnicodifyImage ]] Clear the filename property.
		MCValueAssign(filename, kMCEmptyString);
		
		setrep(t_rep);
		t_rep->Release();
	}

	return t_success;
}

bool MCImage::setbitmap(MCImageBitmap *p_bitmap, MCGFloat p_scale, bool p_update_geometry)
{
	bool t_success = true;

	MCImageCompressedBitmap *t_compressed = nil;

	t_success = MCImageCompress(p_bitmap, (flags & F_DONT_DITHER) == 0, t_compressed);

	if (t_success)
		t_success = setcompressedbitmap(t_compressed);

	MCImageFreeCompressedBitmap(t_compressed);

	if (t_success)
	{
		angle = 0;
		// IM-2013-10-30: [[ FullscreenMode ]] REVISIT: currently, the scale will always be 1.0 but if it becomes
		// possible to set the bitmap at some other scale this section will need to be revised
		
		if (p_update_geometry)
		{
	#ifdef FEATURE_DONT_RESIZE
			if (!(flags & F_LOCK_LOCATION) && !(flags & F_PLAYER_DONT_RESIZE))
	#else
			if (!(flags & F_LOCK_LOCATION))
	#endif
			{
				uint32_t t_width, t_height;
				/* UNCHECKED */ getsourcegeometry(t_width, t_height);
				rect . width = t_width;
				rect . height = t_height;
			}
		}
	}

	return t_success;
}

bool MCImage::setcompressedbitmap(MCImageCompressedBitmap *p_compressed)
{
	bool t_success = true;

	MCImageRep *t_rep = nil;

	switch (p_compressed->compression)
	{
	case F_GIF:
	case F_PNG:
	case F_JPEG:
		t_success = MCImageRepGetResident(p_compressed->data, p_compressed->size, t_rep);
		break;
	case F_PICT:
		t_success = MCImageRepGetVector(p_compressed->data, p_compressed->size, t_rep);
		break;
	case F_RLE:
		t_success = MCImageRepGetCompressed(p_compressed, t_rep);
		break;
	default:
		t_success = false;
	}

	if (t_success)
	{
		setrep(t_rep);
		t_rep->Release();
		flags &= ~(F_HAS_FILENAME | F_COMPRESSION | F_TRUE_COLOR | F_NEED_FIXING);
		flags |= p_compressed->compression;

		if (p_compressed->compression == F_RLE)
		{
			if (p_compressed->color_count == 0)
				flags |= F_TRUE_COLOR;
		}
		
		// MW-2013-09-05: [[ UnicodifyImage ]] Clear the filename property.
		MCValueAssign(filename, kMCEmptyString);
	}

	return t_success;
}

///////////////////////////////////////////////////////////////////////////////

// IM-2013-11-06: [[ RefactorGraphics ]] Return a copy of the bitmap with the given transform applied
// IM-2014-05-12: [[ ImageRepUpdate ]] Modify function to take MCGImage parameter
// IM-2014-09-02: [[ Bug 13295 ]] Add optional target size param.
bool MCImageBitmapCreateWithTransformedMCGImage(MCGImageRef p_src, MCGAffineTransform p_transform, MCGImageFilter p_quality, const MCGIntegerSize *p_target_size, MCImageBitmap *&r_bitmap)
{
	if (p_src == nil)
		return false;
	
	bool t_success;
	t_success = true;
	
	MCGRectangle t_image_rect;
	t_image_rect = MCGRectangleMake(0, 0, MCGImageGetWidth(p_src), MCGImageGetHeight(p_src));
	
	// IM-2013-11-06: [[ Bug 11390 ]] Calculate target bitmap dimensions by transforming the source image rect.
	MCGRectangle t_trans_rect;
	t_trans_rect = MCGRectangleApplyAffineTransform(t_image_rect, p_transform);
	
	uint32_t t_trans_width = ceilf(t_trans_rect.size.width);
	uint32_t t_trans_height = ceilf(t_trans_rect.size.height);
	
	if (p_target_size != nil)
	{
		// IM-2014-09-02: [[ Bug 13295 ]] Adjust transform to fit transformed image within target size
		p_transform = MCGAffineTransformConcat(MCGAffineTransformMakeScale(p_target_size->width / t_trans_rect.size.width, p_target_size->height / t_trans_rect.size.height), p_transform);
		t_trans_width = p_target_size->width;
		t_trans_height = p_target_size->height;
	}

	MCImageBitmap *t_bitmap;
	t_bitmap = nil;
	
	t_success = MCImageBitmapCreate(t_trans_width, t_trans_height, t_bitmap);
	
	MCGContextRef t_context;
	t_context = nil;
	
	if (t_success)
	{
		MCImageBitmapClear(t_bitmap);
		t_success = MCGContextCreateWithPixels(t_bitmap->width, t_bitmap->height, t_bitmap->stride, t_bitmap->data, true, t_context);
	}
	
	if (t_success)
	{
		MCGContextConcatCTM(t_context, p_transform);
		MCGContextDrawImage(t_context, p_src, t_image_rect, p_quality);
		
		MCImageBitmapCheckTransparency(t_bitmap);
	}
	
	MCGContextRelease(t_context);
	
	if (t_success)
		r_bitmap = t_bitmap;
	else
		MCImageFreeBitmap(t_bitmap);
	
	return t_success;
}

// IM-2014-05-12: [[ ImageRepUpdate ]] Refactor code common to lockbitmap/copybitmap to this method
// IM-2013-07-26: [[ ResIndependence ]] render the image at the requested size,
// with any transformations (scale, angle) applied
// IM-2024-09-02: [[ Bug 13295 ]] Replace scale param with optional target size.
bool MCImage::lockbitmap(bool p_premultiplied, bool p_update_transform, const MCGIntegerSize *p_size, MCImageBitmap *&r_bitmap)
{
	bool t_success;
	t_success = true;
	
	if (p_update_transform)
		apply_transform();

	// IM-2013-11-06: [[ RefactorGraphics ]] Use common method to get image rep & transform
	// so imagedata & rendered image have the same appearance
	MCImageRep *t_rep;
	bool t_has_transform;
	MCGAffineTransform t_transform;
	
	MCGImageFrame t_frame;
	
	uint32_t t_width, t_height;
	MCGFloat t_transform_scale;
	
	t_success = get_rep_and_transform(t_rep, t_has_transform, t_transform);
	
	if (t_success)
		t_success = t_rep != nil;
	
	// IM-2014-08-01: [[ Bug 13021 ]] We can't reliably determine the image density before
	// locking, so instead we lock the image, then calculate the resulting horizontal &
	// vertical densities.
	if (t_success)
		t_success = t_rep->GetGeometry(t_width, t_height);
	
	MCGIntegerSize t_size;
	if (p_size == nil)
		t_size = MCGIntegerSizeMake(rect.width, rect.height);
	else
		t_size = *p_size;

    /* Special case vector image reps - as they don't have pixels! */
    if (t_success &&
        t_rep->GetType() == kMCImageRepVector)
    {
        /* Start with the identity transform, if none */
		if (!t_has_transform)
			t_transform = MCGAffineTransformMakeIdentity();
        
        /* Scale from the rect size to the requested size. */
		MCGFloat t_x_scale, t_y_scale;
		t_x_scale = (MCGFloat)t_size.width / (MCGFloat)rect.width;
		t_y_scale = (MCGFloat)t_size.height / (MCGFloat)rect.height;
		t_transform = MCGAffineTransformConcat(MCGAffineTransformMakeScale(t_x_scale, t_y_scale), t_transform);
        
        MCImageBitmap *t_bitmap = nullptr;
        t_success = MCImageBitmapCreate(t_size.width, t_size.height, t_bitmap);
        
        MCGContextRef t_context = nullptr;
        if (t_success)
        {
            MCImageBitmapClear(t_bitmap);
            t_success = MCGContextCreateWithPixels(t_bitmap->width, t_bitmap->height, t_bitmap->stride, t_bitmap->data, true, t_context);
        }
        
        if (t_success)
        {
            MCGContextConcatCTM(t_context, t_transform);
            
            auto t_vector_rep = static_cast<MCVectorImageRep *>(t_rep);
            
            void* t_data;
            uindex_t t_data_size;
            t_vector_rep->GetData(t_data, t_data_size);
            
            MCGPaintRef t_current_color;
            getcurrentcolor(t_current_color);
            
            MCGContextPlaybackRectOfDrawing(t_context, MCMakeSpan((const byte_t*)t_data, t_data_size), MCGRectangleMake(0, 0, t_width, t_height), MCGRectangleMake(0, 0, t_size.width, t_size.height), t_current_color);
            
            MCGPaintRelease(t_current_color);
        }
        
        MCGContextRelease(t_context);
        
        if (t_success)
        {
            MCImageBitmapCheckTransparency(t_bitmap);
            if (!p_premultiplied)
            {
                MCImageBitmapUnpremultiply(t_bitmap);
            }
            
            r_bitmap = t_bitmap;
        }
        else
        {
            MCImageFreeBitmap(t_bitmap);
        }
        
        return t_success;
    }

	if (t_success)
	{
		if (!t_has_transform)
			t_transform = MCGAffineTransformMakeIdentity();
		
		// IM-2014-09-02: [[ Bug 13295 ]] Adjust transform to fit transformed image within target size
		MCGFloat t_x_scale, t_y_scale;
		t_x_scale = (MCGFloat)t_size.width / (MCGFloat)rect.width;
		t_y_scale = (MCGFloat)t_size.height / (MCGFloat)rect.height;
		t_transform = MCGAffineTransformConcat(MCGAffineTransformMakeScale(t_x_scale, t_y_scale), t_transform);
		
		t_transform_scale = MCGAffineTransformGetEffectiveScale(t_transform);
		t_success = t_rep->LockImageFrame(currentframe, t_transform_scale, t_frame);
	}

	if (t_success)
	{
		// IM-2014-08-01: [[ Bug 13021 ]] Scale image frame size -> logical size
		t_transform = MCGAffineTransformConcat(t_transform, MCGAffineTransformMakeScale(t_width / (MCGFloat)MCGImageGetWidth(t_frame.image), t_height / (MCGFloat)MCGImageGetHeight(t_frame.image)));
		
		bool t_copy_pixels;
		t_copy_pixels = MCGAffineTransformIsIdentity(t_transform);

		if (t_copy_pixels && !p_premultiplied)
		{
			// IM-2014-08-01: [[ Bug 13021 ]] The locked frame has premultiplied alpha,
			// so we have to release and lock the unpremultiplied bitmap frame.
			t_rep->UnlockImageFrame(currentframe, t_frame);
			
			MCImageBitmap *t_bitmap;
			t_bitmap = nil;
			
			t_success = t_rep->LockBitmap(currentframe, t_transform_scale, t_bitmap);
			
			if (t_success)
			{
				m_locked_bitmap = t_bitmap;
				m_locked_rep = t_rep;
			}
		}
		else if (t_copy_pixels && p_premultiplied)
		{
			MCGRaster t_raster;
			t_success = MCGImageGetRaster(t_frame.image, t_raster);

			if (t_success)
				t_success = MCMemoryNew(m_locked_bitmap);

			if (t_success)
			{
				*m_locked_bitmap = MCImageBitmapFromMCGRaster(t_raster);
				m_locked_image = MCGImageRetain(t_frame.image);
			}
		
			t_rep->UnlockImageFrame(currentframe, t_frame);
		}
		else
		{
			// IM-2013-11-06: [[ RefactorGraphics ]] Factor out transformed image creation code
			// MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types.
			MCGImageFilter t_filter;
			t_filter = getimagefilter();
			
			t_success = MCImageBitmapCreateWithTransformedMCGImage(t_frame.image, t_transform, t_filter, &t_size, m_locked_bitmap);
			
			if (t_success && !p_premultiplied)
				MCImageBitmapUnpremultiply(m_locked_bitmap);
			
			t_rep->UnlockImageFrame(currentframe, t_frame);
		}
	}
	
	if (t_success)
		r_bitmap = m_locked_bitmap;

	return t_success;
}

// IM-2014-05-12: [[ ImageRepUpdate ]] Reimplement by locking the bitmap and taking
// ownership of the produced bitmap (if the result of transforming the source) or copying it otherwise.
// IM-2013-07-26: [[ ResIndependence ]] render the image with any transformations (scale, angle) applied
// IM-2014-09-02: [[ Bug 13295 ]] Remove unused scale param.
bool MCImage::copybitmap(bool p_premultiplied, MCImageBitmap *&r_bitmap)
{
	bool t_success;
	t_success = true;
	
	MCImageBitmap *t_bitmap;
	t_bitmap = nil;

    // PM-2014-11-05: [[ Bug 13938 ]] Make sure new alphaData does not add to previous one
	t_success = lockbitmap(t_bitmap, p_premultiplied);

	if (t_success)
	{
		if (m_locked_rep == nil && m_locked_image == nil)
		{
			r_bitmap = t_bitmap;
			m_locked_bitmap = nil;
		}
		else
		{
			t_success = MCImageCopyBitmap(t_bitmap, r_bitmap);
			unlockbitmap(t_bitmap);
		}
	}

	return t_success;
}

bool MCImage::lockbitmap(MCImageBitmap *&r_bitmap, bool p_premultiplied, bool p_update_transform)
{
	return lockbitmap(p_premultiplied, p_update_transform, nil, r_bitmap);
}

void MCImage::unlockbitmap(MCImageBitmap *p_bitmap)
{
	if (m_locked_rep != nil)
	{
		m_locked_rep->UnlockBitmap(currentframe, m_locked_bitmap);
		m_locked_rep = nil;
		m_locked_bitmap = nil;
	}
	else if (m_locked_image != nil)
	{
		MCGImageRelease(m_locked_image);
		m_locked_image = nil;
		MCMemoryDelete(m_locked_bitmap);
		m_locked_bitmap = nil;
	}
	else if (m_locked_bitmap != nil)
	{
		MCImageFreeBitmap(m_locked_bitmap);
		m_locked_bitmap = nil;
	}
}

///////////////////////////////////////////////////////////////////////////////

uint32_t MCImage::getcompression()
{
	uint32_t t_compression = F_RLE;

	// IM-2013-10-30: [[ FullscreenMode ]] getDataCompression() now part of base MCImageRep class
	if (m_rep != nil)
		t_compression = m_rep->GetDataCompression();

	return t_compression;
}

void MCImage::getrawdata(MCDataRef& r_data)
{
 	if (m_rep == nil || m_rep->GetType() != kMCImageRepResident)
    {
        r_data = MCValueRetain(kMCEmptyData);
    }
	
	void *t_data;
	uindex_t t_size;
	static_cast<MCResidentImageRep*>(m_rep)->GetData(t_data, t_size);
	
	/* UNCHECKED */ MCDataCreateWithBytes((const byte_t*)t_data, t_size, r_data);
}
// PM-2014-12-12: [[ Bug 13860 ]] Allow exporting referenced images to album
void MCImage::getimagefilename(MCStringRef &r_filename)
{
    MCStringRef t_filename;
    if (m_rep == nil || m_rep->GetType() != kMCImageRepReferenced)
        r_filename = MCValueRetain(kMCEmptyString);
    
    t_filename = static_cast<MCReferencedImageRep*>(m_rep)->GetSearchKey();
    
    MCStringCopy(t_filename, r_filename);
    
}

bool MCImage::isReferencedImage()
{
    return m_rep->GetType() == kMCImageRepReferenced;
}


bool MCImage::getsourcegeometry(uint32_t &r_pixwidth, uint32_t &r_pixheight)
{
	// IM-2014-08-01: [[ Bug 13021 ]] GetGeometry() now returns the logical image size, so just pass that through without scaling
	return m_rep != nil && m_rep->GetGeometry(r_pixwidth, r_pixheight);
}

void MCImage::getgeometry(uint32_t &r_pixwidth, uint32_t &r_pixheight)
{
	// while cropping
	if ((state & CS_EDITED) && (state & CS_SIZE))
	{
		r_pixwidth = m_current_width;
		r_pixheight = m_current_height;
		return;
	}

	if (getsourcegeometry(r_pixwidth, r_pixheight))
		return;

	r_pixwidth = rect.width;
	r_pixheight = rect.height;
}

MCGFloat MCImage::getdevicescale(void)
{
	if (!getstack())
		return 1.0;
	else
		return getstack()->getdevicescale();
}

///////////////////////////////////////////////////////////////////////////////
