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
#include "param.h"

#include "mblcalendar.h"


////////////////////////////////////////////////////////////////////////////////


MC_EXEC_DEFINE_EXEC_METHOD(Calendar, ShowEvent, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Calendar, CreateEvent, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Calendar, UpdateEvent, 1)
MC_EXEC_DEFINE_GET_METHOD(Calendar, EventData, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Calendar, RemoveEvent, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Calendar, AddEvent, 2)
MC_EXEC_DEFINE_GET_METHOD(Calendar, CalendarEvent, 0)


////////////////////////////////////////////////////////////////////////////////

bool MCSystemCreateEvent(MCStringRef& r_result);

////////////////////////////////////////////////////////////////////////////////


void MCCalendarExecShowEvent(MCExecContext& ctxt, MCStringRef p_id)
{
#ifdef /* MCShowEventExec */ LEGACY_EXEC
    char *t_result;
    t_result = nil;
    MCSystemShowEvent(p_event_id, t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
#endif /* MCShowEventExec */
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
#ifdef /* MCCreateEventExec */ LEGACY_EXEC
    char *t_result;
    t_result = nil;
    MCSystemCreateEvent(t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
#endif /* MCCreateEventExec */
    MCAutoStringRef t_result;
    
    bool t = MCSystemCreateEvent(&t_result);

    if (t)
    {
        ctxt.SetTheResultToValue(*t_result);
        return;
    }
    
    ctxt.SetTheResultToEmpty();
    ctxt.Throw();
}

void MCCalendarExecUpdateEvent(MCExecContext& ctxt, MCStringRef p_id)
{
#ifdef /* MCUpdateEventExec */ LEGACY_EXEC
    char *t_result;
    t_result = nil;
    MCSystemUpdateEvent(p_event_id, t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
#endif /* MCUpdateEventExec */
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
#ifdef /* MCGetEventDataExec */ LEGACY_EXEC
    MCVariableValue *r_event_data = nil;
    MCSystemGetEventData(p_ctxt, p_event_id, r_event_data);
    if (r_event_data == nil)
        p_ctxt.SetTheResultToEmpty();
    else
        p_ctxt.GetEP().setarray(r_event_data, True);
#endif /* MCGetEventDataExec */
    MCSystemGetEventData(ctxt, p_id, r_data);
    if (r_data == nil)
        ctxt.SetTheResultToEmpty();
    else
        ctxt.SetTheResultToValue(r_data);
}

void MCCalendarExecRemoveEvent(MCExecContext& ctxt, MCStringRef p_id, bool p_reocurring)
{
#ifdef /* MCRemoveEventExec */ LEGACY_EXEC
    char  *t_event_id_deleted;
    t_event_id_deleted = nil;
    MCSystemRemoveEvent (p_event_id, p_reocurring, t_event_id_deleted);
    if (t_event_id_deleted != nil)
        p_ctxt.SetTheResultToCString(t_event_id_deleted);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_event_id_deleted);
#endif /* MCRemoveEventExec */
    MCAutoStringRef t_event_id_deleted;
    
    if(MCSystemRemoveEvent (p_id, p_reocurring, &t_event_id_deleted))
    {
        ctxt.SetTheResultToValue(*t_event_id_deleted);
        return;
    }
    
    ctxt.SetTheResultToEmpty();
    ctxt.Throw();
}

void MCCalendarExecAddEvent(MCExecContext& ctxt, MCArrayRef p_data)
{
#ifdef /* MCAddEventExec */ LEGACY_EXEC
    char *t_result;
    t_result = nil;
    MCSystemAddEvent(p_new_event_data, t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
#endif /* MCAddEventExec */
    MCAutoStringRef t_result;
    MCCalendar t_new_event_data;
    bool t_success;
    
    t_success = MCArrayDataToCalendar(p_data, t_new_event_data);
    
    if (t_success)
    {
        if (MCSystemAddEvent(t_new_event_data, &t_result))
        {
            ctxt.SetTheResultToValue(*t_result);
            return;
        }
    }
    
    ctxt.SetTheResultToEmpty();
    ctxt.Throw();
}

void MCCalendarGetCalendars(MCExecContext& ctxt)
{
#ifdef /* MCGetCalendarsEventExec */ LEGACY_EXEC
    char *t_result;
    t_result = nil;
    MCSystemGetCalendarsEvent(t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
#endif /* MCGetCalendarsEventExec */
    MCAutoStringRef t_result;
    
    if (MCSystemGetCalendarsEvent(&t_result))
    {
        ctxt.SetTheResultToValue(*t_result);
        return;
    }
    
    ctxt.SetTheResultToEmpty();
    ctxt.Throw();
}

void MCCalendarExecFindEvent(MCExecContext& ctxt, MCDateTime p_start_date, MCDateTime p_end_date)
{
#ifdef /* MCFindEventExec */ LEGACY_EXEC
    char *t_result;
    t_result = nil;
    MCSystemFindEvent(p_start_date, p_end_date, t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
#endif /* MCFindEventExec */
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


