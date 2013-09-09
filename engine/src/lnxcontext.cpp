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

  File Desc : X11 Context
  Author : Tim SHields
  Date : 21-06-2007
  
  Info:  Most of the stuff here has been moved out of xdc.cpp etc and moved into here. 
  
  */

#include "lnxprefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "filedefs.h"
#include "execpt.h"
#include "util.h"

#include "globals.h"
#include "path.h"

#include "filedefs.h"
#include "stack.h"
#include "gradient.h"
#include "paint.h"
#include "bitmapeffect.h"

#include "lnxdc.h"
#include "lnxcontext.h"
#include "lnximagecache.h"
#include "lnxtheme.h"
#include "lnxgtkthemedrawing.h"

#include <gdk/gdkx.h>

#include <sys/shm.h>

typedef void (*surface_combiner_t)(void *p_dst, uint4 p_dst_stride, void *p_src, uint4 p_src_stride, uint4 p_width, uint4 p_height, uint1 p_opacity);
extern surface_combiner_t s_surface_combiners[];
extern surface_combiner_t s_surface_combiners_nda[];
extern void surface_merge(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height);
extern void surface_merge_with_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);
extern void surface_merge_with_alpha_non_pre(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
extern void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
extern void surface_merge_with_alpha_and_mask(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);
uint4 g_current_background_colour = 0x000000;
extern void surface_extract_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
extern void surface_unmerge_pre(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height);

void surface_merge_with_alpha_and_mask(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);
  
extern char * rgbtostr ( uint4 red, uint4 green, uint4 blue ) ;



extern gint moz_gtk_widget_paint(GtkThemeWidgetType widget, GdkDrawable * drawable,
            			         GdkRectangle * rect, GdkRectangle * cliprect,
                     			GtkWidgetState * state, gint flags);

// Constructor  
MCX11Context::MCX11Context()
{
	memset(((uint1 *)this) + sizeof(MCContext), 0, sizeof(MCX11Context) - sizeof(MCContext));
	
}  


// Destructor
MCX11Context::~MCX11Context()
{
	Layer *t_layer ;
	
	uint2 t_width;
	uint2 t_height;
	uint2 t_depth;
	MCscreen -> getpixmapgeometry(m_layers -> surface, t_width, t_height, t_depth);
	
	if (t_depth != 1)
		mergealpha();
	
	if (!m_layers -> ownAlpha)
	{
		flush_mask();
	}
		
	// free the base layer ( each compound layer is free'd in MCX11Context::end() )
	if ( m_layers != NULL ) 
	{
		t_layer = m_layers ;

		if ( t_layer -> alpha != NULL && t_layer -> ownAlpha ) 
			MCscreen -> destroyimage ( t_layer -> alpha ) ;

		// MW-2012-04-18: [[ Bug ]] Check to see if the context owns the (non-shm) surface
		//   before trying to delete it.
		if ( t_layer -> shmpixmap ) 
			destroypixmap_shm ( t_layer ) ;
		else if (t_layer -> ownsurface)
			MCscreen -> freepixmap(t_layer -> surface);
	
		free ( t_layer -> shminfo ) ;
		t_layer -> shminfo = NULL ;

		if ( t_layer -> combiner_image != NULL ) 
			MCscreen -> destroyimage ( t_layer -> combiner_image );
		
		delete t_layer ;
		
	}

    if ( f_fill . pattern_image != NULL )
    {
        MCscreen -> destroyimage ( f_fill . pattern_image ) ;
        f_fill.pattern_image = NULL;
	}
	
	XFreeGC( m_display, m_gc32 ) ;
	XFreeGC( m_display, m_gc );
	XFreeGC( m_display, m_gc1 ) ;
	XFreePixmap ( m_display, m_mask ) ;
	
	delete f_stroke . dash . data;
}

/*--===========================================================--

	         M A S K     R O U T I N E S 

--===========================================================-- */


void MCX11Context::update_mask( int4 x, int4 y, int4 w, int4 h ) 
{
	assert(w >= 0);
	assert(h >= 0);
	
	if (x + w < 0)
		return;
	
	if (y + h < 0)
		return;
	
	if (x > m_layers -> clip . x + m_layers -> clip . width - m_layers -> origin . x)
		return;
	
	if (y > m_layers -> clip . y + m_layers -> clip . height - m_layers -> origin . y)
		return;
	
	
	MCRectangle t_rect ;
	
	
	// We need to find the intersection of the requested mask area and the current clipping area.
	
	// The mask rect is in layer co-ords, but the layer is in stack co-ords.
	// Convert the mask to stack co-ords.
	t_rect . x =  x + m_layers -> origin . x ;
	t_rect . y = y + m_layers -> origin . y ;
	t_rect . width = w ;
	t_rect . height = h ;
	
	t_rect = MCU_intersect_rect ( t_rect, m_layers -> clip ) ;
	
	// Convert the mask back into layer co-ords.
	t_rect . x -= m_layers -> origin . x ;
	t_rect . y -= m_layers -> origin . y ;

	if ( mask_changed() ) 
	    dirty_mask_area = MCU_union_rect (  dirty_mask_area, t_rect );
	else
	{
		dirty_mask_area . x = t_rect . x ;
		dirty_mask_area . y = t_rect . y ;
		dirty_mask_area . width = t_rect . width ;
		dirty_mask_area . height = t_rect . height ;
	}
}


void MCX11Context::clear_mask(void)
{
	
	
	XSetFunction ( m_display, m_gc1, GXclear);
	XCopyArea(m_display, m_mask, m_mask, m_gc1, 0, 0, m_mask_width, m_mask_height, 0, 0);
	XSetFunction( m_display, m_gc1, GXor);	
	
	dirty_mask_area.x = 0 ;
	dirty_mask_area.y = 0 ;
	dirty_mask_area.width =0 ;
	dirty_mask_area.height = 0 ;
	
}




bool MCX11Context::mask_changed()
{
	return ( dirty_mask_area . width != 0 && dirty_mask_area . height != 0 ) ;
}


void MCX11Context::flush_mask(void)
{

	Layer *t_src_layer;
	uint1 *t_pixel_ptr;
	uint1 *t_mask_ptr;
	
	int4 p_w, p_h ;

	uint4 t_mask_stride;
	uint4 t_pixel_stride;
	MCBitmap *t_mask_image ;

	// If the mask has not changed (i.e. the dirty region = (0,0,0,0) then we dont need to flush it
	if ( !mask_changed() )
		return ;	
	

	t_src_layer = m_layers;
	
	// Grab the server copy if the mask image so we can manipulate it by hand.
	// As the mask is only applicable to the current layer, we can assume it has the same
	// origin as the layer that we are flushing...
	// However, now we are using a dirty region to work out what (if any) of the mask
	// has been changed - so grab that.

	
	t_mask_image = MCscreen -> getimage ( m_mask, 
										  dirty_mask_area . x ,
										  dirty_mask_area . y ,
										  dirty_mask_area . width, 
										  dirty_mask_area . height ) ;

	
	assert ( t_mask_image != DNULL) ;
	
	t_pixel_stride = t_src_layer -> alpha -> bytes_per_line ;
	t_mask_stride =  t_mask_image -> bytes_per_line ; 

	p_w = dirty_mask_area . width ; 
	p_h = dirty_mask_area . height ;

	t_pixel_ptr = (uint1 *)t_src_layer -> alpha -> data ;
	t_mask_ptr = (uint1 *)t_mask_image -> data ;
	
	// Need to update the pointer to the correct part of the ALPHA mask for the given dirty mask region.
	t_pixel_ptr = (uint1*)t_pixel_ptr + ( dirty_mask_area . y * t_pixel_stride ) + dirty_mask_area . x ;
	
	for(uint4 y = p_h; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_mask_ptr += t_mask_stride)
	{
		uint4 x;

		uint1 *t_pixels = t_pixel_ptr;
		uint1 *t_mskels = t_mask_ptr;

		for(x = p_w; x >= 8; x -= 8)
		{
			uint1 b = *t_mskels;
			*t_mskels++ = 0;

			if ((b & (1 << 7)) != 0) t_pixels[0] = 0xFF;
			if ((b & (1 << 6)) != 0) t_pixels[1] = 0xFF;
			if ((b & (1 << 5)) != 0) t_pixels[2] = 0xFF;
			if ((b & (1 << 4)) != 0) t_pixels[3] = 0xFF;
			if ((b & (1 << 3)) != 0) t_pixels[4] = 0xFF;
			if ((b & (1 << 2)) != 0) t_pixels[5] = 0xFF;
			if ((b & (1 << 1)) != 0) t_pixels[6] = 0xFF;
			if ((b & (1 << 0)) != 0) t_pixels[7] = 0xFF;

			t_pixels += 8;
		}

		if (x == 0)
			continue;

		uint1 b = *t_mskels;
		*t_mskels = 0;

		switch(7 - x)
		{
		case 0: if ((b & (1 << 1)) != 0) t_pixels[6] = 0xFF;
		case 1: if ((b & (1 << 2)) != 0) t_pixels[5] = 0xFF;
		case 2:	if ((b & (1 << 3)) != 0) t_pixels[4] = 0xFF;
		case 3: if ((b & (1 << 4)) != 0) t_pixels[3] = 0xFF;
		case 4: if ((b & (1 << 5)) != 0) t_pixels[2] = 0xFF;
		case 5: if ((b & (1 << 6)) != 0) t_pixels[1] = 0xFF;
		case 6: if ((b & (1 << 7)) != 0) t_pixels[0] = 0xFF;
		default:
			break;
		}
	}

	MCscreen -> destroyimage(t_mask_image);
	
	clear_mask();
	
	
#ifndef SYNCMODE
	XSync ( m_display, False ) ;
#endif
	
}


	

MCX11Context::Layer *MCX11Context::layer_create_with_parameters(uint4 p_width, uint4 p_height, bool p_transient, bool p_needs_alpha, bool p_createsurface)
{
	Layer *t_layer;
	t_layer = new Layer;

	t_layer -> hasAlpha = p_needs_alpha ;
	t_layer -> parent = NULL;
	t_layer -> origin . x = 0;
	t_layer -> origin . y = 0;
	t_layer -> clip . x = 0;
	t_layer -> clip . y = 0;
	t_layer -> clip . width = p_width;
	t_layer -> clip . height = p_height;
	t_layer -> width = p_width;
	t_layer -> height = p_height;
	t_layer -> function = GXcopy;
	t_layer -> opacity = 255;
	t_layer -> nesting = 0;
	t_layer -> surface = DNULL ;
	t_layer -> shmsurface = DNULL ;
	t_layer -> shmpixmap = False ;
	t_layer -> combiner_image = NULL ;
	t_layer -> effects = NULL;
	t_layer -> shminfo = (XShmSegmentInfo *)malloc( sizeof ( XShmSegmentInfo ) ) ;
	memset(t_layer -> shminfo, 0 , sizeof(XShmSegmentInfo) );
	
	// MW-2012-04-18: [[ Bug ]] Set the 'ownsurface' var appropriately.
	if ( p_createsurface ) 
	{
		t_layer -> surface = createpixmap_shm ( t_layer, p_width, p_height, ((MCScreenDC*)MCscreen) -> getdepth() ) ;
		t_layer -> ownsurface = true;
	}
	else
		t_layer -> ownsurface = false;

	
	t_layer -> alpha = MCscreen -> createimage ( 8, p_width, p_height, true, 0, False, False );	// Client side image map
	t_layer -> ownAlpha = true ; // We own this alpha channel
	// Make sure that the alpha layer starts off totally transparent.
	memset( (char*)t_layer -> alpha -> data, p_needs_alpha ? 0x00 : 0xff, ( t_layer -> alpha -> bytes_per_line * t_layer -> alpha -> height ) ) ;
	
	// MW-2010-07-14: Make sure the mask is big enough
	if (m_mask != None && (m_mask_width < p_width || m_mask_height < p_height))
	{
		MCscreen -> freepixmap(m_mask);

		if (p_width > m_mask_width)
			m_mask_width = p_width;
		if (p_height > m_mask_height)
			m_mask_height = p_height;
		m_mask = MCscreen -> createpixmap(m_mask_width, m_mask_height, 1, False);
		clear_mask();
	}

	assert ( t_layer -> alpha != DNULL ) ;
	
	return t_layer;
}





MCX11Context::Layer *MCX11Context::layer_create(const MCRectangle& p_new_clip)
{
	assert(m_layers != NULL);
	
	Layer *t_layer;
			
	t_layer = layer_create_with_parameters(p_new_clip . width, p_new_clip . height, true, true, true);
	if (t_layer != NULL)
	{
		t_layer -> parent = m_layers;
		t_layer -> origin . x = p_new_clip . x;
		t_layer -> origin . y = p_new_clip . y;
		t_layer -> clip = p_new_clip;
	}
	
	return t_layer;
}

bool MCX11Context::is_a_nest(bool p_group)
{
	return ( !p_group &&
			 m_layers -> opacity == 255 && 
		     ( m_layers -> function == GXcopy));
}



void MCX11Context::begin( bool p_group)
{
	
	if (is_a_nest(p_group))
	{
		m_layers -> nesting += 1;
		return;
	}

	// Flush the mask into the ALPHA mask
	flush_mask();
	
	Layer *t_layer;
	t_layer = layer_create(m_layers -> clip);
	if (t_layer == NULL)
	{
		m_layers -> nesting += 1;
		return;
	}	
	
	m_layers = t_layer;

	setclip(m_layers -> clip);

	// Clear out the newly created surfaces (and the context draw mask);
	clear_surfaces ( t_layer ) ;
	
}

// MW-2009-06-11: [[ Bitmap Effects ]] OS X implementation of layer with effects.
bool MCX11Context::begin_with_effects(MCBitmapEffectsRef p_effects, const MCRectangle& p_shape)
{
	// Flush the mask into the ALPHA mask
	flush_mask();

	// Compute what region of the shape is required to correctly render
	// the full clip of the current layer.
	MCRectangle t_layer_clip;
	MCBitmapEffectsComputeClip(p_effects, p_shape, getclip(), t_layer_clip);

	if (t_layer_clip . width == 0 || t_layer_clip . height == 0)
		return false;

	Layer *t_new_layer;
	t_new_layer = layer_create(t_layer_clip);
	if (t_new_layer == NULL)
	{
		m_layers -> nesting += 1;
		return true;
	}
	
	// Set the effect parameters
	t_new_layer -> effects = p_effects;
	t_new_layer -> effects_shape = p_shape;
	
	// Link it into the chain
	m_layers = t_new_layer;

	// Set the clip properly
	setclip(m_layers -> clip);

	// Clear out the newly created surfaces (and the context draw mask);
	clear_surfaces ( t_new_layer ) ;

	return true;
}

void MCX11Context::end ( void ) 
{
	
   Layer *t_src_layer, *t_dst_layer;
	
	t_src_layer = m_layers;
	t_dst_layer = m_layers -> parent;

	if (t_src_layer -> nesting > 0)
	{
		t_src_layer -> nesting -= 1;
		return;
	}
	
	// Sanity check --- make sure we are not using the same layer for source and destination!
	assert ( t_src_layer != t_dst_layer);
	
	uint1 *t_src_ptr;
	uint4 t_src_stride;
	
	uint1 *t_dst_ptr; 
	uint4 t_dst_stride;
	
	MCBitmap *t_dest_image ;
	MCBitmap *t_src_image ;
	
	int4 x1, y1, x2, y2 ;
	x1 = t_src_layer -> origin . x ;
	y1 = t_src_layer -> origin . y ;
	x2 = t_dst_layer -> origin . x ;
	y2 = t_dst_layer -> origin . y ;

	// tx & ty are the origin of the SOURCE in the DESTINATION image co-ordinate system.
	int4 tx, ty, tw, th;
	tx = ( t_dst_layer -> clip . x - t_dst_layer -> origin . x ) ;
	ty = ( t_dst_layer -> clip . y - t_dst_layer -> origin . y ) ;
	tw = t_dst_layer -> clip . width;
	th = t_dst_layer -> clip . height;
	
	flush_mask();
	
	t_dest_image = lock_layer ( t_dst_ptr, t_dst_stride, t_dst_layer, tx, ty, tw, th ) ;
	t_src_image = lock_layer ( t_src_ptr, t_src_stride, t_src_layer, 0, 0, t_src_layer-> width, t_src_layer -> height ) ;
	
	if ( t_dest_image != NULL ) 
	{
		t_dst_ptr = (uint1*)t_dest_image -> data ;
		t_dst_stride = t_dest_image -> bytes_per_line ;
	}
	
	if ( t_src_image != NULL )
	{
		t_src_ptr = (uint1*)t_src_image -> data ;
		t_src_stride = t_src_image -> bytes_per_line ;
	}
	
	
	// MW-2009-06-11: [[ Bitmap Effects ]] If we have effects to apply, hand off layer
	//   compositing.
	if (t_src_layer -> effects == NULL)
	{
		surface_combiner_t t_combiner;
	
		t_combiner = m_layers -> parent == NULL ? s_surface_combiners_nda[t_dst_layer -> function] : s_surface_combiners[t_dst_layer -> function];
		t_combiner(t_dst_ptr, t_dst_stride, t_src_ptr, t_src_stride, tw, th, t_dst_layer -> opacity);
	}
	else
	{
		// For now, if opacity is not 100%, or we have a non-srcOver function then we must
		// use a temporary buffer.
		void *t_tmp_bits;
		uint32_t t_tmp_stride;
		if (t_dst_layer -> opacity != 255 || (t_dst_layer -> function != GXblendSrcOver && t_dst_layer -> function != GXcopy))
		{
			t_tmp_bits = malloc(t_dst_layer -> clip . width * t_dst_layer -> clip . height * sizeof(uint4));
			t_tmp_stride = t_dst_layer -> clip . width * sizeof(uint4);
			memset(t_tmp_bits, 0, t_dst_layer -> clip . width * t_dst_layer -> clip . height * sizeof(uint4));
		}
		else
			t_tmp_bits = NULL, t_tmp_stride = 0;
		
		MCBitmapEffectLayer t_dst;
		t_dst . bounds = t_dst_layer -> clip;
		if (t_tmp_bits == NULL)
		{
			t_dst . stride = t_dst_stride;
			t_dst . bits = (uint1 *)t_dst_ptr;
			t_dst . has_alpha = m_layers -> parent != NULL;
		}
		else
		{
			t_dst . stride = t_tmp_stride;
			t_dst . bits = t_tmp_bits;
			t_dst . has_alpha = true;
		}
		
		MCBitmapEffectLayer t_src;
		MCU_set_rect(t_src . bounds, t_src_layer -> origin . x, t_src_layer -> origin . y, t_src_layer -> width, t_src_layer -> height);
		t_src . stride = t_src_stride;
		t_src . bits = t_src_ptr;
		t_src . has_alpha = true;
		
		MCBitmapEffectsRender(t_src_layer -> effects, t_src_layer -> effects_shape, t_dst, t_src);
		
		if (t_tmp_bits != NULL)
		{
			surface_combiner_t t_combiner;
			t_combiner = m_layers -> parent != NULL ? s_surface_combiners[t_dst_layer -> function] : s_surface_combiners_nda[t_dst_layer -> function];
			t_combiner(t_dst_ptr, t_dst_stride,
					   t_tmp_bits, t_tmp_stride, t_dst_layer -> clip . width, t_dst_layer -> clip . height, t_dst_layer -> opacity);
			free(t_tmp_bits);
		}		
	}

	// Now we "unlock" the layer - This means we extract the alpha back into our local plane buffer and destroy the image.
	unlock_layer ( t_dst_ptr, t_dst_stride, t_dst_layer, tx, ty, tw, th ) ;
	unlock_layer ( t_src_ptr, t_src_stride, t_src_layer, 0, 0, t_src_layer -> width, t_src_layer -> height ) ;
	
	// And finally, dump the created image back to the XServer for display.
	if ( t_dest_image != NULL)
	{
		MCscreen -> putimage ( t_dst_layer -> surface, t_dest_image, 0, 0, tx, ty, tw, th ) ;
		
		MCscreen -> destroyimage ( t_dest_image );
	}
	
	
	if ( t_src_image != NULL ) 
	{
		MCscreen -> destroyimage ( t_src_image ) ;
	}
	
	
	Layer * t_layer ;
	t_layer = m_layers ;
	m_layers = m_layers -> parent ;

	
	if ( t_layer -> alpha != NULL && t_layer -> ownAlpha ) 
		MCscreen -> destroyimage ( t_layer -> alpha ) ;

	if ( t_layer -> combiner_image != NULL ) 
		MCscreen -> destroyimage ( t_layer -> combiner_image );

	// MW-2012-04-18: [[ Bug ]] Only free the surface pixmap if we own it.
	if ( t_layer -> shmpixmap ) 
		destroypixmap_shm ( t_layer ) ;
	else if (t_layer -> ownsurface)
		MCscreen -> freepixmap(t_layer -> surface);
	
	free ( t_layer -> shminfo ) ;
	t_layer -> shminfo = NULL ;

	delete t_layer ;
	
	setclip(m_layers -> clip);
	
#ifndef SYNCMODE
	//XSync ( m_display, False ) ;
#endif
}





MCX11Context *MCX11Context::create_context(Drawable p_drawable, uint2 w, uint2 h, Display *p_display, bool p_wantsalpha)
{
	

	MCX11Context *t_context ;
	
	
	t_context = new MCX11Context;
	
	t_context -> m_display = p_display;
	t_context -> m_gc = XCreateGC(p_display, p_drawable, 0, NULL);
	
	Drawable t_32 = MCscreen -> createpixmap ( 1, 1, ((MCScreenDC*)MCscreen)->getdepth(), false );
	t_context -> m_gc32 = XCreateGC( p_display, t_32, 0, NULL ) ;
	MCscreen -> freepixmap ( t_32 ) ;
	
	
	t_context -> m_layers = t_context -> layer_create_with_parameters(w, h, False, p_wantsalpha, False);
	t_context -> m_layers -> surface = p_drawable ; 
	t_context -> m_layers -> shmpixmap = False ;
	
	t_context -> m_mask = MCscreen -> createpixmap( w, h, 1, False ) ;
	t_context -> m_mask_width = w;
	t_context -> m_mask_height = h;

	t_context -> m_gc1 = XCreateGC(p_display, t_context -> m_mask, 0, NULL);
		
	
	t_context -> f_width = w ;
	t_context -> f_height = h ;
	
	t_context -> f_fill . pattern_image = NULL ;
	
	t_context -> clear_surfaces ( t_context -> m_layers ) ;
	

	return t_context;
	
}

MCX11Context *MCX11Context::create_memory_context(uint32_t p_width, uint32_t p_height, Display *p_display, bool p_with_alpha)
{
	MCX11Context *t_context ;
	
	
	t_context = new MCX11Context;
	
	Drawable t_32 = MCscreen -> createpixmap ( 1, 1, ((MCScreenDC*)MCscreen)->getdepth(), false );
	t_context -> m_display = p_display;
	t_context -> m_gc = XCreateGC(p_display, t_32, 0, NULL);
	t_context -> m_gc32 = XCreateGC( p_display, t_32, 0, NULL ) ;
	MCscreen -> freepixmap ( t_32 ) ;

	t_context -> m_layers = t_context -> layer_create_with_parameters(p_width, p_height, False, p_with_alpha, True);
	
	t_context -> m_mask = MCscreen -> createpixmap( p_width, p_height, 1, False ) ;
	t_context -> m_mask_width = p_width;
	t_context -> m_mask_height = p_height;

	t_context -> m_gc1 = XCreateGC(p_display, t_context -> m_mask, 0, NULL);
	t_context -> f_width = p_width ;
	t_context -> f_height = p_height ;
	t_context -> f_fill . pattern_image = NULL ;
	t_context -> clear_surfaces ( t_context -> m_layers ) ;

	return t_context;
}

//===================================================================================================//
//
//                                             S T A R T       B L O C K
//
//                            Start of old X11 routines that draw straight to dc
//===================================================================================================//

void MCX11Context::drawline(int2 x1, int2 y1, int2 x2, int2 y2)
{
	int4 x, y, w, h ;
		
	if ( f_quality == QUALITY_DEFAULT )
	{
		XDrawLine(m_display, m_layers -> surface, m_gc, x1 - m_layers -> origin . x, y1 - m_layers -> origin . y, x2 - m_layers -> origin . x , y2 - m_layers -> origin . y);

		XDrawLine(m_display, m_mask,             m_gc1, x1 - m_layers -> origin . x, y1 - m_layers -> origin . y, x2 - m_layers -> origin . x , y2 - m_layers -> origin . y);
		
		
		if ( x1 < x2 ) 
		{
			x = x1 - m_layers -> origin . x;
			w = x2 - x1 + 1;
		}
		else if (x1 > x2)
		{
			x = x2 - m_layers -> origin . x;
			w = x1 - x2 + 1;
		}
		else
		{
			x = x1 - m_layers -> origin . x;
			w = 1;
		}
		
		if ( y1 < y2 ) 
		{
			y = y1 - m_layers -> origin . y;
			h = y2 - y1 + 1;
		}
		else if (y1 > y2)
		{
			y = y2 - m_layers -> origin . y;
 			h = y1 - y2 + 1;
		}
		else
		{
			y = y1 - m_layers -> origin . y;
			h = 1;
		}
		
		update_mask ( x, y, w , h ) ;
	}	
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_line(x1, y1 , x2 , y2, true);
		drawpath(t_path);
		t_path -> release();
	}
}





void MCX11Context::drawlines(MCPoint *points, uint2 npoints, bool p_closed)
{
#ifdef __GCC
	MCPoint n_points[npoints] ;
#else  
        MCPoint *n_points = (MCPoint *)alloca(sizeof(MCPoint) * npoints);
#endif
	MCRectangle rect ;
	
	adjustPoints( n_points, points, npoints, &rect );
	
	if ( f_quality == QUALITY_DEFAULT )
	{
		XDrawLines(m_display, m_layers -> surface, m_gc, (XPoint *)n_points , npoints, CoordModeOrigin);
		XDrawLines(m_display,             m_mask, m_gc1, (XPoint *)n_points , npoints, CoordModeOrigin);

		uint4 t_capsize ;
		t_capsize = round( f_stroke . width / 2.0);
		update_mask ( rect . x - t_capsize , rect . y - t_capsize , rect . width + t_capsize * 2 , rect . height + t_capsize * 2 ) ;
	}
	else
	{
		MCPath *t_path;
		if (p_closed)
			t_path = MCPath::create_polygon(points, npoints, true);
		else
			t_path = MCPath::create_polyline(points, npoints, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCX11Context::drawsegments(MCSegment *segs, uint2 nsegs)
{
#ifdef __GCC
	MCSegment n_segs[nsegs] ;
#else  
        MCSegment *n_segs = (MCSegment *)alloca(sizeof(MCSegment) * nsegs);
#endif
        MCRectangle rect ;
	adjustSegs(n_segs, segs, nsegs, &rect);
	
	if ( f_quality == QUALITY_DEFAULT )
	{
		XDrawSegments(m_display, m_layers -> surface, m_gc, (XSegment *)n_segs, nsegs);
		XDrawSegments(m_display,             m_mask, m_gc1, (XSegment *)n_segs, nsegs);
		update_mask ( rect . x , rect . y, rect . width , rect . height ) ;
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_polypolyline(segs, nsegs, true);
		drawpath(t_path);
		t_path -> release();
	}
}


void MCX11Context::drawsegment(const MCRectangle& p_rectangle, uint2 p_start, uint2 p_angle)
{

	
	if (f_quality == QUALITY_DEFAULT)
	{

		XDrawArc ( m_display, m_layers -> surface, m_gc, p_rectangle . x, p_rectangle . y, p_rectangle . x + p_rectangle . width, p_rectangle . y + p_rectangle . height, p_start, p_start + p_angle);
		XDrawArc ( m_display, m_mask            , m_gc1, p_rectangle . x, p_rectangle . y, p_rectangle . x + p_rectangle . width, p_rectangle . y + p_rectangle . height, p_start, p_start + p_angle);

		int2 cx = p_rectangle.x + (p_rectangle.width >> 1);
		int2 cy = p_rectangle.y + (p_rectangle.height >> 1);
		real8 torad = M_PI * 2.0 / 360.0;
		real8 tw = (real8)p_rectangle.width;
		real8 th = (real8)p_rectangle.height;
		real8 sa = (real8)p_start * torad;
		
		int2 dx = cx + (int2)(cos(sa) * tw / 2.0);
		int2 dy = cy - (int2)(sin(sa) * th / 2.0);
		drawline(cx, cy, dx, dy);

		sa = (real8)(p_start + p_angle) * torad;
		dx = cx + (int2)(cos(sa) * tw / 2.0);
		dy = cy - (int2)(sin(sa) * th / 2.0);
		drawline(cx, cy, dx, dy);
		
		update_mask ( p_rectangle . x, p_rectangle . y, p_rectangle . width, p_rectangle . height ) ;
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_segment(p_rectangle, p_start, p_angle, true);
		drawpath(t_path);
		t_path -> release();
	}

}


bool MCX11Context::want_painted_text (void)
{
	return m_layers -> hasAlpha && !getflags(FLAG_IS_OPAQUE);
}

void MCX11Context::drawtext(int2 x, int2 y, const char *s, uint2 l,
                          MCFontStruct *f, Boolean image, bool p_unicode_override)
{
	MCFontlistGetCurrent() -> ctxt_drawtext(this, x, y, s, l, f, image, p_unicode_override);
}

void MCX11Context::drawrect(const MCRectangle &rect)
{
	
	
	if ( f_quality == QUALITY_DEFAULT )
	{
		XDrawRectangle(m_display, m_layers -> surface, m_gc, rect.x - m_layers -> origin . x , rect.y - m_layers -> origin . y ,
	    	           rect.width - 1, rect.height - 1);
		XDrawRectangle(m_display,             m_mask, m_gc1, rect.x - m_layers -> origin . x , rect.y - m_layers -> origin . y ,
		               rect.width - 1, rect.height - 1);
		uint2 t_width = 0x1 & f_stroke.width ? (f_stroke.width >> 1) + 1 : (f_stroke.width >> 1);
		update_mask ( rect.x - t_width - m_layers -> origin . x,
				rect.y - t_width - m_layers -> origin . y,
				rect.width + (t_width << 1),
				rect.height + (t_width << 1));
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rectangle(rect, true);
		drawpath(t_path);
		t_path -> release();
	}
}

void MCX11Context::fillrect(const MCRectangle &rect)
{
		
	if ( f_quality == QUALITY_DEFAULT )
	{
		XFillRectangle(m_display, m_layers -> surface, m_gc, rect.x - m_layers -> origin . x , rect.y - m_layers -> origin . y , rect.width, rect.height);
		XFillRectangle(m_display,             m_mask, m_gc1, rect.x - m_layers -> origin . x , rect.y - m_layers -> origin . y , rect.width, rect.height);
		update_mask ( rect.x - m_layers -> origin . x , 
					  rect.y - m_layers -> origin . y , 
					  rect.width + 2, 
					  rect.height + 2);
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rectangle(rect, false);
		fillpath(t_path);
		t_path -> release();
	}
 
}

void MCX11Context::fillrect_with_native_function(const MCRectangle& rect, int function)
{
	XSetFunction(m_display, m_gc, function);
	XFillRectangle(m_display, m_layers -> surface, m_gc, rect . x - m_layers -> origin . x , rect.y - m_layers -> origin . y , rect.width, rect.height);
	XSetFunction(m_display, m_gc, GXcopy);
}

void MCX11Context::fillrects(MCRectangle *rects, uint2 nrects)
{
#ifdef __GCC
	MCRectangle n_rects[nrects] ;
#else  
        MCRectangle *n_rects = (MCRectangle *)alloca(sizeof(MCRectangle) * nrects);
#endif
	MCRectangle rect ;
	adjustRects(n_rects, rects, nrects, &rect);

	if ( f_quality == QUALITY_DEFAULT )
	{
		XFillRectangles(m_display, m_layers -> surface, m_gc, (XRectangle *)n_rects , nrects);
		XFillRectangles(m_display, m_mask, m_gc1, (XRectangle *)n_rects , nrects);
	}
	else
	{
		for(uint4 t_rectangle = 0; t_rectangle < nrects; ++t_rectangle)
			fillrect(rects[t_rectangle]);
	}
}

void MCX11Context::fillpolygon(MCPoint *points, uint2 npoints)
{
#ifdef __GCC
	MCPoint n_points[npoints] ;
#else  
        MCPoint *n_points = (MCPoint *)alloca(sizeof(MCPoint) * npoints);
#endif
	MCRectangle rect ;
	adjustPoints(n_points, points, npoints, &rect);

	if ( f_quality == QUALITY_DEFAULT )
	{
		XFillPolygon(m_display, m_layers -> surface, m_gc, (XPoint *)n_points, npoints, Complex, CoordModeOrigin);
		XFillPolygon(m_display,             m_mask, m_gc1, (XPoint *)n_points, npoints, Complex, CoordModeOrigin);
		update_mask ( rect . x, rect . y, rect . width , rect . height ) ;
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_polygon(points, npoints, true);
		fillpath(t_path);
		t_path -> release();
	}
	
}

void MCX11Context::drawroundrect(const MCRectangle &rect, uint2 radius)
{
	if ( f_quality == QUALITY_DEFAULT )
	{
		MCPoint *points = NULL;
		uint2 npoints = 0;
		MCU_roundrect(points, npoints, rect, radius);
		drawlines(points, npoints);
		delete points;
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rounded_rectangle(rect, radius, true);
		drawpath(t_path);
		t_path -> release();
	}

}

void MCX11Context::fillroundrect(const MCRectangle &rect, uint2 radius)
{
	if ( f_quality == QUALITY_DEFAULT ) 
	{
		MCPoint *points = NULL;
		uint2 npoints = 0;
		MCU_roundrect(points, npoints, rect, radius);
		fillpolygon(points, npoints);
		delete points;
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_rounded_rectangle(rect, radius, false);
		fillpath(t_path);
		t_path -> release();
	}
	
}

void MCX11Context::drawarc(const MCRectangle &rect, uint2 start, uint2 angle)
{
	if ( f_quality == QUALITY_DEFAULT )
	{
		XDrawArc(m_display, m_layers -> surface, m_gc, rect.x - m_layers -> origin . x ,  rect.y - m_layers -> origin . y, 
	    	     rect.width - 1, rect.height - 1, start * 64, angle * 64);
	
		XDrawArc(m_display,             m_mask, m_gc1, rect.x - m_layers -> origin . x ,  rect.y - m_layers -> origin . y, 
		         rect.width - 1, rect.height - 1, start * 64, angle * 64);

		uint2 t_width = 0x1 & f_stroke.width ? (f_stroke.width >> 1) + 1 : (f_stroke.width >> 1);
		update_mask ( rect.x - t_width - m_layers -> origin . x,
				rect.y - t_width - m_layers -> origin . y,
				rect.width + (t_width << 1) - 1,
				rect.height + (t_width << 1) - 1);
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_arc(rect, start, angle, true);
		drawpath(t_path);
		t_path -> release();
	}
	
 
}

void MCX11Context::fillarc(const MCRectangle &rect, uint2 start, uint2 angle)
{
	if ( f_quality == QUALITY_DEFAULT )
	{
		XFillArc(m_display, m_layers -> surface, m_gc, rect.x - m_layers -> origin . x , rect.y - m_layers -> origin . y ,
	    	     rect.width - 1, rect.height - 1, start * 64, angle * 64);
		
		XFillArc(m_display,             m_mask, m_gc1, rect.x - m_layers -> origin . x , rect.y - m_layers -> origin . y ,
		         rect.width - 1, rect.height - 1, start * 64, angle * 64);
	
		update_mask ( rect.x - m_layers -> origin . x , 
					  rect.y - m_layers -> origin . y , 
					  ( rect.width - 1) , 
					  ( rect.height - 1) );
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_segment(rect, start, angle, false);
		fillpath(t_path);
		t_path -> release();
	}
}


void MCX11Context::draweps(real8 sx, real8 sy, int2 angle,
                         real8 xscale, real8 yscale, int2 tx, int2 ty,
                         const char *prolog, const char *psprolog,
                         uint4 psprologlength, const char *ps, uint4 length,
                         const char *fontname, uint2 fontsize,
                         uint2 fontstyle, MCFontStruct *font,
                         const MCRectangle &trect)
{
}




void MCX11Context::setfont(const char *fontname, uint2 fontsize,
                         uint2 fontstyle, MCFontStruct *font)
{
	MCFontlistGetCurrent() -> ctxt_setfont(this, fontname, fontsize, fontstyle, font);
}

void MCX11Context::setforeground(const MCColor &c)
{
	
	
	((MCScreenDC*)MCscreen) -> savedpixel = c.pixel | 0xff000000 ;
	f_fill . colour = c.pixel | 0xff000000;
	XSetForeground(m_display, m_gc, c.pixel | 0xff000000 );
}


void MCX11Context::setbackground(const MCColor &c)
{

	m_bgcolor= c.pixel | 0xff000000 ;
	XSetBackground(m_display, m_gc, c.pixel | 0xff000000);
}

void MCX11Context::setdashes(uint2 p_offset, const uint1 *p_data, uint2 p_length)
{
	XSetDashes(m_display, m_gc, p_offset, (char *)p_data, p_length);
	XSetDashes(m_display, m_gc1, p_offset, (char *)p_data, p_length);
	
	delete[] f_stroke . dash . data;
	f_stroke . dash . data = new uint4[p_length + 2];

	bool t_on;
	uint2 t_start;

	t_start = 0;
	t_on = true;

	while(p_offset > 0 && p_offset >= p_data[t_start])
	{
		p_offset -= p_data[t_start++];
		t_start %= p_length;
		t_on = !t_on;
	}

	uint2 t_current;
	t_current = 0;

	f_stroke . dash . length = p_length;

	if (!t_on)
	{
		f_stroke . dash . data[t_current++] = 0;
		f_stroke . dash . length += 1;
	}

	f_stroke . dash . data[t_current++] = p_data[t_start++] - p_offset;

	for(uint4 t_index = 1; t_index < p_length; ++t_index)
	{
		f_stroke . dash . data[t_current++] = p_data[t_start++];
		t_start %= p_length;
	}

	if (p_offset != 0)
	{
		f_stroke . dash . data[t_current++] = p_offset;
		f_stroke . dash . length++;
	}
}

void MCX11Context::setmiterlimit(real8 p_limit)
{
	f_stroke . miter_limit = p_limit;
}

void MCX11Context::setfillstyle(uint2 style, Pixmap p, int2 x, int2 y)
{
	f_fill . style = style ;
	f_fill . pattern = p ;
	
	if (style == FillTiled)
	{
		XSetTile(m_display, m_gc, p);
		
		uint2 w, h, d;
		((MCScreenDC*)MCscreen) -> getpixmapgeometry(p, w, h, d);

        if ( f_fill . pattern_image != NULL )
        {
            MCscreen -> destroyimage ( f_fill . pattern_image ) ;
            f_fill.pattern_image = NULL;
        }
		
		f_fill . pattern_image = MCscreen -> getimage ( f_fill . pattern, 0, 0, w, h, False );
		assert ( f_fill . pattern_image != NULL);
		
		f_fill . x = x ;
		f_fill . y = y ;
		f_fill . origin . x = x ;
		f_fill . origin . y = y ;
		
		x %= w;
		y %= h;
		if (x < 0)
			x += w;
		if (y < 0)
			y += h;
			
	}
	else
		if (style != FillSolid)
			if (p != DNULL)
				XSetStipple(m_display, m_gc, p);
			else
				XSetStipple(m_display, m_gc, ((MCScreenDC *)MCscreen) -> getstipple());
	XSetTSOrigin(m_display, m_gc, x - m_layers -> origin . x , y - m_layers -> origin . y );
	XSetFillStyle(m_display, m_gc, style);
}

void MCX11Context::setgradient(MCGradientFill *p_gradient)
{
	bool t_changed = false;
	if (f_gradient_fill == NULL)
	{
		if (p_gradient != NULL)
		{
			t_changed = true;
			f_gradient_fill = p_gradient;
		}
	}
	else
	{
		if (p_gradient == NULL)
		{
			t_changed = true;
			f_gradient_fill = p_gradient;
		}
		else
		{
			if (p_gradient->kind != f_gradient_fill->kind || p_gradient->ramp_length != f_gradient_fill->ramp_length)
			{
				t_changed = true;
				f_gradient_fill = p_gradient;
			}
			else
				for (uint4 i=0; i<p_gradient->ramp_length; i++)
					if (p_gradient->ramp[i].offset != f_gradient_fill->ramp[i].offset || p_gradient->ramp[i].color != f_gradient_fill->ramp[i].color)
					{
						t_changed = true;
						f_gradient_fill = p_gradient;
						break;
					}
		}
	}
}

void MCX11Context::setlineatts(uint2 linesize, uint2 linestyle,
                             uint2 capstyle, uint2 joinstyle)
{
	XSetLineAttributes(m_display, m_gc, linesize, linestyle, capstyle & CapMask, joinstyle);
	XSetLineAttributes(m_display, m_gc1, linesize, linestyle, capstyle & CapMask, joinstyle);
	
	f_stroke . width = linesize;
	f_stroke . style = linestyle;
	f_stroke . cap = capstyle;
	f_stroke . join = joinstyle;

}


//===================================================================================================//
//
//                                             E N D    B L O C K
//                                  Old X11 routines that draw straight to dc
//===================================================================================================//


void MCX11Context::unlock(MCBitmap *p_bitmap)
{
	// Memory leak here --- if we did a lock() and DID not get a shm segment , we need to delete the data too.
	// if the bitmap was constructed from shm then the delete_image function ptr should be null
	if (p_bitmap -> f . destroy_image != NULL)
	{
		MCscreen->destroyimage(p_bitmap);
	}
	else
		delete p_bitmap; 
	
}

uint2 MCX11Context::getdepth(void) const
{
	return ( (MCScreenDC*)MCscreen) -> getdepth();
}

const MCColor& MCX11Context::getblack(void) const
{
	return MCscreen -> black_pixel;
}

const MCColor& MCX11Context::getwhite(void) const
{
	return MCscreen -> white_pixel;
}

const MCColor& MCX11Context::getgray(void) const
{
	return MCscreen -> gray_pixel;
}

const MCColor& MCX11Context::getbg(void) const
{
	return MCscreen -> background_pixel;
}

void MCX11Context::drawpict(uint1 *data, uint4 length, bool embed, const MCRectangle& drect, const MCRectangle& crect)
{
  // dummy
}





MCContextType MCX11Context::gettype(void) const
{
	return CONTEXT_TYPE_SCREEN;
}

bool MCX11Context::changeopaque(bool p_new_value)
{
	bool t_old_value;
	t_old_value = getflags(FLAG_IS_OPAQUE);
	setflags(FLAG_IS_OPAQUE, p_new_value);
	return t_old_value;
}

const MCRectangle& MCX11Context::getclip(void) const
{
	return m_layers -> clip;
}


void MCX11Context::setclip(const MCRectangle &rect)
{
    MCRectangle clip;
    MCRectangle layer ;
    MCRectangle intersection ;
	
    
    clip . x = rect . x ;
    clip . y = rect . y ;
    clip . width = rect . width ;
    clip . height = rect .height ;
    
    layer . x = m_layers -> origin . x ;
    layer . y = m_layers -> origin . y ;
    layer . width = m_layers -> width ;
    layer . height = m_layers -> height ;

	
	
	// We need to work out the intersection of the layer and the requested clipping area because there 
	// is NO guarantee that the requested clip will live wholy inside the layer bounds.
	
    intersection = MCU_intersect_rect(layer, clip );
	
	MCRectangle r2 ;
	r2 = intersection ;
	
	m_layers -> clip = r2;

	r2 . x -= m_layers -> origin . x ;
	r2 . y -= m_layers -> origin . y ;

	XSetClipRectangles(m_display, m_gc, 0, 0, (XRectangle *)&r2, 1, YXBanded);
	XSetClipRectangles(m_display, m_gc32, 0, 0, (XRectangle *)&r2, 1, YXBanded);
	XSetClipRectangles(m_display, m_gc1, 0, 0, (XRectangle *)&r2, 1, YXBanded);
}

void MCX11Context::clearclip()
{
	XSetClipMask(m_display, m_gc, None);
	XSetClipMask(m_display, m_gc1, None);

	m_layers -> clip . x = 0 ;
	m_layers -> clip . y = 0 ;
	m_layers -> clip . width = 0 ;
	m_layers -> clip . height = 0 ; 

}



void MCX11Context::setorigin(int2 x, int2 y)
{
	
	m_layers -> origin . x = x ;
	m_layers -> origin . y = y ;
	
}

void MCX11Context::clearorigin(void)
{
	setorigin(0, 0);
}

void MCX11Context::setquality(uint1 quality)
{
	f_quality = quality;
}

void MCX11Context::setfunction(uint1 function)
{
	m_layers -> function = function ;
}

uint1 MCX11Context::getfunction(void)
{
	return m_layers -> function ;
}

void MCX11Context::setopacity(uint1 opacity)
{
	m_layers -> opacity = opacity;
}

uint1 MCX11Context::getopacity(void)
{
  return m_layers -> opacity;
}

void MCX11Context::setprintmode(void)
{
	setflags(FLAG_IS_PRINTER, true);
}


void MCX11Context::drawalphaimage_direct ( MCBitmap *p_data,  int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy ) 
{
	
	flush_mask();
	
	
	
	MCRectangle t_clip, t_dr;
	t_clip = m_layers -> clip;
	MCU_set_rect(t_dr, dx, dy, sw, sh);
	t_dr = MCU_intersect_rect(t_dr, t_clip);
	if (t_dr . width == 0 || t_dr . height == 0)
		return;

	sx += t_dr . x - dx;
	sy += t_dr . y - dy;
	sw = t_dr . width;
	sh = t_dr . height;
	dx = t_dr . x - m_layers -> origin . x ;
	dy = t_dr . y - m_layers -> origin . y ;
	
	
	MCBitmap *t_src_image;
	t_src_image = p_data ;
	
	uint4 t_src_stride;
	t_src_stride = t_src_image -> bytes_per_line;

	void *t_src_ptr;
	t_src_ptr = (uint1*)t_src_image -> data;
	t_src_ptr = (uint1*)t_src_ptr + ( sy * t_src_stride ) + ( sx * 4 ) ;
	
	

	MCBitmap *t_dst_image;
	uint1 *t_dst_ptr;
	uint4 t_dst_stride;

	t_dst_image = lock_layer ( t_dst_ptr, t_dst_stride,  m_layers, dx, dy, sw, sh );
	if ( t_dst_image != NULL)
	{
		
		t_dst_ptr = (uint1*) t_dst_image -> data;
		t_dst_stride = t_dst_image -> bytes_per_line;
	}

	surface_combiner_t t_combiner;
	t_combiner = m_layers -> parent == NULL ? s_surface_combiners_nda[m_layers -> function] : s_surface_combiners[m_layers -> function];
	t_combiner(t_dst_ptr, t_dst_stride, t_src_ptr, t_src_stride, sw, sh, m_layers -> opacity);
	

	if ( t_dst_image != NULL ) 
		MCscreen -> putimage(m_layers -> surface, t_dst_image, 0, 0, dx, dy, sw, sh);

	unlock_layer ( t_dst_ptr, t_dst_stride, m_layers, dx, dy, sw, sh ) ;

	if ( t_dst_image != NULL)
		MCscreen -> destroyimage(t_dst_image);
}	
	


void MCX11Context::drawimage(const MCImageDescriptor& p_image, int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx, int2 dy)
{
	flush_mask();
	
	MCRectangle t_clip, t_dr;
	t_clip = m_layers -> clip;
	MCU_set_rect(t_dr, dx, dy, sw, sh);
	t_dr = MCU_intersect_rect(t_dr, t_clip);
	if (t_dr . width == 0 || t_dr . height == 0)
		return;

	sx += t_dr . x - dx;
	sy += t_dr . y - dy;
	sw = t_dr . width;
	sh = t_dr . height;
	dx = t_dr . x - m_layers -> origin . x ;
	dy = t_dr . y - m_layers -> origin . y ;

	MCBitmap *t_src_image;
	t_src_image = MCscreen -> createimage(32, sw, sh, False, 0x0, False, False);
	
	void *t_src_ptr;
	t_src_ptr = t_src_image -> data;
	
	uint4 t_src_stride;
	t_src_stride = t_src_image -> bytes_per_line;
	
	uint1 *t_alpha_ptr;
	uint4 t_alpha_stride;

	MCBitmap *t_dst_image;
	uint1 *t_dst_ptr;
	uint4 t_dst_stride;

	t_dst_image = lock_layer ( t_dst_ptr, t_dst_stride,  m_layers, dx, dy, sw, sh );
	
	if ( t_dst_image != NULL)
	{
		t_dst_ptr = (uint1*) t_dst_image -> data;
		t_dst_stride = t_dst_image -> bytes_per_line;
	}

	MCImageBitmapPremultiplyRegion(p_image.bitmap, sx, sy, sw, sh, t_src_stride, (uint32_t*)t_src_ptr);
	
	surface_combiner_t t_combiner;
	t_combiner = m_layers -> parent == NULL ? s_surface_combiners_nda[m_layers -> function] : s_surface_combiners[m_layers -> function];
	
	
	
	
	if (!m_layers -> ownAlpha && m_layers -> alpha != NULL )
	{
		t_alpha_stride = m_layers -> alpha -> bytes_per_line ;
		t_alpha_ptr = (uint1*)m_layers -> alpha -> data + ( dy  * t_alpha_stride ) + ( dx ) ;
		
		surface_merge_with_alpha(t_dst_ptr, t_dst_stride,
								 t_alpha_ptr, t_alpha_stride,
								 sw, sh);
	}
	

	
	t_combiner(t_dst_ptr, t_dst_stride, t_src_ptr, t_src_stride, sw, sh, m_layers -> opacity);
	
	
	
	if (!m_layers -> ownAlpha && m_layers -> alpha != NULL )
	{
		surface_extract_alpha(t_dst_ptr, t_dst_stride ,
							  t_alpha_ptr, t_alpha_stride, 
							  sw, sh ) ;
		surface_unmerge_pre(t_dst_ptr, t_dst_stride, sw, sh);
	}

	
	

	if ( t_dst_image != NULL ) 
		MCscreen -> putimage(m_layers -> surface, t_dst_image, 0, 0, dx, dy, sw, sh);
	
	if (m_layers -> ownAlpha && m_layers -> alpha != NULL )
		unlock_layer ( t_dst_ptr, t_dst_stride, m_layers, dx, dy, sw, sh ) ;

	if ( t_src_image != NULL)
		MCscreen -> destroyimage(t_src_image);
	if ( t_dst_image != NULL)
		MCscreen -> destroyimage(t_dst_image);
	
	
	
}	

			

int4 MCX11Context::textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override)
{
	return MCscreen -> textwidth(f, s, l, p_unicode_override);
}



void MCX11Context::applywindowshape(MCWindowShape *p_mask, uint4 p_u_width, uint4 p_u_height)
{
	MCRectangle t_clip;
	t_clip = getclip();

	uint1 *t_dst_ptr;
	uint4 t_dst_stride ;
	uint1 *t_src_ptr;
	uint4 t_src_stride ;
	MCBitmap * t_dst_bm ;
	
	clearclip();	
		
	XSetFunction ( m_display, m_gc, GXcopy ) ;
	XSetForeground(m_display, m_gc, 0x00000000);
		
	if ( p_u_width > p_mask->width )
	{
		XFillRectangle ( m_display, m_layers -> surface , m_gc, 
						p_mask->width , 0,  
						p_u_width - p_mask -> width ,
						p_u_height ) ;
	}
	
		
	if ( p_u_height > p_mask -> height )
	{
		XFillRectangle ( m_display,m_layers -> surface  , m_gc, 
						0 , p_mask -> height, 
						p_u_width ,
						p_u_height - p_mask -> height ) ;
	}
		
	XSetForeground(m_display, m_gc, ((MCScreenDC*)MCscreen) -> savedpixel);
	XSetFunction ( m_display, m_gc, m_layers->function);
		
	setclip ( t_clip );
	
	
	t_dst_bm = lock_layer(t_dst_ptr, t_dst_stride, m_layers, t_clip . x - m_layers -> origin . x, t_clip . y - m_layers -> origin . y, t_clip . width, t_clip . height);
	if (t_dst_bm != NULL)
	{
		t_dst_ptr = (uint1*)t_dst_bm -> data ;
		t_dst_stride = t_dst_bm -> bytes_per_line ;
	}
	
	t_src_ptr = (uint1*)p_mask -> data + t_clip . y * p_mask -> stride + t_clip . x;

	surface_merge_with_alpha(t_dst_ptr, t_dst_stride, t_src_ptr, p_mask -> stride, t_clip . width, t_clip . height);
	
	unlock_layer ( t_dst_ptr, t_dst_stride, m_layers, t_clip . x - m_layers -> origin . x, t_clip . y - m_layers -> origin . y, t_clip . width, t_clip . height);
						 
	if ( t_dst_bm != NULL )
	{
		MCscreen -> putimage(m_layers -> surface, t_dst_bm, 0, 0, t_clip . x - m_layers -> origin . x, t_clip . y - m_layers -> origin . y, t_clip . width, t_clip . height);
		MCscreen -> destroyimage ( t_dst_bm );
	}

}

MCBitmap *MCX11Context::lock(void)
{
	MCBitmap *t_bitmap;
	uint1 *t_image_ptr ;
	uint4 t_image_stride ;
	
	Layer *t_layer ;
	t_layer = m_layers ;
	
	
	flush_mask();
	
	t_bitmap = lock_layer ( t_image_ptr, t_image_stride, t_layer, 0, 0, t_layer -> width, t_layer -> height );
	
	if ( t_bitmap == NULL ) // We got a lock on the shared memory data.
	{
		t_bitmap = new MCBitmap; // OK - lets build the MCBitmap by hand.
	
		t_bitmap -> width = t_layer -> width  ;
		t_bitmap -> height = t_layer -> height ;
		t_bitmap -> data = (char*)t_image_ptr ;
		t_bitmap -> bytes_per_line = t_image_stride ;
		t_bitmap -> format = ZPixmap;

		// MW-2012-11-20: [[ Bug ]] Make sure the depth is initialized appropriately.
		t_bitmap -> depth = 0;
		
		t_bitmap -> f . destroy_image = NULL;
		
	}
	
	return t_bitmap;
}


void MCX11Context::combine(Pixmap p_src, int4 dx, int4 dy, int4 sx, int4 sy, uint4 sw, uint4 sh)
{
	MCBitmap *t_image;
	t_image = MCscreen -> getimage(p_src, sx, sy, sw, sh, False);
	drawalphaimage_direct(t_image, sx, sy, sw, sh, dx, dy);
	MCscreen -> destroyimage(t_image);
}

void MCX11Context::copyarea(Drawable p_src, uint4 p_dx, uint4 p_dy, uint4 p_sx, uint4 p_sy, uint4 p_sw, uint4 p_sh)
{
	
	XCopyArea(m_display,
	          p_src,
	          m_layers -> surface ,
	          m_gc,
	          p_sx, p_sy,	// srcx, srcy
	          p_sw, p_sh, 
			  p_dx, p_dy );
			
}

MCBitmap * MCX11Context::calc_alpha_from_bitmaps ( MCBitmap *t_bm_black, MCBitmap * t_bm_white, uint1 rh, uint1 bh, uint1 gh  ) 
{
	uint1 *t_black_ptr ;
	uint1 *t_white_ptr ;
	uint4 t_black_stride ;
	uint4 t_white_stride ;
	
	uint4 t_w , t_h ;
	
	t_w = t_bm_black -> width ;
	t_h = t_bm_black -> height ; 
	
	/*
		Formula for calculating the alpha of the source, by 
		rendering it twice - once against black, the second time
		agaist white.
	
			Dc' = Sc.Sa + (1 - Sa) * Dc

			composite against black (Dc == 0):
			Dc'b = Sc.Sa => Dc'b <= Sa

			composite against white (Dc == 1.0)
			Dc'w = Sc.Sa + (1 - Sa)

			Sa = 1 - (Dc'w - Dc'b)
			Sc.Sa = Dc'b
	
			As Sc=Dc'b all we need to actually do is recalculate the alpha byte for the black image.
	*/
	
	uint1 rb, rw ;
	uint1 na ;
	uint1 a ;
	uint4 x, y ;

	
	t_black_stride = t_bm_black -> bytes_per_line ;	
	t_white_stride = t_bm_white -> bytes_per_line ;
	
	bool t_bad;
	t_bad = false;
	
	for ( y = 0 ; y < t_h; y ++ )
	{
		for ( x = 0 ; x < t_w ; x++ )
		{	
						
			t_white_ptr = (uint1*)t_bm_white -> data + ( t_white_stride * y ) + (x*4) ;
			t_black_ptr = (uint1*)t_bm_black -> data + ( t_black_stride * y ) + (x*4) ;
			
			rb = *(t_black_ptr);
			rw = *(t_white_ptr);
			
			na = (( uint1) ( 255 - rw + rb ) );
			*(t_black_ptr + 3 ) = na;
			
		}
	}
	
	MCscreen->destroyimage ( t_bm_white ) ;
	
	return ( t_bm_black ) ;
}
	
	

void	MCX11Context::black_and_white_masks ( Drawable t_black, Drawable t_white ) 
{
	MCRectangle t_clip ;	// Needed to store the clipping area as we need to clear it before rendering
							// our black and white backgrounds.
	
	// Store the current layers clip mask and clear it in the GC ( so the XFillRectangle is correct)
	t_clip = getclip() ;
	clearclip();
	
	uint2 t_w, t_h, t_d ;
	
	MCscreen -> getpixmapgeometry ( t_black, t_w, t_h, t_d ) ;
	
	// Colour one in black, the other white.
	XSetFunction ( m_display, m_gc, GXcopy );
	
	XSetForeground ( m_display, m_gc, 0xff000000 );
	XFillRectangle(m_display, (t_black), m_gc, 0, 0, t_w, t_h ) ;
	
	XSetForeground ( m_display, m_gc, 0xffffffff );
	XFillRectangle(m_display, (t_white), m_gc, 0, 0, t_w, t_h ) ;
	
	XSetForeground ( m_display, m_gc, ((MCScreenDC*)MCscreen)->savedpixel ) ;

	XFlush(m_display);
	
	
	// Don't forget to reset the clipmask back.
	setclip( t_clip ) ;

}	

static void fill_gdk_drawable(GdkDrawable *p_drawable, GdkColormap *p_colormap, int p_red, int p_green, int p_blue, int p_width, int p_height)
{
	GdkGC *t_gc;
	t_gc = gdk_gc_new(p_drawable);
	gdk_gc_set_colormap(t_gc, p_colormap);
	
	GdkColor t_color;
	t_color . red = p_red;
	t_color . green = p_green;
	t_color . blue = p_blue;
	
	gdk_gc_set_rgb_fg_color(t_gc, &t_color);
	gdk_draw_rectangle(p_drawable, t_gc, TRUE, 0, 0, p_width, p_height);
	g_object_unref(t_gc);
}

MCBitmap * MCX11Context::drawtheme_calc_alpha ( MCThemeDrawInfo &p_info)
{
	MCBitmap *t_bm_black ;
	MCBitmap *t_bm_white ;
		
	GdkPixmap *t_black ;
	GdkPixmap *t_white ;

	GdkColormap *cm ;
	GdkVisual *best_vis ;
	
	uint4	t_w ;
	uint4	t_h ;
	
		
	t_w = p_info.drect.width ;
	t_h = p_info.drect.height ;

	// Create two new pixmaps
	t_black = gdk_pixmap_new( NULL, t_w, t_h, getdepth());
	t_white = gdk_pixmap_new( NULL, t_w, t_h, getdepth());
	
	// We need to attach a colourmap to the Drawables in GDK
	best_vis = gdk_visual_get_best_with_depth(getdepth());
	cm = gdk_colormap_new( best_vis , False ) ;
	gdk_drawable_set_colormap( t_black, cm);
	gdk_drawable_set_colormap( t_white, cm);

	//gdk_flush();
	
	// Render solid black into one and white into the other.
	//black_and_white_masks ( gdk_x11_drawable_get_xid( t_black ) , gdk_x11_drawable_get_xid(t_white));
	
	fill_gdk_drawable(t_black, cm, 0, 0, 0, t_w, t_h);
	fill_gdk_drawable(t_white, cm, 65535, 65535, 65535, t_w, t_h);
	
	MCThemeDrawInfo t_info;
	
	t_info = p_info;
	moz_gtk_widget_paint ( p_info.moztype, t_white , &t_info.drect, &t_info.cliprect, &t_info.state, t_info.flags ) ;
	
	t_info = p_info;
	moz_gtk_widget_paint ( p_info.moztype, t_black , &t_info.drect, &t_info.cliprect, &t_info.state, t_info.flags ) ;

	gdk_flush();
	
	// Get the byte data for each of these pixmaps
	t_bm_black = MCscreen -> getimage ( gdk_x11_drawable_get_xid(t_black), 0, 0, t_w, t_h, False ) ;
	t_bm_white = MCscreen -> getimage ( gdk_x11_drawable_get_xid(t_white), 0, 0, t_w, t_h, False ) ;
	
	// Calculate the alpha from these two bitmaps --- the t_bm_black image now has full ARGB
	calc_alpha_from_bitmaps ( t_bm_black, t_bm_white ) ;
	
	// clean up.
	g_object_unref( t_black ) ;
	g_object_unref( t_white ) ;
	g_object_unref( cm ) ;
		
	return ( t_bm_black ) ;
	
}



void MCX11Context::drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_info)
{
	MCXImageCacheNode *cache_node = NULL ;
	MCBitmap * t_argb_image ;
	bool t_cached ;
	
	if ( ( p_info -> moztype != MOZ_GTK_CHECKBUTTON ) && ( p_info -> moztype != MOZ_GTK_RADIOBUTTON ) )
		cache_node = MCimagecache -> find_cached_image ( p_info -> drect.width, p_info -> drect.height, p_info -> moztype, &p_info -> state, p_info -> flags ) ;
	
	if ( cache_node != NULL ) 
	{
		t_argb_image = MCimagecache -> get_from_cache( cache_node ) ;
		t_cached = true ;
	}
	else
	{
		// Calculate the alpha for the rendered widget, by rendering against white & black.
		t_argb_image = drawtheme_calc_alpha ( *p_info ) ;
		t_cached = MCimagecache -> add_to_cache ( t_argb_image, *p_info ) ;
		
	}

	drawalphaimage_direct (  t_argb_image, 
						   	0, 0,
				   			p_info -> crect . width,
				   			p_info -> crect . height, 
			   				p_info -> crect . x , 
			   				p_info -> crect . y );
	
	if (!t_cached)
		MCscreen->destroyimage(t_argb_image);
}

void MCX11Context::drawlink(const char *link, const MCRectangle& region)
{
}

void MCX11Context::getfillstyle(uint2& style, Pixmap& p, int2& x, int2& y)
{
	style = f_fill . style;
	p = f_fill . pattern;
	x = f_fill . origin . x;
	y = f_fill . origin . y;
}

MCRegionRef MCX11Context::computemaskregion(void)
{
	MCRectangle t_clip ;

	t_clip = getclip() ;

	XPoint pa[4] ;
				
	pa[0].x = t_clip . x ;
	pa[0].y = t_clip . y ;

	pa[1].x = t_clip . x + t_clip . width ;
	pa[1].y = t_clip . y ;

	pa[2].x = t_clip . x + t_clip . width ;
	pa[2].y = t_clip . y + t_clip . height ;

	pa[3].x = t_clip . x ;
	pa[3].y = t_clip . y + t_clip . height ;


	Region crect;			
	crect = XPolygonRegion(pa, 4, WindingRule ) ;

	return (MCRegionRef)crect; 
}


void MCX11Context::clear(const MCRectangle *rect)
{
	MCRectangle t_rect ;
	if (rect == nil)
	{
		t_rect . x = 0; 
		t_rect . y = 0; 
		t_rect . width = m_layers -> width ;
		t_rect . height = m_layers -> height ;
	}
	else
		t_rect = *rect;

	if (m_layers -> hasAlpha)
	{
		void *t_bits;
		uint32_t t_stride;
		MCBitmap *t_bitmap;
		t_bitmap = lock_bits(t_bits, t_stride);
		for(int32_t y = 0; y < t_rect . height; y++)
			memset((char *)t_bits + t_stride * (t_rect . y + y - m_layers -> origin . y) + (t_rect . x - m_layers -> origin . x) * sizeof(uint32_t), 0, t_rect . width * sizeof(uint32_t));
		unlock_bits(t_bitmap);
	}
	else
	{
		setforeground(getblack());
		fillrect(t_rect);
	}
}

MCPoint * MCX11Context::adjustPoints ( MCPoint *n_points, MCPoint *points, uint2 npoints, MCRectangle *p_bounds  )
{
	uint4 a ;
	
	int4 maxx, maxy, minx, miny ;

	maxx = maxy = 0 ;
	minx = miny = MAXINT4;
	
	for (a=0; a<npoints; a++)
	{
		n_points[a] . x = points[a] . x - m_layers -> origin . x ;
		n_points[a] . y = points[a] . y - m_layers -> origin . y ;
		
		if ( n_points[a] . x < minx ) minx = n_points[a] . x;
		if ( n_points[a] . y < miny ) miny = n_points[a] . y ;
		if ( n_points[a] . x > maxx ) maxx = n_points[a] . x ;
		if ( n_points[a] . y > maxy ) maxy = n_points[a] . y ;
		
	}
	
	p_bounds -> x = minx ;
	p_bounds -> y = miny ;
	p_bounds -> width = maxx - minx ;
	p_bounds -> height = maxy - miny ;
	
	return (n_points);
}


MCRectangle * MCX11Context::adjustRects ( MCRectangle *n_rects, MCRectangle *rects, uint2 nrects, MCRectangle *p_bounds  )
{
	uint4 a ;
	for (a=0; a<nrects; a++)
	{
		n_rects[a] . x = rects[a] . x - m_layers -> origin . x ;
		n_rects[a] . y = rects[a] . y - m_layers -> origin . y ;
		n_rects[a] . width = rects[a] . width ;
		n_rects[a] . height = rects[a] . height ;
		
		update_mask ( n_rects[a] . x , n_rects[a] . y, n_rects[a] . width, n_rects[a] . height ) ;
		
	}
	
	return (n_rects);
}


MCSegment * MCX11Context::adjustSegs(MCSegment *n_segs, MCSegment *segs, uint2 nsegs, MCRectangle *p_bounds )
{
	uint4 a ;

	int4 maxx, maxy, minx, miny ;

	maxx = maxy = 0 ;
	minx = miny = MAXINT4;
	
	
	for (a=0; a<nsegs; a++)
	{
		n_segs[a] . x1 = segs[a] . x1 - m_layers -> origin . x ;
		n_segs[a] . y1 = segs[a] . y1 - m_layers -> origin . y ;
		n_segs[a] . x2 = segs[a] . x2 - m_layers -> origin . x ;
		n_segs[a] . y2 = segs[a] . y2 - m_layers -> origin . y ;
		
		if ( n_segs[a] . x1 < minx ) minx = n_segs[a] . x1;
		if ( n_segs[a] . y1 < miny ) miny = n_segs[a] . y1 ;
		if ( n_segs[a] . x1 > maxx ) maxx = n_segs[a] . x1 ;
		if ( n_segs[a] . y1 > maxy ) maxy = n_segs[a] . y1 ;
		
		if ( n_segs[a] . x2 < minx ) minx = n_segs[a] . x2 ;
		if ( n_segs[a] . y2 < miny ) miny = n_segs[a] . y2 ;
		if ( n_segs[a] . x2 > maxx ) maxx = n_segs[a] . x2 ;
		if ( n_segs[a] . y2 > maxy ) maxy = n_segs[a] . y2 ;
		
	}
	
	p_bounds -> x = minx ;
	p_bounds -> y = miny ;
	p_bounds -> width = maxx - minx ;
	p_bounds -> height = maxy - miny ;
	
	
	return (n_segs);
}



//===================================================================================================//
//
//                          M A S K , A L P H A , S H A R E D M E M O R Y
//            All the stuff related to Masks, Alpha blending and Shared memory Pixmaps
//===================================================================================================//


void MCX11Context::clear_surfaces ( Layer *p_layer ) 
{
	XSetForeground ( m_display, m_gc1, WhitePixel(m_display, 0) );
	XSetBackground ( m_display, m_gc1, BlackPixel(m_display, 0) ) ;
	
	
	
	// Only clear the surface if this is NOT the base pixmap --- else we might be clearing any image that we
	// are wrapping this context around.
	if ( m_layers -> parent != NULL ) 
	{
		XSetFunction( m_display, m_gc, GXclear);
		XCopyArea( m_display, p_layer -> surface, p_layer -> surface ,m_gc, 0, 0, p_layer -> width , p_layer -> height , 0, 0);
		XSetFunction( m_display, m_gc, m_layers -> function );
	}
	
	clear_mask();

#ifndef SYNCMODE
	XSync ( m_display, False ) ;
#endif
	
}

	
MCBitmap *MCX11Context::lock_bits(void*& r_bits, uint32_t& r_stride)
{
	MCBitmap *t_bitmap;
	uint1 *t_dst_ptr;
	uint32_t t_dst_stride;
	t_bitmap = lock_layer ( t_dst_ptr, t_dst_stride, m_layers, 0, 0, m_layers -> width, m_layers-> height );
	if (t_bitmap != nil)
	{
		t_dst_ptr = (uint1*)t_bitmap -> data;
		t_dst_stride = t_bitmap -> bytes_per_line;
	}

	r_bits = t_dst_ptr;
	r_stride = t_dst_stride;

	return t_bitmap;
}

void MCX11Context::unlock_bits(MCBitmap *p_bitmap)
{
	uint1 *t_ci_ptr ;
	uint4 t_ci_stride ;
	
	if ( p_bitmap == NULL ) 	// This is because we are using shared memory for this layer so there is no local image
	{
		t_ci_ptr = (uint1*)m_layers -> shmdata ;
		t_ci_stride = m_layers -> shm_bytes_per_line ;
	}
	else
	{
		t_ci_ptr = (uint1*)p_bitmap -> data ;
		t_ci_stride = p_bitmap -> bytes_per_line ;
	}
	
	// Unlock the layer (with transfers the alpha data to the local mask again)
	unlock_layer ( t_ci_ptr, t_ci_stride, m_layers, 0, 0, m_layers -> width, m_layers -> height ) ;			  
	
	
	putimage_shm ( m_layers, m_layers -> surface, p_bitmap, 0, 0, 0, 0, m_layers -> width, m_layers -> height );
	if ( p_bitmap != NULL )
		MCscreen -> destroyimage ( p_bitmap ) ;
}

// This function merges the local ALPHA plane mask with the fetched image to produce a RGBA surface
MCBitmap * MCX11Context::lock_layer ( uint1 *&p_data_ptr, uint4  & p_data_stride, Layer *t_layer, int4 p_x, int4 p_y,  int4 p_width, int4 p_height  ) 
{
	MCBitmap * t_dst_image ;
	void * t_alpha_ptr ;
	uint1 * t_dst_ptr ;
	uint4 t_dst_stride ;
	uint4 t_alpha_stride ;
	uint4 alpha_offset ;
	
	
	t_dst_ptr = getimage_shm (t_layer , t_dst_stride, t_layer -> surface, p_x, p_y, p_width, p_height );
	if ( t_dst_ptr == NULL)
	{
		t_dst_image = MCscreen -> getimage( t_layer->surface, p_x, p_y, p_width, p_height, False ) ;
		
		assert(t_dst_image != NULL);
		
		t_dst_ptr = (uint1*)t_dst_image -> data ;
		t_dst_stride = t_dst_image -> bytes_per_line ;
		
		// We no longer need to adjust out ptr as we are getting from the correct co-ords rather than from
		// the base of the image. 
		//t_dst_ptr = (uint1*)t_dst_ptr + ( p_y * t_dst_stride ) + ( p_x * 4 ) ;
	}	
	else
	{									
		p_data_stride = t_dst_stride ;
		p_data_ptr = t_dst_ptr ;
		t_dst_image = NULL ;
	}
	
	
	t_alpha_stride = t_layer-> alpha -> bytes_per_line ;

	t_alpha_ptr = (uint1*)t_layer -> alpha -> data ;
	
	
	alpha_offset = (p_y * t_alpha_stride ) + p_x;
	t_alpha_ptr = (uint1*)t_alpha_ptr + alpha_offset ;
	
	surface_merge_with_alpha_non_pre ( t_dst_ptr, t_dst_stride , 
							   t_alpha_ptr , t_alpha_stride, 
							   p_width, p_height ) ;

	return (t_dst_image);
	
}

// This function takes a complete RGBA surface and extracts the alpha layer back to the local plane mask
void MCX11Context::unlock_layer ( void *p_image_ptr, uint4 p_image_stride , Layer *p_layer, int4 p_x, int4 p_y,  int4 p_width, int4 p_height  ) 
{

	void *t_image_ptr ;
	void *t_alpha_ptr ;

	t_image_ptr = p_image_ptr; 
	t_alpha_ptr = p_layer -> alpha -> data ;
	
	t_alpha_ptr = (uint1*) t_alpha_ptr + ( ( p_y * p_layer -> alpha -> bytes_per_line ) + p_x ) ;
	
	surface_extract_alpha(t_image_ptr, p_image_stride, //  -> bytes_per_line, 
						  t_alpha_ptr, p_layer -> alpha -> bytes_per_line, 
						  p_width, p_height) ;
}





Drawable MCX11Context::createpixmap_shm (Layer *p_layer, int4 p_width, int4 p_height, uint4 p_depth )
{
	MCBitmap *t_tmp_image ;
	Drawable pm ;
	uint4 shm_pixmap_format ;

	
	if ( p_width >=32 && p_height >= 32 && MCshmpix ) 
	{
		p_layer -> shmpixmap = False ; // Lets make sure so there is no mistakes later!
		
		// Create a shared IMAGE first --- this is useful for getting things such as Bytes_per_line to be used later for little overhead.	
		t_tmp_image =  (MCBitmap *)XShmCreateImage ( m_display, ((MCScreenDC*)MCscreen) -> getvisual(), p_depth, ZPixmap, NULL, p_layer -> shminfo, p_width, p_height );
		
		assert(t_tmp_image != DNULL);
		
		p_layer -> shm_bytes_per_line = t_tmp_image -> bytes_per_line ;
		
		if ( t_tmp_image -> data != NULL ) 
		{
			delete t_tmp_image->data;
			t_tmp_image->data = NULL;
		}
		
		XDestroyImage ( (XImage *)t_tmp_image ) ;
		
		p_layer -> shminfo -> shmid = shmget (IPC_PRIVATE, p_layer -> shm_bytes_per_line * p_height, IPC_CREAT|0777);
		
		
		
		if ( p_layer -> shminfo -> shmid == -1 ) 
		{
			fprintf(stderr, "Sorry, shmget() failed for reason %d\n", errno);
		}
		
		
		if ( p_layer -> shminfo -> shmid > -1 ) 
		{

		// Now attach the shared memory segment to our process
			
			p_layer -> shminfo -> shmaddr  = (char*)shmat (p_layer -> shminfo -> shmid, 0, 0);
			
			memset(p_layer -> shminfo -> shmaddr, 0, p_layer -> shm_bytes_per_line * p_height);

			assert (p_layer -> shminfo -> shmaddr != NULL);
		
			p_layer -> shmdata = p_layer -> shminfo -> shmaddr ;

		// We want to read and write the shared memory segment
			p_layer -> shminfo -> readOnly = False;
		
		// Finally, attach it all to our display.
			XShmAttach (m_display , p_layer -> shminfo);
		
			shm_pixmap_format = XShmPixmapFormat (m_display);
		
			assert( shm_pixmap_format == ZPixmap ) ;
			
			pm =  ( XShmCreatePixmap ( m_display, 
									    ((MCScreenDC*)MCscreen) -> getroot(),
									    p_layer -> shminfo -> shmaddr , 
									    p_layer -> shminfo, 
									    p_width,
									    p_height, 
									    p_depth ));
			
                
			p_layer -> shmpixmap = (pm != DNULL) && ( p_layer -> shminfo -> shmid > -1 ) ;

		
			p_layer -> shmsurface = 0 ;
			
			XSync( m_display, False ) ;
			
			if ( p_layer -> shmpixmap == True ) 
			{
				p_layer -> shmsurface = pm ;
				return (pm);
			}
		}
	}
		
	return (MCscreen -> createpixmap ( p_width, p_height, p_depth, False )) ;
}



 

uint1* MCX11Context::getimage_shm ( Layer *p_layer, uint4 & p_image_stride, Drawable d, int4 x, int4 y, int4 w, int4 h ) 
{
	uint1 *t_tmp_ptr ;

	if ( d == p_layer -> surface && MCshmpix && p_layer -> shmpixmap )
	{
		p_image_stride = p_layer -> shm_bytes_per_line ;
		t_tmp_ptr = (uint1*)p_layer -> shminfo -> shmaddr ;
		t_tmp_ptr =( (uint1*)t_tmp_ptr + ( p_layer -> shm_bytes_per_line * y ) + ( x * 4 ) );
		return (t_tmp_ptr );
	}
	else
	{
		return ( NULL ) ;
	}
}


void MCX11Context::putimage_shm ( Layer *p_layer, Drawable p_d, MCBitmap *p_image, int4 p_sx, int4 p_sy, int4 p_dx, int4 p_dy, int4 p_w, int4 p_h )
{
	if ( p_d == p_layer -> surface && MCshmpix  && p_layer -> shmpixmap ) 
	{
		return ;
	}
	else
	{
		MCscreen -> putimage (p_d, p_image, p_sx, p_sy, p_dx, p_dy, p_w, p_h );		
	}
}


void MCX11Context::destroypixmap_shm ( Layer *p_layer ) 
{
	uint2 ret ;
	if ( p_layer -> shmpixmap && p_layer -> shmsurface > 0 && p_layer -> shmdata != NULL )
	{
		
		XFreePixmap(m_display, p_layer -> shmsurface);
		XShmDetach (m_display, p_layer -> shminfo);

		
		if ( shmdt (p_layer -> shminfo -> shmaddr) != 0 ) 
			fprintf(stderr, "shmdt() failed for %d\n", errno ) ;
		
		shmctl (p_layer -> shminfo -> shmid, IPC_RMID, 0);
		
		memset(p_layer -> shminfo, 0 , sizeof(XShmSegmentInfo) );
		p_layer -> shmpixmap = false ;
		p_layer -> shmsurface = DNULL ;
		p_layer -> surface = DNULL ;
	}
}













//===================================================================================================//
//
//                                  C O M B I N E R S   A N D   P A T H S
//                                
//===================================================================================================//



void MCX11Context::drawpath(MCPath *path)
{
	MCCombiner *t_combiner = combiner_lock();

	if (t_combiner == NULL)
		return;

	MCRectangle t_clip;
	t_clip = getclip();
	path -> stroke(t_combiner, t_clip, &f_stroke);
	combiner_unlock(t_combiner);
}

void MCX11Context::fillpath(MCPath *path, bool p_evenodd)
{
	MCCombiner *t_combiner = combiner_lock();

	if (t_combiner == NULL)
		return;

	MCRectangle t_clip;
	t_clip = getclip();
	path -> fill(t_combiner, t_clip, p_evenodd);
	combiner_unlock(t_combiner);
}

//-==================================================================================================-

void MCX11Context::mergealpha(void)
{
	if (!m_layers -> hasAlpha || !MCscreen -> hasfeature(PLATFORM_FEATURE_WINDOW_TRANSPARENCY) || !m_layers -> ownAlpha)
		return;

	if (f_width == 0 || f_height == 0)
		return;
		
	uint1 *t_dst_ptr;
	uint4 t_dst_stride ;
	uint1 *t_src_ptr;
	uint4 t_src_stride ;
	MCBitmap * t_dst_bm ;
	
	flush_mask();
	
	t_dst_bm = lock_layer(t_dst_ptr, t_dst_stride, m_layers, 0, 0, f_width, f_height);
	if (t_dst_bm != NULL)
	{
		t_dst_ptr = (uint1*)t_dst_bm -> data ;
		t_dst_stride = t_dst_bm -> bytes_per_line ;
	}
	
	
	
	unlock_layer ( t_dst_ptr, t_dst_stride, m_layers, 0, 0, f_width, f_height);
						 
	if ( t_dst_bm != NULL )
	{
		MCscreen -> putimage(m_layers -> surface, t_dst_bm, 0, 0, 0, 0, f_width, f_height );
		MCscreen -> destroyimage ( t_dst_bm );
	}
}

//-==================================================================================================-

#ifdef _MSC_VER
#define INLINE __forceinline
#else
#define INLINE inline
#endif

// r_i = (x_i * a) / 255
static INLINE uint4 packed_scale_bounded(uint4 x, uint1 a)
{
	uint4 u, v;

	u = ((x & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

// r_i = (x_i * a + y_i * b) / 255
static INLINE uint4 packed_bilinear_bounded(uint4 x, uint1 a, uint4 y, uint1 b)
{
	uint4 u, v;

	u = (x & 0xff00ff) * a + (y & 0xff00ff) * b + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

static void solid_combiner_begin(MCCombiner *_self, int4 y)
{
	MCSolidCombiner *self = (MCSolidCombiner *)_self;
	self -> bits += y * self -> stride;
}

static void solid_combiner_advance(MCCombiner *_self, int4 dy)
{
	MCSolidCombiner *self = (MCSolidCombiner *)_self;
	self -> bits += dy * self -> stride;
}

static void solid_combiner_blend(MCCombiner *_self, int4 fx, int4 tx, uint1 alpha)
{
	MCSolidCombiner *self = (MCSolidCombiner *)_self;
	uint4 *d;
	uint4 s;

	d = self -> bits;

	if (alpha == 255)
	{
		s = self -> pixel;
		for(; fx < tx; ++fx)
			d[fx] = s;
	}
	else
	{
		s = packed_scale_bounded(self -> pixel, alpha);
		for(; fx < tx; ++fx)
			d[fx] = packed_scale_bounded(d[fx], 255 - alpha) + s;
	}
}

static void solid_combiner_end(MCCombiner *_self)
{
}

static void pattern_combiner_begin(MCCombiner *_self, int4 y)
{
	MCPatternCombiner *self = (MCPatternCombiner *)_self;
	self -> pattern_offset = ((y - self -> origin_y) % self -> height) * self -> pattern_stride;
	self -> bits += y * self -> stride;
}

static void pattern_combiner_advance(MCCombiner *_self, int4 dy)
{
	MCPatternCombiner *self = (MCPatternCombiner *)_self;
	self -> pattern_offset += dy * self -> pattern_stride;
	self -> pattern_offset %= self -> height * self -> pattern_stride;
	self -> bits += dy * self -> stride;
}

static void pattern_combiner_blend(MCCombiner *_self, int4 fx, int4 tx, uint1 alpha)
{
	MCPatternCombiner *self = (MCPatternCombiner *)_self;
	uint4 x, w;
	uint4 *s, *d;

	d = self -> bits;
	s = self -> pattern_bits + self -> pattern_offset;

	w = self -> width;
	x = (fx - self -> origin_x) % w;
	
	for(; fx < tx; ++fx)
	{
		d[fx] = packed_bilinear_bounded(s[x] | 0xFF000000, alpha, d[fx], 255 - alpha);
		x++;
		if (x == w)
			x = 0;
	}
}

static void pattern_combiner_end(MCCombiner *_self)
{
	MCPatternCombiner *self = (MCPatternCombiner *)_self;
}



MCCombiner *MCX11Context::combiner_lock(void)
{
	static bool s_solid_combiner_initialised = false;
	static MCSolidCombiner s_solid_combiner;
	static bool s_pattern_combiner_initialised = false;
	static MCPatternCombiner s_pattern_combiner;

	MCSurfaceCombiner *t_combiner;
	t_combiner = NULL;
	if (f_gradient_fill != NULL && f_gradient_fill->kind != kMCGradientKindNone && f_gradient_fill->ramp_length > 1)
	{
		t_combiner = MCGradientFillCreateCombiner(f_gradient_fill, m_layers -> clip);
	}
	else if (f_fill . style == FillSolid)
	{
		if (!s_solid_combiner_initialised)
		{
			s_solid_combiner . begin = solid_combiner_begin;
			s_solid_combiner . advance = solid_combiner_advance;
			s_solid_combiner . blend = solid_combiner_blend;
			s_solid_combiner . end = solid_combiner_end;
			s_solid_combiner . combine = NULL;
			s_solid_combiner_initialised = true;
		}

		s_solid_combiner . pixel = 0xff000000 | (f_fill . colour & 0xffffff);
		t_combiner = &s_solid_combiner;
	}
	else if (f_fill . style == FillTiled)
	{
		if (!s_pattern_combiner_initialised)
		{
			s_pattern_combiner . begin = pattern_combiner_begin;
			s_pattern_combiner . advance = pattern_combiner_advance;
			s_pattern_combiner . blend = pattern_combiner_blend;
			s_pattern_combiner . end = pattern_combiner_end;
			s_pattern_combiner . combine = NULL;
			s_pattern_combiner_initialised = true;
		}
		
		MCRectangle t_pattern_bounds;
		
		t_pattern_bounds . x = 0 ;
		t_pattern_bounds . y = 0 ;
		t_pattern_bounds . width = f_fill . pattern_image -> width ;
		t_pattern_bounds . height = f_fill . pattern_image -> height ;

		s_pattern_combiner . pattern_bits = (uint4*)f_fill . pattern_image -> data ;
		s_pattern_combiner . pattern_stride = f_fill . pattern_image -> bytes_per_line / 4  ;
		s_pattern_combiner . width = t_pattern_bounds . width ;
		s_pattern_combiner . height = t_pattern_bounds . height ;
		
		s_pattern_combiner . origin_x = f_fill . x ; 
		s_pattern_combiner . origin_y = f_fill . y ; 

		if (s_pattern_combiner . origin_x < 0)
			s_pattern_combiner . origin_x += s_pattern_combiner . width;
		if (s_pattern_combiner . origin_y < 0)
			s_pattern_combiner . origin_y += s_pattern_combiner . height;

		t_combiner = &s_pattern_combiner;
	}

	if (t_combiner != NULL)
	{
		uint1 *t_dst_ptr;
		uint4 t_dst_stride;

		flush_mask();
		
		m_layers -> combiner_image = NULL ;		
		m_layers -> combiner_image = lock_layer ( t_dst_ptr, t_dst_stride, m_layers, 
												  0, 
												  0, 
												  m_layers -> width , 
												  m_layers-> height );
		
		if ( m_layers -> combiner_image != NULL ) 
		{
			t_dst_ptr = (uint1*)m_layers -> combiner_image -> data ;
			t_dst_stride = m_layers -> combiner_image -> bytes_per_line ;
		}
		
		t_combiner -> bits = (uint4 *)t_dst_ptr - m_layers -> origin . y * t_dst_stride / 4 - m_layers -> origin . x;
		t_combiner -> stride = t_dst_stride / 4;
	}

	return t_combiner;
}


void MCX11Context::combiner_unlock(MCCombiner *p_combiner)
{
	uint1 *t_ci_ptr ;
	uint4 t_ci_stride ;
	
	if ( m_layers -> combiner_image == NULL ) 	// This is because we are using shared memory for this layer so there is no local image
	{
		t_ci_ptr = (uint1*)m_layers -> shmdata ;
		t_ci_stride = m_layers -> shm_bytes_per_line ;
	}
	else
	{
		t_ci_ptr = (uint1*)m_layers -> combiner_image -> data ;
		t_ci_stride = m_layers -> combiner_image -> bytes_per_line ;
	}
	
	
	// Unlock the layer (with transfers the alpha data to the local mask again)
	unlock_layer ( t_ci_ptr, t_ci_stride, m_layers, 0, 0, m_layers -> width, m_layers -> height ) ;			  
	
	
	putimage_shm ( m_layers, m_layers -> surface, m_layers -> combiner_image, 0, 0, 0, 0, m_layers -> width, m_layers -> height );
	if ( m_layers -> combiner_image != NULL )
		MCscreen -> destroyimage ( m_layers -> combiner_image ) ;

	m_layers -> combiner_image = NULL ;
}



// Functions written to support themes....
Drawable MCX11Context::get_surface(void) 
{
	return ( m_layers -> surface ) ;
}


int4 MCX11Context::get_origin_x(void)
{
	return ( m_layers -> origin . x ) ;
}


int4 MCX11Context::get_origin_y(void)
{
	return ( m_layers -> origin . y ) ;
}


GC MCX11Context::get_gc(void)
{
	return ( m_gc ) ;
}



void MCX11Context::mask_rect_area ( MCRectangle *rect ) 
{
		XFillRectangle(m_display, m_mask, m_gc1, rect->x - m_layers -> origin . x , rect->y - m_layers -> origin . y , rect->width, rect->height);
		update_mask ( rect->x - m_layers -> origin . x , 
					  rect->y - m_layers -> origin . y , 
					  rect->width , 
					  rect->height );
}

void MCX11Context::map_alpha_data ( MCBitmap * p_new_alpha ) 
{
	if (m_layers -> alpha != NULL ) 
		MCscreen -> destroyimage ( m_layers -> alpha ) ;
	m_layers -> ownAlpha = false ;	// Mark this alpha data as no longer "owned" by us.
	m_layers -> alpha = p_new_alpha ;
}


