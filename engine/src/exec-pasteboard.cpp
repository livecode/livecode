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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "field.h"
#include "stack.h"
#include "card.h"

#include "exec.h"
#include "dispatch.h"
#include "image.h"
#include "stacklst.h"
#include "sellst.h"
#include "chunk.h"

#include "raw-clipboard.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, Clipboard, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, ClipboardKeys, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, RawClipboardKeys, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, DropChunk, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, DragDestination, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, DragSource, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, DragDropKeys, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, IsAmongTheKeysOfTheClipboardData, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, IsNotAmongTheKeysOfTheClipboardData, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, IsAmongTheKeysOfTheRawClipboardData, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, IsNotAmongTheKeysOfTheRawClipboardData, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, IsAmongTheKeysOfTheDragData, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, IsNotAmongTheKeysOfTheDragData, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, DragSourceAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, DragDestinationAsObject, 1)

MC_EXEC_DEFINE_EXEC_METHOD(Pasteboard, Paste, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Pasteboard, Copy, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Pasteboard, CopyTextToClipboard, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Pasteboard, CopyObjectsToClipboard, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Pasteboard, Cut, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Pasteboard, CutTextToClipboard, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Pasteboard, CutObjectsToClipboard, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Pasteboard, LockClipboard, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Pasteboard, UnlockClipboard, 0)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, AcceptDrop, 1)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, AcceptDrop, 1)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, DragAction, 1)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, DragAction, 1)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, DragImage, 1)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, DragImage, 1)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, DragImageOffset, 1)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, DragImageOffset, 1)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, AllowableDragActions, 1)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, AllowableDragActions, 1)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, ClipboardData, 2)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, ClipboardData, 2)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, RawClipboardData, 2)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, RawClipboardData, 2)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, DragData, 2)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, DragData, 2)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, ClipboardOrDragData, 2)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, ClipboardOrDragData, 2)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, ClipboardTextData, 2)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, ClipboardTextData, 2)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, RawClipboardTextData, 2)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, RawClipboardTextData, 2)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, DragTextData, 2)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, DragTextData, 2)

////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  Enumeration:
//    MCTransferType
//
//  Type:
//    Platform independent enumeration
//
//  Description:
//    The MCTransferType enumeration describes the type of data that is being
//    passed around in the transfer mechanism.
//
//    The types are as follows:
//      - TEXT: a string of single byte per character characters in native text
//        encoding (ISO8859-1 on UNIX, MacRoman on Mac OS X, CP-1252 on Windows)
//      - UNICODE_TEXT: a string of UTF-16 codepoints in native byte-order
//      - STYLED_TEXT: a LiveCode styled-text pickle
//      - RTF_TEXT: a string of single byte per character characters in native
//        encoding describing text formatted as RTF. Note this format can also
//        contain unicode characters though the appropriate RTF escaping
//        mechanism.
//      - HTML_TEXT: a string of single byte per character characters in native
//        encoding, describing text formatted as RHTML. Note this format can
//        also contain unicode characters through the appropriate RTF escaping
//        mechanism.
//      - IMAGE: a binary data stream containing a PNG, JPEG or GIF image.
//      - FILES: a return-separated list of files in LiveCode path format
//        encoded in native text encoding.
//      - PRIVATE: a binary data stream
//      - OBJECTS: a sequence of LiveCode object pickles.
//
//    These types are classified into the following classes:
//      - text: TEXT, UNICODE_TEXT, STYLED_TEXT, RTF_TEXT, HTML_TEXT
//      - image: IMAGE
//      - files: FILES
//      - private: PRIVATE
//      - objects: OBJECTS
//
//    The RTF_TEXT and HTML_TEXT types are considered to be part of an
//    additional class 'derived'.
//
//    The derived types and PRIVATE type are never passed to a method of
//    MCScreenDC and as such lower-level interfaces need not take them into
//    account.
//
enum MCTransferType
{
    TRANSFER_TYPE_NULL,
    TRANSFER_TYPE_TEXT,
    
    TRANSFER_TYPE_TEXT__FIRST = TRANSFER_TYPE_TEXT,
    TRANSFER_TYPE_UNICODE_TEXT,
    TRANSFER_TYPE_STYLED_TEXT,
    TRANSFER_TYPE_STYLED_TEXT_ARRAY,
    TRANSFER_TYPE_RTF_TEXT,
    TRANSFER_TYPE_HTML_TEXT,
    TRANSFER_TYPE_TEXT__LAST = TRANSFER_TYPE_HTML_TEXT,
    
    TRANSFER_TYPE_IMAGE,
    TRANSFER_TYPE_FILES,
    TRANSFER_TYPE_PRIVATE,
    TRANSFER_TYPE_OBJECTS
    
};

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCPasteboardDragActionElementInfo[] =
{
	{ "none", DRAG_ACTION_NONE, false },
	{ "move", DRAG_ACTION_MOVE, false },
	{ "copy", DRAG_ACTION_COPY, false },
	{ "link", DRAG_ACTION_LINK, false },
};

static MCExecEnumTypeInfo _kMCPasteboardDragActionTypeInfo =
{
	"Pasteboard.DragAction",
	sizeof(_kMCPasteboardDragActionElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCPasteboardDragActionElementInfo
};

//////////

static MCExecSetTypeElementInfo _kMCPasteboardAllowableDragActionsElementInfo[] =
{
	{ "move", DRAG_ACTION_MOVE_BIT },
	{ "copy", DRAG_ACTION_COPY_BIT },
	{ "link", DRAG_ACTION_LINK_BIT },
};

static MCExecSetTypeInfo _kMCPasteboardAllowableDragActionsTypeInfo =
{
	"Pasteboard.AllowableDragActions",
	sizeof(_kMCPasteboardAllowableDragActionsElementInfo) / sizeof(MCExecSetTypeElementInfo),
	_kMCPasteboardAllowableDragActionsElementInfo
};

//////////

MCExecEnumTypeInfo *kMCPasteboardDragActionTypeInfo = &_kMCPasteboardDragActionTypeInfo;
MCExecSetTypeInfo *kMCPasteboardAllowableDragActionsTypeInfo = &_kMCPasteboardAllowableDragActionsTypeInfo;

////////////////////////////////////////////////////////////////////////////////

MCTransferType MCPasteboardTransferTypeFromName(MCNameRef p_key)
{
	if (MCNameIsEqualTo(p_key, MCN_text))
		return TRANSFER_TYPE_TEXT;

	if (MCNameIsEqualTo(p_key, MCN_unicode))
		return TRANSFER_TYPE_UNICODE_TEXT;

	if (MCNameIsEqualTo(p_key, MCN_styles))
		return TRANSFER_TYPE_STYLED_TEXT;

	if (MCNameIsEqualTo(p_key, MCN_rtf))
		return TRANSFER_TYPE_RTF_TEXT;

	if (MCNameIsEqualTo(p_key, MCN_html))
		return TRANSFER_TYPE_HTML_TEXT;

	if (MCNameIsEqualTo(p_key, MCN_files))
		return TRANSFER_TYPE_FILES;

	if (MCNameIsEqualTo(p_key, MCN_private))
		return TRANSFER_TYPE_PRIVATE;

	if (MCNameIsEqualTo(p_key, MCN_image))
		return TRANSFER_TYPE_IMAGE;

	if (MCNameIsEqualTo(p_key, MCN_objects))
		return TRANSFER_TYPE_OBJECTS;

	return TRANSFER_TYPE_NULL;
}

MCNameRef MCPasteboardTransferTypeToName(MCTransferType p_type)
{
	switch(p_type)
	{
	case TRANSFER_TYPE_TEXT:
		return MCN_text;
	case TRANSFER_TYPE_UNICODE_TEXT:
		return MCN_unicode;
	case TRANSFER_TYPE_STYLED_TEXT:
		return MCN_styles;
	case TRANSFER_TYPE_RTF_TEXT:
		return MCN_rtf;
	case TRANSFER_TYPE_HTML_TEXT:
		return MCN_html;
	case TRANSFER_TYPE_IMAGE:
		return MCN_image;
	case TRANSFER_TYPE_FILES:
		return MCN_files;
	case TRANSFER_TYPE_PRIVATE:
		return MCN_private;
	case TRANSFER_TYPE_OBJECTS:
		return MCN_objects;
    case TRANSFER_TYPE_NULL:
    case TRANSFER_TYPE_STYLED_TEXT_ARRAY:
    default:
        break;
	}

	return kMCEmptyName;
}

bool MCPasteboardListKeys(MCClipboard *p_clipboard, char_t p_delimiter, MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable(p_delimiter, &t_list))
		return false;

    // Ensure the clipboard stays consistent while we query it
	if (!p_clipboard->Lock())
		return false;

	bool t_success = true;

    // Does the clipboard have plain text or equivalents?
    if (t_success && p_clipboard->HasTextOrCompatible())
        t_success = MCListAppend(*t_list, MCN_text) && MCListAppend(*t_list, MCN_unicode);
    
    // Does the clipboard have styled text or equivalents?
    // TODO: why isn't HTML on this list?
    if (t_success && p_clipboard->HasLiveCodeStyledTextOrCompatible())
        t_success = MCListAppend(*t_list, MCN_styles)
                    && MCListAppend(*t_list, MCN_rtf)
                    && MCListAppend(*t_list, MCN_text)
                    && MCListAppend(*t_list, MCN_unicode);
    
    // Does the clipboard contain an image recognised by LiveCode?
    if (t_success && p_clipboard->HasImage())
        t_success = MCListAppend(*t_list, MCN_image);
    
    // Does the clipboard contain a list of files?
    if (t_success && p_clipboard->HasFileList())
        t_success = MCListAppend(*t_list, MCN_files);
    
    // Does the clipboard contain serialised LiveCode objects?
    if (t_success && p_clipboard->HasLiveCodeObjects())
        t_success = MCListAppend(*t_list, MCN_objects);

    // Unlock the clipboard.
	p_clipboard->Unlock();

	if (t_success)
		t_success = MCListCopy(*t_list, r_list);

	return t_success;
}

void MCPasteboardEvalIsAmongTheKeysOf(MCExecContext& ctxt, MCClipboard* p_clipboard, MCNameRef p_key, bool& r_result)
{
    // Turn the array index into a tranfer type
    MCTransferType t_type = MCPasteboardTransferTypeFromName(p_key);
    
    // Is this type present in the array?
    switch (t_type)
    {
        case TRANSFER_TYPE_TEXT:
        case TRANSFER_TYPE_UNICODE_TEXT:
            r_result = p_clipboard->HasTextOrCompatible();
            break;

        case TRANSFER_TYPE_STYLED_TEXT:
        case TRANSFER_TYPE_RTF_TEXT:
        case TRANSFER_TYPE_HTML_TEXT:
            r_result = p_clipboard->HasLiveCodeStyledTextOrCompatible();
            break;
            
        case TRANSFER_TYPE_IMAGE:
            r_result = p_clipboard->HasImage();
            break;
            
        case TRANSFER_TYPE_FILES:
            r_result = p_clipboard->HasFileList();
            break;
            
        case TRANSFER_TYPE_OBJECTS:
            r_result = p_clipboard->HasLiveCodeObjects();
            break;
            
        case TRANSFER_TYPE_PRIVATE:
            r_result = p_clipboard->HasPrivateData();
            break;
            
        case TRANSFER_TYPE_NULL:
        case TRANSFER_TYPE_STYLED_TEXT_ARRAY:
        default:
            r_result = false;
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalClipboard(MCExecContext& ctxt, MCNameRef& r_type)
{
    // We need to ensure that the clipboard contents are consistent while
    // checking for multiple data types
    if (MCclipboard->Lock())
    {
        // Only one data type can be returned by this function. The order of
        // these is fixed and cannot be changed without breaking compatibility.
        MCNameRef t_format;
        if (MCclipboard->HasFileList())
            t_format = MCN_files;
        else if (MCclipboard->HasLiveCodeObjects())
            t_format = MCN_objects;
        else if (MCclipboard->HasImage())
            t_format=  MCN_image;
        else if (MCclipboard->HasTextOrCompatible())
            t_format = MCN_text;
        else if (MCclipboard->HasPrivateData())
            t_format = MCN_private;
        else
            t_format = MCN_empty;
        
        // Unlock the clipboard.
        //
        // Unfortunately, this introduces a race condition -- by the time a
        // script can make use of this information, the clipboard contents
        // could have changed.
        MCclipboard->Unlock();
        r_type = MCValueRetain(t_format);
    }
	else
	{
        // Could not gain access to the clipboard.
        r_type = MCValueRetain(kMCEmptyName);
		ctxt . SetTheResultToCString("unable to access clipboard");
	}
}

void MCPasteboardEvalClipboardKeys(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (!MCPasteboardListKeys(MCclipboard, '\n', &t_list))
	{
		ctxt . SetTheResultToCString("unable to query clipboard");
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}
	if (MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalRawClipboardKeys(MCExecContext& ctxt, MCStringRef& r_string)
{
    // Ensure there is an active script clipboard
    if (MCscriptrawclipboard == NULL)
    {
        ctxt.LegacyThrow(EE_RAW_CLIPBOARD_NOT_LOCKED);
        return;
    }
    
    // TODO: support multiple items
    // Get the first item on the clipboard
    MCAutoRefcounted<const MCRawClipboardItem> t_item = MCscriptrawclipboard->GetItemAtIndex(0);
    if (t_item == NULL)
    {
        r_string = kMCEmptyString;
        MCValueRetain(kMCEmptyString);
    }
    else
    {
        MCAutoListRef t_list;
        /* UNCHECKED */ MCListCreateMutable('\n', &t_list);
        
        uindex_t t_type_count = t_item->GetRepresentationCount();
        for (uindex_t i = 0; i < t_type_count; i++)
        {
            MCAutoStringRef t_type(t_item->FetchRepresentationAtIndex(i)->CopyTypeString());
            /* UNCHECKED */ MCListAppend(*t_list, *t_type);
        }
        
        /* UNCHECKED */ MCListCopyAsString(*t_list, r_string);
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalDropChunk(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCdropfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCdropfield -> returnchunk(MCdropchar, MCdropchar, r_string))
		return;

	ctxt . Throw();
}

void MCPasteboardEvalDragDestination(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCdragdest == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	MCdragdest->getstringprop(ctxt, 0, P_LONG_ID, False, r_string);
}

void MCPasteboardEvalDragSource(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCdragsource == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	MCdragsource->getstringprop(ctxt, 0, P_LONG_ID, False, r_string);
}

void MCPasteboardEvalDragDropKeys(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (!MCPasteboardListKeys(MCdragboard, '\n', &t_list))
	{
		ctxt . SetTheResultToCString("unable to query clipboard");
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}
	if (MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalIsAmongTheKeysOfTheClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    MCPasteboardEvalIsAmongTheKeysOf(ctxt, MCclipboard, p_key, r_result);
}

void MCPasteboardEvalIsNotAmongTheKeysOfTheClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
	MCPasteboardEvalIsAmongTheKeysOfTheClipboardData(ctxt, p_key, r_result);
	r_result = !r_result;
}

//////////

void MCPasteboardEvalIsAmongTheKeysOfTheRawClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    // Ensure there is an active script clipboard
    if (MCscriptrawclipboard == NULL)
    {
        ctxt.LegacyThrow(EE_RAW_CLIPBOARD_NOT_LOCKED);
        return;
    }
    
    // TODO: support multiple items
    // Get the first item on the clipboard
    MCAutoRefcounted<const MCRawClipboardItem> t_item = MCscriptrawclipboard->GetItemAtIndex(0);
    if (t_item == NULL)
    {
        // Clipboard is empty so the key is not present
        r_result = false;
    }
    else
    {
        // Check whether the key is a valid representation for this item
        r_result = t_item->HasRepresentation(MCNameGetString(p_key));
    }
}

void MCPasteboardEvalIsNotAmongTheKeysOfTheRawClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    // Inverse of the result from "is among the keys of"
    MCPasteboardEvalIsAmongTheKeysOfTheRawClipboardData(ctxt, p_key, r_result);
    if (!ctxt.HasError())
        r_result = !r_result;
}

//////////

void MCPasteboardEvalIsAmongTheKeysOfTheDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    MCPasteboardEvalIsAmongTheKeysOf(ctxt, MCdragboard, p_key, r_result);
}

void MCPasteboardEvalIsNotAmongTheKeysOfTheDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
	MCPasteboardEvalIsAmongTheKeysOfTheClipboardData(ctxt, p_key, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardExecPaste(MCExecContext& ctxt)
{
	MCObject *p_object;
	if (!MCdispatcher -> dopaste(p_object, true))
	{
		ctxt . SetTheResultToStaticCString("can't paste (empty clipboard or locked destination)");
		return;
	}
	else
		if (p_object == NULL)
			return;
		else
		{
			MCAutoValueRef t_target;
			if (p_object->names(P_LONG_ID, &t_target))
			{
				ctxt . SetItToValue(*t_target);
				return;
			}
		}
	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardProcessToClipboard(MCExecContext& ctxt, MCObjectPtr *p_targets, uindex_t p_object_count, bool p_cut)
{
	// Pickle the list of objects. The only reason this could fail is due to lack of
	// memory.
	MCPickleContext *t_context;

	// AL-2014-02-14: [[ UnicodeFileFormat ]] When pickling for the clipboard, make sure it
	//   includes 2.7, 5.5 and 7.0 stackfile formats.
	t_context = MCObject::startpickling(true);
	if (t_context == NULL)
	{
		ctxt . LegacyThrow(EE_NO_MEMORY);
		return;
	}

	for(uindex_t i = 0; i < p_object_count; ++i)
		MCObject::continuepickling(t_context, p_targets[i] . object, p_targets[i] . part_id);

	MCAutoDataRef t_pickle;
	MCObject::stoppickling(t_context, &t_pickle);
	if (*t_pickle == nil)
	{
		ctxt . LegacyThrow(EE_NO_MEMORY);
		return;
	}

	bool t_success;
	t_success = true;

	// Open the clipboard ready for storing data
    bool t_clipboard_locked = false;
	if (t_success)
        t_success = t_clipboard_locked = MCclipboard->Lock();

	// Attempt to store the objects onto the clipboard
	if (t_success)
        t_success = MCclipboard->AddLiveCodeObjects(*t_pickle);

	// If we've managed to store objects, and we are only copying/cutting one
	// image object, attempt to copy the image data.
	// Note that we don't care if this doesn't succeed as its not critical.
	if (t_success)
		if (p_object_count == 1 && p_targets[0] . object -> gettype() == CT_IMAGE)
		{
			MCAutoDataRef t_image_data;
			if (static_cast<MCImage *>(p_targets[0] . object) -> getclipboardtext(&t_image_data))
                /* UNCHECKED */ MCclipboard->AddImage(*t_image_data);
		}

	// Close the clipboard. If this returns false, then it means the
	// writing was unsuccessful.
	if (t_clipboard_locked)
        t_success = MCclipboard->Unlock();

	// If all is well, delete the original objects.
	if (t_success)
	{
		if (p_cut)
		{
			for(uint4 i = 0; i < p_object_count; ++i)
			{
				if (p_targets[i] . object -> del())
                {
                    if (p_targets[i] . object -> gettype() == CT_STACK)
                        MCtodestroy -> remove(static_cast<MCStack *>(p_targets[i] . object));
                    p_targets[i] . object -> scheduledelete();
                }
			}
		}
	}
	else
		ctxt . SetTheResultToStaticCString("unable to write to clipboard");
}

void MCPasteboardProcessTextToClipboard(MCExecContext &ctxt, MCObjectChunkPtr p_target, bool p_cut)
{
    MCField *t_field;
    t_field = static_cast<MCField *>(p_target . object);
    
    findex_t t_si, t_ei;
    // AL-2014-07-14: [[ Bug 12777 ]] t_ei is end index, not length of chunk.
    t_si = p_target . mark . start;
    t_ei = p_target . mark . finish;
    
	if (p_cut)
		t_field -> cuttextindex(p_target . part_id, t_si, t_ei);
	else
		t_field -> copytextindex(p_target . part_id, t_si, t_ei);
}

void MCPasteboardExecCopy(MCExecContext& ctxt)
{
	if (MCactivefield != NULL)
		MCactivefield -> copytext();
	else if (MCactiveimage != NULL)
		MCactiveimage -> copyimage();
	else
		MCselected -> copy();
}

void MCPasteboardExecCopyTextToClipboard(MCExecContext& ctxt, MCObjectChunkPtr p_target)
{
	MCPasteboardProcessTextToClipboard(ctxt, p_target, false);
}

void MCPasteboardExecCopyObjectsToClipboard(MCExecContext& ctxt, MCObjectPtr *p_targets, uindex_t p_target_count)
{
	MCPasteboardProcessToClipboard(ctxt, p_targets, p_target_count, false);
}

void MCPasteboardExecCut(MCExecContext& ctxt)
{
	if (MCactivefield != NULL)
		MCactivefield -> cuttext();
	else if (MCactiveimage != NULL)
		MCactiveimage -> cutimage();
	else
		MCselected -> cut();
}

void MCPasteboardExecCutTextToClipboard(MCExecContext& ctxt, MCObjectChunkPtr p_target)
{
	MCPasteboardProcessTextToClipboard(ctxt, p_target, true);
}

void MCPasteboardExecCutObjectsToClipboard(MCExecContext& ctxt, MCObjectPtr *p_targets, uindex_t p_target_count)
{
	MCPasteboardProcessToClipboard(ctxt, p_targets, p_target_count, true);
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardGetAcceptDrop(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCdragaction != DRAG_ACTION_NONE;
}

void MCPasteboardSetAcceptDrop(MCExecContext& ctxt, bool p_value)
{
	MCdragaction = p_value ? DRAG_ACTION_COPY : DRAG_ACTION_NONE;
}

void MCPasteboardGetDragAction(MCExecContext& ctxt, intenum_t& r_value)
{
	r_value = (intenum_t)MCdragaction;
}

void MCPasteboardSetDragAction(MCExecContext& ctxt, intenum_t p_value)
{
	MCdragaction = (MCDragAction)p_value;
}

void MCPasteboardGetDragImage(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCdragimageid;
}

void MCPasteboardSetDragImage(MCExecContext& ctxt, uinteger_t p_value)
{
	MCdragimageid = p_value;
}

void MCPasteboardGetDragImageOffset(MCExecContext& ctxt, MCPoint*& r_value)
{
	if (MCdragimageid != 0)
		r_value = &MCdragimageoffset;

	r_value = MCdragimageid != 0 ? &MCdragimageoffset : NULL;
}

void MCPasteboardSetDragImageOffset(MCExecContext& ctxt, MCPoint *p_value)
{
    if (p_value != nil)
        MCdragimageoffset = *p_value;
}

void MCPasteboardGetAllowableDragActions(MCExecContext& ctxt, intset_t& r_value)
{
	r_value = MCallowabledragactions;
}

void MCPasteboardSetAllowableDragActions(MCExecContext& ctxt, intset_t p_value)
{
	MCallowabledragactions = (MCDragActionSet)p_value;
}

void MCPasteboardGetClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_index, bool p_is_clipboard, MCValueRef& r_data)
{
    // Clipboard or drag board?
    MCClipboard* t_clipboard;
	if (p_is_clipboard)
		t_clipboard = MCclipboard;
	else
		t_clipboard = MCdragboard;

	bool t_success;
	t_success = true;
    
    // Lock the clipboard so it doesn't change while we're querying it.
    bool t_clipboard_locked;
    t_success = t_clipboard_locked = t_clipboard->Lock();

	if (t_success)
	{
        // Which data type are we fetching?
        MCTransferType t_type;
        if (p_index == nil)
            t_type = TRANSFER_TYPE_TEXT;
        else
            t_type = MCPasteboardTransferTypeFromName(p_index);
        
        // Retrieve the contents using the appropriate type.
        MCAutoValueRef t_value;
        bool t_retrieved = false;
        switch (t_type)
        {
            case TRANSFER_TYPE_TEXT:
                t_retrieved = t_clipboard->CopyAsText((MCStringRef&)&t_value);
                break;
                
            case TRANSFER_TYPE_UNICODE_TEXT:
            {
                // Retrieve as text and then encode into UTF-16
                MCAutoStringRef t_string;
                t_retrieved = t_clipboard->CopyAsText(&t_string);
                if (t_retrieved)
                    t_success = MCStringEncode(*t_string, kMCStringEncodingUTF16, false, (MCDataRef&)&t_value);
                break;
            }
                
            case TRANSFER_TYPE_STYLED_TEXT:
                t_retrieved = t_clipboard->CopyAsLiveCodeStyledText((MCDataRef&)&t_value);
                break;
                
            case TRANSFER_TYPE_STYLED_TEXT_ARRAY:
                t_retrieved = t_clipboard->CopyAsLiveCodeStyledTextArray((MCArrayRef&)&t_value);
                break;
                
            case TRANSFER_TYPE_RTF_TEXT:
                t_retrieved = t_clipboard->CopyAsRTFText((MCDataRef&)&t_value);
                break;
                
            case TRANSFER_TYPE_HTML_TEXT:
                t_retrieved = t_clipboard->CopyAsHTML((MCDataRef&)&t_value);
                break;
                
            case TRANSFER_TYPE_IMAGE:
                t_retrieved = t_clipboard->CopyAsImage((MCDataRef&)&t_value);
                break;
                
            case TRANSFER_TYPE_FILES:
                t_retrieved = t_clipboard->CopyAsFileList((MCStringRef&)&t_value);
                break;
            
            case TRANSFER_TYPE_OBJECTS:
                t_retrieved = t_clipboard->CopyAsLiveCodeObjects((MCDataRef&)&t_value);
                break;
            
            case TRANSFER_TYPE_PRIVATE:
                t_retrieved = t_clipboard->CopyAsPrivateData((MCDataRef&)&t_value);
                break;
                
            case TRANSFER_TYPE_NULL:
                // Can't retrieve any data for these types
                break;
                
            default:
                MCUnreachable();
        }
        
        // If the requested data format was not on the clipboard, set the
        // result to indicate this.
        if (t_success && !t_retrieved)
		{
			r_data = MCValueRetain(kMCEmptyData);
			ctxt . SetTheResultToStaticCString("format not available");
		}
        
        // Return the retrieved data
        if (t_success && t_retrieved)
            r_data = MCValueRetain(*t_value);
    }
	
    // Unlock the clipboard.
    if (t_clipboard_locked)
        t_clipboard->Unlock();
    
	if (!t_success)
	{
		r_data = MCValueRetain(kMCEmptyData);
		ctxt . SetTheResultToStaticCString("unable to query clipboard");
	}
}

void MCPasteboardSetClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_index, bool p_is_clipboard, MCValueRef p_data)
{
    // Clipboard or drag board?
    MCClipboard* t_clipboard;
	if (p_is_clipboard)
		t_clipboard = MCclipboard;
	else
		t_clipboard = MCdragboard;
	
    // What type of data are we being asked to place on the clipboard?
	MCTransferType t_type;
	if (p_index == nil)
        t_type = TRANSFER_TYPE_TEXT;
	else
        t_type = MCPasteboardTransferTypeFromName(p_index);

    bool t_success;
    t_success = true;
    
    // Lock the clipboard so no other changes happen as we modify it.
    bool t_clipboard_locked;
    t_success = t_clipboard_locked = t_clipboard->Lock();
    
    // If we have both a valid transfer type and some data, add it to the
    // clipboard under the appropriate type.
    //
    // If either of these is missing, do nothing but don't throw an error.
	if (t_type != TRANSFER_TYPE_NULL && p_data != nil)
	{
		switch (t_type)
		{
			case TRANSFER_TYPE_TEXT:
            {
                // Add as text
                MCAutoStringRef t_string;
                t_success = ctxt.ConvertToString(p_data, &t_string);
                if (t_success)
                    t_success = t_clipboard->AddText(*t_string);
                
                break;
            }
                
			case TRANSFER_TYPE_FILES:
			{
				// Add as a newline-separated file list
				MCAutoStringRef t_string;
				t_success = ctxt.ConvertToString(p_data, &t_string);
				if (t_success)
                    t_success = t_clipboard->AddFileList(*t_string);
				
				break;
			}
				
			case TRANSFER_TYPE_UNICODE_TEXT:
            {
                // Decode from UTF-16 and add as text
                MCAutoDataRef t_data;
                MCAutoStringRef t_string;
                t_success = ctxt.ConvertToData(p_data, &t_data);
                if (t_success)
                    t_success = MCStringDecode(*t_data, kMCStringEncodingUTF16, false, &t_string);
                if (t_success)
                    t_success = t_clipboard->AddText(*t_string);
                
                break;
            }
                
			case TRANSFER_TYPE_STYLED_TEXT:
            {
                // Add to the clipboard as serialised styled text
                MCAutoDataRef t_data;
                t_success = ctxt.ConvertToData(p_data, &t_data);
                if (t_success)
                    t_success = t_clipboard->AddLiveCodeStyledText(*t_data);
                
                break;
            }
                
			case TRANSFER_TYPE_RTF_TEXT:
            {
                // Add to the clipboard as RTF data
                MCAutoDataRef t_data;
                t_success = ctxt.ConvertToData(p_data, &t_data);
                if (t_success)
                    t_success = t_clipboard->AddRTFText(*t_data);
                
                break;
            }
                
			case TRANSFER_TYPE_IMAGE:
            {
                // Add to the clipboard as image data
                MCAutoDataRef t_data;
                t_success = ctxt.ConvertToData(p_data, &t_data);
                if (t_success)
                    t_success = t_clipboard->AddImage(*t_data);
                
                break;
            }
                
			case TRANSFER_TYPE_PRIVATE:
            {
                // Add to the clipboard as private data
                MCAutoDataRef t_data;
                t_success = ctxt.ConvertToData(p_data, &t_data);
                if (t_success)
                    t_success = t_clipboard->AddPrivateData(*t_data);
                break;
            }
                
			case TRANSFER_TYPE_OBJECTS:
			{
				// Add to the clipboard as serialised LiveCode objects
				MCAutoDataRef t_data;
				t_success = ctxt.ConvertToData(p_data, &t_data);
				if (t_success)
                    t_success = t_clipboard->AddLiveCodeObjects(*t_data);
				
				break;
			}
				
			case TRANSFER_TYPE_HTML_TEXT:
			{
				// For backwards compatibility, HTML can be added to the
                // clipboard as either data or text.
				if (MCValueGetTypeCode(p_data) == kMCValueTypeCodeData)
                    t_success = t_clipboard->AddHTML(static_cast<MCDataRef>(p_data));
				else
				{
                    // If it is a string, encode it into the native encoding
                    // then add as data. This probably should be ASCII but we've
                    // always used the native encoding here...
                    MCAutoStringRef t_string;
                    MCAutoDataRef t_data;
                    t_success = ctxt.ConvertToString(p_data, &t_string);
                    if (t_success)
                        t_success = MCStringEncode(*t_string, kMCStringEncodingNative, false, &t_data);
                    if (t_success)
                        t_success = t_clipboard->AddHTML(*t_data);
				}
				
				break;
			}
				
			case TRANSFER_TYPE_STYLED_TEXT_ARRAY:
			{
				// If the incoming data is an array, add it. If it isn't, add
                // an empty array in its place.
				if (MCValueGetTypeCode(p_data) == kMCValueTypeCodeArray)
                    t_success = t_clipboard->AddLiveCodeStyledTextArray(static_cast<MCArrayRef>(p_data));
				else
                    t_success = t_clipboard->AddLiveCodeStyledTextArray(static_cast<MCArrayRef>(p_data));
				
				break;
			}
				
			case TRANSFER_TYPE_NULL:
            default:
				MCUnreachable();
		}
	}

    // Unlock the clipboard so the changes are pushed to the OS' clipboard.
    if (t_clipboard_locked)
        t_clipboard->Unlock();
    
    if (!t_success)
        ctxt.Throw();
}

void MCPasteboardGetClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data)
{
	MCPasteboardGetClipboardOrDragData(ctxt, p_index, true, r_data);
}

void MCPasteboardSetClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data)
{
	MCPasteboardSetClipboardOrDragData(ctxt, p_index, true, p_data);
}

void MCPasteboardGetDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data)
{
	MCPasteboardGetClipboardOrDragData(ctxt, p_index, false, r_data);
}

void MCPasteboardSetDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data)
{
	MCPasteboardSetClipboardOrDragData(ctxt, p_index, false, p_data);
}

void MCPasteboardGetClipboardTextData(MCExecContext& ctxt, MCValueRef& r_data)
{
	MCPasteboardGetClipboardOrDragData(ctxt, nil, true, r_data);
}

void MCPasteboardSetClipboardTextData(MCExecContext& ctxt, MCValueRef p_data)
{
	MCPasteboardSetClipboardOrDragData(ctxt, nil, true, p_data);
}

void MCPasteboardGetDragTextData(MCExecContext& ctxt, MCValueRef& r_data)
{
	MCPasteboardGetClipboardOrDragData(ctxt, nil, false, r_data);
}

void MCPasteboardSetDragTextData(MCExecContext& ctxt, MCValueRef p_data)
{
	MCPasteboardSetClipboardOrDragData(ctxt, nil, false, p_data);
}

void MCPasteboardGetRawClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data)
{
    // Ensure there is an active script clipboard
    if (MCscriptrawclipboard == NULL)
    {
        ctxt.LegacyThrow(EE_RAW_CLIPBOARD_NOT_LOCKED);
        return;
    }
    
    // TODO: support multiple items
    // Get the first item on the clipboard
    MCAutoRefcounted<const MCRawClipboardItem> t_item = MCscriptrawclipboard->GetItemAtIndex(0);
    if (t_item == NULL)
    {
        // Clipboard is empty so the key is not present
        ctxt.LegacyThrow(EE_RAW_CLIPBOARD_BADREP);
    }
    else
    {
        // Attempt to get the data for this item
        const MCRawClipboardItemRep* t_rep = t_item->FetchRepresentationByType(MCNameGetString(p_index));
        if (t_rep == NULL)
            ctxt.LegacyThrow(EE_RAW_CLIPBOARD_BADREP);
        else
        {
            // Get the data for this representation
            MCDataRef t_data = t_rep->CopyData();
            if (t_data == NULL)
                ctxt.LegacyThrow(EE_RAW_CLIPBOARD_BADREP);
            r_data = t_data;
        }
    }
}

void MCPasteboardSetRawClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data)
{
    // Ensure there is an active script clipboard
    if (MCscriptrawclipboard == NULL)
    {
        ctxt.LegacyThrow(EE_RAW_CLIPBOARD_NOT_LOCKED);
        return;
    }
    
    // We cannot write to the clipboard if it contains externally-sourced data
    if (MCscriptrawclipboard->IsExternalData())
    {
        ctxt.LegacyThrow(EE_RAW_CLIPBOARD_EXTERNAL_DATA);
        return;
    }
    
    // TODO: support multiple items
    // Get the first item in the clipboard
    MCAutoRefcounted<MCRawClipboardItem> t_item = MCscriptrawclipboard->GetItemAtIndex(0);
    
    // If there is no item on the clipboard yet, we will have to allocate one
    if (t_item == NULL)
    {
        t_item = MCscriptrawclipboard->CreateNewItem();
        if (!MCscriptrawclipboard->AddItem(t_item))
        {
            ctxt.LegacyThrow(EE_RAW_CLIPBOARD_INSERT_FAILED);
            return;
        }
    }
    
    // Convert the incoming valueref to data
    MCAutoDataRef t_data;
    if (!ctxt.ConvertToData(p_data, &t_data))
    {
        ctxt.LegacyThrow(EE_RAW_CLIPBOARD_BADREP);
        return;
    }
    
    // Add the representation to the clipboard item
    if (!t_item->AddRepresentation(MCNameGetString(p_index), *t_data))
    {
        ctxt.LegacyThrow(EE_RAW_CLIPBOARD_BADREP);
        return;
    }
}

void MCPasteboardGetRawClipboardTextData(MCExecContext& ctxt, MCValueRef& r_value)
{
    // Ensure there is an active script clipboard
    if (MCscriptrawclipboard == NULL)
    {
        ctxt.LegacyThrow(EE_RAW_CLIPBOARD_NOT_LOCKED);
        return;
    }
    
    // Calling this method is always an error
    ctxt.LegacyThrow(EE_RAW_CLIPBOARD_BADREP);
}

void MCPasteboardSetRawClipboardTextData(MCExecContext& ctxt, MCValueRef p_value)
{
    // Ensure there is an active script clipboard
    if (MCscriptrawclipboard == NULL)
    {
        ctxt.LegacyThrow(EE_RAW_CLIPBOARD_NOT_LOCKED);
        return;
    }
    
    // The only supported value here is empty as it is used to clear the
    // contents of the clipboard to prepare it for writing.
    if (!MCValueIsEmpty(p_value))
    {
        ctxt.LegacyThrow(EE_RAW_CLIPBOARD_BADREP);
        return;
    }
    
    // Clear the clipboard
    MCscriptrawclipboard->Clear();
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalDragSourceAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCdragsource != nil)
    {
        r_object . object = MCdragsource;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCPasteboardEvalDragDestinationAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCdragdest != nil)
    {
        r_object . object = MCdragdest;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

////////////////////////////////////////////////////////////////////////////////


void MCPasteboardExecLockClipboard(MCExecContext& ctxt)
{
    // Don't allow recursive locking
    if (MCscriptrawclipboard != NULL)
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_ALREADY_LOCKED);
        return;
    }
    
    // Lock the main clipboard. This has the side-effect of pulling updates.
    if (!MCclipboard->Lock())
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_NOT_LOCKED);
        return;
    }
    
    // Use the same underlying clipboard as the higher-level clipboard code (so
    // that they are kept in sync) for the raw clipboard accesses.
    MCscriptrawclipboard = MCclipboard->GetRawClipboard()->Retain();
}

void MCPasteboardExecUnlockClipboard(MCExecContext& ctxt)
{
    // Make sure a clipboard has actually been locked
    if (MCscriptrawclipboard == NULL)
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_NOT_LOCKED);
        return;
    }
    
    // Release our hold on the scripting world's raw clipboard object
    MCscriptrawclipboard->Release();
    MCscriptrawclipboard = NULL;
    
    // Unlock the main clipboard. This will push any changes to the system.
    MCclipboard->Unlock();
}

////////////////////////////////////////////////////////////////////////////////
