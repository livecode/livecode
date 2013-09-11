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

//
// X ScreenDC display specific functions
//
#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "card.h"
#include "field.h"
#include "sellst.h"
#include "util.h"
#include "execpt.h"
#include "debug.h"
#include "osspec.h"

#include "globals.h"
#include "unicode.h"

#include "lnxdc.h"
#include "lnxgtkthemedrawing.h"
#include "lnxtransfer.h"

#include "resolution.h"

#define XK_Window_L 0xFF6C
#define XK_Window_R 0xFF6D

#define SELECTIONNAME "MCConvertDestination"

extern int UTF8ToUnicode(const char * lpSrcStr, int cchSrc,
                  uint2 * lpDestStr, int cchDest);

int xerror(Display *dpy, XErrorEvent *ev);
void xdnd_target_event_loop ( XEvent xevent ) ;
extern Atom XdndEnter ;

Boolean tripleclick = False;

typedef struct _WM_STATE
{
	uint4 state;
	uint4 windowid;
}
WM_STATE;

Atom MCstateatom;
Atom MCprotocolatom;
Atom MCtakefocusatom;
Atom MCdeletewindowatom;
Atom MCmwmmessageatom;
Atom MCmwmhintsatom;
Atom MColwinatom;
Atom MColwtotheratom;
Atom MColwtpropatom;
Atom MColadecoratom;
Atom MColddecoratom;
Atom MColresizeatom;
Atom MColheaderatom;
Atom MColcloseatom;
Atom MClayeratom;
Atom MCclipboardatom;

Atom MCworkareaatom;
Atom MCstrutpartialatom;
Atom MCclientlistatom;

static int shmopcode;
static uint4 clicktime;
static Boolean dragclick;



void MCScreenDC::initatoms()
{
	XSetErrorHandler(xerror);
	XSync( dpy, false ) ;
	
	int ev1, ev2;
	int major, minor;
	Bool pix;
	MCshmpix = False ;
	if (!MCshmoff )
		if ( XQueryExtension(dpy, "MIT-SHM", &shmopcode, &ev1, &ev2))
	        if ( XShmQueryVersion(dpy, &major, &minor, &pix))
	{
		MCshm = True;
		MCshmpix = pix; // For shared memory pixmap's we need this to be true as well...
	}

	create_stipple();

	selectionatom = XInternAtom(dpy, SELECTIONNAME, False);
	
	MCprotocolatom = XInternAtom(dpy, "WM_PROTOCOLS", False);
	MCtakefocusatom = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	MCdeletewindowatom = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	MCstateatom = XInternAtom(dpy, "WM_STATE", False);
	MCmwmmessageatom = XInternAtom(dpy, "_MOTIF_WM_MESSAGES", True);
	MCmwmhintsatom = XInternAtom(dpy, "_MOTIF_WM_HINTS", True);
	
#ifdef OLWM
	MColwinatom = XInternAtom(dpy, "_OL_WIN_ATTR" , True);
	MColwtotheratom = XInternAtom(dpy, "_OL_WT_OTHER", True);
	MColwtpropatom = XInternAtom(dpy, "_OL_WT_PROP", True);
	MColadecoratom = XInternAtom(dpy, "_OL_DECOR_ADD", True);
	MColddecoratom = XInternAtom(dpy, "_OL_DECOR_DEL", True);
	MColresizeatom = XInternAtom(dpy, "_OL_DECOR_RESIZE", True);
	MColheaderatom = XInternAtom(dpy, "_OL_DECOR_HEADER", True);
	MColcloseatom = XInternAtom(dpy, "_OL_DECOR_CLOSE", True);
#endif
	
	MClayeratom = XInternAtom(dpy, "_WIN_LAYER", False);
	MCclipboardatom = XInternAtom(dpy, "CLIPBOARD", False);
	
	MCclientlistatom = XInternAtom(dpy, "_NET_CLIENT_LIST", True);
	MCstrutpartialatom = XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", True);
	MCworkareaatom = XInternAtom(dpy, "_NET_WORKAREA", True);
}

void MCScreenDC::setupcolors()
{
	ncolors = MCU_min(vis->colormap_size, MAX_CELLS);
	colors = new MCColor[ncolors];
	colornames = new char *[ncolors];
	allocs = new int2[ncolors];
	int2 i;
	for (i = 0 ; i < ncolors ; i++)
	{
		colors[i].flags = DoRed | DoGreen | DoBlue;
		colors[i].pixel = i;
		colornames[i] = NULL;
		allocs[i] = 0;
	}
}



uint2 MCScreenDC::getscreen()
{
	return vis->screen;
}

Colormap MCScreenDC::getcmap()
{

	return cmap;
}

Visual *MCScreenDC::getvisual()
{
	return vis->visual;
}

uint2 MCScreenDC::getbitorder()
{
	return BitmapBitOrder(dpy);
}

uint2 MCScreenDC::getbyteorder()
{
	return ImageByteOrder(dpy);
}

uint2 MCScreenDC::getunit()
{
	return BitmapUnit(dpy);
}

KeySym MCScreenDC::translatekeysym(KeySym sym, uint4 keycode)
{
	switch (sym)
	{
	case 0:
		switch (keycode)
		{
		case 115:
			return XK_Window_L;
		case 116:
			return XK_Window_R;
		case 117:

			return XK_Menu;
		}
		break;
	case XK_L4:
	case XK_Undo:
		return XK_osfUndo;
	case XK_L10:
	case XK_apCut:
	case XK_SunCut:
		return XK_osfCut;
	case XK_L6:
	case XK_apCopy:
	case XK_SunCopy:
		return XK_osfCopy;
	case XK_L8:
	case XK_apPaste:
	case XK_SunPaste:
		return XK_osfPaste;
	case XK_Help:
		return XK_osfHelp;
	case XK_osfInsert:
	case XK_hpInsertChar:
		return XK_Insert;
	case XK_osfDelete:
	case XK_hpDeleteChar:
		return XK_Delete;
	case 0x1000FF74: // HP shift-tab keysysm
		return XK_Tab;
	}
	return sym;
}

static Boolean isKeyPressed(char *km, uint1 keycode)
{
	return (km[keycode >> 3] >> (keycode & 7)) & 1;
}

void MCScreenDC::getkeysdown(MCExecPoint &ep)
{
	char kstring[U4L];
	char km[32];
	ep.clear();
	MCmodifierstate = querymods();
	XQueryKeymap(dpy, km);
	bool first = true;
	uint2 i;
	KeySym ksym;
	for (i = 0; i < 256; i++)
	{
		if (isKeyPressed(km, i))
		{
			ksym = i;
			if (MCmodifierstate & MS_SHIFT || MCmodifierstate & MS_CAPS_LOCK)
				ksym = XKeycodeToKeysym(dpy, i, 1);
			else
				ksym  = XKeycodeToKeysym(dpy, i, 0);
			if (ksym > 0)
			{
				ep.concatuint(ksym, EC_COMMA, first);
				first = false;
			}
		}
	}
}

void MCScreenDC::create_stipple()
{
	graystipple = XCreatePixmap(dpy, getroot(), 32, 32, 1);
	gc1 = XCreateGC(dpy, graystipple, 0, NULL);
	XSetGraphicsExposures(dpy, gc1, False);
	XSetForeground(dpy, gc1, 1);
	XSetBackground(dpy, gc1, 0);
}

void MCScreenDC::setmods(uint2 state, KeySym sym,
                         uint2 button, Boolean release)
{
	if (lockmods)
		return;
	if (button)
		if (release)
			MCbuttonstate = state >> 8 & 0x1F & ~(0x1L << button - 1);
		else
			MCbuttonstate = state >> 8 & 0x1F | (0x1L << button - 1);
	else
		MCbuttonstate = state >> 8 & 0x1F;
	if (sym >= XK_Shift_L && sym <= XK_Hyper_R)
		MCmodifierstate = querymods();
	else
	{
		state &= 0xFF;
		MCmodifierstate = state >> 1 & 0xFFFE | state & 0x1;
		if (state & 2)
			MCmodifierstate |= MS_CAPS_LOCK;
	}
}

//XDND
bool xdnd_interested_in_event ( XEvent xevent ) ; 
char * xdnd_get_window_title ( Window w ) ;
unsigned int keyval_to_unicode (unsigned int keyval);


unsigned int utf32_to_utf16(unsigned int p_codepoint, unsigned short *p_buffer);

Boolean MCScreenDC::handle(Boolean dispatch, Boolean anyevent,
                           Boolean &abort, Boolean &reset)
{
	XEvent event;
	char buffer[XLOOKUPSTRING_SIZE];
	KeySym keysym;
	Boolean handled = False;
	XAnyEvent *xanyevent =  (XAnyEvent *)&event;
	XFocusChangeEvent *fievent = (XFocusChangeEvent *)&event;
	XFocusChangeEvent *foevent = (XFocusChangeEvent *)&event;
	XKeyEvent *kpevent = (XKeyEvent *)&event;
	XKeyEvent *krevent = (XKeyEvent *)&event;
	XCrossingEvent *cevent = (XCrossingEvent *)&event;
	XMotionEvent *mevent = (XMotionEvent *)&event;
	XButtonEvent *bpevent = (XButtonEvent *)&event;
	XButtonEvent *brevent = (XButtonEvent *)&event;
	XPropertyEvent *pevent = (XPropertyEvent *)&event;
	XConfigureEvent *cnevent = (XConfigureEvent *)&event;
	XDestroyWindowEvent *dwevent = (XDestroyWindowEvent *)&event;
	XReparentEvent *rnevent = (XReparentEvent *)&event;
	XClientMessageEvent *cmevent = (XClientMessageEvent *)&event;
	XSelectionClearEvent *scevent = (XSelectionClearEvent *)&event;
	XSelectionEvent *snevent = (XSelectionEvent *)&event;
	XSelectionRequestEvent *srevent = (XSelectionRequestEvent *)&event;
	XMappingEvent *mnevent = (XMappingEvent *)&event;

	abort = reset = False;
	while (dispatch && pendingevents != NULL || XPending(dpy))
	{
		Boolean live;
		if (dispatch && pendingevents != NULL)
		{
			event = pendingevents->event;
			MCEventnode *tptr = (MCEventnode *)pendingevents->remove
			                    (pendingevents);
			delete tptr;
			live = False;
		}
		else
		{
			if (!XPending(dpy))
				break;
			XNextEvent(dpy, &event);
			live = True;
		}

		switch (event.type)
		{
		case Expose:
		case GraphicsExpose:
			XPutBackEvent(dpy, &event);
			expose();
			break;
		case NoExpose:
			break;
		case FocusIn:
			
				// If the application does not currently have focus, then as we are getting this event
				// we can assume that it now DOES have focus.
			if ( !m_application_has_focus)
			{
				m_application_has_focus = true ;
				hidebackdrop(false);
				if (MCdefaultstackptr != NULL)
					MCdefaultstackptr->getcard()->message(MCM_resume);
			}

			
			if (dispatch)
			{
				if (fievent->window != MCtracewindow)
					MCdispatcher->wkfocus(fievent->window);
			}
			else
			{
				MCEventnode *tptr = new MCEventnode(event);
				tptr->appendto(pendingevents);
			}
			handled = True;
			break;
		case FocusOut:
				
				// Try and work out if this FocusOut message means one of our windows has lost focus
				// to another of our windows, or if our whole application has lost focus.
			bool t_lostfocus ;
			Window t_return_window ;
			int t_return_revert_window ;

			if ( m_application_has_focus)
			{
				// Get the window that focus has been turned over to.
				XGetInputFocus ( MCdpy, &t_return_window, &t_return_revert_window );				
				t_lostfocus = ( MCdispatcher -> findstackd(t_return_window) == NULL) ;

				// Test to see if the focus has actually changed to the backdrop. As the backdrop is not 
				// associated with any stack, the backdrop will not be found by findstackd() above.
				if ( t_return_window == backdrop )
					t_lostfocus = false ;
				
				if ( t_lostfocus )
				{
					m_application_has_focus = false ;
					hidebackdrop(true) ;
					if (MCdefaultstackptr != NULL)
						MCdefaultstackptr->getcard()->message(MCM_suspend);
				}
			}
			
				
			if (dispatch)
			{
				if (foevent->window != MCtracewindow)
					MCdispatcher->wkunfocus(foevent->window);
			}
			else
			{
				MCEventnode *tptr = new MCEventnode(event);
				tptr->appendto(pendingevents);
			}
			handled = True;
			break;
		case KeyPress:
			{
				int t_len ;
				MCExecPoint utf_ep ;
				uint4 t_unicode ;
					
		
				
				t_len = XLookupString(kpevent, buffer, XLOOKUPSTRING_SIZE, &keysym, NULL) ;
				t_unicode = keyval_to_unicode ( keysym ) ;
			
				if (t_unicode != 0)
				{
					uint1 t_char;
					unsigned short t_utf16_buffer[2];
					uint4 t_utf16_length;
					t_utf16_length = utf32_to_utf16(t_unicode, t_utf16_buffer);
					if (MCUnicodeMapToNative(t_utf16_buffer, t_utf16_length, t_char))
					{
						buffer[0] = t_char;
						buffer[1] = 0;
						t_len = 1;
					}
					else
					{
						if (MCactivefield)
						{
							MCactivefield -> finsertnew(FT_IMEINSERT, MCString((char*)t_utf16_buffer, t_utf16_length * 2), LCH_UNICODE, true);
							break;
						}
					}
				}
				else
					buffer[1] = 0, t_len = 1;
				
				
				if (t_len == 0)
					buffer[0] = 0;
								
				keysym = translatekeysym(keysym, kpevent->keycode);
				setmods(kpevent->state, keysym, 0, False);
				if (MCmodifierstate & MS_CONTROL)
					if (keysym == XK_Break || keysym == '.')
					{
						if (MCallowinterrupts && !MCdefaultstackptr->cantabort())
							abort = True;
						else
							MCinterrupt = True;
					}
				if (dispatch)
				{
					if (kpevent->window != MCtracewindow)
					{
						MCeventtime = kpevent->time;
						MCdispatcher->wkdown(kpevent->window, buffer, keysym);
						reset = True;
					}
				}
				else
				{
					MCEventnode *tptr = new MCEventnode(event);
					tptr->appendto(pendingevents);
				}
				handled = True;
				break;
			}
		case KeyRelease:
			buffer[0] = buffer[1] = 0;
			XLookupString(krevent, buffer, XLOOKUPSTRING_SIZE, &keysym, NULL);
			keysym = translatekeysym(keysym, krevent->keycode);
			setmods(krevent->state, keysym, 0, False);
			if (dispatch)
			{
				if (krevent->window != MCtracewindow)
				{
					MCeventtime = krevent->time;
					MCdispatcher->wkup(krevent->window, buffer, keysym);
					reset = True;
				}
			}
			else
			{
				MCEventnode *tptr = new MCEventnode(event);
				tptr->appendto(pendingevents);
			}
			handled = True;
			break;
		case EnterNotify:
		case LeaveNotify:
			if (event.type == EnterNotify)
			{
				MCmousestackptr = MCdispatcher->findstackd(cevent->window);
				if (MCmousestackptr != NULL)
					MCmousestackptr->resetcursor(True);
			}
			else
				MCmousestackptr = NULL;
			if (dispatch)
			{
				if (cevent->window != MCtracewindow)
					if (event.type == EnterNotify)
					{
						if (MCmousestackptr != NULL)
						{
							MCdispatcher->enter(cevent->window);
							Window root, child;
							int rx, ry, wx, wy;
							unsigned int state;
							XQueryPointer(dpy, cevent->window, &root, &child, &rx, &ry,
							              &wx, &wy, &state);
							MCdispatcher->wmfocus(cevent->window, wx, wy);
						}
					}
					else
						MCdispatcher->wmunfocus(cevent->window);
			}
			else
			{
				MCEventnode *tptr = new MCEventnode(event);
				tptr->appendto(pendingevents);
			}
			handled = True;
			break;
		case MotionNotify:
		{
			//XDND
			// Do this so we can store the last message time stamp, which is needed later for xDnD protocol.
			xdnd_interested_in_event ( event ) ;
			
			if (live)
				while (XCheckTypedWindowEvent(dpy, mevent->window,
				                              MotionNotify, &event))
					;
			setmods(mevent->state, 0, 0, False);
			
			// IM-2013-08-12: [[ ResIndependence ]] Scale mouse coords to user space
			MCGFloat t_scale;
			t_scale = MCResGetDeviceScale();
			
			MCPoint t_mouseloc;
			t_mouseloc.x = mevent->x / t_scale;
			t_mouseloc.y = mevent->y / t_scale;
			
			MCmousex = t_mouseloc.x;
			MCmousey = t_mouseloc.y;
			MCmousestackptr = MCdispatcher->findstackd(mevent->window);
			
			//XDND
			if ( !dragclick && (MCU_abs(MCmousex - MCclicklocx) > 4 || MCU_abs(MCmousey - MCclicklocy) > 4) && MCbuttonstate != 0  ) 
			{
				last_window = mevent -> window ; 
				dragclick = true ;
				MCdispatcher -> wmdrag(mevent->window);
				
			}
			
			if (dispatch)
			{
				if (mevent->window != MCtracewindow)
				{
					MCeventtime = mevent->time;
					MCdispatcher->wmfocus(mevent->window,
					                      (int2)t_mouseloc.x, (int2)t_mouseloc.y);
				}
			}
			else
			{
				MCEventnode *tptr = new MCEventnode(event);
				tptr->appendto(pendingevents);
			}
			handled = True;
			break;
		}
		case ButtonPress:
		{
			dragclick = false ;
			
			setmods(bpevent->state, 0, bpevent->button, False);
			
			// IM-2013-08-12: [[ ResIndependence ]] Scale mouse coords to user space
			MCGFloat t_scale;
			t_scale = MCResGetDeviceScale();
			
			MCGPoint t_clickloc;
			t_clickloc.x = brevent->x / t_scale;
			t_clickloc.y = brevent->y / t_scale;
			
			MCGPoint t_oldclickloc;
			t_oldclickloc.x = MCclicklocx;
			t_oldclickloc.y = MCclicklocy;
			
			MCclicklocx = t_clickloc.x;
			MCclicklocy = t_clickloc.y;
			
			MCclickstackptr = MCmousestackptr;
			if (dispatch)
			{
				if (bpevent->window != MCtracewindow)
				{					MCeventtime = bpevent->time;
					if (MCmousestackptr != NULL
					        && (bpevent->button == Button4 || bpevent->button == Button5))
					{
						MCObject *mfocused = MCmousestackptr->getcard()->getmfocused();
						if (mfocused == NULL)
							mfocused = MCmousestackptr -> getcard();
						if (mfocused != NULL)
						{
							if (bpevent->button == Button4)
								mfocused->kdown("", XK_WheelDown);
							else
								mfocused->kdown("", XK_WheelUp);
						}
					}
					else
					{
						uint2 delay;
						if (bpevent->time < clicktime) /* uint4 wrap */
							delay = (bpevent->time + 10000) - (clicktime + 10000);
						else
							delay = bpevent->time - clicktime;
						clicktime = bpevent->time;
						if (backdrop != DNULL && brevent->window == backdrop)
							MCdefaultstackptr->getcard()->message_with_args(MCM_mouse_down_in_backdrop,
							                                      brevent->button);
						else
						{
							if (delay < MCdoubletime
							        && MCU_abs(t_oldclickloc.x - t_clickloc.x) < MCdoubledelta
							        && MCU_abs(t_oldclickloc.y - t_clickloc.x) < MCdoubledelta)
							{
								if (doubleclick)
								{
									doubleclick = False;
									tripleclick = True;
									MCdispatcher->wmdown(bpevent->window, bpevent->button);
								}
								else
								{
									doubleclick = True;
									MCdispatcher->wdoubledown(bpevent->window, bpevent->button);
								}
							}
							else
							{
								tripleclick = doubleclick = False;
								MCdispatcher->wmfocus(bpevent->window,
								                      (int2)t_clickloc.x, (int2)t_clickloc.y);
								MCdispatcher->wmdown(bpevent->window, bpevent->button);
							}
							reset = True;
						}
					}
				}
			}
			else
			{
				MCEventnode *tptr = new MCEventnode(event);
				tptr->appendto(pendingevents);
			}
			handled = True;
			break;
		}
		case ButtonRelease:
			// Discard button4 or 5 release event (button4= ScrollWheel up, button5 = ScrollWheel down)
				
			dragclick = false ;
			
			if ( brevent-> button != Button4 && brevent -> button != Button5 ) 
			{
				setmods(brevent->state, 0, brevent->button, True);
				if (dispatch)
				{



					if (backdrop != DNULL && brevent->window == backdrop)
						MCdefaultstackptr->getcard()->message_with_args(MCM_mouse_up_in_backdrop,
															  brevent->button);
					else
						if (brevent->window != MCtracewindow)
						{
							MCeventtime = brevent->time;
							if (doubleclick)
								MCdispatcher->wdoubleup(brevent->window, brevent->button);
							else
								MCdispatcher->wmup(brevent->window, brevent->button);
							reset = True;
						}
				}
				else
				{
					MCEventnode *tptr = new MCEventnode(event);
					tptr->appendto(pendingevents);
				}
			}
			handled = True;
			break;
		case PropertyNotify:
			MCeventtime = pevent->time;
			if (pevent->state == PropertyNewValue)
				if (pevent->atom == MCstateatom)
				{
					MCStack *target = MCdispatcher->findstackd(pevent->window);
					if (target != NULL)
					{
						Atom type = None;
						int format;
						long unsigned int nitems;
						unsigned long extra;
						WM_STATE *prop;
						XGetWindowProperty(dpy, pevent->window, pevent->atom, 0,
						                   sizeof(WM_STATE), False, AnyPropertyType,
						                   &type, &format, &nitems, &extra,
						                   (unsigned char **)&prop);
						if (type != None)
						{
							switch (prop->state)
							{
							case DontCareState:
#ifndef LINUX

								if (target->getstate(CS_ISOPENING))
								{
									target->setstate(False,  CS_ISOPENING);
									break;
								}
								target->close();
#endif

								break;
							case IconicState:
								target->iconify();
								break;
							case NormalState:
								target->uniconify();
								break;
							default:
								break;
							}
							XFree((char *)prop);
						}
					}
				}
			handled = True;
			break;
		case ConfigureNotify:
			MCdispatcher->configure(cnevent->window);
			break;
		case CirculateNotify:
			break;
		case MapNotify:
			break;
		case DestroyNotify:
			break;
		case GravityNotify:
			break;
		case ReparentNotify:
			// MW-2010-11-29: We don't want to do any reconfig on reparenting as that is only
			//   done by the WM and will just result in our rect becoming screwed.
			//MCdispatcher->configure(rnevent->window);
			break;
		case UnmapNotify:
			break;
		case ClientMessage:
				
			//XDND - If we have an XdndEnter atom, we can push over to the xdnd_target_event_loop() function to handle events .
			last_window = cmevent->window ;
			if ( xdnd_interested_in_event ( event ) )
				xdnd_target_event_loop ( event ) ;

				
			if (cmevent->message_type == MCprotocolatom)
			{
				if (cmevent->data.l[0] == (long)MCtakefocusatom)
					MCdispatcher->kfocusset(cmevent->window);
				else
					if (cmevent->data.l[0] == (long)MCdeletewindowatom)
					{
						Window dw = cmevent->window;
						Window root, child;

						int rx, ry, wx, wy;
						unsigned int state;
						XGrabPointer(dpy, dw, False,
						             PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
						             GrabModeAsync, GrabModeAsync, None,
						             None, CurrentTime);
						XQueryPointer(dpy, getroot(), &root, &child, &rx, &ry,
						              &wx, &wy, &state);
						// if mouse is down, wait for it to go up
						while (state & (Button1Mask | Button2Mask | Button3Mask))
						{
							XQueryPointer(dpy, getroot(), &root, &child, &rx, &ry,
							              &wx, &wy, &state);
						}
						XUngrabPointer(dpy, CurrentTime);
						MCdispatcher->wclose(dw);
					}
			}
			handled = True;
			break;
		case SelectionClear:
			if (scevent->time != MCeventtime)
				if (scevent->selection == XA_PRIMARY)
				{
					if (MCactivefield != NULL)
						MCactivefield->unselect(False, False);
					ownselection = False;
				}
			break;
		case SelectionNotify:
			//XDND
			if ( xdnd_interested_in_event ( event ) )
				xdnd_target_event_loop ( event ) ;
			break;
		case SelectionRequest:
			
			XSelectionEvent sendevent;
			sendevent.type = SelectionNotify;
			sendevent.send_event = True;
			sendevent.display = srevent->display;
			sendevent.requestor = srevent->requestor;
			sendevent.selection = srevent->selection;
			sendevent.time = srevent->time;
			sendevent.target = srevent->target;


#ifdef DEBUG_DND
			fprintf(stderr, "I have a request for %s from %x into %s\n", XGetAtomName(dpy, srevent->target),  srevent -> requestor, XGetAtomName ( dpy, srevent -> property) );
			xdnd_get_window_title(srevent->requestor);
#endif

			if ( srevent -> target == XInternAtom(dpy,"TIMESTAMP", false))
			{
				
				
			}

			
			if ( srevent -> target == XA_TARGETS )
			{
					
				uint4 t_count ;
				Atom *t_atoms ;
				t_atoms = m_Clipboard_store -> QueryAtoms ( t_count );
				
				if ( t_atoms != NULL)
				{
#ifdef DEBUG_DND
					fprintf(stderr, "Responding with : \n");
					for ( uint4 a = 0; a < t_count; a++)
						fprintf(stderr, "\t%s\n", XGetAtomName (dpy, t_atoms[a]));
#endif
					
					
					XChangeProperty(dpy, srevent -> requestor, srevent -> property,
					                srevent->target, 32, PropModeReplace,
					                (const unsigned char *)t_atoms,
					                t_count);

					XSendEvent(dpy, srevent -> requestor, False, 0, (XEvent *)&sendevent);
					
					free(t_atoms) ;

				}
			}
			else 
			{
				
				sendevent.property = None ;
				// If I don't own the clipboard at this point, then something has gone wrong	
				if ( ownsclipboard() ) 
				{
					MCSharedString * t_data; 
					if (m_Clipboard_store -> Fetch(  new MCMIMEtype(dpy, srevent -> target), t_data, None, None, DNULL, DNULL, MCeventtime ))
					{
						XChangeProperty(dpy, srevent -> requestor, srevent -> property,
					                srevent -> target, 8, PropModeReplace,
					                (const unsigned char *)t_data -> Get() . getstring(),
					                t_data -> Get() . getlength());
						
						if (srevent->property != None)
							sendevent.property = srevent->property;
						else
							sendevent.property = srevent -> target;

					}
				}

				XSendEvent (dpy, sendevent.requestor, False, 0, (XEvent *)&sendevent );
			}
			break;
		case MappingNotify:
			if (mnevent->request == MappingKeyboard)
				XRefreshKeyboardMapping(mnevent);
			break;
		default:
			break;

		}
		
#ifdef _LINUX_DESKTOP
		if (!handled && MCcurtheme)
			moz_gtk_handle_event(event);
#endif

		if (anyevent && handled)
			break;
	}
	return handled;
}

void MCScreenDC::waitmessage(Window w, int event_type)
{
	real8 endtime = MCS_time() + CONFIGURE_WAIT;
	do
	{
		XEvent event;
		if (XCheckTypedWindowEvent(dpy, w, event_type, &event))
			break;
		XSync(dpy, False);

	}
	while (MCS_time() < endtime);
}

int xerror(Display *dpy, XErrorEvent *ev)
{	
	if (ev->request_code == shmopcode)
		MCshm = MCvcshm = False;
	else
	{
		if (ev->request_code != 88 && ev->request_code != 42)
		{
			if (ev->request_code == 53)
				fprintf(stderr,
						"%s: XCreatePixmap failed, X server is out of memory --- oops\n", MCcmd);
			else
			{
				// SB-2013-05-30: [[ XErrorMsg ]] Added 'XGetErrorText()' for more helpful error message.
				char msg[80];
	 			XGetErrorText(dpy, ev->error_code, msg, 80);
				fprintf(stderr,
						"%s: X error major code %d minor code %d error was %d : %s\n",
						MCcmd, ev->request_code, ev->minor_code, ev->error_code, msg);
			}
		}
	}
	return 0;
}
