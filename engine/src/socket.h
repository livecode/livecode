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

#ifndef SOCKET_H
#define SOCKET_H

#include "dllst.h"

enum MCSocketType {
    MCSOCK_TCP,
    MCSOCK_DATAGRAM,
    MCSOCK_SSL
};

#define SSTATE_NONE 0
#define SSTATE_CONNECTED    (1UL << 1)
#define SSTATE_RETRYCONNECT    (1UL << 2)
#define SSTATE_RETRYREAD    (1UL << 3)
#define SSTATE_RETRYWRITE    (1UL << 4)
#define SSTATE_FATALERROR  (1UL << 5)
#define SSTATE_RETRYACCEPT  (1UL << 6)

#define SSLRETRYFLAGS (SSTATE_RETRYCONNECT | SSTATE_RETRYREAD | SSTATE_RETRYWRITE | SSTATE_RETRYACCEPT)

class MCSocketread : public MCDLlist
{
public:
	uint4 size;
	char *until;
	real8 timeout;
	MCObject *optr;
	MCNameRef message;
	MCSocketread(uint4 s, char *u, MCObject *o, MCNameRef m);
	~MCSocketread();
	MCSocketread *next()
	{
		return (MCSocketread *)MCDLlist::next();
	}
	MCSocketread *prev()
	{
		return (MCSocketread *)MCDLlist::prev();
	}
	void appendto(MCSocketread *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCSocketread *remove
	(MCSocketread *&list)
	{
		return (MCSocketread *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

class MCSocketwrite : public MCDLlist
{
public:
	real8 timeout;
	MCObject *optr;
	MCNameRef message;
	char *buffer;
	uint4 size;
	uint4 done;
	Boolean writedone;
	
	// MM-2014-02-12: [[ SecureSocket ]] We now store against each individual write if it should be encrypted (rather than checking against socket).
	Boolean secure;
	
	MCSocketwrite(const MCString &d, MCObject *o, MCNameRef m, Boolean secure);
	~MCSocketwrite();
	MCSocketwrite *next()
	{
		return (MCSocketwrite *)MCDLlist::next();
	}
	MCSocketwrite *prev()
	{
		return (MCSocketwrite *)MCDLlist::prev();
	}
	void appendto(MCSocketwrite *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCSocketwrite *remove
	(MCSocketwrite *&list)
	{
		return (MCSocketwrite *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

typedef enum _mcsocketstate
{
	kMCSocketStateNew,
	kMCSocketStateResolving,
	kMCSocketStateConnecting,
	kMCSocketStateClosed,
	kMCSocketStateError,
} MCSocketState ;

class MCSocket
{
public:
	char *name;
	Boolean closing;
	Boolean waiting;
	Boolean datagram;
	Boolean accepting;
	Boolean doread;
	Boolean added;
	Boolean connected;
	Boolean shared;
	MCSocketState resolve_state;
	MCObject *object;
	MCNameRef message;
	MCSocketread *revents;
	MCSocketwrite *wevents;
	char *rbuffer;
	uint4 rsize;
	uint4 nread;
	char *error;
	real8 timeout;
	MCSocketHandle fd;
	
	// MM-2014-06-13: [[ Bug 12567 ]] Added support for specifying an end host name to verify against.
	char *endhostname;
	
	MCSocket(char *n, MCObject *o, MCNameRef m, Boolean d, MCSocketHandle sock, Boolean a, Boolean s, Boolean issecure);
	void setselect();
	void setselect(uint2 sflags);

	void close();
	Boolean init(MCSocketHandle newfd);

	// MM-2014-02-12: [[ SecureSocket ]] Pass if the read/write should be encrypted rather than checking against socket.
	int4 write(const char *buffer, uint4 towrite, Boolean securewrite);
	int4 read(char *buffer, uint4 toread, Boolean secureread);
	
	uint2 sslstate;
	Boolean sslverify;
	void doclose();

#if defined _WINDOWS
	void acceptone();
#endif

	~MCSocket();
	void deletereads();
	void deletewrites();

	Boolean read_done();
	void readsome();
	void writesome();
	void processreadqueue();

	//ssl methods
	//ssl specific

	char *sslgraberror();
	Boolean secure;
	Boolean initsslcontext();
	Boolean sslconnect();
	Boolean sslaccept();
	void sslclose();
protected:
#ifdef _MACOSX
	CFSocketRef cfsockref;
	CFRunLoopSourceRef rlref;
#endif

	static Boolean sslinited;
	static Boolean sslinit();

#ifdef MCSSL
	SSL *_ssl_conn;
	SSL_CTX *_ssl_context;
#endif
};



extern bool MCS_init_sockets();
extern bool MCS_compare_host_domain(const char *p_host_a, const char *p_host_b);
// MM-2014-06-13: [[ Bug 12567 ]] Added passing through the host name to verify against.
extern MCSocket *MCS_open_socket(char *name, Boolean datagram, MCObject *o, MCNameRef m, Boolean secure, Boolean sslverify, char *sslcertfile, char* hostname);
extern void MCS_close_socket(MCSocket *s);
extern void MCS_read_socket(MCSocket *s, MCExecPoint &ep, uint4 length, char *until, MCNameRef m);
extern void MCS_write_socket(const MCString &d, MCSocket *s, MCObject *optr, MCNameRef m);
// MM-2014-02-12: [[ SecureSocket ]] New secure socket command.
// MM-2014-06-13: [[ Bug 12567 ]] Added host name to verify against.
void MCS_secure_socket(MCSocket *s, Boolean sslverify, char *hostname);
extern MCSocket *MCS_accept(uint2 p, MCObject *o, MCNameRef m, Boolean datagram,Boolean secure,Boolean sslverify,char *sslcertfile);
extern void MCS_ha(MCExecPoint &ep, MCSocket *s);
extern void MCS_hn(MCExecPoint &ep);
extern void MCS_aton(MCExecPoint &ep);
extern void MCS_ntoa(MCExecPoint &ep, MCExecPoint &ep2);
extern void MCS_pa(MCExecPoint &ep, MCSocket *s);




typedef bool (*MCHostNameResolveCallback)(void *p_context, bool p_resolved, bool p_final, struct sockaddr *p_addr, int p_addrlen);
typedef void (*MCSockAddrToStringCallback)(void *p_context, bool p_resolved, const char *p_hostname);

bool MCSocketHostNameResolve(const char *p_name, const char *p_port, int p_socktype, bool p_blocking,
							 MCHostNameResolveCallback p_callback, void *p_context);
bool MCSocketAddrToString(struct sockaddr *p_sockaddr, int p_addrlen, bool p_lookup_hostname, bool p_blocking,
						MCSockAddrToStringCallback p_callback, void *p_context);

bool MCS_name_to_sockaddr(const char *p_name_in, struct sockaddr_in *r_addr,
						  MCHostNameResolveCallback p_callback, void *p_context);
bool MCS_name_to_sockaddr(const char *p_name_in, struct sockaddr_in &r_addr);



bool addrinfo_lookup(const char *p_name, const char *p_port, int p_socktype, struct addrinfo *&r_addrinfo);
bool sockaddr_to_string(struct sockaddr *p_addr, int p_addrlen, bool p_lookup_hostname, char *&r_string);

bool MCS_sockaddr_to_string(struct sockaddr *p_addr, int p_addrlen, bool p_lookup_hostname, char *&r_string,
							MCSockAddrToStringCallback p_callback, void *p_context);
bool MCS_sockaddr_to_string(struct sockaddr *p_addr, int p_addrlen, bool p_lookup_hostname, char *&r_string);

#endif // SOCKET_H
