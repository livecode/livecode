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

#include <Carbon/Carbon.h>
#include <Cocoa/Cocoa.h>
#include <QTKit/QTKit.h>

#include <revolution/external.h>
#include "videograbber.h"

////////////////////////////////////////////////////////////////////////////////

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define kDefaultAudioCompressor @"QTCompressionOptionsHighQualityAACAudio"
#define kDefaultVideoCompressor @"QTCompressionOptions240SizeH264Video"

class CQTXVideoGrabber: public CVideoGrabber
{
public:
	CQTXVideoGrabber(WindowPtr window);
	virtual ~CQTXVideoGrabber(void);
	
	virtual Bool IsInited();
	virtual void DoIdle();
	virtual void SetRect(short left, short top, short right, short bottom);
	virtual void GetRect(short *left, short *top, short *right, short *bottom);
	virtual void SetVisible(Bool tvisible);
	virtual void StartRecording(char *filename);
	virtual void StopRecording();
	virtual void AddError(char *serr);
	virtual void GetSettingsString(ExternalString &s);
	virtual void SetSettingsString(ExternalString &s);
	virtual void StartPreviewing();
	virtual void StopPreviewing();  
	virtual void VideoFormatDialog();
	virtual void VideoSourceDialog();
	virtual void VideoDisplayDialog();
	virtual void VideoCompressionDialog();
	virtual void VideoDefaultDialog(void);
	virtual void AudioDefaultDialog(void);
	virtual void SetAudioCapture(Bool bSet);
	virtual Bool GetAudioCapture(void);
	
	virtual char *getCompressors();
	virtual char *GetCurrentCodecName();
	virtual void setCompressor(char* codecName);
	
	virtual void setAudioCompressor(char *codecName);
	virtual char *GetCurrentAudioCodecName();
	virtual char *getAudioCompressors();
	
	virtual void setAudioFormat(int channels, int bits, int frequency);
	virtual void getAudioFormat(int* channels, int* bits, int* frequency);	
	
	virtual void GetFrameRate(double *frate);
	virtual void SetFrameRate(int framerate);
	
	virtual void GetFrameSize(int *fwidth,int *fheight);
	virtual void SetFrameSize(int fwidth,int fheight);
	
	virtual Bool Draw(int twidth,int theight, GWorldPtr gworldMem);
	
private:
	bool Initialize(WindowPtr window);
	void Finalize(void);
	void SyncOutputOptions(void);
	
	void AttachToParent(WindowRef p_parent);
	void DetachFromParent(void);
	void SyncWithParent(void);
	static OSStatus ParentEventHandler(EventHandlerCallRef p_call_chain, EventRef p_event, void *p_context);
	
	bool m_initialized;
	bool m_visible;
	Rect m_bounds;
	WindowRef m_parent;
	NSWindow *m_container;
	WindowGroupRef m_group;
	EventHandlerRef m_parent_handler;
	
	QTCaptureView *m_capture_view;
	QTCaptureSession *m_capture_session;
	QTCaptureDeviceInput *m_capture_video_device_input;
	QTCaptureDeviceInput *m_capture_audio_device_input;
	
	QTCaptureFileOutput *m_capture_output;
	
	NSString *m_audio_compressor;
	NSString *m_video_compressor;
	
	int32_t m_max_frame_width, m_max_frame_height;
	int32_t m_max_frame_rate;
};

////////////////////////////////////////////////////////////////////////////////

CQTXVideoGrabber::CQTXVideoGrabber(WindowPtr p_window)
{
	m_parent = nil;
	m_container = nil;
	m_group = nil;
	m_parent_handler = nil;
	m_visible = false;
	
	m_capture_view = nil;
	m_capture_session = nil;
	m_capture_video_device_input = nil;
	m_capture_audio_device_input = nil;
	m_capture_output = nil;
	
	m_max_frame_width = 0;
	m_max_frame_height = 0;
	m_max_frame_rate = 0;
	
	m_initialized = Initialize(p_window);
	if (!m_initialized)
		Finalize();
}

CQTXVideoGrabber::~CQTXVideoGrabber(void)
{
	Finalize();
}

Bool CQTXVideoGrabber::IsInited(void)
{
	return m_initialized;
}

////////////////////////////////////////////////////////////////////////////////

bool CQTXVideoGrabber::Initialize(WindowRef p_parent)
{
	// Set up floating window
	::SetRect(&m_bounds, 0, 0, 32, 32);
	Rect t_content_rect;
	GetWindowBounds(m_parent, kWindowContentRgn, &t_content_rect);
	m_container = [[NSWindow alloc] initWithContentRect: NSMakeRect(t_content_rect . left, t_content_rect . top, 32, 32) styleMask: NSBorderlessWindowMask backing: NSBackingStoreBuffered defer: NO];
	AttachToParent(p_parent);
	SyncWithParent();
	m_visible = true;
	
	// Now init the capture stuff
	m_capture_session = [[QTCaptureSession alloc] init];
	if (m_capture_session == nil)
		return false;
	
	m_capture_view = [[QTCaptureView alloc] initWithFrame: NSMakeRect(0, 0, 32, 32)];
	if (m_capture_view == nil)
		return false;
	
	[m_container setContentView: m_capture_view];
	
	// Error handling
	NSError *t_error;
	t_error = nil;
	
	// Create the output
	m_capture_output = [[QTCaptureMovieFileOutput alloc] init];
	if (![m_capture_session addOutput: m_capture_output error: &t_error])
		return false;
	
	// Open the default video device
	QTCaptureDevice *t_video_device;
	t_video_device = [QTCaptureDevice defaultInputDeviceWithMediaType: QTMediaTypeVideo];
	if (![t_video_device open: &t_error])
	{
		t_video_device = [QTCaptureDevice defaultInputDeviceWithMediaType: QTMediaTypeMuxed];
		if (![t_video_device open: &t_error])
			return false;
	}
	
	// Attach the video device
	m_capture_video_device_input = [[QTCaptureDeviceInput alloc] initWithDevice: t_video_device];
	if (![m_capture_session addInput: m_capture_video_device_input error: &t_error])
		return false;
	
	// Now see if we can attach audio, if we don't have it already
	if (![t_video_device hasMediaType: QTMediaTypeSound] && ![t_video_device hasMediaType: QTMediaTypeMuxed])
	{
		QTCaptureDevice *t_audio_device;
		t_audio_device = [QTCaptureDevice defaultInputDeviceWithMediaType: QTMediaTypeSound];
		if ([t_audio_device open: &t_error])
		{
			m_capture_audio_device_input = [[QTCaptureDeviceInput alloc] initWithDevice: t_audio_device];
			if (![m_capture_session addInput: m_capture_audio_device_input error: &t_error])
			{
				[m_capture_audio_device_input release];
				m_capture_audio_device_input = nil;
			}
		}
	}
	
	[m_capture_session startRunning];
	
	return true;
}

void CQTXVideoGrabber::Finalize(void)
{
	DetachFromParent();
		
	if (m_capture_session != nil)
		[m_capture_session stopRunning];
			 
	if ([[m_capture_video_device_input device] isOpen])
	{
		[[m_capture_video_device_input device] close];
		[m_capture_video_device_input release];
		m_capture_video_device_input = nil;
	}
	
	if ([[m_capture_audio_device_input device] isOpen])
	{
		[[m_capture_audio_device_input device] close];
		[m_capture_audio_device_input release];
		m_capture_audio_device_input = nil;
	}
	
	if (m_capture_output != nil)
	{
		[m_capture_session removeOutput: m_capture_output];
		
		[m_capture_output setDelegate: nil];
		[m_capture_output release];
		m_capture_output = nil;
	}
	
	
	if (m_capture_session != nil)
	{
		[m_capture_session release];
		m_capture_session = nil;
	}
	
	if (m_capture_view != nil)
	{
		[m_capture_view release];
		m_capture_view = nil;
	}
	
	if (m_container != nil)
	{
		[m_container orderOut: nil];
		[m_container release];
		m_container = nil;
	}
	
	if (m_video_compressor != nil)
	{
		[m_video_compressor release];
		m_video_compressor = nil;
	}
	
	if (m_audio_compressor != nil)
	{
		[m_audio_compressor release];
		m_audio_compressor = nil;
	}
}

void CQTXVideoGrabber::SyncOutputOptions(void)
{
	NSEnumerator *t_connections;
	t_connections = [[m_capture_output connections] objectEnumerator];

	QTCaptureConnection *t_connection;
	while((t_connection = [t_connections nextObject]))
	{
		NSString *t_media_type;
		t_media_type = [t_connection mediaType];
		
		QTCompressionOptions *t_options;
		t_options = nil;
		
		if ([t_media_type isEqualToString: QTMediaTypeVideo])
		{
			t_options = [QTCompressionOptions compressionOptionsWithIdentifier: m_video_compressor];
			if (t_options == nil)
				t_options = [QTCompressionOptions compressionOptionsWithIdentifier: kDefaultVideoCompressor];
		}
		else if ([t_media_type isEqualToString: QTMediaTypeSound])
		{
			t_options = [QTCompressionOptions compressionOptionsWithIdentifier: m_audio_compressor];
			if (t_options == nil)
				t_options = [QTCompressionOptions compressionOptionsWithIdentifier: kDefaultAudioCompressor];
		}
		
		[m_capture_output setCompressionOptions: t_options forConnection: t_connection];
	}

	if ([m_capture_output respondsToSelector: @selector(setMaximumVideoSize:)])
	{
		NSSize t_max_size;
		if (m_max_frame_width != 0 && m_max_frame_height != 0)
			t_max_size = NSZeroSize;
			else
				t_max_size = NSMakeSize(m_max_frame_width, m_max_frame_height);
		[m_capture_output setMaximumVideoSize: t_max_size];
	}

	if ([m_capture_output respondsToSelector: @selector(setMinimumVideoFrameInterval:)])
	{
		NSTimeInterval t_interval;
		if (m_max_frame_rate == 0)
			t_interval = 0;
			else
				t_interval = 1.0 / m_max_frame_rate;
		[m_capture_output setMinimumVideoFrameInterval: t_interval];
	}
}

void CQTXVideoGrabber::AttachToParent(WindowRef p_parent)
{
	m_parent = p_parent;
	
	WindowGroupRef t_current_group;
	t_current_group = GetWindowGroup(m_parent);
	
	WindowClass t_current_class;
	GetWindowClass(m_parent, &t_current_class);
	
	if (t_current_class == kFloatingWindowClass)
		[m_container setLevel: kCGFloatingWindowLevel];
	else if (t_current_class == kUtilityWindowClass)
		[m_container setLevel: kCGUtilityWindowLevel];

	m_group = NULL;
	if (t_current_group != NULL)
	{
		CFStringRef t_group_name;
		t_group_name = NULL;
		CopyWindowGroupName(t_current_group, &t_group_name);
		if (t_group_name != NULL)
		{
			if (CFStringCompare(t_group_name, CFSTR("MCCONTROLGROUP"), 0) == 0)
				m_group = t_current_group;
			CFRelease(t_group_name);
		}
	}
	
	if (m_group != NULL)
	{
		if (GetWindowGroup(m_parent) != m_group)
		{
			ChangeWindowGroupAttributes(m_group, 0, kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrSharedActivation);
			SetWindowGroupParent(m_group, GetWindowGroup(m_parent));
		}
		SetWindowGroup((WindowRef)[m_container windowRef], m_group);
	}
	else
	{
		CreateWindowGroup(0, &m_group);
		SetWindowGroupName(m_group, CFSTR("MCCONTROLGROUP"));
		SetWindowGroupOwner(m_group, m_parent);
		SetWindowGroupParent(m_group, GetWindowGroup(m_parent));
		SetWindowGroup(m_parent, m_group);
		SetWindowGroup((WindowRef)[m_container windowRef], m_group);
	}
	
	WindowGroupAttributes fwinAttributes = kWindowGroupAttrSelectAsLayer | kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrSharedActivation;
	ChangeWindowGroupAttributes(m_group, fwinAttributes, 0); 
	SetWindowGroupLevel(m_group, [m_container level]);
	
	static EventTypeSpec s_parent_events[] =
	{
		{ kEventClassWindow, kEventWindowBoundsChanged },
		{ kEventClassWindow, kEventWindowShown },
		{ kEventClassWindow, kEventWindowHidden },
		{ kEventClassWindow, kEventWindowClosed },
		{ kEventClassWindow, kEventWindowExpanded },
		{ kEventClassWindow, kEventWindowCollapsed }
	};
	
	InstallEventHandler(GetWindowEventTarget(m_parent), ParentEventHandler, sizeof(s_parent_events) / sizeof(EventTypeSpec), s_parent_events, this, &m_parent_handler);	
}

void CQTXVideoGrabber::DetachFromParent(void)
{
	if (m_parent_handler != nil)
	{
		RemoveEventHandler(m_parent_handler);
		m_parent_handler = nil;
	}
	
	if (m_container != nil)
	{
		SetWindowGroup((WindowRef)[m_container windowRef], GetWindowGroupOfClass(kDocumentWindowClass));
		[m_container orderOut: nil];
	}
	
	m_parent = nil;
}

void CQTXVideoGrabber::SyncWithParent(void)
{
	if (m_parent == NULL)
		return;
	
	Rect t_parent_bounds;
	GetWindowBounds(m_parent, kWindowContentRgn, &t_parent_bounds);
	
	HIRect t_view_bounds;
	t_view_bounds . origin . x = 0;
	t_view_bounds . origin . y = 0;
	t_view_bounds . size . width = m_bounds . right - m_bounds . left;
	t_view_bounds . size . height = m_bounds . bottom - m_bounds . top;
	
	Rect t_container_bounds;
	t_container_bounds . left = max(t_parent_bounds . left, t_parent_bounds . left + m_bounds . left);
	t_container_bounds . top = max(t_parent_bounds . top, t_parent_bounds . top + m_bounds . top);
	t_container_bounds . right = min(t_parent_bounds . right, t_parent_bounds . left + m_bounds . right);
	t_container_bounds . bottom = min(t_parent_bounds . bottom, t_parent_bounds . top + m_bounds . bottom);
	
	bool t_is_null;
	if (t_container_bounds . left >= t_container_bounds . right || t_container_bounds . top >= t_container_bounds . bottom)
		t_is_null = true;
	else
		t_is_null = false;
	
	if (!t_is_null)
	{
		ChangeWindowGroupAttributes(m_group,0, kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrSharedActivation);

		NSRect t_container_rect;
		t_container_rect = NSMakeRect(t_container_bounds . left, t_container_bounds . top, t_container_bounds . right - t_container_bounds . left, t_container_bounds . bottom - t_container_bounds . top);
		t_container_rect . origin . y = [[[NSScreen screens] objectAtIndex: 0] frame] . size . height - t_container_rect . size . height - t_container_rect . origin . y;
		[m_container setFrame: [NSWindow frameRectForContentRect: t_container_rect styleMask: NSBorderlessWindowMask] display: YES];
		
		if (m_capture_view != nil)
			[m_capture_view setFrame: NSMakeRect(0, t_container_rect . size . height - (m_bounds . bottom - m_bounds . top), m_bounds . right - m_bounds . left, m_bounds . bottom - m_bounds . top)];
		
		ChangeWindowGroupAttributes(m_group, kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrSharedActivation, 0);
	}
	
	bool t_parent_visible;
	t_parent_visible = IsWindowVisible(m_parent) && !IsWindowCollapsed(m_parent);
	
	if (t_parent_visible && m_visible && !t_is_null)
		[m_container orderFront: nil];
	else
		[m_container orderOut: nil];
}

OSStatus CQTXVideoGrabber::ParentEventHandler(EventHandlerCallRef p_call_chain, EventRef p_event, void *p_context)
{
	switch(GetEventKind(p_event))
	{
		case kEventWindowBoundsChanged:
		case kEventWindowShown:
		case kEventWindowHidden:
		case kEventWindowCollapsed:
		case kEventWindowExpanded:
			((CQTXVideoGrabber *)p_context) -> SyncWithParent();
			break;
			
		case kEventWindowClosed:
			break;
	}
	
	return eventNotHandledErr;
}

////////////////////////////////////////////////////////////////////////////////

void CQTXVideoGrabber::DoIdle()
{
}

void CQTXVideoGrabber::SetRect(short left, short top, short right, short bottom)
{
	::SetRect(&m_bounds, left, top, right, bottom);	
	SyncWithParent();
}

void CQTXVideoGrabber::GetRect(short *left, short *top, short *right, short *bottom)
{
	*left = m_bounds . left;
	*top = m_bounds . top;
	*right = m_bounds . right;
	*bottom = m_bounds . bottom;
}

void CQTXVideoGrabber::SetVisible(Bool p_visible)
{
	if (!m_initialized)
		return;
	
	bool t_is_visible;
	t_is_visible = [m_capture_view captureSession] != nil;
	
	if (t_is_visible == p_visible)
		return;
	
	if (p_visible)
		StartPreviewing();
	else
		StopPreviewing();
}

void CQTXVideoGrabber::StartRecording(char *filename)
{
	if (!m_initialized)
		return;
	
	if (videomode == VIDEOGRABBERMODE_RECORDING)
		return;
	
	SyncOutputOptions();
		
	[m_capture_output recordToOutputFileURL: [NSURL fileURLWithPath: [NSString stringWithCString: filename encoding: NSMacOSRomanStringEncoding]]];

	videomode = VIDEOGRABBERMODE_RECORDING;
}

void CQTXVideoGrabber::StopRecording()
{
	if (videomode == VIDEOGRABBERMODE_RECORDING)
	{
		[m_capture_output recordToOutputFileURL: nil];
		videomode = VIDEOGRABBERMODE_PREVIEWING;
	}
	
	SyncOutputOptions();
}

void CQTXVideoGrabber::AddError(char *serr)
{
}

void CQTXVideoGrabber::GetSettingsString(ExternalString &s)
{
}

void CQTXVideoGrabber::SetSettingsString(ExternalString &s)
{
}

void CQTXVideoGrabber::StartPreviewing()
{
	StopRecording();
	[m_capture_view setCaptureSession: m_capture_session];
	videomode = VIDEOGRABBERMODE_PREVIEWING;
}

void CQTXVideoGrabber::StopPreviewing()
{
	if (videomode == VIDEOGRABBERMODE_PREVIEWING)
	{
		[m_capture_view setCaptureSession: nil];
		videomode = VIDEOGRABBERMODE_NONE;
	}
}

////////////////////////////////////////////////////////////////////////////////

static char *qtx_get_compressors(NSString *p_media_type)
{
	NSArray *t_compressors;
	t_compressors = [QTCompressionOptions compressionOptionsIdentifiersForMediaType: p_media_type];
	
	NSString *t_compressors_string;
	t_compressors_string = [t_compressors componentsJoinedByString: @"\n"];
	
	return strdup([t_compressors_string cStringUsingEncoding: NSMacOSRomanStringEncoding]);
}

////////////////////////////////////////////////////////////////////////////////

char *CQTXVideoGrabber::getCompressors()
{
	return qtx_get_compressors(QTMediaTypeVideo);
}

char *CQTXVideoGrabber::GetCurrentCodecName()
{
	NSString *t_codec;
	t_codec = m_video_compressor;
	if (t_codec == nil)
		t_codec = kDefaultVideoCompressor;
	return (char *)[t_codec cStringUsingEncoding: NSMacOSRomanStringEncoding];
}

void CQTXVideoGrabber::setCompressor(char* codecName)
{
	[m_video_compressor release];
	m_video_compressor = [NSString stringWithCString: codecName encoding: NSMacOSRomanStringEncoding];
	[m_video_compressor retain];
	
	if (videomode == VIDEOGRABBERMODE_PREVIEWING)
		SyncOutputOptions();
}

////////////////////////////////////////////////////////////////////////////////

char *CQTXVideoGrabber::getAudioCompressors()
{
	return qtx_get_compressors(QTMediaTypeSound);
}

char *CQTXVideoGrabber::GetCurrentAudioCodecName()
{
	NSString *t_codec;
	t_codec = m_audio_compressor;
	if (t_codec == nil)
		t_codec = kDefaultAudioCompressor;
	return (char *)[t_codec cStringUsingEncoding: NSMacOSRomanStringEncoding];
}

void CQTXVideoGrabber::setAudioCompressor(char *codecName)
{
	[m_audio_compressor release];
	m_audio_compressor = [NSString stringWithCString: codecName encoding: NSMacOSRomanStringEncoding];
	[m_audio_compressor retain];
	
	if (videomode == VIDEOGRABBERMODE_PREVIEWING)
		SyncOutputOptions();
}

////////////////////////////////////////////////////////////////////////////////

void CQTXVideoGrabber::SetAudioCapture(Bool bSet)
{
}

Bool CQTXVideoGrabber::GetAudioCapture(void)
{
	return False;
}

void CQTXVideoGrabber::setAudioFormat(int channels, int bits, int frequency)
{
}

void CQTXVideoGrabber::getAudioFormat(int* channels, int* bits, int* frequency)
{
}

////////////////////////////////////////////////////////////////////////////////

void CQTXVideoGrabber::GetFrameRate(double *frate)
{
	*frate = m_max_frame_rate;
}

void CQTXVideoGrabber::SetFrameRate(int framerate)
{
	m_max_frame_rate = framerate;
}

void CQTXVideoGrabber::GetFrameSize(int *fwidth,int *fheight)
{
	*fwidth = m_max_frame_width;
	*fheight = m_max_frame_height;
}

void CQTXVideoGrabber::SetFrameSize(int fwidth,int fheight)
{
	m_max_frame_width = fwidth;
	m_max_frame_height = fheight;
}

////////////////////////////////////////////////////////////////////////////////

Bool CQTXVideoGrabber::Draw(int twidth,int theight, GWorldPtr gworldMem)
{
    return False;
}

////////////////////////////////////////////////////////////////////////////////

void CQTXVideoGrabber::VideoFormatDialog()
{
}

void CQTXVideoGrabber::VideoSourceDialog()
{
}

void CQTXVideoGrabber::VideoDisplayDialog()
{
}

void CQTXVideoGrabber::VideoCompressionDialog()
{
}

void CQTXVideoGrabber::VideoDefaultDialog(void)
{
}

void CQTXVideoGrabber::AudioDefaultDialog(void)
{
}

////////////////////////////////////////////////////////////////////////////////

CVideoGrabber *CreateQTXVideoGrabber(WindowRef p_window)
{
	return new CQTXVideoGrabber(p_window);
}

////////////////////////////////////////////////////////////////////////////////
