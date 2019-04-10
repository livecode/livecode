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
#include "mcerror.h"

#include "raw-clipboard.h"

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
    TRANSFER_TYPE_UNICODE_TEXT,
    TRANSFER_TYPE_STYLED_TEXT,
    TRANSFER_TYPE_STYLED_TEXT_ARRAY,
    TRANSFER_TYPE_RTF_TEXT,
    TRANSFER_TYPE_HTML_TEXT,
    
    TRANSFER_TYPE_IMAGE,
    TRANSFER_TYPE_FILES,
    TRANSFER_TYPE_PRIVATE,
    TRANSFER_TYPE_OBJECTS,
    
    // The following transfer types only apply for the full{clipboard,drag}Data
    TRANSFER_TYPE_RTF,
    TRANSFER_TYPE_HTML,
    TRANSFER_TYPE_PNG,
    TRANSFER_TYPE_GIF,
    TRANSFER_TYPE_JPEG,
	TRANSFER_TYPE_BMP,
	TRANSFER_TYPE_WIN_METAFILE,
	TRANSFER_TYPE_WIN_ENH_METAFILE,
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

// Forward declarations of internal utility functions
static MCTransferType MCPasteboardFindTransferTypeForLegacyClipboard(const MCClipboard* p_clipboard);
static bool MCPasteboardCopyAsTypeForLegacyClipboard(const MCClipboard* p_clipboard, MCTransferType p_as_type, MCValueRef& r_value);
static bool MCPasteboardListKeysLegacy(MCClipboard *p_clipboard, char_t p_delimiter, MCListRef& r_list);
static void MCPasteboardGetRawClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_index, const MCClipboard* p_clipboard, MCValueRef& r_data);
static void MCPasteboardSetRawClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_index, MCClipboard* p_clipboard, MCValueRef p_data);
static void MCPasteboardGetFullClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_index, const MCClipboard* p_clipboard, MCValueRef& r_data);
static void MCPasteboardSetFullClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_index, MCClipboard* p_clipboard, MCValueRef p_data);
static void MCPasteboardEvalRawClipboardOrDragKeys(MCExecContext& ctxt, const MCClipboard* p_clipboard, MCStringRef& r_keys);
static void MCPasteboardEvalFullClipboardOrDragKeys(MCExecContext& ctxt, const MCClipboard* p_clipboard, MCStringRef& r_keys);
static void MCPasteboardEvalIsAmongTheKeysOfTheRawClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_key, const MCClipboard* p_clipboard, bool& r_result);
static void MCPasteboardEvalIsAmongTheKeysOfTheFullClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_key, const MCClipboard* p_clipboard, bool& r_result);
static void MCPasteboardGetRawClipboardOrDragTextData(MCExecContext& ctxt, const MCClipboard* p_clipboard, MCValueRef& r_value);
static void MCPasteboardSetRawClipboardOrDragTextData(MCExecContext& ctxt, MCClipboard* p_clipboard, MCValueRef p_value);
static void MCPasteboardGetFullClipboardOrDragTextData(MCExecContext& ctxt, const MCClipboard* p_clipboard, MCValueRef& r_value);
static void MCPasteboardSetFullClipboardOrDragTextData(MCExecContext& ctxt, MCClipboard* p_clipboard, MCValueRef p_value);

////////////////////////////////////////////////////////////////////////////////


// The legacy clipboard accessors (the 'clipboard' function and the
// 'clipboardData' property) only support a single representation of the item
// on the clipboard, but are capable of converting this representation to
// various forms. For example, if the clipboard contains both an image and some
// text, only the image will be made available. If it contains RTF, however, all
// the text formats that the engine can convert to are available.
//
// The conversions that the engine performs are summarised below:
//
//      Files can be copied as: files,text
//      Images can be copied as: image
//      Objects can be copied as: objects
//      Any text type can be copied as: styles,styledText,rtf,unicode,text
//
// The priority order is:
//
//      1. LiveCode objects
//      2. Files
//      3. Image or text (whichever comes first)
//      4. private data
//


MCTransferType MCPasteboardFindTransferTypeForLegacyClipboard(const MCClipboard* p_clipboard)
{
    if (p_clipboard->HasLiveCodeObjects())
        return TRANSFER_TYPE_OBJECTS;
    if (p_clipboard->HasFileList())
        return TRANSFER_TYPE_FILES;
    
    // In theory, the old clipboard code checked for whether image or text data
    // came first in the list of clipboard formats and chose the format that
    // way. However, there was a bug in the code that prevented this from
    // working properly, so text was always chosen in preference to images.
    //
    // The correct code is provided here in case we decide that enough time has
    // passed that the behaviour can now be corrected but, until then, it is
    // disabled and the "text over images" method is used instead.
    //
    if (/* DISABLES CODE */ (false))
    {
        // If we have both an image and text on the clipboard, whichever one comes
        // first determines what we resolve the clipboard type to be.
        int t_order = p_clipboard->GetLegacyOrdering();
        if (t_order > 0)
            return TRANSFER_TYPE_IMAGE;
        if (t_order < 0)
            return TRANSFER_TYPE_TEXT;
    }
    else
    {
        if (p_clipboard->HasTextOrCompatible())
            return TRANSFER_TYPE_TEXT;
        if (p_clipboard->HasImage())
            return TRANSFER_TYPE_IMAGE;
    }
    
    // Does the clipboard contain private data?
    if (p_clipboard->HasPrivateData())
        return TRANSFER_TYPE_PRIVATE;
    
    // Nothing is recognised
    return TRANSFER_TYPE_NULL;
}

bool MCPasteboardCopyAsTypeForLegacyClipboard(const MCClipboard* p_clipboard, MCTransferType p_as_type, MCValueRef& r_value)
{
    // Get the legacy transfer type class for the clipboard
    MCTransferType t_clipboard_type = MCPasteboardFindTransferTypeForLegacyClipboard(p_clipboard);
    
    // Highest priority: the clipboard contains LiveCode objects
    if (t_clipboard_type == TRANSFER_TYPE_OBJECTS)
    {
        // The only type supported is objects
        if (p_as_type == TRANSFER_TYPE_OBJECTS)
            return p_clipboard->CopyAsLiveCodeObjects((MCDataRef&)r_value);
    }
    
    // Next priority: the clipboard contains files
    else if (t_clipboard_type == TRANSFER_TYPE_FILES)
    {
        // Conversion to either "files" or "text" is supported - either way, it
        // is a newline-separated list of files.
        if (p_as_type == TRANSFER_TYPE_TEXT || p_as_type == TRANSFER_TYPE_FILES)
            return p_clipboard->CopyAsFileList((MCStringRef&)r_value);
    }
    
    // Does the clipboard contain images or text? The relative priority of these
    // varies but t_clipboard_type contains the one that should take priority
    // for the current contents of the clipboard.
    else if (t_clipboard_type == TRANSFER_TYPE_IMAGE)
    {
        // Can only convert to the "image" type
        if (p_as_type == TRANSFER_TYPE_IMAGE)
            return p_clipboard->CopyAsImage((MCDataRef&)r_value);
    }
    else if (t_clipboard_type == TRANSFER_TYPE_TEXT)
    {
        // Multiple conversion types are supported for text. Which is requested?
        if (p_as_type == TRANSFER_TYPE_TEXT)
        {
            return p_clipboard->CopyAsText((MCStringRef&)r_value);
        }
        else if (p_as_type == TRANSFER_TYPE_UNICODE_TEXT)
        {
            // Copy as text and then encode to UTF-16
            MCAutoStringRef t_as_text;
            if (p_clipboard->CopyAsText(&t_as_text))
                return MCStringEncode(*t_as_text, kMCStringEncodingUTF16, false, (MCDataRef&)r_value);
        }
        else if (p_as_type == TRANSFER_TYPE_STYLED_TEXT)
        {
            return p_clipboard->CopyAsLiveCodeStyledText((MCDataRef&)r_value);
        }
        else if (p_as_type == TRANSFER_TYPE_STYLED_TEXT_ARRAY)
        {
            return p_clipboard->CopyAsLiveCodeStyledTextArray((MCArrayRef&)r_value);
        }
        else if (p_as_type == TRANSFER_TYPE_RTF_TEXT)
        {
            // Copy as the round-tripped form of RTF
            return p_clipboard->CopyAsRTFText((MCDataRef&)r_value);
        }
        else if (p_as_type == TRANSFER_TYPE_HTML_TEXT)
        {
            // Copy as the round-tripped form of HTML
            return p_clipboard->CopyAsHTMLText((MCStringRef&)r_value);
        }
    }
    
    // Finally, handle private data
    else if (t_clipboard_type == TRANSFER_TYPE_PRIVATE)
    {
        // Private data cannot be converted to anything else
        if (p_as_type == TRANSFER_TYPE_PRIVATE)
            return p_clipboard->CopyAsPrivateData((MCDataRef&)r_value);
    }
    
    // Couldn't copy the data in the requested format
    return false;
}

////////////////////////////////////////////////////////////////////////////////


MCTransferType MCPasteboardTransferTypeFromName(MCNameRef p_key, bool p_legacy = false)
{
	if (MCNameIsEqualToCaseless(p_key, MCN_text))
		return TRANSFER_TYPE_TEXT;

	if (MCNameIsEqualToCaseless(p_key, MCN_unicode))
		return TRANSFER_TYPE_UNICODE_TEXT;

	if (MCNameIsEqualToCaseless(p_key, MCN_styles))
		return TRANSFER_TYPE_STYLED_TEXT;

    if (MCNameIsEqualToCaseless(p_key, MCN_styledtext))
        return TRANSFER_TYPE_STYLED_TEXT_ARRAY;
    
	if (MCNameIsEqualToCaseless(p_key, MCN_rtf))
        return p_legacy ? TRANSFER_TYPE_RTF_TEXT : TRANSFER_TYPE_RTF;

	if (MCNameIsEqualToCaseless(p_key, MCN_html))
        return p_legacy ? TRANSFER_TYPE_HTML_TEXT : TRANSFER_TYPE_HTML;

	if (MCNameIsEqualToCaseless(p_key, MCN_files))
		return TRANSFER_TYPE_FILES;

	if (MCNameIsEqualToCaseless(p_key, MCN_private))
		return TRANSFER_TYPE_PRIVATE;

	if (MCNameIsEqualToCaseless(p_key, MCN_image))
		return TRANSFER_TYPE_IMAGE;

	if (MCNameIsEqualToCaseless(p_key, MCN_objects))
		return TRANSFER_TYPE_OBJECTS;
    
    if (MCNameIsEqualToCaseless(p_key, MCN_rtftext))
        return TRANSFER_TYPE_RTF_TEXT;
    
    if (MCNameIsEqualToCaseless(p_key, MCN_htmltext))
        return TRANSFER_TYPE_HTML_TEXT;
    
    if (MCNameIsEqualToCaseless(p_key, MCN_png))
        return TRANSFER_TYPE_PNG;
    
    if (MCNameIsEqualToCaseless(p_key, MCN_gif))
        return TRANSFER_TYPE_GIF;
    
    if (MCNameIsEqualToCaseless(p_key, MCN_jpeg))
        return TRANSFER_TYPE_JPEG;

	if (MCNameIsEqualToCaseless(p_key, MCN_win_bitmap))
		return TRANSFER_TYPE_BMP;

	if (MCNameIsEqualToCaseless(p_key, MCN_win_metafile))
		return TRANSFER_TYPE_WIN_METAFILE;

	if (MCNameIsEqualToCaseless(p_key, MCN_win_enh_metafile))
		return TRANSFER_TYPE_WIN_ENH_METAFILE;

	return TRANSFER_TYPE_NULL;
}

MCNameRef MCPasteboardTransferTypeToName(MCTransferType p_type, bool p_legacy = false)
{
	switch(p_type)
	{
	case TRANSFER_TYPE_TEXT:
		return MCN_text;
	case TRANSFER_TYPE_UNICODE_TEXT:
		return MCN_unicode;
	case TRANSFER_TYPE_STYLED_TEXT:
		return MCN_styles;
    case TRANSFER_TYPE_STYLED_TEXT_ARRAY:
        return MCN_styledtext;
	case TRANSFER_TYPE_RTF_TEXT:
        return p_legacy ? MCN_rtf : MCN_rtftext;
	case TRANSFER_TYPE_HTML_TEXT:
        return p_legacy ? MCN_html : MCN_htmltext;
    case TRANSFER_TYPE_HTML:
        return MCN_html;
    case TRANSFER_TYPE_RTF:
        return MCN_rtf;
	case TRANSFER_TYPE_IMAGE:
		return MCN_image;
    case TRANSFER_TYPE_PNG:
        return MCN_png;
    case TRANSFER_TYPE_GIF:
        return MCN_gif;
    case TRANSFER_TYPE_JPEG:
        return MCN_jpeg;
	case TRANSFER_TYPE_BMP:
		return MCN_win_bitmap;
	case TRANSFER_TYPE_WIN_METAFILE:
		return MCN_win_metafile;
	case TRANSFER_TYPE_WIN_ENH_METAFILE:
		return MCN_win_enh_metafile;
	case TRANSFER_TYPE_FILES:
		return MCN_files;
	case TRANSFER_TYPE_PRIVATE:
		return MCN_private;
	case TRANSFER_TYPE_OBJECTS:
		return MCN_objects;
    case TRANSFER_TYPE_NULL:
    default:
        break;
	}

	return kMCEmptyName;
}

bool MCPasteboardListKeysLegacy(MCClipboard *p_clipboard, char_t p_delimiter, MCListRef& r_list)
{
    // Create the output list
    MCAutoListRef t_list;
    if (!MCListCreateMutable(p_delimiter, &t_list))
        return false;
    
    // The set of keys for each transfer type is fixed. Get the transfer type
    // and add the appropriate keys.
    bool t_success = true;
    MCTransferType t_clipboard_type = MCPasteboardFindTransferTypeForLegacyClipboard(p_clipboard);
    if (t_clipboard_type == TRANSFER_TYPE_OBJECTS)
        t_success = MCListAppend(*t_list, MCN_objects);
    else if (t_clipboard_type == TRANSFER_TYPE_FILES)
        t_success = MCListAppend(*t_list, MCN_files)
                    && MCListAppend(*t_list, MCN_text);
    else if (t_clipboard_type == TRANSFER_TYPE_IMAGE)
        t_success = MCListAppend(*t_list, MCN_image);
    else if (t_clipboard_type == TRANSFER_TYPE_PRIVATE)
        t_success = MCListAppend(*t_list, MCN_private);
    else if (t_clipboard_type == TRANSFER_TYPE_TEXT)
        t_success = MCListAppend(*t_list, MCN_text)
                    && MCListAppend(*t_list, MCN_unicode)
                    && MCListAppend(*t_list, MCN_styles)
                    && MCListAppend(*t_list, MCN_rtf)
                    && MCListAppend(*t_list, MCN_html);
    
    // Copy the list to the output and return
    if (t_success)
        t_success = MCListCopy(*t_list, r_list);
    return t_success;
}

void MCPasteboardEvalIsAmongTheKeysOfLegacy(MCExecContext& ctxt, MCClipboard* p_clipboard, MCNameRef p_key, bool& r_result)
{
    // Turn the array index into a tranfer type
    MCTransferType t_type = MCPasteboardTransferTypeFromName(p_key, true);
    
    // Get the type of the data  on the clipboard
    MCTransferType t_clipboard_type = MCPasteboardFindTransferTypeForLegacyClipboard(p_clipboard);
    
    // If either of the types is NULL, then the key isn't present
    if (t_type == TRANSFER_TYPE_NULL || t_clipboard_type == TRANSFER_TYPE_NULL)
    {
        r_result = false;
        return;
    }
    
    // If the types are equal, the key is present
    if (t_type == t_clipboard_type)
    {
        r_result = true;
        return;
    }
    
    // Otherwise, check for potential conversions
    r_result = false;
    if (t_clipboard_type == TRANSFER_TYPE_FILES && t_type == TRANSFER_TYPE_TEXT)
        r_result = true;
    if (t_clipboard_type == TRANSFER_TYPE_TEXT
        && (t_type == TRANSFER_TYPE_UNICODE_TEXT
            || t_type == TRANSFER_TYPE_RTF_TEXT
            || t_type == TRANSFER_TYPE_HTML_TEXT
            || t_type == TRANSFER_TYPE_STYLED_TEXT
            || t_type == TRANSFER_TYPE_STYLED_TEXT_ARRAY))
        r_result = true;
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
        MCTransferType t_type = MCPasteboardFindTransferTypeForLegacyClipboard(MCclipboard);
        if (t_type == TRANSFER_TYPE_NULL)
            r_type = MCValueRetain(MCN_empty);
        else
            r_type = MCValueRetain(MCPasteboardTransferTypeToName(t_type, true));
        
        MCclipboard->Unlock();
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
	if (!MCPasteboardListKeysLegacy(MCclipboard, '\n', &t_list))
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
    MCPasteboardEvalRawClipboardOrDragKeys(ctxt, MCclipboard, r_string);
}

void MCPasteboardEvalRawDragKeys(MCExecContext& ctxt, MCStringRef& r_string)
{
    MCPasteboardEvalRawClipboardOrDragKeys(ctxt, MCdragboard, r_string);
}

void MCPasteboardEvalRawClipboardOrDragKeys(MCExecContext& ctxt, const MCClipboard* p_clipboard, MCStringRef& r_string)
{
    // Ensure there is an active script clipboard
    if (!p_clipboard->IsLocked())
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_NOTLOCKED);
        return;
    }
    
    // TODO: support multiple items
    // Get the first item on the clipboard
    MCAutoRefcounted<const MCRawClipboardItem> t_item = p_clipboard->GetRawClipboard()->GetItemAtIndex(0);
    if (t_item == NULL)
    {
        r_string = MCValueRetain(kMCEmptyString);
		return;
    }
	
	MCAutoListRef t_list;
	bool t_success = MCListCreateMutable('\n', &t_list);
	
	uindex_t t_type_count = t_item->GetRepresentationCount();
	for (uindex_t i = 0; t_success && i < t_type_count; i++)
	{
        MCAutoStringRef t_type;
        t_type.Give(t_item->FetchRepresentationAtIndex(i)->CopyTypeString());
        if (!t_type.IsSet())
        {
            continue;
        }
        
		t_success = MCListAppend(*t_list, *t_type);
	}
	
	if (t_success)
		t_success = MCListCopyAsString(*t_list, r_string);
	
	if (t_success)
		return;
	
	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalDropChunk(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (!MCdropfield)
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
	if (!MCdragdest)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	MCdragdest->getstringprop(ctxt, 0, P_LONG_ID, False, r_string);
}

void MCPasteboardEvalDragSource(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (!MCdragsource)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	MCdragsource->getstringprop(ctxt, 0, P_LONG_ID, False, r_string);
}

void MCPasteboardEvalDragDropKeys(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (!MCPasteboardListKeysLegacy(MCdragboard, '\n', &t_list))
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
    MCPasteboardEvalIsAmongTheKeysOfLegacy(ctxt, MCclipboard, p_key, r_result);
}

void MCPasteboardEvalIsNotAmongTheKeysOfTheClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
	MCPasteboardEvalIsAmongTheKeysOfTheClipboardData(ctxt, p_key, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalIsAmongTheKeysOfTheRawClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    MCPasteboardEvalIsAmongTheKeysOfTheRawClipboardOrDragData(ctxt, p_key, MCclipboard, r_result);
}

void MCPasteboardEvalIsNotAmongTheKeysOfTheRawClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    MCPasteboardEvalIsAmongTheKeysOfTheRawClipboardOrDragData(ctxt, p_key, MCclipboard, r_result);
    r_result = !r_result;
}

void MCPasteboardEvalIsAmongTheKeysOfTheRawDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    MCPasteboardEvalIsAmongTheKeysOfTheRawClipboardOrDragData(ctxt, p_key, MCdragboard, r_result);
}

void MCPasteboardEvalIsNotAmongTheKeysOfTheRawDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    MCPasteboardEvalIsAmongTheKeysOfTheRawClipboardOrDragData(ctxt, p_key, MCdragboard, r_result);
    r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalIsAmongTheKeysOfTheRawClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_key, const MCClipboard* p_clipboard, bool& r_result)
{
    // Ensure there is an active script clipboard
    if (!p_clipboard->IsLocked())
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_NOTLOCKED);
        return;
    }
    
    // TODO: support multiple items
    // Get the first item on the clipboard
    MCAutoRefcounted<const MCRawClipboardItem> t_item = p_clipboard->GetRawClipboard()->GetItemAtIndex(0);
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

//////////

void MCPasteboardEvalIsAmongTheKeysOfTheDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    MCPasteboardEvalIsAmongTheKeysOfLegacy(ctxt, MCdragboard, p_key, r_result);
}

void MCPasteboardEvalIsNotAmongTheKeysOfTheDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
	MCPasteboardEvalIsAmongTheKeysOfTheDragData(ctxt, p_key, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalFullClipboardKeys(MCExecContext& ctxt, MCStringRef& r_keys)
{
    MCPasteboardEvalFullClipboardOrDragKeys(ctxt, MCclipboard, r_keys);
}

void MCPasteboardEvalFullDragKeys(MCExecContext& ctxt, MCStringRef& r_keys)
{
    MCPasteboardEvalFullClipboardOrDragKeys(ctxt, MCdragboard, r_keys);
}

void MCPasteboardEvalFullClipboardOrDragKeys(MCExecContext& ctxt, const MCClipboard* p_clipboard, MCStringRef& r_keys)
{
    // Create a list for the keys
    MCAutoListRef t_list;
    if (!MCListCreateMutable('\n', &t_list))
    {
        ctxt.Throw();
        return;
    }
    
    // Lock the clipboard
    p_clipboard->Lock();
    
    // Check for the auto-converted text types
    bool t_success = true;
    if (t_success && (p_clipboard->HasTextOrCompatible() || p_clipboard->HasFileList()))
        t_success = MCListAppend(*t_list, MCN_text);
    if (t_success && p_clipboard->HasLiveCodeStyledTextOrCompatible())
    {
        t_success = MCListAppend(*t_list, MCN_rtftext)
                    && MCListAppend(*t_list, MCN_htmltext)
                    && MCListAppend(*t_list, MCN_styles)
                    && MCListAppend(*t_list, MCN_styledtext);
    }
    
    // Check for any image type
    if (t_success && p_clipboard->HasImage())
        t_success = MCListAppend(*t_list, MCN_image);
    
    // Check for specific image types
    if (t_success && p_clipboard->HasPNG())
        t_success = MCListAppend(*t_list, MCN_png);
    if (t_success && p_clipboard->HasGIF())
        t_success = MCListAppend(*t_list, MCN_gif);
    if (t_success && p_clipboard->HasJPEG())
        t_success = MCListAppend(*t_list, MCN_jpeg);
	if (t_success && p_clipboard->HasBMP())
		t_success = MCListAppend(*t_list, MCN_win_bitmap);
	if (t_success && p_clipboard->HasWinMetafile())
		t_success = MCListAppend(*t_list, MCN_win_metafile);
	if (t_success && p_clipboard->HasWinEnhMetafile())
		t_success = MCListAppend(*t_list, MCN_win_enh_metafile);
    
    // Check for specific styled text formats. These are the "true" formats and
    // aren't round-tripped via a LiveCode field.
    if (t_success && p_clipboard->HasRTF())
        t_success = MCListAppend(*t_list, MCN_rtf);
    if (t_success && p_clipboard->HasHTML())
        t_success = MCListAppend(*t_list, MCN_html);
    
    // Check for serialised LiveCode objects
    if (t_success && p_clipboard->HasLiveCodeObjects())
        t_success = MCListAppend(*t_list, MCN_objects);
    
    // Check for a file list
    if (t_success && p_clipboard->HasFileList())
        t_success = MCListAppend(*t_list, MCN_files);
    
    // Check for private data
    if (t_success && p_clipboard->HasPrivateData())
        t_success = MCListAppend(*t_list, MCN_private);
    
    // Copy the list to the output
    if (t_success)
        t_success = MCListCopyAsString(*t_list, r_keys);
    
    // Unlock the clipboard
    p_clipboard->Unlock();
    
    // Final check for errors
    if (!t_success)
        ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalIsAmongTheKeysOfTheFullClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    MCPasteboardEvalIsAmongTheKeysOfTheFullClipboardOrDragData(ctxt, p_key, MCclipboard, r_result);
}

void MCPasteboardEvalIsNotAmongTheKeysOfTheFullClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    MCPasteboardEvalIsAmongTheKeysOfTheFullClipboardOrDragData(ctxt, p_key, MCclipboard, r_result);
    r_result = !r_result;
}

void MCPasteboardEvalIsAmongTheKeysOfTheFullDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    MCPasteboardEvalIsAmongTheKeysOfTheFullClipboardOrDragData(ctxt, p_key, MCdragboard, r_result);
}

void MCPasteboardEvalIsNotAmongTheKeysOfTheFullDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result)
{
    MCPasteboardEvalIsAmongTheKeysOfTheFullClipboardOrDragData(ctxt, p_key, MCdragboard, r_result);
    r_result = !r_result;
}

void MCPasteboardEvalIsAmongTheKeysOfTheFullClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_key, const MCClipboard* p_clipboard, bool& r_result)
{
    // Convert the key into a transfer type
    MCTransferType t_type = MCPasteboardTransferTypeFromName(p_key);
    if (t_type == TRANSFER_TYPE_NULL)
        r_result = false;
    
    // Lock the clipboard
    p_clipboard->Lock();
    
    // Is this transfer type present on the clipboard?
    switch (t_type)
    {
        case TRANSFER_TYPE_TEXT:
            r_result = p_clipboard->HasTextOrCompatible() || p_clipboard->HasFileList();
            break;
            
        case TRANSFER_TYPE_HTML_TEXT:
        case TRANSFER_TYPE_RTF_TEXT:
        case TRANSFER_TYPE_STYLED_TEXT:
        case TRANSFER_TYPE_STYLED_TEXT_ARRAY:
            r_result = p_clipboard->HasLiveCodeStyledTextOrCompatible();
            break;
            
        case TRANSFER_TYPE_HTML:
            r_result = p_clipboard->HasHTML();
            break;
            
        case TRANSFER_TYPE_RTF:
            r_result = p_clipboard->HasRTF();
            break;
            
        case TRANSFER_TYPE_IMAGE:
            r_result = p_clipboard->HasImage();
            break;
            
        case TRANSFER_TYPE_PNG:
            r_result = p_clipboard->HasPNG();
            break;
            
        case TRANSFER_TYPE_GIF:
            r_result = p_clipboard->HasGIF();
            break;
            
        case TRANSFER_TYPE_JPEG:
            r_result = p_clipboard->HasJPEG();
            break;

		case TRANSFER_TYPE_BMP:
			r_result = p_clipboard->HasBMP();
			break;

		case TRANSFER_TYPE_WIN_METAFILE:
			r_result = p_clipboard->HasWinMetafile();
			break;

		case TRANSFER_TYPE_WIN_ENH_METAFILE:
			r_result = p_clipboard->HasWinEnhMetafile();
			break;
            
        case TRANSFER_TYPE_FILES:
            r_result = p_clipboard->HasFileList();
            break;
        
        case TRANSFER_TYPE_PRIVATE:
            r_result = p_clipboard->HasPrivateData();
            break;
            
        case TRANSFER_TYPE_NULL:
        case TRANSFER_TYPE_UNICODE_TEXT:    // Legacy only - not supported
        default:
            r_result = false;
            break;
    }
    
    // Unlock the clipboard
    p_clipboard->Unlock();
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
                if (p_targets[i] . object -> del(true))
                {
                    if (p_targets[i] . object -> gettype() == CT_STACK)
                        MCtodestroy -> remove(static_cast<MCStack *>(p_targets[i] . object));
                    p_targets[i] . object -> scheduledelete();
                }
                else if (!MCeerror -> isempty())
                {
                    ctxt . Throw();
                    return;
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
	if (MCactivefield)
		MCactivefield -> copytext();
	else if (MCactiveimage)
		MCactiveimage -> copyimage();
	else if (!MCselected -> copy() && !MCeerror->isempty())
        ctxt . Throw();
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
	if (MCactivefield)
		MCactivefield -> cuttext();
	else if (MCactiveimage)
		MCactiveimage -> cutimage();
    else if (!MCselected -> cut() && !MCeerror->isempty())
        ctxt . Throw();
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

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardGetClipboardOrDragDataLegacy(MCExecContext& ctxt, MCNameRef p_index, bool p_is_clipboard, MCValueRef& r_data)
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
        // Convert the key name into a transfer type
        MCTransferType t_type;
        if (p_index == nil)
            t_type = TRANSFER_TYPE_TEXT;
        else
            t_type = MCPasteboardTransferTypeFromName(p_index, true);
        
        // Get the requested data from the clipboard
        t_success = MCPasteboardCopyAsTypeForLegacyClipboard(t_clipboard, t_type, r_data);
        
        // If the requested data format was not on the clipboard, set the
        // result to indicate this.
        if (!t_success)
		{
			r_data = MCValueRetain(kMCEmptyData);
			ctxt . SetTheResultToStaticCString("format not available");
		}
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

void MCPasteboardSetClipboardOrDragDataLegacy(MCExecContext& ctxt, MCNameRef p_index, bool p_is_clipboard, MCValueRef p_data)
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
        t_type = MCPasteboardTransferTypeFromName(p_index, true);

    bool t_success;
    t_success = true;
    
    // Lock the clipboard so no other changes happen as we modify it.
    bool t_clipboard_locked;
    t_success = t_clipboard_locked = t_clipboard->Lock();
    
    // Clear the clipboard contents - only one representation can be placed onto
    // the legacy clipboard.
    t_clipboard->Clear();
    
    // If we have both a valid transfer type and some data, add it to the
    // clipboard under the appropriate type.
    //
    // If either of these is missing, do nothing but don't throw an error.
	if (t_type != TRANSFER_TYPE_NULL && p_data != nil && !MCValueIsEmpty(p_data))
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
				if (MCValueGetTypeCode(p_data) != kMCValueTypeCodeString)
				{
					// Legacy behaviour: treat data as native-encoded text
					MCAutoStringRef t_html;
					t_success = ctxt.ConvertToString(p_data, &t_html);
					if (t_success)
						t_success = t_clipboard->AddHTMLText(*t_html);
				}
				else
				{
                        t_success = t_clipboard->AddHTMLText(static_cast<MCStringRef>(p_data));
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
                    t_success = t_clipboard->AddLiveCodeStyledTextArray(kMCEmptyArray);
				
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
	MCPasteboardGetClipboardOrDragDataLegacy(ctxt, p_index, true, r_data);
}

void MCPasteboardSetClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data)
{
	MCPasteboardSetClipboardOrDragDataLegacy(ctxt, p_index, true, p_data);
}

void MCPasteboardGetDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data)
{
	MCPasteboardGetClipboardOrDragDataLegacy(ctxt, p_index, false, r_data);
}

void MCPasteboardSetDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data)
{
	MCPasteboardSetClipboardOrDragDataLegacy(ctxt, p_index, false, p_data);
}

void MCPasteboardGetClipboardTextData(MCExecContext& ctxt, MCValueRef& r_data)
{
	MCPasteboardGetClipboardOrDragDataLegacy(ctxt, nil, true, r_data);
}

void MCPasteboardSetClipboardTextData(MCExecContext& ctxt, MCValueRef p_data)
{
	MCPasteboardSetClipboardOrDragDataLegacy(ctxt, nil, true, p_data);
}

void MCPasteboardGetDragTextData(MCExecContext& ctxt, MCValueRef& r_data)
{
	MCPasteboardGetClipboardOrDragDataLegacy(ctxt, nil, false, r_data);
}

void MCPasteboardSetDragTextData(MCExecContext& ctxt, MCValueRef p_data)
{
	MCPasteboardSetClipboardOrDragDataLegacy(ctxt, nil, false, p_data);
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardGetRawClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data)
{
    MCPasteboardGetRawClipboardOrDragData(ctxt, p_index, MCclipboard, r_data);
}

void MCPasteboardSetRawClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data)
{
    MCPasteboardSetRawClipboardOrDragData(ctxt, p_index, MCclipboard, p_data);
}

void MCPasteboardGetRawDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data)
{
    MCPasteboardGetRawClipboardOrDragData(ctxt, p_index, MCdragboard, r_data);
}

void MCPasteboardSetRawDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data)
{
    MCPasteboardSetRawClipboardOrDragData(ctxt, p_index, MCdragboard, p_data);
}

void MCPasteboardGetRawClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_index, const MCClipboard* p_clipboard, MCValueRef& r_data)
{
    // Ensure there is an active script clipboard
    if (!p_clipboard->IsLocked())
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_NOTLOCKED);
        return;
    }
    
    // TODO: support multiple items
    // Get the first item on the clipboard
    MCAutoRefcounted<const MCRawClipboardItem> t_item = p_clipboard->GetRawClipboard()->GetItemAtIndex(0);
    if (t_item == NULL)
    {
        // Clipboard is empty so the key is not present
        ctxt.LegacyThrow(EE_CLIPBOARD_BADREP);
    }
    else
    {
        // Attempt to get the data for this item
        const MCRawClipboardItemRep* t_rep = t_item->FetchRepresentationByType(MCNameGetString(p_index));
        if (t_rep == NULL)
            ctxt.LegacyThrow(EE_CLIPBOARD_BADREP);
        else
        {
            // Get the data for this representation
            MCDataRef t_data = t_rep->CopyData();
            if (t_data == NULL)
                ctxt.LegacyThrow(EE_CLIPBOARD_BADREP);
            r_data = t_data;
        }
    }
}

void MCPasteboardSetRawClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_index, MCClipboard* p_clipboard, MCValueRef p_data)
{
    // Ensure there is an active script clipboard
    if (!p_clipboard->IsLocked())
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_NOTLOCKED);
        return;
    }
    
    // We cannot write to the clipboard if it contains externally-sourced data
    MCRawClipboard* t_raw_clipboard = p_clipboard->GetRawClipboard();
    if (t_raw_clipboard->IsExternalData())
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_EXTERNALDATA);
        return;
    }
    
    // TODO: support multiple items
    // Get the first item in the clipboard
    MCAutoRefcounted<MCRawClipboardItem> t_item = t_raw_clipboard->GetItemAtIndex(0);
    
    // If there is no item on the clipboard yet, we will have to allocate one
    if (t_item == NULL)
    {
        t_item = t_raw_clipboard->CreateNewItem();
        if (!t_raw_clipboard->AddItem(t_item))
        {
            ctxt.LegacyThrow(EE_CLIPBOARD_INSERTFAILED);
            return;
        }
    }
    
    // Convert the incoming valueref to data
    MCAutoDataRef t_data;
    if (!ctxt.ConvertToData(p_data, &t_data))
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_BADREP);
        return;
    }
    
    // Add the representation to the clipboard item
    if (!t_item->AddRepresentation(MCNameGetString(p_index), *t_data))
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_BADREP);
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardGetFullClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_value)
{
    MCPasteboardGetFullClipboardOrDragData(ctxt, p_index, MCclipboard, r_value);
}

void MCPasteboardSetFullClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_value)
{
    MCPasteboardSetFullClipboardOrDragData(ctxt, p_index, MCclipboard, p_value);
}

void MCPasteboardGetFullDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_value)
{
    MCPasteboardGetFullClipboardOrDragData(ctxt, p_index, MCdragboard, r_value);
}

void MCPasteboardSetFullDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_value)
{
    MCPasteboardSetFullClipboardOrDragData(ctxt, p_index, MCdragboard, p_value);
}

void MCPasteboardGetFullClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_index, const MCClipboard* p_clipboard, MCValueRef& r_value)
{
    // Convert the key to a transfer type
    MCTransferType t_type = MCPasteboardTransferTypeFromName(p_index);
    
    // Lock the clipboard
    p_clipboard->Lock();
    
    // Try to copy the data from the clipboard, as requested.
    MCAutoValueRef t_data;
    switch (t_type)
    {
        case TRANSFER_TYPE_TEXT:
            p_clipboard->CopyAsText((MCStringRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_RTF_TEXT:
            p_clipboard->CopyAsRTFText((MCDataRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_HTML_TEXT:
            p_clipboard->CopyAsHTMLText((MCStringRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_STYLED_TEXT:
            p_clipboard->CopyAsLiveCodeStyledText((MCDataRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_STYLED_TEXT_ARRAY:
            p_clipboard->CopyAsLiveCodeStyledTextArray((MCArrayRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_RTF:
            p_clipboard->CopyAsRTF((MCDataRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_HTML:
            p_clipboard->CopyAsHTML((MCStringRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_IMAGE:
            p_clipboard->CopyAsImage((MCDataRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_PNG:
            p_clipboard->CopyAsPNG((MCDataRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_GIF:
            p_clipboard->CopyAsGIF((MCDataRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_JPEG:
            p_clipboard->CopyAsJPEG((MCDataRef&)&t_data);
            break;

		case TRANSFER_TYPE_BMP:
			p_clipboard->CopyAsBMP((MCDataRef&)&t_data);
			break;

		case TRANSFER_TYPE_WIN_METAFILE:
			p_clipboard->CopyAsWinMetafile((MCDataRef&)&t_data);
			break;

		case TRANSFER_TYPE_WIN_ENH_METAFILE:
			p_clipboard->CopyAsWinEnhMetafile((MCDataRef&)&t_data);
			break;
            
        case TRANSFER_TYPE_OBJECTS:
            p_clipboard->CopyAsLiveCodeObjects((MCDataRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_FILES:
            p_clipboard->CopyAsFileList((MCStringRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_PRIVATE:
            p_clipboard->CopyAsPrivateData((MCDataRef&)&t_data);
            break;
            
        case TRANSFER_TYPE_NULL:
        case TRANSFER_TYPE_UNICODE_TEXT:    // Legacy only - not supported
        default:
            break;
    }
    
    // Unlock the clipboard
    p_clipboard->Unlock();
    
    // Did we manage to get some data?
    if (*t_data == NULL)
        ctxt.LegacyThrow(EE_CLIPBOARD_BADREP);
    else
        r_value = MCValueRetain(*t_data);
}

void MCPasteboardSetFullClipboardOrDragData(MCExecContext& ctxt, MCNameRef p_index, MCClipboard* p_clipboard, MCValueRef p_value)
{
    // Lock the clipboard
    p_clipboard->Lock();
    
    // We can't write to the clipboard if it contains external data -- it needs
    // to be cleared first. Do that automatically.
    if (p_clipboard->GetRawClipboard()->IsExternalData())
        p_clipboard->Clear();

    // What transfer type is desired?
    MCTransferType t_type = MCPasteboardTransferTypeFromName(p_index);
    
    // Attempt to copy the data to the clipboard
    MCAutoStringRef t_string;
    MCAutoDataRef t_data;
    MCAutoArrayRef t_array;
    bool t_success = false;
    switch (t_type)
    {
        case TRANSFER_TYPE_TEXT:
            if (ctxt.ConvertToString(p_value, &t_string))
                t_success = p_clipboard->AddText(*t_string);
            break;
            
        case TRANSFER_TYPE_HTML_TEXT:
            if (ctxt.ConvertToString(p_value, &t_string))
                t_success = p_clipboard->AddHTMLText(*t_string);
            break;
            
        case TRANSFER_TYPE_RTF_TEXT:
            if (ctxt.ConvertToData(p_value, &t_data))
                t_success = p_clipboard->AddRTFText(*t_data);
            break;
            
        case TRANSFER_TYPE_STYLED_TEXT:
            if (ctxt.ConvertToData(p_value, &t_data))
                t_success = p_clipboard->AddLiveCodeStyledText(*t_data);
            break;
            
        case TRANSFER_TYPE_STYLED_TEXT_ARRAY:
            if (ctxt.ConvertToArray(p_value, &t_array))
                t_success = p_clipboard->AddLiveCodeStyledTextArray(*t_array);
            break;
            
        case TRANSFER_TYPE_RTF:
            if (ctxt.ConvertToData(p_value, &t_data))
                t_success = p_clipboard->AddRTF(*t_data);
            break;
            
        case TRANSFER_TYPE_HTML:
            if (ctxt.ConvertToString(p_value, &t_string))
                t_success = p_clipboard->AddHTML(*t_string);
            break;
            
        case TRANSFER_TYPE_IMAGE:
            if (ctxt.ConvertToData(p_value, &t_data))
                t_success = p_clipboard->AddImage(*t_data);
            break;
            
        case TRANSFER_TYPE_PNG:
            if (ctxt.ConvertToData(p_value, &t_data))
                t_success = p_clipboard->AddPNG(*t_data);
            break;
            
        case TRANSFER_TYPE_GIF:
            if (ctxt.ConvertToData(p_value, &t_data))
                t_success = p_clipboard->AddGIF(*t_data);
            break;
        
        case TRANSFER_TYPE_JPEG:
            if (ctxt.ConvertToData(p_value, &t_data))
                t_success = p_clipboard->AddJPEG(*t_data);
            break;

		case TRANSFER_TYPE_BMP:
			if (ctxt.ConvertToData(p_value, &t_data))
				t_success = p_clipboard->AddBMP(*t_data);
			break;

		case TRANSFER_TYPE_WIN_METAFILE:
			if (ctxt.ConvertToData(p_value, &t_data))
				t_success = p_clipboard->AddWinMetafile(*t_data);
			break;

		case TRANSFER_TYPE_WIN_ENH_METAFILE:
			if (ctxt.ConvertToData(p_value, &t_data))
				t_success = p_clipboard->AddWinEnhMetafile(*t_data);
			break;
            
        case TRANSFER_TYPE_FILES:
            if (ctxt.ConvertToString(p_value, &t_string))
                t_success = p_clipboard->AddFileList(*t_string);
            break;
            
        case TRANSFER_TYPE_OBJECTS:
            if (ctxt.ConvertToData(p_value, &t_data))
                t_success = p_clipboard->AddLiveCodeObjects(*t_data);
            break;
            
        case TRANSFER_TYPE_PRIVATE:
            if (ctxt.ConvertToData(p_value, &t_data))
                t_success = p_clipboard->AddPrivateData(*t_data);
            break;
            
        case TRANSFER_TYPE_UNICODE_TEXT:    // Legacy only -- not supported
        case TRANSFER_TYPE_NULL:
        default:
            break;
    }
    
    // Unlock the clipboard
    p_clipboard->Unlock();
    
    // Did the data get added successfully?
    if (!t_success)
        ctxt.LegacyThrow(EE_CLIPBOARD_BADREP);
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardGetRawClipboardTextData(MCExecContext& ctxt, MCValueRef& r_value)
{
    MCPasteboardGetRawClipboardOrDragTextData(ctxt, MCclipboard, r_value);
}

void MCPasteboardSetRawClipboardTextData(MCExecContext& ctxt, MCValueRef p_value)
{
    MCPasteboardSetRawClipboardOrDragTextData(ctxt, MCclipboard, p_value);
}

void MCPasteboardGetRawDragTextData(MCExecContext& ctxt, MCValueRef& r_value)
{
    MCPasteboardGetRawClipboardOrDragTextData(ctxt, MCdragboard, r_value);
}

void MCPasteboardSetRawDragTextData(MCExecContext& ctxt, MCValueRef p_value)
{
    MCPasteboardSetRawClipboardOrDragTextData(ctxt, MCdragboard, p_value);
}

void MCPasteboardGetRawClipboardOrDragTextData(MCExecContext& ctxt, const MCClipboard* p_clipboard, MCValueRef& r_value)
{
    // Ensure there is an active script clipboard
    if (!p_clipboard->IsLocked())
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_NOTLOCKED);
        return;
    }
    
    // Calling this method is always an error
    ctxt.LegacyThrow(EE_CLIPBOARD_BADREP);
}

void MCPasteboardSetRawClipboardOrDragTextData(MCExecContext& ctxt, MCClipboard* p_clipboard, MCValueRef p_value)
{
    // Ensure there is an active script clipboard
    if (!p_clipboard->IsLocked())
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_NOTLOCKED);
        return;
    }
    
    // Ensure that the incoming value is an array
    MCAutoArrayRef t_array;
    if (!ctxt.ConvertToArray(p_value, &t_array))
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_BADREP);
        return;
    }
    
    // Clear the clipboard. Even if adding one of the items in the array fails,
    // we will guarantee that the old clipboard contents have been removed.
    p_clipboard->Clear();
    
    // Place each item in the array onto the clipboard
    if (!MCValueIsEmpty(*t_array))
    {
        uintptr_t t_iterator = 0;
        MCNameRef t_key;
        MCValueRef t_value;
        while (MCArrayIterate(*t_array, t_iterator, t_key, t_value))
        {
            // Attempt to add this item to the clipboard
            MCPasteboardSetRawClipboardOrDragData(ctxt, t_key, p_clipboard, t_value);
            if (ctxt.HasError())
                break;
        }
    }
    
    // If an error occurred while writing the clipboard, clear the partial write
    if (ctxt.HasError())
        p_clipboard->Clear();
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardGetFullClipboardTextData(MCExecContext& ctxt, MCValueRef& r_value)
{
    MCPasteboardGetFullClipboardOrDragTextData(ctxt, MCclipboard, r_value);
}

void MCPasteboardSetFullClipboardTextData(MCExecContext& ctxt, MCValueRef p_value)
{
    MCPasteboardSetFullClipboardOrDragTextData(ctxt, MCclipboard, p_value);
}

void MCPasteboardGetFullDragTextData(MCExecContext& ctxt, MCValueRef& r_value)
{
    MCPasteboardGetFullClipboardOrDragTextData(ctxt, MCdragboard, r_value);
}

void MCPasteboardSetFullDragTextData(MCExecContext& ctxt, MCValueRef p_value)
{
    MCPasteboardSetFullClipboardOrDragTextData(ctxt, MCdragboard, p_value);
}

void MCPasteboardGetFullClipboardOrDragTextData(MCExecContext& ctxt, const MCClipboard* p_clipboard, MCValueRef& r_value)
{
    // Calling this method is always an error
    ctxt.LegacyThrow(EE_CLIPBOARD_BADREP);
}

void MCPasteboardSetFullClipboardOrDragTextData(MCExecContext& ctxt, MCClipboard* p_clipboard, MCValueRef p_value)
{
    // Ensure that the incoming value is an array
    MCAutoArrayRef t_array;
    if (!ctxt.ConvertToArray(p_value, &t_array))
    {
        ctxt.LegacyThrow(EE_CLIPBOARD_BADREP);
        return;
    }
    
    // Lock the clipboard implicitly
    p_clipboard->Lock();
    
    // Clear the clipboard. Even if adding one of the items in the array fails,
    // we will guarantee that the old clipboard contents have been removed.
    p_clipboard->Clear();
    
    // Place each item in the array onto the clipboard
    if (!MCValueIsEmpty(*t_array))
    {
        uintptr_t t_iterator = 0;
        MCNameRef t_key;
        MCValueRef t_value;
        while (MCArrayIterate(*t_array, t_iterator, t_key, t_value))
        {
            // Attempt to add this item to the clipboard
            MCPasteboardSetFullClipboardOrDragData(ctxt, t_key, p_clipboard, t_value);
            if (ctxt.HasError())
                break;
        }
    }
    
    // If an error occurred while writing the clipboard, clear the partial write
    if (ctxt.HasError())
        p_clipboard->Clear();
    
    // Unlock the clipboard to confirm the changes
    p_clipboard->Unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardEvalDragSourceAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCdragsource)
    {
        r_object . object = MCdragsource;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCPasteboardEvalDragDestinationAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCdragdest)
    {
        r_object . object = MCdragdest;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCPasteboardEvalDropChunkAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCdragdest)
    {
        r_object . object = MCdropfield;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

////////////////////////////////////////////////////////////////////////////////

void MCPasteboardExecLockClipboard(MCExecContext& ctxt)
{
    // Lock the main clipboard. This has the side-effect of pulling updates.
    MCclipboard->Lock();
    MCclipboardlockcount++;
}

void MCPasteboardExecUnlockClipboard(MCExecContext& ctxt)
{
    // Unlock the main clipboard. This will push any changes to the system.
    if (MCclipboardlockcount > 0)
    {
        MCclipboardlockcount--;
        MCclipboard->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////
