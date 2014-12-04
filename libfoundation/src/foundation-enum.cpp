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

////////////////////////////////////////////////////////////////////////////////

bool
MCEnumCreate (MCTypeInfoRef p_typeinfo,
              MCValueRef p_value,
              MCEnumRef & r_enum)
{
	MCAssert (p_typeinfo != nil);
	MCAssert (p_value != nil);

	bool t_success = true;

	/* Before doing anything else, check that p_value is actually a
	 * valid value for the requested enumerated type */
	if (!MCEnumTypeInfoHasValue (p_typeinfo, p_value))
		return false;

	__MCEnum *self = nil;
	if (t_success)
		t_success = __MCValueCreate(kMCValueTypeCodeEnum, self);

	if (t_success)
		t_success = MCValueInter(p_value, self->value);

	if (t_success)
	{
		self->typeinfo = MCValueRetain(p_typeinfo);
		r_enum = self;
	}

	if (!t_success)
		MCValueRelease (self);

	return t_success;
}

bool
MCEnumCopy (MCEnumRef self,
            MCEnumRef & r_new_enum)
{
	return MCEnumCreate (MCValueGetTypeInfo (self),
	                     MCEnumGetValue (self),
	                     r_new_enum);
}

bool
MCEnumCopyAndRelease (MCEnumRef self,
                      MCEnumRef & r_new_enum)
{
	if (!MCEnumCopy (self, r_new_enum))
		return false;

	MCValueRelease (self);
	return true;
}

MCValueRef
MCEnumGetValue (MCEnumRef self)
{
	MCAssert (self != nil);
	return self->value;
}

////////////////////////////////////////////////////////////////////////////////

void
__MCEnumDestroy (__MCEnum *self)
{
	MCValueRelease (self->value);
	MCValueRelease (self->typeinfo);
}

hash_t
__MCEnumHash (__MCEnum *self)
{
	hash_t t_hash = 0;

	hash_t t_typeinfo_hash = MCHashPointer(self->typeinfo);
	t_hash = MCHashBytesStream(t_hash, &t_typeinfo_hash, sizeof(hash_t));

	hash_t t_value_hash = MCValueHash(self->value);
	t_hash = MCHashBytesStream (t_hash, &t_value_hash, sizeof(hash_t));

	return t_hash;
}

bool
__MCEnumIsEqualTo (__MCEnum *self,
                   __MCEnum *other_self)
{
	/* Enums must have the same typeinfo to be equal */
	if (self->typeinfo != other_self->typeinfo)
		return false;

	/* The values should also have been interned, so compare by pointer */
	return self->value == other_self->value;
}

bool
__MCEnumCopyDescription (__MCEnum *self,
                         MCStringRef & r_description)
{
	return false;
}

bool
__MCEnumImmutableCopy (__MCEnum *self,
                       bool p_release,
                       __MCEnum *& r_immutable_value)
{
	if (p_release)
		return MCEnumCopyAndRelease(self, r_immutable_value);
	else
		return MCEnumCopy(self, r_immutable_value);
}

bool
__MCEnumInitialize (void)
{
	return true;
}

void
__MCEnumFinalize (void)
{
}
