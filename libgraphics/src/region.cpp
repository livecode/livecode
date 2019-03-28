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

#include "graphics.h"
#include "graphics-internal.h"

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

bool MCGRegionCreate(MCGRegionRef &r_region)
{
	bool t_success;
	t_success = true;
	
	MCGRegionRef t_region;
	t_region = nil;
	
	if (t_success)
		t_success = MCMemoryCreate(t_region);

	if (t_success)
		r_region = t_region;
	else
		MCGRegionDestroy(t_region);
	
	return t_success;
}

void MCGRegionDestroy(MCGRegionRef p_region)
{
	if (p_region == nil)
		return;
	
	MCMemoryDestroy(p_region);
}

bool MCGRegionCopy(MCGRegionRef p_region, MCGRegionRef &r_copy)
{
	if (p_region == nil)
		return false;
	
	bool t_success;
	t_success = true;
	
	MCGRegionRef t_copy;
	t_copy = nil;
	
	if (t_success)
		t_success = MCGRegionCreate(t_copy) && MCGRegionSetRegion(t_copy, p_region);
	
	if (t_success)
		r_copy = t_copy;
	else
		MCGRegionDestroy(t_copy);
	
	return t_success;
}

//////////

bool MCGRegionIsEmpty(MCGRegionRef p_region)
{
	return p_region == nil || p_region->region.isEmpty();
}

//////////

MCGIntegerRectangle MCGRegionGetBounds(MCGRegionRef p_region)
{
	if (p_region == nil)
		return MCGIntegerRectangleMake(0, 0, 0, 0);
	
	SkIRect t_irect;
	t_irect = p_region->region.getBounds();
	
	return MCGIntegerRectangleMake(t_irect.x(), t_irect.y(), t_irect.width(), t_irect.height());
}

//////////

bool MCGRegionSetEmpty(MCGRegionRef p_region)
{
	if (p_region == nil)
		return false;
	
	p_region->region.setEmpty();
	
	return true;
}

bool MCGRegionSetRect(MCGRegionRef p_region, const MCGIntegerRectangle &p_rect)
{
	if (p_region == nil)
		return false;
	
	p_region->region.setRect(p_rect.origin.x, p_rect.origin.y, p_rect.origin.x + p_rect.size.width, p_rect.origin.y + p_rect.size.height);
	
	return true;
}

bool MCGRegionSetRegion(MCGRegionRef p_region, MCGRegionRef p_other)
{
	if (p_region == nil || p_other == nil)
		return false;
	
	p_region->region.setRegion(p_other->region);
	
	return true;
}

//////////

bool MCGRegionAddRect(MCGRegionRef p_region, const MCGIntegerRectangle &p_rect)
{
	if (p_region == nil)
		return false;
	
	p_region->region.op(p_rect.origin.x, p_rect.origin.y, p_rect.origin.x + p_rect.size.width, p_rect.origin.y + p_rect.size.height, SkRegion::kUnion_Op);
	
	return true;
}

bool MCGRegionAddRegion(MCGRegionRef p_region, MCGRegionRef p_other)
{
	if (p_region == nil || p_other == nil)
		return false;
	
	p_region->region.op(p_other->region, SkRegion::kUnion_Op);
	
	return true;
}

bool MCGRegionIntersectRect(MCGRegionRef p_region, const MCGIntegerRectangle &p_rect)
{
	if (p_region == nil)
		return false;
	
	p_region->region.op(p_rect.origin.x, p_rect.origin.y, p_rect.origin.x + p_rect.size.width, p_rect.origin. y + p_rect.size.height, SkRegion::kIntersect_Op);
	
	return true;
}

bool MCGRegionIntersectRegion(MCGRegionRef p_region, MCGRegionRef p_other)
{
	if (p_region == nil || p_other == nil)
		return false;
	
	p_region->region.op(p_other->region, SkRegion::kIntersect_Op);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCGRegionTranslate(MCGRegionRef p_region, int32_t p_dx, int32_t p_dy)
{
	if (p_region == nil)
		return false;
	
	p_region->region.translate(p_dx, p_dy);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCGRegionIterate(MCGRegionRef p_region, MCGRegionIterateCallback p_callback, void *p_context)
{
	if (p_region == nil)
		return false;

	bool t_success;
	t_success = true;
	
	SkRegion::Iterator t_iterator(p_region->region);
	
	while (t_success && !t_iterator.done())
	{
		const SkIRect &t_rect = t_iterator.rect();
		t_success = p_callback(p_context, MCGIntegerRectangleMake(t_rect.x(), t_rect.y(), t_rect.width(), t_rect.height()));
		t_iterator.next();
	}
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

struct MCGRegionCopyWithTransformIterateContext
{
	MCGRegionRef region;
	MCGAffineTransform transform;
};

bool MCGRegionCopyWithTransformIterateCallback(void *p_context, const MCGIntegerRectangle &p_rect)
{
	MCGRegionCopyWithTransformIterateContext *t_context;
	t_context = static_cast<MCGRegionCopyWithTransformIterateContext*>(p_context);
	
	return MCGRegionAddRect(t_context->region, MCGIntegerRectangleGetTransformedBounds(p_rect, t_context->transform));
}

bool MCGRegionCopyWithTransform(MCGRegionRef p_region, const MCGAffineTransform &p_transform, MCGRegionRef &r_copy)
{
	if (p_region == nil)
		return false;
	
	MCGRegionRef t_copy;
	t_copy = nil;
	
	if (!MCGRegionCreate(t_copy))
		return false;
	
	if (MCGAffineTransformIsRectangular(p_transform))
	{
		MCGRegionCopyWithTransformIterateContext t_context;
		t_context.region = t_copy;
		t_context.transform = p_transform;
		
		if (MCGRegionIterate(p_region, MCGRegionCopyWithTransformIterateCallback, &t_context))
		{
			r_copy = t_copy;
			return true;
		}
		else
		{
			MCGRegionDestroy(t_copy);
			return false;
		}
	}
	else
	{
		SkMatrix t_matrix;
		MCGAffineTransformToSkMatrix(p_transform, t_matrix);
		
		SkPath t_path;
		p_region->region.getBoundaryPath(&t_path);
		t_path.transform(t_matrix);
		
		SkRect t_bounds;
		t_bounds = t_path.getBounds();
		
		SkRegion t_clip;
		t_clip.setRect(floorf(t_bounds.left()), floorf(t_bounds.top()), ceilf(t_bounds.right()), ceilf(t_bounds.bottom()));
		
		t_copy->region.setPath(t_path, t_clip);
        
        r_copy = t_copy;
		
		return true;
	}
}

////////////////////////////////////////////////////////////////////////////////
