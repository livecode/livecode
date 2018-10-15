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
#include "mcio.h"

#include "uidc.h"
#include "util.h"
#include "image.h"
#include "globals.h"

#include "imageloader.h"

#include <setjmp.h>

extern "C"
{
#include <jpeglib.h>
#include <jerror.h>
}

typedef struct
{
	struct jpeg_source_mgr pub;	/* public fields */
	JOCTET *buffer;		/* start of buffer */
	uint4  buffersize;
}
MC_jpegsrc;

typedef struct
{
	struct jpeg_destination_mgr pub; /* public fields */
	IO_handle stream;		/* target stream */
	JOCTET *buffer;		/* start of buffer */
}
MC_jpegdest;

struct MC_jpegerr
{
	struct jpeg_error_mgr pub;	/* "public" fields */
	jmp_buf setjmp_buffer;	/* for return to caller */
};

#define JPEG_BUF_SIZE  4096	/* choose a reasonable max buffer size*/

typedef MC_jpegsrc *MC_jpegsrc_ptr;
typedef struct MC_jpegerr *MC_jpegerr_ptr;
typedef MC_jpegdest *MC_jpegdest_ptr;

//extended source manager
extern "C" void init_source(j_decompress_ptr cinfo)
{
}

/* Fill the input buffer --- called whenever buffer is emptied.*/
extern "C" boolean fill_input_buffer(j_decompress_ptr cinfo)
{
	MC_jpegsrc_ptr src = (MC_jpegsrc_ptr) cinfo->src;
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = src->buffersize;
	return TRUE;
}

extern "C" void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	MC_jpegsrc_ptr src = (MC_jpegsrc_ptr) cinfo->src;
	if (num_bytes > 0)
	{
		while (num_bytes > (long) src->pub.bytes_in_buffer)
		{
			num_bytes -= (long) src->pub.bytes_in_buffer;
			(void) fill_input_buffer(cinfo);
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

extern "C" void term_source(j_decompress_ptr cinfo)
{
}

void jpg_MCsetsource(j_decompress_ptr cinfo, uint1 *data,uint4 datasize)
{
	MC_jpegsrc_ptr src;
	if (cinfo->src == NULL)
	{	/* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr *)
		             (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
		                                         sizeof(MC_jpegsrc));
		src = (MC_jpegsrc_ptr) cinfo->src;
	}
	src = (MC_jpegsrc_ptr) cinfo->src;
	src->buffer = (JOCTET *)data;
	src->buffersize = datasize;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}

extern "C" void jpg_MCerr_exit(j_common_ptr cinfo)
{
	MC_jpegerr_ptr myerr = (MC_jpegerr_ptr) cinfo->err;
	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

bool MCImageValidateJpeg(const void *p_data, uint32_t p_length, uint16_t& r_width, uint16_t& r_height)
{
	uint4 height,width;
	struct jpeg_decompress_struct cinfo;
	struct MC_jpegerr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = jpg_MCerr_exit;
	if (setjmp(jerr.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
		return false;
	}
	jpeg_create_decompress(&cinfo);
	jpg_MCsetsource(&cinfo, (uint1 *)p_data, p_length);
	(void) jpeg_read_header(&cinfo, TRUE);
	cinfo.out_color_space = JCS_RGB;
	(void) jpeg_start_decompress(&cinfo);
	width = cinfo.output_width;
	height = cinfo.output_height;

	r_width = (uint16_t)width;
	r_height = (uint16_t)height;

	return true;
}

static boolean read_icc_profile(j_decompress_ptr cinfo, JOCTET **icc_data_ptr, unsigned int *icc_data_len);
static bool read_exif_orientation(j_decompress_ptr cinfo, uint32_t* r_orientation);

////DECODING

static uint16_t exif_swap_uint16(uint16_t p_value, bool p_is_little)
{
#ifdef __LITTLE_ENDIAN__
	if (p_is_little)
		return p_value;
#else
	if (!p_is_little)
		return p_value;
#endif
	return ((p_value >> 8) & 0xff) | ((p_value << 8) & 0xff00); 
}

static uint32_t exif_swap_uint32(uint32_t p_value, bool p_is_little)
{
#ifdef __LITTLE_ENDIAN__
	if (p_is_little)
		return p_value;
#else
	if (!p_is_little)
		return p_value;
#endif
	return (p_value >> 24) | ((p_value >> 8) & 0xff00) | ((p_value & 0xff00) << 8) | (p_value << 24); 
}

// MW-2012-09-04: [[ Exif ]] Read any orientation flag out of the exif data.
static bool read_exif_orientation(j_decompress_ptr cinfo, uint32_t *r_orientation)
{
	jpeg_saved_marker_ptr t_marker;
	t_marker = nil;
	for(t_marker = cinfo -> marker_list; t_marker != nil; t_marker = t_marker -> next)
		if (t_marker->marker == (JPEG_APP0 + 1))
			break;
	
	if (t_marker == nil)
		return false;
	
	uint8_t *t_data;
	uint32_t t_data_length;
	t_data = (uint8_t *)t_marker -> data;
	t_data_length = t_marker -> data_length;
	if (t_data_length < 14 ||
		t_data[0] != 'E' || t_data[1] != 'x' || t_data[2] != 'i' || t_data[3] != 'f' ||
		t_data[4] != 0 || t_data[5] != 0)
		return false;
	
	bool t_is_little_endian;
	if (t_data[6] == 'I' && t_data[7] == 'I')
		t_is_little_endian = true;
	else if (t_data[6] == 'M' && t_data[7] == 'M')
		t_is_little_endian = false;
	else
		return false;
	
	uint16_t t_tag;
	memcpy(&t_tag, t_data + 8, 2);
	t_tag = exif_swap_uint16(t_tag, t_is_little_endian);
	if (t_tag != 0x002A)
		return false;
	
	uint32_t t_offset;
	memcpy(&t_offset, t_data + 10, 4);
	t_offset = exif_swap_uint32(t_offset, t_is_little_endian);
	
	if (t_data_length < t_offset + 2)
		return false;
	
	uint16_t t_count;
	memcpy(&t_count, t_data + 6 + t_offset, 2);
	t_count = exif_swap_uint16(t_count, t_is_little_endian);
	
	if (t_data_length < 6 + t_offset + 2 + 12 * t_count)
		return false;
	
	uint8_t *t_ifd_ptr;
	t_ifd_ptr = t_data + 6 + t_offset + 2;
	
	for(uindex_t i = 0; i < t_count; i++)
	{
		uint16_t t_format;
		uint32_t t_components;
		memcpy(&t_tag, t_ifd_ptr + 0, 2);
		memcpy(&t_format, t_ifd_ptr + 2, 2);
		memcpy(&t_components, t_ifd_ptr + 4, 4);
		t_tag = exif_swap_uint16(t_tag, t_is_little_endian);
		t_format = exif_swap_uint16(t_format, t_is_little_endian);
		t_components = exif_swap_uint32(t_components, t_is_little_endian);
		
		if (t_tag == 0x0112)
		{
			uint16_t t_orientation;
			memcpy(&t_orientation, t_ifd_ptr + 8, 2);
			t_orientation = exif_swap_uint16(t_orientation, t_is_little_endian);
			*r_orientation = t_orientation;
			return true;
		}
		
		t_ifd_ptr += 12;
	}
	
	return false;
}

template<typename T> static inline void swap(T& a, T& b)
{
	T t;
	t = a;
	a = b;
	b = t;
}

// MW-2012-09-04: [[ Exif ]] Apply the new orientation to the image data.
static bool apply_exif_orientation(uint32_t p_orientation, MCImageBitmap *p_bitmap)
{
	bool t_flip_x, t_flip_y, t_invert;
	switch(p_orientation)
	{
		case 1:
			t_flip_x = t_flip_y = t_invert = false;
			break;
		case 2:
			t_flip_x = true;
			t_flip_y = t_invert = false;
			break;
		case 3:
			t_flip_x = t_flip_y = true;
			t_invert = false;
			break;
		case 4:
			t_flip_y = true;
			t_flip_x = t_invert = false;
			break;
		case 5:
			t_invert = true;
			t_flip_x = t_flip_y = false;
			break;
		case 6:
			t_invert = t_flip_y = true;
			t_flip_x = false;
			break;
		case 7:
			t_flip_x = t_flip_y = t_invert = true;
			break;
		case 8:
			t_invert = t_flip_x = true;
			t_flip_y = false;
			break;
		default:
			return true;
	}
	
	uint32_t *t_data;
	t_data = (uint32_t *)p_bitmap -> data;
	
	uint32_t t_stride;
	t_stride = p_bitmap -> stride / 4;
	
	if (t_flip_x)
		for(int32_t y = 0; y < p_bitmap -> height; y++)
			for(int32_t x = 0; x < p_bitmap -> width / 2; x++)
				swap(t_data[y * t_stride + x], t_data[y * t_stride + (p_bitmap -> width - x - 1)]);
	
	if (t_flip_y)
		for(int32_t y = 0; y < p_bitmap -> height / 2; y++)
			for(int32_t x = 0; x < p_bitmap -> width; x++)
				swap(t_data[y * t_stride + x], t_data[(p_bitmap -> height - y - 1) * t_stride + x]);
	
	if (!t_invert)
		return true;
	
	uint32_t *t_new_data;
	t_new_data = (uint32_t *)malloc(p_bitmap -> width * p_bitmap -> height * 4);
	if (t_new_data == nil)
		return false;
	
	uint32_t t_new_stride;
	t_new_stride = p_bitmap -> height;
	for(int32_t y = 0; y < p_bitmap -> width; y++)
		for(int32_t x = 0; x < p_bitmap -> height; x++)
			t_new_data[y * t_new_stride + x] = t_data[x * t_stride + y];
	
	MCMemoryDeallocate(p_bitmap -> data);
	p_bitmap -> data = t_new_data;
	p_bitmap -> stride = t_new_stride * 4;
	swap(p_bitmap -> width, p_bitmap -> height);

	return true;
}

////////////////////////////////////////////////////////////////////////
//
//  Little bit of code cribbed from littleCMS - this extracts any icc
//  profile data from a jpeg.
//

#define ICC_MARKER  (JPEG_APP0 + 2)	/* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14		/* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533	/* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)

static boolean
marker_is_icc (jpeg_saved_marker_ptr marker)
{
	if (marker->marker == ICC_MARKER &&
	    marker->data_length >= ICC_OVERHEAD_LEN &&
	    /* verify the identifying string */
	    GETJOCTET(marker->data[0]) == 0x49 &&
	    GETJOCTET(marker->data[1]) == 0x43 &&
	    GETJOCTET(marker->data[2]) == 0x43 &&
	    GETJOCTET(marker->data[3]) == 0x5F &&
	    GETJOCTET(marker->data[4]) == 0x50 &&
	    GETJOCTET(marker->data[5]) == 0x52 &&
	    GETJOCTET(marker->data[6]) == 0x4F &&
	    GETJOCTET(marker->data[7]) == 0x46 &&
	    GETJOCTET(marker->data[8]) == 0x49 &&
	    GETJOCTET(marker->data[9]) == 0x4C &&
	    GETJOCTET(marker->data[10]) == 0x45 &&
	    GETJOCTET(marker->data[11]) == 0x0)
		return TRUE;
	else
		return FALSE;
}


/*
 * See if there was an ICC profile in the JPEG file being read;
 * if so, reassemble and return the profile data.
 *
 * TRUE is returned if an ICC profile was found, FALSE if not.
 * If TRUE is returned, *icc_data_ptr is set to point to the
 * returned data, and *icc_data_len is set to its length.
 *
 * IMPORTANT: the data at **icc_data_ptr has been allocated with malloc()
 * and must be freed by the caller with free() when the caller no longer
 * needs it.  (Alternatively, we could write this routine to use the
 * IJG library's memory allocator, so that the data would be freed implicitly
 * at jpeg_finish_decompress() time.  But it seems likely that many apps
 * will prefer to have the data stick around after decompression finishes.)
 *
 * NOTE: if the file contains invalid ICC APP2 markers, we just silently
 * return FALSE.  You might want to issue an error message instead.
 */

boolean
read_icc_profile (j_decompress_ptr cinfo,
		  JOCTET **icc_data_ptr,
		  unsigned int *icc_data_len)
{
  jpeg_saved_marker_ptr marker;
  int num_markers = 0;
  int seq_no;
  JOCTET *icc_data;
  unsigned int total_length;
#define MAX_SEQ_NO  255		/* sufficient since marker numbers are bytes */
  char marker_present[MAX_SEQ_NO+1];	  /* 1 if marker found */
  unsigned int data_length[MAX_SEQ_NO+1]; /* size of profile data in marker */
  unsigned int data_offset[MAX_SEQ_NO+1]; /* offset for data in marker */

  *icc_data_ptr = NULL;		/* avoid confusion if FALSE return */
  *icc_data_len = 0;

  /* This first pass over the saved markers discovers whether there are
   * any ICC markers and verifies the consistency of the marker numbering.
   */

  for (seq_no = 1; seq_no <= MAX_SEQ_NO; seq_no++)
    marker_present[seq_no] = 0;

  for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
    if (marker_is_icc(marker)) {
      if (num_markers == 0)
	num_markers = GETJOCTET(marker->data[13]);
      else if (num_markers != GETJOCTET(marker->data[13]))
	return FALSE;		/* inconsistent num_markers fields */
      seq_no = GETJOCTET(marker->data[12]);
      if (seq_no <= 0 || seq_no > num_markers)
	return FALSE;		/* bogus sequence number */
      if (marker_present[seq_no])
	return FALSE;		/* duplicate sequence numbers */
      marker_present[seq_no] = 1;
      data_length[seq_no] = marker->data_length - ICC_OVERHEAD_LEN;
    }
  }

  if (num_markers == 0)
    return FALSE;

  /* Check for missing markers, count total space needed,
   * compute offset of each marker's part of the data.
   */

  total_length = 0;
  for (seq_no = 1; seq_no <= num_markers; seq_no++) {
    if (marker_present[seq_no] == 0)
      return FALSE;		/* missing sequence number */
    data_offset[seq_no] = total_length;
    total_length += data_length[seq_no];
  }

  if (total_length <= 0)
    return FALSE;		/* found only empty markers? */

  /* Allocate space for assembled data */
  icc_data = (JOCTET *) malloc(total_length * sizeof(JOCTET));
  if (icc_data == NULL)
    return FALSE;		/* oops, out of memory */

  /* and fill it in */
  for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
    if (marker_is_icc(marker)) {
      JOCTET FAR *src_ptr;
      JOCTET *dst_ptr;
      unsigned int length;
      seq_no = GETJOCTET(marker->data[12]);
      dst_ptr = icc_data + data_offset[seq_no];
      src_ptr = marker->data + ICC_OVERHEAD_LEN;
      length = data_length[seq_no];
      while (length--) {
	*dst_ptr++ = *src_ptr++;
      }
    }
  }

  *icc_data_ptr = icc_data;
  *icc_data_len = total_length;

  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

struct MCJPEGSrcManager
{
	jpeg_source_mgr src;
	IO_handle stream;
	JOCTET *buffer;
};

void srcmgr_init_source(j_decompress_ptr p_jpeg)
{
	MCJPEGSrcManager *t_src = (MCJPEGSrcManager*)p_jpeg->src;
	t_src->src.bytes_in_buffer = 0;
}

boolean srcmgr_fill_input_buffer(j_decompress_ptr p_jpeg)
{
	MCJPEGSrcManager *t_src = (MCJPEGSrcManager*)p_jpeg->src;
	uindex_t t_bytes_read = JPEG_BUF_SIZE;
	IO_stat t_stat = MCS_readall(t_src->buffer, t_bytes_read, t_src->stream, t_bytes_read); /// ??? readall ???
	if (t_stat == IO_ERROR)
		ERREXIT(p_jpeg, JERR_FILE_READ);

	t_src->src.bytes_in_buffer = t_bytes_read;
	t_src->src.next_input_byte = t_src->buffer;

	return TRUE;
}

void srcmgr_skip_input_data(j_decompress_ptr p_jpeg, long p_count)
{
	MCJPEGSrcManager *t_src = (MCJPEGSrcManager*)p_jpeg->src;

	int32_t t_consume = MCMin((int32_t)p_count, (int32_t)t_src->src.bytes_in_buffer);
	p_count -= t_consume;
	t_src->src.next_input_byte += t_consume;
	t_src->src.bytes_in_buffer -= t_consume;

	if (p_count > 0)
	{
		if (IO_NORMAL != MCS_seek_cur(t_src->stream, p_count))
			ERREXIT(p_jpeg, JERR_FILE_READ);
	}
}

void srcmgr_term_source(j_decompress_ptr p_jpeg)
{
}

bool MCJPEGCreateSrcManager(IO_handle p_stream, MCJPEGSrcManager *&r_manager)
{
	bool t_success = true;

	MCJPEGSrcManager *t_manager = nil;
	t_success = MCMemoryNew(t_manager) && MCMemoryAllocate(JPEG_BUF_SIZE, t_manager->buffer);
	if (t_success)
	{
		t_manager->src.init_source = srcmgr_init_source;
		t_manager->src.fill_input_buffer = srcmgr_fill_input_buffer;
		t_manager->src.skip_input_data = srcmgr_skip_input_data;
		t_manager->src.resync_to_restart = jpeg_resync_to_restart;
		t_manager->src.term_source = srcmgr_term_source;
		t_manager->stream = p_stream;

		r_manager = t_manager;
	}
	else
	{
		MCMemoryDelete(t_manager);
	}

	return t_success;
}

void MCJPEGFreeSrcManager(MCJPEGSrcManager *p_manager)
{
	if (p_manager != nil)
	{
		MCMemoryDeallocate(p_manager->buffer);
		MCMemoryDelete(p_manager);
	}
}

class MCJPEGImageLoader : public MCImageLoader
{
public:
	MCJPEGImageLoader(IO_handle p_stream);
	virtual ~MCJPEGImageLoader();
	
	virtual MCImageLoaderFormat GetFormat() { return kMCImageFormatJPEG; }
	
protected:
	virtual bool LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata);
	virtual bool LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count);
	
private:
	jpeg_decompress_struct m_jpeg;
	MC_jpegerr m_error;
	MCJPEGSrcManager *m_src;
	JOCTET *m_icc;
	uint32_t m_icc_length;
	uint32_t m_orientation;
};

MCJPEGImageLoader::MCJPEGImageLoader(IO_handle p_stream) : MCImageLoader(p_stream)
{
	m_src = nil;
	m_icc = nil;
	m_icc_length = 0;
	m_orientation = 0;
	
	MCMemoryClear(&m_jpeg, sizeof(m_jpeg));
}

MCJPEGImageLoader::~MCJPEGImageLoader()
{
	jpeg_destroy_decompress(&m_jpeg);
	
	if (m_src != nil)
		MCJPEGFreeSrcManager(m_src);

	if (m_icc != nil)
		MCMemoryDeallocate(m_icc);
}

bool MCJPEGImageLoader::LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata)
{
	bool t_success = true;
	
	m_jpeg.err = jpeg_std_error((jpeg_error_mgr*) &m_error);
	m_error.pub.error_exit = jpg_MCerr_exit;
	
	if (setjmp(m_error.setjmp_buffer))
	{
		t_success = false;
	}

	if (t_success)
		jpeg_create_decompress(&m_jpeg);

	if (t_success)
		t_success = MCJPEGCreateSrcManager(GetStream(), m_src);

	if (t_success)
	{
		m_jpeg.src = (jpeg_source_mgr*)m_src;
		
		// MW-2012-09-04: [[ Exif ]] Save any APP1 markers so we can see if there is
		//   any orientation flags.
		jpeg_save_markers(&m_jpeg, JPEG_APP0 + 1, 0xffff);
		
		// MW-2009-12-09: Save any APP2 markers so we can see if there is any color
		//   profile data.
		jpeg_save_markers(&m_jpeg, JPEG_APP0 + 2, 0xffff);

		jpeg_read_header(&m_jpeg, TRUE);

		// IM-2014-07-31: [[ ImageLoader ]] call calculate_dimensions here to ensure output width & height are set in the jpeg struct
		jpeg_calc_output_dimensions(&m_jpeg);
	}
    
    // MERG-2014-09-12: [[ ImageMetadata ]] load image metatadata
    if (t_success)
    {
        /* density_unit can be 0 for unknown, */
        /* 1 for dots/inch, or 2 for dots/cm. */
        if (m_jpeg.density_unit != 0)
        {
            MCImageMetadata t_metadata;
            MCMemoryClear(&t_metadata, sizeof(t_metadata));
            t_metadata.has_density = true;
            if (m_jpeg.density_unit == 1)
                t_metadata.density = m_jpeg.X_density;
            else
                t_metadata.density = floor(m_jpeg.X_density * 2.54 + 0.5);
         
            r_metadata = t_metadata;
        }
        
    }
    
   	if (t_success)
	{
		// MW-2009-12-09: [[ Bug 2983 ]] Add support for CMYK jpegs.
		if (m_jpeg.jpeg_color_space == JCS_CMYK ||
			m_jpeg.jpeg_color_space == JCS_YCCK)
		{
			m_jpeg.out_color_space = JCS_CMYK;
		}
		else
			m_jpeg.out_color_space = JCS_RGB;
	}

	if (t_success)
	{
		// Fetch the color profile, if any.
		read_icc_profile(&m_jpeg, &m_icc, &m_icc_length);
		
		// Fetch the orientation tag, if any.
		if (!read_exif_orientation(&m_jpeg, &m_orientation))
			m_orientation = 0;
	}

	if (t_success)
	{
		r_width = m_jpeg.output_width;
		r_height = m_jpeg.output_height;
		
		if (m_orientation > 4 && m_orientation <= 8)
			swap(r_width, r_height);
		
		r_xhot = r_yhot = 0;
		r_name = MCValueRetain(kMCEmptyString);
		r_frame_count = 1;
	}
	
	return t_success;
}

bool MCJPEGImageLoader::LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count)
{
	bool t_success = true;
	
	MCBitmapFrame *t_frame;
	t_frame = nil;
	
	JSAMPROW t_row_buffer = nil;
	
	if (setjmp(m_error.setjmp_buffer))
	{
		t_success = false;
	}

	if (t_success)
		jpeg_start_decompress(&m_jpeg);

	if (t_success)
		t_success = MCMemoryNew(t_frame);

	if (t_success)
		t_success = MCImageBitmapCreate(m_jpeg.output_width, m_jpeg.output_height, t_frame->image);

	if (t_success)
		t_success = MCMemoryAllocate(m_jpeg.output_width * m_jpeg.output_components, t_row_buffer);

	// Read the input data scanline by scanline and convert into the bit format
	// we need. This is ARGB or KYMC order (as bytes in a 32-bit word).
	if (t_success)
	{
		while (m_jpeg.output_scanline < m_jpeg.output_height)
		{
			uint32_t *t_dst_ptr = (uint32_t*) (((uint8_t*)t_frame->image->data) + m_jpeg.output_scanline * t_frame->image->stride);

			jpeg_read_scanlines(&m_jpeg, &t_row_buffer, 1);
			JSAMPROW t_src_ptr = t_row_buffer;

			if (m_jpeg.out_color_space != JCS_CMYK)
			{
				for (uindex_t x = 0; x < m_jpeg.output_width; x++)
				{
					*t_dst_ptr++ = MCGPixelPackNative(t_src_ptr[0], t_src_ptr[1], t_src_ptr[2], 255);
					t_src_ptr += 3;
				}
			}
			else
			{
				for (uindex_t x = 0; x < m_jpeg.output_width; x++)
				{
					uint32_t t_pixel;

					t_pixel = (t_src_ptr[3] << 24) | (t_src_ptr[2] << 16) | (t_src_ptr[1] << 8) | (t_src_ptr[0] << 0);
					
					if (m_jpeg.saw_Adobe_marker)
						t_pixel ^= 0xFFFFFFFF;

					*t_dst_ptr++ = t_pixel;
					t_src_ptr += 4;
				}
			}
		}
	}
	
	// Now we have the bitmap, we now pass through color matching if there is
	// a profile. Note that if matching fails and input is CMYK, we must process
	// the pixel data to RGB ourselves.
	MCColorTransformRef t_transform = nil;
	if (t_success)
	{
		if (m_icc != nil)
		{
			MCColorSpaceInfo t_colorspace;
			t_colorspace . type = kMCColorSpaceEmbedded;
			t_colorspace . embedded . data = m_icc;
			t_colorspace . embedded . data_size = m_icc_length;
		
			t_transform = MCscreen -> createcolortransform(t_colorspace);
		}

		if ((t_transform == nil ||
			 !MCImageBitmapApplyColorTransform(t_frame->image, t_transform)) &&
			m_jpeg . out_color_space == JCS_CMYK)
		{
			uint8_t *t_img_ptr = (uint8_t*)t_frame->image->data;
			for(uint32_t y = 0; y < t_frame->image -> height; y++)
			{
				uint4 *dptr = (uint4 *)t_img_ptr;
				for(uint32_t x = 0; x < t_frame->image -> width; x++)
				{
					uint32_t p, k;
					p = *dptr;
					k = 255 - (p >> 24);
					uint8_t t_r, t_g, t_b;
					t_r = ((255 - (p & 0xff)) * k / 255);
					t_g = ((255 - ((p >> 8) & 0xff)) * k / 255);
					t_b = ((255 - ((p >> 16) & 0xff)) * k / 255);
					*dptr++ = MCGPixelPackNative(t_r, t_g, t_b, 255);
				}
				t_img_ptr += t_frame->image->stride;
			}
		}

		if (t_transform != nil)
			MCscreen -> destroycolortransform(t_transform);
	}

	if (t_success)
		jpeg_finish_decompress(&m_jpeg);

	if (t_row_buffer != nil)
		MCMemoryDeallocate(t_row_buffer);

    // SN-2015-07-07: [[ Bug 15569 ]] Ensure that we do not touch the image if
    //  an error occurred on decompression.
    if (t_success && m_orientation != 0)
		apply_exif_orientation(m_orientation, t_frame->image);

	if (t_success)
	{
		r_frames = t_frame;
		r_count = 1;
	}
	else
		MCImageFreeFrames(t_frame, 1);

	return t_success;
}

bool MCImageLoaderCreateForJPEGStream(IO_handle p_stream, MCImageLoader *&r_loader)
{
	MCJPEGImageLoader *t_loader;
	t_loader = new (nothrow) MCJPEGImageLoader(p_stream);
	
	if (t_loader == nil)
		return false;
	
	r_loader = t_loader;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

struct MCJPEGDestManager
{
	jpeg_destination_mgr dest;
	IO_handle stream;
	uindex_t byte_count;
	JOCTET *buffer;
};

void destmgr_init_destination(j_compress_ptr p_jpeg_info)
{
	MCJPEGDestManager *t_manager = (MCJPEGDestManager*)p_jpeg_info->dest;
	t_manager->dest.next_output_byte = t_manager->buffer;
	t_manager->dest.free_in_buffer = JPEG_BUF_SIZE;
}

boolean destmgr_empty_output_buffer(j_compress_ptr p_jpeg_info)
{
	MCJPEGDestManager *t_manager = (MCJPEGDestManager*)p_jpeg_info->dest;
	if (IO_write(t_manager->buffer,sizeof(uint1), JPEG_BUF_SIZE, t_manager->stream) != IO_NORMAL)
		ERREXIT(p_jpeg_info, JERR_FILE_WRITE);
	t_manager->byte_count += JPEG_BUF_SIZE;
	t_manager->dest.next_output_byte = t_manager->buffer;
	t_manager->dest.free_in_buffer = JPEG_BUF_SIZE;
	return TRUE;
}

void destmgr_term_destination(j_compress_ptr p_jpeg_info)
{
	MCJPEGDestManager *t_manager = (MCJPEGDestManager*)p_jpeg_info->dest;
	uindex_t t_data_count = JPEG_BUF_SIZE - t_manager->dest.free_in_buffer;
	if (t_data_count > 0)
	{
		if (IO_write(t_manager->buffer,sizeof(uint1), t_data_count, t_manager->stream) != IO_NORMAL)
			ERREXIT(p_jpeg_info, JERR_FILE_WRITE);
		t_manager->byte_count += t_data_count;
	}
}

bool MCJPEGCreateDestManager(IO_handle p_stream, MCJPEGDestManager *&r_manager)
{
	bool t_success = true;

	MCJPEGDestManager *t_manager = nil;
	t_success = MCMemoryNew(t_manager) && MCMemoryAllocate(JPEG_BUF_SIZE, t_manager->buffer);
	if (t_success)
	{
		t_manager->dest.init_destination = destmgr_init_destination;
		t_manager->dest.empty_output_buffer = destmgr_empty_output_buffer;
		t_manager->dest.term_destination = destmgr_term_destination;
		t_manager->stream = p_stream;

		r_manager = t_manager;
	}
	else
	{
		MCMemoryDelete(t_manager);
	}

	return t_success;
}

void MCJPEGFreeDestManager(MCJPEGDestManager *p_manager)
{
	if (p_manager != nil)
	{
		MCMemoryDeallocate(p_manager->buffer);
		MCMemoryDelete(p_manager);
	}
}

bool MCImageEncodeJPEG(MCImageBitmap *p_image, MCImageMetadata *p_metadata, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;

	jpeg_compress_struct t_jpeg;
	MC_jpegerr t_err;

	MCJPEGDestManager *t_dest_mgr = nil;
	JSAMPROW t_row_buffer = nil;

	t_jpeg.err = jpeg_std_error((jpeg_error_mgr*) &t_err);
	t_err.pub.error_exit = jpg_MCerr_exit;

	if (setjmp(t_err.setjmp_buffer))
	{
		t_success = false;
	}

	jpeg_create_compress(&t_jpeg);

	if (t_success)
		t_success = MCJPEGCreateDestManager(p_stream, t_dest_mgr);

	if (t_success)
	{
		t_jpeg.dest = (jpeg_destination_mgr*)t_dest_mgr;
		t_jpeg.image_width = p_image->width;
		t_jpeg.image_height = p_image->height;
		t_jpeg.input_components = 3;
		t_jpeg.in_color_space = JCS_RGB;

		jpeg_set_defaults(&t_jpeg);
		jpeg_set_quality(&t_jpeg, MCjpegquality, TRUE);
        
        if (p_metadata != nil)
        {
            if (p_metadata -> has_density)
            {
                uint16_t t_ppi = (uint16_t)p_metadata -> density;
                if (t_ppi > 0)
                {
                    t_jpeg.density_unit = 1; // dots per inch
                    t_jpeg.X_density = t_ppi;
                    t_jpeg.Y_density = t_ppi;
                }
            }
        }

		jpeg_start_compress(&t_jpeg, TRUE);
	}

	//Allocate array of pixel RGB values
	if (t_success)
		t_success = MCMemoryAllocate(p_image->width * 3, t_row_buffer);

	if (t_success)
	{
		/*write jpeg bits. jpeg_write_scanlines expects an array of pointers
		  to scanlines. Here we write one scanline at a time. 
		*/
		while (t_jpeg.next_scanline < t_jpeg.image_height)
		{
			JSAMPROW t_dst_ptr = t_row_buffer;
			uint32_t *t_src_ptr = (uint32_t*) (((uint8_t*)p_image->data) + t_jpeg.next_scanline * p_image->stride);
			uint8_t t_alpha;
			for (uindex_t x = 0; x < p_image->width; x++)
			{
				MCGPixelUnpackNative(*t_src_ptr++, t_dst_ptr[0], t_dst_ptr[1], t_dst_ptr[2], t_alpha);

				t_dst_ptr += 3;
			}

			jpeg_write_scanlines(&t_jpeg, &t_row_buffer, 1);
		}
	}

	if (t_success)
	{
		jpeg_finish_compress(&t_jpeg); //Always finish
	}

	jpeg_destroy_compress(&t_jpeg); //Free resources

	if (t_row_buffer != nil)
		MCMemoryDeallocate(t_row_buffer);

	if (t_success)
		r_bytes_written = t_dest_mgr->byte_count;

	if (t_dest_mgr != nil)
		MCJPEGFreeDestManager(t_dest_mgr);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
