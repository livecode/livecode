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
// MCHandlerlist class declarations
//
#ifndef	HANDLERLIST_H
#define	HANDLERLIST_H

#include "handler.h"

// The MCHandlerArray class wraps a list of MCHandler* pointers and provides log(n)
// searching for a handler using binary search. This is an improvement over the previous
// method which just iterated through a linked-list.
//
class MCHandlerArray
{
public:
	MCHandlerArray(void);
	~MCHandlerArray(void);

	// Destroy the list of handlers.
	void clear(void);

	// Sort the list of handlers ready for finding.
	void sort(void);

	// Find the handler with the given name.
	MCHandler *find(MCNameRef name);

	// Return the list of handler pointers.
	MCHandler **get(void)
	{
		return m_handlers;
	}

	// Return the number of handlers.
	uint32_t count(void)
	{
		return m_count;
	}

	// Add a handler to the list.
	void append(MCHandler *p_handler);

	// Search the array for a handler with the given name linearly.
	// This is used for validity checks and does not assume the list
	// is already sorted.
	bool exists(MCNameRef name);

private:
	uint32_t m_count;
	MCHandler **m_handlers;

	static int compare_handler(const void *a, const void *b);
};

typedef bool (*MCHandlerlistListConstantsCallback)(void *p_context, MCHandlerConstantInfo *info);
typedef bool (*MCHandlerlistListVariablesCallback)(void *p_context, MCVariable *p_variable);
typedef bool (*MCHandlerlistListHandlersCallback)(void *p_context, Handler_type p_type, MCHandler* p_handler, bool p_include_all);

class MCHandlerlist
{
	// MW-2012-08-08: [[ BeforeAfter ]] The before/after handlers are stored
	//   in separate arrays - indices 4 and 5.
	// Store the handlers in six parallel arrays. The correct array to use
	// is given by (handlertype - 1).
	MCHandlerArray handlers[6];

	MCObject *parent;
	MCVariable *vars;
	MCVariable **globals;
	MCHandlerConstantInfo *cinfo;
	uint2 nglobals;
	uint2 nconstants;

	// MW-2008-10-28: [[ ParentScripts ]] We keep track of the number of script
	//   local vars so we can store the index in the MCVarrefs for when we
	//   execute in parent script context.
	uint2 nvars;

	// MW-2008-10-28: [[ ParentScripts ]] We keep track of the initializers for
	//   the script locals so we can initialize the vars correctly when a use
	//   is used.
	MCValueRef *vinits;

	// MW-2008-10-28: [[ ParentScripts ]] Store the old variables in a vector
	//   rather than a list. Once a variable has been re-used when re-compiling
	//   its entry will be set to NULL. This change means we preserve the index
	//   for variable preservation in parentScripts.
	static MCVariable **s_old_variables;
	static uint32_t s_old_variable_count;

	// MW-2008-10-28: [[ ParentScripts ]] This is used when re-parsing if
	//   preserveVariables is set. It is used to rework the list of variables
	//   on a per-use basis of a parentscript. It is a mapping of old var
	//   index to new var index.
	static uint32_t *s_old_variable_map;

public:
	MCHandlerlist();
	~MCHandlerlist();
	void reset(void);
	MCObject *getparent();
	// MW-2011-08-23: [[ UQL ]] 'ignore_uql' ignores UQL vars when searching.
    //   This is used when going from handler to script scope for var searches.
	Parse_stat findvar(MCNameRef name, bool ignore_uql, MCVarref **);
	Parse_stat newvar(MCNameRef name, MCValueRef init, MCVarref **, Boolean initialised);
	Parse_stat findconstant(MCNameRef name, MCExpression **);
	Parse_stat newconstant(MCNameRef name, MCValueRef value);
	bool getlocalnames(MCListRef& r_list);
	bool getglobalnames(MCListRef& r_list);
    bool getconstantnames(MCListRef& r_list);
    void appendglobalnames(MCStringRef& r_string, bool first);
	void newglobal(MCNameRef name);
	
    Parse_stat parse(MCObject *, MCDataRef);
    Parse_stat parse(MCObject *, MCStringRef);
	
	Exec_stat findhandler(Handler_type, MCNameRef name, MCHandler *&);
	bool hashandler(Handler_type type, MCNameRef name);
	void addhandler(Handler_type type, MCHandler *handler);

	uint2 getnglobals(void);
	MCVariable *getglobal(uint2 p_index);
    bool enumerate(MCExecContext& ctxt, bool p_include_private, bool p_first, uindex_t& r_count, MCStringRef*& r_handlers);
	
	bool listconstants(MCHandlerlistListConstantsCallback p_callback, void *p_context);
	bool listhandlers(MCHandlerlistListHandlersCallback p_callback, void *p_context, bool p_include_all);
    bool listvariables(MCHandlerlistListVariablesCallback p_callback, void *p_context);
    bool listglobals(MCHandlerlistListVariablesCallback p_callback, void *p_context);
    
	uint2 getnvars(void)
	{
		return nvars;
	}

	MCVariable *getvars(void)
	{
		return vars;
	}

	MCValueRef *getvinits(void)
	{
		return vinits;
	}

	Boolean hashandlers()
	{
		return handlers[0] . count() != 0;
	}
	Boolean hasvars()
	{
		return vars != NULL;
	}
	uint4 linecount();
};
#endif
