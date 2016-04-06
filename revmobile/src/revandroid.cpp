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

#include <external.h>

#if defined(_MACOSX) || defined(_LINUX)
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <stdarg.h>
#include <unistd.h>
#elif defined(_WINDOWS)
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <stdarg.h>
#include <windows.h>
#include <winsock.h>
#include <process.h>
#endif

////////////////////////////////////////////////////////////////////////////////

#if defined(_MACOSX) || defined(_LINUX)

#include <pthread.h>

typedef pthread_mutex_t sys_lock_t;
typedef pthread_t sys_thread_t;

void sys_start_thread(sys_thread_t *p_thread, void *(*start_routine)(void *))
{
	pthread_create(p_thread, nil, start_routine, nil);
}

void sys_finish_thread(sys_thread_t p_thread)
{
	void *t_result;
	pthread_join(p_thread, &t_result);
}

void sys_init_lock(sys_lock_t *p_lock)
{
	pthread_mutex_init(p_lock, nil);
}

void sys_enter_lock(sys_lock_t *p_lock)
{
	pthread_mutex_lock(p_lock);
}

void sys_leave_lock(sys_lock_t *p_lock)
{
	pthread_mutex_unlock(p_lock);
}

void sys_execute(const char *command, const char *p_argument)
{
	/*char *t_argv[16];
	int t_argc;
	t_argc = 0;
	t_argv[t_argc++] = (char *)command;
	
	va_list t_args;
	va_start(t_args, command);
	for(;;)
	{
		t_argv[t_argc++] = va_arg(t_args, char *);
		if (t_argv[t_argc - 1] == nil || t_argc == 15)
			break;
	}
	va_end(t_args);*/

	char *t_argv[3];
	t_argv[0] = (char *)command;
	t_argv[1] = (char *)p_argument;
	t_argv[2] = NULL;
	
	pid_t t_pid;
	t_pid = fork();
	if (t_pid == 0)
	{
		for(int i = 3; i < getdtablesize(); i++)
			close(i);
		
		execv(t_argv[0], t_argv);
		_exit(-1);
	}
	
	int t_stat;
	waitpid(t_pid, &t_stat, 0);
}

void sys_sleep(int millisecs)
{
	usleep(millisecs * 1000);
}

int sys_write( int  fd, const char*  buf, int  len )
{
    int  result = 0;
    while (len > 0)
	{
        int  len2 = write(fd, buf, len);
        if (len2 < 0)
		{
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return -1;
        }
        result += len2;
        len -= len2;
        buf += len2;
    }
    return  result;
}

int sys_read( int  fd, char*  buf, int  len )
{
    int  result = 0;
    while (len > 0)
	{
        int  len2 = read(fd, buf, len);
        if (len2 < 0)
		{
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return -1;
        }
		else if (len2 == 0)
			return -1;
        result += len2;
        len -= len2;
        buf += len2;
    }
    return  result;
}

void sys_close(int fd)
{
	close(fd);
}

#elif defined(_WINDOWS)

typedef CRITICAL_SECTION sys_lock_t;
typedef HANDLE sys_thread_t;

void sys_init_lock(sys_lock_t *p_lock)
{
	InitializeCriticalSection(p_lock);
}

void sys_enter_lock(sys_lock_t *p_lock)
{
	EnterCriticalSection(p_lock);
}

void sys_leave_lock(sys_lock_t *p_lock)
{
	LeaveCriticalSection(p_lock);
}

void sys_execute(const char *p_command, const char *p_argument)
{
	char t_command_line[1024];
	sprintf(t_command_line, "\"%s\" %s", p_command, p_argument);

	STARTUPINFOA t_startup_info;
	memset(&t_startup_info, 0, sizeof(STARTUPINFOA));
	t_startup_info . cb = sizeof(STARTUPINFOA);
	t_startup_info . dwFlags = STARTF_USESHOWWINDOW;
	t_startup_info . wShowWindow = SW_HIDE;

	PROCESS_INFORMATION t_process_info;
	if (CreateProcessA(p_command, t_command_line, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &t_startup_info, &t_process_info) != 0)
	{
		CloseHandle(t_process_info . hThread);
		WaitForSingleObject(t_process_info . hProcess, INFINITE);
		CloseHandle(t_process_info . hProcess);
	}
}

void sys_sleep(int p_millisecs)
{
	Sleep(p_millisecs);
}

static unsigned __stdcall sys_start_thread_shim(void *routine)
{
	void *t_result;
	t_result = ((void *(*)(void *))routine)(NULL);
	return (unsigned)t_result;
}

void sys_start_thread(sys_thread_t *p_thread, void *(*start_routine)(void *))
{
	*p_thread = (sys_thread_t)_beginthreadex(NULL, 0, sys_start_thread_shim, start_routine, 0, NULL);
}

void sys_finish_thread(sys_thread_t p_thread)
{
	WaitForSingleObject(p_thread, INFINITE);
	CloseHandle(p_thread);
}

int sys_write(int fd, const char *buf, int  len)
{
    int result = 0;
    while (len > 0)
	{
        int len2 = send(fd, buf, len, 0);
        if (len2 < 0)
		{
			int t_error;
			t_error = WSAGetLastError();
            if (t_error == WSAEINTR || t_error == WSATRY_AGAIN)
                continue;
            return -1;
        }
        result += len2;
        len -= len2;
        buf += len2;
    }
    return  result;
}

int sys_read(int fd, char *buf, int len)
{
    int  result = 0;
    while (len > 0)
	{
        int len2 = recv(fd, buf, len, 0);
        if (len2 == SOCKET_ERROR)
		{
			int t_error;
			t_error = WSAGetLastError();
            if (t_error == WSAEINTR || errno == WSATRY_AGAIN)
                continue;
            return -1;
        }
		else if (len2 == 0)
			return -1;
        result += len2;
        len -= len2;
        buf += len2;
    }
    return  result;
}

void sys_close(int fd)
{
	closesocket(fd);
}

#endif

////////////////////////////////////////////////////////////////////////////////

static sys_lock_t s_global_lock;

static char *s_adb_path = nil;

static sys_thread_t s_tracking_thread = nil;
static bool s_tracking_enabled = false;
static int s_tracking_socket = -1;
static char *s_tracking_string = nil;
static bool s_tracking_notification_pending = false;
static MCObjectRef s_tracking_target = nil;

static int connect_to_adb_and_start_tracking(void)
{
	bool t_success;
	t_success = true;
	
	struct sockaddr_in t_server;
	memset(&t_server, 0, sizeof(t_server));
	t_server . sin_family = AF_INET;
	t_server . sin_port = htons(5037);
	t_server . sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	int t_socket;
	t_socket = -1;
	if (t_success)
	{
		t_socket = socket(PF_INET, SOCK_STREAM, 0);
		if (t_socket < 0)
			t_success = false;
	}
	
	if (t_success)
	{
		struct timeval tv;
		tv . tv_sec = 0;
		tv . tv_usec = 500 * 1000;
		setsockopt(t_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
	}
	
	if (t_success)
		if (connect(t_socket, (struct sockaddr*)&t_server, sizeof(t_server)) < 0)
			t_success = false;
	
	if (t_success)
	{
		const char *t_request = "host:track-devices";
		char t_buffer[64];
		sprintf(t_buffer, "%04x%s", (unsigned int)strlen(t_request), t_request);
		if (sys_write(t_socket, t_buffer, strlen(t_buffer)) < 0)
			t_success = false;
	}
	
	if (t_success)
	{
		char t_buffer[4];
		if (sys_read(t_socket, t_buffer, 4) != 4 ||
			memcmp(t_buffer, "OKAY", 4) != 0)
			t_success = false;
	}
	
	if (!t_success)
	{
		if (t_socket != -1)
			sys_close(t_socket);
		t_socket = -1;
	}
	
	return t_socket;
}

static int ensure_adb_server_and_connect(void)
{
	bool t_continue;
	t_continue = true;
	
	int t_socket;
	while(t_continue)
	{
		// Socket begins undefined
		t_socket = -1;
		
		// Acquire the global lock.
		sys_enter_lock(&s_global_lock);
		
		if (s_tracking_enabled)
		{
			// Make sure there is an adb server running
			sys_execute(s_adb_path, "start-server");
			
			// Now attempt to connect
			t_socket = connect_to_adb_and_start_tracking();
		}
		else
			t_continue = false;
		
		if (t_socket != -1)
		{
			s_tracking_socket = t_socket;
			t_continue = false;
		}
		
		sys_leave_lock(&s_global_lock);
		
		// If there is no socket yet, sleep for 5 seconds before trying again.
		if (t_continue && t_socket == -1)
			sys_sleep(5000);
	}
	
	return t_socket;
}

static void device_tracking_notification(void *context)
{
	sys_enter_lock(&s_global_lock);
	s_tracking_notification_pending = false;
	sys_leave_lock(&s_global_lock);
	
	MCDispatchStatus t_status;
	MCObjectDispatch(s_tracking_target, kMCDispatchTypeCommand, "androidDevicesChanged", nil, 0, &t_status);
}

static void *device_tracking_thread(void *context)
{
	for(;;)
	{
		// Try to connect to the adb server, connection failure at this point
		// means tracking has been stopped.
		int t_socket;
		t_socket = ensure_adb_server_and_connect();
		if (t_socket < 0)
			return nil;
		
		// Loop continuously until connection breaks in some way
		bool t_continue;
		t_continue = true;
		while(t_continue)
		{
			char t_header[5];
			
			if (sys_read(t_socket, t_header, 4) < 0)
				t_continue = false;
			
			int t_length;
			t_length = 0;
			if (t_continue)
				if (sscanf(t_header, "%04x", &t_length) != 1)
					t_continue = false;
			
			char *t_buffer;
			t_buffer = nil;
			if (t_continue)
			{	
				t_buffer = (char *)malloc(t_length + 1);
				if (sys_read(t_socket, t_buffer, t_length) != t_length)
					t_continue = false;
				
				t_buffer[t_length] = '\0';
			}
			
			// Only update the string inside the lock
			sys_enter_lock(&s_global_lock);
			if (s_tracking_string == nil ||
				t_buffer == nil ||
				strcmp(s_tracking_string, t_buffer) != 0)
			{
				free(s_tracking_string);
				s_tracking_string = t_buffer;
				if (!s_tracking_notification_pending)
				{
					s_tracking_notification_pending = false;
					MCRunOnMainThread(device_tracking_notification, nil, kMCRunOnMainThreadLater);
				}
			}
			sys_leave_lock(&s_global_lock);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

static void start_tracking_thread(void)
{
	if (s_tracking_enabled)
		return;
	
	s_tracking_enabled = true;
	sys_start_thread(&s_tracking_thread, device_tracking_thread);
}

static void stop_tracking_thread(void)
{
	if (!s_tracking_enabled)
		return;
	
	// Inside the lock we disable tracking and close the socket. This breaks
	// any pending IO and forces the thread to leave its looping.
	sys_enter_lock(&s_global_lock);
	if (s_tracking_socket != -1)
	{
		sys_close(s_tracking_socket);
		s_tracking_socket = -1;
	}
	s_tracking_enabled = false;
	sys_leave_lock(&s_global_lock);
	
	// Wait for the thread to finish.
	sys_finish_thread(s_tracking_thread);
	s_tracking_thread = nil;
}

bool revAndroidSetADBPath(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	if (argc != 1)
		return false;
	
	const char *t_path;
	if (MCVariableFetch(argv[0], kMCOptionAsCString, &t_path) != kMCErrorNone)
		return false;
	
	stop_tracking_thread();
	
	if (s_tracking_target != nil)
	{
		MCObjectRelease(s_tracking_target);
		s_tracking_target = nil;
	}
	
	if (s_tracking_string != nil)
	{
		free(s_tracking_string);
		s_tracking_string = nil;
	}

	if (*t_path != '\0')
	{
		s_adb_path = strdup(t_path);
		start_tracking_thread();
		
		MCContextTarget(&s_tracking_target);
	}
	
	return true;
}

bool revAndroidListDevices(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	sys_enter_lock(&s_global_lock);
	const char *t_cstring;
	if (s_tracking_string == nil)
		t_cstring = "";
	else
		t_cstring = s_tracking_string;
	MCVariableStore(result, kMCOptionAsCString, &t_cstring);
	sys_leave_lock(&s_global_lock);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool revAndroidStartLogging(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	return false;
}

bool revAndroidStopLogging(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool revAndroidStartup(void)
{
	s_tracking_thread = nil;
	s_tracking_enabled = false;
	s_tracking_socket = -1;
	s_tracking_string = nil;
	s_tracking_notification_pending = false;
	s_tracking_target = nil;
	
	sys_init_lock(&s_global_lock);
	
	return true;
}

void revAndroidShutdown(void)
{
	stop_tracking_thread();
	
	if (s_tracking_target != nil)
	{
		MCObjectRelease(s_tracking_target);
		s_tracking_target = nil;
	}
	
	if (s_tracking_string != nil)
	{
		free(s_tracking_string);
		s_tracking_string = nil;
	}
}

////////////////////////////////////////////////////////////////////////////////

MC_EXTERNAL_NAME("revandroid")
MC_EXTERNAL_STARTUP(revAndroidStartup)
MC_EXTERNAL_SHUTDOWN(revAndroidShutdown)
MC_EXTERNAL_HANDLERS_BEGIN
	MC_EXTERNAL_COMMAND("revAndroidSetADBPath", revAndroidSetADBPath)
	MC_EXTERNAL_FUNCTION("revAndroidListDevices", revAndroidListDevices)
	MC_EXTERNAL_COMMAND("revAndroidStartLogging", revAndroidStartLogging)
	MC_EXTERNAL_COMMAND("revAndroidStopLogging", revAndroidStopLogging)
MC_EXTERNAL_HANDLERS_END

////////////////////////////////////////////////////////////////////////////////
