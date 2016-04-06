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

///////////////////////////////////////////////////////////////////////////////
// MAC OS X Support function implementations

#include "osxsupport.h"

#ifndef _MAC_SERVER
void UnloadDBBundle(CFBundleRef p_bundle)
{
	if (p_bundle)
	{
		CFBundleUnloadExecutable(p_bundle);
		CFRelease(p_bundle);
	}
}

bool FolderExists(const char *p_path)
{
	struct stat t_stat;
	if (stat(p_path, &t_stat) != 0)
		return 0;
		
	return (t_stat . st_mode & S_IFDIR) != 0;
}

char *GetBundleFolder(CFBundleRef p_bundle)
{
	CFURLRef t_bundle_url;
	t_bundle_url = CFBundleCopyBundleURL(p_bundle);
	if (t_bundle_url == NULL)
		return NULL;
	
	CFURLRef t_bundle_folder_url;
	t_bundle_folder_url = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, t_bundle_url);
	CFRelease(t_bundle_url);
	if (t_bundle_folder_url == NULL)
		return NULL;
		
	CFStringRef t_bundle_folder_path;
	t_bundle_folder_path = CFURLCopyFileSystemPath(t_bundle_folder_url, kCFURLPOSIXPathStyle);
	CFRelease(t_bundle_folder_url);
	if (t_bundle_folder_path == NULL)
		return NULL;
		
	CFIndex t_length;
	t_length = CFStringGetMaximumSizeForEncoding(CFStringGetLength(t_bundle_folder_path), kCFStringEncodingUTF8);
	
	char *t_result;
	t_result = (char *)malloc(t_length + 1);
	CFStringGetCString(t_bundle_folder_path, t_result, t_length + 1, kCFStringEncodingUTF8);
	CFRelease(t_bundle_folder_path);
	t_result = (char *)realloc(t_result, strlen(t_result) + 1);
	
	return t_result;
}

const char *GetExternalFolder(void)
{
	static char *s_folder = NULL;
	if (s_folder == NULL)
	{		
		CFBundleRef t_bundle;
		t_bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.runrev.revdb"));
		if (t_bundle != NULL)
			s_folder = GetBundleFolder(t_bundle);
	}
	return s_folder;
}

const char *GetApplicationFolder(void)
{
	static char *s_folder = NULL;
	if (s_folder == NULL)
	{		
		CFBundleRef t_bundle;
		t_bundle = CFBundleGetMainBundle();
		if (t_bundle != NULL)
			s_folder = GetBundleFolder(t_bundle);
	}
	return s_folder;
}

DATABASEREC *DoLoadDatabaseDriver(const char *p_path)
{
	char t_actual_path[PATH_MAX];
	
	if (!FolderExists(p_path))
	{
		sprintf(t_actual_path, "%s.bundle", p_path);
		if (!FolderExists(t_actual_path))
			return NULL;
	}
	else
		strcpy(t_actual_path, p_path);
	
	CFStringRef t_path_string;
	t_path_string = CFStringCreateWithCString(kCFAllocatorDefault, t_actual_path, kCFStringEncodingUTF8);
	if (t_path_string == NULL)
		return NULL;
	
	CFURLRef t_url;
	t_url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, t_path_string, kCFURLPOSIXPathStyle, true);
	CFRelease(t_path_string);
	if (t_url == NULL)
		return NULL;
	
	CFBundleRef t_bundle;
	t_bundle = CFBundleCreate(kCFAllocatorDefault, t_url);
	CFRelease(t_url);
	if (t_bundle == NULL)
		return NULL;
		
	if (!CFBundleLoadExecutable(t_bundle))
	{
		CFRelease(t_bundle);
		return NULL;
	}
	
	DATABASEREC *t_result;
	t_result = new DATABASEREC;
	t_result -> driverref = t_bundle;
	t_result -> idcounterptr = (idcounterrefptr)CFBundleGetFunctionPointerForName(t_bundle, CFSTR("setidcounterref"));
	t_result -> newconnectionptr = (new_connectionrefptr)CFBundleGetFunctionPointerForName(t_bundle, CFSTR("newdbconnectionref"));
	t_result -> releaseconnectionptr = (release_connectionrefptr)CFBundleGetFunctionPointerForName(t_bundle, CFSTR("releasedbconnectionref"));
    t_result -> setcallbacksptr = (set_callbacksrefptr)CFBundleGetFunctionPointerForName(t_bundle, CFSTR("setcallbacksref"));
	
	return t_result;
}

void FreeDatabaseDriver( DATABASEREC *tdatabaserec)
{
	UnloadDBBundle(tdatabaserec -> driverref);
}
#else

#include <dlfcn.h>

DATABASEREC *DoLoadDatabaseDriver(const char *p_path)
{
	char *t_filename;
	t_filename = NULL;
	
	if (t_filename == NULL)
	{
		t_filename = (char *)malloc((sizeof(char) * strlen(p_path)) + 4);
		sprintf(t_filename, "%s.dylib", p_path);
	}
	
	void *t_driver_handle;
	t_driver_handle = dlopen(t_filename, RTLD_NOW);
	
	if (t_driver_handle == NULL)
	{
		free(t_filename);
		return NULL;
	}
	
	DATABASEREC *t_result;
	t_result = new DATABASEREC;
	
	t_result -> driverref = t_driver_handle;
	t_result -> idcounterptr = (idcounterrefptr)dlsym(t_driver_handle, "setidcounterref");
	t_result -> newconnectionptr = (new_connectionrefptr)dlsym(t_driver_handle, "newdbconnectionref");
	t_result -> releaseconnectionptr = (release_connectionrefptr)dlsym(t_driver_handle, "releasedbconnectionref");
    t_result -> setcallbacksptr = (set_callbacksrefptr)dlsym(t_driver_handle, "setcallbacksref");
	free(t_filename);
	return t_result;
	
}

const char *GetExternalFolder(void)
{
	static const char *s_folder = NULL;
	if (s_folder == NULL)
	{
		Dl_info t_info;
		if (dladdr((void *)GetExternalFolder, &t_info) == 0)
			return NULL;
		
		s_folder = strdup(t_info . dli_fname);
		strrchr(s_folder, '/')[0] = '\0';
	}
	return s_folder;
}

const char *GetApplicationFolder(void)
{
	return NULL;
}

void FreeDatabaseDriver( DATABASEREC *tdatabaserec)
{
	dlclose(tdatabaserec -> driverref);
}

#endif

void MCU_path2std(char *p_path)
{
  if (p_path == NULL || !*p_path)
    return;

  do 
  {
	  if (*p_path == '/')
	  {
		  *p_path = ':';
	  }
	  else
	  {
		  if (*p_path == ':')
			  *p_path = '/';
	  }
  } while (*++p_path);
}

void MCU_path2native(char *dptr)
{
  if (dptr == NULL || !*dptr)
    return;
}


void MCU_fix_path(char *cstr)
{
  char *fptr = cstr;
  while (*fptr) 
  {
    if (*fptr == '/' && *(fptr + 1) == '.' && *(fptr + 2) == '.' && *(fptr + 3) == '/') 
	{
		if (fptr == cstr)
		  strcpy(fptr, fptr + 3);
		else
		{
			char *bptr = fptr - 1;
			while (True)
			{
				if (*bptr == '/')
				{
					strcpy(bptr, fptr + 3);
					fptr = bptr;
					break;
				}
				else
					bptr--;
			}
		}
	}
    else
      if (*fptr == '/' && *(fptr + 1) == '.' && *(fptr + 2) == '/')
		  strcpy(fptr, fptr + 2);
	  else
		  if (*fptr == '/' && *(fptr + 1) == '/')
			  strcpy(fptr, fptr + 1);
		  else
			  fptr++;
  }
}

#define strclone strdup

char *MCS_getcurdir()
{
	char *t_path = new char[PATH_MAX + 2];
	getcwd(t_path, PATH_MAX);
	return t_path;
}

char *MCS_resolvepath(const char *p_path)
{
  char *cstr;

  if (p_path == NULL) 
  {
    char *tpath = MCS_getcurdir();
    return tpath;
  }

  char *tildepath;
  if (p_path[0] == '~') {
    char *tpath = strclone(p_path);
    char *tptr = strchr(tpath, '/');
    if (tptr == NULL) {
      tpath[0] = '\0';
      tptr = tpath;
    }
    else
      *tptr++ = '\0';
    
    struct passwd *pw;
    if (*(tpath + 1) == '\0')
      pw = getpwuid(getuid());
    else
      pw = getpwnam(tpath + 1);
    if (pw == NULL)
      return NULL;
    tildepath = new char[strlen(pw->pw_dir) + strlen(tptr) + 2];
    strcpy(tildepath, pw->pw_dir);
    if (*tptr) {
      strcat(tildepath, "/");
      strcat(tildepath, tptr);
    }
    delete tpath;
  }
  else
    tildepath = strclone(p_path);
  if (p_path[0] != '/') {
    cstr = MCS_getcurdir();
    if (strlen(cstr) + strlen(tildepath) + 2 < PATH_MAX) {
      strcat(cstr, "/");
      strcat(cstr, p_path);
    }
    delete tildepath;
    tildepath = cstr;
  }
  struct stat buf;
  if (lstat(tildepath, (struct stat *)&buf) != 0 || !S_ISLNK(buf.st_mode))
    return tildepath;
  int size;
  char *newname = new char[PATH_MAX + 2];
  if ((size = readlink(tildepath, newname, PATH_MAX)) < 0) {
    delete tildepath;
    delete newname;
    return NULL;
  }
  delete tildepath;
  newname[size] = '\0';
  if (newname[0] != '/') {
    char *fullpath = new char[strlen(p_path) + strlen(newname) + 2];
    strcpy(fullpath, p_path);
    char *sptr = strrchr(fullpath, '/');
    if (sptr == NULL)
      sptr = fullpath;
    else
      sptr++;
    strcpy(sptr, newname);
    delete newname;
    newname = MCS_resolvepath(fullpath);
    delete fullpath;
  }
  return newname;

}
