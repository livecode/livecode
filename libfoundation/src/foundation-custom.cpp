/*                                                                     -*-c++-*-
Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include <foundation.h>
#include "foundation-private.h"

/* ================================================================
 * Default implementations of custom value operations
 * ================================================================ */

void
__MCCustomDefaultDestroy (MCValueRef value)
{
}

bool
__MCCustomDefaultCopy (MCValueRef self,
                       bool p_release,
                       MCValueRef & r_value)
{
	/* This implementation doesn't handle mutable custom values. */
	if (MCValueIsMutable (self)) return false;

	r_value = MCValueRetain (self);
	if (p_release)
		MCValueRelease (self);
	return true;
}

bool
__MCCustomDefaultEqual (MCValueRef self,
                        MCValueRef p_other)
{
	return (self == p_other);
}

hash_t
__MCCustomDefaultHash (MCValueRef self)
{
	return MCHashPointer (self);
}

bool
__MCCustomDefaultDescribe (MCValueRef self,
                           MCStringRef & r_desc)
{
	return false;
}

bool
__MCCustomDefaultIsMutable (MCValueRef self)
{
	return false;
}

bool
__MCCustomDefaultMutableCopy (MCValueRef self,
                              bool release,
                              MCValueRef & r_value)
{
	return false;
}
