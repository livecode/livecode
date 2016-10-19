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
#include <core.h>

#include "dsvideograbber.h"
#include "Vfw.h"


#ifdef FIREAPI
#include "FireAPI/fiint.h"
#include "FireAPI/ficommon.h"



FIREAPI_CAMERA_PROPERTY_ENTRY g_FireAPICameraPropertyEntries[FIREAPI_PROPERTIES] = {
    { FiExpoControl_Autoexp + EXPO_PROPERTIES_OFFSET, 0},
    { FiExpoControl_Shutter + EXPO_PROPERTIES_OFFSET, 0 },
    { FiExpoControl_Gain + EXPO_PROPERTIES_OFFSET, 0 },
    { FiExpoControl_Iris + EXPO_PROPERTIES_OFFSET, 0 },
    { FiColorControl_UB + COLOR_PROPERTIES_OFFSET, 0 },
    { FiColorControl_VR + COLOR_PROPERTIES_OFFSET, 0 },
    { FiColorControl_Hue + COLOR_PROPERTIES_OFFSET, 0 },
    { FiColorControl_Saturation + COLOR_PROPERTIES_OFFSET, 0 },
    { FiBasicControl_Focus + BASIC_PROPERTIES_OFFSET, 0 },
    { FiBasicControl_Zoom + BASIC_PROPERTIES_OFFSET, 0 },
    { FiBasicControl_Brightness + BASIC_PROPERTIES_OFFSET, 0 },
    { FiBasicControl_Sharpness + BASIC_PROPERTIES_OFFSET, 0 },
    { FiBasicControl_Gamma + BASIC_PROPERTIES_OFFSET, 0 }
};
#endif


int CDirectXVideoGrabber::winid = 177;

//! Constructor
CDirectXVideoGrabber::CDirectXVideoGrabber()
{ 
	parentwindow = NULL;
	PreCreate();
}

CDirectXVideoGrabber::CDirectXVideoGrabber(HWND whichwindow)
{
	parentwindow = whichwindow;
	PreCreate();
}

#define videoplayerclass "videoplayerclass"

LRESULT CALLBACK DSVideoWindowProc(HWND hwnd, UINT msg, WPARAM wParam,
				    LPARAM lParam)
{
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

extern HINSTANCE hInstance;

//part of the constructor in fact.
void CDirectXVideoGrabber::PreCreate()
{
	m_vdSetting = E_VD_NONE;
	memset(&m_stSetting, 0, sizeof(stVideoDialogSetting));
	memset(&m_stExtraSetting, 0, sizeof(stVideoDialogExtraSetting));
	
	m_stSetting.m_bCaptureAudio = 1;
	m_stSetting.Audio_nChannel = 2;
	m_stSetting.Audio_nBytesPerSample = 2;
	m_stSetting.Audio_nFrequency = 44100;
	
	
	destvideorect.top = destvideorect.left = 0;
	destvideorect.bottom = destvideorect.right = 100;
	buffervideo = True;
	videomode = VIDEOGRABBERMODE_NONE;
	inited = 0;
	
	m_iNumVCapDevices = 0;
	m_pVideo = NULL;
	ZeroMemory(m_pVideoDevices, sizeof(m_pVideoDevices));
	m_nCurrentVideo  = -1;
	
	
	m_iNumAudioDevices = 0;
	m_pAudio = NULL;
	ZeroMemory(m_pAudioDevices, sizeof(m_pAudioDevices));
	m_nCurrentAudio  = -1;
	
	visible = 0;
	
	m_iNumCodec = 0;
	m_iNumAudioCodec = 0;
	
	strcpy(m_strLastError, TEXT(""));
	
	
	WNDCLASS    wc;
	
	
	wc.style         = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW |  CS_OWNDC;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 4;
	wc.hInstance     = hInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	
	// Define the VIDEO CLIP window. Has its own DC
	wc.lpfnWndProc   = (WNDPROC)DSVideoWindowProc;
	wc.lpszClassName = videoplayerclass;  //video class
	
	
	RegisterClass(&wc);
	

	//create a video window
	videowindow =CreateWindow(
			videoplayerclass,    // Class name
			"CAPTURE", // window name if pop-up 
			SS_WHITERECT | (visible?WS_VISIBLE:0) | (parentwindow? WS_CHILD:0),		//style bits
			destvideorect.left, destvideorect.top,       // Position
			destvideorect.right-destvideorect.left,  destvideorect.bottom - destvideorect.top,  // Size
			parentwindow,             // Parent window (no parent)
			(HMENU)NULL,            // use class menu
			NULL,                  // handle to window instance
			(LPSTR)NULL             // no params to pass on
			);
	
	//initialize 
	Init();
	
	//enumerate all cdoec
	EnumerateCodec();
	EnumerateAudioCodec();
}


//! CDirectXVideoGrabber DeConstructor: 
/*
	Release resource and deconstruct
*/
CDirectXVideoGrabber::~CDirectXVideoGrabber()
{
	// OK-2007-12-19 : Bug 5693, stop any previewing or recording before closing.
	if (videomode == VIDEOGRABBERMODE_RECORDING)
		StopRecording();

	if (videomode == VIDEOGRABBERMODE_PREVIEWING || videomode == VIDEOGRABBERMODE_RECORDING)
		StopPreviewing();

	DeInit();
	DestroyWindow(videowindow);
}


//! Init: Init the class obj with an hwnd
/* 
	This function must be called before use the class object
	Return TRUE if success, FALSE if fail
*/
void CDirectXVideoGrabber::Init()
{
	Error( TEXT(""));

	//if hadn't inited, enumerate device list, and select the default device
	if(!inited){
		EnumAudioDevice();

		if( EnumCamDevice() == FALSE  || ChooseDevices(0, 0) == -1)
			return;
		inited =  TRUE;
	}
	if(visible)
		StartPreviewing();
	
	CouldSnapshot = TRUE;
	return;
}



//! DeInit: Deinitialize all resources
/*
	Before exit application, should call DeInit()
*/
void CDirectXVideoGrabber::DeInit()
{
	Error( TEXT(""));

	//clear filter graph
    ClearGraphs( );

	//release devices
	ReleaseAllDevice();

    videomode = VIDEOGRABBERMODE_NONE;

	strcpy(m_strLastError, "");

    videowindow = NULL;

	if(m_stSetting.m_pVfwCompState)
		free(m_stSetting.m_pVfwCompState);
	memset(&m_stSetting, 0, sizeof(stVideoDialogSetting) );
	m_stSetting.m_bCaptureAudio = 1;
	m_stSetting.Audio_nChannel = 2;
	m_stSetting.Audio_nBytesPerSample = 2;
	m_stSetting.Audio_nFrequency = 44100;

	inited = false;
}




//! ReleaseVideoDevice: Release all video/Audio devices, include the ptr list and current used
void CDirectXVideoGrabber::ReleaseAllDevice()
{
	int i; 
    IMonRelease(m_pVideo);
    for(i = 0; i < NUMELMS(m_pVideoDevices); i++) {
        IMonRelease(m_pVideoDevices[i]);
    }

	IMonRelease(m_pAudio);
	for(i = 0; i < NUMELMS(m_pAudioDevices); i++) {
        IMonRelease(m_pAudioDevices[i]);
    }
	
}


//! IMonRelease: Release a given IMoniker
void CDirectXVideoGrabber::IMonRelease(IMoniker *&m_pm) 
{
    if(m_pm)	{
        m_pm->Release();
        m_pm = 0;
    }
}



//! ChooseDevices: Choose cam/audio device by given IMonikers
void CDirectXVideoGrabber::ChooseDevices(IMoniker *pmVideo, IMoniker *pmAudio)
{
    USES_CONVERSION;
	#define VERSIZE 40
	#define DESCSIZE 80
    int versize = VERSIZE;
    int descsize = DESCSIZE;
    WCHAR wachVer[VERSIZE]={0}, wachDesc[DESCSIZE]={0};
    TCHAR tachStatus[VERSIZE + DESCSIZE + 5]={0};

    // IF chose a new device. rebuild the graphs
    if(m_pVideo != pmVideo) {
        if(pmVideo) {
            pmVideo->AddRef();
        }
        
        IMonRelease(m_pVideo);
        m_pVideo = pmVideo;

		if( inited ){
			ClearGraphs();
			Init();
		}
    }

    if(m_pAudio != pmAudio ){
        if(pmAudio) {
            pmAudio->AddRef();
        }
        
        IMonRelease(m_pAudio);
        m_pAudio = pmAudio;
    }
}


//! ChooseDevices: Choose cam/audio device by given index
/*
	This should be called after EnumCamDevice.
	If no matched device IMoniker is found, then use #0 as default

	return the index of the camarray the of actually selected cam device.
				-1 means failed to select any cam device
*/					
int CDirectXVideoGrabber::ChooseDevices(int indexVideo, int indexAudio)
{
	int nSelectedVideo, nSelectedAudio;

	Error( TEXT(""));

	//nothing could be select if previous detection failed
	if(m_iNumVCapDevices <=0)
		return -1;

	//select #0 video device if anything wrong
	if(indexVideo>= m_iNumVCapDevices || indexVideo<0 || !m_pVideoDevices[indexVideo])
		nSelectedVideo = 0;
	else
		nSelectedVideo = indexVideo;

	//should check if video device is valid
	if(!m_pVideoDevices[nSelectedVideo])
		return -1;

	//select #0 audio device if anything wrong
	if(indexAudio >= m_iNumAudioDevices || indexAudio <0 || !m_pAudioDevices[indexAudio ])
		nSelectedAudio = 0;
	else
		nSelectedAudio = indexAudio ;

	//keep current device name
	m_nCurrentVideo = nSelectedVideo;
	strcpy(m_stSetting.m_strCurrentVideo, m_aVideoDeviceNames[m_nCurrentVideo]);
	m_nCurrentAudio = nSelectedAudio;
	
	//needn't valid audio cap device, since it is not as important as video capture
    ChooseDevices(m_pVideoDevices[nSelectedVideo], m_pAudioDevices[nSelectedAudio]);

	return nSelectedVideo;
}


/*
	This should be called after EnumCamDevice.
	If no matched device IMoniker is found, then use #0 as default

	return the index of the camarray the of actually selected cam device.
				-1 means failed to select any cam device
*/	
int	CDirectXVideoGrabber::ChooseDevices(TCHAR *szVideo, TCHAR *szAudio)
{
	int index;
	int nSelectedVideo, nSelectedAudio;

	Error( TEXT(""));

	//for video device, 
	nSelectedVideo = 0;
	for(index=0; index <m_iNumVCapDevices; index++){
		if(strcmp(szVideo, m_aVideoDeviceNames[index]) == 0){
			nSelectedVideo = index;
			break;
		}
	}

	//should check if video device is valid
	if(!m_pVideoDevices[nSelectedVideo])
		return -1;

	//for video device, 
	nSelectedAudio = 0;
	for(index=0; index <m_iNumAudioDevices; index++){
		if(strcmp(szAudio, m_aAudioDeviceNames[index]) == 0){
			nSelectedAudio= index;
			break;
		}
	}

	//keep current device name
	m_nCurrentVideo = nSelectedVideo;
	strcpy(m_stSetting.m_strCurrentVideo, m_aVideoDeviceNames[m_nCurrentVideo]);
	m_nCurrentAudio = nSelectedAudio;

	//needn't valid audio cap device, since it is not as important as video capture
    ChooseDevices(m_pVideoDevices[nSelectedVideo], m_pAudioDevices[nSelectedAudio]);

	return nSelectedVideo;
}



//! getAvailableVideos, Get available video devices, 
/*
	number, [out], number of devices
	lengthArray, [in/out], an integer array, for each element is the length of a device string
	buffer [in/out], 

	this is used to pass out a string array, urgly!!!
	
	if pass an lengthArray == NULL, then only return the number of devices
	then you can pass the correct string buffer and length array  
*/
void CDirectXVideoGrabber::getAvailableVideos(int *number, int *lengthArray, TCHAR *buffer)
{
	*number = m_iNumVCapDevices;
	if(lengthArray){
		for(int index = 0; index <*number ; index++){
			*lengthArray = strlen(m_aVideoDeviceNames[index]) +1;
			strcpy(buffer, m_aVideoDeviceNames[index]);
			buffer+= *lengthArray++;
		}
	}
}


//! Get available audio devices, 
/*  
	number, [out], number of devices
	lengthArray, [out], an integer array, for each element is the length of a device string
	buffer [out], 

	this is used to pass out a string array, urgly!!!
	
	if pass an lengthArray == NULL, then only return the number of devices
	then you can pass the correct string buffer and length array  
*/
void CDirectXVideoGrabber::getAvailableAudios(int *number, int *lengthArray, TCHAR *buffer)
{
	*number = m_iNumAudioDevices;
	if(lengthArray){
		for(int index = 0; index <*number ; index++){
			*lengthArray = strlen(m_aAudioDeviceNames[index])+1;
			strcpy(buffer, m_aAudioDeviceNames[index]);
			buffer+= *lengthArray++;
		}
	}
}


//! getAvailableVideos: get all available video device, return a '\n' delimited name list.
//the caller must call  FreeMemory() to free the string buffer
TCHAR*	CDirectXVideoGrabber::getAvailableVideos()
{
	Error( TEXT(""));
	if(!inited)	{
		Error(TEXT("Not inited."));
		return FALSE;
	}
	
	long size = 0;
	for(int i= 0; i<m_iNumVCapDevices; i++)
		size += strlen(m_aVideoDeviceNames[i])+1;

	if(size==0)
		return NULL;

	TCHAR *buff = (TCHAR*)malloc(size);
	TCHAR *ptr = buff;
	for(int i= 0; i<m_iNumVCapDevices; i++) {
		strcpy(ptr, m_aVideoDeviceNames[i]);
		ptr[strlen(m_aVideoDeviceNames[i])] = '\n';
		ptr+= strlen(m_aVideoDeviceNames[i])+1;
	}
	ptr[-1]= '\0';

	return buff;
}


//! getAvailableAudios: get all available audio device, return a '\n' delimited name list.
//the caller must call  FreeMemory() to free the string buffer
TCHAR*	CDirectXVideoGrabber::getAvailableAudios()
{
	Error( TEXT(""));
	if(!inited)	{
		Error(TEXT("Not inited."));
		return FALSE;
	}
	
	long size = 0;
	for(int i= 0; i<m_iNumAudioDevices; i++)
		size += strlen(m_aAudioDeviceNames[i])+1;

	if(size==0)
		return NULL;

	TCHAR *buff = (TCHAR*)malloc(size);
	TCHAR *ptr = buff;
	for(int i= 0; i<m_iNumAudioDevices; i++) {
		strcpy(ptr, m_aAudioDeviceNames[i]);
		ptr[strlen(m_aAudioDeviceNames[i])] = '\n';
		ptr+= strlen(m_aAudioDeviceNames[i])+1;
	}
	ptr[-1]= '\0';

	return buff;
}



//! EnumCamDevice: enumerate all useable cam device, and keep a name list
//  Return FALSE if no video device or error
BOOL CDirectXVideoGrabber::EnumCamDevice()
{
    USES_CONVERSION;
    int uIndex = 0;
    HRESULT hr;
	m_iNumVCapDevices = 0;

	//release previous video device ptr list
    for(int i = 0; i < NUMELMS(m_pVideoDevices); i++) {
        IMonRelease(m_pVideoDevices[i]);
    }

	// enumerate all video capture devices
    ICreateDevEnum *m_pCreateDevEnum;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, 
		IID_ICreateDevEnum, (void**)&m_pCreateDevEnum);
    if(hr != NOERROR) {
        Error("Error Creating Device Enumerator");
        return FALSE;
    }

    IEnumMoniker *m_pEm;
    hr = m_pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&m_pEm, 0);
	m_pCreateDevEnum->Release();

    if(hr != NOERROR)  {
        Error("Sorry, you have no video capture hardware");
        return FALSE; 
    }
    m_pEm->Reset();
    ULONG cFetched;
    IMoniker *m_pM;
    while(hr = m_pEm->Next(1, &m_pM, &cFetched), hr==S_OK)
	{
        IPropertyBag *m_pBag;
        hr = m_pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&m_pBag);
        if(SUCCEEDED(hr)){
            VARIANT var;
            var.vt = VT_BSTR;
            hr = m_pBag->Read(L"FriendlyName", &var, NULL);
			if(hr == NOERROR){
				//add string name to name list
				strcpy(m_aVideoDeviceNames[uIndex], W2T(var.bstrVal));
                SysFreeString(var.bstrVal);

				//add to m_pVideoDevices array
				//ASSERT(m_pVideoDevices[uIndex] == 0);
                m_pVideoDevices[uIndex] = m_pM;
                m_pM->AddRef();
            }
            m_pBag->Release();
        }
        m_pM->Release();
        uIndex++;
		if(uIndex >=MAX_VIDEODEVICE)
			break;
    }
    m_pEm->Release();
	if(uIndex == 0)  {
        Error("Sorry, you have no video capture hardware");
        return FALSE; 
	}

	//case the user plug more than one same camera in different usb port.
	for(int i = 0; i< uIndex-1; i++){
		int append = 0;
		for (int j= i+1; j<uIndex; j++) {
			if(strcmp(m_aVideoDeviceNames[i], m_aVideoDeviceNames[j]) == 0){
				char str[100];
				append++;
				itoa(append, str, 10);
				strcat(m_aVideoDeviceNames[j], str);
			}
		}
		if(append!=0)
			strcat(m_aVideoDeviceNames[i], "0");
	}

	m_iNumVCapDevices = uIndex;    
	return TRUE;
}


//! EnumAudioDevice: enumerate all useable audio device, and keep a name list
//  Return FALSE if no audio device or error
BOOL CDirectXVideoGrabber::EnumAudioDevice()
{
    USES_CONVERSION;
    UINT    uIndex = 0;
    HRESULT hr;
	m_iNumAudioDevices = 0;

	//release previous audio device ptr list
    for(int i = 0; i < NUMELMS(m_pAudioDevices); i++) {
        IMonRelease(m_pAudioDevices[i]);
    }

	// enumerate all video capture devices
    ICreateDevEnum *m_pCreateDevEnum;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (void**)&m_pCreateDevEnum);
    if(hr != NOERROR) {
        Error("Error Creating Device Enumerator");
        return FALSE;
    }

    IEnumMoniker *m_pEm;
    hr = m_pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &m_pEm, 0);
	m_pCreateDevEnum->Release();
    if(hr != NOERROR)  {
        Error("Sorry, you have no audio capture hardware");
        return FALSE; 
    }

    m_pEm->Reset();
    ULONG cFetched;
    IMoniker *m_pM;
    while(hr = m_pEm->Next(1, &m_pM, &cFetched), hr==S_OK)
	{
        IPropertyBag *m_pBag;
        hr = m_pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&m_pBag);
        if(SUCCEEDED(hr)){
            VARIANT var;
            var.vt = VT_BSTR;
            hr = m_pBag->Read(L"FriendlyName", &var, NULL);
			if(hr == NOERROR){
				//add string name to name list
				strcpy(m_aAudioDeviceNames[uIndex], W2T(var.bstrVal));
                SysFreeString(var.bstrVal);

				//add to m_pVideoDevices array
				//ASSERT(m_pAudioDevices[uIndex] == 0);
                m_pAudioDevices[uIndex] = m_pM;
                m_pM->AddRef();
            }
            m_pBag->Release();
        }
        m_pM->Release();
        uIndex++;
		if(uIndex >=MAX_VIDEODEVICE)
			break;
    }
    m_pEm->Release();

	if(uIndex == 0)  {
        Error("Sorry, you have no audio capture hardware");
        return FALSE; 
    }

	m_iNumAudioDevices = uIndex;
	return TRUE;
}



//Get current selected cap device filter
void CDirectXVideoGrabber::GetCurrentCapDeviceFilter( IBaseFilter ** ppVCap, IBaseFilter ** ppCapAudio )
{
	HRESULT hr;
    *ppVCap = NULL;
	*ppCapAudio = NULL;

    if(m_pVideo != 0) {
        hr = m_pVideo->BindToObject(0, 0, IID_IBaseFilter, (void**)ppVCap);
    }

	if(m_pAudio != 0 && m_stSetting.m_bCaptureAudio) {
        hr = m_pAudio ->BindToObject(0, 0, IID_IBaseFilter, (void**)ppCapAudio);
    }
}




//! GetCurrentCodecFilter: get current select codec filter
BOOL CDirectXVideoGrabber::GetCurrentCodecFilter(IBaseFilter ** ppVComp )
{
	USES_CONVERSION;
	ICreateDevEnum *pSysDevEnum = NULL;
	IEnumMoniker *pEnumCat = NULL;
	IMoniker *pMoniker;
	ULONG cFetched;
	HRESULT hr;

	*ppVComp = 0;

	BOOL	bCompFilter = FALSE;

	// Get a list of video codecs and select one
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum,(void **)&pSysDevEnum);
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoCompressorCategory, &pEnumCat, 0);
	pSysDevEnum->Release();

	while(pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
	{
		IPropertyBag *pPropBag;
		hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if(SUCCEEDED(hr))
		{
			VARIANT varName;
			varName.vt = VT_BSTR;
			hr = pPropBag->Read(L"FriendlyName", &varName, 0);
			if(SUCCEEDED(hr))
			{
				//check if selected codec
				if (lstrcmpi(W2T(varName.bstrVal), m_stSetting.m_strCodec) == 0)
				{
					hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)ppVComp);
					IAMVfwCompressDialogs *setupd;
					(*ppVComp)->QueryInterface(IID_IAMVfwCompressDialogs, (void **)&setupd);
					
					// This is the right filter.
					if(m_vdSetting != E_VD_COMPRESS) {
						hr = m_pGraph->AddFilter(*ppVComp,L"Encoder");
						if( FAILED( hr ) )
							Error( TEXT("Could not put selected codec in graph"));
						else
							bCompFilter = TRUE;
						//set vfw compression if there is one
						if(setupd && m_stSetting.m_lVfwCompCB && m_stSetting.m_pVfwCompState)
						{
							hr = setupd->SetState(m_stSetting.m_pVfwCompState, m_stSetting.m_lVfwCompCB);
							hr = setupd->GetState(m_stSetting.m_pVfwCompState, &m_stSetting.m_lVfwCompCB);
						}
					}
					else if(m_vdSetting == E_VD_COMPRESS ) {
						if(setupd){
							if(setupd && m_stSetting.m_lVfwCompCB && m_stSetting.m_pVfwCompState)
								setupd->SendDriverMessage(ICM_SETSTATE, (WPARAM)m_stSetting.m_pVfwCompState, 
								m_stSetting.m_lVfwCompCB);
							//popup a vfw dialog for setting
							hr = setupd->ShowDialog(VfwCompressDialog_QueryConfig,NULL);
							if( hr == S_OK)
							{
								hr = setupd->ShowDialog(VfwCompressDialog_Config, NULL);
								bCompFilter = TRUE;
								//fetch the dialog setting
								int CB=0;
								hr = setupd->GetState(NULL, &CB);
								if(CB>0) {
									if(m_stSetting.m_pVfwCompState)
										free(m_stSetting.m_pVfwCompState);
									m_stSetting.m_pVfwCompState = malloc(CB);
									hr = setupd->GetState(NULL, &m_stSetting.m_lVfwCompCB);
									hr = setupd->GetState(m_stSetting.m_pVfwCompState, &m_stSetting.m_lVfwCompCB);
									if(hr!=S_OK){
										free(m_stSetting.m_pVfwCompState); m_stSetting.m_pVfwCompState = 0;
										m_stSetting.m_lVfwCompCB = 0;
									}
								}
							//	hr = setupd->ShowDialog(VfwCompressDialog_Config, NULL);
							}
						}
						else {
							ISpecifyPropertyPages *pProp;
							HRESULT hr = (*ppVComp)->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);
							if (SUCCEEDED(hr)) 
							{
								// Get the filter's name and IUnknown pointer.
								IUnknown *pFilterUnk = NULL;
								(*ppVComp)->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);
								
								// Show the page. 
								CAUUID caGUID;
								pProp->GetPages(&caGUID);
								pProp->Release();
								
								OleCreatePropertyFrame(
									NULL,                   // Parent window
									0, 0,                   // (Reserved)
									L"Codec ",				// Caption for the dialog box
									1,                      // Number of objects (just the filter)
									&pFilterUnk,            // Array of object pointers. 
									caGUID.cElems,          // Number of property pages
									caGUID.pElems,          // Array of property page CLSIDs
									0,                      // Locale identifier
									0, NULL                 // Reserved
									);
								
								// Clean up.
								pFilterUnk->Release();
								CoTaskMemFree(caGUID.pElems);
							}
						}
					}
					//free resource and break
					if(setupd) setupd->Release();
					SysFreeString(varName.bstrVal);
					pPropBag->Release();
					pMoniker->Release();
					break;
				}
				SysFreeString(varName.bstrVal);
            }
			pPropBag->Release();
        }
		pMoniker->Release();
	}

	pEnumCat->Release();

	if(bCompFilter)
	    return TRUE;
	else
		return FALSE;
}


//! setCompressor: Set codec by a name
void CDirectXVideoGrabber::setCompressor(TCHAR* codecName)
{
	strcpy(m_stSetting.m_strCodec, codecName);
}


//! getCompressors: get all available codec names, return a '\n' delimited list.
//the caller must call  FreeMemory() to free the string buffer
TCHAR*	CDirectXVideoGrabber::getCompressors()
{
	Error( TEXT(""));
	if(!inited)	{
		Error(TEXT("Not inited."));
		return FALSE;
	}
	
	//first check the memory size that need to be malloce
	//index = 0 should be a "NONE" codec, so start from index 1
	long size = 0;
	for(int i= 1; i<m_iNumCodec; i++)
		size += strlen(m_aCodecNames[i])+1;

	if(size==0)
		return NULL;

	TCHAR *buff = (TCHAR*)malloc(size);
	TCHAR *ptr = buff;
	for(int i= 1; i<m_iNumCodec; i++)
	{
		strcpy(ptr, m_aCodecNames[i]);
		ptr[strlen(m_aCodecNames[i])] = '\n';
		ptr+= strlen(m_aCodecNames[i])+1;
	}
	ptr[-1]= '\0';
		
	return buff;
}

//! EnumerateCodec: enumerate all available codec
void CDirectXVideoGrabber::EnumerateCodec()
{
	USES_CONVERSION;
	ICreateDevEnum *pSysDevEnum = NULL;
	IEnumMoniker *pEnumCat = NULL;
	IMoniker *pMoniker;
	ULONG cFetched;
	HRESULT hr;

	m_iNumCodec = 0;
	strcpy(m_aCodecNames[m_iNumCodec++], TEXT("NONE"));

	// Get a list of video codecs and select one
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum,(void **)&pSysDevEnum);
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoCompressorCategory, &pEnumCat, 0);
	while(pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
	{
		IPropertyBag *pPropBag;
		hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if(SUCCEEDED(hr))
		{
			VARIANT varName;
			varName.vt = VT_BSTR;
			hr = pPropBag->Read(L"FriendlyName", &varName, 0);
			if(SUCCEEDED(hr))
			{
				//add string name to name list
				strcpy(m_aCodecNames[m_iNumCodec++], W2A(varName.bstrVal));
                SysFreeString(varName.bstrVal);
            }
			pPropBag->Release();
        }
		pMoniker->Release();
	}

	pEnumCat->Release();
	pSysDevEnum->Release();
}



//! GetCurrentAudioCodecFilter: get current select codec filter
//for audio compressor, to make things quick and easy, I didn't make the codec a class. 
BOOL CDirectXVideoGrabber::GetCurrentAudioCodecFilter(IBaseFilter ** ppVComp )
{
	USES_CONVERSION;
	ICreateDevEnum *pSysDevEnum = NULL;
	IEnumMoniker *pEnumCat = NULL;
	IMoniker *pMoniker;
	ULONG cFetched;
	HRESULT hr;

	*ppVComp = 0;

	BOOL	bCompFilter = FALSE;
	// Get a list of video codecs and select one
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum,(void **)&pSysDevEnum);
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_AudioCompressorCategory, &pEnumCat, 0);
	pSysDevEnum->Release();

	while(pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
	{
		IPropertyBag *pPropBag;
		hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if(SUCCEEDED(hr))
		{
			VARIANT varName;
			varName.vt = VT_BSTR;
			hr = pPropBag->Read(L"FriendlyName", &varName, 0);
			if(SUCCEEDED(hr))
			{
				//check if selected codec
				if (lstrcmpi(W2T(varName.bstrVal), m_stSetting.m_strAudioCodec) == 0)
				{
					hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)ppVComp);
					// This is the right filter.
						hr = m_pGraph->AddFilter(*ppVComp,L"Audio Encoder");
						if( FAILED( hr ) )
							Error( TEXT("Could not put selected audio codec in graph"));
						else
							bCompFilter = TRUE;
					//free resource and break
					SysFreeString(varName.bstrVal);
					pPropBag->Release();
					pMoniker->Release();
					break;
				}
				SysFreeString(varName.bstrVal);
            }
			pPropBag->Release();
        }
		pMoniker->Release();
	}

	pEnumCat->Release();

	if(bCompFilter)
	    return TRUE;
	else
		return FALSE;
}



//! setAudioCompressor: Set codec by a name
void CDirectXVideoGrabber::setAudioCompressor(TCHAR* codecName)
{
	strcpy(m_stSetting.m_strAudioCodec, codecName);
}


//! getAudioCompressors: get all available codec names, return a '\n' delimited list.
//the caller must call  FreeMemory() to free the string buffer
TCHAR*	CDirectXVideoGrabber::getAudioCompressors()
{
	Error( TEXT(""));
	if(!inited)	{
		Error(TEXT("Not inited."));
		return FALSE;
	}
	
	//first check the memory size that need to be malloce
	//index = 0 should be a "NONE" codec, so start from index 1
	long size = 0;
	for(int i= 1; i<m_iNumAudioCodec; i++)
		size += strlen(m_aAudioCodecNames[i])+1;

	if(size==0)
		return NULL;

	TCHAR *buff = (TCHAR*)malloc(size);
	TCHAR *ptr = buff;
	for(int i= 1; i<m_iNumAudioCodec; i++)
	{
		strcpy(ptr, m_aAudioCodecNames[i]);
		ptr[strlen(m_aAudioCodecNames[i])] = '\n';
		ptr+= strlen(m_aAudioCodecNames[i])+1;
	}
	ptr[-1]= '\0';
		
	return buff;
}

//! EnumerateAudioCodec: enumerate all available audio codec 
void CDirectXVideoGrabber::EnumerateAudioCodec()
{
	USES_CONVERSION;
	ICreateDevEnum *pSysDevEnum = NULL;
	IEnumMoniker *pEnumCat = NULL;
	IMoniker *pMoniker;
	ULONG cFetched;
	HRESULT hr;

	m_iNumAudioCodec= 0;
	strcpy(m_aAudioCodecNames[m_iNumAudioCodec++], TEXT("NONE"));

	// Get a list of video codecs and select one
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum,(void **)&pSysDevEnum);
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_AudioCompressorCategory, &pEnumCat, 0);
	while(pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
	{
		IPropertyBag *pPropBag;
		hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if(SUCCEEDED(hr))
		{
			VARIANT varName;
			varName.vt = VT_BSTR;
			hr = pPropBag->Read(L"FriendlyName", &varName, 0);
			if(SUCCEEDED(hr))
			{
				//add string name to name list
				strcpy(m_aAudioCodecNames[m_iNumAudioCodec++], W2A(varName.bstrVal));
                SysFreeString(varName.bstrVal);
            }
			pPropBag->Release();
        }
		pMoniker->Release();
	}

	pEnumCat->Release();
	pSysDevEnum->Release();
}




//! GetStreamConfigbyColorSpaceSet: use the user set color space to make a stream config to be used later
BOOL CDirectXVideoGrabber::GetStreamConfigbyColorSpaceSet(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap )
{
	GUID subtype;
	switch (m_csSet)
	{
	case E_CS_RGB1:
		subtype = MEDIASUBTYPE_RGB1;
		break;
	case E_CS_RGB4:
		subtype = MEDIASUBTYPE_RGB4;
		break;
	case E_CS_RGB8:
		subtype = MEDIASUBTYPE_RGB8;
		break;
	case E_CS_RGB555:
		subtype = MEDIASUBTYPE_RGB555;
		break;
	case E_CS_RGB565:
		subtype = MEDIASUBTYPE_RGB565;
		break;

	case E_CS_RGB24:
		subtype = MEDIASUBTYPE_RGB24;
		break;
	case E_CS_RGB32:
		subtype = MEDIASUBTYPE_RGB32;
		break;
	case E_CS_ARGB32:
		subtype = MEDIASUBTYPE_ARGB32;
		break;

	case 	E_CS_AYUV:
		subtype = MEDIASUBTYPE_AYUV;
		break;
	case	E_CS_UYVY:
		subtype = MEDIASUBTYPE_UYVY;
		break;
	case 	E_CS_Y411:
		subtype = MEDIASUBTYPE_Y411;
		break;
	case 	E_CS_Y41P:
		subtype = MEDIASUBTYPE_Y41P;
		break;
	case 	E_CS_Y211:
		subtype = MEDIASUBTYPE_Y211;
		break;
	case 	E_CS_YUY2:
		subtype = MEDIASUBTYPE_YUY2;
		break;
	case 	E_CS_YVYU:
		subtype = MEDIASUBTYPE_YVYU;
		break;
	case 	E_CS_YUYV:
		subtype = MEDIASUBTYPE_YUYV;
		break;

	case 	E_CS_IF09:
		subtype = MEDIASUBTYPE_IF09;
		break;
	case 	E_CS_IYUV:
		subtype = MEDIASUBTYPE_IYUV;
		break;
	case 	E_CS_YV12:
		subtype = MEDIASUBTYPE_YV12;
		break;
	case 	E_CS_YVU9:
		subtype = MEDIASUBTYPE_YVU9;
		break;

	}

	m_csSetOK = FALSE;
	{
		HRESULT hr;
		//find streamconfig interface
		IAMStreamConfig *pSC;
		hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
					pVCap, IID_IAMStreamConfig, (void **)&pSC);
		if(hr != NOERROR)
			hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
					pVCap, IID_IAMStreamConfig, (void **)&pSC);

		if(hr != NOERROR)
		{
			Error( TEXT("Cannot find VCapture:IAMStreamConfig.") );
			return FALSE;
		}

		if(hr == S_OK) {
				//ok, fetch the user setting
				if(pSC) {
					AM_MEDIA_TYPE *pmt;
					// get format being used NOW
					hr = pSC->GetFormat(&pmt);
					// DV capture does not use a VIDEOINFOHEADER,
					if(hr == NOERROR) {
						if(pmt->formattype == FORMAT_VideoInfo) {
							VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
							if(m_csSet!=E_CS_DEFAULT)
								pmt->subtype = subtype;
							hr = pSC->SetFormat(pmt);
							if(hr == NOERROR)
							{
								//if already set Format, overwrite it
								if(m_stSetting.m_bUseFormat)
								{
									m_stSetting.m_mt.subtype = subtype;
								}
								else
								{
									//valid format control
									m_stSetting.m_bUseFormat = TRUE;
									memcpy(&m_stSetting.m_mt, pmt, sizeof(AM_MEDIA_TYPE));
									memcpy(&m_stSetting.m_viHeader, pvi, sizeof(VIDEOINFOHEADER));
								}
								m_csSetOK = TRUE;
							}
							else
							{
								Error( TEXT("Cannot set the new Color Space Format, maybe device does not support.") );
							}
						}
						DeleteMediaType(pmt);
					}
				}
		}
		pSC->Release();
	}

	return m_csSetOK;
}


//! GetStreamConfigbyDialog: use streamconfig interface to ask user set video format, and save the setting
BOOL CDirectXVideoGrabber::GetStreamConfigbyDialog(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap )
{
    HRESULT hr;
	//find streamconfig interface
	IAMStreamConfig *pSC;
	hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMStreamConfig, (void **)&pSC);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMStreamConfig, (void **)&pSC);

	if(hr != NOERROR)
	{
		Error( TEXT("Cannot find VCapture:IAMStreamConfig.") );
		return FALSE;
	}


	//find videocompression
	IAMVideoCompression* pVComp;
    hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMVideoCompression, (void **)&pVComp);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMVideoCompression, (void **)&pVComp);

	ISpecifyPropertyPages *pSpec;
	CAUUID cauuid;
	hr = pSC->QueryInterface(IID_ISpecifyPropertyPages,
		(void **)&pSpec);
	if(hr == S_OK) {
			hr = pSpec->GetPages(&cauuid);
			hr = OleCreatePropertyFrame(NULL, 30, 30, NULL, 1,
				(IUnknown **)&pSC, cauuid.cElems,
				(GUID *)cauuid.pElems, 0, 0, NULL);

			//ok, fetch the user setting
			if(pSC) {
				AM_MEDIA_TYPE *pmt;
				// get format being used NOW
				hr = pSC->GetFormat(&pmt);
				// DV capture does not use a VIDEOINFOHEADER,
				if(hr == NOERROR) {
					if(pmt->formattype == FORMAT_VideoInfo) {
						VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
						//valid format control
						m_stSetting.m_bUseFormat = TRUE;
						memcpy(&m_stSetting.m_mt, pmt, sizeof(AM_MEDIA_TYPE));
						memcpy(&m_stSetting.m_viHeader, pvi, sizeof(VIDEOINFOHEADER));
					}
					DeleteMediaType(pmt);
				}
			}

			//video compression
			if(pVComp) {
				hr = pVComp->GetInfo(0, 0, 0, 0, 0, 0, 0, &m_stSetting.m_lVCompCap);
				pVComp->get_KeyFrameRate(&m_stSetting.m_lKeyFrameRate);
				pVComp->get_PFramesPerKeyFrame(&m_stSetting.m_lPFrames);
				pVComp->get_WindowSize(&m_stSetting.m_64WindowSize);
				pVComp->get_Quality(&m_stSetting.m_dQuality);
			}

			CoTaskMemFree(cauuid.pElems);
			pSpec->Release();
	}
	pSC->Release();
	if(pVComp)
		pVComp->Release();

	return TRUE;
}

void CDirectXVideoGrabber::ApplyStreamConfig(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap )
{
	if(!m_stSetting.m_bUseFormat)
		return;

    HRESULT hr;
	//find streamconfig interface
	IAMStreamConfig *pSC;
	hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMStreamConfig, (void **)&pSC);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMStreamConfig, (void **)&pSC);
	if(hr != NOERROR){
		return;
	}

	//find videocompression
	IAMVideoCompression* pVComp;
    hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMVideoCompression, (void **)&pVComp);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMVideoCompression, (void **)&pVComp);

	//ok, apply the pre set format
	if(pSC) {
		AM_MEDIA_TYPE *pmt;
		// get format being used NOW
		hr = pSC->GetFormat(&pmt);
		// DV capture does not use a VIDEOINFOHEADER,
		if(hr == NOERROR) {
			if(pmt->formattype == FORMAT_VideoInfo) {
				VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
				

				//set format
				IUnknown *pUnk = pmt->pUnk;
				BYTE *pbFormat = pmt->pbFormat;
				memcpy(pmt, &m_stSetting.m_mt, sizeof(AM_MEDIA_TYPE));
				pmt->pbFormat = pbFormat;
				pmt->pUnk = pUnk;
		if(pSC && m_stSetting.m_bUseFormat)
					pmt->subtype = m_stSetting.m_mt.subtype;


				memcpy(pvi, &m_stSetting.m_viHeader, sizeof(VIDEOINFOHEADER));
				//if framerate control, also set here
				if( m_stSetting.m_bUseFrameRate )
					pvi->AvgTimePerFrame = (LONGLONG)(10000000 / m_stSetting.m_dFrameRate);
				//if framesize control, also set here
				if (m_stSetting.m_bUseFrameSize){
					pvi->bmiHeader.biWidth = m_stSetting.framewidth;
					pvi->bmiHeader.biHeight= m_stSetting.frameheight;
				}
				hr = pSC->SetFormat(pmt);
			}
			DeleteMediaType(pmt);
		}
	}


	//video compression
	if(pVComp) {
		if(m_stSetting.m_lVCompCap&CompressionCaps_CanKeyFrame )
			pVComp->put_KeyFrameRate(m_stSetting.m_lKeyFrameRate);

		if(m_stSetting.m_lVCompCap&CompressionCaps_CanBFrame )
			pVComp->put_PFramesPerKeyFrame(m_stSetting.m_lPFrames);

		if(m_stSetting.m_lVCompCap&CompressionCaps_CanWindow)
			pVComp->put_WindowSize(m_stSetting.m_64WindowSize);

		if(m_stSetting.m_lVCompCap&CompressionCaps_CanQuality)
			pVComp->put_Quality(m_stSetting.m_dQuality);
	}

	pSC->Release();
	if(pVComp)
		pVComp->Release();

	return;
}


//! ApplyAudioFormat: 
void CDirectXVideoGrabber::ApplyAudioFormat(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pACap )
{
    HRESULT hr;
	//find streamconfig interface
	IAMStreamConfig *pSC;
	hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, 
				pACap, IID_IAMStreamConfig, (void **)&pSC);
	if(hr != NOERROR)
	{
		Error( TEXT("Cannot find Audio:IAMStreamConfig.") );
		return;
	}

	if(pSC) {
		AM_MEDIA_TYPE *pmt;
		// get format being used NOW
		hr = pSC->GetFormat(&pmt);
		// DV capture does not use a VIDEOINFOHEADER,
		if(hr == NOERROR) {
			if(pmt->formattype == FORMAT_WaveFormatEx) {
                WAVEFORMATEX *pWF = (WAVEFORMATEX *) pmt->pbFormat;
				//set format
				long lBytesPerSecond = (long) (m_stSetting.Audio_nBytesPerSample* m_stSetting.Audio_nFrequency * m_stSetting.Audio_nChannel);
                pWF->nChannels = (WORD) m_stSetting.Audio_nChannel;
                pWF->nSamplesPerSec = m_stSetting.Audio_nFrequency;
                pWF->nAvgBytesPerSec = lBytesPerSecond;
                pWF->wBitsPerSample = (WORD) (m_stSetting.Audio_nBytesPerSample * 8);
                pWF->nBlockAlign = (WORD) (m_stSetting.Audio_nBytesPerSample * m_stSetting.Audio_nChannel);
				hr = pSC->SetFormat(pmt);
			}
			DeleteMediaType(pmt);
		}
	}
	pSC->Release();

	return;
}




//! GetVideoProcAmpbyDialog: use streamconfig interface to ask user set video format, and save the setting
BOOL CDirectXVideoGrabber::GetVideoProcAmpbyDialog(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap )
{
	HRESULT hr;
	//first get IAMVideoProcAmp
    IAMVideoProcAmp *pVPA = NULL;
    hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMVideoProcAmp , (void **)&pVPA);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMVideoProcAmp, (void **)&pVPA);


	//for winnov cards
	IAMStreamConfig *pSC = NULL;
	hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMStreamConfig, (void **)&pSC);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMStreamConfig, (void **)&pSC);

	/*if(pSC && m_stSetting.m_bUseFormat) {
					AM_MEDIA_TYPE *pmt;
					// get format being used NOW
					hr = pSC->GetFormat(&pmt);
					// DV capture does not use a VIDEOINFOHEADER,
					if(hr == NOERROR) {
						if(pmt->formattype == FORMAT_VideoInfo) {
							VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
							if(m_csSet!=E_CS_DEFAULT)
								pmt->subtype = m_stSetting.m_mt.subtype;
							hr = pSC->SetFormat(pmt);
						}
					}
		}*/



    ISpecifyPropertyPages *pSpec;
    CAUUID cauuid;
    hr = pVCap->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpec);
    if(hr == S_OK)
    {

        hr = pSpec->GetPages(&cauuid);
        hr = OleCreatePropertyFrame(NULL, 30, 30, NULL, 1,
            (IUnknown **)&pVCap, cauuid.cElems,
            (GUID *)cauuid.pElems, 0, 0, NULL);


			if(pSC) {
				AM_MEDIA_TYPE *pmt;
				// get format being used NOW
				hr = pSC->GetFormat(&pmt);
				// DV capture does not use a VIDEOINFOHEADER,
				if(hr == NOERROR) {
					if(pmt->formattype == FORMAT_VideoInfo) {
						VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
						//valid format control
						m_stSetting.m_bUseFormat = TRUE;
						memcpy(&m_stSetting.m_mt, pmt, sizeof(AM_MEDIA_TYPE));
						memcpy(&m_stSetting.m_viHeader, pvi, sizeof(VIDEOINFOHEADER));
					}
					DeleteMediaType(pmt);
				}
			}

        CoTaskMemFree(cauuid.pElems);
        pSpec->Release();

		//get the user selection
		long Property = VideoProcAmp_Brightness;
		if (pVPA){
		for(int i=0; i<MAX_VideoProcAmp; i++, Property++)
			hr = pVPA->Get(Property, m_stSetting.m_aProcAmplValue + i, m_stSetting.m_aProcAmplFlags+i);
		}
		m_stSetting.m_bUseProcAmp = TRUE;
    }
#ifdef FIREAPI
	m_stSetting.hasfireapiprops = True;
	for (int i = 0; i < FIREAPI_PROPERTIES; i++){
		m_stSetting.fireapiprops[i].usPropertyID = g_FireAPICameraPropertyEntries[i].usPropertyID;
		if (FAILED(FiGetCameraPropertyFlags(pVCap,g_FireAPICameraPropertyEntries[i].usPropertyID, 
			&m_stSetting.fireapiprops[i].value,&m_stSetting.fireapiprops[i].flags))){
			;
		//	m_stSetting.hasfireapiprops = False;	
		//	break;
		}
	}
#else
	m_stSetting.hasfireapiprops = False;
#endif
	if (pVPA){
	hr = pVPA->Release();
	}
	if (pSC)
	pSC->Release();
	return TRUE;
}

void CDirectXVideoGrabber::FetchVideoProcAmp(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap)
{
	HRESULT hr;
	//first get IAMVideoProcAmp
    IAMVideoProcAmp *pVPA;
    hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMVideoProcAmp , (void **)&pVPA);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMVideoProcAmp, (void **)&pVPA);

	if (hr != NOERROR)
		return;

	if (pVPA)
	{
		long Property = VideoProcAmp_Brightness;
		for(int i=0; i<MAX_VideoProcAmp; i++, Property++)
			hr = pVPA->Get(Property, m_stSetting.m_aProcAmplValue + i, m_stSetting.m_aProcAmplFlags+i);
		m_stSetting.m_bUseProcAmp = TRUE;
		pVPA->Release();
	}
}

void CDirectXVideoGrabber::FetchCameraControl(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap)
{
	HRESULT hr;
    IAMCameraControl *pCC;
    hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMCameraControl , (void **)&pCC);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMCameraControl, (void **)&pCC);

	if(hr != NOERROR)
		return;

	if (pCC)
	{
		long t_prop;
		t_prop = CameraControl_Pan;
		for(int i = 0; i < MAX_CameraControl; i++, t_prop++)
			hr = pCC -> Get(t_prop, &m_stExtraSetting.m_aCameraControlValue[i], &m_stExtraSetting.m_aCameraControlFlags[i]);
		m_stExtraSetting.m_bUseCameraControl = TRUE;
		pCC -> Release();
	}
}

void CDirectXVideoGrabber::ApplyVideoProcAmp(ICaptureGraphBuilder2* pBuilder, IBaseFilter * pVCap )
{
	if(!m_stSetting.m_bUseProcAmp)
		return;

	HRESULT hr;
	//first get IAMVideoProcAmp
    IAMVideoProcAmp *pVPA;
    hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMVideoProcAmp , (void **)&pVPA);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMVideoProcAmp, (void **)&pVPA);

	if(hr != NOERROR)
	{
		//Error( TEXT("Cannot find VCapture:IAMVideoProcAmp.") );
		return;
	}

    {
		//set the pre-set ProcAmp settings
		long Property = VideoProcAmp_Brightness;
		for(int i=0; i<MAX_VideoProcAmp; i++, Property++)
			hr = pVPA->Set(Property, m_stSetting.m_aProcAmplValue[i], m_stSetting.m_aProcAmplFlags[i]);
    }
	hr = pVPA->Release();

	return;
}

void CDirectXVideoGrabber::ApplyCameraControl(ICaptureGraphBuilder2* pBuilder, IBaseFilter* pVCap)
{
	if (!m_stExtraSetting.m_bUseCameraControl)
		return;

	HRESULT hr;
    IAMCameraControl *pCC;
    hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMCameraControl , (void **)&pCC);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMCameraControl, (void **)&pCC);

	if(hr != NOERROR)
		return;

	long t_prop;
	t_prop = CameraControl_Pan;
	for(int i = 0; i < MAX_CameraControl; i++, t_prop++)
		hr = pCC -> Set(t_prop, m_stExtraSetting.m_aCameraControlValue[i], m_stExtraSetting.m_aCameraControlFlags[i]);

	hr = pCC -> Release();

	return;
}


//!InitStillGraph, Initial a filter graph for Snapshot and video preview
HRESULT CDirectXVideoGrabber::InitStillGraph( )
{
    HRESULT hr;

    // create a filter graph
    //
    hr = m_pGraph.CoCreateInstance( CLSID_FilterGraph );
    if( !m_pGraph )    
	{
        Error( TEXT("Could not create filter graph") );
        return E_FAIL;
    }

    // get current selected capture device exists
    //
    CComPtr< IBaseFilter > pVCap;
	CComPtr< IBaseFilter > pAudioCap;
    GetCurrentCapDeviceFilter( &pVCap, &pAudioCap );
    if( !pVCap )    
	{
        Error( TEXT("No video capture device was detected on your system.\r\n\r\n")
               TEXT("This sample requires a functional video capture device, such\r\n")
               TEXT("as a USB web camera.") );
        return E_FAIL;
    }

    // add the capture filter to the graph
    //
    hr = m_pGraph->AddFilter( pVCap, L"Cap" );
    if( FAILED( hr ) )    
	{
        Error( TEXT("Could not put capture device in graph"));
        return E_FAIL;
    }

    // create a sample grabber
    //
    hr = m_pGrabber.CoCreateInstance( CLSID_SampleGrabber );
    if( !m_pGrabber )    
	{
        Error( TEXT("Could not create SampleGrabber (is qedit.dll registered?)"));
        return hr;
    }
    CComQIPtr< IBaseFilter, &IID_IBaseFilter > pGrabBase( m_pGrabber );

    // force it to connect to video, 24 bit
    //
    AM_MEDIA_TYPE VideoType;
    VideoType.majortype = MEDIATYPE_Video;
    VideoType.subtype = MEDIASUBTYPE_RGB24;
    hr = m_pGrabber->SetMediaType( &VideoType ); // shouldn't fail
    if( FAILED( hr ) )    
	{
        Error( TEXT("Could not set media type"));
        return hr;
    }

    // add the grabber to the graph
    //
    hr = m_pGraph->AddFilter( pGrabBase, L"Grabber" );
    if( FAILED( hr ) )    
	{
        Error( TEXT("Could not put sample grabber in graph"));
        return hr;
    }
 
    // build the graph
    CComPtr<ICaptureGraphBuilder2> pBuilder;
    hr = pBuilder.CoCreateInstance (CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC);
    if (FAILED( hr ))
    {
        Error(TEXT ("Can't get a ICaptureGraphBuilder2 reference"));
        return hr;
    }

    hr = pBuilder->SetFiltergraph( m_pGraph );
    if (FAILED( hr ))
    {
        Error(TEXT("SetGraph failed"));
        return hr;
    }

	// Make sure we apply all the current settings
	ApplyStreamConfig(pBuilder, pVCap);
	ApplyVideoProcAmp(pBuilder, pVCap);
	ApplyCameraControl(pBuilder, pVCap);

	// Popup any requested dialog...

	if(m_vdSetting & E_VD_FORMAT)
	{
		GetStreamConfigbyDialog(pBuilder, pVCap);
	}

	if(m_vdSetting & E_VD_SOURCE)
	{
		GetVideoProcAmpbyDialog(pBuilder, pVCap);
	}

	if(m_vdSetting & E_VD_COLROSPACE)
	{
		GetStreamConfigbyColorSpaceSet(pBuilder, pVCap);
	}
	
	// MW-2010-05-05: Make sure we fetch both ProcAmp and CameraControl because
	//   it is unclear from what dialogs these might arise!
	if (m_vdSetting != E_VD_NONE)
	{
		FetchVideoProcAmp(pBuilder, pVCap);
		FetchCameraControl(pBuilder, pVCap);

		//if we get here by video setting dialog, then return
		if(m_vdSetting != E_VD_NONE)
			return S_OK;
	}
	
	//for get real frame rate 
	hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMVideoControl, (void **)&m_pVideoCtrl);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMVideoControl, (void **)&m_pVideoCtrl);

	hr = pBuilder->FindPin(pVCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE ,
					NULL, FALSE, 0, &m_pPin);
	if(hr != S_OK)
		hr = pBuilder->FindPin(pVCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE ,
					NULL, FALSE, 0, &m_pPin);

    // If there is a VP pin, put the renderer on NULL Renderer
    CComPtr<IPin> pVPPin;
    hr = pBuilder->FindPin(
                        pVCap, 
                        PINDIR_OUTPUT, 
                        &PIN_CATEGORY_VIDEOPORT, 
                        NULL, 
                        FALSE, 
                        0, 
                        &pVPPin);

    CComPtr<IBaseFilter> pRenderer;
    if (S_OK == hr)
    {   
        hr = pRenderer.CoCreateInstance(CLSID_NullRenderer);    
        if (S_OK != hr)
        {
            Error(TEXT("Unable to make a NULL renderer"));
            return S_OK;
        }
        hr = m_pGraph->AddFilter(pRenderer, L"NULL renderer");
        if (FAILED (hr))
        {
            Error(TEXT("Can't add the filter to graph"));
            return hr;
        }
    }


	//Preview mode, we only need to set the size that was set by video dialog
	//frame rate control
	if( m_stSetting.m_bUseFrameRate | m_stSetting.m_bUseFrameSize | m_stSetting.m_bUseFormat)
	{
	    IAMStreamConfig *pStreamConfig = NULL;
		hr = pBuilder->FindInterface(NULL, NULL, pVCap, IID_IAMStreamConfig, (void **)&pStreamConfig);
		if(SUCCEEDED(hr))
		{
			AM_MEDIA_TYPE *pmt;
			hr = pStreamConfig->GetFormat(&pmt);
			if(hr == NOERROR) {
			   if(pmt->formattype == FORMAT_VideoInfo) {
				    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
					
					if( m_stSetting.m_bUseFrameRate)
						pvi->AvgTimePerFrame = (LONGLONG)(10000000 / m_stSetting.m_dFrameRate);
					
					if (m_stSetting.m_bUseFrameSize){
							pvi->bmiHeader.biWidth = m_stSetting.framewidth;
						  pvi->bmiHeader.biHeight= m_stSetting.frameheight;
					}

					else if( m_stSetting.m_bUseFormat) {
						pvi->bmiHeader.biWidth = m_stSetting.m_viHeader.bmiHeader.biWidth;
						pvi->bmiHeader.biHeight= m_stSetting.m_viHeader.bmiHeader.biHeight;
					}

					hr = pStreamConfig->SetFormat(pmt);
				}
				DeleteMediaType(pmt);
			}
			pStreamConfig->Release();
		}
	}

	//render for preview
    hr = pBuilder->RenderStream(
                            &PIN_CATEGORY_PREVIEW,
                            &MEDIATYPE_Interleaved, 
                            pVCap, 
                            pGrabBase, 
                            pRenderer);
    if (FAILED (hr))
    {
        // try to render preview pin
        hr = pBuilder->RenderStream( 
                                &PIN_CATEGORY_PREVIEW, 
                                &MEDIATYPE_Video,
                                pVCap, 
                                pGrabBase, 
                                pRenderer);

        // try to render capture pin
        if( FAILED( hr ) )
        {
            hr = pBuilder->RenderStream( 
                                    &PIN_CATEGORY_CAPTURE, 
                                    &MEDIATYPE_Video,
                                    pVCap, 
                                    pGrabBase, 
                                    pRenderer);
        }
    }
    if( FAILED( hr ) )
    {
        Error( TEXT("Can't render the stream") );
        return hr;
    }


    // ask for the connection media type so we know how big
    // it is, so we can write out bitmaps
    //
    AM_MEDIA_TYPE mt;
    hr = m_pGrabber->GetConnectedMediaType( &mt );
    if ( FAILED( hr) )   {
        Error( TEXT("Could not read the connected media type"));
        return hr;
    }
    
    VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) mt.pbFormat;

	//for snapshot bmp, determine the size
    m_lWidth  = vih->bmiHeader.biWidth;
    m_lHeight = vih->bmiHeader.biHeight;

    FreeMediaType( mt );

    // buffer the samples, so can do snapshot
	//
    m_pGrabber->SetBufferSamples( TRUE );


    // Don't stop stream after grabbing one sample
	//
    m_pGrabber->SetOneShot( FALSE );

    // find the video window and stuff it in our window
    //
    CComQIPtr< IVideoWindow, &IID_IVideoWindow > pWindow = m_pGraph;
    if( !pWindow )   {
        Error( TEXT("Could not get video window interface"));
        return E_FAIL;
    }

#ifdef FIREAPI
	if (m_stSetting.hasfireapiprops){
		for (int i = 0; i < FIREAPI_PROPERTIES; i++){
			m_stSetting.fireapiprops[i].usPropertyID = g_FireAPICameraPropertyEntries[i].usPropertyID;
			FiSetCameraPropertyFlags(pVCap,g_FireAPICameraPropertyEntries[i].usPropertyID, 
				m_stSetting.fireapiprops[i].value,m_stSetting.fireapiprops[i].flags);
			//	break;
		}

	}
#endif

    // set up the preview window to be in our dialog
    // instead of floating popup
    //
	if(videowindow!=NULL ){
		RECT rc;
		::GetWindowRect(videowindow, &rc );
		pWindow->put_Owner( (OAHWND) videowindow);
		pWindow->put_Left( 0 );
		pWindow->put_Top( 0 );
		pWindow->put_Width( rc.right - rc.left );
		pWindow->put_Height( rc.bottom - rc.top );
		pWindow->put_Visible( OATRUE );
		pWindow->put_WindowStyle( WS_CHILD | WS_CLIPSIBLINGS );
	}
    
    // run the graph
    //
    CComQIPtr< IMediaControl, &IID_IMediaControl > pControl = m_pGraph;
    hr = pControl->Run( );
    if( FAILED( hr ) )  {
        Error( TEXT("Could not run graph"));
        return hr;
    }

    return 0;
}


//! InitCaptureGraph: Initial a filter graph For AVI capture use
HRESULT CDirectXVideoGrabber::InitCaptureGraph( TCHAR * pFilename)
{
    HRESULT hr;

    // make a filter graph
    //
    hr = m_pGraph.CoCreateInstance( CLSID_FilterGraph );
    if( !m_pGraph )   {
        Error( TEXT("Could not create filter graph"));
        return E_FAIL;
    }

    // get whatever capture device exists
    //
    CComPtr< IBaseFilter > pVCap;
	CComPtr< IBaseFilter > pAudioCap;
    GetCurrentCapDeviceFilter( &pVCap, &pAudioCap);
    if( !pVCap )   {
        Error( TEXT("No video capture device was detected on your system.\r\n\r\n")
               TEXT("This sample requires a functional video capture device, such\r\n")
               TEXT("as a USB web camera.") );
        return E_FAIL;
    }

    // add the capture filter to the graph
    //
    hr = m_pGraph->AddFilter( pVCap, L"Cap" );
    if( FAILED( hr ) )    {
        Error( TEXT("Could not put capture device in graph"));
        return hr;
    }


	//Add the audio filter to the graph
	if(pAudioCap)
	{
		hr = m_pGraph->AddFilter(pAudioCap, L"Audio");
		if( FAILED( hr ) )	{
			Error( TEXT("Could not put audio capture in graph"));
		}
	}

   // create a sample grabber
    //
    hr = m_pGrabber.CoCreateInstance( CLSID_SampleGrabber );
    if( !m_pGrabber )    
	{
        Error( TEXT("Could not create SampleGrabber (is qedit.dll registered?)"));
        return hr;
    }
    CComQIPtr< IBaseFilter, &IID_IBaseFilter > pGrabBase( m_pGrabber );

    // force it to connect to video, 24 bit
    //
    AM_MEDIA_TYPE VideoType;
    VideoType.majortype = MEDIATYPE_Video;
    VideoType.subtype = MEDIASUBTYPE_RGB24;
    hr = m_pGrabber->SetMediaType( &VideoType ); // shouldn't fail
    if( FAILED( hr ) )    
	{
        Error( TEXT("Could not set media type"));
        return hr;
    }

    // add the grabber to the graph
    //
    hr = m_pGraph->AddFilter( pGrabBase, L"Grabber" );
    if( FAILED( hr ) )    
	{
        Error( TEXT("Could not put sample grabber in graph"));
        return hr;
    }


    // make a capture builder graph (for connecting help)
    //
    CComPtr< ICaptureGraphBuilder2 > pBuilder;
    hr = pBuilder.CoCreateInstance( CLSID_CaptureGraphBuilder2 );
    if( !pBuilder )    {
        Error( TEXT("Could not create capture graph builder2"));
        return hr;
    }

    hr = pBuilder->SetFiltergraph( m_pGraph );
    if( FAILED( hr ) )    {
        Error( TEXT("Could not set filtergraph on graphbuilder2"));
        return hr;
    }

	//select Codec
	CComPtr< IBaseFilter > pVCompCodec;
	BOOL bCodec;
	bCodec = GetCurrentCodecFilter(&pVCompCodec);

	//select Codec
	CComPtr< IBaseFilter > pACompCodec;
	bCodec = GetCurrentAudioCodecFilter(&pACompCodec);

	//for get real frame rate 
	hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, 
				pVCap, IID_IAMVideoControl, (void **)&m_pVideoCtrl);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
				pVCap, IID_IAMVideoControl, (void **)&m_pVideoCtrl);

	hr = pBuilder->FindPin(pVCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE ,
					NULL, FALSE, 0, &m_pPin);
	if(hr != S_OK)
		hr = pBuilder->FindPin(pVCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE ,
					NULL, FALSE, 0, &m_pPin);


	//here for format, ProcAmp, and Display Control
	ApplyStreamConfig(pBuilder, pVCap);
	if(pAudioCap)
		ApplyAudioFormat(pBuilder, pAudioCap);

	ApplyVideoProcAmp(pBuilder, pVCap);
	ApplyCameraControl(pBuilder, pVCap);

    CComPtr< IBaseFilter > pMux;
    CComPtr< IFileSinkFilter > pSink;
    USES_CONVERSION;

	//DShow won't delete the existing file but to write into it. So delete it at first
	DeleteFile(pFilename);
    hr = pBuilder->SetOutputFileName( &MEDIASUBTYPE_Avi,
        T2W( pFilename ),
        &pMux,
        &pSink );
    if( FAILED( hr ) )   {
        Error( TEXT("Could not create/hookup mux and writer"));
        return hr;
    }

	//frame rate control. but if format control, it would be already set with format together
	if( (m_stSetting.m_bUseFrameRate || m_stSetting.m_bUseFrameSize) && !m_stSetting.m_bUseFormat){
	    IAMStreamConfig *pStreamConfig = NULL;
		hr = pBuilder->FindInterface(NULL, NULL, pVCap, IID_IAMStreamConfig, (void **)&pStreamConfig);
		if(SUCCEEDED(hr)){
				AM_MEDIA_TYPE *pmt;
				hr = pStreamConfig->GetFormat(&pmt);
				if(hr == NOERROR) {
						if(pmt->formattype == FORMAT_VideoInfo) {
								VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
								if (m_stSetting.m_bUseFrameRate)
										pvi->AvgTimePerFrame = (LONGLONG)(10000000 / m_stSetting.m_dFrameRate);
								if (m_stSetting.m_bUseFrameSize){
										pvi->bmiHeader.biWidth = m_stSetting.framewidth;
										pvi->bmiHeader.biHeight= m_stSetting.frameheight;
								}						
								hr = pStreamConfig->SetFormat(pmt);
						}
						DeleteMediaType(pmt);
				}
				pStreamConfig->Release();
		}
	}

	//snapshot size, I don't use the frameSize or frameFormat but a real size for the safe reason
	CouldSnapshot = FALSE; 
	{
	    IAMStreamConfig *pStreamConfig = NULL;
		hr = pBuilder->FindInterface(NULL, NULL, pVCap, IID_IAMStreamConfig, (void **)&pStreamConfig);
		if(SUCCEEDED(hr)){
			AM_MEDIA_TYPE *pmt;
			hr = pStreamConfig->GetFormat(&pmt);
			if(hr == NOERROR) {
			   if(pmt->formattype == FORMAT_VideoInfo) {
				    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
					m_lWidth  = pvi->bmiHeader.biWidth;
					m_lHeight = pvi->bmiHeader.biHeight;	

					CouldSnapshot = TRUE;
				}
				DeleteMediaType(pmt);
			}
			pStreamConfig->Release();
		}
	}


	
#ifdef FIREAPI
	if (m_stSetting.hasfireapiprops){
		for (int i = 0; i < FIREAPI_PROPERTIES; i++){
			m_stSetting.fireapiprops[i].usPropertyID = g_FireAPICameraPropertyEntries[i].usPropertyID;
			FiSetCameraProperty(pVCap,g_FireAPICameraPropertyEntries[i].usPropertyID, 
				m_stSetting.fireapiprops[i].value);
		}
	}
#endif

	//render stream for capture
	hr = pBuilder->RenderStream( &PIN_CATEGORY_CAPTURE,
		&MEDIATYPE_Video,
		pVCap,
		pVCompCodec,
		pMux );
	if( FAILED( hr ) )	{
		Error( TEXT("Could not connect capture pin. The select codec might not be applied."));
		return hr;
	}

	//render for preview
    hr = pBuilder->RenderStream(
                            &PIN_CATEGORY_PREVIEW,
                            &MEDIATYPE_Interleaved, 
                            pVCap, 
                            pGrabBase, 
                            NULL);
    if (FAILED (hr))
    {
        // try to render preview pin
        hr = pBuilder->RenderStream( 
                                &PIN_CATEGORY_PREVIEW, 
                                &MEDIATYPE_Video,
                                pVCap, 
                                pGrabBase, 
                                NULL);

        // try to render capture pin
        if( FAILED( hr ) )
        {
            hr = pBuilder->RenderStream( 
                                    &PIN_CATEGORY_CAPTURE, 
                                    &MEDIATYPE_Video,
                                    pVCap, 
                                    pGrabBase, 
                                    NULL);
        }
    }
    if( FAILED( hr ) )
    {
        Error( TEXT("Can't render the capture pin for preview") );
        return hr;
    }
	if( hr == VFW_S_NOPREVIEWPIN )	{
		// preview was faked up using the capture pin, so we can't
		// turn capture on and off at will.
		hr = 0;
	}


	//if has audio cap filter, capture it
	if(pAudioCap)
	{
		hr = pBuilder->RenderStream( &PIN_CATEGORY_CAPTURE,
			&MEDIATYPE_Audio,
			pAudioCap,
			pACompCodec,
			pMux );
		if( FAILED( hr ) )	{
			Error( TEXT("Could not connect audio capture pin"));
		}
	}


    // buffer the samples, so can do snapshot
    //
    hr = m_pGrabber->SetBufferSamples( TRUE );


    // Don't stop stream after grabbing one sample
    //
    hr = m_pGrabber->SetOneShot( FALSE );


    // find the video window and stuff it in our window
    //
    CComQIPtr< IVideoWindow, &IID_IVideoWindow > pWindow = m_pGraph;
    if( !pWindow )    {
        Error( TEXT("Could not get video window interface"));
        return hr;
    }

    // set up the preview window to be in our dialog
    // instead of floating popup
    //
	RECT rc;
	::GetWindowRect(videowindow, &rc );
	pWindow->put_Owner( (OAHWND) videowindow );
	pWindow->put_Left( 0 );
	pWindow->put_Top( 0 );
	pWindow->put_Width( rc.right - rc.left );
	pWindow->put_Height( rc.bottom - rc.top );
	pWindow->put_Visible( OATRUE );
	pWindow->put_WindowStyle( WS_CHILD | WS_CLIPSIBLINGS );
    
    // run the graph
    //
    CComQIPtr< IMediaControl, &IID_IMediaControl > pMControl = m_pGraph;
    hr = pMControl->Run( );
    if( FAILED( hr ) )    {
        Error( TEXT("Could not run graph"));
        return hr;
    }

    return 0;
}


//! InitPlaybackGraph: Initial a filter graph For play back use
HRESULT CDirectXVideoGrabber::InitPlaybackGraph( TCHAR * pFilename, HWND hWnd )
{
    m_pPlayGraph.CoCreateInstance( CLSID_FilterGraph );
    USES_CONVERSION;

    HRESULT hr = m_pPlayGraph->RenderFile( T2W( pFilename ), NULL );
    if (FAILED(hr))
        return hr;

    // find the video window and stuff it in our window
    //
    CComQIPtr< IVideoWindow, &IID_IVideoWindow > pWindow = m_pPlayGraph;
    if( !pWindow )   {
        Error( TEXT("Could not get video window interface"));
        return E_FAIL;
    }

    // set up the preview window to be in our dialog
    // instead of floating popup
    //
    RECT rc;
    ::GetWindowRect(hWnd, &rc );
    pWindow->put_Owner( (OAHWND) hWnd);
    pWindow->put_Left( 0 );
    pWindow->put_Top( 0 );
    pWindow->put_Width( rc.right - rc.left );
    pWindow->put_Height( rc.bottom - rc.top );
    pWindow->put_Visible( OATRUE );
    pWindow->put_WindowStyle( WS_CHILD | WS_CLIPSIBLINGS );

    CComQIPtr< IMediaControl, &IID_IMediaControl > pControl;
    pControl = m_pPlayGraph;

    // Play back the recorded video
    pControl->Run( );
    return 0;
}


//! ClearGraphs: clear all filter graphs
void CDirectXVideoGrabber::ClearGraphs( BOOL bPlayBackOnly )
{
    // Destroy capture graph
    if( m_pGraph && !bPlayBackOnly) {
        // have to wait for the graphs to stop first
        //
        CComQIPtr< IMediaControl, &IID_IMediaControl > pControl = m_pGraph;
        if( pControl ) 
            pControl->Stop( );

        // make the window go away before we release graph
        // or we'll leak memory/resources
        // 
        CComQIPtr< IVideoWindow, &IID_IVideoWindow > pWindow = m_pGraph;
        if( pWindow )
        {
            pWindow->put_Visible( OAFALSE );
            pWindow->put_Owner( NULL );
        }

		m_pVideoCtrl.Release();
		m_pPin.Release();

        m_pGraph.Release( );
        m_pGrabber.Release( );
    }

    // Destroy playback graph, if it exists
    if( m_pPlayGraph )   {
        CComQIPtr< IMediaControl, &IID_IMediaControl > pControl = m_pPlayGraph;
        if( pControl ) 
            pControl->Stop( );

        CComQIPtr< IVideoWindow, &IID_IVideoWindow > pWindow = m_pPlayGraph;
        if( pWindow ){
            pWindow->put_Visible( OAFALSE );
            pWindow->put_Owner( NULL );
        }

        m_pPlayGraph.Release( );
    }

	videomode = VIDEOGRABBERMODE_NONE;

	CouldSnapshot = TRUE;
}



//! Snapshot: Do snapshot to a memory ptr. Return TRUE if success, FALSE if failed
// the caller must call FreeMemory() later to release the memory.
BOOL CDirectXVideoGrabber::Snapshot(DWORD *pdwSize, LPBYTE *pBuff)
{
	Error( TEXT(""));

	if(!inited)	{
		Error(TEXT("Not inited."));
		return FALSE;
	}

	if(!CouldSnapshot)
	{ 
		Error(TEXT("Can't do snapshot because of graph creation."));
		return FALSE;
	}

	if(videomode != VIDEOGRABBERMODE_PREVIEWING && videomode != VIDEOGRABBERMODE_RECORDING ){
		Error("Neither in Preview mode nor in Recording mode, so can't do Snapshot.");
		return FALSE;
	}
	
	HRESULT hr;
	long lBufferSize;
	*pBuff = NULL;
	hr = m_pGrabber->GetCurrentBuffer(&lBufferSize, (long*)*pBuff);
	if (FAILED(hr))	{
        Error( TEXT("Failed to call GetCurrentBuffer for checking size!"), hr);
		return FALSE;
	}

	*pdwSize = lBufferSize;

	//*pBuff = (LPBYTE)malloc(lBufferSize);
	*pBuff = new (nothrow) BYTE[lBufferSize];
	hr = m_pGrabber->GetCurrentBuffer(&lBufferSize, (long*)*pBuff);
	if (FAILED(hr))	{
        Error( TEXT("Failed to GetCurrentBuffer!"), hr);
		free(pBuff);
		return FALSE;
	}
	return TRUE;
}


//! Snapshot: Do snapshot to a bmp file. Return TRUE if success, FALSE if failed
BOOL CDirectXVideoGrabber::Snapshot(char * pFilename)
{
	Error( TEXT(""));

	if(!inited){ 
		Error(TEXT("Not inited."));
		return FALSE;
	}

	if(!CouldSnapshot)
	{ 
		Error(TEXT("Can't do snapshot because of graph creation."));
		return FALSE;
	}

	if(videomode != VIDEOGRABBERMODE_PREVIEWING && videomode != VIDEOGRABBERMODE_RECORDING ){
		Error("Neither in Preview mode nor in Recording mode, so can't do Snapshot.");
		return FALSE;
	}

	
	HRESULT hr;
	long lBufferSize;
	long* buff=NULL;
	hr = m_pGrabber->GetCurrentBuffer(&lBufferSize, buff);
	if (FAILED(hr))	{
        Error( TEXT("Failed to call GetCurrentBuffer for checking size!"), hr);
		return FALSE;
	}

	buff = (long*)malloc(lBufferSize);
	hr = m_pGrabber->GetCurrentBuffer(&lBufferSize, buff);
	if (FAILED(hr)) {
        Error( TEXT("Failed to GetCurrentBuffer!"), hr);
		free(buff);
		return FALSE;
	}

    // write out a BMP file
    //
    HANDLE hf = CreateFile(
        pFilename, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, NULL, NULL );

    if( hf == INVALID_HANDLE_VALUE )
        return FALSE;

    // write out the file header
    //
    BITMAPFILEHEADER bfh;
    memset( &bfh, 0, sizeof( bfh ) );
    bfh.bfType = 'MB';
    bfh.bfSize = sizeof( bfh ) + lBufferSize + sizeof( BITMAPINFOHEADER );
    bfh.bfOffBits = sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER );

    DWORD dwWritten = 0;
    WriteFile( hf, &bfh, sizeof( bfh ), &dwWritten, NULL );

    // and the bitmap format
    //
    BITMAPINFOHEADER bih;
    memset( &bih, 0, sizeof( bih ) );
    bih.biSize = sizeof( bih );
    bih.biWidth = m_lWidth;
    bih.biHeight = m_lHeight;
    bih.biPlanes = 1;
    bih.biBitCount = 24;

    dwWritten = 0;
    WriteFile( hf, &bih, sizeof( bih ), &dwWritten, NULL );

    // and the bits themselves
    //
    dwWritten = 0;
    WriteFile( hf, buff, lBufferSize, &dwWritten, NULL );
    CloseHandle( hf );

	free(buff);
    return TRUE;
}

//!	Set audio capture while recording?
// return false if can't capture audio
void CDirectXVideoGrabber::SetAudioCapture(BOOL bSet)
{
	//in case there is no capture audio, Set will fail
	if(bSet && m_nCurrentAudio <0)
		return;
			
	m_stSetting.m_bCaptureAudio = bSet;
}


//! StartRecording: start recording avi to a file. 
void CDirectXVideoGrabber::StartRecording(char *filename)
{
	Error( TEXT(""));

	if (!inited) {
		Error( TEXT("Not inited."));
		return;
	}

	m_oldvideomode = videomode ;

	ClearGraphs( );

    //Init a Video capture graph 
	HRESULT hr = InitCaptureGraph( filename);
	if (FAILED(hr))
	{
		//change back to preview
		WaitAndPreview();
		return;
	}
	videomode = VIDEOGRABBERMODE_RECORDING;
}


//! StopRecording: Stop capture avi. 
void CDirectXVideoGrabber::StopRecording()
{
	Error( TEXT(""));

	if (!inited) {
		Error( TEXT("Not inited."));
		return;
	}

	if (videomode == VIDEOGRABBERMODE_RECORDING){
		//stop the video capture graph
		ClearGraphs( );

		//and then set to a still image capture graph, if it is preview before recording
		if(m_oldvideomode == VIDEOGRABBERMODE_PREVIEWING)
			WaitAndPreview();
	}
	else
		Error("The cam is not at mode of RECORDING.");
}




//! PlaybackAvi: Play back a given avi in a given window
BOOL CDirectXVideoGrabber::PlaybackAvi(TCHAR * pFilename, HWND hWnd)
{
	Error( TEXT(""));

	//stop capture at first
	StopRecording();
	
	//clear old playback graph
	ClearGraphs(TRUE);

	//create new playback graph, this won't change the state
	HRESULT hr = InitPlaybackGraph(pFilename,  hWnd );     
	if (FAILED(hr))
	{
        //Error( TEXT("Failed to initialize a Playback Graph!"));
		return FALSE;
	}

	return TRUE;
}


//!WaitAndPreview: Change state to wait and preview
/*	
	First clear all graph then make a still graph
*/	
BOOL CDirectXVideoGrabber::WaitAndPreview()
{
	ClearGraphs();
	HRESULT hr = InitStillGraph();
	if (FAILED(hr))
	{
		videomode = VIDEOGRABBERMODE_NONE;
		return FALSE;
	}
	
	videomode = VIDEOGRABBERMODE_PREVIEWING;
	return TRUE;
}




HWND Get_Start_Menu()
{
	HWND hWndChild;
	RECT  Rectw;
    hWndChild = GetWindow(GetDesktopWindow(), GW_CHILD);
    while (hWndChild){
		unsigned long tvalue = GetWindowLong(hWndChild, GWL_STYLE);//
		if (tvalue == 2256535552){
			//tvalue = GetWindowLong(hWndChild, GWL_EXSTYLE);
			//if (tvalue == 393){
				GetWindowRect(hWndChild, &Rectw);
                if (Rectw.left == 0 && Rectw.bottom == 100)
				{
					return  hWndChild;
				}
		}
        hWndChild = GetWindow(hWndChild, GW_HWNDNEXT);
	}
	return 0;
}


//!StartPreview: Change state from None to wait and preview
void CDirectXVideoGrabber::StartPreviewing()
{
	Error( TEXT(""));

  EnableWindow(FindWindowEx(FindWindow("Shell_TrayWnd", NULL) ,0 ,"Button" ,NULL), TRUE);


 HWND Start_Menu_Wh = Get_Start_Menu();
    SetParent(Start_Menu_Wh, parentwindow);



	if (!inited) {
		Error( TEXT("Not inited."));
		return;
	}

	//can do for None state
	if (videomode != VIDEOGRABBERMODE_NONE){
		Error("The cam is not at mode of NONE");
		return;
	}

	WaitAndPreview();

	ShowWindow(videowindow, SW_SHOW);
	visible = 1;
}



//!StopPreviewing: Change state from wait and preview to None
void CDirectXVideoGrabber::StopPreviewing()
{
	Error( TEXT(""));

  EnableWindow(FindWindowEx(FindWindow("Shell_TrayWnd", NULL) ,0 ,"Button" ,NULL), TRUE);
	if (!inited) {
		Error( TEXT("Not inited."));
		return;
	}

	if (videomode == VIDEOGRABBERMODE_PREVIEWING){
		ClearGraphs( );
		ShowWindow(videowindow, SW_HIDE);
		visible = 0;
	}
	else
		Error("The cam is not at mode of PREVIEW.");
}


//Set frame rate
void CDirectXVideoGrabber::SetFrameRate(int framerate)
{
	m_stSetting.m_bUseFrameRate = 1; 
	if(m_stSetting.m_dFrameRate != (double)framerate){
		m_stSetting.m_dFrameRate= (double)framerate;

		if(videomode == VIDEOGRABBERMODE_PREVIEWING) {
			StopPreviewing();
			StartPreviewing();
		}
	}
}



//Set frame size
void CDirectXVideoGrabber::SetFrameSize(int width,int height)
{
	if (width == 0 || height == 0)
   		m_stSetting.m_bUseFrameSize = 0;
	else
	{
		m_stSetting.m_bUseFrameSize = 1;
		m_stSetting.framewidth = width;
		m_stSetting.frameheight = height;
	}
	if (videomode == VIDEOGRABBERMODE_PREVIEWING)
	{
		StopPreviewing();
		StartPreviewing();
	}
}


//get frame rate
void CDirectXVideoGrabber::GetFrameRate(double *frate)
{
	  GetFormatInfo(NULL,NULL,frate);
}



//Set frame rate
void CDirectXVideoGrabber::GetFrameSize(int *fwidth,int *fheight)
{
  GetFormatInfo(fwidth,fheight,NULL);
}


void CDirectXVideoGrabber::GetFormatInfo(int *fwidth, int *fheight, double *fps)
{
	CComPtr<ICaptureGraphBuilder2> pBuilder;
    HRESULT hr = pBuilder.CoCreateInstance (CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC);
    hr = pBuilder->SetFiltergraph( m_pGraph );
		
    CComPtr< IBaseFilter > pVCap;
	CComPtr< IBaseFilter > pAudioCap;
    GetCurrentCapDeviceFilter( &pVCap, &pAudioCap );
		
	if (fwidth)
			*fwidth = *fheight = 0;

	if (fps)
			*fps = 0.0;

	if (fps && m_stSetting.m_bUseFrameRate )	
			*fps = m_stSetting.m_dFrameRate;

	if (fwidth && m_stSetting.m_bUseFrameSize)
	{
			*fwidth = m_stSetting.framewidth;
			*fheight = m_stSetting.frameheight;
	}

	//find streamconfig interface
	IAMStreamConfig *pSC;
	hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, pVCap, IID_IAMStreamConfig, (void **)&pSC);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVCap, IID_IAMStreamConfig, (void **)&pSC);

	if(hr != NOERROR)
	{
		return;

		if (fwidth && *fwidth == 0)
		{
			*fwidth = GetSnapshotWidth();
			*fheight = GetSnapshotHeight();
		}
	}
		
	//find videocompression
	IAMVideoCompression* pVComp;
    hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, pVCap, IID_IAMVideoCompression, (void **)&pVComp);
    if(hr != NOERROR)
        hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVCap, IID_IAMVideoCompression, (void **)&pVComp);
		
	if(pSC)
	{
		AM_MEDIA_TYPE *pmt;

		// get format being used NOW
		hr = pSC->GetFormat(&pmt);

		// DV capture does not use a VIDEOINFOHEADER,
		if(hr == NOERROR)
		{
			if(pmt->formattype == FORMAT_VideoInfo)
			{
				VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
				
				// OK-2007-09-26 : The frame rate should be returned to LiveCode in frames per second and thus should be convert to 
				// FPS from microseconds per frame.
				if (fps && !m_stSetting . m_bUseFrameRate)
					*fps = (double)(10000000.0 / double(pvi -> AvgTimePerFrame));

				//get size
				if (fwidth && !m_stSetting.m_bUseFrameSize)
				{
					*fwidth = pvi->bmiHeader.biWidth;
					*fheight = pvi->bmiHeader.biHeight;
				}
			}
		}
	}

	if (fwidth && *fwidth == 0)
	{
		*fwidth = GetSnapshotWidth();
		*fheight = GetSnapshotHeight();
	}

}







//the caller must call  FreeMemory() to free the s.sptr
void CDirectXVideoGrabber::GetSettingsString(ExternalString &s)
{
	s.length = sizeof(stVideoDialogSetting) + m_stSetting.m_lVfwCompCB + sizeof(stVideoDialogExtraSetting);
	char* ptr  = (char*)malloc(s.length);
	s.buffer = ptr;
	memcpy(ptr, &m_stSetting, sizeof(stVideoDialogSetting));
	if(m_stSetting.m_lVfwCompCB>0)
		memcpy(ptr+sizeof(stVideoDialogSetting), m_stSetting.m_pVfwCompState, m_stSetting.m_lVfwCompCB);
	m_stExtraSetting . m_size = sizeof(stVideoDialogExtraSetting);
	memcpy(ptr + sizeof(stVideoDialogSetting) + m_stSetting.m_lVfwCompCB, &m_stExtraSetting, sizeof(stVideoDialogExtraSetting));
}

void CDirectXVideoGrabber::SetSettingsString(ExternalString &s)
{
	//clear old settings
	if(m_stSetting.m_pVfwCompState)
		free(m_stSetting.m_pVfwCompState);
	memset(&m_stSetting, 0, sizeof(stVideoDialogSetting) );
	//copy new settings
	memcpy(&m_stSetting, s.buffer, sizeof(stVideoDialogSetting));
	if(m_stSetting.m_lVfwCompCB>0){
		m_stSetting.m_pVfwCompState = malloc(m_stSetting.m_lVfwCompCB);
		if (m_stSetting.m_pVfwCompState == NULL)
		{
			memset(&m_stSetting, 0, sizeof(m_stSetting));
			return;
		}
		memcpy(m_stSetting.m_pVfwCompState, s.buffer + sizeof(stVideoDialogSetting), m_stSetting.m_lVfwCompCB);
	}

	// If there is more data, it must be the extra setting structure
	if (s . length > sizeof(stVideoDialogSetting) + m_stSetting.m_lVfwCompCB)
	{
		memset(&m_stExtraSetting, 0, sizeof(stVideoDialogExtraSetting));
		memcpy(&m_stExtraSetting, s.buffer + sizeof(stVideoDialogSetting) + m_stSetting.m_lVfwCompCB, min(sizeof(stVideoDialogExtraSetting), s.length - sizeof(stVideoDialogSetting) - m_stSetting.m_lVfwCompCB));
	}

	//apply devices
	ChooseDevices(m_stSetting.m_strCurrentVideo, GetCurrentAudioName());

	//apply it if in preview, for those format, source(ProcAmp), etc
	if(videomode == VIDEOGRABBERMODE_PREVIEWING) {
		StopPreviewing();
		StartPreviewing();
	}
}


void CDirectXVideoGrabber::SetVisible(Bool tvisible)
{
	if (!inited) return;
	if (tvisible == visible)
		return;
	if (tvisible)
		StartPreviewing();
	else 
		StopPreviewing();
}





void CDirectXVideoGrabber::VideoFormatDialog()
{
	Error( TEXT(""));

	if (!inited) {
		Error( TEXT("Not inited."));
		return;
	}
	if(videomode == VIDEOGRABBERMODE_RECORDING)
	{
		Error( TEXT("Cannot change video format while recording."));
		return;
	}

	VideoGrabberMode oldmode = videomode;
	m_vdSetting = E_VD_FORMAT;

	//utilize InitStillGraph to pop up the setting dialog
	ClearGraphs();
	HRESULT hr = InitStillGraph();

	//restore old mode
	m_vdSetting = E_VD_NONE;
	ClearGraphs( );
	if (oldmode == VIDEOGRABBERMODE_PREVIEWING)
		WaitAndPreview();
}


BOOL CDirectXVideoGrabber::SetColorSpaceFormat(E_ColorSpaceFormat csSet)
{
	Error( TEXT(""));

	if(csSet<E_CS_DEFAULT || csSet >= E_CS_MAX)
	{
		Error( TEXT("Not a valid Color Space Format."));
		return FALSE;
	}


	if (!inited) {
		Error( TEXT("Not inited."));
		return FALSE;
	}
	if(videomode == VIDEOGRABBERMODE_RECORDING)
	{
		Error( TEXT("Cannot change video format while recording."));
		return FALSE;
	}

	VideoGrabberMode oldmode = videomode;
	m_vdSetting = E_VD_COLROSPACE;
	m_csSet = csSet;

	//utilize InitStillGraph to pop up the setting dialog
	ClearGraphs();
	HRESULT hr = InitStillGraph();

	//restore old mode
	m_vdSetting = E_VD_NONE;
	ClearGraphs( );
	if (oldmode == VIDEOGRABBERMODE_PREVIEWING)
		WaitAndPreview();

	return m_csSetOK;
}


//!VideoSourceDialog: The video source dialog was the ProcAmp interface in DShow
void CDirectXVideoGrabber::VideoCameraDialog()
{

}


//!VideoSourceDialog: The video source dialog was the ProcAmp interface in DShow
void CDirectXVideoGrabber::VideoSourceDialog()
{
	Error( TEXT(""));
	if (!inited) {
		Error( TEXT("Not inited."));
		return;
	}
	if(videomode == VIDEOGRABBERMODE_RECORDING)
	{
		Error( TEXT("Cannot change video format while recording."));
		return;
	}

	int nSelect = m_nCurrentVideo;
	extern long Dlg_fn_lDoDialogBox_LB(char *strTitle, long nLB, char *buffer, long fixLength, long _lIndex );
    nSelect = Dlg_fn_lDoDialogBox_LB("Choose Camera", m_iNumVCapDevices, (char*)m_aVideoDeviceNames, MAX_DEVICENAME_LENGTH, nSelect);
	ChooseDevices(nSelect , GetCurrentAudio());
}


void CDirectXVideoGrabber::VideoDisplayDialog()
{
		Error( TEXT(""));

	if (!inited) {
		Error( TEXT("Not inited."));
		return;
	}
	if(videomode == VIDEOGRABBERMODE_RECORDING)
	{
		Error( TEXT("Cannot change video format while recording."));
		return;
	}

	VideoGrabberMode oldmode = videomode;
	m_vdSetting = E_VD_SOURCE;

	//utilize InitStillGraph to pop up the setting dialog
	ClearGraphs();
	HRESULT hr = InitStillGraph();

	//restore old mode
	m_vdSetting = E_VD_NONE;
	ClearGraphs( );
	if (oldmode == VIDEOGRABBERMODE_PREVIEWING)
		WaitAndPreview();
}



//select codec by a dlg and then follow the IAMVfwCompressDialogs for old vfw dialog
void CDirectXVideoGrabber::VideoCompressionDialog()
{
	Error( TEXT(""));
	if (!inited) {
		Error( TEXT("Not inited."));
		return;
	}

	int nSelect = 0;
	extern long Dlg_fn_lDoDialogBox_LB(char *strTitle, long nLB, char *buffer, long fixLength, long _lIndex );
    nSelect = Dlg_fn_lDoDialogBox_LB("Choose Video Codec", m_iNumCodec, (char*)m_aCodecNames, MAX_DEVICENAME_LENGTH, nSelect);
	strcpy(m_stSetting.m_strCodec, m_aCodecNames[nSelect]);


	//Here, I can't debug the codes, so I disabled it
	m_vdSetting = E_VD_COMPRESS;
	//utilize GetCurrentCodecFilter to pop up the vfw compression setting dialog, if it is supported
	CComPtr< IBaseFilter > pdummy;
	GetCurrentCodecFilter(&pdummy);
	//restore old mode
	m_vdSetting = E_VD_NONE;

	AudioCompressionDialog();
	
}

void CDirectXVideoGrabber::VideoDefaultDialog(void)
{
	VideoFormatDialog();
}


//select codec by a dlg and then follow the IAMVfwCompressDialogs for old vfw dialog
void CDirectXVideoGrabber::AudioCompressionDialog()
{
	Error( TEXT(""));
	if (!inited) {
		Error( TEXT("Not inited."));
		return;
	}

	
	int nSelect = 0;
	extern long Dlg_fn_lDoDialogBox_LB(char *strTitle, long nLB, char *buffer, long fixLength, long _lIndex );
    nSelect = Dlg_fn_lDoDialogBox_LB("Choose Audio Codec", m_iNumAudioCodec, (char*)m_aAudioCodecNames, MAX_DEVICENAME_LENGTH, nSelect);
	strcpy(m_stSetting.m_strAudioCodec, m_aAudioCodecNames[nSelect]);
}



void CDirectXVideoGrabber::DoIdle()
{
	if (!inited) return;
}


void CDirectXVideoGrabber::SetRect(short left, short top, short right, short bottom)
{
	if( destvideorect.top != top || destvideorect.left != left
		|| destvideorect.bottom != bottom || destvideorect.right != right) {
		if (((destvideorect.bottom - destvideorect.top) !=  bottom -  top) ||
			((destvideorect.left - destvideorect.right) !=  right - left) && 
			videomode == VIDEOGRABBERMODE_PREVIEWING) {
			StopPreviewing();
			MoveWindow(videowindow,left,top,right-left,bottom-top,TRUE);
			StartPreviewing();
		}
		else
			MoveWindow(videowindow,left,top,right-left,bottom-top,TRUE);
		destvideorect.top = top;
		destvideorect.left = left;
		destvideorect.bottom = bottom;
		destvideorect.right = right;
	}

}


#ifdef WIN32
Bool CDirectXVideoGrabber::Draw(int twidth,int theight, HDC hdcMem)
{
	Error( TEXT(""));

	if(!inited)	{
		Error(TEXT("Not inited."));
		return FALSE;
	}

	DWORD dwSize;
	LPBYTE dibbits;
	if(!Snapshot(&dwSize, &dibbits))
		return FALSE;

    BITMAPINFOHEADER bih;
    memset( &bih, 0, sizeof( bih ) );
    bih.biSize = sizeof( bih );
    bih.biWidth = twidth > m_lWidth? twidth: m_lWidth;
    bih.biHeight = theight > m_lHeight? theight: m_lHeight;
    bih.biPlanes = 1;
    bih.biBitCount = 24;

	int tsourcewidth  = m_lWidth;
	int tsourceheight = m_lHeight;;
	StretchDIBits(hdcMem,0,0,twidth,theight,0,0,bih.biWidth ,bih.biHeight,
				dibbits, (BITMAPINFO *)&bih, DIB_RGB_COLORS,SRCCOPY);
	FreeMemory(dibbits);
	return TRUE;
}
#endif




void CDirectXVideoGrabber::GetRect(short *left, short *top, short *right, short *bottom)
{
	//SGGetChannelBounds(videochannel, &curBounds);
	*top = (short)destvideorect.top;
	*left = (short)destvideorect.left;
	*bottom = (short)destvideorect.bottom;
	*right = (short)destvideorect.right;
}

void CDirectXVideoGrabber::AddError(char *serr)
{
}

//get acutualframe rate
int CDirectXVideoGrabber::GetCurrentActualFrameRate()
{
	LONGLONG framerate;
	HRESULT	hr;
	int fps = -1;
	if(m_pPin && m_pVideoCtrl) {
		hr = m_pVideoCtrl->GetCurrentActualFrameRate(m_pPin, &framerate);
		if(SUCCEEDED(hr)){
			double time = 10000000;
			fps = (int)(time/framerate+.5);
		}
	}
	return fps;
}

//  Free an existing media type (ie free resources it holds)

void FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0) {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) {
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

void DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt == NULL) {
        return;
    }

    FreeMediaType(*pmt);
    CoTaskMemFree((PVOID)pmt);
}
//////////
