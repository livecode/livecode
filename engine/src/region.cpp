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
	return MCGRegionCreate(r_region);
}

void MCRegionDestroy(MCRegionRef p_region)
{
	MCGRegionDestroy(p_region);
}

//////////

MCRectangle MCRegionGetBoundingBox(MCRegionRef p_region)
{
	return MCRectangleFromMCGIntegerRectangle(MCGRegionGetBounds(p_region));
}

//////////

bool MCRegionIsEmpty(MCRegionRef p_region)
{
	return MCGRegionIsEmpty(p_region);
}

//////////

bool MCRegionSetEmpty(MCRegionRef p_region)
{
	return MCGRegionSetEmpty(p_region);
}

bool MCRegionSetRect(MCRegionRef p_region, const MCRectangle &p_rect)
{
	return MCGRegionSetRect(p_region, MCRectangleToMCGIntegerRectangle(p_rect));
}

//////////

bool MCRegionIncludeRect(MCRegionRef p_region, const MCRectangle &p_rect)
{
	return MCGRegionAddRect(p_region, MCRectangleToMCGIntegerRectangle(p_rect));
}

bool MCRegionAddRegion(MCRegionRef p_region, MCRegionRef p_other)
{
	return MCGRegionAddRegion(p_region, (MCGRegionRef)p_other);
}

////////////////////////////////////////////////////////////////////////////////

struct MCRegionTransformContext
{
	MCGRegionRef region;
	MCGAffineTransform transform;
};

bool MCRegionTransformCallback(void *p_context, const MCRectangle &p_rect)
{
	MCRegionTransformContext *t_context;
	t_context = static_cast<MCRegionTransformContext*>(p_context);
	
	MCRectangle t_transformed_rect;
	t_transformed_rect = MCRectangleGetTransformedBounds(p_rect, t_context->transform);
	
	return MCGRegionAddRect(t_context->region, MCRectangleToMCGIntegerRectangle(t_transformed_rect));
}

bool MCRegionTransform(MCRegionRef p_region, const MCGAffineTransform &p_transform, MCRegionRef &r_transformed_region)
{
	return MCGRegionCopyWithTransform((MCGRegionRef)p_region, p_transform, (MCGRegionRef&)r_transformed_region);
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
	
	return MCGRegionIterate(p_region, MCRegionForEachRectIterator, &t_context);
}

////////////////////////////////////////////////////////////////////////////////
