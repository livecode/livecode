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

#ifndef X_TRANSFER_H
#define X_TRANSFER_H


#define REV_MIME_STR "application/x-revolution"
#define REV_TEXT_STR "text/x-revolution-styled"

extern const char *RevTypeToString(MCTransferType p_type); 

extern guint32 LastEventTime;
extern guint32 LastPositionTime;


////////////////////////////////////////////////////////////////////////////////
//
// Class MCMIMEtype:
// 		Encapsulates a MIME type. Stored as an Atom you can use this class
//		to convert between an Atom, a String, the LiveCode transfer type
// 		and the Rev transfer type string.

class MCMIMEtype
{

public:
    
    MCMIMEtype(GdkDisplay * p_display, Atom p_atom);
    MCMIMEtype(GdkDisplay * p_display, const char* p_atom);
	
    ~MCMIMEtype();
	
	MCTransferType 	asRev()         ;
	char * 			asString()      ;
	const char *	asRevString() 	{ return RevTypeToString(asRev()); }
	Atom			asAtom()        { return m_atom; }
		
private:
		
    GdkAtom m_atom;
    char* m_atom_name;
    GdkDisplay* m_display;
};


class MCGdkTransferStore
{
public:
    
    MCGdkTransferStore(GdkDisplay *p_display);
    ~MCGdkTransferStore();
    
    // Adding data and types to the store
    void addType(MCMIMEtype *p_mime, MCTransferType p_type, MCDataRef p_data);
    void addRevType(MCTransferType p_type, MCDataRef p_data);
    void addAtom(GdkAtom p_atom);
    void cleartypes();
    
    // Gets the types from an external selection
    void GetExternalTypes(GdkAtom p_selection, GdkWindow *p_source);
    
    // Interface with MCGdkPasteboard
    bool Query(MCTransferType* &r_types, size_t &r_type_count);
    bool Fetch(MCTransferType p_type, MCDataRef &r_data, GdkAtom p_atom, GdkWindow *p_source, GdkWindow *p_target, guint32 p_event_time);
    bool Fetch(MCMIMEtype* p_mime_type, MCDataRef &r_data, GdkAtom p_atom, GdkWindow *p_source, GdkWindow *p_target, guint32 p_event_time);
    
    // Sets the list of supported selection targets for a window
    void SetTypesOnWindow(GdkWindow* p_window);
    
    // Interface with clipboard
    GdkAtom* QueryAtoms(size_t &r_count);
    
    // Returns a list of types suitable for a GDK drag-and-drop operation
    GList* ListTargets();
    
    GdkDragContext* CreateDragContext(GdkWindow*);
    
private:
    
    // Querying the MIME types we hold
    char* asString(uint32_t p_index);
    GdkAtom asAtom(uint32_t p_index);
    
    // Methods to search the XTransfer_lookup_table
    int32_t find_entry_with_rev_type(MCTransferType);
    int32_t find_table_entry_with_rev_type(MCTransferType);
    int32_t find_table_entry_with_MIME_type(MCMIMEtype*);
    int32_t find_table_entry_with_full_types(MCTransferType, MCMIMEtype*);
    
    // Conversion of the data between different types
    bool Convert_MIME_to_REV(MCDataRef p_input, MCMIMEtype* p_mime, MCDataRef &r_output);
    bool Convert_REV_to_MIME(MCDataRef p_input, MCTransferType p_type, MCMIMEtype* p_mime, MCDataRef &r_output);
    MCMIMEtype* rev_to_MIME_stored(MCTransferType p_type);
    
    // Interface with the GDK selection mechanism
    //bool WaitForEventCompletion(GdkEvent *p_event);
    //bool GetSelection(GdkWindow *window, GdkAtom property, MCDataRef &r_data);
    bool GetExternalData(MCTransferType p_type, GdkAtom p_atom, GdkWindow *p_source, GdkWindow *p_target, MCDataRef &r_data, guint32 p_event_time);
    
    int32_t find_type(MCMIMEtype* p_type);
    
    struct Entry
    {
        MCTransferType m_type;  // type
        MCMIMEtype* m_mime;     // format
        MCDataRef m_data;       // data
    };
    
    // Type table
    Entry *m_entries;
    uint32_t m_entry_count;
    MCTransferType *m_types_list;
    uint32_t m_types_count;
    GList *m_targets;
    
    GdkDisplay *m_display;
    
    //bool m_internal_dnd;
};

class MCXTransferStore 
{
public:


	
private:	
		
	// Querying the MIME types we hold
		char * AtomAsString ( Atom p_atom ) { return gdk_atom_name ( p_atom ) ; } ;
																
	// Methods to search the XTransfer_lookup_table

	// Conversion of the DATA between different types

	// Interface with the XSelection
		bool 			WaitForEventCompletion(GdkEvent *p_xevent);
		char *			GetSelection ( Window w, Atom property, uint4& p_count ) ; 
		bool GetExternalData ( MCTransferType p_type, Atom p_public_atom, Atom p_private_atom, Window p_source_window, Window p_target_window, guint32 p_lock_time, MCDataRef& r_output)  ;
	
} ;


typedef bool (*MCConverterToRev)(MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output ) ;
typedef bool (*MCConverterToMIME)(MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output ) ;

////////////////////////////////////////////////////////////////////////////////
//
// Conversion routines in our table

bool ConvertFile_rev_to_MIME ( MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output ) ;
bool ConvertStyled_rev_to_HTML ( MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output ) ;
bool ConvertStyled_rev_to_RTF ( MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output ) ;
bool ConvertStyled_rev_to_TEXT ( MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output ) ;

bool ConvertTextToUnicode ( MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output ) ;
bool ConvertStyledToUnicode ( MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output ) ;

bool ConvertFile_MIME_to_rev ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output )  ;
bool ConvertStyled_RTF_to_rev ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output )  ;
bool ConvertStyled_HTML_to_rev ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output )  ;
bool ConvertStyled_Text_to_rev ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output ) ;

bool ConvertUnicodeToText ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output ) ;
bool ConvertUnicodeToStyled ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output ) ;
	

#define XDND_TYPE_TABLE_SIZE	14
typedef struct 
{
	const char * 			mime_type ;
	MCTransferType 			rev_type ;
	uint4 					priority ;
	MCConverterToRev		f_converter_to_rev;
	MCConverterToMIME		f_converter_to_mime ;
}
XdndMimeTable ;

#endif

