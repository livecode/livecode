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
#include "exec.h"

#include "platform.h"

#import <UIKit/UIKit.h>

#include "mbldc.h"
#include "mbliphone.h"
#include "mbliphonecontrol.h"

////////////////////////////////////////////////////////////////////////////////

class MCiOSCameraControl: public MCiOSControl
{
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
	static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;
	
public:
	MCiOSCameraControl(void);
	
	virtual MCNativeControlType GetType(void);
	
	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
	virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
	
	// Camera properties
	void GetDevices(MCExecContext& ctxt, MCNativeControlCameraDevice &r_devices);
	void GetFeatures(MCExecContext& ctxt, MCNativeControlCameraFeature &r_features);
	void GetFlashModes(MCExecContext& ctxt, MCNativeControlCameraFlashMode &r_modes);
	
	void SetDevice(MCExecContext& ctxt, MCNativeControlCameraDevice p_device);
	void GetDevice(MCExecContext& ctxt, MCNativeControlCameraDevice &r_device);
	
	void SetFlashMode(MCExecContext& ctxt, MCNativeControlCameraFlashMode p_mode);
	void GetFlashMode(MCExecContext& ctxt, MCNativeControlCameraFlashMode &r_mode);
	
	void GetIsFlashAvailable(MCExecContext& ctxt, bool &r_available);
	void GetIsFlashActive(MCExecContext& ctxt, bool &r_active);
	
	
	// Camera-specific actions
	void ExecStartRecording(MCExecContext& ctxt, MCStringRef p_filename);
	void ExecStopRecording(MCExecContext& ctxt);
	void ExecTakePicture(MCExecContext& ctxt);
	
protected:
	virtual ~MCiOSCameraControl(void);
	virtual UIView *CreateView(void);
	virtual void DeleteView(UIView *view);
	
private:
	bool EnsureCamera();
	void ReleaseCamera();
	
	MCPlatformCameraRef m_camera;
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCiOSCameraControl::kProperties[] =
{
	DEFINE_RO_CTRL_SET_PROPERTY(P_DEVICES, NativeControlCameraDevice, MCiOSCameraControl, Devices)
	DEFINE_RW_CTRL_SET_PROPERTY(P_DEVICE, NativeControlCameraDevice, MCiOSCameraControl, Device)
	DEFINE_RO_CTRL_SET_PROPERTY(P_FEATURES, NativeControlCameraFeature, MCiOSCameraControl, Features)
	DEFINE_RO_CTRL_SET_PROPERTY(P_FLASH_MODES, NativeControlCameraFlashMode, MCiOSCameraControl, FlashModes)
	DEFINE_RW_CTRL_SET_PROPERTY(P_FLASH_MODE, NativeControlCameraFlashMode, MCiOSCameraControl, FlashMode)
	DEFINE_RO_CTRL_PROPERTY(P_IS_FLASH_AVAILABLE, Bool, MCiOSCameraControl, IsFlashAvailable)
	DEFINE_RO_CTRL_PROPERTY(P_IS_FLASH_ACTIVE, Bool, MCiOSCameraControl, IsFlashActive)
};

MCObjectPropertyTable MCiOSCameraControl::kPropertyTable =
{
	&MCiOSControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCiOSCameraControl::kActions[] =
{
	DEFINE_CTRL_WAITABLE_EXEC_UNARY_METHOD(CameraStartRecording, MCiOSCameraControl, String, StartRecording)
	DEFINE_CTRL_EXEC_METHOD(CameraStopRecording, MCiOSCameraControl, StopRecording)
	DEFINE_CTRL_WAITABLE_EXEC_METHOD(CameraTakePicture, MCiOSCameraControl, TakePicture)
};

MCNativeControlActionTable MCiOSCameraControl::kActionTable =
{
	&MCiOSControl::kActionTable,
	sizeof(kActions) / sizeof(kActions[0]),
	&kActions[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlType MCiOSCameraControl::GetType()
{
	return kMCNativeControlTypeCamera;
}

////////////////////////////////////////////////////////////////////////////////

MCiOSCameraControl::MCiOSCameraControl()
{
	m_camera = nil;
}

MCiOSCameraControl::~MCiOSCameraControl()
{
	if (m_camera != nil)
		MCPlatformCameraRelease(m_camera);
}

////////////////////////////////////////////////////////////////////////////////

UIView *MCiOSCameraControl::CreateView()
{
	if (!EnsureCamera())
		return nil;
	
	MCPlatformCameraOpen(m_camera);
	
	void *t_view;
	t_view = nil;
	
	if (!MCPlatformCameraGetNativeView(m_camera, t_view))
		return nil;
	
	return (UIView*)t_view;
}

void MCiOSCameraControl::DeleteView(UIView *view)
{
	if (m_camera != nil)
	{
		MCPlatformCameraClose(m_camera);
		MCPlatformCameraRelease(m_camera);
		m_camera = nil;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCiOSCameraControl::EnsureCamera()
{
	if (m_camera == nil)
		MCPlatformCameraCreate(m_camera);
	
	return m_camera != nil;
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSCameraControl::GetDevices(MCExecContext &ctxt, MCNativeControlCameraDevice &r_devices)
{
	if (!EnsureCamera())
		return;
	
	/* UNCHECKED */
	MCPlatformCameraGetProperty(m_camera, kMCPlatformCameraPropertyDevices, kMCPlatformPropertyTypeCameraDevice, &r_devices);
}

void MCiOSCameraControl::GetFeatures(MCExecContext &ctxt, MCNativeControlCameraFeature &r_features)
{
	if (!EnsureCamera())
		return;
	
	/* UNCHECKED */
	MCPlatformCameraGetProperty(m_camera, kMCPlatformCameraPropertyFeatures, kMCPlatformPropertyTypeCameraFeature, &r_features);
}

void MCiOSCameraControl::GetFlashModes(MCExecContext &ctxt, MCNativeControlCameraFlashMode &r_modes)
{
	if (!EnsureCamera())
		return;
	
	/* UNCHECKED */
	MCPlatformCameraGetProperty(m_camera, kMCPlatformCameraPropertyFlashModes, kMCPlatformPropertyTypeCameraFlashMode, &r_modes);
}

void MCiOSCameraControl::SetDevice(MCExecContext &ctxt, MCNativeControlCameraDevice p_device)
{
	if (!EnsureCamera())
		return;
	
	/* UNCHECKED */
	MCPlatformCameraSetProperty(m_camera, kMCPlatformCameraPropertyDevice, kMCPlatformPropertyTypeCameraDevice, &p_device);
}

void MCiOSCameraControl::GetDevice(MCExecContext &ctxt, MCNativeControlCameraDevice &r_device)
{
	if (!EnsureCamera())
		return;
	
	/* UNCHECKED */
	MCPlatformCameraGetProperty(m_camera, kMCPlatformCameraPropertyDevice, kMCPlatformPropertyTypeCameraDevice, &r_device);
}

void MCiOSCameraControl::SetFlashMode(MCExecContext &ctxt, MCNativeControlCameraFlashMode p_mode)
{
	if (!EnsureCamera())
		return;
	
	/* UNCHECKED */
	MCPlatformCameraSetProperty(m_camera, kMCPlatformCameraPropertyFlashMode, kMCPlatformPropertyTypeCameraFlashMode, &p_mode);
}

void MCiOSCameraControl::GetFlashMode(MCExecContext &ctxt, MCNativeControlCameraFlashMode &r_mode)
{
	if (!EnsureCamera())
		return;
	
	/* UNCHECKED */
	MCPlatformCameraGetProperty(m_camera, kMCPlatformCameraPropertyFlashMode, kMCPlatformPropertyTypeCameraFlashMode, &r_mode);
}

void MCiOSCameraControl::GetIsFlashAvailable(MCExecContext &ctxt, bool &r_available)
{
	if (!EnsureCamera())
		return;
	
	/* UNCHECKED */
	MCPlatformCameraGetProperty(m_camera, kMCPlatformCameraPropertyIsFlashAvailable, kMCPlatformPropertyTypeBool, &r_available);
}

void MCiOSCameraControl::GetIsFlashActive(MCExecContext &ctxt, bool &r_active)
{
	if (!EnsureCamera())
		return;
	
	/* UNCHECKED */
	MCPlatformCameraGetProperty(m_camera, kMCPlatformCameraPropertyIsFlashActive, kMCPlatformPropertyTypeBool, &r_active);
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSCameraControl::ExecStartRecording(MCExecContext &ctxt, MCStringRef p_filename)
{
	if (!EnsureCamera())
		return;

	/* UNCHECKED */
	MCPlatformCameraStartRecording(m_camera, p_filename);
}

void MCiOSCameraControl::ExecStopRecording(MCExecContext &ctxt)
{
	if (!EnsureCamera())
		return;
	
	/* UNCHECKED */
	MCPlatformCameraStopRecording(m_camera);
}

void MCiOSCameraControl::ExecTakePicture(MCExecContext &ctxt)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = EnsureCamera();
	
	MCAutoDataRef t_data;
	
	if (t_success)
		t_success = MCPlatformCameraTakePicture(m_camera, &t_data);
	
	if (t_success)
		ctxt.SetTheResultToValue(*t_data);
	else
		ctxt.SetTheResultToEmpty();
}

////////////////////////////////////////////////////////////////////////////////

bool MCNativeCameraControlCreate(MCNativeControl *&r_control)
{
	MCiOSCameraControl *t_control;
	t_control = new MCiOSCameraControl();
	
	if (t_control == nil)
		return false;
	
	r_control = t_control;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
