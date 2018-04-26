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

#ifndef	XDC_H
#define	XDC_H

#include "uidc.h"
#include "lnxflst.h"
#include "lnx-clipboard.h"

#define XLOOKUPSTRING_SIZE 32
#define CONFIGURE_WAIT 0.5
#define FOCUS_WAIT 0.5
#define REFRESH_RATE 10.0
#define SELECTION_WAIT 5000

#define MAX_CELLS 4096
#define COL_QUANT 12

#define fixmaskrop(a) (a)
#define fixmaskcolor(a) (a)

#include <gtk/gtk.h>


class MCEventnode : public MCDLlist
{
public:
	GdkEvent* event;
	MCEventnode(GdkEvent* e)
	{
		event = e;
	}
    ~MCEventnode()
    {
        gdk_event_free(event);
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

/*extern Atom MCstateatom;
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
extern Atom MCclipboardatom;*/

extern Atom MCclientlistatom;
extern Atom MCstrutpartialatom;
extern Atom MCworkareaatom;
extern Atom MCdndselectionatom;

extern Boolean tripleclick;

class MCScreenDC : public MCUIDC
{
	GdkGC* gc = nullptr;		// This is the GC in "Native" (i.e. actual screen) depth
	
	bool m_application_has_focus = false; // This allows us to track if the application is at the front.
	
	Atom statusatom = GDK_NONE;
	Atom selectionatom = GDK_NONE;

	uint2 destdepth = 0; //
	
	Boolean opened = false;
	
	Window Xwin = None; //
	Window NULLWindow = None;
	
	MCEventnode *pendingevents = nullptr;
	
	Boolean ownselection = False;
	MCString selectiontext;
	Boolean doubleclick = False;

	GdkColormap *cmap = nullptr;			// Native colourmap
	GdkColormap *cmap32 = nullptr;		// 32-bit colourmap

	GdkVisual *vis = nullptr;		// Native visual
	GdkVisual *vis32 = nullptr;	// 32-bit visual

	bool backdrop_active = false;
	bool backdrop_hard = false;
	Window backdrop = None;
	MCColor backdropcolor {0, 0, 0};
	// IM-2014-04-15: [[ Bug 11603 ]] Store converted backdrop pattern pixmap
	Pixmap m_backdrop_pixmap = nullptr;

	Window last_window = None; 	//XDND - Used for the moment to shunt the ID
	
	bool m_has_native_theme = false;
	bool m_has_native_color_dialogs = false;
	bool m_has_native_print_dialogs = false;
	bool m_has_native_file_dialogs = false;
    
    // Set if GTK is available
    bool m_has_gtk = false;
    
    // Input context for IME integration
    GtkIMContext *m_im_context = nullptr;

public:
	
	char * syslocale = nullptr;
	
	GdkDisplay *dpy = nullptr;
	Boolean has_composite_wm = false;
	Drawable dest = None; //

	MCNewAutoNameRef displayname;
	MCNewAutoNameRef vendorname;
	uint4 savedpixel = 0; // Move into per-context

	MCScreenDC();
	virtual ~MCScreenDC();

	virtual bool hasfeature(MCPlatformFeature p_feature);

	virtual uint2 getdepth();
	virtual uint2 getrealdepth(void);
	
	virtual void setstatus(MCStringRef status);
	virtual Drawable getdest();
	virtual Boolean open();
	virtual Boolean close(Boolean force);
	virtual MCNameRef getdisplayname();
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
	virtual void setname(Window window, MCStringRef newname);
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
    
	virtual MCBitmap *createimage(uint2 depth, uint2 width, uint2 height, bool set, uint1 value);
	virtual void destroyimage(MCBitmap *image);
	virtual void putimage(Drawable dest, MCBitmap *source, int2 sx, int2 sy, int2 dx, int2 dy, uint2 w, uint2 h);
	virtual MCBitmap *getimage(Drawable pm, int2 x, int2 y, uint2 w, uint2 h);
	virtual void flipimage(MCBitmap *image, int2 byte_order, int2 bit_order);
	
	virtual MCColorTransformRef createcolortransform(const MCColorSpaceInfo& info);
	virtual void destroycolortransform(MCColorTransformRef transform);
	virtual bool transformimagecolors(MCColorTransformRef transform, MCImageBitmap *image);

	MCCursorRef createcursor(MCImageBitmap *p_image, int2 p_xhot, int2 p_yhot);
	virtual void freecursor(MCCursorRef c);

    virtual void setfunction(uint4 rop);
	virtual uintptr_t dtouint(Drawable d);
    virtual Boolean uinttowindow(uintptr_t, Window &w);
    virtual void getbeep(uint4 property, int4& r_value);
	virtual void setbeep(uint4 property, int4 beep);
	virtual MCNameRef getvendorname(void);
	virtual uint2 getpad();
	virtual Window getroot();

	virtual MCImageBitmap *snapshot(MCRectangle &r, uint4 window, MCStringRef displayname, MCPoint *size);
	
    //virtual void createbackdrop(MCStringRef color);
	void destroybackdrop();
	
	void createbackdrop_window(void);
	Window get_backdrop(void) { return backdrop; };
	void hidebackdrop(bool p_hide);

	virtual void enablebackdrop(bool p_hard = false);
	virtual void disablebackdrop(bool p_hard = false);
	virtual void configurebackdrop(const MCColor& p_colour, MCPatternRef p_pattern, MCImage *p_badge);
	virtual void assignbackdrop(Window_mode p_mode, Window p_window);

	// IM-2014-01-29: [[ HiDPI ]] Update device_* methods to platform-specific logical coord based methods
	virtual uint16_t platform_getwidth(void);
	virtual uint16_t platform_getheight(void);
	virtual bool platform_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count);
	virtual bool platform_getwindowgeometry(Window w, MCRectangle &drect);
	virtual void platform_boundrect(MCRectangle &rect, Boolean title, Window_mode m, Boolean resizable);
	virtual void platform_querymouse(int16_t &r_x, int16_t &r_y);
	virtual void platform_setmouse(int16_t p_x, int16_t p_y);
	
	virtual bool platform_get_display_handle(void *&r_display);

	virtual void *GetNativeWindowHandle(Window p_window);
	
	// IM-2014-01-29: [[ HiDPI ]] Convenience methods to convert logical to screen coords and back
	MCPoint logicaltoscreenpoint(const MCPoint &p_point);
	MCPoint screentologicalpoint(const MCPoint &p_point);
	MCRectangle logicaltoscreenrect(const MCRectangle &p_rect);
	MCRectangle screentologicalrect(const MCRectangle &p_rect);
	
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

    virtual MCDragAction dodragdrop(Window w, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint* p_image_offset);

	void setupcolors();
	GdkScreen* getscreen();
	GdkColormap* getcmap();
	GdkVisual* getvisual();
	KeySym translatekeysym(KeySym sym, uint4 keycode);
	virtual bool getkeysdown(MCListRef& r_list);
	void setmods(guint state, KeySym sym, uint2 button, Boolean release);
	Boolean handle(Boolean dispatch, Boolean anyevent,
	               Boolean &abort, Boolean &reset);
	void waitmessage(Window w, int event_type);
	
	// IM-2014-01-29: [[ HiDPI ]] Apply screen workarea to given MCDisplay array
	bool apply_workarea(MCDisplay *p_displays, uint32_t p_display_count);

	// IM-2014-01-29: [[ HiDPI ]] Apply screen struts to given MCDisplay array
	bool apply_partial_struts(MCDisplay *p_displays, uint32_t p_display_count);
		
	GdkDisplay *getDisplay() { return dpy; };
	
	Window  GetNullWindow (void ) { return NULLWindow ; } ;
	
	// Some utility functions used for ARGB windows and composite window manager stuff.
	Bool is_composite_wm ( int screen_id ) ;
	
	Atom make_atom ( char * p_atom_name ) ;
	bool check_clipboard_manager(void) ;
	void make_clipboard_persistant(void) ;
	
	
	// Public acccess functions get get GC's, visuals and cmaps for different depths
	GdkGC* getgc() { return gc; }

	GdkColormap* getcmapnative ( void ) { return cmap ; } ;
	GdkColormap* getcmap32 ( void ) { return cmap32 ; } ;

	
	virtual bool listprinters(MCStringRef& r_printers);
	virtual MCPrinter *createprinter(void);

    // Processes all outstanding GDK events and adds them to the event queue
    void EnqueueGdkEvents(bool p_may_block = false);
    
    // Searches the event queue for an event that passes the given filter
    typedef bool (*event_filter)(GdkEvent*, void *);
    bool GetFilteredEvent(event_filter, GdkEvent* &r_event, void *, bool p_may_block = false);
    
    // Utility function - maps an X drawing operation to the GDK equivalent
    static GdkFunction XOpToGdkOp(int op);
    
    // Queues an event as a pending event
    void EnqueueEvent(GdkEvent *);
    
    // IME events
    void IME_OnCommit(GtkIMContext*, gchar *p_utf8_string);
    bool IME_OnDeleteSurrounding(GtkIMContext*, gint p_offset, gint p_count);
    void IME_OnPreeditChanged(GtkIMContext*);
    void IME_OnPreeditEnd(GtkIMContext*);
    void IME_OnPreeditStart(GtkIMContext*);
    void IME_OnRetrieveSurrounding(GtkIMContext*);
    
    virtual void clearIME(Window w);
    virtual void configureIME(int32_t x, int32_t y);
	virtual void activateIME(Boolean activate);
	//virtual void closeIME();
    
    virtual bool loadfont(MCStringRef p_path, bool p_globally, void*& r_loaded_font_handle);
    virtual bool unloadfont(MCStringRef p_path, bool p_globally, void *r_loaded_font_handle);

private:
    
    void DnDClientEvent(GdkEvent*);
};
#endif
