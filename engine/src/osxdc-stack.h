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

#ifndef __MC_OSXDC_STACK__
#define __MC_OSXDC_STACK__

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCStackWindowDelegate: NSObject<NSWindowDelegate>
{
	MCStack *m_stack;
}

- (id)init;
- (void)dealloc;

- (void)setStack: (MCStack *)stack;
- (MCStack *)stack;

- (NSSize)windowWillResize: (NSWindow *)window toSize: (NSSize)frameSize;
- (void)windowDidMove: (NSNotification *)notification;
- (void)windowDidMiniaturize: (NSNotification *)notification;
- (void)windowDidDeminiaturize: (NSNotification *)notification;
- (void)windowShouldClose: (id)sender;
- (void)windowDidBecomeKey: (NSNotification *)notification;
- (void)windowDidResignKey: (NSNotification *)notification;

@end

@interface com_runrev_livecode_MCStackView: NSView<NSTextInputClient>
{
	MCStack *m_stack;
	NSTrackingArea *m_tracking_area;
	
	bool m_use_input_method : 1;
}

- (id)initWithFrame:(NSRect)frameRect;
- (void)dealloc;

- (void)updateTrackingAreas;

- (BOOL)isFlipped;

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent;
- (BOOL)acceptsFirstResponder;

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

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange;
- (void)doCommandBySelector:(SEL)aSelector;
- (void)setMarkedText:(id)aString selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange;
- (void)unmarkText;
- (NSRange)selectedRange;
- (NSRange)markedRange;
- (BOOL)hasMarkedText;
- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange;
- (NSArray*)validAttributesForMarkedText;
- (NSRect)firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange;
- (NSUInteger)characterIndexForPoint:(NSPoint)aPoint;

//////////

- (void)setUseInputMethod: (BOOL)shouldUse;
- (BOOL)useInputMethod;

- (void)setStack: (MCStack *)stack;
- (MCStack *)stack;

- (void)handleMousePress: (NSEvent *)event pressed: (BOOL)pressed;
- (void)handleMouseFocus: (NSEvent *)event inside: (BOOL)inside;
- (void)handleMouseMove: (NSEvent *)event forceOutside: (BOOL)forceOutside;
- (void)handleFlagsChanged: (NSEvent *)event;
- (void)handleKeyPress: (NSEvent *)event pressed: (BOOL)pressed;

@end

@interface NSView (com_runrev_livecode_NSViewAdditions)

- (MCPoint)localToGlobal: (MCPoint)location;
- (MCPoint)globalToLocal: (MCPoint)location;
- (MCRectangle)localRectToGlobal: (MCRectangle)location;
- (MCRectangle)globalRectToLocal: (MCRectangle)location;
- (MCStack *)stack;

@end

MCPoint NSPointToMCPointGlobal(NSPoint p);
NSPoint NSPointFromMCPointGlobal(MCPoint p);
MCPoint NSPointToMCPointLocal(NSView *view, NSPoint p);
NSPoint NSPointFromMCPointLocal(NSView *view, MCPoint p);
NSRect NSRectFromMCRectangleLocal(NSView *view, MCRectangle r);
MCRectangle NSRectToMCRectangleLocal(NSView *view, NSRect r);
NSRect NSRectFromMCRectangleGlobal(MCRectangle r);
MCRectangle NSRectToMCRectangleGlobal(NSRect r);

////////////////////////////////////////////////////////////////////////////////

#endif
