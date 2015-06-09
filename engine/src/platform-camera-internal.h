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

#ifndef __PLATFORM_CAMERA_INTERNAL_H__
#define __PLATFORM_CAMERA_INTERNAL_H__

////////////////////////////////////////////////////////////////////////////////

class MCPlatformCamera
{
public:
    MCPlatformCamera(void);
    virtual ~MCPlatformCamera(void);
    
    void Retain(void);
    void Release(void);

    virtual void Open(void) = 0;
    virtual void Close(void) = 0;
    
    virtual void Attach(void *owner) = 0;
    virtual void Detach(void) = 0;
	
	virtual bool GetNativeView(void *&r_view) = 0;
    
    virtual bool SetProperty(MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value) = 0;
    virtual bool GetProperty(MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value) = 0;
    
    virtual bool StartRecording(MCStringRef filename) = 0;
    virtual bool StopRecording(void) = 0;
    
    virtual bool TakePicture(MCDataRef& r_data) = 0;
    
private:
    uindex_t m_references;
};

////////////////////////////////////////////////////////////////////////////////

#endif