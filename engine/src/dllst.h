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

#include "foundation-legacy.h"

// Forward declarations
class MCObject;

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
	virtual void removelink(class MCObject *optr);
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
    
    // This function is called on all 'IO_stat' return in load methods. It
    // provides an easy hook for a breakpoint to work out why a particular file
    // failed to load.
    inline static IO_stat checkloadstat(IO_stat stat)
    {
        if (stat != IO_NORMAL)
        {
            return stat;
        }
        return stat;
    }

#ifdef  _DEBUG_MALLOC_INC
	void verify(char *where);
#endif
};

// Define to unify some boilerplate for MCDLlist-derived classes
//      MCDLlistAdaptorFunctions(ClassName);
#define MCDLlistAdaptorFunctions(T) \
    T* next()                           { return static_cast<T*>(this->MCDLlist::next()); } \
    T* prev()                           { return static_cast<T*>(this->MCDLlist::prev()); } \
    const T* next() const               { return static_cast<const T*>(this->MCDLlist::next()); } \
    const T* prev() const               { return static_cast<const T*>(this->MCDLlist::prev()); } \
    void totop(T*& list)                { MCDLlist* l = list; this->MCDLlist::totop(l); list = static_cast<T*>(l); } \
    void insertto(T*& list)             { MCDLlist* l = list; this->MCDLlist::insertto(l); list = static_cast<T*>(l); } \
    void appendto(T*& list)             { MCDLlist* l = list; this->MCDLlist::appendto(l); list = static_cast<T*>(l); } \
    void append(T* node)                { this->MCDLlist::append(node); } \
    void splitat(T* node)               { this->MCDLlist::splitat(node); } \
    T* remove(T*& list)                 { MCDLlist *l = list, *r; r = this->MCDLlist::remove(l); list = static_cast<T*>(l); return static_cast<T*>(r); } \

#endif
