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
#include "mcio.h"

#include "uidc.h"
#include "scriptpt.h"

#include "chunk.h"
#include "operator.h"
#include "mcerror.h"
#include "util.h"
#include "date.h"
#include "securemode.h"
#include "osspec.h"

#include "globals.h"
#include "exec.h"

///////////////////////////////////////////////////////////////////////////////
//
//  Logical operators
//

void MCAnd::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    bool t_result;
    bool t_left, t_right;

    if (!ctxt . EvalExprAsNonStrictBool(left, EE_AND_BADLEFT, t_left))
        return;

    /* CONDITIONAL EVALUATION */
    if(t_left)
    {
        if (!ctxt . EvalExprAsNonStrictBool(right, EE_AND_BADRIGHT, t_right))
            return;

        MCLogicEvalAnd(ctxt, t_left, t_right, t_result);
    }
    else
        t_result = false;

    if (!ctxt . HasError())
        MCExecValueTraits<bool>::set(r_value, t_result);
}


void MCOr::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    bool t_result;
    bool t_left, t_right;

    if (!ctxt . EvalExprAsNonStrictBool(left, EE_OR_BADLEFT, t_left))
        return;

    if (!t_left)
    {
        if (!ctxt . EvalExprAsNonStrictBool(right, EE_OR_BADRIGHT, t_right))
            return;

        MCLogicEvalOr(ctxt, t_left, t_right, t_result);
    }
    else
        t_result = true;

    if (!ctxt.HasError())
        MCExecValueTraits<bool>::set(r_value, t_result);
}

void MCNot::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    bool t_right;
    bool t_result;

    if (!ctxt . EvalExprAsNonStrictBool(right, EE_NOT_BADRIGHT, t_right))
        return;

    MCLogicEvalNot(ctxt, t_right, t_result);

    if (!ctxt . HasError())
        MCExecValueTraits<bool>::set(r_value, t_result);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bitwise operators
///////////////////////////////////////////////////////////////////////////////
//
//  String operators
//

void MCConcat::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    
    MCAutoValueRef t_left, t_right;
    if (!ctxt . EvalExprAsValueRef(left, EE_CONCAT_BADLEFT, &t_left))
        return;
    
    if (!ctxt . EvalExprAsValueRef(right, EE_CONCAT_BADRIGHT, &t_right))
        return;
    
    // AL-2014-09-11: [[ Bug 12195 ]] Data ought to be concatenated without conversion to text
    if (MCValueGetTypeCode(*t_left) == kMCValueTypeCodeData &&
        MCValueGetTypeCode(*t_right) == kMCValueTypeCodeData)
    {
        MCAutoDataRef t_result;
        MCStringsEvalConcatenate(ctxt, (MCDataRef)*t_left, (MCDataRef)*t_right, &t_result);
        
        if (!ctxt . HasError())
            MCExecValueTraits<MCDataRef>::set(r_value, MCValueRetain(*t_result));
        return;
    }
    
    MCAutoStringRef t_left_string, t_right_string;
    if (!ctxt . ConvertToString(*t_left, &t_left_string))
        return;
    
    if (!ctxt . ConvertToString(*t_right, &t_right_string))
        return;
    
    MCAutoStringRef t_result;
    MCStringsEvalConcatenate(ctxt, *t_left_string, *t_right_string, &t_result);
    
    if (!ctxt . HasError())
        MCExecValueTraits<MCStringRef>::set(r_value, MCValueRetain(*t_result));
}

Parse_stat MCBeginsEndsWith::parse(MCScriptPoint& sp, Boolean the)
{
    initpoint(sp);

    if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) != PS_NORMAL)
    {
        MCperror -> add(PE_BEGINSENDS_NOWITH, sp);
        return PS_ERROR;
    }

    return PS_NORMAL;
}

void MCBeginsWith::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCAutoStringRef t_left, t_right;
    bool t_result;

    if (!ctxt . EvalExprAsStringRef(left, EE_BEGINSENDS_BADLEFT, &t_left)
            || !ctxt . EvalExprAsStringRef(right, EE_BEGINSENDS_BADRIGHT, &t_right))
        return;

    MCStringsEvalBeginsWith(ctxt, *t_left, *t_right, t_result);

    MCExecValueTraits<bool>::set(r_value, t_result);
}

void MCEndsWith::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCAutoStringRef t_left, t_right;
    bool t_result;

    if (!ctxt . EvalExprAsStringRef(left, EE_BEGINSENDS_BADLEFT, &t_left)
            || !ctxt . EvalExprAsStringRef(right, EE_BEGINSENDS_BADRIGHT, &t_right))
        return;

    MCStringsEvalEndsWith(ctxt, *t_left, *t_right, t_result);

    MCExecValueTraits<bool>::set(r_value, t_result);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Numeric operators
//

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the left or right can be an array or number so we use 'tona'.
void MCMinus::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCExecValue t_left, t_right;

    if (left == nil)
    {
        MCExecValueTraits<double>::set(t_left, 0.0);
    }
    else if (left -> eval_ctxt(ctxt, t_left), ctxt . HasError()
             || !ctxt . ConvertToNumberOrArray(t_left))
    {
        ctxt . LegacyThrow(EE_MINUS_BADLEFT);
        return;
    }

    if (right -> eval_ctxt(ctxt, t_right), ctxt . HasError()
            || !ctxt . ConvertToNumberOrArray(t_right))
    {
        ctxt . LegacyThrow(EE_MINUS_BADRIGHT);
        MCExecTypeRelease(t_left);
        return;
    }

    r_value . valueref_value = nil;
    if (t_left. type == kMCExecValueTypeArrayRef)
    {
        if (t_right . type == kMCExecValueTypeArrayRef)
            MCMathEvalSubtractArrayFromArray(ctxt, t_left . arrayref_value, t_right . arrayref_value, r_value . arrayref_value);
        else
            MCMathEvalSubtractNumberFromArray(ctxt, t_left . arrayref_value, t_right . double_value, r_value . arrayref_value);
    }
    else
    {
        if (t_right . type == kMCExecValueTypeArrayRef)
            ctxt . LegacyThrow(EE_MINUS_MISMATCH);
        else
            MCMathEvalSubtract(ctxt, t_left . double_value, t_right . double_value, r_value . double_value);
    }

    if (!ctxt . HasError())
        r_value . type = t_left . type;
    
    MCExecTypeRelease(t_left);
    MCExecTypeRelease(t_right);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Miscellaneous operators
//

void MCGrouping::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    if (right != NULL)
    {
        if (!ctxt . EvaluateExpression(right, EE_GROUPING_BADRIGHT, r_value))
            return;
    }
    else
        ctxt . LegacyThrow(EE_GROUPING_BADRIGHT);
}

Parse_stat MCIs::parse(MCScriptPoint &sp, Boolean the)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_UNOP, O_NOT) == PS_NORMAL)
		form = IT_NOT;
    if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_STRICTLY) == PS_NORMAL)
    {
        if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_NOTHING) == PS_NORMAL)
            valid = IV_UNDEFINED;
        else
        {
            if (sp.skip_token(SP_VALIDATION, TT_UNDEFINED, TT_UNDEFINED) != PS_NORMAL)
            {
	            MCperror -> add(PE_ISSTRICTLY_NOAN, sp);
                return PS_ERROR;
            }
            
            if (sp.skip_token(SP_SORT, TT_UNDEFINED, ST_BINARY) == PS_NORMAL)
            {
                if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_STRING) != PS_NORMAL)
                {
                    MCperror -> add(PE_ISSTRICTLY_NOSTRING, sp);
                    return PS_ERROR;
                }
                
                valid = IV_BINARY_STRING;
            }
            else if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_NOTHING) == PS_NORMAL)
                valid = IV_UNDEFINED;
            else if (sp . skip_token(SP_VALIDATION, TT_UNDEFINED, IV_LOGICAL) == PS_NORMAL)
                valid = IV_LOGICAL;
            else if (sp . skip_token(SP_VALIDATION, TT_UNDEFINED, IV_ARRAY) == PS_NORMAL)
                valid = IV_ARRAY;
            else if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_STRING) == PS_NORMAL)
                valid = IV_STRING;
            else if (sp . skip_token(SP_VALIDATION, TT_UNDEFINED, IV_INTEGER) == PS_NORMAL)
                valid = IV_INTEGER;
            else if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_REAL) == PS_NORMAL)
                valid = IV_REAL;
            else
            {
                MCperror -> add(PE_ISSTRICTLY_NOTYPE, sp);
                return PS_ERROR;
            }
        }
            
        if (form != IT_NOT)
            form = IT_STRICTLY;
        else
            form = IT_NOT_STRICTLY;
        
        return PS_BREAK;
    }
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_IS_NORIGHT, sp);
		return PS_ERROR;
	}
	if (type != ST_ID || sp.lookup(SP_FACTOR, te) != PS_NORMAL)
	{
		if (type != ST_ID || sp.lookup(SP_VALIDATION, te) != PS_NORMAL)
		{
			sp.backup();
			return PS_NORMAL;
		}
		if (te->which == IV_UNDEFINED)
		{
			if (sp.next(type) != PS_NORMAL)
			{
				MCperror->add(PE_IS_NOVALIDTYPE, sp);
				return PS_ERROR;
			}
			if (type != ST_ID || sp.lookup(SP_VALIDATION, te) != PS_NORMAL)
			{
				MCperror->add(PE_IS_BADVALIDTYPE, sp);
				return PS_ERROR;
			}
            
            valid = (Is_validation)te->which;
			
			// MERG-2013-06-24: [[ IsAnAsciiString ]] Parse 'is an ascii string'.
            if (te->which == IV_ASCII)
            {
				if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_STRING) != PS_NORMAL)
                {
                	MCperror->add(PE_IS_BADVALIDTYPE, sp);
                    return PS_ERROR;
                }
            }
			
			return PS_BREAK;
		}
		else
			if (te->which == IV_AMONG)
			{
				sp.skip_token(SP_FACTOR, TT_THE);
				if (sp.next(type) != PS_NORMAL)
				{
					MCperror->add(PE_IS_NOVALIDTYPE, sp);
					return PS_ERROR;
				}
				if (sp.lookup(SP_FACTOR, te) != PS_NORMAL
                    || te -> which == CT_ELEMENT
                    || (te->type != TT_CLASS
                        && (te->type != TT_FUNCTION || te->which != F_KEYS)))
				{
					MCperror->add(PE_IS_BADAMONGTYPE, sp);
					return PS_ERROR;
				}
				
				if (te -> type == TT_FUNCTION && te -> which == F_KEYS)
					delimiter = CT_KEY;
				else if (te -> type == TT_CLASS)
					delimiter = (Chunk_term)te -> which;
				else
                    MCUnreachableReturn(PS_ERROR);

				if (delimiter == CT_CHARACTER)
					if (form == IT_NOT)
						form = IT_NOT_IN;
					else
						form = IT_IN;
				else
					if (form == IT_NOT)
						form = IT_NOT_AMONG;
					else
						form = IT_AMONG;

				sp.skip_token(SP_FACTOR, TT_OF);

				// Support for 'is among the keys of the dragData'
				if (delimiter == CT_KEY)
				{
					if (sp . skip_token(SP_FACTOR, TT_THE) == PS_NORMAL)
					{
						Symbol_type t_type;
						const LT *t_te;
						if (sp . next(t_type) == PS_NORMAL)
						{
							if ((sp.lookup(SP_FACTOR, t_te) == PS_NORMAL
									&& (Properties(t_te->which) == P_DRAG_DATA
                                        || Properties(t_te->which) == P_CLIPBOARD_DATA
                                        || Properties(t_te->which) == P_RAW_CLIPBOARD_DATA
                                        || Properties(t_te->which) == P_RAW_DRAGBOARD_DATA
                                        || Properties(t_te->which) == P_FULL_CLIPBOARD_DATA
                                        || Properties(t_te->which) == P_FULL_DRAGBOARD_DATA)))
							{
								if (Properties(t_te -> which) == P_CLIPBOARD_DATA)
								{
									if (form == IT_NOT_AMONG)
										form = IT_NOT_AMONG_THE_CLIPBOARD_DATA;
									else
										form = IT_AMONG_THE_CLIPBOARD_DATA;
								}
                                else if (Properties(t_te -> which) == P_RAW_CLIPBOARD_DATA)
                                {
                                    if (form == IT_NOT_AMONG)
                                        form = IT_NOT_AMONG_THE_RAW_CLIPBOARD_DATA;
                                    else
                                        form = IT_AMONG_THE_RAW_CLIPBOARD_DATA;
                                }
                                else if (Properties(t_te -> which) == P_RAW_DRAGBOARD_DATA)
                                {
                                   if (form == IT_NOT_AMONG)
                                       form = IT_NOT_AMONG_THE_RAW_DRAGBOARD_DATA;
                                    else
                                        form = IT_AMONG_THE_RAW_DRAGBOARD_DATA;
                                }
                                else if (Properties(t_te -> which) == P_FULL_CLIPBOARD_DATA)
                                {
                                    if (form == IT_NOT_AMONG)
                                        form = IT_NOT_AMONG_THE_FULL_CLIPBOARD_DATA;
                                    else
                                        form = IT_AMONG_THE_FULL_CLIPBOARD_DATA;
                                }
                                else if (Properties(t_te -> which) == P_FULL_DRAGBOARD_DATA)
                                {
                                    if (form == IT_NOT_AMONG)
                                        form = IT_NOT_AMONG_THE_FULL_DRAGBOARD_DATA;
                                    else
                                        form = IT_AMONG_THE_FULL_DRAGBOARD_DATA;
                                }
								else /* if (Properties(te -> which) == P_DRAG_DATA) */
								{
									if (form == IT_NOT_AMONG)
										form = IT_NOT_AMONG_THE_DRAG_DATA;
									else
										form = IT_AMONG_THE_DRAG_DATA;
								}
								left = NULL;
								return PS_BREAK;
							}
							sp . backup();
						}
						sp . backup();
					}
				}
			}
			else
			{
				MCperror->add(PE_IS_NOVALIDTYPE, sp);
				return PS_ERROR;
			}
		return PS_NORMAL;
	}
   	if (te->type == TT_IN)
	{
		rank = FR_COMPARISON;
		if (form == IT_NOT)
			form = IT_NOT_IN;
		else
			form = IT_IN;
	}
	else
		if (te->type == TT_FUNCTION && te->which == F_WITHIN)
		{
			if (form == IT_NOT)
				form = IT_NOT_WITHIN;
			else
				form = IT_WITHIN;
		}
		else
			sp.backup();
	return PS_NORMAL;
}

void MCIs::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    bool t_result = false;
    
    // Implementation of 'is [ not ] strictly'
    if (form == IT_STRICTLY || form == IT_NOT_STRICTLY)
    {
        MCAutoValueRef t_value;
        
        if (!ctxt . EvalExprAsValueRef(right, EE_IS_BADLEFT, &t_value))
            return;
        
        switch(valid)
        {
            case IV_UNDEFINED:
                if (form == IT_STRICTLY)
                    MCEngineEvalIsStrictlyNothing(ctxt, *t_value, t_result);
                else
                    MCEngineEvalIsNotStrictlyNothing(ctxt, *t_value, t_result);
                break;
            case IV_LOGICAL:
                if (form == IT_STRICTLY)
                    MCEngineEvalIsStrictlyABoolean(ctxt, *t_value, t_result);
                else
                    MCEngineEvalIsNotStrictlyABoolean(ctxt, *t_value, t_result);
                break;
            case IV_INTEGER:
                if (form == IT_STRICTLY)
                    MCEngineEvalIsStrictlyAnInteger(ctxt, *t_value, t_result);
                else
                    MCEngineEvalIsNotStrictlyAnInteger(ctxt, *t_value, t_result);
                break;
            case IV_REAL:
                if (form == IT_STRICTLY)
                    MCEngineEvalIsStrictlyAReal(ctxt, *t_value, t_result);
                else
                    MCEngineEvalIsNotStrictlyAReal(ctxt, *t_value, t_result);
                break;
            case IV_STRING:
                if (form == IT_STRICTLY)
                    MCEngineEvalIsStrictlyAString(ctxt, *t_value, t_result);
                else
                    MCEngineEvalIsNotStrictlyAString(ctxt, *t_value, t_result);
                break;
            case IV_BINARY_STRING:
                if (form == IT_STRICTLY)
                    MCEngineEvalIsStrictlyABinaryString(ctxt, *t_value, t_result);
                else
                    MCEngineEvalIsNotStrictlyABinaryString(ctxt, *t_value, t_result);
                break;
            case IV_ARRAY:
                if (form == IT_STRICTLY)
                    MCEngineEvalIsStrictlyAnArray(ctxt, *t_value, t_result);
                else
                    MCEngineEvalIsNotStrictlyAnArray(ctxt, *t_value, t_result);
                break;
			default:
				MCUnreachable();
		}
        
        if (!ctxt . HasError())
            MCExecValueTraits<bool>::set(r_value, t_result);
        
		return;
    }

    // Implementation of 'is a <type>'
    if (valid != IV_UNDEFINED)
    {
        MCAutoValueRef t_value;

        if (!ctxt . EvalExprAsValueRef(right, EE_IS_BADLEFT, &t_value))
            return;

        switch (valid)
        {
        case IV_ARRAY:
            if (form == IT_NORMAL)
                MCArraysEvalIsAnArray(ctxt, *t_value, t_result);
            else
                MCArraysEvalIsNotAnArray(ctxt, *t_value, t_result);
            break;
        case IV_COLOR:
            if (form == IT_NORMAL)
                MCGraphicsEvalIsAColor(ctxt, *t_value, t_result);
            else
                MCGraphicsEvalIsNotAColor(ctxt, *t_value, t_result);
            break;
        case IV_DATE:
            if (form == IT_NORMAL)
                MCDateTimeEvalIsADate(ctxt, *t_value, t_result);
            else
                MCDateTimeEvalIsNotADate(ctxt, *t_value, t_result);
            break;
        case IV_INTEGER:
            if (form == IT_NORMAL)
                MCMathEvalIsAnInteger(ctxt, *t_value, t_result);
            else
                MCMathEvalIsNotAnInteger(ctxt, *t_value, t_result);
            break;
        case IV_NUMBER:
            if (form == IT_NORMAL)
                MCMathEvalIsANumber(ctxt, *t_value, t_result);
            else
                MCMathEvalIsNotANumber(ctxt, *t_value, t_result);
            break;
        case IV_LOGICAL:
            if (form == IT_NORMAL)
                MCLogicEvalIsABoolean(ctxt, *t_value, t_result);
            else
                MCLogicEvalIsNotABoolean(ctxt, *t_value, t_result);
            break;
        case IV_POINT:
            if (form == IT_NORMAL)
                MCGraphicsEvalIsAPoint(ctxt, *t_value, t_result);
            else
                MCGraphicsEvalIsNotAPoint(ctxt, *t_value, t_result);
            break;
        case IV_RECT:
            if (form == IT_NORMAL)
                MCGraphicsEvalIsARectangle(ctxt, *t_value, t_result);
            else
                MCGraphicsEvalIsNotARectangle(ctxt, *t_value, t_result);
            break;
        // MERG-2013-06-24: [[ IsAnAsciiString ]] Implementation for ascii string
        //   check.
        case IV_ASCII:
            if (form == IT_NORMAL)
                MCStringsEvalIsAscii(ctxt, *t_value, t_result);
            else
                MCStringsEvalIsNotAscii(ctxt, *t_value, t_result);
            break;

		default:
			MCUnreachable();
			break;
        }

        if (!ctxt . HasError())
            MCExecValueTraits<bool>::set(r_value, t_result);

        return;
    }

    // Implementation of 'is'
    if (form == IT_NORMAL || form == IT_NOT)
    {
        MCAutoValueRef t_left, t_right;

        if (!ctxt . EvalExprAsValueRef(left, EE_FACTOR_BADLEFT, &t_left)
                || !ctxt . EvalExprAsValueRef(right, EE_FACTOR_BADRIGHT, &t_right))
            return;

        if (form == IT_NORMAL)
            MCLogicEvalIsEqualTo(ctxt, *t_left, *t_right, t_result);
        else
            MCLogicEvalIsNotEqualTo(ctxt, *t_left, *t_right, t_result);
    }


    // The rest
    switch (form)
    {
    case IT_AMONG:
    case IT_NOT_AMONG:
        if (delimiter == CT_KEY)
        {
            MCAutoArrayRef t_array;
            MCNewAutoNameRef t_name;

            if (!ctxt . EvalExprAsNameRef(left, EE_IS_BADLEFT, &t_name))
                return;

            ctxt . TryToEvalExprAsArrayRef(right, EE_IS_BADRIGHT, &t_array);

            if (form == IT_AMONG)
                MCArraysEvalIsAmongTheKeysOf(ctxt, *t_name, *t_array, t_result);
            else
                MCArraysEvalIsNotAmongTheKeysOf(ctxt, *t_name, *t_array, t_result);
        }
        else if (delimiter == CT_BYTE)
        {
            MCAutoDataRef t_left, t_right;
            
            if (!ctxt . EvalExprAsDataRef(left, EE_IS_BADLEFT, &t_left))
                return;

            if (!ctxt . EvalExprAsDataRef(right, EE_IS_BADRIGHT, &t_right))
                return;
            
            if (form == IT_AMONG)
                MCStringsEvalIsAmongTheBytesOf(ctxt, *t_left, *t_right, t_result);
            else
                MCStringsEvalIsNotAmongTheBytesOf(ctxt, *t_left, *t_right, t_result);
            break;
        }
        else
        {
            MCAutoStringRef t_left, t_right;

            if (!ctxt . EvalExprAsStringRef(left, EE_IS_BADLEFT, &t_left))
                return;

            if (!ctxt . EvalExprAsStringRef(right, EE_IS_BADRIGHT, &t_right))
                return;

            switch (delimiter)
            {
            case CT_TOKEN:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheTokensOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheTokensOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_WORD:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheWordsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheWordsOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_LINE:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheLinesOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheLinesOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_ITEM:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheItemsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheItemsOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_PARAGRAPH:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheParagraphsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheParagraphsOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_SENTENCE:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheSentencesOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheSentencesOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_TRUEWORD:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheTrueWordsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheTrueWordsOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_CODEPOINT:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheCodepointsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheCodepointsOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_CODEUNIT:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheCodeunitsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheCodeunitsOf(ctxt, *t_left, *t_right, t_result);
                break;

			default:
				MCUnreachable();
				break;
            }
        }
        break;
    case IT_AMONG_THE_CLIPBOARD_DATA:
    case IT_NOT_AMONG_THE_CLIPBOARD_DATA:
    case IT_AMONG_THE_DRAG_DATA:
    case IT_NOT_AMONG_THE_DRAG_DATA:
    case IT_AMONG_THE_RAW_CLIPBOARD_DATA:
    case IT_NOT_AMONG_THE_RAW_CLIPBOARD_DATA:
    case IT_AMONG_THE_RAW_DRAGBOARD_DATA:
    case IT_NOT_AMONG_THE_RAW_DRAGBOARD_DATA:
    case IT_AMONG_THE_FULL_CLIPBOARD_DATA:
    case IT_NOT_AMONG_THE_FULL_CLIPBOARD_DATA:
    case IT_AMONG_THE_FULL_DRAGBOARD_DATA:
    case IT_NOT_AMONG_THE_FULL_DRAGBOARD_DATA:
        {
            MCNewAutoNameRef t_right;

            if (!ctxt . EvalExprAsNameRef(right, EE_IS_BADLEFT, &t_right))
                return;

            if (form == IT_AMONG_THE_CLIPBOARD_DATA)
                MCPasteboardEvalIsAmongTheKeysOfTheClipboardData(ctxt, *t_right, t_result);
            else if (form == IT_NOT_AMONG_THE_CLIPBOARD_DATA)
                MCPasteboardEvalIsNotAmongTheKeysOfTheClipboardData(ctxt, *t_right, t_result);
            else if (form == IT_AMONG_THE_DRAG_DATA)
                MCPasteboardEvalIsAmongTheKeysOfTheDragData(ctxt, *t_right, t_result);
            else if (form == IT_NOT_AMONG_THE_DRAG_DATA)
                MCPasteboardEvalIsNotAmongTheKeysOfTheDragData(ctxt, *t_right, t_result);
            else if (form == IT_AMONG_THE_RAW_CLIPBOARD_DATA)
                MCPasteboardEvalIsAmongTheKeysOfTheRawClipboardData(ctxt, *t_right, t_result);
            else if (form == IT_NOT_AMONG_THE_RAW_CLIPBOARD_DATA)
                MCPasteboardEvalIsNotAmongTheKeysOfTheRawClipboardData(ctxt, *t_right, t_result);
            else if (form == IT_AMONG_THE_RAW_DRAGBOARD_DATA)
                MCPasteboardEvalIsAmongTheKeysOfTheRawDragData(ctxt, *t_right, t_result);
            else if (form == IT_NOT_AMONG_THE_RAW_DRAGBOARD_DATA)
                MCPasteboardEvalIsNotAmongTheKeysOfTheRawDragData(ctxt, *t_right, t_result);
            else if (form == IT_AMONG_THE_FULL_CLIPBOARD_DATA)
                MCPasteboardEvalIsAmongTheKeysOfTheFullClipboardData(ctxt, *t_right, t_result);
            else if (form == IT_NOT_AMONG_THE_FULL_CLIPBOARD_DATA)
                MCPasteboardEvalIsNotAmongTheKeysOfTheFullClipboardData(ctxt, *t_right, t_result);
            else if (form == IT_AMONG_THE_FULL_DRAGBOARD_DATA)
                MCPasteboardEvalIsAmongTheKeysOfTheFullDragData(ctxt, *t_right, t_result);
            else if (form == IT_NOT_AMONG_THE_FULL_DRAGBOARD_DATA)
                MCPasteboardEvalIsNotAmongTheKeysOfTheFullDragData(ctxt, *t_right, t_result);
            else
                MCUnreachable();
        }
        break;
    case IT_IN:
    case IT_NOT_IN:
        {
            MCAutoStringRef t_left, t_right;

            if (!ctxt . EvalExprAsStringRef(left, EE_IS_BADLEFT, &t_left))
                return;

            if (!ctxt . EvalExprAsStringRef(right, EE_IS_BADRIGHT, &t_right))
                return;

            if (form == IT_IN)
                MCStringsEvalContains(ctxt, *t_right, *t_left, t_result);
            else
                MCStringsEvalDoesNotContain(ctxt, *t_right, *t_left,t_result);
        }
        break;
    case IT_WITHIN:
    case IT_NOT_WITHIN:
        {
            MCPoint t_point;
            MCRectangle t_rectangle;

            if (!ctxt . EvalExprAsPoint(left, EE_IS_BADLEFT, t_point))
                return;

            if (!ctxt . EvalExprAsRectangle(right, EE_IS_BADRIGHT, t_rectangle))
                return;

            if (form == IT_WITHIN)
                MCGraphicsEvalIsWithin(ctxt, t_point, t_rectangle, t_result);
            else
                MCGraphicsEvalIsNotWithin(ctxt, t_point, t_rectangle, t_result);
        }
        break;
    default:
        break;
    }

    if (!ctxt . HasError())
        MCExecValueTraits<bool>::set(r_value, t_result);
}

MCThere::~MCThere()
{
	delete object;
}

Parse_stat MCThere::parse(MCScriptPoint &sp, Boolean the)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_BINOP, O_IS) != PS_NORMAL)
	{
		MCperror->add(PE_THERE_NOIS, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_UNOP, O_NOT) == PS_NORMAL)
		form = IT_NOT;
	sp.skip_token(SP_VALIDATION, TT_UNDEFINED, IV_UNDEFINED);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_THERE_NOOBJECT, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_THERE, te) != PS_NORMAL)
	{
		sp.backup();
		object = new (nothrow) MCChunk(False);
		if (object->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_THERE_NOOBJECT, sp);
			return PS_ERROR;
		}
		right = new (nothrow) MCExpression(); // satisfy check in scriptpt.parse
	}
	else
	{
		mode = (There_mode)te->which;
		if (sp.parseexp(True, False, &right) != PS_NORMAL)
		{
			MCperror->add(PE_THERE_BADFILE, sp);
			return PS_ERROR;
		}
	}
	return PS_BREAK;
}

void MCThere::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	bool t_result;

	if (object == NULL)
	{
        MCAutoStringRef t_string;

        if (!ctxt . EvalExprAsStringRef(right, EE_THERE_BADSOURCE, &t_string))
            return;

		switch (mode)
		{
		case TM_PROCESS:
			if (form == IT_NORMAL)
				MCFilesEvalThereIsAProcess(ctxt, *t_string, t_result);
			else
				MCFilesEvalThereIsNotAProcess(ctxt, *t_string, t_result);
			break;
		case TM_FILE:
			if (form == IT_NORMAL)
				MCFilesEvalThereIsAFile(ctxt, *t_string, t_result);
			else
				MCFilesEvalThereIsNotAFile(ctxt, *t_string, t_result);
			break;
        // AL-2014-10-02: [[ Bug 13579 ]] Default behavior is to check if there is a folder.
        // In particular, this is the codepath for 'there is a url' for some reason.
		case TM_DIRECTORY:
        default:
			if (form == IT_NORMAL)
				MCFilesEvalThereIsAFolder(ctxt, *t_string, t_result);
			else
				MCFilesEvalThereIsNotAFolder(ctxt, *t_string, t_result);
			break;
		}
	}
	else
	{
		if (form == IT_NORMAL)
			MCInterfaceEvalThereIsAnObject(ctxt, object, t_result);
		else
			MCInterfaceEvalThereIsNotAnObject(ctxt, object, t_result);
	}

    if (!ctxt.HasError())
        MCExecValueTraits<bool>::set(r_value, t_result);
}
