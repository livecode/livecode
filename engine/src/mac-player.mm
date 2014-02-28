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

#include "core.h"
#include "globdefs.h"
#include "imagebitmap.h"
#include "region.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

class MCPlatformPlayer
{
public:
	MCPlatformPlayer(void);
	virtual ~MCPlatformPlayer(void);
	
	void Retain(void);
	void Release(void);
	
	void Attach(MCPlatformWindowRef window);
	void Detach(void);
	
	virtual bool IsPlaying(void) = 0;
	virtual void Start(void) = 0;
	virtual void Stop(void) = 0;
	virtual void Step(int amount) = 0;
	
	virtual void LockBitmap(MCImageBitmap*& r_bitmap) = 0;
	virtual void UnlockBitmap(MCImageBitmap *bitmap) = 0;
	
	virtual void SetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value) = 0;
	virtual void GetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value) = 0;
	
	virtual void CountTracks(uindex_t& r_count) = 0;
	virtual void FindTrackWithId(uint32_t id, uindex_t& r_index) = 0;
	virtual void SetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value) = 0;
	virtual void GetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value) = 0;

protected:
	virtual void Realize(void) = 0;
	virtual void Unrealize(void) = 0;
	
	static void DoWindowStateChanged(void *object, bool realized);
	
protected:
	uint32_t m_references;

	MCPlatformWindowRef m_window;
};

////////////////////////////////////////////////////////////////////////////////

class MCQTKitPlayer: public MCPlatformPlayer
{
public:
	MCQTKitPlayer(void);
	virtual ~MCQTKitPlayer(void);
	
	virtual bool IsPlaying(void);
	virtual void Start(void);
	virtual void Stop(void);
	virtual void Step(int amount);
	
	virtual void LockBitmap(MCImageBitmap*& r_bitmap);
	virtual void UnlockBitmap(MCImageBitmap *bitmap);
	
	virtual void SetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);
	virtual void GetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);
	
	virtual void CountTracks(uindex_t& r_count);
	virtual void FindTrackWithId(uint32_t id, uindex_t& r_index);
	virtual void SetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
	virtual void GetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
	
protected:
	virtual void Realize(void);
	virtual void Unrealize(void);
	
private:
	void Load(const char *filename, bool is_url);
	void Synchronize(void);
	void Switch(bool new_offscreen);
	
	static void DoSwitch(void *context);
	static OSErr MovieDrawingComplete(Movie movie, long ref);
	
	QTMovie *m_movie;
	QTMovieView *m_view;
	CVImageBufferRef m_current_frame;
	
	MCRectangle m_rect;
	bool m_visible : 1;
	bool m_offscreen : 1;
	bool m_show_controller : 1;
	bool m_show_selection : 1;
	
	bool m_pending_offscreen : 1;
	
	bool m_switch_scheduled : 1;
};

////////////////////////////////////////////////////////////////////////////////

MCPlatformPlayer::MCPlatformPlayer(void)
{
	NSLog(@"Create player %p", this);
	m_references = 1;
	m_window = nil;
}

MCPlatformPlayer::~MCPlatformPlayer(void)
{
	NSLog(@"Destroy player %p", this);
	if (m_window != nil)
		Detach();
}

void MCPlatformPlayer::Retain(void)
{
	m_references += 1;
}

void MCPlatformPlayer::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
		delete this;
}

void MCPlatformPlayer::Attach(MCPlatformWindowRef p_window)
{
	if (m_window == p_window)
		return;
	
	if (m_window != nil)
		Detach();
	
	NSLog(@"Do attach %p to %p", this, p_window);
	
	m_window = p_window;
	MCPlatformRetainWindow(m_window);
	
	m_window -> AttachObject(this, DoWindowStateChanged);
}

void MCPlatformPlayer::Detach(void)
{
	if (m_window == nil)
		return;
	
	NSLog(@"Do detach %p from %p", this, m_window);
	
	m_window -> DetachObject(this);
	
	MCPlatformReleaseWindow(m_window);
	m_window = nil;
}

void MCPlatformPlayer::DoWindowStateChanged(void *p_ctxt, bool p_realized)
{
	NSLog(@"%p state change %d", p_ctxt, p_realized);
	
	MCPlatformPlayer *t_player;
	t_player = (MCPlatformPlayer *)p_ctxt;
	
	if (p_realized)
		t_player -> Realize();
	else
		t_player -> Unrealize();
}

////////////////////////////////////////////////////////////////////////////////

MCQTKitPlayer::MCQTKitPlayer(void)
{
	m_movie = [[QTMovie movie] retain];
	m_view = [[QTMovieView alloc] initWithFrame: NSZeroRect];
	
	m_current_frame = nil;
	
	m_rect = MCRectangleMake(0, 0, 0, 0);
	m_visible = true;
	m_offscreen = false;
	m_pending_offscreen = false;
	m_show_controller = false;
	m_show_selection = false;
	
	m_switch_scheduled = false;
}

MCQTKitPlayer::~MCQTKitPlayer(void)
{
	if (m_current_frame != nil)
		CFRelease(m_current_frame);
	
	[m_view release];
	[m_movie release];
}

OSErr MCQTKitPlayer::MovieDrawingComplete(Movie p_movie, long p_ref)
{
	MCQTKitPlayer *t_self;
	t_self = (MCQTKitPlayer *)p_ref;

	QTVisualContextRef t_context;
	t_context = nil;
	GetMovieVisualContext([t_self -> m_movie quickTimeMovie], &t_context);
	
	CVImageBufferRef t_image;
	t_image = nil;
	if (t_context != nil)
		QTVisualContextCopyImageForTime(t_context, kCFAllocatorDefault, NULL, &t_image);
	
	if (t_image != nil)
	{
		if (t_self -> m_current_frame != nil)
			CFRelease(t_self -> m_current_frame);
		t_self -> m_current_frame = t_image;
	}
	
	MCPlatformCallbackSendPlayerFrameChanged(t_self);
	
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
	
	NSLog(@"Do switch %p", ctxt);
	
	t_player -> m_switch_scheduled = false;
	
	if (t_player -> m_pending_offscreen == t_player -> m_offscreen)
	{
		// Do nothing if there is no state change.
	}
	else if (t_player -> m_pending_offscreen)
	{
		if (t_player -> m_view != nil)
			t_player -> Unrealize();
		
		SetMovieDrawingCompleteProc([t_player -> m_movie quickTimeMovie], movieDrawingCallWhenChanged, MCQTKitPlayer::MovieDrawingComplete, (long int)t_player);
		
		t_player -> m_offscreen = t_player -> m_pending_offscreen;
	}
	else
	{
		if (t_player -> m_current_frame != nil)
		{
			CFRelease(t_player -> m_current_frame);
			t_player -> m_current_frame = nil;
		}
		SetMovieDrawingCompleteProc([t_player -> m_movie quickTimeMovie], movieDrawingCallWhenChanged, nil, nil);
		
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
	
	NSLog(@"Realize movie view %p", m_view);
	
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
	
	NSLog(@"Unrealize movie view %p", m_view);
	
	if (!m_offscreen)
	{
		MCMacPlatformWindow *t_window;
		t_window = (MCMacPlatformWindow *)m_window;
	
		MCWindowView *t_parent_view;
		t_parent_view = t_window -> GetView();
	
		[m_view removeFromSuperview];
	}
}

void MCQTKitPlayer::Load(const char *p_filename, bool p_is_url)
{
	NSError *t_error;
	t_error = nil;
	
	NSDictionary *t_attrs;
	t_attrs = [NSDictionary dictionaryWithObjectsAndKeys:
			   [NSString stringWithCString: p_filename encoding: NSMacOSRomanStringEncoding], p_is_url ? QTMovieURLAttribute : QTMovieFileNameAttribute,
			   /* [NSNumber numberWithBool: YES], QTMovieOpenForPlaybackAttribute, */
			   [NSNumber numberWithBool: NO], QTMovieOpenAsyncOKAttribute,
			   [NSNumber numberWithBool: NO], QTMovieOpenAsyncRequiredAttribute,
			   nil];
	
	QTMovie *t_new_movie;
	t_new_movie = [[QTMovie alloc] initWithAttributes: t_attrs
												error: &t_error];
	
	if (t_error != nil)
	{
		NSLog(@"Error: @%", t_error);
		[t_new_movie release];
		return;
	}
	
	[m_movie release];
	m_movie = t_new_movie;
	
	// This method seems to be there - but isn't 'public'. Given QTKit is now deprecated as long
	// as it works on the platforms we support, it should be fine.
	[m_movie setDraggable: NO];
	
	[m_view setMovie: m_movie];
}

void MCQTKitPlayer::Synchronize(void)
{
	if (m_window == nil)
		return;
	
	NSLog(@"Synchronize movie");
	
	MCMacPlatformWindow *t_window;
	t_window = (MCMacPlatformWindow *)m_window;
	
	NSRect t_frame;
	t_window -> MapMCRectangleToNSRect(m_rect, t_frame);

	[m_view setFrame: t_frame];
	
	[m_view setHidden: !m_visible];
	
	[m_view setEditable: m_show_selection];
	[m_view setControllerVisible: m_show_controller];
	
	MCMovieChanged([m_movie quickTimeMovieController], [m_movie quickTimeMovie]);
}

bool MCQTKitPlayer::IsPlaying(void)
{
	return [m_movie rate] != 0;
}

void MCQTKitPlayer::Start(void)
{
	[m_movie setRate: 1.0];
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
	// First get the image from the view - this will have black where the movie
	// should be.
	
	NSRect t_rect;
	if (m_offscreen)
		t_rect = NSMakeRect(0, 0, m_rect . width, m_rect . height);
	else
		t_rect = [m_view frame];
	
	NSRect t_movie_rect;
	t_movie_rect = [m_view movieBounds];
	
	NSBitmapImageRep *t_rep;
	t_rep = [m_view bitmapImageRepForCachingDisplayInRect: t_rect];
	[m_view cacheDisplayInRect: t_rect toBitmapImageRep: t_rep];
	
	MCImageBitmap *t_bitmap;
	t_bitmap = new MCImageBitmap;
	t_bitmap -> width = [t_rep pixelsWide];
	t_bitmap -> height = [t_rep pixelsHigh];
	t_bitmap -> stride = [t_rep bytesPerRow];
	t_bitmap -> data = (uint32_t *)[t_rep bitmapData];
	t_bitmap -> has_alpha = t_bitmap -> has_transparency = true;
	
	// Now if we have a current frame, then composite at the appropriate size into
	// the movie portion of the buffer.
	if (m_current_frame != nil)
	{
		extern CGBitmapInfo MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha);
		
		CGColorSpaceRef t_colorspace;
		t_colorspace = CGColorSpaceCreateDeviceRGB();
		
		CGContextRef t_cg_context;
		t_cg_context = CGBitmapContextCreate(t_bitmap -> data, t_bitmap -> width, t_bitmap -> height, 8, t_bitmap -> stride, t_colorspace, MCGPixelFormatToCGBitmapInfo(kMCGPixelFormatNative, true));
		
		CIContext *t_ci_context;
		t_ci_context = [CIContext contextWithCGContext: t_cg_context options: nil];
		
		CIImage *t_ci_image;
		t_ci_image = [[CIImage alloc] initWithCVImageBuffer: m_current_frame];
		
		[t_ci_context drawImage: t_ci_image inRect: CGRectMake(0, t_rect . size . height - t_movie_rect . size . height, t_movie_rect . size . width, t_movie_rect . size . height) fromRect: [t_ci_image extent]];
		
		[t_ci_image release];
		
		CGContextRelease(t_cg_context);
		CGColorSpaceRelease(t_colorspace);
	}
	
	r_bitmap = t_bitmap;
}

void MCQTKitPlayer::UnlockBitmap(MCImageBitmap *bitmap)
{
	delete bitmap;
}

void MCQTKitPlayer::SetProperty(MCPlatformPlayerProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
	switch(p_property)
	{
		case kMCPlatformPlayerPropertyURL:
			Load(*(const char **)p_value, true);
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyFilename:
			Load(*(const char **)p_value, false);
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
			[m_movie setCurrentTime: QTMakeTime(*(uint32_t *)p_value, [m_movie duration] . timeScale)];
			break;
		case kMCPlatformPlayerPropertyStartTime:
		{
			//QTTimeRange t_selection;
			//t_selection = [m_movie selection];
			
		}
		break;
		case kMCPlatformPlayerPropertyFinishTime:
			break;
		case kMCPlatformPlayerPropertyPlayRate:
			break;
		case kMCPlatformPlayerPropertyVolume:
			break;
		case kMCPlatformPlayerPropertyShowBadge:
			break;
		case kMCPlatformPlayerPropertyShowController:
			m_show_controller = *(bool *)p_value;
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyShowSelection:
			m_show_selection = *(bool *)p_value;
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyOnlyPlaySelection:
			[m_movie setAttribute: [NSNumber numberWithBool: *(bool *)p_value] forKey: QTMoviePlaysSelectionOnlyAttribute];
			break;
		case kMCPlatformPlayerPropertyLoop:
			[m_movie setAttribute: [NSNumber numberWithBool: *(bool *)p_value] forKey: QTMovieLoopsAttribute];
			break;
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
			t_value = [m_movie attributeForKey: QTMovieNaturalSizeAttribute];
			*(MCRectangle *)r_value = MCRectangleMake(0, 0, [t_value sizeValue] . width, [t_value sizeValue] . height);
			NSLog(@"NaturalSize = %d, %d", (*(MCRectangle *)r_value) . width, (*(MCRectangle *)r_value) . height);
		}
		break;
		case kMCPlatformPlayerPropertyVisible:
			*(bool *)r_value = m_visible;
			break;
		case kMCPlatformPlayerPropertyMediaTypes:
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
		case kMCPlatformPlayerPropertyShowBadge:
			break;
		case kMCPlatformPlayerPropertyShowController:
			break;
		case kMCPlatformPlayerPropertyShowSelection:
			break;
		case kMCPlatformPlayerPropertyOnlyPlaySelection:
			break;
		case kMCPlatformPlayerPropertyLoop:
			break;
	}
}

void MCQTKitPlayer::CountTracks(uindex_t& r_count)
{
	r_count = 0;
}

void MCQTKitPlayer::FindTrackWithId(uint32_t id, uindex_t& r_index)
{
}

void MCQTKitPlayer::SetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value)
{
}

void MCQTKitPlayer::GetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCreatePlayer(MCPlatformPlayerRef& r_player)
{
	r_player = new MCQTKitPlayer();
}

void MCPlatformPlayerRetain(MCPlatformPlayerRef player)
{
	player -> Retain();
}

void MCPlatformPlayerRelease(MCPlatformPlayerRef player)
{
	player -> Release();
}

void MCPlatformAttachPlayer(MCPlatformPlayerRef player, MCPlatformWindowRef window)
{
	player -> Attach(window);
}

void MCPlatformDetachPlayer(MCPlatformPlayerRef player)
{
	player -> Detach();
}

bool MCPlatformPlayerIsPlaying(MCPlatformPlayerRef player)
{
	return player -> IsPlaying();
}

void MCPlatformStepPlayer(MCPlatformPlayerRef player, int amount)
{
	player -> Step(amount);
}

void MCPlatformStartPlayer(MCPlatformPlayerRef player)
{
	player -> Start();
}

void MCPlatformStopPlayer(MCPlatformPlayerRef player)
{
	player -> Stop();
}

void MCPlatformLockPlayerBitmap(MCPlatformPlayerRef player, MCImageBitmap*& r_bitmap)
{
	player -> LockBitmap(r_bitmap);
}

void MCPlatformUnlockPlayerBitmap(MCPlatformPlayerRef player, MCImageBitmap *bitmap)
{
	player -> UnlockBitmap(bitmap);
}

void MCPlatformSetPlayerProperty(MCPlatformPlayerRef player, MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value)
{
	player -> SetProperty(property, type, value);
}

void MCPlatformGetPlayerProperty(MCPlatformPlayerRef player, MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value)
{
	player -> GetProperty(property, type, value);
}

void MCPlatformCountPlayerTracks(MCPlatformPlayerRef player, uindex_t& r_track_count)
{
}

void MCPlatformGetPlayerTrackProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value)
{
}

void MCPlatformSetPlayerTrackProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value)
{
}

bool MCPlatformFindPlayerTrackWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index)
{
}

void MCPlatformCountPlayerNodes(MCPlatformPlayerRef player, uindex_t& r_node_count)
{
}

void MCPlatformGetPlayerNodeProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerNodeProperty property, MCPlatformPropertyType type, void *value)
{
}

void MCPlatformSetPlayerNodeProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerNodeProperty property, MCPlatformPropertyType type, void *value)
{
}

void MCPlatformFindPlayerNodeWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index)
{
}

void MCPlatformCountPlayerHotSpots(MCPlatformPlayerRef player, uindex_t& r_node_count)
{
}

void MCPlatformGetPlayerHotSpotProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerHotSpotProperty property, MCPlatformPropertyType type, void *value)
{
}

void MCPlatformSetPlayerHotSpotProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerHotSpotProperty property, MCPlatformPropertyType type, void *value)
{
}

void MCPlatformFindPlayerHotSpotWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index)
{
}

////////////////////////////////////////////////////////////////////////////////
