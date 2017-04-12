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

#include "param.h"
#include "mcerror.h"

#include "exec.h"
#include "util.h"
#include "object.h"
#include "socket.h"

#include "globals.h"
#include "text.h"
#include "textlayout.h"

#include "system.h"
#include "srvdebug.h"
#include "srvmain.h"

#include "mcssl.h"

#include <signal.h>

#define CURL_STATICLIB
#include <curl/curl.h>

////////////////////////////////////////////////////////////////////////////////

Boolean tripleclick = False;

uint32_t g_current_background_colour = 0;

////////////////////////////////////////////////////////////////////////////////

typedef const char *(*MCUrlExecuteCallback)(void *state, CURL *curl_handle);

extern char *strndup(const char *, size_t);

static bool url_build_header_list(const char *p_list, curl_slist*& r_headers)
{
	bool t_success;
	t_success = true;
	
	curl_slist *t_slist;
	t_slist = NULL;
	
	const char *t_ptr;
	t_ptr = p_list;
	while(t_success)
	{
		const char *t_break;
		t_break = strchr(t_ptr, '\n');
		if (t_break == NULL)
			t_break = t_ptr + strlen(t_ptr);
		
		if (t_break - t_ptr != 0)
		{
			char *t_header;
			t_header = strndup(t_ptr, t_break - t_ptr);
			if (t_header == NULL)
				t_success = false;
			
			curl_slist *t_new_slist;
			t_new_slist = NULL;
			if (t_success)
			{
				t_new_slist = curl_slist_append(t_slist, t_header);
				if (t_new_slist == NULL)
					t_success = false;
			}
			
			if (t_success)
				t_slist = t_new_slist;
			
			free(t_header);
		}
		
		if (*t_break == '\0')
			break;
		
		t_ptr = t_break + 1;
	}
	
	if (t_success)
		r_headers = t_slist;
	else if (t_slist != NULL)
		curl_slist_free_all(t_slist);
	
	return t_success;
}

static size_t url_write_callback(void *p_buffer, size_t p_size, size_t p_count, void *p_context)
{
	MCAutoStringRef t_string;
	/* UNCHECKED */ MCStringCreateWithNativeChars((const char_t *)p_buffer, p_size * p_count, &t_string);
	MCExecContext ctxt;
	MCurlresult -> set(ctxt, *t_string, kMCVariableSetAfter);
	return p_count;
}

// IM-2014-07-28: [[ Bug 12822 ]] Setup CURL ssl certificates using shared loading function.
static CURLcode _set_ssl_certificates_callback(CURL *p_curl, void *p_ssl_ctx, void *p_context)
{
	/* UNCHECKED */ MCSSLContextLoadCertificates((SSL_CTX*)p_ssl_ctx, nil);
	
	return CURLE_OK;
}

static void url_execute(MCStringRef p_url, MCUrlExecuteCallback p_callback, void *p_state, MCStringRef& r_error)
{
	const char *t_error;
	t_error = NULL;
	
	bool t_is_http, t_is_https;
	t_is_http = MCStringBeginsWithCString(p_url, (const char_t*)"http", kMCCompareExact);
	t_is_https = MCStringBeginsWithCString(p_url, (const char_t*)"https", kMCCompareExact);
	
	curl_slist *t_headers;
	t_headers = NULL;
	if (t_error == NULL && !MCStringIsEmpty(MChttpheaders) && t_is_http)
	{
		if (!url_build_header_list(MCStringGetCString(MChttpheaders), t_headers))
			t_error = "couldn't build header list";
	}
	
	CURL *t_url_handle;
	t_url_handle = NULL;
	if (t_error == NULL)
	{
		t_url_handle = curl_easy_init();
		if (t_url_handle == NULL)
			t_error = "couldn't create handle";
	}
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(t_url_handle, CURLOPT_URL, MCStringGetCString(p_url)) != CURLE_OK)
			t_error = "couldn't set url";
	}
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(t_url_handle, CURLOPT_WRITEFUNCTION, url_write_callback) != CURLE_OK)
			t_error = "couldn't set callback";
	}
	
	if (t_error == NULL && t_headers != NULL)
	{
		if (curl_easy_setopt(t_url_handle, CURLOPT_HTTPHEADER, t_headers) != CURLE_OK)
			t_error = "couldn't set headers";
	}
	
	if (t_error == NULL && t_is_https)
	{
		// IM-2014-07-28: [[ Bug 12822 ]] Override default ssl certificate loading.
		if (curl_easy_setopt(t_url_handle, CURLOPT_SSL_VERIFYPEER, 1) != CURLE_OK ||
			curl_easy_setopt(t_url_handle, CURLOPT_SSL_VERIFYHOST, 2) != CURLE_OK
#if defined(_LINUX) || defined(_WIN32)
            // These options are not supported when using the OSX system libcurl
            // as it uses the OS' certificate database and not a cert file.
			|| curl_easy_setopt(t_url_handle, CURLOPT_CAINFO, nil) != CURLE_OK
			|| curl_easy_setopt(t_url_handle, CURLOPT_SSL_CTX_FUNCTION, _set_ssl_certificates_callback) != CURLE_OK
#endif
            )
			t_error = "couldn't configure ssl";
	}

	if (t_error == NULL)
	{
#if LIBCURL_VERSION_MINOR >= 19
		if (curl_easy_setopt(t_url_handle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS) != CURLE_OK ||
			curl_easy_setopt(t_url_handle, CURLOPT_FOLLOWLOCATION, 1) != CURLE_OK)
			t_error = "couldn't configure follow";
#endif
	}
	
	// MM-2011-07-07: Added support for specifying which network interface to use.
	if (t_error == NULL)
	{
		if (MCdefaultnetworkinterface != NULL)
			if (curl_easy_setopt(t_url_handle, CURLOPT_INTERFACE, MCdefaultnetworkinterface) != CURLE_OK)
				t_error = "couldn't set network interface";
	}
	
	if (t_error == NULL && p_callback != NULL)
		t_error = p_callback(p_state, t_url_handle);
	
	if (t_error == NULL)
	{
		char t_error_buffer[CURL_ERROR_SIZE];
		curl_easy_setopt(t_url_handle, CURLOPT_ERRORBUFFER, t_error_buffer);

		MCurlresult -> clear();
		MCresult -> clear();
		if (curl_easy_perform(t_url_handle) == CURLE_OK)
		{
			if (t_is_http)
			{
				long t_code;
				if (curl_easy_getinfo(t_url_handle, CURLINFO_RESPONSE_CODE, &t_code) == CURLE_OK)
				{
					if (t_code != 200)
					{
						static char t_error_str[64];
						sprintf(t_error_str, "error %ld", t_code);
						MCresult -> copysvalue(t_error_str);
					}
				}
				else
					t_error = "couldn't fetch response code";
			}
		}
		else
		{
			static char t_error_str[CURL_ERROR_SIZE + 64];
			sprintf(t_error_str, "error %s", t_error_buffer);
			MCresult -> copysvalue(t_error_str);
		}
	}
	
	if (t_error == NULL && t_is_http)
	{
	}
	
	if (t_url_handle != NULL)
		curl_easy_cleanup(t_url_handle);
	
	if (t_headers != NULL)
		curl_slist_free_all(t_headers);
	
	/* UNCHECKED */ MCStringCreateWithCString(t_error, r_error);
	
}

void MCS_geturl(MCObject *p_target, MCStringRef p_url)
{

	MCAutoStringRef t_error;
	
	if (MCStringIsEmpty(*t_error))
	{
		if (!MCStringBeginsWithCString(p_url, (const char_t*)"https:", kMCCompareExact) && !MCStringBeginsWithCString(p_url, (const char_t*)"http:", kMCCompareExact) && !MCStringBeginsWithCString(p_url, (const char_t*)"ftp:", kMCCompareExact))
			/* UNCHECKED */ MCStringCreateWithCString("unsupported protocol", &t_error);
	}
	
	if (MCStringIsEmpty(*t_error))
	{
		url_execute(p_url, NULL, NULL, &t_error);
	}
	
	if (!MCStringIsEmpty(*t_error))
	{
		MCurlresult -> clear();
        MCresult -> setvalueref(*t_error);
	}
}

static const char *url_execute_post(void *p_state, CURL *p_curl)
{
	MCDataRef state;
	state = static_cast<MCDataRef>(p_state);
	
	const char *t_error;
	t_error = NULL;
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(p_curl, CURLOPT_POST, 1) != CURLE_OK)
			t_error = "couldn't configure post";
	}
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(p_curl, CURLOPT_POSTFIELDS, MCDataGetBytePtr(state)) != CURLE_OK)
			t_error = "couldn't set post data";
	}
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(p_curl, CURLOPT_POSTFIELDSIZE, MCDataGetLength(state)) != CURLE_OK)
			t_error = "couldn't set post size";
	}
	
	return t_error;
}

void MCS_posttourl(MCObject *p_target, MCDataRef p_data, MCStringRef p_url)
{
	MCAutoStringRef t_error;

	if (MCStringIsEmpty(*t_error))
	{
		if (!MCStringBeginsWithCString(p_url, (const char_t*)"https:", kMCCompareExact) && !MCStringBeginsWithCString(p_url, (const char_t*)"http:", kMCCompareExact))
			/* UNCHECKED */ MCStringCreateWithCString("unsupported protocol", &t_error);
	}
	
	if (MCStringIsEmpty(*t_error))
	{
		url_execute(p_url, url_execute_post, (void *)p_data, &t_error);
	}

	if (!MCStringIsEmpty(*t_error))
	{
		MCurlresult -> clear();
        MCresult -> setvalueref(*t_error);
	}
}

static const char *url_execute_upload(void *p_state, CURL *p_curl)
{
  MCDataRef state;
	state = static_cast<MCDataRef>(p_state);
	
	const char *t_error;
	t_error = NULL;
	
	if (t_error == NULL)
	{

	if (curl_easy_setopt(p_curl, CURLOPT_CUSTOMREQUEST, "PUT") != CURLE_OK)
		return "couldn't configure upload";
	}
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(p_curl, CURLOPT_POSTFIELDS, MCDataGetBytePtr(state)) != CURLE_OK)
			t_error = "couldn't set upload data";
	}
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(p_curl, CURLOPT_POSTFIELDSIZE, MCDataGetLength(state)) != CURLE_OK)
			t_error = "couldn't set upload size";
	}
	
	return t_error;
}

void MCS_putintourl(MCObject *p_target, MCDataRef p_data, MCStringRef p_url)
{
	MCAutoStringRef t_error;
		
	if (MCStringIsEmpty(*t_error))
	{
		if (!MCStringBeginsWithCString(p_url, (const char_t*)"https:", kMCCompareExact) && !MCStringBeginsWithCString(p_url, (const char_t*)"http:", kMCCompareExact) && !MCStringBeginsWithCString(p_url, (const char_t*)"ftp:", kMCCompareExact))
			/* UNCHECKED */ MCStringCreateWithCString("unsupported protocol", &t_error);
	}
	
	if (MCStringIsEmpty(*t_error))
    {
      url_execute(p_url, url_execute_upload, (void *)p_data, &t_error);
	}
	
	if (!MCStringIsEmpty(*t_error))
	{
		MCurlresult -> clear();
        MCresult -> setvalueref(*t_error);
	}
}

static const char *url_execute_http_delete(void *p_state, CURL *p_curl)
{
	if (curl_easy_setopt(p_curl, CURLOPT_CUSTOMREQUEST, "DELETE") != CURLE_OK)
		return "couldn't configure delete";
	return NULL;
}

static const char *url_execute_ftp_delete(void *p_state, CURL *p_curl)
{
	const char *t_url;
	t_url = static_cast<const char *>(p_state);
	
	curl_slist *t_command;
	t_command = NULL;
	
	bool t_is_folder;
	t_is_folder = t_url[strlen(t_url) - 1] == '/';
	
	if (t_is_folder)
		t_command = curl_slist_append(t_command, "RMD .");
	else
	{
		const char *t_file;
		t_file = strrchr(t_url, '/') + 1;

		char *t_cmd_string;
		t_cmd_string = new (nothrow) char[strlen(t_file) + 5 + 1];
		if (t_cmd_string != NULL)
		{
			sprintf(t_cmd_string, "%s %s", (t_is_folder ? "RMD" : "DELE"), t_file); 
			t_command = curl_slist_append(t_command, t_cmd_string);
			delete t_cmd_string;
		}
	}
	
	if (t_command == NULL)
		return "couldn't build command";
	
	if (curl_easy_setopt(p_curl, CURLOPT_NOBODY, 1) != CURLE_OK ||
		curl_easy_setopt(p_curl, CURLOPT_POSTQUOTE, t_command) != CURLE_OK)
		return "couldn't configure delete";
	
	return NULL;
}

void MCS_deleteurl(MCObject *p_target, MCStringRef p_url)
{
	MCAutoStringRef t_error;
	
	if (MCStringBeginsWithCString(p_url, (const char_t*)"http://", kMCCompareExact) || MCStringBeginsWithCString(p_url, (const char_t*)"https://", kMCCompareExact))
		url_execute(p_url, url_execute_http_delete, NULL, &t_error);
	else if (MCStringBeginsWithCString(p_url, (const char_t*)"ftp://", kMCCompareExact))
		url_execute(p_url, url_execute_ftp_delete, (void *) MCStringGetCString(p_url), &t_error);
	else
		/* UNCHECKED */ MCStringCreateWithCString("unsupported protocol", &t_error);

	if (!MCStringIsEmpty(*t_error))
	{
		MCurlresult -> clear();
        MCresult -> setvalueref(*t_error);
	}
}

void MCS_unloadurl(MCObject *p_object, MCStringRef p_url)
{
}

void MCS_loadurl(MCObject *p_object, MCStringRef p_url, MCNameRef p_message)
{
}

////////////////////////////////////////////////////////////////////////////////

extern void MCServerPutOutput(MCStringRef data);
extern void MCServerPutUnicodeOutput(MCDataRef data);
extern void MCServerPutBinaryOutput(MCDataRef data);
extern void MCServerPutHeader(MCStringRef data, bool add);
extern void MCServerPutContent(MCStringRef data);
extern void MCServerPutUnicodeContent(MCDataRef data);
extern void MCServerPutMarkup(MCStringRef data);
extern void MCServerPutUnicodeMarkup(MCDataRef data);

bool MCS_put(MCExecContext &ctxt, MCSPutKind p_kind, MCStringRef p_data_ref)
{
    switch(p_kind)
	{
	case kMCSPutOutput:
	case kMCSPutBeforeMessage:
	case kMCSPutIntoMessage:
	case kMCSPutAfterMessage:
		MCServerPutOutput(p_data_ref);
		break;
			
	case kMCSPutHeader:
		MCServerPutHeader(p_data_ref, false);
		break;

	case kMCSPutNewHeader:
		MCServerPutHeader(p_data_ref, true);
		break;

	case kMCSPutContent:
		MCServerPutContent(p_data_ref);
		break;

	case kMCSPutMarkup:
		MCServerPutMarkup(p_data_ref);
		break;
			
	default:
		break;
	}

	return true;
}


bool MCS_put_binary(MCExecContext& ctxt, MCSPutKind p_kind, MCDataRef p_data)
{
	
	switch(p_kind)
	{		
	case kMCSPutBinaryOutput:
		MCServerPutBinaryOutput(p_data);
		break;

	default:
		break;
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////

void MCS_set_errormode(MCSErrorMode p_mode)
{
	MCservererrormode = p_mode;
}

MCSErrorMode MCS_get_errormode(void)
{
	return MCservererrormode;
}

////////////////////////////////////////////////////////////////////////////////

void MCS_set_outputtextencoding(MCSOutputTextEncoding encoding)
{
	MCserveroutputtextencoding = encoding;
}

MCSOutputTextEncoding MCS_get_outputtextencoding(void)
{
	return MCserveroutputtextencoding;
}

void MCS_set_outputlineendings(MCSOutputLineEndings ending)
{
	MCserveroutputlineendings = ending;
}

MCSOutputLineEndings MCS_get_outputlineendings(void)
{
	return MCserveroutputlineendings;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemLaunchUrl(MCStringRef p_url)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCTextLayoutInitialize(void)
{
	return false;
}

void MCTextLayoutFinalize(void)
{
}

bool MCTextLayout(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font, MCTextLayoutCallback p_callback, void *p_context)
{	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

int MCA_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	return 0;
}

int MCA_ask_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	return 0;
}

int MCA_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	return 0;
}

int MCA_ask_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	return 0;
}

int MCA_folder(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	return 0;
}

bool MCA_color(MCStringRef title, MCColor initial_color, bool as_sheet, bool& r_chosen, MCColor& r_chosen_color)
{
	return true;
}

// MERG-2013-08-18: Stubs for colorDialogColors.
void MCA_setcolordialogcolors(MCColor* p_list, uindex_t p_count)
{
    
}

void MCA_getcolordialogcolors(MCColor*& r_colors, uindex_t& r_count)
{
    r_count = 0;
}


////////////////////////////////////////////////////////////////////////////////
