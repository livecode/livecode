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

#define _WIN32_WINNT 0x0500
#define NTDDI_VERSION 0x05000400

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

// w32dcw32
#include <winsock2.h>

// w32ans
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_VISTA
#include <shobjidl.h>
#include <shlobj.h>
#include <shlwapi.h>
#undef NTDDI_VERSION
#define NTDDI_VERSION 0x05000400

// w32clipboard
#include <objidl.h>

// w32color
#include <icm.h>

// w32dcs
#include <mmsystem.h>

// w32dnd
#include <shlguid.h>

// w32icon
#include <shellapi.h>

// w32printer
#include <commdlg.h>
#include <cderr.h>
#include <winspool.h>

//
extern HINSTANCE MChInst;

#undef GetCurrentTime
// Undef GetObject because GetObjectW was called instead of MCExecContext::GetObject()
#undef GetObject
