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

#include "param.h"
#include "mcerror.h"
#include "execpt.h"
#include "util.h"
#include "object.h"
#include "socket.h"

#include "globals.h"
#include "text.h"
#include "textlayout.h"

#include "system.h"
#include "srvdebug.h"
#include "srvmain.h"

#include <signal.h>

#define CURL_STATICLIB
#include <curl/curl.h>

////////////////////////////////////////////////////////////////////////////////

Boolean tripleclick = False;

uint32_t g_current_background_colour = 0;

////////////////////////////////////////////////////////////////////////////////

extern void MCS_common_init(void);

extern MCSystemInterface *MCServerCreatePosixSystem(void);
extern MCSystemInterface *MCServerCreateMacSystem(void);
extern MCSystemInterface *MCServerCreateWindowsSystem(void);

extern "C" char *__cxa_demangle(const char *, char *, size_t *, int*);

#ifndef _LINUX_SERVER
static char *strndup(const char *s, uint32_t n)
{
	char *r;
	r = (char *)malloc(n + 1);
	strncpy(r, s, n);
	r[n] = '\0';
	return r;
}
#endif

#ifdef _LINUX_SERVER
#include <execinfo.h>
#include <dlfcn.h>

#define dl_info Dl_info

static void handle_backtrace()
{
	void *t_callstack[16];
	int t_frame_count;
	t_frame_count = backtrace(t_callstack, 16);
	
	for(int i = 1; i < t_frame_count; i++)
	{
		dl_info t_info;
		if (dladdr(t_callstack[i], &t_info) != 0 && t_info . dli_sname != NULL)
		{
			bool t_handled;
			t_handled = false;
			
			if (t_info . dli_sname[0] == '_' && t_info . dli_sname[1] == 'Z')
			{
				int t_status;
				char *t_symbol;
				t_symbol = __cxa_demangle(t_info . dli_sname, NULL, NULL, &t_status);
				if (t_status == 0)
				{
					fprintf(stderr, "  in %s @ %u\n", t_symbol, (char *)t_callstack[i] - (char *)t_info . dli_saddr);
					t_handled = true;
				}
			}
			
			if (!t_handled)
				fprintf(stderr, "  in %s @ %u\n", t_info . dli_sname, (char *)t_callstack[i] - (char *)t_info . dli_saddr);
		}
		else
			fprintf(stderr, "  in <unknown> @ %p\n", t_callstack[i]);
	}
}
#else
void handle_backtrace(void)
{
}
#endif

#ifdef _WINDOWS_SERVER
static void handle_signal(int p_signal)
{
	switch(p_signal)
	{
		case SIGTERM:
			fprintf(stderr, "livecode-server exited by request\n");
			MCquit = True;
			MCexitall = True;
			break;
		case SIGILL:
		case SIGSEGV:
		case SIGABRT:
			fprintf(stderr, "livecode-server exited due to fatal signal %d\n", p_signal);
			handle_backtrace();
			exit(-1);
			break;
		case SIGINT:
			// We received an interrupt so let the debugger (if present) handle it.
			MCServerDebugInterrupt();
			break;
		case SIGFPE:
			errno = EDOM;
			break;
		default:
			break;
	}
}
#else
static void handle_signal(int p_signal)
{
	switch(p_signal)
	{
		case SIGUSR1:
			MCsiguser1++;
			break;
		case SIGUSR2:
			MCsiguser2++;
			break;
		case SIGTERM:
			fprintf(stderr, "livecode-server exited by request\n");
			MCquit = True;
			MCexitall = True;
			break;
		case SIGILL:
		case SIGSEGV:
		case SIGABRT:
			fprintf(stderr, "livecode-server exited due to fatal signal %d\n", p_signal);
			handle_backtrace();
			exit(-1);
			break;
		case SIGINT:
			// We received an interrupt so let the debugger (if present) handle it.
			MCServerDebugInterrupt();
			break;
		case SIGHUP:
		case SIGQUIT:
			fprintf(stderr, "livecode-server exited due to termination signal %d\n", p_signal);
			exit(1);
			break;
		case SIGFPE:
			MCS_seterrno(EDOM);
			break;
		case SIGCHLD:
			break;
		case SIGALRM:
			break;
		case SIGPIPE:
			break;
		default:
			break;
	}
}
#endif

void MCS_init(void)
{
#if defined(_WINDOWS_SERVER)
	MCsystem = MCServerCreateWindowsSystem();
#elif defined(_MAC_SERVER)
	MCsystem = MCServerCreateMacSystem();
#elif defined(_LINUX_SERVER) || defined(_DARWIN_SERVER)
	MCsystem = MCServerCreatePosixSystem();
#else
#error Unknown server platform.
#endif

#ifndef _WINDOWS_SERVER
	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);
	signal(SIGBUS, handle_signal);
	signal(SIGHUP, handle_signal);
	signal(SIGQUIT, handle_signal);
	signal(SIGCHLD, handle_signal);
	signal(SIGALRM, handle_signal);
	signal(SIGPIPE, handle_signal);
#endif
	
	signal(SIGTERM, handle_signal);
	signal(SIGILL, handle_signal);
	signal(SIGSEGV, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGABRT, handle_signal);
	signal(SIGFPE, handle_signal);

	MCS_common_init();
}

////////////////////////////////////////////////////////////////////////////////

typedef const char *(*MCUrlExecuteCallback)(void *state, CURL *curl_handle);

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
	MCString t_string((char *)p_buffer, p_size * p_count);
	MCExecPoint ep;
	ep . setsvalue(t_string);
	MCurlresult -> append(ep, True);
	return p_count;
}

static const char *url_execute(const char *p_url, MCUrlExecuteCallback p_callback, void *p_state)
{
	const char *t_error;
	t_error = NULL;
	
	bool t_is_http, t_is_https;
	t_is_http =  strncmp(p_url, "http", 4) == 0;
	t_is_https = strncmp(p_url, "https", 5) == 0;
	
	curl_slist *t_headers;
	t_headers = NULL;
	if (t_error == NULL && MChttpheaders != NULL && t_is_http)
	{
		if (!url_build_header_list(MChttpheaders, t_headers))
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
		if (curl_easy_setopt(t_url_handle, CURLOPT_URL, p_url) != CURLE_OK)
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
		if (curl_easy_setopt(t_url_handle, CURLOPT_SSL_VERIFYPEER, 1) != CURLE_OK ||
			curl_easy_setopt(t_url_handle, CURLOPT_SSL_VERIFYHOST, 2) != CURLE_OK ||
			(MCsslcertificates != NULL && curl_easy_setopt(t_url_handle, CURLOPT_CAINFO, MCsslcertificates) != CURLE_OK))
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
	
	return t_error;
}

void MCS_geturl(MCObject *p_target, const char *p_url)
{
	const char *t_error;
	t_error = NULL;
	
	if (t_error == NULL)
	{
		if (strncmp(p_url, "https:", 6) != 0 && strncmp(p_url, "http:", 5) != 0 && strncmp(p_url, "ftp:", 4) != 0)
			t_error = "unsupported protocol";
	}
	
	if (t_error == NULL)
		t_error = url_execute(p_url, NULL, NULL);
	
	if (t_error != NULL)
	{
		MCurlresult -> clear();
		MCresult -> sets(t_error);
	}
}

static const char *url_execute_post(void *p_state, CURL *p_curl)
{
	MCString *state;
	state = static_cast<MCString *>(p_state);
	
	const char *t_error;
	t_error = NULL;
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(p_curl, CURLOPT_POST, 1) != CURLE_OK)
			t_error = "couldn't configure post";
	}
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(p_curl, CURLOPT_POSTFIELDS, state -> getstring()) != CURLE_OK)
			t_error = "couldn't set post data";
	}
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(p_curl, CURLOPT_POSTFIELDSIZE, state -> getlength()) != CURLE_OK)
			t_error = "couldn't set post size";
	}
	
	return t_error;
}

void MCS_posttourl(MCObject *p_target, const MCString& p_data, const char *p_url)
{
	const char *t_error;
	t_error = NULL;

	if (t_error == NULL)
	{
		if (strncmp(p_url, "https:", 6) != 0 && strncmp(p_url, "http:", 5) != 0)
			t_error = "unsupported protocol";
	}
	
	if (t_error == NULL)
		t_error = url_execute(p_url, url_execute_post, (void *)&p_data); 

	if (t_error != NULL)
	{
		MCurlresult -> clear();
		MCresult -> sets(t_error);
	}
}

static size_t url_upload_read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	MCString *t_data;
	t_data = static_cast<MCString *>(stream);
	
	size_t t_amount;
	t_amount = size * nmemb;
	if (t_amount > t_data -> getlength())
		t_amount = t_data -> getlength();
	
	memcpy(ptr, t_data -> getstring(), t_amount);
	
	t_data -> set(t_data -> getstring() + t_amount, t_data -> getlength() - t_amount);
	
	return t_amount;
}

static const char *url_execute_upload(void *p_state, CURL *p_curl)
{
	const char *t_error;
	t_error = NULL;
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(p_curl, CURLOPT_UPLOAD, 1) != CURLE_OK)
			t_error = "couldn't configure upload";
	}
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(p_curl, CURLOPT_READFUNCTION, url_upload_read_callback) != CURLE_OK)
			t_error = "couldn't set read function";
	}
	
	if (t_error == NULL)
	{
		if (curl_easy_setopt(p_curl, CURLOPT_READDATA, p_state) != CURLE_OK)
			t_error = "couldn't set read data";
	}
	
	return t_error;
}

void MCS_putintourl(MCObject *p_target, const MCString& p_data, const char *p_url)
{
	const char *t_error;
	t_error = NULL;
	
	if (t_error == NULL)
	{
		if (strncmp(p_url, "https:", 6) != 0 && strncmp(p_url, "http:", 5) != 0 && strncmp(p_url, "ftp:", 4) != 0)
			t_error = "unsupported protocol";
	}
	
	if (t_error == NULL)
	{
		MCString t_data;
		t_data = p_data;
		t_error = url_execute(p_url, url_execute_upload, (void *)&t_data);
	}
	
	if (t_error != NULL)
	{
		MCurlresult -> clear();
		MCresult -> sets(t_error);
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
		t_cmd_string = new char[strlen(t_file) + 5 + 1];
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

void MCS_deleteurl(MCObject *p_target, const char *p_url)
{
	const char *t_error;
	t_error = NULL;

	if (strncmp(p_url, "http://", 7) == 0 || strncmp(p_url, "https://", 8) == 0)
		t_error = url_execute(p_url, url_execute_http_delete, NULL);
	else if (strncmp(p_url, "ftp://", 6) == 0)
		t_error = url_execute(p_url, url_execute_ftp_delete, (void *)p_url);
	else
		t_error = "unsupported protocol";

	if (t_error != NULL)
	{
		MCurlresult -> clear();
		MCresult -> sets(t_error);
	}
}

void MCS_unloadurl(MCObject *p_object, const char *p_url)
{
}

void MCS_loadurl(MCObject *p_object, const char *p_url, const char *p_message)
{
}

////////////////////////////////////////////////////////////////////////////////

extern void MCServerPutOutput(const MCString& data);
extern void MCServerPutUnicodeOutput(const MCString& data);
extern void MCServerPutBinaryOutput(const MCString& data);
extern void MCServerPutHeader(const MCString& data, bool add);
extern void MCServerPutContent(const MCString& data);
extern void MCServerPutUnicodeContent(const MCString& data);
extern void MCServerPutMarkup(const MCString& data);
extern void MCServerPutUnicodeMarkup(const MCString& data);

bool MCS_put(MCExecPoint& ep, MCSPutKind p_kind, const MCString& p_data)
{
	switch(p_kind)
	{
	case kMCSPutOutput:
	case kMCSPutBeforeMessage:
	case kMCSPutIntoMessage:
	case kMCSPutAfterMessage:
		MCServerPutOutput(p_data);
		break;
			
	case kMCSPutBinaryOutput:
		MCServerPutBinaryOutput(p_data);
		break;

	case kMCSPutUnicodeOutput:
		MCServerPutUnicodeOutput(p_data);
		break;
			
	case kMCSPutHeader:
		MCServerPutHeader(p_data, false);
		break;

	case kMCSPutNewHeader:
		MCServerPutHeader(p_data, true);
		break;

	case kMCSPutContent:
		MCServerPutContent(p_data);
		break;
			
	case kMCSPutUnicodeContent:
		MCServerPutUnicodeContent(p_data);
		break;

	case kMCSPutMarkup:
		MCServerPutMarkup(p_data);
		break;
			
	case kMCSPutUnicodeMarkup:
		MCServerPutUnicodeMarkup(p_data);
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

bool MCSystemLaunchUrl(const char *p_url)
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

int MCA_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options)
{
	return 0;
}

int MCA_ask_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options)
{
	return 0;
}

int MCA_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options)
{
	return 0;
}

int MCA_ask_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options)
{
	return 0;
}

int MCA_folder(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_initial, unsigned int p_options)
{
	return 0;
}

int MCA_color(MCExecPoint& ep, const char *p_title, const char *p_initial, Boolean sheet)
{
	return 0;
}

// MERG-2013-08-18: Stubs for colorDialogColors.
void MCA_setcolordialogcolors(MCExecPoint& p_ep)
{
    
}

void MCA_getcolordialogcolors(MCExecPoint& p_ep)
{
	p_ep.clear();
}


////////////////////////////////////////////////////////////////////////////////
