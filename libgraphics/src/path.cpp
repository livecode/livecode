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

bool MCPathIsMutable(MCGPathRef self)
{
	return self -> is_mutable;
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
	
	SkPoint *t_points;
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
	
	SkPoint *t_points;
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