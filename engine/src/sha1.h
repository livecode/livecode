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

#ifndef __MC_SHA1__
#define __MC_SHA1__

struct sha1_state_t
{
	uint32_t state[5];
	uint32_t count[2];
	uint8_t buffer[64];
};

void sha1_init(sha1_state_t *pms);
void sha1_append(sha1_state_t *pms, const void *data, uint32_t nbytes);
void sha1_finish(sha1_state_t *pms, uint8_t digest[20]);

#endif
