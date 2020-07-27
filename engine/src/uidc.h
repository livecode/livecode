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

//
// swallow graphics output (for non-GUI scripts)
//
#ifndef	UIDC_H
#define	UIDC_H

#include "dllst.h"
#include "filedefs.h"
#include "graphics.h"
#include "imagebitmap.h"
#include "object.h"

enum
{
    DRAG_ACTION_MOVE_BIT = 0,
    DRAG_ACTION_COPY_BIT,
    DRAG_ACTION_LINK_BIT,
    
    DRAG_ACTION_NONE = 0,
    DRAG_ACTION_MOVE = 1 << DRAG_ACTION_MOVE_BIT,
    DRAG_ACTION_COPY = 1 << DRAG_ACTION_COPY_BIT,
    DRAG_ACTION_LINK = 1 << DRAG_ACTION_LINK_BIT,
};

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
    WP_ASRECT,
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

// Converts a keysym to its lower-case equivalent
KeySym MCKeySymToLower(KeySym p_key);

class MCPendingMessage
{
public:
    
    MCObjectHandle      m_object;
    MCNewAutoNameRef    m_message;
    real64_t            m_time = 0;
    MCParameter*        m_params = nullptr;
    uint32_t            m_id = 0;

    constexpr MCPendingMessage() = default;

    MCPendingMessage(const MCPendingMessage& other) = default;

    // [[ C++11 ]] Replace with `MCPendingMessage(MCObject*, MCNameRef, real64_t, MCParameter*, uint32_t) = default;`
    MCPendingMessage(MCObject* p_object, MCNameRef p_message, real64_t p_time, MCParameter* p_params, uint32_t p_id) :
      m_object(p_object),
      m_message(p_message),
      m_time(p_time),
      m_params(p_params),
      m_id(p_id)
    {
    }

    MCPendingMessage& operator= (const MCPendingMessage& other)
    {
        m_object = other.m_object;
        m_message.Reset(*other.m_message);
        m_time = other.m_time;
        m_params = other.m_params;
        m_id = other.m_id;
        
        return *this;
    }
    
    void DeleteParameters();
};

// [[ C++ ]] This should probably be a std::vector
class MCPendingMessagesList
{
public:
    
    MCPendingMessagesList() :
      m_array(nil),
      m_capacity(0),
      m_count(0)
    {
    }
    
    ~MCPendingMessagesList();
    
    const MCPendingMessage& operator[] (size_t offset) const
    {
        MCAssert(offset < m_count);
        return m_array[offset];
    }
    
    bool InsertMessageAtIndex(size_t index, const MCPendingMessage&);
    void DeleteMessage(size_t index, bool delete_params);
    void ShiftMessageTo(size_t to, size_t from, real64_t newtime);
    
    size_t GetCount() const
    {
        return m_count;
    }
    
private:
    
    MCPendingMessage*   m_array;
    
    size_t m_capacity;
    size_t m_count;
};

// IM-2014-01-23: [[ HiDPI ]] Add screen pixelScale field to display info
// IM-2014-01-23: [[ HiDPI ]] Remove device-coordinate versions of viewport & workarea rects
struct MCDisplay
{
	uint4 index;
//	MCRectangle device_viewport;
//	MCRectangle device_workarea;
	
	MCRectangle viewport;
	MCRectangle workarea;
	
	MCGFloat pixel_scale;
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
	MCObjectHandle object;
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

// IM-2014-03-06: [[ revBrowserCEF ]] Actions to call during the runloop
typedef void (*MCRunloopActionCallback)(void *context);

typedef struct _MCRunloopAction
{
	MCRunloopActionCallback callback;
	void *context;
	
	uint32_t references;

	_MCRunloopAction *next;
} MCRunloopAction, *MCRunloopActionRef;

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

enum MCSystemAppearance
{
	kMCSystemAppearanceLight = 0,
	kMCSystemAppearanceDark = 1,
};

class MCUIDC
{
public:
    
    typedef void (*modal_break_callback_fn)(void *);
    struct modal_loop
    {
        modal_break_callback_fn break_function;
        void* context;
        modal_loop* chain;
        bool broken;
    };
    
protected:
	MCPendingMessagesList m_messages;
	MCMovingList *moving;
	Boolean lockmoves;
	real8 locktime;
	uint4 messageid;
	MCColor *colors;
	MCStringRef *colornames;
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
	
    modal_loop* m_modal_loops;
    
	// IM-2014-01-24: [[ HiDPI ]] Cache displays array returned from platform-specific methods
	static MCDisplay *s_displays;
	static uint4 s_display_count;
	static bool s_display_info_effective;

	// IM-2014-03-06: [[ revBrowserCEF ]] List of actions to run during the runloop
	MCRunloopAction *m_runloop_actions;
	
public:
	MCColor white_pixel;
	MCColor black_pixel;
	MCColor gray_pixel;
	MCColor background_pixel;

	MCUIDC();
	virtual ~MCUIDC();
	
	virtual bool setbeepsound ( MCStringRef p_beep_sound) ;
	virtual bool getbeepsound ( MCStringRef& r_beep_sound );

	virtual bool hasfeature(MCPlatformFeature p_feature);

	virtual void setstatus(MCStringRef status);

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
	virtual uint2 getdepth();
	virtual uint2 getrealdepth(void);

////////////////////////////////////////////////////////////////////////////////

	// IM-2013-09-23: [[ FullscreenMode ]] Part of view abstraction
	// get/set mouse location in view coordinates, translating to/from stack coordinates
	void setmouseloc(MCStack *p_target, MCPoint p_loc);
	void getmouseloc(MCStack *&r_target, MCPoint &r_loc);

	// get/set click location in view coordinates, translating to/from stack coordinates
	void setclickloc(MCStack *p_target, MCPoint p_loc);
	void getclickloc(MCStack *&r_target, MCPoint &r_loc);

////////////////////////////////////////////////////////////////////////////////
	
	// IM-2014-01-28: [[ HiDPI ]] Convenience methods to convert logical to screen coords and back
	// IM-2014-07-09: [[ Bug 12602 ]] Move screen coord conversion methods into MCUIDC
	virtual MCPoint logicaltoscreenpoint(const MCPoint &p_point);
	virtual MCPoint screentologicalpoint(const MCPoint &p_point);
	virtual MCRectangle logicaltoscreenrect(const MCRectangle &p_rect);
	virtual MCRectangle screentologicalrect(const MCRectangle &p_rect);

	// IM-2013-07-31: [[ ResIndependence ]] refactor logical coordinate based methods
	uint2 getwidth();
	uint2 getheight();
	
	uint4 getdisplays(MCDisplay const *& p_displays, bool effective);
	
	// IM-2014-01-28: [[ HiDPI ]] Update the currently held display info, returning whether or not an changes have occurred
	void updatedisplayinfo(bool &r_changed);

	// IM-2014-01-24: [[ HiDPI ]] Clear the currently held display information. Should be called
	// when the display info needs to be refreshed, for example when a screen is (dis)connected
	// or screen resolution settings are changed.
	void cleardisplayinfocache(void);
	
	// IM-2014-01-28: [[ HiDPI ]] Return true if the platform can detect
	//   desktop changes and will clear the cache when changes occur.
	virtual bool platform_displayinfocacheable(void);
	
	// IM-2014-01-24: [[ HiDPI ]] Return the maximum pixel scale of all displays in use
	bool getmaxdisplayscale(MCGFloat &r_scale);
	
	const MCDisplay *getnearestdisplay(const MCRectangle& p_rectangle);
	Boolean getwindowgeometry(Window w, MCRectangle &drect);
	void boundrect(MCRectangle &rect, Boolean title, Window_mode m, Boolean resizable);
	void querymouse(int2 &x, int2 &y);
	void setmouse(int2 x, int2 y);

	// MW-2012-10-08: [[ HitTest ]] Get the stack at the given screen location.
	MCStack *getstackatpoint(int32_t x, int32_t y);
	
//////////
	
	// IM-2014-01-23: [[ HiDPI ]] Change device methods to platform-specific logical-coordinate based methods
	
	virtual uint16_t platform_getwidth(void);
	virtual uint16_t platform_getheight(void);
	virtual bool platform_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count);
	virtual bool platform_getwindowgeometry(Window w, MCRectangle &drect);
	virtual void platform_boundrect(MCRectangle &rect, Boolean title, Window_mode m, Boolean resizable);
	virtual void platform_querymouse(int16_t &r_x, int16_t &r_y);
	virtual void platform_setmouse(int16_t p_x, int16_t p_y);

	virtual MCStack *platform_getstackatpoint(int32_t x, int32_t y);
	
	virtual bool platform_get_display_handle(void *&r_display);
	
////////////////////////////////////////////////////////////////////////////////
	
	// IM-2013-09-30: [[ FullscreenMode ]] Returns true if windows on this display are 
	// always fullscreen (i.e. on mobile devices)
	virtual bool fullscreenwindows(void);

	// IM-2013-09-30: [[ FullscreenMode ]] Return the rect that will be occupied by
	// fullscreen windows on the given display
	virtual MCRectangle fullscreenrect(const MCDisplay *p_display);

	virtual void openwindow(Window w, Boolean override);
	virtual void closewindow(Window window);
	virtual void destroywindow(Window &window);
	virtual void raisewindow(Window window);
	virtual void iconifywindow(Window window);
	virtual void uniconifywindow(Window window);

	// Set the name of 'window' to the UTF-8 string 'newname'
	virtual void setname(Window window, MCStringRef newname);
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

	virtual uintptr_t dtouint(Drawable d);
	virtual Boolean uinttowindow(uintptr_t, Window &w);
	
	virtual void *GetNativeWindowHandle(Window p_window);

	virtual void getbeep(uint4 property, int4& r_value);
	virtual void setbeep(uint4 property, int4 beep);
	virtual MCNameRef getvendorname(void);
	virtual uint2 getpad();
	virtual Window getroot();

	virtual MCImageBitmap *snapshot(MCRectangle &r, uint4 window, MCStringRef displayname, MCPoint *size);

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
    virtual void delaymessage(MCObject *optr, MCNameRef name, MCStringRef p1 = nil, MCStringRef p2 = nil);
	
    // When called, all modal loops should be exited and control should return
    // to the main event loop. The intended use of this method is to prevent UI
    // lockups when a script error occurs during a modal loop (e.g during
    // the drag-and-drop loop)
    void breakModalLoops();
    
    // Indicates that a modal event loop is being entered. A callback function
    // should be passed in order to allow the breaking of the loop.
    void modalLoopStart(modal_loop& info);
    
    // Indicates that the innermost modal loop is being exited
    void modalLoopEnd();
    
	// Wait for at most 'duration' seconds. If 'dispatch' is true then event
	// dispatch will occur. If 'anyevent' is true then the call will return
	// as soon as something notable happens. If an abort/quit occurs while the
	// wait is occuring, True will be returned.
	virtual Boolean wait(real8 duration, Boolean dispatch, Boolean anyevent);
	// Notify any current wait loop that something has occurred that it might
	// not be aware of. This will cause any OS loop to be exited and events
	// and such to be processed. If the wait was called with 'anyevent' True
	// then it will cause termination of the wait.
	virtual void pingwait(void);

	bool FindRunloopAction(MCRunloopActionCallback p_callback, void *p_context, MCRunloopActionRef &r_action);
	// IM-2014-03-06: [[ revBrowserCEF ]] Add action to runloop
	bool AddRunloopAction(MCRunloopActionCallback p_callback, void *p_context, MCRunloopActionRef &r_action);
	// IM-2014-03-06: [[ revBrowserCEF ]] Remove action from runloop
	void RemoveRunloopAction(MCRunloopActionRef p_action);
	// IM-2014-03-06: [[ revBrowserCEF ]] Perform runloop actions
	void DoRunloopActions(void);
	
	// IM-2014-06-25: [[ Bug 12671 ]] Return if any runloop actions are registered.
	bool HasRunloopActions(void);

	virtual void flushevents(uint2 e);
	virtual void updatemenubar(Boolean force);
	virtual Boolean istripleclick();
	virtual bool getkeysdown(MCListRef& r_list);
	
	virtual uint1 fontnametocharset(MCStringRef p_fontname);
//	virtual char *charsettofontname(uint1 charset, const char *oldfontname);
	
	virtual void clearIME(Window w);
    virtual void configureIME(int32_t x, int32_t y);
	virtual void openIME();
	virtual void activateIME(Boolean activate);
	virtual void closeIME();

	virtual void seticon(uint4 p_icon);
	virtual void seticonmenu(MCStringRef p_menu);
	virtual void configurestatusicon(uint32_t icon_id, MCStringRef menu, MCStringRef tooltip);
	virtual void enactraisewindows(void);

	//

	virtual MCPrinter *createprinter(void);
	virtual bool listprinters(MCStringRef& r_printers);

	//

	virtual int4 getsoundvolume(void);
	virtual void setsoundvolume(int4 p_volume);
	virtual void startplayingsound(IO_handle p_stream, MCObject *p_callback, bool p_next, int p_volume);
	virtual void stopplayingsound(void);

	//

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
	// that no drop occurred.
	//
	virtual MCDragAction dodragdrop(Window w, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint *p_image_offset);
	
	//

	virtual MCScriptEnvironment *createscriptenvironment(MCStringRef p_language);

	//
    
    // Show an answer dialog using the native APIs. If 'blocking' is true
    // then messages will not be dispatched until the dialog is closed.
	virtual int32_t popupanswerdialog(MCStringRef *p_buttons, uint32_t p_button_count, uint32_t p_type, MCStringRef p_title, MCStringRef p_message, bool p_blocking);
	virtual bool popupaskdialog(uint32_t p_type, MCStringRef p_title, MCStringRef p_message, MCStringRef p_initial, bool p_hint, MCStringRef& r_result);
	
	//
    
    // TD-2013-05-29: [[ DynamicFonts ]]
	virtual bool loadfont(MCStringRef p_path, bool p_globally, void*& r_loaded_font_handle);
    virtual bool unloadfont(MCStringRef p_path, bool p_globally, void *r_loaded_font_handle);
    
    //
	
	virtual void controlgainedfocus(MCStack *s, uint32_t id);
	virtual void controllostfocus(MCStack *s, uint32_t id);
	
	//
    
    // MW-2014-04-26: [[ Bug 5545 ]] Hides the cursor until the mouse moves on platforms which
    //   require this action.
    virtual void hidecursoruntilmousemoves(void);
	
	virtual void getsystemappearance(MCSystemAppearance &r_appearance);
    
    //

	void addtimer(MCObject *optr, MCNameRef name, uint4 delay);
	void cancelmessageindex(size_t i, bool dodelete);
	void cancelmessageid(uint4 id);
	void cancelmessageobject(MCObject *optr, MCNameRef name, MCValueRef param = nil);
    bool listmessages(MCExecContext& ctxt, MCListRef& r_list);
    void doaddmessage(MCObject *optr, MCNameRef name, real8 time, uint4 id, MCParameter *params = nil);
    size_t doshiftmessage(size_t index, real8 newtime);
    
    void addsubtimer(MCObject *target, MCValueRef subtarget, MCNameRef name, uint4 delay);
    void cancelsubtimer(MCObject *target, MCNameRef name, MCValueRef subtarget);

    // MW-2014-05-28: [[ Bug 12463 ]] This is used by 'send in time' - separating user sent messages from
    //   engine sent messages. The former are subject to a limit to stop pending message queue overflow.
    bool addusermessage(MCObject *optr, MCNameRef name, real8 time, MCParameter *params);
    
    // Returns true if there are any pending messages to dispatch right now.
    bool hasmessagestodispatch(void);
    
	Boolean handlepending(real8 &curtime, real8 &eventtime, Boolean dispatch);
	Boolean getlockmoves() const;
	void setlockmoves(Boolean b);
	void addmove(MCObject *optr, MCPoint *pts, uint2 npts,
	             real8 &duration, Boolean waiting);
	bool listmoves(MCExecContext& ctxt, MCListRef& r_list);
	void stopmove(MCObject *optr, Boolean finish);
	void handlemoves(real8 &curtime, real8 &eventtime);
	void siguser();
	Boolean lookupcolor(MCStringRef s, MCColor *color);
	void dropper(Window w, int2 mx, int2 my, MCColor *cptr);
	bool parsecolor(MCStringRef p_string, MCColor& r_color);
	Boolean parsecolor(MCStringRef s, MCColor& r_color, MCStringRef *cname);
	Boolean parsecolors(const MCString &values, MCColor *colors,
	                    char *cnames[], uint2 ncolors);
	Boolean setcolors(const MCString &);
	bool getcolornames(MCStringRef&);
	void getpaletteentry(uint4 n, MCColor &c);

    
	Boolean hasmessages()
	{
		return m_messages.GetCount() != 0;
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
    
    const MCColor& getbg(void) const
    {
        return background_pixel;
    }
};

////////////////////////////////////////////////////////////////////////////////

// MCColor Utility functions
static inline uint32_t MCColorGetPixel(const MCColor &p_color, MCGPixelFormat p_format = kMCGPixelFormatNative)
{
	return MCGPixelPack(p_format, p_color.red >> 8, p_color.green >> 8, p_color.blue >> 8, 0xFF);
}

static inline void MCColorSetPixel(MCColor &x_color, uint32_t p_pixel, MCGPixelFormat p_format = kMCGPixelFormatNative)
{
	uint8_t r, g, b, a;
	MCGPixelUnpack(p_format, p_pixel, r, g, b, a);
	x_color.red = (uint16_t)((r << 8) | r);
	x_color.green = (uint16_t)((g << 8) | g);
	x_color.blue = (uint16_t)((b << 8) | b);
}

#endif
