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

#ifndef __FILEPATH_H__
#define __FILEPATH_H__

#include "foundation.h"

extern bool MCPathIsRemoteURL(MCStringRef p_path);
extern bool MCPathIsAbsolute(MCStringRef p_path);
extern bool MCPathAppend(MCStringRef p_base, MCStringRef p_path, MCStringRef &r_new);

#endif//__FILEPATH_H__
