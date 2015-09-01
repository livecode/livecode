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

#include "w32prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "osspec.h"

#include <Wincrypt.h>

////////////////////////////////////////////////////////////////////////////////

bool MCS_random_bytes(size_t p_count, MCDataRef& r_bytes)
{
	// Acquire crypt provider.
	HCRYPTPROV t_provider;
	t_provider = NULL;
	if (CryptAcquireContextW(&t_provider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT) == 0)
		return false;

	// Attempt to generate random bytes.
	bool t_success;

	MCAutoByteArray t_bytes;
	t_success = t_bytes . New(p_count);

	if (t_success && CryptGenRandom(t_provider, p_count, (BYTE *)t_bytes . Bytes()) == 0)
		t_success = false;

	// Release the provider.
	CryptReleaseContext(t_provider, 0);

	// Return whether we successfully generated random bytes or not.
	if (t_success)
		return MCDataCreateWithBytes(t_bytes . Bytes(), t_bytes . ByteCount(), r_bytes);
	else
		return false;
}

////////////////////////////////////////////////////////////////////////////////