/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

//
// regex code for match functions
//

#ifndef	MCREGEX_H
#define	MCREGEX_H

#define REG_OKAY 0

#define PATTERN_CACHE_SIZE 20
extern MCStringRef MCregexpatterns[];

//regex structure
typedef struct
{
	void *re_pcre;
	size_t re_nsub;
	size_t re_erroffset;
}
regex_t;

/* The structure in which a captured offset is returned. */

typedef int regoff_t;

typedef struct
{
	regoff_t rm_so;
	regoff_t rm_eo;
}
regmatch_t;

#define NSUBEXP  50

typedef struct _regexp
{
	regex_t rexp;
	uint2 nsubs;
	regmatch_t matchinfo[NSUBEXP];
}
regexp;

void MCR_geterror(MCStringRef &r_error);
regexp *MCR_compile(MCStringRef exp);
int MCR_exec(regexp *prog, MCStringRef string, uint4 len);
void MCR_free(regexp *prog);

extern regexp *MCregexcache[];
#endif
