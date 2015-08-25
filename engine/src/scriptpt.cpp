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
#include "execpt.h"
#include "hndlrlst.h"
#include "handler.h"
#include "cmds.h"
#include "visual.h"
#include "keywords.h"
#include "property.h"
#include "chunk.h"
#include "funcs.h"
#include "operator.h"
#include "constant.h"
#include "literal.h"
#include "newobj.h"
#include "mcerror.h"
#include "util.h"

#include "globals.h"

#define LOWERED_PAD 64

extern uint8_t type_table[];
extern Cvalue constant_table[];
extern const uint4 constant_table_size;
extern LT *table_pointers[];
extern uint2 table_sizes[];
extern LT command_table[];
extern const uint4 command_table_size;
extern LT factor_table[];
extern const uint4 factor_table_size;

MCScriptPoint::MCScriptPoint(MCObject *o, MCHandlerlist *hl, const char *s)
{
	script = NULL;
	curobj = o;
	curhlist = hl;
	curhandler = NULL;
	curptr = tokenptr = backupptr = (const uint1 *)s;
	lowered = NULL;
	loweredsize = 0;
	line = pos = 1;
	escapes = False;
	tagged = False;
	in_tag = False;
	was_in_tag = False;
	token_nameref = nil;
}

MCScriptPoint::MCScriptPoint(MCScriptPoint &sp)
{
	script = NULL;
	curobj = sp.curobj;
	curhlist = sp.curhlist;
	curhandler = sp.curhandler;
	curptr = sp.curptr;
	tokenptr = sp.tokenptr;
	backupptr = sp.backupptr;
	token = sp.token;
	lowered = NULL;
	loweredsize = 0;
	line = sp.line;
	pos = sp.pos;
	escapes = sp.escapes;
	tagged = sp.tagged;
	in_tag = sp.in_tag;
	was_in_tag = sp.was_in_tag;
	token_nameref = nil;
}

MCScriptPoint::MCScriptPoint(MCExecPoint &ep)
{
	script = ep.getsvalue().clone();
	curobj = ep.getobj();
	curhlist = ep.gethlist();
	curhandler = ep.gethandler();
	curptr = tokenptr = backupptr = (uint1 *)script;
	lowered = NULL;
	loweredsize = 0;
	line = pos = 0;
	escapes = False;
	tagged = False;
	in_tag = False;
	was_in_tag = False;
	token_nameref = nil;
}

MCScriptPoint::MCScriptPoint(const MCString &s)
{
	script = s.clone();
	curobj = NULL;
	curhlist = NULL;
	curhandler = NULL;
	curptr = tokenptr = backupptr = (uint1 *)script;
	lowered = NULL;
	loweredsize = 0;
	line = pos = 0;
	escapes = False;
	tagged = False;
	in_tag = False;
	was_in_tag = False;
	token_nameref = nil;
}

MCScriptPoint::~MCScriptPoint()
{
	MCNameDelete(token_nameref);
	delete script;
	delete lowered;
}

void MCScriptPoint::cleartoken(void)
{
	token . setlength(0);
	MCNameDelete(token_nameref);
	token_nameref = nil;
}

MCNameRef MCScriptPoint::gettoken_nameref(void)
{
	if (token_nameref == nil)
		/* UNCHECKED */ MCNameCreateWithOldString(token, token_nameref);
	return token_nameref;
}

Parse_stat MCScriptPoint::skip_space()
{
	while (type_table[*curptr] == ST_SPC)
		curptr++;
	switch (type_table[*curptr])
	{
	case ST_COM:
		while (*curptr && type_table[*curptr] != ST_EOL)
			curptr++;
		if (!*curptr)
			return PS_EOF;
		return PS_EOL;
	case ST_MIN:
		if (type_table[*(curptr + 1)] == ST_MIN)
		{
			while (*curptr && type_table[*curptr] != ST_EOL)
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
			while (*curptr && type_table[*curptr] != ST_EOL)
				curptr++;
			if (*curptr)
				return PS_EOL;
			else
				return PS_EOF;
		}
		else
			if (*curptr == '/' && *(curptr + 1) == '*')
			{
				const uint1 *startptr = curptr;
				const uint2 startline = line;
				const uint2 startpos = pos;
				do
				{
					curptr++;
					if (*curptr == '*' && *(curptr + 1) == '/')
						break;
					if (type_table[*curptr] == ST_EOL)
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
		while (*curptr && type_table[*curptr] != ST_EOL)
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
		type = type_table[*curptr];
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

	const uint1 *startptr = curptr;
	
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
		uint32_t t_tag_length;

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
	
	type = type_table[*curptr];
	
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
	if (lowered == NULL)
	{
		lowered = new char[LOWERED_PAD];
		loweredsize = LOWERED_PAD;
	}
	char *lptr = lowered;
	switch (type)
	{
	case ST_ID:
		if (curptr[0] == '$' && curptr[1] == '#')
		{
			curptr += 2;
			*lptr++ = '$';
			*lptr++ = '#';
			*lptr = '\0';
		}
		else
		{
			while (True)
			{
				Symbol_type newtype = type_table[*curptr];
				// MW-2010-09-08: [[Bug 8946]] Crash caused by appending a NUL byte when there is no space for it.
				if (lptr - lowered == loweredsize)
				{
					MCU_realloc((char **)&lowered, loweredsize, loweredsize + LOWERED_PAD, sizeof(uint1));
					lptr = lowered + loweredsize;
					loweredsize += LOWERED_PAD;
				}
				if (newtype != ST_ID && newtype != ST_NUM)
				{
					// Anything other than TAG or TAG> causes the token to finish.
					if (newtype != ST_TAG || (tagged && curptr[1] == '>'))
					{
						*lptr = '\0';
						break;
					}
				}
				*lptr++ = MCS_tolower(*curptr++);
			}
		}
		break;
	case ST_LIT:
		while (True)
		{
			Symbol_type newtype = type_table[*curptr];
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
			Symbol_type newtype = type_table[*curptr];
			if (newtype != type)
			{
				*lptr = '\0';
				break;
			}
			if (lptr - lowered == loweredsize)
			{
				MCU_realloc((char **)&lowered, loweredsize,
				            loweredsize + LOWERED_PAD, sizeof(uint1));
				lptr = lowered + loweredsize;
				loweredsize += LOWERED_PAD;
			}
			*lptr++ = *curptr++;
		}
		break;
	case ST_NUM:
		while (True)
		{
			Symbol_type newtype = type_table[*curptr];
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
		*lptr++ = *curptr++;
		*lptr = '\0';
		break;
	}
	if (type == ST_LIT && type_table[*curptr] == ST_LIT)
	{
		token.setlength(curptr - tokenptr - 1);
		curptr++;
	}
	else
		token.setlength(curptr - tokenptr);
	pos += curptr - startptr;
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
	if (token.getlength())
	{
		const LT *table = table_pointers[t];
		uint2 high = table_sizes[t];
		uint2 low = 0;
		int4 cond;

		while (low < high)
		{
			uint2 mid = low + ((high - low) >> 1);
			if ((cond = strcmp(lowered, table[mid].token)) < 0)
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

Parse_stat MCScriptPoint::lookupconstant(MCExpression **dest)
{
	if (gethandler() != NULL
	        && gethandler()->findconstant(gettoken_nameref(), dest) == PS_NORMAL)
		return PS_NORMAL;
	uint2 high = constant_table_size;
	uint2 low = 0;
	int4 cond;
	while (low < high)
	{
		uint2 mid = low + ((high - low) >> 1);
		if ((cond = strcmp(lowered, constant_table[mid].token)) < 0)
			high = mid;
		else
			if (cond > 0)
				low = mid + 1;
			else
			{
				if (strequal(lowered, "null"))
				{
					MCString s("", 1);
					*dest = new MCConstant(s, BAD_NUMERIC);
				}
				else
					*dest = new MCConstant(constant_table[mid].svalue,
					                       constant_table[mid].nvalue);
				return PS_NORMAL;
			}
	}
	return PS_NO_MATCH;
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
			if (needfact)
			{
				MCperror->add(PE_EXPRESSION_NOFACT, *this);
				return PS_ERROR;
			}
			else
				if (depth == 0)
					return PS_NORMAL;
				else
				{
					MCperror->add(PE_EXPRESSION_NORPAR, *this);
					return PS_ERROR;
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
			real8 nvalue;
			if (!MCU_stor8(gettoken(), nvalue))
			{
				MCperror->add(PE_EXPRESSION_NOTLITERAL, *this);
				return PS_ERROR;
			}
			newfact = insertfactor(new MCLiteralNumber(gettoken_nameref(), nvalue), curfact, top);
			newfact->parse(*this, doingthe);
			needfact = False;
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
				switch (te->type)
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
					        || curfact->getrank() == FR_GROUPING
					        && curfact->getright() == NULL
					        || curfact->getright() == NULL && curfact->getleft() != NULL)
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
					newfact = new MCChunk(False);
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
					thesp = *this;
					thesp.backup();
					if (newfact->parse(*this, doingthe) != PS_NORMAL)
					{
						delete newfact;
						if (doingthe)
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
								newfact = new MCLiteral(gettoken_nameref());
						newfact->parse(*this, doingthe);
					}
					insertfactor(newfact, curfact, top);
					doingthe = needfact = False;
					break;
				case TT_PROPERTY:
					thesp = *this;
					backup();
					newfact = new MCProperty;
					MCerrorlock++;
					if (newfact->parse(*this, doingthe) != PS_NORMAL)
					{
						delete newfact;
						*this = thesp;
						if (MCexplicitvariables)
						{
							MCerrorlock--;
							MCperror->add(PE_EXPRESSION_NOTLITERAL, *this);
							return PS_ERROR;
						}
						newfact = new MCLiteral(gettoken_nameref());
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
					newfact = new MCProperty;
					MCerrorlock++;
					if (newfact->parse(*this, doingthe) != PS_NORMAL)
					{
						delete newfact;
						*this = thesp;
						MCerrorlock--;

						if (gettoken().getlength() == 0)
						{
							MCperror->add(PE_EXPRESSION_NOTFACT, *this);
							return PS_ERROR;
						}

						newfact = new MCFuncref(gettoken_nameref());
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

						MCAutoNameRef t_name;
						/* UNCHECKED */ t_name . Clone(gettoken_nameref());

						if (next(type) == PS_NORMAL)
							backup();
						else
							type = ST_ERR;
						// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
						//   execution outwith a handler.
						if (type != ST_LP && findvar(t_name, &newvar) == PS_NORMAL)
						{
							newvar->parsearray(*this);
							newfact = newvar;
						}
						else if (type == ST_LB && !MCexplicitvariables)
							{
								// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
								//   execution outwith a handler.
							if (findnewvar(t_name, kMCEmptyName, &newvar) != PS_NORMAL)
								{
									MCperror->add(PE_EXPRESSION_NOTFACT, *this);
									return PS_ERROR;
								}
								newvar->parsearray(*this);
								newfact = newvar;
							}
						else if (type == ST_LP)
								newfact = new MCFuncref(t_name);
						if (newfact == NULL)
							if (MCexplicitvariables)
							{
								MCperror->add(PE_EXPRESSION_NOTLITERAL, *this);
								return PS_ERROR;
							}
							else
							{
								// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
								//   execution outwith a handler.
								if (finduqlvar(t_name, &newvar) != PS_NORMAL)
								{
									MCperror->add(PE_EXPRESSION_NOTFACT, *this);
									return PS_ERROR;
								}
								newvar->parsearray(*this);
								newfact = newvar;
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

Exec_stat MCScriptPoint::getcommands(MCExecPoint &ep)
{
	bool first = true;
	uint2 i;
	for (i = 0 ; i < command_table_size ; i++)
		if (command_table[i].type == TT_STATEMENT)
		{
			ep.concatcstring(command_table[i].token, EC_RETURN, first);
			first = false;
		}
	return ES_NORMAL;
}

Exec_stat MCScriptPoint::getfactors(MCExecPoint &ep, Token_type which)
{
	bool first = true;
	uint2 i;
	for (i = 0 ; i < factor_table_size ; i++)
		if (factor_table[i].type == which)
		{
			ep.concatcstring(factor_table[i].token, EC_RETURN, first);
			first = false;
		}
	return ES_NORMAL;
}

Exec_stat MCScriptPoint::getconstants(MCExecPoint &ep)
{
	uint2 i;
	for (i = 0 ; i < constant_table_size ; i++)
		ep.concatcstring(constant_table[i].token, EC_RETURN, i == 0);
	return ES_NORMAL;
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
