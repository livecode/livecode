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

#include "hndlrlst.h"
#include "scriptpt.h"
#include "handler.h"
#include "param.h"
#include "funcs.h"
#include "chunk.h"
#include "object.h"
#include "field.h"
#include "image.h"
#include "button.h"
#include "card.h"
#include "stack.h"
#include "aclip.h"
#include "player.h"
#include "dispatch.h"
#include "stacklst.h"
#include "sellst.h"
#include "mcerror.h"
#include "util.h"
#include "date.h"
#include "regex.h"
#include "scriptenvironment.h"
#include "securemode.h"
#include "osspec.h"
#include "flst.h"

#include "socket.h"
#include "mcssl.h"

#include "globals.h"
#include "license.h"
#include "mode.h"
#include "stacksecurity.h"
#include "uuid.h"
#include "font.h"

#include "exec.h"

#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

Parse_stat MCFunction::parse(MCScriptPoint &sp, Boolean the)
{
	if (!the)
	{
		if (get0params(sp) != PS_NORMAL)
		{
			MCperror->add
			(PE_FUNCTION_BADFORM, sp);
			return PS_ERROR;
		}
	}
	else
		initpoint(sp);
	return PS_NORMAL;
}

Parse_stat MCFunction::parsetarget(MCScriptPoint &sp, Boolean the,
                                   Boolean needone, MCChunk *&object)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_OF) == PS_NORMAL)
	{
		object = new (nothrow) MCChunk(False);
		if (object->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_FUNCTION_BADOBJECT, sp);
			return PS_ERROR;
		}
	}
	else
		if (needone || !the)
		{
			if (sp.skip_token(SP_FACTOR, TT_LPAREN) != PS_NORMAL)
			{
				MCperror->add
				(PE_FACTOR_NOLPAREN, sp);
				return PS_ERROR;
			}
			if (!needone && sp.skip_token(SP_FACTOR, TT_RPAREN) == PS_NORMAL)
				return PS_NORMAL;
			object = new (nothrow) MCChunk(False);
			if (object->parse(sp, False) != PS_NORMAL)
			{
				MCperror->add
				(PE_FUNCTION_BADOBJECT, sp);
				return PS_ERROR;
			}
			if (sp.skip_token(SP_FACTOR, TT_RPAREN) != PS_NORMAL)
			{
				MCperror->add
				(PE_FACTOR_NORPAREN, sp);
				return PS_ERROR;
			}
		}
	return PS_NORMAL;
}

////

MCArrayEncode::~MCArrayEncode()
{
	delete source;
    delete version;
}

Parse_stat MCArrayEncode::parse(MCScriptPoint &sp, Boolean the)
{
	if (get1or2params(sp, &source, &version, the) != PS_NORMAL)
	{
		MCperror->add(PE_ARRAYENCODE_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCArrayEncode::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{    
    MCAutoArrayRef t_array;
    if (!ctxt . EvalExprAsArrayRef(source, EE_ARRAYENCODE_BADSOURCE, &t_array))
        return;
    
    // AL-2014-05-15: [[ Bug 12203 ]] Add version parameter to arrayEncode, to allow
    //  version 7.0 variant to preserve unicode.
    MCAutoStringRef t_version;
    if (!ctxt . EvalOptionalExprAsNullableStringRef(version, EE_ARRAYENCODE_BADSOURCE, &t_version))
        return;
    
	MCArraysEvalArrayEncode(ctxt, *t_array, *t_version, r_value . dataref_value);
    
    if (!ctxt . HasError())
        r_value . type = kMCExecValueTypeDataRef;
}

MCBaseConvert::~MCBaseConvert()
{
	delete source;
	delete sourcebase;
	delete destbase;
}

Parse_stat MCBaseConvert::parse(MCScriptPoint &sp, Boolean the)
{
	if (get2or3params(sp, &source, &sourcebase, &destbase) != PS_NORMAL
	        || destbase == NULL)
	{
		MCperror->add
		(PE_BASECONVERT_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCBaseConvert::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    uinteger_t dbase;
    if (!ctxt . EvalExprAsUInt(destbase, EE_BASECONVERT_BADDESTBASE, dbase))
        return;
    
    uinteger_t sbase;
    if (!ctxt . EvalExprAsUInt(sourcebase, EE_BASECONVERT_BADSOURCEBASE, sbase))
        return;
    
    MCAutoStringRef t_source;
    if (!ctxt . EvalExprAsStringRef(source, EE_BASECONVERT_BADSOURCE, &t_source))
        return;
    
	MCMathEvalBaseConvert(ctxt, *t_source, sbase, dbase, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

MCBinaryDecode::~MCBinaryDecode()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCBinaryDecode::parse(MCScriptPoint &sp, Boolean the)
{
	if (getparams(sp, &params) != PS_NORMAL || params == NULL)
	{
		MCperror->add(PE_BINARYD_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCBinaryDecode::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCAutoStringRef t_format;
    MCAutoValueRef t_format_valueref;
	MCParameter *t_params = nil;
    
    if (!params->eval(ctxt, &t_format_valueref) || !ctxt . ConvertToString(*t_format_valueref, &t_format))
	{
		ctxt . LegacyThrow(EE_BINARYD_BADSOURCE);
		return;
	}
    
    MCAutoValueRef t_data_valueref;
    MCAutoDataRef t_data;
    if (params->getnext() != nil)
	{
		if (!params->getnext()->eval(ctxt, &t_data_valueref) || !ctxt .ConvertToData(*t_data_valueref, &t_data))
		{
			ctxt . LegacyThrow(EE_BINARYD_BADPARAM);
			return;
		}
		t_params = params->getnext()->getnext();
	}
    uinteger_t t_result_count = 0;
    
	for (MCParameter *p = t_params; p != nil; p = p->getnext())
		t_result_count++;
    
	MCAutoValueRefArray t_results;
	/* UNCHECKED */ t_results.New(t_result_count);
    
    MCFiltersEvalBinaryDecode(ctxt, *t_format, *t_data, *t_results, t_result_count, r_value . int_value);
    r_value . type = kMCExecValueTypeInt;

    if (!ctxt.HasError())
    {
        uinteger_t t_skipped;
        t_skipped = 0;
        for (uindex_t i = 0; i < t_result_count && (integer_t) i < r_value . int_value; i++)
        {
            if (t_results[i] != nil)
            {
                // AL-2014-09-09: [[ Bug 13359 ]] Make sure containers are used in case a param is a handler variable
                // AL-2014-09-18: [[ Bug 13465 ]] Use auto class to prevent memory leak
                MCContainer t_container;
                if (!t_params->evalcontainer(ctxt, t_container))
                {
                    ctxt . LegacyThrow(EE_BINARYD_BADDEST);
                    return;
                }
                
                /* UNCHECKED */ t_container.set_valueref(t_results[i]);
            }
            else
            {
                t_skipped++;
            }
            t_params = t_params->getnext();
        }
        
        // Account for the skipped ("x") parameters
        if ((integer_t) t_skipped >= r_value . int_value)
            r_value . int_value = 0;
        else
            r_value . int_value -= t_skipped;
    }
}

MCBinaryEncode::~MCBinaryEncode()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCBinaryEncode::parse(MCScriptPoint &sp, Boolean the)
{
	if (getparams(sp, &params) != PS_NORMAL || params == NULL)
	{
		MCperror->add(PE_BINARYE_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCBinaryEncode::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    MCAutoValueRef t_format_valueref;
    MCAutoStringRef t_format;
	MCAutoValueRefArray t_values;
	uindex_t t_value_count = 0;
    if (!params->eval(ctxt, &t_format_valueref) || !ctxt . ConvertToString(*t_format_valueref, &t_format))
	{
		ctxt . LegacyThrow(EE_BINARYE_BADSOURCE);
		return;
	}
    MCParameter *t_params = params->getnext();
	for (MCParameter *p = t_params; p != nil; p = p->getnext())
		t_value_count++;
    
	/* UNCHECKED */ t_values.New(t_value_count);
	for (uindex_t i = 0; i < t_value_count; i++)
	{
		if (!t_params->eval(ctxt, t_values[i]))
		{
			ctxt . LegacyThrow(EE_BINARYE_BADPARAM);
			return;
		}
        
		t_params = t_params->getnext();
	}

    MCFiltersEvalBinaryEncode(ctxt, *t_format, *t_values, t_value_count, r_value . dataref_value);
    r_value . type = kMCExecValueTypeDataRef;
}

MCChunkOffset::~MCChunkOffset()
{
	delete part;
	delete whole;
	delete offset;
}

Parse_stat MCChunkOffset::parse(MCScriptPoint &sp, Boolean the)
{
	if (get2or3params(sp, &part, &whole, &offset) != PS_NORMAL)
	{
		MCperror->add(PE_OFFSET_BADPARAMS, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCChunkOffset::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    uinteger_t t_start;
    if (!ctxt . EvalOptionalExprAsUInt(offset, 0, EE_OFFSET_BADOFFSET, t_start))
        return;

    if (delimiter == CT_BYTE)
    {
        MCAutoDataRef t_chunk;
        if (!ctxt . EvalExprAsDataRef(part, EE_OFFSET_BADPART, &t_chunk))
            return;
        
        MCAutoDataRef t_string;
        if (!ctxt . EvalExprAsDataRef(whole, EE_OFFSET_BADWHOLE, &t_string))
            return;
        
        MCStringsEvalByteOffset(ctxt, *t_chunk, *t_string, t_start, r_value . uint_value);
        r_value . type = kMCExecValueTypeUInt;
        return;
    }
    
    MCAutoStringRef t_chunk;
    if (!ctxt . EvalExprAsStringRef(part, EE_OFFSET_BADPART, &t_chunk))
        return;
    
    MCAutoStringRef t_string;
    if (!ctxt . EvalExprAsStringRef(whole, EE_OFFSET_BADWHOLE, &t_string))
        return;
    
	switch (delimiter)
	{
	case CT_ITEM:
		MCStringsEvalItemOffset(ctxt, *t_chunk, *t_string, t_start, r_value . uint_value);
		break;
	case CT_LINE:
		MCStringsEvalLineOffset(ctxt, *t_chunk, *t_string, t_start, r_value . uint_value);
		break;
	case CT_WORD:
		MCStringsEvalWordOffset(ctxt, *t_chunk, *t_string, t_start, r_value . uint_value);
		break;
    case CT_TOKEN:
        MCStringsEvalTokenOffset(ctxt, *t_chunk, *t_string, t_start, r_value . uint_value);
        break;
	case CT_CHARACTER:
		MCStringsEvalOffset(ctxt, *t_chunk, *t_string, t_start, r_value . uint_value);
		break;
    case CT_PARAGRAPH:
        MCStringsEvalParagraphOffset(ctxt, *t_chunk, *t_string, t_start, r_value . uint_value);
        break;
    case CT_SENTENCE:
        MCStringsEvalSentenceOffset(ctxt, *t_chunk, *t_string, t_start, r_value . uint_value);
        break;
    case CT_TRUEWORD:
        MCStringsEvalTrueWordOffset(ctxt, *t_chunk, *t_string, t_start, r_value . uint_value);
        break;
    case CT_CODEPOINT:
        MCStringsEvalCodepointOffset(ctxt, *t_chunk, *t_string, t_start, r_value . uint_value);
        break;
    case CT_CODEUNIT:
        MCStringsEvalCodeunitOffset(ctxt, *t_chunk, *t_string, t_start, r_value . uint_value);
        break;
	default:
		MCUnreachable();
		break;
	}
    
    r_value . type = kMCExecValueTypeUInt;
}

Parse_stat MCCommandArguments::parse(MCScriptPoint &sp, Boolean the)
{
    if (!get0or1param(sp, &(&argument_index), the))
    {
        MCperror -> add(PE_FACTOR_BADPARAM, line, pos);
        return PS_ERROR;
    }

    return PS_NORMAL;
}

void MCCommandArguments::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    // If no parameter has been provided, then we return the list of parameters
    //  as an array.
    if (*argument_index == nullptr)
    {
        MCEngineEvalCommandArguments(ctxt, r_value.arrayref_value);
        r_value.type = kMCExecValueTypeArrayRef;
    }
    else
    {
        uinteger_t t_index;
        if (!ctxt . EvalExprAsUInt(*argument_index, EE_COMMANDARGUMENTS_BADPARAM, t_index))
            return;

        MCEngineEvalCommandArgumentAtIndex(ctxt, t_index, r_value . stringref_value);
        r_value.type = kMCExecValueTypeStringRef;
    }
}

void MCCommandName::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    MCEngineEvalCommandName(ctxt, r_value.stringref_value);
    r_value.type = kMCExecValueTypeStringRef;
}


MCDriverNames::~MCDriverNames()
{
	delete type;
}

Parse_stat MCDriverNames::parse(MCScriptPoint &sp, Boolean the)
{
	if (!the)
	{
		if (get0or1param(sp, &type, the) != PS_NORMAL)
		{
			MCperror->add(PE_DRIVERNAMES_BADPARAM, sp);
			return PS_ERROR;
		}
	}
	else
		initpoint(sp);
	return PS_NORMAL;
}


MCDrives::~MCDrives()
{
	delete type;
}

Parse_stat MCDrives::parse(MCScriptPoint &sp, Boolean the)
{
	if (!the)
	{
		if (get0or1param(sp, &type, the) != PS_NORMAL)
		{
			MCperror->add(PE_DRIVES_BADPARAM, sp);
			return PS_ERROR;
		}
	}
	else
		initpoint(sp);
	return PS_NORMAL;
}


MCExists::~MCExists()
{
	delete object;
}

Parse_stat MCExists::parse(MCScriptPoint &sp, Boolean the)
{
	return parsetarget(sp, the, True, object);
}

void MCExists::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCInterfaceEvalThereIsAnObject(ctxt, object, r_value . bool_value);
    r_value . type = kMCExecValueTypeBool;
}


MCFileItems::~MCFileItems()
{
	delete m_folder;
    delete m_kind;
}

Parse_stat
MCFileItems::parse(MCScriptPoint & sp, Boolean p_is_the)
{
	if (p_is_the)
	{
		initpoint(sp);
	}
	else
	{
		if (PS_NORMAL != get0or1or2params(sp, &m_folder, &m_kind, p_is_the))
		{
			MCperror->add(m_files ? PE_FILES_BADPARAM : PE_FOLDERS_BADPARAM, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

void
MCFileItems::eval_ctxt(MCExecContext & ctxt, MCExecValue & r_value)
{
    MCAutoStringRef t_folder;
    bool t_is_detailed = false;
    bool t_is_utf8 = false;
    if (m_folder)
    {
		if (!ctxt.EvalExprAsStringRef(m_folder, m_files ? EE_FILES_BADFOLDER : EE_FOLDERS_BADFOLDER, &t_folder))
			return;

        if (m_kind != nullptr)
        {
            MCAutoStringRef t_kind;
            if (!ctxt.EvalExprAsStringRef(m_kind, m_files ? EE_FILES_BADKIND : EE_FOLDERS_BADKIND, &t_kind))
            {
                return;
            }
            if (!MCStringIsEmpty(*t_kind))
            {
                if (MCStringIsEqualToCString(*t_kind, "detailed", kMCStringOptionCompareCaseless))
                {
                    t_is_detailed = true;
                }
                else if (MCStringIsEqualToCString(*t_kind, "detailed-utf8", kMCStringOptionCompareCaseless))
                {
                    t_is_detailed = true;
                    t_is_utf8 = true;
                }
                else
                {
                    ctxt.LegacyThrow(m_files ? EE_FILES_BADKIND : EE_FOLDERS_BADKIND);
                    return;
                }
            }
        }
    }
    
    r_value.type = kMCExecValueTypeStringRef;
    MCFilesEvalFileItemsOfDirectory(ctxt, *t_folder, m_files, t_is_detailed, t_is_utf8, r_value.stringref_value);
}


MCFontNames::~MCFontNames()
{
	delete type;
}

Parse_stat MCFontNames::parse(MCScriptPoint &sp, Boolean the)
{
	if (!the)
	{
		if (get0or1param(sp, &type, the) != PS_NORMAL)
		{
			MCperror->add(PE_FONTNAMES_BADPARAM, sp);
			return PS_ERROR;
		}
	}
	else
		initpoint(sp);
	return PS_NORMAL;
}


void MCFontNames::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCAutoStringRef t_type;
    MCAutoStringRef t_result;
    if (!ctxt . EvalOptionalExprAsStringRef(type, kMCEmptyString, EE_FONTNAMES_BADTYPE, &t_type))
        return;

    MCTextEvalFontNames(ctxt, *t_type, &t_result);

    if (!ctxt . HasError())
    {
        r_value . type = kMCExecValueTypeStringRef;
        r_value . stringref_value = MCValueRetain(*t_result);
    }
}

MCFontStyles::~MCFontStyles()
{
	delete fontname;
	delete fontsize;
}

Parse_stat MCFontStyles::parse(MCScriptPoint &sp, Boolean the)
{
	if (get2params(sp, &fontname, &fontsize) != PS_NORMAL)
	{
		MCperror->add(PE_FONTSTYLES_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCFontStyles::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCAutoStringRef t_fontname;
    if (!ctxt . EvalExprAsStringRef(fontname, EE_FONTSTYLES_BADFONTNAME, &t_fontname))
        return;
    uinteger_t fsize;
    if (!ctxt . EvalExprAsStrictUInt(fontsize, EE_FONTSTYLES_BADFONTSIZE, fsize))
        return;
    
	MCTextEvalFontStyles(ctxt, *t_fontname, fsize, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

MCFormat::~MCFormat()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCFormat::parse(MCScriptPoint &sp, Boolean the)
{
	sp.allowescapes(True);
	if (getparams(sp, &params) != PS_NORMAL || params == NULL)
	{
		MCperror->add
		(PE_FORMAT_BADPARAM, line, pos);
		return PS_ERROR;
	}
	sp.allowescapes(False);
	return PS_NORMAL;
}

#define INT_VALUE 0
#define PTR_VALUE 1
#define DOUBLE_VALUE 2

void MCFormat::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCAutoValueRef t_format_valueref;
    MCAutoStringRef t_format;
	MCAutoValueRefArray t_values;
	uindex_t t_value_count = 0;
    if (!params->eval(ctxt, &t_format_valueref) || !ctxt . ConvertToString(*t_format_valueref, &t_format))
	{
		ctxt . LegacyThrow(EE_FORMAT_BADSOURCE);
		return;
	}

	MCParameter *t_params = params->getnext();
	for (MCParameter *p = t_params; p != nil; p = p->getnext())
		t_value_count++;

	/* UNCHECKED */ t_values.New(t_value_count);
	for (uindex_t i = 0; i < t_value_count; i++)
	{
		if (!t_params->eval(ctxt, t_values[i]))
		{
			ctxt . LegacyThrow(EE_FORMAT_BADSOURCE);
			return;
		}

		t_params = t_params->getnext();
	}

	MCStringsEvalFormat(ctxt, *t_format, *t_values, t_value_count, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

MCHostNtoA::~MCHostNtoA()
{
	delete name;
	delete message;
}

Parse_stat MCHostNtoA::parse(MCScriptPoint &sp, Boolean the)
{
	if (get1or2params(sp, &name, &message, the) != PS_NORMAL)
	{
		MCperror->add(PE_HOSTNTOA_BADNAME, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCHostNtoA::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCAutoStringRef t_hostname;
    if (!ctxt . EvalExprAsStringRef(name, EE_HOSTNTOA_BADNAME, &t_hostname))
        return;
    
	MCNewAutoNameRef t_message;
    if (!ctxt . EvalOptionalExprAsNameRef(message, kMCEmptyName, EE_OPEN_BADMESSAGE, &t_message))
        return;

	MCNetworkEvalHostNameToAddress(ctxt, *t_hostname, *t_message, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

void MCInsertScripts::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	if (front)
		MCEngineEvalFrontScripts(ctxt, r_value .stringref_value);
	else
		MCEngineEvalBackScripts(ctxt, r_value . stringref_value);
    
    r_value . type = kMCExecValueTypeStringRef;
}


MCIntersect::~MCIntersect()
{
	delete o1;
	delete o2;

	// MW-2011-10-08: [[ Bug ]] Make sure we delete the threshold parameter to stop a
	//   memory leak.
	delete threshold;
}

Parse_stat MCIntersect::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_LPAREN) != PS_NORMAL)
	{
		MCperror->add(PE_FACTOR_NOLPAREN, sp);
		return PS_ERROR;
	}
	
	o1 = new (nothrow) MCChunk(False);
	o2 = new (nothrow) MCChunk(False);
	
	Symbol_type stype;
	if (o1->parse(sp, False) != PS_NORMAL
	        || sp.next(stype) != PS_NORMAL || stype != ST_SEP
	        || o2->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add(PE_INTERSECT_NOOBJECT, sp);
		return PS_ERROR;
	}
	
	// MW-2011-09-20: [[ Collision ]] Add an optional parameter for the type of intersection.
	if (sp . next(stype) == PS_NORMAL)
	{
		if (stype == ST_SEP)
		{
			if (sp.parseexp(False, False, &threshold) != PS_NORMAL)
			{
				MCperror->add(PE_INTERSECT_NOOBJECT, sp);
				return PS_ERROR;
			}
		}
		else
		{
			// MW-2011-09-23: [[ Bug ]] If we didn't find a sep, then backup.
			sp . backup();
		}
	}
	
	if (sp.skip_token(SP_FACTOR, TT_RPAREN) != PS_NORMAL)
	{
		MCperror->add(PE_FACTOR_NORPAREN, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCIntersect::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCObjectPtr t_object_a, t_object_b;

	if (!o1->getobj(ctxt, t_object_a, True)	|| !o2->getobj(ctxt, t_object_b, True))
	{
		ctxt . LegacyThrow(EE_INTERSECT_NOOBJECT);
		return;
	}
	
    MCAutoStringRef t_threshold;
    
	if (threshold != nil)
	{
        if (!ctxt . EvalExprAsStringRef(threshold, EE_INTERSECT_BADTHRESHOLD, &t_threshold))
            return;
		
		MCInterfaceEvalIntersectWithThreshold(ctxt, t_object_a, t_object_b, *t_threshold, r_value . bool_value);
        r_value . type = kMCExecValueTypeBool;
	}
	else
    {
		MCInterfaceEvalIntersect(ctxt, t_object_a, t_object_b, r_value . bool_value);
        r_value . type = kMCExecValueTypeBool;
    }
}

MCKeys::~MCKeys()
{
	delete source;
}

Parse_stat MCKeys::parse(MCScriptPoint &sp, Boolean the)
{
	Boolean parens = False;
	initpoint(sp);
	if (!the && sp.skip_token(SP_FACTOR, TT_LPAREN) == PS_NORMAL)
		parens = True;
	else
		if (sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL)
		{
			MCperror->add(PE_FACTOR_NOOF, sp);
			return PS_ERROR;
		}
	if (sp.skip_token(SP_FACTOR, TT_THE) == PS_NORMAL)
	{
		Symbol_type type;
		const LT *te;
		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add(PE_KEYS_BADPARAM, sp);
			return PS_ERROR;
		}
		if (sp.lookup(SP_FACTOR, te) != PS_NORMAL
		        || (te->which != P_DRAG_DATA
                    && te->which != P_CLIPBOARD_DATA
                    && te->which != P_RAW_CLIPBOARD_DATA
                    && te->which != P_RAW_DRAGBOARD_DATA
                    && te->which != P_FULL_CLIPBOARD_DATA
                    && te->which != P_FULL_DRAGBOARD_DATA))
		{
			MCperror->add(PE_KEYS_BADPARAM, sp);
			return PS_ERROR;
		}
		which = (Properties)te->which;
		if (parens)
			sp.skip_token(SP_FACTOR, TT_RPAREN);
		return PS_NORMAL;
	}
	if (sp.parseexp(True, False, &source) != PS_NORMAL)
	{
		MCperror->add(PE_KEYS_BADPARAM, sp);
		return PS_ERROR;
	}
	if (parens)
		sp.skip_token(SP_FACTOR, TT_RPAREN);
	return PS_NORMAL;
}

void MCKeys::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    
	//MCAutoStringRef t_result;

	if (source != NULL)
	{
        MCAutoArrayRef t_array;
        ctxt . TryToEvalExprAsArrayRef(source, EE_KEYS_BADSOURCE, &t_array);
		MCArraysEvalKeys(ctxt, *t_array, r_value . stringref_value);
        r_value . type = kMCExecValueTypeStringRef;
	}
	else
	{
		if (which == P_DRAG_DATA)
			MCPasteboardEvalDragDropKeys(ctxt, r_value . stringref_value);
        else if (which == P_RAW_CLIPBOARD_DATA)
            MCPasteboardEvalRawClipboardKeys(ctxt, r_value . stringref_value);
        else if (which == P_RAW_DRAGBOARD_DATA)
            MCPasteboardEvalRawDragKeys(ctxt, r_value . stringref_value);
        else if (which == P_FULL_CLIPBOARD_DATA)
            MCPasteboardEvalFullClipboardKeys(ctxt, r_value . stringref_value);
        else if (which == P_FULL_DRAGBOARD_DATA)
            MCPasteboardEvalFullDragKeys(ctxt, r_value . stringref_value);
		else if (which == P_CLIPBOARD_DATA)
			MCPasteboardEvalClipboardKeys(ctxt, r_value . stringref_value);
        else
            MCUnreachable();
        r_value . type = kMCExecValueTypeStringRef;
	}
}

MCLicensed::~MCLicensed()
{
	delete source;
}

Parse_stat MCLicensed::parse(MCScriptPoint &sp, Boolean the)
{
	if (!the)
	{
		if (get0or1param(sp, &source, the) != PS_NORMAL)
			return PS_ERROR;
	}
	else
		initpoint(sp);
	return PS_NORMAL;
}


Parse_stat MCLocals::parse(MCScriptPoint &sp, Boolean the)
{
	return MCFunction::parse(sp, the);
}

MCMatch::~MCMatch()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCMatch::parse(MCScriptPoint &sp, Boolean the)
{
	sp.allowescapes(True);
	if (getparams(sp, &params) != PS_NORMAL || params == NULL
	        || params->getnext() == NULL)
	{
		MCperror->add(PE_MATCH_BADPARAM, line, pos);
		return PS_ERROR;
	}
	sp.allowescapes(False);
	return PS_NORMAL;
}

bool MCStringsGetCachedPattern(MCStringRef p_pattern, regexp*& r_compiled);
bool MCStringsCachePattern(MCStringRef p_pattern, regexp* p_compiled);
bool MCStringsCompilePattern(MCStringRef p_pattern, regexp*& r_compiled);

void MCMatch::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    
    MCAutoValueRef t_source_valueref;
    MCAutoStringRef t_source;
    if (!params->eval(ctxt, &t_source_valueref))
    {
        ctxt . LegacyThrow(EE_MATCH_BADPARAM);
        return;
    }
    
    // SN-2015-07-27: [[ Bug 15379 ]] PCRE takes UTF-16 as input parameters, but
    //  if that input parameter is a DataRef, then we want to copy byte-to-unichar_t
    //  its contents. Otherwise, ConvertToString makes a native StringRef out of
    //  it, which will then be unnativised before being passed to MCR_exec; any
    //  byte in the range [0x80;0xFF] will be converted from the OS-specific
    //  extended ASCII table to the corresponding Unicode char.
    bool t_success;
    if (MCValueGetTypeCode(*t_source_valueref) == kMCValueTypeCodeData)
        t_success = MCStringCreateUnicodeStringFromData((MCDataRef)*t_source_valueref, false, &t_source);
    else
        t_success = ctxt . ConvertToString(*t_source_valueref, &t_source);
    
    if (!t_success)
    {
        ctxt . LegacyThrow(EE_MATCH_BADPARAM);
        return;
    }

    MCAutoValueRef t_pattern_valueref;
    MCAutoStringRef t_pattern;
	if (!params->getnext()->eval(ctxt, &t_pattern_valueref) || !ctxt . ConvertToString(*t_pattern_valueref, &t_pattern))
	{
		ctxt . LegacyThrow(EE_MATCH_BADPATTERN);
		return;
	}
    
    MCParameter *t_result_params = params->getnext()->getnext();
    uindex_t t_result_count = 0;
    for (MCParameter *p = t_result_params; p != nil; p = p->getnext())
        t_result_count++;
    
    MCAutoStringRefArray t_results;
    /* UNCHECKED */ t_results.New(t_result_count);
    
    if (chunk)
        MCStringsEvalMatchChunk(ctxt, *t_source, *t_pattern, *t_results, t_result_count, r_value . bool_value);
    else
        MCStringsEvalMatchText(ctxt, *t_source, *t_pattern, *t_results, t_result_count, r_value . bool_value);
    r_value .type = kMCExecValueTypeBool;
    
    if (!r_value . bool_value || ctxt . HasError())
    {
        return;
    }
    
    for (uindex_t i = 0; i < t_result_count; i++)
    {
        // AL-2014-09-09: [[ Bug 13359 ]] Make sure containers are used in case a param is a handler variable
		MCContainer t_container;
		if (!t_result_params->evalcontainer(ctxt, t_container))
		{
			ctxt . LegacyThrow(EE_MATCH_BADDEST);
			return;
		}
		
        /* UNCHECKED */ t_container.set_valueref(t_results[i]);
        
        t_result_params = t_result_params->getnext();
    }
}

Parse_stat MCMe::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	if (the)
	{
		MCperror->add(PE_ME_THE, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

	
MCMouse::~MCMouse()
{
	delete which;
}

Parse_stat MCMouse::parse(MCScriptPoint &sp, Boolean the)
{
	if (!the)
	{
		if (get0or1param(sp, &which, the) != PS_NORMAL)
		{
			MCperror->add
			(PE_MOUSE_BADPARAM, sp);
			return PS_ERROR;
		}
	}
	else
		initpoint(sp);
	return PS_NORMAL;
}

void MCMouse::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	uinteger_t b;
    if (!ctxt . EvalOptionalExprAsUInt(which, 0, EE_MOUSE_BADSOURCE, b))
        return;
	
	MCInterfaceEvalMouse(ctxt, b, r_value . nameref_value);
    r_value . type = kMCExecValueTypeNameRef;
}

Parse_stat MCParamCount::parse(MCScriptPoint &sp, Boolean the)
{
	return MCFunction::parse(sp, the);
}

Parse_stat MCParams::parse(MCScriptPoint &sp, Boolean the)
{
	return MCFunction::parse(sp, the);
}

MCReplaceText::~MCReplaceText()
{
	delete source;
	delete pattern;
	delete replacement;
}

Parse_stat MCReplaceText::parse(MCScriptPoint &sp, Boolean the)
{
	if (get2or3params(sp, &source, &pattern, &replacement) != PS_NORMAL
	        || replacement == NULL)
	{
		MCperror->add(PE_REPLACETEXT_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCReplaceText::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCAutoStringRef t_source;
    if (!ctxt . EvalExprAsStringRef(source, EE_REPLACETEXT_BADSOURCE, &t_source))
        return;

    MCAutoStringRef t_pattern;
    if (!ctxt . EvalExprAsStringRef(pattern, EE_REPLACETEXT_BADPATTERN, &t_pattern))
        return;
	
    MCAutoStringRef t_replacement;
    if (!ctxt . EvalExprAsStringRef(replacement, EE_REPLACETEXT_BADSOURCE, &t_replacement))
        return;
    
	MCStringsEvalReplaceText(ctxt, *t_source, *t_pattern, *t_replacement, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

// MW-2010-12-15: [[ Bug ]] Make sure the value of 'the result' is grabbed, otherwise
//   if it is modified by a function in an expression and used directly in that
//   expression, bogus things can happen. i.e.
//      the result = func_modifying_result()

void MCScreenRect::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCInterfaceEvalScreenRect(ctxt, false, f_plural, false, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

MCSelectedButton::~MCSelectedButton()
{
	delete family;
	delete object;
}

Parse_stat MCSelectedButton::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL)
	{
		MCperror->add(PE_FACTOR_NOOF, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_BACKGROUND) == PS_NORMAL)
		bg = True;
	else
		sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_CARD);
	if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_FAMILY) != PS_NORMAL)
	{
		MCperror->add(PE_SELECTEDBUTTON_NOFAMILY, sp);
		return PS_ERROR;
	}
	if (sp.parseexp(True, False, &family) != PS_NORMAL)
	{
		MCperror->add(PE_FACTOR_BADPARAM, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_OF) == PS_NORMAL)
	{
		object = new (nothrow) MCChunk(False);
		if (object->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_SELECTEDBUTTON_NOOBJECT, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

void MCSelectedButton::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	integer_t t_family;
    if (!ctxt . EvalExprAsInt(family, EE_SELECTEDBUTTON_BADFAMILY, t_family))
        return;

    if (object != NULL)
	{
		MCObjectPtr t_object;
		if (!object->getobj(ctxt, t_object, True))
		{
			ctxt . LegacyThrow(EE_SELECTEDBUTTON_BADPARENT);
			return;
		}	
		MCLegacyEvalSelectedButtonOf(ctxt, bg == True, t_family, t_object, r_value . stringref_value);
        r_value . type = kMCExecValueTypeStringRef;
	}
	else
	{
		MCLegacyEvalSelectedButton(ctxt, bg == True, t_family, r_value . stringref_value);
        r_value . type = kMCExecValueTypeStringRef;
	}
}

MCSelectedChunk::~MCSelectedChunk()
{
	delete object;
}

Parse_stat MCSelectedChunk::parse(MCScriptPoint &sp, Boolean the)
{
	return parsetarget(sp, the, False, object);
}

void MCSelectedChunk::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCAutoStringRef t_result;

	if (object != NULL)
	{
		MCObjectPtr optr;
		if (!object->getobj(ctxt, optr, True))
		{
			ctxt . LegacyThrow(EE_SELECTED_BADSOURCE);
			return;
		}
		MCInterfaceEvalSelectedChunkOf(ctxt, optr, r_value . stringref_value);
        r_value . type = kMCExecValueTypeStringRef;
	}
	else
	{
		MCInterfaceEvalSelectedChunk(ctxt, r_value . stringref_value);
        r_value . type = kMCExecValueTypeStringRef;
	}
}

MCSelectedLine::~MCSelectedLine()
{
	delete object;
}

Parse_stat MCSelectedLine::parse(MCScriptPoint &sp, Boolean the)
{
	return parsetarget(sp, the, False, object);
}

void MCSelectedLine::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	if (object != NULL)
	{
		MCObjectPtr optr;
		if (!object->getobj(ctxt, optr, True))
		{
			ctxt . LegacyThrow(EE_SELECTED_BADSOURCE);
			return;
		}
		MCInterfaceEvalSelectedLineOf(ctxt, optr, r_value . stringref_value);
        r_value . type = kMCExecValueTypeStringRef;
	}
	else
	{
		MCInterfaceEvalSelectedLine(ctxt, r_value . stringref_value);
        r_value . type = kMCExecValueTypeStringRef;
	}
}

MCSelectedLoc::~MCSelectedLoc()
{
	delete object;
}

Parse_stat MCSelectedLoc::parse(MCScriptPoint &sp, Boolean the)
{
	return parsetarget(sp, the, False, object);
}

void MCSelectedLoc::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    
    if (object != NULL)
	{
		MCObjectPtr optr;
		if (!object->getobj(ctxt, optr, True))
		{
			ctxt . LegacyThrow(EE_SELECTED_BADSOURCE);
			return;
		}
		MCInterfaceEvalSelectedLocOf(ctxt, optr, r_value . stringref_value);
        r_value .type = kMCExecValueTypeStringRef;
	}
	else
	{
		MCInterfaceEvalSelectedLoc(ctxt, r_value . stringref_value);
        r_value .type = kMCExecValueTypeStringRef;
	}
}

MCSelectedText::~MCSelectedText()
{
	delete object;
}

Parse_stat MCSelectedText::parse(MCScriptPoint &sp, Boolean the)
{
	return parsetarget(sp, the, False, object);
}

void MCSelectedText::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCAutoStringRef t_result;

	if (object != NULL)
	{
		MCObjectPtr t_target;
		if (!object->getobj(ctxt, t_target . object, t_target . part_id, True))
		{
			ctxt . LegacyThrow(EE_SELECTED_BADSOURCE);
			return;
		}
		MCInterfaceEvalSelectedTextOf(ctxt, t_target, r_value .stringref_value);
        r_value .type = kMCExecValueTypeStringRef;
	}
	else
	{
		MCInterfaceEvalSelectedText(ctxt, r_value .stringref_value);
        r_value .type = kMCExecValueTypeStringRef;
	}
}

Parse_stat MCTarget::parse(MCScriptPoint &sp, Boolean the)
{
	contents = False;
	if (!the)
	{
		if (sp.skip_token(SP_FACTOR, TT_LPAREN) == PS_NORMAL)
		{
			if (sp.skip_token(SP_FACTOR, TT_RPAREN) != PS_NORMAL)
			{
				MCperror->add(PE_FACTOR_NORPAREN, sp);
				return PS_ERROR;
			}
		}
		else
		{
			contents = True;
		}
	}
	initpoint(sp);
	return PS_NORMAL;
}

void MCTarget::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	if (contents)
		MCEngineEvalTargetContents(ctxt, r_value . stringref_value);
	else
		MCEngineEvalTarget(ctxt, r_value . stringref_value);
    
    r_value . type = kMCExecValueTypeStringRef;
}

// MW-2008-11-05: [[ Owner Reference ]] This is the 'owner' function syntax class.
//   It simply attempts to fetch the target object, and then evaluates its 'owner'
//   property.
MCOwner::~MCOwner(void)
{
	delete object;
}

Parse_stat MCOwner::parse(MCScriptPoint &sp, Boolean the)
{
	return parsetarget(sp, the, True, object);
}

void MCOwner::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCObjectPtr t_objptr;
	if (!object -> getobj(ctxt, t_objptr, True))
		return;

    MCEngineEvalOwner(ctxt, t_objptr, r_value .stringref_value);
    r_value .type = kMCExecValueTypeStringRef;
}

MCTextDecode::~MCTextDecode()
{
    delete m_data;
    delete m_encoding;
}

Parse_stat MCTextDecode::parse(MCScriptPoint& sp, Boolean the)
{
    if (get1or2params(sp, &m_data, &m_encoding, the) != PS_NORMAL)
    {
        MCperror->add(PE_TEXTDECODE_BADPARAM, sp);
        return PS_ERROR;
    }
    
    return PS_NORMAL;
}

void MCTextDecode::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    MCAutoDataRef t_data;
    m_data->eval(ctxt, &t_data);
    if (ctxt.HasError())
    {
        ctxt.LegacyThrow(EE_TEXTDECODE_BADDATA);
        return;
    }
    
    MCAutoStringRef t_encoding;
    if (m_encoding != NULL)
    {
        m_encoding->eval(ctxt, &t_encoding);
        if (ctxt.HasError())
        {
            ctxt.LegacyThrow(EE_TEXTDECODE_BADENCODING);
            return;
        }
    }
    else
    {
        t_encoding = MCSTR("native");
    }
    
    MCStringsEvalTextDecode(ctxt, *t_encoding, *t_data, r_value.stringref_value);
    r_value.type = kMCExecValueTypeStringRef;
}

MCTextEncode::~MCTextEncode()
{
    delete m_string;
    delete m_encoding;
}

Parse_stat MCTextEncode::parse(MCScriptPoint& sp, Boolean the)
{
    if (get1or2params(sp, &m_string, &m_encoding, the) != PS_NORMAL)
    {
        MCperror->add(PE_TEXTENCODE_BADPARAM, sp);
        return PS_ERROR;
    }
    
    return PS_NORMAL;
}

void MCTextEncode::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    MCAutoStringRef t_string;
    m_string->eval(ctxt, &t_string);
    if (ctxt.HasError())
    {
        ctxt.LegacyThrow(EE_TEXTENCODE_BADTEXT);
        return;
    }
    
    MCAutoStringRef t_encoding;
    if (m_encoding != NULL)
    {
        m_encoding->eval(ctxt, &t_encoding);
        if (ctxt.HasError())
        {
            ctxt.LegacyThrow(EE_TEXTENCODE_BADENCODING);
            return;
        }
    }
    else
    {
        t_encoding = MCSTR("native");
    }
    
    MCStringsEvalTextEncode(ctxt, *t_encoding, *t_string, r_value.dataref_value);
    r_value.type = kMCExecValueTypeDataRef;
}

MCNormalizeText::~MCNormalizeText()
{
    delete m_text;
    delete m_form;
}

Parse_stat MCNormalizeText::parse(MCScriptPoint& sp, Boolean the)
{
    if (get2params(sp, &m_text, &m_form) != PS_NORMAL)
    {
        MCperror->add(PE_NORMALIZETEXT_BADPARAM, sp);
        return PS_ERROR;
    }
    
    return PS_NORMAL;
}

void MCNormalizeText::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    MCAutoStringRef t_text;
    m_text->eval(ctxt, &t_text);
    if (ctxt.HasError())
    {
        ctxt.LegacyThrow(EE_NORMALIZETEXT_BADTEXT);
        return;
    }
    
    MCAutoStringRef t_form;
    m_form->eval(ctxt, &t_form);
    if (ctxt.HasError())
    {
        ctxt.LegacyThrow(EE_NORMALIZETEXT_BADFORM);
        return;
    }
    
    MCStringsEvalNormalizeText(ctxt, *t_text, *t_form, r_value.stringref_value);
    r_value.type = kMCExecValueTypeStringRef;
}

MCCodepointProperty::~MCCodepointProperty()
{
    delete m_codepoint;
    delete m_property;
}

Parse_stat MCCodepointProperty::parse(MCScriptPoint &sp, Boolean the)
{
    if (get2params(sp, &m_codepoint, &m_property) != PS_NORMAL)
    {
        MCperror->add(PE_CODEPOINTPROPERTY_BADPARAM, sp);
        return PS_ERROR;
    }
    
    return PS_NORMAL;
}

void MCCodepointProperty::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    MCAutoStringRef t_codepoint;
    m_codepoint->eval(ctxt, &t_codepoint);
    if (ctxt.HasError())
    {
        ctxt.LegacyThrow(EE_CODEPOINTPROPERTY_BADCODEPOINT);
        return;
    }
    
    MCAutoStringRef t_property;
    m_property->eval(ctxt, &t_property);
    if (ctxt.HasError())
    {
        ctxt.LegacyThrow(EE_CODEPOINTPROPERTY_BADPROPERTY);
        return;
    }
    
    MCStringsEvalCodepointProperty(ctxt, *t_codepoint, *t_property, r_value.valueref_value);
    r_value.type = kMCExecValueTypeValueRef;
}

MCTextHeightSum::~MCTextHeightSum()
{
	delete object;
}

Parse_stat MCTextHeightSum::parse(MCScriptPoint &sp, Boolean the)
{
	return parsetarget(sp, the, True, object);
}

void MCTextHeightSum::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCObjectPtr t_object;
	if (!object->getobj(ctxt, t_object, True))
	{
		ctxt . LegacyThrow(EE_TEXT_HEIGHT_SUM_NOOBJECT);
		return;
	}

	MCLegacyEvalTextHeightSum(ctxt, t_object, r_value . int_value);
    r_value . type = kMCExecValueTypeInt;
}

MCTopStack::~MCTopStack()
{
	delete which;
}

Parse_stat MCTopStack::parse(MCScriptPoint &sp, Boolean the)
{
	if (get0or1param(sp, &which, the) != PS_NORMAL)
	{
		MCperror->add(PE_TOPSTACK_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;

}

void MCTopStack::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	uinteger_t t_stack_number;
	MCAutoStringRef t_result;

	if (which != NULL)
	{
        if (!ctxt . EvalExprAsUInt(which, EE_TOPSTACK_BADSOURCE, t_stack_number))
            return;
		MCInterfaceEvalTopStackOf(ctxt, t_stack_number, r_value . stringref_value);
        r_value .type = kMCExecValueTypeStringRef;
	}
	else
    {
		MCInterfaceEvalTopStack(ctxt, r_value . stringref_value);
        r_value .type = kMCExecValueTypeStringRef;
    }
}

MCUniDecode::~MCUniDecode()
{
	delete source;
	delete language;
}

Parse_stat MCUniDecode::parse(MCScriptPoint &sp, Boolean the)
{
	if (get1or2params(sp, &source, &language, the) != PS_NORMAL)
	{
		MCperror->add(PE_UNIENCODE_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCUniDecode::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCNewAutoNameRef t_language;
    if (!ctxt . EvalOptionalExprAsNameRef(language, kMCEmptyName, EE_UNIDECODE_BADLANGUAGE, &t_language))
        return;
	
    MCAutoDataRef t_source;
    if (!ctxt .EvalExprAsDataRef(source, EE_UNIDECODE_BADSOURCE, &t_source))
        return;

	if (language)
	{
		// Explicit language, destination is a dataref
		MCAutoDataRef t_data;
		
		MCFiltersEvalUniDecodeToEncoding(ctxt, *t_source, *t_language, r_value . dataref_value);
        r_value . type = kMCExecValueTypeDataRef;
    }
	else
	{
		// No language, destination encoding is native
		MCAutoStringRef t_string;
		
		MCFiltersEvalUniDecodeToNative(ctxt, *t_source, r_value . stringref_value);
        r_value . type = kMCExecValueTypeStringRef;
	}
}

MCUniEncode::~MCUniEncode()
{
	delete source;
	delete language;
}

Parse_stat MCUniEncode::parse(MCScriptPoint &sp, Boolean the)
{
	if (get1or2params(sp, &source, &language, the) != PS_NORMAL)
	{
		MCperror->add(PE_UNIENCODE_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCUniEncode::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	
	MCNewAutoNameRef t_language;
    if (!ctxt . EvalOptionalExprAsNameRef(language, kMCEmptyName, EE_UNIENCODE_BADLANGUAGE, &t_language))
        return;


	
	MCAutoDataRef t_result;

	MCAutoValueRef t_source_valueref;
    if (!ctxt . EvalExprAsValueRef(source, EE_UNIENCODE_BADSOURCE, &t_source_valueref))
        return;
	
	if (language)
	{
		// Explicit language, source is a data ref
		MCAutoDataRef t_source;
        /* UNCHECKED */ ctxt . ConvertToData(*t_source_valueref, &t_source);
				
		MCFiltersEvalUniEncodeFromEncoding(ctxt, *t_source, *t_language, r_value . dataref_value);
        r_value . type = kMCExecValueTypeDataRef;
	}
	else
	{
		// No language, source encoding is native
		MCAutoStringRef t_source;
		/* UNCHECKED */ ctxt . ConvertToString(*t_source_valueref, &t_source);
		
		MCFiltersEvalUniEncodeFromNative(ctxt, *t_source, r_value . dataref_value);
        r_value . type = kMCExecValueTypeDataRef;
	}
}

MCValue::~MCValue()
{
	delete source;
	delete object;
}

Parse_stat MCValue::parse(MCScriptPoint &sp, Boolean the)
{
	if (the)
	{
		if (get1param(sp, &source, the) != PS_NORMAL)
		{
			MCperror->add(PE_VALUE_BADPARAM, sp);
			return PS_ERROR;
		}
	}
	else
	{
		initpoint(sp);
		if (sp.skip_token(SP_FACTOR, TT_LPAREN) != PS_NORMAL)
		{
			MCperror->add(PE_FACTOR_NOLPAREN, sp);
			return PS_ERROR;
		}
		if (sp.parseexp(False, False, &source) != PS_NORMAL)
		{
			MCperror->add(PE_VALUE_BADPARAM, sp);
			return PS_ERROR;
		}
		Symbol_type type;
		if (sp.next(type) != PS_NORMAL || (type != ST_RP && type != ST_SEP))
		{
			MCperror->add(PE_FACTOR_NORPAREN, sp);
			return PS_ERROR;
		}
		if (type == ST_SEP)
		{
			object = new (nothrow) MCChunk(False);
			if (object->parse(sp, False) != PS_NORMAL)
			{
				MCperror->add(PE_VALUE_BADOBJECT, sp);
				return PS_ERROR;
			}
			if (sp.next(type) != PS_NORMAL || (type != ST_RP && type != ST_SEP))
			{
				MCperror->add(PE_FACTOR_NORPAREN, sp);
				return PS_ERROR;
			}
		}
	}
	return PS_NORMAL;
}

void MCValue::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCAutoStringRef t_source;
	MCAutoValueRef t_result;
	
    if (!ctxt . EvalOptionalExprAsNullableStringRef(source, EE_VALUE_BADSOURCE, &t_source))
        return;

	if (*t_source != nil && object != nil)
	{
		MCObjectPtr t_object;

		if (!object->getobj(ctxt, t_object, True))
		{
			ctxt . LegacyThrow(EE_VALUE_NOOBJ);
			return;
		}

		MCEngineEvalValueWithObject(ctxt, *t_source, t_object, r_value . valueref_value);
        r_value .type = kMCExecValueTypeValueRef;
	}
	else
    {
		MCEngineEvalValue(ctxt, *t_source, r_value . valueref_value);
        r_value .type = kMCExecValueTypeValueRef;
    }
}

Parse_stat MCVariables::parse(MCScriptPoint &sp, Boolean the)
{
	return MCFunction::parse(sp, the);
}

MCWithin::~MCWithin()
{
	delete object;
	delete point;
}

Parse_stat MCWithin::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (sp.skip_token(SP_FACTOR, TT_LPAREN) != PS_NORMAL)
	{
		MCperror->add(PE_FACTOR_NOLPAREN, sp);
		return PS_ERROR;
	}
	object = new (nothrow) MCChunk(False);
	if (object->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add(PE_WITHIN_NOOBJECT, sp);
		return PS_ERROR;
	}
	Symbol_type type;
	if (sp.next(type) != PS_NORMAL || type != ST_SEP
	        || sp.parseexp(True, False, &point) != PS_NORMAL)
	{
		MCperror->add(PE_WITHIN_BADPOINT, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_RPAREN) != PS_NORMAL)
	{
		MCperror->add(PE_FACTOR_NORPAREN, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCWithin::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCObjectPtr t_object;
    bool t_result;

    if (!object->getobj(ctxt, t_object, True))
	{
        ctxt . LegacyThrow(EE_WITHIN_NOCONTROL);
        return;
	}

	MCPoint t_point;
    if (!ctxt . EvalExprAsPoint(point, EE_WITHIN_NAP, t_point))
        return;

    MCInterfaceEvalWithin(ctxt, t_object, t_point, t_result);

    if (!ctxt . HasError())
    {
        r_value . type = kMCExecValueTypeBool;
        r_value . bool_value = t_result;
    }
}

// platform specific functions
MCMCISendString::~MCMCISendString()
{
	delete string;
}

Parse_stat MCMCISendString::parse(MCScriptPoint &sp, Boolean the)
{
	if (!the)
	{
		if (get1param(sp, &string, the) != PS_NORMAL)
		{
			MCperror->add
			(PE_MCISENDSTRING_BADPARAM, sp);
			return PS_ERROR;
		}
	}
	else
		initpoint(sp);
	return PS_NORMAL;
}

void MCMCISendString::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    
    MCAutoStringRef t_string;
    MCAutoStringRef t_result;

    if (!MCExecValueTraits<MCStringRef>::eval(ctxt, string, EE_MCISENDSTRING_BADSOURCE, &t_string))
        return;
    
    MCMultimediaEvalMCISendString(ctxt, *t_string, &t_result);
    
    if (!ctxt . HasError())
        MCExecValueTraits<MCStringRef>::set(r_value, MCValueRetain(*t_result));
}

MCQueryRegistry::~MCQueryRegistry()
{
	delete key;
	delete type;
}

Parse_stat MCQueryRegistry::parse(MCScriptPoint &sp, Boolean the)
{
	if (get1or2params(sp, &key, &type, the) != PS_NORMAL)
	{
		MCperror->add(PE_QUERYREGISTRY_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCQueryRegistry::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCAutoStringRef t_key;
    if (!ctxt . EvalExprAsStringRef(key, EE_QUERYREGISTRY_BADEXP, &t_key))
        return;

    MCContainer t_container;
    MCAutoStringRef t_type;
	if (type != NULL)
	{
		if (!type -> evalcontainer(ctxt, t_container))
		{
			ctxt . LegacyThrow(EE_QUERYREGISTRY_BADDEST);
			return;
		}
		MCFilesEvalQueryRegistryWithType(ctxt, *t_key, &t_type, r_value . valueref_value);
        r_value . type = kMCExecValueTypeStringRef;
	}
	else
    {
		MCFilesEvalQueryRegistry(ctxt, *t_key, r_value . valueref_value);
        r_value . type = kMCExecValueTypeStringRef;
    }
    

	if (!ctxt.HasError())
	{
        if (type != NULL)
            /* UNCHECKED */ t_container.set_valueref(*t_type);
	}
}

MCSetRegistry::~MCSetRegistry()
{
	delete key;
	delete value;
	delete type;
}

Parse_stat MCSetRegistry::parse(MCScriptPoint &sp, Boolean the)
{
	if (get2or3params(sp, &key, &value, &type) != PS_NORMAL)
	{
		MCperror->add(PE_SETREGISTRY_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCSetRegistry::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCAutoStringRef t_key;
    if (!ctxt . EvalExprAsStringRef(key, EE_SETREGISTRY_BADEXP, &t_key))
        return;
	
    MCAutoStringRef t_value;
    if (!ctxt . EvalExprAsStringRef(value, EE_SETREGISTRY_BADEXP, &t_value))
        return;
	
	if (type != NULL)
	{
        MCAutoStringRef t_type;
        if (!ctxt . EvalExprAsStringRef(type, EE_SETREGISTRY_BADEXP, &t_type))
            return;

		MCFilesEvalSetRegistryWithType(ctxt, *t_key, *t_value, *t_type, r_value . bool_value);
        r_value . type = kMCExecValueTypeBool;
	}
	else
    {
		MCFilesEvalSetRegistry(ctxt, *t_key, *t_value, r_value . bool_value);
        r_value . type = kMCExecValueTypeBool;
    }
}

MCCopyResource::~MCCopyResource()
{
	delete source;
	delete dest;
	delete type;
	delete name;
	delete newid;
}

Parse_stat MCCopyResource::parse(MCScriptPoint &sp, Boolean the)
{
	if (get4or5params(sp, &source, &dest, &type, &name, &newid) != PS_NORMAL)
	{
		MCperror->add(PE_RESOURCES_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCCopyResource::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	
	MCAutoStringRef t_source,t_result;
    if (!ctxt . EvalExprAsStringRef(source, EE_RESOURCES_BADPARAM, &t_source))
        return;
    
    MCAutoStringRef t_dest;
	if (!ctxt . EvalExprAsStringRef(dest, EE_RESOURCES_BADPARAM, &t_dest))
        return;
    
    MCAutoStringRef t_type;
	if (!ctxt . EvalExprAsStringRef(type, EE_RESOURCES_BADPARAM, &t_type))
        return;
    
    MCAutoStringRef t_name;
	if (!ctxt . EvalExprAsStringRef(name, EE_RESOURCES_BADPARAM, &t_name))
        return;
    
    MCAutoStringRef t_newid;
	if (!ctxt . EvalOptionalExprAsNullableStringRef(newid, EE_RESOURCES_BADPARAM, &t_newid))
        return;
    
	if (newid != nil)
		MCFilesEvalCopyResourceWithNewId(ctxt, *t_source, *t_dest, *t_type, *t_name, *t_newid, r_value . stringref_value);
	else
		MCFilesEvalCopyResource(ctxt, *t_source, *t_dest, *t_type, *t_name, r_value . stringref_value);
    
    r_value . type = kMCExecValueTypeStringRef;
}

MCDeleteResource::~MCDeleteResource()
{
	delete source;
	delete type;
	delete name;
}

Parse_stat MCDeleteResource::parse(MCScriptPoint &sp, Boolean the)
{
	if (get3params(sp, &source, &type, &name) != PS_NORMAL)
	{
		MCperror->add(PE_RESOURCES_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCDeleteResource::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCAutoStringRef t_source;
    if (!ctxt . EvalExprAsStringRef(source, EE_RESOURCES_BADPARAM, &t_source))
        return;
	
    MCAutoStringRef t_type;
    if (!ctxt . EvalExprAsStringRef(type, EE_RESOURCES_BADPARAM, &t_type))
        return;
    
    MCAutoStringRef t_name;
    if (!ctxt . EvalExprAsStringRef(name, EE_RESOURCES_BADPARAM, &t_name))
        return;
	
	MCFilesEvalDeleteResource(ctxt, *t_source, *t_type, *t_name, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

MCGetResource::~MCGetResource()
{
	delete source;
	delete type;
	delete name;
}

Parse_stat MCGetResource::parse(MCScriptPoint &sp, Boolean the)
{
	if (get3params(sp, &source, &type, &name) != PS_NORMAL)
	{
		MCperror->add(PE_RESOURCES_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCGetResource::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	
    MCAutoStringRef t_source;
    if (!ctxt . EvalExprAsStringRef(source, EE_RESOURCES_BADPARAM, &t_source))
        return;
    
    MCAutoStringRef t_type;
    if (!ctxt . EvalExprAsStringRef(type, EE_RESOURCES_BADPARAM, &t_type))
        return;
    
    MCAutoStringRef t_name;
    if (!ctxt . EvalExprAsStringRef(name, EE_RESOURCES_BADPARAM, &t_name))
        return;
	
	MCFilesEvalGetResource(ctxt, *t_source, *t_type, *t_name, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

MCGetResources::~MCGetResources()
{
	delete source;
	delete type;
}

Parse_stat MCGetResources::parse(MCScriptPoint &sp, Boolean the)
{
	if (get1or2params(sp, &source, &type, the) != PS_NORMAL)
	{
		MCperror->add(PE_RESOURCES_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCGetResources::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCAutoStringRef t_source;
    if (!ctxt . EvalExprAsStringRef(source, EE_RESOURCES_BADPARAM, &t_source))
        return;
    
    MCAutoStringRef t_type;
    if (!ctxt . EvalOptionalExprAsNullableStringRef(type, EE_RESOURCES_BADPARAM, &t_type))
        return;
	
	if (type != nil)
		MCFilesEvalGetResourcesWithType(ctxt, *t_source, *t_type, r_value . stringref_value);
	else
		MCFilesEvalGetResources(ctxt, *t_source, r_value . stringref_value);
    
    r_value . type = kMCExecValueTypeStringRef;
}

MCSetResource::~MCSetResource()
{
	delete source;
	delete type;
	delete id;
	delete name;
	delete flags;
	delete value;
}

Parse_stat MCSetResource::parse(MCScriptPoint &sp, Boolean the)
{
	if (get6params(sp, &source, &type, &id, &name, &flags, &value) != PS_NORMAL)
	{
		MCperror->add(PE_RESOURCES_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCSetResource::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	
    MCAutoStringRef t_source;
    if (!ctxt . EvalExprAsStringRef(source, EE_RESOURCES_BADPARAM, &t_source))
        return;
    
    MCAutoStringRef t_type;
    if (!ctxt . EvalExprAsStringRef(type, EE_RESOURCES_BADPARAM, &t_type))
        return;
    
    MCAutoStringRef t_id;
    if (!ctxt . EvalExprAsStringRef(id, EE_RESOURCES_BADPARAM, &t_id))
        return;
    
    MCAutoStringRef t_name;
    if (!ctxt . EvalExprAsStringRef(name, EE_RESOURCES_BADPARAM, &t_name))
        return;
    
    MCAutoStringRef t_flags;
    if (!ctxt . EvalExprAsStringRef(flags, EE_RESOURCES_BADPARAM, &t_flags))
        return;
    
    MCAutoStringRef t_value;
    if (!ctxt . EvalExprAsStringRef(value, EE_RESOURCES_BADPARAM, &t_value))
        return;
    
    if (MCStringIsEmpty(*t_id) || MCStringIsEmpty(*t_name))
    {
        ctxt . LegacyThrow(EE_RESOURCES_BADPARAM);
        return;
    }
	
	MCFilesEvalSetResource(ctxt, *t_source, *t_type, *t_id, *t_name, *t_flags, *t_value, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

///////////////////////////////////////////////////////////////////////////////

MCHTTPProxyForURL::~MCHTTPProxyForURL(void)
{
	delete url;
	delete host;
	delete pac;
}

Parse_stat MCHTTPProxyForURL::parse(MCScriptPoint &sp, Boolean the)
{
	if (get2or3params(sp, &url, &host, &pac) != PS_NORMAL)
	{
		MCperror->add(PE_ALIASREFERENCE_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCHTTPProxyForURL::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	MCAutoStringRef t_url;
    if (!ctxt . EvalExprAsStringRef(url, EE_UNDEFINED, &t_url))
        return;

	MCAutoStringRef t_host;
    if (!ctxt . EvalExprAsStringRef(host, EE_UNDEFINED, &t_host))
        return;
    
	MCAutoStringRef t_pac;
    if (!ctxt . EvalOptionalExprAsNullableStringRef(pac, EE_UNDEFINED, &t_pac))
        return;
    
	if (pac == nil)
		MCNetworkEvalHTTPProxyForURL(ctxt, *t_url, *t_host, r_value . stringref_value);
	else
		MCNetworkEvalHTTPProxyForURLWithPAC(ctxt, *t_url, *t_host, *t_pac, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

///////////////////////////////////////////////////////////////////////////////

MCControlAtLoc::~MCControlAtLoc()
{
    delete location;
}

Parse_stat MCControlAtLoc::parse(MCScriptPoint &sp, Boolean the)
{
	if (get1param(sp, &location, the) != PS_NORMAL)
	{
		MCperror->add(PE_CONTROLATLOC_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCControlAtLoc::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCPoint t_location;
    if (!ctxt . EvalExprAsPoint(location, EE_CONTROLATLOC_NAP, t_location))
        return;

	MCAutoStringRef t_result;
	if (!is_screen)
		MCInterfaceEvalControlAtLoc(ctxt, t_location, &t_result);
	else
		MCInterfaceEvalControlAtScreenLoc(ctxt, t_location, &t_result);
    
    if (!ctxt.HasError())
    {
        r_value . type = kMCExecValueTypeStringRef;
        r_value . stringref_value = MCValueRetain(*t_result);
    }
}

///////////////////////////////////////////////////////////////////////////////

MCUuidFunc::~MCUuidFunc(void)
{
	delete type;
	delete name;
	delete namespace_id;
}

// Syntax:
//   uuid() - random uuid
//   uuid("random") - random uuid
//   uuid("md5" | "sha1", <namespace_id>, <name>)
// So either 0, 1, or 3 parameters.
Parse_stat MCUuidFunc::parse(MCScriptPoint& sp, Boolean the)
{
	// Parameters are parsed by 'getexps' into this array.
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	
	// Parse the parameters and check that there are 0, 1 or 3 of them.
	if (getexps(sp, earray, ecount) != PS_NORMAL || (ecount != 0 && ecount != 1 && ecount != 3))
	{
		// If there are the wrong number of params, free the exps.
		freeexps(earray, ecount);
		
		// Throw a parse error.
		MCperror -> add(PE_UUID_BADPARAM, sp);
		return PS_ERROR;
	}
	
	// Assign the expressions as appropriate.
	if (ecount > 0)
	{
		type = earray[0];
	
		if (ecount > 1)
		{
			namespace_id = earray[1];
			name = earray[2];
		}
	}
	
	// We are done, so return.
	return PS_NORMAL;
}

void MCUuidFunc::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    // First work out what type we want.
	MCUuidType t_type;
	if (type == nil)
		t_type = kMCUuidTypeRandom;
	else
	{
        MCAutoStringRef t_stringtype;

        if (!ctxt . EvalExprAsStringRef(type, EE_UUID_BADTYPE, &t_stringtype))
            return;

        if (MCStringIsEqualToCString(*t_stringtype, "random", kMCCompareCaseless))
		{
			// If there is more than one parameter, it's an error.
			if (name != nil)
			{
                ctxt . LegacyThrow(EE_UUID_TOOMANYPARAMS);
                return;
			}
			
			t_type = kMCUuidTypeRandom;
		}
        else if (MCStringIsEqualToCString(*t_stringtype, "md5", kMCCompareCaseless))
			t_type = kMCUuidTypeMD5;
        else if (MCStringIsEqualToCString(*t_stringtype, "sha1", kMCCompareCaseless))
			t_type = kMCUuidTypeSHA1;
		else
		{
			// If the type isn't one of 'random', 'md5', 'sha1' then it's
			// an error.
            ctxt . LegacyThrow(EE_UUID_UNKNOWNTYPE);
            return;
		}
	}
	
	// If it is not of random type, then evaluate the other params.
	MCAutoStringRef t_namespace_id;
	MCAutoStringRef t_name;
	if (t_type != kMCUuidTypeRandom)
	{
		// If there aren't namespace_id and name exprs, its an error.
		if (namespace_id == nil || name == nil)
		{
            ctxt . LegacyThrow(EE_UUID_TOOMANYPARAMS);
            return;
		}
        
        if (!ctxt . EvalExprAsStringRef(namespace_id, EE_UUID_BADNAMESPACEID, &t_namespace_id))
            return;
        
        if (!ctxt . EvalExprAsStringRef(name, EE_UUID_BADNAME, &t_name))
            return;
	}

	// Generate the uuid.
    MCAutoStringRef t_uuid;
	switch(t_type)
	{
        case kMCUuidTypeRandom:
            MCEngineEvalRandomUuid(ctxt, &t_uuid);
            break;
            
        case kMCUuidTypeMD5:
            MCEngineEvalMD5Uuid(ctxt, *t_namespace_id, *t_name, &t_uuid);
            break;
            
        case kMCUuidTypeSHA1:
            MCEngineEvalSHA1Uuid(ctxt, *t_namespace_id, *t_name, &t_uuid);
            break;
            
        default:
            assert(false);
            break;
	}
	
    if (!ctxt . HasError())
        MCExecValueTraits<MCStringRef>::set(r_value, MCValueRetain(*t_uuid));
}

///////////////////////////////////////////////////////////////////////////////

// MERG-2013-08-14: [[ MeasureText ]] Measure text relative to the effective font on an object
MCMeasureText::~MCMeasureText(void)
{
	delete m_object;
	delete m_text;
	delete m_mode;
}

// Syntax:
// measure[Unicode]Text(<text>,<object>,[<mode>])
Parse_stat MCMeasureText::parse(MCScriptPoint &sp, Boolean the)
{
    initpoint(sp);
    
	if (sp.skip_token(SP_FACTOR, TT_LPAREN) != PS_NORMAL)
	{
		MCperror->add
		(PE_FACTOR_NOLPAREN, sp);
		return PS_ERROR;
	}
    
    if (sp.parseexp(True, False, &m_text) != PS_NORMAL)
	{
		MCperror->add
		(PE_MEASURE_TEXT_BADTEXT, sp);
		return PS_ERROR;
	}
	
	Symbol_type type;
	m_object = new (nothrow) MCChunk(False);
	if (sp.next(type) != PS_NORMAL || type != ST_SEP
        || m_object->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_MEASURE_TEXT_NOOBJECT, sp);
		return PS_ERROR;
	}
    
    if (sp.next(type) != PS_NORMAL || (type != ST_RP && type != ST_SEP))
    {
        MCperror->add
        (PE_FACTOR_NORPAREN, sp);
        return PS_ERROR;
    }
    if (type == ST_SEP)
    {
        if (sp.parseexp(True, False, &m_mode) != PS_NORMAL)
        {
            MCperror->add
            (PE_MEASURE_TEXT_BADMODE, sp);
            return PS_ERROR;
        }
        
        if (sp.next(type) != PS_NORMAL || (type != ST_RP && type != ST_SEP))
        {
            MCperror->add
            (PE_FACTOR_NORPAREN, sp);
            return PS_ERROR;
        }
    }

	return PS_NORMAL;
}

void MCMeasureText::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    
    MCObject *t_object_ptr;
	uint4 parid;
    if (!m_object->getobj(ctxt, t_object_ptr, parid, True))
	{
        ctxt . LegacyThrow(EE_MEASURE_TEXT_NOOBJECT);
        return;
	}
    
    MCAutoStringRef t_text;
    if (!ctxt . EvalExprAsStringRef(m_text, EE_CHUNK_BADTEXT, &t_text))
        return;
    
    MCAutoStringRef t_result;
    MCAutoStringRef t_mode;
    if (!ctxt . EvalOptionalExprAsNullableStringRef(m_mode, EE_CHUNK_BADTEXT, &t_mode))
        return;

    MCTextEvalMeasureText(ctxt, t_object_ptr, *t_text, *t_mode, m_is_unicode, &t_result);

    if (!ctxt . HasError())
    {
        r_value . stringref_value = MCValueRetain(*t_result);
        r_value . type = kMCExecValueTypeStringRef;
    }
}

///////////////////////////////////////////////////////////////////////////////

Parse_stat
MCMessageDigestFunc::parse(MCScriptPoint &sp,
                           Boolean the)
{
    MCExpression *t_params[MAX_EXP];
    uint2 t_param_count = 0;

    if (getexps(sp, t_params, t_param_count) != PS_NORMAL ||
        (t_param_count != 2))
    {
        /* Wrong number of parameters or some other probleem */
        freeexps(t_params, t_param_count);

        MCperror->add(PE_MESSAGEDIGEST_BADPARAM, sp);
        return PS_ERROR;
    }

    m_data.Reset(t_params[0]);
    m_type.Reset(t_params[1]);
    return PS_NORMAL;
}

void
MCMessageDigestFunc::eval_ctxt(MCExecContext &ctxt,
                               MCExecValue &r_value)
{
    MCNewAutoNameRef t_name;
    if (!ctxt.EvalExprAsNameRef(m_type.Get(), EE_MESSAGEDIGEST_BADTYPE, &t_name))
        return;
    MCAutoDataRef t_data;
    if (!ctxt.EvalExprAsDataRef(m_data.Get(), EE_MESSAGEDIGEST_BADDATA, &t_data))
        return;
    MCAutoDataRef t_digest;
    MCFiltersEvalMessageDigest(ctxt, *t_data, *t_name, &t_digest);
    if (!ctxt.HasError())
    {
        r_value.dataref_value = t_digest.Take();
        r_value.type = kMCExecValueTypeDataRef;
    }
}

///////////////////////////////////////////////////////////////////////////////

#ifdef _TEST
#include "test.h"

static void TestIsOperator(void)
{
    MCTestAssertTrue("something is correct", true);
}

TEST_DEFINE(IsOperator, TestIsOperator)

#endif
