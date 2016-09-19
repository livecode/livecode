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


#ifndef LINUX_CLIPBOARD_H
#define LINUX_CLIPBOARD_H


#include "raw-clipboard.h"
#include "foundation-auto.h"

#include <gdk/gdk.h>


// Forward declarations of classes
class MCLinuxRawClipboard;
class MCLinuxRawClipboardItem;
class MCLinuxRawClipboardItemRep;


class MCLinuxRawClipboardItemRep :
  public MCRawClipboardItemRep
{
public:
    
    // Inherited from MCRawClipboardItemRep
    virtual MCStringRef CopyTypeString() const;
    virtual MCDataRef CopyData() const;
    
private:
    
    // Cache of the type string and data for this representation
    MCAutoStringRef m_type;
    MCAutoDataRef m_bytes;
    
    // Controlled by the parent MCLinuxRawClipboardItem
    friend class MCLinuxRawClipboardItem;
    MCLinuxRawClipboardItemRep(MCLinuxRawClipboard* p_clipboard, GdkAtom p_selection, GdkAtom p_atom);
    MCLinuxRawClipboardItemRep(MCStringRef p_type, MCDataRef p_bytes);
    ~MCLinuxRawClipboardItemRep();
};


class MCLinuxRawClipboardItem :
  public MCRawClipboardItem
{
public:
    
    // Inherited from MCRawClipboardItem
    virtual uindex_t GetRepresentationCount() const;
    virtual const MCLinuxRawClipboardItemRep* FetchRepresentationAtIndex(uindex_t p_index) const;
    virtual bool AddRepresentation(MCStringRef p_type, MCDataRef p_bytes);
    virtual bool AddRepresentation(MCStringRef p_type, render_callback_t, void* p_context);
    
private:
    
    // Parent clipboard
    MCLinuxRawClipboard* m_clipboard;
    
    // Indicates whether the cached reps are from an external source
    bool m_data_is_external;
    
    // Array caching the representations of this item
    mutable MCAutoArray<MCLinuxRawClipboardItemRep*> m_reps;
    
    // Lifetime is managed by the parent MCLinuxRawClipboard
    friend class MCLinuxRawClipboard;
    friend class MCLinuxRawClipboardItemRep;
    MCLinuxRawClipboardItem(MCLinuxRawClipboard* p_parent);
    MCLinuxRawClipboardItem(MCLinuxRawClipboard* p_parent, GdkAtom p_selection, GdkDragContext *p_context);
    ~MCLinuxRawClipboardItem();
    
    // Ensures that the representations have been loaded if the data is from an
    // external source. Does nothing if the data is local.
    void FetchExternalRepresentations(GdkDragContext* p_context) const;
    
    // Utility function that converts from a text/uri-list encoding to the
    // file list types used internally by GNOME and KDE
    static MCDataRef UriListToCopyList(MCDataRef p_data);
};


class MCLinuxRawClipboard :
  public MCRawClipboard
{
public:
    
    // Inherited from MCRawClipboard
    virtual uindex_t GetItemCount() const;
    virtual const MCLinuxRawClipboardItem* GetItemAtIndex(uindex_t p_index) const;
    virtual MCLinuxRawClipboardItem* GetItemAtIndex(uindex_t p_index);
    virtual void Clear();
    virtual bool IsOwned() const;
    virtual bool IsExternalData() const;
    virtual MCLinuxRawClipboardItem* CreateNewItem();
    virtual bool AddItem(MCRawClipboardItem* p_item);
    virtual bool PushUpdates();
    virtual bool PullUpdates();
    virtual bool FlushData();
    virtual uindex_t GetMaximumItemCount() const;
    virtual MCStringRef GetKnownTypeString(MCRawClipboardKnownType p_type) const;
    virtual MCDataRef EncodeFileListForTransfer(MCStringRef p_file_list) const;
    virtual MCStringRef DecodeTransferredFileList(MCDataRef p_data) const;
	virtual MCDataRef EncodeHTMLFragmentForTransfer(MCDataRef p_html) const;
	virtual MCDataRef DecodeTransferredHTML(MCDataRef p_html) const;
    
    // Gets the item that was most recently pushed to the system (this is used
    // to respond to selection requests in order to perform copies)
    const MCLinuxRawClipboardItem* GetSelectionItem() const;
    
    // Sets the clipboard as being dirty
    void SetDirty();
    
    // Call to indicate that this clipboard no longer owns its selection. This
    // allows it to clean up the local data backing that selection.
    void LostSelection();
    
    // Returns the selection atom for this clipboard
    GdkAtom GetSelectionAtom() const;
    
    // Returns a DataRef containing the contents of the "TARGETS" representation
    // for this clipboard (this is the representation that lists the other
    // supported representations)
    MCDataRef CopyTargets() const;
    
    // Returns the window being used for clipboard operations
    GdkWindow* GetClipboardWindow() const;
    
    // Sets a custom window to use as the owner of this clipboard. This is used
    // for drag-and-drop operations as some targets get upset if the selection
    // owner is not the same as the drag window.
    void SetClipboardWindow(GdkWindow* p_window);
    
    // Sets a drag context to be used as an additional source of dragboard data
    // (the X11 drag protocol allows for the targets to be specified in the XDnD
    // message rather than using the TARGETS selection type).
    void SetDragContext(GdkDragContext* p_context);
    
    
    // Returns the atom for the given type string. If it hasn't been registered
    // yet, this function will do so and then return it.
    static GdkAtom CopyAtomForType(MCStringRef p_type);
    
    // Returns the string associated with the given atom
    static MCStringRef CopyTypeForAtom(GdkAtom p_atom);
    
    // Returns the GdkDisplay in use by the engine
    static GdkDisplay* GetDisplay();
    
    // Checks whether GDK is available or not
    static bool HasGDK();
    
    
    // Constructors
    MCLinuxRawClipboard(const char* p_selection_atom_name);
    MCLinuxRawClipboard(GdkAtom p_selection);
    
    
private:
    
    // The atom identifying the selection used by this clipboard
    GdkAtom m_selection;
    
    // The one (and only) item on the clipboard
    MCLinuxRawClipboardItem* m_item;
    
    // A copy of the data item used to back the current selection
    MCLinuxRawClipboardItem* m_selected_item;
    
    // Custom window, if any, to be used for this selection
    GdkWindow* m_window;
    
    // Drag context, if any, associated with this selection
    GdkDragContext* m_drag_context;
    
    // Information about the data on the clipboard
    bool m_dirty;               // Data has been modified
    bool m_external_data;       // Data is from outside LiveCode
    bool m_owned;               // Clipboard is owned by LiveCode
    
    // Destructor
    ~MCLinuxRawClipboard();
    
    
    // Table for mapping the known-type constants
    static const char* const s_formats[];
    
    // Invisible window used for clipboard accesses
    static GdkWindow* s_clipboard_window;
};


#endif  /* ifndef LINUX_CLIPBOARD_H */
