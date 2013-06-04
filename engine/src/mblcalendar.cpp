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

#include "mblcalendar.h"

void MCShowEventExec(MCExecContext& p_ctxt, const char* p_event_id)
{
    char *t_result;
    t_result = nil;
    MCSystemShowEvent(p_event_id, t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
}

void MCCreateEventExec(MCExecContext& p_ctxt)
{
    char *t_result;
    t_result = nil;
    MCSystemCreateEvent(t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
}

void MCUpdateEventExec(MCExecContext& p_ctxt, const char* p_event_id)
{
    char *t_result;
    t_result = nil;
    MCSystemUpdateEvent(p_event_id, t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
}

void MCGetEventDataExec(MCExecContext& p_ctxt, const char* p_event_id)
{
    MCAutoArrayRef t_event_data;
    MCSystemGetEventData(p_ctxt, p_event_id, &t_event_data);
    if (*t_event_data == nil)
        p_ctxt.SetTheResultToEmpty();
    else
        p_ctxt.SetTheResultToValue(*t_event_data);
}

void MCRemoveEventExec(MCExecContext& p_ctxt, bool p_reocurring, const char* p_event_id)
{    
    char  *t_event_id_deleted;
    t_event_id_deleted = nil;
    MCSystemRemoveEvent (p_event_id, p_reocurring, t_event_id_deleted);
    if (t_event_id_deleted != nil)
        p_ctxt.SetTheResultToCString(t_event_id_deleted);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_event_id_deleted);
}

void MCAddEventExec(MCExecContext& p_ctxt, MCCalendar p_new_event_data)
{
    char *t_result;
    t_result = nil;
    MCSystemAddEvent(p_new_event_data, t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
}

void MCGetCalendarsEventExec(MCExecContext& p_ctxt)
{
    char *t_result;
    t_result = nil;
    MCSystemGetCalendarsEvent(t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
}

void MCFindEventExec(MCExecContext& p_ctxt, MCDateTime p_start_date, MCDateTime p_end_date)
{
    char *t_result;
    t_result = nil;
    MCSystemFindEvent(p_start_date, p_end_date, t_result);
    if (t_result != nil)
        p_ctxt.SetTheResultToCString(t_result);
    else
        p_ctxt.SetTheResultToEmpty();
    MCCStringFree(t_result);
}

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

Exec_stat MCHandleShowEvent(void *context, MCParameter *p_parameters)
{
    const char* t_event_id = NULL;
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_event_id = ep.getsvalue().getstring();
    }
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCShowEventExec(ctxt, t_event_id);
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleUpdateEvent(void *context, MCParameter *p_parameters)
{
    const char* t_event_id = NULL;
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_event_id = ep.getsvalue().getstring();
    }
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCUpdateEventExec(ctxt, t_event_id);
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCreateEvent(void *context, MCParameter *p_parameters)
{
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCCreateEventExec(ctxt);
    // Set return value
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleGetEventData(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    const char* t_event_id = NULL;
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_event_id = ep.getsvalue().getstring();
    }
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCGetEventDataExec(ctxt, t_event_id);
    if (MCresult->isempty())
	{
		MCAutoStringRef t_value;
		/* UNCHECKED */ ep . copyasstringref(&t_value);
        ctxt . SetTheResultToValue(*t_value);
	}

    if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleRemoveEvent(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    const char* t_event_id = NULL;
    bool t_reocurring = false;
    bool t_success = true;
    // Handle parameters.
    t_success = MCParseParameters(p_parameters, "s", &t_event_id);
    
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "b", &t_reocurring);
    }
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCRemoveEventExec(ctxt, t_reocurring, t_event_id);
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleAddEvent(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    // Handle parameters. We are doing that in a dedicated call
    MCCalendar t_new_event_data;
    t_new_event_data = MCParameterDataToCalendar(p_parameters, t_new_event_data);
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCAddEventExec(ctxt, t_new_event_data);
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleGetCalendarsEvent(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCGetCalendarsEventExec(ctxt);
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleFindEvent(void *context, MCParameter *p_parameters)
{
    MCDateTime t_start_date;
    MCDateTime t_end_date;
    bool t_success = true;
    const char *r_result = NULL;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_start_date);
        }
        p_parameters = p_parameters->getnext();
    }
    if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_end_date);
        }
    }
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCFindEventExec(ctxt, t_start_date, t_end_date);
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}