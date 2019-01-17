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


#include "sellst.h"
#include "util.h"
#include "font.h"
#include "date.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "cdata.h"
#include "image.h"
#include "button.h"
#include "field.h"
#include "stacklst.h"
#include "undolst.h"
#include "mcerror.h"
#include "param.h"

#include "globals.h"
#include "mctheme.h"

#include "context.h"
#include "graphics_util.h"

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCButton::draw(MCDC *dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite)
{
	MCRectangle dirty;
	dirty = p_dirty;

	if (!p_isolated)
	{
		// MW-2011-09-06: [[ Redraw ]] If rendering as a sprite, don't change opacity or ink.
		if (!p_sprite)
		{
			dc -> setopacity(blendlevel * 255 / 100);
			dc -> setfunction(ink);
		}
		
		// MW-2009-06-10: [[ Bitmap Effects ]]
		if (m_bitmap_effects == NULL)
			dc -> begin(false);
		else
		{
			if (!dc -> begin_with_effects(m_bitmap_effects, MCU_reduce_rect(rect, -gettransient())))
				return;
			dirty = dc -> getclip();
		}
	}

	MCRectangle shadowrect;
	shadowrect = rect;
	
	MCRectangle t_content_rect;
	t_content_rect = rect;
	
	uint2 loff = 0;
	if (labelwidth != 0)
	{
		shadowrect.x += labelwidth;
		shadowrect.width -= labelwidth;
	}
	if (flags & F_SHADOW)
	{
		if ((signed char)shadowoffset < 0)
		{
			shadowrect.x -= shadowoffset;
			shadowrect.y -= shadowoffset;
			shadowrect.width += shadowoffset;
			shadowrect.height += shadowoffset;
		}
		else
		{
			shadowrect.width -= shadowoffset;
			shadowrect.height -= shadowoffset;
		}
	}
	Boolean indicator = getstyleint(flags) == F_CHECK
	                    || getstyleint(flags) == F_RADIO;
	if (needfocus())
		drawfocus(dc, p_dirty);

	Boolean isstdbtn = standardbtn();
	uint2 i;
	Boolean noback = isstdbtn && !getcindex(DI_BACK, i) && !getpindex(DI_BACK,i);
	bool t_isvista = MCmajorosversion >= 0x0600 && MCcurtheme != NULL;

	bool t_themed_menu = false;
	bool t_use_alpha_layer = false;

	if (entry != NULL)
	{
		drawcombo(dc, shadowrect);
		entry->draw(dc, dirty, false, false);
	}
	else
	{
		int2 centerx =  shadowrect.x + leftmargin
		                + ((shadowrect.width - leftmargin - rightmargin) >> 1);
		int2 centery =  shadowrect.y + topmargin
		                + ((shadowrect.height - topmargin - bottommargin) >> 1);
		uint2 style = getstyleint(flags);
		Boolean macoption = IsMacEmulatedLF() && style == F_MENU
		                    && (menumode == WM_OPTION || menumode == WM_COMBO);
		Boolean white = MClook != LF_MOTIF && !macoption && !(t_isvista && style == F_MENU && menumode == WM_OPTION)
		                && ((style == F_MENU && (menumode == WM_OPTION || menumode == WM_COMBO))
		                    || (flags & F_AUTO_ARM && state & CS_ARMED
		                    && !(flags & F_SHOW_BORDER)));
		if (flags & F_3D && MClook == LF_WIN95
		        && state & CS_HILITED && (style == F_STANDARD
		                                  || (indicator && flags & F_SHOW_ICON)))
			loff = 1;

		if (style == F_MENU && menumode == WM_PULLDOWN && MCcurtheme != NULL &&
			getstack() -> hasmenubar() && getparent() -> hasname(getstack() -> getmenubar()) &&
			MCcurtheme -> drawmenuheaderbackground(dc, dirty, this))
		{
			t_themed_menu = true;
            //dc -> setforeground(getflag(F_DISABLED) ? dc -> getgray() : dc -> getblack());
            setforeground(dc, DI_PSEUDO_BUTTON_TEXT, False);
		}
		else if (menucontrol != MENUCONTROL_NONE && MCcurtheme != NULL &&
				MCcurtheme -> drawmenuitembackground(dc, dirty, this))
		{
			t_themed_menu = true;
			indicator = False;
            //dc -> setforeground(getflag(F_DISABLED) ? dc -> getgray() : dc -> getblack());
            setforeground(dc, DI_PSEUDO_BUTTON_TEXT, False);
        }
		else
		{
		if (flags & F_OPAQUE && (MCcurtheme == NULL || !noback
		                         || ((style == F_STANDARD && (MCcurtheme != NULL &&
		                                                     !MCcurtheme->iswidgetsupported(WTHEME_TYPE_PUSHBUTTON))) ||
		                             (style == F_RECTANGLE && (MCcurtheme != NULL &&
		                                                      !MCcurtheme->iswidgetsupported(WTHEME_TYPE_BEVELBUTTON))))
		                         || !(flags & F_SHOW_BORDER)
		                         || flags & F_SHADOW))
		{//define when to not draw using themes
			MCRectangle trect = shadowrect;
			if (style != F_ROUNDRECT)
			{
				if (flags & F_SHOW_BORDER && !(isstdbtn && noback))
					trect = MCU_reduce_rect(trect, borderwidth);
			}
			else
				if (flags & F_SHOW_BORDER)
					trect = MCU_reduce_rect(trect, borderwidth >> 1);
			setforeground(dc, DI_BACK,
			              (((state & CS_HILITED && flags & F_HILITE_FILL)
			               || (state & CS_ARMED && flags & F_ARM_FILL)
			               || (state & CS_ARMED && !(flags & F_SHOW_BORDER)
			               && MClook != LF_MOTIF)) && loff == 0
			              && (flags & F_SHOW_ICON || !indicator))
			              || (white && state & CS_KFOCUSED && !(state & CS_SUBMENU))
			              || (macoption && state & CS_SUBMENU),
                          (white || ((state & CS_ARMED && !(flags & F_SHOW_BORDER))
                                     && !macoption)));
			switch (style)
			{
			case F_MENU:
			case F_CHECK:
			case F_RADIO:
			case F_RECTANGLE:
			case F_STANDARD:
                    if ((isstdbtn && (state & CS_HILITED))
                        || MCcurtheme == NULL
                        || !((getstack()->ismetal() && MCcurtheme->drawmetalbackground(dc, dirty, trect, this))
                              || (style == F_MENU && menumode == WM_TOP_LEVEL && MCcurtheme->iswidgetsupported(WTHEME_TYPE_TABPANE))))

				{
					if (isstdbtn && noback)
					{
						trect = MCU_reduce_rect(trect, 1);
						if (state & CS_HILITED)
						{
                            uint2 t_index;
							if (getcindex(DI_HILITE, t_index) || getpindex(DI_HILITE, t_index))
								setforeground(dc, DI_HILITE, False, False);
							else
								dc->setforeground(dc->getgray());
						}
						dc->fillrect(trect);
					}
					else
					{
						if (style == F_MENU && menumode == WM_TOP_LEVEL)
						{
							uint2 theight = MCFontGetAscent(m_font) + 8 - 1;
							trect.y += theight;
							trect.height -= theight;
						}
						dc->fillrect(trect);
					}
					}
				break;
			case F_ROUNDRECT:
				dc->fillroundrect(trect, DEFAULT_RADIUS);
				break;
			case F_OVAL_BUTTON:
				dc->fillarc(trect, 0, 360);
				break;
			}
		}

		if (state & CS_SHOW_DEFAULT && !MCcurtheme)
			switch (MClook)
			{
			case LF_MOTIF:
				draw3d(dc, MCU_reduce_rect(shadowrect, -MOTIF_DEFAULT_WIDTH), ETCH_SUNKEN, borderwidth >> 1);
				drawborder(dc,  MCU_reduce_rect(shadowrect, -MOTIF_DEFAULT_WIDTH), borderwidth >> 1);
				break;
			case LF_MAC:
				drawmacdefault(dc, MCU_reduce_rect(shadowrect, -3));
				break;
			case LF_WIN95:
				drawborder(dc, MCU_reduce_rect(shadowrect, -WIN95_DEFAULT_WIDTH), WIN95_DEFAULT_WIDTH);
			default:
				break;
			}
		switch (getstyleint(flags))
		{
		case F_MENU:
			switch (menumode)
			{
			case WM_PULLDOWN:
                    if ((state & CS_ARMED && !(flags & F_SHOW_BORDER) &&
                         (MClook == LF_MOTIF || (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEGTK)))
				        || flags & F_SHOW_BORDER)
					drawpulldown(dc, shadowrect);
				break;
			case WM_OPTION:
				drawoption(dc, shadowrect, t_content_rect);
				break;
			case WM_COMBO:
				break;
			case WM_TOP_LEVEL:
				drawtabs(dc, shadowrect);
				break;
			}
			if (menumode != WM_CASCADE || !(flags & F_AUTO_ARM))
				break;
		case F_CHECK:
		case F_RADIO:
		case F_RECTANGLE:
		case F_STANDARD:
			if (flags & F_SHADOW)
				drawshadow(dc, rect, shadowoffset);
			if (flags & F_3D)
			{
				if ((state & CS_HILITED && flags & F_HILITE_BORDER)
				        || (flags & F_AUTO_ARM && state & CS_ARMED && flags & F_ARM_BORDER
				        && !indicator && MClook == LF_MOTIF))
					if (isstdbtn && noback)
						drawstandardbutton(dc, shadowrect);
					else
						if (MClook != LF_MOTIF && state & CS_SHOW_DEFAULT
						        && borderwidth == DEFAULT_BORDER)
						{
							setforeground(dc, DI_BOTTOM, False);
							dc->drawrect(shadowrect);
						}
						else
							draw3d(dc, shadowrect, flags & F_SHOW_BORDER ? ETCH_SUNKEN_BUTTON
							       : ETCH_RAISED, borderwidth);
				else
					if (flags & F_SHOW_BORDER || (MClook == LF_MOTIF
					        && (state & (CS_ARMED | CS_KFOCUSED)) && flags & F_ARM_BORDER))
					{
						if (isstdbtn && noback)
							drawstandardbutton(dc, shadowrect);
						else
						{
							uint2 bwidth = borderwidth;
							draw3d(dc, shadowrect, shadowrect.height <= bwidth
							       || (state & CS_HILITED && flags & F_HILITE_BORDER)
							       ? ETCH_SUNKEN_BUTTON : ETCH_RAISED, bwidth);
						}
					}
					else if (!(flags & F_SHOW_BORDER) && MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEGTK
                             && (!getcindex(DI_BACK, i) && !getpindex(DI_BACK,i))
					         && flags & MENU_ITEM_FLAGS && flags & F_AUTO_ARM)
					{
						// FG-2014-07-30: [[ Bugfix 9405 ]]
						// Clear the previously drawn highlight before drawing GTK highlight
						setforeground(dc, DI_BACK, False, False);
						dc->fillrect(shadowrect);

                        if (state & CS_ARMED)
                        {
                            MCWidgetInfo winfo;
                            winfo.type = WTHEME_TYPE_MENUITEMHIGHLIGHT;
                            getwidgetthemeinfo(winfo);
                            MCcurtheme->drawwidget(dc, winfo, shadowrect);
                        }
					}

			}
			else
				if ((state & CS_HILITED && flags & F_HILITE_BORDER)
				        || flags & F_SHOW_BORDER
				        || ((state & (CS_ARMED | CS_KFOCUSED)) && flags & F_ARM_BORDER))
					drawborder(dc, shadowrect, borderwidth);
			break;
		case F_ROUNDRECT:
			if (borderwidth != 0)
			{
				// MM-2014-04-08: [[ Bug 11662/11398 ]] Let the context take care of insetting the rectangle.
				//  This solves 2 bugs. Round rect buttons are drawn at the wrong size (compared to square buttons) (11662)
				//  and the corners of round rect buttons are inconsistent (11398).
				dc->setlineatts(borderwidth - 1, LineSolid, CapButt, JoinBevel);
				setforeground(dc, DI_FORE, False);
				dc->drawroundrect(shadowrect, DEFAULT_RADIUS, true);
				dc->setlineatts(0, LineSolid, CapButt, JoinBevel);
			}
			break;
		case F_OVAL_BUTTON:
			if ((state & CS_HILITED && flags & F_HILITE_BORDER)
			        || flags & F_SHOW_BORDER
			        || ((state & (CS_ARMED | CS_KFOCUSED)) && flags & F_ARM_BORDER))
			{
				setforeground(dc, DI_FORE, False);
				dc->drawarc(shadowrect, 0, 360);
			}
			break;
		}

		if (flags & F_DISABLED)
        {
			if (MClook == LF_MOTIF)
			{
				setforeground(dc, DI_FORE, False);
				dc->setopacity(127);
				dc->begin(false);
				t_use_alpha_layer = true;
			}
			else if (IsMacLF())
			{
				dc->setforeground(dc->getgray());
				dc->setfillstyle(FillSolid, nil, 0, 0);
			}
			else
				setforeground(dc, DI_TOP, False);
        }
		else
        {
			if ((white && state & CS_KFOCUSED && !(state & CS_SUBMENU)) ||
				(isstdbtn && noback && (MCcurtheme == NULL || (MCcurtheme->getthemeid() != LF_NATIVEWIN && MCcurtheme->getthemeid() != LF_NATIVEGTK)) && state & CS_HILITED && !MCaqua) ||
				(MClook != LF_MOTIF && style == F_MENU && flags & F_OPAQUE && state & CS_ARMED && !(flags & F_SHOW_BORDER)))
				setforeground(dc, DI_PSEUDO_BUTTON_TEXT_SEL, False, True);
			else
				setforeground(dc, DI_FORE, ((state & CS_HILITED && flags & F_HILITE_FILL)
				                            || ((state & CS_ARMED && flags & F_ARM_FILL) && flags & F_OPAQUE && ((MClook != LF_WIN95 && !MCaqua)))
				                            || style != F_STANDARD), False);
        }
		}

		// MW-2009-06-14: We will assume (perhaps unwisely) that is 'opaque' is set
		//   then the background is now, completely opaque.
		bool t_was_opaque = false;
		if (getflag(F_OPAQUE))
			t_was_opaque = dc -> changeopaque(true);

		MCStringRef t_label = getlabeltext();
		Boolean icondrawed = False;
		
        // SN-2014-08-12: [[ Bug 13155 ]] Don't try to draw the icons if the button has not got any
        // SN-2014-12-17: [[ Bug 14249 ]] Do not try to draw the curicon if there is no current
        //  icon (as it may after loading a stack).
		if (icons != NULL && icons -> curicon && m_icon_gravity != kMCGravityNone)
		{
			// MW-2014-06-19: [[ IconGravity ]] Use iconGravity to place the icon.
			int t_left, t_top, t_right, t_bottom;
			t_left = rect . x + leftmargin + borderwidth;
			t_top = rect . y + topmargin + borderwidth;
			t_right = rect . x + rect . width - rightmargin - borderwidth;
			t_bottom = rect . y + rect . height - bottommargin - borderwidth;
			
			MCRectangle t_rect;
			if (t_left < t_right)
				t_rect . x = t_left, t_rect . width = t_right - t_left;
			else
				t_rect . x = (t_left + t_right) / 2, t_rect . width = 0;
			if (t_top < t_bottom)
				t_rect . y = t_top, t_rect . height = t_bottom - t_top;
			else
				t_rect . y = (t_top + t_bottom) / 2, t_rect . height = 0;
			icons -> curicon -> drawwithgravity(dc, t_rect, m_icon_gravity);
			
			icondrawed = True;
		}
		
		if (flags & F_SHOW_NAME && !MCStringIsEmpty(t_label) && menucontrol != MENUCONTROL_SEPARATOR)
		{
			// Split the string on the newlines
			MCAutoArrayRef lines;
			/* UNCHECKED */ MCStringSplit(t_label, MCSTR("\n"), nil, kMCCompareExact, &lines);
			uindex_t nlines = MCArrayGetCount(*lines);
			
            coord_t fascent, fdescent, fleading, fxheight;
            fascent = MCFontGetAscent(m_font);
            fdescent = MCFontGetDescent(m_font);
            fleading = MCFontGetLeading(m_font);
            fxheight = MCFontGetXHeight(m_font);

			coord_t fheight;
            fheight = fascent + fdescent + fleading;

            coord_t sx, sy, theight;
            if (nlines == 1)
            {
                // Centre things on the middle of the ascent
                sx = shadowrect.x + leftmargin + borderwidth - DEFAULT_BORDER;
                sy = roundf(centery + (fascent-fdescent)/2);
                theight = fascent;
            }
            else
            {
                // Centre things by centring the bounding box of the text
                sx = shadowrect.x + leftmargin + borderwidth - DEFAULT_BORDER;
                sy = centery - (nlines * fheight / 2) + fleading/2 + fascent;
                theight = nlines * fheight;
            }
            
            coord_t starty = sy;
            uint2 t_line;
            coord_t twidth = 0;
            
            dc->save();
            dc->cliprect(t_content_rect);

            // MW-2014-06-19: [[ IconGravity ]] Use old method of calculating icon location if gravity is none.
			if (flags & F_SHOW_ICON && icons != NULL && icons->curicon != NULL && m_icon_gravity == kMCGravityNone)
			{
				switch (flags & F_ALIGNMENT)
				{
				case F_ALIGN_LEFT:
				case F_ALIGN_JUSTIFY:
					centerx = shadowrect.x + shadowrect.width - rightmargin
					          - (icons->curicon->getrect().width >> 1);
					break;
				case F_ALIGN_CENTER:
                        centery -= (theight / 2) + 1;
                        sy = shadowrect.y + shadowrect.height - bottommargin - theight
                            + fascent - fdescent - 1;
					break;
				case F_ALIGN_RIGHT:
					centerx = shadowrect.x + leftmargin
					          + (icons->curicon->getrect().width >> 1);
					break;
				}
			}
			if (flags & F_SHOW_ICON && icons != NULL && icons->curicon != NULL && m_icon_gravity == kMCGravityNone)
			{
				icons->curicon->drawcentered(dc, centerx + loff, centery + loff, (state & CS_HILITED) != 0);
				icondrawed = True;
			}
			
			uindex_t t_totallen = 0;
			for (t_line = 0 ; t_line < nlines ; t_line++)
			{
				// Note: 'lines' is an array of strings
				MCValueRef lineval = nil;
				/* UNCHECKED */ MCArrayFetchValueAtIndex(*lines, t_line + 1, lineval);
				MCStringRef line = (MCStringRef)(lineval);
                // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
                twidth = MCFontMeasureTextFloat(m_font, line, getstack() -> getdevicetransform());
				
				// Mnemonic position calculation
				uindex_t t_mnemonic = 0;
				uindex_t t_linelen = MCStringGetLength(line);
				if (mnemonic > t_totallen && mnemonic <= t_totallen + t_linelen)
					t_mnemonic = mnemonic - t_totallen;
				
				switch (flags & F_ALIGNMENT)
				{
				case F_ALIGN_LEFT:
				case F_ALIGN_JUSTIFY:
					if (t_line == 0)
					{
						if (indicator && !(flags & F_SHOW_ICON))
						{
							sx += GetCheckSize() + leftmargin;
							if (MClook == LF_WIN95)
							{
								sy++;
								sx--;
							}
						}
						if (MCaqua && IsMacLFAM()
                            && ((style == F_STANDARD && noback)|| (style == F_MENU
                                                                   && menumode == WM_OPTION)))
							sx += 5;
					}
					break;
				case F_ALIGN_CENTER:
					sx = floorf(centerx - (twidth / 2));
					if (menumode == WM_OPTION || menumode == WM_COMBO)
						sx -= optionrect.width;
					// MH-2007-03-16 [[ Bug 3598 ]] Centering of the label did not work correctly on mac os classic and non vista windows option buttons.
					if (menumode == WM_OPTION && ((MClook == LF_WIN95 && !t_themed_menu) || MClook == LF_MAC))
						sx = t_content_rect . x + (t_content_rect . width - leftmargin) / 2 - twidth / 2;
					if (menumode == WM_CASCADE)
						sx -= rect.height >> 1;
					break;
				case F_ALIGN_RIGHT:
					sx = shadowrect.x + shadowrect.width - rightmargin - twidth
					     - (borderwidth - DEFAULT_BORDER);
					if (menumode == WM_OPTION)
						sx -= optionrect.width << 1;
					if (menumode == WM_CASCADE)
						sx -= rect.height;
					break;
				}
				uint2 fontstyle;
				fontstyle = gettextstyle();
				if ((flags & F_DISABLED) != 0 && !t_themed_menu && MClook == LF_WIN95)
				{
					drawlabel(dc, sx + 1 + loff, sy + 1 + loff, twidth, shadowrect, line, fontstyle, t_mnemonic);
					if (getstyleint(flags) == F_MENU && menumode == WM_CASCADE)
					{
						shadowrect.x++;
						shadowrect.y++;
						drawcascade(dc, shadowrect); // draw arrow in text color
						shadowrect.x--;
						shadowrect.y--;
					}
					setforeground(dc, DI_BOTTOM, False);
				}
#ifdef _MACOSX
                // FG-2014-10-29: [[ Bugfix 13842 ]] On Yosemite, glowing buttons
                // should draw with white text.
                if (IsMacLFAM() && MCmajorosversion >= 0x10A0 && MCaqua
                    && !(flags & F_DISABLED) && isstdbtn && getstyleint(flags) == F_STANDARD
                    && ((state & CS_HILITED) || (state & CS_SHOW_DEFAULT))
                    && rect.height <= 24 && MCappisactive)
                    setforeground(dc, DI_BACK, False, True);
                // PM-2014-11-26: [[ Bug 14070 ]] [Removed code] Make sure text color in menuButton inverts when hilited
        
#endif
				drawlabel(dc, sx + loff, sy + loff, twidth, shadowrect, line, fontstyle, t_mnemonic);

				if (getstyleint(flags) == F_MENU && menumode == WM_CASCADE && !t_themed_menu)
					drawcascade(dc, shadowrect); // draw arrow in text color
				if ((flags & F_DISABLED && MClook == LF_WIN95) || t_themed_menu)
					setforeground(dc, DI_TOP, False);
				sy += fheight;
				
				t_totallen += t_linelen;
			}

			dc->restore();

			if (labelwidth != 0 && !isunnamed())
			{
				MCStringRef t_name = MCNameGetString(getname());
                drawdirectionaltext(dc, rect.x + leftmargin, starty, t_name, m_font);
			}
			
			// MW-2012-01-27: [[ Bug 9432 ]] Native GTK handles focus borders itself
			//   so don't render the win95-style one.
			if (MClook == LF_WIN95 && !IsNativeGTK() && state & CS_KFOCUSED && !(flags & F_AUTO_ARM) && !white)
			{
				MCRectangle trect;
				if (indicator && nlines == 1)
					MCU_set_rect(trect, sx - 1, starty - fascent, twidth + 2, fascent + fdescent);
				else
					trect = MCU_reduce_rect(shadowrect, 3);

#if defined(X11) || defined(_WINDOWS)
				dc->setforeground(dc->getblack());
				dc->setfillstyle(FillSolid, nil, 0, 0);
				dc->setlineatts(1, LineOnOffDash, CapButt, JoinBevel);				
				dc->setdashes(0, dotlist, 2);
				dc->setfunction(GXinvert);

				// MM-2013-11-05: [[ Bug 11355 ]] - Handle the insetting of the dashed border manually - was causing drawing artefacts previously.
				trect . width -= 1; trect . height -= 1;
				dc->drawrect(trect, false);

				dc->setfunction(GXcopy);
				dc->setlineatts(0, LineSolid, CapButt, JoinBevel);
#else
				// Windows 95 and Mac don't support user line styles, so kludge away
				setforeground(dc, DI_BACK, False);
				dc->setbackground(dc->getblack());
				dc->setfillstyle(FillOpaqueStippled, nil, 0, 0);
				MCRectangle xrect = trect;
				xrect.height = 1;
				dc->fillrect(xrect);
				xrect.y = trect.y + trect.height - 1;
				dc->fillrect(xrect);
				xrect = trect;
				xrect.width = 1;
				dc->fillrect(xrect);
				xrect.x = trect.x + trect.width - 1;
				dc->fillrect(xrect);
				dc->setbackground(MCzerocolor);
				dc->setfillstyle(FillSolid, nil, 0 , 0);
#endif

			}
		}
        
        if (t_use_alpha_layer)
        {
            dc->end();
        }
        
		if (flags & F_DISABLED && MClook == LF_MOTIF)
			dc->setfillstyle(FillSolid, nil, 0 , 0);
		if (indicator && !(flags & F_SHOW_ICON)
		        && (state & CS_HILITED || !(flags & F_SHOW_HILITE)))
		{
			if (getstyleint(flags) == F_CHECK)
				drawcheck(dc, shadowrect, white);
			if (getstyleint(flags) == F_RADIO)
				drawradio(dc, shadowrect, white);
		}

		if (!icondrawed &&flags & F_SHOW_ICON && icons != NULL && icons->curicon != NULL)
			icons->curicon->drawcentered(dc, centerx + loff, centery + loff,
			                             (state & CS_HILITED) != 0);
	
		// MW-2009-06-14: Reset opaqueness
		if (getflag(F_OPAQUE))
			dc -> changeopaque(t_was_opaque);
	}

	if (!p_isolated)
    {
		dc -> end();
    }
}

void MCButton::drawlabel(MCDC *dc, int2 sx, int sy, uint2 twidth, const MCRectangle &srect, MCStringRef p_label, uint2 fstyle, uindex_t p_mnemonic)
{
	if (getstyleint(flags) == F_MENU && menumode == WM_OPTION
	        && MClook != LF_WIN95)
		sx += 2;
	/*if (MCaqua && IsMacLFAM()
	        && (getstyleint(flags) == F_STANDARD  || getstyleint(flags) == F_MENU
	            && menumode == WM_OPTION))
		sy--;*/

    drawdirectionaltext(dc, sx, sy, p_label, m_font);
	
	if (!MCStringIsEmpty(acceltext))
	{
        // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
        uint2 awidth = MCFontMeasureText(m_font, acceltext, getstack() -> getdevicetransform());
		if (rightmargin == defaultmargin || menucontrol == MENUCONTROL_ITEM)
            drawdirectionaltext(dc, srect.x + srect.width - rightmargin - awidth, sy, acceltext, m_font);
		else
            drawdirectionaltext(dc, srect.x + srect.width - rightmargin, sy, acceltext, m_font);
	}

	if (fstyle & FA_UNDERLINE)
		dc->drawline(sx, sy + 1, sx + twidth, sy + 1);
	if (fstyle & FA_STRIKEOUT)
	{
		int32_t fascent;
		fascent = MCFontGetAscent(m_font);
		dc->drawline(sx, sy - (fascent >> 1), sx + twidth, sy - (fascent >> 1));
	}
	if (!IsMacLF() && mnemonic
        && (gettheme() == kMCInterfaceThemeLegacy || (MCscreen->querymods() & MS_ALT)))
	{
		if (p_mnemonic > 0)
		{
			MCRange t_before = MCRangeMake(0, mnemonic - 1);
			MCRange t_letter = MCRangeMake(mnemonic - 1, 1);
            
			// MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
            int32_t mstart = sx + MCFontMeasureTextSubstring(m_font, p_label, t_before, getstack() -> getdevicetransform());
            int32_t mwidth = MCFontMeasureTextSubstring(m_font, p_label, t_letter, getstack() -> getdevicetransform());

#ifdef TARGET_PLATFORM_WINDOWS
			// No idea why this fudge-factor is required on Windows...
			// Without it, the mnemonic underlines are drawn too far
			// to the left, mis-aligning them with the mnemonic letter.
			mstart -= 1;
#endif

			sy += mnemonicoffset;
			dc->drawline(mstart, sy, mstart + mwidth - 1, sy);
		}
	}
}

void MCButton::drawcheck(MCDC *dc, MCRectangle &srect, Boolean white)
{
	MCRectangle trect;
	trect.x = srect.x + leftmargin - 2;
	trect.y = srect.y + ((srect.height - CHECK_SIZE) >> 1) + 1;
	trect.width = trect.height = CHECK_SIZE;
	uint2 index;
	if (!(flags & F_AUTO_ARM) && MCcurtheme &&
	        MCcurtheme->iswidgetsupported(WTHEME_TYPE_CHECKBOX))
	{
        if (IsNativeGTK())
        {
            int32_t t_size, t_spacing;
            t_size = MCcurtheme -> getmetric(WTHEME_METRIC_CHECKBUTTON_INDICATORSIZE);
            t_spacing = MCcurtheme -> getmetric(WTHEME_METRIC_CHECKBUTTON_INDICATORSPACING);
            trect . x = srect . x + leftmargin - t_spacing;
            trect . y = srect . y;
            trect . width = t_size + 2*t_spacing;
            trect . height = srect . height;
        }
        
		MCWidgetInfo widgetinfo;
		widgetinfo.type = WTHEME_TYPE_CHECKBOX;
		getwidgetthemeinfo(widgetinfo);
		MCcurtheme->drawwidget(dc, widgetinfo, IsMacLFAM()?rect:trect);
		return;
	}
	switch (MClook)
	{
	case LF_MAC:
		if (!(flags & F_AUTO_ARM))
		{
			trect.width--;
			trect.height--;
			trect = MCU_reduce_rect(trect, 1);
			if (flags & F_DISABLED)
			{
				dc->setfillstyle(FillSolid, nil, 0, 0);
				dc->setforeground(dc->getgray());
				dc->setfillstyle(FillSolid, nil, 0, 0);
				dc->drawrect(trect);
				break;
			}
			if (state & CS_MFOCUSED && !(state & CS_SELECTED)
			        && MCU_point_in_rect(rect, mx, my))
			{
				dc->setforeground(maccolors[MAC_SHADOW]);
				dc->setfillstyle(FillSolid, nil, 0, 0);
				dc->fillrect(trect);
				draw3d(dc, trect, ETCH_SUNKEN, 1);
			}
			else
			{
				setforeground(dc, DI_BACK, False);
				dc->fillrect(trect);
				draw3d(dc, trect, ETCH_RAISED, 1);
			}
			if (state & CS_HILITED)
			{
				MCPoint p[6];
				p[0].x = trect.x + 1;
				p[1].x = p[4].x = trect.x + 4;
				p[2].x = trect.x + 11;
				p[3].x = trect.x + 10;
				p[5].x = trect.x + 2;
				p[0].y = p[5].y = trect.y + 4;
				p[1].y = trect.y + 7;
				p[2].y = p[3].y = trect.y;
				p[4].y = trect.y + 6;
				dc->setforeground(dc->getblack());
				dc->setfillstyle(FillSolid, nil, 0, 0);
				dc->drawlines(p, 6);
				p[0].x = trect.x + 2;
				p[1].x = p[0].x + 2;
				p[2].x = p[0].x + 3;
				p[3].x = p[0].x + 10;
				p[0].y = trect.y + 6;
				p[1].y = p[2].y = p[0].y + 2;
				p[3].y = trect.y + 1;
				setforeground(dc, DI_SHADOW, False);
				dc->drawlines(p, 4);
				p[0] = p[1];
				p[1] = p[3];
				p[1].x--;
				dc->setforeground(maccolors[MAC_SHADOW]);
				dc->setfillstyle(FillSolid, nil, 0, 0);
				dc->drawlines(p, 2);
			}
			trect = MCU_reduce_rect(trect, -1);
			dc->setforeground(dc->getblack());
			dc->setfillstyle(FillSolid, nil, 0, 0);
			dc->drawrect(trect);
			break;
		}
		// fall through
	case LF_WIN95:
		if (flags & F_AUTO_ARM)
			trect = MCU_reduce_rect(trect, 2);
		else
		{
			draw3d(dc, trect, ETCH_SUNKEN, DEFAULT_BORDER);
			trect = MCU_reduce_rect(trect, 2);
			if (getcindex(DI_BACK, index) || getpindex(DI_BACK, index))
				setforeground(dc, DI_BACK, False);
			else
			{
				if (state & CS_MFOCUSED && !(state & CS_SELECTED)
				        && MCU_point_in_rect(rect, mx, my))
					dc->setforeground(dc->getgray());
				else
					dc->setforeground(dc->getwhite());
				dc->setfillstyle(FillSolid, nil, 0, 0);
			}
			if (!(flags & F_DISABLED))
				dc->fillrect(trect);
		}
		if (state & CS_HILITED)
		{
			// MM-2013-12-16: [[ Bug 11558 ]] Convert checkmark drawing to use polygons rather than lines.
			MCPoint p[6];
			p[0].x = p[5].x = trect.x + 1;
			p[1].x = p[4].x = p[0].x + 2;
			p[2].x = p[3].x = p[1].x + 5;
			p[0].y = trect.y + 3;
			p[1].y = p[0].y + 2;
			p[2].y = p[1].y - 5;
			p[3].y = p[2].y + 3;
			p[4].y = p[1].y + 3;
			p[5].y = p[0].y + 3;

			if (white && state & CS_ARMED)
				dc->setforeground(dc->getwhite());
			else
				dc->setforeground(dc->getblack());
			dc->setfillstyle(FillSolid, nil, 0, 0);
			dc->fillpolygon(p, 6);
		}
		break;
	case LF_MOTIF:
		trect.x += 2;
		trect.y--;
		if (flags & F_3D)
			draw3d(dc, trect, state & CS_HILITED ? ETCH_SUNKEN : ETCH_RAISED,
			       DEFAULT_BORDER);
		else
			drawborder(dc, trect, DEFAULT_BORDER);
		trect = MCU_reduce_rect(trect, 2);
		setforeground(dc, DI_BACK, (state & CS_HILITED) != 0);
		dc->fillrect(trect);
		break;
	}
}

void MCButton::drawradio(MCDC *dc, MCRectangle &srect, Boolean white)
{
	MCPoint p[12];
	MCRectangle trect;

	int2 lx = srect.x + leftmargin - 1;
	int2 cy = srect.y + (srect.height >> 1);
	int2 ty = cy - (CHECK_SIZE >> 1);
	uint2 index;
	if (!(flags & F_AUTO_ARM) && MCcurtheme &&
	        MCcurtheme->iswidgetsupported(WTHEME_TYPE_RADIOBUTTON))
	{
		if (!IsNativeGTK())
		{
			trect.x = srect.x + leftmargin - 2;
			trect.y = srect.y + ((srect.height - CHECK_SIZE) >> 1) + 1;
			trect.width = trect.height = CHECK_SIZE;
		}
		else
		{
			int32_t t_size, t_spacing;
			t_size = MCcurtheme -> getmetric(WTHEME_METRIC_RADIOBUTTON_INDICATORSIZE);
			t_spacing = MCcurtheme -> getmetric(WTHEME_METRIC_RADIOBUTTON_INDICATORSPACING);
			trect . x = srect . x + leftmargin - t_spacing;
			trect . y = srect . y;
			trect . width = t_size + 2*t_spacing;
			trect . height = srect . height;
		}

		MCWidgetInfo widgetinfo;
		widgetinfo.type = WTHEME_TYPE_RADIOBUTTON;
		getwidgetthemeinfo(widgetinfo);
		MCcurtheme->drawwidget(dc, widgetinfo, IsMacLFAM()?rect:trect);
		return;
	}
	switch (MClook)
	{
	case LF_MAC:
		if (!(flags & F_AUTO_ARM))
		{
			if (flags & F_DISABLED)
			{
				dc->setforeground(dc->getgray());
				dc->setfillstyle(FillSolid, nil, 0, 0);
			}
			else
			{
				lx += 5;
				if (state & CS_MFOCUSED && !(state & CS_SELECTED)
				        && MCU_point_in_rect(rect, mx, my))
					if (state & CS_HILITED)
						macrbhilitetrack->drawcentered(dc, lx, cy, False);
					else
						macrbtrack->drawcentered(dc, lx, cy, False);
				else
					if (state & CS_HILITED)
						macrbhilite->drawcentered(dc, lx, cy, False);
					else
						macrb->drawcentered(dc, lx, cy, False);
				break;
			}
		}
		// fall through
	case LF_WIN95:
		if (!(flags & F_AUTO_ARM))
		{
			p[0].x = p[1].x = p[4].x = p[5].x = lx + 1;
			p[2].x = p[3].x = lx;
			p[6].x = lx + 2;
			p[7].x = lx + 3;
			p[8].x = lx + 4;
			p[9].x = lx + 7;
			p[10].x = lx + 8;
			p[11].x = lx + 9;

			p[0].y = ty + 9;
			p[1].y = ty + 8;
			p[2].y = ty + 7;
			p[3].y = ty + 4;
			p[4].y = ty + 3;
			p[5].y = ty + 2;
			p[6].y = p[7].y = p[10].y = p[11].y = ty + 1;
			p[8].y = p[9].y = ty;
			if (!IsMacEmulatedLF() || !(flags & F_DISABLED))
				setforeground(dc, DI_BOTTOM, False);
			dc->drawlines(p, 12);
			p[0].x = lx + 2;
			p[1].x = lx + 3;
			p[2].x = lx + 4;
			p[3].x = lx + 7;
			p[4].x = lx + 8;
			p[5].x = lx + 9;
			p[6].x = p[7].x = p[10].x = p[11].x = lx + 10;
			p[8].x = p[9].x = lx + 11;
			p[0].y = p[1].y = p[4].y = p[5].y = ty + 10;
			p[2].y = p[3].y = ty + 11;
			p[6].y = ty + 9;
			p[7].y = ty + 8;
			p[8].y = ty + 7;
			p[9].y = ty + 4;
			p[10].y = ty + 3;
			p[11].y = ty + 2;
			if (!IsMacEmulatedLF() || !(flags & F_DISABLED))
				setforeground(dc, DI_TOP, False);
			dc->drawlines(p, 12);
			if (IsMacEmulatedLF() && flags & F_DISABLED)
				break;
			p[0].x = p[3].x = p[4].x = lx + 2;
			p[1].x = p[2].x = lx + 1;
			p[5].x = lx + 3;
			p[6].x = lx + 4;
			p[7].x = lx + 7;
			p[8].x = lx + 8;
			p[9].x = lx + 9;
			p[0].y = ty + 8;
			p[1].y = ty + 7;
			p[2].y = ty + 4;

			p[3].y = ty + 3;
			p[4].y = p[5].y = p[8].y = p[9].y = ty + 2;
			p[6].y = p[7].y = ty + 1;
			dc->setforeground(dc->getblack());
			dc->setfillstyle(FillSolid, nil, 0, 0);
			dc->drawlines(p, 10);
			p[0].x = p[1].x = lx + 2;
			p[2].x = p[7].x = lx + 4;
			p[3].x = p[6].x = lx + 7;
			p[4].x = p[5].x = lx + 9;
			p[0].y = p[5].y = ty + 7;
			p[1].y = p[4].y = ty + 4;
			p[2].y = p[3].y = ty + 2;
			p[6].y = p[7].y = ty + 9;
			p[8] = p[0];

			if (getcindex(DI_BACK, index) || getpindex(DI_BACK, index))
				setforeground(dc, DI_BACK, False);
			else
			{
				if (state & CS_MFOCUSED && !(state & CS_SELECTED)

				        && MCU_point_in_rect(rect, mx, my))
					dc->setforeground(dc->getgray());
				else
					dc->setforeground(dc->getwhite());
				dc->setfillstyle(FillSolid, nil, 0, 0);
			}
			if (!(flags & F_DISABLED))
			{
				dc->fillpolygon(p, 9);
				dc->drawlines(p, 9);
			}
		}

		if (state & CS_HILITED)
		{
			if (getcindex(DI_HILITE, index) || getpindex(DI_HILITE, index))
				setforeground(dc, DI_HILITE, False);
			else
				if (flags & F_DISABLED)
					setforeground(dc, DI_BOTTOM, False);
				else
				{
					if (white && state & CS_ARMED)
						dc->setforeground(dc->getwhite());
					else
						dc->setforeground(dc->getblack());
					dc->setfillstyle(FillSolid, nil, 0, 0);
				}
			trect.x = lx + 4;
			trect.y = ty + 5;
			trect.width = 4;
			trect.height = 2;
			dc->fillrect(trect);
			trect.x = lx + 5;
			trect.y = ty + 4;
			trect.width = 2;
			trect.height = 4;
			dc->fillrect(trect);
		}
		break;
	case LF_MOTIF:
		// IM-2013-09-09: [[ RefactorGraphics ]] draw with polygons to improve scaled appearance
		p[0].x = ++lx;
		p[0].y = --cy;
		p[1].x = lx + (CHECK_SIZE >> 1);
		p[1].y = ty;
		p[2].x = lx + CHECK_SIZE - 1;
		p[2].y = cy;
		p[3].x = lx + (CHECK_SIZE >> 1);
		p[3].y = cy + (CHECK_SIZE >> 1);
		//p[4].x = lx + 1;
		//p[4].y = cy + 1;
		setforeground(dc, DI_BACK, (state & CS_HILITED) != 0);
		//dc->fillpolygon(p, 5);
		dc->fillpolygon(p, 4);

		MCPoint t_top_points[6];
		t_top_points[0] = p[0];
		t_top_points[1] = p[1];
		t_top_points[2] = p[2];
		t_top_points[3] = p[2];
		t_top_points[3].x -= 3;
		t_top_points[4] = p[1];
		t_top_points[4].y += 3;
		t_top_points[5] = p[0];
		t_top_points[5].x += 3;

		MCPoint t_bottom_points[6];
		t_bottom_points[0] = p[2];
		t_bottom_points[1] = p[3];
		t_bottom_points[2] = p[0];
		t_bottom_points[3] = p[0];
		t_bottom_points[3].x += 3;
		t_bottom_points[4] = p[3];
		t_bottom_points[4].y -= 3;
		t_bottom_points[5] = p[2];
		t_bottom_points[5].x -= 3;
		if (flags & F_3D)
		{
			setforeground(dc, DI_TOP, (state & CS_HILITED) != 0);
			dc->fillpolygon(t_top_points, 6);
			setforeground(dc, DI_BOTTOM, (state & CS_HILITED) != 0);
			dc->fillpolygon(t_bottom_points, 6);
		}
		else
		{
			setforeground(dc, DI_BORDER, False);
			dc->fillpolygon(t_top_points, 6);
			dc->fillpolygon(t_bottom_points, 6);
		}
		break;
	}
}

void MCButton::drawpulldown(MCDC *dc, MCRectangle &srect)
{
#if defined(TARGET_PLATFORM_LINUX)
	if (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEGTK &&
	        state & CS_ARMED && !(flags & F_SHOW_BORDER))
	{
		// FG-2014-07-30: [[ Bugfix 9405 ]]
		// Clear the previously drawn highlight before drawing GTK highlight
		setforeground(dc, DI_BACK, False, False);
		dc->fillrect(srect);

		MCWidgetInfo winfo;
		winfo.type = WTHEME_TYPE_MENUITEMHIGHLIGHT;
		getwidgetthemeinfo(winfo);
		MCcurtheme->drawwidget(dc, winfo, srect);
	}
	else
#endif
		if (borderwidth)
		{
			if (MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_PULLDOWN))
			{
				MCWidgetInfo widgetinfo;
				widgetinfo.type = WTHEME_TYPE_PULLDOWN;
				getwidgetthemeinfo(widgetinfo);
#ifdef _MACOSX
				uint2 i;
				if (!getcindex(DI_BACK, i) && !getpindex(DI_BACK, i))
					MCcurtheme->drawwidget(dc, widgetinfo, srect);
				else
					draw3d(dc, srect, ETCH_RAISED, borderwidth);
#else
				MCcurtheme->drawwidget(dc, widgetinfo, srect);
#endif

			}
			else
				draw3d(dc, srect, ETCH_RAISED, borderwidth);
		}
}

// MH-2007-03-16: [[ Bug 3598 ]] Added r_content_rect parameter to return the content rect of the option button.
void MCButton::drawoption(MCDC *dc, MCRectangle &srect, MCRectangle& r_content_rect)
{
	MCRectangle trect;
	r_content_rect = rect;
	if (MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_OPTIONBUTTON))
	{
		MCWidgetInfo widgetinfo;
		widgetinfo.type = WTHEME_TYPE_OPTIONBUTTON;
		getwidgetthemeinfo(widgetinfo);
		
		r_content_rect . width -= MCcurtheme -> getmetric(WTHEME_METRIC_OPTIONBUTTONARROWSIZE);

		MCcurtheme->drawwidget(dc, widgetinfo, rect);
		return;
	}
	switch (MClook)
	{
	case LF_MOTIF:
		optionrect.y = srect.y + ((srect.height - optionrect.height) >> 1);
		optionrect.x = srect.x + srect.width - optionrect.width
		               - (optionrect.width >> 1) - (borderwidth - DEFAULT_BORDER);
		if (flags & F_3D)
		{

			draw3d(dc, optionrect, ETCH_RAISED, DEFAULT_BORDER);
			draw3d(dc, srect, ETCH_RAISED, borderwidth);
		}
		else
		{
			drawborder(dc, optionrect, DEFAULT_BORDER);
			drawborder(dc, srect, borderwidth);
		}
		r_content_rect . width -= (25 + borderwidth);
		break;
	case LF_MAC:
		trect = MCU_reduce_rect(srect, 1);
		trect.width -= trect.height;
		if (!(flags & F_DISABLED))
			draw3d(dc, trect, state & CS_SUBMENU ? ETCH_SUNKEN : ETCH_RAISED, 1);
		drawmacborder(dc, srect);
		trect.x += trect.width;
		trect = MCU_reduce_rect(trect, 1);
		trect.width = trect.height;
		drawmacpopup(dc, trect);
		
		r_content_rect . width -= rect . height;
		break;
	case LF_WIN95:
		drawcombo(dc, srect);
		
		// MW-2008-02-27: [[ Bug 5854 ]] Adjust for border appropriately to stop text
		//   overlapping left hand side.
		r_content_rect . width -= rect . height + 4;
		r_content_rect . x += 4;
		break;
	}
}

void MCButton::drawcascade(MCDC *dc, MCRectangle &srect)
{
	MCPoint arrow[3];
	switch (MClook)
	{
	case LF_MOTIF:
		{
			arrow[0].x = srect.x + srect.width - (srect.height >> 2) - 1;
			arrow[1].x = arrow[2].x = arrow[0].x - (srect.height >> 1) + 3;
			arrow[0].y = srect.y + (srect.height >> 1);
			arrow[1].y = arrow[0].y - (srect.height >> 2) + 2;
			arrow[2].y = arrow[0].y + (srect.height >> 2) - 2;
			Boolean hilite = state & CS_ARMED || state & CS_KFOCUSED;
			if (hilite)
			{
				setforeground(dc, DI_HILITE, False);
				dc->fillpolygon(arrow, 3);
			}
			uint1 i;
			for (i = 0 ; i < borderwidth ; i++)
			{
				setforeground(dc, DI_TOP, hilite);
				dc->drawlines(arrow, 3);
				setforeground(dc, DI_BOTTOM, hilite);
				dc->drawline(arrow[0].x, arrow[0].y, arrow[2].x, arrow[2].y);
				arrow[0].x++;
				arrow[1].y--;
				arrow[1].x--;
				arrow[2].y++;
				arrow[2].x--;
			}
		}
		break;
	default:
		arrow[0].x = srect.x + srect.width - 9;
		arrow[1].x = arrow[2].x = arrow[0].x - 4;
		arrow[0].y = srect.y + (srect.height >> 1);
		arrow[1].y = arrow[0].y + 4;
		arrow[2].y = arrow[0].y - 4;
		dc->fillpolygon(arrow, 3);
		break;
	}
	if (flags & F_SHOW_BORDER && !IsMacLFAM())
		draw3d(dc, srect, ETCH_RAISED, borderwidth);
}

void MCButton::drawcombo(MCDC *dc, MCRectangle &srect)
{
	MCRectangle trect = srect;
	if (MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_COMBO))
	{
		//draw frame around combobobox
		MCWidgetInfo winfo;
		winfo.type = WTHEME_TYPE_COMBO;
		getwidgetthemeinfo(winfo);
		winfo.data = this;
		winfo.datatype = WTHEME_DATA_MCOBJECT;
		MCcurtheme->drawwidget(dc, winfo, srect);
		return;
	}
	switch (MClook)
	{
	case LF_MOTIF:
		trect.x += trect.width - trect.height;
		trect.width = trect.height;
		setforeground(dc, DI_BACK, False);
		dc->fillrect(trect);
		drawarrow(dc, srect.x + srect.width - srect.height + borderwidth,
		          srect.y + borderwidth * 2 + 1,srect.height - borderwidth * 2 - 6,
		          AD_DOWN, True, (state & CS_HILITED) != 0);
		draw3d(dc, srect, ETCH_SUNKEN, borderwidth);
		break;
	case LF_MAC:
		{
			trect.width -= trect.height + 2;
			drawborder(dc, trect, 1);
			trect.x = srect.x + srect.width - srect.height;
			trect.width = trect.height;
			setforeground(dc, DI_BACK, (state & CS_SUBMENU) != 0, False);
			dc->fillrect(trect);
			drawmacborder(dc, trect);
			trect = MCU_reduce_rect(trect, 2);
			drawmacpopup(dc, trect);
		}
		break;
	case LF_WIN95:
		drawarrow(dc, srect.x + srect.width - srect.height + borderwidth,
		          srect.y + borderwidth, srect.height - borderwidth * 2,
		          AD_DOWN, True, (state & CS_HILITED) != 0);
		draw3d(dc, srect, ETCH_SUNKEN, borderwidth);
		break;
	}
}

void MCButton::drawtabs(MCDC *dc, MCRectangle &srect)
{
	MCPoint p[10];  //polygon for each tab
	MCPoint box[6]; //polygon for outline the entire tab box
	int2 topx = 0;
	uint2 topwidth = 0;
    uint2 theight = ceilf(MCFontGetAscent(m_font) + MCFontGetDescent(m_font));
    uint2 t_tab_button_height = theight + 4;      // Magic number for padding
    uint2 t_pane_y_offset = t_tab_button_height;
	int2 taboverlap,tabrightmargin,tableftmargin,tabstartoffset;
	taboverlap = tabrightmargin = tableftmargin = 0;
	tabstartoffset = 2;
	//catch the values here
	if (MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_TAB))
	{
		taboverlap = MCcurtheme->getmetric(WTHEME_METRIC_TABOVERLAP);
		tabrightmargin = MCcurtheme->getmetric(WTHEME_METRIC_TABRIGHTMARGIN);
		tableftmargin = MCcurtheme->getmetric(WTHEME_METRIC_TABLEFTMARGIN);
		tabstartoffset = MCcurtheme->getmetric(WTHEME_METRIC_TABSTARTOFFSET);
        
        // If the theme uses a fixed tab button height, force the height to it
        uint2 t_fixed_height = MCcurtheme->getmetric(WTHEME_METRIC_TABBUTTON_HEIGHT);
        if (t_fixed_height != 0)
            t_tab_button_height = t_fixed_height;
        
        // If the theme uses tab buttons that overlap the pane, adjust the
        // offset at which the pane is drawn.
        if (MCcurtheme->getthemepropbool(WTHEME_PROP_TABBUTTONSOVERLAPPANE))
            t_pane_y_offset = t_tab_button_height / 2;
	}
	int2 curx = srect.x + tabstartoffset;
    int2 cury = srect.y + (t_tab_button_height - theight + 1)/2 + ceilf(MCFontGetAscent(m_font) + MCFontGetLeading(m_font));
	int2 yoffset = 0;
	uint2 i;
	uint2 curtab = MAXUINT2;
	Boolean reversetext = False;
	uint2 t_mousetab;
	t_mousetab = getmousetab(curx);
	if (state & CS_MFOCUSED && tabselectonmouseup())
		curtab = t_mousetab == starttab ? starttab : MAXUINT2;
	if (IsMacEmulatedLF())
		theight -= 2;
	MCWidgetInfo tabwinfo;
	tabwinfo.type = WTHEME_TYPE_TAB;
	getwidgetthemeinfo(tabwinfo);
	
	uindex_t t_ntabs;
	t_ntabs = MCArrayGetCount(tabs);
	for (i = 0 ; i < t_ntabs ; i++)
	{
		Boolean disabled = False;
		MCValueRef t_tabval = nil;
		/* UNCHECKED */ MCArrayFetchValueAtIndex(tabs, i + 1, t_tabval);
		MCStringRef t_tab;
		t_tab = (MCStringRef)t_tabval;
		MCRange t_range = MCRangeMake(0, MCStringGetLength(t_tab));
		if (MCStringGetCharAtIndex(t_tab, 0) == '(')
		{
			disabled = True;
			t_range.offset++;
			t_range.length--;
		}
		uint2 textx; //x coord of the begining of the button text
        // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
        uint2 twidth = MCFontMeasureTextSubstring(m_font, t_tab, t_range, getstack() -> getdevicetransform());

		if (MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_TABPANE) &&
		        MCcurtheme->iswidgetsupported(WTHEME_TYPE_TAB))
		{
			textx = curx + tableftmargin;
			twidth += tableftmargin + tabrightmargin;
			tabwinfo.state = disabled == True || flags & F_DISABLED? WTHEME_STATE_DISABLED: WTHEME_STATE_CLEAR;
			tabwinfo.attributes = WTHEME_ATT_CLEAR;
			if (i == menuhistory)
				tabwinfo.attributes |= WTHEME_ATT_TABRIGHTEDGE;
			else if (i + 2 == menuhistory)
				tabwinfo.attributes |= WTHEME_ATT_TABLEFTEDGE;


			//some themes require that you draw tab pane first to avoid major hacks
			if ((MCcurtheme->getthemepropbool( WTHEME_PROP_DRAWTABPANEFIRST) && i == 0) ||
			        (i + 1 == menuhistory && !MCcurtheme->getthemepropbool( WTHEME_PROP_DRAWTABPANEFIRST)))
			{
				// MW-2010-04-26: [[ Bug ]] This rect was being computed with an extra pixel width
				//   and height - 'compute_rect' treats corners as *inclusive*.
				MCRectangle tabpanelrect = MCU_compute_rect(srect.x, srect.y + t_pane_y_offset,
				                           srect.x + srect.width - 1, srect.y + srect.height - 1);
				MCWidgetInfo tabpanelwinfo;
				tabpanelwinfo.type = WTHEME_TYPE_TABPANE;
				getwidgetthemeinfo(tabpanelwinfo);
				tabpanelwinfo.state = flags & F_DISABLED? WTHEME_STATE_DISABLED: 0;
				MCWidgetTabPaneInfo inf;
				if (i + 1 == menuhistory)
				{
					//tell tab pane where not to draw border for selected tab
					inf.gap_start = curx - srect.x;
					inf.gap_length = twidth;
				}
				tabpanelwinfo.datatype = WTHEME_DATA_TABPANE;
				tabpanelwinfo.data = &inf;
				MCcurtheme->drawwidget(dc, tabpanelwinfo,tabpanelrect);
			}
			if (i + 1 == menuhistory)
			{
				tabwinfo.state |= WTHEME_STATE_HILITED;
				yoffset = 0;
			}
			else
			{
				if (i == curtab)
					tabwinfo.state |= WTHEME_STATE_PRESSED | WTHEME_STATE_HOVER;
				else if (i == focusedtab && ishovering)
					tabwinfo.state |= WTHEME_STATE_HOVER;
				yoffset = MCcurtheme->getmetric(WTHEME_METRIC_TABNONSELECTEDOFFSET);

			}
			if (i == 0)
				tabwinfo.attributes |= WTHEME_ATT_TABFIRSTSELECTED;
			//

			if (i==0)
                tabwinfo.attributes |= WTHEME_ATT_FIRSTTAB;
            // MW-2014-04-25: [[ Bug 6400 ]] A tab can be both first and last :)
            if (i == (t_ntabs - 1))
				tabwinfo.attributes |= WTHEME_ATT_LASTTAB;
            
			MCRectangle tabrect = MCU_compute_rect(curx, srect.y + yoffset, curx + twidth, srect.y + t_tab_button_height);
			if (MCcurtheme->getthemeid() != LF_NATIVEGTK || (srect.x + srect.width > curx + twidth + 5 &&
			        srect.y + srect.height > (srect.y + theight * 2)))
				MCcurtheme->drawwidget(dc, tabwinfo, tabrect);
			twidth -= taboverlap;

#ifdef _MACOSX
            // FG-2014-10-24: [[ Bugfix 11912 ]]
            // On OSX, reverse the text colour for selected tab buttons
            if (i+1 == menuhistory)
                reversetext = True;
            else
                reversetext = False;
#endif
		}
		else
			switch (MClook)
			{
			case LF_MAC:
				textx = curx + theight / 2 + 2;
				//draw black line (the outer most line)
				p[0].x = curx;
				p[0].y = p[7].y = srect.y + theight;

				p[1].x = p[0].x + (theight - 2) / 3;
				p[1].y = p[6].y = srect.y + 3;

				p[2].x = p[1].x + 2;
				p[2].y = p[5].y = p[1].y - 2;

				p[3].x = p[2].x + 2;
				p[3].y = p[4].y = srect.y;
				//p[3] is the straight line on top of the tab/button
				p[4].x = p[3].x + twidth + 2;
				p[5].x = p[4].x + 2;
				p[6].x = p[5].x + 2;
				p[7].x = p[6].x + (theight - 2) / 3;

				//fill tab with color
				p[0].y += 2;
				p[7].y = p[0].y;
				reversetext = False;

				if (i + 1 == menuhistory) //the tab that is on top(current tab)
					setforeground(dc, DI_BACK, False); //fill with background color
				else
					if (i == curtab)
					{
						dc->setforeground(maccolors[MAC_SHADOW]);
						dc->setfillstyle(FillSolid, nil, 0, 0);
						reversetext = True;
					}
					else
						setforeground(dc, DI_SHADOW, False);
				dc->fillpolygon(p, 8);
				p[0].y -= 2;
				p[7].y = p[0].y;

				//draw Black outline
				dc->setforeground(dc->getblack());
				dc->setfillstyle(FillSolid, nil, 0, 0);
				dc->drawlines(p, 8);

				if (i + 1 == menuhistory)
				{//Set the coord of Black box under the tab.


					//keep the first point and last point of the current tab button
					box[0] = p[0];
					box[5] = p[7];
				}

				//define gray polygon (inside of black line)
				p[0].x += 1;
				//p[0].y and p[7].y no change
				p[1].x += 1;
				p[6].y = p[1].y;
				p[2].y += 1;
				p[5].y = p[2].y;
				p[3].y += 1;
				p[4].y += 1;
				p[4].x -= 1;
				p[5].x -= 1;
				p[6].x -= 1;
				p[7].x -= 1;
				if (i + 1 != menuhistory)
				{//if not current tab, draw this polygon
					setforeground(dc, DI_BOTTOM, False);
					dc->drawlines(p, 8);
				}
				//draw white plygon (inside of gray line)
				p[0].x += 1; //only draw to p[4] point
				p[1].y += 2; //p[1].x and p[3].x are unchanged
				p[2].x += 1;
				p[2].y += 1;
				p[3].y += 1;
				p[4].y += 1;
				p[4].x += 1;
				setforeground(dc, DI_TOP, False);
				dc->drawlines(p, 5);

				twidth += theight * 2 / 3 + 9;
				break;
			default:
				twidth += 12;
				uint2 index;
				if (i + 1 == menuhistory
				        || (!getcindex(DI_SHADOW, index) && !getpindex(DI_SHADOW, index)))
					setforeground(dc, DI_BACK, False); //fill with background color
				else
					setforeground(dc, DI_SHADOW, False);
				MCRectangle trect;
				if (i + 1 == menuhistory)
				{
					trect.x = curx + 1;
					trect.width = twidth + 4;
					trect.y = srect.y + 1;
					trect.height = theight + 2;
					yoffset = 0;
				}


				else
				{
					trect.x = curx + 3;
					trect.width = twidth - 5;
					trect.y = srect.y + 2;
					trect.height = theight - 3;
					yoffset = 2;
				}
				dc->fillrect(trect);
				setforeground(dc, DI_TOP, False);
				textx = curx + 8; //set the x coord of the button text
				if (i + 1 == menuhistory)
				{
					topx = curx;
					topwidth = twidth + 2;
					p[0].x = p[1].x = curx;
					p[2].x = curx + 2;
					p[3].x = curx + twidth;
					p[0].y = srect.y + theight;

					p[1].y = srect.y + 2;
					p[2].y = p[3].y = srect.y;
					dc->drawlines(p, 4);
					p[0].x = p[1].x = curx + twidth + 1;
					p[0].y = srect.y + 2;
					p[1].y = srect.y + theight;
					setforeground(dc, DI_BOTTOM, False);
					dc->drawlines(p, 2);
					p[2].x = ++p[1].x;
					p[2].y = p[1].y;
					p[0].y--;
					p[1].y = p[0].y + 1;
					dc->setforeground(dc->getblack());
					dc->setfillstyle(FillSolid, nil, 0, 0);
					dc->drawlines(p, 3);
				}

				else
				{
					p[0].x = p[1].x = curx + 2;
					p[2].x = curx + 4;
					p[3].x = curx + twidth - 1;
					p[0].y = srect.y + theight - 1;
					p[1].y = srect.y + 4;
					p[2].y = p[3].y = srect.y + 2;
					if (i == menuhistory)
					{
						p[1].x++;
						p[1].y--;
						dc->drawlines(&p[1], 3);
					}
					else
						dc->drawlines(p, 4);
					if (i + 2 != menuhistory)
					{
						p[0].x = p[1].x = curx + twidth;
						p[0].y = srect.y + 4;
						p[1].y = srect.y + theight - 1;
						setforeground(dc, DI_BOTTOM, False);
						dc->drawlines(p, 2);
						p[2].x = ++p[1].x;
						p[2].y = p[1].y;
						p[0].y--;
						p[1].y = p[0].y + 1;
						dc->setforeground(dc->getblack());
						dc->setfillstyle(FillSolid, nil, 0, 0);
						dc->drawlines(p, 3);
					}
				}
			}
		if (disabled || flags & F_DISABLED)
		{

			switch (MClook)
			{
			case LF_MOTIF:
				setforeground(dc, DI_FORE, False);
				dc->setopacity(127);
				break;
			case LF_MAC:
				dc->setforeground(dc->getgray());
				dc->setfillstyle(FillSolid, nil, 0, 0);
				break;
			default:
				setforeground(dc, DI_TOP, False);
                dc -> drawtext_substring(textx, cury + yoffset + 1, t_tab, t_range, m_font, false, kMCDrawTextNoBreak);
				setforeground(dc, DI_BOTTOM, False);
				break;
			}
		}
		else
			if (reversetext)
				setforeground(dc, DI_PSEUDO_BUTTON_TEXT_SEL, False, True);
			else
				setforeground(dc, DI_PSEUDO_BUTTON_TEXT, False);
        // AL-2014-09-24: [[ Bug 13528 ]] Don't draw character indicating button is disabled
        dc -> drawtext_substring(textx, cury + yoffset, t_tab, t_range, m_font, false, kMCDrawTextNoBreak);
		if ((disabled || flags & F_DISABLED) && MClook == LF_MOTIF)
			dc->setfillstyle(FillSolid, nil, 0 , 0);
		curx += twidth;
	} //end of tab buttons loop
	if (!(MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_TABPANE) &&
	        MCcurtheme->iswidgetsupported(WTHEME_TYPE_TAB)))
	{//already done with draw panel
		switch (MClook)
		{
		case LF_MAC:
			//draw the Black box first
			box[1].x = box[2].x = srect.x;
			box[1].y = box[0].y;
			box[2].y = box[3].y = srect.y + srect.height - 1;
			box[3].x = box[4].x = srect.x + srect.width - 1;
			box[4].y = box[5].y;
			dc->setforeground(dc->getblack());
			dc->setfillstyle(FillSolid, nil, 0, 0);
			dc->drawlines(box, 6);
			//draw gray box(inside of black box) under the tab
			box[0].x += 1;
			box[0].y = box[1].y = box[4].y = box[5].y += 1;
			box[2].x = box[1].x += 1;
			box[2].y = box[3].y -= 1;
			box[3].x = box[4].x -= 1;
			setforeground(dc, DI_BOTTOM, False);
			dc->drawlines(box, 6);
			//draw white box(inside of black box) under the tab
			box[1].x = box[2].x += 1;
			box[0].y = box[1].y = box[4].y = box[5].y += 1;
			box[2].y = box[3].y -= 1;
			box[3].x = box[4].x -= 1;
			setforeground(dc, DI_TOP, False);
			dc->drawlines(box, 6);
			break;
		default:
			srect.y += theight;
			srect.height -= theight;
			setforeground(dc, DI_TOP, False);
			p[0].x = p[1].x = srect.x + 2;
			p[2].x = topx;
			p[0].y = srect.y + srect.height - 2;
			p[1].y = p[2].y = srect.y;
			dc->drawlines(p, 3);
			p[0].x = topx + topwidth + 1;
			p[1].x = srect.x + srect.width - 2;
			p[0].y = srect.y;
			dc->drawlines(p, 2);
			setforeground(dc, DI_BOTTOM, False);
			p[0].x = srect.x + 2;
			p[1].x = p[2].x = srect.x + srect.width - 2;
			p[0].y = p[1].y = srect.y + srect.height - 2;
			p[2].y = srect.y + 1;
			dc->drawlines(p, 3);
			dc->setforeground(dc->getblack());

			dc->setfillstyle(FillSolid, nil, 0, 0);
			p[0].x--;
			p[0].y = ++p[1].y;
			p[1].x = ++p[2].x;
			dc->drawlines(p, 3);
		}
	}
}

void MCButton::drawmacdefault(MCDC *dc, const MCRectangle &srect)
{
	int2 rx = srect.x + srect.width - 1;
	int2 by = srect.y + srect.height - 1;
	MCPoint p[20];
	p[0].x = p[5].x = srect.x + 3;
	p[1].x = p[4].x = rx - 3;
	p[2].x = p[3].x = rx;
	p[6].x = p[7].x = srect.x;
	p[0].y = p[1].y = srect.y;
	p[2].y = p[7].y = srect.y + 3;
	p[3].y = p[6].y = by - 3;
	p[4].y = p[5].y = by;
	p[8] = p[0];
	if (flags & F_DISABLED)
	{
		dc->setforeground(dc->getgray());
		dc->setfillstyle(FillSolid, nil, 0, 0);
		dc->drawlines(p, 9);
		return;
	}
	dc->setforeground(dc->getblack());
	dc->setfillstyle(FillSolid, nil, 0, 0);
	dc->drawlines(p, 9);
	p[0].x = p[12].x = p[19].x = srect.x + 4;
	p[1].x = p[2].x = p[9].x = rx - 4;
	p[3].x = p[4].x = p[7].x = p[8].x = p[10].x = rx - 3;
	p[5].x = p[6].x = rx - 2;
	p[13].x = p[14].x = p[17].x = p[18].x = srect.x + 3;
	p[15].x = p[16].x = p[11].x = srect.x + 2;
	p[0].y = p[1].y = srect.y + 2;
	p[2].y = p[3].y = p[5].y = p[18].y = p[19].y = srect.y + 3;
	p[4].y = p[16].y = p[17].y = srect.y + 4;
	p[7].y = p[14].y = by - 4;
	p[6].y = p[8].y = p[9].y = p[12].y = p[13].y = p[15].y = by - 3;
	p[10].y = p[11].y = by - 2;
	setforeground(dc, DI_BOTTOM, False);
	dc->drawlines(p, 20);
	p[0].x = srect.x + 3;
	p[1].x = rx - 3;
	p[2].x = p[3].x = rx - 1;
	p[0].y = p[1].y = by - 1;
	p[2].y = by - 3;
	p[3].y = srect.y + 3;
	dc->setforeground(maccolors[MAC_SHADOW]);
	dc->setfillstyle(FillSolid, nil, 0, 0);
	dc->drawlines(p, 4);
}


void MCButton::drawstandardbutton(MCDC *dc, MCRectangle &srect)
{
	if (MCcurtheme)
	{
		MCWidgetInfo winfo;
		winfo.type = getstyleint(flags) == F_STANDARD ? WTHEME_TYPE_PUSHBUTTON:WTHEME_TYPE_BEVELBUTTON;
		if (MCcurtheme->iswidgetsupported(winfo.type))
		{
			getwidgetthemeinfo(winfo);
			if (state & CS_SHOW_DEFAULT)
			{
				winfo.state |= WTHEME_STATE_HASDEFAULT;
			}
			
			if (MCmousestackptr && MCmousestackptr == this->getstack())
			{
				// if the mouse button is down after clicking on a button, don't draw the default button animation
				MCControl *t_control = MCmousestackptr->getcard()->getmfocused();
				if (MCbuttonstate && t_control && t_control->gettype() == CT_BUTTON)
                {
                    // FG-2014-11-05: [[ Bugfix 13909 ]]
                    // Don't suppress the default on OSX Yosemite if it is this
                    // button being pressed but the mouse is outside the button.
                    if (rect.x > MCmousex && MCmousex >= rect.x+rect.width
                              && rect.y > MCmousey && MCmousey >= rect.y+rect.height)
                        winfo.state |= WTHEME_STATE_SUPPRESSDEFAULT;
                }
			}
	
            // On Yosemite, the default button theme is only suppressed when the
            // app is not active.
            if (getflag(F_DEFAULT) && IsMacLFAM() && MCaqua && MCmajorosversion >= 0x10A0)
            {
                winfo.state &= ~WTHEME_STATE_SUPPRESSDEFAULT;
                winfo.state |= WTHEME_STATE_HASDEFAULT;
            }
            
#ifdef _MACOSX
			// MW-2010-12-05: [[ Bug 9210 ]] Make sure we disable the default look when the app is
			//   in the background.
			if (!MCappisactive)
				winfo.state |= WTHEME_STATE_SUPPRESSDEFAULT;
#endif
			
			if (!(winfo.state & WTHEME_STATE_PRESSED) && winfo.state & WTHEME_STATE_HASDEFAULT && IsMacLFAM() && MCaqua && dc -> gettype() == CONTEXT_TYPE_SCREEN && !(flags & F_DISABLED) && getstyleint(flags) == F_STANDARD)
			{
				MCcurtheme->drawwidget(dc, winfo, srect);
                
                // MM-2014-07-31: [[ ThreadedRendering ]] Make sure only a single thread posts the timer message (i.e. the first that gets here)
                if (!m_animate_posted)
                {
                    if (!m_animate_posted)
                    {
                        m_animate_posted = true;
                        MCscreen->addtimer(this, MCM_internal, THROB_RATE);
                    }
                }
			}
			else
				MCcurtheme->drawwidget(dc, winfo, srect);

			return;
		}
	}

	drawmacborder(dc, srect);
	if (flags & F_DISABLED)
		return;
	int2 rx = srect.x + srect.width - 1;
	int2 by = srect.y + srect.height - 1;
	MCPoint p[5];
	p[0].x = srect.x + 1;
	p[1].x = p[2].x = rx - 3;
	p[3].x = p[4].x = rx - 2;

	p[0].y = p[1].y = by - 2;
	p[2].y = p[3].y = by - 3;
	p[4].y = srect.y + 1;
	if (state & CS_HILITED)
	{
		dc->setforeground(maccolors[MAC_SHADOW]);
		dc->setfillstyle(FillSolid, nil, 0, 0);
	}
	else
		setforeground(dc, DI_BOTTOM, False);
	dc->drawlines(p, 5);
	if (!(state & CS_HILITED))
	{
		p[0].x = p[1].x = srect.x + 2;
		p[2].x = srect.x + 3;
		p[4].x = rx - 2;
		p[0].y = by - 2;
		p[1].y = p[4].y = srect.y + 2;
		p[2].y = srect.y + 3;
		p[3] = p[1];
		setforeground(dc, DI_TOP, False);
		dc->drawlines(p, 5);
	}
	p[0].x = srect.x + 2;
	p[1].x = p[2].x = rx - 2;
	p[3].x = p[4].x = rx - 1;
	p[0].y = p[1].y = by - 1;
	p[2].y = p[3].y = by - 2;
	p[4].y = srect.y + 2;
	if (state & CS_HILITED)
		setforeground(dc, DI_BOTTOM, False);
	else
	{

		dc->setforeground(maccolors[MAC_SHADOW]);
		dc->setfillstyle(FillSolid, nil, 0, 0);
	}
	dc->drawlines(p, 5);
}

void MCButton::drawmacborder(MCDC *dc, MCRectangle &srect)
{
	int2 rx = srect.x + srect.width - 1;
	int2 by = srect.y + srect.height - 1;
	MCPoint p[9];

	p[0].x = p[5].x = srect.x + 2;
	p[1].x = p[4].x = rx - 2;
	p[2].x = p[3].x = rx;
	p[6].x = p[7].x = srect.x;
	p[0].y = p[1].y = srect.y;
	p[2].y = p[7].y = srect.y + 2;
	p[3].y = p[6].y = by - 2;
	p[4].y = p[5].y = by;
	p[8] = p[0];
	if (flags & F_DISABLED)
		dc->setforeground(dc->getgray());
	else
		dc->setforeground(dc->getblack());
	dc->setfillstyle(FillSolid, nil, 0, 0);
	dc->drawlines(p, 9);
}

void MCButton::drawmacpopup(MCDC *dc, MCRectangle &srect)
{
	if (flags & F_DISABLED)
	{
		dc->setforeground(dc->getgray());
		dc->setfillstyle(FillSolid, nil, 0, 0);
		dc->drawline(srect.x, srect.y - 1, srect.x, srect.y + srect.height);
	}
	else
	{
		draw3d(dc, srect, state & CS_SUBMENU ? ETCH_SUNKEN : ETCH_RAISED, 1);
		dc->setforeground(maccolors[MAC_SHADOW]);
		dc->setfillstyle(FillSolid, nil, 0, 0);
		MCPoint p[5];
		p[0].x = srect.x;
		p[1].x = p[2].x = srect.x + srect.width - 1;
		p[3].x = p[4].x = p[1].x + 1;
		p[0].y = p[1].y = srect.y + srect.height;
		p[2].y = p[3].y = p[0].y - 1;
		p[4].y = srect.y;
		dc->drawlines(p, 5);
	}
	uint2 q = srect.width >> 2;
	drawarrow(dc, srect.x + q / 2, srect.y - 1, srect.width - q, AD_UP,
	          False, (state & CS_SUBMENU) != 0);
	drawarrow(dc, srect.x + q / 2, srect.y + q + 1, srect.width - q, AD_DOWN,
	          False, (state & CS_SUBMENU) != 0);
}


void MCButton::getwidgetthemeinfo(MCWidgetInfo &widgetinfo)
{
	//sets the widgetinfo structure based on the objects current state & flags
	//developer should always pass in structure with widgetinfo.type set because we
	//may optimize in future and only set attributes based on type..in addition this should
	//always be called before manipulating the widgetinfo structure directly just
	//in case structure changes
	uint4 wstate = 0;

	MCControl::getwidgetthemeinfo(widgetinfo);
	if (flags & F_DISABLED)
		wstate |= WTHEME_STATE_DISABLED;
	else
		if (state & CS_MFOCUSED && !(state & CS_SELECTED)
		        && MCU_point_in_rect(rect, mx, my) &&
		        (entry == NULL || !MCU_point_in_rect(entry->getrect(), mx, my))
		        && (widgetinfo.type != WTHEME_TYPE_PUSHBUTTON || state & CS_HILITED))
			wstate |= WTHEME_STATE_PRESSED;
	if (state & CS_HILITED)
		wstate |= WTHEME_STATE_HILITED;
	if (!(state & CS_SELECTED) && MCU_point_in_rect(rect, mx, my) && ishovering)
		wstate |= WTHEME_STATE_HOVER;
	if (state & CS_KFOCUSED)
		wstate |= WTHEME_STATE_HASFOCUS;
	
	// MW-2005-07-25: [[Bug 2574]] A 'hilited' push-button, should actually be pressed
	if (widgetinfo . type == WTHEME_TYPE_PUSHBUTTON && state & CS_HILITED)
		wstate |= WTHEME_STATE_PRESSED;
	
	widgetinfo.state = wstate;
}


// MW-2011-09-30: [[ Redraw ]] This method is used when state potentially changes
//   to issue a redraw. It only does so if the button is not unadorned.
void MCButton::mayberedrawall(void)
{
	// If the button consists of potentially more than an icon, then redraw all.
	if (!getflag(F_SHOW_ICON) ||
		getflag(F_OPAQUE | F_SHOW_BORDER | F_SHOW_NAME | F_SHADOW) ||
		(getflag(F_HILITE_BORDER) && getstate(CS_HILITED)) ||
		(getflag(F_ARM_BORDER) && getstate(CS_ARMED)) ||
		((extraflags & EF_NO_FOCUS_BORDER) == 0 && getstate(CS_KFOCUSED)) ||
		entry != nil)
	{
		layer_redrawall();
		return;
	}

	// If the button has auto hilite on, and has a hilite icon then redraw all.
	if (getflag(F_AUTO_HILITE) && getflag(F_HAS_ICONS) &&
		icons -> iconids[CI_HILITED] != 0)
	{
		layer_redrawall();
		return;
	}

	// Otherwise if the button has a hover icon, redraw all.
	if (getflag(F_HAS_ICONS) &&
		icons -> iconids[CI_HOVER] != 0)
	{
		layer_redrawall();
		return;
	}
}

// MW-2011-09-20: [[ Collision ]] The button's shape could possibly be the icon it
//   references so optimize for that.
bool MCButton::lockshape(MCObjectShape& r_shape)
{
	// Make sure we only consider the case where the button has only an icon.
	if (!getflag(F_SHOW_ICON) ||
		getflag(F_OPAQUE | F_SHOW_BORDER | F_SHOW_NAME | F_SHADOW) ||
		(getflag(F_HILITE_BORDER) && getstate(CS_HILITED)) ||
		(getflag(F_ARM_BORDER) && getstate(CS_ARMED)) ||
		((extraflags & EF_NO_FOCUS_BORDER) == 0 && getstate(CS_KFOCUSED)) ||
		entry != nil)
	{
		// More work needed here to optimize opaque case.
		r_shape . type = kMCObjectShapeComplex;
		r_shape . bounds = getrect();
		return true;
	}
	
	// Assuming the dissection of the draw method is correct, then when here we
	// should be a transparent button with just an icon!
	
	// Compute the center of the icon (taken from buttondraw).
	MCRectangle shadowrect;
	shadowrect = rect;
	int2 centerx =  shadowrect.x + leftmargin + ((shadowrect.width - leftmargin - rightmargin) >> 1);
	int2 centery =  shadowrect.y + topmargin + ((shadowrect.height - topmargin - bottommargin) >> 1);
	
	// Get the image object - if none, we must be empty.
	MCImage *t_image;
	t_image = icons -> curicon;
	if (t_image == nil)
	{
		r_shape . type = kMCObjectShapeEmpty;
		return true;
	}
	
	MCRectangle t_image_rect;
	t_image_rect = t_image -> getrect();
	
	MCRectangle t_bounds;
	t_bounds = getrect();
	
	MCPoint t_origin;
	t_origin = MCPointMake(centerx - t_image_rect . width / 2, centery - t_image_rect . height / 2);

	return t_image -> lockbitmapshape(t_bounds, t_origin, r_shape);
}

void MCButton::unlockshape(MCObjectShape& p_shape)
{
	if (p_shape . type == kMCObjectShapeMask)
		icons -> curicon -> unlockbitmap(p_shape . mask . bits);
}

int16_t MCButton::GetCheckSize() const
{
    // If we aren't using GTK at the theming engine, return the fixed size
    if (!IsNativeGTK())
        return CHECK_SIZE;
    
    return MCcurtheme -> getmetric(WTHEME_METRIC_CHECKBUTTON_INDICATORSIZE);
}
