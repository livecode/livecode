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

//
// MCFunction class declarations
//
#ifndef	FUNCTIONS_H
#define	FUNCTIONS_H

#include "express.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

class MCFunction : public MCExpression
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	Parse_stat parsetarget(MCScriptPoint &spt, Boolean the,
	                       Boolean needone, MCChunk *&chunk);
	Exec_stat params_to_doubles(MCExecPoint& ep, MCParameter *p_params, real64_t*& r_doubles, uindex_t& r_count);
	
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
};
////////////////////////////////////////////////////////////////////////////////

class MCArrayDecode: public MCUnaryFunction
{
	MCExpression *source;
public:
	MCArrayDecode()
	{
		source = NULL;
	}
	virtual ~MCArrayDecode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCArraysEvalArrayDecodeMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCArrayEncode: public MCUnaryFunction
{
	MCExpression *source;
public:
	MCArrayEncode()
	{
		source = NULL;
	}
	virtual ~MCArrayEncode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCArraysEvalArrayEncodeMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCBase64Decode : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCBase64Decode()
	{
		source = NULL;
	}
	virtual ~MCBase64Decode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalBase64DecodeMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCBase64Encode : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCBase64Encode()
	{
		source = NULL;
	}
	virtual ~MCBase64Encode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalBase64EncodeMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCBuildNumber : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalBuildNumberMethodInfo; }
};

class MCCachedUrls : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalCachedUrlsMethodInfo; }
};

class MCCapsLockKey : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalCapsLockKeyMethodInfo; }
};

class MCCharToNum : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCCharToNum()
	{
		source = NULL;
	}
	virtual ~MCCharToNum();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalCharToNumMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCByteToNum : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCByteToNum()
	{
		source = NULL;
	}
	virtual ~MCByteToNum();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalByteToNumMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCClipboard : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalClipboardMethodInfo; }
};

class MCCompress : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCCompress()
	{
		source = NULL;
	}
	virtual ~MCCompress();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalCompressMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCConstantNames : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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

class MCClickChar : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickCharMethodInfo; }
};

class MCClickCharChunk : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickCharChunkMethodInfo; }
};

class MCClickChunk : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickChunkMethodInfo; }
};

class MCClickField : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickFieldMethodInfo; }
};

class MCClickH : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickHMethodInfo; }
};

class MCClickLine : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickLineMethodInfo; }
};

class MCClickLoc : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickLocMethodInfo; }
};

class MCClickStack : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickStackMethodInfo; }
};

class MCClickText : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickTextMethodInfo; }
};

class MCClickV : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickVMethodInfo; }
};

class MCColorNames : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalColorNamesMethodInfo; }

};

class MCCommandNames : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalCommandNamesMethodInfo; }

};

class MCCommandKey : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalCommandKeyMethodInfo; }
};

class MCControlKey : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalControlKeyMethodInfo; }
};

class MCDate : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalDateMethodInfo; }

};

class MCDateFormat : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalDateFormatMethodInfo; }

};

class MCDecompress : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCDecompress()
	{
		source = NULL;
	}
	virtual ~MCDecompress();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	static Exec_stat do_decompress(MCExecPoint& ep, uint2, uint2);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalDecompressMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCDirectories : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDirectoriesMethodInfo; }

};

class MCDiskSpace : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDiskSpaceMethodInfo; }

};

class MCDNSServers : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalDNSServersMethodInfo; }

};

class MCDragDestination: public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalDragDestinationMethodInfo; }

};

class MCDragSource: public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalDragSourceMethodInfo; }

};

class MCDriverNames : public MCConstantFunction
{
	MCExpression *type;
public:
	MCDriverNames()
	{
		type = NULL;
	}
	virtual ~MCDriverNames();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDriverNamesMethodInfo; }

};

class MCDrives : public MCConstantFunction
{
	MCExpression *type;
public:
	MCDrives()
	{
		type = NULL;
	}
	virtual ~MCDrives();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDrivesMethodInfo; }

};

class MCDropChunk: public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalDropChunkMethodInfo; }

};

class MCQTEffects : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalQTEffectsMethodInfo; }

};

class MCRecordCompressionTypes: public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalRecordCompressionTypesMethodInfo; }

};

class MCRecordLoudness : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalRecordLoudnessMethodInfo; }

};

class MCEnvironment : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalEnvironmentMethodInfo; }

};

class MCEncrypt : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCEncrypt()
	{
		source = NULL;
	}
	virtual ~MCEncrypt();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCSecurityEvalEncryptMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
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
	virtual Exec_stat eval(MCExecPoint &);

	virtual void compile(MCSyntaxFactoryRef);
};

class MCExtents : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCExtents()
	{
		source = NULL;
	}
	virtual ~MCExtents();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCArraysEvalExtentsMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCTheFiles : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalFilesMethodInfo; }

};

class MCFlushEvents : public MCUnaryFunction
{
	MCExpression *type;
public:
	MCFlushEvents()
	{
		type = NULL;
	}
	virtual ~MCFlushEvents();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFlushEventsMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCFocusedObject : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCTextEvalFontNamesMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCFontLanguage : public MCUnaryFunction
{
	MCExpression *fontname;
public:
	MCFontLanguage()
	{
		fontname = NULL;
	}
	virtual ~MCFontLanguage();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCTextEvalFontLanguageMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return fontname; }
};


class MCFontSizes : public MCUnaryFunction
{
	MCExpression *fontname;
public:
	MCFontSizes()
	{
		fontname = NULL;
	}
	virtual ~MCFontSizes();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCTextEvalFontSizesMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return fontname; }
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
	virtual Exec_stat eval(MCExecPoint &);
	
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCFoundChunk : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundChunkMethodInfo; }

};

class MCFoundField : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundFieldMethodInfo; }
};

class MCFoundLine : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundLineMethodInfo; }

};

class MCFoundLoc : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundLocMethodInfo; }

};

class MCFoundText : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundTextMethodInfo; }


};

class MCFunctionNames : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalFunctionNamesMethodInfo; }
};

class MCGlobalLoc : public MCUnaryFunction
{
	MCExpression *point;
public:
	MCGlobalLoc()
	{
		point = NULL;
	}
	virtual ~MCGlobalLoc();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalGlobalLocMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return point; }
};

class MCGlobals : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalGlobalNamesMethodInfo; }

};

class MCHasMemory : public MCUnaryFunction
{
	MCExpression *amount;
public:
	MCHasMemory()
	{
		amount = NULL;
	}
	virtual ~MCHasMemory();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalHasMemoryMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return amount; }
};

class MCHeapSpace : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalHeapSpaceMethodInfo; }

};

class MCHostAddress : public MCUnaryFunction
{
	MCExpression *socket;
public:
	MCHostAddress()
	{
		socket = NULL;
	}
	virtual ~MCHostAddress();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalHostAddressMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return socket; }
};

class MCHostAtoN : public MCUnaryFunction
{
	MCExpression *address;
public:
	MCHostAtoN()
	{
		address = NULL;
	}
	virtual ~MCHostAtoN();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalHostAddressToNameMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return address; }
};

class MCHostName : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);

	virtual void compile(MCSyntaxFactoryRef);
};

class MCInsertScripts : public MCConstantFunction
{
protected:
	Boolean front;
public:
	virtual Exec_stat eval(MCExecPoint &);
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

class MCInterrupt : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCIsoToMac : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCIsoToMac()
	{
		source = NULL;
	}
	virtual ~MCIsoToMac();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalIsoToMacMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCIsNumber : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCIsNumber()
	{
		source = NULL;
	}
	virtual ~MCIsNumber();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalIsNumberMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
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
	virtual Exec_stat eval(MCExecPoint &);

	virtual void compile(MCSyntaxFactoryRef);
};

class MCKeysDown : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalKeysDownMethodInfo; }

};

class MCLength : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCLength()
	{
		source = NULL;
	}
	virtual ~MCLength();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalLengthMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCLicensed : public MCConstantFunction
{
	MCExpression *source;
public:
	MCLicensed()
	{
		source = NULL;
	}
	virtual ~MCLicensed();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalLicensedMethodInfo; }
};

class MCLocalLoc : public MCUnaryFunction
{
	MCExpression *point;
public:
	MCLocalLoc()
	{
		point = NULL;
	}
	virtual ~MCLocalLoc();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalLocalLocMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return point; }
};

class MCLocals : public MCConstantFunction
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalLocalNamesMethodInfo; }
};

class MCMachine : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalMachineMethodInfo; }
};

class MCMacToIso : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCMacToIso()
	{
		source = NULL;
	}
	virtual ~MCMacToIso();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalMacToIsoMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCMainStacks : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
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


class MCMe : public MCConstantFunction
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalMeMethodInfo; }
};

class MCMenuObject : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalMenuObjectMethodInfo; }
};

class MCMenus : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalMenusMethodInfo; }
};

class MCMerge : public MCUnaryFunction
{
public:
	MCHandler *h;
	MCExpression *source;
public:
	MCMerge()
	{
		source = NULL;
	}
	virtual ~MCMerge();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	Exec_stat doscript(MCExecPoint &);
	void deletestatements(MCStatement *statements);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalMergeMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCMillisecs : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalMillisecondsMethodInfo; }
};

class MCMonthNames : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCMouseChar : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseCharMethodInfo; }
};

class MCMouseCharChunk : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseCharChunkMethodInfo; }
};

class MCMouseChunk : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseChunkMethodInfo; }
};

class MCMouseClick : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseClickMethodInfo; }
};

class MCMouseColor : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseColorMethodInfo; }
};

class MCMouseControl : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseControlMethodInfo; }
};

class MCMouseH : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseHMethodInfo; }
};

class MCMouseLine : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseLineMethodInfo; }
};

class MCMouseLoc : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseLocMethodInfo; }
};

class MCMouseStack : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseStackMethodInfo; }
};

class MCMouseText : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseTextMethodInfo; }
};

class MCMouseV : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseVMethodInfo; }
};

class MCMovie : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalMovieMethodInfo; }
};

class MCMovingControls : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMovingControlsMethodInfo; }
};

class MCNumToChar: public MCFunction
{
	MCExpression *source;
public:
	MCNumToChar()
	{
		source = NULL;
	}
	virtual ~MCNumToChar();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalNumToCharMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCNumToByte: public MCFunction
{
	MCExpression *source;
public:
	MCNumToByte(void)
	{
		source = NULL;
	}
	virtual ~MCNumToByte(void);
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalNumToByteMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCOpenFiles : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalOpenFilesMethodInfo; }
};

class MCOpenProcesses : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalOpenProcessesMethodInfo; }
};

class MCOpenProcessIds : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalOpenProcessesIdsMethodInfo; }
};

class MCOpenSockets : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalOpenSocketsMethodInfo; }
};

class MCOpenStacks : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalOpenStacksMethodInfo; }
};

class MCOptionKey : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalOptionKeyMethodInfo; }
};

class MCParam : public MCUnaryFunction
{
	MCHandler *h;
	MCExpression *source;
public:
	MCParam()
	{
		source = NULL;
	}
	virtual ~MCParam();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalParamMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCParamCount : public MCConstantFunction
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalParamCountMethodInfo; }
};

class MCParams : public MCConstantFunction
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalParamsMethodInfo; }
};

class MCPeerAddress : public MCUnaryFunction
{
	MCExpression *socket;
public:
	MCPeerAddress()
	{
		socket = NULL;
	}
	virtual ~MCPeerAddress();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalPeerAddressMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return socket; }
};

class MCPendingMessages : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalPendingMessagesMethodInfo; }
};

class MCPid : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalProcessIdMethodInfo; }
};

class MCPlatform : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalPlatformMethodInfo; }
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of populationStdDev
class MCPopulationStdDev : public MCFunction
{
	MCParameter *params;
public:
	MCPopulationStdDev()
	{
		params = NULL;
	}
	virtual ~MCPopulationStdDev();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of populationVariance
class MCPopulationVariance : public MCFunction
{
	MCParameter *params;
public:
	MCPopulationVariance()
	{
		params = NULL;
	}
	virtual ~MCPopulationVariance();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCProcessor : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalProcessorMethodInfo; }
};

class MCPropertyNames : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalPropertyNamesMethodInfo; }
};

class MCQTVersion : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCTheResult : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalResultMethodInfo; }
};

class MCScreenColors : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalScreenColorsMethodInfo; }
};

class MCScreenDepth : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalScreenDepthMethodInfo; }
};

class MCScreenLoc : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalScreenLocMethodInfo; }
};

class MCScreenName : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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

	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);

	static void evaluate(MCExecPoint&, bool working, bool plural, bool effective);
};

class MCScreenType : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalScreenTypeMethodInfo; }
};

class MCScreenVendor : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalScreenVendorMethodInfo; }
};

class MCScriptLimits : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalScriptLimitsMethodInfo; }
};

class MCSeconds : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSelectedField : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalSelectedFieldMethodInfo; }
};

class MCSelectedImage : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSelectedObject : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCShell : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCShell()
	{
		source = NULL;
	}
	virtual ~MCShell();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalShellMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCShiftKey : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalShiftKeyMethodInfo; }
};

class MCSound : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalSoundMethodInfo; }
};

class MCStacks : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalStacksMethodInfo; }
};

class MCStackSpace : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalStackSpaceMethodInfo; }
};

class MCSysError : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalSysErrorMethodInfo; }
};

class MCSystemVersion : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalSystemVersionMethodInfo; }
};

class MCTarget : public MCConstantFunction
{
	Boolean contents;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint&);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCTempName : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalTempNameMethodInfo; }
};

class MCTicks : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalTicksMethodInfo; }
};

class MCTheTime : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalTimeMethodInfo; }
};

class MCToLower : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCToLower()
	{
		source = NULL;
	}
	virtual ~MCToLower();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalToLowerMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCToUpper : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCToUpper()
	{
		source = NULL;
	}
	virtual ~MCToUpper();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalToUpperMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
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
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCUrlDecode : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCUrlDecode()
	{
		source = NULL;
	}
	virtual ~MCUrlDecode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalUrlDecodeMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCUrlEncode : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCUrlEncode()
	{
		source = NULL;
	}
	virtual ~MCUrlEncode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalUrlEncodeMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCUrlStatus : public MCUnaryFunction
{
	MCExpression *url;
public:
	MCUrlStatus()
	{
		url = NULL;
	}
	virtual ~MCUrlStatus();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalUrlStatusMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return url; }
};

class MCValue : public MCFunction
{
	MCHandler *h;
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCVariables : public MCConstantFunction
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalVariableNamesMethodInfo; }
};

class MCVersion : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalVersionMethodInfo; }
};

class MCWaitDepth : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalWaitDepthMethodInfo; }
};

class MCWeekDayNames : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

// platform specific functions in funcs.cpp

class MCMCISendString : public MCUnaryFunction
{
	MCExpression *string;
public:
	MCMCISendString()
	{
		string = NULL;
	}
	virtual ~MCMCISendString();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalMCISendStringMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return string; }
};

class MCDeleteRegistry : public MCUnaryFunction
{
	MCExpression *key;
public:
	MCDeleteRegistry()
	{
		key = NULL;
	}
	virtual ~MCDeleteRegistry();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDeleteRegistryMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return key; }
};

class MCListRegistry : public MCUnaryFunction
{
	MCExpression *key;
public:
	MCListRegistry()
	{
		key = NULL;
	}
	virtual ~MCListRegistry();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalListRegistryMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return key; }
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
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSpecialFolderPath : public MCUnaryFunction
{
	MCExpression *type;
public:
	MCSpecialFolderPath()
	{
		type = NULL;
	}
	virtual ~MCSpecialFolderPath();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalSpecialFolderPathMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCShortFilePath : public MCUnaryFunction
{
	MCExpression *type;
public:
	MCShortFilePath()
	{
		type = NULL;
	}
	virtual ~MCShortFilePath();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalShortFilePathMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCLongFilePath : public MCUnaryFunction
{
	MCExpression *type;
public:
	MCLongFilePath()
	{
		type = NULL;
	}
	virtual ~MCLongFilePath();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalLongFilePathMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCAlternateLanguages : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCScriptingEvalAlternateLanguagesMethodInfo; }
};

class MCAliasReference: public MCUnaryFunction
{
	MCExpression *type;
public:
	MCAliasReference()
	{
		type = NULL;
	}
	virtual ~MCAliasReference();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalAliasReferenceMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCCipherNames : public MCConstantFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCSecurityEvalCipherNamesMethodInfo; }
};

// Math functions in funcsm.cpp

class MCAbsFunction : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCAbsFunction()
	{
		source = NULL;
	}
	virtual ~MCAbsFunction();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalAbsMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCAcos : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCAcos()
	{
		source = NULL;
	}
	virtual ~MCAcos();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalAcosMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of arithmeticMean
class MCArithmeticMean : public MCFunction
{
	MCParameter *params;
public:
	MCArithmeticMean()
	{
		params = NULL;
	}
	virtual ~MCArithmeticMean();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCAsin : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCAsin()
	{
		source = NULL;
	}
	virtual ~MCAsin();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalAsinMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCAtan : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCAtan()
	{
		source = NULL;
	}
	virtual ~MCAtan();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalAtanMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of averageDev (was average)
class MCAvgDev : public MCFunction
{
	MCParameter *params;
public:
	MCAvgDev()
	{
		params = NULL;
	}
	virtual ~MCAvgDev();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalAverageMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCCos : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCCos()
	{
		source = NULL;
	}
	virtual ~MCCos();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalCosMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCExp : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCExp()
	{
		source = NULL;
	}
	virtual ~MCExp();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalExpMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCExp1 : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCExp1()
	{
		source = NULL;
	}
	virtual ~MCExp1();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalExp1MethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCExp2 : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCExp2()
	{
		source = NULL;
	}
	virtual ~MCExp2();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalExp2MethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCExp10 : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCExp10()
	{
		source = NULL;
	}
	virtual ~MCExp10();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalExp10MethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of geometricMean
class MCGeometricMean : public MCFunction
{
	MCParameter *params;
public:
	MCGeometricMean()
	{
		params = NULL;
	}
	virtual ~MCGeometricMean();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of harmonicMean
class MCHarmonicMean : public MCFunction
{
	MCParameter *params;
public:
	MCHarmonicMean()
	{
		params = NULL;
	}
	virtual ~MCHarmonicMean();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCLn : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCLn()
	{
		source = NULL;
	}
	virtual ~MCLn();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalLnMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCLn1 : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCLn1()
	{
		source = NULL;
	}
	virtual ~MCLn1();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalLn1MethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCLog2 : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCLog2()
	{
		source = NULL;
	}
	virtual ~MCLog2();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalLog2MethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCLog10 : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCLog10()
	{
		source = NULL;
	}
	virtual ~MCLog10();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalLog10MethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCMaxFunction : public MCParamFunction
{
	MCParameter *params;
public:
	MCMaxFunction()
	{
		params = NULL;
	}
	virtual ~MCMaxFunction();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalMaxMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
};

class MCMedian : public MCParamFunction
{
	MCParameter *params;
public:
	MCMedian()
	{
		params = NULL;
	}
	virtual ~MCMedian();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalMedianMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
};

class MCMD5Digest : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCMD5Digest()
	{
		source = NULL;
	}
	virtual ~MCMD5Digest();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalMD5DigestMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCSHA1Digest : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCSHA1Digest()
	{
		source = NULL;
	}
	virtual ~MCSHA1Digest();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalSHA1DigestMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCMinFunction : public MCParamFunction
{
	MCParameter *params;
public:
	MCMinFunction()
	{
		params = NULL;
	}
	virtual ~MCMinFunction();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalMinMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
};

class MCRandom : public MCUnaryFunction
{
	MCExpression *limit;
public:
	MCRandom()
	{
		limit = NULL;
	}
	virtual ~MCRandom();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalRandomMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return limit; }
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSin : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCSin()
	{
		source = NULL;
	}
	virtual ~MCSin();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalSinMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of sampleStdDev (was stdDev)
class MCSampleStdDev : public MCFunction
{
	MCParameter *params;
public:
	MCSampleStdDev()
	{
		params = NULL;
	}
	virtual ~MCSampleStdDev();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of sampleVariance
class MCSampleVariance : public MCFunction
{
	MCParameter *params;
public:
	MCSampleVariance()
	{
		params = NULL;
	}
	virtual ~MCSampleVariance();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCSqrt : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCSqrt()
	{
		source = NULL;
	}
	virtual ~MCSqrt();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalSqrtMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCStdDev : public MCParamFunction
{
	MCParameter *params;
public:
	MCStdDev()
	{
		params = NULL;
	}
	virtual ~MCStdDev();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalMinMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
};

class MCSum : public MCParamFunction
{
	MCParameter *params;
public:
	MCSum()
	{
		params = NULL;
	}
	virtual ~MCSum();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalSumMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
};

class MCTan : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCTan()
	{
		source = NULL;
	}
	virtual ~MCTan();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalTanMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCTranspose : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCTranspose()
	{
		source = NULL;
	}
	virtual ~MCTranspose();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCArraysEvalTransposeMatrixMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCTrunc : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCTrunc()
	{
		source = NULL;
	}
	virtual ~MCTrunc();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalTruncMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCHTTPProxyForURL: public MCFunction
{
	MCExpression *url;
	MCExpression *host;
	MCExpression *pac;

	static MCScriptEnvironment *pac_engine;

public:
	MCHTTPProxyForURL(void)
	{
		url = NULL;
		host = NULL;
		pac = NULL;
	}

	virtual ~MCHTTPProxyForURL(void);

	virtual Parse_stat parse(MCScriptPoint& sp, Boolean the);
	virtual Exec_stat eval(MCExecPoint& ep);
	virtual void compile(MCSyntaxFactoryRef);

private:
	static char *PACdnsResolve(const char* const* p_arguments, unsigned int p_argument_count);
	static char *PACmyIpAddress(const char* const* p_arguments, unsigned int p_argument_count);
};

class MCRandomBytes: public MCUnaryFunction
{
	MCExpression *byte_count;
public:
	MCRandomBytes(void)
	{
		byte_count = NULL;
	}
	virtual ~MCRandomBytes(void);
	virtual Parse_stat parse(MCScriptPoint &sp, Boolean the);
	virtual Exec_stat eval(MCExecPoint &ep);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCSecurityEvalRandomBytesMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return byte_count; }
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
	
	virtual ~MCControlAtLoc(void);
	virtual Parse_stat parse(MCScriptPoint &sp, Boolean the);
	virtual Exec_stat eval(MCExecPoint &ep);
	
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
	virtual Exec_stat eval(MCExecPoint &ep);
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
	virtual Exec_stat eval(MCExecPoint &ep);
};


#endif



