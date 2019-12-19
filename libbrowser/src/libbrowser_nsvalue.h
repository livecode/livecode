/* Copyright (C) 2019 LiveCode Ltd.
 
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

#ifndef __LIBBROWSER_NSVALUE_H__
#define __LIBBROWSER_NSVALUE_H__

#include "libbrowser.h"
#include "libbrowser_internal.h"

typedef bool (*MCCustomNSObjectConverter)(id p_obj, MCBrowserValue &r_value);

bool MCNSArrayToBrowserList(NSArray *p_array, MCBrowserListRef &r_list, MCCustomNSObjectConverter p_converter = nil);

bool MCNSNumberToBrowserValue(NSNumber *p_number, MCBrowserValue &r_value);
bool MCNSObjectToBrowserValue(id p_obj, MCBrowserValue &r_value, MCCustomNSObjectConverter p_converter = nil);
bool MCNSDictionaryToBrowserValue(NSDictionary *p_dictionary, MCBrowserValue &r_value, MCCustomNSObjectConverter p_converter = nil);
bool MCNSArrayToBrowserValue(NSArray * p_array, MCBrowserValue &r_value, MCCustomNSObjectConverter p_converter = nil);

#endif // __LIBBROWSER_NSVALUE_H__
