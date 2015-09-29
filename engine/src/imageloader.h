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

#ifndef __MC_IMAGE_LOADER_H__
#define __MC_IMAGE_LOADER_H__

#include "objdefs.h"

// Supported import formats
enum MCImageLoaderFormat
{
	kMCImageFormatUnknown,
	
	kMCImageFormatGIF,
	kMCImageFormatPNG,
	kMCImageFormatJPEG,
	kMCImageFormatMetafile,
	kMCImageFormatBMP,
	kMCImageFormatXBM,
	kMCImageFormatXPM,
	kMCImageFormatNetPBM,
	kMCImageFormatXWD,
};

// Abstract class for reading an image in 2 stages - header data then bitmap data
class MCImageLoader
{
public:
	MCImageLoader(IO_handle p_stream);
	virtual ~MCImageLoader();
	
	// Returns the image format understood by the loader
	virtual MCImageLoaderFormat GetFormat() = 0;
	
	// Returns the width & height of the image
	bool GetGeometry(uint32_t &r_width, uint32_t &r_height);
	// Returns the image hotspot
	bool GetHotSpot(uint32_t &r_x, uint32_t &r_y);
	// Returns the image name or nil if not specified
	bool GetName(MCStringRef &r_name);
	// Returns the number of frames in the image
	virtual bool GetFrameCount(uint32_t &r_frame_count);
    // Returns the metadata for the image
    bool GetMetadata(MCImageMetadata &r_metadata);
	// Returns the decoded image bitmap frames
	bool GetFrames(MCBitmapFrame *&r_frames, uint32_t &r_frame_count);
	
	// Returns the image bitmap frames, transferring ownership to the caller
	bool TakeFrames(MCBitmapFrame *&r_frames, uint32_t &r_count);
	
	//////////

	// Returns an image loader class that can decode the specified image format
	static bool LoaderForStreamWithFormat(IO_handle p_stream, MCImageLoaderFormat p_format, MCImageLoader *&r_loader);
	// Returns an image loader class that can decode the image format identified by the stream contents
	static bool LoaderForStream(IO_handle p_stream, MCImageLoader *&r_loader);
	// Returns the best guess of the image format based on the contents of the stream
	static bool IdentifyFormat(IO_handle p_stream, MCImageLoaderFormat &r_format);

protected:
	// Implemented by subclasses to perform the image header loading
	virtual bool LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata) = 0;
	// Implemented by subclasses to perform the image bitmap frame loading
	virtual bool LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count) = 0;
	
	// Used by subclasses to get the data stream
	IO_handle GetStream();
	
	bool EnsureHeader();
	bool EnsureFrames();
	
private:
	
	bool m_header_loaded;
	bool m_frames_loaded;
	bool m_valid;
	
	IO_handle m_stream;
	
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_xhot;
	uint32_t m_yhot;
	
	MCBitmapFrame *m_frames;
	uint32_t m_frame_count;
    
    MCImageMetadata m_metadata;
	
	MCStringRef m_name;
};

#endif // __MC_IMAGE_LOADER_H__
