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
#include "osspec.h"

#include "object.h"
#include "stack.h"
#include "card.h"
#include "mcerror.h"

#include "param.h"
#include "handler.h"
#include "util.h"
#include "globals.h"

#include "socket.h"
#include "notify.h"

#include "ports.cpp"

// MW-2010-08-24: Make sure we include 'wspiapi' making it think we are
//   targetting Win2K to ensure that it 'makes' getaddrinfo.
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <process.h>
#else
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/param.h>
#endif

#ifdef TARGET_SUBPLATFORM_ANDROID
#include "mblandroidjava.h"
#endif

////////////////////////////////////////////////////////////////////////////////

uint32_t g_name_resolution_count = 0;

////////////////////////////////////////////////////////////////////////////////

bool addrinfo_lookup(const char *p_name, const char *p_port, int p_socktype, struct addrinfo *&r_addrinfo)
{
	struct addrinfo *t_addrinfo;
	struct addrinfo t_hints;

	MCMemoryClear(&t_hints, sizeof(t_hints));
	t_hints.ai_socktype = p_socktype;
	// specify IPv4 addresses only
	t_hints.ai_family = AF_INET;
	t_hints.ai_flags = 0;

	int t_status;

	t_status = getaddrinfo(p_name, p_port, &t_hints, &t_addrinfo);

	if (t_status == 0)
	{
		r_addrinfo = t_addrinfo;
		return true;
	}
	else
		return false;
}

bool sockaddr_to_string(struct sockaddr *p_addr, int p_addrlen, bool p_lookup_hostname, char *&r_string)
{
	bool t_success = true;
	char *t_buffer = NULL;

	t_success = MCMemoryAllocate(MAXHOSTNAMELEN, t_buffer);

	if (t_success)
	{
		int t_flags = 0;
		if (!p_lookup_hostname)
			t_flags |= NI_NUMERICHOST;
		int t_status = getnameinfo(p_addr, p_addrlen, t_buffer, MAXHOSTNAMELEN, NULL, 0, t_flags);
		t_success = (t_status == 0);
	}

	if (t_success)
		r_string = t_buffer;
	else
		MCMemoryDeallocate(t_buffer);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////


typedef void (*_thread_function)(void *p_context);
typedef void (*_notify_callback)(void *p_context);

struct _thread_info
{
	_thread_function m_function;
	void * m_context;
};

#if defined(_WIN32)
unsigned int __stdcall w32_thread(void *p_context)
{
	_thread_info *t_info = (_thread_info*)p_context;

	t_info->m_function(t_info->m_context);

	MCMemoryDelete(t_info);
	return 0;
}

bool platform_launch_thread(_thread_function p_thread, void *p_context)
{
	bool t_success = true;

	_thread_info *t_info = NULL;
	t_success = MCMemoryNew(t_info);

	if (t_success)
	{
		t_info->m_function = p_thread;
		t_info->m_context = p_context;
		HANDLE t_thread_handle = NULL;
		t_thread_handle = (HANDLE)_beginthreadex(NULL, 0, w32_thread, t_info, 0, NULL);
		t_success = (t_thread_handle != 0);
		CloseHandle(t_thread_handle);
	}

	if (!t_success)
		MCMemoryDelete(t_info);

	return t_success;
}
#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(_LINUX_SERVER) || defined(_MAC_SERVER) || defined(__EMSCRIPTEN__)
void * pthread_thread(void *p_context)
{
	_thread_info *t_info = (_thread_info*)p_context;

	t_info->m_function(t_info->m_context);

	MCMemoryDelete(t_info);
	return NULL;
}

bool platform_launch_thread(_thread_function p_thread, void *p_context)
{
	bool t_success = true;

	_thread_info *t_info = NULL;
	t_success = MCMemoryNew(t_info);

	if (t_success)
	{
		t_info->m_function = p_thread;
		t_info->m_context = p_context;
		pthread_attr_t t_thread_attr;
		pthread_attr_init(&t_thread_attr);
		pthread_attr_setdetachstate(&t_thread_attr, PTHREAD_CREATE_DETACHED);
		pthread_t t_thread_id = 0;
		t_success = pthread_create(&t_thread_id, &t_thread_attr, pthread_thread, t_info) == 0;
		pthread_attr_destroy(&t_thread_attr);
	}

	if (!t_success)
		MCMemoryDelete(t_info);

	return t_success;
}
#endif

struct _thread_notify_info
{
	_thread_function m_function;
	_notify_callback m_callback;
	void * m_context;
};

void notify_thread(void *p_context)
{
#ifdef TARGET_SUBPLATFORM_ANDROID
    // MM-2015-08-04: [[ Bug 15679 ]] Pushing the callback notification will call BreakWait which accesses the JNI.
    //   Make sure we attach this thread to the JVM so that we have a JNI interface pointer.
    MCJavaAttachCurrentThread();
#endif
    
	_thread_notify_info *t_info;
	t_info = (_thread_notify_info*)p_context;

	t_info->m_function(t_info->m_context);

	// IM-2011-08-23: call MCNotifyPush() with safe == false, as notification can be dispatched
	// during unsafe wait.  fixes broken hostnametoaddress call
	MCNotifyPush(t_info->m_callback, t_info->m_context, false, false);
	MCMemoryDelete(t_info);
    
#ifdef TARGET_SUBPLATFORM_ANDROID
    MCJavaDetachCurrentThread();
#endif
}

bool launch_thread_with_notify(_thread_function p_thread, _notify_callback p_callback, void *p_context)
{
	bool t_success = true;
	_thread_notify_info *t_info = NULL;

	t_success = MCMemoryNew(t_info);

	if (t_success)
	{
		t_info->m_function = p_thread;
		t_info->m_callback = p_callback;
		t_info->m_context = p_context;

		t_success = platform_launch_thread(notify_thread, t_info);
	}

	if (!t_success)
		MCMemoryDelete(t_info);

	return t_success;
}

struct _addrinfo_lookup_info
{
	char *m_name;
	char *m_port;
	int m_socktype;

	bool m_success;
	addrinfo *m_addrinfo;

	bool m_blocking;
	bool m_finished;
	MCHostNameResolveCallback m_callback;
	void *m_context;
};

void free_addrinfo_lookup_info(_addrinfo_lookup_info *t_info)
{
	if (t_info)
	{
		if (t_info->m_name)
			MCCStringFree(t_info->m_name);
		if (t_info->m_port)
			MCCStringFree(t_info->m_port);
		MCMemoryDelete(t_info);
	}
}

void hostname_resolve_thread(void *p_context)
{
	_addrinfo_lookup_info *t_info = (_addrinfo_lookup_info*)p_context;

	struct addrinfo *t_addrinfo;
	bool t_success = addrinfo_lookup(t_info->m_name, t_info->m_port, t_info->m_socktype, t_addrinfo);

	if (t_success)
		t_info->m_addrinfo = t_addrinfo;

	t_info->m_success = t_success;
}

void hostname_resolve_notify_callback(void *p_context)
{
	_addrinfo_lookup_info *t_info = (_addrinfo_lookup_info*)p_context;

	g_name_resolution_count -= 1;
	
	if (t_info->m_success)
	{
		addrinfo *t_addrinfo = t_info->m_addrinfo;
		if (t_info->m_callback != NULL)
		{
			bool t_continue = true;
			addrinfo *t_node = t_addrinfo;

			while (t_node != NULL && t_continue)
			{
				t_continue = t_info->m_callback(t_info->m_context, true, t_node->ai_next == NULL, t_node->ai_addr, t_node->ai_addrlen);
				t_node = t_node->ai_next;
			}
		}
		freeaddrinfo(t_addrinfo);
	}
	else
		t_info->m_callback(t_info->m_context, false, true, NULL, 0);

	if (t_info->m_blocking)
	{
		t_info->m_finished = true;
	}
	else
		free_addrinfo_lookup_info(t_info);
}

bool MCSocketHostNameResolve(const char *p_name, const char *p_port, int p_socktype, bool p_blocking,
							MCHostNameResolveCallback p_callback, void *p_context)
{
	_addrinfo_lookup_info *t_info = NULL;
	bool t_success = true;

	t_success = MCMemoryNew(t_info);

	if (t_success)
	{
		t_info->m_socktype = p_socktype;

		t_info->m_finished = false;
		t_info->m_blocking = p_blocking;

		t_info->m_callback = p_callback;
		t_info->m_context = p_context;

		t_success = (MCCStringClone(p_name, t_info->m_name) &&
			MCCStringClone(p_port, t_info->m_port));
	}
    // SN-2014-12-16: [[ Bug 14181 ]] We can't notify on servers as there is no RunLoop.
    //  We do not create a thread to resolve the hostname.
	if (t_success)
#ifdef _SERVER
    {
        hostname_resolve_thread((void*)t_info);
        hostname_resolve_notify_callback((void*)t_info);
    }
#else
		t_success = launch_thread_with_notify(hostname_resolve_thread, hostname_resolve_notify_callback, t_info);
#endif
	if (t_success)
	{
		g_name_resolution_count += 1;
		if (p_blocking)
		{
			while (!t_info->m_finished)
			{
				// MW-2010-09-09: This call shouldn't allow dispatch.
				MCscreen->wait(MCsockettimeout, false, true);
			}
			t_success = t_info->m_success;
			free_addrinfo_lookup_info(t_info);
		}
	}
	else
		free_addrinfo_lookup_info(t_info);
	
	return t_success;
}

bool hostname_resolve_first_sockaddr_callback(void *p_context, bool p_resolved, bool p_final, struct sockaddr *p_addr, int p_addrlen)
{
	if (p_resolved)
	{
		struct sockaddr_in *t_sockaddr = (struct sockaddr_in *)p_context;
		memcpy(&t_sockaddr->sin_addr, &((struct sockaddr_in *)p_addr)->sin_addr, sizeof(t_sockaddr->sin_addr));
	}
	return false;
}


bool MCS_name_to_sockaddr(MCStringRef p_name_in, struct sockaddr_in *r_addr, MCHostNameResolveCallback p_callback, void *p_context)
{
	if (!MCS_init_sockets())
		return false;

    MCAutoStringRef t_name;
    uindex_t t_or;

    // support multiple opens to same host:port
    if (MCStringFirstIndexOfChar(p_name_in, '|', 0, kMCCompareExact, t_or))
    {
        if (!MCStringCopySubstring(p_name_in, MCRangeMake(0, t_or), &t_name))
            return false;
    }
    else
        t_name = p_name_in;

    // get port & id if set
    MCAutoStringRef t_host;
    MCAutoNumberRef t_port;
    if (!MCS_name_to_host_and_port(*t_name, &t_host, &t_port))
        return false;
    
    // default port to http
    if (!t_port.IsSet())
    {
        if (!MCNumberCreateWithUnsignedInteger(80, &t_port))
            return false;
    }

    return MCS_host_and_port_to_sockaddr(*t_host, *t_port, r_addr, p_callback, p_context);
}

bool MCS_name_to_sockaddr(MCStringRef p_name, struct sockaddr_in &r_addr)
{
	return MCS_name_to_sockaddr(p_name, &r_addr, NULL, NULL);
}

bool MCS_name_to_host_and_port(MCStringRef p_name, MCStringRef &r_host, MCNumberRef &r_port)
{
    MCAutoStringRef t_host;
    MCAutoNumberRef t_port;

    uindex_t t_colon;
    if (MCStringFirstIndexOfChar(p_name, ':', 0, kMCCompareExact, t_colon))
    {
        MCAutoStringRef t_port_string;
        if (!MCStringDivideAtIndex(p_name, t_colon, &t_host, &t_port_string))
        {
            MCresult->sets("not a valid host:port string");
            return false;
        }
        
        if (!MCNumberParse(*t_port_string, &t_port))
        {
            uint2 i;
            for (i = 0 ; i < ELEMENTS(port_table) ; i++)
            {
                if (MCStringIsEqualToCString(*t_port_string, port_table[i].name, kMCCompareExact))
                {
                    if (!MCNumberCreateWithUnsignedInteger(port_table[i].port, &t_port))
                    {
                        MCresult->sets("can't set port");
                        return false;
                    }
                    break;
                }
            }
            
            if (i == ELEMENTS(port_table))
            {
                MCresult->sets("not a valid port");
                return false;
            }
        }

        if (!MCNumberIsInteger(*t_port))
        {
            MCresult->sets("not a valid port");
            return false;
        }

        integer_t t_port_int = MCNumberFetchAsInteger(*t_port);
        if (t_port_int < 0 || t_port_int > UINT16_MAX)
        {
            MCresult->sets("not a valid port");
            return false;
        }
    }
    else
        t_host = p_name;

    if (!MCStringIsEmpty(*t_host))
        MCValueAssign(r_host, *t_host);
    else
        r_host = NULL;
    if (t_port.IsSet())
        MCValueAssign(r_port, *t_port);
    else
        r_port = NULL;
    return true;
}

bool MCS_host_and_port_to_sockaddr(MCStringRef p_host, MCNumberRef p_port, struct sockaddr_in *r_addr, MCHostNameResolveCallback p_callback, void *p_context)
{
    if (MCValueIsEmpty(p_host) || MCValueIsEmpty(p_port))
        return false;
    
    MCAutoPointer<char> t_host;
    if (!MCStringConvertToCString(p_host, &t_host))
        return false;
    
    uinteger_t t_port = MCNumberFetchAsUnsignedInteger(p_port);
    MCMemoryClear(r_addr, sizeof(struct sockaddr_in));
    r_addr->sin_family = AF_INET;
    r_addr->sin_port = MCSwapInt16HostToNetwork((uint16_t) t_port);

    // if callback provided then start name resolve thread with that callback
    // else start name resolve thread with our own callback and wait for thread to finish
    if (p_callback == NULL)
        return MCSocketHostNameResolve(*t_host, NULL, SOCK_STREAM, true, hostname_resolve_first_sockaddr_callback, r_addr);
    else
        return MCSocketHostNameResolve(*t_host, NULL, SOCK_STREAM, false, p_callback, p_context);
}

bool MCS_host_and_port_to_sockaddr(MCStringRef p_host, MCNumberRef p_port, struct sockaddr_in &r_addr)
{
    return MCS_host_and_port_to_sockaddr(p_host, p_port, &r_addr, NULL, NULL);
}

////////////////////////////////////////////////////////////////////////////////

struct _sockaddr_to_string_info
{
	struct sockaddr *m_addr;
	int m_addrlen;

	bool m_success;
	char *m_string;

	bool m_blocking;
	bool m_finished;
	MCSockAddrToStringCallback m_callback;
	void *m_context;
};

void free_sockaddr_to_string_info(_sockaddr_to_string_info *t_info)
{
	if (t_info)
	{
		if (t_info->m_addr)
			MCMemoryDeallocate(t_info->m_addr);
		if (t_info->m_string)
			MCCStringFree(t_info->m_string);
		MCMemoryDelete(t_info);
	}
}

void sockaddr_to_string_thread(void *p_context)
{
	_sockaddr_to_string_info *t_info = (_sockaddr_to_string_info*)p_context;

	t_info->m_success = sockaddr_to_string(t_info->m_addr, t_info->m_addrlen, true, t_info->m_string);
}

void sockaddr_to_string_notify_callback(void *p_context)
{
	_sockaddr_to_string_info *t_info = (_sockaddr_to_string_info*)p_context;

	t_info->m_callback(t_info->m_context, t_info->m_success, t_info->m_string);

	if (t_info->m_blocking)
	{
		t_info->m_finished = true;
	}
	else
		free_sockaddr_to_string_info(t_info);
}

bool MCSocketAddrToString(struct sockaddr *p_sockaddr, int p_addrlen, bool p_lookup_hostname, bool p_blocking,
						MCSockAddrToStringCallback p_callback, void *p_context)
{
	_sockaddr_to_string_info *t_info = NULL;
	bool t_success = true;

	t_success = MCMemoryNew(t_info);

	if (t_success)
	{
		t_success = MCMemoryAllocateCopy(p_sockaddr, p_addrlen, t_info->m_addr);
		t_info->m_addrlen = p_addrlen;

		t_info->m_finished = false;
		t_info->m_blocking = p_blocking;

		t_info->m_callback = p_callback;
		t_info->m_context = p_context;
	}
	if (t_success)
	{
		t_success = launch_thread_with_notify(sockaddr_to_string_thread, sockaddr_to_string_notify_callback, t_info);
	}

	if (t_success)
	{
		g_name_resolution_count += 1;
		if (p_blocking)
		{
			while (!t_info->m_finished)
			{
				MCscreen->wait(MCsockettimeout, false, true);
			}
			t_success = t_info->m_success;
			free_sockaddr_to_string_info(t_info);
		}
	}
	else
		free_sockaddr_to_string_info(t_info);

	return t_success;
}

bool MCS_sockaddr_to_string(struct sockaddr *p_addr, int p_addrlen, bool p_lookup_hostname, bool p_blocking,
							MCSockAddrToStringCallback p_callback, void *p_context)
{
	bool t_success = true;

	if (p_callback == NULL)
		return false;

	if (!p_lookup_hostname)
	{
		char *t_string;
        t_string = NULL;
		t_success = sockaddr_to_string(p_addr, p_addrlen, false, t_string);
		p_callback(p_context, t_success, t_string);
		MCCStringFree(t_string);
	}
	else
	{
		t_success = MCSocketAddrToString(p_addr, p_addrlen, p_lookup_hostname, p_blocking, p_callback, p_context);
	}

	return t_success;
}

struct _sockaddr_to_string_context
{
	bool success;
	char *name;
};

void sockaddr_to_string_callback(void *p_context, bool p_resolved, const char *p_name)
{
	struct _sockaddr_to_string_context* t_info = (_sockaddr_to_string_context*) p_context;

	t_info->success = p_resolved && MCCStringClone(p_name, t_info->name);
}

bool MCS_sockaddr_to_string(struct sockaddr *p_addr, int p_addrlen, bool p_lookup_hostname, char *&r_string)
{
	struct _sockaddr_to_string_context t_info;
	t_info.success = true;
	t_info.name = nil;
	if (MCS_sockaddr_to_string(p_addr, p_addrlen, p_lookup_hostname, true, sockaddr_to_string_callback, &t_info) &&
		t_info.success)
	{
		r_string = t_info.name;
		return true;
	}

	return false;
}

struct _sockaddr_to_stringref_context
{
	bool success;
	MCAutoStringRef name;
};

void sockaddr_to_stringref_callback(void *p_context, bool p_resolved, const char *p_name)
{
	struct _sockaddr_to_stringref_context* t_info = (_sockaddr_to_stringref_context*) p_context;

	t_info->success = p_resolved && MCStringCreateWithNativeChars((const char_t*)p_name, MCCStringLength(p_name), &t_info->name);
}

bool MCS_sockaddr_to_string(struct sockaddr *p_addr, int p_addrlen, bool p_lookup_hostname, MCStringRef& r_string)
{
	struct _sockaddr_to_stringref_context t_info;
	t_info.success = true;

	return MCS_sockaddr_to_string(p_addr, p_addrlen, p_lookup_hostname, true, sockaddr_to_stringref_callback, &t_info) &&
		t_info.success && MCStringCopy(*t_info.name, r_string);
}
