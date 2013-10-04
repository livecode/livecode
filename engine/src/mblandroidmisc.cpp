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
#include "system.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "chunk.h"
#include "scriptpt.h"

#include "eventqueue.h"
#include "debug.h"

#include "mblandroid.h"
#include "mblandroidutil.h"
#include "mbldc.h"
#include "mblstore.h"

////////////////////////////////////////////////////////////////////////////////

extern int32_t g_android_keyboard_type;

////////////////////////////////////////////////////////////////////////////////

void MCQuit(void)
{
	// IM-2013-05-01: [[ BZ 10586 ]] send shutdown message when closing due
	// to unhandled backKey message
	if (MCdefaultstackptr != nil)
		MCdefaultstackptr->getcard()->message(MCM_shut_down);
	MCquit = True;
	MCexitall = True;
	MCtracestackptr = NULL;
	MCtraceabort = True;
	MCtracereturn = True;
	// IM-2013-05-01: [[ BZ 10586 ]] No longer call finishActivity from here,
	// instead wait for main loop to terminate in mobile_main()
}

class MCMessageEvent : public MCCustomEvent
{
public:
	template <class C>
	static MCMessageEvent* create(MCObjectHandle *p_object, const char *p_message)
	{
		C *t_event = nil;
		if (!MCMemoryNew(t_event))
			return nil;

		if (!t_event->setMessage(p_message))
		{
			MCMemoryDelete(t_event);
			return nil;
		}

		t_event->setObject(p_object);

		return t_event;
	}
    
    static MCMessageEvent* create(MCObjectHandle *p_object, const char *p_message)
    {
        return create<MCMessageEvent>(p_object, p_message);
    }

	void Dispatch(void)
	{
        //MCLog("dispatch message \"%s\"", MCNameGetCString(m_message));
		MCObject *t_object;
		t_object = m_object -> Get();
		if (t_object != nil)
			t_object -> message(m_message);
	}

	void Destroy(void)
	{
		MCNameDelete(m_message);
		if (m_object != nil)
			m_object->Release();
		delete this;
	}

protected:
	MCNameRef m_message;
	MCObjectHandle *m_object;

	bool setMessage(const char *p_message)
	{
		return MCNameCreateWithCString(p_message, m_message);
	}

	bool setObject(MCObjectHandle *p_object)
	{
		m_object = p_object;
		m_object->Retain();
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...)
{
	MCExecPoint ep(nil, nil, nil);
	
	bool t_success;
	t_success = true;
	
	bool t_now_optional;
	t_now_optional = false;
	
	va_list t_args;
	va_start(t_args, p_format);
	
	while(*p_format != '\0' && t_success)
	{
		if (*p_format == '|')
		{
			t_now_optional = true;
			p_format++;
			continue;
		}
		
		if (p_parameters != nil)
			t_success = p_parameters -> eval_argument(ep) == ES_NORMAL;
		else if (t_now_optional)
			break;
		else
			t_success = false;
		
		switch(*p_format)
		{
			case 'b':
				if (t_success)
					*(va_arg(t_args, bool *)) = ep . getsvalue() == MCtruemcstring;
				break;
				
			case 's':
				if (t_success)
					*(va_arg(t_args, char **)) = ep . getsvalue() . clone();
				else
					*(va_arg(t_args, char **)) = nil;
				break;
			
			case 'd':
				if (t_success)
					(va_arg(t_args, MCString *)) -> set(ep . getsvalue() . clone(), ep . getsvalue() . getlength());
				else
					(va_arg(t_args, MCString *)) -> set(nil, 0);
				break;
			
			case 'a':
				if (t_success && ep . getformat() == VF_ARRAY)
					*(va_arg(t_args, MCVariableValue **)) = ep . getarray();
				else
					*(va_arg(t_args, MCVariableValue **)) = nil;
				break;
				
			case 'r':
			{
				int2 i1, i2, i3, i4;
				if (t_success)
					t_success = MCU_stoi2x4(ep . getsvalue(), i1, i2, i3, i4) == True;
				if (t_success)
					MCU_set_rect(*(va_arg(t_args, MCRectangle *)), i1, i2, i3 - i1, i4 - i2);
			}
				break;
				
			case 'i':
				if (t_success)
				{
					if (ep . getformat() != VF_STRING || ep . ston() == ES_NORMAL)
						*(va_arg(t_args, int32_t *)) = ep . getint4();
					else
						t_success = false;
				}
				break;
				
			case 'u':
				if (t_success)
				{
					if (ep . getformat() != VF_STRING || ep . ston() == ES_NORMAL)
						*(va_arg(t_args, uint32_t *)) = ep . getuint4();
					else
						t_success = false;
				}
				break;
		};
		
		p_format += 1;
		
		if (p_parameters != nil)
			p_parameters = p_parameters -> getnext();
	}
	
	va_end(t_args);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemPick(const char *p_options, bool p_is_unicode, bool p_use_checkmark, uint32_t p_initial_index, uint32_t& r_chosen_index, MCRectangle p_button_rect)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

extern int UnicodeToUTF8(const uint2 *p_source_str, int p_source, char *p_dest_str, int p_dest);
extern int UTF8ToUnicode(const char *p_source_str, int p_source, uint2 *p_dest_str, int p_dest);

uint32_t NativeToUnicode(const char *p_string, uint32_t p_string_length, unichar_t *p_buffer, uint32_t p_buffer_length)
{
	uint32_t t_to_write = MCU_min(p_string_length, p_buffer_length);
	if (p_buffer != nil)
		for (uint32_t i = 0; i < t_to_write; i++)
			p_buffer[i] = p_string[i];
	else
		t_to_write = p_string_length;

	return t_to_write;
}

uint32_t UnicodeToNative(const unichar_t *p_string, uint32_t p_string_length, char *p_buffer, uint32_t p_buffer_length)
{
	uint32_t t_to_write = MCU_min(p_string_length, p_buffer_length);
	if (p_buffer != nil)
		for (uint32_t i = 0; i < t_to_write; i++)
		{
			if (p_string[i] & 0xFF00)
				p_buffer[i] = '?';
			else
				p_buffer[i] = (char) p_string[i];
		}
	else
		t_to_write = p_buffer_length;

	return t_to_write;
}

typedef struct
{
	uint1 charset;
	const char *name;
} charset_to_name_t;

static charset_to_name_t s_charset_to_name[] = {
	{ LCH_ENGLISH, "ISO-8859-1" },
	{ LCH_ROMAN, "ISO-8859-1" },
	{ LCH_JAPANESE, "Shift_JIS" },
	{ LCH_CHINESE, "Big5" },
	{ LCH_KOREAN, "windows-949" },
	{ LCH_ARABIC, "windows-1256" },
	{ LCH_HEBREW, "windows-1255" },
	{ LCH_TURKISH, "windows-1254" },
	{ LCH_POLISH, "windows-1250" },
	{ LCH_GREEK, "windows-1253" },
	{ LCH_SIMPLE_CHINESE, "GB2312" },
	{ LCH_THAI, "cp874" },
	{ LCH_VIETNAMESE, "windows-1258" },
	{ LCH_UTF8, "UTF-8" },
	{ LCH_UNICODE, "UTF-16LE" },

	{ LCH_RUSSIAN, "windows-1251" },
	{ LCH_BULGARIAN, "windows-1251" },
	{ LCH_UKRAINIAN, "windows-1251" },

	{ LCH_LITHUANIAN, "windows-1257" },
	//LCH_DEFAULT = 255
};

const char *MCCharsetToName(uint1 p_charset)
{
	for (uint32_t i = 0; i < (sizeof(s_charset_to_name) / sizeof(charset_to_name_t)); i++)
		if (s_charset_to_name[i].charset == p_charset)
			return s_charset_to_name[i].name;

	return nil;
}

uint32_t MCAndroidSystem::TextConvert(const void *p_string, uint32_t p_string_length, void *p_buffer, uint32_t p_buffer_length, uint32_t p_from_charset, uint32_t p_to_charset)
{
	if (p_from_charset == LCH_UTF8 && p_to_charset == LCH_UNICODE)
		return UTF8ToUnicode((const char *)p_string, p_string_length, (unichar_t*)p_buffer, p_buffer_length / 2) * 2;
	else if (p_from_charset == LCH_UNICODE && p_to_charset == LCH_UTF8)
		return UnicodeToUTF8((unichar_t*)p_string, p_string_length / 2, (char *)p_buffer, p_buffer_length);
	else if ((p_from_charset == LCH_ENGLISH || p_from_charset == LCH_ROMAN) && p_to_charset == LCH_UNICODE)
		return NativeToUnicode((const char *)p_string, p_string_length, (unichar_t*)p_buffer, p_buffer_length / 2) * 2;
	else if (p_from_charset == LCH_UNICODE && (p_to_charset == LCH_ENGLISH || p_to_charset == LCH_ROMAN))
		return UnicodeToNative((unichar_t*)p_string, p_string_length / 2, (char *)p_buffer, p_buffer_length);
	//MCLog("text conversion %d to %d", p_from_charset, p_to_charset);

	MCString t_from_string;
	MCString t_to_string;

	t_from_string.set((const char *)p_string, p_string_length);
	t_to_string.set(NULL, 0);

	const char *t_from_charset, *t_to_charset;
	t_from_charset = MCCharsetToName(p_from_charset);
	t_to_charset = MCCharsetToName(p_to_charset);

	if (p_buffer == NULL)
	{
		int32_t t_bytecount = 0;
		MCAndroidEngineCall("conversionByteCount", "idss", &t_bytecount, &t_from_string, t_from_charset, t_to_charset);
		//MCLog("byte count: %d", t_bytecount);
		return t_bytecount;
	}
	else
	{
		MCAndroidEngineCall("convertCharset", "ddss", &t_to_string, &t_from_string, t_from_charset, t_to_charset);

		if (t_to_string.getlength() > 0)
		{
			MCMemoryCopy(p_buffer, t_to_string.getstring(), t_to_string.getlength());
			MCMemoryDeallocate((char*)t_to_string.getstring());
		}

		//MCLog("converted string: %.*s", t_to_string.getlength(), t_to_string.getstring());
		return t_to_string.getlength();
	}
}

bool MCAndroidSystem::TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

class MCBackPressedEvent : public MCMessageEvent
{
public:
	void Dispatch()
	{
		//MCLog("dispatching backPressed event", nil);
		MCObject *t_object;
		t_object = m_object -> Get();
		if (t_object != nil)
		{
			Exec_stat t_stat = t_object -> message(m_message);
			//MCLog("message result: %d", t_stat);
			if (ES_PASS == t_stat || ES_NOT_HANDLED == t_stat)
			{
				//MCLog("quiting", nil);
				MCQuit();
			}
		}
	}
};

void MCAndroidBackPressed()
{
	MCMessageEvent *t_event;
	MCObjectHandle *t_handle;
	t_handle = MCdefaultstackptr->getcurcard()->gethandle();
	t_event = MCMessageEvent::create<MCBackPressedEvent>(t_handle, "backKey");
	t_handle->Release();
	MCEventQueuePostCustom(t_event);
}

void MCAndroidMenuKey()
{
    MCMessageEvent *t_event;
    MCObjectHandle *t_handle;
    t_handle = MCdefaultstackptr->getcurcard()->gethandle();
    t_event = MCMessageEvent::create(t_handle, "menuKey");
    t_handle->Release();
    MCEventQueuePostCustom(t_event);
}

void MCAndroidSearchKey()
{
    //MCLog("MCAndroidSearchKey()", nil);
    MCMessageEvent *t_event;
    MCObjectHandle *t_handle;
    t_handle = MCdefaultstackptr->getcurcard()->gethandle();
    t_event = MCMessageEvent::create(t_handle, "searchKey");
    t_handle->Release();
    MCEventQueuePostCustom(t_event);
}

////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCHandleLibUrlDownloadToFileAndroid */ LEGACY_EXEC
static Exec_stat MCHandleLibUrlDownloadToFile(void *context, MCParameter *p_parameters)
{
	char *t_url, *t_filename;
	t_url = nil;
	t_filename = nil;
	
	MCExecPoint ep(nil, nil, nil);
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_url = ep . getsvalue() . clone();
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_filename = ep . getsvalue() . clone();
		p_parameters = p_parameters -> getnext();
	}
	
	extern void MCS_downloadurl(MCObject *, const char *, const char *);
	MCS_downloadurl(MCtargetptr, t_url, t_filename);
	
	return ES_NORMAL;
}
#endif /* MCHandleLibUrlDownloadToFileAndroid */

// MW-2013-10-02: [[ MobileSSLVerify ]] Handle libUrlSetSSLVerification for Android.
static Exec_stat MCHandleLibUrlSetSSLVerification(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	bool t_enabled;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "b", &t_enabled);
	
	extern void MCS_seturlsslverification(bool enabled);
	if (t_success)
		MCS_seturlsslverification(t_enabled);
	
	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCHandleCameraFeaturesAndroid */ LEGACY_EXEC
static Exec_stat MCHandleCameraFeatures(void *context, MCParameter *p_parameters)
{
	char *t_camera_dir = nil;
	int32_t t_cam_count = 0;
	char *t_info_string = nil;

	MCAndroidEngineCall("getCameraDirections", "s", &t_camera_dir);
	t_cam_count = MCCStringLength(t_camera_dir);
	bool t_front_cam = false;
	bool t_rear_cam = false;
	for (int32_t i = 0; i < t_cam_count; i++)
	{
		if (t_camera_dir[i] == 'f')
			t_front_cam = true;
		else if (t_camera_dir[i] == 'b')
			t_rear_cam = true;
	}
	bool t_no_args = true;
	MCExecPoint ep(nil, nil, nil);
	ep.clear();
	if (p_parameters != nil)
	{
		t_no_args = false;
		p_parameters->eval_argument(ep);
		if (ep.getsvalue() == "front")
		{
			if (t_front_cam)
				MCCStringAppend(t_info_string, "photo,");
		}
		else if (ep.getsvalue() == "rear")
		{
			if (t_front_cam)
				MCCStringAppend(t_info_string, "photo,");
		}
		else
			t_no_args = true;
	}
	if (t_no_args)
	{
		if (t_front_cam)
			MCCStringAppend(t_info_string, "front photo,");
		if (t_rear_cam)
			MCCStringAppend(t_info_string, "rear photo,");
	}

	if (MCCStringLength(t_info_string) == 0)
		MCresult->clear();
	else
		MCresult->copysvalue(MCString(t_info_string, MCCStringLength(t_info_string) - 1));

	MCCStringFree(t_info_string);
	MCCStringFree(t_camera_dir);
	return ES_NORMAL;
}
#endif /* MCHandleCameraFeaturesAndroid */

void MCMobileCreateImageFromData(const char *p_bytes, uint32_t p_length)
{
	MCtemplateimage->setparent((MCObject *)MCdefaultstackptr -> getcurcard());
	MCImage *iptr = (MCImage *)MCtemplateimage->clone(False, OP_NONE, false);
	MCtemplateimage->setparent(NULL);
	iptr -> attach(OP_CENTER, false);
	
	MCExecPoint ep(nil, nil, nil);
	ep . setsvalue(MCString(p_bytes, p_length));
	iptr -> setprop(0, P_TEXT, ep, false);
}

////////////////////////////////////////////////////////////////////////////////

bool MCMobilePickPhoto(const char *p_source, int32_t p_max_width, int32_t p_max_height)
{
#ifdef /* MCMobilePickPhoto */ LEGACY_EXEC
	MCAndroidEngineCall("showPhotoPicker", "vs", nil, p_source);
#endif /* MCMobilePickPhoto */
}

static char *s_pick_photo_data = nil;
static uint32_t s_pick_photo_size = 0;
static char *s_pick_photo_err = nil;
static bool s_pick_photo_returned = false;

#ifdef /* MCHandlePickPhotoAndroid */ LEGACY_EXEC
static Exec_stat MCHandlePickPhoto(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	ep . clear();
	
	MCParameter *t_source_param, *t_width_param, *t_height_param;
	t_source_param = p_parameters;
	t_width_param = t_source_param != nil ? t_source_param -> getnext() : nil;
	t_height_param = t_width_param != nil ? t_width_param -> getnext() : nil;
	
	int32_t t_width, t_height;
	t_width = t_height = 0;
	if (t_width_param != nil)
	{
		t_width_param -> eval_argument(ep);
		t_width = ep . getint4();
	}
	if (t_height_param != nil)
	{
		t_height_param -> eval_argument(ep);
		t_height = ep . getint4();
	}
	
	if (p_parameters != nil)
		p_parameters -> eval_argument(ep);
	s_pick_photo_returned = false;

	MCMobilePickPhoto(ep.getcstring(), t_width, t_height);

	while (!s_pick_photo_returned)
		MCscreen->wait(60.0, False, True);

	if (s_pick_photo_data != nil)
	{
		//MCLog("photo picked", nil);
		MCMobileCreateImageFromData(s_pick_photo_data, s_pick_photo_size);
		MCMemoryDeallocate(s_pick_photo_data);
		s_pick_photo_data = nil;

		MCresult->clear();
	}
	else
	{
		//MCLog("photo pick canceled", nil);
		MCresult->grab(s_pick_photo_err, MCCStringLength(s_pick_photo_err));
		s_pick_photo_err = nil;
	}

	return ES_NORMAL;
}
#endif /* MCHandlePickPhotoAndroid */

void MCAndroidPhotoPickDone(const char *p_data, uint32_t p_size)
{
#ifdef /* MCAndroidPhotoPickDone */ LEGACY_EXEC
	if (s_pick_photo_data != nil)
	{
		MCMemoryDeallocate(s_pick_photo_data);
		s_pick_photo_data = nil;
	}

	if (p_data != nil)
	{
		MCMemoryAllocateCopy(p_data, p_size, (void*&)s_pick_photo_data);
		s_pick_photo_size = p_size;
	}
	s_pick_photo_returned = true;
#endif /* MCAndroidPhotoPickDone */
}

void MCAndroidPhotoPickError(const char *p_error)
{
#ifdef /* MCAndroidPhotoPickError */ LEGACY_EXEC
	if (s_pick_photo_data != nil)
	{
		MCMemoryDeallocate(s_pick_photo_data);
		s_pick_photo_data = nil;
	}
	if (s_pick_photo_err != nil)
		MCCStringFree(s_pick_photo_err);
	MCCStringClone(p_error, s_pick_photo_err);
	s_pick_photo_returned = true;
#endif /* MCAndroidPhotoPickError */
}

void MCAndroidPhotoPickCanceled()
{
#ifdef /* MCAndroidPhotoPickCanceled */ LEGACY_EXEC
	MCAndroidPhotoPickError("cancel");
#endif /* MCAndroidPhotoPickCanceled */
}

////////////////////////////////////////////////////////////////////////////////
#ifdef /* MCHandleSetKeyboardTypeAndroid */ LEGACY_EXEC
Exec_stat MCHandleSetKeyboardType(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);

	if (p_parameters != nil)
	{
		int32_t t_type;
		
		p_parameters -> eval_argument(ep);
		if (ep . getsvalue() == "default")
			t_type = 1;
		else if (ep . getsvalue() == "alphabet")
			t_type = 1;
		else if (ep . getsvalue() == "numeric" || ep . getsvalue() == "decimal")
			t_type = 3;
		else if (ep . getsvalue() == "number")
			t_type = 2;
		else if (ep . getsvalue() == "phone")
			t_type = 4;
		else if (ep . getsvalue() == "email")
			t_type = 5;
		else
			t_type = 1;
		
		g_android_keyboard_type = t_type;
	}
	
	MCAndroidEngineRemoteCall("setTextInputMode", "vi", nil, g_android_keyboard_type);
	
	return ES_NORMAL;
}
#endif /* MCHandleSetKeyboardTypeAndroid */

////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCHandleSetStatusbarVisibility */ LEGACY_EXEC
Exec_stat MCHandleSetStatusbarVisibility(void *context, MCParameter *parameters)
{

	bool t_visible;
	t_visible = ((uint32_t)context) != 0;

	MCAndroidEngineRemoteCall("setStatusbarVisibility", "vb", nil, t_visible);

	return ES_NORMAL;

}
#endif /* MCHandleSetStatusbarVisibility */

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandlePixelDensity(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePixelDensity */ LEGACY_EXEC
	float t_density;
	MCAndroidEngineRemoteCall("getPixelDensity", "f", &t_density);
	MCresult -> setnvalue(t_density);
	return ES_NORMAL;
#endif /* MCHandlePixelDensity */
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleBuildInfo(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleBuildInfo */ LEGACY_EXEC
	MCExecPoint ep(nil, nil, nil);

	if (p_parameters != nil)
	{
		char *t_value;
		t_value = NULL;

		char *t_key;
		t_key = NULL;

		p_parameters -> eval_argument(ep);
		t_key = ep . getsvalue() . clone();

		if (!MCAndroidGetBuildInfo(t_key, t_value))
			return ES_ERROR;

		MCresult->grab(t_value, MCCStringLength(t_value));

		MCCStringFree(t_key);
	}

	return ES_NORMAL;
#endif /* MCHandleBuildInfo */
}

void MCS_getnetworkinterfaces(MCExecPoint& ep)
{
	char *t_network_interfaces = NULL;
	
	MCAndroidEngineCall("getNetworkInterfaces", "s", &t_network_interfaces);

	if (t_network_interfaces == NULL)
		ep.clear();
	else
		ep.grabbuffer(t_network_interfaces, MCCStringLength(t_network_interfaces));
}

////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCHandlePreferredLanguagesAndroid */ LEGACY_EXEC
Exec_stat MCHandlePreferredLanguages(void *context, MCParameter *p_parameters)
{
    char *r_preferred_languages = NULL;
	MCAndroidEngineCall("getPreferredLanguages", "s", &r_preferred_languages);
    MCresult -> sets(r_preferred_languages);
	return ES_NORMAL;
}
#endif /* MCHandlePreferredLanguagesAndroid */

#ifdef /* MCHandleCurrentLocaleAndroid */ LEGACY_EXEC
Exec_stat MCHandleCurrentLocale(void *context, MCParameter *p_parameters)
{
    char *r_preferred_locale = NULL;
	MCAndroidEngineCall("getPreferredLocale", "s", &r_preferred_locale);
   	MCresult -> sets(r_preferred_locale);
	return ES_NORMAL;
}
#endif /* MCHandleCurrentLocaleAndroid */

#ifdef /* MCHandleLockIdleTimerAndroid */ LEGACY_EXEC
Exec_stat MCHandleLockIdleTimer(void *context, MCParameter *p_parameters)
{
	MCAndroidEngineCall("doLockIdleTimer", "v", nil);
	return ES_NORMAL;
}
#endif /* MCHandleLockIdleTimerAndroid */

#ifdef /* MCHandleUnlockIdleTimerAndroid */ LEGACY_EXEC
Exec_stat MCHandleUnlockIdleTimer(void *context, MCParameter *p_parameters)
{
	MCAndroidEngineCall("doUnlockIdleTimer", "v", nil);
	return ES_NORMAL;
}
#endif /* MCHandleUnlockIdleTimerAndroid */

#ifdef /* MCHandleIdleTimerLockedAndroid */ LEGACY_EXEC
Exec_stat MCHandleIdleTimerLocked(void *context, MCParameter *p_parameters)
{
    bool r_idle_timer_locked = false;
	MCAndroidEngineCall("getLockIdleTimerLocked", "b", &r_idle_timer_locked);
    MCresult -> sets(MCU_btos(r_idle_timer_locked));
	return ES_NORMAL;
}
#endif /* MCHandleIdleTimerLockedAndroid */

////////////////////////////////////////////////////////////////////////////////

static bool is_png_data(const MCString& p_data)
{
	return p_data . getlength() > 4 && MCMemoryEqual(p_data . getstring(), "\211PNG", 4);
}

static bool is_gif_data(const MCString& p_data)
{
	return p_data . getlength() > 4 && MCMemoryEqual(p_data . getstring(), "GIF8", 4);
}

static bool is_jpeg_data(const MCString& p_data)
{
	return p_data . getlength() > 2 && MCMemoryEqual(p_data . getstring(), "\xff\xd8", 2);
}

#ifdef /* MCHandleExportImageToAlbumAndroid */ LEGACY_EXEC
Exec_stat MCHandleExportImageToAlbum(void *context, MCParameter *p_parameters)
{
    char *r_save_result = NULL;
    char *t_file_name = NULL;
    char t_file_extension[5];
    t_file_extension[0] = '\0';
	MCString t_raw_data = NULL;
    bool t_success = true;
	MCLog("MCHandleExportImageToAlbum() called", nil);

    if (p_parameters == nil)
		return ES_NORMAL;
	
	MCExecPoint ep(nil, nil, nil);	
	p_parameters -> eval_argument(ep);
    
	if (is_png_data(ep . getsvalue()))
    {
        sprintf (t_file_extension, ".png\n");
    }
    else if (is_gif_data(ep . getsvalue()))
    {
        sprintf (t_file_extension, ".gif\n");
    }
    else if (is_jpeg_data(ep . getsvalue()))
    {
        sprintf (t_file_extension, ".jpg\n");
    }
    if (t_file_extension[0] != '\0')
    {
        t_raw_data = ep . getsvalue();
    }
	else
    {
        MCLog("Type not found", nil);
		uint4 parid;
		MCObject *objptr;
		MCChunk *tchunk = new MCChunk(False);
		MCerrorlock++;
		MCScriptPoint sp(ep);
		Parse_stat stat = tchunk->parse(sp, False);
		if (stat != PS_NORMAL || tchunk->getobj(ep, objptr, parid, True) != ES_NORMAL)
		{
            MCLog("could not find image", nil);
			MCresult -> sets("could not find image");
			MCerrorlock--;
			delete tchunk;
			return ES_NORMAL;
		}
		
		if (objptr -> gettype() != CT_IMAGE)
		{
            MCLog("not an image", nil);
			MCresult -> sets("not an image");
			return ES_NORMAL;
		}
		
		MCImage *t_image;
		t_image = static_cast<MCImage *>(objptr);
		if (t_image -> getcompression() == F_PNG)
        {
            sprintf (t_file_extension, ".png\n");
        }
        else if (t_image -> getcompression() == F_JPEG)
        {
            sprintf (t_file_extension, ".jpg\n");
        }
        else if (t_image -> getcompression() == F_GIF)
		{
            sprintf (t_file_extension, ".gif\n");
        }
        else
        {
            MCLog("not a supported image", nil);
			MCresult -> sets("not a supported format");
			return ES_NORMAL;
		}
        MCLog("MCHandleExportImageToAlbum() converting to raw data", nil);
		t_raw_data = t_image -> getrawdata();
    }
    // See if the user provided us with a file name
    if (t_success)
    {
        if (p_parameters != nil)
        {
            p_parameters = p_parameters -> getnext();
            if (p_parameters != nil)
            {
                p_parameters -> eval_argument(ep);
                t_file_name = ep . getsvalue() . clone();
            }
        }
        MCAndroidEngineCall("exportImageToAlbum", "sdss", &r_save_result, &t_raw_data, t_file_name, t_file_extension);
        MCresult -> sets(r_save_result);
    }
    else
    {
        MCresult -> sets("export failed");
    }       
	return ES_NORMAL;
}
#endif /* MCHandleExportImageToAlbumAndroid */

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    kMCAndroidMediaWaiting,
    kMCAndroidMediaDone,
    kMCAndroidMediaCanceled,
} MCAndroidMediaStatus;

static MCAndroidMediaStatus s_media_status = kMCAndroidMediaWaiting; 
static char *s_media_content = NULL;

Exec_stat MCHandlePickMedia(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePickMedia */ LEGACY_EXEC
	bool t_success;
    bool t_audio = false;
    bool t_video = false;
    char *t_option_list;

    s_media_status = kMCAndroidMediaWaiting;
    
	t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	while (t_success)
	{
		if ((MCCStringEqualCaseless(t_option_list, "music")) ||
		    (MCCStringEqualCaseless(t_option_list, "podCast")) ||
		    (MCCStringEqualCaseless(t_option_list, "audioBook")) ||
            (MCCStringEqualCaseless(t_option_list, "anyAudio")))
        {
            t_audio = true;
        }
		if ((MCCStringEqualCaseless(t_option_list, "movie")) || 
			(MCCStringEqualCaseless(t_option_list, "tv")) ||
            (MCCStringEqualCaseless(t_option_list, "videoPodcast")) ||
            (MCCStringEqualCaseless(t_option_list, "musicVideo")) ||
            (MCCStringEqualCaseless(t_option_list, "videoITunesU")) ||
            (MCCStringEqualCaseless(t_option_list, "anyVideo")))
        {
            t_video = true;
		}
		t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	}
	if (t_audio && !t_video)
	{
        MCAndroidEngineCall("pickMedia", "vs", nil, "audio/*");
	}
	else if (!t_audio && t_video)
	{
        MCAndroidEngineCall("pickMedia", "vs", nil, "video/*");
	}
    else
	{
        MCAndroidEngineCall("pickMedia", "vs", nil, "audio/* video/*");
	}

    while (s_media_status == kMCAndroidMediaWaiting)
        MCscreen->wait(60.0, False, True);
    MCresult -> sets (s_media_content);
//    MCLog("Media Types Returned: %s", s_media_content);
    
	return ES_NORMAL;
#endif /* MCHandlePickMedia */
}

void MCAndroidMediaDone(char *p_media_content)
{
#ifdef /* MCAndroidMediaDone */ LEGACY_EXEC
    s_media_content = p_media_content;
//    MCLog("MCAndroidMediaDone() called %s", p_media_content);
	s_media_status = kMCAndroidMediaDone;
#endif /* MCAndroidMediaDone */
}

void MCAndroidMediaCanceled()
{
#ifdef /* MCAndroidMediaCanceled */ LEGACY_EXEC
//    MCLog("MCAndroidMediaCanceled() called", nil);
	s_media_status = kMCAndroidMediaCanceled;
#endif /* MCAndroidMediaCanceled */
}

////////////////////////////////////////////////////////////////////////////////

extern Exec_stat MCHandleCanMakePurchase(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleEnablePurchaseUpdates(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleDisablePurchaseUpdates(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleRestorePurchases(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandlePurchaseList(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandlePurchaseCreate(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandlePurchaseState(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandlePurchaseError(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandlePurchaseSet(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandlePurchaseGet(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandlePurchaseSendRequest(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandlePurchaseConfirmDelivery(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandlePurchaseVerify(void *context, MCParameter *p_parameters);

////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCHandleClearTouchesAndroid */ LEGACY_EXEC
static Exec_stat MCHandleClearTouches(void *context, MCParameter *p_parameters)
{
	MCscreen -> wait(1/25.0, False, False);
	static_cast<MCScreenDC *>(MCscreen) -> clear_touches();
	MCEventQueueClearTouches();
	return ES_NORMAL;
}
#endif /* MCHandleClearTouchesAndroid */

////////////////////////////////////////////////////////////////////////////////

extern Exec_stat MCHandleRevMail(void *context, MCParameter *parameters);
extern Exec_stat MCHandleCanSendMail(void *context, MCParameter *parameters);
extern Exec_stat MCHandleComposePlainMail(void *context, MCParameter *parameters);
extern Exec_stat MCHandleComposeUnicodeMail(void *context, MCParameter *parameters);
extern Exec_stat MCHandleComposeHtmlMail(void *context, MCParameter *parameters);

extern Exec_stat MCHandleDeviceOrientation(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleOrientation(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleAllowedOrientations(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleSetAllowedOrientations(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleLockOrientation(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleUnlockOrientation(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleOrientationLocked(void *context, MCParameter *p_parameters);

extern Exec_stat MCHandleStartTrackingSensor(void *, MCParameter *);
extern Exec_stat MCHandleStopTrackingSensor(void *, MCParameter *);
extern Exec_stat MCHandleSensorReading(void *, MCParameter *);
extern Exec_stat MCHandleSensorAvailable(void *, MCParameter *);

extern Exec_stat MCHandleCurrentLocation(void *, MCParameter *);
extern Exec_stat MCHandleCurrentHeading(void *, MCParameter *);
extern Exec_stat MCHandleAccelerometerEnablement(void *, MCParameter *);
extern Exec_stat MCHandleCanTrackLocation(void *, MCParameter *);
extern Exec_stat MCHandleCanTrackHeading(void *, MCParameter *);
extern Exec_stat MCHandleLocationTrackingState(void *, MCParameter *);
extern Exec_stat MCHandleHeadingTrackingState(void *, MCParameter *);

extern Exec_stat MCHandlePickDate(void *, MCParameter *);
extern Exec_stat MCHandlePickTime(void *, MCParameter *);
extern Exec_stat MCHandlePickDateAndTime(void *, MCParameter *);
extern Exec_stat MCHandlePick(void *, MCParameter *);

extern Exec_stat MCHandleStartBusyIndicator(void *, MCParameter *);
extern Exec_stat MCHandleStopBusyIndicator(void *, MCParameter *);

extern Exec_stat MCHandleBeep(void *, MCParameter *);
extern Exec_stat MCHandleVibrate(void *, MCParameter *);
extern Exec_stat MCHandleComposeTextMessage(void *, MCParameter *);
extern Exec_stat MCHandleCanComposeTextMessage(void *, MCParameter *);

extern Exec_stat MCHandleControlCreate(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleControlDelete(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleControlSet(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleControlGet(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleControlDo(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleControlTarget(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleControlList(void *context, MCParameter *p_parameters);

// MM-2012-02-09: Add support for multi channel sound syntax
extern Exec_stat MCHandlePlaySoundOnChannel(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandlePausePlayingOnChannel(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleResumePlayingOnChannel(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleStopPlayingOnChannel(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleDeleteSoundChannel(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleSetSoundChannelVolume(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleSoundChannelVolume(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleSoundChannelStatus(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleSoundOnChannel(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleNextSoundOnChannel(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleSoundChannels(void *context, MCParameter *p_parameters);

extern Exec_stat MCHandleCreateLocalNotification(void *, MCParameter *);
extern Exec_stat MCHandleGetRegisteredNotifications(void *, MCParameter *);
extern Exec_stat MCHandleGetNotificationDetails(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleCancelLocalNotification(void *, MCParameter *);
extern Exec_stat MCHandleCancelAllLocalNotifications(void *, MCParameter *);
extern Exec_stat MCHandleGetDeviceToken (void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleGetLaunchUrl (void *context, MCParameter *p_parameters);

// MM-2012-02-22: Added support for ad management
extern Exec_stat MCHandleAdRegister(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleAdCreate(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleAdDelete(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleAdGetVisible(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleAdSetVisible(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleAdGetTopLeft(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleAdSetTopLeft(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleAds(void *context, MCParameter *p_parameters);

extern Exec_stat MCHandlePickContact(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleShowContact(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleGetContactData(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleUpdateContact(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleCreateContact(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleAddContact(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleFindContact(void *context, MCParameter *p_parameters);
extern Exec_stat MCHandleRemoveContact(void *context, MCParameter *p_parameters);

extern Exec_stat MCHandleCreateEvent(void *context, MCParameter *p_parameters);

////////////////////////////////////////////////////////////////////////////////

typedef Exec_stat (*MCPlatformMessageHandler)(void *context, MCParameter *parameters);

struct MCPlatformMessageSpec
{
	const char *message;
	MCPlatformMessageHandler handler;
	void *context;
};

static MCPlatformMessageSpec s_platform_messages[] =
{
    // MM-2012-02-22: Added support for ad management
    {"mobileAdRegister", MCHandleAdRegister, nil},
    {"mobileAdCreate", MCHandleAdCreate, nil},
    {"mobileAdDelete", MCHandleAdDelete, nil},
    {"mobileAdGetVisible", MCHandleAdGetVisible, nil},
    {"mobileAdSetVisible", MCHandleAdSetVisible, nil},
    {"mobileAdGetTopLeft", MCHandleAdGetTopLeft, nil},
    {"mobileAdSetTopLeft", MCHandleAdSetTopLeft, nil},
    {"mobileAds", MCHandleAds, nil},
    
	{"libUrlDownloadToFile", MCHandleLibUrlDownloadToFile, nil},
	
	// MW-2013-10-02: [[ MobileSSLVerify ]] Added support for libUrlSetSSLVerification.
	{"libUrlSetSSLVerification", MCHandleLibUrlSetSSLVerification, nil},
    
    {"mobileStartTrackingSensor", MCHandleStartTrackingSensor, nil},
    {"mobileStopTrackingSensor", MCHandleStopTrackingSensor, nil},
    {"mobileSensorReading", MCHandleSensorReading, nil},
    {"mobileSensorAvailable", MCHandleSensorAvailable, nil},	
    
    // MM-2012-02-11: Added support old style senseor syntax
	/* DEPRECATED */ {"mobileCanTrackLocation", MCHandleCanTrackLocation, nil},
    /* DEPRECATED */ {"mobileStartTrackingLocation", MCHandleLocationTrackingState, (void *)true},
	/* DEPRECATED */ {"mobileStopTrackingLocation", MCHandleLocationTrackingState, (void *)false},
	/* DEPRECATED */ {"mobileCurrentLocation", MCHandleCurrentLocation, nil},
	
	/* DEPRECATED */ {"mobileCanTrackHeading", MCHandleCanTrackHeading, nil},
    /* DEPRECATED */ {"mobileStartTrackingHeading", MCHandleHeadingTrackingState, (void *)true},
	/* DEPRECATED */ {"mobileStopTrackingHeading", MCHandleHeadingTrackingState, (void *)false},
	/* DEPRECATED */ {"mobileCurrentHeading", MCHandleCurrentHeading, nil},
    
	/* DEPRECATED */ {"mobileEnableAccelerometer", MCHandleAccelerometerEnablement, (void *)true},
	/* DEPRECATED */ {"mobileDisableAccelerometer", MCHandleAccelerometerEnablement, (void *)false},

    {"mobileBusyIndicatorStart", MCHandleStartBusyIndicator, nil},
    {"mobileBusyIndicatorStop", MCHandleStopBusyIndicator, nil},
    
    {"mobileBeep", MCHandleBeep, nil},
    {"mobileVibrate", MCHandleVibrate, nil},
    {"mobileComposeTextMessage", MCHandleComposeTextMessage, nil},
    {"mobileCanComposeTextMessage", MCHandleCanComposeTextMessage, nil},
    
	{"mobileCameraFeatures", MCHandleCameraFeatures, nil},
	{"mobilePickPhoto", MCHandlePickPhoto, nil},
	
    {"mobilePickDate", MCHandlePickDate, nil},
    {"mobilePickTime", MCHandlePickTime, nil},
    {"mobilePickDateAndTime", MCHandlePickDateAndTime, nil},
    
    {"mobilePick", MCHandlePick, nil},
    
    {"mobilePickMedia", MCHandlePickMedia, nil},

	{"revMail", MCHandleRevMail, nil},
	{"mobileCanSendMail", MCHandleCanSendMail, nil},
	{"mobileComposeMail", MCHandleComposePlainMail, nil},
	{"mobileComposeUnicodeMail", MCHandleComposeUnicodeMail, nil},
	{"mobileComposeHtmlMail", MCHandleComposeHtmlMail, nil},

	{"mobileDeviceOrientation", MCHandleDeviceOrientation, nil},
	{"mobileOrientation", MCHandleOrientation, nil},
	{"mobileAllowedOrientations", MCHandleAllowedOrientations, nil},
	{"mobileSetAllowedOrientations", MCHandleSetAllowedOrientations, nil},
	{"mobileLockOrientation", MCHandleLockOrientation, nil},
	{"mobileUnlockOrientation", MCHandleUnlockOrientation, nil},
	{"mobileOrientationLocked", MCHandleOrientationLocked, nil},
	
	{"mobileSetKeyboardType", MCHandleSetKeyboardType, nil},

	{"mobileShowStatusbar", MCHandleSetStatusbarVisibility, (void*)1},
	{"mobileHideStatusbar", MCHandleSetStatusbarVisibility, (void*)0},

	{"mobilePixelDensity", MCHandlePixelDensity, nil},

	{"mobileBuildInfo", MCHandleBuildInfo, nil},

    {"mobileCanMakePurchase", MCHandleCanMakePurchase, nil},
	{"mobileEnablePurchaseUpdates", MCHandleEnablePurchaseUpdates, nil},
	{"mobileDisablePurchaseUpdates", MCHandleDisablePurchaseUpdates, nil},
	{"mobileRestorePurchases", MCHandleRestorePurchases, nil},
	{"mobilePurchases", MCHandlePurchaseList, nil},
	{"mobilePurchaseCreate", MCHandlePurchaseCreate, nil},
	{"mobilePurchaseState", MCHandlePurchaseState, nil},
	{"mobilePurchaseError", MCHandlePurchaseError, nil},
	{"mobilePurchaseGet", MCHandlePurchaseGet, nil},
	{"mobilePurchaseSet", MCHandlePurchaseSet, nil},
	{"mobilePurchaseSendRequest", MCHandlePurchaseSendRequest, nil},
	{"mobilePurchaseConfirmDelivery", MCHandlePurchaseConfirmDelivery, nil},
    {"mobilePurchaseVerify", MCHandlePurchaseVerify, nil},
    
	{"mobileControlCreate", MCHandleControlCreate, nil},
	{"mobileControlDelete", MCHandleControlDelete, nil},
	{"mobileControlSet", MCHandleControlSet, nil},
	{"mobileControlGet", MCHandleControlGet, nil},
	{"mobileControlDo", MCHandleControlDo, nil},
	{"mobileControlTarget", MCHandleControlTarget, nil},
	{"mobileControls", MCHandleControlList, nil},

	{"mobilePreferredLanguages", MCHandlePreferredLanguages, nil},
	{"mobileCurrentLocale", MCHandleCurrentLocale, nil},
    
    // MM-2012-02-09: Add support for multi channel sound syntax
    {"mobilePlaySoundOnChannel", MCHandlePlaySoundOnChannel, nil},
	{"mobilePausePlayingOnChannel", MCHandlePausePlayingOnChannel},
	{"mobileResumePlayingOnChannel", MCHandleResumePlayingOnChannel},
	{"mobileStopPlayingOnChannel", MCHandleStopPlayingOnChannel, nil},
	{"mobileDeleteSoundChannel", MCHandleDeleteSoundChannel, nil},
	{"mobileSetSoundChannelVolume", MCHandleSetSoundChannelVolume, nil},
	{"mobileSoundChannelVolume", MCHandleSoundChannelVolume, nil},
	{"mobileSoundChannelStatus", MCHandleSoundChannelStatus, nil},
	{"mobileSoundOnChannel", MCHandleSoundOnChannel, nil},
	{"mobileNextSoundOnChannel", MCHandleNextSoundOnChannel, nil},
	{"mobileSoundChannels", MCHandleSoundChannels, nil},
    
	{"mobileLockIdleTimer", MCHandleLockIdleTimer, nil},
	{"mobileUnlockIdleTimer", MCHandleUnlockIdleTimer, nil},
	{"mobileIdleTimerLocked", MCHandleIdleTimerLocked, nil},
    
    {"mobileCreateLocalNotification", MCHandleCreateLocalNotification, nil},
    {"mobileGetRegisteredNotifications", MCHandleGetRegisteredNotifications, nil},
    {"mobileGetNotificationDetails", MCHandleGetNotificationDetails, nil},
    {"mobileCancelLocalNotification", MCHandleCancelLocalNotification, nil},
    {"mobileCancelAllLocalNotifications", MCHandleCancelAllLocalNotifications, nil},
    
    {"mobileGetDeviceToken", MCHandleGetDeviceToken, nil},
    {"mobileGetLaunchUrl", MCHandleGetLaunchUrl, nil},
    
    {"mobileExportImageToAlbum", MCHandleExportImageToAlbum, nil},
        
    {"mobilePickContact", MCHandlePickContact, nil},
    {"mobileShowContact", MCHandleShowContact, nil},
    {"mobileGetContactData", MCHandleGetContactData, nil},
    {"mobileUpdateContact", MCHandleUpdateContact, nil},
    {"mobileCreateContact", MCHandleCreateContact, nil},
    {"mobileAddContact", MCHandleAddContact, nil},
    {"mobileFindContact", MCHandleFindContact, nil},
    {"mobileRemoveContact", MCHandleRemoveContact, nil},

    {"mobileCreateEvent", MCHandleCreateEvent, nil},
	
	{"mobileClearTouches", MCHandleClearTouches, nil},
    
    {nil, nil, nil}
};

Exec_stat MCHandlePlatformMessage(Handler_type p_type, const MCString& p_message, MCParameter *p_parameters)
{
	for(uint32_t i = 0; s_platform_messages[i] . message != nil; i++)
		if (p_message == s_platform_messages[i] . message)
			return s_platform_messages[i] . handler(s_platform_messages[i] . context, p_parameters);
	
	return ES_NOT_HANDLED;
}

////////////////////////////////////////////////////////////////////////////////

// AL-2013-14-07 [[ Bug 10445 ]] Sort international on Android
int MCSystemCompareInternational(const char *p_left, const char *p_right)
{
    int32_t t_compare;
    MCAndroidEngineCall("compareInternational", "iss", &t_compare, p_left, p_right);
    return t_compare;
}
