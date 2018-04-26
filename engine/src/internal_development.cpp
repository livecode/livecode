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


#include "exec.h"
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

    void exec_ctxt(MCExecContext &ctxt)
	{
#if defined(WIN32) && defined(_DEBUG)
		_CrtDbgBreak();
#endif
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

    void exec_ctxt(MCExecContext &ctxt)
    {
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

    void exec_ctxt(MCExecContext &ctxt)
    {
		bool t_success;
		t_success = true;

		MCAutoStringRef t_old_filename;
        if (!ctxt . EvalExprAsStringRef(m_old_file, EE_INTERNAL_BSDIFF_BADOLD, &t_old_filename))
            return;

		MCAutoStringRef t_new_filename;
        if (!ctxt . EvalExprAsStringRef(m_new_file, EE_INTERNAL_BSDIFF_BADNEW, &t_new_filename))
            return;

		MCAutoStringRef t_patch_filename;
        if (!ctxt . EvalExprAsStringRef(m_patch_file, EE_INTERNAL_BSDIFF_BADPATCH, &t_patch_filename))
            return;

		IO_handle t_old_handle;
		t_old_handle = nil;
		if (t_success)
		{
			t_old_handle = MCS_open(*t_old_filename, kMCOpenFileModeRead, False, False, 0);
			if (t_old_handle == nil)
				t_success = false;
		}

		IO_handle t_new_handle;
		t_new_handle = nil;
		if (t_success)
		{
			t_new_handle = MCS_open(*t_new_filename, kMCOpenFileModeRead, False, False, 0);
			if (t_new_handle == nil)
				t_success = false;
		}

		IO_handle t_patch_handle;
		t_patch_handle = nil;
		if (t_success)
		{
			t_patch_handle = MCS_open(*t_patch_filename, kMCOpenFileModeWrite, False, False, 0);
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
            ctxt . SetTheResultToEmpty();
		else
            ctxt . SetTheResultToCString("patch building failed");

		if (t_patch_handle != nil)
			MCS_close(t_patch_handle);
		if (t_new_handle != nil)
			MCS_close(t_new_handle);
		if (t_old_handle != nil)
            MCS_close(t_old_handle);
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
			return IO_read(p_buffer, p_count, handle) == IO_NORMAL;

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

    void exec_ctxt(MCExecContext &ctxt)
    {
        MCNewAutoNameRef t_name;
        if (!ctxt . EvalExprAsNameRef(m_stackfile, EE_INTERNAL_BOOTSTRAP_BADSTACK, &t_name))
            return;

        MCStack *t_new_home;
		t_new_home = MCdispatcher -> findstackname(*t_name);

		if (t_new_home == nil &&
			MCdispatcher -> loadfile(MCNameGetString(*t_name), t_new_home) != IO_NORMAL)
        {
            ctxt . Throw();
            return;
        }

		MCdispatcher -> changehome(t_new_home);	

		MCdefaultstackptr = MCstaticdefaultstackptr = t_new_home;
		MCdefaultstackptr -> setextendedstate(true, ECS_DURING_STARTUP);
		MCdefaultstackptr -> getcard() -> message(MCM_start_up);
		MCdefaultstackptr -> setextendedstate(false, ECS_DURING_STARTUP);
		if (!MCquit)
            t_new_home -> open();
	}

private:
	MCExpression *m_stackfile;
};

////////////////////////////////////////////////////////////////////////////////
// MM-2012-09-05: [[ Property Listener ]]

#ifdef FEATURE_PROPERTY_LISTENER

struct MCObjectListenerTarget
{
	MCObjectHandle target;
	MCObjectListenerTarget *next;
};

struct MCObjectListener
{
	MCObjectHandle object;
	MCObjectListenerTarget *targets;
	MCObjectListener *next;
	double last_update_time;
};

static MCObjectListener *s_object_listeners = nil;
static bool s_listeners_locked = false;

static void prune_object_listeners()
{
    if (s_listeners_locked)
        return;
    
    MCObjectListener *t_listener;
    t_listener = s_object_listeners;
    
    MCObjectListener *t_prev_listener;
    t_prev_listener = nil;
    
    MCObjectListener *t_next_listener;
    t_next_listener = nil;

    while (t_listener != nil)
    {
        t_next_listener = t_listener -> next;
        
        MCObjectListenerTarget *t_target;
        t_target = t_listener -> targets;
        
        MCObjectListenerTarget *t_prev_target;
        t_prev_target = nil;
        
        MCObjectListenerTarget *t_next_target;
        t_next_target = nil;
        
        bool t_listener_exists;
        t_listener_exists = t_listener -> object . IsValid();
        
        // Check all the listener's targets to see if they can be pruned
        while (t_target != nil)
        {
            t_next_target = t_target -> next;
            
            // Prune target from listener target list if listener
            // doesn't exist, or if target doesn't exist, or if target
            // has been set to nil.
            if (!t_listener_exists || !t_target->target.IsValid())
            {
                // Release the target if it hasn't already been released
                t_target->target = nil;
                
                if (t_prev_target != nil)
                    t_prev_target -> next = t_next_target;
                else
                    t_listener -> targets = t_next_target;
                
                MCMemoryDestroy(t_target);
            }
            else
            {
                t_prev_target = t_target;
            }
            
            t_target = t_next_target;
        }
        
        // If the listening object no longer exists, or the list of
        // listener targets is nil, then prune from listener list.
        if (!t_listener_exists || t_listener -> targets == nil)
        {
            t_listener -> object = nil;
            
            if (t_prev_listener != nil)
                t_prev_listener -> next = t_next_listener;
            else
                s_object_listeners = t_next_listener;
            
            MCMemoryDestroy(t_listener);
        }
        else
        {
            t_prev_listener = t_listener;
        }
        
        t_listener = t_next_listener;
    }
}

void MCInternalObjectListenerMessagePendingListeners(void)
{
	if (MCobjectpropertieschanged)
	{
        // Lock the listener list, so that no items are removed as a result
        // of propertyChenged messages in the IDE. If an object is unlistened
        // to as a result of these messages, Exists() will return false and
        // the object will be skipped, and removed properly later on.
        s_listeners_locked = true;
        
        bool t_changed;
        t_changed = false;
        
		MCobjectpropertieschanged = False;
		
		MCObjectListener *t_listener;
		t_listener = s_object_listeners;
		while(t_listener != nil)
		{
			if (!t_listener -> object.IsValid())
            {
                t_changed = true;
            }
			else
			{
				uint8_t t_properties_changed;
				t_properties_changed = t_listener -> object -> propertieschanged();
				if (t_properties_changed != kMCPropertyChangedMessageTypeNone)
                {
                    MCExecContext ctxt(nil, nil, nil);
					MCAutoStringRef t_string;
					t_listener -> object -> getstringprop(ctxt, 0, P_LONG_ID, False, &t_string);
					MCObjectListenerTarget *t_target;
					t_target = nil;
					
					double t_new_time;
					t_new_time = MCS_time();
					if (t_listener -> last_update_time + MCpropertylistenerthrottletime / 1000.0 < t_new_time)
					{
						t_listener -> last_update_time = t_new_time;
						t_target = t_listener->targets;
                        
						while (t_target != nil)
						{
                            MCObjectHandle t_obj = t_target -> target;
                            
                            bool t_target_exists;
                            t_target_exists = t_obj.IsValid();
                            
                            // Make sure the target object still exists and is still
                            // being listened to before sending any messages.
                            if (t_target_exists
                                && t_properties_changed & kMCPropertyChangedMessageTypePropertyChanged)
                            {
                                t_obj -> message_with_valueref_args(MCM_property_changed, *t_string);
                                t_obj = t_target -> target;
                                t_target_exists = t_obj.IsValid();
                            }
                            
                            if (t_target_exists
                                && t_properties_changed & kMCPropertyChangedMessageTypeResizeControlStarted)
                            {
                                t_obj -> message_with_valueref_args(MCM_resize_control_started, *t_string);
                                t_obj = t_target -> target;
                                t_target_exists = t_obj.IsValid();
                            }
                                
                            if (t_target_exists
                                && t_properties_changed & kMCPropertyChangedMessageTypeResizeControlEnded)
                            {
                                t_obj -> message_with_valueref_args(MCM_resize_control_ended, *t_string);
                                t_obj = t_target -> target;
                                t_target_exists = t_obj.IsValid();
                            }
                                
                            if (t_target_exists
                                && t_properties_changed & kMCPropertyChangedMessageTypeGradientEditStarted)
                            {
                                t_obj -> message_with_valueref_args(MCM_gradient_edit_started, *t_string);
                                t_obj = t_target -> target;
                                t_target_exists = t_obj.IsValid();
                            }
                                
                            if (t_target_exists
                                && t_properties_changed & kMCPropertyChangedMessageTypeGradientEditEnded)
                            {
                                t_obj -> message_with_valueref_args(MCM_gradient_edit_ended, *t_string);
                                t_obj = t_target -> target;
                                t_target_exists = t_obj.IsValid();
                            }
                            
                            if (!t_target_exists)
                            {
                                t_changed = true;
                            }
								
							t_target = t_target -> next;
						}
					}
					else
						t_listener -> object -> signallistenerswithmessage(t_properties_changed);
				}
			}
			
            t_listener = t_listener -> next;
		}
        s_listeners_locked = false;
        
        if (t_changed)
            prune_object_listeners();
	}
}

void MCInternalObjectListenerGetListeners(MCExecContext& ctxt, MCStringRef*& r_listeners, uindex_t& r_count)
{
    prune_object_listeners();
	
    MCObjectHandle t_current_object;
	t_current_object = ctxt . GetObject() -> GetHandle();
	
	MCObjectListener *t_listener;
	t_listener = s_object_listeners;
    
    MCAutoArray<MCStringRef> t_listeners;
	while(t_listener != nil)
	{
        MCStringRef t_string;

        MCObjectListenerTarget *t_target;
        t_target = nil;
        
        if (t_listener -> object . IsValid())
        {
            MCAutoValueRef t_long_id;
            t_listener -> object -> names(P_LONG_ID, &t_long_id);
        
            for (t_target = t_listener -> targets; t_target != nil; t_target = t_target -> next)
            {
                if (t_target -> target == t_current_object)
                {
                    ctxt . ConvertToString(*t_long_id, t_string);
                    t_listeners . Push(t_string);
                }
            }
        }
        
        t_listener = t_listener -> next;
	}
    t_listeners . Take(r_listeners, r_count);
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
		m_object = new (nothrow) MCChunk(False);
		if (m_object -> parse(sp, False) != PS_NORMAL)
		{
			MCperror -> add(PE_OBJECT_NAME, sp);
			return PS_ERROR;
		}
		return PS_NORMAL;		
	}
	
    void exec_ctxt(MCExecContext &ctxt)
    {
		MCObject *t_object;
		uint4 parid;
        if (!m_object -> getobj(ctxt, t_object, parid, True))
		{
            ctxt . LegacyThrow(EE_NO_MEMORY);
            return;
		}		
		
		MCObjectListener *t_listener;
		t_listener = nil;
		
		MCObjectHandle t_object_handle;
		t_object_handle = t_object -> GetHandle();
		
		for (t_listener = s_object_listeners; t_listener != nil; t_listener = t_listener -> next)
		{
			if (t_listener -> object == t_object_handle)
				break;
		}
			
		if (t_listener == nil)
		{
			if (!MCMemoryCreate(t_listener))
			{
                ctxt . LegacyThrow(EE_NO_MEMORY);
                return;
			}
			t_object -> listen();
			t_listener -> object = t_object_handle;
			t_listener -> targets = nil;
			t_listener -> next = s_object_listeners;
			t_listener -> last_update_time = 0.0;
			s_object_listeners = t_listener;			
		}
		
		MCObjectListenerTarget *t_target;
		t_target = nil;
		
		MCObjectHandle t_target_object;
        t_target_object = ctxt . GetObjectHandle();
		
		for (t_target = t_listener -> targets; t_target != nil; t_target = t_target -> next)
		{
			if (t_target -> target == t_target_object)
				break;
		}
		
		if (t_target == nil)
		{
			if (!MCMemoryCreate(t_target))
			{
                ctxt . LegacyThrow(EE_NO_MEMORY);
                return;
			}
			t_target -> target = t_target_object;
			t_target -> next = t_listener -> targets;
			t_listener -> targets = t_target;						
        }
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
		m_object = new (nothrow) MCChunk(False);
		if (m_object -> parse(sp, False) != PS_NORMAL)
		{
			MCperror -> add(PE_OBJECT_NAME, sp);
			return PS_ERROR;
		}
		return PS_NORMAL;
	}
	
    void exec_ctxt(MCExecContext &ctxt)
	{
		MCObject *t_object;
		uint4 parid;
        if (!m_object -> getobj(ctxt, t_object, parid, True))
        {
            // Was formerly returning ES_NORMAL
            ctxt . IgnoreLastError();
            return;
        }
		
		MCObjectListener *t_listener;
		t_listener = nil;
		
		MCObjectHandle t_object_handle;
		t_object_handle = t_object -> GetHandle();
		
		for (t_listener = s_object_listeners; t_listener != nil; t_listener = t_listener -> next)
		{
			if (t_listener -> object == t_object_handle)
				break;
		}
		
		if (t_listener != nil)
		{
			MCObjectListenerTarget *t_target;
			t_target = nil;
			
			MCObjectHandle t_target_object;
            t_target_object = ctxt . GetObjectHandle();
			
            bool t_changed;
            t_changed = false;
			for (t_target = t_listener -> targets; t_target != nil; t_target = t_target -> next)
			{
				if (t_target -> target == t_target_object)
				{
                    t_target -> target = nil;
                    t_changed = true;
                    break;
				}
			}
            
            if (t_changed)
                prune_object_listeners();
        }
	}
	
private:
	MCChunk *m_object;
};

#endif

////////////////////////////////////////////////////////////////////////////////

extern bool MCS_get_browsers(MCStringRef &r_browsers);

class MCInternalListBrowsers: public MCStatement
{
public:
    Parse_stat parse(MCScriptPoint& sp)
    {
        return PS_NORMAL;
    }

    void exec_ctxt(MCExecContext &ctxt)
    {
        MCAutoStringRef t_browsers;
        if (MCS_get_browsers(&t_browsers))
            ctxt.SetTheResultToValue(*t_browsers);
        else
            ctxt.SetTheResultToEmpty();
    }
};

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
	{ "syntax", "tokenize", class_factory<MCIdeSyntaxTokenize> },
	{ "syntax", "recognize", class_factory<MCIdeSyntaxRecognize> },
	{ "filter", "controls", class_factory<MCIdeFilterControls> },
    { "list", "browsers", class_factory<MCInternalListBrowsers> },

	{ nil, nil, nil }
};

