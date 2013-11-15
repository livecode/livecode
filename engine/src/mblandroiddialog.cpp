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

#include <string.h>

#include <jni.h>

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    kMCDialogResultUnknown,
    kMCDialogResultOk,
    kMCDialogResultCanceled,
    kMCDialogResultError,
} MCDialogResult;

static bool s_in_popup_dialog = false;
static int s_popup_dialog_action = -1;
static char *s_popup_dialog_text = nil;
static MCDialogResult s_dialog_result;

int32_t MCScreenDC::popupanswerdialog(const char *p_buttons[], uint32_t p_button_count, uint32_t p_type, const char *p_title, const char *p_message)
{
	if (s_in_popup_dialog)
		return -1;

	const char *t_ok_button, *t_cancel_button, *t_other_button;
	t_ok_button = nil;
	t_cancel_button = nil;
	t_other_button = nil;

	if (p_button_count == 0)
		t_ok_button = "OK";
	
	if (p_button_count >= 1)
		t_ok_button = p_buttons[0];

	if (p_button_count >= 3)
		t_other_button = p_buttons[1];

	if (p_button_count >= 2)
		t_cancel_button = p_buttons[p_button_count - 1];

	s_in_popup_dialog = true;
	s_popup_dialog_action = -1;
	MCAndroidEngineRemoteCall("popupAnswerDialog", "vsssss", nil, p_title, p_message, t_ok_button, t_cancel_button, t_other_button);

	while(s_in_popup_dialog)
		MCscreen -> wait(60.0, True, True);

	if (s_popup_dialog_action == 1 && p_button_count >= 3)
		s_popup_dialog_action = 2;
	else if (s_popup_dialog_action == 2)
		s_popup_dialog_action = 1;

	return s_popup_dialog_action;
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doAnswerDialogDone(JNIEnv *env, jobject object, int action) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doAnswerDialogDone(JNIEnv *env, jobject object, int action)
{
	s_in_popup_dialog = false;
	s_popup_dialog_action = action;
	MCAndroidBreakWait();
}

////////////////////////////////////////////////////////////////////////////////

char *MCScreenDC::popupaskdialog(uint32_t p_type, const char *p_title, const char *p_message, const char *p_initial, bool p_hint)
{
	if (s_in_popup_dialog)
		return nil;

	s_in_popup_dialog = true;
	MCAndroidEngineRemoteCall("popupAskDialog", "vbsssb", nil, p_type == AT_PASSWORD, p_title, p_message, p_initial, p_hint);

	while(s_in_popup_dialog)
		MCscreen -> wait(60.0, True, True);

	char *t_result = nil;
	if (s_popup_dialog_text != nil)
	{
		t_result = s_popup_dialog_text;
		s_popup_dialog_text = nil;
	}
	return t_result;
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doAskDialogDone(JNIEnv *env, jobject object, jstring result) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doAskDialogDone(JNIEnv *env, jobject object, jstring result)
{
	s_in_popup_dialog = false;

	if (s_popup_dialog_text != nil)
	{
		MCCStringFree(s_popup_dialog_text);
		s_popup_dialog_text = nil;
	}

	if (result != nil)
	{
		// TODO - java -> native string conversion
		const char *t_utfchars = nil;
		t_utfchars = env->GetStringUTFChars(result, nil);
		if (t_utfchars != nil)
			MCCStringClone(t_utfchars, s_popup_dialog_text);
		env->ReleaseStringUTFChars(result, t_utfchars);
	}
	MCAndroidBreakWait();
}

////////////////////////////////////////////////////////////////////////////////

static MCDateTime s_selected_date;

bool MCSystemPickDate(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect)
{
    if (s_in_popup_dialog)
        return false;
    
    int32_t t_current, t_min, t_max;
    bool t_use_min, t_use_max;

    t_use_min = p_min != nil;
    t_use_max = p_max != nil;
    
    MCExecPoint ep(nil, nil, nil);
    
    if (p_current != nil)
    {
        MCD_convert_from_datetime(ep, CF_SECONDS, CF_UNDEFINED, *p_current);
        t_current = ep.getint4();
    }
    else
        t_current = MCS_time();
    
    if (t_use_min)
    {
        MCD_convert_from_datetime(ep, CF_SECONDS, CF_UNDEFINED, *p_min);
        t_min = ep.getint4();
    }
    if (t_use_max)
    {
        MCD_convert_from_datetime(ep, CF_SECONDS, CF_UNDEFINED, *p_max);
        t_max = ep.getint4();
    }
    
    s_in_popup_dialog = true;
    s_dialog_result = kMCDialogResultUnknown;
	// IM-2012-10-31 [[ BZ 10483 ]] - make sure we have the timezone bias for the date
	MCS_getlocaldatetime(s_selected_date);
    MCAndroidEngineRemoteCall("showDatePicker", "vbbiii", nil, t_use_min, t_use_max, t_min, t_max, t_current);
    
    while(s_in_popup_dialog)
		MCscreen -> wait(60.0, True, True);

    if (s_dialog_result == kMCDialogResultError)
        return false;
    
    r_canceled = s_dialog_result == kMCDialogResultCanceled;
    if (!r_canceled)
	{
		// IM-2012-10-31 [[ BZ 10483 ]] - convert the return value back to UTC
		MCS_datetimetouniversal(s_selected_date);
        *r_result = s_selected_date;
	}
    
    return true;
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doDatePickerDone(JNIEnv *env, jobject object, jint p_year, jint p_month, jint p_day, jboolean p_done) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doDatePickerDone(JNIEnv *env, jobject object, jint p_year, jint p_month, jint p_day, jboolean p_done)
{
    if (!p_done)
        s_dialog_result = kMCDialogResultCanceled;
    else
    {
        s_selected_date.year = p_year;
        s_selected_date.month = p_month;
        s_selected_date.day = p_day;
        
        s_dialog_result = kMCDialogResultOk;
    }
    
    s_in_popup_dialog = false;
}

bool MCSystemPickTime(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect)
{
    if (s_in_popup_dialog)
        return false;
    
    int32_t t_hour, t_minute;
    
    MCExecPoint ep(nil, nil, nil);
    
    MCDateTime t_current;
    if (p_current != nil)
        t_current = *p_current;
    else
    {
        ep.setnvalue(MCS_time());
        MCD_convert_to_datetime(ep, CF_SECONDS, CF_UNDEFINED, t_current);
    }
    
    // IM-2012-05-09 - make sure we show the correct local hour + minute values
    MCS_datetimetolocal(t_current);
    t_hour = t_current.hour;
    t_minute = t_current.minute;

    s_in_popup_dialog = true;
    s_dialog_result = kMCDialogResultUnknown;
	// IM-2012-10-31 [[ BZ 10483 ]] - make sure we have the timezone bias for the date
	s_selected_date = t_current;
    MCAndroidEngineRemoteCall("showTimePicker", "vii", nil, t_hour, t_minute);
    
    while (s_in_popup_dialog)
        MCscreen->wait(60.0, True, True);
    
    if (s_dialog_result == kMCDialogResultError)
        return false;
    
    r_canceled = s_dialog_result == kMCDialogResultCanceled;
    if (!r_canceled)
	{
		// IM-2012-10-31 [[ BZ 10483 ]] - convert the return value back to UTC
		MCS_datetimetouniversal(s_selected_date);
        *r_result = s_selected_date;
	}
    
    return true;
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doTimePickerDone(JNIEnv *env, jobject object, jint p_hour, jint p_minute, jboolean p_done) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doTimePickerDone(JNIEnv *env, jobject object, jint p_hour, jint p_minute, jboolean p_done)
{
    if (!p_done)
        s_dialog_result = kMCDialogResultCanceled;
    else
    {
        s_selected_date.hour = p_hour;
        s_selected_date.minute = p_minute;
        
        s_dialog_result = kMCDialogResultOk;
    }
    
    s_in_popup_dialog = false;
}

bool MCSystemPickDateAndTime(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect)
{
    // no date+time picker on android
    return false;
}

////////////////////////////////////////////////////////////////////////////////

static uint32_t s_selected_index;

bool MCSystemPickOption(const_cstring_array_t **p_expression, const_int32_array_t *p_indexes, uint32_t p_expression_cnt, const_int32_array_t *&r_result, bool p_use_checkmark, bool p_use_picker, bool p_use_cancel, bool p_use_done, bool &r_canceled, MCRectangle p_button_rect)
{
    MCLog("indexes: (%p) {length = %d, element[0] = %d}", p_indexes, p_indexes ? p_indexes->length : 0, p_indexes && p_indexes->length > 0 ? p_indexes->elements[0] : 0); 
    // multi-pick list not supported
    if (p_expression_cnt != 1)
        return false;
    
    bool t_success = true;
    // convert cstring array to java list of strings
    
    JNIEnv *t_env = MCJavaGetThreadEnv();
    jobject t_joptionlist = nil;
    const char *t_title = nil;
    bool t_has_selection = false;
    uint32_t t_selected_index = 0;
    
    t_success = MCJavaInitList(t_env, t_joptionlist);
    
    if (t_success)
    {
        for (uint32_t i = 0; t_success && i < p_expression[0]->length; i++)
        {
            MCString t_string(p_expression[0]->elements[i]);
            t_success = MCJavaListAppendString(t_env, t_joptionlist, &t_string);
        }
    }
    
    if (t_success)
    {
        t_has_selection = (p_indexes != nil && p_indexes->length != 0);
        if (t_has_selection)
            t_selected_index = p_indexes->elements[0];
        
        MCLog("selected index: %d", t_selected_index);
        s_in_popup_dialog = true;
        s_dialog_result = kMCDialogResultUnknown;
        MCAndroidEngineRemoteCall("showListPicker", "vlsbibbb", nil, t_joptionlist, t_title, t_has_selection, t_selected_index, p_use_checkmark, p_use_cancel, p_use_done);
    
        while (s_in_popup_dialog)
            MCscreen->wait(60.0, True, True);
        
        if (s_dialog_result == kMCDialogResultError)
            t_success = false;
        
        r_canceled = s_dialog_result == kMCDialogResultCanceled;
        if (!r_canceled)
        {
            t_success = MCMemoryNew(r_result) && MCMemoryNewArray(1, r_result->elements);
            if (t_success)
            {
                r_result->length = 1;
                r_result->elements[0] = s_selected_index + 1;
            }
        }

    }
    
    if (t_joptionlist != nil)
        MCJavaFreeList(t_env, t_joptionlist);
    
    return t_success;
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doListPickerDone(JNIEnv *env, jobject object, jint p_index, jboolean p_done) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doListPickerDone(JNIEnv *env, jobject object, jint p_index, jboolean p_done)
{
    if (!p_done)
        s_dialog_result = kMCDialogResultCanceled;
   else
    {
        s_selected_index = p_index;        
        s_dialog_result = kMCDialogResultOk;
    }
    
    s_in_popup_dialog = false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemPickMedia(MCMediaType *p_media_type, const char *&r_result, bool p_multiple)
{
    // UNIMPLEMENTED
    return false;
}

