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
	virtual bool LockImageFrame(uindex_t p_index, bool p_premultiplied, MCGFloat p_density, MCImageFrame *&r_frame) = 0;
	virtual void UnlockImageFrame(uindex_t p_index, MCImageFrame *p_frame) = 0;
	virtual bool GetGeometry(uindex_t &r_width, uindex_t &r_height) = 0;

	virtual MCGFloat GetDensity() { return 1.0; };
	
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

	virtual const char *GetSearchKey() { return nil; };
	
	//////////
	
	virtual uint32_t GetFrameByteCount() = 0;
	virtual void ReleaseFrames() = 0;
	
	//////////
	
	static bool FindWithKey(const char *p_key, MCCachedImageRep *&r_rep);
	
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

	virtual uindex_t GetFrameCount();
	virtual bool LockImageFrame(uindex_t p_index, bool p_premultiplied, MCGFloat p_density, MCImageFrame *&r_frame);
	virtual void UnlockImageFrame(uindex_t p_index, MCImageFrame *p_frame);

	virtual bool GetGeometry(uindex_t &r_width, uindex_t &r_height);

	//////////

	virtual uint32_t GetFrameByteCount();
	virtual void ReleaseFrames();
	
protected:
	virtual bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height) = 0;
	// IM-2013-11-05: [[ RefactorGraphics ]] Add return parameter to indicate whether or not
	// returned frames are premultiplied
	virtual bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied) = 0;

	bool m_have_geometry;
	uindex_t m_width, m_height;

	bool m_premultiplied;

private:
	bool EnsureImageFrames(bool p_premultiplied);
	void PremultiplyFrames();
	void UnpremultiplyFrames();
	
	uindex_t m_lock_count;

	MCImageFrame *m_frames;
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

	uint32_t GetDataCompression();

protected:
	// returns the image frames as decoded from the input stream
	bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);

	//////////

	// return the input stream from which the image data will be read
	virtual bool GetDataStream(IO_handle &r_stream) = 0;

	//////////

	uint32_t m_compression;
};

//////////

class MCReferencedImageRep : public MCEncodedImageRep
{
public:
	MCReferencedImageRep(const char *p_filename, const char *p_searchkey);
	~MCReferencedImageRep();

	MCImageRepType GetType() { return kMCImageRepReferenced; }

	//////////

	const char *GetSearchKey()
	{
		return m_search_key;
	}

	//////////

protected:
	// open a datastream to the referenced image file
	bool GetDataStream(IO_handle &r_stream);

	char *m_file_name;
	char *m_search_key;

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
	bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied);
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
	bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);

	//////////

	MCImageCompressedBitmap *m_compressed;
};

////////////////////////////////////////////////////////////////////////////////
// Image representation that will return the given source at the specified scale using
// bicubic filter

class MCResampledImageRep : public MCLoadableImageRep
{
public:
	MCResampledImageRep(MCGFloat p_h_scale, MCGFloat p_v_scale, MCImageRep *p_source);
	~MCResampledImageRep();
	
	MCImageRepType GetType() { return kMCImageRepResampled; }
	uindex_t GetFrameCount() { return m_source->GetFrameCount(); }
	uint32_t GetDataCompression() { return m_source->GetDataCompression(); }
	
	//////////
	
	bool Matches(MCGFloat p_h_scale, MCGFloat p_v_scale, const MCImageRep *p_source);
	
protected:
	bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);
	
	//////////
	
	MCGFloat m_h_scale, m_v_scale;
	MCImageRep *m_source;
};

////////////////////////////////////////////////////////////////////////////////
// Image representation that can return different images for specific density values

class MCDensityMappedImageRep : public MCCachedImageRep
{
public:
	MCDensityMappedImageRep(const char *p_filename);
	~MCDensityMappedImageRep();
	
	MCImageRepType GetType() { return kMCImageRepReferenced; }
	uint32_t GetDataCompression();
	
	uindex_t GetFrameCount();
	bool LockImageFrame(uindex_t p_index, bool p_premultiplied, MCGFloat p_density, MCImageFrame *&r_frame);
	void UnlockImageFrame(uindex_t p_index, MCImageFrame *p_frame);
	bool GetGeometry(uindex_t &r_width, uindex_t &r_height);
	
	MCGFloat GetDensity();
	
	//////////

	const char *GetSearchKey() { return m_filename; }
	
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
	
	MCGFloat m_last_density;
	bool m_locked;
	uint32_t m_locked_source;
	
	char *m_filename;
};

////////////////////////////////////////////////////////////////////////////////

bool MCImageRepCreateReferencedWithSearchKey(const char *p_filename, const char *p_searchkey, MCImageRep *&r_rep);

bool MCImageRepGetReferenced(const char *p_filename, MCImageRep *&r_rep);
bool MCImageRepGetResident(void *p_data, uindex_t p_size, MCImageRep *&r_rep);
bool MCImageRepGetVector(void *p_data, uindex_t p_size, MCImageRep *&r_rep);
bool MCImageRepGetCompressed(MCImageCompressedBitmap *p_compressed, MCImageRep *&r_rep);
bool MCImageRepGetDensityMapped(const char *p_filename, MCImageRep *&r_rep);

bool MCImageRepGetResampled(MCGFloat p_h_scale, MCGFloat p_v_scale, MCImageRep *p_source, MCImageRep *&r_rep);

////////////////////////////////////////////////////////////////////////////////

#endif // __MC_IMAGE_REP_H__