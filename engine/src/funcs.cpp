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

//#include "execpt.h"
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

#include "syntax.h"
#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

void MCFunction::compile_with_args(MCSyntaxFactoryRef ctxt, MCExecMethodInfo *p_method, ...)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	// The rest of the argument list will be the MCExpression's that should be
	// compiled to form the argument list. We loop through them and compile
	// in turn.
	va_list t_args;
	va_start(t_args, p_method);
	for(uindex_t i = 0; i < p_method -> arity - 1; i++)
	{
		MCExpression *t_arg;
		t_arg = va_arg(t_args, MCExpression *);
		t_arg -> compile(ctxt);
	}
	va_end(t_args);
	
	MCSyntaxFactoryEvalMethod(ctxt, p_method);
	
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCConstantFunction::compile(MCSyntaxFactoryRef ctxt)
{
	// A constant function takes no arguments.
	compile_with_args(ctxt, getmethodinfo());
}

void MCUnaryFunction::compile(MCSyntaxFactoryRef ctxt)
{
	// An unary function takes a single argument.
	compile_with_args(ctxt, getmethodinfo(), getmethodarg());
}

void MCParamFunction::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	uindex_t t_count;
	t_count = 0;

	for (MCParameter *t_param = getmethodarg(); t_param != nil; t_param = t_param -> getnext())
	{
		t_param -> compile(ctxt);
		t_count++;
	}
	
	MCSyntaxFactoryEvalList(ctxt, t_count);

	MCSyntaxFactoryEvalMethod(ctxt, getmethodinfo());

	MCSyntaxFactoryEndExpression(ctxt);
}

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
		object = new MCChunk(False);
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
			object = new MCChunk(False);
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

#ifdef /* MCBase64Decode */ LEGACY_EXEC
    if (source->eval(ep) != ES_NORMAL)
    {
        MCeerror->add(EE_BASE64DECODE_BADSOURCE, line, pos);
        return ES_ERROR;
    }
    MCU_base64decode(ep);
    return ES_NORMAL;
#endif /* MCBase64Decode */

#ifdef /* MCBase64Encode */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_BASE64ENCODE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCU_base64encode(ep);
	return ES_NORMAL;
#endif /* MCBase64Encode */

void MCArrayEncode::compile(MCSyntaxFactoryRef ctxt)
{
    MCSyntaxFactoryBeginExpression(ctxt, line, pos);
    
    source -> compile(ctxt);
    
    if (version)
        version -> compile(ctxt);
    else
        MCSyntaxFactoryEvalConstantNil(ctxt);
        
	MCSyntaxFactoryEvalMethod(ctxt, kMCArraysEvalArrayEncodeMethodInfo);
    
	MCSyntaxFactoryEndExpression(ctxt);
        
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
#ifdef /* MCBaseConvert */ LEGACY_EXEC
	if (destbase->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL
	        || ep.getnvalue() < 2.0 || ep.getnvalue() > 36.0)
	{
		MCeerror->add
		(EE_BASECONVERT_BADDESTBASE, line, pos);
		return ES_ERROR;
	}
	uint2 dbase = ep.getuint2();
	if (sourcebase->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL
	        || ep.getnvalue() < 2.0 || ep.getnvalue() > 36.0)
	{
		MCeerror->add
		(EE_BASECONVERT_BADSOURCEBASE, line, pos);
		return ES_ERROR;
	}
	uint2 sbase = ep.getuint2();
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_BASECONVERT_BADSOURCE, line, pos);
		return ES_ERROR;
	}


	uint4 value = 0;
	Boolean negative = False;

	// MW-2008-01-31: [[ Bug 5841 ]] Added in some more strict error checking to
	//   stop baseConvert attempting to convert strings with digits outside the
	//   source-base.
	bool t_error;
	t_error = false;

	const char *t_string;
	uint4 t_length;
	t_string = ep . getsvalue() . getstring();
	t_length = ep . getsvalue() . getlength();

	if (t_length == 0)
		t_error = true;
	
	uint4 i;
	if (!t_error)
	{
		if (t_string[0] == '+')
			i = 1;
		else if (t_string[0] == '-')
			i = 1, negative = True;
		else
			i = 0;
	}
	
	while(!t_error && i < t_length)
	{
		value *= sbase;
		char source = MCS_toupper(t_string[i]);
		if (isdigit((uint1)source))
		{
			if (source - '0' >= sbase)
				t_error = true;
			else
				value += source - '0';
		}
		else if (source >= 'A' && source < 'A' + sbase - 10)
			value += source - 'A' + 10;
		else
			t_error = true;
	
		i += 1;
	}

	if (t_error)
	{
		MCeerror->add(EE_BASECONVERT_CANTCONVERT, line, pos, ep.getsvalue());
		return ES_ERROR;
	}

	char result[64];
	char *dptr = &result[63];
	do
	{
		uint2 digit = value % dbase;
		value /= dbase;
		if (digit >= 10)
			*dptr-- = digit - 10 + 'A';
		else
			*dptr-- = digit + '0';
	}
	while (value);
	if (negative)
		*dptr-- = '-';
	dptr++;
	ep.copysvalue(dptr, 64 - (dptr - result));
	return ES_NORMAL;
#endif /* MCBaseConvert */

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

void MCBaseConvert::compile(MCSyntaxFactoryRef ctxt)
{
	compile_with_args(ctxt,kMCMathEvalBaseConvertMethodInfo, source, sourcebase, destbase);
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
#ifdef /* MCBinaryDecode */ LEGACY_EXEC
	if (params->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_BINARYD_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	char *format = ep.getsvalue().clone();
	const char *fptr = format;
	ep.clear();
	MCParameter *pptr = params->getnext();
	if (pptr == NULL || pptr->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_BINARYD_BADPARAM, line, pos);
		delete format;
		return ES_ERROR;
	}
	const char *buffer = ep.getsvalue().getstring();
	int4 length = ep.getsvalue().getlength();
	int4 done = 0;
	int4 offset = 0;
	pptr = pptr->getnext();
	MCExecPoint ep2(ep);
	
	// MW-2009-01-14: Update parameters to use containers properly
	MCVariable *t_var;
	MCVariableValue *t_var_value;
	
	while (*fptr && offset < length )
	{
		char cmd;
		int4 count;
		int4 i;
		MCU_gettemplate(fptr, cmd, count);
		if (count == 0 && cmd != '@')
			continue;
		
		if (cmd != 'x')
			if (pptr == NULL || pptr -> evalcontainer(ep, t_var, t_var_value) == ES_ERROR)
			{
				MCeerror->add(EE_BINARYD_BADDEST, line, pos);
				delete format;
				return ES_ERROR;
			}
		
		switch (cmd)
		{
			case 'a':
			case 'A':
			{
				if (count == BINARY_ALL)
					count = length - offset;
				else
					if (count == BINARY_NOCOUNT)
						count = 1;
				if (count > length - offset)
					break;
				const char *src = buffer + offset;
				uint4 size = count;
				if (cmd == 'A')
				{
					while (size > 0)
					{
						if (src[size - 1] != '\0' && src[size - 1] != ' ')
							break;
						size--;
					}
				}
				MCString s(&buffer[offset], size);
				
				t_var_value -> assign_string(s);
				
				done++;
				offset += count;
				break;
			}
			case 'b':
			case 'B':
			{
				if (count == BINARY_ALL)
					count = (length - offset) * 8;
				else
					if (count == BINARY_NOCOUNT)
						count = 1;
				if (count > (length - offset) * 8)
					break;
				uint1 *src = (uint1 *)buffer + offset;
				uint1 value = 0;
				char *dest = ep2.getbuffer(count);
				if (cmd == 'b')
					for (i = 0 ; i < count ; i++)
					{
						if (i % 8)
							value >>= 1;
						else
							value = *src++;
						*dest++ = (value & 1) ? '1' : '0';
					}
				else
					for (i = 0 ; i < count ; i++)
					{
						if (i % 8)
							value <<= 1;
						else
							value = *src++;
						*dest++ = (value & 0x80) ? '1' : '0';
					}
				ep2.setlength(count);
				
				t_var_value -> assign_string(ep2.getsvalue());
				
				done++;
				offset += (count + 7 ) / 8;
				break;
			}
			case 'h':
			case 'H':
			{
				if (count == BINARY_ALL)
					count = (length - offset) * 2;
				else
					if (count == BINARY_NOCOUNT)
						count = 1;
				if (count > (length - offset) * 2)
					break;
				uint1 *src = (uint1 *)buffer + offset;
				uint1 value = 0;
				char *dest = ep2.getbuffer(count);
				static char hexdigit[] = "0123456789abcdef";
				if (cmd == 'h')
					for (i = 0 ; i < count ; i++)
					{
						if (i % 2)
							value >>= 4;
						else
							value = *src++;
						*dest++ = hexdigit[value & 0xf];
					}
				else
					for (i = 0 ; i < count ; i++)
					{
						if (i % 2)
							value <<= 4;
						else
							value = *src++;
						*dest++ = hexdigit[(value >> 4) & 0xf];
					}
				ep2.setlength(count);
				t_var_value -> assign_string(ep2.getsvalue());
				
				done++;
				offset += (count + 1) / 2;
				break;
			}
			case 'c':
			case 'C':
			case 's':
			case 'S':
			case 'i':
			case 'I':
			case 'm':
			case 'M':
			case 'n':
			case 'N':
			case 'f':
			case 'd':
				if (count == BINARY_ALL || count == BINARY_NOCOUNT)
					count = 1;
				while (count--)
				{
					int4 oldoffset = offset;
					switch (cmd)
					{
						case 'c':
							if (offset + (int4)sizeof(int1) <= length)
							{
								int1 c;
								memcpy(&c, buffer + offset, sizeof(int1));
								t_var_value -> assign_real((double)c);
								offset += sizeof(int1);
							}
							break;
						case 'C':
							if (offset + (int4)sizeof(uint1) <= length)
							{
								uint1 c;
								memcpy(&c, buffer + offset, sizeof(uint1));
								t_var_value -> assign_real((double)c);
								offset += sizeof(uint1);
							}
							break;
						case 's':
							if (offset + (int4)sizeof(int2) <= length)
							{
								int2 c;
								memcpy(&c, buffer + offset, sizeof(int2));
								t_var_value -> assign_real((double)c);
								offset += sizeof(int2);
							}
							break;
						case 'S':
							if (offset + (int4)sizeof(uint2) <= length)
							{
								uint2 c;
								memcpy(&c, buffer + offset, sizeof(uint2));
								t_var_value -> assign_real((double)c);
								offset += sizeof(uint2);
							}
							break;
						case 'i':
							if (offset + (int4)sizeof(int4) <= length)
							{
								int4 c;
								memcpy(&c, buffer + offset, sizeof(int4));
								t_var_value -> assign_real((double)c);
								offset += sizeof(int4);
							}
							break;
						case 'I':
							if (offset + (int4)sizeof(uint4) <= length)
							{
								uint4 c;
								memcpy(&c, buffer + offset, sizeof(uint4));
								t_var_value -> assign_real((double)c);
								offset += sizeof(uint4);
							}
							break;
						case 'm':
							if (offset + (int4)sizeof(uint2) <= length)
							{
								uint2 c;
								memcpy(&c, buffer + offset, sizeof(uint2));
								t_var_value -> assign_real((double)(uint2)MCSwapInt16HostToNetwork(c));
								offset += sizeof(int2);
							}
							break;
						case 'M':
							if (offset + (int4)sizeof(uint4) <= length)
							{
								uint4 c;
								memcpy(&c, buffer + offset, sizeof(uint4));
								t_var_value -> assign_real((double)(uint4)MCSwapInt32HostToNetwork(c));
								offset += sizeof(uint4);
							}
							break;
							
							// MW-2007-09-11: [[ Bug 5315 ]] Make sure we coerce to signed integers
							//   before we convert to doubles - failing to do this results in getting
							//   unsigned results on little endian machines.
						case 'n':
							if (offset + (int4)sizeof(int2) <= length)
							{
								int2 c;
								memcpy(&c, buffer + offset, sizeof(int2));
								t_var_value -> assign_real((double)(int2)MCSwapInt16HostToNetwork(c));
								offset += sizeof(int2);
							}
							break;
						case 'N':
							if (offset + (int4)sizeof(int4) <= length)
							{
								int4 c;
								memcpy(&c, buffer + offset, sizeof(int4));
								t_var_value -> assign_real((double)(int4)MCSwapInt32HostToNetwork(c));
								offset += sizeof(int4);
							}
							break;
							
						case 'f':
							if (offset + (int4)sizeof(float) <= length)
							{
								float f;
								memcpy(&f, buffer + offset, sizeof(float));
								t_var_value -> assign_real((double)f);
								offset += sizeof(float);
							}
							break;
						case 'd':
							if (offset + (int4)sizeof(double) <= length)
							{
								double d;
								memcpy(&d, buffer + offset, sizeof(double));
								t_var_value -> assign_real((double)d);
								offset += sizeof(double);
								break;
							}
							break;
					}
					if (offset == oldoffset)
					{
						offset = length;
						t_var_value -> clear();
						break;
					}
					done++;
					if (count)
					{
						pptr = pptr->getnext();
						if (pptr == NULL || pptr -> evalcontainer(ep, t_var, t_var_value) == ES_ERROR)
						{
							MCeerror->add(EE_BINARYD_BADPARAM, line, pos);
							delete format;
							return ES_ERROR;
						}
					}
				}
				break;
			case 'x':
				if (count == BINARY_NOCOUNT)
					count = 1;
				if (count == BINARY_ALL || (count > (length - offset)))
					offset = length;
				else
					offset += count;
				break;
			default:
				MCeerror->add(EE_BINARYD_BADFORMAT, line, pos);
				delete format;
				return ES_ERROR;
		}
		pptr = pptr->getnext();
	}
	delete format;
	ep.setnvalue(done);
	while (pptr != NULL)
	{
		if (pptr -> evalcontainer(ep, t_var, t_var_value) == ES_ERROR)
		{
			MCeerror->add(EE_BINARYD_BADPARAM, line, pos);
			return ES_ERROR;
		}
		t_var_value -> clear();
		
		pptr = pptr->getnext();
	}
	return ES_NORMAL;
#endif /* MCBinaryDecode */

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
        for (uindex_t i = 0; i < t_result_count && i < r_value . int_value; i++)
        {
            if (t_results[i] != nil)
            {
                // AL-2014-09-09: [[ Bug 13359 ]] Make sure containers are used in case a param is a handler variable
                // AL-2014-09-18: [[ Bug 13465 ]] Use auto class to prevent memory leak
                MCAutoPointer<MCContainer> t_container;
                if (!t_params->evalcontainer(ctxt, &t_container))
                {
                    ctxt . LegacyThrow(EE_BINARYD_BADDEST);
                    return;
                }
                
                /* UNCHECKED */ t_container->set_valueref(t_results[i]);
            }
            else
            {
                t_skipped++;
            }
            t_params = t_params->getnext();
        }
        
        // Account for the skipped ("x") parameters
        if (t_skipped >= r_value . int_value)
            r_value . int_value = 0;
        else
            r_value . int_value -= t_skipped;
    }
}

void MCBinaryDecode::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	uindex_t t_count;
	t_count = 0;

	MCParameter *t_parameter = params;
	t_parameter -> compile(ctxt);
	t_parameter = t_parameter -> getnext();

	if (t_parameter != nil)
	{
		t_parameter -> compile(ctxt);
		t_parameter = t_parameter -> getnext();
	}
	else
		MCSyntaxFactoryEvalConstantNil(ctxt);
		
	for (MCParameter *p = t_parameter; p != nil; p = p -> getnext())
	{
		p -> compile_out(ctxt);
		t_count++;
	}
	
	MCSyntaxFactoryEvalList(ctxt, t_count);

	MCSyntaxFactoryEvalMethod(ctxt, kMCFiltersEvalBinaryDecodeMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
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
#ifdef /* MCBinaryEncode */ LEGACY_EXEC
	if (params->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_BINARYE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	char *format = ep.getsvalue().clone();
	const char *fptr = format;
	ep.clear();
	MCParameter *pptr = params->getnext();
	MCExecPoint ep2(ep);
	while (*fptr)
	{
		char cmd;
		int4 count;
		MCU_gettemplate(fptr, cmd, count);
		if (count == 0 && cmd != '@')
		{
			pptr = pptr->getnext();
			continue;
		}
		if (pptr == NULL || pptr->eval(ep2) != ES_NORMAL)
		{
			MCeerror->add(EE_BINARYE_BADPARAM, line, pos);
			delete format;
			return ES_ERROR;
		}
		pptr = pptr->getnext();
		const char *bytes = ep2.getsvalue().getstring();
		int4 length = ep2.getsvalue().getlength();
		switch (cmd)
		{
		case 'a':
		case 'A':
			{
				char pad = cmd == 'a' ? '\0' : ' ';
				if (count == BINARY_ALL)
					count = length;
				else
					if (count == BINARY_NOCOUNT)
						count = 1;
				if (length >= count)
					ep.appendbytes(bytes, count);
				else
				{
					ep.appendbytes(bytes, length);
					ep.pad(pad, count - length);
				}
				break;
			}
		case 'b':
		case 'B':
			{
				if (count == BINARY_ALL)
					count = length;
				else
					if (count == BINARY_NOCOUNT)
						count = 1;
				uint1 *cursor = ep.pad('\0', (count + 7) / 8);
				uint1 value = 0;
				if (count > length)
					count = length;
				int4 offset;
				if (cmd == 'B')
					for (offset = 0 ; offset < count ; offset++)
					{
						value <<= 1;
						if (bytes[offset] == '1')
							value |= 1;
						else
							if (bytes[offset] != '0')
							{
								MCeerror->add(EE_BINARYE_BADFORMAT, line, pos, ep2.getsvalue());
								delete format;
								return ES_ERROR;
							}
						if ((offset + 1) % 8 == 0)
						{
							*cursor++ = value;
							value = 0;
						}
					}
				else
					for (offset = 0 ; offset < count ; offset++)
					{
						value >>= 1;
						if (bytes[offset] == '1')
							value |= 128;
						else
							if (bytes[offset] != '0')
							{
								MCeerror->add(EE_BINARYE_BADFORMAT, line, pos, ep2.getsvalue());
								delete format;
								return ES_ERROR;
							}
						if (!((offset + 1) % 8))
						{
							*cursor++ = value;
							value = 0;
						}
					}
				if ((offset % 8) != 0)
				{
					if (cmd == 'B')
						value <<= 8 - (offset % 8);
					else
						value >>= 8 - (offset % 8);
					*cursor++ = value;
				}
				break;
			}
		case 'h':
		case 'H':
			{
				int c;
				if (count == BINARY_ALL)
					count = length;
				else
					if (count == BINARY_NOCOUNT)
						count = 1;
				uint1 *cursor = ep.pad('\0', (count + 1) / 2);
				uint1 value = 0;
				if (count > length)
					count = length;
				int4 offset;
				if (cmd == 'H')
					for (offset = 0 ; offset < count ; offset++)
					{
						value <<= 4;
						if (!isxdigit(bytes[offset]))
						{
							MCeerror->add(EE_BINARYE_BADFORMAT, line, pos, ep2.getsvalue());
							delete format;
							return ES_ERROR;
						}
						c = bytes[offset] - '0';
						if (c > 9)
							c += ('0' - 'A') + 10;
						if (c > 16)
							c += ('A' - 'a');
						value |= (c & 0xf);
						if (offset % 2)
						{
							*cursor++ = value;
							value = 0;
						}
					}
				else
					for (offset = 0 ; offset < count ; offset++)
					{
						value >>= 4;
						if (!isxdigit(bytes[offset]))
						{
							MCeerror->add(EE_BINARYE_BADFORMAT, line, pos, ep2.getsvalue());
							delete format;
							return ES_ERROR;
						}
						c = bytes[offset] - '0';
						if (c > 9)
							c += ('0' - 'A') + 10;
						if (c > 16)
							c += ('A' - 'a');
						value |= ((c << 4) & 0xf0);
						if (offset % 2)
						{
							*cursor++ = value;
							value = 0;
						}
					}
				if (offset % 2)
				{
					if (cmd == 'H')
						value <<= 4;
					else
						value >>= 4;
					*cursor++ = (unsigned char) value;
				}
				break;
			}
		case 'c':
		case 'C':
		case 's':
		case 'S':
		case 'i':
		case 'I':
		case 'm':
		case 'M':
		case 'n':
		case 'N':
		case 'f':
		case 'd':
			if (count == BINARY_ALL || count == BINARY_NOCOUNT)
				count = 1;
			while (count--)
			{
				if (ep2.ton() != ES_NORMAL)
				{
					MCeerror->add(EE_BINARYE_BADFORMAT, line, pos, ep2.getsvalue());
					delete format;
					return ES_ERROR;
				}
				switch (cmd)
				{
				case 'c':
					{
						int1 *cursor = (int1 *)ep.pad('\0', sizeof(int1));
						int1 c = (int1)ep2.getint4();
						memcpy(cursor, &c, sizeof(int1));
						break;
					}
				case 'C':
					{
						uint1 *cursor = ep.pad('\0', sizeof(uint1));
						uint1 c = (uint1)ep2.getuint4();
						memcpy(cursor, &c, sizeof(uint1));
						break;
					}
				case 's':
					{
						int2 *cursor = (int2 *)ep.pad('\0', sizeof(int2));
						int2 c = (int2)ep2.getint4();
						memcpy(cursor, &c, sizeof(int2));
						break;
					}
				case 'S':
					{
						uint2 *cursor = (uint2 *)ep.pad('\0', sizeof(uint2));
						uint2 c = (uint2)ep2.getuint4();
						memcpy(cursor, &c, sizeof(uint2));
						break;
					}
				case 'i':
					{
						int4 *cursor = (int4 *)ep.pad('\0', sizeof(int4));
						int4 c = (int4)ep2.getint4();
						memcpy(cursor, &c, sizeof(int4));
						break;
					}
				case 'I':
					{
						uint4 *cursor = (uint4 *)ep.pad('\0', sizeof(uint4));
						uint4 c = (uint4)ep2.getuint4();
						memcpy(cursor, &c, sizeof(uint4));
						break;
					}
				case 'm':
					{
						uint2 *cursor = (uint2 *)ep.pad('\0', sizeof(uint2));
						int2 c = MCSwapInt16HostToNetwork((uint2)ep2.getuint4());
						memcpy(cursor, &c, sizeof(uint2));
						break;
					}
				case 'M':
					{
						uint4 *cursor = (uint4 *)ep.pad('\0', sizeof(uint4));
						uint4 c = MCSwapInt32HostToNetwork(ep2.getuint4());
						memcpy(cursor, &c, sizeof(uint4));
						break;
					}
				case 'n':
					{
						int2 *cursor = (int2 *)ep.pad('\0', sizeof(int2));
						int2 c = MCSwapInt16HostToNetwork((int2)ep2.getint4());
						memcpy(cursor, &c, sizeof(int2));
						break;
					}
				case 'N':
					{
						int4 *cursor = (int4 *)ep.pad('\0', sizeof(int4));
						int4 c = MCSwapInt32HostToNetwork(ep2.getint4());
						memcpy(cursor, &c, sizeof(int4));
						break;
					}
				case 'f':
					{
						uint1 *cursor = ep.pad('\0', sizeof(float));
						float f = (float)ep2.getnvalue();
						memcpy(cursor, &f, sizeof(float));
						break;
					}
				case 'd':
					{
						uint1 *cursor = ep.pad('\0', sizeof(double));
						double d = ep2.getnvalue();
						memcpy(cursor, &d, sizeof(double));
						break;
					}
				}
				if (count)
				{
					if (pptr == NULL || pptr->eval(ep2) != ES_NORMAL)
					{
						MCeerror->add(EE_BINARYE_BADPARAM, line, pos);
						delete format;
						return ES_ERROR;
					}
					pptr = pptr->getnext();
				}
			}
			break;
		case 'x':
			if (count == BINARY_ALL)
				count = length;
			else
				if (count == BINARY_NOCOUNT)
					count = 1;
			ep.pad('\0', count);
			break;
		default:
			MCeerror->add(EE_BINARYE_BADFORMAT, line, pos);
			delete format;
			return ES_ERROR;
		}
	}
	delete format;
	return ES_NORMAL;
#endif /* MCBinaryEncode */

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

void MCBinaryEncode::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	uindex_t t_count;
	t_count = 0;

	for (MCParameter *t_parameter = params; t_parameter != nil; t_parameter = t_parameter -> getnext())
	{
		t_parameter -> compile(ctxt);
		t_count++;
	}
	
	MCSyntaxFactoryEvalList(ctxt, t_count - 1);

	MCSyntaxFactoryEvalMethod(ctxt, kMCFiltersEvalBinaryEncodeMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
}

#ifdef /* MCBuildNumber */ LEGACY_EXEC
	ep.setint(MCbuildnumber);
	return ES_NORMAL;
#endif /* MCBuildNumber */


#ifdef /* MCCachedUrls */ LEGACY_EXEC
	ep.getobj()->message(MCM_get_cached_urls, (MCParameter*)NULL, False, True);
	MCresult->fetch(ep);
	return ES_NORMAL;
#endif /* MCCachedUrls */


#ifdef /* MCCapsLockKey */ LEGACY_EXEC
	ep.setstaticcstring(MCU_ktos((MCscreen->querymods() & MS_CAPS_LOCK) != 0));
	return ES_NORMAL;
#endif /* MCCapsLockKey */


#ifdef /* MCCharToNum */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_CHARTONUM_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	if (ep.getsvalue().getlength())
	{
		if (ep.getuseunicode())
		{
			if (ep.getsvalue().getlength() >= 2)
			{
				const uint2 *sptr = (const uint2 *)ep.getsvalue().getstring();
				ep.setnvalue(*sptr);
			}


			else
				ep.clear();
		}
		else
		{
			const uint1 *sptr = (const uint1 *)ep.getsvalue().getstring();
			ep.setnvalue(sptr[0]);
		}
	}
	else
		ep.clear();
	return ES_NORMAL;
#endif /* MCCharToNum */

#ifdef /* MCByteToNum */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep . getsvalue() . getlength() != 1)
	{
		MCeerror->add(EE_BYTETONUM_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	ep . setnvalue(((uint1*)ep . getsvalue() . getstring())[0]);
	return ES_NORMAL;
#endif /* MCByteToNum */

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
#ifdef /* MCChunkOffset */ LEGACY_EXEC
	uint4 start = 0;
	if (offset != NULL)
	{
		if (offset->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_OFFSET_BADOFFSET, line, pos);
			return ES_ERROR;
		}
		start = ep.getuint4();
	}
	if (part->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_OFFSET_BADPART, line, pos);
		return ES_ERROR;
	}
	MCExecPoint epw(ep);
	if (whole->eval(epw) != ES_NORMAL)
	{
		MCeerror->add(EE_OFFSET_BADWHOLE, line, pos);
		return ES_ERROR;
	}
	uint4 i = 0;
	if (delimiter == CT_CHARACTER)
	{
		if (start <= epw.getsvalue().getlength())
		{
			MCString w(&epw.getsvalue().getstring()[start],
			           epw.getsvalue().getlength() - start);
			if (MCU_offset(ep.getsvalue(), w, i, ep.getcasesensitive()))
				ep.setnvalue(i + 1);
			else
				ep.setnvalue(0.0);
		}
		else
			ep.setnvalue(0.0);
	}
	else
	{
		uint4 chunkstart = 0;
		uint4 length = epw.getsvalue().getlength();
		const char *wptr = epw.getsvalue().getstring();
		if (delimiter == CT_WORD)
		{
			while (chunkstart < length && i < start)
			{
				while (chunkstart < length && isspace((uint1)wptr[chunkstart]))
					chunkstart++;
				if (wptr[chunkstart] == '"')
				{
					chunkstart++;
					while (chunkstart < length && wptr[chunkstart] != '"'
					        && wptr[chunkstart] != '\n')
						chunkstart++;
					if (chunkstart < length && wptr[chunkstart] == '"')
						chunkstart++;
				}
				else
					while (chunkstart < length && !isspace((uint1)wptr[chunkstart]))
						chunkstart++;
				i++;
			}
		}
		else
		{
			char c = delimiter == CT_LINE ? '\n' : ep.getitemdel();
			while (chunkstart < length && i < start)
				if (wptr[chunkstart++] == c)
					i++;
		}
		MCString w(&wptr[chunkstart], length - chunkstart);
		MCU_chunk_offset(ep, w, ep.getwholematches(), delimiter);
	}
	return ES_NORMAL;
#endif /* MCChunkOffset */

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
	}
    
    r_value . type = kMCExecValueTypeUInt;
}

void MCChunkOffset::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	part -> compile(ctxt);
	whole -> compile(ctxt);

	if (offset != nil)
		offset -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstantUInt(ctxt, 0);

	switch (delimiter)
	{
	case CT_ITEM:
		MCSyntaxFactoryEvalMethod(ctxt, kMCStringsEvalItemOffsetMethodInfo);
		break;
	case CT_LINE:
		MCSyntaxFactoryEvalMethod(ctxt, kMCStringsEvalLineOffsetMethodInfo);
		break;
	case CT_WORD:
		MCSyntaxFactoryEvalMethod(ctxt, kMCStringsEvalWordOffsetMethodInfo);
		break;
	case CT_CHARACTER:
		MCSyntaxFactoryEvalMethod(ctxt, kMCStringsEvalOffsetMethodInfo);
		break;
	}
}

#ifdef /* MCClickCharChunk */ LEGACY_EXEC
	if (MCclickfield == NULL)
		ep.clear();
	else
		MCclickfield->loccharchunk(ep, True);
	return ES_NORMAL;
#endif /* MCClickCharChunk */


#ifdef /* MCClickChunk */ LEGACY_EXEC
	if (MCclickfield == NULL)
		ep.clear();
	else
		MCclickfield->locchunk(ep, True);
	return ES_NORMAL;
#endif /* MCClickChunk */


#ifdef /* MCClickField */ LEGACY_EXEC
	if (MCclickfield != NULL)
	{
		MCclickfield->getprop(0, P_NUMBER, ep, False);
		ep.setstringf(MCclickfield->getparent()->gettype() == CT_CARD && MCclickfield->getstack()->hcaddress() ? "card field %d" : "field %d", ep.getuint4());
	}
	else
		ep.clear();
	return ES_NORMAL;
#endif /* MCClickField */


#ifdef /* MCClickH */ LEGACY_EXEC
	ep.setint(MCclicklocx);
	return ES_NORMAL;
#endif /* MCClickH */


#ifdef /* MCClickLine */ LEGACY_EXEC
	if (MCclickfield == NULL)
		ep.clear();
	else
		MCclickfield->locline(ep, True);
	return ES_NORMAL;
#endif /* MCClickLine */


#ifdef /* MCClickLoc */ LEGACY_EXEC
	ep.setpoint(MCclicklocx, MCclicklocy);
	return ES_NORMAL;
#endif /* MCClickLoc */

#ifdef /* MCClickStack */ LEGACY_EXEC
	if (MCclickstackptr == NULL)
	{
		ep.clear();
		return ES_NORMAL;
	}
	return MCclickstackptr->getprop(0, P_LONG_NAME, ep, False);
#endif /* MCClickStack */


#ifdef /* MCClickText */ LEGACY_EXEC
	if (MCclickfield == NULL)
		ep.clear();
	else
		MCclickfield->loctext(ep, True);
	return ES_NORMAL;
#endif /* MCClickText */

#ifdef /* MCClickV */ LEGACY_EXEC
	ep.setnvalue(MCclicklocy);
	return ES_NORMAL;
#endif /* MCClickV */

#ifdef /* MCClipboard */ LEGACY_EXEC
	bool t_success;
	t_success = true;

	if (MCclipboarddata -> Lock())
	{
		if (MCclipboarddata -> Contains(TRANSFER_TYPE_FILES, false))
			ep . setstaticcstring("files");
		else if (MCclipboarddata -> Contains(TRANSFER_TYPE_OBJECTS, false))
			ep . setstaticcstring("objects");
		else if (MCclipboarddata -> Contains(TRANSFER_TYPE_IMAGE, true))
			ep . setstaticcstring("image");
		else if (MCclipboarddata -> Contains(TRANSFER_TYPE_PRIVATE, false))
			ep . setstaticcstring("private");
		else if (MCclipboarddata -> Contains(TRANSFER_TYPE_TEXT, true))
			ep . setstaticcstring("text");
		else
			ep . setstaticcstring("empty");

		MCclipboarddata -> Unlock();
	}
	else
		t_success = false;

	if (!t_success)	
	{
		ep . clear();
		MCresult -> sets("unable to access clipboard");
	}

	return ES_NORMAL;
#endif /* MCClipboard */


#ifdef /* MCCommandKey */ LEGACY_EXEC
	ep.setstaticcstring(MCU_ktos((MCscreen->querymods() & MS_CONTROL) != 0));
	return ES_NORMAL;
#endif /* MCCommandKey */


#ifdef /* MCCompress */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_COMPRESS_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	uint4 size = ep.getsvalue().getlength() + 12 + GZIP_HEADER_SIZE + 8;
	size += size / 999;  //dest must be "0.1% larger than (source) plus 12 bytes"
	char *newbuffer = new char[size];
	if (newbuffer == NULL)
		return ES_ERROR;
	memcpy(newbuffer, gzip_header, GZIP_HEADER_SIZE);
	z_stream zstrm;
	memset((char *)&zstrm, 0, sizeof(z_stream));
	zstrm.next_in = (unsigned char *)ep.getsvalue().getstring();
	zstrm.avail_in = ep.getsvalue().getlength();
	zstrm.next_out = (unsigned char *)newbuffer + GZIP_HEADER_SIZE;
	zstrm.avail_out = size - GZIP_HEADER_SIZE - 8;
	if (deflateInit2(&zstrm, Z_DEFAULT_COMPRESSION,
	                 Z_DEFLATED, -MAX_WBITS, 8, 0) != Z_OK
	        || deflate(&zstrm, Z_FINISH) != Z_STREAM_END
	        || deflateEnd(&zstrm) != Z_OK)
	{
		delete newbuffer;
		MCeerror->add(EE_COMPRESS_ERROR, line, pos);
		return ES_ERROR;
	}
	uint4 osize = zstrm.total_out + GZIP_HEADER_SIZE;
	uint4 check = crc32(0L, Z_NULL, 0);
	check = crc32(check, (unsigned char *)ep.getsvalue().getstring(),
	              ep.getsvalue().getlength());
	MCswapbytes = !MCswapbytes;
	swap_uint4(&check);
	memcpy(newbuffer + osize, &check, 4);
	check = ep.getsvalue().getlength();
	swap_uint4(&check);
	memcpy(newbuffer + osize + 4, &check, 4);
	MCswapbytes = !MCswapbytes;
	char *obuff = ep.getbuffer(0);
	delete obuff;
	ep.setbuffer(newbuffer, size);
	ep.setlength(osize + 8);
	return ES_NORMAL;
#endif /* MCCompress */

#ifdef /* MCControlKey */ LEGACY_EXEC
	ep.setstaticcstring(MCU_ktos((MCscreen->querymods() & MS_MAC_CONTROL) != 0));
	return ES_NORMAL;
#endif /* MCControlKey */


#ifdef /* MCColorNames */ LEGACY_EXEC
	MCscreen->getcolornames(ep);
	return ES_NORMAL;
#endif /* MCColorNames */

Parse_stat MCCommandArguments::parse(MCScriptPoint &sp, Boolean the)
{
    if (!get0or1param(sp, &argument_index, the))
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
    if (argument_index == NULL)
    {
        MCExecValueTraits<MCArrayRef>::set(r_value, MCValueRetain(MCcommandarguments));
        return;
    }
    else
    {
        integer_t t_index;
        if (!ctxt . EvalExprAsInt(argument_index, EE_COMMANDARGUMENTS_BADPARAM, t_index))
            return;

        MCStringRef t_value;
        // If the index is wrong (< 0 or > argument count) then we return empty
        if (!MCArrayFetchValueAtIndex(MCcommandarguments, t_index, (MCValueRef&)t_value))
            t_value = kMCEmptyString;

        MCExecValueTraits<MCStringRef>::set(r_value, MCValueRetain(t_value));
    }
}

void MCCommandName::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    if (MCcommandname != NULL)
        MCExecValueTraits<MCStringRef>::set(r_value, MCcommandname);
    else
        MCExecValueTraits<MCStringRef>::set(r_value, kMCEmptyString);
}

#ifdef /* MCCommandNames */ LEGACY_EXEC
	ep.clear();
	MCScriptPoint sp(ep);
	return sp.getcommands(ep);
#endif /* MCCommandNames */


#ifdef /* MCConstantNames */ LEGACY_EXEC
	ep.clear();
	MCScriptPoint sp(ep);
	return sp.getconstants(ep);
#endif /* MCConstantNames */


#ifdef /* MCDate */ LEGACY_EXEC
	MCD_date(P_UNDEFINED, ep);
	return ES_NORMAL;
#endif /* MCDate */



#ifdef /* MCDateFormat */ LEGACY_EXEC
	MCD_dateformat(P_UNDEFINED, ep);
	return ES_NORMAL;
#endif /* MCDateFormat */

#ifdef /* MCDecompress */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_DECOMPRESS_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return do_decompress(ep, line, pos);
#endif /* MCDecompress */

#ifdef /* MCDecompress::do_decompress */ LEGACY_EXEC
Exec_stat MCDecompress::do_decompress(MCExecPoint& ep, uint2 line, uint2 pos)
{
	const char *sptr = ep.getsvalue().getstring();
	if (ep.getsvalue().getlength() < 10 || sptr[0] != gzip_header[0]
	        || sptr[1] != gzip_header[1] || sptr[2] != gzip_header[2]
	        || sptr[3] & GZIP_RESERVED)
	{
		MCeerror->add(EE_DECOMPRESS_NOTCOMPRESSED, line, pos);
		return ES_ERROR;
	}
	MCswapbytes = !MCswapbytes;
	uint4 startindex = 10;
	if (sptr[3] & GZIP_EXTRA_FIELD)
	{ /* skip the extra field */
		uint2 len;
		memcpy(&len, &sptr[startindex], 2);
		swap_uint2(&len);
		startindex += len;
	}
	if (sptr[3] & GZIP_ORIG_NAME) /* skip the original file name */
		while (sptr[startindex++])
			;
	if (sptr[3] & GZIP_COMMENT)   /* skip the .gz file comment */
		while (sptr[startindex++])
			;
	if (sptr[3] & GZIP_HEAD_CRC) /* skip the header crc */
		startindex += 2;
	uint4 size;
	memcpy(&size, &sptr[ep.getsvalue().getlength() - 4], 4);
	swap_uint4(&size);
	MCswapbytes = !MCswapbytes;
	if (size == 0)
	{
		ep.clear();
		return ES_NORMAL;
	}
	char *newbuffer = new char[size];
	if (newbuffer == NULL)
	{
		// SMR 1935 no need to abort rest of script...
		MCabortscript = False;
		return ES_ERROR;
	}

	z_stream zstrm;
	memset((char *)&zstrm, 0, sizeof(z_stream));
	zstrm.next_in = (unsigned char *)sptr + startindex;
	zstrm.avail_in = ep.getsvalue().getlength() - startindex - 8;
	zstrm.next_out = (unsigned char *)newbuffer;
	zstrm.avail_out = size;
	int err;
	if (inflateInit2(&zstrm, -MAX_WBITS) != Z_OK
	        || (err = inflate(&zstrm, Z_FINISH)) != Z_STREAM_END
	        && err != Z_OK && err != Z_BUF_ERROR // bug on OS X returns this error
	        || inflateEnd(&zstrm) != Z_OK)
	{
		delete newbuffer;
		MCeerror->add
		(EE_DECOMPRESS_ERROR, line, pos);
		return ES_ERROR;
	}
	char *obuff = ep.getbuffer(0);
	delete obuff;
	ep.setbuffer(newbuffer, size);
	ep.setlength(size);
	return ES_NORMAL;
}
#endif /* MCDecompress::do_decompress */


#ifdef /* MCDirectories */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}
	MCS_getentries(ep, false, false);
	return ES_NORMAL;
#endif /* MCDirectories */


#ifdef /* MCDiskSpace */ LEGACY_EXEC
	ep . setnvalue(MCS_getfreediskspace());
	return ES_NORMAL;
#endif /* MCDiskSpace */



#ifdef /* MCDNSServers */ LEGACY_EXEC
	if (!MCSecureModeCheckNetwork())
		return ES_ERROR;

	MCS_getDNSservers(ep);
	return ES_NORMAL;
#endif /* MCDNSServers */


#ifdef /* MCDragDestination */ LEGACY_EXEC
	if (MCdragdest != NULL)
		return MCdragdest->getprop(0, P_LONG_ID, ep, False);
	else
		ep.clear();
	return ES_NORMAL;
#endif /* MCDragDestination */


#ifdef /* MCDragSource */ LEGACY_EXEC
	if (MCdragsource != NULL)
		return MCdragsource->getprop(0, P_LONG_ID, ep, False);
	else
		ep.clear();
	return ES_NORMAL;
#endif /* MCDragSource */


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


#ifdef /* MCDriverNames */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}
	if (type != NULL)
	{
		if (type->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_DRIVERNAMES_BADTYPE, line, pos);
			return ES_ERROR;
		}
	}
	else
		ep.clear();
	return MCS_getdevices(ep) ? ES_NORMAL : ES_ERROR;
#endif /* MCDriverNames */


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


#ifdef /* MCDrives */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}
	if (type != NULL)
	{
		if (type->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_DRIVES_BADTYPE, line, pos);
			return ES_ERROR;
		}
	}
	else
		ep.clear();
	return MCS_getdrives(ep) ? ES_NORMAL : ES_ERROR;
#endif /* MCDrives */


#ifdef /* MCDropChunk */ LEGACY_EXEC
	if (MCdropfield == NULL)
		ep.clear();
	else
		MCdropfield->returnchunk(ep, MCdropchar, MCdropchar);
	return ES_NORMAL;
#endif /* MCDropChunk */



#ifdef /* MCQTEffects */ LEGACY_EXEC
	extern void MCQTEffectsList(MCExecPoint& ep);
	MCQTEffectsList(ep);
	return ES_NORMAL;
#endif /* MCQTEffects */

#ifdef /* MCRecordCompressionTypes */ LEGACY_EXEC
Exec_stat MCRecordCompressionTypes::eval(MCExecPoint &ep)
{
#ifdef FEATURE_PLATFORM_RECORDER
    
    ep . clear();
    
    extern MCPlatformSoundRecorderRef MCrecorder;
    
    if (MCrecorder == nil)
        MCPlatformSoundRecorderCreate(MCrecorder);
    
    if (MCrecorder != nil)
    {
        MCPlatformSoundRecorderListCompressorsState t_state;
        t_state . ep = &ep;
        t_state . first = true;
        
        MCPlatformSoundRecorderListCompressors(MCrecorder, list_compressors_callback, &t_state);
    }
#else
	extern void MCQTGetRecordCompressionList(MCExecPoint& ep);
	MCQTGetRecordCompressionList(ep);
#endif
    
	return ES_NORMAL;
#endif /* MCRecordCompressionTypes */



#ifdef /* MCRecordLoudness */ LEGACY_EXEC
    
#ifdef FEATURE_PLATFORM_RECORDER
    extern MCPlatformSoundRecorderRef MCrecorder;
    
    double t_loudness;
    t_loudness = 0;
    
    if (MCrecorder != nil)
        t_loudness = MCPlatformSoundRecorderGetLoudness(MCrecorder);
    
    ep . setuint(floor(t_loudness));
        
#else
	extern void MCQTGetRecordLoudness(MCExecPoint& ep);
	MCQTGetRecordLoudness(ep);
#endif
    
	return ES_NORMAL;
#endif /* MCRecordLoudness */


#ifdef /* MCEncrypt */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_ENCRYPT_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	char *t_enc = nil;
	if (!MCStackSecurityEncryptString(ep.getsvalue().getstring(), ep.getsvalue().getlength(), t_enc))
	{
		return ES_ERROR;
	}
	
	ep.copysvalue(t_enc, strlen(t_enc));
	MCCStringFree(t_enc);

	return ES_NORMAL;
#endif /* MCEncrypt */

#ifdef /* MCEnvironment */ LEGACY_EXEC
	ep . setstaticcstring(MCModeGetEnvironment());
	return ES_NORMAL;
#endif /* MCEnvironment */


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
#ifdef /* MCExists */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;
	MCerrorlock++;
	ep.setboolean(object->getobj(ep, optr, parid, True) == ES_NORMAL);
	MCerrorlock--;
	return ES_NORMAL;
#endif /* MCExists */

	MCInterfaceEvalThereIsAnObject(ctxt, object, r_value . bool_value);
    r_value . type = kMCExecValueTypeBool;
}

void MCExists::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	object -> compile(ctxt);

	MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalThereIsAnObjectMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
}

#ifdef /* MCExtents */ LEGACY_EXEC
	if (source -> eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_EXTENTS_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	
	// MW-2008-07-01: [[ Bug ]] Make sure we only fetch the array if the type of
	//   the exec point is actually ARRAY, otherwise we get strange effects...
	if (ep . getformat() == VF_ARRAY)
	{
		MCVariableValue *t_array;
		Boolean t_delete_array;
		ep . takearray(t_array, t_delete_array);
		t_array -> getextents(ep);
		if (t_delete_array)
			delete t_array;
	}
	else
		ep . clear();
	
	return ES_NORMAL;
#endif /* MCExtents */

#ifdef /* MCTheFiles */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}
	MCS_getentries(ep, true, false);
	return ES_NORMAL;
#endif /* MCTheFiles */

#ifdef /* MCFlushEvents */ LEGACY_EXEC
	static const char *enames[FE_LAST] =
	    { "all", "mousedown", "mouseup",
	      "keydown", "keyup", "autokey",
	      "disk", "activate", "highlevel",
	      "system"
	    };
	if (type->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_FLUSHEVENTS_BADTYPE, line, pos);
		return ES_ERROR;
	}
	uint2 i;
	for (i = 0 ; i < FE_LAST ; i++)
		if (ep.getsvalue() == enames[i])
			MCscreen->flushevents(i);
	ep.clear();
	return ES_NORMAL;
#endif /* MCFlushEvents */

#ifdef /* MCFocusedObject */ LEGACY_EXEC
	if (MCfocusedstackptr != NULL)
	{
		MCControl *cptr = MCfocusedstackptr->getcard()->getkfocused();
		if (cptr != NULL)
			return cptr->getprop(0, P_LONG_ID, ep, False);
		return MCfocusedstackptr->getcard()->getprop(0, P_LONG_ID, ep, False);
	}
	ep.clear();
	return ES_NORMAL;
#endif /* MCFocusedObject */


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
#ifdef /* MCFontNames */ LEGACY_EXEC
	if (type != NULL)
	{
		if (type->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_FONTNAMES_BADTYPE, line, pos);
			return ES_ERROR;
		}
	}
	char *type = ep.getsvalue().clone();
	MCdispatcher->getfontlist()->getfontnames(ep,type);

	delete type;
	return ES_NORMAL;
#endif /* MCFontNames */

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

#ifdef /* MCFontLanguage */ LEGACY_EXEC
	if (fontname->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_FONTSIZES_BADFONTNAME, line, pos);
		return ES_ERROR;
	}
	char *fname = ep.getsvalue().clone();
	uint1 charset = MCscreen->fontnametocharset(fname);
	ep.setstaticcstring(MCU_charsettolanguage(charset));
	delete fname;
	return ES_NORMAL;
#endif /* MCFontLanguage */

#ifdef /* MCFontSizes */ LEGACY_EXEC
	if (fontname->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_FONTSIZES_BADFONTNAME, line, pos);
		return ES_ERROR;
	}
	char *fname = ep.getsvalue().clone();
	MCdispatcher->getfontlist()->getfontsizes(fname, ep);
	delete fname;
	return ES_NORMAL;
#endif /* MCFontSizes */

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
#ifdef /* MCFontStyles */ LEGACY_EXEC
	if (fontname->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_FONTSTYLES_BADFONTNAME, line, pos);
		return ES_ERROR;
	}
	char *fname = ep.getsvalue().clone();
	uint2 fsize;
	if (fontsize->eval(ep) != ES_NORMAL
	        || ep.getuint2(fsize, line, pos, EE_FONTSTYLES_BADFONTSIZE) != ES_NORMAL)
		return ES_ERROR;
	MCdispatcher->getfontlist()->getfontstyles(fname, fsize, ep);
	delete fname;

	return ES_NORMAL;
#endif /* MCFontStyles */

    MCAutoStringRef t_fontname;
    if (!ctxt . EvalExprAsStringRef(fontname, EE_FONTSTYLES_BADFONTNAME, &t_fontname))
        return;
    uinteger_t fsize;
    if (!ctxt . EvalExprAsStrictUInt(fontsize, EE_FONTSTYLES_BADFONTSIZE, fsize))
        return;
    
	MCTextEvalFontStyles(ctxt, *t_fontname, fsize, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

void MCFontStyles::compile(MCSyntaxFactoryRef ctxt)
{
	compile_with_args(ctxt, kMCTextEvalFontStylesMethodInfo, fontname, fontsize);
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
#ifdef /* MCFormat */ LEGACY_EXEC
	MCExecPoint ep2(ep);
	if (params->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_FORMAT_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCString s;
	char *string = ep.getsvalue().clone();
	const char *format = string;
	char *sbuffer = NULL;
	uint4 sbuffersize = 0;
	MCParameter *paramptr = params->getnext();

	ep.clear();
	while (*format)
	{
		char newFormat[40];
		char *dptr = newFormat;
		int4 width = 0;

		uint4 precision = 0;
		uint4 size;
		int4 intValue;
		real8 doubleValue = 0.0;
		uint4 whichValue = PTR_VALUE;
		char *ptrValue = NULL;
		Boolean useShort = False;
		const char *end;

		if (*format == '\\')
		{
			char result = 0;
			switch (*++format)
			{
			case 'a':
				result = '\a';
				break;
			case 'b':
				result = '\b';
				break;
			case 'f':
				result = '\f';
				break;
			case 'n':
				result = '\n';
				break;
			case 'r':
				result = '\r';
				break;
			case 't':
				result = '\t';
				break;
			case 'v':
				result = '\v';
				break;
			case '\\':
				result = '\\';
				break;
			case '?':
				result = '?';
				break;
			case '\'':
				result = '\'';
				break;
			case '"':
				result = '"';
				break;
			case 'x':
				if (isxdigit(*++format))
				{
					char buffer[3];
					memcpy(buffer, format, 2);
					buffer[2] = '\0';
					result = (char)strtoul(buffer, (char **)&end, 16);
					format += end - buffer - 1;
				}
				break;
			default:
				if (isdigit((uint1)*format))
				{
					const char *sptr = format;
					while (isdigit((uint1)*format) && format - sptr < 3)
						result = (result << 3) + (*format++ - '0');
					format--;
				}
				break;
			}
			ep.appendchar(result);
			format++;
			continue;
		}
		if (*format != '%')
		{
			const char *startptr = format;
			while (*format && *format != '%' && *format != '\\')
				format++;
			ep.appendchars(startptr, format - startptr);
			continue;
		}

		if (format[1] == '%')
		{
			ep.appendchar('%');
			format += 2;
			continue;
		}
		*dptr++ = *format++;
		while (*format == '-' || *format == '#' || *format == '0'
		        || *format == ' ' || *format == '+')
			*dptr++ = *format++;
		if (isdigit((uint1)*format))
		{
			width = strtol(format, (char **)&end, 10);
			format = end;
		}
		else
			if (*format == '*')
			{
				if (paramptr == NULL || paramptr->eval(ep2) != ES_NORMAL)
					goto fmtError;
				ep2.getint4(width, line, pos, EE_FORMAT_BADSOURCE);
				paramptr = paramptr->getnext();
				format++;
			}
		if (width != 0)
		{
			sprintf(dptr, "%d", width);
			while (*++dptr)
				;
		}
		if (*format == '.')
			*dptr++ = *format++;
		if (isdigit((uint1)*format))
		{
			precision = strtoul(format, (char **)&end, 10);
			format = end;
		}
		else
			if (*format == '*')
			{
				if (paramptr == NULL || paramptr->eval(ep2) != ES_NORMAL)
					goto fmtError;
				ep2.getuint4(precision, line, pos, EE_FORMAT_BADSOURCE);
				paramptr = paramptr->getnext();
				format++;
			}
		if (precision != 0)
		{
			sprintf(dptr, "%d", precision);
			while (*++dptr)
				;
		}
		if (*format == 'l')
			format++;
		else
			if (*format == 'h')
			{
				useShort = 1;
				*dptr++ = *format++;
			}
		*dptr++ = *format;
		*dptr = 0;
		switch (*format)
		{
		case 'i':
			dptr[-1] = 'd';
		case 'd':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
			if (paramptr == NULL || paramptr->eval(ep2) != ES_NORMAL)
				goto fmtError;
			ep2.getint4(intValue, line, pos, EE_FORMAT_BADSOURCE);
			whichValue = INT_VALUE;
			size = I4L;
			break;
		case 's':
			if (paramptr == NULL || paramptr->eval(ep2) != ES_NORMAL)
				goto fmtError;
			size = ep2.getsvalue().getlength();
			ptrValue = ep2.getsvalue().clone();
			break;
		case 'c':
			if (paramptr == NULL || paramptr->eval(ep2) != ES_NORMAL)
				goto fmtError;
			ep2.getint4(intValue, line, pos, EE_FORMAT_BADSOURCE);
			whichValue = INT_VALUE;
			size = 2;
			break;
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			if (paramptr == NULL || paramptr->eval(ep2) != ES_NORMAL
			        || ep2.ton() != ES_NORMAL)
				goto fmtError;
			doubleValue = ep2.getnvalue();
			whichValue = DOUBLE_VALUE;
			size = R8L;
			break;
		default:
			goto fmtError;
		}
		paramptr = paramptr->getnext();
		format++;
		if (width < 0)
			width = -width;
		if (width > (int4)size)
			size = width;
		if (size + 1 > sbuffersize)
		{
			delete sbuffer;
			sbuffer = new char[size + 1];
			sbuffersize = size + 1;
		}
		switch (whichValue)
		{
		case DOUBLE_VALUE:
			sprintf(sbuffer, newFormat, doubleValue);
			break;
		case INT_VALUE:
			if (useShort)
				sprintf(sbuffer, newFormat, (short)intValue);
			else
				sprintf(sbuffer, newFormat, intValue);
			break;
		default:
			sprintf(sbuffer, newFormat, ptrValue);
			delete ptrValue;
			break;
		}
		ep.appendcstring(sbuffer); 
	}

	delete string;
	delete sbuffer;
	return ES_NORMAL;

fmtError:
	MCeerror->add(EE_FORMAT_BADSOURCE, line, pos);
	delete string;
	delete sbuffer;
	return ES_ERROR;
#endif /* MCFormat */

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

void MCFormat::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	uindex_t t_count;
	t_count = 0;

	for (MCParameter *t_parameter = params; t_parameter != nil; t_parameter = t_parameter -> getnext())
	{
		t_parameter -> compile(ctxt);
		t_count++;
	}
	
	MCSyntaxFactoryEvalList(ctxt, t_count - 1);

	MCSyntaxFactoryEvalMethod(ctxt, kMCStringsEvalFormatMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
}


#ifdef /* MCFoundChunk */ LEGACY_EXEC
	if (MCfoundfield == NULL)
		ep.clear();
	else
		MCfoundfield->foundchunk(ep);
	return ES_NORMAL;
#endif /* MCFoundChunk */


#ifdef /* MCFoundField */ LEGACY_EXEC
	if (MCfoundfield != NULL)
	{
		MCfoundfield->getprop(0, P_NUMBER, ep, False);
		ep.setstringf("field %d", ep.getuint4());
	}
	else
		ep.clear();
	return ES_NORMAL;
#endif /* MCFoundField */


#ifdef /* MCFoundLine */ LEGACY_EXEC
	if (MCfoundfield == NULL)
		ep.clear();
	else
		MCfoundfield->foundline(ep);
	return ES_NORMAL;
#endif /* MCFoundLine */



#ifdef /* MCFoundLoc */ LEGACY_EXEC
	if (MCfoundfield == NULL)
		ep.clear();
	else
		MCfoundfield->foundloc(ep);
	return ES_NORMAL;
#endif /* MCFoundLoc */


#ifdef /* MCFoundText */ LEGACY_EXEC
	if (MCfoundfield == NULL)
		ep.clear();
	else
		MCfoundfield->foundtext(ep);
	return ES_NORMAL;
#endif /* MCFoundText */



#ifdef /* MCFunctionNames */ LEGACY_EXEC
	ep.clear();
	MCScriptPoint sp(ep);
	return sp.getfactors(ep, TT_FUNCTION);
#endif /* MCFunctionNames */

#ifdef /* MCGlobalLoc */ LEGACY_EXEC
	int2 x, y;
	if (point->eval(ep) != ES_NORMAL || !MCU_stoi2x2(ep.getsvalue(), x, y))
	{
		MCeerror->add(EE_GLOBALLOC_NAP, line, pos, ep.getsvalue());
		return ES_ERROR;
	}

	// IM-2013-10-09: [[ FullscreenMode ]] Update to use stack coord conversion methods
	MCPoint t_loc;
	t_loc = MCPointMake(x, y);
	t_loc = MCdefaultstackptr->stacktogloballoc(t_loc);

	ep.setpoint(t_loc.x, t_loc.y);

	return ES_NORMAL;
#endif /* MCGlobalLoc */

#ifdef /* MCGlobals */ LEGACY_EXEC
	ep.clear();
	MCVariable *tmp;
	uint2 i = 0;
	for (tmp = MCglobals ; tmp != NULL ; tmp = tmp->getnext())
		if (!tmp->isfree() || tmp->isarray())
			ep.concatnameref(tmp->getname(), EC_COMMA, i++ == 0);
	return ES_NORMAL;
#endif /* MCGlobals */

#ifdef /* MCHasMemory */ LEGACY_EXEC
	if (amount->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_HASMEMORY_BADAMOUNT, line, pos);
		return ES_ERROR;
	}
	uint4 bytes = ep.getuint4();

	char *dummy = (char *)malloc(bytes);
	ep.setboolean(dummy != NULL);
	free(dummy);

	return ES_NORMAL;
#endif /* MCHasMemory */

#ifdef /* MCHeapSpace */ LEGACY_EXEC
	ep.setstaticcstring(HEAP_SPACE);
	return ES_NORMAL;
#endif /* MCHeapSpace */

#ifdef /* MCHostAddress */ LEGACY_EXEC
	if (socket->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_HOSTADDRESS_BADSOCKET, line, pos);
		return ES_ERROR;
	}
	char *name = ep.getsvalue().clone();
	uint2 index;
	if (IO_findsocket(name, index))
		MCS_ha(ep, MCsockets[index]);
	else
		ep.setstaticcstring("not an open socket");
	delete name;
	return ES_NORMAL;
#endif /* MCHostAddress */

#ifdef /* MCHostAtoN */ LEGACY_EXEC
	if (address->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_HOSTATON_BADADDRESS, line, pos);
		return ES_ERROR;
	}
	MCS_aton(ep);
	
	// We only allow an address to name lookup if the resulting is the secure domain
	// unless we have network access.
	if (!MCSecureModeCanAccessNetwork() && !MCModeCanAccessDomain(ep . getcstring()))
	{
		MCeerror -> add(EE_NETWORK_NOPERM, line, pos);
		return ES_ERROR;
	}

	return ES_NORMAL;
#endif /* MCHostAtoN */

#ifdef /* MCHostName */ LEGACY_EXEC
	MCS_hn(ep);
	return ES_NORMAL;
#endif /* MCHostName */


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
#ifdef /* MCHostNtoA */ LEGACY_EXEC
	if (name->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_HOSTNTOA_BADNAME, line, pos);
		return ES_ERROR;
	}

	MCExecPoint ep2;
	ep2.clear();

	if (message && message->eval(ep2) != ES_NORMAL)
	{
		MCeerror->add(EE_OPEN_BADMESSAGE, line, pos);
		return ES_ERROR;
	}
	
	// We only allow an name to address lookup to occur for the secure domain.
	if (!MCSecureModeCanAccessNetwork() && !MCModeCanAccessDomain(ep . getcstring()))
	{
		MCeerror -> add(EE_NETWORK_NOPERM, line, pos);
		return ES_ERROR;
	}

	MCS_ntoa(ep, ep2);
	
	return ES_NORMAL;
#endif /* MCHostNtoA */

    MCAutoStringRef t_hostname;
    if (!ctxt . EvalExprAsStringRef(name, EE_HOSTNTOA_BADNAME, &t_hostname))
        return;
    
	MCNewAutoNameRef t_message;
    if (!ctxt . EvalOptionalExprAsNameRef(message, kMCEmptyName, EE_OPEN_BADMESSAGE, &t_message))
        return;

	MCNetworkEvalHostNameToAddress(ctxt, *t_hostname, *t_message, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

void MCHostNtoA::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	name -> compile(ctxt);

	if (message)
		message -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstant(ctxt, kMCEmptyName);

	MCSyntaxFactoryEvalMethod(ctxt, kMCNetworkEvalHostNameToAddressMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
}

void MCInsertScripts::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCInsertScripts */ LEGACY_EXEC
	ep.clear();
	MCObjectList *lptr = front ? MCfrontscripts : MCbackscripts;
	if (lptr != NULL)
	{
		MCExecPoint ep2(ep);
		MCObjectList *optr = lptr;
		bool first = true;
		do
		{
			if (!optr->getremoved())
			{
				optr->getobject()->getprop(0, P_LONG_ID, ep2, False);
				ep.concatmcstring(ep2.getsvalue(), EC_RETURN, first);
				first = false;
			}
			optr = optr->next();
		}
		while (optr != lptr);
	}
	return ES_NORMAL;
#endif /* MCInsertScripts */

	if (front)
		MCEngineEvalFrontScripts(ctxt, r_value .stringref_value);
	else
		MCEngineEvalBackScripts(ctxt, r_value . stringref_value);
    
    r_value . type = kMCExecValueTypeStringRef;
}


#ifdef /* MCInterrupt */ LEGACY_EXEC
	ep.setboolean(MCinterrupt);
	return ES_NORMAL;
#endif /* MCInterrupt */


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
	
	o1 = new MCChunk(False);
	o2 = new MCChunk(False);
	
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
#ifdef /* MCIntersect */ LEGACY_EXEC
	MCObject *o1ptr, *o2ptr;
	uint4 parid;
	if (o1->getobj(ep, o1ptr, parid, True) != ES_NORMAL
		|| o2->getobj(ep, o2ptr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_INTERSECT_NOOBJECT, line, pos);
		return ES_ERROR;
	}
	
	// MW-2011-09-23: [[ Collides ]] Determine the threshold of the alpha mask
	//   conversion. Either an integer, "bounds" => 0, "pixels" => 1, "opaque pixels" => 255
	uint32_t t_threshold;
	if (threshold != nil)
	{
		if (threshold -> eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_INTERSECT_BADTHRESHOLD, line, pos);
			return ES_ERROR;
		}
		
		// MW-2013-04-12: [[ Bug 10844 ]] Make sure we use ton(), otherwise it assumes
		//   input is a string.
		if (ep . ton() == ES_NORMAL)
			t_threshold = ep . getuint4();
		else
		{
			MCString t_token;
			t_token = ep . getsvalue();
			if (t_token == "bounds")
				t_threshold = 0;
			else if (t_token == "pixels")
				t_threshold = 1;
			else if (t_token == "opaque pixels")
				t_threshold = 255;
			else
			{
				MCeerror -> add(EE_INTERSECT_ILLEGALTHRESHOLD, line, pos);
				return ES_ERROR;
			}
		}
	}
	else
		t_threshold = 0;

	ep . setboolean(o1ptr -> intersects(o2ptr, t_threshold));
	
	return ES_NORMAL;
#endif /* MCIntersect */

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

void MCIntersect::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	o1 -> compile_object_ptr(ctxt);
	o2 -> compile_object_ptr(ctxt);

	if (threshold != nil)
	{
		threshold -> compile(ctxt);
		MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalIntersectWithThresholdMethodInfo);
	}
	else
		MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalIntersectMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
}

#ifdef /* MCIsNumber */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_ISNUMBER_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	real8 r;
	ep.setboolean(MCU_stor8(ep.getsvalue(), r));
	return ES_NORMAL;
#endif /* MCIsNumber */

#ifdef /* MCIsoToMac */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_ISOTOMAC_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	ep.grabsvalue();
	IO_iso_to_mac(ep.getbuffer(0), ep.getsvalue().getlength());
	return ES_NORMAL;
#endif /* MCIsoToMac */

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
		        || (te->which != P_DRAG_DATA && te->which != P_CLIPBOARD_DATA))
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
#ifdef /* MCKeys */ LEGACY_EXEC
	if (source != NULL)
	{
		if (source -> eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_KEYS_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		
		// MW-2008-07-01: [[ Bug ]] Make sure we only fetch the array if the type of
		//   the exec point is actually ARRAY, otherwise we get strange effects...
		if (ep . getformat() == VF_ARRAY)
		{
			MCVariableValue *t_array;
			Boolean t_delete_array;
			ep . takearray(t_array, t_delete_array);
			t_array -> getkeys(ep);
			if (t_delete_array)
				delete t_array;
		}
		else
			ep . clear();
	}
	else
	{
		MCTransferData *t_data;
		if (which == P_DRAG_DATA)
			t_data = MCdragdata;
		else
			t_data = MCclipboarddata;

		bool t_success;
		t_success = true;

		if (t_data -> Lock())
		{
			MCTransferType *t_types;
			uint4 t_count;
			if (t_data -> Query(t_types, t_count))
			{
				ep . clear();
				for(uint4 i = 0; i < t_count; ++i)
				{
					switch(t_types[i])
					{
					case TRANSFER_TYPE_TEXT:
						ep . concatcstring("text", EC_RETURN, i == 0);
						break;

					case TRANSFER_TYPE_UNICODE_TEXT:
						ep . concatcstring("unicode", EC_RETURN, i == 0);
						ep . concatcstring("text", EC_RETURN, false);
						break;

					case TRANSFER_TYPE_STYLED_TEXT:
						// MW-2014-03-12: [[ ClipboardStyledText ]] Synthentic 'styledText' key - always present
						//   if styles is on the clipboard.
						ep . concatcstring("styledText", EC_RETURN, i == 0);
						ep . concatcstring("styles", EC_RETURN, false);
						ep . concatcstring("rtf", EC_RETURN, false);
						ep . concatcstring("unicode", EC_RETURN, false);
						ep . concatcstring("text", EC_RETURN, false);
						break;

					case TRANSFER_TYPE_IMAGE:
						ep . concatcstring("image", EC_RETURN, i == 0);
						break;

					case TRANSFER_TYPE_FILES:
						ep . concatcstring("files", EC_RETURN, i == 0);
						ep . concatcstring("text", EC_RETURN, false);
						break;

					case TRANSFER_TYPE_PRIVATE:
						ep . concatcstring("private", EC_RETURN, i == 0);
						break;

					case TRANSFER_TYPE_OBJECTS:
						ep . concatcstring("objects", EC_RETURN, i == 0);
						break;

					default:
						// MW-2009-04-05: Stop GCC warning
					break;
					}
				}
			}
			else
				t_success = false;

			t_data -> Unlock();
		}
		else
			t_success = false;

		if (!t_success)
		{
			ep . clear();
			MCresult -> sets("unable to query clipboard");
		}
	}

	return ES_NORMAL;
#endif /* MCKeys */
    
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
		else
			MCPasteboardEvalClipboardKeys(ctxt, r_value . stringref_value);
        r_value . type = kMCExecValueTypeStringRef;
	}
}

void MCKeys::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	if (source != nil)
	{
		source -> compile(ctxt);
		MCSyntaxFactoryEvalMethod(ctxt, kMCArraysEvalKeysMethodInfo);
	}
	else
	{
		if (which == P_DRAG_DATA)
			MCSyntaxFactoryEvalMethod(ctxt, kMCPasteboardEvalDragDropKeysMethodInfo);
		else
			MCSyntaxFactoryEvalMethod(ctxt, kMCPasteboardEvalClipboardKeysMethodInfo);
	}

	MCSyntaxFactoryEndExpression(ctxt);
}


#ifdef /* MCKeysDown */ LEGACY_EXEC
	MCscreen->getkeysdown(ep);
	return ES_NORMAL;
#endif /* MCKeysDown */

#ifdef /* MCLength */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_LENGTH_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	ep.setnvalue(ep.getsvalue().getlength());
	return ES_NORMAL;
#endif /* MCLength */

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


#ifdef /* MCLicensed */ LEGACY_EXEC
	ep . setboolean(MCModeGetLicensed());
	return ES_NORMAL;
#endif /* MCLicensed */

#ifdef /* MCLocalLoc */ LEGACY_EXEC
	int2 x, y;
	if (point->eval(ep) != ES_NORMAL || !MCU_stoi2x2(ep.getsvalue(), x, y))
	{
		MCeerror->add(EE_LOCALLOC_NAP, line, pos, ep.getsvalue());
		return ES_ERROR;
	}

	// IM-2013-10-09: [[ FullscreenMode ]] Update to use stack coord conversion methods
	MCPoint t_loc;
	t_loc = MCPointMake(x, y);
	t_loc = MCdefaultstackptr->globaltostackloc(t_loc);

	ep.setpoint(t_loc.x, t_loc.y);

	return ES_NORMAL;
#endif /* MCLocalLoc */

Parse_stat MCLocals::parse(MCScriptPoint &sp, Boolean the)
{
	return MCFunction::parse(sp, the);
}

#ifdef /* MCLocals */ LEGACY_EXEC
	// MW-2013-11-15: [[ Bug 11277 ]] Server mode may call this outwith a handler.
	
	if (ep . gethandler() != nil)
		return ep . gethandler() -> getvarnames(ep, False);
	
	ep.clear();
	ep . gethlist() -> appendlocalnames(ep);
	
	return ES_NORMAL;
#endif /* MCLocals */


#ifdef /* MCMachine */ LEGACY_EXEC
	ep.setstaticcstring(MCS_getmachine());
	return ES_NORMAL;
#endif /* MCMachine */

#ifdef /* MCMacToIso */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_MACTOISO_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	ep.grabsvalue();
	IO_mac_to_iso(ep.getbuffer(0), ep.getsvalue().getlength());
	return ES_NORMAL;
#endif /* MCMacToIso */

#ifdef /* MCMainStacks */ LEGACY_EXEC
	MCdispatcher->getmainstacknames(ep);
	return ES_NORMAL;
#endif /* MCMainStacks */

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
#ifdef /* MCMatch */ LEGACY_EXEC
	if (params->getnext()->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MATCH_BADPATTERN, line, pos);
		return ES_ERROR;
	}
    // JS-2013-06-21: [[ EnhancedFilter ]] refactored regex caching mechanism and case sentitivity
	// MW-2013-07-01: [[ EnhancedFilter ]] Use ep directly as MCR_compile copies pattern (if needed).
	// MW-2013-07-01: [[ EnhancedFilter ]] Removed 'usecache' parameter as there's
	//   no reason not to use the cache.
	regexp *compiled = MCR_compile(ep.getcstring(), True /* casesensitive */);

	if (compiled == NULL)
	{
        MCAutoStringRef t_error;
        MCR_geterror(&t_error);
		MCeerror->add(EE_MATCH_BADPATTERN, line, pos, *t_error);
		return ES_ERROR;
	}
	if (params->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MATCH_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	Boolean match;

	// MW-2008-06-16: If our string is NULL-style empty, then make sure we pass a non-NULL pointer
	//   else we get breakage.
	match = MCR_exec(compiled, ep.getsvalue().getstring(), ep.getsvalue().getlength());

	MCParameter *p = params->getnext()->getnext();
	uint2 i = 1;
	if (chunk)
	{
		while (p != NULL && p->getnext() != NULL)
		{
			// [[ Containers ]] Update to use evalcontainer so array keys can be
			//   passed.

			MCAutoPointer<MCContainer> t_container1;
			if (p -> evalcontainer(ep, &t_container1) != ES_NORMAL)
			{
				MCeerror -> add(EE_MATCH_BADDEST, line, pos);
				return ES_ERROR;
			}

			p = p->getnext();

			MCAutoPointer<MCContainer> t_container2;
			if (p -> evalcontainer(ep, &t_container2) != ES_NORMAL)
			{
				MCeerror -> add(EE_MATCH_BADDEST, line, pos);
				return ES_ERROR;
			}
		
			p = p->getnext();

			if (match && compiled->matchinfo[i].rm_so != -1)
			{
				char buffer[U4L];
				sprintf(buffer, "%d", (int)(compiled->matchinfo[i].rm_so + 1));
				/* UNCHECKED */ t_container1 -> set_cstring(buffer);
				sprintf(buffer, "%d", (int)(compiled->matchinfo[i].rm_eo));
				/* UNCHECKED */ t_container2 -> set_cstring(buffer);
			}
			else
			{
				/* UNCHECKED */ t_container1 -> clear();
				/* UNCHECKED */ t_container2 -> clear();
			}
			if (++i >= NSUBEXP)
				i = 0;
		}
	}
	else
	{
		while (p != NULL)
		{
			// [[ Containers ]] Update to use evalcontainer so array keys can be
			//   passed.
			MCVariable *var = p->evalvar(ep);
			p = p->getnext();
			if (var == NULL)
			{
				MCeerror->add(EE_MATCH_BADDEST, line, pos);
				return ES_ERROR;
			}
			if (match && compiled->matchinfo[i].rm_so != -1)
			{
				const char *sptr = ep.getsvalue().getstring();
				sptr += compiled->matchinfo[i].rm_so;
				uint4 length = compiled->matchinfo[i].rm_eo
				               - compiled->matchinfo[i].rm_so;
				MCString s(sptr, length);
				var->copysvalue(s);
			}
			else
				var->clear(False);
			if (++i >= NSUBEXP)
				i = 0;
		}
	}
	ep.setboolean(match);
	return ES_NORMAL;
#endif /* MCMatch */
    
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
    
    if (!ctxt.HasError())
    {
        for (uindex_t i = 0; i < t_result_count; i++)
        {
            // AL-2014-09-09: [[ Bug 13359 ]] Make sure containers are used in case a param is a handler variable
            // AL-2014-09-18: [[ Bug 13465 ]] Use auto class to prevent memory leak
            MCAutoPointer<MCContainer> t_container;
            if (!t_result_params->evalcontainer(ctxt, &t_container))
            {
                ctxt . LegacyThrow(EE_MATCH_BADDEST);
                return;
            }

            /* UNCHECKED */ t_container->set_valueref(t_results[i]);
            
            t_result_params = t_result_params->getnext();
        }
    }
}

void MCMatch::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	uindex_t t_count;
	t_count = 0;

	for (MCParameter *t_parameter = params; t_parameter != nil; t_parameter = t_parameter -> getnext())
	{
		if (t_count < 2)
			t_parameter -> compile(ctxt);
		else
			t_parameter -> compile_out(ctxt);
		t_count++;
	}
	
	MCSyntaxFactoryEvalList(ctxt, t_count - 2);

	if (chunk)
		MCSyntaxFactoryEvalMethod(ctxt, kMCStringsEvalMatchChunkMethodInfo);
	else
		MCSyntaxFactoryEvalMethod(ctxt, kMCStringsEvalMatchTextMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
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

	
#ifdef /* MCMe */ LEGACY_EXEC
	MCObject *target = ep.getobj();
	if (target->gettype() != CT_FIELD && target->gettype() != CT_BUTTON)
		return target->getprop(0, P_NAME, ep, False);
	return target->getprop(0, P_TEXT, ep, False);
#endif /* MCMe */


#ifdef /* MCMenuObject */ LEGACY_EXEC
	if (MCmenuobjectptr == NULL)
	{
		ep.clear();
		return ES_NORMAL;
	}
	return MCmenuobjectptr->getprop(0, P_NAME, ep, False);
#endif /* MCMenuObject */


#ifdef /* MCMenus */ LEGACY_EXEC
	ep.clear();  // hc compatibility
	return ES_NORMAL;
#endif /* MCMenus */

#ifdef /* MCMerge */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MERGE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	if (ep.getsvalue().getlength() == 0)
		return ES_NORMAL;
	const char *sptr, *eptr, *pend;
	const char *pstart = NULL;
	int4 rlength = 0;
	Boolean isexpression = False;
	MCExecPoint ep2(ep);
	char *tstring = ep.getsvalue().clone();
	sptr = tstring;
	eptr = tstring + ep.getsvalue().getlength();
	MCerrorlock++;
	do
	{
		Boolean match = False;
		if (*sptr == '<' && ++sptr < eptr && *sptr == '?')
		{
			pstart = sptr - 1;
			sptr++;
			while (sptr < eptr)
				if (*sptr == '?' && ++sptr < eptr && *sptr == '>')
				{
					match = True;
					isexpression = False;
					break;
				}
				else
					sptr++;
			if (!match)
				sptr = pstart + 2;//no end tags (stray ?>) jump back)
		}
		else
			if (*sptr == '[' && ++sptr < eptr && *sptr == '[')
			{
                // AL-2013-10-15 [[ Bug 11274 ]] Merge function should ignore square bracket if part of inner expression
                uint4 t_skip;
                t_skip = 0;
				pstart = sptr - 1;
				sptr++;
				while (sptr < eptr)
                {
                    if (*sptr == '[')
                        t_skip++;
					else if (*sptr == ']')
					{
                        if (t_skip > 0)
                            t_skip--;
                        else if (++sptr < eptr && *sptr == ']')
                        {
                            match = True;
                            isexpression = True;
                            break;
                        }
					}
                    sptr++;
                }
				if (!match)
					sptr = pstart + 2;//no end tags (stray ?>) jump back)
			}
		if (match)
		{
			pend = sptr + 1;
			uint4 si = pstart - tstring + rlength;
			uint4 ei = si + sptr + 1 - pstart;
			MCString s(pstart + 2, pend - pstart - 4);
			ep2.setsvalue(s);
			// MW-2013-11-15: [[ Bug 11277 ]] If ep has no handler, then execute in
			//   server script object context.
			if (isexpression)
			{
				Exec_stat t_stat;
                t_stat = ep . eval(ep2);
                
				if (t_stat != ES_ERROR)
				{
					ep.insert(ep2.getsvalue(), si, ei);
					rlength += ep2.getsvalue().getlength() - (pend - pstart);
				}
			}
			else
			{
                // SN-2015-06-03: [[ Bug 11277 ]] Refactor doscript (same for both
                //  MCHandler and MCHandlerlist)
				Exec_stat t_stat;
                t_stat = ep . doscript(ep2, line, pos);
                
				if (t_stat != ES_ERROR)
				{
					MCresult->fetch(ep2);
					ep.insert(ep2.getsvalue(), si, ei);
					rlength += ep2.getsvalue().getlength() - (pend - pstart);
					MCresult->clear(False);
				}
			}
		}
	}
	while (++sptr < eptr);
	MCerrorlock--;
	delete tstring;
	return ES_NORMAL;
#endif /* MCMerge */

#ifdef /* MCMillisecs */ LEGACY_EXEC
	ep.setnvalue(floor(MCS_time() * 1000.0));
	return ES_NORMAL;
#endif /* MCMillisecs */


#ifdef /* MCMonthNames */ LEGACY_EXEC
	MCD_monthnames(P_UNDEFINED, ep);
	return ES_NORMAL;
#endif /* MCMonthNames */


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
#ifdef /* MCMouse */ LEGACY_EXEC
	uint2 b = 0;
	if (which != NULL)
	{
		if (which->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_MOUSE_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		b = ep.getuint2();
	}
	Boolean t_abort;
	ep.setstaticcstring(MCU_ktos(MCscreen->getmouse(b, t_abort)));
	
	// MW-2008-03-17: [[ Bug 6098 ]] Make sure we check for an abort
	if (t_abort)
	{
		ep . clear();
		MCeerror -> add(EE_WAIT_ABORT, line, pos);
		return ES_ERROR;
	}
	
	return ES_NORMAL;
#endif /* MCMouse */

	uinteger_t b;
    if (!ctxt . EvalOptionalExprAsUInt(which, 0, EE_MOUSE_BADSOURCE, b))
        return;
	
	MCInterfaceEvalMouse(ctxt, b, r_value . nameref_value);
    r_value . type = kMCExecValueTypeNameRef;
}

void MCMouse::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	if (which != nil)
		which -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstantUInt(ctxt, 0);

	MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalMouseMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
}


#ifdef /* MCMouseChar */ LEGACY_EXEC
	ep.clear();
	if (MCmousestackptr != NULL)
	{
		MCControl *mfocused = MCmousestackptr->getcard()->getmfocused();
		if (mfocused != NULL && mfocused->gettype() == CT_FIELD)
		{
			MCField *fptr = (MCField *)mfocused;
			fptr->locchar(ep, False);
		}
	}
	return ES_NORMAL;
#endif /* MCMouseChar */


#ifdef /* MCMouseCharChunk */ LEGACY_EXEC
	ep.clear();
	if (MCmousestackptr != NULL)
	{
		MCControl *mfocused = MCmousestackptr->getcard()->getmfocused();
		if (mfocused != NULL && mfocused->gettype() == CT_FIELD)
		{
			MCField *fptr = (MCField *)mfocused;
			fptr->loccharchunk(ep, False);
		}
	}
	return ES_NORMAL;
#endif /* MCMouseCharChunk */


#ifdef /* MCMouseChunk */ LEGACY_EXEC
	ep.clear();
	if (MCmousestackptr != NULL)
	{
		MCControl *mfocused = MCmousestackptr->getcard()->getmfocused();
		if (mfocused != NULL && mfocused->gettype() == CT_FIELD)
		{
			MCField *fptr = (MCField *)mfocused;
			fptr->locchunk(ep, False);
		}
	}
	return ES_NORMAL;
#endif /* MCMouseChunk */


#ifdef /* MCMouseClick */ LEGACY_EXEC
	Boolean t_abort;
	ep.setboolean(MCscreen->getmouseclick(0, t_abort));
	
	// MW-2008-03-17: [[ Bug 6098 ]] Make sure we check for an abort
	if (t_abort)
	{
		ep . clear();
		MCeerror -> add(EE_WAIT_ABORT, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCMouseClick */


#ifdef /* MCMouseColor */ LEGACY_EXEC
	int2 mx, my;
	MCscreen->querymouse(mx, my);
	MCColor c;
	MCscreen->dropper(NULL, mx, my, &c);
	ep.setcolor(c);
	return ES_NORMAL;
#endif /* MCMouseColor */


#ifdef /* MCMouseControl */ LEGACY_EXEC
	ep.clear();
	if (MCmousestackptr != NULL)
	{
		MCControl *t_focused;
		t_focused = MCmousestackptr -> getcard() -> getmousecontrol();
		if (t_focused != NULL)
		{
			t_focused -> getprop(0, P_LAYER, ep, False);
			ep . insert("control ", 0, 0);
		}
	}
	return ES_NORMAL;
#endif /* MCMouseControl */


#ifdef /* MCMouseH */ LEGACY_EXEC
	int2 x, y;
	MCscreen->querymouse(x, y);

	// IM-2013-10-10: [[ FullscreenMode ]] Update to use stack coord conversion methods
	MCPoint t_mouseloc;
	t_mouseloc = MCdefaultstackptr->globaltostackloc(MCPointMake(x, y));
	
	ep.setint(t_mouseloc.x);
	
	return ES_NORMAL;
#endif /* MCMouseH */


#ifdef /* MCMouseLine */ LEGACY_EXEC
	ep.clear();
	if (MCmousestackptr != NULL)
	{
		MCControl *mfocused = MCmousestackptr->getcard()->getmfocused();
		if (mfocused != NULL && mfocused->gettype() == CT_FIELD)
		{
			MCField *fptr = (MCField *)mfocused;
			fptr->locline(ep, False);
		}
	}
	return ES_NORMAL;
#endif /* MCMouseLine */


#ifdef /* MCMouseLoc */ LEGACY_EXEC
	int2 x, y;
	MCscreen->querymouse(x, y);
	
	// IM-2013-10-09: [[ FullscreenMode ]] Update to use stack coord conversion methods
	MCPoint t_mouseloc;
	t_mouseloc = MCdefaultstackptr->globaltostackloc(MCPointMake(x, y));

	ep.setpoint(t_mouseloc.x, t_mouseloc.y);
	
	return ES_NORMAL;
#endif /* MCMouseLoc */


#ifdef /* MCMouseStack */ LEGACY_EXEC
	if (MCmousestackptr == NULL)
	{
		ep.clear();
		return ES_NORMAL;
	}
	return MCmousestackptr->getprop(0, P_SHORT_NAME, ep, False);
#endif /* MCMouseStack */


#ifdef /* MCMouseText */ LEGACY_EXEC
	ep.clear();
	if (MCmousestackptr != NULL)
	{
		MCControl *mfocused = MCmousestackptr->getcard()->getmfocused();
		if (mfocused != NULL && mfocused->gettype() == CT_FIELD)
		{
			MCField *fptr = (MCField *)mfocused;
			fptr->loctext(ep, False);
		}
	}
	return ES_NORMAL;
#endif /* MCMouseText */


#ifdef /* MCMouseV */ LEGACY_EXEC
	int2 x, y;
	MCscreen->querymouse(x, y);
	
	// IM-2013-10-09: [[ FullscreenMode ]] Update to use stack coord conversion methods
	MCPoint t_mouseloc;
	t_mouseloc = MCdefaultstackptr->globaltostackloc(MCPointMake(x, y));
	
	ep.setint(t_mouseloc.y);
	
	return ES_NORMAL;
#endif /* MCMouseV */


#ifdef /* MCMovie */ LEGACY_EXEC
#ifdef X11
	IO_cleanprocesses();
#else

	real8 ctime = MCS_time();
	real8 etime = ctime;
	MCscreen->handlepending(ctime, etime, True);
#endif

	Boolean done = False;
	if (MCplayers != NULL)
	{
		ep.clear();
		MCExecPoint ep2(ep);
		MCPlayer *tptr = MCplayers;
		while (tptr != NULL)
		{
			if (tptr->isdisposable())
			{
				tptr->getprop(0, P_NAME, ep2, False);
				ep.concatmcstring(ep2.getsvalue(), EC_RETURN, tptr == MCplayers);
				done = True;
			}
			tptr = tptr->getnextplayer();
		}
	}
	if (!done)
		ep.setstaticcstring(MCdonestring);
	return ES_NORMAL;
#endif /* MCMovie */


#ifdef /* MCMovingControls */ LEGACY_EXEC
	MCscreen->listmoves(ep);
	return ES_NORMAL;
#endif /* MCMovingControls */


#ifdef /* MCNumToChar */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_NUMTOCHAR_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	if (ep.getuseunicode())
	{
		uint2 d = (uint2)ep.getint4();
		ep.copysvalue((char *)&d, 2);
	}
	else
	{
		char d = (char)ep.getint4();
		ep.copysvalue(&d, 1);
	}
	return ES_NORMAL;
#endif /* MCNumToChar */


#ifdef /* MCNumToByte */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_NUMTOBYTE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	uint1 d;
	d = (uint1)ep.getuint4();
	ep . copysvalue((const char *)&d, 1);
	return ES_NORMAL;
#endif /* MCNumToByte */


#ifdef /* MCOpenFiles */ LEGACY_EXEC
	ep.clear();
	for(uint2 i = 0 ; i < MCnfiles ; i++)
		ep.concatcstring(MCfiles[i] . name, EC_RETURN, i == 0);
	return ES_NORMAL;
#endif /* MCOpenFiles */


#ifdef /* MCOpenProcesses */ LEGACY_EXEC
	IO_cleanprocesses();
	ep.clear();
	for(uint2 i = 0 ; i < MCnprocesses ; i++)
		if (MCprocesses[i].mode != OM_VCLIP)
			ep.concatcstring(MCprocesses[i].name, EC_RETURN, i == 0);
	return ES_NORMAL;
#endif /* MCOpenProcesses */


#ifdef /* MCOpenProcessIds */ LEGACY_EXEC
	IO_cleanprocesses();
	ep.clear();
	for(uint2 i = 0 ; i < MCnprocesses ; i++)
		ep.concatuint(MCprocesses[i] . pid, EC_RETURN, i == 0);
	return ES_NORMAL;
#endif /* MCOpenProcessIds */


#ifdef /* MCOpenSockets */ LEGACY_EXEC
	IO_cleansockets(MCS_time());
	ep.clear();
	uint2 j = 0;
	for(uint2 i = 0 ; i < MCnsockets; i++)
		if (!MCsockets[i]->closing)
			ep.concatcstring(MCsockets[i] -> name, EC_RETURN, j++ == 0);
	return ES_NORMAL;
#endif /* MCOpenSockets */


#ifdef /* MCOpenStacks */ LEGACY_EXEC
	MCstacks->stackprops(ep, P_SHORT_NAME);
	return ES_NORMAL;
#endif /* MCOpenStacks */


#ifdef /* MCOptionKey */ LEGACY_EXEC
	ep.setstaticcstring(MCU_ktos((MCscreen->querymods() & MS_MOD1) != 0));
	return ES_NORMAL;
#endif /* MCOptionKey */

#ifdef /* MCParam */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_PARAM_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	// MW-2013-11-15: [[ Bug 11277 ]] If we don't have a handler then 'the param'
	//   makes no sense so just return empty.
	if (ep.gethandler() != nil)
	{
		if (ep . gethandler()->getparam(ep.getuint2(), ep) != ES_NORMAL)
		{
			MCeerror->add(EE_PARAM_BADINDEX, line, pos, ep.getsvalue());
			return ES_ERROR;
		}
	}
	else
	{
		ep.clear();
	}
	return ES_NORMAL;
#endif /* MCParam */

Parse_stat MCParamCount::parse(MCScriptPoint &sp, Boolean the)
{
	return MCFunction::parse(sp, the);
}

#ifdef /* MCParamCount */ LEGACY_EXEC
	// MW-2013-11-15: [[ Bug 11277 ]] If we don't have a handler then 'the param'
	//   makes no sense so just return 0.
	if (ep.gethandler() != nil)
	{
		uint2 count;
		ep.gethandler()->getnparams(count);
		ep.setnvalue(count);
	}
	else
	{
		ep.setnvalue(0);
	}

	return ES_NORMAL;
#endif /* MCParamCount */


Parse_stat MCParams::parse(MCScriptPoint &sp, Boolean the)
{
	return MCFunction::parse(sp, the);
}

#ifdef /* MCParams */ LEGACY_EXEC
	// MW-2013-11-15: [[ Bug 11277 ]] If we don't have a handler then 'the param'
	//   makes no sense so just return empty.
	if (ep.gethandler() == nil)
	{
		ep . clear();
		return ES_NORMAL;
	}
	
	MCHandler *h;
	h = ep.gethandler();
	
	ep . setnameref_unsafe(h -> getname());
	ep . appendchar(h -> gettype() == HT_FUNCTION ? '(' : ' ');

	MCExecPoint ep2(ep);
	uint2 count;
	h->getnparams(count);
	for(uint2 i = 1 ; i <= count ; i++)
	{
		h->getparam(i, ep2);
		ep . appendchar('"');
		ep . appendmcstring(ep2 . getsvalue());
		ep . appendchar('"');
		if (i < count)
			ep . appendchar(',');
	}
	if (h->gettype() == HT_FUNCTION)
		ep . appendchar(')');
	return ES_NORMAL;
#endif /* MCParams */

#ifdef /* MCPeerAddress */ LEGACY_EXEC
	if (socket->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_PEERADDRESS_BADSOCKET, line, pos);
		return ES_ERROR;
	}
	char *name = ep.getsvalue().clone();
	uint2 index;
	if (IO_findsocket(name, index))
		MCS_pa(ep, MCsockets[index]);
	else
		ep.setstaticcstring("not an open socket");
	delete name;
	return ES_NORMAL;
#endif /* MCPeerAddress */

#ifdef /* MCPendingMessages */ LEGACY_EXEC
	MCscreen->listmessages(ep);
	return ES_NORMAL;
#endif /* MCPendingMessages */


#ifdef /* MCPid */ LEGACY_EXEC
	ep.setnvalue(MCS_getpid());
	return ES_NORMAL;
#endif /* MCPid */


#ifdef /* MCPlatform */ LEGACY_EXEC
	ep.setstaticcstring(MCplatformstring);
	return ES_NORMAL;
#endif /* MCPlatform */


#ifdef /* MCProcessor */ LEGACY_EXEC
	ep.setstaticcstring(MCS_getprocessor());
	return ES_NORMAL;
#endif /* MCProcessor */


#ifdef /* MCPropertyNames */ LEGACY_EXEC
	ep.clear();
	MCScriptPoint sp(ep);
	return sp.getfactors(ep, TT_PROPERTY);
#endif /* MCPropertyNames */


#ifdef /* MCQTVersion */ LEGACY_EXEC
	MCtemplateplayer->getversion(ep);
	return ES_NORMAL;
#endif /* MCQTVersion */


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

static void *realloc_range(void *p_block, unsigned int p_minimum, unsigned int p_maximum, unsigned int& p_limit)
{
	void *p_result;
	unsigned int p_size;
	p_minimum = (p_minimum + 3) & ~3;
	p_maximum = (p_maximum + 3) & ~3;
	do
	{
		p_size = p_maximum;
		p_result = realloc(p_block, p_maximum);
		p_maximum = (p_maximum - p_minimum) >> 1;
	}
	while(p_result == NULL && p_maximum > p_minimum);
	p_limit = p_size;
	if (p_result == NULL)
		free(p_block);
	return p_result;
}

void MCReplaceText::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCReplaceText */ LEGACY_EXEC
	if (replacement->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_REPLACETEXT_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	char *rstring = ep.getsvalue().clone();
	MCString s(rstring, strlen(rstring));

	if (pattern->eval(ep) != ES_NORMAL)
	{
		delete rstring;
		MCeerror->add(EE_REPLACETEXT_BADPATTERN, line, pos);
		return ES_ERROR;
	}
	if (ep.getsvalue().getlength() == 0)
	{
		delete rstring;
		return ES_NORMAL;
	}
    const char *pattern = NULL;
    // JS-2013-06-21: [[ EnhancedFilter ]] refactored regex caching mechanism and case sentitivity
	// MW-2013-07-01: [[ EnhancedFilter ]] Use ep directly since MCR_compile copies pattern string (if needed).
	// MW-2013-07-01: [[ EnhancedFilter ]] Removed 'usecache' parameter as there's
	//   no reason not to use the cache.
	regexp *compiled = MCR_compile(ep.getcstring(), True /*casesensitive*/);
    if (compiled != NULL)
        pattern = compiled->pattern;
	if (compiled == NULL)
	{
		delete rstring;
        MCAutoStringRef t_error;
        MCR_geterror(&t_error);
		MCeerror->add(EE_REPLACETEXT_BADPATTERN, line, pos, *t_error);
		return ES_ERROR;
	}
	if (source->eval(ep) != ES_NORMAL)
	{
		delete rstring;
		MCeerror->add(EE_REPLACETEXT_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	if (ep.getsvalue().getlength() != 0)
	{
		// MW-2005-05-17: We can do *considerably* better than the old implementation
		const char *t_source = ep . getsvalue() . getstring();
		uint4 t_source_length = ep . getsvalue() . getlength();
		uint4 t_source_offset = 0;
		char *t_target = NULL;
		uint4 t_target_length = 0, t_target_limit = 0;

		// While not at the end of the source string, and the regular expression matches something between the source offset and the end of the source string.
		while(t_source_offset < t_source_length && MCR_exec(compiled, &t_source[t_source_offset], t_source_length - t_source_offset))
		{
			uint4 t_start, t_end;
			t_start = compiled -> matchinfo[0] . rm_so + t_source_offset;
			t_end = compiled -> matchinfo[0] . rm_eo + t_source_offset;

			// OK-2008-12-04: [[Bug 7200]] - Don't keep searching if no matches were found
			if (t_start == t_end)
				break;

			// OK-2010-01-12: [[Bug 8264]] - Crash when replacing single match at end of string with longer replacement string.
			// Fixed by ensuring that initial allocation is source length + difference between replacement and match strings.
			if (t_start - t_source_offset + s . getlength() > t_target_limit - t_target_length)
				t_target = (char *)realloc_range(t_target, t_target_length + t_start - t_source_offset + s . getlength(), MCU_max((t_target_limit == 0 ? 256 : t_target_limit) * 2, t_source_length + (s . getlength() - (t_end - t_start))), t_target_limit);

			// Copy the bit before the found match into the target buffer
			memcpy(t_target + t_target_length, t_source + t_source_offset, t_start - t_source_offset);
			t_target_length += t_start - t_source_offset;

			// Copy the replacement string into the target buffer where the found match was
			memcpy(t_target + t_target_length, s . getstring(), s . getlength());
			t_target_length += s . getlength();

			// Begin searching again after the end of the match
			t_source_offset = t_end;

			if (pattern[0] == '^')
				break;
		}
		
		// MW-2005-06-08: Always execute this, otherwise we get errors in the replacement
		// This is copying the stuff after the last match into the target buffer.
		t_target = (char *)realloc(t_target, t_target_length + t_source_length - t_source_offset);
		memcpy(t_target + t_target_length, t_source + t_source_offset, t_source_length - t_source_offset);
		t_target_length += t_source_length - t_source_offset;
		ep . grabbuffer(t_target, t_target_length);
	}
	delete rstring;
	return ES_NORMAL;
#endif /* MCReplaceText */

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

void MCReplaceText::compile(MCSyntaxFactoryRef ctxt)
{
	compile_with_args(ctxt, kMCStringsEvalReplaceTextMethodInfo, source, pattern, replacement);
}

// MW-2010-12-15: [[ Bug ]] Make sure the value of 'the result' is grabbed, otherwise
//   if it is modified by a function in an expression and used directly in that
//   expression, bogus things can happen. i.e.
//      the result = func_modifying_result()

#ifdef /* MCTheResult */ LEGACY_EXEC
	if (MCresult->fetch(ep) != ES_NORMAL)
		return ES_ERROR;
	ep.grab();
	return ES_NORMAL;
#endif /* MCTheResult */


#ifdef /* MCScreenColors */ LEGACY_EXEC
	ep.setnvalue(pow(2.0, MCscreen->getdepth()));
	return ES_NORMAL;
#endif /* MCScreenColors */


#ifdef /* MCScreenDepth */ LEGACY_EXEC
	ep.setnvalue(MCscreen->getdepth());
	return ES_NORMAL;
#endif /* MCScreenDepth */


#ifdef /* MCScreenLoc */ LEGACY_EXEC
	MCDisplay const *t_displays;
	MCscreen -> getdisplays(t_displays, false);
	MCRectangle t_viewport = t_displays -> viewport;
	ep.setpoint(t_viewport . x + (t_viewport . width / 2), t_viewport . y + (t_viewport . height / 2));
	return ES_NORMAL;
#endif /* MCScreenLoc */


#ifdef /* MCScreenName */ LEGACY_EXEC
	ep.setstaticcstring(MCscreen->getdisplayname());
	return ES_NORMAL;
#endif /* MCScreenName */


void MCScreenRect::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCScreenRect */ LEGACY_EXEC
	evaluate(ep, false, f_plural, false);
	return ES_NORMAL;
#endif /* MCScreenRect */

    MCInterfaceEvalScreenRect(ctxt, false, f_plural, false, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

void MCScreenRect::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	MCSyntaxFactoryEvalConstantBool(ctxt, false);
	MCSyntaxFactoryEvalConstantBool(ctxt, f_plural);
	MCSyntaxFactoryEvalConstantBool(ctxt, false);

	MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalScreenRectMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
}

#ifdef /* MCScreenRect::evaluate */ LEGACY_EXEC
void MCScreenRect::evaluate(MCExecPoint& ep, bool p_working, bool p_plural, bool p_effective)
{
	const MCDisplay *t_displays;
	uint4 t_count;
	
	t_count = MCscreen -> getdisplays(t_displays, p_effective);
	ep . clear();
	if (!p_plural)
		t_count = 1;
	
	for(uint4 t_index = 0; t_index < t_count; ++t_index)
	{
		char t_buffer[U2L * 4 + 4];
		MCRectangle t_rectangle;
		t_rectangle = p_working ? t_displays[t_index] . workarea : t_displays[t_index] . viewport;
		sprintf(t_buffer, "%d,%d,%d,%d", t_rectangle . x, t_rectangle . y,
				t_rectangle . x + t_rectangle . width,
				t_rectangle . y + t_rectangle . height);
		ep.concatcstring(t_buffer, EC_RETURN, t_index == 0);
	}
}
#endif /* MCScreenRect::evaluate */


#ifdef /* MCScreenType */ LEGACY_EXEC
	switch (MCscreen->getvclass())
	{
	case StaticGray:
		ep.setstaticcstring("StaticGray");
		break;
	case GrayScale:
		ep.setstaticcstring("GrayScale");
		break;
	case StaticColor:
		ep.setstaticcstring("StaticColor");
		break;
	case PseudoColor:
		ep.setstaticcstring("PseudoColor");
		break;
	case TrueColor:
		ep.setstaticcstring("TrueColor");
		break;
	case DirectColor:
		ep.setstaticcstring("DirectColor");
		break;
	}
	return ES_NORMAL;
#endif /* MCScreenType */


#ifdef /* MCScreenVendor */ LEGACY_EXEC
	MCscreen->getvendorstring(ep);
	return ES_NORMAL;
#endif /* MCScreenVendor */


#ifdef /* MCScriptLimits */ LEGACY_EXEC
	ep.setstringf("%d,%d,%d,%d", MClicenseparameters . script_limit, MClicenseparameters . do_limit, MClicenseparameters . using_limit, MClicenseparameters . insert_limit);
	return ES_NORMAL;
#endif /* MCScriptLimits */


#ifdef /* MCSeconds */ LEGACY_EXEC
	ep.setnvalue(floor(MCS_time()));
	return ES_NORMAL;
#endif /* MCSeconds */


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
		object = new MCChunk(False);
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
#ifdef /* MCSelectedButton */ LEGACY_EXEC
	if (family->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_SELECTEDBUTTON_BADFAMILY, line, pos);
		return ES_ERROR;
	}
	uint4 parid = 0;
	MCCard *cptr;
	if (object != NULL)
	{
		MCObject *optr;
		if (object->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_SELECTEDBUTTON_BADPARENT, line, pos);
			return ES_ERROR;
		}
		switch (optr->gettype())
		{
		case CT_CARD:
			cptr = (MCCard *)optr;
			break;
		case CT_STACK:
			{
				MCStack *sptr = (MCStack *)optr;
				cptr = sptr->getchild(CT_THIS, MCnullmcstring, CT_CARD);
			}
			break;
		default:
			MCeerror->add(EE_SELECTEDBUTTON_BADPARENT, line, pos);
			return ES_ERROR;
		}
	}
	else
		cptr = MCdefaultstackptr->getchild(CT_THIS, MCnullmcstring, CT_CARD);
	cptr->selectedbutton(ep.getuint2(), bg, ep);
	return ES_NORMAL;
#endif /* MCSelectedButton */

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

void MCSelectedButton::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	MCSyntaxFactoryEvalConstantBool(ctxt, bg == True);
	family -> compile(ctxt);

	if (object != nil)
	{
		object -> compile_object_ptr(ctxt);
		MCSyntaxFactoryEvalMethod(ctxt, kMCLegacyEvalSelectedButtonOfMethodInfo);
	}
	else
		MCSyntaxFactoryEvalMethod(ctxt, kMCLegacyEvalSelectedButtonMethodInfo);	

	MCSyntaxFactoryEndExpression(ctxt);
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
#ifdef /* MCSelectedChunk */ LEGACY_EXEC
	if (object != NULL)
	{
		MCObject *optr;
		uint4 parid;
		if (object->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_SELECTED_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		switch (optr->gettype())
		{
		case CT_FIELD:
			{
				MCField *fptr = (MCField *)optr;
				fptr->selectedchunk(ep);
			}
			break;
		case CT_BUTTON:
			{
				MCButton *bptr = (MCButton *)optr;
				bptr->selectedchunk(ep);
			}
			break;
		default:
			MCeerror->add(EE_SELECTED_BADSOURCE, line, pos);
			return ES_ERROR;
		}
	}
	else if (MCactivefield == NULL)
		ep.clear();
	else
	{
		// MW-2013-08-07: [[ Bug 10689 ]] If the parent of the field is a button
		//   then return the chunk of the button, not the embedded field.
		if (MCactivefield -> getparent() -> gettype() == CT_BUTTON)
			static_cast<MCButton *>(MCactivefield -> getparent()) -> selectedchunk(ep);
		else
			MCactivefield->selectedchunk(ep);
	}
	return ES_NORMAL;
#endif /* MCSelectedChunk */

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

void MCSelectedChunk::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	if (object != nil)
	{
		object -> compile_object_ptr(ctxt);
		MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalSelectedChunkOfMethodInfo);
	}
	else
		MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalSelectedChunkMethodInfo);	

	MCSyntaxFactoryEndExpression(ctxt);
}


#ifdef /* MCSelectedField */ LEGACY_EXEC
	if (MCactivefield != NULL)
	{
		MCactivefield->getprop(0, P_NUMBER, ep, False);
		ep.setstringf("field %d", ep.getuint4());
	}
	else
		ep.clear();
	return ES_NORMAL;
#endif /* MCSelectedField */


#ifdef /* MCSelectedImage */ LEGACY_EXEC
	if (MCactiveimage != NULL)
	{
		MCactiveimage->getprop(0, P_NUMBER, ep, False);
		ep.setstringf("image %d", ep.getuint4());

	}
	else
		ep.clear();
	return ES_NORMAL;
#endif /* MCSelectedImage */


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
#ifdef /* MCSelectedLine */ LEGACY_EXEC
	if (object != NULL)
	{
		MCObject *optr;
		uint4 parid;
		if (object->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_SELECTED_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		switch (optr->gettype())
		{
		case CT_FIELD:
			{
				MCField *fptr = (MCField *)optr;
				fptr->selectedline(ep);
			}
			break;
		case CT_BUTTON:
			{
				MCButton *bptr = (MCButton *)optr;
				bptr->selectedline(ep);
			}
			break;
		default:
			MCeerror->add(EE_SELECTED_BADSOURCE, line, pos);
			return ES_ERROR;
		}
	}
	else
		if (MCactivefield == NULL)
			ep.clear();
		else
			MCactivefield->selectedline(ep);
	return ES_NORMAL;
#endif /* MCSelectedLine */

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

void MCSelectedLine::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	if (object != nil)
	{
		object -> compile_object_ptr(ctxt);
		MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalSelectedLineOfMethodInfo);
	}
	else
		MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalSelectedLineMethodInfo);	

	MCSyntaxFactoryEndExpression(ctxt);
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
#ifdef /* MCSelectedLoc */ LEGACY_EXEC
	if (object != NULL)
	{
		MCObject *optr;
		uint4 parid;
		if (object->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_SELECTED_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		switch (optr->gettype())
		{
		case CT_FIELD:
			{
				MCField *fptr = (MCField *)optr;
				fptr->selectedloc(ep);
			}
			break;
		case CT_BUTTON:
			ep.clear();
			break;
		default:
			MCeerror->add(EE_SELECTED_BADSOURCE, line, pos);
			return ES_ERROR;
		}
	}
	else
		if (MCactivefield == NULL)
			ep.clear();
		else
			MCactivefield->selectedloc(ep);
	return ES_NORMAL;
#endif /* MCSelectedLoc */
    
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

void MCSelectedLoc::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
    
	if (object != nil)
	{
		object -> compile_object_ptr(ctxt);
		MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalSelectedLocOfMethodInfo);
	}
	else
		MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalSelectedLocMethodInfo);
    
	MCSyntaxFactoryEndExpression(ctxt);
}


#ifdef /* MCSelectedObject */ LEGACY_EXEC
	MCselected->getids(ep);
	return ES_NORMAL;
#endif /* MCSelectedObject */


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
#ifdef /* MCSelectedText */ LEGACY_EXEC
	if (object != NULL)
	{
		MCObject *optr;
		uint4 parid;
		if (object->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_SELECTED_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		switch (optr->gettype())
		{
		case CT_FIELD:
			{
				MCField *fptr = (MCField *)optr;
				fptr->selectedtext(ep);
			}
			break;
		case CT_BUTTON:
			{
				MCButton *bptr = (MCButton *)optr;
				bptr->selectedtext(ep);
			}
			break;
		default:
			MCeerror->add(EE_SELECTED_BADSOURCE, line, pos);
			return ES_ERROR;
		}
	}
	else
		if (MCactivefield == NULL)
			ep.clear();
		else
			MCactivefield->selectedtext(ep);
	return ES_NORMAL;
#endif /* MCSelectedText */

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

void MCSelectedText::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	if (object != nil)
	{
		object -> compile_object_ptr(ctxt);
		MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalSelectedTextOfMethodInfo);
	}
	else
		MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalSelectedTextMethodInfo);	

	MCSyntaxFactoryEndExpression(ctxt);
}


#ifdef /* MCShell */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_SHELL_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	if (MCsecuremode & MC_SECUREMODE_PROCESS)
	{
		MCeerror->add(EE_SHELL_NOPERM, line, pos);
		return ES_ERROR;
	}
	if (MCS_runcmd(ep) != IO_NORMAL)
	{
		MCeerror->add(EE_SHELL_BADCOMMAND, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCShell */

#ifdef /* MCShiftKey */ LEGACY_EXEC
	ep.setstaticcstring(MCU_ktos((MCscreen->querymods() & MS_SHIFT) != 0));
	return ES_NORMAL;
#endif /* MCShiftKey */


#ifdef /* MCSound */ LEGACY_EXEC
#ifdef _MOBILE
	extern bool MCSystemGetPlayingSound(const char *& r_sound);
	const char *t_sound;
	if (MCSystemGetPlayingSound(t_sound))
	{
		if (t_sound != nil)
			ep . copysvalue(t_sound);
		else
			ep . setsvalue(MCdonestring);
		return ES_NORMAL;
	}
#endif

// Keep old behaviour if FEATURE_PLATFORM_AUDIO is not defined
#ifndef FEATURE_PLATFORM_AUDIO
    MCU_play();
#endif
	if (MCacptr != NULL && MCacptr -> isPlaying())
		return MCacptr->getprop(0, P_NAME, ep, False);
	ep.setstaticcstring(MCdonestring);
	return ES_NORMAL;
#endif /* MCSound */


#ifdef /* MCStacks */ LEGACY_EXEC
	MCstacks->stackprops(ep, P_FILE_NAME);
	return ES_NORMAL;
#endif /* MCStacks */


#ifdef /* MCStackSpace */ LEGACY_EXEC
	ep.setstaticcstring(STACK_SPACE);
	return ES_NORMAL;
#endif /* MCStackSpace */


#ifdef /* MCSysError */ LEGACY_EXEC
	ep.setnvalue(MCS_getsyserror());
	return ES_NORMAL;
#endif /* MCSysError */


#ifdef /* MCSystemVersion */ LEGACY_EXEC
	ep.setstaticcstring(MCS_getsystemversion());
	return ES_NORMAL;
#endif /* MCSystemVersion */


Parse_stat MCTarget::parse(MCScriptPoint &sp, Boolean the)
{
	contents = False;
	if (!the)
		if (sp.skip_token(SP_FACTOR, TT_LPAREN) == PS_NORMAL)
		{
			if (sp.skip_token(SP_FACTOR, TT_RPAREN) != PS_NORMAL)
			{
				MCperror->add(PE_FACTOR_NORPAREN, sp);
				return PS_ERROR;
			}
		}
		else
			contents = True;
	initpoint(sp);
	return PS_NORMAL;
}

void MCTarget::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCTarget */ LEGACY_EXEC
	if (MCtargetptr == NULL)
	{
		ep.clear();
		return ES_NORMAL;
	}
	if (!contents || MCtargetptr->gettype() != CT_FIELD)
		return MCtargetptr->getprop(0, P_NAME, ep, False);
	return MCtargetptr->getprop(0, P_TEXT, ep, False);
#endif /* MCTarget */

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
#ifdef /* MCOwner */ LEGACY_EXEC
	MCObject *t_objptr;
	uint4 t_part;
	if (object -> getobj(ep, t_objptr, t_part, True) != ES_NORMAL)
	{
		return ES_ERROR;
	}
	return t_objptr -> getprop(0, P_OWNER, ep, False);
#endif /* MCOwner */

	MCObjectPtr t_objptr;
	if (!object -> getobj(ctxt, t_objptr, True))
		return;

    MCEngineEvalOwner(ctxt, t_objptr, r_value .stringref_value);
    r_value .type = kMCExecValueTypeStringRef;
}

void MCOwner::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	object -> compile_object_ptr(ctxt);

	MCSyntaxFactoryEvalMethod(ctxt, kMCEngineEvalOwnerMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
}

#ifdef /* MCTempName */ LEGACY_EXEC
	ep.setcstring(MCS_tmpnam());
	return ES_NORMAL;
#endif /* MCTempName */

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

void MCTextDecode::compile(MCSyntaxFactoryRef ctxt)
{
    compile_with_args(ctxt, kMCStringsEvalTextDecodeMethodInfo, m_data, m_encoding);
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

void MCTextEncode::compile(MCSyntaxFactoryRef ctxt)
{
    compile_with_args(ctxt, kMCStringsEvalTextEncodeMethodInfo, m_string, m_encoding);
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

void MCNormalizeText::compile(MCSyntaxFactoryRef ctxt)
{
    compile_with_args(ctxt, kMCStringsEvalNormalizeTextMethodInfo, m_text, m_form);
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

void MCCodepointProperty::compile(MCSyntaxFactoryRef ctxt)
{
    compile_with_args(ctxt, kMCStringsEvalCodepointPropertyMethodInfo, m_codepoint, m_property);
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
#ifdef /* MCTextHeightSum */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;
	if (object->getobj(ep, optr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_TEXT_HEIGHT_SUM_NOOBJECT, line, pos);
		return ES_ERROR;
	}
	return optr->getprop(0, P_FORMATTED_HEIGHT, ep, False);
#endif /* MCTextHeightSum */

	integer_t t_result;

	MCObjectPtr t_object;
	if (!object->getobj(ctxt, t_object, True))
	{
		ctxt . LegacyThrow(EE_TEXT_HEIGHT_SUM_NOOBJECT);
		return;
	}

	MCLegacyEvalTextHeightSum(ctxt, t_object, r_value . int_value);
    r_value . type = kMCExecValueTypeInt;
}

void MCTextHeightSum::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	object -> compile_object_ptr(ctxt);

	MCSyntaxFactoryEvalMethod(ctxt, kMCLegacyEvalTextHeightSumMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
}


#ifdef /* MCTicks */ LEGACY_EXEC
	ep.setnvalue(floor(MCS_time() * 60.0));
	return ES_NORMAL;
#endif /* MCTicks */


#ifdef /* MCTheTime */ LEGACY_EXEC
	MCD_time(P_UNDEFINED, ep);
	return ES_NORMAL;
#endif /* MCTheTime */

#ifdef /* MCToLower */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_TOLOWER_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	ep.lower();
	return ES_NORMAL;
#endif /* MCToLower */

#ifdef /* MCToUpper */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_TOUPPER_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	ep.upper();
	return ES_NORMAL;
#endif /* MCToUpper */

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
#ifdef /* MCTopStack */ LEGACY_EXEC
	MCStack *sptr = MCtopstackptr;
	if (which != NULL)
	{
		if (which->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_TOPSTACK_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		sptr = MCstacks->getstack(ep.getuint2());
	}
	if (sptr != NULL)
		return sptr->getprop(0, P_LONG_NAME, ep, False);
	else
	{
		ep.clear();
		return ES_NORMAL;
	}
#endif /* MCTopStack */

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

void MCTopStack::compile(MCSyntaxFactoryRef ctxt)
{
	if (which != nil)
		compile_with_args(ctxt, kMCInterfaceEvalTopStackOfMethodInfo, which);
	else
		compile_with_args(ctxt, kMCInterfaceEvalTopStackMethodInfo);
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
#ifdef /* MCUniDecode */ LEGACY_EXEC
	uint1 destcharset = 0;
	if (language)
	{
		if (language->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_UNIDECODE_BADLANGUAGE, line, pos);
			return ES_ERROR;
		}
		char *langname = ep.getsvalue().clone();
		destcharset = MCU_languagetocharset(langname);
		delete langname;
	}
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_UNIDECODE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	char *startptr = ep.getsvalue().clone();
	uint4 length = ep.getsvalue().getlength();
	uint4 dlength = 0;
	MCU_unicodetomultibyte(startptr,length, NULL, 0, dlength, destcharset);
	dlength = MCU_max(length << 1, dlength);
	char *dptr = ep.getbuffer(dlength);
	MCU_unicodetomultibyte(startptr, length, dptr, dlength, dlength, destcharset);
	ep.setsvalue(MCString(dptr,dlength));
	delete startptr;
	return ES_NORMAL;
#endif /* MCUniDecode */

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

void MCUniDecode::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	source -> compile(ctxt);

	if (language)
		language -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstant(ctxt, kMCEmptyName);

	MCSyntaxFactoryEvalMethod(ctxt, kMCFiltersEvalUniDecodeMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
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
#ifdef /* MCUniEncode */ LEGACY_EXEC
	uint1 srccharset = 0;
	if (language)
	{
		if (language->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_UNIENCODE_BADLANGUAGE, line, pos);
			return ES_ERROR;
		}
		char *langname = ep.getsvalue().clone();
		srccharset = MCU_languagetocharset(langname);
		delete langname;
	}
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_UNIENCODE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	char *startptr = ep.getsvalue().clone();
	uint4 length = ep.getsvalue().getlength();
	uint4 dlength;
	MCU_multibytetounicode(startptr, length, NULL, 0, dlength, srccharset);
	char *dptr = ep.getbuffer(length << 1);
	MCString s(dptr, dlength);
	MCU_multibytetounicode(startptr, length, dptr, dlength, dlength, srccharset);
	s.setlength(dlength);
	ep.setsvalue(s);
	delete startptr;
	return ES_NORMAL;
#endif /* MCUniEncode */

	
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

void MCUniEncode::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	source -> compile(ctxt);

	if (language)
		language -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstant(ctxt, kMCEmptyName);

	MCSyntaxFactoryEvalMethod(ctxt, kMCFiltersEvalUniEncodeMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
}

#ifdef /* MCUrlDecode */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_URLDECODE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCU_urldecode(ep);
	return ES_NORMAL;
#endif /* MCUrlDecode */

#ifdef /* MCUrlEncode */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_URLENCODE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCU_urlencode(ep);
	return ES_NORMAL;
#endif /* MCUrlEncode */

#ifdef /* MCUrlStatus */ LEGACY_EXEC
	if (url->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_URLSTATUS_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCParameter p1(ep.getsvalue());
	ep.getobj()->message(MCM_get_url_status, &p1, False, True);
	MCresult->fetch(ep);
	return ES_NORMAL;
#endif /* MCUrlStatus */

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
			object = new MCChunk(False);
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
#ifdef /* MCValue */ LEGACY_EXEC
	if (source == NULL)
	{
		ep.clear();
		return ES_NORMAL;
	}
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_VALUE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	if (ep.getsvalue().getlength() == 0)
	{
		ep.clear();
		return ES_NORMAL;
	}
	if (object != NULL)
	{
		char *tptr = ep.getsvalue().clone();
		MCObject *optr;
		uint4 parid;
		if (object->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_VALUE_NOOBJ, line, pos);
			delete tptr;
			return ES_ERROR;
		}

		// OK-2009-01-05: [[Bug 7574]] - Due to fix for bug 7463, MCFuncref::eval now takes the object
		// from the exec point passed to it, so we have to change this to the required object in order
		// for value to work for objects other than the current one executing.
		MCObject *t_old_object;
		t_old_object = ep . getobj();
		ep . setobj(optr);

		// MW-2009-01-28: [[ Inherited parentScripts ]]
		// The parentScript property is now a MCParentScriptUse* so, save and restore that instead.
		MCParentScriptUse *t_old_parentscript;
		t_old_parentscript = ep . getparentscript();
		ep . setparentscript(NULL);

		if (optr->eval(tptr, ep) != ES_NORMAL)
		{
			MCeerror->add(EE_VALUE_ERROR, line, pos, tptr);
			delete tptr;

			ep . setobj(t_old_object);

			return ES_ERROR;
		}

		ep . setobj(t_old_object);
		ep . setparentscript(t_old_parentscript);

		delete tptr;
	}
	else
	{
		Exec_stat t_stat;
        t_stat = ep . eval(ep);
		
		if (t_stat != ES_NORMAL)
		{
			MCeerror->add(EE_VALUE_ERROR, line, pos, ep.getsvalue());
			return ES_ERROR;
		}
	}
	return ES_NORMAL;
#endif /* MCValue */

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

void MCValue::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	if (source != nil)
		source -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstantNil(ctxt);

	if (source != nil && object != nil)
	{
		object -> compile_object_ptr(ctxt);

		MCSyntaxFactoryEvalMethod(ctxt, kMCEngineEvalValueWithObjectMethodInfo);
	}
	else
		MCSyntaxFactoryEvalMethod(ctxt,kMCEngineEvalValueMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
}

Parse_stat MCVariables::parse(MCScriptPoint &sp, Boolean the)
{
	return MCFunction::parse(sp, the);
}

#ifdef /* MCVariables */ LEGACY_EXEC
	// MW-2013-11-15: [[ Bug 11277 ]] If no handler, then process the handler list
	//   (server script scope).
	if (ep . gethandler() != nil)
		return ep.gethandler()->getvarnames(ep, True);
	else
	{
		ep.clear();
		ep.gethlist() -> appendlocalnames(ep);
		ep.appendnewline();
		ep.gethlist() -> appendglobalnames(ep, True);
	}
	return ES_NORMAL;
#endif /* MCVariables */


#ifdef /* MCVersion */ LEGACY_EXEC
	ep.setstaticcstring(MCversionstring);
	return ES_NORMAL;
#endif /* MCVersion */


#ifdef /* MCWeekDayNames */ LEGACY_EXEC
	MCD_weekdaynames(P_UNDEFINED, ep);
	return ES_NORMAL;
#endif /* MCWeekDayNames */


#ifdef /* MCWaitDepth */ LEGACY_EXEC
	ep.setnvalue(MCwaitdepth);
	return ES_NORMAL;
#endif /* MCWaitDepth */


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
	object = new MCChunk(False);
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
#ifdef /* MCWithin */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;
	if (object->getobj(ep, optr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_WITHIN_NOCONTROL, line, pos);
		return ES_ERROR;
	}
	int2 x, y;
	if (point->eval(ep) != ES_NORMAL || !MCU_stoi2x2(ep.getsvalue(), x, y))
	{
		MCeerror->add(EE_WITHIN_NAP, line, pos, ep.getsvalue());
		return ES_ERROR;
	}
	if (optr->gettype() < CT_GROUP)
		ep.setboolean(MCU_point_in_rect(optr->getrect(), x, y));
	else
	{
		MCControl *cptr = (MCControl *)optr;
		MCRectangle drect;
		MCU_set_rect(drect, x, y, 1, 1);
		ep.setboolean(cptr->maskrect(drect));
	}
	return ES_NORMAL;
#endif /* MCWithin */

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

void MCWithin::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	object -> compile_object_ptr(ctxt);

	point -> compile(ctxt);

	MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalWithinMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
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
#ifdef /* MCMCISendString */ LEGACY_EXEC
	if (string->eval(ep) != ES_NORMAL)
	{
		MCeerror->add (EE_MCISENDSTRING_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	char *sptr = ep.getsvalue().clone();
	char buffer[256];
	buffer[0] = '\0';

	if (MCS_mcisendstring(sptr, buffer))
	{
		ep . copysvalue(buffer);
		MCresult -> clear(False);
	}
		else
		{
		ep . clear();
			MCresult->copysvalue(buffer);
		}

	delete sptr;

	return ES_NORMAL;
#endif /* MCMCISendString */
    
    MCAutoStringRef t_string;
    MCAutoStringRef t_result;

    if (!MCExecValueTraits<MCStringRef>::eval(ctxt, string, EE_MCISENDSTRING_BADSOURCE, &t_string))
        return;
    
    MCMultimediaEvalMCISendString(ctxt, *t_string, &t_result);
    
    if (!ctxt . HasError())
        MCExecValueTraits<MCStringRef>::set(r_value, MCValueRetain(*t_result));
}

void MCMCISendString::compile(MCSyntaxFactoryRef ctxt)
{
    MCSyntaxFactoryBeginExpression(ctxt, line, pos);
    
    string -> compile(ctxt);
    
    MCSyntaxFactoryEvalMethod(ctxt, kMCMultimediaEvalMCISendStringMethodInfo);
    
    MCSyntaxFactoryEndExpression(ctxt);
}

#ifdef /* MCDeleteRegistry */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_REGISTRY_WRITE)
	{
		MCeerror->add(EE_REGISTRY_NOPERM, line, pos);
		return ES_ERROR;
	}
	if (key->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_SETREGISTRY_BADEXP, line, pos);
		return ES_ERROR;
	}
	char *kptr = ep.getsvalue().clone();
	MCS_delete_registry(kptr, ep);
	delete kptr;
	return ES_NORMAL;
#endif /* MCDeleteRegistry */

#ifdef /* MCListRegistry */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_REGISTRY_READ)
	{
		MCeerror->add(EE_REGISTRY_NOPERM, line, pos);
		return ES_ERROR;
	}
	if (key->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_SETREGISTRY_BADEXP, line, pos);
		return ES_ERROR;
	}
	MCS_list_registry(ep);
	return ES_NORMAL;
#endif /* MCListRegistry */

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
#ifdef /* MCQueryRegistry */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_REGISTRY_READ)
	{
		MCeerror->add(EE_REGISTRY_NOPERM, line, pos);
		return ES_ERROR;
	}
	if (key->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_QUERYREGISTRY_BADEXP, line, pos);
		return ES_ERROR;
	}
	
	MCVariable* t_var;
	MCVariableValue *t_var_value;
	if (type != NULL)
	{
		if (type -> evalcontainer(ep, t_var, t_var_value) != ES_NORMAL)
		{
			MCeerror -> add(EE_QUERYREGISTRY_BADDEST, line, pos);
			return ES_ERROR;
		}
	}
	else
	{
		t_var = NULL;
		t_var_value = NULL;
	}
	
	const char *t_type_string = NULL;
	MCS_query_registry(ep, t_var != NULL ? &t_type_string : NULL);
	
	if (t_var != NULL && t_type_string != NULL)
	{
		t_var_value -> assign_constant_string(t_type_string);
		t_var -> synchronize(ep, True);
	}
	
	return ES_NORMAL;
#endif /* MCQueryRegistry */

    MCAutoStringRef t_key;
    if (!ctxt . EvalExprAsStringRef(key, EE_QUERYREGISTRY_BADEXP, &t_key))
        return;

	MCAutoPointer<MCContainer> t_container;
    MCAutoStringRef t_type;
	if (type != NULL)
	{
		if (!type -> evalcontainer(ctxt, &t_container))
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
		if (*t_container != nil)
			/* UNCHECKED */ t_container->set_valueref(*t_type);
	}
}

void MCQueryRegistry::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);

	key -> compile(ctxt);

	if (type != nil)
	{
		type -> compile_out(ctxt);
		MCSyntaxFactoryEvalMethod(ctxt, kMCFilesEvalQueryRegistryWithTypeMethodInfo);
	}
	else
		MCSyntaxFactoryEvalMethod(ctxt, kMCFilesEvalQueryRegistryMethodInfo);

	MCSyntaxFactoryEndExpression(ctxt);
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
#ifdef /* MCSetRegistry */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_REGISTRY_WRITE)
	{
		MCeerror->add(EE_REGISTRY_NOPERM, line, pos);
		return ES_ERROR;
	}
	char *tstring;
	if (type != NULL)
	{
		if (type->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_SETREGISTRY_BADEXP, line, pos);
			return ES_ERROR;
		}
		tstring = ep.getsvalue().clone();
	}
	else
		tstring = NULL;

	if (key->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_SETREGISTRY_BADEXP, line, pos);
		return ES_ERROR;
	}
	char *kptr = ep.getsvalue().clone();
	if (value->eval(ep) != ES_NORMAL)
	{
		delete kptr;
		MCeerror->add
		(EE_SETREGISTRY_BADEXP, line, pos);
		return ES_ERROR;
	}
	MCS_set_registry(kptr, ep, tstring);
	delete tstring;
	delete kptr;
	return ES_NORMAL;
#endif /* MCSetRegistry */

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

void MCSetRegistry::compile(MCSyntaxFactoryRef ctxt)
{
	if (type != nil)
		compile_with_args(ctxt, kMCFilesEvalSetRegistryWithTypeMethodInfo, key, value, type);
	else
		compile_with_args(ctxt, kMCFilesEvalSetRegistryWithTypeMethodInfo, key, value);
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
#ifdef /* MCCopyResource */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}

	char *sptr = NULL;
	char *dptr = NULL;
	char *tptr = NULL;
	char *nptr = NULL;
	char *iptr = NULL;
	Boolean error = False;
	if (source->eval(ep) != ES_NORMAL)
		error = True;
	else
	{
		sptr = ep.getsvalue().clone();
		if (dest->eval(ep) != ES_NORMAL)
			error = True;
		else
		{
			dptr = ep.getsvalue().clone();
			if (type->eval(ep) != ES_NORMAL || ep.getsvalue().getlength() != 4)
				error = True;
			else
			{
				tptr = ep.getsvalue().clone();
				if (name->eval(ep) != ES_NORMAL)
					error = True;
				else
				{
					nptr = ep.getsvalue().clone();
					if (newid != NULL)
						if (newid->eval(ep) != ES_NORMAL)
							error = True;
						else
							iptr = ep.getsvalue().clone();
				}
			}
		}
	}
	if (!error)
		MCS_copyresource(sptr, dptr, tptr, nptr, iptr);
	delete sptr;
	delete dptr;
	delete tptr;
	delete nptr;
	delete iptr;
	if (error)
	{
		MCeerror->add(EE_RESOURCES_BADPARAM, line, pos);
		return ES_ERROR;
	}
	ep.clear();
	return ES_NORMAL;
#endif /* MCCopyResource */

	
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

void MCCopyResource::compile(MCSyntaxFactoryRef ctxt)
{
	if (newid != nil)
		compile_with_args(ctxt, kMCFilesEvalCopyResourceWithNewIdMethodInfo, source, dest, type, name, newid);
	else
		compile_with_args(ctxt, kMCFilesEvalCopyResourceMethodInfo, source, dest, type, name);
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
#ifdef /* MCDeleteResource */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}

	char *sptr = NULL;
	char *tptr = NULL;
	char *nptr = NULL;
	Boolean error = False;
	if (source->eval(ep) != ES_NORMAL)
		error = True;
	else
	{
		sptr = ep.getsvalue().clone();
		if (type->eval(ep) != ES_NORMAL || ep.getsvalue().getlength() != 4)
			error = True;
		else
		{
			tptr = ep.getsvalue().clone();
			if (name->eval(ep) != ES_NORMAL)
				error = True;
			else
				nptr = ep.getsvalue().clone();
		}
	}
	if (!error)
		MCS_deleteresource(sptr, tptr, nptr);
	delete sptr;
	delete tptr;
	delete nptr;
	if (error)
	{
		MCeerror->add(EE_RESOURCES_BADPARAM, line, pos);
		return ES_ERROR;
	}
	ep.clear();
	return ES_NORMAL;
#endif /* MCDeleteResource */

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

void MCDeleteResource::compile(MCSyntaxFactoryRef ctxt)
{
	compile_with_args(ctxt, kMCFilesEvalDeleteResourceMethodInfo, source, type, name);
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
#ifdef /* MCGetResource */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}
	char *sptr = NULL;
	char *tptr = NULL;
	char *nptr = NULL;
	Boolean error = False;
	if (source->eval(ep) != ES_NORMAL)
		error = True;
	else
	{
		sptr = ep.getsvalue().clone();
		if (type->eval(ep) != ES_NORMAL || ep.getsvalue().getlength() != 4)
			error = True;
		else
		{
			tptr = ep.getsvalue().clone();
			if (name->eval(ep) != ES_NORMAL)
				error = True;

			else
				nptr = ep.getsvalue().clone();
		}
	}
	ep.clear();
	if (!error)
		MCS_getresource(sptr, tptr, nptr, ep);
	delete sptr;
	delete tptr;
	delete nptr;
	if (error)
	{
		MCeerror->add
		(EE_RESOURCES_BADPARAM, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCGetResource */
	
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

void MCGetResource::compile(MCSyntaxFactoryRef ctxt)
{
	compile_with_args(ctxt, kMCFilesEvalGetResourceMethodInfo, source, type, name);
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
#ifdef /* MCGetResources */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}
	char *sptr = NULL;
	char *tptr = NULL;
	Boolean error = False;
	if (source->eval(ep) != ES_NORMAL)
		error = True;
	else
	{
		sptr = ep.getsvalue().clone();
		if (type != NULL)
			if (type->eval(ep) != ES_NORMAL)

				error = True;
			else
				tptr = ep.getsvalue().clone();
	}
	char *dptr = NULL;
	if (!error)
		dptr = MCS_getresources(sptr, tptr);
	if (dptr != NULL)
		ep.copysvalue(dptr, strlen(dptr));
	else
		ep.clear();
	delete sptr;
	delete tptr;
	if (error)
	{
		MCeerror->add
		(EE_RESOURCES_BADPARAM, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCGetResources */

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

void MCGetResources::compile(MCSyntaxFactoryRef ctxt)
{
	if (type != nil)
		compile_with_args(ctxt, kMCFilesEvalGetResourcesWithTypeMethodInfo, source, type);
	else
		compile_with_args(ctxt, kMCFilesEvalGetResourcesMethodInfo, source);
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
#ifdef /* MCSetResource */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}
	char *sptr = NULL;
	char *tptr = NULL;
	char *iptr = NULL;
	char *nptr = NULL;
	char *fptr = NULL;
	Boolean error = False;
	if (source->eval(ep) != ES_NORMAL)
		error = True;
	else
	{
		sptr = ep.getsvalue().clone();
		if (type->eval(ep) != ES_NORMAL || ep.getsvalue().getlength() != 4)
			error = True;
		else
		{
			tptr = ep.getsvalue().clone();
			if (id->eval(ep) != ES_NORMAL)
				error = True;
			else
				iptr = ep.getsvalue().clone();
			if (name->eval(ep) != ES_NORMAL)
				error = True;
			else
			{
				nptr = ep.getsvalue().clone();
				if (strlen(nptr) == 0 && strlen(iptr) == 0
				        || flags->eval(ep) != ES_NORMAL)
					error = True;
				else
				{
					fptr = ep.getsvalue().clone();
					if (value->eval(ep) != ES_NORMAL)
						error = True;
				}
			}
		}
	}
	if (!error)
		MCS_setresource(sptr, tptr, iptr, nptr, fptr, ep.getsvalue());
	delete sptr;
	delete tptr;
	delete iptr;
	delete nptr;
	delete fptr;
	if (error)
	{
		MCeerror->add(EE_RESOURCES_BADPARAM, line, pos);
		return ES_ERROR;
	}
	ep.clear();
	return ES_NORMAL;
#endif /* MCSetResource */
	
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
    
    if (!MCStringGetLength(*t_id) > 0 && !MCStringGetLength(*t_name) > 0)
    {
        ctxt . LegacyThrow(EE_RESOURCES_BADPARAM);
        return;
    }
	
	MCFilesEvalSetResource(ctxt, *t_source, *t_type, *t_id, *t_name, *t_flags, *t_value, r_value . stringref_value);
    r_value . type = kMCExecValueTypeStringRef;
}

void MCSetResource::compile(MCSyntaxFactoryRef ctxt)
{
	compile_with_args(ctxt, kMCFilesEvalSetResourceMethodInfo, source, type, id, name, flags, value);
}

#ifdef /* MCSpecialFolderPath */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}
	if (type->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_SPECIALFOLDERPATH_BADPARAM, line, pos);
		return ES_ERROR;
	}
    
    // SN-2015-01-16: [[ Bug 14295 ]] Added mode-specific way to get the resources folder
    if (ep . getsvalue() == "resources")
        MCModeGetResourcesFolder(ep);
    else
        MCS_getspecialfolder(ep);
	return ES_NORMAL;
#endif /* MCSpecialFolderPath */

#ifdef /* MCLongFilePath */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}
	if (type->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_LONGFILEPATH_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_longfilepath(ep);
	return ES_NORMAL;
#endif /* MCLongFilePath */

#ifdef /* MCShortFilePath */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}
	if (type->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_SHORTFILEPATH_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	// MW-2010-06-01: The short file path function sets the result if it fails,
	//   therefore we must ensure that it is empty before hand.
	MCresult -> clear();

	MCS_shortfilepath(ep);

	return ES_NORMAL;
#endif /* MCShortFilePath */

#ifdef /* MCAliasReference */ LEGACY_EXEC
	if (type->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_ALIASREFERENCE_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	if (!MCSecureModeCheckDisk(line, pos))
		return ES_ERROR;

	MCS_resolvealias(ep);

	return ES_NORMAL;
#endif /* MCAliasReference */


#ifdef /* MCAlternateLanguages */ LEGACY_EXEC
	// If we don't have 'do alternate' privileges, this function should just 
	// return empty.
	if (!MCSecureModeCanAccessDoAlternate())
	{
		ep . clear();
		return ES_NORMAL;
	}

	MCS_alternatelanguages(ep);

	return ES_NORMAL;
#endif /* MCAlternateLanguages */


#ifdef /* MCCipherNames */ LEGACY_EXEC
	SSL_ciphernames(ep);
	return ES_NORMAL;
#endif /* MCCipherNames */

///////////////////////////////////////////////////////////////////////////////

MCScriptEnvironment *MCHTTPProxyForURL::pac_engine = NULL;

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
#ifdef /* MCHTTPProxyForURL */ LEGACY_EXEC
	Exec_stat t_result;
	t_result = ES_NORMAL;

	char *t_url;
	t_url = NULL;
	if (t_result == ES_NORMAL)
	{
		t_result = url -> eval(ep);
		if (t_result == ES_NORMAL)
			t_url = ep . getsvalue() . clone();
	}

	char *t_host;
	t_host = NULL;
	if (t_result == ES_NORMAL)
	{
		t_result = host -> eval(ep);
		if (t_result == ES_NORMAL)
			t_host = ep . getsvalue() . clone();
	}

	char *t_pac;
	t_pac = NULL;
	if (t_result == ES_NORMAL && pac != NULL)
	{
		t_result = pac -> eval(ep);
		if (t_result == ES_NORMAL)
			t_pac = ep . getsvalue() . clone();
	}

	if (t_result == ES_NORMAL && t_pac != NULL)
	{
		if (pac_engine != NULL)
		{
			pac_engine -> Release();
			pac_engine = NULL;
		}

		if (strlen(t_pac) > 0)
		{
			pac_engine = MCscreen -> createscriptenvironment("javascript");
			if (pac_engine != NULL)
			{
				bool t_success;
				t_success = pac_engine -> Define("__dnsResolve", PACdnsResolve);

				if (t_success)
					t_success = pac_engine -> Define("__myIpAddress", PACmyIpAddress);

				if (t_success)
				{
					char *t_result;
					t_result = pac_engine -> Run(t_pac);
					t_success = t_result != NULL;
					delete t_result;
				}
				
				if (!t_success)
				{
					pac_engine -> Release();
					pac_engine = NULL;
				}
			}
		}
	}

	char *t_proxies;
	t_proxies = NULL;
	if (t_result == ES_NORMAL && pac_engine != NULL)
	{
		const char *t_arguments[2];
		t_arguments[0] = t_url;
		t_arguments[1] = t_host;
		t_proxies = pac_engine -> Call("__FindProxyForURL", t_arguments, 2);
	}

	if (t_proxies != NULL)
		ep . copysvalue(t_proxies);
	else
		ep . clear();

	delete t_proxies;
	delete t_pac;
	delete t_host;
	delete t_url;

	return t_result;
#endif /* MCHTTPProxyForURL */

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

void MCHTTPProxyForURL::compile(MCSyntaxFactoryRef ctxt)
{
	if (pac != nil)
		compile_with_args(ctxt, kMCNetworkEvalHTTPProxyForURLWithPACMethodInfo, url, host, pac);
	else
		compile_with_args(ctxt, kMCNetworkEvalHTTPProxyForURLMethodInfo, url, host);
}

char *MCHTTPProxyForURL::PACdnsResolve(const char* const* p_arguments, unsigned int p_argument_count)
{
	if (p_argument_count != 1)
		return NULL;

	MCAutoStringRef t_address_string;
	MCAutoStringRef p_arguments0_string;
	/* UNCHECKED */ MCStringCreateWithCString(p_arguments[0], &p_arguments0_string);
	MCS_dnsresolve(*p_arguments0_string, &t_address_string);
	
	char *t_address = nil;
	if (*t_address_string != nil)
		/* UNCHECKED */ MCStringConvertToCString(*t_address_string, t_address);

	return t_address;
}

char *MCHTTPProxyForURL::PACmyIpAddress(const char* const* p_arguments, unsigned int p_argument_count)
{
	if (p_argument_count != 0)
		return NULL;

	MCAutoStringRef t_address_string;
	MCS_hostaddress(&t_address_string);

	char *t_address = nil;
	if (*t_address_string != nil)
		/* UNCHECKED */ MCStringConvertToCString(*t_address_string, t_address);

	return t_address;
}

///////////////////////////////////////////////////////////////////////////////

#ifdef /* MCRandomBytes */ LEGACY_EXEC
	if (byte_count->eval(ep) != ES_NORMAL && ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_RANDOMBYTES_BADCOUNT, line, pos);
		return ES_ERROR;
	}
	
	size_t t_count;
	t_count = ep.getuint4();
	
	// MW-2013-05-21: [[ RandomBytes ]] Updated to use system primitive, rather
	//   than SSL.
	
	void *t_bytes;
	t_bytes = ep . getbuffer(t_count);
	if (t_bytes == nil)
	{
		MCeerror -> add(EE_NO_MEMORY, line, pos);
		return ES_ERROR;
	}
	
	if (MCU_random_bytes(t_count, t_bytes))
		ep . setlength(t_count);
	else
	{
		ep . clear();
		MCresult->copysvalue(MCString("error: could not get random bytes"));
	}
	
	return ES_NORMAL;
#endif /* MCRandomBytes */

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
#ifdef /* MCControlAtLoc */ LEGACY_EXEC
	MCPoint t_location;
	if (location -> eval(ep) != ES_NORMAL ||
		!MCU_stoi2x2(ep . getsvalue(), t_location . x, t_location . y))
	{
		MCeerror -> add(EE_CONTROLATLOC_NAP, line, pos);
		return ES_ERROR;
	}

	MCStack *t_stack;
	if (!is_screen)
		t_stack = MCdefaultstackptr;
	else
	{
		t_stack = MCscreen -> getstackatpoint(t_location . x, t_location . y);
		// IM-2013-10-11: [[ FullscreenMode ]] Update to use stack coord conversion methods
		if (t_stack != nil)
			t_location = t_stack->globaltostackloc(t_location);
	}

	// If the location is over a stack, then return nil.
	if (t_stack == nil)
	{
		ep . clear();
		return ES_NORMAL;
	}
	
	// We now have a stack and a location in card co-ords so let's do the hittest.
	MCObject *t_object;
	t_object = t_stack -> getcard() -> hittest(t_location . x, t_location . y);
	
	if (!is_screen)
	{
		if (t_object -> gettype() != CT_CARD)
		{
			t_object -> getprop(0, P_LAYER, ep, False);
			ep . insert("control ", 0, 0);
		}
		else
			ep . clear();
			
	}
	else
		t_object -> names(P_LONG_ID, ep, 0);
	
	return ES_NORMAL;
#endif /* MCControlAtLoc */

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
#ifdef /* MCUuidFunc */ LEGACY_EXEC
	// First work out what type we want.
	MCUuidType t_type;
	if (type == nil)
		t_type = kMCUuidTypeRandom;
	else
	{
		if (type -> eval(ep) != ES_NORMAL)
		{
			MCeerror -> add(EE_UUID_BADTYPE, line, pos);
			return ES_ERROR;
		}
		
		if (ep . getsvalue() == "random")
		{
			// If there is more than one parameter, it's an error.
			if (name != nil)
			{
				MCeerror -> add(EE_UUID_TOOMANYPARAMS, line, pos);
				return ES_ERROR;
			}
			
			t_type = kMCUuidTypeRandom;
		}
		else if (ep . getsvalue() == "md5")
			t_type = kMCUuidTypeMD5;
		else if (ep . getsvalue() == "sha1")
			t_type = kMCUuidTypeSHA1;
		else
		{
			// If the type isn't one of 'random', 'md5', 'sha1' then it's
			// an error.
			MCeerror -> add(EE_UUID_UNKNOWNTYPE, line, pos);
			return ES_ERROR;
		}
	}
	
	// If it is not of random type, then evaluate the other params.
	MCUuid t_namespace_id;
	MCString t_name;
	if (t_type != kMCUuidTypeRandom)
	{
		// If there aren't namespace_id and name exprs, its an error.
		if (namespace_id == nil || name == nil)
		{
			MCeerror -> add(EE_UUID_TOOMANYPARAMS, line, pos);
			return ES_ERROR;
		}
	
		// Evaluate the namespace parameter.
		if (namespace_id -> eval(ep) != ES_NORMAL)
		{
			MCeerror -> add(EE_UUID_BADNAMESPACEID, line, pos);
			return ES_ERROR;
		}
		
		// Attempt to convert it to a uuid.
		if (!MCUuidFromCString(ep . getcstring(), t_namespace_id))
		{
			MCeerror -> add(EE_UUID_NAMESPACENOTAUUID, line, pos);
			return ES_ERROR;
		}
		
		// Evaluate the name parameter.
		if (name -> eval(ep) != ES_NORMAL)
		{
			MCeerror -> add(EE_UUID_BADNAME, line, pos);
			return ES_ERROR;
		}
		
		// Borrow the value from the ep - this is okay in this instance because
		// ep isn't used again until the name has been utilised.
		t_name = ep . getsvalue();
	}
	
	// Generate the uuid.
	MCUuid t_uuid;
	switch(t_type)
	{
	case kMCUuidTypeRandom:
		if (!MCUuidGenerateRandom(t_uuid))
		{
			MCeerror -> add(EE_UUID_NORANDOMNESS, line, pos);
			return ES_ERROR;
		}
		break;
	
	case kMCUuidTypeMD5:
		MCUuidGenerateMD5(t_namespace_id, t_name, t_uuid);
		break;
		
	case kMCUuidTypeSHA1:
		MCUuidGenerateSHA1(t_namespace_id, t_name, t_uuid);
		break;
		
	default:
		assert(false);
		break;
	}
	
	// Convert the uuid to a string.
	char t_uuid_buffer[kMCUuidCStringLength];
	MCUuidToCString(t_uuid, t_uuid_buffer);
	
	// And set it as the return value (in the ep).
	ep . copysvalue(t_uuid_buffer);
	
	return ES_NORMAL;
#endif /* MCUuidFunc */

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
	m_object = new MCChunk(False);
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
#ifdef /* MCMeasureText */ LEGACY_EXEC
    MCObject *t_object_ptr;
	uint4 parid;
	if (m_object->getobj(ep, t_object_ptr, parid, True) != ES_NORMAL)
	{
		MCeerror->add
		(EE_MEASURE_TEXT_NOOBJECT, line, pos);
		return ES_ERROR;
	}
    
    if (m_text -> eval(ep) != ES_NORMAL)
    {
        MCeerror -> add(EE_CHUNK_BADTEXT, line, pos);
        return ES_ERROR;
    }
    
    MCRectangle t_bounds = t_object_ptr -> measuretext(ep.getsvalue(), m_is_unicode);
    
    if (m_mode)
    {
        if (m_mode -> eval(ep) != ES_NORMAL)
        {
            MCeerror -> add(EE_CHUNK_BADTEXT, line, pos);
            return ES_ERROR;
        }
        
        if (ep.getsvalue() == "size")
        {
            ep.setpoint(t_bounds . width,t_bounds . height);
            return ES_NORMAL;
        }
        
        if(ep.getsvalue() == "bounds")
        {
            ep.setrectangle(t_bounds);
            return ES_NORMAL;
        }
    }
    
    ep.setuint(t_bounds . width);
    return ES_NORMAL;
#endif /* MCMeasureText */ 
    
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
