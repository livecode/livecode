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

#include <foundation.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

static bool __MCSetClone(MCSetRef self, bool as_mutable, MCSetRef& r_new_self);

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCSetCreateSingleton(uindex_t p_element, MCSetRef& r_set)
{
	return MCSetCreateWithIndices(&p_element, 1, r_set);
}

MC_DLLEXPORT_DEF
bool MCSetCreateWithIndices(uindex_t *p_elements, uindex_t p_element_count, MCSetRef& r_set)
{
	if (p_element_count == 0)
	{
		if (nil != kMCEmptySet)
		{
			r_set = MCValueRetain(kMCEmptySet);
			return true;
		}
	}
	else
	{
		MCAssert(nil != p_elements);
	}

	MCSetRef t_set;
	if (!MCSetCreateMutable(t_set))
		return false;

	for(uindex_t i = 0; i < p_element_count; i++)
		MCSetIncludeIndex(t_set, p_elements[i]);

	return MCSetCopyAndRelease(t_set, r_set);
}

MC_DLLEXPORT_DEF
bool MCSetCreateWithLimbsAndRelease(uindex_t *p_limbs, uindex_t p_limb_count, MCSetRef& r_set)
{
	MCAssert(nil != p_limbs);

	__MCSet *self;
	if (!__MCValueCreate(kMCValueTypeCodeSet, self))
		return false;

	self -> limbs = p_limbs;
	self -> limb_count = p_limb_count;

	r_set = self;

	return true;
}

MC_DLLEXPORT_DEF
bool MCSetCreateMutable(MCSetRef& r_set)
{
	__MCSet *self;
	if (!__MCValueCreate(kMCValueTypeCodeSet, self))
		return false;

	self -> flags |= kMCSetFlagIsMutable;

	r_set = self;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCSetCopy(MCSetRef self, MCSetRef& r_new_set)
{
	__MCAssertIsSet(self);

	if (!MCSetIsMutable(self))
	{
		r_new_set = MCValueRetain(self);
		return true;
	}
	return __MCSetClone(self, false, r_new_set);
}

MC_DLLEXPORT_DEF
bool MCSetCopyAndRelease(MCSetRef self, MCSetRef& r_new_set)
{
	__MCAssertIsSet(self);

	if (!MCSetIsMutable(self))
	{
		r_new_set = self;
		return true;
	}

	if (self -> references == 1)
	{
		self -> flags &= ~kMCSetFlagIsMutable;
		r_new_set = self;
		return true;
	}

	return __MCSetClone(self, false, r_new_set);
}

MC_DLLEXPORT_DEF
bool MCSetMutableCopy(MCSetRef self, MCSetRef& r_new_set)
{
	__MCAssertIsSet(self);
	return __MCSetClone(self, true, r_new_set);
}

MC_DLLEXPORT_DEF
bool MCSetMutableCopyAndRelease(MCSetRef self, MCSetRef& r_new_set)
{
	__MCAssertIsSet(self);

	if (self -> references == 1)
	{
		self -> flags |= kMCSetFlagIsMutable;
		r_new_set = self;
		return true;
	}

	return __MCSetClone(self, true, r_new_set);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCSetIsMutable(MCSetRef self)
{
	__MCAssertIsSet(self);

	return (self -> flags & kMCSetFlagIsMutable) != 0;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCSetIsEmpty(MCSetRef self)
{
	__MCAssertIsSet(self);

	for(uindex_t i = 0; i < self -> limb_count; i++)
		if (self -> limbs[i] != 0)
			return false;
	return true;
}

MC_DLLEXPORT_DEF
bool MCSetIsEqualTo(MCSetRef self, MCSetRef other_self)
{
	__MCAssertIsSet(self);
	__MCAssertIsSet(other_self);

	for(uindex_t i = 0; i < MCMax(self -> limb_count, other_self -> limb_count); i++)
	{
		uindex_t t_left_limb, t_right_limb;
		t_left_limb = i < self -> limb_count ? self -> limbs[i] : 0;
		t_right_limb = i < other_self -> limb_count ? other_self -> limbs[i] : 0;

		if ((t_left_limb ^ t_right_limb) != 0)
			return false;
	}
	return true;
}

MC_DLLEXPORT_DEF
bool MCSetContains(MCSetRef self, MCSetRef other_self)
{
	__MCAssertIsSet(self);
	__MCAssertIsSet(other_self);

	for(uindex_t i = 0; i < MCMax(self -> limb_count, other_self -> limb_count); i++)
	{
		uindex_t t_left_limb, t_right_limb;
		t_left_limb = i < self -> limb_count ? self -> limbs[i] : 0;
		t_right_limb = i < other_self -> limb_count ? other_self -> limbs[i] : 0;
		if ((t_left_limb | t_right_limb) != t_left_limb)
			return false;
	}
	return true;
}

MC_DLLEXPORT_DEF
bool MCSetIntersects(MCSetRef self, MCSetRef other_self)
{
	__MCAssertIsSet(self);
	__MCAssertIsSet(other_self);

	for(uindex_t i = 0; i < MCMax(self -> limb_count, other_self -> limb_count); i++)
	{
		uindex_t t_left_limb, t_right_limb;
		t_left_limb = i < self -> limb_count ? self -> limbs[i] : 0;
		t_right_limb = i < other_self -> limb_count ? other_self -> limbs[i] : 0;
		if ((t_left_limb & t_right_limb) != 0)
			return true;
	}
	return false;
}

MC_DLLEXPORT_DEF
bool MCSetContainsIndex(MCSetRef self, uindex_t p_element)
{
	__MCAssertIsSet(self);

	if (p_element >= self -> limb_count * 32)
		return false;
	return (self -> limbs[p_element / 32] & (1 << (p_element % 32))) != 0;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCSetIncludeIndex(MCSetRef self, uindex_t p_index)
{
	if (!MCSetIsMutable(self))
		return false;

	if (p_index / 32 >= self -> limb_count)
		if (!MCMemoryResizeArray(p_index / 32 + 1, self -> limbs, self -> limb_count))
			return false;

	self -> limbs[p_index / 32] |= 1 << (p_index % 32);

	return true;
}

MC_DLLEXPORT_DEF
bool MCSetExcludeIndex(MCSetRef self, uindex_t p_index)
{
	if (!MCSetIsMutable(self))
		return false;

	if (p_index / 32 >= self -> limb_count)
		return true;

	self -> limbs[p_index / 32] &= ~(1 << (p_index % 32));

	return true;
}

MC_DLLEXPORT_DEF
bool MCSetUnion(MCSetRef self, MCSetRef p_other_set)
{
	if (!MCSetIsMutable(self))
		return false;

	if (!MCMemoryResizeArray(MCMax(self -> limb_count, p_other_set -> limb_count), self -> limbs, self -> limb_count))
		return false;

	for(uindex_t i = 0; i < p_other_set -> limb_count; i++)
		self -> limbs[i] |= p_other_set -> limbs[i];

	return true;
}

MC_DLLEXPORT_DEF
bool MCSetDifference(MCSetRef self, MCSetRef p_other_set)
{
	if (!MCSetIsMutable(self))
		return false;

	for(uindex_t i = 0; i < self -> limb_count; i++)
	{
		if (i == p_other_set -> limb_count)
			break;
		self -> limbs[i] &= ~(p_other_set -> limbs[i]);
	}
	
	return true;
}

MC_DLLEXPORT_DEF
bool MCSetIntersect(MCSetRef self, MCSetRef p_other_set)
{
	if (!MCSetIsMutable(self))
		return false;

	if (!MCMemoryResizeArray(MCMin(self -> limb_count, p_other_set -> limb_count), self -> limbs, self -> limb_count))
		return false;

	for(uindex_t i = 0; i < self -> limb_count; i++)
		self -> limbs[i] &= p_other_set -> limbs[i];

	return true;
}

MC_DLLEXPORT_DEF
bool MCSetIterate(MCSetRef self, uindex_t& x_iterator, uindex_t& r_element)
{
	__MCAssertIsSet(self);

	while(x_iterator < self -> limb_count * 32)
	{
		x_iterator++;
		if (MCSetContainsIndex(self, x_iterator - 1))
		{
			r_element = x_iterator - 1;
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

static bool __MCSetClone(MCSetRef self, bool p_as_mutable, MCSetRef& r_new_self)
{
	__MCSet *t_new_set;
	if (!__MCValueCreate(kMCValueTypeCodeSet, t_new_set))
		return false;

	if (!MCMemoryNewArray(self -> limb_count, t_new_set -> limbs, t_new_set -> limb_count))
	{
		MCValueRelease(t_new_set);
		return false;
	}

	MCMemoryCopy(t_new_set -> limbs, self -> limbs, sizeof(uindex_t) * self -> limb_count);

	if (p_as_mutable)
		t_new_set -> flags |= kMCSetFlagIsMutable;

	r_new_self = t_new_set;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCSetRef kMCEmptySet;

bool __MCSetInitialize(void)
{
	if (!MCSetCreateWithIndices(nil, 0, kMCEmptySet))
		return false;

	return true;
}

void __MCSetFinalize(void)
{
	MCValueRelease(kMCEmptySet);
}

void __MCSetDestroy(__MCSet *self)
{
	MCMemoryDeleteArray(self -> limbs);
}

hash_t __MCSetHash(__MCSet *self)
{
	uindex_t t_used;
	t_used = self -> limb_count;
	while(t_used > 0 && self -> limbs[t_used - 1] == 0)
		t_used -= 1;
	return MCHashBytes(self -> limbs, t_used * sizeof(uindex_t));
}

bool __MCSetIsEqualTo(__MCSet *self, __MCSet *other_self)
{
	return MCSetIsEqualTo(self, other_self);
}

bool __MCSetCopyDescription(__MCSet *self, MCStringRef& r_string)
{
	bool t_success;
	t_success = true;

	MCStringRef t_string;
	t_string = nil;
	if (t_success)
		t_success = MCStringCreateMutable(0, t_string);
	if (t_success)
		t_success = MCStringAppendFormat(t_string, "{");
	uindex_t t_iterator, t_element;
	bool t_first;
	t_first = true;
	t_iterator = 0;
	while(t_success && MCSetIterate(self, t_iterator, t_element))
	{
		t_success = MCStringAppendFormat(t_string, "%s%d", t_first ? "" : ",", t_element);
		t_first = false;
	}
	if (t_success)
		t_success = MCStringAppendFormat(t_string, "}");
	if (t_success)
		t_success = MCStringCopyAndRelease(t_string, r_string);
	
    if (!t_success)
        MCValueRelease(t_string);

	return t_success;
}

bool __MCSetImmutableCopy(__MCSet *self, bool p_release, __MCSet*& r_immutable_value)
{
	if (!p_release)
		return MCSetCopy(self, r_immutable_value);
	return MCSetCopyAndRelease(self, r_immutable_value);
}

////////////////////////////////////////////////////////////////////////////////
