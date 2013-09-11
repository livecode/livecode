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
	
	kMCImageRepTransformed,
} MCImageRepType;

////////////////////////////////////////////////////////////////////////////////
// Image representation interface

class MCImageRep
{
public:
	MCImageRep();
	virtual ~MCImageRep();

	virtual MCImageRepType GetType() = 0;
	virtual uindex_t GetFrameCount() = 0;
	virtual bool LockImageFrame(uindex_t p_index, bool p_premultiplied, MCImageFrame *&r_frame) = 0;
	virtual void UnlockImageFrame(uindex_t p_index, MCImageFrame *p_frame) = 0;
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
	MCCachedImageRep();
	virtual ~MCCachedImageRep();

	virtual MCImageRepType GetType() = 0;

	virtual uindex_t GetFrameCount();
	virtual bool LockImageFrame(uindex_t p_index, bool p_premultiplied, MCImageFrame *&r_frame);
	virtual void UnlockImageFrame(uindex_t p_index, MCImageFrame *p_frame);

	virtual bool GetGeometry(uindex_t &r_width, uindex_t &r_height);

	//////////

	uint32_t GetFrameByteCount();
	void ReleaseFrames();

	//////////

	static void init();
	static void AddRep(MCCachedImageRep *p_rep);
	static void RemoveRep(MCCachedImageRep *p_rep);
	static void MoveRepToHead(MCCachedImageRep *p_rep);

	static bool FindReferencedWithFilename(const char *p_filename, MCCachedImageRep *&r_rep);

	static void FlushCache();
	static void FlushCacheToLimit();
	
	static uint32_t GetCacheUsage() { return s_cache_size; }
	static void SetCacheLimit(uint32_t p_limit)	{ s_cache_limit = p_limit; }
	static uint32_t GetCacheLimit() { return s_cache_limit; }
	

protected:
	virtual bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height) = 0;
	virtual bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count) = 0;

	bool m_have_geometry;
	uindex_t m_width, m_height;

	bool m_premultiplied;

private:
	bool EnsureImageFrames(bool p_premultiplied);
	void PremultiplyFrames();
	
	uindex_t m_lock_count;

	MCImageFrame *m_frames;
	uindex_t m_frame_count;

	//////////

	MCCachedImageRep *m_next;
	MCCachedImageRep *m_prev;

	static MCCachedImageRep *s_head;
	static MCCachedImageRep *s_tail;

	//////////

	static uint32_t s_cache_size;
	static uint32_t s_cache_limit;
};

////////////////////////////////////////////////////////////////////////////////
// Encoded image representation

class MCEncodedImageRep : public MCCachedImageRep
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
	bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count);
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
	MCReferencedImageRep(const char *p_filename);
	~MCReferencedImageRep();

	MCImageRepType GetType() { return kMCImageRepReferenced; }

	//////////

	const char *GetFilename()
	{
		return m_file_name;
	}

	//////////

protected:
	// open a datastream to the referenced image file
	bool GetDataStream(IO_handle &r_stream);

	char *m_file_name;

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

class MCVectorImageRep : public MCCachedImageRep
{
public:
	MCVectorImageRep(const void *p_data, uindex_t p_size);
	~MCVectorImageRep();

	MCImageRepType GetType() { return kMCImageRepVector; }

	uindex_t GetFrameCount() { return 1; }

	//////////

	void GetData(void *&r_data, uindex_t &r_size)
	{
		r_data = m_data, r_size = m_size;
	}

	bool Render(MCDC *p_context, bool p_embed, MCRectangle &p_image_rect, MCRectangle &p_clip_rect);

protected:
	bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);

	//////////

	void *m_data;
	uindex_t m_size;
};

////////////////////////////////////////////////////////////////////////////////

class MCCompressedImageRep : public MCCachedImageRep
{
public:
	MCCompressedImageRep(MCImageCompressedBitmap *p_bitmap);
	~MCCompressedImageRep();

	MCImageRepType GetType() { return kMCImageRepCompressed; }

	uindex_t GetFrameCount() { return 1; }

	//////////

	MCImageCompressedBitmap *GetCompressed()
	{
		return m_compressed;
	}

protected:
	bool LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count);
	bool CalculateGeometry(uindex_t &r_width, uindex_t &r_height);

	//////////

	MCImageCompressedBitmap *m_compressed;
};

////////////////////////////////////////////////////////////////////////////////

bool MCImageRepGetReferenced(const char *p_filename, MCImageRep *&r_rep);
bool MCImageRepGetResident(void *p_data, uindex_t p_size, MCImageRep *&r_rep);
bool MCImageRepGetVector(void *p_data, uindex_t p_size, MCImageRep *&r_rep);
bool MCImageRepGetCompressed(MCImageCompressedBitmap *p_compressed, MCImageRep *&r_rep);

////////////////////////////////////////////////////////////////////////////////

#endif // __MC_IMAGE_REP_H__