/*                                                                     -*-c++-*-
Copyright (C) 2003-2015 Runtime Revolution Ltd.

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
#include <foundation-auto.h>
#include <foundation-file.h>

#include "foundation-private.h"

#if defined(__WINDOWS__)
#  include <windows.h>
#  include <Wincrypt.h>
#endif

/* ================================================================
 * Random data generation
 * ================================================================
 *
 * The __MCRandomBytes() function obtains cryptographic-quality
 * uniformly-distributed random bytes from each platform's recommended
 * system random data source.
 */

#if defined(__WINDOWS__)
/* ----------------------------------------------------------------
 * Windows random numbers
 * ---------------------------------------------------------------- */

bool
__MCRandomBytes (void *x_buffer,
                 size_t p_buffer_length)
{
	/* Acquire cryptographic provider */
	HCRYPTPROV t_provider;
	t_provider = NULL;
	if (!CryptAcquireContextW (&t_provider,
	                           0,
	                           0,
	                           PROV_RSA_FULL,
	                           CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
	{
		MCErrorCreateAndThrow (kMCGenericErrorTypeInfo, "reason", MCSTR("Failed to generate random data: failed to acquire cryptographic context"), NULL);
		goto error_cleanup;
	}

	/* Obtain random data */
	if (!CryptGenRandom (t_provider, p_buffer_length, (BYTE*)x_buffer))
	{
		MCErrorCreateAndThrow (kMCGenericErrorTypeInfo, "reason", MCSTR("Failed to generate random data"), NULL);
		goto error_cleanup;
	}

	/* Release provider */
	CryptReleaseContext (t_provider, 0);
	return true;

 error_cleanup:
	if (t_provider)
		CryptReleaseContext (t_provider, 0);
	return false;
}

#else /* !__WINDOWS__ */
/* ----------------------------------------------------------------
 * Non-Windows random numbers
 * ---------------------------------------------------------------- */

bool
__MCRandomBytes (void *x_buffer,
                 size_t p_buffer_length)
{

#	if defined(__MAC__) || defined(__IOS__)
	/* ---------- arc4random(3) library function
	 *
	 * On OSX >= 10.7 and on most versions of iOS, there's an
	 * arc4random(3) function in the standard library.  It's a source
	 * of cryptographic-quality random data.  It self-seeds from the
	 * kernel entropy source when necessary.
	 *
	 * Since we don't know whether arc4random_buf(3) is available at
	 * compile time, check for it using dlsym(3).
	 */
	{
		void (*t_arc4random_buf_func)(void *, size_t);
		void *t_self;

		t_self = dlopen(NULL, 0);
		t_arc4random_buf_func = dlsym (t_self, "arc4random_buf");

		if (NULL != t_arc4random_buf_func)
		{
			t_arc4random_buf_func (x_buffer, p_buffer_length);
			return true;
		}
	}
#	endif /* __MAC__ || __IOS__ */

	/* ---------- POSIX random number device
	 *
	 * Both Linux and OSX/iOS have /dev/random and /dev/urandom.  On
	 * OSX/iOS, they both behave identically, but on Linux, only
	 * /dev/urandom is guaranteed not to block waiting for entropy.
	 */
	{
		static const char k_random_device[] = "/dev/urandom";
		bool t_success = true;
		MCStreamRef t_stream = NULL;

		if (t_success)
			t_success = MCFileCreateStream (MCSTR(k_random_device),
			                                kMCOpenFileModeRead,
			                                t_stream);

		if (t_success)
			t_success = MCStreamRead (t_stream, x_buffer,
			                          p_buffer_length);

		MCValueRelease (t_stream);

		if (t_success)
			return true;
	}
	return false;
}

#endif /* !__WINDOWS__ */
