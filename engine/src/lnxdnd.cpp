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
#include "execpt.h" 
#include "dispatch.h"
#include "image.h"
#include "globals.h"

#include "lnxdc.h"

#include "lnxtransfer.h"
#include "lnxpasteboard.h"
#include "lnxdnd.h" 


bool xDnDInit = false ;
bool xdnd_accept_drop = false ;
bool xdnd_done = false ;
bool xdnd_target_done = false ; 
bool xdnd_set_root_window = false ;
bool xdnd_sent_drop = false ;


int want_position; 
int ready_to_drop;
int will_accept;
XRectangle rectangle;
Atom supported_action;
bool window_is_rev = false ;

Window source_window ;
Window target_window ;

Cursor xdnd_cursor_drop_ok ;
Cursor xdnd_cursor_drop_fail ;


Display *m_display ;


enum XDnDStageType
{
	XDND_STAGE_NONE,
	XDND_STAGE_FINDING_WINDOW,
	XDND_STAGE_DRAGGING,
	XDND_STAGE_POSITION_SENT,
	XDND_STAGE_DROPING,
	XDND_STAGE_WAITING
};

XDnDStageType 	xdnd_stage ;	
bool			xdnd_own_window ;
Window			xdnd_target_window ;
bool 			xdnd_target_window_is_aware ;
bool 			new_target_window_is_aware ;
Atom 			last_target ;
	
MCXTransferStore * MCtransferstore ;


char * xdnd_get_window_title ( Window w )  ;
void dump_atom ( Atom p_atom) ;
void dump_action ( MCDragAction p_action ) ;

/*=========================================================================================

			                     X d n d     P R O T C O L 

=========================================================================================*/


void setup_atoms (void)
{
	MCScreenDC *sdc = (MCScreenDC*)MCscreen ;

	WMname = sdc -> make_atom ( "WM_NAME") ;
	
	XdndMyAtom = sdc -> make_atom ( "XdndRevTransfer");
	XdndRevolutionWindow = sdc -> make_atom ("XdndRevolutionWindow") ; 
	
	XdndAware = sdc -> make_atom ( "XdndAware") ;
	

	XdndSelection = sdc -> make_atom ("XdndSelection" );
    XdndEnter = sdc -> make_atom ("XdndEnter" );
    XdndLeave = sdc -> make_atom ("XdndLeave" );
    XdndPosition = sdc -> make_atom ("XdndPosition" );
    XdndDrop = sdc -> make_atom ("XdndDrop" );
    XdndFinished = sdc -> make_atom ("XdndFinished" );
    XdndStatus = sdc -> make_atom ("XdndStatus" );
    XdndActionCopy = sdc -> make_atom ("XdndActionCopy" );
    XdndActionMove = sdc -> make_atom ("XdndActionMove" );
    XdndActionLink = sdc -> make_atom ("XdndActionLink" );
    XdndActionAsk = sdc -> make_atom ("XdndActionAsk" );
    XdndActionPrivate = sdc -> make_atom ("XdndActionPrivate" );
    XdndTypeList = sdc -> make_atom ("XdndTypeList" );
    XdndActionList = sdc -> make_atom ("XdndActionList" );
    XdndActionDescription = sdc -> make_atom ("XdndActionDescription" );
	
	XA_TARGETS = sdc -> make_atom ("TARGETS");
	
}


// Do all the setup of the xDnD protocol
void init_xDnD(void)
{
	if ( !xDnDInit ) 
	{
		xDnDInit = true ;
		setup_atoms();
		
		xdnd_stage = XDND_STAGE_NONE ;
		xdnd_target_window = DNULL ;
		
		m_display = ((MCScreenDC*)MCscreen) -> dpy;
		
		// Set up the cursors we will use.
		xdnd_cursor_drop_ok = XCreateFontCursor ( m_display, XC_cross ) ;
		xdnd_cursor_drop_fail = XCreateFontCursor ( m_display, XC_pirate ) ;
		
		MCtransferstore = new MCXTransferStore ( m_display ) ;
		

	}
}

void shutdown_xdnd(void)
{
	XFreeCursor ( m_display, xdnd_cursor_drop_ok ) ;
	XFreeCursor ( m_display, xdnd_cursor_drop_fail ) ;
}




void xdnd_set_cursors ( Window w , bool drop_ok, MCImage *p_image )
{
	if ( drop_ok )
		XDefineCursor ( m_display, w, xdnd_cursor_drop_ok ) ;
	else 
		XDefineCursor ( m_display, w, xdnd_cursor_drop_fail ) ;
}	
	
void xdnd_make_window_aware ( Window w )
{
	uint1 version = XDND_VERSION ;		
	//if ( !xdnd_set_root_window ) 
	{
		XChangeProperty (m_display, w, XdndAware, XA_ATOM, 32, PropModeReplace, (unsigned char *) &version, 1);
		xdnd_set_root_window = true ; 
	}
	
	
	// The XdndRevolutionWindow will be used so we can easily see if the window belongs to us...
	XChangeProperty (m_display, w, XdndRevolutionWindow, XA_ATOM, 32, PropModeReplace, (unsigned char *) &version, 1);
}


bool xdnd_window_has_property ( Window w, Atom prop ) 
{
	
	Atom type;
  	int format;
  	unsigned long nitems, extra;
  	unsigned char *uprop;
  	char *newprop;
	Atom *types ;
	uint4 t ;

	XGetWindowProperty( m_display, w, prop, 0, 65535, False, XA_ATOM, &type,
		     &format, &nitems, &extra, &uprop);
	

	return ( nitems > 0 ) ;
}


bool xdnd_window_is_revolution ( Window w ) 
{
	return ( MCdispatcher -> findstackd ( w ) != NULL ) ; 
}



bool xdnd_window_is_aware ( Window w ) 
{
	 Atom actual;
    int format;
    unsigned long count, remaining;
    unsigned char *data = 0;
    Atom *types, *t;
    int version ;

    version = 0;
	
	if ( xdnd_window_has_property ( w, XdndAware ))
	{
		XGetWindowProperty (m_display, w, XdndAware,
				0, 0x8000000L, False, XA_ATOM,
				&actual, &format,
				&count, &remaining, &data);

		types = (Atom *) data;
		version = types[0] ;
		XFree(data);
		return true ;
	}
	
	return false ;
}


Window xdnd_locate_window( XEvent xevent )
{
	Window root_return, child_return;
	Window ret_window ;
	bool done = false ;
	unsigned int mask_return;
		 	
	new_target_window_is_aware = false ;
	
	if (xevent.xmotion.subwindow == NULL) 
	{
		xevent.xmotion.subwindow = xevent.xmotion.window;
		new_target_window_is_aware = xdnd_window_is_aware ( xevent.xmotion.subwindow ) ;
	} 
	else
	{
		while ( XQueryPointer (m_display, xevent.xmotion.subwindow, &root_return, &child_return,
					  &xevent.xmotion.x_root, &xevent.xmotion.y_root, &xevent.xmotion.x,
					  &xevent.xmotion.y, &mask_return) ) 
		{
			
			if ( xdnd_window_is_aware ( xevent.xmotion.subwindow ) )
			{
				new_target_window_is_aware = true ;
				break ;
			}
			
			if (!child_return)
				break ;

			xevent.xmotion.subwindow = child_return;
		}			
	}
	
	return ( xevent.xmotion.subwindow ) ;
}
	





void xdnd_send_event ( Window window, XEvent *xevent ) 
{
    XSendEvent (m_display, window, 0, 0, xevent);
}



void xdnd_send_drop ( Window window, Window from, unsigned long time)
{
    XEvent xevent;

    memset (&xevent, 0, sizeof (xevent));

    xevent.xany.type = ClientMessage;
    xevent.xany.display = m_display ;
    xevent.xclient.window = window;
    xevent.xclient.message_type = XdndDrop;
    xevent.xclient.format = 32;

    XDND_DROP_SOURCE_WIN (&xevent) = from;
	XDND_DROP_TIME (&xevent) = time;

    xdnd_send_event ( window, &xevent);
}


void xdnd_send_position (Window window, Window from, int x, int y, unsigned long time, MCDragAction p_action)
{

	if ( xdnd_sent_drop ) 
		return ;
	XEvent xevent;

    memset (&xevent, 0, sizeof (xevent));

    xevent.xany.type = ClientMessage;
    xevent.xany.display = m_display;
    xevent.xclient.window = window;
    xevent.xclient.message_type = XdndPosition;
    xevent.xclient.format = 32;

    XDND_POSITION_SOURCE_WIN (&xevent) = from;
    XDND_POSITION_ROOT_SET (&xevent, x, y);
	XDND_POSITION_TIME (&xevent) = time;
    XDND_POSITION_ACTION (&xevent) = p_action;

    xdnd_send_event (window, &xevent);
}

void xdnd_send_enter_leave ( Window window, Window from, Atom enterleave ) 
{
    XEvent xevent;
    int n, i;
	n = 0 ;

    memset (&xevent, 0, sizeof (xevent));

    xevent.xany.type = ClientMessage;
    xevent.xany.display = m_display ;
    xevent.xclient.window = window;
    xevent.xclient.message_type = enterleave;
    xevent.xclient.format = 32;

    XDND_ENTER_SOURCE_WIN (&xevent) = from;
	if ( enterleave == XdndEnter )
	{
		i = XDND_VERSION ;
		XDND_ENTER_VERSION_SET (&xevent, i);

		XDND_ENTER_THREE_TYPES_SET (&xevent, MCtransferstore -> getCount() > 3 );
		MCtransferstore -> apply_to_message ( &xevent ) ;
	}
		
#ifdef DEBUG_DND
MCtransferstore -> dumpList("Enter or Leave event");
#endif

	xdnd_send_event ( window, &xevent);
}


void xdnd_send_enter (Window window, Window from, Atom * typelist)
{
	xdnd_send_enter_leave ( window, from , XdndEnter ) ;
}

void xdnd_send_leave (Window window, Window from, Atom * typelist)
{
	xdnd_send_enter_leave ( window, from , XdndLeave ) ;
}




void xdnd_selection_send (XSelectionRequestEvent * request, const char *data, int length)
{
    XEvent xevent;
	
	XChangeProperty (m_display, request->requestor, request->property,
		     request->target, 8, PropModeReplace, (unsigned const char*)data, length);
	
    xevent.xany.type = SelectionNotify;
    xevent.xany.display = request->display;
    xevent.xselection.property = request->property;
    xevent.xselection.requestor = request->requestor;
    xevent.xselection.selection = request->selection;
    xevent.xselection.target = request->target;
    xevent.xselection.time = request->time;
	
    xdnd_send_event ( request->requestor, &xevent);
}


void xdnd_send_status ( Window window, Window from, int will_accept, Atom action)
{
    XEvent xevent;

    memset (&xevent, 0, sizeof (xevent));

    xevent.xany.type = ClientMessage;
    xevent.xany.display = m_display ;
    xevent.xclient.window = window;
    xevent.xclient.message_type = XdndStatus;
    xevent.xclient.format = 32;

    XDND_STATUS_TARGET_WIN (&xevent) = from;
    XDND_STATUS_WILL_ACCEPT_SET (&xevent, will_accept);
    
	// We will ALWAYS want the position, whenever the mouse moves.
	if (will_accept)
		XDND_STATUS_WANT_POSITION_SET (&xevent, true);
    
	XDND_STATUS_RECT_SET (&xevent, 0,0,0,0);

	if ( will_accept )
    	XDND_STATUS_ACTION (&xevent) = action;

    xdnd_send_event ( window, &xevent);
}


void xdnd_send_finished ( Window window, Window from )
{
    XEvent xevent;

    memset (&xevent, 0, sizeof (xevent));

    xevent.xany.type = ClientMessage;
    xevent.xany.display = m_display;
    xevent.xclient.window = window;
    xevent.xclient.message_type = XdndFinished;
    xevent.xclient.format = 32;

    XDND_FINISHED_TARGET_WIN (&xevent) = from;

    xdnd_send_event ( window, &xevent);
}



bool outside_rectangle (int x, int y, XRectangle * r)
{
    return (x < r->x || y < r->y || x >= r->x + r->width || y >= r->y + r->height);
}





void xdnd_get_type_list (XEvent xevent ,  Window window )
{
    Atom type, *a;
    int format, i;
    unsigned long count, remaining;
    unsigned char *data = NULL;

	// If we only have 3 types published, we get them directly from the XdndEnter message.
	// If not, we need to query the XdndTypeList of the source window.
	if ( XDND_ENTER_THREE_TYPES ( &xevent ) )
	{
		count = 3 ;
		for ( i = 0 ; i < 3 ; i++ )
			MCtransferstore -> addAtom ( XDND_ENTER_TYPE( &xevent, i ) ) ;
		
	}
	else 
	{
		
		XGetWindowProperty (m_display, window, XdndTypeList,
				0, 0x8000000L, False, XA_ATOM,
				&type, &format, &count, &remaining, &data);

		a = (Atom *) data;
		for (i = 0; i < count; i++)
			MCtransferstore -> addAtom ( a[i] ) ;

		XFree (data);
	}

#ifdef DEBUG_DND
	MCtransferstore -> dumpList("I am a Target") ;
#endif
	
	
}


// Translates the co-ordinates from root window origin to Window w origin...
void xdnd_translate_coords (Window w, int4 sx, int4 sy, int4 &wx, int4 &wy ) 
{
	Window root_return, child_return ;
	uint4 mask_return ;
	XQueryPointer (m_display, w,  &root_return, &child_return,
					  &sx, &sy, &wx, &wy, &mask_return) ;
}
					  
					  

bool xdnd_interested_in_event ( XEvent xevent ) 
{

	// If we get a MotionNotify, then grab the timestamp.
	if ( xevent.type == MotionNotify ) 
		LastEventTime = xevent.xmotion.time ;
	
	
	// If se get a SelectionNotify, we are only interested if the target property is
	// either XdndSelection or XdndMyAtom.
	if ( xevent.type == SelectionNotify )
		return ( xevent.xselectionrequest.target == XdndSelection ) ||
			   ( xevent.xselectionrequest.target == XdndMyAtom ) ;

	
	// We have recieved a Client Message
	// We are interested in it only if it is related to the xDnD protocol.
	if ( xevent.type == ClientMessage )
		return ( ( xevent.xclient.message_type == XdndEnter ) ||
		   		 ( xevent.xclient.message_type == XdndLeave ) ||
		   		 ( xevent.xclient.message_type == XdndPosition ) ||
		   		 ( xevent.xclient.message_type == XdndDrop ) );
	
	return ( false ) ;
		
}


int xdnd_convert_selection ( Window from, Atom type, Time p_time)
{
    XConvertSelection (m_display, XdndSelection, type , XdndMyAtom , from, p_time);
    return 0;
}




void xdnd_set_cursor ( Window w , uint4 image_id )
{
	MCImage *im ;
	im = (MCImage *)MCdispatcher->getobjid(CT_IMAGE, image_id ) ;
	if ( im != NULL ) 
		((MCScreenDC*)MCscreen) -> setcursor ( w, im -> getcursor() ) ;
}




MCDragAction action_atom_to_rev ( Atom p_action )
{

	if ( p_action == XdndActionCopy ) return ( DRAG_ACTION_COPY);
	if ( p_action == XdndActionMove ) return ( DRAG_ACTION_MOVE);
	if ( p_action == XdndActionLink ) return ( DRAG_ACTION_LINK);
	return (DRAG_ACTION_NONE)	;
}


Atom action_rev_to_atom ( MCDragAction p_action)
{
	switch(p_action)
	{
		case DRAG_ACTION_COPY:
			return ( XdndActionCopy );
		case DRAG_ACTION_MOVE:
			return (XdndActionMove);
		case DRAG_ACTION_LINK:
			return (XdndActionLink);
	}
	return None;
}


uint2 make_modifier_state ( XEvent *xevent ) 
{
	return ( ((MCScreenDC*)MCscreen)->querymods() );
}



MCDragAction best_action_from_set ( MCDragActionSet p_action_set )
{
	if ( p_action_set & DRAG_ACTION_LINK != 0 )
		return ( DRAG_ACTION_LINK);
	if ( p_action_set & DRAG_ACTION_MOVE != 0 )
		return ( DRAG_ACTION_MOVE);
	return ( DRAG_ACTION_COPY );
}


/*=========================================================================================

			                          D n D   T A R G E T

=========================================================================================*/

void xdnd_target_event_loop ( XEvent xevent ) 
{	
	MCXPasteboard * t_pasteboard ;
	// Is this an event that we need to handle as a DnD Target?
	if ( ! xdnd_interested_in_event ( xevent ) )
		return ;
	
	// Make sure we set this to False as we are NOT doing internal DnD
	window_is_rev = false ;
	
	switch ( xevent.type )
	{
		case ClientMessage:

			source_window = XDND_POSITION_SOURCE_WIN ( &xevent ) ;
			target_window = xevent.xclient.window ;
		

			if ( xevent.xclient.message_type == XdndEnter )
			{
				
				t_pasteboard = new MCXPasteboard ( XdndSelection, XdndMyAtom, MCtransferstore ) ;
				t_pasteboard -> SetWindows ( source_window, target_window ) ;

				MCtransferstore -> cleartypes();
				xdnd_get_type_list ( xevent, source_window ) ;
				
				
				uint2 t_old_modstate = MCmodifierstate ;
				MCmodifierstate = make_modifier_state(&xevent);
				MCdispatcher -> wmdragenter ( target_window, t_pasteboard ) ; 
				
				MCmodifierstate = t_old_modstate ;
				t_pasteboard -> Release();
			}

		
			if ( xevent.xclient.message_type == XdndLeave )
			{
				MCdispatcher -> wmdragleave ( target_window ) ;
				MCtransferstore -> cleartypes();
			}
		
		
		
			if ( xevent.xclient.message_type == XdndPosition )
			{
				LastPositionTime = XDND_POSITION_TIME ( &xevent ) ;
				
				MCDragActionSet t_action ;
				int4 sx, sy, wx, wy ;
				
				sx = XDND_POSITION_ROOT_X( &xevent ) ;
				sy = XDND_POSITION_ROOT_Y( &xevent ) ;
				
				// Translate the coords into ones relative to the specified window.
				xdnd_translate_coords ( target_window, sx, sy, wx, wy ) ;
				
				uint2 t_old_modstate = MCmodifierstate ;
				MCmodifierstate = make_modifier_state(&xevent);
				t_action = MCdispatcher -> wmdragmove ( target_window, wx , wy );
				MCmodifierstate = t_old_modstate ;

				//TODO : Need to translate the t_action --> action Atom.
				xdnd_send_status ( source_window, target_window , ( t_action != DRAG_ACTION_NONE ), action_rev_to_atom(t_action) ) ;
			}

		
			if ( xevent.xclient.message_type == XdndDrop )
			{
				
				uint2 t_old_modstate = MCmodifierstate ;
				MCmodifierstate = make_modifier_state(&xevent);
				// The call to wmdragdrop _may_ end up calling fetchdragdata, which will do the rest of the DnD protocol.
				MCdispatcher -> wmdragdrop ( target_window );
				MCmodifierstate = t_old_modstate ;

				// We can now signal that we are all done.
				xdnd_send_finished ( source_window, target_window  ) ;
			}
		
			
			
			break;
		
	}
	
	
}


/*=========================================================================================

			                          D n D   S O U R C E 

=========================================================================================*/

// SN-2014-07-11: [[ Bug 12769 ]] Update the signature - the non-implemented UIDC dodragdrop was called otherwise
MCDragAction MCScreenDC::dodragdrop(Window w, MCPasteboard *p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint* p_image_offset)
{

	XEvent xevent ;
	MCTransferType p_type ;
	MCDragAction t_action ;
	
	
	// Ensure we have initialized everything we need to do for xDnD.... this should preferably have been done on screen
	// open, as that needs the XdndAware atom for any newly created window.
	init_xDnD();
	
	
	MCtransferstore -> cleartypes();
	
	
	// Loop over all types returned by p_pasteboard -> Query ()
	MCTransferType *t_ttypes ;
	uint4 ntypes ;
	MCSharedString *t_data ;
	MCXPasteboard * t_pasteboard ;
	
	MCDragAction t_dragactiondone = DRAG_ACTION_NONE ;
	
	t_pasteboard = new MCXPasteboard ( XdndSelection , XdndMyAtom, MCtransferstore ) ;
	
	if (! p_pasteboard -> Query ( t_ttypes, ntypes ))
		return t_dragactiondone;
	
	for ( uint4 a = 0 ; a < ntypes ; a++)
	{
		if ( p_pasteboard -> Fetch ( t_ttypes[a], t_data ) )
			MCtransferstore -> addRevType ( t_ttypes[a], t_data) ;
	}
	
	MCtransferstore -> apply_to_window ( last_window ) ;
		

	// First grab the pointer -- we don't want anyone else getting our XEvents.
	XGrabPointer(dpy, DefaultRootWindow ( m_display ) , False,
	             PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
	             GrabModeAsync, GrabModeAsync, None,
	             None, CurrentTime);
		
	
	// I want to own the selection please.
	XSetSelectionOwner (m_display, XdndSelection, last_window, MCeventtime);
	
	
	// Initilize various bits and pieces of state information.
	xdnd_accept_drop = false ;
	xdnd_done = false ;
	
	xdnd_target_window = DNULL ;
	source_window = DNULL ;
	target_window = DNULL ;
	xdnd_target_window_is_aware = false ;
	new_target_window_is_aware = false ;
	window_is_rev = false ;
	last_target = 0 ;
	xdnd_sent_drop = false ;
		
#ifdef DEBUG_DND
	fprintf(stderr, "****************** STARTING DODRAGDROP loop ***********************\n");
#endif
	
	uint2 t_old_modstate = MCmodifierstate ;
		
	while (!xdnd_done)
	{

		XAllowEvents (dpy , SyncPointer, CurrentTime);
		XNextEvent( dpy , &xevent);
			
		
		{
			switch (xevent.type) 
			{
				
			
				// Need to pass the expose events back though the normal channels.
				case GraphicsExpose:
				case Expose:
							XPutBackEvent(dpy , &xevent);
							MCscreen -> expose();
				break;
				
				case KeyPress:
				case KeyRelease:
					
					MCmodifierstate = make_modifier_state ( &xevent ) ;

				break;

				case MotionNotify:
					Window new_target_window ;
					
					new_target_window = xdnd_locate_window(xevent);
					window_is_rev = xdnd_window_is_revolution ( new_target_window ) ;
				
					LastEventTime = xevent.xmotion.time ;
				
					
					if ( new_target_window != xdnd_target_window ) 
					{

						// We also need to send an XdndLeave message to the window we are leaving
						if ( xdnd_target_window_is_aware )
						{
							xdnd_stage = XDND_STAGE_DRAGGING ;
							if ( !xdnd_window_is_revolution ( xdnd_target_window ) ) 
							{
								xdnd_send_leave ( xdnd_target_window, last_window, NULL ) ;
								XDefineCursor ( m_display, xdnd_target_window, None ) ;
							}
							else 
								MCdispatcher -> wmdragleave ( xdnd_target_window ) ;
						}
						
						// OK, we have moved into a new window, so send a XdndEnter message

						if  ( new_target_window_is_aware )
						{
							xdnd_stage = XDND_STAGE_DRAGGING ;
							if ( !window_is_rev )
							{
								xdnd_send_enter( new_target_window, last_window, NULL ) ;
							}
							else 
								MCdispatcher -> wmdragenter ( new_target_window, t_pasteboard ) ;
						}
						
						
						xdnd_target_window = new_target_window ;
						xdnd_target_window_is_aware = new_target_window_is_aware ;
		
					}
					else 
					{
						if ( xdnd_target_window_is_aware )
						{
							
							if ( xdnd_stage != XDND_STAGE_POSITION_SENT  && outside_rectangle ( xevent.xmotion.x_root, xevent.xmotion.y_root, &rectangle ) )
							{
								xdnd_stage = XDND_STAGE_POSITION_SENT ;
								
								xdnd_set_cursors( xdnd_target_window, xdnd_accept_drop, p_image ) ;

								
								if ( !window_is_rev ) 
								{
									xdnd_send_position (xdnd_target_window, last_window, xevent.xmotion.x_root, xevent.xmotion.y_root, MCeventtime, best_action_from_set(p_allowed_actions) ) ;
								}
								else 
								{
									
									LastPositionTime = LastEventTime ; 
									
									int4 sx, sy, wx, wy ;
									
									// Get the coords relative to the root window.
									sx = XDND_POSITION_ROOT_X( &xevent ) ;
									sy = XDND_POSITION_ROOT_Y( &xevent ) ;
									
									// Translate the coords into ones relative to the specified window.
									xdnd_translate_coords ( xdnd_target_window, sx, sy, wx, wy ) ;
									
									t_action = MCdispatcher -> wmdragmove ( xdnd_target_window, wx , wy );
									xdnd_accept_drop = ( t_action != DRAG_ACTION_NONE ) ;
									
									xdnd_stage = XDND_STAGE_DRAGGING ;
								}
								
							}
						}
					}
						
					
					break ;
				
				
				case ClientMessage:
				
					if (xevent.xclient.message_type == XdndStatus ) 
					{
				
						want_position = XDND_STATUS_WANT_POSITION (&xevent);
						ready_to_drop = XDND_STATUS_WILL_ACCEPT (&xevent);
						rectangle.x = XDND_STATUS_RECT_X (&xevent);
						rectangle.y = XDND_STATUS_RECT_Y (&xevent);
						rectangle.width = XDND_STATUS_RECT_WIDTH (&xevent);
						rectangle.height = XDND_STATUS_RECT_HEIGHT (&xevent);
						supported_action = XDND_STATUS_ACTION (&xevent);
						t_action = action_atom_to_rev( supported_action );
																		
						xdnd_accept_drop = ready_to_drop ;
						
						xdnd_stage = XDND_STAGE_DRAGGING ; 
					}
				
				// TODO - need a timeout so we will _always_ exit our look, no matter how bad the Target is.
				// Need to be really careful here in case the Target is _BAD_ or has crashed...
				//		    if (xevent.xmotion.time > time + (dnd->time_out ? dnd->time_out * 1000 : 10000)) {	/* allow a ten second timeout as default */

				if ( xevent.xclient.message_type == XdndFinished )
				{
					XDefineCursor ( m_display, xdnd_target_window, None ) ;
					xdnd_done = true ;
				}
				
				break;
				
				// The Target has reqested the selection, which is valid for the xDnD
				// protocol so it can further check if it will accept.
				case SelectionRequest:
					if ( xevent.xselectionrequest.selection == XdndSelection ) 
					{
#ifdef DEBUG_DND
						fprintf(stderr, "A selection request has been made by %x\n" , xevent.xselectionrequest.requestor );
						fprintf(stderr, "\t Selection = %s \t Target = %s \t Property = %s \n", XGetAtomName ( m_display, xevent.xselectionrequest.selection ) , 
																								XGetAtomName ( m_display, xevent.xselectionrequest.target ) , 
																								XGetAtomName ( m_display, xevent.xselectionrequest.property ) );
#endif

						MCSharedString * t_string ;
						MCTransferType t_type ;
						MCMIMEtype * t_mime_type ;
						
						t_mime_type = new MCMIMEtype ( dpy, xevent.xselectionrequest.target ) ;
						t_type = t_mime_type->asRev() ;
						last_target = xevent.xselectionrequest.target ;
						if ( t_pasteboard->Fetch_MIME ( t_mime_type, t_string ) )
							xdnd_selection_send( &xevent.xselectionrequest, t_string -> Get() . getstring() , t_string -> Get() . getlength() ) ;
						xdnd_stage = XDND_STAGE_WAITING ;
					}
				break;
				
				
				case ButtonRelease:
					if ( !window_is_rev ) 
					{
						if ( xdnd_accept_drop ) 
						{
							XSetSelectionOwner ( m_display, last_target, last_window, MCeventtime );
							xdnd_send_drop ( xdnd_target_window, last_window, MCeventtime) ;
							xdnd_sent_drop  = true ;
						}
						else 
						{
							xdnd_send_leave ( xdnd_target_window, last_window, NULL ) ;
							xdnd_done = true ;
						}
					}
					else 
					{
						if ( xdnd_accept_drop ) 
						{
							MCtransferstore -> internal ( true ) ;
							XUngrabPointer(dpy, CurrentTime ) ;
							t_action = MCdispatcher -> wmdragdrop ( xdnd_target_window );

						}
						xdnd_done = true ;
						
					}
				
				break ;
				
			}
		}
		
	}		
	

	
#ifdef DEBUG_DND
	fprintf(stderr, "****************** Ended DODRAGDROP loop ***********************\n");
#endif
	
	XUngrabPointer(dpy, CurrentTime ) ;
	XDefineCursor ( m_display, xdnd_target_window, None ) ;
	
	if ( t_pasteboard != NULL)
		t_pasteboard -> Release();
	
	MCmodifierstate = t_old_modstate ;
	
	return t_action ;
	
}



/*=========================================================================================

			             O L D     E N G I N E    I N T E R F A C E

=========================================================================================*/


const char *RevTypeToString(MCTransferType p_type);

MCTransferType MCScreenDC::querydragdata(void)
{
	return TRANSFER_TYPE_NULL ;
}


MCSharedString *MCScreenDC::fetchdragdata(void)
{
	return NULL;
}






/*=========================================================================================

			                     D E B U G    R O U T I N E S 

=========================================================================================*/




char * xdnd_get_window_title ( Window w ) 
{
	
	Atom type;
  	int format;
  	unsigned long nitems, extra;
  	unsigned char *uprop;
  	char *newprop;

	XGetWindowProperty( m_display, w, WMname, 0, 65535, False, XA_STRING, &type,
		     &format, &nitems, &extra, &uprop);
	
	fprintf(stderr, "Window (%x) title [ %s ] \t", w, uprop );
	fprintf(stderr, "xDnD aware : %s\n", xdnd_window_is_aware ( w ) ? "TRUE" : "FALSE" );

	return ( NULL ) ;
	
}


void dump_motion(XEvent xevent)
{
	fprintf(stderr, "----\n");
	xdnd_get_window_title(xevent.xmotion.root);
	xdnd_get_window_title(xevent.xmotion.window);
	xdnd_get_window_title(xevent.xmotion.subwindow);
	xdnd_get_window_title(xdnd_locate_window(xevent));
	fprintf(stderr, "loc(root) : %d x %d\t", xevent.xmotion.x_root, xevent.xmotion.y_root) ;
	fprintf(stderr, "loc(win ) : %d x %d\n", xevent.xmotion.x, xevent.xmotion.y) ;
}


void dump_atom ( Atom p_atom)
{
	fprintf(stderr, "Atom name = %s\n", XGetAtomName ( m_display, p_atom ) ) ;
}



void dump_action ( MCDragAction p_action ) 
{
	switch(p_action)
	{
		case DRAG_ACTION_NONE:
			fprintf(stderr, "Drag action = NONE\n");
		break ;
		case DRAG_ACTION_COPY:
			fprintf(stderr, "Drag action = COPY\n");
		break ;
		case DRAG_ACTION_MOVE:
			fprintf(stderr, "Drag action = MOVE\n");
		break ;
		case DRAG_ACTION_LINK:
			fprintf(stderr, "Drag action = LINK\n");
		break ;
	}
}

		
