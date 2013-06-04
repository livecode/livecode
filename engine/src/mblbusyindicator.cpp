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
#include "mcio.h"

#include "globals.h"
#include "stack.h"
#include "image.h"
#include "param.h"
#include "card.h"

#include "exec.h"


#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////

void MCSystemBusyIndicatorStart (MCBusyIndicatorType p_indicator, const char *p_label, int32_t p_opacity);
void MCSystemBusyIndicatorStop ();

////////////////////////////////////////////////////////////////////////////////

// MM-2013-02-04: [[ Bug 10642 ]] Added new optional opacity parameter to busy indicator.
void MCBusyIndicatorExecStart (MCExecContext& ctxt, MCBusyIndicatorType p_indicator, const char *p_label, int32_t p_opacity)
{
    MCSystemBusyIndicatorStart(p_indicator, p_label, p_opacity);
}

void MCBusyIndicatorExecStop (MCExecContext& ctxt)
{
    MCSystemBusyIndicatorStop();
}

static MCBusyIndicatorType MCBusyIndicatorTypeFromCString(const char *p_string)
{
    if (MCCStringEqualCaseless(p_string, "in line"))
        return kMCBusyIndicatorInLine;
    else if (MCCStringEqualCaseless(p_string, "square"))
        return kMCBusyIndicatorSquare;
    else if (MCCStringEqualCaseless(p_string, "keyboard"))
        return kMCBusyIndicatorKeyboard;
    
    return kMCBusyIndicatorSquare;
}

static bool MCBusyIndicatorTypeToCString(MCSensorType p_indicator, char *&r_string)
{
    switch (p_indicator)
    {
        case kMCBusyIndicatorInLine:
            return MCCStringClone("in line", r_string);
        case kMCBusyIndicatorSquare:
            return MCCStringClone("square", r_string);
        case kMCBusyIndicatorKeyboard:
            return MCCStringClone("keyboard", r_string);
        default:
            return MCCStringClone("unknown", r_string);
    }
    return false;
}

// MM-2013-02-04: [[ Bug 10642 ]] Added new optional opacity parameter to busy indicator.
Exec_stat MCHandleStartBusyIndicator(void *p_context, MCParameter *p_parameters)
{
    MCBusyIndicatorType t_indicator_type;
    MCExecPoint ep(nil, nil, nil);
    
    if (p_parameters)
    {
        p_parameters -> eval(ep);
        t_indicator_type = MCBusyIndicatorTypeFromCString(ep . getcstring());
        p_parameters = p_parameters -> getnext();
    }
    
    const char *t_label;
    t_label = nil;
    if (p_parameters)
    {
        p_parameters -> eval(ep);
        t_label = ep . getcstring();
        p_parameters = p_parameters -> getnext();
    }
    
    int32_t t_opacity;
    t_opacity = -1;
    if (p_parameters)
    {
        p_parameters -> eval(ep);
        t_opacity = ep . getint4();
        if (t_opacity < 0 || t_opacity > 100)
            t_opacity = -1;
    }
    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    MCBusyIndicatorExecStart(t_ctxt, kMCBusyIndicatorSquare, t_label, t_opacity);
    return t_ctxt.GetExecStat();
}

Exec_stat MCHandleStopBusyIndicator(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    MCBusyIndicatorExecStop(t_ctxt);
    
    return t_ctxt.GetExecStat();
}
