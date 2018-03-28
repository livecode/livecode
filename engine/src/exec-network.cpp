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

#include "globals.h"
#include "socket.h"
#include "securemode.h"
#include "mode.h"
#include "object.h"
#include "osspec.h"
#include "util.h"
#include "scriptenvironment.h"
#include "uidc.h"

#include "param.h"

#include "exec.h"
#include "regex.h"

////////////////////////////////////////////////////////////////////////////////

static MCScriptEnvironment *s_pac_engine = nil;

////////////////////////////////////////////////////////////////////////////////

void MCNetworkEvalCachedUrls(MCExecContext& ctxt, MCStringRef& r_string)
{
	/* UNCHECKED */ ctxt.GetObject()->message(MCM_get_cached_urls, nil, False, True);
	if (ctxt.ForceToString(MCresult->getvalueref(), r_string))
		return;

	ctxt.Throw();
}

void MCNetworkEvalUrlStatus(MCExecContext& ctxt, MCStringRef p_url, MCStringRef& r_status)
{
	MCParameter t_param;
	t_param.setvalueref_argument(p_url);
	/* UNCHECKED */ ctxt.GetObject()->message(MCM_get_url_status, &t_param, False, True);
	if (ctxt.ForceToString(MCresult->getvalueref(), r_status))
		return;

	ctxt.Throw();
}

//////////

void MCNetworkEvalDNSServers(MCExecContext& ctxt, MCStringRef& r_servers)
{
	MCAutoListRef t_list;
	if (MCSecureModeCheckNetwork() && MCS_getDNSservers(&t_list) &&
		MCListCopyAsString(*t_list, r_servers))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCNetworkSplitSocketId(MCStringRef p_sock, MCStringRef& r_host, MCStringRef& r_port, MCStringRef& r_id)
{
	uindex_t t_len;
	uindex_t t_port_index, t_id_index;
	uindex_t t_port_len, t_id_len;
	t_len = MCStringGetLength(p_sock);
	if (MCStringFirstIndexOfChar(p_sock, '|', 0, kMCStringOptionCompareExact, t_id_index))
	{
		t_id_index += 1;
		t_id_len = t_len - t_id_index;
		t_len -= t_id_len + 1;
	}
	else
		t_id_index = t_id_len = 0;

	if (MCStringFirstIndexOfChar(p_sock, ':', 0, kMCStringOptionCompareExact, t_port_index) &&
		t_port_index < t_len)
	{
		t_port_index += 1;
		t_port_len = t_len - t_port_index;
		t_len -= t_port_len + 1;
	}
	else
		t_port_index = t_port_len = 0;

	return MCStringCopySubstring(p_sock, MCRangeMake(0, t_len), r_host) &&
		MCStringCopySubstring(p_sock, MCRangeMake(t_port_index, t_port_len), r_port) &&
		MCStringCopySubstring(p_sock, MCRangeMake(t_id_index, t_id_len), r_id);
}

bool MCNetworkGetHostFromSocketId(MCStringRef p_socket, MCStringRef& r_host)
{
	MCAutoStringRef t_port, t_id;
	return MCNetworkSplitSocketId(p_socket, r_host, &t_port, &t_id);
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkEvalHostAddress(MCExecContext& ctxt, MCNameRef p_socket, MCStringRef& r_address)
{
	uindex_t t_socket_index;
	if (IO_findsocket(p_socket, t_socket_index))
	{
		if (MCS_ha(MCsockets[t_socket_index], r_address))
			return;
	}
	else
	{
		/* RESULT */
		if (MCStringCreateWithCString("not an open socket", r_address))
			return;
	}

	ctxt.Throw();
}

void MCNetworkEvalPeerAddress(MCExecContext& ctxt, MCNameRef p_socket, MCStringRef& r_address)
{
#ifndef _MOBILE
	uindex_t t_socket_index;
	if (IO_findsocket(p_socket, t_socket_index))
	{
		if (MCS_pa(MCsockets[t_socket_index], r_address))
			return;
	}
	else
	{
		/* RESULT */
		if (MCStringCreateWithCString("not an open socket", r_address))
			return;
	}

	// Backwards compatibility: never throws an error
#endif
}

//////////

void MCNetworkEvalHostAddressToName(MCExecContext& ctxt, MCStringRef p_address, MCStringRef &r_name)
{
	if (!MCS_aton(p_address, r_name))
	{
		ctxt.Throw();
		return;
	}
	/* TODO - I.M. this is a bit odd, checking if we have permission to resolve a hostname AFTER we've
	   resolved it.  Would probably make more sense to get the (cached) addresses of the accessible
	   domains and check against those first before calling MCS_aton */

	// We only allow an address to name lookup if the resulting is the secure domain
	// unless we have network access.
	if (!MCSecureModeCanAccessNetwork() && !MCModeCanAccessDomain(r_name))
	{
		MCValueRelease(r_name);
		r_name = nil;
		ctxt.LegacyThrow(EE_NETWORK_NOPERM);
	}
}

//////////

void MCNetworkEvalHostNameToAddress(MCExecContext& ctxt, MCStringRef p_hostname, MCNameRef p_message, MCStringRef& r_string)
{
	// We only allow an name to address lookup to occur for the secure domain.
	if (!MCSecureModeCanAccessNetwork() && !MCModeCanAccessDomain(p_hostname))
	{
		ctxt.LegacyThrow(EE_NETWORK_NOPERM);
		return;
	}
    
    // SN-2014-12-16: [[ Bug 14181 ]] We don't accept callback messages on server
#ifdef _SERVER
    if (!MCNameIsEmpty(p_message))
    {
        ctxt.LegacyThrow(EE_HOSTNAME_BADMESSAGE);
        return;
    }
#endif

	MCAutoListRef t_list;
    if (MCS_ntoa(p_hostname, ctxt.GetObject(), p_message, &t_list))
    {
        if (!MCListCopyAsString(*t_list, r_string))
            ctxt . Throw();
    }
    else
        r_string = MCValueRetain(kMCEmptyString);
}

//////////

void MCNetworkEvalHostName(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCS_hn(r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

static bool MCNetworkOpenSocketsList(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	IO_cleansockets(MCS_time());
	for (uinteger_t i = 0; i < MCnsockets; i++)
		if (!MCsockets[i]->closing)
			if (!MCListAppend(*t_list, MCsockets[i]->name))
				return false;

	return MCListCopy(*t_list, r_list);
}

void MCNetworkEvalOpenSockets(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCNetworkOpenSocketsList(&t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

static char *PACdnsResolve(const char* const* p_arguments, unsigned int p_argument_count)
{
	if (p_argument_count != 1)
		return NULL;

	MCAutoStringRef t_address_string;
	MCAutoStringRef t_arguments_string;
	/* UNCHECKED */ MCStringCreateWithCString(p_arguments[0], &t_arguments_string);
	MCS_dnsresolve(*t_arguments_string, &t_address_string);

	char *t_address = nil;
	if (*t_address_string != nil)
		/* UNCHECKED */ MCStringConvertToCString(*t_address_string, t_address);

	return t_address;
}

static char *PACmyIpAddress(const char* const* p_arguments, unsigned int p_argument_count)
{
	if (p_argument_count != 0)
		return NULL;

	MCAutoStringRef t_address_string;
	MCS_hostaddress(&t_address_string);
	char *t_address = nil;
	if (*t_address_string != nil)
		/* UNCHECKED */ MCStringConvertToCString(*t_address_string, t_address);

	return t_address;
}
void MCNetworkEvalHTTPProxyForURL(MCExecContext& ctxt, MCStringRef p_url, MCStringRef p_host, MCStringRef& r_proxy)
{
	if (s_pac_engine == nil)
    {
        r_proxy = MCValueRetain(kMCEmptyString);
        return;
    }

	const char *t_arguments[2];
	char *t_url, *t_host;
    /* UNCHECKED */ MCStringConvertToCString(p_url, t_url);
    /* UNCHECKED */ MCStringConvertToCString(p_host, t_host);
    
    t_arguments[0] = t_url;
    t_arguments[1] = t_host;

    /* UNCHECKED */ MCAutoPointer<char[]> t_proxies =
        s_pac_engine -> Call("__FindProxyForURL", t_arguments, 2);

	if (*t_proxies != nil)
		/* UNCHECKED */ MCStringCreateWithCString(*t_proxies, r_proxy);
	else
		r_proxy = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCNetworkEvalHTTPProxyForURLWithPAC(MCExecContext& ctxt, MCStringRef p_url, MCStringRef p_host, MCStringRef p_pac, MCStringRef& r_proxy)
{
	if (s_pac_engine != NULL)
	{
		s_pac_engine -> Release();
		s_pac_engine = NULL;
	}

	if (MCStringGetLength(p_pac) > 0)
	{
		s_pac_engine = MCscreen -> createscriptenvironment(MCSTR("javascript"));
		if (s_pac_engine != NULL)
		{
			bool t_success;
			t_success = s_pac_engine -> Define("__dnsResolve", PACdnsResolve);

			if (t_success)
				t_success = s_pac_engine -> Define("__myIpAddress", PACmyIpAddress);

			if (t_success)
			{
				MCAutoStringRef t_result;
				s_pac_engine -> Run(p_pac, &t_result);
				t_success = *t_result != NULL;
			}
			
			if (!t_success)
			{
				s_pac_engine -> Release();
				s_pac_engine = NULL;
			}
		}
	}

	MCNetworkEvalHTTPProxyForURL(ctxt, p_url, p_host, r_proxy);
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecLoadUrl(MCExecContext& ctxt, MCStringRef p_url, MCNameRef p_message)
// SJT-2014-09-11: [[ URLMessages ]] Send "loadURL" messages on all platforms.
{
  // Send "loadURL" message.
	MCParameter p1;
	p1 . setvalueref_argument(p_url);
	MCParameter p2;
	p2 . setvalueref_argument(p_message);
	p1.setnext(&p2);
	// MW-2006-03-03: I've changed this from False, True to True, True to ensure 'target' is returned correctly for libURL.
  Exec_stat t_stat = ctxt . GetObject() -> message(MCM_load_url, &p1, True, True);
	
	switch (t_stat)
	{
  case ES_NOT_HANDLED:
  case ES_PASS:
    // Either there was no message handler, or the handler passed the message,
    // so process the URL in the engine.
    MCS_loadurl(ctxt . GetObject(), p_url, p_message);
    break;

  case ES_ERROR:
    ctxt . Throw();
    break;
    
  default:
    break;
  }
}

void MCNetworkExecUnloadUrl(MCExecContext& ctxt, MCStringRef p_url)
// SJT-2014-09-11: [[ URLMessages ]] Send "unloadURL" messages on all platforms.
{
  // Send "unloadURL" message.
	MCParameter p1;
	p1 . setvalueref_argument(p_url);
  Exec_stat t_stat = ctxt . GetObject() -> message(MCM_unload_url, &p1, False, True);
	
	switch (t_stat)
	{
  case ES_NOT_HANDLED:
  case ES_PASS:
    // Either there was no message handler, or the handler passed the message,
    // so process the URL in the engine.
    MCS_unloadurl(ctxt . GetObject(), p_url);
    break;

  case ES_ERROR:
    ctxt . Throw();
    break;
    
  default:
    break;
  }
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecPostToUrl(MCExecContext& ctxt, MCValueRef p_data, MCStringRef p_url)
// SJT-2014-09-11: [[ URLMessages ]] Send "postURL" messages on all platforms.
{
	if (MCU_couldbeurl(p_url))
	{
		MCAutoDataRef t_data;

		// Send "postURL" message.
		MCParameter p1;
		p1 . setvalueref_argument(p_data);
		MCParameter p2;
		p2 . setvalueref_argument(p_url);
		p1.setnext(&p2);
		Exec_stat t_stat = ctxt . GetObject() -> message(MCM_post_url, &p1, False, True);

		switch (t_stat)
		{
		case ES_NOT_HANDLED:
		case ES_PASS:
			// Either there was no message handler, or the handler passed the message,
			// so process the URL in the engine.
			/* UNCHECKED */ ctxt.ConvertToData(p_data, &t_data);
			MCS_posttourl(ctxt . GetObject(), *t_data, p_url);
			// don't break!

		default:
			ctxt . SetItToValue(MCurlresult -> getvalueref());
			break;

		case ES_ERROR:
			ctxt . Throw();
			break;
		}
	}
	else
	{
		MCAutoStringRef t_err;
		MCStringFormat(&t_err, "invalid URL: %@", p_url);
		MCresult -> setvalueref(*t_err);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecDeleteUrl(MCExecContext& ctxt, MCStringRef p_target)
// SJT-2014-09-11: [[ URLMessages ]] Send "deleteURL" messages on all platforms.
{
	MCAutoStringRef t_filename;
	if ((MCStringGetLength(p_target) > 5 &&
		MCStringBeginsWithCString(p_target, (const char_t*)"file:", kMCCompareCaseless)) ||
		(MCStringGetLength(p_target) > 8 &&
		MCStringBeginsWithCString(p_target, (const char_t*)"binfile:", kMCCompareCaseless)))
	{
		// Check the disk access here since MCS_unlink is used more
		// generally.
		if (!ctxt . EnsureDiskAccessIsAllowed())
			return;
		if (MCStringBeginsWithCString(p_target, (const char_t*)"file:", kMCCompareCaseless))
			MCStringCopySubstring(p_target, MCRangeMakeMinMax(5, MCStringGetLength(p_target)), &t_filename);
		else
			MCStringCopySubstring(p_target, MCRangeMakeMinMax(8, MCStringGetLength(p_target)), &t_filename);
		if (!MCS_unlink(*t_filename))
			ctxt . SetTheResultToStaticCString("can't delete that file");
		else
			ctxt . SetTheResultToEmpty();
	}
	else if (MCStringGetLength(p_target) > 8 &&
		MCStringBeginsWithCString(p_target, (const char_t*)"resfile:", kMCCompareCaseless))
	{
		MCStringCopySubstring(p_target, MCRangeMakeMinMax(8, MCStringGetLength(p_target)), &t_filename);
		MCS_saveresfile(*t_filename, kMCEmptyData);
	}
	else if (MCU_couldbeurl(p_target))
	{
		// Send "deleteURL" message
		Boolean oldlock = MClockmessages;
		MClockmessages = False;
		MCParameter p1;
		p1 . setvalueref_argument(p_target);
		Exec_stat t_stat = ctxt . GetObject() -> message(MCM_delete_url, &p1, False, True);
		MClockmessages = oldlock;

		switch (t_stat)
		{
		case ES_NOT_HANDLED:
		case ES_PASS:
			// Either there was no message handler, or the handler passed the message,
			// so process the URL in the engine.
			MCS_deleteurl(ctxt.GetObject(), p_target);
			break;

		case ES_ERROR:
			ctxt . Throw();
			break;

		default:
			break;
		}
	}
	else
	{
		MCAutoStringRef t_err;
		MCStringFormat(&t_err, "invalid URL: %@", p_target);
		MCresult -> setvalueref(*t_err);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecPerformOpenSocket(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_from_address, MCNameRef p_message, bool p_datagram, bool p_secure, bool p_ssl, MCNameRef p_end_hostname)
{
	if (!ctxt . EnsureNetworkAccessIsAllowed() && !MCModeCanAccessDomain(MCNameGetString(p_name)))
		return;

	uindex_t t_index;
	if (IO_findsocket(p_name, t_index))
	{
		ctxt . SetTheResultToStaticCString("socket is already open");
		return;
	}

	// MW-2012-10-26: [[ Bug 10062 ]] Make sure we clear the result.
	MCresult -> clear();
    
    // MM-2014-06-13: [[ Bug 12567 ]] Added support for specifying an end host name to verify against.
	MCSocket *s = MCS_open_socket(p_name, p_from_address, p_datagram, ctxt . GetObject(), p_message, p_secure, p_ssl, kMCEmptyString, p_end_hostname);
	if (s != NULL)
        MCSocketsAppendToSocketList(s);
}

void MCNetworkExecOpenSocket(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_from_address, MCNameRef p_message, MCNameRef p_end_hostname)
{
	MCNetworkExecPerformOpenSocket(ctxt, p_name, p_from_address, p_message, false, false, false, p_end_hostname);
}

void MCNetworkExecOpenSecureSocket(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_from_address, MCNameRef p_message, MCNameRef p_end_hostname, bool p_with_verification)
{
	MCNetworkExecPerformOpenSocket(ctxt, p_name, p_from_address, p_message, false, true, p_with_verification, p_end_hostname);
}

void MCNetworkExecOpenDatagramSocket(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_from_address, MCNameRef p_message, MCNameRef p_end_hostname)
{
	MCNetworkExecPerformOpenSocket(ctxt, p_name, p_from_address, p_message, true, false, false, p_end_hostname);
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecCloseSocket(MCExecContext& ctxt, MCNameRef p_socket)
{
	uindex_t t_index;
	if (IO_findsocket(p_socket, t_index))	
	{		
		MCS_close_socket(MCsockets[t_index]);	
		ctxt . SetTheResultToEmpty();
	}
	else
		ctxt . SetTheResultToStaticCString("socket is not open");
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecPerformAcceptConnections(MCExecContext& ctxt, uint2 p_port, MCNameRef p_message, bool p_datagram, bool p_secure, bool p_with_verification)
{
	// MW-2005-01-28: Fix bug 2412 - accept doesn't clear the result.
	MCresult -> clear();
    ctxt . SetItToValue(kMCEmptyData);
    
	if (!ctxt . EnsureNetworkAccessIsAllowed())
		return;

	MCSocket *s = MCS_accept(p_port, ctxt . GetObject(), p_message, p_datagram ? True : False, p_secure ? True : False, p_with_verification ? True : False, kMCEmptyString);
	if (s != NULL)
    {
        MCSocketsAppendToSocketList(s);
        ctxt . SetItToValue(s -> name);
    }
}

void MCNetworkExecAcceptConnectionsOnPort(MCExecContext& ctxt, uint2 p_port, MCNameRef p_message)
{
	MCNetworkExecPerformAcceptConnections(ctxt, p_port, p_message, false, false, false);
}

void MCNetworkExecAcceptDatagramConnectionsOnPort(MCExecContext& ctxt, uint2 p_port, MCNameRef p_message)
{
	MCNetworkExecPerformAcceptConnections(ctxt, p_port, p_message, true, false, false);
}

void MCNetworkExecAcceptSecureConnectionsOnPort(MCExecContext& ctxt, uint2 p_port, MCNameRef p_message, bool p_with_verification)
{
	MCNetworkExecPerformAcceptConnections(ctxt, p_port, p_message, false, true, p_with_verification);
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecReadFromSocket(MCExecContext& ctxt, MCNameRef p_socket, uint4 p_count, MCStringRef p_sentinel, MCNameRef p_message)
{
	uindex_t t_index;
	if (IO_findsocket(p_socket, t_index))
	{
		if (MCsockets[t_index] -> datagram && (p_message == nil || p_message == kMCEmptyName))
		{
			ctxt . LegacyThrow(EE_READ_NOTVALIDFORDATAGRAM);
			return;
		}
		
		// MW-2012-10-26: [[ Bug 10062 ]] Make sure we clear the result.
		ctxt . SetTheResultToEmpty();

        MCDataRef t_data;
		if (p_sentinel != nil)
        {
            MCAutoPointer<char> t_sentinel;
            /* UNCHECKED */ MCStringConvertToCString(p_sentinel, &t_sentinel);
            t_data = MCS_read_socket(MCsockets[t_index], ctxt, p_count, *t_sentinel, p_message);
        }
		else
			t_data = MCS_read_socket(MCsockets[t_index], ctxt, p_count, nil, p_message);

		if (p_message == NULL)
		{
            // PM-2015-01-20: [[ Bug 14409 ]] Prevent a crash if MCS_read_socket fails
            if (t_data == nil)
                ctxt . SetItToValue(kMCEmptyData);
            else
                ctxt . SetItToValue(t_data);
		}
        
        MCValueRelease(t_data);
        
	}
	else
		ctxt . SetTheResultToStaticCString("socket is not open");
}

void MCNetworkExecReadFromSocketFor(MCExecContext& ctxt, MCNameRef p_socket, uint4 p_count, int p_unit_type, MCNameRef p_message)
{
	MCAutoStringRef t_until;
	switch (p_unit_type)
	{
	case FU_ITEM:
		MCStringCreateWithCString(",", &t_until);
		break;
	case FU_LINE:
		MCStringCreateWithCString("\n", &t_until);
		break;
	case FU_WORD:
		MCStringCreateWithCString(" ", &t_until);
		break;
	default:
		break;
	}
	MCNetworkExecReadFromSocket(ctxt, p_socket, p_count, *t_until, p_message);
}

void MCNetworkExecReadFromSocketUntil(MCExecContext& ctxt, MCNameRef p_socket, MCStringRef p_sentinel, MCNameRef p_message)
{
	MCNetworkExecReadFromSocket(ctxt, p_socket, 0, p_sentinel, p_message);
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecWriteToSocket(MCExecContext& ctxt, MCNameRef p_socket, MCStringRef p_data, MCNameRef p_message)
{
	uindex_t t_index;
	if (IO_findsocket(p_socket, t_index))
	{
		ctxt . SetTheResultToEmpty();
		MCS_write_socket(p_data, MCsockets[t_index], ctxt . GetObject(), p_message);
	}
	else
		ctxt . SetTheResultToStaticCString("socket is not open");
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecPutIntoUrl(MCExecContext& ctxt, MCValueRef p_value, int p_where, MCUrlChunkPtr p_chunk)
{
	MCAutoValueRef t_new_value;
	if (p_chunk . chunk == CT_UNDEFINED)
	{
		if (p_where == PT_INTO)
			t_new_value = p_value;
		else
		{
			MCAutoValueRef t_old_data;
			/* UNCHECKED */ MCU_geturl(ctxt, p_chunk.url, &t_old_data);
            
            if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeString
                && MCValueGetTypeCode(*t_old_data) == kMCValueTypeCodeString)
            {
                // Both old and new are strings
                if (p_where == PT_AFTER)
                    MCStringCreateWithStrings((MCStringRef&)&t_new_value, (MCStringRef)*t_old_data, (MCStringRef)p_value);
                else
                    MCStringCreateWithStrings((MCStringRef&)&t_new_value, (MCStringRef)p_value, (MCStringRef)*t_old_data);
            }
            else
            {
                // Not strings, treat as data
                MCAutoDataRef t_old, t_value;
                
                /* UNCHECKED */ ctxt.ConvertToData(*t_old_data, &t_old);
                /* UNCHECKED */ ctxt.ConvertToData(p_value, &t_value);
                
                if (p_where == PT_AFTER)
                    MCDataCreateWithData((MCDataRef&)&t_new_value, *t_old, *t_value);
                else
                    MCDataCreateWithData((MCDataRef&)&t_new_value, *t_value, *t_old);
            }
		}
	}
	else
	{
        MCAutoStringRef t_value;
        /* UNCHECKED */ ctxt . ConvertToString(p_value, &t_value);
        
        MCStringRef t_string;
        MCRange t_range;
        /* UNCHECKED */ MCStringMutableCopy((MCStringRef)p_chunk . mark . text, t_string);

        // SN-2015-05-19: [[ Bug 15368 ]] Insert the new string at the right
        //  position: might be after or before the chunk, not only into it.
        if (p_where == PT_INTO)
            t_range = MCRangeMakeMinMax(p_chunk . mark . start, p_chunk . mark . finish);
        else if (p_where == PT_BEFORE)
            t_range = MCRangeMake(p_chunk . mark . start, 0);
        else // p_where == PT_AFTER
            t_range = MCRangeMake(p_chunk . mark . finish, 0);

        /* UNCHECKED */ MCStringReplace(t_string, t_range, *t_value);
		/* UNCHECKED */ MCStringCopyAndRelease(t_string, (MCStringRef&)&t_new_value);
	}
	
	//ctxt.SetTheResultToValue(*t_new_value);
    ctxt.SetTheResultToEmpty();

	/* UNCHECKED */ MCU_puturl(ctxt, p_chunk.url, *t_new_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecReturnValueAndUrlResult(MCExecContext& ctxt, MCValueRef p_result, MCValueRef p_url_result)
{
	ctxt . SetTheResultToValue(p_result);
	if (MCurlresult -> setvalueref(p_url_result))
		return;
	
	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkGetUrlResponse(MCExecContext& ctxt, MCStringRef& r_value)
{
	// MW-2008-08-12: Add access to the MCurlresult internal global variable
	//   this is set by libURL after doing DELETE, POST, PUT or GET type queries.
    ctxt.ConvertToString(MCurlresult -> getvalueref(), r_value);
}

void MCNetworkGetFtpProxy(MCExecContext& ctxt, MCStringRef& r_value)
{
	
	if (MCStringIsEmpty(MCftpproxyhost))
	{
		r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
		return;
	}
	else
	{
		if (MCStringFormat(r_value, "%@:%d", MCftpproxyhost, MCftpproxyport))
			return;
	}

	ctxt . Throw();
}

void MCNetworkSetFtpProxy(MCExecContext& ctxt, MCStringRef p_value)
{
	
	MCAutoStringRef t_host, t_port;
	/* UNCHECKED */ MCStringDivideAtChar(p_value, ':', kMCCompareExact, &t_host, &t_port);
	if (*t_port != nil)
		/* UNCHECKED */ MCStringToUInt16(*t_port, MCftpproxyport);
	else
		MCftpproxyport = 80;
	MCValueAssign(MCftpproxyhost, *t_host);
		
}

void MCNetworkGetHttpProxy(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(MChttpproxy);
}

void MCNetworkSetHttpProxy(MCExecContext& ctxt, MCStringRef p_value)
{
	MCValueAssign(MChttpproxy, p_value);
}

void MCNetworkGetHttpHeaders(MCExecContext& ctxt, MCStringRef& r_value)
{
		r_value = MCValueRetain(MChttpheaders);
}

void MCNetworkSetHttpHeaders(MCExecContext& ctxt, MCStringRef p_value)
{
	MCValueAssign(MChttpheaders, p_value);
}

void MCNetworkGetSocketTimeout(MCExecContext& ctxt, double& r_value)
{
	r_value = MCsockettimeout * 1000;
}

void MCNetworkSetSocketTimeout(MCExecContext& ctxt, double p_value)
{
	MCsockettimeout = p_value;
	if (MCsockettimeout < 1.0)
		MCsockettimeout = 0.001;
	else
		MCsockettimeout /= 1000.0;
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkGetDefaultNetworkInterface(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (MCStringCreateWithCString(MCdefaultnetworkinterface, r_value))
		return;

	ctxt . Throw();
}

void MCNetworkSetDefaultNetworkInterface(MCExecContext& ctxt, MCStringRef p_value)
{
	if (MCStringGetLength(p_value) == 0)
	{
		delete MCdefaultnetworkinterface;
		MCdefaultnetworkinterface = nil;
	}
	else
	{
		regexp *t_net_int_regex;
		t_net_int_regex = MCR_compile(MCSTR("\\b(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b"), true /* casesensitive */);
		int t_net_int_valid;
		t_net_int_valid = MCR_exec(t_net_int_regex, p_value, MCRangeMake(0, MCStringGetLength(p_value)));
		delete t_net_int_regex;
		if (t_net_int_valid != 0)
		{
			delete MCdefaultnetworkinterface;
            char* t_value;
            /* UNCHECKED */ MCStringConvertToCString(p_value, t_value);
			MCdefaultnetworkinterface = t_value;
		}
		else
			ctxt . LegacyThrow(EE_PROPERTY_BADNETWORKINTERFACE);
	}
}

void MCNetworkGetNetworkInterfaces(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (MCS_getnetworkinterfaces(r_value))
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkGetAllowDatagramBroadcasts(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCallowdatagrambroadcasts == True;
}

void MCNetworkSetAllowDatagramBroadcasts(MCExecContext& ctxt, bool p_value)
{
	MCallowdatagrambroadcasts = p_value;
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecSetUrl(MCExecContext& ctxt, MCValueRef p_value, MCStringRef p_url)
{
    MCU_puturl(ctxt, p_url, p_value);
}

void MCNetworkExecPutIntoUrl(MCExecContext& ctxt, MCValueRef p_value, int p_where, MCStringRef p_url)
{
    if (p_where == PT_INTO)
        MCNetworkExecSetUrl(ctxt, p_value, p_url);
    else
    {
        MCAutoValueRef t_old_data;
        /* UNCHECKED */ MCU_geturl(ctxt, p_url, &t_old_data);
        
        if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeString
            && MCValueGetTypeCode(*t_old_data) == kMCValueTypeCodeString)
        {
            MCAutoStringRef t_new_value;
            
            // Both old and new are strings
            if (p_where == PT_AFTER)
                MCStringCreateWithStrings(&t_new_value, (MCStringRef)*t_old_data, (MCStringRef)p_value);
            else
                MCStringCreateWithStrings(&t_new_value, (MCStringRef)p_value, (MCStringRef)*t_old_data);
            
            MCNetworkExecSetUrl(ctxt, *t_new_value, p_url);
        }
        else
        {
            // Not strings, treat as data
            MCDataRef t_old, t_new;
            MCAutoDataRef t_value, t_new_value;
            
            /* UNCHECKED */ ctxt.ConvertToData(*t_old_data, t_old);
            /* UNCHECKED */ ctxt.ConvertToData(p_value, &t_value);
            
            /* UNCHECKED */ MCDataMutableCopyAndRelease(t_old, t_new);
            if (p_where == PT_AFTER)
            /* UNCHECKED */ MCDataAppend(t_new, *t_value);
            else
            /* UNCHECKED */ MCDataPrepend(t_new, *t_value);
            
            /* UNCHECKED */ MCDataCopyAndRelease(t_new, &t_new_value);
            
            MCNetworkExecSetUrl(ctxt, *t_new_value, p_url);
        }
    }
}

void MCNetworkMarkUrl(MCExecContext& ctxt, MCStringRef p_url, MCMarkedText& r_mark)
{
    MCAutoValueRef t_data;

    MCU_geturl(ctxt, p_url, &t_data);
    /* UNCHECKED */ ctxt . ConvertToString(*t_data, (MCStringRef &)r_mark . text);
    
    r_mark . start = 0;
    r_mark . finish = MAXUINT4;
}
