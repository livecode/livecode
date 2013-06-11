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

#include "mcerror.h"
#include "globals.h"
#include "exec.h"
#include "param.h"

#include "mblcalendar.h"


////////////////////////////////////////////////////////////////////////////////


MC_EXEC_DEFINE_EXEC_METHOD(Calendar, ShowEvent, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Calendar, CreateEvent, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Calendar, UpdateEvent, 1)
MC_EXEC_DEFINE_GET_METHOD(Calendar, EventData, 2)


////////////////////////////////////////////////////////////////////////////////

bool MCSystemCreateEvent(MCStringRef& r_result);

////////////////////////////////////////////////////////////////////////////////


void MCCalendarExecShowEvent(MCExecContext& ctxt, MCStringRef p_id)
{
    MCAutoStringRef t_result;    
    if(MCSystemShowEvent(p_id, &t_result))
    {
        ctxt.SetTheResultToValue(*t_result);
        return;
    }
    
    ctxt.SetTheResultToEmpty();
    ctxt.Throw();
}

void MCCalendarExecCreateEvent(MCExecContext& ctxt)
{
    MCAutoStringRef t_result;

    if (MCSystemCreateEvent(&t_result))
    {
        ctxt.SetTheResultToValue(*t_result);
        return;
    }
    
    ctxt.SetTheResultToEmpty();
    ctxt.Throw();
}

void MCCalendarExecUpdateEvent(MCExecContext& ctxt, MCStringRef p_id)
{
    MCAutoStringRef t_result;
    
    if(MCSystemUpdateEvent(p_id, &t_result))
    {
        ctxt.SetTheResultToValue(*t_result);
        return;
    }
    
    ctxt.SetTheResultToEmpty();
    return;
}

void MCCalendarGetEventData(MCExecContext& ctxt, MCStringRef p_id, MCArrayRef& r_data)
{
    MCSystemGetEventData(ctxt, p_id, r_data);
    if (r_data == nil)
        ctxt.SetTheResultToEmpty();
    else
        ctxt.SetTheResultToValue(r_data);
}

void MCCalendarExecRemoveEvent(MCExecContext& ctxt, MCStringRef p_id, bool p_reocurring)
{
    MCAutoStringRef t_event_id_deleted;
    
    if(MCSystemRemoveEvent (p_id, p_reocurring, &t_event_id_deleted))
    {
        ctxt.SetTheResultToValue(*t_event_id_deleted);
        return;
    }
    
    ctxt.SetTheResultToEmpty();
    ctxt.Throw();
}

void MCCalendarExecAddEvent(MCExecContext& ctxt, MCCalendar p_new_event_data, MCStringRef p_id)
{
    MCAutoStringRef t_result;
    
    if (MCSystemAddEvent(p_new_event_data, &t_result))
    {
        ctxt.SetTheResultToValue(*t_result);
        return;
    }
    
    ctxt.SetTheResultToEmpty();
    ctxt.Throw();
}

void MCGetCalendarsEventExec(MCExecContext& ctxt)
{
    MCAutoStringRef t_result;
    
    if (MCSystemGetCalendarsEvent(&t_result))
    {
        ctxt.SetTheResultToValue(*t_result);
        return;
    }
    
    ctxt.SetTheResultToEmpty();
    ctxt.Throw();
}

void MCFindEventExec(MCExecContext& ctxt, MCDateTime p_start_date, MCDateTime p_end_date)
{
    MCAutoStringRef t_result;
    
    if (MCSystemFindEvent(p_start_date, p_end_date, &t_result))
    {
        ctxt.SetTheResultToValue(*t_result);
        return;
    }
    
    ctxt.SetTheResultToEmpty();
    ctxt.Throw();
}


////////////////////////////////////////////////////////////////////////////////


