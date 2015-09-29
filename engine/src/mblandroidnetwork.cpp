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

#include "core.h"
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

bool MCAndroidSystem::HostNameToAddress(const char *p_hostname, MCSystemHostResolveCallback p_callback, void *p_context)
{
	struct hostent *he;
	he = gethostbyname(p_hostname);
	if (he == NULL)
		return false;
	
	struct in_addr **ptr;
	ptr = (struct in_addr **)he -> h_addr_list;
	
	for(uint32_t i = 0; ptr[i] != NULL; i++)
		if (!p_callback(p_context, inet_ntoa(*ptr[i])))
			return false;
	
	return true;
}

bool MCAndroidSystem::AddressToHostName(const char *p_address, MCSystemHostResolveCallback p_callback, void *p_context)
{
	struct in_addr addr;
	if (!inet_aton(p_address, &addr))
		return false;
		
	struct hostent *he;
	he = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
	if (he == NULL)
		return false;
	
	return p_callback(p_context, he -> h_name);
}

////////////////////////////////////////////////////////////////////////////////
