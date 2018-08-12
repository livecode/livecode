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


/*
  LiveCode does not use exceptions and the codebase assumes that `new`
  returns NULL if the allocation fails.  This is not the default
  behaviour of modern C++ compilers so we need to check that the
  compiler has the correct flags to get the desired behaviour.

  We need to check that:

   - If an allocation fails then new returns a null pointer and
     doesn't (try) to throw an exception.

   - If an allocation fails then the constructor is not called.

*/

#include "gtest/gtest.h"

#include "prefix.h"


enum {
	kSize =
// MSVC borks if a declared array is > 0x7fffffff bytes long, so make sure
// VeryBig doesn't exceed this.
#if defined(__32_BIT__) || defined(__WINDOWS__)
	1 << 15
#else
	1 << 20
#endif
 };

class Big {
public:
	Big() {
		// Check that the constructor is not called after the
		// allocation failed.  If the compiler flags are not set
		// correctly it may be assuming that `new` never returns
		// NULL and so calls the constructor without checking it's
		// not NULL.  Writing to `m_big[0]` will cause a seg fault
		// if `this` is NULL.
		m_big[0] = 0;
	}

	char m_big[kSize];
};

struct VeryBig {
	Big m_bigger[kSize];
};


#if defined(__EMSCRIPTEN__) || defined(__MAC__)
TEST(new, DISABLED_new)
#else
TEST(new, new)
#endif
//
// Checks that new returns NULL on error.
//
{
	VeryBig* pointers[1000];
	int i = 0;

	VeryBig* p = new (nothrow) VeryBig;

	for (; p != NULL && i < 1000; i++) {
		pointers[i] = p;
		p = new (nothrow) VeryBig;
	}

	ASSERT_NE(1000, i) << "All alocations succeed!";

	for (i--; i >= 0; i--) {
		delete pointers[i];
	}
}


#if defined(__EMSCRIPTEN__) || defined(__MAC__)
TEST(new, DISABLED_array)
#else
TEST(new, array)
#endif
//
// Checks that new[] returns NULL on error.
//
{
	VeryBig* pointers[1000];
	int i = 0;

	VeryBig* p = new (nothrow) VeryBig[1];

	for (; p != NULL && i < 1000; i++) {
		pointers[i] = p;
		p = new (nothrow) VeryBig[1];
	}

	ASSERT_NE(1000, i) << "All alocations succeed!";

	for (i--; i >= 0; i--) {
		delete [] pointers[i];
	}
}
