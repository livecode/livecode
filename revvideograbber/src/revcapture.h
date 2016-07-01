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

#ifndef __REV_CAPTURE__
#define __REV_CAPTURE__

struct MCCaptureInputDevice
{
    virtual ~MCCaptureInputDevice() {}

	virtual bool IsDefaultForAudio(void) = 0;
	virtual bool IsDefaultForVideo(void) = 0;
	
	virtual bool IsAudio(void) = 0;
	virtual bool IsVideo(void) = 0;
	
	virtual const char *GetName(void) = 0;
};

struct MCCaptureAudioPreviewDevice
{
    virtual ~MCCaptureAudioPreviewDevice() {}

	virtual bool IsDefault(void) = 0;
	
	virtual const char *GetName(void) = 0;
};

struct MCCaptureSessionDelegate
{
	// The list of available input devices has changed.
	virtual void InputDevicesChanged(void) = 0;
	
	// The list of available audio preview devices has changed.
	virtual void AudioPreviewDevicesChanged(void) = 0;
	
	// The video preview has changed frame.
	virtual void VideoPreviewChanged(void *pixel_data) = 0;
	
	// An error has occured on the session.
	virtual void ErrorOccured(void) = 0;
};

typedef bool (*MCCaptureListInputDevicesCallback)(void *context, MCCaptureInputDevice *device);
typedef bool (*MCCaptureListAudioPreviewDevicesCallback)(void *context, MCCaptureAudioPreviewDevice *device);

enum MCCaptureState
{
	kMCCaptureStateStopped,
	kMCCaptureStatePrepared,
	kMCCaptureStateRunning,
	kMCCaptureStatePaused,
};

enum MCCaptureError
{
	kMCCaptureErrorNone,
	
	kMCCaptureErrorOutOfMemory,
	
	kMCCaptureErrorCouldNotSetupSession,
	
	// The given operation cannot be performed while the session is
	// recording or previewing.
	kMCCaptureErrorNotWhileRunning,
	
	// The device pointer passed to the method is invalid.
	kMCCaptuerErrorBadDevice,
	// The device could not be opened.
	kMCCaptureErrorCouldNotOpenDevice,
	// The device could not be wrapped with an input
	kMCCaptureErrorCouldNotWrapDevice,
	// The device could not be connected to.
	kMCCaptureErrorCouldNotConnectToDevice,
	
	// The input device passed to the method cannot produce audio.
	kMCCaptureErrorNotAnAudioDevice,
	// The input device passed to the method cannot produce video.
	kMCCaptureErrorNotAVideoDevice,
	
	// The volume parameter is out of range.
	kMCCaptureErrorBadAudioPreviewVolume,
	// The size parameter(s) is out of range.
	kMCCaptureErrorBadVideoPreviewSize,
	
	// The video preview device could not be configured
	kMCCaptureErrorCouldNotSetupVideoPreview
};

struct MCCaptureImageBuffer
{
	int32_t width;
	int32_t height;
	int32_t stride;
	void *pixels;
};

struct MCCaptureSession
{
    virtual ~MCCaptureSession() {}

	virtual bool Create(MCCaptureSessionDelegate *delegate) = 0;
	virtual void Destroy(void) = 0;
	
	virtual MCCaptureError Error(void) = 0;
	
	virtual bool ListInputDevices(MCCaptureListInputDevicesCallback callback, void *context) = 0;
	virtual bool GetAudioInputDevice(MCCaptureInputDevice*& r_device) = 0;
	virtual bool SetAudioInputDevice(MCCaptureInputDevice* device) = 0;
	virtual bool GetVideoInputDevice(MCCaptureInputDevice*& r_device) = 0;
	virtual bool SetVideoInputDevice(MCCaptureInputDevice* device) = 0;
	
	virtual bool ListAudioPreviewDevices(MCCaptureListAudioPreviewDevicesCallback callback, void *context) = 0;
	virtual bool GetAudioPreviewDevice(MCCaptureAudioPreviewDevice*& r_device) = 0;
	virtual bool SetAudioPreviewDevice(MCCaptureAudioPreviewDevice *device) = 0;
	virtual bool GetAudioPreviewVolume(double*& r_volume) = 0;
	virtual bool SetAudioPreviewVolume(double volume) = 0;
	virtual bool SetVideoPreviewBuffer(const MCCaptureImageBuffer *buffer) = 0;
	virtual bool GetVideoPreviewBuffer(MCCaptureImageBuffer& r_buffer) = 0;
	
	virtual bool GetPreviewingState(MCCaptureState& r_status) = 0;
	virtual bool StartPreviewing(void) = 0;
	virtual bool StopPreviewing(void) = 0;
	virtual bool PausePreviewing(void) = 0;
	virtual bool ResumePreviewing(void) = 0;
};

bool MCQTXCaptureSessionFactory(MCCaptureSession*& r_session);

#endif
