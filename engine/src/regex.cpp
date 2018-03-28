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

#include "globdefs.h"
#include "parsedef.h"
#include "regex.h"

#include "pcre.h"

#define pcre16_free free




extern "C"
{
	/* Options defined by POSIX. */

#define REG_ICASE     0x01
#define REG_NEWLINE   0x02
#define REG_NOTBOL    0x04
#define REG_NOTEOL    0x08

	/* These are not used by PCRE, but by defining them we make it easier
	to slot PCRE into existing programs that make POSIX calls. */

#define REG_EXTENDED  0
#define REG_NOSUB     0

	/* Error values. Not all these are relevant or used by the wrapper. */

	enum {
	    REG_ASSERT = 1,  /* internal error ? */
	    REG_BADBR,       /* invalid repeat counts in {} */
	    REG_BADPAT,      /* pattern error */
	    REG_BADRPT,      /* ? * + invalid */
	    REG_EBRACE,      /* unbalanced {} */
	    REG_EBRACK,      /* unbalanced [] */
	    REG_ECOLLATE,    /* collation error - not relevant */
	    REG_ECTYPE,      /* bad class */
	    REG_EESCAPE,     /* bad escape sequence */
	    REG_EMPTY,       /* empty expression */
	    REG_EPAREN,      /* unbalanced () */
	    REG_ERANGE,      /* bad range inside [] */
	    REG_ESIZE,       /* expression too big */
	    REG_ESPACE,      /* failed to get memory */
	    REG_ESUBREG,     /* bad back reference */
	    REG_INVARG,      /* bad argument */
	    REG_NOMATCH      /* match failed */
	};
}   /* extern "C" */

static int eint[] =
{
	REG_EESCAPE, /* "\\ at end of pattern" */
	REG_EESCAPE, /* "\\c at end of pattern" */
	REG_EESCAPE, /* "unrecognized character follows \\" */
	REG_BADBR,   /* "numbers out of order in {} quantifier" */
	REG_BADBR,   /* "number too big in {} quantifier" */
	REG_EBRACK,  /* "missing terminating ] for character class" */
	REG_ECTYPE,  /* "invalid escape sequence in character class" */
	REG_ERANGE,  /* "range out of order in character class" */
	REG_BADRPT,  /* "nothing to repeat" */
	REG_BADRPT,  /* "operand of unlimited repeat could match the empty string" */
	REG_ASSERT,  /* "internal error: unexpected repeat" */
	REG_BADPAT,  /* "unrecognized character after (?" */
	REG_ESIZE,   /* "too many capturing parenthesized sub-patterns" */
	REG_EPAREN,  /* "missing )" */
	REG_ESUBREG, /* "back reference to non-existent subpattern" */
	REG_INVARG,  /* "erroffset passed as NULL" */
	REG_INVARG,  /* "unknown option bit(s) set" */
	REG_EPAREN,  /* "missing ) after comment" */
	REG_ESIZE,   /* "too many sets of parentheses" */
	REG_ESIZE,   /* "regular expression too large" */
	REG_ESPACE,  /* "failed to get memory" */
	REG_EPAREN,  /* "unmatched brackets" */
	REG_ASSERT,  /* "internal error: code overflow" */
	REG_BADPAT,  /* "unrecognized character after (?<" */
	REG_BADPAT,  /* "lookbehind assertion is not fixed length" */
	REG_BADPAT,  /* "malformed number after (?(" */
	REG_BADPAT,  /* "conditional group containe more than two branches" */
	REG_BADPAT,  /* "assertion expected after (?(" */
	REG_BADPAT,  /* "(?p must be followed by )" */
	REG_ECTYPE,  /* "unknown POSIX class name" */
	REG_BADPAT,  /* "POSIX collating elements are not supported" */
	REG_INVARG,  /* "this version of PCRE is not compiled with PCRE_UTF8 support" */
	REG_BADPAT,  /* "characters with values > 255 are not yet supported in classes" */
	REG_BADPAT,  /* "character value in \x{...} sequence is too large" */
	REG_BADPAT,   /* "invalid condition (?(0)" */
	REG_BADPAT,  /* "\\C not allowed in lookbehind assertion" */
	REG_EESCAPE, /* "PCRE does not support \\L, \\l, \\N, \\U, or \\u" */
	REG_BADPAT,  /* "number after (?C is > 255" */
	REG_BADPAT,  /* "closing ) for (?C expected" */
	REG_BADPAT,  /* "recursive call could loop indefinitely" */
	REG_BADPAT,  /* "unrecognized character after (?P" */
	REG_BADPAT,  /* "syntax error after (?P" */
	REG_BADPAT,  /* "two named groups have the same name" */
	REG_BADPAT,  /* "invalid UTF-8 string" */
	REG_BADPAT,  /* "support for \\P, \\p, and \\X has not been compiled" */
	REG_BADPAT,  /* "malformed \\P or \\p sequence" */
	REG_BADPAT,  /* "unknown property name after \\P or \\p" */
	REG_BADPAT,  /* subpattern name is too long (maximum 32 characters) */
	REG_BADPAT,  /* too many named subpatterns (maximum 10,000) */
	REG_BADPAT,  /* repeated subpattern is too long */
	REG_BADPAT   /* octal value is greater than \377 (not in UTF-8 mode) */
};

/* Table of texts corresponding to POSIX error codes */

static const char *pstring[] =
    {
        "",                                /* Dummy for value 0 */
        "internal error",                  /* REG_ASSERT */
        "invalid repeat counts in {}",     /* BADBR      */
        "pattern error",                   /* BADPAT     */
        "? * + invalid",                   /* BADRPT     */
        "unbalanced {}",                   /* EBRACE     */
        "unbalanced []",                   /* EBRACK     */
        "collation error - not relevant",  /* ECOLLATE   */
        "bad class",                       /* ECTYPE     */
        "bad escape sequence",             /* EESCAPE    */
        "empty expression",                /* EMPTY      */
        "unbalanced ()",                   /* EPAREN     */
        "bad range inside []",             /* ERANGE     */
        "expression too big",              /* ESIZE      */
        "failed to get memory",            /* ESPACE     */
        "bad back reference",              /* ESUBREG    */
        "bad argument",                    /* INVARG     */
        "match failed"                     /* NOMATCH    */
    };

/*************************************************
*          Translate error code to string        *
*************************************************/

size_t
regerror(int errcode, const regex_t *preg, MCStringRef &errbuf)
{
	const char *message, *addmessage;
	size_t length, addlength;

	message = (errcode >= (int)(sizeof(pstring)/sizeof(char *)))
	          ? "unknown error code" : pstring[errcode];
	length = strlen(message) + 1;

	addmessage = " at offset ";
	addlength = (preg != NULL && (int)preg->re_erroffset != -1)
	            ? strlen(addmessage) + 6 : 0;
	
	
    if (addlength > 0)
    {
        MCAutoStringRef t_error_string;
        MCStringFormat(&t_error_string, "%s%s%-6d", message, addmessage, (int)preg->re_erroffset);
        MCValueAssign(errbuf, *t_error_string);

    }
    else
    {
		if (errbuf != nil)
			MCValueRelease(errbuf);
        /* UNCHECKED */ MCStringCreateWithNativeChars((const char_t *) message, strlen(message), errbuf);
    }
	
	return length + addlength;
}




/*************************************************
*           Free store held by a regex           *
*************************************************/

void regfree(regex_t *preg)
{
	(pcre16_free)(preg->re_pcre);
}

/*************************************************
*            Compile a regular expression        *
*************************************************/

/*
Arguments:
  preg        points to a structure for recording the compiled expression
  pattern     the pattern to compile
  cflags      compilation flags
 
Returns:      0 on success
              various non-zero codes on failure
*/

int regcomp(regex_t *preg, MCStringRef pattern, int cflags)
{
	const char *errorptr;
	int erroffset;
	int options = 0;

	if ((cflags & REG_ICASE) != 0)
		options |= PCRE_CASELESS;
	if ((cflags & REG_NEWLINE) != 0)
		options |= PCRE_MULTILINE;

	// AL-2014-08-20: [[ Bug 13186 ]] Ensure pattern string doesn't get
	// permanently converted to UTF-16
	MCAutoStringRef t_temp;
	/* UNCHECKED */ MCStringMutableCopy(pattern, &t_temp);

	// SN-2014-01-14: [[ libpcre update ]]
	preg->re_pcre = pcre16_compile((PCRE_SPTR16)MCStringGetCharPtr(*t_temp),
									options,
									&errorptr,
									&erroffset,
									NULL);
	preg->re_erroffset = erroffset;

	if (preg->re_pcre == NULL)
		return eint[erroffset];

	/* UNCHECKED */ preg->re_pattern = MCValueRetain(pattern);
	preg->re_flags = cflags;

	// SN-2014-01-10: [[ libpcre udpate ]] pcre_info() is deprecated,
	// must be replaced with pcre_fullinfo()
	return pcre16_fullinfo((const pcre16 *)preg->re_pcre,
							NULL,
							PCRE_INFO_CAPTURECOUNT,
							&preg->re_nsub);
}

/*************************************************
*              Match a regular expression        *
*************************************************/

/* Unfortunately, PCRE requires 3 ints of working space for each captured
substring, so we have to get and release working store instead of just using
the POSIX structures as was done in earlier releases when PCRE needed only 2
ints. */

int regexec(regex_t *preg, const unichar_t *string, int len, size_t nmatch,
            regmatch_t pmatch[], int eflags)
{
	int rc;
	int options = 0;
	int *ovector = NULL;

	if ((eflags & REG_NOTBOL) != 0)
		options |= PCRE_NOTBOL;
	if ((eflags & REG_NOTEOL) != 0)
		options |= PCRE_NOTEOL;

	preg->re_erroffset = (size_t)(-1);   /* Only has meaning after compile */
	if (nmatch > 0)
	{
		ovector = (int *)malloc(sizeof(int) * nmatch * 3);
		if (ovector == NULL)
			return REG_ESPACE;
	}

	// [[ libprce update ]] SN-2014-01-14: now handles unicode-encoded input
	rc = pcre16_exec((const pcre16 *)preg->re_pcre,
					  NULL,
					  (PCRE_SPTR16)string,
					  len,
					  0,
					  options,
					  ovector,
					  nmatch * 3);

	if (rc == 0)
		rc = nmatch;    /* All captured slots were filled in */

	if (rc >= 0)
	{
		int i;
		for (i = 0 ; i < rc ; i++)
		{
			pmatch[i].rm_so = ovector[i*2];
			pmatch[i].rm_eo = ovector[i*2+1];
		}
		if (ovector != NULL)
			free(ovector);
		for (; i < (int)nmatch; i++)
			pmatch[i].rm_so = pmatch[i].rm_eo = -1;
		return 0;
	}
	else
	{
		if (ovector != NULL)
			free(ovector);
		switch(rc)
		{
		case PCRE_ERROR_NOMATCH:
			return REG_NOMATCH;
		case PCRE_ERROR_NULL:
			return REG_INVARG;
		case PCRE_ERROR_BADOPTION:
			return REG_INVARG;
		case PCRE_ERROR_BADMAGIC:
			return REG_INVARG;
		case PCRE_ERROR_UNKNOWN_NODE:
			return REG_ASSERT;
		case PCRE_ERROR_NOMEMORY:
			return REG_ESPACE;
		default:
			return REG_ASSERT;
		}
	}
}

static MCStringRef regexperror;

void MCR_copyerror(MCStringRef &r_error)
{
    if (regexperror == nil)
        r_error = MCValueRetain(kMCEmptyString);
    else
        r_error = MCValueRetain(regexperror);
}

regex_t *MCregexcache[PATTERN_CACHE_SIZE];

// JS-2013-07-01: [[ EnhancedFilter ]] Updated to support case-sensitivity and caching.
// MW-2013-07-01: [[ EnhancedFilter ]] Tweak to take 'const char *' and copy pattern as required.
// MW-2013-07-01: [[ EnhancedFilter ]] Removed 'usecache' parameter as there's
//   no reason not to use the cache.
regexp *MCR_compile(MCStringRef exp, bool casesensitive)
{
	Boolean found = False;
	regex_t *re = nil;
	int flags = REG_EXTENDED;
	if (!casesensitive)
		flags |= REG_ICASE;

	// Search the cache - keeping a note of any free slots in t_last_free.
	uint2 i;
	int t_last_free = -1;
	for (i = 0 ; i < PATTERN_CACHE_SIZE ; i++)
	{
		if (MCregexcache[i] == NULL)
		{
			t_last_free = i;
		}
		
		// Notice we only do a pointer comparison here. So the cache will
		// work if the same pattern valueref is used more than once, but
		// not if a pattern has a different valueref but same content.
		if (MCregexcache[i] &&
			exp == MCregexcache[i]->re_pattern &&
			flags == MCregexcache[i]->re_flags)
		{
			found = True;
			re = MCregexcache[i];
			break;
		}
	}
	
	// If the pattern isn't found with the given flags, then create a new one.
	if (re == nil)
	{
		/* UNCHECKED */ re = new(std::nothrow) regex_t;
		int status;
		status = regcomp(re, exp, flags);
		if (status != REG_OKAY)
		{
			regerror(status, nil, regexperror);
			delete re;
			return(nil);
		}
	}
	
	// If the pattern is new, put it in the cache.
	if (!found)
	{
		// If we don't have a free slot, evict a randomish one.
		if (t_last_free == -1)
		{
			t_last_free = MCHashPointer(exp) % PATTERN_CACHE_SIZE;
			MCR_free(MCregexcache[t_last_free]);
		}
		
		// Make sure the new re is at position 0 in the cache.
		// (We assume that if a pattern is used once then it is
		// likely to be used again immediately after).
		MCregexcache[t_last_free] = MCregexcache[0];
		MCregexcache[0] = re;
	}
	
	regexp *treg = nil;
	/* UNCHECKED */ treg = new(std::nothrow) regexp;
	treg->rexp = re;
	
	return treg;
}

int MCR_exec(regexp *prog, MCStringRef string, MCRange p_range)
{
	int status;
	int flags = 0;
	
	// Make sure we take a copy of the string to ensure it is Unicode.
	// Otherwise, subsequent uses of 'string' will follow much slower
	// codepaths causing unexpected degredation in performance.
	MCAutoStringRef t_uni_string;
	if (!MCStringUnicodeCopy(string, &t_uni_string))
	{
		regerror(REG_ESPACE, NULL, regexperror);
		return 0;
	}
	
	// AL-2014-06-25: [[ Bug 12676 ]] Ensure string is not unnativized by MCR_exec
	// AL-2015-02-05: [[ Bug 14504 ]] Now that 'CanBeNative' flag is preserved, we can just use MCStringGetCharPtr here.
	status = regexec(prog->rexp,
					 MCStringGetCharPtr(*t_uni_string) + p_range . offset,
					 p_range . length,
					 NSUBEXP,
					 prog->matchinfo,
					 flags);

	if (status != REG_OKAY)
	{
		if (status == REG_NOMATCH)
		{
			return (0);
		}
		//MCValueRelease(regexperror);
		regerror(status, NULL, regexperror);
		return(0);
	}
	return (1);
}

void MCR_free(regex_t *preg)
{
	if (preg)
	{
		regfree(preg);
		// MW-2013-07-01: [[ EnhancedFilter ]] Release the pattern.
		MCValueRelease(preg->re_pattern);
		delete preg;
	}
}

// JS-2013-07-01: [[ EnhancedFilter ]] Clear out the cache.
void MCR_clearcache()
{
	uint2 i;
	for (i = 0 ; i < PATTERN_CACHE_SIZE ; i++)
	{
		MCR_free(MCregexcache[i]);
		// PM-2014-10-02: [[ Bug 11647 ]] Make sure we clear old data to prevent a crash when restarting the app
		MCregexcache[i] = nil;
	}
}
