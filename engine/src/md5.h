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

////////////////////////////////////////////////////////////////////////////////
//
//  Private Header File:
//    md5.h
//
//  Description:
//    This file contains definitions for a stream-based md5 digest generator.
//
//  Changes:
//    2009-06-24 MW Created by refactoring code from funcsm.cpp.
//    2009-07-04 MW Added md5_finish_copy which constructs the md5 without
//                  changing the state.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MC_MD5__
#define __MC_MD5__

typedef unsigned char md5_byte_t; /* 8-bit byte */
typedef unsigned int md5_word_t; /* 32-bit word */

/* Define the state of the MD5 Algorithm. */
typedef struct md5_state_s
{
	md5_word_t count[2];        /* message length in bits, lsw first */
	md5_word_t abcd[4];         /* digest buffer */
	md5_byte_t buf[64];         /* accumulate block */
}
md5_state_t;

void md5_init(md5_state_t *pms);
void md5_append(md5_state_t *pms, const md5_byte_t *data, int nbytes);
void md5_finish_copy(md5_state_t *pms, md5_byte_t digest[16]);
void md5_finish(md5_state_t *pms, md5_byte_t digest[16]);
void md5_compute(const char *p_data, unsigned int p_length, void *p_buffer);

#endif
