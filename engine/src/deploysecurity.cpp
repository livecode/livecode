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

#include "prefix.h"

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "deploy.h"

////////////////////////////////////////////////////////////////////////////////

bool MCDeploySecuritySecureStandalone(void *p_file, uint32_t p_start_offset, uint32_t p_amount, uint32_t& x_offset, uint8_t *p_digest)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		uint32_t t_zero;
		t_zero = 0;
		x_offset = (x_offset + 3) & ~3;
		t_success = MCDeployFileWriteAt((MCDeployFileRef)p_file, &t_zero, sizeof(uint32_t), x_offset);
	}
	
	if (t_success)
		x_offset += sizeof(uint32_t);
	
	return t_success;
}


////////////////////////////////////////////////////////////////////////////////
