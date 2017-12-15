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

////////////////////////////////////////////////////////////////////////////////
//
//  Private Header File:
//    ide.h
//
//  Description:
//    This file contains syntax tree classes for the _internal ide commands.
//
//  Changes:
//    2009-06-24 MW Added revlet deployment option to deploy command.
//    2009-07-12 MW Added MCIdeSign command definition.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __IDE__
#define __IDE__

class MCIdeScriptAction : public MCStatement
{
public:
	Parse_stat parse_target(MCScriptPoint& p_script, MCChunk*& r_target);
	Parse_stat parse_target_range(MCScriptPoint& p_script, Chunk_term& r_type, MCExpression*& r_start, MCExpression*& r_end, MCChunk*& r_target);

    bool eval_target(MCExecContext &ctxt, MCChunk *p_target, MCField*& r_field);
    bool eval_target_range(MCExecContext &ctxt, MCExpression *p_start, MCExpression *p_end, MCChunk *p_target, int4& r_start, int4& r_end, MCField*& r_target);
};

class MCIdeScriptFlush : public MCIdeScriptAction
{
public:
	MCIdeScriptFlush(void);
	virtual ~MCIdeScriptFlush(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
    virtual void exec_ctxt(MCExecContext &ctxt);

private:
	MCChunk *f_target;
};

class MCIdeScriptColourize : public MCIdeScriptAction
{
public:
	MCIdeScriptColourize(void);
	virtual ~MCIdeScriptColourize(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
    virtual void exec_ctxt(MCExecContext &ctxt);

private:
	Chunk_term f_type;
	MCExpression *f_start;
	MCExpression *f_end;
	MCChunk *f_target;
};

class MCIdeScriptReplace : public MCIdeScriptAction
{
public:
	MCIdeScriptReplace(void);
	virtual ~MCIdeScriptReplace(void);
	
	virtual Parse_stat parse(MCScriptPoint& p_script);
    virtual void exec_ctxt(MCExecContext &ctxt);
	
private:
	Chunk_term f_type;
	MCExpression *f_start;
	MCExpression *f_end;
	MCChunk *f_target;
	MCExpression *f_text;
};

class MCIdeScriptConfigure : public MCIdeScriptAction
{
public:
	MCIdeScriptConfigure(void);
	virtual ~MCIdeScriptConfigure(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
    virtual void exec_ctxt(MCExecContext &ctxt);

private:
	enum
	{
		TYPE_NONE,
		TYPE_CLASSES,
		TYPE_KEYWORDS,
		TYPE_OPERATORS
	};

	uint4 f_type;
	MCExpression *f_settings;
};

class MCIdeScriptDescribe : public MCIdeScriptAction
{
public:
	MCIdeScriptDescribe(void);
	virtual ~MCIdeScriptDescribe(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
    virtual void exec_ctxt(MCExecContext &ctxt);

private:
	enum
	{
		TYPE_NONE,
		TYPE_CLASSES,
		TYPE_KEYWORDS,
		TYPE_OPERATORS,
		TYPE_STYLES,
		TYPE_CLASS_STYLES,
		TYPE_KEYWORD_STYLES,
		TYPE_OPERATOR_STYLES
	};

	uint4 f_type;
};

class MCIdeScriptStrip : public MCIdeScriptAction
{
public:
	MCIdeScriptStrip(void);
	virtual ~MCIdeScriptStrip(void);
	
	virtual Parse_stat parse(MCScriptPoint& p_script);
    virtual void exec_ctxt(MCExecContext &ctxt);

private:
	Chunk_term f_type;
	MCExpression *f_start;
	MCExpression *f_end;
	MCChunk *f_target;
};

// This command tokenizes the string contents of a container eliminating
// comments and concatenating continuations. The result is a return-separated
// list of separated phrases, each phrase consisting of a tab-separated
// list of tokens.
class MCIdeScriptTokenize: public MCStatement
{
public:
	MCIdeScriptTokenize(void);
	virtual ~MCIdeScriptTokenize(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
    virtual void exec_ctxt(MCExecContext &ctxt);

private:
	MCChunk *m_script;
	bool m_with_location;
};

class MCIdeScriptClassify: public MCStatement
{
public:
	MCIdeScriptClassify(void);
	virtual ~MCIdeScriptClassify(void);
	
	virtual Parse_stat parse(MCScriptPoint& p_script);
    virtual void exec_ctxt(MCExecContext &ctxt);

private:
	MCExpression *m_script;
	MCChunk *m_target;
};

enum MCIdeFilterControlsProperty
{
	kMCIdeFilterPropertyNone,
	kMCIdeFilterPropertyScriptLines,
	kMCIdeFilterPropertyName,
	kMCIdeFilterPropertyVisible,
	kMCIdeFilterPropertyType,
};

enum MCIdeFilterControlsOperator
{
	kMCIdeFilterOperatorNone,
	kMCIdeFilterOperatorLessThan,
	kMCIdeFilterOperatorLessThanOrEqual,
	kMCIdeFilterOperatorEqual,
	kMCIdeFilterOperatorNotEqual,
	kMCIdeFilterOperatorGreaterThanOrEqual,
	kMCIdeFilterOperatorGreaterThan,
	kMCIdeFilterOperatorBeginsWith,
	kMCIdeFilterOperatorEndsWith,
	kMCIdeFilterOperatorContains,
};

class MCIdeFilterControls: public MCStatement
{
public:
	MCIdeFilterControls(void);
	virtual ~MCIdeFilterControls(void);
	
	virtual Parse_stat parse(MCScriptPoint& p_script);
    virtual void exec_ctxt(MCExecContext &ctxt);

private:
	MCIdeFilterControlsProperty m_property;
	MCIdeFilterControlsOperator m_operator;
	MCExpression *m_pattern;
	MCChunk *m_stack;
};

class MCIdeDeploy: public MCStatement
{
public:
	MCIdeDeploy(void);
	virtual ~MCIdeDeploy(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
	virtual void exec_ctxt(MCExecContext& p_ctxt);

private:
	enum
	{
        PLATFORM_NONE,
        
		PLATFORM_WINDOWS,
		PLATFORM_LINUX,
		PLATFORM_MACOSX,

		PLATFORM_IOS,
		PLATFORM_ANDROID,
		PLATFORM_WINMOBILE,
		PLATFORM_LINUXMOBILE,

		PLATFORM_IOS_EMBEDDED,
		PLATFORM_ANDROID_EMBEDDED,

		PLATFORM_EMSCRIPTEN,
		
		PLATFORM_SERVER,
	};

	uint32_t m_platform;
	MCExpression *m_params;
};

class MCIdeSign: public MCStatement
{
public:
	MCIdeSign(void);
	virtual ~MCIdeSign(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
	virtual void exec_ctxt(MCExecContext& p_ctxt);

private:
	enum
	{
        PLATFORM_NONE,
		PLATFORM_WINDOWS,
		PLATFORM_LINUX,
		PLATFORM_MACOSX
	};

	uint32_t m_platform;
	MCExpression *m_params;
};

class MCIdeDiet: public MCStatement
{
public:
	MCIdeDiet(void);
	virtual ~MCIdeDiet(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
	virtual void exec_ctxt(MCExecContext& p_ctxt);

private:
	enum
	{
        PLATFORM_NONE,
		PLATFORM_WINDOWS,
		PLATFORM_LINUX,
		PLATFORM_MACOSX
	};

	uint32_t m_platform;
	MCExpression *m_params;
};

class MCIdeDmgDump: public MCStatement
{
public:
	MCIdeDmgDump(void);
	virtual ~MCIdeDmgDump(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
	virtual void exec_ctxt(MCExecContext& p_ctxt);

private:
	MCExpression *m_filename;
};

class MCIdeDmgBuild: public MCStatement
{
public:
	MCIdeDmgBuild(void);
	virtual ~MCIdeDmgBuild(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
	virtual void exec_ctxt(MCExecContext& p_ctxt);

private:
	MCExpression *m_items;
	MCExpression *m_filename;
};

class MCIdeExtract: public MCStatement
{
public:
	MCIdeExtract(void);
	virtual ~MCIdeExtract(void);
	
	virtual Parse_stat parse(MCScriptPoint& p_script);
	virtual void exec_ctxt(MCExecContext& p_ctxt);
	
private:
	MCExpression *m_filename;
	MCExpression *m_segment_name;
	MCExpression *m_section_name;
};

//////////

class MCIdeSyntaxTokenize: public MCStatement
{
public:
	MCIdeSyntaxTokenize(void);
	virtual ~MCIdeSyntaxTokenize(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
    virtual void exec_ctxt(MCExecContext &ctxt);

private:
	MCChunk *m_script;
};

class MCIdeSyntaxRecognize: public MCStatement
{
public:
	MCIdeSyntaxRecognize(void);
	virtual ~MCIdeSyntaxRecognize(void);

	virtual Parse_stat parse(MCScriptPoint& p_script);
    virtual void exec_ctxt(MCExecContext &ctxt);

private:
	MCExpression *m_script;
	MCExpression *m_language;
};


#endif
