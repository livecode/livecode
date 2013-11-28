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

#include "execpt.h"
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

typedef char *(*XCB)(const char *arg1, const char *arg2, const char *arg3, int *retval);
typedef int (*SECURITYHANDLER)(const char *);
typedef void (*DELETER)(void *data);
typedef void (*GETXTABLE)(XCB *, DELETER, const char **, Xternal **, DELETER *);
typedef void (*CONFIGURESECURITY)(SECURITYHANDLER *handlers);
typedef void (*SHUTDOWNXTABLE)(void);

extern XCB MCcbs[];
extern SECURITYHANDLER MCsecuritycbs[];

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
	// Get the info from the main external entry point (we now this symbol exists
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
		if (!p_callback(p_state, m_table[i] . type[0] == XCOMMAND[0] ? HT_MESSAGE : HT_FUNCTION, m_table[i] . name, i))
			return false;

	return true;
	}

Exec_stat MCExternalV0::Handle(MCObject *p_context, Handler_type p_type, uint32_t p_index, MCParameter *p_parameters)
{
	char t_type;
	if (p_type == HT_FUNCTION)
		t_type = XFUNCTION[0];
	else
		t_type = XCOMMAND[0];

	_Xternal *t_handler;
	t_handler = &m_table[p_index];
	if (t_handler -> type[0] != t_type)
		return ES_NOT_HANDLED;

		char *retval;
		Bool Xpass, Xerr;
		int nargs = 0;
		char **args = NULL;
		MCExecPoint ep(p_context, NULL, NULL);
        MCExecContext ctxt(ep);

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

            MCStringConvertToCString(*t_string, args[nargs++]);
			p_parameters = p_parameters -> getnext();
		}

		(t_handler -> call)(args, nargs, &retval, &Xpass, &Xerr);

		// MW-2011-03-02: [[ Bug ]] Memory leak as we aren't freeing any error string that
		//   is returned.
		if (Xerr)
		{
			MCeerror -> add(EE_EXTERNAL_EXCEPTION, 0, 0, retval == NULL ? "" : retval);
			m_free(retval);
		}
		else if (retval == NULL)
            MCresult->clear(False);
		else
		{
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
	if (MCECptr->GetEP().findvar(t_name, &newvar) != PS_NORMAL)
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
		MCExecPoint ep;
        MCExecContext ctxt(ep);
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
	MCExecPoint ep;
    MCExecContext ctxt(ep);
	*retval = xresSucc;
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringCreateWithCString(arg2, &t_string);
	ep.setsvalue(arg2);
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
		fptr->settext_oldstring(fptr->getcard()->getid(), arg3, False);
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
		fptr->settext_oldstring(fptr->getcard()->getid(), arg3, False);
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
		fptr->settext_oldstring(fptr->getcard()->getid(), arg3, False);
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
			MCExecPoint ep2(nil, nil, nil);
            MCExecContext ctxt(ep2);
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
	var -> eval(MCECptr->GetEP());
	return MCECptr->GetEP().getsvalue().clone();
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
	MCECptr->GetEP().setsvalue(arg2);
	var->set(MCECptr->GetEP());
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

	if (arg2 != NULL && strlen(arg2) != 0)
	{
		MCNameRef t_key;
		/* UNCHECKED */ MCNameCreateWithCString(arg2, t_key);
		var -> eval(MCECptr->GetEP(), &t_key, 1);
		MCValueRelease(t_key);
	}
	else
		var -> eval(MCECptr->GetEP());

	*value = MCECptr->GetEP().getsvalue();
	return NULL;
}

static char *set_variable_ex(const char *arg1, const char *arg2,
                             const char *arg3, int *retval)
{
	MCString *value = (MCString *)arg3;
	MCVariable *var = NULL;
	if (MCECptr == NULL)
	{
		*retval = xresFail;
		return NULL;
	}
	*retval = trans_stat(getvarptr(*MCECptr, arg1,&var));
	if (var == NULL)
		return NULL;
	MCECptr->GetEP().setsvalue(*value);
	if (arg2 != NULL && strlen(arg2) > 0)
	{
		MCNameRef t_key;
		/* UNCHECKED */ MCNameCreateWithCString(arg2, t_key);
		var->set(MCECptr->GetEP(), &t_key, 1);
		MCValueRelease(t_key);
	}
	else
		var->set(MCECptr->GetEP());

	return NULL;
}

struct get_array_element_t
{
	uindex_t index;
	uindex_t limit;
	char **keys;
	MCstring *strings;
};

static bool get_array_element(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	get_array_element_t *ctxt;
	ctxt = (get_array_element_t *)p_context;

	ctxt -> keys[ctxt -> index] = (char *)MCNameGetCString(p_key);
	if (ctxt -> strings != nil)
	{
		ctxt -> strings[ctxt -> index] . length = MCStringGetLength((MCStringRef)p_value);
		ctxt -> strings[ctxt -> index] . sptr = (const char *)MCStringGetNativeCharPtr((MCStringRef)p_value);
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

	if (!var -> converttoarrayofstrings(MCECptr->GetEP()))
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
	var->remove(MCECptr->GetEP(),nil, 0);//clear variable
	char tbuf[U4L];
	for (unsigned int i = 0; i <value->nelements; i++)
	{
		MCString *s = (MCString *)&value->strings[i];
		MCECptr->GetEP().setsvalue(*s);
		MCNameRef t_key;
		if (value->keys == NULL ||  value->keys[i] == NULL)
		{
			sprintf(tbuf,"%d",i+1);
			/* UNCHECKED */ MCNameCreateWithCString(tbuf, t_key);
		}
		else
			/* UNCHECKED */ MCNameCreateWithCString(value -> keys[i], t_key);
		var->set(MCECptr->GetEP(), &t_key, 1);
	}
	return NULL;
}

XCB MCcbs[] =
{
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
	set_array
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

SECURITYHANDLER MCsecuritycbs[] =
{
	can_access_file,
    can_access_host,
	can_access_library
};
