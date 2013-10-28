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

MC_EXEC_DEFINE_EVAL_METHOD(Network, DNSServers, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Network, CachedUrls, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Network, UrlStatus, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Network, HostAddress, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Network, PeerAddress, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Network, HostAddressToName, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Network, HostNameToAddress, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Network, HostName, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Network, OpenSockets, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Network, HTTPProxyForURL, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Network, HTTPProxyForURLWithPAC, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Network, CloseSocket, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Network, DeleteUrl, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Network, LoadUrl, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Network, UnloadUrl, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Network, OpenSocket, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Network, OpenSecureSocket, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Network, OpenDatagramSocket, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Network, PostToUrl, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Network, AcceptConnectionsOnPort, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Network, AcceptDatagramConnectionsOnPort, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Network, AcceptSecureConnectionsOnPort, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Network, ReadFromSocketFor, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Network, ReadFromSocketUntil, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Network, WriteToSocket, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Network, PutIntoUrl, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Network, ReturnValueAndUrlResult, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Network, ReturnValueAndUrlResultFromVar, 2)
MC_EXEC_DEFINE_GET_METHOD(Network, UrlResponse, 1)
MC_EXEC_DEFINE_GET_METHOD(Network, FtpProxy, 1)
MC_EXEC_DEFINE_SET_METHOD(Network, FtpProxy, 1)
MC_EXEC_DEFINE_GET_METHOD(Network, HttpProxy, 1)
MC_EXEC_DEFINE_SET_METHOD(Network, HttpProxy, 1)
MC_EXEC_DEFINE_GET_METHOD(Network, HttpHeaders, 1)
MC_EXEC_DEFINE_SET_METHOD(Network, HttpHeaders, 1)
MC_EXEC_DEFINE_GET_METHOD(Network, SocketTimeout, 1)
MC_EXEC_DEFINE_SET_METHOD(Network, SocketTimeout, 1)
MC_EXEC_DEFINE_GET_METHOD(Network, DefaultNetworkInterface, 1)
MC_EXEC_DEFINE_SET_METHOD(Network, DefaultNetworkInterface, 1)
MC_EXEC_DEFINE_GET_METHOD(Network, NetworkInterfaces, 1)

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

	ctxt.Throw();
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

	MCAutoListRef t_list;
	if (MCS_ntoa(p_hostname, ctxt.GetObject(), p_message, &t_list) &&
		MCListCopyAsString(*t_list, r_string))
		return;

	ctxt . Throw();
}

//////////

void MCNetworkEvalHostName(MCExecContext& ctxt, MCStringRef& r_string)
{
#ifndef _MOBILE
	if (MCS_hn(r_string))
		return;

	ctxt.Throw();
#endif
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
		return;

	const char *t_arguments[2];
	t_arguments[0] = MCStringGetCString(p_url);
	t_arguments[1] = MCStringGetCString(p_host);

	MCAutoPointer<char> t_proxies;
	t_proxies = s_pac_engine -> Call("__FindProxyForURL", t_arguments, 2);

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
				char *t_result;
				t_result = s_pac_engine -> Run(MCStringGetCString(p_pac));
				t_success = t_result != NULL;
				delete t_result;
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
{
	MCS_loadurl(ctxt . GetObject(), p_url, p_message);
}

void MCNetworkExecUnloadUrl(MCExecContext& ctxt, MCStringRef p_url)
{
	MCS_unloadurl(ctxt . GetObject(), p_url);
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecPostToUrl(MCExecContext& ctxt, MCDataRef p_data, MCStringRef p_url)
{
	MCS_posttourl(ctxt . GetObject(), p_data, p_url);
	ctxt . SetItToValue(MCurlresult -> getvalueref());
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecDeleteUrl(MCExecContext& ctxt, MCStringRef p_target)
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
			MCStringCopySubstring(p_target, MCRangeMake(5, MCStringGetLength(p_target)-5), &t_filename);
		else
			MCStringCopySubstring(p_target, MCRangeMake(8, MCStringGetLength(p_target)-8), &t_filename);
		if (!MCS_unlink(*t_filename))
			ctxt . SetTheResultToStaticCString("can't delete that file");
		else
			ctxt . SetTheResultToEmpty();
	}
	else 
	{
		if (MCStringGetLength(p_target) > 8 &&
			MCStringBeginsWithCString(p_target, (const char_t*)"resfile:", kMCCompareCaseless))
		{
			MCStringCopySubstring(p_target, MCRangeMake(8, MCStringGetLength(p_target)-8), &t_filename);
			MCS_saveresfile(*t_filename, kMCEmptyData);
		}
		else
			MCS_deleteurl(ctxt . GetObject(), p_target);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkExecPerformOpenSocket(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_message, bool p_datagram, bool p_secure, bool p_ssl)
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
	MCresult -> clear(True);

	MCSocket *s = MCS_open_socket(p_name, p_datagram, ctxt . GetObject(), p_message, p_secure, p_ssl, kMCEmptyString);
	if (s != NULL)
	{
		MCU_realloc((char **)&MCsockets, MCnsockets, MCnsockets + 1, sizeof(MCSocket *));
		MCsockets[MCnsockets++] = s;
	}
}

void MCNetworkExecOpenSocket(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_message)
{
	MCNetworkExecPerformOpenSocket(ctxt, p_name, p_message, false, false, false);
}

void MCNetworkExecOpenSecureSocket(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_message, bool p_with_verification)
{
	MCNetworkExecPerformOpenSocket(ctxt, p_name, p_message, false, true, p_with_verification);
}

void MCNetworkExecOpenDatagramSocket(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_message)
{
	MCNetworkExecPerformOpenSocket(ctxt, p_name, p_message, true, false, false);
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
	MCresult -> clear(False);

	if (!ctxt . EnsureNetworkAccessIsAllowed())
		return;

	MCSocket *s = MCS_accept(p_port, ctxt . GetObject(), p_message, p_datagram ? True : False, p_secure ? True : False, p_with_verification ? True : False, kMCEmptyString);
	if (s != NULL)
	{
		MCU_realloc((char **)&MCsockets, MCnsockets,
		            MCnsockets + 1, sizeof(MCSocket *));
		MCsockets[MCnsockets++] = s;
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

		MCAutoDataRef t_data;
		
		if (p_sentinel != nil)
			MCS_read_socket(MCsockets[t_index], ctxt, p_count, MCStringGetCString(p_sentinel), p_message, &t_data);
		else
			MCS_read_socket(MCsockets[t_index], ctxt, 0, nil, p_message, &t_data);

		if (p_message == NULL)
		{
			ctxt . SetItToValue(*t_data);
		}
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

void MCNetworkExecPutIntoUrl(MCExecContext& ctxt, MCStringRef p_value, int p_where, MCUrlChunkPtr p_chunk)
{
	MCAutoStringRef t_new_value;
	if (p_chunk . chunk == CT_UNDEFINED)
	{
		if (p_where == PT_INTO)
			t_new_value = p_value;
		else
		{
			MCStringRef t_old_data;
			/* UNCHECKED */ MCU_geturl(ctxt, p_chunk.url, t_old_data);
			/* UNCHECKED */ MCStringMutableCopyAndRelease(t_old_data, t_old_data);
			if (p_where == PT_AFTER)
				/* UNCHECKED */ MCStringAppend(t_old_data, p_value);
			else
				/* UNCHECKED */ MCStringPrepend(t_old_data, p_value);
			/* UNCHECKED */ MCStringCopyAndRelease(t_old_data, &t_new_value);
		}
	}
	else
	{
		MCAutoStringRef t_old_data;
		/* UNCHECKED */ MCU_geturl(ctxt, p_chunk.url, &t_old_data);
		
		MCStringRef t_string;
		/* UNCHECKED */ MCStringMutableCopy(*t_old_data, t_string);
		/* UNCHECKED */ MCStringReplace(t_string, MCRangeMake(p_chunk.mark.start, p_chunk.mark.finish - p_chunk.mark.start), p_value);
		/* UNCHECKED */ MCStringCopyAndRelease(t_string, &t_new_value);
	}
	
	ctxt.SetTheResultToValue(*t_new_value);

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

void MCNetworkExecReturnValueAndUrlResultFromVar(MCExecContext& ctxt, MCValueRef p_result, MCVarref *p_variable)
{
    MCAutoValueRef t_value;
	if (!p_variable -> eval(ctxt, &t_value))
	{
		ctxt . LegacyThrow(EE_RETURN_BADEXP);
		return;
	}
	
	ctxt . SetTheResultToValue(p_result);
	MCurlresult -> set(ctxt, *t_value);
	p_variable -> dofree(ctxt);
}

////////////////////////////////////////////////////////////////////////////////

void MCNetworkGetUrlResponse(MCExecContext& ctxt, MCStringRef& r_value)
{
	// MW-2008-08-12: Add access to the MCurlresult internal global variable
	//   this is set by libURL after doing DELETE, POST, PUT or GET type queries.
	r_value = (MCStringRef)MCValueRetain(MCurlresult -> getvalueref());
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
		if (MCStringFormat(r_value, "%s:%d", MCStringGetCString(MCftpproxyhost), MCftpproxyport))
			return;
	}

	ctxt . Throw();
}

void MCNetworkSetFtpProxy(MCExecContext& ctxt, MCStringRef p_value)
{
	
	MCAutoStringRef t_host, t_port;
	/* UNCHECKED */ MCStringDivideAtChar(p_value, ':', kMCCompareCaseless, &t_host, &t_port);
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
		t_net_int_valid = MCR_exec(t_net_int_regex, p_value);
		MCR_free(t_net_int_regex);			
		if (t_net_int_valid != 0)
		{
			delete MCdefaultnetworkinterface;
			MCdefaultnetworkinterface = strclone(MCStringGetCString(p_value));
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

void MCNetworkExecSetUrl(MCExecContext& ctxt, MCStringRef p_value, MCStringRef p_url)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecPoint ep2(nil, nil, nil);
    
    ep . setvalueref(p_url);
    ep2 . setvalueref(p_value);
    
	MCU_puturl(ep, ep2);
}

void MCNetworkExecPutIntoUrl(MCExecContext& ctxt, MCStringRef p_value, int p_where, MCStringRef p_url)
{
    if (p_where == PT_INTO)
        MCNetworkExecSetUrl(ctxt, p_value, p_url);
    else
    {
        MCAutoStringRef t_string;
        MCU_geturl(ctxt, p_url, &t_string);
        
        MCAutoStringRef t_new_value;
        MCStringMutableCopy(*t_string, &t_new_value);
        
        if (p_where == PT_AFTER)
            MCStringAppend(*t_new_value, p_value);
        else
            MCStringPrepend(*t_new_value, p_value);
        MCNetworkExecSetUrl(ctxt, *t_new_value, p_url);
    }
}

void MCNetworkMarkUrl(MCExecContext& ctxt, MCStringRef p_url, MCMarkedText& r_mark)
{
    MCU_geturl(ctxt, p_url, r_mark . text);
    r_mark . start = 0;
    r_mark . finish = MAXUINT4;
}
