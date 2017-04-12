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

#include <windows.h>

#include "revspeech.h"

#include "w32sapi5speech.h"

INarrator *InstantiateNarrator(NarratorProvider p_provider)
{
#ifdef OLD_SAPI
	if (p_provider == kNarratorProviderDefault)
	{
		INarrator *t_narrator;
		t_narrator = new (nothrow) WindowsSAPI5Narrator();
		if (!t_narrator -> Initialize())
		{
			delete t_narrator;

			t_narrator = new (nothrow) WindowsSAPI4Narrator();
			if (!t_narrator -> Initialize())
			{
				delete t_narrator;
				t_narrator = NULL;
			}
		}

		return t_narrator;
	}
	else if (p_provider == kNarratorProviderSAPI4)
		return new WindowsSAPI4Narrator();
#endif

	return new WindowsSAPI5Narrator();
}

extern "C" BOOL WINAPI DllMain(HINSTANCE tInstance, DWORD dwReason, LPVOID)
{
	return TRUE;
}
