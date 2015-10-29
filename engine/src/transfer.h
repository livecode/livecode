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

///////////////////////////////////////////////////////////////////////////////
//
//  Overview of LiveCode data-transfer features
//
//  The LiveCode script and objects interact with the data-transfer features
//  via derivations of the MCTransferData object: MCDragData, MCSelectionData
//  MCClipboardData.
//
//  These objects, in turn, communicate with the system via the appropriate
//  MCScreenDC calls. Communication at this level is done via the MCPasteboard
//  interface.
//
//  Note that the MC*Data objects automatically handle local caching and data
//  transfer, meaning that the system objects are only queried for remote
//  requests.
//
//  Usage of data-types is also layered. LiveCode scripts can request the
//  full range of data types, while an MCPasteboard object will never contain
//  types of class 'derived'. It is possible for an MCPasteboard object to
//  contain types of class 'private' and these data-types should be ignored
//  by the MCScreenDC calls and not published externally.
//
//  The layering of data-transfer features can be roughly represented as
//  follows:
//
//  ----User --------------------------------------------------------------
//    the clipboard                    TEXT/UNICODE_TEXT/STYLED_TEXT
//    the clipboardData                RTF_TEXT/HTML_TEXT
//    the dragData                     OBJECTS/IMAGE/FILES/PRIVATE
//    copy
//    cut
//    paste
//    <middle click in field>
//  ----Script-------------------------------------------------------------
//    MCSelectionData                  TEXT/UNICODE_TEXT/STYLED_TEXT
//    MCDragData                       RTF_TEXT/HTML_TEXT
//    MCClipboardData                  OBJECTS/IMAGE/FILES/PRIVATE
//  ----Screen-------------------------------------------------------------
//    dodragdrop                       TEXT/UNICODE_TEXT/STYLED_TEXT
//    setclipboard                     OBJECTS/IMAGE/FILES/PRIVATE
//    getclipboard
//    ownsclipboard
//    setselection
//    getselection
//    ownsselection
//  ----System-------------------------------------------------------------
//    X Selections (X11)               TEXT/UNICODE_TEXT/STYLED_TEXT
//    IDataObject (Windows)            OBJECTS/IMAGE/FILES
//    PasteboardRef (Mac OS X)
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __MC_TRANSFER__
#define __MC_TRANSFER__

class MCParagraph;
class MCField;

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

///////////////////////////////////////////////////////////////////////////////
//
//  Interface:
//    MCPasteboard
//
//  Type:
//    Platform independent interface
//
//  Description:
//    The MCPasteboard interface represents a read-only view of a collection of
//    types and their data.
//
//    When a method returns a pasteboard, the caller can assume that its
//    contents will remain constant for the lifetime of the object (i.e. until
//    it is fully released).
//
//    In practice this means that if the pasteboard is wrapping a system object
//    the list of formats available should be cached when the pasteboard is
//    created. Subsequently, if a Fetch is made, and the system object has
//    'Changed' so that the type requested cannot be fetched, 'false' should be
//    returned.
//
//    An alternative implementation strategy is to validate that the contents
//    have remained unchanged since creation of the object and return false from
//    methods if this is not the case.
//
class MCPasteboard
{
public:
	// Increase references to this object
	virtual void Retain(void) = 0;

	// Release a reference to this object, when the reference count reaches
	// zero the object should be destroyed and the underlying system
	// pasteboard 'unlocked'.
	virtual void Release(void) = 0;

	// Query the list of transfer types present on the pasteboard. Note that
	// only one type of each class should be present in this list, and for
	// this purpose FILES is considered the same class as TEXT.
	//
	// If there are no 'known' types on the clipboard, r_type_count should
	// be zero, and true should be returned.
	//
	// If the pasteboard could not be queried, false should be returned.
	//
	virtual bool Query(MCTransferType*& r_types, size_t& r_type_count) = 0;

	// Fetch the given transfer type from the pasteboard. It is an error
	// for this method to be called with a transfer type that was not
	// returned by 'Query'.
	//
	// If data could not be fetched for the given transfer type for any
	// reason, false should be returned.
	//
	// The data returned by this method is copied. The callee is responsible
	// for releasing the reference.
	//
	virtual bool Fetch(MCTransferType p_type, MCDataRef& r_data) = 0;

	// Future extension methods for accessing system formats directly.
	//
	// virtual bool QuerySystem(const char**& r_system_types, unsigned int& r_system_type_count) = 0;
	// virtual bool FetchSystem(const char *p_system_type, MCSharedString*& r_data) = 0;
};


///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    MCLocalPasteboard
//
//  Type:
//    Platform independent utility class
//
//  Description:
//    The MCLocalPasteboard class represents a client-side collection of trasfer
//    data. This is used by the higher level syntax classes to pass data into
//    the MCScreenDC methods dealing with data transfer.
//
//    The object implements the MCPasteboard interface faithfully and
//    additionally provides Store and Append methods to allow the data-types to
//    be added. These methods enforce the rules about the contents of a
//    pasteboard, automatically overwriting types of identical classes.
//
class MCLocalPasteboard: public MCPasteboard
{
public:
	// Construct a new empty local pasteboard
	MCLocalPasteboard(void);
	
	// Add a reference to the object
	void Retain(void);

	// Remove a reference from the object
	void Release(void);

	// Query the object for its list of types (see MCPasteboard for
	// precise semantics).
	bool Query(MCTransferType*& r_types, size_t& r_type_count);

	// Fetch the data of the given data type from the object (see
	// MCPasteboard for precise semantics).
	bool Fetch(MCTransferType p_type, MCDataRef& r_data);

	// Store the given data-type in the object. If a type of the given
	// class already exists, it is overridden.
    bool Store(MCTransferType p_type, MCValueRef p_data);

private:
	// Destroy the object.
	~MCLocalPasteboard(void);

	// Search for the given type on the pasteboard and return the index.
	// If the type is not found, false is returned.
	bool Find(MCTransferType p_type, uint4& r_index);

	// Convert the given data into a form that can be stored on a pasteboard.
	// At the moment this call just converts RTF/HTML to STYLED.
	//
	// Here r_normal_string has copy semantics, and p_string is unchanged.
	//
    bool Normalize(MCTransferType p_type, MCValueRef p_string, MCTransferType& r_normal_type, MCDataRef &r_normal_data);

	// The number of references to this object.
	uint4 m_references;

	// The number of datatypes present on the pasteboard.
	uint4 m_count;

	// The array of types.
	MCTransferType *m_types;

	// The array of data buffers.
	MCDataRef *m_datas;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Abstract Class:
//    MCTransferData
//
//  Type:
//    Platform independent abstract utility class
//
//  Description:
//    The MCTransferData class is a common implementation of drag data and
//    clipboard data services designed to be used by 'the clipboardData'
//    and 'the dragData' syntax.
//
//    It abstracts getting and setting pasteboard data which are implemented
//    by derived classes.
//
//    It has facilities for both setting the contents, and getting the contents
//    of the pasteboard it represents.
//
//    A sequence of read accesses to the pasteboard can be made atomic by
//    using the Lock and Unlock methods around a sequence of such calls.
//
//    A sequence of write accesses to the pasteboard can be made atomic by
//    using the Open and Close methods around a sequence of such calls.
//
//    Note that the only way to place multiple data type classes on the
//    pasteboard is to make multiple Store/Append calls within an Open/Close
//    pair.
//
class MCTransferData
{
public:
	MCTransferData(void);
	virtual ~MCTransferData(void);

	// Return true if there is a type of class 'Text'
	bool HasText(void);

	// Return true if there is a type of class 'Image'
	bool HasImage(void);

	// Return true if there is a type of class 'Files'
	bool HasFiles(void);

	// Return true if there is a type of class 'Private'
	bool HasPrivate(void);

	// Return true if there is a type of class 'Objects'
	bool HasObjects(void);


	// Lock the object (for read). Using this call ensures that
	// 'Contains' and 'Fetch' calls are consistent.
	//
	// If a lock on the pasteboard required could not be obtained,
	// the result is false.
	bool Lock(void);

	// Return a list of (non-derived) types present on the clipboard
	// note this call must be made inside an explicit Lock/Unlock pair
	// due to the lifetime of the returned arrays.
	bool Query(MCTransferType*& r_types, size_t& r_type_count);

	// Return true if fetching the given type would succeed. If
	// <p_with_conversion> is true, it also checks to see if <p_type>
	// can be derived from one of the types present.
	bool Contains(MCTransferType p_type, bool p_with_conversion = false);

	// Return data of the given type, converting as necessary.
	// The returned string has copy semantics, i.e. the caller must
	// release it when done with it.
    bool Fetch(MCTransferType p_type, MCValueRef &r_data);
	
	// Unlock the object.
	void Unlock(void);


	// Return the current text content of the clipboard as paragraphs suitable
	// for pasting into a field.
	MCParagraph *FetchParagraphs(MCField *p_field);


	// Open the object for update, all updates are queued until
	// a corresponding 'Close'. Opening the object does an implicit
	// clear.
	void Open(void);

	// Store the given data in the object. This call does an implicit
	// Open/Close if one hasn't previously been performed.
    bool Store(MCTransferType p_type, MCValueRef p_data);

	// Close the object applying any updates. If applying the
	// updates failed, false is returned.
	bool Close(void);

	// Convert the given string to a transfer type.
	static MCTransferType StringToType(MCStringRef p_string);

	// Conver the given type to a constant string.
	static const char *TypeToString(MCTransferType p_type);

protected:
	// This method is implemented by a derived class and should return
	// pasteboard object locking the appropriate selection.
	//
	// The returned pasteboard has copy semantics and must be released
	// by the caller.
	//
	// If this fails, NULL should be returned.
	//
	virtual MCPasteboard *Get(void) = 0;

	// This method is implemented by a derived class and should attempt
	// to change the contents of the appropriate selection.
	//
	// The passed pasteboard is copied.
	//
	// If this fails, false should be returned.
	//
	virtual bool Set(MCPasteboard *p_pasteboard) = 0;

private:
	// The number of times the object has been locked.
	uint4 m_lock_count;

	// The pasteboard object this wraps - this will only be non-NULL
	// if inside a lock.
	MCPasteboard *m_pasteboard;

	// The number of times the object has been opened.
	uint4 m_open_count;

	// The local pasteboard - this will only be non-NULL when if inside
	// an Open/Close pair.
	MCLocalPasteboard *m_open_pasteboard;

	// Returns true if there is a valid conversion from source type to
	// target type.
	bool HasTypeConversion(MCTransferType p_source, MCTransferType p_target);
};

// The MCSelectionData class is a support object for handling the transient
// selection data. It is constructed on demand and simply wraps the MCTransferData
// class by implemeted Get/Set methods for the 'selection' collection of
// MCScreenDC APIs.

///////////////////////////////////////////////////////////////////////////////
//
//  Interface:
//    MCSelectionData
//
//  Type:
//    Platform independent utility class
//
//  Description:
//    The MCSelectionData class is an implementation of MCTransferData wrapping
//    the system transient selection.
//
//    It is used by the field object to handle middle button copy operations.
//
class MCSelectionData: public MCTransferData
{
public:
	MCSelectionData(void);
	~MCSelectionData(void);

protected:
	MCPasteboard *Get(void);
	bool Set(MCPasteboard *p_pasteboard);

private:
	// A copy of the current local data, used to fulfil internal requests.
	MCPasteboard *m_local_pasteboard;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Interface:
//    MCDragData
//
//  Type:
//    Platform independent abstract utility class
//
//  Description:
//    The MCDragData class is an implementation of MCTransferData wrapping
//    both source and target drag-drop data.
//
//    It is used by 'the dragData' syntax to implement the language features
//    related to drag-drop data transfer.
//
//    The class uses one of three pasteboards depending on the current
//    context.
//
//    If the application is a drag target and script has not changed the
//    contents of the drag-data, the pasteboard passed to the last call of
//    MCDispatch::wmdragenter is used. This method updates the current
//    target pasteboard using SetTarget/ResetTarget.
//
//    If the application is a drag target and script has changed the contents
//    of the drag-data, a temporarly local pasteboard is used. This pasteboard
//    is destroyed on ResetTarget.
//
//    If the application is a drag source, a local pasteboard is used which
//    can be fetched by the GetSource method. The GetSource/ResetSource methods
//    are used in MCDispatch::wmdrag if a drag operation has been requested.
//
class MCDragData: public MCTransferData
{
public:
	MCDragData(void);
	~MCDragData(void);

	// Set the target pasteboard to the given object.
	void SetTarget(MCPasteboard *p_pasteboard);

	// Release the current target pasteboard.
	void ResetTarget(void);

	// Retrieve the current source pasteboard.
	//
	// The returned object has get semantics with lifetime until the
	// next method call.
	MCPasteboard *GetSource(void);

	// Clear the current source pasteboard.
	void ResetSource(void);

protected:
	// If a drag-target, returns m_target_pasteboard. Otherwise returns
	// m_source_pasteboard.
	MCPasteboard *Get(void);
	
	// If a drag-target, releases the current target pasteboard and
	// replaces it with the given object. Otherwise replaces the
	// current source pasteboard.
	bool Set(MCPasteboard *p_pasteboard);

private:
	// This stores the current target pasteboard that has been set.
	// This will either be a pasteboard passed to the object via SetTarget,
	// a temporary local pasteboard constructed when the contents were changed
	// while the application is a drag-target.
	MCPasteboard *m_target_pasteboard;

	// This stores the current source pasteboard. This will always be
	// a local pasteboard object, constructed when the data types are added
	// before a drag-drop operation commences.
	MCPasteboard *m_source_pasteboard;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Interface:
//    MCClipboardData
//
//  Type:
//    Platform independent utility class
//
//  Description:
//    The MCClipboardData class is an implementation of MCTransferData wrapping
//    the system clipboard.
//
class MCClipboardData: public MCTransferData
{
public:
	MCClipboardData(void);
	~MCClipboardData(void);

protected:
	MCPasteboard *Get(void);
	bool Set(MCPasteboard *p_pasteboard);

private:
	// A copy of the current local data, used to fulfil internal requests.
	MCPasteboard *m_local_pasteboard;
};

///////////////////////////////////////////////////////////////////////////////

// Drag-Drop related types
//
enum
{
	DRAG_ACTION_MOVE_BIT,
	DRAG_ACTION_COPY_BIT,
	DRAG_ACTION_LINK_BIT,
	
	DRAG_ACTION_NONE = 0,
	DRAG_ACTION_MOVE = 1 << DRAG_ACTION_MOVE_BIT,
	DRAG_ACTION_COPY = 1 << DRAG_ACTION_COPY_BIT,
	DRAG_ACTION_LINK = 1 << DRAG_ACTION_LINK_BIT,
};
typedef uint4 MCDragAction;
typedef uint4 MCDragActionSet;

typedef bool (*MCTransferConverter)(MCDataRef p_input, MCDataRef r_output);

bool MCConvertStyledTextToText(MCDataRef p_input, MCDataRef& r_output);
bool MCConvertStyledTextToUnicode(MCDataRef p_input, MCDataRef& r_output);
bool MCConvertStyledTextToRTF(MCDataRef p_input, MCDataRef& r_output);
bool MCConvertStyledTextToHTML(MCDataRef p_input, MCDataRef& r_output);
bool MCConvertTextToStyledText(MCDataRef p_input, MCDataRef& r_output);
bool MCConvertUnicodeToStyledText(MCDataRef p_input, MCDataRef& r_output);
bool MCConvertRTFToStyledText(MCDataRef p_input, MCDataRef& r_output);
bool MCConvertHTMLToStyledText(MCDataRef p_input, MCDataRef& r_output);

// MW-2014-03-12: [[ ClipboardStyledText ]] Converters to and from styledText arrays.
bool MCConvertStyledTextToStyledTextArray(MCDataRef p_string, MCArrayRef &r_array);
bool MCConvertStyledTextArrayToStyledText(MCArrayRef p_array, MCDataRef &r_output);

///////////////////////////////////////////////////////////////////////////////

enum MCImageFormat
{
	IMAGE_FORMAT_UNKNOWN,
	IMAGE_FORMAT_PNG,
	IMAGE_FORMAT_GIF,
	IMAGE_FORMAT_JFIF,
	IMAGE_FORMAT_DIB
};

struct MCImageData
{
	uint4 width;
	uint4 height;
	uint4 *pixels;
};

MCImageFormat MCImageDataIdentify(MCDataRef p_string);
void MCImageDataCompress(MCImageData *p_data, MCImageFormat p_as_format, MCDataRef &r_output);

MCImageFormat MCImageDataIdentify(const MCString& p_string);
MCImageFormat MCImageDataIdentify(IO_handle p_stream);

MCImageData *MCImageDataDecompress(const MCString& p_string);

MCImage *MCImageDataToObject(MCImageData *p_data);
MCImageData *MCImageDataFromObject(MCImage *p_object);

///////////////////////////////////////////////////////////////////////////////

bool MCFormatStyledTextIsUnicode(MCDataRef p_string);

#endif
