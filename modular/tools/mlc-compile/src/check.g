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
        
        -- Check the syntax definitions are all correct.
        CheckSyntaxDefinitions(Module)

--------------------------------------------------------------------------------

-- At this point all identifiers either have a defined meaning, or are defined
-- to be a pointer to the definingid. The next step is to check that bindings
-- are appropriate to each id:
--   BD1) DEFINITION'constant(Value) - all id's in Value must be constant.
--
--   BT1) TYPE'named(Id) - Id must be bound to a type.
--   BT2) TYPE'opaque(FIELD'action(Handler)) - Handler must be bound to a handler.
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
        /* BD1 */ CheckBindingsOfConstantExpression(Value)

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

    'rule' CheckBindings(STATEMENT'repeatforeach(_, Iterator, Slot, Container, Body)):
        /* BS3 */ CheckBindingIsVariableId(Slot)
        CheckBindings(Iterator)
        CheckBindings(Container)
        CheckBindings(Body)

    'rule' CheckBindings(STATEMENT'call(_, Handler, Arguments)):
        /* B4 */ CheckBindingIsVariableOrHandlerId(Handler)
        CheckBindings(Arguments)

    --

    'rule' CheckBindings(EXPRESSION'slot(_, Name)):
        print("check slot")
        /* BE1 */ CheckBindingIsVariableOrHandlerId(Name)

    'rule' CheckBindings(EXPRESSION'call(_, Handler, Arguments)):
        /* BE2 */ CheckBindingIsVariableOrHandlerId(Handler)
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
        QueryId(Id -> type)
        
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
        QueryId(Id -> handler)
        
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
        QueryId(Id -> variable)
        
    'rule' CheckBindingIsVariableId(Id):
        QueryId(Id -> parameter)

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
        QueryId(Id -> variable)
        
    'rule' CheckBindingIsVariableOrHandlerId(Id):
        QueryId(Id -> parameter)

    'rule' CheckBindingIsVariableOrHandlerId(Id):
        QueryId(Id -> handler)

    'rule' CheckBindingIsVariableOrHandlerId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAVariableOrHandler(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

'action' CheckBindingIsSyntaxRuleOfExpressionType(ID)

    'rule' CheckBindingIsSyntaxRuleOfExpressionType(Id):
        -- Do nothing if the meaning is error.
        QueryId(Id -> error)

    'rule' CheckBindingIsSyntaxRuleOfExpressionType(Id):
        QueryId(Id -> syntaxexpressionrule)

    'rule' CheckBindingIsSyntaxRuleOfExpressionType(Id):
        QueryId(Id -> syntaxexpressionlistrule)
        
    'rule' CheckBindingIsSyntaxRuleOfExpressionType(Id):
        QueryId(Id -> syntaxrule(Info))
        Info'Class -> phrase
        
    'rule' CheckBindingIsSyntaxRuleOfExpressionType(Id):
        QueryId(Id -> syntaxrule(_))
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
        QueryId(Id -> syntaxmark(_))
        
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

    'rule' CheckBindingIsConstantSyntaxValue(_, string(_, _)):

    'rule' CheckBindingIsConstantSyntaxValue(Id, _):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAConstantSyntaxValue(Position, Name)

'action' CheckBindingIsSyntaxMarkUse(ID)

    'rule' CheckBindingIsSyntaxMarkUse(Id):
        QueryId(Id -> error)

    'rule' CheckBindingIsSyntaxMarkUse(Id):
        QueryId(Id -> syntaxmark(_))

    'rule' CheckBindingIsSyntaxMarkUse(Id):
        QueryId(Id -> syntaxoutputmark)

    'rule' CheckBindingIsSyntaxMarkUse(Id):
        QueryId(Id -> syntaxinputmark)

    'rule' CheckBindingIsSyntaxMarkUse(Id):
        QueryId(Id -> syntaxcontextmark)
        
    'rule' CheckBindingIsSyntaxMarkUse(Id):
        QueryId(Id -> syntaxcontainermark)

    'rule' CheckBindingIsSyntaxMarkUse(Id):
        QueryId(Id -> syntaxiteratormark)

    'rule' CheckBindingIsSyntaxMarkUse(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToASyntaxMark(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

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
--       e) A parameter bound to a constant mark must be of 'in' mode.
--       f) A parameter bound to a phrase (descent) mark must be of 'in' mode.
--       h) A parameter bound to context must be of 'in' mode.
--       b) A parameter bound to input must be of 'in' mode.
--       c) A parameter bound to output must be of 'out' mode.
--       *i) A method bound to syntax must return nothing.
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
        ComputeSyntaxConstantType(Constant -> Type)
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
            where(MarkType -> undefined)
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

    'rule' ComputeSyntaxRuleType(Id -> error):
        QueryId(Id -> syntaxexpressionrule)

    'rule' ComputeSyntaxRuleType(Id -> error):
        QueryId(Id -> syntaxexpressionlistrule)
        
    'rule' ComputeSyntaxRuleType(Id -> phrase):
        QueryId(Id -> syntaxrule(Info))
        Info'Class -> phrase
        
    'rule' ComputeSyntaxRuleType(Id -> expression):
        QueryId(Id -> syntaxrule(Info))
        Info'Class -> expression
        
    'rule' ComputeSyntaxRuleType(Id -> error):
        QueryId(Id -> Meaning)
        Fatal_InternalInconsistency("Referenced syntax rule bound to non-expression")

'action' ComputeSyntaxConstantType(SYNTAXCONSTANT -> SYNTAXMARKTYPE)

    'rule' ComputeSyntaxConstantType(true(_) -> boolean):
        --

    'rule' ComputeSyntaxConstantType(false(_) -> boolean):
        --

    'rule' ComputeSyntaxConstantType(integer(_, _) -> integer):
        --
        
    'rule' ComputeSyntaxConstantType(string(_, _) -> string):
        --

    'rule' ComputeSyntaxConstantType(Foo -> error):
        print(Foo)
        Fatal_InternalInconsistency("Non-constant syntax value present in constant context")

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
        QueryMethodIdSignature(Name -> signature(Parameters, ReturnType))
        CheckSyntaxMethodReturnType(Position, ReturnType)
        CheckSyntaxMethodArguments(Position, Parameters, Arguments)

'action' CheckSyntaxMethodReturnType(POS, TYPE)

    'rule' CheckSyntaxMethodReturnType(_, undefined(_)):
        -- no type is fine
        
    'rule' CheckSyntaxMethodReturnType(Position, _):
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

    'rule' CheckSyntaxMethodArgument(_, parameter(_, Mode, Name, Type), Argument):
        --

/*'action' CheckSyntaxMethodArgumentMode(ID, MODE, SYNTAXCONSTANT)

    'rule' CheckSyntaxMethodArgumentMode(_, in, undefined(_)):
        -- fine

    'rule' CheckSyntaxMethodArgumentMode(_, in, true(_)):
        -- fine

    'rule' CheckSyntaxMethodArgumentMode(_, in, false(_)):
        -- fine

    'rule' CheckSyntaxMethodArgumentMode(_, in, integer(_, _)):
        -- fine

    'rule' CheckSyntaxMethodArgumentMode(_, in, string(_, _)):
        -- fine

    'rule' CheckSyntaxMethodArgumentMode(_, in, variable(_, Name)):
        QueryId(Name -> syntaxinputmark)

    'rule' CheckSyntaxMethodArgumentMode(_, in, variable(_, Name)):
        QueryId(Name -> syntaxcontextmark)

    'rule' CheckSyntaxMethodArgumentMode(_, out, variable(_, Name)):
        QueryId(Name -> syntaxoutputmark)
        
    'rule' CheckSyntaxMethodArgumentMode(_, in, variable(_, Name)):
        --QueryId(Name -> syntaxrule(_))*/

----------

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
        QueryId(Name -> Meaning)
        (|
            (|
                where(Meaning -> syntaxexpressionrule)
            ||
                where(Meaning -> syntaxexpressionlistrule)
            |)
            where(SYNTAXTERM'expression -> Prefix)
            where(SYNTAXTERM'expression -> Suffix)
        ||
            where(Meaning -> syntaxrule(Info))
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
        ||
            where(SYNTAXTERM'error -> Prefix)
            where(SYNTAXTERM'error -> Suffix)
        |)

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
        QueryId(Name -> syntaxrule(Info))
        Info'Syntax -> Syntax
        SyntaxIsNullable(Syntax)
        
    'rule' SyntaxIsNullable(markedrule(_, _, Name)):
        QueryId(Name -> syntaxrule(Info))
        Info'Syntax -> Syntax
        SyntaxIsNullable(Syntax)

    'rule' SyntaxIsNullable(mark(_, _, _)):
        -- marks are nullable

--------------------------------------------------------------------------------

'action' QueryMethodIdSignature(ID -> SIGNATURE)

    'rule' QueryMethodIdSignature(Id -> Signature)
        QueryId(Id -> handler(Signature))

'action' QueryId(ID -> MEANING)

    'rule' QueryId(Id -> Meaning):
        Id'Meaning -> definingid(DefId)
        DefId'Meaning -> Meaning
        
    'rule' QueryId(Id -> Meaning):
        Id'Meaning -> Meaning

--------------------------------------------------------------------------------
