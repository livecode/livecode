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
#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "card.h"
#include "field.h"
#include "stacklst.h"
#include "sellst.h"
#include "util.h"

#include "debug.h"
#include "param.h"

#include "mctheme.h"

#include "globals.h"

#include "w32dc.h"

#include <shobjidl.h>
#include <shlobj.h>
#include <shlwapi.h>

#ifdef FEATURE_RELAUNCH_SUPPORT

#define CWM_TASKBAR_NOTIFICATION (WM_USER + 1)
#define CWM_RELAUNCH (WM_USER + 3)

static HANDLE s_relaunch_pid_handle = NULL;
static char *s_relaunch_pid_folder = NULL;
static char *s_relaunch_pid_filename = NULL;

struct instance_t
{
	unsigned int id[8];
	unsigned int launch_time;
	DWORD process_id;
  HWND message_window;
	HANDLE process_handle;
};

struct message_t
{
	HWND window;
	DWORD id;
	void *data;
	unsigned int data_length;
	unsigned int timeout;
};

extern void md5_compute(const char *p_data, unsigned int p_length, void *p_buffer);
extern MCStringRef MCcmdline;

static char *s_previous_folder = NULL;

char *strndup(const char *p_string, size_t p_length)
{
	char *t_result;
	t_result = (char *)malloc(p_length + 1);
	strncpy(t_result, p_string, p_length);
	t_result[p_length] = '\0';
	return t_result;
}

static bool push_folder(const char *p_folder)
{
	if (s_previous_folder != NULL)
		assert(FALSE);

	DWORD t_length;
	t_length = GetCurrentDirectoryA(0, NULL);
	s_previous_folder = (char *)malloc(t_length);
	GetCurrentDirectoryA(t_length, s_previous_folder);

	return SetCurrentDirectoryA(p_folder) == TRUE;
}

static void pop_folder(void)
{
	if (s_previous_folder == NULL)
		assert(FALSE);

	SetCurrentDirectoryA(s_previous_folder);
	free(s_previous_folder);
	s_previous_folder = NULL;
}

static bool ensure_folder(const char *p_folder)
{
	bool t_error;
	t_error = false;

	char *t_previous_folder;

	DWORD t_length;
	t_length = GetCurrentDirectoryA(0, NULL);
	t_previous_folder = (char *)malloc(t_length);
	GetCurrentDirectoryA(t_length, t_previous_folder);

	const char *t_separator;
  t_separator = p_folder;
	while(t_separator != NULL && !t_error)
	{
		const char *t_segment_end;
		t_segment_end = strchr(t_separator, '\\');

		char *t_segment;
		if (t_segment_end != NULL)
		{
			t_segment = strndup(t_separator, t_segment_end - t_separator + 1);
			t_separator = t_segment_end + 1;
		}
		else
		{
			t_segment = strdup(t_separator);
			t_separator = NULL;
		}

		if (!SetCurrentDirectoryA(t_segment))
		{
			t_error = CreateDirectoryA(t_segment, NULL) != TRUE;
			if (!t_error)
				t_error = SetCurrentDirectoryA(t_segment) != TRUE;
		}

		free(t_segment);
	}

	SetCurrentDirectoryA(t_previous_folder);
	free(t_previous_folder);

	return t_error;
}

static char *get_executable_folder(void)
{
	char t_executable_path[4096];

	if (GetModuleFileNameA(NULL, t_executable_path, 4096) == 4096)
		return NULL;
	
	char *t_last_separator;
	t_last_separator = strrchr(t_executable_path, '\\');
	if (t_last_separator != NULL)
		*t_last_separator = '\0';

	return strdup(t_executable_path);
}

static char *get_special_folder(DWORD t_id, bool p_ensure)
{
	char t_path[MAX_PATH];
	char *t_result;
	t_result = NULL;

	LPITEMIDLIST t_item_id_list;
	LPMALLOC t_malloc;
	SHGetMalloc(&t_malloc);
	if (SHGetSpecialFolderLocation(HWND_DESKTOP, t_id, &t_item_id_list) == 0)
	{
		if (SHGetPathFromIDListA(t_item_id_list, t_path))
			t_result = strdup(t_path);
		t_malloc -> Free(t_item_id_list);
	}
	t_malloc -> Release();
		
	return t_result;
}

static char *concatenate(char *p_first, bool p_free_first, char *p_last, bool p_free_last)
{
	char *t_result;
	t_result = (char *)malloc(strlen(p_first) + strlen(p_last) + 1);
	if (t_result != NULL)
	{
		strcpy(t_result, p_first);
		strcat(t_result, p_last);
	}

	if (p_free_first)
		free(p_first);

	if (p_free_last)
		free(p_last);

	return t_result;
}

bool message_send_with_data(message_t *p_message, unsigned int *r_reply)
{
	COPYDATASTRUCT t_data;
	DWORD_PTR t_result;

	t_data . dwData = CWM_RELAUNCH;
	t_data . cbData = p_message -> data_length;
	t_data . lpData = p_message -> data;
	if (SendMessageTimeoutA(p_message -> window, WM_COPYDATA, (WPARAM)(((MCScreenDC *)MCscreen) -> getinvisiblewindow()), (LPARAM)&t_data, SMTO_BLOCK, p_message -> timeout, &t_result) == 0)
	{
		DWORD t_error;
		t_error = GetLastError();
		return false;
	}

	*r_reply = t_result;

	return true;
}

bool relaunch_get_current_instance(instance_t& r_instance, MCStringRef p_id)
{
	char t_executable_path[4096];

	if (GetModuleFileNameA(NULL, t_executable_path, 4096) == 4096)
		return false;

	unsigned int t_file_id[4];
	unsigned int t_file_id_length = 0;

	HANDLE t_executable_file_handle;
	t_executable_file_handle = CreateFileA(t_executable_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (t_executable_file_handle != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION t_file_info;
		if (GetFileInformationByHandle(t_executable_file_handle, &t_file_info))
		{
			t_file_id[t_file_id_length++] = t_file_info . dwVolumeSerialNumber;
			t_file_id[t_file_id_length++] = t_file_info . nFileIndexHigh;
			t_file_id[t_file_id_length++] = t_file_info . nFileIndexLow;
		}

		CloseHandle(t_executable_file_handle);
	}
	
	if (t_file_id_length == 0)
	{
		md5_compute(t_executable_path, strlen(t_executable_path), t_file_id);
		t_file_id_length = 4;
	}

	if (t_file_id_length == 3)
		t_file_id[t_file_id_length++] = 0;

	unsigned int t_stack_id[4];
    MCAutoPointer<char> t_id;
    /* UNCHECKED */ MCStringConvertToCString(p_id, &t_id);
	md5_compute(*t_id, MCStringGetLength(p_id), t_stack_id);

	unsigned int t_process_id;
	t_process_id = GetCurrentProcessId();

	memcpy(&r_instance . id[0], t_file_id, 16);
	memcpy(&r_instance . id[4], t_stack_id, 16);
	r_instance . process_id = t_process_id;
	r_instance . message_window = MCscreen != NULL ? ((MCScreenDC *)MCscreen) -> getinvisiblewindow() : NULL;
	r_instance . launch_time = timeGetTime();
	r_instance . process_handle = NULL;

	return true;
}

void relaunch_end_instance(void)
{
	if (s_relaunch_pid_handle != NULL)
	{
		CloseHandle(s_relaunch_pid_handle);
		s_relaunch_pid_handle = NULL;

		push_folder(s_relaunch_pid_folder);
		DeleteFileA(s_relaunch_pid_filename);
		pop_folder();

		delete s_relaunch_pid_folder;
		s_relaunch_pid_folder = NULL;
		delete s_relaunch_pid_filename;
		s_relaunch_pid_filename = NULL;
	}
}

bool relaunch_begin_instance(const char *p_instance_folder, const instance_t& p_current_instance)
{
	bool t_error;
	t_error = false;

	t_error = !push_folder(p_instance_folder);

	char t_pid_file[128];
	if (!t_error)
	{
		sprintf(t_pid_file, "._%08x%08x%08x%08x%08x%08x%08x%08x_%08x.pid",
		  									p_current_instance . id[0], p_current_instance . id[1], p_current_instance . id[2], p_current_instance . id[3],
			  								p_current_instance . id[4], p_current_instance . id[5], p_current_instance . id[6], p_current_instance . id[7],
				  							p_current_instance . process_id);

  	s_relaunch_pid_handle = CreateFileA(t_pid_file, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
	  t_error = (s_relaunch_pid_handle == INVALID_HANDLE_VALUE);
	}
	
	if (!t_error)
	{
		DWORD t_written;
		if (!WriteFile(s_relaunch_pid_handle, &p_current_instance, sizeof(instance_t), &t_written, NULL))
			t_written = 0;

		t_error = (t_written != sizeof(instance_t));
	}

	if (!t_error)
	{
		s_relaunch_pid_folder = strdup(p_instance_folder);
		s_relaunch_pid_filename = strdup(t_pid_file);
		atexit(relaunch_end_instance);
	}
	else
	{
		if (s_relaunch_pid_handle != NULL)
		{
			CloseHandle(s_relaunch_pid_handle);
			s_relaunch_pid_handle = NULL;
		}
	}

	pop_folder();

	return !t_error;
}

// Return a list of the current instances matching p_current_instance in the given instance folder.
bool relaunch_list_instances(const char *p_instance_folder, const instance_t& p_current_instance, instance_t*& r_instances, unsigned int& r_instance_count)
{
	if (!push_folder(p_instance_folder))
		return false;

	char t_find_pattern[128];
	sprintf(t_find_pattern, "._%08x%08x%08x%08x%08x%08x%08x%08x_????????.pid",
													p_current_instance . id[0], p_current_instance . id[1], p_current_instance . id[2], p_current_instance . id[3],
													p_current_instance . id[4], p_current_instance . id[5], p_current_instance . id[6], p_current_instance . id[7]);

	HANDLE t_find_handle;
	WIN32_FIND_DATAA t_find_data;
	t_find_handle = FindFirstFileA(t_find_pattern, &t_find_data);
	if (t_find_handle != INVALID_HANDLE_VALUE)
	{
		instance_t *t_instances;
		unsigned int t_instance_count;

		t_instances = NULL;
		t_instance_count = 0;

		do
		{
			HANDLE t_file_handle;
			t_file_handle = CreateFileA(t_find_data . cFileName, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

			if (t_file_handle == INVALID_HANDLE_VALUE)
			{
				DWORD t_error;
				t_error = GetLastError();
				continue;
			}

			instance_t t_instance;
			DWORD t_read;
			if (!ReadFile(t_file_handle, &t_instance, sizeof(instance_t), &t_read, NULL))
				t_read = 0;

			CloseHandle(t_file_handle);

			if (t_read != sizeof(instance_t))
				continue;

			if (DeleteFileA(t_find_data . cFileName))
				continue;

			HANDLE t_process_handle;
			t_process_handle = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, t_instance . process_id);
			if (t_process_handle == NULL)
				continue;

			t_instance . process_handle = t_process_handle;

			t_instances = (instance_t *)realloc(t_instances, (t_instance_count + 1) * sizeof(instance_t));
			t_instances[t_instance_count++] = t_instance;
		}
		while(FindNextFileA(t_find_handle, &t_find_data));
		FindClose(t_find_handle);

		r_instances = t_instances;
		r_instance_count = t_instance_count;
	}

	pop_folder();

	return true;
}

bool relaunch_startup(MCStringRef p_stack_name)
{
	bool t_error;
	t_error = false;

	bool t_exit;
	t_exit = false;

	char *t_relaunch_folder;
	t_relaunch_folder = NULL;

	t_relaunch_folder = get_special_folder(CSIDL_LOCAL_APPDATA, true);
	if (t_relaunch_folder == NULL)
		t_relaunch_folder = get_special_folder(CSIDL_APPDATA, true);
	if (t_relaunch_folder != NULL)
		t_relaunch_folder = concatenate(t_relaunch_folder, true, "\\._LiveCode_.", false);
	if (t_relaunch_folder != NULL)
		ensure_folder(t_relaunch_folder);

	t_error = t_relaunch_folder == NULL;

	instance_t t_current_instance;
	if (!t_error)
		t_error = !relaunch_get_current_instance(t_current_instance, p_stack_name);

	instance_t *t_existing_instances;
	unsigned int t_existing_instance_count;
	t_existing_instances = NULL;
	t_existing_instance_count = 0;
	if (!t_error)
		t_error = !relaunch_list_instances(t_relaunch_folder, t_current_instance, t_existing_instances, t_existing_instance_count);

	if (!t_error)
	{
		for(unsigned int t_instance = 0; t_instance < t_existing_instance_count; ++t_instance)
		{
			MCAutoStringRefAsLPWSTR t_cmdline_wstr;
			/* UNCHECKED */ t_cmdline_wstr.Lock(MCcmdline);
			
			message_t t_message;
			t_message . window = t_existing_instances[t_instance] . message_window;
			t_message . id = CWM_RELAUNCH;
			t_message . data = *t_cmdline_wstr;
			// SN-2014-11-19: [[ Bug 14058 ]] We pass the number of bytes, not the numbers of wchars 
			t_message . data_length = MCStringGetLength(MCcmdline) * 2;
			t_message . timeout = 1 << 30;

			unsigned int t_reply;
			if (message_send_with_data(&t_message, &t_reply))
				if (t_reply != NULL)
				{
					if ((HWND)t_reply != HWND_BOTTOM && IsWindowVisible((HWND)t_reply))
						SetForegroundWindow((HWND)t_reply);
					t_exit = true;
					break;
				}
		}

		if (!t_exit)
			t_error = relaunch_begin_instance(t_relaunch_folder, t_current_instance);
	}

	if (t_existing_instances != NULL)
	{
		for(unsigned int t_index = 0; t_index < t_existing_instance_count; t_index++)
			CloseHandle(t_existing_instances[t_index] . process_handle);

		free(t_existing_instances);
	}

	if (t_relaunch_folder != NULL)
		free(t_relaunch_folder);

	return t_exit && !t_error;
}

void relaunch_shutdown(void)
{
	relaunch_end_instance();
}

#endif
