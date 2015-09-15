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
#include "mcio.h"

#include "mcerror.h"
#include "globals.h"
#include "variable.h"
#include "util.h"
#include "socket.h"
//#include "mode.h"
//#include "securemode.h"
//#include "dispatch.h"
//#include "stacklst.h"
//#include "stack.h"
//#include "card.h"
//#include "tooltip.h"
#include "osspec.h"
//#include "redraw.h"
#include "system.h"

#include <netdb.h>
#include <arpa/inet.h>

#ifdef _ANDROID_MOBILE
#include <unistd.h>
#include <sys/socket.h>
#endif

////////////////////////////////////////////////////////////////////////////////

static bool MCS_mbl_hostNameToAddress(MCStringRef p_hostname, MCSystemHostResolveCallback p_callback, void *p_context)
{
	struct hostent *he;
    MCAutoStringRefAsUTF8String t_hostname;
    /* UNCHECKED */ t_hostname . Lock(p_hostname);
	he = gethostbyname(*t_hostname);
	if (he == NULL)
		return false;
    
	struct in_addr **ptr;
	ptr = (struct in_addr **)he -> h_addr_list;
    
	for(uint32_t i = 0; ptr[i] != NULL; i++)
	{
		MCAutoStringRef t_address;
        const char *t_addr_str = inet_ntoa(*ptr[i]);
		if (!MCStringCreateWithCString(t_addr_str, &t_address))
			return false;
		if (!p_callback(p_context, *t_address))
			return false;
	}
    
	return true;
}

bool MCS_mbl_addressToHostName(MCStringRef p_address, MCSystemHostResolveCallback p_callback, void *p_context)
{
	struct in_addr addr;
    MCAutoPointer<char> t_address;
    /* UNCHECKED */ MCStringConvertToCString(p_address, &t_address);
	if (!inet_aton(*t_address, &addr))
		return false;
    
	struct hostent *he;
	he = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
	if (he == NULL)
		return false;
    
	MCAutoStringRef t_name;
	return MCStringCreateWithNativeChars((char_t*)he->h_name, MCCStringLength(he->h_name), &t_name) &&
    p_callback(p_context, *t_name);
}

static bool MCS_mbl_resolve_callback(void *p_context, MCStringRef p_host)
{
	MCListRef t_list = (MCListRef)p_context;
	return MCListAppend(t_list, p_host);
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_hn(MCStringRef& r_string)
{
	MCAutoNativeCharArray t_hostname;
    t_hostname.New(256);
	if (gethostname((char*)t_hostname.Chars(), 256) != 0)
        return false;
   
    t_hostname.Shrink(MCCStringLength((char*)t_hostname.Chars()));
    
	return t_hostname.CreateStringAndRelease(r_string);
}

bool MCS_aton(MCStringRef p_address, MCStringRef& r_name)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;
	if (!MCS_mbl_addressToHostName(p_address, MCS_mbl_resolve_callback, *t_list))
	{
		r_name = MCValueRetain(kMCEmptyString);
		MCresult -> sets("invalid host address");
	}
	else
	{
		MCresult -> clear();
		return MCListCopyAsString(*t_list, r_name);
	}
	return true;
}

bool MCS_ntoa(MCStringRef p_hostname, MCObject *p_target, MCNameRef p_message, MCListRef& r_addr)
{
	if (!MCNameIsEqualTo(p_message, kMCEmptyName))
	{
		MCresult -> sets("not supported");
		r_addr = MCValueRetain(kMCEmptyList);
		return true;
	}

	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;
	if (!MCS_mbl_hostNameToAddress(p_hostname, MCS_mbl_resolve_callback, *t_list))
	{
		r_addr = MCValueRetain(kMCEmptyList);
		MCresult -> sets("invalid host name");
	}
	else
	{
		MCresult -> clear();
		return MCListCopy(*t_list, r_addr);
	}
	return true;
}

bool MCS_dnsresolve(MCStringRef p_hostname, MCStringRef& r_dns)
{
	return false;
}

bool MCS_hostaddress(MCStringRef& r_host_address)
{
	return false;
}
