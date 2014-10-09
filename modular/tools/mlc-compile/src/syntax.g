'module' syntax

'use'
    types
    support

'export'
    InitializeSyntax
    GenerateSyntax

--------------------------------------------------------------------------------

'action' InitializeSyntax

    'rule' InitializeSyntax:
        -- do nothing

'sweep' GenerateSyntax(ANY)

    -- Handle the top-level definition
    'rule' GenerateSyntax(DEFINITION'syntax(_, _, Id, Class, Syntax, Methods)):
        Id'Name -> Name
        (|
            where(Class -> statement)
            BeginStatementSyntaxRule(Name)
        ||
            where(Class -> expression)
            BeginExpressionSyntaxRule(Name)
        ||
            where(Class -> unary(left, Precedence))
            BeginLeftUnaryOperatorSyntaxRule(Name, Precedence)
        ||
            where(Class -> unary(right, Precedence))
            BeginRightUnaryOperatorSyntaxRule(Name, Precedence)
        ||
            where(Class -> binary(left, Precedence))
            BeginLeftBinaryOperatorSyntaxRule(Name, Precedence)
        ||
            where(Class -> binary(right, Precedence))
            BeginRightBinaryOperatorSyntaxRule(Name, Precedence)
        ||
            where(Class -> binary(neutral, Precedence))
            BeginNeutralBinaryOperatorSyntaxRule(Name, Precedence)
        ||
            Fatal_InternalInconsistency("'phrase' syntax rules not implemented")
        |)

        BeginSyntaxGrammar()
        GenerateSyntax(Syntax)
        EndSyntaxGrammar()
        
        BeginSyntaxMappings()
        GenerateSyntax(Methods)
        EndSyntaxMappings()

        EndSyntaxRule()
        
    -- Handle the syntax grammar part of the definition
    'rule' GenerateSyntax(SYNTAX'concatenate(_, Left, Right)):
        GenerateSyntax(Left)
        GenerateSyntax(Right)
        ConcatenateSyntaxGrammar()

    'rule' GenerateSyntax(SYNTAX'alternate(_, Left, Right)):
        GenerateSyntax(Left)
        GenerateSyntax(Right)
        AlternateSyntaxGrammar()

    'rule' GenerateSyntax(SYNTAX'optional(_, Operand)):
        GenerateSyntax(Operand)
        PushEmptySyntaxGrammar()
        AlternateSyntaxGrammar()

    'rule' GenerateSyntax(SYNTAX'repeat(_, Element)):
        GenerateSyntax(Element)
        PushEmptySyntaxGrammar()
        RepeatSyntaxGrammar()
        
    'rule' GenerateSyntax(SYNTAX'list(_, Element, Delimiter)):
        GenerateSyntax(Element)
        GenerateSyntax(Delimiter)
        RepeatSyntaxGrammar()
        
    'rule' GenerateSyntax(SYNTAX'keyword(_, Value)):
        PushKeywordSyntaxGrammar(Value)

    'rule' GenerateSyntax(SYNTAX'markedrule(_, Variable, Id)):
        Id'Name -> Name
        Variable'Meaning -> syntaxmark(Index)
        PushMarkedDescentSyntaxGrammar(Index, Name)
        
    'rule' GenerateSyntax(SYNTAX'rule(_, Id)):
        Id'Name -> Name
        PushDescentSyntaxGrammar(Name)
        
    'rule' GenerateSyntax(SYNTAX'mark(_, Variable, Value)):
        Variable'Meaning -> syntaxmark(Index)
        (|
            where(Value -> true(_))
            PushMarkedTrueSyntaxGrammar(Index)
        ||
            where(Value -> false(_))
            PushMarkedFalseSyntaxGrammar(Index)
        ||
            where(Value -> integer(_, IntValue))
            PushMarkedIntegerSyntaxGrammar(Index, IntValue)
        ||
            where(Value -> string(_, StringValue))
            PushMarkedStringSyntaxGrammar(Index, StringValue)
        ||
            Fatal_InternalInconsistency("invalid constant type for marked variable")
        |)

    -- Handle the syntax mapping part of the definition
    'rule' GenerateSyntax(SYNTAXMETHOD'method(_, Id, Arguments)):
        Id'Name -> Name
        BeginMethodSyntaxMapping(Name)
        GenerateSyntax(Arguments)
        EndMethodSyntaxMapping()
        
    'rule' GenerateSyntax(SYNTAXCONSTANT'undefined(_)):
        PushUndefinedArgumentSyntaxMapping()
        
    'rule' GenerateSyntax(SYNTAXCONSTANT'true(_)):
        PushTrueArgumentSyntaxMapping()
        
    'rule' GenerateSyntax(SYNTAXCONSTANT'false(_)):
        PushFalseArgumentSyntaxMapping()

    'rule' GenerateSyntax(SYNTAXCONSTANT'integer(_, Value)):
        PushIntegerArgumentSyntaxMapping(Value)

    'rule' GenerateSyntax(SYNTAXCONSTANT'string(_, Value)):
        PushStringArgumentSyntaxMapping(Value)

    'rule' GenerateSyntax(SYNTAXCONSTANT'name(_, Value)):
        Value'Meaning -> syntaxmark(Index)
        PushMarkArgumentSyntaxMapping(Index)

--------------------------------------------------------------------------------
