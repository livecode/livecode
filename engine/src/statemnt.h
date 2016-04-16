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
// MCHandler class declarations
//
#ifndef	STATEMENT_H
#define	STATEMENT_H

class MCScriptPoint;
class MCParameter;
class MCChunk;
class MCExpression;
class MCVarref;
class MCHandler;

#ifdef FEATURE_PROFILE
typedef void (*MCProfilingReportCallback)(uint2 line, uint64_t count);

class MCProfilingTimer
{
public:
    MCProfilingTimer(void)
    {
        m_total_count = 0;
        m_start_count = 0;
    }
    
    void Start(void)
    {
        m_start_count = ReadCount();
    }
    
    void Stop(void)
    {
        uint64_t t_finish_count;
        t_finish_count = ReadCount();
        m_total_count += t_finish_count - m_start_count;
    }
    
    uint64_t Total(void) const
    {
        return m_total_count;
    }
    
private:
    uint64_t ReadCount(void)
    {
        uint32_t high, low;
        asm volatile ("rdtsc" : "=a" (low), "=d" (high) : : "memory");
        return (((uint64_t)high) << 32) | low;
    }
    
    uint64_t m_total_count;
    uint64_t m_start_count;
};
#endif

class MCStatement
{
protected:
	uint2 line;
	uint2 pos;
	MCStatement *next;
public:
	MCStatement();
	
	virtual ~MCStatement();
	virtual Parse_stat parse(MCScriptPoint &);
#ifdef LEGACY_EXEC
	virtual Exec_stat exec(MCExecPoint &);
#endif
	virtual void exec_ctxt(MCExecContext&);
	virtual void compile(MCSyntaxFactoryRef factory);
	virtual uint4 linecount();
	
	void setnext(MCStatement *n)
	{
		next = n;
	}
	MCStatement *getnext()
	{
		return next;
	}
	uint4 countlines(MCStatement *stmp);
	void deletestatements(MCStatement *start);
	void deletetargets(MCChunk **targets);
	Parse_stat gettargets(MCScriptPoint &, MCChunk **targets, Boolean forset);
	Parse_stat getparams(MCScriptPoint &, MCParameter **params);
	Parse_stat getmods(MCScriptPoint &, uint2 &mstate);
	Parse_stat gettime(MCScriptPoint &sp, MCExpression **in, Functions &units);
	void initpoint(MCScriptPoint &);
	uint2 getline()
	{
		return line;
	}
	uint2 getpos()
	{
		return pos;
	}
    
#ifdef FEATURE_PROFILE
    void execute(MCExecContext& ctxt)
    {
        starttiming();
        exec_ctxt(ctxt);
        finishtiming();
    }
    
    virtual void starttiming(void);
    virtual void finishtiming(void);
    virtual void reporttiming(MCProfilingReportCallback report);
#else
    void execute(MCExecContext& ctxt)
    {
        exec_ctxt(ctxt);
    }
#endif

protected:
#ifdef FEATURE_PROFILE
    MCProfilingTimer timer;
#endif
};

class MCComref : public MCStatement
{
	MCNameRef name;
    MCHandler *handler;
	MCParameter *params;
	bool resolved : 1;
    bool global_handler : 1;
public:
	MCComref(MCNameRef n);
	virtual ~MCComref();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext&);
};

#endif
