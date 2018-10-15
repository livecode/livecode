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

#include "mcerror.h"

#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"

#include "date.h"

#include "mbldc.h"

#include "mblandroidutil.h"
#include "mblandroidjava.h"

#include "mblsyntax.h"
#include "mblcalendar.h"

#include <string.h>

#include <jni.h>

typedef enum
{
    kMCAndroidCalendarEventWaiting,
    kMCAndroidCalendarEventDone,
    kMCAndroidCalendarEventCanceled,
} MCAndroidCalendarEventStatus;


static MCAndroidCalendarEventStatus s_calendar_event_status = kMCAndroidCalendarEventWaiting; 
static MCStringRef s_calendar_event_selected;
static MCStringRef s_calendar_events_selected;
static MCCalendar s_calendar_event_data;

bool MCAndroidCalendarInitialize(void)
{
	s_calendar_event_selected = MCValueRetain(kMCEmptyString);
	s_calendar_events_selected = MCValueRetain(kMCEmptyString);

    return true;
}

void MCAndroidCalendarFinalize(void)
{
	MCValueRelease(s_calendar_event_selected);
	MCValueRelease(s_calendar_events_selected);
}

bool MCSystemShowEvent(MCStringRef p_calendar_event_id, MCStringRef& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemShowCalendarEvent");
    MCAndroidEngineRemoteCall("showCalendarEvent", "vx", nil, p_calendar_event_id);
    s_calendar_event_status = kMCAndroidCalendarEventWaiting;
    while (s_calendar_event_status == kMCAndroidCalendarEventWaiting)
        MCscreen->wait(60.0, False, True);
	r_result = MCValueRetain(s_calendar_event_selected);
    MCLog("MCSystemShowCalendarEvent - finished");
    return true;
}

void MCAndroidShowCalendarEventDone(MCStringRef p_calendar_event_id)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    s_calendar_event_selected = MCValueRetain(p_calendar_event_id);
    MCLog("MCAndroidShowCalendarEventDone() called %@", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventDone;
}

void MCAndroidShowCalendarEventCanceled(MCStringRef p_calendar_event_id)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    s_calendar_event_selected = MCValueRetain(p_calendar_event_id);
    MCLog("MCAndroidShowCalendarEventCanceled() called %@", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventCanceled;
}

bool MCSystemCreateEvent(MCStringRef& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemCreateCalendarEvent");
    MCAndroidEngineRemoteCall("createCalendarEvent", "v", nil);
    s_calendar_event_status = kMCAndroidCalendarEventWaiting;
    while (s_calendar_event_status == kMCAndroidCalendarEventWaiting)
        MCscreen->wait(60.0, False, True);
    MCLog("MCSystemCreateCalendarEvent - finished");
    return true;
}

void MCAndroidCreateCalendarEventDone(MCStringRef p_calendar_event_id)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    s_calendar_event_selected = MCValueRetain(p_calendar_event_id);
    MCLog("MCAndroidCreateCalendarEventDone() called %@", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventDone;
}

void MCAndroidCreateCalendarEventCanceled(MCStringRef p_calendar_event_id)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    s_calendar_event_selected = MCValueRetain(p_calendar_event_id);
    MCLog("MCAndroidCreateCalendarEventCanceled() called %@", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventCanceled;
}

bool MCSystemUpdateEvent(MCStringRef p_new_calendar_event_data, MCStringRef& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemUpdateCalendarEvent");
    MCAndroidEngineRemoteCall("updateCalendarEvent", "vx", nil, p_new_calendar_event_data);
    s_calendar_event_status = kMCAndroidCalendarEventWaiting;
    while (s_calendar_event_status == kMCAndroidCalendarEventWaiting)
        MCscreen->wait(60.0, False, True);
    
	r_result = MCValueRetain(s_calendar_event_selected);
    MCLog("MCSystemUpdateCalendarEvent - finished");

    return true;
}

void MCAndroidUpdateCalendarEventDone(MCStringRef p_calendar_event_id)
{
// STILL TO BE IMPLEMENTED IPA LEVEL 14
    s_calendar_event_selected = MCValueRetain(p_calendar_event_id);
    MCLog("MCAndroidUpdateCalendarEventDone() called %@", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventDone;
}

void MCAndroidUpdateCalendarEventCanceled(MCStringRef p_calendar_event_id)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    s_calendar_event_selected = MCValueRetain(p_calendar_event_id);
    MCLog("MCAndroidUpdateCalendarEventCanceled() called %@", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventCanceled;
}

bool MCSystemGetEventData(MCExecContext &r_ctxt, MCStringRef p_calendar_event_id, MCArrayRef &r_calendar_event_data)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemGetEventData: %@", p_calendar_event_id);
    MCAndroidEngineRemoteCall("getCalendarEventData", "vx", nil, p_calendar_event_id);
    MCCalendarToArrayData (r_ctxt, s_calendar_event_data, r_calendar_event_data); 
    return true;
}

bool MCSystemRemoveEvent(MCStringRef p_calendar_event_id, bool p_reoccurring, MCStringRef& r_calendar_event_id_deleted)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemRemoveCalendarEvent: %@", p_calendar_event_id);
    MCAndroidEngineRemoteCall("removeCalendarEvent", "xx", r_calendar_event_id_deleted, p_calendar_event_id);
    
	r_calendar_event_id_deleted = MCValueRetain(s_calendar_event_selected);
    return true;
}

bool MCSystemAddEvent(MCCalendar p_new_calendar_event_data, MCStringRef& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemAddCalendarEvent");
    MCAndroidEngineRemoteCall("addCalendarEvent", "xxxxxbbbbiixiix", &r_result,
                              p_new_calendar_event_data.mceventid, p_new_calendar_event_data.mctitle,
                              p_new_calendar_event_data.mcnote, p_new_calendar_event_data.mclocation,
                              p_new_calendar_event_data.mcalldayset, p_new_calendar_event_data.mcallday,
                              p_new_calendar_event_data.mcstartdateset,
                              p_new_calendar_event_data.mcenddateset,
                              p_new_calendar_event_data.mcalert1, p_new_calendar_event_data.mcalert2, 
                              p_new_calendar_event_data.mcfrequency, p_new_calendar_event_data.mcfrequencycount,
                              p_new_calendar_event_data.mcfrequencyinterval, p_new_calendar_event_data.mccalendar);  

	r_result = MCValueRetain(s_calendar_event_selected);
    return true;
    
}

bool MCSystemFindEvent(MCDateTime p_start_date, MCDateTime p_end_date, MCStringRef& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
	r_result = MCValueRetain(s_calendar_events_selected);
    MCLog("MCSystemFindCalendarEvent result: %@", r_result);
    return true;
}

bool MCSystemGetCalendarsEvent(MCStringRef& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    return false;
}

// Get data from Java and assign the values to class values that are then returned to the
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowCalendarEvent(JNIEnv *env, jobject object, jstring p_calendar_event_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doGetCalendarEventData(JNIEnv *env, jobject object,
                                                                                        jstring p_eventid, jstring p_title,
                                                                                        jstring p_note, jstring p_location,
                                                                                        bool p_alldayset, bool p_allday,
                                                                                        bool p_startdateset, jint p_startdate, 
                                                                                        bool p_enddateset, jint p_enddate, 
                                                                                        jint p_alert1, jint p_alert2, 
                                                                                        jstring p_frequency, jint p_frequencycount,
                                                                                        jint p_frequencyinterval, jstring p_calendar) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUpdateCalendarEventDone(JNIEnv *env, jobject object, jstring p_calendar_event_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUpdateCalendarEventCanceled(JNIEnv *env, jobject object, jstring p_calendar_event_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreateCalendarEventDone(JNIEnv *env, jobject object, jstring p_calendar_event_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreateCalendarEventCanceled(JNIEnv *env, jobject object, jstring p_calendar_event_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowCalendarEventDone(JNIEnv *env, jobject object, jstring p_calendar_event_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowCalendarEventCanceled(JNIEnv *env, jobject object, jstring p_calendar_event_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doAddCalendarEvent(JNIEnv *env, jobject object, jstring p_calendar_event_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doFindCalendarEvent(JNIEnv *env, jobject object, jstring p_calendar_events_found) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRemoveCalendarEvent(JNIEnv *env, jobject object, jstring p_calendar_event_id) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doGetCalendarEventData(JNIEnv *env, jobject object,
                                                                             jstring p_eventid, jstring p_title,
                                                                             jstring p_note, jstring p_location,
                                                                             bool p_alldayset, bool p_allday,
                                                                             bool p_startdateset, int p_startdate, 
                                                                             bool p_enddateset, int p_enddate,
                                                                             jint p_alert1, jint p_alert2, 
                                                                             jstring p_frequency, jint p_frequencycount,
                                                                             jint p_frequencyinterval, jstring p_calendar)
{
    MCValueRelease(s_calendar_event_data.mceventid);
    MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_eventid, s_calendar_event_data.mceventid);
    
    MCValueRelease(s_calendar_event_data.mctitle);
	MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_title, s_calendar_event_data.mctitle);
    
    MCValueRelease(s_calendar_event_data.mcnote);
	MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_note, s_calendar_event_data.mcnote);
	
    MCValueRelease(s_calendar_event_data.mclocation);
	MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_location, s_calendar_event_data.mclocation);
    
    s_calendar_event_data.mcalert1 = p_alert1;
    s_calendar_event_data.mcalert2 = p_alert2;
    
    MCValueRelease(s_calendar_event_data.mcfrequency);
	MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_frequency, s_calendar_event_data.mcfrequency);
      
    s_calendar_event_data.mcfrequencycount = p_frequencycount;
    s_calendar_event_data.mcfrequencyinterval = p_frequencyinterval;
    
    MCValueRelease(s_calendar_event_data.mccalendar);
	MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_calendar, s_calendar_event_data.mccalendar);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUpdateCalendarEventDone(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    MCAutoStringRef t_mcstring;
    MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_calendar_event_id, &t_mcstring);
    MCAndroidUpdateCalendarEventDone(*t_mcstring);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUpdateCalendarEventCanceled(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    MCAutoStringRef t_mcstring;
    MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_calendar_event_id, &t_mcstring);
    MCAndroidUpdateCalendarEventCanceled(*t_mcstring);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreateCalendarEventDone(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    MCAutoStringRef t_mcstring;
    MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_calendar_event_id, &t_mcstring);
    MCAndroidCreateCalendarEventDone(*t_mcstring);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreateCalendarEventCanceled(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    MCAutoStringRef t_mcstring;
    MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_calendar_event_id, &t_mcstring);
    MCAndroidCreateCalendarEventCanceled(*t_mcstring);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowCalendarEventDone(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    MCAutoStringRef t_mcstring;
    MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_calendar_event_id, &t_mcstring);
    MCAndroidShowCalendarEventDone(*t_mcstring);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowCalendarEventCanceled(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    MCAutoStringRef t_mcstring;
    MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_calendar_event_id, &t_mcstring);
    MCAndroidShowCalendarEventCanceled(*t_mcstring);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doAddCalendarEvent(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_calendar_event_id, s_calendar_event_selected);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doFindCalendarEvent(JNIEnv *env, jobject object, jstring p_calendar_events_found)
{
	MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_calendar_events_found, s_calendar_events_selected);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRemoveCalendarEvent(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    MCJavaStringToStringRef(MCJavaGetThreadEnv(), p_calendar_event_id, s_calendar_event_selected);
}
