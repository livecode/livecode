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

// This method resets the layer-related attribtues to defaults and marks them
// as needing recomputing.
void MCControl::layer_resetattrs(void)
{
	m_layer_mode = kMCLayerModeHintStatic;
	m_layer_is_opaque = false;
	m_layer_is_unadorned = false;
	m_layer_is_sprite = false;
	m_layer_is_direct = false;
	m_layer_attr_changed = true;
	m_layer_id = 0;
}

// This method updates all the layer attributes of the control to make sure they
// are consistent with the controls current set of flags. If commit is false,
// then the new layermode is returned without changing anything.
MCLayerModeHint MCControl::layer_computeattrs(bool p_commit)
{
	// If the attrs have not changed, there is nothing to do.
	if (!m_layer_attr_changed)
		return m_layer_mode;

	// If the layer id is 0, then it means we should clear current settings.
	if (m_layer_id == 0)
	{
		m_layer_mode = kMCLayerModeHintStatic;
		m_layer_is_opaque = false;
		m_layer_is_unadorned = false;
		m_layer_is_sprite = false;
		m_layer_is_direct = false;
	}

	// The opacity of a control depends on what flags it has set - in particular
	// the 'opaque' flag. However, as 'opaque' determines themed bg rendering this
	// is not a sufficient condition.
	//
	// If a control has external bitmap effects (drop shadow, outerglow) then it
	// cannot be opaque. Similarly, if the control is rendered with themed bgs then
	// it cannot be opaque.
	//
	// Opacity is more dynamic an attribute than adornedness and should be handled
	// as a separate computation in the future.
	//
	bool t_is_opaque;
	t_is_opaque = false;
	if (MCBitmapEffectsIsInteriorOnly(getbitmapeffects()))
	{
		switch(gettype())
		{
		case CT_GROUP:
			// Only consider groups unadorned groups to be opaque.
			t_is_opaque = getflag(F_OPAQUE) &&
				!getflag(F_HSCROLLBAR | F_VSCROLLBAR | F_SHOW_NAME | F_SHOW_BORDER);
			break;
		case CT_FIELD:
			// Only consider unadorned fields to be opaque.
			t_is_opaque = getflag(F_OPAQUE) && 
				!getflag(F_HSCROLLBAR | F_VSCROLLBAR | F_SHOW_BORDER | F_SHADOW) && (extraflags & EF_NO_FOCUS_BORDER) != 0;
			break;
		case CT_BUTTON:
		case CT_IMAGE:
		case CT_SCROLLBAR:
		case CT_GRAPHIC:
		case CT_PLAYER:
		default:
			// The rest of the control types are hard to assess for opacity as
			// that depends on their content / or complex theming considerations.
			t_is_opaque = false;
			break;
		}
	}
	else
		t_is_opaque = false;

	// The unadorned state depends on control type, but in general  means that the
	// control consists of background + content. For a group content is the child
	// controls, for a field its the text, for an image its the bits, for a button
	// its the icon (if any), for a graphic it means just its shape.
	//
	// If a control has bitmap effects, it is always adorned as this requires further
	// processing of the image.
	//
	// If a control is selected, it is always adorned, since the selection handles
	// are part of the object.
	//
	bool t_is_unadorned;
	if (getbitmapeffects() == nil)
	{
		switch(gettype())
		{
		case CT_GROUP:
			// A group is unadorned if it has no scrollbars, no border and doesn't
			// show a name.
			t_is_unadorned = !getflag(F_HSCROLLBAR | F_VSCROLLBAR | F_SHOW_NAME | F_SHOW_BORDER);
				
			uint16_t t_index;
			// IM-2014-05-02: [[ Bugfix 12044 ]] An opaque group is unadorned if its background is a color and it disallows overscroll
			t_is_unadorned &= !getflag(F_OPAQUE) || (getcindex(DI_BACK, t_index) && !getflag(F_UNBOUNDED_HSCROLL | F_UNBOUNDED_VSCROLL));
			break;
		case CT_FIELD:
			// A field is unadorned if it has no shadow, no scrollbars, no border and no focus
			// border.
			t_is_unadorned = !getflag(F_HSCROLLBAR | F_VSCROLLBAR | F_SHOW_BORDER | F_SHADOW) && (extraflags & EF_NO_FOCUS_BORDER) != 0;
			break;
		case CT_BUTTON:
			// A button is unadorned if it is not a combo-box is showing an icon, has no border,
			// no name, no shadow, no hilite border, no arm border and no focus border.
			if (((MCButton *)this) -> getmenumode() != WM_COMBO)
				t_is_unadorned = getflag(F_SHOW_ICON) &&
									!getflag(F_SHOW_BORDER | F_SHOW_NAME | F_SHADOW | F_HILITE_BORDER | F_ARM_BORDER) &&
									(extraflags & EF_NO_FOCUS_BORDER) != 0;
			else
				t_is_unadorned = false;
			break;
		case CT_IMAGE:
			// An image is unadorned if it is not a pict, has no border and no focus border.
			if (static_cast<MCImage*>(this)->getcompression() != F_PICT)
				t_is_unadorned = !getflag(F_SHOW_BORDER) && (extraflags & EF_NO_FOCUS_BORDER) != 0;
			else
				t_is_unadorned = false;
			break;
		case CT_SCROLLBAR:
		case CT_GRAPHIC:
		case CT_PLAYER:
		default:
			t_is_unadorned = false;
			break;
		}
	}
	else
		t_is_unadorned = false;

	// The actual type of layer we will use depends on opacity, adornedness,
	// type and ink.
	MCLayerModeHint t_layer_mode;
	if (m_layer_mode_hint == kMCLayerModeHintStatic)
	{
		// To be a static layer, we must have an ink that is GXcopy or
		// GXblendSrcOver.
		if (ink == GXcopy || ink == GXblendSrcOver)
			t_layer_mode = kMCLayerModeHintStatic;
		else
			t_layer_mode = kMCLayerModeHintDynamic;
	}
	else if (m_layer_mode_hint == kMCLayerModeHintDynamic)
	{
		// There is no restriction on what control props can be to be
		// a dynamic layer.
		t_layer_mode = kMCLayerModeHintDynamic;
	}
	else if (m_layer_mode_hint == kMCLayerModeHintScrolling)
	{
		// A scrolling layer must be unadorned and a group.
		if (gettype() == CT_GROUP && t_is_unadorned)
			t_layer_mode = kMCLayerModeHintScrolling;
		else
			t_layer_mode = kMCLayerModeHintDynamic;
	}
	else if (m_layer_mode_hint == kMCLayerModeHintContainer)
	{
		// A container layer must be unadorned, non-opaque and a group.
		if (gettype() == CT_GROUP && !t_is_opaque && t_is_unadorned)
			t_layer_mode = kMCLayerModeHintContainer;
		else
			t_layer_mode = kMCLayerModeHintStatic;
	}
    else
    {
        MCUnreachableReturn(m_layer_mode);
    }

	// Now compute the sprite attribute.
	bool t_is_sprite;
	if (t_layer_mode == kMCLayerModeHintDynamic || t_layer_mode == kMCLayerModeHintScrolling)
		t_is_sprite = true;
	else
		t_is_sprite = false;

	// And now the direct attribute.
	bool t_is_direct;
	if (t_is_sprite)
	{
		// An unadorned image or button are direct.
		if (t_is_unadorned && (gettype() == CT_IMAGE || gettype() == CT_BUTTON))
			t_is_direct = true;
		else
			t_is_direct = false;
	}
	else
		t_is_direct = false;

	// Finally, sync the attribtues.
	if (p_commit)
	{
		m_layer_mode = t_layer_mode;
		m_layer_is_opaque = t_is_opaque;
		m_layer_is_unadorned = t_is_unadorned;
		m_layer_is_sprite = t_is_sprite;
		m_layer_is_direct = t_is_direct;

		// We've updated the layer attrs now - yay!
		m_layer_attr_changed = false;
	}

	return m_layer_mode;
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
	
    /* If the control is visible and selected, make sure we dirty the selection
     * layer. */
    if (t_is_visible && getselected())
    {
		getcard()->dirtyselection(p_old_rect);
        getcard()->dirtyselection(rect);
    }
    
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
			getcard()->layer_dirtyrect(geteffectiverect());
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
		getcard()->layer_dirtyrect(MCU_intersect_rect(p_updated_rect, geteffectiverect()));
}

void MCControl::layer_dirtyeffectiverect(const MCRectangle& p_effective_rect, bool p_update_card)
{
	// The dirty rect will be the input effective rect expanded by any effects
	// applied by the parent groups (if any).
	MCRectangle t_dirty_rect;
	t_dirty_rect = p_effective_rect;
    
    // Fetch the tilecache we are using (if any).
    MCTileCacheRef t_tilecache;
    t_tilecache = getstack() -> view_gettilecache();

	// Expand the effective rect by that of all parent groups or controls up
    // until we hit a layer (if tilecache is present).
	MCControl *t_control;
	t_control = this;
	while(t_control -> parent -> gettype() != CT_CARD &&
          (t_tilecache == nullptr || t_control->m_layer_id == 0))
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

	// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
	// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
	MCGAffineTransform t_transform;
	t_transform = getstack()->getdevicetransform();
	
	MCRectangle32 t_device_rect;
	t_device_rect = MCRectangle32GetTransformedBounds(t_dirty_rect, t_transform);
	
    // Notify any tilecache of the changes
    // If the layer id is zero, there is nothing to do.
	if (t_tilecache != nil && t_control -> m_layer_id != 0)
	{
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
	// false if the object was invisible) - parent must be a card here due to the
    // above loop.
	if (p_update_card)
		t_control->getcard()->layer_dirtyrect(t_dirty_rect);
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

	// Fetch the tilecache, making it nil if the parent is a non-container group
    // (in the latter case, this is just a dirty op).
	MCTileCacheRef t_tilecache;
	if (parent->gettype() == CT_CARD ||
        (parent->gettype() == CT_GROUP && parent.GetAs<MCGroup>()->layer_iscontainer()))
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
        MCCard *t_card = getcard();
		t_card->layer_dirtyrect(p_old_effective_rect);
		t_card->layer_dirtyrect(t_new_effective_rect);
	}
	
	// If the layer id is 0, then there is nothing to do.
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

void MCCard::layer_added(MCControl *p_control, MCControl *p_previous, MCControl *p_next)
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

		// Recompute the control's attributes.
		p_control -> layer_computeattrs(true);

		// If the control is on a dynamic layer there is nothing to do (sprites will
		// be created implicitly at first render).
		if (p_control -> layer_issprite())
			return;

		// If the control is not being added between two existing layers then there
		// is nothing to do.
		if (p_previous == nil || p_next == nil)
			return;

		// skip previous container layers
		while (p_previous != nil && p_previous->layer_iscontainer())
			p_previous = MCControlPreviousByLayer(p_previous);

		// If the previous (layer below) objptr has no id, then there is nothing to do
		// also. This will only occur if only new layers have been added above,
		// or if no rendering has been done yet.
		uint32_t t_before_layer_id;
		t_before_layer_id = p_previous -> layer_getid();
		if (t_before_layer_id == 0)
			return;

		// MW-2013-06-21: [[ Bug 10974 ]] If the previous layer is a sprite then this layer
		//   will change the lower limit of the scenery layers above, thus there is
		//   nothing to do.
		if (p_previous -> layer_issprite())
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
		if (!p_control->layer_iscontainer())
			p_control -> layer_setid(t_before_layer_id);
	}
}

void MCCard::layer_removed(MCControl *p_control, MCControl *p_previous, MCControl *p_next)
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
		
		// skip previous container layers
		while (p_previous != nil && p_previous->layer_iscontainer())
			p_previous = MCControlPreviousByLayer(p_previous);
		
		// skip next container layers
		while (p_next != nil && p_next->layer_iscontainer())
			p_next = MCControlNextByLayer(p_next);
		// If there is no previous or next control we have no tweaks to ids
		// to perform.
		if (p_previous == nil || p_next == nil)
			return;

		// Now layer ids for new layers percolate from the next layer, so the
		// original layers are always at the bottom of the stack. If the next
		// layer has a different id than us, make sure all previous layers
		// with the same id match it.
		uint32_t t_before_layer_id;
		t_before_layer_id = p_previous -> layer_getid();

		// The layer below us has the same id so there's nothing to do, we are
		// removing a 'new' layer before its been redrawn.
		if (t_before_layer_id == p_control -> layer_getid())
			return;
		
		// MW-2013-06-21: [[ Bug 10974 ]] If the layer below is a sprite, then removing
		//   this layer will increase the lower limit of the scenery stack above
		//   thus there is nothing to do.
		if (p_previous -> layer_issprite())
			return;

		// The layer below us has a different id, so this is an existing layer
		// and thus we must ensure all layers above us now use the id of the
		// layer below.
		while(p_next != nil &&
				!p_next->layer_issprite() &&
				p_next->layer_getid() == p_control->layer_getid())
		{
			if (!p_next->layer_iscontainer())
				p_next->layer_setid(t_before_layer_id);
			p_next = MCControlNextByLayer(p_next);
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

void MCCard::layer_dirtyrect(const MCRectangle& p_dirty_rect)
{
	getstack() -> dirtyrect(p_dirty_rect);
}

////////////////////////////////////////////////////////////////////////////////

// IM-2013-08-21: [[ ResIndependence ]] callback wrapper function to create scaled MCContext
// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
typedef bool (*MCTileCacheDeviceRenderCallback)(void *context, MCContext *target, const MCRectangle& region);
static bool tilecache_device_renderer(MCTileCacheDeviceRenderCallback p_callback, void *p_context, MCGContextRef p_target, const MCRectangle32 &p_rectangle, bool p_is_sprite)
{
	MCControl *t_control;
	t_control = static_cast<MCControl*>(p_context);
	
	// IM-2013-09-30: [[ FullscreenMode ]] Apply stack transform to device context
	MCGAffineTransform t_transform;
	t_transform = t_control->getstack()->getdevicetransform();
	
	// MW-2013-10-29: [[ Bug 11329 ]] Tilecache expects sprite rects to be
	//   relative to top-left of sprite.
	if (p_is_sprite)
	{
		t_transform . tx = 0.0f;
		t_transform . ty = 0.0f;
	}
	
	MCGContextSave(p_target);
	MCGContextConcatCTM(p_target, t_transform);
	
	MCGraphicsContext *t_gfx_context;
	/* UNCHECKED */ t_gfx_context = new (nothrow) MCGraphicsContext(p_target);
	
	MCRectangle t_user_rect;
	t_user_rect = MCRectangleGetTransformedBounds(p_rectangle, MCGAffineTransformInvert(t_transform));
    
	bool t_success;
	t_success = p_callback(p_context, t_gfx_context, t_user_rect);
	
	delete t_gfx_context;
	
	MCGContextRestore(p_target);
	
	return t_success;
}

static bool testtilecache_sprite_renderer(void *p_context, MCContext *p_target, const MCRectangle& p_rectangle)
{
	MCControl *t_control;
	t_control = (MCControl *)p_context;
				
	// A scrolling layer is an unadorned group.
	bool t_scrolling;
	t_scrolling = t_control -> layer_isscrolling();

	MCRectangle t_control_rect, t_dirty_rect;
	if (!t_scrolling)
	{
		t_control_rect = t_control -> geteffectiverect();
		t_dirty_rect = MCU_intersect_rect(t_control_rect, MCU_offset_rect(p_rectangle, t_control_rect . x, t_control_rect . y));
	}
	else
	{
		t_control_rect = t_control -> layer_getcontentrect();
		t_dirty_rect = MCU_intersect_rect(t_control_rect, MCU_offset_rect(p_rectangle, t_control_rect . x, t_control_rect . y));
	}
	
	if (MCU_empty_rect(t_dirty_rect))
		return true;
	
	// IM-2014-07-03: [[ GraphicsPerformance ]] Context origin is the topleft of the sprite so adjust to card coords.
	p_target -> setorigin(-t_control_rect . x, -t_control_rect . y);
	p_target -> cliprect(t_dirty_rect);
	p_target -> setfunction(GXcopy);
	p_target -> setopacity(255);

	t_control -> draw(p_target, t_dirty_rect, false, true);

	return true;
}

static bool testtilecache_device_sprite_renderer(void *p_context, MCGContextRef p_target, const MCRectangle32& p_rectangle)
{
	return tilecache_device_renderer(testtilecache_sprite_renderer, p_context, p_target, p_rectangle, true);
}

static bool testtilecache_scenery_renderer(void *p_context, MCContext *p_target, const MCRectangle& p_rectangle)
{
	MCControl *t_control;
	t_control = (MCControl *)p_context;
    
    p_target->setclip(t_control->getstack()->getvisiblerect());

	// IM-2014-07-02: [[ GraphicsPerformance ]] Use the redraw() method instead of 
	// reproducing the visibility tests and context clipping here.
	t_control->redraw(p_target, p_rectangle);

	return true;
}

static bool testtilecache_device_scenery_renderer(void *p_context, MCGContextRef p_target, const MCRectangle32& p_rectangle)
{
	return tilecache_device_renderer(testtilecache_scenery_renderer, p_context, p_target, p_rectangle, false);
}

bool MCCard::tilecache_render_foreground(void *p_context, MCContext *p_target, const MCRectangle& p_dirty)
{
	MCCard *t_card;
	t_card = (MCCard *)p_context;
	
	p_target -> setfunction(GXcopy);
	p_target -> setopacity(255);
    
    t_card -> drawcardborder(p_target, p_dirty);
    t_card -> drawselection(p_target, p_dirty);
    
	return true;
}

bool device_render_foreground(void *p_context, MCGContextRef p_target, const MCRectangle32& p_rectangle)
{
	return tilecache_device_renderer(MCCard::tilecache_render_foreground, p_context, p_target, p_rectangle, false);
}

bool MCCard::tilecache_render_background(void *p_context, MCContext *p_target, const MCRectangle& p_dirty)
{
	MCCard *t_card;
	t_card = (MCCard *)p_context;
	
	// MW-2013-10-23: [[ FullscreenMode ]] Make sure we set the clip to the visible rect.
	// IM-2013-12-20: [[ ShowAll ]] Use MCStack::getvisiblerect() to get the visible area
	MCRectangle t_visible_rect;
	t_visible_rect = t_card->getstack()->getvisiblerect();
	
	// IM-2014-07-02: [[ GraphicsPerformance ]] Use cliprect() to reduce the clipping region.
	p_target -> cliprect(t_visible_rect);
	p_target -> setfunction(GXcopy);
	p_target -> setopacity(255);

	// IM-2013-09-13: [[ RefactorGraphics ]] Use shared code to render card background
	t_card -> drawbackground(p_target, p_dirty);

	return true;
}

bool device_render_background(void *p_context, MCGContextRef p_target, const MCRectangle32& p_rectangle)
{
	return tilecache_device_renderer(MCCard::tilecache_render_background, p_context, p_target, p_rectangle, false);
}

void MCCard::render_control(MCTileCacheRef p_tiler, MCControl *p_control, const MCRectangle& p_visible_rect, const MCGAffineTransform& p_transform, bool p_parent_is_container)
{
    // Take note of whether the spriteness of a layer has changed.
    bool t_old_is_sprite;
    t_old_is_sprite = p_control -> layer_issprite();
    
    // Take not of whether the containerness of a layer has changed.
    bool t_old_is_container;
    t_old_is_container = p_control -> layer_iscontainer();
    
    // Sync the attributes, make sure we commit the new values.
    p_control -> layer_computeattrs(true);
    
    // Initialize the common layer props.
    MCTileCacheLayer t_layer;
    t_layer . id = p_control -> layer_getid();
    t_layer . opacity = p_control -> getopacity();
    t_layer . ink = p_control -> getink();
    t_layer . context = p_control;
    
    // The opaqueness of a layer has already been computed.
    t_layer . is_opaque = p_control -> layer_isopaque();
    
    // Now compute the layer's region/clip.
    MCRectangle t_layer_region, t_layer_clip;
    if (!p_control -> getflag(F_VISIBLE) && !showinvisible())
    {
        // Invisible layers just have empty region/clip.
        t_layer_region = MCU_make_rect(0, 0, 0, 0);
        t_layer_clip = MCU_make_rect(0, 0, 0, 0);
    }
    else if (!p_control -> layer_isscrolling())
    {
        // Non-scrolling layer's are the size of their effective rects.
        t_layer_region = p_control -> geteffectiverect();
        t_layer_clip = t_layer_region;
    }
    else
    {
        // For a scrolling layer, the clip is the bounds of the control, while
        // the region we draw is the group's minrect.
        t_layer_region = p_control -> layer_getcontentrect();
        t_layer_clip = p_control -> geteffectiverect();
    }
    
    // IM-2013-10-14: [[ FullscreenMode ]] Constrain each layer to the visible area
    t_layer_clip = MCU_intersect_rect(t_layer_clip, p_visible_rect);
    
    /* If the layer has a layerClipRect, then apply it here. */
    if (p_control->layer_has_clip_rect())
    {
        t_layer_clip = MCU_intersect_rect(t_layer_clip, p_control->layer_get_clip_rect());
    }
    
    // IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
    // IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
    t_layer . region = MCRectangle32GetTransformedBounds(t_layer_region, p_transform);
    t_layer . clip = MCRectangle32GetTransformedBounds(t_layer_clip, p_transform);
    
    if (t_old_is_container || (p_control->layer_iscontainer() && p_parent_is_container))
    {
        MCGroup *t_group = static_cast<MCGroup*>(p_control);
        
        MCControl *t_controls = t_group->getcontrols();
        if (t_controls != nullptr)
        {
            MCControl *t_child = t_controls->prev();
            do
            {
                render_control(p_tiler, t_child, t_layer_clip, p_transform, p_control->layer_iscontainer() && p_parent_is_container);
                
                t_child = t_child->prev();
            }
            while(t_child != t_controls->prev());
        }
    }
    
    // Only render layers if the parent is a container.
    if (p_parent_is_container)
    {
        if (!p_control->layer_iscontainer())
        {
            // Now render the layer - what method we use depends on whether the
            // layer is a sprite or not.
            if (p_control -> layer_issprite())
            {
                // If the layer was not a sprite before, remove the scenery
                // layer that it was.
                if (!t_old_is_sprite && t_layer . id != 0)
                {
                    // IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
                    // IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
                    MCTileCacheRemoveScenery(p_tiler, t_layer . id, t_layer . region);
                    t_layer . id = 0;
                }
                
                t_layer . callback = testtilecache_device_sprite_renderer;
                MCTileCacheRenderSprite(p_tiler, t_layer);
            }
            else
            {
                // If the layer was a sprite before, remove the sprite
                // layer that it was.
                if (t_old_is_sprite && t_layer . id != 0)
                {
                    MCTileCacheRemoveSprite(p_tiler, t_layer . id);
                    t_layer . id = 0;
                }
                
                // MW-2013-10-29: [[ Bug 11349 ]] Scenery layers regions are clipped
                //   by the clip directly.
                t_layer . region = MCRectangle32Intersect(t_layer . region, t_layer . clip);
                
                t_layer . callback = testtilecache_device_scenery_renderer;
                MCTileCacheRenderScenery(p_tiler, t_layer);
            }
        }
    }
    else
    {
        if (t_layer.id != 0)
        {
            if (p_control->layer_issprite())
            {
                MCTileCacheRemoveSprite(p_tiler, t_layer . id);
            }
            else
            {
                MCTileCacheRemoveScenery(p_tiler, t_layer . id, t_layer . region);
            }
        }
        t_layer.id = 0;
    }
    
    MCLog("Layer %d - sprite %d, container %d, region %d,%d,%d,%d", t_layer.id, p_control->layer_issprite(), p_control->layer_iscontainer(), t_layer.region.x, t_layer.region.y, t_layer.region.width, t_layer.region.height);
    
    // Upate the id.
    p_control -> layer_setid(t_layer . id);
}

void MCCard::render_control_reset_ids(MCControl *p_control)
{
    p_control->layer_resetattrs();
    
    if (p_control->gettype() == CT_GROUP)
    {
        MCGroup *t_group = static_cast<MCGroup*>(p_control);
        
        MCControl *t_controls = t_group->getcontrols();
        if (t_controls != nullptr)
        {
            MCControl *t_child = t_controls;
            do
            {
                render_control_reset_ids(t_child);
                t_child = t_child->next();
            }
            while(t_child != t_controls->prev());
        }
    }
}

void MCCard::render(void)
{
	MCTileCacheRef t_tiler;
	t_tiler = getstack() -> view_gettilecache();
    
    MCObjptr *t_objptrs;
    t_objptrs = getobjptrs();
    
	if (t_objptrs != nullptr && MCTileCacheIsClean(t_tiler))
    {
        MCObjptr *t_objptr;
        t_objptr = t_objptrs -> prev();
        do
        {
            MCControl *t_control;
            t_control = t_objptr -> getref();
            
            render_control_reset_ids(t_control);
            
            t_objptr = t_objptr -> prev();
        }
        while(t_objptr != t_objptrs -> prev());
    }

	// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
	MCGAffineTransform t_transform;
	t_transform = getstack()->getdevicetransform();
	
	// IM-2013-10-14: [[ FullscreenMode ]] Get the visible area of the stack
	// IM-2013-12-20: [[ ShowAll ]] Use MCStack::getvisiblerect() to get the visible area
	MCRectangle t_visible_rect;
	t_visible_rect = getstack()->getvisiblerect();
    
    MCTileCacheLayer t_fg_layer;
    t_fg_layer . id = m_fg_layer_id;
    t_fg_layer . region = MCRectangle32GetTransformedBounds(rect, t_transform);
    t_fg_layer . clip = MCRectangle32GetTransformedBounds(t_visible_rect, t_transform);
    t_fg_layer . is_opaque = false;
    t_fg_layer . opacity = 255;
    t_fg_layer . ink = GXblendSrcOver;
    t_fg_layer . callback = device_render_foreground;
    t_fg_layer . context = this;
    MCTileCacheRenderScenery(t_tiler, t_fg_layer);
    m_fg_layer_id = t_fg_layer . id;
    
	if (t_objptrs != nil)
	{
		MCObjptr *t_objptr;
		t_objptr = t_objptrs -> prev();
		do
		{
			MCControl *t_control;
			t_control = t_objptr -> getref();
            
            render_control(t_tiler, t_control, t_visible_rect, t_transform, true);

			// Advance to the object below.
			t_objptr = t_objptr -> prev();
		}
		while(t_objptr != t_objptrs -> prev());
	}

	// IM-2013-10-14: [[ FullscreenMode ]] Render the background into the card's visible area
	MCTileCacheLayer t_bg_layer;
	t_bg_layer . id = m_bg_layer_id;
	t_bg_layer . region = MCRectangle32GetTransformedBounds(t_visible_rect, t_transform);
	t_bg_layer . clip = t_bg_layer . region;
	t_bg_layer . is_opaque = true;
	t_bg_layer . opacity = 255;
	t_bg_layer . ink = GXblendSrcOver;
	t_bg_layer . callback = device_render_background;
	t_bg_layer . context = this;
	MCTileCacheRenderScenery(t_tiler, t_bg_layer);
	m_bg_layer_id = t_bg_layer . id;
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
		sptr -> dirtyall();
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

MCControl *MCControlTopChildByLayer(MCControl *p_control)
{
	if (p_control->gettype() == CT_GROUP)
	{
		MCControl *t_controls = static_cast<MCGroup*>(p_control)->getcontrols();

		if (t_controls != nil)
			return MCControlTopChildByLayer(t_controls->prev());
	}

	return p_control;
}

MCControl *MCControlPreviousByLayer(MCControl *p_control)
{
	MCObject *t_parent = nil;
	t_parent = p_control->getparent();
	
	if (t_parent == nil || t_parent->gettype() == CT_STACK)
		return nil;
	
	if (t_parent->gettype() == CT_GROUP)
	{
		MCControl *t_controls = static_cast<MCGroup*>(t_parent)->getcontrols();

		if (t_controls == p_control)
			return static_cast<MCControl*>(t_parent);
		else
			return MCControlTopChildByLayer(p_control->prev());
	}
	else if (t_parent->gettype() == CT_CARD)
	{
		MCObjptr *t_object = static_cast<MCCard*>(t_parent)->getobjptrforcontrol(p_control);
		if (t_object == nil)
			return nil; // object not on card
		
		// check if first object on card
		if (t_object == static_cast<MCCard*>(t_parent)->getobjptrs())
			return nil;
		
		return MCControlTopChildByLayer(static_cast<MCControl*>(t_object->prev()->getref()));
	}
	else
	{
		// control not in group or on card
		return nil;
	}
}

MCControl *MCControlNextByLayer(MCControl *p_control)
{
	if (p_control->gettype() == CT_GROUP)
	{
		MCControl *t_controls = static_cast<MCGroup*>(p_control)->getcontrols();
		if (t_controls != nil)
			return t_controls;
	}
	
	MCControl *t_child = p_control;
	MCObject *t_parent = t_child->getparent();
	
	while (true)
	{
		if (t_parent == nil || t_parent->gettype() == CT_STACK)
			return nil;
		
		if (t_parent->gettype() == CT_GROUP)
		{
			MCControl *t_controls = static_cast<MCGroup*>(t_parent)->getcontrols();
			if (t_controls->prev() != t_child)
				return t_child->next();
		}
		else if (t_parent->gettype() == CT_CARD)
		{
			MCObjptr *t_obj = static_cast<MCCard*>(t_parent)->getobjptrforcontrol(t_child);
			if (t_obj == nil)
				return nil; // object no longer on card
			
			MCObjptr *t_next = t_obj->next();
			// check for wrap-around
			if (t_next == static_cast<MCCard*>(t_parent)->getobjptrs())
				return nil;
			else
				return static_cast<MCControl*>(t_next->getref());
		}
		
		t_child = static_cast<MCControl*>(t_parent);
		t_parent = t_child->getparent();
	}
}

