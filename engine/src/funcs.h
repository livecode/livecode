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

////////////////////////////////////////////////////////////////////////////////

class MCFunction : public MCExpression
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	Parse_stat parsetarget(MCScriptPoint &spt, Boolean the,
	                       Boolean needone, MCChunk *&chunk);
    bool params_to_doubles(MCExecContext& ctxt, MCParameter *p_params, real64_t*& r_doubles, uindex_t& r_count);
	
	// General method for compiling a function that maps to a single method.
	// The variadic argument list should be the MCExpression's the function
	// takes as arguments.
	virtual void compile_with_args(MCSyntaxFactoryRef ctxt, MCExecMethodInfo *method, ...);
};

// Helper class that simplifies compiling of functions not taking any arguments.
class MCConstantFunction: public MCFunction
{
public:
	virtual MCExecMethodInfo *getmethodinfo(void) const = 0;
	virtual void compile(MCSyntaxFactoryRef ctxt);
};

// Helper class that simplifies compiling of functions taking one arguments.
class MCUnaryFunction: public MCFunction
{
public:
	virtual MCExecMethodInfo *getmethodinfo(void) const = 0;
	virtual MCExpression *getmethodarg(void) const = 0;
    virtual void compile(MCSyntaxFactoryRef ctxt);
};

// Helper class that simplifies compiling of functions taking a variable number of of MCParameters.
class MCParamFunction: public MCFunction
{
public:
	virtual MCExecMethodInfo *getmethodinfo(void) const = 0;
	virtual MCParameter *getmethodarg(void) const = 0;
	virtual void compile(MCSyntaxFactoryRef ctxt);

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
         Parse_errors ParseError,
         MCExecMethodInfo*& MethodInfo>
class MCUnaryFunctionCtxt: public MCUnaryFunction
{
public:
    MCUnaryFunctionCtxt() { m_expression = nil; }

    virtual ~MCUnaryFunctionCtxt() { delete m_expression; }

    virtual Parse_stat parse(MCScriptPoint &sp, Boolean the)
    {
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

    virtual MCExecMethodInfo *getmethodinfo(void) const { return MethodInfo; }
    virtual MCExpression *getmethodarg(void) const { return m_expression; }

protected:
    MCExpression *m_expression;
};

template<void (*EvalExprMethod)(MCExecContext &, real64_t*, uindex_t, real64_t&),
         Exec_errors EvalError,
         Parse_errors ParseError,
         MCExecMethodInfo*& MethodInfo>
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

    virtual MCExecMethodInfo *getmethodinfo(void) const { return MethodInfo; }
    virtual MCParameter *getmethodarg(void) const { return params; }
};

////////////////////////////////////////////////////////////////////////////////

class MCArrayDecode: public MCUnaryFunctionCtxt<MCDataRef, MCArrayRef, MCArraysEvalArrayDecode, EE_ARRAYDECODE_BADSOURCE, PE_ARRAYDECODE_BADPARAM, kMCArraysEvalArrayDecodeMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCBase64Decode : public MCUnaryFunctionCtxt<MCStringRef, MCDataRef, MCFiltersEvalBase64Decode, EE_BASE64DECODE_BADSOURCE, PE_BASE64DECODE_BADPARAM, kMCFiltersEvalBase64DecodeMethodInfo>
{
public:
    MCBase64Decode() {}
    virtual ~MCBase64Decode(){}
};

class MCBase64Encode : public MCUnaryFunctionCtxt<MCDataRef, MCStringRef, MCFiltersEvalBase64Encode, EE_BASE64ENCODE_BADSOURCE, PE_BASE64ENCODE_BADPARAM, kMCFiltersEvalBase64EncodeMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

// AL-2014-10-17: [[ BiDi ]] Returns the result of applying the bi-directional algorithm to text
class MCBidiDirection : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCStringsEvalBidiDirection, EE_BIDIDIRECTION_BADSOURCE, PE_BIDIDIRECTION_BADPARAM, kMCStringsEvalBidiDirectionMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCBuildNumber : public MCConstantFunctionCtxt<integer_t, MCEngineEvalBuildNumber>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalBuildNumberMethodInfo; }
};

class MCCachedUrls : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalCachedUrls>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalCachedUrlsMethodInfo; }
};

class MCCapsLockKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalCapsLockKey>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalCapsLockKeyMethodInfo; }
};

class MCCharToNum : public MCUnaryFunctionCtxt<MCValueRef, MCValueRef, MCStringsEvalCharToNum, EE_CHARTONUM_BADSOURCE, PE_CHARTONUM_BADPARAM, kMCStringsEvalCharToNumMethodInfo>
{
public:
    MCCharToNum(){}
    virtual ~MCCharToNum(){}
};

class MCByteToNum : public MCUnaryFunctionCtxt<MCStringRef, integer_t, MCStringsEvalByteToNum, EE_BYTETONUM_BADSOURCE, PE_BYTETONUM_BADPARAM, kMCStringsEvalByteToNumMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCClipboardFunc : public MCConstantFunctionCtxt<MCNameRef, MCPasteboardEvalClipboard>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalClipboardMethodInfo; }
};

class MCCompress : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalCompress, EE_COMPRESS_BADSOURCE, PE_COMPRESS_BADPARAM, kMCFiltersEvalCompressMethodInfo>
{
public:
    MCCompress(){}
    virtual ~MCCompress(){}
};

class MCConstantNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalConstantNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalConstantNamesMethodInfo; }

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
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickCharMethodInfo; }
};

class MCClickCharChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickCharChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickCharChunkMethodInfo; }
};

class MCClickChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickChunkMethodInfo; }
};

class MCClickField : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickField>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickFieldMethodInfo; }
};

class MCClickH : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalClickH>
{
public:
	//virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickHMethodInfo; }
};

class MCClickLine : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickLine>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickLineMethodInfo; }
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
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickLocMethodInfo; }
};

class MCClickStack : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickStack>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickStackMethodInfo; }
};

class MCClickText : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickText>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickTextMethodInfo; }
};

class MCClickV : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalClickV>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickVMethodInfo; }
};

class MCColorNames : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalColorNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalColorNamesMethodInfo; }

};

class MCCommandNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalCommandNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalCommandNamesMethodInfo; }

};

class MCCommandKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalCommandKey>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalCommandKeyMethodInfo; }
};

class MCControlKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalControlKey>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalControlKeyMethodInfo; }
};

class MCDate : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalDate>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalDateMethodInfo; }

};

class MCDateFormat : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalDateFormat>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalDateFormatMethodInfo; }

};

class MCDecompress : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalDecompress, EE_DECOMPRESS_BADSOURCE, PE_DECOMPRESS_BADPARAM, kMCFiltersEvalDecompressMethodInfo>
{
public:
    MCDecompress(){}
    virtual ~MCDecompress(){}
};

class MCDirectories : public MCFunction
{
public:
	MCDirectories() : m_folder(nil) {}
	virtual ~MCDirectories();
	virtual Parse_stat parse(MCScriptPoint &, Boolean p_is_the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
	virtual void compile(MCSyntaxFactoryRef);
private:
	MCExpression *m_folder;
};

class MCDiskSpace : public MCConstantFunctionCtxt<double, MCFilesEvalDiskSpace>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDiskSpaceMethodInfo; }

};

class MCDNSServers : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalDNSServers>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalDNSServersMethodInfo; }

};

class MCDragDestination: public MCConstantFunctionCtxt<MCStringRef, MCPasteboardEvalDragDestination>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalDragDestinationMethodInfo; }

};

class MCDragSource: public MCConstantFunctionCtxt<MCStringRef, MCPasteboardEvalDragSource>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalDragSourceMethodInfo; }

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
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDriverNamesMethodInfo; }

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
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDrivesMethodInfo; }

};

class MCDropChunk: public MCConstantFunctionCtxt<MCStringRef, MCPasteboardEvalDropChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalDropChunkMethodInfo; }

};

class MCQTEffects : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalQTEffects>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalQTEffectsMethodInfo; }

};

class MCRecordCompressionTypes: public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalRecordCompressionTypes>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalRecordCompressionTypesMethodInfo; }

};

class MCRecordLoudness : public MCConstantFunctionCtxt<integer_t, MCMultimediaEvalRecordLoudness>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalRecordLoudnessMethodInfo; }

};

class MCEnvironment : public MCConstantFunctionCtxt<MCNameRef, MCEngineEvalEnvironment>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalEnvironmentMethodInfo; }

};

class MCEncrypt : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCSecurityEvalEncrypt, EE_ENCRYPT_BADSOURCE, PE_ENCRYPT_BADPARAM, kMCSecurityEvalEncryptMethodInfo>
{
public:
    MCEncrypt(){}
    virtual ~MCEncrypt(){}
};

class MCEventCapsLockKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalEventCapsLockKey>
{
public:
    virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalEventCapsLockKeyMethodInfo; }
};

class MCEventCommandKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalEventCommandKey>
{
public:
    virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalEventCommandKeyMethodInfo; }
};

class MCEventControlKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalEventControlKey>
{
public:
    virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalEventControlKeyMethodInfo; }
};

class MCEventOptionKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalEventOptionKey>
{
public:
    virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalEventOptionKeyMethodInfo; }
};

class MCEventShiftKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalEventShiftKey>
{
public:
    virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalEventShiftKeyMethodInfo; }
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

	virtual void compile(MCSyntaxFactoryRef);
};

class MCExtents : public MCUnaryFunctionCtxt<MCArrayRef, MCStringRef, MCArraysEvalExtents, EE_EXTENTS_BADSOURCE, PE_EXTENTS_BADPARAM, kMCArraysEvalExtentsMethodInfo>
{
public:
    MCExtents(){}
    virtual ~MCExtents(){}
};

class MCTheFiles : public MCFunction
{
public:
	MCTheFiles() : m_folder(nil) {}
	virtual ~MCTheFiles();
	virtual Parse_stat parse(MCScriptPoint &, Boolean p_is_the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
	virtual void compile(MCSyntaxFactoryRef);
private:
	MCExpression *m_folder;
};

class MCFlushEvents : public MCUnaryFunctionCtxt<MCNameRef, MCStringRef, MCInterfaceEvalFlushEvents, EE_FLUSHEVENTS_BADTYPE, PE_FLUSHEVENTS_BADPARAM, kMCInterfaceEvalFlushEventsMethodInfo>
{
public:
    MCFlushEvents(){}
    virtual ~MCFlushEvents(){}
};

class MCFocusedObject : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFocusedObject>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFocusedObjectMethodInfo; }

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

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCTextEvalFontNamesMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCFontLanguage : public MCUnaryFunctionCtxt<MCStringRef, MCNameRef, MCTextEvalFontLanguage, EE_FONTSIZES_BADFONTNAME, PE_FONTNAMES_BADPARAM, kMCTextEvalFontLanguageMethodInfo>
{
public:
    MCFontLanguage(){}
    virtual ~MCFontLanguage(){}
};


class MCFontSizes : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCTextEvalFontSizes, EE_FONTSIZES_BADFONTNAME, PE_FONTSIZES_BADPARAM, kMCTextEvalFontSizesMethodInfo>
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
	
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCFoundChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundChunkMethodInfo; }

};

class MCFoundField : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundField>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundFieldMethodInfo; }
};

class MCFoundLine : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundLine>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundLineMethodInfo; }

};

class MCFoundLoc : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundLoc>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundLocMethodInfo; }

};

class MCFoundText : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundText>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundTextMethodInfo; }


};

class MCFunctionNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalFunctionNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalFunctionNamesMethodInfo; }
};

class MCGlobalLoc : public MCUnaryFunctionCtxt<MCPoint, MCPoint, MCInterfaceEvalGlobalLoc, EE_GLOBALLOC_NAP, PE_GLOBALLOC_BADPOINT, kMCInterfaceEvalGlobalLocMethodInfo>
{
public:
    MCGlobalLoc(){}
    virtual ~MCGlobalLoc(){}
};

class MCGlobals : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalGlobalNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalGlobalNamesMethodInfo; }

};

class MCHasMemory : public MCUnaryFunctionCtxt<uinteger_t, bool, MCLegacyEvalHasMemory, EE_HASMEMORY_BADAMOUNT, PE_HASMEMORY_BADPARAM, kMCLegacyEvalHasMemoryMethodInfo>
{
public:
    MCHasMemory(){}
    virtual ~MCHasMemory(){}
};

class MCHeapSpace : public MCConstantFunctionCtxt<integer_t, MCLegacyEvalHeapSpace>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalHeapSpaceMethodInfo; }

};

class MCHostAddress : public MCUnaryFunctionCtxt<MCNameRef, MCStringRef, MCNetworkEvalHostAddress, EE_HOSTADDRESS_BADSOCKET, PE_HOSTADDRESS_BADSOCKET, kMCNetworkEvalHostAddressMethodInfo>
{
public:
    MCHostAddress(){}
    virtual ~MCHostAddress(){}
};

class MCHostAtoN : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCNetworkEvalHostAddressToName, EE_HOSTATON_BADADDRESS, PE_HOSTATON_BADADDRESS, kMCNetworkEvalHostAddressToNameMethodInfo>
{
public:
    MCHostAtoN(){}
    virtual ~MCHostAtoN(){}
};

class MCHostName : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalHostName>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalHostNameMethodInfo; }

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

	virtual void compile(MCSyntaxFactoryRef);
};

class MCInsertScripts : public MCConstantFunction
{
protected:
	Boolean front;
public:
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return front == True ? kMCEngineEvalFrontScriptsMethodInfo : kMCEngineEvalBackScriptsMethodInfo; }
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
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalInterruptMethodInfo; }

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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCIsoToMac : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalIsoToMac, EE_ISOTOMAC_BADSOURCE, PE_ISOTOMAC_BADPARAM, kMCFiltersEvalIsoToMacMethodInfo>
{
public:
    MCIsoToMac(){}
    virtual ~MCIsoToMac(){}
};

class MCIsNumber : public MCUnaryFunctionCtxt<MCStringRef, bool, MCLegacyEvalIsNumber, EE_ISNUMBER_BADSOURCE, PE_ISNUMBER_BADPARAM, kMCLegacyEvalIsNumberMethodInfo>
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

	virtual void compile(MCSyntaxFactoryRef);
};

class MCKeysDown : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalKeysDown>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalKeysDownMethodInfo; }

};

class MCLength : public MCUnaryFunctionCtxt<MCStringRef, integer_t, MCStringsEvalLength, EE_LENGTH_BADSOURCE, PE_LENGTH_BADPARAM, kMCStringsEvalLengthMethodInfo>
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
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalLicensedMethodInfo; }
};

class MCLocalLoc : public MCUnaryFunctionCtxt<MCPoint, MCPoint, MCInterfaceEvalLocalLoc, EE_LOCALLOC_NAP, PE_LOCALLOC_BADPOINT, kMCInterfaceEvalLocalLocMethodInfo>
{
public:
    MCLocalLoc(){}
    virtual ~MCLocalLoc(){}
};

class MCLocals : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalLocalNames>
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalLocalNamesMethodInfo; }
};

class MCMachine : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalMachine>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalMachineMethodInfo; }
};

class MCMacToIso : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalMacToIso, EE_MACTOISO_BADSOURCE, PE_MACTOISO_BADPARAM, kMCFiltersEvalMacToIsoMethodInfo>
{
public:
    MCMacToIso(){}
    virtual ~MCMacToIso(){}
};

class MCMainStacks : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMainStacks>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMainStacksMethodInfo; }
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
	virtual void compile(MCSyntaxFactoryRef);
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
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalMeMethodInfo; }
};

class MCMenuObject : public MCConstantFunctionCtxt<MCStringRef, MCLegacyEvalMenuObject>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalMenuObjectMethodInfo; }
};

class MCMenus : public MCConstantFunctionCtxt<MCStringRef, MCLegacyEvalMenus>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalMenusMethodInfo; }
};

class MCMerge : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCStringsEvalMerge, EE_MERGE_BADSOURCE, PE_MERGE_BADPARAM, kMCStringsEvalMergeMethodInfo>
{
public:
    MCMerge(){}
    virtual ~MCMerge(){}
};

class MCMillisecs : public MCConstantFunctionCtxt<double, MCDateTimeEvalMilliseconds>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalMillisecondsMethodInfo; }
};

class MCMonthNames : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalMonthNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalMonthNamesMethodInfo; }
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCMouseChar : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseChar>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseCharMethodInfo; }
};

class MCMouseCharChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseCharChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseCharChunkMethodInfo; }
};

class MCMouseChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseChunkMethodInfo; }
};

class MCMouseClick : public MCConstantFunctionCtxt<bool, MCInterfaceEvalMouseClick>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseClickMethodInfo; }
};

class MCMouseColor : public MCConstantFunctionCtxt<MCColor, MCInterfaceEvalMouseColor>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseColorMethodInfo; }
};

class MCMouseControl : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseControl>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseControlMethodInfo; }
};

class MCMouseH : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalMouseH>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseHMethodInfo; }
};

class MCMouseLine : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseLine>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseLineMethodInfo; }
};

class MCMouseLoc : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseLoc>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseLocMethodInfo; }
};

class MCMouseStack : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseStack>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseStackMethodInfo; }
};

class MCMouseText : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseText>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseTextMethodInfo; }
};

class MCMouseV : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalMouseV>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseVMethodInfo; }
};

class MCMovie : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalMovie>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalMovieMethodInfo; }
};

class MCMovingControls : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMovingControls>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMovingControlsMethodInfo; }
};

class MCNativeCharToNum : public MCUnaryFunctionCtxt<MCStringRef, uinteger_t, MCStringsEvalNativeCharToNum, EE_CHARTONUM_BADSOURCE, PE_CHARTONUM_BADPARAM, kMCStringsEvalNumToCharMethodInfo> // FIXME
{
public:
    MCNativeCharToNum(){}
    virtual ~MCNativeCharToNum(){}
};

class MCNumToChar: public MCUnaryFunctionCtxt<uinteger_t, MCValueRef, MCStringsEvalNumToChar, EE_NUMTOCHAR_BADSOURCE, PE_NUMTOCHAR_BADPARAM, kMCStringsEvalNumToCharMethodInfo>
{
public:
    MCNumToChar(){}
    virtual ~MCNumToChar(){}
};

class MCNumToNativeChar : public MCUnaryFunctionCtxt<uinteger_t, MCStringRef, MCStringsEvalNumToNativeChar,
    EE_NUMTOCHAR_BADSOURCE, PE_NUMTOCHAR_BADPARAM, kMCStringsEvalNumToCharMethodInfo> // FIXME
{
public:
    MCNumToNativeChar(){}
    virtual ~MCNumToNativeChar(){}
};

class MCNumToUnicodeChar : public MCUnaryFunctionCtxt<uinteger_t, MCStringRef, MCStringsEvalNumToUnicodeChar,
    EE_NUMTOCHAR_BADSOURCE, PE_NUMTOCHAR_BADPARAM, kMCStringsEvalNumToCharMethodInfo> // FIXME
{
public:
    MCNumToUnicodeChar(){}
    virtual ~MCNumToUnicodeChar(){}
};

// AL-2014-10-21: [[ Bug 13740 ]] numToByte should return a DataRef
class MCNumToByte: public MCUnaryFunctionCtxt<integer_t, MCDataRef, MCStringsEvalNumToByte, EE_NUMTOBYTE_BADSOURCE, PE_NUMTOBYTE_BADPARAM, kMCStringsEvalNumToByteMethodInfo>
{
public:
    MCNumToByte(void){}
    virtual ~MCNumToByte(){}
};

class MCOpenFiles : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalOpenFiles>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalOpenFilesMethodInfo; }
};

class MCOpenProcesses : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalOpenProcesses>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalOpenProcessesMethodInfo; }
};

class MCOpenProcessIds : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalOpenProcessesIds>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalOpenProcessesIdsMethodInfo; }
};

class MCOpenSockets : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalOpenSockets>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalOpenSocketsMethodInfo; }
};

class MCOpenStacks : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalOpenStacks>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalOpenStacksMethodInfo; }
};

class MCOptionKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalOptionKey>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalOptionKeyMethodInfo; }
};

class MCParam : public MCUnaryFunctionCtxt<integer_t, MCValueRef, MCEngineEvalParam, EE_PARAM_BADINDEX, PE_PARAM_BADPARAM, kMCEngineEvalParamMethodInfo>
{
public:
    MCParam(){}
    virtual ~MCParam(){}
};

class MCParamCount : public MCConstantFunctionCtxt<integer_t, MCEngineEvalParamCount>
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalParamCountMethodInfo; }
};

class MCParams : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalParams>
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalParamsMethodInfo; }
};

class MCPeerAddress : public MCUnaryFunctionCtxt<MCNameRef, MCStringRef, MCNetworkEvalPeerAddress, EE_HOSTADDRESS_BADSOCKET, PE_PEERADDRESS_BADSOCKET, kMCNetworkEvalPeerAddressMethodInfo>
{
public:
    MCPeerAddress(){}
    virtual ~MCPeerAddress(){}
};

class MCPendingMessages : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalPendingMessages>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalPendingMessagesMethodInfo; }
};

class MCPid : public MCConstantFunctionCtxt<integer_t, MCFilesEvalProcessId>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalProcessIdMethodInfo; }
};

class MCPlatform : public MCConstantFunctionCtxt<MCNameRef, MCEngineEvalPlatform>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalPlatformMethodInfo; }
};


// JS-2013-06-19: [[ StatsFunctions ]] Definition of populationStdDev
class MCPopulationStdDev : public MCParamFunctionCtxt<MCMathEvalPopulationStdDev, EE_POP_STDDEV_BADSOURCE, PE_POP_STDDEV_BADPARAM, kMCMathEvalPopulationStdDevMethodInfo>
{
public:
    MCPopulationStdDev(){}
    virtual ~MCPopulationStdDev(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of populationVariance
class MCPopulationVariance : public MCParamFunctionCtxt<MCMathEvalPopulationVariance, EE_POP_VARIANCE_BADSOURCE, PE_POP_VARIANCE_BADPARAM, kMCMathEvalPopulationVarianceMethodInfo>
{
public:
    MCPopulationVariance(){}
    virtual ~MCPopulationVariance(){}
};

class MCProcessor : public MCConstantFunctionCtxt<MCNameRef, MCEngineEvalProcessor>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalProcessorMethodInfo; }
};

class MCPropertyNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalPropertyNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalPropertyNamesMethodInfo; }
};

class MCQTVersion : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalQTVersion>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalQTVersionMethodInfo; }
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCTheResult : public MCConstantFunctionCtxt<MCValueRef, MCEngineEvalResult>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalResultMethodInfo; }
};

class MCScreenColors : public MCConstantFunctionCtxt<double, MCInterfaceEvalScreenColors>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalScreenColorsMethodInfo; }
};

class MCScreenDepth : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalScreenDepth>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalScreenDepthMethodInfo; }
};

class MCScreenLoc : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalScreenLoc>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalScreenLocMethodInfo; }
};

class MCScreenName : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalScreenName>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalScreenNameMethodInfo; }
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCScreenType : public MCConstantFunctionCtxt<MCNameRef, MCLegacyEvalScreenType>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalScreenTypeMethodInfo; }
};

class MCScreenVendor : public MCConstantFunctionCtxt<MCNameRef, MCLegacyEvalScreenVendor>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalScreenVendorMethodInfo; }
};

class MCScriptLimits : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalScriptLimits>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalScriptLimitsMethodInfo; }
};

class MCSeconds : public MCConstantFunctionCtxt<double, MCDateTimeEvalSeconds>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalSecondsMethodInfo; }
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
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSelectedField : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalSelectedField>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalSelectedFieldMethodInfo; }
};

class MCSelectedImage : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalSelectedImage>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalSelectedImageMethodInfo; }
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
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSelectedObject : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalSelectedObject>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalSelectedObjectMethodInfo; }
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCShell : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalShell, EE_SHELL_BADSOURCE, PE_SHELL_BADPARAM, kMCFilesEvalShellMethodInfo>
{
public:
    MCShell(){}
    virtual ~MCShell(){}
};

class MCShiftKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalShiftKey>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalShiftKeyMethodInfo; }
};

class MCSound : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalSound>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalSoundMethodInfo; }
};

class MCStacks : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalStacks>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalStacksMethodInfo; }
};

class MCStackSpace : public MCConstantFunctionCtxt<integer_t, MCLegacyEvalStackSpace>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalStackSpaceMethodInfo; }
};

class MCSysError : public MCConstantFunctionCtxt<uinteger_t, MCEngineEvalSysError>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalSysErrorMethodInfo; }
};

class MCSystemVersion : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalSystemVersion>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalSystemVersionMethodInfo; }
};

class MCTarget : public MCConstantFunction
{
	Boolean contents;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext &, MCExecValue &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return contents == True ? kMCEngineEvalTargetContentsMethodInfo : kMCEngineEvalTargetMethodInfo; }
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCTempName : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalTempName>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalTempNameMethodInfo; }
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
    virtual void compile(MCSyntaxFactoryRef);
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
    virtual void compile(MCSyntaxFactoryRef);
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
    virtual void compile(MCSyntaxFactoryRef);
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
    virtual void compile(MCSyntaxFactoryRef);
};

class MCTicks : public MCConstantFunctionCtxt<double, MCDateTimeEvalTicks>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalTicksMethodInfo; }
};

class MCTheTime : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalTime>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalTimeMethodInfo; }
};

class MCToLower : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCStringsEvalToLower, EE_TOLOWER_BADSOURCE, PE_TOLOWER_BADPARAM, kMCStringsEvalToLowerMethodInfo>
{
public:
    MCToLower(){}
    virtual ~MCToLower(){}
};

class MCToUpper : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCStringsEvalToUpper, EE_TOUPPER_BADSOURCE, PE_TOUPPER_BADPARAM, kMCStringsEvalToUpperMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCUnicodeCharToNum : public MCUnaryFunctionCtxt<MCStringRef, uinteger_t, MCStringsEvalUnicodeCharToNum,
    EE_CHARTONUM_BADSOURCE, PE_CHARTONUM_BADPARAM, kMCStringsEvalCharToNumMethodInfo> // FIXME
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
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCUrlDecode : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFiltersEvalUrlDecode, EE_URLDECODE_BADSOURCE, PE_URLDECODE_BADPARAM, kMCFiltersEvalUrlDecodeMethodInfo>
{
public:
    MCUrlDecode(){}
    virtual ~MCUrlDecode(){}
};

class MCUrlEncode : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFiltersEvalUrlEncode, EE_URLENCODE_BADSOURCE, PE_URLENCODE_BADPARAM, kMCFiltersEvalUrlEncodeMethodInfo>
{
public:
    MCUrlEncode(){}
    virtual ~MCUrlEncode(){}
};

class MCUrlStatus : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCNetworkEvalUrlStatus, EE_URLSTATUS_BADSOURCE, PE_URLSTATUS_BADPARAM, kMCNetworkEvalUrlStatusMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCVariables : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalVariableNames>
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalVariableNamesMethodInfo; }
};

class MCVersion : public MCConstantFunctionCtxt<MCNameRef, MCEngineEvalVersion>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalVersionMethodInfo; }
};

class MCWaitDepth : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalWaitDepth>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalWaitDepthMethodInfo; }
};

class MCWeekDayNames : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalWeekDayNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalWeekDayNamesMethodInfo; }
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
	virtual void compile(MCSyntaxFactoryRef);
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
    virtual void compile(MCSyntaxFactoryRef);
};

class MCDeleteRegistry : public MCUnaryFunctionCtxt<MCStringRef, bool, MCFilesEvalDeleteRegistry, EE_SETREGISTRY_BADEXP, PE_SETREGISTRY_BADPARAM, kMCFilesEvalDeleteRegistryMethodInfo>
{
public:
    MCDeleteRegistry(){}
    virtual ~MCDeleteRegistry(){}
};

class MCListRegistry : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalListRegistry, EE_SETREGISTRY_BADEXP, PE_SETREGISTRY_BADPARAM, kMCFilesEvalListRegistryMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSpecialFolderPath : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalSpecialFolderPath, EE_SPECIALFOLDERPATH_BADPARAM, PE_SPECIALFOLDERPATH_BADTYPE, kMCFilesEvalSpecialFolderPathMethodInfo>
{
public:
    MCSpecialFolderPath(){}
    virtual ~MCSpecialFolderPath(){}
};

class MCShortFilePath : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalShortFilePath, EE_SHORTFILEPATH_BADSOURCE, PE_SHORTFILEPATH_BADPARAM, kMCFilesEvalShortFilePathMethodInfo>
{
public:
    MCShortFilePath(){}
    virtual ~MCShortFilePath(){}
};

class MCLongFilePath : public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalLongFilePath, EE_LONGFILEPATH_BADSOURCE, PE_LONGFILEPATH_BADPARAM, kMCFilesEvalLongFilePathMethodInfo>
{
public:
    MCLongFilePath(){}
    virtual ~MCLongFilePath(){}
};

class MCAlternateLanguages : public MCConstantFunctionCtxt<MCStringRef, MCScriptingEvalAlternateLanguages>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCScriptingEvalAlternateLanguagesMethodInfo; }
};

class MCAliasReference: public MCUnaryFunctionCtxt<MCStringRef, MCStringRef, MCFilesEvalAliasReference, EE_ALIASREFERENCE_BADSOURCE, PE_ALIASREFERENCE_BADPARAM, kMCFilesEvalAliasReferenceMethodInfo>
{
public:
    MCAliasReference(){}
    virtual ~MCAliasReference(){}
};

class MCCipherNames : public MCConstantFunctionCtxt<MCStringRef, MCSecurityEvalCipherNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCSecurityEvalCipherNamesMethodInfo; }
};

// Math functions in funcsm.cpp

class MCAbsFunction : public MCUnaryFunctionCtxt<double, double, MCMathEvalAbs, EE_ABS_BADSOURCE, PE_ABS_BADPARAM, kMCMathEvalAbsMethodInfo>
{
public:
	MCAbsFunction(){}
    virtual ~MCAbsFunction(){}
};

class MCAcos : public MCUnaryFunctionCtxt<double, double, MCMathEvalAcos, EE_ACOS_BADSOURCE, PE_ACOS_BADPARAM, kMCMathEvalAcosMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of arithmeticMean
class MCArithmeticMean : public MCParamFunctionCtxt<MCMathEvalArithmeticMean, EE_AVERAGE_BADSOURCE, PE_AVERAGE_BADPARAM, kMCMathEvalArithmeticMeanMethodInfo>
{
public:
    MCArithmeticMean(){}
    virtual ~MCArithmeticMean(){}
};

class MCAsin : public MCUnaryFunctionCtxt<double, double, MCMathEvalAsin, EE_ASIN_BADSOURCE, PE_ASIN_BADPARAM, kMCMathEvalAsinMethodInfo>
{
public:
	MCAsin(){}
	virtual ~MCAsin(){}
};

class MCAtan : public MCUnaryFunctionCtxt<double, double, MCMathEvalAtan, EE_ATAN_BADSOURCE, PE_ATAN_BADPARAM, kMCMathEvalAtanMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of averageDev (was average)
class MCAvgDev : public MCParamFunctionCtxt<MCMathEvalAverageDeviation, EE_AVERAGE_BADSOURCE, PE_AVERAGE_BADPARAM, kMCMathEvalAverageMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCCos : public MCUnaryFunctionCtxt<double, double, MCMathEvalCos, EE_COS_BADSOURCE, PE_COS_BADPARAM, kMCMathEvalCosMethodInfo>
{
public:
	MCCos(){}
	virtual ~MCCos(){}
};

class MCExp : public MCUnaryFunctionCtxt<double, double, MCMathEvalExp, EE_EXP_BADSOURCE, PE_EXP_BADPARAM, kMCMathEvalExpMethodInfo>
{
public:
	MCExp(){}
	virtual ~MCExp(){}
};

class MCExp1 : public MCUnaryFunctionCtxt<double, double, MCMathEvalExp1, EE_EXP1_BADSOURCE, PE_EXP1_BADPARAM, kMCMathEvalExp1MethodInfo>
{
public:
	MCExp1(){}
	virtual ~MCExp1(){}
};

class MCExp2 : public MCUnaryFunctionCtxt<double, double, MCMathEvalExp2, EE_EXP2_BADSOURCE, PE_EXP2_BADPARAM, kMCMathEvalExp2MethodInfo>
{
public:
	MCExp2(){}
	virtual ~MCExp2(){}
};

class MCExp10 : public MCUnaryFunctionCtxt<double, double, MCMathEvalExp10, EE_EXP10_BADSOURCE, PE_EXP10_BADPARAM, kMCMathEvalExp10MethodInfo>
{
public:
	MCExp10(){}
	virtual ~MCExp10(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of geometricMean
class MCGeometricMean : public MCParamFunctionCtxt<MCMathEvalGeometricMean, EE_GEO_MEAN_BADSOURCE, PE_GEO_MEAN_BADPARAM, kMCMathEvalGeometricMeanMethodInfo>
{
public:
    MCGeometricMean(){}
    virtual ~MCGeometricMean(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of harmonicMean
class MCHarmonicMean : public MCParamFunctionCtxt<MCMathEvalHarmonicMean, EE_HAR_MEAN_BADSOURCE, PE_HAR_MEAN_BADPARAM, kMCMathEvalHarmonicMeanMethodInfo>
{
public:
    MCHarmonicMean(){}
    virtual ~MCHarmonicMean(){}
};

class MCLn : public MCUnaryFunctionCtxt<double, double, MCMathEvalLn, EE_LN_BADSOURCE, PE_LN_BADPARAM, kMCMathEvalLnMethodInfo>
{
public:
	MCLn(){}
	virtual ~MCLn(){}
};

class MCLn1 : public MCUnaryFunctionCtxt<double, double, MCMathEvalLn1, EE_LN1_BADSOURCE, PE_LN1_BADPARAM, kMCMathEvalLn1MethodInfo>
{
public:
	MCLn1(){}
	virtual ~MCLn1(){}
};

class MCLog2 : public MCUnaryFunctionCtxt<double, double, MCMathEvalLog2, EE_LOG2_BADSOURCE, PE_LOG2_BADPARAM, kMCMathEvalLog2MethodInfo>
{
public:
	MCLog2(){}
	virtual ~MCLog2(){}
};

class MCLog10 : public MCUnaryFunctionCtxt<double, double, MCMathEvalLog10, EE_LOG10_BADSOURCE, PE_LOG10_BADPARAM, kMCMathEvalLog10MethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCMaxFunction : public MCParamFunctionCtxt<MCMathEvalMax, EE_MAX_BADSOURCE, PE_MAX_BADPARAM, kMCMathEvalMaxMethodInfo>
{
public:
    MCMaxFunction(){}
    virtual ~MCMaxFunction(){}
};

class MCMedian : public MCParamFunctionCtxt<MCMathEvalMedian, EE_MEDIAN_BADSOURCE, PE_MEDIAN_BADPARAM, kMCMathEvalMedianMethodInfo>
{
public:
    MCMedian(){}
    virtual ~MCMedian(){}
};

class MCMD5Digest : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalMD5Digest, EE_MD5DIGEST_BADSOURCE, PE_MD5DIGEST_BADPARAM, kMCFiltersEvalMD5DigestMethodInfo>
{
public:
	MCMD5Digest(){}
	virtual ~MCMD5Digest(){}
};

class MCSHA1Digest : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, MCFiltersEvalSHA1Digest, EE_SHA1DIGEST_BADSOURCE, PE_SHA1DIGEST_BADPARAM, kMCFiltersEvalSHA1DigestMethodInfo>
{
public:
	MCSHA1Digest(){}
	virtual ~MCSHA1Digest(){}
};

class MCMinFunction : public MCParamFunctionCtxt<MCMathEvalMin, EE_MIN_BADSOURCE, PE_MIN_BADPARAM, kMCMathEvalMinMethodInfo>
{
public:
    MCMinFunction(){}
    virtual ~MCMinFunction(){}
};

class MCRandom : public MCUnaryFunctionCtxt<double, double, MCMathEvalRandom, EE_RANDOM_BADSOURCE, PE_RANDOM_BADPARAM, kMCMathEvalRandomMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSin : public MCUnaryFunctionCtxt<double, double, MCMathEvalSin, EE_SIN_BADSOURCE, PE_SIN_BADPARAM, kMCMathEvalSinMethodInfo>
{
public:
	MCSin(){}
	virtual ~MCSin(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of sampleStdDev (was stdDev)
class MCSampleStdDev : public MCParamFunctionCtxt<MCMathEvalSampleStdDev, EE_STDDEV_BADSOURCE, PE_STDDEV_BADPARAM, kMCMathEvalSampleStdDevMethodInfo>
{
public:
    MCSampleStdDev(){}
    virtual ~MCSampleStdDev(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of sampleVariance
class MCSampleVariance : public MCParamFunctionCtxt<MCMathEvalSampleVariance, EE_VARIANCE_BADSOURCE, PE_VARIANCE_BADPARAM, kMCMathEvalPopulationVarianceMethodInfo>
{
public:
    MCSampleVariance(){}
    virtual ~MCSampleVariance(){}
};

class MCSqrt : public MCUnaryFunctionCtxt<double, double, MCMathEvalSqrt, EE_SQRT_BADSOURCE, PE_SQRT_BADPARAM, kMCMathEvalSqrtMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCStdDev : public MCParamFunctionCtxt<MCMathEvalSampleStdDev, EE_STDDEV_BADSOURCE, PE_STDDEV_BADPARAM, kMCMathEvalSampleStdDevMethodInfo>
{
public:
    MCStdDev(){}
    virtual ~MCStdDev(){}
};

class MCSum : public MCParamFunctionCtxt<MCMathEvalSum, EE_SUM_BADSOURCE, PE_SUM_BADPARAM, kMCMathEvalSumMethodInfo>
{
public:
    MCSum(){}
    virtual ~MCSum(){}
};

class MCTan : public MCUnaryFunctionCtxt<double, double, MCMathEvalTan, EE_TAN_BADSOURCE, PE_TAN_BADPARAM, kMCMathEvalTanMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCTranspose : public MCUnaryFunctionCtxt<MCArrayRef, MCArrayRef, MCArraysEvalTransposeMatrix, EE_TRANSPOSE_BADSOURCE, PE_TRANSPOSE_BADPARAM, kMCArraysEvalTransposeMatrixMethodInfo>
{
public:
	MCTranspose(){}
	virtual ~MCTranspose(){}
};

/*
class MCMathOperator : public MCUnaryFunctionCtxt<double, double, MCExecValueTraits, Exec_errors, Parse_errors, MCExecMethodInfo>
{
public:
    MCMathOperator(){}
    virtual ~MCMathOperator(){}
};
*/

class MCTrunc : public MCUnaryFunctionCtxt<double, double, MCMathEvalTrunc, EE_TRUNC_BADSOURCE, PE_TRUNC_BADPARAM, kMCMathEvalTruncMethodInfo>
{
public:
    MCTrunc(){}
    virtual ~MCTrunc(){}
};

// MDW-2014-08-23 : [[ feature_floor ]]
class MCFloor : public MCUnaryFunctionCtxt<double, double, MCMathEvalFloor, EE_FLOOR_BADSOURCE, PE_FLOOR_BADPARAM, kMCMathEvalFloorMethodInfo>
{
public:
	MCFloor(){}
	virtual ~MCFloor(){}
};

class MCCeil : public MCUnaryFunctionCtxt<double, double, MCMathEvalCeil, EE_CEIL_BADSOURCE, PE_CEIL_BADPARAM, kMCMathEvalCeilMethodInfo>
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
	virtual void compile(MCSyntaxFactoryRef);
};

class MCRandomBytes: public MCUnaryFunctionCtxt<uinteger_t, MCDataRef, MCSecurityEvalRandomBytes, EE_RANDOMBYTES_BADCOUNT, PE_RANDOMBYTES_BADPARAM, kMCSecurityEvalRandomBytesMethodInfo>
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
	
    virtual MCExecMethodInfo *getmethodinfo(void) const { return is_screen ? kMCInterfaceEvalControlAtScreenLocMethodInfo : kMCInterfaceEvalControlAtLocMethodInfo; }
    virtual MCExpression *getmethodarg(void) const { return location; }
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



