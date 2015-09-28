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

////////////////////////////////////////////////////////////////////////////////

extern void ThrowException(const char *p_error);

extern BOOL LockLiveCodeImage(const char *p_image_id, GWorldPtr* r_gworld);
extern void UnlockLiveCodeImage(const char *p_image_id, GWorldPtr* x_gworld);
extern void UpdateLiveCodeImage(const char *p_image_id);

////////////////////////////////////////////////////////////////////////////////

typedef enum _CaptureSessionState
{
	kCaptureSessionStateStopped,
	kCaptureSessionStateRunning,
	kCaptureSessionStatePaused,
	kCaptureSessionStatePreparing,
	kCaptureSessionStateFinishing,
	kCaptureSessionStateError,
} CaptureSessionState;

static NSString *s_capture_session_state_strings[] =
{
	@"stopped",
	@"running",
	@"paused",
	@"preparing",
	@"finishing",
	@"error"
};

@interface com_runrev_rrecapture_CaptureSession : NSObject
{
	// Mutex for thread-interactions
	NSLock *m_lock;
	
	// QT Capture Objects
	QTCaptureSession *m_session;
	QTCaptureDevice *m_audio_device;
	QTCaptureDevice *m_video_device;
	QTCaptureDeviceInput *m_audio_input;
	QTCaptureDeviceInput *m_video_input;
	QTCaptureAudioPreviewOutput *m_audio_preview_output;
	QTCaptureVideoPreviewOutput *m_video_preview_output;
	QTCompressionOptions *m_audio_codec;
	QTCompressionOptions *m_video_codec;
	QTCaptureFileOutput *m_record_output;
	NSString *m_record_filename;
	int32_t m_max_frame_rate;
	NSSize m_max_frame_size;
	
	// Preview Image Interconnect
	NSString *m_video_preview_image_id;
	GWorldPtr m_video_preview_gworld;
	CGContextRef m_video_preview_cgcontext;
	CIContext *m_video_preview_cicontext;
	BOOL m_video_preview_update_pending;
	
	// General state
	const char *m_error;
	CaptureSessionState m_preview_state;
	CaptureSessionState m_record_state;
}

- (id)init;
- (void)dealloc;

//////////

- (BOOL)startup;
- (void)shutdown;

//////////

- (NSString *)listInputsForType: (NSString*)type;

- (NSString *)inputForType: (NSString*)type;
- (BOOL)setInput: (NSString *)inputName forType: (NSString*)type;

- (int32_t)previewVolume;
- (BOOL)setPreviewVolume: (int32_t)volume;
- (NSString *)previewImage;
- (BOOL)setPreviewImage: (NSString *)image;

- (BOOL)startPreviewing;
- (void)stopPreviewing;
- (void)pausePreviewing;
- (void)resumePreviewing;

- (NSString *)previewState;

- (void)captureOutput:(QTCaptureOutput *)captureOutput didOutputVideoFrame:(CVImageBufferRef)videoFrame withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection;
- (void)updatePreview;

//////////

- (NSString *)listCodecsForType: (NSString*)type;

- (NSString *)codecForType: (NSString*)type;
- (BOOL)setCodec: (NSString *)codecName forType: (NSString*)type;

- (NSString *)recordFile;
- (BOOL)setRecordFile: (NSString *)filename;

- (BOOL)startRecording: (BOOL *)success;
- (BOOL)stopRecording: (BOOL *)success;
- (void)cancelRecording;
- (void)pauseRecording;
- (void)resumeRecording;
- (void)finishRecording;

- (int32_t)maxFrameRate;
- (void)setMaxFrameRate: (int32_t)rate;

- (NSSize)maxFrameSize;
- (void)setMaxFrameSize: (NSSize)size;

- (NSString *)recordState;

- (void)captureOutput:(QTCaptureFileOutput *)captureOutput didStartRecordingToOutputFileAtURL:(NSURL *)fileURL forConnections:(NSArray *)connections;
- (void)captureOutput:(QTCaptureFileOutput *)captureOutput didFinishRecordingToOutputFileAtURL:(NSURL *)outputFileURL forConnections:(NSArray *)connections dueToError:(NSError *)error;

//////////

- (void)removeInputForType: (NSString*)type;
- (BOOL)changeToInput: (QTCaptureDevice *)device forType: (NSString*)type;

//////////

- (const char *)error;

@end

@compatibility_alias CaptureSession com_runrev_rrecapture_CaptureSession;

////////////////////////////////////////////////////////////////////////////////

static void QTCaptureDeviceInputSetEnabledForType(QTCaptureInput *self, BOOL p_enabled, NSString *p_media_type)
{
	NSArray *t_connections;
	t_connections = [self connections];
	
	uint32_t i = 0;
	for(i = 0; i < [t_connections count]; i++)
	{
		QTCaptureConnection *t_connection;
		t_connection = (QTCaptureConnection *)[t_connections objectAtIndex: i];
		if ([t_connection mediaType] == p_media_type)
			[t_connection setEnabled: p_enabled];
	}
}

static void QTCaptureFileOutputSetCompressionForType(QTCaptureFileOutput *self, QTCompressionOptions *p_options, NSString *p_media_type)
{
	NSArray *t_connections;
	t_connections = [self connections];
	
	uint32_t i = 0;
	for(i = 0; i < [t_connections count]; i++)
	{
		QTCaptureConnection *t_connection;
		t_connection = (QTCaptureConnection *)[t_connections objectAtIndex: i];
		if ([t_connection mediaType] == p_media_type)
			[self setCompressionOptions: p_options forConnection: t_connection];
	}
}

@implementation com_runrev_rrecapture_CaptureSession

- (id)init
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_session = nil;
	m_audio_device = nil;
	m_video_device = nil;
	m_audio_input = nil;
	m_video_input = nil;
	m_audio_preview_output = nil;
	m_video_preview_output = nil;
	m_audio_codec = nil;
	m_video_codec = nil;
	m_record_output = nil;
	m_record_filename = nil;
	m_max_frame_rate = 0;
	m_max_frame_size = NSZeroSize;
	
	m_lock = nil;
	m_video_preview_image_id = nil;
	m_video_preview_gworld = nil;
	m_video_preview_cgcontext = nil;
	m_video_preview_cicontext = nil;
	m_video_preview_update_pending = NO;
	
	m_preview_state = kCaptureSessionStateStopped;
	m_record_state = kCaptureSessionStateStopped;
	
	m_error = nil;
		
	return self;
}

- (void)dealloc
{
	[super dealloc];
}

//////////

- (BOOL)startup
{
	m_lock = [[NSLock alloc] init];
	m_session = [[QTCaptureSession alloc] init];

	return YES;
}

- (void)shutdown
{
	// Stop previewing, this will disconnect the preview image and
	// such.
	[self stopPreviewing];
	
	// Clear preview settings (this frees the preview outputs and
	// associated stuff).
	[self setPreviewVolume: 0];
	[self setPreviewImage: nil];
	
	// Cancel recording, this will get rid of any pending file and such.
	[self cancelRecording];
	
	// Clear output and compression options
	[self setRecordFile: nil];
	[self setCodec: nil forType: QTMediaTypeSound];
	[self setCodec: nil forType: QTMediaTypeVideo];
	
	// Remove the inputs from the session (this frees the devices and
	// input objects).
	[self removeInputForType: QTMediaTypeSound];
	[self removeInputForType: QTMediaTypeVideo];
	
	// Now release the session
	[m_session release];
	m_session = nil;
	
	// Dispose of the lock
	[m_lock release];
	m_lock = nil;
}

//////////

- (NSString *)listInputsForType: (NSString*)p_type
{
	NSArray *t_inputs;
	t_inputs = [QTCaptureDevice inputDevices];
	 
	NSMutableArray *t_filtered_inputs;
	t_filtered_inputs = [NSMutableArray arrayWithCapacity: 0];
	
	uint32_t i;
	for(i = 0; i < [t_inputs count]; i++)
	{
		QTCaptureDevice *t_device;
		t_device = (QTCaptureDevice *)[t_inputs objectAtIndex: i];
		if ([t_device hasMediaType: p_type])
			[t_filtered_inputs addObject: t_device];
	}
	
	return [t_filtered_inputs componentsJoinedByString: @"\n"];
}

- (NSString *)inputForType: (NSString*)p_type
{
	QTCaptureDevice *t_device;
	if ([p_type isEqualTo: QTMediaTypeSound])
		t_device = m_audio_device;
	else
		t_device = m_video_device;

	if (t_device == nil)
		return @"none";
	
	return [t_device localizedDisplayName];
}

- (BOOL)setInput: (NSString *)p_input_name forType: (NSString*)p_type
{
	if (p_input_name == nil || [p_input_name isEqualTo: @""] || [p_input_name caseInsensitiveCompare: @"none"] == NSOrderedSame)
	{
		// Remove input
		[self removeInputForType: p_type];
		return YES;
	}
	
	QTCaptureDevice *t_new_device;
	t_new_device = nil;
	if ([p_input_name caseInsensitiveCompare: @"default"] == NSOrderedSame)
		t_new_device = [QTCaptureDevice defaultInputDeviceWithMediaType: p_type];
	else
	{
		NSArray *t_inputs;
		t_inputs = [QTCaptureDevice inputDevices];
		
		uint32_t i;
		for(i = 0; i < [t_inputs count]; i++)
		{
			QTCaptureDevice *t_device;
			t_device = (QTCaptureDevice *)[t_inputs objectAtIndex: i];
			if ([t_device hasMediaType: p_type] &&
				[[t_device localizedDisplayName] caseInsensitiveCompare: p_input_name] == NSOrderedSame)
			{
				t_new_device = t_device;
				break;
			}
		}
	}
	
	if (t_new_device == nil)
	{
		m_error = "input not found";
		return NO;
	}
	
	// Change input
	return [self changeToInput: t_new_device forType: p_type];
}

//////////

- (int32_t)previewVolume
{
	if (m_audio_preview_output == nil)
		return 0;
	
	return (int32_t)([m_audio_preview_output volume] * 100);
}

- (BOOL)setPreviewVolume: (int32_t)p_volume
{
	// Clamp the parameter
	if (p_volume < 0)
		p_volume = 0;
	else if (p_volume > 100)
		p_volume = 100;
	
	// If we are setting the volume to zero, then remove the output.
	if (p_volume == 0)
	{
		if (m_audio_preview_output == nil)
			return YES;
		
		if (m_preview_state != kCaptureSessionStateStopped)
			[m_session removeOutput: m_audio_preview_output];
		[m_audio_preview_output release];
		m_audio_preview_output = nil;
		
		return YES;
	}
	
	// If we don't currently have an audio preview output, make one and
	// attach it if we are running.
	if (m_audio_preview_output == nil)
	{
		m_audio_preview_output = [[QTCaptureAudioPreviewOutput alloc] init];
		
		if (m_preview_state != kCaptureSessionStateStopped)
		{
			NSError *t_error;
			if (![m_session addOutput: m_audio_preview_output error: &t_error])
			{
				[m_audio_preview_output release];
				m_audio_preview_output = nil;
				
				m_error = "could not connect to output";
				
				return NO;
			}
		}
	}
	
	// We now have a audio preview output, so we can set its volume.
	[m_audio_preview_output setVolume: p_volume / 100.0];

	return YES;
}

- (NSString *)previewImage
{
	if (m_video_preview_output == nil)
		return @"";
	
	return [m_video_preview_image_id autorelease];
}

- (BOOL)setPreviewImage: (NSString *)p_image_id
{
	// If the image is nil, or empty, then remove the output.
	if (p_image_id == nil || [p_image_id isEqualTo: @""])
	{
		if (m_video_preview_output == nil)
			return YES;
		
		// Remove the preview output from the session, if its attached.
		if (m_preview_state != kCaptureSessionStateStopped)
			[m_session removeOutput: m_video_preview_output];
		[m_video_preview_output release];
		m_video_preview_output = nil;
		
		// Remove and free the CIContext and CGContext, but make sure
		// we do so atomically.
		[m_lock lock];
		[m_video_preview_cicontext release];
		m_video_preview_cicontext = nil;
		CGContextRelease(m_video_preview_cgcontext);
		m_video_preview_cgcontext = nil;
		[m_lock unlock];
		
		// Now unhook ourselve from the LiveCode image object we are
		// bound to.
		UnlockLiveCodeImage([m_video_preview_image_id cStringUsingEncoding: NSMacOSRomanStringEncoding], &m_video_preview_gworld);
		[m_video_preview_image_id release];
		m_video_preview_image_id = nil;
		
		return YES;
	}
	
	// If the image id is the same as the current one, then we are done.
	if (m_video_preview_image_id != nil && [p_image_id isEqualTo: m_video_preview_image_id])
		return YES;
	
	// Now try to 'lock' the new image id
	GWorldPtr t_image_gworld;
	if (!LockLiveCodeImage([p_image_id cStringUsingEncoding: NSMacOSRomanStringEncoding], &t_image_gworld))
		return NO;
		
	// If we don't have a video preview output, make one; otherwise temporarily
	// unset the delegate.
	if (m_video_preview_output == nil)
	{
		m_video_preview_output = [[QTCaptureVideoPreviewOutput alloc] init];
		
		if (m_preview_state != kCaptureSessionStateStopped)
		{
			NSError *t_error;
			if (![m_session addOutput: m_video_preview_output error: &t_error])
			{
				[m_video_preview_output release];
				m_video_preview_output = nil;
				UnlockLiveCodeImage([p_image_id cStringUsingEncoding: NSMacOSRomanStringEncoding], &t_image_gworld);
				
				m_error = "could not connect to output";
				
				return NO;
			}
		}
	}
	else
		[m_video_preview_output setDelegate: nil];
		
	// We now have everything we need, so we can switch over.
	[m_lock lock];
	
	// Free any existing items
	if (m_video_preview_gworld != nil)
	{
		[m_video_preview_cicontext release];
		m_video_preview_cicontext = nil;
		CGContextRelease(m_video_preview_cgcontext);
		m_video_preview_cgcontext = nil;
		UnlockLiveCodeImage([m_video_preview_image_id cStringUsingEncoding: NSMacOSRomanStringEncoding], &m_video_preview_gworld);
		[m_video_preview_image_id release];
		m_video_preview_image_id = nil;
	}
		
	// Assign the image / gworld
	m_video_preview_image_id = [p_image_id retain];
	m_video_preview_gworld = t_image_gworld;
	
	// Fetch GWorld buffer details
	void *t_pixels;
	int32_t t_stride;
	Rect t_bounds;
	GetPixBounds(GetPortPixMap(t_image_gworld), &t_bounds);
	t_pixels = GetPixBaseAddr(GetPortPixMap(t_image_gworld));
	t_stride = GetPixRowBytes(GetPortPixMap(t_image_gworld));
	
	// Create the various contexts
	CGColorSpaceRef t_color_space;
	t_color_space = CGColorSpaceCreateDeviceRGB();
	m_video_preview_cgcontext = CGBitmapContextCreate(t_pixels, t_bounds . right - t_bounds . left, t_bounds . bottom - t_bounds . top, 8, t_stride, t_color_space, kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host);
	m_video_preview_cicontext = [[CIContext contextWithCGContext: m_video_preview_cgcontext options: nil] retain];
	CGColorSpaceRelease(t_color_space);
		
	// Configure the pixel buffer output parameters
	NSDictionary *t_attr;
	t_attr = [NSDictionary dictionaryWithObjectsAndKeys:
			  [NSNumber numberWithDouble: t_bounds . right - t_bounds . left], (NSString *)kCVPixelBufferWidthKey,
			  [NSNumber numberWithDouble: t_bounds . bottom - t_bounds . top], (NSString *)kCVPixelBufferHeightKey,
			  [NSNumber numberWithUnsignedInt: k32ARGBPixelFormat], (NSString *)kCVPixelBufferPixelFormatTypeKey, 
			  nil];
	[m_video_preview_output setPixelBufferAttributes: t_attr];

	[m_lock unlock];
	
	// Configure the delegate
	[m_video_preview_output setDelegate: self];
		
	return YES;
}

//////////

- (BOOL)startPreviewing
{
	// If we are paused, or running there is nothing to do.
	if (m_preview_state != kCaptureSessionStateStopped)
		return YES;
	
	// Try to attach the outputs
	NSError *t_error;
	if ((m_audio_preview_output != nil && ![m_session addOutput: m_audio_preview_output error: &t_error]) ||
		(m_video_preview_output != nil && ![m_session addOutput: m_video_preview_output error: &t_error]))
	{
		[m_session removeOutput: m_audio_preview_output];
		[m_session removeOutput: m_video_preview_output];
		m_error = "could not connect to output";
		return NO;
	}
	
	// Now set the session running.
	[m_session startRunning];
	
	// Change the preview state to 'running'.
	m_preview_state = kCaptureSessionStateRunning;
	
	return YES;
}

- (void)stopPreviewing
{
	// If we are stopped already, there is nothing to do.
	if (m_preview_state == kCaptureSessionStateStopped)
		return;
	
	// Stop the session - but only if we aren't recording either.
	if (m_record_state == kCaptureSessionStateStopped)
		[m_session stopRunning];
	
	// Remove the outputs
	[m_session removeOutput: m_audio_preview_output];
	[m_session removeOutput: m_video_preview_output];
	
	// Change the preview state to 'stopped'.
	m_preview_state = kCaptureSessionStateStopped;
}

- (void)pausePreviewing
{
}

- (void)resumePreviewing
{
}

- (NSString *)previewState
{
	return s_capture_session_state_strings[m_preview_state];
}

- (void)captureOutput:(QTCaptureOutput *)captureOutput didOutputVideoFrame:(CVImageBufferRef)videoFrame withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection
{	
	CIImage *t_image;
	t_image = [CIImage imageWithCVImageBuffer: videoFrame];
	
	[m_lock lock];
	if (m_video_preview_cicontext != nil)
	{
		[m_video_preview_cicontext drawImage: t_image atPoint: CGPointMake(0.0, 0.0) fromRect: [t_image extent]];
		if (!m_video_preview_update_pending)
		{
			m_video_preview_update_pending = YES;
			[self performSelectorOnMainThread: @selector(updatePreview) withObject: nil waitUntilDone: NO];
		}
	}
	[m_lock unlock];
}

- (void)updatePreview
{
	m_video_preview_update_pending = NO;
	if (m_video_preview_image_id != nil)
		UpdateLiveCodeImage([m_video_preview_image_id cStringUsingEncoding: NSMacOSRomanStringEncoding]);
}

////////////////////////////////////////////////////////////////////////////////

- (NSString *)listCodecsForType: (NSString*)p_type
{
	NSArray *t_ids;
	t_ids = [QTCompressionOptions compressionOptionsIdentifiersForMediaType: p_type];
	
	NSMutableArray *t_options;
	t_options = [NSMutableArray arrayWithCapacity: 0];
	
	uint32_t i;
	for(i = 0; i < [t_ids count]; i++)
		[t_options addObject: [[QTCompressionOptions compressionOptionsWithIdentifier: (NSString *)[t_ids objectAtIndex: i]] localizedDisplayName]];
	
	return [t_options componentsJoinedByString: @"\n"];
}

- (NSString *)codecForType: (NSString*)p_type
{
	QTCompressionOptions *t_this_options;
	if ([p_type isEqualTo: QTMediaTypeSound])
		t_this_options = m_audio_codec;
	else
		t_this_options = m_video_codec;
	
	return t_this_options == nil ? @"" : [t_this_options localizedDisplayName];
}

- (BOOL)setCodec: (NSString *)p_codec forType: (NSString*)p_type
{
	if (m_record_state != kCaptureSessionStateStopped)
	{
		m_error = "recording in progress";
		return NO;
	}
	
	QTCompressionOptions *t_options;
	if (p_codec == nil || [p_codec isEqualTo: @""] || [p_codec caseInsensitiveCompare: @"none"] == NSOrderedSame)
		t_options = nil;
	else
	{
		NSArray *t_ids;
		t_ids = [QTCompressionOptions compressionOptionsIdentifiersForMediaType: p_type];
		
		uint32_t i;
		t_options = nil;
		for(i = 0; i < [t_ids count]; i++)
		{
			QTCompressionOptions *t_current_options;
			t_current_options = [QTCompressionOptions compressionOptionsWithIdentifier: (NSString *)[t_ids objectAtIndex: i]];
			if ([[t_current_options localizedDisplayName] isEqualTo: p_codec])
			{
				t_options = t_current_options;
				break;
			}
		}
		
		if (t_options == nil)
		{
			m_error = "invalid codec for media type";
			return NO;
		}
	}
	
	QTCompressionOptions **t_this_options;
	if ([p_type isEqualTo: QTMediaTypeSound])
		t_this_options = &m_audio_codec;
	else
		t_this_options = &m_video_codec;
	
	[*t_this_options release];
	*t_this_options = [t_options retain];

	return YES;
}

- (NSString *)recordFile
{
	return m_record_filename == nil ? @"" : [m_record_filename autorelease];
}

- (BOOL)setRecordFile: (NSString *)p_filename
{
	if (m_record_state != kCaptureSessionStateStopped)
	{
		m_error = "recording in progress";
		return NO;
	}
	
	if (p_filename == nil || [p_filename isEqualTo: @""])
	{
		[m_record_filename release];
		m_record_filename = nil;
		return YES;
	}
	
	[m_record_filename release];
	m_record_filename = [p_filename retain];
	
	return YES;
}

- (BOOL)startRecording: (BOOL *)r_success
{
	if (m_record_state != kCaptureSessionStateStopped)
		return YES;
	
	// Make our record output and add it (this is easier for now than 'threading'
	// the option setting - it seems that compression changes only take effect at
	// the next 'wait').
	m_record_output = [[QTCaptureMovieFileOutput alloc] init];
	[m_record_output setDelegate: self];
	
	NSError *t_error;
	if (![m_session addOutput: m_record_output error: &t_error])
	{
		[m_record_output release];
		m_record_output = nil;
		
		m_error = "could not connect to output";
		
		return NO;
	}
	
	// Configure the output
	QTCaptureFileOutputSetCompressionForType(m_record_output, m_audio_codec, QTMediaTypeSound);
	QTCaptureFileOutputSetCompressionForType(m_record_output, m_video_codec, QTMediaTypeVideo);
	if ([m_record_output respondsToSelector: @selector(setMaximumVideoSize:)])
		[m_record_output setMaximumVideoSize: m_max_frame_size . width == 0 || m_max_frame_size . height == 0 ? NSZeroSize : m_max_frame_size];
	if ([m_record_output respondsToSelector: @selector(setMinimumVideoFrameInterval:)])
		[m_record_output setMinimumVideoFrameInterval: m_max_frame_rate == 0 ? 0 : 1.0 / m_max_frame_rate];
	
	// Start the recording
	[m_record_output recordToOutputFileURL: [NSURL fileURLWithPath: m_record_filename]];
	
	[m_session startRunning];
	
	// Put ourselves in the 'preparing' state and wait until the file has
	// started writing.
	m_record_state = kCaptureSessionStatePreparing;
	for(;;)
	{
		CaptureSessionState t_state;
		[m_lock lock];
		t_state = m_record_state;
		[m_lock unlock];
		
		if (t_state != kCaptureSessionStatePreparing)
			break;
		
		ExecuteLiveCodeScript("wait until revCaptureRecordState() is not \"preparing\"");
	}
	
	if (m_record_state == kCaptureSessionStateError)
	{
		[self finishRecording];
		*r_success = NO;
	}
	else
	{
		m_record_state = kCaptureSessionStateRunning;
		*r_success = YES;
	}
	
	return YES;
}

- (BOOL)stopRecording: (BOOL *)r_success
{
	if (m_record_state == kCaptureSessionStateStopped)
		return NO;
	
	[m_record_output recordToOutputFileURL: nil];
	
	// Put ourselves in the 'finishing' state and wait until the file has
	// finished writing.
	m_record_state = kCaptureSessionStateFinishing;
	for(;;)
	{
		CaptureSessionState t_state;
		[m_lock lock];
		t_state = m_record_state;
		[m_lock unlock];
		
		if (t_state != kCaptureSessionStateFinishing)
			break;
		
		ExecuteLiveCodeScript("wait until revCaptureRecordState() is not \"finishing\"");
	}
	
	if (m_record_state == kCaptureSessionStateError)
	{
		[self finishRecording];
		*r_success = NO;
	}
	else
	{	
		[self finishRecording];
		*r_success = YES;
	}
		
	return YES;
}

- (void)finishRecording
{
	// Stop the session - but only if we aren't recording either.
	if (m_preview_state == kCaptureSessionStateStopped)
		[m_session stopRunning];
	
	[m_session removeOutput: m_record_output];
	[m_record_output setDelegate: nil];
	[m_record_output release];
	m_record_output = nil;
	
	m_record_state = kCaptureSessionStateStopped;
}

- (void)cancelRecording
{
	if (m_record_state == kCaptureSessionStateStopped)
		return;
	
	BOOL t_success;
	[self stopRecording: &t_success];
	
	// Delete the output file here.
}

- (void)pauseRecording
{
}

- (void)resumeRecording
{
}

//////////

- (int32_t)maxFrameRate
{
	return m_max_frame_rate;
}

- (void)setMaxFrameRate: (int32_t)p_rate
{
	m_max_frame_rate = p_rate;
}

- (NSSize)maxFrameSize
{
	return m_max_frame_size;
}

- (void)setMaxFrameSize: (NSSize)p_size
{
	m_max_frame_size = p_size;
}

//////////

- (NSString *)recordState
{
	// While we are only 'reading' from m_record_state, we use the lock as a
	// memory barrier.
	CaptureSessionState t_state;
	
	[m_lock lock];
	t_state = m_record_state;
	[m_lock unlock];
	
	return s_capture_session_state_strings[t_state];
}

- (void)captureOutput:(QTCaptureFileOutput *)p_output didStartRecordingToOutputFileAtURL:(NSURL *)p_url forConnections:(NSArray *)p_connections
{
	//NSLog(@"didStart: %@\n", [p_url absoluteString]);
	
	// Use a lock as a memory barrier and set our state to running.
	[m_lock lock];
	m_record_state = kCaptureSessionStateRunning;
	[m_lock unlock];
}

- (void)captureOutput:(QTCaptureFileOutput *)p_output didFinishRecordingToOutputFileAtURL:(NSURL *)p_url forConnections:(NSArray *)p_connections dueToError:(NSError *)p_error
{
	//NSLog(@"didFinish: %@\n", [p_url absoluteString]);
	
	// Use a lock as a memory barrier and set our state to stopped.
	[m_lock lock];
	if (p_error != nil)
		m_record_state = kCaptureSessionStateError;
	else
		m_record_state = kCaptureSessionStateStopped;
	[m_lock unlock];
}

////////////////////////////////////////////////////////////////////////////////

- (void)removeInputForType: (NSString*)p_type
{
	// Fetch references to the input/device for this media type.
	QTCaptureDevice **t_this_device;
	QTCaptureDeviceInput **t_this_input;
	if ([p_type isEqualToString: QTMediaTypeSound])
		t_this_device = &m_audio_device, t_this_input = &m_audio_input;
	else
		t_this_device = &m_video_device, t_this_input = &m_video_input;
	
	// If there is no device for the given type, there's nothing to do.
	if (*t_this_device == nil)
		return;
	
	// If both devices are the same, then we just disable 'this' media
	// type on the device. Otherwise, we remove the input and close the
	// device.
	if (m_audio_device == m_video_device)
		QTCaptureDeviceInputSetEnabledForType(*t_this_input, NO, p_type);
	else
	{
		[m_session removeInput: *t_this_input];
		[*t_this_device close];
	}
	
	// Either way, make sure the references to the input/device are freed
	// for 'this' media type.
	[*t_this_input release];
	*t_this_input = nil;
	[*t_this_device release];
	*t_this_device = nil;
}

- (BOOL)changeToInput: (QTCaptureDevice *)p_new_device forType: (NSString*)p_type
{
	// First get references to the (device, input) pairs for this media type
	// and the 'other' media type.
	QTCaptureDevice **t_this_device, **t_other_device;
	QTCaptureDeviceInput **t_this_input, **t_other_input;
	if ([p_type isEqualToString: QTMediaTypeSound])
	{
		t_this_device = &m_audio_device;
		t_this_input = &m_audio_input;
		t_other_device = &m_video_device;
		t_other_input = &m_video_input;
	}
	else
	{
		t_this_device = &m_video_device;
		t_this_input = &m_video_input;
		t_other_device = &m_audio_device;
		t_other_input = &m_audio_input;
	}
	
	// If the new device is the same as the old, we do nothing.
	if (p_new_device == *t_this_device)
		return YES;
		
	// Regardless of what happens next, we remove the existing input.
	[self removeInputForType: p_type];
	
	// If the new device is the same as the other, we remove the current one,
	// and enable the appropriate connection.
	if (p_new_device == *t_other_device)
	{
		*t_this_device = [*t_other_device retain];
		*t_this_input = [*t_other_input retain];
		QTCaptureDeviceInputSetEnabledForType(*t_this_input, YES, p_type);
		return YES;
	}
	
	// Now try to open the new device,
	NSError *t_error;
	t_error = nil;
	if (![p_new_device open: &t_error])
	{
		m_error = "could not open input";
		return NO;
	}
	
	// Finally, create a new input and try to add it to the session.
	QTCaptureDeviceInput *t_new_input;
	t_new_input = [[QTCaptureDeviceInput alloc] initWithDevice: p_new_device];
	if (![m_session addInput: t_new_input error: &t_error])
	{
		[t_new_input release];
		[p_new_device close];
		m_error = "could not connect to input";
		return NO;
	}
	
	*t_this_device = [p_new_device retain];
	*t_this_input = t_new_input;
	
	return YES;
}

- (const char *)error
{
	return m_error;
}

@end

////////////////////////////////////////////////////////////////////////////////

CaptureSession *g_session = nil;

static BOOL CaptureSessionExists(void)
{
	if (g_session != nil)
		return YES;
		
	ThrowException("no session");
	
	return NO;
}

static void CaptureSessionThrow(void)
{
	ThrowException([g_session error]);
}

////////////////////////////////////////////////////////////////////////////////

void rreCaptureBeginSession(void)
{
	g_session = [[CaptureSession alloc] init];
	if (![g_session startup])
		CaptureSessionThrow(); // Throw exception
}

void rreCaptureEndSession(void)
{
	[g_session shutdown];
	[g_session release];
	g_session = nil;
}

//////////

NSString *rreCaptureListAudioInputs(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	return [g_session listInputsForType: QTMediaTypeSound];
}

NSString *rreCaptureListVideoInputs(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	return [g_session listInputsForType: QTMediaTypeVideo];
}

//////////

NSString *rreCaptureGetAudioInput(void)
{
	if (!CaptureSessionExists())
		return nil;

	return [g_session inputForType: QTMediaTypeSound];
}

void rreCaptureSetAudioInput(NSString *p_input_name)
{
	if (!CaptureSessionExists())
		return;
		
	if (![g_session setInput: p_input_name forType: QTMediaTypeSound])
		CaptureSessionThrow();
}

NSString *rreCaptureGetVideoInput(void)
{
	if (!CaptureSessionExists())
		return nil;

	return [g_session inputForType: QTMediaTypeVideo];
}

void rreCaptureSetVideoInput(NSString *p_input_name)
{
	if (!CaptureSessionExists())
		return;
		
	if (![g_session setInput: p_input_name forType: QTMediaTypeVideo])
		CaptureSessionThrow();
}

//////////

uint32_t rreCaptureGetPreviewVolume(void)
{
	if (!CaptureSessionExists())
		return 0;

	return [g_session previewVolume];
}

void rreCaptureSetPreviewVolume(int32_t p_volume)
{
	if (!CaptureSessionExists())
		return;
		
	if (![g_session setPreviewVolume: p_volume])
		CaptureSessionThrow();
}

NSString *rreCaptureGetPreviewImage(void)
{
	if (!CaptureSessionExists())
		return nil;
		
	return [g_session previewImage];
}

void rreCaptureSetPreviewImage(NSString *p_image_id)
{
	if (!CaptureSessionExists())
		return;
		
	if (![g_session setPreviewImage: p_image_id])
		CaptureSessionThrow();
}

void rreCaptureStartPreviewing(void)
{
	if (!CaptureSessionExists())
		return;
		
	if (![g_session startPreviewing])
		CaptureSessionThrow();
}

void rreCaptureStopPreviewing(void)
{
	if (!CaptureSessionExists())
		return;
		
	[g_session stopPreviewing];
}

void rreCapturePausePreviewing(void)
{
	if (!CaptureSessionExists())
		return;

	[g_session pausePreviewing];
}

void rreCaptureResumePreviewing(void)
{
	if (!CaptureSessionExists())
		return;

	[g_session resumePreviewing];
}

NSString *rreCapturePreviewState(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	return [g_session previewState];
}

////////////////////////////////////////////////////////////////////////////////

NSString *rreCaptureListAudioCodecs(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	return [g_session listCodecsForType: QTMediaTypeSound];
}

NSString *rreCaptureListVideoCodecs(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	return [g_session listCodecsForType: QTMediaTypeVideo];
}

//////////

NSString *rreCaptureGetAudioCodec(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	return [g_session codecForType: QTMediaTypeSound];
}

void rreCaptureSetAudioCodec(NSString *p_codec_name)
{
	if (!CaptureSessionExists())
		return;
	
	if (![g_session setCodec: p_codec_name forType: QTMediaTypeSound])
		CaptureSessionThrow();
}

NSString *rreCaptureGetVideoCodec(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	return [g_session codecForType: QTMediaTypeVideo];
}

void rreCaptureSetVideoCodec(NSString *p_codec_name)
{
	if (!CaptureSessionExists())
		return;
	
	if (![g_session setCodec: p_codec_name forType: QTMediaTypeVideo])
		CaptureSessionThrow();
}

/////////

NSString *rreCaptureGetRecordOutput(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	return [g_session recordFile];
}

void rreCaptureSetRecordOutput(NSString *p_image_id)
{
	if (!CaptureSessionExists())
		return;
	
	if (![g_session setRecordFile: p_image_id])
		CaptureSessionThrow();
}

NSString *rreCaptureStartRecording(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	BOOL t_success;
	if (![g_session startRecording: &t_success])
	{
		CaptureSessionThrow();
		return nil;
	}
	
	return t_success ? @"" : @"recording failed";
}

NSString *rreCaptureStopRecording(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	BOOL t_success;
	[g_session stopRecording: &t_success];
	
	return t_success ? @"" : @"recording failed";
}

void rreCaptureCancelRecording(void)
{
	if (!CaptureSessionExists())
		return;
	
	[g_session cancelRecording];
}

void rreCapturePauseRecording(void)
{
	if (!CaptureSessionExists())
		return;
	
	[g_session pauseRecording];
}

void rreCaptureResumeRecording(void)
{
	if (!CaptureSessionExists())
		return;
	
	[g_session resumeRecording];
}

int32_t rreCaptureGetRecordFrameRate(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	return [g_session maxFrameRate];
}

void rreCaptureSetRecordFrameRate(int32_t p_rate)
{
	if (!CaptureSessionExists())
		return;
	
	[g_session setMaxFrameRate: p_rate];
}

NSString *rreCaptureGetRecordFrameSize(void)
{
	if (!CaptureSessionExists())
		return nil;
	
	NSSize t_size;
	t_size = [g_session maxFrameSize];
	
	return [NSString stringWithFormat: @"%d,%d", (int32_t)t_size . width, (int32_t)t_size . height];
}

void rreCaptureSetRecordFrameSize(int32_t p_width, int32_t p_height)
{
	if (!CaptureSessionExists())
		return;
	
	[g_session setMaxFrameSize: NSMakeSize(p_width, p_height)];
}


NSString *rreCaptureRecordState(void)
{
	if (!CaptureSessionExists())
        return nil;
	
	return [g_session recordState];
}

////////////////////////////////////////////////////////////////////////////////
