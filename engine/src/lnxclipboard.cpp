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
#include "transfer.h"
//#include "execpt.h" 
#include "dispatch.h"
#include "image.h" 
#include "globals.h"

#include "lnxdc.h"
#include "lnxtransfer.h" 
#include "lnxpasteboard.h"

///////////////////////////////////////////////////////////////////////////////
//
// Selection implimentation

bool MCScreenDC::ownsselection(void)
{
	GdkWindow *t_sel_owner;
	t_sel_owner = gdk_selection_owner_get_for_display(dpy, GDK_SELECTION_PRIMARY);
    bool t_owns;
    t_owns = t_sel_owner == NULLWindow || (t_sel_owner != NULL && MCdispatcher->findstackd(t_sel_owner));
    return t_owns;
}

MCPasteboard *MCScreenDC::getselection(void)
{
	MCGdkPasteboard *t_pasteboard ;

	if (!ownsclipboard()) 
		m_Selection_store->GetExternalTypes(GDK_SELECTION_CLIPBOARD, NULLWindow);

	// Create the pasteboard we will use, wrapping the clipboard
	t_pasteboard = new MCGdkPasteboard(GDK_SELECTION_PRIMARY, m_Selection_store);
	t_pasteboard->SetWindows(gdk_selection_owner_get_for_display(dpy, GDK_SELECTION_PRIMARY), NULLWindow);
	
	return t_pasteboard;
}

bool MCScreenDC::setselection(MCPasteboard *p_pasteboard)
{
	MCTransferType *t_ttypes;
	size_t ntypes;
	
	if (p_pasteboard != NULL)
	{
		m_Selection_store->cleartypes();

		p_pasteboard->Query(t_ttypes, ntypes);
		
		for (size_t i = 0; i < ntypes; i++)
		{
			MCAutoDataRef t_data;
			if (p_pasteboard->Fetch(t_ttypes[i], &t_data))
				m_Selection_store->addRevType(t_ttypes[i], *t_data);
		}
		
        gdk_selection_owner_set_for_display(dpy, NULLWindow, GDK_SELECTION_PRIMARY, MCeventtime, TRUE);
 
		return true;
	}
    
	return false ;
}

///////////////////////////////////////////////////////////////////////////////
//
// Clipboard implimentation



// Checks that there is a clipboard manager running
// and that it supports clipboard persistance
bool MCScreenDC::check_clipboard_manager(void)
{
    //Atom clipboard_manager;

    //clipboard_manager = make_atom ("CLIPBOARD_MANAGER");
    //return XGetSelectionOwner (dpy, clipboard_manager) != None;
    return false;
}



/////////////////////////////////////////////////////////////////////////////////
///
/// Make the clipboard selection live on after application exit.
///
/// First we need to convert the CLIPBOARD_MANAGER to SAVE_TARGETS --- i.e. ask 
/// the owner of the CLIPBOARD_MANAGER atom to start the SAVE_TARGETS protocol.
///
/// As we do not give it a list of targets we export, it will then ask us for the
/// target list - i.e. We get a SelectionRequest with target==TARGETS. We list 
/// the targets we support in the normal way and return them.
///
/// When we return the TARGETS we support, we will then get a MULTIPLE request from
/// the clipboard manager - SelectionRequest, target == MULTIPLE.
///
/// To support the MULTIPLE protocol for the CLIPBOARD_MANAGER we do :
///		1. Get the data for MULTIPLE from the clipboard manager. This is a list
///			of ATOM pairs --- property name <-> targets
///		2. Loop through each pair, grabbing our data in the specified target
///			format and setting the data of the specified property to the data.
///		3. If we cannot grab data in the format, we simple set the property data
///			to the None ATOM.
///		4. When we have finished our loop, send a XSendEvent to trigger a
///			SelectionNotify in the clipboard manager. This will allow the 
///			manager to grab all the data in the various formats.
///		5. Lastly, we loop though our list of targets that we grabbed from the
///			MULTIPLE event and delete each names property from the clipboard 
///			manamger. 
///

void MCScreenDC::make_clipboard_persistant(void)
{
	/*XEvent xevent ;
	
	Atom clipboard_manager, save_targets, rev_targets;
 
	clipboard_manager = make_atom ("CLIPBOARD_MANAGER");
	save_targets = 		make_atom ("SAVE_TARGETS");
	

	if ( ownsclipboard() )
	{
		XConvertSelection ( dpy, clipboard_manager, save_targets, None, NULLWindow, MCeventtime );
		
		bool done = false ;
		
		while ( !done )
		{
		
			XAllowEvents (dpy , SyncPointer, CurrentTime);
			XNextEvent( dpy , &xevent);
			
			if ( xevent.type == SelectionNotify )
				done = true ;
			

			if ( xevent.type == SelectionRequest )
			{
				
				XSelectionRequestEvent * srevent ;
				srevent = &xevent.xselectionrequest ;

				XSelectionEvent sendevent;
				sendevent.type = SelectionNotify;
				sendevent.send_event = True;
				sendevent.display = srevent->display;
				sendevent.requestor = srevent->requestor;
				sendevent.selection = srevent->selection;
				sendevent.time = srevent->time;
				sendevent.target = srevent->target;
				sendevent.property = srevent->property ;

	
				if ( srevent->target == XA_TARGETS)
				{
					uint4 t_count ;
					Atom *t_atoms ;
					t_atoms = m_Clipboard_store -> QueryAtoms ( t_count );
					
					if ( t_atoms != NULL)
					{
						XChangeProperty(dpy, srevent -> requestor, srevent -> property,
										XA_ATOM, 32, PropModeReplace,
										(const unsigned char *)t_atoms,
										t_count) ;
						
						sendevent.property = srevent->target;						
						XSendEvent(dpy, srevent -> requestor, False, 0, (XEvent *)&sendevent);
						
						free(t_atoms) ;
					}
				}
				else if ( srevent->target == make_atom("MULTIPLE") )
				{
					unsigned long remaining;
					unsigned long count;

					unsigned char *s = NULL ;
					Atom actual;
					int format;
					XGetWindowProperty (dpy, srevent->requestor , srevent->property, 
										0, 65536, false,
										AnyPropertyType, &actual, &format,
										&count, &remaining,
										&s) ;

					Atom *atoms ;
					uint4 t_bytes = count * ( format / 8 );
			
					atoms = (Atom*)malloc ( t_bytes ) ;
					memset ( atoms, 0, t_bytes ) ;
					memcpy(atoms, s, t_bytes ) ;
			
					XFree(s);
					
					Atom property, target ;
					for (uint4 a=0; a<count; a++)
					{
						property = target = atoms[a] ;
						a++;

						MCAutoDataRef t_data; 
						
						if ( ( target != make_atom("TARGETS") ) &&
							 ( target != make_atom("TIMESTAMP") ) &&
							 ( target != make_atom("SAVE_TARGETS") ))
						{
							if (m_Clipboard_store -> Fetch(  new MCMIMEtype(dpy, target), &t_data, None, None, DNULL, DNULL, MCeventtime ))
							{
								XChangeProperty(dpy, srevent -> requestor, property,
											XA_STRING, 8, PropModeReplace,
											(const unsigned char *)MCDataGetBytePtr(*t_data),
											MCDataGetLength(*t_data));
								
							}
							else 
							{
								XChangeProperty(dpy, srevent -> requestor, property,
											XInternAtom(dpy, "None", false), 8, PropModeReplace,
											(const unsigned char *)NULL, 0 );
							}
						}
					}
							
						
					sendevent.property = srevent->property ;
					XSendEvent (dpy, sendevent.requestor, False, 0, (XEvent *)&sendevent );

					for (uint4 a=0; a<count; a++)
					{
						a++;
						XDeleteProperty(dpy, srevent->requestor, target);
					}
							
					XDeleteProperty(dpy, srevent->requestor, srevent->property);
					free(atoms);

				}
			}
		}
	}
     */
}

void MCScreenDC::flushclipboard(void)
{
	if (check_clipboard_manager())
		make_clipboard_persistant();
}

bool MCScreenDC::ownsclipboard(void)
{
	GdkWindow *w;
	w = gdk_selection_owner_get_for_display(dpy, GDK_SELECTION_CLIPBOARD);
	return w == NULLWindow || (w != NULL && MCdispatcher -> findstackd(w) != NULL);
}

MCPasteboard *MCScreenDC::getclipboard(void)
{
	MCGdkPasteboard *t_pasteboard ;

	if (!ownsclipboard()) 
		m_Clipboard_store -> GetExternalTypes(GDK_SELECTION_CLIPBOARD, NULLWindow);

	// Create the pasteboard we will use, wrapping the clipboard
	t_pasteboard = new MCGdkPasteboard(GDK_SELECTION_CLIPBOARD, m_Clipboard_store);
	t_pasteboard -> SetWindows(gdk_selection_owner_get_for_display(dpy, GDK_SELECTION_CLIPBOARD), NULLWindow);
	
	return t_pasteboard;
}

bool MCScreenDC::setclipboard(MCPasteboard *p_pasteboard)
{
	MCTransferType *t_ttypes;
	size_t ntypes;
	
	m_Clipboard_store -> cleartypes();

	p_pasteboard -> Query(t_ttypes, ntypes);
	
	for (size_t i = 0; i < ntypes; i++)
	{
		MCAutoDataRef t_data;
		if (p_pasteboard -> Fetch(t_ttypes[i], &t_data))
			m_Clipboard_store -> addRevType(t_ttypes[i], *t_data);
	}
	
	gdk_selection_owner_set_for_display(dpy, NULLWindow, GDK_SELECTION_CLIPBOARD, MCeventtime, TRUE);

	return true;
}

