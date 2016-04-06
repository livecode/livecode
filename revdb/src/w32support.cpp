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
// Windows support function implementations

#include "w32support.h"

HINSTANCE hInstance = NULL;

char *GetModuleFolder(HINSTANCE p_module)
{
	char t_path[MAX_PATH];
	if (GetModuleFileNameA(p_module, t_path, MAX_PATH) == 0)
		return NULL;

	char *t_last_separator;
	t_last_separator = strrchr(t_path, '\\');
	if (t_last_separator != NULL)
		*t_last_separator = '\0';

	for(int n = 0; t_path[n] != '\0'; ++n)
		if (t_path[n] == '\\')
			t_path[n] = '/';

	return istrdup(t_path);
}

const char *GetExternalFolder(void)
{
	static char *s_folder = NULL;

	if (s_folder == NULL)
		s_folder = GetModuleFolder(hInstance);

	return s_folder;
}

const char *GetApplicationFolder(void)
{
	static char *s_folder = NULL;

	if (s_folder != NULL)
		s_folder = GetModuleFolder(GetModuleHandle(NULL));

	return s_folder;
}

DATABASEREC *DoLoadDatabaseDriver(const char *p_path)
{
	char t_win_path[MAX_PATH];

	strcpy(t_win_path, p_path);
	for(int n = 0; t_win_path[n] != '\0'; ++n)
	{
		if (t_win_path[n] == '/')
			t_win_path[n] = '\\';
	}

	HMODULE t_module;
	t_module = LoadLibraryA(t_win_path);
	if (t_module == NULL)
		return NULL;

	DATABASEREC *t_result;
	t_result = new DATABASEREC;
	t_result -> driverref = t_module;
	t_result -> idcounterptr = (idcounterrefptr)GetProcAddress(t_module, "setidcounterref");
	t_result -> newconnectionptr = (new_connectionrefptr)GetProcAddress(t_module, "newdbconnectionref");
	t_result -> releaseconnectionptr = (release_connectionrefptr)GetProcAddress(t_module, "releasedbconnectionref");
    t_result -> setcallbacksptr = (set_callbacksrefptr)GetProcAddress(t_module, "setcallbacksref");
	return t_result;
}

void FreeDatabaseDriver( DATABASEREC *tdatabaserec)
{
	FreeLibrary(tdatabaserec->driverref);
}

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

void MCU_path2native(char *p_path)
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

char *MCS_getcurdir(void)
{
  char *t_path = new char[PATH_MAX + 2];
  GetCurrentDirectory(PATH_MAX +1, (LPTSTR)t_path);
  MCU_path2std(t_path);
  return t_path;
}

#define strclone istrdup

char *MCS_resolvepath(const char *p_path)
{				
  if (p_path == NULL)
  {
    char *t_path = MCS_getcurdir();
    MCU_path2native(t_path);
    return t_path;
  }

  char *cstr = strclone(p_path);
  MCU_path2native(cstr);
  return cstr;
}


extern void REVDB_INIT(void);
extern void REVDB_QUIT(void);

BOOL WINAPI DllMain(HINSTANCE tInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
#ifdef _DEBUG
	int t_dbg_flags;
	t_dbg_flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	t_dbg_flags |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(t_dbg_flags);
#endif

		hInstance = tInstance;
		REVDB_INIT();
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		REVDB_QUIT();

#ifdef _DEBUG
		_CrtDumpMemoryLeaks();
#endif
	}
	return TRUE;
}
