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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"
#include "execpt.h"
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
static MCString s_calendar_event_selected = "";
static MCString s_calendar_events_selected = "";
static MCCalendar s_calendar_event_data;

bool MCSystemShowEvent(const char* p_calendar_event_id, char*& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemShowCalendarEvent", NULL);
    MCAndroidEngineRemoteCall("showCalendarEvent", "vs", nil, p_calendar_event_id);
    s_calendar_event_status = kMCAndroidCalendarEventWaiting;
    while (s_calendar_event_status == kMCAndroidCalendarEventWaiting)
        MCscreen->wait(60.0, False, True);
    r_result = s_calendar_event_selected.clone();
    MCLog("MCSystemShowCalendarEvent - finished", NULL);
    return true;
}

void MCAndroidShowCalendarEventDone(const char* p_calendar_event_id)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    s_calendar_event_selected = p_calendar_event_id;
    MCLog("MCAndroidShowCalendarEventDone() called %s", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventDone;
}

void MCAndroidShowCalendarEventCanceled(const char* p_calendar_event_id)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    s_calendar_event_selected = p_calendar_event_id;
    MCLog("MCAndroidShowCalendarEventCanceled() called %s", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventCanceled;
}

bool MCSystemCreateEvent(char*& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemCreateCalendarEvent", NULL);
    MCAndroidEngineRemoteCall("createCalendarEvent", "v", nil);
    s_calendar_event_status = kMCAndroidCalendarEventWaiting;
    while (s_calendar_event_status == kMCAndroidCalendarEventWaiting)
        MCscreen->wait(60.0, False, True);
    MCLog("MCSystemCreateCalendarEvent - finished", NULL);
    return true;
}

void MCAndroidCreateCalendarEventDone(const char* p_calendar_event_id)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    s_calendar_event_selected = p_calendar_event_id;
    MCLog("MCAndroidCreateCalendarEventDone() called %s", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventDone;
}

void MCAndroidCreateCalendarEventCanceled(const char* p_calendar_event_id)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    s_calendar_event_selected = p_calendar_event_id;
    MCLog("MCAndroidCreateCalendarEventCanceled() called %s", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventCanceled;
}

bool MCSystemUpdateEvent(const char* p_new_calendar_event_data, char*& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemUpdateCalendarEvent", NULL);
    MCAndroidEngineRemoteCall("updateCalendarEvent", "vs", nil, p_new_calendar_event_data);
    s_calendar_event_status = kMCAndroidCalendarEventWaiting;
    while (s_calendar_event_status == kMCAndroidCalendarEventWaiting)
        MCscreen->wait(60.0, False, True);
    r_result = s_calendar_event_selected.clone();
    MCLog("MCSystemUpdateCalendarEvent - finished", NULL);
    return true;
}

void MCAndroidUpdateCalendarEventDone(const char* p_calendar_event_id)
{
// STILL TO BE IMPLEMENTED IPA LEVEL 14
    s_calendar_event_selected = p_calendar_event_id;
    MCLog("MCAndroidUpdateCalendarEventDone() called %s", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventDone;
}

void MCAndroidUpdateCalendarEventCanceled(const char* p_calendar_event_id)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    s_calendar_event_selected = p_calendar_event_id;
    MCLog("MCAndroidUpdateCalendarEventCanceled() called %s", p_calendar_event_id);
	s_calendar_event_status = kMCAndroidCalendarEventCanceled;
}

bool MCSystemGetEventData(MCExecContext &r_ctxt, const char* p_calendar_event_id, MCVariableValue *&r_calendar_event_data)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemGetEventData: %s", p_calendar_event_id);
    MCAndroidEngineRemoteCall("getCalendarEventData", "vs", nil, p_calendar_event_id);
    MCCalendarToArrayData (r_ctxt, s_calendar_event_data, r_calendar_event_data); 
    return true;
}

bool MCSystemRemoveEvent(const char* p_calendar_event_id, bool p_reoccurring, char*& r_calendar_event_id_deleted)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemRemoveCalendarEvent: %s", p_calendar_event_id);
    MCAndroidEngineRemoteCall("removeCalendarEvent", "ss", &r_calendar_event_id_deleted, p_calendar_event_id);
    r_calendar_event_id_deleted = s_calendar_event_selected.clone();
    return true;
}

bool MCSystemAddEvent(MCCalendar p_new_calendar_event_data, char*& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    MCLog("MCSystemAddCalendarEvent", NULL);
    MCAndroidEngineRemoteCall("addCalendarEvent", "issssbbbbiisiis", &r_result,
                              p_new_calendar_event_data.mceventid.getstring(), p_new_calendar_event_data.mctitle.getstring(),
                              p_new_calendar_event_data.mcnote.getstring(), p_new_calendar_event_data.mclocation.getstring(),
                              p_new_calendar_event_data.mcalldayset, p_new_calendar_event_data.mcallday,
                              p_new_calendar_event_data.mcstartdateset,
                              p_new_calendar_event_data.mcenddateset,
                              p_new_calendar_event_data.mcalert1, p_new_calendar_event_data.mcalert2, 
                              p_new_calendar_event_data.mcfrequency.getstring(), p_new_calendar_event_data.mcfrequencycount,
                              p_new_calendar_event_data.mcfrequencyinterval, p_new_calendar_event_data.mccalendar.getstring());    
    r_result = s_calendar_event_selected.clone();
    return true;
}

bool MCSystemFindEvent(MCDateTime p_start_date, MCDateTime p_end_date, char *& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
    char *t_result;
    r_result = s_calendar_events_selected.clone();
    MCLog("MCSystemFindCalendarEvent result: %s", r_result);
    return true;
}

bool MCSystemGetCalendarsEvent(char *& r_result)
{
	// TODO - IMPLEMENT SUPPORT FOR API LEVEL 14
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
    char *t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_eventid, t_temp_string);
    s_calendar_event_data.mceventid = t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_title, t_temp_string);
    s_calendar_event_data.mctitle = t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_note, t_temp_string);
    s_calendar_event_data.mcnote = t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_location, t_temp_string);
    s_calendar_event_data.mclocation = t_temp_string;
    
    s_calendar_event_data.mcalert1 = p_alert1;
    s_calendar_event_data.mcalert2 = p_alert2; 
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_frequency, t_temp_string);
    s_calendar_event_data.mcfrequency = t_temp_string;
    s_calendar_event_data.mcfrequencycount = p_frequencycount;
    s_calendar_event_data.mcfrequencyinterval = p_frequencyinterval;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_calendar, t_temp_string);
    s_calendar_event_data.mccalendar = t_temp_string;
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUpdateCalendarEventDone(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    char *t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_calendar_event_id, t_temp_string);
    MCAndroidUpdateCalendarEventDone (t_temp_string);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUpdateCalendarEventCanceled(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    char *t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_calendar_event_id, t_temp_string);
    MCAndroidUpdateCalendarEventCanceled (t_temp_string);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreateCalendarEventDone(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    char *t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_calendar_event_id, t_temp_string);
    MCAndroidCreateCalendarEventDone (t_temp_string);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreateCalendarEventCanceled(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    char *t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_calendar_event_id, t_temp_string);
    MCAndroidCreateCalendarEventCanceled (t_temp_string);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowCalendarEventDone(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    char *t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_calendar_event_id, t_temp_string);
    MCAndroidShowCalendarEventDone (t_temp_string);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowCalendarEventCanceled(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    char *t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_calendar_event_id, t_temp_string);
    MCAndroidShowCalendarEventCanceled (t_temp_string);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doAddCalendarEvent(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    char *t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_calendar_event_id, t_temp_string);
    s_calendar_event_selected = t_temp_string;
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doFindCalendarEvent(JNIEnv *env, jobject object, jstring p_calendar_events_found)
{
    char *t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_calendar_events_found, t_temp_string);
    s_calendar_events_selected = t_temp_string;
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRemoveCalendarEvent(JNIEnv *env, jobject object, jstring p_calendar_event_id)
{
    char *t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_calendar_event_id, t_temp_string);
    s_calendar_event_selected = t_temp_string;
}
