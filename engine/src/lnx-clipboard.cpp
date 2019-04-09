/* Copyright (C) 2015 LiveCode Ltd.
 
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


#include "lnx-clipboard.h"

#include <gdk/gdk.h>

#include "globals.h"
#include "lnxdc.h"
#include "lnxprefix.h"
#include "util.h"


// Functions used to load the weakly-linked GDK library
extern "C" bool initialise_weak_link_X11();
extern "C" bool initialise_weak_link_gobject();
extern "C" bool initialise_weak_link_gdk();


GdkWindow* MCLinuxRawClipboard::s_clipboard_window = NULL;

const char * const MCLinuxRawClipboard::s_formats[kMCRawClipboardKnownTypeLast+1] =
{
    "UTF8_STRING",                  // UTF-8 text
    "text/plain;charset=utf-16",    // UTF-16 text
    "STRING",                       // ISO-8859-1 text
    NULL,                           // MacRoman text
    NULL,                           // Windows codepage 1252 text
    
    "text/rtf",                     // Rich Text format
    "text/html",                    // HTML
    
    "image/png",                    // PNG image
    "image/gif",                    // GIF image
    "image/jpeg",                   // JPEG image
	"image/tiff",					// TIFF image
    NULL,                           // Windows metafile
    NULL,                           // Windows enhanced metafile
    NULL,                           // Windows bitmap
    NULL,                           // Windows v5 bitmap
    
    "application/x-revolution",     // LiveCode serialised objects
    "text/x-revolution-styled",     // LiveCode styled text
    
    NULL,                           // File path
    "text/uri-list",                // File path list
    NULL,                           // URL
    NULL,                           // URL list
};


MCRawClipboard* MCRawClipboard::CreateSystemClipboard()
{
    return new MCLinuxRawClipboard(GDK_SELECTION_CLIPBOARD);
}

MCRawClipboard* MCRawClipboard::CreateSystemSelectionClipboard()
{
    return new MCLinuxRawClipboard(GDK_SELECTION_PRIMARY);
}

MCRawClipboard* MCRawClipboard::CreateSystemDragboard()
{
    return new MCLinuxRawClipboard("XdndSelection");
}


MCLinuxRawClipboard::MCLinuxRawClipboard(const char* p_selection_atom_name) :
  MCRawClipboard(),
  m_selection(0),
  m_item(NULL),
  m_selected_item(NULL),
  m_window(NULL),
  m_drag_context(NULL),
  m_dirty(false),
  m_external_data(false),
  m_owned(false)
{
    // If GDK isn't available, don't try to use it. We can use the clipboard
    // without it but only within this LiveCode instance.
    if (HasGDK())
        m_selection = gdk_atom_intern(p_selection_atom_name, FALSE);
}

MCLinuxRawClipboard::MCLinuxRawClipboard(GdkAtom p_selection) :
  MCRawClipboard(),
  m_selection(p_selection),
  m_item(NULL),
  m_selected_item(NULL),
  m_window(NULL),
  m_drag_context(NULL),
  m_dirty(false),
  m_external_data(false),
  m_owned(false)
{
    ;
}

MCLinuxRawClipboard::~MCLinuxRawClipboard()
{
    // Relinquish the selection, if held
    if (HasGDK() && IsOwned())
        gdk_selection_owner_set_for_display(GetDisplay(), NULL, m_selection, GDK_CURRENT_TIME, FALSE);
    
    // Ensure all resources are freed
    LostSelection();
    Clear();
}


uindex_t MCLinuxRawClipboard::GetItemCount() const
{
    // The X11 clipboard can only hold a single item
    return m_item == NULL ? 0 : 1;
}

const MCLinuxRawClipboardItem* MCLinuxRawClipboard::GetItemAtIndex(uindex_t p_index) const
{
    // Only a single item is supported
    if (p_index > 0 || m_item == NULL)
        return NULL;
    m_item->Retain();
    return m_item;
}

MCLinuxRawClipboardItem* MCLinuxRawClipboard::GetItemAtIndex(uindex_t p_index)
{
    // Only a single item is supported
    if (p_index > 0 || m_item == NULL)
        return NULL;
    m_item->Retain();
    return m_item;
}

void MCLinuxRawClipboard::Clear()
{
    // Discard any data we currently have stored
    m_dirty = true;
    m_external_data = false;
    if (m_item)
        m_item->Release();
    m_item = NULL;
}

bool MCLinuxRawClipboard::IsExternalData() const
{
    return m_external_data;
}

MCLinuxRawClipboardItem* MCLinuxRawClipboard::CreateNewItem()
{
    // Fail if there is an existing item on the clipboard
    if (m_item != NULL)
        return NULL;
    
    // Create a new data item
    m_item = new (nothrow) MCLinuxRawClipboardItem(this);
    m_item->Retain();
    return m_item;
}

bool MCLinuxRawClipboard::AddItem(MCRawClipboardItem* p_item)
{
    // Fail if this is not the single item belonging to this clipboard
    if (m_item != p_item)
        return false;
    
    // Item was already added at creation time
    m_dirty = true;
    return true;
}

uindex_t MCLinuxRawClipboard::GetMaximumItemCount() const
{
    // Only a single item is supported
    return 1;
}

MCStringRef MCLinuxRawClipboard::GetKnownTypeString(MCRawClipboardKnownType p_type) const
{
    // Index into the mapping table
    if (p_type > kMCRawClipboardKnownTypeLast)
        return NULL;
    if (s_formats[p_type] == NULL)
        return NULL;
    return MCSTR(s_formats[p_type]);
}

MCDataRef MCLinuxRawClipboard::EncodeFileListForTransfer(MCStringRef p_list) const
{
    // Split the string up into individual paths
    MCAutoArrayRef t_path_list;
    if (!MCStringSplit(p_list, MCSTR("\n"), NULL, kMCStringOptionCompareExact, &t_path_list))
        return NULL;
    
    // Create an output string
    MCAutoStringRef t_modified_path_list;
    if (!MCStringCreateMutable(0, &t_modified_path_list))
        return NULL;
    
    // Loop over the paths in the file list
    for (uindex_t i = 1; i <= MCArrayGetCount(*t_path_list); i++)
    {
        // Get the path at this index (and sanity-check it)
        MCValueRef t_path;
        if (!MCArrayFetchValueAtIndex(*t_path_list, i, t_path))
            return NULL;
        MCAssert(MCValueGetTypeCode(t_path) == kMCValueTypeCodeString);
        
        // Encode the string as UTF-8 data and then URL encode it
        MCAutoStringRef t_encoded_path;
        MCU_urlencode((MCStringRef)t_path, true, &t_encoded_path);
        
        // Unfortunately, URL encoding also replaces the '/' characters - undo
        // that.
        MCAutoStringRef t_fixed_path;
        if (!MCStringMutableCopy(*t_encoded_path, &t_fixed_path))
            return NULL;
        if (!MCStringFindAndReplace(*t_fixed_path, MCSTR("%2F"), MCSTR("/"), kMCStringOptionCompareExact))
            return NULL;
        
        // Build up the modified path. The XDND protocol specifies that paths
        // are given as fully-qualified URLs but, in practice, a large number
        // of apps elide the hostname. We do that here for simplicity.
        //
        // Note that the line termination is CRLF
        if (!MCStringAppendFormat(*t_modified_path_list, "file://%@%@\r\n", kMCEmptyString, *t_fixed_path))
            return NULL;
    }
    
    // Convert the encoded string to data (we just want to grab the bytes so the
    // native encoding is used here).
    MCDataRef t_encoded;
    if (!MCStringEncode(*t_modified_path_list, kMCStringEncodingNative, false, t_encoded))
        return NULL;
    
    // Done
    return t_encoded;
}

MCStringRef MCLinuxRawClipboard::DecodeTransferredFileList(MCDataRef p_data) const
{
    // We need to convert the data to a string, preserving all the byte values,
    // as the URL decoding routine expects a stringref.
    MCAutoStringRef t_encoded_string;
    if (!MCStringCreateWithBytes(MCDataGetBytePtr(p_data), MCDataGetLength(p_data), kMCStringEncodingNative, false, &t_encoded_string))
        return NULL;
    
    // URL-decode the string and convert from UTF-8
    MCAutoStringRef t_string;
    MCU_urldecode(*t_encoded_string, true, &t_string);
    
    // Split the string on CRLF sequences
    MCAutoArrayRef t_uri_list;
    if (!MCStringSplit(*t_string, MCSTR("\r\n"), NULL, kMCStringOptionCompareExact, &t_uri_list))
        return NULL;
    
    // Output list
    MCAutoListRef t_output_list;
    if (!MCListCreateMutable('\n', &t_output_list))
        return NULL;
    
    // Loop over the paths and convert to the expected form
    for (uindex_t i = 1; i <= MCArrayGetCount(*t_uri_list); i++)
    {
        // Get the path at this index (and sanity-check it)
        MCStringRef t_path;
        if (!MCArrayFetchValueAtIndex(*t_uri_list, i, (MCValueRef&)t_path))
            return NULL;
        MCAssert(MCValueGetTypeCode(t_path) == kMCValueTypeCodeString);
        
        // Create a mutable copy of this path
        MCAutoStringRef t_modified_path;
        if (!MCStringMutableCopy(t_path, &t_modified_path))
            return NULL;
        
        // Remove any "file://" prefix that the path has
        if (MCStringBeginsWithCString(*t_modified_path, (const char_t*)"file:///", kMCStringOptionCompareExact))
        {
            // Remove the "file://" prefix
            MCStringRemove(*t_modified_path, MCRangeMake(0, 7));
        }
        else if (MCStringBeginsWithCString(*t_modified_path, (const char_t*)"file://", kMCStringOptionCompareExact))
        {
            // Find the end of the hostname portion of the path
            uindex_t t_end;
            if (!MCStringFirstIndexOfChar(*t_modified_path, '/', 7, kMCStringOptionCompareExact, t_end))
                return NULL;
            MCStringRemove(*t_modified_path, MCRangeMake(0, t_end-1));
        }
        
        // Append to the list
        if (!MCListAppend(*t_output_list, *t_modified_path))
            return NULL;
    }
    
    // Return the list as a string
    MCStringRef t_paths;
    if (!MCListCopyAsString(*t_output_list, t_paths))
        return NULL;
    return t_paths;
}

MCDataRef MCLinuxRawClipboard::EncodeHTMLFragmentForTransfer(MCDataRef p_html) const
{
	return MCValueRetain(p_html);
}

MCDataRef MCLinuxRawClipboard::DecodeTransferredHTML(MCDataRef p_html) const
{
	return MCValueRetain(p_html);
}

const MCLinuxRawClipboardItem* MCLinuxRawClipboard::GetSelectionItem() const
{
    if (m_selected_item)
        m_selected_item->Retain();
    return m_selected_item;
}

GdkAtom MCLinuxRawClipboard::CopyAtomForType(MCStringRef p_type)
{
    // Check for a valid type
    if (!HasGDK() || p_type == NULL || MCStringIsEmpty(p_type))
        return 0;
    
    // Encode the type string as UTF-8
    MCAutoDataRef t_encoded_type;
    if (!MCStringEncode(p_type, kMCStringEncodingUTF8, false, &t_encoded_type))
        return 0;
    
    // Get the atom for this string
    return gdk_atom_intern((const gchar*)MCDataGetBytePtr(*t_encoded_type), FALSE);
}

MCStringRef MCLinuxRawClipboard::CopyTypeForAtom(GdkAtom p_atom)
{
    // Check for a valid atom
    if (!HasGDK() || p_atom == 0)
        return NULL;
    
    // Get the C-style tring associated with this atom
    gchar* t_atom_name = gdk_atom_name(p_atom);
    if (t_atom_name == NULL)
        return NULL;
    
    // We just assume that the atom name is UTF-8 encoded - it's not actually
    // specified but no other choices are particularly better...
    MCStringRef t_type = NULL;
    MCStringCreateWithBytes((const byte_t*)t_atom_name, strlen(t_atom_name), kMCStringEncodingUTF8, false, t_type);
    
    // Done
    g_free(t_atom_name);
    return t_type;
}

bool MCLinuxRawClipboard::IsOwned() const
{
    // Determining the ownership in GDK isn't reliable so we need to track it
    // ourselves
    return m_owned;
}

bool MCLinuxRawClipboard::PushUpdates()
{
#ifndef _SERVER
    // Fail if GDK isn't available
    if (!HasGDK())
        return false;
    
    // Do nothing if there are no changes to push
    if (!m_dirty)
        return true;
    
    // Take ownership of the selection
    if (!gdk_selection_owner_set_for_display(GetDisplay(), GetClipboardWindow(), m_selection, GDK_CURRENT_TIME, TRUE))
        return false;
    
    // Copy the item so that we still have the data even if the clipboard is
    // cleared after the push
    if (m_selected_item)
        m_selected_item->Release();
    m_selected_item = m_item;
    if (m_selected_item)
        m_selected_item->Retain();
    
    // We now own the selection
    m_owned = true;
    m_dirty = false;
#endif
    return true;
}

bool MCLinuxRawClipboard::PullUpdates()
{
#ifndef _SERVER
    // Fail if GDK isn't available
    if (!HasGDK())
        return false;
    
    // If we're still the owner of the clipboard, do nothing
    if (IsOwned())
        return true;
    
    // Release the current clipboard contents
    if (m_item)
        m_item->Release();
    if (m_selected_item)
        m_selected_item->Release();
    m_item = m_selected_item = NULL;
    
    // Create a new item for the new contents
    m_external_data = true;
    m_item = new (nothrow) MCLinuxRawClipboardItem(this, m_selection, m_drag_context);
    return (m_item != NULL);
#else
    return true;
#endif
}

bool MCLinuxRawClipboard::FlushData()
{
    // Not supported by the X11 clipboard model
    return false;
}

void MCLinuxRawClipboard::SetDirty()
{
    m_dirty = true;
}

void MCLinuxRawClipboard::LostSelection()
{
    // Release the selection data
    if (m_selected_item)
        m_selected_item->Release();
    m_selected_item = NULL;
    m_owned = false;
}

GdkAtom MCLinuxRawClipboard::GetSelectionAtom() const
{
    return m_selection;
}

MCDataRef MCLinuxRawClipboard::CopyTargets() const
{
#ifndef _SERVER
    // Do nothing if we don't have GDK available
    if (!MCLinuxRawClipboard::HasGDK())
        return NULL;
    
    // Check that we actually have an item
    if (m_selected_item == NULL)
        return NULL;
    
    // Create a DataRef for the atoms
    MCAutoDataRef t_data;
    if (!MCDataCreateMutable(0, &t_data))
        return NULL;
    
    // Add the representations in the form of atoms
    for (uindex_t i = 0; i < m_selected_item->GetRepresentationCount(); i++)
    {
        // Get this representation
        const MCLinuxRawClipboardItemRep* t_rep = m_selected_item->FetchRepresentationAtIndex(i);
        
        // Add it to the data. Note that GDK expects 32-bit atoms to be in the
        // form of gulong, even when sizeof(gulong) > sizeof(uint32_t).
        MCAutoStringRef t_type;
        t_type.Give(t_rep->CopyTypeString());
        gulong t_atom = gulong(CopyAtomForType(*t_type));
        MCDataAppendBytes(*t_data, (const byte_t*)&t_atom, sizeof(t_atom));
    }
    
    // Add some special targets to the list too
    gulong t_targets = gulong(gdk_atom_intern_static_string("TARGETS"));
    gulong t_multiple = gulong(gdk_atom_intern_static_string("MULTIPLE"));
    gulong t_timestamp = gulong(gdk_atom_intern_static_string("TIMESTAMP"));
    MCDataAppendBytes(*t_data, (const byte_t*)&t_targets, sizeof(t_targets));
    MCDataAppendBytes(*t_data, (const byte_t*)&t_multiple, sizeof(t_multiple));
    MCDataAppendBytes(*t_data, (const byte_t*)&t_timestamp, sizeof(t_timestamp));
    
    // Done
    return MCValueRetain(*t_data);
#else
    return NULL;
#endif
}

GdkDisplay* MCLinuxRawClipboard::GetDisplay()
{
#ifndef _SERVER
    return MCdpy;
#else
    return NULL;
#endif
}

GdkWindow* MCLinuxRawClipboard::GetClipboardWindow() const
{
    // Use the custom window, if specified
    if (m_window)
        return m_window;
    
    // Fail if GDK isn't available
    if (!HasGDK())
        return NULL;
    
    // If a window has already been created, just use it
    if (s_clipboard_window != NULL)
        return s_clipboard_window;
    
    // Attributes for the window
    GdkWindowAttr t_attributes;
    memset(&t_attributes, 0, sizeof(t_attributes));
    t_attributes.title = "LiveCode Clipboard Helper";
    t_attributes.event_mask = GDK_ALL_EVENTS_MASK;
    t_attributes.wclass = GDK_INPUT_ONLY;
    t_attributes.window_type = GDK_WINDOW_TOPLEVEL;
    
    // Create the window
    s_clipboard_window = gdk_window_new(NULL, &t_attributes, GDK_WA_TITLE);
    return s_clipboard_window;
}

void MCLinuxRawClipboard::SetClipboardWindow(GdkWindow* p_window)
{
    m_window = p_window;
}

void MCLinuxRawClipboard::SetDragContext(GdkDragContext* p_context)
{
    m_drag_context = p_context;
}

bool MCLinuxRawClipboard::HasGDK()
{
#ifndef _SERVER
    // Only try to initialise GDK once
    static bool s_try_gdk = false;
    static bool s_has_gdk = false;
    if (!s_try_gdk)
    {
        s_try_gdk = true;
        s_has_gdk = !MCnoui && initialise_weak_link_X11()
            && initialise_weak_link_gobject()
            && initialise_weak_link_gdk();
        if (s_has_gdk)
            gdk_init(0, NULL);
    }
    return s_has_gdk;
#else
    return false;
#endif
}


MCLinuxRawClipboardItem::MCLinuxRawClipboardItem(MCLinuxRawClipboard* p_clipboard) :
  MCRawClipboardItem(),
  m_clipboard(p_clipboard),
  m_data_is_external(false),
  m_reps()
{
    ;
}

MCLinuxRawClipboardItem::MCLinuxRawClipboardItem(MCLinuxRawClipboard* p_clipboard, GdkAtom p_selection, GdkDragContext* p_drag_context) :
  MCRawClipboardItem(),
  m_clipboard(p_clipboard),
  m_data_is_external(true),
  m_reps()
{
    // We have to fetch the representations now in case the selection moves to
    // another owner.
    FetchExternalRepresentations(p_drag_context);
}

MCLinuxRawClipboardItem::~MCLinuxRawClipboardItem()
{
    // Delete all of the cached representations
    for (uindex_t i = 0; i < m_reps.Size(); i++)
        delete m_reps[i];
}


uindex_t MCLinuxRawClipboardItem::GetRepresentationCount() const
{
    // All representations are fetched up-front
    return m_reps.Size();
}

const MCLinuxRawClipboardItemRep* MCLinuxRawClipboardItem::FetchRepresentationAtIndex(uindex_t p_index) const
{
    // Is the index valid?
    if (p_index >= GetRepresentationCount())
        return NULL;
    
    // All representations are fetched up-front
    return m_reps[p_index];
}

bool MCLinuxRawClipboardItem::AddRepresentation(MCStringRef p_type, MCDataRef p_bytes)
{
    // Fail if this is externally-provided data
    if (m_data_is_external)
        return false;
    
    // Mark the clipboard as dirty
    m_clipboard->SetDirty();
    
    // Look for an existing representation with this type
    MCLinuxRawClipboardItemRep* t_rep = NULL;
    for (uindex_t i = 0; i < GetRepresentationCount(); i++)
    {
        MCAutoStringRef t_type;
        t_type.Give(m_reps[i]->CopyTypeString());
        if (t_type.IsSet() &&
            MCStringIsEqualTo(*t_type, p_type, kMCStringOptionCompareExact))
        {
            // This is the rep we're looking for. Updates it.
            t_rep = m_reps[i];
            t_rep->m_bytes.Reset(p_bytes);
            break;
        }
    }
    
    // If there wasn't an existing representation, create one now
    if (t_rep == NULL)
    {
        // Extend the representation array to hold this new representation
        uindex_t t_index = m_reps.Size();
        if (!m_reps.Extend(t_index + 1))
            return false;
        
        // Allocate a new representation object.
        m_reps[t_index] = t_rep = new (nothrow) MCLinuxRawClipboardItemRep(p_type, p_bytes);
    }
    
    // If we still have no rep, something went wrong
    if (t_rep == NULL)
        return false;
    
    // Various built-in formats may be offered under multiple MIME-types. This
    // duplication is based on the keys provided by LibreOffice and is done so
    // that the largest number of apps will understand the clipboard data.
    if (MCStringIsEqualToCString(p_type, "text/rtf", kMCStringOptionCompareExact))
    {
        AddRepresentation(MCSTR("application/rtf"), p_bytes);
        AddRepresentation(MCSTR("text/richtext"), p_bytes);
    }
    else if (MCStringIsEqualToCString(p_type, "UTF8_STRING", kMCStringOptionCompareExact))
    {
        AddRepresentation(MCSTR("UTF-8"), p_bytes);
        AddRepresentation(MCSTR("text/plain;charset=utf-8"), p_bytes);
        AddRepresentation(MCSTR("text/plain;charset=UTF-8"), p_bytes);
    }
    else if (MCStringIsEqualToCString(p_type, "STRING", kMCStringOptionCompareExact))
    {
        AddRepresentation(MCSTR("TEXT"), p_bytes);
        AddRepresentation(MCSTR("text/plain"), p_bytes);
    }
    else if (MCStringIsEqualToCString(p_type, "text/uri-list", kMCStringOptionCompareExact))
    {
        // Various GNOME and KDE file managers don't like copying file lists
        // without also specifying some special MIME types.
        MCAutoDataRef t_copy_list(UriListToCopyList(p_bytes));
        AddRepresentation(MCSTR("x-special/gnome-copied-files"), *t_copy_list);
        AddRepresentation(MCSTR("application/x-kde-cutselection"), *t_copy_list);
    }
    
    // All done
    return true;
}

bool MCLinuxRawClipboardItem::AddRepresentation(MCStringRef p_type, render_callback_t p_callback, void* p_context)
{
    return false;
}

MCDataRef MCLinuxRawClipboardItem::UriListToCopyList(MCDataRef p_in)
{
    // Convert the bytes to a stringref
    MCAutoStringRef t_string_in;
    if (!MCStringCreateWithBytes(MCDataGetBytePtr(p_in), MCDataGetLength(p_in), kMCStringEncodingNative, false, &t_string_in))
        return NULL;
    
    // Make mutable
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(*t_string_in, &t_string))
        return NULL;
    
    // Convert CRLF sequences to LF
    if (!MCStringFindAndReplace(*t_string, MCSTR("\r\n"), MCSTR("\n"), kMCStringOptionCompareExact))
        return NULL;
    
    // Replace the ending LF with a NUL
    if (!MCStringRemove(*t_string, MCRangeMake(MCStringGetLength(*t_string)-1, 1)))
        return NULL;
    if (!MCStringAppendChar(*t_string, '\0'))
        return NULL;
    
    // Prepend a "copy" instruction
    if (!MCStringPrepend(*t_string, MCSTR("copy\n")))
        return NULL;
    
    // Re-encode to data
    MCDataRef t_out;
    if (!MCStringEncode(*t_string, kMCStringEncodingNative, false, t_out))
        return NULL;
    return t_out;
}

#ifndef _SERVER
static bool SelectionNotifyFilter(GdkEvent *t_event, void *)
{
    return (t_event->type == GDK_SELECTION_NOTIFY);
}

// Timeout function that is triggered if we don't receive a reply from the
// current seelction owner. If this timeout triggers, the selection is regarded
// as being empty.
static bool s_selection_timeout;
static gboolean SelectionNotifyTimeout(gpointer)
{
    // If we didn't receive the selection notification by now, we have timed out
    s_selection_timeout = true;
    
    // Remove this timer
    return FALSE;
}

static bool WaitForSelectionNotify()
{
    // Add a timeout that will be triggered if there is no reply. We will wait
    // for a maximum of 1 second.
    guint t_timeout_event;
    s_selection_timeout = false;
    t_timeout_event = g_timeout_add(1000, &SelectionNotifyTimeout, NULL);
    
    // Loop until a selection notify event is received
    MCScreenDC *dc = (MCScreenDC*)MCscreen;
    GdkEvent *t_notify;
    while (!dc->GetFilteredEvent(SelectionNotifyFilter, t_notify, NULL, true))
    {
        // If we timed out, stop waiting
        if (s_selection_timeout)
            break;
    }
    
    // If the timeout didn't trigger, remove it manually
    if (!s_selection_timeout)
        g_source_destroy(g_main_context_find_source_by_id(NULL, t_timeout_event));
    
    return !s_selection_timeout;
}
#endif

void MCLinuxRawClipboardItem::FetchExternalRepresentations(GdkDragContext* p_drag_context) const
{
#ifndef _SERVER
    // Fail if GDK isn't available
    if (!MCLinuxRawClipboard::HasGDK())
        return;
    
    // This method is only necessary for externally-supplied data
    if (!m_data_is_external)
        return;
    
    // Do nothing if this has already been done
    if (m_reps.Size() > 0)
        return;
    
    // If we have a drag context, get the target list from it
    if (p_drag_context != NULL)
    {
        GList* t_targets = gdk_drag_context_list_targets(p_drag_context);
        while (t_targets != NULL)
        {
            // Extend the representation list
            uindex_t t_index = m_reps.Size();
            m_reps.Extend(t_index + 1);
            
            // Add the new representation
            m_reps[t_index] = new (nothrow) MCLinuxRawClipboardItemRep(m_clipboard, m_clipboard->GetSelectionAtom(), (GdkAtom)t_targets->data);
            
            // Next item on the target list
            t_targets = t_targets->next;
        }
    }
    // If we have no drag context, get the "TARGETS" property from the selection
    else
    {
        // Get the atom for the "TARGETS" property
        GdkAtom t_targets_atom = gdk_atom_intern_static_string("TARGETS");
        
        // Request the list of targets that the selection can supply
        gdk_selection_convert(m_clipboard->GetClipboardWindow(),
                              m_clipboard->GetSelectionAtom(),
                              t_targets_atom,
                              GDK_CURRENT_TIME);
        
        // Wait for a SelectionNotify event that tells us the data is ready
        bool t_timed_out = !WaitForSelectionNotify();
        
        // Get the data for this property
        GdkAtom t_type;
        gint t_format;
        guchar* t_data = NULL;
        gint t_data_length = 0;
        if (!t_timed_out)
        {
            t_data_length = gdk_selection_property_get(m_clipboard->GetClipboardWindow(), &t_data, &t_type, &t_format);
        }
        
        // Was the property received successfully?
        if (t_data == NULL || t_type != GDK_SELECTION_TYPE_ATOM)
            return;
        
        // The number of atoms that were read. GDK always allocates an extra byte
        // in the returned data, which we need to ignore.
        //
        // If the format isn't valid, act as if there are no representations.
        uindex_t t_atom_count = 0;
        if (t_format == 8)
            t_atom_count = (t_data_length - 1) / sizeof(guchar);
        else if (t_format == 16)
            t_atom_count = (t_data_length - 1) / sizeof(gushort);
        else if (t_format == 32)
            t_atom_count = (t_data_length - 1) / sizeof(gulong);
        
        // Allocate each of the representations
        for (uindex_t i = 0; i < t_atom_count; i++)
        {
            // Get the atom for this index. Note that these are stored using the
            // GDK types, which may not correspond to their true sizes.
            gulong t_atom;
            if (t_format == 8)
                t_atom = reinterpret_cast<guchar*>(t_data)[i];
            else if (t_format == 16)
                t_atom = reinterpret_cast<gushort*>(t_data)[i];
            else /* if (t_format == 32) */
                t_atom = reinterpret_cast<gulong*>(t_data)[i];
            
            // If the atom is invalid, skip this entry
            if (t_atom == 0)
                continue;
            
            // Extend the representation array
            uindex_t t_index = m_reps.Size();
            if (!m_reps.Extend(t_index + 1))
                break;
            
            // Add the representation
            m_reps[t_index] = new (nothrow) MCLinuxRawClipboardItemRep(m_clipboard, m_clipboard->GetSelectionAtom(), (GdkAtom)t_atom);
        }
        
        // Free the memory containing the atoms
        g_free(t_data);
    }
#endif
}


MCLinuxRawClipboardItemRep::MCLinuxRawClipboardItemRep(MCLinuxRawClipboard* p_clipboard, GdkAtom p_selection, GdkAtom p_atom) :
  MCRawClipboardItemRep(),
  m_type(NULL),
  m_bytes(NULL)
{
#ifndef _SERVER
    // Do nothing if we don't have GDK available
    if (!MCLinuxRawClipboard::HasGDK())
        return;
    
    // Fetch the type name for this representation
    m_type.Reset(MCLinuxRawClipboard::CopyTypeForAtom(p_atom));
    MCAssert(*m_type != NULL);
    
    // Request the data for this representation
    gdk_selection_convert(p_clipboard->GetClipboardWindow(),
                          p_selection, p_atom, GDK_CURRENT_TIME);
    
    // Wait for a SelectionNotify event that tells us the data is ready
    bool t_timed_out = !WaitForSelectionNotify();
    
    // Get the data for this property
    GdkAtom t_type;
    gint t_format;
    guchar* t_data = NULL;
    gint t_data_length = 0;
    if (!t_timed_out)
    {
        t_data_length = gdk_selection_property_get(p_clipboard->GetClipboardWindow(), &t_data, &t_type, &t_format);
    }
    
    // Copy the data for this property
    MCDataCreateWithBytes(t_data, t_data_length, &m_bytes);
    g_free(t_data);
#endif
}

MCLinuxRawClipboardItemRep::MCLinuxRawClipboardItemRep(MCStringRef p_type, MCDataRef p_bytes) :
  MCRawClipboardItemRep(),
  m_type(MCValueRetain(p_type)),
  m_bytes(MCValueRetain(p_bytes))
{
    ;
}

MCLinuxRawClipboardItemRep::~MCLinuxRawClipboardItemRep()
{
    ;
}


MCStringRef MCLinuxRawClipboardItemRep::CopyTypeString() const
{
    return MCValueRetain(*m_type);
}

MCDataRef MCLinuxRawClipboardItemRep::CopyData() const
{
    return MCValueRetain(*m_bytes);
}
