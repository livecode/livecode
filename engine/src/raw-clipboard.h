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


#ifndef RAW_CLIPBOARD_H
#define RAW_CLIPBOARD_H


#include "foundation.h"
#include "mixin-refcounted.h"


// Interface for accessing individual items on a multi-item clipboard
class MCRawClipboardItem;

// Interface to a particular representation of a clipboard item
class MCRawClipboardItemRep;


// Enumeration used to identify types that the engine can interpret itself
enum MCRawClipboardKnownType
{
    // Plain text types
    kMCRawClipboardKnownTypeUTF8 = 0,   // UTF-8 encoded text
    kMCRawClipboardKnownTypeUTF16,      // UTF-16 encoded text
    kMCRawClipboardKnownTypeISO8859_1,  // ISO-8859-1 encoded text
    kMCRawClipboardKnownTypeMacRoman,   // MacRoman encoded text
    kMCRawClipboardKnownTypeCP1252,     // Codepage-1252 encoded text
    
    // Formatted text types
    kMCRawClipboardKnownTypeRTF,        // Rich Text Format data
    kMCRawClipboardKnownTypeHTML,       // HTML data (see header for encoding)
    
    // Image types
    kMCRawClipboardKnownTypePNG,        // PNG image
    kMCRawClipboardKnownTypeGIF,        // GIF image
    kMCRawClipboardKnownTypeJPEG,       // JPEG image
	kMCRawClipboardKnownTypeTIFF,		// TIFF image
    kMCRawClipboardKnownTypeWinMF,      // Windows Metafile image
    kMCRawClipboardKnownTypeWinEMF,     // Windows Enhanced Metafile image
    kMCRawClipboardKnownTypeWinDIB,     // Windows BMP image
    kMCRawClipboardKnownTypeWinDIBv5,   // Windows BMPv5 image
    
    // LiveCode internal types
    kMCRawClipboardKnownTypeLiveCodeObjects,    // LiveCode object serialisation
    kMCRawClipboardKnownTypeLiveCodeStyledText, // LiveCode text serialisation
    
    // Files and URLs
    kMCRawClipboardKnownTypeFileURL,    // Single file URL
    kMCRawClipboardKnownTypeFileURLList,// List of NUL-separated file URLs
    kMCRawClipboardKnownTypeURL,        // Single generic URL
    kMCRawClipboardKnownTypeURLList,    // List of NUL-separated URLs
    
    kMCRawClipboardKnownTypeLast = kMCRawClipboardKnownTypeURLList
};


// class MCRawClipboard
//
//      Interface to the platform-specific code providing low-level access to
//      the system clipboard (or other clipboard-like objects).
//
// Note that it is not possible to modify the contents of a clipboard without
// clearing all contents first -- this is to avoid problems when the current
// clipboard contents were placed there by another program.
//
// The recommended sequence of operations for modifying the contents of the
// clipboard are:
//  1. Render the data into the formats being provided
//  2. Create the clipboard item(s) and add those representations
//  3. Clear the clipboard to gain ownership
//  4. Add the items to the clipboard
//  5. Synchronise the clipboard contents
//
// If the data representations are "small", it may be worth also doing:
//  6. Flush the data to the clipboard
//
// On shutdown of LiveCode, the clipboard data should also be flushed if it
// hasn't been done already and LiveCode still owns the clipboard. Otherwise,
// the data on the clipboard will silently disappear and users will be unhappy.
//
class MCRawClipboard :
  public MCMixinRefcounted<MCRawClipboard>
{
public:
    
    // Returns the number of items on the clipboard. This may be subject to
    // change at any time as the clipboard contents may get replaced.
    virtual uindex_t GetItemCount() const = 0;
    
    // Returns the item at the requested slot in the clipboard, or NULL if the
    // index is not valid. The returned item holds a reference to the clipboard
    // data and must be release()'d after use.
    virtual const MCRawClipboardItem* GetItemAtIndex(uindex_t p_index) const = 0;
    virtual MCRawClipboardItem* GetItemAtIndex(uindex_t p_index) = 0;
    
    // Deletes the current contents of the clipboard. This must be called before
    // any modifications to the clipboard or undefined behaviour will result.
    //
    // Clearing the clipboard will transfer ownership of the clipboard to this
    // instance of LiveCode.
    virtual void Clear() = 0;
    
    // Indicates whether this clipboard is currently owned by this LiveCode
    // instance. To gain ownership of the clipboard, call Clear().
    virtual bool IsOwned() const = 0;
    
    // Indicates whether the data on this clipboard came from LiveCode or an
    // external source. Externally-provided data cannot be modified; the
    // clipboard must be cleared and data re-added.
    virtual bool IsExternalData() const = 0;
    
    // Creates a new clipboard data item with no representations. Ownership of
    // the item is passed to the caller.
    virtual MCRawClipboardItem* CreateNewItem() = 0;
    
    // Pushes an item onto the clipboard. It will be inserted after all other
    // items on the clipboard (i.e at index getItemCount()). Ownership of the
    // item remains with the caller.
    //
    // Only items created by a call to createNewItem() on *this* clipboard may
    // be added to this clipboard -- any items from other sources will result
    // in undefined behaviour.
    //
    // Returns false if the insertion failed.
    virtual bool AddItem(MCRawClipboardItem* p_item) = 0;
    
    // Ensures that all updates to the clipboard have been synchronised with the
    // system -- note that this does not guarantee that the *data* has been
    // uploaded somewhere; just that the meta-data is up-to-date.
    virtual bool PushUpdates() = 0;
    
    // Fetches the latest changes from the system clipboard.
    //
    // Note that if the clipboard is not owned, this will discard the contents
    // of this clipboard object in favour of the newer updates from the system's
    // clipboard.
    virtual bool PullUpdates() = 0;
    
    // Uploads all the offered representations to the clipboard to ensure that
    // they will remain available after the termination of the LiveCode process.
    // This is not needed on some systems and not possible on others but, for
    // those that do implement it, not doing so is a huge user annoyance!
    virtual bool FlushData() = 0;
    
    // Returns the maximum number of items that can be added to the clipboard.
    // This will usually be either 1 or a very large integer (e.g. UINDEX_MAX),
    // depending on whether the operating system supports multi-item clipboards.
    virtual uindex_t GetMaximumItemCount() const = 0;
    
    // Checks for multi-item clipboard support
    inline bool SupportsMultipleItems() const
      { return GetMaximumItemCount() > 1; }
    
    // Returns the type string that this clipboard uses to identify particular
    // data representations. If the platform doesn't have a 'well-known' type
    // for the requested representation, these will return NULL.
    virtual MCStringRef GetKnownTypeString(MCRawClipboardKnownType p_type) const = 0;
    
    // For single-item clipboards, this method takes a newline-separated list
	// of file paths and encodes it into a DataRef containing the system-
	// specific data used to pass file path lists.
	//
	// For multi-item clipboards, encodes a single path.
	virtual MCDataRef EncodeFileListForTransfer(MCStringRef p_paths) const = 0;

	// Reverses CreateEncodedFileList.
	virtual MCStringRef DecodeTransferredFileList(MCDataRef p_data) const = 0;

	// Convert html to system-specific encoding for transfer on the clipboard
	virtual MCDataRef EncodeHTMLFragmentForTransfer(MCDataRef p_html) const = 0;
	virtual MCDataRef DecodeTransferredHTML(MCDataRef p_html) const = 0;

	// Converts a Windows DIB (BMP file) to and from the on-clipboard format
	virtual MCDataRef EncodeBMPForTransfer(MCDataRef p_bmp) const;
	virtual MCDataRef DecodeTransferredBMP(MCDataRef p_bmp) const;

    // Destructor
    virtual ~MCRawClipboard() = 0;
    
    
    // Creates a new clipboard associated with the main system clipboard. Note
    // that these are not kept synchronised - clipboards are only updated when
    // the "synchronizeUpdates" method is called.
    static MCRawClipboard* CreateSystemClipboard();
    static MCRawClipboard* CreateSystemSelectionClipboard();
    static MCRawClipboard* CreateSystemDragboard();
};


// class MCRawClipboardItem
//
//      Interface to the platform-specific code for managing data items stored
//      on the clipboard.
//
// Each data item on the clipboard can have multiple 'representations' - in
// other words, different formats for the data. For example, formatted text
// placed on the clipboard by a word processing application may be represented
// with the following types:
//   - a private format holding the application's internal data structures
//   - a generic rich text format, like RTF
//   - unformatted plain text
//   - an image of the formatted text, as displayed in the word processor
//
// Some systems provide clipboards that are ordered from the most authentic
// representation to the most generic. Others don't. This interface preserves
// that ordering where it exists but makes no attempt to add one where it
// doesn't.
//
// On systems where clipboard formats are represented by integer atoms, the atom
// is resolved to its corresponding string (e.g. "TEXT" rather than 42). If that
// resolution fails (which, for example,  may happen on Windows for
// un-registered private clipboard formats), a string containing the value
// of the atom formatted as a decimal integer is returned instead.
//
class MCRawClipboardItem :
  public MCMixinRefcounted<MCRawClipboardItem>
{
public:
    
    // Returns the number of representations that are available for this item.
    virtual uindex_t GetRepresentationCount() const = 0;
    
    // Fetches the representation at the requested index. No ownership of the
    // representation is transferred.
    virtual const MCRawClipboardItemRep* FetchRepresentationAtIndex(uindex_t p_index) const = 0;
    
    // Fetches the representation (if any) identified by the representation's
    // type string. No ownership of the representation is transferred.
    const MCRawClipboardItemRep* FetchRepresentationByType(MCStringRef p_type) const;
    
    // Adds a representation for this clipboard item. On systems that use atoms
    // to store clipboard format strings, an atom will be automatically
    // registered if it doesn't already exist. Ownership of the supplied type
    // string and data remains with the caller.
    //
    // If there is already a representation with the given type, it will be
    // replaced by the new data.
    virtual bool AddRepresentation(MCStringRef p_type, MCDataRef p_bytes) = 0;
    
    // Adds a representation for this clipboard item without supplying any data.
    // The provided callback will be invoked when the data is required (this
    // could be immediately, if the system does not support deferral!).
    //
    // When invoked, the callback should return a DataRef containing the item
    // rendered into the requested format or NULL if rendering failed.
    typedef MCDataRef (*render_callback_t)(void* p_context, MCRawClipboardItem* p_item, MCStringRef p_type);
    virtual bool AddRepresentation(MCStringRef p_type, render_callback_t, void* p_context) = 0;
    
    // Indicates whether the named type is available as a representation for
    // this item.
    bool HasRepresentation(MCStringRef p_type) const;
    
protected:
    
    // Destructor is protected as lifetime is managed by reference counting.
    friend class MCMixinRefcounted<MCRawClipboardItem>;
    virtual ~MCRawClipboardItem() = 0;
};


// class MCRawClipboardItemRep
//
//      Interface to the platform-specific code for managing representations of
//      data items stored on the clipboard.
//
// See the description of the MCRawClipboardItem class for information on
// the different representations of clipboard data items.
//
// The lifetime of instances of this class is tied to the lifetime of the parent
// MCRawClipboardItem class -- they are destroyed along with the parent. (It is
// done this way as the underlying platform APIs have similar semantics. Plus,
// you can't really do much with a representation when the underlying item
// supplying it is gone...).
//
class MCRawClipboardItemRep
{
public:
    
    // Returns a string that in some way identifies this representation. The
    // form that these strings take is platform-specific (so some platform-
    // specific knowledge is required to interpret them). This string should be
    // released when no longer required.
    virtual MCStringRef CopyTypeString() const = 0;
    
    // Fetches the data for this representation. This is potentially a very
    // slow operation! (There is the possibility on Windows, for example, that
    // the COM object supplying the data is on a remote computer and so the
    // transfer time is potentially unbounded). The data should be released when
    // no longer required.
    virtual MCDataRef CopyData() const = 0;
    
    
protected:
    
    // Destructor is protected as lifetime is tied to the parent
    // MCRawClipboardItem instance.
    virtual ~MCRawClipboardItemRep() = 0;
};


#endif  /* ifndef RAW_CLIPBOARD_H */
