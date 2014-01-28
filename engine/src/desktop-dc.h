#ifndef __MC_DESKTOP_DC__
#define __MC_DESKTOP_DC__

#ifndef __MC_UIDC__
#include "uidc.h"
#endif

class MCSystemPasteboard: public MCPasteboard
{
public:
	MCSystemPasteboard(MCPlatformPasteboardRef pasteboard);
	~MCSystemPasteboard(void);
	
	virtual void Retain(void);
	virtual void Release(void);	
	
	virtual bool Query(MCTransferType*& r_types, unsigned int& r_type_count);
	virtual bool Fetch(MCTransferType p_type, MCSharedString*& r_data);
	
private:
	uint32_t m_references;
};

class MCScreenDC: public MCUIDC
{
private:
	uint2 beeppitch;
	uint2 beepduration;
	Boolean menubarhidden;
	
public:
	static MCDisplay *s_monitor_displays;
	static uint4 s_monitor_count;
	
	MCScreenDC(void);
	virtual ~MCScreenDC(void);
	
	virtual bool hasfeature(MCPlatformFeature p_feature);
	
	virtual Boolean open();
	virtual Boolean close(Boolean force);

	virtual const char *getdisplayname();
	virtual uint2 getmaxpoints(void);
	virtual uint2 getvclass(void);
	virtual uint2 getdepth(void);
	virtual void getvendorstring(MCExecPoint &ep);
	virtual uint2 getpad();
	
	virtual MCColor *getaccentcolors();
	
	virtual uint16_t device_getwidth(void);
	virtual uint16_t device_getheight(void);
	virtual bool device_getdisplays(bool p_effective, MCDisplay *& r_displays, uint32_t &r_count);
	virtual void device_boundrect(MCRectangle &rect, Boolean title, Window_mode mode);
	
	virtual void resetcursors();
	virtual void setcursor(Window w, MCCursorRef c);
	virtual MCCursorRef createcursor(MCImageBitmap *image, int2 xhot, int2 yhot);
	virtual void freecursor(MCCursorRef c);
	
	virtual bool device_getwindowgeometry(Window w, MCRectangle &drect);
	virtual void openwindow(Window w, Boolean override);
	virtual void closewindow(Window window);
	virtual void destroywindow(Window &window);
	virtual void raisewindow(Window window);
	virtual void iconifywindow(Window window);
	virtual void uniconifywindow(Window window);
	virtual void setname(Window window, const char *newname);
	virtual void setinputfocus(Window window);
	virtual uint4 dtouint4(Drawable d);
	virtual Boolean uint4towindow(uint4, Window &w);
	
	virtual void enablebackdrop(bool p_hard);
	virtual void disablebackdrop(bool p_hard);
	virtual void configurebackdrop(const MCColor& p_colour, MCPatternRef p_pattern, MCImage *p_badge);
	virtual void assignbackdrop(Window_mode p_mode, Window p_window);
	
	virtual void hidemenu();
	virtual void showmenu();
	virtual void updatemenubar(Boolean force);
	
	virtual MCColorTransformRef createcolortransform(const MCColorSpaceInfo& info);
	virtual void destroycolortransform(MCColorTransformRef transform);
	virtual bool transformimagecolors(MCColorTransformRef transform, MCImageBitmap *image);
	
	virtual void beep();
	virtual void getbeep(uint4 which, MCExecPoint &ep);
	virtual void setbeep(uint4 which, int4 beep);
	
	virtual MCStack *device_getstackatpoint(int32_t x, int32_t y);
	virtual void device_querymouse(int2 &x, int2 &y);
	virtual void device_setmouse(int2 x, int2 y);
	virtual void grabpointer(Window w);
	virtual void ungrabpointer();
	virtual Boolean istripleclick();
	virtual Boolean abortkey();
	virtual uint2 querymods();
	virtual void getkeysdown(MCExecPoint &ep);
	virtual Boolean getmouse(uint2 button, Boolean& r_abort);
	virtual Boolean getmouseclick(uint2 button, Boolean& r_abort);
	virtual Boolean wait(real8 duration, Boolean dispatch, Boolean anyevent);
	virtual void flushevents(uint2 e);
	
	virtual void clearIME(Window w);
	virtual void openIME();
	virtual void activateIME(Boolean activate);
	virtual void closeIME();
	
	virtual void listprinters(MCExecPoint& ep);
	virtual MCPrinter *createprinter(void);
	
	virtual void flushclipboard(void);
	virtual bool ownsclipboard(void);
	virtual bool setclipboard(MCPasteboard *p_pasteboard);
	virtual MCPasteboard *getclipboard(void);
	
    virtual bool loadfont(const char *p_path, bool p_globally, void*& r_loaded_font_handle);
    virtual bool unloadfont(const char *p_path, bool p_globally, void *r_loaded_font_handle);
	
	virtual MCImageBitmap *snapshot(MCRectangle &r, MCGFloat p_scale_factor, uint4 window, const char *displayname);
	
	virtual MCDragAction dodragdrop(MCPasteboard *p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint* p_image_offset);
	virtual MCScriptEnvironment *createscriptenvironment(const char *p_language);
};

#endif
