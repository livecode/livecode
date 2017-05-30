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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "globals.h"
#include "object.h"
#include "mbldc.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <UIKit/UIGraphics.h>
#import <UIKit/UIImage.h>
#import <UIKit/UIImagePickerController.h>

#include "mblsyntax.h"
#include "mbliphoneapp.h"

////////////////////////////////////////////////////////////////////////////////

// MM-2013-09-23: [[ iOS7 Support ]] Added missing delegates implemented in order to appease llvm 5.0.
@interface com_runrev_livecode_MCIPhoneImagePickerDialog : UIImagePickerController<UIImagePickerControllerDelegate, UIPopoverControllerDelegate, UINavigationControllerDelegate>
{
	bool m_cancelled;
	bool m_running;
    bool m_lock_idle_timer;
	int32_t m_max_width;
	int32_t m_max_height;
	UIImagePickerControllerSourceType m_source_type;
	UIImagePickerControllerCameraDevice m_device_type;
	BOOL m_status_bar_hidden;
	UIStatusBarStyle m_status_bar_style;
	UIPopoverController *m_popover_controller;
	NSData *m_image_data;
}
@end

static com_runrev_livecode_MCIPhoneImagePickerDialog *s_image_picker = nil;

@implementation com_runrev_livecode_MCIPhoneImagePickerDialog

- (void)setMaxWidth:(int32_t)mwidth maxHeight:(int32_t)mheight
{
	m_max_width = mwidth;
	m_max_height = mheight;
}

- (NSData *)takeImageData
{
	NSData *t_data;
	t_data = m_image_data;
	m_image_data = nil;
	return t_data;
}

- (void)restoreIdleTimerDisabled
{
    if (m_lock_idle_timer)
        MCSystemLockIdleTimer();
}

// PM-2015-02-17: [[ Bug 11544 ]] Make sure the visibility of the status bar is respected when presenting a UIImagePickerController
- (void)navigationController:(UINavigationController *)navigationController willShowViewController:(UIViewController *)viewController animated:(BOOL)animated
{
    if (m_lock_idle_timer)
        MCSystemUnlockIdleTimer();
    [[UIApplication sharedApplication] setStatusBarHidden:m_status_bar_hidden];
}

// MW-2011-01-18: [[ Bug 9303 ]] Make sure we take account of scale and orientation of the UIImage.
- (void)imagePickerController: (UIImagePickerController*)controller didFinishPickingImage: (UIImage *)p_image editingInfo: (NSDictionary*)info
{
	uint32_t t_width, t_height;
	t_width = [p_image size] . width * [p_image scale];
	t_height = [p_image size] . height * [p_image scale];
	
	double t_scale_x, t_scale_y;
	t_scale_x = m_max_width != 0  && t_width > m_max_width ? m_max_width / (double)t_width : 1.0;
	t_scale_y = m_max_height != 0 && t_height > m_max_height ? m_max_height / (double)t_height : 1.0;
	
	double t_scale;
	t_scale = fmin(t_scale_x, t_scale_y);
    
    // MM-2012-05-03: [[ Bug 10192 ]] Portrait images imported from camera are in lnadscape oreintation.
    //  Solve by scaling images to appropriate wisth and height.
	if (t_scale < 0.0 || t_scale > 1.0)
        t_scale = 1.0;
        
    CGSize t_size;
    t_size = CGSizeMake(t_width * t_scale, t_height * t_scale);
    UIGraphicsBeginImageContext(t_size);
    [p_image drawInRect: CGRectMake(0.0f, 0.0f, t_size . width, t_size . height)];
    p_image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
	
	NSData *t_data;
	t_data = nil;
	if (MCpaintcompression == EX_JPEG)
		t_data = UIImageJPEGRepresentation(p_image, MCjpegquality / 100.0f);
	else
		t_data = UIImagePNGRepresentation(p_image);
	
	[t_data retain];
	m_image_data = t_data;
	
	m_running = false;
	m_cancelled = false;
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

- (void)imagePickerControllerDidCancel: (UIImagePickerController*)controller
{
	m_running = false;
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

- (void)popoverControllerDidDismissPopover: (UIPopoverController *)popoverController
{
	m_running = false;
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

- (void)preWait
{	
	// MM-2012-03-01: [[ BUG 10033 ]] Store the status bar style as it appears on iOS 5, this is overwittern by album picker
    m_status_bar_hidden = [[UIApplication sharedApplication] isStatusBarHidden];
    m_status_bar_style = [[UIApplication sharedApplication] statusBarStyle];
	m_lock_idle_timer = ([[UIApplication sharedApplication] isIdleTimerDisabled] == YES);
    
	[ self setSourceType: m_source_type ];
	
	if (m_source_type == UIImagePickerControllerSourceTypeCamera &&
		[UIImagePickerController isCameraDeviceAvailable: m_device_type])
		[self setCameraDevice: m_device_type]; 

    [ self setDelegate: (id<UINavigationControllerDelegate, UIImagePickerControllerDelegate>)self ];

	// 19/10/2010 IM - fix crash in iOS4.0
	// UIPopoverController only available on iPad but NSClassFromString returns non-nil in iOS4 phone/pod
    // AL-2013-10-04 [[ Bug 11255 ]] Uninitialised variable can cause crash in iPhonePickPhoto
    id t_popover = nil;
    uint32_t t_orientations;
	// PM-2016-02-23: [[ Bug 16972 ]] Fix crash when accessing photo lib
    t_orientations = [MCIPhoneGetApplication() allowedOrientations];
    bool t_allowed_landscape = false;
    bool t_allowed_portrait_upside_down = false;
    
    if ((t_orientations & (1 << UIInterfaceOrientationLandscapeLeft)) != 0 || (t_orientations & (1 << UIInterfaceOrientationLandscapeRight)) != 0)
        t_allowed_landscape = true;
    
    if ((t_orientations & (1 << UIInterfaceOrientationPortraitUpsideDown)) != 0)
        t_allowed_portrait_upside_down = true;
    
    // PM-2014-10-13: [[ Bug 13236 ]] If we are on iPad and the supported orientations contain any of LandscapeLeft, LandscapeRight, PortraitUpsideDown,
    // then the photo-picker is displayed using the standard iOS fullscreen overlay view (as it is the case with iphone).
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad && !t_allowed_landscape && !t_allowed_portrait_upside_down && m_source_type == UIImagePickerControllerSourceTypeCamera)
        t_popover = NSClassFromString(@"UIPopoverController");
    
    // PM-2014-10-17: [[ Bug 13708 ]] If we are on iPad and the source is other than camera, use a popover to display the photo-picker.
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad && m_source_type != UIImagePickerControllerSourceTypeCamera)
        t_popover = NSClassFromString(@"UIPopoverController");
	
	if (t_popover != nil)
	{
		UIViewController *t_main_controller;
		t_main_controller = MCIPhoneGetViewController();
		
		CGRect t_rect;
		if (MCtargetptr)
		{
			MCRectangle t_mc_rect = MCtargetptr -> getrect();
			t_rect = MCUserRectToLogicalCGRect(t_mc_rect);
		}
		else
			t_rect = [[t_main_controller view] frame];
		
		m_popover_controller = [[t_popover alloc] initWithContentViewController: self];
        [m_popover_controller setDelegate: (id<UIPopoverControllerDelegate>)self];

		[m_popover_controller presentPopoverFromRect: t_rect inView: [t_main_controller view] permittedArrowDirections: UIPopoverArrowDirectionAny animated: YES];
	}
	else
		[ MCIPhoneGetViewController() presentModalViewController: self animated: YES ];
}

- (void)postWait
{
	if (m_popover_controller != nil)
	{
		[m_popover_controller dismissPopoverAnimated: YES];
		[m_popover_controller release];
		
	}
	else		
		[ MCIPhoneGetViewController() dismissModalViewControllerAnimated: YES ];
	
	// MM-2012-03-01: [[ BUG 10033 ]] Restore the status bar style after picker dismissed
    [[UIApplication sharedApplication] setStatusBarHidden: m_status_bar_hidden withAnimation: UIStatusBarAnimationNone];
    [[UIApplication sharedApplication] setStatusBarStyle: m_status_bar_style animated: NO];
    
    // PM-2017-05-17: [[Bug 19646]] We need at least a 0.6 delay, otherwise the value of lockIdleTimer is not restored, not sure why
    [self performSelector:@selector(restoreIdleTimerDisabled) withObject:nil afterDelay:0.6];
}

+ (void)prepare
{
	// MM-2011-12-09: [[ Bug 9902 ]] Destroy previous picker.  Fixes bug with iOS 5 where 
	//		cameraDevice is ignored on second running of pickPhoto.
    if (s_image_picker != nil)
    {
        [s_image_picker release];
        s_image_picker = nil;
    }
    
	if (s_image_picker == nil)
		s_image_picker = [[com_runrev_livecode_MCIPhoneImagePickerDialog alloc] init];
	
}

+ (NSData *)showModalForSource: (UIImagePickerControllerSourceType)sourceType deviceType: (UIImagePickerControllerCameraDevice)deviceType maxWidth: (int32_t)maxWidth maxHeight: (int32_t)maxHeight
{
	MCIPhoneCallSelectorOnMainFiber([com_runrev_livecode_MCIPhoneImagePickerDialog class], @selector(prepare));
	
	s_image_picker -> m_running = true;
	s_image_picker -> m_cancelled = true;
	s_image_picker -> m_image_data = nil;
	s_image_picker -> m_max_width = maxWidth;
	s_image_picker -> m_max_height = maxHeight;
	s_image_picker -> m_source_type = sourceType;
	s_image_picker -> m_device_type = deviceType;
	
	MCIPhoneCallSelectorOnMainFiber(s_image_picker, @selector(preWait));
	
	while(s_image_picker -> m_running)
		MCscreen -> wait(60.0, True, True);
	
	MCIPhoneCallSelectorOnMainFiber(s_image_picker, @selector(postWait));
	
	return [s_image_picker takeImageData];
}

@end

////////////////////////////////////////////////////////////////////////////////

static void map_photo_source_to_source_and_device(MCPhotoSourceType p_source, UIImagePickerControllerSourceType& r_source_type, UIImagePickerControllerCameraDevice& r_device_type)
{
	switch(p_source)
	{
		case kMCPhotoSourceTypeLibrary:
			r_source_type = UIImagePickerControllerSourceTypePhotoLibrary;
			break;
		case kMCPhotoSourceTypeAlbum:
			r_source_type = UIImagePickerControllerSourceTypeSavedPhotosAlbum;
			break;
		case kMCPhotoSourceTypeFrontCamera:
			r_source_type = UIImagePickerControllerSourceTypeCamera;
			r_device_type = UIImagePickerControllerCameraDeviceFront;
			break;
        case kMCPhotoSourceTypeCamera:
		case kMCPhotoSourceTypeRearCamera:
			r_source_type = UIImagePickerControllerSourceTypeCamera;
			r_device_type = UIImagePickerControllerCameraDeviceRear;
			break;
		default:
			assert(false);
			break;
	}
}

bool MCSystemCanAcquirePhoto(MCPhotoSourceType p_source)
{
	UIImagePickerControllerSourceType t_source_type;
	UIImagePickerControllerCameraDevice t_device_type;
	map_photo_source_to_source_and_device(p_source, t_source_type, t_device_type);
	
	if (![UIImagePickerController isSourceTypeAvailable: t_source_type])
		return false;
	
	if (t_source_type == UIImagePickerControllerSourceTypeCamera &&
		![UIImagePickerController isCameraDeviceAvailable: t_device_type])
		return false;
	
	return true;
}

bool MCSystemAcquirePhoto(MCPhotoSourceType p_source, int32_t p_max_width, int32_t p_max_height, void*& r_image_data, size_t& r_image_data_size, MCStringRef& r_result)
{
	UIImagePickerControllerSourceType t_source_type;
	UIImagePickerControllerCameraDevice t_device_type;
	map_photo_source_to_source_and_device(p_source, t_source_type, t_device_type);
	
	NSData *t_ns_image_data;
	t_ns_image_data = [com_runrev_livecode_MCIPhoneImagePickerDialog showModalForSource: t_source_type deviceType: t_device_type maxWidth: p_max_width maxHeight: p_max_height];
	
	if (t_ns_image_data != nil)
	{
		/* UNCHECKED */ MCMemoryAllocateCopy([t_ns_image_data bytes], [t_ns_image_data length], r_image_data);
		r_image_data_size = [t_ns_image_data length];
	}
	else
	{
		r_image_data = nil;
		r_image_data_size = 0;
        MCStringCreateWithCString("cancel", r_result);
	}
	
	[t_ns_image_data release];

	return true;
}

MCCameraFeaturesType MCSystemGetSpecificCameraFeatures(MCCameraSourceType p_source)
{
	UIImagePickerControllerCameraDevice t_device_type;
	if (p_source == kMCCameraSourceTypeRear)
		t_device_type = UIImagePickerControllerCameraDeviceRear;
	else
		t_device_type = UIImagePickerControllerCameraDeviceFront;

	if (![UIImagePickerController isCameraDeviceAvailable: t_device_type])
		return 0;
	
	MCCameraFeaturesType t_features;
	t_features = kMCCameraFeaturePhoto;
	
	if ([UIImagePickerController isFlashAvailableForCameraDevice: t_device_type])
		t_features |= kMCCameraFeatureFlash;
	
	for(NSNumber *t_element in [UIImagePickerController availableCaptureModesForCameraDevice: t_device_type])
		if ([t_element longValue] == UIImagePickerControllerCameraCaptureModeVideo)
		{
			t_features |= kMCCameraFeatureVideo;
			break;
		}
	
	return t_features;
}

MCCamerasFeaturesType MCSystemGetAllCameraFeatures()
{
    MCCameraFeaturesType t_front_features;
    MCCameraFeaturesType t_rear_features;
    
    t_front_features = MCSystemGetSpecificCameraFeatures(kMCCameraSourceTypeFront);
    t_rear_features = MCSystemGetSpecificCameraFeatures(kMCCameraSourceTypeRear);
    
    MCCamerasFeaturesType t_features;
    t_features = 0;
    
    t_features |= t_front_features;
    t_features |= (t_rear_features << kMCCameraFeatureRearShift);
    
    return t_features;
}

////////////////////////////////////////////////////////////////////////////////

