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

#include "dispatch.h"
#include "execpt.h"
#include "stack.h"
#include "card.h"
#include "tooltip.h"
#include "util.h"
#include "globals.h"
#include "mctheme.h"
#include "mode.h"
#include "font.h"

#include "context.h"

MCTooltip::MCTooltip()
{
	setname_cstring("Tool Tip");
	tooltip = NULL;
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
	if (MCNameIsEqualTo(mptr, MCM_internal, kMCCompareCaseless))
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
	if (tooltip != NULL)
		if (opened)
			MCscreen->addtimer(this, MCM_internal2, MCtooltime);
		else
			if (!(state & CS_NO_FOCUS) && MCtooltipdelay != 0)
				MCscreen->addtimer(this, MCM_internal, MCtooltipdelay);
}

void MCTooltip::clearmatch(MCCard *c)
{
	if (card == c)
		card = NULL;
}

void MCTooltip::settip(const char *tip)
{
	if (tip != tooltip)
	{
		tooltip = tip;
		state &= ~CS_NO_FOCUS;
		if (opened && !(state & CS_IGNORE_CLOSE))
		{
			close();
			if (tooltip == NULL)
				MCscreen->cancelmessageobject(this, NULL);
			else
				opentip();
		}
		else
			if (tooltip == NULL)
				MCscreen->cancelmessageobject(this, NULL);
			else
				if (MCtooltipdelay != 0)
					MCscreen->addtimer(this, MCM_internal, MCtooltipdelay);
	}
}

void MCTooltip::opentip()
{
	if (tooltip == NULL || card == NULL)
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
		char *t_colorname;
		t_colorname = nil;
		MCscreen -> parsecolor(MCttbgcolor, &t_color, &t_colorname);
		delete t_colorname;
		MCModeShowToolTip(trect . x, trect . y + 16,
				MCttsize, (t_color . red >> 8) | (t_color . green & 0xFF00) | ((t_color . blue & 0xFF00) << 8), MCttfont,
				tooltip);
		MCscreen->addtimer(this, MCM_internal2, MCtooltime);
		openrect(trect, WM_TOOLTIP, NULL, WP_DEFAULT,OP_NONE);
		state |= CS_NO_FOCUS;
		return;
	}

	minheight = minwidth = 1;
	setsprop(P_BACK_COLOR, MCttbgcolor);

	// MW-2012-02-17: [[ LogFonts ]] Convert the tooltip font string to
	//   a name and create the font.
	MCAutoNameRef t_tt_font;
	t_tt_font . CreateWithCString(MCttfont);
	/* UNCHECKED */ MCFontCreate(t_tt_font, MCFontStyleFromTextStyle(FA_DEFAULT_STYLE), MCttsize, m_font);

	rect.width = 0;

	if (MCcurtheme != NULL)
		rect . height = MCcurtheme -> fetchtooltipstartingheight();
	else
		rect . height = 0;

	int32_t t_fheight;
	t_fheight = MCFontGetAscent(m_font) + MCFontGetDescent(m_font);

	const char *t_tooltip;
	t_tooltip = tooltip;
	do
	{
		const char *t_next_line;
		t_next_line = strchr(t_tooltip, 10);

		if (t_next_line == NULL)
			t_next_line = t_tooltip + strlen(t_tooltip);

		// MW-2012-03-13: [[ UnicodeToolTip ]] Convert the UTF-8 to UTF-16 and measure.
		MCExecPoint ep;
		ep . setsvalue(MCString(t_tooltip, t_next_line - t_tooltip));
		ep . utf8toutf16();
		rect.width = MCU_max(MCFontMeasureText(m_font, ep . getsvalue() . getstring(), ep . getsvalue() . getlength(), true) + 8, rect.width);
		rect.height += t_fheight + 3;

		t_tooltip = t_next_line;
		if (*t_tooltip == 10)
			t_tooltip += 1;
	}
	while(*t_tooltip != '\0');

	openrect(trect, WM_TOOLTIP, NULL, WP_DEFAULT,OP_NONE);
	state |= CS_NO_FOCUS;

	if (MCcurtheme != NULL && window != DNULL)
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
	if (tooltip == nil)
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

	int t_y;
	const char *t_tooltip;
	t_tooltip = tooltip;
	t_y = 0;
	do
	{
		const char *t_next_line;
		t_next_line = strchr(t_tooltip, 10);

		if (t_next_line == NULL)
			t_next_line = t_tooltip + strlen(t_tooltip);

		// MW-2012-03-13: [[ UnicodeToolTip ]] Convert the UTF-8 to UTF-16 and draw.
		MCExecPoint ep;
		ep . setsvalue(MCString(t_tooltip, t_next_line - t_tooltip));
		ep . utf8toutf16();
        dc -> drawtext(4, t_y + t_fheight, ep.getsvalue().getstring(), ep.getsvalue().getlength(), m_font, false, true);

		t_y += t_fheight + 3;

		t_tooltip = t_next_line;
		if (*t_tooltip == 10)
			t_tooltip += 1;
	}
	while(*t_tooltip != '\0');

	if (!MCaqua && !t_themed)
		drawborder(dc, trect, 1);
}
