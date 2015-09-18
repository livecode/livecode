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

#include "w32prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "transfer.h"
//#include "execpt.h"
#include "exec.h"
#include "image.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "util.h"
#include "osspec.h"

#include "mcio.h"

#include "w32dc.h"
#include "w32transfer.h"

///////////////////////////////////////////////////////////////////////////////

// These are the Windows standard extended clipboard formats LiveCode uses.
#define CFSTR_RTF "Rich Text Format"
#define CFSTR_PNG "PNG"
#define CFSTR_GIF "GIF"
#define CFSTR_JFIF "JFIF"

// These are the private clipboard formats LiveCode uses to publish data to
// other LiveCode instances.

// The LiveCode Objects format is a sequence of object pickles.
#define CFSTR_REVOLUTION_OBJECTS "com.runrev.revolution.objects/1"

// The LiveCode Styled Text format a styled text pickle.
#define CFSTR_REVOLUTION_STYLED_TEXT "com.runrev.revolution.text/styled/1"

// This macro defines a 'once' function for registering the clipboard formats
// we can use.
//
// CLEANUP: Move clipboard format registration to a single place on startup.
//
#define WINDOWS_REGISTER_CLIPBOARD_FORMAT(m_name, m_format) \
	CLIPFORMAT m_name(void) \
	{ \
		static CLIPFORMAT s_id = 0; \
		if (s_id == 0) s_id = RegisterClipboardFormatA(m_format); \
		return s_id; \
	}

WINDOWS_REGISTER_CLIPBOARD_FORMAT(CF_RTF, CFSTR_RTF)
WINDOWS_REGISTER_CLIPBOARD_FORMAT(CF_PNG, CFSTR_PNG)
WINDOWS_REGISTER_CLIPBOARD_FORMAT(CF_JFIF, CFSTR_JFIF)
WINDOWS_REGISTER_CLIPBOARD_FORMAT(CF_GIF, CFSTR_GIF)
WINDOWS_REGISTER_CLIPBOARD_FORMAT(CF_REVOLUTION_OBJECTS, CFSTR_REVOLUTION_OBJECTS)
WINDOWS_REGISTER_CLIPBOARD_FORMAT(CF_REVOLUTION_STYLED_TEXT, CFSTR_REVOLUTION_STYLED_TEXT)

static const char *ClipboardFormatName(CLIPFORMAT p_format)
{
	static char t_name[256];
	if (GetClipboardFormatNameA(p_format, t_name, 256) == 0)
		t_name[0] = '\0';
	return t_name;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    FormatEnumerator
//

FormatEnumerator::FormatEnumerator(IDataObject *p_object)
{
	if (p_object-> EnumFormatEtc(DATADIR_GET, &m_enumerator) != S_OK)
		m_enumerator = NULL;

	memset(&m_format, 0, sizeof(FORMATETC));
}

FormatEnumerator::~FormatEnumerator(void)
{
	if (m_format . ptd != NULL)
		CoTaskMemFree(m_format . ptd);

	if (m_enumerator != NULL)
		m_enumerator -> Release();
}

bool FormatEnumerator::Valid(void) const
{
	return m_enumerator != NULL;
}

bool FormatEnumerator::Next(void)
{
	if (m_enumerator == NULL)
		return false;

	if (m_format . ptd != NULL)
	{
		CoTaskMemFree(m_format . ptd);
		m_format . ptd = NULL;
	}

	return m_enumerator -> Next(1, &m_format, NULL) == S_OK;
}

void FormatEnumerator::Reset(void)
{
	m_enumerator -> Reset();
}

FORMATETC& FormatEnumerator::operator *(void)
{
	return m_format;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    FormatData
//

FormatData::FormatData(IDataObject *p_object, FORMATETC& p_format)
{
	m_valid = p_object -> GetData(&p_format, &m_storage) == S_OK;
}

FormatData::FormatData(IDataObject *p_object, CLIPFORMAT p_clip_format)
{
	FORMATETC t_format;
	t_format . cfFormat = p_clip_format;
	t_format . dwAspect = DVASPECT_CONTENT;
	t_format . lindex = -1;
	t_format . ptd = NULL;
	if (p_clip_format == CF_ENHMETAFILE)
		t_format . tymed = TYMED_ENHMF;
	else
		t_format . tymed = TYMED_HGLOBAL;
	m_valid = p_object -> GetData(&t_format, &m_storage) == S_OK;
}

FormatData::~FormatData(void)
{
	if (m_valid)
		ReleaseStgMedium(&m_storage);
}

void FormatData::Get(MCDataRef& r_data)
{
	if (!m_valid)
	{
		r_data = MCValueRetain(kMCEmptyData);
		return;
	}

	if (m_storage . tymed != TYMED_HGLOBAL)
	{
		r_data = MCValueRetain(kMCEmptyData);
		return;
	}

	void *t_data;
	uint4 t_size;

	t_size = GlobalSize(m_storage . hGlobal);
	t_data = GlobalLock(m_storage . hGlobal);

	if (MCDataCreateWithBytes((const byte_t *)t_data, t_size, r_data))
		return;

	r_data = MCValueRetain(kMCEmptyData);
}
HGLOBAL FormatData::GetHandle(void) const
{
	if (!m_valid)
		return NULL;

	if (m_storage . tymed != TYMED_HGLOBAL && m_storage . tymed != TYMED_ENHMF)
		return NULL;

	return m_storage . hGlobal;
}

bool FormatData::Valid(void) const
{
	return m_valid;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    TransferDataEnum
//

TransferDataEnum::TransferDataEnum(void)
{
	m_references = 0;
	m_formats = NULL;
	m_format_count = 0;
	m_index = 0;
}

TransferDataEnum::~TransferDataEnum(void)
{
	delete m_formats;
}

HRESULT TransferDataEnum::QueryInterface(REFIID p_iid, void** r_interface)
{
	if (IsEqualIID(p_iid, IID_IUnknown) || IsEqualIID(p_iid, IID_IEnumFORMATETC))
	{
		*r_interface = this;
		AddRef();
		return S_OK;
	}
	
	*r_interface = NULL;
	return E_NOINTERFACE;
}

ULONG TransferDataEnum::AddRef(void)
{
	return ++m_references;
}

ULONG TransferDataEnum::Release(void)
{
	if (--m_references == 0)
	{
		delete this;
		return 0;
	}

	return m_references;
}

HRESULT TransferDataEnum::Next(ULONG p_count, LPFORMATETC r_elements, ULONG* r_fetched)
{
	if (r_fetched == NULL && p_count != 1)
		return E_POINTER;

	if (r_elements == NULL)
		return S_FALSE;

	ULONG t_actual_count;
	t_actual_count = 0;

	while(m_index < m_format_count && p_count > 0)
	{
		*r_elements++ = m_formats[m_index++];
		t_actual_count++;
		p_count--;
	}

	if (r_fetched != NULL)
		*r_fetched = t_actual_count;

	return t_actual_count == 0 ? S_FALSE : S_OK;
}

HRESULT TransferDataEnum::Skip(ULONG p_count)
{
	if (p_count + m_index >= m_format_count)
		return S_FALSE;

	m_index += p_count;

	return S_OK;
}

HRESULT TransferDataEnum::Reset(void)
{
	m_index = 0;
	return S_OK;
}

HRESULT TransferDataEnum::Clone(IEnumFORMATETC** r_clone)
{
	TransferDataEnum *t_new_enum;
	t_new_enum = new TransferDataEnum;
	if (t_new_enum == NULL)
		return E_OUTOFMEMORY;

	for(unsigned int i = 0; i < m_format_count; ++i)
		if (!t_new_enum -> Add(&m_formats[i]))
		{
			delete t_new_enum;
			return E_OUTOFMEMORY;
		}

	t_new_enum -> AddRef();
	t_new_enum -> m_index = m_index;

	*r_clone = t_new_enum;

	return S_OK;
}

bool TransferDataEnum::Add(FORMATETC *p_formats)
{
	FORMATETC *t_new_formats;
	t_new_formats = (FORMATETC *)realloc(m_formats, sizeof(FORMATETC) * (m_format_count + 1));
	if (t_new_formats == NULL)
		return false;

	t_new_formats[m_format_count] = *p_formats;

	m_formats = t_new_formats;
	m_format_count += 1;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    TransferData
//

TransferData::TransferData(void)
{
	m_references = 0;
	m_entries = NULL;
	m_entry_count = 0;
}

TransferData::~TransferData(void)
{
	for(unsigned int i = 0; i < m_entry_count; ++i)
	{
		if (m_entries[i] . data != nil)
			MCValueRelease(m_entries[i] . data);
		ReleaseStgMedium(&m_entries[i] . storage);
	}

	delete m_entries;
}

bool TransferData::Publish(FORMATETC *p_format, MCDataRef p_data, MCWindowsConversionCallback p_converter)
{
	Entry *t_new_entries;
	t_new_entries = (Entry *)realloc(m_entries, sizeof(Entry) * (m_entry_count + 1));
	if (t_new_entries == NULL)
		return false;

	t_new_entries[m_entry_count] . format = *p_format;
	t_new_entries[m_entry_count] . data = MCValueRetain(p_data);
	t_new_entries[m_entry_count] . converter = p_converter;
	t_new_entries[m_entry_count] . storage . tymed = TYMED_NULL;
	t_new_entries[m_entry_count] . storage . pUnkForRelease = NULL;

	m_entries = t_new_entries;
	m_entry_count += 1;

	return true;
}

bool TransferData::Publish(CLIPFORMAT p_clip_format, TYMED p_storage, MCDataRef p_data, MCWindowsConversionCallback p_converter)       
{
	FORMATETC t_format;
	t_format . cfFormat = p_clip_format;
	t_format . dwAspect = DVASPECT_CONTENT;
	t_format . lindex = -1;
	t_format . ptd = NULL;
	t_format . tymed = p_storage;
	
	return Publish(&t_format, p_data, p_converter);
}

bool TransferData::Publish(MCTransferType p_type, MCDataRef p_data)
{
	bool t_success;
	t_success = true;

	switch(p_type)
	{
	case TRANSFER_TYPE_TEXT:
		if (t_success)
			t_success = Publish(CF_TEXT, TYMED_HGLOBAL, p_data, MCConvertTextToWindowsAnsi);
		if (t_success)
		{
			// We use the English - United States locale for the tag. The
			// language is unimportant, the critical thing is that it will
			// map to a code page of 1252.
			unsigned int t_locale_id;
			t_locale_id = 1033;
			MCAutoDataRef t_locale_id_string;
			/* UNCHECKED */ MCDataCreateWithBytes((char_t *)&t_locale_id, 4, &t_locale_id_string);
			t_success = Publish(CF_LOCALE, TYMED_HGLOBAL, *t_locale_id_string, NULL);
		}
	break;
	case TRANSFER_TYPE_UNICODE_TEXT:
		if (t_success)
			t_success = Publish(CF_UNICODETEXT, TYMED_HGLOBAL, p_data, MCConvertUnicodeToWindowsWide);
	break;
	case TRANSFER_TYPE_STYLED_TEXT:
		// MW-2008-03-11: Disabling this clipboard format - I don't want LiveCode Text stream being
		//   published publically at the present time.
		//if (t_success)
		//	t_success = Publish(CF_REVOLUTION_STYLED_TEXT(), TYMED_HGLOBAL, p_data, NULL);

		if (t_success)
			t_success = Publish(CF_RTF(), TYMED_HGLOBAL, p_data, MCConvertStyledTextToWindowsRTF);
		if (t_success)
		{
			if (MCFormatStyledTextIsUnicode(p_data))
				t_success = Publish(CF_UNICODETEXT, TYMED_HGLOBAL, p_data, MCConvertStyledTextToWindowsWide);
			else
			{
				t_success = Publish(CF_TEXT, TYMED_HGLOBAL, p_data, MCConvertStyledTextToWindowsAnsi);
				if (t_success)
				{
					// We use the English - United States locale for the tag. The
					// language is unimportant, the critical thing is that it will
					// map to a code page of 1252.
					unsigned int t_locale_id;
					t_locale_id = 1033;
					MCAutoDataRef t_locale_id_string;
					/* UNCHECKED */ MCDataCreateWithBytes((char_t *)&t_locale_id, 4, &t_locale_id_string);
					t_success = Publish(CF_LOCALE, TYMED_HGLOBAL, *t_locale_id_string, NULL);
				}
			}
		}
	break;
	case TRANSFER_TYPE_IMAGE:
		if (t_success && MCImageDataIsPNG(p_data))
			t_success = Publish(CF_PNG(), TYMED_HGLOBAL, p_data, NULL);
		if (t_success && MCImageDataIsGIF(p_data))
			t_success = Publish(CF_GIF(), TYMED_HGLOBAL, p_data, NULL);
		if (t_success && MCImageDataIsJPEG(p_data))
			t_success = Publish(CF_JFIF(), TYMED_HGLOBAL, p_data, NULL);
		if (t_success)
			t_success = Publish(CF_METAFILEPICT, TYMED_MFPICT, p_data, MCConvertImageToWindowsMetafile);
		if (t_success)
			t_success = Publish(CF_ENHMETAFILE, TYMED_ENHMF, p_data, MCConvertImageToWindowsEnhancedMetafile);
		if (t_success)
			t_success = Publish(CF_DIB, TYMED_HGLOBAL, p_data, MCConvertImageToWindowsBitmap);
		if (t_success)
			t_success = Publish(CF_DIBV5, TYMED_HGLOBAL, p_data, MCConvertImageToWindowsV5Bitmap);
	break;
	case TRANSFER_TYPE_FILES:
		if (t_success)
			t_success = Publish(CF_HDROP, TYMED_HGLOBAL, p_data, MCConvertFilesToWindowsHDROP);
	break;
	case TRANSFER_TYPE_OBJECTS:
		if (t_success)
			t_success = Publish(CF_REVOLUTION_OBJECTS(), TYMED_HGLOBAL, p_data, NULL);
	break;
	case TRANSFER_TYPE_PRIVATE:
	break;
	}

	return t_success;
}

bool TransferData::Publish(MCPasteboard *p_pasteboard)
{
	bool t_success;
	t_success = true;

	// Attempt to fetch the types we want to publish.
	MCTransferType *t_types;
	uint4 t_type_count;
	if (t_success)
		t_success = p_pasteboard -> Query(t_types, t_type_count);

	if (t_success)
	{
		for(uint4 i = 0; i < t_type_count && t_success; ++i)
		{
			// Note that data is fetched from the pasteboard with copy semantics
			// so use MCAutoStringRef to release the value afterwards.
			MCAutoDataRef t_data;
			if (p_pasteboard -> Fetch(t_types[i], &t_data))
				t_success = Publish(t_types[i], *t_data);
			else
				t_success = false;
		}
	}

	return t_success;
}

HRESULT TransferData::Find(LPFORMATETC p_format, TransferData::Entry*& r_entry)
{
	if (p_format -> ptd != NULL)
		return DV_E_DVTARGETDEVICE;

	for(unsigned int i = 0; i < m_entry_count; ++i)
	{
		if (p_format -> cfFormat != m_entries[i] . format . cfFormat)
			continue;

		if (p_format -> dwAspect != m_entries[i] . format . dwAspect)
			continue;

		if (p_format -> lindex != m_entries[i] . format . lindex)
			continue;

		if ((p_format -> tymed & m_entries[i] . format . tymed) == 0)
			return DV_E_TYMED;

		r_entry = &m_entries[i];
		return S_OK;
	}

	return DV_E_FORMATETC;
}

HRESULT TransferData::Subscribe(TransferData::Entry *p_entry)
{
	if (p_entry -> storage . tymed == TYMED_NULL)
	{
		if (p_entry -> converter == NULL)
		{
			const char_t *t_data_ptr = MCDataGetBytePtr(p_entry->data);
			uint32_t t_data_len = MCDataGetLength(p_entry->data);
			HGLOBAL t_handle;
			t_handle = GlobalAlloc(GMEM_FIXED, t_data_len);
			if (t_handle == NULL)
				return E_OUTOFMEMORY;

			memcpy((void *)t_handle, t_data_ptr, t_data_len);

			p_entry -> storage . tymed = TYMED_HGLOBAL;
			p_entry -> storage . hGlobal = t_handle;
		}
		else if (!(p_entry -> converter)(p_entry -> data, p_entry -> storage))
			return E_OUTOFMEMORY;

		p_entry -> storage . pUnkForRelease = NULL;

		MCValueRelease(p_entry -> data);
		
		p_entry -> data = NULL;
		p_entry -> converter = NULL;
	}

	if (p_entry -> storage . tymed != TYMED_NULL)
		return S_OK;

	return E_OUTOFMEMORY;
}

HRESULT TransferData::AddRefStgMedium(STGMEDIUM* p_in, STGMEDIUM* p_out, BOOL p_copy)
{
	HRESULT t_result;
	t_result = S_OK;

	STGMEDIUM t_out;
	t_out = *p_in;

	if (p_in -> pUnkForRelease == NULL && !(p_in->tymed & (TYMED_ISTREAM | TYMED_ISTORAGE)))
	{
		if (p_copy)
		{
			if (p_in -> tymed == TYMED_HGLOBAL)
			{
				HGLOBAL t_handle;
				t_handle = NULL;

				void *t_buffer;
				t_buffer = GlobalLock(p_in -> hGlobal);
				if (t_buffer != NULL)
				{
					unsigned int t_size;
					t_size = GlobalSize(p_in -> hGlobal);
					t_handle = GlobalAlloc(GMEM_FIXED, t_size);
					if (t_handle != NULL)
						CopyMemory(t_handle, t_buffer, t_size);
					GlobalUnlock(p_in -> hGlobal);
				}

				if (t_handle != NULL)
					t_out . hGlobal = t_handle;
				else
					t_result = E_OUTOFMEMORY;
			}
			else
				t_result = DV_E_TYMED;
		}
		else
			t_out . pUnkForRelease = static_cast<IDataObject *>(this);
	}

	if (t_result == S_OK)
	{
		switch(t_out . tymed)
		{
		case TYMED_ISTREAM:
			t_out . pstm -> AddRef();
		break;
		case TYMED_ISTORAGE:
			t_out . pstg -> AddRef();
		break;
		}
		if (t_out . pUnkForRelease != NULL)
			t_out . pUnkForRelease -> AddRef();
		*p_out = t_out;
	}

	return t_result;
}

HRESULT TransferData::QueryInterface(REFIID p_iid, void** r_interface)
{
	if (IsEqualIID(p_iid, IID_IUnknown) || IsEqualIID(p_iid, IID_IDataObject))
	{
		*r_interface = this;
		AddRef();
		return S_OK;
	}
	
	*r_interface = NULL;
	return E_NOINTERFACE;
}

ULONG TransferData::AddRef(void)
{
	return ++m_references;
}

ULONG TransferData::Release(void)
{
	if (--m_references == 0)
	{
		delete this;
		return 0;
	}

	return m_references;
}

HRESULT TransferData::GetData(LPFORMATETC p_format, LPSTGMEDIUM p_medium)
{
	HRESULT t_result;
	t_result = S_OK;

	//if (p_format -> cfFormat != 49365 && p_format -> cfFormat != 49366 && p_format -> cfFormat != 49717)
	//	_RPT2(_CRT_WARN, "Fetch %s (%d)\n", ClipboardFormatName(p_format -> cfFormat), p_format -> cfFormat);

	Entry *t_entry;
	if (t_result == S_OK)
		t_result = Find(p_format, t_entry);

	if (t_result == S_OK)
		t_result = Subscribe(t_entry);

	if (t_result == S_OK)
		t_result = AddRefStgMedium(&t_entry -> storage, p_medium, FALSE);

	return t_result;
}

HRESULT TransferData::GetDataHere(LPFORMATETC p_format, LPSTGMEDIUM p_medium)
{
	//Entry *t_entry;
	return E_NOTIMPL; //Find(p_format, t_entry);
}

HRESULT TransferData::QueryGetData(LPFORMATETC p_format)
{
	Entry *t_entry;
	return Find(p_format, t_entry);
}

HRESULT TransferData::EnumFormatEtc(DWORD p_direction, LPENUMFORMATETC* r_enum)
{
	TransferDataEnum *t_enum;
	t_enum = new TransferDataEnum;
	if (t_enum == NULL)
		return E_OUTOFMEMORY;

	if (p_direction == DATADIR_GET)
	{
		for(unsigned int i = 0; i < m_entry_count; ++i)
		{
			if (!t_enum -> Add(&m_entries[i] . format))
			{
				delete t_enum;
				return E_OUTOFMEMORY;
			}
		}
	}

	t_enum -> AddRef();

	*r_enum = t_enum;

	return S_OK;
}

HRESULT TransferData::GetCanonicalFormatEtc(LPFORMATETC p_format_in, LPFORMATETC p_format_out)
{
	p_format_out -> cfFormat = p_format_in -> cfFormat;
	p_format_out -> dwAspect = p_format_in -> dwAspect;
	p_format_out -> lindex = -1;
	p_format_out -> ptd = NULL;
	p_format_out -> tymed = p_format_in -> tymed;
	return DATA_S_SAMEFORMATETC;
}

static IUnknown *GetCanonicalIUnknown(IUnknown *p_unknown)
{
	IUnknown *t_canonical;
	if (p_unknown != NULL && p_unknown -> QueryInterface(IID_IUnknown, (void **)&t_canonical) == S_OK)
		t_canonical -> Release();
	else
		t_canonical = p_unknown;
	return t_canonical;
}

HRESULT TransferData::SetData(LPFORMATETC p_format, STGMEDIUM* p_medium, BOOL p_release)
{
	Entry *t_entry;
	if (Find(p_format, t_entry) != S_OK)
	{
		Entry *t_new_entries;
		t_new_entries = (Entry *)realloc(m_entries, sizeof(Entry) * (m_entry_count + 1));
		if (t_new_entries == NULL)
			return E_OUTOFMEMORY;

		t_entry = &t_new_entries[m_entry_count];

		m_entries = t_new_entries;
		m_entry_count += 1;
	}
	else
	{
		if (t_entry -> data != NULL)
			MCValueRelease(t_entry -> data);

		ReleaseStgMedium(&t_entry -> storage);

		t_entry -> format . cfFormat = 0;
		t_entry -> converter = NULL;
		t_entry -> data = NULL; 
		t_entry -> storage . tymed = TYMED_NULL;
		t_entry -> storage . pUnkForRelease = NULL;
	}

	HRESULT t_result;
	t_result = AddRefStgMedium(p_medium, &t_entry -> storage, !p_release);
	if (t_result == S_OK)
	{
		t_entry -> format = *p_format;
		t_entry -> data = NULL;
		t_entry -> converter = NULL;

		// If adding a reference to the storage medium resulted in us being referenced
		// we must break the loop...
		if (GetCanonicalIUnknown(t_entry -> storage . pUnkForRelease) ==
				GetCanonicalIUnknown(static_cast<IDataObject *>(this)))
		{
			t_entry -> storage . pUnkForRelease -> Release();
			t_entry -> storage . pUnkForRelease = NULL;
		}
	}

	return t_result;
}

HRESULT TransferData::DAdvise(FORMATETC* p_format, DWORD p_flags, LPADVISESINK p_sink, DWORD *p_connection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT TransferData::DUnadvise(DWORD p_connection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT TransferData::EnumDAdvise(LPENUMSTATDATA *r_enum)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    MCWindowsPasteboard
//

MCWindowsPasteboard::MCWindowsPasteboard(IDataObject *p_object)
{
	m_references = 1;

	m_data_object = p_object;
	m_data_object -> AddRef();

	m_entry_count = 0;
	m_entries = NULL;

	m_types = NULL;

	m_valid = false;

	Resolve();
}

MCWindowsPasteboard::~MCWindowsPasteboard(void)
{
	// Release any cached shared strings.
	for(unsigned int i = 0; i < m_entry_count; ++i)
		if (m_entries[i] . data != NULL)
			MCValueRelease(m_entries[i] . data);
	
	// Release the system object
	m_data_object -> Release();

	// Delete the entries array
	delete m_entries;

	// Delete the type array
	delete m_types;
}

void MCWindowsPasteboard::Retain(void)
{
	m_references += 1;
}

void MCWindowsPasteboard::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
		delete this;
}

// This function determines the primary types of the given data object. The
// primary types are that which is presented to the LiveCode application as
// 'the' types of the clipboard/drag-and-drop data.
//
// The clipboard formats that are understood by LiveCode are:
//   - CF_TEXT/CF_LOCALE (class text)
//   - CF_UNICODETEXT (class text)
//   - CF_RTF (class text)
//   - CF_DIB (class image)
//   - CF_DIBV5 (class image)
//   - CF_ENHMETAFILE (class image)
//   - CF_HDROP (class files)
//   - "com.runrev.revolution.object" (class objects)
//   - "com.runrev.revolution.text/styled" (class text)
//   - "com.runrev.revolution.image/gif" (class image)
//   - "com.runrev.revolution.image/png" (class image)
//   - "com.runrev.revolution.image/jpeg" (class image)
//
// The priority of the classes is:
//   objects
//   image text
//   files
//
// If both image and text is present, then the primary class is whichever
// appears first in enumeration.
//
// If both objects and image are present, they are assumed to represent
// the same object and are two types will be returned by the object.
//
// If the primary class is text and there is both TEXT and UNICODETEXT
// present, the LOCALE format is used to disambiguate. If LOCALE is
// Windows-1252, TEXT is returned, else UNICODETEXT is returned. If
// LOCALE is not present, the data associated to the UNICODETEXT is
// fetched an attempt to convert to Windows-1252 is made. If the
// conversion attempt is successfule, the result is TEXT.
//
void MCWindowsPasteboard::Resolve(void)
{
	// The priority variables contain the index of location of the first format
	// of the given class that has been found, or 0 if none of that class have
	// been found.

	unsigned int t_objects_priority;
	t_objects_priority = 0xFFFFFFFFU;
	
	unsigned int t_image_priority;
	t_image_priority = 0xFFFFFFFFU;

	unsigned int t_text_priority;
	t_text_priority = 0xFFFFFFFFU;

	unsigned int t_files_priority;
	t_files_priority = 0xFFFFFFFFU;

	// These flags are used to disambiguate the text type
	bool t_text_has_locale;
	t_text_has_locale = false;

	bool t_text_has_plain;
	t_text_has_plain = false;

	CLIPFORMAT t_text_format;
	t_text_format = 0;

	CLIPFORMAT t_image_format;
	t_image_format = 0;

	CLIPFORMAT t_objects_format;
	t_objects_format = 0;

	// This variable contains the current index of the format as presented by
	// the enumerator
	unsigned int t_priority;
	t_priority = 0;

	FormatEnumerator t_enum(m_data_object);
	if (!t_enum . Valid())
	{
		m_valid = false;
		return;
	}

	// MW-2008-08-26: [[ Bug 6982 ]] CF_BITMAP format should be handled the same as CF_DIB (Windows does
	//   an auto-conversion from BITMAP -> DIB).
	while(t_enum . Next())
	{
		CLIPFORMAT t_format;
		t_format = (*t_enum) . cfFormat;

		switch(t_format)
		{
		case CF_TEXT:
			t_text_priority = MCU_min(t_priority, t_text_priority);
			if (t_text_format == 0)
				t_text_format = CF_TEXT;
			t_text_has_plain = true;
		break;
		case CF_LOCALE:
			t_text_has_locale = true;
		break;
		case CF_UNICODETEXT:
			t_text_priority = MCU_min(t_priority, t_text_priority);
			if (t_text_format == 0 || t_text_format == CF_TEXT)
				t_text_format = CF_UNICODETEXT;
		break;
		case CF_BITMAP:
		case CF_DIB:
			t_image_priority = MCU_min(t_priority, t_image_priority);
			if (t_image_format == 0)
				t_image_format = CF_DIB;
		break;
		case CF_DIBV5:
			t_image_priority = MCU_min(t_priority, t_image_priority);
			if (t_image_format == 0 || t_image_format == CF_DIB)
				t_image_format = CF_DIBV5;
		break;
		case CF_ENHMETAFILE:
			t_image_priority = MCU_min(t_priority, t_image_priority);
			if (t_image_format == 0 || t_image_format == CF_DIB || t_image_format == CF_DIBV5)
				t_image_format = CF_ENHMETAFILE;
		break;
		case CF_HDROP:
			t_files_priority = MCU_min(t_priority, t_files_priority);
		break;
		default:
			if (t_format == CF_RTF())
			{
				t_text_priority = MCU_min(t_priority, t_text_priority);
				if (t_text_format == 0 || t_text_format == CF_TEXT || t_text_format == CF_UNICODETEXT)
					t_text_format = t_format;
			}
			else if (t_format == CF_REVOLUTION_STYLED_TEXT())
			{
				t_text_priority = MCU_min(t_priority, t_text_priority);
				t_text_format = t_format;
			}
			else if (t_format == CF_REVOLUTION_OBJECTS())
			{
				t_objects_priority = MCU_min(t_priority, t_objects_priority);
				t_objects_format = t_format;
			}
			else if (t_format == CF_GIF())
			{
				t_image_priority = MCU_min(t_priority, t_image_priority);
				if (t_image_format == 0 || t_image_format == CF_DIB || t_image_format == CF_DIBV5 || t_image_format == CF_ENHMETAFILE)
					t_image_format = t_format;
			}
			else if (t_format == CF_PNG())
			{
				t_image_priority = MCU_min(t_priority, t_image_priority);
				if (t_image_format == 0 || t_image_format == CF_DIB || t_image_format == CF_DIBV5 || t_image_format == CF_ENHMETAFILE || t_image_format == CF_GIF() || t_image_format == CF_JFIF())
					t_image_format = t_format;
			}
			else if (t_format == CF_JFIF())
			{
				t_image_priority = MCU_min(t_priority, t_image_priority);
				if (t_image_format == 0 || t_image_format == CF_DIB || t_image_format == CF_DIBV5 || t_image_format == CF_ENHMETAFILE)
					t_image_format = t_format;
			}
			else
			{
				char t_format_name[256];
				GetClipboardFormatNameA(t_format, t_format_name, 256);
				
				int t_foo;
				t_foo = 0;
			}
		break;
		}
	}

	if (t_objects_priority != 0xFFFFFFFFU)
	{
		AddEntry(TRANSFER_TYPE_OBJECTS, t_objects_format);
		if (t_image_priority != 0xFFFFFFFFU)
			AddEntry(TRANSFER_TYPE_IMAGE, t_image_format);
	}
	else if (t_image_priority != 0xFFFFFFFFU || t_text_priority != 0xFFFFFFFFU)
	{
		if (t_image_priority < t_text_priority)
			// An image type was published first, so the primary format is 'IMAGE'
			AddEntry(TRANSFER_TYPE_IMAGE, t_image_format); 
		else if (t_text_format == CF_UNICODETEXT)
			AddEntry(TRANSFER_TYPE_UNICODE_TEXT, CF_UNICODETEXT);
		else if (t_text_format == CF_TEXT)
		{
			// A locale format was published, so fetch it and check it is
			// Windows-1252. If it isn't, then the text must be unicode.
			if (t_text_has_locale)
			{
				DWORD t_codepage;
				t_codepage = 0;

				FormatData t_locale_data(m_data_object, CF_LOCALE);
				if (t_locale_data . Valid())
				{
					MCAutoDataRef t_locale_string;
					/* UNCHECKED */ t_locale_data . Get(&t_locale_string);

					if (MCDataGetLength(*t_locale_string) == 4)
					{
						LCID t_locale_id;
						memcpy(&t_locale_id, MCDataGetBytePtr(*t_locale_string), 4);
						if (GetLocaleInfoA(t_locale_id, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER, (LPSTR)&t_codepage, 4) != 4)
							t_codepage = 0;
					}
				}

				if (t_codepage != 1252)
					AddEntry(TRANSFER_TYPE_UNICODE_TEXT, CF_UNICODETEXT);
				else
					AddEntry(TRANSFER_TYPE_TEXT, CF_TEXT);
			}
		}
		else
			AddEntry(TRANSFER_TYPE_STYLED_TEXT, t_text_format);
	}
	else if (t_files_priority != 0xFFFFFFFFU)
		AddEntry(TRANSFER_TYPE_FILES, CF_HDROP);
	
	m_valid = true;
}

bool MCWindowsPasteboard::AddEntry(MCTransferType p_type, CLIPFORMAT p_format)
{
	Entry *t_new_entries;
	t_new_entries = (Entry *)realloc(m_entries, sizeof(Entry) * (m_entry_count + 1));
	if (t_new_entries == NULL)
		return false;

	t_new_entries[m_entry_count] . type = p_type;
	t_new_entries[m_entry_count] . format = p_format;
	t_new_entries[m_entry_count] . data = NULL;

	m_entries = t_new_entries;
	m_entry_count += 1;

	return true;
}

bool MCWindowsPasteboard::Query(MCTransferType*& r_types, unsigned int& r_type_count)
{
	if (!m_valid)
		return false;

	if (m_types == NULL)
	{
		m_types = new MCTransferType[m_entry_count];
		if (m_types == NULL)
			return false;

		for(unsigned int i = 0; i < m_entry_count; ++i)
			m_types[i] = m_entries[i] . type;
	}

	r_types = m_types;
	r_type_count = m_entry_count;

	return true;
}

extern bool create_temporary_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits);

static inline uint32_t packed_scale_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v;

	u = ((x & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u + v;
}

typedef void (*MCGDIDrawFunc)(HDC p_hdc, void *p_context);

bool MCGDIDrawAlpha(uint32_t p_width, uint32_t p_height, MCGDIDrawFunc p_draw, void *p_context, MCImageBitmap *&r_bitmap);

void MCGDIDrawMetafile(HDC p_dc, void *context)
{
	/* OVERHAUL - REVISIT - code for rendering metafile to a bitmap */
	HENHMETAFILE t_metafile;	
	t_metafile = (HENHMETAFILE)context;

	bool t_success;
	t_success = true;

	ENHMETAHEADER t_header;
	if (GetEnhMetaFileHeader(t_metafile, sizeof(ENHMETAHEADER), &t_header) == 0)
		t_success = false;

	// MW-2012-11-29: [[ Bug 10556 ]] Make sure we create a bitmap of the size of the bounds
	//   of the metafile, rather than original device size.
	uint4 t_width, t_height;
	if (t_success)
	{
		t_width = t_header . rclBounds . right - t_header . rclBounds . left;
		t_height = t_header . rclBounds . bottom - t_header . rclBounds . top;
	}

	if (t_success)
	{
		SaveDC(p_dc);
		SetMapMode(p_dc, MM_ANISOTROPIC);
		// MW-2012-11-29: [[ Bug 10556 ]] Map the top-left of the metafile frame to 0, 0 in device
		//   coords.
		SetWindowOrgEx(p_dc, t_header . rclFrame . left, t_header . rclFrame . top, NULL);
		SetWindowExtEx(p_dc, t_header . szlMillimeters . cx * 100, t_header . szlMillimeters . cy * 100, NULL);
		SetViewportExtEx(p_dc, t_header . szlDevice . cx, t_header . szlDevice . cy, NULL);

		RECT t_rect;
		SetRect(&t_rect, t_header . rclFrame . left, t_header . rclFrame . top, t_header . rclFrame . right, t_header . rclFrame . bottom);
		PlayEnhMetaFile(p_dc, t_metafile, &t_rect);
		RestoreDC(p_dc, -1);
	}
}

bool MCWindowsPasteboard::Fetch(MCTransferType p_type, MCDataRef& r_data)
{
	if (!m_valid)
		return false;

	unsigned int t_entry;
	for(t_entry = 0; t_entry < m_entry_count; ++t_entry)
		if (m_entries[t_entry] . type == p_type)
			break;

	if (t_entry == m_entry_count)
		return false;

	if (m_entries[t_entry] . data != NULL)
	{
		r_data = MCValueRetain(m_entries[t_entry] . data);
		return true;
	}

	MCAutoDataRef t_out_data;

	FormatData t_in_data(m_data_object, m_entries[t_entry] . format);
	if (!t_in_data . Valid())
		return false;

	MCAutoDataRef t_in_string;
	t_in_data . Get(&t_in_string);

	switch(m_entries[t_entry] . format)
	{
	case CF_TEXT:
	{
		 MCStringRef t_input, t_output;
		 /* UNCHECKED */ MCStringDecode(*t_in_string, kMCStringEncodingWindows1252, false, t_input);
		 if (MCStringGetNativeCharAtIndex(t_input, MCStringGetLength(t_input) - 1) == '\0')
		 {
			 MCStringMutableCopyAndRelease(t_input, t_input);
			 MCStringRemove(t_input, MCRangeMake(MCStringGetLength(t_input) - 1, 1));
		 }
		 /* UNCHECKED */ MCStringConvertLineEndingsToLiveCode(t_input, t_output);
		 /* UNCHECKED */ MCStringEncode(t_output, kMCStringEncodingWindows1252, false, &t_out_data);
		 MCValueRelease(t_input);
		 MCValueRelease(t_output);
	}
	break;
	case CF_UNICODETEXT:
	{
		MCStringRef t_input, t_output;
		 /* UNCHECKED */ MCStringDecode(*t_in_string, kMCStringEncodingUTF16, false, t_input);
		 if (MCStringGetNativeCharAtIndex(t_input, MCStringGetLength(t_input) - 1) == '\0')
		 {
			 MCStringMutableCopyAndRelease(t_input, t_input);
			 MCStringRemove(t_input, MCRangeMake(MCStringGetLength(t_input) - 1, 1));
		 }
		 /* UNCHECKED */ MCStringConvertLineEndingsToLiveCode(t_input, t_output);
		 /* UNCHECKED */ MCStringEncode(t_output, kMCStringEncodingUTF16, false, &t_out_data);
		 MCValueRelease(t_input);
		 MCValueRelease(t_output);
	}
	break;
	case CF_DIB:
	case CF_DIBV5:
	{
		/* OVERHAUL - REVISIT - now that MCImageDecodeBMP() understands 16 bit (555 / 565) bitmaps
		we could prepend a BMP file header to the data rather than convert to PNG */
		bool t_success = true;

		IO_handle t_stream = nil;
		MCImageBitmap *t_bitmap = nil;
		uindex_t t_byte_count = 0;
		char *t_buffer = nil;
		uint32_t t_length = 0;

        if (t_success)
			t_success = nil != (t_stream = MCS_fakeopen(MCDataGetBytePtr(*t_in_string), MCDataGetLength(*t_in_string)));

		if (t_success)
			t_success = MCImageDecodeBMPStruct(t_stream, t_byte_count, t_bitmap);

		if (t_stream != nil)
			MCS_close(t_stream);
		t_stream = nil;

		if (t_success)
			t_success = nil != (t_stream = MCS_fakeopenwrite());

		if (t_success)
			t_success = MCImageEncodePNG(t_bitmap, NULL, t_stream, t_byte_count);

		if (t_success)
			t_success = IO_NORMAL == MCS_closetakingbuffer_uint32(t_stream, *(void**)(&t_buffer), t_length);

		if (t_success)
			t_success = MCDataCreateWithBytesAndRelease((char_t*)t_buffer, t_length, &t_out_data);

		MCImageFreeBitmap(t_bitmap);
	}
	break;
	case CF_ENHMETAFILE:
	{
		/* OVERHAUL - REVISIT - code for rendering metafile to a bitmap */
		HENHMETAFILE t_metafile;	
		t_metafile = (HENHMETAFILE)t_in_data . GetHandle();

		bool t_success;
		t_success = true;

		ENHMETAHEADER t_header;
		if (GetEnhMetaFileHeader(t_metafile, sizeof(ENHMETAHEADER), &t_header) == 0)
			t_success = false;

		// MW-2012-11-29: [[ Bug 10556 ]] Make sure we create a bitmap of the size of the bounds
		//   of the metafile, rather than original device size.
		uint4 t_width, t_height;
		if (t_success)
		{
			t_width = t_header . rclBounds . right - t_header . rclBounds . left;
			t_height = t_header . rclBounds . bottom - t_header . rclBounds . top;
		}

		MCImageBitmap *t_bitmap = nil;
		if (t_success)
			t_success = MCGDIDrawAlpha(t_width, t_height, MCGDIDrawMetafile, t_metafile, t_bitmap);

		IO_handle t_stream = nil;
		char *t_bytes = nil;
		uindex_t t_byte_count = 0;
		if (t_success)
			t_success = nil != (t_stream = MCS_fakeopenwrite());

		if (t_success)
		{
			t_success = MCImageEncodePNG(t_bitmap, NULL, t_stream, t_byte_count);
		}
        
        if (t_success)
        {
			/* UNCHECKED */ MCS_closetakingbuffer(t_stream, *(void**)(&t_bytes), t_byte_count);
			/* UNCHECKED */ MCDataCreateWithBytes((char_t*)t_bytes, t_byte_count, &t_out_data);
			MCMemoryDeallocate(t_bytes);
		}

		MCImageFreeBitmap(t_bitmap);
	}
	break;
	case CF_HDROP:
	{
		HDROP t_hdrop;
		t_hdrop = (HDROP)t_in_data . GetHandle();

		UINT t_count;
		t_count = DragQueryFileW(t_hdrop, 0xFFFFFFFF, NULL, 0);
		
		// Create a mutable list ref.
		MCListRef t_output;
		/* UNCHECKED */ MCListCreateMutable('\n', t_output);
		for(unsigned int i = 0; i < t_count; ++i)
		{
			UINT t_size;
			t_size = DragQueryFileW(t_hdrop, i, NULL, 0);

			MCAutoArray<unichar_t> t_buffer;
			// SN-2014-07-24: [[ Bug 12953 ]] The buffer must be able to contain the
			//  NULL-terminated char, which is not included in the size returned by DragQueryFileW
			/* UNCHECKED */ t_buffer.New(t_size + 1);

			DragQueryFileW(t_hdrop, i, t_buffer.Ptr(), t_buffer.Size());
			MCAutoStringRef t_std_path;
			MCAutoStringRef t_native_path;

			// SN-2014-08-25: [[ Bug 13258 ]] We don't want to put the last, NULL char in the output string
			/* UNCHECKED */ MCStringCreateWithChars(t_buffer.Ptr(), t_size, &t_std_path);
			/* UNCHECKED */ MCS_pathtonative(*t_std_path, &t_native_path);
			
			/* UNCHECKED */ MCListAppend(t_output, *t_native_path);
		}
		MCAutoStringRef t_out_string;
		/* UNCHECKED */ MCListCopyAsStringAndRelease(t_output, &t_out_string);
		// SN-2014-11-13: [[ Bug 13993 ]] The files are now stored in UTF-16 in the clipboard
		/* UNCHECKED */ MCStringEncode(*t_out_string, kMCStringEncodingUTF16, false, &t_out_data);
	}
	break;
	default:
	{
		if (m_entries[t_entry] . format == CF_RTF())
			/* UNCHECKED */ MCConvertRTFToStyledText(*t_in_string, &t_out_data);
		else if (m_entries[t_entry] . format == CF_GIF())
			t_out_data = *t_in_string;
		else if (m_entries[t_entry] . format == CF_PNG())
			t_out_data = *t_in_string;
		else if (m_entries[t_entry] . format == CF_JFIF())
			t_out_data = *t_in_string;
		else if (m_entries[t_entry] . format == CF_REVOLUTION_STYLED_TEXT())
			t_out_data = *t_in_string;
		else if (m_entries[t_entry] . format == CF_REVOLUTION_OBJECTS())
			t_out_data = *t_in_string;
	}
	break;
	}

	if (*t_out_data == nil)
		return false;

	m_entries[t_entry] . data = MCValueRetain(*t_out_data);
	r_data = MCValueRetain(*t_out_data);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  MCConvert support functions for Windows clipboard formats
//

static bool CloneStringToStorage(const MCString& p_string, STGMEDIUM& r_storage)
{
	HGLOBAL t_handle;
	t_handle = GlobalAlloc(GMEM_FIXED, p_string . getlength());
	if (t_handle != NULL)
	{
		memcpy(t_handle, p_string . getstring(), p_string . getlength());
		r_storage . tymed = TYMED_HGLOBAL;
		r_storage . hGlobal = t_handle;
	}

	return t_handle != NULL;
}

static bool CloneStringToStorage(MCDataRef p_string, STGMEDIUM& r_storage)
{
	HGLOBAL t_handle;
	t_handle = GlobalAlloc(GMEM_FIXED, MCDataGetLength(p_string));
	if (t_handle != NULL)
	{
		memcpy(t_handle, MCDataGetBytePtr(p_string), MCDataGetLength(p_string));
		r_storage . tymed = TYMED_HGLOBAL;
		r_storage . hGlobal = t_handle;
	}

	return t_handle != NULL;
}

// Windows Ansi Text format is a NUL terminated string of characters in the
// default ANSI codepage of the locale id specified in an accompnying
// CF_LOCALE.
//
// The line separator is always CRLF.
//
bool MCConvertTextToWindowsAnsi(MCDataRef p_input, STGMEDIUM& r_storage)
{
	MCAutoStringRef t_input, t_output;
	/* UNCHECKED */ MCStringDecode(p_input, kMCStringEncodingWindows1252, false, &t_input);
	/* UNCHECKED */ MCStringConvertLineEndingsFromLiveCode(*t_input, &t_output);
	MCAutoDataRef t_output_data;
	/* UNCHECKED */ MCStringEncode(*t_output, kMCStringEncodingWindows1252, false, &t_output_data);

	if (MCDataIsEmpty(*t_output_data) || MCDataGetByteAtIndex(*t_output_data, MCDataGetLength(*t_output_data) - 1) != '\0')
	{
		MCAutoDataRef t_output_data_mutable;
		/* UNCHECKED */ MCDataMutableCopy(*t_output_data, &t_output_data_mutable);
		/* UNCHECKED */ MCDataPad(*t_output_data_mutable, '\0', 1);
		return CloneStringToStorage(*t_output_data_mutable, r_storage);
	}

	return CloneStringToStorage(*t_output_data, r_storage);
}

// Windows Wide Text format is a NUL terminated string of UTF-16 codepoints.
//
// The line separator is always CRLF.
//
bool MCConvertTextToWindowsWide(MCDataRef p_input, STGMEDIUM& r_storage)
{
	MCAutoStringRef t_input, t_output;
	/* UNCHECKED */ MCStringDecode(p_input, kMCStringEncodingWindows1252, false, &t_input);
	/* UNCHECKED */ MCStringConvertLineEndingsFromLiveCode(*t_input, &t_output);
	MCAutoDataRef t_output_data;
	/* UNCHECKED */ MCStringEncode(*t_output, kMCStringEncodingUTF16, false, &t_output_data);

	if (MCDataGetLength(*t_output_data) < 2 || MCDataGetByteAtIndex(*t_output_data, MCDataGetLength(*t_output_data) - 2) != '\0' || MCDataGetByteAtIndex(*t_output_data, MCDataGetLength(*t_output_data) - 1) != '\0')
	{
		MCAutoDataRef t_output_data_mutable;
		/* UNCHECKED */ MCDataMutableCopy(*t_output_data, &t_output_data_mutable);
		/* UNCHECKED */ MCDataPad(*t_output_data_mutable, '\0', 2);
		return CloneStringToStorage(*t_output_data_mutable, r_storage);
	}

	return CloneStringToStorage(*t_output_data, r_storage);
}

bool MCConvertUnicodeToWindowsAnsi(MCDataRef p_input, STGMEDIUM& r_storage)
{
	MCAutoStringRef t_input, t_output;
	/* UNCHECKED */ MCStringDecode(p_input, kMCStringEncodingUTF16, false, &t_input);
	/* UNCHECKED */ MCStringConvertLineEndingsFromLiveCode(*t_input, &t_output);
	MCAutoDataRef t_output_data;
	/* UNCHECKED */ MCStringEncode(*t_output, kMCStringEncodingWindows1252, false, &t_output_data);

	if (MCDataIsEmpty(*t_output_data) || MCDataGetByteAtIndex(*t_output_data, MCDataGetLength(*t_output_data) - 1) != '\0')
	{
		MCAutoDataRef t_output_data_mutable;
		/* UNCHECKED */ MCDataMutableCopy(*t_output_data, &t_output_data_mutable);
		/* UNCHECKED */ MCDataPad(*t_output_data_mutable, '\0', 1);
		return CloneStringToStorage(*t_output_data_mutable, r_storage);
	}

	return CloneStringToStorage(*t_output_data, r_storage);
}

bool MCConvertUnicodeToWindowsWide(MCDataRef p_input, STGMEDIUM& r_storage)
{
	MCAutoStringRef t_input, t_output;
	/* UNCHECKED */ MCStringDecode(p_input, kMCStringEncodingUTF16, false, &t_input);
	/* UNCHECKED */ MCStringConvertLineEndingsFromLiveCode(*t_input, &t_output);
	MCAutoDataRef t_output_data;
	/* UNCHECKED */ MCStringEncode(*t_output, kMCStringEncodingUTF16, false, &t_output_data);

	if (MCDataGetLength(*t_output_data) < 2 || MCDataGetByteAtIndex(*t_output_data, MCDataGetLength(*t_output_data) - 2) != '\0' || MCDataGetByteAtIndex(*t_output_data, MCDataGetLength(*t_output_data) - 1) != '\0')
	{
		MCAutoDataRef t_output_data_mutable;
		/* UNCHECKED */ MCDataMutableCopy(*t_output_data, &t_output_data_mutable);
		/* UNCHECKED */ MCDataPad(*t_output_data_mutable, '\0', 2);
		return CloneStringToStorage(*t_output_data_mutable, r_storage);
	}

	return CloneStringToStorage(*t_output_data, r_storage);
}

bool MCConvertStyledTextToWindowsAnsi(MCDataRef p_input, STGMEDIUM& r_storage)
{
	bool t_result;
	t_result = false;

	MCAutoDataRef t_text;
	MCConvertStyledTextToText(p_input, &t_text);
	if (*t_text != nil)
		t_result = MCConvertTextToWindowsAnsi(*t_text, r_storage);

	return t_result;
}

bool MCConvertStyledTextToWindowsWide(MCDataRef p_input, STGMEDIUM& r_storage)
{
	bool t_result;
	t_result = false;

	MCAutoDataRef t_text;
	MCConvertStyledTextToUnicode(p_input, &t_text);
	if (*t_text != nil)
		t_result = MCConvertUnicodeToWindowsWide(*t_text, r_storage);

	return t_result;
}

bool MCConvertImageToWindowsBitmap(MCDataRef p_input, STGMEDIUM& r_storage)
{
	const uint8_t *t_data = MCDataGetBytePtr(p_input);
	uint32_t t_length = MCDataGetLength(p_input);

	bool t_success = true;

	MCWinSysHandle t_handle = nil;
	MCBitmapFrame *t_frames = nil;
	uindex_t t_frame_count = 0;

	t_success = MCImageDecode(t_data, t_length, t_frames, t_frame_count) &&
		MCImageBitmapToDIB(t_frames[0].image, t_handle);

	MCImageFreeFrames(t_frames, t_frame_count);

	if (t_success)
	{
		r_storage . tymed = TYMED_HGLOBAL;
		r_storage . hGlobal = t_handle;
	}

	return t_success;
}

bool MCConvertImageToWindowsV5Bitmap(MCDataRef p_input, STGMEDIUM& r_storage)
{
	const uint8_t *t_data = MCDataGetBytePtr(p_input);
	uint32_t t_length = MCDataGetLength(p_input);	

	bool t_success = true;

	MCWinSysHandle t_handle = nil;
	MCBitmapFrame *t_frames = nil;
	uindex_t t_frame_count = 0;

	t_success = MCImageDecode(t_data, t_length, t_frames, t_frame_count) &&
		MCImageBitmapToV5DIB(t_frames[0].image, t_handle);

	MCImageFreeFrames(t_frames, t_frame_count);

	if (t_success)
	{
		r_storage . tymed = TYMED_HGLOBAL;
		r_storage . hGlobal = t_handle;
	}

	return t_success;
}

bool MCConvertImageToWindowsEnhancedMetafile(MCDataRef p_input, STGMEDIUM& r_storage)
{
    const uint8_t *t_data = MCDataGetBytePtr(p_input);
	uint32_t t_length = MCDataGetLength(p_input);
    
	bool t_success = true;

	MCWinSysEnhMetafileHandle t_handle = nil;
	MCBitmapFrame *t_frames = nil;
	uindex_t t_frame_count = 0;

	t_success = MCImageDecode(t_data, t_length, t_frames, t_frame_count) &&
		MCImageBitmapToEnhancedMetafile(t_frames[0].image, t_handle);

	MCImageFreeFrames(t_frames, t_frame_count);

	if (t_success)
	{
		r_storage . tymed = TYMED_ENHMF;
		r_storage . hEnhMetaFile = (HENHMETAFILE)t_handle;
	}

	return t_success;
}

bool MCConvertImageToWindowsMetafile(MCDataRef p_input, STGMEDIUM& r_storage)
{
	const uint8_t *t_data = MCDataGetBytePtr(p_input);
	uint32_t t_length = MCDataGetLength(p_input);	

	bool t_success = true;

	MCWinSysMetafileHandle t_handle = nil;
	MCBitmapFrame *t_frames = nil;
	uindex_t t_frame_count = 0;

	t_success = MCImageDecode(t_data, t_length, t_frames, t_frame_count) &&
		MCImageBitmapToMetafile(t_frames[0].image, t_handle);

	MCImageFreeFrames(t_frames, t_frame_count);

	if (t_success)
	{
		r_storage . tymed = TYMED_MFPICT;
		r_storage . hMetaFilePict = t_handle;
	}

	return t_success;
}

bool MCConvertFilesToWindowsHDROP(MCDataRef p_input, STGMEDIUM& r_storage)
{
	const uint8_t *t_data = MCDataGetBytePtr(p_input);
	uint32_t t_length = MCDataGetLength(p_input);
	
	HGLOBAL t_handle;
	t_handle = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, sizeof(DROPFILES) + t_length + 2);
	if (t_handle == NULL)
		return false;

	LPDROPFILES t_drop_files;
	t_drop_files = (LPDROPFILES)GlobalLock(t_handle);
	t_drop_files -> fWide = False;
	t_drop_files -> pFiles = sizeof(DROPFILES);

	const char *t_buffer;
	t_buffer = (const char *)t_data;

	char *t_output_buffer;
	t_output_buffer = (char *)(t_drop_files + 1);

	do
	{
		const char *t_start;
		t_start = t_buffer;
		if (!MCU_strchr(t_buffer, t_length, '\n', False))
		{
			t_buffer += t_length;
			t_length = 0;
		}

		memcpy(t_output_buffer, t_start, t_buffer - t_start);
		t_output_buffer += t_buffer - t_start;

		*t_output_buffer = '\0';
		t_output_buffer += 1;
	}
	while(t_length > 0);

	*t_output_buffer = '\0';

	GlobalUnlock(t_handle);

	r_storage . tymed = TYMED_HGLOBAL;
	r_storage . hGlobal = t_handle;

	return true;
}

bool MCConvertStyledTextToWindowsRTF(MCDataRef p_input, STGMEDIUM& r_storage)
{
	bool t_success;
	t_success = true;

	MCAutoDataRef t_rtf;
	MCConvertStyledTextToRTF(p_input, &t_rtf);
	if (*t_rtf != nil)
		t_success = CloneStringToStorage(*t_rtf, r_storage);
	else
		t_success = false;

	return t_success;
}
