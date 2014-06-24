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


////////////////////////////////////////////////////////////////////////////////

bool MCSystemBeep(int32_t p_number_of_times);
bool MCSystemVibrate(int32_t p_number_of_times);

////////////////////////////////////////////////////////////////////////////////

void MCBeepExec (MCExecContext& ctxt, int32_t p_number_of_times)
{
    MCSystemVibrate(p_number_of_times);
}

void MCVibrateExec (MCExecContext& ctxt, int32_t p_number_of_times)
{
    MCSystemVibrate(p_number_of_times);
}

Exec_stat MCHandleBeep(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleBeep */ LEGACY_EXEC
    int32_t t_number_of_times = 1;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_number_of_times = ep . getint4();
    }
    MCSystemBeep(t_number_of_times);
  	return ES_NORMAL;
#endif /* MCHandleBeep */
}

Exec_stat MCHandleVibrate(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleVibrate */ LEGACY_EXEC
    int32_t t_number_of_times = 1;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_number_of_times = ep . getint4();
    }
    MCSystemVibrate(t_number_of_times);
	return ES_NORMAL;
#endif /* MCHandleVibrate */
}
