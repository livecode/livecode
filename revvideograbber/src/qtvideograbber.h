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

#ifndef QTVIDEO_H

#define QTVIDEO_H
#include "videograbber.h"
#ifdef WIN32
// This is really nasty but required to avoid breaking revvideograbber for Windows:
//    The QuickTime SDK includes a bundled stdint.h header that conflicts with the
//    version supplied with VS2010 and later, so we need to prevent its inclusion.
#define _STDINT_H
#include <windows.h>
#include <QTML.h>
#include <QuickTimeComponents.h>
#endif
#include <stdlib.h>
#include <string.h>

class CQTVideoGrabber: public CVideoGrabber
{
public:
#ifdef WIN32
	  CQTVideoGrabber(HWND whichwindow);
#else
	  CQTVideoGrabber(WindowPtr whichwindow);
#endif
	  CQTVideoGrabber();
	  virtual ~CQTVideoGrabber();
	  Bool IsInited() {return inited;}
	  void DoIdle();
	  void Init();
	  void SetRect(short left, short top, short right, short bottom);
	  void GetRect(short *left, short *top, short *right, short *bottom);
	  void SetVisible(Bool tvisible);
	  void GetSettingsString(ExternalString &s);
	  void SetSettingsString(ExternalString &s);
	  void StartRecording(char *filename);
	  void StopRecording();
	  void InitChannels();
	  void StartPreviewing();
	  void StopPreviewing();
	  void VideoFormatDialog();
	  void VideoSourceDialog();
	  void VideoDisplayDialog();
	  void VideoDefaultDialog(void);
	  void AudioDefaultDialog(void);
	  void VideoCompressionDialog() {;}
	  void SetFrameRate(int framerate);
	  void SetAudioCapture(Bool bSet);
	  void setCompressor(char* codecName);
	  char *getCompressors();
	  void AddError(char *serr);
	  void setAudioFormat(int channels, int bits, int frequency);
	  void getAudioFormat(int* channels, int* bits, int* frequency);


	  	 Bool GetAudioCapture() {return False;}

		void	setAudioCompressor(char *codecName){};
		//get all available codec names, return a '\n' delimited list.
		//the caller must call  FreeMemory() to free the string buffer
		char *getAudioCompressors(){return NULL;}
		char *GetCurrentCodecName(){return NULL;}
		char *GetCurrentAudioCodecName(){return NULL;}



#ifdef WIN32
	  Bool Draw(int twidth,int theight, HDC hdcMem);
#else
		 Bool Draw(int twidth,int theight, GWorldPtr hdcMem);
#endif
	
		void GetFrameRate(double *frate);
		void GetFrameSize(int *fwidth,int *fheight);
		void SetFrameSize(int fwidth,int fheight);

#ifdef _MACOSX
		void Synchronize(bool p_visible);
#endif

protected:
	  void DoDialog();
	  Bool inited;
	  SeqGrabComponent videograbber;
	  SGChannel videochannel;
	  SGChannel	soundchannel;
	  GWorldPtr destvideobuffer;
	  #ifdef WIN32
	  HWND parentwindow;
	  HWND videowindow;
	  #else
	  void *parentwindow;
	  WindowPtr videowindow;
    void *videowindow_cocoa;
	void *m_window_observer;
	  #endif
	  Bool buffervideo;
	  Rect destvideorect;
	  Rect srcvideorect;
};

#endif
