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

#ifndef DXVIDEO_H

#define DXVIDEO_H
#include "videograbber.h"

#include <strmif.h>
#include "qedit.h"
#include <dshow.h>

#include "atlsubset.h"

#define MAX_VIDEODEVICE 10
#define MAX_AUDIODEVICE 10
#define MAX_CODEC		50
#define MAX_DEVICENAME_LENGTH 256

// defines for video dialog setting
enum E_VideoDialog
{
	E_VD_NONE = 0,
	E_VD_FORMAT = 0x1,
	E_VD_SOURCE = 0x2,
	E_VD_DISPLAY = 0x4,
	E_VD_COMPRESS = 0x8,
	E_VD_COLROSPACE = 0x10
};


enum E_ColorSpaceFormat
{
	E_CS_DEFAULT = 0,

	E_CS_RGB1    = 1,
	E_CS_RGB4    = 2,
	E_CS_RGB8    = 3,
	E_CS_RGB555  = 4,
	E_CS_RGB565  = 5,
	E_CS_RGB24   = 6,
	E_CS_RGB32   = 7,
	E_CS_ARGB32  = 8,

	E_CS_AYUV	 = 9,
	E_CS_UYVY	 = 10,
	E_CS_Y411	 = 11,
	E_CS_Y41P	 = 12,
	E_CS_Y211	 = 13,
	E_CS_YUY2	 = 14,
	E_CS_YVYU	 = 15, 
	E_CS_YUYV	 = 16,

	E_CS_IF09	 = 17,
	E_CS_IYUV	 = 18,
	E_CS_YV12	 = 19,
	E_CS_YVU9	 = 20,

	E_CS_MAX	= 21
};


#define MAX_VideoProcAmp 10


#ifdef FIREAPI
typedef struct
{
    short usPropertyID;
	float value;
	long flags;
} FIREAPI_CAMERA_PROPERTY_ENTRY;

#define FIREAPI_PROPERTIES 13
#endif

typedef struct tdstVideoDialogSetting{
	BOOL			m_bUseFormat; //for format dialog,
	AM_MEDIA_TYPE	m_mt;		  //StreamConfig interface
	VIDEOINFOHEADER m_viHeader;  
	
	long		m_lVCompCap; //	video compression interface
	long		m_lKeyFrameRate, m_lPFrames;
	DWORDLONG	m_64WindowSize;
	double		m_dQuality;

	BOOL		m_bUseProcAmp;//this is for source dialog 
	long		m_aProcAmplValue[MAX_VideoProcAmp];
	long		m_aProcAmplFlags[MAX_VideoProcAmp];

	LPVOID		m_pVfwCompState; //for vfw compression dialog if there it is
	int			m_lVfwCompCB;



	BOOL		m_bUseDisplay;//for video display dialog
						  //compress dialog in DShow is the codec selection 
						  //and the compress rate is controled by format dialog

	//current codec setting
	TCHAR	m_strCodec[1024];
	TCHAR	m_strAudioCodec[1024];

	//frame rate setting
	BOOL	m_bUseFrameRate;
	double	m_dFrameRate;

	BOOL	m_bUseFrameSize;
	int framewidth;
	int frameheight;

	//save to MPEG2?
	BOOL	m_bSaveMpeg2;

	//Capture audio when recording?
	BOOL	m_bCaptureAudio;
	TCHAR	m_strCurrentVideo[MAX_DEVICENAME_LENGTH];

	//for audio format
	long	Audio_nChannel;			//1:mono, 2:stereo(default)
	long	Audio_nBytesPerSample;	//1:8bit, 2:16bit(default)
	long	Audio_nFrequency;		//11025, 22050, 44100(default);
	BOOL hasfireapiprops;
#ifdef FIREAPI
	FIREAPI_CAMERA_PROPERTY_ENTRY fireapiprops[FIREAPI_PROPERTIES];
#endif
}stVideoDialogSetting;

#define MAX_CameraControl 7

typedef struct tdstVideoDialogExtraSetting
{
	int m_size;
	BOOL m_bUseCameraControl;
	long m_aCameraControlValue[MAX_CameraControl];
	long m_aCameraControlFlags[MAX_CameraControl];
}stVideoDialogExtraSetting;
//

class CDirectXVideoGrabber: public CVideoGrabber
{
public:
	  CDirectXVideoGrabber(HWND whichwindow);
	  CDirectXVideoGrabber();
	  virtual ~CDirectXVideoGrabber();
	  Bool IsInited() {return inited;}
	  void DoIdle();
	  void Init();
	  void SetRect(short left, short top, short right, short bottom);
	  void GetRect(short *left, short *top, short *right, short *bottom);
	   void SetVisible(Bool tvisible);
	  void StartPreviewing();
	  void StopPreviewing();
	  void StartRecording(char *filename);
	  void StopRecording();
	  void GetSettingsString(ExternalString &s);
	  void SetSettingsString(ExternalString &s);
	  void VideoFormatDialog();
	  void VideoSourceDialog();
	  void VideoCameraDialog();
	  void VideoDisplayDialog();
	  void VideoDefaultDialog(void);
	  void AudioDefaultDialog(void) {VideoDefaultDialog();}
	  void VideoCompressionDialog();
	  void AudioCompressionDialog();
	  void AddError(char *serr);

	  BOOL SetColorSpaceFormat(E_ColorSpaceFormat csSet);
#ifdef WIN32
	  Bool Draw(int twidth,int theight, HDC hdcMem);
#endif

protected:
	  //when create graph, some maybe wrong so can't do snapshot
	  Bool CouldSnapshot;

 	  Bool inited;
	  HWND parentwindow;
	  HWND videowindow;
	  Bool buffervideo;
	  RECT destvideorect;
	  RECT srcvideorect;
	  static int winid;


	  //Add by RAC coder
public:
	//Before exit application, should call DeInit()
	void	DeInit();

	//Get available devices, because can't use MFC CString, so have to make it complex
	//if pass an lengtharray == NULL, then only return the number of devices
	//then you can pass in the correct size of string buffer and length array
	void	getAvailableVideos(int *number, int *lengthArray, TCHAR *buffer);
	void	getAvailableAudios(int *number, int *lengthArray, TCHAR *buffer);

	//get all available devices, return a '\n' delimited list of device name.
	//the caller must call  FreeMemory() to free the string buffer
	TCHAR*	getAvailableVideos();
	TCHAR*	getAvailableAudios();


	//Choose a cam/audio device by given names, 
	int		ChooseDevices(TCHAR *szVideo, TCHAR *szAudio);
	//Choose a cam/audio device by given index, 
	int		ChooseDevices(int indexVido, int indexAudio);
	
	//get current selected device
	int		GetCurrentVideo() {return m_nCurrentVideo; };
	TCHAR*  GetCurrentVideoName() {return m_aVideoDeviceNames[m_nCurrentVideo];};
	int		GetCurrentAudio() {return m_nCurrentAudio; };
	TCHAR*  GetCurrentAudioName() {return m_aAudioDeviceNames[m_nCurrentAudio];};


	//Do snapshot to a bmp file. return False if failed, TRUE if success
	BOOL	Snapshot(char * pFilename);
	//Do snapshot to a memory ptr. return False if failed, TRUE if success, 
	//the caller should call FreeMemory() to free the buffer 
	BOOL	Snapshot(DWORD *pdwSize, LPBYTE *pBuff);
	//free memory that is allocated by previous snapshot to memory
	void	FreeMemory(LPBYTE ptr) {free(ptr);};

	long	GetSnapshotWidth(){ return m_lWidth;};
	long	GetSnapshotHeight(){ return m_lHeight;};

	//Set audio capture while recording?
	void	SetAudioCapture(BOOL bSet);
	BOOL	GetAudioCapture(){return m_stSetting.m_bCaptureAudio;};

	//Set a codec by a give name
	void	setCompressor(TCHAR* codecName);
	void	setAudioCompressor(TCHAR* codecName);

	//get all available codec names, return a '\n' delimited list.
	//the caller must call  FreeMemory() to free the string buffer
	TCHAR*	getCompressors();
	TCHAR*	getAudioCompressors();

	TCHAR*  GetCurrentCodecName(){ return m_stSetting.m_strCodec;};
	TCHAR*  GetCurrentAudioCodecName(){ return m_stSetting.m_strAudioCodec;};

	void	setAudioFormat(int channels, int bits, int frequency){
			m_stSetting.Audio_nChannel  = channels;
			m_stSetting.Audio_nBytesPerSample  = bits/8;
			m_stSetting.Audio_nFrequency = frequency;
		};
	void    getAudioFormat(int* channels, int* bits, int* frequency){
			*channels = m_stSetting.Audio_nChannel;
			*bits = m_stSetting.Audio_nBytesPerSample*8;
			*frequency = m_stSetting.Audio_nFrequency;
		};

	//Set Frame Rate
	void	SetFrameRate(int framerate);

	//Play back a avi file in a given window
	BOOL	PlaybackAvi(TCHAR * pFilename, HWND hWnd);

	//Get last error message
	TCHAR*	GetLastErrorMsg() { return m_strLastError;};


	void GetFrameRate(double *frate);
	void GetFrameSize(int *fwidth,int *fheight);
	void SetFrameSize(int fwidth,int fheight);



	VideoGrabberMode GetCurrentMode(){ return videomode;};

	int GetCurrentActualFrameRate();

protected:

	  //Add by RAC coder
    // either the capture live graph, or the capture still graph
    CComPtr< IGraphBuilder > m_pGraph;

    // the playback graph when capturing video
    CComPtr< IGraphBuilder > m_pPlayGraph;

    // the sample grabber for grabbing stills
    CComPtr< ISampleGrabber > m_pGrabber;

	CComPtr< IAMVideoControl >m_pVideoCtrl;

	CComPtr< IPin >			  m_pPin;



	//avaible video devices number and list. Would be get by EnumCamDevice();
	int		m_iNumVCapDevices;
	IMoniker *m_pVideoDevices[MAX_VIDEODEVICE];
	TCHAR	m_aVideoDeviceNames[MAX_VIDEODEVICE][MAX_DEVICENAME_LENGTH];
	//Current video device
	IMoniker *m_pVideo;
	int		m_nCurrentVideo;

	//avaible audio devices number and list. 
	int		m_iNumAudioDevices;
	IMoniker *m_pAudioDevices[MAX_AUDIODEVICE];
	TCHAR	m_aAudioDeviceNames[MAX_AUDIODEVICE][MAX_DEVICENAME_LENGTH];
	//Current audio device
	IMoniker *m_pAudio;
	int		m_nCurrentAudio;

	//available codec
	int		m_iNumCodec;
	TCHAR	m_aCodecNames[MAX_CODEC][MAX_DEVICENAME_LENGTH];

	//available audio codec
	int		m_iNumAudioCodec;
	TCHAR	m_aAudioCodecNames[MAX_CODEC][MAX_DEVICENAME_LENGTH];


	//size of the preview, for snapshot bmp.
	long	m_lWidth;
	long	m_lHeight;

	//videomode before recording
	VideoGrabberMode m_oldvideomode;

	//Last error Message
	TCHAR	m_strLastError[1024];

	//For setting dialog and keep the settings 
	E_VideoDialog	m_vdSetting;

	stVideoDialogSetting m_stSetting;
	stVideoDialogExtraSetting m_stExtraSetting;

	E_ColorSpaceFormat	m_csSet;
	BOOL			    m_csSetOK;
	
	//use streamconfig interface to ask user set video format, and save the setting
	BOOL GetStreamConfigbyDialog(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap );

	//use the user set color space to make a stream config to be used later
	BOOL GetStreamConfigbyColorSpaceSet(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap );
	void ApplyStreamConfig(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap );

	void ApplyAudioFormat(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pACap );

	//use VideoProcAmp interface to ask user set video source, and save the setting
	BOOL GetVideoProcAmpbyDialog(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap );

	void ApplyVideoProcAmp(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap );
	void ApplyCameraControl(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap );

	void FetchVideoProcAmp(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap);
	void FetchCameraControl(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap);


	//part of the constructor
	void PreCreate();

	//Initial a filter graph for Snapshot and video preview
	//This will also be utilized when we do 4 video dialog
    HRESULT InitStillGraph( );

	//Initial a filter graph For AVI capture use
    HRESULT InitCaptureGraph( TCHAR * pFilename );

	//Initial a filter graph For play back use
    HRESULT InitPlaybackGraph( TCHAR * pFilename,  HWND hWnd ); 

	//Get current selected cap device filter
	void	GetCurrentCapDeviceFilter( IBaseFilter ** ppVCap, IBaseFilter ** ppCapAudio  );

	//clear all filter graphs
    void	ClearGraphs(BOOL bPlayBackOnly= FALSE);

	//release all video/audio devices, include the ptr list and current used
	void	ReleaseAllDevice();

	//Release a given IMoniker
	void	IMonRelease(IMoniker *&m_pm);


	//enumerate video/audio devices
	BOOL	EnumCamDevice();
	BOOL	EnumAudioDevice();

	//enumerate codecs
	void	EnumerateCodec();
	void	EnumerateAudioCodec();

	//choose a cam/audio device by given IMonikers, internal use
	void	ChooseDevices(IMoniker *pmVideo, IMoniker *pmAudio);

	//internal error message
    void	Error( TCHAR * pText ){ strcpy(m_strLastError, pText); };
	void	Error( TCHAR * pText, HRESULT hr){ /*sprintf(m_strLastError, "%s Error Code:0x%x", pText, hr);*/ };

	//apply current selected codec filter
	BOOL	GetCurrentCodecFilter(IBaseFilter ** ppVComp );
	BOOL	GetCurrentAudioCodecFilter(IBaseFilter ** ppAComp );

	//change state to wait and preview
	BOOL	WaitAndPreview();

	void GetFormatInfo(int *fwidth, int *fheight, double *fps);
};

	void DeleteMediaType(AM_MEDIA_TYPE *pmt);
	void FreeMediaType(AM_MEDIA_TYPE& mt);

#endif
