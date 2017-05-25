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

#include "prefix.h"

#include "redraw.h"

#include "mbliphoneview.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#undef MCLog
#define MCLog(...) NSLog(@__VA_ARGS__)
#endif

////////////////////////////////////////////////////////////////////////////////

@implementation MCIPhoneRootView

- (id)init
{
	MCLog("MainView: init\n");
	
	self = [super init];
	if (self == nil)
		return nil;
	
	// MW-2012-03-05: [[ ViewStack ]] Initialize the view's stack to nil.
	m_stack = nil;
	
	m_in_deactivate_keyboard = false;
	m_animate_autorotation = true;
	
	m_text_view = [[UITextView alloc] initWithFrame: CGRectZero];
	[m_text_view setHidden: YES];
	[m_text_view setEditable: YES];
	[m_text_view setDelegate: self];
	[m_text_view setAutocorrectionType: UITextAutocorrectionTypeNo];
	[m_text_view setAutocapitalizationType: UITextAutocapitalizationTypeNone];
	
	m_main_view = [[MCIPhoneMainView alloc] initWithFrame: CGRectZero];
	
	m_display_view = [[MCIPhoneUIKitDisplayView alloc] initWithFrame: CGRectZero];

	[self addSubview: m_display_view];
	[self addSubview: m_text_view];
	[self addSubview: m_main_view];
	
	return self;
}

- (void)dealloc
{
	[m_display_view release];
	[m_main_view release];
	
	[super dealloc];
}

//////////

// MW-2012-03-05: [[ ViewStack ]] Change the current stack of the view.
- (void)setCurrentStack:(MCStack *)p_stack
{
	m_stack = p_stack;
}

// MW-2012-03-05: [[ ViewStack ]] Fetch the current stack of the view.
- (MCStack *)currentStack
{
	return m_stack;
}

//////////

- (UITextView *)textView
{
	return m_text_view;
}

- (MCIPhoneMainView *)mainView
{
	return m_main_view;
}

- (MCIPhoneDisplayView *)displayView
{
	return m_display_view;
}

//////////

- (void)reshape
{
	CGRect t_view_bounds;
	t_view_bounds = MCIPhoneGetViewBounds();
	
	MCLog("RootView: reshape (%f, %f, %f, %f)\n", t_view_bounds . origin . x, t_view_bounds . origin . y, t_view_bounds . size . width, t_view_bounds . size . height);
	
	if (MCIPhoneIsEmbedded())
	{
		[m_main_view setFrame: CGRectMake(0.0, 0.0, t_view_bounds . size . width, t_view_bounds . size . height)];
		[m_display_view setFrame: CGRectMake(0.0, 0.0, t_view_bounds . size . width, t_view_bounds . size . height)];
	}
	else
	{
		[m_main_view setFrame: t_view_bounds];
		[m_display_view setFrame: t_view_bounds];
	}

	MCIPhoneHandleViewBoundsChanged();
}

- (void)setFrame: (CGRect)p_new_frame
{
	MCLog("RootView: setFrame (%f, %f, %f, %f)\n", p_new_frame . origin . x, p_new_frame . origin . y, p_new_frame . size . width, p_new_frame . size . height);
	
	[self reshape];
	
	[super setFrame: p_new_frame];
}

- (void)switchToDisplayClass: (Class)p_new_class
{
	if ([m_display_view isMemberOfClass: p_new_class])
		return;
	
	MCIPhoneDisplayView *t_new_display;
	t_new_display = [[p_new_class alloc] initWithFrame: [m_display_view frame]];
	
	[m_display_view removeFromSuperview];
	[m_display_view release];
	
	m_display_view = t_new_display;

	[self insertSubview: m_display_view atIndex: 0];
}

//////////

- (void)setAnimateAutorotation: (bool)p_animate
{
	m_animate_autorotation = p_animate;
}

- (void)beginAutorotation
{
	if (m_animate_autorotation)
	{
		// If screen updates aren't disabled, disable them.
		m_screen_updates_were_enabled = MCRedrawIsScreenUpdateEnabled();
		if (m_screen_updates_were_enabled)
			MCRedrawDisableScreenUpdates();
		
		// Get the engine to post resizeStack and reshape the view.
		MCIPhoneHandleViewBoundsChanged();
	}
	else
	{
		// Disable UIView animations.
		[UIView setAnimationsEnabled: NO];
	}
}

- (void)animateAutorotation
{
	if (m_animate_autorotation)
	{
		// Update the bounds of the main and display views.
		CGRect t_view_bounds;
		t_view_bounds = MCIPhoneGetViewBounds();;
		[m_main_view setFrame: t_view_bounds];
		[m_display_view setFrame: t_view_bounds];
	}
}

- (void)endAutorotation
{
	if (m_animate_autorotation)
	{
		// If screen updates were enabled, re-enable them.
		if (m_screen_updates_were_enabled)
		{
			MCRedrawEnableScreenUpdates();
			
			// MW-2012-10-12: [[ Bug 10143 ]] Make sure we wake up the event loop
			//   to ensure a redraw happens sharpish!
			MCIPhoneBreakWait();
		}
	}
	else
	{
        // We can do this here as we are still on the engine fibre (as anims were
        // disabld). Tell the engine to reshape the stack.
        [self reshape];
        
		// Re-enable UIView animations.
		[UIView setAnimationsEnabled: YES];
	}
}

//////////

- (BOOL)activateKeyboard
{
	if (![m_text_view isFirstResponder])
		[m_text_view becomeFirstResponder];
	
	return YES;
}

- (BOOL)deactivateKeyboard
{
	if (![m_text_view isFirstResponder])
		return NO;
	
	m_in_deactivate_keyboard = true;
	[m_text_view resignFirstResponder];
	[m_main_view becomeFirstResponder];
	m_in_deactivate_keyboard = false;
	
	return YES;
}

- (void)textViewDidBeginEditing: (UITextView *)textView
{
	[m_text_view setText: @" "];
	[m_text_view setSelectedRange: NSMakeRange(1, 0)];
	
	MCIPhoneHandleBeginTextInput();
}

- (void)textViewDidEndEditing: (UITextView *)textView
{
	if (m_in_deactivate_keyboard)
		return;
	
	MCIPhoneHandleEndTextInput();
}

- (BOOL)textView: (UITextView *)textView shouldChangeTextInRange: (NSRange)range replacementText: (NSString*)text
{
    // Handle deletion
    if (range . location == 0 || [text length] == 0)
	{
        MCIPhoneHandleProcessTextInput(0, 0xff08);
        return NO;
	}
    
    // Handle newlines
    if ([text characterAtIndex:0] == 0x0a)
    {
        MCIPhoneHandleProcessTextInput(0, 0xff0d);
        return NO;
    }

    // Loop over the UTF-16 codeunits and insert them as codepoints
    bool t_is_surrogate = false;
    unichar_t t_surrogate = 0;
    for (NSUInteger i = 0; i < [text length]; i++)
    {
        // Get the next character
        unichar_t t_char = [text characterAtIndex:i];
        
        // Process based on whether we are dealing with a surrogate or not
        codepoint_t t_codepoint;
        if (t_is_surrogate)
        {
            // We're expecting a trailing surrogate
            if (!MCUnicodeCodepointIsTrailingSurrogate(t_char))
            {
                // Something has gone wrong... abort processing of this text
                return NO;
            }
            
            // Re-combine the surrogate
            t_codepoint = MCUnicodeCombineSurrogates(t_surrogate, t_char);
            t_is_surrogate = false;
            
        }
        else if (MCUnicodeCodepointIsLeadingSurrogate(t_char))
        {
            // Wait for the second half of the pair
            t_is_surrogate = true;
            t_surrogate = t_char;
            continue;
        }
        else
        {
            // Not a surrogate - use directly as a codepoint
            t_codepoint = t_char;
        }
        
        MCIPhoneHandleProcessTextInput(t_codepoint, 0);
    }
	
	return NO;
}

//////////

- (BOOL)com_runrev_livecode_passMotionEvents
{
	return NO;
}

@end

////////////////////////////////////////////////////////////////////////////////

@implementation MCIPhoneMainView

- (BOOL)isMultipleTouchEnabled
{
	return YES;
}

- (BOOL)canBecomeFirstResponder
{
	return YES;
}

- (BOOL)becomeFirstResponder
{
	return YES;
}

- (void)touchesBegan: (NSSet *)touches withEvent: (UIEvent *)event
{
	MCIPhoneHandleTouches(self, touches, UITouchPhaseBegan);
}

- (void)touchesMoved: (NSSet *)touches withEvent: (UIEvent *)event
{
	MCIPhoneHandleTouches(self, touches, UITouchPhaseMoved);
}

- (void)touchesEnded: (NSSet *)touches withEvent: (UIEvent *)event
{
	MCIPhoneHandleTouches(self, touches, UITouchPhaseEnded);
}

- (void)touchesCancelled: (NSSet *)touches withEvent: (UIEvent *)event
{
	MCIPhoneHandleTouches(self, touches, UITouchPhaseCancelled);
}

@end

////////////////////////////////////////////////////////////////////////////////

@implementation MCIPhoneDisplayView

- (id)initWithFrame: (CGRect)p_frame
{
	self = [super initWithFrame: p_frame];
	if (self == nil)
		return nil;
	
	[self setContentMode: UIViewContentModeRedraw];
	[self setAutoresizingMask: UIViewAutoresizingNone];
	
	return self;
}

// The view that displays content is currently always opaque.
- (BOOL)isOpaque
{
	return YES;
}

// Our drawing methods ensure we start off with a clear canvas, or just render
// opaque content.
- (BOOL)clearsContextBeforeDrawing
{
	return NO;
}

// The display view is transparent to touches, this means that it can be
// switched on the fly with no unpleasant touch dropping effects.
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
	return NO;
}

- (void)render: (MCStack *)p_stack region: (MCRegionRef)region
{
}

@end

////////////////////////////////////////////////////////////////////////////////
