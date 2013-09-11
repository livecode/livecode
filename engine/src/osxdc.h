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
// Xlib for graphics output
//
#ifndef	MACDC_H
#define	MACDC_H

#include "uidc.h"
#include "osxflst.h"

#define kHICommandPreferences 'pref'

#define XLOOKUPSTRING_SIZE 32
#define CONFIGURE_WAIT 0.5
#define FOCUS_WAIT 0.5
#define REFRESH_RATE 10.0
#define SELECTION_WAIT 5000
#define THREAD_WAIT 2 // ticks between yields
#define SOCKET_WAIT 0.5 // max secs to wait with open sockets

#define MAX_CELLS 4096
#define COL_QUANT 12

#define mApple  1  //create an "Apple" menu at start time. menu ID starts from 1
//copy from MacWindows.h file.  WindowClass enum defines
#define kOverlayWindowClass 14L // Mac OS X only

#define fixmaskrop(a) (a)
#define fixmaskcolor(a) (a)

enum Mac_platform {
    MP_68K,
    MP_POWERPC
};

//for displaying and building MAC menus, wihich includes hierachical menu

#define MAX_SUBMENU_DEPTH  31    //for Hierachical menus

#define SUB_MENU_START_ID  257   //the ID of the first sub menu. MAC allows from 1 to 235
#define SUB_MENU_LAST_ID 20000
#define MENUIDSIZE 2
#define MENUIDOFFSET 3

#define MAIN_MENU_LAST_ID  32
#define MAIN_MENU_FIRST_ID 2

typedef struct OpaqueScrapRef*          ScrapRef;

typedef struct
{
	MenuHandle mh;
	char *raw;      // item text with Menu Manager special characters
	char *filtered; // filtered item text for user
	uint2 rawlength;
	uint2 filteredlength;
	char mark;
	uint1 modifier;
	uint2 glyph;
	uint2 after; //insert new menu item after this number
}
MenuStructure;

class MCEventnode : public MCDLlist
{
public:
	EventRecord event;  //Mac event record
	MCEventnode(EventRecord &e)
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

typedef struct
{
	uint4 red_mask;
	uint4 green_mask;
	uint4 blue_mask;
}
MCVisualInfo;

enum mac_platform {
    MP_POWER,
    MP_68,
    MP_32S
};

extern pascal OSErr TSMPositionToOffset(const AppleEvent *theAppleEvent,
	                                        AppleEvent *reply, long handlerRefcon);
extern pascal OSErr TSMOffsetToPosition(const AppleEvent *theAppleEvent,
	                                        AppleEvent *reply, long handlerRefcon);
extern pascal OSErr TSMUpdateHandler(const AppleEvent *theAppleEvent,
	                                     AppleEvent *reply, long handlerRefcon);
extern pascal OSErr TSMUnicodeNotFromInputHandler(const AppleEvent *theAE,
	        AppleEvent *reply, long handlerRefcon);
extern pascal OSErr DragTrackingHandler(DragTrackingMessage message,
	                                        WindowPtr theWindow, void *hRefCon,
	                                        DragReference theDrag);
extern pascal OSErr DragReceiveHandler(WindowPtr theWindow, void *hRefCon,
	                                       DragReference theDrag);

extern uint4 MCMacGlyphToKeysym(uint2 glyph);
extern uint2 MCMacKeysymToGlyph(uint4 key);

static inline Rect MCRectToMacRect(const MCRectangle &p_rect)
{
	Rect t_rect;
	t_rect.left = p_rect.x;
	t_rect.top = p_rect.y;
	t_rect.right = p_rect.x + p_rect.width;
	t_rect.bottom = p_rect.y + p_rect.height;
	
	return t_rect;
}

static inline MCRectangle MCMacRectToMCRect(const Rect &p_rect)
{
	MCRectangle t_rect;
	t_rect.x = p_rect.left;
	t_rect.y = p_rect.top;
	t_rect.width = p_rect.right - p_rect.left;
	t_rect.height = p_rect.bottom - p_rect.top;
	
	return t_rect;
}

class MCScreenDC : public MCUIDC
{
	Boolean ownselection;
	uint2 devdepth;
	uint2 linesize;
	MCEventnode *pendingevents;
	MCVisualInfo *vis;
	uint2 beeppitch;
	uint2 beepduration;
	Boolean grabbed;  //mouse is grabbed
	Boolean mdown;  //mouse is down
	Boolean doubleclick;
	Boolean ge;
	Boolean usetemp;
	Boolean lockcolormap;
	Mac_platform platform;
	Window activewindow;
	Window mousewindow;
	Window lastactivewindow;
	RgnHandle mouseMoveRgn; //app does not get a mouse move msg if mouse is in
	// this region, we make it the smalles possible.
	// it's used in WaitNextEvent() call
	short mouseInWinRgn;    //mouse is in which window region
	Boolean menuBarHidden; //flag to indicat is the menu bar is hidden
	static uint2 ink_table_m[];
	static uint2 ink_table_c[];
	Handle menuBar;	 //Handle to menu for the entire MC app
	MenuHandle appleMenu;  //handle to Apple menu
	WindowPtr invisibleWin;
	Boolean bgmode;

	bool backdrop_hard;
	Boolean backdrop_active;
	WindowPtr backdrop_window;
	WindowGroupRef backdrop_group;
	WindowGroupRef backdrop_background_group;
	WindowGroupRef backdrop_document_group;
	WindowGroupRef backdrop_palette_group;
	MCColor backdrop_colour;
	MCPatternRef backdrop_pattern;
	MCImage *backdrop_badge;
	
	MenuRef f_icon_menu;
	
	//

	// Universal Procedure Pointers for clipboard and drag-drop
	DragSendDataUPP m_drag_send_data_upp;
	ScrapPromiseKeeperUPP m_scrap_promise_keeper_upp;

	// The current scrap reference, non-NULL if we think we own the scrap.
	ScrapRef m_current_scrap;

	// The last set of scrap data we placed on the scrap. We keep this as the
	// promise keeper callback needs the data.
	MCMacOSXTransferData *m_current_scrap_data;

	// This EventRecord is the last mouseDown event which is used by the
	// drag-drop code to call TrackDrag
	EventRecord m_drag_event;

	// Used to indicate whether we've down drag detection for the current click
	bool m_drag_click;

	//
	
	// MW-2008-07-22: [[ Bug 1276 ]] Resize continues after mouse has been released.
	//   This flag is true when we are inside a 'ResizeWindow' call. When true, we
	//   restrict the events we filter for in the core GetNextEvent call - in particular
	//   we make sure we don't eat the mouseUp even the OS wants to end the Resize!
	bool m_in_resize;
	
	//
	
	// MW-2009-12-22: The default destination profile
	CMProfileRef m_dst_profile;
	CMProfileRef m_srgb_profile;
	
	//

	// To make sure we don't end up deleting windows while inside system event
	// handlers, we accumulate all deletions into a list and then delete at
	// suitable moments.
	static WindowRef *c_window_deletions;
	static unsigned int c_window_deletion_count;

	//

	static EventHandlerUPP s_icon_menu_event_handler_upp;
	
	// for storing non-application main menu IDs. Only member 33 to 235
	// are used to keep track of the ids that are being used for submenus
	// and button's menus such as Pop-up, option, and pulldows

	static uint2 submenuIDs[20000];
	
	GWorldPtr bgw;
	
	////TSM - TEXT SERVICES MANAGER DOCUMENT
	static TSMDocumentID tsmdocument;
	static AEEventHandlerUPP TSMPositionToOffsetUPP;
	static AEEventHandlerUPP TSMOffsetToPositionUPP;
	static AEEventHandlerUPP TSMUpdateHandlerUPP;
	static AEEventHandlerUPP TSMUnicodeNotFromInputUPP;
	static DragReceiveHandlerUPP dragdropUPP;
	static DragTrackingHandlerUPP dragmoveUPP;
	Boolean owndnd;
	DragReference dnddata;
	
	Boolean cursorhidden ;
	Boolean menubarhidden ;
	
protected:
	uint2 opened;
	
public:
	static MCDisplay *s_monitor_displays;
	static uint4 s_monitor_count;
	
	static CFAbsoluteTime s_animation_start_time;
	static CFAbsoluteTime s_animation_current_time;

	// in macdc.cc  Device Context routines
	MCScreenDC();
	virtual ~MCScreenDC();
	
	Boolean getmenubarhidden(void) { return menubarhidden; };

	virtual int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false);
	
	virtual bool hasfeature(MCPlatformFeature p_feature);
	
			// in macdcs.cc   Screen routines
	virtual void setstatus(const char *status);
	
	virtual Boolean open();
	virtual Boolean close(Boolean force);
		
	virtual void resetcursors();
	virtual void setcursor(Window w, MCCursorRef c);
	
	virtual void grabpointer(Window w);
	virtual void ungrabpointer();
	
	virtual const char *getdisplayname();
	virtual uint2 getmaxpoints(void);
	virtual uint2 getvclass(void);
	virtual uint2 getdepth(void);
	
	// IM-2013-08-01: [[ ResIndependence ]] refactored methods that return device coordinates
	virtual uint16_t device_getwidth(void);
	virtual uint16_t device_getheight(void);
	virtual bool device_getdisplays(bool p_effective, MCDisplay *& r_displays, uint32_t &r_count);
	virtual bool device_getwindowgeometry(Window w, MCRectangle &drect);
	virtual void device_boundrect(MCRectangle &rect, Boolean title, Window_mode m);
	virtual void device_querymouse(int2 &x, int2 &y);
	virtual void device_setmouse(int2 x, int2 y);
	virtual MCStack *device_getstackatpoint(int32_t x, int32_t y);
	
	virtual void openwindow(Window w, Boolean override);
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
	
	virtual bool lockpixmap(Pixmap p_pixmap, void*& r_data, uint4& r_stride);
	virtual void unlockpixmap(Pixmap p_pixmap, void *p_data, uint4 p_stride);

	virtual Boolean getpixmapgeometry(Pixmap p, uint2 &w, uint2 &h, uint2 &d);
	
	virtual void setgraphicsexposures(Boolean on, MCStack *sptr);
	virtual void copyarea(Drawable source, Drawable dest, int2 depth,
	                      int2 sx, int2 sy, uint2 sw, uint2 sh,
	                      int2 dx, int2 dy, uint4 rop);
	
	virtual MCColorTransformRef createcolortransform(const MCColorSpaceInfo& info);
	virtual void destroycolortransform(MCColorTransformRef transform);
	virtual bool transformimagecolors(MCColorTransformRef transform, MCImageBitmap *image);
	
	virtual MCCursorRef createcursor(MCImageBitmap *image, int2 xhot, int2 yhot);
	virtual void freecursor(MCCursorRef c);

	virtual uint4 dtouint4(Drawable d);
	virtual Boolean uint4towindow(uint4, Window &w);
	virtual void getbeep(uint4 property, MCExecPoint &ep);
	virtual void setbeep(uint4 property, int4 beep);
	virtual void getvendorstring(MCExecPoint &ep);
	virtual uint2 getpad();
	virtual Window getroot();
	virtual MCImageBitmap *snapshot(MCRectangle &r, MCGFloat p_scale_factor, uint4 window,
	                           const char *displayname);

	virtual void enablebackdrop(bool p_hard);
	virtual void disablebackdrop(bool p_hard);
	virtual void configurebackdrop(const MCColor& p_colour, MCPatternRef p_pattern, MCImage *p_badge);
	virtual void assignbackdrop(Window_mode p_mode, Window p_window);
	
	virtual void hidemenu();
	virtual void showmenu();

	virtual MCColor *getaccentcolors();

	virtual void expose();
	virtual Boolean abortkey();
	virtual uint2 querymods();
	virtual Boolean getmouse(uint2 button, Boolean& r_abort);
	virtual Boolean getmouseclick(uint2 button, Boolean& r_abort);
	virtual Boolean wait(real8 duration, Boolean dispatch, Boolean anyevent);
	virtual void flushevents(uint2 e);
	virtual void updatemenubar(Boolean force);
	virtual Boolean istripleclick();
	virtual uint1 fontnametocharset(const char *oldfontname);
	virtual char *charsettofontname(uint1 chharset, const char *oldfontname);
	virtual void clearIME(Window w);
	virtual void openIME();
	virtual void activateIME(Boolean activate);
	virtual void closeIME();

	virtual void listprinters(MCExecPoint& ep);
	virtual MCPrinter *createprinter(void);

	//

	virtual void flushclipboard(void);
	virtual bool ownsclipboard(void);
	virtual bool setclipboard(MCPasteboard *p_pasteboard);
	virtual MCPasteboard *getclipboard(void);
    
    // TD-2013-07-01: [[ DynamicFonts ]]
    virtual bool loadfont(const char *p_path, bool p_globally, void*& r_loaded_font_handle);
    virtual bool unloadfont(const char *p_path, bool p_globally, void *r_loaded_font_handle);

	//

	virtual MCDragAction dodragdrop(MCPasteboard *p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint* p_image_offset);

	//
	
	virtual MCScriptEnvironment *createscriptenvironment(const char *p_language);

	//
	
	// in macdcmac.cc  screen routines that are specfic to MAC platform
	void getmods();
	void setmods(int2 modifiers);
	void getkeycode(EventRecord &event, char *buffer);
	virtual void getkeysdown(MCExecPoint &ep);
	// MW-2012-09-21: [[ Bug 10404 ]] If post_or_handle is false, no event is generated in the
	//   queue. This is used by getmouseclick() to ensure mouseWindow is updated correctly.
	void mfocus(EventRecord *event, Point p, Boolean dispatch, bool post_or_handle = true);
	Boolean dispatchevent(EventRecord &event, Boolean dispatch,
	                      Boolean &abort, Boolean &reset);
	Boolean handle(real8 sleep, Boolean dispatch, Boolean anyevent,
	               Boolean &abort, Boolean &reset);
	void waitmessage(Window w, int event_type);
	void activatewindow(Window window);
	void doredraw(EventRecord &event, bool p_update_called = false);


	void drawdefaultbutton(const MCRectangle &trect);
	void setbgmode(Boolean on)
	{
		bgmode = on;
	}

	void copybits(Drawable s, Drawable d, int2 sx, int2 sy,
	              uint2 sw, uint2 sh, int2 dx, int2 dy, uint4 rop);
	short allocateSubMenuID(Boolean issubmenu);
	void freeMenuAndID(short menuID, MCButton *p_menubutton = NULL);
	Boolean addMenuItemsAndSubMenu(uint2 mainMenuID, MenuHandle mainMenu, char *itemlist, uint4 itemlistlength, uint1 menumode, MCFontStruct *f);
	Boolean addMenuItemsAndSubMenu(uint2 mainMenuID, MenuHandle mainMenu, MCButton *p_menubutton, uint1 menumode);
	void clearmdown(uint2 which);
	void setdnddata(DragReference theDrag);
	
	void seticon(uint4 p_icon);
	void seticonmenu(const char *p_menu);
	virtual void configurestatusicon(uint32_t icon_id, const char *menu, const char *tooltip);
	
	void enactraisewindows(void);
	
	WindowPtr getinvisiblewin()
	{
		return invisibleWin;
	}
	Boolean getowndnd()
	{
		return owndnd;
	}
	
	// This routine converts a UTF-8 string into a CFString suitable for passing
	// to Carbon APIs. The caller is responsible for CFRelease'ing the string.
	static CFStringRef convertutf8tocf(const char *p_utf8_string);

private:
	void updatebackdrop(const MCRectangle& p_region);
	void redrawbackdrop(MCContext *p_context, const MCRectangle& p_dirty);
	bool initialisebackdrop(void);
	void finalisebackdrop(void);
	
	WindowPartCode mode_findwindow(Point p, WindowRef* x_window);
	void mode_globaltolocal(Point& p);
	
	static OSStatus handleiconmenuevent(EventHandlerCallRef p_ref, EventRef p_event, void *p_data); 
	
	static pascal OSErr DragTrackingHandler(DragTrackingMessage p_message, WindowRef p_window, void *p_context, DragRef p_drag);
	static pascal OSErr DragReceiveHandler(WindowPtr p_window, void *p_context, DragRef p_drag);
};
#endif
