/* TODO - insert copyright notice */

#include <Windows.h>
#include <atlbase.h>
#include <DShow.h>

#include "globals.h"
#include "stack.h"

#include "graphics_util.h"
#include "platform.h"
#include "platform-internal.h"

////////////////////////////////////////////////////////////////////////////////

extern HINSTANCE MChInst;

void MCWin32BSTRFree(BSTR p_str) { SysFreeString(p_str); }

#define kMCDSEventWindowClass "DSEVENTWINDOWCLASS"
#define kMCDSVideoWindowClass "DSVIDEOWINDOWCLASS"

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
	
	virtual bool IsPlaying(void);
	// PM-2014-05-28: [[ Bug 12523 ]] Take into account the playRate property
	virtual void Start(double rate);
	virtual void Stop(void);
	virtual void Step(int amount);
	
	virtual bool LockBitmap(uint32_t p_width, uint32_t p_height, MCImageBitmap*& r_bitmap);
	virtual void UnlockBitmap(MCImageBitmap *bitmap);
	
	virtual void SetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);
	virtual void GetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);
	
	virtual void CountTracks(uindex_t& r_count);
	virtual bool FindTrackWithId(uint32_t id, uindex_t& r_index);
	virtual void SetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
	virtual void GetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
	
	bool Initialize();
	bool HandleGraphEvent();
	bool SetVideoWindow(HWND hwnd);
	bool SetVideoWindowSize(uint32_t p_width, uint32_t p_height);

protected:
	virtual void Realize(void);
	virtual void Unrealize(void);

private:

	// Properties
	bool SetUrl(MCStringRef p_url);
	bool GetPlayRate(double &r_play_rate);
	bool SetPlayRate(double p_play_rate);
	bool GetCurrentPosition(uint32_t &r_position);
	bool SetCurrentPosition(uint32_t p_position);
	bool GetStartPosition(uint32_t &r_position);
	bool SetStartPosition(uint32_t p_position);
	bool GetFinishPosition(uint32_t &r_position);
	bool SetFinishPosition(uint32_t p_position);
	bool GetLoop(bool &r_loop);
	bool SetLoop(bool p_loop);
	bool GetDuration(uint32_t &r_duration);
	bool GetTimeScale(uint32_t &r_time_scale);
	bool GetVolume(uint16_t &r_volume);
	bool SetVolume(uint16_t p_volume);
	bool GetOffscreen(bool &r_offscreen);
	bool SetOffscreen(bool p_offscreen);
	bool GetMirrored(bool &r_mirrored);
	bool SetMirrored(bool p_mirrored);

	bool Play();
	bool Pause();
	bool SeekRelative(int32_t p_amount);

	bool SetEventWindow(HWND hwnd);
	bool SetFilterGraph(IGraphBuilder *p_graph);
	bool SetVisible(bool p_visible);
	bool GetFormattedSize(uint32_t &r_width, uint32_t &r_height);

	bool OpenFile(MCStringRef p_filename);
	void CloseFile();

	MCWin32DSPlayerState m_state;
	bool m_is_valid;
	HWND m_video_window;

	static HWND g_event_window;
    CComPtr<IGraphBuilder>  m_graph;
    CComPtr<IMediaEventEx>  m_event;
    CComPtr<IMediaControl>  m_control;
	CComPtr<IMediaSeeking>  m_seeking;
};

////////////////////////////////////////////////////////////////////////////////

#define WM_GRAPHNOTIFY (WM_APP + 1)   // Window message for graph events
long FAR PASCAL DSHiddenWindowProc (HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	switch(message)
	{
	//case WM_LBUTTONUP:
	//	DispatchMetaCardMessage("XMediaPlayer_LMouseUp","");
	//	break;
	//case WM_RBUTTONUP:
	//	DispatchMetaCardMessage("XMediaPlayer_RMouseUp","");
	//	break;
	//case WM_MOUSEMOVE:
	//	DispatchMetaCardMessage("XMediaPlayer_MouseMove","");
	//	break;
	case WM_GRAPHNOTIFY:
		MCWin32DSPlayer *whichplayer = (MCWin32DSPlayer *)lParam;
		whichplayer->HandleGraphEvent();
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
		wc.lpfnWndProc   = (WNDPROC)DSHiddenWindowProc;
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

long FAR PASCAL DSVideoWindowProc(HWND hWnd, UINT message, UINT wParam, LONG lParam)
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
			if (t_player != nil)
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

bool CreateEventWindow(HWND &r_window)
{
	if (!RegisterEventWindowClass())
		return false;

	HWND t_window;
	t_window = CreateWindow(kMCDSEventWindowClass, "EventWindow", 0, 0, 0, 2, 3, HWND_MESSAGE, nil, MChInst, 0);

	if (t_window == nil)
		return false;

	r_window = t_window;
	return true;
}

bool CreateVideoWindow(MCWin32DSPlayer *p_player, HWND &r_window)
{
	if (!RegisterVideoWindowClass())
		return false;

	HWND t_window;
	t_window = CreateWindow(kMCDSVideoWindowClass, "VideoWindow", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1, 1, (HWND)MCdefaultstackptr->getrealwindow(), nil, MChInst, p_player);

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
			MCPlatformCallbackSendPlayerFinished(this);
			break;
		}


        m_event->FreeEventParams(evCode, param1, param2);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

HWND MCWin32DSPlayer::g_event_window = nil;

MCWin32DSPlayer::MCWin32DSPlayer()
{
	m_video_window = nil;
	m_state = kMCWin32DSPlayerStopped;
	m_is_valid = false;
}

MCWin32DSPlayer::~MCWin32DSPlayer()
{
	CloseFile();
	if (m_video_window != nil)
		DestroyWindow(m_video_window);
}

bool MCWin32DSPlayer::Initialize()
{
	if (g_event_window == nil)
	{
		if (!CreateEventWindow(g_event_window))
			return false;
	}

	HWND t_video_window;
	if (!CreateVideoWindow(this, t_video_window))
		return false;

	m_video_window = t_video_window;
	return true;
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

	if (t_success)
	{
        if (m_graph)
        {           
			// clean up graph and related objects
            Stop();

            CComQIPtr<IVideoWindow> t_video(m_graph);
            if (t_video)
            {
                t_video->put_Visible(OAFALSE);
                t_video->put_Owner(NULL);
            }
        }

		m_graph = p_graph;
        m_event = t_event;
        m_control = t_control;
		m_seeking = t_seeking;
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
	if (pVideo)	
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

//open file
bool MCWin32DSPlayer::OpenFile(MCStringRef p_filename)
{
	bool t_success;
	t_success = true;

	CComPtr<IGraphBuilder> t_graph;

	if (t_success)
		t_success = S_OK == t_graph.CoCreateInstance(CLSID_FilterGraph);

	MCAutoStringRefAsCustom<BSTR, MCStringConvertToBSTR, MCWin32BSTRFree> t_filename;
	if (t_success)
		t_success = t_filename.Lock(p_filename);

	if (t_success)
		t_success = S_OK == t_graph->RenderFile(*t_filename, nil);

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

bool MCWin32DSPlayer::GetFormattedSize(uint32_t &r_width, uint32_t &r_height)
{
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

	if (t_success)
		t_success = SetVideoWindow(m_video_window);

	if (t_success)
		t_success = SetEventWindow(g_event_window);

	if (t_success)
		t_success = SetVisible(true);

	if (!t_success)
		CloseFile();

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

bool MCWin32DSPlayer::GetTimeScale(uint32_t &r_time_scale)
{
	if (m_seeking == nil)
		return false;

	// convert current time format unit to media reference time units (100 nanoseconds)
	LONGLONG t_media_time;
	if (S_OK != m_seeking->ConvertTimeFormat(&t_media_time, &TIME_FORMAT_MEDIA_TIME, 1, NULL))
		return false;

	r_time_scale = (uint32_t)(10000 / t_media_time); // convert to units/second
	return true;
}

bool MCWin32DSPlayer::GetDuration(uint32_t &r_duration)
{
	if (m_seeking == nil)
		return false;

	LONGLONG t_duration;
	if (S_OK != m_seeking->GetDuration(&t_duration))
		return false;

	r_duration = (uint32_t)t_duration;
	return true;
}

bool MCWin32DSPlayer::GetCurrentPosition(uint32_t &r_position)
{
	if (m_seeking == nil)
		return false;

	LONGLONG t_current;
	if (S_OK != m_seeking->GetCurrentPosition(&t_current))
		return false;

	r_position = t_current;

	return true;
}

bool MCWin32DSPlayer::SetCurrentPosition(uint32_t p_position)
{
	if (m_seeking == nil)
		return false;

	LONGLONG t_current = p_position;
	return S_OK == m_seeking->SetPositions(&t_current, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
}

// TODO - implement start position property
bool MCWin32DSPlayer::GetStartPosition(uint32_t &r_position)
{
	r_position = 0;
	return true;
}

bool MCWin32DSPlayer::SetStartPosition(uint32_t p_position)
{
	return false;
}

bool MCWin32DSPlayer::GetFinishPosition(uint32_t &r_position)
{
	if (m_seeking == nil)
		return false;

	LONGLONG t_finish;
	if (S_OK != m_seeking->GetStopPosition(&t_finish))
		return false;

	r_position = t_finish;
	return true;
}

bool MCWin32DSPlayer::SetFinishPosition(uint32_t p_position)
{
	if (m_seeking == nil)
		return false;

	LONGLONG t_finish = p_position;
	return S_OK == m_seeking->SetPositions(nil, AM_SEEKING_NoPositioning, &t_finish, AM_SEEKING_AbsolutePositioning);
}

bool MCWin32DSPlayer::GetVolume(uint16_t &r_volume)
{
	CComQIPtr<IBasicAudio> t_audio(m_graph);
	if (t_audio == nil)
		return false;

	LONG t_volume;
	if (S_OK != t_audio->get_Volume(&t_volume))
		return false;

	r_volume = t_volume / 100;
}

bool MCWin32DSPlayer::SetVolume(uint16_t p_volume)
{
	CComQIPtr<IBasicAudio> t_audio(m_graph);
	if (t_audio == nil)
		return false;

	return S_OK == t_audio->put_Volume(p_volume * 100);
}

// TODO - implement loop property
bool MCWin32DSPlayer::GetLoop(bool &r_loop)
{
	r_loop = false;
	return true;
}

bool MCWin32DSPlayer::SetLoop(bool p_loop)
{
	return false;
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
	r_mirrored = false;
	return true;
}

bool MCWin32DSPlayer::SetMirrored(bool p_mirrored)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

//play
bool MCWin32DSPlayer::Play()
{
	if (m_control == nil)
		return false;

	if (S_OK != m_control->Run())
		return false;

	m_state = kMCWin32DSPlayerRunning;

	return true;
}

//pause playback
bool MCWin32DSPlayer::Pause()
{
	if (m_control == nil)
		return false;

	if (S_OK != m_control->Pause())
		return false;

    m_state = kMCWin32DSPlayerPaused;

	return true;
}

bool MCWin32DSPlayer::SeekRelative(int32_t p_amount)
{
	if (m_seeking == nil)
		return false;

	LONGLONG t_amount = p_amount;
	return S_OK == m_seeking->SetPositions(&t_amount, AM_SEEKING_RelativePositioning, NULL, AM_SEEKING_NoPositioning);
}

////////////////////////////////////////////////////////////////////////////////

bool MCWin32DSPlayer::GetNativeView(void *&r_view)
{
	if (m_video_window == nil)
		return false;

	r_view = m_video_window;
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
			MCAssert(p_type == kMCPlatformPropertyTypeUInt32);
			uint32_t t_duration;
			if (GetDuration(t_duration))
				*((uint32_t*)r_value) = t_duration;
			break;
		}

	case kMCPlatformPlayerPropertyTimescale:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeUInt32);
			uint32_t t_time_scale;
			if (GetTimeScale(t_time_scale))
				*((uint32_t*)r_value) = t_time_scale;
			break;
		}

	case kMCPlatformPlayerPropertyCurrentTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeUInt32);
			uint32_t t_position;
			if (GetCurrentPosition(t_position))
				*((uint32_t*)r_value) = t_position;
			break;
		}

	case kMCPlatformPlayerPropertyStartTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeUInt32);
			uint32_t t_position;
			if (GetStartPosition(t_position))
				*((uint32_t*)r_value) = t_position;
			break;
		}

	case kMCPlatformPlayerPropertyFinishTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeUInt32);
			uint32_t t_position;
			if (GetFinishPosition(t_position))
				*((uint32_t*)r_value) = t_position;
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

	case kMCPlatformPlayerPropertyMediaTypes:
	case kMCPlatformPlayerPropertyURL:
		MCLog("UNIMPLEMENTED");
		break;

	// Can safely ignore these
	case kMCPlatformPlayerPropertyOnlyPlaySelection:
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
			MCAssert(p_type == kMCPlatformPropertyTypeUInt32);
			uint32_t t_position = *((uint32_t*)p_value);
			SetCurrentPosition(t_position);

			break;
		}

	case kMCPlatformPlayerPropertyStartTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeUInt32);
			uint32_t t_start = *((uint32_t*)p_value);
			SetStartPosition(t_start);

			break;
		}

	case kMCPlatformPlayerPropertyFinishTime:
		{
			MCAssert(p_type == kMCPlatformPropertyTypeUInt32);
			uint32_t t_finish = *((uint32_t*)p_value);
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

	case kMCPlatformPlayerPropertyMediaTypes:
	
	case kMCPlatformPlayerPropertyMarkers:
	case kMCPlatformPlayerPropertyLoadedTime:
    
	case kMCPlatformPlayerPropertyScalefactor:
		MCLog("UNIMPLEMENTED");
		break;

	// Can safely ignore these
	case kMCPlatformPlayerPropertyShowSelection:
	case kMCPlatformPlayerPropertyOnlyPlaySelection:
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCWin32DSPlayer::LockBitmap(uint32_t p_width, uint32_t p_height, MCImageBitmap*& r_bitmap)
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
	t_player = new MCWin32DSPlayer();

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
