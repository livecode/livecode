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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"

#include "globals.h"

#import <UIKit/UIKit.h>
#import <MediaPlayer/MPMediaPickerController.h>

#include "mbliphoneapp.h"
#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////

UIView *MCIPhoneGetView(void);
UIViewController *MCIPhoneGetViewController(void);

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCIPhonePickMediaDelegate : UIViewController <MPMediaPickerControllerDelegate, UIPopoverControllerDelegate>
{
	bool m_running;
	NSArray *media_returned;
	MPMediaPickerController *media_picker;
}

- (id)init;
- (void)dealloc;

@end

@implementation com_runrev_livecode_MCIPhonePickMediaDelegate

- (id)init
{
	m_running = true;
	media_picker = nil;
	media_returned = nil;
	return self;
}

- (void)dealloc
{
	[media_picker release];
    [super dealloc];
}

- (void) mediaPicker: (MPMediaPickerController *)mediaPicker didPickMediaItems: (MPMediaItemCollection *)mediaItemCollection
{
	if ([mediaItemCollection count] > 0)
		media_returned = [[mediaItemCollection items] copy];
	m_running = false;
	MCscreen -> pingwait();
}

- (void) mediaPickerDidCancel:(MPMediaPickerController *)mediaPicker
{
	m_running = false;
	MCscreen -> pingwait();
}

- (void) showMediaPicker: (bool) p_allow_multiple_items andTypes: (MPMediaType) p_media_types withResult: (NSArray*&)r_chosen
{
	int t_i;
	NSString *t_result;
	media_picker = [[MPMediaPickerController alloc] initWithMediaTypes: p_media_types];
	media_picker.delegate = self;
	media_picker.allowsPickingMultipleItems = p_allow_multiple_items;
    MCIPhoneRunBlockOnMainFiber(^() {
        [MCIPhoneGetViewController() presentModalViewController: media_picker animated: YES];
    });
	while (m_running)
		MCscreen -> wait(60.0, False, True);
	r_chosen = media_returned;
    MCIPhoneRunBlockOnMainFiber(^() {
        [MCIPhoneGetViewController() dismissModalViewControllerAnimated: YES];
    });
}
@end

// MM-2012-09-25: [[ Bug 10406 ]] Make sure media picker UI is run on main thread.
bool MCIPhonePickMedia(bool p_allow_multiple_items, MPMediaType p_media_types, NSString*& r_media_returned)
{
// HC-2011-10-28: [[ Media Picker ]] Ensure we do not crash if the media picker is used on the simulator.
	r_media_returned = nil;
#ifndef __i386__
	__block com_runrev_livecode_MCIPhonePickMediaDelegate *t_media_picker;
	__block NSArray *t_result_data;
	t_result_data = nil;
    MCIPhoneRunBlockOnMainFiber(^(void) {
        t_media_picker = [[com_runrev_livecode_MCIPhonePickMediaDelegate alloc] init];
    });
    [t_media_picker showMediaPicker: p_allow_multiple_items andTypes: p_media_types withResult: t_result_data];
	if (t_result_data != nil)
	{
        for (int t_i = 0; t_i < [t_result_data count]; t_i++)
		{
            // HC-2012-03-06: [[ Media Picker ]] Ensure we do not crash if the media picker is to play m4p files.
            // Test that we are handling valid URLs
            if ([[t_result_data objectAtIndex:t_i] valueForProperty:MPMediaItemPropertyAssetURL] != nil)
            {
                // Convert media_returned into an NSString with paths to the media entries.
                if (r_media_returned == nil)
                    r_media_returned = [[NSString alloc] initWithString:[[[t_result_data objectAtIndex:t_i] valueForProperty:MPMediaItemPropertyAssetURL] absoluteString]];
                else
                {
                    r_media_returned = [r_media_returned stringByAppendingString:@"\n"];
                    r_media_returned = [r_media_returned stringByAppendingString:[[[t_result_data objectAtIndex:t_i] valueForProperty:MPMediaItemPropertyAssetURL] absoluteString]];
                }
            }
		}
	}
    MCIPhoneRunBlockOnMainFiber(^() {
        [t_media_picker release];    
    });
#endif
	return ES_NORMAL;
}

bool MCSystemPickMedia(MCMediaType p_media_type, bool p_multiple, MCStringRef& r_result)
{
	char *t_option_list;
	MPMediaType t_media_types;
	NSString *r_return_media_types;
	
	t_media_types = 0;
	
	t_option_list = nil;
	
	// Get the options list.
    if (p_media_type & kMCMediaTypeSongs) // music
        t_media_types += MPMediaTypeMusic;
    if (p_media_type & kMCMediaTypePodcasts) // podCast
		t_media_types += MPMediaTypePodcast;
	if (p_media_type & kMCMediaTypeAudiobooks) // audioBook
		t_media_types += MPMediaTypeAudioBook;
#ifdef __IPHONE_5_0
	if (MCmajorosversion >= 500)
	{
		if (p_media_type & kMCMediaTypeMovies) // movie
			t_media_types += MPMediaTypeMovie;
		if (p_media_type & kMCMediaTypeTv) // tv
			t_media_types += MPMediaTypeTVShow;
		if (p_media_type & kMCMediaTypeVideoPodcasts) // videoPodcast
			t_media_types += MPMediaTypeVideoPodcast;
		if (p_media_type & kMCMediaTypeMusicVideos) // musicVideo videoITunesU
        {
            t_media_types += MPMediaTypeMusicVideo;
			t_media_types += MPMediaTypeVideoITunesU;
		}
	}
#endif
	if (t_media_types == 0)
	{
		t_media_types = MPMediaTypeAnyAudio;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
			t_media_types += MPMediaTypeAnyVideo;
#endif		
	}
	// Call MCIPhonePickMedia to process the media pick selection. 
	if (MCIPhonePickMedia(p_multiple, t_media_types, r_return_media_types) && r_return_media_types != nil)
		return MCStringCreateWithCFStringRef((CFStringRef)r_return_media_types, r_result);

	return false;
}

////////////////////////////////////////////////////////////////////////////////
