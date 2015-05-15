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

#include <Cocoa/Cocoa.h>
#include <AVFoundation/AVFoundation.h>

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "util.h"

#include "platform.h"
#include "platform-internal.h"
#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

class MCPlatformCamera
{
public:
    MCPlatformCamera(void);
    virtual ~MCPlatformCamera(void);
    
    void Retain(void);
    void Release(void);

    virtual void Open(void) = 0;
    virtual void Close(void) = 0;
    
    virtual void Attach(void *owner) = 0;
    virtual void Detach(void) = 0;
    
    virtual bool SetProperty(MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value) = 0;
    virtual bool GetProperty(MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value) = 0;
    
    virtual bool StartRecording(MCStringRef filename) = 0;
    virtual bool StopRecording(void) = 0;
    
    virtual bool TakePicture(MCDataRef& r_data) = 0;
    
private:
    uindex_t m_references;
};

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCAVCameraView : NSView

#ifndef _MACOSX
+ (Class)layerClass;
#endif

- (AVCaptureSession *)session;
- (void)setSession: (AVCaptureSession *)session;

@end

////////////////////////////////////////////////////////////////////////////////

class MCAVCamera: public MCPlatformCamera
{
public:
    MCAVCamera(void);
    ~MCAVCamera(void);
    
    void Open(void);
    void Close(void);
    
    void Attach(void *owner);
    void Detach(void);
    
    bool SetProperty(MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value);
    bool GetProperty(MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value);
    
    bool StartRecording(MCStringRef p_filename);
    bool StopRecording(void);
    
    bool TakePicture(MCDataRef& r_data);
    
private:
    void Realize(void);
    void Unrealize(void);
    void Display(void);
    
    bool m_prepared : 1;
    bool m_running : 1;
    
    bool m_active : 1;
    MCRectangle m_rectangle;
    MCNameRef m_device;
    bool m_visible : 1;
    
    ////
    
    AVCaptureSession *m_session;
    
    AVCaptureDevice *m_audio_device;
    AVCaptureDevice *m_video_device;
    
    AVCaptureInput *m_audio_input;
    AVCaptureInput *m_video_input;
    AVCaptureStillImageOutput *m_image_output;
    AVCaptureMovieFileOutput *m_movie_output;
    
    com_runrev_livecode_MCAVCameraView *m_preview;
    
    void *m_owner;
};

////////////////////////////////////////////////////////////////////////////////

MCPlatformCamera::MCPlatformCamera(void)
{
    m_references = 1;
}

MCPlatformCamera::~MCPlatformCamera(void)
{
}

void MCPlatformCamera::Retain(void)
{
    m_references += 1;
}

void MCPlatformCamera::Release(void)
{
    m_references -= 1;
    if (m_references == 0)
        delete this;
}

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCAVCameraView

#ifndef _MACOSX
+ (Class)layerClass
{
    return [AVCaptureVideoPreviewLayer class];
}
#endif

- (AVCaptureSession *)session
{
    return [(AVCaptureVideoPreviewLayer *)[self layer] session];
}

- (void)setSession:(AVCaptureSession *)session
{
#ifdef _MACOSX
    if ([self layer] == nil)
    {
        AVCaptureVideoPreviewLayer *t_layer;
        t_layer = [AVCaptureVideoPreviewLayer layerWithSession: session];
        [t_layer setVideoGravity: AVLayerVideoGravityResize];
        [self setLayer: t_layer];
        [self setWantsLayer: YES];
        return;
    }
#endif
    [(AVCaptureVideoPreviewLayer *)[self layer] setSession:session];
}

@end

////////////////////////////////////////////////////////////////////////////////

MCAVCamera::MCAVCamera(void)
{
    m_prepared = false;
    m_running = false;
    
    m_active = true;
    MCU_set_rect(m_rectangle, 0, 0, 0, 0);
    m_device = nil;
    m_visible = true;
    
    m_session = nil;
    m_audio_device = nil;
    m_video_device = nil;
    m_audio_input = nil;
    m_video_input = nil;
    m_image_output = nil;
    m_movie_output = nil;
    
    m_preview = nil;
    
    m_owner = nil;
}

MCAVCamera::~MCAVCamera(void)
{
    Close();
}

void MCAVCamera::Open(void)
{
    if (m_prepared)
        return;
    
    m_session = [[AVCaptureSession alloc] init];
    
    m_audio_device = [[AVCaptureDevice defaultDeviceWithMediaType: AVMediaTypeAudio] retain];
    m_video_device = [[AVCaptureDevice defaultDeviceWithMediaType: AVMediaTypeVideo] retain];
    
    if (m_audio_device != nil)
        m_audio_input = [[AVCaptureDeviceInput deviceInputWithDevice: m_audio_device error: nil] retain];
    
    if (m_video_device != nil && m_video_device != m_audio_device)
        m_video_input = [[AVCaptureDeviceInput deviceInputWithDevice: m_video_device error: nil] retain];
    
    if (m_audio_input != nil)
        [m_session addInput: m_audio_input];
    
    if (m_video_input != nil)
        [m_session addInput: m_video_input];
    
    m_image_output = [[AVCaptureStillImageOutput alloc] init];
    [m_image_output setOutputSettings: @{ AVVideoCodecKey : AVVideoCodecJPEG }];
    [m_session addOutput: m_image_output];

    m_movie_output = [[AVCaptureMovieFileOutput alloc] init];
    [m_session addOutput: m_movie_output];
    
    m_preview = [[com_runrev_livecode_MCAVCameraView alloc] initWithFrame: NSMakeRect(0, 0, 0, 0)];
    [m_preview setSession: m_session];
    if (m_owner != nil)
        Realize();
    
    m_prepared = true;
    
    if (m_active)
    {
        [m_session startRunning];
        m_running = [m_session isRunning];
    }
    
    Realize();
}

void MCAVCamera::Close(void)
{
    if (!m_prepared)
        return;
    
    Unrealize();
    
    if (m_running)
    {
        [m_session stopRunning];
        m_running = false;
    }
    
    m_prepared = false;

    [m_preview setSession: nil];
    [m_preview release];
    m_preview = nil;
    
    [m_movie_output release];
    m_movie_output = nil;
    [m_image_output release];
    m_image_output = nil;
    [m_video_input release];
    m_video_input = nil;
    [m_audio_input release];
    m_audio_input = nil;
    [m_video_device release];
    m_video_device = nil;
    [m_audio_device release];
    m_audio_device = nil;
    [m_session release];
    m_session = nil;
}

void MCAVCamera::Attach(void *p_owner)
{
    if (m_owner == p_owner)
        return;
    
    if (m_owner != nil)
        Detach();
    
    m_owner = p_owner;
    
    if (m_prepared)
        Realize();
}

void MCAVCamera::Detach(void)
{
    if (m_owner == nil)
        return;
    
    if (m_prepared)
        Unrealize();
    
    m_owner = nil;
}

void MCAVCamera::Realize(void)
{
    if (m_owner == nil || !m_prepared)
        return;
    
#ifdef _MACOSX
	MCMacPlatformWindow *t_window;
	t_window = (MCMacPlatformWindow *)m_owner;
    
    // Force the window to have an NSWindow*.
    uint32_t t_id;
    MCPlatformGetWindowProperty(t_window, kMCPlatformWindowPropertySystemId, kMCPlatformPropertyTypeUInt32, &t_id);
    
    MCWindowView *t_parent_view;
    t_parent_view = t_window -> GetView();
    [t_parent_view addSubview: m_preview];
#endif
    
    Display();
}

void MCAVCamera::Unrealize(void)
{
    if (m_owner == nil || !m_prepared)
        return;
    
#ifdef _MACOSX
    MCMacPlatformWindow *t_window;
    t_window = (MCMacPlatformWindow *)m_owner;
    
    MCWindowView *t_parent_view;
    t_parent_view = t_window -> GetView();
    
    [m_preview removeFromSuperview];
#endif
}

void MCAVCamera::Display(void)
{
    if (m_owner == nil || !m_prepared)
        return;
    
#ifdef _MACOSX
	MCMacPlatformWindow *t_window;
	t_window = (MCMacPlatformWindow *)m_owner;
    
	NSRect t_frame;
	t_window -> MapMCRectangleToNSRect(m_rectangle, t_frame);
	[m_preview setFrame: t_frame];
    
	[m_preview setHidden: !m_visible];
#endif
}

bool MCAVCamera::SetProperty(MCPlatformCameraProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
    switch(p_property)
    {
        case kMCPlatformCameraPropertyRectangle:
			m_rectangle = *(MCRectangle *)p_value;
			Display();
            break;
        
        case kMCPlatformCameraPropertyVisible:
			m_visible = *(bool *)p_value;
			Display();
            break;
        
        case kMCPlatformCameraPropertyActive:
        {
            bool t_new_active;
            t_new_active = *(bool *)p_value;
            if (t_new_active != m_active)
            {
                if (m_active)
                    [m_session stopRunning];
                else
                    [m_session startRunning];
                
                m_active = t_new_active;
            }
        }
        break;

        case kMCPlatformCameraPropertyDevice:
            break;
        case kMCPlatformCameraPropertyDevices:
            break;
        case kMCPlatformCameraPropertyFeatures:
            break;
        case kMCPlatformCameraPropertyFlashMode:
            break;
        case kMCPlatformCameraPropertyIsFlashActive:
            break;
        case kMCPlatformCameraPropertyIsFlashAvailable:
            break;
    }
    return true;
}

bool MCAVCamera::GetProperty(MCPlatformCameraProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
    switch(p_property)
    {
        case kMCPlatformCameraPropertyRectangle:
			*(MCRectangle *)r_value = m_rectangle;
            break;
            
        case kMCPlatformCameraPropertyVisible:
			*(bool *)r_value = m_visible;
            break;
            
        case kMCPlatformCameraPropertyActive:
            *(bool *)r_value = m_active;
            break;
            
        case kMCPlatformCameraPropertyDevice:
            break;
        case kMCPlatformCameraPropertyDevices:
            break;
        case kMCPlatformCameraPropertyFeatures:
            break;
        case kMCPlatformCameraPropertyFlashMode:
            break;
        case kMCPlatformCameraPropertyIsFlashActive:
            break;
        case kMCPlatformCameraPropertyIsFlashAvailable:
            break;
    }
    
    return true;
}

bool MCAVCamera::StartRecording(MCStringRef p_filename)
{
    return false;
}

bool MCAVCamera::StopRecording(void)
{
    return false;
}

bool MCAVCamera::TakePicture(MCDataRef& r_data)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCameraCreate(MCPlatformCameraRef& r_camera)
{
    if (MCmajorosversion >= 0x1070)
        r_camera = new MCAVCamera;
    else
        r_camera = nil;
}

void MCPlatformCameraRetain(MCPlatformCameraRef camera)
{
    if (camera != nil)
        camera -> Retain();
}

void MCPlatformCameraRelease(MCPlatformCameraRef camera)
{
    if (camera != nil)
        camera -> Release();
}

void MCPlatformCameraAttach(MCPlatformCameraRef camera, void *p_target)
{
    if (camera != nil)
        camera -> Attach(p_target);
}

void MCPlatformCameraDetach(MCPlatformCameraRef camera)
{
    if (camera != nil)
        camera -> Detach();
}

void MCPlatformCameraOpen(MCPlatformCameraRef camera)
{
    if (camera != nil)
        camera -> Open();
}

void MCPlatformCameraClose(MCPlatformCameraRef camera)
{
    if (camera != nil)
        camera -> Close();
}

bool MCPlatformCameraSetProperty(MCPlatformCameraRef camera, MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value)
{
    if (camera != nil)
        return camera -> SetProperty(property, type, value);
    return true;
}

bool MCPlatformCameraGetProperty(MCPlatformCameraRef camera, MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value)
{
    if (camera != nil)
        return camera -> GetProperty(property, type, value);
    return true;
}

bool MCPlatformCameraStartRecording(MCPlatformCameraRef camera, MCStringRef filename)
{
    if (camera != nil)
        return camera -> StartRecording(filename);
    return false;
}

bool MCPlatformCameraStopRecording(MCPlatformCameraRef camera)
{
    if (camera != nil)
        return camera -> StopRecording();
    return false;
}

bool MCPlatformCameraTakePicture(MCPlatformCameraRef camera, MCDataRef& r_image_data)
{
    if (camera != nil)
        return camera -> TakePicture(r_image_data);
    return false;
}

////////////////////////////////////////////////////////////////////////////////
