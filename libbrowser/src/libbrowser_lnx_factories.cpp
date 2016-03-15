/* Copyright (C) 2015 LiveCode Ltd.
 
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

#include <core.h>

#include "libbrowser_internal.h"

extern bool MCCefBrowserFactoryCreate(MCBrowserFactoryRef &r_factory);


// Overcome the tautological error if s_factory_list is a static pointer
MCBrowserFactoryMap kMCBrowserFactoryMap[] =
{
    { "CEF", nil, MCCefBrowserFactoryCreate },
    { nil, nil, nil },
};

MCBrowserFactoryMap* s_factory_list = kMCBrowserFactoryMap;

