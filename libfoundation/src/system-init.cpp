/*                                                                     -*-c++-*-
Copyright (C) 2015 LiveCode Ltd.

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

#include "system-private.h"

MC_DLLEXPORT_DEF bool
MCSInitialize(void)
{
	return
		__MCSCommandLineInitialize() &&
		__MCSFileInitialize() &&
		__MCSStreamInitialize() &&
        __MCSLibraryInitialize();
}

MC_DLLEXPORT_DEF void
MCSFinalize(void)
{
    __MCSLibraryFinalize();
	__MCSStreamFinalize();
	__MCSFileFinalize();
	__MCSCommandLineFinalize();
}
