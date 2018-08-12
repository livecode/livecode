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

#include "dispatch.h"

#include "stack.h"
#include "card.h"
#include "tooltip.h"
#include "util.h"
#include "globals.h"
#include "mctheme.h"
#include "mode.h"
#include "font.h"
#include "exec.h"

#include "context.h"

MCTooltip::MCTooltip()
{
	setname_cstring("Tool Tip");
	tip = MCValueRetain(kMCEmptyString);
	state |= CS_NO_MESSAGES;
	card = NULL;
	cards = MCtemplatecard->clone(False, False);
	cards->setparent(this);
	cards->setstate(True, CS_NO_MESSAGES);

	m_font = nil;
}

MCTooltip::~MCTooltip()
{}

void MCTooltip::close(void)
{
	if (!MCModeMakeLocalWindows())
		MCModeHideToolTip();
	MCStack::close();
}

void MCTooltip::timer(MCNameRef mptr, MCParameter *params)
{
#ifndef _MOBILE
	if (MCNameIsEqualToCaseless(mptr, MCM_internal))
		opentip();
	else
		close();
#endif
}

void MCTooltip::mousemove(int2 x, int2 y, MCCard *c)
{
	MCscreen->cancelmessageobject(this, NULL);
	mx = x;
	my = y;
	card = c;
	if (!MCStringIsEmpty(tip))
    {
		if (opened)
			MCscreen->addtimer(this, MCM_internal2, MCtooltime);
		else
			if (!(state & CS_NO_FOCUS) && MCtooltipdelay != 0)
				MCscreen->addtimer(this, MCM_internal, MCtooltipdelay);
    }
}

void MCTooltip::clearmatch(MCCard *c)
{
	if (card == c)
		card = NULL;
}


void MCTooltip::cleartip()
{
	if (!MCStringIsEmpty(tip))
	{
		state &= ~CS_NO_FOCUS;
		if (opened && !(state & CS_IGNORE_CLOSE))
			close();
		MCscreen->cancelmessageobject(this, NULL);
	}
	MCValueAssign(tip, kMCEmptyString);
}

void MCTooltip::settip(MCStringRef p_tip)
{	
	if (MCStringIsEqualTo(tip, p_tip, kMCStringOptionCompareExact))
		return;
	
	if (MCStringIsEmpty(p_tip))
	{
		cleartip();
		return;
	}
	
	MCValueAssign(tip, p_tip);
	state &= ~CS_NO_FOCUS;
	if (opened && !(state & CS_IGNORE_CLOSE))
	{
		close();
		opentip();
	}
	else if (MCtooltipdelay != 0)
		MCscreen->addtimer(this, MCM_internal, MCtooltipdelay);
}

void MCTooltip::opentip()
{
    // PM-2015-07-02: [[ Bug 15561 ]] Tooltip should not appear for controls out of the visible window
    if (MCStringIsEmpty(tip) || card == NULL
             || !MCU_point_in_rect(card -> getrect(), mx, my))
		return;

	MCStack *sptr = card->getstack();
	if (sptr->getmode() != WM_PALETTE && !sptr->getstate(CS_KFOCUSED))
		return;

	MCRectangle trect;
	MCU_set_rect(trect, mx, my, 16, 16);
	trect = MCU_recttoroot(card->getstack(), trect);

	parent = MCdispatcher;

	if (!MCModeMakeLocalWindows())
	{
		MCColor t_color;
		MCscreen -> parsecolor(MCttbgcolor, t_color, nil);
		MCModeShowToolTip(trect . x, trect . y + 16,
				MCttsize, (t_color . red >> 8) | (t_color . green & 0xFF00) | ((t_color . blue & 0xFF00) << 8), MCttfont,
				tip);
		MCscreen->addtimer(this, MCM_internal2, MCtooltime);
		openrect(trect, WM_TOOLTIP, NULL, WP_DEFAULT,OP_NONE);
		state |= CS_NO_FOCUS;
		return;
	}

	minheight = minwidth = 1;
    
    // Get the colour for the tooltip background
    MCColor t_bg_color;
    if (MCPlatformGetControlThemePropColor(getcontroltype(), getcontrolsubpart(), getcontrolstate(), kMCPlatformThemePropertyBackgroundColor, t_bg_color))
    {
        MCExecContext ctxt(this, nil, nil);
		uint32_t t_pixel = MCColorGetPixel(t_bg_color);
        SetBackPixel(ctxt, &t_pixel);
    }
    else
        setsprop(P_BACK_COLOR, MCttbgcolor);

    // Get the font for the tooltip
    if (!MCPlatformGetControlThemePropFont(getcontroltype(), getcontrolsubpart(), getcontrolstate(), kMCPlatformThemePropertyTextFont, m_font))
    {
        // MW-2012-02-17: [[ LogFonts ]] Convert the tooltip font string to
        //   a name and create the font.
        MCNewAutoNameRef t_tt_font;
        /* UNCHECKED */ MCNameCreate(MCttfont, &t_tt_font);
        /* UNCHECKED */ MCFontCreate(*t_tt_font, MCFontStyleFromTextStyle(FA_DEFAULT_STYLE), MCttsize, m_font);
    }
    
	rect.width = 0;

	if (MCcurtheme != NULL)
		rect . height = MCcurtheme -> fetchtooltipstartingheight();
	else
		rect . height = 0;

	int32_t t_fheight;
	t_fheight = MCFontGetAscent(m_font) + MCFontGetDescent(m_font);

	// Split the tooltip into lines in order to measure its bounding box
	MCAutoArrayRef lines;
	/* UNCHECKED */ MCStringSplit(tip, MCSTR("\n"), nil, kMCCompareExact, &lines);
	uindex_t nlines = MCArrayGetCount(*lines);
	for (uindex_t i = 0; i < nlines; i++)
	{
		MCStringRef t_line = nil;
		MCValueRef t_lineval = nil;
		/* UNCHECKED */ MCArrayFetchValueAtIndex(*lines, i + 1, t_lineval);
		t_line = (MCStringRef)t_lineval;
        // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
        rect.width = MCU_max(MCFontMeasureText(m_font, t_line, getstack() -> getdevicetransform()) + 8, rect.width);
		rect.height += t_fheight +3;
	}

	openrect(trect, WM_TOOLTIP, NULL, WP_DEFAULT,OP_NONE);
	state |= CS_NO_FOCUS;

	if (MCcurtheme != NULL && window != NULL)
		MCcurtheme -> applythemetotooltipwindow(window, rect);
}

void MCTooltip::closetip()
{
	MCFontRelease(m_font);
	m_font = nil;

	MCscreen->cancelmessageobject(this, NULL);
	if (opened)
		close();
}

void MCTooltip::render(MCContext *dc, const MCRectangle &dirty)
{
	// IM-2012-05-31 [[ Malte ]] fix linux crashes that can occur when the tooltip text is NULL
	// (probably shouldn't happen, but there you go)!
	// SJT-2014-05-29 Fix crash when m_font was NULL, seems we
	// were being called on a closed window.  Probably caused by
	// stale update events.
	if (!opened)
		return;

	MCRectangle trect;
	MCU_set_rect(trect, 0, 0, rect.width, rect.height);

	bool t_themed;
	t_themed = false;

	if (MCcurtheme != NULL &&
		MCcurtheme -> drawtooltipbackground(dc, trect))
	{
		MCcurtheme -> settooltiptextcolor(dc);
		t_themed = true;
	}
	else
	{
		setforeground(dc, DI_BACK, False);
		dc -> fillrect(trect);
		dc -> setforeground(MCscreen -> black_pixel);
	}

	int32_t t_fheight;
	t_fheight = MCFontGetAscent(m_font) + MCFontGetDescent(m_font);

	// Split the tooltip into lines in order to measure its bounding box
	int t_y = 0;
	MCAutoArrayRef lines;
	/* UNCHECKED */ MCStringSplit(tip, MCSTR("\n"), nil, kMCCompareExact, &lines);
	uindex_t nlines = MCArrayGetCount(*lines);
	for (uindex_t i = 0; i < nlines; i++)
	{
		MCStringRef t_line = nil;
		MCValueRef t_lineval = nil;
		/* UNCHECKED */ MCArrayFetchValueAtIndex(*lines, i + 1, t_lineval);
		t_line = (MCStringRef)t_lineval;
        
        drawdirectionaltext(dc, 4, t_y + t_fheight, t_line, m_font);

		t_y += t_fheight + 3;
	}

	if (!MCaqua && !t_themed)
		drawborder(dc, trect, 1);
}

MCPlatformControlType MCTooltip::getcontroltype()
{
    MCPlatformControlType t_type;
    t_type = MCObject::getcontroltype();
    
    if (t_type != kMCPlatformControlTypeGeneric)
        return t_type;
    else
        return kMCPlatformControlTypeTooltip;
}
