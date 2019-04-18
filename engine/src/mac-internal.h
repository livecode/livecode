#ifndef __MC_MAC_PLATFORM__
#define __MC_MAC_PLATFORM__

#import <AppKit/NSColorPanel.h>

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformWindow;
class MCMacPlatformSurface;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCPendingAppleEvent: NSObject
{
    AppleEvent m_event;
    AppleEvent m_reply;
}

- (id)initWithEvent: (const AppleEvent *)event andReply: (AppleEvent *)reply;
- (void)dealloc;

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
}

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
bool MCMacPlatformApplicationWindowIsMoving(MCPlatformWindowRef p_window);
void MCMacPlatformApplicationWindowStartedMoving(MCPlatformWindowRef p_window);
void MCMacPlatformApplicationWindowStoppedMoving(MCPlatformWindowRef p_window);
void MCMacPlatformApplicationBecomePseudoModalFor(NSWindow *p_window);
NSWindow *MCMacPlatformApplicationPseudoModalFor(void);

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
	MCPlatformMenuRef m_menu;
}

//////////

- (id)initWithPlatformMenuRef: (MCPlatformMenuRef)p_menu_ref;
- (void)dealloc;

//////////

- (MCPlatformMenuRef)platformMenuRef;

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

void MCMacPlatformScheduleCallback(void (*)(void*), void *);

void MCMacPlatformBeginModalSession(MCMacPlatformWindow *window);
void MCMacPlatformEndModalSession(MCMacPlatformWindow *window);

void MCMacPlatformHandleMouseCursorChange(MCPlatformWindowRef window);
void MCMacPlatformHandleMousePress(uint32_t p_button, bool p_is_down);
void MCMacPlatformHandleMouseMove(MCPoint p_screen_location);
void MCMacPlatformHandleMouseScroll(CGFloat dx, CGFloat dy);
void MCMacPlatformHandleMouseSync(void);
void MCMacPlatformHandleMouseAfterWindowHidden(void);

void MCMacPlatformHandleMouseForResizeStart(void);
void MCMacPlatformHandleMouseForResizeEnd(void);

void MCMacPlatformSyncMouseBeforeDragging(void);
void MCMacPlatformSyncMouseAfterTracking(void);

void MCMacPlatformHandleModifiersChanged(MCPlatformModifiers modifiers);

bool MCMacPlatformMapKeyCode(uint32_t mac_key_code, uint32_t modifier_flags, MCPlatformKeyCode& r_key_code);

bool MCMacMapNSStringToCodepoint(NSString *string, codepoint_t& r_codepoint);
bool MCMacMapCodepointToNSString(codepoint_t p_codepoint, NSString*& r_string);
bool MCMacMapSelectorToTextInputAction(SEL p_selector, MCPlatformTextInputAction& r_action);

void MCMacPlatformMapScreenMCPointToNSPoint(MCPoint point, NSPoint& r_point);
void MCMacPlatformMapScreenNSPointToMCPoint(NSPoint point, MCPoint& r_point);

void MCMacPlatformMapScreenMCRectangleToNSRect(MCRectangle rect, NSRect& r_rect);
void MCMacPlatformMapScreenNSRectToMCRectangle(NSRect rect, MCRectangle& r_rect);

MCPlatformModifiers MCMacPlatformMapNSModifiersToModifiers(NSUInteger p_modifiers);

NSEvent *MCMacPlatformGetLastMouseEvent(void);

NSMenu *MCMacPlatformGetIconMenu(void);

void MCMacPlatformLockMenuSelect(void);
void MCMacPlatformUnlockMenuSelect(void);
// SN-2014-11-06: [[ Bug 13836 ]] Returns whether the last item selected was a shadowed item
bool MCMacPlatformWasShadowItemSelected(void);

bool MCMacPlatformMapMenuItemActionToSelector(MCPlatformMenuItemAction action, SEL& r_selector);

void MCMacPlatformResetCursor(void);

void MCMacPlatformGetGlobalVolume(double& r_volume);
void MCMacPlatformSetGlobalVolume(double volume);

// MW-2014-04-23: [[ CocoaBackdrop ]] Ensures the windows are stacked correctly.
void MCMacPlatformSyncBackdrop(void);

// MW-2014-06-11: [[ Bug 12436 ]] These are used to ensure that the cursor doesn't get clobbered whilst in an user area snapshot.
void MCMacPlatformLockCursor(void);
void MCMacPlatformUnlockCursor(void);

////////////////////////////////////////////////////////////////////////////////

NSDragOperation MCMacPlatformMapDragOperationToNSDragOperation(MCPlatformDragOperation);
MCPlatformDragOperation MCMacPlatformMapNSDragOperationToDragOperation(NSDragOperation);

void MCMacPlatformPasteboardCreate(NSPasteboard *pasteboard, MCPlatformPasteboardRef& r_pasteboard);

////////////////////////////////////////////////////////////////////////////////

bool MCPlatformInitializeMenu(void);
void MCPlatformFinalizeMenu(void);

bool MCPlatformInitializeAbortKey(void);
void MCPlatformFinalizeAbortKey(void);

bool MCPlatformInitializeColorTransform(void);
void MCPlatformFinalizeColorTransform(void);

////////////////////////////////////////////////////////////////////////////////

// IM-2014-09-29: [[ Bug 13451 ]] Return the standard colorspace for images on OSX
bool MCMacPlatformGetImageColorSpace(CGColorSpaceRef &r_colorspace);

////////////////////////////////////////////////////////////////////////////////

// IM-2014-10-03: [[ Bug 13432 ]] Store both alpha data and derived cg image in the mask.
struct MCMacPlatformWindowMask
{
	MCGRaster mask;
	CGImageRef cg_mask;
	
	uint32_t references;
};

// IM-2014-09-30: [[ Bug 13501 ]] Allow system event checking to be enabled/disabled
void MCMacPlatformEnableEventChecking(void);
void MCMacPlatformDisableEventChecking(void);
bool MCMacPlatformIsEventCheckingEnabled(void);

////////////////////////////////////////////////////////////////////////////////

#endif
