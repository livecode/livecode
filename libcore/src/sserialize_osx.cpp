/* Copyright (C) 2009-2015 LiveCode Ltd.

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

/*
 *  sserialize_osx.cpp
 *  libcore
 *
 *  Created by Ian Macphail on 22/10/2009.
 *
 */

#include "sserialize_osx.h"

#include "sserialize.h"

////////////////////////////////////////////////////////////////////////////////

typedef OSStatus (*PMPrintSettingsCreateDataRepresentationProcPtr)(PMPrintSettings settings, CFDataRef* r_data, uint32_t format);
typedef OSStatus (*PMPageFormatCreateDataRepresentationProcPtr)(PMPageFormat settings, CFDataRef* r_data, uint32_t format);
typedef OSStatus (*PMPrintSettingsCreateWithDataRepresentationProcPtr)(CFDataRef data, PMPrintSettings *settings);
typedef OSStatus (*PMPageFormatCreateWithDataRepresentationProcPtr)(CFDataRef data, PMPageFormat *settings);

static bool s_has_weaklinked = false;
static CFBundleRef s_appservices_bundle = nil;
static PMPrintSettingsCreateDataRepresentationProcPtr PMPrintSettingsCreateDataRepresentationProc = nil;
static PMPageFormatCreateDataRepresentationProcPtr PMPageFormatCreateDataRepresentationProc = nil;
static PMPrintSettingsCreateWithDataRepresentationProcPtr PMPrintSettingsCreateWithDataRepresentationProc = nil;
static PMPageFormatCreateWithDataRepresentationProcPtr PMPageFormatCreateWithDataRepresentationProc = nil;

static void weaklink(void)
{
	if (s_has_weaklinked)
		return;
	
	s_has_weaklinked = true;
	
	s_appservices_bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.ApplicationServices"));
	if (s_appservices_bundle == nil ||
		!CFBundleLoadExecutable(s_appservices_bundle))
		return;
	
	PMPrintSettingsCreateDataRepresentationProc = (PMPrintSettingsCreateDataRepresentationProcPtr)CFBundleGetFunctionPointerForName(s_appservices_bundle, CFSTR("PMPrintSettingsCreateDataRepresentation"));
	PMPageFormatCreateDataRepresentationProc = (PMPageFormatCreateDataRepresentationProcPtr)CFBundleGetFunctionPointerForName(s_appservices_bundle, CFSTR("PMPageFormatCreateDataRepresentation"));
	PMPrintSettingsCreateWithDataRepresentationProc = (PMPrintSettingsCreateWithDataRepresentationProcPtr)CFBundleGetFunctionPointerForName(s_appservices_bundle, CFSTR("PMPrintSettingsCreateWithDataRepresentation"));
	PMPageFormatCreateWithDataRepresentationProc = (PMPageFormatCreateWithDataRepresentationProcPtr)CFBundleGetFunctionPointerForName(s_appservices_bundle, CFSTR("PMPageFormatCreateWithDataRepresentation"));
}

////////////////////////////////////////////////////////////////////////////////

bool serialize_cfdata(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, CFDataRef p_data)
{
	if (p_data == nil)
		return serialize_data(r_stream, r_stream_size, r_offset, nil, 0);
	return serialize_data(r_stream, r_stream_size, r_offset, CFDataGetBytePtr(p_data), CFDataGetLength(p_data));
}

bool deserialize_cfdata(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, CFDataRef &r_data)
{
	void *t_data = nil;
	uint32_t t_data_size = 0;
	CFDataRef t_data_ref = nil;
	bool t_success = deserialize_data(p_stream, p_stream_size, r_offset, t_data, t_data_size);
	if (t_success && t_data != nil)
	{
		t_data_ref = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (UInt8*)t_data, t_data_size, kCFAllocatorDefault);
		t_success = (t_data_ref != nil);
	}
	if (t_success)
		r_data = t_data_ref;
	return t_success;
}

bool serialize_cfstring(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, CFStringRef p_string)
{
	bool t_success = true;
	CFDataRef t_data_ref = nil;

	if (p_string == nil)
		return serialize_cfdata(r_stream, r_stream_size, r_offset, nil);
	t_data_ref = CFStringCreateExternalRepresentation(kCFAllocatorDefault, p_string, kCFStringEncodingUTF8, 0);
	t_success = (t_data_ref != nil);

	if (t_success)
	{
		t_success = serialize_cfdata(r_stream, r_stream_size, r_offset, t_data_ref);
		CFRelease(t_data_ref);
	}
	return t_success;
}

bool deserialize_cfstring(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, CFStringRef &r_string)
{
	bool t_success = true;
	CFStringRef t_string = nil;
	CFDataRef t_data;
	
	t_success = deserialize_cfdata(p_stream, p_stream_size, r_offset, t_data);
	if (t_success && t_data != nil)
	{
		t_string = CFStringCreateFromExternalRepresentation(kCFAllocatorDefault, t_data, kCFStringEncodingUTF8);
		CFRelease(t_data);
		t_success = (t_string != nil);
	}
	
	if (t_success)
		r_string = t_string;
	
	return t_success;
}

#ifndef _MACOSX_NOCARBON

bool serialize_handle(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, Handle p_data)
{
	bool t_success = true;
	HLock(p_data);
	t_success = serialize_data(r_stream, r_stream_size, r_offset, (char *)*p_data, GetHandleSize(p_data));
	HUnlock(p_data);
	return t_success;
}

bool deserialize_handle(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, Handle &r_handle)
{
	void *t_data = nil;
	uint32_t t_data_size = 0;
	Handle t_handle = nil;
	bool t_success = deserialize_data(p_stream, p_stream_size, r_offset, t_data, t_data_size);
	if (t_success && t_data != nil)
	{
		t_success = (PtrToHand(t_data, &t_handle, t_data_size) == noErr);
		MCMemoryDeallocate(t_data);
	}
	if (t_success)
		r_handle = t_handle;
	return t_success;
}

#endif

////////////////////////////////////////////////////////////////////////////////

bool serialize_printer_settings(char *&r_stream, uint32_t &r_stream_size, PMPrintSession p_session, PMPrinter p_printer, PMPrintSettings p_settings, PMPageFormat p_format)
{
	bool t_success = true;

	weaklink();
	
	CFStringRef t_string;
	t_string = PMPrinterGetID(p_printer);

	uint32_t t_offset = 0;

	t_success = (t_string != nil);
	if (t_success)
		t_success = serialize_cfstring(r_stream, r_stream_size, t_offset, t_string);

	if (PMPrintSettingsCreateDataRepresentationProc != nil)
	{
		CFDataRef t_data = nil;
		if (t_success)
			t_success = (PMPrintSettingsCreateDataRepresentationProc(p_settings, &t_data, 0) == noErr);
		if (t_success)
		{
			t_success = serialize_cfdata(r_stream, r_stream_size, t_offset, t_data);
			CFRelease(t_data);
		}
		
		if (t_success)
			t_success = (PMPageFormatCreateDataRepresentationProc(p_format, &t_data, 0) == noErr);
		if (t_success)
		{
			t_success = serialize_cfdata(r_stream, r_stream_size, t_offset, t_data);
			CFRelease(t_data);
		}
		
	}
#ifndef _MACOSX_NOCARBON
	else
	{
		Handle t_handle = nil;
		if (t_success)
			t_success = (PMFlattenPrintSettings(p_settings, &t_handle) == noErr);
		if (t_success)
		{
			t_success = serialize_handle(r_stream, r_stream_size, t_offset, t_handle);
			DisposeHandle(t_handle);
		}
		
		if (t_success)
			t_success = (PMFlattenPageFormat(p_format, &t_handle) == noErr);
		if (t_success)
		{
			t_success = serialize_handle(r_stream, r_stream_size, t_offset, t_handle);
			DisposeHandle(t_handle);
		}
	}
#endif

	PMDestinationType t_dest_type;
	CFURLRef t_dest_url;
	if (t_success)
		t_success = (PMSessionGetDestinationType(p_session, p_settings, &t_dest_type) == noErr);
	if (t_success)
		t_success = serialize_uint32(r_stream, r_stream_size, t_offset, (uint32_t)t_dest_type);
	if (t_success)
		t_success = (PMSessionCopyDestinationLocation(p_session, p_settings, &t_dest_url) == noErr);
	if (t_success)
	{
		if (t_dest_url == NULL)
			t_success = serialize_cfstring(r_stream, r_stream_size, t_offset, NULL);
		else
		{
			t_success = serialize_cfstring(r_stream, r_stream_size, t_offset, CFURLGetString(t_dest_url));
			CFRelease(t_dest_url);
		}
	}
	if (t_success)
		t_success = (PMSessionCopyDestinationFormat(p_session, p_settings, &t_string) == noErr);
	if (t_success)
	{
		t_success = serialize_cfstring(r_stream, r_stream_size, t_offset, t_string);
		if (t_string != NULL)
			CFRelease(t_string);
	}
	return t_success;
}

bool deserialize_printer_settings(const char *p_stream, uint32_t p_stream_size, PMPrintSession &r_session, PMPrinter &r_printer, PMPrintSettings &r_settings, PMPageFormat &r_format)
{
	bool t_success = true;
	
	weaklink();
	
	CFStringRef t_string;
	PMPrinter t_printer = NULL;
	PMPrintSettings t_settings = NULL;
	PMPageFormat t_format = NULL;
	PMPrintSession t_session = NULL;
	
	uint32_t t_offset = 0;
	
	t_success = deserialize_cfstring(p_stream, p_stream_size, t_offset, t_string);
	if (t_success)
	{
		t_printer = PMPrinterCreateFromPrinterID(t_string);
		CFRelease(t_string);
		t_success = (t_printer != NULL);
	}
	
	if (PMPrintSettingsCreateDataRepresentationProc != nil)
	{
		CFDataRef t_data;
		
		if (t_success)
			t_success = deserialize_cfdata(p_stream, p_stream_size, t_offset, t_data);
		if (t_success)
		{
			t_success = (PMPrintSettingsCreateWithDataRepresentationProc(t_data, &t_settings) == noErr);
			CFRelease(t_data);
		}
		
		if (t_success)
			t_success = deserialize_cfdata(p_stream, p_stream_size, t_offset, t_data);
		if (t_success)
		{
			t_success = (PMPageFormatCreateWithDataRepresentationProc(t_data, &t_format) == noErr);
			CFRelease(t_data);
		}
	}
#ifndef _MACOSX_NOCARBON
	else
	{
		Handle t_handle;
		
		if (t_success)
			t_success = deserialize_handle(p_stream, p_stream_size, t_offset, t_handle);
		if (t_success)
		{
			t_success = (PMUnflattenPrintSettings(t_handle, &t_settings) == noErr);
			DisposeHandle(t_handle);
		}
		
		if (t_success)
			t_success = deserialize_handle(p_stream, p_stream_size, t_offset, t_handle);
		if (t_success)
		{
			t_success = (PMUnflattenPageFormat(t_handle, &t_format) == noErr);
			DisposeHandle(t_handle);
		}
	}	
#endif
	
	if (t_success && r_settings != NULL)
	{
		t_success = (PMCopyPrintSettings(t_settings, r_settings) == noErr);
		if (t_success)
		{
			PMRelease(t_settings);
			t_settings = r_settings;
		}
	}
	
	if (t_success && r_format != NULL)
	{
		t_success = (PMCopyPageFormat(t_format, r_format) == noErr);
		if (t_success)
		{
			PMRelease(t_format);
			t_format = r_format;
		}
	}
	
	if (t_success)
	{
		if (r_session == NULL)
			t_success = (PMCreateSession(&t_session) == noErr);
		else
			t_session = r_session;
	}
	
	PMDestinationType t_dest_type;
	CFStringRef t_dest_format_string = NULL, t_dest_location_string = NULL;
	CFURLRef t_dest_location_url = NULL;
	if (t_success)
	{
		uint32_t t_val;
		t_success = deserialize_uint32(p_stream, p_stream_size, t_offset, t_val);
		t_dest_type = (PMDestinationType)t_val;
	}
	if (t_success)
		t_success = deserialize_cfstring(p_stream, p_stream_size, t_offset, t_dest_location_string);
	if (t_success && t_dest_location_string != NULL)
		t_success = (t_dest_location_url = CFURLCreateWithString(kCFAllocatorDefault, t_dest_location_string, NULL)) != NULL;
	if (t_success)
		t_success = deserialize_cfstring(p_stream, p_stream_size, t_offset, t_dest_format_string);
	if (t_success)
		t_success = (PMSessionSetDestination(t_session, t_settings, t_dest_type, t_dest_format_string, t_dest_location_url) ==  noErr);
	if (t_dest_location_string != NULL)
		CFRelease(t_dest_location_string);
	if (t_dest_location_url != NULL)
		CFRelease(t_dest_location_url);
	if (t_dest_format_string != NULL)
		CFRelease(t_dest_format_string);
		
	if (t_success)
	{
		r_session = t_session;
		r_printer = t_printer;
		r_settings = t_settings;
		r_format = t_format;
	}
	else
	{
		if (t_session != NULL && t_session != r_session)
			PMRelease(t_session);
		if (t_printer != NULL)
			PMRelease(t_printer);
		if (t_settings != NULL && t_settings != r_settings)
			PMRelease(t_settings);
		if (t_format != NULL && t_format != r_format)
			PMRelease(t_format);
	}
	return t_success;
}
////////////////////////////////////////////////////////////////////////////////
