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

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"

bool MCPickleRead(MCStreamRef stream, MCPickleRecordInfo *p_info, MCValueRef*& r_value_pool, uindex_t& r_value_pool_size, void *r_record)
{
    return false;
}

bool MCPickleWrite(MCStreamRef stream, MCPickleRecordInfo *p_info, MCValueRef *p_value_pool, uindex_t p_value_pool_size, void *p_record)
{
    return false;
}
