/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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
#include "execpt.h"
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

void MCImage::drawme(MCDC *dc, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy)
{
	MCRectangle drect, crect;

	if (m_rep != nil)
	{
		if (m_rep->GetType() == kMCImageRepVector)
		{
			MCU_set_rect(drect, dx - sx, dy - sy, rect.width, rect.height);
			MCU_set_rect(crect, dx, dy, sw, sh);
			static_cast<MCVectorImageRep*>(m_rep)->Render(dc, false, drect, crect);
		}
		else
		{
			// IM-2013-03-19: The original image bitmap & compressed format is only
			// needed when outputting to a printer context, so we only need to use
			// the current transformed version otherwise. This should prevent large
			// source images from flushing everything else out of the cache.
			bool t_success = true;

			MCImageBitmap *t_bitmap = nil;
			MCImageFrame *t_frame = nil;

			bool t_printer = dc->gettype() == CONTEXT_TYPE_PRINTER;
			bool t_update = !((state & CS_SIZE) && (state & CS_EDITED));

			if (t_printer && t_success)
				t_success = m_rep->LockImageFrame(currentframe, t_frame);
			if (t_success)
				t_success = lockbitmap(t_bitmap, t_update);
			if (t_success)
			{
				MCImageDescriptor t_image;

				t_image . bitmap = t_bitmap;
				t_image . original_bitmap = t_frame != nil ? t_frame->image : nil;
				t_image . angle = angle;

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

				dc -> drawimage(t_image, sx, sy, sw, sh, dx, dy);
			}
			else
			{
				// can't get image data from rep
				MCU_set_rect(drect, dx, dy, sw, sh);
				setforeground(dc, DI_BACK, False);
				dc->setbackground(MCscreen->getwhite());
				dc->setfillstyle(FillOpaqueStippled, DNULL, 0, 0);
				dc->fillrect(drect);
				dc->setbackground(MCzerocolor);
				dc->setfillstyle(FillSolid, DNULL, 0, 0);
			}

			m_rep->UnlockImageFrame(currentframe, t_frame);
			unlockbitmap(t_bitmap);
		}

		if (state & CS_DO_START)
		{
			MCImageFrame *t_frame = nil;
			if (m_rep->LockImageFrame(currentframe, t_frame))
			{
				MCscreen->addtimer(this, MCM_internal, t_frame->duration);
				m_rep->UnlockImageFrame(currentframe, t_frame);

				state &= ~CS_DO_START;
			}
		}
	}
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
	drawme(dc, 0, 0, rect.width, rect.height, x - (rect.width >> 1), y - (rect.height >> 1));
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
	MCStack *sptr = getstack()->findstackname_oldstring("Magnify");
	if (sptr == NULL)
		return;
	if (MCmagimage != NULL)
		MCmagimage->endmag(False);
	MCmagimage = this;
	state |= CS_MAGNIFY;

	//MCAutoStringRef buffer;
	//MCStringFormat(&buffer, "%d", rect.width * MCmagnification);
    //sptr->setsprop(P_MAX_WIDTH, *buffer);
    MCExecPoint ep(this, NULL, NULL);
    MCExecContext ctxt(ep);
    
    sptr->setintprop(ctxt, 0, P_MAX_WIDTH, False, rect.width * MCmagnification);

	//MCStringFormat(&buffer, "%d", rect.width * MCmagnification);
	//sptr->setsprop(P_MAX_HEIGHT, *buffer);
    sptr->setintprop(ctxt, 0, P_MAX_HEIGHT, False, rect.width * MCmagnification);


	uint2 ssize = MCU_min(32, rect.width);
	//MCStringFormat(&buffer, "%d", ssize * MCmagnification);
	//sptr->setsprop(P_WIDTH, *buffer);
    sptr->setintprop(ctxt, 0, P_WIDTH, False, ssize * MCmagnification);


	ssize = MCU_min(32, rect.height);
//	MCStringFormat(&buffer, "%d", ssize * MCmagnification);
//	sptr->setsprop(P_HEIGHT, *buffer);
    sptr->setintprop(ctxt, 0, P_HEIGHT, False, ssize * MCmagnification);


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
	MCmagimage = NULL;
	if (close)
	{
		MCStack *sptr = getstack()->findstackname_oldstring("Magnify");
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
		dc->setfillstyle(FillSolid, DNULL, 0, 0);
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
	if (MCmagnifier == NULL)
		endmag(False);

	MCRectangle t_mr;
	MCU_set_rect(t_mr, magrect . x + rect . x, magrect . y + rect . y, magrect . width, magrect . height);

	MCContext *t_context;
	t_context = MCscreen -> creatememorycontext(magrect . width, magrect . height, true, true);
	t_context -> setorigin(magrect . x + rect . x, magrect . y + rect . y);
	t_context -> setclip(t_mr);
	
	getstack() -> getcurcard() -> draw(t_context, t_mr, false); 
	
	MCBitmap *magimage;
	magimage = t_context -> lock();
	
	assert(magimage->depth == 32);

	uint2 linewidth = magrect.width * MCmagnification;
	MCImageBitmap *t_line = nil;
	/* UNCHECKED */ MCImageBitmapCreate(linewidth, MCmagnification, t_line);
	
	uint2 dy = drect.y / MCmagnification * MCmagnification;
	uint2 sbytes = magimage->bytes_per_line;
	uint4 yoffset = dy / MCmagnification * sbytes;
	while (dy < drect.y + drect.height)
	{
		uint32_t *t_src_row = (uint32_t*)&magimage->data[yoffset];
		uint32_t *t_dst_row = (uint32_t*)t_line->data;
		for (uindex_t x = 0 ; x < magrect.width ; x++)
		{
			uint4 color = 0xFF000000 | *t_src_row++;
			for (uint32_t i = 0; i < MCmagnification; i++)
				*t_dst_row++ = color;
		}
		for (uint32_t i = 1; i < MCmagnification; i++)
			memcpy((uint8_t*)t_line->data + i * t_line->stride, t_line->data, t_line->stride);
		
		// Render the scanline into the destination context.
		MCImageDescriptor t_image;
		memset(&t_image, 0, sizeof(MCImageDescriptor));
		t_image . bitmap = t_line;
		dest_context -> drawimage(t_image, 0, 0, linewidth, MCmagnification, 0, dy);
		
		dy += MCmagnification;
		yoffset += magimage->bytes_per_line;
	}

	MCImageFreeBitmap(t_line);
	
	t_context -> unlock(magimage);
	MCscreen -> freecontext(t_context);
}

Boolean MCImage::magmfocus(int2 x, int2 y)
{
	mx = x / MCmagnification + magrect.x + rect.x;
	my = y / MCmagnification + magrect.y + rect.y;

	static_cast<MCMutableImageRep *>(m_rep)->image_mfocus(mx, my);

	return True;
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

	if (static_cast<MCMutableImageRep *>(m_rep)->image_mdown(which) == True)
		return True;

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
