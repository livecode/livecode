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
// Script class declarations
//
#ifndef	SCRIPTPOINT_H
#define	SCRIPTPOINT_H

typedef struct
{
	const char *token;
	Token_type type;
	uint2 which;
}
LT;

typedef enum {
    kCValueTypeReal,
    kCValueTypeInteger,
    kCValueTypeString,
    kCValueTypeNull,
    kCValueTypeEmpty,
    kCValueTypeTrue,
    kCValueTypeFalse,
    kCValueTypeInfinity
} CvalueType;

struct Cvalue
{
    /* Each constructor is labelled constexpr so that it should be
     * evaluated at compile time. Overloading is used to infer the
     * type to use, based on the second argument. */
    
    constexpr Cvalue(const char *p_token, integer_t p_integer)
        : token(p_token),
          type(kCValueTypeInteger),
          integer(p_integer)
    {
    }
    
    constexpr Cvalue(const char *p_token, double p_real)
        : token(p_token),
          type(kCValueTypeReal),
          real(p_real)
    {
    }
    
    constexpr Cvalue(const char *p_token, const char *p_string)
        : token(p_token),
          type(kCValueTypeString),
          string(p_string)
    {
    }
    
    /* As CvalueType is an explicit enum type distinct from the other
     * value types, we can use that to construct a Cvalue with explicit
     * type. Note: As this is a constexpr constructor *all* fields must be
     * initialized, we choose integer = 0 for the union in this case. */
    
    constexpr Cvalue(const char *p_token, CvalueType p_type)
        : token(p_token),
          type(p_type),
          integer(0)
    {
    }
    
    const char *token;
    CvalueType type;
    union
    {
        double real;
        integer_t integer;
        const char *string;
    };
};

class MCScriptPoint
{
    MCDataRef utf16_script;
    uint32_t length;

	MCObject *curobj;
	MCHandlerlist *curhlist;
	MCHandler *curhandler;
	const unichar_t *curptr;
	const unichar_t *tokenptr;
	const unichar_t *backupptr;
    const unichar_t *endptr;
	MCString token;
	MCNameRef token_nameref;
	uint2 line;
	uint2 pos;
	Boolean escapes;
	Symbol_type m_type;
	
    codepoint_t codepoint;
    uint1 curlength;
    
	// MW-2011-06-23: If this is true, then we parse the script in 'tag' mode.
	Boolean tagged;
	// MW-2011-06-23: This is true if we are currently consuming tokens inside
	//   a <?rev tag.
	Boolean in_tag;
	// MW-2011-06-23: This is true if we backed up over a tag change point.
	Boolean was_in_tag;

public:
	MCScriptPoint(MCScriptPoint &sp);
    MCScriptPoint(MCObject *, MCHandlerlist *, MCDataRef utf8_script);
    MCScriptPoint(MCObject *, MCHandlerlist *, MCStringRef utf8_script);
    MCScriptPoint(MCExecContext &ctxt);
    MCScriptPoint(MCExecContext &ctxt, MCStringRef p_string);
	MCScriptPoint(MCStringRef p_string);

	~MCScriptPoint();
	MCScriptPoint& operator=(const MCScriptPoint& sp);
	
	void allowescapes(Boolean which)
	{
		escapes = which;
	}
	
	// MW-2009-03-03: If allowtags is true then we use PHP style tagged parsing,
	//   producing a ST_DATA token in between valid tags.
	void allowtags(Boolean which)
	{
		tagged = which;
	}

	uint2 getline()
	{
		return line;
	}
	void setline(uint2 l)
	{
		line = l;
	}
	ptrdiff_t getpos()
	{
		return pos > (curptr - backupptr)
		       ?  pos - (curptr - backupptr) : 1;
	}
	bool token_is_cstring(const char *p_cstring);

	MCNameRef gettoken_nameref(void);
	MCStringRef gettoken_stringref(void);

	MCObject *getobj()
	{
		return curobj;
	}
	void sethandler(MCHandler *h)
	{
		curhandler = h;
	}
	MCHandler *gethandler()
	{
		return curhandler;
	}
	MCHandlerlist *gethlist()
	{
		return curhlist;
	}

	const unichar_t *getcurptr(void)
	{
		return curptr;
	}
    
    uindex_t getindex(void)
    {
        // Warning: explicitly truncated to a uindex_t
        // This imposes a limit of 4GB on scripts
        size_t index = (const unichar_t *)token . getstring() + length - endptr;
        MCAssert(uindex_t(index) == index);
        
        return uindex_t(index);
    }
    
    bool is_eol()
    {
        Symbol_type t_dummy;
        Parse_stat t_stat = next(t_dummy);
        backup();
        return t_stat == PS_EOL;
    }

	Parse_stat skip_space();
	Parse_stat skip_eol();
	Parse_stat backup();
	Parse_stat next(Symbol_type &type);
	Parse_stat nexttoken();
	void cleartoken(void);
	Parse_stat lookup(Script_point, const LT *&);
    bool constantnameconvertstoconstantvalue();
	Parse_stat lookupconstant(MCExpression **);
	Parse_stat skip_token(Script_point, Token_type, uint2 n = 0);
	MCExpression *insertfactor(MCExpression *nfact, MCExpression *&cfact,
	                           MCExpression **top);
	MCExpression *insertbinop(MCExpression *nfact, MCExpression *&cfact,
	                          MCExpression **top);
	Parse_stat parseexp(Boolean single, Boolean items, MCExpression **);
	
	// Search for an existing variable in scope, returning an error if it
	// doesn't exist.
	Parse_stat findvar(MCNameRef name, MCVarref** r_var);

	// Search for an existing variable in scope, creating one if it doesn't
	// exist.
	Parse_stat findnewvar(MCNameRef name, MCNameRef init, MCVarref** r_var);

	// Search for an existing variable in scope, creating a uql-var if it
	// doesn't exist. A uql-var starts off with the same content as its
	// name, but as soon as its used as a container becomes empty.
	Parse_stat finduqlvar(MCNameRef name, MCVarref** r_var);
    
    Symbol_type gettype(codepoint_t p_codepoint);
    
    // A codepoint can be an initial character of an identifier if it
    // - has type ST_ID
    // - is a Unicode letter
    // It can be part of an identifier if it can be an initial character, or it
    // - has type ST_NUM
    // - is a Unicode digit
    // - is a Unicode combining mark
    // - is a Unicode connector punctuation mark
    bool is_identifier(codepoint_t p_codepoint, bool p_initial);
    
    // Increment the index
    void advance(uindex_t number = 1);
    
    codepoint_t getcurrent();
    codepoint_t getnext();
    codepoint_t getcodepointatindex(uindex_t index);
    
    void setcurptr(const unichar_t *ptr);
    
private:
    bool lookupconstantintable(int& r_position);
};
#endif

