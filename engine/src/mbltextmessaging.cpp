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

#include "exec.h"

#include "mblsyntax.h"

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

bool MCSystemCanSendTextMessage();
bool MCSystemComposeTextMessage(const char *p_recipients, const char *p_body);

////////////////////////////////////////////////////////////////////////////////

void MCCanSendTextMessageExec(MCExecContext& ctxt, bool& r_can_send)
{
    r_can_send = MCSystemCanSendTextMessage();
}

void MCComposeTextMessageExec(MCExecContext& ctxt, const char *p_recipients, const char *p_body)
{
	MCSystemComposeTextMessage(p_recipients, p_body);
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleCanComposeTextMessage(void *p_context, MCParameter *p_parameters)
{
    if (MCSystemCanSendTextMessage())
        MCresult -> sets(MCtruestring);
	else
        MCresult -> sets(MCfalsestring);
	return ES_NORMAL;
}

Exec_stat MCHandleComposeTextMessage(void *p_context, MCParameter *p_parameters)
{
    char *t_recipients, *t_body;
    bool t_success;
	MCExecPoint ep(nil, nil, nil);
	ep . clear();
	if (!MCSystemCanSendTextMessage())
    {
        MCresult -> sets(MCfalsestring);
        return ES_NORMAL;
    }
	
	t_success = MCParseParameters(p_parameters, "s", &t_recipients);
    if (t_success == false)
    {
        MCresult -> sets(MCfalsestring);
        return ES_NORMAL;
    }
	t_success = MCParseParameters(p_parameters, "s", &t_body);
    
    MCExecContext t_ctxt(ep);
    
	MCComposeTextMessageExec(t_ctxt, t_recipients, t_body);
    
	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////