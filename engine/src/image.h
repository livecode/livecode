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

//
// MCImage class
//
#ifndef	IMAGE_H
#define	IMAGE_H

#include "mccontrol.h"
#include "imagebitmap.h"
#include "graphics.h"
#include "exec.h"

#define MAG_WIDTH 8
#define MAX_PLANES 8
#define MAX_CMASK 7

typedef struct
{
	int2 y;
	int2 lx;
	int2 rx;
	int2 direction;
} MCstacktype;

typedef struct
{
	MCGImageRef image;
	int32_t xhot;
	int32_t yhot;
} MCBrush;

////////////////////////////////////////////////////////////////////////////////

bool MCImageBitmapApplyColorTransform(MCImageBitmap *p_bitmap, MCColorTransformRef p_transform);

bool MCImageQuantizeImageBitmap(MCImageBitmap *p_bitmap, MCColor *p_colors, uindex_t p_color_count, bool p_dither, bool p_add_transparency_index, MCImageIndexedBitmap *&r_indexed);
// create a new colour palette and map image pixels to palette colours
bool MCImageQuantizeColors(MCImageBitmap *p_bitmap, MCImagePaletteSettings *p_palette_settings, bool p_dither, bool p_transparency_index, MCImageIndexedBitmap *&r_indexed);

bool MCImageScaleBitmap(MCImageBitmap *p_src_bitmap, uindex_t p_width, uindex_t p_height, uint8_t p_quality, MCImageBitmap *&r_scaled);
bool MCImageRotateBitmap(MCImageBitmap *p_src, real64_t p_angle, uint8_t p_quality, uint32_t p_backing_color, MCImageBitmap *&r_rotated);
// IM-2013-11-07: [[ RefactorGraphics ]] Flip the pixels of the given image in the specified directions
void MCImageFlipBitmapInPlace(MCImageBitmap *p_bitmap, bool p_horizontal, bool p_vertical);

// Image format encode / decode function
bool MCImageEncodeGIF(MCImageBitmap *p_image, IO_handle p_stream, bool p_dither, uindex_t &r_bytes_written);
bool MCImageEncodeGIF(MCImageIndexedBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);

bool MCImageEncodeJPEG(MCImageBitmap *p_image, MCImageMetadata *p_metadata, IO_handle p_stream, uindex_t &r_bytes_written);

bool MCImageEncodePNG(MCImageBitmap *p_bitmap, MCImageMetadata *p_metadata, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageEncodePNG(MCImageIndexedBitmap *p_bitmap, MCImageMetadata *p_metadata, IO_handle p_stream, uindex_t &r_bytes_written);

bool MCImageEncodeBMP(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageDecodeBMPStruct(IO_handle p_stream, uindex_t &x_bytes_read, MCImageBitmap *&r_bitmap);

bool MCImageEncodeRawTrueColor(MCImageBitmap *p_bitmap, IO_handle p_stream, Export_format p_format, uindex_t &r_bytes_written);
bool MCImageEncodeRawIndexed(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageEncodeRawIndexed(MCImageIndexedBitmap *p_indexed, IO_handle p_stream, uindex_t &r_bytes_written);

bool MCImageEncodePBM(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageEncodePPM(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written);
bool MCImageDecodeNetPBM(IO_handle p_stream, MCImageBitmap *&r_bitmap);

// Legacy Functions
void MCImageBitmapSetAlphaValue(MCImageBitmap *p_bitmap, uint8_t p_alpha);

bool MCImageParseMetadata(MCExecContext& ctxt, MCArrayRef p_array, MCImageMetadata& r_metadata);
// MERG-2014-09-18: [[ ImageMetadata ]] Convert image metadata scruct to array
bool MCImageGetMetadata(MCExecPoint& ep, MCImageMetadata& p_metadata);

////////////////////////////////////////////////////////////////////////////////

struct MCImageCompressedBitmap
{
	uint32_t compression;
	uint8_t *data;
	uindex_t size;

	// PICT & RLE compression schemes don't encode these
	uindex_t width, height, depth;
	MCColor *colors;
	uindex_t color_count;
	uint8_t *mask;
	uindex_t mask_size;

	uint8_t **planes;
	uindex_t *plane_sizes;
};

bool MCImageCreateCompressedBitmap(uint32_t p_compression, MCImageCompressedBitmap *&r_compressed);
bool MCImageCopyCompressedBitmap(MCImageCompressedBitmap *p_src, MCImageCompressedBitmap *&r_dst);
void MCImageFreeCompressedBitmap(MCImageCompressedBitmap *p_compressed);

bool MCImageCompressRLE(MCImageBitmap *p_bitmap, MCImageCompressedBitmap *&r_compressed);
bool MCImageCompressRLE(MCImageIndexedBitmap *p_indexed, MCImageCompressedBitmap *&r_compressed);
bool MCImageDecompressRLE(MCImageCompressedBitmap *p_compressed, MCImageBitmap *&r_bitmap);

bool MCImageCompress(MCImageBitmap *p_bitmap, bool p_dither, MCImageCompressedBitmap *&r_compressed);

bool MCImageGetMetafileGeometry(IO_handle p_stream, uindex_t &r_width, uindex_t &r_height);
bool MCImageImport(IO_handle p_stream, IO_handle p_mask_stream, MCPoint &r_hotspot, MCStringRef &r_name, MCImageCompressedBitmap *&r_compressed, MCImageBitmap *&r_bitmap);
bool MCImageExport(MCImageBitmap *p_bitmap, Export_format p_format, MCImagePaletteSettings *p_palette_settings, bool p_dither, MCImageMetadata *metadata, IO_handle p_stream, IO_handle p_mask_stream);

bool MCImageDecode(IO_handle p_stream, MCBitmapFrame *&r_frames, uindex_t &r_frame_count);
bool MCImageDecode(const uint8_t *p_data, uindex_t p_size, MCBitmapFrame *&r_frames, uindex_t &r_frame_count);

bool MCImageCreateClipboardData(MCImageBitmap *p_bitmap, MCDataRef &r_data);

////////////////////////////////////////////////////////////////////////////////

#if defined(TARGET_PLATFORM_WINDOWS)
bool MCImageBitmapToDIB(MCImageBitmap *p_bitmap, MCWinSysHandle &r_dib);
bool MCImageBitmapToV5DIB(MCImageBitmap *p_bitmap, MCWinSysHandle &r_dib);
bool MCImageBitmapToMetafile(MCImageBitmap *p_bitmap, MCWinSysMetafileHandle &r_metafile);
bool MCImageBitmapToEnhancedMetafile(MCImageBitmap *p_bitmap, MCWinSysEnhMetafileHandle &r_metafile);
#elif defined(TARGET_PLATFORM_MACOS_X)
bool MCImageBitmapToPICT(MCImageBitmap *p_bitmap, MCMacSysPictHandle &r_pict);
#endif

////////////////////////////////////////////////////////////////////////////////

#include "image_rep.h"

// IM-2013-10-30: [[ FullscreenMode ]] Factor out image rep creation & preparation
// Retrieve an image rep for the given file path.
bool MCImageGetRepForFileWithStackContext(MCStringRef p_path, MCStack *p_stack, MCImageRep *&r_rep);
// Retrieve an image rep for the given reference, which may be a file path or a url.
bool MCImageGetRepForReferenceWithStackContext(MCStringRef p_reference, MCStack *p_stack, MCImageRep *&r_rep);
// Retrieve an image rep for the named resource.
bool MCImageGetRepForResource(MCStringRef p_resource_file, MCImageRep *&r_rep);

void MCImagePrepareRepForDisplayAtDensity(MCImageRep *p_rep, MCGFloat p_density);

class MCMutableImageRep : public MCImageRep
{
public:
	MCMutableImageRep(MCImage *p_owner, MCImageBitmap *p_bitmap);
	~MCMutableImageRep();

	// Image Rep interface
	MCImageRepType GetType() { return kMCImageRepMutable; }
	uindex_t GetFrameCount();
	
	bool LockBitmap(uindex_t p_index, MCGFloat p_density, MCImageBitmap *&r_bitmap);
	void UnlockBitmap(uindex_t p_index, MCImageBitmap *p_bitmap);
	
	bool LockImageFrame(uindex_t p_index, MCGFloat p_density, MCGImageFrame& r_frame);
	void UnlockImageFrame(uindex_t p_index, MCGImageFrame& p_frame);
	
	bool GetGeometry(uindex_t &r_width, uindex_t &r_height);
	// IM-2014-11-25: [[ ImageRep ]] Added ImageRep method to get frame duration.
	bool GetFrameDuration(uindex_t p_index, uint32_t &r_duration);
	
	uint32_t GetDataCompression();

	// implementation
	
	// IM-2013-12-17: [[ Bug 11604 ]] Notify image rep when the image object is moved
	void owner_rect_changed(const MCRectangle &p_new_rect);
	
	bool copy_selection(MCImageBitmap *&r_bitmap);

	Boolean image_mfocus(int2 x, int2 y);
	Boolean image_mdown(uint2 which);
	Boolean image_mup(uint2 which);
	Boolean image_doubledown(uint2 which);
	Boolean image_doubleup(uint2 which);

	bool has_selection();

	void drawsel(MCDC *dc);
	void drawselrect(MCDC *dc);

	void image_undo(Ustruct *p_undo);
	void image_freeundo(Ustruct *p_undo);

	void startdraw();
	void continuedraw();
	void enddraw();

	void canceldraw();

	void startrub();
	MCRectangle continuerub(Boolean line);
	void endrub();

	void startseldrag();
	void endsel();

	void put_brush(int2 x, int2 y, MCBrush *which);
	void fill_line(MCGRaster &plane, int2 left, int2 right, int2 y);
	bool bucket_line(MCImageBitmap *simage, uint4 color,
	                    int2 x, int2 y, int2 &l, int2 &r);
	bool bucket_point(MCImageBitmap *simage, uint4 color, MCGRaster &dimage,
	                     MCstacktype pstack[], uint2 &pstacktop,
	                     uint2 &pstackptr, int2 xin, int2 yin, int2 direction,
	                     int2 &xleftout, int2 &xrightout, bool &collide);
	void bucket_fill(MCImageBitmap *simage, uint4 scolor, MCGRaster &dimage,
	                 int2 xleft, int2 oldy);

	MCRectangle drawbrush(Tool which);
	void drawbucket();
	MCRectangle drawline(Boolean cancenter);
	MCRectangle drawreg();
	MCRectangle drawroundrect();
	MCRectangle drawoval();
	MCRectangle drawpencil();
	MCRectangle drawrectangle();

	void fill_path(MCGPathRef p_path);
	void stroke_path(MCGPathRef p_path);
	void draw_path(MCGPathRef p_path);
	void apply_stroke_style(MCGContextRef p_context, bool p_miter);
	void apply_fill_paint(MCGContextRef p_context, MCPatternRef p_pattern, const MCColor &p_color);
	void apply_stroke_paint(MCGContextRef p_context, MCPatternRef p_pattern, const MCColor &p_color);

	void battson(MCContext *ctxt, uint2 depth);

	void fillimage(const MCRectangle &drect);
	void eraseimage(const MCRectangle &drect);

	MCRectangle getopaqueregion(uint1 p_threshold = 0);
	void croptoopaque();

	void selimage();

	void getsel(Boolean cut);
	void cutoutsel();
	void stampsel();
	void rotatesel(int2 angle);
	void flipsel(Boolean ishorizontal);

	void pasteimage(MCImageBitmap *p_bitmap);

	static void init();
	static void shutdown();
    
    // MERG-2014-09-16: [[ ImageMetadata ]] Support for image metadata property
    bool GetMetadata(MCImageMetadata& r_metadata);
    
    void Lock();
    void Unlock();
    
    bool IsLocked() const;

private:
	MCImage *m_owner;
	MCGImageFrame m_gframe;
	MCImageBitmap *m_locked_bitmap;

	MCImageBitmap *m_bitmap;
	MCImageBitmap *m_selection_image;
	MCImageBitmap *m_undo_image;
	MCImageBitmap *m_rub_image;
	MCGRaster m_draw_mask;

	/* DUPE */ MCRectangle rect;

	/* DUPE */MCRectangle selrect;

	/* DUPE */uint32_t state;

	/* DUPE */int16_t mx, my;
	/* DUPE */int16_t startx, starty;

	static Boolean erasing;

	static Tool oldtool;
	static MCRectangle newrect;

	static MCPoint *points;
	static uint2 npoints;
	static uint2 polypoints;
    
    bool m_is_locked;
};

class MCImageNeed
{
public:
	MCImageNeed(MCObject *p_object);
	~MCImageNeed();

	void Add(MCImageNeed *&p_list);
	void Remove(MCImageNeed *&p_list);

	MCObject *GetObject();
	MCImageNeed *GetNext();

private:
	MCObjectHandle m_object;
	MCImageNeed *m_prev;
	MCImageNeed *m_next;
};

typedef MCObjectProxy<MCImage>::Handle MCImageHandle;

class MCImage : public MCControl, public MCMixinObjectHandle<MCImage>
{
public:
    
    enum { kObjectType = CT_IMAGE };
    using MCMixinObjectHandle<MCImage>::GetHandle;

private:
    
	friend class MCHcbmap;
	
	MCImageRep *m_rep;
	// IM-2013-11-05: [[ RefactorGraphics ]] Resampled image rep used to store cached
	// best-quality scaled image
	MCResampledImageRep *m_resampled_rep;

	// IM-2014-05-12: [[ ImageRepUpdate ]] The possible sources of the currently locked bitmap
	MCImageRep *m_locked_rep;
	MCGImageRef m_locked_image;
	MCImageBitmap *m_locked_bitmap;

	uint32_t m_image_opened;

	bool m_has_transform;
	MCGAffineTransform m_transform;
	
    MCRectangle m_center_rect;
    
	// MW-2013-10-25: [[ Bug 11300 ]] These control whether a horz/vert flip is
	//   applied to the transform on referenced images (set by the flip cmd).
	bool m_flip_x : 1;
	bool m_flip_y : 1;

	MCImageNeed *m_needs;
	
	uint32_t m_current_width;
	uint32_t m_current_height;

	int2 xhot;
	int2 yhot;
	uint2 angle;
	int2 currentframe;
	int2 repeatcount;
	int2 irepeatcount;
	uint1 resizequality;
	MCStringRef filename;
	static int2 magmx;
	static int2 magmy;
	static MCRectangle magrect;
	static MCObject *magtoredraw;

	static Boolean filledborder;
	static MCBrush brush;
	static MCBrush spray;
	static MCBrush eraser;
	static MCCursorRef cursor;
	static MCCursorRef defaultcursor;
	static uint2 cmasks[8];
    
    // MM-2014-07-31: [[ ThreadedRendering ]] Used to ensure the image animate message is only posted from a single thread.
    bool m_animate_posted : 1;
	
public:
	// replace the current image data with the new bitmap
	// IM-2013-07-19: [[ ResIndependence ]] add scale param for hi-res images
	bool setbitmap(MCImageBitmap *p_bitmap, MCGFloat p_scale, bool p_update_geometry = false);

	// MW-2013-09-05: [[ Bug 11127 ]] These are used when saving / loading an image
	//   they hold the control's colors, as the ones serialized in MCObject are the
	//   image colors (for <= 8 color RLE images).
	static bool s_have_control_colors;
	static uint16_t s_control_color_count;
	static MCColor *s_control_colors;
	static MCStringRef *s_control_color_names;
	static uint16_t s_control_pixmap_count;
	static MCPatternInfo *s_control_pixmapids;
	static uint16_t s_control_color_flags;
	
	// IM-2014-05-21: [[ HiResPatterns ]] Convert image resize quality to MCGImageFilter
	static MCGImageFilter resizequalitytoimagefilter(uint8_t p_quality);

private:
	void setrep(MCImageRep *p_rep);
	
	bool setcompressedbitmap(MCImageCompressedBitmap *p_compressed);
	bool setfilename(MCStringRef p_filename);
	bool setdata(void *p_data, uindex_t p_size);
	
	void notifyneeds(bool p_deleting);
	

	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
public:
	MCImage();
	MCImage(const MCImage &iref);
	// virtual functions from MCObject
	virtual ~MCImage();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();
	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    
    virtual bool visit_self(MCObjectVisitor *p_visitor);
    
	virtual void open();
	virtual void close();
	virtual Boolean mfocus(int2 x, int2 y);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual void applyrect(const MCRectangle &nrect);

	virtual void select();
	virtual void deselect();
	virtual void undo(Ustruct *us);
	virtual void freeundo(Ustruct *us);
	
	// MW-2011-09-20: [[ Collision ]] Compute shape of image - will use mask directly if possible.
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);
	// IM-2013-10-16: [[ ResIndependence ]] Compute the shape from the image bitmap data
	bool lockbitmapshape(const MCRectangle &p_bounds, const MCPoint &p_origin, MCObjectShape &r_shape);

	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);
	
	// MW-2012-03-28: [[ Bug 10130 ]] No-op for images as there is no font.
	virtual bool recomputefonts(MCFontRef parent_font, bool force);

	// virtual functions from MCControl
	IO_stat load(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);

	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);
	virtual Boolean maskrect(const MCRectangle &srect);

	// MW-2013-09-05: [[ UnicodifyImage ]] Returns true if the image is editable.
	bool iseditable(void) const { return MCStringIsEmpty(filename); }
	
	bool isediting() const;
	void startediting(uint16_t p_which);
	void finishediting();
	void sourcerectchanged(MCRectangle p_newrect);

	void invalidate_rep(MCRectangle &p_rect);

	bool convert_to_mutable();

	void resetimage();

	void flip(bool horz);
	
	void rotate_transform(int32_t p_angle);
	void resize_transform();
	void flip_transform();
	void apply_transform();

	uint8_t getresizequality()
	{
		return resizequality;
	}

	// IM-2014-05-21: [[ HiResPatterns ]] Return the image filter used when transforming this image
	MCGImageFilter getimagefilter()
	{
		return resizequalitytoimagefilter(resizequality);
	}

    void getcurrentcolor(MCGPaintRef& r_current_color);
    
	void setframe(int32_t p_newframe);
	void advanceframe();

	uint32_t getcompression();

	// IM-2014-05-12: [[ ImageRepUpdate ]] Returns a bitmap version of the image at the requested size.
	// IM-2024-09-02: [[ Bug 13295 ]] Replace scale param with optional target size.
	// Release the bitmap with unlockbitmap() once done with it.
	bool lockbitmap(bool p_premultiplied, bool p_update_transform, const MCGIntegerSize *p_size, MCImageBitmap *&r_bitmap);

	// get the current (transformed) image data
	bool lockbitmap(MCImageBitmap *&r_bitmap, bool p_premultiplied, bool p_update_transform = true);
	void unlockbitmap(MCImageBitmap *p_bitmap);
	
	// IM-2013-07-26: [[ ResIndependence ]] create bitmap copy of transformed image.
	// IM-2014-09-02: [[ Bug 13295 ]] Remove unused scale param.
	bool copybitmap(bool p_premultiplied, MCImageBitmap *&r_bitmap);

	// IM-2013-07-19: [[ ResIndependence ]] get image source size (in points)
	bool getsourcegeometry(uint32_t &r_pixwidth, uint32_t &r_pixheight);
	void getgeometry(uint32_t &r_pixwidth, uint32_t &r_pixheight);

	// IM-2013-11-06: [[ RefactorGraphics ]] get the image rep & transform used to render the image
	bool get_rep_and_transform(MCImageRep *&r_rep, bool &r_has_transform, MCGAffineTransform &r_transform);
	
	// IM-2013-10-30: [[ FullscreenMode ]] Returns the stack device scale or 1.0 if image object not attached
	MCGFloat getdevicescale(void);
	
	void addneed(MCObject *p_object);

	void endsel();

	// in idraw.cc
	void drawme(MCDC *dc, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy, uint2 dw, uint2 dh);
	void drawcentered(MCDC *dc, int2 x, int2 y, Boolean reverse);
    void drawnodata(MCDC *dc, uint2 sw, uint2 sh, int2 dx, int2 dy, uint2 dw, uint2 dh);

    void drawwithgravity(MCDC *dc, MCRectangle rect, MCGravity gravity);

	void canceldraw(void);
	void startmag(int2 x, int2 y);
	void endmag(Boolean close);
	void drawmagrect(MCDC *dc);
	void magredrawdest(const MCRectangle &brect);
	// MW-2012-01-05: [[ Bug 9895 ]] Magnifier redraw now targets a context, rather than fixed window.
	void magredrawrect(MCContext *context, const MCRectangle &dirty);
	Boolean magmfocus(int2 x, int2 y);
	Boolean magmdown(uint2 which);
	Boolean magmup(uint2 which);
	Boolean magdoubledown(uint2 which);
	Boolean magdoubleup(uint2 which);
	// in iutil.cc
	static void init();
	static void shutdown();
	
	void cutimage();
	void copyimage();
	void selimage();
	void delimage();
	void pasteimage(MCImage *clipimage);

	void rotatesel(int2 angle);
	void flipsel(Boolean ishorizontal);

	void compute_gravity(MCRectangle &trect, int2 &xorigin, int2 &yorigin);
	void compute_offset(MCRectangle &p_rect, int16_t &r_xoffset, int16_t &r_yoffset);
	void crop(MCRectangle *newrect);
	void createbrush(Properties which);

	static MCBrush *getbrush(Tool p_which);

	MCCursorRef createcursor();
	MCCursorRef getcursor(bool p_is_default = false);
	bool createpattern(MCPatternRef &r_pattern);
	// in ifile.cc
	Boolean noblack();
	void recompress();
	bool decompressbrush(MCGImageRef &r_brush);
	void openimage();
	void closeimage();
	void prepareimage();
	void reopen(bool p_newfile, bool p_lock_size = false);
	// in iimport.cc
	IO_stat import(MCStringRef newname, IO_handle stream, IO_handle mstream);

	// Return a shared string containing the image data in the format required for
	// publishing to the clipboard. For PNG, GIF, JPEG images this is the text 
	// property of the image. For RAW images, the data will be compressed and
	// returned as PNG.
	bool getclipboardtext(MCDataRef& r_data);
	
	// MW-2011-09-13: [[ Masks ]] Updated to return a 'MCWindowMask'
	// IM-2014-10-22: [[ Bug 13746 ]] Add size parameter to allow scaled window shapes
	MCWindowShape *makewindowshape(const MCGIntegerSize &p_size);
	
#if defined(_MAC_DESKTOP)
	CGImageRef makeicon(uint4 p_width, uint4 p_height);
	CGImageRef converttodragimage(void);
#elif defined(_WINDOWS_DESKTOP)
	MCWinSysIconHandle makeicon(uint4 p_width, uint4 p_height);
	void converttodragimage(MCDataRef& r_output);
#endif

	void set_gif(uint1 *data, uint4 length);

	//MCString getrawdata(void);
    void getrawdata(MCDataRef& r_data);
    
    // PM-2014-12-12: [[ Bug 13860 ]] Allow exporting referenced images to album
    void getimagefilename(MCStringRef &r_filename);
    bool isReferencedImage(void);
    
	MCImage *next()
	{
		return (MCImage *)MCDLlist::next();
	}

	MCImage *prev()
	{
		return (MCImage *)MCDLlist::prev();
	}

	void totop(MCImage *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}

	void insertto(MCImage *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}

	void appendto(MCImage *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}

	void append(MCImage *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}

	void splitat(MCImage *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}

	MCImage *remove(MCImage *&list)
	{
		return (MCImage *)MCDLlist::remove((MCDLlist *&)list);
	}

	////////// PROPERTY SUPPORT METHODS

	void GetTransparencyData(MCExecContext &ctxt, bool p_flatten, MCDataRef &r_data);
	void SetTransparencyData(MCExecContext &ctxt, bool p_flatten, MCDataRef p_data);
	
	////////// PROPERTY ACCESSORS

	void GetXHot(MCExecContext& ctxt, integer_t& r_x);
	void SetXHot(MCExecContext& ctxt, integer_t p_x);
	void GetYHot(MCExecContext& ctxt, integer_t& r_y);
	void SetYHot(MCExecContext& ctxt, integer_t p_y);
	void GetHotSpot(MCExecContext& ctxt, MCPoint& r_spot);
	void SetHotSpot(MCExecContext& ctxt, MCPoint p_spot);
	void GetFileName(MCExecContext& ctxt, MCStringRef& r_name);
	void SetFileName(MCExecContext& ctxt, MCStringRef p_name);
	void GetAlwaysBuffer(MCExecContext& ctxt, bool& r_setting);
	void SetAlwaysBuffer(MCExecContext& ctxt, bool setting);
	void GetImagePixmapId(MCExecContext& ctxt, uinteger_t*& r_id);
	void SetImagePixmapId(MCExecContext& ctxt, uinteger_t* p_id);
	void GetMaskPixmapId(MCExecContext& ctxt, uinteger_t*& r_id);
	void SetMaskPixmapId(MCExecContext& ctxt, uinteger_t* p_id);
	void GetDontDither(MCExecContext& ctxt, bool& r_setting);
	void SetDontDither(MCExecContext& ctxt, bool setting);
	void GetMagnify(MCExecContext& ctxt, bool& r_setting);
	void SetMagnify(MCExecContext& ctxt, bool setting);
	void GetSize(MCExecContext& ctxt, uinteger_t& r_size);
	void GetCurrentFrame(MCExecContext& ctxt, uinteger_t& r_frame);
	void SetCurrentFrame(MCExecContext& ctxt, uinteger_t p_frame);
	void GetFrameCount(MCExecContext& ctxt, integer_t& r_count);
	void GetPalindromeFrames(MCExecContext& ctxt, bool& r_setting);
	void SetPalindromeFrames(MCExecContext& ctxt, bool setting);
	void GetConstantMask(MCExecContext& ctxt, bool& r_setting);
	void SetConstantMask(MCExecContext& ctxt, bool setting);
	void GetRepeatCount(MCExecContext& ctxt, integer_t& r_count);
	void SetRepeatCount(MCExecContext& ctxt, integer_t p_count);
	void GetFormattedHeight(MCExecContext& ctxt, integer_t& r_height);
	void GetFormattedWidth(MCExecContext& ctxt, integer_t& r_width);
	void GetText(MCExecContext& ctxt, MCDataRef& r_text);
	void SetText(MCExecContext& ctxt, MCDataRef p_text);
	void GetImageData(MCExecContext& ctxt, MCDataRef& r_data);
    void SetImageData(MCExecContext& ctxt, MCDataRef p_data);
	void GetMaskData(MCExecContext& ctxt, MCDataRef& r_data);
	void SetMaskData(MCExecContext& ctxt, MCDataRef p_data);
	void GetAlphaData(MCExecContext& ctxt, MCDataRef& r_data);
	void SetAlphaData(MCExecContext& ctxt, MCDataRef p_data);
	void GetResizeQuality(MCExecContext& ctxt, intenum_t& r_quality);
	void SetResizeQuality(MCExecContext& ctxt, intenum_t p_quality);
	void GetPaintCompression(MCExecContext& ctxt, intenum_t& r_compression);
	void GetAngle(MCExecContext& ctxt, integer_t& r_angle);
	void SetAngle(MCExecContext& ctxt, integer_t p_angle);
    // SN-2014-06-23: [[ IconGravity ]] Getters and setters added
    void SetCenterRectangle(MCExecContext& ctxt, MCRectangle *p_rectangle);
    void GetCenterRectangle(MCExecContext& ctxt, MCRectangle *&r_rectangle);
    void GetMetadataProperty(MCExecContext& ctxt, MCNameRef p_prop, MCExecValue& r_value);
    
    virtual void SetBlendLevel(MCExecContext& ctxt, uinteger_t level);
	virtual void SetInk(MCExecContext& ctxt, intenum_t ink);
    virtual void SetVisible(MCExecContext& ctxt, uinteger_t part, bool setting);
};

extern bool MCU_israwimageformat(Export_format p_format);

#endif
