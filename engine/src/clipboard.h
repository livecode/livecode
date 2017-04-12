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


#ifndef CLIPBOARD_H
#define CLIPBOARD_H


#include "foundation.h"
#include "foundation-auto.h"
#include "mixin-refcounted.h"
#include "raw-clipboard.h"


class MCClipboard :
  public MCMixinRefcounted<MCClipboard>
{
public:
    
    // Is this clipboard currently owned by this LiveCode instance?
    bool IsOwned() const;
    
    // These methods lock the clipboard -- this prevents any implicit
    // synchronisation with the OS clipboard for the duration of the lock.
    // Explicit synchronisation (via Push/PullUpdates) is not affected by
    // locking.
    //
    // Unlocking can only fail if there are changes to be written out to the OS'
    // clipboard; it will always succeed when only reads were performed.
    //
    // For efficiency, a clipboard used as a drag board should be permanently
    // locked as this inhibits automatic updates. (The contents of the drag
    // board are only meaningful during a drag-and-drop operation).
    bool Lock(bool p_skip_pull = false) const;
    bool Unlock() const;
    
    // Indicates whether the clipboard is currently locked
    bool IsLocked() const;
    
    // Clears the contents of this clipboard.
    void Clear();
    
    // Discards any clipboard data cached within LiveCode unless LiveCode is
    // the source of the data (i.e it will only discard data that can be safely
    // re-fetched from the system).
    //
    // The primary use of this function is during drag-and-drop operations,
    // where we stop caring about the data if a drag from outside LiveCode
    // leaves a LiveCode window.
    void ReleaseData();
    
    // Pulls changes from the OS clipboard, overwriting any local changes. This
    // is implied when the lock count goes from zero to one.
    bool PullUpdates() const;
    
    // Pushes local changes to the OS clipboard, overwriting its contents. This
    // is implied when the lock count goes from one to zero and there have been
    // changes made locally.
    //
    // If forced, the clipboard will be pushed even in the absence of changes.
    bool PushUpdates(bool p_force = false) const;
    
    // Ensures all data in this clipboard has been pushed out to the OS'
    // clipboard. This means that the data will still be available after
    // LiveCode exits.
    void FlushData();
    
    // Returns the clipboard underlying this one.
    MCRawClipboard* GetRawClipboard();
    const MCRawClipboard* GetRawClipboard() const;
    
    // On some platforms, the drag board used for an incoming drag-and-drop
    // operation may be different from the main system drag board. This
    // method can be called to get this clipboard to re-bind to the supplied
    // low-level clipboard object.
    bool Rebind(MCRawClipboard* p_clipboard);
    
    // Indicates whether this clipboard contains any data
    bool IsEmpty() const;
    
    // These methods place data of the specified type on to the clipboard. As
    // a side-effect, the data will be placed in multiple forms if appropriate.
    // For example, on an OSX system, text will be added in 3 encodings:
    // UTF-16, UTF-8 and MacRoman.
    bool AddFileList(MCStringRef p_file_names);
    bool AddText(MCStringRef p_string);
    bool AddLiveCodeObjects(MCDataRef p_picked_objects);
    bool AddLiveCodeStyledText(MCDataRef p_picked_text);
    bool AddLiveCodeStyledTextArray(MCArrayRef p_styled_text);
    bool AddRTFText(MCDataRef p_rtf_data);
    bool AddHTMLText(MCStringRef p_html_string);
    bool AddRTF(MCDataRef p_rtf_data);
    bool AddHTML(MCStringRef p_html_string);
    bool AddPNG(MCDataRef p_png);
    bool AddGIF(MCDataRef p_gif);
    bool AddJPEG(MCDataRef p_jpeg);
	bool AddBMP(MCDataRef p_bmp);
	bool AddWinMetafile(MCDataRef p_wmf);
	bool AddWinEnhMetafile(MCDataRef p_emf);
    
    // Utility method for adding images - given the binary data for a PNG, GIF
    // or JPEG-encoded image, the image is added to the clipboard tagged with
    // the correct type.
    bool AddImage(MCDataRef p_image_data);
    
    // Indicates whether data of the specified form is available
    bool HasFileList() const;
    bool HasText() const;
    bool HasLiveCodeObjects() const;
    bool HasLiveCodeStyledText() const;
    bool HasHTML() const;
    bool HasRTF() const;
    bool HasPNG() const;
    bool HasGIF() const;
    bool HasJPEG() const;
	bool HasBMP() const;
	bool HasWinMetafile() const;
	bool HasWinEnhMetafile() const;
    bool HasImage() const;  // Any of PNG, GIF, JPEG or BMP
    
    // Utility methods that indicate whether the clipboard contains data of that
    // type or something that can be auto-converted to that type.
    bool HasTextOrCompatible() const;
    bool HasLiveCodeStyledTextOrCompatible() const;
    
    // Fetches the data from the clipboard, if it is available in the specified
    // format. If not available, NULL will be returned.
    bool CopyAsFileList(MCStringRef& r_file_list) const;
    bool CopyAsText(MCStringRef& r_text) const;
    bool CopyAsLiveCodeObjects(MCDataRef& r_objects) const;
    bool CopyAsLiveCodeStyledText(MCDataRef& r_pickled_text) const;
    bool CopyAsLiveCodeStyledTextArray(MCArrayRef& r_style_array) const;
    bool CopyAsRTFText(MCDataRef& r_rtf_data) const;        // Round-tripped via a field object
    bool CopyAsHTMLText(MCStringRef& r_html_string) const;  // Round-tripped via a field object
    bool CopyAsRTF(MCDataRef& r_rtf_data) const;
    bool CopyAsHTML(MCStringRef& r_html_string) const;
    bool CopyAsPNG(MCDataRef& r_png_data) const;
    bool CopyAsGIF(MCDataRef& r_gif_data) const;
    bool CopyAsJPEG(MCDataRef& r_jpeg_data) const;
	bool CopyAsBMP(MCDataRef& r_bmp_data) const;
	bool CopyAsWinMetafile(MCDataRef& r_wmf_data) const;
	bool CopyAsWinEnhMetafile(MCDataRef& r_emf_data) const;
    
    // Utility method for copying images - it tries to copy as any image format
    // that is supported by LiveCode (PNG, JPEG, GIF or BMP, in that order).
    bool CopyAsImage(MCDataRef& r_image_data) const;
    
    // Utility method for pasting into fields -- converts the clipboard contents
    // into MCParagraph structures, using the given field to supply the settings
    // for the conversion (properties, etc).
    class MCParagraph* CopyAsParagraphs(class MCField* p_via_field) const;
    
    // The clipboard can also contain private data - this is data that is local
    // to this instance of LiveCode and doesn't get placed on the system
    // clipboard.
    bool AddPrivateData(MCDataRef p_private);
    bool HasPrivateData() const;
    bool CopyAsPrivateData(MCDataRef& r_private) const;
	void ClearPrivateData();
    
    // Utility method for assisting with legacy clipboard support - returns a
    // positive value if an image is found before a text type, negative if a
    // text type is found first or zero if neither were found.
    int GetLegacyOrdering() const;
    
    // Creates wrappers for the specified clipboards. If the clipboard is not
    // implemented on the platform, a valid but non-functional clipboard will
    // be returned.
    static MCClipboard* CreateSystemClipboard();
    static MCClipboard* CreateSystemSelectionClipboard();
    static MCClipboard* CreateSystemDragboard();
    
	bool IsDragboard() const;
private:
    
    // The raw clipboard that is being wrapped
    MCAutoRefcounted<MCRawClipboard> m_clipboard;
    
    // Number of times the clipboard has been locked
    mutable uindex_t m_lock_count;
    
    // Private data (if any) added to this clipboard
    MCAutoDataRef m_private_data;
    
    // Set whenever a modification is made to the clipboard. No updates are
    // pushed if no modifications have been made.
    bool m_dirty;
    
    
    // Constructor and destructor
    friend class MCMixinRefcounted<MCClipboard>;
    MCClipboard(MCRawClipboard* t_underlying_clipboard);
    
    
    // Returns the first item on the clipboard, creating a new one if required
    MCRawClipboardItem* GetItem();
    const MCRawClipboardItem* GetItem() const;
    
    
    // Various utility functions for format conversion
    static MCStringRef ConvertStyledTextToText(MCDataRef p_pickled_text);
    static MCDataRef ConvertStyledTextToRTF(MCDataRef p_pickled_text);
    static MCStringRef ConvertStyledTextToHTML(MCDataRef p_pickled_text);
    static MCArrayRef ConvertStyledTextToStyledTextArray(MCDataRef p_pickled_text);
    static MCDataRef ConvertRTFToStyledText(MCDataRef p_rtf_data);
    static MCDataRef ConvertHTMLToStyledText(MCStringRef p_html_string);
    static MCDataRef ConvertStyledTextArrayToStyledText(MCArrayRef p_styles);
    static MCDataRef ConvertTextToStyledText(MCStringRef p_text);
    
    // Utility functions for extracting data from the clipboard
    bool CopyAsEncodedText(const MCRawClipboardItem* p_item, MCRawClipboardKnownType p_type, MCStringEncoding p_encoding, MCStringRef& r_text) const;
    bool CopyAsData(MCRawClipboardKnownType, MCDataRef& r_data) const;
    
    // Utility function for adding text representations
    bool AddTextToItem(MCRawClipboardItem* p_item, MCStringRef p_string);
	
	// Utility function that examines an HTML fragment to guess its encoding
	// Defaults to UTF-8 if it can't be automatically determined
	static MCStringEncoding GuessHTMLEncoding(MCDataRef p_html_data);
    
    // Private utility class for clipboard locking
    class AutoLock;
};


#endif  /* ifndef CLIPBOARD_H */
