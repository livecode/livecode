/* Copyright (C) 2003-2015 Runtime Revolution Ltd.
 
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

//#include "w32prefix.h"

#include "platform.h"
#include "platform-internal.h"
#include "platform-camera-internal.h"

#include <dshow.h>

#include "atlsubset.h"
#include "qedit.h"

#include "graphics_util.h"
#include "imagebitmap.h"
#include "parsedef.h"
#include "objdefs.h"
#include "mcio.h"
#include "image.h"

////////////////////////////////////////////////////////////////////////////////
template <class T>
void inline MCMemoryClearStruct(T&p_struct)
{
	MCMemoryClear(&p_struct, sizeof(T));
}

static inline bool MCRectangleIsEqual(const MCRectangle &p_left, const MCRectangle &p_right)
{
	return p_left.x == p_right.x && p_left.y == p_right.y && p_left.width == p_right.width && p_left.height == p_right.height;
}

void DeleteMediaType(AM_MEDIA_TYPE *p_type)
{
    if (p_type == NULL) {
        return;
    }

    if (p_type->cbFormat != 0) {
        CoTaskMemFree((PVOID)p_type->pbFormat);
        p_type->cbFormat = 0;
        p_type->pbFormat = NULL;
    }
    if (p_type->pUnk != NULL) {
        p_type->pUnk->Release();
        p_type->pUnk = NULL;
    }

    CoTaskMemFree((PVOID)p_type);
}

////////////////////////////////////////////////////////////////////////////////

class MCDSCamera: public MCPlatformCamera
{
public:
    MCDSCamera(void);
    ~MCDSCamera(void);
    
    void Open(void);
    void Close(void);
    
    void Attach(void *owner);
    void Detach(void);
    
    bool SetProperty(MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value);
    bool GetProperty(MCPlatformCameraProperty property, MCPlatformPropertyType type, void *value);
	
	bool GetNativeView(void *&r_view);
    
	//////////
	
	bool SetRect(const MCRectangle &p_rect);
	bool GetRect(MCRectangle &r_rect);

	bool SetVisible(bool p_visible);
	bool GetVisible(bool &r_visible);

	bool GetDevices(intset_t &r_devices);
	bool GetFeatures(intset_t &r_features);
	bool GetFlashModes(intset_t &r_modes);
	
	bool SetDevice(intset_t p_device);
	bool GetDevice(intset_t &r_device);
	
	bool SetFlashMode(intset_t p_mode);
	bool GetFlashMode(intset_t &r_mode);
	
	bool GetIsFlashAvailable(bool &r_available);
	bool GetIsFlashActive(bool &r_active);
	
	//////////
	
    bool StartRecording(MCStringRef p_filename);
    bool StopRecording(void);
    
    bool TakePicture(MCDataRef& r_data);
    
private:
    void Realize(void);
    void Unrealize(void);
    void Display(void);

	bool OpenVideoInput();
	void CloseVideoInput();
	bool ConfigureVideoInput(IAMStreamConfig* p_config);

	bool OpenAudioInput();
	void CloseAudioInput();

	bool OpenSampleGrabber();
	void CloseSampleGrabber();

	bool OpenFilterGraph();
	void CloseFilterGraph();

	bool OpenCaptureGraph();
	void CloseCaptureGraph();

	void StartPreview();
	void StopPreview();

	//////////

	MCPlatformCameraDevice m_device;
	bool m_capture_video;
	MCRectangle m_rect;

	bool m_visible;
	bool m_previewing;
	uint32_t m_snapshot_width;
	uint32_t m_snapshot_height;

	HWND m_parent;

	//////////

	CComPtr<IMoniker> m_video_device;
	CComPtr<IMoniker> m_audio_device;

	CComPtr<IBaseFilter> m_video_input;
	CComPtr<IBaseFilter> m_audio_input;

	CComPtr<ISampleGrabber> m_sample_grabber;
	CComPtr<IBaseFilter> m_sample_grabber_output;

	CComPtr<IGraphBuilder> m_filter_graph;
	CComPtr<ICaptureGraphBuilder2> m_capture_graph;

	CComPtr<IMediaControl> m_media_control;
	CComPtr<IVideoWindow> m_video_window;

	HWND m_preview_window;
};

////////////////////////////////////////////////////////////////////////////////

MCDSCamera::MCDSCamera()
{
	m_device = kMCPlatformCameraDeviceDefault;
	m_capture_video = false;
	m_previewing = false;
	m_visible = false;

	m_rect = MCRectangleMake(0,0,0,0);

	m_parent = nil;
	m_preview_window = nil;
}

MCDSCamera::~MCDSCamera()
{
	Close();
}

////////////////////////////////////////////////////////////////////////////////

bool MCDSCameraFetchDefaultDeviceMoniker(const IID &p_device_category, IMoniker **r_moniker)
{
	bool t_success;
	t_success = true;

	CComPtr<ICreateDevEnum> t_create_enum;

	if (t_success)
		t_success = SUCCEEDED(t_create_enum.CoCreateInstance(CLSID_SystemDeviceEnum));

	CComPtr<IEnumMoniker> t_enum_moniker;

	if (t_success)
		t_success = SUCCEEDED(t_create_enum->CreateClassEnumerator(p_device_category, &t_enum_moniker, 0));

	CComPtr<IMoniker> t_moniker;

	if (t_success && t_enum_moniker != nil)
	{
		ULONG t_fetched;
		t_fetched = 0;

		// retrieve the first device moniker
		t_success = S_OK == t_enum_moniker->Next(1, &t_moniker, &t_fetched);
	}

	if (t_success)
		*r_moniker = t_moniker.Detach();

	return t_success;
}

bool MCDSCamera::OpenVideoInput()
{
	if (m_video_input != nil)
		return true;

	CComPtr<IMoniker> t_device;
	CComPtr<IBaseFilter> t_input;

	if (!MCDSCameraFetchDefaultDeviceMoniker(CLSID_VideoInputDeviceCategory, &t_device))
		return false;

	if (t_device == nil)
		return false;
	
	if (!SUCCEEDED(t_device->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&t_input)))
		return false;

	m_video_device = t_device;
	m_video_input = t_input;

	return true;
}

void MCDSCamera::CloseVideoInput()
{
	m_video_input.Release();
	m_video_device.Release();
}

bool MCDSCamera::OpenAudioInput()
{
	if (m_audio_input != nil)
		return true;

	CComPtr<IMoniker> t_device;
	CComPtr<IBaseFilter> t_input;

	if (!MCDSCameraFetchDefaultDeviceMoniker(CLSID_AudioInputDeviceCategory, &t_device))
		return false;
	
	if (!SUCCEEDED(t_device->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&t_input)))
		return false;

	m_audio_device = t_device;
	m_audio_input = t_input;

	return true;
}

void MCDSCamera::CloseAudioInput()
{
	m_audio_input.Release();
	m_audio_device.Release();
}

bool MCDSCamera::OpenSampleGrabber()
{
	if (m_sample_grabber != nil)
		return true;

	bool t_success;
	t_success = true;

	CComPtr<ISampleGrabber> t_grabber;
	if (t_success)
		t_success = SUCCEEDED(t_grabber.CoCreateInstance(CLSID_SampleGrabber));
	if (t_success)
		t_success = t_grabber != nil;

	if (t_success)
	{
		AM_MEDIA_TYPE t_video_type;
		MCMemoryClearStruct(t_video_type);
		t_video_type.majortype = MEDIATYPE_Video;
		t_video_type.subtype = MEDIASUBTYPE_RGB24;
		t_success = SUCCEEDED(t_grabber-> SetMediaType(&t_video_type));
	}

	if (t_success)
		t_success = SUCCEEDED(t_grabber->SetBufferSamples(TRUE));
	if (t_success)
		t_success = SUCCEEDED(t_grabber->SetOneShot(FALSE));

	CComPtr<IBaseFilter> t_output_filter;
	if (t_success)
		t_success = SUCCEEDED(t_grabber->QueryInterface(&t_output_filter));

	if (t_success)
	{
		m_sample_grabber = t_grabber;
		m_sample_grabber_output = t_output_filter;
	}

	return t_success;
}

void MCDSCamera::CloseSampleGrabber()
{
	m_sample_grabber_output.Release();
	m_sample_grabber.Release();
}

bool MCDSCamera::OpenFilterGraph()
{
	if (m_filter_graph != nil)
		return true;

	bool t_success;
	t_success = true;

	if (t_success)
		t_success = OpenVideoInput();
	if (t_success)
		t_success = !m_capture_video || OpenAudioInput();
	if (t_success)
		t_success = OpenSampleGrabber();

	CComPtr<IGraphBuilder> t_builder;
	if (t_success)
		t_success = SUCCEEDED(t_builder.CoCreateInstance(CLSID_FilterGraph));

	if (t_success)
		t_success = SUCCEEDED(t_builder->AddFilter(m_video_input, L"VideoInput"));

	if (t_success && m_capture_video)
		t_success = SUCCEEDED(t_builder->AddFilter(m_audio_input, L"AudioInput"));

	if (t_success)
		t_success = SUCCEEDED(t_builder->AddFilter(m_sample_grabber_output, L"Grabber"));

	if (t_success)
		m_filter_graph = t_builder;

	return t_success;
}

void MCDSCamera::CloseFilterGraph()
{
	m_filter_graph.Release();
}

bool MCDSCamera::ConfigureVideoInput(IAMStreamConfig *p_config)
{
	bool t_success;
	t_success = true;

	int t_count, t_size;
	if (t_success)
		t_success = SUCCEEDED(p_config->GetNumberOfCapabilities(&t_count, &t_size));

	if (t_success)
		t_success = t_size == sizeof(VIDEO_STREAM_CONFIG_CAPS);

	AM_MEDIA_TYPE *t_preferred_format;
	t_preferred_format = nil;

	VIDEOINFOHEADER *t_pref_vidinfo;
	t_pref_vidinfo = nil;

	for (uint32_t i = 0; t_success && i < t_count; i++)
	{
		VIDEO_STREAM_CONFIG_CAPS t_caps;
		AM_MEDIA_TYPE *t_type;
		t_success = SUCCEEDED(p_config->GetStreamCaps(i, &t_type, (BYTE*)&t_caps));
		if (t_success)
		{
			if (t_type->formattype == FORMAT_VideoInfo)
			{
				VIDEOINFOHEADER *t_vidinfo;
				t_vidinfo = (VIDEOINFOHEADER*)t_type->pbFormat;

				if (t_preferred_format == nil || (t_vidinfo->bmiHeader.biWidth > t_pref_vidinfo->bmiHeader.biWidth && t_vidinfo->bmiHeader.biHeight > t_pref_vidinfo->bmiHeader.biHeight))
				{
					DeleteMediaType(t_preferred_format);
					t_preferred_format = t_type;
					t_pref_vidinfo = (VIDEOINFOHEADER*)t_preferred_format->pbFormat;
					t_type = nil;
				}
			}
			DeleteMediaType(t_type);
		}
	}

	if (t_success && t_preferred_format != nil)
		t_success = SUCCEEDED(p_config->SetFormat(t_preferred_format));

	DeleteMediaType(t_preferred_format);

	return t_success;
}

bool MCDSCamera::OpenCaptureGraph()
{
	if (m_capture_graph != nil)
		return true;

	bool t_success;
	t_success = true;

	if (t_success)
		t_success = OpenFilterGraph();

	CComPtr<ICaptureGraphBuilder2> t_capture_graph;
	if (t_success)
		t_success = SUCCEEDED(t_capture_graph.CoCreateInstance(CLSID_CaptureGraphBuilder2));

	if (t_success)
		t_success = SUCCEEDED(t_capture_graph->SetFiltergraph(m_filter_graph));

	CComPtr<IPin> t_videoport_pin;
	CComPtr<IBaseFilter> t_null_renderer;
	if (t_success && SUCCEEDED(t_capture_graph->FindPin(m_video_input, PINDIR_OUTPUT, &PIN_CATEGORY_VIDEOPORT, nil, FALSE, 0, &t_videoport_pin)))
	{
		t_success = SUCCEEDED(t_null_renderer.CoCreateInstance(CLSID_NullRenderer));
		if (t_success)
			t_success = SUCCEEDED(m_filter_graph->AddFilter(t_null_renderer, L"NULL Renderer"));
	}

	CComPtr<IAMStreamConfig> t_stream_config;
	if (t_success)
		t_success =
			SUCCEEDED(t_capture_graph->FindInterface(NULL, &MEDIATYPE_Interleaved, m_video_input, IID_IAMStreamConfig, (void**)&t_stream_config)) ||
			SUCCEEDED(t_capture_graph->FindInterface(NULL, &MEDIATYPE_Video, m_video_input, IID_IAMStreamConfig, (void**)&t_stream_config));

	if (t_success)
		t_success = ConfigureVideoInput(t_stream_config);

	if (t_success)
		t_success = 
			SUCCEEDED(t_capture_graph->RenderStream(
				&PIN_CATEGORY_PREVIEW,
				&MEDIATYPE_Interleaved,
				m_video_input,
				m_sample_grabber_output,
				t_null_renderer)) ||
			SUCCEEDED(t_capture_graph->RenderStream(
				&PIN_CATEGORY_PREVIEW,
				&MEDIATYPE_Video,
				m_video_input,
				m_sample_grabber_output,
				t_null_renderer)) ||
			SUCCEEDED(t_capture_graph->RenderStream(
				&PIN_CATEGORY_CAPTURE,
				&MEDIATYPE_Video,
				m_video_input,
				m_sample_grabber_output,
				t_null_renderer));

	AM_MEDIA_TYPE t_media_type;
	if (t_success)
		t_success = SUCCEEDED(m_sample_grabber->GetConnectedMediaType(&t_media_type));

	if (t_success)
	{
		if (t_media_type.formattype == FORMAT_VideoInfo)
		{
			VIDEOINFOHEADER *t_video_info;
			t_video_info = (VIDEOINFOHEADER*)t_media_type.pbFormat;
			m_snapshot_width = t_video_info->bmiHeader.biWidth;
			m_snapshot_height = t_video_info->bmiHeader.biHeight;
		}

		m_capture_graph = t_capture_graph;
	}

	return t_success;
}

void MCDSCamera::CloseCaptureGraph()
{
	m_capture_graph.Release();
}

////////////////////////////////////////////////////////////////////////////////

void MCDSCamera::StartPreview()
{
	if (m_previewing)
		return;

	if (m_preview_window == nil)
		return;

	if (!m_visible)
		return;

	bool t_success;
	t_success = true;

	if (t_success)
		t_success = OpenCaptureGraph();

	CComPtr<IVideoWindow> t_video_window;
	if (t_success)
		t_success = SUCCEEDED(m_filter_graph->QueryInterface(&t_video_window));

	if (t_success)
	{
		RECT t_rect;
		GetWindowRect(m_preview_window, &t_rect);
		t_video_window->put_Owner((OAHWND)m_preview_window);
		t_video_window->put_Left(0);
		t_video_window->put_Top(0);
		t_video_window->put_Width(t_rect.right - t_rect.left);
		t_video_window->put_Height(t_rect.bottom - t_rect.top);
		t_video_window->put_Visible(TRUE);
		t_video_window->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
	}

	CComPtr<IMediaControl> t_media_control;
	if (t_success)
		t_success = SUCCEEDED(m_filter_graph->QueryInterface(&t_media_control));

	if (t_success)
		t_success = SUCCEEDED(t_media_control->Run());

	if (t_success)
	{
		m_media_control = t_media_control;
		m_video_window = t_video_window;
		m_previewing = true;

		ShowWindow(m_preview_window, SW_SHOW);
	}
}

void MCDSCamera::StopPreview()
{
	if (!m_previewing)
		return;

	m_media_control->Stop();
	m_media_control.Release();

	m_video_window->put_Visible(FALSE);
	m_video_window->put_Owner(NULL);
	m_video_window.Release();

	ShowWindow(m_preview_window, SW_HIDE);

	CloseCaptureGraph();
	CloseFilterGraph();

	m_previewing = false;
}

////////////////////////////////////////////////////////////////////////////////

extern HINSTANCE MChInst;

#define PREVIEW_WINDOW_CLASS L"MCDSCameraPreview"

LRESULT CALLBACK MCDSCameraPreviewProc(HWND p_hwnd, UINT p_msg, WPARAM p_wparam, LPARAM p_lparam)
{
	return DefWindowProc(p_hwnd, p_msg, p_wparam, p_lparam);
}

void MCDSCamera::Open()
{
	if (m_preview_window != nil)
		return;

	static ATOM s_class_atom = nil;

	if (s_class_atom == nil)
	{
		WNDCLASS t_class;
		t_class.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
		t_class.cbClsExtra = 0;
		t_class.cbWndExtra = 4;
		t_class.hInstance = MChInst;
		t_class.hIcon = nil;
		t_class.hCursor = nil;
		t_class.hbrBackground = nil;
		t_class.lpszMenuName = nil;
		t_class.lpfnWndProc = MCDSCameraPreviewProc;
		t_class.lpszClassName = PREVIEW_WINDOW_CLASS;

		s_class_atom = RegisterClass(&t_class);

		if (s_class_atom == nil)
			return;
	}

	bool t_success;
	t_success = true;

	HWND t_window;
	t_window = nil;
	
	DWORD t_flags;
	t_flags = SS_WHITERECT | (m_visible ? WS_VISIBLE : 0) | (m_parent != nil ? WS_CHILD : 0);
	t_window = CreateWindow(PREVIEW_WINDOW_CLASS, L"Preview", t_flags, m_rect.x, m_rect.y, m_rect.width, m_rect.height, m_parent, nil, nil, nil);
	t_success = t_window != nil;

	DWORD t_error;
	t_error = GetLastError();

	if (t_success)
		m_preview_window = t_window;

	if (t_success)
		StartPreview();
}

void MCDSCamera::Close()
{
	if (m_preview_window == nil)
		return;

	StopPreview();
	DestroyWindow(m_preview_window);
	m_preview_window = nil;
}

void MCDSCamera::Attach(void *owner)
{
	HWND t_parent;
	t_parent = nil;

	if (owner != nil)
	{
		Window t_window = (Window)owner;
		t_parent = (HWND)t_window->handle.window;
	}

	if (t_parent == m_parent)
		return;

	m_parent = (HWND)t_parent;
	if (m_preview_window != nil)
	{
		SetParent(m_preview_window, m_parent);
		StartPreview();
	}
}

void MCDSCamera::Detach()
{
	StopPreview();
	if (m_preview_window != nil)
		SetParent(m_preview_window, NULL);
}

////////////////////////////////////////////////////////////////////////////////

bool MCDSCamera::GetNativeView(void *&r_view)
{
	if (m_preview_window == nil)
		return false;

	r_view = m_preview_window;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCDSCamera::SetRect(const MCRectangle &p_rect)
{
	if (MCRectangleIsEqual(p_rect, m_rect))
		return true;

	if (m_previewing && ((p_rect.width != m_rect.width) || (p_rect.height != m_rect.height)))
	{
		StopPreview();
		MoveWindow(m_preview_window, p_rect.x, p_rect.y, p_rect.width, p_rect.height, TRUE);
		StartPreview();
	}
	else if (m_preview_window != nil)
		MoveWindow(m_preview_window, p_rect.x, p_rect.y, p_rect.width, p_rect.height, TRUE);

	m_rect = p_rect;
	return true;
}

bool MCDSCamera::GetRect(MCRectangle &r_rect)
{
	r_rect = m_rect;
	return true;
}

bool MCDSCamera::SetVisible(bool p_visible)
{
	if (p_visible == m_visible)
		return true;

	m_visible = p_visible;
	if (m_visible)
		StartPreview();
	else
		StopPreview();

	return true;
}

bool MCDSCamera::GetVisible(bool &r_visible)
{
	return m_visible;
}

bool MCDSCamera::GetDevices(intset_t &r_devices)
{
	CComPtr<IMoniker> t_moniker;

	if (!OpenVideoInput())
		return false;

	if (m_video_input == nil)
		r_devices = 0;
	else
		r_devices = kMCPlatformCameraDeviceDefault;

	return true;
}

bool MCDSCamera::GetFeatures(intset_t &r_features)
{
	r_features = 0;
	return true;
}

bool MCDSCamera::GetFlashModes(intset_t &r_modes)
{
	r_modes = 0;
	return true;
}

bool MCDSCamera::SetDevice(intset_t p_device)
{
	if (p_device == m_device)
		return true;

	intset_t t_devices;
	if (!GetDevices(t_devices))
		return false;

	if ((t_devices & p_device) == 0)
		return false;

	m_device = (MCPlatformCameraDevice)p_device;

	// TODO - set up device

	return true;
}

bool MCDSCamera::GetDevice(intset_t &r_device)
{
	r_device = m_device;
	return true;
}

bool MCDSCamera::SetFlashMode(intset_t p_mode)
{
	return false;
}

bool MCDSCamera::GetFlashMode(intset_t &r_mode)
{
	return false;
}

bool MCDSCamera::GetIsFlashAvailable(bool &r_available)
{
	return false;
}

bool MCDSCamera::GetIsFlashActive(bool &r_active)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCDSCamera::SetProperty(MCPlatformCameraProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
    switch(p_property)
    {
		case kMCPlatformCameraPropertyRectangle:
			return SetRect(*(MCRectangle*)p_value);
        
        case kMCPlatformCameraPropertyVisible:
			return SetVisible(*(bool*)p_value);
        
		//case kMCPlatformCameraPropertyActive:
		//	return SetActive(*(bool*)p_value);

        case kMCPlatformCameraPropertyDevice:
			return SetDevice(*(intset_t*)p_value);

        case kMCPlatformCameraPropertyFlashMode:
			return SetFlashMode(*(intset_t*)p_value);

    }
    return true;
}

bool MCDSCamera::GetProperty(MCPlatformCameraProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
    switch(p_property)
    {
        case kMCPlatformCameraPropertyRectangle:
			return GetRect(*(MCRectangle*)r_value);
            
        case kMCPlatformCameraPropertyVisible:
			return GetVisible(*(bool*)r_value);
            
		//case kMCPlatformCameraPropertyActive:
		//	return GetActive(*(bool*)r_value);
            
        case kMCPlatformCameraPropertyDevices:
			return GetDevices(*(intset_t*)r_value);
		
        case kMCPlatformCameraPropertyDevice:
			return GetDevice(*(intset_t*)r_value);
		
        case kMCPlatformCameraPropertyFeatures:
			return GetFeatures(*(intset_t*)r_value);

		case kMCPlatformCameraPropertyFlashModes:
			return GetFlashModes(*(intset_t*)r_value);
		
        case kMCPlatformCameraPropertyFlashMode:
			return GetFlashMode(*(intset_t*)r_value);

        case kMCPlatformCameraPropertyIsFlashActive:
			return GetIsFlashActive(*(bool*)r_value);

		case kMCPlatformCameraPropertyIsFlashAvailable:
			return GetIsFlashAvailable(*(bool*)r_value);
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCDSCamera::StartRecording(MCStringRef p_filename)
{
	return false;
}

bool MCDSCamera::StopRecording()
{
	return false;
}

bool MCDSCamera::TakePicture(MCDataRef &r_data)
{
	if (!m_previewing)
		return false;

	bool t_success;
	t_success = true;

	long t_buffer_size;
	t_buffer_size = 0;

	long *t_buffer;
	t_buffer = nil;

	if (t_success)
		t_success = SUCCEEDED(m_sample_grabber->GetCurrentBuffer(&t_buffer_size, NULL));

	if (t_success)
		t_success = MCMemoryAllocate(t_buffer_size, t_buffer);

	if (t_success)
		t_success = SUCCEEDED(m_sample_grabber->GetCurrentBuffer(&t_buffer_size, t_buffer));

	//if (t_success)
	MCImageBitmap *t_bitmap;
	t_bitmap = nil;

	if (t_success)
		t_success = MCImageBitmapCreate(m_snapshot_width, m_snapshot_height, t_bitmap);

	if (t_success)
	{
		// point to last row of dst bitmap (convert bottom-up to top-down)
		uint8_t *t_dst_row;
		t_dst_row = (uint8_t*)t_bitmap->data + (t_bitmap->height - 1) * t_bitmap->stride;

		uint8_t *t_src_ptr;
		t_src_ptr = (uint8_t*)t_buffer;

		for (uint32_t y = 0; y < t_bitmap->height; y++)
		{
			uint32_t *t_dst_pixel;
			t_dst_pixel = (uint32_t*)t_dst_row;
			for (uint32_t x = 0; x < t_bitmap->width; x++)
			{
				uint8_t r, g, b;
				b = *t_src_ptr++;
				g = *t_src_ptr++;
				r = *t_src_ptr++;

				*t_dst_pixel++ = MCGPixelPackNative(r, g, b, 0xFF);
			}
			t_dst_row -= t_bitmap->stride;
		}
	}

	if (t_buffer != nil)
		MCMemoryDeallocate(t_buffer);

	IO_handle t_stream;
	t_stream = nil;
	
	if (t_success)
		t_success = nil != (t_stream = MCS_fakeopenwrite());

	uindex_t t_byte_count;
	t_byte_count = 0;

	if (t_success)
		t_success = MCImageEncodeJPEG(t_bitmap, nil, t_stream, t_byte_count);

	if (t_bitmap != nil)
		MCImageFreeBitmap(t_bitmap);

	size_t t_data_size;
	t_data_size = 0;

	void *t_data;
	t_data = nil;

	if (t_success)
		t_success = IO_NORMAL == MCS_closetakingbuffer(t_stream, t_data, t_data_size);
	else if (t_stream != nil)
		MCS_close(t_stream);

	if (t_success)
		t_success = MCDataCreateWithBytesAndRelease((byte_t*)t_data, t_data_size, r_data);

	if (!t_success && t_data != nil)
		MCMemoryDeallocate(t_data);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCameraCreate(MCPlatformCameraRef& r_camera)
{
	MCPlatformCamera *t_camera;
	t_camera = new MCDSCamera();

	//if (t_camera == nil)
	//	return false;

	r_camera = (MCPlatformCameraRef)t_camera;
	//return true;
}

////////////////////////////////////////////////////////////////////////////////
