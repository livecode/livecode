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


void MCCalendarExecShowEvent(MCExecContext& ctxt, MCStringRef p_id)
{
    char *t_result;
    t_result = nil;
    
    MCSystemShowEvent(MCStringGetCString(p_id), t_result);
    
    if (t_result != nil)
        ctxt.SetTheResultToCString(t_result);
    else
        ctxt.SetTheResultToEmpty();
    
    MCCStringFree(t_result);
}

void MCcalendarExecCreateEvent(MCExecContext& ctxt)
{
    char *t_result;
    t_result = nil;
    
    MCSystemCreateEvent(t_result);
    if (t_result != nil)
        ctxt.SetTheResultToCString(t_result);
    else
        ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
}

void MCCalendarExecUpdateEvent(MCExecContext& ctxt, MCStringRef p_id)
{
    char *t_result;
    t_result = nil;
    MCSystemUpdateEvent(MCStringGetCString(p_id), t_result);
    if (t_result != nil)
        ctxt.SetTheResultToCString(t_result);
    else
        ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
}

void MCCalendarGetEventData(MCExecContext& ctxt, MCStringRef p_id, MCArrayRef& r_data)
{
    MCSystemGetEventData(ctxt, MCStringGetCString(p_id), r_data);
    if (r_data == nil)
        ctxt.SetTheResultToEmpty();
    else
        ctxt.SetTheResultToValue(r_data);
}

//void MCCalendarExecRemoveEvent(MCExecContext& ctxt, MCStringRef p_id)
//{
//    char  *t_event_id_deleted;
//    t_event_id_deleted = nil;
//    MCSystemRemoveEvent (p_event_id, p_reocurring, t_event_id_deleted);
//    if (t_event_id_deleted != nil)
//        p_ctxt.SetTheResultToCString(t_event_id_deleted);
//    else
//        p_ctxt.SetTheResultToEmpty();
//    MCCStringFree(t_event_id_deleted);
//}

//void MCCalendarExecAddEvent(MCExecContext& ctxt, MCStringRef p_id)
//{
//    char *t_result;
//    t_result = nil;
//    MCSystemAddEvent(p_new_event_data, t_result);
//    if (t_result != nil)
//        p_ctxt.SetTheResultToCString(t_result);
//    else
//        p_ctxt.SetTheResultToEmpty();
//    MCCStringFree(t_result);
//}
//
//void MCGetCalendarsEventExec(MCExecContext& p_ctxt)
//{
//    char *t_result;
//    t_result = nil;
//    MCSystemGetCalendarsEvent(t_result);
//    if (t_result != nil)
//        p_ctxt.SetTheResultToCString(t_result);
//    else
//        p_ctxt.SetTheResultToEmpty();
//    MCCStringFree(t_result);
//}
//
//void MCFindEventExec(MCExecContext& p_ctxt, MCDateTime p_start_date, MCDateTime p_end_date)
//{
//    char *t_result;
//    t_result = nil;
//    MCSystemFindEvent(p_start_date, p_end_date, t_result);
//    if (t_result != nil)
//        p_ctxt.SetTheResultToCString(t_result);
//    else
//        p_ctxt.SetTheResultToEmpty();
//    MCCStringFree(t_result);
//}


////////////////////////////////////////////////////////////////////////////////


