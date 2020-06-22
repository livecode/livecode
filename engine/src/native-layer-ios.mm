/* Copyright (C) 2015 LiveCode Ltd.
 
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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"


#include "util.h"
#include "mcerror.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "widget.h"
#include "param.h"
#include "osspec.h"
#include "cmds.h"
#include "scriptpt.h"
#include "hndlrlst.h"
#include "debug.h"
#include "redraw.h"
#include "font.h"
#include "chunk.h"
#include "graphicscontext.h"
#include "objptr.h"
#include "group.h"

#include "globals.h"
#include "context.h"

#include "native-layer-ios.h"

#include "platform.h"
#include "platform-internal.h"

#include "mbliphoneapp.h"
#include "mblcontrol.h"

#include "graphics_util.h"

MCNativeLayerIOS::MCNativeLayerIOS(MCObject *p_object, UIView *p_native_view) :
  m_view([p_native_view retain])
{
	m_object = p_object;
}

MCNativeLayerIOS::~MCNativeLayerIOS()
{
    if (m_view != nil)
    {
        doDetach();
        MCIPhoneRunBlockOnMainFiber(^{[m_view release];});
    }
}

void MCNativeLayerIOS::doAttach()
{
    // Act as if there was a re-layer to put the widget in the right place
    doRelayer();
    
	doSetVisible(ShouldShowLayer());
}

void MCNativeLayerIOS::doDetach()
{
    // Remove the view from the stack's content view
    MCIPhoneRunBlockOnMainFiber(^{
        [m_view removeFromSuperview];
    });
}

// Rendering view to context not supported on iOS.
bool MCNativeLayerIOS::GetCanRenderToContext()
{
	return false;
}

bool MCNativeLayerIOS::doPaint(MCGContextRef p_context)
{
	return false;
}

void MCNativeLayerIOS::doSetViewportGeometry(const MCRectangle& p_rect)
{
}

void MCNativeLayerIOS::doSetGeometry(const MCRectangle& p_rect)
{
    CGRect t_nsrect;
    MCGRectangle t_xformed;
    t_xformed = MCNativeControlUserRectToDeviceRect(MCGRectangleMake(p_rect.x, p_rect.y, p_rect.width, p_rect.height));
    t_nsrect = CGRectMake(t_xformed.origin.x, t_xformed.origin.y, t_xformed.size.width, t_xformed.size.height);
    
    MCIPhoneRunBlockOnMainFiber(^{
        [m_view setFrame:t_nsrect];
        [m_view setNeedsDisplay];
    });
}

void MCNativeLayerIOS::doSetVisible(bool p_visible)
{
    MCIPhoneRunBlockOnMainFiber(^{
        [m_view setHidden:p_visible ? NO : YES];
    });
	if (p_visible)
		doSetGeometry(m_object->getrect());
}

void MCNativeLayerIOS::doRelayer()
{
    // Find which native layer this should be inserted below
    MCObject* t_before;
    t_before = findNextLayerAbove(m_object);
	
	UIView *t_parent_view;
	t_parent_view = nil;
	
	if (!getParentView(t_parent_view))
		return;
	
    // Insert the widget in the correct place (but only if the card is current)
    if (isAttached() && m_object->getstack()->getcard() == m_object->getstack()->getcurcard())
    {
        MCIPhoneRunBlockOnMainFiber(^{
            [m_view removeFromSuperview];
			if (t_before != nil)
			{
				// There is another native layer above this one
				UIView *t_before_view;
				/* UNCHECKED */ t_before->GetNativeView((void*&)t_before_view);
				[t_parent_view insertSubview:m_view belowSubview:t_before_view];
			}
            else
            {
                // This is the top-most native layer
                [t_parent_view addSubview:m_view];
            }
            [t_parent_view setNeedsDisplay];
        });
    }
}

bool MCNativeLayerIOS::getParentView(UIView *&r_view)
{
	if (m_object->getparent()->gettype() == CT_GROUP)
	{
		MCNativeLayer *t_container;
		t_container = nil;
		
		if (!((MCGroup*)m_object->getparent())->getNativeContainerLayer(t_container))
			return false;
		
		return t_container->GetNativeView((void*&)r_view);
	}
	else
	{
		r_view = MCIPhoneGetView();
		return true;
	}
}

bool MCNativeLayerIOS::GetNativeView(void *&r_view)
{
	if (m_view == nil)
		return false;
	
	r_view = m_view;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer *MCNativeLayer::CreateNativeLayer(MCObject *p_object, void *p_native_view)
{
	return new MCNativeLayerIOS(p_object, (UIView*)p_native_view);
}

//////////

// IM-2015-12-16: [[ NativeLayer ]] Keep the coordinate system of group contents the same as
//                the top-level window view by keeping its bounds the same as its frame.
//                This allows us to place contents in terms of window coords without having to
//                adjust for the location of the group container.
@interface com_runrev_livecode_MCContainerView: UIView

- (void)setFrame:(CGRect)frame;
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event;

@end

@compatibility_alias MCContainerView com_runrev_livecode_MCContainerView;

@implementation com_runrev_livecode_MCContainerView

- (void)setFrame:(CGRect)frame
{
	[super setFrame:frame];
	[self setBounds:frame];
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
	UIView *t_hit_view = [super hitTest:point withEvent:event];
	
	/* The container view should not trap any events so if the hitTest returns self we
	 * return nil so the heirarchy continues to be navigated to find a view */
	if (t_hit_view == self)
	{
		return nil;
	}
	
	return t_hit_view;
}

@end

bool MCNativeLayer::CreateNativeContainer(MCObject *p_object, void *&r_view)
{
	UIView *t_view;
	t_view = [[[MCContainerView alloc] init] autorelease];
	
	if (t_view == nil)
		return false;
	
	[t_view setAutoresizesSubviews:NO];
    [t_view setClipsToBounds:YES];
    
	r_view = t_view;
	
	return true;
}

//////////

void MCNativeLayer::ReleaseNativeView(void *p_view)
{
	MCIPhoneRunBlockOnMainFiber(^{[(UIView*)p_view release];});
}

////////////////////////////////////////////////////////////////////////////////
