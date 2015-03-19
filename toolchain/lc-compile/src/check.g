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

'module' check

'export'
    Check

'use'
    support
    types

--------------------------------------------------------------------------------

'action' Check(MODULE)

    'rule' Check(Module):
        -- Check that all the ids bind to the correct type of thing. Any ids
        -- which aren't bound correctly get assigned a meaning of 'error'.
        CheckBindings(Module)
        
        -- Check that all the uses of syntax are appropriate to context.
        CheckInvokes(Module)
        
        -- Check that appropriate types are used in appropriate places.
        CheckDeclaredTypes(Module)

        -- Check the syntax definitions are all correct.
        CheckSyntaxDefinitions(Module)

        -- Check that suitable identifiers are used in definitions
        CheckIdentifiers(Module)

--------------------------------------------------------------------------------

-- At this point all identifiers either have a defined meaning, or are defined
-- to be a pointer to the definingid. The next step is to check that bindings
-- are appropriate to each id:
--   BD1) DEFINITION'constant(Value) - all id's in Value must be constant.
--   BD2) DEFINITION'property(Getter, Setter) - both id's must be handlers or variables
--
--   BT1) TYPE'named(Id) - Id must be bound to a type.
--
--   BS1) STATEMENT'repeatupto(Slot) - Slot must be bound to a variable
--   BS2) STATEMENT'repeatdownto(Slot) - Slot must be bound to a variable
--   BS3) STATEMENT'repeatforeach(Slot) - Slot must be bound to a variable
--   BS4) STATEMENT'call(Handler) - Handler must be bound to a callable id
--
--   BE1) EXPRESSION'slot(Name) - Name must be bound to a fetachable id
--   BE2) EXPRESSION'call(Handler) - Handler must be bound to a callable id

--   BX1) SYNTAX'markedrule(Name) - Name must be bound to an expression-returning syntax rule
--   BX2) SYNTAX'markedrule(Variable) - Variable must be bound to a syntax mark
--   BX3) SYNTAX'rule(Name) - Name must be bound to an expression-returning syntax rule
--   BX4) SYNTAX'mark(Variable) - Variable must be bound to a syntax mark
--   BX5) SYNTAX'mark(Value) - Value must be bound to a non-mark constant
--   BX6) SYNTAXMETHOD'method(Name) - Name must be bound to a handler
--   BX7) SYNTAXCONSTANT'variable(Name) - Name must be bound to a syntax mark
--   BX8) SYNTAXCONSTANT'indexedvariable(Name) - Name must be bound to a syntax mark.

'sweep' CheckBindings(ANY)
    
    --

    'rule' CheckBindings(DEFINITION'constant(Position, _, _, Value)):
        --/* BD1 */ CheckBindingsOfConstantExpression(Value)
        (|
            IsExpressionSimpleConstant(Value)
        ||
            Error_ConstantsMustBeSimple(Position)
        |)

    'rule' CheckBindings(DEFINITION'property(Position, _, _, Getter, OptionalSetter)):
        /* BD2 */ CheckBindingIsVariableOrGetHandlerId(Getter)
        [|
            where(OptionalSetter -> id(Setter))
            /* BD2 */ CheckBindingIsVariableOrSetHandlerId(Setter)
        |]

    --

    'rule' CheckBindings(TYPE'named(Position, Name)):
        /* BT1 */ CheckBindingIsTypeId(Name)

    'rule' CheckBindings(FIELD'action(_, _, Handler)):
        /* BT2 */ CheckBindingIsHandlerId(Handler)

    --
    
    'rule' CheckBindings(STATEMENT'repeatupto(_, Slot, Start, Finish, Step, Body)):
        /* BS1 */ CheckBindingIsVariableId(Slot)
        CheckBindings(Start)
        CheckBindings(Finish)
        CheckBindings(Step)
        CheckBindings(Body)

    'rule' CheckBindings(STATEMENT'repeatdownto(_, Slot, Start, Finish, Step, Body)):
        /* BS2 */ CheckBindingIsVariableId(Slot)
        CheckBindings(Start)
        CheckBindings(Finish)
        CheckBindings(Step)
        CheckBindings(Body)

    'rule' CheckBindings(STATEMENT'repeatforeach(_, Iterator, Container, Body)):
        -- /* BS3 */ CheckBindingIsVariableId(Slot)
        CheckBindings(Iterator)
        CheckBindings(Container)
        CheckBindings(Body)

    'rule' CheckBindings(STATEMENT'call(_, Handler, Arguments)):
        /* B4 */ CheckBindingIsCallableVariableOrHandlerId(Handler)
        CheckBindings(Arguments)

    --

    'rule' CheckBindings(EXPRESSION'slot(_, Name)):
        /* BE1 */ CheckBindingIsConstantOrVariableOrHandlerId(Name)

    'rule' CheckBindings(EXPRESSION'call(_, Handler, Arguments)):
        /* BE2 */ CheckBindingIsCallableVariableOrHandlerId(Handler)
        CheckBindings(Arguments)
        
    --
    
    'rule' CheckBindings(SYNTAX'markedrule(_, Variable, Name)):
        /* BX1 */ CheckBindingIsSyntaxRuleOfExpressionType(Name)
        /* BX2 */ CheckBindingIsSyntaxMark(Variable)

    'rule' CheckBindings(SYNTAX'rule(_, Name)):
        /* BX3 */ CheckBindingIsSyntaxRuleOfExpressionType(Name)
        
    'rule' CheckBindings(SYNTAX'mark(Position, Variable, Value)):
        /* BX4 */ CheckBindingIsSyntaxMark(Variable)
        /* BX5 */ CheckBindingIsConstantSyntaxValue(Variable, Value)
    
    'rule' CheckBindings(SYNTAXMETHOD'method(_, Name, Arguments)):
        /* BX6 */ CheckBindingIsHandlerId(Name)
        CheckBindings(Arguments)
        
    'rule' CheckBindings(SYNTAXCONSTANT'variable(_, Name)):
        /* BX7 */ CheckBindingIsSyntaxMarkUse(Name)

    'rule' CheckBindings(SYNTAXCONSTANT'indexedvariable(_, Name, _)):
        /* BX7 */ CheckBindingIsSyntaxMarkUse(Name)
        
---------

'sweep' CheckBindingsOfConstantExpression(ANY)

    'rule' CheckBindingsOfConstantExpression(EXPRESSION'nil):
        -- TODO

'action' CheckBindingIsTypeId(ID)

    'rule' CheckBindingIsTypeId(Id):
        -- Do nothing if the meaning is error.
        QueryId(Id -> error)

    'rule' CheckBindingIsTypeId(Id):
        QueryKindOfSymbolId(Id -> type)
        
    'rule' CheckBindingIsTypeId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAType(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error
        
'action' CheckBindingIsHandlerId(ID)

    'rule' CheckBindingIsHandlerId(Id):
        -- Do nothing if the meaning is error.
        QueryId(Id -> error)

    'rule' CheckBindingIsHandlerId(Id):
        QueryKindOfSymbolId(Id -> handler)
        
    'rule' CheckBindingIsHandlerId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAHandler(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

'action' CheckBindingIsVariableId(ID)

    'rule' CheckBindingIsVariableId(Id):
        -- Do nothing if the meaning is error.
        QueryId(Id -> error)

    'rule' CheckBindingIsVariableId(Id):
        QueryKindOfSymbolId(Id -> variable)
        
    'rule' CheckBindingIsVariableId(Id):
        QueryKindOfSymbolId(Id -> parameter)

    'rule' CheckBindingIsVariableId(Id):
        QueryKindOfSymbolId(Id -> local)

    'rule' CheckBindingIsVariableId(Id):
        QueryKindOfSymbolId(Id -> context)

    'rule' CheckBindingIsVariableId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAVariable(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

'action' CheckBindingIsVariableOrHandlerId(ID)

    'rule' CheckBindingIsVariableOrHandlerId(Id):
        -- Do nothing if the meaning is error.
        QueryId(Id -> error)

    'rule' CheckBindingIsVariableOrHandlerId(Id):
        QueryKindOfSymbolId(Id -> variable)
        
    'rule' CheckBindingIsVariableOrHandlerId(Id):
        QueryKindOfSymbolId(Id -> parameter)

    'rule' CheckBindingIsVariableOrHandlerId(Id):
        QueryKindOfSymbolId(Id -> local)

    'rule' CheckBindingIsVariableOrHandlerId(Id):
        QueryKindOfSymbolId(Id -> context)

    'rule' CheckBindingIsVariableOrHandlerId(Id):
        QueryKindOfSymbolId(Id -> handler)

    'rule' CheckBindingIsVariableOrHandlerId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAVariableOrHandler(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

'action' CheckBindingIsConstantOrVariableOrHandlerId(ID)

    'rule' CheckBindingIsConstantOrVariableOrHandlerId(Id):
        -- Do nothing if the meaning is error.
        QueryId(Id -> error)

    'rule' CheckBindingIsConstantOrVariableOrHandlerId(Id):
        QueryKindOfSymbolId(Id -> constant)

    'rule' CheckBindingIsConstantOrVariableOrHandlerId(Id):
        QueryKindOfSymbolId(Id -> variable)
        
    'rule' CheckBindingIsConstantOrVariableOrHandlerId(Id):
        QueryKindOfSymbolId(Id -> parameter)

    'rule' CheckBindingIsConstantOrVariableOrHandlerId(Id):
        QueryKindOfSymbolId(Id -> local)

    'rule' CheckBindingIsConstantOrVariableOrHandlerId(Id):
        QueryKindOfSymbolId(Id -> context)

    'rule' CheckBindingIsConstantOrVariableOrHandlerId(Id):
        QueryKindOfSymbolId(Id -> handler)

    'rule' CheckBindingIsConstantOrVariableOrHandlerId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAConstantOrVariableOrHandler(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

'action' FullyResolveType(TYPE -> TYPE)

    'rule' FullyResolveType(optional(_, Type) -> Base):
        FullyResolveType(Type -> Base)
    
    'rule' FullyResolveType(named(_, Id) -> Base):
        QuerySymbolId(Id -> Named)
        Named'Type -> Type
        FullyResolveType(Type -> Base)
        
    'rule' FullyResolveType(Type -> Type):

'action' CheckBindingIsCallableVariableOrHandlerId(ID)

    'rule' CheckBindingIsCallableVariableOrHandlerId(Id):
        CheckBindingIsVariableOrHandlerId(Id)
        [|
            QuerySymbolId(Id -> Info)
            Info'Kind -> Kind
            -- If we get here the symbol is either a handler or variable (local or global)
            ne(Kind, handler)
            Info'Type -> Type
            FullyResolveType(Type -> BaseType)
            (|
                where(BaseType -> handler(_, Signature))
            ||
                Id'Position -> Position
                Error_NonHandlerTypeVariablesCannotBeCalled(Position)
            |)
        |]

'action' CheckBindingIsVariableOrGetHandlerId(ID)

    'rule' CheckBindingIsVariableOrGetHandlerId(Id):
        QuerySymbolId(Id -> Info)
        Info'Kind -> handler
        Info'Type -> handler(_, Signature)
        (|
            where(Signature -> signature(nil, ReturnType))
            (|
                where(ReturnType -> undefined(_))
                Id'Name -> Name
                Id'Position -> Position
                Error_HandlerNotSuitableForPropertyGetter(Position, Name)
            ||
                -- all non-void return values are fine
            |)
        ||
            Id'Name -> Name
            Id'Position -> Position
            Error_HandlerNotSuitableForPropertyGetter(Position, Name)
        |)

    'rule' CheckBindingIsVariableOrGetHandlerId(Id):
        CheckBindingIsVariableOrHandlerId(Id)

'action' CheckBindingIsVariableOrSetHandlerId(ID)

    'rule' CheckBindingIsVariableOrSetHandlerId(Id):
        QuerySymbolId(Id -> Info)
        Info'Kind -> handler
        Info'Type -> handler(_, Signature)
        (|
            where(Signature -> signature(parameterlist(parameter(_, in, _, _), nil), _))
        ||
            Id'Name -> Name
            Id'Position -> Position
            Error_HandlerNotSuitableForPropertySetter(Position, Name)
        |)

    'rule' CheckBindingIsVariableOrSetHandlerId(Id):
        CheckBindingIsVariableOrHandlerId(Id)

'action' CheckBindingIsSyntaxRuleOfExpressionType(ID)

    'rule' CheckBindingIsSyntaxRuleOfExpressionType(Id):
        -- Do nothing if the meaning is error.
        QueryId(Id -> error)

    'rule' CheckBindingIsSyntaxRuleOfExpressionType(Id):
        QueryClassOfSyntaxId(Id -> expressionphrase)

    'rule' CheckBindingIsSyntaxRuleOfExpressionType(Id):
        QueryClassOfSyntaxId(Id -> expressionlistphrase)
        
    'rule' CheckBindingIsSyntaxRuleOfExpressionType(Id):
        QueryClassOfSyntaxId(Id -> phrase)
        
    'rule' CheckBindingIsSyntaxRuleOfExpressionType(Id):
        QueryClassOfSyntaxId(Id -> _)
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAPhrase(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error
        
    'rule' CheckBindingIsSyntaxRuleOfExpressionType(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToASyntaxRule(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

'action' CheckBindingIsSyntaxMark(ID)

    'rule' CheckBindingIsSyntaxMark(Id):
        QueryId(Id -> error)

    'rule' CheckBindingIsSyntaxMark(Id):
        QueryTypeOfSyntaxMarkId(Id -> _)
        
    'rule' CheckBindingIsSyntaxMark(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToASyntaxMark(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

'action' CheckBindingIsConstantSyntaxValue(ID, SYNTAXCONSTANT)

    'rule' CheckBindingIsConstantSyntaxValue(_, undefined(_)):

    'rule' CheckBindingIsConstantSyntaxValue(_, true(_)):

    'rule' CheckBindingIsConstantSyntaxValue(_, false(_)):

    'rule' CheckBindingIsConstantSyntaxValue(_, integer(_, _)):

    'rule' CheckBindingIsConstantSyntaxValue(_, real(_, _)):

    'rule' CheckBindingIsConstantSyntaxValue(_, string(_, _)):

    'rule' CheckBindingIsConstantSyntaxValue(Id, _):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAConstantSyntaxValue(Position, Name)

'action' CheckBindingIsSyntaxMarkUse(ID)

    'rule' CheckBindingIsSyntaxMarkUse(Id):
        QueryId(Id -> error)

    'rule' CheckBindingIsSyntaxMarkUse(Id):
        QueryTypeOfSyntaxMarkId(Id -> _)

    'rule' CheckBindingIsSyntaxMarkUse(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToASyntaxMark(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

'condition' IsExpressionSimpleConstant(EXPRESSION)

    'rule' IsExpressionSimpleConstant(undefined(_)):
    'rule' IsExpressionSimpleConstant(true(_)):
    'rule' IsExpressionSimpleConstant(false(_)):
    'rule' IsExpressionSimpleConstant(unsignedinteger(_, _)):
    'rule' IsExpressionSimpleConstant(integer(_, _)):
    'rule' IsExpressionSimpleConstant(real(_, _)):
    'rule' IsExpressionSimpleConstant(string(_, _)):
    'rule' IsExpressionSimpleConstant(list(_, List)):
        IsExpressionListSimpleConstant(List)
        
'condition' IsExpressionListSimpleConstant(EXPRESSIONLIST)

    'rule' IsExpressionListSimpleConstant(expressionlist(Head, Tail)):
        IsExpressionSimpleConstant(Head)
        IsExpressionListSimpleConstant(Tail)

    'rule' IsExpressionListSimpleConstant(nil):
        -- nothing

--------------------------------------------------------------------------------

-- The syntax clauses have the following rules:
--   S1) Mark variables can be used only once on any concrete path through the
--      syntax. (DONE)
--   S2) Mark variables cannot have the name 'output', 'input', 'context', 'iterator'
--       'container' or 'error'. (DONE - in Bind)
--   S3) A signature of a method referenced must match the derived signature
--      of the parameters specified for it.
--   S4) A binary operator must start with an Expression and end with an
--      Expression. (DONE)
--   S5) A prefix operator must end with an Expression. (DONE)
--   S6) A postfix operator must start with an Expression. (DONE)
--   S7) Method conformance:
--       a) An expression class rule must have one parameter bound to input, or output but not both.
--       d) A method can bind at most one parameter to context.
--       *g) A method must have the same number of parameters as arguments.
--       *e) A parameter bound to a constant mark must be of 'in' mode.
--       *f) A parameter bound to a phrase (descent) mark must be of 'in' mode.
--       *h) A parameter bound to context must be of 'in' mode.
--       *b) A parameter bound to input must be of 'in' mode.
--       *c) A parameter bound to output must be of 'out' mode.
--       *i) A method bound to syntax must return nothing.
--       j) Indexed syntax arguments must bind to expressionlist.
--   S8) Only terminals are allowed in the delimiter section of a repetition. (DONE)
--   S9) The element section of a repetition must not be nullable. (DONE)
--   S10) Rules must be of expression type. (DONE - CheckBindings)
--   S11) Values of marks must be constant. (DONE)
--   S12) Optional elements must contain a terminal (DONE)

'sweep' CheckSyntaxDefinitions(ANY)

    'rule' CheckSyntaxDefinitions(DEFINITION'syntax(Position, _, _, Class, Syntax, Methods)):
        -- Check that mark variables are only defined once on each path.
        PushEmptySet()
        /* S1 */ CheckSyntaxMarkDefinitions(Syntax)
        
        -- Check that operators have the appropriate form.
        /* S4, S5, S6 */ CheckSyntaxStructure(Position, Class, Syntax)
        
        -- Check that the repeat clauses have supported structure.
        /* S8, S9 */ CheckSyntaxRepetitions(Syntax)
        
        -- Check that optional elements contain a terminal
        /* S12 */ CheckSyntaxOptionals(Syntax)
        
        -- Check that values of marks are constant
        /* S11 */ CheckSyntaxMarks(Syntax)
        
        -- Check method conformance for the class.
        /* S7 */ CheckSyntaxMethods(Class, Methods)

        -- Check keywords have appropriate form
        CheckSyntaxKeywords(Syntax)
        
-- Mark variables can only be defined once for each possible path through a
-- syntax rule. For example:
--   <Foo: Expression> <Foo: Expression> is not allowed
--   ( <Foo: Expression> | <Foo: Expression> ) is allowed.
--
'action' CheckSyntaxMarkDefinitions(SYNTAX)

    'rule' CheckSyntaxMarkDefinitions(concatenate(_, Left, Right)):
        CheckSyntaxMarkDefinitions(Left)
        CheckSyntaxMarkDefinitions(Right)
        
    'rule' CheckSyntaxMarkDefinitions(alternate(_, Left, Right)):
        DuplicateSet()
        CheckSyntaxMarkDefinitions(Left)
        ExchangeSet()
        CheckSyntaxMarkDefinitions(Right)
        UnionSet()

    'rule' CheckSyntaxMarkDefinitions(repeat(_, Element)):
        CheckSyntaxMarkDefinitions(Element)
        
    'rule' CheckSyntaxMarkDefinitions(list(_, Element, Delimiter)):
        CheckSyntaxMarkDefinitions(Element)
        
    'rule' CheckSyntaxMarkDefinitions(optional(_, Operand)):
        CheckSyntaxMarkDefinitions(Operand)

    'rule' CheckSyntaxMarkDefinitions(markedrule(_, Variable, Rule)):
        CheckSyntaxMarkVariableNotDefined(Variable)
        ComputeSyntaxRuleType(Rule -> Type)
        CheckSyntaxMarkVariableType(Variable, Type)

    'rule' CheckSyntaxMarkDefinitions(mark(_, Variable, Constant)):
        CheckSyntaxMarkVariableNotDefined(Variable)
        ComputeSyntaxArgumentType(Constant -> Type)
        CheckSyntaxMarkVariableType(Variable, Type)

    'rule' CheckSyntaxMarkDefinitions(_):
        -- do nothing

'action' CheckSyntaxMarkVariableNotDefined(ID)

    'rule' CheckSyntaxMarkVariableNotDefined(Variable):
        Variable'Meaning -> syntaxmark(Info)
        Info'Index -> Index
        (|
            IsIndexInSet(Index)
            Variable'Position -> Position
            Variable'Name -> Name
            Error_SyntaxMarkVariableAlreadyDefined(Position, Name)
        ||
            IncludeIndexInSet(Index)
        |)
        
    'rule' CheckSyntaxMarkVariableNotDefined(Variable):
        Variable'Meaning -> error

'action' CheckSyntaxMarkVariableType(ID, SYNTAXMARKTYPE)

    'rule' CheckSyntaxMarkVariableType(Variable, Type):
        Variable'Meaning -> syntaxmark(Info)
        Info'Type -> MarkType
        (|
            where(MarkType -> uncomputed)
            Info'Type <- Type
        ||
            eq(MarkType, Type)
            -- This is fine
        ||
            eq(MarkType, phrase)
            eq(Type, expression)
        ||
            eq(MarkType, expression)
            eq(Type, phrase)
        ||
            eq(Type, error)
            -- This is fine
        ||
            Variable'Position -> Position
            Variable'Name -> Name
            Error_SyntaxMarkVariableAlreadyDefinedWithDifferentType(Position, Name)
        |)
        
'action' ComputeSyntaxRuleType(ID -> SYNTAXMARKTYPE)

    'rule' ComputeSyntaxRuleType(Id -> error):
        QueryId(Id -> error)

    'rule' ComputeSyntaxRuleType(Id -> expression):
        QueryClassOfSyntaxId(Id -> expressionphrase)

    'rule' ComputeSyntaxRuleType(Id -> expression):
        QueryClassOfSyntaxId(Id -> expressionlistphrase)
        
    'rule' ComputeSyntaxRuleType(Id -> phrase):
        QueryClassOfSyntaxId(Id -> phrase)
        
    'rule' ComputeSyntaxRuleType(Id -> error):
        Fatal_InternalInconsistency("Referenced syntax rule bound to non-expression")

'action' ComputeSyntaxArgumentType(SYNTAXCONSTANT -> SYNTAXMARKTYPE)

    'rule' ComputeSyntaxArgumentType(undefined(_) -> undefined):

    'rule' ComputeSyntaxArgumentType(true(_) -> boolean):

    'rule' ComputeSyntaxArgumentType(false(_) -> boolean):

    'rule' ComputeSyntaxArgumentType(integer(_, _) -> integer):

    'rule' ComputeSyntaxArgumentType(real(_, _) -> real):
        
    'rule' ComputeSyntaxArgumentType(string(_, _) -> string):

    'rule' ComputeSyntaxArgumentType(variable(_, Name) -> Type):
        QueryTypeOfSyntaxMarkId(Name -> Type)

    'rule' ComputeSyntaxArgumentType(indexedvariable(_, Name, _) -> Type):
        QueryTypeOfSyntaxMarkId(Name -> Type)
        
    'rule' ComputeSyntaxArgumentType(_ -> error):
        --

--

'action' CheckSyntaxStructure(POS, SYNTAXCLASS, SYNTAX)

    'rule' CheckSyntaxStructure(Position, Class, Syntax):
        ComputeSyntaxPrefixAndSuffix(Syntax -> Prefix, Suffix)
        CheckPrefixOfSyntaxStructure(Position, Class, Prefix)
        CheckSuffixOfSyntaxStructure(Position, Class, Suffix)

'action' CheckPrefixOfSyntaxStructure(POS, SYNTAXCLASS, SYNTAXTERM)

    'rule' CheckPrefixOfSyntaxStructure(Position, _, error):
        -- Do nothing if term is an error.

    'rule' CheckPrefixOfSyntaxStructure(Position, expression, Term):
        [|
            where(Term -> expression)
            Error_ExpressionSyntaxCannotStartWithExpression(Position)
        |]

    'rule' CheckPrefixOfSyntaxStructure(Position, prefix, Term):
        [|
            where(Term -> expression)
            Error_PrefixSyntaxCannotStartWithExpression(Position)
        |]
        
    'rule' CheckPrefixOfSyntaxStructure(Position, postfix, Term):
        (|
            where(Term -> expression)
        ||
            Error_PostfixSyntaxMustStartWithExpression(Position)
        |)
        
    'rule' CheckPrefixOfSyntaxStructure(Position, binary, Term):
        (|
            where(Term -> expression)
        ||
            Error_BinarySyntaxMustStartWithExpression(Position)
        |)
        
    'rule' CheckPrefixOfSyntaxStructure(_, _, _):
        -- Do nothing

'action' CheckSuffixOfSyntaxStructure(POS, SYNTAXCLASS, SYNTAXTERM)

    'rule' CheckSuffixOfSyntaxStructure(Position, _, error):
        -- Do nothing if term is an error.

    'rule' CheckSuffixOfSyntaxStructure(Position, expression, Term):
        [|
            where(Term -> expression)
            Error_ExpressionSyntaxCannotFinishWithExpression(Position)
        |]

    'rule' CheckSuffixOfSyntaxStructure(Position, prefix, Term):
        (|
            where(Term -> expression)
        ||
            Error_PrefixSyntaxMustFinishWithExpression(Position)
        |)
        
    'rule' CheckSuffixOfSyntaxStructure(Position, postfix, Term):
        [|
            where(Term -> expression)
            Error_PostfixSyntaxCannotFinishWithExpression(Position)
        |]
        
    'rule' CheckSuffixOfSyntaxStructure(Position, binary, Term):
        (|
            where(Term -> expression)
        ||
            Error_BinarySyntaxMustFinishWithExpression(Position)
        |)

    'rule' CheckSuffixOfSyntaxStructure(_, _, _):
        -- Do nothing

--

'action' CheckSyntaxRepetitions(SYNTAX)

    'rule' CheckSyntaxRepetitions(concatenate(_, Left, Right)):
        CheckSyntaxRepetitions(Left)
        CheckSyntaxRepetitions(Right)
        
    'rule' CheckSyntaxRepetitions(alternate(_, Left, Right)):
        CheckSyntaxRepetitions(Left)
        CheckSyntaxRepetitions(Right)

    'rule' CheckSyntaxRepetitions(repeat(_, Element)):
        CheckSyntaxRepetitions(Element)
        
    'rule' CheckSyntaxRepetitions(list(Position, Element, Delimiter)):
        CheckSyntaxRepetitions(Element)
        CheckSyntaxOnlyKeywordsInDelimiter(Delimiter)
        [|
            SyntaxIsNullable(Element)
            Error_ElementSyntaxCannotBeNullable(Position)
        |]
        
    'rule' CheckSyntaxRepetitions(optional(_, Operand)):
        CheckSyntaxRepetitions(Operand)

    'rule' CheckSyntaxRepetitions(_):
        -- do nothing
        
'action' CheckSyntaxOnlyKeywordsInDelimiter(SYNTAX)

    'rule' CheckSyntaxOnlyKeywordsInDelimiter(concatenate(_, Left, Right)):
        CheckSyntaxOnlyKeywordsInDelimiter(Left)
        CheckSyntaxOnlyKeywordsInDelimiter(Right)
        
    'rule' CheckSyntaxOnlyKeywordsInDelimiter(alternate(_, Left, Right)):
        CheckSyntaxOnlyKeywordsInDelimiter(Left)
        CheckSyntaxOnlyKeywordsInDelimiter(Right)

    'rule' CheckSyntaxOnlyKeywordsInDelimiter(optional(_, Operand)):
        CheckSyntaxOnlyKeywordsInDelimiter(Operand)

    'rule' CheckSyntaxOnlyKeywordsInDelimiter(Syntax):
        Syntax'Position -> Position
        Error_OnlyKeywordsAllowedInDelimiterSyntax(Position)

----------

'action' CheckSyntaxOptionals(SYNTAX)

    'rule' CheckSyntaxOptionals(concatenate(_, Left, Right)):
        CheckSyntaxOptionals(Left)
        CheckSyntaxOptionals(Right)
        
    'rule' CheckSyntaxOptionals(alternate(_, Left, Right)):
        CheckSyntaxOptionals(Left)
        CheckSyntaxOptionals(Right)

    'rule' CheckSyntaxOptionals(repeat(_, Element)):
        CheckSyntaxOptionals(Element)
        
    'rule' CheckSyntaxOptionals(list(Position, Element, Delimiter)):
        CheckSyntaxOptionals(Element)
        CheckSyntaxOptionals(Delimiter)
        
    'rule' CheckSyntaxOptionals(optional(Position, Operand)):
        ComputeSyntaxPrefixAndSuffix(Operand -> Prefix, Suffix)
        [|
            eq(Prefix, Suffix)
            eq(Prefix, mark)
            Error_OptionalSyntaxCannotContainOnlyMarks(Position)
        |]

    'rule' CheckSyntaxOptionals(_):
        -- do nothing

----------

'action' CheckSyntaxMarks(SYNTAX)

    'rule' CheckSyntaxMarks(concatenate(_, Left, Right)):
        CheckSyntaxMarks(Left)
        CheckSyntaxMarks(Right)
        
    'rule' CheckSyntaxMarks(alternate(_, Left, Right)):
        CheckSyntaxMarks(Left)
        CheckSyntaxMarks(Right)

    'rule' CheckSyntaxMarks(repeat(_, Element)):
        CheckSyntaxMarks(Element)
        
    'rule' CheckSyntaxMarks(list(Position, Element, Delimiter)):
        CheckSyntaxMarks(Element)
        CheckSyntaxMarks(Delimiter)
        
    'rule' CheckSyntaxMarks(optional(_, Operand)):
        CheckSyntaxMarks(Operand)

    'rule' CheckSyntaxMarks(mark(Position, Variable, Value)):
        [|
            (|
                where(Value -> variable(_, _))
            ||
                where(Value -> indexedvariable(_, _, _))
            |)
            Error_SyntaxMarksMustBeConstant(Position)
        |]
        
    'rule' CheckSyntaxMarks(_):
        -- do nothing

----------

'action' CheckSyntaxMethods(SYNTAXCLASS, SYNTAXMETHODLIST)

    'rule' CheckSyntaxMethods(Class, methodlist(Head, Tail)):
        CheckSyntaxMethod(Class, Head)
        CheckSyntaxMethods(Class, Tail)
        
    'rule' CheckSyntaxMethods(_, nil):
        -- nothing
        
'action' CheckSyntaxMethod(SYNTAXCLASS, SYNTAXMETHOD)

    'rule' CheckSyntaxMethod(Class, method(Position, Name, Arguments)):
        QuerySymbolId(Name -> Info)
        Info'Type -> handler(_, signature(Parameters, ReturnType))
        Info'Access -> Access
        [|
            ne(Access, public)
            Error_HandlersBoundToSyntaxMustBePublic(Position)
        |]
        CheckSyntaxMethodReturnType(Position, Class, ReturnType)
        CheckSyntaxMethodArguments(Position, Parameters, Arguments)
        (|
            IsLValueSyntaxMethodBinding(Arguments)
            ComputeLModeOfSyntaxMethodArguments(Parameters, Arguments)
        ||
            ComputeRModeOfSyntaxMethodArguments(Parameters, Arguments)
        |)
        CheckSyntaxMethodCanBeBound(Position, Class, Parameters, ReturnType, Arguments)

    'rule' CheckSyntaxMethod(Class, method(Position, Name, Arguments)):
        -- fails if the id isn't a handler id (i.e. is error)
        
'action' CheckSyntaxMethodCanBeBound(POS, SYNTAXCLASS, PARAMETERLIST, TYPE, SYNTAXCONSTANTLIST)

    'rule' CheckSyntaxMethodCanBeBound(Position, phrase, Parameters, ReturnType, Arguments):
        (|
            CheckEquivalentSyntaxMethodArguments(-1, Parameters, Arguments)
        || 
            Error_SyntaxMethodArgumentsMustMatch(Position)
        |)
        
    'rule' CheckSyntaxMethodCanBeBound(Position, statement, Parameters, ReturnType, Arguments):
        (|
            CheckEquivalentSyntaxMethodArguments(-1, Parameters, Arguments)
        || 
            Error_SyntaxMethodArgumentsMustMatch(Position)
        |)

    'rule' CheckSyntaxMethodCanBeBound(Position, Class, Parameters, ReturnType, Arguments):
        (|
            where(Class -> expression)
        ||
            where(Class -> prefix(_))
        ||
            where(Class -> postfix(_))
        ||
            where(Class -> binary(_, _))
        |)
        (|
            IsFirstArgumentOfClass(Arguments, input)
            (|
                CheckEquivalentSyntaxMethodArguments(-1, Parameters, Arguments)
            ||
                Error_LSyntaxMethodArgumentsDontConform(Position)
            |)
        ||
            IsLastArgumentOfClass(Arguments, output)
            (|
                CheckEquivalentSyntaxMethodArguments(-1, Parameters, Arguments)
            ||
                Error_RSyntaxMethodArgumentsDontConform(Position)
            |)
        ||
            Error_ExpressionSyntaxMethodArgumentsDontConform(Position)
        |)

    'rule' CheckSyntaxMethodCanBeBound(Position, iterator, Parameters, ReturnType, Arguments):
        (|
            IsFirstArgumentOfClass(Arguments, iterator)
            IsLastArgumentOfClass(Arguments, container)
            CheckEquivalentSyntaxMethodArguments(-1, Parameters, Arguments)
        ||
            Error_IterateSyntaxMethodArgumentsDontConform(Position)
        |)
        

    'rule' CheckSyntaxMethodCanBeBound(Position, Class, Parameters, ReturnType, Arguments):
        print(Class)
        eq(0,1)
        
'condition' IsFirstArgumentOfClass(SYNTAXCONSTANTLIST, SYNTAXMARKTYPE)

    'rule' IsFirstArgumentOfClass(constantlist(variable(_, Id), Tail), WantedClass):
        QuerySyntaxMarkId(Id -> Info)
        Info'Type -> Class
        eq(Class, WantedClass)
        
'condition' IsLastArgumentOfClass(SYNTAXCONSTANTLIST, SYNTAXMARKTYPE)

    'rule' IsLastArgumentOfClass(constantlist(variable(_, Id), nil), WantedClass):
        QuerySyntaxMarkId(Id -> Info)
        Info'Type -> Class
        eq(Class, WantedClass)

    'rule' IsLastArgumentOfClass(constantlist(Head, Tail), Class):
        IsLastArgumentOfClass(Tail, Class)

-- Syntax method arguments are equivalent to parameters if the parameter list is bound to a
-- monotonically increasing list of index vars.
'condition' CheckEquivalentSyntaxMethodArguments(INT, PARAMETERLIST, SYNTAXCONSTANTLIST)

    'rule' CheckEquivalentSyntaxMethodArguments(Index, parameterlist(ParamHead, ParamTail), constantlist(variable(_, Id), ArgTail)):
        QuerySyntaxMarkId(Id -> Info)
        (|
            (|
                Info'Type -> input
            ||
                Info'Type -> output
            ||
                Info'Type -> iterator
            ||
                Info'Type -> container
            |)
            where(Index -> VarIndex)
        ||
            Info'Index -> VarIndex
            gt(VarIndex, Index)
        |)
        CheckEquivalentSyntaxMethodArguments(VarIndex, ParamTail, ArgTail)

    'rule' CheckEquivalentSyntaxMethodArguments(Index, nil, nil):
        --


'condition' IsLValueSyntaxMethodBinding(SYNTAXCONSTANTLIST)

    'rule' IsLValueSyntaxMethodBinding(constantlist(variable(_, Name), _))
        QuerySyntaxMarkId(Name -> Info)
        Info'Type -> input
        
    'rule' IsLValueSyntaxMethodBinding(constantlist(_, Rest)):
        IsLValueSyntaxMethodBinding(Rest)

'action' CheckSyntaxMethodReturnType(POS, SYNTAXCLASS, TYPE)

    'rule' CheckSyntaxMethodReturnType(Position, iterator, Type):
        (|
            where(Type -> boolean(_))
        ||
            where(Type -> named(_, Id))
            Id'Name -> Name
            IsNameEqualToString(Name, "CBool")
        ||
            Error_IterateSyntaxMethodMustReturnBoolean(Position)
        |)
        
    /*'rule' CheckSyntaxMethodReturnType(Position, phrase, Type):
        [|
            where(Type -> undefined(_))
            Error_PhraseSyntaxMethodMustReturnAValue(Position)
        |]*/

    'rule' CheckSyntaxMethodReturnType(Position, statement, _):
        -- statement syntax methods can return anything they want

    'rule' CheckSyntaxMethodReturnType(_, _, undefined(_)):
        -- other types must not return a value
        
    'rule' CheckSyntaxMethodReturnType(Position, _, _):
        Error_HandlersBoundToSyntaxMustNotReturnAValue(Position)

'action' CheckSyntaxMethodArguments(POS, PARAMETERLIST, SYNTAXCONSTANTLIST)

    'rule' CheckSyntaxMethodArguments(_, nil, nil):
        -- done
        
    'rule' CheckSyntaxMethodArguments(Position, nil, _):
        Error_TooManyArgumentsPassedToHandler(Position)
        
    'rule' CheckSyntaxMethodArguments(Position, _, nil):
        Error_TooFewArgumentsPassedToHandler(Position)
        
    'rule' CheckSyntaxMethodArguments(Position, parameterlist(Param, ParamRest), constantlist(Arg, ArgRest)):
        CheckSyntaxMethodArgument(Position, Param, Arg)
        CheckSyntaxMethodArguments(Position, ParamRest, ArgRest)
        
'action' CheckSyntaxMethodArgument(POS, PARAMETER, SYNTAXCONSTANT)

    'rule' CheckSyntaxMethodArgument(_, parameter(_, Mode, Name, _), Argument):
        ComputeSyntaxArgumentType(Argument -> ArgType)
        Argument'Position -> Position
        CheckSyntaxMethodArgumentMode(Position, ArgType, Mode)

'action' CheckSyntaxMethodArgumentMode(POS, SYNTAXMARKTYPE, MODE)

    'rule' CheckSyntaxMethodArgumentMode(Position, error, Mode):
        -- do nothing

    'rule' CheckSyntaxMethodArgumentMode(Position, expression, Mode):
        -- do nothing

    'rule' CheckSyntaxMethodArgumentMode(Position, Type, Mode):
        (|
            where(Type -> undefined)
        ||
            where(Type -> boolean)
        ||
            where(Type -> integer)
        ||
            where(Type -> real)
        ||
            where(Type -> string)
        |)
        [|
            ne(Mode, in)
            Error_ConstantSyntaxArgumentMustBindToInParameter(Position)
        |]
    
    'rule' CheckSyntaxMethodArgumentMode(Position, context, Mode):
        [|
            ne(Mode, in)
            Error_ContextSyntaxArgumentMustBindToInParameter(Position)
        |]

    'rule' CheckSyntaxMethodArgumentMode(Position, input, Mode):
        [|
            ne(Mode, in)
            Error_InputSyntaxArgumentMustBindToInParameter(Position)
        |]

    'rule' CheckSyntaxMethodArgumentMode(Position, output, Mode):
        [|
            ne(Mode, out)
            Error_OutputSyntaxArgumentMustBindToOutParameter(Position)
        |]
        
    'rule' CheckSyntaxMethodArgumentMode(Position, iterator, Mode):
        [|
            ne(Mode, inout)
            Error_IteratorSyntaxArgumentMustBindToInOutParameter(Position)
        |]
        
    'rule' CheckSyntaxMethodArgumentMode(Position, container, Mode):
        [|
            ne(Mode, in)
            Error_ContainerSyntaxArgumentMustBindToInParameter(Position)
        |]

    'rule' CheckSyntaxMethodArgumentMode(Position, phrase, Mode):
        [|
            ne(Mode, in)
            Error_PhraseBoundMarkSyntaxArgumentMustBindToInParameter(Position)
        |]

    'rule' CheckSyntaxMethodArgumentMode(Position, Type, Mode):
        print(Type)

----------

'sweep' CheckSyntaxKeywords(ANY)

    'rule' CheckSyntaxKeywords(SYNTAX'keyword(Position, Value)):
        (|
            IsStringSuitableForKeyword(Value)
        ||
            Error_UnsuitableStringForKeyword(Position, Value)
        |)

    'rule' CheckSyntaxKeywords(SYNTAX'unreservedkeyword(Position, Value)):
        (|
            IsStringSuitableForKeyword(Value)
        ||
            Error_UnsuitableStringForKeyword(Position, Value)
        |)

----------

'action' ComputeLModeOfSyntaxMethodArguments(PARAMETERLIST, SYNTAXCONSTANTLIST)

    'rule' ComputeLModeOfSyntaxMethodArguments(parameterlist(parameter(_, Mode, _, _), ParamRest), constantlist(Arg, ArgRest)):
        ComputeSyntaxMethodArgumentLMode(Arg, Mode)
        ComputeLModeOfSyntaxMethodArguments(ParamRest, ArgRest)
        
    'rule' ComputeLModeOfSyntaxMethodArguments(_, _):
        --

'action' ComputeRModeOfSyntaxMethodArguments(PARAMETERLIST, SYNTAXCONSTANTLIST)

    'rule' ComputeRModeOfSyntaxMethodArguments(parameterlist(parameter(_, Mode, _, _), ParamRest), constantlist(Arg, ArgRest)):
        ComputeSyntaxMethodArgumentRMode(Arg, Mode)
        ComputeRModeOfSyntaxMethodArguments(ParamRest, ArgRest)
        
    'rule' ComputeRModeOfSyntaxMethodArguments(_, _):
        --


'action' ComputeSyntaxMethodArgumentRMode(SYNTAXCONSTANT, MODE)

    'rule' ComputeSyntaxMethodArgumentRMode(variable(Position, Id), Mode):
        QuerySyntaxMarkId(Id -> Info)
        Info'Type -> expression
        Info'RMode -> CurrentMode
        (|
            eq(CurrentMode, Mode)
        ||
            eq(CurrentMode, uncomputed)
            Info'RMode <- Mode
        ||
            Error_VariableSyntaxArgumentMustBindToConsistentMode(Position)
        |)
        
    'rule' ComputeSyntaxMethodArgumentRMode(_, _):
        -- do nothing.

'action' ComputeSyntaxMethodArgumentLMode(SYNTAXCONSTANT, MODE)

    'rule' ComputeSyntaxMethodArgumentLMode(variable(Position, Id), Mode):
        QuerySyntaxMarkId(Id -> Info)
        Info'Type -> expression
        Info'LMode -> CurrentMode
        (|
            eq(CurrentMode, Mode)
        ||
            eq(CurrentMode, uncomputed)
            Info'LMode <- Mode
        ||
            Error_VariableSyntaxArgumentMustBindToConsistentMode(Position)
        |)
        
    'rule' ComputeSyntaxMethodArgumentLMode(_, _):
        -- do nothing.

'action' ComputeSyntaxPrefixAndSuffix(SYNTAX -> SYNTAXTERM, SYNTAXTERM)

    'rule' ComputeSyntaxPrefixAndSuffix(concatenate(_, Left, Right) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffix(Left -> LeftPrefix, LeftSuffix)
        ComputeSyntaxPrefixAndSuffix(Right -> RightPrefix, RightSuffix)
        (|
            SyntaxIsNullable(Left)
            UnionSyntaxTerm(LeftPrefix, RightPrefix -> Prefix)
        ||
            where(LeftPrefix -> Prefix)
        |)
        (|
            SyntaxIsNullable(Right)
            UnionSyntaxTerm(LeftSuffix, RightSuffix -> Suffix)
        ||
            where(RightSuffix -> Suffix)
        |)

    'rule' ComputeSyntaxPrefixAndSuffix(alternate(_, Left, Right) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffix(Left -> LeftPrefix, LeftSuffix)
        ComputeSyntaxPrefixAndSuffix(Right -> RightPrefix, RightSuffix)
        UnionSyntaxTerm(LeftPrefix, RightPrefix -> Prefix)
        UnionSyntaxTerm(LeftSuffix, RightSuffix -> Suffix)
        
    'rule' ComputeSyntaxPrefixAndSuffix(optional(_, Element) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffix(Element -> Prefix, Suffix)

    'rule' ComputeSyntaxPrefixAndSuffix(repeat(_, Element) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffix(Element -> Prefix, Suffix)

    'rule' ComputeSyntaxPrefixAndSuffix(list(_, Element, _) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffix(Element -> Prefix, Suffix)

    'rule' ComputeSyntaxPrefixAndSuffix(rule(_, Name) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffixOfRule(Name -> Prefix, Suffix)

    'rule' ComputeSyntaxPrefixAndSuffix(markedrule(_, _, Name) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffixOfRule(Name -> Prefix, Suffix)
        
    'rule' ComputeSyntaxPrefixAndSuffix(mark(_, _, _) -> mark, mark):
        -- do nothing

    'rule' ComputeSyntaxPrefixAndSuffix(_ -> keyword, keyword):
        -- do nothing

'action' ComputeSyntaxPrefixAndSuffixOfRule(ID -> SYNTAXTERM, SYNTAXTERM)

    'rule' ComputeSyntaxPrefixAndSuffixOfRule(Name -> Prefix, Suffix)
        QuerySyntaxId(Name -> Info)
        (|
            Info'Prefix -> undefined
            Info'Syntax -> Syntax
            ComputeSyntaxPrefixAndSuffix(Syntax -> Prefix, Suffix)
            Info'Prefix <- Prefix
            Info'Suffix <- Suffix
        ||
            Info'Prefix -> Prefix
            Info'Suffix -> Suffix
        |)
        
    'rule' ComputeSyntaxPrefixAndSuffixOfRule(_ -> error, error):
        -- do nothing

'action' UnionSyntaxTerm(SYNTAXTERM, SYNTAXTERM -> SYNTAXTERM)

    'rule' UnionSyntaxTerm(error, _ -> error):
    'rule' UnionSyntaxTerm(_, error -> error):
    'rule' UnionSyntaxTerm(mark, Right -> Right):
    'rule' UnionSyntaxTerm(Left, mark -> Left):
    'rule' UnionSyntaxTerm(Left, Right -> Left):
        eq(Left, Right)
    'rule' UnionSyntaxTerm(_, _ -> mixed):
        --

'condition' SyntaxIsNullable(SYNTAX)

    'rule' SyntaxIsNullable(concatenate(_, Left, Right)):
        SyntaxIsNullable(Left)
        SyntaxIsNullable(Right)
        
    'rule' SyntaxIsNullable(alternate(_, Left, Right)):
        (|
            SyntaxIsNullable(Left)
        ||
            SyntaxIsNullable(Right)
        |)

    'rule' SyntaxIsNullable(optional(_, Operand)):
        -- optionals are nullable

    'rule' SyntaxIsNullable(rule(_, Name)):
        QuerySyntaxOfSyntaxId(Name -> Syntax)
        SyntaxIsNullable(Syntax)
        
    'rule' SyntaxIsNullable(markedrule(_, _, Name)):
        QuerySyntaxOfSyntaxId(Name -> Syntax)
        SyntaxIsNullable(Syntax)

    'rule' SyntaxIsNullable(mark(_, _, _)):
        -- marks are nullable

--------------------------------------------------------------------------------

'var' IgnoredModulesList : NAMELIST

'condition' ImportContainsCanvas(IMPORT)

'sweep' CheckInvokes(ANY)

    'rule' CheckInvokes(MODULE'module(_, Kind, Name, Imports, Definitions)):
        (|
            ne(Kind, widget)
            (|
                ImportContainsCanvas(Imports)
                MakeNameLiteral("com.livecode.widget" -> WidgetModuleName)
                IgnoredModulesList <- namelist(WidgetModuleName, nil)
            ||
                MakeNameLiteral("com.livecode.widget" -> WidgetModuleName)
                MakeNameLiteral("com.livecode.canvas" -> CanvasModuleName)
                IgnoredModulesList <- namelist(CanvasModuleName, namelist(WidgetModuleName, nil))
            |)
        ||
            IgnoredModulesList <- nil
        |)

        CheckInvokes(Definitions)

    'rule' CheckInvokes(STATEMENT'call(Position, Handler, Arguments)):
        [|
            -- This call will fail if Handler is an untyped or generic handler type variable
            QueryHandlerIdSignature(Handler -> signature(Parameters, ReturnType))
            CheckCallArguments(Position, Parameters, Arguments)
            CheckInvokes(Arguments)
        |]

    'rule' CheckInvokes(STATEMENT'invoke(Position, Info, Arguments)):
        (|
            IsInvokeAllowed(Info)
            (|
                ComputeInvokeSignature(execute, Info, Arguments -> Signature)
                CheckInvokeArguments(Position, Signature, Arguments)
            ||
                Fatal_InternalInconsistency("No execute method for statement syntax")
            |)
            CheckInvokes(Arguments)
        ||
            Error_SyntaxNotAllowedInThisContext(Position)
        |)
        
    'rule' CheckInvokes(STATEMENT'put(Position, Source, Target)):
        CheckExpressionIsEvaluatable(Source)
        CheckExpressionIsAssignable(Target)
        CheckInvokes(Source)
        CheckInvokes(Target)

    'rule' CheckInvokes(STATEMENT'repeatcounted(Position, Count, Body)):
        CheckExpressionIsEvaluatable(Count)
        CheckInvokes(Count)
        CheckInvokes(Body)
        
    'rule' CheckInvokes(STATEMENT'repeatwhile(Position, Condition, Body)):
        CheckExpressionIsEvaluatable(Condition)
        CheckInvokes(Condition)
        CheckInvokes(Body)

    'rule' CheckInvokes(STATEMENT'repeatuntil(Position, Condition, Body)):
        CheckExpressionIsEvaluatable(Condition)
        CheckInvokes(Condition)
        CheckInvokes(Body)

    'rule' CheckInvokes(STATEMENT'repeatupto(Position, Slot, Start, Finish, Step, Body)):
        CheckExpressionIsEvaluatable(Start)
        CheckExpressionIsEvaluatable(Finish)
        CheckExpressionIsEvaluatable(Step)
        CheckInvokes(Start)
        CheckInvokes(Finish)
        CheckInvokes(Step)
        CheckInvokes(Body)

    'rule' CheckInvokes(STATEMENT'repeatdownto(Position, Slot, Start, Finish, Step, Body)):
        CheckExpressionIsEvaluatable(Start)
        CheckExpressionIsEvaluatable(Finish)
        CheckExpressionIsEvaluatable(Step)
        CheckInvokes(Start)
        CheckInvokes(Finish)
        CheckInvokes(Step)
        CheckInvokes(Body)

    'rule' CheckInvokes(STATEMENT'repeatforeach(Position, invoke(IteratorPosition, IteratorInvoke, IteratorArguments), Container, Body)):
        (|
            IsInvokeAllowed(IteratorInvoke)
            (|
                ComputeInvokeSignature(iterate, IteratorInvoke, IteratorArguments -> IteratorSignature)
                CheckInvokeArguments(IteratorPosition, IteratorSignature, IteratorArguments)
            ||
                Fatal_InternalInconsistency("No iterate method for repeat for each syntax")
            |)
            CheckInvokes(IteratorArguments)
        ||
            Error_SyntaxNotAllowedInThisContext(Position)
        |)
        CheckExpressionIsEvaluatable(Container)
        CheckInvokes(IteratorArguments)
        CheckInvokes(Container)
        CheckInvokes(Body)
        
    'rule' CheckInvokes(STATEMENT'return(Position, Value)):
        [|
            ne(Value, nil)
            CheckExpressionIsEvaluatable(Value)
            CheckInvokes(Value)
        |]

    'rule' CheckInvokes(STATEMENT'throw(Position, Error)):
        CheckExpressionIsEvaluatable(Error)
        CheckInvokes(Error)

    'rule' CheckInvokes(EXPRESSION'call(Position, Handler, Arguments)):
        [|
            -- This call will fail if Handler is an untyped or generic handler type variable
            QueryHandlerIdSignature(Handler -> signature(Parameters, ReturnType))
            CheckCallArguments(Position, Parameters, Arguments)
            CheckInvokes(Arguments)
        |]
    
    'rule' CheckInvokes(EXPRESSION'invoke(Position, Info, Arguments)):
        (|
            IsInvokeAllowed(Info)
            (|
                ComputeInvokeSignature(evaluate, Info, Arguments -> Signature)
                CheckInvokeArguments(Position, Signature, Arguments)
            ||
                Error_NonEvaluatableExpressionUsedForInContext(Position)
            |)
            CheckInvokes(Arguments)
        ||
            Error_SyntaxNotAllowedInThisContext(Position)
        |)

    'rule' CheckInvokes(EXPRESSION'logicaland(Position, Left, Right)):
        CheckExpressionIsEvaluatable(Left)
        CheckExpressionIsEvaluatable(Right)
        CheckInvokes(Left)
        CheckInvokes(Right)

    'rule' CheckInvokes(EXPRESSION'logicalor(Position, Left, Right)):
        CheckExpressionIsEvaluatable(Left)
        CheckExpressionIsEvaluatable(Right)
        CheckInvokes(Left)
        CheckInvokes(Right)


'action' CheckCallArguments(POS, PARAMETERLIST, EXPRESSIONLIST)

    'rule' CheckCallArguments(_, nil, nil):
        -- done!

    'rule' CheckCallArguments(Position, parameterlist(parameter(_, Mode, _, _), ParamRest), expressionlist(Argument, ArgRest)):
        [|
            ne(Mode, out)
            CheckExpressionIsEvaluatable(Argument)
        |]
        [|
            ne(Mode, in)
            CheckExpressionIsAssignable(Argument)
        |]
        CheckCallArguments(Position, ParamRest, ArgRest)

    'rule' CheckCallArguments(Position, nil, _):
        Error_TooManyArgumentsPassedToHandler(Position)
        
    'rule' CheckCallArguments(Position, _, nil):
        Error_TooFewArgumentsPassedToHandler(Position)

'action' CheckInvokeArguments(POS, INVOKESIGNATURE, EXPRESSIONLIST)

    'rule' CheckInvokeArguments(_, nil, _):
        -- done!
        
    'rule' CheckInvokeArguments(Position, invokesignature(Mode, Index, SigTail), Arguments):
        eq(Index, -1)
        CheckInvokeArguments(Position, SigTail, Arguments)
        
    'rule' CheckInvokeArguments(Position, invokesignature(Mode, Index, SigTail), Arguments):
        GetExpressionAtIndex(Arguments, Index -> Argument)
        [|
            ne(Mode, out)
            CheckExpressionIsEvaluatable(Argument)
        |]
        [|
            ne(Mode, in)
            CheckExpressionIsAssignable(Argument)
        |]
        CheckInvokeArguments(Position, SigTail, Arguments)

'action' CheckExpressionIsEvaluatable(EXPRESSION)

    'rule' CheckExpressionIsEvaluatable(invoke(Position, Info, Arguments)):
        (|
            IsInvokeAllowed(Info)
            (|
                ComputeInvokeSignature(evaluate, Info, Arguments -> Signature)
                CheckInvokeArguments(Position, Signature, Arguments)
            ||
                Error_NonEvaluatableExpressionUsedForInContext(Position)
            |)
        ||
            Error_SyntaxNotAllowedInThisContext(Position)
        |)
        
    'rule' CheckExpressionIsEvaluatable(_):
        -- everything else can be evaluated
        
'action' CheckExpressionIsAssignable(EXPRESSION)

    'rule' CheckExpressionIsAssignable(invoke(Position, Info, Arguments)):
        (|
            IsInvokeAllowed(Info)
            (|
                ComputeInvokeSignature(assign, Info, Arguments -> Signature)
                CheckInvokeArguments(Position, Signature, Arguments)
            ||
                Error_NonAssignableExpressionUsedForOutContext(Position)
            |)
        ||
            Error_SyntaxNotAllowedInThisContext(Position)
        |)
        
    'rule' CheckExpressionIsAssignable(slot(Position, Id)):
        (|
            QueryKindOfSymbolId(Id -> handler)
            Id'Name -> Name
            Error_CannotAssignToHandlerId(Position, Name)
        ||
            QueryKindOfSymbolId(Id -> constant)
            Id'Name -> Name
            Error_CannotAssignToConstantId(Position, Name)
        ||
        |)
        
    'rule' CheckExpressionIsAssignable(Expr):
        -- everything else is not
        GetExpressionPosition(Expr -> Position)
        Error_NonAssignableExpressionUsedForOutContext(Position)

'condition' IsInvokeAllowed(INVOKELIST)
    'rule' IsInvokeAllowed(invokelist(Head, _)):
        IsInvokeModuleAllowed(Head)
    'rule' IsInvokeAllowed(invokelist(_, Tail)):
        IsInvokeAllowed(Tail)

'condition' IsInvokeModuleAllowed(INVOKEINFO)
    'rule' IsInvokeModuleAllowed(Info):
        Info'ModuleName -> NameString
        MakeNameLiteral(NameString -> Name)
        IgnoredModulesList -> Modules
        IsNameNotInList(Name, Modules)

'condition' ComputeInvokeSignature(INVOKEMETHODTYPE, INVOKELIST, EXPRESSIONLIST -> INVOKESIGNATURE)

    'rule' ComputeInvokeSignature(Type, invokelist(Head, Tail), Arguments -> Signature)
        IsInvokeModuleAllowed(Head)
        Head'Methods -> Methods
        (|
            ComputeInvokeSignatureForMethods(Type, Methods, Arguments -> Signature)
        ||
            ComputeInvokeSignature(Type, Tail, Arguments -> Signature)
        |)
        
'condition' ComputeInvokeSignatureForMethods(INVOKEMETHODTYPE, INVOKEMETHODLIST, EXPRESSIONLIST -> INVOKESIGNATURE)

    'rule' ComputeInvokeSignatureForMethods(WantType, methodlist(_, IsType, Signature, Tail), Arguments -> OutSig):
        (|
            eq(WantType, IsType)
            AreAllArgumentsDefinedForInvokeMethod(Arguments, Signature)
            CountDefinedArguments(Arguments -> ArgCount)
            CountInvokeParameters(Signature -> ParamCount)
            eq(ArgCount, ParamCount)
            where(Signature -> OutSig)
        ||
            ComputeInvokeSignatureForMethods(WantType, Tail, Arguments -> OutSig)
        |)

'condition' AreAllArgumentsDefinedForInvokeMethod(EXPRESSIONLIST, INVOKESIGNATURE)
'action' CountInvokeParameters(INVOKESIGNATURE -> INT)
'action' CountDefinedArguments(EXPRESSIONLIST -> INT)
'action' GetExpressionAtIndex(EXPRESSIONLIST, INT -> EXPRESSION)
'condition' IsNameNotInList(NAME, NAMELIST)

'action' GetExpressionPosition(EXPRESSION -> POS)
    'rule' GetExpressionPosition(undefined(Position) -> Position):
    'rule' GetExpressionPosition(true(Position) -> Position):
    'rule' GetExpressionPosition(false(Position) -> Position):
    'rule' GetExpressionPosition(unsignedinteger(Position, _) -> Position):
    'rule' GetExpressionPosition(integer(Position, _) -> Position):
    'rule' GetExpressionPosition(real(Position, _) -> Position):
    'rule' GetExpressionPosition(string(Position, _) -> Position):
    'rule' GetExpressionPosition(slot(Position, _) -> Position):
    'rule' GetExpressionPosition(logicaland(Position, _, _) -> Position):
    'rule' GetExpressionPosition(logicalor(Position, _, _) -> Position):
    'rule' GetExpressionPosition(as(Position, _, _) -> Position):
    'rule' GetExpressionPosition(list(Position, _) -> Position):
    'rule' GetExpressionPosition(call(Position, _, _) -> Position):
    'rule' GetExpressionPosition(invoke(Position, _, _) -> Position):
    'rule' GetExpressionPosition(nil -> Position)
        GetUndefinedPosition(-> Position)

--------------------------------------------------------------------------------

-- TODO: Remove - we don't restrict types anymore - caveat coder!
'sweep' CheckDeclaredTypes(ANY)

    'rule' CheckDeclaredTypes(DEFINITION'variable(Position, _, _, Type)):
        -- Variable types must be high-level
        (|
            IsHighLevelType(Type)
        ||
            --Error_VariableMustHaveHighLevelType(Position)
        |)
        
    'rule' CheckDeclaredTypes(DEFINITION'contextvariable(Position, _, _, Type, _)):
        -- Variable types must be high-level
        (|
            IsHighLevelType(Type)
        ||
            --Error_VariableMustHaveHighLevelType(Position)
        |)

    'rule' CheckDeclaredTypes(DEFINITION'foreignhandler(_, _, _, Signature, _)):
        -- Foreign handler signatures can contain any type so no need to
        -- check anything here.
        
    'rule' CheckDeclaredTypes(PARAMETER'parameter(Position, _, _, Type)):
        (|
            IsHighLevelType(Type)
        ||
            --Error_ParameterMustHaveHighLevelType(Position)
        |)
        
    'rule' CheckDeclaredTypes(STATEMENT'variable(Position, _, Type)):
        -- Variable types must be high-level
        (|
            IsHighLevelType(Type)
        ||
            --Error_VariableMustHaveHighLevelType(Position)
        |)

'condition' IsHighLevelType(TYPE)

    'rule' IsHighLevelType(any(_)):
    'rule' IsHighLevelType(undefined(_)):
    'rule' IsHighLevelType(foreign(_, _)):
    'rule' IsHighLevelType(named(_, Id)):
        (|
            QuerySymbolId(Id -> Info)
            Info'Type -> Type
            IsHighLevelType(Type)
        || 
            QueryId(Id -> error)
        |)
    'rule' IsHighLevelType(optional(_, Type)):
        IsHighLevelType(Type)
    'rule' IsHighLevelType(handler(_, _)):
    'rule' IsHighLevelType(record(_, _, _)):
    'rule' IsHighLevelType(boolean(_)):
    'rule' IsHighLevelType(integer(_)):
    'rule' IsHighLevelType(real(_)):
    'rule' IsHighLevelType(number(_)):
    'rule' IsHighLevelType(string(_)):
    'rule' IsHighLevelType(data(_)):
    'rule' IsHighLevelType(array(_)):
    'rule' IsHighLevelType(list(_, _)):

--------------------------------------------------------------------------------

-- Emit warnings if there are any potential problems with the
-- identifiers that are declared in the module.  In particular, warn
-- if there are any possible ambiguities with syntax keywords.

-- This sweep only covers the places where identifiers are defined.
-- This prevents warnings from being emitted everywhere that an
-- identifier is used.  It also prevents warnings about identifiers
-- defined in a different module, which the author of the current
-- module might not be able to do anything about.

'sweep' CheckIdentifiers(ANY)

    'rule' CheckIdentifiers(MODULE'module(_, _, Id, Imports, Definitions)):
        CheckIdIsSuitableForDefinition(Id)
        CheckIdentifiers(Imports)
        CheckIdentifiers(Definitions)

    --

    'rule' CheckIdentifiers(DEFINITION'type(_, _, Id, Type)):
        CheckIdIsSuitableForDefinition(Id)
        CheckIdentifiers(Type)

    'rule' CheckIdentifiers(DEFINITION'constant(_, _, Id, Value)):
        CheckIdIsSuitableForDefinition(Id)
        CheckIdentifiers(Value)

    'rule' CheckIdentifiers(DEFINITION'variable(_, _, Id, _)):
        CheckIdIsSuitableForDefinition(Id)

    'rule' CheckIdentifiers(DEFINITION'contextvariable(_, _, Id, Type, _)):
        CheckIdIsSuitableForDefinition(Id)
        CheckIdentifiers(Type)

    'rule' CheckIdentifiers(DEFINITION'handler(_, _, Id, _, Signature, Definitions, Body)):
        CheckIdIsSuitableForDefinition(Id)
        CheckIdentifiers(Signature)
        CheckIdentifiers(Definitions)
        CheckIdentifiers(Body)

    'rule' CheckIdentifiers(DEFINITION'foreignhandler(_, _, Id, Signature, _)):
        CheckIdIsSuitableForDefinition(Id)
        CheckIdentifiers(Signature)

    'rule' CheckIdentifiers(DEFINITION'property(_, _, Id, _, _)):
        CheckIdIsSuitableForDefinition(Id)

    'rule' CheckIdentifiers(DEFINITION'event(_, _, Id, Signature)):
        CheckIdIsSuitableForDefinition(Id)
        CheckIdentifiers(Signature)

    'rule' CheckIdentifiers(DEFINITION'syntax(_, _, Id, _, _, _)):
        CheckIdIsSuitableForDefinition(Id)

    --

    'rule' CheckIdentifiers(PARAMETER'parameter(_, _, Id, _)):
        CheckIdIsSuitableForDefinition(Id)

    --

    'rule' CheckIdentifiers(STATEMENT'variable(_, Id, _)):
        CheckIdIsSuitableForDefinition(Id)

'action' CheckIdIsSuitableForDefinition(ID)

    'rule' CheckIdIsSuitableForDefinition(Id):
        Id'Name -> Name
        Id'Position -> Position
        (|
            IsNameSuitableForDefinition(Name)
        ||
            Warning_UnsuitableNameForDefinition(Position, Name)
        |)

--------------------------------------------------------------------------------

'condition' QueryHandlerIdSignature(ID -> SIGNATURE)

    'rule' QueryHandlerIdSignature(Id -> Signature)
        QueryId(Id -> symbol(Info))
        Info'Kind -> handler
        Info'Type -> handler(_, Signature)
        
    'rule' QueryHandlerIdSignature(Id -> Signature)
        QueryId(Id -> symbol(Info))
        Info'Kind -> variable
        Info'Type -> Type
        FullyResolveType(Type -> handler(_, Signature))

'condition' QueryKindOfSymbolId(ID -> SYMBOLKIND)

    'rule' QueryKindOfSymbolId(Id -> Kind):
        QueryId(Id -> Meaning)
        where(Meaning -> symbol(Info))
        Info'Kind -> Kind
        
'condition' QueryClassOfSyntaxId(ID -> SYNTAXCLASS)

    'rule' QueryClassOfSyntaxId(Id -> Class):
        QueryId(Id -> Meaning)
        where(Meaning -> syntax(Info))
        Info'Class -> Class

'condition' QuerySyntaxOfSyntaxId(ID -> SYNTAX)

    'rule' QuerySyntaxOfSyntaxId(Id -> Syntax):
        QueryId(Id -> Meaning)
        where(Meaning -> syntax(Info))
        Info'Class -> Class
        ne(Class, expressionphrase)
        ne(Class, expressionlistphrase)
        Info'Syntax -> Syntax

'condition' QueryTypeOfSyntaxMarkId(ID -> SYNTAXMARKTYPE)

    'rule' QueryTypeOfSyntaxMarkId(Id -> Type):
        QueryId(Id -> Meaning)
        where(Meaning -> syntaxmark(Info))
        Info'Type -> Type

'condition' QuerySyntaxId(ID -> SYNTAXINFO)

    'rule' QuerySyntaxId(Id -> Info):
        QueryId(Id -> syntax(Info))

'condition' QuerySyntaxMarkId(ID -> SYNTAXMARKINFO)

    'rule' QuerySyntaxMarkId(Id -> Info):
        QueryId(Id -> syntaxmark(Info))

'action' QueryId(ID -> MEANING)

    'rule' QueryId(Id -> Meaning):
        Id'Meaning -> definingid(DefId)
        DefId'Meaning -> Meaning
        
    'rule' QueryId(Id -> Meaning):
        Id'Meaning -> Meaning
        
'condition' QuerySymbolId(ID -> SYMBOLINFO)

--------------------------------------------------------------------------------
