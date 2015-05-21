/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"
//#include "execpt.h"
#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"
#include "exec.h"


#include <jni.h>
#include "mblandroidjava.h"
#include "mblandroidcontrol.h"
#include "mblandroidutil.h"

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

class MCAndroidCameraControl: public MCAndroidControl
{
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
	static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;
	
public:
	MCAndroidCameraControl(void);
	
	virtual MCNativeControlType GetType(void);
	
	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
	virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
	
	//////////
	
	void SetDevice(MCExecContext& ctxt, MCNativeControlCameraDevice p_device);
	void SetFlashMode(MCExecContext& ctxt, MCNativeControlCameraFlashMode p_mode);
	
	void GetAvailableDevices(MCExecContext& ctxt, MCNativeControlCameraDevice& r_devices);
	void GetDevice(MCExecContext& ctxt, MCNativeControlCameraDevice& r_device);
	void GetFeatures(MCExecContext& ctxt, MCNativeControlCameraFeature& r_features);
	void GetFlashMode(MCExecContext& ctxt, MCNativeControlCameraFlashMode& r_mode);
	void GetFlashIsActive(MCExecContext& ctxt, bool& r_mode);
	void GetFlashIsAvailable(MCExecContext& ctxt, bool& r_mode);
	
	// Camera-specific actions
	void ExecStartRecording(MCExecContext& ctxt);
	void ExecStopRecording(MCExecContext& ctxt);
	void ExecTakePicture(MCExecContext& ctxt);
	
protected:
	virtual ~MCAndroidCameraControl(void);
	virtual jobject CreateView(void);
	virtual void DeleteView(jobject view);
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCAndroidCameraControl::kProperties[] =
{
	DEFINE_RO_CTRL_SET_PROPERTY(P_DEVICES, NativeControlCameraDevice, MCAndroidCameraControl, AvailableDevices)
	DEFINE_RW_CTRL_SET_PROPERTY(P_DEVICE, NativeControlCameraDevice, MCAndroidCameraControl, Device)
	DEFINE_RO_CTRL_SET_PROPERTY(P_FEATURES, NativeControlCameraFeature, MCAndroidCameraControl, Features)
	DEFINE_RW_CTRL_ENUM_PROPERTY(P_FLASH_MODE, NativeControlCameraFlashMode, MCAndroidCameraControl, FlashMode)
	DEFINE_RO_CTRL_PROPERTY(P_IS_FLASH_AVAILABLE, Bool, MCAndroidCameraControl, FlashIsAvailable)
	DEFINE_RO_CTRL_PROPERTY(P_IS_FLASH_ACTIVE, Bool, MCAndroidCameraControl, FlashIsActive)
};

MCObjectPropertyTable MCAndroidCameraControl::kPropertyTable =
{
	&MCAndroidControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCAndroidCameraControl::kActions[] =
{
	DEFINE_CTRL_EXEC_METHOD(CameraStartRecording, MCAndroidCameraControl, StartRecording)
	DEFINE_CTRL_EXEC_METHOD(CameraStopRecording, MCAndroidCameraControl, StopRecording)
	DEFINE_CTRL_EXEC_METHOD(CameraTakePicture, MCAndroidCameraControl, TakePicture)
};

MCNativeControlActionTable MCAndroidCameraControl::kActionTable =
{
	&MCAndroidControl::kActionTable,
	sizeof(kActions) / sizeof(kActions[0]),
	&kActions[0],
};

////////////////////////////////////////////////////////////////////////////////

MCAndroidCameraControl::MCAndroidCameraControl(void)
{
}

MCAndroidCameraControl::~MCAndroidCameraControl(void)
{
}

MCNativeControlType MCAndroidCameraControl::GetType(void)
{
	return kMCNativeControlTypeCamera;
}

void MCAndroidCameraControl::SetDevice(MCExecContext& ctxt, MCNativeControlCameraDevice p_device)
{
	jobject t_view;
	t_view = GetView();
	
	if (t_view != nil)
		MCAndroidObjectRemoteCall(t_view, "setDevice", "vi", nil, p_device);
}

void MCAndroidCameraControl::SetFlashMode(MCExecContext& ctxt, MCNativeControlCameraFlashMode p_mode)
{
	jobject t_view;
	t_view = GetView();
	
	if (t_view != nil)
		MCAndroidObjectRemoteCall(t_view, "setFlashMode", "vi", nil, p_mode);
}

void MCAndroidCameraControl::GetAvailableDevices(MCExecContext& ctxt, MCNativeControlCameraDevice& r_devices)
{
	jobject t_view;
	t_view = GetView();
	
	if (t_view != nil)
		MCAndroidObjectRemoteCall(t_view, "getDevices", "i", &r_devices);
}

void MCAndroidCameraControl::GetDevice(MCExecContext& ctxt, MCNativeControlCameraDevice& r_device)
{
	jobject t_view;
	t_view = GetView();
	
	if (t_view != nil)
		MCAndroidObjectRemoteCall(t_view, "getDevice", "i", &r_device);
}

void MCAndroidCameraControl::GetFeatures(MCExecContext& ctxt, MCNativeControlCameraFeature& r_features)
{
	jobject t_view;
	t_view = GetView();
	
	if (t_view != nil)
		MCAndroidObjectRemoteCall(t_view, "getFeatures", "i", &r_features);
}

void MCAndroidCameraControl::GetFlashMode(MCExecContext& ctxt, MCNativeControlCameraFlashMode& r_mode)
{
	jobject t_view;
	t_view = GetView();
	
	if (t_view != nil)
		MCAndroidObjectRemoteCall(t_view, "getFlashMode", "i", &r_mode);
}

void MCAndroidCameraControl::GetFlashIsAvailable(MCExecContext& ctxt, bool& r_available)
{
	jobject t_view;
	t_view = GetView();
	
	if (t_view != nil)
		MCAndroidObjectRemoteCall(t_view, "getFlashIsAvailable", "i", &r_available);
}

void MCAndroidCameraControl::GetFlashIsActive(MCExecContext& ctxt, bool& r_active)
{
	jobject t_view;
	t_view = GetView();
	
	if (t_view != nil)
		MCAndroidObjectRemoteCall(t_view, "getFlashIsActive", "i", &r_active);
}

void MCAndroidCameraControl::ExecStartRecording(MCExecContext& ctxt)
{
	jobject t_view;
	t_view = GetView();
	
	if (t_view == nil)
		return;
	
	MCAndroidObjectRemoteCall(t_view, "startRecording", "v", nil);
}

void MCAndroidCameraControl::ExecStopRecording(MCExecContext& ctxt)
{
	jobject t_view;
	t_view = GetView();
	
	if (t_view == nil)
		return;
	
	MCAndroidObjectRemoteCall(t_view, "stopRecording", "v", nil);
}

static bool s_take_picture_finished;
static MCDataRef s_picture_data;

void MCAndroidCameraControl::ExecTakePicture(MCExecContext& ctxt)
{
	jobject t_view;
	t_view = GetView();
	
	if (t_view == nil)
		return;
	
	s_take_picture_finished = false;
	s_picture_data = nil;
	
	bool t_success;
	t_success = true;
	
	MCAndroidObjectRemoteCall(t_view, "takePicture", "b", &t_success);
	
	if (t_success)
	{
		while (!s_take_picture_finished)
			MCscreen->wait(60.0, True, True);
	}
	
	if (s_picture_data == nil)
		ctxt.SetTheResultToEmpty();
	else
		ctxt.SetTheResultToValue(s_picture_data);
}

////////////////////////////////////////////////////////////////////////////////

jobject MCAndroidCameraControl::CreateView(void)
{
	MCLog("MCAndroidCameraControl::CreateView", nil);
	jobject t_view;
	MCAndroidEngineRemoteCall("createCameraControl", "o", &t_view);
	return t_view;
}

void MCAndroidCameraControl::DeleteView(jobject p_view)
{
	JNIEnv *env;
	env = MCJavaGetThreadEnv();
	
	env->DeleteGlobalRef(p_view);
}

////////////////////////////////////////////////////////////////////////////////

bool MCNativeCameraControlCreate(MCNativeControl *&r_control)
{
	MCLog("MCNativeCameraControlCreate", nil);
	MCAndroidCameraControl *t_control = new MCAndroidCameraControl();
	
	r_control = t_control;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_CameraControl_onPictureTaken(JNIEnv *env, jobject object, jbyteArray p_data) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_CameraControl_onPictureTaken(JNIEnv *env, jobject object, jbyteArray p_data)
{
	s_picture_data = nil;
	/* UNCHECKED */ MCJavaByteArrayToDataRef(env, p_data, s_picture_data);
	
	s_take_picture_finished = true;
	MCAndroidBreakWait();
}

////////////////////////////////////////////////////////////////////////////////
