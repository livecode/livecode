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

#ifndef VideoGrabber_H

#define VideoGrabber_H


#ifndef Bool
#define Bool int
#define True 1
#define False 0
#endif

#ifdef WIN32
#include <windows.h>
#else
#include <ApplicationServices/ApplicationServices.h>
#include <QuickTime/QuickTime.h>
typedef CGrafPtr GWorldPtr;
#endif

enum VideoGrabberMode
{
	VIDEOGRABBERMODE_NONE = 0,
	VIDEOGRABBERMODE_RECORDING,
	VIDEOGRABBERMODE_PREVIEWING
};

enum CVGSettingTypeTag
{
	kCVGSettingTypeAuto,
	kCVGSettingTypeInteger,
	kCVGSettingTypeCardinal,
	kCVGSettingTypeReal,
	kCVGSettingTypeCString
};

struct CVGSetting
{
	CVGSettingTypeTag type;
	union
	{
		int integer;
		unsigned int cardinal;
		float real;
		char *cstring;
	};
};

typedef bool (*CVGListSettingsCallback)(void *context, const char *name);

class CVideoGrabber
{
public:
	virtual ~CVideoGrabber() {;}
	virtual Bool IsInited() = 0;
	virtual void DoIdle() = 0;
	virtual void SetRect(short left, short top, short right, short bottom) = 0;
	virtual void GetRect(short *left, short *top, short *right, short *bottom) = 0;
	virtual void SetVisible(Bool tvisible) = 0;
	virtual void StartRecording(char *filename) = 0;
	virtual void StopRecording() = 0;
	virtual void AddError(char *serr) = 0;
	virtual void GetSettingsString(ExternalString &s) = 0;
	virtual void SetSettingsString(ExternalString &s) = 0;
	virtual void StartPreviewing() = 0;
	virtual void StopPreviewing() = 0;  
	virtual void VideoFormatDialog() = 0;
	virtual void VideoSourceDialog() = 0;
	virtual void VideoDisplayDialog() = 0;
	virtual void VideoCompressionDialog() = 0;
	virtual void VideoDefaultDialog(void) = 0;
	virtual void AudioDefaultDialog(void) = 0;
	virtual void SetFrameRate(int framerate) = 0;
	virtual void SetAudioCapture(Bool bSet) = 0;
	virtual void setCompressor(char* codecName) = 0;
	virtual void setAudioCompressor(char *codecName)= 0;
	//get all available codec names, return a '\n' delimited list.
	//the caller must call  FreeMemory() to free the string buffer
	virtual char *getCompressors() = 0;
	virtual char *getAudioCompressors() = 0;
	virtual char *GetCurrentCodecName() = 0;
	virtual char *GetCurrentAudioCodecName() = 0;
	virtual void setAudioFormat(int channels, int bits, int frequency) = 0;
	virtual void getAudioFormat(int* channels, int* bits, int* frequency) = 0;	
	virtual void GetFrameRate(double *frate) = 0;
	virtual void GetFrameSize(int *fwidth,int *fheight) = 0;
	virtual void SetFrameSize(int fwidth,int fheight) = 0;
	virtual Bool GetAudioCapture() {return False;}

	virtual bool ListSettings(CVGListSettingsCallback callback, void *context) {return false;}
	virtual bool GetSetting(const char *name, CVGSetting& r_setting) {return false;}
	virtual bool SetSetting(const char *name, const CVGSetting p_setting) {return false;}

#ifdef WIN32
	virtual Bool Draw(int twidth,int theight, HDC hdcMem) = 0;
#else
 virtual Bool Draw(int twidth,int theight, GWorldPtr gworldMem) = 0;
#endif
protected:
	VideoGrabberMode videomode;
	Bool visible; 
};

#endif
