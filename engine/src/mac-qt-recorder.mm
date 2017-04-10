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

#include <algorithm>

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"

#include "graphics.h"
#include "stack.h"

#include "player.h"
#include "util.h"
#include "osspec.h"

#include "osxprefix.h"

#include "platform.h"
#include "platform-internal.h"

#include "variable.h"

#include <Cocoa/Cocoa.h>
#if defined(FEATURE_QUICKTIME)
#   include <QTKit/QTKit.h>
#   include <QuickTime/QuickTime.h>
#endif
#include <CoreAudioKit/CoreAudioKit.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>

#import <sys/stat.h>

////////////////////////////////////////////////////////////////////////////////

#ifdef FEATURE_QUICKTIME

class MCQTSoundRecorder;

@interface com_runrev_livecode_MCQTSoundRecorderObserver: NSObject
{
    MCQTSoundRecorder *m_recorder;
    BOOL m_stop_requested;
}

- (id)initWithRecorder: (MCQTSoundRecorder *)recorder;
- (void)startTimer;
- (void)idleTimer:(NSTimer*)timer;
- (void)stopTimer;

@end

class MCQTSoundRecorder: public MCPlatformSoundRecorder
{
public:
    MCQTSoundRecorder(void);
    virtual ~MCQTSoundRecorder(void);
    
    virtual void BeginDialog(void);
    virtual MCPlatformDialogResult EndDialog(void);
    
    virtual bool StartRecording(MCStringRef filename);
    virtual void StopRecording(void);
    virtual void PauseRecording(void);
    virtual void ResumeRecording(void);
    
    virtual double GetLoudness(void);
    
    virtual bool ListInputs(MCPlatformSoundRecorderListInputsCallback callback, void *context);
    virtual bool ListCompressors(MCPlatformSoundRecorderListCompressorsCallback callback, void *context);
    virtual bool ListFormats(MCPlatformSoundRecorderListFormatsCallback callback, void *context);
    
    void Idle(void);
    
    // Set the sound recorder's MCPlatformSoundRecorderConfiguration
    void Configure(const AudioStreamBasicDescription &p_description, CFArrayRef *p_codec_settings, void *p_magic_cookie, uint32_t p_magic_cookie_size);
    
    // Get an AudioStreamBasicDescription from the sound recorder's configuration
    void GetASBD(AudioStreamBasicDescription &r_description);
    
    // Initialize the configuration of the sound recorder according to the MCrecord globals
    void InitializeConfiguration(void);
    
    // Ensure we have a valid channel to set properties on
    bool EnsureInitialized(void);
    
protected:
    MCPlatformDialogResult m_dialog_result;
    SeqGrabComponent m_seq_grab;
    SGChannel m_channel;

    com_runrev_livecode_MCQTSoundRecorderObserver *m_observer;

    bool m_has_magic_cookie;
    
private:
    MCStringRef m_temp_file;
};

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCQTSoundRecorderObserver

- (id)initWithRecorder:(MCQTSoundRecorder *)recorder
{
    self = [super init];
    if (self == nil)
        return nil;
    
    m_recorder = recorder;
    m_stop_requested = NO;
    
    return self;
}

- (void)startTimer
{
    const UInt32 kDefaultAudioIdlesPerSecond = 10;
    
	NSTimer * t = [[NSTimer alloc] initWithFireDate:
                   [NSDate dateWithTimeIntervalSinceNow:1./(double)kDefaultAudioIdlesPerSecond]
                                           interval:1./(double)kDefaultAudioIdlesPerSecond
                                             target:self
                                           selector:@selector(idleTimer:)
                                           userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer:t forMode:NSDefaultRunLoopMode];
	[[NSRunLoop currentRunLoop] addTimer:t forMode:NSModalPanelRunLoopMode];
	[[NSRunLoop currentRunLoop] addTimer:t forMode:NSEventTrackingRunLoopMode];
    m_stop_requested = NO;
}

- (void)idleTimer:(NSTimer*)timer
{
if (m_stop_requested == YES)
    {
        [timer invalidate];
		[timer release];
    }
    else {
        m_recorder -> Idle();
    }
}

- (void)stopTimer
{
    m_stop_requested = YES;
}

@end

////////////////////////////////////////////////////////////////////////////////

// Utility functions
static SampleDescriptionHandle scanSoundTracks(Movie tmovie)
{
	short trackCount, index;
	SampleDescriptionHandle aDesc = NULL;
	trackCount = (short)GetMovieTrackCount(tmovie);
	for (index = 1 ; index <= trackCount ; index++)
	{
		OSType aTrackType;
		Track aTrack = NULL;
		Media aMedia = NULL;
		aTrack = GetMovieIndTrack(tmovie, index);
		aMedia = GetTrackMedia(aTrack);
		GetMediaHandlerDescription(aMedia, &aTrackType, 0, 0);
		if (aTrackType == SoundMediaType)
		{
			aDesc = (SampleDescriptionHandle)NewHandle(sizeof(SoundDescription));
			GetMediaSampleDescription(aMedia, 1, aDesc);
			if (GetMoviesError() != noErr)
			{
				DisposeHandle((Handle)aDesc);
				aDesc = NULL;
				continue;
			}
		}
	}
	return aDesc;
}

static bool path_to_dataref(MCStringRef p_path, DataReferenceRecord& r_rec)
{
	bool t_success = true;
	CFStringRef t_cf_path = NULL;
	t_cf_path = CFStringCreateWithCString(NULL, MCStringGetCString(p_path), kCFStringEncodingWindowsLatin1);
	t_success = (t_cf_path != NULL);
	if (t_success)
	{
		OSErr t_error;
		t_error = QTNewDataReferenceFromFullPathCFString(t_cf_path, kQTNativeDefaultPathStyle, 0, &r_rec . dataRef, &r_rec . dataRefType);
		t_success = noErr == t_error;
	}
	CFRelease(t_cf_path);
	return t_success;
}

static void exportToSoundFile(MCStringRef sourcefile, MCStringRef destfile)
{
	bool t_success = true;
	SoundDescriptionHandle myDesc = NULL;
	ComponentResult result = 0;
	Movie tmovie = nil;
	
    MCAutoStringRef t_src_resolved;
	MCAutoStringRef t_dst_resolved;
    
    // AL-2015-01-05: [[ Bug 14302 ]] Assign resolved path strings correctly
    t_success = MCS_resolvepath(sourcefile, &t_src_resolved)
    && MCS_resolvepath(destfile, &t_dst_resolved);
    
	t_success = (*t_src_resolved != NULL && *t_dst_resolved != NULL);
	
	DataReferenceRecord t_src_rec, t_dst_rec;
	t_src_rec.dataRef = NULL;
	t_dst_rec.dataRef = NULL;
	
	if (t_success)
	{
		t_success = path_to_dataref(*t_src_resolved, t_src_rec)
        && path_to_dataref(*t_dst_resolved, t_dst_rec);
	}
	
	Boolean isActive = true;
	QTVisualContextRef aVisualContext = NULL;
	QTNewMoviePropertyElement aMovieProperties[] = {
		{kQTPropertyClass_DataLocation, kQTDataLocationPropertyID_DataReference, sizeof(t_src_rec), &t_src_rec, 0},
		{kQTPropertyClass_NewMovieProperty, kQTNewMoviePropertyID_Active, sizeof(isActive), &isActive, 0},
		{kQTPropertyClass_Context, kQTContextPropertyID_VisualContext, sizeof(aVisualContext), &aVisualContext, 0},
	};
	
	if (t_success)
		t_success = noErr == NewMovieFromProperties(3, aMovieProperties, 0, NULL, &tmovie);
	
	if (t_success)
		myDesc = (SoundDescriptionHandle)scanSoundTracks(tmovie);
	
	if (myDesc)
	{
		//open movie export component
		MovieExportComponent exporter;
		Component c;
		ComponentDescription cd;
		cd.componentType = MovieExportType;
        cd.componentSubType = MCrecordformat;

        cd.componentManufacturer = SoundMediaType;
		cd.componentFlags = canMovieExportFiles;
		cd.componentFlagsMask = canMovieExportFiles;
		c = FindNextComponent(nil, &cd);
		(**myDesc).numChannels = MCrecordchannels;
		(**myDesc).sampleSize = MCrecordsamplesize;
		(**myDesc).sampleRate = (uint32_t)(MCrecordrate * 1000 * 65536);
		exporter = nil;
		exporter = OpenComponent(c);
		result = MovieExportSetSampleDescription(exporter, (SampleDescriptionHandle)myDesc,
												 SoundMediaType);
		errno = ConvertMovieToDataRef(tmovie, 0, t_dst_rec . dataRef, t_dst_rec . dataRefType, cd.componentSubType,
									  0, 0, exporter);
		// try showUserSettingsDialog | movieToFileOnlyExport | movieFileSpecValid
		DisposeHandle((Handle) myDesc);
		if (exporter)
			CloseComponent(exporter);
		if (errno != noErr)
		{
			MCAutoStringRef t_error;
			/* UNCHECKED */ MCStringFormat(&t_error, "error %d exporting recording", errno);
			MCresult->setvalueref(*t_error);
		}
	}
	
	if (t_src_rec.dataRef != NULL)
		DisposeHandle(t_src_rec.dataRef);
	if (t_dst_rec.dataRef != NULL)
		DisposeHandle(t_dst_rec.dataRef);
	
	DisposeMovie(tmovie);
}

////////////////////////////////////////////////////////////////////////////////

MCQTSoundRecorder::MCQTSoundRecorder(void)
{
    m_dialog_result = kMCPlatformDialogResultContinue;
    m_seq_grab = nil;
    m_channel = nil;
    m_temp_file = nil;
    m_filename = nil;
    m_has_magic_cookie = false;
    
    m_observer = [[com_runrev_livecode_MCQTSoundRecorderObserver alloc] initWithRecorder: this];
    
    OpenADefaultComponent(SeqGrabComponentType, 0, &m_seq_grab);
    SGInitialize(m_seq_grab);
}

MCQTSoundRecorder::~MCQTSoundRecorder(void)
{
    if (m_channel != nil)
    {
        SGDisposeChannel(m_seq_grab, m_channel);
        m_channel = nil;
    }
    
    if (m_seq_grab != nil)
    {
        CloseComponent(m_seq_grab);
        m_seq_grab = nil;
    }
    
    if (m_configuration . extra_info_size)
        MCMemoryDeallocate(m_configuration . extra_info);
}

////////////////////////////////////////////////////////

void MCQTSoundRecorder::InitializeConfiguration()
{
    m_configuration . sample_rate = MCrecordrate;
    m_configuration . sample_bit_count = MCrecordsamplesize;
    m_configuration . channel_count = MCrecordchannels;
    
    uint32_t t_compression;
    t_compression = kAudioFormatLinearPCM;
    if (!MCCStringEqual(MCrecordcompression, "raw "))
    {
        memcpy(&t_compression, MCrecordcompression, 4);
        t_compression = MCSwapInt32HostToNetwork(t_compression);
    }
    
    if (m_configuration . compression_type != t_compression)
    {
        // If we have changed format, dispose of the previous codec-specific settings
        m_configuration . compression_type = t_compression;
    
        if (m_configuration . extra_info_size)
            MCMemoryDeallocate(m_configuration . extra_info);
        
        m_configuration . extra_info = nil;
        m_configuration . extra_info_size = 0;
    }
    
    uint32_t t_input;
    t_input = 0;
    
    if (!MCCStringEqual(MCrecordinput, "dflt"))
    {
        memcpy(&t_input, MCrecordinput, 4);
        t_input = MCSwapInt32HostToNetwork(t_input);
    }
    else
    {
        NSArray *t_input_list = nil;
        QTGetComponentProperty(m_channel, kQTPropertyClass_SGAudioRecordDevice, kQTSGAudioPropertyID_InputListWithAttributes, sizeof(t_input_list), &t_input_list, NULL);
        
        if (t_input_list)
        {
            NSDictionary *t_dict = [t_input_list objectAtIndex:0];
                
            t_input = [(NSNumber*)[t_dict objectForKey:(id)kQTAudioDeviceAttribute_DeviceInputID] unsignedIntValue];
        }
    }
    m_configuration . input = t_input;
}

bool MCQTSoundRecorder::EnsureInitialized()
{
    if (m_channel != nil)
        return true;
    
    bool t_success;
    t_success = true;

    if (t_success)
        t_success = SGNewChannel(m_seq_grab, SGAudioMediaType, &m_channel) == noErr;
    
    if (t_success)
        t_success = SGSetChannelUsage(m_channel, seqGrabRecord) == noErr;
    
    if (!t_success)
    {
        SGDisposeChannel(m_seq_grab, m_channel);
        m_channel = nil;
        
        CloseComponent(m_seq_grab);
        m_seq_grab = nil;
    }
    
    return t_success;
}

struct MCQTSoundRecorderAvailableCompressionIdsState
{
    UInt32 *list;
    uindex_t count;
};

static bool available_compression_ids(void *context, unsigned int p_id, const char *p_label)
{
    MCQTSoundRecorderAvailableCompressionIdsState *t_state = static_cast<MCQTSoundRecorderAvailableCompressionIdsState *>(context);
    
    if (MCMemoryResizeArray(t_state -> count + 1, t_state -> list, t_state -> count))
    {
        t_state -> list[t_state -> count - 1] = p_id;
        return true;
    }
    
    return false;
}

void MCQTSoundRecorder::Configure(const AudioStreamBasicDescription &p_description, CFArrayRef *p_codec_settings, void *p_magic_cookie, uint32_t p_magic_cookie_size)
{
    m_configuration . sample_rate = p_description . mSampleRate / 1000;
    m_configuration . sample_bit_count = p_description . mBitsPerChannel;
    m_configuration . channel_count = p_description . mChannelsPerFrame;
    m_configuration . compression_type = p_description . mFormatID;
    
    if (p_codec_settings)
        m_configuration . extra_info = (uint8_t *)p_codec_settings;
    else if (p_magic_cookie_size)
    {
        m_configuration . extra_info = (uint8_t *)p_magic_cookie;
        m_has_magic_cookie = true;
    }
}

void MCQTSoundRecorder::GetASBD(AudioStreamBasicDescription &r_description)
{
    memset(&r_description, 0 , sizeof(r_description));
    r_description . mFormatID = m_configuration . compression_type;
    r_description . mSampleRate = m_configuration . sample_rate * 1000;
    r_description . mChannelsPerFrame = m_configuration . channel_count;
    switch (m_configuration . compression_type)
    {
        case kAudioFormatLinearPCM:
            r_description . mFormatFlags = 12;
            // fall through
        case kAudioFormatALaw:
        case kAudioFormatULaw:
        {
            uint32_t bytes = m_configuration . sample_bit_count * m_configuration . channel_count / 8;
            r_description . mBytesPerPacket = bytes;
            r_description . mFramesPerPacket = 1;
            r_description . mBytesPerFrame = bytes;
            r_description . mBitsPerChannel = m_configuration . sample_bit_count;
        }
            break;
        case kAudioFormatAppleLossless:
            r_description . mFramesPerPacket = 1 << 12;
            break;
        case kAudioFormatMPEG4AAC_HE:
        case kAudioFormatMPEG4AAC_HE_V2:
            if (m_configuration . sample_rate < 32)
                r_description . mSampleRate = 32000;
            r_description . mFramesPerPacket = 1 << 11;
            break;
        case kAudioFormatMPEG4AAC:
            r_description . mFramesPerPacket = 1 << 10;
            break;
        case kAudioFormatQUALCOMM:
        case kAudioFormatiLBC:
            // QUALCOMM & iLBC basic description must be mono with 8000 bit rate
            r_description . mChannelsPerFrame = 1;
            r_description . mSampleRate = 8000;
            r_description . mFramesPerPacket = 1 << 9;
            break;
        case kAudioFormatMPEG4AAC_LD:
        case kAudioFormatMPEG4AAC_ELD:
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_6
        case kAudioFormatMPEG4AAC_ELD_SBR:
        case kAudioFormatMPEG4AAC_ELD_V2:
#endif
            if (m_configuration . sample_rate < 16)
                r_description . mSampleRate = 16000;
            r_description . mFramesPerPacket = 1 << 9;
            break;
        default:
            r_description . mFramesPerPacket = 1 << 9;
            break;
    }
}

void MCQTSoundRecorder::BeginDialog()
{
    // use StdAudio dialog to let user set an output format
    ComponentInstance ci;
    OpenADefaultComponent(StandardCompressionType, StandardCompressionSubTypeAudio, &ci);
    
	if (ci == NULL)
	{
		MCresult->sets("can't open dialog");
		return;
	}
    
    AudioStreamBasicDescription t_description;
    InitializeConfiguration();
    GetASBD(t_description);
    
    OSErr t_error;
    t_error = noErr;
    
    MCQTSoundRecorderAvailableCompressionIdsState t_state;
    t_state . list = nil;
    t_state . count = 0;
    
    if (ListCompressors(available_compression_ids, &t_state))
        QTSetComponentProperty(ci, kQTPropertyClass_SCAudio,kQTSCAudioPropertyID_ClientRestrictedCompressionFormatList,sizeof(UInt32) * t_state . count, &t_state . list[0]);
    
    while (t_state . count--)
    {
        if (t_state . list[t_state . count] == t_description . mFormatID)
        {
            // Set defaults of dialog if possible
            QTSetComponentProperty(ci, kQTPropertyClass_SCAudio,kQTSCAudioPropertyID_BasicDescription,sizeof(t_description), &t_description);
            break;
        }
    }
    
    MCMemoryDeleteArray(t_state . list);
    
    AudioChannelLayoutTag t_layout_tags[] =
    {
        kAudioChannelLayoutTag_UseChannelDescriptions,
        kAudioChannelLayoutTag_Mono,
        kAudioChannelLayoutTag_Stereo,
    };

    QTSetComponentProperty(ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_ClientRestrictedChannelLayoutTagList,sizeof(t_layout_tags), t_layout_tags);
    
    if (t_error == noErr)
        t_error = SCRequestImageSettings(ci);
    
	if (t_error == noErr)
        t_error = QTGetComponentProperty(ci, kQTPropertyClass_SCAudio,kQTSCAudioPropertyID_BasicDescription,sizeof(t_description), &t_description, NULL);
    
    if (t_error == noErr)
	{
        CFArrayRef t_codec_settings = nil;
        void *t_magic_cookie = nil;
        UInt32 t_magic_cookie_size = 0;
        
        QTGetComponentProperty(ci, kQTPropertyClass_SCAudio,
                               kQTSCAudioPropertyID_CodecSpecificSettingsArray,
                               sizeof(t_codec_settings), &t_codec_settings, NULL);
        
        if (!t_codec_settings &&
            (noErr == QTGetComponentPropertyInfo(ci, kQTPropertyClass_SCAudio,
                                                 kQTSCAudioPropertyID_MagicCookie,
                                                 NULL, &t_magic_cookie_size, NULL)) && t_magic_cookie_size)
        {
            MCMemoryAllocate(t_magic_cookie_size, t_magic_cookie);
            QTGetComponentProperty(ci, kQTPropertyClass_SCAudio,
                                               kQTSCAudioPropertyID_MagicCookie,
                                               t_magic_cookie_size, t_magic_cookie, &t_magic_cookie_size);
        }
       
        Configure(t_description, &t_codec_settings, t_magic_cookie, t_magic_cookie_size);
        
        // Push back settings to MCrecord globals
        MCrecordrate = m_configuration . sample_rate;
        MCrecordsamplesize = m_configuration . sample_bit_count;
		MCrecordchannels = m_configuration . channel_count;
        
        uint32_t t_comp = MCSwapInt32NetworkToHost(m_configuration . compression_type);
		memcpy(MCrecordcompression, &t_comp, 4);
        
        m_dialog_result = kMCPlatformDialogResultSuccess;
	}
	else
    {
		if (t_error == userCanceledErr)
			m_dialog_result = kMCPlatformDialogResultCancel;
		else
		{
			char buffer[22 + U4L];
			sprintf(buffer, "error %d opening dialog", t_error);
			MCresult->copysvalue(buffer);
            m_dialog_result = kMCPlatformDialogResultError;
			return;
		}
    }
    
	CloseComponent(ci);
}

MCPlatformDialogResult MCQTSoundRecorder::EndDialog()
{
    return m_dialog_result;
}

bool MCQTSoundRecorder::StartRecording(MCStringRef p_filename)
{
    // Stop recording in case currently recording already
    StopRecording();
 
    // Ensure the sequence grabber has been initialized
    if (!EnsureInitialized())
        return false;
    
    AudioStreamBasicDescription t_description;
    InitializeConfiguration();
    GetASBD(t_description);
    
    OSErr t_error;
    t_error = noErr;
    
    // Set the chosen settings
    if (t_error == noErr)
        t_error = QTSetComponentProperty(m_channel, kQTPropertyClass_SGAudio, kQTSGAudioPropertyID_StreamFormat, sizeof(t_description), &t_description);

    // Set any additional settings for this configuration
    if (m_configuration . extra_info_size != 0)
    {
        if (m_has_magic_cookie)
            QTSetComponentProperty(m_channel, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_MagicCookie,
                                   m_configuration . extra_info_size, m_configuration . extra_info);
        else
            QTSetComponentProperty(m_channel, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_CodecSpecificSettingsArray,
                                         m_configuration . extra_info_size, m_configuration . extra_info);
    }
    
    // Set the device input
    if (t_error == noErr)
        t_error = QTSetComponentProperty(m_channel, kQTPropertyClass_SGAudioRecordDevice, kQTSGAudioPropertyID_InputSelection, sizeof(m_configuration . input), &m_configuration . input);

    // Turn on sound input metering
    if (t_error == noErr)
    {
        bool t_meter = true;
        t_error =QTSetComponentProperty(m_channel, kQTPropertyClass_SGAudio, kQTSGAudioPropertyID_LevelMetersEnabled, sizeof(t_meter), &t_meter);
    }
    
    bool t_success;
    t_success = t_error == noErr;
    
    if (t_success)
        t_success = MCS_tmpnam(m_temp_file);
    
    if (t_success)
        m_filename = MCValueRetain(p_filename);
    
    if (t_success)
    {
        // MW-2008-03-15: [[ Bug 6076 ]] Make sure we create the file before we start recording to it
        //   otherwise no recording happens.
        FILE *t_file;
        MCAutoStringRefAsUTF8String t_utf8_tempfile;
        /* UNCHECKED */ t_utf8_tempfile . Lock(m_temp_file);
        t_file = fopen(*t_utf8_tempfile, "w");
        
        if (t_file != NULL)
            fclose(t_file);
        
        NSString *t_capture_path;
        t_capture_path = MCStringConvertToAutoreleasedNSString(m_temp_file);
        
        Handle t_data_ref = nil;
        OSType t_data_ref_type = 0;
        
        if (t_capture_path)
        {
            QTNewDataReferenceFromFullPathCFString((CFStringRef)t_capture_path, (UInt32)kQTNativeDefaultPathStyle, 0, &t_data_ref, &t_data_ref_type);
            
            int t_flags;
            t_flags = seqGrabToDisk | seqGrabDontPreAllocateFileSize | seqGrabAppendToFile;
            
            t_error = SGSetDataRef(m_seq_grab, t_data_ref, t_data_ref_type, t_flags);
        }
    }
    
    if (t_success && t_error == noErr)
        t_error = SGStartRecord(m_seq_grab);
    
    t_success = t_error == noErr;
    
    if (t_success)
    {
        [m_observer startTimer];
        m_recording = true;
        MCrecording = true;
    }
    else
    {
        char buffer[21 + U4L];
		sprintf(buffer, "error %d starting recording", errno);
		MCresult->copysvalue(buffer);
        
		if (m_channel != NULL)
		{
			SGDisposeChannel(m_seq_grab, m_channel);
            m_channel = nil;
		}
        
        if (m_filename != nil)
        {
            MCValueRelease(m_filename);
            m_filename = nil;
        }
        
        if (m_temp_file != nil)
        {
            MCValueRelease(m_temp_file);
            m_temp_file = nil;
        }
    }
    return t_success;
}

void MCQTSoundRecorder::StopRecording(void)
{
    if (!m_recording)
        return;
    
    [m_observer stopTimer];
    SGStop(m_seq_grab);
    
	// PM-2015-07-22: [[ Bug 15625 ]] Make sure we properly recreate the exported file, if it already exists
	MCS_unlink(m_filename);
    exportToSoundFile(m_temp_file, m_filename);
	MCS_unlink(m_temp_file);

	if (m_filename != nil)
    {
		MCValueRelease(m_filename);
        m_filename = nil;
    }

	if (m_temp_file != nil)
    {
		MCValueRelease(m_temp_file);
        m_temp_file = nil;
    }

    if (m_channel != NULL)
    {
        SGDisposeChannel(m_seq_grab, m_channel);
        m_channel = nil;
    }
    
    m_recording = false;
    MCrecording = false;
}

void MCQTSoundRecorder::PauseRecording(void)
{
    if (!m_recording)
        return;
    
    SGPause(m_seq_grab, seqGrabPause);
}

void MCQTSoundRecorder::ResumeRecording(void)
{
    if (!m_recording)
        return;
    
    SGPause(m_seq_grab, seqGrabUnpause);
}

bool MCQTSoundRecorder::ListInputs(MCPlatformSoundRecorderListInputsCallback p_callback, void *context)
{    
    if (!EnsureInitialized())
        return false;
    
    NSArray *t_input_list = nil;
    QTGetComponentProperty(m_channel, kQTPropertyClass_SGAudioRecordDevice, kQTSGAudioPropertyID_DeviceListWithAttributes, sizeof(t_input_list), &t_input_list, NULL);
    
    if (t_input_list)
    {
        for (uindex_t i = 0; i < [t_input_list count]; i++)
        {
            NSDictionary *t_dict = [t_input_list objectAtIndex:i];
			
            unsigned int t_id = [(NSNumber*)[t_dict objectForKey:(id)kQTAudioDeviceAttribute_DeviceInputID] unsignedIntValue];
            const char *t_label = [(NSString*)[t_dict objectForKey:(id)kQTAudioDeviceAttribute_DeviceInputDescription] cStringUsingEncoding:NSMacOSRomanStringEncoding];
            
            if (!p_callback(context, t_id, t_label))
                return false;
        }
    }
    
    return true;
}

struct FormatTable
{
    const char *label;
    OSType value;
};

static const FormatTable record_formats[] =
{
    { "aiff", kQTFileTypeAIFF },
    { "wave", kQTFileTypeWave },
    { "ulaw", kQTFileTypeMuLaw },
    { "movie", kQTFileTypeMovie },
};

bool MCQTSoundRecorder::ListFormats(MCPlatformSoundRecorderListFormatsCallback p_callback, void *context)
{
    return std::all_of(std::begin(record_formats), std::end(record_formats),
                       [&](const FormatTable& p_fmt) {
                           return p_callback(context, p_fmt.value, MCSTR(p_fmt.label));
                       });
}

bool MCQTSoundRecorder::ListCompressors(MCPlatformSoundRecorderListCompressorsCallback p_callback, void *context)
{
     Component component = 0;
     ComponentDescription desc, info;
     Handle name = NewHandle(0);
     desc.componentType = kAudioEncoderComponentType;
     desc.componentSubType = 0;
     desc.componentManufacturer = 0;
     desc.componentFlags = 0;
     desc.componentFlagsMask = 0;
    
    if (!p_callback(context, kAudioFormatLinearPCM, "Linear PCM"))
        return false;
    
     while ((component = FindNextComponent(component, &desc)) != NULL)
     {
         GetComponentInfo(component, &info, name, 0, 0);
        if (GetHandleSize(name))
        {
            HLock(name);
            if (!p_callback(context, info.componentSubType, p2cstr((unsigned char *)*name)))
                return false;
            HUnlock(name);
        }
     }
     DisposeHandle(name);
    
    return true;
}

void MCQTSoundRecorder::Idle()
{
    if (SGIdle(m_seq_grab) != noErr)
        StopRecording();
}

double MCQTSoundRecorder::GetLoudness()
{
    if (!m_recording)
        return 0;

    double t_loudness = 0; 
    Float32 *t_levels;
    size_t t_array_size;
    t_array_size = m_configuration . channel_count * sizeof(Float32);
    MCMemoryNewArray(m_configuration . channel_count, t_levels);
    QTGetComponentProperty(m_channel, kQTPropertyClass_SGAudio, kQTSGAudioPropertyID_AveragePowerLevels, t_array_size,  t_levels, NULL);
    
    // convert decibels to amplitude
    t_loudness = pow(10., t_levels[0] * 0.05);
    
    MCMemoryDeleteArray(t_levels);
    
    return MCU_min(t_loudness * 100.0, 100.0);
}

MCQTSoundRecorder *MCQTSoundRecorderCreate(void)
{
    return new MCQTSoundRecorder;
}

#else   /* ifdef FEATURE_QUICKTIME */

class MCQTSoundRecorder* MCQTSoundRecorderCreate()
{
    return NULL;
}

#endif

////////////////////////////////////////////////////////
