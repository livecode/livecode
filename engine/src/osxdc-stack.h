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

#ifndef __MC_OSXDC_STACK__
#define __MC_OSXDC_STACK__

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCStackView: NSView
{
	MCStack *m_stack;
	NSTrackingArea *m_tracking_area;
}

- (id)initWithFrame:(NSRect)frameRect;
- (void)dealloc;

- (void)updateTrackingAreas;

- (BOOL)isFlipped;
- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent;

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
- (MCStack *)stack;

@end

MCPoint NSPointToMCPointGlobal(NSPoint p);
NSPoint NSPointFromMCPointGlobal(MCPoint p);
MCPoint NSPointToMCPointLocal(NSView *view, NSPoint p);
NSPoint NSPointFromMCPointLocal(NSView *view, MCPoint p);
NSRect NSRectFromMCRectangleLocal(MCRectangle r);
MCRectangle NSRectToMCRectangleLocal(NSRect r);
NSRect NSRectFromMCRectangleGlobal(MCRectangle r);
MCRectangle NSRectToMCRectangleGlobal(NSRect r);

////////////////////////////////////////////////////////////////////////////////

#endif
