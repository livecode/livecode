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

#include "regex.h"

////////////////////////////////////////////////////////////////////////////////

// JS-2013-07-01: [[ EnhancedFilter ]] Utility class and descendents handling the
//   regex and wildcard style pattern matchers.
class MCPatternMatcher
{
protected:
	MCStringRef pattern;
	bool casesensitive;
public:
	MCPatternMatcher(MCStringRef p, bool cs)
	{
		pattern = MCValueRetain(p);
		casesensitive = cs;
	}
	virtual ~MCPatternMatcher();
	virtual bool compile(MCStringRef& r_error) = 0;
	virtual bool match(MCStringRef s) = 0;
};

class MCRegexMatcher : public MCPatternMatcher
{
protected:
	regexp *compiled;
public:
	MCRegexMatcher(MCStringRef p, bool cs) : MCPatternMatcher(p, cs)
	{
		compiled = NULL;
	}
	virtual bool compile(MCStringRef& r_error);
	virtual bool match(MCStringRef s);
};

class MCWildcardMatcher : public MCPatternMatcher
{
public:
	MCWildcardMatcher(MCStringRef p, Boolean cs) : MCPatternMatcher(p, cs)
	{
	}
	virtual bool compile(MCStringRef& r_error);
	virtual bool match(MCStringRef s);
protected:
	static bool match(const char *s, const char *p, Boolean cs);
};

MCPatternMatcher::~MCPatternMatcher()
{
	MCValueRelease(pattern);
}

////////////////////////////////////////////////////////////////////////////////