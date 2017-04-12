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

#include <stdlib.h>
#include "core.h"

#ifdef _MACOSX
#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>
#endif

#include <revolution/external.h>
#include "qtvideograbber.h"

// MW-2010-08-16: Previously, settings were only stored for the video channel.
//   To rectify this, when the two channels are present we use this record
//   which will always have a 'header' as RVG0. These fields are serialized
//   in network byte-order.

enum
{
	kSettingsRecordHeader = 'RVG0'
};

struct SettingsRecord
{
	uint32_t header;
	uint32_t video_size;
	uint32_t audio_size;
};

#if defined _MACOSX

RgnHandle getWindowContentRegion(WindowRef window,RgnHandle contentRegion)
{
    Rect portBounds;
    GetWindowBounds(window, kWindowGlobalPortRgn, &portBounds);
    RectRgn(contentRegion, &portBounds);
    return contentRegion;
}

static pascal long BorderlessWindowProc(short varCode, WindowRef window,
                                        short message, long param)
{
#pragma unused( varCode )
    switch (message) {
        case kWindowMsgGetFeatures:
            *(OptionBits*) param = kWindowCanGetWindowRegion
            | kWindowDefSupportsColorGrafPort;
            return 1;
        case kWindowMsgGetRegion:
        {
            GetWindowRegionRec* rgnRec  = (GetWindowRegionRec*) param;
            switch (rgnRec->regionCode) {
                case kWindowTitleBarRgn:
                case kWindowTitleTextRgn:
                case kWindowCloseBoxRgn:
                case kWindowZoomBoxRgn:
                case kWindowDragRgn:
                case kWindowGrowRgn:
                case kWindowCollapseBoxRgn:
                    SetEmptyRgn(rgnRec->winRgn);
                    break;
                case kWindowStructureRgn:
                case kWindowContentRgn:
                    getWindowContentRegion(window, rgnRec->winRgn);
                    break;
                case kWindowUpdateRgn:
                    break;
                default:
                    return errWindowRegionCodeInvalid;
            }
            return noErr;
        }
        case kWindowMsgHitTest:
            Point hitPoint;
            static RgnHandle tempRgn = nil;
            if (!tempRgn)
                tempRgn = NewRgn();
            SetPt(&hitPoint, LoWord(param), HiWord(param));//get the point clicked
            if (PtInRgn(hitPoint, getWindowContentRegion(window, tempRgn)))
                return wInContent;
            else
                return wNoHit;
        default:
            break;
    }
    return 0;
}
#endif

#ifdef WIN32
void Path2Native(char *dptr)
{
  if (!*dptr)
    return;
#if defined WIN32 || defined MACOS && !defined MACHO
#ifdef MACOS
  if (*dptr == '/')
    strcpy(dptr, dptr + 1);
#endif
  do {
    if (*dptr == '/')
#ifdef WIN32
      *dptr = '\\';
    else
      if (*dptr == '\\')
        *dptr = '/';
#else
      *dptr = ':';
    else
      if (*dptr == ':')
        *dptr = '/';
#endif
  } while (*++dptr);
#endif
}

static OSErr path2FSSpec(const char *fname, FSSpec *fspec)
{
	char *nativepath = new (nothrow) char[strlen(fname)+1];
	strcpy(nativepath, fname);
	Path2Native(nativepath);
	OSErr err = noErr;
	err = NativePathNameToFSSpec(nativepath, fspec, 0);
	delete nativepath;
	return err;
}

#endif


#ifdef WIN32

void setDataOutput(SeqGrabComponent p_grabber, const char *p_filename)
{
	FSSpec t_file_system_spec;
	path2FSSpec(p_filename, &t_file_system_spec);
	SGSetDataOutput(p_grabber, &t_file_system_spec, seqGrabToDisk);
}

#else

void setDataOutput(SeqGrabComponent p_grabber, const char *p_filename)
{
	bool t_error;
	t_error = false;
	
	OSErr t_sys_error;

	if (p_filename == NULL)
		t_error = true;
		
	CFStringRef t_cf_path;
	Handle t_handle;
	OSType t_type;
		
	t_cf_path = NULL;
	t_handle = NULL;
		
	if (!t_error)
	{
		t_cf_path = CFStringCreateWithCString(kCFAllocatorDefault, p_filename, kCFStringEncodingUTF8);
		if (t_cf_path == NULL)
			t_error = true;
	}
	if (!t_error)
	{
		t_sys_error = QTNewDataReferenceFromFullPathCFString(t_cf_path, kQTPOSIXPathStyle, 0, &t_handle, &t_type);
		if (t_sys_error != noErr)
			t_error = true;
	}
	
	if (!t_error)
	{
		t_sys_error = CreateMovieStorage(t_handle, t_type, 'TVOD', smSystemScript, createMovieFileDeleteCurFile | createMovieFileDontCreateMovie | createMovieFileDontOpenFile, NULL, NULL);
		if (t_sys_error != noErr)
			t_error = true;
	}
		
	if (!t_error)
	{
		t_sys_error = SGSetDataRef(p_grabber, t_handle, t_type, seqGrabToDisk | seqGrabDontPreAllocateFileSize | seqGrabAppendToFile);
		if (t_sys_error != noErr)
			t_error = true;
	}
	else if (t_handle != NULL)
		DisposeHandle(t_handle);
	
	if (t_cf_path != NULL)
		CFRelease(t_cf_path);
		
}

#endif




#ifdef WIN32
extern HINSTANCE hInstance;

long FAR PASCAL VideoWindowProc (HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	switch(message)
	{
		case WM_PAINT:
			break;

		case WM_ENTERSIZEMOVE:
			break;

		case WM_EXITSIZEMOVE:
			break;
	}
	return DefMDIChildProc (hWnd, message, wParam, lParam);
}


Bool RegisterWindowClass()
{
	WNDCLASS	wc={0};
	static Boolean windowclassregistered = False;
	if (!windowclassregistered) 
	{
		// Register the Monitor child window class
		wc.style         = 0;
		wc.lpfnWndProc   = (WNDPROC)VideoWindowProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = hInstance;
		wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = "VIDEOGRABBER";
		if (RegisterClass(&wc))
			windowclassregistered = True;
	}
	return windowclassregistered;
}
#endif

#ifdef WIN32
//open sequence grabber component instance and draw to window
CQTVideoGrabber::CQTVideoGrabber(HWND whichwindow)
{ 
	parentwindow = whichwindow;
	if (!RegisterWindowClass())
		return;	
	videowindow = CreateWindow ("VIDEOGRABBER",
		"CAPTURE",WS_CHILD | WS_VISIBLE,
		0, 0, 160, 120,
		(HWND) parentwindow,NULL,hInstance,0); 
	CreatePortAssociation(videowindow, NULL, 0);
	destvideobuffer = (GWorldPtr)GetNativeWindowPort(videowindow);
	buffervideo = False;
	Init();
}
#else

// IM-2014-07-24: [[ Bug 12863 ]] Observer class to monitor changes to parent window size
@interface com_runrev_livecode_MCQTVideoGrabberWindowObserver : NSObject
{
	CQTVideoGrabber *m_video_grabber;
}

- (id)initWithVideoGrabber:(CQTVideoGrabber *)p_grabber;
- (void)windowDidResize:(NSNotification *)notification;
@end

@implementation com_runrev_livecode_MCQTVideoGrabberWindowObserver

- (id) initWithVideoGrabber:(CQTVideoGrabber *)p_grabber
{
	self = [self init];
	if (self == nil)
		return nil;
	
	m_video_grabber = p_grabber;
    
    return self;
}

- (void)windowDidResize:(NSNotification *)notification
{
	m_video_grabber->Synchronize(true);
}

@end

@compatibility_alias MCQTVideoGrabberWindowObserver com_runrev_livecode_MCQTVideoGrabberWindowObserver;

CQTVideoGrabber::CQTVideoGrabber(WindowPtr whichwindow)
{
    parentwindow = [NSApp windowWithWindowNumber: (uint32_t)whichwindow];

	Rect r;
	r.top = r.left = 0;
    r.right = 160;
    r.bottom = 120;
	
    const WindowDefSpec customWindowDefSpec
	  = {kWindowDefProcPtr, {NewWindowDefUPP(BorderlessWindowProc)}};
	CreateCustomWindow(&customWindowDefSpec, kFloatingWindowClass, kWindowNoActivatesAttribute | kWindowNoShadowAttribute,
			   &r, &videowindow);

    videowindow_cocoa = [[NSWindow alloc] initWithWindowRef: videowindow];
    
	destvideobuffer = (GWorldPtr)GetWindowPort(videowindow);
	buffervideo = False;
	Init();
	Synchronize(true);
    [(NSWindow *)parentwindow addChildWindow: (id)videowindow_cocoa ordered: NSWindowAbove];
	
	// IM-2014-07-24: [[ Bug 12863 ]] Add observer to reposition video window when parent size changes
	m_window_observer = [[MCQTVideoGrabberWindowObserver alloc] initWithVideoGrabber:this];
	[[NSNotificationCenter defaultCenter] addObserver:(id)m_window_observer selector: @selector(windowDidResize:) name:NSWindowDidResizeNotification object:(NSWindow*)parentwindow];
	
	ShowWindow(videowindow);
}
#endif

//open sequence grabber component instance and draw to gworld
CQTVideoGrabber::CQTVideoGrabber()
{ 
	destvideorect.top = destvideorect.left = 0;
	destvideorect.bottom = destvideorect.right = 100;
	videowindow = NULL;
	parentwindow = NULL;
	destvideobuffer = NULL;
	QTNewGWorld(&destvideobuffer, k32BGRAPixelFormat, &destvideorect, NULL, NULL, 0);
	buffervideo = True;
	Init();
}

#ifdef _MACOSX
void CQTVideoGrabber::Synchronize(bool p_visible)
{
    NSWindow *t_parent_window;
    t_parent_window = (NSWindow *)parentwindow;
    
    NSRect t_content;
    t_content = [t_parent_window contentRectForFrameRect: [t_parent_window frame]];
    
    int t_height;
    t_height = [[[NSScreen screens] objectAtIndex: 0] frame] . size . height;
    
	Rect t_bounds;
	t_bounds . left = t_content . origin . x + destvideorect . left;
	t_bounds . top = t_height - (t_content . origin . y + t_content . size . height) + destvideorect . top;
	t_bounds . right = t_content . origin . x + destvideorect . right;
	t_bounds . bottom = t_height - (t_content . origin . y + t_content . size . height) + destvideorect . bottom;
	SetWindowBounds(videowindow, kWindowContentRgn, &t_bounds);
}
#endif

void CQTVideoGrabber::Init()
{
	ComponentResult			result = noErr;
	inited = False;
	videograbber = NULL;
	videochannel = NULL;
	soundchannel = NULL;
	videomode = VIDEOGRABBERMODE_NONE;
	destvideorect.top = destvideorect.left = destvideorect.bottom = destvideorect.right = 1;
	srcvideorect.top = srcvideorect.left = srcvideorect.bottom = srcvideorect.right = 1;
	videograbber = OpenDefaultComponent(SeqGrabComponentType,0);
	if (!videograbber)
	{
		AddError("Can't open sequence grabber component");
		return;
	}

	//initialize sequence grabber
	result = SGInitialize (videograbber);
	result = SGSetGWorld(videograbber, destvideobuffer, NULL);

	// Get a video channel
	result = SGNewChannel (videograbber, VideoMediaType, &videochannel);
	if (!videochannel)
	{
		AddError("Can't create video channel");
		return;
	}
#ifdef _MACOSX
	result = SGNewChannel (videograbber, SGAudioMediaType, &soundchannel);
#else
	result = SGNewChannel (videograbber, SoundMediaType, &soundchannel);
#endif
	if ((soundchannel != nil) && (result == noErr))
	{
		if (soundchannel != nil)
		{
			result = SGSetChannelUsage (soundchannel, seqGrabPreview | seqGrabRecord);
			result = SGSetChannelVolume (soundchannel, 0x0010);
		}
	}

	//set whether to buffer offscreen
	result = SGSetUseScreenBuffer(videochannel, buffervideo == False);
	if (result != noErr)
	{
		AddError("Can't buffer offscreen");
		return;
	}
	result = SGGetSrcVideoBounds (videochannel, &srcvideorect);
	//set channel usage to preview and record and grab picture
	result = SGSetChannelUsage (videochannel, seqGrabPreview | seqGrabRecord | seqGrabPlayDuringRecord);
	SGSetChannelPlayFlags(videochannel,channelPlayNormal|channelPlayAllData);
	//set default channel rect to size of video
	result = SGSetChannelBounds (videochannel,  &destvideorect);
	inited = True;
}

CQTVideoGrabber::~CQTVideoGrabber()
{
	StopRecording();
	StopPreviewing();
	if (videowindow)
	{
#ifdef WIN32
		DestroyPortAssociation(destvideobuffer);
		DestroyWindow(videowindow);
#else
        [(NSWindow *)parentwindow removeChildWindow: (id)videowindow_cocoa];
		[(NSWindow *)videowindow_cocoa release];
		
		// IM-2014-07-24: [[ Bug 12863 ]] Remove and release observer class
		[[NSNotificationCenter defaultCenter] removeObserver:(id)m_window_observer];
		[(MCQTVideoGrabberWindowObserver*)m_window_observer release];
#endif
	}
	else 
		DisposeGWorld(destvideobuffer);

	CloseComponent(videograbber);
}


inline uint32_t byteswap_uint32(uint32_t x)
{
#ifndef __BIG_ENDIAN__
	return ((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00) | ((x >> 24) & 0xff);
#else
	return x;
#endif
}


void CQTVideoGrabber::GetSettingsString(ExternalString &s)
{
	if (!inited)
		return;

	UserData t_video_data, t_audio_data;
	t_video_data = nil;
	t_audio_data = nil;
	SGGetChannelSettings(videograbber, videochannel, &t_video_data, 0);
	SGGetChannelSettings(videograbber, soundchannel, &t_audio_data, 0);
	
	Handle t_video_handle, t_audio_handle;
	t_video_handle = NewHandle(0);
	t_audio_handle = NewHandle(0);
	if (t_video_handle != nil)
		PutUserDataIntoHandle(t_video_data, t_video_handle);
	if (t_audio_handle != nil)
		PutUserDataIntoHandle(t_audio_data, t_audio_handle);
	
	int32_t t_video_size, t_audio_size;
	t_video_size = 0;
	t_audio_size = 0;
	if (t_video_handle != nil)
		t_video_size = GetHandleSize(t_video_handle);
	if (t_audio_handle != nil)
		t_audio_size = GetHandleSize(t_audio_handle);

	SettingsRecord *t_record;
	t_record = (SettingsRecord *)malloc(sizeof(SettingsRecord) + t_video_size + t_audio_size);
	if (t_record != nil)
	{
		memset(t_record, 0, sizeof(SettingsRecord) + t_video_size + t_audio_size);
		
		t_record -> header = byteswap_uint32(kSettingsRecordHeader);
		t_record -> video_size = byteswap_uint32(t_video_size);
		t_record -> audio_size = byteswap_uint32(t_audio_size);
		
		HLock(t_video_handle);
		memcpy(t_record + 1, *t_video_handle, t_video_size);
		HUnlock(t_video_handle);
		
		HLock(t_audio_handle);
		memcpy((char *)(t_record + 1) + t_video_size, *t_audio_handle, t_audio_size);
		HUnlock(t_audio_handle);
		
		s . length = sizeof(SettingsRecord) + t_video_size + t_audio_size;
		s . buffer = (char *)t_record;
	}
	
	if (t_audio_handle != nil)
		DisposeHandle(t_audio_handle);
	if (t_video_handle != nil)
		DisposeHandle(t_video_handle);
	
	if (t_audio_data != nil)
		DisposeUserData(t_audio_data);
	if (t_video_data != nil)
		DisposeUserData(t_video_data);
}

void CQTVideoGrabber::SetSettingsString(ExternalString &s)
{
	if (!inited)
		return;

	SettingsRecord *t_record;
	t_record = (SettingsRecord *)s . buffer;
	
	uint32_t t_video_size, t_audio_size;
	t_video_size = byteswap_uint32(t_record -> video_size);
	t_audio_size = byteswap_uint32(t_record -> audio_size);
	
	Handle t_video_handle, t_audio_handle;
	t_video_handle = nil;
	t_audio_handle = nil;
	if (byteswap_uint32(t_record -> header) == kSettingsRecordHeader)
	{
		if (t_video_size != 0)
		{
			t_video_handle = NewHandle(t_video_size);
			if (t_video_handle != nil)
			{
				HLock(t_video_handle);
				memcpy(*t_video_handle, t_record + 1, t_video_size);
				HUnlock(t_video_handle);
			}
		}
		
		if (t_audio_size != 0)
		{
			t_audio_handle = NewHandle(t_audio_size);
			if (t_audio_handle != nil)
			{
				HLock(t_audio_handle);
				memcpy(*t_audio_handle, (char *)(t_record + 1) + t_video_size, t_audio_size);
				HUnlock(t_audio_handle);
			}
		}
	}
	else
	{
		t_video_handle = NewHandle(s . length);
		HLock(t_video_handle);
		memcpy(*t_video_handle, s . buffer, s . length);
		HUnlock(t_video_handle);
	}
	
	UserData t_video_data;
	t_video_data = nil;
	if (t_video_handle != nil)
	{
		if (NewUserDataFromHandle(t_video_handle, &t_video_data) == noErr)
		{
			SGSetChannelSettings(videograbber, videochannel, t_video_data, 0);
			DisposeUserData(t_video_data);
		}
		DisposeHandle(t_video_handle);
	}
	
	UserData t_audio_data;
	t_audio_data = nil;
	if (t_audio_handle != nil)
	{
		if (NewUserDataFromHandle(t_audio_handle, &t_audio_data) == noErr)
		{
			SGSetChannelSettings(videograbber, soundchannel, t_audio_data, 0);
			DisposeUserData(t_audio_data);
		}
		DisposeHandle(t_audio_handle);
	}
}


void CQTVideoGrabber::DoIdle()
{
	if (!inited)
		return;

	SGIdle(videograbber);
}

void CQTVideoGrabber::SetVisible(Bool tvisible)
{
	if (!inited)
		return;

	if (tvisible == visible)
		return;

	if (tvisible)
		StartPreviewing();
	else
		StopPreviewing();

}

void CQTVideoGrabber::StartPreviewing()
{
	StopRecording();
	SGStop(videograbber);
	SGStartPreview(videograbber);
	videomode = VIDEOGRABBERMODE_PREVIEWING;
}


void CQTVideoGrabber::StopPreviewing()
{
	if (videomode == VIDEOGRABBERMODE_PREVIEWING)
	{
		SGStop(videograbber);
		videomode = VIDEOGRABBERMODE_NONE;
	}
}

void CQTVideoGrabber::StartRecording(char *filename)
{
	if (!inited) return;
	SGStop(videograbber);
	videomode = VIDEOGRABBERMODE_RECORDING;
	setDataOutput(videograbber, filename);
	
	SGStartRecord(videograbber);
	videomode =  VIDEOGRABBERMODE_RECORDING;
}


void CQTVideoGrabber::StopRecording()
{
	if (!inited) return;
	if (videomode == VIDEOGRABBERMODE_RECORDING){
		SGStop(videograbber);
		videomode = VIDEOGRABBERMODE_NONE;
		if (visible)
			StartPreviewing();
	}
	
}

void CQTVideoGrabber::SetRect(short left, short top, short right, short bottom)
{
#ifdef WIN32
	// OK-2007-10-02 : Convert the coordinates to global to be consistent with DirectX and Video For Windows drivers
	RECT t_parent_window_rect;
	if (!GetWindowRect(parentwindow, &t_parent_window_rect))
		return;	
	
	destvideorect . top = t_parent_window_rect . top + top;
	destvideorect . left = t_parent_window_rect . left + left;
	destvideorect . bottom = t_parent_window_rect . top + bottom;
	destvideorect . right = t_parent_window_rect . left + right;

	ComponentResult result = noErr;;
	result = SGPause(videograbber,True);
	Rect videochannelrect;
	
	::MacSetRect(&videochannelrect, 0, 0, destvideorect . right - destvideorect . left, destvideorect . bottom - destvideorect . top);
	
	if (!videowindow)
	{
		DisposeGWorld(destvideobuffer);
		QTNewGWorld(&destvideobuffer, k32BGRAPixelFormat, &destvideorect, NULL, NULL, NULL);
		SGSetGWorld(videograbber, destvideobuffer, NULL);
	}
	else
		MoveWindow(videowindow, destvideorect . left, destvideorect . top, destvideorect . right - destvideorect . left, destvideorect . bottom - destvideorect . top, TRUE);
	result = SGSetChannelBounds (videochannel, &videochannelrect);
	result = SGPause(videograbber,False);
#else
	destvideorect.top = top;
	destvideorect.left = left;
	destvideorect.bottom = bottom;
	destvideorect.right = right;
	ComponentResult result = noErr;;
	result = SGPause(videograbber,True);
	Rect videochannelrect;
	::MacSetRect(&videochannelrect,0,0,right-left,bottom-top);
	if (!videowindow){
		DisposeGWorld(destvideobuffer);
		QTNewGWorld(&destvideobuffer, k32BGRAPixelFormat, &destvideorect, NULL, NULL, 0);
		SGSetGWorld(videograbber, destvideobuffer, NULL);
	}
	else
	{
		Synchronize(true);
	}
	result = SGSetChannelBounds (videochannel, &videochannelrect);
	result = SGPause(videograbber,False);
#endif
}

void CQTVideoGrabber::GetRect(short *left, short *top, short *right, short *bottom)
{
	*top = destvideorect.top;
	*left = destvideorect.left;
	*bottom = destvideorect.bottom;
	*right = destvideorect.right;
}


void CQTVideoGrabber::VideoFormatDialog()
{
	DoDialog();
}

void CQTVideoGrabber::VideoSourceDialog()
{
	DoDialog();
}


void CQTVideoGrabber::VideoDisplayDialog()
{
	DoDialog();
}

void CQTVideoGrabber::VideoDefaultDialog(void)
{
	DoDialog();
}

static pascal Boolean
videograbberModalFilterProc (DialogPtr theDialog, const EventRecord *theEvent,
	short *itemHit, long refCon)
{
	Boolean	handled = false;
	return (handled);
}


void CQTVideoGrabber::AudioDefaultDialog(void)
{
	//short	width, height;
	ComponentResult	err;
	// Pause
	if (videograbber == NULL || (soundchannel == NULL && videochannel == NULL))
		return;
	err = SGPause (videograbber, True);
    
    // AL-2014-08-14: [[ Bug 12966 ]] Replace deprecated quicktime functions for MacOSX
#ifdef _MACOSX
    ComponentInstance ci;
    OpenADefaultComponent(StandardCompressionType, StandardCompressionSubTypeAudio, &ci);
    
    AudioStreamBasicDescription t_description;
    
    AudioChannelLayoutTag t_layout_tags[] =
    {
        kAudioChannelLayoutTag_UseChannelDescriptions,
        kAudioChannelLayoutTag_Mono,
        kAudioChannelLayoutTag_Stereo,
    };
    
    QTSetComponentProperty(ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_ClientRestrictedChannelLayoutTagList,sizeof(t_layout_tags), t_layout_tags);
    
    if (err == noErr)
        err = SCRequestImageSettings(ci);
    
	if (err == noErr)
        err = QTGetComponentProperty(ci, kQTPropertyClass_SCAudio,kQTSCAudioPropertyID_BasicDescription,sizeof(t_description), &t_description, NULL);
    
    if (err == noErr)
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
        
        if (err == noErr)
            err = QTSetComponentProperty(soundchannel, kQTPropertyClass_SGAudio, kQTSGAudioPropertyID_StreamFormat, sizeof(t_description), &t_description);
        
        // Set any additional settings for this configuration
        if (t_magic_cookie_size != 0)
                QTSetComponentProperty(soundchannel, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_MagicCookie,
                                       t_magic_cookie_size, t_magic_cookie);
        else if (t_codec_settings)
                QTSetComponentProperty(soundchannel, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_CodecSpecificSettingsArray,
                                       sizeof(t_codec_settings), t_codec_settings);
    }
#else
	SGModalFilterUPP	videograbberModalFilterUPP;
	videograbberModalFilterUPP = (SGModalFilterUPP)NewSGModalFilterUPP(videograbberModalFilterProc);
	err = SGSettingsDialog(videograbber, soundchannel, 0, nil, 0L, videograbberModalFilterUPP, (long)0);
    DisposeRoutineDescriptor((UniversalProcPtr)videograbberModalFilterUPP);
#endif
	// The pause that refreshes
	err = SGPause (videograbber, False);	
}


void CQTVideoGrabber::DoDialog()
{
	//short	width, height;
	ComponentResult	err;
	SGModalFilterUPP	videograbberModalFilterUPP;
	// Pause
	if (videograbber == NULL || (soundchannel == NULL && videochannel == NULL))
		return;
	err = SGPause (videograbber, True);
	videograbberModalFilterUPP = (SGModalFilterUPP)NewSGModalFilterUPP(videograbberModalFilterProc);
	err = SGSettingsDialog(videograbber, videochannel, 0, nil, 0L, videograbberModalFilterUPP, (long)0);
	DisposeRoutineDescriptor((UniversalProcPtr)videograbberModalFilterUPP);
	// The pause that refreshes
	err = SGPause (videograbber, False);
}







#ifdef WIN32
Bool CQTVideoGrabber::Draw(int twidth,int theight, HDC hdcMem)
{
	if (!inited) 
		return False;

	HGLOBAL videodib = NULL;
	HDC cool = (HDC)GetPortHDC((GrafPtr)destvideobuffer);
	PicHandle videopic = NULL;
	ComponentResult	result = noErr;
	Rect videochannelrect;
	MacSetRect(&videochannelrect,0,0,destvideorect.right-destvideorect.left,destvideorect.bottom-destvideorect.top);
	result = SGGrabPict (videograbber, &videopic, &videochannelrect, 0, 0L);
	if (result != noErr) 
		return False;

	videodib = GetDIBFromPICT(videopic);
	KillPicture(videopic);
	char *dibdata = NULL;
	char *dibbits = NULL;
	BITMAPINFO *pbmp;
	Bool res = False;
	dibdata = (char *)GlobalLock(videodib);
	pbmp = (BITMAPINFO *)dibdata;
	int tsourcewidth  = pbmp->bmiHeader.biWidth;
	int tsourceheight = pbmp->bmiHeader.biHeight;
	int palentries = pbmp->bmiHeader.biClrUsed;
	int bitcount = pbmp->bmiHeader.biBitCount;
	int lenrgb = 0;
	if (bitcount < 24)
	{
		if (palentries == 0)
			palentries = (1 << bitcount);

		lenrgb = palentries * sizeof(RGBQUAD);
	}
	dibbits = (char *)(dibdata + sizeof(BITMAPINFOHEADER) + lenrgb);
	StretchDIBits(hdcMem,0,0,twidth,theight,0,0,tsourcewidth,tsourceheight, (BITMAPINFO *)dibbits, pbmp, DIB_RGB_COLORS,SRCCOPY);
	GlobalUnlock(videodib);
	GlobalFree(videodib);
	res = True;
	return res;
}
#else
Bool CQTVideoGrabber::Draw(int twidth,int theight, GWorldPtr gworldMem)
{
	if (!inited) 
		return False;
	PicHandle videopic = NULL;
	ComponentResult	result = noErr;
	Rect videochannelrect;
	//::MacSetRect(&videochannelrect,0,0,twidth,theight);

	::MacSetRect(&videochannelrect,0,0,destvideorect.right-destvideorect.left,destvideorect.bottom-destvideorect.top);
	SGPause (videograbber, True);
	//result = SGGrabPict (videograbber, &videopic, &destvideorect, 0, grabPictIgnoreClip|grabPictCurrentImage);
	//if (videopic == NULL)
		result = SGGrabPict (videograbber, &videopic, &videochannelrect, 0, grabPictOffScreen|grabPictIgnoreClip);

	SGPause (videograbber, False);
	if (result != noErr)
		return False;
	CGrafPtr GRFsaved;
	GDHandle GDorigdevice;
	GetGWorld(&GRFsaved,&GDorigdevice);
	SetGWorld(gworldMem,NULL);
	::MacSetRect(&videochannelrect,0,0,twidth,theight);
	DrawPicture(videopic,&videochannelrect);
	SetGWorld(GRFsaved,GDorigdevice);
	KillPicture(videopic);
	return True;
}
#endif




void CQTVideoGrabber::AddError(char *serr)
{
}


void CQTVideoGrabber::SetFrameRate(int framerate)
{
}

void CQTVideoGrabber::GetFrameRate(double *r_rate)
{

}

void CQTVideoGrabber::SetFrameSize(int p_width, int p_height)
{

}

void CQTVideoGrabber::GetFrameSize(int *r_width, int *r_height)
{

}

void CQTVideoGrabber::SetAudioCapture(Bool bSet)
{
}


void CQTVideoGrabber::setCompressor(char* codecName)
{
}

char *CQTVideoGrabber::getCompressors()
{
	return NULL;
}

void CQTVideoGrabber::setAudioFormat(int channels, int bits, int frequency)
{
}
void CQTVideoGrabber::getAudioFormat(int* channels, int* bits, int* frequency)
{
}
