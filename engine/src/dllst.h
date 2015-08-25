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

//
// Base double linked list class
//
#ifndef	DLLIST_H
#define	DLLIST_H

class MCDLlist
{
protected:
	MCDLlist *nptr;
	MCDLlist *pptr;
public:
	MCDLlist()
	{
		pptr = nptr = this;
	}
	MCDLlist(const MCDLlist &dref)
	{
		nptr = pptr = this;
	}
	virtual ~MCDLlist();
	// shared by buttons and text blocks
	virtual void removelink(MCObject *optr);
	MCDLlist *next()
	{
		return nptr;
	}
	MCDLlist *prev()
	{
		return pptr;
	}
	const MCDLlist *next() const
	{
		return nptr;
	}
	const MCDLlist *prev() const
	{
		return pptr;
	}
	void totop(MCDLlist *&list);
	void insertto(MCDLlist *&list);
	void appendto(MCDLlist *&list);
	void append(MCDLlist *node);
	void splitat(MCDLlist *node);
	MCDLlist *remove(MCDLlist *&list);

#ifdef  _DEBUG_MALLOC_INC
	void verify(char *where);
#endif
};
#endif
