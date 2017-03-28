/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

#ifndef __MC_MAC_PLATFORM__
#define __MC_MAC_PLATFORM__

#import <AppKit/AppKit.h>
#include <Carbon/Carbon.h>
#include "visualeffect.h"

#include "typedefs.h"
#include "platform.h"
#include "visual.h"

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformWindow;
class MCMacPlatformSurface;
class MCMacPlatformMenu;
class MCMacPlatformCore;

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
    
    MCMacPlatformCore * m_platform;
}

@property (nonatomic, assign) MCMacPlatformCore * platform;

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

@property (readonly) MCPlatformDialogResult result;
@property (readonly) MCColor color;



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
{
    MCMacPlatformCore * m_platform;
}

- (id)initWithPlatform:(MCMacPlatformCore *) platform;
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
    void RenderCGImage(CGContextRef p_target, CGRect p_dst_rect, CGImageRef p_src, MCGFloat p_alpha, MCGBlendMode p_blend);
    void RenderImageToCG(CGContextRef p_target, CGRect p_dst_rect, MCGImageRef &p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend);
    void RenderRasterToCG(CGContextRef p_target, CGRect p_dst_rect, const MCGRaster &p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend);
    
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
    void WindowMoved(NSWindow *p_window);
    
    void SwitchFocusToView(uint32_t p_id);
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

MCPlatformModifiers MCMacPlatformMapNSModifiersToModifiers(NSUInteger p_modifiers);

////////////////////////////////////////////////////////////////////////////////

NSDragOperation MCMacPlatformMapDragOperationToNSDragOperation(MCPlatformDragOperation);
MCPlatformDragOperation MCMacPlatformMapNSDragOperationToDragOperation(NSDragOperation);

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
    CGImageRef m_cg_mask = nil;
    
    friend MCMacPlatformSurface;
};

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformLoadedFont: public MCPlatformLoadedFont
{
public:
    MCMacPlatformLoadedFont(void) = default;
    virtual ~MCMacPlatformLoadedFont(void);
    virtual bool CreateWithPath(MCStringRef p_path, bool p_globally);
private:
    MCAutoStringRef m_path;
    bool m_globally = false;
};

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformCursor: public MCPlatformCursor
{
public:
    MCMacPlatformCursor(void) = default;
    virtual ~MCMacPlatformCursor(void);
    virtual void CreateStandard(MCPlatformStandardCursor p_standard_cursor);
    virtual void CreateCustom(MCImageBitmap *p_image, MCPoint p_hotspot);
    virtual void Set(void);
private:
    bool is_standard = false;
    MCPlatformStandardCursor standard = kMCPlatformStandardCursorNone;
    NSCursor *custom = nil;
};


////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformColorTransform: public MCPlatformColorTransform
{
public:
    MCMacPlatformColorTransform(void) = default;
    virtual ~MCMacPlatformColorTransform(void);
    virtual bool Apply(MCImageBitmap *p_image);
    virtual bool CreateWithColorSpace(const MCColorSpaceInfo& p_info);
private:
    CGColorSpaceRef m_colorspace = nil;
};


////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformPrintDialogSession: public MCPlatformPrintDialogSession
{
public:
    constexpr MCMacPlatformPrintDialogSession(void) = default;
    virtual ~MCMacPlatformPrintDialogSession(void);
    virtual void BeginPageSetup(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format);
    virtual void BeginSettings(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format);
    virtual void CopyInfo(void *&r_session, void *&r_settings, void *&r_page_format);
    virtual void SetResult(MCPlatformPrintDialogResult p_result);
    virtual MCPlatformPrintDialogResult GetResult(void);
private:
    virtual void Begin(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format, id p_panel);
    
    MCPlatformPrintDialogResult m_result = kMCPlatformPrintDialogResultContinue;
    MCPlatformWindowRef m_owner = nil;
    
    NSPrintInfo *m_info = nil;
    PMPrintSession m_session = nil;
    PMPrintSettings m_settings = nil;
    PMPageFormat m_page_format = nil;
};

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCOpenSaveDialogDelegate: NSObject

- (void)panelDidEnd:(id)printDialog returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo;

@end

@compatibility_alias MCOpenSaveDialogDelegate com_runrev_livecode_MCOpenSaveDialogDelegate;

////////////////////////////////////////////////////////////////////////////////

struct MCMacPlatformDialogNest
{
    MCMacPlatformDialogNest *next;
    MCPlatformDialogResult result;
    MCPlatformWindowRef owner;
    NSSavePanel *panel;
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

struct MCFileFilter
{
    MCFileFilter *next;
    MCStringRef tag;
    MCStringRef *extensions;
    uint32_t extension_count;
    MCStringRef *filetypes;
    uint32_t filetypes_count;
};

////////////////////////////////////////////////////////////////////////////////

typedef struct __coreimage_visualeffect_t coreimage_visualeffect_t;
typedef coreimage_visualeffect_t *coreimage_visualeffect_ref_t;
struct __coreimage_visualeffect_t
{
    coreimage_visualeffect_t *next;
    NSString *name;
    rei_visualeffect_info_ref_t info;
};

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformCore: public MCPlatform::Core
{
public:
    MCMacPlatformCore(void);
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
    virtual MCPlatformColorTransformRef CreateColorTransform(void);
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
    virtual uint32_t GetMenuSelectLock(void) { return m_menu_select_lock;}
    virtual void SetMenuSelectLock(uint32_t p_lock) { m_menu_select_lock = p_lock;}
    
    virtual NSMenu *GetIconMenu(void);
    virtual bool MapMenuItemActionToSelector(MCPlatformMenuItemAction action, SEL& r_selector);
    virtual void SetQuitSelected(bool p_selected) {m_quit_selected = p_selected;}
    
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
    virtual bool GetResponderChangeLock(void) {return m_lock_responder_change; }
    virtual void SetResponderChangeLock(bool p_lock_responder_change) { m_lock_responder_change = p_lock_responder_change; }
    
    // Window mask
    virtual MCPlatformWindowMaskRef CreateWindowMask(void);
    
    // Color dialog
    virtual void BeginColorDialog(MCStringRef p_title, const MCColor& p_color);
    virtual MCPlatformDialogResult EndColorDialog(MCColor& r_color);
    
    // File & folder dialog
    virtual void BeginFileDialog(MCPlatformFileDialogKind p_kind, MCPlatformWindowRef p_owner, MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial_folder, MCStringRef p_initial_file);
    virtual MCPlatformDialogResult EndFileDialog(MCPlatformFileDialogKind p_kind, MCStringRef &r_paths, MCStringRef &r_type);
    virtual void BeginFolderDialog(MCPlatformWindowRef p_owner, MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial);
    virtual MCPlatformDialogResult EndFolderDialog(MCStringRef& r_selected_folder);
    virtual void BeginOpenSaveDialog(MCPlatformWindowRef p_owner, NSSavePanel *p_panel, MCStringRef p_folder, MCStringRef p_file);
    virtual MCPlatformDialogResult EndOpenSaveDialog(void);
    virtual bool FileFilterCreate(MCStringRef p_desc, MCFileFilter*& r_filter);
    
    // Print dialog
    virtual MCPlatformPrintDialogSessionRef CreatePrintDialogSession(void);
    
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
    virtual bool GetInsideFocusEvent(void) {return m_inside_focus_event; }
    virtual void SetInsideFocusEvent(bool p_inside_focus_event) { m_inside_focus_event = p_inside_focus_event; }
    
    // Mice
    virtual MCPlatformCursorRef CreateCursor(void);
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
    virtual bool GetCursorIsHidden(void) { return m_cursor_is_hidden; }
    virtual void SetCursorIsHidden(bool p_hidden) { m_cursor_is_hidden = p_hidden; }
    
    // Modifier Keys
    virtual MCPlatformModifiers GetModifiersState(void);
    virtual bool GetKeyState(MCPlatformKeyCode*& r_codes, uindex_t& r_code_count);
    
    // Drag and drop
    virtual void DoDragDrop(MCPlatformWindowRef p_window, MCPlatformAllowedDragOperations p_allowed_operations, MCImageBitmap *p_image, const MCPoint *p_image_loc, MCPlatformDragOperation& r_operation);
    
    // Point translation
    virtual void SetHasDesktopHeight(bool p_has_desktop_height);
    virtual CGFloat GetDesktopHeight();
    virtual void MapScreenMCPointToNSPoint(MCPoint p, NSPoint& r_point);
    virtual void MapScreenNSPointToMCPoint(NSPoint p, MCPoint& r_point);
    virtual void MapScreenMCRectangleToNSRect(MCRectangle r, NSRect& r_rect);
    virtual void MapScreenNSRectToMCRectangle(NSRect r, MCRectangle& r_rect);
    
    // Screens
    virtual void GetScreenCount(uindex_t& r_count);
    virtual void GetScreenViewport(uindex_t p_index, MCRectangle& r_viewport);
    virtual void GetScreenWorkarea(uindex_t p_index, MCRectangle& r_workarea);
    virtual void GetScreenPixelScale(uindex_t p_index, MCGFloat& r_scale);
    virtual void DisableScreenUpdates(void);
    virtual void EnableScreenUpdates(void);
    
    // Backdrop
    virtual void SyncBackdrop(void);
    virtual void ConfigureBackdrop(MCPlatformWindowRef p_backdrop_window);
    
    // Scripting
    virtual MCPlatformScriptEnvironmentRef CreateScriptEnvironment(void);
    
    // Fonts
    virtual MCPlatformLoadedFontRef CreateLoadedFont(void);
    
    // Sound
    virtual MCPlatformSoundRef CreateSound(void);
    
    // Native Layer
    virtual MCPlatformNativeLayerRef CreateNativeLayer(void);
    virtual bool CreateNativeContainer(void *&r_view);
    virtual void ReleaseNativeView(void *p_view);
    
    // Theme
    virtual bool GetControlThemePropBool(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, bool& r_bool);
    virtual bool GetControlThemePropInteger(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, int& r_int);
    virtual bool GetControlThemePropColor(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCColor& r_color);
    virtual bool GetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCFontRef& r_font);
    virtual bool GetControlThemePropString(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCStringRef& r_string);
    virtual bool DrawTheme(MCGContextRef p_context, MCThemeDrawType p_type, MCThemeDrawInfo *p_info_ptr);
    virtual bool LoadTheme(void);
    virtual uint16_t GetThemeId(void);
    virtual uint16_t GetThemeFamilyId(void);
    virtual bool IsThemeWidgetSupported(Widget_Type wtype);
    virtual int32_t GetThemeMetric(Widget_Metric wmetric);
    virtual int32_t GetThemeWidgetMetric(const MCWidgetInfo &winfo,Widget_Metric wmetric);
    virtual void GetThemeWidgetRect(const MCWidgetInfo &winfo, Widget_Metric wmetric, const MCRectangle &srect, MCRectangle &drect);
    virtual bool GetThemePropBool(Widget_ThemeProps themeprop);
    virtual bool DrawThemeWidget(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &d);
    virtual Widget_Part HitTestTheme(const MCWidgetInfo &winfo, int2 mx, int2 my, const MCRectangle &drect);
    virtual void UnloadTheme(void);
    virtual bool DrawThemeFocusBorder(MCContext *p_context, const MCRectangle& p_dirty, const MCRectangle& p_rect);
    virtual bool DrawThemeMetalBackground(MCContext *p_context, const MCRectangle& p_dirty, const MCRectangle& p_rect, MCPlatformWindowRef p_window);
    virtual Widget_Part HitTestScrollControls(const MCWidgetInfo &winfo, int2 mx,int2 my, const MCRectangle &drect);
    virtual void DrawMacAMScrollControls(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect, CFAbsoluteTime p_start_time, CFAbsoluteTime p_current_time);
    virtual void fillTrackDrawInfo(const MCWidgetInfo &winfo, HIThemeTrackDrawInfo &drawInfo, const MCRectangle &drect);
    virtual bool CreateCGContextForBitmap(MCImageBitmap *p_bitmap, CGContextRef &r_context);
    
    // Callbacks
    virtual MCPlatformCallbackRef GetCallback(void) { return m_callback;}
    virtual void SetCallback(MCPlatformCallbackRef p_callback) {m_callback = p_callback;}

    // Platform extensions
    virtual bool QueryInterface(const char * p_interface_id, MCPlatform::Base *&r_interface);
    
#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER)
    // Apple platforms only
    virtual void RunBlockOnMainFiber(void (^block)(void));
#endif
    
    // Core image
    virtual rei_boolean_t CoreImageVisualEffectInitialize(void);
    virtual void CoreImageVisualEffectFinalize(void);
    virtual rei_boolean_t CoreImageVisualEffectLookup(const char *p_name, rei_visualeffect_info_ref_t *r_info);
    virtual rei_boolean_t CoreImageVisualEffectBegin(rei_handle_t p_handle, MCGImageRef p_image_a, MCGImageRef p_image_b, rei_rectangle_ref_t p_area, float p_surface_height, rei_visualeffect_parameter_list_ref_t p_parameters);
    virtual rei_boolean_t CoreImageVisualEffectStep(MCStackSurface *p_target, float p_time);
    virtual rei_boolean_t CoreImageVisualEffectEnd(void);
    virtual bool MCGImageToCIImage(MCGImageRef p_image, CIImage *&r_image);
    
    // Core graphics
    virtual bool MCImageBitmapToCGImage(MCImageBitmap *p_bitmap, bool p_copy, bool p_invert, CGImageRef &r_image);
    virtual bool MCImageBitmapToCGImage(MCImageBitmap *p_bitmap, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image);
    virtual void CGImageToMCImageBitmap(CGImageRef p_image, MCPoint p_size, MCImageBitmap*& r_bitmap);
    
    // Apple events
    virtual OSErr SpecialAppleEvent(const AppleEvent *ae, AppleEvent *reply);
    virtual OSErr OpenDocAppleEvent(const AppleEvent *theAppleEvent, AppleEvent *reply);
    virtual OSErr AnswerAppleEvent(const AppleEvent *ae, AppleEvent *reply);
    virtual void Send(MCStringRef p_message, MCStringRef p_program, MCStringRef p_eventtype, Boolean p_reply, MCStringRef &r_result);
    virtual void Reply(MCStringRef p_message, MCStringRef p_keyword, Boolean p_error);
    virtual void RequestAE(MCStringRef p_message, uint16_t p_ae, MCStringRef& r_value);
    virtual bool RequestProgram(MCStringRef p_message, MCStringRef p_program, MCStringRef& r_value, MCStringRef& r_result);
protected:
    // Sound
    void GetGlobalVolume(double& r_volume);
    void SetGlobalVolume(double p_volume);
    
    uindex_t m_event_checking_enabled = 0;
    
    // Abort key
    MCAbortKeyThread *m_abort_key_thread = nil;
    
    // Windows
    MCPlatformWindowRef m_moving_window = nil;
    NSWindow *m_pseudo_modal_for = nil;
    MCModalSession *m_modal_sessions = nil;
    uindex_t m_modal_session_count = 0;
    bool m_lock_responder_change = false;
    bool m_inside_focus_event = false;

    // Wait
    bool m_in_blocking_wait = false;
    CFRunLoopObserverRef m_observer = nil;
    bool m_wait_broken = false;

    // Callbacks
    NSLock *m_callback_lock = nil;
    MCCallback *m_callbacks = nil;
    uindex_t m_callback_count = 0;
    
    // Snapshots
    void ScreenSnapshotOfWindowWithinBounds(uint32_t p_window_id, MCRectangle p_bounds, MCPoint *p_size, MCImageBitmap *&r_bitmap);
    void WaitForDisplayRefresh(void);
    CGPoint m_snapshot_start_point = CGPointZero;
    CGPoint m_snapshot_end_point = CGPointZero;
    bool m_snapshot_done = false;
    bool m_display_link_fired = false;

    // Mice
    NSEvent *m_last_mouse_event = nil;
    
    // If this is true, then the mouse is currently grabbed so we should defer
    // switching active window until ungrabbed.
    bool m_mouse_grabbed = false;
    
    // If this is true there was an explicit request for grabbing.
    bool m_mouse_grabbed_explicit = false;
    
    // This is the currently active window (the one receiving mouse events).
    MCPlatformWindowRef m_mouse_window = nil;
    
    // This is the current mask of buttons that are pressed.
    uint32_t m_mouse_buttons = 0;
    
    // This is the button that is being dragged (if not 0xffffffff).
    uint32_t m_mouse_drag_button = 0xffffffff;
    
    // This is the number of successive clicks detected on the primary button.
    uint32_t m_mouse_click_count = 0;
    
    // This is the button of the last click (mouseDown then mouseUp) that was
    // detected.
    uint32_t m_mouse_last_click_button = 0;
    
    // This is the time of the last mouseUp, used to detect multiple clicks.
    uint32_t m_mouse_last_click_time = 0;
    
    // This is the screen position of the last click, used to detect multiple
    // clicks.
    MCPoint m_mouse_last_click_screen_position = { 0, 0 };
    
    // This is the window location in the mouse window that we last posted
    // a position event for.
    MCPoint m_mouse_position = { INT16_MIN, INT16_MAX };
    
    // This is the last screen location we received a mouse move message for.
    MCPoint m_mouse_screen_position = { 0, 0 };
    
    // This is the current modifier state, and whether the control key was down
    // for a button 0 press.
    MCPlatformModifiers m_mouse_modifiers = 0;
    bool m_mouse_was_control_click = false;
    
    // MW-2014-06-11: [[ Bug 12436 ]] This is used to temporarily turn off cursor setting
    //   when doing an user-import snapshot.
    bool m_mouse_cursor_locked = false;
    
    bool m_have_desktop_height = false;
    CGFloat m_desktop_height = 0.0f;
    
    bool m_cursor_is_hidden = false;
    
    // Backdrop
    MCPlatformWindowRef m_backdrop_window = nil;
    
    MCMacPlatformDialogNest *m_dialog_nesting = nil;
    MCOpenSaveDialogDelegate *m_dialog_delegate = nil;
    
    // Color dialog
    MCColorPanelDelegate* m_color_dialog_delegate = nil;
    
    // Menus
    uint32_t m_menu_select_lock = 0;
    // SN-2014-11-06: [[ Bug 13836 ]] Stores whether a quit item got selected
    bool m_quit_selected = false;
    // The menuref currently set as the menubar.
    MCMacPlatformMenu * m_menubar = nil;
    uint32_t m_quitting_state_count = 0;
    uint8_t *m_quitting_states = nil;
    MCMacPlatformMenu *m_icon_menu = nil;
    
    CFAbsoluteTime m_animation_start_time = 0;
    CFAbsoluteTime m_animation_current_time = 0;
    
    // Core image
    NSArray *m_g_effects = nil;
    coreimage_visualeffect_ref_t m_g_effect_infos = nil;
    CIFilter *m_g_current_filter = nil;
    rei_rectangle_t m_g_current_area;
    // IM-2013-08-29: [[ RefactorGraphics ]] Record surface height so we can transform image location to flipped context
    float m_g_current_height = 0.0f;
    
    // Engine callbacks
    MCPlatformCallbackRef m_callback = nil;
    
    // Apple Events
    AEKeyword m_replykeyword = 0;   // Use in DoSpecial & other routines
    MCStringRef m_AEReplyMessage = nil;
    MCStringRef m_AEAnswerData = nil;
    MCStringRef m_AEAnswerErr = nil;
    const AppleEvent *m_aePtr = nil; //current apple event for mcs_request_ae()
};

////////////////////////////////////////////////////////////////////////////////

class MCMacPlatformSound: public MCPlatformSound
{
public:
    constexpr MCMacPlatformSound(void) = default;
    virtual ~MCMacPlatformSound(void);
    
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
    NSSound *m_sound = nil;
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
    
    virtual uint32_t GetOpenMenuItems(void) { return m_open_menu_items ;}
    virtual void SetOpenMenuItems(uint32_t p_open_menu_items) {m_open_menu_items = p_open_menu_items;}
    
    virtual void SetMenuItemSelected(bool p_selected) {m_menu_item_selected = p_selected;}
 private:
    void DestroyMenuItem(uindex_t p_index);
    void MapMenuItemIndex(uindex_t& x_index);
    void ClampMenuItemIndex(uindex_t& x_index);
    
    NSMenu *menu = nil;
    MCMenuDelegate *menu_delegate = nil;
    
    // If the menu is being used as a menubar then this is true. When this
    // is the case, some items will be hidden and a (API-wise) invisible
    // menu will be inserted at the front (the application menu).
    bool is_menubar = false;
    
    // If the quit item in this menu has an accelerator, this is true.
    // (Cocoa seems to 'hide' the quit menu item accelerator for some inexplicable
    // reason - it returns 'empty').
    NSMenuItem* quit_item = nil;
    
    // SN-2014-11-06: [[ Bu 13940 ]] Add a flag for the presence of a Preferences shortcut
    //  to allow the menu item to be disabled.
    NSMenuItem* preferences_item = nil;
    
    NSMenuItem* about_item = nil;
    
    // SN-2014-11-10: [[ Bug 13836 ]] Keeps the track about the open items in the menu bar.
    uint32_t m_open_menu_items = 0;
    
    bool m_menu_item_selected = false;
    
    // The delegate for the app menu.
    MCAppMenuDelegate *m_app_menu_delegate = nil;
};

////////////////////////////////////////////////////////////////////////////////

typedef const struct OpaqueJSContext* JSContextRef;
typedef struct OpaqueJSContext* JSGlobalContextRef;
typedef struct OpaqueJSString* JSStringRef;
typedef struct OpaqueJSClass* JSClassRef;
typedef struct OpaqueJSPropertyNameArray* JSPropertyNameArrayRef;
typedef struct OpaqueJSPropertyNameAccumulator* JSPropertyNameAccumulatorRef;
typedef const struct OpaqueJSValue* JSValueRef;
typedef struct OpaqueJSValue* JSObjectRef;

typedef enum {
    kJSTypeUndefined,
    kJSTypeNull,
    kJSTypeBoolean,
    kJSTypeNumber,
    kJSTypeString,
    kJSTypeObject
} JSType;

enum {
    kJSPropertyAttributeNone         = 0,
    kJSPropertyAttributeReadOnly     = 1 << 1,
    kJSPropertyAttributeDontEnum     = 1 << 2,
    kJSPropertyAttributeDontDelete   = 1 << 3
};
typedef unsigned JSPropertyAttributes;

enum {
    kJSClassAttributeNone = 0,
    kJSClassAttributeNoAutomaticPrototype = 1 << 1
};
typedef unsigned JSClassAttributes;

typedef void
(*JSObjectInitializeCallback) (JSContextRef ctx, JSObjectRef object);
typedef void
(*JSObjectFinalizeCallback) (JSObjectRef object);
typedef bool
(*JSObjectHasPropertyCallback) (JSContextRef ctx, JSObjectRef object, JSStringRef propertyName);
typedef JSValueRef
(*JSObjectGetPropertyCallback) (JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception);
typedef bool
(*JSObjectSetPropertyCallback) (JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception);
typedef bool
(*JSObjectDeletePropertyCallback) (JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception);
typedef void
(*JSObjectGetPropertyNamesCallback) (JSContextRef ctx, JSObjectRef object, JSPropertyNameAccumulatorRef propertyNames);
typedef JSValueRef
(*JSObjectCallAsFunctionCallback) (JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
typedef JSObjectRef
(*JSObjectCallAsConstructorCallback) (JSContextRef ctx, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
typedef bool
(*JSObjectHasInstanceCallback)  (JSContextRef ctx, JSObjectRef constructor, JSValueRef possibleInstance, JSValueRef* exception);
typedef JSValueRef
(*JSObjectConvertToTypeCallback) (JSContextRef ctx, JSObjectRef object, JSType type, JSValueRef* exception);
typedef struct {
    const char* const name;
    JSObjectCallAsFunctionCallback callAsFunction;
    JSPropertyAttributes attributes;
} JSStaticFunction;

typedef struct {
    const char* const name;
    JSObjectGetPropertyCallback getProperty;
    JSObjectSetPropertyCallback setProperty;
    JSPropertyAttributes attributes;
} JSStaticValue;

typedef struct {
    int                                 version; // current (and only) version is 0
    JSClassAttributes                   attributes;
    
    const char*                         className;
    JSClassRef                          parentClass;
    
    const JSStaticValue*                staticValues;
    const JSStaticFunction*             staticFunctions;
    
    JSObjectInitializeCallback          initialize;
    JSObjectFinalizeCallback            finalize;
    JSObjectHasPropertyCallback         hasProperty;
    JSObjectGetPropertyCallback         getProperty;
    JSObjectSetPropertyCallback         setProperty;
    JSObjectDeletePropertyCallback      deleteProperty;
    JSObjectGetPropertyNamesCallback    getPropertyNames;
    JSObjectCallAsFunctionCallback      callAsFunction;
    JSObjectCallAsConstructorCallback   callAsConstructor;
    JSObjectHasInstanceCallback         hasInstance;
    JSObjectConvertToTypeCallback       convertToType;
} JSClassDefinition;

typedef JSGlobalContextRef (*JSGlobalContextCreatePtr)(JSClassRef globalObjectClass);
typedef void (*JSGlobalContextReleasePtr)(JSGlobalContextRef ctx);
typedef JSObjectRef (*JSContextGetGlobalObjectPtr)(JSContextRef ctx);
typedef JSValueRef (*JSEvaluateScriptPtr)(JSContextRef ctx, JSStringRef script, JSObjectRef thisObject, JSStringRef sourceURL, int startingLineNumber, JSValueRef* exception);
typedef bool (*JSCheckScriptSyntaxPtr)(JSContextRef ctx, JSStringRef script, JSStringRef sourceURL, int startingLineNumber, JSValueRef* exception);
typedef JSStringRef (*JSStringCreateWithCFStringPtr)(CFStringRef string);
typedef CFStringRef (*JSStringCopyCFStringPtr)(CFAllocatorRef alloc, JSStringRef string);
typedef JSValueRef (*JSValueMakeStringPtr)(JSContextRef ctx, JSStringRef string);
typedef JSStringRef (*JSValueToStringCopyPtr)(JSContextRef ctx, JSValueRef value, JSValueRef* exception);
typedef JSObjectRef (*JSValueToObjectPtr)(JSContextRef ctx, JSValueRef value, JSValueRef* exception);
typedef void (*JSValueProtectPtr)(JSContextRef ctx, JSValueRef value);
typedef void (*JSValueUnprotectPtr)(JSContextRef ctx, JSValueRef value);
typedef void (*JSStringReleasePtr)(JSStringRef string);
typedef JSClassRef (*JSClassCreatePtr)(const JSClassDefinition* definition);
typedef void (*JSClassReleasePtr)(JSClassRef jsClass);
typedef JSObjectRef (*JSObjectMakePtr)(JSContextRef ctx, JSClassRef jsClass, void* data);
typedef void* (*JSObjectGetPrivatePtr)(JSObjectRef object);
typedef JSValueRef (*JSObjectGetPropertyPtr)(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception);
typedef void (*JSObjectSetPropertyPtr)(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSPropertyAttributes attributes, JSValueRef* exception);
typedef JSValueRef (*JSObjectCallAsFunctionPtr)(JSContextRef ctx, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);

static const char kJavaScriptCoreLibraryPath[] = "/System/Library/Frameworks/JavaScriptCore.framework/Versions/A/JavaScriptCore";

static const void *JavaScriptCoreLibrary = NULL;

static JSGlobalContextCreatePtr JSGlobalContextCreate;
static JSGlobalContextReleasePtr JSGlobalContextRelease;
static JSContextGetGlobalObjectPtr JSContextGetGlobalObject;
static JSEvaluateScriptPtr JSEvaluateScript;
static JSCheckScriptSyntaxPtr JSCheckScriptSyntax;
static JSStringCreateWithCFStringPtr JSStringCreateWithCFString;
static JSStringCopyCFStringPtr JSStringCopyCFString;
static JSValueMakeStringPtr JSValueMakeString;
static JSValueToStringCopyPtr JSValueToStringCopy;
static JSValueToObjectPtr JSValueToObject;
static JSValueProtectPtr JSValueProtect;
static JSValueUnprotectPtr JSValueUnprotect;
static JSStringReleasePtr JSStringRelease;
static JSClassCreatePtr JSClassCreate;
static JSClassReleasePtr JSClassRelease;
static JSObjectMakePtr JSObjectMake;
static JSObjectGetPrivatePtr JSObjectGetPrivate;
static JSObjectGetPropertyPtr JSObjectGetProperty;
static JSObjectSetPropertyPtr JSObjectSetProperty;
static JSObjectCallAsFunctionPtr JSObjectCallAsFunction;

///////////////////////////////////////////////////////////////////////////////

#define GET_JSC_SYMBOL(sym) \
sym = (sym##Ptr)NSAddressOfSymbol(NSLookupSymbolInImage((const mach_header *)JavaScriptCoreLibrary, "_"#sym, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND));

class MCMacPlatformScriptEnvironment: public MCPlatformScriptEnvironment
{
public:
    constexpr MCMacPlatformScriptEnvironment(void) = default;
    virtual ~MCMacPlatformScriptEnvironment(void);
    
    virtual bool Define(const char *p_function, MCPlatformScriptEnvironmentCallback p_callback);
    
    virtual void Run(MCStringRef p_script, MCStringRef &r_result);
    
    virtual char *Call(const char *p_method, const char **p_arguments, unsigned int p_argument_count);
    
private:
    struct Function
    {
        char *name;
        MCPlatformScriptEnvironmentCallback callback;
    };
    
    JSGlobalContextRef m_runtime = nil;
    
    Function *m_functions = nil;
    uint4 m_function_count = 0;
};

///////////////////////////////////////////////////////////////////////////////

// IM-2015-12-16: [[ NativeLayer ]] Keep the coordinate system of group contents the same as
//                the top-level window view by keeping its bounds the same as its frame.
//                This allows us to place contents in terms of window coords without having to
//                adjust for the location of the group container.
@interface com_runrev_livecode_MCContainerView: NSView

- (void)setFrameOrigin:(NSPoint)newOrigin;
- (void)setFrameSize:(NSSize)newSize;

@end

@compatibility_alias MCContainerView com_runrev_livecode_MCContainerView;

///////////////////////////////////////////////////////////////////////////////

class MCMacPlatformNativeLayer : public MCPlatformNativeLayer
{
public:
    constexpr MCMacPlatformNativeLayer(void) = default;
    ~MCMacPlatformNativeLayer(void);
    
    virtual bool GetNativeView(void *&r_view);
    virtual void SetNativeView(void *p_view);
    
    // Performs the attach/detach operations
    virtual void Attach(MCPlatformWindowRef p_window, void *p_container_view, void *p_view_above, bool p_visible);
    virtual void Detach();
    
    virtual bool Paint(MCGContextRef p_context);
    virtual void SetGeometry(const MCRectangle &p_rect);
    virtual void SetViewportGeometry(const MCRectangle &p_rect);
    virtual void SetVisible(bool p_visible);
    
    // Performs a relayering operation
    virtual void Relayer(void *p_container_view, void *p_view_above);
    
private:
    
    NSView* m_view = nil;
    NSBitmapImageRep *m_cached = nil;
    MCPlatformWindowRef m_window = nil;
    NSRect calculateFrameRect(const MCRectangle &p_rect);
    
};

///////////////////////////////////////////////////////////////////////////////

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
