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
//#include "execpt.h"
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

#include "syntax.h"
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

	source = new MCChunk(False);
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
#ifdef /* MCClone */ LEGACY_EXEC
	MCStack *odefaultstackptr = MCdefaultstackptr;
	MCObject *optr = NULL;
	uint4 parid;

	if (source->getobj(ep, optr, parid, True) != ES_NORMAL)
	{
		MCeerror->add
		(EE_CLONE_NOTARGET, line, pos);
		return ES_ERROR;
	}
	
	switch (optr->gettype())
	{
	case CT_STACK:
		{
			MCStack *sptr = (MCStack *)optr;
			optr = sptr->clone();
			if (newname == NULL)
			{
				sptr->getprop(0, P_SHORT_NAME, ep, False);
				ep.insert(MCcopystring, 0, 0);
				optr->setprop(0, P_NAME, ep, False);
			}
			MCdefaultstackptr = (MCStack *)optr;

			// OK-2008-06-23: [[Bug 6590]]
			if (!visible)
				optr->setflag(visible, F_VISIBLE);

			optr->open();
		}
		break;
	case CT_CARD:
		// MW-2005-03-10: Fix issue with card cloning which meant it wasn't working...
		if ((optr -> getstack() -> islocked() && MCdefaultstackptr == optr -> getstack()) ||
		        (MCdefaultstackptr != optr -> getstack() && MCdefaultstackptr -> islocked()))
		{
			MCeerror -> add(EE_CLONE_LOCKED, line, pos);
			return ES_ERROR;
		}
		else if (MCdefaultstackptr != optr -> getstack() &&
		         (!optr -> getstack() -> iskeyed() || !MCdefaultstackptr -> iskeyed()))
		{
			MCeerror -> add(EE_CLONE_CANTCLONE, line, pos);
			return ES_ERROR;
		}
		else
		{
			MCCard *cptr = (MCCard *)optr;
			cptr->getstack()->stopedit();
			optr = cptr->clone(True, True);
		}
		break;
	case CT_GROUP:
		// MW-2010-10-12: [[ Bug 8494 ]] Surely its just the group being edited that you
		//   don't want to clone... Indeed, it shouldn't even be possible to reference
		//   that group since it 'doesn't exist' in group editing mode.
		if (optr->getstack()->isediting() && optr->getstack()->getediting() == optr)
		{
			optr = NULL;
			break;
		}
	case CT_BUTTON:
	case CT_FIELD:
	case CT_IMAGE:
	case CT_SCROLLBAR:
	case CT_PLAYER:
	case CT_GRAPHIC:
	case CT_EPS:
	case CT_COLOR_PALETTE:
	case CT_MAGNIFY:
		if (optr -> getstack() -> islocked())
		{
			MCeerror -> add
				(EE_CLONE_LOCKED, line, pos);
			return ES_ERROR;
		}
		else
		{
			MCControl *coptr = (MCControl *)optr;
			
			optr = coptr -> clone(True, OP_OFFSET, !visible);
		}
		break;
	default:
		break;
	}
	if (newname != NULL)
	{
		if (newname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CLONE_BADNAME, line, pos);
			return ES_ERROR;
		}
		optr->setprop(0, P_NAME, ep, False);
	}
	if (optr == NULL)
	{
		MCeerror->add
		(EE_CLONE_CANTCLONE, line, pos);
		return ES_ERROR;
	}
	optr->getprop(0, P_LONG_ID, ep, False);
	ep.getit()->set(ep);
	MCdefaultstackptr = odefaultstackptr;
	return ES_NORMAL; 
#endif /* MCClone */

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

void MCClone::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	source -> compile_object_ptr(ctxt);
	if (newname != nil)
		newname -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstantNil(ctxt);
	MCSyntaxFactoryEvalConstantBool(ctxt, visible == False);

	MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecCloneMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
		dest = new MCChunk(False);
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
#ifdef /* MCClipboardCmd */ LEGACY_EXEC
	// Implicit form - use current context
	if (targets == NULL)
	{
		if (MCactivefield != NULL)
		{
			if (iscut())
				MCactivefield -> cuttext();
			else
				MCactivefield -> copytext();
		}
		else if (MCactiveimage != NULL)
		{
			if (iscut())
				MCactiveimage -> cutimage();
			else
				MCactiveimage -> copyimage();
		}
		else
		{
			if (iscut())
				MCselected -> cut();
			else
				MCselected -> copy();
		}

		return ES_NORMAL;
	}

	// Explicit form (1) - text chunk
	if (targets -> istextchunk())
	{
		if (targets -> next != NULL)
		{
			MCeerror -> add(EE_CLIPBOARD_BADMIX, line, pos);
			return ES_ERROR;
		}

		MCField* t_field;
		uint4 t_part, t_start;
		uint4 t_end;
		if (targets -> marktextchunk(ep, t_field, t_part, t_start, t_end) != ES_NORMAL)
		{
			MCeerror -> add(EE_CLIPBOARD_BADTEXT, line, pos);
			return ES_ERROR;
		}

		if (iscut())
			t_field -> cuttextindex(t_part, t_start, t_end);
		else
			t_field -> copytextindex(t_part, t_start, t_end);

		return ES_NORMAL;
	}

	// Explicit form (2)/(3) - object chunks
	Exec_errors t_error;
	t_error = EE_UNDEFINED;

	MCObjectRef *t_objects;
	uint4 t_object_count;
	t_objects = NULL;
	t_object_count = 0;
	for(MCChunk *t_target = targets; t_target != NULL && t_error == EE_UNDEFINED; t_target = t_target -> next)
	{
		if (t_target -> istextchunk())
			t_error = EE_CLIPBOARD_BADMIX;

		MCObject *t_object;
		uint4 t_part;
		if (t_error == EE_UNDEFINED)
		{
			if (t_target -> getobj(ep, t_object, t_part, True) != ES_NORMAL)
				t_error = EE_CLIPBOARD_BADOBJ;
		}

		if (t_error == EE_UNDEFINED)
		{
			if (!t_object -> getstack() -> iskeyed())
			{
				MCresult -> sets("can't cut object (stack is password protected)");
				continue;
			}
		}

		if (t_error == EE_UNDEFINED)
		{
			MCObjectRef *t_new_objects;
			t_new_objects = (MCObjectRef *)realloc(t_objects, sizeof(MCObjectRef) * (t_object_count + 1));
		if (t_new_objects == NULL)
				t_error = EE_NO_MEMORY;
			else
			{
				t_objects = t_new_objects;
				t_objects[t_object_count] . object = t_object;
				t_objects[t_object_count] . part = t_part;
				t_object_count += 1;
			}
		}
	}

	// Calculate destination object (if applicable)
	MCObject *t_dst_object;
	t_dst_object = NULL;
	if (t_error == EE_UNDEFINED && dest != NULL)
	{
		uint4 t_part;
		if (dest -> getobj(ep, t_dst_object, t_part, True) != ES_NORMAL)
			t_error = EE_CLIPBOARD_NODEST;
	}

	// Check destination compatibility
	if (t_error == EE_UNDEFINED && dest != NULL)
	{
		Chunk_term t_dst_type;
		t_dst_type = t_dst_object -> gettype();
		for(uint4 i = 0; i < t_object_count; ++i)
		{
			Chunk_term t_src_type;
			t_src_type = t_objects[i] . object -> gettype();

			if (t_src_type <= CT_CARD && t_dst_type != CT_STACK)
				t_error = EE_CLIPBOARD_BADDEST;
			else if (t_dst_type != CT_STACK && t_dst_type != CT_CARD && t_dst_type != CT_GROUP)
				t_error = EE_CLIPBOARD_BADDEST;
		}
	}

	if (t_object_count > 0)
	{
		// MW-2013-11-08: [[ RefactorIt ]] Both 'processto' methods in theory need context so
		//   pass ep.
		if (t_error == EE_UNDEFINED && dest != NULL)
			t_error = processtocontainer(ep, t_objects, t_object_count, t_dst_object);
		else if (t_error == EE_UNDEFINED)
			t_error = processtoclipboard(ep, t_objects, t_object_count);
	}

	Exec_stat t_stat;
	if (t_error == EE_UNDEFINED)
		t_stat = ES_NORMAL;
	else
	{
		MCeerror -> add(t_error, line, pos);
		t_stat = ES_ERROR;
	}

	if (t_objects != NULL)
		free(t_objects);

	return t_stat;
#endif /* MCClipboardCmd */

    if (targets == NULL)
	{
		// Implicit form - use current context
		if (iscut())
			MCPasteboardExecCut(ctxt);
		else
			MCPasteboardExecCopy(ctxt);
	}
	else if (targets -> istextchunk())
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

void MCClipboardCmd::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (targets == NULL)
	{
		if (iscut())
			MCSyntaxFactoryExecMethod(ctxt, kMCPasteboardExecCutMethodInfo);
		else
			MCSyntaxFactoryExecMethod(ctxt, kMCPasteboardExecCopyMethodInfo);
	}
	else
	{
		uindex_t t_count;
		t_count = 0;

		for (MCChunk *chunkptr = targets; chunkptr != nil; chunkptr = chunkptr -> next)
		{
			chunkptr -> compile_object_ptr(ctxt);
			t_count++;
		}
		
		if (dest != nil)
		{
			MCSyntaxFactoryEvalList(ctxt, t_count);
			dest -> compile_object_ptr(ctxt);

			if (iscut())
				MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecCutObjectsToContainerMethodInfo);
			else
				MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecCopyObjectsToContainerMethodInfo);
		}
		else if (t_count > 1)
		{
			MCSyntaxFactoryEvalList(ctxt, t_count);

			if (iscut())
				MCSyntaxFactoryExecMethod(ctxt, kMCPasteboardExecCutObjectsToClipboardMethodInfo);
			else
				MCSyntaxFactoryExecMethod(ctxt, kMCPasteboardExecCopyObjectsToClipboardMethodInfo);
		}
		else
		{
			if (iscut())
			{
				MCSyntaxFactoryExecMethod(ctxt, kMCPasteboardExecCutTextToClipboardMethodInfo);
				MCSyntaxFactoryExecMethod(ctxt, kMCPasteboardExecCutObjectsToClipboardMethodInfo);
			}
			else
			{
				MCSyntaxFactoryExecMethod(ctxt, kMCPasteboardExecCopyTextToClipboardMethodInfo);
				MCSyntaxFactoryExecMethod(ctxt, kMCPasteboardExecCopyObjectsToClipboardMethodInfo);
			}
		}
	}
	MCSyntaxFactoryEndStatement(ctxt);
}

#ifdef LEGACY_EXEC
Exec_errors MCClipboardCmd::processtocontainer(MCExecPoint& ep, MCObjectRef *p_objects, uint4 p_object_count, MCObject *p_dst)
{
	bool t_cut;
	t_cut = iscut();

	MCObject *t_new_object;
	for(uint4 i = 0; i < p_object_count; ++i)
	{
		MCObject *t_object;
		t_object = p_objects[i] . object;

		uint4 t_part;
		t_part = p_objects[i] . part;

		switch(t_object -> gettype())
		{
		case CT_AUDIO_CLIP:
		{
			MCAudioClip *t_aclip;
			if (t_cut)
			{
				t_aclip = static_cast<MCAudioClip *>(t_object);
				t_object -> getstack() -> removeaclip(t_aclip);
			}
			else
				t_aclip = new MCAudioClip(*static_cast<MCAudioClip *>(t_object));

			t_new_object = t_aclip;
			p_dst -> getstack() -> appendaclip(t_aclip);
		}
		break;

		case CT_VIDEO_CLIP:
		{
			MCVideoClip *t_aclip;
			if (t_cut)
			{
				t_aclip = static_cast<MCVideoClip *>(t_object);
				t_object -> getstack() -> removevclip(t_aclip);
			}
			else
				t_aclip = new MCVideoClip(*static_cast<MCVideoClip *>(t_object));

			t_new_object = t_aclip;
			p_dst -> getstack() -> appendvclip(t_aclip);
		}
		break;

		case CT_CARD:
		{
			if (!t_cut)
			{
				MCStack *t_old_default;
				t_old_default = MCdefaultstackptr;
				MCdefaultstackptr = static_cast<MCStack *>(p_dst);
				MCdefaultstackptr -> stopedit();

				MCCard *t_card;
				t_card = static_cast<MCCard *>(t_object);

				t_new_object = t_card -> clone(True, True);

				MCdefaultstackptr = t_old_default;
			}
		}
		break;

		case CT_GROUP:
		case CT_BUTTON:
		case CT_SCROLLBAR:
		case CT_PLAYER:
		case CT_IMAGE:
		case CT_GRAPHIC:
		case CT_EPS:
		case CT_COLOR_PALETTE:
		case CT_FIELD:
		{
			if (p_dst -> gettype() == CT_STACK)
				p_dst = static_cast<MCStack *>(p_dst) -> getcurcard();

			if (!t_cut)
			{
				MCObject *t_old_parent;
				t_old_parent = t_object -> getparent();
				t_object -> setparent(p_dst);

				MCControl *t_control;
				t_control = static_cast<MCControl *>(t_object);

				// MW-2011-08-18: [[ Redraw ]] Move to use redraw lock/unlock.
				MCRedrawLockScreen();
				
				t_new_object = t_control -> clone(True, OP_NONE, false);

				MCControl *t_new_control;
				t_new_control = static_cast<MCControl *>(t_new_object);
                // SN-2014-12-08: [[ Bug 12726 ]] Avoid to dereference a nil pointer (and fall back
                //  to the default stack).
                if (t_old_parent == NULL)
                    t_new_control -> resetfontindex(MCdefaultstackptr);
                else if (p_dst -> getstack() != t_old_parent -> getstack())
					t_new_control -> resetfontindex(t_old_parent -> getstack());

				// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
				t_new_control -> layer_redrawall();

				MCRedrawUnlockScreen();

				t_object -> setparent(t_old_parent);
			}
		}
		break;

		default:
			return EE_CLIPBOARD_BADOBJ;
		break;
		}
	}

	if (t_new_object != NULL)
	{
		// MW-2013-11-08: [[ RefactorIt ]] Use a temp-ep for the value, but use the real ep for
		//   it setting.
		MCExecPoint ep2(NULL, NULL, NULL);
		t_new_object -> getprop(0, P_LONG_ID, ep2, False);
		ep.getit() -> set(ep2);
	}

	return EE_UNDEFINED;
}
#endif

#ifdef LEGACY_EXEC
Exec_errors MCClipboardCmd::processtoclipboard(MCExecPoint& ep, MCObjectRef *p_objects, uint4 p_object_count)
{
	// Pickle the list of objects. The only reason this could fail is due to lack of
	// memory.
	MCPickleContext *t_context;

	// MW-2012-03-04: [[ StackFile5500 ]] When pickling for the clipboard, make sure it
	//   includes both 2.7 and 5.5 stackfile formats.
	t_context = MCObject::startpickling(true);
	if (t_context == NULL)
		return EE_NO_MEMORY;

	for(uint4 i = 0; i < p_object_count; ++i)
		MCObject::continuepickling(t_context, p_objects[i] . object, p_objects[i] . part);

	MCSharedString *t_pickle;
	t_pickle = MCObject::stoppickling(t_context);
	if (t_pickle == NULL)
		return EE_NO_MEMORY;

	bool t_success;
	t_success = true;

	// Open the clipboard ready for storing data
	MCclipboarddata -> Open();

	// Attempt to store the objects onto the clipboard
	if (t_success)
		if (!MCclipboarddata -> Store(TRANSFER_TYPE_OBJECTS, t_pickle))
			t_success = false;

	// If we've managed to store objects, and we are only copying/cutting one
	// image object, attempt to copy the image data.
	// Note that we don't care if this doesn't succeed as its not critical.
	if (t_success)
		if (p_object_count == 1 && p_objects[0] . object -> gettype() == CT_IMAGE)
		{
			// MW-2011-02-26: [[ Bug 9403 ]] If no image data is fetched, then don't try
			//   and store anything!
			MCSharedString *t_image_data;
			t_image_data = static_cast<MCImage *>(p_objects[0] . object) -> getclipboardtext();
			if (t_image_data != nil)
			{
				MCclipboarddata -> Store(TRANSFER_TYPE_IMAGE, t_image_data);
				t_image_data -> Release();
			}
		}

	// Close the clipboard. If this returns false, then it means the
	// writing was unsuccessful.
	if (!MCclipboarddata -> Close())
		t_success = false;

	// If all is well, delete the original objects.
	if (t_success)
	{
		if (iscut())
		{
			for(uint4 i = 0; i < p_object_count; ++i)
			{
				if (p_objects[i] . object -> del())
                {
                    if (p_objects[i] . object -> gettype() == CT_STACK)
                        MCtodestroy -> remove(static_cast<MCStack *>(p_objects[i] . object));
                    p_objects[i] . object -> scheduledelete();
                }
			}
		}
	}
	else
		MCresult -> sets("unable to write to clipboard");

	if (t_pickle != NULL)
		t_pickle -> Release();

	return EE_UNDEFINED;
}
#endif

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
                container = new MCChunk(False);
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

#ifdef LEGACY_EXEC
MCControl *MCCreate::getobject(MCObject *&parent)
{
	switch (otype)
	{
	case CT_BACKGROUND:
	case CT_GROUP:
		return MCtemplategroup;
	case CT_BUTTON:
		return MCtemplatebutton;
	case CT_MENU:
		parent = MCmenubar != NULL ? MCmenubar : MCdefaultmenubar;
		return MCtemplatebutton;
	case CT_SCROLLBAR:
		return MCtemplatescrollbar;
	case CT_PLAYER:
		return (MCControl*)MCtemplateplayer;
	case CT_IMAGE:
		return MCtemplateimage;
	case CT_GRAPHIC:
		return MCtemplategraphic;
	case CT_EPS:
		return MCtemplateeps;
	case CT_FIELD:
		return MCtemplatefield;
	default:
		return NULL;
	}
}
#endif

void MCCreate::exec_ctxt(MCExecContext& ctxt)
{
#ifdef /* MCCreate */ LEGACY_EXEC
	if (directory)
	{
		if (MCsecuremode & MC_SECUREMODE_DISK)
		{
			MCeerror->add(EE_DISK_NOPERM, line, pos);
			return ES_ERROR;
		}
		if (newname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_CREATE_BADEXP, line, pos);
			return ES_ERROR;
		}
		char *fname = ep.getsvalue().clone();
		if (!MCS_mkdir(fname))
			MCresult->sets("can't create that directory");
		else
			MCresult->clear(False);
		delete fname;
		return ES_NORMAL;
	}
	if (alias)
	{
		if (MCsecuremode & MC_SECUREMODE_DISK)
		{
			MCeerror->add(EE_DISK_NOPERM, line, pos);
			return ES_ERROR;
		}
		if (newname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CREATE_BADFILEEXP, line, pos);
			return ES_ERROR;
		}
		char *aliasname = ep.getsvalue().clone();
		if (file->eval(ep) != ES_NORMAL)
		{
			delete aliasname;
			MCeerror->add
			(EE_CREATE_BADEXP, line, pos);
			return ES_ERROR;
		}
		char *fname = ep.getsvalue().clone();
		if (!MCS_createalias(fname, aliasname))
			MCresult->sets("can't create that alias");
		else
			MCresult->clear(False);
		delete fname;
		delete aliasname;
		return ES_NORMAL;
	}
    
    MCObject *optr;
    if (script_only_stack)
    {
        MCStack *t_new_stack;
        MCStackSecurityCreateStack(t_new_stack);
        MCdispatcher -> appendstack(t_new_stack);
        t_new_stack -> setparent(MCdispatcher -> gethome());
        t_new_stack -> message(MCM_new_stack);
        t_new_stack -> setflag(False, F_VISIBLE);
		// PM-2015-10-26: [[ Bug 16283 ]] Automatically update project browser to show newly created script only stacks
		t_new_stack -> open();
        ep . clear();
        t_new_stack -> setasscriptonly(ep);
        optr = t_new_stack;
    }
    else
    {
        if (otype != CT_STACK && MCdefaultstackptr->islocked())
        {
            MCeerror->add(EE_CREATE_LOCKED, line, pos);
            return ES_ERROR;
        }
        if (otype == CT_STACK)
        {
            MCStack *odefaultstackptr = MCdefaultstackptr;
            Boolean wasvisible = MCtemplatestack->isvisible();
            
            if (!visible)
                MCtemplatestack->setflag(visible, F_VISIBLE);
            if (container != NULL)
            {
                MCObject *tptr;
                uint4 parid;
                if (container->getobj(ep, tptr, parid, True) != ES_NORMAL ||
                        (tptr->gettype() != CT_GROUP && tptr->gettype() != CT_STACK))
                {
                    MCeerror->add(EE_CREATE_BADBGORCARD, line, pos);
                    return ES_ERROR;
                }
                MCdefaultstackptr = MCtemplatestack->clone();
                MCdefaultstackptr->open();
                if (tptr->gettype() == CT_GROUP)
                {
                    MCGroup *gptr = (MCGroup *)tptr;
                    MCdefaultstackptr->setrect(gptr->getstack()->getrect());
                    gptr = (MCGroup *)gptr->clone(False, OP_NONE, false);
                    gptr->setparent(MCdefaultstackptr);
                    gptr->resetfontindex(tptr->getstack());
                    gptr->attach(OP_NONE, false);
                }
                else
                {
                    tptr->getprop(0, P_NAME, ep, False);
                    if (MCdefaultstackptr->setprop(0, P_MAIN_STACK, ep, False) != ES_NORMAL)
                    {
                        delete MCdefaultstackptr;
                        MCeerror->add(EE_CREATE_BADBGORCARD, line, pos);
                        return ES_ERROR;
                    }
                }
            }
            else
            {
                MCdefaultstackptr = MCtemplatestack->clone();
                MCdefaultstackptr->open();
            }
            MCtemplatestack->setflag(wasvisible, F_VISIBLE);
            optr = MCdefaultstackptr;
            MCdefaultstackptr = odefaultstackptr;
        }
        else
        {
            MCObject *parent = NULL;
            if (container != NULL)
            {
                uint4 parid;
                if (container->getobj(ep, parent, parid, True) != ES_NORMAL
                        || parent->gettype() != CT_GROUP)
                {
                    MCeerror->add
                    (EE_CREATE_BADBGORCARD, line, pos);
                    return ES_ERROR;
                }
            }
            if (otype == CT_CARD)
            {
                MCdefaultstackptr->stopedit();
                optr = MCtemplatecard->clone(True, False);
            }
            else
            {
                MCControl *cptr = getobject(parent);
                if (cptr == NULL)
                    return ES_ERROR;
                Boolean wasvisible = cptr->isvisible();
                if (!visible)
                    cptr->setflag(visible, F_VISIBLE);
                cptr->setparent(parent);
                optr = cptr->clone(True, OP_CENTER, false);
                if (cptr == getobject(parent))
                { // handle case where template reset
                    cptr->setparent(NULL);
                    if (!visible)
                        cptr->setflag(wasvisible, F_VISIBLE);
                }
                if (otype == CT_MENU)
                {
                    MCButton *bptr = (MCButton *)optr;
                    bptr->setupmenu();
                }
            }
        }
    }
	if (newname != NULL)
	{
		if (newname->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CREATE_BADEXP, line, pos);
			return ES_ERROR;
		}
		optr->setprop(0, P_NAME, ep, False);
	}
	optr->getprop(0, P_LONG_ID, ep, False);
	ep.getit()->set(ep);
	return ES_NORMAL;
#endif /* MCCreate */

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

        MCObject *optr;
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
                MCInterfaceExecCreateCard(ctxt, *t_new_name, visible == False);
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
                    
                    if (!container->getobj(ctxt, parent, parid, True) || parent->gettype() != CT_GROUP)
                    {
                        ctxt . LegacyThrow(EE_CREATE_BADBGORCARD);
                        return;
                    }
                }
                MCInterfaceExecCreateWidget(ctxt, *t_new_name, *t_kind, (MCGroup *)parent, visible == False);
                break;
            }
            default:
            {
                MCObject *parent = nil;
                if (container != nil)
                {
                    uint4 parid;

                    if (!container->getobj(ctxt, parent, parid, True)
                            || parent->gettype() != CT_GROUP)
                    {
                        ctxt . LegacyThrow(EE_CREATE_BADBGORCARD);
                        return;
                    }
                }
                MCInterfaceExecCreateControl(ctxt, *t_new_name, otype, (MCGroup *)parent, visible == False);
            }
                break;
            }
        }
	}
}

void MCCreate::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (directory)
	{
		newname -> compile(ctxt);

		MCSyntaxFactoryExecMethod(ctxt, kMCFilesExecCreateFolderMethodInfo);
	}
	else if (alias)
	{
		file -> compile(ctxt);
		newname -> compile(ctxt);

		MCSyntaxFactoryExecMethod(ctxt, kMCFilesExecCreateAliasMethodInfo);
	}
	else 
	{
		switch (otype)
		{
		case CT_STACK:
			if (container != nil)
				container -> compile_object_ptr(ctxt);
			else
				MCSyntaxFactoryEvalConstantNil(ctxt);
			
			if (newname != nil)
				newname -> compile(ctxt);
			else
				MCSyntaxFactoryEvalConstantNil(ctxt);

			MCSyntaxFactoryEvalConstantBool(ctxt, visible == False);
			
			if (container != nil)
				MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecCreateStackWithGroupMethodInfo); 

			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecCreateStackMethodInfo);
			break;

		case CT_CARD:
			if (newname != nil)
				newname -> compile(ctxt);
			else
				MCSyntaxFactoryEvalConstantNil(ctxt);

			MCSyntaxFactoryEvalConstantBool(ctxt, visible == False);

			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecCreateCardMethodInfo);
			break;

		default:
			if (newname != nil)
				newname -> compile(ctxt);
			else
				MCSyntaxFactoryEvalConstantNil(ctxt);

			MCSyntaxFactoryEvalConstantInt(ctxt, otype);

			if (container != nil)
				container -> compile_object_ptr(ctxt);
			else
				MCSyntaxFactoryEvalConstantNil(ctxt);

			MCSyntaxFactoryEvalConstantBool(ctxt, visible == False);

			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecCreateControlMethodInfo);
			break;
		}
	}

	MCSyntaxFactoryEndStatement(ctxt);
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
	target = new MCChunk(False);
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
#ifdef /* MCCustomProp */ LEGACY_EXEC
	return ES_NORMAL;
#endif /* MCCustomProp */
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
#ifdef /* MCDelete */ LEGACY_EXEC
	if (var != NULL)
		return var->dofree(ep);
	if (file != NULL)
	{
		if (file->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_DELETE_BADFILEEXP, line, pos);
			return ES_ERROR;
		}
		if (url)
		{
			char *sptr = NULL;
			if (ep.getsvalue().getlength() > 5
			        && !MCU_strncasecmp(ep.getsvalue().getstring(), "file:", 5)
			        || ep.getsvalue().getlength() > 8
			        && !MCU_strncasecmp(ep.getsvalue().getstring(), "binfile:", 8))
			{
				// Check the disk access here since MCS_unlink is used more
				// generally.
				if (!MCSecureModeCheckDisk(line, pos))
					return ES_ERROR;

				if (!MCU_strncasecmp(ep.getsvalue().getstring(), "file:", 5))
					ep.tail(5);
				else
					ep.tail(8);
				sptr = ep.getsvalue().clone();
				if (!MCS_unlink(sptr))
					MCresult->sets("can't delete that file");
				else
					MCresult->clear(False);
				delete sptr;
			}
			else if (ep.getsvalue().getlength() > 8
						&& !MCU_strncasecmp(ep.getsvalue().getstring(), "resfile:", 8))
			{
				ep.tail(8);
				MCS_saveresfile(ep.getsvalue(), MCnullmcstring);
			}
			else
				MCS_deleteurl(ep.getobj(), ep.getcstring());
		}
		else
		{
			if (!MCSecureModeCheckDisk(line, pos))
				return ES_ERROR;

			char *fname = ep.getsvalue().clone();
			Boolean done = False;
			if (MCS_exists(fname, !directory))
				if (directory)
					done = MCS_rmdir(fname);
				else
				{
					IO_closefile(fname);
					done = MCS_unlink(fname);
				}
			if (!done)
				MCresult->sets("can't delete that file");
			else
				MCresult->clear(False);
			delete fname;
		}
		return ES_NORMAL;
	}
	if (targets != NULL)
	{
		MCChunk *tptr = targets;
		while (tptr != NULL)
		{
			if (tptr->del(ep) != ES_NORMAL)
			{
				MCeerror->add
				(EE_DELETE_NOOBJ, line, pos);
				return ES_ERROR;
			}
			tptr = tptr->next;
		}
	}
	else if (MCactivefield != NULL)
		MCactivefield->deleteselection(False);
	else if (MCactiveimage != NULL)
		MCactiveimage->delimage();
	else if (session)
	{
#ifdef _SERVER
		if (!MCServerDeleteSession())
		{
			MCeerror->add(EE_UNDEFINED, line, pos);
			return ES_ERROR;
		}
#else
		MCeerror->add(EE_SESSION_BADCONTEXT, line, pos);
		return ES_ERROR;
#endif
	}
	else
		MCselected->del();
	return ES_NORMAL; 
#endif /* MCDelete */

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
    else if (targets != nil && targets -> istextchunk())
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
    else if (targets != nil)
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

void MCDelete::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (var != nil)
	{
		var -> compile(ctxt);
		
		MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecDeleteVariableMethodInfo);
	}
	else if (file != NULL)
	{
		file -> compile(ctxt);

		if (url)
			MCSyntaxFactoryExecMethod(ctxt, kMCNetworkExecDeleteUrlMethodInfo);
		else
			MCSyntaxFactoryExecMethod(ctxt, kMCFilesExecDeleteFileMethodInfo);
	}
	else if (targets != nil)
	{
		uindex_t t_count;
		t_count = 0;

		for (MCChunk *t_chunk = targets; t_chunk != nil; t_chunk = t_chunk -> next)
		{
			t_chunk -> compile_object_ptr(ctxt);
			t_count++;
		}
		
		MCSyntaxFactoryEvalList(ctxt, t_count);

		MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecDeleteVariableChunksMethodInfo);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecDeleteObjectChunksMethodInfo);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecDeleteObjectsMethodInfo); 
	}
	else if (session)
	{
#ifdef _SERVER
		MCSyntaxFactoryExecMethod(ctxt, kMCServerExecDeleteSessionMethodInfo);
#endif
	}
	else
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecDeleteMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
#ifdef /* MCChangeProp */ LEGACY_EXEC
    return targets->changeprop(ep, prop, value);
#endif /* MCChangeProp */

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

void MCChangeProp::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	targets -> compile(ctxt);

	switch(prop)
	{
	case P_DISABLED:
		if (value)
		{
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecDisableChunkOfButtonMethodInfo);
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecDisableObjectMethodInfo);
		}
		else
		{
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecEnableChunkOfButtonMethodInfo);
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecEnableObjectMethodInfo);
		}
		break;
	case P_HILITE:
		if (value)
		{
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecHiliteChunkOfButtonMethodInfo);
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecHiliteObjectMethodInfo);
		}
		else
		{
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecUnhiliteChunkOfButtonMethodInfo);
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecUnhiliteObjectMethodInfo);
		}
		break;
	default:
		break;
	}

	MCSyntaxFactoryEndStatement(ctxt);
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
	image = new MCChunk(False);
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
#ifdef /* MCFlip */ LEGACY_EXEC
	bool t_created_selection;
	MColdtool = MCcurtool;

	if (image != NULL)
	{
		MCObject *optr;
		uint4 parid;
		if (image->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_FLIP_NOIMAGE, line, pos);
			return ES_ERROR;
		}
		// MW-2013-07-01: [[ Bug 10999 ]] Throw an error if the image is not editable.
		if (optr->gettype() != CT_IMAGE)
		{
			MCeerror->add(EE_FLIP_NOTIMAGE, line, pos);
			return ES_ERROR;
		}
		
		// MW-2013-10-25: [[ Bug 11300 ]] If this is a reference image, then flip using
		//   transient flags in the image object.
		MCImage *iptr = (MCImage *)optr;
		if (optr->getflag(F_HAS_FILENAME))
		{
			iptr -> flip(direction == FL_HORIZONTAL);
			return ES_NORMAL;
		}
		
		iptr->selimage();
		t_created_selection = true;
	}
	else
		t_created_selection = false;

	if (MCactiveimage != NULL)
	{
		MCactiveimage->flipsel(direction == FL_HORIZONTAL);

		// IM-2013-06-28: [[ Bug 10999 ]] ensure MCactiveimage is not null when calling endsel() method
		if (t_created_selection)
			MCactiveimage -> endsel();
	}

	MCcurtool = MColdtool;

	return ES_NORMAL;
#endif /* MCFlip */

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

    if (MCactiveimage != nil)
        MCGraphicsExecFlipSelection(ctxt, direction == FL_HORIZONTAL);
}

void MCFlip::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (image != nil)
	{
		image -> compile(ctxt);
		MCSyntaxFactoryEvalConstantBool(ctxt, direction == FL_HORIZONTAL);

		MCSyntaxFactoryExecMethod(ctxt, kMCGraphicsExecFlipImageMethodInfo);
	}
	else
	{
		MCSyntaxFactoryEvalConstantBool(ctxt, direction == FL_HORIZONTAL);

		MCSyntaxFactoryExecMethod(ctxt, kMCGraphicsExecFlipSelectionMethodInfo);
	}

	MCSyntaxFactoryEndStatement(ctxt);
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
#ifdef /* MCGrab */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;
	if (control->getobj(ep, optr, parid, True) != ES_NORMAL
	        || optr->gettype() < CT_GROUP)
	{
		MCeerror->add
		(EE_GRAB_NOOBJ, line, pos);
		return ES_ERROR;
	}
	MCControl *cptr = (MCControl *)optr;
	cptr->grab();
	return ES_NORMAL;
#endif /* MCGrab */


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

void MCGrab::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	control -> compile(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecGrabMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
		widget = new MCChunk(False);
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
#ifdef /* MCLaunch */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_PROCESS)
	{
		MCeerror->add(EE_PROCESS_NOPERM, line, pos);
		return ES_ERROR;
	}

	char *appname;
	if (app != NULL)
	{
		if (app->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_LAUNCH_BADAPPEXP, line, pos);
			return ES_ERROR;
		}
		appname = ep.getsvalue().clone();
	}
	else
		appname = NULL;

	char *docname;
	if (doc != NULL)
	{
		if (doc->eval(ep) != ES_NORMAL)
		{
			delete appname;
			MCeerror->add(EE_LAUNCH_BADAPPEXP, line, pos);
			return ES_ERROR;
		}
		docname = ep.getsvalue().clone();
	}
	else
		docname = NULL;

	if (appname != NULL)
	{
		uint2 index;
		if (IO_findprocess(appname, index))
		{
			MCresult->sets("process is already open");
			delete appname;
			return ES_NORMAL;
		}
		MCS_startprocess(appname, docname == NULL ? strclone("") : docname, OM_NEITHER, False);
	}
	else if (docname != NULL)
	{
		if (as_url)
		{
			char *t_url;
			t_url = docname;
			
			// MW-2008-04-02: [[ Bug 6306 ]] Make sure we escape invalid URL characters to save
			//   the user having to do so.
			MCExecPoint t_new_ep(NULL, NULL, NULL);
			while(*t_url != '\0')
			{
				// MW-2008-08-14: [[ Bug 6898 ]] Interpreting this as a signed char causes sprintf
				//   to produce bad results.

				unsigned char t_char;
				t_char = *((unsigned char *)t_url);

				// MW-2008-06-12: [[ Bug 6446 ]] We must not escape '#' because this breaks URL
				//   anchors.
				if (t_char < 128 && (isalnum(t_char) || strchr("$-_.+!*'%(),;/?:@&=#", t_char) != NULL))
					t_new_ep . appendchar(t_char);
				else
					t_new_ep . appendstringf("%%%02X", t_char);

				t_url += 1;
			}
			
			delete docname;
			docname = t_new_ep . getsvalue() . clone();
			
			MCS_launch_url(docname);
		}
		else
			MCS_launch_document(docname);
	}

	return ES_NORMAL;
#endif /* MCLaunch */

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

void MCLaunch::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);
	
	if (app != nil)
	{
		app -> compile(ctxt);
		doc -> compile(ctxt);

		MCSyntaxFactoryExecMethod(ctxt, kMCFilesExecLaunchAppMethodInfo);
	}
	else if (doc != nil)
	{	
		doc -> compile(ctxt);

		if (as_url)
		{
			if (widget)
			{
				widget->compile(ctxt);
				MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecLaunchUrlInWidgetMethodInfo);
			}
			else
				MCSyntaxFactoryExecMethod(ctxt, kMCFilesExecLaunchUrlMethodInfo);
		}
		else
			MCSyntaxFactoryExecMethod(ctxt, kMCFilesExecLaunchDocumentMethodInfo);
	}

	MCSyntaxFactoryEndStatement(ctxt);
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
#ifdef /* MCLoad */ LEGACY_EXEC
	char *mptr;
	if (message != NULL)
	{
		if (message->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_LOAD_BADMESSAGEEXP, line, pos);
			return ES_ERROR;
		}
		mptr = ep.getsvalue().clone();
	}
	else
		mptr = strclone(MCnullstring);

	if (url->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_LOAD_BADURLEXP, line, pos);
		return ES_ERROR;
	}
	
	MCS_loadurl(ep . getobj(), ep . getcstring(), mptr);

	delete mptr;
	return ES_NORMAL;
#endif /* MCLoad */
    
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

void MCLoad::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);
	
	url -> compile(ctxt);

	if (message != nil)
		message -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstant(ctxt, kMCEmptyName);

	MCSyntaxFactoryExecMethod(ctxt, kMCNetworkExecLoadUrlMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
#ifdef /* MCUnload */ LEGACY_EXEC
	if (url->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_UNLOAD_BADURLEXP, line, pos);
		return ES_ERROR;
	}
	MCS_unloadurl(ep . getobj(), ep . getcstring());
	return ES_NORMAL;
#endif /* MCUnload */
    
    MCAutoStringRef t_url;
    if (!ctxt . EvalExprAsStringRef(url, EE_LOAD_BADURLEXP, &t_url))
        return;
    
	if (is_extension)
		MCEngineExecUnloadExtension(ctxt, *t_url);
	else
		MCNetworkExecUnloadUrl(ctxt, *t_url);
}

void MCUnload::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);
	
	url -> compile(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCNetworkExecUnloadUrlMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
#ifdef /* MCPost */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_POST_BADSOURCEEXP, line, pos);
		return ES_ERROR;
	}
	MCExecPoint ep2(ep);
	if (dest->eval(ep2) != ES_NORMAL)
	{
		MCeerror->add(EE_POST_BADDESTEXP, line, pos);
		return ES_ERROR;
	}
	MCS_posttourl(ep . getobj(), ep . getsvalue(), ep2 . getcstring());
	MCurlresult->fetch(ep);
	return ep.getit()->set(ep);
#endif /* MCPost */
\
    MCAutoValueRef t_data;
    if (!ctxt . EvalExprAsValueRef(source, EE_POST_BADSOURCEEXP, &t_data))
        return;
    
    MCAutoStringRef t_url;
    if (!ctxt . EvalExprAsStringRef(dest, EE_POST_BADDESTEXP, &t_url))
        return;

    MCNetworkExecPostToUrl(ctxt, *t_data, *t_url);
}

void MCPost::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	source -> compile(ctxt);
	dest -> compile(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCNetworkExecPostToUrlMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
#ifdef /* MCMakeGroup */ LEGACY_EXEC
	if (targets != NULL)
	{
		MCObject *optr;
		uint4 parid;
		MCChunk *chunkptr = targets;
		
		// MW-2013-06-20: [[ Bug 10863 ]] Make sure all objects have this parent, after
		//   the first object has been resolved.
		MCObject *t_required_parent;
		t_required_parent = NULL;
		
		while (chunkptr != NULL)
		{
			if (chunkptr->getobj(ep, optr, parid, True) != ES_NORMAL)
			{
				MCeerror->add(EE_GROUP_NOOBJ, line, pos);
				return ES_ERROR;
			}
			
			// MW-2013-06-20: [[ Bug 10863 ]] Only objects which are controls, and have a
			//   parent are groupable.
			if (optr->gettype() > CT_LAST_CONTROL ||
				optr->gettype() < CT_GROUP ||
				optr->getparent() == NULL ||
				optr->getparent()->gettype() != CT_CARD)
			{
				MCeerror->add(EE_GROUP_NOTGROUPABLE, line, pos);
				return ES_ERROR;
			}
			
			// MW-2011-01-21: Make sure we don't try and group shared groups
			if (optr -> gettype() == CT_GROUP && static_cast<MCGroup *>(optr) -> isshared())
			{
				MCeerror->add(EE_GROUP_NOBG, line, pos);
				return ES_ERROR;
			}
			
			// MW-2013-06-20: [[ Bug 10863 ]] Take the parent of the first object for
			//   future comparisons.
			if (t_required_parent == NULL)
				t_required_parent = optr -> getparent();
            
			// MERG-2013-05-07: [[ Bug 10863 ]] Make sure all objects have the same
			//   parent.
            if (optr->getparent() != t_required_parent)
            {
                MCeerror->add(EE_GROUP_DIFFERENTPARENT, line, pos);
				return ES_ERROR;
            }
			
			chunkptr->setdestobj(optr);
			chunkptr = chunkptr->next;
		}
		chunkptr = targets;
		MCControl *controls = NULL;
		MCCard *tcard = NULL;
		while (chunkptr != NULL)
		{
			MCControl *cptr = (MCControl *)chunkptr->getdestobj();
			tcard = cptr->getcard();
			tcard->removecontrol(cptr, False, True);
			cptr->getstack()->removecontrol(cptr);
			cptr->appendto(controls);
			chunkptr = chunkptr->next;
		}
		MCGroup *gptr;
		if (MCsavegroupptr == NULL)
			gptr = (MCGroup *)MCtemplategroup->clone(False, OP_NONE, false);
		else
			gptr = (MCGroup *)MCsavegroupptr->remove(MCsavegroupptr);
		gptr->makegroup(controls, tcard);
	}
	else
		return MCselected->group(line,pos);
	return ES_NORMAL;
#endif /* MCMakeGroup */

    if (targets != NULL)
	{
		MCAutoArray<MCObjectPtr> t_objects;
		for(MCChunk *t_chunk = targets; t_chunk != nil; t_chunk = t_chunk -> next)
		{
			MCObjectPtr t_object;
			if (!t_chunk -> getobj(ctxt, t_object, True) || t_object . object -> gettype() < CT_FIRST_CONTROL || t_object . object -> gettype() > CT_LAST_CONTROL)
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

void MCMakeGroup::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (targets != nil)
	{
		uindex_t t_count;
		t_count = 0;
		for (MCChunk *t_chunk = targets; t_chunk != nil; t_chunk = t_chunk -> next)
		{
			t_chunk -> compile_object_ptr(ctxt);
			t_count++;
		}

		MCSyntaxFactoryEvalList(ctxt, t_count);

		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecGroupControlsMethodInfo);
	}
	else
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecGroupSelectionMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
#ifdef /* MCPasteCmd */ LEGACY_EXEC
	MCObject *optr;
	if (!MCdispatcher -> dopaste(optr, true))
		MCresult->sets("can't paste (empty clipboard or locked destination)");
	else
		if (optr != NULL)
		{
			optr->getprop(0, P_LONG_ID, ep, False);
			ep.getit()->set(ep);
		}
	return ES_NORMAL;
#endif /* MCPasteCmd */


    MCPasteboardExecPaste(ctxt);
}

void MCPasteCmd::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	MCSyntaxFactoryExecMethod(ctxt, kMCPasteboardExecPasteMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
}

MCPlace::~MCPlace()
{
	delete group;
	delete card;
}

Parse_stat MCPlace::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	group = new MCChunk(False);
	if (group->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_PLACE_BADBACKGROUND, sp);
		return PS_ERROR;
	}
	while (sp.skip_token(SP_FACTOR, TT_PREP) == PS_NORMAL)
		;
	card = new MCChunk(False);
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
#ifdef /* MCPlace */ LEGACY_EXEC
	MCObject *gptr;
	uint4 parid;
	if (group->getobj(ep, gptr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_PLACE_NOBACKGROUND, line, pos);
		return ES_ERROR;
	}
	// MW-2008-03-31: [[ Bug 6281 ]] A little too draconian here - it is possible
	//   for a parent of a placeable group to be either a card or a stack.
	if (gptr->gettype() != CT_GROUP || (gptr -> getparent() -> gettype() != CT_CARD && gptr -> getparent() -> gettype() != CT_STACK))
	{
		MCeerror->add(EE_PLACE_NOTABACKGROUND, line, pos);
		return ES_ERROR;
	}
	MCObject *optr;
	if (card->getobj(ep, optr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_PLACE_NOCARD, line, pos);
		return ES_ERROR;
	}
	if (optr->gettype() != CT_CARD)
	{
		MCeerror->add(EE_PLACE_NOTACARD, line, pos);
		return ES_ERROR;
	}
	MCCard *cptr = (MCCard *)optr;
	if (cptr->getparent() != gptr->getstack()
	        || cptr->countme(gptr->getid(), False))
	{
		MCeerror->add(EE_PLACE_ALREADY, line, pos);
		return ES_ERROR;
	}
	
	// MW-2011-08-09: [[ Groups ]] If the group is not already marked as shared
	//   then turn on backgroundBehavior (legacy requirement).
	if (!static_cast<MCGroup *>(gptr) -> isshared())
		gptr->setflag(False, F_GROUP_ONLY);
	
	// MW-2011-08-09: [[ Groups ]] Make sure the group is marked as shared.
	gptr->setflag(True, F_GROUP_SHARED);

	cptr->newcontrol((MCControl *)gptr, True);

	return ES_NORMAL;
#endif /* MCPlace */

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

void MCPlace::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	group -> compile_object_ptr(ctxt);
	card -> compile_object_ptr(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecPlaceGroupOnCardMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
#ifdef /* MCRecord */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_PRIVACY)
	{
		MCeerror->add(EE_PROCESS_NOPERM, line, pos);
		return ES_ERROR;
	}

#ifdef FEATURE_PLATFORM_RECORDER
    extern MCPlatformSoundRecorderRef MCrecorder;
#endif
    
    if (file != nil)
    {
        if (file->eval(ep) != ES_NORMAL)
        {
            MCeerror->add(EE_RECORD_BADFILE, line, pos);
            return ES_ERROR;
        }
        char *soundfile = MCS_get_canonical_path(ep.getcstring());
        
#ifdef FEATURE_PLATFORM_RECORDER
        if (MCrecorder == nil)
            MCPlatformSoundRecorderCreate(MCrecorder);
        
        if (MCrecorder != nil)
            MCPlatformSoundRecorderStart(MCrecorder, soundfile);
#else
        extern void MCQTRecordSound(char *soundfile);
        MCQTRecordSound(soundfile);
#endif
    }
    else
    {
#ifdef FEATURE_PLATFORM_RECORDER
        if (MCrecorder != nil)
        {
            if (pause)
                MCPlatformSoundRecorderPause(MCrecorder);
            else
                MCPlatformSoundRecorderResume(MCrecorder);
        }
#else
        if (pause)
        {
            extern void MCQTRecordPause(void);
            MCQTRecordPause();
        }
        else
        {
            extern void MCQTRecordResume(void);
            MCQTRecordResume();
        }
#endif
    }
	return ES_NORMAL;
#endif /* MCRecord */

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

void MCRecord::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	file -> compile(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCMultimediaExecRecordMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
}

void MCRedo::exec_ctxt(MCExecContext &)
{
#ifdef /* MCRedo */ LEGACY_EXEC
    return ES_NORMAL;
#endif /* MCRedo */
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
		target = new MCChunk(False);
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

	target = new MCChunk(False);
	if (target->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_REMOVE_BADOBJECT, sp);
		return PS_ERROR;
	}
	while (sp.skip_token(SP_FACTOR, TT_FROM) == PS_NORMAL)
		;
	card = new MCChunk(False);
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
#ifdef /* MCRemove */ LEGACY_EXEC
	if (all)
	{
		MCObjectList *listptr = where == IP_FRONT ? MCfrontscripts : MCbackscripts;
		MCObjectList *lptr = listptr;
		do
		{
			lptr->setremoved(True);
			lptr = lptr->next();
		}
		while (lptr != listptr);
	}
	else
	{
		MCObject *optr;
		uint4 parid;
		if (target->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_REMOVE_NOOBJECT, line, pos);
			return ES_ERROR;
		}
		if (script)
			optr->removefrom(where == IP_FRONT ? MCfrontscripts : MCbackscripts);
		else
		{
			if (optr->gettype() != CT_GROUP)
			{
				MCeerror->add(EE_REMOVE_NOTABACKGROUND, line, pos);
				return ES_ERROR;
			}
			
			// MW-2011-08-09: [[ Groups ]] If the group's parent is a group then we
			//   can't unplace it.
			if (optr -> getparent() -> gettype() == CT_GROUP)
			{
				MCeerror -> add(EE_GROUP_CANNOTBEBGORSHARED, line, pos);
				return ES_ERROR;
			}

			MCObject *cptr;
			if (card->getobj(ep, cptr, parid, True) != ES_NORMAL)
			{
				MCeerror->add(EE_REMOVE_NOOBJECT, line, pos);
				return ES_ERROR;
			}

			if (cptr->gettype() != CT_CARD)
			{
				MCeerror->add(EE_REMOVE_NOTACARD, line, pos);
				return ES_ERROR;
			}

			MCCard *cardptr = (MCCard *)cptr;
			cardptr->removecontrol((MCControl *)optr, True, True);

			// MW-2011-08-09: [[ Groups ]] Removing a group from a card implicitly
			//   makes it shared (rather than a background).
			optr -> setflag(True, F_GROUP_SHARED);
		}
	}
	return ES_NORMAL;
#endif /* MCRemove */

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

void MCRemove::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (all)
	{
		MCSyntaxFactoryEvalConstantBool(ctxt, where == IP_FRONT);

		MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecRemoveAllScriptsFromMethodInfo);
	}
	else
	{
		target -> compile_object_ptr(ctxt);

		if (script)
		{
			MCSyntaxFactoryEvalConstantBool(ctxt, where == IP_FRONT);

			MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecRemoveScriptOfObjectFromMethodInfo);
		}
		else
		{
			card -> compile_object_ptr(ctxt);

			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecRemoveGroupFromCardMethodInfo);
		}
	}

	MCSyntaxFactoryEndStatement(ctxt);
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
#ifdef /* MCRename */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_RENAME_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	char *s = ep.getsvalue().clone();
	if (dest->eval(ep) != ES_NORMAL)
	{
		delete s;
		MCeerror->add
		(EE_RENAME_BADDEST, line, pos);
		return ES_ERROR;
	}
	char *d = ep.getsvalue().clone();
	if (!MCS_rename(s, d))
		MCresult->sets("can't rename file");
	else
		MCresult->clear(False);
	delete s;
	delete d;
	return ES_NORMAL; 
#endif /* MCRename */

    MCAutoStringRef t_from;
    if (!ctxt . EvalExprAsStringRef(source, EE_RENAME_BADSOURCE, &t_from))
        return;
    
    MCAutoStringRef t_to;
    if (!ctxt . EvalExprAsStringRef(dest, EE_RENAME_BADDEST, &t_to))
        return;
    
    MCFilesExecRename(ctxt, *t_from, *t_to);
}

void MCRename::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	source -> compile(ctxt);
	dest -> compile(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCFilesExecRenameMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
	container = new MCChunk(True);
	if (container->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add(PE_REPLACE_BADCONTAINER, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCReplace::exec_ctxt(MCExecContext& ctxt)
{
#ifdef /* MCReplace */ LEGACY_EXEC
	MCExecPoint epp(ep);
	MCExecPoint epr(ep);
	if (pattern->eval(epp) != ES_NORMAL || epp.tos() != ES_NORMAL || epp.getsvalue().getlength() < 1)
	{
		MCeerror->add(EE_REPLACE_BADPATTERN, line, pos);
		return ES_ERROR;
	}
	MCString pstring = epp.getsvalue();
	if (replacement->eval(epr) != ES_NORMAL || epr.tos() != ES_NORMAL)
	{
		MCeerror->add(EE_REPLACE_BADREPLACEMENT, line, pos);
		return ES_ERROR;
	}
	MCString rstring = epr.getsvalue();
	if (container->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_REPLACE_BADCONTAINER, line, pos);
		return ES_ERROR;
	}
	
	// If both pattern and replacement are both 1 char in length, we can use
	// the special case methods; otherwise we must use the general case.
	if (pstring.getlength() == 1 && rstring.getlength() == 1)
	{
		char *t_target;
		ep.grabsvalue();
		t_target = ep.getbuffer(0);
		// MW-2012-03-22: [[ Bug ]] char and uint8_t are different on some platforms
		//   causing problems so we revert to coercing to uint8_t.
		if (ep.getcasesensitive())
			replace_char_with_char((uint8_t *)t_target, ep.getsvalue().getlength(), (uint8_t)pstring.getstring()[0], (uint8_t)rstring.getstring()[0]);
		else
			replace_char_with_char_caseless((uint8_t *)t_target, ep.getsvalue().getlength(), (uint8_t)pstring.getstring()[0], (uint8_t)rstring.getstring()[0]);
	}
	else
	{
		char *t_output;
		uint32_t t_output_length;
		replace_general(ep.getsvalue(), pstring, rstring, ep.getcasesensitive() == True, t_output, t_output_length);
		ep.grabbuffer(t_output, t_output_length);
	}
	
	if (container->set(ep, PT_INTO) != ES_NORMAL)
	{
		MCeerror->add(EE_REPLACE_CANTSET, line, pos);
		return ES_ERROR;
	}
	
	return ES_NORMAL;
#endif /* MCReplace */

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
    
    
    MCAutoStringRef t_target;
    if (!ctxt . EvalExprAsMutableStringRef(container, EE_REPLACE_BADCONTAINER, &t_target))
        return;

    MCStringsExecReplace(ctxt, *t_pattern, *t_replacement, *t_target);
    
    if (ctxt . HasError())
        return;
    
    container -> set(ctxt, PT_INTO, *t_target);
}

void MCReplace::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	pattern -> compile(ctxt);
	replacement -> compile(ctxt);
	container -> compile_inout(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCStringsExecReplaceMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
}

void MCRevert::exec_ctxt(MCExecContext& ctxt)
{
#ifdef /* MCRevert */ LEGACY_EXEC
	if (MCtopstackptr != NULL)
	{
		Window_mode oldmode = MCtopstackptr->getmode();
		MCRectangle oldrect = MCtopstackptr->getrect();
		MCStack *sptr = MCtopstackptr;
		if (!MCdispatcher->ismainstack(sptr))
			sptr = (MCStack *)sptr->getparent();
		if (sptr == MCdispatcher->gethome())
		{
			MCeerror->add
			(EE_REVERT_HOME, line, pos);
			return ES_ERROR;
		}
		sptr->getprop(0, P_FILE_NAME, ep, False);
		ep.grabsvalue();
		Boolean oldlock = MClockmessages;
		MClockmessages = True;
		MCerrorlock++;
		if (sptr->del())
            sptr -> scheduledelete();
		MCerrorlock--;
		MClockmessages = oldlock;
		sptr = MCdispatcher->findstackname(ep.getsvalue());
		if (sptr != NULL)
			sptr->openrect(oldrect, oldmode, NULL, WP_DEFAULT, OP_NONE);
	}
	return ES_NORMAL;
#endif /* MCRevert */

	MCInterfaceExecRevert(ctxt);
}

void MCRevert::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecRevertMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
		image = new MCChunk(False);
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
#ifdef /* MCRotate */ LEGACY_EXEC
	// MW-2012-01-05: [[ Bug 9909 ]] If we are a mobile platform, the image
	//   editing operations are not supported yet.
#ifndef _MOBILE

	MCImage *iptr = nil;

	if (image != NULL)
	{
		MCObject *optr;
		uint4 parid;
		if (image->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_ROTATE_NOIMAGE, line, pos);
			return ES_ERROR;
		}
		if (optr->gettype() != CT_IMAGE || optr->getflag(F_HAS_FILENAME))
		{
			MCeerror->add(EE_ROTATE_NOTIMAGE, line, pos);
			return ES_ERROR;
		}
		iptr = (MCImage *)optr;
	}
	else
		iptr = MCactiveimage;

	if (angle->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ROTATE_BADANGLE, line, pos);
		return ES_ERROR;
	}

	int2 tangle = ep.getint4() % 360;
	if (tangle < 0)
		tangle += 360;

	if (tangle != 0 && iptr != NULL)
		iptr->rotatesel(tangle);

#endif

	return ES_NORMAL;
#endif /* MCRotate */
    
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

void MCRotate::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (image != nil)
	{
		image -> compile_object_ptr(ctxt);
		angle -> compile(ctxt);

		MCSyntaxFactoryExecMethod(ctxt, kMCGraphicsExecRotateImageMethodInfo);
	}
	else
	{
		angle -> compile(ctxt);

		MCSyntaxFactoryExecMethod(ctxt, kMCGraphicsExecRotateSelectionMethodInfo);
	}
	
	MCSyntaxFactoryEndStatement(ctxt);
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
		image = new MCChunk(False);
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
#ifdef /* MCCrop */ LEGACY_EXEC
	if (image != NULL)
	{
		MCObject *optr;
		uint4 parid;
		if (image->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CROP_NOIMAGE, line, pos);
			return ES_ERROR;
		}
		if (optr->gettype() != CT_IMAGE || optr->getflag(F_HAS_FILENAME))
		{
			MCeerror->add
			(EE_CROP_NOTIMAGE, line, pos);
			return ES_ERROR;
		}
		MCImage *iptr = (MCImage *)optr;
		MCRectangle trect;
		if (newrect->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CROP_CANTGETRECT, line, pos);
			return ES_ERROR;
		}
		int2 i1, i2, i3, i4;
		if (!MCU_stoi2x4(ep.getsvalue(), i1, i2, i3, i4))
		{
			MCeerror->add
			(EE_CROP_NAR, line, pos, ep.getsvalue());
			return ES_ERROR;
		}
		trect.x = i1;
		trect.y = i2;
		trect.width = MCU_max(i3 - i1, 1);
		trect.height = MCU_max(i4 - i2, 1);
		iptr->crop(&trect);
	}
	return ES_NORMAL;
#endif /* MCCrop */

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

void MCCrop::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (image != nil)
	{
		image -> compile_object_ptr(ctxt);
		newrect -> compile(ctxt);
	}
	else
	{
		MCSyntaxFactoryEvalConstantNil(ctxt);
		MCSyntaxFactoryEvalConstantLegacyRectangle(ctxt, MCRectangleMake(0,0,0,0));
	}

	MCSyntaxFactoryExecMethod(ctxt, kMCGraphicsExecCropImageMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
#ifdef /* MCSelect */ LEGACY_EXEC
	if (targets == NULL)
	{
		MCselected->clear(True);
		if (MCactivefield != NULL)
		{
			MCactivefield->unselect(False, True);
			if (MCactivefield != NULL)
				MCactivefield->getcard()->kunfocus();
		}
		return ES_NORMAL;
	}
	MCChunk *tptr = targets;
	while (tptr != NULL)
	{
		if (tptr->select(ep, where, text, tptr == targets) != ES_NORMAL)
		{
			MCeerror->add(EE_SELECT_BADTARGET, line, pos);
			return ES_ERROR;
		}
		tptr = tptr->next;
	}
	return ES_NORMAL;
#endif /* MCSelect */

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

void MCSelect::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (targets == NULL)
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecSelectEmptyMethodInfo);
	else if (text && where == PT_AT)
	{
		targets -> compile_object_ptr(ctxt);
		
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecSelectAllTextOfFieldMethodInfo);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecSelectAllTextOfButtonMethodInfo);
	}
	else 
	{
		MCSyntaxFactoryEvalConstantInt(ctxt, where);
		
		uindex_t t_count;
		t_count = 0;

        if (!text)
        {
            for (MCChunk *chunkptr = targets; chunkptr != nil; chunkptr = chunkptr -> next)
            {
                chunkptr -> compile_object_ptr(ctxt);
                t_count++;
            }
		}
        
		if (t_count > 1)
		{
			MCSyntaxFactoryEvalList(ctxt, t_count);
			MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCInterfaceExecSelectObjectsMethodInfo, 1);
		}
		else
		{
			MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCInterfaceExecSelectTextOfFieldMethodInfo, 0, 1);
			MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCInterfaceExecSelectTextOfButtonMethodInfo, 0, 1);
			MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCInterfaceExecSelectObjectsMethodInfo, 1);
		}
	}
}

void MCUndoCmd::exec_ctxt(MCExecContext& ctxt)
{
#ifdef /* MCUndoCmd */ LEGACY_EXEC
    MCundos->undo();
	return ES_NORMAL;
#endif /* MCUndoCmd */

    MCInterfaceExecUndo(ctxt);
}

void MCUndoCmd::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecUndoMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
	group = new MCChunk(False);
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
#ifdef /* MCUngroup */ LEGACY_EXEC
	MCObject *gptr;
	if (group != NULL)
	{
		uint4 parid;
		if (group->getobj(ep, gptr, parid, True) != ES_NORMAL)
		{
			MCeerror->add
			(EE_UNGROUP_NOGROUP, line, pos);
			return ES_ERROR;
		}
	}
	else
		gptr = MCselected->getfirst();
	if (gptr == NULL || gptr->gettype() != CT_GROUP)
	{
		MCeerror->add
		(EE_UNGROUP_NOTAGROUP, line, pos);
		return ES_ERROR;
	}
	gptr->getstack()->ungroup((MCGroup *)gptr);
	return ES_NORMAL;
#endif /* MCUngroup */

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

void MCUngroup::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (group != nil)
	{
		group -> compile_object_ptr(ctxt);	
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecUngroupObjectMethodInfo);
	}
	else
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecUngroupSelectionMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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

	control = new MCChunk(False);
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
		target = new MCChunk(False);
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
#ifdef /* MCRelayer */ LEGACY_EXEC
	// Fetch the source object.
	MCObject *t_source;
	uint32_t t_source_partid;
	if (control -> getobj(ep, t_source, t_source_partid, True) != ES_NORMAL)
	{
		MCeerror -> add(EE_RELAYER_NOSOURCE, line, pos);
		return ES_ERROR;
	}

	// Fetch the card of the source.
	MCCard *t_card;
	t_card = t_source -> getstack() -> getcard(t_source_partid);

	MCObject *t_target;
	uint32_t t_target_partid;
	switch(form)
	{
	case kMCRelayerFormRelativeToLayer:
		if (layer -> eval(ep) != ES_NORMAL)
		{
			MCeerror -> add(EE_RELAYER_BADLAYER, line, pos);
			return ES_ERROR;
		}
		if (ep . ton() != ES_NORMAL)
		{
			MCeerror -> add(EE_RELAYER_LAYERNAN, line, pos);
			return ES_ERROR;
		}
		t_target = t_card -> getobjbylayer(ep . getuint4());
		if (t_target == nil)
		{
			MCeerror -> add(EE_RELAYER_NOTARGET, line, pos);
			return ES_ERROR;
		}
		t_target_partid = t_source_partid;
		break;
	case kMCRelayerFormRelativeToControl:
		if (target -> getobj(ep, t_target, t_target_partid, True) != ES_NORMAL)
		{
			MCeerror -> add(EE_RELAYER_NOTARGET, line, pos);
			return ES_ERROR;
		}
		break;
	case kMCRelayerFormRelativeToOwner:
		t_target = t_source -> getparent();
		t_target_partid = t_source_partid;
		break;
	}

	// Ensure the source object is a control (group etc.)
	if (t_source -> gettype() < CT_GROUP)
	{
		MCeerror -> add(EE_RELAYER_SOURCENOTCONTROL, line, pos);
		return ES_ERROR;
	}

	// If the relation is front or back, then ensure the target is a card or
	// group.
	if ((relation == kMCRelayerRelationFront || relation == kMCRelayerRelationBack) &&
		(t_target -> gettype() != CT_CARD && t_target -> gettype() != CT_GROUP))
	{
		MCeerror -> add(EE_RELAYER_TARGETNOTCONTAINER, line, pos);
		return ES_ERROR;
	}

	// If the relation is before or after, make sure the target is a control.
	if ((relation == kMCRelayerRelationBefore || relation == kMCRelayerRelationAfter) &&
		t_target -> gettype() < CT_GROUP)
	{
		MCeerror -> add(EE_RELAYER_TARGETNOTCONTROL, line, pos);
		return ES_ERROR;
	}

	// Make sure source and target objects are on the same card.
	if (t_source -> getstack() != t_target -> getstack() ||
		t_card != t_target -> getstack() -> getcard(t_target_partid))
	{
		MCeerror -> add(EE_RELAYER_CARDNOTSAME, line, pos);
		return ES_ERROR;
	}

	// Next resolve everything in terms of before.
	MCObject *t_new_owner;
	MCControl *t_new_target;
	if (relation == kMCRelayerRelationFront || relation == kMCRelayerRelationBack)
	{
		// The new owner is just the target.
		t_new_owner = t_target;

		if (t_new_owner -> gettype() == CT_CARD)
		{
			MCObjptr *t_ptrs;
			t_ptrs = static_cast<MCCard *>(t_new_owner) -> getobjptrs();
			if (t_ptrs != nil && relation == kMCRelayerRelationBack)
				t_new_target = t_ptrs -> getref();
			else
				t_new_target = nil;
		}
		else
		{
			MCControl *t_controls;
			t_controls = static_cast<MCGroup *>(t_new_owner) -> getcontrols();
			if (t_controls != nil && relation == kMCRelayerRelationBack)
				t_new_target = t_controls;
			else
				t_new_target = nil;
		}
	}
	else
	{
		// The new owner is the owner of the target.
		t_new_owner = t_target -> getparent();

		if (t_new_owner -> gettype() == CT_CARD)
		{
			MCObjptr *t_target_objptr;
			t_target_objptr = t_card -> getobjptrforcontrol(static_cast<MCControl *>(t_target));

			MCObjptr *t_ptrs;
			t_ptrs = static_cast<MCCard *>(t_new_owner) -> getobjptrs();
			if (relation == kMCRelayerRelationAfter)
				t_new_target = t_target_objptr -> next() != t_ptrs ? t_target_objptr -> next() -> getref() : nil;
			else
				t_new_target = static_cast<MCControl *>(t_target);
		}
		else
		{
			MCControl *t_controls;
			t_controls = static_cast<MCGroup *>(t_new_owner) -> getcontrols();
			if (relation == kMCRelayerRelationAfter)
				t_new_target = t_target -> next() != t_controls ? static_cast<MCControl *>(t_target -> next()) : nil;
			else
				t_new_target = static_cast<MCControl *>(t_target);
		}
	}

	// At this point the operation is couched entirely in terms of new owner and
	// object that should follow this one - or nil if it should go at end of the
	// owner. We must now check that we aren't trying to put a control into a
	// descendent - i.e. that source is not an ancestor of new target.
	if (t_new_owner -> gettype() == CT_GROUP)
	{
		MCObject *t_ancestor;
		t_ancestor = t_new_owner -> getparent();
		while(t_ancestor -> gettype() != CT_CARD)
		{
			if (t_ancestor == t_source)
			{
				MCeerror -> add(EE_RELAYER_ILLEGALMOVE, line, pos);
				return ES_ERROR;
			}
			t_ancestor = t_ancestor -> getparent();
		}
	}

	// Perform the relayering.
	bool t_success;
	t_success = true;
	if (t_new_owner == t_source -> getparent())
	{
		t_new_owner -> relayercontrol(static_cast<MCControl *>(t_source), t_new_target);

		// MW-2013-04-29: [[ Bug 10861 ]] Make sure we trigger a property update as 'layer'
		//   is changing.
		t_source -> signallisteners(P_LAYER);

		if (t_card -> getstack() == MCmousestackptr && MCU_point_in_rect(t_source->getrect(), MCmousex, MCmousey))
			t_card -> mfocus(MCmousex, MCmousey);
	}
	else
	{
		// As we call handlers that might invoke messages, we need to take
		// object handles here.
		MCObjectHandle *t_source_handle, *t_new_owner_handle, *t_new_target_handle;
		t_source_handle = t_source -> gethandle();
		t_new_owner_handle = t_new_owner -> gethandle();
		t_new_target_handle = t_new_target != nil ? t_new_target -> gethandle() : nil;

		// Make sure we remove focus from the control.
		bool t_was_mfocused, t_was_kfocused;
		t_was_mfocused = t_card -> getstate(CS_MFOCUSED) == True;
		t_was_kfocused = t_card -> getstate(CS_KFOCUSED) == True;
		if (t_was_mfocused)
			t_card -> munfocus();
		if (t_was_kfocused)
			t_card -> kunfocus();

		// Check the source and new owner objects exist, and if we have a target object
		// that that exists and is still a child of new owner.
		if (t_source_handle -> Exists() &&
			t_new_owner_handle -> Exists() && 
			(t_new_target == nil || t_new_target_handle -> Exists() && t_new_target -> getparent() == t_new_owner))
		{
			t_source -> getparent() -> relayercontrol_remove(static_cast<MCControl *>(t_source));
			t_new_owner -> relayercontrol_insert(static_cast<MCControl *>(t_source), t_new_target);
			
			// MW-2013-04-29: [[ Bug 10861 ]] Make sure we trigger a property update as 'layer'
			//   is changing.
			t_source -> signallisteners(P_LAYER);
		}
		else
		{
			MCeerror -> add(EE_RELAYER_OBJECTSVANISHED, line, pos);
			t_success = false;
		}

		if (t_was_kfocused)
			t_card -> kfocus();
		if (t_was_mfocused)
			t_card -> mfocus(MCmousex, MCmousey);

		t_source_handle -> Release();
		t_new_owner_handle -> Release();
		if (t_new_target != nil)
			t_new_target_handle -> Release();
	}

	return t_success ? ES_NORMAL : ES_ERROR;
#endif /* MCRelayer */
    
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
