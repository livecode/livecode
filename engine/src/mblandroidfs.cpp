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

#include "core.h"
#include "system.h"

#include "mblandroid.h"
#include "mblandroidutil.h"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

extern char *MCcmd;

Boolean MCU_stoi4(const MCString&, int4& d);
Boolean MCU_stob(const MCString &s, Boolean &condition);

////////////////////////////////////////////////////////////////////////////////

static char *s_current_apk_folder = nil;

bool is_apk_path(const char *p_path)
{
	int32_t t_cmdlen;
	t_cmdlen = MCCStringLength(MCcmd);
	return MCCStringBeginsWith(p_path, MCcmd) && (p_path[t_cmdlen] == '/' || p_path[t_cmdlen] == '\0');
}

bool path_to_apk_path(const char * p_path, const char *&r_apk_path)
{
	char *t_path = nil;
	if (!is_apk_path(p_path))
		return false;
	r_apk_path = &p_path[MCCStringLength(MCcmd)];
	if (r_apk_path[0] == '/')
		r_apk_path += 1;
	return true;
}

bool path_from_apk_path(const char *p_apk_path, char *&r_path)
{
	return MCCStringFormat(r_path, "%s/%s", MCcmd, p_apk_path);
}

bool apk_folder_exists(const char *p_apk_path)
{
	bool t_exists = false;
	MCAndroidEngineCall("isAssetFolder", "bs", &t_exists, p_apk_path);
	return t_exists;
}

bool apk_file_exists(const char *p_apk_path)
{
	bool t_exists;
	MCAndroidEngineCall("isAssetFile", "bs", &t_exists, p_apk_path);
	return t_exists;
}

bool apk_get_file_length(const char *p_apk_path, int32_t &r_length)
{
	if (!apk_file_exists(p_apk_path))
		return false;
	MCAndroidEngineCall("getAssetFileLength", "is", &r_length, p_apk_path);
	return true;
}

bool apk_get_file_offset(const char *p_apk_path, int32_t &r_offset)
{
	if (!apk_file_exists(p_apk_path))
		return false;
	MCAndroidEngineCall("getAssetFileStartOffset", "is", &r_offset, p_apk_path);
	return true;
}

const char *apk_get_current_folder()
{
	return s_current_apk_folder;
}

bool apk_set_current_folder(const char *p_apk_path)
{
	if (p_apk_path == nil)
	{
		MCCStringFree(s_current_apk_folder);
		s_current_apk_folder = nil;
		return true;
	}

	if (!apk_folder_exists(p_apk_path))
		return false;

	char *t_new_path = nil;
	if (!MCCStringClone(p_apk_path, t_new_path))
		return false;

	MCCStringFree(s_current_apk_folder);
	s_current_apk_folder = t_new_path;
	return true;
}

bool apk_list_folder_entries(MCSystemListFolderEntriesCallback p_callback, void *p_context)
{
	bool t_success = true;
	char *t_list = nil;
	MCAndroidEngineCall("getAssetFolderEntryList", "ss", &t_list, apk_get_current_folder());

	t_success = t_list != nil;

	// getAssetFolderEntryList returns entries in the following format:
	// each entry comprises two lines - the filename on the first line,
	// followed by a second line containing the filesize & is_folder boolean separated by a comma

	char *t_fname;
	char *t_fsize;
	char *t_ffolder;

	MCSystemFolderEntry t_entry;
	MCMemoryClear(&t_entry, sizeof(t_entry));
	t_entry.permissions = 0444;

	// get stat info from bundle file
	struct stat t_stat;
	stat(MCcmd, &t_stat);
	
	t_entry . modification_time = t_stat . st_mtime;
	t_entry . access_time = t_stat . st_atime;
	t_entry . user_id = t_stat . st_uid;
	t_entry . group_id = t_stat . st_gid;
	t_entry . permissions = t_stat . st_mode & 0444;

	char *t_next_entry;
	t_next_entry = t_list;
	while (t_success && t_next_entry[0] != '\0')
	{
		uint32_t t_next_index = 0;
		uint32_t t_size_index = 0;
		uint32_t t_folder_index = 0;
		int32_t t_size;
		Boolean t_is_folder;

		t_fname = t_next_entry;
		t_success = MCCStringFirstIndexOf(t_fname, '\n', t_size_index);

		if (t_success)
		{
			t_fsize = t_fname + t_size_index;
			*t_fsize++ = '\0';

			t_success = MCCStringFirstIndexOf(t_fsize, ',', t_folder_index);
		}
		if (t_success)
		{
			t_ffolder = t_fsize + t_folder_index;
			*t_ffolder++ = '\0';

			if (MCCStringFirstIndexOf(t_ffolder, '\n', t_next_index))
			{
				t_next_entry = t_ffolder + t_next_index;
				*t_next_entry++ = '\0';
			}
			else
				t_next_entry = t_ffolder + MCCStringLength(t_ffolder);

			t_success = MCU_stoi4(t_fsize, t_size) && MCU_stob(t_ffolder, t_is_folder);
		}
		if (t_success)
		{
			t_entry.name = t_fname;
			t_entry.data_size = t_size;
			t_entry.is_folder = t_is_folder;

			t_success = p_callback(p_context, &t_entry);
		}
	}

	MCCStringFree(t_list);
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::CreateFolder(const char *p_path)
{
	if (is_apk_path(p_path))
		return false;

	return mkdir(p_path, 0777) == 0;
}

bool MCAndroidSystem::DeleteFolder(const char *p_path)
{
	if (is_apk_path(p_path))
		return false;

	return rmdir(p_path) == 0;
}

bool MCAndroidSystem::DeleteFile(const char *p_path)
{
	if (is_apk_path(p_path))
		return false;

	return unlink(p_path) == 0;
}

bool MCAndroidSystem::RenameFileOrFolder(const char *p_old_name, const char *p_new_name)
{
	if (is_apk_path(p_old_name) || is_apk_path(p_new_name))
		return false;

	return rename(p_old_name, p_new_name) == 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::BackupFile(const char *p_old_name, const char *p_new_name)
{
	return RenameFileOrFolder(p_old_name, p_new_name);
}

bool MCAndroidSystem::UnbackupFile(const char *p_old_name, const char *p_new_name)
{
	return RenameFileOrFolder(p_old_name, p_new_name);
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::CreateAlias(const char *p_target, const char *p_alias)
{
	if (is_apk_path(p_target) || is_apk_path(p_alias))
		return false;

	return symlink(p_target, p_alias) == 0;
}

char *MCAndroidSystem::ResolveAlias(const char *p_target)
{
	if (is_apk_path(p_target))
		return false;

	return strdup(p_target);
}

////////////////////////////////////////////////////////////////////////////////

char *MCAndroidSystem::GetCurrentFolder(void)
{
	char *t_folder = NULL;
	if (apk_get_current_folder() != nil)
		path_from_apk_path(apk_get_current_folder(), t_folder);
	else
	{
		if (MCMemoryAllocate(PATH_MAX + 1, t_folder))
		{
			if (NULL == getcwd(t_folder, PATH_MAX + 1))
			{
				MCMemoryDeallocate(t_folder);
				return NULL;
			}
			uindex_t t_len = MCCStringLength(t_folder);
			MCMemoryReallocate(t_folder, t_len + 1, t_folder);
		}
	}
	return t_folder;
}

bool MCAndroidSystem::SetCurrentFolder(const char *p_path)
{
	MCLog("SetCurrentFolder(%s)", p_path);
	const char *t_apk_path = nil;
	if (path_to_apk_path(p_path, t_apk_path))
		return apk_set_current_folder(t_apk_path);
	else
	{
		bool t_success = chdir(p_path) == 0;
		if (t_success)
			apk_set_current_folder(nil);
		return t_success;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::FileExists(const char *p_path) 
{
	const char *t_apk_path = nil;
	if (path_to_apk_path(p_path, t_apk_path))
		return apk_file_exists(t_apk_path);

	struct stat t_info;
	
	bool t_found;
	t_found = stat(p_path, &t_info) == 0;
	if (t_found && (t_info.st_mode & S_IFDIR) == 0)
		return true;
	
	return false;
}

bool MCAndroidSystem::FolderExists(const char *p_path)
{
	const char *t_apk_path = nil;
	if (path_to_apk_path(p_path, t_apk_path))
		return apk_folder_exists(t_apk_path);

	struct stat t_info;
	
	bool t_found;
	t_found = stat(p_path, &t_info) == 0;
	if (t_found && (t_info.st_mode & S_IFDIR) != 0)
		return true;
	
	return false;
}

bool MCAndroidSystem::FileNotAccessible(const char *p_path)
{
	const char *t_apk_path = nil;
	if (path_to_apk_path(p_path, t_apk_path))
		return !apk_file_exists(t_apk_path) && !apk_folder_exists(t_apk_path);

	struct stat t_info;
	if (stat(p_path, &t_info) != 0)
		return false;
	
	if ((t_info . st_mode & S_IFDIR) != 0)
		return true;
	
	if ((t_info . st_mode & S_IWUSR) == 0)
		return true;
	
	return false;
}

bool MCAndroidSystem::ChangePermissions(const char *p_path, uint2 p_mask)
{
	if (is_apk_path(p_path))
		return false;

	return chmod(p_path, p_mask) == 0;
}

uint2 MCAndroidSystem::UMask(uint2 p_mask)
{
	return umask(p_mask);
}

////////////////////////////////////////////////////////////////////////////////

char *MCAndroidSystem::GetTemporaryFileName(void)
{
	return strdup(tmpnam(NULL));
}

char *MCAndroidSystem::GetStandardFolder(const char *p_folder)
{
    char *t_stdfolder = NULL;
    // SN-2015-04-16: [[ Bug 14295 ]] The resources folder on Mobile is the same
    //   as the engine folder.
	if (MCCStringEqualCaseless(p_folder, "engine") ||
            MCCStringEqualCaseless(p_folder, "resources"))
		MCCStringClone(MCcmd, t_stdfolder);
	else
		MCAndroidEngineCall("getSpecialFolderPath", "ss", &t_stdfolder, p_folder);

	MCLog("GetStandardFolder(\"%s\") -> \"%s\"", p_folder, t_stdfolder == NULL ? "" : t_stdfolder);
	return t_stdfolder;
}

////////////////////////////////////////////////////////////////////////////////

char *MCAndroidSystem::LongFilePath(const char *p_path)
{
	return strclone(p_path);
}

char *MCAndroidSystem::ShortFilePath(const char *p_path)
{
	return strclone(p_path);
}

////////////////////////////////////////////////////////////////////////////////

char *MCAndroidSystem::PathToNative(const char *p_path)
{
	return strdup(p_path);
}

char *MCAndroidSystem::PathFromNative(const char *p_path)
{
	return strdup(p_path);
}

char *MCAndroidSystem::ResolvePath(const char *p_path)
{
	return ResolveNativePath(p_path);
}

char *MCAndroidSystem::ResolveNativePath(const char *p_path)
{
	MCLog("MCAndroidSystem::ResolveNativePath called", 0);
	MCLog("p_path=<%p>: \"%s\"", p_path, p_path != NULL ? p_path : "");
	char *t_absolute_path;
	if (p_path[0] != '/')
	{
		char *t_folder;
		t_folder = GetCurrentFolder();
		MCLog("current folder: <%p>: \"%s\"", t_folder, t_folder != NULL ? t_folder : "");
		t_absolute_path = new char[strlen(t_folder) + strlen(p_path) + 2];
		strcpy(t_absolute_path, t_folder);
		strcat(t_absolute_path, "/");
		strcat(t_absolute_path, p_path);
		
		// MW-2011-07-04: GetCurrentFolder() returns a string that must be
		//   freed.
		free(t_folder);
	}
	else
		t_absolute_path = strdup(p_path);
	
	// IM-2012-10-09 - [[ BZ 10432 ]] strip out extra slashes from paths
	uindex_t t_length = MCCStringLength(t_absolute_path);
	char *t_str_ptr = t_absolute_path + 1;
	for (uindex_t i = 1; i < t_length; i++)
	{
		if (t_absolute_path[i - 1] != '/' || t_absolute_path[i] != '/')
			*t_str_ptr++ = t_absolute_path[i];
	}
	*t_str_ptr = '\0';
	
	MCLog("MCAndroidSystem::ResolveNativePath done", 0);
	MCLog("resolvedpath=<%p>: \"%s\"", t_absolute_path, t_absolute_path != NULL ? t_absolute_path : "");
	return t_absolute_path;
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::ListFolderEntries(MCSystemListFolderEntriesCallback p_callback, void *p_context)
{
	if (apk_get_current_folder() != nil)
		return apk_list_folder_entries(p_callback, p_context);

	DIR *t_dir;
	t_dir = opendir(".");
	if (t_dir == NULL)
		return false;
	
	MCSystemFolderEntry t_entry;
	memset(&t_entry, 0, sizeof(MCSystemFolderEntry));
	
	bool t_success;
	t_success = true;
	while(t_success)
	{
		struct dirent *t_dir_entry;
		t_dir_entry = readdir(t_dir);
		if (t_dir_entry == NULL)
			break;
		
		if (strcmp(t_dir_entry -> d_name, ".") == 0)
			continue;
		
		struct stat t_stat;
		stat(t_dir_entry -> d_name, &t_stat);
		
		t_entry . name = t_dir_entry -> d_name;
		t_entry . data_size = t_stat . st_size;
		t_entry . resource_size = 0;
		t_entry . modification_time = t_stat . st_mtime;
		t_entry . access_time = t_stat . st_atime;
		t_entry . user_id = t_stat . st_uid;
		t_entry . group_id = t_stat . st_gid;
		t_entry . permissions = t_stat . st_mode & 0777;
		t_entry . is_folder = (t_stat . st_mode & S_IFDIR) != 0;
		
		t_success = p_callback(p_context, &t_entry);
	}
	
	closedir(t_dir);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
