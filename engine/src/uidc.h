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
// swallow graphics output (for non-GUI scripts)
//
#ifndef	UIDC_H
#define	UIDC_H

#ifndef DLLST_H
#include "dllst.h"
#endif

#ifndef FILEDEFS_H
#include "filedefs.h"
#endif

#ifndef __MC_TRANSFER__
#include "transfer.h"
#endif

#include "graphics.h"
#include "imagebitmap.h"

enum Flush_events {
    FE_ALL,
    FE_MOUSEDOWN,
    FE_MOUSEUP,
    FE_KEYDOWN,
    FE_KEYUP,
    FE_AUTOKEY,
    FE_DISK,
    FE_ACTIVATE,
    FE_HIGHLEVEL,
    FE_SYSTEM,
    FE_LAST
};

enum Window_position
{ //Will use in the future for allowing scripter to specify position
    //of window when opening
    WP_DEFAULT,
    WP_CENTERMAINSCREEN,
    WP_CENTERPARENT,
    WP_CENTERPARENTSCREEN,
    WP_PARENTRIGHT,
    WP_PARENTLEFT,
    WP_PARENTTOP,
    WP_PARENTBOTTOM
};

enum Window_mode {
    WM_CLOSED,
    WM_TOP_LEVEL,  // also button "tab" style
    WM_TOP_LEVEL_LOCKED,
    WM_MODELESS,
    WM_PALETTE,
    WM_MODAL,
    WM_SHEET,
    WM_PULLDOWN,
    WM_POPUP,
    WM_OPTION,     // also drop down list
    WM_CASCADE,
    WM_COMBO,
    WM_ICONIC,
    WM_DRAWER,
    WM_TOOLTIP,
    WM_LICENSE,
    WM_LAST
};

enum Transfer_type {
    TRT_UNDEFINED,
    TRT_HTML,
    TRT_IMAGE,
    TRT_OBJECT,
    TRT_TEXT,
    TRT_RTF,
    TRT_UNICODE,
    TRT_MAC_STYLED_TEXT,
    TRT_FILES
};

enum Transfer_mode {
    TRM_UNDEFINED,
    TRM_CLIPBOARD,
    TRM_DRAGDROP
};

typedef struct
{
	MCObject *object;
	MCNameRef message;
	real8 time;
	MCParameter *params;
	uint4 id;
}
MCMessageList;

struct MCDisplay
{
	uint4 index;
	MCRectangle device_viewport;
	MCRectangle device_workarea;
	
	MCRectangle viewport;
	MCRectangle workarea;
};

enum MCColorSpaceType
{
	kMCColorSpaceNone,
	kMCColorSpaceCalibratedRGB,
	kMCColorSpaceStandardRGB,
	kMCColorSpaceEmbedded
};

enum MCColorSpaceIntent
{
	kMCColorSpaceIntentPerceptual,
	kMCColorSpaceIntentRelativeColorimetric,
	kMCColorSpaceIntentSaturation,
	kMCColorSpaceIntentAbsoluteColorimetric
};

struct MCColorSpaceInfo
{
	MCColorSpaceType type;
	union
	{
		struct
		{
			double white_x, white_y;
			double red_x, red_y;
			double green_x, green_y;
			double blue_x, blue_y;
			double gamma;
		} calibrated;
		struct
		{
			MCColorSpaceIntent intent;
		} standard;
		struct
		{
			void *data;
			uint32_t data_size;
		} embedded;
	};
};

typedef void *MCColorTransformRef;

class MCMovingList : public MCDLlist
{
public:
	MCObject *object;
	MCPoint *pts;
	uint2 lastpt;
	uint2 curpt;
	int2 donex, doney;
	real8 dx, dy;
	real8 starttime;
	real8 duration;
	real8 speed;
	Boolean waiting;
	MCMovingList()
	{
		pts = NULL;
	}
	~MCMovingList();
	MCMovingList *next()
	{
		return (MCMovingList *)MCDLlist::next();
	}
	MCMovingList *prev()
	{
		return (MCMovingList *)MCDLlist::prev();
	}
	void appendto(MCMovingList *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCMovingList *remove(MCMovingList *&list)
	{
		return (MCMovingList *)MCDLlist::remove((MCDLlist *&)list);
	}
};

struct MCImageBuffer;

//

enum
{
	kMCAnswerDialogButtonOk,
	kMCAnswerDialogButtonCancel,
	kMCAnswerDialogButtonRetry,
	kMCAnswerDialogButtonYes,
	kMCAnswerDialogButtonNo,
	kMCAnswerDialogButtonAbort,
	kMCAnswerDialogButtonIgnore
};

enum
{
	kMCAnswerDialogTypeInformation,
	kMCAnswerDialogTypeQuestion,
	kMCAnswerDialogTypeWarning,
	kMCAnswerDialogTypeError
};

//

enum MCPlatformFeature
{
	PLATFORM_FEATURE_WINDOW_TRANSPARENCY,
	PLATFORM_FEATURE_OS_FILE_DIALOGS,
	PLATFORM_FEATURE_OS_COLOR_DIALOGS,
	PLATFORM_FEATURE_OS_PRINT_DIALOGS,
	PLATFORM_FEATURE_NATIVE_THEMES,
	PLATFORM_FEATURE_TRANSIENT_SELECTION
};

class MCUIDC
{
protected:
	MCMessageList *messages;
	MCMovingList *moving;
	uint4 messageid;
	uint2 nmessages;
	uint2 maxmessages;
	MCColor *colors;
	char **colornames;
	int2 *allocs;
	int2 ncolors;
	Boolean modalclosed;
	Boolean lockmods;
	uint2 redshift;
	uint2 greenshift;
	uint2 blueshift;
	uint2 redbits;
	uint2 greenbits;
	uint2 bluebits;
	const char *  m_sound_internal ;
public:
	MCColor white_pixel;
	MCColor black_pixel;
	MCColor gray_pixel;
	MCColor background_pixel;

	MCUIDC();
	virtual ~MCUIDC();
	
	virtual bool setbeepsound ( const char * p_internal) ;
	virtual const char * getbeepsound ( void );

	virtual bool hasfeature(MCPlatformFeature p_feature);

	virtual void setstatus(const char *status);

	virtual int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false);

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
	virtual uint2 getdepth();
	virtual uint2 getrealdepth(void);

////////////////////////////////////////////////////////////////////////////////
	
	// IM-2013-07-31: [[ ResIndependence ]] refactor logical coordinate based methods
	uint2 getwidth();
	uint2 getheight();
	uint4 getdisplays(MCDisplay const *& p_displays, bool effective);
	const MCDisplay *getnearestdisplay(const MCRectangle& p_rectangle);
	Boolean getwindowgeometry(Window w, MCRectangle &drect);
	void boundrect(MCRectangle &rect, Boolean title, Window_mode m);
	void querymouse(int2 &x, int2 &y);
	void setmouse(int2 x, int2 y);

	// MW-2012-10-08: [[ HitTest ]] Get the stack at the given screen location.
	MCStack *getstackatpoint(int32_t x, int32_t y);
	
//////////
	
	const MCDisplay *device_getnearestdisplay(const MCRectangle& p_rectangle);
	
	virtual uint16_t device_getwidth(void);
	virtual uint16_t device_getheight(void);
	virtual bool device_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count);
	virtual bool device_getwindowgeometry(Window w, MCRectangle &drect);
	virtual void device_boundrect(MCRectangle &rect, Boolean title, Window_mode m);
	virtual void device_querymouse(int16_t &r_x, int16_t &r_y);
	virtual void device_setmouse(int16_t p_x, int16_t p_y);

	virtual MCStack *device_getstackatpoint(int32_t x, int32_t y);
	
////////////////////////////////////////////////////////////////////////////////
	

	virtual void openwindow(Window w, Boolean override);
	virtual void closewindow(Window window);
	virtual void destroywindow(Window &window);
	virtual void raisewindow(Window window);
	virtual void iconifywindow(Window window);
	virtual void uniconifywindow(Window window);

	// Set the name of 'window' to the UTF-8 string 'newname'
	virtual void setname(Window window, const char *newname);
	virtual void setcmap(MCStack *sptr);

	virtual void sync(Window w);

	virtual void flush(Window w);

	virtual void beep();

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
	virtual MCImageBitmap *snapshot(MCRectangle &r, MCGFloat p_scale, uint4 window, const char *displayname);

	virtual void enablebackdrop(bool p_hard = false);
	virtual void disablebackdrop(bool p_hard = false);
	virtual void configurebackdrop(const MCColor& p_colour, MCPatternRef p_pattern, MCImage *p_badge);
	virtual void assignbackdrop(Window_mode p_mode, Window p_window);

	virtual void hidemenu();
	virtual void hidetaskbar();
	virtual void showmenu();
	virtual void showtaskbar();

	virtual MCColor *getaccentcolors();

	virtual void expose();
	virtual Boolean abortkey();
	virtual void waitconfigure(Window w);
	virtual void waitreparent(Window w);
	virtual void waitfocus();
	virtual uint2 querymods();
	virtual Boolean getmouse(uint2 button, Boolean& r_abort);
	virtual Boolean getmouseclick(uint2 button, Boolean& r_abort);
	virtual void addmessage(MCObject *optr, MCNameRef name, real8 time, MCParameter *params);
	virtual void delaymessage(MCObject *optr, MCNameRef name, char *p1 = NULL, char *p2 = NULL);
	
	// Wait for at most 'duration' seconds. If 'dispatch' is true then event
	// dispatch will occur. If 'anyevent' is true then the call will return
	// as soon as something notable happens. If an abort/quit occurs while the
	// wait is occuring, True will be returned.
	virtual Boolean wait(real8 duration, Boolean dispatch, Boolean anyevent);
	// Notify any current wait loop that something has occured that it might
	// not be aware of. This will cause any OS loop to be exited and events
	// and such to be processed. If the wait was called with 'anyevent' True
	// then it will cause termination of the wait.
	virtual void pingwait(void);

	virtual void flushevents(uint2 e);
	virtual void updatemenubar(Boolean force);
	virtual Boolean istripleclick();
	virtual void getkeysdown(MCExecPoint &ep);
	
	virtual uint1 fontnametocharset(const char *oldfontname);
	virtual char *charsettofontname(uint1 charset, const char *oldfontname);
	
	virtual void clearIME(Window w);
	virtual void openIME();
	virtual void activateIME(Boolean activate);
	virtual void closeIME();

	virtual void seticon(uint4 p_icon);
	virtual void seticonmenu(const char *p_menu);
	virtual void configurestatusicon(uint32_t icon_id, const char *menu, const char *tooltip);
	virtual void enactraisewindows(void);

	//

	virtual MCPrinter *createprinter(void);
	virtual void listprinters(MCExecPoint& ep);

	//

	virtual int4 getsoundvolume(void);
	virtual void setsoundvolume(int4 p_volume);
	virtual void startplayingsound(IO_handle p_stream, MCObject *p_callback, bool p_next, int p_volume);
	virtual void stopplayingsound(void);

	//

	// Returns true if this application currently owns the transient selection.
	//
	virtual bool ownsselection(void);

	// Attempt to grab the transient selection with the given list of data types.
	// Upon success, true should be returned.
	//
	// If p_pasteboard is NULL, then the selection is ungrabbed if this application
	// is the current owner.
	//
	// The pasteboard object is passed with get semantics, the callee must retain
	// the object if it wishes to keep a reference.
	//
	virtual bool setselection(MCPasteboard *p_pasteboard);

	// Return an object allowing access to the current transient selection. See
	// the definition of MCPasteboard for more information.
	//
	// The pasteboard object is returned with copy semantics - the caller must
	// release the object when it is finished with it.
	//
	virtual MCPasteboard *getselection(void);

	//

	// Flush the contents of the current clipboard to the OS if we are the current
	// owner. This method is called before final shutdown to ensure that the
	// clipboard isn't lost when a Revolution application quits.
	virtual void flushclipboard(void);

	// Returns true if this application currently owns the clipboard.
	//
	virtual bool ownsclipboard(void);

	// Attempt to grab the clipboard and place the given list of data types
	// upon it.
	//
	// If p_pasteboard is NULL and this application is the current owner of the
	// clipboard, ownersup should be relinquished.
	// 
	// The pasteboard object is passed with get semantics, the callee must retain
	// the object if it wishes to keep a reference.
	//
	virtual bool setclipboard(MCPasteboard *p_pasteboard);

	// Return an object allowing access to the current clipboard. See
	// the definition of MCPasteboard for more information.
	//
	// The returned object has copy semantics, the caller should release it when
	// it is finished with it.
	//
	// Note that the system clipboard should be considered locked by this method
	// and will remain so until the object is released. Therefore, the object 
	// should not be held for 'too long'.
	//
	virtual MCPasteboard *getclipboard(void);

	// Begin a drag-drop operation with this application as source and with the
	// given list of data-types.
	//
	// If the datatypes list is empty, the drag-drop operation should still
	// occur, its just that no data should be published (in the same way as
	// for the other 'selection' related operations).
	//
	// The <p_allowed_actions> parameter indicates the set of operations that the source application
	// understands - these are used to negotiate with the target application what should occur
	// with the data.
	//
	// The method returns the actual result of the drag-drop operation - DRAG_ACTION_NONE meaning
	// that no drop occured.
	//
	virtual MCDragAction dodragdrop(MCPasteboard *p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint *p_image_offset);
	
	//

	virtual MCScriptEnvironment *createscriptenvironment(const char *p_language);

	//

	virtual int32_t popupanswerdialog(const char **p_buttons, uint32_t p_button_count, uint32_t p_type, const char *p_title, const char *p_message);
	virtual char *popupaskdialog(uint32_t p_type, const char *p_title, const char *p_message, const char *p_initial, bool p_hint);
	
	//
    
    // TD-2013-05-29: [[ DynamicFonts ]]
	virtual bool loadfont(const char *p_path, bool p_globally, void*& r_loaded_font_handle);
    virtual bool unloadfont(const char *p_path, bool p_globally, void *r_loaded_font_handle);
    
    //

	void addtimer(MCObject *optr, MCNameRef name, uint4 delay);
	void cancelmessageindex(uint2 i, Boolean dodelete);
	void cancelmessageid(uint4 id);
	void cancelmessageobject(MCObject *optr, MCNameRef name);
	void listmessages(MCExecPoint &ep);
	Boolean handlepending(real8 &curtime, real8 &eventtime, Boolean dispatch);
	void addmove(MCObject *optr, MCPoint *pts, uint2 npts,
	             real8 &duration, Boolean waiting);
	void listmoves(MCExecPoint &ep);
	void stopmove(MCObject *optr, Boolean finish);
	void handlemoves(real8 &curtime, real8 &eventtime);
	void siguser();
	Boolean lookupcolor(const MCString &s, MCColor *color);
	void dropper(Window w, int2 mx, int2 my, MCColor *cptr);
	Boolean parsecolor(const MCString &s, MCColor *color, char **cname);
	Boolean parsecolors(const MCString &values, MCColor *colors,
	                    char *cnames[], uint2 ncolors);
	void alloccolor(MCColor &color);
	void querycolor(MCColor &color);
	Boolean getcolors(MCExecPoint &);
	Boolean setcolors(const MCString &);
	void getcolornames(MCExecPoint &);
	void getpaletteentry(uint4 n, MCColor &c);

	Boolean hasmessages()
	{
		return nmessages != 0;
	}
	void closemodal()
	{
		modalclosed = True;
	}
	void setlockmods(Boolean l)
	{
		lockmods = l;
	}
	Boolean getlockmods()
	{
		return lockmods;
	}
	void dodrop(MCStack *dropstack);

	const MCColor& getblack(void) const
	{
		return black_pixel;
	}

	const MCColor& getwhite(void) const
	{
		return white_pixel;
	}

	const MCColor& getgray(void) const
	{
		return gray_pixel;
	}
};

#endif
