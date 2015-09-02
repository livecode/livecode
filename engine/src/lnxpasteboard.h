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

#ifndef X_PASTEBOARD_H
#define X_PASTEBOARD_H





class MCXPasteboard : public MCPasteboard
{
public:
	MCXPasteboard ( Atom p_public_atom, Atom p_private_atom, MCXTransferStore * p_store ) ;
	
	void Retain(void) ;
	void Release(void) ;
	bool Query(MCTransferType*& r_types, unsigned int& r_type_count) ;
	bool Fetch(MCTransferType p_type, MCSharedString*& r_data) ;
	bool Fetch_MIME(MCMIMEtype *p_type, MCSharedString*& r_data);
	
	void SetWindows ( Window p_source_window , Window p_target_window ) ;
	
private:
		
	Atom m_public_atom ; 
	Atom m_private_atom ;
	Window m_source_window ;
	Window m_target_window ;
	Time m_lock_time ;
	
	// The number of references to this object
	unsigned int m_references;

	unsigned int m_type_cound ;
	MCTransferType *m_types;

	bool m_valid;
	
	// Pointer to the data store that this pasteboard acts upon.
	MCXTransferStore * m_transfer_store ;
};




#endif 
