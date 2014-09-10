#ifndef __MC_DESKTOP_DC__
#define __MC_DESKTOP_DC__

#ifndef __MC_UIDC__
#include "uidc.h"
#endif

#ifndef __MC_PLATFORM__
#include "platform.h"
#endif

class MCSystemPasteboard: public MCPasteboard
{
public:
	MCSystemPasteboard(MCPlatformPasteboardRef pasteboard);
	~MCSystemPasteboard(void);
	
	virtual void Retain(void);
	virtual void Release(void);	
	
	virtual bool Query(MCTransferType*& r_types, size_t& r_type_count);
	virtual bool Fetch(MCTransferType p_type, MCDataRef& r_data);
	
private:
	bool IsValid(void);
	void Resolve(void);
	void AddEntry(MCTransferType type, MCPlatformPasteboardFlavor flavor);
	
	struct Entry
	{
		MCTransferType type;
		MCPlatformPasteboardFlavor flavor;
		MCDataRef data;
	};
	
	uint32_t m_references;
	
	MCPlatformPasteboardRef m_pasteboard;
	uindex_t m_generation;
	
	MCTransferType *m_types;
	Entry *m_entries;
	uindex_t m_entry_count;
	
	bool m_valid;
};

class MCScreenDC: public MCUIDC
{
private:
	uint2 beeppitch;
	uint2 beepduration;
	Boolean menubarhidden;
	
	bool backdrop_enabled;
	MCColor backdrop_colour;
	MCPatternRef backdrop_pattern;
	MCPlatformWindowRef backdrop_window;
	
	MCPlatformMenuRef icon_menu;
	
public:
	MCScreenDC(void);
	virtual ~MCScreenDC(void);
	
	virtual bool hasfeature(MCPlatformFeature p_feature);
	
	virtual Boolean open();
	virtual Boolean close(Boolean force);

	virtual MCNameRef getdisplayname();
	virtual uint2 getmaxpoints(void);
	virtual uint2 getvclass(void);
	virtual uint2 getdepth(void);
	virtual MCNameRef getvendorname(void);
	virtual uint2 getpad();
	
	virtual MCColor *getaccentcolors();
	
	virtual uint16_t platform_getwidth(void);
	virtual uint16_t platform_getheight(void);
	virtual bool platform_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count);
	virtual void platform_boundrect(MCRectangle &rect, Boolean title, Window_mode m);
	
	virtual void resetcursors();
	virtual void setcursor(Window w, MCCursorRef c);
	virtual MCCursorRef createcursor(MCImageBitmap *image, int2 xhot, int2 yhot);
	virtual void freecursor(MCCursorRef c);
	
	virtual bool platform_getwindowgeometry(Window w, MCRectangle &drect);
	virtual void openwindow(Window w, Boolean override);
	virtual void closewindow(Window window);
	virtual void destroywindow(Window &window);
	virtual void raisewindow(Window window);
	virtual void iconifywindow(Window window);
	virtual void uniconifywindow(Window window);
	virtual void setname(Window window, MCStringRef newname);
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
	virtual void getbeep(uint4 which, int4 &r_beep);
	virtual void setbeep(uint4 which, int4 beep);
	
	virtual MCStack *platform_getstackatpoint(int32_t x, int32_t y);
	virtual void platform_querymouse(int16_t &r_x, int16_t &r_y);
	virtual void platform_setmouse(int16_t p_x, int16_t p_y);
	virtual void grabpointer(Window w);
	virtual void ungrabpointer();
	virtual Boolean istripleclick();
	virtual Boolean abortkey();
	virtual uint2 querymods();
	virtual bool getkeysdown(MCListRef& r_list);
	virtual Boolean getmouse(uint2 button, Boolean& r_abort);
	virtual Boolean getmouseclick(uint2 button, Boolean& r_abort);
	virtual Boolean wait(real8 duration, Boolean dispatch, Boolean anyevent);
	virtual void flushevents(uint2 e);
	
	virtual void clearIME(Window w);
	virtual void openIME();
	virtual void activateIME(Boolean activate);
	virtual void closeIME();
	
	virtual void seticon(uint4 p_icon);
	virtual void seticonmenu(MCStringRef p_menu);
	virtual void configurestatusicon(uint32_t icon_id, MCStringRef menu, MCStringRef tooltip);
	virtual void enactraisewindows(void);
	
	virtual bool listprinters(MCStringRef& r_printers);
	virtual MCPrinter *createprinter(void);
	
	virtual void flushclipboard(void);
	virtual bool ownsclipboard(void);
	virtual bool setclipboard(MCPasteboard *p_pasteboard);
	virtual MCPasteboard *getclipboard(void);
	
	virtual bool loadfont(MCStringRef p_path, bool p_globally, void*& r_loaded_font_handle);
    virtual bool unloadfont(MCStringRef p_path, bool p_globally, void *r_loaded_font_handle);
	
    // SN-2014-07-23: [[ Bug 12907 ]] File > Import as control > Snapshot from screen
    //  Mismatching types - thus the 'unimplemented' MCUICDC::snapshot was called instead of the MCScreenDC one
	virtual MCImageBitmap *snapshot(MCRectangle &r, uint4 window, MCStringRef displayname, MCPoint *size);
	
	virtual MCDragAction dodragdrop(Window w, MCPasteboard *p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint* p_image_offset);
    // SN-2014-07-23: [[ Bug 12907 ]] File > Import as control > Snapshot from screen
    //  Update as well MCSreenDC::createscriptenvironment (and callees)
	virtual MCScriptEnvironment *createscriptenvironment(MCStringRef p_language);
	
	// IM-2014-01-28: [[ HiDPI ]] Return true if the platform can detect
	//   desktop changes and will clear the cache when changes occur.
	virtual bool platform_displayinfocacheable(void);
	
	virtual void controlgainedfocus(MCStack *s, uint32_t id);
	virtual void controllostfocus(MCStack *s, uint32_t id);
    
    // MW-2014-04-26: [[ Bug 5545 ]] Override this method to defer to the MCPlatform method.
    virtual void hidecursoruntilmousemoves(void);
    
	//////////
	
	bool isbackdrop(MCPlatformWindowRef window);
	void redrawbackdrop(MCPlatformSurfaceRef p_surface, MCGRegionRef p_region);
	void mousedowninbackdrop(uint32_t button, uint32_t count);
	void mouseupinbackdrop(uint32_t button, uint32_t count);
	void mousereleaseinbackdrop(uint32_t button);
	
	/////////
	
	bool isiconmenu(MCPlatformMenuRef menu);
};

#endif
