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
// MCFunction class declarations
//
#ifndef	FUNCTIONS_H
#define	FUNCTIONS_H

#include "express.h"
#include "executionerrors.h"
#include "parseerrors.h"
#include "mcerror.h"

#include "exec.h"
#include "param.h"
#include "scriptpt.h"

////////////////////////////////////////////////////////////////////////////////

class MCFunction : public MCExpression
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	Parse_stat parsetarget(MCScriptPoint &spt, Boolean the,
	                       Boolean needone, MCChunk *&chunk);
    bool params_to_doubles(MCExecContext& ctxt, MCParameter *p_params, real64_t*& r_doubles, uindex_t& r_count);
};

// Helper class that simplifies compiling of functions not taking any arguments.
class MCConstantFunction: public MCFunction
{
public:
};

// Helper class that simplifies compiling of functions taking one arguments.
class MCUnaryFunction: public MCFunction
{
public:
};

// Helper class that simplifies compiling of functions taking a variable number of of MCParameters.
class MCParamFunction: public MCFunction
{
public:
    bool params_to_doubles(MCExecContext &ctxt, real64_t *&r_doubles, uindex_t &r_count);

protected:
    MCParameter* params;
};

////////////////////////////////////////////////////////////////////////////////

// Helper class that simplifies evaluation of functions not taking any arguments.
extern MCError *MCperror;

template<typename ReturnType, void (*EvalFunction)(MCExecContext&, typename MCExecValueTraits<ReturnType>::out_type)>
class MCConstantFunctionCtxt: public MCConstantFunction
{
public:
	void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
	{
		ReturnType t_result;
		EvalFunction(ctxt, t_result);
		if (!ctxt . HasError())
			MCExecValueTraits<ReturnType>::set(r_value, t_result);
	}
};

template<typename ParamType,
         typename ReturnType,
         void (*EvalFunction)(MCExecContext&, typename MCExecValueTraits<ParamType>::in_type, typename MCExecValueTraits<ReturnType>::out_type),
         Exec_errors EvalError,
         Parse_errors ParseError>
class MCUnaryFunctionCtxt: public MCUnaryFunction
{
public:
    MCUnaryFunctionCtxt() { m_expression = nil; }

    virtual ~MCUnaryFunctionCtxt() { delete m_expression; }

    virtual Parse_stat parse(MCScriptPoint &sp, Boolean the)
    {
        if (sp.is_eol())
        {
            MCperror -> add(PE_FACTOR_NOOF, sp);
            return PS_ERROR;
        }

        if (get1param(sp, &m_expression, the) != PS_NORMAL)
        {
            MCperror -> add(ParseError, sp);
            return PS_ERROR;
        }

        return PS_NORMAL;
    }

    virtual void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
    {
        ReturnType t_result;
        ParamType t_param;

        if (!MCExecValueTraits<ParamType>::eval(ctxt, m_expression, EvalError, t_param))
            return;

        EvalFunction(ctxt, t_param, t_result);

        MCExecValueTraits<ParamType>::release(t_param);

        if (!ctxt . HasError())
            MCExecValueTraits<ReturnType>::set(r_value, t_result);
    }

protected:
    MCExpression *m_expression;
};

template<void (*EvalExprMethod)(MCExecContext &, real64_t*, uindex_t, real64_t&),
         Exec_errors EvalError,
         Parse_errors ParseError>
class MCParamFunctionCtxt: public MCParamFunction
{
public:
    MCParamFunctionCtxt()
    {
        params = nil;
    }

    virtual ~MCParamFunctionCtxt()
    {
        while (params != nil)
        {
            MCParameter *tparams = params;
            params = params->getnext();
            delete tparams;
        }
    }

    virtual Parse_stat parse(MCScriptPoint &sp, Boolean the)
    {
        initpoint(sp);

        if (getparams(sp, &params) != PS_NORMAL)
        {
            MCperror->add(ParseError, line, pos);
            return PS_ERROR;
        }
        return PS_NORMAL;
    }

    void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
    {
        MCAutoArray<real64_t> t_values;
        real64_t t_result;

        if (!params_to_doubles(ctxt, t_values.PtrRef(), t_values.SizeRef()))
        {
            ctxt . LegacyThrow(EvalError);
            return;
        }

        EvalExprMethod(ctxt, t_values.Ptr(), t_values.Size(), t_result);

        if (!ctxt . HasError())
        {
            r_value . double_value = t_result;
            r_value . type = kMCExecValueTypeDouble;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class MCArrayDecode: public MCUnaryFunctionCtxt<MCDataRef, MCArrayRef, MCArraysEvalArrayDecode, EE_ARRAYDECODE_BADSOURCE, PE_ARRAYDECODE_BADPARAM>
{
public:
    MCArrayDecode(){}
    virtual ~MCArrayDecode(){}
};

class MCArrayEncode: public MCFunction
{
    MCExpression *source;
    MCExpression *version;
public:
    MCArrayEncode()
    {
        source = version = NULL;
    }

    virtual ~MCArrayEncode();
    virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCBase64Decode : public MCUnaryFunctionCtxt<MCStringRef, MCDataRef, MCFiltersEvalBase64Decode, EE_BASE64DECODE_BADSOURCE, PE_BASE64DECODE_BADPARAM>
{
public:
    MCBase64Decode() {}
    virtual ~MCBase64Decode(){}
};

class MCBase64Encode : public MCUnaryFunctionCtxt<MCDataRef, MCStringRef, MCFiltersEvalBase64Encode, EE_BASE64ENCODE_BADSOURCE, PE_BASE64ENCODE_BADPARAM>
{
public:
    MCBase64Encode(){}
    virtual ~MCBase64Encode(){}
};

class MCBaseConvert : public MCFunction
{
	MCExpression *source;
	MCExpression *sourcebase;
    MCExpression *destbase;
public:
	MCBaseConvert()
	{
		source = sourcebase = destbase = NULL;
	}
	virtual ~MCBaseConvert();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

// AL-2014-10-17: [[ BiDi ]] Returns the result of applying the bi-directional algorithm to text
class MCBidiDirection : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCStringsEvalBidiDirection, EE_BIDIDIRECTION_BADSOURCE, PE_BIDIDIRECTION_BADPARAM>
{
public:
    MCBidiDirection(){}
    virtual ~MCBidiDirection(){}
};

class MCBinaryEncode : public MCFunction
{
	MCParameter *params;
public:
	MCBinaryEncode()
	{
		params = NULL;
	}
	virtual ~MCBinaryEncode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCBinaryDecode : public MCFunction
{
	MCParameter *params;
public:
	MCBinaryDecode()
	{
		params = NULL;
	}
	virtual ~MCBinaryDecode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCBuildNumber : public MCConstantFunctionCtxt<integer_t, MCEngineEvalBuildNumber>
{
public:
};

class MCCachedUrls : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalCachedUrls>
{
public:
};

class MCCapsLockKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalCapsLockKey>
{
public:
};

class MCCharToNum : public MCUnaryFunctionCtxt<MCValueRef, MCValueRef, MCStringsEvalCharToNum, EE_CHARTONUM_BADSOURCE, PE_CHARTONUM_BADPARAM>
{
public:
    MCCharToNum(){}
    virtual ~MCCharToNum(){}
};

class MCByteToNum : public MCUnaryFunctionCtxt<MCStringRef, integer_t, MCStringsEvalByteToNum, EE_BYTETONUM_BADSOURCE, PE_BYTETONUM_BADPARAM>
{
public:
    MCByteToNum(){}
    virtual ~MCByteToNum(){}
};

class MCChunkOffset : public MCFunction
{
	MCExpression *part;
	MCExpression *whole;
	MCExpression *offset;
protected:
	Chunk_term delimiter;
public:
	MCChunkOffset()
	{
		part = whole = offset = NULL;
	}
	virtual ~MCChunkOffset();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCClipboardFunc : public MCConstantFunctionCtxt<MCNameRef, MCPasteboardEvalClipboard>
{
public:
};

class MCCompress : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalCompress, EE_COMPRESS_BADSOURCE, PE_COMPRESS_BADPARAM>
{
public:
    MCCompress(){}
    virtual ~MCCompress(){}
};

class MCConstantNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalConstantNames>
{
public:

};

class MCItemOffset : public MCChunkOffset
{
public:
	MCItemOffset()
	{
		delimiter = CT_ITEM;
	}
};

class MCLineOffset : public MCChunkOffset
{
public:
	MCLineOffset()
	{
		delimiter = CT_LINE;
	}
};

class MCOffset : public MCChunkOffset
{
public:
	MCOffset()
	{
		delimiter = CT_CHARACTER;
	}
};

class MCWordOffset : public MCChunkOffset
{
public:
	MCWordOffset()
	{
		delimiter = CT_WORD;
	}
};

class MCParagraphOffset : public MCChunkOffset
{
public:
	MCParagraphOffset()
	{
		delimiter = CT_PARAGRAPH;
	}
};

class MCSentenceOffset : public MCChunkOffset
{
public:
	MCSentenceOffset()
	{
		delimiter = CT_SENTENCE;
	}
};

class MCTrueWordOffset : public MCChunkOffset
{
public:
	MCTrueWordOffset()
	{
		delimiter = CT_TRUEWORD;
	}
};

class MCCodepointOffset : public MCChunkOffset
{
public:
	MCCodepointOffset()
	{
		delimiter = CT_CODEPOINT;
	}
};

class MCCodeunitOffset : public MCChunkOffset
{
public:
	MCCodeunitOffset()
	{
		delimiter = CT_CODEUNIT;
	}
};

class MCByteOffset : public MCChunkOffset
{
public:
	MCByteOffset()
	{
		delimiter = CT_BYTE;
	}
};

class MCTokenOffset : public MCChunkOffset
{
public:
    MCTokenOffset()
    {
        delimiter = CT_TOKEN;
    }
};

class MCClickChar : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickChar>
{
public:
};

class MCClickCharChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickCharChunk>
{
public:
};

class MCClickChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickChunk>
{
public:
};

class MCClickField : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickField>
{
public:
};

class MCClickH : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalClickH>
{
public:
};

class MCClickLine : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickLine>
{
public:
};

class MCCommandArguments : public MCFunction
{
    MCAutoPointer<MCExpression> argument_index;
public:
    MCCommandArguments() :
        argument_index(nullptr)
    {
    }
    virtual Parse_stat parse(MCScriptPoint &sp, Boolean the);
    virtual void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
};

class MCCommandName : public MCFunction
{
public:
    virtual void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
};

class MCClickLoc : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickLoc>
{
public:
};

class MCClickStack : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickStack>
{
public:
};

class MCClickText : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickText>
{
public:
};

class MCClickV : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalClickV>
{
public:
};

class MCColorNames : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalColorNames>
{
public:

};

class MCCommandNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalCommandNames>
{
public:

};

class MCCommandKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalCommandKey>
{
public:
};

class MCControlKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalControlKey>
{
public:
};

class MCDate : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalDate>
{
public:
};

class MCDateFormat : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalDateFormat>
{
public:
};

class MCDecompress : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalDecompress, EE_DECOMPRESS_BADSOURCE, PE_DECOMPRESS_BADPARAM>
{
public:
    MCDecompress(){}
    virtual ~MCDecompress(){}
};

class MCDiskSpace : public MCConstantFunctionCtxt<double, MCFilesEvalDiskSpace>
{
public:
};

class MCDNSServers : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalDNSServers>
{
public:
};

class MCDragDestination: public MCConstantFunctionCtxt<MCStringRef, MCPasteboardEvalDragDestination>
{
public:
};

class MCDragSource: public MCConstantFunctionCtxt<MCStringRef, MCPasteboardEvalDragSource>
{
public:
};

class MCDriverNames : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalDriverNames>
{
	MCExpression *type;
public:
	MCDriverNames()
	{
		type = NULL;
	}
	virtual ~MCDriverNames();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
};

class MCDrives : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalDrives>
{
	MCExpression *type;
public:
	MCDrives()
	{
		type = NULL;
	}
	virtual ~MCDrives();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
};

class MCDropChunk: public MCConstantFunctionCtxt<MCStringRef, MCPasteboardEvalDropChunk>
{
public:
};

class MCQTEffects : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalQTEffects>
{
public:
};

class MCRecordCompressionTypes: public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalRecordCompressionTypes>
{
public:
};

class MCRecordFormats: public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalRecordFormats>
{
public:
};

class MCRecordLoudness : public MCConstantFunctionCtxt<integer_t, MCMultimediaEvalRecordLoudness>
{
public:
};

class MCEnvironment : public MCConstantFunctionCtxt<MCNameRef, MCEngineEvalEnvironment>
{
public:
};

class MCEncrypt : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCSecurityEvalEncrypt, EE_ENCRYPT_BADSOURCE, PE_ENCRYPT_BADPARAM>
{
public:
    MCEncrypt(){}
    virtual ~MCEncrypt(){}
};

class MCEventCapsLockKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalEventCapsLockKey>
{
public:
};

class MCEventCommandKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalEventCommandKey>
{
public:
};

class MCEventControlKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalEventControlKey>
{
public:
};

class MCEventOptionKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalEventOptionKey>
{
public:
};

class MCEventShiftKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalEventShiftKey>
{
public:
};

class MCExists : public MCFunction
{
	MCChunk *object;
public:
	MCExists()
	{
		object = NULL;
	}
	virtual ~MCExists();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCExtents : public MCUnaryFunctionCtxt<MCArrayRef, MCStringRef, MCArraysEvalExtents, EE_EXTENTS_BADSOURCE, PE_EXTENTS_BADPARAM>
{
public:
    MCExtents(){}
    virtual ~MCExtents(){}
};

class MCFileItems : public MCFunction
{
public:
	MCFileItems(bool p_files) : m_folder(nil), m_kind(nullptr), m_files(p_files)  {}
	virtual ~MCFileItems();
	virtual Parse_stat parse(MCScriptPoint &, Boolean p_is_the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
private:
	MCExpression *m_folder;
    MCExpression *m_kind;
    bool m_files;
};

class MCFlushEvents : public MCUnaryFunctionCtxt<MCNameRef, MCStringRef, MCInterfaceEvalFlushEvents, EE_FLUSHEVENTS_BADTYPE, PE_FLUSHEVENTS_BADPARAM>
{
public:
    MCFlushEvents(){}
    virtual ~MCFlushEvents(){}
};

class MCFocusedObject : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFocusedObject>
{
public:
};

class MCFontNames : public MCUnaryFunction
{
	MCExpression *type;
public:
	MCFontNames()
	{
		type = NULL;
	}
	virtual ~MCFontNames();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
};

class MCFontLanguage : public MCUnaryFunctionCtxt<MCStringRef, MCNameRef, MCTextEvalFontLanguage, EE_FONTLANGUAGE_BADFONTNAME, PE_FONTLANGUAGE_BADPARAM>
{
public:
    MCFontLanguage(){}
    virtual ~MCFontLanguage(){}
};


class MCFontSizes : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCTextEvalFontSizes, EE_FONTSIZES_BADFONTNAME, PE_FONTSIZES_BADPARAM>
{
public:
    MCFontSizes(){}
    virtual ~MCFontSizes(){}
};

class MCFontStyles : public MCFunction
{
	MCExpression *fontname;
	MCExpression *fontsize;
public:
	MCFontStyles()
	{
		fontname = fontsize = NULL;
	}
	virtual ~MCFontStyles();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCFormat : public MCFunction
{
	MCParameter *params;
public:
	MCFormat()
	{
		params = NULL;
	}
	virtual ~MCFormat();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCFoundChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundChunk>
{
public:
};

class MCFoundField : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundField>
{
public:
};

class MCFoundLine : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundLine>
{
public:
};

class MCFoundLoc : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundLoc>
{
public:
};

class MCFoundText : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundText>
{
public:
};

class MCFunctionNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalFunctionNames>
{
public:
};

class MCGlobalLoc : public MCUnaryFunctionCtxt<MCPoint, MCPoint, MCInterfaceEvalGlobalLoc, EE_GLOBALLOC_NAP, PE_GLOBALLOC_BADPOINT>
{
public:
    MCGlobalLoc(){}
    virtual ~MCGlobalLoc(){}
};

class MCGlobals : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalGlobalNames>
{
public:
};

class MCHasMemory : public MCUnaryFunctionCtxt<uinteger_t, bool, MCLegacyEvalHasMemory, EE_HASMEMORY_BADAMOUNT, PE_HASMEMORY_BADPARAM>
{
public:
    MCHasMemory(){}
    virtual ~MCHasMemory(){}
};

class MCHeapSpace : public MCConstantFunctionCtxt<integer_t, MCLegacyEvalHeapSpace>
{
public:
};

class MCHostAddress : public MCUnaryFunctionCtxt<MCNameRef, MCStringRef, MCNetworkEvalHostAddress, EE_HOSTADDRESS_BADSOCKET, PE_HOSTADDRESS_BADSOCKET>
{
public:
    MCHostAddress(){}
    virtual ~MCHostAddress(){}
};

class MCHostAtoN : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCNetworkEvalHostAddressToName, EE_HOSTATON_BADADDRESS, PE_HOSTATON_BADADDRESS>
{
public:
    MCHostAtoN(){}
    virtual ~MCHostAtoN(){}
};

class MCHostName : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalHostName>
{
public:
};

class MCHostNtoA : public MCFunction
{
	MCExpression *name;
	MCExpression *message;
public:
	MCHostNtoA()
	{
		name = NULL;
		message = NULL;
	}
	virtual ~MCHostNtoA();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCInsertScripts : public MCConstantFunction
{
protected:
	Boolean front;
public:
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCBackScripts : public MCInsertScripts
{
public:
	MCBackScripts()
	{
		front = False;
	}
};

class MCFrontScripts : public MCInsertScripts
{
public:
	MCFrontScripts()
	{
		front = True;
	}
};

class MCInterrupt : public MCConstantFunctionCtxt<bool, MCEngineEvalInterrupt>
{
public:
};

class MCIntersect : public MCFunction
{
	MCChunk *o1;
	MCChunk *o2;
	// MW-2011-09-23: [[ Collision ]] The threshold value to use for alpha -> sharp conversion.
	MCExpression *threshold;
public:
	MCIntersect()
	{
		o1 = o2 = NULL;
		threshold = NULL;
	}
	virtual ~MCIntersect();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCIsoToMac : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalIsoToMac, EE_ISOTOMAC_BADSOURCE, PE_ISOTOMAC_BADPARAM>
{
public:
    MCIsoToMac(){}
    virtual ~MCIsoToMac(){}
};

class MCIsNumber : public MCUnaryFunctionCtxt<MCStringRef, bool, MCLegacyEvalIsNumber, EE_ISNUMBER_BADSOURCE, PE_ISNUMBER_BADPARAM>
{
public:
    MCIsNumber(){}
    virtual ~MCIsNumber(){}
};

class MCKeys : public MCFunction
{
	MCExpression *source;
	Properties which;
public:
	MCKeys()
	{
		source = NULL;
		which = P_UNDEFINED;
	}
	virtual ~MCKeys();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCKeysDown : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalKeysDown>
{
public:
};

class MCLength : public MCUnaryFunctionCtxt<MCStringRef, integer_t, MCStringsEvalLength, EE_LENGTH_BADSOURCE, PE_LENGTH_BADPARAM>
{
public:
    MCLength(){}
    virtual ~MCLength(){}
};

class MCLicensed : public MCConstantFunctionCtxt<bool, MCLegacyEvalLicensed>
{
	MCExpression *source;
public:
	MCLicensed()
	{
		source = NULL;
	}
	virtual ~MCLicensed();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
};

class MCLocalLoc : public MCUnaryFunctionCtxt<MCPoint, MCPoint, MCInterfaceEvalLocalLoc, EE_LOCALLOC_NAP, PE_LOCALLOC_BADPOINT>
{
public:
    MCLocalLoc(){}
    virtual ~MCLocalLoc(){}
};

class MCLocals : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalLocalNames>
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
};

class MCMachine : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalMachine>
{
public:
};

class MCMacToIso : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalMacToIso, EE_MACTOISO_BADSOURCE, PE_MACTOISO_BADPARAM>
{
public:
    MCMacToIso(){}
    virtual ~MCMacToIso(){}
};

class MCMainStacks : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMainStacks>
{
public:
};

class MCMatch : public MCFunction
{
	MCParameter *params;
protected:
	Boolean chunk;
public:
	MCMatch()
	{
		params = NULL;
	}
	virtual ~MCMatch();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCMatchChunk : public MCMatch
{
public:
	MCMatchChunk()
	{
		chunk = True;
	}
};

class MCMatchText : public MCMatch
{
public:
	MCMatchText()
	{
		chunk = False;
	}
};


class MCMe : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalMe>
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
};

class MCMenuObject : public MCConstantFunctionCtxt<MCStringRef, MCLegacyEvalMenuObject>
{
public:
};

class MCMenus : public MCConstantFunctionCtxt<MCStringRef, MCLegacyEvalMenus>
{
public:
};

class MCMerge : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCStringsEvalMerge, EE_MERGE_BADSOURCE, PE_MERGE_BADPARAM>
{
public:
    MCMerge(){}
    virtual ~MCMerge(){}
};

class MCMillisecs : public MCConstantFunctionCtxt<double, MCDateTimeEvalMilliseconds>
{
public:
};

class MCMonthNames : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalMonthNames>
{
public:
};

class MCMouse : public MCFunction
{
	MCExpression *which;
public:
	MCMouse()
	{
		which = NULL;
	}
	virtual ~MCMouse();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCMouseChar : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseChar>
{
public:
};

class MCMouseCharChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseCharChunk>
{
public:
};

class MCMouseChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseChunk>
{
public:
};

class MCMouseClick : public MCConstantFunctionCtxt<bool, MCInterfaceEvalMouseClick>
{
public:
};

class MCMouseColor : public MCConstantFunctionCtxt<MCColor, MCInterfaceEvalMouseColor>
{
public:
};

class MCMouseControl : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseControl>
{
public:
};

class MCMouseH : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalMouseH>
{
public:
};

class MCMouseLine : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseLine>
{
public:
};

class MCMouseLoc : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseLoc>
{
public:
};

class MCMouseStack : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseStack>
{
public:
};

class MCMouseText : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseText>
{
public:
};

class MCMouseV : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalMouseV>
{
public:
};

class MCMovie : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalMovie>
{
public:
};

class MCMovingControls : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMovingControls>
{
public:
};

class MCNativeCharToNum : public MCUnaryFunctionCtxt<MCStringRef, uinteger_t, MCStringsEvalNativeCharToNum, EE_CHARTONUM_BADSOURCE, PE_CHARTONUM_BADPARAM> // FIXME
{
public:
    MCNativeCharToNum(){}
    virtual ~MCNativeCharToNum(){}
};

class MCNumToChar: public MCUnaryFunctionCtxt<uinteger_t, MCValueRef, MCStringsEvalNumToChar, EE_NUMTOCHAR_BADSOURCE, PE_NUMTOCHAR_BADPARAM>
{
public:
    MCNumToChar(){}
    virtual ~MCNumToChar(){}
};

class MCNumToNativeChar : public MCUnaryFunctionCtxt<uinteger_t, MCStringRef, MCStringsEvalNumToNativeChar,
    EE_NUMTOCHAR_BADSOURCE, PE_NUMTOCHAR_BADPARAM> // FIXME
{
public:
    MCNumToNativeChar(){}
    virtual ~MCNumToNativeChar(){}
};

class MCNumToUnicodeChar : public MCUnaryFunctionCtxt<uinteger_t, MCStringRef, MCStringsEvalNumToUnicodeChar,
    EE_NUMTOCHAR_BADSOURCE, PE_NUMTOCHAR_BADPARAM> // FIXME
{
public:
    MCNumToUnicodeChar(){}
    virtual ~MCNumToUnicodeChar(){}
};

// AL-2014-10-21: [[ Bug 13740 ]] numToByte should return a DataRef
class MCNumToByte: public MCUnaryFunctionCtxt<integer_t, MCDataRef, MCStringsEvalNumToByte, EE_NUMTOBYTE_BADSOURCE, PE_NUMTOBYTE_BADPARAM>
{
public:
    MCNumToByte(void){}
    virtual ~MCNumToByte(){}
};

class MCOpenFiles : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalOpenFiles>
{
public:
};

class MCOpenProcesses : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalOpenProcesses>
{
public:
};

class MCOpenProcessIds : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalOpenProcessesIds>
{
public:
};

class MCOpenSockets : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalOpenSockets>
{
public:
};

class MCOpenStacks : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalOpenStacks>
{
public:
};

class MCOptionKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalOptionKey>
{
public:
};

class MCParam : public MCUnaryFunctionCtxt<integer_t, MCValueRef, MCEngineEvalParam, EE_PARAM_BADINDEX, PE_PARAM_BADPARAM>
{
public:
    MCParam(){}
    virtual ~MCParam(){}
};

class MCParamCount : public MCConstantFunctionCtxt<integer_t, MCEngineEvalParamCount>
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
};

class MCParams : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalParams>
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
};

class MCPeerAddress : public MCUnaryFunctionCtxt<MCNameRef, MCStringRef, MCNetworkEvalPeerAddress, EE_HOSTADDRESS_BADSOCKET, PE_PEERADDRESS_BADSOCKET>
{
public:
    MCPeerAddress(){}
    virtual ~MCPeerAddress(){}
};

class MCPendingMessages : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalPendingMessages>
{
public:
};

class MCPid : public MCConstantFunctionCtxt<integer_t, MCFilesEvalProcessId>
{
public:
};

class MCPlatform : public MCConstantFunctionCtxt<MCNameRef, MCEngineEvalPlatform>
{
public:
};


// JS-2013-06-19: [[ StatsFunctions ]] Definition of populationStdDev
class MCPopulationStdDev : public MCParamFunctionCtxt<MCMathEvalPopulationStdDev, EE_POP_STDDEV_BADSOURCE, PE_POP_STDDEV_BADPARAM>
{
public:
    MCPopulationStdDev(){}
    virtual ~MCPopulationStdDev(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of populationVariance
class MCPopulationVariance : public MCParamFunctionCtxt<MCMathEvalPopulationVariance, EE_POP_VARIANCE_BADSOURCE, PE_POP_VARIANCE_BADPARAM>
{
public:
    MCPopulationVariance(){}
    virtual ~MCPopulationVariance(){}
};

class MCProcessor : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalProcessor>
{
public:
};

class MCPropertyNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalPropertyNames>
{
public:
};

class MCQTVersion : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalQTVersion>
{
public:
};

class MCReplaceText : public MCFunction
{
	MCExpression *source;
	MCExpression *pattern;
	MCExpression *replacement;
public:
	MCReplaceText()
	{
		source = pattern = replacement = NULL;
	}
	virtual ~MCReplaceText();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCTheResult : public MCConstantFunctionCtxt<MCValueRef, MCEngineEvalResult>
{
public:
};

class MCScreenColors : public MCConstantFunctionCtxt<double, MCInterfaceEvalScreenColors>
{
public:
};

class MCScreenDepth : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalScreenDepth>
{
public:
};

class MCScreenLoc : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalScreenLoc>
{
public:
};

class MCScreenName : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalScreenName>
{
public:
};

class MCScreenRect : public MCFunction
{
	bool f_plural;
public:
	MCScreenRect(bool p_plural)
		: f_plural(p_plural)
	{
	}

	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCScreenType : public MCConstantFunctionCtxt<MCNameRef, MCLegacyEvalScreenType>
{
public:
};

class MCScreenVendor : public MCConstantFunctionCtxt<MCNameRef, MCLegacyEvalScreenVendor>
{
public:
};

class MCScriptLimits : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalScriptLimits>
{
public:
};

class MCSeconds : public MCConstantFunctionCtxt<double, MCDateTimeEvalSeconds>
{
public:
};

class MCSelectedButton : public MCFunction
{
	MCExpression *family;
	MCChunk *object;
	Boolean bg;
public:
	MCSelectedButton()
	{
		family = NULL;
		object = NULL;
		bg = False;
	}
	virtual ~MCSelectedButton();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCSelectedChunk : public MCFunction
{
	MCChunk *object;
public:
	MCSelectedChunk()
	{
		object = NULL;
	}
	virtual ~MCSelectedChunk();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCSelectedField : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalSelectedField>
{
public:
};

class MCSelectedImage : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalSelectedImage>
{
public:
};

class MCSelectedLine : public MCFunction
{
	MCChunk *object;
public:
	MCSelectedLine()
	{
		object = NULL;
	}
	virtual ~MCSelectedLine();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCSelectedLoc : public MCFunction
{
	MCChunk *object;
public:
	MCSelectedLoc()
	{
		object = NULL;
	}
	virtual ~MCSelectedLoc();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCSelectedObject : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalSelectedObject>
{
public:
};

class MCSelectedText : public MCFunction
{
	MCChunk *object;
public:
	MCSelectedText()
	{
		object = NULL;
	}
	virtual ~MCSelectedText();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCShell : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalShell, EE_SHELL_BADSOURCE, PE_SHELL_BADPARAM>
{
public:
    MCShell(){}
    virtual ~MCShell(){}
};

class MCShiftKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalShiftKey>
{
public:
};

class MCSound : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalSound>
{
public:
};

class MCStacks : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalStacks>
{
public:
};

class MCStackSpace : public MCConstantFunctionCtxt<integer_t, MCLegacyEvalStackSpace>
{
public:
};

class MCSysError : public MCConstantFunctionCtxt<uinteger_t, MCEngineEvalSysError>
{
public:
};

class MCSystemVersion : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalSystemVersion>
{
public:
};

class MCTarget : public MCConstantFunction
{
	Boolean contents;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

// MW-2008-11-05: [[ Owner Reference ]] The owner function syntax class.
class MCOwner : public MCFunction
{
	MCChunk *object;
public:
	MCOwner(void)
	{
		object = NULL;
	}
	~MCOwner(void);
	virtual Parse_stat parse(MCScriptPoint&, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCTempName : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalTempName>
{
public:
};

class MCTextDecode : public MCFunction
{
    MCExpression *m_data;
    MCExpression *m_encoding;
public:
    MCTextDecode()
    {
        m_data = m_encoding = NULL;
    }
    virtual ~MCTextDecode();
    virtual Parse_stat parse(MCScriptPoint&, Boolean the);
    virtual void eval_ctxt(MCExecContext&, MCExecValue&);
};

class MCTextEncode : public MCFunction
{
    MCExpression *m_string;
    MCExpression *m_encoding;
public:
    MCTextEncode()
    {
        m_string = m_encoding = NULL;
    }
    virtual ~MCTextEncode();
    virtual Parse_stat parse(MCScriptPoint&, Boolean the);
    virtual void eval_ctxt(MCExecContext&, MCExecValue&);
};

class MCNormalizeText : public MCFunction
{
    MCExpression *m_text;
    MCExpression *m_form;
public:
    MCNormalizeText()
    {
        m_text = m_form = NULL;
    }
    virtual ~MCNormalizeText();
    virtual Parse_stat parse(MCScriptPoint&, Boolean the);
    virtual void eval_ctxt(MCExecContext&, MCExecValue&);
};


class MCCodepointProperty : public MCFunction
{
    MCExpression *m_codepoint;
    MCExpression *m_property;
public:
    MCCodepointProperty()
    {
        m_codepoint = m_property = NULL;
    }
    virtual ~MCCodepointProperty();
    virtual Parse_stat parse(MCScriptPoint&, Boolean the);
    virtual void eval_ctxt(MCExecContext&, MCExecValue&);
};

class MCTicks : public MCConstantFunctionCtxt<double, MCDateTimeEvalTicks>
{
public:
};

class MCTheTime : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalTime>
{
public:
};

class MCToLower : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCStringsEvalToLower, EE_TOLOWER_BADSOURCE, PE_TOLOWER_BADPARAM>
{
public:
    MCToLower(){}
    virtual ~MCToLower(){}
};

class MCToUpper : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCStringsEvalToUpper, EE_TOUPPER_BADSOURCE, PE_TOUPPER_BADPARAM>
{
public:
    MCToUpper(){}
    virtual ~MCToUpper(){}
};

class MCTopStack : public MCFunction
{
	MCExpression *which;
public:
	MCTopStack()
	{
		which = NULL;
	}
	virtual ~MCTopStack();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCUnicodeCharToNum : public MCUnaryFunctionCtxt<MCStringRef, uinteger_t, MCStringsEvalUnicodeCharToNum,
    EE_CHARTONUM_BADSOURCE, PE_CHARTONUM_BADPARAM> // FIXME
{
public:
    MCUnicodeCharToNum(){}
    virtual ~MCUnicodeCharToNum(){}
};

class MCUniDecode : public MCFunction
{
	MCExpression *source;
	MCExpression *language;
public:
	MCUniDecode()
	{
		source = NULL;
		language = NULL;
	}
	virtual ~MCUniDecode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCUniEncode : public MCFunction
{
	MCExpression *source;
	MCExpression *language;
public:
	MCUniEncode()
	{
		source = NULL;
		language = NULL;
	}
	virtual ~MCUniEncode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCUrlDecode : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFiltersEvalUrlDecode, EE_URLDECODE_BADSOURCE, PE_URLDECODE_BADPARAM>
{
public:
    MCUrlDecode(){}
    virtual ~MCUrlDecode(){}
};

class MCUrlEncode : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFiltersEvalUrlEncode, EE_URLENCODE_BADSOURCE, PE_URLENCODE_BADPARAM>
{
public:
    MCUrlEncode(){}
    virtual ~MCUrlEncode(){}
};

class MCUrlStatus : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCNetworkEvalUrlStatus, EE_URLSTATUS_BADSOURCE, PE_URLSTATUS_BADPARAM>
{
public:
    MCUrlStatus(){}
    virtual ~MCUrlStatus(){}
};

class MCValue : public MCFunction
{
	MCExpression *source;
	MCChunk *object;
public:
	MCValue()
	{
		source = NULL;
		object = NULL;
	}
	virtual ~MCValue();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCVariables : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalVariableNames>
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
};

class MCVersion : public MCConstantFunctionCtxt<MCNameRef, MCEngineEvalVersion>
{
public:
};

class MCWaitDepth : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalWaitDepth>
{
public:
};

class MCWeekDayNames : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalWeekDayNames>
{
public:
};

class MCWithin : public MCFunction
{
	MCChunk *object;
	MCExpression *point;
public:
	MCWithin()
	{
		object = NULL;
		point = NULL;
	}
	virtual ~MCWithin();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
};

// platform specific functions in funcs.cpp

class MCMCISendString : public MCFunction
{
	MCExpression *string;
public:
	MCMCISendString()
	{
		string = NULL;
	}
	virtual ~MCMCISendString();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
    virtual void eval_ctxt(MCExecContext& ctxt, MCExecValue &r_value);
};

class MCDeleteRegistry : public MCUnaryFunctionCtxt<MCStringRef, bool, MCFilesEvalDeleteRegistry, EE_SETREGISTRY_BADEXP, PE_SETREGISTRY_BADPARAM>
{
public:
    MCDeleteRegistry(){}
    virtual ~MCDeleteRegistry(){}
};

class MCListRegistry : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalListRegistry, EE_SETREGISTRY_BADEXP, PE_SETREGISTRY_BADPARAM>
{
public:
    MCListRegistry(){}
    virtual ~MCListRegistry(){}
};

class MCQueryRegistry : public MCFunction
{
	MCExpression *key;
	MCExpression *type;
public:
	MCQueryRegistry()
	{
		key = type = NULL;
	}
	virtual ~MCQueryRegistry();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCSetRegistry : public MCFunction
{
	MCExpression *key;
	MCExpression *value;
	MCExpression *type;
public:
	MCSetRegistry()
	{
		key = value = type = NULL;
	}
	virtual ~MCSetRegistry();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCCopyResource : public MCFunction
{
	MCExpression *source;
	MCExpression *dest;
	MCExpression *type;
	MCExpression *name;
	MCExpression *newid;
public:
	MCCopyResource()
	{
		source = dest = type = name = newid = NULL;
	}
	virtual ~MCCopyResource();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCDeleteResource : public MCFunction
{
	MCExpression *source;
	MCExpression *type;
	MCExpression *name;
public:
	MCDeleteResource()
	{
		source = type = name = NULL;
	}
	virtual ~MCDeleteResource();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCGetResource : public MCFunction
{
	MCExpression *source;
	MCExpression *type;
	MCExpression *name;
public:
	MCGetResource()
	{
		source = type = name = NULL;
	}
	virtual ~MCGetResource();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCGetResources : public MCFunction
{
	MCExpression *source;
	MCExpression *type;
public:
	MCGetResources()
	{
		source = type = NULL;
	}
	virtual ~MCGetResources();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &e);
};

class MCSetResource : public MCFunction
{
	MCExpression *source;
	MCExpression *type;
	MCExpression *id;
	MCExpression *name;
	MCExpression *flags;
	MCExpression *value;
public:
	MCSetResource()
	{
		source = type = id = name = flags = value = NULL;
	}
	virtual ~MCSetResource();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCSpecialFolderPath : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalSpecialFolderPath, EE_SPECIALFOLDERPATH_BADPARAM, PE_SPECIALFOLDERPATH_BADTYPE>
{
public:
    MCSpecialFolderPath(){}
    virtual ~MCSpecialFolderPath(){}
};

class MCShortFilePath : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalShortFilePath, EE_SHORTFILEPATH_BADSOURCE, PE_SHORTFILEPATH_BADPARAM>
{
public:
    MCShortFilePath(){}
    virtual ~MCShortFilePath(){}
};

class MCLongFilePath : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalLongFilePath, EE_LONGFILEPATH_BADSOURCE, PE_LONGFILEPATH_BADPARAM>
{
public:
    MCLongFilePath(){}
    virtual ~MCLongFilePath(){}
};

class MCAlternateLanguages : public MCConstantFunctionCtxt<MCStringRef, MCScriptingEvalAlternateLanguages>
{
public:
};

class MCAliasReference: public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalAliasReference, EE_ALIASREFERENCE_BADSOURCE, PE_ALIASREFERENCE_BADPARAM>
{
public:
    MCAliasReference(){}
    virtual ~MCAliasReference(){}
};

class MCCipherNames : public MCConstantFunctionCtxt<MCStringRef, MCSecurityEvalCipherNames>
{
public:
};

// Math functions in funcsm.cpp

class MCAbsFunction : public MCUnaryFunctionCtxt<double, double, MCMathEvalAbs, EE_ABS_BADSOURCE, PE_ABS_BADPARAM>
{
public:
	MCAbsFunction(){}
    virtual ~MCAbsFunction(){}
};

class MCAcos : public MCUnaryFunctionCtxt<double, double, MCMathEvalAcos, EE_ACOS_BADSOURCE, PE_ACOS_BADPARAM>
{
	public:
	MCAcos(){}	
	virtual ~MCAcos(){}
};

class MCAnnuity : public MCFunction
{
	MCExpression *rate;
	MCExpression *periods;
public:
	MCAnnuity()
	{
		rate = periods = NULL;
	}
	virtual ~MCAnnuity();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of arithmeticMean
class MCArithmeticMean : public MCParamFunctionCtxt<MCMathEvalArithmeticMean, EE_AVERAGE_BADSOURCE, PE_AVERAGE_BADPARAM>
{
public:
    MCArithmeticMean(){}
    virtual ~MCArithmeticMean(){}
};

class MCAsin : public MCUnaryFunctionCtxt<double, double, MCMathEvalAsin, EE_ASIN_BADSOURCE, PE_ASIN_BADPARAM>
{
public:
	MCAsin(){}
	virtual ~MCAsin(){}
};

class MCAtan : public MCUnaryFunctionCtxt<double, double, MCMathEvalAtan, EE_ATAN_BADSOURCE, PE_ATAN_BADPARAM>
{
public:
	MCAtan(){}
	virtual ~MCAtan(){}
};

class MCAtan2 : public MCFunction
{
	MCExpression *s1;
	MCExpression *s2;
public:
	MCAtan2()
	{
		s1 = s2 = NULL;
	}
	virtual ~MCAtan2();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of averageDev (was average)
class MCAvgDev : public MCParamFunctionCtxt<MCMathEvalAverageDeviation, EE_AVERAGE_BADSOURCE, PE_AVERAGE_BADPARAM>
{
public:
    MCAvgDev(){}
    virtual ~MCAvgDev(){}
};

class MCCompound : public MCFunction
{
	MCExpression *rate;
	MCExpression *periods;
public:
	MCCompound()
	{
		rate = periods = NULL;
	}
	virtual ~MCCompound();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCCos : public MCUnaryFunctionCtxt<double, double, MCMathEvalCos, EE_COS_BADSOURCE, PE_COS_BADPARAM>
{
public:
	MCCos(){}
	virtual ~MCCos(){}
};

class MCExp : public MCUnaryFunctionCtxt<double, double, MCMathEvalExp, EE_EXP_BADSOURCE, PE_EXP_BADPARAM>
{
public:
	MCExp(){}
	virtual ~MCExp(){}
};

class MCExp1 : public MCUnaryFunctionCtxt<double, double, MCMathEvalExp1, EE_EXP1_BADSOURCE, PE_EXP1_BADPARAM>
{
public:
	MCExp1(){}
	virtual ~MCExp1(){}
};

class MCExp2 : public MCUnaryFunctionCtxt<double, double, MCMathEvalExp2, EE_EXP2_BADSOURCE, PE_EXP2_BADPARAM>
{
public:
	MCExp2(){}
	virtual ~MCExp2(){}
};

class MCExp10 : public MCUnaryFunctionCtxt<double, double, MCMathEvalExp10, EE_EXP10_BADSOURCE, PE_EXP10_BADPARAM>
{
public:
	MCExp10(){}
	virtual ~MCExp10(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of geometricMean
class MCGeometricMean : public MCParamFunctionCtxt<MCMathEvalGeometricMean, EE_GEO_MEAN_BADSOURCE, PE_GEO_MEAN_BADPARAM>
{
public:
    MCGeometricMean(){}
    virtual ~MCGeometricMean(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of harmonicMean
class MCHarmonicMean : public MCParamFunctionCtxt<MCMathEvalHarmonicMean, EE_HAR_MEAN_BADSOURCE, PE_HAR_MEAN_BADPARAM>
{
public:
    MCHarmonicMean(){}
    virtual ~MCHarmonicMean(){}
};

class MCLn : public MCUnaryFunctionCtxt<double, double, MCMathEvalLn, EE_LN_BADSOURCE, PE_LN_BADPARAM>
{
public:
	MCLn(){}
	virtual ~MCLn(){}
};

class MCLn1 : public MCUnaryFunctionCtxt<double, double, MCMathEvalLn1, EE_LN1_BADSOURCE, PE_LN1_BADPARAM>
{
public:
	MCLn1(){}
	virtual ~MCLn1(){}
};

class MCLog2 : public MCUnaryFunctionCtxt<double, double, MCMathEvalLog2, EE_LOG2_BADSOURCE, PE_LOG2_BADPARAM>
{
public:
	MCLog2(){}
	virtual ~MCLog2(){}
};

class MCLog10 : public MCUnaryFunctionCtxt<double, double, MCMathEvalLog10, EE_LOG10_BADSOURCE, PE_LOG10_BADPARAM>
{
public:
	MCLog10(){}
	virtual ~MCLog10(){}
};

class MCMatrixMultiply : public MCFunction
{
	MCExpression *dest;
	MCExpression *source;
public:
	MCMatrixMultiply()
	{
		source = NULL;
		dest = NULL;
	}
	virtual ~MCMatrixMultiply();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCVectorDotProduct : public MCFunction
{
	MCExpression *first;
	MCExpression *second;
public:
	MCVectorDotProduct(void)
	{
		first = NULL;
		second = NULL;
	}
	virtual ~MCVectorDotProduct();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCMaxFunction : public MCParamFunctionCtxt<MCMathEvalMax, EE_MAX_BADSOURCE, PE_MAX_BADPARAM>
{
public:
    MCMaxFunction(){}
    virtual ~MCMaxFunction(){}
};

class MCMedian : public MCParamFunctionCtxt<MCMathEvalMedian, EE_MEDIAN_BADSOURCE, PE_MEDIAN_BADPARAM>
{
public:
    MCMedian(){}
    virtual ~MCMedian(){}
};

class MCMD5Digest : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalMD5Digest, EE_MD5DIGEST_BADSOURCE, PE_MD5DIGEST_BADPARAM>
{
public:
	MCMD5Digest(){}
	virtual ~MCMD5Digest(){}
};

class MCSHA1Digest : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalSHA1Digest, EE_SHA1DIGEST_BADSOURCE, PE_SHA1DIGEST_BADPARAM>
{
public:
	MCSHA1Digest(){}
	virtual ~MCSHA1Digest(){}
};

class MCMessageDigestFunc: public MCFunction
{
	MCAutoPointer<MCExpression> m_data;
	MCAutoPointer<MCExpression> m_type;

public:
	virtual ~MCMessageDigestFunc(void) {};
	virtual Parse_stat parse(MCScriptPoint &sp, Boolean the);
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
};

class MCMinFunction : public MCParamFunctionCtxt<MCMathEvalMin, EE_MIN_BADSOURCE, PE_MIN_BADPARAM>
{
public:
    MCMinFunction(){}
    virtual ~MCMinFunction(){}
};

class MCRandom : public MCUnaryFunctionCtxt<double, double, MCMathEvalRandom, EE_RANDOM_BADSOURCE, PE_RANDOM_BADPARAM>
{
public:
	MCRandom(){}
	virtual ~MCRandom(){}
};

class MCRound : public MCFunction
{
	MCExpression *source;
	MCExpression *digit;
public:
	MCRound()
	{
		source = digit = NULL;
	}
	virtual ~MCRound();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
};

class MCSin : public MCUnaryFunctionCtxt<double, double, MCMathEvalSin, EE_SIN_BADSOURCE, PE_SIN_BADPARAM>
{
public:
	MCSin(){}
	virtual ~MCSin(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of sampleStdDev (was stdDev)
class MCSampleStdDev : public MCParamFunctionCtxt<MCMathEvalSampleStdDev, EE_STDDEV_BADSOURCE, PE_STDDEV_BADPARAM>
{
public:
    MCSampleStdDev(){}
    virtual ~MCSampleStdDev(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of sampleVariance
class MCSampleVariance : public MCParamFunctionCtxt<MCMathEvalSampleVariance, EE_VARIANCE_BADSOURCE, PE_VARIANCE_BADPARAM>
{
public:
    MCSampleVariance(){}
    virtual ~MCSampleVariance(){}
};

class MCSqrt : public MCUnaryFunctionCtxt<double, double, MCMathEvalSqrt, EE_SQRT_BADSOURCE, PE_SQRT_BADPARAM>
{
public:
	MCSqrt(){}
	virtual ~MCSqrt(){}
};

class MCStatRound : public MCFunction
{
	MCExpression *source;
	MCExpression *digit;
public:
	MCStatRound()
	{
		source = digit = NULL;
	}
	virtual ~MCStatRound();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCStdDev : public MCParamFunctionCtxt<MCMathEvalSampleStdDev, EE_STDDEV_BADSOURCE, PE_STDDEV_BADPARAM>
{
public:
    MCStdDev(){}
    virtual ~MCStdDev(){}
};

class MCSum : public MCParamFunctionCtxt<MCMathEvalSum, EE_SUM_BADSOURCE, PE_SUM_BADPARAM>
{
public:
    MCSum(){}
    virtual ~MCSum(){}
};

class MCTan : public MCUnaryFunctionCtxt<double, double, MCMathEvalTan, EE_TAN_BADSOURCE, PE_TAN_BADPARAM>
{
public:
	MCTan(){}
	virtual ~MCTan(){}
};

class MCTextHeightSum : public MCFunction
{
	MCChunk *object;
public:
	MCTextHeightSum()
	{
		object = NULL;
	}
	virtual ~MCTextHeightSum();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCTranspose : public MCUnaryFunctionCtxt<MCArrayRef, MCArrayRef, MCArraysEvalTransposeMatrix, EE_TRANSPOSE_BADSOURCE, PE_TRANSPOSE_BADPARAM>
{
public:
	MCTranspose(){}
	virtual ~MCTranspose(){}
};

class MCTrunc : public MCUnaryFunctionCtxt<double, double, MCMathEvalTrunc, EE_TRUNC_BADSOURCE, PE_TRUNC_BADPARAM>
{
public:
    MCTrunc(){}
    virtual ~MCTrunc(){}
};

// MDW-2014-08-23 : [[ feature_floor ]]
class MCFloor : public MCUnaryFunctionCtxt<double, double, MCMathEvalFloor, EE_FLOOR_BADSOURCE, PE_FLOOR_BADPARAM>
{
public:
	MCFloor(){}
	virtual ~MCFloor(){}
};

class MCCeil : public MCUnaryFunctionCtxt<double, double, MCMathEvalCeil, EE_CEIL_BADSOURCE, PE_CEIL_BADPARAM>
{
public:
	MCCeil(){}
	virtual ~MCCeil(){}
};
// MDW-2014-08-23 : [[ feature_floor ]]

class MCHTTPProxyForURL: public MCFunction
{
	MCExpression *url;
	MCExpression *host;
	MCExpression *pac;

public:
	MCHTTPProxyForURL(void)
	{
		url = NULL;
		host = NULL;
		pac = NULL;
	}

	virtual ~MCHTTPProxyForURL(void);

	virtual Parse_stat parse(MCScriptPoint& sp, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
};

class MCRandomBytes: public MCUnaryFunctionCtxt<uinteger_t, MCDataRef, MCSecurityEvalRandomBytes, EE_RANDOMBYTES_BADCOUNT, PE_RANDOMBYTES_BADPARAM>
{
public:
    MCRandomBytes(void){}
    virtual ~MCRandomBytes(void){}
};

// MW-2012-10-08: [[ HitTest ]] controlAtLoc and controlAtScreenLoc function.
class MCControlAtLoc: public MCUnaryFunction
{
    MCExpression *location;
	bool is_screen : 1;
	
public:
    MCControlAtLoc(bool p_is_screen)
    {
        location = NULL;
		is_screen = p_is_screen;
	}
	
    virtual ~MCControlAtLoc();
    Parse_stat parse(MCScriptPoint &sp, Boolean the);
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
};

// MW-20113-05-08: [[ Uuid ]] The uuid generation function.
class MCUuidFunc: public MCFunction
{
	MCExpression *type;
	MCExpression *namespace_id;
	MCExpression *name;
	
public:
	MCUuidFunc(void)
	{
		type = nil;
		namespace_id = nil;
		name = nil;
	}
	
	virtual ~MCUuidFunc(void);
	virtual Parse_stat parse(MCScriptPoint &sp, Boolean the);
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
};

// MERG-2013-08-14: [[ MeasureText ]] Measure text relative to the effective font on an object
class MCMeasureText: public MCFunction
{
    MCChunk *m_object;
    MCExpression *m_text;
    MCExpression *m_mode;
    bool m_is_unicode;
    
public:
    MCMeasureText(bool p_is_unicode)
    {
        m_object = nil;
        m_text = nil;
        m_mode = nil;
        m_is_unicode = p_is_unicode;
    }
    
    virtual ~MCMeasureText(void);
	virtual Parse_stat parse(MCScriptPoint &sp, Boolean the);
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
};

#endif



