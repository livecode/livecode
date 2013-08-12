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

#ifndef	XDC_H
#define	XDC_H

#include "uidc.h"
#include "lnxflst.h"
#include "lnxtransfer.h"

#define XLOOKUPSTRING_SIZE 32
#define CONFIGURE_WAIT 0.5
#define FOCUS_WAIT 0.5
#define REFRESH_RATE 10.0
#define SELECTION_WAIT 5000

#define MAX_CELLS 4096
#define COL_QUANT 12

#define fixmaskrop(a) (a)
#define fixmaskcolor(a) (a)


class MCEventnode : public MCDLlist
{
public:
	XEvent event;
	MCEventnode(XEvent &e)
	{
		event = e;
	}
	MCEventnode *next()
	{
		return (MCEventnode *)MCDLlist::next();
	}
	MCEventnode *prev()
	{
		return (MCEventnode *)MCDLlist::prev();
	}
	void totop(MCEventnode *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCEventnode *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCEventnode *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCEventnode *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCEventnode *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCEventnode *remove
	(MCEventnode *&list)
	{
		return (MCEventnode *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

extern Atom MCstateatom;
extern Atom MCprotocolatom;
extern Atom MCtakefocusatom;
extern Atom MCdeletewindowatom;
extern Atom MCmwmmessageatom;
extern Atom MCmwmhintsatom;
extern Atom MColwinatom;
extern Atom MColwtotheratom;
extern Atom MColwtpropatom;
extern Atom MColadecoratom;
extern Atom MColddecoratom;
extern Atom MColresizeatom;
extern Atom MColheaderatom;
extern Atom MColcloseatom;
extern Atom MClayeratom;
extern Atom MCclipboardatom;

extern Atom MCclientlistatom;
extern Atom MCstrutpartialatom;
extern Atom MCworkareaatom;

extern Boolean tripleclick;

class MCScreenDC : public MCUIDC
{

	GC gc;		// This is the GC in "Native" (i.e. actual screen) depth
	GC gc1;		// This is the GC in 1-bit depth used for image masks
	
	bool m_application_has_focus ; // This allows us to track if the application is at the front.
	
	Atom statusatom;
	Atom selectionatom;

	uint2 destdepth; //
	
	Boolean opened;
	
	Window Xwin; //
	Window NULLWindow ;
	
	Pixmap graystipple;
	MCEventnode *pendingevents;
	
	Boolean ownselection;
	MCString selectiontext;
	Boolean doubleclick;

	Colormap cmap;			// Native colourmap
	Colormap cmap32 ;		// 32-bit colourmap

	XVisualInfo *vis;		// Native visual
	XVisualInfo *vis32 ;	// 32-bit visual

	bool backdrop_active;
	bool backdrop_hard;
	Window backdrop;
	MCColor backdropcolor;

	Window last_window ; 	//XDND - Used for the moment to shunt the ID
	
	bool m_has_native_theme;
	bool m_has_native_color_dialogs;
	bool m_has_native_print_dialogs;
	bool m_has_native_file_dialogs;
	
	MCXTransferStore * m_DND_store ;
	MCXTransferStore * m_Clipboard_store ;
	MCXTransferStore * m_Selection_store ;

public:
	

	static MCDisplay *s_monitor_displays;
	static uint4 s_monitor_count;
	
	bool getdisplays_init;
	bool Xinerama_available; 
	
	char * syslocale ;
	
	Display *dpy;
	Boolean has_composite_wm ;
	Drawable dest; //

	char *displayname;
	uint4 savedpixel; // Move into per-context

	MCScreenDC();
	virtual ~MCScreenDC();

	virtual bool hasfeature(MCPlatformFeature p_feature);

	virtual int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false);
	
	virtual uint2 getdepth();
	virtual uint2 getrealdepth(void);
	
	virtual void setstatus(const char *status);
	virtual Boolean setdest(Drawable d, uint2 depth);
	virtual Drawable getdest();
	virtual Boolean open();
	virtual Boolean close(Boolean force);
	virtual const char *getdisplayname();
	virtual void resetcursors();
	virtual void setcursor(Window w, MCCursorRef c);
	virtual void grabpointer(Window w);
	virtual void ungrabpointer();
	virtual uint2 getwidthmm();
	virtual uint2 getheightmm();
	virtual uint2 getmaxpoints();
	virtual uint2 getvclass();
	virtual void openwindow(Window window, Boolean override);
	virtual void closewindow(Window window);
	virtual void destroywindow(Window &window);
	virtual void raisewindow(Window window);
	virtual void iconifywindow(Window window);
	virtual void uniconifywindow(Window window);
	virtual void setname(Window window, const char *newname);
	virtual void setcmap(MCStack *sptr);
	virtual void sync(Window w);
	virtual void flush(Window w);
	virtual void beep();
	virtual void setinputfocus(Window window);
	virtual void freepixmap(Pixmap &pixmap);
	virtual Pixmap createpixmap(uint2 width, uint2 height,
	                            uint2 depth, Boolean purge);
	virtual Boolean getpixmapgeometry(Pixmap p, uint2 &w, uint2 &h, uint2 &d);

	virtual void setgraphicsexposures(Boolean on, MCStack *sptr);
	virtual void copyarea(Drawable source, Drawable dest, int2 depth,
	                      int2 sx, int2 sy, uint2 sw, uint2 sh,
	                      int2 dx, int2 dy, uint4 rop);
	virtual MCBitmap *createimage(uint2 depth, uint2 width, uint2 height,
	                              Boolean set
		                              , uint1 value,
		                              Boolean shm, Boolean forceZ);
	virtual void destroyimage(MCBitmap *image);
	virtual void putimage(Drawable dest, MCBitmap *source, int2 sx, int2 sy,
	                      int2 dx, int2 dy, uint2 w, uint2 h);
	virtual XImage *getimage(Drawable pm, int2 x, int2 y,
	                           uint2 w, uint2 h, Boolean shm = False);
	virtual void flipimage(XImage *image, int2 byte_order, int2 bit_order);
	
	virtual MCColorTransformRef createcolortransform(const MCColorSpaceInfo& info);
	virtual void destroycolortransform(MCColorTransformRef transform);
	virtual bool transformimagecolors(MCColorTransformRef transform, MCImageBitmap *image);

	MCCursorRef createcursor(MCImageBitmap *p_image, int2 p_xhot, int2 p_yhot);
	virtual void freecursor(MCCursorRef c);

	virtual void setfunction(uint4 rop);
	virtual uint4 dtouint4(Drawable d);
	virtual Boolean uint4towindow(uint4, Window &w);
	virtual void getbeep(uint4 property, MCExecPoint &ep);
	virtual void setbeep(uint4 property, int4 beep);
	virtual void getvendorstring(MCExecPoint &ep);
	virtual uint2 getpad();
	virtual Window getroot();
	virtual MCImageBitmap *snapshot(MCRectangle &r, uint4 window,
	                           const char *displayname);
	
	
	virtual void createbackdrop(const char *color);
	virtual void destroybackdrop();
	
	void createbackdrop_window(void);
	Window get_backdrop(void) { return backdrop; };
	void hidebackdrop(bool p_hide);

	virtual void enablebackdrop(bool p_hard = false);
	virtual void disablebackdrop(bool p_hard = false);
	virtual void configurebackdrop(const MCColor& p_colour, MCGImageRef p_pattern, MCImage *p_badge);
	virtual void assignbackdrop(Window_mode p_mode, Window p_window);

	// IM-2013-08-12: [[ ResIndependence ]] refactored methods that return device coordinates
	virtual uint16_t device_getwidth(void);
	virtual uint16_t device_getheight(void);
	virtual bool device_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count);
	virtual bool device_getwindowgeometry(Window w, MCRectangle &drect);
	virtual void device_boundrect(MCRectangle &rect, Boolean title, Window_mode m);
	virtual void device_querymouse(int16_t &r_x, int16_t &r_y);
	virtual void device_setmouse(int16_t p_x, int16_t p_y);
	
	virtual void expose();
	virtual Boolean abortkey();
	virtual void waitconfigure(Window w);
	virtual void waitreparent(Window w);
	virtual void waitfocus();
	virtual uint2 querymods();
	virtual Boolean getmouse(uint2 button, Boolean& r_abort);
	virtual Boolean getmouseclick(uint2 button, Boolean& r_abort);
	virtual Boolean wait(real8 duration, Boolean dispatch, Boolean anyevent);
	virtual void flushevents(uint2 e);
	virtual Boolean istripleclick();
	
	
	virtual MCTransferType querydragdata(void);
	virtual MCSharedString *fetchdragdata(void);
	
	virtual MCDragAction dodragdrop(MCPasteboard *p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint *p_image_offset);

	
	// Clipboard and selection interface
	virtual bool ownsselection(void); 
	virtual bool setselection(MCPasteboard *p_pasteboard);
	virtual MCPasteboard *getselection(void);
	
	virtual bool ownsclipboard(void);
	virtual bool setclipboard(MCPasteboard *p_pasteboard);
	virtual MCPasteboard *getclipboard(void);
	virtual void flushclipboard(void);

	void initatoms();
	void setupcolors();
	uint2 getscreen();
	Colormap getcmap();
	Visual *getvisual();
	uint2 getbitorder();
	uint2 getbyteorder();
	uint2 getunit();
	KeySym translatekeysym(KeySym sym, uint4 keycode);
	virtual void getkeysdown(MCExecPoint &ep);
	void create_stipple();
	void setmods(uint2 state, KeySym sym, uint2 button, Boolean release);
	Boolean handle(Boolean dispatch, Boolean anyevent,
	               Boolean &abort, Boolean &reset);
	void waitmessage(Window w, int event_type);
	
	bool apply_workarea();
	bool apply_partial_struts();
		
	Display *getDisplay() { return dpy; };
	
	Window  GetNullWindow (void ) { return NULLWindow ; } ;
	
	// Some utility functions used for ARGB windows and composite window manager stuff.
	Bool is_composite_wm ( int screen_id ) ;
	
	Atom make_atom ( char * p_atom_name ) ;
	bool check_clipboard_manager(void) ;
	void make_clipboard_persistant(void) ;
	
	
	// Public acccess functions get get GC's, visuals and cmaps for different depths
	GC getgc(void) ; 
	GC getgc1 (void) { return gc1; } ;
	GC getgcnative (void) { return gc; } ;
	
	XVisualInfo *getvisnative ( void ) { return vis ; } ;
	XVisualInfo *getvis32 ( void ) { return vis32; } ;
	
	Colormap getcmapnative ( void ) { return cmap ; } ;
	Colormap getcmap32 ( void ) { return cmap32 ; } ; 

	
	virtual void listprinters(MCExecPoint& ep);
	virtual MCPrinter *createprinter(void);
	
#ifdef OLD_GRAPHICS
	MCBitmap *regiontomask(MCRegionRef r, int32_t w, int32_t h);
#endif

};
#endif
