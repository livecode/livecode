/* Copyright (C) 2009-2015 LiveCode Ltd.

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

#ifndef __MC_THUNK__
#define __MC_THUNK__

#ifndef __MC_CORE__
#include "core.h"
#endif

////////////////////////////////////////////////////////////////////////////////

bool MCThunkInitialize(void);
void MCThunkFinalize(void);

// Create a thunk from stdcall -> fastcall
bool MCThunkNew(void *object, void *method, void*& r_closure);

// Create a thunk from stdcall -> stdcall
bool MCThunkNewStdCall(void *object, void *function, void*& r_closure);

void MCThunkDelete(void *closure);


/////////

#define __MCThunkNewTemplate(m_result, m_args) \
	inline bool MCThunkNew(T *p_object, m_result (__thiscall T::*p_method) m_args, m_result (__stdcall *& r_closure) m_args) \
	{ \
		typedef m_result (__stdcall *Closure) m_args; \
		void *t_closure; \
		if (MCThunkNew(p_object, *reinterpret_cast<void **>(&p_method), t_closure)) \
		{ \
			r_closure = static_cast<Closure>(t_closure); \
			return true; \
		} \
		return false; \
	}

//////////

// These don't seem needed...
#if 0
template<typename T>
__MCThunkNewTemplate(void, (void))

template<typename T, typename P1>
__MCThunkNewTemplate(void, (P1))

template<typename T, typename P1, typename P2>
__MCThunkNewTemplate(void, (P1, P2))

template<typename T, typename P1, typename P2, typename P3>
__MCThunkNewTemplate(void, (P1, P2, P3))

template<typename T, typename P1, typename P2, typename P3, typename P4>
__MCThunkNewTemplate(void, (P1, P2, P3, P4))
#endif

//////////

template<typename T, typename R>
__MCThunkNewTemplate(R, (void))

template<typename T, typename R, typename P1>
__MCThunkNewTemplate(R, (P1))

template<typename T, typename R, typename P1, typename P2>
__MCThunkNewTemplate(R, (P1, P2))

template<typename T, typename R, typename P1, typename P2, typename P3>
__MCThunkNewTemplate(R, (P1, P2, P3))

template<typename T, typename R, typename P1, typename P2, typename P3, typename P4>
__MCThunkNewTemplate(R, (P1, P2, P3, P4))

////////////////////////////////////////////////////////////////////////////////

#endif
