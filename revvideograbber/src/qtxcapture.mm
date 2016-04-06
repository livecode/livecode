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

#include "revcapture.h"

////////////////////////////////////////////////////////////////////////////////

@interface QTCaptureInput (MCQTXCaptureAdditions)
- (void)setEnabled: (BOOL)p_enabled forMediaType: (NSString *)p_media_type;
@end

@implementation QTCaptureInput (MCQTXCaptureAdditions)
- (void)setEnabled: (BOOL)p_enabled forMediaType: (NSString *)p_media_type
{
	// And we just need to enable the 'input media' connection(s)
	NSArray *t_connections;
	t_connections = [self connections];
	for(uint32_t i = 0; i < [t_connections count]; i++)
	{
		QTCaptureConnection *t_connection;
		t_connection = (QTCaptureConnection *)[t_connections objectAtIndex: i];
		if ([t_connection mediaType] == p_media_type)
			[t_connection setEnabled: p_enabled];
	}
}
@end

////////////////////////////////////////////////////////////////////////////////

class MCQTXCaptureInputDevice: public MCCaptureInputDevice
{
public:
	static bool Create(MCQTXCaptureInputDevice *next, QTCaptureDevice *device, MCQTXCaptureInputDevice*& r_device);
	static void Destroy(MCQTXCaptureInputDevice *device);
	
	/////////
	
	bool IsDefaultForAudio(void);
	bool IsDefaultForVideo(void);
	
	bool IsAudio(void);
	bool IsVideo(void);
	
	const char *GetName(void);
	
	////////
	
	MCQTXCaptureInputDevice *GetNext(void);
	QTCaptureDevice *GetCaptureDevice(void);
	
private:
	MCQTXCaptureInputDevice(MCQTXCaptureInputDevice *next, QTCaptureDevice *device);
	~MCQTXCaptureInputDevice(void);
	
	MCQTXCaptureInputDevice *m_next;
	QTCaptureDevice *m_device;
};

struct MCQTXCaptureAudioPreviewDevice: public MCCaptureAudioPreviewDevice
{
public:
	static bool Create(MCQTXCaptureAudioPreviewDevice *next, MCQTXCaptureAudioPreviewDevice*& r_device);
	static void Destroy(MCQTXCaptureAudioPreviewDevice *device);
	
	bool IsDefault(void);
	const char *GetName(void);
	
	MCQTXCaptureAudioPreviewDevice *GetNext(void);
	
private:
	MCQTXCaptureAudioPreviewDevice(MCQTXCaptureAudioPreviewDevice *next);
	~MCQTXCaptureAudioPreviewDevice(void);
	
	MCQTXCaptureAudioPreviewDevice *m_next;
};

class MCQTXCaptureSession: public MCCaptureSession
{
public:
	MCQTXCaptureSession(void);
	
	virtual bool Create(MCCaptureSessionDelegate *delegate);
	virtual void Destroy(void);
	
	virtual MCCaptureError Error(void);
	virtual const char *ErrorDetail(void);
	
	virtual bool ListInputDevices(MCCaptureListInputDevicesCallback callback, void *context);
	virtual bool GetAudioInputDevice(MCCaptureInputDevice*& r_device);
	virtual bool SetAudioInputDevice(MCCaptureInputDevice* device);
	virtual bool GetVideoInputDevice(MCCaptureInputDevice*& r_device);
	virtual bool SetVideoInputDevice(MCCaptureInputDevice* device);
	
	virtual bool ListAudioPreviewDevices(MCCaptureListAudioPreviewDevicesCallback callback, void *context);
	virtual bool GetAudioPreviewDevice(MCCaptureAudioPreviewDevice*& r_device);
	virtual bool SetAudioPreviewDevice(MCCaptureAudioPreviewDevice *device);
	virtual bool GetAudioPreviewVolume(double*& r_volume);
	virtual bool SetAudioPreviewVolume(double volume);
	virtual bool SetVideoPreviewBuffer(const MCCaptureImageBuffer *buffer);
	virtual bool GetVideoPreviewBuffer(MCCaptureImageBuffer& r_buffer);
	
	virtual bool GetPreviewingState(MCCaptureState& r_status);
	virtual bool StartPreviewing(void);
	virtual bool StopPreviewing(void);
	virtual bool PausePreviewing(void);
	virtual bool ResumePreviewing(void);
	
private:
	bool Throw(MCCaptureError error, NSError *detail);
	
	//////////
	
	bool BuildInputDeviceList(void);
	void ClearInputDeviceList(void);
	bool ValidateInputDevice(MCCaptureInputDevice *device);
	bool FindInputDeviceByObject(QTCaptureDevice *object, MCCaptureInputDevice*& r_input_device);
	
	bool BuildAudioPreviewDeviceList(void);
	void ClearAudioPreviewDeviceList(void);
	
	//////////
	
	bool AddDeviceInput(QTCaptureDevice *p_new_device,
						NSString *p_this_media, QTCaptureDevice*& x_this_device, QTCaptureDeviceInput*& x_this_input,
						NSString *p_other_media, QTCaptureDevice *p_other_device, QTCaptureDeviceInput *p_other_input);
	void RemoveDeviceInput(NSString *p_this_media, QTCaptureDevice*& x_this_device, QTCaptureDeviceInput*& x_this_input,
						   NSString *p_other_media, QTCaptureDevice *p_other_device, QTCaptureDeviceInput *p_other_input);
	bool ConfigureDeviceInput(QTCaptureDevice *p_new_device,
							  NSString *p_this_media, QTCaptureDevice*& x_this_device, QTCaptureDeviceInput*& x_this_input,
							  NSString *p_other_media, QTCaptureDevice *p_other_device, QTCaptureDeviceInput *p_other_input);

	//////////
	
	MCCaptureError m_error;
	char *m_error_detail;
	
	MCCaptureSessionDelegate *m_delegate;
	
	MCCaptureState m_preview_state;
	MCCaptureState m_record_state;
	
	QTCaptureSession *m_session;
	QTCaptureDevice *m_audio_device;
	QTCaptureDevice *m_video_device;
	QTCaptureDeviceInput *m_audio_input;
	QTCaptureDeviceInput *m_video_input;
	QTCaptureAudioPreviewOutput *m_audio_preview_output;
	QTCaptureVideoPreviewOutput *m_video_preview_output;
	
	CGContextRef m_video_preview_cgcontext;
	CIContext *m_video_preview_cicontext;
	
	MCQTXCaptureInputDevice *m_input_devices;
	MCQTXCaptureAudioPreviewDevice *m_audio_preview_devices;
};

////////////////////////////////////////////////////////////////////////////////

MCQTXCaptureSession::MCQTXCaptureSession(void)
{
	m_error = kMCCaptureErrorNone;
	m_error_detail = nil;
	
	m_delegate = nil;
	
	m_preview_state = kMCCaptureStateStopped;
	m_record_state = kMCCaptureStateStopped;
	
	m_session = nil;
	m_audio_device = nil;
	m_video_device = nil;
	m_audio_input = nil;
	m_video_input = nil;
	m_audio_preview_output = nil;
	m_video_preview_output = nil;
	
	m_video_preview_cgcontext = nil;
	m_video_preview_cicontext = nil;
	
	m_input_devices = nil;
	m_audio_preview_devices = nil;
}

bool MCQTXCaptureSession::Create(MCCaptureSessionDelegate *p_delegate)
{
	bool t_success;
	t_success = true;
	
	// Create the session
	if (t_success)
	{
		m_session = [[QTCaptureSession alloc] init];
		if (m_session == nil)
			t_success = Throw(kMCCaptureErrorCouldNotSetupSession, nil);
	}

	// Setup the list of available input devices
	if (t_success)
		if (!BuildInputDeviceList())
			t_success = Throw(kMCCaptureErrorCouldNotSetupSession, nil);
	
	// Setup the list of available audio preview devices
	if (t_success)
		if (!BuildAudioPreviewDeviceList())
			t_success = Throw(kMCCaptureErrorCouldNotSetupSession, nil);
	
	// Finish setting up other instance vars
	if (t_success)
	{
		[m_session startRunning];
		m_delegate = p_delegate;
	}
	else
		Destroy();
	
	return t_success;
}

void MCQTXCaptureSession::Destroy(void)
{
	if (m_preview_state != kMCCaptureStateStopped)
		StopPreviewing();
	
	ClearInputDeviceList();
	ClearAudioPreviewDeviceList();

	[m_audio_preview_output release];
	[m_video_preview_output release];
	
	[m_video_preview_cicontext release];
	if (m_video_preview_cgcontext != nil)
		CGContextRelease(m_video_preview_cgcontext);
			
	[m_session release];
	
	delete m_error_detail;
	
	delete this;
}
			
////////////////////////////////////////////////////////////////////////////////

bool MCQTXCaptureSession::BuildInputDeviceList(void)
{
	bool t_success;
	t_success = true;
	
	// Get the list of input devices in the system
	NSArray *t_devices;
	t_devices = nil;
	if (t_success)
		t_devices = [QTCaptureDevice inputDevices];
	
	// Loop through them building up our singly-linked list of devices as we go.
	if (t_success)
		for(uint32_t i = 0; i < [t_devices count] && t_success; i++)
			t_success = MCQTXCaptureInputDevice::Create(m_input_devices, (QTCaptureDevice *)[t_devices objectAtIndex: i], m_input_devices);
				
	// If something failed (memory allocation, in this case) clear out the list
	// and fail.
	if (!t_success)
		ClearInputDeviceList();
	
	return t_success;
}

void MCQTXCaptureSession::ClearInputDeviceList(void)
{
	while(m_input_devices != nil)
	{
		MCQTXCaptureInputDevice *t_current;
		t_current = m_input_devices;
		m_input_devices = m_input_devices -> GetNext();
		MCQTXCaptureInputDevice::Destroy(t_current);
	}	
}

bool MCQTXCaptureSession::FindInputDeviceByObject(QTCaptureDevice *p_capture_device, MCCaptureInputDevice*& r_device)
{
	for(MCQTXCaptureInputDevice *t_device = m_input_devices; t_device != nil; t_device = t_device -> GetNext())
		if (t_device -> GetCaptureDevice() == p_capture_device)
		{
			r_device = t_device;
			return true;
		}
	return false;
}

bool MCQTXCaptureSession::ValidateInputDevice(MCCaptureInputDevice *p_device)
{
	for(MCQTXCaptureInputDevice *t_device = m_input_devices; t_device != nil; t_device = t_device -> GetNext())
		if (p_device == t_device)
			return true;
	
	return Throw(kMCCaptuerErrorBadDevice, nil);
}

bool MCQTXCaptureSession::BuildAudioPreviewDeviceList(void)
{
	// For no we just add a mock 'system' object.
	// TODO: Query for audio outputs!
	
	return MCQTXCaptureAudioPreviewDevice::Create(m_audio_preview_devices, m_audio_preview_devices);
}

void MCQTXCaptureSession::ClearAudioPreviewDeviceList(void)
{
	while(m_audio_preview_devices != nil)
	{
		MCQTXCaptureAudioPreviewDevice *t_current;
		t_current = m_audio_preview_devices;
		m_audio_preview_devices = m_audio_preview_devices -> GetNext();
		MCQTXCaptureAudioPreviewDevice::Destroy(t_current);
	}
}

bool MCQTXCaptureSession::Throw(MCCaptureError p_error, NSError *p_error_detail)
{
	m_error = p_error;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCCaptureError MCQTXCaptureSession::Error(void)
{
	return m_error;
}

const char *MCQTXCaptureSession::ErrorDetail(void)
{
	return m_error_detail;
}

////////////////////////////////////////////////////////////////////////////////

void MCQTXCaptureSession::RemoveDeviceInput(NSString *p_this_media, QTCaptureDevice*& x_this_device, QTCaptureDeviceInput*& x_this_input,
											NSString *p_other_media, QTCaptureDevice *p_other_device, QTCaptureDeviceInput *p_other_input)
{
	if (x_this_device == p_other_device)
		[x_this_input setEnabled: NO forMediaType: p_this_media];
	else
	{
		[m_session removeInput: x_this_input];
		[x_this_device close];
	}
	
	[x_this_input release];
	x_this_input = nil;
	[x_this_device release];
	x_this_device = nil;
}

bool MCQTXCaptureSession::AddDeviceInput(QTCaptureDevice *p_new_device,
										 NSString *p_this_media, QTCaptureDevice*& x_this_device, QTCaptureDeviceInput*& x_this_input,
										 NSString *p_other_media, QTCaptureDevice *p_other_device, QTCaptureDeviceInput *p_other_input)
										 
{
	if (p_new_device == p_other_device)
	{
		[p_other_input setEnabled: YES forMediaType: p_this_media];
		x_this_device = [p_other_device retain];
		x_this_input = [p_other_input retain];
		return true;
	}
	
	bool t_success;
	t_success = true;
	
	bool t_did_open;
	t_did_open = false;
	if (t_success)
	{
		NSError *t_error;
		t_error = nil;
		if ([p_new_device open: &t_error])
			t_did_open = true;
		else
			t_success = Throw(kMCCaptureErrorCouldNotOpenDevice, t_error);
	}
	
	QTCaptureDeviceInput *t_new_input;
	t_new_input = nil;
	if (t_success)
	{
		t_new_input = [[QTCaptureDeviceInput alloc] initWithDevice: p_new_device];
		if (t_new_input == nil)
			t_success = Throw(kMCCaptureErrorCouldNotWrapDevice, nil);
	}
	
	if (t_success)
	{
		NSError *t_error;
		t_error = nil;
		if (![m_session addInput: t_new_input error: &t_error])
			t_success = Throw(kMCCaptureErrorCouldNotConnectToDevice, t_error);
	}
	
	if (t_success)
	{
		x_this_device = [p_new_device retain];
		x_this_input = t_new_input;
	}
	else
	{
		[t_new_input release];
		if (t_did_open)
			[p_new_device close];
	}
	
	return t_success;
}

bool MCQTXCaptureSession::ConfigureDeviceInput(QTCaptureDevice *p_new_device,
											   NSString *p_this_media, QTCaptureDevice*& x_this_device, QTCaptureDeviceInput*& x_this_input,
											   NSString *p_other_media, QTCaptureDevice *p_other_device, QTCaptureDeviceInput *p_other_input)
{	
	// Do nothing if the device is the same.
	if (p_new_device == x_this_device)
		return true;
	
	// Remove the current device input.
	RemoveDeviceInput(p_this_media, x_this_device, x_this_input, p_other_media, p_other_device, p_other_input);
	
	// Finally, add the new.
	return AddDeviceInput(p_new_device, p_this_media, x_this_device, x_this_input, p_other_media, p_other_device, p_other_input);
}

/////////

bool MCQTXCaptureSession::ListInputDevices(MCCaptureListInputDevicesCallback p_callback, void *p_context)
{
	for(MCQTXCaptureInputDevice *t_device = m_input_devices; t_device != nil; t_device = t_device -> GetNext())
		if (!p_callback(p_context, t_device))
			break;
	
	return true;
}

bool MCQTXCaptureSession::GetAudioInputDevice(MCCaptureInputDevice*& r_device)
{
	if (m_audio_device == nil)
	{
		r_device = nil;
		return true;
	}
	
	return FindInputDeviceByObject(m_audio_device, r_device);
}

bool MCQTXCaptureSession::SetAudioInputDevice(MCCaptureInputDevice *p_device)
{
	bool t_success;
	t_success = true;
	
	// Make sure the input pointer is actually one we recognize
	if (t_success)
		t_success = ValidateInputDevice(p_device);
	
	// Check that the device does audio
	if (t_success)
		if (p_device != nil && p_device -> IsAudio())
			t_success = Throw(kMCCaptureErrorNotAnAudioDevice, nil);
	
	// Now configure the input - this is slightly destructive it if fails.
	if (t_success)
		t_success = ConfigureDeviceInput(p_device != nil ? static_cast<MCQTXCaptureInputDevice *>(p_device) -> GetCaptureDevice() : nil, 
										 QTMediaTypeSound, m_audio_device, m_audio_input,
										 QTMediaTypeVideo, m_video_device, m_video_input);
	
	return t_success;
}

bool MCQTXCaptureSession::GetVideoInputDevice(MCCaptureInputDevice*& r_device)
{
	if (m_video_device == nil)
	{
		r_device = nil;
		return true;
	}
	
	return FindInputDeviceByObject(m_video_device, r_device);
}

bool MCQTXCaptureSession::SetVideoInputDevice(MCCaptureInputDevice *p_device)
{
	bool t_success;
	t_success = true;
	
	// Make sure the input pointer is actually one we recognize
	if (t_success)
		t_success = ValidateInputDevice(p_device);
	
	// Check that the device does audio
	if (t_success)
		if (p_device != nil && p_device -> IsVideo())
			t_success = Throw(kMCCaptureErrorNotAVideoDevice, nil);
	
	// Now configure the input - this is slightly destructive it if fails.
	if (t_success)
		t_success = ConfigureDeviceInput(p_device != nil ? static_cast<MCQTXCaptureInputDevice *>(p_device) -> GetCaptureDevice() : nil,
										 QTMediaTypeVideo, m_video_device, m_video_input, 
										 QTMediaTypeSound, m_audio_device, m_audio_input);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

@interface MCQTXCaptureVideoPreviewOutput : QTCaptureVideoPreviewOutput
{
	NSLock *m_lock;
	CIContext *m_context;
}

- (id)init;
- (void)dealloc;

- (void)setContext: (CIContext *)context;

- (void)outputVideoFrame:(CVImageBufferRef)videoFrame withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection;
@end

@implementation MCQTXCaptureVideoPreviewOutput

- (id)init
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_lock = [[NSLock alloc] init];
	
	return self;
}

- (void)dealloc
{
	[m_lock dealloc];
	[super dealloc];
}

- (void)setContext: (CIContext *)p_context
{
	[m_lock lock];
	m_context = p_context;
	[m_lock unlock];
}

- (void)outputVideoFrame:(CVImageBufferRef)videoFrame withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection
{
	CIImage *t_image;
	t_image = [CIImage imageWithCVImageBuffer: videoFrame];
	
	[m_lock lock];
	[m_context drawImage: t_image atPoint: CGPointMake(0.0, 0.0) fromRect: [t_image extent]];
	[m_lock unlock];
}

@end

bool MCQTXCaptureSession::ListAudioPreviewDevices(MCCaptureListAudioPreviewDevicesCallback callback, void *context)
{
	return false;
}

bool MCQTXCaptureSession::GetAudioPreviewDevice(MCCaptureAudioPreviewDevice*& r_device)
{
	return false;
}

bool MCQTXCaptureSession::SetAudioPreviewDevice(MCCaptureAudioPreviewDevice *device)
{
	return false;
}

bool MCQTXCaptureSession::GetAudioPreviewVolume(double*& r_volume)
{
	return false;
}

bool MCQTXCaptureSession::SetAudioPreviewVolume(double volume)
{
	return false;
}

bool MCQTXCaptureSession::SetVideoPreviewBuffer(const MCCaptureImageBuffer *p_buffer)
{	
	if (p_buffer == nil)
	{
		if (m_video_preview_output != nil)
		{
			[m_session removeOutput: m_video_preview_output];
			[m_video_preview_output release];
			m_video_preview_output = nil;
			
			[m_video_preview_cicontext release];
			m_video_preview_cicontext = nil;
			
			CGContextRelease(m_video_preview_cgcontext);
			m_video_preview_cgcontext = nil;
		}
		return true;
	}
	
	if (m_video_preview_output == nil)
	{
		NSError *t_error;
		t_error = nil;
		m_video_preview_output = [[MCQTXCaptureVideoPreviewOutput alloc] init];
		if (m_video_preview_output == nil ||
			![m_session addOutput: m_video_preview_output error: &t_error])
		{
			[m_video_preview_output release];
			m_video_preview_output = nil;
			return Throw(kMCCaptureErrorCouldNotSetupVideoPreview, t_error);
		}
	}
	
	// Disable rendering for the moment.
	[m_video_preview_output setContext: nil];
	
	[m_video_preview_cicontext release];
	if (m_video_preview_cgcontext != nil)
		CGContextRelease(m_video_preview_cgcontext);

	CGColorSpaceRef t_color_space;
	t_color_space = CGColorSpaceCreateDeviceRGB();
	m_video_preview_cgcontext = CGBitmapContextCreate(p_buffer -> pixels, p_buffer -> width, p_buffer -> height, 8, p_buffer -> stride, t_color_space, kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host);
	m_video_preview_cicontext = [[CIContext contextWithCGContext: m_video_preview_cgcontext options: nil] retain];
	CGColorSpaceRelease(t_color_space);
	
	NSDictionary *t_attr;
	t_attr = [NSDictionary dictionaryWithObjectsAndKeys:
					[NSNumber numberWithDouble: p_buffer -> width], (NSString *)kCVPixelBufferWidthKey,
					[NSNumber numberWithDouble: p_buffer -> height], (NSString *)kCVPixelBufferHeightKey,
					[NSNumber numberWithUnsignedInt: k32ARGBPixelFormat], (NSString *)kCVPixelBufferPixelFormatTypeKey, 
					nil];
	
	[m_video_preview_output setPixelBufferAttributes: t_attr];
	
	// Re-enable rendering.
	[m_video_preview_output setContext: m_video_preview_cicontext];
	
	return true;
}

bool MCQTXCaptureSession::GetVideoPreviewBuffer(MCCaptureImageBuffer& r_buffer)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCQTXCaptureSession::GetPreviewingState(MCCaptureState& r_status)
{
	return false;
}

bool MCQTXCaptureSession::StartPreviewing(void)
{
	return false;
}

bool MCQTXCaptureSession::StopPreviewing(void)
{
	return false;
}

bool MCQTXCaptureSession::PausePreviewing(void)
{
	return false;
}

bool MCQTXCaptureSession::ResumePreviewing(void)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

MCQTXCaptureInputDevice::MCQTXCaptureInputDevice(MCQTXCaptureInputDevice *p_next, QTCaptureDevice *p_device)
{
	m_next = p_next;
	m_device = [p_device retain];
}

MCQTXCaptureInputDevice::~MCQTXCaptureInputDevice(void)
{
	[m_device release];
}

bool MCQTXCaptureInputDevice::IsDefaultForAudio(void)
{
	return [QTCaptureDevice defaultInputDeviceWithMediaType: QTMediaTypeSound] == m_device;
}

bool MCQTXCaptureInputDevice::IsDefaultForVideo(void)
{
	return [QTCaptureDevice defaultInputDeviceWithMediaType: QTMediaTypeVideo] == m_device;
}

bool MCQTXCaptureInputDevice::IsAudio(void)
{
	return [m_device hasMediaType: QTMediaTypeSound];
}

bool MCQTXCaptureInputDevice::IsVideo(void)
{
	return [m_device hasMediaType: QTMediaTypeVideo];
}

const char *MCQTXCaptureInputDevice::GetName(void)
{
	return [[m_device localizedDisplayName] cStringUsingEncoding: NSMacOSRomanStringEncoding];
}

MCQTXCaptureInputDevice *MCQTXCaptureInputDevice::GetNext(void)
{
	return m_next;
}

QTCaptureDevice *MCQTXCaptureInputDevice::GetCaptureDevice(void)
{
	return m_device;
}

bool MCQTXCaptureInputDevice::Create(MCQTXCaptureInputDevice *p_next, QTCaptureDevice *p_device, MCQTXCaptureInputDevice*& r_device)
{
	MCQTXCaptureInputDevice *t_device;
	t_device = new MCQTXCaptureInputDevice(p_next, p_device);
	if (t_device == nil)
		return false;
	
	r_device = t_device;
	
	return true;
}

void MCQTXCaptureInputDevice::Destroy(MCQTXCaptureInputDevice *p_device)
{
	delete p_device;
}

////////////////////////////////////////////////////////////////////////////////

MCQTXCaptureAudioPreviewDevice::MCQTXCaptureAudioPreviewDevice(MCQTXCaptureAudioPreviewDevice *p_next)
{
	m_next = p_next;
}

MCQTXCaptureAudioPreviewDevice::~MCQTXCaptureAudioPreviewDevice(void)
{
}

bool MCQTXCaptureAudioPreviewDevice::IsDefault(void)
{
	return true;
}

const char *MCQTXCaptureAudioPreviewDevice::GetName(void)
{
	return "System";
}

MCQTXCaptureAudioPreviewDevice *MCQTXCaptureAudioPreviewDevice::GetNext(void)
{
	return m_next;
}

bool MCQTXCaptureAudioPreviewDevice::Create(MCQTXCaptureAudioPreviewDevice *p_next, MCQTXCaptureAudioPreviewDevice*& r_device)
{
	MCQTXCaptureAudioPreviewDevice *t_device;
	t_device = new MCQTXCaptureAudioPreviewDevice(p_next);
	if (t_device == nil)
		return false;
	
	r_device = t_device;
	
	return true;
}

void MCQTXCaptureAudioPreviewDevice::Destroy(MCQTXCaptureAudioPreviewDevice *p_device)
{
	delete p_device;
}

////////////////////////////////////////////////////////////////////////////////

bool MCQTXCaptureSessionFactory(MCCaptureSession*& r_session)
{
	r_session = new MCQTXCaptureSession;
	return r_session != nil;
}
