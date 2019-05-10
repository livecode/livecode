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
#include "hndlrlst.h"

#include "stack.h"
#include "card.h"

#include "exec.h"
#include "util.h"

#include "express.h"
#include "variable.h"
#include "chunk.h"
#include "securemode.h"
#include "dispatch.h"

#include "uuid.h"

#include "libscript/script.h"

#include "license.h"

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

// AL-2014-10-29: [[ Bug 13704 ]] Security permissions set type should use bits rather than bit-shifted values
static MCExecSetTypeElementInfo _kMCEngineSecurityCategoriesElementInfo[] =
{
	{ "disk", kMCSecureModeTypeDiskBit },
	{ "network", kMCSecureModeTypeNetworkBit },
	{ "process", kMCSecureModeTypeProcessBit },
	{ "registryRead", kMCSecureModeTypeRegistryReadBit },
	{ "registryWrite", kMCSecureModeTypeRegistryWriteBit },
	{ "printing", kMCSecureModeTypePrintBit },	
	{ "privacy", kMCSecureModeTypePrivacyBit },
	{ "applescript", kMCSecureModeTypeApplescriptBit },
	{ "doalternate", kMCSecureModeTypeDoalternateBit },
	{ "external", kMCSecureModeTypeExternalBit },
    { "extension", kMCSecureModeTypeExtensionBit },
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

extern const LT command_table[];
extern const uint4 command_table_size;
extern const Cvalue *constant_table;
extern const uint4 constant_table_size;
extern const LT factor_table[];
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

void MCEngineEvalProcessor(MCExecContext& ctxt, MCStringRef& r_string)
{
    if (MCS_getprocessor(r_string))
        return;
    
    ctxt.Throw();
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
        if (!v->isclear() || v->isarray())
			t_success = MCListAppend(*t_list, v->getname());

	if (t_success && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

void MCEngineEvalLocalNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	// MW-2013-11-15: [[ Bug 11277 ]] Server mode may call this outwith a handler.
	MCAutoListRef t_list;
	if (ctxt.GetHandler() != nil)
	{
		if (ctxt.GetHandler()->getvarnames(false, &t_list) && MCListCopyAsString(*t_list, r_string))
			return;
	}
	else
	{
		if (ctxt.GetHandlerList()->getlocalnames(&t_list) && MCListCopyAsString(*t_list, r_string))
			return;
	}
	
	ctxt.Throw();
}

void MCEngineEvalVariableNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	// MW-2013-11-15: [[ Bug 11277 ]] If no handler, then process the handler list
	//   (server script scope).
	MCAutoListRef t_list;
	if (ctxt.GetHandler() != nil)
	{
		if (ctxt.GetHandler()->getvarnames(true, &t_list) && MCListCopyAsString(*t_list, r_string))
			return;
	}
	else
	{
		MCAutoListRef t_local_list, t_global_list;
		if (MCListCreateMutable('\n', &t_list) &&
			ctxt.GetHandlerList()->getlocalnames(&t_local_list) &&
			MCListAppend(*t_list, *t_local_list) &&
			ctxt.GetHandlerList()->getglobalnames(&t_global_list) &&
			MCListAppend(*t_list, *t_global_list) &&
			MCListCopyAsString(*t_list, r_string))
			return;
	}
	
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
	// MW-2013-11-15: [[ Bug 11277 ]] If we don't have a handler then 'the param'
    //   makes no sense so just return 0.
	if (ctxt.GetHandler() != nil)
		r_count = ctxt.GetHandler()->getnparams();
	else
        r_count = 0;
}

void MCEngineEvalParams(MCExecContext& ctxt, MCStringRef& r_string)
{
    MCAutoStringRef t_string;
    
    MCHandler* t_handler = ctxt.GetHandler();
    
	// MW-2013-11-15: [[ Bug 11277 ]] If we don't have a handler then 'the params'
	//   makes no sense so just return empty.
	if (t_handler == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}
	
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
				MCAutoValueRef t_id_string;
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
	if (!MCtargetptr)
		r_string = MCValueRetain(kMCEmptyString);
	else
		MCtargetptr -> getstringprop(ctxt, MCtargetptr.getPart(), P_NAME, False, r_string);
}

void MCEngineEvalTargetContents(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (!MCtargetptr)
		r_string = MCValueRetain(kMCEmptyString);
	else
		MCtargetptr -> getstringprop(ctxt, MCtargetptr.getPart(), MCtargetptr -> gettype() == CT_FIELD ? P_TEXT : P_NAME, False, r_string);
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

    // SN-2015-06-03: [[ Bug 11277 ]] MCHandler::eval refactored
    ctxt.eval(ctxt, p_script, r_value);

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
    MCExecValue t_value;
    t_value . valueref_value = MCValueRetain(p_value);
    t_value . type = kMCExecValueTypeValueRef;
	
    p_target -> set(ctxt, t_value);
    if (ctxt . HasError())
	{
		ctxt . LegacyThrow(EE_SET_BADSET);
		return;
	}
}

void MCEngineExecGet(MCExecContext& ctxt, /* take */ MCExecValue& p_value)
{
    ctxt . GiveValueToIt(p_value);
}

void MCEngineExecPutOutput(MCExecContext& ctxt, MCStringRef p_value)
{
	if (!MCS_put(ctxt, kMCSPutOutput, p_value))
		ctxt . LegacyThrow(EE_PUT_CANTSETINTO);
}

void MCEngineExecPutOutputUnicode(MCExecContext& ctxt, MCDataRef p_value)
{
	MCAutoStringRef t_string;
	if (!MCStringCreateWithChars((const unichar_t*)MCDataGetBytePtr(p_value), MCDataGetLength(p_value)/sizeof(unichar_t), &t_string)
		|| !MCS_put(ctxt, kMCSPutOutput, *t_string))
		ctxt . LegacyThrow(EE_PUT_CANTSETINTO);
}

void MCEngineExecPutIntoVariable(MCExecContext& ctxt, MCValueRef p_value, int p_where, MCVariableChunkPtr p_var)
{	
	p_var . variable -> clearuql();
	
	if (p_var . chunk == CT_UNDEFINED)
	{
        // SN-2014-04-11 [[ FasterVariables ]] Now chosing from here the position where to add a string on a variable
		if (p_where == PT_INTO)
			p_var . variable -> set(ctxt, p_value, kMCVariableSetInto);
		else if (p_where == PT_AFTER)
			p_var . variable -> set(ctxt, p_value, kMCVariableSetAfter);
		else
			p_var . variable -> set(ctxt, p_value, kMCVariableSetBefore);
	}
	else
    {
        if (p_where == PT_BEFORE)
            p_var . mark . finish = p_var . mark . start;
        else if (p_where == PT_AFTER)
            p_var . mark . start = p_var . mark . finish;
        
        // AL-2014-06-12: [[ Bug 12195 ]] If either the mark or the value is non-data, then convert to string.
        //  Otherwise we get data loss for 'put <unicode string> after byte <n> of tVar'
        if (MCValueGetTypeCode(p_var . mark . text) == kMCValueTypeCodeData &&
            MCValueGetTypeCode(p_value) == kMCValueTypeCodeData)
        {
            // SN-2014-09-03: [[ Bug 13314 ]] MCMarkedText::changed updated to store the number of chars appended
            if (p_var . mark . changed != 0)
            {
                MCAutoDataRef t_data;
                if (!MCDataMutableCopyAndRelease((MCDataRef)p_var . mark . text, &t_data))
                    return;
                
                /* UNCHECKED */ MCDataReplace(*t_data, MCRangeMakeMinMax(p_var . mark . start, p_var . mark . finish), (MCDataRef)p_value);
                p_var . variable -> set(ctxt, *t_data, kMCVariableSetInto);
            }
            else
            {
                // AL-2014-11-12: [[ Bug 13987 ]] Release the mark here, so that eg 'put x into byte y of z'
                //  can take advantage of the fact that z has only one reference. Otherwise it requires a copy
                MCValueRelease(p_var . mark . text);
                
                p_var . variable -> replace(ctxt, (MCDataRef)p_value, MCRangeMakeMinMax(p_var . mark . start, p_var . mark . finish));
            }
        }
        else
        {
            MCAutoStringRef t_value_string;
            if (!ctxt . ConvertToString(p_value, &t_value_string))
            {
                ctxt . Throw();
                return;
            }
            
            // AL-2015-04-01: [[ Bug 15139 ]] Make sure the mark text is the correct value type.
            MCValueRef t_mark_text;
            if (!ctxt . ConvertToString(p_var . mark . text, (MCStringRef &)t_mark_text))
            {
                ctxt . Throw();
                return;
            }

            // SN-2015-07-27: [[ Bug 15646 ]] MCExecContext::ConvertToString
            //  already makes a copy. We want to make sure that p_var.mark.text
            //  has 0 reference when exiting this function, so p_var.mark.text
            //  must become t_mark_text, not get a copy of it.
            MCValueRelease(p_var . mark . text);
            p_var . mark . text = t_mark_text;
            
            // SN-2014-09-03: [[ Bug 13314 ]] MCMarkedText::changed updated to store the number of chars appended
            if (p_var . mark . changed != 0)
            {
                MCAutoStringRef t_string;
                if (!MCStringMutableCopyAndRelease((MCStringRef)p_var . mark . text, &t_string))
                    return;
            
                /* UNCHECKED */ MCStringReplace(*t_string, MCRangeMakeMinMax(p_var . mark . start, p_var . mark . finish), *t_value_string);
                p_var . variable -> set(ctxt, *t_string, kMCVariableSetInto);
            }
            else
            {
                // AL-2014-11-12: [[ Bug 13987 ]] Release the mark here, so that eg 'put x into char y of z'
                //  can take advantage of the fact that z has only one reference. Otherwise it requires a copy
                MCValueRelease(p_var . mark . text);
                
                p_var . variable -> replace(ctxt, *t_value_string, MCRangeMakeMinMax(p_var . mark . start, p_var . mark . finish));
            }
        }
	}
}

void MCEngineExecPutIntoVariable(MCExecContext& ctxt, MCExecValue p_value, int p_where, MCVariableChunkPtr p_var)
{
	p_var . variable -> clearuql();
	
	if (p_var . chunk == CT_UNDEFINED)
	{
        // SN-2014-04-11 [[ FasterVariables ]] Now chosing from here the position where to add a string on a variable
		if (p_where == PT_INTO)
			p_var . variable -> give_value(ctxt, p_value, kMCVariableSetInto);
		else if (p_where == PT_AFTER)
			p_var . variable -> give_value(ctxt, p_value, kMCVariableSetAfter);
        else
            p_var . variable -> give_value(ctxt, p_value, kMCVariableSetBefore);
	}
	else
    {
        if (MCValueGetTypeCode(p_var . mark . text) == kMCValueTypeCodeData &&
            MCExecTypeIsValueRef(p_value.type) &&
            MCValueGetTypeCode(p_value.valueref_value) == kMCValueTypeCodeData)
        {
            // AL-2014-11-20: Make sure the incoming exec value is released.
            MCAutoDataRef t_value_data;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeDataRef, &(&t_value_data));
            if (ctxt . HasError())
                return;
            
            MCEngineExecPutIntoVariable(ctxt, *t_value_data, p_where, p_var);
            return;
        }
        
        MCAutoStringRef t_value_string;
        MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_value_string));
        if (ctxt . HasError())
            return;
        
        MCEngineExecPutIntoVariable(ctxt, *t_value_string, p_where, p_var);
    }
}

void MCEngineExecReturn(MCExecContext& ctxt, MCValueRef p_value)
{
	ctxt.SetTheResultToValue(p_value);
}

void MCEngineExecReturnValue(MCExecContext& ctxt, MCValueRef p_value)
{
    ctxt.SetTheReturnValue(p_value);
}

void MCEngineExecReturnError(MCExecContext& ctxt, MCValueRef p_value)
{
    ctxt.SetTheReturnError(p_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineExecDo(MCExecContext& ctxt, MCStringRef p_script, int p_line, int p_pos)
{
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ctxt.SetLineAndPos(p_line, p_pos);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = True;
	}

    // SN-2015-06-03: [[ Bug 11277 ]] MCHandler::doscript refactored
    ctxt.doscript(ctxt, p_script, p_line, p_pos);

	if (added)
		MCnexecutioncontexts--;
}

void MCEngineExecDoInCaller(MCExecContext& ctxt, MCStringRef p_script, int p_line, int p_pos)
{
    Boolean added = False;
    if (MCnexecutioncontexts < MAX_CONTEXTS)
    {
        ctxt.SetLineAndPos(p_line, p_pos);
        MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
        added = True;
    }
    
    if (MCnexecutioncontexts < 2)
    {
        if (added)
            MCnexecutioncontexts--;
        ctxt . LegacyThrow(EE_DO_NOCALLER);
        return;
    }
    
    MCExecContext *caller = MCexecutioncontexts[MCnexecutioncontexts - 2];
    
    // SN-2015-06-03: [[ Bug 11277 ]] MCHandler::doscript refactored
    caller -> doscript(*caller, p_script, p_line, p_pos);
    
    if (added)
        MCnexecutioncontexts--;
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineExecQuit(MCExecContext& ctxt, integer_t p_retcode)
{
// MW-2011-06-22: [[ SERVER ]] Don't send messages in server-mode.
#ifndef _SERVER
    if (MCdefaultstackptr && !MCdefaultstackptr->getstate(CS_DELETE_STACK))
    {
        switch(MCdefaultstackptr->getcard()->message(MCM_shut_down_request))
        {
            case ES_PASS:
            case ES_NOT_HANDLED:
                break;
            default:
                return;
        }
        // IM-2013-05-01: [[ BZ 10586 ]] remove #ifdefs so this message is sent
        // here on Android in the same place as (almost) everything else
        MCdefaultstackptr->getcard()->message(MCM_shut_down);
    }
#endif

	MCretcode = p_retcode;
	MCquit = True;
	MCquitisexplicit = True;
	MCexitall = True;
	MCtracestackptr = nil;
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
	MCObjectList *olptr = new (nothrow) MCObjectList(p_script);
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
        bool t_stop;
		
		MCU_play();

        if (!ctxt . EvalExprAsBool(p_condition, EE_WAIT_BADEXP, t_stop))
            return;

        if (t_stop == true)
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
        bool t_continue;
		
		MCU_play();
		
        if (!ctxt . EvalExprAsBool(p_condition, EE_WAIT_BADEXP, t_continue))
            return;

        if (t_continue == false)
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
	if (!p_target->dofree(ctxt))
		ctxt . Throw();
}

void MCEngineExecDeleteVariableChunks(MCExecContext& ctxt, MCVariableChunkPtr *p_chunks, uindex_t p_chunk_count)
{
	for(uindex_t i = 0; i < p_chunk_count; i++)
    {
        /*
        MCAutoStringRef t_string;
        if (!ctxt . EvalExprAsMutableStringRef(p_chunks[i] . variable, EE_ENGINE_DELETE_BADVARCHUNK, &t_string))
            return;

        if (MCStringReplace(*t_string, MCRangeMakeMinMax(p_chunks[i] . mark . start, p_chunks[i] . mark . finish), kMCEmptyString))
        {
            p_chunks[i] . variable -> set(ctxt, *t_string, kMCVariableSetInto);
        } */
        // SN-2014-04-11 [[ FasterVariables ]] Deletiong of the content of a variable is now done without copying
        p_chunks[i] . variable -> deleterange(ctxt, MCRangeMakeMinMax(p_chunks[i] . mark . start, p_chunks[i] . mark . finish));
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
	if ((sptr = MCdefaultstackptr->findstackname_string(p_name)) == NULL)
    {
        ctxt . LegacyThrow(EE_START_BADTARGET);
        return;
    }
    
    // MW-2014-10-23: Throw a different error if the script won't compile.
    if (!sptr->parsescript(True))
    {
        ctxt . LegacyThrow(EE_START_WONTCOMPILE);
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
	if ((sptr = MCdefaultstackptr->findstackname_string(p_name)) == NULL)
		{
			ctxt . LegacyThrow(EE_START_BADTARGET);
			return;
		}
	MCEngineExecStopUsingStack(ctxt, sptr);
}
			        
///////////////////////////////////////////////////////////////////////////////

Exec_stat _MCEngineExecDoDispatch(MCExecContext &ctxt, int p_handler_type, MCNameRef p_message, MCObjectPtr *p_target, MCParameter *p_parameters)
{
	if (MCscreen -> abortkey())
	{
		ctxt . LegacyThrow(EE_HANDLER_ABORT);
		return ES_ERROR;
	}
	
	// Work out the target object
	MCObjectPartHandle t_object;
	if (p_target != nil)
		t_object = *p_target;
	else
		t_object = ctxt . GetObjectPtr();
		
	// Fetch current default stack and target settings
	MCStackHandle t_old_stack = MCdefaultstackptr;
	
	// Cache the current 'this stack' (used to see if we should switch back
	// the default stack).
	MCStack *t_this_stack;
	t_this_stack = t_object -> getstack();
	
	// Retarget this stack and the target to be relative to the target object
	MCdefaultstackptr = t_this_stack;
    MCObjectPartHandle t_old_target(t_object);
    swap(t_old_target, MCtargetptr);
    
	// MW-2012-10-30: [[ Bug 10478 ]] Turn off lockMessages before dispatch.
	Boolean t_old_lock;
	t_old_lock = MClockmessages;
	MClockmessages = False;
	
	// Add a new entry in the execution contexts
	MCExecContext *oldctxt = MCECptr;
	MCECptr = &ctxt;
	Exec_stat t_stat;
	t_stat = ES_NOT_HANDLED;
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = True;
	}

	// Dispatch the message
	t_stat = MCU_dofrontscripts((Handler_type)p_handler_type, p_message, p_parameters);
	Boolean olddynamic = MCdynamicpath;
	MCdynamicpath = MCdynamiccard.IsValid();
	if (t_stat == ES_PASS || t_stat == ES_NOT_HANDLED)
    {
        switch(t_stat = t_object -> handle((Handler_type)p_handler_type, p_message, p_parameters, t_object.Get()))
        {
        case ES_ERROR:
            ctxt . LegacyThrow(EE_DISPATCH_BADCOMMAND, p_message);
            break;
        default:
            break;
        }
    }
	
	// Reset the default stack pointer and target - note that we use 'send'esque
	// semantics here. i.e. If the default stack has been changed, the change sticks.
	if (t_old_stack.IsValid() &&
		MCdefaultstackptr == t_this_stack)
		MCdefaultstackptr = t_old_stack;

	// Reset target pointer
    swap(MCtargetptr, t_old_target);
	MCdynamicpath = olddynamic;
	
	// MW-2012-10-30: [[ Bug 10478 ]] Restore lockMessages.
	MClockmessages = t_old_lock;
	
	// Remove our entry from the contexts list
	MCECptr = oldctxt;
	if (added)
		MCnexecutioncontexts--;
	
	return t_stat;
}

void MCEngineExecDispatch(MCExecContext& ctxt, int p_handler_type, MCNameRef p_message, MCObjectPtr *p_target, MCParameter *p_parameters)
{
	Exec_stat t_stat;
	t_stat = _MCEngineExecDoDispatch(ctxt, p_handler_type, p_message, p_target, p_parameters);
	
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
}

///////////////////////////////////////////////////////////////////////////////

static void MCEngineSplitScriptIntoMessageAndParameters(MCExecContext& ctxt, MCStringRef p_script, MCNameRef& r_message, MCParameter*& r_params)
{
	MCParameter *params = NULL;
	MCParameter *tparam = NULL;
	
    uindex_t t_offset;
    t_offset = 0;
    
    uindex_t t_length;
    t_length = MCStringGetLength(p_script);
    
	while (t_offset < t_length && !isspace(MCStringGetCharAtIndex(p_script, t_offset)))
		t_offset++;
		
    MCRange t_msg_range;
    t_msg_range = MCRangeMake(0, t_offset);
    t_offset++;
    
	MCerrorlock++;
    unichar_t t_char = '\0';
    uindex_t t_start_offset;
    t_start_offset = t_offset;
    
    MCRange t_exp_range;
    
	while (t_offset <= t_length)
	{
        if (t_offset < t_length)
            t_char = MCStringGetCharAtIndex(p_script, t_offset);
        
        if (t_offset == t_length || t_char == ',')
        {
            t_exp_range = MCRangeMakeMinMax(t_start_offset, t_offset);

            MCAutoStringRef t_expression;
            /* UNCHECKED */ MCStringCopySubstring(p_script, t_exp_range, &t_expression);
            
            MCParameter *newparam = new (nothrow) MCParameter;
            
            // MW-2011-08-11: [[ Bug 9668 ]] Make sure we copy 'pdata' if we use it, since
            //   mptr (into which it points) only lasts as long as this method call.
            // SN-2015-06-03: [[ Bug 11277 ]] MCHandler::eval_ctxt refactored
            MCExecValue t_value;
            ctxt . eval_ctxt(ctxt, *t_expression, t_value);
            if (!ctxt.HasError())
                newparam->give_exec_argument(t_value);
            else
                newparam->setvalueref_argument(*t_expression);
            
            // Not being able to evaluate the parameter doesn't cause an error at this stage
            ctxt.IgnoreLastError();
            
            if (tparam == NULL)
                params = tparam = newparam;
            else
            {
                tparam->setnext(newparam);
                tparam = newparam;
            }
            t_start_offset = ++t_offset;
        }
        else if (t_char == '"')
        {
            while (t_offset < t_length && MCStringGetCharAtIndex(p_script, ++t_offset) != '"')
                ;
            // AL-2014-05-28: [[ Bug 12544 ]] Increment offset past closing quotation mark
            t_offset++;
        }
        else
            t_offset++;
	}
	MCerrorlock--;
	
    MCAutoStringRef t_msg;
    /* UNCHECKED */ MCStringCopySubstring(p_script, t_msg_range, &t_msg);
	/* UNCHECKED */ MCNameCreate(*t_msg, r_message);
	r_params = params;
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
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = True;
	}
	if ((stat = optr->message(*t_message, t_params, p_is_send, True)) == ES_NOT_HANDLED)
	{
        // The message was not handled by the target object, so this is
        // just a bunch of script to be executed as if it were in a handler
        // in the target object (using domess).
        
		MCHandler *t_handler;
		t_handler = optr -> findhandler(HT_MESSAGE, *t_message);
		if (t_handler != NULL && t_handler -> isprivate())
        {
			ctxt . LegacyThrow(EE_SEND_BADEXP, *t_message);
            goto cleanup;
        }

        // The 'split into message and parameters' function above is used
        // to ensure that all the parameters are evaluated in the current
        // context (not the target). Since domess just takes a string, we
        // convert the entire script back into a string with the params
        // having been evaluated. This means in particular that variables
        // containing arrays will not work here - they will be converted to
        // the empty string.
        
        MCAutoListRef t_param_list;
        MCListCreateMutable(',', &t_param_list);
        MCParameter *t_param_ptr;
        t_param_ptr = t_params;
        
        bool t_has_params;
        t_has_params = t_params != nil;
        while (t_param_ptr != NULL)
        {
            MCAutoValueRef t_value;
            MCAutoStringRef t_value_string;
            
            if (!t_param_ptr->eval_argument(ctxt, &t_value) ||
                !ctxt . ConvertToString(*t_value, &t_value_string) ||
                !MCListAppend(*t_param_list, *t_value_string))
                goto cleanup;

            t_param_ptr = t_param_ptr -> getnext();
        }
        
        MCAutoStringRef tptr;
        if (t_has_params)
        {
            MCAutoStringRef t_params_string;
            if (!MCListCopyAsString(*t_param_list, &t_params_string) ||
                !MCStringCreateWithStringsAndSeparator(&tptr, ' ',
                                                       MCNameGetString(*t_message),
                                                       *t_params_string))
                goto cleanup;
        }
        else
            tptr = MCNameGetString(*t_message);
        
        if (optr->domess(*tptr, nil, false) == ES_ERROR)
            ctxt . Throw();
	}
	else if (stat == ES_PASS)
		stat = ES_NORMAL;
	else if (stat == ES_ERROR)
		ctxt . LegacyThrow(EE_SEND_BADEXP, *t_message);

cleanup:
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

void MCEngineExecSendScript(MCExecContext& ctxt, MCStringRef p_script, MCObjectPtr *p_target)
{
    MCObject *optr;
    if (p_target == nil)
        optr = ctxt . GetObject();
    else
        optr = p_target -> object;
    
    Boolean oldlock = MClockmessages;
    MClockmessages = False;

    Boolean added = False;
    if (MCnexecutioncontexts < MAX_CONTEXTS)
    {
        MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
        added = True;
    }
    
    if (optr->domess(p_script, nil, false) == ES_ERROR)
        ctxt . Throw();
    
	if (added)
		MCnexecutioncontexts--;
	MClockmessages = oldlock;
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
    
    // AL-2014-07-22: [[ Bug 12846 ]] Copy bugfix to refactored code
    // MW-2014-05-28: [[ Bug 12463 ]] If we cannot add the pending message, then throw an
    //   error.
	if (MCscreen->addusermessage(p_target . object, *t_message, MCS_time() + p_delay, t_params))
        return;
    
    ctxt . LegacyThrow(EE_SEND_TOOMANYPENDING, *t_message);
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

void MCEngineGetFormSensitive(MCExecContext& ctxt, bool& r_value)
{
    r_value = ctxt . GetFormSensitive();
}

void MCEngineSetFormSensitive(MCExecContext& ctxt, bool p_value)
{
    ctxt . SetFormSensitive(p_value);
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

void MCEngineSetItemDelimiter(MCExecContext& ctxt, MCStringRef p_value)
{
	ctxt . SetItemDelimiter(p_value);
}

void MCEngineGetItemDelimiter(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(ctxt . GetItemDelimiter());
}

void MCEngineSetLineDelimiter(MCExecContext& ctxt, MCStringRef p_value)
{
	ctxt . SetLineDelimiter(p_value);
}

void MCEngineGetLineDelimiter(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(ctxt . GetLineDelimiter());
}

void MCEngineSetColumnDelimiter(MCExecContext& ctxt, MCStringRef p_value)
{
	ctxt . SetColumnDelimiter(p_value);
}

void MCEngineGetColumnDelimiter(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(ctxt . GetColumnDelimiter());
}

void MCEngineSetRowDelimiter(MCExecContext& ctxt, MCStringRef p_value)
{
	ctxt . SetRowDelimiter(p_value);
}

void MCEngineGetRowDelimiter(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(ctxt . GetRowDelimiter());
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
	ctxt . SetNumberFormat(p_format . fw, p_format . trailing, p_format . force);
}

void MCEngineGetNumberFormat(MCExecContext& ctxt, MCEngineNumberFormat& r_format)
{
	r_format . fw = ctxt . GetNumberFormatWidth();
	r_format . trailing = ctxt . GetNumberFormatTrailing();
	r_format . force = ctxt . GetNumberFormatForce();
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
	r_value = ~MCsecuremode;
}

void MCEngineSetSecurityPermissions(MCExecContext& ctxt, intset_t p_value)
{
	// MW-2013-11-05: [[ Bug 11114 ]] Reinstate ability to set the securityPermissions.
    MCsecuremode |= (~p_value) & MC_SECUREMODE_ALL;
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
			MCAutoValueRef t_stack_name;
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
    MCExecContext ctxt(nil, nil, nil);
    MCAutoStringRef t_string;
    ctxt . ConvertToString(p_value, &t_string);
    MCScriptPoint sp(ctxt, *t_string);

    MCChunk *tchunk = new (nothrow) MCChunk(False);
    MCerrorlock++;
    Symbol_type type;
    
    bool t_parse_error;
    bool t_success;
    t_parse_error = tchunk->parse(sp, False) == PS_NORMAL;
    t_success = (!t_parse_error && (!p_strict || sp.next(type) == PS_EOF));

    MCerrorlock--;
    if (t_success)
        t_success = tchunk->getobj(ctxt, r_object, False);
    delete tchunk;
    
    r_parse_error = t_parse_error;
    return t_success;
}

void MCEngineEvalValueAsObject(MCExecContext& ctxt, MCValueRef p_value, MCObjectPtr& r_object)
{
    bool t_parse_error;
    if (!MCEngineEvalValueAsObject(p_value, true, r_object, t_parse_error))
        ctxt . LegacyThrow(EE_CHUNK_BADOBJECTEXP);
}

void MCEngineEvalOwnerAsObject(MCExecContext& ctxt, MCObjectPtr p_object, MCObjectPtr& r_owner)
{
    if (!(p_object . object -> gettype() == CT_STACK && MCdispatcher -> ismainstack(static_cast<MCStack *>(p_object . object))))
    {
        r_owner . object = p_object . object -> getparent();
        // SN-2015-01-13: [[ Bug 14376 ]] Let's get the parid of the owner, as in pre-7.0
        r_owner . part_id  = p_object . part_id;
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
            t_object = (MCObject *)MCtemplatestack;
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
    
    if (ctxt . GetParentScript() == NULL)
        r_object . object = nil; // destobj!
    else
        r_object = ctxt . GetObjectPtr();
}

void MCEngineEvalMenuObjectAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCmenuobjectptr)
    {
        r_object . object = MCmenuobjectptr;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCEngineEvalTargetAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCtargetptr)
    {
        r_object = MCtargetptr.getObjectPtr();
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCEngineEvalErrorObjectAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCerrorptr)
    {
        r_object . object = MCerrorptr;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCEngineMarkVariable(MCExecContext& ctxt, MCVarref *p_variable, bool p_data, MCMarkedText& r_mark)
{
    if (p_variable == nil)
	{
		ctxt . LegacyThrow(EE_CHUNK_BADCONTAINER);
		return;
	}
    // AL-2014-09-10: [[ Bug 13400 ]] Keep marked strings the correct type where possible
    if (p_data)
    {
        if (!ctxt . EvalExprAsDataRef(p_variable, EE_CHUNK_SETCANTGETDEST, (MCDataRef&)r_mark . text))
            return;
        r_mark . start = 0;
        r_mark . finish = MCDataGetLength((MCDataRef)r_mark . text);
    }
    else
    {
        if (!ctxt . EvalExprAsStringRef(p_variable, EE_CHUNK_SETCANTGETDEST, (MCStringRef&)r_mark . text))
            return;
        r_mark . start = 0;
        r_mark . finish = MCStringGetLength((MCStringRef)r_mark . text);
    }
}

///////////////////////////////////////////////////////////////////////////////

static bool MCEngineUuidToStringRef(MCUuid p_uuid, MCStringRef& r_string)
{
    // Convert the uuid to a string.
	char t_uuid_buffer[kMCUuidCStringLength];
	MCUuidToCString(p_uuid, t_uuid_buffer);
    
    return MCStringCreateWithNativeChars((const char_t *)t_uuid_buffer, kMCUuidCStringLength - 1, r_string);
}

void MCEngineEvalRandomUuid(MCExecContext& ctxt, MCStringRef& r_uuid)
{
    MCUuid t_uuid;
    if (!MCUuidGenerateRandom(t_uuid))
    {
        ctxt . LegacyThrow(EE_UUID_NORANDOMNESS);
        return;
    }
    
    if (MCEngineUuidToStringRef(t_uuid, r_uuid))
        return;
    
    ctxt . Throw();
}

void MCEngineDoEvalUuid(MCExecContext& ctxt, MCStringRef p_namespace_id, MCStringRef p_name, bool p_is_md5, MCStringRef& r_uuid)
{
    MCUuid t_namespace, t_uuid;
    // Attempt to convert it to a uuid.
    MCAutoPointer<char> t_namespace_id;
    /* UNCHECKED */ MCStringConvertToCString(p_namespace_id, &t_namespace_id);
    
    if (!MCUuidFromCString(*t_namespace_id, t_namespace))
    {
        ctxt . LegacyThrow(EE_UUID_NAMESPACENOTAUUID);
        return;
    }
    
    if (p_is_md5)
        MCUuidGenerateMD5(t_namespace, p_name, t_uuid);
    else
        MCUuidGenerateSHA1(t_namespace, p_name, t_uuid);
    
    if (MCEngineUuidToStringRef(t_uuid, r_uuid))
        return;
    
    ctxt . Throw();
}

void MCEngineEvalMD5Uuid(MCExecContext& ctxt, MCStringRef p_namespace_id, MCStringRef p_name, MCStringRef& r_uuid)
{
    MCEngineDoEvalUuid(ctxt, p_namespace_id, p_name, true, r_uuid);
}

void MCEngineEvalSHA1Uuid(MCExecContext& ctxt, MCStringRef p_namespace_id, MCStringRef p_name, MCStringRef& r_uuid)
{
    MCEngineDoEvalUuid(ctxt, p_namespace_id, p_name, false, r_uuid);
}

void MCEngineGetEditionType(MCExecContext& ctxt, MCStringRef& r_edition)
{
    if (!MCStringFromLicenseClass(MClicenseparameters.license_class,
                                     true,
                                     r_edition))
    {
        ctxt . Throw();
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalIsStrictlyNothing(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = MCValueGetTypeCode(value) == kMCValueTypeCodeNull;
}

void MCEngineEvalIsNotStrictlyNothing(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = MCValueGetTypeCode(value) != kMCValueTypeCodeNull;
}

void MCEngineEvalIsStrictlyABoolean(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = MCValueGetTypeCode(value) == kMCValueTypeCodeBoolean;
}

void MCEngineEvalIsNotStrictlyABoolean(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = MCValueGetTypeCode(value) != kMCValueTypeCodeBoolean;
}

void MCEngineEvalIsStrictlyAnInteger(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = MCValueGetTypeCode(value) == kMCValueTypeCodeNumber &&
                MCNumberIsInteger((MCNumberRef)value);
}

void MCEngineEvalIsNotStrictlyAnInteger(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = !(MCValueGetTypeCode(value) == kMCValueTypeCodeNumber &&
                 MCNumberIsInteger((MCNumberRef)value));
}

void MCEngineEvalIsStrictlyAReal(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = MCValueGetTypeCode(value) == kMCValueTypeCodeNumber &&
                MCNumberIsReal((MCNumberRef)value);
}

void MCEngineEvalIsNotStrictlyAReal(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = !(MCValueGetTypeCode(value) == kMCValueTypeCodeNumber &&
                 MCNumberIsReal((MCNumberRef)value));
}

void MCEngineEvalIsStrictlyAString(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = MCValueGetTypeCode(value) == kMCValueTypeCodeString ||
                MCValueGetTypeCode(value) == kMCValueTypeCodeName;
}

void MCEngineEvalIsNotStrictlyAString(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = !(MCValueGetTypeCode(value) == kMCValueTypeCodeString ||
                 MCValueGetTypeCode(value) == kMCValueTypeCodeName);
}

void MCEngineEvalIsStrictlyABinaryString(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = MCValueGetTypeCode(value) == kMCValueTypeCodeData;
}

void MCEngineEvalIsNotStrictlyABinaryString(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = MCValueGetTypeCode(value) != kMCValueTypeCodeData;
}

void MCEngineEvalIsStrictlyAnArray(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = MCValueGetTypeCode(value) == kMCValueTypeCodeArray;
}

void MCEngineEvalIsNotStrictlyAnArray(MCExecContext& ctxt, MCValueRef value, bool& r_result)
{
    r_result = MCValueGetTypeCode(value) != kMCValueTypeCodeArray;
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineEvalCommandName(MCExecContext& ctxt, MCStringRef& r_result)
{
    if (MCcommandname != nullptr)
        r_result = MCValueRetain(MCcommandname);
    else
        r_result = MCValueRetain(kMCEmptyString);
}

void MCEngineEvalCommandArguments(MCExecContext& ctxt, MCArrayRef& r_result)
{
    r_result = MCValueRetain(MCcommandarguments);
}

void MCEngineEvalCommandArgumentAtIndex(MCExecContext& ctxt, uinteger_t t_index, MCStringRef& r_result)
{
    if (t_index == 0)
    {
        ctxt . LegacyThrow(EE_COMMANDARGUMENTS_BADPARAM);
        return;
    }
    
    MCStringRef t_result = nullptr;
    // If the index > argument count then we return empty
    if (!MCArrayFetchValueAtIndex(MCcommandarguments, t_index, (MCValueRef&)t_result))
        t_result = kMCEmptyString;

    r_result = MCValueRetain(t_result);
}

////////////////////////////////////////////////////////////////////////////////

void MCEngineGetRevLibraryMappingByKey(MCExecContext& ctxt, MCNameRef p_library, MCStringRef& r_mapping)
{
    MCArrayRef t_mappings = MCdispatcher->getlibrarymappings();
    
    MCStringRef t_value = nullptr;
    // m_library_mapping only stores strings (function above)
    if (!MCArrayFetchValue(t_mappings, false, p_library, (MCValueRef&)t_value) ||
        MCStringIsEmpty(t_value))
    {
        ctxt . LegacyThrow(EE_BAD_LIBRARY_MAPPING);
        return;
    }
    
    r_mapping = MCValueRetain(t_value);
}

void MCEngineSetRevLibraryMappingByKey(MCExecContext& ctxt, MCNameRef p_library, MCStringRef p_mapping)
{
    MCArrayRef t_mappings = MCdispatcher->getlibrarymappings();
    if (!MCArrayStoreValue(t_mappings, false, p_library, p_mapping))
    {
        ctxt . LegacyThrow(EE_BAD_LIBRARY_MAPPING);
        return;
    }
}

