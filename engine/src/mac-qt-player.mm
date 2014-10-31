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

#include <Cocoa/Cocoa.h>
#include <QTKit/QTKit.h>

#include "globdefs.h"
#include "imagebitmap.h"
#include "region.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

#include "mac-player.h"
#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

 class MCQTKitPlayer;
 
 @interface com_runrev_livecode_MCQTKitPlayerObserver: NSObject
 {
     MCQTKitPlayer *m_player;
 }
 
 - (id)initWithPlayer: (MCQTKitPlayer *)player;
 
 - (void)movieFinished: (id)object;
 - (void)currentTimeChanged: (id)object;
 - (void)rateChanged: (id)object;
 - (void)selectionChanged: (id)object;
 
 @end

@interface QTMovie(QtExtensions)
- (NSArray*)loadedRanges;
- (QTTime)maxTimeLoaded;
@end
 
class MCQTKitPlayer: public MCPlatformPlayer
{
public:
    MCQTKitPlayer(void);
    virtual ~MCQTKitPlayer(void);
    
    virtual bool IsPlaying(void);
    // PM-2014-05-28: [[ Bug 12523 ]] Take into account the playRate property
    virtual void Start(double rate);
    virtual void Stop(void);
    virtual void Step(int amount);
    
    virtual void LockBitmap(MCImageBitmap*& r_bitmap);
    virtual void UnlockBitmap(MCImageBitmap *bitmap);
    
    virtual void SetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);
    virtual void GetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);
    
    virtual void CountTracks(uindex_t& r_count);
    virtual bool FindTrackWithId(uint32_t id, uindex_t& r_index);
    virtual void SetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
    virtual void GetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
    
    void MovieFinished(void);
    void CurrentTimeChanged(void);
    void MovieIsLoading(QTTimeRange p_timerange);
    
protected:
    virtual void Realize(void);
    virtual void Unrealize(void);
    
private:
    void Load(MCStringRef filename, bool is_url);
    void Synchronize(void);
    void Switch(bool new_offscreen);
    
    void CacheCurrentFrame(void);
    
    static void DoSwitch(void *context);
    static OSErr MovieDrawingComplete(Movie movie, long ref);
    static Boolean MovieActionFilter(MovieController mc, short action, void *params, long refcon);
    
    QTMovie *m_movie;
    QTMovieView *m_view;
    CVImageBufferRef m_current_frame;
    QTTime m_last_current_time;
    QTTime m_buffered_time;
    
    com_runrev_livecode_MCQTKitPlayerObserver *m_observer;
    
    uint32_t *m_markers;
    uindex_t m_marker_count;
    uint32_t m_last_marker;
    
    MCRectangle m_rect;
    bool m_visible : 1;
    bool m_offscreen : 1;
    bool m_show_controller : 1;
    bool m_show_selection : 1;
    bool m_pending_offscreen : 1;
    bool m_switch_scheduled : 1;
    bool m_playing : 1;
    bool m_synchronizing : 1;
};

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCQTKitPlayerObserver

- (id)initWithPlayer: (MCQTKitPlayer *)player
{
    self = [super init];
    if (self == nil)
        return nil;
    
    m_player = player;
    
    return self;
}

- (void)movieFinished: (id)object
{
    m_player -> MovieFinished();
}

- (void)currentTimeChanged: (id)object
{
    m_player -> CurrentTimeChanged();
}

@end
 
////////////////////////////////////////////////////////////////////////////////


inline QTTime do_QTMakeTime(long long timeValue, long timeScale)
{
    typedef QTTime (*QTMakeTimePtr)(long long timeValue, long timescale);
    extern QTMakeTimePtr QTMakeTime_ptr;
    return QTMakeTime_ptr(timeValue, timeScale);
}

inline NSComparisonResult do_QTTimeCompare (QTTime time, QTTime otherTime)
{
    typedef NSComparisonResult (*QTTimeComparePtr)(QTTime time, QTTime otherTime);
    extern QTTimeComparePtr QTTimeCompare_ptr;
    return QTTimeCompare_ptr(time, otherTime);
}

MCQTKitPlayer::MCQTKitPlayer(void)
{
	m_movie = [[NSClassFromString(@"QTMovie") movie] retain];
	m_view = [[NSClassFromString(@"QTMovieView") alloc] initWithFrame: NSZeroRect];
	m_observer = [[com_runrev_livecode_MCQTKitPlayerObserver alloc] initWithPlayer: this];
    
	m_current_frame = nil;
	
    m_markers = nil;
    m_marker_count = 0;
    
	m_rect = MCRectangleMake(0, 0, 0, 0);
	m_visible = true;
	m_offscreen = false;
	m_pending_offscreen = false;
	
	m_switch_scheduled = false;
    
    m_playing = false;
    m_show_controller = false;
	m_show_selection = false;
    m_synchronizing = false;
    
    m_last_current_time = do_QTMakeTime(INT64_MAX, 1);
    m_buffered_time = do_QTMakeTime(0, 1);
}

MCQTKitPlayer::~MCQTKitPlayer(void)
{
	if (m_current_frame != nil)
		CFRelease(m_current_frame);
	
    // MW-2014-07-16: [[ Bug 12506 ]] Make sure we unhook the callbacks before releasing (it
    //   seems it takes a while for QTKit to actually release the objects!).
    MCSetActionFilterWithRefCon([m_movie quickTimeMovieController], nil, nil);
    SetMovieDrawingCompleteProc([m_movie quickTimeMovie], movieDrawingCallAlways, nil, nil);
    
    [[NSNotificationCenter defaultCenter] removeObserver: m_observer];
    [m_observer release];
	[m_view release];
	[m_movie release];
    
    MCMemoryDeleteArray(m_markers);
}

void MCQTKitPlayer::MovieIsLoading(QTTimeRange p_timerange)
{
    QTTime t_buffered_time;
    t_buffered_time = p_timerange.duration;
    m_buffered_time = t_buffered_time;
    MCPlatformCallbackSendPlayerBufferUpdated(this);
    /*
    float t_movie_duration, t_loaded_part;
    t_movie_duration = (float)m_movie.duration.timeValue/(float)m_movie.duration.timeScale;
    t_loaded_part = (float)t_buffered_time.timeValue/(float)t_buffered_time.timeScale;
    MCLog("=============Loaded %.2f / 1.00", (float)t_loaded_part/(float)t_movie_duration);
    */
    
}

void MCQTKitPlayer::MovieFinished(void)
{
    m_playing = false;
    MCPlatformCallbackSendPlayerFinished(this);
}

void MCQTKitPlayer::CurrentTimeChanged(void)
{
    if (!m_synchronizing)
        MCPlatformCallbackSendPlayerCurrentTimeChanged(this);
}

void MCQTKitPlayer::CacheCurrentFrame(void)
{
    QTVisualContextRef t_context;
	t_context = nil;
	GetMovieVisualContext([m_movie quickTimeMovie], &t_context);
	
	CVImageBufferRef t_image;
	t_image = nil;
	if (t_context != nil)
		QTVisualContextCopyImageForTime(t_context, kCFAllocatorDefault, NULL, &t_image);
	
	if (t_image != nil)
	{
		if (m_current_frame != nil)
			CFRelease(m_current_frame);
		m_current_frame = t_image;
	}
}

OSErr MCQTKitPlayer::MovieDrawingComplete(Movie p_movie, long p_ref)
{
	MCQTKitPlayer *t_self;
	t_self = (MCQTKitPlayer *)p_ref;
    
    t_self -> CacheCurrentFrame();
    MCPlatformCallbackSendPlayerFrameChanged(t_self);
    
    // PM-2104-08-28: [[ Bug 12830 ]] Make sure frames are updated only when movie is playing
    if (t_self -> IsPlaying())
        t_self -> CurrentTimeChanged();
	
	return noErr;
}

void MCQTKitPlayer::Switch(bool p_new_offscreen)
{
	// If the new setting is the same as the pending setting, do nothing.
	if (p_new_offscreen == m_pending_offscreen)
		return;
	
	// Update the pending offscreen setting and schedule a switch.
	m_pending_offscreen = p_new_offscreen;

	if (m_switch_scheduled)
		return;
	
	Retain();
	MCMacPlatformScheduleCallback(DoSwitch, this);
	
	m_switch_scheduled = true;
}

void MCQTKitPlayer::DoSwitch(void *ctxt)
{
	MCQTKitPlayer *t_player;
	t_player = (MCQTKitPlayer *)ctxt;
    
	t_player -> m_switch_scheduled = false;
	
	if (t_player -> m_pending_offscreen == t_player -> m_offscreen)
	{
		// Do nothing if there is no state change.
	}
	else if (t_player -> m_pending_offscreen)
	{
        t_player -> CacheCurrentFrame();
        
		if (t_player -> m_view != nil)
			t_player -> Unrealize();

        // PM-2104-08-28: [[ Bug 12830 ]] The movieDrawingCallAlways flag indicates that we want QuickTime to call MovieDrawingComplete every time the movie is tasked (that is, every time our application calls MoviesTask, either directly or indirectly).
		SetMovieDrawingCompleteProc([t_player -> m_movie quickTimeMovie], movieDrawingCallAlways, MCQTKitPlayer::MovieDrawingComplete, (long int)t_player);
        
		t_player -> m_offscreen = t_player -> m_pending_offscreen;
	}
	else
	{
		if (t_player -> m_current_frame != nil)
		{
			CFRelease(t_player -> m_current_frame);
			t_player -> m_current_frame = nil;
		}

		SetMovieDrawingCompleteProc([t_player -> m_movie quickTimeMovie], movieDrawingCallAlways, nil, nil);
        
		// Switching to non-offscreen
		t_player -> m_offscreen = t_player -> m_pending_offscreen;
		t_player -> Realize();
	}
	
	t_player -> Release();
}

void MCQTKitPlayer::Realize(void)
{
	if (m_window == nil)
		return;
	
	MCMacPlatformWindow *t_window;
	t_window = (MCMacPlatformWindow *)m_window;
	
	if (!m_offscreen)
	{
		MCWindowView *t_parent_view;
		t_parent_view = t_window -> GetView();
		[t_parent_view addSubview: m_view];
	}
	
	Synchronize();
}

void MCQTKitPlayer::Unrealize(void)
{
	if (m_offscreen || m_window == nil)
		return;
    
	if (!m_offscreen)
	{
		MCMacPlatformWindow *t_window;
		t_window = (MCMacPlatformWindow *)m_window;
        
		MCWindowView *t_parent_view;
		t_parent_view = t_window -> GetView();
        
		[m_view removeFromSuperview];
	}
}

Boolean MCQTKitPlayer::MovieActionFilter(MovieController mc, short action, void *params, long refcon)
{
    switch(action)
    {
        case mcActionIdle:
        case mcActionGoToTime:
        {
            MCQTKitPlayer *self;
            self = (MCQTKitPlayer *)refcon;
            
            QTTime t_current_time;
            t_current_time = [self -> m_movie currentTime];
            
            // PM-2014-10-28: [[ Bug 13773 ]] If the thumb is after the first marker and the user drags it before the first marker, then we have to reset m_last marker, so as to be dispatched
            if (t_current_time . timeValue < self -> m_last_marker)
                self -> m_last_marker = -1;
            
            if (do_QTTimeCompare(t_current_time, self -> m_last_current_time) != 0)
            {
                self -> m_last_current_time = t_current_time;
                
                if (self -> m_marker_count > 0)
                {
                    // We search for the marker time immediately before the
                    // current time and if last marker is not that time,
                    // dispatch it.
                    uindex_t t_index;
                    for(t_index = 0; t_index < self -> m_marker_count; t_index++)
                        if (self -> m_markers[t_index] > t_current_time . timeValue)
                            break;
                    
                    // t_index is now the first marker greater than the current time.
                    if (t_index > 0)
                    {
                        if (self -> m_markers[t_index - 1] != self -> m_last_marker)
                        {
                            self -> m_last_marker = self -> m_markers[t_index - 1];
                            MCPlatformCallbackSendPlayerMarkerChanged(self, self -> m_last_marker);
                            self -> m_synchronizing = true;
                        }
                    }
                }
                
                // PM-2014-10-28: [[ Bug 13773 ]] Make sure we don't send a currenttimechanged messsage if the callback is processed
                if (!self -> m_offscreen && !self -> m_synchronizing && self -> IsPlaying())
                    self -> CurrentTimeChanged();
                
                self -> m_synchronizing = false;
            }
        }
        break;
            
        default:
            break;
    }
    
    return False;
}

void MCQTKitPlayer::Load(MCStringRef p_filename, bool p_is_url)
{
    NSError *t_error;
	t_error = nil;
    
    MCStringRef t_filename;
    if (p_filename == nil)
        t_filename = kMCEmptyString;
    else
        t_filename = p_filename;
    
    id t_filename_or_url;
    if (!p_is_url)
        t_filename_or_url = [NSString stringWithMCStringRef: t_filename];
    else
        t_filename_or_url = [NSURL URLWithString: [NSString stringWithMCStringRef: t_filename]];
    
	NSDictionary *t_attrs;
    extern NSString **QTMovieFileNameAttribute_ptr;
    extern NSString **QTMovieOpenAsyncOKAttribute_ptr;
    extern NSString **QTMovieOpenAsyncRequiredAttribute_ptr;
    extern NSString **QTMovieURLAttribute_ptr;
    
	t_attrs = [NSDictionary dictionaryWithObjectsAndKeys:
			   t_filename_or_url, p_is_url ? *QTMovieURLAttribute_ptr : *QTMovieFileNameAttribute_ptr,
			   /* [NSNumber numberWithBool: YES], QTMovieOpenForPlaybackAttribute, */
			   [NSNumber numberWithBool: NO], *QTMovieOpenAsyncOKAttribute_ptr,
			   [NSNumber numberWithBool: NO], *QTMovieOpenAsyncRequiredAttribute_ptr,
			   nil];
	
	QTMovie *t_new_movie;
	t_new_movie = [[NSClassFromString(@"QTMovie") alloc] initWithAttributes: t_attrs
                                                                      error: &t_error];
	
	if (t_error != nil)
	{
		[t_new_movie release];
		return;
	}
	
    // MW-2014-07-18: [[ Bug ]] Clean up callbacks before we release.
    MCSetActionFilterWithRefCon([m_movie quickTimeMovieController], nil, nil);
    SetMovieDrawingCompleteProc([m_movie quickTimeMovie], movieDrawingCallAlways, nil, nil);
	[m_movie release];
    
    // PM-2014-09-02: [[ Bug 13306 ]] Make sure we reset the previous value of loadedtime when loading a new movie
    m_buffered_time = do_QTMakeTime(0, 1);
    
	m_movie = t_new_movie;
    
    [[NSNotificationCenter defaultCenter] removeObserver: m_observer];
    
    extern NSString **QTMovieDidEndNotification_ptr;
    [[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(movieFinished:) name: *QTMovieDidEndNotification_ptr object: m_movie];
    
    extern NSString **QTMovieTimeDidChangeNotification_ptr;
    [[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(currentTimeChanged:) name: *QTMovieTimeDidChangeNotification_ptr object: m_movie];
    
	// This method seems to be there - but isn't 'public'. Given QTKit is now deprecated as long
	// as it works on the platforms we support, it should be fine.
	[m_movie setDraggable: NO];
	
    [m_view setControllerVisible: NO];
    
	[m_view setMovie: m_movie];
    
    // MW-2014-07-16: [[ Bug 12836 ]] Make sure we give movies some time to collect the first
    //   frame.
    MoviesTask([m_movie quickTimeMovie], 0);
    
    // Set the last marker to very large so that any marker will trigger.
    m_last_marker = UINT32_MAX;
    
    MCSetActionFilterWithRefCon([m_movie quickTimeMovieController], MovieActionFilter, (long)this);
    
    // MW-2014-07-18: [[ Bug 12837 ]] Make sure we add a moviedrawingcomplete callback to the object
    //   if we are already offscreen.
    if (m_offscreen)
    {
		SetMovieDrawingCompleteProc([m_movie quickTimeMovie], movieDrawingCallAlways, MCQTKitPlayer::MovieDrawingComplete, (long int)this);
        CacheCurrentFrame();
    }
    if (p_is_url && [m_movie respondsToSelector:@selector(loadedRanges)])
    {
        NSArray *t_load_ranges = [m_movie loadedRanges];
        if (t_load_ranges)
        {
            QTTimeRange t_time_range;
            t_time_range = [[t_load_ranges objectAtIndex: 0] QTTimeRangeValue];
            MovieIsLoading(t_time_range);
        }
    }
}

void MCQTKitPlayer::Synchronize(void)
{
	if (m_window == nil)
		return;
	
	MCMacPlatformWindow *t_window;
	t_window = (MCMacPlatformWindow *)m_window;
	
	NSRect t_frame;
	t_window -> MapMCRectangleToNSRect(m_rect, t_frame);
    
    m_synchronizing = true;
    
	[m_view setFrame: t_frame];
	
	[m_view setHidden: !m_visible];
    
    [m_view setEditable: m_show_selection];
	[m_view setControllerVisible: m_show_controller];
	
	MCMovieChanged([m_movie quickTimeMovieController], [m_movie quickTimeMovie]);
    
    m_synchronizing = false;
}

bool MCQTKitPlayer::IsPlaying(void)
{
	return [m_movie rate] != 0;
}

// PM-2014-05-28: [[ Bug 12523 ]] Take into account the playRate property
void MCQTKitPlayer::Start(double rate)
{
	[m_movie setRate: rate];
}

void MCQTKitPlayer::Stop(void)
{
	[m_movie setRate: 0.0];
}

void MCQTKitPlayer::Step(int amount)
{
	if (amount > 0)
		[m_movie stepForward];
	else if (amount < 0)
		[m_movie stepBackward];
}

void MCQTKitPlayer::LockBitmap(MCImageBitmap*& r_bitmap)
{
	MCImageBitmap *t_bitmap;
	t_bitmap = new MCImageBitmap;
	t_bitmap -> width = m_rect . width;
	t_bitmap -> height = m_rect . height;
	t_bitmap -> stride = m_rect . width * sizeof(uint32_t);
	t_bitmap -> data = (uint32_t *)malloc(t_bitmap -> stride * t_bitmap -> height);
    memset(t_bitmap -> data, 0,t_bitmap -> stride * t_bitmap -> height);
	t_bitmap -> has_alpha = t_bitmap -> has_transparency = true;
    
	
	// Now if we have a current frame, then composite at the appropriate size into
	// the movie portion of the buffer.
	if (m_current_frame != nil)
	{
		extern CGBitmapInfo MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha);
		
		CGColorSpaceRef t_colorspace;
		/* UNCHECKED */ MCMacPlatformGetImageColorSpace(t_colorspace);
		
		CGContextRef t_cg_context;
		t_cg_context = CGBitmapContextCreate(t_bitmap -> data, t_bitmap -> width, t_bitmap -> height, 8, t_bitmap -> stride, t_colorspace, MCGPixelFormatToCGBitmapInfo(kMCGPixelFormatNative, true));
		
		CIImage *t_ci_image;
		t_ci_image = [[CIImage alloc] initWithCVImageBuffer: m_current_frame];
        
        NSAutoreleasePool *t_pool;
        t_pool = [[NSAutoreleasePool alloc] init];
        
		CIContext *t_ci_context;
		t_ci_context = [CIContext contextWithCGContext: t_cg_context options: nil];
		
		[t_ci_context drawImage: t_ci_image inRect: CGRectMake(0, 0, m_rect . width, m_rect . height) fromRect: [t_ci_image extent]];
		
        [t_pool release];
        
		[t_ci_image release];
		
		CGContextRelease(t_cg_context);
		CGColorSpaceRelease(t_colorspace);
	}
	
	r_bitmap = t_bitmap;
}

void MCQTKitPlayer::UnlockBitmap(MCImageBitmap *bitmap)
{
    delete bitmap -> data;
	delete bitmap;
}

extern NSString **QTMovieLoopsAttribute_ptr;
extern NSString **QTMoviePlaysSelectionOnlyAttribute_ptr;

void MCQTKitPlayer::SetProperty(MCPlatformPlayerProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
    m_synchronizing = true;
    
	switch(p_property)
	{
		case kMCPlatformPlayerPropertyURL:
			Load(*(MCStringRef*)p_value, true);
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyFilename:
			Load(*(MCStringRef*)p_value, false);
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyOffscreen:
			Switch(*(bool *)p_value);
			break;
		case kMCPlatformPlayerPropertyRect:
			m_rect = *(MCRectangle *)p_value;
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyVisible:
			m_visible = *(bool *)p_value;
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyCurrentTime:
			[m_movie setCurrentTime: do_QTMakeTime(*(uint32_t *)p_value, [m_movie duration] . timeScale)];
			break;
		case kMCPlatformPlayerPropertyStartTime:
		{
			QTTime t_selection_start, t_selection_end;
			t_selection_start = [m_movie selectionStart];
			t_selection_end = [m_movie selectionEnd];
			
			uint32_t t_start_time, t_end_time;
			t_start_time = *(uint32_t *)p_value;
			t_end_time = t_selection_end . timeValue;
			
			if (t_start_time > t_end_time)
				t_end_time = t_start_time;
			
			QTTimeRange t_selection;
			t_selection . time . timeValue = t_start_time;
			t_selection . time . timeScale = t_selection_start . timeScale;
			t_selection . duration . timeValue = t_end_time - t_start_time;
			t_selection . duration . timeScale = t_selection_start . timeScale;
			[m_movie setSelection: t_selection];
		}
            break;
		case kMCPlatformPlayerPropertyFinishTime:
		{
			QTTime t_selection_start, t_selection_end;
			t_selection_start = [m_movie selectionStart];
			t_selection_end = [m_movie selectionEnd];
			
			uint32_t t_start_time, t_end_time;
			t_start_time = t_selection_start . timeValue;
			t_end_time = *(uint32_t *)p_value;
			
			if (t_start_time > t_end_time)
				t_start_time = t_end_time;
			
			QTTimeRange t_selection;
			t_selection . time . timeValue = t_start_time;
			t_selection . time . timeScale = t_selection_start . timeScale;
			t_selection . duration . timeValue = t_end_time - t_start_time;
			t_selection . duration . timeScale = t_selection_start . timeScale;
			[m_movie setSelection: t_selection];
		}
            break;
		case kMCPlatformPlayerPropertyPlayRate:
			[m_movie setRate: *(double *)p_value];
			break;
		case kMCPlatformPlayerPropertyVolume:
			[m_movie setVolume: *(uint16_t *)p_value / 100.0f];
			break;
		case kMCPlatformPlayerPropertyShowSelection:
			m_show_selection = *(bool *)p_value;
			Synchronize();
			break;
        case kMCPlatformPlayerPropertyOnlyPlaySelection:
			[m_movie setAttribute: [NSNumber numberWithBool: *(bool *)p_value] forKey: *QTMoviePlaysSelectionOnlyAttribute_ptr];
			break;
		case kMCPlatformPlayerPropertyLoop:
			[m_movie setAttribute: [NSNumber numberWithBool: *(bool *)p_value] forKey: *QTMovieLoopsAttribute_ptr];
			break;
        case kMCPlatformPlayerPropertyMarkers:
        {
            array_t<uint32_t> *t_markers;
            t_markers = (array_t<uint32_t> *)p_value;
            
            m_last_marker = UINT32_MAX;
            MCMemoryDeleteArray(m_markers);
            m_markers = nil;
            
            /* UNCHECKED */ MCMemoryResizeArray(t_markers -> count, m_markers, m_marker_count);
            MCMemoryCopy(m_markers, t_markers -> ptr, m_marker_count * sizeof(uint32_t));
        }
            break;
	}
    
    m_synchronizing = false;
}

static Boolean IsQTVRMovie(Movie theMovie)
{
	Boolean IsQTVR = False;
	OSType evaltype,targettype =  kQTVRUnknownType;
	UserData myUserData;
	if (theMovie == NULL)
		return False;
	myUserData = GetMovieUserData(theMovie);
	if (myUserData != NULL)
	{
		GetUserDataItem(myUserData, &targettype, sizeof(targettype),
		                kUserDataMovieControllerType, 0);
		evaltype = EndianU32_BtoN(targettype);
		if (evaltype == kQTVRQTVRType || evaltype == kQTVROldPanoType
			|| evaltype == kQTVROldObjectType)
			IsQTVR = true;
	}
	return(IsQTVR);
}

static Boolean QTMovieHasType(Movie tmovie, OSType movtype)
{
	switch (movtype)
	{
		case VisualMediaCharacteristic:
		case AudioMediaCharacteristic:
			return (GetMovieIndTrackType(tmovie, 1, movtype,
										 movieTrackCharacteristic) != NULL);
		case kQTVRQTVRType:
			return IsQTVRMovie(tmovie);
		default:
			return (GetMovieIndTrackType(tmovie, 1, movtype,
										 movieTrackMediaType) != NULL);
	}
}

void MCQTKitPlayer::GetProperty(MCPlatformPlayerProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	switch(p_property)
	{
		case kMCPlatformPlayerPropertyOffscreen:
			*(bool *)r_value = m_offscreen;
			break;
		case kMCPlatformPlayerPropertyRect:
			*(MCRectangle *)r_value = m_rect;
			break;
		case kMCPlatformPlayerPropertyMovieRect:
		{
			NSValue *t_value;
            extern NSString **QTMovieNaturalSizeAttribute_ptr;
			t_value = [m_movie attributeForKey: *QTMovieNaturalSizeAttribute_ptr];
			*(MCRectangle *)r_value = MCRectangleMake(0, 0, [t_value sizeValue] . width, [t_value sizeValue] . height);
		}
            break;
		case kMCPlatformPlayerPropertyVisible:
			*(bool *)r_value = m_visible;
			break;
		case kMCPlatformPlayerPropertyMediaTypes:
		{
			MCPlatformPlayerMediaTypes t_types;
			t_types = 0;
			if (QTMovieHasType([m_movie quickTimeMovie], VisualMediaCharacteristic))
				t_types |= kMCPlatformPlayerMediaTypeVideo;
			if (QTMovieHasType([m_movie quickTimeMovie], AudioMediaCharacteristic))
				t_types |= kMCPlatformPlayerMediaTypeAudio;
			if (QTMovieHasType([m_movie quickTimeMovie], TextMediaType))
				t_types |= kMCPlatformPlayerMediaTypeText;
			if (QTMovieHasType([m_movie quickTimeMovie], kQTVRQTVRType))
				t_types |= kMCPlatformPlayerMediaTypeQTVR;
			if (QTMovieHasType([m_movie quickTimeMovie], SpriteMediaType))
				t_types |= kMCPlatformPlayerMediaTypeSprite;
			if (QTMovieHasType([m_movie quickTimeMovie], FlashMediaType))
				t_types |= kMCPlatformPlayerMediaTypeFlash;
			*(MCPlatformPlayerMediaTypes *)r_value = t_types;
		}
            break;
        // PM-2014-08-20 [[ Bug 13121 ]] Added property for displaying download progress
        case kMCPlatformPlayerPropertyLoadedTime:
			*(uint32_t *)r_value = m_buffered_time . timeValue;
			break;
		case kMCPlatformPlayerPropertyDuration:
			*(uint32_t *)r_value = [m_movie duration] . timeValue;
			break;
		case kMCPlatformPlayerPropertyTimescale:
			*(uint32_t *)r_value = [m_movie currentTime] . timeScale;
			break;
		case kMCPlatformPlayerPropertyCurrentTime:
			*(uint32_t *)r_value = [m_movie currentTime] . timeValue;
			break;
		case kMCPlatformPlayerPropertyStartTime:
			*(uint32_t *)r_value = [m_movie selectionStart] . timeValue;
			break;
		case kMCPlatformPlayerPropertyFinishTime:
			*(uint32_t *)r_value = [m_movie selectionEnd] . timeValue;
			break;
		case kMCPlatformPlayerPropertyPlayRate:
			*(double *)r_value = [m_movie rate];
			break;
		case kMCPlatformPlayerPropertyVolume:
			*(uint16_t *)r_value = [m_movie volume] * 100.0f;
			break;
            
		case kMCPlatformPlayerPropertyShowSelection:
			*(bool *)r_value = m_show_selection;
			break;
		case kMCPlatformPlayerPropertyOnlyPlaySelection:
			*(bool *)r_value = [(NSNumber *)[m_movie attributeForKey: *QTMoviePlaysSelectionOnlyAttribute_ptr] boolValue] == YES;
			break;
		case kMCPlatformPlayerPropertyLoop:
			*(bool *)r_value = [(NSNumber *)[m_movie attributeForKey: *QTMovieLoopsAttribute_ptr] boolValue] == YES;
			break;
	}
}

void MCQTKitPlayer::CountTracks(uindex_t& r_count)
{
	r_count = GetMovieTrackCount([m_movie quickTimeMovie]);
}

bool MCQTKitPlayer::FindTrackWithId(uint32_t p_id, uindex_t& r_index)
{
	Movie t_movie;
	t_movie = [m_movie quickTimeMovie];
	for(uindex_t i = 1; i <= GetMovieTrackCount(t_movie); i++)
		if (GetTrackID(GetMovieIndTrack(t_movie, i)) == p_id)
		{
			r_index = i - 1;
			return true;
		}
	return false;
}

void MCQTKitPlayer::SetTrackProperty(uindex_t p_index, MCPlatformPlayerTrackProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
	if (p_property != kMCPlatformPlayerTrackPropertyEnabled)
		return;
	
	Movie t_movie;
	t_movie = [m_movie quickTimeMovie];
	
	Track t_track;
	t_track = GetMovieIndTrack(t_movie, p_index + 1);
	
	SetTrackEnabled(t_track, *(bool *)p_value);
}

void MCQTKitPlayer::GetTrackProperty(uindex_t p_index, MCPlatformPlayerTrackProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	Movie t_movie;
	t_movie = [m_movie quickTimeMovie];
	
	Track t_track;
	t_track = GetMovieIndTrack(t_movie, p_index + 1);
	
	switch(p_property)
	{
		case kMCPlatformPlayerTrackPropertyId:
			*(uint32_t *)r_value = GetTrackID(t_track);
			break;
		case kMCPlatformPlayerTrackPropertyMediaTypeName:
		{
			Media t_media;
			t_media = GetTrackMedia(t_track);
			MediaHandler t_handler;
			t_handler = GetMediaHandler(t_media);
			
			unsigned char t_name[256];
			MediaGetName(t_handler, t_name, 0, nil);
			p2cstr(t_name);
            MCStringCreateWithCString((char*)t_name, *(MCStringRef*)r_value);
		}
            break;
		case kMCPlatformPlayerTrackPropertyOffset:
			*(uint32_t *)r_value = GetTrackOffset(t_track);
			break;
		case kMCPlatformPlayerTrackPropertyDuration:
			*(uint32_t *)r_value = GetTrackDuration(t_track);
			break;
		case kMCPlatformPlayerTrackPropertyEnabled:
			*(bool *)r_value = GetTrackEnabled(t_track);
			break;
	}
}

////////////////////////////////////////////////////////

MCQTKitPlayer *MCQTKitPlayerCreate(void)
{
    return new MCQTKitPlayer;
}

////////////////////////////////////////////////////////
