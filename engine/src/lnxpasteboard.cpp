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
#include "execpt.h" 
#include "dispatch.h"
#include "image.h" 
#include "globals.h"

#include "lnxdc.h"
#include "lnxtransfer.h" 
#include "lnxpasteboard.h"



MCXPasteboard::MCXPasteboard ( Atom p_public_atom, Atom p_private_atom, MCXTransferStore * p_transfer_store ) 
{
	m_references = 1 ;
	m_private_atom = p_private_atom ;
	m_public_atom = p_public_atom ;
	m_lock_time = LastEventTime ;
	m_transfer_store = p_transfer_store ;
}

void MCXPasteboard::SetWindows ( Window p_source_window, Window p_target_window ) 
{
	m_source_window = p_source_window ;
	m_target_window = p_target_window ;
}


void MCXPasteboard::Retain(void)
{
	m_references += 1;
}


void MCXPasteboard::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
		delete this;
}



bool MCXPasteboard::Query(MCTransferType*& r_types, unsigned int& r_type_count)
{

	return ( m_transfer_store -> Query ( r_types, r_type_count ) ) ;
  	
}


bool MCXPasteboard::Fetch_MIME(MCMIMEtype *p_type, MCSharedString*& r_data)
{
	bool t_success ;
	r_data = NULL ;
	t_success = m_transfer_store -> Fetch ( p_type, r_data, m_public_atom, m_private_atom, m_source_window, m_target_window, m_lock_time ) ;
	if ( t_success )
		r_data -> Retain() ;
	return ( t_success ) ;
}


bool MCXPasteboard::Fetch(MCTransferType p_type, MCSharedString*& r_data)
{
	bool t_success ;
	r_data = NULL ;
	t_success = m_transfer_store -> Fetch ( p_type, r_data, m_public_atom, m_private_atom, m_source_window, m_target_window, m_lock_time ) ;
	if ( t_success )
		r_data -> Retain() ;
	return ( t_success ) ;
}
