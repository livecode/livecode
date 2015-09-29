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

#ifndef __MC_EXEC__
#define __MC_EXEC__

#ifndef __MC_CORE__
#include "core.h"
#endif

#ifndef __MC_EXECPT__
#include "execpt.h"
#endif

#ifndef OBJDEFS_H
#include "objdefs.h"
#endif

#ifndef OBJECT_H
#include "object.h"
#endif


class MCExecContext
{
public:
	MCExecContext(MCExecPoint& ep)
		: m_ep(ep)
	{
		m_stat = ES_NORMAL;
	}
	
	MCExecPoint& GetEP(void)
	{
		return m_ep;
	}
	
    // MM-2011-02-16: Added ability to get handle of current object
    MCObjectHandle *GetObjectHandle(void)
    {
        extern MCExecPoint *MCEPptr;
		return MCEPptr->getobj()->gethandle();
    }
    
	Exec_stat GetStat(void)
	{
		return m_stat;
	}
	
	void SetTheResultToEmpty(void)
	{
		MCresult -> clear();
	}
	
	void SetTheResultToStaticCString(const char *p_cstring)
	{
		MCresult -> sets(p_cstring);
	}
	
    void SetTheResultToNumber(real64_t p_value)
    {
        MCresult -> setnvalue(p_value);
    }
    
    void GiveCStringToResult(char *p_cstring)
    {
        MCresult -> grab(p_cstring, MCCStringLength(p_cstring));
    }
    
    void SetTheResultToCString(const char *p_string)
    {
        MCresult -> copysvalue(p_string);
    }
    
private:
	MCExecPoint& m_ep;
	Exec_stat m_stat;
};

class MCAutoRawMemoryBlock
{
public:
	MCAutoRawMemoryBlock(void)
	{
		m_block = nil;
	}
	
	~MCAutoRawMemoryBlock(void)
	{
		MCMemoryDeallocate(m_block);
	}
	
	void Give(void *p_block)
	{
		assert(m_block == nil);
		m_block = p_block;
	}
	
	const void *Borrow(void)
	{
		return m_block;
	}
	
	void *Take(void)
	{
		void *t_block;
		t_block = m_block;
		m_block = nil;
		return t_block;
	}
	
	operator void *& (void)
	{
		assert(m_block == nil);
		return m_block;
	}
	
	operator const void *(void) const
	{
		return m_block;
	}
	
private:
	MCAutoRawMemoryBlock(const MCAutoRawMemoryBlock&) {}
	
	void *m_block;
};	

class MCAutoRawCString
{
public:
	MCAutoRawCString(void)
	{
		m_cstring = nil;
	}
	
	~MCAutoRawCString(void)
	{
		MCCStringFree(m_cstring);
	}
	
	void Give(char *p_string)
	{
		assert(m_cstring == nil);
		m_cstring = p_string;
	}
	
	const char *Borrow(void)
	{
		return m_cstring;
	}
	
	char *Take(void)
	{
		char *t_cstring;
		t_cstring = m_cstring;
		m_cstring = nil;
		return t_cstring;
	}
	
	operator char *& (void)
	{
		assert(m_cstring == nil);
		return m_cstring;
	}
	
	operator const char *(void) const
	{
		return m_cstring;
	}
	
private:
	MCAutoRawCString(const MCAutoCString&) {}
	
	char *m_cstring;
};

#endif
