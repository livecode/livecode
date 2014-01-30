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

////////////////////////////////////////////////////////////////////////////////
//
//  Private Source File:
//    internal_development.cpp
//
//  Description:
//    This file contains the implementation of the internal engine command
//    syntax for the ide engine.
//
//  Todo:
//    2009-06-29 MW Add support for generate_uuid for linux where libuuid is not
//                  available.
//
//  Changes:
//    2010-05-09 MW Refactored from internal.cpp
//
////////////////////////////////////////////////////////////////////////////////

#include "prefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "execpt.h"
#include "handler.h"
#include "scriptpt.h"
#include "newobj.h"
#include "cmds.h"
#include "chunk.h"
#include "param.h"
#include "mcerror.h"
#include "property.h"
#include "object.h"
#include "button.h"
#include "field.h"
#include "image.h"
#include "card.h"
#include "eps.h"
#include "graphic.h"
#include "group.h"
#include "scrolbar.h"
#include "player.h"
#include "aclip.h"
#include "vclip.h"
#include "stack.h"
#include "dispatch.h"
#include "stacklst.h"
#include "sellst.h"
#include "undolst.h"
#include "util.h"
#include "date.h"
#include "printer.h"
#include "hndlrlst.h"
#include "osspec.h"

#include "debug.h"

#include "globals.h"
#include "internal.h"
#include "ide.h"
#include "bsdiff.h"

#ifdef _WINDOWS
#include "w32prefix.h"
#endif

////////////////////////////////////////////////////////////////////////////////

enum MCInternalType
{
	INTERNAL_TYPE_VOID,
	INTERNAL_TYPE_BOOL,
	INTERNAL_TYPE_BOOLEAN,
	INTERNAL_TYPE_UINT8,
	INTERNAL_TYPE_SINT8,
	INTERNAL_TYPE_UINT16,
	INTERNAL_TYPE_SINT16,
	INTERNAL_TYPE_UINT32,
	INTERNAL_TYPE_SINT32,
	INTERNAL_TYPE_UINT64,
	INTERNAL_TYPE_SINT64,
	INTERNAL_TYPE_CSTRING,
	INTERNAL_TYPE_STRING,
	INTERNAL_TYPE_REAL4,
	INTERNAL_TYPE_REAL8
};

struct MCInternalMethod
{
	MCInternalType return_type;
	const char *name;
	void (*pointer)(void **);
};

void internal_generate_uuid(void **r_result);
void internal_notify_association_changed(void **r_result);

static MCInternalMethod s_internal_methods[] =
{
	{INTERNAL_TYPE_CSTRING, "generate_uuid", internal_generate_uuid},
#ifdef WIN32
	{INTERNAL_TYPE_VOID, "notify_association_changed", internal_notify_association_changed},
#endif
	{INTERNAL_TYPE_VOID, NULL, NULL},
};

const int s_internal_method_count = sizeof(s_internal_methods) / sizeof(MCInternalMethod);

////////////////////////////////////////////////////////////////////////////////

static Exec_stat internal_get_value(MCExecPoint& ep, MCInternalType p_type, void *p_value)
{
	switch(p_type)
	{
		case INTERNAL_TYPE_BOOL:
			ep . setboolean(*(bool *)p_value);
		break;

		case INTERNAL_TYPE_BOOLEAN:
			ep . setboolean(*(Boolean *)p_value);
		break;

		case INTERNAL_TYPE_UINT8:
			ep . setnvalue(*(uint1 *)p_value);
		break;

		case INTERNAL_TYPE_SINT8:
			ep . setnvalue(*(int1 *)p_value);
		break;

		case INTERNAL_TYPE_UINT16:
			ep . setnvalue(*(uint2 *)p_value);
		break;

		case INTERNAL_TYPE_SINT16:
			ep . setnvalue(*(int2 *)p_value);
		break;

		case INTERNAL_TYPE_UINT32:
			ep . setnvalue(*(uint4 *)p_value);
		break;

		case INTERNAL_TYPE_SINT32:
			ep . setnvalue(*(int4 *)p_value);
		break;

		case INTERNAL_TYPE_CSTRING:
			ep . copysvalue((char *)p_value, strlen((char *)p_value));
		break;

		case INTERNAL_TYPE_STRING:
			ep . copysvalue(((MCString *)p_value) -> getstring(), ((MCString *)p_value) -> getlength());
		break;

		case INTERNAL_TYPE_REAL4:
			ep . setnvalue(*(float *)p_value);
		break;

		case INTERNAL_TYPE_REAL8:
			ep . setnvalue(*(double *)p_value);
		break;

		default:
			ep . clear();
		break;
	}

	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

class MCInternalBreak: public MCStatement
{
public:
	MCInternalBreak(void) {}
	~MCInternalBreak(void) {}

	Parse_stat parse(MCScriptPoint& sp)
	{
		return PS_NORMAL;
	}

	Exec_stat exec(MCExecPoint& ep)
	{
#if defined(WIN32) && defined(_DEBUG)
		_CrtDbgBreak();
#endif
		return ES_NORMAL;
	}
};

class MCInternalCall: public MCStatement
{
public:
	MCInternalCall(void)
	{
		m_noun = nil;
	}

	~MCInternalCall(void)
	{
		delete m_noun;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		if (sp . parseexp(False, True, &m_noun) != PS_NORMAL)
		{
			MCperror -> add(PE_PUT_BADEXP, sp);
			return PS_ERROR;
		}

		return PS_NORMAL;
	}

	Exec_stat exec(MCExecPoint& ep)
	{
		if (m_noun -> eval(ep) != ES_NORMAL)
			return ES_ERROR;

		MCInternalMethod *t_method;
		t_method = NULL;
		for(int n = 0; n < s_internal_method_count; ++n)
			if (ep . getsvalue() == s_internal_methods[n] . name)
			{
				t_method = &s_internal_methods[n];
				break;
			}

		if (t_method == NULL)
		{
			MCeerror -> add(EE_PUT_CANTSET, line, pos);
			return ES_ERROR;
		}

		void *t_value;
		t_method -> pointer(&t_value);
		if (internal_get_value(ep, t_method -> return_type, t_value) != ES_NORMAL)
			return ES_ERROR;

		return MCresult -> store(ep, False);
	}

private:
	MCExpression *m_noun;
};

class MCInternalBsDiff: public MCStatement
{
public:
	MCInternalBsDiff(void)
	{
		m_old_file = nil;
		m_new_file = nil;
		m_patch_file = nil;
	}

	~MCInternalBsDiff(void)
	{
		delete m_old_file;
		delete m_new_file;
		delete m_patch_file;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);

		if (sp . parseexp(False, True, &m_old_file) != PS_NORMAL)
			return PS_ERROR;
		if (sp . skip_token(SP_FACTOR, TT_TO, PT_TO) != PS_NORMAL)
			return PS_ERROR;
		if (sp . parseexp(False, True, &m_new_file) != PS_NORMAL)
			return PS_ERROR;
		if (sp . skip_token(SP_FACTOR, TT_PREP, PT_INTO) != PS_NORMAL)
			return PS_ERROR;
		if (sp . parseexp(False, True, &m_patch_file) != PS_NORMAL)
			return PS_ERROR;

		return PS_NORMAL;
	}

	Exec_stat exec(MCExecPoint& ep)
	{
		bool t_success;
		t_success = true;

		char *t_old_filename;
		t_old_filename = nil;
		if (t_success && m_old_file -> eval(ep) == ES_NORMAL)
			t_old_filename = ep . getsvalue() . clone();
		else
			t_success = false;

		char *t_new_filename;
		t_new_filename = nil;
		if (t_success && m_new_file -> eval(ep) == ES_NORMAL)
			t_new_filename = ep . getsvalue() . clone();
		else
			t_success = false;

		char *t_patch_filename;
		t_patch_filename = nil;
		if (t_success && m_patch_file -> eval(ep) == ES_NORMAL)
			t_patch_filename = ep . getsvalue() . clone();
		else
			t_success = false;

		IO_handle t_old_handle;
		t_old_handle = nil;
		if (t_success)
		{
			t_old_handle = MCS_open(t_old_filename, IO_READ_MODE, False, False, 0);
			if (t_old_handle == nil)
				t_success = false;
		}

		IO_handle t_new_handle;
		t_new_handle = nil;
		if (t_success)
		{
			t_new_handle = MCS_open(t_new_filename, IO_READ_MODE, False, False, 0);
			if (t_new_handle == nil)
				t_success = false;
		}

		IO_handle t_patch_handle;
		t_patch_handle = nil;
		if (t_success)
		{
			t_patch_handle = MCS_open(t_patch_filename, IO_WRITE_MODE, False, False, 0);
			if (t_patch_handle == nil)
				t_success = false;
		}

		if (t_success)
		{
			InputStream t_new_stream, t_old_stream;
			OutputStream t_patch_stream;
			t_new_stream . handle = t_new_handle;
			t_old_stream . handle = t_old_handle;
			t_patch_stream . handle = t_patch_handle;
			t_success = MCBsDiffBuild(&t_old_stream, &t_new_stream, &t_patch_stream);
		}

		if (t_success)
			MCresult -> clear();
		else
			MCresult -> sets("patch building failed");

		if (t_patch_handle != nil)
			MCS_close(t_patch_handle);
		if (t_new_handle != nil)
			MCS_close(t_new_handle);
		if (t_old_handle != nil)
			MCS_close(t_old_handle);

		delete t_patch_filename;
		delete t_new_filename;
		delete t_old_filename;
		
		return ES_NORMAL;
	}

private:
	struct InputStream: public MCBsDiffInputStream
	{
		IO_handle handle;

		bool Measure(uint32_t& r_size)
		{
			r_size = (uint32_t)MCS_fsize(handle);
			return true;
		}

		bool ReadBytes(void *p_buffer, uint32_t p_count)
		{
			return IO_read_bytes(p_buffer, p_count, handle) == IO_NORMAL;

		}

		bool ReadInt32(int32_t& r_value)
		{
			return IO_read_int4(&r_value, handle) == IO_NORMAL;
		}
	};

	struct OutputStream: public MCBsDiffOutputStream
	{
		IO_handle handle;

		bool Rewind(void)
		{
			return MCS_seek_set(handle, 0) == IO_NORMAL;
		}

		bool WriteBytes(const void *buffer, uint32_t count)
		{
			return IO_write(buffer, 1, count, handle) == IO_NORMAL;
		}

		bool WriteInt32(int32_t value)
		{
			return IO_write_int4(value, handle) == IO_NORMAL;
		}
	};

	MCExpression *m_old_file;
	MCExpression *m_new_file;
	MCExpression *m_patch_file;
};

////////////////////////////////////////////////////////////////////////////////

class MCInternalBootstrap: public MCStatement
{
public:
	MCInternalBootstrap(void)
	{
		m_stackfile = nil;
	}

	~MCInternalBootstrap(void)
	{
		delete m_stackfile;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		if (sp . parseexp(False, True, &m_stackfile) != PS_NORMAL)
		{
			MCperror -> add(PE_PUT_BADEXP, sp);
			return PS_ERROR;
		}

		return PS_NORMAL;
	}

	Exec_stat exec(MCExecPoint& ep)
	{
		if (m_stackfile -> eval(ep) != ES_NORMAL)
			return ES_ERROR;

		MCStack *t_new_home;
		t_new_home = MCdispatcher -> findstackname(ep . getsvalue() . getstring());

		if (t_new_home == nil &&
			MCdispatcher -> loadfile(ep . getcstring(), t_new_home) != IO_NORMAL)
			return ES_ERROR;

		MCdispatcher -> changehome(t_new_home);	

		MCdefaultstackptr = MCstaticdefaultstackptr = t_new_home;
		MCdefaultstackptr -> setextendedstate(true, ECS_DURING_STARTUP);
		MCdefaultstackptr -> getcard() -> message(MCM_start_up);
		MCdefaultstackptr -> setextendedstate(false, ECS_DURING_STARTUP);
		if (!MCquit)
			t_new_home -> open();

		return ES_NORMAL;
	}

private:
	MCExpression *m_stackfile;
};

////////////////////////////////////////////////////////////////////////////////
// MM-2012-09-05: [[ Property Listener ]]

#ifdef FEATURE_PROPERTY_LISTENER

struct MCObjectListenerTarget
{
	MCObjectHandle *target;
	MCObjectListenerTarget *next;
};

struct MCObjectListener
{
	MCObjectHandle *object;
	MCObjectListenerTarget *targets;
	MCObjectListener *next;
	double last_update_time;
};

static MCObjectListener *s_object_listeners = nil;

// MW-2013-08-27: [[ Bug 11126 ]] Whilst processing, these statics hold the next
//   listener / target to process, thus avoiding dangling pointers.
static MCObjectListener *s_next_listener_to_process = nil;
static MCObjectListenerTarget *s_next_listener_target_to_process = nil;

static void remove_object_listener_from_list(MCObjectListener *&p_listener, MCObjectListener *p_prev_listener)
{
	MCObjectListenerTarget *t_target;
	t_target = nil;
	MCObjectListenerTarget *t_next_target;
	t_next_target = nil;
	for (t_target = p_listener -> targets; t_target != nil; t_target = t_next_target)
	{
		t_next_target = t_target -> next;
		t_target -> target -> Release();
		MCMemoryDelete(t_target);
	}	
	
	if (p_listener -> object -> Exists())
		p_listener -> object -> Get() -> unlisten();
	p_listener -> object -> Release();
	if (p_prev_listener != nil)
		p_prev_listener -> next = p_listener -> next;
	else
		s_object_listeners = p_listener -> next;
	// MW-2013-08-27: [[ Bug 11126 ]] If this pointer is going away, make sure we fetch the next
	//   listener into the static.
	if (s_next_listener_to_process == p_listener)
		s_next_listener_to_process = p_listener -> next;
	MCMemoryDelete(p_listener);
	p_listener = p_prev_listener;
}

static void remove_object_listener_target_from_list(MCObjectListenerTarget *&p_target, MCObjectListenerTarget *p_prev_target, MCObjectListener* &p_listener, MCObjectListener* p_prev_listener)
{
	p_target -> target -> Release();
	if (p_prev_target != nil)
		p_prev_target -> next = p_target -> next;
	else
		p_listener -> targets = p_target -> next;
	if (p_listener -> targets == nil)
		remove_object_listener_from_list(p_listener, p_prev_listener);
	// MW-2013-08-27: [[ Bug 11126 ]] If this pointer is going away, make sure we fetch the next
	//   target into the static.
	if (p_target == s_next_listener_target_to_process)
		s_next_listener_target_to_process = p_target -> next;
	MCMemoryDelete(p_target);
	p_target = p_prev_target;
}

void MCInternalObjectListenerMessagePendingListeners(void)
{
	if (MCobjectpropertieschanged)
	{
		MCobjectpropertieschanged = False;
		
		MCObjectListener *t_prev_listener;
		t_prev_listener = nil;
		
		MCObjectListener *t_listener;
		t_listener = s_object_listeners;
		while(t_listener != nil)
		{
			// MW-2013-08-27: [[ Bug 11126 ]] This static is updated by the remove_* functions
			//   to ensure we don't get any dangling pointers.
			s_next_listener_to_process = t_listener -> next;
			
			if (!t_listener -> object -> Exists())
				remove_object_listener_from_list(t_listener, t_prev_listener);
			else
			{
				uint8_t t_properties_changed;
				t_properties_changed = t_listener -> object -> Get() -> propertieschanged();
				if (t_properties_changed != kMCPropertyChangedMessageTypeNone)
				{			
					MCExecPoint ep(nil, nil, nil);
					t_listener -> object -> Get() -> getprop(0, P_LONG_ID, ep, False);			
					
					MCObjectListenerTarget *t_target;
					t_target = nil;
					MCObjectListenerTarget *t_prev_target;
					t_prev_target = nil;	
					
					double t_new_time;
					t_new_time = MCS_time();
					if (t_listener -> last_update_time + MCpropertylistenerthrottletime / 1000.0 < t_new_time)
					{
						t_listener -> last_update_time = t_new_time;
						t_target = t_listener->targets;
						while (t_target != nil)
						{
							// MW-2013-08-27: [[ Bug 11126 ]] This static is updated by the remove_* functions
							//   to ensure we don't get any dangling pointers.
							s_next_listener_target_to_process = t_target -> next;
							
							if (!t_target -> target -> Exists())
								remove_object_listener_target_from_list(t_target, t_prev_target, t_listener, t_prev_listener);
							else
							{
								// MM-2012-11-06: Added resizeControl(Started/Ended) and gradientEdit(Started/Ended) messages.
								if (t_properties_changed & kMCPropertyChangedMessageTypePropertyChanged)								
									t_target -> target -> Get() -> message_with_args(MCM_property_changed, ep . getsvalue());
								if (t_properties_changed & kMCPropertyChangedMessageTypeResizeControlStarted)								
									t_target -> target -> Get() -> message_with_args(MCM_resize_control_started, ep . getsvalue());
								if (t_properties_changed & kMCPropertyChangedMessageTypeResizeControlEnded)								
									t_target -> target -> Get() -> message_with_args(MCM_resize_control_ended, ep . getsvalue());
								if (t_properties_changed & kMCPropertyChangedMessageTypeGradientEditStarted)								
									t_target -> target -> Get() -> message_with_args(MCM_gradient_edit_started, ep . getsvalue());
								if (t_properties_changed & kMCPropertyChangedMessageTypeGradientEditEnded)								
									t_target -> target -> Get() -> message_with_args(MCM_gradient_edit_ended, ep . getsvalue());
								
								t_prev_target = t_target;					
							}
							
							t_target = s_next_listener_target_to_process;
						}
					}
					else
						t_listener -> object -> Get() -> signallistenerswithmessage(t_properties_changed);
				}
				
				t_prev_listener = t_listener;
			}
			
			// MW-2013-08-27: [[ Bug 11126 ]] Use the static as the next in the chain.
			t_listener = s_next_listener_to_process;
		}
	}
}

void MCInternalObjectListenerListListeners(MCExecPoint &ep)
{
	ep . clear();
	MCObjectHandle *t_current_object;
	t_current_object = ep . getobj() -> gethandle();
	
	bool t_first;
	t_first = true;
	
	MCObjectListener *t_prev_listener;
	t_prev_listener = nil;
	
	MCObjectListener *t_listener;
	t_listener = s_object_listeners;
	while(t_listener != nil)
	{
		if (!t_listener -> object -> Exists())
			remove_object_listener_from_list(t_listener, t_prev_listener);
		else
		{				
			MCObjectListenerTarget *t_target;
			t_target = nil;
			MCObjectListenerTarget *t_prev_target;
			t_prev_target = nil;	
			
			MCExecPoint ep1(ep);
			t_listener -> object -> Get() -> getprop(0, P_LONG_ID, ep1, false);
						
			for (t_target = t_listener -> targets; t_target != nil; t_target = t_target -> next)
			{
				if (!t_target -> target -> Exists())
					remove_object_listener_target_from_list(t_target, t_prev_target, t_listener, t_prev_listener);
				else if (t_target -> target ==  t_current_object)
				{
					ep . concatmcstring(ep1 . getsvalue(), EC_RETURN, t_first);
					t_first = false;		
				}
			}
			
			t_prev_listener = t_listener;
		}
		
		if (t_listener != nil)
			t_listener = t_listener -> next;
		else
			t_listener = s_object_listeners;
	}
}

class MCInternalObjectListen: public MCStatement
{
public:
	MCInternalObjectListen(void)
	{
		m_object = nil;
	}
	
	~MCInternalObjectListen(void)
	{
		if (m_object != nil)
			delete m_object;
	}
	
	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);
		if (sp . skip_token(SP_FACTOR, TT_TO, PT_TO) != PS_NORMAL)
		{
			MCperror -> add(PE_OBJECT_NAME, sp);
			return PS_ERROR;
		}				
		m_object = new MCChunk(False);
		if (m_object -> parse(sp, False) != PS_NORMAL)
		{
			MCperror -> add(PE_OBJECT_NAME, sp);
			return PS_ERROR;
		}
		return PS_NORMAL;		
	}
	
	Exec_stat exec(MCExecPoint& ep)
	{
		MCObject *t_object;
		uint4 parid;
		if (m_object -> getobj(ep, t_object, parid, True) != ES_NORMAL)
		{
			MCeerror -> add(EE_NO_MEMORY, line, pos);
			return ES_ERROR;
		}		
		
		MCObjectListener *t_listener;
		t_listener = nil;
		
		MCObjectHandle *t_object_handle;
		t_object_handle = t_object -> gethandle();
		
		for (t_listener = s_object_listeners; t_listener != nil; t_listener = t_listener -> next)
		{
			if (t_listener -> object == t_object_handle)
				break;
		}
			
		if (t_listener == nil)
		{
			if (!MCMemoryNew(t_listener))
			{
				MCeerror -> add(EE_NO_MEMORY, line, pos);
				return ES_ERROR;
			}
			t_object -> listen();
			t_listener -> object = t_object_handle;
			t_listener -> object -> Retain();
			t_listener -> targets = nil;
			t_listener -> next = s_object_listeners;
			t_listener -> last_update_time = 0.0;
			s_object_listeners = t_listener;			
		}
		
		MCObjectListenerTarget *t_target;
		t_target = nil;
		
		MCObjectHandle *t_target_object;
		t_target_object = ep . getobj() -> gethandle();
		
		for (t_target = t_listener -> targets; t_target != nil; t_target = t_target -> next)
		{
			if (t_target -> target == t_target_object)
				break;
		}
		
		if (t_target == nil)
		{
			if (!MCMemoryNew(t_target))
			{
				MCeerror -> add(EE_NO_MEMORY, line, pos);
				return ES_ERROR;
			}
			t_target -> target = t_target_object;
			t_target -> target -> Retain();
			t_target -> next = t_listener -> targets;
			t_listener -> targets = t_target;						
		}
		
		return ES_NORMAL;
	}
	
private:
	MCChunk *m_object;
};

class MCInternalObjectUnListen: public MCStatement
{
public:
	MCInternalObjectUnListen(void)
	{
		m_object = nil;
	}
	
	~MCInternalObjectUnListen(void)
	{
		if (m_object != nil)
			delete m_object;
	}
	
	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);
		if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_LISTENER) != PS_NORMAL)
		{
			MCperror -> add(PE_OBJECT_NAME, sp);
			return PS_ERROR;
		}
		if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_FOR) != PS_NORMAL)
		{
			MCperror -> add(PE_OBJECT_NAME, sp);
			return PS_ERROR;
		}
		m_object = new MCChunk(False);
		if (m_object -> parse(sp, False) != PS_NORMAL)
		{
			MCperror -> add(PE_OBJECT_NAME, sp);
			return PS_ERROR;
		}
		return PS_NORMAL;
	}
	
	Exec_stat exec(MCExecPoint& ep)
	{
		MCObject *t_object;
		uint4 parid;
		if (m_object -> getobj(ep, t_object, parid, True) != ES_NORMAL)
			return ES_NORMAL;
		
		MCObjectListener *t_prev_listener;
		t_prev_listener = nil;
		MCObjectListener *t_listener;
		t_listener = nil;
		
		MCObjectHandle *t_object_handle;
		t_object_handle = t_object -> gethandle();
		
		for (t_listener = s_object_listeners; t_listener != nil; t_listener = t_listener -> next)
		{
			if (t_listener -> object == t_object_handle)
				break;
			t_prev_listener = t_listener;
		}
		
		if (t_listener != nil)
		{
			MCObjectListenerTarget *t_target;
			t_target = nil;
			MCObjectListenerTarget *t_prev_target;
			t_prev_target = nil;
			
			MCObjectHandle *t_target_object;
			t_target_object = ep . getobj() -> gethandle();
			
			for (t_target = t_listener -> targets; t_target != nil; t_target = t_target -> next)
			{
				if (t_target -> target == t_target_object)
				{
					remove_object_listener_target_from_list(t_target, t_prev_target, t_listener, t_prev_listener);
					return ES_NORMAL;
				}
				t_prev_target = t_target;
			}			
		}
		
		return ES_NORMAL;
	}
	
private:
	MCChunk *m_object;
};

#endif

////////////////////////////////////////////////////////////////////////////////

template<class T> inline MCStatement *class_factory(void)
{
	return new T;
}

MCInternalVerbInfo MCinternalverbs[] =
{
	{ "break", nil, class_factory<MCInternalBreak> },
	{ "call", nil, class_factory<MCInternalCall> },
	{ "deploy", nil, class_factory<MCIdeDeploy> },
	{ "sign", nil, class_factory<MCIdeSign> },
	{ "diet", nil, class_factory<MCIdeDiet> },
	{ "extract", nil, class_factory<MCIdeExtract> },
	{ "script", "flush", class_factory<MCIdeScriptFlush> },
	{ "script", "colorize", class_factory<MCIdeScriptColourize> },
	{ "script", "configure", class_factory<MCIdeScriptConfigure> },
	{ "script", "describe", class_factory<MCIdeScriptDescribe> },
	{ "script", "replace", class_factory<MCIdeScriptReplace> },
	{ "script", "strip", class_factory<MCIdeScriptStrip> },
	{ "script", "tokenize", class_factory<MCIdeScriptTokenize> },
	{ "script", "classify", class_factory<MCIdeScriptClassify> },
	{ "bsdiff", nil, class_factory<MCInternalBsDiff> },
	{ "dmg", "dump", class_factory<MCIdeDmgDump> },
	{ "dmg", "build", class_factory<MCIdeDmgBuild> },
	{ "bootstrap", nil, class_factory<MCInternalBootstrap> },
#ifdef FEATURE_PROPERTY_LISTENER
	{ "listen", nil, class_factory<MCInternalObjectListen> },
	{ "cancel", nil, class_factory<MCInternalObjectUnListen> },
#endif
	{ "filter", "controls", class_factory<MCIdeFilterControls> },
	{ nil, nil, nil }
};

////////////////////////////////////////////////////////////////////////////////

void internal_generate_uuid(void **r_result)
{
	static char t_result[128];

	if (!MCS_generate_uuid(t_result))
		t_result[0] = '\0';
	
	*r_result = (void *)t_result;
}

#ifdef WIN32
void internal_notify_association_changed(void **r_result)
{
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}
#endif
