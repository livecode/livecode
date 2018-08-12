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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"

#include "player.h"
#include "util.h"
#include "osspec.h"

#include "osxprefix.h"

#include "platform.h"
#include "platform-internal.h"

#include "variable.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Platform Sound Recorder Class Implementation
//

MCPlatformSoundRecorder::MCPlatformSoundRecorder(void)
{
    m_references = 1;

    m_recording = false;
    m_filename = nil;
    
    m_configuration . input = 0;
    m_configuration . sample_rate = MCrecordrate;
    m_configuration . sample_bit_count = MCrecordsamplesize;
    m_configuration . channel_count = MCrecordchannels;
    // TODO - compression
    // m_configuration . compression_type = MCrecordcompression;
    m_configuration . extra_info = nil;
    m_configuration . extra_info_size = 0;
}

MCPlatformSoundRecorder::~MCPlatformSoundRecorder(void)
{
    if (m_configuration . extra_info != nil)
        MCMemoryDelete(m_configuration . extra_info);
    
    if (m_filename != nil)
        MCMemoryDelete(m_filename);
}

void MCPlatformSoundRecorder::Retain(void)
{
	m_references += 1;
}

void MCPlatformSoundRecorder::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
		delete this;
}

bool MCPlatformSoundRecorder::IsRecording()
{
    return m_recording;
}

void MCPlatformSoundRecorder::SetProperty(MCPlatformSoundRecorderProperty p_property, MCPlatformPropertyType type, void *p_value)
{
    switch(p_property)
	{
		case kMCPlatformSoundRecorderPropertyInput:
            m_configuration . input = *(unsigned int *)p_value;
			break;
		case kMCPlatformSoundRecorderPropertySampleRate:
            m_configuration . sample_rate = *(double *)p_value;
			break;
		case kMCPlatformSoundRecorderPropertySampleBitCount:
            m_configuration . sample_bit_count = *(unsigned int *)p_value;
			break;
		case kMCPlatformSoundRecorderPropertyChannelCount:
            m_configuration . channel_count = *(unsigned int *)p_value;
			break;
		case kMCPlatformSoundRecorderPropertyCompressionType:
            m_configuration . compression_type = *(unsigned int *)p_value;
			break;
		case kMCPlatformSoundRecorderPropertyExtraInfo:
			MCUnreachable();
			break;
    }
}

void MCPlatformSoundRecorder::GetProperty(MCPlatformSoundRecorderProperty p_property, MCPlatformPropertyType type, void *r_value)
{
    switch(p_property)
	{
		case kMCPlatformSoundRecorderPropertyInput:
            *(unsigned int *)r_value = m_configuration . input;
			break;
		case kMCPlatformSoundRecorderPropertySampleRate:
            *(double *)r_value = m_configuration . sample_rate;
			break;
		case kMCPlatformSoundRecorderPropertySampleBitCount:
            *(unsigned int *)r_value = m_configuration . sample_bit_count;
			break;
		case kMCPlatformSoundRecorderPropertyChannelCount:
            *(unsigned int *)r_value = m_configuration . channel_count;
			break;
		case kMCPlatformSoundRecorderPropertyCompressionType:
            *(unsigned int *)r_value = m_configuration . compression_type;
			break;
		case kMCPlatformSoundRecorderPropertyExtraInfo:
			MCUnreachable();
			break;
    }
}

void MCPlatformSoundRecorder::GetConfiguration(MCPlatformSoundRecorderConfiguration &r_config)
{
    r_config . input = m_configuration . input;
    r_config . sample_rate = m_configuration . sample_rate;
    r_config . sample_bit_count = m_configuration . sample_bit_count;
    r_config . channel_count = m_configuration . channel_count;
    r_config . compression_type = m_configuration . compression_type;
    
    uint8_t *extra_info;
    size_t extra_info_size;
    extra_info_size = m_configuration . extra_info_size;
    
    MCMemoryAllocateCopy(m_configuration . extra_info, sizeof(uint8_t) *extra_info_size, extra_info);

    r_config . extra_info = extra_info;
    r_config . extra_info_size = extra_info_size;
}

void MCPlatformSoundRecorder::SetConfiguration(const MCPlatformSoundRecorderConfiguration p_config)
{
    m_configuration . input = p_config . input;
    m_configuration . sample_rate = p_config . sample_rate;
    m_configuration . sample_bit_count = p_config . sample_bit_count;
    m_configuration . channel_count = p_config . channel_count;
    m_configuration . compression_type = p_config . compression_type;
    
    if (m_configuration . extra_info != nil)
        MCMemoryDelete(m_configuration . extra_info);
    
    uint8_t *extra_info;
    size_t extra_info_size;
    extra_info_size = m_configuration . extra_info_size;
    
    MCMemoryAllocateCopy(m_configuration . extra_info, sizeof(uint8_t) *extra_info_size, extra_info);
    
    m_configuration . extra_info = extra_info;
    m_configuration . extra_info_size = extra_info_size;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Platform Sound Recorder Procedural Wrappers
//

class MCQTSoundRecorder;
extern MCQTSoundRecorder *MCQTSoundRecorderCreate();
extern bool MCQTInit(void);

void MCPlatformSoundRecorderCreate(MCPlatformSoundRecorderRef& r_recorder)
{
    if (!MCQTInit() && MCmajorosversion >= 0x1080)
        MCresult -> sets("could not initialize quicktime");
    else
        r_recorder = (MCPlatformSoundRecorderRef)MCQTSoundRecorderCreate();
}

void MCPlatformSoundRecorderRetain(MCPlatformSoundRecorderRef p_recorder)
{
    p_recorder -> Retain();
}

void MCPlatformSoundRecorderRelease(MCPlatformSoundRecorderRef p_recorder)
{
    p_recorder -> Release();
}

bool MCPlatformSoundRecorderIsRecording(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> IsRecording();
}

double MCPlatformSoundRecorderGetLoudness(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> GetLoudness();
}

bool MCPlatformSoundRecorderStart(MCPlatformSoundRecorderRef p_recorder, MCStringRef p_filename)
{
    return p_recorder -> StartRecording(p_filename);
}

void MCPlatformSoundRecorderStop(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> StopRecording();
}

void MCPlatformSoundRecorderPause(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> PauseRecording();
}

void MCPlatformSoundRecorderResume(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> ResumeRecording();
}

bool MCPlatformSoundRecorderListInputs(MCPlatformSoundRecorderRef p_recorder, MCPlatformSoundRecorderListInputsCallback callback, void *context)
{
    return p_recorder -> ListInputs(callback, context);
}

bool MCPlatformSoundRecorderListCompressors(MCPlatformSoundRecorderRef p_recorder, MCPlatformSoundRecorderListCompressorsCallback callback, void *context)
{
    return p_recorder -> ListCompressors(callback, context);
}

bool MCPlatformSoundRecorderListFormats(MCPlatformSoundRecorderRef p_recorder, MCPlatformSoundRecorderListFormatsCallback callback, void *context)
{
    return p_recorder -> ListFormats(callback, context);
}

void MCPlatformSoundRecorderGetConfiguration(MCPlatformSoundRecorderRef p_recorder, MCPlatformSoundRecorderConfiguration& r_config)
{
    p_recorder -> GetConfiguration(r_config);
}

void MCPlatformSoundRecorderSetConfiguration(MCPlatformSoundRecorderRef p_recorder, const MCPlatformSoundRecorderConfiguration& p_config)
{
    p_recorder -> SetConfiguration(p_config);
}

void MCPlatformSoundRecorderBeginConfigurationDialog(MCPlatformSoundRecorderRef p_recorder)
{
    p_recorder -> BeginDialog();
}

MCPlatformDialogResult MCPlatformSoundRecorderEndConfigurationDialog(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> EndDialog();
}

////////////////////////////////////////////////////////////////////////////////
