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

//#include "execpt.h"
#include "exec.h"
#include "param.h"
#include "field.h"
#include "card.h"
#include "stack.h"
#include "image.h"
#include "handler.h"
#include "license.h"
#include "util.h"
#include "mcerror.h"
#include "osspec.h"
#include "globals.h"
#include "securemode.h"
#include "mode.h"

#include "scriptpt.h"
#include "chunk.h"

#include "dispatch.h"

#include "graphics_util.h"

#include "external.h"

////////////////////////////////////////////////////////////////////////////////

extern MCExecContext *MCECptr;

////////////////////////////////////////////////////////////////////////////////

#define XCOMMAND "C"
#define XFUNCTION "F"
#define XNONE ""
#define xresSucc 0
#define xresFail 1
#define xresNotImp 2
#define xresAbort 3

// IM-2014-03-06: [[ revBrowserCEF ]] Add revision number to v0 external interface
// SN-2014-07-08: [[ UnicodeExternalsV0 ]] Bump revision number after unicode update
#define EXTERNAL_INTERFACE_VERSION 2

typedef struct _Xternal
{
	char *name;
	char *type;
	Atom atom;
	void (*call)(char *args[], int nargs, char **retval, Bool *pass, Bool *err);
	void (*abort)();
} Xternal;

struct MCstring
{
	const char *sptr;
    int length;
};

struct MCarray
{
	unsigned int nelements;
	MCstring *strings;
	char **keys;
};

////////////////////////////////////////////////////////////////////////////////
// New struct allowing to allocate memory when it's needed, and which ensures
// the memory is then free'd properly.

struct MCExternalAllocPool
{
    unsigned int size;
    char **strings;

    MCExternalAllocPool()
    {
        size = 0;
        strings = nil;
    }
};

void MCExternalDeallocatePool(MCExternalAllocPool *p_pool)
{
    for (unsigned int i = 0; i < p_pool -> size; ++i)
        MCMemoryDeleteArray(p_pool -> strings[i]);

    MCMemoryDeleteArray(p_pool -> strings);

    delete p_pool;
    p_pool = nil;
}

bool MCExternalAddAllocatedString(MCExternalAllocPool *p_pool, char* p_string)
{
    if (!MCMemoryResizeArray(p_pool -> size + 1, p_pool -> strings, p_pool -> size))
        return false;

    p_pool -> strings[p_pool -> size - 1] = p_string;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

typedef char *(*XCB)(const char *arg1, const char *arg2, const char *arg3, int *retval);
typedef int (*SECURITYHANDLER)(const char *);
typedef void (*DELETER)(void *data);
typedef void (*GETXTABLE)(XCB *, DELETER, const char **, Xternal **, DELETER *);
typedef void (*CONFIGURESECURITY)(SECURITYHANDLER *handlers);
typedef void (*SHUTDOWNXTABLE)(void);

// IM-2014-03-06: [[ revBrowserCEF ]] define setVersion function signature
typedef void (*SETEXTERNALINTERFACEVERSION)(unsigned int version);

extern XCB MCcbs[];
extern SECURITYHANDLER MCsecuritycbs[];

static MCExternalAllocPool* MCexternalallocpool = nil;

////////////////////////////////////////////////////////////////////////////////

class MCExternalV0: public MCExternal
{
public:
	MCExternalV0(void);
	virtual ~MCExternalV0(void);

	virtual const char *GetName(void) const;
	virtual Handler_type GetHandlerType(uint32_t index) const;
	virtual bool ListHandlers(MCExternalListHandlersCallback callback, void *state);
	virtual Exec_stat Handle(MCObject *p_context, Handler_type p_type, uint32_t p_index, MCParameter *p_parameters);

private:
	virtual bool Prepare(void);
	virtual bool Initialize(void);
	virtual void Finalize(void);

	const char *m_name;
	_Xternal *m_table;
	void (*m_free)(void *);
    void (*m_shutdown)(void);
};


////////////////////////////////////////////////////////////////////////////////

MCExternalV0::MCExternalV0(void)
{
	m_table = nil;
	m_free = nil;
	m_shutdown = nil;
}

MCExternalV0::~MCExternalV0(void)
{
}

static void deleter(void *d)
{
	free(d);
}

bool MCExternalV0::Prepare(void)
{
	// IM-2014-03-06: [[ revBrowserCEF ]] call the setExternalInterfaceVersion() function if present
	SETEXTERNALINTERFACEVERSION t_set_version;
	t_set_version = (SETEXTERNALINTERFACEVERSION)MCS_resolvemodulesymbol(m_module, MCSTR("setExternalInterfaceVersion"));
	if (t_set_version != nil)
		t_set_version(EXTERNAL_INTERFACE_VERSION);
	
	// Get the info from the main external entry point (we know this symbol exists
	// as it is used to determine if its a V0 external!).

	GETXTABLE t_getter;
	t_getter = (GETXTABLE)MCS_resolvemodulesymbol(m_module, MCSTR("getXtable"));
    t_getter(MCcbs, deleter, &m_name, &m_table, &m_free);
	
	CONFIGURESECURITY t_conf_security;
	t_conf_security = (CONFIGURESECURITY)MCS_resolvemodulesymbol(m_module, MCSTR("configureSecurity"));
	if (t_conf_security != nil)
		t_conf_security(MCsecuritycbs);

	SHUTDOWNXTABLE t_shutdown;
	t_shutdown = (SHUTDOWNXTABLE)MCS_resolvemodulesymbol(m_module, MCSTR("shutdownXtable"));
	if (t_shutdown != nil)
		m_shutdown = t_shutdown;

	return true;
}

bool MCExternalV0::Initialize(void)
{
	return true;
}

void MCExternalV0::Finalize(void)
{
	if (m_shutdown != nil)
		m_shutdown();
}

const char *MCExternalV0::GetName(void) const
{
	return m_name;
}

Handler_type MCExternalV0::GetHandlerType(uint32_t p_index) const
	{
	if (m_table[p_index] . type[0] == XCOMMAND[0])
		return HT_MESSAGE;
	return HT_FUNCTION;
}

bool MCExternalV0::ListHandlers(MCExternalListHandlersCallback p_callback, void *p_state)
{
	for(uint32_t i = 0; m_table[i] . name[0] != '\0'; i++)
    {
        // MW-2014-08-05: [[ OldExternalsV2 ]] If there is a UTF8 variant of the handler with
        //   the same name, then we don't include the non-UTF8 variant.
        bool t_has_utf8_variant;
        t_has_utf8_variant = false;
        
        if (isupper(m_table[i] . type[0]))
            for(uint32_t j = 0; m_table[j] . name[0] != '\0'; j++)
            {
                if (i == j)
                    continue;
                
                if (islower(m_table[j] . type[0]) &&
                    strcmp(m_table[j] . name, m_table[i] . name) == 0)
                {
                    t_has_utf8_variant = true;
                    break;
                }
            }
    
        if (t_has_utf8_variant)
            continue;
        
		if (!p_callback(p_state, toupper(m_table[i] . type[0]) == XCOMMAND[0] ? HT_MESSAGE : HT_FUNCTION, m_table[i] . name, i))
			return false;
    }
    
	return true;
}

Exec_stat MCExternalV0::Handle(MCObject *p_context, Handler_type p_type, uint32_t p_index, MCParameter *p_parameters)
{
	char t_type;
	if (p_type == HT_FUNCTION)
		t_type = XFUNCTION[0];
	else
		t_type = XCOMMAND[0];
    
    // MW-2014-08-05: [[ OldExternalsV2 ]] Compare the upper of the handler tags.
	_Xternal *t_handler;
	t_handler = &m_table[p_index];
	if (toupper(t_handler -> type[0]) != t_type)
		return ES_NOT_HANDLED;
    
    // If we want UTF8, then the type is lowercase.
    bool t_wants_utf8;
    t_wants_utf8 = islower(t_handler -> type[0]);
    
    char *retval;
    Bool Xpass, Xerr;
    int nargs = 0;
    char **args = NULL;
    MCExecContext ctxt(p_context, nil, nil);
    
    while (p_parameters != NULL)
    {
        // MW-2013-06-20: [[ Bug 10961 ]] Make sure we evaluate the parameter as an
        //   argument. This takes the value from the variable (by-ref), or built-in
        //   value (by-value).
        MCAutoValueRef t_value;
        MCAutoStringRef t_string;
        
        if (!p_parameters->eval_argument(ctxt, &t_value))
            return ES_ERROR;
        MCU_realloc((char **)&args, nargs, nargs + 1, sizeof(char *));
        
        if (!ctxt . ConvertToString(*t_value, &t_string))
            return ES_ERROR;
        
        // If we want UTF8 use a different conversion method.
        if (t_wants_utf8)
            MCStringConvertToUTF8String(*t_string, args[nargs++]);
        else
            MCStringConvertToCString(*t_string, args[nargs++]);
        p_parameters = p_parameters -> getnext();
    }
    
    // Handling of memory allocation for C-strings
    MCExternalAllocPool *t_old_pool = MCexternalallocpool;
    MCexternalallocpool = new MCExternalAllocPool;
    
    (t_handler -> call)(args, nargs, &retval, &Xpass, &Xerr);
    
    MCExternalDeallocatePool(MCexternalallocpool);
    MCexternalallocpool = t_old_pool;
    
    // MW-2011-03-02: [[ Bug ]] Memory leak as we aren't freeing any error string that
    //   is returned.
    if (Xerr)
    {
        MCeerror -> add(EE_EXTERNAL_EXCEPTION, 0, 0, retval == NULL ? "" : retval);
        m_free(retval);
    }
    else if (retval == NULL)
        ctxt . SetTheResultToEmpty();
    else
    {
        // If we want UTF8 use a different conversion method.
        if (t_wants_utf8)
        {
            MCAutoStringRef t_res_string;
            MCStringCreateWithBytes((byte_t *)retval, strlen(retval), kMCStringEncodingUTF8, false, &t_res_string);
            ctxt . SetTheResultToValue(*t_res_string);
        }
        else
            ctxt . SetTheResultToCString(retval);
        m_free(retval);
    }
    
    if (args != NULL)
    {
        while (nargs--)
            delete args[nargs];
        
        delete args;
    }
    
    if (Xerr)
        return ES_ERROR;
	else if (Xpass)
        return ES_PASS;
    
    return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

MCExternal *MCExternalCreateV0(void)
{
	return new MCExternalV0;
}

////////////////////////////////////////////////////////////////////////////////

static int trans_stat(Exec_stat stat)
{
	switch(stat)
	{
	case ES_NORMAL:
		return xresSucc;
	case ES_ERROR:
		return xresFail;
	default:
		break;
	}
	return xresNotImp;
}

static Exec_stat getvarptr(MCExecContext& ctxt, const MCString &vname,MCVariable **tvar)
{
	MCAutoNameRef t_name;
	/* UNCHECKED */ t_name . CreateWithOldString(vname);

	MCVarref *newvar;
    
    if (MCECptr -> FindVar(t_name, &newvar) != PS_NORMAL)
		return ES_ERROR;
	
	if ((*tvar = newvar->evalvar(ctxt)) == NULL)
	{
		delete newvar;
		return ES_ERROR;
	}
	delete newvar;

	return ES_NORMAL;
}

static Exec_stat getvarptr_utf8(MCExecContext& ctxt, const MCString &vname, MCVariable **tvar)
{
	MCNewAutoNameRef t_name;
    MCAutoStringRef t_arg1_string;
    
	if (!MCStringCreateWithBytes((byte_t*)vname.getstring(), vname.getlength(), kMCStringEncodingUTF8, false, &t_arg1_string)
            || !MCNameCreate(*t_arg1_string, &t_name))
        return ES_ERROR;
    
	MCVarref *newvar;
    
    if (MCECptr -> FindVar(*t_name, &newvar) != PS_NORMAL)
		return ES_ERROR;
	
	if ((*tvar = newvar->evalvar(ctxt)) == NULL)
	{
		delete newvar;
		return ES_ERROR;
	}
	delete newvar;
    
	return ES_NORMAL;
}

static MCControl *getobj(Chunk_term otype, Chunk_term etype,
                         const char *str, const char *group)
{
	Chunk_term ctype = CT_UNDEFINED;
	if (strequal(group, MCtruestring))
		ctype = CT_CARD;
	else
		if (strequal(group, MCfalsestring))
			ctype = CT_BACKGROUND;
	MCStack *s = MCdefaultstackptr;
	MCAutoStringRef t_string;
	/* UNCHECKED */ MCStringCreateWithCString(str, &t_string);
	return s->getcurcard()->getchild(etype, *t_string, otype, ctype);
}

// SN-2014-07-08: [[ UnicodeExternalsV0 ]] Added functions taking UTF8 strings as parameters rather than C-strings
static MCControl *getobj_utf8(Chunk_term otype, Chunk_term etype,
                              const char *str, const char *group)
{
	Chunk_term ctype = CT_UNDEFINED;
	if (strequal(group, MCtruestring))
		ctype = CT_CARD;
	else
		if (strequal(group, MCfalsestring))
			ctype = CT_BACKGROUND;
	MCStack *s = MCdefaultstackptr;
	MCAutoStringRef t_string;
	/* UNCHECKED */ MCStringCreateWithBytes((byte_t*)str, strlen(str), kMCStringEncodingUTF8, false, &t_string);
	return s->getcurcard()->getchild(etype, *t_string, otype, ctype);
}

static char *getfield(MCField *fptr, int *retval)
{
	if (fptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = xresSucc;
	// MW-2012-02-21: [[ FieldExport ]] Use the new text export method.
	MCAutoStringRef t_string;;
	fptr->exportastext(0, 0, INT32_MAX, &t_string);
    char *t_result;
    /* UNCHECKED */ MCStringConvertToCString(*t_string, t_result);
	return t_result;
}

// SN-2014-07-08: [[ UnicodeExternalsV0 ]] Added functions returning UTF8 strings rather than C-strings
static char *getfield_utf8(MCField *fptr, int *retval)
{
	if (fptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = xresSucc;
	// MW-2012-02-21: [[ FieldExport ]] Use the new text export method.
	MCAutoStringRef t_string;;
	fptr->exportastext(0, 0, INT32_MAX, &t_string);
    char *t_result;
    /* UNCHECKED */ MCStringConvertToUTF8String(*t_string, t_result);
	return t_result;
}

static char *set_idle_func(const char *arg1, const char *arg2,
                           const char *arg3, int *retval)
{
	MCdefaultstackptr->setidlefunc((void (*)())arg1);
	return NULL;
}

static char *set_idle_rate(const char *arg1, const char *arg2,
                           const char *arg3, int *retval)
{
	MCidleRate = (int)(intptr_t)arg1;
	return NULL;
}

static char *card_message(const char *arg1, const char *arg2,
                          const char *arg3, int *retval)
{
	MCStack *s = MCdefaultstackptr;
	Boolean oldcheck = MCcheckstack;
	MCcheckstack = False;
    MCAutoStringRef t_arg1;
    /* UNCHECKED */ MCStringCreateWithCString(arg1, &t_arg1);
	*retval = trans_stat(s->getcurcard()->domess(*t_arg1));
	MCcheckstack = oldcheck;
	return NULL;
}

static char *mc_message(const char *arg1, const char *arg2,
                        const char *arg3, int *retval)
{
	return card_message(arg1, arg2, arg3, retval);
}

static char *eval_expr(const char *arg1, const char *arg2,
                       const char *arg3, int *retval)
{
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	
	MCAutoStringRef t_string;
	MCAutoValueRef t_result;
	/* UNCHECKED */ MCStringCreateWithCString(arg1, &t_string);
	MCECptr->GetHandler()->eval(*MCECptr, *t_string, &t_result);
	
	if (MCECptr->HasError())
	{
		*retval = xresFail;
		return NULL;
	}
	
	MCAutoStringRef t_return;
	/* UNCHECKED */ MCECptr->ConvertToString(*t_result, &t_return);
	*retval = xresSucc;
	return MCStringGetOldString(*t_return).clone();
}

static char *get_global(const char *arg1, const char *arg2,
                        const char *arg3, int *retval)
{
	MCVariable *tmp;
	tmp = MCVariable::lookupglobal_cstring(arg1);
	if (tmp != nil)
	{
        *retval = xresSucc;
        MCExecContext ctxt(nil, nil, nil);
        MCAutoValueRef t_value;
		tmp->eval(ctxt, &t_value);
        MCAutoStringRef t_string;
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
        char *t_result;
        /* UNCHECKED */ MCStringConvertToCString(*t_string, t_result);
		return t_result;
	}
	*retval = xresFail;
	return NULL;
}

static char *set_global(const char *arg1, const char *arg2,
                        const char *arg3, int *retval)
{
	MCNewAutoNameRef t_arg1;
	/* UNCHECKED */ MCNameCreateWithCString(arg1, &t_arg1);
	MCVariable *tmp;
	if (!MCVariable::ensureglobal(*t_arg1, tmp))
	{
		*retval = xresFail;
		return NULL;
    }

    MCExecContext ctxt(nil, nil, nil);
	*retval = xresSucc;
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithCString(arg2, &t_string);
	tmp->set(ctxt, *t_string);
	return NULL;
}

static char *get_field_by_name(const char *arg1, const char *arg2,
                               const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj(CT_FIELD, CT_EXPRESSION, arg1, arg2);
	return getfield(fptr, retval);
}

static char *get_field_by_num(const char *arg1, const char *arg2,
                              const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj(CT_FIELD, CT_EXPRESSION, arg1, arg2);
	return getfield(fptr, retval);
}

static char *get_field_by_id(const char *arg1, const char *arg2,
                             const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj(CT_FIELD, CT_ID, arg1, arg2);
	return getfield(fptr, retval);
}

static char *set_field_by_name(const char *arg1, const char *arg2,
                               const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj(CT_FIELD, CT_EXPRESSION, arg1, arg2);
	if (fptr == NULL)
		*retval = xresFail;
	else
	{
        MCAutoStringRef t_string;
        MCStringCreateWithBytes((byte_t*)arg3, strlen(arg3), kMCStringEncodingUTF8, false, &t_string);
        fptr->settext(fptr->getcard()->getid(), *t_string, False);
		*retval = xresSucc;
	}
	return NULL;
}

static char *set_field_by_num(const char *arg1, const char *arg2,
                              const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj(CT_FIELD, CT_EXPRESSION, arg1, arg2);
	if (fptr == NULL)
		*retval = xresFail;
	else
	{
        MCAutoStringRef t_string;
        MCStringCreateWithBytes((byte_t*)arg3, strlen(arg3), kMCStringEncodingUTF8, false, &t_string);
        fptr->settext(fptr->getcard()->getid(), *t_string, False);
		*retval = xresSucc;
	}
	return NULL;
}

static char *set_field_by_id(const char *arg1, const char *arg2,
                             const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj(CT_FIELD, CT_ID, arg1, arg2);
	if (fptr == NULL)
		*retval = xresFail;
	else
	{
        MCAutoStringRef t_string;
        MCStringCreateWithBytes((byte_t*)arg3, strlen(arg3), kMCStringEncodingUTF8, false, &t_string);
        fptr->settext(fptr->getcard()->getid(), *t_string, False);
		*retval = xresSucc;
	}
	return NULL;
}

static char *show_image_by_name(const char *arg1, const char *arg2,
                                const char *arg3, int *retval)
{
	MCImage *iptr = (MCImage *)getobj(CT_IMAGE, CT_EXPRESSION, arg1, arg2);
	if (iptr == NULL)
		*retval = xresFail;
	else
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		iptr->layer_redrawall();
		*retval = xresSucc;
	}
	return NULL;
}

static char *show_image_by_num(const char *arg1, const char *arg2,
                               const char *arg3, int *retval)
{
	MCImage *iptr = (MCImage *)getobj(CT_IMAGE, CT_EXPRESSION, arg1, arg2);
	if (iptr == NULL)
		*retval = xresFail;
	else
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		iptr->layer_redrawall();
		*retval = xresSucc;
	}
	return NULL;
}

static char *show_image_by_id(const char *arg1, const char *arg2,
                              const char *arg3, int *retval)
{
	MCImage *iptr;
	iptr = NULL;
	if (isdigit(arg1[0]))
		iptr = (MCImage *)getobj(CT_IMAGE, CT_ID, arg1, arg2);
	else
	{
        MCAutoStringRef arg1_str;
        /* UNCHECKED */ MCStringCreateWithCString(arg1, &arg1_str);
		MCScriptPoint sp(*arg1_str);
		MCChunk *t_chunk;
		t_chunk = new MCChunk(False);
		
		Symbol_type t_next_type;
		MCerrorlock++;
		if (t_chunk -> parse(sp, False) == PS_NORMAL && sp.next(t_next_type) == PS_EOF)
        {
            MCExecContext ctxt(nil, nil, nil);
			MCObject *t_object;
			uint32_t t_part_id;
			if (t_chunk -> getobj(ctxt, t_object, t_part_id, False)  &&
				t_object -> gettype() == CT_IMAGE)
				iptr = static_cast<MCImage *>(t_object);
		}
		MCerrorlock--;
	}	
	
	if (iptr == NULL)
		*retval = xresFail;
	else
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		iptr->layer_redrawall();
		*retval = xresSucc;
	}
	return NULL;
}

static char *get_variable(const char *arg1, const char *arg2,
                          const char *arg3, int *retval)
{
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr(*MCECptr, arg1, &var));
	if (var == NULL)
		return NULL;
    MCAutoValueRef t_value;
	var -> eval(*MCECptr, &t_value);
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCECptr -> ConvertToString(*t_value, &t_string);
    char *t_result;
    /* UNCHECKED */ MCStringConvertToCString(*t_string, t_result);
	return t_result;
}

static char *set_variable(const char *arg1, const char *arg2,
                          const char *arg3, int *retval)
{
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr(*MCECptr, arg1,&var));
	if (var == NULL)
		return NULL;
	//MCECptr->GetEP().setsvalue(arg2);
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithCString(arg2, &t_string);
	var->set(*MCECptr, *t_string);
	return NULL;
}

static char *get_variable_ex(const char *arg1, const char *arg2,
                             const char *arg3, int *retval)
{
	MCString *value = (MCString *)arg3;
	Boolean array = False;
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr(*MCECptr, arg1, &var));
	if (var == NULL)
		return NULL;
    
    MCAutoValueRef t_value;

	if (arg2 != NULL && strlen(arg2) != 0)
	{
		MCNameRef t_key;
		/* UNCHECKED */ MCNameCreateWithCString(arg2, t_key);
		var -> eval(*MCECptr, &t_key, 1, &t_value);
		MCValueRelease(t_key);
	}
	else
		var -> eval(*MCECptr, &t_value);
    
    
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCECptr -> ConvertToString(*t_value, &t_string);
    char *t_result;
    /* UNCHECKED */ MCStringConvertToCString(*t_string, t_result);
    
    // SN-2014-04-07 [[ Bug 12118 ]] revExecuteSQL writes incomplete data into SQLite BLOB columns
    // arg3 is not a char* but rather a MCString; whence setting the length should not be forgotten,
    // in case '\0' are present in the value fetched.
	value -> set(t_result, MCStringGetLength(*t_string));
	return NULL;
}

static char *set_variable_ex(const char *arg1, const char *arg2,
                             const char *arg3, int *retval)
{
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr(*MCECptr, arg1,&var));
	if (var == NULL)
		return NULL;
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithOldString(*(MCString*)arg3, &t_string);
	if (arg2 != NULL && strlen(arg2) > 0)
	{
		MCNameRef t_key;
		/* UNCHECKED */ MCNameCreateWithCString(arg2, t_key);
		var->set(*MCECptr, *t_string, &t_key, 1);
		MCValueRelease(t_key);
	}
	else
		var->set(*MCECptr, *t_string);

	return NULL;
}

struct get_array_element_t
{
	uindex_t index;
	uindex_t limit;
	char **keys;
    MCstring *strings;
    bool is_text;
};

static bool get_array_element(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	get_array_element_t *ctxt;
	ctxt = (get_array_element_t *)p_context;

	char* t_key;
	MCStringConvertToCString(MCNameGetString(p_key), t_key);

	ctxt -> keys[ctxt -> index] = t_key;
	MCExternalAddAllocatedString(MCexternalallocpool, t_key);

	if (ctxt -> strings != nil)
	{
        // The value needs to be converted as a C-string
        uindex_t t_length;
        char_t *t_chars;
        MCAutoStringRef t_string;

        if (!MCECptr -> ConvertToString(p_value, &t_string))
            t_string = kMCEmptyString;

        /* UNCHECKED */ MCStringConvertToNative(*t_string, t_chars, t_length);

        ctxt -> strings[ctxt -> index] . length = (int)t_length;
        ctxt -> strings[ctxt -> index] . sptr = (const char*)t_chars;

        MCExternalAddAllocatedString(MCexternalallocpool, (char*)t_chars);
	}
	ctxt -> index++;

	if (ctxt -> index < ctxt -> limit)
		return true;

	return false;
}

static char *get_array(const char *arg1, const char *arg2,
                       const char *arg3, int *retval)
{
	MCarray *value = (MCarray *)arg3;
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr(*MCECptr, arg1,&var));
	if (var == NULL)
		return NULL;

	if (!var -> isarray())
		return NULL;

    MCArrayRef t_array;
    t_array = (MCArrayRef)var -> getvalueref();
	if (t_array == nil)
		return NULL;

	if (value->nelements == 0)
	{
		value->nelements = MCArrayGetCount(t_array);
		return NULL;
	}

	value->nelements = MCU_min(value->nelements, MCArrayGetCount(t_array));

	get_array_element_t t_ctxt;
	t_ctxt . index = 0;
	t_ctxt . limit = value -> nelements;
	t_ctxt . keys = value -> keys;
    t_ctxt . strings = value -> strings;
	MCArrayApply(t_array, get_array_element, &t_ctxt);

	return NULL;

#if 0
	var -> getvalue() . getkeys(value->keys, value->nelements);
	if (value->strings != NULL)
	{

		MCVariableValue& t_var_value = var -> getvalue();

		for (unsigned int i = 0; i < value->nelements; i++)
		{
			MCString t_string;
			MCstring *t_value_ptr;
			t_value_ptr = &value -> strings[i];
			
			MCVariableValue *t_entry;
			t_var_value . lookup_element(*MCEPptr, value -> keys[i], t_entry);
			if (t_entry -> is_number())
			{
				t_entry -> ensure_string(*MCEPptr);
				t_string = t_entry -> get_string();
			}
			else if (t_entry -> is_string())
			{
				t_string = t_entry -> get_string();
			}
			else
			{
				t_string = MCnullmcstring;
			}

			t_value_ptr -> length = t_string . getlength();
			t_value_ptr -> sptr = t_string . getstring();
		}
	}
	return NULL;
#endif
}

static char *set_array(const char *arg1, const char *arg2,
                       const char *arg3, int *retval)
{
	MCarray *value = (MCarray *)arg3;
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr(*MCECptr, arg1,&var));
	if (var == NULL)
		return NULL;
	var->remove(*MCECptr, nil, 0);//clear variable
	char tbuf[U4L];
	for (unsigned int i = 0; i <value->nelements; i++)
	{
		MCString *s = (MCString *)&value->strings[i];
        MCAutoStringRef t_string;
        /* UNCHECKED */ MCStringCreateWithOldString(*s, &t_string);
        MCNameRef t_key;
		if (value->keys == NULL ||  value->keys[i] == NULL)
		{
			sprintf(tbuf,"%d",i+1);
			/* UNCHECKED */ MCNameCreateWithCString(tbuf, t_key);
		}
		else
			/* UNCHECKED */ MCNameCreateWithCString(value -> keys[i], t_key);
		var->set(*MCECptr, *t_string, &t_key, 1);
	}
	return NULL;
}

// IM-2014-03-06: [[ revBrowserCEF ]] Extend the externals interface to allow externals to hook into the wait loop

// Add callback function (arg1) to the wait loop
static char *add_runloop_action(const char *arg1, const char *arg2,
								const char *arg3, int *retval)
{
	MCRunloopActionCallback t_callback;
	t_callback = (MCRunloopActionCallback)arg1;

	void *t_context;
	t_context = (void*)arg2;

	MCRunloopActionRef *r_action;
	r_action = (MCRunloopActionRef*)arg3;

	if (!MCscreen->AddRunloopAction(t_callback, t_context, *r_action))
	{
		*retval = xresFail;
		return NULL;
	}

	*retval = xresSucc;

	return NULL;
}

// Remove callback function from the wait loop
static char *remove_runloop_action(const char *arg1, const char *arg2,
								const char *arg3, int *retval)
{
	MCRunloopActionRef t_action;
	t_action = (MCRunloopActionRef)arg1;
	
	MCscreen->RemoveRunloopAction(t_action);

	*retval = xresSucc;
	return NULL;
}

// Run the waitloop
static char *runloop_wait(const char *arg1, const char *arg2,
						  const char *arg3, int *retval)
{
	MCscreen->wait(60.0, True, True);
	*retval = xresSucc;
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// SN-2014-07-08: [[ UnicodeExternalsV0 ]] Add functions allowing UTF8-encoded parameter

static char *card_message_utf8(const char *arg1, const char *arg2,
                          const char *arg3, int *retval)
{
	MCStack *s = MCdefaultstackptr;
	Boolean oldcheck = MCcheckstack;
	MCcheckstack = False;
    MCAutoStringRef t_arg1;
    /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg1, strlen(arg1), kMCStringEncodingUTF8, false, &t_arg1);
	*retval = trans_stat(s->getcurcard()->domess(*t_arg1));
	MCcheckstack = oldcheck;
	return NULL;
}

static char *eval_expr_utf8(const char *arg1, const char *arg2,
                       const char *arg3, int *retval)
{
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	
	MCAutoStringRef t_string;
	MCAutoValueRef t_result;
	/* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg1, strlen(arg1), kMCStringEncodingUTF8, false, &t_string);
	MCECptr->GetHandler()->eval(*MCECptr, *t_string, &t_result);
	
	if (MCECptr->HasError())
	{
		*retval = xresFail;
		return NULL;
	}
	
	MCAutoStringRef t_return;
    char *t_chars;
	/* UNCHECKED */ MCECptr->ConvertToString(*t_result, &t_return);
    /* UNCHECKED */ MCStringConvertToUTF8String(*t_return, t_chars);
	*retval = xresSucc;
	return t_chars;
}

static char *get_global_utf8(const char *arg1, const char *arg2,
                        const char *arg3, int *retval)
{
	MCVariable *tmp;
    MCAutoStringRef t_arg1_string;
    MCNewAutoNameRef t_arg1_name;
    /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg1, strlen(arg1), kMCStringEncodingUTF8, false, &t_arg1_string);
    /* UNCHECKED */ MCNameCreate(*t_arg1_string, &t_arg1_name);
    
	tmp = MCVariable::lookupglobal(*t_arg1_name);
	if (tmp != nil)
	{
        *retval = xresSucc;
        MCExecContext ctxt(nil, nil, nil);
        MCAutoValueRef t_value;
		tmp->eval(ctxt, &t_value);
        MCAutoStringRef t_string;
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
        char *t_result;
        /* UNCHECKED */ MCStringConvertToUTF8String(*t_string, t_result);
		return t_result;
	}
	*retval = xresFail;
	return NULL;
}

static char *set_global_utf8(const char *arg1, const char *arg2,
                        const char *arg3, int *retval)
{
	MCNewAutoNameRef t_arg1_name;
    MCAutoStringRef t_arg1_string;
    /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg1, strlen(arg1), kMCStringEncodingUTF8, false, &t_arg1_string);
    /* UNCHECKED */ MCNameCreate(*t_arg1_string, &t_arg1_name);
	MCVariable *tmp;
	if (!MCVariable::ensureglobal(*t_arg1_name, tmp))
	{
		*retval = xresFail;
		return NULL;
    }
    
    MCExecContext ctxt(nil, nil, nil);
	*retval = xresSucc;
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg2, strlen(arg2), kMCStringEncodingUTF8, false, &t_string);
	tmp->set(ctxt, *t_string);
	return NULL;
}

static char *get_field_by_name_utf8(const char *arg1, const char *arg2,
                               const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj_utf8(CT_FIELD, CT_EXPRESSION, arg1, arg2);
	return getfield_utf8(fptr, retval);
}

static char *get_field_by_num_utf8(const char *arg1, const char *arg2,
                              const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj_utf8(CT_FIELD, CT_EXPRESSION, arg1, arg2);
	return getfield_utf8(fptr, retval);
}

static char *get_field_by_id_utf8(const char *arg1, const char *arg2,
                             const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj_utf8(CT_FIELD, CT_ID, arg1, arg2);
	return getfield_utf8(fptr, retval);
}

static char *set_field_by_name_utf8(const char *arg1, const char *arg2,
                               const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj_utf8(CT_FIELD, CT_EXPRESSION, arg1, arg2);
	if (fptr == NULL)
		*retval = xresFail;
	else
	{
        MCAutoStringRef t_string;
        /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg3, strlen(arg3), kMCStringEncodingUTF8, false, &t_string);
        fptr->settext(fptr->getcard()->getid(), *t_string, False);
		*retval = xresSucc;
	}
	return NULL;
}

static char *set_field_by_num_utf8(const char *arg1, const char *arg2,
                              const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj_utf8(CT_FIELD, CT_EXPRESSION, arg1, arg2);
	if (fptr == NULL)
		*retval = xresFail;
	else
	{
        MCAutoStringRef t_string;
        /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg3, strlen(arg3), kMCStringEncodingUTF8, false, &t_string);
        fptr->settext(fptr->getcard()->getid(), *t_string, False);
		*retval = xresSucc;
	}
	return NULL;
}

static char *set_field_by_id_utf8(const char *arg1, const char *arg2,
                             const char *arg3, int *retval)
{
	MCField *fptr = (MCField *)getobj_utf8(CT_FIELD, CT_ID, arg1, arg2);
	if (fptr == NULL)
		*retval = xresFail;
	else
	{
        MCAutoStringRef t_string;
        /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg3, strlen(arg3), kMCStringEncodingUTF8, false, &t_string);
        fptr->settext(fptr->getcard()->getid(), *t_string, False);
		*retval = xresSucc;
	}
	return NULL;
}

static char *show_image_by_name_utf8(const char *arg1, const char *arg2,
                                const char *arg3, int *retval)
{
	MCImage *iptr = (MCImage *)getobj_utf8(CT_IMAGE, CT_EXPRESSION, arg1, arg2);
	if (iptr == NULL)
		*retval = xresFail;
	else
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		iptr->layer_redrawall();
		*retval = xresSucc;
	}
	return NULL;
}

static char *show_image_by_num_utf8(const char *arg1, const char *arg2,
                               const char *arg3, int *retval)
{
	MCImage *iptr = (MCImage *)getobj_utf8(CT_IMAGE, CT_EXPRESSION, arg1, arg2);
	if (iptr == NULL)
		*retval = xresFail;
	else
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		iptr->layer_redrawall();
		*retval = xresSucc;
	}
	return NULL;
}

static char *show_image_by_id_utf8(const char *arg1, const char *arg2,
                              const char *arg3, int *retval)
{
	MCImage *iptr;
	iptr = NULL;
	if (isdigit(arg1[0]))
		iptr = (MCImage *)getobj_utf8(CT_IMAGE, CT_ID, arg1, arg2);
	else
	{
        MCAutoStringRef arg1_str;
        /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg1, strlen(arg1), kMCStringEncodingUTF8, false, &arg1_str);
		MCScriptPoint sp(*arg1_str);
		MCChunk *t_chunk;
		t_chunk = new MCChunk(False);
		
		Symbol_type t_next_type;
		MCerrorlock++;
		if (t_chunk -> parse(sp, False) == PS_NORMAL && sp.next(t_next_type) == PS_EOF)
        {
            MCExecContext ctxt(nil, nil, nil);
			MCObject *t_object;
			uint32_t t_part_id;
			if (t_chunk -> getobj(ctxt, t_object, t_part_id, False)  &&
				t_object -> gettype() == CT_IMAGE)
				iptr = static_cast<MCImage *>(t_object);
		}
		MCerrorlock--;
	}
	
	if (iptr == NULL)
		*retval = xresFail;
	else
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		iptr->layer_redrawall();
		*retval = xresSucc;
	}
	return NULL;
}

static char *get_variable_utf8(const char *arg1, const char *arg2,
                          const char *arg3, int *retval)
{
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr_utf8(*MCECptr, arg1, &var));
	if (var == NULL)
		return NULL;
    MCAutoValueRef t_value;
	var -> eval(*MCECptr, &t_value);
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCECptr -> ConvertToString(*t_value, &t_string);
    char *t_result;
    /* UNCHECKED */ MCStringConvertToUTF8String(*t_string, t_result);
	return t_result;
}

static char *set_variable_utf8(const char *arg1, const char *arg2,
                          const char *arg3, int *retval)
{
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr_utf8(*MCECptr, arg1,&var));
	if (var == NULL)
		return NULL;
	//MCECptr->GetEP().setsvalue(arg2);
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg2, strlen(arg2), kMCStringEncodingUTF8, false, &t_string);
	var->set(*MCECptr, *t_string);
	return NULL;
}

//////////

static char *get_variable_ex_utf8(const char *arg1, const char *arg2,
                                  const char *arg3, int *retval, Bool p_is_text)
{
	MCString *value = (MCString *)arg3;
	Boolean array = False;
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return false;
	}
	*retval = trans_stat(getvarptr_utf8(*MCECptr, arg1, &var));
	if (var == NULL)
		return false;
    
    MCAutoValueRef t_value;
    
	if (arg2 != NULL && strlen(arg2) != 0)
	{
		MCNameRef t_key;
        MCAutoStringRef t_key_as_string;
        /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg2, strlen(arg2), kMCStringEncodingUTF8, false, &t_key_as_string);
		/* UNCHECKED */ MCNameCreate(*t_key_as_string, t_key);
		var -> eval(*MCECptr, &t_key, 1, &t_value);
		MCValueRelease(t_key);
	}
	else
		var -> eval(*MCECptr, &t_value);
    
    
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCECptr -> ConvertToString(*t_value, &t_string);
    
    char *t_result;
    uindex_t t_char_count;
    if (p_is_text)
        /* UNCHECKED */ MCStringConvertToUTF8(*t_string, t_result, t_char_count);
    else
        /* UNCHECKED */ MCStringConvertToNative(*t_string, (char_t*&)t_result, t_char_count);
    
    // SN-2014-04-07 [[ Bug 12118 ]] revExecuteSQL writes incomplete data into SQLite BLOB columns
    // arg3 is not a char* but rather a MCString; whence setting the length should not be forgotten,
    // in case '\0' are present in the value fetched.
	value -> set(t_result, t_char_count);
	return NULL;
}

static char *get_variable_ex_utf8_text(const char *arg1, const char *arg2,
                                  const char *arg3, int *retval)
{
    return get_variable_ex_utf8(arg1, arg2, arg3, retval, true);
}

static char *get_variable_ex_utf8_binary(const char *arg1, const char *arg2,
                                  const char *arg3, int *retval)
{
    return get_variable_ex_utf8(arg1, arg2, arg3, retval, false);
}

//////////

static char *set_variable_ex_utf8(const char *arg1, const char *arg2,
                             const char *arg3, int *retval, Bool p_is_text)
{
    MCString *t_value = (MCString*)arg3;
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr_utf8(*MCECptr, arg1,&var));
	if (var == NULL)
		return NULL;
    
    MCAutoStringRef t_string;
    if (p_is_text)
        /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)t_value->getstring(), t_value->getlength(), kMCStringEncodingUTF8, false, &t_string);
    else
        /* UNCHECKED */ MCStringCreateWithOldString(*t_value, &t_string);
    
	if (arg2 != NULL && strlen(arg2) > 0)
	{
		MCNameRef t_key;
        MCAutoStringRef t_key_as_string;
        /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)arg2, strlen(arg2), kMCStringEncodingUTF8, false, &t_key_as_string);
		/* UNCHECKED */ MCNameCreate(*t_key_as_string, t_key);
		var->set(*MCECptr, *t_string, &t_key, 1);
		MCValueRelease(t_key);
	}
	else
		var->set(*MCECptr, *t_string);
    
	return NULL;
}

static char *set_variable_ex_utf8_text(const char *arg1, const char *arg2,
                                  const char *arg3, int *retval)
{
    return set_variable_ex_utf8(arg1, arg2, arg3, retval, true);
}

static char *set_variable_ex_utf8_binary(const char *arg1, const char *arg2,
                                  const char *arg3, int *retval)
{
    return set_variable_ex_utf8(arg1, arg2, arg3, retval, false);
}

//////////

static bool get_array_element_utf8(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	get_array_element_t *ctxt;
	ctxt = (get_array_element_t *)p_context;
    
	char* t_key;
	MCStringConvertToUTF8String(MCNameGetString(p_key), t_key);
    
	ctxt -> keys[ctxt -> index] = t_key;
	MCExternalAddAllocatedString(MCexternalallocpool, t_key);
    
	if (ctxt -> strings != nil)
	{
        // The value needs to be converted as a UTF8/C-string
        uindex_t t_length;
        char *t_chars;
        MCAutoStringRef t_string;
        
        if (!MCECptr -> ConvertToString(p_value, &t_string))
            t_string = kMCEmptyString;
        
        if (ctxt -> is_text)
            /* UNCHECKED */ MCStringConvertToUTF8(*t_string, t_chars, t_length);
        else
            /* UNCHECKED */ MCStringConvertToNative(*t_string, (char_t*&)t_chars, t_length);
        
        ctxt -> strings[ctxt -> index] . length = (int)t_length;
        ctxt -> strings[ctxt -> index] . sptr = (const char*)t_chars;
        
        MCExternalAddAllocatedString(MCexternalallocpool, (char*)t_chars);
	}
	ctxt -> index++;
    
	if (ctxt -> index < ctxt -> limit)
		return true;
    
	return false;
}

static char *get_array_utf8(const char *arg1, const char *arg2,
                       const char *arg3, int *retval, bool p_is_text)
{
	MCarray *value = (MCarray *)arg3;
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr_utf8(*MCECptr, arg1,&var));
	if (var == NULL)
		return NULL;
    
	if (!var -> isarray())
		return NULL;
    
    MCArrayRef t_array;
    t_array = (MCArrayRef)var -> getvalueref();
	if (t_array == nil)
		return NULL;
    
	if (value->nelements == 0)
	{
		value->nelements = MCArrayGetCount(t_array);
		return NULL;
	}
    
	value->nelements = MCU_min(value->nelements, MCArrayGetCount(t_array));
    
	get_array_element_t t_ctxt;
	t_ctxt . index = 0;
	t_ctxt . limit = value -> nelements;
	t_ctxt . keys = value -> keys;
    t_ctxt . strings = value -> strings;
    t_ctxt . is_text = p_is_text;
	MCArrayApply(t_array, get_array_element_utf8, &t_ctxt);
    
	return NULL;
    
#if 0
	var -> getvalue() . getkeys(value->keys, value->nelements);
	if (value->strings != NULL)
	{
        
		MCVariableValue& t_var_value = var -> getvalue();
        
		for (unsigned int i = 0; i < value->nelements; i++)
		{
			MCString t_string;
			MCstring *t_value_ptr;
			t_value_ptr = &value -> strings[i];
			
			MCVariableValue *t_entry;
			t_var_value . lookup_element(*MCEPptr, value -> keys[i], t_entry);
			if (t_entry -> is_number())
			{
				t_entry -> ensure_string(*MCEPptr);
				t_string = t_entry -> get_string();
			}
			else if (t_entry -> is_string())
			{
				t_string = t_entry -> get_string();
			}
			else
			{
				t_string = MCnullmcstring;
			}
            
			t_value_ptr -> length = t_string . getlength();
			t_value_ptr -> sptr = t_string . getstring();
		}
	}
	return NULL;
#endif
}

static char *get_array_utf8_text(const char *arg1, const char *arg2,
                                   const char *arg3, int *retval)
{
    return get_array_utf8(arg1, arg2, arg3, retval, true);
}

static char *get_array_utf8_binary(const char *arg1, const char *arg2,
                                   const char *arg3, int *retval)
{
    return get_array_utf8(arg1, arg2, arg3, retval, false);
}

//////////

static char *set_array_utf8(const char *arg1, const char *arg2,
                       const char *arg3, int *retval, bool p_is_text)
{
	MCarray *value = (MCarray *)arg3;
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr_utf8(*MCECptr, arg1,&var));
	if (var == NULL)
		return NULL;
	var->remove(*MCECptr, nil, 0);//clear variable
	char tbuf[U4L];
	for (unsigned int i = 0; i <value->nelements; i++)
	{
		MCString *s = (MCString *)&value->strings[i];
        MCAutoStringRef t_string;
        if (p_is_text)
            /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)s->getstring(), s->getlength(), kMCStringEncodingUTF8, false, &t_string);
        else
            /* UNCHECKED */ MCStringCreateWithOldString(*s, &t_string);
        
        MCNameRef t_key;
		if (value->keys == NULL ||  value->keys[i] == NULL)
		{
			sprintf(tbuf,"%d",i+1);
			/* UNCHECKED */ MCNameCreateWithCString(tbuf, t_key);
		}
		else
        {
            MCAutoStringRef t_key_as_string;
            MCStringCreateWithBytes((byte_t*)value -> keys[i], strlen(value -> keys[i]), kMCStringEncodingUTF8, false, &t_key_as_string);
            /* UNCHECKED */ MCNameCreate(*t_key_as_string, t_key);
        }
		var->set(*MCECptr, *t_string, &t_key, 1);
        
        MCNameDelete(t_key);
	}
	return NULL;
}

static char *set_array_utf8_text(const char *arg1, const char *arg2,
                            const char *arg3, int *retval)
{
    return set_array_utf8(arg1, arg2, arg3, retval, true);
}

static char *set_array_utf8_binary(const char *arg1, const char *arg2,
                                 const char *arg3, int *retval)
{
    return set_array_utf8(arg1, arg2, arg3, retval, false);
}


////////////////////////////////////////////////////////////////////////////////
// IM-2014-07-09: [[ Bug 12225 ]] Add external functions to convert stack coordinates based on current transform

// convert stack to logical window coords
static char *stack_to_window_rect(const char *arg1, const char *arg2,
									   const char *arg3, int *retval)
{
	uintptr_t t_win_id;
	t_win_id = uintptr_t(arg1);

	MCStack *t_stack;
	t_stack = MCdispatcher->findstackwindowid(t_win_id);

	if (t_stack == nil)
	{
		*retval = xresFail;
		return nil;
	}

	MCRectangle32 *t_rect;
	t_rect = (MCRectangle32*)arg2;

	*t_rect = MCRectangle32GetTransformedBounds(*t_rect, t_stack->getviewtransform());

	// IM-2014-07-09: [[ Bug 12602 ]] Convert window -> screen coords
	*t_rect = MCRectangle32FromMCRectangle(MCscreen->logicaltoscreenrect(MCRectangle32ToMCRectangle(*t_rect)));

	*retval = xresSucc;
	return nil;
}

// convert logical window to stack coords
static char *window_to_stack_rect(const char *arg1, const char *arg2,
									   const char *arg3, int *retval)
{
	uintptr_t t_win_id;
	t_win_id = (uintptr_t)arg1;

	MCStack *t_stack;
	t_stack = MCdispatcher->findstackwindowid(t_win_id);

	if (t_stack == nil)
	{
		*retval = xresFail;
		return nil;
	}

	MCRectangle32 *t_rect;
	t_rect = (MCRectangle32*)arg2;

	// IM-2014-07-09: [[ Bug 12602 ]] Convert screen -> window coords
	*t_rect = MCRectangle32FromMCRectangle(MCscreen->screentologicalrect(MCRectangle32ToMCRectangle(*t_rect)));

	*t_rect = MCRectangle32GetTransformedBounds(*t_rect, MCGAffineTransformInvert(t_stack->getviewtransform()));

	*retval = xresSucc;
	return nil;
}

// IM-2014-03-06: [[ revBrowserCEF ]] Add externals extension to the callback list
// SN-2014-07-08: [[ UnicodeExternalsV0 ]] Add externals extension to handle UTF8-encoded parameters
XCB MCcbs[] =
{
	// Externals interface V0 functions
	set_idle_func,
	NULL,
	set_idle_rate,
	card_message,
	mc_message,
	eval_expr,
	get_global,
	set_global,
	get_field_by_name,
	get_field_by_num,
	get_field_by_id,
	set_field_by_name,
	set_field_by_num,
	set_field_by_id,
	show_image_by_name,
	show_image_by_num,
	show_image_by_id,
	get_variable,
	set_variable,
	get_variable_ex,
	set_variable_ex,
	get_array,
	set_array,

	// Externals interface V1 functions
	add_runloop_action,
	remove_runloop_action,
	runloop_wait,
    
	stack_to_window_rect,
	window_to_stack_rect,
    
    // Externals interface unicode functions
    // SN-2014-07-22: [[ Bug 12874 ]] revBrowser (both original and CEF) crashes LiveCode 7.0 DP7
    //  added *_by_id_* in the MCcbs...
	card_message_utf8,
	eval_expr_utf8,
	get_global_utf8,
	set_global_utf8,
	get_field_by_name_utf8,
	get_field_by_num_utf8,
    get_field_by_id_utf8,
	set_field_by_name_utf8,
	set_field_by_num_utf8,
    set_field_by_id_utf8,
	show_image_by_name_utf8,
	show_image_by_num_utf8,
    show_image_by_id_utf8,
	get_variable_utf8,
	set_variable_utf8,
	get_variable_ex_utf8_text,
	get_variable_ex_utf8_binary,
	set_variable_ex_utf8_text,
	set_variable_ex_utf8_binary,
	get_array_utf8_text,
	get_array_utf8_binary,
	set_array_utf8_text,
	set_array_utf8_binary,

	NULL
};

////////////////////////////////////////////////////////////////////////////////

static int can_access_file(const char *p_file)
{
	return MCSecureModeCanAccessDisk();
}

static int can_access_host(const char *p_host)
{
	if (MCSecureModeCanAccessNetwork())
		return true;
    MCAutoStringRef t_host;
    /* UNCHECKED */ MCStringCreateWithCString(p_host, &t_host);
	return MCModeCanAccessDomain(*t_host);
}

static int can_access_library(const char *p_host)
{
	return true;
}

// SN-2014-07-08: [[ UnicodeExternalsV0 ]] Added can_access* functions with UTF8-encoded parameters
static int can_access_file_utf8(const char *p_file)
{
	return MCSecureModeCanAccessDisk();
}

static int can_access_host_utf8(const char *p_host)
{
	if (MCSecureModeCanAccessNetwork())
		return true;
    MCAutoStringRef t_host;
    /* UNCHECKED */ MCStringCreateWithBytes((byte_t*)p_host, strlen(p_host), kMCStringEncodingUTF8, false, &t_host);
	return MCModeCanAccessDomain(*t_host);
}

static int can_access_library_utf8(const char *p_host)
{
	return true;
}

SECURITYHANDLER MCsecuritycbs[] =
{
	can_access_file,
    can_access_host,
	can_access_library,
    
    // SN-2014-07-08: [[ UnicodeExternalsV0 ]] Added new unicode functions in the table
	can_access_file_utf8,
    can_access_host_utf8,
	can_access_library_utf8
};
