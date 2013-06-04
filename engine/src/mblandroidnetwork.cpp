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

#include "system.h"
#include "mblandroid.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>

////////////////////////////////////////////////////////////////////////////////

char *MCAndroidSystem::GetHostName(void)
{
	char t_hostname[256];
	gethostname(t_hostname, 256);
	return strdup(t_hostname);
}

bool MCAndroidSystem::HostNameToAddress(MCStringRef p_hostname, MCSystemHostResolveCallback p_callback, void *p_context)
{
	struct hostent *he;
	he = gethostbyname(MCStringGetCString(p_hostname));
	if (he == NULL)
		return false;
	
	struct in_addr **ptr;
	ptr = (struct in_addr **)he -> h_addr_list;
	
	for(uint32_t i = 0; ptr[i] != NULL; i++)
	{
		MCAutoStringRef t_address;
		char *t_addr_str = inet_ntoa(*ptr[i]);
		if (!MCStringCreateWithNativeChars((char_t*)t_addr_str, MCCStringLength(t_addr_str), &t_address))
			return false;
		if (!p_callback(p_context, *t_address))
			return false;
	}
	
	return true;
}

bool MCAndroidSystem::AddressToHostName(MCStringRef p_address, MCSystemHostResolveCallback p_callback, void *p_context)
{
	struct in_addr addr;
	if (!inet_aton(MCStringGetCString(p_address), &addr))
		return false;
		
	struct hostent *he;
	he = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
	if (he == NULL)
		return false;
	
	MCAutoStringRef t_name;
	return MCStringCreateWithNativeChars((char_t*)he->h_name, MCCStringLength(he->h_name), &t_name) &&
		p_callback(p_context, *t_name);
}

////////////////////////////////////////////////////////////////////////////////
