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

class MCFunction : public MCExpression
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	Parse_stat parsetarget(MCScriptPoint &spt, Boolean the,
	                       Boolean needone, MCChunk *&chunk);
	Exec_stat evalparams(Functions func, MCParameter *params, MCExecPoint &);
};

// non-math functions in funcs.cpp

class MCArrayDecode: public MCFunction
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
};

class MCArrayEncode: public MCFunction
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
};

class MCBase64Decode : public MCFunction
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
};

class MCBase64Encode : public MCFunction
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
};

class MCBuildNumber : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCCachedUrls : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCCapsLockKey : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCCharToNum : public MCFunction
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
};

class MCByteToNum : public MCFunction
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
};

class MCClipboard : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCCompress : public MCFunction
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
};

class MCConstantNames : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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

class MCClickChar : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCClickCharChunk : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCClickChunk : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCClickField : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCClickH : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCClickLine : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCClickLoc : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCClickStack : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCClickText : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCClickV : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCColorNames : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCCommandNames : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCCommandKey : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCControlKey : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCDate : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCDateFormat : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCDecompress : public MCFunction
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
};

class MCDirectories : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCDiskSpace : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCDNSServers : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCDragDestination: public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCDragSource: public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCDriverNames : public MCFunction
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
};

class MCDrives : public MCFunction
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
};

class MCDropChunk: public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCQTEffects : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCRecordCompressionTypes: public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCRecordLoudness : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCEnvironment : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCEncrypt : public MCFunction
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
};

class MCExtents : public MCFunction
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
};

class MCTheFiles : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCFlushEvents : public MCFunction
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
};

class MCFocusedObject : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCFontNames : public MCFunction
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
};

class MCFontLanguage : public MCFunction
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
};


class MCFontSizes : public MCFunction
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
};

class MCFoundChunk : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCFoundField : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCFoundLine : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCFoundLoc : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCFoundText : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCFunctionNames : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCGlobalLoc : public MCFunction
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
};

class MCGlobals : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCHasMemory : public MCFunction
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
};

class MCHeapSpace : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCHostAddress : public MCFunction
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
};

class MCHostAtoN : public MCFunction
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
};

class MCHostName : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
};

class MCInsertScripts : public MCFunction
{
protected:
	Boolean front;
public:
	virtual Exec_stat eval(MCExecPoint &);
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

class MCInterrupt : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
};

class MCIsoToMac : public MCFunction
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
};

class MCIsNumber : public MCFunction
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
};

class MCKeysDown : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCLength : public MCFunction
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
};

class MCLicensed : public MCFunction
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
};

class MCLocalLoc : public MCFunction
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
};

class MCLocals : public MCFunction
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMachine : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMacToIso : public MCFunction
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
};

class MCMainStacks : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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


class MCMe : public MCFunction
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMenuObject : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMenus : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMerge : public MCFunction
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
};

class MCMillisecs : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMonthNames : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
};

class MCMouseChar : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMouseCharChunk : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMouseChunk : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMouseClick : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMouseColor : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMouseControl : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMouseH : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMouseLine : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMouseLoc : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMouseStack : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMouseText : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMouseV : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMovie : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMovingControls : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
};

class MCOpenFiles : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCOpenProcesses : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCOpenProcessIds : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCOpenSockets : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCOpenStacks : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCOptionKey : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCParam : public MCFunction
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
};

class MCParamCount : public MCFunction
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCParams : public MCFunction
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCPeerAddress : public MCFunction
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
};

class MCPendingMessages : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCPid : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCPlatform : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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

class MCProcessor : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCPropertyNames : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCQTVersion : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
};

class MCTheResult : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCScreenColors : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCScreenDepth : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCScreenLoc : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCScreenName : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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

	static void evaluate(MCExecPoint&, bool working, bool plural, bool effective);
};

class MCScreenType : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCScreenVendor : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCScriptLimits : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCSeconds : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
};

class MCSelectedField : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCSelectedImage : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
};

class MCSelectedObject : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
};

class MCShell : public MCFunction
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
};

class MCShiftKey : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCSound : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCStacks : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCStackSpace : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCSysError : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCSystemVersion : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCTarget : public MCFunction
{
	Boolean contents;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
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
};

class MCTempName : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCTicks : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCTheTime : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCToLower : public MCFunction
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
};

class MCToUpper : public MCFunction
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
};

class MCUrlDecode : public MCFunction
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
};

class MCUrlEncode : public MCFunction
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
};

class MCUrlStatus : public MCFunction
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
};

class MCVariables : public MCFunction
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCVersion : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCWaitDepth : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCWeekDayNames : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
};

class MCDeleteRegistry : public MCFunction
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
};

class MCListRegistry : public MCFunction
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
};

class MCSpecialFolderPath : public MCFunction
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
};

class MCShortFilePath : public MCFunction
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
};

class MCLongFilePath : public MCFunction
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
};

class MCAlternateLanguages : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

class MCAliasReference: public MCFunction
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
};

class MCCipherNames : public MCFunction
{
public:
	virtual Exec_stat eval(MCExecPoint &);
};

// Math functions in funcsm.cpp

class MCAbsFunction : public MCFunction
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
};

class MCAcos : public MCFunction
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

class MCAsin : public MCFunction
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
};

class MCAtan : public MCFunction
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
};

class MCCos : public MCFunction
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
};

class MCExp : public MCFunction
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
};

class MCExp1 : public MCFunction
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
};

class MCExp2 : public MCFunction
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
};

class MCExp10 : public MCFunction
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

class MCLn : public MCFunction
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
};

class MCLn1 : public MCFunction
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
};

class MCLog2 : public MCFunction
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
};

class MCLog10 : public MCFunction
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
};

class MCMaxFunction : public MCFunction
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
};

class MCMedian : public MCFunction
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
};

class MCMD5Digest : public MCFunction
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
};

class MCSHA1Digest : public MCFunction
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
};

class MCMinFunction : public MCFunction
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
};

class MCRandom : public MCFunction
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
};

class MCSin : public MCFunction
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

class MCSqrt : public MCFunction
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
};

class MCSum : public MCFunction
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
};

class MCTan : public MCFunction
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
};

class MCTranspose : public MCFunction
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
};

class MCTrunc : public MCFunction
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

private:
	static char *PACdnsResolve(const char* const* p_arguments, unsigned int p_argument_count);
	static char *PACmyIpAddress(const char* const* p_arguments, unsigned int p_argument_count);
};

class MCRandomBytes: public MCFunction
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
};

// MW-2012-10-08: [[ HitTest ]] controlAtLoc and controlAtScreenLoc function.
class MCControlAtLoc: public MCFunction
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



