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

#include "lnxprefix.h"

#include "lnxgtkthemedrawing.h"
#include "lnxtheme.h"
#include "lnximagecache.h"

#define CACHE_PIXEL_LIMIT	8000000 		// 8,000,000 = 32Mb of memory for the cache.
#define CACHE_WIDGET_PIXEL_LIMIT 4000000


// Constructor when we are caching theme items

void dump_state ( GtkWidgetState * p_state ) 
{
	if ( p_state -> active ) fprintf(stderr, "\tACTIVE");
	if ( p_state -> focused ) fprintf(stderr, "\tFOCUSED");
	if ( p_state -> isDefault ) fprintf(stderr, "\tISDEFAULT");
	if ( p_state -> inHover ) fprintf(stderr, "\tINHOVER");
	if ( p_state -> disabled ) fprintf(stderr, "\tDISABLED");
}

MCXImageCacheNode::MCXImageCacheNode(GdkPixbuf *p_bitmap, GtkThemeWidgetType p_moztype, GtkWidgetState *p_state, uint4 p_flags )
{
	isText = False ;
	
	cached_image = p_bitmap ;
	total_pixels = gdk_pixbuf_get_width(p_bitmap) * gdk_pixbuf_get_height(p_bitmap);
	moztype = p_moztype ;
	state = new (nothrow) GtkWidgetState ;
	memcpy((GtkWidgetState*)state, (GtkWidgetState*)p_state, sizeof ( GtkWidgetState )) ;
	flags = p_flags ;

	next_ptr = prev_ptr = NULL ;
}

// Constructor when we are caching rendered text.
/*MCXImageCacheNode::MCXImageCacheNode(GdkPixbuf *p_bitmap, XftFont *p_font, const char *p_string, uint4 p_string_length, uint4 p_fgColor, uint4 p_bgColor )
{
	isText = True ;
	cached_image = p_bitmap ;	
	font = p_font ;
	total_pixels = ( p_bitmap -> width * p_bitmap -> height ) ;
	text_string = new (nothrow) char[p_string_length];
	memcpy(text_string, p_string, p_string_length);
	text_string_length = p_string_length;
	fgColor = p_fgColor ;
	bgColor = p_bgColor ;

	next_ptr = prev_ptr = NULL ;
}*/


MCXImageCacheNode::~MCXImageCacheNode()
{
	
	// Destroy the cached image.
	if ( cached_image != NULL )
	{
		g_object_unref(cached_image);
	}
	
	if ( !isText ) 
		delete state ;
	
	// Remove from the list chain
	if ( next_ptr != NULL ) 
		next_ptr -> prev_ptr = prev_ptr ;
	if ( prev_ptr != NULL ) 
		prev_ptr -> next_ptr = next_ptr ;
	
}


void MCXImageCacheNode::add(MCXImageCacheNode *node) 
{
	node->next_ptr = this ;
	prev_ptr = node ;
}

	
	
Boolean MCXImageCacheNode::matches ( int32_t p_width, int32_t p_height, GtkThemeWidgetType p_moztype, GtkWidgetState *p_state, uint4 p_flags )
{
	return(	( !isText ) &&
		    ( p_width == gdk_pixbuf_get_width(cached_image) ) &&
			( p_height == gdk_pixbuf_get_height(cached_image) ) &&
			( p_moztype == moztype ) &&
		    ( p_flags == flags ) &&
		    ( p_state -> active == state -> active ) &&
		    ( p_state -> focused == state -> focused ) &&
		    ( p_state -> isDefault == state -> isDefault ) &&
		    ( p_state -> inHover == state -> inHover ) &&
		    ( p_state -> disabled == state -> disabled ) );
}


GdkPixbuf* MCXImageCacheNode::get_cached_image (void)
{
	return(cached_image);
}





/* --=====================================================================================================--

				 			W I D G E T   C A C H E    R O U T I N E S 

 --=====================================================================================================--*/



MCXImageCacheNode *MCXImageCache::find_cached_image ( uint4 p_width, uint4 p_height, GtkThemeWidgetType p_moztype, GtkWidgetState *p_state, uint4 p_flags ) 
{
	MCXImageCacheNode *node ;
	
	node = cache_head ;
	while ( node != NULL ) 
	{
		if ( node->matches ( p_width, p_height, p_moztype, p_state, p_flags ) )
			return (node);

		node = node->prev() ;
	}
	return(NULL);
}


void MCXImageCache::destroy_image_cache (void)
{
	
	MCXImageCacheNode *node ;
	
	node = cache_tail ;
	while ( node != NULL ) 
	{
		cache_tail = node -> next();
		delete node ;
		node = cache_tail ;
	}
	
	cache_tail = cache_head = NULL ;
}


bool MCXImageCache::too_big_to_cache (GdkPixbuf * p_bitmap)
{
	return  ( gdk_pixbuf_get_width(p_bitmap) * gdk_pixbuf_get_height(p_bitmap) ) > CACHE_WIDGET_PIXEL_LIMIT;
}


void MCXImageCache::adjust_cache_size (GdkPixbuf * p_bitmap)
{
	MCXImageCacheNode * cache_node ;
	// Need to remove some cached images until we are back under our limit.
	cache_total_pixels += ( gdk_pixbuf_get_width(p_bitmap) * gdk_pixbuf_get_height(p_bitmap) ) ;
	while ( cache_total_pixels > CACHE_PIXEL_LIMIT ) 
	{
		uint4 node_pixels ;
		cache_node = cache_tail ;
		node_pixels = cache_node -> total_pixels ;
		cache_total_pixels -= node_pixels ;
		cache_tail = cache_tail -> next() ;
		delete cache_node ;
		
	}
}
	

bool MCXImageCache::add_to_cache (GdkPixbuf * p_bitmap, MCThemeDrawInfo& p_info)
{
	
	// If a single widget would take up a massive amount of the cache, then don't cache.
	if ( too_big_to_cache ( p_bitmap ) )
	  return false ;
	
	MCXImageCacheNode * cache_node ;
	
	cache_node = new (nothrow) MCXImageCacheNode ( p_bitmap, p_info . moztype, &p_info . state, p_info.flags  ) ;

	if ( cache_tail == NULL )
		cache_tail = cache_node ;
		
	if (  cache_head != NULL ) 
		cache_node -> add ( cache_head ) ;
	
		
	cache_head = cache_node ;
		
	adjust_cache_size( p_bitmap );
	return true ;
	
}


GdkPixbuf* MCXImageCache::get_from_cache ( MCXImageCacheNode * p_node )
{
	// When we access a node from the cache list, we want to move that node to the front.
	if ( cache_head != p_node ) 
	{
		
		if ( cache_tail == p_node ) 
			cache_tail = p_node -> next_ptr ;
		else 
			p_node -> prev_ptr -> next_ptr = p_node -> next_ptr ;
		
		p_node -> next_ptr -> prev_ptr = p_node -> prev_ptr ;
		p_node -> next_ptr = NULL ;
		
		p_node -> prev_ptr = cache_head ;
		cache_head -> next_ptr = p_node ;
		cache_head = p_node ;
	}
		
	return ( p_node -> get_cached_image() ) ;
}


MCXImageCache::MCXImageCache(void )
{
	cache_head = cache_tail = NULL ;
	cache_total_pixels = 0 ;
}


MCXImageCache::~MCXImageCache(void )
{
	destroy_image_cache() ;	
}


	
void MCXImageCache::cache_size(void)
{
	MCXImageCacheNode *node = NULL;
	uint4 count = 0 ;
	
	node = cache_tail ;
	while ( node != NULL ) 
	{
		count++;
		node = node->next() ;
	}
	fprintf(stderr, "Cache size : %d nodes and %d pixels \n", count, cache_total_pixels ) ;
}
	
