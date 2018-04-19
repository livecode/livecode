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

//
// regex code for match functions
//

#ifndef	MCREGEX_H
#define	MCREGEX_H

#define REG_OKAY 0

#define PATTERN_CACHE_SIZE 20

//regex structure
typedef struct
{
	void *re_pcre;
	size_t re_nsub;
	size_t re_erroffset;
	// JS-2013-07-01: [[ EnhancedFilter ]] The pattern associated with the compiled
	//   regexp (used by the cache).
	MCStringRef re_pattern;
	// JS-2013-07-01: [[ EnhancedFilter ]] The flags used to compile the pattern
	//   (used to implement caseSensitive option).
	int re_flags;
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
	regex_t *rexp;
	uint2 nsubs;
	regmatch_t matchinfo[NSUBEXP];
}
regexp;

// JS-2013-07-01: [[ EnhancedFilter ]] Updated to manage case and allow case-insensitive matching.
// MW-2013-07-01: [[ EnhancedFilter ]] Removed 'usecache' parameter as there's no reason not to use the cache.
regexp *MCR_compile(MCStringRef exp, bool casesensitive);
int MCR_exec(regexp *prog, MCStringRef string, MCRange p_range);
void MCR_copyerror(MCStringRef &r_error);
void MCR_free(regex_t *prog);

// JS-2013-07-01: [[ EnhancedFilter ]] Clear out the PCRE cache.
void MCR_clearcache();

#endif
