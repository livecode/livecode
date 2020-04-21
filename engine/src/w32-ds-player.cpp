/* Copyright (C) 2016 LiveCode Ltd.
Portions (C) 2016 Docas AG.

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

#include <Windows.h>
#include <atlbase.h>
#include <DShow.h>
#include <d3d9.h>
#include <vmr9.h>
#include <dvdmedia.h>

#include "globals.h"
#include "stack.h"
#include "osspec.h"

#include "graphics_util.h"
#include "platform.h"
#include "platform-internal.h"

////////////////////////////////////////////////////////////////////////////////

extern HINSTANCE MChInst;

void MCWin32BSTRFree(BSTR p_str) { SysFreeString(p_str); }

#define kMCDSEventWindowClass "DSEVENTWINDOWCLASS"
#define kMCDSVideoWindowClass "DSVIDEOWINDOWCLASS"

#define kMCDSTimerID (1)
#define kMCDSTimerInterval (100) // 100 ms, 10x per second

////////////////////////////////////////////////////////////////////////////////

enum MCWin32DSPlayerState
{
	kMCWin32DSPlayerStopped,
	kMCWin32DSPlayerPaused,
	kMCWin32DSPlayerRunning,
};

//forward decleration for this callback
class MCWin32DSPlayer;
typedef void (*MCWin32DSPlayerGraphCallback)(MCWin32DSPlayer *p_player, long evCode, LONG_PTR Param1, LONG_PTR Param2, void *instanceData);

class MCWin32DSPlayer : public MCPlatformPlayer
{
public:
	MCWin32DSPlayer(void);
	virtual ~MCWin32DSPlayer(void);
	
	virtual bool GetNativeView(void *&r_view);
	virtual bool SetNativeParentView(void *p_view);
	
	virtual bool IsPlaying(void);
	// PM-2014-05-28: [[ Bug 12523 ]] Take into account the playRate property
	virtual void Start(double rate);
	virtual void Stop(void);
	virtual void Step(int amount);
	
	virtual bool LockBitmap(const MCGIntegerSize &p_size, MCImageBitmap*& r_bitmap);
	virtual void UnlockBitmap(MCImageBitmap *bitmap);
	
	virtual void SetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);
	virtual void GetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);
	
	virtual void CountTracks(uindex_t& r_count);
	virtual bool FindTrackWithId(uint32_t id, uindex_t& r_index);
	virtual void SetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
	virtual void GetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
	
	bool Initialize();
	bool HandleGraphEvent();
	bool HandleTimer();
	bool SetVideoWindow(HWND hwnd);
	bool SetVideoWindowSize(uint32_t p_width, uint32_t p_height);

	bool StartTimer();
	void StopTimer();

	bool HasVideo();
	bool HasAudio();

protected:
	virtual void Realize(void);
	virtual void Unrealize(void);

private:

	// Properties
	bool SetUrl(MCStringRef p_url);
	bool GetPlayRate(double &r_play_rate);
	bool SetPlayRate(double p_play_rate);
	bool GetCurrentPosition(MCPlatformPlayerDuration &r_position);
	bool SetCurrentPosition(MCPlatformPlayerDuration p_position);
	bool GetStartPosition(MCPlatformPlayerDuration &r_position);
	bool SetStartPosition(MCPlatformPlayerDuration p_position);
	bool GetFinishPosition(MCPlatformPlayerDuration &r_position);
	bool SetFinishPosition(MCPlatformPlayerDuration p_position);
	bool GetLoadedPosition(MCPlatformPlayerDuration &r_position);
	bool GetPlaySelection(bool &r_play_selection);
	bool SetPlaySelection(bool p_play_selection);
	bool GetLoop(bool &r_loop);
	bool SetLoop(bool p_loop);
	bool GetDuration(MCPlatformPlayerDuration &r_duration);
	bool GetTimeScale(MCPlatformPlayerDuration &r_time_scale);
	bool GetVolume(uint16_t &r_volume);
	bool SetVolume(uint16_t p_volume);
	bool GetOffscreen(bool &r_offscreen);
	bool SetOffscreen(bool p_offscreen);
	bool GetMirrored(bool &r_mirrored);
	bool SetMirrored(bool p_mirrored);
	bool GetMediaTypes(MCPlatformPlayerMediaTypes &r_types);
	bool SetCallbackMarkers(const MCPlatformPlayerDurationArray &p_markers);

	bool Play();
	bool Pause(bool p_stop = false);
	bool SeekRelative(int32_t p_amount);
	bool SetPositions(MCPlatformPlayerDuration p_start, MCPlatformPlayerDuration p_finish);

	bool SetEventWindow(HWND hwnd);
	bool SetFilterGraph(IGraphBuilder *p_graph);
	bool SetVisible(bool p_visible);
	bool GetFormattedSize(uint32_t &r_width, uint32_t &r_height);

	bool OpenFile(MCStringRef p_filename);
	void CloseFile();

	MCWin32DSPlayerState m_state;
	bool m_is_valid;
	HWND m_video_window;
	HWND m_event_window;

	MCPlatformPlayerMediaTypes m_media_types;
	MCPlatformPlayerDuration m_frame_length;

	bool m_play_selection;
	MCPlatformPlayerDuration m_start_position;
	MCPlatformPlayerDuration m_finish_position;

	MCPlatformPlayerDurationArray m_callback_markers;
	index_t m_last_marker;

	bool m_looping;
	bool m_mirrored;

    CComPtr<IGraphBuilder>  m_graph;
    CComPtr<IMediaEventEx>  m_event;
    CComPtr<IMediaControl>  m_control;
	CComPtr<IMediaSeeking>  m_seeking;
};

////////////////////////////////////////////////////////////////////////////////

#define WM_GRAPHNOTIFY (WM_APP + 1)   // Window message for graph events
LRESULT CALLBACK DSHiddenWindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		CREATESTRUCT *t_create;
		t_create = (CREATESTRUCT*)lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)t_create->lpCreateParams);
		break;

	case WM_GRAPHNOTIFY:
		{
			MCWin32DSPlayer *whichplayer = (MCWin32DSPlayer *)lParam;
			whichplayer->HandleGraphEvent();
		}
		break;

	case WM_TIMER:
		if (wParam == kMCDSTimerID)
		{
			MCWin32DSPlayer *t_player;
			t_player = (MCWin32DSPlayer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (t_player != nil)
				t_player->HandleTimer();
		}
		break;
	}

	return DefWindowProc (hWnd, message, wParam, lParam);
	
}

bool RegisterEventWindowClass()
{
	WNDCLASS	wc={0};
	static bool windowclassregistered = false;
	if (!windowclassregistered) 
	{
		// Register the Monitor child window class
		wc.style         = CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_SAVEBITS;
		wc.lpfnWndProc   = DSHiddenWindowProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = MChInst;
		wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = kMCDSEventWindowClass;
		if (RegisterClass(&wc))
			windowclassregistered = true;
	}
	return windowclassregistered;
}

LRESULT CALLBACK DSVideoWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		CREATESTRUCT *t_create;
		t_create = (CREATESTRUCT*)lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)t_create->lpCreateParams);
		break;

	case WM_SIZE:
		{
			MCWin32DSPlayer *t_player;
			t_player = (MCWin32DSPlayer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (t_player != nil && t_player->HasVideo())
				t_player->SetVideoWindowSize(LOWORD(lParam), HIWORD(lParam));
			break;
		}
	}
	return DefWindowProc (hWnd, message, wParam, lParam);
	
}

bool RegisterVideoWindowClass()
{
	static bool s_registered = false;

	if (!s_registered)
	{
		WNDCLASSEX t_class;
		MCMemoryClear(t_class);

		t_class.cbSize = sizeof(WNDCLASSEX);
		t_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_SAVEBITS;
		t_class.lpfnWndProc = DSVideoWindowProc;
		t_class.hInstance = MChInst;
		t_class.lpszClassName = kMCDSVideoWindowClass;

		if (RegisterClassEx(&t_class))
			s_registered = true;
	}

	return s_registered;
}

bool CreateEventWindow(MCWin32DSPlayer *p_player, HWND &r_window)
{
	if (!RegisterEventWindowClass())
		return false;

	HWND t_window;
	t_window = CreateWindow(kMCDSEventWindowClass, "EventWindow", 0, 0, 0, 2, 3, HWND_MESSAGE, nil, MChInst, p_player);

	if (t_window == nil)
		return false;

	r_window = t_window;
	return true;
}

bool CreateVideoWindow(MCWin32DSPlayer *p_player, HWND p_parent_hwnd, HWND &r_window)
{
	if (!RegisterVideoWindowClass())
		return false;

	HWND t_window;
	t_window = CreateWindow(kMCDSVideoWindowClass, "VideoWindow", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1, 1, p_parent_hwnd, nil, MChInst, p_player);

	if (t_window == nil)
		return false;

	r_window = t_window;
	return true;
}


bool MCWin32DSPlayer::HandleGraphEvent()
{
    HRESULT hr;
    LONG evCode = 0;
	LONG_PTR param1 = 0, param2 = 0;


    // Verify that the IMediaEventEx interface is valid
    if (!m_event)
        return false;

    // Process all events
    while ((hr = m_event->GetEvent(&evCode, &param1, &param2, 0)) == S_OK)
	{
		switch (evCode)
		{
		case EC_COMPLETE: 
		case EC_ERRORABORT:
		case EC_USERABORT:
			m_state = kMCWin32DSPlayerStopped;
			if (m_looping)
				Play();
			else
			{
				Pause(true);
				MCPlatformCallbackSendPlayerFinished(this);
			}
			break;
		}


        m_event->FreeEventParams(evCode, param1, param2);
    }

    return true;
}

bool MCWin32DSPlayer::HandleTimer()
{
	if (IsPlaying())
	{
		MCPlatformPlayerDuration t_current;
		if (!GetCurrentPosition(t_current))
			return false;

		if (m_callback_markers.count > 0)
		{
			while (m_last_marker >= 0 && t_current < m_callback_markers.ptr[m_last_marker])
				m_last_marker--;

			index_t t_index = 0;
			while (t_index < m_callback_markers.count && m_callback_markers.ptr[t_index] <= t_current)
				t_index++;

			if (t_index - 1 > m_last_marker)
			{
				m_last_marker = t_index - 1;
				MCPlatformCallbackSendPlayerMarkerChanged(this, m_callback_markers.ptr[m_last_marker]);
			}
		}

		MCPlatformCallbackSendPlayerCurrentTimeChanged(this);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCWin32DSFreeMediaType(AM_MEDIA_TYPE &p_type)
{
	if (p_type.cbFormat != 0)
	{
		CoTaskMemFree(p_type.pbFormat);
		p_type.cbFormat = 0;
		p_type.pbFormat = nil;
	}
	if (p_type.pUnk != nil)
	{
		p_type.pUnk->Release();
		p_type.pUnk = nil;
	}
}

void MCWin32DSDeleteMediaType(AM_MEDIA_TYPE *p_type)
{
	if (p_type != nil)
	{
		MCWin32DSFreeMediaType(*p_type);
		CoTaskMemFree(p_type);
	}
}

////////////////////////////////////////////////////////////////////////////////

MCWin32DSPlayer::MCWin32DSPlayer()
{
	m_video_window = nil;
	m_state = kMCWin32DSPlayerStopped;
	m_media_types = 0;
	m_frame_length = 0;
	m_is_valid = false;

	m_play_selection = false;
	m_start_position = m_finish_position = 0;

	m_callback_markers.count = 0;
	m_callback_markers.ptr = nil;
	m_last_marker = -1;

	m_looping = false;
	m_mirrored = false;
}

MCWin32DSPlayer::~MCWin32DSPlayer()
{
	CloseFile();

	if (m_event_window != nil)
		DestroyWindow(m_event_window);

	if (m_video_window != nil)
		DestroyWindow(m_video_window);

	MCPlatformArrayClear(m_callback_markers);
}

bool MCWin32DSPlayer::Initialize()
{
	bool t_success;
	t_success = true;

	HWND t_event_window = nil;
	if (t_success)
		t_success = CreateEventWindow(this, t_event_window);

	if (t_success)
	{
		m_event_window = t_event_window;
	}
	else
	{
		if (t_event_window != nil)
			DestroyWindow(t_event_window);
	}

	return t_success;
}

bool MCWin32DSPlayer::HasVideo()
{
	return 0 != (m_media_types & kMCPlatformPlayerMediaTypeVideo);
}

bool MCWin32DSPlayer::HasAudio()
{
	return 0 != (m_media_types & kMCPlatformPlayerMediaTypeAudio);
}

bool MCWin32DSPinIsConnected(IPin *p_pin)
{
	CComPtr<IPin> t_connected;
	HRESULT t_result;
	t_result = p_pin->ConnectedTo(&t_connected);
	return t_result == S_OK;
}

bool MCWin32DSFilterIsConnected(IBaseFilter *p_filter, PIN_DIRECTION p_direction)
{
	CComPtr<IEnumPins> t_enum_pins;
	if (!SUCCEEDED(p_filter->EnumPins(&t_enum_pins)))
		return false;

	CComPtr<IPin> t_pin;
	while (S_OK == t_enum_pins->Next(1, &t_pin, NULL))
	{
		PIN_DIRECTION t_direction;
		if (SUCCEEDED(t_pin->QueryDirection(&t_direction)))
		{
			if (MCWin32DSPinIsConnected(t_pin) && t_direction == p_direction)
				return true;
		}
		t_pin.Release();
	}

	return false;
}

bool MCWin32DSFilterGetFirstPin(IBaseFilter *p_filter, PIN_DIRECTION p_direction, CComPtr<IPin> &r_pin)
{
	CComPtr<IEnumPins> t_enum_pins;
	if (!SUCCEEDED(p_filter->EnumPins(&t_enum_pins)))
		return false;

	CComPtr<IPin> t_pin;
	while (S_OK == t_enum_pins->Next(1, &t_pin, NULL))
	{
		PIN_DIRECTION t_direction;
		if (SUCCEEDED(t_pin->QueryDirection(&t_direction)))
		{
			if (MCWin32DSPinIsConnected(t_pin) && t_direction == p_direction)
			{
				r_pin = t_pin;
				return true;
			}
		}
		t_pin.Release();
	}

	return false;
}

bool MCWin32DSGetMediaTypeFrameDuration(const AM_MEDIA_TYPE *p_type, MCPlayerDuration &r_duration)
{
	if (p_type == nil)
		return false;

	if (p_type->pbFormat == nil)
		return false;

	if (p_type->formattype == FORMAT_VideoInfo)
	{
		if (p_type->cbFormat < sizeof(VIDEOINFOHEADER))
			return false;

		VIDEOINFOHEADER *t_videoinfoheader = (VIDEOINFOHEADER*)p_type->pbFormat;
		r_duration = t_videoinfoheader->AvgTimePerFrame;
	}
	else if (p_type->formattype == FORMAT_VideoInfo2)
	{
		if (p_type->cbFormat < sizeof(VIDEOINFOHEADER2))
			return false;

		VIDEOINFOHEADER2 *t_videoinfoheader2 = (VIDEOINFOHEADER2*)p_type->pbFormat;
		r_duration = t_videoinfoheader2->AvgTimePerFrame;
	}
	else
		return false;

	return true;
}

// Examines the input pins of each renderer filter to determine the supported media types, and video frame-rate if applicable
bool MCWin3DSGetFilterGraphMediaInfo(IFilterGraph *p_graph, MCPlatformPlayerMediaTypes &r_types, MCPlatformPlayerDuration &r_frame_length)
{
	bool t_success;
	t_success = true;

	MCPlatformPlayerMediaTypes t_types = 0;
	MCPlatformPlayerDuration t_frame_length = 0;

	CComPtr<IEnumFilters> t_enum_filters;
	if (t_success)
		t_success = SUCCEEDED(p_graph->EnumFilters(&t_enum_filters));

	IBaseFilter *t_base_filter = nil;
	while (t_success && S_OK == t_enum_filters->Next(1, &t_base_filter, nil))
	{
		CComPtr<IEnumPins> t_enum_pins;
		if (t_success)
			t_success = SUCCEEDED(t_base_filter->EnumPins(&t_enum_pins));

		bool t_is_renderer = true;
		MCPlatformPlayerMediaTypes t_filter_types = 0;

		IPin *t_pin = nil;
		while (t_success && S_OK == t_enum_pins->Next(1, &t_pin, nil))
		{
			PIN_DIRECTION t_direction;
			bool t_connected;
			
			t_connected = MCWin32DSPinIsConnected(t_pin);

			if (t_success)
				t_success = SUCCEEDED(t_pin->QueryDirection(&t_direction));

			if (t_success && t_connected)
			{
				if (t_direction == PINDIR_OUTPUT)
					t_is_renderer = false;
				else
				{
					AM_MEDIA_TYPE t_media_type;
					MCMemoryClear(t_media_type);
					t_success = SUCCEEDED(t_pin->ConnectionMediaType(&t_media_type));

					if (t_success)
					{
						if (t_media_type.majortype == MEDIATYPE_Audio)
							t_filter_types |= kMCPlatformPlayerMediaTypeAudio;
						else if (t_media_type.majortype == MEDIATYPE_Video)
						{
							// Take a note of the video frame rate
							/* UNCHECKED */ MCWin32DSGetMediaTypeFrameDuration(&t_media_type, t_frame_length);

							t_filter_types |= kMCPlatformPlayerMediaTypeVideo;
						}

						MCWin32DSFreeMediaType(t_media_type);
					}
				}
			}

			t_pin->Release();
		}

		// combine filter types with others if it is a renderer
		if (t_is_renderer)
			t_types |= t_filter_types;

		t_base_filter->Release();
	}

	if (t_success)
	{
		r_types = t_types;
		r_frame_length = t_frame_length;
	}

	return t_success;
}

bool MCWin32DSPlayer::SetFilterGraph(IGraphBuilder *p_graph)
{
	bool t_success;
	t_success = true;

	CComPtr<IMediaEventEx> t_event;
	CComPtr<IMediaControl> t_control;
	CComPtr<IMediaSeeking> t_seeking;

    if (p_graph)
    {
		if (t_success)
			t_success = S_OK == p_graph->QueryInterface(&t_event); // IMediaEventEx

		if (t_success)
			t_success = S_OK == p_graph->QueryInterface(&t_control); // IMediaControl

		if (t_success)
			t_success = S_OK == p_graph->QueryInterface(&t_seeking); //IMediaSeeking
    }

	MCPlatformPlayerMediaTypes t_types = 0;
	MCPlatformPlayerDuration t_frame_length = 0;
	if (t_success && p_graph != nil)
		t_success = MCWin3DSGetFilterGraphMediaInfo(p_graph, t_types, t_frame_length);

	if (t_success)
	{
        if (m_graph)
        {           
			// clean up graph and related objects
            Stop();

			if (HasVideo())
			{
				SetVisible(false);
				SetVideoWindow(nil);
			}
        }

		m_graph = p_graph;
        m_event = t_event;
        m_control = t_control;
		m_seeking = t_seeking;

		m_media_types = t_types;
		m_frame_length = t_frame_length;
    }
    
	return t_success;
}

//tell ds  where to position video window
bool MCWin32DSPlayer::SetVideoWindow(HWND p_hwnd)
{
    CComQIPtr<IVideoWindow> t_video(m_graph);

    if (t_video == nil)
		return false;

	if (S_OK != t_video->put_Owner((OAHWND)p_hwnd))
		return false;

	if (S_OK != t_video->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS))
		return false;
	
	if (p_hwnd != nil)
	{
	    RECT grc;
	    if (!GetClientRect(p_hwnd, &grc))
			return false;

		if (S_OK != t_video->SetWindowPosition(0, 0, grc.right - grc.left, grc.bottom - grc.top))
			return false;
	}

    return true;
}

bool MCWin32DSPlayer::SetVideoWindowSize(uint32_t p_width, uint32_t p_height)
{
    CComQIPtr<IVideoWindow> t_video(m_graph);

	if (t_video == nil)
		return false;

	return S_OK == t_video->SetWindowPosition(0, 0, p_width, p_height);
}

//tell ds which window will get wm_graphnotify events
bool MCWin32DSPlayer::SetEventWindow(HWND hwnd)
{
	if (!m_graph)
		return false;
	CComQIPtr<IVideoWindow> pVideo(m_graph);
	if (pVideo && HasVideo())	
		 pVideo->put_MessageDrain((OAHWND)hwnd);
    if (m_event)
    {
		// pass pointer to the class for lparam, so the receiver can
		//find out which mediaviewer the event is from
        return m_event->SetNotifyWindow((OAHWND)hwnd, WM_GRAPHNOTIFY, (LONG_PTR)this) == S_OK;
    }
    else
        return false;
}

bool MCWin32DSPlayer::SetVisible(bool isVisible)
{
	if (!m_graph)
		return false;
	CComQIPtr<IVideoWindow> pVideo(m_graph);
	if (!pVideo)
		return false;
	return S_OK == pVideo->put_Visible(isVisible ? OATRUE : OAFALSE );
}

static inline bool _mediasubtype_supports_mirror(const GUID &p_subtype)
{
	return p_subtype == MEDIASUBTYPE_ARGB32;
}

// check the connection video subformat and attempt to reconnect if it'll break video mirroring.
static bool MCWin32DSReconnectVideoRendererConnection(IGraphBuilder *p_graph, IBaseFilter *p_vmr9)
{
	bool t_success = true;

	CComPtr<IPin> t_pin;
	if (t_success)
		t_success = MCWin32DSFilterGetFirstPin(p_vmr9, PINDIR_INPUT, t_pin);

	AM_MEDIA_TYPE t_current_mediatype;
	MCMemoryClear(t_current_mediatype);
	if (t_success)
		t_success = SUCCEEDED(t_pin->ConnectionMediaType(&t_current_mediatype));

	AM_MEDIA_TYPE t_new_mediatype;
	MCMemoryClear(t_new_mediatype);

	// If the current connection media type is not compatible then look for one that is
	if (t_success && !_mediasubtype_supports_mirror(t_current_mediatype.subtype))
	{
		CComPtr<IPin> t_other_pin;
		if (t_success)
			t_success = SUCCEEDED(t_pin->ConnectedTo(&t_other_pin));

		CComPtr<IEnumMediaTypes> t_mediatype_enum;
		if (t_success)
			t_success = SUCCEEDED(t_other_pin->EnumMediaTypes(&t_mediatype_enum));

		AM_MEDIA_TYPE *t_mt = nil;
		while (t_success && (t_new_mediatype.majortype == MEDIATYPE_NULL) && (S_OK == t_mediatype_enum->Next(1, &t_mt, NULL)))
		{
			if (_mediasubtype_supports_mirror(t_mt->subtype))
			{
				t_new_mediatype.majortype = t_mt->majortype;
				t_new_mediatype.subtype = t_mt->subtype;
				t_new_mediatype.bFixedSizeSamples = t_mt->bFixedSizeSamples;
				t_new_mediatype.bTemporalCompression = t_mt->bTemporalCompression;
			}

			MCWin32DSDeleteMediaType(t_mt);
		}

		// Fail if a compatible media type was not found
		if (t_success)
			t_success = t_new_mediatype.majortype != MEDIATYPE_NULL;

		CComPtr<IFilterGraph2> t_fg2;
		if (t_success)
			t_success = SUCCEEDED(p_graph->QueryInterface(&t_fg2));
		if (t_success)
			t_success = SUCCEEDED(t_fg2->ReconnectEx(t_pin, &t_new_mediatype));
	}

	MCWin32DSFreeMediaType(t_current_mediatype);
	MCWin32DSFreeMediaType(t_new_mediatype);

	return t_success;
}

//open file
bool MCWin32DSPlayer::OpenFile(MCStringRef p_filename)
{
	bool t_success;
	t_success = true;

	CComPtr<IGraphBuilder> t_graph;

	if (t_success)
		t_success = S_OK == t_graph.CoCreateInstance(CLSID_FilterGraph);

	MCAutoCustomPointer<OLECHAR, MCWin32BSTRFree> t_filename;
	if (t_success)
		t_success = MCStringConvertToBSTR(p_filename, &t_filename);

	CComPtr<IBaseFilter> t_source_filter;

	if (t_success)
	{
		// Attempt to load using WM ASF Reader first as this has better support for some formats (mp3) than the
		//    source filter selected by the graph builder.
		bool t_asf_success = true;

		CComPtr<IFileSourceFilter> t_asf_reader;
		t_asf_success = SUCCEEDED(t_asf_reader.CoCreateInstance(CLSID_WMAsfReader));

		if (t_asf_success)
			t_asf_success = SUCCEEDED(t_asf_reader->Load(*t_filename, NULL));

		HRESULT t_result;
		if (t_asf_success)
		{
			CComQIPtr<IBaseFilter> t_base_filter(t_asf_reader);
			t_asf_success = t_base_filter != nil;

			if (t_asf_success)
			{
				t_result = t_graph->AddFilter(t_base_filter, L"MCWin32DSASFReaderFilter");
				t_asf_success = SUCCEEDED(t_result);
			}
		}

		if (t_asf_success)
			t_source_filter = t_asf_reader;
	}

	// FALLBACK: Allow the graph builder to select the source filter.
	if (t_success && t_source_filter == nil)
		t_success = SUCCEEDED(t_graph->AddSourceFilter(*t_filename, L"MCWin32DSSourceFilter", &t_source_filter));

	// try to add VMR9 to filter graph - we remove it later if it's not used (no video)
	CComPtr<IBaseFilter> t_vmr9;
	// don't fail loading if VMR9 isn't available
	if (t_success && SUCCEEDED(t_vmr9.CoCreateInstance(CLSID_VideoMixingRenderer9)))
	{
		CComPtr<IVMRFilterConfig9> t_fc9;
		if (t_success)
			t_success = SUCCEEDED(t_vmr9->QueryInterface(&t_fc9));
		if (t_success)
			t_success = SUCCEEDED(t_fc9->SetNumberOfStreams(1));
		if (t_success)
			t_success = SUCCEEDED(t_graph->AddFilter(t_vmr9, L"MCWin32DSVMR9Filter"));
	}

	CComPtr<IEnumPins> t_enum_pins;
	if (t_success)
		t_success = SUCCEEDED(t_source_filter->EnumPins(&t_enum_pins));

	if (t_success)
	{
		// Ask the graph builder to render each output pin of the source filter.
		IPin *t_pin;
		while (t_success && S_OK == t_enum_pins->Next(1, &t_pin, NULL))
		{
			PIN_DIRECTION t_direction;
			t_success = SUCCEEDED(t_pin->QueryDirection(&t_direction));

			if (t_success && t_direction == PINDIR_OUTPUT)
			{
				t_success = SUCCEEDED(t_graph->Render(t_pin));
			}

			t_pin->Release();
		}
	}

	if (t_success && t_vmr9 != NULL)
	{
		if (MCWin32DSFilterIsConnected(t_vmr9, PINDIR_INPUT))
			/* UNCHECKED */ MCWin32DSReconnectVideoRendererConnection(t_graph, t_vmr9);
		else
			t_success = SUCCEEDED(t_graph->RemoveFilter(t_vmr9));
	}

	if (t_success)
		t_success = SetFilterGraph(t_graph);

	m_is_valid = t_success;

	return t_success;
}

void MCWin32DSPlayer::CloseFile()
{
	SetEventWindow(nil);
	SetVideoWindow(nil);
	SetFilterGraph(nil);
}

bool MCWin32DSPlayer::StartTimer()
{
	UINT t_elapse;
	return 0 != SetTimer(m_event_window, kMCDSTimerID, kMCDSTimerInterval, nil);
}

void MCWin32DSPlayer::StopTimer()
{
	KillTimer(m_event_window, kMCDSTimerID);
}

bool MCWin32DSPlayer::GetFormattedSize(uint32_t &r_width, uint32_t &r_height)
{
	if (!HasVideo() && HasAudio())
	{
		r_width = r_height = 0;
		return true;
	}

    CComQIPtr<IBasicVideo> pVideo(m_graph);
    if (pVideo == nil)
		return false;

	long lWidth, lHeight;
	if (S_OK != pVideo->get_VideoWidth(&lWidth) || S_OK != pVideo->get_VideoHeight(&lHeight))
		return false;

	r_width = lWidth;
	r_height = lHeight;

	return true;
}

//////////

bool MCWin32DSPlayer::SetUrl(MCStringRef p_url)
{
	bool t_success;
	t_success = true;

	CloseFile();

	if (t_success)
		t_success = OpenFile(p_url);

	if (t_success && HasVideo())
		t_success = SetVideoWindow(m_video_window);

	if (t_success)
		t_success = SetEventWindow(m_event_window);

	if (t_success && HasVideo())
		t_success = SetVisible(true);

	if (!t_success)
		CloseFile();

	Pause();

	//InvalidateRect(m_video_window, NULL, FALSE);
	//UpdateWindow(m_video_window);
	//m_MediaViewer->Pause();

	return t_success;
}

bool MCWin32DSPlayer::GetPlayRate(double &r_play_rate)
{
	if (m_seeking == nil)
		return false;

	double t_rate;
	if (S_OK != m_seeking->GetRate(&t_rate))
		return false;

	r_play_rate = t_rate;
	return true;
}

bool MCWin32DSPlayer::SetPlayRate(double p_play_rate)
{
	if (m_seeking == nil)
		return false;

	return S_OK == m_seeking->SetRate(p_play_rate);
}

bool MCWin32DSPlayer::GetTimeScale(MCPlatformPlayerDuration &r_time_scale)
{
	if (m_seeking == nil)
		return false;

	// convert current time format unit to media reference time units (100 nanoseconds)
	LONGLONG t_media_time;
	if (S_OK != m_seeking->ConvertTimeFormat(&t_media_time, &TIME_FORMAT_MEDIA_TIME, 1, NULL))
		return false;

	r_time_scale = (MCPlatformPlayerDuration)(10000000 / t_media_time); // convert to units/second
	return true;
}

bool MCWin32DSPlayer::GetDuration(MCPlatformPlayerDuration &r_duration)
{
	if (m_seeking == nil)
		return false;

	LONGLONG t_duration;
	if (S_OK != m_seeking->GetDuration(&t_duration))
		return false;

	if (t_duration > MCPlatformPlayerDurationMax)
		return false;

	r_duration = (MCPlatformPlayerDuration)t_duration;
	return true;
}

bool MCWin32DSPlayer::GetCurrentPosition(MCPlatformPlayerDuration &r_position)
{
	if (m_seeking == nil)
		return false;

	LONGLONG t_current;
	if (S_OK != m_seeking->GetCurrentPosition(&t_current))
		return false;

	if (t_current > MCPlatformPlayerDurationMax)
		return false;

	r_position = (MCPlatformPlayerDuration)t_current;

	return true;
}

bool MCWin32DSPlayer::SetCurrentPosition(MCPlatformPlayerDuration p_position)
{
	if (m_seeking == nil)
		return false;

	LONGLONG t_current = p_position;

	HRESULT t_result;
	t_result = m_seeking->SetPositions(&t_current, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

	if (t_result != S_OK && t_result != S_FALSE) // S_FALSE -> no change
		return false;

	// Enter paused state if not currently playing
	if (m_state != kMCWin32DSPlayerRunning)
		return Pause();

	return true;
}

bool MCWin32DSPlayer::SetPositions(MCPlatformPlayerDuration p_start, MCPlatformPlayerDuration p_finish)
{
	if (m_seeking == nil)
		return false;

	MCPlatformPlayerDuration t_current, t_duration;
	if (!GetCurrentPosition(t_current))
		return false;
	if (!GetDuration(t_duration))
		return false;

	MCPlatformPlayerDuration t_start, t_finish;
	t_start = 0;
	t_finish = t_duration;

	if (m_play_selection)
	{
		t_finish = MCMin(t_duration, m_finish_position);
		t_start = MCMin(t_finish, m_start_position);

		// don't allow span <= 0
		if (t_finish - t_start == 0)
		{
			t_start = 0;
			t_finish = t_duration;
		}
	}

	DWORD t_current_pos_flags;
	if (t_start > t_current)
		t_current_pos_flags = AM_SEEKING_AbsolutePositioning;
	else
		t_current_pos_flags = AM_SEEKING_NoPositioning;

	LONGLONG t_c, t_f;
	t_c = t_start;
	t_f = t_finish;

	HRESULT t_result = m_seeking->SetPositions(&t_c, t_current_pos_flags, &t_f, AM_SEEKING_AbsolutePositioning);

	if (t_result != S_OK && t_result != S_FALSE) // S_FALSE -> no change
		return false;

	// If not playing, pausing here will make sure the next frame is displayed
	if (m_state != kMCWin32DSPlayerRunning)
		return Pause(m_state == kMCWin32DSPlayerStopped);

	return true;
}

bool MCWin32DSPlayer::GetStartPosition(MCPlatformPlayerDuration &r_position)
{
	r_position = m_start_position;
	return true;
}

bool MCWin32DSPlayer::SetStartPosition(MCPlatformPlayerDuration p_position)
{
	m_start_position = p_position;
	return SetPositions(m_start_position, m_finish_position);
}

bool MCWin32DSPlayer::GetFinishPosition(MCPlatformPlayerDuration &r_position)
{
	r_position = m_finish_position;
	return true;
}

bool MCWin32DSPlayer::SetFinishPosition(MCPlatformPlayerDuration p_position)
{
	m_finish_position = p_position;
	return SetPositions(m_start_position, m_finish_position);
}

bool MCWin32DSPlayer::GetLoadedPosition(MCPlatformPlayerDuration &r_loaded)
{
	if (m_seeking == nil)
		return false;

	LONGLONG t_start, t_end;
	if (S_OK != m_seeking->GetAvailable(&t_start, &t_end))
		return false;

	if (t_end > MCPlatformPlayerDurationMax)
		return false;

	r_loaded = t_end;

	return true;
}

bool MCWin32DSPlayer::GetPlaySelection(bool &r_play_selection)
{
	r_play_selection = m_play_selection;
	return true;
}

bool MCWin32DSPlayer::SetPlaySelection(bool p_play_selection)
{
	m_play_selection = p_play_selection;
	// Update configured start / finish positions.
	return SetPositions(m_start_position, m_finish_position);
}

#define VOLUME_MAX     0L
#define VOLUME_MIN  -7000L

#define VOLUME_RANGE (VOLUME_MAX - VOLUME_MIN)


long percentToVolume(int percent)
{
	return MCClamp(VOLUME_MIN + (percent * VOLUME_RANGE) / 100, VOLUME_MIN, VOLUME_MAX);
}

long volumeToPercent(int volume)
{
	return MCClamp((100 * (volume - VOLUME_MIN)) / VOLUME_RANGE, 0, 100);
}

bool MCWin32DSPlayer::GetVolume(uint16_t &r_volume)
{
	CComQIPtr<IBasicAudio> t_audio(m_graph);
	if (t_audio == nil)
		return false;

	LONG t_volume;
	if (S_OK != t_audio->get_Volume(&t_volume))
		return false;

	r_volume = volumeToPercent(t_volume);
}

bool MCWin32DSPlayer::SetVolume(uint16_t p_volume)
{
	CComQIPtr<IBasicAudio> t_audio(m_graph);
	if (t_audio == nil)
		return false;

	return S_OK == t_audio->put_Volume(percentToVolume(p_volume));
}

bool MCWin32DSPlayer::GetLoop(bool &r_loop)
{
	r_loop = m_looping;
	return true;
}

bool MCWin32DSPlayer::SetLoop(bool p_loop)
{
	m_looping = p_loop;
	return true;
}

// TODO - implement offscreen property
bool MCWin32DSPlayer::GetOffscreen(bool &r_offscreen)
{
	r_offscreen = false;
	return true;
}

bool MCWin32DSPlayer::SetOffscreen(bool p_offscreen)
{
	return false;
}

// TODO - implement mirror property
bool MCWin32DSPlayer::GetMirrored(bool &r_mirrored)
{
	r_mirrored = m_mirrored;
	return true;
}

bool MCWin32DSPlayer::SetMirrored(bool p_mirrored)
{
	if (m_mirrored == p_mirrored)
		return true;

	if (m_graph == nil)
		return false;

	CComPtr<IBaseFilter> t_vmr9;
	if (!SUCCEEDED(m_graph->FindFilterByName(L"MCWin32DSVMR9Filter", &t_vmr9)))
		return false;

	CComPtr<IVMRMixerControl9> t_mc9;
	if (!SUCCEEDED(t_vmr9->QueryInterface(&t_mc9)))
		return false;

	VMR9NormalizedRect t_rect;
	if (p_mirrored)
	{
		t_rect.right = t_rect.top = 0.0;
		t_rect.left = t_rect.bottom = 1.0;
	}
	else
	{
		t_rect.left = t_rect.top = 0.0;
		t_rect.right = t_rect.bottom = 1.0;
	}

	if (!SUCCEEDED(t_mc9->SetOutputRect(0, &t_rect)))
		return false;

	if (m_state != kMCWin32DSPlayerRunning)
	{
		MCPlatformPlayerDuration t_duration;
		if (GetCurrentPosition(t_duration))
		{
			SetCurrentPosition(t_duration);
		}
	}

	m_mirrored = p_mirrored;
	return true;
}

bool MCWin32DSPlayer::GetMediaTypes(MCPlatformPlayerMediaTypes &r_types)
{
	r_types = m_media_types;
	return true;
}

bool MCWin32DSPlayer::SetCallbackMarkers(const MCPlatformPlayerDurationArray &p_markers)
{
	MCPlatformPlayerDurationArray t_markers = { nil, 0 };

	if (!MCPlatformArrayCopy(p_markers, t_markers))
		return false;

	MCPlatformArrayClear(m_callback_markers);
	m_callback_markers = t_markers;
	m_last_marker = -1;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

//play
bool MCWin32DSPlayer::Play()
{
	if (m_control == nil)
		return false;

	if (m_state == kMCWin32DSPlayerStopped)
	{
		// if play has stopped, then reset stream to the beginning
		MCPlatformPlayerDuration t_start_position = 0;
		if (m_play_selection && m_start_position < m_finish_position)
			t_start_position = m_start_position;

		if (!SetCurrentPosition(t_start_position))
			return false;
	}

	HRESULT t_result;
	t_result = m_control->Run();
	if (t_result != S_OK && t_result != S_FALSE)
		return false;

	StartTimer();

	m_state = kMCWin32DSPlayerRunning;

	return true;
}

//pause playback
bool MCWin32DSPlayer::Pause(bool p_stop)
{
	if (m_control == nil)
		return false;

	HRESULT t_result;
	t_result = m_control->Pause();

	if (t_result == S_FALSE)
	{
		OAFilterState t_state;
		t_result = m_control->GetState(10, &t_state);
		while (t_result == VFW_S_STATE_INTERMEDIATE)
		{
			MCscreen->wait(0.01, false, true);
			t_result = m_control->GetState(10, &t_state);
		}
	}

	if (t_result == E_FAIL)
		return false;

	StopTimer();

	if (p_stop)
		m_state = kMCWin32DSPlayerStopped;
	else
		m_state = kMCWin32DSPlayerPaused;

	return true;
}

// seek forward / backward n frames
bool MCWin32DSPlayer::SeekRelative(int32_t p_amount)
{
	if (m_seeking == nil)
		return false;

	LONGLONG t_current_position;
	if (!SUCCEEDED(m_seeking->GetCurrentPosition(&t_current_position)))
		return false;

	if (!SUCCEEDED(m_seeking->ConvertTimeFormat(&t_current_position, &TIME_FORMAT_MEDIA_TIME, t_current_position, NULL)))
		return false;

	t_current_position += (p_amount * m_frame_length);

	if (!SUCCEEDED(m_seeking->ConvertTimeFormat(&t_current_position, NULL, t_current_position, &TIME_FORMAT_MEDIA_TIME)))
		return false;

	return SUCCEEDED(m_seeking->SetPositions(&t_current_position, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning));
}

////////////////////////////////////////////////////////////////////////////////

bool MCWin32DSPlayer::GetNativeView(void *&r_view)
{
	if (m_video_window == nil)
		return false;

	r_view = m_video_window;
	return true;
}

bool MCWin32DSPlayer::SetNativeParentView(void *p_view)
{
	if (m_video_window != nil)
		return true;

	HWND t_video_window = nil;
	if (!CreateVideoWindow(this, (HWND)p_view, t_video_window))
		return false;

	if (m_graph && !SetVideoWindow(t_video_window))
	{
		DestroyWindow(t_video_window);
		return false;
	}

	m_video_window = t_video_window;

	return true;
}

void MCWin32DSPlayer::Realize()
{
	// STUB
}

void MCWin32DSPlayer::Unrealize()
{
	// STUB
}

void MCWin32DSPlayer::Start(double p_play_rate)
{
	if (!SetPlayRate(p_play_rate))
		return;

	Play();
}

void MCWin32DSPlayer::Stop()
{
	Pause();
}

void MCWin32DSPlayer::Step(int p_amount)
{
	SeekRelative(p_amount);
}

bool MCWin32DSPlayer::IsPlaying()
{
	return m_state == kMCWin32DSPlayerRunning;
}

void MCWin32DSPlayer::CountTracks(uindex_t &r_track_count)
{
	// TODO - implement track switching support
	r_track_count = 0;
}

bool MCWin32DSPlayer::FindTrackWithId(uint32_t p_id, uindex_t &r_index)
{
	// TODO - implement track switching support
	return false;
}

void MCWin32DSPlayer::GetTrackProperty(uindex_t p_index, MCPlatformPlayerTrackProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	// TODO - implement track switching support
}

void MCWin32DSPlayer::SetTrackProperty(uindex_t p_index, MCPlatformPlayerTrackProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
	// TODO - implement track switching support
}

void MCWin32DSPlayer::GetProperty(MCPlatformPlayerProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	switch (p_property)
	{
	case kMCPlatformPlayerPropertyDuration:
		{
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			MCPlatformPlayerDuration t_duration;
			if (GetDuration(t_duration))
				*((MCPlatformPlayerDuration*)r_value) = t_duration;
			break;
		}

	case kMCPlatformPlayerPropertyTimescale:
		{
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			MCPlatformPlayerDuration t_time_scale;
			if (GetTimeScale(t_time_scale))
				*((MCPlatformPlayerDuration*)r_value) = t_time_scale;
			break;
		}

	case kMCPlatformPlayerPropertyCurrentTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			MCPlatformPlayerDuration t_position;
			if (GetCurrentPosition(t_position))
				*((MCPlatformPlayerDuration*)r_value) = t_position;
			break;
		}

	case kMCPlatformPlayerPropertyStartTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			MCPlatformPlayerDuration t_position;
			if (GetStartPosition(t_position))
				*((MCPlatformPlayerDuration*)r_value) = t_position;
			break;
		}

	case kMCPlatformPlayerPropertyFinishTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			MCPlatformPlayerDuration t_position;
			if (GetFinishPosition(t_position))
				*((MCPlatformPlayerDuration*)r_value) = t_position;
			break;
		}

	case kMCPlatformPlayerPropertyLoadedTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			MCPlatformPlayerDuration t_position;
			if (GetLoadedPosition(t_position))
				*((MCPlatformPlayerDuration*)r_value) = t_position;
			break;
		}

	case kMCPlatformPlayerPropertyLoop:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeBool);
			bool t_loop;
			if (GetLoop(t_loop))
				*((bool*)r_value) = t_loop;
			break;
		}

	case kMCPlatformPlayerPropertyPlayRate:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeDouble);
			double t_rate;
			if (GetPlayRate(t_rate))
				*((double*)r_value) = t_rate;
			break;
		}

	case kMCPlatformPlayerPropertyMovieRect:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeRectangle);
			uint32_t t_width, t_height;
			if (GetFormattedSize(t_width, t_height))
				*((MCRectangle*)r_value) = MCRectangleMake(0, 0, t_width, t_height);
			break;
		}

	case kMCPlatformPlayerPropertyVolume:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeUInt16);
			uint16_t t_volume;
			if (GetVolume(t_volume))
				*((uint16_t*)r_value) = t_volume;
			break;
		}

	case kMCPlatformPlayerPropertyInvalidFilename:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeBool);
			*((bool*)r_value) = !m_is_valid;
			break;
		}

	case kMCPlatformPlayerPropertyOffscreen:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeBool);
			bool t_offscreen;
			if (GetOffscreen(t_offscreen))
				*((bool*)r_value) = t_offscreen;
			break;
		}

	case kMCPlatformPlayerPropertyMirrored:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeBool);
			bool t_mirrored;
			if (GetMirrored(t_mirrored))
				*((bool*)r_value) = t_mirrored;
			break;
		}

	case kMCPlatformPlayerPropertyOnlyPlaySelection:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeBool);
			bool t_play_selection;
			if (GetPlaySelection(t_play_selection))
				*((bool*)r_value) = t_play_selection;
			break;
		}

	case kMCPlatformPlayerPropertyMediaTypes:
		{
			MCAssert(p_type == kMCPlatformPropertyTypePlayerMediaTypes);
			MCPlatformPlayerMediaTypes t_types;
			if (GetMediaTypes(t_types))
				*((MCPlatformPlayerMediaTypes*)r_value) = t_types;
			break;
		}

	case kMCPlatformPlayerPropertyURL:
		MCLog("UNIMPLEMENTED");
		break;

	// Can safely ignore these
	case kMCPlatformPlayerPropertyShowSelection:
		break;
	}
}

void MCWin32DSPlayer::SetProperty(MCPlatformPlayerProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
	switch (p_property)
	{
	case kMCPlatformPlayerPropertyURL:
	case kMCPlatformPlayerPropertyFilename:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeMCString);
			MCStringRef t_url = *((MCStringRef*)p_value);
			SetUrl(t_url);

			break;
		}

	case kMCPlatformPlayerPropertyCurrentTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			MCPlatformPlayerDuration t_position = *((MCPlatformPlayerDuration*)p_value);
			SetCurrentPosition(t_position);

			break;
		}

	case kMCPlatformPlayerPropertyStartTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			MCPlatformPlayerDuration t_start = *((MCPlatformPlayerDuration*)p_value);
			SetStartPosition(t_start);

			break;
		}

	case kMCPlatformPlayerPropertyFinishTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			MCPlatformPlayerDuration t_finish = *((MCPlatformPlayerDuration*)p_value);
			SetFinishPosition(t_finish);

			break;
		}

	case kMCPlatformPlayerPropertyLoop:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeBool);
			bool t_loop = *((bool*)p_value);
			SetLoop(t_loop);

			break;
		}

	case kMCPlatformPlayerPropertyPlayRate:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeDouble);
			double t_rate = *((double*)p_value);
			SetPlayRate(t_rate);

			break;
		}

	case kMCPlatformPlayerPropertyVolume:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeUInt16);
			uint16_t t_volume = *((uint16_t*)p_value);
			SetVolume(t_volume);

			break;
		}

	case kMCPlatformPlayerPropertyOffscreen:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeBool);
			bool t_offscreen = *((bool*)p_value);
			SetOffscreen(t_offscreen);

			break;
		}

	case kMCPlatformPlayerPropertyMirrored:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeBool);
			bool t_mirrored = *((bool*)p_value);
			SetMirrored(t_mirrored);

			break;
		}

	case kMCPlatformPlayerPropertyOnlyPlaySelection:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeBool);
			bool t_play_selection = *((bool*)p_value);
			SetPlaySelection(t_play_selection);

			break;
		}

	case kMCPlatformPlayerPropertyMarkers:
		{
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDurationArray);
			MCPlatformPlayerDurationArray *t_markers;
			t_markers = (MCPlatformPlayerDurationArray*)p_value;
			SetCallbackMarkers(*t_markers);

			break;
		}

	case kMCPlatformPlayerPropertyScalefactor:
		MCLog("UNIMPLEMENTED");
		break;

	// Can safely ignore these
	case kMCPlatformPlayerPropertyShowSelection:
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCWin32DSPlayer::LockBitmap(const MCGIntegerSize &p_size, MCImageBitmap*& r_bitmap)
{
	// TODO implement offscreen rendering
	return false;
}

void MCWin32DSPlayer::UnlockBitmap(MCImageBitmap *bitmap)
{
	// TODO implement offscreen rendering
}

////////////////////////////////////////////////////////////////////////////////

MCWin32DSPlayer *MCWin32DSPlayerCreate()
{
	MCWin32DSPlayer *t_player;
	t_player = new (nothrow) MCWin32DSPlayer();

	if (t_player == nil)
		return nil;

	if (!t_player->Initialize())
	{
		delete t_player;
		return nil;
	}

	return t_player;
}

////////////////////////////////////////////////////////////////////////////////
