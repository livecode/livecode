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

#ifndef __MC_HANDLER__
#define	__MC_HANDLER__

struct MCExecValue;

// A single variable definition. If 'init' is nil then it means the var should
// be created as a uql.
struct MCHandlerVarInfo
{
	MCNameRef name;
	MCValueRef init;
};

struct MCHandlerParamInfo
{
	MCNameRef name;
	bool is_reference : 1;
};

struct MCHandlerConstantInfo
{
	MCNameRef name;
	MCValueRef value;
};

class MCHandler
{
	MCHandlerlist *hlist;
	MCStatement *statements;
	MCVariable **vars;
	MCVariable **globals;
	MCContainer **params;
	MCParameter *paramlist;
	MCHandlerVarInfo *vinfo;
	MCHandlerParamInfo *pinfo;
	MCHandlerConstantInfo *cinfo;
	uint2 nglobals;
	uint2 npassedparams;
	uint2 nparams;
	uint2 nvnames;
	uint2 npnames;
	uint2 nconstants;
	uint2 executing;
	uint2 firstline;
	uint2 lastline;
	// MW-2011-06-22: [[ SERVER ]] This is the index of the file that this
	//   handler came from, it was loaded in server-script mode.
	uint2 fileindex;
	MCNameRef name;
	Boolean prop;
	Boolean array;
	Boolean is_private;
	uint1 type;
	
	// MW-2013-11-08: [[ RefactorIt ]] The 'it' variable is now always defined
	//   and this varref is used by things that want to set it.
	MCVarref *m_it;
	
	static Boolean gotpass;
public:
	MCHandler(uint1 htype, bool p_is_private = false);
	~MCHandler();

	MCNameRef getname(void)
	{
		return name;
	}
	
	const char *getname_cstring(void)
	{
        char *t_name;
        /* UNCHECKED */ MCStringConvertToCString(MCNameGetString(name), t_name);
		return t_name;
	}
	
	bool hasname(MCNameRef other_name)
	{
		return MCNameIsEqualTo(name, other_name, kMCCompareCaseless);
	}

	Parse_stat parse(MCScriptPoint &sp, Boolean isprop);
#ifdef LEGACY_EXEC
	Exec_stat exec(MCExecPoint &, MCParameter *);
#endif
    Exec_stat exec(MCExecContext &, MCParameter *);
	void compile(MCSyntaxFactoryRef factory);
	
    MCVariable *getvar(uint2 index, Boolean isparam);
    MCContainer *getcontainer(uint2 index, Boolean isparam);
    
	integer_t getnparams(void);
    MCValueRef getparam(uindex_t p_index);
	Parse_stat findvar(MCNameRef name, MCVarref **);
	Parse_stat newvar(MCNameRef name, MCValueRef init, MCVarref **);
	Parse_stat findconstant(MCNameRef name, MCExpression **);
	Parse_stat newconstant(MCNameRef name, MCValueRef value);
	void newglobal(MCNameRef name);
	bool getparamnames(MCListRef& r_list);
	bool getvariablenames(MCListRef& r_list);
	bool getglobalnames(MCListRef& r_list);
	bool getvarnames(bool p_all, MCListRef& r_list);
#ifdef LEGACY_EXEC
	Exec_stat getvarnames(MCExecPoint &, Boolean all);
#endif
	void eval_ctxt(MCExecContext& ctxt, MCStringRef p_expression, MCExecValue& r_value);
	void eval(MCExecContext& ctxt, MCStringRef p_expression, MCValueRef& r_value);
	//Exec_stat eval(MCExecPoint &);
	uint4 linecount();
	void deletestatements(MCStatement *statements);
	void doscript(MCExecContext& ctxt, MCStringRef p_script, uinteger_t p_line = 0, uinteger_t p_pos = 0);

	// Used by the externals API, this method returns the current incarnation of
	// the 'it' variable in this handler - if any.
	MCVarref *getit(void);

	void clearpass()
	{
		gotpass = False;
	}
	Boolean getpass() const
	{
		return gotpass;
	}
	Handler_type gettype() const
	{
		return (Handler_type)type;
	}
	uint2 getstartline(void) const
	{
		return firstline;
	}
	uint2 getendline(void) const
	{
		return lastline;
	}
	void setfileindex(uint2 p_fileindex)
	{
		fileindex = p_fileindex;
	}
	uint2 getfileindex(void) const
	{
		return fileindex;
	}
	bool isprivate(void) const
	{
		return is_private == True;
	}

	void getvarlist(MCVariable**& r_vars, uint32_t& r_var_count)
	{
		r_vars = vars;
		r_var_count = nvnames;
	}
	
	void getparamlist(MCContainer**& r_vars, uint32_t& r_param_count)
	{
		r_vars = params;
		r_param_count = npnames;
	}
	
	void getgloballist(MCVariable**& r_vars, uint32_t& r_var_count)
	{
		r_vars = globals;
		r_var_count = nglobals;
	}
	
	void sethlist(MCHandlerlist *p_list)
	{
		hlist = p_list;
	}

	// OK-2010-01-14: [[Bug 6506]] - These two methods needed to support global watchedVariables
	uint2 getnglobals(void) const
	{
		return nglobals;
	}

	MCVariable *getglobal(uint2 p_index) const
	{
		return globals[p_index];
	}

private:
	Parse_stat newparam(MCScriptPoint& sp);
};
#endif
