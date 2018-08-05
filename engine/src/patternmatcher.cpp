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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "handler.h"
#include "variable.h"
#include "hndlrlst.h"
#include "osspec.h"

#include "scriptpt.h"
#include "util.h"

#include "exec.h"
#include "exec-strings.h"

#include "chunk.h"
#include "date.h"

#include "foundation-chunk.h"
#include "patternmatcher.h"

MCPatternMatcher::MCPatternMatcher(MCStringRef p_pattern, MCStringRef p_string, MCStringOptions p_options)
{
    m_pattern = p_pattern;
    if (m_pattern != nil)
        MCValueRetain(m_pattern);
    m_string_source = MCValueRetain(p_string);
    m_options = p_options;
    m_array_source = nil;
}
MCPatternMatcher::MCPatternMatcher(MCStringRef p_pattern, MCArrayRef p_array, MCStringOptions p_options)
{
    m_pattern = p_pattern;
    if (m_pattern != nil)
        MCValueRetain(m_pattern);
    m_array_source = MCValueRetain(p_array);
    m_options = p_options;
    m_string_source = nil;
}

MCPatternMatcher::~MCPatternMatcher()
{
    MCValueRelease(m_pattern);
    MCValueRelease(m_string_source);
    MCValueRelease(m_array_source);
}


MCRegexMatcher::MCRegexMatcher(MCStringRef p_pattern, MCStringRef p_string, MCStringOptions p_options) : MCPatternMatcher(p_pattern, p_string, p_options)
{
    // if appropriate, normalize the pattern string.
    if (p_options & kMCStringOptionNormalizeBit)
    {
        MCAutoStringRef normalized_pattern;
        MCStringNormalizedCopyNFC(m_pattern, &normalized_pattern);
        MCValueAssign(m_pattern, *normalized_pattern);
    }
    
    m_compiled = NULL;
}
MCRegexMatcher::MCRegexMatcher(MCStringRef p_pattern, MCArrayRef p_array, MCStringOptions p_options) : MCPatternMatcher(p_pattern, p_array, p_options)
{
    // if appropriate, normalize the pattern string.
    if (m_options & kMCStringOptionNormalizeBit)
    {
        MCAutoStringRef t_normalized_pattern;
        MCStringNormalizedCopyNFC(m_pattern, &t_normalized_pattern);
        MCValueAssign(m_pattern, *t_normalized_pattern);
    }
    
    m_compiled = NULL;
}

MCRegexMatcher::~MCRegexMatcher()
{
    if (m_compiled != NULL)
        delete m_compiled;
}

// JS-2013-07-01: [[ EnhancedFilter ]] Implementation of pattern matching classes.
bool MCRegexMatcher::compile(MCStringRef& r_error)
{
    // MW-2013-07-01: [[ EnhancedFilter ]] Removed 'usecache' parameter as there's
    //   no reason not to use the cache.
    // AL-2014-07-11: [[ Bug 12797 ]] Compare options correctly
    m_compiled = MCR_compile(m_pattern, (m_options == kMCStringOptionCompareExact || m_options == kMCStringOptionCompareNonliteral));
    if (m_compiled == nil)
    {
        MCR_copyerror(r_error);
        return false;
    }
    return true;
}

bool MCRegexMatcher::match(MCExecContext& ctxt, MCRange p_range)
{
    // if appropriate, normalize the source string.
    // AL-2014-07-11: [[ Bug 12797 ]] Compare options correctly and normalize the source, not the pattern
    if (m_options == kMCStringOptionCompareNonliteral || m_options == kMCStringOptionCompareCaseless)
    {
        MCAutoStringRef t_string, normalized_source;
        MCStringCopySubstring(m_string_source, p_range, &t_string);
        MCStringNormalizedCopyNFC(*t_string, &normalized_source);
        return MCR_exec(m_compiled, *normalized_source, MCRangeMake(0, MCStringGetLength(*normalized_source)));
    }
    
    return MCR_exec(m_compiled, m_string_source, p_range);
    
}

bool MCRegexMatcher::match(MCExecContext& ctxt, MCNameRef p_key, bool p_match_key)
{
    MCAutoStringRef t_string;
    MCAutoStringRef t_normalized_source;
    if (p_match_key)
        t_string = MCNameGetString(p_key);
    else
    {
        MCValueRef t_element;
        if (!MCArrayFetchValue(m_array_source, m_options == kMCStringOptionCompareCaseless, p_key, t_element))
            return false;
        
        if (!ctxt . ConvertToString(t_element, &t_string))
            return false;
    }
        
    MCStringNormalizedCopyNFC(*t_string, &t_normalized_source);
    return MCR_exec(m_compiled, *t_normalized_source, MCRangeMake(0, MCStringGetLength(*t_normalized_source)));
}

MCWildcardMatcher::MCWildcardMatcher(MCStringRef p_pattern, MCStringRef p_string, MCStringOptions p_options) : MCPatternMatcher(p_pattern, p_string, p_options)
{
    m_native = (MCStringIsNative(p_pattern) && MCStringIsNative(p_string));
}

MCWildcardMatcher::MCWildcardMatcher(MCStringRef p_pattern, MCArrayRef p_array, MCStringOptions p_options) : MCPatternMatcher(p_pattern, p_array, p_options)
{
    m_native = false;
}

MCWildcardMatcher::~MCWildcardMatcher()
{
}

bool MCWildcardMatcher::compile(MCStringRef& r_error)
{
    // wildcard patterns are not compiled
    return true;
}

#define OPEN_BRACKET '['
#define CLOSE_BRACKET ']'

static bool MCStringsWildcardMatchNative(const char *s, uindex_t s_length, const char *p, uindex_t p_length, bool casesensitive)
{
    uindex_t s_index = 0;
    uindex_t p_index = 0;
    uint1 scc, c;
    
    while (s_index < s_length)
    {
        scc = *s++;
        s_index++;
        c = *p++;
        p_index++;
        switch (c)
        {
            case OPEN_BRACKET:
            {
                bool ok = false;
                int lc = -1;
                int notflag = 0;
                
                if (*p == '!' )
                {
                    notflag = 1;
                    p++;
                    p_index++;
                }
                while (p_index < p_length)
                {
                    c = *p++;
                    p_index++;
                    if (c == CLOSE_BRACKET && lc >= 0)
                        return ok ? MCStringsWildcardMatchNative(s, s_length - s_index, p, p_length - p_index, casesensitive) : false;
                    else
                        if (c == '-' && lc >= 0 && *p != CLOSE_BRACKET)
                        {
                            c = *p++;
                            p_index++;
                            if (notflag)
                            {
                                if (lc > scc || scc > c)
                                    ok = true;
                                else
                                    return false;
                            }
                            else
                            {
                                if (lc < scc && scc <= c)
                                    ok = true;
                            }
                        }
                        else
                        {
                            if (notflag)
                            {
                                if (scc != c)
                                    ok = true;
                                else
                                    return false;
                            }
                            else
                                if (scc == c)
                                    ok = true;
                            lc = c;
                        }
                }
            }
                return false;
            case '?':
                break;
            case '*':
                while (*p == '*')
                {
                    p++;
                    p_index++;
                }
                if (*p == 0)
                    return true;
                --s;
                --s_index;
                c = *p;
                // AL-2014-05-23: [[ Bug 12489 ]] Ensure source string does not overrun length
                while (*s && s_index < s_length)
                    if ((casesensitive ? c != *s : MCS_tolower(c) != MCS_tolower(*s))
                        && *p != '?' && *p != OPEN_BRACKET)
                    {
                        s++;
                        s_index++;
                    }
                    else
                        if (MCStringsWildcardMatchNative(s++, s_length - s_index++, p, p_length - p_index, casesensitive))
                            return true;
                return false;
            case 0:
                return scc == 0;
            default:
                if (casesensitive)
                {
                    if (c != scc)
                        return false;
                }
                else
                    if (MCS_tolower(c) != MCS_tolower(scc))
                        return false;
                break;
        }
    }
    while (p_index < p_length && *p == '*')
    {
        p++;
        p_index++;
    }
    return p_index == p_length;
}

bool MCWildcardMatcher::match(MCExecContext& ctxt, MCRange p_source_range)
{
    if (m_native)
    {
        const char *t_source = (const char *)MCStringGetNativeCharPtr(m_string_source);
        const char *t_pattern = (const char *)MCStringGetNativeCharPtr(m_pattern);
        
        // AL-2014-05-23: [[ Bug 12489 ]] Pass through case sensitivity properly
        if (t_source != nil && t_pattern != nil)
            return MCStringsWildcardMatchNative(t_source + p_source_range . offset, p_source_range . length, t_pattern, MCStringGetLength(m_pattern), (m_options == kMCStringOptionCompareExact || m_options == kMCStringOptionCompareNonliteral));
    }
    
    return MCStringWildcardMatch(m_string_source, p_source_range, m_pattern, m_options);
}

bool MCWildcardMatcher::match(MCExecContext& ctxt, MCNameRef p_key, bool p_match_key)
{
    MCAutoStringRef t_string;
    if (p_match_key)
        t_string = MCNameGetString(p_key);
    else
    {
        MCValueRef t_element;
        if (!MCArrayFetchValue(m_array_source, m_options == kMCStringOptionCompareCaseless, p_key, t_element))
            return false;
        if (!ctxt . ConvertToString(t_element, &t_string))
            return false;
    }
    
    return MCStringWildcardMatch(*t_string, MCRangeMake(0, MCStringGetLength(*t_string)), m_pattern, m_options);
}


MCExpressionMatcher::MCExpressionMatcher(MCExpression* p_expression, MCStringRef p_string, MCStringOptions p_options)
    : MCPatternMatcher(nil, p_string, p_options)
{
    m_expression = p_expression;
}

MCExpressionMatcher::MCExpressionMatcher(MCExpression* p_expression, MCArrayRef p_array, MCStringOptions p_options)
    : MCPatternMatcher(nil, p_array, p_options)
{
    m_expression = p_expression;
}

MCExpressionMatcher::~MCExpressionMatcher()
{
}

bool MCExpressionMatcher::compile(MCStringRef& r_error)
{
    // expression patterns are not compiled
    return true;
}

bool MCExpressionMatcher::match(MCExecContext& ctxt, MCRange p_source_range)
{
    MCAutoStringRef t_string;
    if (!MCStringCopySubstring(m_string_source, p_source_range, &t_string))
        return false;
    
    MCeach->set(ctxt, *t_string);
    
    MCerrorlock++;
    bool t_match, t_success;
    t_success = ctxt . EvalExprAsBool(m_expression, EE_UNDEFINED, t_match);
    MCerrorlock--;
    
    if (!t_success)
        return t_success;
    
    return t_match;
}

bool MCExpressionMatcher::match(MCExecContext& ctxt, MCNameRef p_key, bool p_match_key)
{
    if (p_match_key)
    {
        MCeach->set(ctxt, p_key);
    }
    else
    {
        MCValueRef t_element;
        if (!MCArrayFetchValue(m_array_source, m_options == kMCStringOptionCompareCaseless, p_key, t_element))
            return false;
        MCeach->set(ctxt, t_element);
    }
    
    MCerrorlock++;
    bool t_match, t_success;
    t_success = ctxt . EvalExprAsBool(m_expression, EE_UNDEFINED, t_match);
    MCerrorlock--;
    
    if (!t_success)
        return t_success;
    
    return t_match;
}

