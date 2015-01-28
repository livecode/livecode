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

#include "graphics.h"
#include "graphics-internal.h"

#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkBitmap.h>
#include <SkDashPathEffect.h>

////////////////////////////////////////////////////////////////////////////////

static void MCGPathDestroy(MCGPathRef self)
{
	if (self != NULL)
	{
		if (self -> path != NULL)
			delete self -> path;
	}
	MCMemoryDelete(self);
}

bool MCGPathCreate(const MCGPathCommand *commands, const MCGFloat *parameters, MCGPathRef& r_path)
{
	// TODO: Implement
	return false;
}

bool MCGPathCreateMutable(MCGPathRef& r_path)
{
	bool t_success;
	t_success = true;
	
	__MCGPath *t_path;
	t_path = NULL;
	
	if (t_success)
		t_success = MCMemoryNew(t_path);
	
	if (t_success)
	{
		t_path -> path = new SkPath();
		t_success = t_path -> path != NULL;
	}
	
	if (t_success)
	{
		t_path -> is_valid = true;
		t_path -> references = 1;
		t_path -> is_mutable = true;
	}
	
	if (t_success)
		r_path = t_path;
	else
	{
		MCGPathDestroy(t_path);
		r_path = NULL;
	}
	
	return t_success;
}

MCGPathRef MCGPathRetain(MCGPathRef self)
{
	if (self != NULL)
		self -> references++;
	return self;
}

void MCGPathRelease(MCGPathRef self)
{
	if (self != NULL)
	{
		self -> references--;
		if (self -> references <= 0)
			MCGPathDestroy(self);
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCGPathIsValid(MCGPathRef self)
{
	return self != NULL && 	self -> is_valid;
}

bool MCGPathIsMutable(MCGPathRef self)
{
	return self -> is_mutable;
}

bool MCGPathIsEmpty(MCGPathRef self)
{
	return self -> path -> isEmpty();
}

bool MCGPathIsEqualTo(MCGPathRef a, MCGPathRef b)
{
	return *a->path == *b->path;
}

////////////////////////////////////////////////////////////////////////////////

void MCGPathCopy(MCGPathRef self, MCGPathRef& r_new_path)
{
	if(!self -> is_mutable)
		r_new_path = MCGPathRetain(self);
	else
	{
		MCGPathRef t_new_path;
		MCGPathCreateMutable(t_new_path);
		if (MCGPathIsValid(t_new_path))
			MCGPathAddPath(t_new_path, self);
		if (MCGPathIsValid(t_new_path))
		{	
			t_new_path -> is_mutable = false;
			r_new_path = t_new_path;
		}
	}
}

void MCGPathCopyAndRelease(MCGPathRef self, MCGPathRef& r_new_path)
{
	if (!self -> is_mutable)
		r_new_path = self;
	else if (self -> references == 1)
	{
		self -> is_mutable = false;
		r_new_path = self;
	}
	else
	{
		MCGPathRef t_new_path;
		MCGPathCreateMutable(t_new_path);
		if (MCGPathIsValid(t_new_path))
			MCGPathAddPath(t_new_path, self);
		if (MCGPathIsValid(t_new_path))
		{	
			t_new_path -> is_mutable = false;
			r_new_path = t_new_path;
		}
		MCGPathRelease(self);
	}		
}

void MCGPathMutableCopy(MCGPathRef self, MCGPathRef& r_new_path)
{
	MCGPathRef t_new_path;
	MCGPathCreateMutable(t_new_path);
	if (MCGPathIsValid(t_new_path))
		MCGPathAddPath(t_new_path, self);
	if (MCGPathIsValid(t_new_path))
		r_new_path = t_new_path;
}

void MCGPathMutableCopyAndRelease(MCGPathRef self, MCGPathRef& r_new_path)
{
	MCGPathMutableCopy(self, r_new_path);
	if (MCGPathIsValid(r_new_path))	
		MCGPathRelease(self);
}

////////////////////////////////////////////////////////////////////////////////

struct MCGPathMutableCopySubpathsContext
{
	int32_t first_subpath;
	int32_t last_subpath;
	int32_t current_subpath;
	
	MCGPathRef path;
	
	bool done;
};

bool MCGPathMutableCopySubpathsCallback(void *p_context, MCGPathCommand p_command, MCGPoint *p_points, uint32_t p_point_count)
{
	MCGPathMutableCopySubpathsContext *t_context;
	t_context = static_cast<MCGPathMutableCopySubpathsContext*>(p_context);
	
	if (p_command == kMCGPathCommandMoveTo)
		t_context->current_subpath++;

	// skip processing if subpath outside the requested range
	if (t_context->current_subpath < t_context->first_subpath)
		return true;
	// end iteration after last requested subpath processed
	if (t_context->current_subpath > t_context->last_subpath)
	{
		t_context->done = true;
		return false;
	}
	
	switch (p_command)
	{
		case kMCGPathCommandMoveTo:
			MCGPathMoveTo(t_context->path, p_points[0]);
			break;
			
		case kMCGPathCommandLineTo:
			MCGPathLineTo(t_context->path, p_points[0]);
			break;
			
		case kMCGPathCommandQuadCurveTo:
			MCGPathQuadraticTo(t_context->path, p_points[0], p_points[1]);
			break;
			
		case kMCGPathCommandCubicCurveTo:
			MCGPathCubicTo(t_context->path, p_points[0], p_points[1], p_points[2]);
			break;

		case kMCGPathCommandCloseSubpath:
			MCGPathCloseSubpath(t_context->path);
			break;
			
		case kMCGPathCommandEnd:
			t_context->done = true;
			break;
			
		default:
			MCAssert(false);
	}
	
	return MCGPathIsValid(t_context->path);
}

bool MCGPathMutableCopySubpaths(MCGPathRef self, uint32_t p_first, uint32_t p_last, MCGPathRef &r_subpaths)
{
	if (!MCGPathIsValid(self))
		return false;
	
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
		t_success = MCGPathCreateMutable(t_path);
	
	MCGPathMutableCopySubpathsContext t_context;
	if (t_success)
	{
		t_context.first_subpath = p_first;
		t_context.last_subpath = p_last;
		t_context.current_subpath = -1;
		t_context.path = t_path;
		t_context.done = false;
		
		t_success = MCGPathIterate(self, MCGPathMutableCopySubpathsCallback, &t_context) || t_context.done;
	}
	
	if (t_success)
		r_subpaths = t_path;
	else
		MCGPathRelease(t_path);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCGPathAddRectangle(MCGPathRef self, MCGRectangle p_bounds)
{	
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
	{
		SkRect t_bounds;
		t_bounds = MCGRectangleToSkRect(p_bounds);
		self -> path -> addRect(t_bounds);
	}
	
	self -> is_valid = t_success;
}

void MCGPathAddRoundedRectangle(MCGPathRef self, MCGRectangle p_bounds, MCGSize p_corner_radii)
{	
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
	{
		SkRect t_bounds;
		t_bounds = MCGRectangleToSkRect(p_bounds);
		self -> path -> addRoundRect(t_bounds, MCGFloatToSkScalar(p_corner_radii . width), MCGFloatToSkScalar(p_corner_radii . height));
	}
	
	self -> is_valid = t_success;
}

void MCGPathAddEllipse(MCGPathRef self, MCGPoint p_center, MCGSize p_radii, MCGFloat p_rotation)
{
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
	{
		SkRect t_bounds;
		t_bounds = SkRect::MakeXYWH(MCGCoordToSkCoord(p_center . x - (p_radii . width * 0.5f)), MCGCoordToSkCoord(p_center . y - (p_radii . height * 0.5f)),
									MCGFloatToSkScalar(p_radii . width), MCGFloatToSkScalar(p_radii . height));
		self -> path -> addOval(t_bounds);
		
		// TODO: Handle rotation
	}
	
	self -> is_valid = t_success;
}

void MCGPathAddArc(MCGPathRef self, MCGPoint p_center, MCGSize p_radii, MCGFloat p_rotation, MCGFloat p_start_angle, MCGFloat p_finish_angle)
{	
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
	{
		SkRect t_bounds;
		t_bounds = SkRect::MakeXYWH(MCGCoordToSkCoord(p_center . x - (p_radii . width * 0.5f)), MCGCoordToSkCoord(p_center . y - (p_radii . height * 0.5f)),
									MCGFloatToSkScalar(p_radii . width), MCGFloatToSkScalar(p_radii . height));
		self -> path -> addArc(t_bounds, MCGFloatToSkScalar(p_start_angle), MCGFloatToSkScalar(p_finish_angle - p_start_angle));
		
		// TODO: Handle rotation
	}
	
	self -> is_valid = t_success;
}

void MCGPathAddSector(MCGPathRef self, MCGPoint p_center, MCGSize p_radii, MCGFloat p_rotation, MCGFloat p_start_angle, MCGFloat p_finish_angle)
{
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
	{		
		SkRect t_bounds;
		t_bounds = SkRect::MakeXYWH(MCGCoordToSkCoord(p_center . x - (p_radii . width * 0.5f)), MCGCoordToSkCoord(p_center . y - (p_radii . height * 0.5f)),
									MCGFloatToSkScalar(p_radii . width), MCGFloatToSkScalar(p_radii . height));
		
		self -> path -> arcTo(t_bounds, MCGFloatToSkScalar(p_start_angle), MCGFloatToSkScalar(p_finish_angle - p_start_angle), false);		
		self -> path -> close();
		
		// TODO: Handle rotation
	}
	
	self -> is_valid = t_success;
}

void MCGPathAddSegment(MCGPathRef self, MCGPoint p_center, MCGSize p_radii, MCGFloat p_rotation, MCGFloat p_start_angle, MCGFloat p_finish_angle)
{
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;	
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
	{
		SkRect t_bounds;
		t_bounds = SkRect::MakeXYWH(MCGCoordToSkCoord(p_center . x - (p_radii . width * 0.5f)), MCGCoordToSkCoord(p_center . y - (p_radii . height * 0.5f)),
									MCGFloatToSkScalar(p_radii . width), MCGFloatToSkScalar(p_radii . height));
		
		self -> path -> moveTo(MCGCoordToSkCoord(p_center . x), MCGCoordToSkCoord(p_center . y));		
		self -> path -> arcTo(t_bounds, MCGFloatToSkScalar(p_start_angle), MCGFloatToSkScalar(p_finish_angle - p_start_angle), false);		
		self -> path -> close();
		
		// TODO: Handle rotation
	}
	
	self -> is_valid = t_success;	
}

void MCGPathAddLine(MCGPathRef self, MCGPoint p_start, MCGPoint p_finish)
{
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
	{
		self -> path -> moveTo(MCGCoordToSkCoord(p_start . x), MCGCoordToSkCoord(p_start . y));
		self -> path -> lineTo(MCGCoordToSkCoord(p_finish . x), MCGCoordToSkCoord(p_finish . y));
	}	
	
	self -> is_valid = t_success;
}

void MCGPathAddPolygon(MCGPathRef self, const MCGPoint *p_points, uindex_t p_arity)
{
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> is_mutable;
	
	SkPoint *t_points = NULL;
	if (t_success)
		t_success = MCMemoryNewArray(p_arity, t_points);
	
	if (t_success)
		for (uint32_t i = 0; i < p_arity; i++)
			t_points[i] = MCGPointToSkPoint(p_points[i]);
	
	if (t_success)
		self -> path -> addPoly(t_points, p_arity, true);
	
	MCMemoryDeleteArray(t_points);
	
	self -> is_valid = t_success;
}

void MCGPathAddPolyline(MCGPathRef self, const MCGPoint *p_points, uindex_t p_arity)
{
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> is_mutable;
	
	SkPoint *t_points = NULL;
	if (t_success)
		t_success = MCMemoryNewArray(p_arity, t_points);
	
	if (t_success)
		for (uint32_t i = 0; i < p_arity; i++)
			t_points[i] = MCGPointToSkPoint(p_points[i]);
	
	if (t_success)
		self -> path -> addPoly(t_points, p_arity, false);
	
	MCMemoryDeleteArray(t_points);
	
	self -> is_valid = t_success;	
}

void MCGPathAddPath(MCGPathRef self, MCGPathRef p_path_to_add)
{
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;	
	
	if (t_success)
		t_success = self -> is_mutable && MCGPathIsValid(p_path_to_add);
	
	if (t_success)
		self -> path -> addPath(*p_path_to_add -> path);
	
	self -> is_valid = t_success;	
}

////////////////////////////////////////////////////////////////////////////////

void MCGPathMoveTo(MCGPathRef self, MCGPoint p_end_point)
{	
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;	
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
		self -> path -> moveTo(MCGPointToSkPoint(p_end_point));
	
	self -> is_valid = t_success;
}

void MCGPathLineTo(MCGPathRef self, MCGPoint p_end_point)
{
	bool t_success;
	t_success = true;	
	
	if (t_success)
		t_success = MCGPathIsValid(self) && self -> is_mutable;
	
	if (t_success)
		self -> path -> lineTo(MCGPointToSkPoint(p_end_point));	
	
	self -> is_valid = t_success;
}

void MCGPathQuadraticTo(MCGPathRef self, MCGPoint p_control_point, MCGPoint p_end_point)
{
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;	
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
		self -> path -> quadTo(MCGPointToSkPoint(p_control_point), MCGPointToSkPoint(p_end_point));	
	
	self -> is_valid = t_success;
}

void MCGPathCubicTo(MCGPathRef self, MCGPoint p_first_control_point, MCGPoint p_second_control_point, MCGPoint p_end_point)
{
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;	
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
		self -> path -> cubicTo(MCGPointToSkPoint(p_first_control_point), MCGPointToSkPoint(p_second_control_point), MCGPointToSkPoint(p_end_point));	
	
	self -> is_valid = t_success;	
}

void MCGPathArcTo(MCGPathRef self, MCGSize p_radii, MCGFloat p_rotation, bool p_large_arc, bool p_sweep, MCGPoint p_end_point)
{	
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
	{
		// TODO: Implement
	}
	
	self -> is_valid = t_success;	
}

void MCGPathCloseSubpath(MCGPathRef self)
{
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;	
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
		self -> path -> close();
	
	self -> is_valid = t_success;		
}

////////////////////////////////////////////////////////////////////////////////

void MCGPathThicken(MCGPathRef self, const MCGStrokeAttr& p_attr, MCGPathRef& r_thick_path)
{
	if (!MCGPathIsValid(self))
		return;
	
	// TODO: Implement
}

void MCGPathFlatten(MCGPathRef self, MCGFloat p_flatness, MCGPathRef& r_flat_path)
{
	if (!MCGPathIsValid(self))
		return;
	
	// TODO: Implement
}

void MCGPathSimplify(MCGPathRef self, MCGPathRef& r_simple_path)
{
	if (!MCGPathIsValid(self))
		return;
	
	// TODO: Implement
}

////////////////////////////////////////////////////////////////////////////////

bool MCGPathGetBoundingBox(MCGPathRef self, MCGRectangle &r_bounds)
{
	if (!MCGPathIsValid(self))
		return false;
	
	r_bounds = MCGRectangleFromSkRect(self->path->getBounds());
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCGPathTransform(MCGPathRef self, const MCGAffineTransform &p_transform)
{
	if (!MCGPathIsValid(self))
		return false;
	
	if (!MCGPathIsMutable(self))
		return false;
	
	SkMatrix t_matrix;
	MCGAffineTransformToSkMatrix(p_transform, t_matrix);
	self->path->transform(t_matrix);
}

////////////////////////////////////////////////////////////////////////////////

bool MCGPathIterate(MCGPathRef self, MCGPathIterateCallback p_callback, void *p_context)
{
	if (!MCGPathIsValid(self))
		return false;
	
	bool t_success;
	t_success = true;
	
	MCGPoint t_points[3];
	uint32_t t_point_count;
	MCGPathCommand t_command;
	
	SkPath::Iter t_iter(*self->path, false);
	SkPath::Verb t_verb;
	SkPoint t_sk_points[4];
	
	while (t_success && (t_verb = t_iter.next(t_sk_points)) != SkPath::kDone_Verb)
	{
		switch(t_verb)
		{
			case SkPath::kMove_Verb:
				t_command = kMCGPathCommandMoveTo;
				t_points[0] = MCGPointFromSkPoint(t_sk_points[0]);
				t_point_count = 1;
				break;
				
			case SkPath::kLine_Verb:
				if (t_iter.isCloseLine())
				{
					t_command = kMCGPathCommandCloseSubpath;
					t_point_count = 0;
				}
				else
				{
					t_command = kMCGPathCommandLineTo;
					t_points[0] = MCGPointFromSkPoint(t_sk_points[0]);
					t_point_count = 1;
				}
				break;
				
			case SkPath::kQuad_Verb:
				t_command = kMCGPathCommandQuadCurveTo;
				t_points[0] = MCGPointFromSkPoint(t_sk_points[0]);
				t_points[1] = MCGPointFromSkPoint(t_sk_points[1]);
				t_point_count = 2;
				break;
				
			case SkPath::kCubic_Verb:
				t_command = kMCGPathCommandCubicCurveTo;
				t_points[0] = MCGPointFromSkPoint(t_sk_points[0]);
				t_points[1] = MCGPointFromSkPoint(t_sk_points[1]);
				t_points[2] = MCGPointFromSkPoint(t_sk_points[2]);
				t_point_count = 3;
				break;
				
			default:
				// Unknown path instruction
				t_success = false;
				break;
		}
		
		t_success = p_callback(p_context, t_command, t_points, t_point_count);
	}
	
	if (t_success)
		t_success = p_callback(p_context, kMCGPathCommandEnd, nil, 0);
	
	return t_success;
}
