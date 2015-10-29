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

#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h" 
#include "objdefs.h"
#include "parsedef.h" 

#include "util.h"

#include "transfer.h"
//#include "execpt.h" 
#include "exec.h" 
#include "dispatch.h"
#include "image.h"
#include "globals.h"

#include "lnxdc.h"
#include "lnxtransfer.h"


guint32 LastEventTime ;
guint32 LastPositionTime ;

static XdndMimeTable XTransfer_lookup_table[XDND_TYPE_TABLE_SIZE] = 
	{ 	
		{ "text/uri-list", 	TRANSFER_TYPE_FILES, 		5, ConvertFile_MIME_to_rev, ConvertFile_rev_to_MIME } ,
		{ "image/gif",	 	TRANSFER_TYPE_IMAGE, 		5, NULL, NULL },
		{ "image/jpeg",	 	TRANSFER_TYPE_IMAGE, 		5, NULL, NULL },
		{ "image/png",	 	TRANSFER_TYPE_IMAGE, 		5, NULL, NULL },
		{ REV_MIME_STR, 	TRANSFER_TYPE_OBJECTS, 		5, NULL, NULL },
		{ REV_TEXT_STR,		TRANSFER_TYPE_STYLED_TEXT,	4, NULL, NULL },
		{ "text/richtext", 	TRANSFER_TYPE_STYLED_TEXT,	2, ConvertStyled_RTF_to_rev, ConvertStyled_rev_to_RTF },
		{ "STRING",		 	TRANSFER_TYPE_TEXT,	 		3, NULL, NULL }, 
		{ "COMPOUND_TEXT", 	TRANSFER_TYPE_TEXT,	 		3, NULL, NULL }, 
		{ "UTF8_STRING", 	TRANSFER_TYPE_UNICODE_TEXT,	3, ConvertUnicodeToText, ConvertTextToUnicode },
		{ "STRING",		 	TRANSFER_TYPE_STYLED_TEXT, 	2, ConvertStyled_Text_to_rev, ConvertStyled_rev_to_TEXT  }, 	
		{ "COMPOUND_TEXT",	TRANSFER_TYPE_STYLED_TEXT, 	2, ConvertStyled_Text_to_rev, ConvertStyled_rev_to_TEXT  }, 	
		{ "UTF8_STRING",	TRANSFER_TYPE_STYLED_TEXT, 	2, ConvertUnicodeToStyled, ConvertStyledToUnicode }, 	
		{ "text/plain", 	TRANSFER_TYPE_TEXT, 		1, NULL, NULL }
	} ; 

////////////////////////////////////////////////////////////////////////////////
//
// MCMIMEtype implementation


MCMIMEtype::MCMIMEtype(GdkDisplay * p_display, GdkAtom p_atom)
{
	m_display = p_display;
	m_atom = p_atom;
	m_atom_name = NULL;
}


MCMIMEtype::MCMIMEtype(GdkDisplay * p_display, const char * p_atom)
{
	m_display = p_display;
	m_atom = gdk_atom_intern(p_atom, false);
	m_atom_name = NULL;
}

MCMIMEtype::~MCMIMEtype(void)
{
	if (m_atom_name != NULL)
		g_free(m_atom_name);
}


char * MCMIMEtype::asString(void)
{
	if (m_atom_name == NULL)
		m_atom_name = gdk_atom_name(m_atom);
	return m_atom_name;
}

MCTransferType MCMIMEtype::asRev ( void ) 
{
	for (int a = 0; a < XDND_TYPE_TABLE_SIZE; a++)
		if (gdk_atom_intern(XTransfer_lookup_table[a] . mime_type, false) == m_atom)
			return XTransfer_lookup_table[a] . rev_type;
	return TRANSFER_TYPE_NULL;
}

 

/*=========================================================================================

					T R A N S F E R S T O R E     O B J E C T

=========================================================================================*/

MCGdkTransferStore::MCGdkTransferStore(GdkDisplay *p_display)
: m_entries(NULL), m_entry_count(0), m_types_list(NULL), m_types_count(0),
    m_targets(NULL), m_display(p_display)
{
    ;
}

MCGdkTransferStore::~MCGdkTransferStore()
{
    cleartypes();
}

void MCGdkTransferStore::cleartypes()
{
    if (m_entries != NULL)
    {
        for (int32_t i = 0; i < m_entry_count; i++)
        {
            if (m_entries[i].m_data != nil)
                MCValueRelease(m_entries[i].m_data);
            delete m_entries[i].m_mime;
        }
    }
    
    delete[] m_entries;
    m_entries = NULL;
    m_entry_count = 0;
    
    delete[] m_types_list;
    m_types_list = NULL;
    m_types_count = 0;
    
    if (m_targets != NULL)
        g_list_free(m_targets);
    m_targets = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
// Functions to add data into the TransferStore

void MCGdkTransferStore::addType(MCMIMEtype* p_mime, MCTransferType p_type, MCDataRef p_data)
{
    if (p_mime == NULL)
        return;
    
    // Use the transfer type if specified; otherwise work it out based on MIME
    MCTransferType t_type;
    t_type = (p_type == TRANSFER_TYPE_NULL) ? p_mime->asRev() : p_type;
    
    if (t_type != TRANSFER_TYPE_NULL)
    {
        m_entry_count++;
        
        m_entries = (Entry*)realloc(m_entries, sizeof(Entry)*m_entry_count);
        
        m_entries[m_entry_count-1].m_mime = p_mime;
        m_entries[m_entry_count-1].m_type = t_type;
        m_entries[m_entry_count-1].m_data = (p_data != nil) ? MCValueRetain(p_data) : nil;
    }
}

void MCGdkTransferStore::addRevType(MCTransferType p_type, MCDataRef p_data)
{
    switch (p_type)
    {
        case TRANSFER_TYPE_TEXT:
            addType(new MCMIMEtype(m_display, "text/plain"), p_type, p_data);
            addType(new MCMIMEtype(m_display, "STRING"), p_type, p_data);
            addType(new MCMIMEtype(m_display, "COMPOUND_TEXT"), p_type, p_data);
            break;
            
        case TRANSFER_TYPE_STYLED_TEXT:
            addType(new MCMIMEtype(m_display, "text/richtext"), p_type, p_data);
            addType(new MCMIMEtype(m_display, "STRING"), p_type, p_data);
            addType(new MCMIMEtype(m_display, "COMPOUND_TEXT"), p_type, p_data);
            addType(new MCMIMEtype(m_display, "UTF8_STRING"), p_type, p_data);
            break;
            
        case TRANSFER_TYPE_IMAGE:
            if (MCImageDataIsPNG(p_data))
                addType(new MCMIMEtype(m_display, "image/png"), p_type, p_data);
            if (MCImageDataIsJPEG(p_data))
                addType(new MCMIMEtype(m_display, "image/jpeg"), p_type, p_data);
            if (MCImageDataIsGIF(p_data))
                addType(new MCMIMEtype(m_display, "image/gif"), p_type, p_data);
            break;
            
        case TRANSFER_TYPE_FILES:
            addType(new MCMIMEtype(m_display, "text/uri-list"), p_type, p_data);
            break;
            
        case TRANSFER_TYPE_OBJECTS:
            addType(new MCMIMEtype(m_display, REV_MIME_STR), p_type, p_data);
            break;
    }
}

void MCGdkTransferStore::addAtom(GdkAtom p_atom)
{
    MCMIMEtype* t_mime = new MCMIMEtype(m_display, p_atom);
    addType(t_mime, TRANSFER_TYPE_NULL, NULL);
}

///////////////////////////////////////////////////////////////////////////////////////////
//
// X11 Specific functions

/*
bool MCGdkTransferStore::GetSelection(GdkWindow *w, GdkAtom property, MCDataRef &r_data)
{
    // TODO: could we use the gdk selection getter instead?
    
    MCAutoDataRef t_data;
    if (!MCDataCreateMutable(0, &t_data))
        return false;
    
    // Count of bytes read
    uindex_t t_total_bytes = 0;
    
    unsigned long count, remaining;
    
    do
    {
        unsigned char *s = NULL;
        x11::Atom actual;
        int format;
        
        // The GDK equivalent of XGetWindowProperty (gdk_property_get) is to be
        // avoided, according to the GDK documentation, in favour of just using
        // XGetWindowProperty directly instead.
        int t_result;
        t_result = x11::XGetWindowProperty(x11::gdk_x11_display_get_xdisplay(m_display),
                                           x11::gdk_x11_drawable_get_xid(w),
                                           x11::gdk_x11_atom_to_xatom_for_display(m_display, property),
                                           t_total_bytes / 4,
                                           65536,
                                           false,
                                           AnyPropertyType,
                                           &actual,
                                           &format,
                                           &count,
                                           &remaining,
                                           &s);
        
        // Given the format and count, how many bytes did we receive?
        uindex_t t_bytes = count * (format / 8);
        t_total_bytes += t_bytes;
        
        if (t_result != Success || !MCDataAppendBytes(*t_data, s, t_bytes))
        {
            x11::XFree(s);
            return false;
        }
        
        x11::XFree(s);
    }
    while (remaining > 0);
    
    return MCDataCopy(*t_data, r_data);
}
*/

static bool selection_notify(GdkEvent *t_event, void *)
{
    return (t_event->type == GDK_SELECTION_NOTIFY);
}

static bool WaitForEventCompletion()
{
    // Loop until a selection notify event is received
    MCScreenDC *dc = (MCScreenDC*)MCscreen;
    GdkEvent *t_notify;
    while (!dc->GetFilteredEvent(selection_notify, t_notify, NULL))
    {
        // TODO: timeout
    }
    
    return true;
}

void MCGdkTransferStore::GetExternalTypes(GdkAtom p_selection, GdkWindow *p_target)
{
    // Get the list of targets (types) supported by the external selection
    guchar *t_bytes;
    gint t_byte_len;
    GdkAtom t_property_type;
    gint t_property_format;
    gdk_selection_convert(p_target, p_selection, gdk_atom_intern_static_string("TARGETS"), LastEventTime);
    
    // Wait until the selection to be done
    if (!WaitForEventCompletion())
        return;
    
    t_byte_len = gdk_selection_property_get(p_target, &t_bytes, &t_property_type, &t_property_format);
    
    // The target list should be a list of 32-bit atoms
    cleartypes();
    if (t_property_type != GDK_SELECTION_TYPE_ATOM || t_property_format != 32)
        return;
    
    for (gint i = 0; i < t_byte_len/sizeof(GdkAtom); i++)
    {
        addAtom(((GdkAtom*)t_bytes)[i]);
    }
    
    g_free(t_bytes);
}

/*
void MCXTransferStore::GetExternalTypes ( Atom p_public_atom, Atom p_private_atom, Window w)
{
	gdk_selection_owner_set_for_display(m_display, w, p_private_atom, LastEventTime, TRUE);
    gdk_selection_convert(w, p_public_atom, gdk_atom_intern_static_string("TARGETS"), LastEventTime);
    
    XEvent xevent ;
	XSetSelectionOwner(m_display, p_private_atom, w, LastEventTime);
	XConvertSelection (m_display, p_public_atom, XA_TARGETS, p_private_atom, w, LastEventTime );
	if ( WaitForEventCompletion(xevent) )
	{
		Atom * t_data ;
		uint4 t_count ;
		
		cleartypes();
		t_data = (Atom*)GetSelection ( xevent.xselection.requestor, p_private_atom , t_count) ;		
		
#ifdef DEBUG_DND
		fprintf(stderr, "\t I have retrieved %d types....\n", t_count ) ;
#endif
		for ( uint4 a =0 ; a < t_count ; a++)
		{
#ifdef DEBUG_DND
			fprintf(stderr, "\t%s\n", AtomAsString((Atom)t_data[a]));
#endif
			addAtom( (Atom)t_data[a]);
		}
		
		free(t_data) ;
	}
	
#ifdef DEBUG_DND
	dumpList("GetExternalTypes() -- So, this is what I think is on the clipboard!");
#endif
	
}






bool MCXTransferStore::WaitForEventCompletion(XEvent &p_xevent)
{

	XEvent xevent ;
	bool xdnd_target_done = false ;
	
	guint32 t_loop_time = LastEventTime ;
	
	while (!xdnd_target_done )
	{

		XAllowEvents (m_display , SyncPointer, CurrentTime);
		XNextEvent( m_display , &xevent);
	
		
		if ( xevent.type == SelectionNotify ) 
		{
			xdnd_target_done = true ;
			p_xevent = xevent ;
			return true ;
		}
		
		
		
		
		if ( xevent.type == MotionNotify ) 
		{
			if (xevent.xmotion.time > ( t_loop_time + 1000) )
			  return false ;
		}
		
		if ( ( xevent.type == Expose ) || ( xevent.type == GraphicsExpose ) )
		{
			XPutBackEvent(m_display , &xevent);
			MCscreen -> expose();
		}
		
		
	}
}
*/

bool MCGdkTransferStore::GetExternalData(MCTransferType p_type, GdkAtom p_atom, GdkWindow *p_source, GdkWindow *p_target, MCDataRef &r_data, guint32 p_event_time)
{
    // Check that the selection atom is still the source
    // Check dummied out because GDK won't create a window object for foreign
    // windows, making this check pretty much useless.
    //if (gdk_selection_owner_get(p_atom) == p_source)
    {
        // Find the MIME type corresponding to the transfer type we desire
        MCMIMEtype *t_mime_type;
        int32_t t_index;
        t_index = find_entry_with_rev_type(p_type);
        if (t_index > -1)
        {
            t_mime_type = m_entries[t_index].m_mime;
            
            // Request the source to convert the data into this type
            gdk_selection_convert(p_target, p_atom, t_mime_type->asAtom(), p_event_time);
            
            // We need to wait for an event notifying us of the conversion
            if (!WaitForEventCompletion())
                return false;
            
            // Now the data has been converted, fetch it
            guchar *t_bytes;
            gint t_byte_len;
            GdkAtom t_property_type;
            gint t_property_format;
            t_byte_len = gdk_selection_property_get(p_target, &t_bytes, &t_property_type, &t_property_format);
            
            MCAutoDataRef t_data;
            bool t_success;
            t_success = MCDataCreateWithBytes(t_bytes, t_byte_len, &t_data);
            
            g_free(t_bytes);
            
            return t_success && Convert_MIME_to_REV(*t_data, t_mime_type, r_data);
        }
    }
    
    return false;
}

/*
bool MCXTransferStore::GetExternalData ( MCTransferType p_type, Atom p_public_atom, Atom p_private_atom, Window p_source_window, Window p_target_window, Time p_lock_time, MCDataRef& r_data)  
{
	XEvent xevent ;
	
	// Check that the Source application actually owns the XdndSelection.
	if ( XGetSelectionOwner ( m_display, p_public_atom ) == p_source_window ) 
	{

		// Take ownership of my Xdnd Atom
		XSetSelectionOwner (m_display, p_private_atom, p_target_window, p_lock_time);
		
		// Now try and convert XdndSelection to XdndMyAtom as type "text/plain"
		// NOTE: here we need to seach through out entries and find the MIME type of the first matching
		//       rev type.
		
		MCMIMEtype * p_mime_type ;
		int4   idx ;
		idx = find_entry_with_rev_type ( p_type );
		if ( idx > -1 )
		{
				p_mime_type = m_entries[idx] . format ;
			
			
			XConvertSelection (m_display, p_public_atom, p_mime_type -> asAtom(), p_private_atom , p_target_window, p_lock_time);
			
			// Now we want to wait until we get a SelectionNotify message or we time-out
			// waiting for it.
			if ( WaitForEventCompletion(xevent) )
			{

                char * t_data ;
                uint4 t_count ;
                t_data = GetSelection ( xevent.xselection.requestor, p_private_atom , t_count) ;
                MCAutoDataRef t_ret;
                if (MCDataCreateWithBytes((const char_t *)t_data, t_count, &t_ret )) // there was a ';' before here???
                    return Convert_MIME_to_REV ( *t_ret, p_mime_type, r_data );
                return false;
            }
		}
	
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Utility functions for searching tables etc.


char * MCXTransferStore::asString( uint4 p_idx ) 
{
	if ( ( m_entries != NULL ) && ( p_idx <= m_entry_count ) )
		return ( m_entries[p_idx] . format->asString() ) ;	 
	return NULL ;
}


Atom   MCXTransferStore::asAtom  ( uint4 p_idx ) 
{
	if  ( ( m_entries != NULL ) && ( p_idx <= m_entry_count ) ) 
		return m_entries[p_idx] . format->asAtom() ;
	return 0 ;
	
}
*/

int32_t MCGdkTransferStore::find_type(MCMIMEtype* p_type)
{
	for (uint32_t i = 0; i < m_entry_count; i++)
    {
        if (strcmp(p_type->asString(), m_entries[i].m_mime->asString()) == 0)
            return i;
    }
    return -1;
}
 
int32_t MCGdkTransferStore::find_entry_with_rev_type(MCTransferType p_type) 
{
	for (uint32_t i = 0; i < m_entry_count; i++)
    {
        if (m_entries[i].m_type == p_type)
            return i;
    }
    return -1;
}

int32_t MCGdkTransferStore::find_table_entry_with_rev_type(MCTransferType p_type) 
{
	for (uint32_t i = 0; i < XDND_TYPE_TABLE_SIZE; i++)
    {
        if (XTransfer_lookup_table[i].rev_type == p_type)
            return i;
    }
    return -1;
}

int32_t MCGdkTransferStore::find_table_entry_with_MIME_type(MCMIMEtype* p_MIME) 
{
	for (uint32_t i = 0; i < XDND_TYPE_TABLE_SIZE; i++)
    {
        if (strcmp(p_MIME->asString(), XTransfer_lookup_table[i].mime_type) == 0)
            return i;
    }
    return -1;
}

int32_t MCGdkTransferStore::find_table_entry_with_full_types(MCTransferType p_type, MCMIMEtype* p_MIME)
{
	for (uint32_t i = 0; i < XDND_TYPE_TABLE_SIZE; i++)
    {
        if (strcmp(p_MIME->asString(), XTransfer_lookup_table[i].mime_type) == 0
            && p_type == XTransfer_lookup_table[i].rev_type)
        {
            return i;
        }
    }

	return -1 ;
} 


///////////////////////////////////////////////////////////////////////////////
//
// Functions that convert between the REV and System type descriptors


MCMIMEtype * MCGdkTransferStore::rev_to_MIME_stored(MCTransferType p_type)
{
	int32_t idx;
	idx = find_entry_with_rev_type(p_type);
	if (idx > -1)
		return (m_entries[idx].m_mime);
	else 
		return NULL;
}



/////////////////////////////////////////////////////////////////////////////////
//
// Interface with the Clipboard


GdkAtom *MCGdkTransferStore::QueryAtoms(size_t &r_count)
{
    uint32_t t_top = 0;
    uint32_t t_index;
    uint32_t t_count = 0;
    
    if (m_entries != NULL)
    {
        // Find the highest-priority type
        for (uint32_t i = 0; i < m_entry_count; i++)
        {
            t_index = find_table_entry_with_full_types(m_entries[i].m_type, m_entries[i].m_mime);
            if (XTransfer_lookup_table[t_index].priority > t_top)
                t_top = XTransfer_lookup_table[t_index].priority;
        }
        
        MCAutoArray<GdkAtom> t_ret;
        for (uint32_t i = 0; i < m_entry_count; i++)
        {
            t_index = find_table_entry_with_full_types(m_entries[i].m_type, m_entries[i].m_mime);
            t_ret.Extend(++t_count);
            t_ret[t_count-1] = m_entries[i].m_mime->asAtom();
        }
        
        GdkAtom* t_ptr;
        uindex_t t_count;
        t_ret.Take(t_ptr, t_count);
        r_count = t_count;
        return t_ptr;
    }
    
    r_count = 0;
    return NULL;
}

/*
Atom * MCXTransferStore::QueryAtoms (uint4 & r_count)
{
	
#ifdef DEBUG_DND
	dumpList("QueryAtoms");
#endif
	
	Atom * t_ret = NULL;
	uint4 t_top = 0 ; 
	uint4 idx ;
	uint4 t_count = 0 ;
	
	if ( m_entries != NULL ) 
	{
		// First we need to find the highest priority type we hold. 
		for ( uint4 a = 0 ; a < m_entry_count; a++)
		{
			idx = find_table_entry_with_full_types ( m_entries[a] . type, m_entries[a] . format ) ;
			if ( XTransfer_lookup_table[idx] . priority > t_top )
				t_top = XTransfer_lookup_table[idx] . priority ;
		}
 
		t_ret = (Atom*)malloc(sizeof(Atom) * 3);
		t_ret[0] = XInternAtom(m_display,"TIMESTAMP", false);	// We MUST respond to TIMESTAMP requests
		t_ret[1] = XInternAtom(m_display,"TARGETS", false);		// We MUST respond to TARGETS requests. 
		t_ret[2] = XInternAtom(m_display,"SAVE_TARGETS", false);		// We MUST respond to TARGETS requests.
		
		t_count +=3 ;
		// Now will will return all data types we hold with that highest priority
		for ( uint4 a = 0 ; a < m_entry_count; a++)
		{
			idx = find_table_entry_with_full_types ( m_entries[a] . type, m_entries[a] . format ) ;
#ifdef DEBUG_DND
			fprintf(stderr, "TopPri = %d, IDX = %d, MIME = %s, REV = %s\n", t_top, idx, m_entries[a].format->asString(), m_entries[a].format->asRevString());
#endif 
			{
				t_count ++ ;
				t_ret = (Atom*)realloc(t_ret, (sizeof(Atom) * t_count ) ) ;
				t_ret[t_count - 1 ] = m_entries[a] . format->asAtom() ;
			}
		}

		r_count = t_count ;
	}
	return t_ret ;
}
*/
	

////////////////////////////////////////////////////////////////////////////////
//
// Interface with MCXPasteboard


bool in_list ( MCTransferType * p_list, uint4 p_count, MCTransferType p_type)
{
	for (uint4 a = 0 ; a < p_count ; a++)
		if ( p_list[a] == p_type)
			return true ;
	return false ;
}


bool should_include ( MCTransferType * p_list, uint4 p_count, MCTransferType p_type)
{
	
	if ( (( p_type == TRANSFER_TYPE_TEXT ) || ( p_type == TRANSFER_TYPE_UNICODE_TEXT )) && in_list(p_list, p_count, TRANSFER_TYPE_STYLED_TEXT ))
		return false ;
	
	if (( p_type == TRANSFER_TYPE_TEXT ) && in_list(p_list, p_count, TRANSFER_TYPE_UNICODE_TEXT ))
		return false ;
	
	return ( !in_list ( p_list, p_count, p_type));	
}


bool MCGdkTransferStore::Query(MCTransferType* &r_types, size_t &r_type_count)
{
    MCAutoArray<MCTransferType> t_list;
    uint32_t t_top = 0;
    uint32_t t_index;
    uint32_t t_count = 0;
    
    if (m_entries != NULL)
    {
        if (m_types_list == NULL)
        {
            // Find the highest-priority type
            for (uint32_t i = 0; i < m_entry_count; i++)
            {
                t_index = find_table_entry_with_full_types(m_entries[i].m_type, m_entries[i].m_mime);
                if (XTransfer_lookup_table[t_index].priority > t_top)
                    t_top = XTransfer_lookup_table[t_index].priority;
            }
            
            for (uint32_t i = 0; i < m_entry_count; i++)
            {
                t_index = find_table_entry_with_full_types(m_entries[i].m_type, m_entries[i].m_mime);
                if ((XTransfer_lookup_table[t_index].priority == t_top) && should_include(t_list.Ptr(), t_count, m_entries[i].m_type))
                {
                    t_list.Extend(++t_count);
                    t_list[t_count-1] = m_entries[i].m_type;
                }
            }
            
            t_list.Take(m_types_list, m_types_count);
        }
        
        r_type_count = m_types_count;
        r_types = m_types_list;
        return true;
    }
    
    return false;
}


/*bool MCXTransferStore::Query(MCTransferType*& r_types, unsigned int& r_type_count)
{

	
	MCTransferType * t_list = NULL;
	uint4 t_top = 0 ; 
	uint4 idx ;
	uint4 t_count = 0 ; 
	
	if ( m_entries != NULL ) 
	{
		if ( m_types_list == NULL)
		{
				 
			// First we need to find the highest priority type we hold.
			for ( uint4 a = 0 ; a < m_entry_count; a++)
			{
				idx = find_table_entry_with_full_types ( m_entries[a].type, m_entries[a] . format );
				if ( XTransfer_lookup_table[idx] . priority > t_top )
					t_top = XTransfer_lookup_table[idx] . priority ; 
			}

			// Now will will return all data types we hold with that highest priority
			for ( uint4 a = 0 ; a < m_entry_count; a++)
			{
				idx = find_table_entry_with_full_types ( m_entries[a].type, m_entries[a] . format );
				if (( XTransfer_lookup_table[idx] . priority == t_top ) && should_include( t_list, t_count, m_entries[a].type) )
				{
					t_count ++ ;
					if ( t_list == NULL )
						t_list = (MCTransferType*)malloc(sizeof(MCTransferType)) ;
					else 
						t_list = (MCTransferType*)realloc(t_list, sizeof(MCTransferType) * t_count  );
					
					t_list[t_count - 1 ] = m_entries[a] . type ;
				}
			}

			r_type_count = t_count ;
			m_types_count = t_count ;
			r_types = t_list ;
			m_types_list = t_list ;
		}
		else 
		{
			r_types = m_types_list ;
			r_type_count = m_types_count ;
		}

#ifdef DEBUG_DND
		fprintf(stderr, "\tDump\t\n----\n");
		for ( uint4 a = 0 ; a < r_type_count ; a ++)
			fprintf(stderr, "\t%s\n", RevTypeToString(m_types_list[a]));
#endif
		return true ;
	}
	return false ;
}
*/

bool MCGdkTransferStore::Fetch(MCMIMEtype *p_mime_type, MCDataRef& r_data, GdkAtom p_atom, GdkWindow *p_source, GdkWindow *p_target, guint32 p_event_time)
{
    if (m_entries != NULL)
    {
        int32_t t_index = find_type(p_mime_type);
        if (t_index > -1)
        {
            if (m_entries[t_index].m_data == NULL)
            {
                // We have no data for this type so fetch it from the source
                GetExternalData(m_entries[t_index].m_type, p_atom, p_source, p_target, r_data, p_event_time);
                if (r_data != NULL)
                    m_entries[t_index].m_data = MCValueRetain(r_data);
            }
            else
            {
                // If this is an internal copy operation, skip the conversion.
                // (We can tell that it is internal if we recognise the source
                // and destination windows as being our stacks).
                if (MCdispatcher->findstackd(p_source) && MCdispatcher->findstackd(p_target))
                    r_data = MCValueRetain(m_entries[t_index].m_data);
                else
                    Convert_REV_to_MIME(m_entries[t_index].m_data, m_entries[t_index].m_type, m_entries[t_index].m_mime, r_data);
            }
            
            // This may happen if the selection owner has changed
            if (r_data == NULL)
                return false;
            
            return true;
        }
    }
    
    return false;
}

/*
bool MCXTransferStore::Fetch(MCMIMEtype * p_mime_type, MCDataRef& r_data, Atom p_public_atom, Atom p_private_atom, Window p_source_window, Window p_target_window, Time p_lock_time) 
{
#ifdef DEBUG_DND
	fprintf(stderr, "Fetch_MIME() : Type = %s\n", p_mime_type -> asString() ) ;
#endif 
	if ( m_entries != NULL )
	{
		int4 t_idx = find_type ( p_mime_type ) ;
		if ( t_idx > -1 )
		{
			if ( m_entries[t_idx] . data == NULL )	// We SHOULD only need to get the data if we are the TARGET
			{
				GetExternalData( m_entries[t_idx] . type , p_public_atom, p_private_atom, p_source_window, p_target_window, p_lock_time, r_data) ;
				if (r_data != nil)
					m_entries[t_idx] . data = MCValueRetain(r_data);
			}
			else 
			{
				if (!m_internal_dnd) 
					Convert_REV_to_MIME ( m_entries[ t_idx ] . data , m_entries[t_idx] . type,  m_entries [t_idx] . format, r_data ) ;
				else
					r_data = MCValueRetain(m_entries[t_idx] . data);
			}

	
			if ( r_data == nil )		// This may happen if the selection owner has changed.
				return false ;
		
			return true ;
		}
	}
	return false ;
}
*/

bool MCGdkTransferStore::Fetch(MCTransferType p_type, MCDataRef& r_data, GdkAtom p_atom, GdkWindow *p_source, GdkWindow *p_target, guint32 p_event_time)
{
    return Fetch(rev_to_MIME_stored(p_type), r_data, p_atom, p_source, p_target, p_event_time);
}

/*
bool MCXTransferStore::Fetch(MCTransferType p_type, MCDataRef& r_data, Atom p_public_atom, Atom p_private_atom, Window p_source_window, Window p_target_window, Time p_lock_time)
{
#ifdef DEBUG_DND
	fprintf(stderr, "Fecth() : Asked for REV type %s\n", RevTypeToString (p_type));
#endif
	return ( Fetch ( rev_to_MIME_stored ( p_type ), r_data, p_public_atom, p_private_atom, p_source_window, p_target_window, p_lock_time ) ) ;
}


////////////////////////////////////////////////////////////////////////////////
//
// Utility and Misc functions


void MCXTransferStore::dumpClipboard(const char * p_msg)
{
Window w ;
	fprintf(stderr, "------- CLIPBOARD [ %s ] ----------\n", p_msg );
	w = XGetSelectionOwner(m_display, MCclipboardatom);
	fprintf(stderr, "Clipboard owner is : %x\n", w );
	xdnd_get_window_title ( w ) ;
	w = ((MCScreenDC*)MCscreen) -> GetNullWindow () ;
	fprintf(stderr, "Types on the clipboard : \n");

	XEvent xevent ;
	XConvertSelection (m_display, MCclipboardatom, XA_TARGETS, XdndMyAtom, w, LastEventTime ); 
	if ( WaitForEventCompletion(xevent) )
	{
		Atom * t_data ;
		uint4 t_count ;

		cleartypes();
		t_data = (Atom*)GetSelection ( xevent.xselection.requestor, XdndMyAtom , t_count) ;			
		for ( uint4 a =0 ; a < t_count ; a++)
			fprintf(stderr, "\t%s\n", AtomAsString((Atom)t_data[a]));
	}

	
	
	
}




void MCXTransferStore::dumpList ( char * p_msg )
{
	Entry t_entry ;
	fprintf(stderr, "MCXdndTypeList Dump -[ %s ]- \n", p_msg);
	for ( uint4 a = 0; a < m_entry_count ; a++)
	{
		t_entry = m_entries[a] ;
	  	fprintf(stderr, "\t RevType = %s \t Atom ID = %d \t Atom name = %s \t Data pointer = %x\n", RevTypeToString ( t_entry . type ) ,  t_entry . format, asString ( a ), t_entry . data ) ;
	}
}




const char *RevTypeToString(MCTransferType p_type)
{
	switch(p_type)
	{
	case TRANSFER_TYPE_TEXT:
		return "TRANSFER_TYPE_TEXT";
	case TRANSFER_TYPE_UNICODE_TEXT:
		return "TRANSFER_TYPE_UNICODE_TEXT";
	case TRANSFER_TYPE_STYLED_TEXT:
		return "TRANSFER_TYPE_STYLED_TEXT";
	case TRANSFER_TYPE_RTF_TEXT:
		return "TRANSFER_TYPE_RTF_TEXT";
	case TRANSFER_TYPE_HTML_TEXT:
		return "TRANSFER_TYPE_HTML_TEXT";
	case TRANSFER_TYPE_IMAGE:
		return "TRANSFER_TYPE_IMAGE";
	case TRANSFER_TYPE_FILES:
		return "TRANSFER_TYPE_FILES";
	case TRANSFER_TYPE_PRIVATE:
		return "TRANSFER_TYPE_PRIVATE";
	case TRANSFER_TYPE_OBJECTS:
		return "TRANSFER_TYPE_OBJECTS";
	}

	return "";
}




////////////////////////////////////////////////////////////////////////////////
//
// Format conversion routines
*/

bool MCGdkTransferStore::Convert_REV_to_MIME(MCDataRef p_input, MCTransferType p_type, MCMIMEtype *p_mime, MCDataRef &r_output)
{
    int32_t t_index;
    t_index = find_table_entry_with_full_types(p_type, p_mime);
    if (t_index > -1)
    {
        if (XTransfer_lookup_table[t_index].f_converter_to_mime != NULL)
        {
            return XTransfer_lookup_table[t_index].f_converter_to_mime(p_input, p_mime->asRev(), r_output);
        }
        else
        {
            r_output = MCValueRetain(p_input);
            return true;
        }
    }
    
    return false;
}

/*
bool MCXTransferStore::Convert_REV_to_MIME ( MCDataRef p_input, MCTransferType p_type,  MCMIMEtype * p_mime, MCDataRef& r_output ) 
{
#ifdef DEBUG_DND
	fprintf(stderr, "--> Converting type [%s] into [%s]\n",  RevTypeToString(p_type), p_mime->asString() ) ;
#endif
		
	int4 t_idx ;
	//t_idx = find_table_entry_with_MIME_type ( p_mime ) ;
	t_idx = find_table_entry_with_full_types ( p_type , p_mime ) ;
	if ( t_idx > -1 )
		if ( XTransfer_lookup_table [ t_idx ] . f_converter_to_mime != NULL )
		{
#ifdef DEBUG_DND
			fprintf(stderr, "\tHave got converter function.\n");
#endif
			return XTransfer_lookup_table [ t_idx ] . f_converter_to_mime ( p_input, p_mime->asRev(), r_output ) ;
		}
		else
		{
#ifdef DEBUG_DND
			fprintf(stderr, "\tno conversion function defined... doing nothing\n");
#endif
			r_output = MCValueRetain(p_input);
			return true;
		}
}
*/

bool MCGdkTransferStore::Convert_MIME_to_REV(MCDataRef p_input, MCMIMEtype *p_mime, MCDataRef &r_output)
{
    int32_t t_index;
    t_index = find_table_entry_with_MIME_type(p_mime);
    if (t_index > -1)
    {
        if (XTransfer_lookup_table[t_index].f_converter_to_rev != NULL)
        {
            return XTransfer_lookup_table[t_index].f_converter_to_rev(p_input, p_mime, r_output);
        }
        else
        {
            r_output = MCValueRetain(p_input);
            return true;
        }
    }
    
    return false;
}

/*
bool MCXTransferStore::Convert_MIME_to_REV ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output ) 
{
#ifdef DEBUG_DND
	fprintf(stderr, "<-- Converting type [%s] into [%s]\n", p_MIME->asString(), p_MIME->asRevString() ) ;
#endif

	int4 t_idx ;
	t_idx = find_table_entry_with_MIME_type ( p_MIME ) ;
	if ( t_idx > -1 )
		if ( XTransfer_lookup_table [ t_idx ] . f_converter_to_rev != NULL )
		{
#ifdef DEBUG_DND
			fprintf(stderr, "\tHave got converter function.\n");
#endif
			return XTransfer_lookup_table [ t_idx ] . f_converter_to_rev ( p_input, p_MIME, r_output ) ;
		}
		else 
		{
#ifdef DEBUG_DND
			fprintf(stderr, "\tno conversion function defined... doing nothing\n");
#endif
			r_output = MCValueRetain(p_input);
			return true;
		}
}*/

// Convert from UTF8 to TRANSFER_TYPE_STYLED_TEXT
bool ConvertUnicodeToStyled ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output ) 
{
    MCAutoDataRef t_unicode;
    if (!MCU_multibytetounicode(p_input, LCH_UTF8, &t_unicode))
		return false;

	return MCConvertUnicodeToStyledText(*t_unicode, r_output);
}


// Convert from TRANSFER_TYPE_STYLED_TEXT to UTF8
bool ConvertStyledToUnicode ( MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output ) 
{
	// Convert from Styles to UTF16
	MCAutoDataRef t_unicode;
    if (!MCConvertStyledTextToUnicode(p_input, &t_unicode))
		return false;

    return MCU_unicodetomultibyte(*t_unicode, LCH_UTF8, r_output);
}



// Convert from UTF8 to TRANSFER_TYPE_TEXT
bool ConvertUnicodeToText ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output )
{
    return MCU_multibytetounicode(p_input, LCH_UTF8, r_output);
}

// Convert from TRANSFER_TYPE_TEXT to UTF8
bool ConvertTextToUnicode (MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output)
{
    return MCU_unicodetomultibyte(p_input, LCH_UTF8, r_output);
}

bool ConvertStyled_rev_to_HTML ( MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output ) 
{
	return MCConvertStyledTextToHTML ( p_input, r_output );
}


bool ConvertStyled_rev_to_RTF ( MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output )  
{
	return MCConvertStyledTextToRTF ( p_input, r_output );
}


bool ConvertStyled_RTF_to_rev ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output )
{
	return MCConvertRTFToStyledText ( p_input, r_output );
}


bool ConvertStyled_HTML_to_rev ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output )
{
	return MCConvertHTMLToStyledText ( p_input, r_output );
}


bool ConvertStyled_rev_to_TEXT ( MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output ) 
{
	return MCConvertStyledTextToText ( p_input, r_output );
}

bool ConvertStyled_Text_to_rev ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output ) 
{
	r_output = MCValueRetain(p_input);
	return true;
}

bool ConvertFile_rev_to_MIME ( MCDataRef p_input, MCTransferType p_type, MCDataRef& r_output )  
{

    MCAutoStringRef t_input_files;
    // SN-2014-10-14: [[ Bug 13660 ]] If the decoding fails, returns false
    if (!MCStringDecode(p_input, kMCStringEncodingNative, false, &t_input_files))
        return false;

	// First break the string into chunks using return as the delimiter. This
	// gives us an array of ranges of the pieces between delimiters.
	MCAutoPointer<MCRange> t_ranges;
	uindex_t t_range_count;
	/* UNCHECKED */ MCStringBreakIntoChunks(*t_input_files, '\n', kMCStringOptionCompareExact, &t_ranges, t_range_count);

	// Create a mutable list ref.
	MCListRef t_output_files;
	/* UNCHECKED */ MCListCreateMutable('\n', t_output_files);

	// Loop through the ranges, using a formatted string and %*@ to append the appropriate portion of t_input_files as an element.
	for(uindex_t i = 0; i < t_range_count; i++)
		/* UNCHECKED */ MCListAppendFormat(t_output_files, "file://%*@", &(*t_ranges)[i], *t_input_files);

	// Build the output stringref.
	MCStringRef t_output_files_string;
	/* UNCHECKED */ MCListCopyAsStringAndRelease(t_output_files, t_output_files_string);

	// Finally encode as native string and encapsulate in a dataref.
	/* UNCHECKED */ MCStringEncodeAndRelease(t_output_files_string, kMCStringEncodingNative, false, r_output);

    return true;
}

bool ConvertFile_MIME_to_rev ( MCDataRef p_input, MCMIMEtype * p_MIME, MCDataRef& r_output )  
{
	MCAutoStringRef t_input_files;
    if (!MCStringDecode(p_input, kMCStringEncodingNative, false, &t_input_files))
        return false;

	MCAutoStringRef t_input_files_livecode;
	/* UNCHECKED */ MCStringConvertLineEndingsToLiveCode(*t_input_files, &t_input_files_livecode);
	MCAutoStringRef t_input_files_livecode_decoded;
	MCU_urldecode(*t_input_files_livecode, true, &t_input_files_livecode_decoded);
	
	MCAutoPointer<MCRange> t_ranges;
	uindex_t t_range_count;
	/* UNCHECKED */ MCStringBreakIntoChunks(*t_input_files_livecode_decoded, '\n', kMCStringOptionCompareExact, &t_ranges, t_range_count);

	// Create a mutable list ref.
	MCListRef t_output_files;
	/* UNCHECKED */ MCListCreateMutable('\n', t_output_files);

	for(uindex_t i = 0; i < t_range_count; i++)
	{
		MCAutoStringRef t_substring;
		MCStringCopySubstring(*t_input_files_livecode_decoded, (*t_ranges)[i], &t_substring);
		if (MCStringBeginsWithCString(*t_substring, (const char_t*) "file://", kMCCompareExact))  
			/* UNCHECKED */ MCListAppendSubstring(t_output_files, *t_substring, MCRangeMake(7, MCStringGetLength(*t_substring) - 7));
	}

	MCStringRef t_output_files_string;
	/* UNCHECKED */ MCListCopyAsStringAndRelease(t_output_files, t_output_files_string);

    // SN-2014-11-13: [[ Bug 13993 ]] MCPasteBoard::Fetch now returns a UTF-16 encoded string for the files
    /* UNCHECKED */ MCStringEncodeAndRelease(t_output_files_string, kMCStringEncodingUTF16, false, r_output);

	return true;
}

GdkDragContext* MCGdkTransferStore::CreateDragContext(GdkWindow *p_source)
{
    return gdk_drag_begin(p_source, ListTargets());
}

GList* MCGdkTransferStore::ListTargets()
{
    if (m_targets == NULL)
    {
        for (uint32_t i = 0; i < m_entry_count; i++)
        {
            m_targets = g_list_append(m_targets, gpointer(m_entries[i].m_mime->asAtom()));
        }
    }
    
    return m_targets;
}
