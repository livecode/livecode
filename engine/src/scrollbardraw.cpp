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
#include "font.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "field.h"
#include "scrolbar.h"
#include "mcerror.h"
#include "param.h"
#include "globals.h"
#include "mctheme.h"

#include "context.h"

static MCWidgetScrollBarInfo themesbinfo;

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCScrollbar::draw(MCDC *dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite)
{
	MCRectangle dirty;
	dirty = p_dirty;

	flags &= ~F_STYLE;
	if (rect.width > rect.height)
		flags |= F_HORIZONTAL;
	else
		flags |= F_VERTICAL;

	if (!p_isolated)
	{
		// MW-2011-09-06: [[ Redraw ]] If rendering as a sprite, don't change opacity or ink.
		if (!p_sprite)
		{
			dc -> setfunction(ink);
			dc -> setopacity(blendlevel * 255 / 100);
		}
		
		// MW-2009-06-10: [[ Bitmap Effects ]]
		if (m_bitmap_effects == NULL)
			dc -> begin(false);
		else
		{
			if (!dc -> begin_with_effects(m_bitmap_effects, rect))
				return;
			dirty = dc -> getclip();
		}
	}

	MCWidgetInfo winfo;
	winfo.type = (Widget_Type)getwidgetthemetype();
	if (MCcurtheme && MCcurtheme->iswidgetsupported(winfo.type))
	{
		getwidgetthemeinfo(winfo);
		MCcurtheme->drawwidget(dc, winfo, rect);
#ifdef _MAC_DESKTOP
		// MW-2012-09-17: [[ Bug 9212 ]] If we are a progress bar then make sure
		//   we animate.
		if (getflag(F_PROGRESS))
        {
            // MM-2014-07-31: [[ ThreadedRendering ]] Make sure only a single thread posts the timer message (i.e. the first that gets here)
            if (!m_animate_posted)
            {
                if (!m_animate_posted)
                {
                    m_animate_posted = true;
                    MCscreen -> addtimer(this, MCM_internal3, 1000 / 30);
                }
            }
        }
#endif
		if (flags & F_SHOW_VALUE)
		{
			MCRectangle thumb = compute_thumb(thumbpos);
			drawvalue(dc, thumb);
		}
	}
	else
	{
		MCRectangle frect = compute_bar();
		if (MClook == LF_MOTIF && state & CS_KFOCUSED
		        && !(extraflags & EF_NO_FOCUS_BORDER))
			drawfocus(dc, p_dirty);

		MCRectangle trect;
		if (IsMacEmulatedLF())
			trect = MCU_reduce_rect(frect, 1);
		else
			trect = MCU_reduce_rect(frect, borderwidth);

		// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
		if (IsMacEmulatedLF() && issbdisabled() && !(flags & F_SCALE))
		{
			//Mac's disabled Scale does not need background filled.
			//need to use a color that is a bit darker than MAC_DISABLED for disabled
			//Mac Progress bar. If (flags & F_PROGRESS) set different foreground color
			dc->setforeground(maccolors[MAC_DISABLED]);
			dc->setfillstyle(FillSolid, nil, 0, 0);
			dc->fillrect(trect);
		}
		//window's progress bar, Motif's scrollbar and
		//Mac's regular scroll bar need to have rectangle
		//no need to fill the background for Win's Progress & Scale bars
		if (MClook == LF_WIN95 && !(flags & F_SB_STYLE))
		{
			dc->setforeground(dc->getgray());
			dc->setbackground(MCscreen->getwhite());
			dc->setfillstyle(FillOpaqueStippled, nil, 0, 0);
			dc->fillrect(trect);
			dc->setbackground(MCzerocolor);
			dc->setfillstyle(FillSolid, nil, 0, 0);
		}
		else		
			// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
			if (!issbdisabled() && flags & F_OPAQUE
			        && (MClook == LF_MOTIF || flags & F_PROGRESS
			            || getstyleint(flags) == F_VERTICAL
			            || (IsMacEmulatedLF() && !(flags & F_SB_STYLE))))
			{ //filled
				setforeground(dc, DI_SHADOW, False);
				dc->fillrect(trect);
				if (IsMacEmulatedLF() && !(flags & F_SB_STYLE))
				{
					MCRectangle brect = trect;
					dc->setforeground(MCscreen->getblack());
					dc->setfillstyle(FillSolid, nil, 0, 0);
					if (getstyleint(flags) == F_VERTICAL)
					{
						brect.y += trect.width;
						brect.height -= (trect.width << 1) + 2;
						dc->drawline(brect.x, brect.y, brect.x + brect.width, brect.y);
						dc->drawline(brect.x, brect.y + brect.height + 1,
						             brect.x + brect.width, brect.y + brect.height + 1);
						brect.y++;
					}
					else
					{
						brect.x += trect.height;
						brect.width -= (trect.height << 1) + 2;
						dc->drawline(brect.x, brect.y, brect.x, brect.y + brect.height);
						dc->drawline(brect.x + brect.width + 1, brect.y,
						             brect.x + brect.width + 1, brect.y + brect.height);
						brect.x++;
					}
					draw3d(dc, brect, ETCH_SUNKEN, 1);
				}
			}
		if (!(flags & F_SB_STYLE))
		{
			// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
			Boolean db = !IsMacEmulatedLF() || issbdisabled() == 0;
			if (getstyleint(flags) == F_VERTICAL)
			{
				drawarrow(dc, trect.x, trect.y, trect.width, AD_UP, db,
				          mode == SM_LINEDEC && state & CS_MFOCUSED);
				drawarrow(dc, trect.x, trect.y + trect.height - trect.width, trect.width,
				          AD_DOWN, db, mode == SM_LINEINC && state & CS_MFOCUSED);
				if (!db)
				{
					dc->setforeground(dc->getgray());
					dc->setfillstyle(FillSolid, nil, 0, 0);
					int2 y = trect.y + trect.width;
					dc->drawline(trect.x, y, trect.x + trect.width, y);
					y = trect.y + trect.height - trect.width;
					dc->drawline(trect.x, y, trect.x + trect.width, y);
				}
			}
			else
			{
				drawarrow(dc, trect.x, trect.y, trect.height, AD_LEFT, db,
				          mode == SM_LINEDEC && state & CS_MFOCUSED);
				drawarrow(dc, trect.x + trect.width - trect.height, trect.y,
				          trect.height, AD_RIGHT, db,
				          mode == SM_LINEINC && state & CS_MFOCUSED);
				if (!db)
				{
					dc->setforeground(dc->getgray());
					dc->setfillstyle(FillSolid, nil, 0, 0);
					int2 x = trect.x + trect.height;
					dc->drawline(x, trect.y, x, trect.y + trect.height);
					x = trect.x + trect.width - trect.height;
					dc->drawline(x, trect.y, x, trect.y + trect.height);
				}
			}
		}
		if (flags & F_PROGRESS)
		{
			trect = MCU_reduce_rect(trect, 1);
			int2 endpos = (int2)(thumbpos / (endvalue - startvalue)
			                     * (real8)trect.width) + trect.x;
			if (IsMacEmulatedLF() && getstyleint(flags) == F_HORIZONTAL)
				DrawMacProgressBar(dc, trect, endpos);
			else
				DrawWinProgressBar(dc, trect, endpos);
		}
		else
		{ //draw Scale and Scrollbar
			MCRectangle thumb = compute_thumb(thumbpos);
			if (MClook != LF_MOTIF && flags & F_SCALE
			        && getstyleint(flags) == F_HORIZONTAL)
			{
				if (IsMacEmulatedLF())
					DrawMacScale(dc, trect, thumb);
				else
					DrawWinScale(dc, trect, thumb);
			}
			else		
				// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
				if (!issbdisabled())
				{
					if (thumb.width > 0 && thumb.height > 0)
					{
						if (MClook != LF_MOTIF && flags & F_OPAQUE)
							if (IsMacEmulatedLF())
							{
								if (state & CS_SCROLL)
									dc->setforeground(maccolors[MAC_THUMB_BOTTOM]);
								else
									dc->setforeground(maccolors[MAC_THUMB_BACK]);
								dc->setfillstyle(FillSolid, nil, 0, 0);
							}
							else
								if (parent->gettype() == CT_FIELD)
									parent->getparent()->setforeground(dc, DI_BACK, False);
								else
									setforeground(dc, DI_BACK, False);
						else
							setforeground(dc, DI_BACK, False);
						dc->fillrect(thumb);
						if (dc->getdepth() != 1)
						{
							if (IsMacEmulatedLF() && !(flags & F_SCALE))
								drawmacthumb(dc, thumb);
							else
								if (flags & F_3D)
									draw3d(dc, thumb, ETCH_RAISED_SMALL, DEFAULT_BORDER);
								else
									drawborder(dc, thumb, DEFAULT_BORDER);
						}
						thumb = MCU_reduce_rect(thumb, DEFAULT_BORDER);
						if (flags & F_SCALE)
						{ //draw Motif's Scale
							if (getstyleint(flags) == F_VERTICAL)
							{ //Vertical scale
								uint2 y = thumb.y + (thumb.height >> 1);
								setforeground(dc, DI_TOP, False);
								dc->drawline(thumb.x - 1, y, thumb.x + thumb.width, y);
								y--;
								setforeground(dc, DI_BOTTOM, False);
								dc->drawline(thumb.x - 1, y, thumb.x + thumb.width, y);
							}
							else
							{
								uint2 x = thumb.x + (thumb.width >> 1);
								setforeground(dc, DI_TOP, False);
								dc->drawline(x, thumb.y - 1,
								             x, thumb.y + thumb.height);
								x--;
								setforeground(dc, DI_BOTTOM, False);
								dc->drawline(x, thumb.y - 1,
								             x, thumb.y + thumb.height);
							}
						}
					}
					if (flags & F_SHOW_VALUE)
						drawvalue(dc, thumb);
				}
		}
		if (flags & F_SHOW_BORDER)
		{
			if (IsMacEmulatedLF())
			{
				if (flags & F_PROGRESS)
				{
					MCPoint p[5];
					//draw a white outline around ther entire progress bar, if it's disabled
					// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
					if (issbdisabled())
					{
						p[0].x = p[1].x = frect.x ;
						p[0].y = p[3].y = frect.y;
						p[1].y = p[2].y = p[0].y + frect.height -1;
						p[2].x = p[3].x = p[0].x + frect.width - 1;
						p[4] = p[0];
						setforeground(dc, DI_TOP, False);
						dc->drawlines(p, 5);
						//draw a Gray outline to the disabled progress bar
						p[0].x +=1;
						p[1].x = p[0].x;
						p[0].y += 1;
						p[3].y = p[0].y;
						p[1].y -= 1;
						p[2].y = p[1].y;
						p[2].x -= 1;
						p[3].x = p[2].x;
						p[4] = p[0];
						setforeground(dc, DI_FORE, False);
						dc->drawlines(p, 5);
					}
					else
					{
						// draw a MAC 3D effect arround the progress bar, which
						// means Gray on top White on bottom draw Gray color first
						p[0].x = p[1].x = frect.x;
						p[0].y = frect.y + frect.height;
						p[1].y = frect.y;
						p[2].x = frect.x + frect.width - 1;
						p[2].y = p[1].y;
						dc->setforeground(maccolors[MAC_SHADOW]);
						dc->setfillstyle(FillSolid, nil, 0, 0);
						dc->drawlines(p, 3);
						//draw White part of 3D effect
						p[0].x = frect.x;
						p[0].y = frect.y + frect.height - 1;
						p[1].x = frect.x + frect.width - 1;
						p[1].y = p[0].y;
						p[2].y = frect.y;
						p[2].x = p[1].x;
						setforeground(dc, DI_TOP, False);
						dc->drawlines(p, 3);
						/* draw a Black border inside of 3D outline */
						p[0].x = p[1].x = frect.x + 1;
						p[0].y = p[3].y = frect.y + 1;
						p[1].y = p[2].y = p[0].y + frect.height - 3;
						p[2].x = p[3].x = p[0].x + frect.width - 3;
						p[4] = p[0];
						dc->setforeground(dc->getblack());
						dc->drawlines(p, 5);
					}
				}
				else
				{
					if (MClook == LF_MOTIF || !(flags & F_SB_STYLE)
					        || getstyleint(flags) == F_VERTICAL)
						drawborder(dc, frect, 1);
				}
			}
			else
			{
				if (MClook != LF_WIN95 || !(flags & F_SCALE)
				        || getstyleint(flags) == F_VERTICAL)
				{
					if (flags & F_3D)
						draw3d(dc, frect, ETCH_SUNKEN, borderwidth);
					else
						drawborder(dc, frect, borderwidth);
				}
			}
		}
	}

	if (!p_isolated)
	{
		dc -> end();
	}
}

/*the color enum is defined in mccontrol.h as:
  5 colors: MAC_THUMB_TOP,MAC_THUMB_BACK,MAC_THUMB_BOTTOM,
  MAC_THUMB_GRIP,MAC_THUMB_HILITE, from light blue to darkest blue */
#define MAC_THUMBCOLORS 5

void MCScrollbar::DrawMacProgressBar(MCDC *dc, MCRectangle &trect, int2 endpos)
{
	int2 saveWidth = trect.width;
	trect.width = endpos - trect.x; //compute the thumb width
	trect.x -=1;
	trect.y -=1;
	trect.width +=1;
	trect.height +=2;

	// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
	if (issbdisabled())
	{//draw the progress bar's thumb
		//fill the thumb with color
		dc->setforeground(maccolors[MAC_THUMB_BACK]);
		dc->setfillstyle(FillSolid, nil, 0, 0);



		dc->fillrect(trect);
		//draw a Gray vertical line at the right edge of the thumb
		setforeground(dc, DI_FORE, False);
	}
	else
	{//Draw a Black vertical line at the right edge of the thumb
		dc->setforeground(dc->getblack());
		dc->setfillstyle(FillSolid, nil, 0, 0);
	}
	dc->drawline(trect.x + trect.width, trect.y, trect.x + trect.width,
	             trect.y + trect.height);

	/* draw active Progress bar's thumb */
	// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
	if (!issbdisabled())
	{
		if (trect.width < 2)
			return;
		MCPoint p[3];

		//draw a vertical gray line next to the vertical balck line
		dc->setforeground(maccolors[MAC_SHADOW]);
		dc->setfillstyle(FillSolid, nil, 0, 0);
		dc->drawline(trect.x + trect.width + 1, trect.y, trect.x + trect.width + 1,
		             trect.y + trect.height);
		/*draw Mac 3D effect, gray on top left corner, white on bottom right corner*/
		dc->setforeground(dc->getgray());//draw  gray part of 3D effect
		p[0].x = p[1].x = trect.x + trect.width + 2;
		p[0].y = trect.y + trect.height;
		p[1].y = p[2].y = trect.y + 1;
		p[2].x = trect.x + saveWidth + 1;
		dc->drawlines(p, 3);
		p[0].x += 1; //draw white part of 3D effect. base on existing p values
		p[2].x -= 1;
		p[1].x = p[2].x;
		p[0].y -= 2;
		p[1].y = p[0].y;
		setforeground(dc, DI_TOP, False);
		dc->drawlines(p, 3);

		/* draw thumb part of the progress bar */
		MCRectangle oldRect = trect; //save a copy of trect
		MCRectangle saveRect = trect;

		uint2 rectCount = trect.height / 2; //count how many rect to be drawn
		uint2 thumbHilitH = rectCount - 1;
		int2 i, t;
		for (i = 0; trect.width && trect.height ; i++)
		{
			t = (i * MAC_THUMBCOLORS) / (rectCount - 1);
			//in order to draw more lighter blue rect, we increase rectCount + 1
			//each time. To make sure (MAC_THUMBCOLORS - t) does not go less
			//than 1, use MCU_max() function set the color to draw. from outside
			//going in, darkest colors to lightest color
			dc->setforeground(maccolors[MCU_max(MAC_THUMBCOLORS -t, 1)]);
			dc->drawrect(trect); //draw a rect
			trect = MCU_reduce_rect(trect, 1);
		}

		uint2 linethick = oldRect.height / 8;
		//draw light blue vertical line to the left side of thumb
		dc->setforeground(maccolors[MAC_THUMB_BOTTOM]);
		for (i = 0; i < linethick; i++)
		{
			dc->drawline (oldRect.x + 1, oldRect.y, oldRect.x + 1,
			              oldRect.y + oldRect.height);
			oldRect.x += 1;
		} //oldRect.x has been moved to the right the width of linethick

		//draw another light blue vertical line
		for (i = 0; i < linethick; i++ )
		{
			dc->setforeground(maccolors[MAC_THUMB_BACK]);
			p[0].x = p[1].x = oldRect.x + 1;
			p[0].y = oldRect.y + (thumbHilitH / 3);
			p[1].y = oldRect.y + oldRect.height - (thumbHilitH / 2);
			dc->drawline(p[0].x, p[0].y, p[1].x, p[1].y);
			oldRect.x += 1;
		}
		// draw a white T-shape lines in the center of the thumb if thumb width >= height
		setforeground(dc, DI_TOP, False);
		oldRect = saveRect;
		if (oldRect.width >= oldRect.height)
		{
			//draw the horizontal line of the T-shape
			for (i = 0; i < linethick; i++)
			{
				p[0].y = p[1].y =  oldRect.y + thumbHilitH + 1;
				p[0].x = oldRect.x + thumbHilitH;
				p[1].x = oldRect.x + oldRect.width - thumbHilitH;
				dc->drawline(p[0].x, p[0].y, p[1].x, p[1].y);
				oldRect.y +=1;
			}
			//draw the vertical line of the T-shape
			for (i=0; i< linethick; i++)
			{
				p[0].x = oldRect.x + thumbHilitH;
				p[1].x = oldRect.x + oldRect.width - thumbHilitH;
				//p[0].y unchanged
				dc->drawline(p[0].x, p[0].y + 1, p[0].x, p[0].y - linethick);
				oldRect.x -= 1;
			}
		} // thumb width >= height
	}
}

void MCScrollbar::DrawWinProgressBar(MCDC *dc, MCRectangle &trect, int2 endpos)
{
	uint2 index;
	if (getpindex(DI_HILITE, index))
	{
		setforeground(dc, DI_HILITE, False);
		trect.width = endpos - trect.x;
		dc->fillrect(trect);
	}
	else
	{
		if (getcindex(DI_HILITE, index))
			setforeground(dc, DI_HILITE, False);
		else
		{
			dc->setforeground(MCaccentcolor);
			dc->setfillstyle(FillSolid, nil, 0, 0);
		}
		uint2 blockwidth = trect.height * 2 / 3;
		trect.width = blockwidth;
		while (trect.x < endpos)
		{
			dc->fillrect(trect);
			trect.x += blockwidth + 2;
		}
	}
}

void MCScrollbar::DrawMacScale(MCDC *dc, MCRectangle &trect, MCRectangle &thumb)
{
	uint2 centerHeight = 4;
	MCRectangle r = trect;
	r.x +=2;
	r.y = r.y + centerHeight;
	r.width -=2;
	r.height = 3;
	setforeground(dc, DI_SHADOW, False);
	dc->fillrect(r);

	/*save the y height for thumb drawing latter */
	uint2 thumbStartH = r.y - 2; //start from the pixel above the topmost 3d line

	/*draw Black or Gray(disabled) outline arround the filled center area */

	// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
	if (issbdisabled())
		dc->setforeground(dc->getgray());
	else
		dc->setforeground(dc->getblack());
	dc->setfillstyle(FillSolid, nil, 0, 0);
	MCPoint p[12];
	p[0].x = p[1].x = r.x + 1;
	p[0].y = r.y;
	p[1].y = r.y + 2;
	p[2].x = p[0].x + 1;
	p[2].y = p[3].y = p[1].y + 1;
	p[3].x = p[2].x + r.width - 4;
	p[4].x = p[5].x = p[3].x + 1;
	p[4].y = p[3].y - 1;
	p[5].y = p[0].y;
	p[6].x = p[5].x - 1;
	p[6].y = p[7].y = p[5].y - 1;
	p[7].x = p[0].x + 1;
	dc->drawlines(p, 8);

	/* draw 3D around enabled Scale */
	
	// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
	if (!issbdisabled())
	{

		//draw White border from left to right on the bottom
		p[0].y = p[1].y + 1;
		p[1].x = p[0].x + 1;
		p[1].y = p[0].y + 1;
		p[2].x = p[3].x + 1;
		p[2].y = p[1].y;
		p[3].x = p[2].x;
		p[3].y = p[2].y - 1;
		p[4].x = p[3].x + 1;
		p[4].y = p[3].y;
		p[5].x = p[4].x;
		p[5].y = p[4].y - 3;
		p[6].x = p[5].x - 1;
		p[6].y = p[5].y - 1;
		setforeground(dc, DI_TOP, False);
		dc->drawlines(p, 7);
		//draw Gray border from right to left on top of the White hightlight
		p[0].x = p[6].x - 1;
		p[0].y = p[1].y = p[2].y = p[6].y - 1;
		p[1].x = rect.x + 5;
		p[2].x = p[3].x = p[1].x - 1;
		p[3].y = p[4].y = p[2].y + 1;
		p[4].x = p[5].x = p[3].x - 1;
		p[5].y = p[4]. y + 3;
		dc->setforeground(dc->getgray());
		dc->setfillstyle(FillSolid, nil, 0, 0);
		dc->drawlines(p, 6);
	}
	/* draw the thumb of the scale, Mac Scale's thumb is 15 wide x 16 tall */
	
	// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
	if (issbdisabled())
	{ //fill with light gray color if is disabled
		setforeground(dc, DI_BACK, False);
	}
	else
	{
		if (state & CS_SCROLL)
			dc->setforeground(maccolors[MAC_THUMB_BOTTOM]);
		else
			dc->setforeground(maccolors[MAC_THUMB_BACK]); //light blue color
		dc->setfillstyle(FillSolid, nil, 0, 0);
	}
	//draw the thumb outline start from 9 o'clock and clockwise arround 360 degree
	p[0].x = p[1].x = thumb.x;
	p[0].y = thumbStartH;
	p[1].y = p[0].y - 2;
	p[2].x = p[1].x + 1;
	p[2].y = p[3].y = p[1].y - 1;
	p[3].x = p[2].x + 12;
	p[4].x = p[5].x = p[3].x + 1;
	p[4].y = p[3].y + 1;
	p[5].y = p[4].y + 8;
	p[6].x = p[5].x - 6;
	p[6].y = p[5].y + 6;
	p[7].x = p[6].x - 2;
	p[7].y = p[6].y;
	p[8].x = p[7].x - 5;
	p[8].y = p[7].y - 5;
	p[9].x = p[8].x - 1;
	p[9].y = p[8].y - 1;
	p[10] = p[0];
	dc->fillpolygon(p, 11); //fill the thumb with background color

	
	// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
	if (issbdisabled())
		dc->setforeground(dc->getgray());
	else
		dc->setforeground(dc->getblack());
	dc->setfillstyle(FillSolid, nil, 0, 0);
	dc->drawlines(p, 11); //draw thumb outline

	// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
	if (!issbdisabled())
	{//draw blue highlights in the thumb
		MCPoint cp;
		cp = p[0]; //save the p[0] into cp as a reference point.
		p[0].x +=1;
		p[0].y += 6;
		p[1].x = p[0].x;
		p[1].y = p[2].y = p[0].y - 8;
		p[2].x = p[1].x + 12;
		dc->setforeground(maccolors[MAC_THUMB_TOP]);
		dc->drawlines(p, 3);
		p[0] = p[2];
		p[1].x = p[0].x;
		p[1].y = p[0].y + 8;
		dc->setforeground(maccolors[MAC_THUMB_BOTTOM]);
		dc->drawlines(p, 3);
		//draw the shading of the triangle part of the thumb
		p[0].x = p[5].x - 1;
		p[0].y = p[5].y;
		p[1].x = p[0].x - 5;
		p[1].y = p[2].y = p[0].y + 5;
		p[2].x = p[1].x - 2;
		p[3].x = p[2].x - 4;
		p[3].y = p[2].y - 4;
		dc->drawlines(p, 4);
		//draw 3 vertical light blue lines from bottom to top and from left to right
		uint2 shift = state & CS_SCROLL ? 1 : 0;
		p[0].x = cp.x + 4;
		p[0].y = cp.y;
		p[1].y = cp.y + 5;
		dc->setforeground(maccolors[MAC_THUMB_TOP + shift]);
		dc->drawline(p[0].x, p[0].y, p[0].x, p[1].y);
		p[0].x += 2;
		dc->drawline(p[0].x , p[0].y, p[0].x, p[1].y);
		p[0].x += 2;
		dc->drawline(p[0].x , p[0].y, p[0].x, p[1].y);
		//draw 3 vertical dark blue lines from bottom to top and from left to right
		dc->setforeground(maccolors[MAC_THUMB_GRIP + shift]);
		p[0].x = cp.x + 5;
		p[0].y = cp.y + 1;
		p[1].y = cp.y + 6;
		dc->drawline(p[0].x , p[0].y, p[0].x, p[1].y);
		p[0].x += 2;
		dc->drawline(p[0].x , p[0].y, p[0].x, p[1].y);
		p[0].x += 2;
		dc->drawline(p[0].x , p[0].y, p[0].x, p[1].y);
	}
	if (flags & F_SHOW_VALUE)
		drawvalue(dc, thumb);
	else
		drawticks(dc, thumb);
}

void MCScrollbar::DrawWinScale(MCDC *dc, MCRectangle &trect, MCRectangle &thumb)
{ /*routine draws WIN style horizontal scale */
	MCPoint p[6];
	real8 sf = 1.0;     //scale factor



	/*if the rect height is less than 26, scale down the object by a scale factor */
	if (trect.height < WIN_SCALE_HEIGHT) //26.0
		sf = (real8)trect.height / WIN_SCALE_HEIGHT;

	/*draw horizontal lines accross the width of the rect 1/4 the way down */
	uint2 h = MCU_min(8, (uint2)(rect.height / 4.0 * sf) + 1);
	p[0].x = rect.x;
	p[0].y = rect.y + h;
	p[1].x = rect.x + rect.width;
	dc->setforeground(dc->getgray()); //gray
	dc->setfillstyle(FillSolid, nil, 0, 0);
	dc->drawline(p[0].x, p[0].y, p[1].x, p[0].y);
	p[0].y += 1;
	dc->setforeground(dc->getblack()); //black
	dc->drawline(p[0].x, p[0].y, p[1].x, p[0].y);
	p[0].y += 1;
	setforeground(dc, DI_TOP, False); //white
	dc->drawline(p[0].x, p[0].y, p[1].x, p[0].y);
	p[0].y += 1;
	setforeground(dc, DI_TOP, False); //white
	dc->drawline(p[0].x, p[0].y, p[1].x, p[0].y);
	/*draw the thumb*/
	int2 mx = thumb.x + (thumb.width >> 1);
	int2 by = thumb.y + thumb.height - (thumb.width >> 1) - 1;
	// define the shape of the thumb, to be filled with background color
	//this filled area is 4 pixel narrower than the actual thumb width, due to the
	//3D effect on other side of the thumb
	p[0].x = p[4].x = thumb.x + 2;
	p[1].x = p[2].x = thumb.x + thumb.width - 2;
	p[3].x = mx;
	p[0].y = p[1].y = thumb.y + 2;
	p[2].y = p[4].y = by;
	p[3].y = thumb.y + thumb.height - 2;
	p[5] = p[0];
	
	// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
	if (issbdisabled()) //fill with back color if is disabled
		setforeground(dc, DI_BACK, False);
	else
		setforeground(dc, DI_SHADOW, False);
	dc->fillpolygon(p, 6);

	//draw light gray
	p[0].x = thumb.x + thumb.width - 2;
	p[1].x = p[2].x = thumb.x + 1;
	p[3].x = mx - 1;
	p[0].y = p[1].y = thumb.y + 1;
	p[2].y = by;
	p[3].y = thumb.y + thumb.height - 3;
	setforeground(dc, DI_BACK, False);
	dc->drawlines(p, 4);

	//dark gray on the right side
	p[0].x = p[1].x = thumb.x + thumb.width - 2;
	p[2].x = mx;
	p[0].y = thumb.y + 1;
	p[1].y = by;
	p[2].y = thumb.y + thumb.height - 2;
	dc->setforeground(dc->getgray());
	dc->setfillstyle(FillSolid, nil, 0, 0);
	dc->drawlines(p, 3);

	//draw black
	p[0].x = p[1].x = thumb.x + thumb.width - 1;
	p[2].x = mx;
	p[0].y = thumb.y;
	p[1].y = by;
	p[2].y = thumb.y + thumb.height - 1;
	dc->setforeground(dc->getblack());
	dc->drawlines(p, 3);

	//draw white
	p[0].x = thumb.x + thumb.width - 2;
	p[1].x = p[2].x = thumb.x;
	p[3].x = mx - 1;
	p[0].y = p[1].y = thumb.y;
	p[2].y = by;
	p[3].y = thumb.y + thumb.height - 2;
	setforeground(dc, DI_TOP, False); //White
	dc->drawlines(p, 4);
	if (flags & F_SHOW_VALUE)
		drawvalue(dc, thumb);
	else
		drawticks(dc, thumb);
}

void MCScrollbar::drawvalue(MCDC *dc, MCRectangle &thumb)
{
	int32_t fascent, fdescent;
	fascent = MCFontGetAscent(m_font);
	fdescent = MCFontGetDescent(m_font);
	if (rect.height - thumb.height > fascent)
	{
		MCAutoStringRef t_data;
		/* UNCHECKED */ MCU_r8tos(thumbpos, nffw, nftrailing, nfforce, &t_data);
        // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
        uint2 tw = MCFontMeasureText(m_font, *t_data, getstack() -> getdevicetransform());
		if (getstyleint(flags) == F_VERTICAL)
		{
			uint2 sx = thumb.x + thumb.width + ((rect.width - thumb.width - tw) >> 1);
			uint2 sy = thumb.y + ((thumb.height + fascent) >> 1);
			setforeground(dc, DI_FORE, False);
            dc -> drawtext(sx, sy, *t_data, m_font, false, kMCDrawTextNoBreak);
		}
		else
		{
			int2 sx = thumb.x + ((thumb.width - tw) >> 1);
			if (sx < rect.x)
				sx = rect.x;
			if (sx + tw > rect.x + rect.width)
				sx = rect.x + rect.width - tw;
			uint2 sy;
			if (MClook == LF_MOTIF)
				sy = rect.y + fascent;
			else
				sy = rect.y + rect.height - fdescent;
			setforeground(dc, DI_FORE, False);
            dc -> drawtext(sx, sy, *t_data, m_font, false, kMCDrawTextNoBreak);
		}
	}
}

void MCScrollbar::drawticks(MCDC *dc, MCRectangle &thumb)
{
	MCPoint p;
	dc->setforeground(dc->getblack()); //black
	dc->setfillstyle(FillSolid, nil, 0, 0);
	int2 sx;
	if (IsMacEmulatedLF())
	{
		thumb.width = 27;
		sx = rect.x + 1;
	}
	else
		sx = rect.x;
	uint2 halfThumbWidth = thumb.width >> 1;
	//draw scale marks in the middle
	//a space between tip of thumb and the top of tick mark
	
	// MW-2007-08-30: [[ Bug 4155 ]] Ensure the case of endvalue < startvalue is handled correctly.
	uint2 nticks = (uint2)(fabs(endvalue - startvalue) / (pageinc - lineinc)) + 1;
	uint2 sw = rect.width - thumb.width + 1;
	p.y = thumb.y + thumb.height + 2;
	uint2 i;
	for (i = 1 ; i < nticks ; i++)
	{
		p.x = sx + halfThumbWidth + i * sw / nticks;
		dc->drawline(p.x, p.y, p.x, p.y + 3);
	}
	//draw first scale mark
	p.x = sx + halfThumbWidth;
	dc->drawline(p.x, p.y, p.x, p.y + 4);
	//draw last scale mark
	p.x = sx + rect.width - halfThumbWidth;
	dc->drawline(p.x, p.y, p.x, p.y + 4);
}

void MCScrollbar::drawmacthumb(MCDC *dc, MCRectangle &thumb)
{
	int2 lx = thumb.x;
	int2 rx = thumb.x + thumb.width - 1;
	int2 ty = thumb.y;
	int2 by = thumb.y + thumb.height - 1;

	uint2 shift = state & CS_SCROLL ? 1 : 0;

	MCPoint p[3];
	MCLineSegment grid[5];
	uint2 gridpoints;
	if (getstyleint(flags) == F_VERTICAL)
	{
		if (thumb.height <= 4)
			return;
		gridpoints = MCU_min(10, thumb.height - 4);
		ty += ((thumb.height - gridpoints) >> 1) + 1;
		gridpoints >>= 1;
		uint2 offset = thumb.width >> 2;
		uint2 i;
		for (i = 0 ; i < gridpoints ; i++)
		{
			grid[i].x1 = lx + offset;
			grid[i].x2 = rx - offset - 1;
			grid[i].y1 = grid[i].y2 = ty;
			ty += 2;
		}
		dc->setforeground(maccolors[MAC_THUMB_TOP + shift]);
		dc->setfillstyle(FillSolid, nil, 0, 0);
		dc->drawsegments(grid, gridpoints - 1);
		for (i = 0 ; i < gridpoints ; i++)
		{
			grid[i].y1 = ++grid[i].y2;
			grid[i].x1++;
			grid[i].x2++;
		}

		dc->setforeground(maccolors[MAC_THUMB_GRIP + shift]);
		dc->drawsegments(grid, gridpoints - 1);
		ty = thumb.y;
		by = thumb.y + thumb.height - 1;
		p[0].x = lx;
		p[1].x = rx;
		p[0].y = p[1].y = ty;
		dc->setforeground(MCscreen->getblack());
		dc->drawlines(p, 2);
		p[0].y = p[1].y = by;
		dc->drawlines(p, 2);
		ty++;
		by--;
	}
	else
	{
		if (thumb.height <= 6)
			return;
		gridpoints = MCU_min(10, thumb.width - 6);
		lx += ((thumb.width - gridpoints) >> 1) + 1;
		gridpoints >>= 1;
		uint2 offset = thumb.height >> 2;
		uint2 i;
		for (i = 0 ; i < gridpoints ; i++)
		{
			grid[i].x1 = grid[i].x2 = lx;
			lx += 2;
			grid[i].y1 = ty + offset;
			grid[i].y2 = by - offset - 1;
		}
		dc->setforeground(maccolors[MAC_THUMB_TOP + shift]);
		dc->setfillstyle(FillSolid, nil, 0, 0);
		dc->drawsegments(grid, gridpoints - 1);
		for (i = 0 ; i < gridpoints ; i++)
		{
			grid[i].x1 = ++grid[i].x2;
			grid[i].y1++;
			grid[i].y2++;
		}
		dc->setforeground(maccolors[MAC_THUMB_GRIP + shift]);
		dc->drawsegments(grid, gridpoints - 1);
		lx = thumb.x;
		rx = thumb.x + thumb.width - 1;
		p[0].x = p[1].x = lx;
		p[0].y = ty;
		p[1].y = by;
		dc->setforeground(MCscreen->getblack());
		dc->drawlines(p, 2);
		p[0].x = p[1].x = rx;
		dc->drawlines(p, 2);
		lx++;
		rx--;
	}
	p[0].x = p[1].x = lx;
	p[2].x = rx;
	p[0].y = by;
	p[1].y = p[2].y = ty;
	dc->setforeground(maccolors[MAC_THUMB_TOP + shift]);
	dc->drawlines(p, 3);
	p[0].x = lx + 1;
	p[1].x = p[2].x = rx;
	p[0].y = p[1].y = by;
	p[2].y = ty + 1;
	dc->setforeground(maccolors[MAC_THUMB_BOTTOM + shift]);
	dc->drawlines(p, 3);
}


uint2 MCScrollbar::getwidgetthemetype()
{
	int4 smallscrollbarsize = 10;
	// MW-2004-11-17: Small scrollbars on OS X are a slightly different size
	// MW-2009-11-01: Make sure we don't divide by zero for 0-width scrollbarwidth's in the field!
	if (IsMacLFAM())
		smallscrollbarsize = 14;
	else if (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEGTK)
		smallscrollbarsize = MCU_min(smallscrollbarsize,MCcurtheme->getmetric(WTHEME_METRIC_TRACKSIZE) - 5);
	if (flags & F_PROGRESS)
		return WTHEME_TYPE_PROGRESSBAR;
	if (flags & F_SCALE)
		return WTHEME_TYPE_SLIDER;
	else if ((rect.width > rect.height && rect.height < smallscrollbarsize) ||
	         (rect.height > rect.width && (rect . width == 0 || rect.height / rect.width < 3)) ||
	         rect.width < smallscrollbarsize)
		return WTHEME_TYPE_SMALLSCROLLBAR;
	else
		return WTHEME_TYPE_SCROLLBAR;
}

void MCScrollbar::getwidgetthemeinfo(MCWidgetInfo &widgetinfo)
{
	uint4 wstate = 0;
	Widget_Part wpart = WTHEME_PART_UNDEFINED;
	uint4 wattributes = 0;
	MCControl::getwidgetthemeinfo(widgetinfo);
	//define type of control
	//define attributes
	if (flags & F_SHOW_VALUE)
		wattributes |= WTHEME_ATT_SHOWVALUE;
	if (rect.height > rect.width)
		wattributes |= WTHEME_ATT_SBVERTICAL;
		
	//define state
	
	// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
	if (issbdisabled())
		wstate |= WTHEME_STATE_DISABLED;
	if (state & CS_MFOCUSED && !(state & CS_SELECTED))
		wstate |= WTHEME_STATE_PRESSED;
	if (state & CS_KFOCUSED)
		wstate |= WTHEME_STATE_HASFOCUS;
	if (state & CS_HILITED)
		wstate |= WTHEME_STATE_HILITED;

	if (state & CS_SCROLL)  //in scrolling mode, means the thumb is pressed
	{
		wpart = WTHEME_PART_THUMB;
		wstate |= WTHEME_STATE_HOVER;
	}
	else if (mode != SM_CLEARED && getstate(CS_MFOCUSED))
	{
		switch (mode)
		{
		case SM_LINEDEC:
			wpart = WTHEME_PART_ARROW_DEC;
			break;
		case SM_LINEINC:
			wpart = WTHEME_PART_ARROW_INC;
			break;
		case SM_PAGEDEC:
			wpart = WTHEME_PART_TRACK_DEC;
			break;
		case SM_PAGEINC:
			wpart = WTHEME_PART_TRACK_INC;
			break;
		default:
			wpart = WTHEME_PART_THUMB;
			break;
		}
		wstate |= WTHEME_STATE_HOVER;
	}
	else if (hover_part != WTHEME_PART_UNDEFINED)
	{
		wpart = (Widget_Part)hover_part;
		wstate |= WTHEME_STATE_HOVER;
	}

	if (!(state & CS_SELECTED))
	{
		if (hover_part != WTHEME_PART_UNDEFINED)
			wstate |= WTHEME_STATE_CONTROL_HOVER;
	}
	//copy sbinfo stuff
	themesbinfo.endvalue = endvalue;
	themesbinfo.startvalue = startvalue;
	themesbinfo.lineinc = lineinc;
	themesbinfo.thumbsize = thumbsize;
	themesbinfo.thumbpos = thumbpos;
	themesbinfo.pageinc = pageinc;
	widgetinfo.datatype = WTHEME_DATA_SCROLLBAR;
	widgetinfo.data = &themesbinfo;
	widgetinfo.part = wpart;
	widgetinfo.state = wstate;
	widgetinfo.attributes = wattributes;
}
