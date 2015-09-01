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

#ifndef __MC_IMAGE_REP_H__
#define __MC_IMAGE_REP_H__

typedef enum
{
	kMCImageRepUnknown,

	kMCImageRepMutable,
	kMCImageRepReferenced,
	kMCImageRepResident,
	kMCImageRepVector,
	kMCImageRepCompressed,
	
	kMCImageRepResampled,
} MCImageRepType;

struct MCGImageFrame
{
	MCGImageRef image;
	
	uint32_t duration;
	
	// IM-2013-10-30: [[ FullscreenMode ]] add density value to image frames
	// IM-2014-08-07: [[ Bug 13021 ]] Split density into x / y scale components
	MCGFloat x_scale;
	MCGFloat y_scale;
};

void MCGImageFramesFree(MCGImageFrame *p_frames, uindex_t p_count);

////////////////////////////////////////////////////////////////////////////////
// Image representation interface

class MCImageRep
{
public:
	MCImageRep();
	virtual ~MCImageRep();

	virtual MCImageRepType GetType() = 0;
	virtual uint32_t GetDataCompression() = 0;
	
	virtual uindex_t GetFrameCount() = 0;
	virtual bool LockBitmapFrame(uindex_t p_index, MCGFloat p_density, MCBitmapFrame *&r_frame) = 0;
	virtual void UnlockBitmapFrame(uindex_t p_index, MCBitmapFrame *p_frame) = 0;

	virtual bool LockImageFrame(uindex_t p_index, MCGFloat p_density, MCGImageFrame& r_frame) = 0;
	virtual void UnlockImageFrame(uindex_t p_index, MCGImageFrame& p_frame) = 0;

	virtual bool GetGeometry(uindex_t &r_width, uindex_t &r_height) = 0;

	//////////

	MCImageRep *Retain();
	void Release();

private:
	uindex_t m_reference_count;
};

////////////////////////////////////////////////////////////////////////////////
// Base ImageRep class for cached images

class MCCachedImageRep : public MCImageRep
{
public:
	virtual ~MCCachedImageRep();

	static void init();
	static void AddRep(MCCachedImageRep *p_rep);
	static void RemoveRep(MCCachedImageRep *p_rep);
	static void MoveRepToHead(MCCachedImageRep *p_rep);

	virtual MCStringRef GetSearchKey() { return nil; };
	
	//////////
	
	virtual uint32_t GetFrameByteCount() = 0;
	virtual void ReleaseFrames() = 0;
	
	//////////
	
	static bool FindWithKey(MCStringRef p_key, MCCachedImageRep *&r_rep);
	
	static uint32_t GetCacheUsage() { return s_cache_size; }
	static void SetCacheLimit(uint32_t p_limit)	{ s_cache_limit = p_limit; }
	static uint32_t GetCacheLimit() { return s_cache_limit; }

	static void FlushCache();
	static void FlushCacheToLimit();
	
protected:
	MCCachedImageRep *m_next;
	MCCachedImageRep *m_prev;
	
	static MCCachedImageRep *s_head;
	static MCCachedImageRep *s_tail;

	//////////
	
	static uint32_t s_cache_size;
	static uint32_t s_cache_limit;
};

// Base CachedImageRep class for loadable image sources
class MCLoadableImageRep : public MCCachedImageRep
{
public:
	MCLoadableImageRep();
	virtual ~MCLoadableImageRep();

	virtual bool LockBitmapFrame(uindex_t p_index, MCGFloat p_density, MCBitmapFrame *&r_frame);
	virtual void UnlockBitmapFrame(uindex_t p_index, MCBitmapFrame *p_frame);
	
	virtual bool LockImageFrame(uindex_t p_index, MCGFloat p_density, MCGImageFrame& r_frame);
	virtual void UnlockImageFrame(uindex_t p_index, MCGImageFrame& p_frame);

	virtual bool GetGeometry(uindex_t &r_width, uindex_t &r_height);

	//////////

	virtual uint32_t GetFrameByteCount();
	virtual void ReleaseFrames();
	
protected:
	virtual bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height) = 0;
	// IM-2013-11-05: [[ RefactorGraphics ]] Add return parameter to indicate whether or not
	// returned frames are premultiplied
	virtual bool LoadImageFrames(MCBitmapFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied) = 0;

	bool m_have_geometry;
	uindex_t m_width, m_height;


private:
	bool ConvertToMCGFrames(MCBitmapFrame *&x_frames, uint32_t p_frame_count, bool p_premultiplied);
	bool EnsureMCGImageFrames();
	
	uindex_t m_lock_count;

	MCBitmapFrame *m_locked_frames;
	MCGImageFrame *m_frames;
	uindex_t m_frame_count;
	bool m_frames_premultiplied;
};

////////////////////////////////////////////////////////////////////////////////
// Encoded image representation

class MCEncodedImageRep : public MCLoadableImageRep
{
public:
	MCEncodedImageRep()
	{
		m_compression = F_RLE;
	}

	virtual ~MCEncodedImageRep();

	virtual uindex_t GetFrameCount();
	uint32_t GetDataCompression();

protected:
	// returns the image frames as decoded from the input stream
	bool LoadImageFrames(MCBitmapFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);

	//////////

	// return the input stream from which the image data will be read
	virtual bool GetDataStream(IO_handle &r_stream) = 0;

	//////////

	uint32_t m_compression;
	uint32_t m_header_frame_count;
};

//////////

class MCReferencedImageRep : public MCEncodedImageRep
{
public:
	MCReferencedImageRep(MCStringRef p_filename, MCStringRef p_searchkey);
	~MCReferencedImageRep();

	MCImageRepType GetType() { return kMCImageRepReferenced; }

	//////////

	MCStringRef GetSearchKey()
	{
		return m_search_key;
	}
    
	//////////

protected:
	// open a datastream to the referenced image file
	bool GetDataStream(IO_handle &r_stream);

	MCStringRef m_file_name;
	MCStringRef m_search_key;

	// hold data from remote image
	void *m_url_data;
	uindex_t m_url_data_size;
	
	// MW-2013-09-25: [[ Bug 10983 ]] Indicates whether an attempt has been made
	//   to load the url data before.
	bool m_url_load_attempted : 1;
};

//////////

class MCResidentImageRep : public MCEncodedImageRep
{
public:
	MCResidentImageRep(const void *p_data, uindex_t p_size);
	~MCResidentImageRep();

	MCImageRepType GetType() { return kMCImageRepResident; }

	//////////

	void GetData(void *&r_data, uindex_t &r_size)
	{
		r_data = m_data;
		r_size = m_size;
	}

protected:
	// open a 'fake' stream to the contained image data
	bool GetDataStream(IO_handle &r_stream);

	void *m_data;
	uindex_t m_size;
};

////////////////////////////////////////////////////////////////////////////////

class MCVectorImageRep : public MCLoadableImageRep
{
public:
	MCVectorImageRep(const void *p_data, uindex_t p_size);
	~MCVectorImageRep();

	MCImageRepType GetType() { return kMCImageRepVector; }
	uint32_t GetDataCompression();

	uindex_t GetFrameCount() { return 1; }

	//////////

	void GetData(void *&r_data, uindex_t &r_size)
	{
		r_data = m_data, r_size = m_size;
	}

	bool Render(MCDC *p_context, bool p_embed, MCRectangle &p_image_rect, MCRectangle &p_clip_rect);

protected:
	bool LoadImageFrames(MCBitmapFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);

	//////////

	void *m_data;
	uindex_t m_size;
};

////////////////////////////////////////////////////////////////////////////////

class MCCompressedImageRep : public MCLoadableImageRep
{
public:
	MCCompressedImageRep(MCImageCompressedBitmap *p_bitmap);
	~MCCompressedImageRep();

	MCImageRepType GetType() { return kMCImageRepCompressed; }
	uint32_t GetDataCompression();

	uindex_t GetFrameCount() { return 1; }

	//////////

	MCImageCompressedBitmap *GetCompressed()
	{
		return m_compressed;
	}

protected:
	bool LoadImageFrames(MCBitmapFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);

	//////////

	MCImageCompressedBitmap *m_compressed;
};

////////////////////////////////////////////////////////////////////////////////
// Image representation that will return the given source at the specified size using
// bicubic filter

// IM-2014-07-23: [[ Bug 12842 ]] Modify resampled image rep to take a target width & height
// and explicit flip params instead of scale values.
class MCResampledImageRep : public MCLoadableImageRep
{
public:
	MCResampledImageRep(uint32_t p_width, uint32_t p_height, bool p_flip_horizontal, bool p_flip_vertical, MCImageRep *p_source);
	~MCResampledImageRep();
	
	MCImageRepType GetType() { return kMCImageRepResampled; }
	uindex_t GetFrameCount() { return m_source->GetFrameCount(); }
	uint32_t GetDataCompression() { return m_source->GetDataCompression(); }
	
	//////////
	
	bool Matches(uint32_t p_width, uint32_t p_height, bool p_flip_horizontal, bool p_flip_vertical, const MCImageRep *p_source);
	
protected:
	bool LoadImageFrames(MCBitmapFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);
	
	//////////
	
	uint32_t m_target_width, m_target_height;
	bool m_h_flip, m_v_flip;
	MCImageRep *m_source;
};

////////////////////////////////////////////////////////////////////////////////
// Image representation that can return different images for specific density values

class MCDensityMappedImageRep : public MCCachedImageRep
{
public:
	MCDensityMappedImageRep(MCStringRef p_filename);
	~MCDensityMappedImageRep();
	
	MCImageRepType GetType() { return kMCImageRepReferenced; }
	uint32_t GetDataCompression();
	
	uindex_t GetFrameCount();
	bool LockBitmapFrame(uindex_t p_index, MCGFloat p_density, MCBitmapFrame *&r_frame);
	void UnlockBitmapFrame(uindex_t p_index, MCBitmapFrame *p_frame);
	
	bool LockImageFrame(uindex_t p_index, MCGFloat p_density, MCGImageFrame& r_frame);
	void UnlockImageFrame(uindex_t p_index, MCGImageFrame& p_frame);
	
	bool GetGeometry(uindex_t &r_width, uindex_t &r_height);
	
	//////////

	MCStringRef GetSearchKey() { return m_filename; }
	
	uint32_t GetFrameByteCount() { return 0; }
	void ReleaseFrames() {};
	
	//////////
	
	bool AddImageSourceWithDensity(MCReferencedImageRep *p_source, MCGFloat p_density);
	
	//////////
	
protected:
	
	bool GetBestMatch(MCGFloat p_density, uindex_t &r_match);
	
	//////////
	
	MCReferencedImageRep **m_sources;
	MCGFloat *m_source_densities;
	uindex_t m_source_count;
	
	bool m_locked;
	uint32_t m_locked_source;
	
	MCStringRef m_filename;
};

////////////////////////////////////////////////////////////////////////////////

bool MCImageRepCreateReferencedWithSearchKey(MCStringRef p_filename, MCStringRef p_searchkey, MCImageRep *&r_rep);

bool MCImageRepGetReferenced(MCStringRef p_filename, MCImageRep *&r_rep);
bool MCImageRepGetResident(void *p_data, uindex_t p_size, MCImageRep *&r_rep);
bool MCImageRepGetVector(void *p_data, uindex_t p_size, MCImageRep *&r_rep);
bool MCImageRepGetCompressed(MCImageCompressedBitmap *p_compressed, MCImageRep *&r_rep);
bool MCImageRepGetDensityMapped(MCStringRef p_filename, MCImageRep *&r_rep);

// IM-2014-07-23: [[ Bug 12842 ]] Modify resampled image rep to take a target width & height
// and explicit flip params instead of scale values.
bool MCImageRepGetResampled(uint32_t p_width, uint32_t p_height, bool p_flip_horizontal, bool p_flip_vertical, MCImageRep *p_source, MCImageRep *&r_rep);

////////////////////////////////////////////////////////////////////////////////

#endif // __MC_IMAGE_REP_H__
