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

// IM-2014-09-24: [[ Bug 13208 ]] Shared code to convert from Linear RGB color space to CIE 1931 XYZ

#include "prefix.h"

#include "graphics.h"
#include "color.h"

////////////////////////////////////////////////////////////////////////////////

MCColorVector3 MCColorVector3ScalarMultiply(const MCColorVector3 &p_vector, MCGFloat p_scalar)
{
	return MCColorVector3Make(p_vector.x * p_scalar, p_vector.y * p_scalar, p_vector.z * p_scalar);
}

void MCColorMatrix3x3Set(MCColorMatrix3x3 &x_matrix,
						 MCGFloat a, MCGFloat b, MCGFloat c,
						 MCGFloat d, MCGFloat e, MCGFloat f,
						 MCGFloat g, MCGFloat h, MCGFloat i)
{
	x_matrix.m[0][0] = a;
	x_matrix.m[1][0] = b;
	x_matrix.m[2][0] = c;
	x_matrix.m[0][1] = d;
	x_matrix.m[1][1] = e;
	x_matrix.m[2][1] = f;
	x_matrix.m[0][2] = g;
	x_matrix.m[1][2] = h;
	x_matrix.m[2][2] = i;
}

void MCColorMatrix3x3Copy(const MCColorMatrix3x3 &p_matrix, MCColorMatrix3x3 &r_copy)
{
	MCMemoryCopy(&r_copy, &p_matrix, sizeof(MCColorMatrix3x3));
}

bool MCColorMatrix3x3Inverse(const MCColorMatrix3x3 &p_matrix, MCColorMatrix3x3 &r_inverse)
{
	MCGFloat a, b, c;
	a = p_matrix.m[1][1] * p_matrix.m[2][2] - p_matrix.m[1][2] * p_matrix.m[2][1];
	b = p_matrix.m[1][2] * p_matrix.m[2][0] - p_matrix.m[1][0] * p_matrix.m[2][2];
	c = p_matrix.m[1][0] * p_matrix.m[2][1] - p_matrix.m[1][1] * p_matrix.m[2][0];
	
	MCGFloat t_det;
	t_det = p_matrix.m[0][0] * a + p_matrix.m[0][1] * b + p_matrix.m[0][2] * c;
	
	if (t_det == 0.0)
		return false;
	
	MCGFloat t_inv_det;
	t_inv_det = 1.0 / t_det;
	
	r_inverse.m[0][0] = t_inv_det * a;
	r_inverse.m[0][1] = t_inv_det * (p_matrix.m[2][1] * p_matrix.m[0][2] - p_matrix.m[2][2] * p_matrix.m[0][1]);
	r_inverse.m[0][2] = t_inv_det * (p_matrix.m[0][1] * p_matrix.m[1][2] - p_matrix.m[0][2] * p_matrix.m[1][1]);
	r_inverse.m[1][0] = t_inv_det * b;
	r_inverse.m[1][1] = t_inv_det * (p_matrix.m[2][2] * p_matrix.m[0][0] - p_matrix.m[2][0] * p_matrix.m[0][2]);
	r_inverse.m[1][2] = t_inv_det * (p_matrix.m[0][2] * p_matrix.m[1][0] - p_matrix.m[0][0] * p_matrix.m[1][2]);
	r_inverse.m[2][0] = t_inv_det * c;
	r_inverse.m[2][1] = t_inv_det * (p_matrix.m[2][0] * p_matrix.m[0][1] - p_matrix.m[2][1] * p_matrix.m[0][0]);
	r_inverse.m[2][2] = t_inv_det * (p_matrix.m[0][0] * p_matrix.m[1][1] - p_matrix.m[0][1] * p_matrix.m[1][0]);
	
	return true;
}

MCColorVector3 MCColorMatrix3x3MultiplyVector(const MCColorMatrix3x3 &p_matrix, const MCColorVector3 &p_vector)
{
	return MCColorVector3Make(p_matrix.m[0][0] * p_vector.x + p_matrix.m[0][1] * p_vector.y + p_matrix.m[0][2] * p_vector.z,
						  p_matrix.m[1][0] * p_vector.x + p_matrix.m[1][1] * p_vector.y + p_matrix.m[1][2] * p_vector.z,
						  p_matrix.m[2][0] * p_vector.x + p_matrix.m[2][1] * p_vector.y + p_matrix.m[2][2] * p_vector.z);
}

MCColorVector3 xy_to_xyz(const MCColorVector2 &p_xy)
{
	return MCColorVector3Make(p_xy.x, p_xy.y, 1.0 - (p_xy.x + p_xy.y));
}

bool MCColorTransformLinearRGBToXYZ(const MCColorVector2 &p_white, const MCColorVector2 &p_red, const MCColorVector2 &p_green, const MCColorVector2 &p_blue,
									MCColorVector3 &r_white, MCColorMatrix3x3 &r_matrix)
{
	MCColorVector3 t_r, t_g, t_b, t_w;
	t_r = xy_to_xyz(p_red);
	t_g = xy_to_xyz(p_green);
	t_b = xy_to_xyz(p_blue);
	
	t_w = xy_to_xyz(p_white);
	t_w = MCColorVector3ScalarMultiply(t_w, 1.0 / t_w.y);
	
	MCColorMatrix3x3 t_matrix;
	MCColorMatrix3x3Set(t_matrix,
					t_r.x, t_r.y, t_r.z,
					t_g.x, t_g.y, t_g.z,
					t_b.x, t_b.y, t_b.z);
	
	MCColorMatrix3x3 t_inverse;
	if (!MCColorMatrix3x3Inverse(t_matrix, t_inverse))
		return false;
	
	MCColorVector3 t_scale;
	t_scale = MCColorMatrix3x3MultiplyVector(t_inverse, t_w);
	
	t_matrix.m[0][0] *= t_scale.x;
	t_matrix.m[1][0] *= t_scale.x;
	t_matrix.m[2][0] *= t_scale.x;
	t_matrix.m[0][1] *= t_scale.y;
	t_matrix.m[1][1] *= t_scale.y;
	t_matrix.m[2][1] *= t_scale.y;
	t_matrix.m[0][2] *= t_scale.z;
	t_matrix.m[1][2] *= t_scale.z;
	t_matrix.m[2][2] *= t_scale.z;
	
//	MCColorMatrix3x3Copy(t_matrix, r_matrix);
	r_matrix = t_matrix;
	r_white = t_w;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
