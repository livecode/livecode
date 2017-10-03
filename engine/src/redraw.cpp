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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "globals.h"
#include "stack.h"
#include "stacklst.h"
#include "card.h"
#include "mccontrol.h"
#include "objptr.h"
#include "group.h"
#include "field.h"
#include "button.h"
#include "image.h"
#include "redraw.h"
#include "util.h"
#include "mctheme.h"
#include "bitmapeffect.h"
#include "tilecache.h"
#include "context.h"

#include "graphicscontext.h"

#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

void MCRedrawBeginDeviceSceneryLayer(MCObject* p_object, MCGContextRef p_target, const MCRectangle32& p_rectangle, MCContext*& r_user_context, MCRectangle& r_user_rect)
{
	// IM-2013-09-30: [[ FullscreenMode ]] Apply stack transform to device context
	MCGAffineTransform t_transform;
	t_transform = p_object->getstack()->getdevicetransform();
	
	MCGContextSave(p_target);
	MCGContextConcatCTM(p_target, t_transform);
	
	/* UNCHECKED */ r_user_context = new (nothrow) MCGraphicsContext(p_target);
	r_user_rect = MCRectangleGetTransformedBounds(p_rectangle, MCGAffineTransformInvert(t_transform));
}

void MCRedrawEndDeviceSceneryLayer(MCContext* p_user_context)
{
	MCGContextRestore(static_cast<MCGraphicsContext *>(p_user_context)->getgcontextref());
	delete p_user_context;
}

void MCRedrawBeginDeviceSpriteLayer(MCObject* p_object, MCGContextRef p_target, const MCRectangle32& p_rectangle, MCContext*& r_user_context, MCRectangle& r_user_rect)
{
	// IM-2013-09-30: [[ FullscreenMode ]] Apply stack transform to device context
	MCGAffineTransform t_transform;
	t_transform = p_object->getstack()->getdevicetransform();
	
	// MW-2013-10-29: [[ Bug 11329 ]] Tilecache expects sprite rects to be
	//   relative to top-left of sprite.
    t_transform . tx = 0.0f;
    t_transform . ty = 0.0f;
    
	MCGContextSave(p_target);
	MCGContextConcatCTM(p_target, t_transform);
	
	/* UNCHECKED */ r_user_context = new (nothrow) MCGraphicsContext(p_target);
	r_user_rect = MCRectangleGetTransformedBounds(p_rectangle, MCGAffineTransformInvert(t_transform));
}

void MCRedrawEndDeviceSpriteLayer(MCContext* p_user_context)
{
	MCGContextRestore(static_cast<MCGraphicsContext *>(p_user_context)->getgcontextref());
	delete p_user_context;
}

////////////////////////////////////////////////////////////////////////////////

// This method resets the layer-related attribtues to defaults and marks them
// as needing recomputing.
void MCControl::layer_resetattrs(void)
{
	m_layer_mode = kMCLayerModeHintStatic;
	m_layer_id = 0;
}

////////////////////////////////////////////////////////////////////////////////

MCRectangle MCControl::layer_getcontentrect(void)
{
	// As getcontentrect is only called if 'isscrolling' is true, this unchecked
	// cast is safe.
	MCRectangle t_content_rect = static_cast<MCGroup *>(this) -> getminrect();
	// IM-2014-04-16: [[ Bug 12044 ]] Include rect when computing the coverage of opaque groups
	if (flags & F_OPAQUE)
		t_content_rect = MCU_union_rect(t_content_rect, rect);
	return t_content_rect;
}

void MCControl::layer_redrawall(void)
{
	if (!opened)
		return;

	// Check the visibility state of the object.
	bool t_is_visible;
	t_is_visible = isvisible() || showinvisible();

	// If we are not a sprite, and are invisible there is nothing to do; otherwise
	// we must at least try to dump cached updated parts of the sprite.
	if (!layer_issprite() && !t_is_visible)
		return;

	// Scrolling layers are a special case as its the content that must be dirtied
	// not the effective image.
	if (!layer_isscrolling())
		layer_dirtyeffectiverect(geteffectiverect(), t_is_visible);
	else
		layer_dirtycontentrect(layer_getcontentrect(), t_is_visible);
}

void MCControl::layer_redrawrect(const MCRectangle& p_dirty_rect)
{
	if (!opened)
		return;

	// Check the visibility state of the object.
	bool t_is_visible;
	t_is_visible = isvisible() || showinvisible();

	// If we are not a sprite, and are invisible there is nothing to do; otherwise
	// we must at least try to dump cached updated parts of the sprite.
	if (!layer_issprite() && !t_is_visible)
		return;

	// Scrolling layers are a special case as its the content that must be dirtied
	// not the effective image.
	if (!layer_isscrolling())
	{
		MCRectangle t_dirty_rect;
		t_dirty_rect = p_dirty_rect;
		if (m_bitmap_effects != nil)
			MCBitmapEffectsComputeBounds(m_bitmap_effects, t_dirty_rect, t_dirty_rect);

		layer_dirtyeffectiverect(t_dirty_rect, t_is_visible);
	}
	else
		layer_dirtycontentrect(p_dirty_rect, t_is_visible);
}

void MCControl::layer_transientchangedandredrawall(int32_t p_old_transient)
{
	if (!opened)
		return;

	// Check the visibility state of the object.
	bool t_is_visible;
	t_is_visible = isvisible() || showinvisible();

	// If we are not a sprite, and are invisible there is nothing to do; otherwise
	// we must at least try to dump cached updated parts of the sprite.
	if (!layer_issprite() && !t_is_visible)
		return;

	MCRectangle t_old_effectiverect;
	t_old_effectiverect = MCU_reduce_rect(rect, -p_old_transient);
	if (m_bitmap_effects != nil)
		MCBitmapEffectsComputeBounds(m_bitmap_effects, t_old_effectiverect, t_old_effectiverect);

	layer_changeeffectiverect(t_old_effectiverect, true, t_is_visible);
}

void MCControl::layer_setrect(const MCRectangle& p_new_rect, bool p_redraw_all)
{
	if (!opened)
	{
		setrect(p_new_rect);
		return;
	}
	
	// Check the visibility state of the object.
	bool t_is_visible;
	t_is_visible = isvisible() || showinvisible();
	
	// If we are not a sprite, and are invisible there is nothing to do; otherwise
	// we must at least try to dump cached updated parts of the sprite.
	if (!layer_issprite() && !t_is_visible)
	{
		setrect(p_new_rect);
		return;
	}

	// IM-2016-09-26: [[ Bug 17247 ]] dirty old selection rect
	if (getselected())
		getcard()->dirtyselection(rect);
	
	MCRectangle t_old_effectiverect;
	t_old_effectiverect = geteffectiverect();

	// If the rect has changed size and we aren't a scrolling layer, then we
	// redraw all.
	if (!layer_isscrolling() && 
		(rect . width != p_new_rect . width || rect . height != p_new_rect . height))
		p_redraw_all = true;
		
	setrect(p_new_rect);

	// IM-2016-09-26: [[ Bug 17247 ]] dirty new selection rect
	if (getselected())
		getcard()->dirtyselection(rect);

	layer_changeeffectiverect(t_old_effectiverect, p_redraw_all, t_is_visible);
}

void MCControl::layer_rectchanged(const MCRectangle& p_old_rect, bool p_redraw_all)
{
	if (!opened)
		return;

	// Check the visibility state of the object.
	bool t_is_visible;
	t_is_visible = isvisible() || showinvisible();
	
	// If we are not a sprite, and are invisible there is nothing to do; otherwise
	// we must at least try to dump cached updated parts of the sprite.
	if (!layer_issprite() && !t_is_visible)
		return;

	// Cache the new rectangle.
	MCRectangle t_new_rect;
	t_new_rect = rect;

	// Temporarily set it back to the old one so we can compute the old effective
	// rect.
	MCRectangle t_old_effectiverect;
	rect = p_old_rect;
	t_old_effectiverect = geteffectiverect();

	// If the rect has changed size and we aren't a scrolling layer, then we
	// redraw all.
	if (!layer_isscrolling() && 
		(rect . width != t_new_rect . width || rect . height != t_new_rect . height))
		p_redraw_all = true;

	// Replace the new rect and do the rect changed updates.
	rect = t_new_rect;
	layer_changeeffectiverect(t_old_effectiverect, p_redraw_all, t_is_visible);
}

void MCControl::layer_effectiverectchangedandredrawall(const MCRectangle& p_old_effective_rect)
{
	if (!opened)
		return;

	// Check the visibility state of the object.
	bool t_is_visible;
	t_is_visible = isvisible() || showinvisible();
	
	// If we are not a sprite, and are invisible there is nothing to do; otherwise
	// we must at least try to dump cached updated parts of the sprite.
	if (!layer_issprite() && !t_is_visible)
		return;

	layer_changeeffectiverect(p_old_effective_rect, true, t_is_visible);
}

void MCControl::layer_effectschanged(const MCRectangle& p_old_effective_rect)
{
	layer_effectiverectchangedandredrawall(p_old_effective_rect);
}

// MW-2011-10-17: [[ Bug 9813 ]] We need the effective rect before visibility is
//   changed, else focus border might be not included in our calculation.
void MCControl::layer_visibilitychanged(const MCRectangle& p_old_effective_rect)
{
	if (!opened)
		return;

	if (!parent -> isvisible() && !showinvisible())
		return;

	// If the control is currently visible, then its old rect must have been
	// empty; otherwise the old rect is its effectiverect.
	layer_changeeffectiverect(getflag(F_VISIBLE) ? MCU_make_rect(0, 0, 0, 0) : p_old_effective_rect, false, true);
}

void MCControl::layer_contentoriginchanged(int32_t p_dx, int32_t p_dy)
{
	if (!opened)
		return;

	if (!layer_isscrolling())
		return;

	// If the layer id is 0 then return.
	if (m_layer_id == 0)
		return;

	// Fetch the tilecache we are using.
	MCTileCacheRef t_tilecache;
	t_tilecache = getstack() -> view_gettilecache();

	// Scroll the sprite - note that this method is only called if
	// layer_isscrolling() is true, which is only possible if we have a tilecache.
	// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
	// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
	
	MCGPoint t_device_point;
	t_device_point = MCGPointApplyAffineTransform(MCGPointMake(p_dx, p_dy), getstack()->getdevicetransform());
	MCGFloat t_dx, t_dy;
	t_dx = t_device_point.x;
	t_dy = t_device_point.y;
	MCTileCacheScrollSprite(t_tilecache, m_layer_id, t_dx, t_dy);
}

void MCControl::layer_scrolled(void)
{
	if (!opened)
		return;
		
	// Check the visibility state of the object.
	bool t_is_visible;
	t_is_visible = isvisible() || showinvisible();

	// If the layer isn't scrolling, we must redraw the whole thing. Otherwise
	// we just need to invalidate a portion of the card.
	if (!layer_isscrolling())
	{
		// We only actually need to do something if we are in the sprite
		// case, or we are visible
		if (layer_issprite() || t_is_visible)
			layer_dirtyeffectiverect(geteffectiverect(), t_is_visible);
	}
	else
	{
		// If we are a scrolling layer and not visible, there is nothing to
		// do.
		if (t_is_visible)
			parent.GetAs<MCCard>()->layer_dirtyrect(geteffectiverect());
	}
}

void MCControl::layer_dirtycontentrect(const MCRectangle& p_updated_rect, bool p_update_card)
{
	if (MCU_empty_rect(p_updated_rect))
		return;

	MCRectangle t_content_rect;
	t_content_rect = layer_getcontentrect();
	
	MCTileCacheRef t_tilecache;
	t_tilecache = getstack() -> view_gettilecache();

	// Note that this method is only called if layer_isscrolling() is true, which is only
	// possible if we have a tilecache.
	if (m_layer_id != 0)
	{
		// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
		// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
		MCGAffineTransform t_transform;
		t_transform = getstack()->getdevicetransform();
		
		MCRectangle32 t_device_updated_rect, t_device_content_rect;
		t_device_updated_rect = MCRectangle32GetTransformedBounds(p_updated_rect, t_transform);
		t_device_content_rect = MCRectangle32GetTransformedBounds(t_content_rect, t_transform);
		MCTileCacheUpdateSprite(t_tilecache, m_layer_id, MCRectangle32Offset(t_device_updated_rect, -t_device_content_rect . x, -t_device_content_rect . y));
	}
		
	// Add the rect to the update region - but only if instructed (update_card will be
	// false if the object was invisible).
	if (p_update_card)
		parent.GetAs<MCCard>()->layer_dirtyrect(MCU_intersect_rect(p_updated_rect, geteffectiverect()));
}

void MCControl::layer_dirtyeffectiverect(const MCRectangle& p_effective_rect, bool p_update_card)
{
	// The dirty rect will be the input effective rect expanded by any effects
	// applied by the parent groups (if any).
	MCRectangle t_dirty_rect;
	t_dirty_rect = p_effective_rect;

	// Expand the effective rect by that of all parent groups or controls
	MCControl *t_control;
	t_control = this;
	while (t_control -> parent -> gettype() != CT_CARD)
	{
                // If we have reached a stack before finding a card, this control is
                // not on an open card (it might be an image being used as a button icon
                // for example). In this case, there is nothing that needs to be done
                if (t_control->parent->gettype() == CT_STACK)
                        return;
        
                MCControl *t_parent_control;
		t_parent_control = t_control->parent.GetAs<MCControl>();
		
		// If the parent control is scrolling, we are done - defer to content
		// dirtying.
		if (t_parent_control -> layer_isscrolling())
		{
			t_parent_control -> layer_dirtycontentrect(t_dirty_rect, p_update_card);
			return;
		}
		
		// Otherwise intersect the dirty rect with the parent's rect.
		t_dirty_rect = MCU_intersect_rect(t_dirty_rect, t_control -> parent -> getrect());

		// Expand due to bitmap effects (if any).
		if (t_parent_control -> m_bitmap_effects != nil)
			MCBitmapEffectsComputeBounds(t_parent_control -> m_bitmap_effects, t_dirty_rect, t_dirty_rect);

		t_control = t_parent_control;
	}

	// Fetch the tilecache we are using (if any).
	MCTileCacheRef t_tilecache;
	t_tilecache = t_control -> getstack() -> view_gettilecache();

	// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
	// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
	MCGAffineTransform t_transform;
	t_transform = getstack()->getdevicetransform();
	
	MCRectangle32 t_device_rect;
	t_device_rect = MCRectangle32GetTransformedBounds(t_dirty_rect, t_transform);
	
	// Notify any tilecache of the changes.
	if (t_tilecache != nil)
	{
		// We must be in tile-cache mode with a top-level control, but if the layer
		// id is zero, there is nothing to do.
		if (t_control -> m_layer_id == 0)
			return;

		// How we handle the layer depends on whether it is a sprite or not.
		if (!t_control -> layer_issprite())
		{
			// Non-dynamic layers are scenery in the tilecache, their rect is in
			// canvas co-ords.
			MCTileCacheUpdateScenery(t_tilecache, t_control -> m_layer_id, t_device_rect);
		}
		else
		{
			// Dynamic layers are sprites in the tilecache, their rect is in
			// sprite co-ords.
			MCRectangle t_offset_rect;
			t_offset_rect = MCU_offset_rect(t_dirty_rect, -t_control -> rect . x, -t_control -> rect . y);
			
			// MW-2013-10-29: [[ Bug 11329 ]] Tilecache expects sprite rects to be
			//   relative to top-left of sprite.
			t_transform . tx = 0.0f;
			t_transform . ty = 0.0f;
			
			// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
			// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
			t_device_rect = MCRectangle32GetTransformedBounds(t_offset_rect, t_transform);
			MCTileCacheUpdateSprite(t_tilecache, t_control -> m_layer_id, t_device_rect);
		}
	}

	// Add the rect to the update region - but only if instructed (update_card will be
	// false if the object was invisible).
	if (p_update_card)
		t_control->parent.GetAs<MCCard>()->layer_dirtyrect(t_dirty_rect);
}

void MCControl::layer_changeeffectiverect(const MCRectangle& p_old_effective_rect, bool p_force_update, bool p_update_card)
{
	// Compute the 'new' effectiverect based on visibility.
	MCRectangle t_new_effective_rect;
	if (getflag(F_VISIBLE) || showinvisible())
		t_new_effective_rect = geteffectiverect();
	else
		MCU_set_rect(t_new_effective_rect, 0, 0, 0, 0);

	// If the effective rect has not changed this is at most an update.
	if (MCU_equal_rect(p_old_effective_rect, t_new_effective_rect))
	{
		// If we are forcing an update, use the dirty method.
		if (p_force_update)
		{
			// If the layer is not scrolling just defer to the normal
			// dirty method; otherwise use the dirty content method.
			if (!layer_isscrolling())
				layer_dirtyeffectiverect(t_new_effective_rect, p_update_card);
			else
				layer_dirtycontentrect(layer_getcontentrect(), p_update_card);
		}

		// We are done.
		return;
	}

	// Fetch the tilecache, making it nil if the parent is a group (in the
	// latter case, this is just a dirty op).
	MCTileCacheRef t_tilecache;
	if (parent -> gettype() != CT_GROUP)
		t_tilecache = getstack() -> view_gettilecache();
	else
		t_tilecache = nil;
	
	// If no tilecache, then just dirty the old and new effective rects.
	if (t_tilecache == nil)
	{
		layer_dirtyeffectiverect(p_old_effective_rect, p_update_card);
		layer_dirtyeffectiverect(t_new_effective_rect, p_update_card);
		return;
	}

	// MW-2011-10-17: [[ Bug 9808 ]] Make sure we update the card regardless of
	//    whether we have a layer id - otherwise new objects don't show!
	// Add the rects to the update region - but only if instructed (update_card will be
	// false if the object was invisible).
	if (p_update_card)
	{
		parent.GetAs<MCCard>()->layer_dirtyrect(p_old_effective_rect);
		parent.GetAs<MCCard>()->layer_dirtyrect(t_new_effective_rect);
	}
	
	// We must be in tile-cache mode with a top-level control, but if the layer
	// id is zero, there is nothing to do.
	if (m_layer_id == 0)
		return;

	if (!layer_issprite())
	{
		// Non-dynamic layers are scenery in the tilecache, and we must use old
		// new effective rects so that the appropriate tiles get flushed. Note
		// that 'force_update' has no effect here as reshaping a scenery layer
		// implicitly invalidates all tiles it touches.
		// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
		// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
		MCGAffineTransform t_transform;
		t_transform = getstack()->getdevicetransform();
		
		MCRectangle32 t_old_device_rect, t_new_device_rect;
		t_old_device_rect = MCRectangle32GetTransformedBounds(p_old_effective_rect, t_transform);
		t_new_device_rect = MCRectangle32GetTransformedBounds(t_new_effective_rect, t_transform);
		
		MCTileCacheReshapeScenery(t_tilecache, m_layer_id, t_old_device_rect, t_new_device_rect);
	}
	else
	{
		// Dynamic layers are sprites in the tilecache, and there is nothing to
		// do unless 'force update' is required. In particular, if the layer is
		// just moving then no redraw of the layer will be needed. Note, however,
		// that this implicitly assumes that 'force update' is true if the content
		// in a sprite-relative co-ord system has changed.
		if (p_force_update)
		{
			MCRectangle t_rect;
			
			// If the layer is not scrolling, just use the width/height from the
			// effective rect; otherwise use content width/height.
			if (!layer_isscrolling())
				t_rect = p_old_effective_rect;
			else
				t_rect = layer_getcontentrect();
				
			t_rect . x = t_rect . y = 0;
			
			// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
			// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
			MCGAffineTransform t_transform;
			t_transform = getstack()->getdevicetransform();
			
			// MW-2013-10-29: [[ Bug 11329 ]] Tilecache expects sprite rects to be
			//   relative to top-left of sprite.
			t_transform . tx = 0.0f;
			t_transform . ty = 0.0f;
			
			MCRectangle32 t_device_rect;
			t_device_rect = MCRectangle32GetTransformedBounds(t_rect, t_transform);
			MCTileCacheUpdateSprite(t_tilecache, m_layer_id, t_device_rect);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCCard::layer_added(MCControl *p_control, MCObjptr *p_previous, MCObjptr *p_next)
{
	MCTileCacheRef t_tilecache;
	t_tilecache = getstack() -> view_gettilecache();

	// Add the rects to the update region (for clarity, would prefer this at the end
	// but there is 'return' fall through in the rest :-( ).
	layer_dirtyrect(p_control -> geteffectiverect());
	
	// Notify any tilecache of the changes.
	if (t_tilecache != nil)
	{
		// Reset all the layer's attributes to defaults, including the layer id.
		p_control -> layer_resetattrs();

		// If the control is on a dynamic layer there is nothing to do (sprites will
		// be created implicitly at first render).
		if (p_control -> layer_getmodehint() != kMCLayerModeHintStatic)
			return;

		// If the control is not being added between two existing layers then there
		// is nothing to do.
		if (p_previous == nil || p_next == nil)
			return;

		// If the previous (layer below) objptr has no id, then there is nothing to do
		// also. This will only occur if only new layers have been added above,
		// or if no rendering has been done yet.
		uint32_t t_before_layer_id;
		t_before_layer_id = p_previous -> getref() -> layer_getid();
		if (t_before_layer_id == 0)
			return;

		// MW-2013-06-21: [[ Bug 10974 ]] If the previous layer is a sprite then this layer
		//   will change the lower limit of the scenery layers above, thus there is
		//   nothing to do.
		if (p_previous -> getref() -> layer_issprite())
			return;
		
		// Now insert the scenery.
		// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
		// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
		MCRectangle32 t_device_rect;
		t_device_rect = MCRectangle32GetTransformedBounds(p_control->geteffectiverect(), getstack()->getdevicetransform());
		MCTileCacheInsertScenery(t_tilecache, t_before_layer_id, t_device_rect);

		// Finally, set the id of the layer to that of the one before. This causes
		// the layer to be treated 'as one' with that layer until a redraw is done.
		// This means that any subsequent updates to the rect of the new layer will
		// appropriately flush the tiles in the cache.
		p_control -> layer_setid(t_before_layer_id);
	}
}

void MCCard::layer_removed(MCControl *p_control, MCObjptr *p_previous, MCObjptr *p_next)
{
	MCTileCacheRef t_tilecache;
	t_tilecache = getstack() -> view_gettilecache();

	// Add the rects to the update region (for clarity, would prefer this at the end
	// but there is 'return' fall through in the rest :-( )
	layer_dirtyrect(p_control -> geteffectiverect());
	
	// Notify any tilecache of the changes.
	if (t_tilecache != nil)
	{
		// If the control has no layer id then there is nothing to do.
		if (p_control -> layer_getid() == 0)
			return;

		// If the control is on a dynamic layer then remove any associated sprite.
		if (p_control -> layer_issprite())
		{
			MCTileCacheRemoveSprite(t_tilecache, p_control -> layer_getid());
			
			// MW-2012-09-21: [[ Bug 10005 ]] Make sure we reset the layer attrs so we
			//   don't try and reuse a dead sprite.
			p_control -> layer_resetattrs();
			
			return;
		}

		// Remove the scenery.

		// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
		// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
		MCRectangle32 t_device_rect;
		t_device_rect = MCRectangle32GetTransformedBounds(p_control->geteffectiverect(), getstack()->getdevicetransform());
		MCTileCacheRemoveScenery(t_tilecache, p_control -> layer_getid(), t_device_rect);
		
		// MW-2012-10-11: [[ Bug ]] Redraw glitch caused by resetting the layer id
		//   before removing the layer.
		// MW-2012-09-21: [[ Bug 10005 ]] Make sure we reset the layer attrs so we
		//   don't try and reuse a dead scenery layer.
		p_control -> layer_resetattrs();
		
		// If there is no previous or next control we have no tweaks to ids
		// to perform.
		if (p_previous == nil || p_next == nil)
			return;

		// Now layer ids for new layers percolate from the next layer, so the
		// original layers are always at the bottom of the stack. If the next
		// layer has a different id than us, make sure all previous layers
		// with the same id match it.
		uint32_t t_before_layer_id;
		t_before_layer_id = p_previous -> getref() -> layer_getid();

		// The layer below us has the same id so there's nothing to do, we are
		// removing a 'new' layer before its been redrawn.
		if (t_before_layer_id == p_control -> layer_getid())
			return;
		
		// MW-2013-06-21: [[ Bug 10974 ]] If the layer below is a sprite, then removing
		//   this layer will increase the lower limit of the scenery stack above
		//   thus there is nothing to do.
		if (p_previous -> getref() -> layer_issprite())
			return;

		// The layer below us has a different id, so this is an existing layer
		// and thus we must ensure all layers above us now use the id of the
		// layer below.
		MCObjptr *t_objptr;
		t_objptr = p_next;
		while(t_objptr != p_previous && t_objptr -> getref() -> layer_getid() == p_control -> layer_getid())
		{
			t_objptr -> getref() -> layer_setid(t_before_layer_id);
			t_objptr = t_objptr -> next();
		}
	}
}

void MCCard::layer_setviewport(int32_t p_x, int32_t p_y, int32_t p_width, int32_t p_height)
{
	// Get the current rect, before updating it.
	MCRectangle t_old_rect;
	t_old_rect = rect;
	
	// Update the rect.
	resize(p_width, p_height);
	
	// Add the rects to the update region.

	// MW-2012-05-01: [[ Bug 10157 ]] If the card has a border then add the whole card
	//   rect to the update region; otherwise just add the exposed rects.
	if (!getflag(F_SHOW_BORDER))
	{
	if (p_width > t_old_rect.width)
		layer_dirtyrect(MCU_make_rect(t_old_rect.width, 0, p_width - t_old_rect.width, p_height));
	if (p_height > t_old_rect.height)
		layer_dirtyrect(MCU_make_rect(0, t_old_rect.height, p_width, p_height - t_old_rect.height));
}
	else
		layer_dirtyrect(rect);
}

void MCCard::layer_selectedrectchanged(const MCRectangle& p_old_rect, const MCRectangle& p_new_rect)
{
	MCTileCacheRef t_tilecache;
	t_tilecache = getstack() -> view_gettilecache();

	if (t_tilecache != nil)
	{
		// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
		// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
		MCGAffineTransform t_transform;
		t_transform = getstack()->getdevicetransform();
		
		MCRectangle32 t_new_device_rect, t_old_device_rect;
		t_new_device_rect = MCRectangle32GetTransformedBounds(p_new_rect, t_transform);
		t_old_device_rect = MCRectangle32GetTransformedBounds(p_old_rect, t_transform);
		MCTileCacheUpdateScenery(t_tilecache, m_fg_layer_id, t_old_device_rect);
        MCTileCacheUpdateScenery(t_tilecache, m_fg_layer_id, t_new_device_rect);
	}

	layer_dirtyrect(p_old_rect);
	layer_dirtyrect(p_new_rect);
}

void MCCard::layer_dirtyrect(const MCRectangle& p_dirty_rect)
{
	getstack() -> dirtyrect(p_dirty_rect);
}

////////////////////////////////////////////////////////////////////////////////

void MCCard::render(void)
{
	MCTileCacheRef t_tilecache;
	t_tilecache = getstack() -> view_gettilecache();

	bool t_reset_ids;
	t_reset_ids = MCTileCacheIsClean(t_tilecache);

	// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
	MCGAffineTransform t_transform;
	t_transform = getstack()->getdevicetransform();
	
	// IM-2013-10-14: [[ FullscreenMode ]] Get the visible area of the stack
	// IM-2013-12-20: [[ ShowAll ]] Use MCStack::getvisiblerect() to get the visible area
	MCRectangle t_visible_rect;
	t_visible_rect = getstack()->getvisiblerect();
    
    /* First emit the foreground layer. This consists of the selection marquee,
     * the card border and selected control's selection handles. */
    MCTileCacheLayer t_fg_layer;
    t_fg_layer.id = m_fg_layer_id;
    t_fg_layer.region = MCRectangle32GetTransformedBounds(rect, t_transform);
    t_fg_layer.clip = MCRectangle32GetTransformedBounds(t_visible_rect, t_transform);
    t_fg_layer.is_opaque = false;
    t_fg_layer.opacity = 255;
    t_fg_layer.ink = GXblendSrcOver;
    t_fg_layer.callback = MCRedrawRenderDeviceSceneryLayer<MCCard, &MCCard::render_foreground>;
    t_fg_layer.context = this;
    MCTileCacheRenderScenery(t_tilecache, t_fg_layer);
    m_fg_layer_id = t_fg_layer.id;
    
    /* Next emit the object layers - starting at the front most object (which 
     * is the last in the last) and working back to the rear most object (which
     * is the first in the list. */
    MCObjptr *t_objptrs = getobjptrs();
    if (t_objptrs != nullptr)
    {
        MCObjptr *t_objptr = t_objptrs->prev();
        do
        {
            /* Fetch the control and call its render method. */
            MCControl *t_control = t_objptr->getref();
            t_control->render(t_tilecache, t_reset_ids, t_transform, t_visible_rect);

            /* Step to the control below */
            t_objptr = t_objptr->prev();
        }
        while(t_objptr != t_objptrs -> prev());
    }
    
    /* Finally emit the background layer which just consists of the rendered
     * card / stack background. */
    MCTileCacheLayer t_bg_layer;
    t_bg_layer.id = m_bg_layer_id;
    t_bg_layer.region = MCRectangle32GetTransformedBounds(rect, t_transform);
    t_bg_layer.clip = MCRectangle32GetTransformedBounds(t_visible_rect, t_transform);
    t_bg_layer.is_opaque = true;
    t_bg_layer.opacity = 255;
    t_bg_layer.ink = GXblendSrcOver;
    t_bg_layer.callback = MCRedrawRenderDeviceSceneryLayer<MCCard, &MCCard::render_background>;
    t_bg_layer.context = this;
    MCTileCacheRenderScenery(t_tilecache, t_bg_layer);
    m_bg_layer_id = t_bg_layer.id;
}

bool MCCard::render_background(MCContext *p_dc, const MCRectangle& p_dirty)
{
    drawbackground(p_dc, p_dirty);
    return true;
}

bool MCCard::render_foreground(MCContext *p_dc, const MCRectangle& p_dirty)
{
    drawselectedchildren(p_dc);
    
    drawcardborder(p_dc, p_dirty);
    
    if (getstate(CS_SIZE))
    {
        drawselectionrect(p_dc);
    }
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool s_screen_is_dirty = false;
static bool s_screen_updates_disabled = false;

bool MCRedrawIsScreenLocked(void)
{
	return MClockscreen != 0;
}

void MCRedrawSaveLockScreen(uint2& r_lock)
{
	r_lock = MClockscreen;
}

void MCRedrawRestoreLockScreen(uint2 p_lock)
{
	MClockscreen = p_lock;
	
	if (MClockscreen == 0 && s_screen_is_dirty && !s_screen_updates_disabled)
        MCActionsSchedule(kMCActionsUpdateScreen);
}

void MCRedrawLockScreen(void)
{
	MClockscreen++;
	
	if (MClockscreen == 0 && s_screen_is_dirty && !s_screen_updates_disabled)
        MCActionsSchedule(kMCActionsUpdateScreen);
}

void MCRedrawUnlockScreen(void)
{
	if (MClockscreen == 0)
		return;

	MClockscreen--;
	
	if (MClockscreen == 0 && s_screen_is_dirty && !s_screen_updates_disabled)
        MCActionsSchedule(kMCActionsUpdateScreen);
}

void MCRedrawUnlockScreenWithEffects(void)
{
	// MW-2011-09-20: [[ Bug ]] If the screen is unlocked, do nothing.
	if (MClockscreen == 0)
		return;

	MClockscreen--;
	if (MClockscreen == 0 && MCcur_effects != nil)
	{
		Boolean t_abort;
		
		// MW-2011-09-24: [[ Effects ]] Play an effect in the chosen rect.
		MCdefaultstackptr -> effectrect(MCcur_effects_rect, t_abort);
	}
	
	if (MClockscreen == 0 && s_screen_is_dirty && !s_screen_updates_disabled)
        MCActionsSchedule(kMCActionsUpdateScreen);
}

void MCRedrawForceUnlockScreen(void)
{
	MClockscreen = 0;
	
	if (MClockscreen == 0 && s_screen_is_dirty && !s_screen_updates_disabled)
        MCActionsSchedule(kMCActionsUpdateScreen);
}

////////////////////////////////////////////////////////////////////////////////

bool MCRedrawIsScreenDirty(void)
{
	return s_screen_is_dirty;
}

void MCRedrawDirtyScreen(void)
{
	MCStacknode *t_stacks;
	t_stacks = MCstacks -> topnode();
	if (t_stacks == nil)
		return;

	// MW-2011-10-17: [[ Bug 9810 ]] Make sure we don't miss any stacks
	//   (was previously looping back to t_stacks, not prev(t_stacks).
	MCStacknode *tptr = t_stacks->prev();
	do
	{
		MCStack *sptr = tptr->getstack();
		sptr -> view_dirty_all();
		tptr = tptr->prev();
	}
	while (tptr != t_stacks -> prev());
}

void MCRedrawScheduleUpdateForStack(MCStack *stack)
{
	s_screen_is_dirty = true;
	
	if (MClockscreen == 0 && s_screen_is_dirty && !s_screen_updates_disabled)
        MCActionsSchedule(kMCActionsUpdateScreen);
}

bool MCRedrawIsScreenUpdateEnabled(void)
{
	return !s_screen_updates_disabled;
}

void MCRedrawDisableScreenUpdates(void)
{
	s_screen_updates_disabled = true;
	
	if (MClockscreen == 0 && s_screen_is_dirty && !s_screen_updates_disabled)
        MCActionsSchedule(kMCActionsUpdateScreen);
}

void MCRedrawEnableScreenUpdates(void)
{
	s_screen_updates_disabled = false;
	
	if (MClockscreen == 0 && s_screen_is_dirty && !s_screen_updates_disabled)
        MCActionsSchedule(kMCActionsUpdateScreen);
}

void MCRedrawDoUpdateScreen(void)
{
	if (MClockscreen != 0)
		return;

	if (!s_screen_is_dirty)
		return;
	
	if (s_screen_updates_disabled)
		return;
		
	MCStacknode *t_stacks;
	t_stacks = MCstacks -> topnode();
	if (t_stacks == nil)
		return;

	MCStacknode *tptr = t_stacks->prev();
	do
	{
		MCStack *sptr = tptr->getstack();

		if (sptr->getstate(CS_NEED_RESIZE))
		{
			sptr->setgeom();
            sptr->openrect(sptr->getrect(), WM_LAST, NULL, WP_DEFAULT, OP_NONE);

            // SN-2015-08-31: [[ Bug 15705 ]] From 6.7.7, MCRedrawUpdateScreen
            //  also removes kMCActionUpdateScreen from MCactionsrequired,
            //  which was not the case beforehand - and the redrawing could nest
            //  here, and eventually set s_screen_is_dirty to false (l.1383)
            if (MClockscreen == 0 && s_screen_is_dirty && !s_screen_updates_disabled)
                MCActionsSchedule(kMCActionsUpdateScreen);

            MCRedrawUpdateScreen();
			return;
		}

		sptr -> applyupdates();

		tptr = tptr->prev();
	}
	while (tptr != t_stacks->prev());

	s_screen_is_dirty = false;
	
	if (MClockscreen == 0 && s_screen_is_dirty && !s_screen_updates_disabled)
        MCActionsSchedule(kMCActionsUpdateScreen);
}

////////////////////////////////////////////////////////////////////////////////

