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

#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "transfer.h"
//#include "execpt.h" 
#include "dispatch.h"
#include "image.h" 
#include "globals.h"

#include "lnxdc.h"
#include "lnxtransfer.h" 
#include "lnxpasteboard.h"


MCGdkPasteboard::MCGdkPasteboard(GdkAtom p_atom, MCGdkTransferStore *p_transfer_store)
: m_atom(p_atom), m_source_window(NULL), m_target_window(NULL), m_lock_time(0),
    m_references(1), m_type_count(0), m_types(NULL), m_valid(false),
    m_transfer_store(p_transfer_store)
{
    ;
}

void MCGdkPasteboard::SetWindows(GdkWindow *p_source, GdkWindow *p_target)
{
    m_source_window = p_source;
    m_target_window = p_target;
}

void MCGdkPasteboard::Retain()
{
    m_references++;
}

void MCGdkPasteboard::Release()
{
    if (--m_references == 0)
        delete this;
}

bool MCGdkPasteboard::Query(MCTransferType* &r_types, size_t &r_type_count)
{
    return m_transfer_store->Query(r_types, r_type_count);
}

bool MCGdkPasteboard::Fetch_MIME(MCMIMEtype *p_type, MCDataRef &r_data)
{
    return m_transfer_store->Fetch(p_type, r_data, m_atom, m_source_window, m_target_window, m_lock_time);
}

bool MCGdkPasteboard::Fetch(MCTransferType p_type, MCDataRef &r_data)
{
    return m_transfer_store->Fetch(p_type, r_data, m_atom, m_source_window, m_target_window, m_lock_time);
}

