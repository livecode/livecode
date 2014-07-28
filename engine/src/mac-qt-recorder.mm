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

static void initialize_configuration(MCPlatformSoundRecorderConfiguration& r_config)
{
    r_config . sample_rate = MCrecordrate;
    r_config . sample_bit_count = MCrecordsamplesize;
    r_config . channel_count = MCrecordchannels;
    
    uint32_t t_compression;
    memcpy(&t_compression, MCrecordcompression, 4);
    t_compression = EndianU32_NtoB(t_compression);
    
    r_config . compression_type = t_compression;
    r_config . input = 0;
    r_config . extra_info = nil;
    r_config . extra_info_size = 0;
}

static void config_to_absd(const MCPlatformSoundRecorderConfiguration& p_config, AudioStreamBasicDescription& r_description)
{
    r_description . mSampleRate = (Float64)p_config . sample_rate * 1000;
    r_description . mFormatID = kAudioFormatLinearPCM; //p_config . compression_type;
    r_description . mFormatFlags = 12;
    r_description . mFramesPerPacket = 1;
    r_description . mChannelsPerFrame = p_config . channel_count;
    r_description . mBitsPerChannel = p_config . sample_bit_count;
    r_description . mBytesPerPacket = sizeof(short) * 2;
    r_description . mBytesPerFrame = sizeof(short) * 2;
    r_description . mReserved = 0;
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
		if (aTrackType == SGAudioMediaType)
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
		cd.componentManufacturer = SGAudioMediaType;
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
    
    initialize_configuration(m_configuration);
    
    AudioStreamBasicDescription t_desc;
    config_to_absd(m_configuration, t_desc);
    if (t_success)
        t_success = QTSetComponentProperty(m_channel, kQTPropertyClass_SCAudio,kQTSCAudioPropertyID_BasicDescription,sizeof(t_desc), &t_desc) == noErr;
    
    if (!t_success)
    {
        CloseComponent(m_seq_grab);
        m_seq_grab = nil;
    }
    
    return t_success;
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
    MCPlatformSoundRecorderConfiguration t_configuration;
    
    initialize_configuration(t_configuration);
    config_to_absd(t_configuration, t_description);
    
    OSErr t_error;
    
    t_error = QTSetComponentProperty(ci, kQTPropertyClass_SCAudio,kQTSCAudioPropertyID_BasicDescription,sizeof(t_description), &t_description);
    
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
    
    if (m_seq_grab == nil)
        return false;
    
    bool t_success;
    t_success = true;
    
    AudioStreamBasicDescription t_description;
    config_to_absd(m_configuration, t_description);
    
    if (t_success)
        t_success = QTSetComponentProperty(m_channel, kQTPropertyClass_SGAudio, kQTSGAudioPropertyID_StreamFormat, sizeof(t_description), &t_description) == noErr;
    
    if (t_success)
    {
        NSArray * list = nil;
        QTGetComponentProperty(m_channel, kQTPropertyClass_SGAudioRecordDevice, kQTSGAudioPropertyID_InputListWithAttributes, sizeof(list), &list, NULL);
        
        uint32_t t_driver;
        if (list)
        {
            NSDictionary * selDict = [list objectAtIndex:0];
            t_driver =
            [(NSNumber*)[selDict objectForKey:(id)kQTAudioDeviceAttribute_DeviceInputID]
             unsignedIntValue];
            t_success = QTSetComponentProperty(m_channel, kQTPropertyClass_SGAudioRecordDevice, kQTSGAudioPropertyID_InputSelection, sizeof(t_driver), &t_driver) == noErr;
            
            if (t_success)
            {
                //turn on sound input metering
                bool t_meter = true;
                t_success = QTSetComponentProperty(m_channel, kQTPropertyClass_SGAudioRecordDevice, kQTSGAudioPropertyID_LevelMetersEnabled, sizeof(t_meter), &t_meter) == noErr;
            }
        }
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
    QTGetComponentProperty(m_channel, kQTPropertyClass_SGAudioRecordDevice, kQTSGAudioPropertyID_InputListWithAttributes, sizeof(t_input_list), &t_input_list, NULL);
    
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
    // How does one get the full description of a compression from its id????
    
    if (!EnsureInitialized())
        return false;
    
    OSStatus err = noErr;
    AudioStreamBasicDescription *t_formats = NULL;
    ByteCount t_size = 0;
    uindex_t t_num_formats;

    QTGetComponentPropertyInfo(m_channel, kQTPropertyClass_SGAudioRecordDevice, kQTSGAudioPropertyID_StreamFormatList, NULL, &t_size, NULL);
    
    MCMemoryAllocate(t_size, t_formats);
    t_num_formats = t_size / sizeof(AudioStreamBasicDescription);
    
    QTGetComponentProperty(m_channel, kQTPropertyClass_SGAudioRecordDevice, kQTSGAudioPropertyID_StreamFormatList, t_size, t_formats, NULL);
    
    for (uindex_t i = 0; i < t_num_formats; i++)
    {
        uint32_t t_id, t_comp;
        char t_label[5];
        t_id = t_formats[i] . mFormatID;
        t_comp = EndianU32_BtoN(t_id);
		memcpy(t_label, &t_comp, 4);
        
        if (!p_callback(context, t_id, t_label))
            return false;
    }
    
    return true;
    
    /*
    Component component = 0;
	ComponentDescription desc, info;
	Handle name = NewHandle(0);
	desc.componentType = kSoundCompressor;
	desc.componentSubType = 0;
	desc.componentManufacturer = 0;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
    if (!p_callback(context, 'raw', "No compression"))
        return false;
    
	while ((component = FindNextComponent(component, &desc)) != NULL)
	{
		GetComponentInfo(component, &info, name, 0, 0);
		if (GetHandleSize(name))
		{
			HLock(name);
			long compType;
			compType = EndianU32_BtoN(info.componentSubType);
			if (!p_callback(context, compType, p2cstr((unsigned char *)*name)))
                return false;
			HUnlock(name);
		}
	}
	DisposeHandle(name);
     */
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
    
    if (!EnsureInitialized())
        return 0;
    
    uint1 t_channel_count;
    t_channel_count = m_configuration . channel_count;
    
    Float32 *t_levels;
    MCMemoryNewArray(t_channel_count, t_levels);
    
    QTGetComponentProperty(m_channel, kQTPropertyClass_SGAudioRecordDevice, kQTSGAudioPropertyID_AveragePowerLevels, t_channel_count * sizeof(Float32), t_levels, NULL);
    
    double t_loudness = t_levels[0];
    MCMemoryDeleteArray(t_levels);
    
    return t_loudness;
}

MCQTSoundRecorder *MCQTSoundRecorderCreate(void)
{
    return new MCQTSoundRecorder;
}

////////////////////////////////////////////////////////
