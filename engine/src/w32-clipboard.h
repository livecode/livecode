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


#ifndef W32_CLIPBOARD_H
#define W32_CLIPBOARD_H


#include "clipboard.h"
#include "foundation-auto.h"

#include <ObjIdl.h>


// Forward declarations of classes
class MCWin32RawClipboardCommon;
class MCWin32RawClipboard;
class MCWin32RawClipboardNull;
class MCWin32RawClipboardItem;
class MCWin32RawClipboardItemRep;
class MCWin32DataObject;
class MCWin32DataObjectFormatEnum;


class MCWin32RawClipboardItemRep :
  public MCRawClipboardItemRep
{
public:
    
    // Inherited from MCRawClipboardItemRep
    virtual MCStringRef CopyTypeString() const;
    virtual MCDataRef CopyData() const;
    
private:

    // Parent clipboard item structure
    MCWin32RawClipboardItem* m_item;
    
    // The FORMATETC structure used by Windows to identify this representation
    FORMATETC m_format;

	// Cache of the type string and data for this representation
	mutable MCAutoStringRef m_type;
	mutable MCAutoDataRef m_bytes;

	// Controlled by the parent MCWin32RawClipboardItem
	friend class MCWin32RawClipboardItem;
	MCWin32RawClipboardItemRep(MCWin32RawClipboardItem* p_parent, const FORMATETC& p_format);
	MCWin32RawClipboardItemRep(MCWin32RawClipboardItem* p_parent, MCStringRef p_type, MCDataRef p_bytes);
	~MCWin32RawClipboardItemRep();
};


class MCWin32RawClipboardItem :
  public MCRawClipboardItem
{
public:
    
    // Inherited from MCRawClipboardItem
    virtual uindex_t GetRepresentationCount() const;
    virtual const MCWin32RawClipboardItemRep* FetchRepresentationAtIndex(uindex_t p_index) const;
    virtual bool AddRepresentation(MCStringRef p_type, MCDataRef p_bytes);
    virtual bool AddRepresentation(MCStringRef p_type, render_callback_t, void* p_context);
    
	// Returns the data object representing the clipboard data
	IDataObject* GetIDataObject() const;
    
    // Returns m_object cast to MCWin32DataObject if the data is non-external.
    // Will return NULL if m_object_is_external is true.
    MCWin32DataObject* GetDataObject();

private:
    
    // Parent clipboard
    MCWin32RawClipboardCommon* m_clipboard;
    
    // The COM object containing the clipboard data
	bool m_object_is_external;
    IDataObject* m_object;

	// Array caching the representations of this item
	mutable MCAutoArray<MCWin32RawClipboardItemRep*> m_reps;

	// Lifetime is managed entirely by the parent MCWin32RawClipboard
	friend class MCWin32RawClipboardCommon;
	friend class MCWin32RawClipboard;
	MCWin32RawClipboardItem(MCWin32RawClipboardCommon* p_parent, IDataObject* p_external_data);
	MCWin32RawClipboardItem(MCWin32RawClipboardCommon* p_parent);
	~MCWin32RawClipboardItem();

	// Ensures that the representations have been loaded if the data is from
	// an external source. Does nothing if the data is local.
	void GenerateRepresentations() const;
};


class MCWin32RawClipboardCommon :
  public MCRawClipboard
{
public:
    
    // Inherited from MCRawClipboard
    virtual uindex_t GetItemCount() const;
    virtual const MCWin32RawClipboardItem* GetItemAtIndex(uindex_t p_index) const;
	virtual MCWin32RawClipboardItem* GetItemAtIndex(uindex_t p_index);
    virtual void Clear();
    virtual bool IsOwned() const = 0;
    virtual bool IsExternalData() const;
    virtual MCWin32RawClipboardItem* CreateNewItem();
    virtual bool AddItem(MCRawClipboardItem* p_item);
    virtual bool PushUpdates() = 0;
    virtual bool PullUpdates() = 0;
    virtual bool FlushData() = 0;
    virtual uindex_t GetMaximumItemCount() const;
    virtual MCStringRef GetKnownTypeString(MCRawClipboardKnownType p_type) const;
    virtual MCDataRef EncodeFileListForTransfer(MCStringRef p_file_list) const;
	virtual MCStringRef DecodeTransferredFileList(MCDataRef p_data) const;
	virtual MCDataRef EncodeHTMLFragmentForTransfer(MCDataRef p_html) const;
	virtual MCDataRef DecodeTransferredHTML(MCDataRef p_html) const;
	virtual MCDataRef EncodeBMPForTransfer(MCDataRef p_bmp) const;
	virtual MCDataRef DecodeTransferredBMP(MCDataRef p_bmp) const;

	// Sets the clipboard as being dirty
	void SetDirty();

	// Replaces the item on the clipboard with the given IDataObject. This
	// also marks the clipboard as containing external data.
	bool SetToIDataObject(IDataObject* p_object);

	// Returns the atom for the given type string. If it hasn't been registered
	// yet, this function will do so and return it.
	static UINT CopyAtomForType(MCStringRef p_type);

	// Returns a string associated with the given clipboard format atom
	static MCStringRef CopyTypeForAtom(UINT p_atom);

protected:
    
	// The one (and only) item on the clipboard
	MCWin32RawClipboardItem* m_item;

	// Information about the data on the clipboard
	bool m_dirty;			// Data has been modified
	bool m_external_data;	// Data is from outside LiveCode

	// Table of known type -> clipboard format mappings
	struct format_mapping { UINT atom; const char* string; };
	static format_mapping s_formats[];

	// Constructor
	MCWin32RawClipboardCommon();

	// Destructor
	~MCWin32RawClipboardCommon();
};

class MCWin32RawClipboard :
  public MCWin32RawClipboardCommon
{
public:

	// Inherited from MCWin32RawClipboardCommon
	virtual bool IsOwned() const;
	virtual bool PushUpdates();
	virtual bool PullUpdates();
	virtual bool FlushData();
	virtual MCWin32RawClipboardItem* CreateNewItem();

	MCWin32RawClipboard();
};

class MCWin32RawClipboardNull :
  public MCWin32RawClipboardCommon
{
public:

	// Inherited from MCWin32RawClipboardCommon
	virtual bool IsOwned() const;
	virtual bool PushUpdates();
	virtual bool PullUpdates();
	virtual bool FlushData();

	MCWin32RawClipboardNull();
};


class MCWin32DataObject :
  public IDataObject,
  public MCMixinRefcounted<MCWin32DataObject>
{
public:

	// Methods belonging to the IUnknown interface
	STDMETHODIMP_(ULONG) AddRef();		// Forwards to MCAutoRefcounted::Retain
	STDMETHODIMP_(ULONG) Release();		// Note: hides MCAutoRefcounted::Release (but forwards to it)
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);

	// Methods belonging to the IDataObject interface
	STDMETHODIMP DAdvise(FORMATETC* pformatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection);
	STDMETHODIMP DUnadvise(DWORD dwConnection);
	STDMETHODIMP EnumDAdvise(IEnumSTATDATA** ppenumAdvise);
	STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc);
	STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* pformatetcIn, FORMATETC* pformatetcOut);
	STDMETHODIMP GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium);
	STDMETHODIMP GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pmedium);
	STDMETHODIMP QueryGetData(FORMATETC* pformatetc);
	STDMETHODIMP SetData(FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease);

	// Adds a representation to this object
	bool AddRepresentation(MCStringRef p_type, MCDataRef p_bytes);

	// Constructor
	MCWin32DataObject();

private:

	// The list of representations of this object
	MCAutoStringRefArray m_types;
	MCAutoDataRefArray m_bytes;

	// Lifetime is managed through reference counting
	friend class MCMixinRefcounted<MCWin32DataObject>;
	~MCWin32DataObject();

	// Utility method that finds the DataRef for the given FORMATETC
	HRESULT FindDataForFormat(FORMATETC* pformatetcIn, MCDataRef& r_data);

	// The enumerator needs access to the internals of this class
	friend class MCWin32DataObjectFormatEnum;
};


class MCWin32DataObjectFormatEnum :
  public IEnumFORMATETC,
  public MCMixinRefcounted<MCWin32DataObjectFormatEnum>
{
public:

	// Methods belonging to the IUnknown interface
	STDMETHODIMP_(ULONG) AddRef();		// Forwards to MCAutoRefcounted::Retain
	STDMETHODIMP_(ULONG) Release();		// Note: hides MCAutoRefcounted::Release (but forwards to it)
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);

	// Methods belonging to the IEnumFORMATETC interface
	STDMETHODIMP Clone(IEnumFORMATETC **ppenum);
	STDMETHODIMP Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched);
	STDMETHODIMP Reset();
	STDMETHODIMP Skip(ULONG celt);

	// Constructor
	MCWin32DataObjectFormatEnum(MCWin32DataObject* p_object, uindex_t m_index = 0);

private:

	// The data object being wrapped and the current offset within that object
	MCWin32DataObject* m_object;
	uindex_t m_index;

	// Destructor
	friend class MCMixinRefcounted<MCWin32DataObjectFormatEnum>;
	~MCWin32DataObjectFormatEnum();
};


#endif  /* ifndef W32_CLIPBOARD_H */
