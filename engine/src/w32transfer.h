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

#ifndef __W32TRANSFER__
#define __W32TRANSFER__

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    FormatEnumerator
//
//  Type:
//    Private platform-specific utility class
//
//  Description:
//    The FormatEnumerator class provides a simple wrapper around a COM
//    IDataObject interface. Its purpose is to provide easy access to the
//    format enumerator. The class handles automatic creation and destruction
//    of its resources.
//
//    Note that the contents (accessed via *) of the enumerator is only valid
//    after a Next method call has returned true. The * operator is unchecked
//    in this regard.
//
//  Sample Usage:
//    FormatEnumerator t_enum(t_data_object);
//    while(t_enum . Next())
//    {
//        <do something with (*t_enum)>
//    }
//
//  Known Issues:
//    <none>
//
class FormatEnumerator
{
public:
	// Bind the enumerator object to the format enumerator for <p_object>
	FormatEnumerator(IDataObject *p_object);

	// Destroy the enumerator object and any allocated resources
	~FormatEnumerator(void);
	
	// Returns true if the object contains a valid enumerator
	bool Valid(void) const;

	// Reset the enumerator to the beginning. This method has no effect if
	// Valid() returns false.
	void Reset(void);

	// Advance the enumerator to the next element, returning true if there
	// is an element to look at.
	bool Next(void);

	// Return the current element in the enumeration. This method call is
	// only valid if there has been a Next call and the last one returned
	// true.
	FORMATETC& operator * (void);

private:
	// Private member containing the pointer to the COM enumerator interface
	// for the original data object.
	IEnumFORMATETC *m_enumerator;

	// Private member containing the current format being inspected by the
	// enumerator. The valud of this member is undefined if no Next call
	// has been performed, or the last call to Next returned false.
	FORMATETC m_format;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    FormatData
//
//  Type:
//    Private platform-specific utility class
//
//  Description:
//    The FormatData class provides a simple wrapper around data present in a
//    specific format within in a data object. Its purpose is to provide easy
//    access to the data, in a resource-managed way.
//
//    After construction, an application can use the 'Valid' method to
//    determine if the data format was found and if it was, the Get method
//    to access its contents.
//
//  Sample Usage:
//    FormatData t_data(t_data_object, t_data_format);
//    if (t_data . Valid())
//    {
//        MCString t_string;
//        t_string = t_data . Get();
//        < do something with t_string >
//    }
//
//  Known Issues:
//    The FormatData object only allows access to formats stored as HGLOBALs
//
class FormatData
{
public:
	// Create an unbound FormatData object
	FormatData(void);

	// Bind the object to the given format of the given data object.
	FormatData(IDataObject *p_object, FORMATETC& p_format);

	// Bind the object to the given clipboard format of the given data object.
	FormatData(IDataObject *p_object, CLIPFORMAT p_format);

	// Release resources used by the object and delete it.
	~FormatData(void);

	// Rebind the current object to the given format of the given data object.
	bool Bind(IDataObject *p_object, FORMATETC& p_format);

	// Rebind the current object to the given format of the given data object.
	bool Bind(IDataObject *p_object, CLIPFORMAT p_format);

	// Returns true if the object has a valid data pointer
	bool Valid(void) const;

	// Returns the data to which the object is bound. If the object is not
	// valid (i.e. Valid() is false), or the data it is bound to is not of
	// HGLOBAL type, the empty string is returned.
	MCString Get(void) const;

	// Return the HGLOBAL handle to which the object is bound, or NULL if
	// it is not bound to such a handle.
	HGLOBAL GetHandle(void) const;

private:
	// Private member indicating whether the m_storage member has been
	// initialized.
	bool m_valid;

	// Private member containing the OLE storage information.
	STGMEDIUM m_storage;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    TransferDataEnum
//
//  Type:
//    Private platform-specific utility class
//
//  Description:
//    The TransferDataEnum class is a general implementation of the OLE
//    FORMATETC enumerator interface (IEnumFORMATETC).
//
//    An instance is created and formats added to it via the 'Add' method.
//
//    The class is used by the TransferData class when a request is made
//    to enumerate it.
//
//    The class is not intended for general use and should only be constructed
//    by the TransferData class.
//
//    Note that the initial reference count after construction is zero. An
//    AddRef call must be made before passing it a reference on.
//
//    After an instance of the object has been passed on, the 'Add' method
//    should no longer be called. Doing so would violate the semantics of
//    the IEnumFORMATETC interface.
//
//  Sample Usage:
//    TransferDataEnum *t_enum;
//    t_enum = new TransferDataEnum;
//    if (t_enum != NULL)
//        t_enum -> AddRef();
//    for(unsigned int i = 0; i < t_format_count && t_enum != NULL; ++i)
//        if (!t_enum -> Add(&t_formats[i]))
//        {
//            t_enum -> Release();
//            t_enum = NULL;
//        }
//     <pass instance of t_enum onto user>
//
//  Known Issues:
//     <none>
class TransferDataEnum: public IEnumFORMATETC
{
public:
	// Construct an empty format enumerator
	TransferDataEnum(void);

	// Destroy the format enumerator and release any resources it holds
	~TransferDataEnum(void);

	// Add the given format to the list of formats that should be enumerated
	// to a client of this object.
	bool Add(FORMATETC *p_format);

	// Query the given OLE interface for this object. The new interface (with
	// reference count one) is returned in <r_object>.
	//
	// This implementation responds to IUnknown and IEnumFORMATETC.
	//
	// Returns S_OK if successful.
	STDMETHOD(QueryInterface)(REFIID p_id, void** r_object);

	// Add a reference to this object, returning the new reference count.
	STDMETHOD_(ULONG, AddRef)(void);

	// Remove a reference from this object, returning the new reference count.
	// If the reference count reaches zero, the object is deleted.
	STDMETHOD_(ULONG, Release)(void);

	// Attempt to fetch the next <p_count> elements, placing them into the
	// array pointed to by <r_elements>. The number of elements actually fetched
	// is returned in <r_fetched>.
	//
	// Note that <r_fetched> must be non-NULL if p_count > 1.
	//
	// If no elements were fetched and the parameters were valid, S_FALSE is
	// returned.
	//
	// If at least one element was fetched, S_OK is returned.
	STDMETHODIMP Next(ULONG p_count, LPFORMATETC r_elements, ULONG* r_fetched);

    // Skip <p_count> elements. If there are not enough elements left to skip
	// the required number, S_FALSE is returned.
	//
	// Returns S_OK if successful.
	STDMETHODIMP Skip(ULONG p_count);

	// Reset the enumerator to point to the first element.
	//
	// Always returns S_OK.
	STDMETHODIMP Reset(void);

	// Return a clone of this object in <r_clone>, identical in every respect
	// including current index.
	//
	// Returns S_OK if cloning succeeded, otherwise E_OUTOFMEMORY is returned
	// (running out of memory is the only reason cloning might fail).
	STDMETHODIMP Clone(IEnumFORMATETC** r_clone);

private:
	// The number of references to the object.
	ULONG m_references;

	// The list of formats to be enumerated. This pointer is allocated using
	// 'realloc'.
	FORMATETC *m_formats;

	// The number of formats <m_formats> points to.
	unsigned int m_format_count;

	// The current enumeration index.
	unsigned int m_index;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Callback:
//    MCWindowsConversionCallback
//
//  Type:
//    Private platform-specific callback type
//
//  Description:
//    The MCWindowsConversionCallback function type represents a procedure that
//    converts a MCSharedString object into a STGMEDIUM suitable for publishing
//    in an OLE IDataObject.
//
//    The callback should attempt to convert the binary data pointed to by
//    p_input into STGMEDIUM structure.
// 
//    On failure, false should be returned. In this case, STGMEDIUM will
//    assumed to be unchanged.
//
//    Note that the pUnkForRelease member of the STGMEDIUM structure must be
//    empty on exit.
//
typedef bool (*MCWindowsConversionCallback)(MCSharedString *p_input, STGMEDIUM& r_storage);

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    TransferData
//
//  Type:
//    Private platform-specific utility class
//
//  Description:
//    The TransferData class is an implementation of the OLE IDataObject
//    implementation specific to Revolution.
//
//    The class allows the publishing of a number of Revolution formats that
//    are presented in formats appropriate to the platform. Conversion is done
//    on demand by a client of the object.
//
//    After conversion to a given format has been requested, the result is
//    cached. This means data is not reconverted continually if fetched more
//    than once.
//
//    The object supports the GetData and SetData calls for client-specific
//    formats allowing it to be used in drag-drop operations requiring image
//    support (the IDropSourceHelper class passes bitmap data using private
//    formats set by SetData).
//
//  Known Issues:
//    <none>
//
class TransferData: public IDataObject
{
public:
	// Construct an empty data object.
	TransferData(void);

	// Destroy the object, releasing any resources it references.
	~TransferData(void);

	// Publish the given format in this object
	// 
	// Requests for the data are satisfied by processing <p_data> using
	// <p_converter>. If <p_converter> is NULL, <p_data> is used directly.
	bool Publish(FORMATETC *p_format, MCSharedString *p_data, MCWindowsConversionCallback p_converter);

	// Publish the given clipboard format in this object
	//
	// This is a simple wrapper around Publish(FORMATETC* ...) which just
	// constructs a standard FORMATETC structure.
	bool Publish(CLIPFORMAT p_format, TYMED p_storage, MCSharedString *p_data, MCWindowsConversionCallback p_converter);

	// Publish the given Revolution transfer type in this object
	//
	// This call will publish appropriate external formats for the given
	// internal format
	bool Publish(MCTransferType p_type, MCSharedString *p_data);

	// Publish the contents of the Revolution pasteboard in this object
	//
	// Note that this will skip the PRIVATE data type.
	bool Publish(MCPasteboard *p_pasteboard);

	// Query the given OLE interface for this object. The new interface (with
	// reference count one) is returned in <r_object>.
	//
	// This implementation responds to IUnknown and IDataObject.
	//
	// Returns S_OK if successful.
	STDMETHOD(QueryInterface)(REFIID p_id, void** r_object);

	// Add a reference to this object, returning the new reference count.
	STDMETHOD_(ULONG, AddRef)(void);

	// Remove a reference from this object, returning the new reference count.
	// If the reference count reaches zero, the object is deleted.
	STDMETHOD_(ULONG, Release)(void);

	// Search for the given format in the object, and if found return a
	// reference to its OLE storage.
	//
	// If the format can not be found, the error will be an DV_E_* error code
	// indicating which field it could not match on.
	//
	// If there was not enough memory to fulfil the request E_OUTOFMEMORY
	// is returned. This can occur if the internal storage handle needed
	// copying, or if implict conversion was required.
	//
	// If the operation was successful and p_medium now points to a valid
	// storage structure S_OK is returned.
	STDMETHOD(GetData)(LPFORMATETC p_format, LPSTGMEDIUM p_medium);

	// This call is not supported and always returnes E_NOTIMPL.
	STDMETHOD(GetDataHere)(LPFORMATETC p_format, LPSTGMEDIUM p_medium);

	// Query the object to determine if the given format is supported.
	//
	// If the given format is not found, the return value will be an DV_E_*
	// code indicating which field it could not match on.
	//
	// If this call returns S_OK, then a subsequent call to GetData with
	// the same format will always either return S_OK or E_OUTOFMEMORY.
	STDMETHOD(QueryGetData)(LPFORMATETC p_format);

	// Canonicalize the input format to a simpler, but equivalent format.
	//
	// This call strips the 'ptd' field of the FORMATETC structure since the
	// implementation does not support device-dependent formats.
	STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC p_format_in, LPFORMATETC p_format_out);

	// Add the given format and data to the list of formats published by
	// this object.
	//
	// To pass ownership of the storage medium to the object call the method
	// with <p_release> true.
	//
	// If the given format is already published by the object, this call will
	// overwrite the data already present.
	//
	// If not enough memory was available to fulfil the request E_OUTOFMEMORY
	// is returned. Otherwise, S_OK will be returned.
	STDMETHOD(SetData)(LPFORMATETC p_format, STGMEDIUM *p_medium, BOOL p_release);

	// Create an enumerator listing the formats published by this object.
	//
	// If <p_direction> is DATADIR_GET, all formats are listed.
	// If <p_direction> is DATADIR_SET, no formats are listed.
	//
	// If there is not enough memory to fulfil the request, E_OUTOFMEMORY is
	// returned. Otherwise S_OK is returned.
	STDMETHOD(EnumFormatEtc)(DWORD p_direction, LPENUMFORMATETC* r_enum);

	// Not implemented, always returns E_NOTIMPL.
	STDMETHOD(DAdvise)(FORMATETC* p_format, DWORD p_flags, LPADVISESINK p_sink, DWORD *p_connection);

	// Not implemented, always returns E_NOTIMPL.
	STDMETHOD(DUnadvise)(DWORD p_connection);

	// Not implemented, always returns E_NOTIMPL.
	STDMETHOD(EnumDAdvise)(LPENUMSTATDATA *r_enum);

private:
	// Structure representing a published format.
	//
	// In this structure, only one of <data> or <storage> will be valid at
	// once.
	//
	// If <data> is not NULL, the <storage> member will be filled in upon
	// request by passing <data> through <converter>. After this occurs,
	// <data> is released and <converter> set to NULL.
	struct Entry
	{
		FORMATETC format;
		STGMEDIUM storage;
		MCSharedString *data;
		MCWindowsConversionCallback converter;
	};

	// The number of references to this object
	ULONG m_references;

	// The list of formats published by the object
	Entry *m_entries;

	// The number of entries pointed to by <m_entries>
	unsigned int m_entry_count;

	// Search for the given format in <m_entries>. If found, S_OK will be
	// returned, and <r_entry> will contain a pointer to the Entry structure
	HRESULT Find(LPFORMATETC p_format, Entry*& r_entry);

	// Ensure the <storage> member of the given Entry is valid, converting
	// the data if necessary.
	// This call returns S_OK if successful, otherwise it means there was
	// not enough memory and E_OUTOFMEMORY is returned.
	HRESULT Subscribe(Entry *p_entry);

	// Standard routine for creating a clone of a given storage medium.
	// The <p_copy> parameter indicates whether the data should be copied
	// rather than just referenced.
	// This call returns S_OK if succesful, E_OUTOFMEMORY if a copy was
	// required and memory ran out, or DV_E_TYMED if a copy was required
	// and <p_in> is not a TYMED_HGLOBAL storage medium.
	HRESULT AddRefStgMedium(STGMEDIUM* p_in, STGMEDIUM* p_out, BOOL p_copy);
};

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    MCWindowsPasteboard
//
//  Type:
//    Private platform-specific utility class
//
//  Description:
//    The MCWindowsPasteboard class is an implementation of the MCPasteboard
//    abstract interface.
//
//    This class wraps an OLE IDataObject and performs appropriate format
//    conversions specific to the Revolution transfer type.
//
//  Known Issues:
//    <none>
//
class MCWindowsPasteboard: public MCPasteboard
{
public:
	// Create by wrapping the given data object
	MCWindowsPasteboard(IDataObject *p_object);

	// Destroy the object
	~MCWindowsPasteboard(void);
	
	// Increase references to this object
	void Retain(void);

	// Decrease references to this object, if the reference count reacches
	// zero, the object is released.
	void Release(void);

	// Return a list of the data-types published by this object. This method
	// returns false if the request could not be fulfilled (i.e. the types
	// could not be computed).
	//
	// If there are no known data types published by the object, true will
	// be returned, but r_type_count will be zero.
	//
	bool Query(MCTransferType*& r_types, unsigned int& r_type_count);

	// Fetch the given type that is published. This method should only be
	// called for types returned by query.
	//
	// If the fetch fails for any reason, false is returned.
	//
	bool Fetch(MCTransferType p_type, MCSharedString*& r_data);

private:
	// An Entry represents a given type that is published by the underlying
	// data object. It contains the Revolution transfer type, clipboard format
	// from which it can be derived and a cache of the data (filled on
	// demand by 'Fetch').
	//
	struct Entry
	{
		MCTransferType type;
		CLIPFORMAT format;
		MCSharedString *data;
	};

	// The number of references to this object
	unsigned int m_references;

	// The underlying OLE data object that we are wrapping
	IDataObject *m_data_object;

	// The number of entries computed for the data object.
	unsigned int m_entry_count;

	// The list of entries computed from the data object.
	Entry *m_entries;

	// A temporary type-list returned by Query
	MCTransferType *m_types;

	// Indicates whether this object is valid. An object is invalid if
	// a list of types was not computable when it was created. If this
	// is not true, the Query and Fetch methods return false immediately.
	bool m_valid;

	// Works out what types are published by this object by inspecting the types
	// enumerated by the data object. This information is stored in the m_types
	// array. If this operation succeeds, m_valid is then true.
	void Resolve(void);

	// Add an entry to the entries table. This is a utility used by Resolve.
	// It returns false if memory couldn't be allocated to perform the operation.
	bool AddEntry(MCTransferType p_type, CLIPFORMAT p_format);
};

///////////////////////////////////////////////////////////////////////////////
//
//  Function:
//    MCConvertTextToWindowsAnsi
//
//  Description:
//    Convert the given input string in the internal Revolution text format to
//    Windows CF_TEXT format.
//
//    This function simply does line-ending conversion mapping LF to CRLF
//    sequences.
//
//    Note when publishing text in this manner, a CF_LOCALE format should
//    accompany it with the setting Windows-1252.
//
bool MCConvertTextToWindowsAnsi(MCSharedString *p_input, STGMEDIUM& r_storage);

///////////////////////////////////////////////////////////////////////////////
//
//  Function:
//    MCConvertTextToWindowsAnsi
//
//  Description:
//    Convert the given input string in the internal Revolution text format to
//    Windows CF_UNICODETEXT format.
//
//    This function first converts line endings from LF to CRLF, then converts
//    the text to Unicode.
//
bool MCConvertTextToWindowsWide(MCSharedString *p_input, STGMEDIUM& r_storage);

///////////////////////////////////////////////////////////////////////////////
//
//  Function:
//    MCConvertUnicodeToWindowsWide
//
//  Description:
//    Convert the given input string in the internal Revolution text format to
//    Windows CF_UNICODETEXT format.
//
//    This call first converts the text to UTF-8, does an LF -> CRLF mapping
//    then converts to UTF-16 in host byte order.
//
bool MCConvertUnicodeToWindowsWide(MCSharedString *p_input, STGMEDIUM& r_storage);

///////////////////////////////////////////////////////////////////////////////
//
//  Function:
//    MCConvertUnicodeToWindowsWide
//
//  Description:
//    Convert the given input string in the internal Revolution text format to
//    Windows CF_UNICODETEXT format.
//
//    This call first converts the text to UTF-8 and does an LF -> CRLF
//    mapping, it then converts the text to the current code page.
//
bool MCConvertUnicodeToWindowsAnsi(MCSharedString *p_input, STGMEDIUM& r_storage);

///////////////////////////////////////////////////////////////////////////////
//
//  Function:
//    MCConvertStyledTextToWindowsWide
//
//  Description:
//    Convert the given input string in the internal Revolution styled text
//    format to wide text.
//
//    This call chains a conversion from styled text to unicode, then unicode
//    to windows wide.
//
bool MCConvertStyledTextToWindowsWide(MCSharedString *p_input, STGMEDIUM& r_storage);

///////////////////////////////////////////////////////////////////////////////
//
//  Function:
//    MCConvertStyledTextToWindowsAnsi
//
//  Description:
//    Convert the given input string in the internal Revolution styled text
//    format to ansi text.
//
//    This call chains a conversion from styled text to text, then text to
//    windows ansi.
//
bool MCConvertStyledTextToWindowsAnsi(MCSharedString *p_input, STGMEDIUM& r_storage);

///////////////////////////////////////////////////////////////////////////////
//
//  Function:
//    MCConvertStyledTextToWindowsRTF
//
//  Description:
//    Convert the given input string in the internal Revolution styled text
//    format to RTF suitable to be published on the Windows clipboard.
//
bool MCConvertStyledTextToWindowsRTF(MCSharedString *p_input, STGMEDIUM& r_storage);

///////////////////////////////////////////////////////////////////////////////
//
//  Function:
//    MCConvertImageToWindowsBitmap
//
//  Description:
//    Convert the given input string in a standard image format (GIF, PNG or
//    JPEG) to Windows CF_DIB format.
//
//    This call decompresses the image and creates a Windows DIB structure
//    representing it. The image data will be composited with a background
//    of black.
//
bool MCConvertImageToWindowsBitmap(MCSharedString *p_input, STGMEDIUM& r_storage);

///////////////////////////////////////////////////////////////////////////////
//
//  Function:
//    MCConvertImageToWindowsV5Bitmap
//
//  Description:
//    Convert the given input string in a standard image format (GIF, PNG or
//    JPEG) to Windows CF_DIBV5 format.
//
//    This call decompresses the image and creates a Windows DIBV5 structure
//    representing it.
//
bool MCConvertImageToWindowsV5Bitmap(MCSharedString *p_input, STGMEDIUM& r_storage);

///////////////////////////////////////////////////////////////////////////////
//
//  Function:
//    MCConvertImageToWindowsMetafile
//
//  Description:
//    Convert the given input string in a standard image format (GIF, PNG or
//    JPEG) to a Windows CF_ENHMETAFILE format.
//
//    This call decompresses the image and creates an Enhanced Metafile
//    object. The image data is rendered into the metafile using the AlphaBlend
//    call.
//
bool MCConvertImageToWindowsMetafile(MCSharedString *p_input, STGMEDIUM& r_storage);
bool MCConvertImageToWindowsEnhancedMetafile(MCSharedString *p_input, STGMEDIUM &r_storage);

///////////////////////////////////////////////////////////////////////////////
//
//  Function:
//    MCConvertFilesToWindowsHDROP
//
//  Description:
//    Convert the given list of files to a windows HDROP object.
//
bool MCConvertFilesToWindowsHDROP(MCSharedString *p_input, STGMEDIUM& r_storage);

#endif
