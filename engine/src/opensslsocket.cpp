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

#include "object.h"
#include "stack.h"
#include "card.h"
#include "mcerror.h"
#include "execpt.h"
#include "param.h"
#include "handler.h"
#include "util.h"
#include "globals.h"
#include "mcssl.h"
#include "osspec.h"

#include "ports.cpp"

#include "notify.h"
#include "socket.h"

#if defined(_WINDOWS_DESKTOP)
#include "w32prefix.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincrypt.h>
#include <iphlpapi.h>
#elif defined(_MAC_DESKTOP)
#include "osxprefix.h"
#include <SystemConfiguration/SCDynamicStore.h>
#include <SystemConfiguration/SCDynamicStoreKey.h>
#include <SystemConfiguration/SCSchemaDefinitions.h>
#include <Security/Security.h>
extern char *osx_cfstring_to_cstring(CFStringRef p_string, bool p_release);
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef MCSSL

#ifndef _WINDOWS
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netinet/in_systm.h>
#include <netinet/udp.h>
#include <netinet/in.h>

#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <net/if.h>

#include <resolv.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#endif

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#if !defined(X11) && (!defined(_MACOSX))
#define socklen_t int
#endif

extern bool MCNetworkGetHostFromSocketId(MCStringRef p_socket, MCStringRef& r_host);

extern real8 curtime;

static char *sslerror = NULL;
static long post_connection_check(SSL *ssl, char *host);
static int verify_callback(int ok, X509_STORE_CTX *store);

#ifdef _MACOSX
extern bool path2utf(MCStringRef p_path, MCStringRef& r_utf);
#endif

#ifdef _WINDOWS
extern Boolean wsainit(void);
extern HWND sockethwnd;
extern "C" char *strdup(const char *);
#endif

#define READ_SOCKET_SIZE  4096

Boolean MCSocket::sslinited = False;

#ifdef _MACOSX
static void socketCallback (CFSocketRef cfsockref, CFSocketCallBackType type, CFDataRef address, const void *pData, void *pInfo)
{
	uint2 i;
	int fd = CFSocketGetNative(cfsockref);
	for (i = 0 ; i < MCnsockets ; i++)
	{
		if ( fd == MCsockets[i]->fd && !MCsockets[i]->shared)
			break;
	}
	if (i < MCnsockets)
	{
		fd_set rmaskfd, wmaskfd, emaskfd;
		FD_ZERO(&rmaskfd);
		FD_ZERO(&wmaskfd);
		FD_ZERO(&emaskfd);
		FD_SET(fd, &rmaskfd);
		struct timeval t_time = {0,0};
		select(fd, &rmaskfd, &wmaskfd, &emaskfd, &t_time);
		switch (type)
		{
		case kCFSocketReadCallBack:
			if (FD_ISSET(fd, &rmaskfd))
			{
				MCsockets[i]->readsome();
				MCsockets[i]->setselect();
			}
			break;
		case kCFSocketWriteCallBack:
			MCsockets[i]->writesome();
			MCsockets[i]->setselect();
			break;
		case kCFSocketConnectCallBack:
			MCsockets[i]->writesome();
			MCsockets[i]->readsome();
			break;
		}
	}
	//MCS_poll(0.0,0);//quick poll of other sockets
}
#endif

#if defined(_MACOSX)
Boolean MCS_handle_sockets()
{
	return MCS_poll(0.0, 0.0);
}
#endif

#ifdef _WINDOWS
typedef SOCKADDR_IN mc_sockaddr_in_t;

bool MCS_init_sockets()
{
	return wsainit() == True;
}

static inline bool MCS_valid_socket(MCSocketHandle p_socket)
{
	return p_socket != INVALID_SOCKET;
}

static int MCS_socket_ioctl(MCSocketHandle p_socket, long p_command, unsigned long &x_args)
{
	return ioctlsocket(p_socket, p_command, &x_args);
}

#else
typedef struct sockaddr_in mc_sockaddr_in_t;

bool MCS_init_sockets()
{
	return true;
}

static inline bool MCS_valid_socket(MCSocketHandle p_socket)
{
	return p_socket >= 0;
}

static int MCS_socket_ioctl(MCSocketHandle p_socket, long p_command, unsigned long &x_args)
{
	return ioctl(p_socket, p_command, &x_args);
}

#endif

#ifdef _WINDOWS
int inet_aton(const char *cp, struct in_addr *inp)
{
	unsigned long rv = inet_addr(cp);
	if (rv == -1)
		return False;
	memcpy(inp, &rv, sizeof(unsigned long));
	return True;
}
#endif

bool MCS_compare_host_domain(MCStringRef p_host_a, MCStringRef p_host_b)
{
	struct sockaddr_in t_host_a, t_host_b;

	if (MCS_name_to_sockaddr(p_host_a, t_host_a) && MCS_name_to_sockaddr(p_host_b, t_host_b))
	{
		return t_host_a.sin_addr.s_addr == t_host_b.sin_addr.s_addr;
	}
	else
		return false;
}

////////////////////////////////////////////////////////////////////////////////
//
// IP / hostname lookup functions
//

bool MCS_ha(MCSocket *s, MCStringRef& r_string)
{
	mc_sockaddr_in_t addr;

	socklen_t addrsize = sizeof(addr);
	getsockname(s->fd, (sockaddr *)&addr, &addrsize);

	return MCS_sockaddr_to_string((sockaddr *)&addr, addrsize, false, r_string);
}

bool MCS_hn(MCStringRef& r_string)
{
	if (!MCS_init_sockets())
	{
		r_string = MCValueRetain(kMCEmptyString);
		return true;
	}

	MCAutoNativeCharArray t_buffer;
	if (!t_buffer.Resize(MAXHOSTNAMELEN + 1))
		return false;

	gethostname((char*)t_buffer.Chars(), MAXHOSTNAMELEN);
	t_buffer.Shrink(MCCStringLength((char*)t_buffer.Chars()));

	return t_buffer.CreateStringAndRelease(r_string);
}

bool MCS_aton(MCStringRef p_address, MCStringRef& r_name)
{
	if (!MCS_init_sockets())
	{
		r_name = MCValueRetain(kMCEmptyString);
		return true;
	}

	MCAutoStringRef t_host;
	if (!MCNetworkGetHostFromSocketId(p_address, &t_host))
		return false;

	bool t_success = true;

	struct sockaddr_in t_addr;
	t_success = MCS_name_to_sockaddr(*t_host, t_addr);
	if (t_success)
	{
		t_success = MCS_sockaddr_to_string((sockaddr*)&t_addr, sizeof(t_addr), true, r_name);
	}
	if (t_success)
	{
		MCresult->sets("");
	}
	else
	{
		MCresult->sets("invalid host address");
		r_name = MCValueRetain(kMCEmptyString);
		t_success = true;
	}

	return t_success;
}

bool MCS_dnsresolve(MCStringRef p_hostname, MCStringRef& r_dns)
{
	if (!MCS_init_sockets())
		return false;

	bool t_success = true;

	struct sockaddr_in t_addr;
	t_success = MCS_name_to_sockaddr(p_hostname, t_addr);
	if (t_success)
	{
		t_success = MCS_sockaddr_to_string((sockaddr*)&t_addr, sizeof(t_addr), false, r_dns);
	}

	if (t_success)
		return true;
	else
		return false;
}

bool ntoa_callback(void *p_context, bool p_resolved, bool p_final, struct sockaddr *p_addr, int p_addrlen)
{
	if (p_resolved)
	{
		MCListRef t_list = (MCListRef) p_context;
		MCAutoStringRef t_name;
		if (MCS_sockaddr_to_string(p_addr, p_addrlen, false, &t_name))
			return MCListAppend(t_list, *t_name);
	}
	return true;
}

typedef struct _mc_ntoa_message_callback_info
{
	MCObjectHandle *target;
	MCStringRef name;
	MCNameRef message;
	MCListRef list;
} MCNToAMessageCallbackInfo;

static void free_ntoa_message_callback_info(MCNToAMessageCallbackInfo *t_info)
{
	if (t_info != NULL)
	{
		MCValueRelease(t_info->message);
		MCValueRelease(t_info->name);
		MCValueRelease(t_info->list);
		if (t_info->target)
			t_info->target->Release();
		MCMemoryDelete(t_info);
	}
}

bool ntoa_message_callback(void *p_context, bool p_resolved, bool p_final, struct sockaddr *p_addr, int p_addrlen)
{
	MCNToAMessageCallbackInfo *t_info = (MCNToAMessageCallbackInfo*)p_context;
	ntoa_callback(t_info->list, p_resolved, p_final, p_addr, p_addrlen);

	if (p_final)
	{
		MCAutoStringRef t_string;
		/* UNCHECKED */ MCListCopyAsString(t_info->list, &t_string);
		MCscreen->delaymessage(t_info->target->Get(), t_info->message, strclone(MCStringGetCString(t_info->name)), strclone(MCStringGetCString(*t_string)));
		free_ntoa_message_callback_info(t_info);
	}
	return true;
}

bool MCS_ntoa(MCStringRef p_hostname, MCObject *p_target, MCNameRef p_message, MCListRef& r_addr)
{
	if (!MCS_init_sockets())
	{
		r_addr = MCValueRetain(kMCEmptyList);
		return true;
	}

	MCAutoStringRef t_host;
	if (!MCNetworkGetHostFromSocketId(p_hostname, &t_host))
		return false;

	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	bool t_success = true;

	if (MCNameIsEqualTo(p_message, kMCEmptyName))
	{
		t_success = MCSocketHostNameResolve(MCStringGetCString(*t_host), NULL, SOCK_STREAM, true, ntoa_callback, *t_list);
	}
	else
	{
		MCNToAMessageCallbackInfo *t_info = NULL;
		t_success = MCMemoryNew(t_info);
		if (t_success)
		{
			t_info->message = MCValueRetain(p_message);
			t_success = MCStringCopy(p_hostname, t_info->name);
		}
		if (t_success)
			t_success = MCListCreateMutable('\n', t_info->list);

		if (t_success)
		{
			t_info->target = p_target->gethandle();
			t_success = MCSocketHostNameResolve(MCStringGetCString(*t_host), NULL, SOCK_STREAM, false, ntoa_message_callback, t_info);
		}
		
		if (!t_success)
			free_ntoa_message_callback_info(t_info);
	}

	if (!t_success)
	{
		/* RESULT - !t_success doesn't necessarily mean an invalid address here */
		MCresult->sets("invalid host address");
	}
	else
	{
		MCresult->sets("");
	}

	if (t_success)
		t_success = MCListCopy(*t_list, r_addr);

	return t_success;
}

bool MCS_pa(MCSocket *s, MCStringRef& r_string)
{
	struct sockaddr_in addr;
	socklen_t addrsize = sizeof(addr);
	getpeername(s->fd, (sockaddr *)&addr, &addrsize);

	return MCS_sockaddr_to_string((sockaddr *)&addr, addrsize, false, r_string);
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_connect_socket(MCSocket *p_socket, struct sockaddr_in *p_addr)
{
	p_socket->resolve_state = kMCSocketStateConnecting;	
	if (p_socket != NULL && p_socket->fd != 0)
	{
		
		// MM-2011-07-07: Added support for binding sockets to a network interface.
		if (MCdefaultnetworkinterface != NULL)
		{
			struct sockaddr_in t_bind_addr;
			MCAutoStringRef MCdefaultnetworkinterface_string;
			/* UNCHECKED */ MCStringCreateWithCString(MCdefaultnetworkinterface, &MCdefaultnetworkinterface_string);
			if (!MCS_name_to_sockaddr(*MCdefaultnetworkinterface_string, t_bind_addr))
			{
				p_socket->error = strclone("can't resolve network interface");
				p_socket->doclose();
				return false;
			}
			
			t_bind_addr.sin_port = 0;
			
			if (0 != bind(p_socket->fd, (struct sockaddr *)&t_bind_addr, sizeof(struct sockaddr_in)))
			{
				p_socket->error = strclone("can't bind to network interface address");
				p_socket->doclose();
				return false;
			}
		}
		
		p_socket->setselect();

#ifdef _WINDOWS

		if (connect(p_socket->fd, (struct sockaddr *)p_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR && errno != EINTR)
		{
			int wsaerr = WSAGetLastError();
			if (wsaerr != WSAEWOULDBLOCK)
			{
				p_socket->error = strclone("can't connect to host");
				p_socket->doclose();
				return false;
			}
		}
#else
		if ((connect(p_socket->fd, (struct sockaddr *)p_addr, sizeof(struct sockaddr_in)) == -1 && errno != EINPROGRESS && errno != EINTR))
		{
				p_socket->error = strclone("can't connect to host");
				p_socket->doclose();
			return false;
		}
#endif

	}
	return true;
}

typedef struct _mc_open_socket_callback_info
{
	MCSocket *m_socket;
	struct sockaddr_in m_sockaddr;
} MCOpenSocketCallbackInfo;

bool open_socket_resolve_callback(void *p_context, bool p_resolved, bool p_final, struct sockaddr *p_addr, int p_addrlen)
{
	MCOpenSocketCallbackInfo *t_info = (MCOpenSocketCallbackInfo*)p_context;
	MCSocket *t_socket = t_info->m_socket;
	if (t_socket->closing)
		t_socket->resolve_state = kMCSocketStateClosed;
	else
	{
		if (p_resolved)
		{
			struct sockaddr_in *t_addr = (struct sockaddr_in*)p_addr;
			t_addr->sin_family = t_info->m_sockaddr.sin_family;
			t_addr->sin_port = t_info->m_sockaddr.sin_port;
			MCS_connect_socket(t_socket, t_addr);
		}
		else
		{
			t_socket->resolve_state = kMCSocketStateError;
			t_socket->error = strclone("can't resolve hostname");
			t_socket->doclose();
		}
	}
	MCMemoryDelete(t_info);
	return false;
}

MCSocket *MCS_open_socket(char *name, Boolean datagram, MCObject *o, MCNameRef mess, Boolean secure, Boolean sslverify, char *sslcertfile)
{
	if (!MCS_init_sockets())
		return NULL;

	struct sockaddr_in t_addr;
	if (mess == NULL)
	{
		MCAutoStringRef t_name_string;
		/* UNCHECKED */ MCStringCreateWithCString(name, &t_name_string);
		if (!MCS_name_to_sockaddr(*t_name_string, t_addr))
			return NULL;
	}

	MCSocketHandle sock = socket(AF_INET, datagram ? SOCK_DGRAM : SOCK_STREAM, 0);

	if (!MCS_valid_socket(sock))
	{
#ifdef _WINDOWS
		MCS_seterrno(WSAGetLastError());
#endif
		MCresult->sets("can't create socket");
		return NULL;
	}

	unsigned long on = 1;

	// set socket nonblocking
	MCS_socket_ioctl(sock, FIONBIO, on);

	MCSocket *s = NULL;
	s = (MCSocket *)new MCSocket(name, o, mess, datagram, sock, False, False,secure);

	if (s != NULL)
	{
		s->setselect();

		if (secure)
		{
			s->sslstate |= SSTATE_RETRYCONNECT;
		}

		s->sslverify = sslverify;

		if (mess == NULL)
		{
			if (!MCS_connect_socket(s, &t_addr))
			{
				s->name = NULL;
				if (s->error != NULL)
					MCresult->copysvalue(MCString(s->error));
				else
					MCresult->sets("can't connect to host");
				delete s;
				s = NULL;
			}
		}
		else
		{
			MCOpenSocketCallbackInfo *t_info;
			MCMemoryNew(t_info);
			t_info->m_socket = s;
			s->resolve_state = kMCSocketStateResolving;
			MCAutoStringRef t_sname_string;
			/* UNCHECKED */ MCStringCreateCString(s->name, &t_sname_string);
			if (!MCS_name_to_sockaddr(*t_sname_string, &t_info->m_sockaddr, open_socket_resolve_callback, t_info))
			{
				MCMemoryDelete(t_info);
				s->name = nil;
				delete s;
				s = nil;

				if (MCresult->isempty())
					MCresult->sets("can't resolve hostname");
			}
		}
	}
	return s;
}

void MCS_close_socket(MCSocket *s)
{
	s->deletereads();

#ifdef _WINDOWS
	if (s->wevents == NULL)
#else
	if (s->wevents == NULL || (s->secure && s->sslstate & SSLRETRYFLAGS))
#endif
		s->deletewrites();

	s->closing = True;
}

void MCS_read_socket(MCSocket *s, MCExecPoint &ep, uint4 length, const char *until, MCNameRef mptr)
{
	ep.clear();
	if (s->datagram)
	{
		MCNameDelete(s->message);
		/* UNCHECKED */ MCNameClone(mptr, s -> message);
		s->object = ep.getobj();
		
	}
	else
	{
		MCSocketread *eptr = new MCSocketread(length, until != nil ? strdup(until) : nil, ep.getobj(), mptr);
		eptr->appendto(s->revents);
		s->setselect();
		if (s->accepting)
		{
			MCresult->sets("can't read from this socket");
			return;
		}
		if (until == NULL)
		{
			if (length > s->rsize - s->nread)
			{
				MCU_realloc((char **)&s->rbuffer, s->nread,
				            length + s->nread, sizeof(char));
				s->rsize = length + s->nread;
			}
		}
		if (mptr != NULL)
		{
#ifdef _WINDOWS
			if (MCnoui)
				s->doread = True;
			else
				PostMessageA(sockethwnd, WM_USER, s->fd, FD_OOB);
#else
			s->doread = True;
#endif
			s->processreadqueue();
			MCresult->clear(True);
			
		}
		
		else
		{
			s->waiting = True;
			while (True)
			{
				if (eptr == s->revents && s->read_done())
				{
					uint4 size = eptr->size;
					if (until != NULL && *until == '\n' && !*(until + 1)
					        && size && s->rbuffer[size - 1] == '\r')
						size--;
					ep.copysvalue(s->rbuffer, size);
					s->nread -= eptr->size;
					// MW-2010-11-19: [[ Bug 9182 ]] This should be a memmove (I think)
					memmove(s->rbuffer, s->rbuffer + eptr->size, s->nread);
					break;
				}
				if (s->error != NULL)
				{
					MCresult->sets(s->error);
					break;
				}
				if (s->fd == 0)
				{
					MCresult->sets("eof");
					break;
				}
				if (curtime > eptr->timeout)
				{
					MCresult->sets("timeout");
					break;
				}
				MCU_play();
				if (MCscreen->wait(READ_INTERVAL, False, True))
				{
					MCresult->sets("interrupted");
					break;
				}
			}
			eptr->remove
			(s->revents);
			delete eptr;
			s->waiting = False;
		}
	}
}

void MCS_write_socket(const MCStringRef d, MCSocket *s, MCObject *optr, MCNameRef mptr)
{
	if (s->datagram)
	{
		// MW-2012-11-13: [[ Bug 10516 ]] Set the 'broadcast' flag based on whether the
		//   user has enabled broadcast addresses.
		int t_broadcast;
		t_broadcast = MCallowdatagrambroadcasts ? 1 : 0;
		setsockopt(s -> fd, SOL_SOCKET, SO_BROADCAST, (const char *)&t_broadcast, sizeof(t_broadcast));
	
		if (s->shared)
		{
			char *portptr = strchr(s->name, ':');
			*portptr = '\0';
			struct sockaddr_in to;
			memset((char *)&to, 0, sizeof(to));
			to.sin_family = AF_INET;
			uint2 port = atoi(portptr + 1);
			to.sin_port = MCSwapInt16HostToNetwork(port);
			if (!inet_aton(s->name, (in_addr *)&to.sin_addr.s_addr)
			        || sendto(s->fd, MCStringGetCString(d), MCStringGetLength(d), 0,
			                  (sockaddr *)&to, sizeof(to)) < 0)
			{
				mptr = NULL;
				MCresult->sets("error sending datagram");
			}
			*portptr = ':';
		}
		else if (send(s->fd, MCStringGetCString(d), MCStringGetLength(d), 0) < 0)
		{
			mptr = NULL;
			MCresult->sets("error sending datagram");
		}
		if (mptr != NULL)
		{
			MCscreen->delaymessage(optr, mptr, strclone(s->name));
			s->added = True;
		}
	}
	else
	{
		MCSocketwrite *eptr = new MCSocketwrite(MCStringGetOldString(d), optr, mptr);
		eptr->appendto(s->wevents);
		s->setselect();
		if (mptr == NULL)
		{
			s->waiting = True;
			if (s->connected)
				s->writesome();
			while (True)
			{
				if (s->error != NULL)
				{
					MCresult->sets(s->error);
					break;
				}
				if (s->fd == 0)
				{
					MCresult->sets("socket closed");
					break;
				}
				if (curtime > eptr->timeout)
				{
					MCresult->sets("timeout");
					break;
				}
				if (s->wevents != NULL && eptr == s->wevents
				        && eptr->done == eptr->size)
					break;
				MCU_play();
				if (MCscreen->wait(READ_INTERVAL, False, True))
				{
					MCresult->sets("interrupted");
					break;
				}
			}
			if (s->wevents != NULL)
			{
				eptr->remove
				(s->wevents);
				delete eptr;
			}
			s->waiting = False;
		}
		else
			if (s->connected)
				s->writesome();
	}
}

MCSocket *MCS_accept(uint2 port, MCObject *object, MCNameRef message, Boolean datagram,Boolean secure,Boolean sslverify,char *sslcertfile)
{
	if (!MCS_init_sockets())
		return NULL;

	MCSocketHandle sock = socket(AF_INET, datagram ? SOCK_DGRAM : SOCK_STREAM, 0);
	if (!MCS_valid_socket(sock))
	{
#ifdef _WINDOWS
		MCS_seterrno(WSAGetLastError());
#endif
		MCresult->sets("can't create socket");
		return NULL;
	}

	unsigned long val = 1;

	MCS_socket_ioctl(sock, FIONBIO, val);

	int on = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

	mc_sockaddr_in_t addr;

	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = MCSwapInt32HostToNetwork(INADDR_ANY);
	addr.sin_port = MCSwapInt16HostToNetwork(port);
#ifdef _WINDOWS

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))
	        || (!datagram && listen(sock, SOMAXCONN))
	        || !MCnoui && WSAAsyncSelect(sock, sockethwnd, WM_USER,
	                                     datagram ? FD_READ : FD_ACCEPT))
	{
		MCS_seterrno(WSAGetLastError());
		char buffer[17 + I4L];
		sprintf(buffer, "Error %d on socket", WSAGetLastError());
		MCresult->copysvalue(buffer);
		closesocket(sock);
		return NULL;
	}
#else
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		MCresult->sets("Error binding socket");
		close(sock);
		return NULL;
	}
	if (!datagram)
		listen(sock, SOMAXCONN);
#endif

	char *portname = new char[U2L];
	sprintf(portname, "%d", port);
	return new MCSocket(portname, object, message, datagram, sock, True, False, secure);
}

// Return the IP address of the host interface that is used to connect to the
// internet.
//
// Note that this needs further research.
//
// On Windows, we currently use the first non-loopback, running interface that
// has family AF_INET. Ideally, we would search for the one which has the 
// 'internet gateway' defined - but it isn't clear how one might do that.
//

char *MCS_hostaddress(void)
{
#if defined(_WINDOWS)
	if (!wsainit())
		return NULL;

	int t_socket;
	t_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (t_socket != INVALID_SOCKET)
	{
		INTERFACE_INFO t_interfaces[32];
		DWORD t_result_length;
		if (WSAIoctl(t_socket, SIO_GET_INTERFACE_LIST, NULL, 0, (LPVOID)t_interfaces, sizeof(t_interfaces), &t_result_length, NULL, NULL) != SOCKET_ERROR)
		{
			for(unsigned int i = 0; i < (t_result_length / sizeof(INTERFACE_INFO)); ++i)
			{
				if ((t_interfaces[i] . iiFlags & IFF_UP) != 0 &&
					(t_interfaces[i] . iiFlags & IFF_LOOPBACK) == 0 &&
					t_interfaces[i] . iiAddress . Address . sa_family == AF_INET)
					return strdup(inet_ntoa(t_interfaces[i] . iiAddress . AddressIn . sin_addr));
			}
		}
	}
#elif defined(_MACOSX)
	bool t_success;
	t_success = true;

	SCDynamicStoreRef t_store;
	t_store = NULL;
	if (t_success)
	{
		t_store = SCDynamicStoreCreate(kCFAllocatorDefault, CFSTR("JSEvaluator"), NULL, NULL);
		if (t_store == NULL)
			t_success = false;
	}

	CFStringRef t_network_key;
	t_network_key = NULL;
	if (t_success)
	{
		t_network_key = SCDynamicStoreKeyCreateNetworkGlobalEntity(kCFAllocatorDefault, kSCDynamicStoreDomainState, kSCEntNetIPv4);
		if (t_network_key == NULL)
			t_success = false;
	}

	CFDictionaryRef t_network_value;
	t_network_value = NULL;
	if (t_success)
	{
		t_network_value = (CFDictionaryRef)SCDynamicStoreCopyValue(t_store, t_network_key);
		if (t_network_value == NULL)
			t_success = false;
	}

	CFStringRef t_interface;
	t_interface = NULL;
	if (t_success)
	{
		t_interface = (CFStringRef)CFDictionaryGetValue(t_network_value, kSCDynamicStorePropNetPrimaryInterface);
		if (t_interface == NULL)
			t_success = false;
	}

	CFStringRef t_interface_key;
	t_interface_key = NULL;
	if (t_success)
	{
		t_interface_key = (CFStringRef)SCDynamicStoreKeyCreateNetworkInterfaceEntity(kCFAllocatorDefault, kSCDynamicStoreDomainState, t_interface, kSCEntNetIPv4);
		if (t_interface_key == NULL)
			t_success = false;
	}

	CFDictionaryRef t_interface_value;
	t_interface_value = NULL;
	if (t_success)
	{
		t_interface_value = (CFDictionaryRef)SCDynamicStoreCopyValue(t_store, t_interface_key);
		if (t_interface_value == NULL)
			t_success = false;
	}

	char *t_result;
	t_result = NULL;
	if (t_success)
	{
		CFArrayRef t_addresses;
		t_addresses = (CFArrayRef)CFDictionaryGetValue(t_interface_value, CFSTR("Addresses"));
		if (t_addresses != NULL)
		{
			CFStringRef t_string;
			t_string = (CFStringRef)CFArrayGetValueAtIndex(t_addresses, 0);
			if (t_string != NULL)
				t_result = osx_cfstring_to_cstring(t_string, false);
		}
	}
	
	if (t_interface_value != NULL)
		CFRelease(t_interface_value);

	if (t_interface_key != NULL)
		CFRelease(t_interface_key);

	if (t_network_value != NULL)
		CFRelease(t_network_value);

	if (t_network_key != NULL)
		CFRelease(t_network_key);

	if (t_store != NULL)
		CFRelease(t_store);

	return t_result;

#elif defined(_LINUX)
#else
#endif

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

MCSocketread::MCSocketread(uint4 s, char *u, MCObject *o, MCNameRef m)
{
	size = s;
	until = u;
	timeout = curtime + MCsockettimeout;
	optr = o;
	if (m != nil)
		/* UNCHECKED */ MCNameClone(m, message);
	else
		message = nil;
}

MCSocketread::~MCSocketread()
{
	MCNameDelete(message);
	delete until;
}

MCSocketwrite::MCSocketwrite(const MCString &d, MCObject *o, MCNameRef m)
{
	if (m != NULL)
		buffer = d.clone();
	else
		buffer = (char *)d.getstring();
	size = d.getlength();
	timeout = curtime + MCsockettimeout;
	optr = o;
	done = 0;
	if (m != nil)
		/* UNCHECKED */ MCNameClone(m, message);
	else
		message = nil;
}

MCSocketwrite::~MCSocketwrite()
{
	if (message != NULL)
	{
		MCNameDelete(message);
		delete buffer;
	}
}

////////////////////////////////////////////////////////////////////////////////

MCSocket::MCSocket(char *n, MCObject *o, MCNameRef m, Boolean d, MCSocketHandle sock, Boolean a, Boolean s, Boolean issecure)
{
	name = n;
	object = o;
	if (m != nil)
		/* UNCHECKED */ MCNameClone(m, message);
	else
		message = nil;
	datagram = d;
	accepting = a;
	connected = datagram;
	shared = s;
	fd = sock;
	closing = doread = added = waiting = False;
	revents = NULL;
	wevents = NULL;
	rbuffer = NULL;
	error = NULL;
	rsize = nread = 0;
	timeout = curtime + MCsockettimeout;
	_ssl_context = NULL;
	_ssl_conn = NULL;
	sslstate = SSTATE_NONE; // Not on Mac?
	secure = issecure;
	resolve_state = kMCSocketStateNew;
	init(fd);
}

MCSocket::~MCSocket()
{
	delete name;
	MCNameDelete(message);
	deletereads();
	deletewrites();

	delete rbuffer;
}

void MCSocket::deletereads()
{
	while (revents != NULL)
	{
		MCSocketread *eptr = revents->remove(revents);
		delete eptr;
	}
	nread = 0;
}

void MCSocket::deletewrites()
{
	while (wevents != NULL)
	{
		MCSocketwrite *eptr = wevents->remove(wevents);
		delete eptr;
	}
	if (fd != 0)
	{
		if (!shared)
			close();
		fd = 0;
	}
}

Boolean MCSocket::read_done()
{
	if (revents->until != NULL)
	{
		if (!*revents->until)
		{
			revents->size = nread;
			if (fd == 0)
				MCresult->sets("eof");
			return True;
		}
		if (*revents->until != '\004')
		{
			char *sptr = rbuffer;
			char *eptr = rbuffer + nread;
			char *dptr = revents->until;
			while (sptr < eptr)
			{
				if (*sptr++ == *dptr)
				{
					char *tsptr = sptr;
					char *tdptr = dptr + 1;
					while (tsptr < eptr && *tdptr && *tsptr == *tdptr)
					{
						tsptr++;
						tdptr++;
					}
					if (!*tdptr)
					{
						revents->size = tsptr - rbuffer;
						return True;
					}
				}
			}
		}
	}
	else
	{
		if (nread >= revents->size)
		{
			if (revents->size == 0)
				if (nread == 0)
					return False;
				else
				{
					revents->size = nread;
					if (fd == 0)
						MCresult->sets("eof");
				}
			return True;
		}
	}
	if (fd == 0 && error == NULL)
	{
		revents->size = nread;
		MCresult->sets("eof");
		return True;
	}
	return False;
}

#ifdef _WINDOWS
void MCSocket::acceptone()
{
	struct sockaddr_in addr;
	int addrsize = sizeof(addr);
	SOCKET newfd = accept(fd, (struct sockaddr *)&addr, &addrsize);
	if (newfd > 0)
	{
		char *t = inet_ntoa(addr.sin_addr);
		char *n = new char[strlen(t) + I4L];
		sprintf(n, "%s:%d", t, newfd);
		MCU_realloc((char **)&MCsockets, MCnsockets,
		            MCnsockets + 1, sizeof(MCSocket *));
		MCsockets[MCnsockets] = new MCSocket(n, object, NULL,
		                                     False, newfd, False, False,False);
		MCsockets[MCnsockets]->connected = True;
		MCsockets[MCnsockets++]->setselect();
		MCscreen->delaymessage(object, message, strclone(n), strclone(name));
		added = True;
	}
}
#endif

void MCSocket::readsome()
{
	struct sockaddr_in addr;
	socklen_t addrsize = sizeof(addr);
	if (datagram)
	{
		int l = 0;
		unsigned long t_available;
		MCS_socket_ioctl(fd, FIONREAD, t_available);
		l = t_available;

		char *dbuffer = new char[l + 1]; // don't allocate 0
#ifdef _WINDOWS

		l++; // Not on MacOS/UNIX?
		if ((l = recvfrom(fd, dbuffer, l, 0, (struct sockaddr *)&addr, &addrsize))
		        == SOCKET_ERROR)
		{
			delete dbuffer;
			error = new char[21 + I4L];
			sprintf(error, "Error %d on socket", WSAGetLastError());
			doclose();
		}
#else
		if ((l = recvfrom(fd, dbuffer, l, 0,
						  (struct sockaddr *)&addr, &addrsize)) < 0)
		{
			delete dbuffer;
			if (!doread && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			{
				error = new char[21 + I4L];
				sprintf(error, "Error %d reading socket", errno);
				doclose();
			}
		}
#endif
		else
		{
			if (message == NULL)
				delete dbuffer;
			else
			{
				char *t = inet_ntoa(addr.sin_addr);
				char *n = new char[strlen(t) + U2L];
				sprintf(n, "%s:%d", t, MCSwapInt16NetworkToHost(addr.sin_port));
				uint2 index;
				if (accepting && !IO_findsocket(n, index))
				{
					MCU_realloc((char **)&MCsockets, MCnsockets,
					            MCnsockets + 1, sizeof(MCSocket *));
					MCsockets[MCnsockets++] = new MCSocket(strclone(n), object, NULL,
					                                       True, fd, False, True,False);
				}
				MCParameter *params = new MCParameter;
				params->setbuffer(n, strlen(n));
				params->setnext(new MCParameter);
				params->getnext()->setbuffer(dbuffer, l);
				params->getnext()->setnext(new MCParameter);
				params->getnext()->getnext()->setbuffer(strclone(name), strlen(name));
				MCscreen->addmessage(object, message, curtime, params);
			}
		}
		added = True;
		doread = False;
	}
	else
	{
		if (accepting)
		{
#ifdef _WINDOWS
			acceptone();
			added = True;
#else

			int newfd = accept(fd, (struct sockaddr *)&addr, &addrsize);
			if (newfd > 0)
			{
				int val = 1;
				ioctl(newfd, FIONBIO, (char *)&val);
				char *t = inet_ntoa(addr.sin_addr);
				char *n = new char[strlen(t) + U2L];
				sprintf(n, "%s:%d", t, MCSwapInt16NetworkToHost(addr.sin_port));
				MCU_realloc((char **)&MCsockets, MCnsockets,
							MCnsockets + 1, sizeof(MCSocket *));
				MCsockets[MCnsockets] = new MCSocket(n, object, NULL,
													 False, newfd, False, False,secure);
				MCsockets[MCnsockets]->connected = True;
				if (secure)
					MCsockets[MCnsockets]->sslaccept();
				MCsockets[MCnsockets++]->setselect();
				MCscreen->delaymessage(object, message, strclone(n), strclone(name));
				added = True;
			}
#endif

		}
		else
		{
			if (fd != 0)
			{
				int l = 0;
				if (secure)
					l = READ_SOCKET_SIZE * 16;
				else
				{
					unsigned long t_available;
					MCS_socket_ioctl(fd, FIONREAD, t_available);
					l = t_available;

					if (l == 0) l++; // don't read 0
				}
				uint4 newsize = nread + l;
				if (newsize > rsize)
				{
					newsize += READ_SOCKET_SIZE;
					MCU_realloc((char **)&rbuffer, nread, newsize, sizeof(char));
					if (rbuffer == NULL)
					{
						error = strclone("Out of memory");
						doclose();

						return;
					}
					rsize = newsize;
				}
				errno = 0;
#ifdef _WINDOWS

				if ((l = read(rbuffer + nread, l)) <= 0 || l == SOCKET_ERROR )
				{
					int wsaerr = WSAGetLastError();
					if (!doread && errno != EAGAIN && wsaerr != WSAEWOULDBLOCK && wsaerr != WSAENOTCONN && errno != EINTR)
					{
#else
				if ((l = read(rbuffer + nread, l)) <= 0)
				{
					if (!doread && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
					{
#endif
						if (errno != 0)
						{
							if (secure)
								error = sslgraberror();
							else
							{
								error = new char[21 + I4L];
								sprintf(error, "Error %d reading socket", errno);
							}
						}
						doclose();

						return;
					}
				}
				else
				{
#ifdef _WINDOWS
					if (l == 0)
					{
						doclose();
						return;
					}
#endif
					nread += l;
					if (revents != NULL)
						revents->timeout = curtime + MCsockettimeout;
				}
			}
			doread = False;
			processreadqueue();
		}
	}
}

void MCSocket::processreadqueue()
{
	if (!waiting)
		while (revents != NULL)
		{
			if (read_done())
			{
				uint4 size = revents->size;
				if (size > 1 && revents->until != NULL && *revents->until == '\n'
				        && !*(revents->until + 1) && rbuffer[size - 1] == '\r')
					rbuffer[--size] = '\n';
				char *datacopy = new char[MCU_max((uint4)size, (uint4)1)]; // can't malloc 0
				memcpy(datacopy, rbuffer, size);
				nread -= revents->size;
				// MW-2010-11-19: [[ Bug 9182 ]] This should be a memmove (I think)
				memmove(rbuffer, rbuffer + revents->size, nread);
				MCSocketread *e = revents->remove
				                  (revents);
				MCParameter *params = new MCParameter;
				params->setbuffer(strclone(name), strlen(name));
				params->setnext(new MCParameter);
				params->getnext()->setbuffer(datacopy, size);
				MCscreen->addmessage(e->optr, e->message, curtime, params);
				delete e;
				if (nread == 0 && fd == 0)
					MCscreen->delaymessage(object, MCM_socket_closed, strclone(name));
				added = True;
			}
			else
				break;
		}
}

void MCSocket::writesome()
{
#ifdef _WINDOWS
	if (!connected && message != NULL)
	{
#else
	if (!accepting && !connected && message != NULL)
	{
#endif
		MCscreen->delaymessage(object, message, strclone(name));
		added = True;
		MCNameDelete(message);
		message = NULL;
	}

	connected = True;
	while (wevents != NULL)
	{
		uint4 towrite = wevents->size - wevents->done;
		int4 nwritten = write( wevents->buffer + wevents->done, towrite);
#ifdef _WINDOWS

		if (nwritten == SOCKET_ERROR)
		{
			int wsaerr = WSAGetLastError();
			if (wsaerr != WSAEWOULDBLOCK && wsaerr != WSAENOTCONN && errno != EAGAIN)
#else
		if (nwritten == -1)
		{
			if (errno == EPIPE)
#endif
			{
				if (secure)
					error = sslgraberror();
				else
				{
					error = new char[16 + I4L];
					sprintf(error, "Error %d on socket", errno);
				}
				doclose();
			}
			break;
		}
		else
		{
			wevents->done += nwritten;
			wevents->timeout = curtime + MCsockettimeout;
			if (wevents->done == wevents->size && wevents->message != NULL)
			{
				MCSocketwrite *e = wevents->remove
				                   (wevents);
				MCscreen->delaymessage(e->optr, e->message, strclone(name));
				added = True;
				delete e;
			}
			else
				break;
		}
	}
#ifdef _WINDOWS
	if (closing && wevents == NULL)
#else
	if (closing && (wevents == NULL || errno == EPIPE))
	//secure && sslstate & SSTATE_RETRYREAD)) {
#endif
	{
		waiting = True;
		doclose();
	}
}

void MCSocket::doclose()
{
	deletewrites();

	if (!waiting)
	{
		if (error != NULL)
		{
			MCscreen->delaymessage(object, MCM_socket_error, strclone(name), error);
			added = True;
		}
		else
			if (nread == 0)
			{
				MCscreen->delaymessage(object, MCM_socket_closed, strclone(name));
				added = True;
			}
	}
}


void MCSocket::setselect()
{
	uint2 bioselectstate = 0;
	if (fd)
	{
#ifdef _WINDOWS
		if (connected && !closing && (!shared && revents != NULL || accepting || datagram))
#else

		if (connected && !closing && (!shared && revents != NULL|| accepting))
#endif

			bioselectstate |= BIONB_TESTREAD;
		if (!connected || wevents != NULL)
			bioselectstate |= BIONB_TESTWRITE;
		setselect(bioselectstate);
	}
}

void MCSocket::setselect(uint2 sflags)
{
#ifdef _WINDOWS
	if (!MCnoui)
	{
		long event = FD_CLOSE;
		if (!connected)
			event |= FD_CONNECT;
		if (sflags & BIONB_TESTWRITE)
			event |= FD_WRITE;
		if (sflags & BIONB_TESTREAD)
			event |= FD_READ;
		WSAAsyncSelect(fd, sockethwnd, WM_USER, event);
	}
#endif
#ifdef _MACOSX
	if (sflags & BIONB_TESTWRITE)
		CFSocketEnableCallBacks(cfsockref,kCFSocketWriteCallBack);
	if (sflags & BIONB_TESTREAD)
		CFSocketEnableCallBacks(cfsockref,kCFSocketReadCallBack);
#endif
}

Boolean MCSocket::init(MCSocketHandle newfd)
{
	fd = newfd;
#ifdef _MACOSX

	cfsockref = NULL;
	rlref = NULL;
	cfsockref = CFSocketCreateWithNative (kCFAllocatorDefault,fd, kCFSocketReadCallBack|kCFSocketWriteCallBack,
	                                      (CFSocketCallBack)&socketCallback, NULL);
	if (cfsockref)
	{
		rlref = CFSocketCreateRunLoopSource(kCFAllocatorDefault, cfsockref, 0);
		CFRunLoopAddSource((CFRunLoopRef)GetCFRunLoopFromEventLoop(GetMainEventLoop()), rlref, kCFRunLoopDefaultMode);
		CFOptionFlags socketOptions = 0 ;
		CFSocketSetSocketFlags( cfsockref, socketOptions );
	}
#endif
	return True;
}

void MCSocket::close()
{

	if (fd)
	{
		if (secure)
			sslclose();
#ifdef _MACOSX

		if (rlref != NULL)
		{
			CFRunLoopRemoveSource (CFRunLoopGetCurrent(), rlref, kCFRunLoopDefaultMode);
			CFRelease (rlref);
			rlref = NULL;
		}
#endif
#ifdef _WINDOWS
		closesocket(fd);
#else

		::close(fd);
#endif

		fd = 0;
#ifdef _MACOSX

		if (cfsockref != NULL)
		{
			CFSocketInvalidate (cfsockref);
			CFRelease (cfsockref);
			cfsockref = NULL;
		}
#endif

	}
}

int4 MCSocket::write(const char *buffer, uint4 towrite)
{
	int4 rc = 0;
	if (secure)
	{
		sslstate &= ~SSTATE_RETRYWRITE;
#ifdef _WINDOWS

		if (sslstate & SSTATE_RETRYCONNECT ||
		        sslstate & SSTATE_RETRYREAD)
		{
#else
		if (sslstate & SSLRETRYFLAGS)
		{
#endif
			if (sslstate & SSTATE_RETRYCONNECT)
				if (!sslconnect())
				{
					errno = EPIPE;
					return -1;
				}
#ifndef _WINDOWS
				else if (sslstate & SSTATE_RETRYACCEPT)
					if (!sslaccept())
						return -1;
#endif
			//for write which requires read...if read is available return and wait for write again
			errno =  EAGAIN;
			return -1;
		}
		if (!_ssl_conn)
			return 0;

		rc = SSL_write(_ssl_conn, buffer, towrite);

		if (rc < 0)
		{
			errno = SSL_get_error(_ssl_conn, rc);
			if ((errno != SSL_ERROR_WANT_READ) && (errno != SSL_ERROR_WANT_WRITE))
			{
				errno = EPIPE;
				return rc;
			}
			else
			{
				if (errno == SSL_ERROR_WANT_WRITE)
					setselect(BIONB_TESTWRITE);
				else if (errno == SSL_ERROR_WANT_READ)
					setselect(BIONB_TESTREAD);
				sslstate |=  SSTATE_RETRYWRITE;
				errno =  EAGAIN;
			}
		}
		return rc;
	}
	else
#ifdef _WINDOWS

		return send(fd, buffer, towrite, 0);
#else

		return ::write(fd, buffer, towrite);
#endif
}

int4 MCSocket::read(char *buffer, uint4 toread)
{
	int4 rc = 0;
	if (secure)
	{
		sslstate &= ~SSTATE_RETRYREAD;
#ifdef _WINDOWS

		if (sslstate & SSTATE_RETRYCONNECT || sslstate & SSTATE_RETRYWRITE)
		{
#else
		if (sslstate & SSLRETRYFLAGS)
		{
#endif
			if (sslstate & SSTATE_RETRYCONNECT)
			{
				if (!sslconnect())
					return -1;
			}
#ifndef _WINDOWS
			else  if (sslstate & SSTATE_RETRYACCEPT)
				if (!sslaccept())
					return -1;
#endif
			//for read which requires write return and wait for read again
			errno =  EAGAIN;
			return -1;
		}

		if (!_ssl_conn)
			return 0;
		int4 rc = SSL_read(_ssl_conn, buffer, toread);
		if (rc < 0)
		{
			int err;
			err = SSL_get_error(_ssl_conn, rc);
			errno = err;
			if ((errno != SSL_ERROR_WANT_READ) && (errno != SSL_ERROR_WANT_WRITE))
				return rc;
			else
			{
				if (errno == SSL_ERROR_WANT_WRITE)
					setselect(BIONB_TESTWRITE);
				else if (errno == SSL_ERROR_WANT_READ)
					setselect(BIONB_TESTREAD);
				sslstate |=  SSTATE_RETRYREAD;
				errno =  EAGAIN;
			}
		}
		return rc;
	}
	else
#ifdef _WINDOWS

		return recv(fd, buffer, toread, 0);
#else
		return ::read(fd, buffer, toread);
#endif
}

//

char *MCSocket::sslgraberror()
{
	char *terror = NULL;

	if (!sslinited)
		return strclone("cannot load SSL library");
	if (sslerror)
	{
		terror = sslerror;
		sslerror = NULL;
	}
	else
	{
		unsigned long ecode = 0;
		ecode = ERR_get_error();
		if (ecode != 0)
		{
			terror = new char[256];
			ERR_error_string_n(ecode,terror,255);
		}
	}
	return terror;
}

bool export_system_root_cert_stack(STACK_OF(X509) *&r_x509_stack);
bool export_system_crl_stack(STACK_OF(X509_CRL) *&r_crl_stack);

static STACK_OF(X509) *s_ssl_system_root_certs;
static STACK_OF(X509_CRL) *s_ssl_system_crls;

Boolean MCSocket::sslinit()
{

	if (!sslinited)
	{

		if (!InitSSLCrypt())
			return False;
		SSL_library_init();
		SSL_load_error_strings();

		if (!(export_system_root_cert_stack(s_ssl_system_root_certs) &&
			export_system_crl_stack(s_ssl_system_crls)))
			return False;
		//consider using SSL_load_error_strings() for SSL error strings;
		//			ENGINE_load_builtin_engines();
		sslinited = True;
	}
	return sslinited;
}

bool load_ssl_ctx_certs_from_folder(SSL_CTX *p_ssl_ctx, const char *p_path);
bool load_ssl_ctx_certs_from_file(SSL_CTX *p_ssl_ctx, const char *p_path);

Boolean MCSocket::initsslcontext()
{
	if (!sslinit())
		return False;
	if (_ssl_context)
		return True;
	
	bool t_success = true;
	
	t_success = NULL != (_ssl_context = SSL_CTX_new(SSLv23_method()));
	uint2 ncerts = 0;
	
	if (t_success)
	{
		if (MCsslcertificates && MCCStringLength(MCsslcertificates) > 0)
		{
			MCString *certs = NULL;
	
			MCU_break_string(MCsslcertificates, certs, ncerts);
			if (ncerts)
			{
				uint2 i;
				for (i = 0; i < ncerts; i++)
				{
					MCAutoStringRef t_certpath;
					/* UNCHECKED */ MCStringCreateWithOldString(certs[i].getsvalue(), &t_certpath);

					MCAutoPointer<char> t_utf8_certpath;
					MCAutoStringRef t_resolved_certpath;
                    if (MCS_resolvepath(*t_certpath, &t_resolved_certpath))
						/* UNCHECKED */ MCStringConvertToUTF8String(*t_resolved_certpath, &t_utf8_certpath);
					
					t_success = (MCS_exists(*t_certpath, True) && load_ssl_ctx_certs_from_file(_ssl_context, *t_utf8_certpath)) ||
							(MCS_exists(*t_certpath, False) && load_ssl_ctx_certs_from_folder(_ssl_context, *t_utf8_certpath));
					if (!t_success)
					{
						MCCStringFormat(sslerror, "Error loading CA file and/or directory %s", utf8str);
					}
				}
			}
			if (certs != NULL)
				delete certs;
		}
		else
		{
			if (!ssl_set_default_certificates())
			{
				MCCStringClone("Error loading default CAs", sslerror);
				
				t_success = false;
			}
		}
	}
	
	if (t_success)
	{
		SSL_CTX_set_verify(_ssl_context, sslverify? SSL_VERIFY_PEER: SSL_VERIFY_NONE,verify_callback);
		SSL_CTX_set_verify_depth(_ssl_context, 9);
	}
	return t_success;
}

#if defined(TARGET_PLATFORM_MACOS_X)
bool load_ssl_ctx_certs_from_folder(SSL_CTX *p_ssl_ctx, const char *p_path)
{
	// on OSX, we're still using the provided version of openSSL so this should still work
	return SSL_CTX_load_verify_locations(p_ssl_ctx, NULL, p_path);
}
#else
struct cert_folder_load_context_t
{
	const char *path;
	SSL_CTX *ssl_context;
};

bool cert_dir_list_callback(void *context, const MCFileSystemEntry& entry)
{
	bool t_success = true;
	cert_folder_load_context_t *t_context = (cert_folder_load_context_t*)context;
	
	if (entry.type != kMCFileSystemEntryFolder && MCCStringEndsWith(entry.filename, ".pem"))
	{
		char *t_file_path = nil;
		t_success = MCCStringFormat(t_file_path, "%s/%s", t_context->path, entry.filename);
		if (t_success)
			t_success = load_ssl_ctx_certs_from_file(t_context->ssl_context, t_file_path);
	}
	return t_success;
}

bool load_ssl_ctx_certs_from_folder(SSL_CTX *p_ssl_ctx, const char *p_path)
{
	bool t_success = true;
	
	cert_folder_load_context_t t_context;
	t_context.path = p_path;
	
	t_context.ssl_context = p_ssl_ctx;
	
	t_success = MCFileSystemListEntries(p_path, 0, cert_dir_list_callback, &t_context);
	
	return t_success;
}
#endif

bool load_ssl_ctx_certs_from_file(SSL_CTX *p_ssl_ctx, const char *p_path)
{
	return SSL_CTX_load_verify_locations(p_ssl_ctx, p_path, NULL) != 0;
}

#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_PLATFORM_WINDOWS)

void free_x509_stack(STACK_OF(X509) *p_stack)
{
	if (p_stack != NULL)
	{
		while (sk_X509_num(p_stack) > 0)
		{
			X509 *t_x509 = sk_X509_pop(p_stack);
			X509_free(t_x509);
		}
		sk_X509_free(p_stack);
	}
}

void free_x509_crl_stack(STACK_OF(X509_CRL) *p_stack)
{
	if (p_stack != NULL)
	{
		while (sk_X509_CRL_num(p_stack) > 0)
		{
			X509_CRL *t_crl = sk_X509_CRL_pop(p_stack);
			X509_CRL_free(t_crl);
		}
		sk_X509_CRL_free(p_stack);
	}
}

bool ssl_ctx_add_cert_stack(SSL_CTX *p_ssl_ctx, STACK_OF(X509) *p_cert_stack, STACK_OF(X509_CRL) *p_crl_stack)
{
	bool t_success = true;
	
	X509_STORE *t_cert_store = NULL;
	
	t_success = NULL != (t_cert_store = SSL_CTX_get_cert_store(p_ssl_ctx));
	
	if (t_success && p_cert_stack != NULL)
	{
		for (int32_t i = 0; t_success && i < sk_X509_num(p_cert_stack); i++)
		{
			X509 *t_x509 = sk_X509_value(p_cert_stack, i);
			if (0 == X509_STORE_add_cert(t_cert_store, t_x509))
			{
				if (ERR_GET_REASON(ERR_get_error()) != X509_R_CERT_ALREADY_IN_HASH_TABLE)
					t_success = false;
			}
		}
	}
	
	if (t_success && p_crl_stack != NULL)
	{
		for (int32_t i = 0; t_success && i < sk_X509_CRL_num(p_crl_stack); i++)
		{
			X509_CRL *t_crl = sk_X509_CRL_value(p_crl_stack, i);
			if (0 == X509_STORE_add_crl(t_cert_store, t_crl))
			{
				t_success = false;
			}
		}
	}
	return t_success;
}

bool MCSocket::ssl_set_default_certificates()
{
	return ssl_ctx_add_cert_stack(_ssl_context, s_ssl_system_root_certs, s_ssl_system_crls);
}

#else

static const char *s_ssl_bundle_paths[] = {
	"/etc/ssl/certs/ca-certificates.crt",
	"/etc/pki/tls/certs/ca-bundle.crt",
};

static const char *s_ssl_hash_dir_paths[] = {
	"/etc/ssl/certs",
	"/etc/pki/tls/certs",
};

bool MCSocket::ssl_set_default_certificates()
{
	bool t_success = true;
	bool t_found = false;
	uint32_t t_path_count = 0;
	
	t_path_count = sizeof(s_ssl_bundle_paths) / sizeof(const char*);
	for (uint32_t i = 0; t_success && !t_found && i < t_path_count; i++)
	{
		if (MCS_exists(MCSTR(s_ssl_bundle_paths[i]), true))
		{
			t_success = load_ssl_ctx_certs_from_file(_ssl_context, s_ssl_bundle_paths[i]);
			if (t_success)
				t_found = true;
		}
	}
	
	t_path_count = sizeof(s_ssl_hash_dir_paths) / sizeof(const char*);
	for (uint32_t i = 0; t_success && !t_found && i < t_path_count; i++)
	{
		if (MCS_exists(MCSTR(s_ssl_hash_dir_paths[i]), false))
		{
			t_success = load_ssl_ctx_certs_from_folder(_ssl_context, s_ssl_bundle_paths[i]);
			if (t_success)
				t_found = true;
		}
	}
	
	return t_success;
}

#endif

#ifdef TARGET_PLATFORM_MACOS_X
bool export_system_root_cert_stack(STACK_OF(X509) *&r_x509_stack)
{
	bool t_success = true;
	
	CFArrayRef t_anchors = NULL;
	STACK_OF(X509) *t_stack = NULL;
	
	t_success = noErr == SecTrustCopyAnchorCertificates(&t_anchors);
	
	t_stack = sk_X509_new(NULL);
	if (t_success)
	{
		UInt32 t_anchor_count = CFArrayGetCount(t_anchors);
		for (UInt32 i = 0; t_success && i < t_anchor_count; i++)
		{
			X509 *t_x509 = NULL;
#if (__MAC_OS_X_VERSION_MAX_ALLOWED > 1050)
			const unsigned char* t_data_ptr = NULL;
#else
			unsigned char *t_data_ptr = NULL;
#endif
			UInt32 t_data_len = 0;
			
			CSSM_DATA t_cert_data;
			t_success = noErr == SecCertificateGetData((SecCertificateRef)CFArrayGetValueAtIndex(t_anchors, i), &t_cert_data);
			
			if (t_success)
			{
				t_data_ptr = t_cert_data.Data;
				t_data_len = t_cert_data.Length;
				t_success = NULL != (t_x509 = d2i_X509(NULL, &t_data_ptr, t_data_len));
			}
			if (t_success)
				t_success = 0 != sk_X509_push(t_stack, t_x509);
		}
	}
	
	if (t_anchors != NULL)
		CFRelease(t_anchors);
	
	if (t_success)
		r_x509_stack = t_stack;
	else if (t_stack != NULL)
		free_x509_stack(t_stack);

	return t_success;
}

bool export_system_crl_stack(STACK_OF(X509_CRL) *&r_crls)
{
	r_crls = NULL;
	return true;
}

#elif defined(TARGET_PLATFORM_WINDOWS)

bool export_system_root_cert_stack(STACK_OF(X509) *&r_cert_stack)
{
	bool t_success = true;

	STACK_OF(X509) *t_cert_stack = NULL;
	HCERTSTORE t_cert_store = NULL;
	PCCERT_CONTEXT t_cert_enum = NULL;

	t_success = NULL != (t_cert_stack = sk_X509_new(NULL));

	if (t_success)
		t_success = NULL != (t_cert_store = CertOpenSystemStore(NULL, L"ROOT"));

	while (t_success && NULL != (t_cert_enum = CertEnumCertificatesInStore(t_cert_store, t_cert_enum)))
	{
		bool t_valid = true;
		if (CertVerifyTimeValidity(NULL, t_cert_enum->pCertInfo))
			t_valid = false;
		if (t_valid)
		{
			X509 *t_x509 = NULL;
#if defined(TARGET_PLATFORM_WINDOWS)
			const unsigned char *t_data = (const unsigned char*) t_cert_enum->pbCertEncoded;
#else
			unsigned char *t_data = t_cert_enum->pbCertEncoded;
#endif
			long t_len = t_cert_enum->cbCertEncoded;

			t_success = NULL != (t_x509 = d2i_X509(NULL, &t_data, t_len));

			if (t_success)
				t_success = 0 != sk_X509_push(t_cert_stack, t_x509);
		}
	}

	if (t_cert_store != NULL)
		CertCloseStore(t_cert_store, 0);

	if (t_success)
		r_cert_stack = t_cert_stack;
	else
		free_x509_stack(t_cert_stack);

	return t_success;
}

bool export_system_crl_stack(STACK_OF(X509_CRL) *&r_crls)
{
	bool t_success = true;

	STACK_OF(X509_CRL) *t_crl_stack = NULL;
	HCERTSTORE t_cert_store = NULL;
	PCCRL_CONTEXT t_crl_enum = NULL;

	t_success = NULL != (t_crl_stack = sk_X509_CRL_new(NULL));

	if (t_success)
		t_success = NULL != (t_cert_store = CertOpenSystemStore(NULL, L"ROOT"));

	while (t_success && NULL != (t_crl_enum = CertEnumCRLsInStore(t_cert_store, t_crl_enum)))
	{
		bool t_valid = true;
		if (CertVerifyCRLTimeValidity(NULL, t_crl_enum->pCrlInfo))
			t_valid = false;
		if (t_valid)
		{
			X509_CRL *t_crl = NULL;
#if defined(TARGET_PLATFORM_WINDOWS)
			const unsigned char *t_data = (const unsigned char*)t_crl_enum->pbCrlEncoded;
#else
			unsigned char *t_data = t_crl_enum->pbCrlEncoded;
#endif
			long t_len = t_crl_enum->cbCrlEncoded;

			t_success = NULL != (t_crl = d2i_X509_CRL(NULL, &t_data, t_len));

			if (t_success)
				t_success = 0 != sk_X509_CRL_push(t_crl_stack, t_crl);
		}
	}

	if (t_cert_store != NULL)
		CertCloseStore(t_cert_store, 0);

	if (t_success)
		r_crls = t_crl_stack;
	else
		free_x509_crl_stack(t_crl_stack);

	return t_success;
}

#else

bool export_system_root_cert_stack(STACK_OF(X509) *&r_cert_stack)
{
	r_cert_stack = NULL;
	return true;
}

bool export_system_crl_stack(STACK_OF(X509_CRL) *&r_crls)
{
	r_crls = NULL;
	return true;
}

#endif

Boolean MCSocket::sslconnect()
{
	sslstate &= ~SSTATE_RETRYCONNECT;
	if (!initsslcontext())
		return False;

	// Setup for SSL
	// TODO: verify certs

	if (!_ssl_conn)
	{
		_ssl_conn = SSL_new(_ssl_context);
		SSL_set_connect_state(_ssl_conn);
		SSL_set_fd(_ssl_conn, fd);
	}

	// Start the SSL connection

	// MW-2005-02-17: Implement the post-connection check suggested by the SSL Book.
	//	The implementation takes the hostname from the string used to open the
	//	socket.
	int4 rc = SSL_connect(_ssl_conn);
	if (rc == 1)
	{
		if (sslverify)
		{
			char *t_hostname;
			t_hostname = strdup(name);
			if (strchr(t_hostname, ':') != NULL)
				strchr(t_hostname, ':')[0] = '\0';
			else if (strchr(t_hostname, '|') != NULL)
				strchr(t_hostname, '|')[0] = '\0';

			rc = post_connection_check(_ssl_conn, t_hostname);

			free(t_hostname);

			if (rc != X509_V_OK)
			{
				const char *t_message = X509_verify_cert_error_string(rc);
				sslerror = strdup(t_message);
				errno = EPIPE;
				return False;
			}
		}

		sslstate |= SSTATE_CONNECTED;
		setselect(BIONB_TESTREAD | BIONB_TESTWRITE);
		return True;
	}

	errno = SSL_get_error(_ssl_conn, rc);
	if ((errno != SSL_ERROR_WANT_READ) && (errno != SSL_ERROR_WANT_WRITE))
	{
		return False;
	}
	else
	{
		sslstate |= SSTATE_RETRYCONNECT;

		if (errno == SSL_ERROR_WANT_WRITE)
			setselect(BIONB_TESTWRITE);
		else if (errno == SSL_ERROR_WANT_READ)
			setselect(BIONB_TESTWRITE);

#ifdef _WINDOWS

		setselect(BIONB_TESTREAD | BIONB_TESTWRITE);
#endif

		return True;
	}
}


// MW-2005-02-17: Temporary routines for matching the identity of an SSL
//  certificate.
static bool ssl_match_component(const char *p_pattern_start, const char *p_pattern_end, const char *p_string_start, const char *p_string_end)
{
	const char *t_pattern;
	const char *t_string;

	for(t_pattern = p_pattern_start, t_string = p_string_start; t_pattern != p_pattern_end && t_string != p_string_end && *t_string == *t_pattern; ++t_pattern, ++t_string)
		;

	if (t_string == p_string_end && t_pattern == p_pattern_end)
		return true;

	if (t_pattern == p_pattern_end || *t_pattern != '*')
		return false;

	if (t_string == p_string_end || t_pattern + 1 == p_pattern_end)
		return true;

	do
	{
		if (ssl_match_component(t_pattern + 1, p_pattern_end, t_string, p_string_end))
			return true;
		++t_string;
	}
	while(t_string != p_string_end);

	return false;
}

static bool ssl_match_identity(const char *p_pattern, const char *p_string)
{
	const char *t_next_pattern, *t_next_string;

	do
	{
		t_next_pattern = strchr(p_pattern, '.');
		if (t_next_pattern == NULL)
			t_next_pattern = p_pattern + strlen(p_pattern);
		else
			t_next_pattern += 1;

		t_next_string = strchr(p_string, '.');
		if (t_next_string == NULL)
			t_next_string = p_string + strlen(p_string);
		else
			t_next_string += 1;

		if (!ssl_match_component(p_pattern, t_next_pattern, p_string, t_next_string))
			return false;

		p_pattern = t_next_pattern;
		p_string = t_next_string;
	}
	while(*p_pattern != '\0' && *p_string != '\0');

	return *p_pattern == *p_string;
}

// MW-2005-02-17: Integrated standard SSL post connection check logic.
static long post_connection_check(SSL *ssl, char *host)
{
	X509 *cert;
	X509_NAME	*subj;
	char data[256];
	int ok = 0;

	STACK_OF(GENERAL_NAME) *t_alt_names = NULL;
	int t_idx = -1;

	if (!(cert = SSL_get_peer_certificate(ssl)) || !host)
		goto err_occured;

	while (NULL != (t_alt_names = (STACK_OF(GENERAL_NAME)*)X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, &t_idx)))
	{
		for (int32_t i = 0; i < sk_GENERAL_NAME_num(t_alt_names); i++)
		{
			GENERAL_NAME *t_name = sk_GENERAL_NAME_value(t_alt_names, i);
			if (t_name->type == GEN_DNS && ssl_match_identity((char*)ASN1_STRING_data(t_name->d.ia5), host))
			{
				ok = 1;
				break;
			}
		}
		GENERAL_NAMES_free(t_alt_names);
	}
		
	if (!ok && (subj = X509_get_subject_name(cert)) &&
	        X509_NAME_get_text_by_NID(subj, NID_commonName, data, 256) > 0)
	{
		data[255] = 0;
		if (!ssl_match_identity(data, host))
			goto err_occured;
	}

	X509_free(cert);
	return SSL_get_verify_result(ssl);

err_occured:
	if (cert)
		X509_free(cert);

	return X509_V_ERR_APPLICATION_VERIFICATION;
}

Boolean MCSocket::sslaccept()
{
	if (!initsslcontext())
		return False;

	// Setup for SSL
	// TODO: verify certs

	if (!_ssl_conn)
	{
		_ssl_conn = SSL_new(_ssl_context);
		SSL_set_accept_state(_ssl_conn);
		SSL_set_fd(_ssl_conn, fd);
	}

	// Start the SSL connection

	// MW-2005-02-17: Implement the post-connection check suggested by the SSL Book.
	//	The implementation takes the hostname from the string used to open the
	//	socket.
	sslstate &= ~SSTATE_RETRYACCEPT;
	int4 rc = SSL_accept(_ssl_conn);
	if (rc == 1)
	{
		if (sslverify)
		{
			char *t_hostname;
			t_hostname = strdup(name);
			if (strchr(t_hostname, ':') != NULL)
				strchr(t_hostname, ':')[0] = '\0';
			else if (strchr(t_hostname, '|') != NULL)
				strchr(t_hostname, '|')[0] = '\0';

			rc = post_connection_check(_ssl_conn, t_hostname);

			free(t_hostname);
			if (rc != X509_V_OK)
				return False;
		}

		sslstate |= SSTATE_CONNECTED;
		setselect(BIONB_TESTREAD|BIONB_TESTWRITE);
		return True;
	}

	errno = SSL_get_error(_ssl_conn, rc);
	if ((errno != SSL_ERROR_WANT_READ) && (errno != SSL_ERROR_WANT_WRITE))
	{
		return False;
	}
	else
	{
		sslstate |= SSTATE_RETRYACCEPT;
		if (errno == SSL_ERROR_WANT_WRITE)
			setselect(BIONB_TESTWRITE);
		else if (errno == SSL_ERROR_WANT_READ)
			setselect(BIONB_TESTWRITE);
		return True;
	}
}

void MCSocket::sslclose()
{
	if (_ssl_context)
	{
		if (_ssl_conn)
			if (sslstate & SSTATE_CONNECTED)
				SSL_shutdown(_ssl_conn);
			else
				SSL_clear(_ssl_conn);
		SSL_free(_ssl_conn);
		SSL_CTX_free(_ssl_context);
		_ssl_context = NULL;
		_ssl_conn = NULL;
		sslstate = SSTATE_NONE;
	}
}

static int verify_callback(int ok, X509_STORE_CTX *store)
{
	char data[256];

	if (!ok)
	{
		X509 *cert = X509_STORE_CTX_get_current_cert(store);
		int  depth = X509_STORE_CTX_get_error_depth(store);
		int  err = X509_STORE_CTX_get_error(store);
		sslerror = new char[3000];
		int certlen = strlen(sslerror);
		sprintf(sslerror, "-Error with certificate at depth: %i\n", depth);
		X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
		certlen = strlen(sslerror);
		sprintf(&sslerror[certlen-1], "  issuer   = %s\n", data);
		X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
		certlen = strlen(sslerror);
		sprintf(&sslerror[certlen-1], "  subject  = %s\n", data);
		certlen = strlen(sslerror);
		sprintf(&sslerror[certlen-1], "  err %i:%s\n", err, X509_verify_cert_error_string(err));
	}

	return ok;
}

#endif

////////////////////////////////////////////////////////////////////////////////
