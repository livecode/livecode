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

#include "object.h"
#include "stack.h"
#include "card.h"
#include "mcerror.h"
#include "exec.h"


#include "param.h"
#include "handler.h"
#include "util.h"
#include "globals.h"
#include "osspec.h"

#include "ports.cpp"

#include "notify.h"
#include "socket.h"
#include "system.h"

#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincrypt.h>
#include <iphlpapi.h>
#elif defined(_MAC_DESKTOP)
#include "osxprefix.h"
#include "platform.h"
#include <SystemConfiguration/SCDynamicStore.h>
#include <SystemConfiguration/SCDynamicStoreKey.h>
#include <SystemConfiguration/SCSchemaDefinitions.h>
// SN-2014-12-22: [[ Bug 14278 ]] Parameter added to choose a UTF-8 string.
extern char *osx_cfstring_to_cstring(CFStringRef p_string, bool p_release, bool p_utf8_string);
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef MCSSL

#if !defined(_WINDOWS_DESKTOP) && !defined(_WINDOWS_SERVER)
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netinet/in_systm.h>

// MM-2014-02-04: [[ LibOpenSSL 1.0.1e ]] Header netinet/udp.h is not included in the iOS device SDK.
#if !defined(TARGET_SUBPLATFORM_IPHONE) || defined(__i386__)
#include <netinet/udp.h>
#endif

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

#include "mcssl.h"

#if defined(TARGET_SUBPLATFORM_ANDROID)
#include "mblandroidjava.h"
#endif

#if defined(_MAC_DESKTOP) || defined(_LINUX_DESKTOP) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID)
#define USE_AUX_THREAD
#endif

#if defined(USE_AUX_THREAD)
#include "mcmanagedpthread.h"
#include <signal.h>

static MCManagedPThread s_socket_poll_thread;
static pthread_mutex_t s_socket_list_mutex;
volatile sig_atomic_t s_socket_poll_thread_enabled;
static int s_socket_poll_signal_pipe[2];
#endif

#if !defined(X11) && !defined(_MACOSX) && !defined(TARGET_SUBPLATFORM_IPHONE) && !defined(_LINUX_SERVER) && !defined(_MAC_SERVER) && !defined(TARGET_SUBPLATFORM_ANDROID)
#define socklen_t int
#endif

extern bool MCNetworkGetHostFromSocketId(MCStringRef p_socket, MCStringRef& r_host);

extern real8 curtime;

static MCStringRef sslerror = NULL;
static long post_connection_check(SSL *ssl, char *host);
static int verify_callback(int ok, X509_STORE_CTX *store);

#ifdef _MACOSX
extern bool path2utf(MCStringRef p_path, MCStringRef& r_utf);
#endif

#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
extern Boolean wsainit(void);
extern HWND sockethwnd;
extern "C" char *strdup(const char *);
extern HANDLE g_socket_wakeup;
#endif

#define READ_SOCKET_SIZE  4096

Boolean MCSocket::sslinited = False;

static void MCSocketsLockSocketList(void)
{
#if defined(USE_AUX_THREAD)
    if (s_socket_poll_thread)
        pthread_mutex_lock(&s_socket_list_mutex);
#endif
}

static void MCSocketsUnlockSocketList(void)
{
#if defined(USE_AUX_THREAD)
    if (s_socket_poll_thread)
        pthread_mutex_unlock(&s_socket_list_mutex);
#endif
}

#if defined(USE_AUX_THREAD)
// MM-2015-07-07: [[ MobileSockets ]] Since on Android we can't hook into system
//  calls to monitor sockets, we instead have an auxiliary thread that polls the
//  sockets checking for any activity. If any sockets are pending, a notification
//  is pushed onto the main thread which will complete the read/write.
// MM-2016-01-27: [[ AuxThread ]] Updated to use the auxiliary thread on all platforms other than Windows.

struct MCSocketsHandleFileDescriptorsCallbackContext
{
    fd_set *rmaskfd;
    fd_set *wmaskfd;
    fd_set *emaskfd;
};

static void MCSocketsHandleFileDescriptorsCallback(void *p_context)
{
    struct MCSocketsHandleFileDescriptorsCallbackContext *t_context;
    t_context = (MCSocketsHandleFileDescriptorsCallbackContext *) p_context;
    MCSocketsHandleFileDescriptorSets(*t_context -> rmaskfd, *t_context -> wmaskfd, *t_context -> emaskfd);
}

static void *MCSocketsPoll(void *p_arg)
{
#if defined(TARGET_SUBPLATFORM_ANDROID)
    MCJavaAttachCurrentThread();
#endif
    
    fd_set rmaskfd, wmaskfd, emaskfd;
    int4 maxfd;
    
    while (s_socket_poll_thread_enabled)
    {
        FD_ZERO(&rmaskfd);
        FD_ZERO(&wmaskfd);
        FD_ZERO(&emaskfd);
        maxfd = 0;
        
        // Add signal pipe to the set of file descriptors to make sure we wake up on any external interrupts.
        // Prevents the select call blocking indefinitely.
        FD_SET(s_socket_poll_signal_pipe[0], &rmaskfd);
        maxfd = s_socket_poll_signal_pipe[0];
        
        // As we're running on an auxiliarry thread, lock to make sure the list of sockets
        // is not mutated by the main thread during our parsing.
        MCSocketsLockSocketList();
        MCSocketsAddToFileDescriptorSets(maxfd, rmaskfd, wmaskfd, emaskfd);
        MCSocketsUnlockSocketList();
        
        int n;
        n = select(maxfd + 1, &rmaskfd, &wmaskfd, &emaskfd, NULL);
        if (n > 0)
        {
            if (FD_ISSET(s_socket_poll_signal_pipe[0], &rmaskfd))
            {
                char t_signal_char;
                read(s_socket_poll_signal_pipe[0], &t_signal_char, 1);
            }
            else
            {
                // Make sure the handling of active sockets takes place on the main thread by posting a notification.
                struct MCSocketsHandleFileDescriptorsCallbackContext t_context;
                t_context . rmaskfd = &rmaskfd;
                t_context . wmaskfd = &wmaskfd;
                t_context . emaskfd = &emaskfd;
                MCNotifyPush(MCSocketsHandleFileDescriptorsCallback, &t_context, true, false);
            }
        }
    }
    
#if defined(TARGET_SUBPLATFORM_ANDROID)
    MCJavaDetachCurrentThread();
#endif

    return NULL;
}
#endif

static bool MCSocketsPollInterrupt(void)
{
    bool t_success;
    t_success = true;
    
#if defined(USE_AUX_THREAD)
    if (t_success)
    {
        if (!s_socket_poll_thread)
        {
            if (t_success)
                t_success = pthread_mutex_init(&s_socket_list_mutex, NULL) == 0;
            if (t_success)
                t_success = pipe(s_socket_poll_signal_pipe) == 0;
            if (t_success)
            {
                s_socket_poll_thread_enabled = true;
                t_success = (0 == s_socket_poll_thread.Create(NULL, MCSocketsPoll, NULL));
            }
        }
        else
            t_success = write(s_socket_poll_signal_pipe[1], "1", 1) == 1;
    }
#endif
    
    return t_success;
}

bool MCSocketsInitialize(void)
{
#if defined(USE_AUX_THREAD)
	MCMemoryReinit(s_socket_poll_thread);
	s_socket_poll_thread_enabled = false;
#endif
    return true;
}

void MCSocketsFinalize(void)
{
#if defined(USE_AUX_THREAD)
    if (s_socket_poll_thread)
    {
		s_socket_poll_thread_enabled = false;
        MCSocketsPollInterrupt();
        
        void *t_result;
		s_socket_poll_thread.Join(&t_result);
        
        pthread_mutex_destroy(&s_socket_list_mutex);
    }
#endif
}

void MCSocketsAppendToSocketList(MCSocket *p_socket)
{
    MCSocketsLockSocketList();
    
    MCU_realloc((char **)&MCsockets, MCnsockets, MCnsockets + 1, sizeof(MCSocket *));
    MCsockets[MCnsockets++] = p_socket;
    
    MCSocketsUnlockSocketList();
    MCSocketsPollInterrupt();
}

void MCSocketsRemoveFromSocketList(uint32_t p_socket_no)
{
    MCSocketsLockSocketList();
    
    delete MCsockets[p_socket_no];
    uint32_t i;
    i = p_socket_no;
    while (++i < MCnsockets)
        MCsockets[i - 1] = MCsockets[i];
    MCnsockets--;
    
    MCSocketsUnlockSocketList();
    MCSocketsPollInterrupt();
}

// MM-2015-07-07: [[ MobileSockets ]] Refactored socket polling code that was common accross all the platforms into 2 function calls.
// MCSocketsAddToFileDescriptorSets adds the required socket file decriptors to the given sets.
// MCSocketsHandleFileDescriptorSets deals with any pending sockets in the given file descriptor sets.

bool MCSocketsAddToFileDescriptorSets(int4 &r_maxfd, fd_set &r_rmaskfd, fd_set &r_wmaskfd, fd_set &r_emaskfd)
{
    bool t_handled;
    t_handled = false;
    
    uint2 i;
    for (i = 0 ; i < MCnsockets ; i++)
    {
        int fd = MCsockets[i]->fd;
        if (!fd || MCsockets[i]->resolve_state == kMCSocketStateResolving ||
            MCsockets[i]->resolve_state == kMCSocketStateError)
            continue;
        if ((MCsockets[i]->connected && !MCsockets[i]->closing
             && !MCsockets[i]->shared) || MCsockets[i]->accepting)
            FD_SET(fd, &r_rmaskfd);
        if (!MCsockets[i]->connected || MCsockets[i]->wevents != NULL)
            FD_SET(fd, &r_wmaskfd);
        FD_SET(fd, &r_emaskfd);
        if (fd > r_maxfd)
            r_maxfd = fd;
        if (MCsockets[i]->added)
        {
            MCsockets[i]->added = False;
            t_handled = true;
        }
    }

    return t_handled;
}

void MCSocketsHandleFileDescriptorSets(fd_set &p_rmaskfd, fd_set &p_wmaskfd, fd_set &p_emaskfd)
{
    uint2 i;
    for (i = 0 ; i < MCnsockets ; i++)
    {
        if (FD_ISSET(MCsockets[i]->fd, &p_emaskfd))
        {
            if (!MCsockets[i]->waiting)
            {
                MCsockets[i]->error = strclone("select error");
                MCsockets[i]->doclose();
            }
        }
        else
        {
            /* read first here, otherwise a situation can arise when select indicates
             * read & write on the socket as part of the sslconnect handshaking
             * and so consumed during writesome() leaving no data to read
             */
            if (FD_ISSET(MCsockets[i]->fd, &p_rmaskfd) && !MCsockets[i]->shared)
                MCsockets[i]->readsome();
            if (FD_ISSET(MCsockets[i]->fd, &p_wmaskfd))
                MCsockets[i]->writesome();
        }
    }
}

#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
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

#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
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
	MCObjectHandle target;
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
		MCMemoryDestroy(t_info);
	}
}

bool ntoa_message_callback(void *p_context, bool p_resolved, bool p_final, struct sockaddr *p_addr, int p_addrlen)
{
	MCNToAMessageCallbackInfo *t_info = (MCNToAMessageCallbackInfo*)p_context;
	ntoa_callback(t_info->list, p_resolved, p_final, p_addr, p_addrlen);

	if (p_final)
	{
        if (t_info -> target . IsValid())
        {
            MCAutoStringRef t_string;
            /* UNCHECKED */ MCListCopyAsString(t_info->list, &t_string);
            MCscreen->delaymessage(t_info->target, t_info->message, t_info->name, *t_string);
        }
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
    MCAutoPointer<char> t_host_cstring;
    /* UNCHECKED */ MCStringConvertToCString(*t_host, &t_host_cstring);

	if (MCNameIsEqualToCaseless(p_message, kMCEmptyName))
	{
		t_success = MCSocketHostNameResolve(*t_host_cstring, NULL, SOCK_STREAM, true, ntoa_callback, *t_list);
	}
	else
	{
		MCNToAMessageCallbackInfo *t_info = NULL;
		t_success = MCMemoryCreate(t_info);
		if (t_success)
		{
			t_info->message = MCValueRetain(p_message);
			t_success = MCStringCopy(p_hostname, t_info->name);
		}
		if (t_success)
			t_success = MCListCreateMutable('\n', t_info->list);

		if (t_success)
		{
			t_info->target = p_target->GetHandle();
			t_success = MCSocketHostNameResolve(*t_host_cstring, NULL, SOCK_STREAM, false, ntoa_message_callback, t_info);
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
    
	if (getpeername(s->fd, (sockaddr *)&addr, &addrsize) == 0)
        return MCS_sockaddr_to_string((sockaddr *)&addr, addrsize, false, r_string);
    else
    {
        // Backwards compatibility
        r_string = MCValueRetain(kMCEmptyString);
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_connect_socket(MCSocket *p_socket, struct sockaddr_in *p_addr)
{
	p_socket->resolve_state = kMCSocketStateConnecting;	
	if (p_socket != NULL && p_socket->fd != 0)
	{
		
        // check to see if a local host or port has been passed and if so, attempt to bind
        // if no local host has been passed then, if set, use the network interface
        MCAutoStringRef t_from_host;
        MCAutoNumberRef t_from_port;
        if (!MCValueIsEmpty(*p_socket->from))
        {
            if (!MCS_name_to_host_and_port(MCNameGetString(*p_socket->from), &t_from_host, &t_from_port))
            {
                p_socket->error = strclone("error parsing the local host and port");
                p_socket->doclose();
                return false;
            }
        }
        if (!t_from_host.IsSet() && MCdefaultnetworkinterface != NULL)
        {
            if (!MCStringCreateWithCString(MCdefaultnetworkinterface, &t_from_host))
            {
                p_socket->error = strclone("error parsing the network interface address");
                p_socket->doclose();
                return false;
            }
        }

        if (t_from_host.IsSet() || t_from_port.IsSet())
        {
            // the 0 defaults tell the OS to choose any avaliable host or port
            if (!t_from_host.IsSet())
            {
                if (!MCStringCreateWithCString("0.0.0.0", &t_from_host))
                {
                    p_socket->error = strclone("error setting the default local host");
                    p_socket->doclose();
                    return false;
                }
            }
            else if (!t_from_port.IsSet())
            {
                if (!MCNumberCreateWithUnsignedInteger(0, &t_from_port))
                {
                    p_socket->error = strclone("error setting the default local port");
                    p_socket->doclose();
                    return false;
                }
            }

            struct sockaddr_in t_bind_addr;
            if (!MCS_host_and_port_to_sockaddr(*t_from_host, *t_from_port, t_bind_addr))
            {
                p_socket->error = strclone("can't resolve local host and port");
                p_socket->doclose();
                return false;
            }

            // setting the SO_REUSEADDR option allows the same local address to be used to connect to multiple hosts
            int t_port_reuse = 1;
            if (setsockopt(p_socket->fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&t_port_reuse, sizeof(t_port_reuse)) != 0)
            {
                p_socket->error = strclone("can't use the local port");
                p_socket->doclose();
                return false;
            }

#ifdef SO_REUSEPORT
            // some platforms also require the SO_REUSEPORT option to be set in order to use the same local port for multiple connections
            t_port_reuse = 1;
            if (setsockopt(p_socket->fd, SOL_SOCKET, SO_REUSEPORT, (const char *)&t_port_reuse, sizeof(t_port_reuse)) != 0)
            {
				switch (errno)
				{
					case ENOPROTOOPT:
						// option not supported by OS, ignore error
						break;
					default:
						p_socket->error = strclone("can't use the local port");
						p_socket->doclose();
						return false;
				}
            }
#endif

            if (bind(p_socket->fd, (struct sockaddr *)&t_bind_addr, sizeof(struct sockaddr_in)) != 0)
            {
                p_socket->error = strclone("can't bind to local host and port");
                p_socket->doclose();
                return false;
            }
        }

		p_socket->setselect();

#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

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
	return MCSocketsPollInterrupt();
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

// MM-2014-06-13: [[ Bug 12567 ]] Added support for specifying an end host name to verify against.
MCSocket *MCS_open_socket(MCNameRef name, MCNameRef from, Boolean datagram, MCObject *o, MCNameRef mess, Boolean secure, Boolean sslverify, MCStringRef sslcertfile, MCNameRef hostname)
{
	if (!MCS_init_sockets())
		return NULL;

	struct sockaddr_in t_addr;
	if (mess == NULL)
	{
		if (!MCS_name_to_sockaddr(MCNameGetString(name), t_addr))
			return NULL;
	}

	MCSocketHandle sock = socket(AF_INET, datagram ? SOCK_DGRAM : SOCK_STREAM, 0);

	if (!MCS_valid_socket(sock))
	{
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
		MCS_seterrno(WSAGetLastError());
#endif
		MCresult->sets("can't create socket");
		return NULL;
	}
    
#ifdef SO_NOSIGPIPE
    int set = 1;
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif

	// HH-2017-01-26: [[ Bug 18454 ]] Set the broadcast flagged base on property 'allowDatagramBroadcasts'
	if(datagram)
	{
		int t_broadcast;
		t_broadcast = MCallowdatagrambroadcasts ? 1 : 0;
		setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&t_broadcast, sizeof(t_broadcast));
	}

	unsigned long on = 1;

	// set socket nonblocking
	MCS_socket_ioctl(sock, FIONBIO, on);

	MCSocket *s = NULL;
	s = (MCSocket *)new MCSocket(name, from, o, mess, datagram, sock, False, False,secure);

	if (s != NULL)
	{
		s->setselect();

		if (secure)
		{
			s->sslstate |= SSTATE_RETRYCONNECT;
		}

		s->sslverify = sslverify;
		
		// MM-2014-06-13: [[ Bug 12567 ]] Added support for specifying an end host name to verify against.
		if (s -> sslverify)
			s -> endhostname = MCValueRetain(hostname);

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
			MCOpenSocketCallbackInfo *t_info = nullptr;
			/* UNCHECKED */ MCMemoryNew(t_info);
			t_info->m_socket = s;
			s->resolve_state = kMCSocketStateResolving;
			if (!MCS_name_to_sockaddr(MCNameGetString(s->name), &t_info->m_sockaddr, open_socket_resolve_callback, t_info))
			{
				MCMemoryDelete(t_info);
				s->name = nil;
				delete s;
				s = nil;

				if (MCresult->isclear())
					MCresult->sets("can't resolve hostname");
			}
		}
	}
	return s;
}

void MCS_close_socket(MCSocket *s)
{
	s->deletereads();

#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
	if (s->wevents == NULL)
#else
	if (s->wevents == NULL || (s->secure && s->sslstate & SSLRETRYFLAGS))
#endif
		s->deletewrites();

	s->closing = True;
}

// PM-2015-01-20: [[ Bug 14409 ]] Return nil in case of failure
MCDataRef MCS_read_socket(MCSocket *s, MCExecContext &ctxt, uint4 length, const char *until, MCNameRef mptr)
{
    MCDataRef t_data;
    t_data = nil;
    
	if (s->datagram)
	{
        MCValueAssign(s->message, mptr);
		s->object = ctxt . GetObject();
	}
	else
	{
		MCSocketread *eptr = new (nothrow) MCSocketread(length, until != nil ? strdup(until) : nil, ctxt . GetObject(), mptr);
		eptr->appendto(s->revents);
		s->setselect();
		if (s->accepting)
		{
			MCresult->sets("can't read from this socket");
			return t_data;
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
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
			if (MCnoui)
			{
				s->doread = True;
				SetEvent(g_socket_wakeup);
			}
			else
				PostMessageA(sockethwnd, WM_USER, s->fd, FD_OOB);
#else
			s->doread = True;
#endif
			s->processreadqueue();
			ctxt . SetTheResultToEmpty();
			
		}
		else
		{
			s->waiting = True;

			// SN-2015-05-11: [[ Bug 14466 ]] On a native Windows machine,
			//  the time between the call to WSAAsyncSelect in setselect()
			//  and the call to s->read_done() below might not be enough
			//  to let Windows send the message to MCWindowProc.
            //  If that occurs, with a read until empty, no byte will be
			//  found by read_done, and the read will always return an
			//  an empty value the first time.
			s -> readsome();

			// SN-2015-03-30: [[ Bug 14466 ]] We want a wait to occur
			//  to avoid any tigh script loop such as
			//     repeat forever
			//        read from socket tSocket until empty
			//        if it is not empty then exit repeat
			//     end repeat
			//  to hang: the wait statement, in the following loop, would
			//  never be reached since s->read_done always returns true
			//  when reading until empty.
			bool t_continue;
			t_continue = true;
			if (MCscreen->wait(0.0, False, True))
			{
				MCresult->sets("interrupted");
				t_continue = false;
			}

			while (t_continue)
			{
				if (eptr == s->revents && s->read_done())
				{
					uint4 size = eptr->size;
					if (until != NULL && *until == '\n' && !*(until + 1)
					        && size && s->rbuffer[size - 1] == '\r')
						size--;
					/* UNCHECKED */ MCDataCreateWithBytes((const byte_t *)s->rbuffer, size, t_data);
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
			eptr->remove(s->revents);
			delete eptr;
			s->waiting = False;
            return t_data;
		}
	}
    MCSocketsPollInterrupt();
    
    // SN-2015-01-29: [[ Bug 14409 ]] The function must return something
    return t_data;
}

void MCS_write_socket(const MCStringRef d, MCSocket *s, MCObject *optr, MCNameRef mptr)
{
	if (s->datagram)
	{
        MCAutoStringRefAsCString temp_d;
        /* UNCHECKED */ temp_d.Lock(d);

        if (s->shared)
		{
            MCAutoCustomPointer<char,MCMemoryDeleteArray> t_name_copy;
            /* UNCHECKED */ MCStringConvertToCString(MCNameGetString(s->name),
                                                     &t_name_copy);

			char *portptr = strchr(*t_name_copy, ':');
			*portptr = '\0';
			struct sockaddr_in to;
			memset((char *)&to, 0, sizeof(to));
			to.sin_family = AF_INET;
			uint2 port = atoi(portptr + 1);
			to.sin_port = MCSwapInt16HostToNetwork(port);
			if (!inet_aton(*t_name_copy, (in_addr *)&to.sin_addr.s_addr)
				|| sendto(s->fd, *temp_d, MCStringGetLength(d), 0,
						  (sockaddr *)&to, sizeof(to)) < 0)
			{
				mptr = NULL;
				MCresult->sets("error sending datagram");
			}
		}
		else if (send(s->fd, *temp_d, MCStringGetLength(d), 0) < 0)
		{
			mptr = NULL;
			MCresult->sets("error sending datagram");
		}
		if (mptr != NULL)
		{
			MCscreen->delaymessage(optr, mptr, MCNameGetString(s->name));
			s->added = True;
		}
	}
	else
	{
		// MM-2014-02-12: [[ SecureSocket ]] Store against the write if it should be encrypted.
		//  This way, upon securing a socket, all pending writes will remain unencrypted whilst all new writes will be encrypted.
		MCSocketwrite *eptr = new (nothrow) MCSocketwrite(d, optr, mptr, s->secure);
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
    
    MCSocketsPollInterrupt();
}

MCSocket *MCS_accept(uint2 port, MCObject *object, MCNameRef message, Boolean datagram,Boolean secure,Boolean sslverify, MCStringRef sslcertfile)
{
	if (!MCS_init_sockets())
		return NULL;

	MCSocketHandle sock = socket(AF_INET, datagram ? SOCK_DGRAM : SOCK_STREAM, 0);
	if (!MCS_valid_socket(sock))
	{
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
		MCS_seterrno(WSAGetLastError());
#endif
		MCresult->sets("can't create socket");
		return NULL;
	}
    
#ifdef SO_NOSIGPIPE
    int set = 1;
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif

	unsigned long val = 1;

	MCS_socket_ioctl(sock, FIONBIO, val);

	int on = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

#ifdef SO_REUSEPORT
    on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (const char *)&on, sizeof(on)) != 0)
    {
		switch (errno)
		{
			case ENOPROTOOPT:
				// option not supported by OS, ignore error
				break;
			default:
				MCresult->sets("can't reuse port");
				return NULL;
		}
    }
#endif

	mc_sockaddr_in_t addr;

	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	
	if (MCdefaultnetworkinterface != NULL)
	{
		MCAutoStringRef MCdefaultnetworkinterface_string;
		if (!MCStringCreateWithCString(MCdefaultnetworkinterface, &MCdefaultnetworkinterface_string) ||
			!MCS_name_to_sockaddr(*MCdefaultnetworkinterface_string, addr))
		{
			MCresult->sets("can't resolve network interface");
			return NULL;
		}
	}
	else
		addr.sin_addr.s_addr = MCSwapInt32HostToNetwork(INADDR_ANY);
	
	addr.sin_port = MCSwapInt16HostToNetwork(port);
	
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))
	        || (!datagram && listen(sock, SOMAXCONN))
	        || (!MCnoui && WSAAsyncSelect(sock, sockethwnd, WM_USER, datagram ? FD_READ : FD_ACCEPT))
			|| (MCnoui && WSAEventSelect(sock, g_socket_wakeup, datagram ? FD_READ : FD_ACCEPT)))
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
    
    if (0 == port)
    {
        socklen_t addrsize = sizeof(addr);
        if (getsockname(sock, (sockaddr *)&addr, &addrsize) < 0)
        {
            MCresult->sets("can't get bound port");
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
            closesocket(sock);
#else
            close(sock);
#endif
            return NULL;
        }
        
        port =  MCSwapInt16NetworkToHost(((struct sockaddr_in *)&addr)->sin_port);
    }
    
    // AL-2015-01-05: [[ Bug 14287 ]] Create name using the number of chars written to the string.
    uindex_t t_length;
    /* UNCHECKED */ MCAutoPointer<char_t[]> t_port_chars = new (nothrow) char_t[U2L];
    t_length = sprintf((char *)(*t_port_chars), "%d", port);

	MCNewAutoNameRef t_portname;
	if (!MCNameCreateWithNativeChars(*t_port_chars, t_length, &t_portname))
        return nil;
    
	return new MCSocket(*t_portname, NULL, object, message, datagram, sock, True, False, secure);
}

// MM-2014-02-12: [[ SecureSocket ]] New secure socket command. If socket is not already secure, flag as secure to ensure future communications are encrypted.
// MM-2014-06-13: [[ Bug 12567 ]] Added support for passing in host name to verify against.
void MCS_secure_socket(MCSocket *s, Boolean sslverify, MCNameRef end_hostname)
{
	if (!s -> secure)
	{
		s -> secure = True;
		s -> sslverify = sslverify;
		s -> sslstate |= SSTATE_RETRYCONNECT;
		
		if (s -> sslverify)
			s -> endhostname = MCValueRetain(end_hostname);
	}
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

bool MCS_hostaddress(MCStringRef &r_host_address)
{
#if defined(_MAC_DESKTOP)
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
                t_result = osx_cfstring_to_cstring(t_string, false, false);
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
    
    return MCStringCreateWithCString(t_result, r_host_address);
 
#else
    
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
					return MCStringCreateWithCString(inet_ntoa(t_interfaces[i] . iiAddress . AddressIn . sin_addr), r_host_address);
			}
		}
	}

#endif

	r_host_address = MCValueRetain(kMCEmptyString);
    return true;
    
#endif
}

////////////////////////////////////////////////////////////////////////////////

MCSocketread::MCSocketread(uint4 s, char *u, MCObject *o, MCNameRef m)
{
	size = s;
	until = u;
	timeout = curtime + MCsockettimeout;
	optr = o;
	if (m != nil)
        message = MCValueRetain(m);
    else
		message = nil;
}

MCSocketread::~MCSocketread()
{
	MCValueRelease(message);
	delete until;
}

MCSocketwrite::MCSocketwrite(MCStringRef d, MCObject *o, MCNameRef m, Boolean securewrite)
{
    char *temp_d;
    /* UNCHECKED */ MCStringConvertToCString(d, temp_d);
	
    buffer = temp_d;
	size = MCStringGetLength(d);
	timeout = curtime + MCsockettimeout;
	secure = securewrite;
	optr = o;
	done = 0;
	if (m != nil)
        message = MCValueRetain(m);
	else
		message = nil;
}

MCSocketwrite::~MCSocketwrite()
{
	if (message != NULL)
	{
		MCValueRelease(message);
		delete buffer;
	}
}

////////////////////////////////////////////////////////////////////////////////

MCSocket::MCSocket(MCNameRef n, MCNameRef f, MCObject *o, MCNameRef m, Boolean d, MCSocketHandle sock, Boolean a, Boolean s, Boolean issecure)
    : from(f)
{
	name = MCValueRetain(n);
	object = o;
	if (m != nil)
        message = MCValueRetain(m);
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
	
	// MM-2014-06-13: [[ Bug 12567 ]] Added support for specifying an end host name to verify against.
	endhostname = MCValueRetain(kMCEmptyName);
}

MCSocket::~MCSocket()
{
	MCValueRelease(name);
	MCValueRelease(message);
	deletereads();
	deletewrites();

	delete rbuffer;
    
    delete[] error;
	
	// MM-2014-06-13: [[ Bug 12567 ]] Added support for specifying an end host name to verify against.
	MCValueRelease(endhostname);
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
			{
				if (nread == 0)
					return False;
				else
				{
					revents->size = nread;
					if (fd == 0)
						MCresult->sets("eof");
				}
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

#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
void MCSocket::acceptone()
{
	struct sockaddr_in addr;
	int addrsize = sizeof(addr);
	SOCKET newfd = accept(fd, (struct sockaddr *)&addr, &addrsize);
	if (newfd > 0 && object . IsValid())
	{
		char *t = inet_ntoa(addr.sin_addr);
		MCAutoStringRef n;
		MCStringFormat(&n, "%s:%d", t, newfd);
		MCNameRef t_name;
		MCNameCreate(*n, t_name);
        MCSocket *t_socket;
        t_socket = new (nothrow) MCSocket(t_name, NULL, object, NULL, False, newfd, False, False,False);
        if (t_socket != NULL)
        {
            MCSocketsAppendToSocketList(t_socket);
            t_socket -> connected = True;
            t_socket -> setselect();
        }
		MCscreen->delaymessage(object, message, *n, MCNameGetString(name));
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

		char *dbuffer = new (nothrow) char[l + 1]; // don't allocate 0
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

		l++; // Not on MacOS/UNIX?
		if ((l = recvfrom(fd, dbuffer, l, 0, (struct sockaddr *)&addr, &addrsize))
		        == SOCKET_ERROR)
		{
			delete[] dbuffer;
			error = new (nothrow) char[21 + I4L];
			sprintf(error, "Error %d on socket", WSAGetLastError());
			doclose();
		}
#else
		if ((l = recvfrom(fd, dbuffer, l, 0,
						  (struct sockaddr *)&addr, &addrsize)) < 0)
		{
			delete[] dbuffer;
			if (!doread && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			{
				error = new (nothrow) char[21 + I4L];
				sprintf(error, "Error %d reading socket", errno);
				doclose();
			}
		}
#endif
		else
		{
			if (message != NULL && object . IsValid())
			{
				char *t = inet_ntoa(addr.sin_addr);
				MCAutoStringRef n;
				MCNewAutoNameRef t_name;
				/* UNCHECKED */ MCStringCreateMutable(strlen(t) + U2L, &n);
				/* UNCHECKED */ MCStringAppendFormat(*n, "%s:%d", t, MCSwapInt16NetworkToHost(addr.sin_port));
				/* UNCHECKED */ MCNameCreate(*n, &t_name);
				uindex_t index;
				if (accepting && !IO_findsocket(*t_name, index))
				{
                    MCSocket *t_socket;
                    t_socket = new (nothrow) MCSocket(*t_name, NULL, object, NULL, True, fd, False, True,False);
                    if (t_socket != NULL)
                        MCSocketsAppendToSocketList(t_socket);
				}
				
				MCAutoDataRef t_data;
				if (MCDataCreateWithBytes((const byte_t *)dbuffer, l, &t_data))
				{
					MCParameter *params = new (nothrow) MCParameter;
					params->setvalueref_argument(*t_name);
					params->setnext(new (nothrow) MCParameter);
					params->getnext()->setvalueref_argument(*t_data);
					params->getnext()->setnext(new (nothrow) MCParameter);
					params->getnext()->getnext()->setvalueref_argument(name);
					MCscreen->addmessage(object, message, curtime, params);
				}
			}
			delete[] dbuffer;
		}
		added = True;
		doread = False;
	}
	else
	{
		if (accepting)
		{
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
			acceptone();
			added = True;
#else

			int newfd = accept(fd, (struct sockaddr *)&addr, &addrsize);
			if (newfd > 0 && object . IsValid())
			{
				int val = 1;
				ioctl(newfd, FIONBIO, (char *)&val);
				char *t = inet_ntoa(addr.sin_addr);
				MCAutoStringRef n;
				MCNewAutoNameRef t_name;
                // SN-2014-05-08 [[ Bug 12407 ]] 'Garbage' with read from socket
                // Was creating a string with the data length, and then appending instead of putting data inside
				/* UNCHECKED */ MCStringFormat(&n, "%s:%d", t, MCSwapInt16NetworkToHost(addr.sin_port));
				/* UNCHECKED */ MCNameCreate(*n, &t_name);
                MCSocket *t_socket;
                t_socket = new (nothrow) MCSocket(*t_name, NULL, object, NULL, False, newfd, False, False,secure);
                if (t_socket != NULL)
                {
                    MCSocketsAppendToSocketList(t_socket);
                    t_socket -> connected = True;
                    if (secure)
                        t_socket -> sslaccept();
                    t_socket -> setselect();
                }
				MCscreen->delaymessage(object, message, MCNameGetString(*t_name), MCNameGetString(name));
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
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

				// MM-2014-02-12: [[ SecureSocket ]] If a scoket is secured, all data read should be assumed to be encrypted.
				if ((l = read(rbuffer + nread, l, secure)) <= 0 || l == SOCKET_ERROR )
				{
					int wsaerr = WSAGetLastError();
					if (!doread && errno != EAGAIN && wsaerr != WSAEWOULDBLOCK && wsaerr != WSAENOTCONN && errno != EINTR)
					{
#else
				// MM-2014-02-12: [[ SecureSocket ]] If a scoket is secured, all data read should be assumed to be encrypted.
				if ((l = read(rbuffer + nread, l, secure)) <= 0)
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
								error = new (nothrow) char[21 + I4L];
								sprintf(error, "Error %d reading socket", errno);
							}
						}
						doclose();

						return;
					}
				}
				else
				{
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
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

				MCAutoDataRef t_data;
				/* UNCHECKED */ MCDataCreateWithBytes((const byte_t *)rbuffer, size, &t_data);
				
				nread -= revents->size;
				// MW-2010-11-19: [[ Bug 9182 ]] This should be a memmove (I think)
				memmove(rbuffer, rbuffer + revents->size, nread);
				MCSocketread *e = revents->remove
				                  (revents);
                if (e -> optr . IsValid())
                {
                    MCParameter *params = new (nothrow) MCParameter;
                    params->setvalueref_argument(name);
                    params->setnext(new MCParameter);
                    params->getnext()->setvalueref_argument(*t_data);
                    MCscreen->addmessage(e->optr, e->message, curtime, params);
                }
				delete e;
				if (nread == 0 && fd == 0 && object . IsValid())
					MCscreen->delaymessage(object, MCM_socket_closed, MCNameGetString(name));
				added = True;
			}
			else
				break;
		}
}

void MCSocket::writesome()
{
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
	if (!connected && message != NULL && object . IsValid())
	{
#else
	if (!accepting && !connected && message != NULL && object . IsValid())
	{
#endif
		MCscreen->delaymessage(object, message, MCNameGetString(name));
		added = True;
		MCValueRelease(message);
		message = NULL;
	}

	connected = True;
	while (wevents != NULL)
	{
		uint4 towrite = wevents->size - wevents->done;

		// MM-2014-02-12: [[ SecureSocket ]] The write should only be encrypted if the write object has been flagged as secured.
		//  (Was previously using the secure flag stored against the socket).
		int4 nwritten = write( wevents->buffer + wevents->done, towrite, wevents->secure);
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

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
					error = new (nothrow) char[16 + I4L];
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
                if (e -> optr . IsValid())
                    MCscreen->delaymessage(e->optr, e->message, MCNameGetString(name));
				
                added = True;
				delete e;
			}
			else
				break;
		}
	}
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
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
            if (object . IsValid())
            {
                MCAutoStringRef t_error;
                /* UNCHECKED */ MCStringCreateWithCString(error, &t_error);
                MCscreen->delaymessage(object, MCM_socket_error, MCNameGetString(name), *t_error);
            }
			added = True;
		}
		else
			if (nread == 0)
			{
				if (object . IsValid())
                    MCscreen->delaymessage(object, MCM_socket_closed, MCNameGetString(name));
				added = True;
			}
	}
}


void MCSocket::setselect()
{
	uint2 bioselectstate = 0;
	if (fd)
	{
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
		if (connected && !closing && ((!shared && revents != NULL) || accepting || datagram))
#else

		if (connected && !closing && ((!shared && revents != NULL) || accepting))
#endif

			bioselectstate |= BIONB_TESTREAD;
		if (!connected || wevents != NULL)
			bioselectstate |= BIONB_TESTWRITE;
		setselect(bioselectstate);
	}
}

void MCSocket::setselect(uint2 sflags)
{
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
	long event = FD_CLOSE;
	if (!connected)
		event |= FD_CONNECT;
	if (sflags & BIONB_TESTWRITE)
		event |= FD_WRITE;
	if (sflags & BIONB_TESTREAD)
		event |= FD_READ;

	if (!MCnoui)
	{
		WSAAsyncSelect(fd, sockethwnd, WM_USER, event);
	}
	else
	{
		WSAEventSelect(fd, g_socket_wakeup, event);
	}
#endif
}

Boolean MCSocket::init(MCSocketHandle newfd)
{
	fd = newfd;
	return True;
}

void MCSocket::close()
{

	if (fd)
	{
		if (secure)
			sslclose();
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
		closesocket(fd);
#else

		::close(fd);
#endif

		fd = 0;
	}
}

// MM-2014-02-12: [[ SecureSocket ]] Updated to pass in if this write should be encrypted, rather than checking against socket.
int4 MCSocket::write(const char *buffer, uint4 towrite, Boolean securewrite)
{
	int4 rc = 0;
	if (securewrite)
	{
		sslstate &= ~SSTATE_RETRYWRITE;
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

		if (sslstate & SSTATE_RETRYCONNECT ||
		        sslstate & SSTATE_RETRYREAD)
		{
#else
		if (sslstate & SSLRETRYFLAGS)
		{
#endif
			if (sslstate & SSTATE_RETRYCONNECT)
			{
				if (!sslconnect())
				{
					errno = EPIPE;
					return -1;
				}
#ifndef _WINDOWS
				else if (sslstate & SSTATE_RETRYACCEPT)
				{
					if (!sslaccept())
						return -1;
				}
#endif
			}
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
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

		return send(fd, buffer, towrite, 0);
#else

		return ::write(fd, buffer, towrite);
#endif
}

// MM-2014-02-12: [[ SecureSocket ]] Updated to pass in if this read is encrypted, rather than checking against socket.
int4 MCSocket::read(char *buffer, uint4 toread, Boolean secureread)
{
	if (secureread)
	{
		sslstate &= ~SSTATE_RETRYREAD;
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

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
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

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
	if (sslerror != nil)
	{
		char *t_sslerror;
        /* UNCHECKED */ MCStringConvertToCString(sslerror, t_sslerror);
        terror = t_sslerror;
		sslerror = NULL;
	}
	else
	{
		unsigned long ecode = 0;
		ecode = ERR_get_error();
		if (ecode != 0)
		{
			terror = new (nothrow) char[256];
			ERR_error_string_n(ecode,terror,255);
		}
	}
	return terror;
}

Boolean MCSocket::sslinit()
{

	if (!sslinited)
	{

		if (!InitSSLCrypt())
			return False;

		sslinited = True;
	}
	return sslinited;
}

Boolean MCSocket::initsslcontext()
{
	if (!sslinit())
		return False;
	if (_ssl_context)
		return True;
	
	bool t_success = true;
	
	t_success = NULL != (_ssl_context = SSL_CTX_new(TLS_method()));
	
	if (t_success)
		t_success = MCSSLContextLoadCertificates(_ssl_context, &sslerror);
	
	if (t_success)
	{
#if defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID)
        // MM-2015-06-04: [[ MobileSockets ]] Since iOS and Android don't expose the root certificates directly, we can't use OpenSSL's verification routines.
        //   Instead we'll do it ourselves using the OS APIs.
        SSL_CTX_set_verify(_ssl_context, SSL_VERIFY_NONE, NULL);
#else
        SSL_CTX_set_verify(_ssl_context, sslverify? SSL_VERIFY_PEER: SSL_VERIFY_NONE,verify_callback);
        SSL_CTX_set_verify_depth(_ssl_context, 9);
#endif
	}
	return t_success;
}

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

    MCAutoStringRef t_hostname;
    // MM-2014-06-13: [[ Bug 12567 ]] If an end host has been specified, verify against that.
    //   Otherwise, use the socket name as before.
    if (!MCNameIsEmpty(endhostname))
        /* UNCHECKED */ MCStringMutableCopy(MCNameGetString(endhostname), &t_hostname);
    else
        /* UNCHECKED */ MCStringMutableCopy(MCNameGetString(name), &t_hostname);

    uindex_t t_pos;
    if (MCStringFirstIndexOfChar(*t_hostname, ':', 0, kMCCompareExact, t_pos))
        /* UNCHECKED */ MCStringRemove(*t_hostname, MCRangeMakeMinMax(t_pos, MCStringGetLength(*t_hostname)));
    else if (MCStringFirstIndexOfChar(*t_hostname, '|', 0, kMCCompareExact, t_pos))
        /* UNCHECKED */ MCStringRemove(*t_hostname, MCRangeMakeMinMax(t_pos, MCStringGetLength(*t_hostname)));

    MCAutoPointer<char> t_host;
    /* UNCHECKED */ MCStringConvertToCString(*t_hostname, &t_host);

    // Let the SSL lib know the host we are trying to connect to, ensuring any SNI servers
    // send the correct certificate during the handshake
    SSL_set_tlsext_host_name(_ssl_conn, *t_host);
    
	// Start the SSL connection

	// MW-2005-02-17: Implement the post-connection check suggested by the SSL Book.
	//	The implementation takes the hostname from the string used to open the
	//	socket.
	int4 rc = SSL_connect(_ssl_conn);
	if (rc == 1)
	{
		if (sslverify)
		{
#if defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID)
            // MM-2015-06-04: [[ MobileSockets ]] On the mobile platforms we verify the certificate ourselves rather than using OpenSSL
            if (!MCSSLVerifyCertificate(_ssl_conn, *t_hostname, sslerror))
            {
                errno = EPIPE;
                return False;
            }
#else
            rc = post_connection_check(_ssl_conn, *t_host);
            
            if (rc != X509_V_OK)
            {
                MCAutoStringRef t_message;
                /* UNCHECKED */ MCStringCreateWithCString(X509_verify_cert_error_string(rc), &t_message);
                sslerror = MCValueRetain(*t_message);
                errno = EPIPE;
                return False;
            }
#endif
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

#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

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
		goto err_occurred;

	while (NULL != (t_alt_names = (STACK_OF(GENERAL_NAME)*)X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, &t_idx)))
	{
		for (int32_t i = 0; i < sk_GENERAL_NAME_num(t_alt_names); i++)
		{
			GENERAL_NAME *t_name = sk_GENERAL_NAME_value(t_alt_names, i);
			if (t_name->type == GEN_DNS && ssl_match_identity((char*)ASN1_STRING_get0_data(t_name->d.ia5), host))
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
			goto err_occurred;
	}

	X509_free(cert);
	return SSL_get_verify_result(ssl);

err_occurred:
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
			MCAutoStringRef t_hostname;
            /* UNCHECKED */ MCStringMutableCopy(MCNameGetString(name), &t_hostname);
            uindex_t t_pos;
            if (MCStringFirstIndexOfChar(*t_hostname, ':', 0, kMCCompareExact, t_pos))
                /* UNCHECKED */ MCStringRemove(*t_hostname, MCRangeMakeMinMax(t_pos, MCStringGetLength(*t_hostname)));
            else if (MCStringFirstIndexOfChar(*t_hostname, '|', 0, kMCCompareExact, t_pos))
                /* UNCHECKED */ MCStringRemove(*t_hostname, MCRangeMakeMinMax(t_pos, MCStringGetLength(*t_hostname)));
			
#if defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID)
            // MM-2015-06-04: [[ MobileSockets ]] On the mobile platforms we verify the certificate ourselves rather than using OpenSSL
            if (!MCSSLVerifyCertificate(_ssl_conn, *t_hostname, sslerror))
            {
                errno = EPIPE;
                return False;
            }
#else
            MCAutoPointer<char> t_host;
            /* UNCHECKED */ MCStringConvertToCString(*t_hostname, &t_host);
			rc = post_connection_check(_ssl_conn, *t_host);

			if (rc != X509_V_OK)
				return False;
#endif
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
		{
			if (sslstate & SSTATE_CONNECTED)
				SSL_shutdown(_ssl_conn);
			else
				SSL_clear(_ssl_conn);
		}
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
		
		/* UNCHECKED */ MCStringCreateMutable(0, sslerror);
		/* UNCHECKED */ MCStringAppendFormat(sslerror, "-Error with certificate at depth: %i\n", depth);
		X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
		
		/* UNCHECKED */ MCStringAppendFormat(sslerror, "  issuer   = %s\n", data);
		X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
		
		/* UNCHECKED */ MCStringAppendFormat(sslerror, "  subject  = %s\n", data);
		/* UNCHECKED */ MCStringAppendFormat(sslerror, "  err %i:%s\n", err, X509_verify_cert_error_string(err));
		
		/* UNCHECKED */ MCStringCopyAndRelease(sslerror, sslerror);
	}

	return ok;
}

#endif

////////////////////////////////////////////////////////////////////////////////
