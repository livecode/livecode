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

#include "region.h"

#include "graphics.h"
#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

bool MCRegionCreate(MCRegionRef &r_region)
{
	return MCGRegionCreate((MCGRegionRef&)r_region);
}

void MCRegionDestroy(MCRegionRef p_region)
{
	MCGRegionDestroy((MCGRegionRef)p_region);
}

//////////

MCRectangle MCRegionGetBoundingBox(MCRegionRef p_region)
{
	return MCRectangleFromMCGIntegerRectangle(MCGRegionGetBounds((MCGRegionRef)p_region));
}

//////////

bool MCRegionIsEmpty(MCRegionRef p_region)
{
	return MCGRegionIsEmpty((MCGRegionRef)p_region);
}

//////////

bool MCRegionSetEmpty(MCRegionRef p_region)
{
	return MCGRegionSetEmpty((MCGRegionRef)p_region);
}

bool MCRegionSetRect(MCRegionRef p_region, const MCRectangle &p_rect)
{
	return MCGRegionSetRect((MCGRegionRef)p_region, MCRectangleToMCGIntegerRectangle(p_rect));
}

//////////

bool MCRegionIncludeRect(MCRegionRef p_region, const MCRectangle &p_rect)
{
	return MCGRegionAddRect((MCGRegionRef)p_region, MCRectangleToMCGIntegerRectangle(p_rect));
}

bool MCRegionAddRegion(MCRegionRef p_region, MCRegionRef p_other)
{
	return MCGRegionAddRegion((MCGRegionRef)p_region, (MCGRegionRef)p_other);
}

////////////////////////////////////////////////////////////////////////////////

struct MCRegionTransformContext
{
	MCRegionRef region;
	MCGAffineTransform transform;
};

bool MCRegionTransformCallback(void *p_context, const MCRectangle &p_rect)
{
	MCRegionTransformContext *t_context;
	t_context = static_cast<MCRegionTransformContext*>(p_context);
	
	MCRectangle t_transformed_rect;
	t_transformed_rect = MCRectangleGetTransformedBounds(p_rect, t_context->transform);
	
	return MCRegionIncludeRect(t_context->region, t_transformed_rect);
}

bool MCRegionTransform(MCRegionRef p_region, const MCGAffineTransform &p_transform, MCRegionRef &r_transformed_region)
{
	return MCGRegionCopyWithTransform((MCGRegionRef)p_region, p_transform, (MCGRegionRef&)r_transformed_region);
/*	bool t_success;
	t_success = true;
	
	MCRegionRef t_new_region;
	t_new_region = nil;
	
	t_success = MCRegionCreate(t_new_region);
	
	if (t_success)
	{
		MCRegionTransformContext t_context;
		t_context.region = t_new_region;
		t_context.transform = p_transform;
		
		t_success = MCRegionForEachRect(p_region, MCRegionTransformCallback, &t_context);
	}
	
	if (t_success)
		r_transformed_region = t_new_region;
	else
		MCRegionDestroy(t_new_region);
	
	return t_success;*/
}

//////////

struct MCRegionForEachRectContext
{
	MCRegionForEachRectCallback callback;
	void *context;
};

bool MCRegionForEachRectIterator(void *p_context, const MCGIntegerRectangle &p_rect)
{
	MCRegionForEachRectContext *t_context = static_cast<MCRegionForEachRectContext*>(p_context);
	
	return t_context->callback(t_context->context, MCRectangleFromMCGIntegerRectangle(p_rect));
}

bool MCRegionForEachRect(MCRegionRef p_region, MCRegionForEachRectCallback p_callback, void *p_context)
{
	MCRegionForEachRectContext t_context;
	t_context.callback = p_callback;
	t_context.context = p_context;
	
	return MCGRegionIterate((MCGRegionRef)p_region, MCRegionForEachRectIterator, &t_context);
}

////////////////////////////////////////////////////////////////////////////////
// Legacy functions

bool MCRegionUnion(MCRegionRef p_dest, MCRegionRef p_a, MCRegionRef p_b)
{
	bool t_success;
	t_success = true;
	
	MCGRegionRef t_region;
	t_region = nil;
	
	if (t_success)
		t_success = MCGRegionCopy((MCGRegionRef)p_a, t_region);
	
	if (t_success)
		t_success = MCGRegionAddRegion(t_region, (MCGRegionRef)p_b);
	
	if (t_success)
		t_success = MCGRegionSetRegion((MCGRegionRef)p_dest, t_region);
	
	MCGRegionDestroy(t_region);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
