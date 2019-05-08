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

#ifndef __MBL_IPHONE_VIEW__
#define __MBL_IPHONE_VIEW__

////////////////////////////////////////////////////////////////////////////////

#ifndef __MBL_IPHONE_APP__
#include "mbliphoneapp.h"
#endif

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

////////////////////////////////////////////////////////////////////////////////

static inline CGRect MCRectangleToCGRect(const MCRectangle &p_rect)
{
    return CGRectMake(p_rect.x, p_rect.y, p_rect.width, p_rect.height);
}

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneRootView : UIView <UITextViewDelegate>
{
	// MW-2012-03-05: [[ ViewStack ]] The stack which is currently being displayed.
	MCStack *m_stack;
	
	UITextView *m_text_view;
	MCIPhoneMainView *m_main_view;
	MCIPhoneDisplayView *m_display_view;
	
	UIImageView *m_rotation_image_view;
	
	bool m_in_deactivate_keyboard : 1;
	bool m_screen_updates_were_enabled : 1;
	
	bool m_animate_autorotation : 1;
}

- (id)init;
- (void)dealloc;

// MW-2012-03-05: [[ ViewStack ]] Property setter and getter for the current stack
//   of the view.
- (void)setCurrentStack: (MCStack *)p_stack;
- (MCStack *)currentStack;

- (void)reshape;

- (void)setAnimateAutorotation: (bool)animate;
- (void)beginAutorotation;
- (void)animateAutorotation;
- (void)endAutorotation;

- (void)textViewDidBeginEditing: (UITextView *)textView;
- (void)textViewDidEndEditing: (UITextView *)textView;
- (BOOL)textView: (UITextView *)textView shouldChangeTextInRange: (NSRange)range replacementText: (NSString*)text;

- (BOOL)activateKeyboard;
- (BOOL)deactivateKeyboard;

- (UITextView *)textView;
- (MCIPhoneMainView *)mainView;
- (MCIPhoneDisplayView *)displayView;

- (void)switchToDisplayClass: (Class)displayClass;

- (BOOL)com_runrev_livecode_passMotionEvents;

@end

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneMainView : UIView

- (BOOL)isMultipleTouchEnabled;

- (BOOL)canBecomeFirstResponder;
- (BOOL)becomeFirstResponder;

- (void)touchesBegan: (NSSet *)touches withEvent: (UIEvent *)event;
- (void)touchesMoved: (NSSet *)touches withEvent: (UIEvent *)event;
- (void)touchesEnded: (NSSet *)touches withEvent: (UIEvent *)event;
- (void)touchesCancelled: (NSSet *)touches withEvent: (UIEvent *)event;

@end

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneDisplayView : UIView

- (id)initWithFrame: (CGRect)p_frame;

- (BOOL)isOpaque;
- (BOOL)clearsContextBeforeDrawing;
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event;

- (void)render: (MCStack *)p_stack region: (MCRegionRef)p_region;

@end

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneUIKitDisplayView : MCIPhoneDisplayView

- (id)initWithFrame: (CGRect)p_frame;
- (void)dealloc;

- (void)drawRect: (CGRect)rect;
- (void)render: (MCStack *)p_stack region: (MCRegionRef)p_region;

@end

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneOpenGLDisplayView : MCIPhoneDisplayView
{
	EAGLContext *m_context;
	GLint m_backing_width;
	GLint m_backing_height;
	
	GLuint m_framebuffer;
	GLuint m_renderbuffer;
}

- (id)initWithFrame: (CGRect)p_frame;
- (void)dealloc;

- (BOOL)resizeFromLayer:(CAEAGLLayer *)layer;
- (void)render: (MCStack *)p_stack region: (MCRegionRef)p_region;

@end

////////////////////////////////////////////////////////////////////////////////

#endif
