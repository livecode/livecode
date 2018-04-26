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

#include "globals.h"
#include "stack.h"
#include "image.h"
#include "param.h"
#include "exec.h"

#include "mblsyntax.h"

#include "mblcalendar.h"

////////////////////////////////////////////////////////////////////////////////

bool MCArrayDataToCalendar (MCArrayRef p_array, MCCalendar& r_calendar)
{
    bool t_success;
    
    MCValueRelease(r_calendar.mcnote);
    MCValueRelease(r_calendar.mctitle);
    MCValueRelease(r_calendar.mclocation);
    MCValueRelease(r_calendar.mccalendar);
    MCValueRelease(r_calendar.mcfrequency);    
    
    t_success = MCArrayGetCount(p_array) > 0;
    
    if (t_success)
    {
        MCValueRef t_str_allday;
    
        t_success = MCArrayFetchValue(p_array, false, MCNAME("allday"), t_str_allday);
        
        if (t_success)
        {
            if (MCStringIsEqualTo((MCStringRef)t_str_allday, kMCTrueString, kMCCompareCaseless))
                r_calendar.mcallday = true;
            else
                r_calendar.mcallday = false;
        }
    }
    
    if (t_success)
    {
        MCValueRef t_str_note;
        
        t_success = MCArrayFetchValue(p_array, false, MCNAME("note"), t_str_note);
        
        if (t_success)
            t_success = MCStringCopy((MCStringRef)t_str_note, r_calendar.mcnote);
    }
    
    if (t_success)
    {
        MCValueRef t_str_title;
        
        t_success = MCArrayFetchValue(p_array, false, MCNAME("title"), t_str_title);
        
        if (t_success)
            t_success = MCStringCopy((MCStringRef)t_str_title, r_calendar.mctitle);
    }
    
    if (t_success)
    {
        MCValueRef t_str_location;
        
        t_success = MCArrayFetchValue(p_array, false, MCNAME("location"), t_str_location);
        
        if (t_success)
            t_success = MCStringCopy((MCStringRef)t_str_location, r_calendar.mclocation);
    }
    
    if (t_success)
    {
        MCValueRef t_str_calendar;
        
        t_success = MCArrayFetchValue(p_array, false, MCNAME("calendar"), t_str_calendar);
        
        if (t_success)
            t_success = MCStringCopy((MCStringRef)t_str_calendar, r_calendar.mccalendar);
    }
    
    if (t_success)
    {
        MCValueRef t_str_frequency;
        MCValueRef t_int_frequency_count;
        MCValueRef t_int_frequency_interval;
        
        t_success = MCArrayFetchValue(p_array, false, MCNAME("frequency"), t_str_frequency);
        
        if (t_success)
            t_success = MCStringCopy((MCStringRef)t_str_frequency, r_calendar.mcfrequency);
        
        if (t_success)
            t_success = MCArrayFetchValue(p_array, false, MCNAME("frequencycount"), t_int_frequency_count);
            
        if (t_success)
            r_calendar.mcfrequencycount = MCNumberFetchAsInteger((MCNumberRef)t_int_frequency_count);
        
        if (t_success)
            t_success = MCArrayFetchValue(p_array, false, MCNAME("frequencyinterval"), t_int_frequency_interval);
        
        if (t_success)
            r_calendar.mcfrequencyinterval = MCNumberFetchAsInteger((MCNumberRef)t_int_frequency_interval);
    }
    
    if (t_success)
    {
        MCValueRef t_int_alert1;
        
        t_success = MCArrayFetchValue(p_array, false, MCNAME("alert1"), t_int_alert1);
        
        if (t_success)
            r_calendar.mcalert1 = MCNumberFetchAsInteger((MCNumberRef)t_int_alert1);
    }
    
    if (t_success)
    {
        MCValueRef t_int_alert2;
        
        t_success = MCArrayFetchValue(p_array, false, MCNAME("alert2"), t_int_alert2);
        
        if (t_success)
            r_calendar.mcalert2 = MCNumberFetchAsInteger((MCNumberRef)t_int_alert2);
    }
    
    if (t_success)
    {
        MCValueRef t_int_startdate;
        
        t_success = MCArrayFetchValue(p_array, false, MCNAME("startdate"), t_int_startdate);
        
        if (t_success)
        {
            MCExecContext ctxt(nil, nil, nil);
			
            t_success = MCD_convert_to_datetime(ctxt, (MCNumberRef)t_int_startdate, CF_SECONDS, CF_SECONDS, r_calendar.mcstartdate);
        }
    }
    
    if (t_success)
    {
        MCValueRef t_int_enddate;
        
        t_success = MCArrayFetchValue(p_array, false, MCNAME("enddate"), t_int_enddate);
        
        if (t_success)
        {
            MCExecContext ctxt(nil, nil, nil);
			
            t_success = MCD_convert_to_datetime(ctxt, t_int_enddate, CF_SECONDS, CF_SECONDS, r_calendar.mcenddate);
        }
    }
    
    return t_success;
}


bool MCCalendarToArrayData (MCExecContext &ctxt, MCCalendar p_calendar, MCArrayRef &r_result)
{
    bool t_success = false;
    
    if (!MCArrayIsMutable(r_result))
        t_success = MCArrayCreateMutable(r_result);
    
    if (t_success)
    {
        MCAutoStringRef t_str_allday;
        
        if (p_calendar.mcallday == true)
            t_success = MCStringCopy(kMCTrueString, &t_str_allday);
        else
            t_success = MCStringCopy(kMCFalseString, &t_str_allday);
        
        t_success = MCArrayStoreValue(r_result, false, MCNAME("allday"), *t_str_allday);
    }
    if (t_success)
    {
        if (MCStringGetLength(p_calendar.mcnote) > 0)
        {
            t_success = MCArrayStoreValue(r_result, false, MCNAME("note"), p_calendar.mcnote);
        }
    }
    if (t_success)
    {
        if (MCStringGetLength(p_calendar.mctitle) > 0)
        {
            t_success = MCArrayStoreValue(r_result, false, MCNAME("title"), p_calendar.mctitle);
        }
    }
    if (t_success)
    {
        if (MCStringGetLength(p_calendar.mclocation) > 0)
        {
            MCArrayStoreValue(r_result, false, MCNAME("location"), p_calendar.mclocation);
        }
    }
    if (t_success)
    {
        MCAutoStringRef t_str_calendar;
        
        if (MCStringGetLength(p_calendar.mccalendar) > 0)
        {
            t_success = MCArrayStoreValue(r_result, false, MCNAME("calendar"), p_calendar.mccalendar);
        }
    }
    
    if (t_success)
    {
        MCAutoNumberRef t_int_frequency_count;
        MCAutoNumberRef t_int_frequency_interval;
        
        if (MCStringGetLength(p_calendar.mcfrequency) > 0)
        {
            t_success = MCArrayStoreValue(r_result, false, MCNAME("frequency"), p_calendar.mcfrequency);
            
            if (t_success)
                t_success = MCNumberCreateWithInteger(p_calendar.mcfrequencycount, &t_int_frequency_count);
            
            if (t_success)
                t_success = MCArrayStoreValue(r_result, false, MCNAME("frequencycount"), *t_int_frequency_count);
            
            if (t_success)
                t_success = MCNumberCreateWithInteger(p_calendar.mcfrequencyinterval, &t_int_frequency_interval);
            
            if (t_success)
                t_success = MCArrayStoreValue(r_result, false, MCNAME("frequencyinterval"), *t_int_frequency_interval);
        }
    }
    if (t_success)
    {
        MCAutoNumberRef t_int_alert1;
        
        if (p_calendar.mcalert1 >= 0)
        {
            t_success = MCNumberCreateWithInteger(p_calendar.mcalert1, &t_int_alert1);
            
            if (t_success)
                t_success = MCArrayStoreValue(r_result, false, MCNAME("alert1"), *t_int_alert1);
        }
    }
    
    if (t_success)
    {
        MCAutoNumberRef t_int_alert2;
        
        if (p_calendar.mcalert2 >= 0)
        {
            t_success = MCNumberCreateWithInteger(p_calendar.mcalert2, &t_int_alert2);
            
            if (t_success)
                t_success = MCArrayStoreValue(r_result, false, MCNAME("alert2"), *t_int_alert2);
        }
    }
    
    if (t_success)
    {
        // Convert the start date to seconds
		MCAutoValueRef t_time;
        if (MCD_convert_from_datetime(ctxt, p_calendar.mcstartdate, CF_SECONDS, CF_SECONDS, &t_time))
        {
            MCAutoNumberRef t_int_startdate;
            
            t_success = ctxt.ConvertToNumber(*t_time, &t_int_startdate);
            
            if (t_success)
                t_success = MCArrayStoreValue(r_result, false, MCNAME("startdate"), *t_int_startdate);
        }
    }
    
    if (t_success)
    {
        // Convert the start date to seconds
		MCAutoValueRef t_time;
        if (MCD_convert_from_datetime(ctxt, p_calendar.mcenddate, CF_SECONDS, CF_SECONDS, &t_time))
        {
            MCAutoNumberRef t_int_enddate;
            
            t_success = ctxt.ConvertToNumber(*t_time, &t_int_enddate);
            
            if (t_success)
                t_success = MCArrayStoreValue(r_result, false, MCNAME("enddate"), *t_int_enddate);
        }
    }
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////
