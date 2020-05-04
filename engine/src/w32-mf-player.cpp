/* Copyright (C) 2020 LiveCode Ltd.

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
#include <mfidl.h>
#include <mfapi.h>
#include <Mferror.h>
#include <evr.h>
#include <propvarutil.h>

#include "globals.h"
#include "stack.h"
#include "osspec.h"

#include "graphics_util.h"
#include "platform.h"
#include "platform-internal.h"

////////////////////////////////////////////////////////////////////////////////

DEFINE_GUID(CLSID_VideoProcessorMFT, 0x88753b26, 0x5b24, 0x49bd, 0xb2, 0xe7, 0xc, 0x44, 0x5c, 0x78, 0xc9, 0x82);

////////////////////////////////////////////////////////////////////////////////

extern HINSTANCE MChInst;

void MCWin32BSTRFree(BSTR p_str) { SysFreeString(p_str); }

#define kMCMFEventWindowClass "MFEVENTWINDOWCLASS"
#define kMCMFTimerWindowClass "MFTIMERWINDOWCLASS"
#define kMCMFVideoWindowClass "MFVIDEOWINDOWCLASS"

#define kMCMFTimerID (1)
#define kMCMFTimerInterval (100) // 100 ms, 10x per second

////////////////////////////////////////////////////////////////////////////////

#if _DEBUG
#define CheckHResult(hresult, expr) \
{\
	if (SUCCEEDED(hresult)) \
	{\
		hresult = expr;\
		if (!SUCCEEDED(hresult))\
		{\
			MCLog("Error: hr=%x, expr=\"%s\"", hresult, #expr); \
		}\
	}\
}

#else
#define CheckHResult(hresult, expr) {if (SUCCEEDED(hresult)) {hresult = expr;}}
#endif

#define CheckBool(success_var, bool_op) \
if (success_var) \
{ \
	success_var = bool_op; \
	if (!success_var) \
	{ \
		MCLog("Error: cmd=\"%s\"", #bool_op); \
	} \
}

inline RECT MCWin32MakeRECT(LONG p_left, LONG p_top, LONG p_right, LONG p_bottom)
{
	RECT t_rect;
	t_rect.left = p_left;
	t_rect.top = p_top;
	t_rect.right = p_right;
	t_rect.bottom = p_bottom;

	return t_rect;
}

inline RECT MCWin32RECTTranslate(const RECT &p_rect, LONG p_dx, LONG p_dy)
{
	return MCWin32MakeRECT(p_rect.left + p_dx, p_rect.top + p_dy, p_rect.right + p_dx, p_rect.bottom + p_dy);
}

template <class T>
inline void MCWin32IUnknownAssign(T* &x_var, T* p_value)
{
	if (p_value != nil)
		p_value->AddRef();
	if (x_var != nil)
		x_var->Release();
	x_var = p_value;
}

inline HRESULT MCWin32MFCopyAttribute(IMFAttributes *p_src, IMFAttributes *p_dst, const GUID p_attribute)
{
	HRESULT t_hresult = S_OK;
	PROPVARIANT t_value;
	PropVariantInit(&t_value);
	CheckHResult(t_hresult, p_src->GetItem(p_attribute, &t_value));
	CheckHResult(t_hresult, p_dst->SetItem(p_attribute, t_value));
	PropVariantClear(&t_value);
	return t_hresult;
}

////////////////////////////////////////////////////////////////////////////////

enum MCWin32MFPlayerState
{
	kMCWin32MFPlayerStopped,
	kMCWin32MFPlayerPaused,
	kMCWin32MFPlayerRunning,
};

struct MCWin32MFMediaSourceInfo
{
	MCPlatformPlayerMediaTypes types;
	MCGIntegerSize frame_size;
	uint32_t frame_rate_numerator;
	uint32_t frame_rate_denominator;
	MCPlatformPlayerDuration duration;

	uindex_t audio_track_count;
};

struct MCWin32MFTopologyConfiguration
{
	MCPlatformPlayerDuration start_position;
	MCPlatformPlayerDuration end_position;
	bool play_selection;
	bool mirrored;
};

class MCWin32MFMediaEventHandler
{
public:
	virtual bool HandleMediaEvent(IMFMediaEvent *p_event) = 0;
};

// async session helper class
class MCWin32MFSession : public IMFAsyncCallback, public MCWin32MFMediaEventHandler
{
public:
	enum SessionOperation
	{
		kSessionOpNone = 0,
		kSessionOpStart,
		kSessionOpStop,
		kSessionOpPause,
		kSessionOpSeek,
		kSessionOpRateChange,
		kSessionOpSetTopology,
	};

	enum SessionPlayState
	{
		kSessionPlayStateStart,
		kSessionPlayStateStop,
		kSessionPlayStatePause,
	};

	struct SessionState
	{
		SessionPlayState play_state;
		MFTIME seek_position;
		float play_rate;
	};

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFAsyncCallback methods
	STDMETHODIMP  GetParameters(DWORD*, DWORD*);
	STDMETHODIMP  Invoke(IMFAsyncResult* pAsyncResult);

	static HRESULT CreateInstance(MCWin32MFMediaEventHandler *p_event_handler, MCWin32MFSession **r_instance);

	bool SetTopology(IMFTopology *p_topology);

	bool StartAtPosition(MCPlatformPlayerDuration p_position);
	bool Start();
	bool Pause();
	bool Stop();
	bool GetCurrentPosition(MCPlatformPlayerDuration &r_position);
	bool SetCurrentPosition(MCPlatformPlayerDuration p_position);
	bool SetPlaybackRate(float p_rate);

	IMFMediaSession *GetSession();

	bool Initialize();
	void Shutdown();

	virtual bool HandleMediaEvent(IMFMediaEvent *p_event);

private:
	MCWin32MFSession(MCWin32MFMediaEventHandler *p_event_handler);
	~MCWin32MFSession();

	void ClearState();

	bool RequireTopologyChange();
	bool RequirePlayStateChange();
	bool RequirePlaybackRateChange();
	bool RequireSeek();

	bool UpdateTopology();
	bool UpdatePlayState();
	bool UpdatePlaybackRate();
	bool UpdateSeekPosition();

	bool UpdateRequestState();

	bool DoSetTopology(IMFTopology *p_topology);
	bool DoSetCurrentPosition(MCPlatformPlayerDuration p_position);
	bool DoSetPlaybackRate(float p_rate);
	bool DoStart();
	bool DoStop();
	bool DoPause();

	bool RequestRequiresScrubbing();
	bool IsScrubbing();
	bool DoScrub();

	bool OnSessionTopologySet(HRESULT p_result);
	bool OnSessionStart(HRESULT p_result);
	bool OnSessionStop(HRESULT p_result);
	bool OnSessionPause(HRESULT p_result);
	bool OnSessionEnded(HRESULT p_result);
	bool OnSessionRateChanged(HRESULT p_result, IMFMediaEvent *p_event);

	long m_ref_count;
	CComPtr<IMFMediaSession> m_session;
	CComPtr<IMFRateSupport> m_rate_support;
	CComPtr<IMFRateControl> m_rate_control;
	HWND m_event_window;
	MCWin32MFMediaEventHandler *m_event_handler;

	bool m_in_request;
	SessionOperation m_pending_op;

	SessionState m_current;
	SessionState m_request;
	bool m_do_seek;

	CComPtr<IMFTopology> m_current_topology;
	CComPtr<IMFTopology> m_loading_topology;
	CComPtr<IMFTopology> m_request_topology;

	bool m_shutdown;
	bool m_closing;
};

class MCWin32MFPlayer : public MCPlatformPlayer, public MCWin32MFMediaEventHandler
{
public:
	MCWin32MFPlayer(void);
	virtual ~MCWin32MFPlayer(void);
	
	virtual bool GetNativeView(void *&r_view);
	virtual bool SetNativeParentView(void *p_view);
	
	virtual bool IsPlaying(void);
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
	bool HandleMediaEvent(IMFMediaEvent *p_event);
	bool HandleTimer();
	bool SetVideoWindow(HWND hwnd);
	bool SetVideoWindowSize(uint32_t p_width, uint32_t p_height);
	bool HandlePaint();
	bool HandleShowWindow(bool p_show);

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

	bool PlayAtPosition(MCPlatformPlayerDuration p_position);
	bool Play();
	bool Pause();
	bool SeekRelative(int32_t p_amount);

	bool ConfigurePositions();
	bool SetTopologyConfiguration(const MCWin32MFTopologyConfiguration &p_config);

	bool SetVisible(bool p_visible);
	bool GetFormattedSize(uint32_t &r_width, uint32_t &r_height);

	bool RedisplayCurrentFrame();

	bool SetMediaSource(IMFMediaSource *p_source);
	bool OpenFile(MCStringRef p_filename);
	void CloseFile();

	MCWin32MFPlayerState m_state;
	bool m_is_valid;
	HWND m_video_window;
	HWND m_timer_window;

	MCWin32MFMediaSourceInfo m_source_info;

	MCWin32MFTopologyConfiguration m_topology_config;

	MCPlatformPlayerDuration m_paused_position;

	MCPlatformPlayerDurationArray m_callback_markers;
	index_t m_last_marker;

	MCPlatformPlayerDuration m_start_position;
	MCPlatformPlayerDuration m_end_position;
	bool m_play_selection;
	float m_play_rate;
	bool m_looping;
	MCStringRef m_source_path;

	CComPtr<IMFMediaSource> m_source;
	CComPtr<IMFVideoDisplayControl> m_display_control;
	CComPtr<IMFSimpleAudioVolume> m_audio_volume;
	//CComPtr<IMFVideoProcessorControl> m_video_processor;

	CComPtr<MCWin32MFSession> m_session_helper;
};

////////////////////////////////////////////////////////////////////////////////

#define WM_MFSESSIONEVENT (WM_APP + 1)   // Window message for MediaFoundation event events
LRESULT CALLBACK MCMFEventWindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		CREATESTRUCT *t_create;
		t_create = (CREATESTRUCT*)lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)t_create->lpCreateParams);
		break;

	case WM_MFSESSIONEVENT:
		{
			CComPtr<MCWin32MFSession> t_session;
			t_session = (MCWin32MFSession*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			CComPtr<IMFMediaEvent> t_event;
			t_event.Attach((IMFMediaEvent*)wParam);
			t_session->HandleMediaEvent(t_event);
		}
		break;
	}

	return DefWindowProc (hWnd, message, wParam, lParam);
	
}

bool RegisterEventWindowClass()
{
	WNDCLASS	wc = { 0 };
	static bool windowclassregistered = false;
	if (!windowclassregistered)
	{
		// Register the Monitor child window class
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_SAVEBITS;
		wc.lpfnWndProc = MCMFEventWindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = MChInst;
		wc.hIcon = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = kMCMFEventWindowClass;
		if (RegisterClass(&wc))
			windowclassregistered = true;
	}
	return windowclassregistered;
}

LRESULT CALLBACK MCMFTimerWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		CREATESTRUCT *t_create;
		t_create = (CREATESTRUCT*)lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)t_create->lpCreateParams);
		break;

	case WM_TIMER:
		if (wParam == kMCMFTimerID)
		{
			MCWin32MFPlayer *t_player;
			t_player = (MCWin32MFPlayer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (t_player != nil)
				t_player->HandleTimer();
		}
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);

}

bool RegisterTimerWindowClass()
{
	WNDCLASS	wc={0};
	static bool windowclassregistered = false;
	if (!windowclassregistered) 
	{
		// Register the Monitor child window class
		wc.style         = CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_SAVEBITS;
		wc.lpfnWndProc   = MCMFTimerWindowProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = MChInst;
		wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = kMCMFTimerWindowClass;
		if (RegisterClass(&wc))
			windowclassregistered = true;
	}
	return windowclassregistered;
}

LRESULT CALLBACK MCMFVideoWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HRESULT t_result = S_OK;
	switch(message)
	{
	case WM_CREATE:
		CREATESTRUCT *t_create;
		t_create = (CREATESTRUCT*)lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)t_create->lpCreateParams);
		break;

	case WM_SIZE:
		{
			MCWin32MFPlayer *t_player;
			t_player = (MCWin32MFPlayer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (t_player != nil && t_player->HasVideo())
				t_player->SetVideoWindowSize(LOWORD(lParam), HIWORD(lParam));
			break;
		}

	case WM_PAINT:
		{
			MCWin32MFPlayer *t_player;
			t_player = (MCWin32MFPlayer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (t_player != nil && t_player->HasVideo())
				t_player->HandlePaint();
			else
			{
				RECT t_update_rect;
				GetUpdateRect(hWnd, &t_update_rect, FALSE);
				PAINTSTRUCT t_ps;
				BeginPaint(hWnd, &t_ps);
				SelectObject(t_ps.hdc, GetStockObject(BLACK_BRUSH));
				Rectangle(t_ps.hdc, t_update_rect.left, t_update_rect.top, t_update_rect.right, t_update_rect.bottom);
				EndPaint(hWnd, &t_ps);
			}
			break;
		}

	case WM_SHOWWINDOW:
	{
		MCWin32MFPlayer *t_player;
		t_player = (MCWin32MFPlayer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (t_player != nil && t_player->HasVideo())
			t_player->HandleShowWindow(wParam);
		break;
	}

	default:
		//MCLog("Video window event %d", message);
		break;
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
		t_class.lpfnWndProc = MCMFVideoWindowProc;
		t_class.hInstance = MChInst;
		t_class.lpszClassName = kMCMFVideoWindowClass;

		if (RegisterClassEx(&t_class))
			s_registered = true;
	}

	return s_registered;
}

bool CreateEventWindow(MCWin32MFSession *p_session, HWND &r_window)
{
	if (!RegisterEventWindowClass())
		return false;

	HWND t_window;
	t_window = CreateWindow(kMCMFEventWindowClass, "EventWindow", 0, 0, 0, 2, 3, HWND_MESSAGE, nil, MChInst, p_session);

	if (t_window == nil)
		return false;

	r_window = t_window;
	return true;
}

bool CreateTimerWindow(MCWin32MFPlayer *p_player, HWND &r_window)
{
	if (!RegisterTimerWindowClass())
		return false;

	HWND t_window;
	t_window = CreateWindow(kMCMFTimerWindowClass, "TimerWindow", 0, 0, 0, 2, 3, HWND_MESSAGE, nil, MChInst, p_player);

	if (t_window == nil)
		return false;

	r_window = t_window;
	return true;
}

bool CreateVideoWindow(MCWin32MFPlayer *p_player, HWND p_parent_hwnd, HWND &r_window)
{
	if (!RegisterVideoWindowClass())
		return false;

	HWND t_window;
	t_window = CreateWindow(kMCMFVideoWindowClass, "VideoWindow", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1, 1, p_parent_hwnd, nil, MChInst, p_player);

	if (t_window == nil)
		return false;

	r_window = t_window;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MCWin32MFCreateMediaSource(LPCWSTR p_filename, IMFMediaSource **r_media_source)
{
	HRESULT t_hresult = S_OK;

	CComPtr<IMFSourceResolver> t_source_resolver;
	CheckHResult(t_hresult, MFCreateSourceResolver(&t_source_resolver));

	CComPtr<IUnknown> t_source;
	MF_OBJECT_TYPE t_source_type;
	CheckHResult(t_hresult, t_source_resolver->CreateObjectFromURL(p_filename, MF_RESOLUTION_MEDIASOURCE, NULL, &t_source_type, &t_source));

	CComPtr<IMFMediaSource> t_media_source;
	CheckHResult(t_hresult, t_source->QueryInterface(&t_media_source));

	if (SUCCEEDED(t_hresult))
		*r_media_source = t_media_source.Detach();

	return t_hresult;
}

bool MCWin32MFOpenFile(MCStringRef p_filename, IMFMediaSource **r_media_source)
{
	bool t_success = true;

	MCAutoCustomPointer<OLECHAR, MCWin32BSTRFree> t_filename;
	if (t_success)
		t_success = MCStringConvertToBSTR(p_filename, &t_filename);

	return SUCCEEDED(MCWin32MFCreateMediaSource(*t_filename, r_media_source));
}

HRESULT MCWin32MFMediaTypeGetVideoSize(IMFMediaType *p_type, MCGIntegerSize &r_size)
{
	HRESULT t_hresult = S_OK;

	GUID t_major_type;
	CheckHResult(t_hresult, p_type->GetMajorType(&t_major_type));
	if (SUCCEEDED(t_hresult) && MFMediaType_Video != t_major_type)
		t_hresult = E_UNEXPECTED;

	UINT32 t_width, t_height;
	CheckHResult(t_hresult, MFGetAttributeSize(p_type, MF_MT_FRAME_SIZE, &t_width, &t_height));

	if (SUCCEEDED(t_hresult))
		r_size = MCGIntegerSizeMake(t_width, t_height);

	return t_hresult;
}

HRESULT MCWin32MFMediaTypeGetVideoFrameRate(IMFMediaType *p_type, uint32_t &r_rate_numerator, uint32_t &r_rate_denominator)
{
	HRESULT t_hresult = S_OK;

	GUID t_major_type;
	CheckHResult(t_hresult, p_type->GetMajorType(&t_major_type));
	if (SUCCEEDED(t_hresult) && MFMediaType_Video != t_major_type)
		t_hresult = E_UNEXPECTED;

	UINT32 t_numerator, t_denominator;
	CheckHResult(t_hresult, MFGetAttributeRatio(p_type, MF_MT_FRAME_RATE, &t_numerator, &t_denominator));

	if (SUCCEEDED(t_hresult))
	{
		r_rate_numerator = t_numerator;
		r_rate_denominator = t_denominator;
	}

	return t_hresult;
}

HRESULT MCWin32MFSourceGetInfo(IMFMediaSource *p_source, MCWin32MFMediaSourceInfo &r_info)
{
	HRESULT t_hresult = S_OK;

	MCWin32MFMediaSourceInfo t_info;
	MCMemoryClear(t_info);

	CComPtr<IMFPresentationDescriptor> t_presentation_descriptor;
	CheckHResult(t_hresult, p_source->CreatePresentationDescriptor(&t_presentation_descriptor));

	CheckHResult(t_hresult, t_presentation_descriptor->GetUINT64(MF_PD_DURATION, &t_info.duration));

	DWORD t_stream_descriptor_count;
	CheckHResult(t_hresult, t_presentation_descriptor->GetStreamDescriptorCount(&t_stream_descriptor_count));

	for (uindex_t i = 0; SUCCEEDED(t_hresult) && i < t_stream_descriptor_count; i++)
	{
		CComPtr<IMFStreamDescriptor> t_stream_descriptor;
		BOOL t_selected;
		CheckHResult(t_hresult, t_presentation_descriptor->GetStreamDescriptorByIndex(i, &t_selected, &t_stream_descriptor));

		if (t_selected)
		{
			CComPtr<IMFMediaTypeHandler> t_media_type_handler;
			CheckHResult(t_hresult, t_stream_descriptor->GetMediaTypeHandler(&t_media_type_handler));

			GUID t_major_type;
			CheckHResult(t_hresult, t_media_type_handler->GetMajorType(&t_major_type));

			if (SUCCEEDED(t_hresult))
			{
				if (MFMediaType_Video == t_major_type)
				{
					if ((t_info.types & kMCPlatformPlayerMediaTypeVideo) == 0)
					{
						// get frame size of first video stream in source
						CComPtr<IMFMediaType> t_current_type;
						CheckHResult(t_hresult, t_media_type_handler->GetCurrentMediaType(&t_current_type));

						CheckHResult(t_hresult, MCWin32MFMediaTypeGetVideoSize(t_current_type, t_info.frame_size));
						CheckHResult(t_hresult, MCWin32MFMediaTypeGetVideoFrameRate(t_current_type, t_info.frame_rate_numerator, t_info.frame_rate_denominator));
					}
					t_info.types |= kMCPlatformPlayerMediaTypeVideo;
				}
				else if (MFMediaType_Audio == t_major_type)
				{
					t_info.types |= kMCPlatformPlayerMediaTypeAudio;
					t_info.audio_track_count++;
				}
			}
		}
	}

	if (SUCCEEDED(t_hresult))
		r_info = t_info;

	return t_hresult;
}

HRESULT MCWin32MFCreateVideoProcessorNode(IMFMediaType *p_input_type, MF_VIDEO_PROCESSOR_MIRROR p_video_mirror, IMFTopologyNode **r_transform_node)
{
	HRESULT t_hresult = S_OK;

	CComPtr<IMFTransform> t_transform;
	CheckHResult(t_hresult, t_transform.CoCreateInstance(CLSID_VideoProcessorMFT));

	CComPtr<IMFVideoProcessorControl> t_video_processor;
	CheckHResult(t_hresult, t_transform.QueryInterface(&t_video_processor));

	CComPtr<IMFMediaType> t_transform_media_type;
	CheckHResult(t_hresult, MFCreateMediaType(&t_transform_media_type));
	CheckHResult(t_hresult, t_transform_media_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CheckHResult(t_hresult, t_transform_media_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
	CheckHResult(t_hresult, t_transform_media_type->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
	CheckHResult(t_hresult, t_transform_media_type->SetUINT32(MF_MT_COMPRESSED, FALSE));
	CheckHResult(t_hresult, MCWin32MFCopyAttribute(p_input_type, t_transform_media_type, MF_MT_FRAME_SIZE));
	CheckHResult(t_hresult, MCWin32MFCopyAttribute(p_input_type, t_transform_media_type, MF_MT_FRAME_RATE));
	CheckHResult(t_hresult, MCWin32MFCopyAttribute(p_input_type, t_transform_media_type, MF_MT_INTERLACE_MODE));
	CheckHResult(t_hresult, MCWin32MFCopyAttribute(p_input_type, t_transform_media_type, MF_MT_PIXEL_ASPECT_RATIO));

	CheckHResult(t_hresult, t_transform->SetInputType(0, t_transform_media_type, 0));
	CheckHResult(t_hresult, t_transform->SetOutputType(0, t_transform_media_type, 0));
	CheckHResult(t_hresult, t_video_processor->SetMirror(p_video_mirror));

	CComPtr<IMFTopologyNode> t_transform_node;
	CheckHResult(t_hresult, MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &t_transform_node));
	CheckHResult(t_hresult, t_transform_node->SetObject(t_transform));

	if (SUCCEEDED(t_hresult))
		*r_transform_node = t_transform_node.Detach();

	return t_hresult;
}

void MCWin32MFUpdateTopologyConfiguration(MCWin32MFTopologyConfiguration &x_config, MCPlatformPlayerDuration p_media_duration, bool p_play_selection, MCPlatformPlayerDuration p_start_position, MCPlatformPlayerDuration p_end_position)
{
	MCPlatformPlayerDuration t_start, t_finish;
	t_start = 0;
	t_finish = 0;

	bool t_play_selection;
	t_play_selection = p_play_selection;

	if (p_play_selection)
	{
		t_finish = MCMin(p_media_duration, p_end_position);
		t_start = MCMin(t_finish, p_start_position);

		// don't allow span == 0
		if (t_start == t_finish)
		{
			t_play_selection = false;
			t_start = 0;
			t_finish = 0;
		}
	}

	x_config.start_position = t_start;
	x_config.end_position = t_finish;
	x_config.play_selection = t_play_selection;
}

HRESULT MCWin32MFCreateTopo(IMFMediaSource *p_source, HWND p_video_window, const MCWin32MFTopologyConfiguration &p_config, IMFTopology **r_topology)
{
	HRESULT t_hresult = S_OK;

	CComPtr<IMFTopology> t_topology;
	CheckHResult(t_hresult, MFCreateTopology(&t_topology));

	CComPtr<IMFPresentationDescriptor> t_presentation_descriptor;
	CheckHResult(t_hresult, p_source->CreatePresentationDescriptor(&t_presentation_descriptor));

	DWORD t_stream_descriptor_count;
	CheckHResult(t_hresult, t_presentation_descriptor->GetStreamDescriptorCount(&t_stream_descriptor_count));

	uindex_t t_audio_stream_count;
	t_audio_stream_count = 0;
	uindex_t t_video_stream_count;
	t_video_stream_count = 0;

	for (uindex_t i = 0; SUCCEEDED(t_hresult) && i < t_stream_descriptor_count; i++)
	{
		CComPtr<IMFStreamDescriptor> t_stream_descriptor;
		BOOL t_selected;
		CheckHResult(t_hresult, t_presentation_descriptor->GetStreamDescriptorByIndex(i, &t_selected, &t_stream_descriptor));

		if (t_selected)
		{
			CComPtr<IMFMediaTypeHandler> t_media_type_handler;
			CheckHResult(t_hresult, t_stream_descriptor->GetMediaTypeHandler(&t_media_type_handler));

			GUID t_major_type;
			CheckHResult(t_hresult, t_media_type_handler->GetMajorType(&t_major_type));

			uindex_t t_stream_index;
			t_stream_index = 0;

			CComPtr<IMFTopologyNode> t_transform_node;

			CComPtr<IMFActivate> t_renderer_activate;
			if (SUCCEEDED(t_hresult))
			{
				if (MFMediaType_Video == t_major_type)
				{
					// only support 1 video stream currently
					if (t_video_stream_count > 0)
					{
						CheckHResult(t_hresult, t_presentation_descriptor->DeselectStream(i));
						continue;
					}

					if (p_config.mirrored)
					{
						CComPtr<IMFMediaType> t_video_stream_type;
						CheckHResult(t_hresult, t_media_type_handler->GetCurrentMediaType(&t_video_stream_type));

						CheckHResult(t_hresult, MCWin32MFCreateVideoProcessorNode(t_video_stream_type, MIRROR_HORIZONTAL, &t_transform_node));
					}

					CheckHResult(t_hresult, MFCreateVideoRendererActivate(p_video_window, &t_renderer_activate));

					if (SUCCEEDED(t_hresult))
					{
						t_stream_index = t_video_stream_count;
						t_video_stream_count++;
					}

				}
				else if (MFMediaType_Audio == t_major_type)
				{
					CheckHResult(t_hresult, MFCreateAudioRendererActivate(&t_renderer_activate));
					if (SUCCEEDED(t_hresult))
					{
						t_stream_index = t_audio_stream_count;
						t_audio_stream_count++;
					}
				}
				else
				{
					// unhandled media type - deselect stream and skip to next stream descriptor
					CheckHResult(t_hresult, t_presentation_descriptor->DeselectStream(i));
					continue;
				}
			}

			CComPtr<IMFTopologyNode> t_source_node;
			CheckHResult(t_hresult, MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &t_source_node));
			CheckHResult(t_hresult, t_source_node->SetUnknown(MF_TOPONODE_SOURCE, p_source));
			CheckHResult(t_hresult, t_source_node->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, t_presentation_descriptor));
			CheckHResult(t_hresult, t_source_node->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, t_stream_descriptor));
			CheckHResult(t_hresult, t_topology->AddNode(t_source_node));

			if (p_config.play_selection)
			{
				CheckHResult(t_hresult, t_source_node->SetUINT64(MF_TOPONODE_MEDIASTART, p_config.start_position));
				CheckHResult(t_hresult, t_source_node->SetUINT64(MF_TOPONODE_MEDIASTOP, p_config.end_position));
			}

			CComPtr<IMFTopologyNode> t_output_node;
			CheckHResult(t_hresult, MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &t_output_node));
			CheckHResult(t_hresult, t_output_node->SetUINT32(MF_TOPONODE_STREAMID, t_stream_index));
			CheckHResult(t_hresult, t_output_node->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE));
			CheckHResult(t_hresult, t_output_node->SetObject(t_renderer_activate));
			CheckHResult(t_hresult, t_topology->AddNode(t_output_node));

			if (t_transform_node != nil)
			{
				CheckHResult(t_hresult, t_topology->AddNode(t_transform_node));
				CheckHResult(t_hresult, t_source_node->ConnectOutput(0, t_transform_node, 0));
				CheckHResult(t_hresult, t_transform_node->ConnectOutput(0, t_output_node, t_stream_index));
			}
			else
			{
				CheckHResult(t_hresult, t_source_node->ConnectOutput(0, t_output_node, t_stream_index));
			}
		}
	}

	if (SUCCEEDED(t_hresult))
		*r_topology = t_topology.Detach();

	return t_hresult;
}

HRESULT MCWin32MFResolveTopologyNode(IMFTopologyNode *p_node)
{
	HRESULT t_hresult = S_OK;

	MF_TOPOLOGY_TYPE t_node_type;
	CheckHResult(t_hresult, p_node->GetNodeType(&t_node_type));

	switch (t_node_type)
	{
	case MF_TOPOLOGY_OUTPUT_NODE:
	{
		CComPtr<IUnknown> t_node_obj;
		CheckHResult(t_hresult, p_node->GetObject(&t_node_obj));

		if (SUCCEEDED(t_hresult))
		{
			bool t_resolved = false;
			CComPtr<IMFStreamSink> t_stream_sink;
			t_resolved = S_OK == t_node_obj.QueryInterface(&t_stream_sink);

			if (!t_resolved)
			{
				CComPtr<IMFActivate> t_activate;
				CheckHResult(t_hresult, t_node_obj.QueryInterface(&t_activate));
				CComPtr<IMFMediaSink> t_media_sink;
				CheckHResult(t_hresult, t_activate->ActivateObject(__uuidof(t_media_sink), (void**)&t_media_sink));
				UINT32 t_stream_id = 0;
				if (SUCCEEDED(t_hresult))
				{
					// get stream id, use default (0) if not set
					if (S_OK != p_node->GetUINT32(MF_TOPONODE_STREAMID, &t_stream_id))
						t_stream_id = 0;

					if (S_OK != t_media_sink->GetStreamSinkById(t_stream_id, &t_stream_sink))
						CheckHResult(t_hresult, t_media_sink->AddStreamSink(t_stream_id, NULL, &t_stream_sink));
				}
				CheckHResult(t_hresult, p_node->SetObject(t_stream_sink));
			}
		}
	}
	break;

	case MF_TOPOLOGY_TRANSFORM_NODE:
	{
		CComPtr<IUnknown> t_node_obj;
		CheckHResult(t_hresult, p_node->GetObject(&t_node_obj));

		bool t_resolved = false;
		CComPtr<IMFTransform> t_transform;
		if (SUCCEEDED(t_hresult))
			t_resolved = S_OK == t_node_obj.QueryInterface(&t_transform);

		if (!t_resolved)
		{
			CComPtr<IMFActivate> t_activate;
			CheckHResult(t_hresult, t_node_obj.QueryInterface(&t_activate));
			CheckHResult(t_hresult, t_activate->ActivateObject(__uuidof(t_transform), (void**)&t_transform));
			CheckHResult(t_hresult, p_node->SetObject(t_transform));
		}
	}
	break;
	}

	return t_hresult;
}

HRESULT MCWin32MFResolveTopology(IMFTopology *p_topo, IMFTopology **r_resolved)
{
	HRESULT t_hresult = S_OK;

	// bind nodes
	WORD t_node_count = 0;
	CheckHResult(t_hresult, p_topo->GetNodeCount(&t_node_count));

	for (uindex_t i = 0; i < t_node_count; i++)
	{
		CComPtr<IMFTopologyNode> t_node;
		CheckHResult(t_hresult, p_topo->GetNode(i, &t_node));
		CheckHResult(t_hresult, MCWin32MFResolveTopologyNode(t_node));
	}

	CComPtr<IMFTopoLoader> t_topo_loader;
	CheckHResult(t_hresult, MFCreateTopoLoader(&t_topo_loader));

	CComPtr<IMFTopology> t_resolved;
	CheckHResult(t_hresult, t_topo_loader->Load(p_topo, &t_resolved, NULL));

	if (SUCCEEDED(t_hresult))
		*r_resolved = t_resolved.Detach();

	return t_hresult;
}

template <class T>
HRESULT MCWin32MFSessionGetService(IMFMediaSession *p_session, GUID p_service_id, T **r_service)
{
	HRESULT t_hresult = S_OK;

	CComPtr<T> t_service;
	CheckHResult(t_hresult, MFGetService(p_session, p_service_id, __uuidof(t_service), (LPVOID*)&t_service));
	if (SUCCEEDED(t_hresult))
		*r_service = t_service.Detach();

	return t_hresult;
}

HRESULT MCWin32MFSessionGetPresentationTime(IMFMediaSession *p_session, MFTIME &r_time)
{
	HRESULT t_hresult = S_OK;

	CComPtr<IMFClock> t_clock;
	CheckHResult(t_hresult, p_session->GetClock(&t_clock));

	CComPtr<IMFPresentationClock> t_presentation_clock;
	CheckHResult(t_hresult, t_clock->QueryInterface(&t_presentation_clock));

	MFTIME t_time;
	CheckHResult(t_hresult, t_presentation_clock->GetTime(&t_time));

	if (SUCCEEDED(t_hresult))
		r_time = t_time;

	return t_hresult;
}

template <class T>
HRESULT MCWin32MFEventGetValueAsInterface(IMFMediaEvent *p_event, T **r_interface)
{
	HRESULT t_hresult = S_OK;

	CComPtr<T> t_interface;

	PROPVARIANT t_var;
	PropVariantInit(&t_var);

	CheckHResult(t_hresult, p_event->GetValue(&t_var));

	if (SUCCEEDED(t_hresult) && t_var.vt != VT_UNKNOWN)
		t_hresult = MF_E_UNEXPECTED;

	CheckHResult(t_hresult, t_var.punkVal->QueryInterface(&t_interface));

	PropVariantClear(&t_var);

	if (SUCCEEDED(t_hresult))
		*r_interface = t_interface.Detach();

	return t_hresult;
}

bool MCWin32MFClosestPlayRate(IMFRateSupport *p_rate_support, float p_rate, bool &r_thin, float &r_rate)
{
	if (p_rate_support == nil)
		return false;

	float t_nearest;
	HRESULT t_result;
	t_result = p_rate_support->IsRateSupported(FALSE, p_rate, &t_nearest);
	if (t_result == S_OK)
	{
		// rate supported
		r_rate = p_rate;
		r_thin = false;
		return true;
	}
	if (t_result == MF_E_REVERSE_UNSUPPORTED)
	{
		// reverse rates unsupported
		return false;
	}

	float t_nearest_thin;
	t_result = p_rate_support->IsRateSupported(TRUE, p_rate, &t_nearest_thin);
	if (t_result == MF_E_THINNING_UNSUPPORTED)
	{
		// return nearest unthinned rate
		r_rate = t_nearest;
		r_thin = false;
		return true;
	}

	// compare nearest thin & non-thin rate - return the closest
	if (MCAbs(p_rate - t_nearest_thin) < MCAbs(p_rate - t_nearest))
	{
		r_rate = t_nearest_thin;
		r_thin = true;
	}
	else
	{
		r_rate = t_nearest;
		r_thin = false;
	}

	return true;
}

HRESULT MCWin32MFSessionStart(IMFMediaSession *p_session, const MCPlatformPlayerDuration *p_new_position)
{
	HRESULT t_hresult = S_OK;

	PROPVARIANT t_start;
	PropVariantInit(&t_start);
	if (p_new_position != nil)
		CheckHResult(t_hresult, InitPropVariantFromInt64(*p_new_position, &t_start));

	CheckHResult(t_hresult, p_session->Start(&GUID_NULL, &t_start));

	PropVariantClear(&t_start);

	return t_hresult;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWin32MFPlayer::HandleMediaEvent(IMFMediaEvent *p_event)
{
	bool t_success;
	t_success = true;

	MediaEventType t_event_type;
	if (t_success)
		t_success = SUCCEEDED(p_event->GetType(&t_event_type));

	HRESULT t_result;
	if (t_success)
		t_success = SUCCEEDED(p_event->GetStatus(&t_result));

	HRESULT t_dbg_HRESULT_result;
	BOOL t_dbg_BOOL_result;

	if (t_success)
	{
		switch (t_event_type)
		{
		case MESessionTopologyStatus:
		{
			CComPtr<IMFTopology> t_topo;
			if (t_success)
				t_success = SUCCEEDED(MCWin32MFEventGetValueAsInterface(p_event, &t_topo));

			UINT32 t_topo_status;
			if (t_success)
				t_success = SUCCEEDED(p_event->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &t_topo_status));

			if (t_success)
			{
				switch (t_topo_status)
				{
				case MF_TOPOSTATUS_READY:
				{
					// Query the topology for the audio volume control service
					CComPtr<IMFSimpleAudioVolume> t_audio_volume;
					/* UNCHECKED */ MCWin32MFSessionGetService(m_session_helper->GetSession(), MR_POLICY_VOLUME_SERVICE, &t_audio_volume);

					// Query the topology for the video display control service
					CComPtr<IMFVideoDisplayControl> t_display_control;
					/* UNCHECKED */ MCWin32MFSessionGetService(m_session_helper->GetSession(), MR_VIDEO_RENDER_SERVICE, &t_display_control);

					m_audio_volume = t_audio_volume;
					m_display_control = t_display_control;

					if (t_success && m_display_control != nil)
					{
						/* UNCHECKED */ t_dbg_BOOL_result = SetVideoWindow(m_video_window);
						/* UNCHECKED */ t_dbg_HRESULT_result = m_display_control->SetAspectRatioMode(MFVideoARMode_PreservePicture);
						/* UNCHECKED */ t_dbg_HRESULT_result = m_display_control->SetRenderingPrefs(MFVideoRenderPrefs_DoNotRepaintOnStop);
					}
				}
				break;
				}
			}
			break;
		}
		case MEEndOfPresentation:
		{
			if (m_state == kMCWin32MFPlayerRunning)
			{
				m_state = kMCWin32MFPlayerStopped;
				if (m_looping)
				{
					if (m_session_helper != nil)
						m_session_helper->Pause();
					Play();
				}
				else
				{
					StopTimer();
				}

				MCPlatformCallbackSendPlayerFinished(this);
			}
			break;
		}
		}
	}

    return t_success;
}

bool MCWin32MFPlayer::HandleTimer()
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

bool MCWin32MFPlayer::HandlePaint()
{
	if (m_display_control == nil)
		return false;

	HRESULT t_result;
	t_result = m_display_control->RepaintVideo();

	return t_result == S_OK;
}

bool MCWin32MFPlayer::HandleShowWindow(bool p_show)
{
	bool t_success = true;

	if (p_show)
		t_success = RedisplayCurrentFrame();

	return t_success;
}

bool MCWin32MFPlayer::RedisplayCurrentFrame()
{
	bool t_success = true;

	if (m_state != kMCWin32MFPlayerRunning)
	{
		MCPlatformPlayerDuration t_position;
		if (t_success)
			t_success = GetCurrentPosition(t_position);
		if (t_success)
			t_success = SetCurrentPosition(t_position);
	}

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

MCWin32MFPlayer::MCWin32MFPlayer()
{
	m_video_window = nil;
	m_state = kMCWin32MFPlayerStopped;

	MCMemoryClear(m_topology_config);

	m_start_position = 0;
	m_end_position = 0;
	m_play_selection = false;

	m_play_rate = 1.0;
	m_paused_position = 0;
	m_looping = false;

	m_source_path = MCValueRetain(kMCEmptyString);

	MCMemoryClear(m_source_info);
	m_is_valid = false;

	m_callback_markers.count = 0;
	m_callback_markers.ptr = nil;
	m_last_marker = -1;
}

MCWin32MFPlayer::~MCWin32MFPlayer()
{
	CloseFile();

	if (m_display_control)
		m_display_control->SetVideoWindow(nil);

	if (m_session_helper != nil)
		m_session_helper->Shutdown();

	if (m_timer_window != nil)
		DestroyWindow(m_timer_window);

	if (m_video_window != nil)
		DestroyWindow(m_video_window);

	MCPlatformArrayClear(m_callback_markers);

	if (m_source_path != nil)
		MCValueRelease(m_source_path);
}

bool MCWin32MFPlayer::Initialize()
{
	bool t_success;
	t_success = true;

	HRESULT t_result;
	t_result = S_OK;

	HWND t_timer_window = nil;
	if (t_success)
		t_success = CreateTimerWindow(this, t_timer_window);

	if (t_success)
	{
		m_timer_window = t_timer_window;
	}
	else
	{
		if (t_timer_window != nil)
			DestroyWindow(t_timer_window);
	}

	return t_success;
}

bool MCWin32MFPlayer::HasVideo()
{
	return 0 != (m_source_info.types & kMCPlatformPlayerMediaTypeVideo);
}

bool MCWin32MFPlayer::HasAudio()
{
	return 0 != (m_source_info.types & kMCPlatformPlayerMediaTypeAudio);
}

//tell MF where to position video window
bool MCWin32MFPlayer::SetVideoWindow(HWND p_hwnd)
{
	if (m_display_control == nil)
		return false;

	bool t_success;
	t_success = true;

	if (t_success)
		t_success = SUCCEEDED(m_display_control->SetVideoWindow(p_hwnd));

	if (t_success && p_hwnd != nil)
	{
	    RECT t_rect;
		if (t_success)
			t_success = GetClientRect(p_hwnd, &t_rect);

		RECT t_new_rect;
		t_new_rect = MCWin32RECTTranslate(t_rect, -t_rect.left, -t_rect.top);
		if (t_success)
			t_success = SUCCEEDED(m_display_control->SetVideoPosition(NULL, &t_new_rect));
	}

    return t_success;
}

bool MCWin32MFPlayer::SetVideoWindowSize(uint32_t p_width, uint32_t p_height)
{
	if (m_display_control == nil)
		return false;

	bool t_success;
	t_success = true;

	RECT t_new_rect;
	if (t_success)
	{
		t_new_rect = MCWin32MakeRECT(0, 0, p_width, p_height);
		t_success = SUCCEEDED(m_display_control->SetVideoPosition(NULL, &t_new_rect));
	}

	return t_success;
}

bool MCWin32MFPlayer::SetVisible(bool isVisible)
{
	/* TODO - figure out what if anything to do here */
	return true;
}

bool MCWin32MFPlayer::SetMediaSource(IMFMediaSource *p_source)
{
	bool t_success;
	t_success = true;

	CComPtr<IMFTopology> t_topology;
	CComPtr<MCWin32MFSession> t_session_helper;

	MCWin32MFMediaSourceInfo t_info;
	MCMemoryClear(t_info);

	if (p_source != nil)
	{
		if (t_success)
			t_success = SUCCEEDED(MCWin32MFSourceGetInfo(p_source, t_info));

		MCWin32MFTopologyConfiguration t_config = m_topology_config;
		MCWin32MFUpdateTopologyConfiguration(t_config, t_info.duration, m_play_selection, m_start_position, m_end_position);

		if (t_success)
			t_success = SUCCEEDED(MCWin32MFCreateTopo(p_source, m_video_window, t_config, &t_topology));

		CComPtr<IMFTopology> t_resolved_topology;
		if (t_success)
			t_success = SUCCEEDED(MCWin32MFResolveTopology(t_topology, &t_resolved_topology));
		if (t_success)
			t_topology = t_resolved_topology;

		if (t_success)
			t_success = SUCCEEDED(t_topology->SetUINT32(MF_TOPOLOGY_HARDWARE_MODE, MFTOPOLOGY_HWMODE_USE_HARDWARE));
		if (t_success)
			t_success = SUCCEEDED(t_topology->SetUINT32(MF_TOPOLOGY_DXVA_MODE, MFTOPOLOGY_DXVA_FULL));

		// Create session for topology
		if (t_success)
			t_success = SUCCEEDED(MCWin32MFSession::CreateInstance(this, &t_session_helper));
		if (t_success)
			t_success = t_session_helper->Initialize();
		if (t_success)
			t_success = t_session_helper->SetTopology(t_topology);
	}

	if (t_success)
	{
		if (m_display_control)
			m_display_control.Release();
		if (m_audio_volume)
			m_audio_volume.Release();

		if (m_session_helper != nil)
		{
			m_session_helper->Shutdown();
			m_session_helper.Release();
		}
		m_session_helper = t_session_helper;

		m_source = p_source;
		m_source_info = t_info;
	}

	return t_success;
}

bool MCWin32MFPlayer::OpenFile(MCStringRef p_filename)
{
	bool t_success;
	t_success = true;

	CComPtr<IMFMediaSource> t_source;
	if (t_success && !MCStringIsEmpty(p_filename))
		t_success = MCWin32MFOpenFile(p_filename, &t_source);

	if (t_success)
		t_success = SetMediaSource(t_source);

	m_is_valid = t_success;

	return t_success;
}

void MCWin32MFPlayer::CloseFile()
{
	SetVideoWindow(nil);
	SetMediaSource(nil);
	m_state = kMCWin32MFPlayerStopped;
}

bool MCWin32MFPlayer::SetTopologyConfiguration(const MCWin32MFTopologyConfiguration &p_config)
{
	if (m_session_helper == nil)
		return true;

	bool t_success = true;

	bool t_restore_position = m_state != kMCWin32MFPlayerStopped;

	MCPlatformPlayerDuration t_current_position = 0;
	if (t_success && t_restore_position)
		t_success = GetCurrentPosition(t_current_position);

	if (t_success)
		t_success = SetMediaSource(nil);

	if (t_success)
	{
		m_state = kMCWin32MFPlayerStopped;
		m_topology_config = p_config;
		t_success = OpenFile(m_source_path);
	}

	if (t_success && t_restore_position)
		t_success = SetCurrentPosition(t_current_position);

	if (t_success)
		MCPlatformCallbackSendPlayerCurrentTimeChanged(this);

	return t_success;
}

bool MCWin32MFPlayer::StartTimer()
{
	UINT t_elapse;
	return 0 != SetTimer(m_timer_window, kMCMFTimerID, kMCMFTimerInterval, nil);
}

void MCWin32MFPlayer::StopTimer()
{
	KillTimer(m_timer_window, kMCMFTimerID);
}

bool MCWin32MFPlayer::GetFormattedSize(uint32_t &r_width, uint32_t &r_height)
{
	if (!HasVideo() && HasAudio())
	{
		r_width = r_height = 0;
		return true;
	}

	if (!HasVideo())
		return false;

	r_width = m_source_info.frame_size.width;
	r_height = m_source_info.frame_size.height;

	return true;
}

//////////

bool MCWin32MFPlayer::SetUrl(MCStringRef p_url)
{
	bool t_success;
	t_success = true;

	if (MCStringIsEqualTo(m_source_path, p_url, kMCStringOptionCompareExact))
		return true;

	MCValueRetain(p_url);
	MCValueRelease(m_source_path);
	m_source_path = p_url;

	if (t_success)
		t_success = OpenFile(p_url);

	Pause();

	return t_success;
}

bool MCWin32MFPlayer::GetPlayRate(double &r_play_rate)
{
	r_play_rate = m_play_rate;
	return true;
}

bool MCWin32MFPlayer::SetPlayRate(double p_play_rate)
{
	if (m_session_helper == nil)
		return false;

	if (p_play_rate == m_play_rate)
		return true;

	bool t_success = true;
	t_success = m_session_helper->SetPlaybackRate(p_play_rate);

	m_play_rate = p_play_rate;
	return t_success;
}

bool MCWin32MFPlayer::GetTimeScale(MCPlatformPlayerDuration &r_time_scale)
{
	// MF uses standard reference time units of 100 nanoseconds (10M units per second)
	r_time_scale = 10000000;
	return true;
}

bool MCWin32MFPlayer::GetDuration(MCPlatformPlayerDuration &r_duration)
{
	if (m_source == nil)
		return false;

	r_duration = m_source_info.duration;
	return true;
}

bool MCWin32MFPlayer::GetCurrentPosition(MCPlatformPlayerDuration &r_position)
{
	if (m_session_helper == nil)
		return false;

	if (m_state == kMCWin32MFPlayerPaused)
	{
		r_position = m_paused_position;
		return true;
	}

	MCPlatformPlayerDuration t_current_position = 0;
	if (!m_session_helper->GetCurrentPosition(t_current_position))
		return false;

	// the current position is offset by the start position
	if (m_topology_config.play_selection)
		t_current_position += m_topology_config.start_position;
	r_position = t_current_position;

	return true;
}

bool MCWin32MFPlayer::SetCurrentPosition(MCPlatformPlayerDuration p_position)
{
	if (m_session_helper == nil)
		return false;

	MCPlatformPlayerDuration t_position = p_position;
	if (m_topology_config.play_selection)
		t_position = MCClamp(t_position, m_topology_config.start_position, m_topology_config.end_position) - m_topology_config.start_position;

	bool t_success = true;
	if (t_success)
		t_success = m_session_helper->SetCurrentPosition(t_position);

	// Enter paused state if not currently playing
	if (t_success && m_state != kMCWin32MFPlayerRunning)
	{
		m_state = kMCWin32MFPlayerPaused;
		if (m_topology_config.play_selection)
			t_position += m_topology_config.start_position;
		m_paused_position = t_position;
	}

	return t_success;
}

bool MCWin32MFPlayer::ConfigurePositions()
{
	MCWin32MFTopologyConfiguration t_config = m_topology_config;
	MCWin32MFUpdateTopologyConfiguration(t_config, m_source_info.duration, m_play_selection, m_start_position, m_end_position);

	if (MCMemoryCompare(&t_config, &m_topology_config, sizeof(MCWin32MFTopologyConfiguration)) == 0)
	{
		// no change in topology configuration
		return true;
	}

	return SetTopologyConfiguration(t_config);
}

bool MCWin32MFPlayer::GetStartPosition(MCPlatformPlayerDuration &r_position)
{
	r_position = m_start_position;
	return true;
}

bool MCWin32MFPlayer::SetStartPosition(MCPlatformPlayerDuration p_position)
{
	if (p_position == m_start_position)
		return true;

	m_start_position = p_position;

	return ConfigurePositions();
}

bool MCWin32MFPlayer::GetFinishPosition(MCPlatformPlayerDuration &r_position)
{
	r_position = m_end_position;
	return true;
}

bool MCWin32MFPlayer::SetFinishPosition(MCPlatformPlayerDuration p_position)
{
	if (p_position == m_end_position)
		return true;

	m_end_position = p_position;

	return ConfigurePositions();
}

bool MCWin32MFPlayer::GetLoadedPosition(MCPlatformPlayerDuration &r_loaded)
{
	/* TODO - implement (unsupported?) */
	return false;
}

bool MCWin32MFPlayer::GetPlaySelection(bool &r_play_selection)
{
	r_play_selection = m_play_selection;
	return true;
}

bool MCWin32MFPlayer::SetPlaySelection(bool p_play_selection)
{
	if (p_play_selection == m_play_selection)
		return true;

	m_play_selection = p_play_selection;

	// Update configured start / finish positions.
	return ConfigurePositions();
}

float percentToVolume(int percent)
{
	return MCClamp(percent / 100.0, 0.0, 1.0);
}

int volumeToPercent(float volume)
{
	return MCClamp((int)(100 * volume), 0, 100);
}

bool MCWin32MFPlayer::GetVolume(uint16_t &r_volume)
{
	if (m_audio_volume == nil)
		return false;

	float t_level;
	if (!SUCCEEDED(m_audio_volume->GetMasterVolume(&t_level)))
		return false;

	r_volume = volumeToPercent(t_level);
	return true;
}

bool MCWin32MFPlayer::SetVolume(uint16_t p_volume)
{
	if (m_audio_volume == nil)
		return false;

	return SUCCEEDED(m_audio_volume->SetMasterVolume(percentToVolume(p_volume)));
}

bool MCWin32MFPlayer::GetLoop(bool &r_loop)
{
	r_loop = m_looping;
	return true;
}

bool MCWin32MFPlayer::SetLoop(bool p_loop)
{
	m_looping = p_loop;
	return true;
}

// TODO - implement offscreen property
bool MCWin32MFPlayer::GetOffscreen(bool &r_offscreen)
{
	r_offscreen = false;
	return true;
}

bool MCWin32MFPlayer::SetOffscreen(bool p_offscreen)
{
	return false;
}

bool MCWin32MFPlayer::GetMirrored(bool &r_mirrored)
{
	r_mirrored = m_topology_config.mirrored;
	return true;
}

bool MCWin32MFPlayer::SetMirrored(bool p_mirrored)
{
	if (m_topology_config.mirrored == p_mirrored)
		return true;

	MCWin32MFTopologyConfiguration t_config = m_topology_config;
	t_config.mirrored = p_mirrored;

	return SetTopologyConfiguration(t_config);
}

bool MCWin32MFPlayer::GetMediaTypes(MCPlatformPlayerMediaTypes &r_types)
{
	if (m_source == nil)
		return false;

	r_types = m_source_info.types;
	return true;
}

bool MCWin32MFPlayer::SetCallbackMarkers(const MCPlatformPlayerDurationArray &p_markers)
{
	MCPlatformPlayerDurationArray t_markers = { nil, 0 };

	if (!MCPlatformArrayCopy(p_markers, t_markers))
		return false;

	MCPlatformArrayClear(m_callback_markers);
	m_callback_markers = t_markers;
	m_last_marker = -1;

	return true;
}

// play at position
bool MCWin32MFPlayer::PlayAtPosition(MCPlatformPlayerDuration p_position)
{
	if (m_session_helper == nil)
		return false;

	if (m_topology_config.play_selection)
		p_position = MCClamp(p_position, m_topology_config.start_position, m_topology_config.end_position) - m_topology_config.start_position;

	bool t_success = true;
	if (t_success)
		t_success = m_session_helper->StartAtPosition(p_position);

	if (t_success)
	{
		StartTimer();

		m_state = kMCWin32MFPlayerRunning;
	}

	return t_success;
}

//play
bool MCWin32MFPlayer::Play()
{
	if (m_session_helper == nil)
		return false;

	if (m_state == kMCWin32MFPlayerStopped)
	{
		// if play has stopped, then reset stream to the beginning
		return PlayAtPosition(0);
	}
	else if (m_state == kMCWin32MFPlayerPaused)
	{
		// resume from previous position
		return PlayAtPosition(m_paused_position);
	}
	else
	{
		bool t_success = true;

		if (t_success)
			t_success = m_session_helper->Start();

		if (t_success)
		{
			StartTimer();

			m_state = kMCWin32MFPlayerRunning;
		}

		return t_success;
	}
}

//pause playback
bool MCWin32MFPlayer::Pause()
{
	if (m_session_helper == nil)
		return false;

	bool t_success;
	t_success = true;

	if (m_state == kMCWin32MFPlayerRunning)
	{
		MCPlatformPlayerDuration t_position = 0;
		if (t_success)
			t_success = GetCurrentPosition(t_position);

		if (t_success)
			t_success = m_session_helper->Stop();

		if (t_success)
		{
			StopTimer();

			m_paused_position = t_position;
			m_state = kMCWin32MFPlayerPaused;
		}
	}
	else if (m_state == kMCWin32MFPlayerStopped)
	{
		m_paused_position = 0;
		m_state = kMCWin32MFPlayerPaused;
	}

	return t_success;
}

// seek forward / backward n frames
bool MCWin32MFPlayer::SeekRelative(int32_t p_amount)
{
	if (m_session_helper == nil)
		return false;

	LONGLONG t_units_per_frame;
	t_units_per_frame = 10000000 * (LONGLONG)m_source_info.frame_rate_denominator / m_source_info.frame_rate_numerator;

	bool t_success;
	t_success = true;
	MCPlatformPlayerDuration t_current;
	if (t_success)
		t_success = GetCurrentPosition(t_current);
	if (t_success)
		t_success = SetCurrentPosition(t_current + p_amount * t_units_per_frame);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWin32MFPlayer::GetNativeView(void *&r_view)
{
	if (m_video_window == nil)
		return false;

	r_view = m_video_window;
	return true;
}

bool MCWin32MFPlayer::SetNativeParentView(void *p_view)
{
	if (m_video_window != nil)
		return true;

	HWND t_video_window = nil;
	if (!CreateVideoWindow(this, (HWND)p_view, t_video_window))
		return false;

	if ((m_session_helper != nil) && HasVideo() && !SetVideoWindow(t_video_window))
	{
		DestroyWindow(t_video_window);
		return false;
	}

	m_video_window = t_video_window;

	return true;
}

void MCWin32MFPlayer::Realize()
{
	// STUB
}

void MCWin32MFPlayer::Unrealize()
{
	// STUB
}

void MCWin32MFPlayer::Start(double p_play_rate)
{
	if (!SetPlayRate(p_play_rate))
		return;

	Play();
}

void MCWin32MFPlayer::Stop()
{
	Pause();
}

void MCWin32MFPlayer::Step(int p_amount)
{
	SeekRelative(p_amount);
}

bool MCWin32MFPlayer::IsPlaying()
{
	return m_state == kMCWin32MFPlayerRunning;
}

void MCWin32MFPlayer::CountTracks(uindex_t &r_track_count)
{
	// TODO - implement track switching support
	r_track_count = 0;
}

bool MCWin32MFPlayer::FindTrackWithId(uint32_t p_id, uindex_t &r_index)
{
	// TODO - implement track switching support
	return false;
}

void MCWin32MFPlayer::GetTrackProperty(uindex_t p_index, MCPlatformPlayerTrackProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	// TODO - implement track switching support
}

void MCWin32MFPlayer::SetTrackProperty(uindex_t p_index, MCPlatformPlayerTrackProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
	// TODO - implement track switching support
}

void MCWin32MFPlayer::GetProperty(MCPlatformPlayerProperty p_property, MCPlatformPropertyType p_type, void *r_value)
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

void MCWin32MFPlayer::SetProperty(MCPlatformPlayerProperty p_property, MCPlatformPropertyType p_type, void *p_value)
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

bool MCWin32MFPlayer::LockBitmap(const MCGIntegerSize &p_size, MCImageBitmap*& r_bitmap)
{
	// TODO implement offscreen rendering
	return false;
}

void MCWin32MFPlayer::UnlockBitmap(MCImageBitmap *bitmap)
{
	// TODO implement offscreen rendering
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MCWin32MFSession::CreateInstance(MCWin32MFMediaEventHandler *p_event_handler, MCWin32MFSession **r_instance)
{
	if (r_instance == nil)
		return E_POINTER;

	MCWin32MFSession *t_instance;
	t_instance = new MCWin32MFSession(p_event_handler);

	if (t_instance == nil)
		return E_OUTOFMEMORY;

	*r_instance = t_instance;

	return S_OK;
}

MCWin32MFSession::MCWin32MFSession(MCWin32MFMediaEventHandler *p_event_handler) :
	m_ref_count(1),
	m_event_handler(p_event_handler),
	m_event_window(nil),
	m_in_request(false),
	m_pending_op(kSessionOpNone),
	m_do_seek(false),
	m_closing(false),
	m_shutdown(false)
{
	MCMemoryClear(m_current);
	MCMemoryClear(m_request);
	ClearState();
}

void MCWin32MFSessionClearState(MCWin32MFSession::SessionState &x_state)
{
	x_state.play_state = MCWin32MFSession::SessionPlayState::kSessionPlayStateStop;
	x_state.seek_position = 0;
	x_state.play_rate = 1.0;
}

void MCWin32MFSession::ClearState()
{
	MCWin32MFSessionClearState(m_current);
	MCWin32MFSessionClearState(m_request);
}

MCWin32MFSession::~MCWin32MFSession()
{
	if (m_event_window != nil)
		DestroyWindow(m_event_window);
}

bool MCWin32MFSession::Initialize()
{
	bool t_success = true;

	if (t_success)
		t_success = CreateEventWindow(this, m_event_window);

	if (t_success)
		t_success = SUCCEEDED(MFCreateMediaSession(NULL, &m_session));

	if (t_success)
		t_success = SUCCEEDED(m_session->BeginGetEvent(this, NULL));

	return t_success;
}

void MCWin32MFSession::Shutdown()
{
	if (m_closing || m_shutdown)
		return;

	//ClearState();

	// stop listening for further events, don't forward any received
	m_event_handler = nil;

	// start async session closing process
	if (m_session != nil)
		m_closing = S_OK == m_session->Close();

	// Prevent deletion until session closed
	if (m_closing)
		AddRef();
}

// IUnknown methods
HRESULT MCWin32MFSession::QueryInterface(REFIID riid, void** ppv)
{
	static const QITAB s_qit[] =
	{
		QITABENT(MCWin32MFSession, IMFAsyncCallback),
		{0},
	};

	return QISearch(this, s_qit, riid, ppv);
}

ULONG MCWin32MFSession::AddRef()
{
	return InterlockedIncrement(&m_ref_count);
}

ULONG MCWin32MFSession::Release()
{
	ULONG t_count;
	t_count = InterlockedDecrement(&m_ref_count);

	if (t_count == 0)
		delete this;

	return t_count;
}

// IMFAsyncCallback methods
HRESULT MCWin32MFSession::GetParameters(DWORD*, DWORD*)
{
	return E_NOTIMPL;
}

HRESULT MCWin32MFSession::Invoke(IMFAsyncResult* pAsyncResult)
{
	if (m_session == nil)
		return S_OK;

	bool t_success;
	t_success = true;

	CComPtr<IMFMediaEvent> t_event;
	if (t_success)
		t_success = SUCCEEDED(m_session->EndGetEvent(pAsyncResult, &t_event));

	// if not closing, begin fetching next event
	MediaEventType t_type;
	if (t_success)
		t_success = SUCCEEDED(t_event->GetType(&t_type));
	if (t_success && t_type != MESessionClosed)
		t_success = SUCCEEDED(m_session->BeginGetEvent(this, NULL));

	if (t_success && m_event_window != nil)
		t_success = PostMessage(m_event_window, WM_MFSESSIONEVENT, (WPARAM)t_event.p, NULL);

	if (t_success)
		t_event.Detach();

	return S_OK;
}

//////////

bool MCWin32MFSession::HandleMediaEvent(IMFMediaEvent *p_event)
{
	bool t_success;
	t_success = true;

	MediaEventType t_event_type;
	if (t_success)
		t_success = SUCCEEDED(p_event->GetType(&t_event_type));

	HRESULT t_result;
	if (t_success)
		t_success = SUCCEEDED(p_event->GetStatus(&t_result));

	if (t_success)
	{
		switch (t_event_type)
		{
		case MESessionTopologySet:
		{
			t_success = OnSessionTopologySet(t_result);
			break;
		}
		case MESessionStarted:
		{
			t_success = OnSessionStart(t_result);
			break;
		}
		case MESessionStopped:
		{
			t_success = OnSessionStop(t_result);
			break;
		}
		case MESessionPaused:
		{
			t_success = OnSessionPause(t_result);
			break;
		}
		case MESessionEnded:
		{
			t_success = OnSessionEnded(t_result);
			break;
		}
		case MESessionRateChanged:
		{
			t_success = OnSessionRateChanged(t_result, p_event);
			break;
		}
		case MESessionClosed:
		{
			/* UNCHECKED */ m_session->Shutdown();
			m_shutdown = true;
			m_closing = false;
			Release();
			return true;
		}
		}
	}
	/* UNCHECKED - forward message to handler */
	if (m_event_handler != nil)
		m_event_handler->HandleMediaEvent(p_event);

	return t_success;
}

//////////

bool MCWin32MFSession::RequireTopologyChange()
{
	return m_request_topology != m_current_topology;
}

bool MCWin32MFSession::RequirePlayStateChange()
{
	if (m_request.play_state == m_current.play_state)
		return false;

	// don't go from stopped to paused
	if (m_current.play_state == kSessionPlayStateStop && m_request.play_state == kSessionPlayStatePause)
		return false;

	// other play state changes are OK
	return true;
}

bool MCWin32MFSession::RequirePlaybackRateChange()
{
	return m_current_topology != nil && m_request.play_rate != m_current.play_rate;
}

bool MCWin32MFSession::RequireSeek()
{
	return m_do_seek;
}

bool MCWin32MFSession::DoSetTopology(IMFTopology *p_topology)
{
	if (m_pending_op)
		return false;

	bool t_success = true;

	if (p_topology)
		t_success = SUCCEEDED(m_session->SetTopology(MFSESSION_SETTOPOLOGY_IMMEDIATE, p_topology));
	else
		t_success = SUCCEEDED(m_session->SetTopology(MFSESSION_SETTOPOLOGY_CLEAR_CURRENT, nil));

	if (t_success)
	{
		m_loading_topology = p_topology;
		m_pending_op = kSessionOpSetTopology;
	}

	return t_success;
}

bool MCWin32MFSession::UpdateTopology()
{
	if (m_pending_op)
		return true;

	if (!RequireTopologyChange())
		return true;

	return DoSetTopology(m_request_topology);
}

bool MCWin32MFSession::SetTopology(IMFTopology *p_topology)
{
	m_request_topology = p_topology;

	if (m_in_request)
		return true;

	return UpdateRequestState();
}

bool MCWin32MFSession::DoStart()
{
	if (m_pending_op)
		return false;

	if (!SUCCEEDED(MCWin32MFSessionStart(m_session, nil)))
		return false;

	m_pending_op = kSessionOpStart;
	return true;
}

bool MCWin32MFSession::DoPause()
{
	if (m_pending_op)
		return false;

	if (SUCCEEDED(m_session->Pause()))
		return false;

	m_pending_op = kSessionOpPause;
	return true;
}

bool MCWin32MFSession::DoStop()
{
	if (m_pending_op)
		return false;

	if (!SUCCEEDED(m_session->Stop()))
		return false;

	m_pending_op = kSessionOpStop;
	return true;
}

bool MCWin32MFSession::UpdatePlayState()
{
	if (m_pending_op)
		return true;

	if (!RequirePlayStateChange())
		return true;

	bool t_success = true;

	switch (m_request.play_state)
	{
	case kSessionPlayStateStart:
		t_success = DoStart();
		break;
	case kSessionPlayStatePause:
		t_success = DoPause();
		break;
	case kSessionPlayStateStop:
		t_success = DoStop();
		break;
	}

	return t_success;
}

bool MCWin32MFSession::StartAtPosition(MCPlatformPlayerDuration p_position)
{
	m_request.play_state = kSessionPlayStateStart;
	m_request.seek_position = p_position;
	m_do_seek = true;

	if (m_in_request)
		return true;

	return UpdateRequestState();
}

bool MCWin32MFSession::Start()
{
	m_request.play_state = kSessionPlayStateStart;

	if (m_in_request)
		return true;

	return UpdateRequestState();
}

bool MCWin32MFSession::Pause()
{
	m_request.play_state = kSessionPlayStatePause;

	if (m_in_request)
		return true;

	return UpdateRequestState();
}

bool MCWin32MFSession::Stop()
{
	m_request.play_state = kSessionPlayStateStop;

	if (m_in_request)
		return true;

	return UpdateRequestState();
}

bool MCWin32MFSession::GetCurrentPosition(MCPlatformPlayerDuration &r_position)
{
	if (RequireSeek() || m_pending_op == kSessionOpSeek)
	{
		r_position = m_request.seek_position;
		return true;
	}
	else if (RequireTopologyChange())
	{
		r_position = 0;
		return true;
	}
	else
	{
		MFTIME t_time;
		if (!SUCCEEDED(MCWin32MFSessionGetPresentationTime(m_session, t_time)))
			return false;

		r_position = t_time;
		return true;
	}
}

bool MCWin32MFSession::DoSetCurrentPosition(MCPlatformPlayerDuration p_position)
{
	if (m_pending_op)
		return false;

	m_do_seek = false;

	if (!SUCCEEDED(MCWin32MFSessionStart(m_session, &p_position)))
		return false;

	m_pending_op = kSessionOpSeek;
	return true;
}

bool MCWin32MFSession::UpdateSeekPosition()
{
	if (m_pending_op)
		return true;

	if (!m_do_seek)
		return true;

	return DoSetCurrentPosition(m_request.seek_position);
}

bool MCWin32MFSession::SetCurrentPosition(MCPlatformPlayerDuration p_position)
{
	m_do_seek = true;
	m_request.seek_position = p_position;

	if (m_in_request)
		return true;

	return UpdateRequestState();
}

bool MCWin32MFSession::DoSetPlaybackRate(float p_rate)
{
	bool t_success = true;

	bool t_thin;
	float t_rate;
	if (t_success)
		t_success = MCWin32MFClosestPlayRate(m_rate_support, p_rate, t_thin, t_rate);

	if (t_success)
		t_success = SUCCEEDED(m_rate_control->SetRate(t_thin, t_rate));

	if (t_success)
		m_current.play_rate = t_rate;

	return t_success;
}

bool MCWin32MFSession::UpdatePlaybackRate()
{
	if (m_pending_op)
		return true;

	if (!RequirePlaybackRateChange())
		return true;

	bool t_success = true;

	// required state for rate transitions:
	// +ve -> -ve : Stopped
	// -ve -> 0   : Stopped
	// +ve -> 0   : Paused or Stopped

	if (t_success)
	{
		if ((m_request.play_rate > 0 && m_current.play_rate < 0) || (m_request.play_rate < 0 && m_current.play_rate > 0))
		{
			// requires stopped

			if (m_current.play_state != kSessionPlayStateStop)
			{
				MFTIME t_current_time;
				if (t_success)
					t_success = SUCCEEDED(MCWin32MFSessionGetPresentationTime(m_session, t_current_time));
				if (t_success)
					t_success = DoStop();
				if (t_success)
				{
					if (!RequireSeek())
					{
						// restore current position if there is no other seek request
						m_do_seek = true;
						m_request.seek_position = t_current_time;
					}
				}

				return t_success;
			}
		}
		else if (m_request.play_rate == 0 && m_current.play_rate != 0)
		{
			// Requires paused or stopped
			if (m_current.play_state != kSessionPlayStatePause && m_current.play_state != kSessionPlayStateStop)
			{
				t_success = DoPause();

				return t_success;
			}
		}
	}

	if (t_success)
	{
		t_success = DoSetPlaybackRate(m_request.play_rate);

		if (t_success)
		{
			// update requested rate with supported rate
			m_request.play_rate = m_current.play_rate;
		}
		else
		{
			// rate unupported - clear request
			m_request.play_rate = m_current.play_rate;
		}
	}

	return t_success;
}

bool MCWin32MFSession::SetPlaybackRate(float p_rate)
{
	m_request.play_rate = p_rate;

	if (m_in_request)
		return true;

	return UpdateRequestState();
}

bool MCWin32MFSession::OnSessionTopologySet(HRESULT p_result)
{
	bool t_success = true;

	if (m_pending_op == kSessionOpSetTopology)
		m_pending_op = kSessionOpNone;

	t_success = S_OK == p_result;

	if (t_success)
	{
		m_current_topology = m_loading_topology;

		if (m_current_topology != nil)
		{
			CComPtr<IMFRateControl> t_rate_control;
			CComPtr<IMFRateSupport> t_rate_support;
			if (t_success)
				t_success = SUCCEEDED(MCWin32MFSessionGetService(m_session, MF_RATE_CONTROL_SERVICE, &t_rate_control));
			if (t_success)
				t_success = SUCCEEDED(MCWin32MFSessionGetService(m_session, MF_RATE_CONTROL_SERVICE, &t_rate_support));
			if (t_success)
			{
				m_rate_control = t_rate_control;
				m_rate_support = t_rate_support;
			}

			// preparing the loaded topology requires the session to be started
			if (m_current.play_state != kSessionPlayStateStart)
			{
				if (t_success && m_current.play_rate != 0.0)
					t_success = DoSetPlaybackRate(0.0);

				if (t_success)
				{
					if (RequireSeek())
						t_success = UpdateSeekPosition();
					else
						t_success = DoStart();
				}
			}
		}
	}

	return UpdateRequestState() && t_success;
}

bool MCWin32MFSession::OnSessionStart(HRESULT p_result)
{
	bool t_success = true;

	if (m_pending_op == kSessionOpStart || m_pending_op == kSessionOpSeek)
		m_pending_op = kSessionOpNone;

	t_success = S_OK == p_result;

	if (t_success)
		m_current.play_state = kSessionPlayStateStart;

	return UpdateRequestState() && t_success;
}

bool MCWin32MFSession::OnSessionStop(HRESULT p_result)
{
	bool t_success = true;

	if (m_pending_op == kSessionOpStop)
		m_pending_op = kSessionOpNone;

	t_success = S_OK == p_result;

	if (t_success)
		m_current.play_state = kSessionPlayStateStop;

	return UpdateRequestState() && t_success;
}

bool MCWin32MFSession::OnSessionPause(HRESULT p_result)
{
	bool t_success = true;

	if (m_pending_op == kSessionOpPause)
		m_pending_op = kSessionOpNone;

	t_success = S_OK == p_result;

	if (t_success)
		m_current.play_state = kSessionPlayStatePause;

	return UpdateRequestState() && t_success;
}

bool MCWin32MFSession::OnSessionEnded(HRESULT p_result)
{
	if (S_OK != p_result)
		return false;

	m_current.play_state = kSessionPlayStateStop;
	m_request.play_state = kSessionPlayStateStop;

	return true;
}

bool MCWin32MFSession::OnSessionRateChanged(HRESULT p_result, IMFMediaEvent *p_event)
{
	bool t_success = true;

	t_success = SUCCEEDED(p_result);

	PROPVARIANT t_value;
	PropVariantInit(&t_value);
	if (t_success)
		t_success = SUCCEEDED(p_event->GetValue(&t_value));
	if (t_success)
		t_success = t_value.vt == VT_R4;
	if (t_success)
		m_current.play_rate = t_value.fltVal;
	PropVariantClear(&t_value);

	return UpdateRequestState() && t_success;
}

bool MCWin32MFSession::RequestRequiresScrubbing()
{
	SessionPlayState t_target_play_state;
	t_target_play_state = m_request.play_state;

	if (RequireSeek() && t_target_play_state != kSessionPlayStateStart)
		return true;

	// transition from stop to pause requires scrubbing (setting rate to 0)
	if (m_request.play_state == kSessionPlayStatePause && m_current.play_state == kSessionPlayStateStop)
		return true;

	return false;
}

bool MCWin32MFSession::IsScrubbing()
{
	return m_current.play_rate == 0.0;
}

bool MCWin32MFSession::DoScrub()
{
	if (m_pending_op)
		return false;

	if (m_current.play_rate == 0.0)
		return DoSetCurrentPosition(m_request.seek_position);
	else if (m_current.play_state == kSessionPlayStateStop)
		return SUCCEEDED(m_rate_control->SetRate(FALSE, 0.0));
	else
		return DoStop();
}

bool MCWin32MFSession::UpdateRequestState()
{
	bool t_success = true;

	// if the session is closing then don't perform any more operations
	if (m_closing)
		return false;

	// first check for changes to the topology
	if (t_success && !m_pending_op && RequireTopologyChange())
		t_success = UpdateTopology();

	// if no topology currently set then we can't do anything yet
	if (m_current_topology == nil)
		return true;

	// if requested changes require scrubbing then do that first
	if (t_success && !m_pending_op && RequestRequiresScrubbing())
		t_success = DoScrub();

	// if able to scrub and seek is requested then do that now
	if (t_success && !m_pending_op && IsScrubbing() && RequireSeek())
		t_success = DoSetCurrentPosition(m_request.seek_position);

	// if able to scrub and play state requested then do that now
	if (t_success && !m_pending_op && IsScrubbing() && RequirePlayStateChange())
		t_success = UpdatePlayState();

	// check for rate changes
	if (t_success && !m_pending_op && RequirePlaybackRateChange())
		t_success = UpdatePlaybackRate();

	// check for seek requests
	if (t_success && !m_pending_op && RequireSeek())
		t_success = UpdateSeekPosition();

	// check for state changes
	if (t_success && !m_pending_op && RequirePlayStateChange())
		t_success = UpdatePlayState();

	return t_success;
}

IMFMediaSession *MCWin32MFSession::GetSession()
{
	return m_session;
}

////////////////////////////////////////////////////////////////////////////////

static bool s_mf_initialized = false;
MCWin32MFPlayer *MCWin32MFPlayerCreate()
{
	if (!s_mf_initialized)
	{
		s_mf_initialized = S_OK == MFStartup(MF_VERSION);
		if (!s_mf_initialized)
			return nil;
	}

	MCWin32MFPlayer *t_player;
	t_player = new (nothrow) MCWin32MFPlayer();

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
