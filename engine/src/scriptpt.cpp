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
#include "filedefs.h"

#include "scriptpt.h"

#include "hndlrlst.h"
#include "handler.h"
#include "cmds.h"
#include "visual.h"
#include "keywords.h"
#include "property.h"
#include "chunk.h"
#include "funcs.h"
#include "operator.h"
#include "literal.h"
#include "newobj.h"
#include "mcerror.h"
#include "util.h"
#include "variable.h"

#include "globals.h"

#define LOWERED_PAD 64

extern const uint8_t type_table[];
extern const uint8_t unicode_type_table[];
extern const Cvalue *constant_table;
extern const uint4 constant_table_size;
extern const LT * const table_pointers[];
extern const uint2 table_sizes[];
extern const LT command_table[];
extern const uint4 command_table_size;
extern const LT factor_table[];
extern const uint4 factor_table_size;

static struct { codepoint_t codepoint; Symbol_type type; } remainder_table[] =
{
    { 0x0131,  ST_ID },    //    �
    { 0x0152,  ST_ID },    //    �
    { 0x0153,  ST_ID },    //    �
    { 0x0178,  ST_ID },    //    �
    { 0x0192,  ST_ID },    //    �
    { 0x02C6,  ST_ID },    //    �
    { 0x02C7,  ST_ID },    //    �
    { 0x02D8,  ST_ID },    //    �
    { 0x02D9,  ST_ID },    //    �
    { 0x02DA,  ST_ID },    //    �
    { 0x02DB,  ST_ID },    //    �
    { 0x02DC,  ST_ID },    //    �
    { 0x02DD,  ST_ID },    //    �
    { 0x03A9,  ST_ID },    //    �
    { 0x03C0,  ST_ID },    //    �
    { 0x2013,  ST_ID },    //    �
    { 0x2014,  ST_ID },    //    �
    { 0x2018,  ST_ID },    //    �
    { 0x2019,  ST_ID },    //    �
    { 0x201A,  ST_ID },    //    �
    { 0x201C,  ST_ID },    //    �
    { 0x201D,  ST_ID },    //    �
    { 0x201E,  ST_ID },    //    �
    { 0x2020,  ST_ID },    //    �
    { 0x2021,  ST_ID },    //    �
    { 0x2022,  ST_ID },    //    �
    { 0x2026,  ST_ID },    //    �
    { 0x2030,  ST_ID },    //    �
    { 0x2039,  ST_ID },    //    �
    { 0x203A,  ST_ID },    //    �
    { 0x2044,  ST_ID },    //    �
    { 0x20AC,  ST_ID },    //    �
    { 0x2122,  ST_ID },    //    �
    { 0x2202,  ST_ID },    //    �
    { 0x2206,  ST_ID },    //    �
    { 0x220F,  ST_ID },    //    �
    { 0x2211,  ST_ID },    //    �
    { 0x221A,  ST_ID },    //    �
    { 0x221E,  ST_ID },    //    �
    { 0x222B,  ST_ID },    //    �
    { 0x2248,  ST_ID },    //    �
    { 0x2260,  ST_OP },    //    �
    { 0x2264,  ST_OP },    //    �
    { 0x2265,  ST_OP },    //    �
    { 0x25CA,  ST_ID },    //    �
    { 0xF8FF,  ST_ID },    //    �
    { 0xFB01,  ST_ID },    //    �
    { 0xFB02,  ST_ID }     //    �
};

static const unichar_t open_comment[] = {'<', '!', '-', '-'};
static const unichar_t close_comment[] = {'-', '-', '>'};
static const unichar_t rev_tag[] = {'r','e','v'};
static const unichar_t livecode_tag[] = {'l','i','v', 'e', 'c', 'o', 'd', 'e'};

MCScriptPoint::MCScriptPoint(MCObject *o, MCHandlerlist *hl, MCDataRef s)
{
    utf16_script = MCValueRetain(s);
    length = MCDataGetLength(s) / 2 - 1;
    
	curobj = o;
	curhlist = hl;
	curhandler = NULL;
	curptr = tokenptr = backupptr = (const unichar_t *)MCDataGetBytePtr(utf16_script);
    endptr = curptr + length;
    
    uindex_t t_index = 0;
    codepoint = MCUnicodeCodepointAdvance(curptr, length, t_index);
    curlength = t_index;
    
	line = pos = 1;
	escapes = False;
	tagged = False;
	in_tag = False;
	was_in_tag = False;
	token_nameref = MCValueRetain(kMCEmptyName);
}

MCScriptPoint::MCScriptPoint(MCObject *o, MCHandlerlist *hl, MCStringRef s)
{
    unichar_t *t_unicode_string;
    /* UNCHECKED */ MCStringConvertToUnicode(s, t_unicode_string, length);
    /* UNCHECKED */ MCDataCreateWithBytesAndRelease((byte_t *)t_unicode_string, (length + 1) * 2, utf16_script);
    
    curobj = o;
    curhlist = hl;
    curhandler = NULL;
    curptr = tokenptr = backupptr = (const unichar_t *)MCDataGetBytePtr(utf16_script);
    endptr = curptr + length;
    
    uindex_t t_index = 0;
    codepoint = MCUnicodeCodepointAdvance(curptr, length, t_index);
    curlength = t_index;
    
    line = pos = 1;
    escapes = False;
    tagged = False;
    in_tag = False;
    was_in_tag = False;
    token_nameref = MCValueRetain(kMCEmptyName);
}

MCScriptPoint::MCScriptPoint(MCScriptPoint &sp)
{
    utf16_script = MCValueRetain(sp . utf16_script);
    endptr = sp.endptr;
    codepoint = sp.codepoint;
    curlength = sp.curlength;
	curobj = sp.curobj;
	curhlist = sp.curhlist;
	curhandler = sp.curhandler;
	curptr = sp.curptr;
	tokenptr = sp.tokenptr;
	backupptr = sp.backupptr;
	token = sp.token;
	line = sp.line;
	pos = sp.pos;
	escapes = sp.escapes;
	tagged = sp.tagged;
	in_tag = sp.in_tag;
	was_in_tag = sp.was_in_tag;
	token_nameref = MCValueRetain(kMCEmptyName);
    m_type = ST_UNDEFINED;
}

MCScriptPoint::MCScriptPoint(MCExecContext &ctxt)
    : utf16_script(MCValueRetain(kMCEmptyData)),
      length(MCDataGetLength(utf16_script)/sizeof(unichar_t)),
      curptr(reinterpret_cast<const unichar_t*>(MCDataGetBytePtr(utf16_script))),
      tokenptr(curptr),
      backupptr(curptr),
      endptr(curptr + length)
{
    codepoint = '\0';
    curlength = 1;
    curobj = ctxt . GetObject();
    curhlist = ctxt . GetHandlerList();
    curhandler = ctxt . GetHandler();
    line = pos = 0;
    escapes = False;
    tagged = False;
    in_tag = False;
    was_in_tag = False;
    token_nameref = MCValueRetain(kMCEmptyName);
    m_type = ST_UNDEFINED;
}

MCScriptPoint::MCScriptPoint(MCExecContext &ctxt, MCStringRef p_string)
{
    unichar_t *t_unicode_string;
	/* UNCHECKED */ MCStringConvertToUnicode(p_string, t_unicode_string, length);
	/* UNCHECKED */ MCDataCreateWithBytesAndRelease((byte_t *)t_unicode_string, (length + 1) * 2, utf16_script);
    
    curobj = ctxt . GetObject();
    curhlist = ctxt . GetHandlerList();
    curhandler = ctxt . GetHandler();
    curptr = tokenptr = backupptr = (const unichar_t *)MCDataGetBytePtr(utf16_script);
    endptr = curptr + length;
    
    uindex_t t_index = 0;
    codepoint = MCUnicodeCodepointAdvance(curptr, length, t_index);
    curlength = t_index;
    
    line = pos = 0;
    escapes = False;
    tagged = False;
    in_tag = False;
    was_in_tag = False;
    token_nameref = MCValueRetain(kMCEmptyName);
    
    m_type = ST_UNDEFINED;
}

MCScriptPoint::MCScriptPoint(MCStringRef p_string)
{
    unichar_t *t_unicode_string;
	/* UNCHECKED */ MCStringConvertToUnicode(p_string, t_unicode_string, length);
	/* UNCHECKED */ MCDataCreateWithBytesAndRelease((byte_t *)t_unicode_string, (length + 1) * 2, utf16_script);
    
	curobj = NULL;
	curhlist = NULL;
	curhandler = NULL;
	curptr = tokenptr = backupptr = (const unichar_t *)MCDataGetBytePtr(utf16_script);
    endptr = curptr + length;
    
    uindex_t t_index = 0;
    codepoint = MCUnicodeCodepointAdvance(curptr, length, t_index);
    curlength = t_index;
    
	line = pos = 0;
	escapes = False;
	tagged = False;
	in_tag = False;
	was_in_tag = False;
	token_nameref = MCValueRetain(kMCEmptyName);
    
    m_type = ST_UNDEFINED;
}

MCScriptPoint& MCScriptPoint::operator =(const MCScriptPoint& sp)
{
    MCValueAssign(utf16_script, sp . utf16_script);
    codepoint = sp.codepoint;
    curlength = sp.curlength;;
    
	curobj = sp.curobj;
	curhlist = sp.curhlist;
	curhandler = sp.curhandler;
	curptr = sp.curptr;
	tokenptr = sp.tokenptr;
	backupptr = sp.backupptr;
    endptr = sp.endptr;
	token = sp.token;
	line = sp.line;
	pos = sp.pos;
    m_type = sp.m_type;
    MCValueAssign(token_nameref, sp.token_nameref);
	return *this;
}

MCScriptPoint::~MCScriptPoint()
{
	MCValueRelease(token_nameref);
	MCValueRelease(utf16_script);
}

void MCScriptPoint::cleartoken(void)
{
	token . setlength(0);
	MCValueAssign(token_nameref, kMCEmptyName);
}

bool MCScriptPoint::token_is_cstring(const char *p_cstring)
{
	return MCStringIsEqualToCString(gettoken_stringref(), p_cstring, kMCCompareCaseless);
}

MCNameRef MCScriptPoint::gettoken_nameref(void)
{
	if (MCNameIsEmpty(token_nameref))
    {
        MCAutoStringRef t_string_token;
        if (token_nameref != nil)
            MCValueRelease(token_nameref);
        /* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)token . getstring(), (token . getlength() * 2), kMCStringEncodingUTF16, false, &t_string_token);
		/* UNCHECKED */ MCNameCreate(*t_string_token, token_nameref);
    }
	return token_nameref;
}

MCStringRef MCScriptPoint::gettoken_stringref(void)
{
	MCNameRef t_name_token;
	t_name_token = gettoken_nameref();
	return MCNameGetString(t_name_token);
}

Symbol_type MCScriptPoint::gettype(codepoint_t p_codepoint)
{
    Symbol_type type;
    type = ST_UNDEFINED;
    
    // Check type table for first 256 Unicode codepoints
    if (p_codepoint <= 0x00FF)
        type = unicode_type_table[p_codepoint];
    
    if (type == ST_UNDEFINED)
    {
        // Otherwise check mac roman from 128 - 256
        uint2 high = ELEMENTS(remainder_table);
        uint2 low = 0;
        codepoint_t t_new_codepoint;
        
        while (low < high)
        {
            uint2 mid = low + ((high - low) >> 1);
            t_new_codepoint = remainder_table[mid] . codepoint;
            if (t_new_codepoint == p_codepoint)
                return remainder_table[mid] . type;
            
            if (t_new_codepoint > p_codepoint)
                high = mid;
            else
                low = mid + 1;
        }
    }
    
    return type;
}

bool MCScriptPoint::is_identifier(codepoint_t p_codepoint, bool p_initial)
{
    Symbol_type t_type;
    t_type = gettype(p_codepoint);
    if (t_type != ST_UNDEFINED)
    {
        if (t_type == ST_ID || (!p_initial && t_type == ST_NUM))
            return true;
        
        return false;
    }
    
    if (p_initial)
        return MCUnicodeIsIdentifierInitial(p_codepoint);
    
    return MCUnicodeIsIdentifierContinue(p_codepoint);
}

void MCScriptPoint::advance(uindex_t number)
{
    curptr += curlength;
    
    uindex_t t_index = 0;
    while (--number)
        MCUnicodeCodepointAdvance(curptr, endptr - curptr, t_index);

    curptr += t_index;
    
    t_index = 0;
    codepoint = MCUnicodeCodepointAdvance(curptr, endptr - curptr, t_index);
    curlength = t_index;
}

codepoint_t MCScriptPoint::getcurrent()
{
    return codepoint;
}

codepoint_t MCScriptPoint::getnext()
{
    uindex_t t_index = 0;
    return MCUnicodeCodepointAdvance((curptr + curlength), endptr - curptr - curlength, t_index);
}

codepoint_t MCScriptPoint::getcodepointatindex(uindex_t p_index)
{
    uindex_t t_index = 0;
    while (p_index--)
        MCUnicodeCodepointAdvance(curptr, endptr - curptr - t_index, t_index);
    
    return MCUnicodeCodepointAdvance(curptr, endptr - curptr - t_index, t_index);
}

void MCScriptPoint::setcurptr(const unichar_t *ptr)
{
    curptr = ptr;
    uindex_t t_index = 0;
    codepoint = MCUnicodeCodepointAdvance(curptr, endptr - curptr, t_index);
    curlength = t_index;
}

#ifdef OLD_SCRIPT_POINT
Parse_stat MCScriptPoint::skip_space()
{
	for(;;)
	{
        if (gettype(*curptr) == ST_SPC)
			curptr++;
		else
			break;
	}
    
	switch (gettype(*curptr))
	{
        case ST_COM:
            while (*curptr && gettype(*curptr) != ST_EOL)
                curptr++;
            if (!*curptr)
                return PS_EOF;
            return PS_EOL;
        case ST_MIN:
            if (gettype(*(curptr + 1), true) == ST_MIN)
            {
                while (*curptr && gettype(*curptr) != ST_EOL)
                    curptr++;
                if (*curptr)
                    return PS_EOL;
                else
                    return PS_EOF;
            }
            else
                return PS_NORMAL;
        case ST_OP:
            if (*curptr == '/' && *(curptr + 1) == '/')
            {
                while (*curptr && gettype(*curptr) != ST_EOL)
                    curptr++;
                if (*curptr)
                    return PS_EOL;
                else
                    return PS_EOF;
            }
            else
                if (*curptr == '/' && *(curptr + 1) == '*')
                {
                    const unichar_t *startptr = curptr;
                    const uint2 startline = line;
                    const uint2 startpos = pos;
                    do
                    {
                        curptr++;
                        if (*curptr == '*' && *(curptr + 1) == '/')
                            break;
                        if (gettype(*curptr) == ST_EOL)
                        {
                            // MW-2011-06-23: [[ SERVER ]] If the line ends with CR LF
                            //   then eat the LF.
                            if (curptr[0] == 13 && curptr[1] == 10)
                                curptr++;
                            
                            line++;
                            pos = 1;
                        }
                        else
                            pos++;
                    }
                    while (*curptr);
                    if (*curptr)
                    {
                        curptr += 2;
                        return skip_space();
                    }
                    else
                    {
                        curptr = startptr;
                        line = startline;
                        pos = startpos;
                        return PS_ERROR;
                    }
                }
                else
                    return PS_NORMAL;
        case ST_ESC:
            while (*curptr && gettype(*curptr) != ST_EOL)
                curptr++;
            if (!*curptr)
                return PS_EOF;
            // MW-2011-06-23: [[ SERVER ]] If the line ends with CR LF
            //   then eat the LF.
            if (curptr[0] == 13 && curptr[1] == 10)
                curptr++;
            curptr++;
            line++;
            pos = 1;
            return skip_space();
        case ST_EOF:
            return PS_EOF;
        case ST_SEMI:
        case ST_EOL:
            return PS_EOL;
        case ST_ERR:
            return PS_ERROR;
        case ST_TAG:
            // MW-2011-06-23: [[ SERVER ]] Make sure we return EOL when we
            //   encounter '?>' (?> is a command separator, essentially)
            if (in_tag && curptr[1] == '>')
                return PS_EOL;
            return PS_NORMAL;
        default:
            return PS_NORMAL;
	}
}

Parse_stat MCScriptPoint::skip_eol()
{
	Symbol_type type;
	Boolean lit = False;
	do
	{
		type = gettype(*curptr);
		if (type == ST_EOF)
			return PS_EOF;
		if (type == ST_LIT)
			lit = !lit;
		// MW-2011-06-23: [[ SERVER ]] When we are asked to skip past a ?>
		//   we must eat the following newling - PHP-semantics.
		if (in_tag && type == ST_TAG && curptr[1] == '>')
		{
			in_tag = False;
			
			// Make sure we eat a subsequence newline
			if (curptr[2] == 10)
			{
				// Take account of CR LF line ending
				if (curptr[3] == 13)
					curptr += 1;
				
				pos = 1;
				curptr += 3;
				line += 1;
			}
			else
			{
				pos += 2;
				curptr += 2;
			}
            
			tokenptr = curptr;
			break;
		}
		curptr++;
	}
	while (type != ST_EOL && (type != ST_SEMI || lit));
	if (type == ST_EOL)
	{
		// MW-2011-06-23: [[ SERVER ]] If the line ends with CR LF
		//   then eat the LF.
		if (curptr[-1] == 13 && curptr[0] == 10)
			curptr++;
		line++;
		pos = 1;
		tokenptr = curptr;
	}
	return PS_NORMAL;
}

Parse_stat MCScriptPoint::backup()
{
	if (curptr == tokenptr)
	{
		pos -= curptr - backupptr;
		curptr = backupptr;
	}
	else
	{
		pos -= curptr - tokenptr;
		curptr = tokenptr;
		
		// MW-2011-06-23: [[ SERVER ]] Restore the backup 'in tag' state.
		if (tagged)
			in_tag = was_in_tag;
	}
	cleartoken();
	return PS_NORMAL;
}

Parse_stat MCScriptPoint::next(Symbol_type &type)
{
	Parse_stat stat;
	
	cleartoken();
    
	const unichar_t *startptr = curptr;
	
	// MW-2011-06-23: [[ SERVER ]] If we are in tagged mode and not in a tag, we
    //   check to see if there is a ST_DATA to produce. This involves advancing
    //   through the input buffer until we encounter a '<?rev'
	if (tagged && !in_tag)
	{
		// We were previously not in a tag, so we need to potentially skip '?>' and subsequent
		// newline. (Indeed, this will be case if we are not at the start)
		if ((line != 1 || pos != 1) && curptr[0] == '?' && curptr[1] == '>')
		{
			if (curptr[2] == 10)
			{
				// Take account of CR LF line ending
				if (curptr[3] == 13)
					curptr += 1;
				pos = 1;
				curptr += 3;
				line += 1;
			}
			else
			{
				pos += 2;
				curptr += 2;
			}
			startptr = curptr;
		}
		
		// Store the previous tag state for backup purposes.
		was_in_tag = False;
		
		// We will be inside a tag after this (or at the end!)
		in_tag = True;
		
		// Stores the length of the <? tag (if found)
		uint32_t t_tag_length = 0;
        
		// Loop until a NUL char, or we find '<?rev'
		bool t_in_comment;
		t_in_comment = false;
		while(*curptr != '\0')
		{
			if (!t_in_comment && curptr[0] == '<' && curptr[1] == '!' && curptr[2] == '-' && curptr[3] == '-')
			{
				pos += 4;
				curptr += 4;
				t_in_comment = true;
				continue;
			}
			else if (t_in_comment && curptr[0] == '-' && curptr[1] == '-' && curptr[2] == '>')
			{
				pos += 3;
				curptr += 3;
				t_in_comment = false;
				continue;
			}
			else if (!t_in_comment && curptr[0] == '<' && curptr[1] == '?')
			{
				if (curptr[2] == 'r' && curptr[3] == 'e' && curptr[4] == 'v')
				{
					t_tag_length = 5;
					break;
				}
				else if (curptr[2] == 'l' && curptr[3] == 'c')
				{
					t_tag_length = 4;
					break;
				}
				else if (strncmp((const char *)(curptr + 2), "livecode", 8) == 0)
				{
					t_tag_length = 10;
					break;
				}
			}
            
			// Check for and advance past any newlines
			if (curptr[0] == 13)
			{
				if (curptr[1] == 10)
					curptr += 1;
                
				pos = 1, line += 1;
			}
			else if (curptr[0] == 10)
				pos = 1, line += 1;
			
			pos += 1;
			curptr += 1;
		}
		
		if (curptr != startptr)
		{
			// Type of symbol is ST_DATA
			type = ST_DATA;
			
			// Set the previous token-pointer
			backupptr = tokenptr;
			
			// Token starts at start (should be immediately after a ?> or beginning of file).
			tokenptr = startptr;
			
			// Set the token string appropriately.
			token.setstring((const char *)tokenptr);
			token.setlength(curptr - tokenptr);
			
			// If we aren't looking at the end of the data, then advance by 5 to skip '<?rev'.
			if (*curptr != '\0')
				curptr += t_tag_length;
			
			// Return our token.
			return PS_NORMAL;
		}
		else
		{
			// There is no literal data, so just advance curptr and carry on.
			if (*curptr != '\0')
				curptr += t_tag_length;
		}
	}
	else if (tagged)
		was_in_tag = True;
	
	if ((stat = skip_space()) != PS_NORMAL)
	{
		if (stat == PS_ERROR)
			MCperror->add(PE_PARSE_BADCHAR, *this);
		token.setstring((const char *)curptr);
		return stat;
	}
	
    if (is_identifier(*curptr, true))
        type = ST_ID;
    else
        type = gettype(*curptr);
    
	if (type == ST_TAG)
	{
		if (tagged && curptr[1] == '>')
			return PS_EOL;
		else
			type = ST_ID;
	}
	if (type == ST_EOF)
	{
		token.setstring((const char *)curptr);
		return PS_EOF;
	}
	if (type == ST_EOL || type == ST_SEMI)
		return PS_EOL;
	if (curptr != tokenptr)
	{
		backupptr = tokenptr;
		tokenptr = curptr;
	}
	if (type == ST_LIT)
		curptr++;
	token.setstring((const char *)curptr);
    
	switch (type)
	{
        case ST_ID:
            if (curptr[0] == '$' && curptr[1] == '#')
            {
                curptr += 2;
            }
            else
            {
                while (True)
                {
                    if (!is_identifier(*curptr, false))
                    {
                        // Anything other than TAG or TAG> causes the token to finish.
                        if (gettype(*curptr) != ST_TAG || (tagged && curptr[1] == '>'))
                            break;
                    }
                    curptr++;
                }
            }
            break;
        case ST_LIT:
            while (True)
            {
                Symbol_type newtype = gettype(*curptr);
                if (escapes && newtype == ST_ESC && *(curptr + 1))
                    curptr += 2;
                else
                {
                    if (newtype == ST_EOL || newtype == ST_EOF)
                    {
                        MCperror->add(PE_PARSE_BADLIT, *this);
                        return PS_ERROR;
                    }
                    else
                        if (newtype == ST_LIT)
                            break;
                    curptr++;
                }
            }
            break;
        case ST_OP:
            while (True)
            {
                Symbol_type newtype = gettype(*curptr);
                if (newtype != type)
                    break;
                curptr++;
            }
            break;
        case ST_NUM:
            while (True)
            {
                Symbol_type newtype = gettype(*curptr);
                if (newtype != type)
                {
                    char c = MCS_tolower(*curptr);
                    if (c == 'e')
                    {
                        if (*(curptr + 1) == '+' || *(curptr + 1) == '-')
                            curptr++;
                    }
                    else
                        if (c != 'x' && (c < 'a' || c > 'f'))
                            break;
                }
                curptr++;
            }
            break;
        default:
            curptr++;
            break;
	}
	if (type == ST_LIT && gettype(*curptr) == ST_LIT)
	{
		token.setlength(curptr - tokenptr - 1);
		curptr++;
	}
	else
		token.setlength(curptr - tokenptr);
	pos += curptr - startptr;
    
	m_type = type;
	return PS_NORMAL;
}

Parse_stat MCScriptPoint::nexttoken()
{
	Symbol_type type;
	Parse_stat ps = next(type);
	while (ps == PS_EOL)
	{
		skip_eol();
		ps = next(type);
	}
	return ps;
}
#endif

Parse_stat MCScriptPoint::skip_space()
{
	while (gettype(getcurrent()) == ST_SPC)
        advance();
    
	switch (gettype(getcurrent()))
	{
        case ST_COM:
            while (*curptr && gettype(getcurrent()) != ST_EOL)
                advance();
            if (!*curptr)
                return PS_EOF;
            return PS_EOL;
        case ST_MIN:
            if (gettype(getnext()) == ST_MIN)
            {
                while (*curptr && gettype(getcurrent()) != ST_EOL)
                    advance();
                if (*curptr)
                    return PS_EOL;
                else
                    return PS_EOF;
            }
            else
                return PS_NORMAL;
        case ST_OP:
            if (getcurrent() == '/' && getnext() == '/')
            {
                while (*curptr && gettype(getcurrent()) != ST_EOL)
                    advance();
                if (*curptr)
                    return PS_EOL;
                else
                    return PS_EOF;
            }
            else
                if (getcurrent() == '/' && getnext() == '*')
                {
                    const unichar_t *startptr = curptr;
                    const uint2 startline = line;
                    const uint2 startpos = pos;
                    do
                    {
                        advance();
                        if (getcurrent() == '*' && getnext() == '/')
                        {
                            advance(2);
                            return skip_space();
                        }
                        if (gettype(getcurrent()) == ST_EOL)
                        {
                            // MW-2011-06-23: [[ SERVER ]] If the line ends with CR LF
                            //   then eat the LF.
                            if (curptr[0] == 13 && curptr[1] == 10)
                                advance();
                            
                            line++;
                            pos = 1;
                        }
                        else
                            pos++;
                    }
                    while (*curptr);

                    setcurptr(startptr);
                    line = startline;
                    pos = startpos;
                    return PS_ERROR;
                }
                else
                    return PS_NORMAL;
        case ST_ESC:
            while (*curptr && gettype(getcurrent()) != ST_EOL)
                advance();
            if (!*curptr)
                return PS_EOF;
            // MW-2011-06-23: [[ SERVER ]] If the line ends with CR LF
            //   then eat the LF.
            if (getcurrent() == 13 && getnext() == 10)
                advance();
            advance();
            line++;
            pos = 1;
            return skip_space();
        case ST_EOF:
            return PS_EOF;
        case ST_SEMI:
        case ST_EOL:
            return PS_EOL;
        case ST_ERR:
            return PS_ERROR;
        case ST_TAG:
            // MW-2011-06-23: [[ SERVER ]] Make sure we return EOL when we
            //   encounter '?>' (?> is a command separator, essentially)
            if (in_tag && getnext() == '>')
                return PS_EOL;
            return PS_NORMAL;
        default:
            return PS_NORMAL;
	}
}

Parse_stat MCScriptPoint::skip_eol()
{
	Symbol_type type;
	Boolean lit = False;
	do
	{
		type = gettype(getcurrent());
		if (type == ST_EOF)
			return PS_EOF;
		if (type == ST_LIT)
			lit = !lit;
		// MW-2011-06-23: [[ SERVER ]] When we are asked to skip past a ?>
		//   we must eat the following newling - PHP-semantics.
		if (in_tag && type == ST_TAG && getnext() == '>')
		{
			in_tag = False;
			
			// Make sure we eat a subsequence newline
			if (getcodepointatindex(2) == 10)
			{
				// Take account of CR LF line ending
				if (getcodepointatindex(3) == 13)
					advance();
				
				pos = 1;
				advance(3);
				line += 1;
			}
			else
			{
				pos += 2;
				advance(2);
			}

			tokenptr = curptr;
			break;
		}
		advance();
	}
	while (type != ST_EOL && (type != ST_SEMI || lit));
	if (type == ST_EOL)
	{
		// MW-2011-06-23: [[ SERVER ]] If the line ends with CR LF
		//   then eat the LF.
		if (curptr[-1] == 13 && curptr[0] == 10)
			advance();
		line++;
		pos = 1;
		tokenptr = curptr;
	}
	return PS_NORMAL;
}

Parse_stat MCScriptPoint::backup()
{
	if (curptr == tokenptr)
	{
		pos -= curptr - backupptr;
		setcurptr(backupptr);
        
	}
	else
	{
		pos -= curptr - tokenptr;
		setcurptr(tokenptr);
		
		// MW-2011-06-23: [[ SERVER ]] Restore the backup 'in tag' state.
		if (tagged)
			in_tag = was_in_tag;
	}
    
	cleartoken();
	return PS_NORMAL;
}

Parse_stat MCScriptPoint::next(Symbol_type &type)
{
	Parse_stat stat;
	
	cleartoken();

	const unichar_t *startptr = curptr;
	
	// MW-2011-06-23: [[ SERVER ]] If we are in tagged mode and not in a tag, we
    //   check to see if there is a ST_DATA to produce. This involves advancing
    //   through the input buffer until we encounter a '<?rev'
	if (tagged && !in_tag)
	{
		// We were previously not in a tag, so we need to potentially skip '?>' and subsequent
		// newline. (Indeed, this will be case if we are not at the start)
		if ((line != 1 || pos != 1) && getcurrent() == '?' && getnext() == '>')
		{
			if (getcodepointatindex(2) == 10)
			{
				// Take account of CR LF line ending
				if (getcodepointatindex(3) == 13)
					advance();
				pos = 1;
				advance(3);
				line += 1;
			}
			else
			{
				pos += 2;
				advance(2);
			}
			startptr = curptr;
		}
		
		// Store the previous tag state for backup purposes.
		was_in_tag = False;
		
		// We will be inside a tag after this (or at the end!)
		in_tag = True;
		
		// Stores the length of the <? tag (if found)
		uint32_t t_tag_length = 0;

		// Loop until a NUL char, or we find '<?rev'
		bool t_in_comment;
		t_in_comment = false;
		while(*curptr != '\0')
		{
			if (!t_in_comment && (MCMemoryCompare(curptr, open_comment, sizeof(open_comment)) == 0))
			{
				pos += 4;
				advance(4);
				t_in_comment = true;
				continue;
			}
			else if (t_in_comment && (MCMemoryCompare(curptr, close_comment, sizeof(close_comment)) == 0))
			{
				pos += 3;
				advance(3);
				t_in_comment = false;
				continue;
			}
			else if (!t_in_comment && getcurrent() == '<' && getnext() == '?')
			{
				if (MCMemoryCompare(curptr + 2, rev_tag, sizeof(rev_tag)) == 0)
				{
					t_tag_length = 5;
					break;
				}
				else if (getcodepointatindex(2) == 'l' && getcodepointatindex(3) == 'c')
				{
					t_tag_length = 4;
					break;
				}
				else if (MCMemoryCompare(curptr + 2, livecode_tag, sizeof(livecode_tag)) == 0)
				{
					t_tag_length = 10;
					break;
				}
			}

			// Check for and advance past any newlines
			if (getcurrent() == 13)
			{
				if (getnext() == 10)
					advance();

				pos = 1, line += 1;
			}
			else if (getcurrent() == 10)
				pos = 1, line += 1;
			
			pos += 1;
			advance();
		}
		
		if (curptr != startptr)
		{
			// Type of symbol is ST_DATA
			type = ST_DATA;
			
			// Set the previous token-pointer
			backupptr = tokenptr;
			
			// Token starts at start (should be immediately after a ?> or beginning of file).
			tokenptr = startptr;
			
			// Set the token string appropriately.
			token.setstring((const char *)tokenptr);
			token.setlength(curptr - tokenptr);
			
			// If we aren't looking at the end of the data, then advance by 5 to skip '<?rev'.
			if (*curptr != '\0')
				advance(t_tag_length);
			
			// Return our token.
			return PS_NORMAL;
		}
		else
		{
			// There is no literal data, so just advance curptr and carry on.
			if (*curptr != '\0')
				advance(t_tag_length);
		}
	}
	else if (tagged)
		was_in_tag = True;
	
	if ((stat = skip_space()) != PS_NORMAL)
	{
		if (stat == PS_ERROR)
			MCperror->add(PE_PARSE_BADCHAR, *this);
		token.setstring((const char *)curptr);
		return stat;
	}
	
    if (is_identifier(getcurrent(), true))
        type = ST_ID;
    else
        type = gettype(getcurrent());
    
	if (type == ST_TAG)
	{
		if (tagged && getnext() == '>')
			return PS_EOL;
		else
			type = ST_ID;
	}
	if (type == ST_EOF)
	{
		token.setstring((const char *)curptr);
		return PS_EOF;
	}
	if (type == ST_EOL || type == ST_SEMI)
		return PS_EOL;
	if (curptr != tokenptr)
	{
		backupptr = tokenptr;
		tokenptr = curptr;
	}
	if (type == ST_LIT)
		advance();
	token.setstring((const char *)curptr);

	switch (type)
	{
	case ST_ID:
		if (getcurrent() == '$' && getnext() == '#')
		{
			advance(2);
		}
		else
		{
			while (True)
			{
				if (!is_identifier(getcurrent(), false))
				{
					// Anything other than TAG or TAG> causes the token to finish.
					if (gettype(getcurrent()) != ST_TAG || (tagged && getnext() == '>'))
						break;
				}
				advance();
			}
		}
		break;
	case ST_LIT:
		while (True)
		{
			Symbol_type newtype = gettype(getcurrent());
			if (escapes && newtype == ST_ESC && getnext())
                advance(2);
			else
			{
				if (newtype == ST_EOL || newtype == ST_EOF)
				{
					MCperror->add(PE_PARSE_BADLIT, *this);
					return PS_ERROR;
				}
				else
					if (newtype == ST_LIT)
						break;
				advance();
			}
		}
		break;
	case ST_OP:
		while (True)
		{
			Symbol_type newtype = gettype(getcurrent());
			if (newtype != type)
				break;
			advance();
		}
		break;
	case ST_NUM:
		while (True)
		{
			Symbol_type newtype = gettype(getcurrent());
			if (newtype != type)
			{
                if (getcurrent() > 127)
                    break;
                
				char c = MCS_tolower(*curptr);
				if (c == 'e')
				{
					if (getnext() == '+' || getnext() == '-')
						advance();
				}
				else
					if (c != 'x' && (c < 'a' || c > 'f'))
						break;
			}
			advance();
		}
		break;
	default:
		advance();
		break;
	}
	if (type == ST_LIT && gettype(getcurrent()) == ST_LIT)
	{
		token.setlength(curptr - tokenptr - 1);
		advance();
	}
	else
		token.setlength(curptr - tokenptr);
	pos += curptr - startptr;

	m_type = type;
	return PS_NORMAL;
}

Parse_stat MCScriptPoint::nexttoken()
{
	Symbol_type type;
	Parse_stat ps = next(type);
	while (ps == PS_EOL)
	{
		skip_eol();
		ps = next(type);
	}
	return ps;
}

Parse_stat MCScriptPoint::lookup(Script_point t, const LT *&dlt)
{
	if (m_type == ST_LIT)
		return PS_NO_MATCH;
	
	if (token.getlength())
	{
		const LT *table = table_pointers[t];
		uint2 high = table_sizes[t];
		uint2 low = 0;
		int4 cond;
        MCAutoStringRefAsCString t_token;
        t_token . Lock(gettoken_stringref());
        const char *token_cstring = *t_token;
        
		while (low < high)
		{
			// Both the table and the token are encoded in UTF-8
			uint2 mid = low + ((high - low) >> 1);
			cond = MCU_strncasecmp(token_cstring, table[mid].token, token.getlength());
			if (cond == 0)
				cond -= table[mid].token[token.getlength()];
			if (cond < 0)
				high = mid;
			else
				if (cond > 0)
					low = mid + 1;
				else
				{
					dlt = &table[mid];
					return PS_NORMAL;
				}
		}
	}
	return PS_NO_MATCH;
}

bool MCScriptPoint::lookupconstantintable(int& r_position)
{
    int high = constant_table_size;
    int low = 0;
    int cond;
    
    MCAutoStringRefAsCString t_token;
    t_token . Lock(gettoken_stringref());
    const char *token_cstring = *t_token;
    while (low < high)
    {
        int mid = low + ((high - low) >> 1);
        cond = MCU_strncasecmp(token_cstring, constant_table[mid].token, token.getlength());
        if (cond == 0)
        {
            cond -= constant_table[mid].token[token.getlength()];
        }
        
        if (cond < 0)
        {
            high = mid;
        }
        else
        {
            if (cond > 0)
            {
                low = mid + 1;
            }
            else
            {
                r_position = mid;
                return true;
            }
        }
    }
    return false;
}

bool MCScriptPoint::constantnameconvertstoconstantvalue()
{
    int t_position;
    if (!lookupconstantintable(t_position))
        return false;
    
    switch (constant_table[t_position].type)
    {
        case kCValueTypeString:
            return MCStringIsEqualToCString(gettoken_stringref(), constant_table[t_position].string, kMCCompareExact);
        case kCValueTypeInfinity:
        case kCValueTypeTrue:
        case kCValueTypeFalse:
            return MCStringIsEqualToCString(gettoken_stringref(), constant_table[t_position].token, kMCCompareExact);
        case kCValueTypeReal:
        case kCValueTypeInteger:
        case kCValueTypeEmpty:
        case kCValueTypeNull:
            return false;
        default:
            MCUnreachableReturn(false);
    }
}

Parse_stat MCScriptPoint::lookupconstant(MCExpression **dest)
{
	if (m_type == ST_LIT)
		return PS_NO_MATCH;
	
	if (gethandler() != NULL
	        && gethandler()->findconstant(gettoken_nameref(), dest) == PS_NORMAL)
		return PS_NORMAL;

    int t_position;
    if (!lookupconstantintable(t_position))
        return PS_NO_MATCH;
    
    MCValueRef t_constant_value = nullptr;
    switch (constant_table[t_position].type)
    {
        case kCValueTypeReal:
            /* UNCHECKED */ MCNumberCreateWithReal(constant_table[t_position].real,
                                                   reinterpret_cast<MCNumberRef&>(t_constant_value));
            break;
        case kCValueTypeInteger:
            /* UNCHECKED */ MCNumberCreateWithInteger(constant_table[t_position].integer,
                                                      reinterpret_cast<MCNumberRef&>(t_constant_value));
            break;
        case kCValueTypeString:
            /* UNCHECKED */ MCNameCreateWithNativeChars(reinterpret_cast<const char_t *>(constant_table[t_position].string),
                                                        strlen(constant_table[t_position].string),
                                                        reinterpret_cast<MCNameRef&>(t_constant_value));
            break;
        case kCValueTypeNull:
            // Create a nameref that contains an explicit nul character
            /* UNCHECKED */ MCNameCreateWithNativeChars(reinterpret_cast<const char_t *>("\0"),
                                                        1,
                                                        reinterpret_cast<MCNameRef&>(t_constant_value));
            break;
        case kCValueTypeEmpty:
            t_constant_value = MCValueRetain(kMCEmptyString);
            break;
        case kCValueTypeTrue:
            t_constant_value = MCValueRetain(kMCTrue);
            break;
        case kCValueTypeFalse:
            t_constant_value = MCValueRetain(kMCFalse);
            break;
        case kCValueTypeInfinity:
            /* UNCHECKED */ MCNumberCreateWithReal(MCinfinity,
                                                   reinterpret_cast<MCNumberRef&>(t_constant_value));
            break;
        default:
            MCUnreachableReturn(PS_NO_MATCH);
    }
    MCAutoValueRef t_unique_value;
    /* UNCHECKED */ MCValueInterAndRelease(t_constant_value, &t_unique_value);
    *dest = new (nothrow) MCLiteral(*t_unique_value);
    
    return PS_NORMAL;
}

Parse_stat MCScriptPoint::skip_token(Script_point table,
                                     Token_type ttype, uint2 which)
{
	Parse_stat stat;
	Symbol_type type;
	const LT *te;

	if ((stat = next(type)) != PS_NORMAL)
		return stat;
	switch (type)
	{
	case ST_OP:
	case ST_MIN:
	case ST_LP:
	case ST_RP:
	case ST_ID:
		if (lookup(table, te) != PS_NORMAL
		        || te->type != ttype || (which != 0 && te->which != which))
		{
			backup();
			return PS_NO_MATCH;
		}
		break;
	default:
		backup();
		return PS_NO_MATCH;
	}
	return PS_NORMAL;
}

MCExpression *MCScriptPoint::insertfactor(MCExpression *nfact,
        MCExpression *&cfact,
        MCExpression **top)
{
	if (cfact == NULL)
		cfact = *top = nfact;
	else
	{
		nfact->setroot(cfact);
		cfact->setright(nfact);
	}
	return nfact;
}

MCExpression *MCScriptPoint::insertbinop(MCExpression *nfact,
        MCExpression *&cfact,
        MCExpression **top)
{
	if (cfact == NULL)
	{
		delete nfact;
		MCperror->add
		(PE_EXPRESSION_NOLFACT, *this);
		return NULL;
	}
	if (nfact->getrank() > cfact->getrank())
	{
		nfact->setroot(cfact);
		nfact->setleft(cfact->getright());
		cfact->setright(nfact);
	}
	else
	{
		while (cfact->getroot() != NULL
		        && nfact->getrank() <= cfact->getroot()->getrank())
			cfact = cfact->getroot();
		if (cfact->getrank() == FR_POW && nfact->getrank() == FR_POW)
		{
			nfact->setleft(cfact->getright());
			cfact->setright(nfact);
			nfact->setroot(cfact);
		}
		else
		{
			nfact->setleft(cfact);
			nfact->setroot(cfact->getroot());
			if (cfact->getroot() == NULL)
				*top = nfact;
			else
				cfact->getroot()->setright(nfact);
		}
	}
	cfact = nfact;
	return nfact;
}

Parse_stat MCScriptPoint::parseexp(Boolean single, Boolean items,
                                   MCExpression **top)
{
	Symbol_type type;
	const LT *te;
	MCExpression *newfact = NULL;
	MCExpression *curfact = NULL;
	Boolean doingthe = False;
	Boolean needfact = True;
	uint2 depth = 0;
	MCScriptPoint thesp(*this);
	Parse_stat pstat;
	Boolean litems = items;

	while (True)
	{
		if (next(type) != PS_NORMAL)
		{
			if (needfact)
			{
				MCperror->add(PE_EXPRESSION_NOFACT, *this);
				return PS_ERROR;
			}
			else if (depth == 0)
					return PS_NORMAL;
				else
				{
					MCperror->add(PE_EXPRESSION_NORPAR, *this);
					return PS_ERROR;
				}
		}
		if (!needfact && type != ST_OP && type != ST_MIN && type != ST_RP
		        && !(type == ST_SEP && litems)
		        && !(type == ST_ID && lookup(SP_FACTOR, te) == PS_NORMAL
		             && (te->type == TT_UNOP || te->type == TT_BINOP)))
		{
			if (depth == 0)
			{
				backup();
				return PS_NORMAL;
			}
			else
			{
				MCperror->add(PE_EXPRESSION_WANTRPAR, *this);
				return PS_ERROR;
			}
		}
		switch (type)
		{
		case ST_NUM:
        {
            // It is absolutely *vital* that we use the same method here for
            // parsing numbers as MCExecContext::ConvertToNumber. If we don't,
            // very strange inconsistencies occur depending on whether the
            // number is used in an arithmetic context or not (for an example,
            // see bug #17338 in Bugzilla).
            real8 nvalue;
            if (!MCTypeConvertStringToReal(gettoken_stringref(), nvalue, false))
			{
				MCperror->add(PE_EXPRESSION_NOTLITERAL, *this);
				return PS_ERROR;
            }

            MCStringSetNumericValue(MCNameGetString(gettoken_nameref()), nvalue);

			newfact = insertfactor(new MCLiteral(gettoken_nameref()), curfact, top);
			newfact->parse(*this, doingthe);
			needfact = False;
        }
			break;
		case ST_LIT:
			newfact = insertfactor(new MCLiteral(gettoken_nameref()), curfact, top);
			newfact->parse(*this, doingthe);
			needfact = False;
			break;
		case ST_SEP:
			if (litems)
			{
				if (needfact)
				{
					MCperror->add(PE_EXPRESSION_DOUBLEBINOP, *this);
					return PS_ERROR;
				}
				if ((newfact = insertbinop(new MCItem, curfact, top)) == NULL)
					return PS_ERROR;
				newfact->parse(*this, doingthe);
				needfact = True;
			}
			else
			{
				if (needfact)
					newfact = insertfactor(new MCLiteral(kMCEmptyName), curfact, top);
				backup();
				return PS_NORMAL;
			}
			break;
		default:
			if (lookup(SP_FACTOR, te) == PS_NORMAL)
			{
				extern bool lookup_property_override(const LT&p_lt, Properties &r_property);
				Properties t_property;
				
				Token_type t_type;
				t_type = te->type;
				if (doingthe && lookup_property_override(*te, t_property))
				{
					t_type = TT_PROPERTY;
				}
				switch (t_type)
				{
				case TT_THE:
					doingthe = True;
					break;
				case TT_RPAREN:
					if (depth == 0)
					{
						if (needfact)
						{
							MCperror->add(PE_EXPRESSION_NOFACT, *this);
							return PS_ERROR;
						}
						backup();
						return PS_NORMAL;
					}
					if (curfact->getright() == NULL)
					{
						MCperror->add(PE_EXPRESSION_NOFACT, *this);
						return PS_ERROR;
					}
					while (curfact->getroot() != NULL
					        && curfact->getrank() != FR_GROUPING)
						curfact = curfact->getroot();
					curfact->setrank(FR_VALUE);
					if (--depth == 0)
						litems = items;
					if (curfact->getroot() != NULL)
						curfact = curfact->getroot();
					break;
				case TT_LPAREN:
					depth++;
					litems = True;
				case TT_UNOP:
					newfact = MCN_new_operator(te->which);
					if ((pstat = newfact->parse(*this, doingthe)) == PS_ERROR)
					{
						delete newfact;
						return PS_ERROR;
					}
					needfact = pstat != PS_BREAK;
					if (curfact == NULL)
						*top = newfact;
					else
						if (curfact->getright() == NULL)
						{
							curfact->setright(newfact);
							newfact->setroot(curfact);
						}
						else
						{
							delete newfact;
							MCperror->add(PE_EXPRESSION_NOBINOP, *this);
							return PS_ERROR;
						}
					curfact = newfact;
					break;
				case TT_BIN_OR_UNOP:
					if (curfact == NULL
					    || (curfact->getrank() == FR_GROUPING
					        && curfact->getright() == NULL)
					    || (curfact->getright() == NULL && curfact->getleft() != NULL))
					{
						newfact = MCN_new_operator(te->which);
						newfact->parse(*this, doingthe);
						newfact->setrank(FR_UNARY);
						if (curfact == NULL)
							*top = newfact;
						else
							if (curfact->getright() == NULL)
							{
								curfact->setright(newfact);
								newfact->setroot(curfact);
							}
						curfact = newfact;
						needfact = True;
						break;
					}
				case TT_BINOP:
					if (needfact)
					{
						MCperror->add(PE_EXPRESSION_DOUBLEBINOP, *this);
						return PS_ERROR;
					}
					newfact = MCN_new_operator(te->which);
					if ((pstat = newfact->parse(*this, doingthe)) == PS_ERROR)
					{
						delete newfact;
						return PS_ERROR;
					}
					needfact = pstat != PS_BREAK;
					if (insertbinop(newfact, curfact, top) == NULL)
						return PS_ERROR;
					if (!needfact)
					{ // switch sides for validation operators
						newfact->setright(newfact->getleft());
						newfact->setleft(NULL);
					}
					break;
				case TT_CHUNK:
					newfact = new (nothrow) MCChunk(False);
					backup();
					if (newfact->parse(*this, doingthe) != PS_NORMAL)
					{
						delete newfact;
						MCperror->add(PE_EXPRESSION_BADCHUNK, *this);
						return PS_ERROR;
					}
					insertfactor(newfact, curfact, top);
					doingthe = needfact = False;
					break;
				case TT_FUNCTION:
					newfact = MCN_new_function(te->which);
                    // SN-2014-11-25: [[ Bug 14088 ]] MCN_new_function returns NULL in case the function doesn't exist
                    if (newfact == NULL)
                    {
                        MCperror->add(PE_EXPRESSION_BADFUNCTION, *this);
                        return PS_ERROR;
                    }
					thesp = *this;
					thesp.backup();
					if (newfact->parse(*this, doingthe) != PS_NORMAL)
					{
						delete newfact;
						if (doingthe || !MCperror->isempty())
						{
							MCperror->add(PE_EXPRESSION_BADFUNCTION, *this);
							return PS_ERROR;
						}
						*this = thesp;
						MCVarref *newvar;
						// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
						//   execution outwith a handler.
						if (gethandler() != NULL && findvar(gettoken_nameref(), &newvar) == PS_NORMAL)
						{
							newvar->parsearray(*this);
							newfact = newvar;
						}
						else
							if (MCexplicitvariables)
							{
								MCperror->add(PE_EXPRESSION_NOTLITERAL, *this);
								return PS_ERROR;
							}
							else
								newfact = new (nothrow) MCLiteral(gettoken_nameref());
						newfact->parse(*this, doingthe);
					}
					insertfactor(newfact, curfact, top);
					doingthe = needfact = False;
					break;
				case TT_PROPERTY:
					thesp = *this;
					backup();
					newfact = new (nothrow) MCProperty;
					MCerrorlock++;
					if (newfact->parse(*this, doingthe) != PS_NORMAL)
					{
						delete newfact;
						*this = thesp;
						if (doingthe || MCexplicitvariables)
						{
							MCerrorlock--;
							MCperror->add(PE_PROPERTY_NOTOF, *this);
							return PS_ERROR;
						}
						newfact = new (nothrow) MCLiteral(gettoken_nameref());
					}
					MCerrorlock--;
					insertfactor(newfact, curfact, top);
					doingthe = needfact = False;
					break;
				default:
					backup();
					MCperror->add(PE_EXPRESSION_NOTFACT, *this);
					return PS_ERROR;
				}
			}
			else
			{
				if (doingthe)
				{
					backup();
					thesp = *this;
					newfact = new (nothrow) MCProperty;
					MCerrorlock++;
					if (newfact->parse(*this, doingthe) != PS_NORMAL)
					{
						delete newfact;
						*this = thesp;
						MCerrorlock--;

                        if (MCStringIsEmpty(gettoken_stringref()))
						{
							MCperror->add(PE_EXPRESSION_NOTFACT, *this);
							return PS_ERROR;
						}

						newfact = new (nothrow) MCFuncref(gettoken_nameref());
						newfact->parse(*this, doingthe);
					}
					else
						MCerrorlock--;
				}
				else
				{
					if (type != ST_ID) 
					{
						MCperror->add(PE_EXPRESSION_NOTFACT, *this);
						return PS_ERROR;
					}

					if (lookupconstant(&newfact) != PS_NORMAL)
					{
						MCVarref *newvar;
						newfact = NULL;

						MCNewAutoNameRef t_name = gettoken_nameref();

						if (next(type) == PS_NORMAL)
							backup();
						else
							type = ST_ERR;
						// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
						//   execution outwith a handler.
						if (type != ST_LP && findvar(*t_name, &newvar) == PS_NORMAL)
						{
							newvar->parsearray(*this);
							newfact = newvar;
						}
						else if (type == ST_LB && !MCexplicitvariables)
							{
								// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
								//   execution outwith a handler.
							if (findnewvar(*t_name, kMCEmptyName, &newvar) != PS_NORMAL)
								{
									MCperror->add(PE_EXPRESSION_NOTFACT, *this);
									return PS_ERROR;
								}
								newvar->parsearray(*this);
								newfact = newvar;
							}
						else if (type == ST_LP)
								newfact = new (nothrow) MCFuncref(*t_name);
						if (newfact == NULL)
						{
							if (MCexplicitvariables)
							{
								MCperror->add(PE_EXPRESSION_NOTLITERAL, *this);
								return PS_ERROR;
							}
							else
							{
								// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
								//   execution outwith a handler.
								if (finduqlvar(*t_name, &newvar) != PS_NORMAL)
								{
									MCperror->add(PE_EXPRESSION_NOTFACT, *this);
									return PS_ERROR;
								}
								newvar->parsearray(*this);
								newfact = newvar;
							}
						}
					}
					// MW-2007-08-30: [[ Bug 2633 ]] Things such as sum2(1, 2+) don't flag a parse error this is
					//   because this parse method could fail - we now produce an error in this case.
					if (newfact->parse(*this, doingthe) != PS_NORMAL)
					{
						delete newfact;
						MCperror->add(PE_EXPRESSION_NOTFACT, *this);
						return PS_ERROR;
					}
				}
				insertfactor(newfact, curfact, top);
				doingthe = needfact = False;
			}
			break;
		}
		if (single && !needfact && depth == 0)
			break;
	}
	return PS_NORMAL;
}

Parse_stat MCScriptPoint::findvar(MCNameRef p_name, MCVarref** r_var)
{
	if (curhandler != NULL)
		return curhandler -> findvar(p_name, r_var);
	
	// MW-2011-08-23: [[ UQL ]] We are only searching in hlist scope, so we
    //   do want to search UQLs.
	if (curhlist != NULL)
		return curhlist -> findvar(p_name, false, r_var);

	return PS_ERROR;
}

Parse_stat MCScriptPoint::findnewvar(MCNameRef p_name, MCNameRef p_init, MCVarref** r_var)
{
	if (findvar(p_name, r_var) == PS_NORMAL)
		return PS_NORMAL;

	if (curhandler != NULL)
		return curhandler -> newvar(p_name, p_init, r_var);

	if (curhlist != NULL)
		return curhlist -> newvar(p_name, p_init, r_var, True);

	return PS_ERROR;
}
	
Parse_stat MCScriptPoint::finduqlvar(MCNameRef p_name, MCVarref** r_var)
{
	if (findvar(p_name, r_var) == PS_NORMAL)
		return PS_NORMAL;

	if (curhandler != NULL)
		return curhandler -> newvar(p_name, nil, r_var);

	if (curhlist != NULL)
		return curhlist -> newvar(p_name, nil, r_var, True);

	return PS_ERROR;
}
