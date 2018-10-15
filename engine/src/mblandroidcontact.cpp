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
#include "mblcontact.h"

#include <string.h>

#include <jni.h>

typedef enum
{
    kMCAndroidContactWaiting,
    kMCAndroidContactDone,
    kMCAndroidContactCanceled,
} MCAndroidContactStatus;

static MCAndroidContactStatus s_contact_status = kMCAndroidContactWaiting; 
static int32_t s_contact_selected = 0;
static MCString s_contacts_selected = "";

extern bool MCAndroidCheckRuntimePermission(MCStringRef p_permission);
bool MCSystemPickContact(int32_t& r_result)
{
    MCLog("MCSystemPickContact");
    
    MCAndroidEngineRemoteCall("pickContact", "i", &r_result);
    s_contact_status = kMCAndroidContactWaiting;
    while (s_contact_status == kMCAndroidContactWaiting)
        MCscreen->wait(60.0, False, True);
    r_result = s_contact_selected;
    return true;
}

void MCAndroidPickContactDone(int32_t p_contact_id)
{
    s_contact_selected = p_contact_id;
    MCLog("MCAndroidPickContactDone() called %d", p_contact_id);
	s_contact_status = kMCAndroidContactDone;
}

void MCAndroidPickContactCanceled(int32_t p_contact_id)
{
    s_contact_selected = p_contact_id;
    MCLog("MCAndroidPickContactCanceled() called %d", p_contact_id);
	s_contact_status = kMCAndroidContactCanceled;
}

bool MCSystemShowContact(int32_t p_contact_id, int32_t& r_result)
{
    MCLog("MCSystemShowContact: %d", p_contact_id);
    MCAndroidEngineRemoteCall("showContact", "ii", &r_result, p_contact_id);
    s_contact_status = kMCAndroidContactWaiting;
    while (s_contact_status == kMCAndroidContactWaiting)
       MCscreen->wait(60.0, False, True);
    r_result = s_contact_selected;
    return true;
}

void MCAndroidShowContactDone(int32_t p_contact_id)
{
    s_contact_selected = p_contact_id;
    MCLog("MCAndroidShowContactDone() called %d", p_contact_id);
	s_contact_status = kMCAndroidContactDone;
}

void MCAndroidShowContactCanceled(int32_t p_contact_id)
{
    s_contact_selected = p_contact_id;
    MCLog("MCAndroidShowContactCanceled() called %d", p_contact_id);
	s_contact_status = kMCAndroidContactCanceled;
}

bool MCSystemCreateContact(int32_t& r_result)
{
    MCLog("MCSystemCreateContact");
    MCAndroidEngineRemoteCall("createContact", "i", &r_result);
    s_contact_status = kMCAndroidContactWaiting;
    while (s_contact_status == kMCAndroidContactWaiting)
        MCscreen->wait(60.0, False, True);
    r_result = s_contact_selected;
    return true;
}

void MCAndroidCreateContactDone(int32_t p_contact_id)
{
    s_contact_selected = p_contact_id;
    MCLog("MCAndroidCreateContactDone() called %d", p_contact_id);
	s_contact_status = kMCAndroidContactDone;
}

void MCAndroidCreateContactCanceled(int32_t p_contact_id)
{
    s_contact_selected = p_contact_id;
    MCLog("MCAndroidCreateContactCanceled() called %d", p_contact_id);
	s_contact_status = kMCAndroidContactCanceled;
}

// 

bool MCAndroidContactToJavaMap(MCArrayRef p_contact, jobject &r_map)
{
	bool t_success = true;
	
	JNIEnv *t_env = MCJavaGetThreadEnv();
	
	t_success = MCJavaMapFromArrayRef(t_env, p_contact, r_map);
	
	return t_success;
}

bool MCAndroidContactFromJavaMap(jobject p_map, MCArrayRef &r_contact)
{
	JNIEnv *t_env = MCJavaGetThreadEnv();	
	return MCJavaMapToArrayRef(t_env, p_map, r_contact);
}

bool MCSystemUpdateContact(MCArrayRef p_contact,
						   MCStringRef p_title, MCStringRef p_message, MCStringRef p_alternate_name,
						   int32_t &r_result)
{
    if (!(MCAndroidCheckRuntimePermission(MCSTR("android.permission.WRITE_CONTACTS"))))
        return false;
    
    MCLog("MCSystemUpdateContact");
	bool t_success = true;
	
	jobject t_map = nil;
	// convert contact to java map
	t_success = MCAndroidContactToJavaMap(p_contact, t_map);
	if (t_success)
	{
		s_contact_status = kMCAndroidContactWaiting;
		MCAndroidEngineRemoteCall("updateContact", "vmxxx", nil, t_map, p_title, p_message, p_alternate_name);
		while (s_contact_status == kMCAndroidContactWaiting)
			MCscreen->wait(60.0, False, True);
		r_result = s_contact_selected;
		return true;
	}

	return false;
}

void MCAndroidUpdateContactDone(int32_t p_contact_id)
{
    s_contact_selected = p_contact_id;
    MCLog("MCAndroidUpdateContactDone() called %d", p_contact_id);
	s_contact_status = kMCAndroidContactDone;
}

void MCAndroidUpdateContactCanceled(int32_t p_contact_id)
{
    s_contact_selected = p_contact_id;
    MCLog("MCAndroidUpdateContactCanceled() called %d", p_contact_id);
	s_contact_status = kMCAndroidContactCanceled;
}

bool MCSystemGetContactData(int32_t p_contact_id, MCArrayRef &r_contact_data)
{
    MCLog("MCSystemGetContactData: %d", p_contact_id);
    
    if (!(MCAndroidCheckRuntimePermission(MCSTR("android.permission.READ_CONTACTS"))))
        return false;
    
	jobject t_jmap = nil;
    MCAndroidEngineRemoteCall("getContactData", "mi", &t_jmap, p_contact_id);
	MCLog("contact map: %p", t_jmap);
	
	if (t_jmap == nil)
		return false;
	
	bool t_success = MCAndroidContactFromJavaMap(t_jmap, r_contact_data);
	if (t_jmap != nil)
		MCJavaGetThreadEnv()->DeleteGlobalRef(t_jmap);
	
	return t_success;
}

bool MCSystemRemoveContact(int32_t p_contact_id)
{
    MCLog("MCSystemRemoveContact: %d", p_contact_id);
    
    if (!(MCAndroidCheckRuntimePermission(MCSTR("android.permission.WRITE_CONTACTS"))))
        return false;
    
    MCAndroidEngineRemoteCall("removeContact", "vi", nil, p_contact_id);
    return true;
}

bool MCSystemAddContact(MCArrayRef p_contact, int32_t &r_result)
{
    MCLog("MCSystemAddContact");
    
    if (!(MCAndroidCheckRuntimePermission(MCSTR("android.permission.WRITE_CONTACTS"))))
        return false;
	
	bool t_success = true;
	jobject t_map = nil;
	// convert contact to java map
	t_success = MCAndroidContactToJavaMap(p_contact, t_map);
	if (t_success)
	{
		MCAndroidEngineRemoteCall("addContact", "im", &r_result, t_map);
		return true;
	}
	return false;
}


bool MCSystemFindContact(MCStringRef p_contact_name, MCStringRef& r_result)
{
    if (!(MCAndroidCheckRuntimePermission(MCSTR("android.permission.READ_CONTACTS"))))
        return false;
    
    MCAndroidEngineRemoteCall("findContact", "vx", nil, p_contact_name);
    return MCStringCreateWithCString(s_contacts_selected . getstring(), r_result);
}

// Get data from Java and assign the values to class values that are then returned to the 
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPickContactDone(JNIEnv *env, jobject object, jint p_contact_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPickContactCanceled(JNIEnv *env, jobject object, jint p_contact_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowContact(JNIEnv *env, jobject object, jint p_contact_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUpdateContactDone(JNIEnv *env, jobject object, jint p_contact_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUpdateContactCanceled(JNIEnv *env, jobject object, jint p_contact_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreateContactDone(JNIEnv *env, jobject object, jint p_contact_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreateContactCanceled(JNIEnv *env, jobject object, jint p_contact_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowContactDone(JNIEnv *env, jobject object, jint p_contact_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowContactCanceled(JNIEnv *env, jobject object, jint p_contact_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doAddContact(JNIEnv *env, jobject object, jint p_contact_id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doFindContact(JNIEnv *env, jobject object, jstring p_contacts_found) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRemoveContact(JNIEnv *env, jobject object, jint p_contact_id) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPickContactDone(JNIEnv *env, jobject object, jint p_contact_id)
{
    MCAndroidPickContactDone (p_contact_id);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPickContactCanceled(JNIEnv *env, jobject object, jint p_contact_id)
{
    MCAndroidPickContactCanceled (p_contact_id);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUpdateContactDone(JNIEnv *env, jobject object, jint p_contact_id)
{
    MCAndroidUpdateContactDone (p_contact_id);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUpdateContactCanceled(JNIEnv *env, jobject object, jint p_contact_id)
{
    MCAndroidUpdateContactCanceled (p_contact_id);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreateContactDone(JNIEnv *env, jobject object, jint p_contact_id)
{
    MCAndroidCreateContactDone (p_contact_id);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doCreateContactCanceled(JNIEnv *env, jobject object, jint p_contact_id)
{
    MCAndroidCreateContactCanceled (p_contact_id);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowContactDone(JNIEnv *env, jobject object, jint p_contact_id)
{
    MCAndroidShowContactDone (p_contact_id);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doShowContactCanceled(JNIEnv *env, jobject object, jint p_contact_id)
{
    MCAndroidShowContactCanceled (p_contact_id);
}

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doFindContact(JNIEnv *env, jobject object, jstring p_contacts_found)
{
    char *t_temp_string;
    MCJavaStringToNative(MCJavaGetThreadEnv(), p_contacts_found, t_temp_string);
    s_contacts_selected = t_temp_string;
}
