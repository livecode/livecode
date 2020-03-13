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

#ifndef	COMMANDS_H
#define	COMMANDS_H

#include "statemnt.h"
#include "objdefs.h"
#include "express.h"
#include "regex.h"
#include "util.h"
#include "uidc.h"
#include "variable.h"
#include "chunk.h"

// general commands in cmds.cc

// MW-2013-11-14: [[ AssertCmd ]] 'assert' command definition.
class MCAssertCmd: public MCStatement
{
	Assert_type m_type;
	MCExpression *m_expr;
	
public:
	MCAssertCmd(void)
	{
		m_type = ASSERT_TYPE_NONE;
		m_expr = nil;
	}
	
	virtual ~MCAssertCmd(void);
	virtual Parse_stat parse(MCScriptPoint& sp);
	virtual void exec_ctxt(MCExecContext& ctxt);
};

class MCChoose : public MCStatement
{
	Tool littool;
	MCExpression *etool;
public:
	MCChoose()
	{
		littool = T_UNDEFINED;
		etool = NULL;
	}
	virtual ~MCChoose();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext& ctxt);
};

class MCConvert : public MCStatement
{
	MCChunk *container;
	MCExpression *source;
	Convert_form fform;
	Convert_form fsform;
	Convert_form pform;
	Convert_form sform;
public:
	MCConvert()
	{
		container = NULL;
		source = NULL;
		fform = CF_UNDEFINED;
		fsform = CF_UNDEFINED;
		pform = CF_UNDEFINED;
		sform = CF_UNDEFINED;
	}
	virtual ~MCConvert();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
	Parse_stat parsedtformat(MCScriptPoint &sp, Convert_form &firstform,
	                         Convert_form &secondform);
};

class MCDo : public MCStatement
{
	MCExpression *source;
	MCExpression *alternatelang;
	MCChunk *widget;
protected:
	bool browser : 1;
	bool debug : 1;
	bool caller : 1;
public:
	MCDo()
	{
		source = NULL;
		alternatelang = NULL;
		browser = false;
		debug = False;
		caller = false;
		widget = nil;
	}
	virtual ~MCDo();
	virtual Parse_stat parse(MCScriptPoint &);
	void deletestatements(MCStatement *statements);
	virtual void exec_ctxt(MCExecContext& ctxt);
};

class MCDebugDo : public MCDo
{
public:
	MCDebugDo()
	{
		debug = True;
	}
};

class MCRegularDo : public MCDo
{
public:
	MCRegularDo()
	{
		debug = False;
	}
};

class MCDoMenu : public MCStatement
{
	MCExpression *source;
public:
	MCDoMenu()
	{
		source = NULL;
	}
	const char *lookup(MCStringRef s);
	virtual ~MCDoMenu();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext& ctxt);
};

class MCEdit : public MCStatement
{
	MCChunk *target;
    // MERG 2013-9-13: [[ EditScriptChunk ]] Added at expression that's passed through as a second parameter to editScript
    MCExpression *m_at;
public:
	MCEdit()
	{
		target = NULL;
        m_at = NULL;
    }
	virtual ~MCEdit();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCFind : public MCStatement
{
	Find_mode mode;
	MCExpression *tofind;
	MCChunk *field;
public:
	MCFind()
	{
		mode = FM_NORMAL;
		tofind = NULL;
		field = NULL;
	}
	virtual ~MCFind();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCGet : public MCStatement
{
	MCExpression *value;
public:
	MCGet()
	{
		value = NULL;
	}
	virtual ~MCGet();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext&);
};

class MCMarking : public MCStatement
{
	MCExpression *where;
	MCExpression *tofind;
	MCChunk *field;
	MCChunk *card;
	Find_mode mode;
protected:
	Boolean mark;
public:
	MCMarking()
	{
		where = tofind = NULL;
		mode = FM_NORMAL;
		field = card = NULL;
	}
	virtual ~MCMarking();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCMarkCommand : public MCMarking
{
public:
	MCMarkCommand()
	{
		mark = True;
	}
};

class MCUnmark : public MCMarking
{
public:
	MCUnmark()
	{
		mark = False;
	}
};

class MCPost : public MCStatement
{
	MCExpression *source;
	MCExpression *dest;
public:
	MCPost()
	{
		source = dest = NULL;
	}
	virtual ~MCPost();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCPut : public MCStatement
{
	MCExpression *source;
	MCChunk *dest;
	Preposition_type prep : 8;
	// MW-2012-02-23: [[ UnicodePut ]] Indicates if the 'unicode' adjective
	//   was present.
	bool is_unicode : 1;

	//cookie
	MCExpression *name;
	MCExpression *path;
	MCExpression *domain;
	MCExpression *expires;
	bool is_secure;
	bool is_httponly;
public:
	MCPut()
	{
		source = NULL;
		// MW-2011-06-22: [[ SERVER ]] Make a distinction between 'put' and 'put .. into msg'
		prep = PT_UNDEFINED;
		dest = NULL;
		// cookie
		name = NULL;
		path = NULL;
		domain = NULL;
		expires = NULL;
		is_secure = false;
		is_httponly = false;
		
		// MW-2012-02-23: [[ UnicodePut ]] Assume non-unicode to begin with.
		is_unicode = false;
	}
	virtual ~MCPut();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCQuit : public MCStatement
{
	MCExpression *retcode;
public:
	MCQuit()
	{
		retcode = NULL;
	}
	virtual ~MCQuit();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext&);
};

class MCReset : public MCStatement
{
	Reset_type which;
public:
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCReturn : public MCStatement
{
    enum Kind
    {
        kReturn,
        kReturnValue,
        kReturnError,
        kReturnWithUrlResult,
    };
	MCExpression *source;
	MCExpression *extra_source;
    Kind kind;
public:
	MCReturn()
	{
        source = NULL;
        extra_source = NULL;
        kind = kReturn;
	}
	virtual ~MCReturn();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
	virtual uint4 linecount();
};

// relayer <control> ( before | after ) layer <expr>
// relayer <control> ( before | after ) <control>
// relayer <control> ( before | after ) owner
// relayer <control> to ( front | back ) of layer <expr>
// relayer <control> to ( front | back ) of <control>
// relayer <control> to ( front | back ) of owner
enum MCRelayerForm
{
	kMCRelayerFormNone,
	kMCRelayerFormRelativeToLayer,
	kMCRelayerFormRelativeToControl,
	kMCRelayerFormRelativeToOwner,
};

class MCRelayer : public MCStatement
{
	MCRelayerForm form : 3;
	Relayer_relation relation : 4;
	MCChunk *control;
	union
	{
		MCExpression *layer;
		MCChunk *target;
	};

public:
	MCRelayer(void);
	virtual ~MCRelayer(void);

	virtual Parse_stat parse(MCScriptPoint& sp);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCScriptError : public MCStatement
{
public:
	virtual Parse_stat parse(MCScriptPoint &);
};

class MCSet : public MCStatement
{
	MCProperty *target;
	MCExpression *value;
public:
	MCSet()
	{
		target = NULL;
		value = NULL;
	}
	virtual ~MCSet();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCSort : public MCStatement
{
	MCChunk *of;
	Chunk_term chunktype;
	Sort_type direction;
	Sort_type format;
	MCExpression *by;
public:
	MCSort()
	{
		of = NULL;
		chunktype = CT_FIELD;
		direction = ST_ASCENDING;
		format = ST_TEXT;
		by = NULL;
	}
	virtual ~MCSort();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &);
};

class MCWait : public MCStatement
{
	Repeat_form condition;
	MCExpression *duration;
	Functions units;
	Boolean messages;
public:
	MCWait()
	{
		duration = NULL;
		messages = False;
	}
	virtual ~MCWait();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

// creation related commands in cmdsc.cc

class MCClone : public MCStatement
{
	MCChunk *source;
	MCExpression *newname;
	Boolean visible;
public:
	MCClone()
	{
		source = NULL;
		newname = NULL;
		visible = True;
	}
	virtual ~MCClone();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCClipboardCmd: public MCStatement
{
	MCChunk *targets;
	MCChunk *dest;

public:
	MCClipboardCmd(void)
	{
		targets = NULL;
		dest = NULL;
	}

	virtual ~MCClipboardCmd(void);

	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);

protected:
	virtual bool iscut(void) const = 0;

private:
};

class MCCopyCmd: public MCClipboardCmd
{
protected:
	virtual bool iscut(void) const;
};

class MCCutCmd: public MCClipboardCmd
{
protected:
	virtual bool iscut(void) const;
};

// MW-2004-11-26: Initialise otype (VG)
class MCCreate : public MCStatement
{
	Chunk_term otype;
	MCExpression *newname;
	MCExpression *file;
    MCExpression *kind;
    MCChunk *container;
    bool directory: 1;
    bool visible: 1;
    bool alias: 1;
    // MW-2014-09-30: [[ ScriptOnlyStack ]] For 'create script only stack ...' form.
    bool script_only_stack : 1;
public:
	MCCreate()
	{
		otype = CT_UNDEFINED;
		newname = NULL;
		file = NULL;
        kind = NULL;
		container = NULL;
		directory = False;
		alias = False;
		visible = True;
        // MW-2014-09-30: [[ ScriptOnlyStack ]] Initial value.
        script_only_stack = False;
	}
	virtual ~MCCreate();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
	MCControl *getobject(MCObject *&parent);
};

class MCCustomProp : public MCStatement
{
	MCExpression *prop;
	MCChunk *target;
protected:
	Boolean define;
public:
	MCCustomProp()
	{
		prop = NULL;
		target = NULL;
	}
	virtual ~MCCustomProp();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &);
};

class MCDefine : public MCCustomProp
{
public:
	MCDefine()
	{
		define = True;
	}
};

class MCUndefine : public MCCustomProp
{
public:
	MCUndefine()
	{
		define = False;
	}
};

class MCDelete : public MCStatement
{
	MCExpression *file;
	MCChunk *targets;
	Boolean directory;
	Boolean url;
	MCVarref *var;
	bool session;
public:
	MCDelete()
	{
		file = NULL;
		targets = NULL;
		directory = url = False;
		var = NULL;
		session = false;
	}
	virtual ~MCDelete();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCChangeProp : public MCStatement
{
protected:
	MCChunk *targets;
	Properties prop;
	Boolean value;
public:
	MCChangeProp()
	{
		targets = NULL;
	}
	virtual ~MCChangeProp();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCDisable : public MCChangeProp
{
public:
	MCDisable()
	{
		prop = P_DISABLED;
		value = True;
	}
};

class MCEnable : public MCChangeProp
{
public:
	MCEnable()
	{
		prop = P_DISABLED;
		value = False;
	}
};

class MCHilite : public MCChangeProp
{
public:
	MCHilite()
	{
		prop = P_HILITE;
		value = True;
	}
};

class MCUnhilite : public MCChangeProp
{
public:
	MCUnhilite()
	{
		prop = P_HILITE;
		value = False;
	}
};

class MCCrop : public MCStatement
{
	MCExpression *newrect;
	MCChunk *image;
public:
	MCCrop()
	{
		image = NULL;
		newrect = NULL;
	}
	virtual ~MCCrop();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCFlip : public MCStatement
{
	Flip_dir direction;
	MCChunk *image;
public:
	MCFlip()
	{
		direction = FL_UNDEFINED;
		image = NULL;
	}
	virtual ~MCFlip();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCGrab : public MCStatement
{
	MCChunk *control;
public:
	MCGrab()
	{
		control = NULL;
	}
	virtual ~MCGrab();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCLaunch : public MCStatement
{
	MCExpression *doc;
	MCExpression *app;
	MCChunk *widget;
	bool as_url;

public:
	MCLaunch()
	{
		doc = app = NULL;
		as_url = false;
		widget = nil;
	}
	virtual ~MCLaunch();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCLoad : public MCStatement
{
	MCExpression *url;
	MCExpression *message;
    bool is_extension : 1;
	bool has_resource_path : 1;
    bool from_data : 1;
public:
	MCLoad()
	{
		url = message = NULL;
        is_extension = false;
		has_resource_path = false;
        from_data = false;
	}
	virtual ~MCLoad();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCUnload : public MCStatement
{
	MCExpression *url;
    bool is_extension : 1;
public:
	MCUnload()
	{
		url = NULL;
        is_extension = false;
	}
	virtual ~MCUnload();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCMakeGroup : public MCStatement
{
	MCChunk *targets;
public:
	MCMakeGroup()
	{
		targets = NULL;
	}
	virtual ~MCMakeGroup();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCPasteCmd : public MCStatement
{
public:
	MCPasteCmd()
	{
	}
	virtual ~MCPasteCmd();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCPlace : public MCStatement
{
	MCChunk *group;
	MCChunk *card;
public:
	MCPlace()
	{
		group = NULL;
		card = NULL;
	}
	virtual ~MCPlace();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCRecord : public MCStatement
{
	MCExpression *file;
    Boolean pause;
public:
	MCRecord()
	{
		file = NULL;
        pause = False;
	}
	virtual ~MCRecord();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCRedo : public MCStatement
{
public:
    virtual void exec_ctxt(MCExecContext &);
};

class MCRemove : public MCStatement
{
	MCChunk *target;
	MCChunk *card;
	Boolean script;
	Boolean all;
	Insert_point where;
public:
	MCRemove()
	{
		target = NULL;
		card = NULL;
		script = all = False;
	}
	virtual ~MCRemove();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCRename : public MCStatement
{
	MCExpression *source;
	MCExpression *dest;
public:
	MCRename()
	{
		source = dest = NULL;
	}
	virtual ~MCRename();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCReplace : public MCStatement
{
	enum Mode
	{
		kIgnoreStyles,
		kReplaceStyles,
		kPreserveStyles,
	};
	
	MCExpression *pattern;
	MCExpression *replacement;
	MCChunk *container;
	Mode mode;
	
public:
	MCReplace()
	{
		pattern = replacement = NULL;
		container = NULL;
		mode = kIgnoreStyles;
	}
	virtual ~MCReplace();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCRevert : public MCStatement
{
    MCChunk *stack;
public:
    MCRevert() : stack(NULL) {}
    virtual ~MCRevert();
    virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCRotate : public MCStatement
{
	MCExpression *angle;
	MCChunk *image;
public:
	MCRotate()
	{
		image = NULL;
		angle = NULL;
	}
	virtual ~MCRotate();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCSelect : public MCStatement
{
	Preposition_type where;
	Boolean text;
	MCChunk *targets;
public:
	MCSelect()
	{
		where = PT_AT;
		text = False;
		targets = NULL;
	}
	virtual ~MCSelect();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCUngroup : public MCStatement
{
	MCChunk *group;
public:
	MCUngroup()
	{
		group = NULL;
	}
	virtual ~MCUngroup();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCUndoCmd : public MCStatement
{
public:
	virtual void exec_ctxt(MCExecContext &);
};

// event related comands in cmdse.cc

class MCAccept : public MCStatement
{
	MCExpression *port;
	MCExpression *message;
	Boolean datagram;
	Boolean secure;
	Boolean secureverify;
	MCExpression *certificate;
public:
	MCAccept()
	{
		port = message = NULL;
		datagram = False;
		secure = False;
		certificate = NULL;
		secureverify = False;
	}
	virtual ~MCAccept();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCBeep : public MCStatement
{
	MCExpression *times;
public:
	MCBeep()
	{
		times = NULL;
	}
	virtual ~MCBeep();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext& ctxt);
};

class MCBreakPoint : public MCStatement
{
public:
	virtual void exec_ctxt(MCExecContext &);
};

class MCCancel : public MCStatement
{
	MCExpression *m_id;
public:
	MCCancel()
	{
		m_id = NULL;
	}
	virtual ~MCCancel();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCClickCmd : public MCStatement
{
	MCExpression *button;
	MCExpression *location;
	uint2 mstate;
	uint2 which;
public:
	MCClickCmd()
	{
		button = location = NULL;
		which = 1;
		mstate = 0;
	}
	virtual ~MCClickCmd();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCDrag : public MCStatement
{
	MCExpression *button;
	MCExpression *startloc;
	MCExpression *endloc;
	MCExpression *duration;
	uint2 mstate;
	uint2 which;
public:
	MCDrag()
	{
		button = startloc = endloc = duration = NULL;
		which = 1;
		mstate = 0;
	}
	virtual ~MCDrag();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

// MW-2008-11-05: [[ Dispatch Command ]] The statement class for the 'dispatch' command.
//   'dispatch' [ command | function ] <message> [ 'to' <target> ] [ 'with' <arguments> ]
class MCDispatchCmd: public MCStatement
{
	MCExpression *message;
	MCChunk *target;
	MCParameter *params;
    struct
    {
        /* The container count is the number of containers needed to execute
         * the command. It is calculated after parsing the node. */
        unsigned container_count : 16;
        bool is_function : 1;
    };

public:
	MCDispatchCmd(void)
	{
		message = NULL;
		target = NULL;
		params = NULL;
		is_function = false;
        container_count = 0;
	}
	~MCDispatchCmd(void);
	
	virtual Parse_stat parse(MCScriptPoint& sp);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCLogCmd: public MCStatement
{
    MCParameter *params;
    struct
    {
        unsigned container_count : 16;
    };
    
public:
    MCLogCmd(void)
    {
        params = nullptr;
        container_count = 0;
    }
    ~MCLogCmd(void);
    
    virtual Parse_stat parse(MCScriptPoint& sp);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCFocus : public MCStatement
{
	MCChunk *object;
public:
	MCFocus()
	{
		object = NULL;
	}
	virtual ~MCFocus();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCInsert : public MCStatement
{
	MCChunk *target;
	Insert_point where;
public:
	MCInsert()
	{
		target = NULL;
	}
	virtual ~MCInsert();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCMessage : public MCStatement
{
	MCAutoPointer<MCExpression> message;
	MCAutoPointer<MCExpression> eventtype;
	MCAutoPointer<MCChunk> target;
	MCAutoPointer<MCExpression> in;
	Functions units;
	Boolean program;
	Boolean reply;
	Boolean send;
    Boolean script;
public:
    MCMessage(Boolean p_send) :
        message(nullptr),
        eventtype(nullptr),
        target(nullptr),
        in(nullptr),
        units(F_TICKS),
        program(False),
        reply(True),
        script(False)
	{
        send = p_send;
    }
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCCall : public MCMessage
{
public:
    MCCall() : MCMessage(False)
	{
    }
};

class MCSend : public MCMessage
{
public:
    MCSend(): MCMessage(True)
	{
	}
};

class MCMove : public MCStatement
{
	MCChunk *object;
	MCExpression *startloc;
	MCExpression *endloc;
	MCExpression *durationexp;
	Functions units;
	Boolean relative;
	Boolean messages;
	Boolean waiting;
public:
	MCMove()
	{
		object = NULL;
		startloc = endloc = durationexp = NULL;
		relative = False;
		messages = waiting = True;
	}
	virtual ~MCMove();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCMM : public MCStatement
{
	MCExpression *clip;
	MCExpression *tempo;
	MCExpression *loc;
	MCExpression *options;
	MCChunk *stack;
	Boolean audio;
	Boolean video;
	Boolean player;
	Boolean pause;
	Boolean resume;
	Boolean stepforward;
	Boolean stepback;
	Boolean looping;
	Boolean stop;
	Boolean image;
	Boolean image_file;
	Chunk_term ptype;
	Chunk_term etype;
protected:
	Boolean prepare;
public:
	MCMM()
	{
		clip = tempo = loc = options = NULL;
		stack = NULL;
		stop = looping = audio = video = player = pause = resume
		                                 = stepforward = stepback = image = image_file = False;
		ptype = CT_CARD;
		etype = CT_EXPRESSION;
	}
	virtual ~MCMM();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCPlay : public MCMM
{
public:
	MCPlay()
	{
		prepare = False;
	}
};

class MCPrepare : public MCMM
{
public:
	MCPrepare()
	{
		prepare = True;
	}
};

class MCReply : public MCStatement
{
	MCExpression *message;
	MCExpression *keyword;
	Boolean error;
public:
	MCReply()
	{
		message = keyword = NULL;
		error = False;
	}
	virtual ~MCReply();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCRequest : public MCStatement
{
	MCExpression *message;
	MCExpression *program;
	Apple_event ae;
public:
	MCRequest()
	{
		message = program = NULL;
		ae = AE_UNDEFINED;
	}
	virtual ~MCRequest();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCStart : public MCStatement
{
	MCChunk *target;
	MCExpression *stack;
    // TD-2013-06-12: [[ DynamicFonts ]] Property to store font path
    MCExpression *font;
    bool is_globally : 1;
protected:
	Start_constants mode;
public:
	MCStart()
	{
		target = NULL;
		stack = NULL;
		mode = SC_UNDEFINED;
        // TD-2013-06-20: [[ DynamicFonts ]] Property to store font path
        font = NULL;
        is_globally = 0;
	}
	virtual ~MCStart();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCLibrary : public MCStart
{
public:
	MCLibrary()
	{
		mode = SC_USING;
	}
};

class MCStop : public MCStatement
{
	MCChunk *target;
	MCExpression *stack;
    // TD-2013-06-20: [[ DynamicFonts ]] Property to store font path
    MCExpression *font;
	Start_constants mode;
public:
	MCStop()
	{
		target = NULL;
		stack = NULL;
        // TD-2013-06-20: [[ DynamicFonts ]] Property to store font path
        font = NULL;
	}
	virtual ~MCStop();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCType : public MCStatement
{
	MCExpression *message;
	MCExpression *duration;
	uint2 mstate;
public:
	MCType()
	{
		message = duration = NULL;
		mstate = 0;
	}
	virtual ~MCType();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

// file related comands in cmdsf.cc

class MCClose : public MCStatement
{
	Open_argument arg;
	MCExpression *fname;
	MCChunk *stack;
public:
	MCClose()
	{
		fname = NULL;
		stack = NULL;
		arg = OA_OBJECT;
	}
	virtual ~MCClose();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

// MW-2004-11-26: Initialise format and sformat (VG)
class MCExport : public MCStatement
{
	Export_format format;
	Export_format sformat;
	MCExpression *exsrect;
	MCExpression *exsstack;
	MCExpression *exsdisplay;
	MCExpression *fname;
	MCExpression *mname;
	MCImagePaletteType palette_type;
	MCExpression *palette_color_list;
	MCExpression *palette_color_count;
	MCChunk *image;
	MCChunk *dest;
	MCExpression *size;
	bool with_effects : 1;
    // MERG-2014-07-11: metadata array
    MCExpression *metadata;
public:
	MCExport()
	{
		format = EX_UNDEFINED;
		sformat = EX_UNDEFINED;
		fname = NULL;
		mname = NULL;
		image = NULL;
		dest = NULL;
		exsstack = NULL;
		exsrect = NULL;
		exsdisplay = NULL;
		palette_type = kMCImagePaletteTypeEmpty;
		palette_color_list = NULL;
		palette_color_count = NULL;
		with_effects = false;
		size = NULL;
        // MERG-2014-07-11: metadata array
        metadata = NULL;
	}
	virtual ~MCExport();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCEncryptionOp : public MCStatement
{
	MCExpression *ciphername;
	MCExpression *source;
	MCExpression *keystr;
	MCExpression *keylen;
	MCExpression *salt;
	MCExpression *iv;
	Boolean ispassword;

	bool is_rsa;
	RSA_KEYTYPE rsa_keytype;
	MCExpression *rsa_key;
	MCExpression *rsa_passphrase;
protected:
	Boolean isdecrypt;
public:
	MCEncryptionOp()
	{
		source = ciphername = keystr = keylen = NULL;
		salt = NULL;
		iv = NULL;
		rsa_key = rsa_passphrase = NULL;
	}
	virtual ~MCEncryptionOp();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCCipherEncrypt : public MCEncryptionOp
{
public:
	MCCipherEncrypt()
	{
		isdecrypt = False;
	}
};

class MCCipherDecrypt : public MCEncryptionOp
{
public:
	MCCipherDecrypt()
	{
		isdecrypt = True;
	}
};

class MCFilter : public MCStatement
{
	// JS-2013-07-01: [[ EnhancedFilter ]] Type of the filter (items or lines).
	Chunk_term chunktype;
	MCChunk *container;
	// JS-2013-07-01: [[ EnhancedFilter ]] Optional output container (into ... clause).
	MCChunk *target;
	// JS-2013-07-01: [[ EnhancedFilter ]] Source expression if source not a container.
	MCExpression *source;
	MCExpression *pattern;
	// JS-2013-07-01: [[ EnhancedFilter ]] Whether to use regex or wildcard pattern matcher.
	Match_mode matchmode;
	// JS-2013-07-01: [[ EnhancedFilter ]] Whether it is 'matching' (False) or 'not matching' (True)
	Boolean discardmatches;
public:
	MCFilter()
	{
		chunktype = CT_UNDEFINED;
		container = NULL;
		target = NULL;
		source = NULL;
		pattern = NULL;
		matchmode = MA_UNDEFINED;
		discardmatches = False;
	}
	virtual ~MCFilter();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &);
};

class MCImport : public MCStatement
{
	Export_format format;
	MCExpression *fname;
	MCExpression *mname;
	MCExpression *dname;
	MCChunk *container;
	MCExpression *size;
	bool with_effects : 1;
public:
	MCImport()
	{
		fname = mname = dname = NULL;
		container = NULL;
		size = NULL;
		with_effects = false;
	}
	virtual ~MCImport();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCKill : public MCStatement
{
	MCExpression *sig;
	MCExpression *pname;
public:
	MCKill()
	{
		sig = NULL;
		pname = NULL;
	}
	virtual ~MCKill();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCOpen : public MCStatement
{
	MCExpression *fname;
	MCExpression *message;
    MCExpression *encoding;
	union
	{
		MCExpression *certificate; // if open secure socket
		MCExpression *options; // if open printing to
	};
	MCGo *go;
	MCStringRef destination;
	Open_argument arg : 4;
	Open_mode mode : 4;
	bool dialog : 1;
	bool datagram : 1;
	bool sheet : 1;
	bool secure : 1;
	bool secureverify : 1;
    bool textmode : 1;

	// MW-2010-05-09: Indicates that the process should be opened with elevated
	//   (admin) permissions
	bool elevated : 1;
	
	// MM-2014-06-13: [[ Bug 12567 ]] Added new "open socket <socket> with verification for <host>" variant.
	MCExpression *verifyhostname;
    MCAutoPointer<MCExpression> fromaddress;
public:
	MCOpen()
	{
		fname = message = NULL;
		mode = OM_UPDATE;
        encoding = NULL;
		datagram = dialog = sheet = False;
		go = NULL;
		certificate = NULL;
		secure = False;
		secureverify = True;
		destination = nil;
		elevated = False;
        textmode = True;		
		verifyhostname = NULL;
	}
	virtual ~MCOpen();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCRead : public MCStatement
{
	Open_argument arg;
	MCExpression *fname;
	Repeat_form cond;
	MCExpression *stop;
	File_unit unit;
	MCExpression *maxwait;
	Functions timeunits;
	MCExpression *at;
public:
    MCRead() :
      arg(OA_UNDEFINED),
      fname(NULL),
      cond(RF_UNDEFINED),
      stop(NULL),
      unit(FU_CHARACTER),
      maxwait(NULL),
      timeunits(F_UNDEFINED),
      at(NULL)
	{
        ;
	}
	virtual ~MCRead();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCSeek : public MCStatement
{
	Preposition_type mode;
	MCExpression *where;
	MCExpression *fname;
public:
	MCSeek()
	{
		fname = NULL;
		where = NULL;
	}
	virtual ~MCSeek();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

class MCWrite : public MCStatement
{
	MCExpression *source;
	Open_argument arg;
	MCExpression *fname;
	File_unit unit;
	MCExpression *at;
public:
	MCWrite()
	{
		source = NULL;
		fname = NULL;
		unit = FU_CHARACTER;
		at = NULL;
	}
	virtual ~MCWrite();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &);
};

// math comands in cmdsm.cc

class MCAdd : public MCStatement
{
	MCExpression *source;
	MCChunk *dest;
	MCVarref *destvar;
public:
	MCAdd()
	{
		source = NULL;
		dest = NULL;
		destvar = NULL;
	}
	virtual ~MCAdd();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCDivide : public MCStatement
{
	MCExpression *source;
	MCChunk *dest;
	MCVarref *destvar;
public:
	MCDivide()
	{
		source = NULL;
		dest = NULL;
		destvar = NULL;
	}
	virtual ~MCDivide();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCMultiply : public MCStatement
{
	MCExpression *source;
	MCChunk *dest;
	MCVarref *destvar;
public:
	MCMultiply()
	{
		source = NULL;
		dest = NULL;
		destvar = NULL;
	}
	virtual ~MCMultiply();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCSubtract : public MCStatement
{
	MCExpression *source;
	MCChunk *dest;
	MCVarref *destvar;
public:
	MCSubtract()
	{
		source = NULL;
		dest = NULL;
		destvar = NULL;
	}
	virtual ~MCSubtract();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCArrayOp : public MCStatement
{
	MCVarref *destvar;
	MCExpression *element;
	MCExpression *key;
protected:
	enum
	{
		TYPE_USER,
		TYPE_ROW,
		TYPE_COLUMN,
		TYPE_LINE,
		TYPE_ITEM,
		TYPE_WORD,
		TYPE_TOKEN,
		TYPE_CHARACTER,
		TYPE_MASK = 0xf,
		TYPE_IS_COMBINE = 1U << 31
	};

	enum
	{
		FORM_NONE,
		FORM_SET
	};

	bool is_combine : 1;
	unsigned mode : 4;
	unsigned form : 4;
public:
	MCArrayOp()
	{
		destvar = NULL;
		element = key = NULL;
		mode = TYPE_USER;
		form = FORM_NONE;
	}
	virtual ~MCArrayOp();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCCombine : public MCArrayOp
{
public:
	MCCombine()
	{
		is_combine = true;
	}
};

class MCSplit : public MCArrayOp
{
public:
	MCSplit()
	{
		is_combine = false;
	}
};

class MCSetOp : public MCStatement
{
public:
    enum Op
    {
        kOpNone,
        kOpUnion,
        kOpUnionRecursively,
        kOpIntersect,
        kOpIntersectRecursively,
        kOpDifference,
        kOpSymmetricDifference
    };
    
private:
	MCAutoPointer<MCVarref> destvar;
    MCAutoPointer<MCExpression> destexpr;
	MCAutoPointer<MCExpression> source;
    Op op = kOpNone;
    bool is_into = false;
    
public:
    MCSetOp(Op p_op)
        : op(p_op)
    {
    }
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};


// MCStack manipulation comands in cmdss.cc

class MCCompact : public MCStatement
{
	MCChunk *target;
public:
	MCCompact()
	{
		target = NULL;
	}
	virtual ~MCCompact();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCGo : public MCStatement
{
	MCCRef *background;
	MCCRef *stack;
	MCCRef *card;
	MCExpression *window;
	Window_mode mode;
	Boolean marked;
	MCInterfaceExecGoVisibility visibility_type;
	Boolean thisstack;
	
	MCChunk *widget;
	Chunk_term direction;
public:
    MCGo() :
        background(nil),
        stack(nil),
        card(nil),
		window(nil),
		mode(WM_LAST),
        marked(False),
        visibility_type(kMCInterfaceExecGoVisibilityImplicit),
        thisstack(False),
		widget(nil),
        direction(CT_BACKWARD)
    {
        ;
    };
	virtual ~MCGo();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
    MCStack *findstack(MCExecContext &ctxt, MCStringRef p_value, Chunk_term etype, MCCard *&cptr);
};

class MCHide : public MCStatement
{
	Show_object which;
	MCChunk *object;
	Boolean card;
	MCVisualEffect *effect;
public:
	MCHide()
	{
		which = SO_OBJECT;
		object = NULL;
		effect = NULL;
	}
	virtual ~MCHide();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCLock : public MCStatement
{
	Lock_constants which;
	// MW-2011-09-24: [[ Effects ]] Holds the rect expr for 'lock screen for visual effect in <rect>'.
	MCExpression *rect;
public:
	MCLock(void)
	{
		which = LC_UNDEFINED;
		rect = nil;
	}
	virtual ~MCLock(void);
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCPop : public MCStatement
{
	Preposition_type prep;
	MCChunk *dest;
public:
	MCPop()
	{
		dest = NULL;
	}
	virtual ~MCPop();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCPush : public MCStatement
{
	MCChunk *card;
	Boolean recent;
public:
	MCPush()
	{
		card = NULL;
		recent = False;
	}
	virtual ~MCPush();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCSave : public MCStatement
{
	MCChunk *target;
	MCExpression *filename;
	MCExpression *format;
	bool newest_format;
public:
	MCSave() : target(NULL), filename(NULL), format(NULL), newest_format(false) {}
	virtual ~MCSave();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCShow : public MCStatement
{
	Show_object which;
	Boolean card;
	MCChunk *ton;
	MCExpression *location;
	MCVisualEffect *effect;
public:
	MCShow()
	{
		which = SO_OBJECT;
		ton = NULL;
		location = NULL;
		effect = NULL;
	}
	virtual ~MCShow();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCSubwindow : public MCStatement
{
	MCChunk *target;
	Boolean thisstack;
	MCExpression *parent;
	MCExpression *at;
	MCExpression *aligned;

	MCExpression *widget;
	MCExpression *properties;
protected:
	Window_mode mode;
public:
	MCSubwindow()
	{
		target = NULL;
		at = NULL;
		parent = NULL;
		thisstack = False;
		aligned = NULL;
		
		widget = nil;
		properties = nil;
	}
	virtual ~MCSubwindow();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

class MCTopLevel : public MCSubwindow
{
public:
	MCTopLevel()
	{
		mode = WM_TOP_LEVEL;
	}
};

class MCModal : public MCSubwindow
{
public:
	MCModal()
	{
		mode = WM_MODAL;
	}
};

class MCModeless : public MCSubwindow
{
public:
	MCModeless()
	{
		mode = WM_MODELESS;
	}
};

class MCOption : public MCSubwindow
{
public:
	MCOption()
	{
		mode = WM_OPTION;
	}
};

class MCPalette : public MCSubwindow
{
public:
	MCPalette()
	{
		mode = WM_PALETTE;
	}
};

class MCPopup : public MCSubwindow
{
public:
	MCPopup()
	{
		mode = WM_POPUP;
	}
};

class MCPulldown : public MCSubwindow
{
public:
	MCPulldown()
	{
		mode = WM_PULLDOWN;
	}
};

class MCSheet : public MCSubwindow
{
public:
	MCSheet()
	{
		mode = WM_SHEET;
	}
};


class MCDrawer : public MCSubwindow
{
public:
	MCDrawer()
	{
		mode = WM_DRAWER;
	}
};

class MCUnlock : public MCStatement
{
	Lock_constants which;
	MCVisualEffect *effect;
public:
	MCUnlock()
	{
		effect = NULL;
	}
	virtual ~MCUnlock();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
};

// MCPrinting cmdsp.cc

class MCPrint : public MCStatement
{
	Print_mode mode;
	MCChunk *target;
	MCExpression *from;
	MCExpression *to;
	MCExpression *rect;
	MCExpression *initial_state;
	bool bookmark_closed;
public:
	MCPrint();
	virtual ~MCPrint();
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt);
private:
    bool evaluate_src_rect(MCExecContext& ctxt, MCPoint& r_from, MCPoint& r_to);
};

class MCInclude: public MCStatement
{
public:
	MCInclude(bool p_is_require)
	{
		is_require = p_is_require;
		filename = NULL;
	}
	
	virtual ~MCInclude(void);
	virtual Parse_stat parse(MCScriptPoint& sp);
	virtual void exec_ctxt(MCExecContext& ctxt);

private:
	bool is_require;
	MCExpression *filename;
};

class MCEcho: public MCStatement
{
public:
	MCEcho(void)
	{
		data = nil;
	}
	
	virtual ~MCEcho(void);
	virtual Parse_stat parse(MCScriptPoint& sp);
	virtual void exec_ctxt(MCExecContext& ctxt);

private:
	MCStringRef data;
};

// Feature:
//   resolve-image
//
// Contributor:
//   Monte Goulding (2013-04-17)
//
// Syntax:
//   resolve image [id] <id or name> relative to <object reference>
//
// Action:
//   This command resolves a short id or name of an image as would be used for
//   an icon and sets it to the long ID of the image according to the documented
//   rules for resolving icons.
//
//   it is set empty if the command fails to resolve the image which means it's
//   not on any stack in memory.

class MCResolveImage : public MCStatement
{
public:
    MCResolveImage(void)
    {
        m_relative_object = nil;
        m_id_or_name  = nil;
    }
	
    virtual ~MCResolveImage(void);
    
    virtual Parse_stat parse(MCScriptPoint &p_sp);
    
    virtual void exec_ctxt(MCExecContext &ctxt);
    
private:
    MCChunk *m_relative_object;
    MCExpression *m_id_or_name;
    bool m_is_id : 1;
};

// MM-2014-02-12: [[ SecureSocket ]] secure socket <socket> [with|without verification]
//  New secure socket command, used to ensure all future communications over the given socket are encrypted.
class MCSecure : public MCStatement
{
public:
	MCSecure(void)
	{
		m_sock_name = NULL;
		m_verify_host_name = NULL;
		secureverify = True;
	}
	
	virtual ~MCSecure();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext&);
	
private:
	MCExpression *m_sock_name;
	bool secureverify : 1;
	
	// MM-2014-06-13: [[ Bug 12567 ]] Added new host name variant for use with verification.
	MCExpression *m_verify_host_name;
};

#endif
