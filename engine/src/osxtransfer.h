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

#ifndef __OSXTRANSFER__
#define __OSXTRANSFER__

///////////////////////////////////////////////////////////////////////////////
//
//  Callback:
//    MCMacOSXConversionCallback
//
//  Type:
//    Private platform-specific callback type
//
//  Description:
//    The MCMacOSXConversionCallback type represents a procedure that converts
//    a the data stored in an MCSharedString object into a different type.
//
//    This callback is used to place converted data either after a drop
//    operation, or a request for promised scrap.
//
typedef MCSharedString* (*MCMacOSXConversionCallback)(MCSharedString *p_input);

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    MCMacOSXTransferData
//
//  Type:
//    Private platform-specific utility class
//
//  Description:
//    The MCMacOSXTransferData class is used to hole data that might need to
//    be converted to a given ScrapFlavorType when published via either the
//    ScrapManager's promise keeper system, or via the DragManager's standard
//    data-transfer mechanism.
//
//    It essentially an array of data-types with associated converters.
// 
//    Note that this class does not actually publish the data to either the
//    Drag or Scrap managers, it is just to be used as the context pointer in
//    the callbacks that these APIs provide for data-fetching.
//
class MCMacOSXTransferData
{
public:
	// Construct an empty data object
	MCMacOSXTransferData(void);

	// Destroy the object, releasing any resources it references
	~MCMacOSXTransferData(void);

	// Publish the given flavor type.
	//
	// If p_callback is not NULL then p_data will be converted upon request
	// using the callback.
	bool Publish(ScrapFlavorType p_type, MCSharedString* p_data, MCMacOSXConversionCallback p_callback);

	// Publish the given transfer type
	//
	bool Publish(MCTransferType p_type, MCSharedString *p_source_data);

	// Publish the contents of the give pasteboard
	//
	bool Publish(MCPasteboard *p_pasteboard);

	// Fetch the data for the given flavor type, this will convert the data
	// as appropriate.
	//
	MCSharedString *Subscribe(ScrapFlavorType p_type);

	// Iterate over the list of Flavors and perform the given callback for
	// each one.
	//
	bool ForEachFlavor(bool (*p_callback)(MCMacOSXTransferData *p_data, ScrapFlavorType p_type, void *p_context), void *p_context);

private:
	struct Entry
	{
		ScrapFlavorType type;
		MCSharedString *data;
		MCMacOSXConversionCallback converter;
	};

	Entry *m_entries;
	uint4 m_entry_count;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    MCMacOSXPasteboard
//
//  Type:
//    Private platform-specific utility abstract class
//
//  Description:
//    The MCMacOSXPasteboard class is a common base-class to the Drag and Scrap
//    Pasteboards. It provides a common implementation of the pasteboard
//    methods, deferring specific implementation to a set of virtual methods.
//
//  Known Issues:
//    <none>
//
class MCMacOSXPasteboard: public MCPasteboard
{
public:
	void Retain(void);
	void Release(void);

	bool Query(MCTransferType*& r_types, unsigned int& r_type_count);
	bool Fetch(MCTransferType p_type, MCSharedString*& r_data);

protected:
	MCMacOSXPasteboard(void);
	virtual ~MCMacOSXPasteboard(void);

	virtual bool QueryFlavors(ScrapFlavorType*& r_types, uint4& r_type_count) = 0;
	virtual bool FetchFlavor(ScrapFlavorType p_type, MCSharedString*& r_data) = 0;

	void Resolve(void);

private:
	struct Entry
	{
		MCTransferType type;
		ScrapFlavorType flavor;
		MCSharedString *data;
	};

	bool AddEntry(MCTransferType p_type, ScrapFlavorType p_flavor);

	uint4 m_references;

	Entry *m_entries;
	uint4 m_entry_count;

	MCTransferType *m_types;

	bool m_valid;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    MCMacOSXScrapPasteboard
//
//  Type:
//    Private platform-specific utility class
//
//  Description:
//    The MCMacOSXScrapPasteboard class is an implementation of the
//    MCPasteboard interface which wraps the client-side of the
//    scrap manager API.
//
//    It uses the common implementation provided by MCMacOSXPasteboard.
//
//  Known Issues:
//    <none>
//

class MCMacOSXScrapPasteboard: public MCMacOSXPasteboard
{
public:
	MCMacOSXScrapPasteboard(ScrapRef p_scrap);

protected:
	bool QueryFlavors(ScrapFlavorType*& r_types, uint4& r_type_count);
	bool FetchFlavor(ScrapFlavorType p_type, MCSharedString*& r_data);

private:
	ScrapRef m_scrap;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Class:
//    MCMacOSXDragPasteboard
//
//  Type:
//    Private platform-specific utility class
//
//  Description:
//    The MCMacOSXDragPasteboard class is an implementation of the
//    MCPasteboard interface which wraps the client-side of the
//    drag manager API.
//
//  Known Issues:
//    <none>
//
class MCMacOSXDragPasteboard: public MCMacOSXPasteboard
{
public:
	MCMacOSXDragPasteboard(DragRef p_drag);

protected:
	bool QueryFlavors(ScrapFlavorType*& r_types, uint4& r_type_count);
	bool FetchFlavor(ScrapFlavorType p_type, MCSharedString*& r_data);
	
private:
	DragRef m_drag;
};

MCSharedString *MCConvertTextToMacPlain(MCSharedString *p_data);
MCSharedString *MCConvertUnicodeToMacUnicode(MCSharedString *p_data);
MCSharedString *MCConvertStyledTextToMacPlain(MCSharedString *p_data);
MCSharedString *MCConvertStyledTextToMacUnicode(MCSharedString *p_data);

MCSharedString *MCConvertStyledTextToMacStyled(MCSharedString *p_data);
MCSharedString *MCConvertMacStyledToStyledText(MCSharedString *p_text_data, MCSharedString *p_style_data);

MCSharedString *MCConvertStyledTextToMacUnicodeStyled(MCSharedString *p_data);
MCSharedString *MCConvertMacUnicodeStyledToStyledText(MCSharedString *p_text_data, MCSharedString *p_style_data, bool p_is_external);

MCSharedString *MCConvertMacPictureToImage(MCSharedString *p_data);
MCSharedString *MCConvertImageToMacPicture(MCSharedString* p_data);

MCSharedString *MCConvertMacTIFFToImage(MCSharedString *p_data);

MCSharedString *MCConvertFilesToMacHFS(MCSharedString *p_data);
MCSharedString *MCConvertMacHFSToFiles(MCSharedString *p_data);
MCSharedString *MCConvertMacHTMLToStyledText(MCSharedString *p_data);

#endif
