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

#include "mcerror.h"
#include "globals.h"
#include "exec.h"
#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////

void MCTextMessagingGetCanComposeTextMessage(MCExecContext& ctxt, bool& r_result)
{
    r_result = MCSystemCanSendTextMessage();
}

void MCTextMessagingExecComposeTextMessage(MCExecContext& ctxt, MCStringRef p_recipients, MCStringRef p_body)
{
    if (!MCSystemCanSendTextMessage())
        ctxt . SetTheResultToValue(kMCFalse);
    else
        MCSystemComposeTextMessage(p_recipients, p_body);
}
