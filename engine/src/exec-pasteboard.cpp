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

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, Clipboard, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, ClipboardKeys, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, DropChunk, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, DragDestination, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, DragSource, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, DragDropKeys, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, IsAmongTheKeysOfTheClipboardData, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Pasteboard, IsNotAmongTheKeysOfTheClipboardData, 2)
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
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, DragData, 2)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, DragData, 2)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, ClipboardOrDragData, 2)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, ClipboardOrDragData, 2)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, ClipboardTextData, 2)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, ClipboardTextData, 2)
MC_EXEC_DEFINE_GET_METHOD(Pasteboard, DragTextData, 2)
MC_EXEC_DEFINE_SET_METHOD(Pasteboard, DragTextData, 2)

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
	}

	return kMCEmptyName;
}

bool MCPasteboardListKeys(MCTransferData *p_data, char_t p_delimiter, MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable(p_delimiter, &t_list))
		return false;

	MCTransferType *t_types;
	size_t t_count;

	if (!p_data->Lock())
		return false;

	bool t_success = true;

	t_success = p_data->Query(t_types, t_count);

	for (uinteger_t i = 0; t_success && i < t_count; i++)
	{
		switch(t_types[i])
		{
		case TRANSFER_TYPE_TEXT:
			t_success = MCListAppend(*t_list, MCN_text);
			break;

		case TRANSFER_TYPE_UNICODE_TEXT:
			t_success = MCListAppend(*t_list, MCN_unicode) &&
				MCListAppend(*t_list, MCN_text);
			break;

		case TRANSFER_TYPE_STYLED_TEXT:
			t_success = MCListAppend(*t_list, MCN_styles) &&
				MCListAppend(*t_list, MCN_rtf) &&
				MCListAppend(*t_list, MCN_unicode) &&
				MCListAppend(*t_list, MCN_text);
			break;

		case TRANSFER_TYPE_IMAGE:
			t_success = MCListAppend(*t_list, MCN_image);
			break;

		case TRANSFER_TYPE_FILES:
			t_success = MCListAppend(*t_list, MCN_files) &&
				MCListAppend(*t_list, MCN_text);
			break;

		case TRANSFER_TYPE_PRIVATE:
			t_success = MCListAppend(*t_list, MCN_private);
			break;

		case TRANSFER_TYPE_OBJECTS:
			t_success = MCListAppend(*t_list, MCN_objects);
			break;

		default:
			// MW-2009-04-05: Stop GCC warning
			break;
		}
	}

	p_data->Unlock();

	if (t_success)
		t_success = MCListCopy(*t_list, r_list);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalClipboard(MCExecContext& ctxt, MCNameRef& r_string)
{
	if (MCclipboarddata -> Lock())
	{
		MCNameRef t_clipboard = nil;
		if (MCclipboarddata -> Contains(TRANSFER_TYPE_FILES, false))
			t_clipboard = MCN_files;
		else if (MCclipboarddata -> Contains(TRANSFER_TYPE_OBJECTS, false))
			t_clipboard = MCN_objects;
		else if (MCclipboarddata -> Contains(TRANSFER_TYPE_IMAGE, true))
			t_clipboard = MCN_image;
		else if (MCclipboarddata -> Contains(TRANSFER_TYPE_PRIVATE, false))
			t_clipboard = MCN_private;
		else if (MCclipboarddata -> Contains(TRANSFER_TYPE_TEXT, true))
			t_clipboard = MCN_text;
		else
			t_clipboard = MCN_empty;

		MCclipboarddata -> Unlock();
		r_string = MCValueRetain(t_clipboard);
	}
	else
	{
		r_string = MCValueRetain(kMCEmptyName);
		ctxt . SetTheResultToCString("unable to access clipboard");
	}
}

void MCPasteboardEvalClipboardKeys(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (!MCPasteboardListKeys(MCclipboarddata, '\n', &t_list))
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
	if (!MCPasteboardListKeys(MCdragdata, '\n', &t_list))
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
	r_result = MCclipboarddata->Contains(MCPasteboardTransferTypeFromName(p_key), true);
}

void MCPasteboardEvalIsNotAmongTheKeysOfTheClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
	MCPasteboardEvalIsAmongTheKeysOfTheClipboardData(ctxt, p_key, r_result);
	r_result = !r_result;
}

//////////

void MCPasteboardEvalIsAmongTheKeysOfTheDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
	r_result = MCclipboarddata->Contains(MCPasteboardTransferTypeFromName(p_key), true);
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
	MCclipboarddata -> Open();

	// Attempt to store the objects onto the clipboard
	if (t_success)
		if (!MCclipboarddata -> Store(TRANSFER_TYPE_OBJECTS, *t_pickle))
			t_success = false;

	// If we've managed to store objects, and we are only copying/cutting one
	// image object, attempt to copy the image data.
	// Note that we don't care if this doesn't succeed as its not critical.
	if (t_success)
		if (p_object_count == 1 && p_targets[0] . object -> gettype() == CT_IMAGE)
		{
			// MW-2011-02-26: [[ Bug 9403 ]] If no image data is fetched, then don't try
			//   and store anything!
			MCAutoDataRef t_image_data;
			if (static_cast<MCImage *>(p_targets[0] . object) -> getclipboardtext(&t_image_data))
				MCclipboarddata -> Store(TRANSFER_TYPE_IMAGE, *t_image_data);
		}

	// Close the clipboard. If this returns false, then it means the
	// writing was unsuccessful.
	if (!MCclipboarddata -> Close())
		t_success = false;

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
	MCTransferData *t_pasteboard;
	if (p_is_clipboard)
		t_pasteboard = MCclipboarddata;
	else
		t_pasteboard = MCdragdata;

	bool t_success;
	t_success = true;

	if (t_pasteboard -> Lock())
	{
		MCTransferType t_type;
		if (p_index == nil)
            t_type = TRANSFER_TYPE_TEXT;
		else
			t_type = MCTransferData::StringToType(MCNameGetString(p_index));

        // MW-2014-03-12: [[ ClipboardStyledText ]] If styledText is being requested, then
        //   convert the styles data to an array and return that.
        if (t_type == TRANSFER_TYPE_STYLED_TEXT_ARRAY &&
            t_pasteboard -> Contains(TRANSFER_TYPE_STYLED_TEXT, true))
        {
            MCAutoValueRef t_data;
            if (t_pasteboard -> Fetch(TRANSFER_TYPE_STYLED_TEXT, &t_data))
                t_success = MCConvertStyledTextToStyledTextArray((MCDataRef)*t_data, (MCArrayRef&)r_data);
            else
                t_success = false;
        }
		else if (t_type != TRANSFER_TYPE_NULL && t_pasteboard -> Contains(t_type, true))
            t_success = t_pasteboard -> Fetch(t_type, r_data);
		else
		{
			r_data = MCValueRetain(kMCEmptyData);
			ctxt . SetTheResultToStaticCString("format not available");
		}
		
		t_pasteboard -> Unlock();
	}
	else
		t_success = false;
	
	if (!t_success)
	{
		r_data = MCValueRetain(kMCEmptyData);
		ctxt . SetTheResultToStaticCString("unable to query clipboard");
	}
}

void MCPasteboardSetClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_index, bool p_is_clipboard, MCValueRef p_data)
{
	MCTransferData *t_pasteboard;
	if (p_is_clipboard)
		t_pasteboard = MCclipboarddata;
	else
		t_pasteboard = MCdragdata;
	
	MCTransferType t_type;
	if (p_index == nil)
        t_type = TRANSFER_TYPE_TEXT;
	else
		t_type = MCTransferData::StringToType(MCNameGetString(p_index));

	if (t_type != TRANSFER_TYPE_NULL && p_data != nil)
	{    
        MCAutoValueRef t_value;
        bool t_success;
        t_success = true;
		
		switch (t_type)
		{
			case TRANSFER_TYPE_TEXT:
			case TRANSFER_TYPE_FILES:
			{
				// convert to MCStringRef
				MCAutoStringRef t_string;
				t_success = ctxt.ConvertToString(p_data, &t_string);
				if (t_success)
					t_value = *t_string;
				
				break;
			}
				
			case TRANSFER_TYPE_UNICODE_TEXT:
			case TRANSFER_TYPE_STYLED_TEXT:
			case TRANSFER_TYPE_RTF_TEXT:
			case TRANSFER_TYPE_IMAGE:
			case TRANSFER_TYPE_PRIVATE:
			case TRANSFER_TYPE_OBJECTS:
			{
				// convert to MCDataRef
				MCAutoDataRef t_data;
				t_success = ctxt.ConvertToData(p_data, &t_data);
				if (t_success)
					t_value = *t_data;
				
				break;
			}
				
			case TRANSFER_TYPE_HTML_TEXT:
			{
				// convert to MCStringRef or MCDataRef
				if (MCValueGetTypeCode(p_data) == kMCValueTypeCodeData)
					t_value = p_data;
				else
				{
					MCAutoStringRef t_string;
					t_success = ctxt.ConvertToString(p_data, &t_string);
					if (t_success)
						t_value = *t_string;
				}
				
				break;
			}
				
			case TRANSFER_TYPE_STYLED_TEXT_ARRAY:
			{
				// convert to MCArrayRef
				if (MCValueGetTypeCode(p_data) == kMCValueTypeCodeArray)
					t_value = p_data;
				else
					t_value = kMCEmptyArray;
				
				break;
			}
				
			case TRANSFER_TYPE_NULL:
				MCUnreachable();
		}
		
        if (t_success)
        {
            if (t_pasteboard -> Store(t_type, *t_value))
                return;
        }
	}
	else
		return;

	ctxt . Throw();
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
