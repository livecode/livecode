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
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <core.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
#define __STDC__ 1
#endif

#include <revolution/external.h>
#include <revolution/support.h>

#include "revvideograbber.h"

#ifndef WIN32
#include "qtvideograbber.h"
#endif

#ifdef WIN32
#include "dsvideograbber.h"
#include "mcivideograbber.h"
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1040
#define kNativeEndianPixMap 0
#define kCGBitmapByteOrder32Host 0
#endif

#define istrdup strdup

static CVideoGrabber *gvideograbber = NULL;

#ifndef WIN32
extern CVideoGrabber *CreateQTXVideoGrabber(WindowPtr window);
#endif

void VideoGrabberDoIdle()
{
	if (gvideograbber)
		gvideograbber->DoIdle();
}


void VIDEOGRABBER_INIT()
{

}

void VIDEOGRABBER_QUIT()
{
	if (gvideograbber){
		delete gvideograbber;
		gvideograbber = NULL;
	}
}

typedef unsigned int uint4;
typedef unsigned char uint1;
typedef unsigned short uint2;
typedef double real8;

static Bool byte_swapped()
{
	uint2 test = 1;
	return *((uint1 *)&test);
}

inline int swap_uint4(uint4 *dest)
{
				if (byte_swapped()) {
					uint1 *tptr = (uint1 *)dest;
					uint1 tmp = tptr[0];
					tptr[0] = tptr[3];
					tptr[3] = tmp;
					tmp = tptr[1];
					tptr[1] = tptr[2];
					tptr[2] = tmp;
				}
				return *dest;
}

struct MYBITMAP
{
	uint1 *data;
	uint2 width;
	uint2 height;
	uint1 depth;
	uint4 bytes_per_line;
	#ifdef WIN32
	HBITMAP bm;
	#else //MAC
	 GWorldPtr bm;
	#endif
};

MYBITMAP *createmybitmap(uint2 depth, uint2 width, uint2 height)
{
  MYBITMAP *image = new (nothrow) MYBITMAP;
  image->width = width;
  image->height = height;
  image->depth = (uint1)depth;
  image->bytes_per_line = ((width * depth + 31) >> 3) & 0xFFFFFFFC;
  image->data = NULL;
  image->bm = NULL;
  #ifdef WIN32
  BITMAPINFO *bmi = NULL;
  bmi = (BITMAPINFO *)new char[sizeof(BITMAPV4HEADER)];
  memset(bmi, 0, sizeof(BITMAPV4HEADER));
  bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi->bmiHeader.biCompression = BI_BITFIELDS;
  BITMAPV4HEADER *b4 = (BITMAPV4HEADER *)bmi;
  b4->bV4RedMask = 0xFF0000;
  b4->bV4GreenMask = 0xFF00;
  b4->bV4BlueMask = 0xFF;
  bmi->bmiHeader.biWidth = width;
  bmi->bmiHeader.biHeight = -height;
  bmi->bmiHeader.biPlanes = 1;
  bmi->bmiHeader.biBitCount = depth;
  bmi->bmiHeader.biSizeImage = image->bytes_per_line * image->height;
  image->bm = CreateDIBSection(GetDC(NULL), bmi, DIB_RGB_COLORS,
	                       (void **)&image->data, NULL, 0);
  if (image->data == NULL || image->bm == NULL)
    image->data = (uint1 *)new char[image->bytes_per_line * image->height];
  delete bmi;
  #else //MAC
   Rect r;
    r.top = r.left = 0;
    r.bottom = height;
    r.right = width;
   QDErr err = NewGWorld(&image->bm, depth, &r, NULL, NULL, useTempMem | kNativeEndianPixMap);
    if (image->bm != NULL || err == noErr) {
      PixMapHandle dpm = GetGWorldPixMap(image->bm);
      LockPixels(dpm);
      image->data = (uint1 *)GetPixBaseAddr(dpm);
      image->bytes_per_line = (*dpm)->rowBytes & 0x3FFF;
    }
    if (image->data == NULL) {
    image->bytes_per_line = ((width * depth + 31) >> 3) & 0xFFFFFFFC;
    image->data = (uint1 *)new char[image->bytes_per_line * height];
    image->bm = NULL;
  	}
    #endif
  return image;
}
	
void destroymybitmap(MYBITMAP *image)
{
#ifdef WIN32
  if (image->bm == NULL)
    delete image->data;
  else
    DeleteObject(image->bm);
    #else
      if (image->bm != NULL) {
    PixMapHandle dpm = GetGWorldPixMap(image->bm);
    UnlockPixels(dpm);
    DisposeGWorld(image->bm);
  }
  else
    delete image->data;
    #endif
  delete image;
}

#ifndef WIN32
Bool InitQT()
{
	static Bool QTInited = False;
	if (QTInited) return QTInited;
#ifdef WIN32
	if (InitializeQTML(0L) == noErr || EnterMovies() == noErr)
		QTInited = True;
#else
    if (EnterMovies() == noErr)
        QTInited = True;
#endif
	return QTInited;
}
#endif


enum VideoGrabberKeyword
{
VIDEOGRABBER_UNDEFINED,
VIDEOGRABBER_OPEN,
VIDEOGRABBER_CLOSE,
VIDEOGRABBER_MOVE,
VIDEOGRABBER_SHOW,
VIDEOGRABBER_HIDE,
VIDEOGRABBER_STARTRECORDING,
VIDEOGRABBER_STOPRECORDING,
VIDEOGRABBER_DIALOG,
VIDEOGRABBER_GRABIMAGE,
VIDEOGRABBER_IDLE,
VIDEOGRABBER_DIALOGFORMAT,
VIDEOGRABBER_DIALOGSOURCE,
VIDEOGRABBER_DIALOGDISPLAY,
VIDEOGRABBER_DIALOGCOMPRESSION,
VIDEOGRABBER_DIALOGAUDIO,
VIDEOGRABBER_DIALOGVIDEO,
VIDEOGRABBER_GETOPTIONS,
VIDEOGRABBER_SETOPTIONS,
VIDEOGRABBER_SETRECT,
VIDEOGRABBER_SETFRAMERATE,
VIDEOGRABBER_GETFRAMERATE,
VIDEOGRABBER_GETFRAMESIZE,
VIDEOGRABBER_SETFRAMESIZE,
VIDEOGRABBER_GETAUDIO,
VIDEOGRABBER_SETAUDIO,
VIDEOGRABBER_GETAUDIOCOMPRESSORS,
VIDEOGRABBER_GETCOMPRESSORS,
VIDEOGRABBER_SETCOMPRESSOR,
VIDEOGRABBER_GETCOMPRESSOR,
VIDEOGRABBER_GETAUDIOCOMPRESSOR,
VIDEOGRABBER_SETAUDIOCOMPRESSOR,
VIDEOGRABBER_SETCOLORSPACE,
VIDEOGRABBER_LISTDEVICESETTINGS,
VIDEOGRABBER_GETDEVICESETTING,
VIDEOGRABBER_SETDEVICESETTING,
};

struct VideoGrabberKeywordRec {
	VideoGrabberKeyword prop;
	const char *str;
};

VideoGrabberKeywordRec VideoGrabberKeywords[] = {
	{VIDEOGRABBER_OPEN,"OPEN"},
	{VIDEOGRABBER_CLOSE,"CLOSE"},
	{VIDEOGRABBER_SHOW,"SHOW"},
	{VIDEOGRABBER_HIDE,"HIDE"},
	{VIDEOGRABBER_STARTRECORDING,"STARTRECORDING"},
	{VIDEOGRABBER_STOPRECORDING,"STOPRECORDING"},
	{VIDEOGRABBER_MOVE,"MOVE"},
	{VIDEOGRABBER_DIALOG,"DIALOG"},
	{VIDEOGRABBER_GRABIMAGE,"GRABIMAGE"},
	{VIDEOGRABBER_IDLE,"IDLE"},
	{VIDEOGRABBER_SETOPTIONS,"SETOPTIONS"},
	{VIDEOGRABBER_GETOPTIONS,"GETOPTIONS"},
	{VIDEOGRABBER_SETRECT,"SETRECT"}
};

VideoGrabberKeywordRec VideoGrabberDialogKeywords[] = {
	{VIDEOGRABBER_DIALOGFORMAT,"FORMAT"},
	{VIDEOGRABBER_DIALOGSOURCE,"SOURCE"},
	{VIDEOGRABBER_DIALOGDISPLAY,"DISPLAY"},
	{VIDEOGRABBER_DIALOGCOMPRESSION,"COMPRESSION"},
	{VIDEOGRABBER_DIALOGAUDIO, "AUDIO"},
	{VIDEOGRABBER_DIALOGVIDEO, "VIDEO"}
};

#include <ctype.h>

#ifdef _MACOSX
int _stricmp(const char *s1, const char *s2)
{
    char c1, c2;
   while (1)
    {
    	c1 = tolower(*s1++);
    	c2 = tolower(*s2++);
        if (c1 < c2) return -1;
        if (c1 > c2) return 1;
        if (c1 == 0) return 0;
    }
}

int _strnicmp(const char *s1, const char *s2, int n)
{
    int i;
    char c1, c2;
    for (i=0; i<n; i++)
    {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
        if (c1 < c2) return -1;
        if (c1 > c2) return 1;
        if (!c1) return 0;
    }
    return 0;
}
#endif

inline char *BoolToStr(Bool b) 
{return b == True ? istrdup("TRUE"): istrdup("FALSE");}

inline  Bool StrToBool(char *boolstr)
{return _strnicmp(boolstr, "TRUE", 4) == 0? True: False;}

#ifdef WIN32

void *getWindowId(char *p_stack_or_window_reference)
{
	// If p_stack_or_window_reference is an integer, we assume it to be the windowId of a Revolution stack
	// and return it as a pointer
	char *t_end_pointer;
	long t_window_id;
	t_window_id = strtol(p_stack_or_window_reference, &t_end_pointer, 10);
	if (t_end_pointer == NULL)
		return (void *)t_window_id;
	
	// If p_stack_or_window_reference is not an integer, but is not empty or NULL then we attempt to
	// resolve it as a stack reference using EvalExp.
	
	const char t_expression_template[] = "the windowId of stack \0";
	char *t_expression;
	t_expression = (char *)malloc(strlen(t_expression_template) + strlen(p_stack_or_window_reference) + 3);
	sprintf(t_expression, "%s\"%s\"\0", t_expression_template, p_stack_or_window_reference);
	
	int t_success;
	t_success = 0;
	
	char *t_result;
	t_result = EvalExpr(t_expression, &t_success);
	t_window_id = strtol(t_result, &t_end_pointer, 10);
	if (t_end_pointer == NULL)
		return (void *)t_window_id;
	
	// If neither of the two above options succeed we revert to the old behavior and return the foremost window's id.
	return GetForegroundWindow();

}

#else

void *getWindowId(char *p_stack_or_window_reference)
{
	// If p_stack_or_window_reference is an integer, we assume it to be the windowId of a Revolution stack
	// and return it as a pointer
	char *t_end_pointer;
	long t_window_id;
	t_window_id = strtol(p_stack_or_window_reference, &t_end_pointer, 10);
	if (*t_end_pointer == NULL)
		return (void *)t_window_id;
	
	// If p_stack_or_window_reference is not an integer, but is not empty or NULL then we attempt to
	// resolve it as a stack reference using EvalExp.
	const char t_expression_template[] = "the windowId of stack \0";
	char *t_expression;
	t_expression = (char *)malloc(strlen(t_expression_template) + strlen(p_stack_or_window_reference) + 3);
	sprintf(t_expression, "%s\"%s\"\0", t_expression_template, p_stack_or_window_reference);
	
	int t_success;
	t_success = 0;
	
	char *t_result;
	t_result = EvalExpr(t_expression, &t_success);
	t_window_id = strtol(t_result, &t_end_pointer, 10);
	if (*t_end_pointer == NULL)
		return (void *)t_window_id;
	
	// If neither of the two above options succeed we revert to the old behavior and return the foremost window's id.
	return FrontWindow();

}

#endif


void REVVideoGrabber(VideoGrabberKeyword whichkeyword, 
			char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error);

void revInitializeVideoGrabber(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_OPEN,args, nargs, retstring,pass, error);
}

void revPreviewVideo(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_SHOW,args, nargs, retstring,pass, error);
}

void revStopPreviewingVideo(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_HIDE,args, nargs, retstring,pass, error);
}

void revRecordVideo(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_STARTRECORDING,args, nargs, retstring,pass, error);
}

#define REVVIDEO_VERSIONSTRING "2.9.0"

void REVVideo_Version(char *args[], int nargs, char **retstring,
		   Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = False;
	*pass = False;
	 result = istrdup(REVVIDEO_VERSIONSTRING);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

void revStopRecordingVideo(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_STOPRECORDING,args, nargs, retstring,pass, error);
}

void  revVideoGrabIdle(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_IDLE,args, nargs, retstring,pass, error);
}

void  revVideoGrabDialog(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber( VIDEOGRABBER_DIALOG,args, nargs, retstring,pass, error);
}

void  revVideoGrabSettings(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_GETOPTIONS,args, nargs, retstring,pass, error);
}

void  revSetVideoGrabSettings(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_SETOPTIONS,args, nargs, retstring,pass, error);
}

void  revVideoFrameImage(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_GRABIMAGE,args, nargs, retstring,pass, error);
}


void  revCloseVideoGrabber(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_CLOSE,args, nargs, retstring,pass, error);
}

void  revSetVideoGrabberRect(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_SETRECT,args, nargs, retstring,pass, error);
}

void  revSetVideoGrabFrameRate(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_SETFRAMERATE,args, nargs, retstring,pass, error);
}


void  revSetVideoGrabFrameSize(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_SETFRAMESIZE,args, nargs, retstring,pass, error);
}



void  revGetVideoGrabFrameSize(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_GETFRAMESIZE,args, nargs, retstring,pass, error);
}

void  revGetVideoGrabFrameRate(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_GETFRAMERATE,args, nargs, retstring,pass, error);
}



void  revGetVideoGrabAudio(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_GETAUDIO,args, nargs, retstring,pass, error);
}

void  revSetVideoGrabAudio(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_SETAUDIO,args, nargs, retstring,pass, error);
}



void  revGetVideoGrabCompressor(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_GETCOMPRESSOR,args, nargs, retstring,pass, error);
}



void  revGetVideoGrabAudioCompressor(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_GETAUDIOCOMPRESSOR,args, nargs, retstring,pass, error);
}



void  revSetVideoGrabAudioCompressor(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_SETAUDIOCOMPRESSOR,args, nargs, retstring,pass, error);
}


void  revSetVideoGrabCompressor(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_SETCOMPRESSOR,args, nargs, retstring,pass, error);
}

void  revVideoGrabCompressors(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_GETCOMPRESSORS,args, nargs, retstring,pass, error);
}


void  revVideoGrabAudioCompressors(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_GETAUDIOCOMPRESSORS,args, nargs, retstring,pass, error);
}


void  revSetVideoColorSpace(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_SETCOLORSPACE,args, nargs, retstring,pass, error);
}


void revListVideoDeviceSettings(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_LISTDEVICESETTINGS, args, nargs, retstring, pass, error);
}

void revGetVideoDeviceSetting(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_GETDEVICESETTING, args, nargs, retstring, pass, error);
}

void revSetVideoDeviceSetting(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	REVVideoGrabber(VIDEOGRABBER_SETDEVICESETTING, args, nargs, retstring, pass, error);
}

static bool vg_list_settings(void *p_context, const char *p_setting)
{
	char **t_string;
	t_string = (char **)p_context;
	
	char *t_new_string;
	t_new_string = (char *)realloc(*t_string, (*t_string == nil ? 0 : strlen(*t_string)) + strlen(p_setting) + 2);
	if (t_new_string == nil)
		return false;

	if (*t_string == nil)
		strcpy(t_new_string, p_setting);
	else
	{
		strcat(t_new_string, "\n");
		strcat(t_new_string, p_setting);
	}

	*t_string = t_new_string;

	return true;
}

void REVVideoGrabber(VideoGrabberKeyword whichkeyword, 
			char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = False;
	*pass = False;
	int i;
	
	int nvideoargs = nargs;
	switch (whichkeyword)
	{
	case VIDEOGRABBER_OPEN:
		if (nvideoargs == 3)
		{
			if (!gvideograbber)
			{
				// OK-2007-10-03 : Improved to actually use the window specified by the user.
#ifdef WIN32
				HWND windowid;
				windowid = (HWND)getWindowId(args[0]);
#else
				WindowPtr windowid;
				windowid = (WindowPtr)getWindowId(args[0]);
#endif



                // SN-2015-04-23: [[ Bug 15255 ]] Windows now defaults to directX
#ifdef WIN32
				if (_strnicmp(args[1], "vfw",strlen("vfw")) == 0)
                    gvideograbber = new (nothrow) CWinVideoGrabber(windowid);
				else
					gvideograbber = new (nothrow) CDirectXVideoGrabber(windowid);
#else
                else if (_strnicmp(args[1], "qt",strlen("qt")) == 0)
                {
                if (stricmp(args[1], "qtx") == 0)
                    gvideograbber = CreateQTXVideoGrabber(windowid);
				else
				{
                    if (InitQT())
                        gvideograbber = new (nothrow) CQTVideoGrabber(windowid);
                    else
                    {
                        // SN-2015-04-17: [[ Bug 13452 ]] Break if qvideograbber
                        //  has not been created successfully
                        result = istrdup("ERROR: cannot load quicktime");
                        break;
                    }
                }
#endif
				int left,top,right,bottom;
                // SN-2015-04-23: [[ Bug 15255 ]] Checking that qtvideograbber
                //  is not NULL can't hurt
                if (gvideograbber && gvideograbber->IsInited()){
					if (sscanf(args[2], "%d,%d,%d,%d", &left,&top,&right,&bottom) == 4)
						gvideograbber->SetRect(left,top,right,bottom);
				}
				else
					result = istrdup("ERROR: cannot load video component");
				break;
			}
		}
		else
			*error = True;

		break;
	case VIDEOGRABBER_SETRECT:
		{
			if (nvideoargs == 1){
				if (gvideograbber){
					int left,top,right,bottom;
					if (sscanf(args[0], "%d,%d,%d,%d", &left,&top,&right,&bottom) == 4)
						gvideograbber->SetRect(left,top,right,bottom);
				}
			}
			else
				*error = True;
		}
		break;
	case VIDEOGRABBER_CLOSE: 
		if (gvideograbber){
			delete gvideograbber;
			gvideograbber = NULL;
		}
		break;
	case VIDEOGRABBER_SHOW:
		if (gvideograbber)
			gvideograbber->SetVisible(True);
		break;
	case VIDEOGRABBER_HIDE:
		if (gvideograbber)
			gvideograbber->SetVisible(False);
		break;
	case VIDEOGRABBER_MOVE:
		if (nvideoargs == 1){
			short left,top,right,bottom;
			if (gvideograbber)
				if (sscanf(args[0], "%d,%d,%d,%d", &left,&top,&right,&bottom) == 4)
					gvideograbber->SetRect(left,top,right,bottom);
		}
		else 
			*error = True;
		break;
	case VIDEOGRABBER_STARTRECORDING:
		if (nvideoargs == 1)
		{
			if (gvideograbber)
			{
				// OK-2008-01-24 : Bug 5280
				char *t_native_path;
				t_native_path = os_path_to_native(args[0]);

				char *t_resolved_path;
				t_resolved_path = os_path_resolve(t_native_path);
				free(t_native_path);

				gvideograbber -> StartRecording(t_resolved_path);
				free(t_resolved_path);
			}
		}
		else 
			*error = True;
		break;
	case VIDEOGRABBER_STOPRECORDING:
		if (gvideograbber)
			gvideograbber->StopRecording();
		break;
	case VIDEOGRABBER_IDLE:
		if (gvideograbber)
			gvideograbber->DoIdle();
		break;
	case VIDEOGRABBER_DIALOG:
		if (nvideoargs == 1)
		{
			if (gvideograbber)
			{
				int numdialogoptions;
				numdialogoptions = sizeof(VideoGrabberDialogKeywords) / sizeof(VideoGrabberKeywordRec);
				VideoGrabberKeyword whichdialog;
				whichdialog = VIDEOGRABBER_UNDEFINED;
				int proplen;
				proplen = strlen(args[0]);

				for (i = 0; i < numdialogoptions; i++)
				{
					if (_strnicmp(args[0], VideoGrabberDialogKeywords[i].str,proplen) == 0 )
					{
						whichdialog = VideoGrabberDialogKeywords[i].prop;
						break;
					}	
				}
				switch (whichdialog)
				{
				case VIDEOGRABBER_DIALOGFORMAT:
					gvideograbber->VideoFormatDialog();
					break;
				case VIDEOGRABBER_DIALOGSOURCE:
					gvideograbber->VideoSourceDialog();
					break;
				case VIDEOGRABBER_DIALOGDISPLAY:
					gvideograbber->VideoDisplayDialog();
					break;
				case VIDEOGRABBER_DIALOGCOMPRESSION:
					gvideograbber->VideoCompressionDialog();
					break;
				case VIDEOGRABBER_DIALOGAUDIO:
					gvideograbber->AudioDefaultDialog();
					break;
				case VIDEOGRABBER_DIALOGVIDEO:
					gvideograbber->VideoDefaultDialog();
				default:
					break;
				}
			}
		}
		// OK-2007-09-25 : Bug 5344 : If no argument provided we show the default dialog. Each driver chooses the most appropriate.
		else if (nvideoargs == 0)
		{
			if (gvideograbber != NULL)
				gvideograbber->VideoDefaultDialog();
		}
		else 
			*error = True;
		break;
	case VIDEOGRABBER_SETOPTIONS:
		if (nvideoargs == 1){
			// MW-2010-08-04: [[ Bug 8889 ]] Make sure we actually get something back to pass!
			if (gvideograbber){
				ExternalString val;
				int retvalue;
				val.buffer = nil;
				val.length = 0;
				retvalue = EXTERNAL_FAILURE;
				GetVariableEx(args[0],"",&val,&retvalue);
				if (retvalue == EXTERNAL_SUCCESS && val.buffer != NULL)
					gvideograbber->SetSettingsString(val);
			}
		}
		else
			*error = True;
		break;
	case VIDEOGRABBER_GETAUDIO:

			int channels,  bits,  frequency;
			if (gvideograbber){
				Bool capaudio = gvideograbber->GetAudioCapture();
				gvideograbber->getAudioFormat(&channels,&bits,&frequency);
				result = (char *)malloc(255);
				sprintf(result,"%s,%d,%d,%d", capaudio == True? "TRUE": "FALSE", channels, bits, frequency);
			}
	
		break;
	case VIDEOGRABBER_SETAUDIO:
		if (nvideoargs == 4){
			if (gvideograbber){
				gvideograbber->SetAudioCapture(StrToBool(args[0]));
				gvideograbber->setAudioFormat(atoi(args[1]),atoi(args[2]), atoi(args[3]));
			}
		}
		else
			*error = True;
		break;
  	case VIDEOGRABBER_GETFRAMERATE:
		{
			double framerate = 0.0;
			if (gvideograbber)
				gvideograbber->GetFrameRate(&framerate);
			result = (char *)malloc(32);
			sprintf(result,"%f",framerate);
		}
		break;
		case VIDEOGRABBER_GETFRAMESIZE:
		{
			int fwidth,fheight;
            fwidth = 0;
            fheight = 0;
			if (gvideograbber)
				gvideograbber->GetFrameSize(&fwidth,&fheight);
			result = (char *)malloc(60);
			sprintf(result,"%d,%d",fwidth,fheight);
		}
		break;
		case VIDEOGRABBER_SETCOLORSPACE:
			{
				if (nvideoargs == 1){
#ifdef WIN32
					CDirectXVideoGrabber *tgrabber = (CDirectXVideoGrabber *)gvideograbber;
						if (tgrabber)
								tgrabber->SetColorSpaceFormat((E_ColorSpaceFormat)atoi(args[0]));
#endif
				}
				else
					*error = True;
			}
				break;
		case VIDEOGRABBER_SETFRAMESIZE:
		{
				if (nvideoargs == 2){
						if (gvideograbber)
								gvideograbber->SetFrameSize(atoi(args[0]),atoi(args[1]));
				}
				else
						*error = True;
		}
		break;
		case VIDEOGRABBER_SETFRAMERATE:
				if (nvideoargs == 1){
						if (gvideograbber)
								gvideograbber->SetFrameRate(atoi(args[0]));
				}
				else
						*error = True;
				break;
		case VIDEOGRABBER_GETCOMPRESSOR:
				if (gvideograbber){
				char *tchar = gvideograbber->GetCurrentCodecName();
				result = istrdup(tchar);
				}
				break;
		case VIDEOGRABBER_GETAUDIOCOMPRESSOR:
				if (gvideograbber){
				char *tchar = gvideograbber->GetCurrentAudioCodecName();
				result = istrdup(tchar);
				}
				break;
		case VIDEOGRABBER_SETAUDIOCOMPRESSOR:
				if (nvideoargs == 1){
						if (gvideograbber)
								gvideograbber->setAudioCompressor(args[0]);
				}
				else
						*error = True;
				break;
		case VIDEOGRABBER_SETCOMPRESSOR:
				if (nvideoargs == 1){
						if (gvideograbber)
								gvideograbber->setCompressor(args[0]);
				}
				else
						*error = True;
				break;
	case VIDEOGRABBER_GETCOMPRESSORS:
		if (gvideograbber){
				char *complist = gvideograbber->getCompressors();
				if (complist)
					result = complist;
		}
		break;
   case VIDEOGRABBER_GETAUDIOCOMPRESSORS:
		if (gvideograbber){
				char *complist = gvideograbber->getAudioCompressors();
				if (complist)
					result = complist;
		}
		break;
	case VIDEOGRABBER_GETOPTIONS:
		if (nvideoargs == 1){
			if (gvideograbber){
				ExternalString val;
				int retvalue;
				val.buffer = NULL;
				gvideograbber->GetSettingsString(val);
				if (val.buffer != NULL)
					SetVariableEx(args[0],"",&val,&retvalue);
			}
		}
		else
			*error = True;
		break;
	case VIDEOGRABBER_LISTDEVICESETTINGS:
		if (nvideoargs == 0)
		{
			if (gvideograbber)
			{
				char *t_result;
				t_result = nil;
				if (!gvideograbber -> ListSettings(vg_list_settings, &t_result))
				{
					free(t_result);
					result = strdup("");
				}
				else
					result = t_result;
			}
		}
		else
			*error = True;
		break;
	case VIDEOGRABBER_GRABIMAGE:
		if (nvideoargs == 3){
			int twidth = atoi(args[0]);
			int theight = atoi(args[1]);
			ExternalString videoimagedata;
			Bool res = False;
			MYBITMAP *videoimage = createmybitmap(32, twidth, theight);
			if (gvideograbber){
				//copy from cardpixmap to image pixmap (which  maps to bit buffer)
				#ifdef WIN32
				 HDC desktophdc,desthdc;	
				 desktophdc = GetDC(NULL);
				 desthdc = CreateCompatibleDC(desktophdc);
				 ReleaseDC(NULL, desktophdc);
				 HBITMAP odbm = (HBITMAP)SelectObject(desthdc, videoimage->bm);
				 res = gvideograbber->Draw(twidth,theight,desthdc);
				 SelectObject(desthdc, odbm);
				 DeleteDC(desthdc);
				 videoimagedata.buffer = (char *)videoimage->data;
					videoimagedata.length = videoimage->bytes_per_line * 
						videoimage->height;
				#else
				  res = gvideograbber->Draw(twidth,theight,videoimage->bm);
					PixMapHandle destpm = GetGWorldPixMap(videoimage->bm);
			       LockPixels(destpm);  
			      int destbpl = ((videoimage->width * 32 + 31) >> 3) & 0xFFFFFFFC;
    		      uint1 *destdata = (uint1 *)new char[destbpl * videoimage->height];
    		     uint1 *dptr = destdata;
    		   uint1 *sptr = videoimage->data;
    		uint4 cbpl = (videoimage->width * 32 + 7) >> 3; 
    		uint2 h = videoimage->height;
    		while (h--) {
      		memcpy(dptr, sptr, cbpl);
      		sptr += videoimage->bytes_per_line;
      		dptr += destbpl;
    		}
			videoimagedata.buffer = (char *)destdata;
			videoimagedata.length = destbpl  * videoimage->height;
			 UnlockPixels(destpm);
				#endif
				
				if (res)
				{
					int retvalue;
					uint4 *fourptr = (uint4 *)	videoimagedata.buffer;
					if (videoimagedata.length > 0){
						uint4 len = videoimagedata.length;
						while (len){
							swap_uint4(fourptr++);
							len-=sizeof(uint4);
						}
					}
					SetVariableEx(args[2],"",&videoimagedata,&retvalue);
				}
				else
					result = istrdup("ERROR: can't get imagedata");
				
				destroymybitmap(videoimage);
			}
		}
		else
			*error = True;
		break;
	default:
		*error = True;
		result = istrdup("ERROR: Video Grabber command not found");
		}
	
		if (result == NULL)
			*retstring = istrdup("");
		else
			*retstring = result;
}

#ifdef _MACOSX
void revCaptureBeginSession(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureEndSession(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureListAudioInputs(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureListVideoInputs(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureGetAudioInput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureSetAudioInput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureGetVideoInput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureSetVideoInput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureGetPreviewImage(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureSetPreviewImage(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureGetPreviewVolume(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureSetPreviewVolume(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureStartPreviewing(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureStopPreviewing(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCapturePausePreviewing(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureResumePreviewing(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCapturePreviewState(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);

void revCaptureListAudioCodecs(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureListVideoCodecs(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureGetAudioCodec(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureSetAudioCodec(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureGetVideoCodec(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureSetVideoCodec(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureGetRecordOutput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureSetRecordOutput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureStartRecording(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureStopRecording(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureCancelRecording(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCapturePauseRecording(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureResumeRecording(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureGetRecordFrameRate(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureSetRecordFrameRate(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureGetRecordFrameSize(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureSetRecordFrameSize(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
void revCaptureRecordState(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error);
#endif

EXTERNAL_BEGIN_DECLARATIONS("revVideoGrabber")
	EXTERNAL_DECLARE_COMMAND("revInitializeVideoGrabber", revInitializeVideoGrabber)
	EXTERNAL_DECLARE_COMMAND("revPreviewVideo", revPreviewVideo)
	EXTERNAL_DECLARE_COMMAND("revStopPreviewingVideo", revStopPreviewingVideo)
	EXTERNAL_DECLARE_COMMAND("revRecordVideo", revRecordVideo)
	EXTERNAL_DECLARE_COMMAND("revStopRecordingVideo", revStopRecordingVideo)
	EXTERNAL_DECLARE_COMMAND("revVideoGrabIdle", revVideoGrabIdle)
	EXTERNAL_DECLARE_COMMAND("revVideoGrabDialog", revVideoGrabDialog)
	EXTERNAL_DECLARE_COMMAND("revVideoGrabSettings", revVideoGrabSettings)
	EXTERNAL_DECLARE_COMMAND("revVideoFrameImage", revVideoFrameImage)
	EXTERNAL_DECLARE_COMMAND("revCloseVideoGrabber", revCloseVideoGrabber)
	EXTERNAL_DECLARE_COMMAND("revSetVideoGrabSettings", revSetVideoGrabSettings)
	EXTERNAL_DECLARE_COMMAND("revSetVideoGrabberRect", revSetVideoGrabberRect)

	EXTERNAL_DECLARE_FUNCTION("revvideo_version", REVVideo_Version)

	EXTERNAL_DECLARE_COMMAND("revVideoGrabCompressors", revVideoGrabCompressors)
	EXTERNAL_DECLARE_COMMAND("revVideoGrabAudioCompressors", revVideoGrabAudioCompressors)

	EXTERNAL_DECLARE_COMMAND("revSetVideoGrabFrameRate", revSetVideoGrabFrameRate)
	EXTERNAL_DECLARE_COMMAND("revGetVideoGrabFrameRate", revGetVideoGrabFrameRate)

	EXTERNAL_DECLARE_COMMAND("revSetVideoGrabAudio", revSetVideoGrabAudio)
	EXTERNAL_DECLARE_COMMAND("revGetVideoGrabAudio", revGetVideoGrabAudio)

	EXTERNAL_DECLARE_COMMAND("revSetVideoGrabCompressor", revSetVideoGrabCompressor)
	EXTERNAL_DECLARE_COMMAND("revGetVideoGrabCompressor", revGetVideoGrabCompressor)

	EXTERNAL_DECLARE_COMMAND("revSetVideoGrabAudioCompressor", revSetVideoGrabAudioCompressor)
	EXTERNAL_DECLARE_COMMAND("revGetVideoGrabAudioCompressor", revGetVideoGrabAudioCompressor)

	EXTERNAL_DECLARE_COMMAND("revSetVideoColorSpace", revSetVideoColorSpace)

	EXTERNAL_DECLARE_COMMAND("revSetVideoGrabFrameSize", revSetVideoGrabFrameSize)
	EXTERNAL_DECLARE_COMMAND("revGetVideoGrabFrameSize", revGetVideoGrabFrameSize)

#if defined(_MACOSX)
	EXTERNAL_DECLARE_COMMAND("revCaptureBeginSession", revCaptureBeginSession)
	EXTERNAL_DECLARE_COMMAND("revCaptureEndSession", revCaptureEndSession)

	EXTERNAL_DECLARE_FUNCTION("revCaptureListAudioInputs", revCaptureListAudioInputs)
	EXTERNAL_DECLARE_FUNCTION("revCaptureListVideoInputs", revCaptureListVideoInputs)
	EXTERNAL_DECLARE_FUNCTION("revCaptureGetAudioInput", revCaptureGetAudioInput)
	EXTERNAL_DECLARE_COMMAND("revCaptureSetAudioInput", revCaptureSetAudioInput)
	EXTERNAL_DECLARE_FUNCTION("revCaptureGetVideoInput", revCaptureGetVideoInput)
	EXTERNAL_DECLARE_COMMAND("revCaptureSetVideoInput", revCaptureSetVideoInput)

	EXTERNAL_DECLARE_FUNCTION("revCaptureGetPreviewImage", revCaptureGetPreviewImage)
	EXTERNAL_DECLARE_COMMAND("revCaptureSetPreviewImage", revCaptureSetPreviewImage)

	EXTERNAL_DECLARE_FUNCTION("revCaptureGetPreviewVolume", revCaptureGetPreviewVolume)
	EXTERNAL_DECLARE_COMMAND("revCaptureSetPreviewVolume", revCaptureSetPreviewVolume)

	EXTERNAL_DECLARE_COMMAND("revCaptureStartPreviewing", revCaptureStartPreviewing)
	EXTERNAL_DECLARE_COMMAND("revCaptureStopPreviewing", revCaptureStopPreviewing)
	//EXTERNAL_DECLARE_COMMAND("revCapturePausePreviewing", revCapturePausePreviewing)
	//EXTERNAL_DECLARE_COMMAND("revCaptureResumePreviewing", revCaptureResumePreviewing)

	EXTERNAL_DECLARE_FUNCTION("revCapturePreviewState", revCapturePreviewState)

	EXTERNAL_DECLARE_FUNCTION("revCaptureListAudioCodecs", revCaptureListAudioCodecs)
	EXTERNAL_DECLARE_FUNCTION("revCaptureListVideoCodecs", revCaptureListVideoCodecs)
	EXTERNAL_DECLARE_FUNCTION("revCaptureGetAudioCodec", revCaptureGetAudioCodec)
	EXTERNAL_DECLARE_COMMAND("revCaptureSetAudioCodec", revCaptureSetAudioCodec)
	EXTERNAL_DECLARE_FUNCTION("revCaptureGetVideoCodec", revCaptureGetVideoCodec)
	EXTERNAL_DECLARE_COMMAND("revCaptureSetVideoCodec", revCaptureSetVideoCodec)

	EXTERNAL_DECLARE_FUNCTION("revCaptureGetRecordOutput", revCaptureGetRecordOutput)
	EXTERNAL_DECLARE_COMMAND("revCaptureSetRecordOutput", revCaptureSetRecordOutput)
	EXTERNAL_DECLARE_FUNCTION("revCaptureGetRecordFrameRate", revCaptureGetRecordFrameRate)
	EXTERNAL_DECLARE_COMMAND("revCaptureSetRecordFrameRate", revCaptureSetRecordFrameRate)
	EXTERNAL_DECLARE_FUNCTION("revCaptureGetRecordFrameSize", revCaptureGetRecordFrameSize)
	EXTERNAL_DECLARE_COMMAND("revCaptureSetRecordFrameSize", revCaptureSetRecordFrameSize)

	EXTERNAL_DECLARE_COMMAND("revCaptureStartRecording", revCaptureStartRecording)
	EXTERNAL_DECLARE_COMMAND("revCaptureStopRecording", revCaptureStopRecording)
	//EXTERNAL_DECLARE_COMMAND("revCaptureCancelRecording", revCaptureCancelRecording)
	//EXTERNAL_DECLARE_COMMAND("revCapturePauseRecording", revCapturePauseRecording)
	//EXTERNAL_DECLARE_COMMAND("revCaptureResumeRecording", revCaptureResumeRecording)

	EXTERNAL_DECLARE_FUNCTION("revCaptureRecordState", revCaptureRecordState)
#endif

EXTERNAL_END_DECLARATIONS


#ifdef WIN32
HINSTANCE hInstance = NULL;

BOOL WINAPI DllMain(HINSTANCE tInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		hInstance = tInstance;
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
	}
	return TRUE;
}
#endif
