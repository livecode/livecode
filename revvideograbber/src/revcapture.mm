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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <revolution/external.h>

#include <Carbon/Carbon.h>
#include <Foundation/Foundation.h>

#include "revcapture.h"

////////////////////////////////////////////////////////////////////////

//struct NSString;

extern "C" void rreCaptureBeginSession(void);
extern "C" void rreCaptureEndSession(void);

extern "C" NSString *rreCaptureListAudioInputs(void);
extern "C" NSString *rreCaptureListVideoInputs(void);
extern "C" NSString *rreCaptureGetAudioInput(void);
extern "C" void rreCaptureSetAudioInput(NSString *);
extern "C" NSString *rreCaptureGetVideoInput(void);
extern "C" void rreCaptureSetVideoInput(NSString *);

extern "C" int32_t rreCaptureGetPreviewVolume(void);
extern "C" void rreCaptureSetPreviewVolume(int32_t);
extern "C" NSString *rreCaptureGetPreviewImage(void);
extern "C" void rreCaptureSetPreviewImage(NSString *);

extern "C" void rreCaptureStartPreviewing(void);
extern "C" void rreCaptureStopPreviewing(void);
extern "C" void rreCapturePausePreviewing(void);
extern "C" void rreCaptureResumePreviewing(void);

extern "C" NSString *rreCapturePreviewState(void);

/////////

extern "C" NSString *rreCaptureListAudioCodecs(void);
extern "C" NSString *rreCaptureListVideoCodecs(void);
extern "C" NSString *rreCaptureGetAudioCodec(void);
extern "C" void rreCaptureSetAudioCodec(NSString *);
extern "C" NSString *rreCaptureGetVideoCodec(void);
extern "C" void rreCaptureSetVideoCodec(NSString *);

extern "C" NSString *rreCaptureGetRecordOutput(void);
extern "C" void rreCaptureSetRecordOutput(NSString *);

extern "C" NSString *rreCaptureStartRecording(void);
extern "C" NSString *rreCaptureStopRecording(void);
extern "C" void rreCaptureCancelRecording(void);
extern "C" void rreCapturePauseRecording(void);
extern "C" void rreCaptureResumeRecording(void);

extern "C" int rreCaptureGetRecordFrameRate(void);
extern "C" void rreCaptureSetRecordFrameRate(int);
extern "C" NSString *rreCaptureGetRecordFrameSize(void);
extern "C" void rreCaptureSetRecordFrameSize(int, int);

extern "C" NSString *rreCaptureRecordState(void);

////////////////////////////////////////////////////////////////////////

extern "C" void ClearException(void);
extern "C" void ThrowException(const char *error);

static const char *s_exception = nil;

void ClearException(void)
{
	s_exception = nil;
}

void ThrowException(const char *p_exception)
{
	s_exception = p_exception;
}

bool CatchException(char **r_result, Bool *r_error)
{
	if (s_exception == nil)
		return false;
	
	*r_result = strdup(s_exception);
	*r_error = True;
	
	return true;
}

static void CatchSyntaxError(char **r_result, Bool *r_error)
{
	*r_result = strdup("syntax error");
	*r_error = True;
}

static void CatchObjCException(id p_error, char **r_result, Bool *r_error)
{
	if ([p_error isKindOfClass: [NSException class]])
	{
		*r_result = strdup([[p_error reason] cStringUsingEncoding: NSMacOSRomanStringEncoding]);
	}
	else
	{
		*r_result = strdup("unknown objc exception");
		*r_error = True;
	}
}

////////////////////////////////////////////////////////////////////////

static bool CStringFormat(char*& r_string, const char *p_format, ...)
{
	va_list t_args;
	int t_count;
#ifdef _WINDOWS
	va_start(t_args, p_format);
	t_count = _vscprintf(p_format, t_args);
	va_end(t_args);
#elif defined(_MACOSX) || defined(_LINUX)
	va_start(t_args, p_format);
	t_count = vsnprintf(nil, 0, p_format, t_args);
	va_end(t_args);
#else
#error "Implement MCCStringFormat"
#endif
	
	char *t_new_string;
	t_new_string = (char *)malloc(t_count + 1);
	if (t_new_string == nil)
		return false;
	
	va_start(t_args, p_format);
	vsprintf(t_new_string, p_format, t_args);
	va_end(t_args);
	
	r_string = t_new_string;
	
	return true;
}

static bool CStringFormatV(char*& r_string, const char *p_format, va_list p_args)
{
	int t_count;
#ifdef _WINDOWS
	t_count = _vscprintf(p_format, p_args);
#elif defined(_MACOSX) || defined(_LINUX)
	t_count = vsnprintf(nil, 0, p_format, p_args);
#else
#error "Implement CStringFormatV"
#endif
	
	char *t_new_string;
	t_new_string = (char *)malloc(t_count + 1);
	if (t_new_string == nil)
		return false;
	
	vsprintf(t_new_string, p_format, p_args);
	
	r_string = t_new_string;
	
	return true;
	
}

static void CStringFree(char *p_string)
{
	free(p_string);
}

static bool EvaluateLiveCodeExpressionV(char*& r_result, const char *p_format, va_list p_args)
{
	bool t_success;
	t_success = true;
	
	char *t_expr;
	t_expr = nil;
	if (t_success)
		t_success = CStringFormatV(t_expr, p_format, p_args);
	
	char *t_result;
	t_result = nil;
	if (t_success)
	{
		int t_retval;
		t_result = EvalExpr(t_expr, &t_retval);
		if (t_retval == EXTERNAL_FAILURE)
			t_success = false;
	}
	
	if (t_success)
		r_result = t_result;
	
	CStringFree(t_expr);
	
	return t_success;
}

static bool EvaluateLiveCodeExpression(char*& r_result, const char *p_format, ...)
{
	bool t_success;
	t_success = true;
	
	va_list t_args;
	va_start(t_args, p_format);
	t_success = EvaluateLiveCodeExpressionV(r_result, p_format, t_args);
	va_end(t_args);
	
	return t_success;
}

static bool EvaluateLiveCodeExpressionAsInt(int32_t& r_result, const char *p_format, ...)
{
	bool t_success;
	t_success = true;
	
	char *t_string_result;
	va_list t_args;
	va_start(t_args, p_format);
	t_success = EvaluateLiveCodeExpressionV(t_string_result, p_format, t_args);
	va_end(t_args);
	
	if (t_success)
		r_result = atoi(t_string_result);
	
	CStringFree(t_string_result);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////

extern "C" BOOL LockLiveCodeImage(const char *p_image_id, GWorldPtr* r_gworld);
extern "C" void UnlockLiveCodeImage(const char *p_image_id, GWorldPtr* x_gworld);
extern "C" void UpdateLiveCodeImage(const char *p_image_id);
extern "C" BOOL ExecuteLiveCodeScript(const char *p_format, ...);

static bool ResolveImageId(const char *p_image_ref, char*& r_image_id)
{
	if (!EvaluateLiveCodeExpression(r_image_id, "the long id of %s", p_image_ref))
	{
		ThrowException("image not found");
		return false;
	}
	
	return true;
	
}

static bool GetWidthAndHeightOfImageId(const char *p_image_ref, int32_t& r_width, int32_t& r_height)
{
	if (!EvaluateLiveCodeExpressionAsInt(r_width, "the width of %s", p_image_ref) ||
		!EvaluateLiveCodeExpressionAsInt(r_height, "the height of %s", p_image_ref))
	{
		ThrowException("could not get image dimensions");
		return false;
	}
	return true;
}

static bool SetImageGWorld(const char *p_image_ref, GWorldPtr p_gworld)
{
	if (p_gworld == nil)
		ExecuteLiveCodeScript("set the imagePixmapId of %s to empty", p_image_ref);
	else
	{
		if (!ExecuteLiveCodeScript("set the imagePixmapId of %s to %u", p_image_ref, p_gworld))
		{
			ThrowException("could not configure image buffer");
			return false;
		}
	}
	
	return true;
}

BOOL ExecuteLiveCodeScript(const char *p_format, ...)
{
	bool t_success;
	t_success = true;
	
	char *t_script;
	t_script = nil;
	if (t_success)
	{
		va_list t_args;
		va_start(t_args, p_format);
		t_success = CStringFormatV(t_script, p_format, t_args);
		va_end(t_args);
	}
	
	if (t_success)
	{
		int t_retval;
		SendCardMessage(t_script, &t_retval);
		if (t_retval == EXTERNAL_FAILURE)
			t_success = false;
	}
	
	CStringFree(t_script);
	
	return t_success ? YES : NO;
	
}

BOOL LockLiveCodeImage(const char *p_image_id, GWorldPtr* r_gworld)
{
	bool t_success;
	t_success = true;
	
	char *t_resolved_image_id;
	t_resolved_image_id = nil;
	if (t_success)
		t_success = ResolveImageId(p_image_id, t_resolved_image_id);
	
	int32_t t_width, t_height;
	if (t_success)
		t_success = GetWidthAndHeightOfImageId(t_resolved_image_id, t_width, t_height);
	
	GWorldPtr t_new_gworld;
	t_new_gworld = nil;
	if (t_success)
	{
		Rect t_rect;
		SetRect(&t_rect, 0, 0, t_width, t_height);
		if (NewGWorld(&t_new_gworld, 32, &t_rect, NULL, NULL, kNativeEndianPixMap) != noErr)
		{
			ThrowException("could not create image buffer");
			t_success = false;
		}
	}
	
	if (t_success)
		t_success = SetImageGWorld(t_resolved_image_id, t_new_gworld);
	
	if (t_success)
		*r_gworld = t_new_gworld;
	else
	{
		if (t_new_gworld != nil)
			DisposeGWorld(t_new_gworld);
	}
	
	free(t_resolved_image_id);
	
	return t_success ? YES : NO;
}

void UnlockLiveCodeImage(const char *p_image_id, GWorldPtr* x_gworld)
{
	if (*x_gworld == nil)
		return;
	
	char *t_resolved_image_id;
	t_resolved_image_id = nil;
	if (ResolveImageId(p_image_id, t_resolved_image_id))
		SetImageGWorld(t_resolved_image_id, nil);
	
	DisposeGWorld(*x_gworld);
	free(t_resolved_image_id);
	
	*x_gworld = nil;
}

void UpdateLiveCodeImage(const char *p_image_id)
{
	int t_success;
	ShowImageByLongId(p_image_id, &t_success);
}

////////////////////////////////////////////////////////////////////////

typedef void (*VoidMethodPtr)(void);
typedef void (*VoidMethodWithObjCStringPtr)(NSString *);
typedef void (*VoidMethodWithIntPtr)(int);
typedef void (*VoidMethodWithIntIntPtr)(int, int);
typedef NSString *(*ObjCStringMethodPtr)(void);
typedef int32_t (*IntMethodPtr)(void);

static void CallVoidMethod(VoidMethodPtr p_method, char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	*r_result = nil;
	*r_error = False;
	*r_pass = False;
	
	if (p_argn != 0)
	{
		CatchSyntaxError(r_result, r_error);
		return;
	}
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	@try
	{
		ClearException();
		p_method();
		CatchException(r_result, r_error);
	}
	@catch(id t_error)
	{
		CatchObjCException(t_error, r_result, r_error);
	}
	
	[t_pool release];
}

static void CallObjCStringMethod(ObjCStringMethodPtr p_method, char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	*r_result = nil;
	*r_error = False;
	*r_pass = False;
	
	if (p_argn != 0)
	{
		CatchSyntaxError(r_result, r_error);
		return;
	}
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	@try
	{
		ClearException();
		
		NSString *t_method_result;
		t_method_result = p_method();
		if (!CatchException(r_result, r_error))
        {
            // SN-2015-01-22: [[ Bug 14423 ]] Convert to UTF-8 by default, because we
            //  want to return the actual codec names, nothing with '?'
            char *t_string;
            NSData* t_data;
            t_data = [t_method_result dataUsingEncoding: NSUTF8StringEncoding
                                   allowLossyConversion:True];
            
            t_string = new char[[t_data length] + 1];
            
            if (t_string == NULL)
                *r_result = NULL;
            else
            {
                memcpy((void*)t_string, (const void*)[t_data bytes], [t_data length]);
                t_string [[t_data length]] = '\0';
                *r_result = t_string;
            }
        }
	}
	@catch(id t_error)
	{
		CatchObjCException(t_error, r_result, r_error);
	}
	
	[t_pool release];
}

static void CallIntMethod(IntMethodPtr p_method, char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	*r_result = nil;
	*r_error = False;
	*r_pass = False;
	
	if (p_argn != 0)
	{
		CatchSyntaxError(r_result, r_error);
		return;
	}
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	@try
	{
		ClearException();
		
		int t_method_result;
		t_method_result = p_method();
		
		if (!CatchException(r_result, r_error))
		{
			char t_buffer[16];
			sprintf(t_buffer, "%d", p_method());
			*r_result = strdup(t_buffer);
		}
	}
	@catch(id t_error)
	{
		CatchObjCException(t_error, r_result, r_error);
	}
	
	[t_pool release];
}

static void CallVoidMethodWithObjCString(VoidMethodWithObjCStringPtr p_method, char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	*r_result = nil;
	*r_error = False;
	*r_pass = False;
	
	if (p_argn != 1)
	{
		CatchSyntaxError(r_result, r_error);
		return;
	}
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	@try
	{
		ClearException();
		
		p_method([NSString stringWithCString: p_argc[0] encoding: NSUTF8StringEncoding]);
		
		CatchException(r_result, r_error);
	}
	@catch(id t_error)
	{
		CatchObjCException(t_error, r_result, r_error);
	}
	
	[t_pool release];
}

static void CallVoidMethodWithInt(VoidMethodWithIntPtr p_method, char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	*r_result = nil;
	*r_error = False;
	*r_pass = False;
	
	if (p_argn != 1)
	{
		CatchSyntaxError(r_result, r_error);
		return;
	}
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	@try
	{
		ClearException();
		
		p_method(atoi(p_argc[0]));
		
		CatchException(r_result, r_error);
	}
	@catch(id t_error)
	{
		CatchObjCException(t_error, r_result, r_error);
	}
	
	[t_pool release];
}

static void CallVoidMethodWithIntInt(VoidMethodWithIntIntPtr p_method, char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	*r_result = nil;
	*r_error = False;
	*r_pass = False;
	
	if (p_argn != 2)
	{
		CatchSyntaxError(r_result, r_error);
		return;
	}
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	@try
	{
		ClearException();
		
		p_method(atoi(p_argc[0]), atoi(p_argc[1]));
		
		CatchException(r_result, r_error);
	}
	@catch(id t_error)
	{
		CatchObjCException(t_error, r_result, r_error);
	}
	
	[t_pool release];
}

////////////////////////////////////////////////////////////////////////

void revCaptureBeginSession(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethod(rreCaptureBeginSession, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureEndSession(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethod(rreCaptureEndSession, p_argc, p_argn, r_result, r_pass, r_error);
}

////////////////////

void revCaptureListAudioInputs(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureListAudioInputs, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureListVideoInputs(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureListVideoInputs, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureGetAudioInput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureGetAudioInput, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureSetAudioInput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethodWithObjCString(rreCaptureSetAudioInput, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureGetVideoInput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureGetVideoInput, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureSetVideoInput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethodWithObjCString(rreCaptureSetVideoInput, p_argc, p_argn, r_result, r_pass, r_error);
}

////////////////////

void revCaptureGetPreviewVolume(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallIntMethod(rreCaptureGetPreviewVolume, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureSetPreviewVolume(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethodWithInt(rreCaptureSetPreviewVolume, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureGetPreviewImage(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureGetPreviewImage, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureSetPreviewImage(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethodWithObjCString(rreCaptureSetPreviewImage, p_argc, p_argn, r_result, r_pass, r_error);
}

////////////////////

void revCaptureStartPreviewing(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethod(rreCaptureStartPreviewing, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureStopPreviewing(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethod(rreCaptureStopPreviewing, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCapturePausePreviewing(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethod(rreCapturePausePreviewing, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureResumePreviewing(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethod(rreCaptureResumePreviewing, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCapturePreviewState(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCapturePreviewState, p_argc, p_argn, r_result, r_pass, r_error);
}

////////////////////////////////////////////////////////////////////////

void revCaptureListAudioCodecs(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureListAudioCodecs, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureListVideoCodecs(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureListVideoCodecs, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureGetAudioCodec(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureGetAudioCodec, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureSetAudioCodec(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethodWithObjCString(rreCaptureSetAudioCodec, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureGetVideoCodec(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureGetVideoCodec, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureSetVideoCodec(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethodWithObjCString(rreCaptureSetVideoCodec, p_argc, p_argn, r_result, r_pass, r_error);
}

////////////////////

void revCaptureGetRecordOutput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureGetRecordOutput, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureSetRecordOutput(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethodWithObjCString(rreCaptureSetRecordOutput, p_argc, p_argn, r_result, r_pass, r_error);
}

////////////////////

void revCaptureStartRecording(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureStartRecording, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureStopRecording(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureStopRecording, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureCancelRecording(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethod(rreCaptureCancelRecording, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCapturePauseRecording(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethod(rreCapturePauseRecording, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureResumeRecording(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethod(rreCaptureResumeRecording, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureGetRecordFrameRate(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallIntMethod(rreCaptureGetRecordFrameRate, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureSetRecordFrameRate(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethodWithInt(rreCaptureSetRecordFrameRate, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureGetRecordFrameSize(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureGetRecordFrameSize, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureSetRecordFrameSize(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallVoidMethodWithIntInt(rreCaptureSetRecordFrameSize, p_argc, p_argn, r_result, r_pass, r_error);
}

void revCaptureRecordState(char *p_argc[], int p_argn, char **r_result, Bool *r_pass, Bool *r_error)
{
	CallObjCStringMethod(rreCaptureRecordState, p_argc, p_argn, r_result, r_pass, r_error);
}

////////////////////////////////////////////////////////////////////////
