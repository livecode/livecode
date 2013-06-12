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

/*

  File Description  X11 Context implimentation
  Author : Tim Shields
  Date : 21/06/07
  
*/


#ifndef X11CONTEXT_H
#define X11CONTEXT_H


#include "typedefs.h"
#include "context.h"
#include "path.h"
#include "lnxdc.h"

enum X11ShmType
{
	SHM_SURFACE ,
	SHM_MASK
};

class MCOldFontlist;
class MCNewFontlist;

class MCX11Context : public MCContext
{
	enum
	{
		FLAG_MASK_CHANGED = 1 << 16,
		FLAG_MASK_OPAQUE = 1 << 17,
		FLAG_MASK_ACTIVE = 1 << 18,
		
		FLAG_FILL_READY = 1 << 19,
		FLAG_STROKE_READY = 1 << 20,
		
		// Used to determine if text needs to do alpha capture.
		FLAG_IS_OPAQUE = 1 << 24,

		FLAG_IS_PRINTER = 1 << 25,
		FLAG_IS_TRANSIENT = 1 << 26,
		FLAG_IS_MEMORY = 1 << 27,
		FLAG_IS_WINDOW = 1 << 28,
		FLAG_IS_ALPHA = 1 << 29,
		
		FLAG_HAS_CLIP = 1 << 30,
		FLAG_HAS_MASK = 1 << 31,
	};
	
	Boolean opened;
	Pixmap graystipple;
	
	
public:
	MCX11Context(void);
	~MCX11Context(void);
	
	// MW-2009-06-10: [[ Bitmap Effects ]]
	void begin(bool p_group);
	bool begin_with_effects(MCBitmapEffectsRef effects, const MCRectangle& shape);
	void end(void);	
	
	MCContextType gettype(void) const; 
	
	bool changeopaque(bool p_new_value);
	void setprintmode(void);

	void setclip(const MCRectangle& rect);
	const MCRectangle& getclip(void) const; 
	void clearclip(void); 
	
	void setorigin(int2 x, int2 y);
	void clearorigin(void);
	
	void setquality(uint1 quality);
	
	void setfunction(uint1 function);
	uint1 getfunction(void);
	
	void setopacity(uint1 opacity);
	uint1 getopacity(void);
	
	void setfont(const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font);
	void setforeground(const MCColor& c);
	void setbackground(const MCColor& c);
	void setdashes(uint2 offset, const uint1 *dashes, uint2 ndashes);
	void setfillstyle(uint2 style, Pixmap p, int2 x, int2 y);
	void getfillstyle(uint2& style, Pixmap& p, int2& x, int2& y);
	void setlineatts(uint2 linesize, uint2 linestyle, uint2 capstyle, uint2 joinstyle);
	void setgradient(MCGradientFill *p_gradient);
	
	void drawline(int2 x1, int2 y1, int2 x2, int2 y2);
	void drawlines(MCPoint *points, uint2 npoints, bool p_closed = false);
	void drawsegments(MCSegment *segments, uint2 nsegs);
	void drawtext(int2 x, int2 y, const char *s, uint2 length, MCFontStruct *f, Boolean image, bool p_unicode_override = false);
	void drawrect(const MCRectangle& rect);
	void fillrect(const MCRectangle& rect);
	void fillrects(MCRectangle *rects, uint2 nrects);
	void fillpolygon(MCPoint *points, uint2 npoints);
	void drawroundrect(const MCRectangle& rect, uint2 radius);
	void fillroundrect(const MCRectangle& rect, uint2 radius);
	void drawarc(const MCRectangle& rect, uint2 start, uint2 angle);
	void drawsegment(const MCRectangle& rect, uint2 start, uint2 angle);
	void fillarc(const MCRectangle& rect, uint2 start, uint2 angle);
	
	void drawpath(MCPath *path);
	void fillpath(MCPath *path, bool p_evenodd = true);
	void setmiterlimit(real8 p_limit);
	
	void drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect);
	void draweps(real8 sx, real8 sy, int2 angle, real8 xscale, real8 yscale, int2 tx, int2 ty,
		const char *prolog, const char *psprolog, uint4 psprologlength, const char *ps, uint4 length,
		const char *fontname, uint2 fontsize, uint2 fontstyle, MCFontStruct *font, const MCRectangle& trect);
	void drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy);
	
	void drawlink(const char *link, const MCRectangle& region);

	int4 textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override = false); 
	
	void applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height); 
	
	void drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_info);
	void copyarea(Drawable p_src, uint4 p_dx, uint4 p_dy, uint4 p_sx, uint4 p_sy, uint4 p_sw, uint4 p_sh); 
	void x_copyarea(Drawable s, Drawable d, int2 depth, int2 sx, int2 sy,  uint2 sw, uint2 sh, int2 dx, int2 dy, uint4 rop);
	
	void combine(Pixmap p_src, int4 p_dx, int4 p_dy, int4 p_sx, int4 p_sy, uint4 p_sw, uint4 p_sh); 
	
	MCBitmap *lock(void); 
	void unlock(MCBitmap *); 
	
	MCRegionRef computemaskregion(void);
	void clear(const MCRectangle *rect);
	
	uint2 getdepth(void) const;
	const MCColor& getblack(void) const;
	const MCColor& getwhite(void) const;
	const MCColor& getgray(void) const;
	const MCColor& getbg(void) const;
	
	void mergealpha(void);
	
	static MCX11Context *create_context(Drawable p_drawable, uint2 w, uint2 h, Display *p_display, bool p_wantsalpha );
	static MCX11Context *create_memory_context(uint32_t width, uint32_t height, Display *display, bool with_alpha);
	
	Drawable get_surface(void) ;
	int4 	 get_origin_x(void);
	int4 	 get_origin_y(void);
	GC		 get_gc(void);
	
	void 	 mask_rect_area ( MCRectangle *rect ) ;
	void 	 *extract_alpha_from_two_pixmaps ( Drawable *p_b_pm , Drawable *p_w_pm, uint4 p_w, uint4 p_h );

	MCBitmap *lock_bits(void*& r_bits, uint32_t& r_stride);
	void unlock_bits(MCBitmap *bitmap);

private:
	struct Layer
	{
		Layer *parent;
		uint4 id ;
		MCRectangle clip;
		MCPoint origin;
		int4 width;
		int4 height;
		uint1 function;
		uint1 opacity;
		uint4 nesting;
		Boolean hasAlpha ;
		MCBitmap *combiner_image ;
		
		Drawable shmsurface ; // Used to debug the SHM stuff...
		Pixmap surface;		// This is the surface we will be drawing to
		// MW-2012-04-18: [[ Bug ]] If true (and shmpixmap is false) then it means
		//   'surface' is owned by the layer and should be freed.
		bool ownsurface;
		bool ownAlpha ;		// This marks if we own our own alpha channel
		MCBitmap *alpha ;    // This is a client side 8 bit surface for drawing the alpha channel stuff to. It is	
							// blended with the Serverside (fetched) xRGB buffer to fill in the <x> part giving a 
							// full ARGB buffer to then work from.
		
		XShmSegmentInfo *shminfo ;
		void  *shmdata ;
		uint4 shm_bytes_per_line ;
		bool  shmpixmap ;
		bool  shmmask ;
		
		// MW-2009-06-11: [[ Bitmap Effects ]] OS X implementation of layer with effects.
		MCBitmapEffectsRef effects;
		MCRectangle effects_shape;
	};		
	
	Layer 	*m_layers;
	Display *m_display;
	
	
	Pixmap  m_mask;   	// This is the 1-bit alpha mask --- all draw operations ALSO go into here
	MCRectangle dirty_mask_area ;	// This will keep track of the part of the mask that needs to be flushed.

	// MW-2010-07-14: Make sure we keep track of the current mask's width/height
	//   as we may have to resize it due to layer effects.
	int32_t m_mask_width, m_mask_height;
	
	
	uint1 f_quality;
	
	GC 		m_gc ;		// Native screen depth GC
	GC		m_gc1 ;		// 1-Bit mask depth GC
	GC 		m_gc32 ;	// Internal Rendering depth GC
	
	int4 f_width;
	int4 f_height;
	
	bool isMemoryContext ;
	

	uint4  m_bgcolor ;
	
	uint4 f_flags;
	
	struct
	{
		uint2 style;
		uint4 colour;
		Pixmap pattern;
		MCPoint origin;
		uint4 x ;
		uint4 y;
		MCBitmap *pattern_image ;
	} f_fill;
	
	MCStrokeStyle f_stroke;
	
	MCFontStruct *f_font;
	
	MCGradientFill *f_gradient_fill;

	bool getflags(uint4 p_flag) const;
	void setflags(uint4 p_flag, bool p_state);
	void changeflags(uint4 p_mask, uint4 p_value);
	
	MCCombiner *combiner_lock(void);
	void combiner_unlock(MCCombiner *);
	
	MCPoint * adjustPoints ( MCPoint *n_points, MCPoint *points, uint2 npoints, MCRectangle *p_bounds );
	MCRectangle * adjustRects ( MCRectangle *n_rects, MCRectangle *rects, uint2 nrects, MCRectangle *p_bounds  );
	MCSegment * adjustSegs(MCSegment *n_segs, MCSegment *segs, uint2 nsegs, MCRectangle *p_bounds );

	
public:
	void flush_mask(void);

private:
	Layer *layer_create(const MCRectangle& p_new_clip);
    Layer *layer_create_with_parameters(uint4 p_width, uint4 p_height, bool p_transient, bool p_needs_alpha, bool p_createsurface);

	
	MCBitmap *lock_layer ( uint1 *&p_data_image, uint4 & p_data_stride, Layer *t_layer, int4 p_x, int4 p_y,  int4 p_width, int4 p_height  ) ;
	void unlock_layer ( void *p_image_ptr, uint4 p_image_stride , Layer *p_layer, int4 p_x, int4 p_y,  int4 p_width, int4 p_height  );
	void clear_surfaces ( Layer *p_layer, bool preserve_contents ) ;

	bool is_a_nest(bool p_group);
	bool mask_changed(void);
	void clear_mask(void);
	void update_mask( int4 x, int4 y, int4 w, int4 h ) ;
	

	// Theme drawing stuff
	MCBitmap 	* drawtheme_calc_alpha ( MCThemeDrawInfo &p_info);
	MCBitmap 	* calc_alpha_from_bitmaps ( MCBitmap *t_bm_black, MCBitmap * t_bm_white, uint1 rh = 0 , uint1 bh = 0 , uint1 gh = 0 ) ;
	void		black_and_white_masks ( Drawable t_black, Drawable t_white ) ;
	bool 		want_painted_text (void) ;
public:
	void 		drawalphaimage_direct ( MCBitmap *p_data, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy ) ;

	
public:	
	// Shared memory functions
	
	Drawable createpixmap_shm (Layer *p_layer, int4 p_width, int4 p_height, uint4 p_depth );
	uint1*	 getimage_shm ( Layer *p_layer, uint4 & p_image_stride, Drawable d, int4 x, int4 y, int4 w, int4 h ) ;
	void 	 destroypixmap_shm ( Layer *p_layer ) ; 
	void 	 putimage_shm ( Layer *p_layer, Drawable p_d, MCBitmap *p_image, int4 p_sx, int4 p_sy, int4 p_dx, int4 p_dy, int4 p_w, int4 p_h );

	Layer  *get_top_layer ( void ) { return ( m_layers ) ; } ;
	void 	map_alpha_data ( MCBitmap * p_new_alpha ) ;

	void 	fillrect_with_native_function(const MCRectangle& rect, int function);

private:
	friend class MCOldFontlist;
	friend class MCNewFontlist;
} ;



inline bool MCX11Context::getflags(uint4 p_flags) const
{
	return ((f_flags & p_flags) != 0);
}

inline void MCX11Context::setflags(uint4 p_flags, bool p_state)
{
	changeflags(p_flags, p_state ? p_flags : 0);
}

inline void MCX11Context::changeflags(uint4 p_mask, uint4 p_value)
{
	f_flags = (f_flags & ~p_mask) | p_value;
}


#endif
