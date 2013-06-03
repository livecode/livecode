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

enum MCOrientation
{
	ORIENTATION_UNKNOWN,
	ORIENTATION_PORTRAIT,
	ORIENTATION_PORTRAIT_UPSIDE_DOWN,
	ORIENTATION_LANDSCAPE_RIGHT,
	ORIENTATION_LANDSCAPE_LEFT,
	ORIENTATION_FACE_UP,
	ORIENTATION_FACE_DOWN
};

enum MCOrientationSet
{
	ORIENTATION_UNKNOWN_BIT = 1 << ORIENTATION_UNKNOWN ,
	ORIENTATION_PORTRAIT_BIT = 1 << ORIENTATION_PORTRAIT,
	ORIENTATION_PORTRAIT_UPSIDE_DOWN_BIT = 1 << ORIENTATION_PORTRAIT_UPSIDE_DOWN,
	ORIENTATION_LANDSCAPE_RIGHT_BIT = 1 << ORIENTATION_LANDSCAPE_RIGHT,
	ORIENTATION_LANDSCAPE_LEFT_BIT = 1 << ORIENTATION_LANDSCAPE_LEFT,
	ORIENTATION_FACE_UP_BIT = 1 << ORIENTATION_FACE_UP,
	ORIENTATION_FACE_DOWN_BIT = 1 << ORIENTATION_FACE_DOWN
};