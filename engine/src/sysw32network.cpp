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



#include <winsock2.h>
#include <iphlpapi.h>

////////////////////////////////////////////////////////////////////////////////

bool MCS_getnetworkinterfaces(MCStringRef& r_interfaces)
{
	bool t_success;
	t_success = true;
	
	MCAutoListRef t_list;
	t_success = MCListCreateMutable('\n', &t_list);
	
	PIP_ADAPTER_INFO t_adapter_info;
	t_adapter_info = NULL;
	if (t_success)
		t_success = MCMemoryAllocate(sizeof(IP_ADAPTER_INFO), t_adapter_info);
	
	// An initial call to GetAdaptersInfo is needed to determine the size of the buffer we
	//	need to allocate.
	ULONG t_output_buffer_len;
	if (t_success)
	{
		t_output_buffer_len = sizeof(IP_ADAPTER_INFO);
		if (GetAdaptersInfo(t_adapter_info, &t_output_buffer_len) == ERROR_BUFFER_OVERFLOW) 
		{
			MCMemoryDelete(t_adapter_info);
			t_success = MCMemoryAllocate(t_output_buffer_len, t_adapter_info); 
		}
	}
	
    if (t_success)
	{
		if (GetAdaptersInfo(t_adapter_info, &t_output_buffer_len) == NO_ERROR)
		{
			PIP_ADAPTER_INFO t_adapter;
			for (t_adapter = t_adapter_info; t_adapter != NULL; t_adapter = t_adapter->Next)
			{
				// Each adapter may have several IP addresses, so be sure to list them all.
				PIP_ADDR_STRING t_ip_addr;
				for(t_ip_addr = &t_adapter->IpAddressList; t_ip_addr != NULL; t_ip_addr = t_ip_addr->Next)
				{
					if (t_success)
					{
						MCAutoStringRef t_interface;
						MCStringFormat(&t_interface, "%s", t_ip_addr->IpAddress.String);
						t_success = MCListAppend(*t_list, *t_interface);
					}
				}
					
			}
		}
	}
	
    if (t_adapter_info != NULL)
        MCMemoryDelete(t_adapter_info);

	if (t_success)
		return MCListCopyAsString(*t_list, r_interfaces);

	return false;
}
/*
void MCS_getnetworkinterfaces(MCExecPoint& ep)
{
	ep.clear();

	bool t_success;
	t_success = true;
	
	PIP_ADAPTER_INFO t_adapter_info;
	t_adapter_info = NULL;
	if (t_success)
		t_success = MCMemoryAllocate(sizeof(IP_ADAPTER_INFO), t_adapter_info);
	
	// An initial call to GetAdaptersInfo is needed to determine the size of the buffer we
	//	need to allocate.
	ULONG t_output_buffer_len;
	if (t_success)
	{
		t_output_buffer_len = sizeof(IP_ADAPTER_INFO);
		if (GetAdaptersInfo(t_adapter_info, &t_output_buffer_len) == ERROR_BUFFER_OVERFLOW) 
		{
			MCMemoryDelete(t_adapter_info);
			t_success = MCMemoryAllocate(t_output_buffer_len, t_adapter_info); 
		}
	}
	
    if (t_success)
	{
		if (GetAdaptersInfo(t_adapter_info, &t_output_buffer_len) == NO_ERROR)
		{
			PIP_ADAPTER_INFO t_adapter;
			for (t_adapter = t_adapter_info; t_adapter != NULL; t_adapter = t_adapter->Next)
			{
				// Each adapter may have several IP addresses, so be sure to list them all.
				PIP_ADDR_STRING t_ip_addr;
				for(t_ip_addr = &t_adapter->IpAddressList; t_ip_addr != NULL; t_ip_addr = t_ip_addr->Next)
				{
					if (ep.getsvalue().getlength() != 0)
						ep.appendstringf("\n");
					ep.appendstringf("%s", t_ip_addr->IpAddress.String);
				}
			}
		}
	}
	
    if (t_adapter_info != NULL)
        MCMemoryDelete(t_adapter_info);
}
*/
////////////////////////////////////////////////////////////////////////////////
