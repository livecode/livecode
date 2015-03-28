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
	if (!MCGPathIsValid(self))
		r_new_path = nil;
	else if(!self -> is_mutable)
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
		else
		{
			MCGPathDestroy(t_new_path);
			r_new_path = nil;
		}
	}
}

void MCGPathCopyAndRelease(MCGPathRef self, MCGPathRef& r_new_path)
{
	if (!MCGPathIsValid(self))
		r_new_path = nil;
	else if (!self -> is_mutable)
		r_new_path = self;
	else if (self -> references == 1)
	{
		self -> is_mutable = false;
		r_new_path = self;
	}
	else
	{
		MCGPathRef t_new_path;
		MCGPathCopy(self, r_new_path);
		if (MCGPathIsValid(r_new_path))
			MCGPathRelease(self);
	}
}

void MCGPathMutableCopy(MCGPathRef self, MCGPathRef& r_new_path)
{
	MCGPathRef t_new_path;
	t_new_path = nil;
	
	if (MCGPathIsValid(self))
	{
		MCGPathCreateMutable(t_new_path);
		MCGPathAddPath(t_new_path, self);
	}
	
	if (MCGPathIsValid(t_new_path))
		r_new_path = t_new_path;
	else
	{
		MCGPathDestroy(t_new_path);
		r_new_path = nil;
	}
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

static inline MCGSize MCGSizeScale(const MCGSize &p_size, MCGFloat p_scale)
{
	return MCGSizeMake(p_size.width * p_scale, p_size.height * p_scale);
}

static inline MCGPoint MCGPointRotate(const MCGPoint &p_point, MCGFloat p_angle)
{
	MCGFloat t_sin, t_cos;
	t_sin = sinf(p_angle);
	t_cos = cosf(p_angle);
	
	return MCGPointMake(p_point.x * t_cos - p_point.y * t_sin, p_point.y * t_cos + p_point.x * t_sin);
}

static inline MCGFloat MCGDegreesToRadians(MCGFloat p_degrees)
{
	return p_degrees * M_PI / 180;
}

enum MCGPathArcBeginStyle
{
	kMCGPathArcContinue,
	kMCGPathArcBeginWithMove,
	kMCGPathArcBeginWithLine,
};

// Approximate arc with angle <= 90 using Bezier curve
static void _MCGPathAcuteArcTo(MCGPathRef self, MCGPathArcBeginStyle p_begin, const MCGPoint &p_center, const MCGSize p_radii, MCGFloat p_start_angle, MCGFloat p_sweep, MCGFloat p_x_angle)
{
#define RIGHT_ANGLE_SCALE 0.551784
	MCGSize t_k;
	
	if (p_sweep == M_PI_2)
		t_k = MCGSizeScale(p_radii, RIGHT_ANGLE_SCALE);
	else if (p_sweep == -M_PI_2)
		t_k = MCGSizeScale(p_radii, -RIGHT_ANGLE_SCALE);
	else
	{
		MCGFloat t_h;
		t_h = tanf(p_sweep / 4);
		t_k = MCGSizeScale(p_radii, 4 * t_h / 3);
	}
	
	MCGFloat t_cos_a, t_sin_a, t_cos_b, t_sin_b;
	
	t_cos_a = cosf(p_start_angle);
	t_sin_a = sinf(p_start_angle);
	t_cos_b = cosf(p_start_angle + p_sweep);
	t_sin_b = sinf(p_start_angle + p_sweep);
	
	if (p_begin != kMCGPathArcContinue)
	{
		// move to start point of arc
		MCGPoint t_p0;
		t_p0 = MCGPointMake(p_radii.width * t_cos_a, p_radii.height * t_sin_a);
		t_p0 = MCGPointTranslate(MCGPointRotate(t_p0, p_x_angle), p_center.x, p_center.y);
		
		if (p_begin == kMCGPathArcBeginWithMove)
			MCGPathMoveTo(self, t_p0);
		else if (p_begin == kMCGPathArcBeginWithLine)
			MCGPathLineTo(self, t_p0);
	}
	
	MCGPoint t_p1, t_p2, t_p3;
	t_p1 = MCGPointMake(p_radii.width * t_cos_a - t_k.width * t_sin_a, p_radii.height * t_sin_a + t_k.height * t_cos_a);
	t_p2 = MCGPointMake(p_radii.width * t_cos_b + t_k.width * t_sin_b, p_radii.height * t_sin_b - t_k.height * t_cos_b);
	t_p3 = MCGPointMake(p_radii.width * t_cos_b, p_radii.height * t_sin_b);
	
	t_p1 = MCGPointTranslate(MCGPointRotate(t_p1, p_x_angle), p_center.x, p_center.y);
	t_p2 = MCGPointTranslate(MCGPointRotate(t_p2, p_x_angle), p_center.x, p_center.y);
	t_p3 = MCGPointTranslate(MCGPointRotate(t_p3, p_x_angle), p_center.x, p_center.y);
	
	MCGPathCubicTo(self, t_p1, t_p2, t_p3);
}

static void _MCGPathArcTo(MCGPathRef self, MCGPathArcBeginStyle p_begin, const MCGPoint &p_center, MCGSize &p_radii, MCGFloat p_start_angle, MCGFloat p_sweep, MCGFloat p_x_angle)
{
	p_start_angle = fmodf(p_start_angle, 2 * M_PI);
	
	MCGFloat t_delta;
	if (p_sweep > 0)
		while (p_sweep > 0)
		{
			t_delta = MCMin( M_PI_2 - fmodf(p_start_angle, M_PI_2), p_sweep);
			p_sweep -= t_delta;
			
			_MCGPathAcuteArcTo(self, p_begin, p_center, p_radii, p_start_angle, t_delta, p_x_angle);
			p_begin = kMCGPathArcContinue;
			p_start_angle += t_delta;
		}
	else
		while (p_sweep < 0)
		{
			t_delta = MCMin( M_PI_2 - fmodf(2 * M_PI - p_start_angle, M_PI_2), -p_sweep);
			p_sweep += t_delta;
			
			_MCGPathAcuteArcTo(self, p_begin, p_center, p_radii, p_start_angle, -t_delta, p_x_angle);
			p_begin = kMCGPathArcContinue;
			p_start_angle -= t_delta;
		}
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
		if (p_rotation != 0)
		{
			// Use Bezier curve approximation
			_MCGPathArcTo(self, kMCGPathArcBeginWithMove, p_center, p_radii, 0, 2 * M_PI, MCGDegreesToRadians(p_rotation));
			
		}
		else
		{
			// Use Skia implementation
			SkRect t_bounds;
			t_bounds = SkRect::MakeXYWH(MCGCoordToSkCoord(p_center . x - (p_radii . width * 0.5f)), MCGCoordToSkCoord(p_center . y - (p_radii . height * 0.5f)),
										MCGFloatToSkScalar(p_radii . width), MCGFloatToSkScalar(p_radii . height));
			self -> path -> addOval(t_bounds);
		}
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
		if (p_rotation != 0)
		{
			// Use Bezier curve approximation
			_MCGPathArcTo(self, kMCGPathArcBeginWithMove, p_center, p_radii, MCGDegreesToRadians(p_start_angle), MCGDegreesToRadians(p_finish_angle - p_start_angle), MCGDegreesToRadians(p_rotation));
		}
		else
		{
			// Use Skia implementation
			SkRect t_bounds;
			t_bounds = SkRect::MakeXYWH(MCGCoordToSkCoord(p_center . x - (p_radii . width * 0.5f)), MCGCoordToSkCoord(p_center . y - (p_radii . height * 0.5f)),
										MCGFloatToSkScalar(p_radii . width), MCGFloatToSkScalar(p_radii . height));
			self -> path -> addArc(t_bounds, MCGFloatToSkScalar(p_start_angle), MCGFloatToSkScalar(p_finish_angle - p_start_angle));
		}
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
		if (p_rotation != 0)
		{
			// Use Bezier curve approximation
			_MCGPathArcTo(self, kMCGPathArcBeginWithMove, p_center, p_radii, MCGDegreesToRadians(p_start_angle), MCGDegreesToRadians(p_finish_angle - p_start_angle), MCGDegreesToRadians(p_rotation));
			self -> path -> close();
		}
		else
		{
			// Use Skia implementation
			SkRect t_bounds;
			t_bounds = SkRect::MakeXYWH(MCGCoordToSkCoord(p_center . x - (p_radii . width * 0.5f)), MCGCoordToSkCoord(p_center . y - (p_radii . height * 0.5f)),
										MCGFloatToSkScalar(p_radii . width), MCGFloatToSkScalar(p_radii . height));
			
			self -> path -> arcTo(t_bounds, MCGFloatToSkScalar(p_start_angle), MCGFloatToSkScalar(p_finish_angle - p_start_angle), false);		
			self -> path -> close();
		}
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
		if (p_rotation != 0)
		{
			// Use Bezier curve approximation
			self -> path -> moveTo(MCGCoordToSkCoord(p_center . x), MCGCoordToSkCoord(p_center . y));
			_MCGPathArcTo(self, kMCGPathArcBeginWithLine, p_center, p_radii, MCGDegreesToRadians(p_start_angle), MCGDegreesToRadians(p_finish_angle - p_start_angle), MCGDegreesToRadians(p_rotation));
			self -> path -> close();
		}
		else
		{
			// Use Skia implementation
			SkRect t_bounds;
			t_bounds = SkRect::MakeXYWH(MCGCoordToSkCoord(p_center . x - (p_radii . width * 0.5f)), MCGCoordToSkCoord(p_center . y - (p_radii . height * 0.5f)),
										MCGFloatToSkScalar(p_radii . width), MCGFloatToSkScalar(p_radii . height));
			
			self -> path -> moveTo(MCGCoordToSkCoord(p_center . x), MCGCoordToSkCoord(p_center . y));		
			self -> path -> arcTo(t_bounds, MCGFloatToSkScalar(p_start_angle), MCGFloatToSkScalar(p_finish_angle - p_start_angle), false);		
			self -> path -> close();
		}
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

////////////////////////////////////////////////////////////////////////////////

static inline MCGFloat _sqr(MCGFloat p_val)
{
	return p_val * p_val;
}

static inline MCGFloat MCGVectorMagnitude(const MCGPoint &v)
{
	return sqrtf(_sqr(v.x) + _sqr(v.y));
}

static inline MCGFloat MCGVectorDotProduct(const MCGPoint &u, const MCGPoint &v)
{
	return u.x * v.x + u.y * v.y;
}

static inline MCGFloat MCGAngleBetweenVectors(const MCGPoint &u, const MCGPoint &v)
{
	compare_t t_s;
	t_s = MCSgn(u.x * v.y - u.y * v.x);
	if (t_s == 0)
		t_s = 1;
	
	return t_s * acosf(MCClamp(MCGVectorDotProduct(u, v) / (MCGVectorMagnitude(u) * MCGVectorMagnitude(v)), -1, 1));
}

//////////

static inline MCGPoint MCGPointGetMidPoint(const MCGPoint &p_a, const MCGPoint &p_b)
{
	return MCGPointMake((p_a.x + p_b.x) / 2, (p_a.y + p_b.y) / 2);
}

static void _MCGPathEllipticArc(MCGPathRef self, const MCGSize &p_radii, MCGFloat p_angle, bool p_large_arc, bool p_sweep, const MCGPoint &p_end_point)
{
	if (p_radii.width == 0 || p_radii.height == 0)
	{
		MCGPathLineTo(self, p_end_point);
		return;
	}
	
	MCGPoint t_last;
	if (!MCGPathGetLastPoint(self, t_last))
		t_last = MCGPointMake(0, 0);
	
	if (MCGPointIsEqual(t_last, p_end_point))
		return;
	
	p_angle = fmodf(p_angle + 2 * M_PI, 2 * M_PI);
	
	MCGPoint t_p;
	t_p = MCGPointRotate(MCGPointMake((t_last.x - p_end_point.x) / 2, (t_last.y - p_end_point.y) / 2), -p_angle);
	
	MCGSize t_radii;
	t_radii = MCGSizeMake(MCAbs(p_radii.width), MCAbs(p_radii.height));
	
	MCGFloat t_a;
	t_a = (_sqr(t_p.x) / _sqr(t_radii.width)) + (_sqr(t_p.y) / _sqr(t_radii.height));
	
	MCGPoint t_c;
	
	// Adjust if radii are too small
	if (t_a > 1)
	{
		t_radii = MCGSizeScale(t_radii, sqrtf(t_a));
		
		t_c = MCGPointMake(0, 0);
	}
	else
	{
		MCGFloat t_scale;
		MCGFloat t_rx2, t_ry2, t_x2, t_y2;
		t_rx2 = _sqr(t_radii.width);
		t_ry2 = _sqr(t_radii.height);
		t_x2 = _sqr(t_p.x);
		t_y2 = _sqr(t_p.y);
		
		t_scale = sqrtf((t_rx2 * t_ry2 - t_rx2 * t_y2 - t_ry2 * t_x2) / (t_rx2 * t_y2 + t_ry2 * t_x2));
		if (p_large_arc == p_sweep)
			t_scale *= -1;
		
		t_c = MCGPointMake(t_scale * t_radii.width * t_p.y / t_radii.height, t_scale * -t_radii.height * t_p.x / t_radii.width);
	}
	
	MCGPoint t_mid;
	t_mid = MCGPointGetMidPoint(t_last, p_end_point);
	
	MCGPoint t_center;
	t_center = MCGPointTranslate(MCGPointRotate(t_c, p_angle), t_mid.x, t_mid.y);
	
	MCGPoint t_v0, t_v1, t_v2;
	t_v0 = MCGPointMake(1, 0);
	t_v1 = MCGPointMake((t_p.x - t_c.x) / t_radii.width, (t_p.y - t_c.y) / t_radii.height);
	t_v2 = MCGPointMake((-t_p.x - t_c.x) / t_radii.width, (-t_p.y - t_c.y) / t_radii.height);
	
	MCGFloat t_start_angle, t_sweep;
	t_start_angle = MCGAngleBetweenVectors(t_v0, t_v1);
	t_sweep = MCGAngleBetweenVectors(t_v1, t_v2);
	t_sweep = fmodf(t_sweep, 2 * M_PI);
	
	if (!p_sweep && t_sweep > 0)
		t_sweep -= 2 * M_PI;
	else if (p_sweep && t_sweep < 0)
		t_sweep += 2 * M_PI;
	
	_MCGPathArcTo(self, kMCGPathArcContinue, t_center, t_radii, t_start_angle, t_sweep, p_angle);
}

////////////////////////////////////////////////////////////////////////////////

void MCGPathArcTo(MCGPathRef self, MCGSize p_radii, MCGFloat p_rotation, bool p_large_arc, bool p_sweep, MCGPoint p_end_point)
{	
	if (!MCGPathIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> is_mutable;
	
	if (t_success)
		_MCGPathEllipticArc(self, p_radii, p_rotation * M_PI / 180, p_large_arc, p_sweep, p_end_point);
	
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

bool MCGPathGetLastPoint(MCGPathRef self, MCGPoint &r_last)
{
	SkPoint t_point;
	if (!self->path->getLastPt(&t_point))
		return false;
	
	r_last = MCGPointFromSkPoint(t_point);
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
	
	return true;
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
	
	// IM-2015-03-20: [[ Bug 15035 ]] The first point returned by SkPath::Iter::next() is
	//  always the last moveTo point; the points for the current verb start at index 1.
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
				// Don't call callback for implicit lineto from close command
				if (t_iter.isCloseLine())
					continue;
				
				t_command = kMCGPathCommandLineTo;
				t_points[0] = MCGPointFromSkPoint(t_sk_points[1]);
				t_point_count = 1;
				break;
				
			case SkPath::kQuad_Verb:
				t_command = kMCGPathCommandQuadCurveTo;
				t_points[0] = MCGPointFromSkPoint(t_sk_points[1]);
				t_points[1] = MCGPointFromSkPoint(t_sk_points[2]);
				t_point_count = 2;
				break;
				
			case SkPath::kCubic_Verb:
				t_command = kMCGPathCommandCubicCurveTo;
				t_points[0] = MCGPointFromSkPoint(t_sk_points[1]);
				t_points[1] = MCGPointFromSkPoint(t_sk_points[2]);
				t_points[2] = MCGPointFromSkPoint(t_sk_points[3]);
				t_point_count = 3;
				break;
				
			case SkPath::kClose_Verb:
				t_command = kMCGPathCommandCloseSubpath;
				t_point_count = 0;
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
