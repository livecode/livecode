/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2009 Chris Wilson
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Chris Wilson.
 *
 * Contributor(s):
 *      Chris Wilson <chris@chris-wilson.co.uk>
 *
 */

#include "cairoint.h"

#include "cairo-rtree-private.h"

cairo_rtree_node_t *
_cairo_rtree_node_create (cairo_rtree_t		 *rtree,
		          cairo_rtree_node_t	 *parent,
			  int			  x,
			  int			  y,
			  int			  width,
			  int			  height)
{
    cairo_rtree_node_t *node;

    node = _cairo_freepool_alloc (&rtree->node_freepool);
    if (node == NULL) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return NULL;
    }

    node->children[0] = NULL;
    node->parent = parent;
    node->owner  = NULL;
    node->state  = CAIRO_RTREE_NODE_AVAILABLE;
    node->pinned = FALSE;
    node->x	 = x;
    node->y	 = y;
    node->width  = width;
    node->height = height;

    cairo_list_add (&node->link, &rtree->available);

    return node;
}

void
_cairo_rtree_node_destroy (cairo_rtree_t *rtree, cairo_rtree_node_t *node)
{
    int i;

    cairo_list_del (&node->link);

    if (node->state == CAIRO_RTREE_NODE_OCCUPIED) {
	if (node->owner != NULL)
	    *node->owner = NULL;
	if (rtree->evict != NULL)
	    rtree->evict (node);
    } else {
	for (i = 0; i < 4 && node->children[i] != NULL; i++)
	    _cairo_rtree_node_destroy (rtree, node->children[i]);
    }

    _cairo_freepool_free (&rtree->node_freepool, node);
}

void
_cairo_rtree_node_collapse (cairo_rtree_t *rtree, cairo_rtree_node_t *node)
{
    int i;

    assert (node->pinned == FALSE);

    do {
	assert (node->state == CAIRO_RTREE_NODE_DIVIDED);

	for (i = 0;  i < 4 && node->children[i] != NULL; i++)
	    if (node->children[i]->state != CAIRO_RTREE_NODE_AVAILABLE)
		return;

	for (i = 0; i < 4 && node->children[i] != NULL; i++)
	    _cairo_rtree_node_destroy (rtree, node->children[i]);

	node->children[0] = NULL;
	node->state = CAIRO_RTREE_NODE_AVAILABLE;
	cairo_list_move (&node->link, &rtree->available);

	node = node->parent;
    } while (node != NULL && ! node->pinned);
}

cairo_status_t
_cairo_rtree_node_insert (cairo_rtree_t *rtree,
	                  cairo_rtree_node_t *node,
			  int width,
			  int height,
			  cairo_rtree_node_t **out)
{
    int w, h, i;

    assert (node->state == CAIRO_RTREE_NODE_AVAILABLE);
    assert (node->pinned == FALSE);

    if (node->width  - width  > rtree->min_size ||
	node->height - height > rtree->min_size)
    {
	w = node->width  - width;
	h = node->height - height;

	i = 0;
	node->children[i] = _cairo_rtree_node_create (rtree, node,
						      node->x, node->y,
						      width, height);
	if (unlikely (node->children[i] == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	i++;

	if (w > rtree->min_size) {
	    node->children[i] = _cairo_rtree_node_create (rtree, node,
							  node->x + width,
							  node->y,
							  w, height);
	    if (unlikely (node->children[i] == NULL))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    i++;
	}

	if (h > rtree->min_size) {
	    node->children[i] = _cairo_rtree_node_create (rtree, node,
							  node->x,
							  node->y + height,
							  width, h);
	    if (unlikely (node->children[i] == NULL))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    i++;

	    if (w > rtree->min_size) {
		node->children[i] = _cairo_rtree_node_create (rtree, node,
							      node->x + width,
							      node->y + height,
							      w, h);
		if (unlikely (node->children[i] == NULL))
		    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
		i++;
	    }
	}

	if (i < 4)
	    node->children[i] = NULL;

	node->state = CAIRO_RTREE_NODE_DIVIDED;
	cairo_list_move (&node->link, &rtree->evictable);
	node = node->children[0];
    }

    node->state = CAIRO_RTREE_NODE_OCCUPIED;
    cairo_list_move (&node->link, &rtree->evictable);
    *out = node;

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_rtree_node_remove (cairo_rtree_t *rtree, cairo_rtree_node_t *node)
{
    assert (node->state == CAIRO_RTREE_NODE_OCCUPIED);
    assert (node->pinned == FALSE);

    node->state = CAIRO_RTREE_NODE_AVAILABLE;
    cairo_list_move (&node->link, &rtree->available);

    if (! node->parent->pinned)
	_cairo_rtree_node_collapse (rtree, node->parent);
}

cairo_int_status_t
_cairo_rtree_insert (cairo_rtree_t	     *rtree,
		     int		      width,
	             int		      height,
	             cairo_rtree_node_t	    **out)
{
    cairo_rtree_node_t *node;

    cairo_list_foreach_entry (node, cairo_rtree_node_t,
			      &rtree->available, link)
    {
	if (node->width >= width && node->height >= height)
	    return _cairo_rtree_node_insert (rtree, node, width, height, out);
    }

    return CAIRO_INT_STATUS_UNSUPPORTED;
}

static uint32_t
hars_petruska_f54_1_random (void)
{
#define rol(x,k) ((x << k) | (x >> (32-k)))
    static uint32_t x;
    return x = (x ^ rol (x, 5) ^ rol (x, 24)) + 0x37798849;
#undef rol
}

cairo_int_status_t
_cairo_rtree_evict_random (cairo_rtree_t	 *rtree,
		           int			  width,
		           int			  height,
		           cairo_rtree_node_t		**out)
{
    cairo_rtree_node_t *node;
    int i, cnt;

    cnt = 0;
    cairo_list_foreach_entry (node, cairo_rtree_node_t,
			      &rtree->evictable, link)
    {
	if (node->width >= width && node->height >= height)
	    cnt++;
    }

    if (cnt == 0)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    cnt = hars_petruska_f54_1_random () % cnt;
    cairo_list_foreach_entry (node, cairo_rtree_node_t,
			      &rtree->evictable, link)
    {
	if (node->width >= width && node->height >= height && cnt-- == 0) {
	    if (node->state == CAIRO_RTREE_NODE_OCCUPIED) {
		if (node->owner != NULL)
		    *node->owner = NULL;
		if (rtree->evict != NULL)
		    rtree->evict (node);
	    } else {
		for (i = 0; i < 4 && node->children[i] != NULL; i++)
		    _cairo_rtree_node_destroy (rtree, node->children[i]);
		node->children[0] = NULL;
	    }

	    node->state = CAIRO_RTREE_NODE_AVAILABLE;
	    cairo_list_move (&node->link, &rtree->available);

	    *out = node;
	    return CAIRO_STATUS_SUCCESS;
	}
    }

    return CAIRO_INT_STATUS_UNSUPPORTED;
}

void *
_cairo_rtree_pin (cairo_rtree_t *rtree, cairo_rtree_node_t *node)
{
    void *ptr = node;

    while (node->pinned == FALSE) {
	cairo_list_move (&node->link, &rtree->pinned);
	node->pinned = TRUE;
	node = node->parent;
	if (node == NULL)
	    break;
    }

    return ptr;
}

void
_cairo_rtree_unpin (cairo_rtree_t *rtree)
{
    cairo_rtree_node_t *node, *next;
    cairo_list_t can_collapse;

    if (cairo_list_is_empty (&rtree->pinned))
	return;

    cairo_list_init (&can_collapse);

    cairo_list_foreach_entry_safe (node, next,
	                           cairo_rtree_node_t,
				   &rtree->pinned,
				   link)
    {
	node->pinned = FALSE;
	if (node->state == CAIRO_RTREE_NODE_OCCUPIED && node->owner == NULL) {
	    cairo_bool_t all_available;
	    int i;

	    node->state = CAIRO_RTREE_NODE_AVAILABLE;
	    cairo_list_move (&node->link, &rtree->available);

	    all_available = TRUE;
	    node = node->parent;
	    for (i = 0; i < 4 && node->children[i] != NULL && all_available; i++)
		all_available &= node->children[i]->state == CAIRO_RTREE_NODE_AVAILABLE;

	    if (all_available) {
		cairo_list_move (&node->link, &can_collapse);
		for (i = 0;  i < 4 && node->children[i] != NULL; i++)
		    cairo_list_del (&node->children[i]->link);
	    }
	}
	else
	{
	    cairo_list_move (&node->link, &rtree->evictable);
	}
    }

    cairo_list_foreach_entry_safe (node, next,
	                           cairo_rtree_node_t,
				   &can_collapse,
				   link)
    {
	_cairo_rtree_node_collapse (rtree, node);
    }
}

void
_cairo_rtree_init (cairo_rtree_t	*rtree,
	           int			 width,
		   int			 height,
		   int			 min_size,
		   int			 node_size,
		   void			 (*evict) (void *node))
{
    rtree->evict = evict;

    assert (node_size >= (int) sizeof (cairo_rtree_node_t));
    _cairo_freepool_init (&rtree->node_freepool, node_size);

    cairo_list_init (&rtree->available);
    cairo_list_init (&rtree->pinned);
    cairo_list_init (&rtree->evictable);

    rtree->min_size = min_size;

    memset (&rtree->root, 0, sizeof (rtree->root));
    rtree->root.width = width;
    rtree->root.height = height;
    rtree->root.state = CAIRO_RTREE_NODE_AVAILABLE;
    cairo_list_add (&rtree->root.link, &rtree->available);
}

void
_cairo_rtree_reset (cairo_rtree_t *rtree)
{
    int i;

    if (rtree->root.state == CAIRO_RTREE_NODE_OCCUPIED) {
	if (rtree->root.owner != NULL)
	    *rtree->root.owner = NULL;
	if (rtree->evict != NULL)
	    rtree->evict (&rtree->root);
    } else {
	for (i = 0; i < 4 && rtree->root.children[i] != NULL; i++)
	    _cairo_rtree_node_destroy (rtree, rtree->root.children[i]);
	rtree->root.children[0] = NULL;
    }

    cairo_list_init (&rtree->available);
    cairo_list_init (&rtree->evictable);
    cairo_list_init (&rtree->pinned);

    rtree->root.state = CAIRO_RTREE_NODE_AVAILABLE;
    rtree->root.pinned = FALSE;
    cairo_list_add (&rtree->root.link, &rtree->available);
}

void
_cairo_rtree_fini (cairo_rtree_t *rtree)
{
    int i;

    if (rtree->root.state == CAIRO_RTREE_NODE_OCCUPIED) {
	if (rtree->root.owner != NULL)
	    *rtree->root.owner = NULL;
	if (rtree->evict != NULL)
	    rtree->evict (&rtree->root);
    } else {
	for (i = 0; i < 4 && rtree->root.children[i] != NULL; i++)
	    _cairo_rtree_node_destroy (rtree, rtree->root.children[i]);
    }

    _cairo_freepool_fini (&rtree->node_freepool);
}
