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

#include "prefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "scriptpt.h"

#include "param.h"
#include "handler.h"
#include "sellst.h"
#include "undolst.h"
#include "chunk.h"
#include "object.h"
#include "mccontrol.h"
#include "mcerror.h"
#include "dispatch.h"
#include "stack.h"
#include "stacklst.h"
#include "aclip.h"
#include "vclip.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "graphic.h"
#include "eps.h"
#include "scrolbar.h"
#include "player.h"
#include "image.h"
#include "field.h"
#include "util.h"
#include "globals.h"
#include "license.h"
#include "cmds.h"
#include "securemode.h"
#include "osspec.h"
#include "redraw.h"
#include "exec.h"
#include "objptr.h"
#include "stacksecurity.h"

#include "graphics_util.h"

MCClone::~MCClone()
{
	delete source;
	delete newname;
}

Parse_stat MCClone::parse(MCScriptPoint &sp)
{
	initpoint(sp);

	// OK-2008-06-23: [[Bug 6590]] - Added [invisible] syntax to allow stacks to be cloned invisibly
	if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_INVISIBLE) == PS_NORMAL)
		visible = False;

	source = new (nothrow) MCChunk(False);
	if (source->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_CLONE_BADCHUNK, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AS) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &newname) != PS_NORMAL)
		{
			MCperror->add
			(PE_CLONE_BADNAME, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

// MW-2004-11-17: New version is too restrictive. Semantics should be:
//   clone a stack - always succeeds
//   clone a card of stack -
//     if target is this stack then only if not locked
//     if target is not this stack then only if keyed and target valid
//   clone an object of a stack - only if not locked
// where a target is valid only if it is not locked and not protected
void MCClone::exec_ctxt(MCExecContext& ctxt)
{
    MCObject *optr = NULL;
    uint4 parid;

    if (!source->getobj(ctxt, optr, parid, True))
    {
        ctxt . LegacyThrow(EE_CLONE_NOTARGET);
        return;
    }
    
    MCAutoStringRef t_new_name;    
    if (!ctxt . EvalOptionalExprAsNullableStringRef(newname, EE_CLONE_BADNAME, &t_new_name))
        return;

	MCInterfaceExecClone(ctxt, optr, *t_new_name, visible == False);
}

///////////////////////////////////////////////////////////////////////////////

MCClipboardCmd::~MCClipboardCmd(void)
{
	deletetargets(&targets);
	delete dest;
}

Parse_stat MCClipboardCmd::parse(MCScriptPoint& sp)
{
	initpoint(sp);
	MCScriptPoint oldsp(sp);
	MCerrorlock++;
	if (gettargets(sp, &targets, False) != PS_NORMAL)
	{
		deletetargets(&targets);
		sp = oldsp;
	}
	MCerrorlock--;
	if (sp.skip_token(SP_FACTOR, TT_TO, PT_TO) == PS_NORMAL)
	{
		dest = new (nothrow) MCChunk(False);
		if (dest->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_COPY_BADDEST, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

void MCClipboardCmd::exec_ctxt(MCExecContext& ctxt)
{
    if (targets == NULL)
	{
		// Implicit form - use current context
		if (iscut())
			MCPasteboardExecCut(ctxt);
		else
			MCPasteboardExecCopy(ctxt);
	}
	else
	{
        // Parse the first chunk before determining if we have a text
        // chunk or not - otherwise things like 'cut tVar' where
        // tVar contains 'line x of field y' do not go through the
        // correct code path.
        MCObjectPtr t_first_object;
        if (!targets -> getobj(ctxt, t_first_object, True))
            return;
        
        if (targets -> istextchunk())
        {
            // Explicit form (1) - text chunk-
            if (targets -> next != NULL)
            {
                ctxt . LegacyThrow(EE_CLIPBOARD_BADMIX);
                return;
            }
            
            MCObjectChunkPtr t_obj_chunk;

            if (!targets -> evalobjectchunk(ctxt, true, false, t_obj_chunk))
            {
                ctxt . LegacyThrow(EE_CLIPBOARD_BADTEXT);
                return;
            }
            
            if (iscut())
                MCPasteboardExecCutTextToClipboard(ctxt, t_obj_chunk);
            else
                MCPasteboardExecCopyTextToClipboard(ctxt, t_obj_chunk);

            MCValueRelease(t_obj_chunk . mark . text);
        }
        else
        {
            // Explicit form (2)/(3) - object chunks
            
            MCChunk *chunkptr = targets;
            MCObjectPtr t_object;
            MCAutoArray<MCObjectPtr> t_objects;
            
            while (chunkptr != NULL)
            {
                if (chunkptr -> istextchunk())
                {
                    ctxt . LegacyThrow(EE_CLIPBOARD_BADMIX);
                    return;
                }
                
                if (!chunkptr -> getobj(ctxt, t_object, True))
                {
                    ctxt . LegacyThrow(EE_CLIPBOARD_BADOBJ);
                    return;
                }
                
                if (!t_objects . Push(t_object))
                {
                    ctxt . LegacyThrow(EE_NO_MEMORY);
                    break;
                }
                
                chunkptr = chunkptr->next;
            }
            
            // Calculate destination object (if applicable)
            MCObjectPtr t_dst_object;
            if (dest != NULL)
            {
                if (!dest -> getobj(ctxt, t_dst_object, True))
                {
                    ctxt  . LegacyThrow(EE_CLIPBOARD_BADOBJ);
                    return;
                }
            }
            
            if (t_objects . Size() > 0)
            {
                if (dest != NULL)
                {
                    if (iscut())
                        MCInterfaceExecCutObjectsToContainer(ctxt, t_objects . Ptr(), t_objects . Size(), t_dst_object);
                    else
                        MCInterfaceExecCopyObjectsToContainer(ctxt, t_objects . Ptr(), t_objects . Size(), t_dst_object);
                }
                else
                {
                    if (iscut())
                        MCPasteboardExecCutObjectsToClipboard(ctxt, t_objects . Ptr(), t_objects . Size());
                    else
                        MCPasteboardExecCopyObjectsToClipboard(ctxt, t_objects . Ptr(), t_objects . Size());
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Primary Verb:
//    copy
//
//  Synonyms:
//    <none>
//
//
//  Syntactic Form 1: copy selection to clipboard
//    copy
//
//  Semantics:
//    Copies the current selection to the clipboard. The current selection is
//    determined by the current active object. The currently active object is
//      a) MCactivefield (the field with insertion point) if not NULL
//      b) MCactiveimage (the image being edited) if not NULL
//      c) MCselected (the list of currently selected objects)
//
//  Result:
//    <none>
//
//  Exceptions:
//    <none>
//
//
//  Syntactic Form 2: copy objects to clipboard
//    copy { <Chunk> , and }
//
//  Semantics:
//    Copies the given chunks to the clipboard. The chunks can be:
//      - an object
//      - a text chunk of a field
//      - the clickField
//      - the selectedField
//      - the mouseControl
//      - the focusedObject
//      - the selectedImage
//
//    An object can only be copied from non-locked stacks.
//
//  Result:
//    empty if successful
//    "can't cut object (stack is password protected)" if one of the objects
//      resides on a locked stack.
//
//  Exceptions:
//    EE_CHUNK_CANTFINDOBJECT if one of the chunks cannot be found
//    EE_CHUNK_BADCONTAINER if one of the chunks is not of an allowed type
//
//
//  Syntactic Form 3: copy objects to container
//    copy { <Chunk> , and } to <Chunk>
//
//  Semantics:
//    Copies the given object chunks to the target container. Compatibility
//    of the source and target is checked - AudioClips, VideoClips and Cards
//    can only be copied to a Stack. All other object types can only be
//    copied to a Stack, Card or Group.
//
//    An object can only be copied from non-locked stacks.
//
//    If no exception occurs, the 'it' variable is filled in with the long id
//    of the last object copied.
//
//  Result:
//    empty if successful
//    "can't cut object (stack is password protected)" if one of the objects
//      resides on a locked stack.
//
//  Exceptions:
//    EE_COPY_NODEST if the target object could not be found
//    EE_COPY_NOOBJ if a source object could not be found
//    EE_COPY_BADDEST if a source object is not compatible with the target
//    EE_COPY_BADOBJ if the source object is not a type that can be copied
//    EE_COPY_BADMIX if the collection of source objects is invalid
//
//  Description:
//    This class implements the 'copy' command.
//
//    It has the following syntactic forms:
//

bool MCCopyCmd::iscut(void) const
{
	return false;
}

MCCreate::~MCCreate()
{
    delete kind;
    delete newname;
	delete file;
	delete container;
}

Parse_stat MCCreate::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_INVISIBLE) == PS_NORMAL)
		visible = False;
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_CREATE_NOTYPE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_FACTOR, te) != PS_NORMAL)
	{
		MCperror->add
		(PE_CREATE_NOTTYPE, sp);
		return PS_ERROR;
	}
	if (te->type == TT_CHUNK)
	{
		switch (te->which)
		{
		case CT_STACK:
		case CT_BACKGROUND:
		case CT_CARD:
		case CT_FIELD:
		case CT_BUTTON:
		case CT_MENU:
		case CT_IMAGE:
		case CT_GROUP:
		case CT_SCROLLBAR:
		case CT_PLAYER:
		case CT_GRAPHIC:
		case CT_EPS:
            case CT_WIDGET:
			otype = (Chunk_term)te->which;
			break;
		case CT_ALIAS:
			alias = True;
			break;
		default:
			MCperror->add
			(PE_CREATE_BADTYPE, sp);
			return PS_ERROR;
		}
	}
	else if (te -> type == TT_PROPERTY && te -> which == P_SCRIPT)
    {
        // Accept 'create script only stack ... [ <name> ]'
        if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_ONLY) != PS_NORMAL ||
            sp . skip_token(SP_FACTOR, TT_CHUNK, CT_STACK) != PS_NORMAL)
        {
            MCperror -> add(PE_CREATE_BADTYPE, sp);
            return PS_ERROR;
        }
        
        script_only_stack = True;
    }
    else if (te->type == TT_PROPERTY && te->which == P_DIRECTORY)
        directory = True;
	
    MCerrorlock++;
	if (sp.parseexp(False, True, &newname) != PS_NORMAL)
	{
		if (directory || alias)
		{
			MCerrorlock--;
			MCperror->add
			(PE_CREATE_NONAME, sp);
			return PS_ERROR;
		}
		delete newname;
		newname = NULL;
	}
	MCerrorlock--;
	if (alias)
	{
		if (sp.skip_token(SP_FACTOR, TT_TO, PT_TO) != PS_NORMAL)
		{
			MCperror->add
			(PE_CREATE_NOFILENAME, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_THERE, TT_UNDEFINED, TM_FILE) != PS_NORMAL)
			sp.skip_token(SP_THERE, TT_UNDEFINED, TM_DIRECTORY);
		if (sp.parseexp(False, True, &file) != PS_NORMAL)
		{
			MCperror->add
			(PE_CREATE_BADFILENAME, sp);
			return PS_ERROR;
		}
	}
	else
    {
        // AL-2015-05-21: [[ Bug 15405 ]] Allow 'create widget as ... in group ...'
        if (otype == CT_WIDGET)
        {
            if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AS) != PS_NORMAL)
            {
                MCperror -> add(PE_CREATE_BADTYPE, sp);
                return PS_ERROR;
            }
            if (sp.parseexp(False, True, &kind) != PS_NORMAL)
            {
                MCperror -> add(PE_CREATE_BADTYPE, sp);
                return PS_ERROR;
            }
        }
        
        if (sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL ||
            (otype == CT_STACK && sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL))
        {
            if (!script_only_stack)
            {
                container = new (nothrow) MCChunk(False);
                if (container->parse(sp, False) != PS_NORMAL)
                {
                    MCperror->add
                    (PE_CREATE_BADBGORCARD, sp);
                    return PS_ERROR;
                }
            }
            else
            {
                MCperror -> add(PE_CREATE_BADTYPE, sp);
                return PS_ERROR;
            }
        }
    }
	return PS_NORMAL;
}

void MCCreate::exec_ctxt(MCExecContext& ctxt)
{
    if (directory)
	{
        MCAutoStringRef t_filename;
        if (!ctxt . EvalExprAsStringRef(newname, EE_CREATE_BADEXP, &t_filename))
            return;
		MCFilesExecCreateFolder(ctxt, *t_filename);
	}
	else if (alias)
	{
        MCAutoStringRef t_alias_name;
        if (!ctxt . EvalExprAsStringRef(newname, EE_CREATE_BADFILEEXP, &t_alias_name))
            return;
		
		MCAutoStringRef t_target_filename;
        if (!ctxt . EvalExprAsStringRef(file, EE_CREATE_BADEXP, &t_target_filename))
            return;
        
		MCFilesExecCreateAlias(ctxt, *t_target_filename, *t_alias_name);
	}
    else
	{
		MCAutoStringRef t_new_name;
        if (!ctxt . EvalOptionalExprAsNullableStringRef(newname, EE_CREATE_BADEXP, &t_new_name))
            return;

        if (script_only_stack)
            MCInterfaceExecCreateScriptOnlyStack(ctxt, *t_new_name);
        else
        {
            switch (otype)
            {
            case CT_STACK:
            {
                MCObject *tptr = nil;
                if (container != nil)
                {
                    uint4 parid;

                    if (!container -> getobj(ctxt, tptr, parid, True)
                            || (tptr->gettype() != CT_GROUP && tptr->gettype() != CT_STACK))
                    {
                        ctxt . LegacyThrow(EE_CREATE_BADBGORCARD);
                        return;
                    }
                }
                if (tptr != nil && tptr->gettype() == CT_GROUP)
                    MCInterfaceExecCreateStackWithGroup(ctxt, (MCGroup *)tptr, *t_new_name, visible == False);
                else
                    MCInterfaceExecCreateStack(ctxt, (MCStack *)tptr, *t_new_name, visible == False);
            }
                break;
            case CT_CARD:
            {
                MCObject *parent = nil;
                if (container != nil)
                {
                    uint32_t parid;
                  
                    if (!container->getobj(ctxt, parent, parid, True) || parent->gettype() != CT_STACK)
                    {
                        ctxt . LegacyThrow(EE_CREATE_BADBGORCARD);
                        return;
                    }
                }
                MCInterfaceExecCreateCard(ctxt, *t_new_name, static_cast<MCStack *>(parent), visible==False);
            }
                break;
            case CT_WIDGET:
            {
                MCNewAutoNameRef t_kind;
                MCObject *parent = nil;
                kind->eval_typed(ctxt, kMCExecValueTypeNameRef, &(&t_kind));
                if (ctxt.HasError())
                {
                    ctxt . LegacyThrow(EE_CREATE_BADEXP);
                    return;
                }
                if (container != nil)
                {
                    uint32_t parid;
                    
                    if (!container->getobj(ctxt, parent, parid, True) 
							|| (parent->gettype() != CT_GROUP && parent->gettype() != CT_CARD))
                    {
                        ctxt . LegacyThrow(EE_CREATE_BADBGORCARD);
                        return;
                    }
                }
                MCInterfaceExecCreateWidget(ctxt, *t_new_name, *t_kind, parent, visible == False);
                break;
            }
            default:
            {
                MCObject *parent = nil;
                if (container != nil)
                {
                    uint4 parid;

                    if (!container->getobj(ctxt, parent, parid, True)
                            || (parent->gettype() != CT_GROUP && parent->gettype() != CT_CARD) )
                    {
                        ctxt . LegacyThrow(EE_CREATE_BADBGORCARD);
                        return;
                    }
                }
                MCInterfaceExecCreateControl(ctxt, *t_new_name, otype, parent, visible == False);
            }
                break;
            }
        }
	}
}

MCCustomProp::~MCCustomProp()
{
	delete prop;
	delete target;
}

Parse_stat MCCustomProp::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	Symbol_type type;
	const LT *te;
	sp.next(type);
	if (type != ST_ID || sp.lookup(SP_FACTOR, te) == PS_NORMAL)
	{
		MCperror->add
		(PE_DEFINE_INVALIDNAME, sp);
		return PS_ERROR;
	}
	sp.skip_token(SP_FACTOR, TT_OF);
	target = new (nothrow) MCChunk(False);
	if (target->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_DEFINE_BADOBJECT, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCCustomProp::exec_ctxt(MCExecContext &ctxt)
{
}

///////////////////////////////////////////////////////////////////////////////
//
//  Primary Verb:
//    cut
//
//  Synonyms:
//    <none>
//
//
//  Syntactic Form 1: cut selection to clipboard
//    cut
//
//  Semantics:
//    Cuts the current selection to the clipboard. The current selection is
//    determined by the current active object. The currently active object is
//      a) MCactivefield (the field with insertion point) if not NULL
//      b) MCactiveimage (the image being edited) if not NULL
//      c) MCselected (the list of currently selected objects)
//
//  Result:
//    <none>
//
//  Exceptions:
//    <none>
//
//
//  Syntactic Form 2: cut objects to clipboard
//    cut { <Chunk> , and }
//
//  Semantics:
//    Copies the given chunks to the clipboard. The chunks can be:
//      - an object
//      - a text chunk of a field
//      - the clickField
//      - the selectedField
//      - the mouseControl
//      - the focusedObject
//      - the selectedImage
//
//  Result:
//    empty if successful
//    "can't cut object (stack is password protected)" if one of the object's
//      couldn't be copied due to access restrictions of its owning stack
//
//  Exceptions:
//    EE_CHUNK_CANTFINDOBJECT if one of the chunks cannot be found
//    EE_CHUNK_BADCONTAINER if one of the chunks is not of an allowed type
//

bool MCCutCmd::iscut(void) const
{
	return true;
}

MCDelete::~MCDelete()
{
	delete file;
	deletetargets(&targets);
	delete var;
}

Parse_stat MCDelete::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	Boolean needfile = False;
	if (sp.skip_token(SP_HANDLER, TT_VARIABLE) == PS_NORMAL)
	{
		// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
		//   execution outwith a handler.
		Symbol_type type;
		if (sp.next(type) != PS_NORMAL
		        || sp.findvar(sp.gettoken_nameref(), &var) != PS_NORMAL)
		{
			MCperror->add(PE_DELETE_BADVAREXP, sp);
			return PS_ERROR;
		}
		var->parsearray(sp);
	}
	else if (sp.skip_token(SP_START, TT_UNDEFINED, SC_SESSION) == PS_NORMAL)
	{
		session = true;
	}
	else
	{
		if (sp.skip_token(SP_THERE, TT_UNDEFINED, TM_FILE) == PS_NORMAL)
			needfile = True;
		else if (sp.skip_token(SP_THERE, TT_UNDEFINED, TM_DIRECTORY) == PS_NORMAL)
		{
			needfile = True;
			directory = True;
		}
		else if (sp.skip_token(SP_THERE, TT_UNDEFINED, TM_URL) == PS_NORMAL)
		{
			needfile = True;
			url = True;
		}
		if (needfile)
		{
			if (sp.parseexp(False, True, &file) != PS_NORMAL)
			{
				MCperror->add
				(PE_DELETE_BADFILEEXP, sp);
				return PS_ERROR;
			}
		}
		else
		{
			MCScriptPoint oldsp(sp);
			MCerrorlock++;
			if (gettargets(sp, &targets, True) != PS_NORMAL)
			{
				deletetargets(&targets);
				sp = oldsp;
			}
			MCerrorlock--;
		}
	}
	return PS_NORMAL;
}

bool MCServerDeleteSession();
void MCDelete::exec_ctxt(MCExecContext& ctxt)
{
    if (var != NULL)
		MCEngineExecDeleteVariable(ctxt, var);
	else if (file != NULL)
	{
        MCAutoStringRef t_target;
        if (!ctxt . EvalExprAsStringRef(file, EE_DELETE_BADFILEEXP, &t_target))
            return;
        
		if (url)
			MCNetworkExecDeleteUrl(ctxt, *t_target);
		else
			MCFilesExecDeleteFile(ctxt, *t_target);
	}
    else if (targets != NULL && targets -> issubstringchunk())
	{
		MCAutoArray<MCVariableChunkPtr> t_chunks;
        bool t_return;
        t_return = false;
        for(MCChunk *t_chunk = targets; t_chunk != nil && !t_return; t_chunk = t_chunk -> next)
		{
			if (!t_chunk -> issubstringchunk())
			{
                ctxt . LegacyThrow(EE_CLIPBOARD_BADMIX);
                t_return = true;
                break;
			}
            
			MCVariableChunkPtr t_var_chunk;

            if (!t_chunk -> evalvarchunk(ctxt, true, false, t_var_chunk))
            {
                t_return = true;
                break;
            }
                        
			if (!t_chunks . Push(t_var_chunk))
			{
                ctxt . LegacyThrow(EE_NO_MEMORY);
                MCValueRelease(t_var_chunk . mark . text);
				break;
			}
		}
        
        if (!t_return)
            MCEngineExecDeleteVariableChunks(ctxt, t_chunks . Ptr(), t_chunks . Size());

        // Release the text stored from evalvarchunk
        for (uindex_t i = 0; i < t_chunks . Size(); ++i)
        {
            MCValueRelease(t_chunks[i] . mark . text);
        }
	}
    else if (targets != nil)
	{
        // Parse the first chunk before determining if we have a text
        // chunk or not - otherwise things like 'delete tVar' where
        // tVar contains 'line x of field y' do not go through the
        // correct code path.
        MCObjectPtr t_first_object;
        if (!targets -> getobj(ctxt, t_first_object, True))
            return;
        
        if (targets -> istextchunk())
        {
            MCAutoArray<MCObjectChunkPtr> t_chunks;
            bool t_return;
            t_return = false;
            for(MCChunk *t_chunk = targets; t_chunk != nil && !t_return; t_chunk = t_chunk -> next)
            {
                if (!t_chunk -> istextchunk())
                {
                    ctxt . LegacyThrow(EE_CLIPBOARD_BADMIX);
                    t_return = true;
                    break;
                }
                
                MCObjectChunkPtr t_obj_chunk;
                if (!t_chunk -> evalobjectchunk(ctxt, true, false, t_obj_chunk))
                {
                    t_return = true;
                    break;
                }
                
                if (!t_chunks . Push(t_obj_chunk))
                {
                    ctxt . LegacyThrow(EE_NO_MEMORY);
                    MCValueRelease(t_obj_chunk . mark . text);
                    break;
                }
            }
            
            if (!t_return)
                MCInterfaceExecDeleteObjectChunks(ctxt, t_chunks . Ptr(), t_chunks . Size());
            
            for (uindex_t i = 0; i < t_chunks . Size(); ++i)
                MCValueRelease(t_chunks[i] . mark . text);
        }
        else
        {
            MCAutoArray<MCObjectPtr> t_objects;
            for(MCChunk *t_chunk = targets; t_chunk != nil; t_chunk = t_chunk -> next)
            {
                MCObjectPtr t_object;
                if (!t_chunk -> getobj(ctxt, t_object, True))
                    return;
                
                if (!t_objects . Push(t_object))
                {
                    ctxt . LegacyThrow(EE_NO_MEMORY);
                    break;
                }
            }
            
            MCInterfaceExecDeleteObjects(ctxt, t_objects . Ptr(), t_objects . Size());
        }
    }
    else if (session)
	{
#ifdef _SERVER
		MCServerExecDeleteSession(ctxt);
#else
        ctxt . LegacyThrow(EE_SESSION_BADCONTEXT);
		return;
#endif
	}
	else
		MCInterfaceExecDelete(ctxt);
}

MCChangeProp::~MCChangeProp()
{
	deletetargets(&targets);
}

Parse_stat MCChangeProp::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (gettargets(sp, &targets, False) != PS_NORMAL || targets == NULL)
	{
		MCperror->add
		(PE_DISABLE_BADCHUNK, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCChangeProp::exec_ctxt(MCExecContext &ctxt)
{
	if (targets != NULL)
	{
		if (targets -> istextchunk())
		{
			MCObjectChunkPtr t_obj_chunk;
            if (!targets -> evalobjectchunk(ctxt, false, true, t_obj_chunk))
			{
                ctxt . LegacyThrow(EE_DISABLE_NOOBJ);
                return;
			}
				
			if (t_obj_chunk . object -> gettype() != CT_BUTTON)
			{
                ctxt . LegacyThrow(EE_DISABLE_NOOBJ);
                MCValueRelease(t_obj_chunk . mark . text);
                return;
			}
				
			switch (prop) {
			case P_DISABLED:
				if (value)
					MCInterfaceExecDisableChunkOfButton(ctxt, t_obj_chunk);
				else
					MCInterfaceExecEnableChunkOfButton(ctxt, t_obj_chunk);
				break;
			case P_HILITE:
				if (value)
					MCInterfaceExecHiliteChunkOfButton(ctxt, t_obj_chunk);
				else
					MCInterfaceExecUnhiliteChunkOfButton(ctxt, t_obj_chunk);
				break;
			default:
				break;
			}
            MCValueRelease(t_obj_chunk . mark . text);
		}
		else
		{
			MCObjectPtr t_obj;
            if (!targets -> getobj(ctxt, t_obj, True))
			{
                ctxt . LegacyThrow(EE_DISABLE_NOOBJ);
                return;
			}
			
			switch (prop) {
			case P_DISABLED:
				if (value)
					MCInterfaceExecDisableObject(ctxt, t_obj);
				else
					MCInterfaceExecEnableObject(ctxt, t_obj);
				break;
			case P_HILITE:
				if (value)
					MCInterfaceExecHiliteObject(ctxt, t_obj);
				else
					MCInterfaceExecUnhiliteObject(ctxt, t_obj);
				break;
			default:
				break;
			}
		}
    }
}

MCFlip::~MCFlip()
{
	delete image;
}

Parse_stat MCFlip::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;
	initpoint(sp);
	
	// Allow flipping the portion currently selected with the Select paint tool
	// In this case an image is not specified, i.e. "flip vertical"
	if (sp.next(type) == PS_NORMAL && sp.lookup(SP_FLIP, te) == PS_NORMAL)
	{
		direction = (Flip_dir)te->which;
		return PS_NORMAL;
	}
	
	sp.backup();
	
	// PM-2015-08-07: [[ Bug 1751 ]] Allow more flexible parsing: 'flip the selobj ..', 'flip last img..' etc
	
	// Parse an arbitrary chunk. If it does not resolve as an image, a runtime error will occur in MCFlip::exec
	image = new (nothrow) MCChunk(False);
	if (image->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add(PE_FLIP_BADIMAGE, sp);
		return PS_ERROR;
	}
	
	
	if (sp.next(type) != PS_NORMAL || sp.lookup(SP_FLIP, te) != PS_NORMAL)
	{
		MCperror->add
		(PE_FLIP_NODIR, sp);
		return PS_ERROR;
	}
	direction = (Flip_dir)te->which;
	return PS_NORMAL;
}

// MW-2007-09-22: [[ Bug 5083 ]] Ensure if we flip a targetted image, we restore
//   back to current tool.
void MCFlip::exec_ctxt(MCExecContext& ctxt)
{
    if (image != NULL)
	{
		MCObject *optr;
		uint4 parid;

        if (!image->getobj(ctxt, optr, parid, True))
        {
            ctxt . LegacyThrow(EE_FLIP_NOIMAGE);
            return;
        }
		
		if (optr->gettype() != CT_IMAGE)
		{
            ctxt . LegacyThrow(EE_FLIP_NOTIMAGE);
			return;
		}
		MCImage *iptr = (MCImage *)optr;
        MCGraphicsExecFlipImage(ctxt, iptr, direction == FL_HORIZONTAL);
    }

    if (MCactiveimage)
        MCGraphicsExecFlipSelection(ctxt, direction == FL_HORIZONTAL);
}

MCGrab::~MCGrab()
{
	delete control;
}

Parse_stat MCGrab::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (gettargets(sp, &control, False) != PS_NORMAL || control == NULL)
	{
		MCperror->add
		(PE_GRAB_BADCHUNK, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCGrab::exec_ctxt(MCExecContext& ctxt)
{
	MCObject *optr;
	uint4 parid;

    if (!control->getobj(ctxt, optr, parid, True)
            || optr->gettype() < CT_GROUP)
	{
        ctxt . LegacyThrow(EE_GRAB_NOOBJ);
		return;
	}
	MCInterfaceExecGrab(ctxt, static_cast<MCControl *>(optr));
}

MCLaunch::~MCLaunch()
{
	delete doc;
	delete app;
	delete widget;
}

// Syntax should be:
//   launch <application>
//     startup the given application
//   launch <document> with <application>
//     startup the given application passing the given filename as initial document
//   launch document <document>
//     launch the given document
//   launch url <url>
//     launch the given url
//
Parse_stat MCLaunch::parse(MCScriptPoint &sp)
{
	bool t_is_document;
	bool t_is_url;
	t_is_document = false;
	t_is_url = false;

	initpoint(sp);

	if (sp . skip_token(SP_FACTOR, TT_CHUNK, CT_DOCUMENT) == PS_NORMAL)
		t_is_document = true;
	else if (sp . skip_token(SP_FACTOR, TT_CHUNK, CT_URL) == PS_NORMAL)
		t_is_url = true;

	if (sp.parseexp(False, True, &app) != PS_NORMAL)
	{
		MCperror->add(PE_LAUNCH_BADAPPEXP, sp);
		return PS_ERROR;
	}

	if (!t_is_document && !t_is_url && sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		doc = app;
		app = NULL;
		if (sp.parseexp(False, True, &app) != PS_NORMAL)
		{
			MCperror->add(PE_LAUNCH_BADAPPEXP, sp);
			return PS_ERROR;
		}
	}
	else if (t_is_document || t_is_url)
	{
		doc = app;
		app = NULL;
		as_url = t_is_url;
	}

	if (t_is_url && sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL)
	{
		widget = new (nothrow) MCChunk(False);
		if (widget->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_LAUNCH_BADWIDGETEXP, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

void MCLaunch::exec_ctxt(MCExecContext& ctxt)
{
    MCNewAutoNameRef t_app;
		
    if (!ctxt. EvalOptionalExprAsNullableNameRef(app, EE_LAUNCH_BADAPPEXP, &t_app))
        return;
	
    MCAutoStringRef t_document;
	
    if (!ctxt . EvalOptionalExprAsNullableStringRef(doc, EE_LAUNCH_BADAPPEXP, &t_document))
        return;
    
	if (app != NULL)
		MCFilesExecLaunchApp(ctxt, *t_app, *t_document);
	else if (doc != NULL)
	{
		if (as_url)
		{
			if (widget != nil)
			{
				MCObject *t_object;
				uint32_t t_parid;
				
				if (!widget->getobj(ctxt, t_object, t_parid, True) || t_object->gettype() != CT_WIDGET)
				{
					ctxt.LegacyThrow(EE_LAUNCH_BADWIDGETEXP);
					return;
				}
				
				MCInterfaceExecLaunchUrlInWidget(ctxt, *t_document, (MCWidget*)t_object);
			}
			else
				MCFilesExecLaunchUrl(ctxt, *t_document);
		}
		else
			MCFilesExecLaunchDocument(ctxt, *t_document);
	}
}

MCLoad::~MCLoad()
{
	delete url;
	delete message;
}

Parse_stat MCLoad::parse(MCScriptPoint &sp)
{
	initpoint(sp);
    
    if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_EXTENSION) == PS_NORMAL)
	{
        is_extension = true;
		
        if (sp.skip_token(SP_FACTOR, TT_FROM) != PS_NORMAL)
        {
            MCperror->add(PE_LOAD_NOFROM, sp);
            return PS_ERROR;
        }
        
        // AL-2015-11-06: [[ Load Extension From Var ]] Allow loading an extension from data in a variable
        if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_DATA) == PS_NORMAL)
        {
            from_data = true;
        }
        else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_FILE) != PS_NORMAL)
        {
            MCperror->add(PE_LOAD_NOFILE, sp);
            return PS_ERROR;
        }
        
		if (sp.parseexp(False, True, &url) != PS_NORMAL)
		{
			MCperror->add(PE_LOAD_BADEXTENSION, sp);
			return PS_ERROR;
		}
		
		if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		{
			has_resource_path = true;
			
			if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_RESOURCE) != PS_NORMAL)
			{
				MCperror->add(PE_LOAD_NORESOURCE, sp);
				return PS_ERROR;
			}
			
			if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_PATH) != PS_NORMAL)
			{
				MCperror->add(PE_LOAD_NOPATH, sp);
				return PS_ERROR;
			}
			
			if (sp.parseexp(False, True, &message) != PS_NORMAL)
			{
				MCperror->add(PE_LOAD_BADRESOURCEPATH, sp);
				return PS_ERROR;
			}
		}
	}
    else
	{
        sp.skip_token(SP_FACTOR, TT_CHUNK, CT_URL);
		
		if (sp.parseexp(False, True, &url) != PS_NORMAL)
		{
			MCperror->add(PE_LOAD_BADURLEXP, sp);
			return PS_ERROR;
		}
		
		if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		{
			sp.skip_token(SP_SUGAR, TT_CHUNK, CT_UNDEFINED);
			if (sp.parseexp(False, True, &message) != PS_NORMAL)
			{
				MCperror->add(PE_LOAD_BADMESSAGEEXP, sp);
				return PS_ERROR;
			}
		}
	}

	return PS_NORMAL;
}

void MCLoad::exec_ctxt(MCExecContext& ctxt)
{
    
	if (is_extension)
	{
		MCAutoStringRef t_resource_path;
        if (has_resource_path && !ctxt . EvalExprAsStringRef(message, EE_LOAD_BADRESOURCEPATH, &t_resource_path))
            return;
        
        // AL-2015-11-06: [[ Load Extension From Var ]] Allow loading an extension from data in a variable
        if (from_data)
        {
            MCAutoDataRef t_data;
            
            if (!ctxt . EvalExprAsDataRef(url, EE_LOAD_BADEXTENSION, &t_data))
                return;
		
            MCEngineLoadExtensionFromData(ctxt, *t_data, *t_resource_path);
        }
        else
        {
            MCAutoStringRef t_filename;
            
            if (!ctxt . EvalExprAsStringRef(url, EE_LOAD_BADEXTENSION, &t_filename))
                return; 
		
            MCEngineExecLoadExtension(ctxt, *t_filename, *t_resource_path);
        }
	}
	else
    {
        MCNewAutoNameRef t_message;
        
        if (!ctxt . EvalOptionalExprAsNameRef(message, kMCEmptyName, EE_LOAD_BADMESSAGEEXP, &t_message))
                return;
        
        MCAutoStringRef t_url;
        if (!ctxt . EvalExprAsStringRef(url, EE_LOAD_BADURLEXP, &t_url))
            return;    
        
        MCNetworkExecLoadUrl(ctxt, *t_url, *t_message);
    }
}

MCUnload::~MCUnload()
{
	delete url;
}

Parse_stat MCUnload::parse(MCScriptPoint &sp)
{
	initpoint(sp);
    
	if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_EXTENSION) == PS_NORMAL)
		is_extension = true;
	else
		sp.skip_token(SP_FACTOR, TT_CHUNK, CT_URL);

	if (sp.parseexp(False, True, &url) != PS_NORMAL)
	{
		MCperror->add(PE_UNLOAD_BADURLEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCUnload::exec_ctxt(MCExecContext &ctxt)
{
    
    MCAutoStringRef t_url;
    if (!ctxt . EvalExprAsStringRef(url, EE_LOAD_BADURLEXP, &t_url))
        return;
    
	if (is_extension)
		MCEngineExecUnloadExtension(ctxt, *t_url);
	else
		MCNetworkExecUnloadUrl(ctxt, *t_url);
}

MCPost::~MCPost()
{
	delete source;
	delete dest;
}

Parse_stat MCPost::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add
		(PE_POST_BADSOURCEEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_TO) != PS_NORMAL)
	{
		MCperror->add
		(PE_POST_NOTO, sp);
		return PS_ERROR;
	}
	sp.skip_token(SP_FACTOR, TT_CHUNK, CT_URL);
	if (sp.parseexp(False, True, &dest) != PS_NORMAL)
	{
		MCperror->add
		(PE_POST_BADDESTEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCPost::exec_ctxt(MCExecContext &ctxt)
{
\
    MCAutoValueRef t_data;
    if (!ctxt . EvalExprAsValueRef(source, EE_POST_BADSOURCEEXP, &t_data))
        return;
    
    MCAutoStringRef t_url;
    if (!ctxt . EvalExprAsStringRef(dest, EE_POST_BADDESTEXP, &t_url))
        return;

    MCNetworkExecPostToUrl(ctxt, *t_data, *t_url);
}

MCMakeGroup::~MCMakeGroup()
{
	deletetargets(&targets);
}

Parse_stat MCMakeGroup::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	MCScriptPoint oldsp(sp);
	MCerrorlock++;
	if (gettargets(sp, &targets, False) != PS_NORMAL)
	{
		deletetargets(&targets);
		sp = oldsp;
	}
	MCerrorlock--;
	return PS_NORMAL;
}

void MCMakeGroup::exec_ctxt(MCExecContext& ctxt)
{
    if (targets != NULL)
	{
		MCAutoArray<MCObjectPtr> t_objects;
		for(MCChunk *t_chunk = targets; t_chunk != nil; t_chunk = t_chunk -> next)
		{
			MCObjectPtr t_object;
			if (!t_chunk -> getobj(ctxt, t_object, True) ||
			    !MCChunkTermIsControl(t_object . object -> gettype()))
            {
                ctxt .Throw();
				return;
            }
            
			if (!t_objects . Push(t_object))
			{
                ctxt . LegacyThrow(EE_NO_MEMORY);
				break;
			}
		}
		MCInterfaceExecGroupControls(ctxt, t_objects . Ptr(), t_objects . Size());
	}
    else
		MCInterfaceExecGroupSelection(ctxt);
}

MCPasteCmd::~MCPasteCmd()
{
}

Parse_stat MCPasteCmd::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	return PS_NORMAL;
}

void MCPasteCmd::exec_ctxt(MCExecContext& ctxt)
{
    MCPasteboardExecPaste(ctxt);
}

MCPlace::~MCPlace()
{
	delete group;
	delete card;
}

Parse_stat MCPlace::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	group = new (nothrow) MCChunk(False);
	if (group->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_PLACE_BADBACKGROUND, sp);
		return PS_ERROR;
	}
	while (sp.skip_token(SP_FACTOR, TT_PREP) == PS_NORMAL)
		;
	card = new (nothrow) MCChunk(False);
	if (card->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_PLACE_BADCARD, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCPlace::exec_ctxt(MCExecContext& ctxt)
{
    MCObject *gptr;
	uint4 parid;
	if (!group->getobj(ctxt, gptr, parid, True))
    {
        ctxt . LegacyThrow(EE_PLACE_NOBACKGROUND);
        return;
    }
	// MW-2008-03-31: [[ Bug 6281 ]] A little too draconian here - it is possible
	//   for a parent of a placeable group to be either a card or a stack.
    
	if (gptr->gettype() != CT_GROUP)
	{
		ctxt . LegacyThrow(EE_PLACE_NOTABACKGROUND);
		return;
	}
    MCObject *optr;
	
    if (!card->getobj(ctxt, optr, parid, True))
    {
        ctxt . LegacyThrow(EE_PLACE_NOCARD);
        return;
    }
    
    
	if (optr->gettype() != CT_CARD)
	{
		ctxt . LegacyThrow(EE_PLACE_NOTACARD);
		return;
	}
    
	MCCard *cptr = (MCCard *)optr;
	MCInterfaceExecPlaceGroupOnCard(ctxt, gptr, cptr);
}

MCRecord::~MCRecord()
{
	delete file;
}

Parse_stat MCRecord::parse(MCScriptPoint &sp)
{
	initpoint(sp);
    
    if (sp.skip_token(SP_RECORD, TT_UNDEFINED, RC_PAUSE) == PS_NORMAL)
		pause = True;
	else if (sp.skip_token(SP_RECORD, TT_UNDEFINED, RC_RESUME) == PS_NORMAL)
		pause = False;
    else
    {
        sp.skip_token(SP_RECORD, TT_UNDEFINED, RC_SOUND); //skip for sc compat
        sp.skip_token(SP_THERE, TT_UNDEFINED, TM_FILE); //always file in mc
        if (sp.parseexp(False, True, &file) != PS_NORMAL)
        {
            MCperror->add(PE_RECORD_BADFILEEXP, sp);
            return PS_ERROR;
        }
    }
	return PS_NORMAL;
}

void MCRecord::exec_ctxt(MCExecContext &ctxt)
{
    if (file != nil)
    {
        MCAutoStringRef t_filename;
        if (!ctxt . EvalExprAsStringRef(file, EE_RECORD_BADFILE, &t_filename))
            return;
    
        MCMultimediaExecRecord(ctxt, *t_filename);
    }
    else
    {
        if (pause)
            MCMultimediaExecRecordPause(ctxt);
        else
            MCMultimediaExecRecordResume(ctxt);
    }
}

void MCRedo::exec_ctxt(MCExecContext &)
{
}

MCRemove::~MCRemove()
{
	delete target;
	delete card;
}

Parse_stat MCRemove::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;
	initpoint(sp);

	sp.skip_token(SP_FACTOR, TT_THE);
	if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_SCRIPT) == PS_NORMAL)
	{
		sp.skip_token(SP_FACTOR, TT_OF);
		target = new (nothrow) MCChunk(False);
		if (target->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_REMOVE_BADOBJECT, sp);
			return PS_ERROR;
		}
		script = True;
	}
	else
		if (sp.skip_token(SP_MARK, TT_UNDEFINED, MC_ALL) == PS_NORMAL)
		{
			sp.next(type);
			all = True;
		}
	if (script || all)
	{
		if (sp.skip_token(SP_FACTOR, TT_FROM) != PS_NORMAL)
		{
			MCperror->add
			(PE_REMOVE_NOFROM, sp);
			return PS_ERROR;
		}
		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add
			(PE_REMOVE_NOPLACE, sp);
			return PS_ERROR;
		}
		if (sp.lookup(SP_INSERT, te) != PS_NORMAL)
		{
			MCperror->add
			(PE_REMOVE_NOPLACE, sp);
			return PS_ERROR;
		}
		where = (Insert_point)te->which;
		return PS_NORMAL;
	}

	target = new (nothrow) MCChunk(False);
	if (target->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_REMOVE_BADOBJECT, sp);
		return PS_ERROR;
	}
	while (sp.skip_token(SP_FACTOR, TT_FROM) == PS_NORMAL)
		;
	card = new (nothrow) MCChunk(False);
	if (card->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_REMOVE_BADOBJECT, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCRemove::exec_ctxt(MCExecContext& ctxt)
{
    if (all)
		MCEngineExecRemoveAllScriptsFrom(ctxt, where == IP_FRONT);
	else
	{
		MCObjectPtr optr;
		uint4 parid;
		if (!target->getobj(ctxt, optr, True))
        {
            ctxt . LegacyThrow(EE_REMOVE_NOOBJECT);
            return;
        }
		
		if (script)
			MCEngineExecRemoveScriptOfObjectFrom(ctxt, optr . object, where == IP_FRONT);
		else
		{
			if (optr . object->gettype() != CT_GROUP)
			{
				ctxt . LegacyThrow(EE_REMOVE_NOTABACKGROUND);
				return;
			}
            
			MCObject *cptr;
			if (!card->getobj(ctxt, cptr, parid, True))
            {
                ctxt . LegacyThrow(EE_REMOVE_NOOBJECT);
                return;
            }
            
			if (cptr->gettype() != CT_CARD)
			{
				ctxt . LegacyThrow(EE_REMOVE_NOTACARD);
				return;
			}
            
			MCCard *cardptr = (MCCard *)cptr;
			MCInterfaceExecRemoveGroupFromCard(ctxt, optr, cardptr);
		}
	}
}

MCRename::~MCRename()
{
	delete source;
	delete dest;
}

Parse_stat MCRename::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	sp.skip_token(SP_THERE, TT_UNDEFINED, TM_FILE);
	sp.skip_token(SP_THERE, TT_UNDEFINED, TM_DIRECTORY);
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add
		(PE_RENAME_BADEXP, sp);
		return PS_ERROR;
	}
	sp.skip_token(SP_EXIT, TT_UNDEFINED, ET_TO);
	if (sp.parseexp(False, True, &dest) != PS_NORMAL)
	{
		MCperror->add
		(PE_RENAME_BADEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCRename::exec_ctxt(MCExecContext &ctxt)
{
    MCAutoStringRef t_from;
    if (!ctxt . EvalExprAsStringRef(source, EE_RENAME_BADSOURCE, &t_from))
        return;
    
    MCAutoStringRef t_to;
    if (!ctxt . EvalExprAsStringRef(dest, EE_RENAME_BADDEST, &t_to))
        return;
    
    MCFilesExecRename(ctxt, *t_from, *t_to);
}

////////////////////////////////////////////////////////////////////////////////

MCReplace::~MCReplace()
{
	delete container;
	delete pattern;
	delete replacement;
}

Parse_stat MCReplace::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &pattern) != PS_NORMAL)
	{
		MCperror->add(PE_REPLACE_BADEXP, sp);
		return PS_ERROR;
	}
	sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH);
	if (sp.parseexp(False, True, &replacement) != PS_NORMAL)
	{
		MCperror->add(PE_REPLACE_BADEXP, sp);
		return PS_ERROR;
	}
	sp.skip_token(SP_FACTOR, TT_IN, PT_IN);
	container = new (nothrow) MCChunk(True);
	if (container->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add(PE_REPLACE_BADCONTAINER, sp);
		return PS_ERROR;
	}
	
	// Parse the replace in field suffix:
	//    replace ... with ... in <fieldchunk> (replacing | preserving) styles
	//
	if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_REPLACING) == PS_NORMAL)
		mode = kReplaceStyles;
	else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_PRESERVING) == PS_NORMAL)
		mode = kPreserveStyles;
	if (mode != kIgnoreStyles)
	{
		if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_STYLES) != PS_NORMAL)
		{
			MCperror->add(PE_REPLACE_NOSTYLES, sp);
			return PS_ERROR;
		}
	}
	
	return PS_NORMAL;
}

void MCReplace::exec_ctxt(MCExecContext& ctxt)
{
    MCAutoStringRef t_pattern;
    if (!ctxt . EvalExprAsStringRef(pattern, EE_REPLACE_BADPATTERN, &t_pattern))
        return;
    
    if (MCStringGetLength(*t_pattern) < 1)
    {
        ctxt . LegacyThrow(EE_REPLACE_BADPATTERN);
        return;
    }
    
    MCAutoStringRef t_replacement;
    if (!ctxt . EvalExprAsStringRef(replacement, EE_REPLACE_BADREPLACEMENT, &t_replacement))
        return;
	
	// The ignore styles mode treats all targets as strings, so we use
	// the string-based method.
	//
	// The replace styles mode is only valid for field chunks. It will
	// replace both the text and complete styling of each found string.
	//
	// The preserve styles mode is only valid for field chunks. It will
	// replace the text and merge the styling of the replacement pattern
	// with the found string. As the replacement string can only be
	// plain text at the moment, it is equivalent to using the same style
	// as the first char in the found string.
	
	if (mode == kIgnoreStyles)
	{
		MCAutoStringRef t_target;
		if (!ctxt . EvalExprAsMutableStringRef(container, EE_REPLACE_BADCONTAINER, &t_target))
			return;

		MCStringsExecReplace(ctxt, *t_pattern, *t_replacement, *t_target);
		
		if (ctxt . HasError())
			return;
		
		container -> set(ctxt, PT_INTO, *t_target);
	}
	else
	{
		MCObjectChunkPtr t_obj_chunk;
		
		if (!container -> evalobjectchunk(ctxt,
										  true,
										  false,
										  t_obj_chunk) ||
			t_obj_chunk . object -> gettype() != CT_FIELD)
		{
			ctxt . LegacyThrow(EE_REPLACE_BADFIELDCHUNK);
			return;
		}
		
		MCInterfaceExecReplaceInField(ctxt,
									  *t_pattern,
									  *t_replacement,
									  t_obj_chunk,
									  mode == kPreserveStyles);
		
		MCValueRelease(t_obj_chunk . mark . text);
	}
}

MCRevert::~MCRevert()
{
    delete stack;
}

Parse_stat MCRevert::parse(MCScriptPoint &sp)
{
    initpoint(sp);
    Symbol_type type;
    if (sp.next(type) == PS_EOL)
        return PS_NORMAL;
    
    // Otherwise backup and parse a chunk
    sp.backup();
    stack = new (nothrow) MCChunk(False);
    if (stack->parse(sp, False) != PS_NORMAL)
    {
        MCperror->add(PE_REVERT_BADSTACK, sp);
        return PS_ERROR;
    }
    
    return PS_NORMAL;
}


void MCRevert::exec_ctxt(MCExecContext& ctxt)
{
    if (stack != nil)
    {
        MCObject *t_object;
        uint4 parid;
        
        if (!stack->getobj(ctxt, t_object, parid, True))
        {
            ctxt . LegacyThrow(EE_SAVE_NOTARGET);
            return;
        }
        
        MCInterfaceExecRevertStack(ctxt, t_object);
    }
    else
        MCInterfaceExecRevert(ctxt);
}

MCRotate::~MCRotate()
{
	delete angle;
	delete image;
}

Parse_stat MCRotate::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_IMAGE) == PS_NORMAL)
	{
		sp.backup();
		image = new (nothrow) MCChunk(False);
		if (image->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_ROTATE_BADIMAGE, sp);
			return PS_ERROR;
		}
	}
	sp.skip_token(SP_FACTOR, TT_PREP, PT_BY);
	if (sp.parseexp(False, True, &angle) != PS_NORMAL)
	{
		MCperror->add
		(PE_ROTATE_BADANGLE, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCRotate::exec_ctxt(MCExecContext& ctxt)
{
    
#ifndef _MOBILE
	MCImage *iptr;
	iptr = NULL;
    
	if (image != NULL)
	{
		MCObject *optr;
		uint4 parid;
        
		if (!image->getobj(ctxt, optr, parid, True) || optr->gettype() != CT_IMAGE)
		{
            ctxt . LegacyThrow(EE_ROTATE_NOTIMAGE);
			return;
		}
		iptr = (MCImage *)optr;
	}
    
    integer_t t_angle;
    if (!ctxt . EvalExprAsInt(angle, EE_ROTATE_BADANGLE, t_angle))
        return;
    
	if (iptr != NULL)
		MCGraphicsExecRotateImage(ctxt, iptr, t_angle);
	else
		MCGraphicsExecRotateSelection(ctxt, t_angle);
    
#endif
}

MCCrop::~MCCrop()
{
	delete newrect;
	delete image;
}

Parse_stat MCCrop::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_IMAGE) == PS_NORMAL)
	{
		sp.backup();
		image = new (nothrow) MCChunk(False);
		if (image->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_CROP_BADIMAGE, sp);
			return PS_ERROR;
		}
	}
	sp.skip_token(SP_FACTOR, TT_TO, PT_TO);
	if (sp.parseexp(False, True, &newrect) != PS_NORMAL)
	{
		MCperror->add
		(PE_CROP_BADRECT, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCCrop::exec_ctxt(MCExecContext& ctxt)
{	
	MCImage *iptr;
	MCRectangle t_rect;
	
	if (image != nil)
	{
		MCObject *optr;
		uint4 parid;
    
		if (!image->getobj(ctxt, optr, parid, True) || optr->gettype() != CT_IMAGE)
		{
            ctxt . LegacyThrow(EE_CROP_NOTIMAGE);
			return;
		}
		iptr = (MCImage *)optr;
        if (!ctxt . EvalExprAsRectangle(newrect, EE_CROP_CANTGETRECT, t_rect))
            return;
    }
    else
	{
		iptr = nil;
		t_rect = MCRectangleMake(0,0,0,0);
	}
    
	MCGraphicsExecCropImage(ctxt, iptr, t_rect);
 }

MCSelect::~MCSelect()
{
	deletetargets(&targets);
}

Parse_stat MCSelect::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	sp.skip_token(SP_FACTOR, TT_THE);
	if (sp.skip_token(SP_SORT, TT_UNDEFINED, ST_TEXT) == PS_NORMAL)
		text = True;
	else
	{
		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add
			(PE_SELECT_NOTARGET, sp);
			return PS_ERROR;
		}
		if (sp.token_is_cstring("empty"))
			return PS_NORMAL;
		if (sp.lookup(SP_FACTOR, te) == PS_NORMAL && te->type == TT_PREP)
		{
			where = (Preposition_type)te->which;
			if (sp.skip_token(SP_SORT, TT_UNDEFINED, ST_TEXT) == PS_NORMAL)
				text = True;
		}
		else
			sp.backup();
	}
	sp.skip_token(SP_FACTOR, TT_OF);
	if (gettargets(sp, &targets, True) != PS_NORMAL)
	{
		MCperror->add
		(PE_SELECT_BADTARGET, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCSelect::exec_ctxt(MCExecContext& ctxt)
{
	if (targets == NULL)
		MCInterfaceExecSelectEmpty(ctxt);
    // AL-2014-08-04: [[ Bug 13079 ]] 'select before/after text' should use chunk variant
	else if (text && where == PT_AT)
	{
		MCObjectPtr t_object;
        if (!targets -> getobj(ctxt, t_object, True))
        {
            ctxt . LegacyThrow(EE_SELECT_BADTARGET);
            return;
        }
        if (t_object . object -> gettype() == CT_FIELD)
            MCInterfaceExecSelectAllTextOfField(ctxt, t_object);
        else if (t_object . object -> gettype() == CT_BUTTON)
            // AL-2014-08-04: [[ Bug 13079 ]] 'select text of button' is valid
            MCInterfaceExecSelectAllTextOfButton(ctxt, t_object);
        else
        {
            ctxt . LegacyThrow(EE_CHUNK_BADCONTAINER);
            return;
        }
	}
    else if (text || targets -> next == nil)
	{
		MCObjectChunkPtr t_chunk;
		
		if (!targets -> evalobjectchunk(ctxt, false, false, t_chunk))
        {
            ctxt . LegacyThrow(EE_SELECT_BADTARGET);
            return;
        }

		if (t_chunk . chunk != CT_UNDEFINED || where == PT_BEFORE || where == PT_AFTER)
		{
			if (t_chunk . object -> gettype() == CT_FIELD)
				MCInterfaceExecSelectTextOfField(ctxt, where, t_chunk);
			else if (t_chunk . object -> gettype() == CT_BUTTON)
				MCInterfaceExecSelectTextOfButton(ctxt, where, t_chunk);
			else
			{
                ctxt . LegacyThrow(EE_CHUNK_BADCONTAINER);
                MCValueRelease(t_chunk . mark . text);
				return;
			}
		}
		else
		{
			MCObjectPtr t_object;
			t_object . object = t_chunk . object;
			t_object . part_id = t_chunk . part_id;
            MCInterfaceExecSelectObjects(ctxt, &t_object, 1);
		}
        MCValueRelease(t_chunk . mark . text);
	}
    else
	{
		MCChunk *chunkptr = targets;
		MCObjectPtr t_object;
		MCAutoArray<MCObjectPtr> t_objects;
        
		while (chunkptr != NULL)
		{
			if (!chunkptr->getobj(ctxt, t_object, True))
            {
                ctxt . LegacyThrow(EE_SELECT_BADTARGET);
                return;
            }
			
			if (!t_objects . Push(t_object))
			{
                ctxt .LegacyThrow(EE_NO_MEMORY);
				return;
			}
            
			chunkptr = chunkptr->next;
		}
		MCInterfaceExecSelectObjects(ctxt, t_objects . Ptr(), t_objects . Size());
	}
}

void MCUndoCmd::exec_ctxt(MCExecContext& ctxt)
{
    MCInterfaceExecUndo(ctxt);
}

MCUngroup::~MCUngroup()
{
	delete group;
}

Parse_stat MCUngroup::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	MCScriptPoint oldsp(sp);
	MCerrorlock++;
	group = new (nothrow) MCChunk(False);
	if (group->parse(sp, False) != PS_NORMAL)
	{
		delete group;
		group = NULL;
		sp = oldsp;
	}
	MCerrorlock--;
	return PS_NORMAL;
}

void MCUngroup::exec_ctxt(MCExecContext& ctxt)
{
	if (group != NULL)
	{
		MCObject *gptr;
		uint4 parid;
		if (!group->getobj(ctxt, gptr, parid, True))
        {
            ctxt . LegacyThrow(EE_UNGROUP_NOGROUP);
            return;
        }

		if (gptr->gettype() != CT_GROUP)
		{
            ctxt . LegacyThrow(EE_UNGROUP_NOTAGROUP);
            return;
		}
		MCInterfaceExecUngroupObject(ctxt, gptr);
	}
	else
		MCInterfaceExecUngroupSelection(ctxt);
}

////////////////////////////////////////////////////////////////////////////////

MCRelayer::MCRelayer(void)
{
	form = kMCRelayerFormNone;
	relation = RR_NONE;
	control = nil;
	layer = nil;
}

MCRelayer::~MCRelayer(void)
{
	delete control;
	if (form == kMCRelayerFormRelativeToLayer)
		delete layer;
	else if (form == kMCRelayerFormRelativeToControl)
		delete target;
}

Parse_stat MCRelayer::parse(MCScriptPoint& sp)
{
	initpoint(sp);

	control = new (nothrow) MCChunk(False);
	if (control -> parse(sp, False) != PS_NORMAL)
	{
		MCperror -> add(PE_RELAYER_BADCONTROL, sp);
		return PS_ERROR;
	}

	if (sp . skip_token(SP_FACTOR, TT_TO, PT_TO) == PS_NORMAL)
	{
		if (sp . skip_token(SP_INSERT, TT_UNDEFINED, IP_FRONT) == PS_NORMAL)
			relation = RR_FRONT;
		else if (sp . skip_token(SP_INSERT, TT_UNDEFINED, IP_BACK) == PS_NORMAL)
			relation = RR_BACK;
		else
		{
			MCperror -> add(PE_RELAYER_BADRELATION, sp);
			return PS_ERROR;
		}
    }
	else if (sp . skip_token(SP_FACTOR, TT_PREP, PT_BEFORE) == PS_NORMAL)
		relation = RR_BEFORE;
	else if (sp . skip_token(SP_FACTOR, TT_PREP, PT_AFTER) == PS_NORMAL)
		relation = RR_AFTER;

	if (relation == RR_FRONT ||
		relation == RR_BACK)
	{
		if (sp . skip_token(SP_FACTOR, TT_OF, PT_OF) != PS_NORMAL)
		{
			MCperror -> add(PE_RELAYER_BADTARGET, sp);
			return PS_ERROR;
		}
	}

	if (sp . skip_token(SP_FACTOR, TT_PROPERTY, P_LAYER) == PS_NORMAL)
	{
		form = kMCRelayerFormRelativeToLayer;
		if (sp . parseexp(False, True, &layer) != PS_NORMAL)
		{
			MCperror -> add(PE_RELAYER_BADTARGET, sp);
			return PS_ERROR;
		}
	}
	else if (sp . skip_token(SP_FACTOR, TT_FUNCTION, F_OWNER) == PS_NORMAL)
		form = kMCRelayerFormRelativeToOwner;
	else
	{
		form = kMCRelayerFormRelativeToControl;
		target = new (nothrow) MCChunk(False);
		if (target -> parse(sp, False) != PS_NORMAL)
		{
			MCperror -> add(PE_RELAYER_BADTARGET, sp);
			return PS_ERROR;
		}
	}

	return PS_NORMAL;
}

void MCRelayer::exec_ctxt(MCExecContext& ctxt)
{
    
    // Fetch the source object.
	MCObjectPtr t_source;
    if (!control -> getobj(ctxt, t_source, True))
	{
        ctxt . LegacyThrow(EE_RELAYER_NOSOURCE);
        return;
	}

	switch(form)
	{
        case kMCRelayerFormRelativeToLayer:
            uint4 t_layer;
            if (!ctxt . EvalExprAsUInt(layer, EE_RELAYER_BADLAYER, t_layer))
                return;
            
            MCInterfaceExecRelayer(ctxt, relation, t_source, t_layer);
            break;
        case kMCRelayerFormRelativeToControl:
        {
            MCObjectPtr t_target;
            if (!target -> getobj(ctxt, t_target, True))
            {
                ctxt . LegacyThrow(EE_RELAYER_NOTARGET);
                return;
            }
            MCInterfaceExecRelayerRelativeToControl(ctxt, relation, t_source, t_target);
        }
            break;
        case kMCRelayerFormRelativeToOwner:
            MCInterfaceExecRelayerRelativeToOwner(ctxt, relation, t_source);
            break;
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
