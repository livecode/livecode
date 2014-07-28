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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "graphics.h"
#include "stack.h"
#include "execpt.h"
#include "player.h"
#include "util.h"
#include "osspec.h"

#include "osxprefix.h"

#include "platform.h"
#include "platform-internal.h"

#include <Cocoa/Cocoa.h>
#include <QTKit/QTKit.h>
#include <QuickTime/QuickTime.h>
#include <CoreAudioKit/CoreAudioKit.h>

#import <sys/stat.h>

////////////////////////////////////////////////////////////////////////////////

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
    virtual bool StartRecording(const char *filename);
    virtual void StopRecording(void);
    virtual void PauseRecording(void);
    virtual void ResumeRecording(void);
    virtual double GetLoudness(void);
    
    virtual bool ListInputs(MCPlatformSoundRecorderListInputsCallback callback, void *context);
    virtual bool ListCompressors(MCPlatformSoundRecorderListCompressorsCallback callback, void *context);
    
    void Idle(void);
    
protected:
    MCPlatformDialogResult m_dialog_result;
    SeqGrabComponent m_seq_grab;
    SGChannel m_channel;
    
    com_runrev_livecode_MCQTSoundRecorderObserver *m_observer;
    
    bool EnsureInitialized(void);
    void InitializeConfiguration(void);
    
private:
    char *m_temp_file;
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
    OSStatus err = noErr;
    
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

static void config_to_absd(const MCPlatformSoundRecorderConfiguration& p_config, AudioStreamBasicDescription& r_description)
{
    ComponentInstance ci;
    OpenADefaultComponent(StandardCompressionType, StandardCompressionSubTypeAudio, &ci);
    
    QTGetComponentProperty(ci, kQTPropertyClass_SCAudio,kQTSCAudioPropertyID_BasicDescription,sizeof(r_description), &r_description, NULL);
    
    r_description . mFormatID = p_config . compression_type;
    r_description . mSampleRate = (Float64)p_config . sample_rate * 1000;
    r_description . mChannelsPerFrame = p_config . channel_count;
    r_description . mBitsPerChannel = p_config . sample_bit_count;
    r_description . mFramesPerPacket = 1;
    r_description . mBytesPerFrame = 1;
    r_description . mBytesPerPacket = 1;
    
    CloseComponent(ci);
}

static void absd_to_config(const AudioStreamBasicDescription& p_description, MCPlatformSoundRecorderConfiguration& r_config)
{
    r_config . sample_rate = p_description . mSampleRate / 1000;
    r_config . sample_bit_count = p_description . mBitsPerChannel;
    r_config . channel_count = p_description . mChannelsPerFrame;
    r_config . compression_type = p_description . mFormatID;
}

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

static bool path_to_dataref(const char *p_path, DataReferenceRecord& r_rec)
{
	bool t_success = true;
	CFStringRef t_cf_path = NULL;
	t_cf_path = CFStringCreateWithCString(NULL, p_path, kCFStringEncodingWindowsLatin1);
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

static void exportToSoundFile(const char *sourcefile, const char *destfile)
{
	bool t_success = true;
	SoundDescriptionHandle myDesc = NULL;
	ComponentResult result = 0;
	Movie tmovie = nil;
	
	char *t_src_resolved = NULL;
	char *t_dst_resolved = NULL;
	t_src_resolved = MCS_resolvepath(sourcefile);
	t_dst_resolved = MCS_resolvepath(destfile);
	t_success = (t_src_resolved != NULL && t_dst_resolved != NULL);
	
	DataReferenceRecord t_src_rec, t_dst_rec;
	t_src_rec.dataRef = NULL;
	t_dst_rec.dataRef = NULL;
	
	if (t_success)
	{
		t_success = path_to_dataref(t_src_resolved, t_src_rec) &&
		path_to_dataref(t_dst_resolved, t_dst_rec);
	}
	
	free(t_src_resolved);
	free(t_dst_resolved);
	
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
		switch (MCrecordformat)
		{
			case EX_WAVE:
				cd.componentSubType = kQTFileTypeWave;
				break;
			case EX_ULAW:
				cd.componentSubType = kQTFileTypeMuLaw;
				break;
			case EX_AIFF:
				cd.componentSubType = kQTFileTypeAIFF;
				break;
			default:
				cd.componentSubType = kQTFileTypeMovie;
				break;
		}
		cd.componentManufacturer = SoundMediaType;
		cd.componentFlags = canMovieExportFiles;
		cd.componentFlagsMask = canMovieExportFiles;
		c = FindNextComponent(nil, &cd);
		(**myDesc).numChannels = MCrecordchannels;
		(**myDesc).sampleSize = MCrecordsamplesize;
		(**myDesc).sampleRate = (uint32_t)(MCrecordrate * 1000 * 65536);
		exporter = nil;
		exporter = OpenComponent(c);
		result = MovieExportSetSampleDescription(exporter, (SampleDescriptionHandle)myDesc, SGAudioMediaType);
		errno = ConvertMovieToDataRef(tmovie, 0, t_dst_rec . dataRef, t_dst_rec . dataRefType, cd.componentSubType,
									  0, 0, exporter);
		// try showUserSettingsDialog | movieToFileOnlyExport | movieFileSpecValid
		DisposeHandle((Handle) myDesc);
		if (exporter)
			CloseComponent(exporter);
		if (errno != noErr)
		{
			char buffer[26 + U4L];
			sprintf(buffer, "error %d exporting recording", errno);
			MCresult->copysvalue(buffer);
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
    m_temp_file = nil;
    m_filename = nil;
    
    m_observer = [[com_runrev_livecode_MCQTSoundRecorderObserver alloc] initWithRecorder: this];
}

MCQTSoundRecorder::~MCQTSoundRecorder(void)
{
}

////////////////////////////////////////////////////////

void MCQTSoundRecorder::InitializeConfiguration()
{
    m_configuration . sample_rate = MCrecordrate;
    m_configuration . sample_bit_count = MCrecordsamplesize;
    m_configuration . channel_count = MCrecordchannels;
    
    uint32_t t_compression;
    memcpy(&t_compression, MCrecordcompression, 4);
    t_compression = EndianU32_NtoB(t_compression);
    
    m_configuration . compression_type = kAudioFormatLinearPCM;// t_compression;
    
    uint32_t t_input;
    t_input = 0;
    
    if (!MCCStringEqual(MCrecordinput, "dflt"))
    {
        memcpy(&t_input, MCrecordinput, 4);
        t_input = EndianU32_NtoB(t_input);
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
    
    m_configuration . extra_info = nil;
    m_configuration . extra_info_size = 0;
}

bool MCQTSoundRecorder::EnsureInitialized()
{
    if (m_seq_grab != nil)
        return true;
    
    OpenADefaultComponent(SeqGrabComponentType, 0, &m_seq_grab);
    
    bool t_success;
    t_success = true;
    
    if (t_success)
        t_success = SGInitialize(m_seq_grab) == noErr;
    
    if (t_success)
        t_success = SGNewChannel(m_seq_grab, SGAudioMediaType, &m_channel) == noErr;
    
    if (t_success)
        t_success = SGSetChannelUsage(m_channel, seqGrabRecord) == noErr;
    
    InitializeConfiguration();
    
    if (!t_success)
    {
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
    config_to_absd(m_configuration, t_description);
    
    OSErr t_error;
    t_error = QTSetComponentProperty(ci, kQTPropertyClass_SCAudio,kQTSCAudioPropertyID_BasicDescription,sizeof(t_description), &t_description);
    
    MCQTSoundRecorderAvailableCompressionIdsState t_state;
    t_state . list = nil;
    t_state . count = 0;
    UInt32 limitedFormats[] = { 'lpcm', 'aac ', 'alac', 'samr', 'ima4' };
   // if (ListCompressors(available_compression_ids, &t_state))
        t_error = QTSetComponentProperty(ci, kQTPropertyClass_SCAudio,kQTSCAudioPropertyID_ClientRestrictedCompressionFormatList,sizeof(limitedFormats), &limitedFormats);
    
    MCMemoryDeleteArray(t_state . list);
    
    AudioChannelLayoutTag t_layout_tags[] =
    {
        kAudioChannelLayoutTag_UseChannelDescriptions,
        kAudioChannelLayoutTag_Mono,
        kAudioChannelLayoutTag_Stereo,
    };

    QTSetComponentProperty(ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_ClientRestrictedChannelLayoutTagList,
                           sizeof(t_layout_tags), t_layout_tags);
    
	errno = SCRequestImageSettings(ci);
	if (errno == noErr)
	{
        QTGetComponentProperty(ci, kQTPropertyClass_SCAudio,kQTSCAudioPropertyID_BasicDescription,sizeof(t_description), &t_description, NULL);
        
        absd_to_config(t_description, m_configuration);
        
		MCrecordrate = m_configuration . sample_rate;
        MCrecordsamplesize = m_configuration . sample_bit_count;
		MCrecordchannels = m_configuration . channel_count;
        
        uint32_t t_comp = EndianU32_BtoN(m_configuration . compression_type);
		memcpy(MCrecordcompression, &t_comp, 4);
     
        m_dialog_result = kMCPlatformDialogResultSuccess;
	}
	else
    {
		if (errno == userCanceledErr)
			m_dialog_result = kMCPlatformDialogResultCancel;
		else
		{
			char buffer[22 + U4L];
			sprintf(buffer, "error %d opening dialog", errno);
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

bool MCQTSoundRecorder::StartRecording(const char *p_filename)
{
    // Ensure the sequence grabber has been initialized
    if (!EnsureInitialized())
        return false;
    
    // Stop recording in case currently recording already
    StopRecording();
    
    bool t_success;
    t_success = true;
    
    AudioStreamBasicDescription t_description;
    config_to_absd(m_configuration, t_description);
    
    // Set the chosen settings
    if (t_success)
        t_success = QTSetComponentProperty(m_channel, kQTPropertyClass_SGAudio, kQTSGAudioPropertyID_StreamFormat, sizeof(t_description), &t_description) == noErr;
    
    // Set the device input
    if (t_success)
        t_success = QTSetComponentProperty(m_channel, kQTPropertyClass_SGAudioRecordDevice, kQTSGAudioPropertyID_InputSelection, sizeof(m_configuration . input), &m_configuration . input) == noErr;
    
    // Turn on sound input metering
    if (t_success)
    {
        bool t_meter = true;
        t_success = QTSetComponentProperty(m_channel, kQTPropertyClass_SGAudio, kQTSGAudioPropertyID_LevelMetersEnabled, sizeof(t_meter), &t_meter) == noErr;
    }
    
    if (t_success)
        t_success = MCCStringClone(MCS_tmpnam(), m_temp_file);
    
    if (t_success)
    {
        // MW-2008-03-15: [[ Bug 6076 ]] Make sure we create the file before we start recording to it
        //   otherwise no recording happens.
        FILE *t_file;
        t_file = fopen(m_temp_file, "w");
        if (t_file != NULL)
            fclose(t_file);
        
        NSString *t_capture_path;
        t_capture_path = [NSString stringWithCString:m_temp_file encoding:NSMacOSRomanStringEncoding];
        
        OSStatus err = noErr;
        Handle t_data_ref = nil;
        OSType t_data_ref_type = 0;
        
        if (t_capture_path)
        {
            QTNewDataReferenceFromFullPathCFString((CFStringRef)t_capture_path, (UInt32)kQTNativeDefaultPathStyle, 0, &t_data_ref, &t_data_ref_type);
            
            int t_flags;
            t_flags = seqGrabToDisk | seqGrabDontPreAllocateFileSize | seqGrabAppendToFile;
            
            t_success = SGSetDataRef(m_seq_grab, t_data_ref, t_data_ref_type, t_flags) == noErr;
        }
    }
    
    if (t_success)
        t_success = MCCStringClone(p_filename, m_filename);
    
    if (t_success)
        t_success = SGStartRecord(m_seq_grab) == noErr;
    
    if (t_success)
    {
        [m_observer startTimer];
        m_recording = true;
        MCrecording = true;
    }
    else
    {
        if (m_seq_grab != nil)
		{
			CloseComponent(m_seq_grab);
			m_seq_grab = nil;
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
    
    if (m_seq_grab != nil)
    {
        CloseComponent(m_seq_grab);
        m_seq_grab = nil;
    }
    
    exportToSoundFile(m_temp_file, m_filename);
    
    delete m_filename;
    m_filename = nil;
    
    delete m_temp_file;
    m_temp_file = nil;
    
    m_recording = false;
    MCrecording = false;
}


void MCQTSoundRecorder::PauseRecording(void)
{
    if (!m_recording)
        return;
    
    SGPause(m_seq_grab, seqGrabPause);
    
    m_recording = false;
    MCrecording = false;
}

void MCQTSoundRecorder::ResumeRecording(void)
{
    if (!m_recording)
        return;
    
    SGPause(m_seq_grab, seqGrabUnpause);
    
    m_recording = true;
    MCrecording = true;
}

bool MCQTSoundRecorder::ListInputs(MCPlatformSoundRecorderListInputsCallback p_callback, void *context)
{
    // kQTSGAudioPropertyID_DeviceListWithAttributes
    // Should be listing kQTSGAudioPropertyID_DeviceUID?
    
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

bool MCQTSoundRecorder::ListCompressors(MCPlatformSoundRecorderListCompressorsCallback p_callback, void *context)
{
     Component component = 0;
     ComponentDescription desc, info;
     Handle name = NewHandle(0);
     desc.componentType = kSoundCompressor;
     desc.componentSubType = 0;
     desc.componentManufacturer = 0;
     desc.componentFlags = 0;
     desc.componentFlagsMask = 0;
     
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
    
    t_loudness = t_levels[0];
    MCMemoryDeleteArray(t_levels);

    return t_loudness;
}

MCQTSoundRecorder *MCQTSoundRecorderCreate(void)
{
    return new MCQTSoundRecorder;
}

////////////////////////////////////////////////////////
