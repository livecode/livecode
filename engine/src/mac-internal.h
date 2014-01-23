#ifndef __MC_MAC_PLATFORM__
#define __MC_MAC_PLATFORM__

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformWindow;
class MCMacPlatformSurface;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCApplicationDelegate: NSObject<NSApplicationDelegate>
{
	int m_argc;
	char **m_argv;
	char **m_envp;
}

// Shutdown and reopening handling
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender;
- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag;

// File opening related requests.
- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename;
- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames;
- (BOOL)application:(NSApplication *)sender openTempFile:(NSString *)filename;
- (BOOL)applicationShouldOpenUntitledFile:(NSApplication *)sender;
- (BOOL)applicationOpenUntitledFile:(NSApplication *)sender;
- (BOOL)application:(id)sender openFileWithoutUI:(NSString *)filename;

// Printing handling
// DEPRECATED - (BOOL)application:(NSApplication *)sender printFile:(NSString *)filename;
- (NSApplicationPrintReply)application:(NSApplication *)application printFiles:(NSArray *)fileNames withSettings:(NSDictionary *)printSettings showPrintPanels:(BOOL)showPrintPanels;

// Dock menu handling.
- (NSMenu *)applicationDockMenu:(NSApplication *)sender;

- (void)applicationWillFinishLaunching:(NSNotification *)notification;
- (void)applicationDidFinishLaunching:(NSNotification *)notification;
- (void)applicationWillHide:(NSNotification *)notification;
- (void)applicationDidHide:(NSNotification *)notification;
- (void)applicationWillUnhide:(NSNotification *)notification;
- (void)applicationDidUnhide:(NSNotification *)notification;
- (void)applicationWillBecomeActive:(NSNotification *)notification;
- (void)applicationDidBecomeActive:(NSNotification *)notification;
- (void)applicationWillResignActive:(NSNotification *)notification;
- (void)applicationDidResignActive:(NSNotification *)notification;
- (void)applicationWillUpdate:(NSNotification *)notification;
- (void)applicationDidUpdate:(NSNotification *)notification;
- (void)applicationWillTerminate:(NSNotification *)notification;
- (void)applicationDidChangeScreenParameters:(NSNotification *)notification;

- (NSError *)application:(NSApplication *)application willPresentError:(NSError *)error;

@end

@compatibility_alias MCApplicationDelegate com_runrev_livecode_MCApplicationDelegate;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCWindowDelegate: NSObject<NSWindowDelegate>
{
	MCMacPlatformWindow *m_window;
}

//////////

- (id)initWithPlatformWindow: (MCMacPlatformWindow *)window;
- (void)dealloc;

- (MCMacPlatformWindow *)platformWindow;

//////////

- (BOOL)windowShouldClose:(id)sender;

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize;
- (void)windowDidMove:(NSNotification *)notification;

- (void)windowDidMiniaturize:(NSNotification *)notification;
- (void)windowDidDeminiaturize:(NSNotification *)notification;

- (void)windowDidBecomeKey:(NSNotification *)notification;
- (void)windowDidResignKey:(NSNotification *)notification;

@end

@compatibility_alias MCWindowDelegate com_runrev_livecode_MCWindowDelegate;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCWindowView: NSView
{
	NSTrackingArea *m_tracking_area;
}

- (id)initWithFrame:(NSRect)frameRect;
- (void)dealloc;

- (void)updateTrackingAreas;

- (BOOL)isFlipped;

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent;
- (BOOL)acceptsFirstResponder;
- (BOOL)becomeFirstResponder;
- (BOOL)resignFirstResponder;

- (void)mouseDown: (NSEvent *)event;
- (void)mouseUp: (NSEvent *)event;
- (void)mouseMoved: (NSEvent *)event;
- (void)mouseDragged: (NSEvent *)event;

- (void)rightMouseDown: (NSEvent *)event;
- (void)rightMouseUp: (NSEvent *)event;
- (void)rightMouseMoved: (NSEvent *)event;
- (void)rightMouseDragged: (NSEvent *)event;

- (void)otherMouseDown: (NSEvent *)event;
- (void)otherMouseUp: (NSEvent *)event;
- (void)otherMouseMoved: (NSEvent *)event;
- (void)otherMouseDragged: (NSEvent *)event;

- (void)mouseEntered: (NSEvent *)event;
- (void)mouseExited: (NSEvent *)event;

- (void)flagsChanged: (NSEvent *)event;

- (void)keyDown: (NSEvent *)event;
- (void)keyUp: (NSEvent *)event;

//////////

- (void)handleMouseMove: (NSEvent *)event;
- (void)handleMousePress: (NSEvent *)event isDown: (BOOL)pressed;
- (void)handleKeyPress: (NSEvent *)event isDown: (BOOL)pressed;

//////////

- (void)drawRect: (NSRect)dirtyRect;

//////////

- (MCRectangle)mapNSRectToMCRectangle: (NSRect)r;
- (NSRect)mapMCRectangleToNSRect: (MCRectangle)r;

@end

@compatibility_alias MCWindowView com_runrev_livecode_MCWindowView;

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformSurface: public MCPlatformSurface
{
public:
	MCMacPlatformSurface(MCMacPlatformWindow *window, CGContextRef cg_context, MCRegionRef update_rgn);
	~MCMacPlatformSurface(void);
	
	virtual bool LockGraphics(MCRegionRef region, MCGContextRef& r_context);
	virtual void UnlockGraphics(void);
	
	virtual bool LockPixels(MCRegionRef region, MCGRaster& r_raster);
	virtual void UnlockPixels(void);
	
	virtual bool LockSystemContext(void*& r_context);
	virtual void UnlockSystemContext(void);
	
	virtual bool Composite(MCGRectangle dst_rect, MCGImageRef src_image, MCGRectangle src_rect, MCGFloat opacity, MCGBlendMode blend);
	
private:
	void Lock(void);
	void Unlock(void);
	
private:
	MCMacPlatformWindow *m_window;
	CGContextRef m_cg_context;
	MCRegionRef m_update_rgn;
	
	MCRectangle m_locked_area;
	MCGContextRef m_locked_context;
	void *m_locked_bits;
	int32_t m_locked_stride;
};

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformWindow: public MCPlatformWindow
{
public:
	MCMacPlatformWindow(void);
	virtual ~MCMacPlatformWindow(void);

	void ProcessCloseRequest();
	void ProcessDidMove(void);
	void ProcessDidResize(void);
	void ProcessDidMiniaturize(void);
	void ProcessDidDeminiaturize(void);
	void ProcessDidBecomeKey(void);
	void ProcessDidResignKey(void);
	
	void ProcessMouseMove(NSPoint location);
	void ProcessMousePress(NSInteger button, bool is_down);
	
	void ProcessKeyDown(MCPlatformKeyCode key_code, codepoint_t unmapped_char, codepoint_t mapped_char);
	void ProcessKeyUp(MCPlatformKeyCode key_code, codepoint_t unmapped_char, codepoint_t mapped_char);
	
protected:
	virtual void DoRealize(void);
	virtual void DoSynchronize(void);
	
	virtual bool DoSetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, const void *value);
	virtual bool DoGetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, void *r_value);
	
	virtual void DoShow(void);
	virtual void DoHide(void);
	virtual void DoFocus(void);
	virtual void DoRaise(void);
	virtual void DoUpdate(void);
	virtual void DoIconify(void);
	virtual void DoUniconify(void);
	
	virtual void DoMapContentRectToFrameRect(MCRectangle content, MCRectangle& r_frame);
	virtual void DoMapFrameRectToContentRect(MCRectangle frame, MCRectangle& r_content);
	
private:
	// Compute the Cocoa window style from the window's current properties.
	void ComputeCocoaStyle(NSUInteger& r_window_style);
	
	// The window delegate object.
	MCWindowDelegate *m_delegate;
	
	// The window content view.
	MCWindowView *m_view;
	
	struct
	{
		// When the mask changes and the window has a shadow we have to
		// explicitly invalidate when updating. If this is true, the Update()
		// method will ensure this is done.
		bool m_shadow_changed : 1;
		
		// When set to true, the window's properties are being synced. This
		// stops events being propagated for state changes caused by script.
		bool m_synchronizing : 1;
	};
	
	// A window might map to one of several different classes, so we use a
	// union for the different types to avoid casts everywhere.
	union 
	{
		id m_handle;
		NSWindow *m_window_handle;
		NSPanel *m_panel_handle;
	};
	
	friend class MCMacPlatformSurface;
};

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformHandleMousePress(uint32_t p_button, bool p_is_down);
void MCMacPlatformHandleMouseMove(MCPoint p_screen_location);

bool MCMacMapKeyCode(uint32_t mac_key_code, MCPlatformKeyCode& r_key_code);
bool MCMacMapNSStringToCodepoint(NSString *string, codepoint_t& r_codepoint);

void MCMacPlatformMapScreenMCPointToNSPoint(MCPoint point, NSPoint& r_point);
void MCMacPlatformMapScreenNSPointToMCPoint(NSPoint point, MCPoint& r_point);

void MCMacPlatformMapScreenMCRectangleToNSRect(MCRectangle rect, NSRect& r_rect);
void MCMacPlatformMapScreenNSRectToMCRectangle(NSRect rect, MCRectangle& r_rect);

////////////////////////////////////////////////////////////////////////////////

#if 0
struct MCPlatformWindowPropertyChanges
{
	bool style_changed : 1;
	bool opacity_changed : 1;
	bool mask_changed : 1;
	bool content_changed : 1;
	bool title_changed : 1;
	bool has_title_widget_changed : 1;
	bool has_close_widget_changed : 1;
	bool has_collapse_widget_changed : 1;
	bool has_zoom_widget_changed : 1;
	bool has_size_widget_changed : 1;
	bool has_shadow_changed : 1;
	bool has_modified_mark_changed : 1;
	bool use_live_resizing_changed : 1;
};

struct __MCPlatformWindow
{
	// The number of references to the window.
	uint32_t references;
	
	// The set of changes since the last sync.
	MCPlatformWindowPropertyChanges changes;
	
	// The window properties that are pending sync.
	MCPlatformWindowStyle style;
	float opacity;
	CGImageRef mask;
	MCRectangle content;
	NSString *title;
	struct
	{
		bool has_title_widget : 1;
		bool has_close_widget : 1;
		bool has_collapse_widget : 1;
		bool has_zoom_widget : 1;
		bool has_size_widget : 1;
		bool has_shadow : 1;
		bool has_modified_mark : 1;
		bool use_live_resizing : 1;
	};
	
	// This is true if show has been called on the window.
	bool is_visible : 1;
	// This is true if the window has been focused.
	bool is_focused : 1;
	
	// If this is true the window shadow needs recomputing on next
	// display.
	bool shadow_changed : 1;
	
	// The underlying Cocoa handle, this could be a window, panel, drawer etc.
	// it depends on the current state of the window.
	union
	{
		id handle;
		NSWindow *handle_as_window;
		NSPanel *handle_as_panel;
	};
};

////////////////////////////////////////////////////////////////////////////////

void MCPlatformSurfaceCreate(MCPlatformWindowRef window, CGContextRef context, MCRegionRef dirty_rgn, MCPlatformSurfaceRef& r_surface);
void MCPlatformSurfaceDestroy(MCPlatformSurfaceRef surface);

bool MCPlatformSurfaceLock(MCPlatformSurfaceRef surface);
void MCPlatformSurfaceUnlock(MCPlatformSurfaceRef surface);

void MCMacPlatformMapScreenMCRectangleToNSRect(MCRectangle rect, NSRect& r_rect);
void MCMacPlatformMapScreenNSRectToMCRectangle(NSRect rect, MCRectangle& r_rect);

////////////////////////////////////////////////////////////////////////////////
#endif

#endif
