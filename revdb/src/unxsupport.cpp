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
// Unix support function implementations

#include "unxsupport.h"
#include <stdlib.h>

void MCU_path2std(char *p_path)
{
  if (p_path == NULL || !*p_path)
    return;

  do 
  {
	 if (*p_path == '/')
	 {
      *p_path = '\\';
	 }
    else
	{
      if (*p_path == '\\')
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
		  if (fptr != cstr && *fptr == '/' && *(fptr + 1) == '/')
			  strcpy(fptr, fptr + 1);
	else
	  fptr++;
  }
}

char *MCS_getcurdir()
{
  char *t_path = new (nothrow) char[PATH_MAX + 2];
  getcwd(t_path, PATH_MAX);
  return t_path;
}

#define strclone strdup

char *MCS_resolvepath(const char *path)
{				
  if (path == NULL)
  {
    char *tpath = MCS_getcurdir();
    return tpath;
  }

  char *tildepath;
  if (path[0] == '~') 
  {
    char *tpath = strclone(path);
    char *tptr = strchr(tpath, '/');
    if (tptr == NULL)
	{
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
    tildepath = static_cast<char*>(malloc(sizeof(*tildepath) *
                                          (strlen(pw->pw_dir) + strlen(tptr) + 2)));
    strcpy(tildepath, pw->pw_dir);
    if (*tptr) {
      strcat(tildepath, "/");
      strcat(tildepath, tptr);
    }
    free(tpath);
  }
  else
    tildepath = strclone(path);
#ifdef SCO
  struct scoutsname scostruct;
  if (__scoinfo(&scostruct, sizeof(scostruct)))
    return tildepath;
  if (scostruct.release[0] < '3'
      || scostruct.release[2] < '2'
      || scostruct.release[4] < '4')
    return tildepath;
#endif
  struct stat buf;
  if (lstat(tildepath, &buf) != 0 || !S_ISLNK(buf.st_mode))
    return tildepath;
  int size;
  char *newname = new (nothrow) char[PATH_MAX + 2];

  if ((size = readlink(tildepath, newname, PATH_MAX)) < 0) {
      free(tildepath);
    delete[] newname;
    return NULL;
  }
  free(tildepath);
  newname[size] = '\0';
  if (newname[0] != '/') {
    char *fullpath = new (nothrow) char[strlen(path) + strlen(newname) + 2];
    strcpy(fullpath, path);
    char *sptr = strrchr(fullpath, '/');
    if (sptr == NULL)
      sptr = fullpath;
    else
      sptr++;
    strcpy(sptr, newname);
    delete[] newname;
    newname = MCS_resolvepath(fullpath);
    delete[] fullpath;
  }
  return newname;
}

