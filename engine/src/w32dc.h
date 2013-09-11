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
// Presentation Manager for graphics output
//

#ifndef	W32DC_H
#define	W32DC_H

#include "uidc.h"
#include "w32flst.h"

#define CWM_TASKBAR_NOTIFICATION (WM_USER + 1)
#define CWM_RELAUNCH (WM_USER + 3)
#define CWM_REMOTE (WM_USER + 6)

#define MC_WIN_CLASS_NAME		"MCWinClass"
#define MC_WIN_CLASS_NAME_W L"MCWinClassW"
#define MC_VIDEO_WIN_CLASS_NAME		"MCVideoWinClass"
#define MC_QTVIDEO_WIN_CLASS_NAME	"MCQTVideoWinClass"
#define MC_POPUP_WIN_CLASS_NAME		"MCPopupWinClass"
#define MC_MENU_WIN_CLASS_NAME		"MCMenuWinClass"
#define MC_SNAPSHOT_WIN_CLASS_NAME	"MCSnapshotWinClass"
#define MC_BACKDROP_WIN_CLASS_NAME      "MCBackdropWinClass"
#define MC_APP_NAME			"Revolution"
#define MC_APP_NAME_W		L"Revolution"

#define REFRESH_RATE			10.0
#define SELECTION_WAIT			5000
#define MAX_CELLS			4096
#define COL_QUANT			12
#define XLOOKUPSTRING_SIZE		32
#define CONFIGURE_WAIT			0.5

#define fixmaskrop(a) ((a == GXand || a == GXor)?(a == GXand?GXor:GXand):(a == GXandInverted?GXorInverted:GXandInverted))//DEBUG
#define fixmaskcolor(a) (a.pixel == 0 ? MConecolor:MCzerocolor)//DEBUG

extern LRESULT CALLBACK MCQTPlayerWindowProc(HWND hwnd, UINT msg, WPARAM wParam,
	        LPARAM lParam);

extern LRESULT CALLBACK MCPlayerWindowProc(HWND hwnd, UINT msg, WPARAM wParam,
	        LPARAM lParam);
extern LRESULT CALLBACK MCWindowProc(HWND hwnd, UINT msg,
	                                     WPARAM wParam, LPARAM lParam);
extern LRESULT CALLBACK MCSnapshotWindowProc(HWND hwnd, UINT msg,
	        WPARAM wParam, LPARAM lParam);
extern LRESULT CALLBACK MCBackdropWindowProc(HWND hwnd, UINT msg,
	        WPARAM wParam, LPARAM lParam);
extern LRESULT CALLBACK MCSocketWindowProc(HWND hwnd, UINT msg,
	        WPARAM wParam, LPARAM lParam);

class MCEventnode : public MCDLlist
{
public:
	HWND hwnd;
	UINT msg;
	WPARAM wParam;
	LPARAM lParam;
	uint2 modifier;
	uint4 time;
	KeySym keysym;
	MCEventnode(HWND h, UINT m, WPARAM w, LPARAM l, KeySym k, uint2 mod, uint4 t)
	{
		hwnd = h;
		msg = m;
		wParam = w;
		lParam = l;
		keysym = k;
		modifier = mod;
		time = t;
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

#define MAX_POINT_IN_POLYGON   3500

class MCScreenDC : public MCUIDC
{
	HDC f_src_dc;
	HDC f_dst_dc;

	MCEventnode *pendingevents;
	MCVisualInfo *vis;
	uint2 beeppitch;
	uint2 beepduration;
	Boolean grabbed;
	Boolean grabbingclipboard;
	Boolean exposures;
	HANDLE mutex;
	Boolean owndnd;
	HWND invisiblehwnd;
	UINT mousetimer;
	LPDATAOBJECT dnddata;

	bool backdrop_active;
	bool backdrop_hard;
	HWND backdrop_window;
	MCColor backdrop_colour;
	MCGImageRef backdrop_pattern;
	MCImage *backdrop_badge;

	HDC m_printer_dc;
	bool m_printer_dc_locked;
	bool m_printer_dc_changed;

	IDataObject *m_clipboard;

	HANDLE m_srgb_profile;

protected:
	static uint4 pen_inks[];
	static uint4 image_inks[];

	uint2 opened;
public:

#ifdef FEATURE_TASKBAR_ICON
	Boolean f_has_icon;
	HMENU f_icon_menu;
#endif

	static MCDisplay *s_monitor_displays;
	static uint4 s_monitor_count;

	int4 system_codepage;
	int4 input_codepage;
	HKL input_default_keyboard;

	// in w32dc.cc
	MCScreenDC();
	virtual ~MCScreenDC();

	virtual int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false);

	virtual bool hasfeature(MCPlatformFeature p_feature);

	// in w32dcs.cc
	virtual void setstatus(const char *status);

	virtual Boolean open();
	virtual Boolean close(Boolean force);
	
	virtual void resetcursors();
	virtual void setcursor(Window w, MCCursorRef c);
	
	virtual void grabpointer(Window w);
	virtual void ungrabpointer();
	
	virtual const char *getdisplayname();
	virtual uint2 getwidthmm();
	virtual uint2 getheightmm();
	virtual uint2 getmaxpoints();
	virtual uint2 getvclass();
	virtual uint2 getdepth();

	virtual uint16_t device_getwidth(void);
	virtual uint16_t device_getheight(void);
	virtual bool device_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count);
	virtual bool device_getwindowgeometry(Window w, MCRectangle &drect);
	virtual void device_boundrect(MCRectangle &rect, Boolean title, Window_mode m);
	virtual void device_querymouse(int16_t &r_x, int16_t &r_y);
	virtual void device_setmouse(int16_t p_x, int16_t p_y);
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
	virtual void beep();
	virtual void showtaskbar();
	virtual void hidetaskbar();
	virtual void setinputfocus(Window window);

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
	//virtual bool selectrect(MCRectangle &r_rect);
	virtual MCImageBitmap *snapshot(MCRectangle &r, uint4 window, const char *displayname);

	virtual HRGN BitmapToRegion(MCImageBitmap *p_bitmap);

	virtual void expose();
	virtual Boolean abortkey();
	virtual uint2 querymods();
	virtual void getkeysdown(MCExecPoint &ep);
	virtual Boolean getmouse(uint2 button, Boolean& r_abort);
	virtual Boolean getmouseclick(uint2 button, Boolean& r_abort);
	virtual void addmessage(MCObject *optr, MCNameRef name, real8 time, MCParameter *params);
	virtual Boolean wait(real8 duration, Boolean dispatch, Boolean anyevent);
	virtual void flushevents(uint2 e);
	virtual Boolean istripleclick();
	virtual char *charsettofontname(uint1 chharset, const char *oldfontname);
	virtual uint1 fontnametocharset(const char *oldfontname);
	virtual void clearIME(Window w);
	virtual void openIME();
	virtual void activateIME(Boolean activate);
	virtual void closeIME();

	virtual void enablebackdrop(bool p_hard);
	virtual void disablebackdrop(bool p_hard);
	virtual void configurebackdrop(const MCColor& p_color, MCGImageRef p_pattern, MCImage *p_badge);
	virtual void assignbackdrop(enum Window_mode p_mode, Window p_window);

	virtual void seticon(uint4 p_icon);
	virtual void seticonmenu(const char *p_menu);
	virtual void configurestatusicon(uint32_t icon_id, const char *menu, const char *tooltip);

	virtual void enactraisewindows(void);
	
	virtual MCPrinter *createprinter(void);
	virtual void listprinters(MCExecPoint& ep);

	//

	virtual int4 getsoundvolume(void);
	virtual void setsoundvolume(int4 p_volume);
	virtual void startplayingsound(IO_handle p_stream, MCObject *p_callback, bool p_next, int p_volume);
	virtual void stopplayingsound(void);

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

	Boolean isgrabbed();
	void setgrabbed(Boolean newgrab);

	uint2 getncolors();

	void appendevent(MCEventnode *tptr);
	void setmods();

	KeySym getkeysym(WPARAM wParam, LPARAM lParam);

	Boolean handle(real8 sleep, Boolean dispatch, Boolean anyevent, Boolean &abort, Boolean &reset);

	Boolean getgrabbed();
	HWND getinvisiblewindow();
	void setmousetimer(UINT t);
	UINT getmousetimer();

	Boolean taskbarhidden;

	HDC getsrchdc(void) const;
	HDC getdsthdc(void) const;

	void restackwindows(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	void hidebackdrop(Boolean p_hide);

	void redrawbackdrop(void);

	void settaskbarstate(bool p_visible);
	void processdesktopchanged(bool p_notify = true);
	void processtaskbarnotify(HWND hwnd, WPARAM wparam, LPARAM lparam);

	// These rountines convert a UTF-8 string into with a 'WIDE' or 'ANSI'
	// string suitable for passing to a windows API W or A function. The
	// caller is reponsible for freeing the returned string.
	static LPWSTR convertutf8towide(const char *p_utf8_string);
	static LPCSTR convertutf8toansi(const char *p_utf8_string);

private:
	bool initialisebackdrop(void);
	void finalisebackdrop(void);
};

inline Boolean MCScreenDC::isgrabbed()
{
	return grabbed;
}

inline void MCScreenDC::setgrabbed(Boolean newgrab)
{
	grabbed = newgrab;
}

inline uint2 MCScreenDC::getncolors()
{
	return ncolors;
}

inline HWND MCScreenDC::getinvisiblewindow(void)
{
	return invisiblehwnd;
}

inline Boolean MCScreenDC::getgrabbed(void)
{
	return grabbed;
}

inline void MCScreenDC::setmousetimer(UINT t)
{
	mousetimer = t;
}

inline UINT MCScreenDC::getmousetimer(void)
{
	return mousetimer;
}

inline HDC MCScreenDC::getsrchdc(void) const
{
	return f_src_dc;
}

inline HDC MCScreenDC::getdsthdc(void) const
{
	return f_dst_dc;
}

#endif
