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


// XDND Utility macros

#define XDND_VERSION 3

#define REV_MIME_STR "application/x-revolution"
#define REV_TEXT_STR "text/x-revolution-styled"

#undef DEBUG_DND
//#define DEBUG_DND

/* XdndEnter */
#define XDND_THREE 3
#define XDND_ENTER_SOURCE_WIN(e)	((e)->xclient.data.l[0])
#define XDND_ENTER_THREE_TYPES(e)	(((e)->xclient.data.l[1] & 0x1UL) == 0)
#define XDND_ENTER_THREE_TYPES_SET(e,b)	(e)->xclient.data.l[1] = ((e)->xclient.data.l[1] & ~0x1UL) | (((b) == 0) ? 0 : 0x1UL)
#define XDND_ENTER_VERSION(e)		((e)->xclient.data.l[1] >> 24)
#define XDND_ENTER_VERSION_SET(e,v)	(e)->xclient.data.l[1] = ((e)->xclient.data.l[1] & ~(0xFF << 24)) | ((v) << 24)
#define XDND_ENTER_TYPE(e,i)		((e)->xclient.data.l[2 + i])	/* i => (0, 1, 2) */

/* XdndPosition */
#define XDND_POSITION_SOURCE_WIN(e)	((e)->xclient.data.l[0])
#define XDND_POSITION_ROOT_X(e)		((e)->xclient.data.l[2] >> 16)
#define XDND_POSITION_ROOT_Y(e)		((e)->xclient.data.l[2] & 0xFFFFUL)
#define XDND_POSITION_ROOT_SET(e,x,y)	(e)->xclient.data.l[2]  = ((x) << 16) | ((y) & 0xFFFFUL)
#define XDND_POSITION_TIME(e)		((e)->xclient.data.l[3])
#define XDND_POSITION_ACTION(e)		((e)->xclient.data.l[4])

/* XdndStatus */
#define XDND_STATUS_TARGET_WIN(e)	((e)->xclient.data.l[0])
#define XDND_STATUS_WILL_ACCEPT(e)	((e)->xclient.data.l[1] & 0x1L)
#define XDND_STATUS_WILL_ACCEPT_SET(e,b) (e)->xclient.data.l[1] = ((e)->xclient.data.l[1] & ~0x1UL) | (((b) == 0) ? 0 : 0x1UL)
#define XDND_STATUS_WANT_POSITION(e)	((e)->xclient.data.l[1] & 0x2UL)
#define XDND_STATUS_WANT_POSITION_SET(e,b) (e)->xclient.data.l[1] = ((e)->xclient.data.l[1] & ~0x2UL) | (((b) == 0) ? 0 : 0x2UL)
#define XDND_STATUS_RECT_X(e)		((e)->xclient.data.l[2] >> 16)
#define XDND_STATUS_RECT_Y(e)		((e)->xclient.data.l[2] & 0xFFFFL)
#define XDND_STATUS_RECT_WIDTH(e)	((e)->xclient.data.l[3] >> 16)
#define XDND_STATUS_RECT_HEIGHT(e)	((e)->xclient.data.l[3] & 0xFFFFL)
#define XDND_STATUS_RECT_SET(e,x,y,w,h)	{(e)->xclient.data.l[2] = ((x) << 16) | ((y) & 0xFFFFUL); (e)->xclient.data.l[3] = ((w) << 16) | ((h) & 0xFFFFUL); }
#define XDND_STATUS_ACTION(e)		((e)->xclient.data.l[4])

/* XdndLeave */
#define XDND_LEAVE_SOURCE_WIN(e)	((e)->xclient.data.l[0])

/* XdndDrop */
#define XDND_DROP_SOURCE_WIN(e)		((e)->xclient.data.l[0])
#define XDND_DROP_TIME(e)		((e)->xclient.data.l[2])

/* XdndFinished */
#define XDND_FINISHED_TARGET_WIN(e)	((e)->xclient.data.l[0])

extern const char *RevTypeToString(MCTransferType p_type) ; 


extern Atom XdndAware;
extern Atom XdndSelection;
extern Atom XdndEnter;
extern Atom XdndLeave;
extern Atom XdndPosition;
extern Atom XdndDrop;
extern Atom XdndFinished;
extern Atom XdndStatus;
extern Atom XdndActionCopy;
extern Atom XdndActionMove;
extern Atom XdndActionLink;
extern Atom XdndActionAsk;
extern Atom XdndActionPrivate;
extern Atom XdndTypeList;
extern Atom XdndActionList;
extern Atom XdndActionDescription;

extern Atom XdndMyAtom ;
extern Atom XdndRevolutionWindow ;

extern Atom XA_TARGETS ;


extern Atom WMname ;
//extern Atom text ;

extern Time LastEventTime ;
extern Time LastPositionTime ;



////////////////////////////////////////////////////////////////////////////////
//
// Class MCMIMEtype:
// 		Encapsulates a MIME type. Stored as an Atom you can use this class
//		to convert between an Atom, a String, the LiveCode transfer type
// 		and the Rev transfer type string.

class MCMIMEtype
{

public:
		MCMIMEtype ( Display * p_display, Atom p_atom );
		MCMIMEtype ( Display * p_display, char * p_atom ) ;
	
		~MCMIMEtype (void);
	
		MCTransferType 	asRev (void) ;
		char * 			asString (void) ;
		const char *	asRevString(void) 	{ return RevTypeToString(asRev());};
		Atom			asAtom (void)   	{ return m_atom ; } ;
		
private:
		Atom m_atom ;
		char * m_atom_name ;
		Display * m_display ;
	
} ;





class MCXTransferStore 
{
public:
		
		MCXTransferStore( Display *p_display ) ;
		~MCXTransferStore();
	
	// Adding data and types to the store
		void addType ( MCMIMEtype *p_mime, MCTransferType p_type, MCSharedString * p_data  ) ;
		void addAtom ( Atom p_atom ) ;
		void addRevType ( MCTransferType p_type, MCSharedString * p_data ) ;
		void cleartypes ( void ) ;
		
	// Applying the types to windows or messages
		void	apply_to_window ( Window w ) ;
		void	apply_to_message ( XEvent *xevent ) ;
	
	
	
	// Interface with MCXPasteboard
		bool 	Query(MCTransferType*& r_types, unsigned int& r_type_count) ;
		bool 	Fetch(MCTransferType p_type, MCSharedString*& r_data, Atom p_public_atom, Atom p_private_atom, Window p_source_window, Window p_target_window, Time p_lock_time) ;
		bool 	Fetch(MCMIMEtype* p_mime_type, MCSharedString*& r_data, Atom p_public_atom, Atom p_private_atom, Window p_source_window, Window p_target_window, Time p_lock_time) ;
	
	// Interface with clipboard 
		Atom 	* QueryAtoms (uint4 & r_count);

	
	// Misc stuff

		void	dumpList (char * p_msg ) ;
		uint4  	getCount (void ) { return m_entry_count ; } ;
		void	internal ( bool i ) { m_internal_dnd = i ; } ;

	
		// Convertsion between MIME and LiveCode type descriptors
		MCMIMEtype * 	rev_to_MIME_stored ( MCTransferType p_type ) ;

	
	
	
		void 			GetExternalTypes ( Atom p_public_atom, Atom p_private_atom, Window w ) ;
		void 			dumpClipboard(const char * p_msg) ;

	
private:	
		
	// Querying the MIME types we hold
		char * asString( uint4 p_idx ) ;
		Atom   asAtom  ( uint4 p_idx ) ;
		char * AtomAsString ( Atom p_atom ) { return XGetAtomName ( m_display, p_atom ) ; } ;
																
	
	
	// Methods to search the XTransfer_lookup_table
		int4  find_entry_with_rev_type ( MCTransferType p_type ) ;
		int4  find_table_entry_with_rev_type ( MCTransferType p_type ) ;
		int4  find_table_entry_with_MIME_type ( MCMIMEtype * p_MIME ) ;
		int4  find_table_entry_with_full_types ( MCTransferType p_type, MCMIMEtype * p_MIME ) ;
	
	
	// Conversion of the DATA between different types
	    MCSharedString * Convert_MIME_to_REV ( MCSharedString * p_in, MCMIMEtype * p_mime ) ;
		MCSharedString * Convert_REV_to_MIME ( MCSharedString * p_in, MCTransferType p_type,  MCMIMEtype * p_mime ) ;
	
	// Interface with the XSelection
		bool 			WaitForEventCompletion(XEvent &p_xevent);
		char *			GetSelection ( Window w, Atom property, uint4& p_count ) ; 
		MCSharedString * GetExternalData ( MCTransferType p_type, Atom p_public_atom, Atom p_private_atom, Window p_source_window, Window p_target_window, Time p_lock_time)  ;

	
		struct Entry
		{
			MCTransferType 	type;
			MCMIMEtype		*format ;
			MCSharedString 	*data;
		};

		
		Entry * m_entries ;
		uint4 m_entry_count ;
		MCTransferType * m_types_list ;
		uint4 m_types_count ;
		
	
		Display *m_display ;
	
		bool m_internal_dnd ; 
	
		int4 find_type ( MCMIMEtype * p_type ) ;
	
} ;




typedef MCSharedString * (*MCConverterToRev)(MCSharedString *p_in, MCMIMEtype * p_MIME ) ;
typedef MCSharedString * (*MCConverterToMIME)(MCSharedString *p_in, MCTransferType p_type ) ;

////////////////////////////////////////////////////////////////////////////////
//
// Conversion routines in our table

MCSharedString * ConvertFile_rev_to_MIME ( MCSharedString * p_in, MCTransferType p_type ) ;
MCSharedString * ConvertStyled_rev_to_HTML ( MCSharedString * p_in, MCTransferType p_type ) ;
MCSharedString * ConvertStyled_rev_to_RTF ( MCSharedString * p_in, MCTransferType p_type ) ;
MCSharedString * ConvertStyled_rev_to_TEXT ( MCSharedString * p_in, MCTransferType p_type ) ;

MCSharedString * ConvertTextToUnicode ( MCSharedString * p_in, MCTransferType p_type ) ;
MCSharedString * ConvertStyledToUnicode ( MCSharedString * p_in, MCTransferType p_type ) ;

MCSharedString * ConvertFile_MIME_to_rev ( MCSharedString * p_in, MCMIMEtype * p_MIME )  ;
MCSharedString * ConvertStyled_RTF_to_rev ( MCSharedString * p_in, MCMIMEtype * p_MIME )  ;
MCSharedString * ConvertStyled_HTML_to_rev ( MCSharedString * p_in, MCMIMEtype * p_MIME )  ;
MCSharedString * ConvertStyled_Text_to_rev ( MCSharedString * p_in, MCMIMEtype * p_MIME ) ;

MCSharedString * ConvertUnicodeToText ( MCSharedString * p_in, MCMIMEtype * p_MIME ) ;
MCSharedString * ConvertUnicodeToStyled ( MCSharedString * p_in, MCMIMEtype * p_MIME ) ;


	

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

