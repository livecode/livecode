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

#undef UNICODE
#undef _UNICODE

#include <revolution/external.h>
#include <stdio.h>

#include "mcivideograbber.h"

int CWinVideoGrabber::winid = 177;

//open sequence grabber component instance and draw to window
CWinVideoGrabber::CWinVideoGrabber(HWND whichwindow)
{ 
	parentwindow = whichwindow;
	videomode = VIDEOGRABBERMODE_NONE;
	videowindow = capCreateCaptureWindow("CAPTURE", WS_CHILD | WS_VISIBLE, 0, 0, 160, 120, (HWND) whichwindow, winid++); 
	SendMessage (videowindow, WM_CAP_DRIVER_CONNECT, 0, 0L); 
	capPreviewScale(videowindow,True);
	capPreviewRate(videowindow, 50);
	Init();
}

 void CWinVideoGrabber::Init()
{
	inited = True;
}

void CWinVideoGrabber::GetSettingsString(ExternalString &s)
{
}

void CWinVideoGrabber::SetSettingsString(ExternalString &s)
{
}

//open sequence grabber component instance and draw to gworld
CWinVideoGrabber::CWinVideoGrabber()
{ 
	destvideorect.top = destvideorect.left = 0;
	destvideorect.bottom = destvideorect.right = 100;
	videowindow = NULL;
	buffervideo = True;
		videomode = VIDEOGRABBERMODE_NONE;
}


CWinVideoGrabber::~CWinVideoGrabber()
{
	if (!inited) return;
	StopRecording();
	StopPreviewing();
	capDriverDisconnect(videowindow); 
	DestroyWindow(videowindow);
}




void CWinVideoGrabber::SetVisible(Bool tvisible)
{
	if (!inited) return;
	if (tvisible == visible)
		return;
	if (tvisible)
		StartPreviewing();
	else 
		StopPreviewing();
}



void CWinVideoGrabber::StartPreviewing()
{
	if (!inited) return;
	StopPreviewing();
	StopRecording();
	videomode = VIDEOGRABBERMODE_PREVIEWING;
	capPreview(videowindow, True);
}


void CWinVideoGrabber::StopPreviewing()
{
	if (!inited) return;
	if (videomode == VIDEOGRABBERMODE_PREVIEWING){
	capPreview(videowindow, FALSE);
	videomode = VIDEOGRABBERMODE_NONE;
	}
}

void CWinVideoGrabber::StartRecording(char *filename)
{
	if (!inited) return;
	StopRecording();
	//StopPreviewing();
	CAPTUREPARMS CaptureParms;
	capCaptureGetSetup(videowindow, &CaptureParms, sizeof(CAPTUREPARMS));
	CaptureParms.fYield = True;
	CaptureParms.fAbortLeftMouse = False;
	CaptureParms.fAbortRightMouse = False;
	capCaptureSetSetup(videowindow, &CaptureParms, sizeof (CAPTUREPARMS)); 
	capFileSetCaptureFile(videowindow, filename);
	capCaptureSequence(videowindow);
	videomode = VIDEOGRABBERMODE_RECORDING;
}


void CWinVideoGrabber::StopRecording()
{
	if (!inited)
		return;

	if (videomode == VIDEOGRABBERMODE_RECORDING)
	{
		capCaptureStop(videowindow);
		videomode = VIDEOGRABBERMODE_NONE;
	}
}


void CWinVideoGrabber::VideoFormatDialog()
{
	if (!inited) return;
	CAPDRIVERCAPS CapDrvCaps; 
	capDriverGetCaps(videowindow, &CapDrvCaps, sizeof (CAPDRIVERCAPS)); 
	if (CapDrvCaps.fHasDlgVideoFormat) 
    capDlgVideoFormat(videowindow); 
}

void CWinVideoGrabber::VideoSourceDialog()
{
	if (!inited) return;
	CAPDRIVERCAPS CapDrvCaps; 
	capDriverGetCaps(videowindow, &CapDrvCaps, sizeof (CAPDRIVERCAPS)); 
	if (CapDrvCaps.fHasDlgVideoSource) 
    capDlgVideoSource(videowindow); 
}


void CWinVideoGrabber::VideoDisplayDialog()
{
	if (!inited) return;
	CAPDRIVERCAPS CapDrvCaps; 
	capDriverGetCaps(videowindow, &CapDrvCaps, sizeof (CAPDRIVERCAPS)); 
	if (CapDrvCaps.fHasDlgVideoDisplay) 
    capDlgVideoDisplay(videowindow); 
}

void CWinVideoGrabber::VideoCompressionDialog()
{
	if (!inited) return;
    capDlgVideoCompression(videowindow); 
}

void CWinVideoGrabber::VideoDefaultDialog(void)
{
	VideoSourceDialog();
}

void CWinVideoGrabber::DoIdle()
{
	if (!inited) return;
}

void CWinVideoGrabber::SetRect(short left, short top, short right, short bottom)
{
	destvideorect.top = top;
	destvideorect.left = left;
	destvideorect.bottom = bottom;
	destvideorect.right = right;
	MoveWindow(videowindow,left,top,right-left,bottom-top,TRUE);
}


#ifdef WIN32
Bool CWinVideoGrabber::Draw(int twidth,int theight, HDC hdcMem)
{
	if (!inited) return False;
	HGLOBAL videodib = NULL;
	char *dibdata = NULL;
	char *dibbits = NULL;
	BITMAPINFO *pbmp;
	Bool res = False;
	capEditCopy(videowindow); // Copy the frame in the clipboard as a DIB
	OpenClipboard(parentwindow);
	if (IsClipboardFormatAvailable(CF_DIB)){
		videodib=GetClipboardData(CF_DIB);
		dibdata = (char *)GlobalLock(videodib);
		pbmp = (BITMAPINFO *)dibdata;
		int tsourcewidth  = pbmp->bmiHeader.biWidth;
		int tsourceheight = pbmp->bmiHeader.biHeight;
		int palentries = pbmp->bmiHeader.biClrUsed;
		int bitcount = pbmp->bmiHeader.biBitCount;
		int lenrgb = 0;
		if (bitcount < 24) {
			if (palentries == 0)
				palentries = (1 << bitcount);
			lenrgb = palentries * sizeof(RGBQUAD);
		}
		dibbits = (char *)(dibdata + sizeof(BITMAPINFOHEADER) + lenrgb);
		StretchDIBits(hdcMem,0,0,twidth,theight,0,0,tsourcewidth,tsourceheight,
			(BITMAPINFO *)dibbits, pbmp, DIB_RGB_COLORS,SRCCOPY);
		GlobalUnlock(videodib);
		res = True;
	}
	CloseClipboard();
	return res;
}
#endif


void CWinVideoGrabber::SetFrameSize(int p_width, int p_height)
{
	if (!inited)
		return;
	
	DWORD t_structure_size;
	t_structure_size = capGetVideoFormat(videowindow, NULL, 0);

	BITMAPINFO *t_bitmap_info;
	t_bitmap_info = (BITMAPINFO *)malloc(t_structure_size);

	capGetVideoFormat(videowindow, t_bitmap_info, t_structure_size);

	t_bitmap_info -> bmiHeader . biWidth = p_width;
	t_bitmap_info -> bmiHeader . biHeight = p_height;

	BOOL t_result;
	t_result = capSetVideoFormat(videowindow, t_bitmap_info, t_structure_size);

	free(t_bitmap_info);

	if (videomode == VIDEOGRABBERMODE_PREVIEWING)
	{
		StopPreviewing();
		StartPreviewing();
	}
}

void CWinVideoGrabber::GetFrameSize(int *r_width, int *r_height)
{
	if (!inited)
		return;

	DWORD t_structure_size;
	t_structure_size = capGetVideoFormat(videowindow, NULL, 0);

	BITMAPINFO *t_bitmap_info;
	t_bitmap_info = (BITMAPINFO *)malloc(t_structure_size);

	capGetVideoFormat(videowindow, t_bitmap_info, t_structure_size);

	*r_width = t_bitmap_info -> bmiHeader . biWidth;
	*r_height = t_bitmap_info -> bmiHeader . biHeight;

	free(t_bitmap_info);
}

void CWinVideoGrabber::SetFrameRate(int p_rate)
{
	if (!inited)
		return;

	CAPTUREPARMS t_parameters;
	capCaptureGetSetup(videowindow, &t_parameters, sizeof(CAPTUREPARMS));

	// p_rate is assumed to be in frames per second. We need to convert this to Microseconds per frame for Windows video.
	DWORD t_microsecs_per_frame;
	t_microsecs_per_frame = 1000000 / p_rate;

	t_parameters . dwRequestMicroSecPerFrame = t_microsecs_per_frame;

	BOOL t_result;
	t_result = capCaptureSetSetup(videowindow, &t_parameters, sizeof(CAPTUREPARMS));

}

void CWinVideoGrabber::GetFrameRate(double *r_rate)
{
	if (!inited)
		return;

	CAPTUREPARMS t_parameters;
	capCaptureGetSetup(videowindow, &t_parameters, sizeof(CAPTUREPARMS));

	// Convert the rate from microseconds per frame to frames per second
	double t_frames_per_second;
	t_frames_per_second = (double)(1000000.0 / (double)t_parameters . dwRequestMicroSecPerFrame);

	*r_rate = t_frames_per_second;
}

void CWinVideoGrabber::SetAudioCapture(Bool p_value)
{
	if (!inited)
		return;

	CAPTUREPARMS t_parameters;
	capCaptureGetSetup(videowindow, &t_parameters, sizeof(CAPTUREPARMS));

	t_parameters . fCaptureAudio = p_value;

	BOOL t_result;
	t_result = capCaptureSetSetup(videowindow, &t_parameters, sizeof(CAPTUREPARMS));
}

Bool CWinVideoGrabber::GetAudioCapture(void)
{
	if (!inited)
		return False;

	CAPTUREPARMS t_parameters;
	capCaptureGetSetup(videowindow, &t_parameters, sizeof(CAPTUREPARMS));

	return t_parameters . fCaptureAudio;
}

void CWinVideoGrabber::setAudioFormat(int p_channels, int p_bits_per_sample, int p_sample_rate)
{
	if (!inited)
		return;

	WAVEFORMATEX t_wave_format;
	capGetAudioFormat(videowindow, &t_wave_format, sizeof(WAVEFORMATEX));

	t_wave_format . nChannels = p_channels;
	t_wave_format . wBitsPerSample = p_bits_per_sample;
	t_wave_format . nSamplesPerSec = p_sample_rate;

	BOOL t_result;
	t_result = capSetAudioFormat(videowindow, &t_wave_format, sizeof(t_wave_format));
}
void CWinVideoGrabber::getAudioFormat(int *r_channels, int *r_bits_per_sample, int *r_sample_rate)
{
	if (!inited)
		return;

	WAVEFORMATEX t_wave_format;
	capGetAudioFormat(videowindow, &t_wave_format, sizeof(WAVEFORMATEX));

	*r_channels = t_wave_format . nChannels;
	*r_bits_per_sample = t_wave_format . wBitsPerSample;
	*r_sample_rate = t_wave_format . nSamplesPerSec;
}


void CWinVideoGrabber::setCompressor(char* codecName)
{
}

char *CWinVideoGrabber::getCompressors()
{
	return NULL;
}

void CWinVideoGrabber::GetRect(short *left, short *top, short *right, short *bottom)
{
	*top = destvideorect.top;
	*left = destvideorect.left;
	*bottom = destvideorect.bottom;
	*right = destvideorect.right;
}

void CWinVideoGrabber::AddError(char *serr)
{
}
