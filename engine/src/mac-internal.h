#ifndef __MC_MAC_PLATFORM__
#define __MC_MAC_PLATFORM__

#import <AppKit/NSSound.h>
#import <AppKit/NSColorPanel.h>
#import <AppKit/NSPrintInfo.h>
#include <Carbon/Carbon.h>

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformWindow;
class MCMacPlatformSurface;
class MCMacPlatformMenu;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCPendingAppleEvent: NSObject
{
    AppleEvent m_event;
    AppleEvent m_reply;
}

- (id)initWithEvent: (const AppleEvent *)event andReply: (AppleEvent *)reply;
- (void)dealloc;

- (OSErr)process;
@end

@compatibility_alias MCPendingAppleEvent com_runrev_livecode_MCPendingAppleEvent;

@interface com_runrev_livecode_MCApplicationDelegate: NSObject<NSApplicationDelegate>
{
	int m_argc;
	MCStringRef *m_argv;
	MCStringRef *m_envp;
    
    bool m_explicit_quit : 1;
    bool m_running : 1;
    
    NSMutableArray *m_pending_apple_events;
    
    MCPlatformCoreRef m_platform;
}

@property (nonatomic, assign) MCPlatformCoreRef platform;

// Platform init / finit.
- (void)initializeModules;
- (void)finalizeModules;

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

// MW-2014-04-22: [[ Bug 12259 ]] Override sendEvent so that we always get a chance
//   at the MouseSync event.
@interface com_runrev_livecode_MCApplication: NSApplication

- (void)sendEvent:(NSEvent *)event;

@end

@protocol com_runrev_livecode_MCMovingFrame <NSObject>

- (NSRect)movingFrame;
- (void)setMovingFrame:(NSRect)p_moving_frame;

@end

@interface com_runrev_livecode_MCWindow: NSWindow <com_runrev_livecode_MCMovingFrame>
{
	bool m_can_become_key : 1;
    NSRect m_moving_frame;
}

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation;

- (void)setCanBecomeKeyWindow: (BOOL)value;

- (BOOL)canBecomeKeyWindow;
- (BOOL)makeFirstResponder: (NSResponder *)responder;

// MW-2014-04-23: [[ Bug 12270 ]] Override so we can stop constraining.
- (NSRect)constrainFrameRect: (NSRect)frameRect toScreen: (NSScreen *)screen;

@end

void MCMacPlatformWindowWindowMoved(NSWindow *p_self, MCPlatformWindowRef p_window);

@interface com_runrev_livecode_MCPanel: NSPanel  <com_runrev_livecode_MCMovingFrame>
{
	bool m_can_become_key : 1;
    bool m_is_popup : 1;
    id m_monitor;
    NSRect m_moving_frame;
}

- (void)setCanBecomeKeyWindow: (BOOL)value;
- (void)dealloc;

- (BOOL)canBecomeKeyWindow;
- (BOOL)makeFirstResponder: (NSResponder *)responder;

// MW-2014-04-23: [[ Bug 12270 ]] Override so we can stop constraining.
- (NSRect)constrainFrameRect: (NSRect)frameRect toScreen: (NSScreen *)screen;
- (void)popupAndMonitor;

@end

bool MCMacPlatformApplicationSendEvent(NSEvent *p_event);

////////////////////////////////////////////////////////////////////////////////

// SN-2014-12-05: [[ Bug 14019 ]] Interface declaration moved to be available from mac-menu.mm

// SN-2014-10-20: [[ Bug 13628 ]] ColorDelegate to react when the colour picker window is closed
@interface com_runrev_livecode_MCColorPanelDelegate: NSObject<NSWindowDelegate>
{
    NSButton *mCancelButton;
    NSButton *mOkButton;
    NSView   *mColorPickerView;
    NSView   *mUpdatedView;
    NSColorPanel *mColorPanel;
    
    MCPlatformDialogResult mResult;
    MCColor mColorPicked;
	id eventMonitor;
}

-(id)   initWithColorPanel: (NSColorPanel*)p_panel
               contentView: (NSView*) p_view;
-(void) dealloc;
-(void) windowDidBecomeKey:(NSNotification *)notification;
-(void) windowWillClose: (NSNotification *)notification;
-(void) windowDidResize:(NSNotification *)notification;
-(void) getColor;
//-(void) changeColor:(id)sender;
-(void) pickerCancelClicked;
-(void) pickerOkClicked;
-(void) processEscKeyDown;
-(void) relayout;

@end

@compatibility_alias MCColorPanelDelegate com_runrev_livecode_MCColorPanelDelegate;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCWindowDelegate: NSObject<NSWindowDelegate>
{
	MCMacPlatformWindow *m_window;
    
    // MW-2014-04-23: [[ Bug 12270 ]] If true the size / position of the window is
    //   being changed by the user.
    bool m_user_reshape : 1;
}

//////////

- (id)initWithPlatformWindow: (MCMacPlatformWindow *)window;
- (void)dealloc;

- (MCMacPlatformWindow *)platformWindow;

// MW-2014-04-23: [[ Bug 12270 ]] Returns the value of 'm_user_reshape'
- (bool)inUserReshape;

- (void)windowMoveFinished;

//////////

- (BOOL)windowShouldClose:(id)sender;

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize;

- (void)windowWillMove:(NSNotification *)notification;
- (void)windowDidMove:(NSNotification *)notification;

- (void)windowWillStartLiveResize:(NSNotification *)notification;
- (void)windowDidEndLiveResize:(NSNotification *)notification;

- (void)windowDidMiniaturize:(NSNotification *)notification;
- (void)windowDidDeminiaturize:(NSNotification *)notification;

- (void)windowDidBecomeKey:(NSNotification *)notification;
- (void)windowDidResignKey:(NSNotification *)notification;

- (void)windowDidChangeBackingProperties:(NSNotification *)notification;

//////////

- (void)viewFocusSwitched: (uint32_t)id;

@end

@compatibility_alias MCWindowDelegate com_runrev_livecode_MCWindowDelegate;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCWindowContainerView: NSView
{
    MCMacPlatformWindow *m_window;
}

- (id)initWithPlatformWindow:(MCMacPlatformWindow *)window;

- (void)setFrameSize: (NSSize)size;

@end

@compatibility_alias MCWindowContainerView com_runrev_livecode_MCWindowContainerView;

@interface com_runrev_livecode_MCWindowView: NSView<NSTextInputClient>
{
    MCMacPlatformWindow *m_window;
    
	NSTrackingArea *m_tracking_area;
    
    // The last event that was passed to the IME.
	NSEvent *m_input_method_event;
    // Whether to pass events through the IME or not.
    bool m_use_input_method : 1;
    
	NSDragOperation m_allowed_drag_operations;
}

- (id)initWithPlatformWindow:(MCMacPlatformWindow *)window;
- (void)dealloc;

- (void)updateTrackingAreas;

- (BOOL)isFlipped;

- (BOOL)canBecomeKeyView;

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

- (BOOL)useTextInput;

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange;
- (void)doCommandBySelector:(SEL)aSelector;
- (void)setMarkedText:(id)aString selectedRange:(NSRange)newSelection replacementRange:(NSRange)replacementRange;
- (void)unmarkText;
- (NSRange)selectedRange;
- (NSRange)markedRange;
- (BOOL)hasMarkedText;
- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange;
- (NSArray*)validAttributesForMarkedText;
- (NSRect)firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange;
- (NSUInteger)characterIndexForPoint:(NSPoint)aPoint;

//////////

- (void)undo:(id)sender;
- (void)redo:(id)sender;
- (void)cut:(id)sender;
- (void)copy:(id)sender;
- (void)paste:(id)sender;
- (void)selectAll:(id)sender;
- (void)delete:(id)sender;

//////////

- (BOOL)wantsPeriodicDraggingUpdates;
- (NSDragOperation)draggingEntered: (id<NSDraggingInfo>)sender;
- (void)draggingExited: (id<NSDraggingInfo>)sender;
- (NSDragOperation)draggingUpdated: (id<NSDraggingInfo>)sender;

- (BOOL)performDragOperation: (id<NSDraggingInfo>)sender;
- (BOOL)prepareForDragOperation: (id<NSDraggingInfo>)sender;
- (void)concludeDragOperation:(id<NSDraggingInfo>)sender;

//////////

- (BOOL)shouldDelayWindowOrderingForEvent: (NSEvent *)event;
- (NSDragOperation)draggingSourceOperationMaskForLocal: (BOOL)isLocal;
- (BOOL)ignoreModifierKeysWhileDragging;
- (void)draggedImage:(NSImage *)image beganAt:(NSPoint)point;
- (void)draggedImage:(NSImage *)image movedTo:(NSPoint)point;
- (void)draggedImage:(NSImage *)image endedAt:(NSPoint)point operation:(NSDragOperation)operation;
- (NSDragOperation)dragImage:(NSImage *)image offset:(NSSize)offset allowing:(NSDragOperation)operations pasteboard:(NSPasteboard *)pboard;

//////////

- (void)handleMouseMove: (NSEvent *)event;
- (void)handleMousePress: (NSEvent *)event isDown: (BOOL)pressed;
- (void)handleKeyPress: (NSEvent *)event isDown: (BOOL)pressed;
- (void)handleAction:(SEL)selector with:(id)sender;

//////////

- (void)setFrameSize: (NSSize)size;
- (void)drawRect: (NSRect)dirtyRect;

//////////

- (MCRectangle)mapNSRectToMCRectangle: (NSRect)r;
- (NSRect)mapMCRectangleToNSRect: (MCRectangle)r;

- (MCPoint)mapNSPointToMCPoint: (NSPoint)r;
- (NSPoint)mapMCPointToNSPoint: (MCPoint)r;

@end

@compatibility_alias MCWindowView com_runrev_livecode_MCWindowView;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCMenuDelegate: NSObject<NSMenuDelegate>
{
	MCMacPlatformMenu * m_menu;
}

//////////

- (id)initWithPlatformMenuRef: (MCMacPlatformMenu *)p_menu_ref;
- (void)dealloc;

//////////

- (MCMacPlatformMenu *)platformMenuRef;

//////////

- (void)menuNeedsUpdate: (NSMenu *)menu;
- (void)menuItemSelected: (id)sender;

- (BOOL)validateMenuItem: (NSMenuItem *)item;

- (BOOL)worksWhenModal;

- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action;

@end

@compatibility_alias MCMenuDelegate com_runrev_livecode_MCMenuDelegate;

@interface com_runrev_livecode_MCAppMenuDelegate: NSObject<NSMenuDelegate>

- (id)init;
- (void)dealloc;

- (void)shadowedMenuItemSelected:(NSString*)tag;
- (NSMenuItem *)findShadowedMenuItem: (NSString *)tag;

- (void)aboutMenuItemSelected: (id)sender;
- (void)preferencesMenuItemSelected: (id)sender;
// SN-2014-11-06: [[ Bug 13940 ]] Added declaration for quitMenuItemSelected
//  and quitApplicationSelected, the latter quitting the app straight.
- (void)quitMenuItemSelected: (id)sender;
- (void)quitApplicationSelected: (id)sender;

- (void)menuNeedsUpdate: (NSMenu *)menu;

- (BOOL)validateMenuItem: (NSMenuItem *)item;

- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action;

@end

@compatibility_alias MCAppMenuDelegate com_runrev_livecode_MCAppMenuDelegate;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCAbortKeyThread: NSThread<NSPortDelegate>
{
    NSPort *m_termination_port;
    BOOL m_is_running;
    BOOL m_abort_key_pressed;
    BOOL m_abort_key_checked;
    BOOL m_current_period_needs_shift; // = false;
    TISInputSourceRef m_current_input_source;
    CGKeyCode m_current_period_keycode; // = 0xffff;
}

@property (nonatomic, assign) BOOL abortKeyChecked;
@property (nonatomic, assign) BOOL abortKeyPressed;
@property (nonatomic, assign) BOOL currentPeriodNeedsShift;
@property (nonatomic, assign) TISInputSourceRef currentInputSource;
@property (nonatomic, assign) CGKeyCode currentPeriodKeycode;

- (void)main;
- (void)terminate;

- (void)handlePortMessage: (NSPortMessage *)message;

@end;

@compatibility_alias MCAbortKeyThread com_runrev_livecode_MCAbortKeyThread;

////////////////////////////////////////////////////////////////////////////////

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use the new platform surface API.
class MCMacPlatformSurface: public MCPlatformSurface
{
public:
	MCMacPlatformSurface(MCMacPlatformWindow *window, CGContextRef cg_context, MCGRegionRef update_rgn);
	~MCMacPlatformSurface(void);
	
	virtual bool LockGraphics(MCGIntegerRectangle area, MCGContextRef& r_context, MCGRaster &r_raster);
	virtual void UnlockGraphics(MCGIntegerRectangle area, MCGContextRef context, MCGRaster &raster);
	
	virtual bool LockPixels(MCGIntegerRectangle area, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area);
	virtual void UnlockPixels(MCGIntegerRectangle area, MCGRaster& raster);
	
	virtual bool LockSystemContext(void*& r_context);
	virtual void UnlockSystemContext(void);
	
	virtual bool Composite(MCGRectangle dst_rect, MCGImageRef src_image, MCGRectangle src_rect, MCGFloat opacity, MCGBlendMode blend);
	
	virtual MCGFloat GetBackingScaleFactor(void);
	   
private:
    void Lock(void);
	void Unlock(void);
	
	// IM-2014-10-03: [[ Bug 13432 ]] Convenience method to clear context and clip to the window mask
	void ApplyMaskToCGContext(void);
    
	MCMacPlatformWindow *m_window;
	CGContextRef m_cg_context;
	MCGRegionRef m_update_rgn;
    
    MCGRaster m_raster;
	
	bool m_cg_context_first_lock;
	
	bool m_opaque;
};

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformWindow: public MCPlatformWindow
{
public:
	MCMacPlatformWindow(void);
	virtual ~MCMacPlatformWindow(void);

	MCWindowView *GetView(void);
    MCWindowContainerView *GetContainerView(void);
    
	id GetHandle(void);
	
    bool IsSynchronizing(void);
    
	void ProcessCloseRequest(void);
	void ProcessDidMove(void);
	void ProcessDidResize(void);
	void ProcessWillMiniaturize(void);
	void ProcessDidMiniaturize(void);
	void ProcessDidDeminiaturize(void);
	void ProcessDidBecomeKey(void);
	void ProcessDidResignKey(void);
	
	void ProcessMouseMove(NSPoint location);
	void ProcessMousePress(NSInteger button, bool is_down);
	void ProcessMouseScroll(CGFloat dx, CGFloat dy);
	
    // SN-2014-07-11: [[ Bug 12747 ]] Shortcuts: the uncomment script shortcut cmd _ does not work
    // Changed parameters order to follow *KeyDown functions consistency
	void ProcessKeyDown(MCPlatformKeyCode key_code, codepoint_t mapped_char, codepoint_t unmapped_char);
	void ProcessKeyUp(MCPlatformKeyCode key_code, codepoint_t mapped_char, codepoint_t unmapped_char);
    
	void MapMCPointToNSPoint(MCPoint location, NSPoint& r_ns_location);
	void MapNSPointToMCPoint(NSPoint location, MCPoint& r_mc_location);
	
	void MapMCRectangleToNSRect(MCRectangle rect, NSRect& r_ns_rect);
	void MapNSRectToMCRectangle(NSRect rect, MCRectangle& r_mc_rect);
	
	// IM-2015-01-30: [[ Bug 14140 ]] Locking the frame will prevent the window from being moved or resized
	void SetFrameLocked(bool p_locked);
	
protected:
	virtual void DoRealize(void);
	virtual void DoSynchronize(void);
	
	virtual bool DoSetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, const void *value);
	virtual bool DoGetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, void *r_value);
	
	virtual void DoShow(void);
	virtual void DoShowAsSheet(MCPlatformWindowRef parent);
	virtual void DoHide(void);
	virtual void DoFocus(void);
	virtual void DoRaise(void);
	virtual void DoUpdate(void);
	virtual void DoIconify(void);
	virtual void DoUniconify(void);
	
	virtual void DoConfigureTextInput(void);
	virtual void DoResetTextInput(void);
	
public:
	virtual void DoMapContentRectToFrameRect(MCRectangle content, MCRectangle& r_frame);
	virtual void DoMapFrameRectToContentRect(MCRectangle frame, MCRectangle& r_content);
	
private:
	// Compute the Cocoa window style from the window's current properties.
	void ComputeCocoaStyle(NSUInteger& r_window_style);
    // MERG-2015-10-11: [[ DocumentFilename ]] Set documentFilename.
    void UpdateDocumentFilename(void);
    
	// The window delegate object.
	MCWindowDelegate *m_delegate;
	
	// The window container view.
	MCWindowContainerView *m_container_view;
	
    // The window's content view.
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
		
		// When set to true, the window has a sheet.
		bool m_has_sheet : 1;
		
		// When the frame is locked, any changes to the window rect will be prevented.
		bool m_frame_locked : 1;
	};
	
	// A window might map to one of several different classes, so we use a
	// union for the different types to avoid casts everywhere.
	union 
	{
		id m_handle;
		NSWindow *m_window_handle;
		NSPanel *m_panel_handle;
	};
	
	// The parent pointer for sheets and drawers.
	MCPlatformWindowRef m_parent;
	
	friend class MCMacPlatformSurface;
};

////////////////////////////////////////////////////////////////////////////////

bool MCMacPlatformMapKeyCode(uint32_t mac_key_code, uint32_t modifier_flags, MCPlatformKeyCode& r_key_code);

bool MCMacMapNSStringToCodepoint(NSString *string, codepoint_t& r_codepoint);
bool MCMacMapCodepointToNSString(codepoint_t p_codepoint, NSString*& r_string);
bool MCMacMapSelectorToTextInputAction(SEL p_selector, MCPlatformTextInputAction& r_action);

void MCMacPlatformMapScreenMCPointToNSPoint(MCPoint point, NSPoint& r_point);
void MCMacPlatformMapScreenNSPointToMCPoint(NSPoint point, MCPoint& r_point);

void MCMacPlatformMapScreenMCRectangleToNSRect(MCRectangle rect, NSRect& r_rect);
void MCMacPlatformMapScreenNSRectToMCRectangle(NSRect rect, MCRectangle& r_rect);

MCPlatformModifiers MCMacPlatformMapNSModifiersToModifiers(NSUInteger p_modifiers);

// MW-2014-04-23: [[ CocoaBackdrop ]] Ensures the windows are stacked correctly.
void MCMacPlatformSyncBackdrop(void);

////////////////////////////////////////////////////////////////////////////////

NSDragOperation MCMacPlatformMapDragOperationToNSDragOperation(MCPlatformDragOperation);
MCPlatformDragOperation MCMacPlatformMapNSDragOperationToDragOperation(NSDragOperation);

void MCMacPlatformPasteboardCreate(NSPasteboard *pasteboard, MCPlatformPasteboardRef& r_pasteboard);

////////////////////////////////////////////////////////////////////////////////

// IM-2014-09-29: [[ Bug 13451 ]] Return the standard colorspace for images on OSX
bool MCMacPlatformGetImageColorSpace(CGColorSpaceRef &r_colorspace);

////////////////////////////////////////////////////////////////////////////////

// IM-2014-10-03: [[ Bug 13432 ]] Store both alpha data and derived cg image in the mask.
class MCMacPlatformWindowMask: public MCPlatformWindowMask
{
public:
    MCMacPlatformWindowMask(void);
    virtual ~MCMacPlatformWindowMask(void);
    
    virtual bool IsValid(void) const;
    
    virtual bool CreateWithAlphaAndRelease(int32_t p_width, int32_t p_height, int32_t p_stride, void *p_bits);
    
private:
	MCGRaster m_mask;
	CGImageRef m_cg_mask;
    
    friend MCMacPlatformSurface;
};

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformLoadedFont: public MCPlatformLoadedFont
{
public:
    MCMacPlatformLoadedFont();
    ~MCMacPlatformLoadedFont(void);
    virtual bool CreateWithPath(MCStringRef p_path, bool p_globally);
private:
    MCAutoStringRef m_path;
    bool m_globally;
};

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformCursor: public MCPlatformCursor
{
public:
    MCMacPlatformCursor(void);
    ~MCMacPlatformCursor(void);
    virtual void CreateStandard(MCPlatformStandardCursor p_standard_cursor);
    virtual void CreateCustom(MCImageBitmap *p_image, MCPoint p_hotspot);
    virtual void Set(void);
private:
    bool is_standard;
    MCPlatformStandardCursor standard;
    NSCursor *custom;
};


////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformColorTransform: public MCPlatformColorTransform
{
public:
    MCMacPlatformColorTransform();
    virtual ~MCMacPlatformColorTransform(void);
    virtual bool Apply(MCImageBitmap *p_image);
    virtual bool CreateWithColorSpace(const MCColorSpaceInfo& p_info);
private:
    CGColorSpaceRef m_colorspace;
};


////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformPrintDialogSession: public MCPlatformPrintDialogSession
{
public:
    MCMacPlatformPrintDialogSession();
    virtual ~MCMacPlatformPrintDialogSession(void);
    virtual void BeginPageSetup(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format);
    virtual void BeginSettings(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format);
    virtual void CopyInfo(void *&r_session, void *&r_settings, void *&r_page_format);
    virtual void SetResult(MCPlatformPrintDialogResult p_result);
    virtual MCPlatformPrintDialogResult GetResult(void);
private:
    virtual void Begin(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format, id p_panel);
    
    MCPlatformPrintDialogResult m_result;
    MCPlatformWindowRef m_owner;
    
    NSPrintInfo *m_info;
    PMPrintSession m_session;
    PMPrintSettings m_settings;
    PMPageFormat m_page_format;
};

////////////////////////////////////////////////////////////////////////////////


struct MCCallback
{
    void (*method)(void *);
    void *context;
};

struct MCModalSession
{
    NSModalSession session;
    MCMacPlatformWindow *window;
    bool is_done;
};

class MCMacPlatformCore: public MCPlatformCore
{
public:
    MCMacPlatformCore();
    virtual ~MCMacPlatformCore(void);
    
    virtual int Run(int argc, char *argv[], char *envp[]);
    
    // Wait
    virtual bool WaitForEvent(double p_duration, bool p_blocking);
    virtual void BreakWait(void);
    virtual bool InBlockingWait(void);
    
    // Callbacks
    virtual void ScheduleCallback(void (*p_callback)(void *), void *p_context);
    
    // Abort key
    virtual bool InitializeAbortKey(void);
    virtual void FinalizeAbortKey(void);
    virtual bool GetAbortKeyPressed(void);
    
    // Color transform
    virtual bool InitializeColorTransform(void);
    virtual void FinalizeColorTransform(void);
    
    // Menus
    virtual MCPlatformMenuRef CreateMenu(void);
    virtual bool InitializeMenu(void);
    virtual void FinalizeMenu(void);
    virtual void ShowMenubar(void);
    virtual void HideMenubar(void);
    virtual void SetMenubar(MCPlatformMenuRef p_menu);
    virtual MCPlatformMenuRef GetMenubar(void);
    virtual void SetIconMenu(MCPlatformMenuRef p_menu);
    virtual void SaveQuittingState();
    virtual void PopQuittingState();
    virtual bool IsInQuittingState(void);
    virtual void LockMenuSelect(void);
    virtual void UnlockMenuSelect(void);
    virtual NSMenu *GetIconMenu(void);
    virtual bool MapMenuItemActionToSelector(MCPlatformMenuItemAction action, SEL& r_selector);
    
    virtual void ShowMessageDialog(MCStringRef p_title, MCStringRef p_message);
    
    // Windows
    virtual MCPlatformWindowRef CreateWindow(void);
    virtual bool GetWindowWithId(uint32_t p_id, MCPlatformWindowRef& r_window);
    virtual void BeginModalSession(MCMacPlatformWindow *p_window);
    virtual void EndModalSession(MCMacPlatformWindow *p_window);
    virtual bool ApplicationWindowIsMoving(MCPlatformWindowRef p_window);
    virtual void ApplicationWindowStartedMoving(MCPlatformWindowRef p_window);
    virtual void ApplicationWindowStoppedMoving(MCPlatformWindowRef p_window);
    virtual void ApplicationBecomePseudoModalFor(NSWindow *p_window);
    virtual NSWindow *ApplicationPseudoModalFor(void);
    virtual bool ApplicationSendEvent(NSEvent *p_event);
    
    // Color dialog
    virtual void BeginColorDialog(MCStringRef p_title, const MCColor& p_color);
    virtual MCPlatformDialogResult EndColorDialog(MCColor& r_color);
    
    // System Properties
    virtual void GetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *r_value);
    virtual void SetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *p_value);

    // Event checking
    virtual void EnableEventChecking(void);
    virtual void DisableEventChecking(void);
    virtual bool IsEventCheckingEnabled(void);
    
    // Player
    virtual MCPlatformPlayerRef CreatePlayer(void);
    
    // Snapshots
    virtual void ScreenSnapshotOfUserArea(MCPoint *p_size, MCImageBitmap*& r_bitmap);
    virtual void ScreenSnapshotOfWindow(uint32_t p_window_id, MCPoint *p_size, MCImageBitmap*& r_bitmap);
    virtual void ScreenSnapshotOfWindowArea(uint32_t p_window_id, MCRectangle p_area, MCPoint *p_size, MCImageBitmap*& r_bitmap);
    virtual void ScreenSnapshot(MCRectangle p_screen_rect, MCPoint *p_size, MCImageBitmap*& r_bitmap);
    virtual void SetSnapshotPoints(CGPoint p_start_point, CGPoint p_end_point);
    virtual void SetDisplayLinkFired(bool p_fired);
    
    virtual void Beep(void);
    
    virtual void DeathGrip(MCPlatform::Base * p_pointer);
    
    // Events
    virtual void FlushEvents(MCPlatformEventMask p_mask);
    virtual uint32_t GetEventTime(void);
    
    // Mice
    virtual void HideCursorUntilMouseMoves(void);
    virtual void ResetCursor(void);
    virtual NSEvent *GetLastMouseEvent(void);
    virtual bool GetMouseButtonState(uindex_t p_button);
    virtual bool GetMouseClick(uindex_t p_button, MCPoint& r_location);
    virtual void GetMousePosition(MCPoint& r_location);
    virtual void SetMousePosition(MCPoint p_location);
    virtual void GetWindowAtPoint(MCPoint p_loc, MCPlatformWindowRef& r_window);
    virtual void LockCursor(void);
    virtual void UnlockCursor(void);
    virtual void GrabPointer(MCPlatformWindowRef p_window);
    virtual void UngrabPointer(void);
    virtual void HandleMousePress(uint32_t p_button, bool p_new_state);
    virtual void HandleMouseCursorChange(MCPlatformWindowRef p_window);
    virtual void HandleMouseAfterWindowHidden(void);
    virtual void HandleMouseForResizeStart(void);
    virtual void HandleMouseForResizeEnd(void);
    virtual void HandleMouseMove(MCPoint p_screen_loc);
    virtual void HandleMouseScroll(CGFloat dx, CGFloat dy);
    virtual void HandleMouseSync(void);
    virtual void SyncMouseBeforeDragging(void);
    virtual void SyncMouseAfterTracking(void);
    virtual void HandleModifiersChanged(MCPlatformModifiers p_modifiers);
    
    // Modifier Keys
    virtual MCPlatformModifiers GetModifiersState(void);
    virtual bool GetKeyState(MCPlatformKeyCode*& r_codes, uindex_t& r_code_count);
    
    // Drag and drop
    virtual void DoDragDrop(MCPlatformWindowRef p_window, MCPlatformAllowedDragOperations p_allowed_operations, MCImageBitmap *p_image, const MCPoint *p_image_loc, MCPlatformDragOperation& r_operation);
private:
    // Sound
    void GetGlobalVolume(double& r_volume);
    void SetGlobalVolume(double p_volume);
    
    uindex_t m_event_checking_enabled;
    
    // Abort key
    MCAbortKeyThread *m_abort_key_thread;
    
    // Windows
    MCPlatformWindowRef m_moving_window;
    NSWindow *m_pseudo_modal_for;
    MCModalSession *m_modal_sessions;
    uindex_t m_modal_session_count;
    
    // Wait
    bool m_in_blocking_wait : 1;
    CFRunLoopObserverRef m_observer;
    bool m_wait_broken : 1;

    // Callbacks
    NSLock *m_callback_lock;
    MCCallback *m_callbacks;
    uindex_t m_callback_count;
    
    // Snapshots
    void ScreenSnapshotOfWindowWithinBounds(uint32_t p_window_id, MCRectangle p_bounds, MCPoint *p_size, MCImageBitmap *&r_bitmap);
    void WaitForDisplayRefresh(void);
    CGPoint m_snapshot_start_point;
    CGPoint m_snapshot_end_point;
    bool m_snapshot_done : 1;
    bool m_display_link_fired : 1;

    // Mice
    NSEvent *m_last_mouse_event;
    
    // If this is true, then the mouse is currently grabbed so we should defer
    // switching active window until ungrabbed.
    bool m_mouse_grabbed : 1;
    
    // If this is true there was an explicit request for grabbing.
    bool m_mouse_grabbed_explicit;
    
    // This is the currently active window (the one receiving mouse events).
    MCPlatformWindowRef m_mouse_window;
    
    // This is the current mask of buttons that are pressed.
    uint32_t m_mouse_buttons;
    
    // This is the button that is being dragged (if not 0xffffffff).
    uint32_t m_mouse_drag_button;
    
    // This is the number of successive clicks detected on the primary button.
    uint32_t m_mouse_click_count;
    
    // This is the button of the last click (mouseDown then mouseUp) that was
    // detected.
    uint32_t m_mouse_last_click_button;
    
    // This is the time of the last mouseUp, used to detect multiple clicks.
    uint32_t m_mouse_last_click_time;
    
    // This is the screen position of the last click, used to detect multiple
    // clicks.
    MCPoint m_mouse_last_click_screen_position;
    
    // This is the window location in the mouse window that we last posted
    // a position event for.
    MCPoint m_mouse_position;
    
    // This is the last screen location we received a mouse move message for.
    MCPoint m_mouse_screen_position;
    
    // This is the current modifier state, and whether the control key was down
    // for a button 0 press.
    MCPlatformModifiers m_mouse_modifiers;
    bool m_mouse_was_control_click : 1;
    
    // MW-2014-06-11: [[ Bug 12436 ]] This is used to temporarily turn off cursor setting
    //   when doing an user-import snapshot.
    bool m_mouse_cursor_locked : 1;
};

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformSound: public MCPlatformSound
{
public:
    MCMacPlatformSound(void);
    ~MCMacPlatformSound(void);
    
    virtual bool IsValid(void) const;
    
    virtual bool CreateWithData(const void *data, size_t data_size);
    
    virtual bool IsPlaying(void) const;
    
    virtual void Play(void);
    virtual void Pause(void);
    virtual void Resume(void);
    virtual void Stop(void);
    
    virtual void SetProperty(MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value);
    virtual void GetProperty(MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value);

private:
    NSSound *m_sound;
};

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformMenu: public MCPlatformMenu
{
public:
    MCMacPlatformMenu(void);
    virtual ~MCMacPlatformMenu(void);
    
    virtual void SetTitle(MCStringRef p_title);
    virtual uindex_t CountItems(void);
    virtual void AddItem(uindex_t p_where);
    virtual void AddSeparatorItem(uindex_t p_where);
    virtual void RemoveItem(uindex_t p_where);
    virtual void RemoveAllItems(void);
    virtual void GetParent(MCPlatformMenuRef& r_parent, uindex_t& r_index);
    virtual void GetItemProperty(uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, void *r_value);
    virtual void SetItemProperty(uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, const void *p_value);
    virtual bool PopUp(MCPlatformWindowRef p_window, MCPoint p_location, uindex_t p_item);
    virtual void StartUsingAsMenubar(void);
    virtual void StopUsingAsMenubar(void);

    virtual NSMenu* GetMenu(){ return menu;}
    virtual NSMenuItem* GetQuitItem(){ return quit_item;}
 private:
    void DestroyMenuItem(uindex_t p_index);
    void MapMenuItemIndex(uindex_t& x_index);
    void ClampMenuItemIndex(uindex_t& x_index);
    
    NSMenu *menu;
    MCMenuDelegate *menu_delegate;
    
    // If the menu is being used as a menubar then this is true. When this
    // is the case, some items will be hidden and a (API-wise) invisible
    // menu will be inserted at the front (the application menu).
    bool is_menubar : 1;
    
    // If the quit item in this menu has an accelerator, this is true.
    // (Cocoa seems to 'hide' the quit menu item accelerator for some inexplicable
    // reason - it returns 'empty').
    NSMenuItem* quit_item;
    
    // SN-2014-11-06: [[ Bu 13940 ]] Add a flag for the presence of a Preferences shortcut
    //  to allow the menu item to be disabled.
    NSMenuItem* preferences_item;
    
    NSMenuItem* about_item;
};

////////////////////////////////////////////////////////////////////////////////

// The function pointer for objc_msgSend_fpret needs to be cast in order
// to get the correct return type, otherwise we can get strange results
// on x86_64 because "long double" return values are returned in
// different registers to "float" or "double".
extern "C" void objc_msgSend_fpret(void);
template <class R, class... Types> R objc_msgSend_fpret_type(id p_id, SEL p_sel, Types... p_params)
{
    // Cast the obj_msgSend_fpret function to the correct type
    R (*t_send)(id, SEL, ...) = reinterpret_cast<R (*)(id, SEL, ...)> (&objc_msgSend_fpret);
    
    // Perform the call
    return t_send(p_id, p_sel, p_params...);
}

////////////////////////////////////////////////////////////////////////////////

#endif
