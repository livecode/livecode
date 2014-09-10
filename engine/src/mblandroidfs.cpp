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

#include "system.h"

#include "mblandroid.h"
#include "mblandroidutil.h"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

extern MCStringRef MCcmd;

Boolean MCU_stoi4(const MCString&, int4& d);
Boolean MCU_stob(const MCString &s, Boolean &condition);

////////////////////////////////////////////////////////////////////////////////

static MCStringRef s_current_apk_folder = nil;

bool is_apk_path(MCStringRef p_path)
{
	int32_t t_cmdlen;
	t_cmdlen = MCStringGetLength(MCcmd);
	return MCStringBeginsWith(p_path, MCcmd, kMCCompareExact) && (MCStringGetNativeCharAtIndex(p_path, t_cmdlen) == '/' || MCStringGetNativeCharAtIndex(p_path, t_cmdlen) == '\0');
}

bool path_to_apk_path(MCStringRef p_path, MCStringRef &r_apk_path)
{    
	if (!is_apk_path(p_path))
		return false;
    
    uint32_t t_start;
    t_start = MCStringGetLength(MCcmd);
    
    if (MCStringGetNativeCharAtIndex(p_path, MCStringGetLength(MCcmd)) == '/')
        t_start++;
        
    return MCStringCopySubstring(p_path, MCRangeMake(t_start, MCStringGetLength(p_path) - t_start), r_apk_path);
}

bool path_from_apk_path(MCStringRef p_apk_path, MCStringRef& r_path)
{
	return MCStringFormat(r_path, "%@/%@", MCcmd, p_apk_path);
}

bool apk_folder_exists(MCStringRef p_apk_path)
{
	bool t_exists = false;
	MCAndroidEngineCall("isAssetFolder", "bx", &t_exists, p_apk_path);
	return t_exists;
}

bool apk_file_exists(MCStringRef p_apk_path)
{
	bool t_exists;
	MCAndroidEngineCall("isAssetFile", "bx", &t_exists, p_apk_path);
	return t_exists;
}

bool apk_get_file_length(MCStringRef p_apk_path, int32_t &r_length)
{
	if (!apk_file_exists(p_apk_path))
		return false;
	MCAndroidEngineCall("getAssetFileLength", "ix", &r_length, p_apk_path);
	return true;
}

bool apk_get_file_offset(MCStringRef p_apk_path, int32_t &r_offset)
{
	if (!apk_file_exists(p_apk_path))
		return false;
	MCAndroidEngineCall("getAssetFileStartOffset", "ix", &r_offset, p_apk_path);
	return true;
}

bool apk_get_current_folder(MCStringRef& r_folder)
{
	if (s_current_apk_folder != nil)
		return MCStringCopy(s_current_apk_folder, r_folder);
	return false;
}

bool apk_set_current_folder(MCStringRef p_apk_path)
{
	if (p_apk_path == nil)
	{
		MCValueRelease(s_current_apk_folder);
		s_current_apk_folder = nil;
		return true;
	}

	if (!apk_folder_exists(p_apk_path))
		return false;

	if (s_current_apk_folder != nil)
		MCValueRelease(s_current_apk_folder);
	s_current_apk_folder = MCValueRetain(p_apk_path);
	return true;
}

bool apk_list_folder_entries(MCSystemListFolderEntriesCallback p_callback, void *p_context)
{
	bool t_success = true;
	MCAutoStringRef t_list;
    MCAutoStringRef t_current_folder;
    
    if (!apk_get_current_folder(&t_current_folder))
        return false;
    
	MCAndroidEngineCall("getAssetFolderEntryList", "xx", &(&t_list), *t_current_folder);

	t_success = *t_list != nil;

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
    MCAutoStringRefAsUTF8String t_utf8_mccmd;
    /* UNCHECKED */ t_utf8_mccmd . Lock(MCcmd);
	stat(*t_utf8_mccmd, &t_stat);
	
	t_entry . modification_time = t_stat . st_mtime;
	t_entry . access_time = t_stat . st_atime;
	t_entry . user_id = t_stat . st_uid;
	t_entry . group_id = t_stat . st_gid;
	t_entry . permissions = t_stat . st_mode & 0444;

    char* t_next_entry;
    MCAutoStringRefAsUTF8String t_utf8_files;
    
    t_success = t_utf8_files . Lock(*t_list);
    
    if (t_success)
        t_next_entry = *t_utf8_files;
    
    bool t_more_entries;
    t_more_entries = true;
    
	while (t_success && t_more_entries)
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
            {
				t_next_entry = t_ffolder + MCCStringLength(t_ffolder);
                // AL-2014-06-25: [[ Bug 12659 ]] If there are no more lines, this is the last entry.
                t_more_entries = false;
            }

			t_success = MCU_stoi4(t_fsize, t_size) && MCU_stob(t_ffolder, t_is_folder);
		}
		if (t_success)
		{
            // SN-2014-01-13: [[ RefactorUnicode ]] Asset filenames are in ASCII
            MCStringRef t_assetFile;
            MCStringCreateWithCString(t_fname, t_assetFile);
            
			t_entry.name = t_assetFile;
			t_entry.data_size = t_size;
			t_entry.is_folder = t_is_folder;

			t_success = p_callback(p_context, &t_entry);
            MCValueRelease(t_assetFile);
		}
	}

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCAndroidSystem::CreateFolder(MCStringRef p_path)
{
	if (is_apk_path(p_path))
		return false;

    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
	return mkdir(*t_utf8_path, 0777) == 0;
}

Boolean MCAndroidSystem::DeleteFolder(MCStringRef p_path)
{
	if (is_apk_path(p_path))
		return false;

    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
	return rmdir(*t_utf8_path) == 0;
}

Boolean MCAndroidSystem::DeleteFile(MCStringRef p_path)
{
	if (is_apk_path(p_path))
		return false;

    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path . Lock(p_path);
	return unlink(*t_utf8_path) == 0;
}

Boolean MCAndroidSystem::RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name)
{
	if (is_apk_path(p_old_name) || is_apk_path(p_new_name))
		return false;

    MCAutoStringRefAsUTF8String t_old_name;
    /* UNCHECKED */ t_old_name . Lock(p_old_name);
    MCAutoStringRefAsUTF8String t_new_name;
    /* UNCHECKED */ t_new_name . Lock(p_new_name);
	return rename(*t_old_name, *t_new_name) == 0;
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCAndroidSystem::BackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
{
	return RenameFileOrFolder(p_old_name, p_new_name);
}

Boolean MCAndroidSystem::UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
{
	return RenameFileOrFolder(p_old_name, p_new_name);
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCAndroidSystem::CreateAlias(MCStringRef p_target, MCStringRef p_alias)
{
	if (is_apk_path(p_target) || is_apk_path(p_alias))
		return false;

    MCAutoStringRefAsUTF8String t_target;
    /* UNCHECKED */ t_target . Lock(p_target);
    MCAutoStringRefAsUTF8String t_alias;
    /* UNCHECKED */ t_alias . Lock(p_alias);
	return symlink(*t_target, *t_alias) == 0;
}

Boolean MCAndroidSystem::ResolveAlias(MCStringRef p_target, MCStringRef& r_dest)
{
	if (is_apk_path(p_target))
		return false;

	return MCStringCopy(p_target, r_dest);
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::GetCurrentFolder(MCStringRef& r_path)
{
    MCAutoStringRef t_folder;
	if (apk_get_current_folder(&t_folder))
	{
		return path_from_apk_path(*t_folder, r_path);
	}
	else
	{
        MCAutoNativeCharArray t_folder_char;
        if (!t_folder_char . New (PATH_MAX + 1))
            return false;

		if (NULL == getcwd((char*)t_folder_char . Chars(), PATH_MAX + 1))
			return false;
        
        // SN-2014-01-15: avoid to create an MCString with junk chars in the end
        t_folder_char . Resize(strlen((char*)t_folder_char . Chars()));
        
        return t_folder_char.CreateString(r_path);
    }
}

Boolean MCAndroidSystem::SetCurrentFolder(MCStringRef p_path)
{
    MCAutoStringRefAsUTF8String t_path;
    t_path . Lock(p_path);
	MCLog("SetCurrentFolder(%s)", *t_path);
	MCAutoStringRef t_apk_path;
	if (path_to_apk_path(p_path, &t_apk_path))
		return apk_set_current_folder(*t_apk_path);
	else
	{
		bool t_success = chdir(*t_path) == 0;
		if (t_success)
			apk_set_current_folder(nil);
		return t_success;
	}
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCAndroidSystem::FileExists(MCStringRef p_path)
{
	MCAutoStringRef t_apk_path;
	if (path_to_apk_path(p_path, &t_apk_path))
		return apk_file_exists(*t_apk_path);

	struct stat t_info;
	
	bool t_found;
    MCAutoStringRefAsUTF8String t_path;
    t_path . Lock(p_path);
	t_found = stat(*t_path, &t_info) == 0;
	if (t_found && (t_info.st_mode & S_IFDIR) == 0)
		return true;
	
	return false;
}

Boolean MCAndroidSystem::FolderExists(MCStringRef p_path)
{
	MCAutoStringRef t_apk_path;
	if (path_to_apk_path(p_path, &t_apk_path))
		return apk_folder_exists(*t_apk_path);

	struct stat t_info;
	
	bool t_found;
    MCAutoStringRefAsUTF8String t_path;
    t_path . Lock(p_path);
	t_found = stat(*t_path, &t_info) == 0;
	if (t_found && (t_info.st_mode & S_IFDIR) != 0)
		return true;
	
	return false;
}

Boolean MCAndroidSystem::FileNotAccessible(MCStringRef p_path)
{
	MCAutoStringRef t_apk_path;
	if (path_to_apk_path(p_path, &t_apk_path))
		return !apk_file_exists(*t_apk_path) && !apk_folder_exists(*t_apk_path);

	struct stat t_info;
    MCAutoStringRefAsUTF8String t_path;
    t_path . Lock(p_path);
	if (stat(*t_path, &t_info) != 0)
		return false;
	
	if ((t_info . st_mode & S_IFDIR) != 0)
		return true;
	
	if ((t_info . st_mode & S_IWUSR) == 0)
		return true;
	
	return false;
}

Boolean MCAndroidSystem::ChangePermissions(MCStringRef p_path, uint2 p_mask)
{
	if (is_apk_path(p_path))
		return false;

    MCAutoStringRefAsUTF8String t_path;
    t_path . Lock(p_path);
	return chmod(*t_path, p_mask) == 0;
}

uint2 MCAndroidSystem::UMask(uint2 p_mask)
{
	return umask(p_mask);
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::GetTemporaryFileName(MCStringRef &r_tmp_name)
{
	return MCStringCreateWithCString(tmpnam(NULL), r_tmp_name);
}

Boolean MCAndroidSystem::GetStandardFolder(MCNameRef p_folder, MCStringRef &r_folder)
{
	if (MCNameIsEqualToCString(p_folder, "engine", kMCCompareExact))
    {
        MCLog("GetStandardFolder(\"%@\") -> \"%@\"", MCNameGetString(p_folder), MCcmd);
		return MCStringCopy(MCcmd, r_folder);
    }
    
 	MCAutoStringRef t_stdfolder;
    MCAndroidEngineCall("getSpecialFolderPath", "xx", &(&t_stdfolder), MCNameGetString(p_folder));
    
    MCLog("GetStandardFolder(\"%@\") -> \"%@\"", p_folder, *t_stdfolder == nil ? kMCEmptyString : *t_stdfolder);
    
	r_folder = MCValueRetain(*t_stdfolder == nil ? kMCEmptyString : *t_stdfolder);
    return True;
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::LongFilePath(MCStringRef p_path, MCStringRef& r_long_path)
{
	return MCStringCopy(p_path, r_long_path);
}

bool MCAndroidSystem::ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path)
{
	return MCStringCopy(p_path, r_short_path);
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::PathToNative(MCStringRef p_path, MCStringRef& r_native)
{
	return MCStringCopy(p_path, r_native);
}

bool MCAndroidSystem::PathFromNative(MCStringRef p_native, MCStringRef& r_path)
{
	return MCStringCopy(p_native, r_path);
}

bool MCAndroidSystem::ResolvePath(MCStringRef p_path, MCStringRef& r_resolved)
{
    MCAutoStringRef t_path_no_extra_slashes;
	MCAutoStringRef t_absolute_path;
	if (MCStringGetCharAtIndex(p_path, 0) != '/')
	{
		MCAutoStringRef t_folder;
		if (!GetCurrentFolder(&t_folder))
			return false;
        
		if (!MCStringMutableCopy(*t_folder, &t_absolute_path) ||
			!MCStringAppendChar(*t_absolute_path, '/') ||
			!MCStringAppend(*t_absolute_path, p_path))
			return false;
        
        /* UNCHECKED */ MCStringMutableCopy(*t_absolute_path, &t_path_no_extra_slashes);
	}
	else
        /* UNCHECKED */ MCStringMutableCopy(p_path, &t_path_no_extra_slashes);

    // IM-2012-10-09 - [[ BZ 10432 ]] strip out extra slashes from paths
    uindex_t t_length;
    t_length = MCStringGetLength(*t_path_no_extra_slashes);
    uindex_t t_offset;
    t_offset = 1;
    while (t_offset < t_length && MCStringFirstIndexOfChar(*t_path_no_extra_slashes, '/', t_offset, kMCStringOptionCompareExact, t_offset))
	{
        if (MCStringGetNativeCharAtIndex(*t_path_no_extra_slashes, ++t_offset) == '/')
        {
			MCStringRemove(*t_path_no_extra_slashes, MCRangeMake(t_offset, 1));
            t_length--;
        }
	}
    
    r_resolved = MCValueRetain(*t_path_no_extra_slashes);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::ListFolderEntries(MCSystemListFolderEntriesCallback p_callback, void *p_context)
{
    MCAutoStringRef t_folder;
	if (apk_get_current_folder(&t_folder))
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
        
        MCStringRef t_unicode_str;
        MCStringCreateWithBytes((byte_t*)t_dir_entry -> d_name, strlen(t_dir_entry -> d_name), kMCStringEncodingUTF8, false, t_unicode_str);        
		
		t_entry . name = t_unicode_str;
		t_entry . data_size = t_stat . st_size;
		t_entry . resource_size = 0;
		t_entry . modification_time = t_stat . st_mtime;
		t_entry . access_time = t_stat . st_atime;
		t_entry . user_id = t_stat . st_uid;
		t_entry . group_id = t_stat . st_gid;
		t_entry . permissions = t_stat . st_mode & 0777;
		t_entry . is_folder = (t_stat . st_mode & S_IFDIR) != 0;
		
		t_success = p_callback(p_context, &t_entry);
        MCValueRelease(t_unicode_str);
	}
	
	closedir(t_dir);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

real8 MCAndroidSystem::GetFreeDiskSpace()
{
    return 0.0;
}

