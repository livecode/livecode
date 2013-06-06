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

MCCalendar MCParameterDataToCalendar (MCParameter *p_parameters, MCCalendar p_result)
{
  	MCExecPoint ep(nil, nil, nil);
	bool t_success = true;
	char *t_property_name = NULL;
	char *t_property_value_s = NULL;
	bool t_property_value_b;
    int32_t t_property_value_d;
    
    // Pre-define some defaults
    p_result.mcallday = false;
    p_result.mcalldayset = false;
    p_result.mcstartdateset = false;
    p_result.mcenddateset = false;
    p_result.mcalert1 = -1;
    p_result.mcalert2 = -1;
    p_result.mcfrequencycount = 1;
    p_result.mcfrequencyinterval = 0;
    
	while (t_success)
	{
		t_success = MCParseParameters(p_parameters, "s", &t_property_name);
        if (t_success)
        {
			if (MCCStringEqualCaseless(t_property_name, "allday"))
            {
                t_success = MCParseParameters(p_parameters, "b", &t_property_value_b);
                p_result.mcallday = t_property_value_b;
                p_result.mcalldayset = true;
            }
            else if (MCCStringEqualCaseless(t_property_name, "eventid"))
            {
                t_success = MCParseParameters(p_parameters, "s", &t_property_value_s);
                p_result.mceventid = t_property_value_s;
            }
            else if (MCCStringEqualCaseless(t_property_name, "note"))
            {
                t_success = MCParseParameters(p_parameters, "s", &t_property_value_s);
                p_result.mcnote = t_property_value_s;
            }
            else if (MCCStringEqualCaseless(t_property_name, "title"))
            {
                t_success = MCParseParameters(p_parameters, "s", &t_property_value_s);
                p_result.mctitle = t_property_value_s;
            }
            else if (MCCStringEqualCaseless(t_property_name, "calendar"))
            {
                t_success = MCParseParameters(p_parameters, "s", &t_property_value_s);
                p_result.mccalendar = t_property_value_s;
            }
            else if (MCCStringEqualCaseless(t_property_name, "location"))
            {
                t_success = MCParseParameters(p_parameters, "s", &t_property_value_s);
                p_result.mclocation = t_property_value_s;
            }
            else if (MCCStringEqualCaseless(t_property_name, "frequency"))
            {
                t_success = MCParseParameters(p_parameters, "s", &t_property_value_s);
                p_result.mcfrequency = t_property_value_s;
            }
            else if (MCCStringEqualCaseless(t_property_name, "frequencycount"))
            {
                t_success = MCParseParameters(p_parameters, "i", &t_property_value_d);
                p_result.mcfrequencycount = t_property_value_d;
            }
            else if (MCCStringEqualCaseless(t_property_name, "frequencyinterval"))
            {
                t_success = MCParseParameters(p_parameters, "i", &t_property_value_d);
                p_result.mcfrequencyinterval = t_property_value_d;
            }
            else if (MCCStringEqualCaseless(t_property_name, "alert1"))
            {
                t_success = MCParseParameters(p_parameters, "i", &t_property_value_d);
                p_result.mcalert1 = t_property_value_d;
            }
            else if (MCCStringEqualCaseless(t_property_name, "alert2"))
            {
                t_success = MCParseParameters(p_parameters, "i", &t_property_value_d);
                p_result.mcalert2 = t_property_value_d;
            }
            else if (MCCStringEqualCaseless(t_property_name, "startdate"))
            {
                p_parameters->eval(ep);
                if (!ep.isempty())
                {
                    t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, p_result.mcstartdate);
                    p_result.mcstartdateset = true;
                }
                t_success = MCParseParameters(p_parameters, "s", &t_property_name);
            }
            else if (MCCStringEqualCaseless(t_property_name, "enddate"))
            {
                p_parameters->eval(ep);
                if (!ep.isempty())
                {
                    t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, p_result.mcenddate);
                    p_result.mcenddateset = true;
                }
                t_success = MCParseParameters(p_parameters, "s", &t_property_name);
            }
        }
    }
    return p_result;
}

void MCCalendarToArrayData (MCExecContext &r_ctxt, MCCalendar p_calendar, MCArrayRef &r_result)
{
    // TODO - use array instead of MCVariableValue
    
    /*   MCExecPoint ep(nil, nil, nil);
     MCVariableValue *t_entry = nil;
     r_result = new MCVariableValue ();
     char t_int_string[11];
     if (r_result != NULL)
     {
     r_result->lookup_element(r_ctxt.GetEP(), "allday", t_entry);
     if (p_calendar.mcallday == true)
     t_entry->assign_string("true");
     else
     t_entry->assign_string("false");
     if (p_calendar.mcnote.getlength() > 0){
     r_result->lookup_element(r_ctxt.GetEP(), "note", t_entry);
     t_entry->assign_string(p_calendar.mcnote);}
     if (p_calendar.mctitle.getlength() > 0){
     r_result->lookup_element(r_ctxt.GetEP(), "title", t_entry);
     t_entry->assign_string(p_calendar.mctitle);}
     if (p_calendar.mclocation.getlength() > 0){
     r_result->lookup_element(r_ctxt.GetEP(), "location", t_entry);
     t_entry->assign_string(p_calendar.mclocation);}
     if (p_calendar.mccalendar.getlength() > 0){
     r_result->lookup_element(r_ctxt.GetEP(), "calendar", t_entry);
     t_entry->assign_string(p_calendar.mccalendar);}
     if (p_calendar.mcfrequency.getlength() > 0){
     r_result->lookup_element(r_ctxt.GetEP(), "frequency", t_entry);
     t_entry->assign_string(p_calendar.mcfrequency);
     r_result->lookup_element(r_ctxt.GetEP(), "fequencycount", t_entry);
     sprintf(t_int_string, "%d", p_calendar.mcfrequencycount);
     t_entry->assign_string(t_int_string);
     r_result->lookup_element(r_ctxt.GetEP(), "fequencyinterval", t_entry);
     sprintf(t_int_string, "%d", p_calendar.mcfrequencyinterval);
     t_entry->assign_string(t_int_string);}
     if (p_calendar.mcalert1 >=0)
     {
     r_result->lookup_element(r_ctxt.GetEP(), "alert1", t_entry);
     sprintf(t_int_string, "%d", p_calendar.mcalert1);
     t_entry->assign_string(t_int_string);
     }
     if (p_calendar.mcalert2 >=0)
     {
     r_result->lookup_element(r_ctxt.GetEP(), "alert2", t_entry);
     sprintf(t_int_string, "%d", p_calendar.mcalert2);
     t_entry->assign_string(t_int_string);
     }
     // Convert the start date to seconds
     int32_t t_secs;
     char t_secs_string[11];
     if (MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, p_calendar.mcstartdate))
     {
     t_secs = ep.getnvalue();
     sprintf (t_secs_string, "%d", t_secs);
     r_result->lookup_element(r_ctxt.GetEP(), "startdate", t_entry);
     t_entry->assign_string(t_secs_string);
     }
     // Convert the end date to seconds
     if (MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, p_calendar.mcenddate))
     {
     t_secs = ep.getnvalue();
     sprintf (t_secs_string, "%d", t_secs);
     r_result->lookup_element(r_ctxt.GetEP(), "enddate", t_entry);
     t_entry->assign_string(t_secs_string);
     }
     }*/
}


////////////////////////////////////////////////////////////////////////////////


