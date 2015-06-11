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

#include "core.h"
#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "scriptpt.h"
#include "execpt.h"
#include "param.h"
#include "handler.h"
#include "sellst.h"
#include "undolst.h"
#include "chunk.h"
#include "object.h"
#include "control.h"
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
#include "objptr.h"
#include "stacksecurity.h"

MCClone::~MCClone()
{
	delete source;
	delete newname;
	delete it;
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
	getit(sp, it);
	return PS_NORMAL;
}

// MW-2004-11-17: New version is too restrictive. Semantics should be:
//   clone a stack - always succeeds
//   clone a card of stack -
//     if target is this stack then only if not locked
//     if target is not this stack then only if keyed and target valid
//   clone an object of a stack - only if not locked
// where a target is valid only if it is not locked and not protected
Exec_stat MCClone::exec(MCExecPoint &ep)
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
	it->set
	(ep);
	MCdefaultstackptr = odefaultstackptr;
	return ES_NORMAL;
#endif /* MCClone */
}

///////////////////////////////////////////////////////////////////////////////

MCClipboardCmd::~MCClipboardCmd(void)
{
	deletetargets(&targets);
	delete dest;
	delete it;
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
		getit(sp, it);
	}
	return PS_NORMAL;
}

Exec_stat MCClipboardCmd::exec(MCExecPoint& ep)
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
		if (t_error == EE_UNDEFINED && dest != NULL)
			t_error = processtocontainer(t_objects, t_object_count, t_dst_object);
		else if (t_error == EE_UNDEFINED)
			t_error = processtoclipboard(t_objects, t_object_count);
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
}
#ifdef /* MCClipboardCmd::processtocontainer */ LEGACY_EXEC
Exec_errors MCClipboardCmd::processtocontainer(MCObjectRef *p_objects, uint4 p_object_count, MCObject *p_dst)
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
		MCExecPoint ep(NULL, NULL, NULL);
		t_new_object -> getprop(0, P_LONG_ID, ep, False);
		it -> set(ep);
	}

	return EE_UNDEFINED;
}
#endif /* MCClipboardCmd::processtocontainer */ 
#ifdef /* MCClipboardCmd::processtoclipboard */ LEGACY_EXEC
Exec_errors MCClipboardCmd::processtoclipboard(MCObjectRef *p_objects, uint4 p_object_count)
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
                    p_objects[i] . object -> scheduledelete();
			}
		}
	}
	else
		MCresult -> sets("unable to write to clipboard");

	if (t_pickle != NULL)
		t_pickle -> Release();

	return EE_UNDEFINED;
}
#endif /* MCClipboardCmd::processtoclipboard */

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
	delete newname;
	delete file;
	delete container;
	delete it;
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
		getit(sp, it);
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
        
        getit(sp, it);
        
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
	else if (sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL ||
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
	return PS_NORMAL;
}

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
		return MCtemplateplayer;
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

Exec_stat MCCreate::exec(MCExecPoint &ep)
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
	it->set(ep);
	return ES_NORMAL;
#endif /* MCCreate */
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

Exec_stat MCCustomProp::exec(MCExecPoint &ep)
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
Exec_stat MCDelete::exec(MCExecPoint &ep)
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

Exec_stat MCChangeProp::exec(MCExecPoint &ep)
{
#ifdef /* MCChangeProp */ LEGACY_EXEC
	return targets->changeprop(ep, prop, value);
#endif /* MCChangeProp */
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
	if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_IMAGE) == PS_NORMAL)
	{
		sp.backup();
		image = new MCChunk(False);
		if (image->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_FLIP_BADIMAGE, sp);
			return PS_ERROR;
		}
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
Exec_stat MCFlip::exec(MCExecPoint &ep)
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

Exec_stat MCGrab::exec(MCExecPoint &ep)
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
}

MCLaunch::~MCLaunch()
{
	delete doc;
	delete app;
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

	return PS_NORMAL;
}

Exec_stat MCLaunch::exec(MCExecPoint &ep)
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
}

MCLoad::~MCLoad()
{
	delete url;
	delete message;
}

Parse_stat MCLoad::parse(MCScriptPoint &sp)
{
	initpoint(sp);

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
	return PS_NORMAL;
}

Exec_stat MCLoad::exec(MCExecPoint &ep)
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
}

MCUnload::~MCUnload()
{
	delete url;
}

Parse_stat MCUnload::parse(MCScriptPoint &sp)
{
	initpoint(sp);

	sp.skip_token(SP_FACTOR, TT_CHUNK, CT_URL);

	if (sp.parseexp(False, True, &url) != PS_NORMAL)
	{
		MCperror->add(PE_UNLOAD_BADURLEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCUnload::exec(MCExecPoint &ep)
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
}

MCPost::~MCPost()
{
	delete source;
	delete dest;
	delete it;
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
	getit(sp, it);
	return PS_NORMAL;
}

Exec_stat MCPost::exec(MCExecPoint &ep)
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
	return it->set(ep);
#endif /* MCPost */
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

Exec_stat MCMakeGroup::exec(MCExecPoint &ep)
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
			if (optr->gettype() > CT_FIELD ||
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
}

MCPasteCmd::~MCPasteCmd()
{
	delete it;
}

Parse_stat MCPasteCmd::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	getit(sp, it);
	return PS_NORMAL;
}

Exec_stat MCPasteCmd::exec(MCExecPoint &ep)
{
#ifdef /* MCPasteCmd */ LEGACY_EXEC
	MCObject *optr;
	if (!MCdispatcher -> dopaste(optr, true))
		MCresult->sets("can't paste (empty clipboard or locked destination)");
	else
		if (optr != NULL)
		{
			optr->getprop(0, P_LONG_ID, ep, False);
			it->set(ep);
		}
	return ES_NORMAL;
#endif /* MCPasteCmd */
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

Exec_stat MCPlace::exec(MCExecPoint &ep)
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

Exec_stat MCRecord::exec(MCExecPoint &ep)
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
}

Exec_stat MCRedo::exec(MCExecPoint &ep)
{
	return ES_NORMAL;
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

Exec_stat MCRemove::exec(MCExecPoint &ep)
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

Exec_stat MCRename::exec(MCExecPoint &ep)
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
}

////////////////////////////////////////////////////////////////////////////////

// MW-2012-02-01: [[ Bug 9647 ]] New implementation of replace to fix the flaws
//   in the old implementation.
// MW-2012-03-22: [[ Bug ]] Use uint8_t rather than char, otherwise we get
//   comparison issues on some platforms (where char is signed!).

// Special case replacing a char with another char when case-sensitive is true.
static void replace_char_with_char(uint8_t *p_chars, uindex_t p_char_count, uint8_t p_from, uint8_t p_to)
{
	// Simplest case, just substitute from for to.
	for(uindex_t i = 0; i < p_char_count; i++)
		if (p_chars[i] == p_from)
			p_chars[i] = p_to;
}

// Special case replacing a char with another char when case-sensitive is false.
static void replace_char_with_char_caseless(uint8_t *p_chars, uindex_t p_char_count, uint8_t p_from, uint8_t p_to)
{
	// Lowercase the from char.
	p_from = MCS_tolower(p_from);
	
	// Now substitute from for to, taking making sure its a caseless compare.
	for(uindex_t i = 0; i < p_char_count; i++)
		if (MCS_tolower(p_chars[i]) == p_from)
			p_chars[i] = p_to;
}

// General replace case, rebuilds the input string in 'output' replacing each
// occurance of from with to.
static bool replace_general(const MCString& p_input, const MCString& p_from, const MCString& p_to, bool p_caseless, char*& r_output, uindex_t& r_output_length)
{
	char *t_output;
	uindex_t t_output_length;
	uindex_t t_output_capacity;
	t_output = nil;
	t_output_length = 0;
	t_output_capacity = 0;
	
	MCString t_whole;
	t_whole = p_input;
	
	for(;;)
	{
		// Search for the next occurance of from in whole.
		Boolean t_found;
		uindex_t t_offset;
		t_found = MCU_offset(p_from, t_whole, t_offset, p_caseless);
		
		// If we found an instance of from, then we need space for to; otherwise,
		// we update the offset, and need just room up to it.
		uindex_t t_space_needed;
		if (t_found)
			t_space_needed = t_offset + p_to . getlength();
		else
		{
			t_offset = t_whole . getlength();
			t_space_needed = t_offset;
		}
		
		// Expand the buffer as necessary.
		if (t_output_length + t_space_needed > t_output_capacity)
		{
			if (t_output_capacity == 0)
				t_output_capacity = 4096;
				
			while(t_output_length + t_space_needed > t_output_capacity)
				t_output_capacity *= 2;
			
			if (!MCMemoryReallocate(t_output, t_output_capacity, t_output))
			{
				MCMemoryDeallocate(t_output);
				return false;
			}
		}
			
		// Copy in whole, up to the offset.
		memcpy(t_output + t_output_length, t_whole . getstring(), t_offset);
		t_output_length += t_offset;

		// No more occurances were found, so we are done.
		if (!t_found)
			break;
			
		// Now copy in to.
		memcpy(t_output + t_output_length, p_to . getstring(), p_to . getlength());
		t_output_length += p_to . getlength();
		
		// Update whole.
		t_whole . set(t_whole . getstring() + t_offset + p_from . getlength(), t_whole . getlength() - (t_offset + p_from . getlength()));
	}
	
	// Make sure the buffer is no bigger than is needed.
	MCMemoryReallocate(t_output, t_output_length, r_output);
	r_output_length = t_output_length;
	
	return true;
}

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

Exec_stat MCReplace::exec(MCExecPoint &ep)
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
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCRevert::exec(MCExecPoint &ep)
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

Exec_stat MCRotate::exec(MCExecPoint &ep)
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

Exec_stat MCCrop::exec(MCExecPoint &ep)
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
		if (sp.gettoken() == "empty")
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

Exec_stat MCSelect::exec(MCExecPoint &ep)
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
}

Exec_stat MCUndoCmd::exec(MCExecPoint &ep)
{
#ifdef /* MCUndoCmd */ LEGACY_EXEC
	MCundos->undo();
	return ES_NORMAL;
#endif /* MCUndoCmd */
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

Exec_stat MCUngroup::exec(MCExecPoint &ep)
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
}

////////////////////////////////////////////////////////////////////////////////

MCRelayer::MCRelayer(void)
{
	form = kMCRelayerFormNone;
	relation = kMCRelayerRelationNone;
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
			relation = kMCRelayerRelationFront;
		else if (sp . skip_token(SP_INSERT, TT_UNDEFINED, IP_BACK) == PS_NORMAL)
			relation = kMCRelayerRelationBack;
		else
		{
			MCperror -> add(PE_RELAYER_BADRELATION, sp);
			return PS_ERROR;
		}
	}
	else if (sp . skip_token(SP_FACTOR, TT_PREP, PT_BEFORE) == PS_NORMAL)
		relation = kMCRelayerRelationBefore;
	else if (sp . skip_token(SP_FACTOR, TT_PREP, PT_AFTER) == PS_NORMAL)
		relation = kMCRelayerRelationAfter;

	if (relation == kMCRelayerRelationFront ||
		relation == kMCRelayerRelationBack)
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

Exec_stat MCRelayer::exec(MCExecPoint& ep)
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
		t_was_mfocused = t_card -> getstate(CS_MFOCUSED);
		t_was_kfocused = t_card -> getstate(CS_KFOCUSED);
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
}

////////////////////////////////////////////////////////////////////////////////
