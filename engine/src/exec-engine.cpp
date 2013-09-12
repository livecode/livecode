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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "scriptpt.h"
#include "mode.h"
#include "handler.h"
#include "osspec.h"
#include "uidc.h"
#include "license.h"
#include "debug.h"
#include "param.h"
#include "property.h"

#include "stack.h"
#include "card.h"

#include "exec.h"
#include "util.h"

#include "express.h"
#include "variable.h"
#include "chunk.h"
#include "securemode.h"
#include "dispatch.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EVAL_METHOD(Engine, Version, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, BuildNumber, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Platform, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Environment, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Machine, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Processor, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, SystemVersion, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, CommandNames, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, ConstantNames, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, FunctionNames, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, PropertyNames, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, GlobalNames, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, LocalNames, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, VariableNames, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Param, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, ParamCount, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Params, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Result, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, BackScripts, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, FrontScripts, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, PendingMessages, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Interrupt, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Me, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Target, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, TargetContents, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Owner, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, ScriptLimits, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, SysError, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, Value, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, ValueWithObject, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, Get, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, PutIntoVariable, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, PutOutput, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, Do, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, InsertScriptOfObjectInto, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, Quit, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, CancelMessage, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, DeleteVariable, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, DeleteVariableChunks, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, RemoveAllScriptsFrom, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, RemoveScriptOfObjectFrom, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, LoadExtension, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, UnloadExtension, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, WaitFor, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, WaitUntil, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, WaitWhile, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, StartUsingStack, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, StartUsingStackByName, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, StopUsingStack, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, StopUsingStackByName, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, Dispatch, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, Send, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, SendInTime, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, Call, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, LockErrors, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, LockMessages, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, UnlockErrors, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, UnlockMessages, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, Set, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Engine, ReturnValue, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, CaseSensitive, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, CaseSensitive, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, CenturyCutOff, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, CenturyCutOff, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, ConvertOctals, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, ConvertOctals, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, ItemDelimiter, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, ItemDelimiter, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, LineDelimiter, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, LineDelimiter, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, ColumnDelimiter, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, ColumnDelimiter, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, RowDelimiter, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, RowDelimiter, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, WholeMatches, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, WholeMatches, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, UseSystemDate, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, UseSystemDate, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, UseUnicode, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, UseUnicode, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, NumberFormat, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, NumberFormat, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, ScriptExecutionErrors, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, ScriptParsingErrors, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, AllowInterrupts, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, AllowInterrupts, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, ExplicitVariables, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, ExplicitVariables, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, PreserveVariables, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, PreserveVariables, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, StackLimit, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, EffectiveStackLimit, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, StackLimit, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, SecureMode, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, SecureMode, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, SecurityCategories, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, SecurityPermissions, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, RecursionLimit, 1)
MC_EXEC_DEFINE_SET_METHOD(Engine, RecursionLimit, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, Address, 1)
MC_EXEC_DEFINE_GET_METHOD(Engine, StacksInUse, 1)

MC_EXEC_DEFINE_EVAL_METHOD(Engine, ValueAsObject, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, OwnerAsObject, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, TemplateAsObject, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, MeAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, MenuObjectAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, TargetAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Engine, ErrorObjectAsObject, 1)

////////////////////////////////////////////////////////////////////////////////

struct MCEngineNumberFormat
{
	uint2 fw;
	uint2 trailing;
	uint2 force;
};

static void MCEngineNumberFormatParse(MCExecContext& ctxt, MCStringRef p_input, MCEngineNumberFormat& r_output)
{
	MCU_setnumberformat(p_input, r_output . fw, r_output . trailing, r_output . force);
}

static void MCEngineNumberFormatFormat(MCExecContext& ctxt, const MCEngineNumberFormat& p_input, MCStringRef& r_output)
{
	if (MCU_getnumberformat(p_input . fw, p_input . trailing, p_input . force, r_output))
		return;

	ctxt . Throw();
}

static void MCEngineNumberFormatFree(MCExecContext& ctxt, MCEngineNumberFormat& p_input)
{
}

static MCExecCustomTypeInfo _kMCEngineNumberFormatTypeInfo =
{
	"Engine.NumberFormat",
	sizeof(MCEngineNumberFormat),
	(void *)MCEngineNumberFormatParse,
	(void *)MCEngineNumberFormatFormat,
	(void *)MCEngineNumberFormatFree,
};

//////////

static MCExecSetTypeElementInfo _kMCEngineSecurityCategoriesElementInfo[] =
{
	{ "disk", MC_SECUREMODE_DISK },
	{ "network", MC_SECUREMODE_NETWORK },
	{ "process", MC_SECUREMODE_PROCESS },
	{ "registryRead", MC_SECUREMODE_REGISTRY_READ },
	{ "registryWrite", MC_SECUREMODE_REGISTRY_WRITE },
	{ "stack", MC_SECUREMODE_STACK },
	{ "printing", MC_SECUREMODE_PRINT },	
	{ "privacy", MC_SECUREMODE_PRIVACY },
	{ "applescript", MC_SECUREMODE_APPLESCRIPT },
	{ "doalternate", MC_SECUREMODE_DOALTERNATE },
	{ "external", MC_SECUREMODE_EXTERNAL },
};

static MCExecSetTypeInfo _kMCEngineSecurityCategoriesTypeInfo =
{
	"Engine.SecurityCategories",
	sizeof(_kMCEngineSecurityCategoriesElementInfo) / sizeof(MCExecSetTypeElementInfo),
	_kMCEngineSecurityCategoriesElementInfo
};


////////////////////////////////////////////////////////////////////////////////

MCExecCustomTypeInfo *kMCEngineNumberFormatTypeInfo = &_kMCEngineNumberFormatTypeInfo;
MCExecSetTypeInfo *kMCEngineSecurityCategoriesTypeInfo = &_kMCEngineSecurityCategoriesTypeInfo;

////////////////////////////////////////////////////////////////////////////////

extern LT command_table[];
extern const uint4 command_table_size;
extern Cvalue constant_table[];
extern const uint4 constant_table_size;
extern LT factor_table[];
extern const uint4 factor_table_size;

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalVersion(MCExecContext& ctxt, MCNameRef& r_name)
{
	r_name = MCValueRetain(MCN_version_string);
}

void MCEngineEvalBuildNumber(MCExecContext& ctxt, integer_t& r_build_number)
{
	r_build_number = MCbuildnumber;
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalPlatform(MCExecContext& ctxt, MCNameRef& r_name)
{
    r_name = MCValueRetain(MCN_platform_string);
}

void MCEngineEvalEnvironment(MCExecContext& ctxt, MCNameRef& r_name)
{
	r_name = MCValueRetain(MCModeGetEnvironment());
}

void MCEngineEvalMachine(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCS_getmachine(r_string))
		return;

	ctxt.Throw();
}

void MCEngineEvalProcessor(MCExecContext& ctxt, MCNameRef& r_name)
{
    r_name = MCValueRetain(MCS_getprocessor());
}

void MCEngineEvalSystemVersion(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCS_getsystemversion(r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalCommandNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	bool t_success = true;

	MCAutoListRef t_list;
	t_success = MCListCreateMutable('\n', &t_list);

	for (uint32_t i = 0 ; t_success && i < command_table_size ; i++)
		if (command_table[i].type == TT_STATEMENT)
			t_success = MCListAppendCString(*t_list, command_table[i].token);

	if (t_success)
		t_success = MCListCopyAsString(*t_list, r_string);

	if (t_success)
		return;

	ctxt . Throw();
}

//////////

void MCEngineEvalConstantNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	bool t_success = true;

	MCAutoListRef t_list;
	t_success = MCListCreateMutable('\n', &t_list);

	for (uint32_t i = 0 ; t_success && i < constant_table_size ; i++)
		t_success = MCListAppendCString(*t_list, constant_table[i].token);

	if (t_success)
		t_success = MCListCopyAsString(*t_list, r_string);

	if (t_success)
		return;

	ctxt . Throw();
}

//////////

void MCEngineEvalFunctionNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	bool t_success = true;

	MCAutoListRef t_list;
	t_success = MCListCreateMutable('\n', &t_list);

	for (uint32_t i = 0 ; t_success && i < factor_table_size ; i++)
		if (factor_table[i].type == TT_FUNCTION)
			t_success = MCListAppendCString(*t_list, factor_table[i].token);

	if (t_success)
		t_success = MCListCopyAsString(*t_list, r_string);

	if (t_success)
		return;

	ctxt . Throw();
}

//////////

void MCEngineEvalPropertyNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	bool t_success = true;

	MCAutoListRef t_list;
	t_success = MCListCreateMutable('\n', &t_list);

	for (uint32_t i = 0 ; t_success && i < factor_table_size ; i++)
		if (factor_table[i].type == TT_PROPERTY)
			t_success = MCListAppendCString(*t_list, factor_table[i].token);

	if (t_success)
		t_success = MCListCopyAsString(*t_list, r_string);

	if (t_success)
		return;

	ctxt . Throw();
}

//////////

void MCEngineEvalGlobalNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	bool t_success = true;
	t_success = MCListCreateMutable(',', &t_list);

	for (MCVariable *v = MCglobals; t_success && v != nil; v = v->getnext())
		if (!v->isfree() || v->isarray())
			t_success = MCListAppend(*t_list, v->getname());

	if (t_success && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

void MCEngineEvalLocalNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (ctxt.GetHandler()->getvarnames(false, &t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

void MCEngineEvalVariableNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (ctxt.GetHandler()->getvarnames(true, &t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalParam(MCExecContext& ctxt, integer_t p_index, MCValueRef& r_value)
{
    if (MCValueCopy(ctxt.GetHandler()->getparam(p_index), r_value))
        return;
    
    ctxt.Throw();
}

void MCEngineEvalParamCount(MCExecContext& ctxt, integer_t& r_count)
{
    r_count = ctxt.GetHandler()->getnparams();
}

void MCEngineEvalParams(MCExecContext& ctxt, MCStringRef& r_string)
{
    MCAutoStringRef t_string;
    
    MCHandler* t_handler = ctxt.GetHandler();
    
    unichar_t t_space_char, t_quote_char, t_comma_char, t_open_bracket_char, t_close_bracket_char;
    t_space_char = ' ';
    t_quote_char = '\"';
    t_comma_char = ',';
    t_open_bracket_char = '(';
    t_close_bracket_char = ')';
    
    bool t_success = true;
    
    t_success = MCStringCreateMutable(0, &t_string);

    if (t_success)
        t_success = MCStringAppend(*t_string, MCNameGetString(t_handler->getname())) &&
            MCStringAppendChars(*t_string, (t_handler->gettype() == HT_FUNCTION) ? &t_open_bracket_char : &t_space_char, 1);
    
    uindex_t t_count = t_handler->getnparams();
    MCExecPoint ep(ctxt.GetEP());
    
    for (uinteger_t i = 1; t_success && i <= t_count; i++)
    {
        MCAutoStringRef t_param_string;
        
        t_success = ctxt.ForceToString(t_handler->getparam(i), &t_param_string) &&
            MCStringAppendChars(*t_string, &t_quote_char, 1) &&
            MCStringAppend(*t_string, *t_param_string) &&
            MCStringAppendChars(*t_string, &t_quote_char, 1);
        
        if (t_success && i < t_count)
            t_success = MCStringAppendChars(*t_string, &t_comma_char, 1);
    }
    
    if (t_success && t_handler->gettype() == HT_FUNCTION)
        t_success = MCStringAppendChars(*t_string, &t_close_bracket_char, 1);
    
    if (t_success)
        t_success = MCStringCopy(*t_string, r_string);
    
    if (t_success)
        return;
    
    ctxt.Throw();
}

//////////

void MCEngineEvalResult(MCExecContext& ctxt, MCValueRef& r_value)
{
    if (MCValueCopy(MCresult->getvalueref(), r_value))
        return;
    
    ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCEngineListObjectIds(MCExecContext& ctxt, MCObjectList *p_objects, MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	if (p_objects != nil)
	{
		MCObjectList *t_obj_ptr = p_objects;
		do
		{
			if (!t_obj_ptr->getremoved())
			{
				MCAutoStringRef t_id_string;
				if (!t_obj_ptr -> getobject() -> names(P_LONG_ID, &t_id_string))
					return false;
				if (!MCListAppend(*t_list, *t_id_string))
					return false;
			}
			t_obj_ptr = t_obj_ptr->next();
		}
		while (t_obj_ptr != p_objects);
	}
	return MCListCopy(*t_list, r_list);
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalBackScripts(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCEngineListObjectIds(ctxt, MCbackscripts, &t_list) &&
		MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

void MCEngineEvalFrontScripts(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCEngineListObjectIds(ctxt, MCfrontscripts, &t_list) &&
		MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalPendingMessages(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCscreen->listmessages(ctxt, &t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalInterrupt(MCExecContext& ctxt, bool& r_bool)
{
	r_bool = MCinterrupt == True;
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalMe(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCObject *t_target = ctxt.GetObject();
	if (t_target->gettype() == CT_FIELD || t_target->gettype() == CT_BUTTON)
		t_target->getstringprop(ctxt, 0, P_TEXT, False, r_string);
	else
		t_target->getstringprop(ctxt, 0, P_NAME, False, r_string);
}

//////////

void MCEngineEvalTarget(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCtargetptr == nil)
		r_string = MCValueRetain(kMCEmptyString);
	else
		MCtargetptr->getstringprop(ctxt, 0, P_NAME, False, r_string);
}

void MCEngineEvalTargetContents(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCtargetptr == nil)
		r_string = MCValueRetain(kMCEmptyString);
	else
		MCtargetptr->getstringprop(ctxt, 0, MCtargetptr->gettype() == CT_FIELD ? P_TEXT : P_NAME, False, r_string);
}

//////////

void MCEngineEvalOwner(MCExecContext& ctxt, MCObjectPtr p_object, MCStringRef& r_string)
{
	p_object . object ->getstringprop(ctxt, p_object . part_id, P_OWNER, False, r_string);
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalScriptLimits(MCExecContext& ctxt, MCStringRef& r_string)
{
	// TODO - create as list?
	if (MCStringFormat(r_string, "%d,%d,%d,%d", MClicenseparameters.script_limit, MClicenseparameters.do_limit, MClicenseparameters.using_limit, MClicenseparameters.insert_limit))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalSysError(MCExecContext& ctxt, uinteger_t& r_error)
{
	r_error = MCS_getsyserror();
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalValue(MCExecContext& ctxt, MCStringRef p_script, MCValueRef& r_value)
{
	if (p_script == nil || MCStringGetLength(p_script) == 0)
	{
		r_value = MCValueRetain(kMCEmptyString);
		return;
	}

	ctxt.GetHandler()->eval(ctxt, p_script, r_value);
	if (ctxt.HasError())
		ctxt.LegacyThrow(EE_VALUE_ERROR, p_script);
}

void MCEngineEvalValueWithObject(MCExecContext& ctxt, MCStringRef p_script, MCObjectPtr p_object, MCValueRef& r_value)
{
	if (MCStringGetLength(p_script) == 0)
	{
		r_value = MCValueRetain(kMCEmptyString);
		return;
	}

	MCExecContext t_ctxt(ctxt);

	t_ctxt.SetObject(p_object . object);
	t_ctxt.SetParentScript(nil);

	p_object . object -> eval(t_ctxt, p_script, r_value);
	if (t_ctxt.HasError())
		ctxt.LegacyThrow(EE_VALUE_ERROR, p_script);
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineExecSet(MCExecContext& ctxt, MCProperty *p_target, MCValueRef p_value)
{
	/* UNCHECKED */ ctxt . GetEP() . setvalueref(p_value);
	if (p_target -> set(ctxt . GetEP()) != ES_NORMAL)
	{
		ctxt . LegacyThrow(EE_SET_BADSET);
		return;
	}
		
	ctxt . SetTheResultToEmpty();
}

void MCEngineExecGet(MCExecContext& ctxt, MCValueRef p_value)
{
	ctxt . SetItToValue(p_value);
}

void MCEngineExecPutOutput(MCExecContext& ctxt, MCStringRef p_value, bool p_is_unicode)
{
	if (!MCS_put(ctxt . GetEP(), p_is_unicode ? kMCSPutUnicodeOutput : kMCSPutOutput, p_value))
		ctxt . LegacyThrow(EE_PUT_CANTSETINTO);
}

void MCEngineExecPutIntoVariable(MCExecContext& ctxt, MCValueRef p_value, int p_where, MCVariableChunkPtr p_var)
{
	MCExecPoint& ep = ctxt . GetEP();
	
	p_var . variable -> clearuql();
	
	if (p_var . chunk == CT_UNDEFINED)
	{
		if (p_where == PT_INTO)
		{
			ep . setvalueref(p_value);
			p_var . variable -> set(ep, False);
		}
		else if (p_where == PT_AFTER)
		{
			ep . setvalueref(p_value);
			p_var . variable -> set(ep, True);
		}
		else
		{
			if (p_var . variable -> eval(ep) != ES_NORMAL)
				return;
			
			MCAutoStringRef t_string;
			/* UNCHECKED */ ep . copyasmutablestringref(&t_string);
			
			MCAutoStringRef t_value_string;
			if (!ctxt . ConvertToString(p_value, &t_value_string))
			{
				ctxt . Throw();
				return;
			}
			
			/* UNCHECKED */ MCStringReplace(*t_string, MCRangeMake(0, 0), *t_value_string);
			
			ep . setvalueref(*t_string);
			p_var . variable -> set(ep, False);
		}
	}
	else
	{
		if (p_var . variable -> eval(ep) != ES_NORMAL)
			return;
		
		MCAutoStringRef t_string;
		/* UNCHECKED */ ep . copyasmutablestringref(&t_string);
		
		MCAutoStringRef t_value_string;
		if (!ctxt . ConvertToString(p_value, &t_value_string))
		{
			ctxt . Throw();
			return;
		}
		
		if (p_where == PT_BEFORE)
			p_var . finish = p_var . start;
		else if (p_where == PT_AFTER)
			p_var . start = p_var . finish;

		/* UNCHECKED */ MCStringReplace(*t_string, MCRangeMake(p_var . start, p_var . finish - p_var . start), *t_value_string);
		
		ep . setvalueref(*t_string);
		p_var . variable -> set(ep, False);
	}
}

void MCEngineExecReturnValue(MCExecContext& ctxt, MCValueRef p_value)
{
	ctxt . SetTheResultToValue(p_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineExecDo(MCExecContext& ctxt, MCStringRef p_script, int p_line, int p_pos)
{
	MCExecPoint& ep = ctxt . GetEP();

	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ep.setline(p_line);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
		added = True;
	}

	ep . setvalueref(p_script);

	if (ep . gethandler() -> doscript(ep, p_line, p_pos) != ES_NORMAL)
		ctxt . Throw();

	if (added)
		MCnexecutioncontexts--;
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineExecQuit(MCExecContext& ctxt, integer_t p_retcode)
{
// MW-2011-06-22: [[ SERVER ]] Don't send messages in server-mode.
#ifndef _SERVER
	switch(MCdefaultstackptr->getcard()->message(MCM_shut_down_request))
	{
		case ES_PASS:
		case ES_NOT_HANDLED:
			break;
		default:
			return;
	}
#ifndef TARGET_SUBPLATFORM_ANDROID
	MCdefaultstackptr->getcard()->message(MCM_shut_down);
#endif
#endif

	MCretcode = p_retcode;
	MCquit = True;
	MCquitisexplicit = True;
	MCexitall = True;
	MCtracestackptr = NULL;
	MCtraceabort = True;
	MCtracereturn = True;
}
////////////////////////////////////////////////////////////////////////////////

void MCEngineExecCancelMessage(MCExecContext& ctxt, integer_t p_id)
{
	if (p_id != 0)
		MCscreen->cancelmessageid(p_id);
}
////////////////////////////////////////////////////////////////////////////////
void MCEngineExecInsertScriptOfObjectInto(MCExecContext& ctxt, MCObject *p_script, bool p_in_front)
{
	if (!p_script->parsescript(True))
	{
		ctxt . LegacyThrow(EE_INSERT_BADTARGET);
		return;
	}
	MCObjectList *&listptr = p_in_front ? MCfrontscripts : MCbackscripts;
	p_script->removefrom(listptr);
	uint4 count = 0;
	if (listptr != NULL)
	{
		MCObjectList *olptr = listptr;
		do
		{
			if (!olptr->getremoved())
				count++;
			olptr = olptr->next();
		}
		while (olptr != listptr);
	}
	if (MClicenseparameters . insert_limit > 0 && count >= MClicenseparameters . insert_limit)
	{
		ctxt . LegacyThrow(EE_INSERT_NOTLICENSED);
		return;
	}
	MCObjectList *olptr = new MCObjectList(p_script);
	olptr->insertto(listptr);
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineExecRemoveAllScriptsFrom(MCExecContext& ctxt, bool p_in_front)
{
	MCObjectList *listptr = p_in_front ? MCfrontscripts : MCbackscripts;
	MCObjectList *lptr = listptr;
	do
	{
		lptr->setremoved(True);
		lptr = lptr->next();
	}
	while (lptr != listptr);
}

void MCEngineExecRemoveScriptOfObjectFrom(MCExecContext& ctxt, MCObject *p_script, bool p_in_front)
{
	p_script->removefrom(p_in_front ? MCfrontscripts : MCbackscripts);
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineExecWaitFor(MCExecContext& ctxt, double p_delay, int p_units, bool p_messages)
{
	MCU_play();
	if (p_units == F_UNDEFINED)
	{
		if (MCscreen->wait(p_delay, p_messages, p_messages) || MCabortscript)
			ctxt . LegacyThrow(EE_WAIT_ABORT);
		return;
	}

	switch (p_units)
	{
	case F_MILLISECS:
		p_delay /= 1000.0;
		break;
	case F_TICKS:
		p_delay /= 60.0;
		break;
	default:
		break;
	}

	if (MCscreen->wait(p_delay, p_messages, False))
	{
		ctxt . LegacyThrow(EE_WAIT_ABORT);
		return;
	}

}

void MCEngineExecWaitUntil(MCExecContext& ctxt, MCExpression *p_condition, bool p_messages)
{
	while(True)
	{
		MCAutoValueRef t_evaluated;
		MCAutoBooleanRef t_evaluated_as_boolean;
		
		MCU_play();
		
		if (!ctxt . EvaluateExpression(p_condition, &t_evaluated))
			return;
		
		if (!ctxt . ForceToBoolean(*t_evaluated, &t_evaluated_as_boolean))
		{
			ctxt . Throw();
			return;
		}

		if (*t_evaluated_as_boolean == kMCTrue)
			return;

		if (MCscreen->wait(WAIT_INTERVAL, p_messages, True))
		{
			ctxt . LegacyThrow(EE_WAIT_ABORT);
			return;
		}
	}
}

void MCEngineExecWaitWhile(MCExecContext& ctxt, MCExpression *p_condition, bool p_messages)
{
	while(True)
	{
		MCAutoValueRef t_evaluated;
		MCAutoBooleanRef t_evaluated_as_boolean;
		
		MCU_play();
		
		if (!ctxt . EvaluateExpression(p_condition, &t_evaluated))
			return;
		
		if (!ctxt . ForceToBoolean(*t_evaluated, &t_evaluated_as_boolean))
		{
			ctxt . Throw();
			return;
		}

		if (*t_evaluated_as_boolean == kMCFalse)
			return;

		if (MCscreen->wait(WAIT_INTERVAL, p_messages, True))
		{
			ctxt . LegacyThrow(EE_WAIT_ABORT);
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineExecDeleteVariable(MCExecContext& ctxt, MCVarref *p_target)
{
	if (p_target->dofree(ctxt . GetEP()) != ES_NORMAL)
		ctxt . Throw();
}

void MCEngineExecDeleteVariableChunks(MCExecContext& ctxt, MCVariableChunkPtr *p_chunks, uindex_t p_chunk_count)
{
	for(uindex_t i = 0; i < p_chunk_count; i++)
	{
		p_chunks[i] . variable -> eval(ctxt . GetEP());
		ctxt . GetEP() . insert(MCnullmcstring, p_chunks[i] . start, p_chunks[i] . finish);
		p_chunks[i] .variable -> set(ctxt . GetEP());
	}
}

///////////////////////////////////////////////////////////////////////////////

void MCEngineExecStartUsingStack(MCExecContext& ctxt, MCStack *p_stack)
{
	uint2 i = MCnusing;
	while (i--)
		if (MCusing[i] == p_stack)
		{
			MCnusing--;
			while (i < MCnusing)
			{
				MCusing[i] = MCusing[i + 1];
				i++;
			}
			break;
		}

	if (MClicenseparameters . using_limit > 0 && MCnusing >= MClicenseparameters . using_limit)
	{
		ctxt . LegacyThrow(EE_START_NOTLICENSED);
		return;
	}

	MCU_realloc((char **)&MCusing, MCnusing, MCnusing + 1, sizeof(MCStack *));
	MCusing[MCnusing++] = p_stack;
	if (p_stack->message(MCM_library_stack) != ES_ERROR)
		return;

	ctxt . Throw();
}

void MCEngineExecStartUsingStackByName(MCExecContext& ctxt, MCStringRef p_name)
{
	MCStack *sptr;
	if ((sptr = MCdefaultstackptr->findstackname_oldstring(MCStringGetOldString(p_name))) == NULL ||
		!sptr->parsescript(True))
		{
			ctxt . LegacyThrow(EE_START_BADTARGET);
			return;
		}
	MCEngineExecStartUsingStack(ctxt, sptr);
}

///////////////////////////////////////////////////////////////////////////////

void MCEngineExecStopUsingStack(MCExecContext& ctxt, MCStack *p_stack)
{
	uint2 i = MCnusing;
	while (i--)
		if (MCusing[i] == p_stack)
		{
			MCnusing--;
			while (i < MCnusing)
			{
				MCusing[i] = MCusing[i + 1];
				i++;
			}
			break;
		}
	p_stack->message(MCM_release_stack);
}

void MCEngineExecStopUsingStackByName(MCExecContext& ctxt, MCStringRef p_name)
{
	MCStack *sptr;
	if ((sptr = MCdefaultstackptr->findstackname_oldstring(MCStringGetOldString(p_name))) == NULL)
		{
			ctxt . LegacyThrow(EE_START_BADTARGET);
			return;
		}
	MCEngineExecStopUsingStack(ctxt, sptr);
}
			        
///////////////////////////////////////////////////////////////////////////////

void MCEngineExecDispatch(MCExecContext& ctxt, int p_handler_type, MCNameRef p_message, MCObjectPtr *p_target, MCParameter *p_parameters)
{
	if (MCscreen -> abortkey())
	{
		ctxt . LegacyThrow(EE_HANDLER_ABORT);
		return;
	}
	
	// Work out the target object
	MCObject *t_object;
	if (p_target != nil)
		t_object = p_target -> object;
	else
		t_object = ctxt . GetObject();
		
	// Fetch current default stack and target settings
	MCStack *t_old_stack;
	t_old_stack = MCdefaultstackptr;
	MCObject *t_old_target;
	t_old_target = MCtargetptr;
	
	// Cache the current 'this stack' (used to see if we should switch back
	// the default stack).
	MCStack *t_this_stack;
	t_this_stack = t_object -> getstack();
	
	// Retarget this stack and the target to be relative to the target object
	MCdefaultstackptr = t_this_stack;
	MCtargetptr = t_object;

	// MW-2012-10-30: [[ Bug 10478 ]] Turn off lockMessages before dispatch.
	Boolean t_old_lock;
	t_old_lock = MClockmessages;
	MClockmessages = False;
	
	// Add a new entry in the execution contexts
	MCExecPoint *oldep = MCEPptr;
	MCEPptr = &ctxt . GetEP();
	Exec_stat t_stat;
	t_stat = ES_NOT_HANDLED;
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt . GetEP();
		added = True;
	}

	// Dispatch the message
	t_stat = MCU_dofrontscripts((Handler_type)p_handler_type, p_message, p_parameters);
	Boolean olddynamic = MCdynamicpath;
	MCdynamicpath = MCdynamiccard != NULL;
	if (t_stat == ES_PASS || t_stat == ES_NOT_HANDLED)
		switch(t_stat = t_object->handle((Handler_type)p_handler_type, p_message, p_parameters, t_object))
		{
		case ES_ERROR:
			ctxt . LegacyThrow(EE_DISPATCH_BADCOMMAND, p_message);
			break;
		default:
			break;
		}
	
	// Set 'it' appropriately
	switch(t_stat)
	{
	case ES_NOT_HANDLED:
	case ES_NOT_FOUND:
		ctxt . SetItToValue(MCN_unhandled);
		t_stat = ES_NORMAL;
		break;
		
	case ES_PASS:
		ctxt . SetItToValue(MCN_passed);
		t_stat = ES_NORMAL;
		break;
	
	case ES_EXIT_HANDLER:
	case ES_NORMAL:
		ctxt . SetItToValue(MCN_handled);
		t_stat = ES_NORMAL;
	break;
	
	default:
		ctxt . SetItToValue(kMCEmptyString);
	break;
	}
	
	// Reset the default stack pointer and target - note that we use 'send'esque
	// semantics here. i.e. If the default stack has been changed, the change sticks.
	if (MCdefaultstackptr == t_this_stack)
		MCdefaultstackptr = t_old_stack;

	// Reset target pointer
	MCtargetptr = t_old_target;
	MCdynamicpath = olddynamic;
	
	// MW-2012-10-30: [[ Bug 10478 ]] Restore lockMessages.
	MClockmessages = t_old_lock;
	
	// Remove our entry from the contexts list
	MCEPptr = oldep;
	if (added)
		MCnexecutioncontexts--;
}

///////////////////////////////////////////////////////////////////////////////

static void MCEngineSplitScriptIntoMessageAndParameters(MCExecContext& ctxt, MCStringRef p_script, MCNameRef& r_message, MCParameter*& r_params)
{
	MCParameter *params = NULL;
	MCParameter *tparam = NULL;
	
	char *mptr = strclone(MCStringGetCString(p_script));
	char *sptr = mptr;
	while (*sptr && !isspace((uint1)*sptr))
		sptr++;
		
	MCerrorlock++;
	if (*sptr)
	{
		*sptr++ = '\0';
		char *startptr = sptr;
		while (*startptr)
		{
			while (*sptr && *sptr != ',')
				if (*sptr == '"')
				{
					sptr++;
					while (*sptr && *sptr++ != '"')
						;
				}
				else
					sptr++;
			if (*sptr)
				*sptr++ = '\0';
			MCString pdata = startptr;
			ctxt . GetEP() . setsvalue(pdata);
			
			MCParameter *newparam = new MCParameter;

			// MW-2011-08-11: [[ Bug 9668 ]] Make sure we copy 'pdata' if we use it, since
			//   mptr (into which it points) only lasts as long as this method call.
			if (ctxt . GetEP() . gethandler() -> eval(ctxt . GetEP()) == ES_NORMAL)
				newparam->set_argument(ctxt . GetEP());
			else
				newparam->copysvalue_argument(pdata);

			if (tparam == NULL)
				params = tparam = newparam;
			else
			{
				tparam->setnext(newparam);
				tparam = newparam;
			}
			startptr = sptr;
		}
	}
	MCerrorlock--;
	
	/* UNCHECKED */ MCNameCreateWithCString(mptr, r_message);
	r_params = params;
	
	delete mptr;
}

static void MCEngineSendOrCall(MCExecContext& ctxt, MCStringRef p_script, MCObjectPtr *p_target, bool p_is_send)
{
	MCNewAutoNameRef t_message;
	MCParameter *t_params;
	MCEngineSplitScriptIntoMessageAndParameters(ctxt, p_script, &t_message, t_params);
	
	MCObject *optr;
	if (p_target == nil)
		optr = ctxt . GetObject();
	else
		optr = p_target -> object;
	
	Boolean oldlock = MClockmessages;
	MClockmessages = False;
	Exec_stat stat;
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt . GetEP();
		added = True;
	}
	if ((stat = optr->message(*t_message, t_params, p_is_send, True)) == ES_NOT_HANDLED)
	{
		MCHandler *t_handler;
		t_handler = optr -> findhandler(HT_MESSAGE, *t_message);
		if (t_handler != NULL && t_handler -> isprivate())
			ctxt . LegacyThrow(EE_SEND_BADEXP, *t_message);
		else
		{
            MCAutoStringRef tptr;

			if (t_params != NULL)
			{
				t_params->eval(ctxt . GetEP());
                MCAutoStringRef t_value;
				ctxt . GetEP() . copyasstringref(&t_value);
                MCStringFormat(&tptr, "%@ %@", *t_message, *t_value);
				
			}
            else
                tptr = MCNameGetString(*t_message);
            
			if ((stat = optr->domess(&tptr)) == ES_ERROR)
				ctxt . LegacyThrow(EE_STATEMENT_BADCOMMAND, *t_message);
		}
	}
	else if (stat == ES_PASS)
		stat = ES_NORMAL;
	else if (stat == ES_ERROR)
		ctxt . LegacyThrow(EE_SEND_BADEXP, *t_message);
	while (t_params != NULL)
	{
		MCParameter *tmp = t_params;
		t_params = t_params->getnext();
		delete tmp;
	}
	if (added)
		MCnexecutioncontexts--;
	MClockmessages = oldlock;
}

void MCEngineExecSend(MCExecContext& ctxt, MCStringRef p_script, MCObjectPtr *p_target)
{
	MCEngineSendOrCall(ctxt, p_script, p_target, true);
}

void MCEngineExecCall(MCExecContext& ctxt, MCStringRef p_script, MCObjectPtr *p_target)
{
	MCEngineSendOrCall(ctxt, p_script, p_target, false);
}

void MCEngineExecSendInTime(MCExecContext& ctxt, MCStringRef p_script, MCObjectPtr p_target, double p_delay, int p_units)
{
	MCNewAutoNameRef t_message;
	MCParameter *t_params;
	MCEngineSplitScriptIntoMessageAndParameters(ctxt, p_script, &t_message, t_params);

	switch (p_units)
	{
	case F_MILLISECS:
		p_delay /= 1000.0;
		break;
	case F_TICKS:
		p_delay /= 60.0;
		break;
	default:
		break;
	}
	
	MCscreen->addmessage(p_target . object, *t_message, MCS_time() + p_delay, t_params);
}

///////////////////////////////////////////////////////////////////////////////

void MCEngineExecLockErrors(MCExecContext& ctxt)
{
	MClockerrors = True;
	MCerrorlockptr = ctxt . GetObject();
}

void MCEngineExecLockMessages(MCExecContext& ctxt)
{
	MClockmessages = True;
}

void MCEngineExecUnlockErrors(MCExecContext& ctxt)
{
	MClockerrors = False;
}

void MCEngineExecUnlockMessages(MCExecContext& ctxt)
{
	MClockmessages = False;;
}

///////////////////////////////////////////////////////////////////////////////

void MCEngineGetCaseSensitive(MCExecContext& ctxt, bool& r_value)
{
	r_value = ctxt . GetCaseSensitive();
}

void MCEngineSetCaseSensitive(MCExecContext& ctxt, bool p_value)
{
	ctxt . SetCaseSensitive(p_value);
}

///////////////////////////////////////////////////////////////////////////////

void MCEngineSetCenturyCutOff(MCExecContext& ctxt, integer_t p_value)
{
	ctxt . SetCutOff(p_value);
}

void MCEngineGetCenturyCutOff(MCExecContext& ctxt, integer_t& r_value)
{
	r_value = ctxt . GetCutOff();
}

void MCEngineSetConvertOctals(MCExecContext& ctxt, bool p_value)
{
	ctxt . SetConvertOctals(p_value);
}

void MCEngineGetConvertOctals(MCExecContext& ctxt, bool& r_value)
{
	r_value = ctxt . GetConvertOctals();
}

void MCEngineSetItemDelimiter(MCExecContext& ctxt, char_t p_value)
{
	ctxt . SetItemDelimiter(p_value);
}

void MCEngineGetItemDelimiter(MCExecContext& ctxt, char_t& r_value)
{
	r_value = ctxt . GetItemDelimiter();
}

void MCEngineSetLineDelimiter(MCExecContext& ctxt, char_t p_value)
{
	ctxt . SetLineDelimiter(p_value);
}

void MCEngineGetLineDelimiter(MCExecContext& ctxt, char_t& r_value)
{
	r_value = ctxt . GetLineDelimiter();
}

void MCEngineSetColumnDelimiter(MCExecContext& ctxt, char_t p_value)
{
	ctxt . SetColumnDelimiter(p_value);
}

void MCEngineGetColumnDelimiter(MCExecContext& ctxt, char_t& r_value)
{
	r_value = ctxt . GetColumnDelimiter();
}

void MCEngineSetRowDelimiter(MCExecContext& ctxt, char_t p_value)
{
	ctxt . SetRowDelimiter(p_value);
}

void MCEngineGetRowDelimiter(MCExecContext& ctxt, char_t& r_value)
{
	r_value = ctxt . GetRowDelimiter();
}

void MCEngineSetWholeMatches(MCExecContext& ctxt, bool p_value)
{
	ctxt . SetWholeMatches(p_value);
}

void MCEngineGetWholeMatches(MCExecContext& ctxt, bool& r_value)
{
	r_value = ctxt . GetWholeMatches();
}

void MCEngineSetUseSystemDate(MCExecContext& ctxt, bool p_value)
{
	ctxt . SetUseSystemDate(p_value);
}

void MCEngineGetUseSystemDate(MCExecContext& ctxt, bool& r_value)
{
	r_value = ctxt . GetUseSystemDate();
}

void MCEngineSetUseUnicode(MCExecContext& ctxt, bool p_value)
{
	ctxt . SetUseUnicode(p_value);
}

void MCEngineGetUseUnicode(MCExecContext& ctxt, bool& r_value)
{
	r_value = ctxt . GetUseUnicode();
}

void MCEngineSetNumberFormat(MCExecContext& ctxt, const MCEngineNumberFormat& p_format)
{
	ctxt . GetEP() . setnumberformat(p_format . fw, p_format . trailing, p_format . force);
}

void MCEngineGetNumberFormat(MCExecContext& ctxt, MCEngineNumberFormat& r_format)
{
	r_format . fw = ctxt . GetEP() . getnffw();
	r_format . trailing = ctxt . GetEP() . getnftrailing();
	r_format . force = ctxt . GetEP() . getnfforce();
}

///////////////////////////////////////////////////////////////////////////////

void MCEngineGetScriptExecutionErrors(MCExecContext& ctxt, MCStringRef &r_value)
{
	if (MCStringCreateWithCString(MCexecutionerrors, r_value))
		return;

	ctxt . Throw();
}

void MCEngineGetScriptParsingErrors(MCExecContext& ctxt, MCStringRef &r_value)
{
	if (MCStringCreateWithCString(MCparsingerrors, r_value))
		return;

	ctxt . Throw();
}

///////////////////////////////////////////////////////////////////////////////

void MCEngineGetAllowInterrupts(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCallowinterrupts == True;
}

void MCEngineSetAllowInterrupts(MCExecContext& ctxt, bool p_value)
{
	MCallowinterrupts = p_value ? True : False;
}

void MCEngineGetExplicitVariables(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCexplicitvariables == True;
}

void MCEngineSetExplicitVariables(MCExecContext& ctxt, bool p_value)
{
	MCexplicitvariables = p_value ? True : False;
}

void MCEngineGetPreserveVariables(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCpreservevariables == True;
}

void MCEngineSetPreserveVariables(MCExecContext& ctxt, bool p_value)
{
	MCpreservevariables = p_value ? True : False;
}

///////////////////////////////////////////////////////////////////////////////

void MCEngineGetStackLimit(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCpendingstacklimit;
}

void MCEngineGetEffectiveStackLimit(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCstacklimit;
}

void MCEngineSetStackLimit(MCExecContext& ctxt, uinteger_t p_value)
{
	MCpendingstacklimit = p_value;
}

///////////////////////////////////////////////////////////////////////////////

void MCEngineGetSecureMode(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCsecuremode == MC_SECUREMODE_ALL;
}

void MCEngineSetSecureMode(MCExecContext& ctxt, bool p_value)
{
	MCnofiles = True;
	MCsecuremode = MC_SECUREMODE_ALL;
}

void MCEngineGetSecurityCategories(MCExecContext& ctxt, intset_t& r_value)
{
	r_value = MC_SECUREMODE_ALL;
}

void MCEngineGetSecurityPermissions(MCExecContext& ctxt, intset_t& r_value)
{
	r_value = MCsecuremode;
}

///////////////////////////////////////////////////////////////////////////////

void MCEngineGetRecursionLimit(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCrecursionlimit;
}

void MCEngineSetRecursionLimit(MCExecContext& ctxt, uinteger_t p_value)
{
	Exec_stat t_stat;

	MCrecursionlimit = p_value;
#ifdef _WINDOWS
	MCrecursionlimit = MCU_min(MCstacklimit - MC_UNCHECKED_STACKSIZE, MCU_max(MCrecursionlimit, MCU_max(MC_UNCHECKED_STACKSIZE, MCU_abs(MCstackbottom - (char *)&t_stat) * 3)));
#else
	MCrecursionlimit = MCU_max(MCrecursionlimit, MCU_abs(MCstackbottom - (char *)&t_stat) * 3); // fudge to 3x current stack depth
#endif
}

///////////////////////////////////////////////////////////////////////////////

void MCEngineGetAddress(MCExecContext& ctxt, MCStringRef &r_value)
{
	if (MCS_getaddress(r_value))
		return;

	ctxt . Throw();
}

void MCEngineGetStacksInUse(MCExecContext& ctxt, MCStringRef &r_value)
{
	bool t_success;
	t_success = true;
	
	MCAutoListRef t_list;
	t_success = MCListCreateMutable('\n', &t_list);

	int2 i;
	i = MCnusing;
	while (i--)
	{
		if (t_success)
		{
			MCAutoStringRef t_stack_name;
			MCusing[i] -> names(P_SHORT_NAME, &t_stack_name);
			t_success = MCListAppend(*t_list, *t_stack_name);
		}
	}
	
	if (t_success && MCListCopyAsString(*t_list, r_value))
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCEngineEvalValueAsObject(MCValueRef p_value, bool p_strict, MCObjectPtr& r_object, bool& r_parse_error)
{
    MCExecPoint ep(nil,nil,nil);
    ep . setvalueref(p_value);
    MCScriptPoint sp(ep);
    MCChunk *tchunk = new MCChunk(False);
    MCerrorlock++;
    Symbol_type type;
    Exec_stat stat;
    
    bool t_parse_error;
    t_parse_error = tchunk->parse(sp, False) == PS_NORMAL;
    if (!t_parse_error && (!p_strict || sp.next(type) == PS_EOF))
        stat = ES_NORMAL;
    MCerrorlock--;
    if (stat == ES_NORMAL)
        stat = tchunk->getobj(ep, r_object, False);
    delete tchunk;
    
    r_parse_error = t_parse_error;
    return stat == ES_NORMAL;
}

void MCEngineEvalValueAsObject(MCExecContext& ctxt, MCValueRef p_value, MCObjectPtr& r_object)
{
    bool t_parse_error;
    if (MCEngineEvalValueAsObject(p_value, true, r_object, t_parse_error))
        ctxt . LegacyThrow(EE_CHUNK_BADOBJECTEXP);
}

void MCEngineEvalOwnerAsObject(MCExecContext& ctxt, MCObjectPtr p_object, MCObjectPtr& r_owner)
{
    if (!(p_object . object -> gettype() == CT_STACK && MCdispatcher -> ismainstack(static_cast<MCStack *>(p_object . object))))
    {
        r_owner . object = p_object . object -> getparent();
        r_owner . part_id  = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCEngineEvalTemplateAsObject(MCExecContext& ctxt, uinteger_t p_template_type, MCObjectPtr& r_object)
{
    MCObject *t_object;
    t_object = nil;
    
    switch ((Dest_type) p_template_type)
    {
        case DT_STACK:
            t_object = MCtemplatestack;
            break;
        case DT_AUDIO_CLIP:
            t_object = (MCObject *)MCtemplateaudio;
            break;
        case DT_VIDEO_CLIP:
            t_object = (MCObject *)MCtemplatevideo;
            break;
        case DT_GROUP:
            t_object = (MCObject *)MCtemplategroup;
            break;
        case DT_CARD:
            t_object = (MCObject *)MCtemplatecard;
            break;
        case DT_BUTTON:
            t_object = (MCObject *)MCtemplatebutton;
            break;
        case DT_FIELD:
            t_object = (MCObject *)MCtemplatefield;
            break;
        case DT_IMAGE:
            t_object = (MCObject *)MCtemplateimage;
            break;
        case DT_SCROLLBAR:
            t_object = (MCObject *)MCtemplatescrollbar;
            break;
        case DT_PLAYER:
            t_object = (MCObject *)MCtemplateplayer;
            break;
        case DT_GRAPHIC:
            t_object = (MCObject *)MCtemplategraphic;
            break;
        case DT_EPS:
            t_object = (MCObject *)MCtemplateeps;
            break;
        default:
            break;
    }
    
    if (t_object != nil)
    {
        r_object . object = t_object;
        r_object . part_id  = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCEngineEvalMeAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    // MW-2009-01-28: [[ Inherited parentScripts ]]
    // If we are executing in the context of a parent-handle invocation
    // (indicated by getparentscript() of the EP being non-NULL) 'me'
    // refers to the derived object context, otherwise it is the object
    // we were compiled in.
    
    MCObject *t_object;
    
    if (ctxt . GetParentScript() == NULL)
        t_object = nil; // destobj!
    else
        t_object = ctxt . GetObject();

    if (t_object != nil)
    {
        r_object . object = t_object;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCEngineEvalMenuObjectAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCmenuobjectptr != nil)
    {
        r_object . object = MCmenuobjectptr;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCEngineEvalTargetAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCtargetptr != nil)
    {
        r_object . object = MCtargetptr;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCEngineEvalErrorObjectAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCerrorptr != nil)
    {
        r_object . object = MCerrorptr;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}
