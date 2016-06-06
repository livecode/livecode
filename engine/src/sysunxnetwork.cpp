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



#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>

// MM-2011-07-14: Added function, builds a return delimited list of the IPv4 addresses
//   of the network adapters this machine has.  For Linux and OS X, this uses getifaddrs
//   to fetch the list and inet_ntop to convert the IP addresses to a human readable form.

bool MCS_getnetworkinterfaces(MCStringRef& r_interfaces)
{
	bool t_success;
	t_success = true;
	
	MCAutoListRef t_list;
	t_success = MCListCreateMutable('\n', &t_list);

	struct ifaddrs *t_if_addrs;
	t_if_addrs = NULL;
	if (t_success && getifaddrs(&t_if_addrs) == 0)
	{
		// We are only concerned with IPvF (AF_INET) addresses, so only need to allocate enough space
		//   (INET_ADDRSTRLEN) to store addresses of that form.  Ignore IPv6 addresses (AF_INET6)
		char t_ip_buff[INET_ADDRSTRLEN];
		struct ifaddrs *t_curr_if_addr;
		for (t_curr_if_addr = t_if_addrs; t_curr_if_addr != NULL; t_curr_if_addr = t_curr_if_addr->ifa_next)
		{
			if (t_success && t_curr_if_addr->ifa_addr != NULL && t_curr_if_addr->ifa_addr->sa_family == AF_INET)
			{
				MCAutoStringRef t_interface;
				MCStringFormat(&t_interface, "%s", inet_ntop(AF_INET, &((struct sockaddr_in *) t_curr_if_addr->ifa_addr)->sin_addr, t_ip_buff, INET_ADDRSTRLEN));
				t_success = MCListAppend(*t_list, *t_interface);
			}
		}
		freeifaddrs(t_if_addrs);
	}

	if (t_success)
		return MCListCopyAsString(*t_list, r_interfaces);

	return false;
}

/*
void MCS_getnetworkinterfaces(MCExecPoint& ep)
{
	ep.clear();

	struct ifaddrs *t_if_addrs;
	t_if_addrs = NULL;
	if (getifaddrs(&t_if_addrs) == 0)
	{
		// We are only concerned with IPvF (AF_INET) addresses, so only need to allocate enough space
		//   (INET_ADDRSTRLEN) to store addresses of that form.  Ignore IPv6 addresses (AF_INET6)
		char t_ip_buff[INET_ADDRSTRLEN];
		struct ifaddrs *t_curr_if_addr;
		for (t_curr_if_addr = t_if_addrs; t_curr_if_addr != NULL; t_curr_if_addr = t_curr_if_addr->ifa_next)
		{
			if (t_curr_if_addr->ifa_addr != NULL && t_curr_if_addr->ifa_addr->sa_family == AF_INET)
			{
				if (ep.getsvalue().getlength() != 0)
					ep.appendstringf("\n");
				ep.appendstringf("%s", inet_ntop(AF_INET, &((struct sockaddr_in *) t_curr_if_addr->ifa_addr)->sin_addr,
												 t_ip_buff, INET_ADDRSTRLEN));
			}
		}
		freeifaddrs(t_if_addrs);
	}
}
*/
