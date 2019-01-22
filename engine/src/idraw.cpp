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

#include "exec.h"
#include "stacklst.h"
#include "undolst.h"
#include "image.h"
#include "stack.h"
#include "card.h"
#include "magnify.h"
#include "util.h"

#include "globals.h"

#include "context.h"
#include "graphicscontext.h"
#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

void MCImage::getcurrentcolor(MCGPaintRef& r_paint)
{
    MCColor t_color;
    MCPatternRef t_pattern = nil;
    int2 x, y;
    if (getforecolor(DI_BACK, false, false, t_color, t_pattern, x, y, CONTEXT_TYPE_SCREEN, this, false))
    {
        MCGPaintCreateWithSolidColor(t_color.red / 65535.0f, t_color.green / 65535.0f, t_color.blue / 65535.0f, 1.0f, r_paint);
    }
    else if (t_pattern != nullptr)
    {
        r_paint = nullptr;
    }
    else
    {
        r_paint = nullptr;
    }
}

bool MCImage::get_rep_and_transform(MCImageRep *&r_rep, bool &r_has_transform, MCGAffineTransform &r_transform)
{
	// IM-2013-11-05: [[ RefactorGraphics ]] Use resampled image rep for best-quality scaling
	// IM-2014-08-07: [[ Bug 13127 ]] Don't use resampled rep for images with a centerRect
	if (m_has_transform && MCGAffineTransformIsRectangular(m_transform) && resizequality == INTERPOLATION_BICUBIC && m_center_rect.x == INT16_MIN)
	{
		bool t_h_flip, t_v_flip;
		t_h_flip = m_transform.a == -1.0;
		t_v_flip = m_transform.d == -1.0;

		if (m_resampled_rep != nil && m_resampled_rep->Matches(rect.width, rect.height, t_h_flip, t_v_flip, m_rep))
			r_rep = m_resampled_rep;
		else
		{
			// MM-2014-08-05: [[ Bug 13112 ]] Make sure only a single thread resamples the image.
			if (m_resampled_rep != nil && m_resampled_rep->Matches(rect.width, rect.height, t_h_flip, t_v_flip, m_rep))
				r_rep = m_resampled_rep;
			else
			{
				if (!MCImageRepGetResampled(rect.width, rect.height, t_h_flip, t_v_flip, m_rep, r_rep))
					return false;

				if (m_resampled_rep != nil)
					m_resampled_rep->Release();
				m_resampled_rep = static_cast<MCResampledImageRep*>(r_rep);
			}
		}
		
		r_has_transform = false;
	}
	else
	{
		r_rep = m_rep;
		r_has_transform = m_has_transform;
		r_transform = m_transform;
	}
	
	return true;
}

void MCImage::drawme(MCDC *dc, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy, uint2 dw, uint2 dh)
{
	if (m_rep != nil)
	{
        /* Printer output generally requires special-casing */
        bool t_printer = dc->gettype() == CONTEXT_TYPE_PRINTER;
        
        /* Update the transform - as necessary */
        bool t_update = !((state & CS_SIZE) && (state & CS_EDITED));

        if (t_update)
            apply_transform();
    
		if (m_rep->GetType() == kMCImageRepVector)
		{
            MCGAffineTransform t_transform;
            if (m_has_transform)
            {
                t_transform = m_transform;
            }
            else
            {
                t_transform = MCGAffineTransformMakeIdentity();
            }
            
            uindex_t t_rep_width, t_rep_height;
            m_rep->GetGeometry(t_rep_width, t_rep_height);
            
            // MW-2014-06-19: [[ IconGravity ]] Scale the image appropriately.
            if (dw != sw || dh != sh)
            {
                t_transform = MCGAffineTransformPreScale(t_transform, dw / (float)sw, dh / (float)sh);
            }

            // MW-2014-06-19: [[ IconGravity ]] Only clip if we are drawing a partial image (we need to double-check, but I don't think sx/sy are ever non-zero).
            MCRectangle t_old_clip;
            t_old_clip = dc->getclip();
            if (sx != 0 && sy != 0)
            {
                dc->setclip(MCRectangleMake(dx, dy, sw, sh));
            }
            
            t_transform = MCGAffineTransformConcat(t_transform,
                                                   MCGAffineTransformMakeTranslation(-(dx - sx), -(dy - sy)));
            t_transform = MCGAffineTransformPreTranslate(t_transform, (dx - sx), (dy - sy));
            
            MCGContextRef t_gcontext;
            if (dc->lockgcontext(t_gcontext))
            {
                MCGContextSave(t_gcontext);
                
                MCGContextConcatCTM(t_gcontext, t_transform);
                
                auto t_vector_rep = static_cast<MCVectorImageRep *>(m_rep);
                
                void* t_data;
                uindex_t t_data_size;
                t_vector_rep->GetData(t_data, t_data_size);
                
                MCGPaintRef t_current_color;
                getcurrentcolor(t_current_color);
                
                MCGContextPlaybackRectOfDrawing(t_gcontext, MCMakeSpan((const byte_t*)t_data, t_data_size), MCGRectangleMake(0, 0, t_rep_width, t_rep_height), MCGRectangleMake(dx - sx, dy - sy, t_rep_width, t_rep_height), t_current_color);
                
                MCGPaintRelease(t_current_color);
                
                MCGContextRestore(t_gcontext);
                
                dc->unlockgcontext(t_gcontext);
            }
            
            dc->setclip(t_old_clip);
		}
		else
		{
			// IM-2013-03-19: The original image bitmap & compressed format is only
			// needed when outputting to a printer context, so we only need to use
			// the current transformed version otherwise. This should prevent large
			// source images from flushing everything else out of the cache.
			bool t_success = true;

			MCGImageFrame t_frame;
			
			// IM-2013-11-06: [[ RefactorGraphics ]] Use common method to get image rep & transform
			// so imagedata & rendered image have the same appearance
			MCImageRep *t_rep;
			bool t_has_transform;
			MCGAffineTransform t_transform;
			
			// MW-2014-03-11: [[ Bug 11608 ]] Make sure we always use the source rep if printing
			//   (rather than resampled).
			if (t_printer)
			{
				t_rep = m_rep;
				t_has_transform = m_has_transform;
				t_transform = m_transform;
			}
			else
				/* UNCHECKED */ get_rep_and_transform(t_rep, t_has_transform, t_transform);
			
            // MW-2014-06-19: [[ IconGravity ]] Scale the image appropriately.
            if (dw != sw || dh != sh)
            {
                if (!t_has_transform)
                {
                    t_has_transform = true;
                    t_transform = MCGAffineTransformMakeIdentity();
                }
                t_transform = MCGAffineTransformPreScale(t_transform, dw / (float)sw, dh / (float)sh);
            }
            
			MCGFloat t_device_scale;
			t_device_scale = 1.0;
			
			MCGAffineTransform t_device_transform;
			t_device_transform = dc->getdevicetransform();
			
			// If the image has a transform, combine it with the context device transform
			if (t_has_transform)
				t_device_transform = MCGAffineTransformConcat(t_device_transform, t_transform);
			
			// get the effective scale from the combined transform
			t_device_scale = MCGAffineTransformGetEffectiveScale(t_device_transform);
			
			// IM-2014-01-31: [[ HiDPI ]] Get the appropriate image for the combined
			//   context device & image transforms
			t_success = t_rep->LockImageFrame(currentframe, t_device_scale, t_frame);
			if (t_success)
			{
				MCImageDescriptor t_image;
				MCMemoryClear(&t_image, sizeof(MCImageDescriptor));

				t_image.has_transform = t_has_transform;
				if (t_has_transform)
					t_image.transform = t_transform;
				
				// IM-2013-07-19: [[ ResIndependence ]] set scale factor so hi-res image draws at the right size
				// IM-2014-08-07: [[ Bug 13021 ]] Split density into x / y scale components
				t_image.x_scale = t_frame.x_scale;
				t_image.y_scale = t_frame.y_scale;

                // MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types.
				t_image.filter = getimagefilter();

				t_image . image = t_frame.image;

				if (t_printer && m_rep->GetType() == kMCImageRepResident)
				{
					MCResidentImageRep *t_resident = static_cast<MCResidentImageRep*>(m_rep);
					t_resident->GetData(t_image . data_bits, t_image . data_size);
					switch(t_resident->GetDataCompression())
					{
					case F_GIF: t_image . data_type = kMCImageDataGIF; break;
					case F_PNG: t_image . data_type = kMCImageDataPNG; break;
					case F_JPEG: t_image . data_type = kMCImageDataJPEG; break;
					default:
						t_image . data_bits = nil;
						t_image . data_size = 0;
						t_image . data_type = kMCImageDataNone;
					}
				}
				else
					t_image . data_type = kMCImageDataNone;

                if (m_center_rect . x != INT16_MIN)
                {
                    t_image . has_center = true;
                    // IM-2014-07-10: [[ Bug 12794 ]] Provide unscaled center rect to context
					t_image . center = MCRectangleToMCGRectangle(m_center_rect);
                }
                else
                    t_image . has_center = false;
                
				dc -> drawimage(t_image, sx, sy, sw, sh, dx, dy);
			}
			else
			{
				// can't get image data from rep
                drawnodata(dc, sw, sh, dx, dy, dw, dh);
			}

            if (t_success)
				t_rep->UnlockImageFrame(currentframe, t_frame);
		}

		if (state & CS_DO_START)
		{
			// MM-2014-07-31: [[ ThreadedRendering ]] Make sure only a single thread posts the timer message (i.e. the first that gets here)
			if (!m_animate_posted)
			{
				if (!m_animate_posted)
				{
					// IM-2014-11-25: [[ ImageRep ]] Use ImageRep method to get frame duration
					uint32_t t_frame_duration;
					t_frame_duration = 0;
					
					/* UNCHECKED */ m_rep->GetFrameDuration(currentframe, t_frame_duration);
					
					m_animate_posted = true;
					MCscreen->addtimer(this, MCM_internal, t_frame_duration);
				}
			}

			state &= ~CS_DO_START;
		}
	}
    // AL-2014-08-04: [[ Bug 13097 ]] Image filename is never nil; check for emptiness instead
    else if (!MCStringIsEmpty(filename))
    {
        // AL-2014-01-15: [[ Bug 11570 ]] Draw stippled background when referenced image file not found
        drawnodata(dc, sw, sh, dx, dy, dw, dh);
    }
}

void MCImage::drawnodata(MCDC *dc, uint2 sw, uint2 sh, int2 dx, int2 dy, uint2 dw, uint2 dh)
{
    MCRectangle drect;
    MCU_set_rect(drect, dx, dy, dw, dh);
    setforeground(dc, DI_BACK, False);
    dc->setbackground(MCscreen->getwhite());
    dc->setfillstyle(FillOpaqueStippled, nil, 0, 0);
    dc->fillrect(drect);
    dc->setbackground(MCzerocolor);
    dc->setfillstyle(FillSolid, nil, 0, 0);
}

void MCImage::drawcentered(MCDC *dc, int2 x, int2 y, Boolean reversed)
{
	uint4 oldflags = flags;
	uint4 oldstate = state;
	flags &= ~F_SHOW_BORDER;
	state &= ~(CS_MAGNIFY | CS_OWN_SELECTION | CS_SELECTED);
	if (reversed)
		state |= CS_REVERSED;
	uint1 t_old_function;
	uint1 t_old_opacity;

	t_old_function = dc -> getfunction();
	dc -> setfunction(ink);
	t_old_opacity = dc -> getopacity();
	dc -> setopacity(blendlevel * 255 / 100);
	drawme(dc, 0, 0, rect.width, rect.height, x - (rect.width >> 1), y - (rect.height >> 1), rect.width, rect.height);
	dc -> setopacity(t_old_opacity);
	dc -> setfunction(t_old_function);
	flags = oldflags;
	state = oldstate;
}

void MCImage::drawwithgravity(MCDC *dc, MCRectangle r, MCGravity p_gravity)
{
    assert(p_gravity != kMCGravityNone);
    
	uint4 oldflags = flags;
	uint4 oldstate = state;
	flags &= ~F_SHOW_BORDER;
	state &= ~(CS_MAGNIFY | CS_OWN_SELECTION | CS_SELECTED);
	uint1 t_old_function;
	uint1 t_old_opacity;
	t_old_function = dc -> getfunction();
	dc -> setfunction(ink);
	t_old_opacity = dc -> getopacity();
	dc -> setopacity(blendlevel * 255 / 100);

	int2 dx = 0;
	int2 dy = 0;
	uint2 dw = 0;
	uint2 dh = 0;
    
    switch(p_gravity)
    {
        case kMCGravityLeft:
        case kMCGravityBottomLeft:
        case kMCGravityTopLeft:
            dx = r . x;
            dw = rect . width;
            break;
            
        case kMCGravityRight:
        case kMCGravityBottomRight:
        case kMCGravityTopRight:
            dx = r . x + r . width - rect . width;
            dw = rect . width;
            break;
            
        case kMCGravityTop:
        case kMCGravityCenter:
        case kMCGravityBottom:
            dx = r . x + r . width / 2 - rect . width / 2;
            dw = rect . width;
            break;
            
        case kMCGravityResize:
        case kMCGravityResizeAspect:
        case kMCGravityResizeAspectFill:
            dx = r . x;
            dw = r . width;
            break;
		case kMCGravityNone:
			MCUnreachable();
			break;
    }
    
    switch(p_gravity)
    {
        case kMCGravityTop:
        case kMCGravityTopLeft:
        case kMCGravityTopRight:
            dy = r . y;
            dh = rect . height;
            break;
            
        case kMCGravityBottom:
        case kMCGravityBottomRight:
        case kMCGravityBottomLeft:
            dy = r . y + r . height - rect . height;
            dh = rect . height;
            break;
            
        case kMCGravityRight:
        case kMCGravityLeft:
        case kMCGravityCenter:
            dy = r . y + r . height / 2 - rect . height / 2;
            dh = rect . height;
            break;
            
        case kMCGravityResize:
        case kMCGravityResizeAspect:
        case kMCGravityResizeAspectFill:
            dy = r . y;
            dh = r . height;
            break;
		case kMCGravityNone:
			MCUnreachable();
			break;
    }
    
    drawme(dc, 0, 0, rect . width, rect . height, dx, dy, dw, dh);
    
	dc -> setopacity(t_old_opacity);
	dc -> setfunction(t_old_function);
	flags = oldflags;
	state = oldstate;
}

void MCImage::endsel()
{
	if (isediting())
		static_cast<MCMutableImageRep*>(m_rep)->endsel();
}


void MCImage::canceldraw(void)
{
	if (isediting())
		static_cast<MCMutableImageRep*>(m_rep)->canceldraw();
}

void MCImage::startmag(int2 x, int2 y)
{
	MCStack *sptr = getstack()->findstackname(MCNAME("Magnify"));
	if (sptr == NULL)
		return;
	if (MCmagimage)
		MCmagimage->endmag(False);
	MCmagimage = this;
	state |= CS_MAGNIFY;

    MCExecContext ctxt(this, nil, nil);
    
    sptr->SetMaxWidth(ctxt, rect.width * MCmagnification);

    sptr->SetMaxHeight(ctxt, rect.height * MCmagnification);

	uint2 ssize = MCU_min(32, rect.width);
	
    sptr->SetWidth(ctxt, ssize * MCmagnification);

	ssize = MCU_min(32, rect.height);

    sptr->SetHeight(ctxt, ssize * MCmagnification);


	MCRectangle drect = sptr->getrect();
	magrect.width = drect.width / MCmagnification;
	magrect.height = drect.height / MCmagnification;
	magrect.x = x - (magrect.width >> 1);
	magrect.y = y - (magrect.height >> 1);
	MCRectangle trect = MCU_intersect_rect(rect, getcard()->getrect());
	magrect = MCU_bound_rect(magrect, trect.x - rect.x, trect.y - rect.y,
	                         trect.width, trect.height);
	sptr->openrect(getstack()->getrect(), WM_PALETTE, NULL, WP_DEFAULT, OP_NONE);
	MCscreen->addtimer(this, MCM_internal2, MCmovespeed);
}

void MCImage::endmag(Boolean close)
{
	state &= ~CS_MAGNIFY;
	if (opened)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	MCmagimage = nil;
	if (close)
	{
		MCStack *sptr = getstack()->findstackname(MCNAME("Magnify"));
		if (sptr != NULL)
			sptr->close();
	}
}

void MCImage::drawmagrect(MCDC *dc)
{
	MCRectangle trect = magrect;
	trect.x += rect.x;
	trect.y += rect.y;
	trect = MCU_reduce_rect(trect, -1);
	uint2 i = MAG_WIDTH;
	while (i--)
	{
		if ((i + dashoffset) & 0x02)
			dc->setforeground(dc->getblack());
		else
			dc->setforeground(dc->getwhite());
		dc->setfillstyle(FillSolid, nil, 0, 0);
		dc->drawrect(trect);
		trect = MCU_reduce_rect(trect, -1);
	}
}

void MCImage::magredrawdest(const MCRectangle &brect)
{
	MCRectangle mrect = brect;
	mrect.x -= rect.x + magrect.x;
	mrect.y -= rect.y + magrect.y;
	mrect = MCU_clip_rect(mrect, 0, 0, magrect.width, magrect.height);
	mrect = MCU_scale_rect(mrect, MCmagnification);
	
	// MW-2011-01-05: [[ Bug 9895 ]] Trigger a redraw of the magnifier object.
	MCmagnifier -> layer_redrawrect(mrect);
}

void MCImage::magredrawrect(MCContext *dest_context, const MCRectangle &drect)
{
	if (!MCmagnifier)
		endmag(False);

	MCRectangle t_mr;
	t_mr = MCU_offset_rect(magrect, rect.x, rect.y);

	MCImageBitmap *t_magimage = nil;
	/* UNCHECKED */ MCImageBitmapCreate(magrect.width, magrect.height, t_magimage);

	MCGContextRef t_context = nil;
	/* UNCHECKED */ MCGContextCreateWithPixels(t_magimage->width, t_magimage->height, t_magimage->stride, t_magimage->data, true, t_context);
	// IM-2014-04-22: [[ Bug 12239 ]] Previous offset calculation was wrong.
	// We can use the calculated redraw rect to get the correct offset.
	MCGContextTranslateCTM(t_context, -t_mr.x, -t_mr.y);
	MCGContextClipToRect(t_context, MCRectangleToMCGRectangle(t_mr));

	MCContext *t_gfxcontext = nil;
	/* UNCHECKED */ t_gfxcontext = new (nothrow) MCGraphicsContext(t_context);

	getstack() -> getcurcard() -> draw(t_gfxcontext, t_mr, false); 
	
	delete t_gfxcontext;
	MCGContextRelease(t_context);

	uint2 linewidth = magrect.width * MCmagnification;
	MCImageBitmap *t_line = nil;
	/* UNCHECKED */ MCImageBitmapCreate(linewidth, MCmagnification, t_line);
	
	uint2 dy = drect.y / MCmagnification * MCmagnification;
	uint2 sbytes = t_magimage->stride;
	uint4 yoffset = dy / MCmagnification * sbytes;
	while (dy < drect.y + drect.height)
	{
		// IM-2014-04-22: [[ Bug 12239 ]] Add brackets to ensure pointer arithmetic is
		// performed on the uint8_t pointer rather than the uint32_t pointer
		uint32_t *t_src_row = (uint32_t*)((uint8_t*)t_magimage->data + yoffset);
		uint32_t *t_dst_row = (uint32_t*)t_line->data;
		for (uindex_t x = 0 ; x < magrect.width ; x++)
		{
			uint4 color = *t_src_row++;
			for (uint32_t i = 0; i < MCmagnification; i++)
				*t_dst_row++ = color;
		}
		for (uint32_t i = 1; i < MCmagnification; i++)
			memcpy((uint8_t*)t_line->data + i * t_line->stride, t_line->data, t_line->stride);
		
		// OVERHAUL - REVISIT: may be able to use scaling transform with nearest filter
		// instead of manually scaling image

		MCGImageRef t_line_img;
		t_line_img = nil;
		
		/* UNCHECKED */ MCGImageCreateWithRasterNoCopy(MCImageBitmapGetMCGRaster(t_line, true), t_line_img);
		
		// Render the scanline into the destination context.
		MCImageDescriptor t_image;
		memset(&t_image, 0, sizeof(MCImageDescriptor));
		t_image . image = t_line_img;

		dest_context -> drawimage(t_image, 0, 0, linewidth, MCmagnification, 0, dy);
		
		dy += MCmagnification;
		yoffset += t_magimage->stride;
		
		MCGImageRelease(t_line_img);
	}

	MCImageFreeBitmap(t_line);
	MCImageFreeBitmap(t_magimage);
}

Boolean MCImage::magmfocus(int2 x, int2 y)
{
	mx = x / MCmagnification + magrect.x + rect.x;
	my = y / MCmagnification + magrect.y + rect.y;

	// IM-2014-09-15: [[ Bug 13429 ]] Make sure we're editing before calling mfocus on the
	// mutable image
	if (isediting())
		static_cast<MCMutableImageRep *>(m_rep)->image_mfocus(mx, my);
		
	return True;
}

static Boolean isEditingTool(Tool p_tool)
{
    switch (p_tool)
    {
        case T_SELECT:
        case T_BUCKET:
        case T_SPRAY:
        case T_ERASER:
        case T_POLYGON:
        case T_CURVE:
        case T_PENCIL:
        case T_BRUSH:
            return True;
        default:
            return False;
    }
}

Boolean MCImage::magmdown(uint2 which)
{
	if (state & CS_MFOCUSED)
		return False;
    
	if (getstack()->gettool(this) == T_DROPPER)
	{
		MCscreen->dropper(MCmagnifier->getw(),
		                  (mx - magrect.x - rect.x) * MCmagnification,
		                  (my - magrect.y - rect.y) * MCmagnification, NULL);
		getcard()->message(MCM_color_changed);
		return True;
	}
	if (MCmodifierstate & MS_CONTROL)
	{
		endmag(True);
		return False;
	}

    // PM-2014-04-01: [[Bug 11072]] Convert image to mutable if an editing tool is selected, to prevent LC crashing
    if (isEditingTool(getstack()->gettool(this)))
    {
        convert_to_mutable();
    
        if (static_cast<MCMutableImageRep *>(m_rep)->image_mdown(which) == True)
            return True;
    }

	state |= CS_MFOCUSED;

	if (which != Button2)
	{
		if (getstack()->gettool(this) == T_POLYGON)
			MCscreen->grabpointer(MCmagnifier->getw());
		return True;
	}
	return False;
}

Boolean MCImage::magmup(uint2 which)
{
	state &= ~CS_MFOCUSED;

	if (isediting())
	{
		if (static_cast<MCMutableImageRep*>(m_rep)->image_mup(which))
			return True;
	}
	return False;
}

Boolean MCImage::magdoubledown(uint2 which)
{
	if (isediting() && static_cast<MCMutableImageRep*>(m_rep)->image_doubledown(which) == True)
		return True;

	return False;
}

Boolean MCImage::magdoubleup(uint2 which)
{
	if (isediting() && static_cast<MCMutableImageRep*>(m_rep)->image_doubleup(which) == True)
		return True;
	return False;
}


////////////////////////////////////////////////////////////////////////////////
