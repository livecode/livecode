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

#ifndef __MOBILE_CALENDAR__
#define __MOBILE_CALENDAR__

#include "mblsyntax.h"

typedef struct
{
    MCString   mceventid;           //
    MCString   mctitle;             //                                     RW
    MCString   mcnote;              //                                     RW
    MCString   mclocation;          //                                     RW
    bool       mcalldayset;         //
    bool       mcallday;            //                                     RW
    bool       mcstartdateset;      //
    MCDateTime mcstartdate;         //                                     RW
    bool       mcenddateset;        //
    MCDateTime mcenddate;           //                                     RW
    int        mcalert1;            //                                     RW
    int        mcalert2;            //                                     RW
    MCString   mcfrequency;         // EKReocurrenceFrequency              RW
    int        mcfrequencycount;    //                                     RW
    int        mcfrequencyinterval; //                                     RW
    MCString   mccalendar;          //
} MCCalendar;

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
void MCCalendarToArrayData (MCExecContext &r_ctxt, MCCalendar p_contact, MCVariableValue *&r_result);
bool MCSystemShowEvent(const char* p_event_id, char*& r_result);
bool MCSystemCreateEvent(char*& r_result);
bool MCSystemUpdateEvent(const char* p_event_id, char*& r_result);
bool MCSystemGetEventData(MCExecContext &r_ctxt, const char* p_event_id, MCVariableValue*& r_event_data);
bool MCSystemRemoveEvent(const char* p_event_id, bool p_reocurring, char*& r_event_id_deleted);
bool MCSystemAddEvent(MCCalendar p_new_calendar_data, char*& r_result);
bool MCSystemGetCalendarsEvent(char*& r_result);
bool MCSystemFindEvent(MCDateTime p_start_date, MCDateTime p_end_date, char*& r_result);

#endif
