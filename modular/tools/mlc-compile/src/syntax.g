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

        BeginSyntaxRuleGrammar()
        GenerateSyntax(Syntax)
        EndSyntaxRuleGrammar()
        
        BeginSyntaxRuleMappings()
        GenerateSyntax(Methods)
        EndSyntaxRuleMappings()

        EndSyntaxRule()
        
    'rule' GenerateSyntax(SYNTAX'concatenate(_, Left, Right)):
        GenerateSyntax(Left)
        GenerateSyntax(Right)
        ConcatenateSyntaxRule()

    'rule' GenerateSyntax(SYNTAX'alternate(_, Left, Right)):
        GenerateSyntax(Left)
        GenerateSyntax(Right)
        AlternateSyntaxRule()

    'rule' GenerateSyntax(SYNTAX'optional(_, Operand)):
        GenerateSyntax(Operand)
        PushEmptySyntaxRule()
        AlternateSyntaxRule()

    'rule' GenerateSyntax(SYNTAX'repeat(_, Element)):
        GenerateSyntax(Element)
        PushEmptySyntaxRule()
        RepeatSyntaxRule()
        
    'rule' GenerateSyntax(SYNTAX'list(_, Element, Delimiter)):
        GenerateSyntax(Element)
        GenerateSyntax(Delimiter)
        RepeatSyntaxRule()
        
    'rule' GenerateSyntax(SYNTAX'keyword(_, Value)):
        PushKeywordSyntaxRule(Value)

    'rule' GenerateSyntax(SYNTAX'markedrule(_, Variable, Id)):
        Id'Name -> Name
        Variable'Meaning -> syntaxmark(Index)
        PushMarkedDescentSyntaxRule(Index, Name)
        
    'rule' GenerateSyntax(SYNTAX'rule(_, Id)):
        Id'Name -> Name
        PushDescentSyntaxRule(Name)
        
    'rule' GenerateSyntax(SYNTAX'mark(_, Variable, Value)):
        Variable'Meaning -> syntaxmark(Index)
        (|
            where(Value -> true(_))
            PushMarkedTrueSyntaxRule(Index)
        ||
            where(Value -> false(_))
            PushMarkedFalseSyntaxRule(Index)
        ||
            where(Value -> integer(_, IntValue))
            PushMarkedIntegerSyntaxRule(Index, IntValue)
        ||
            where(Value -> string(_, StringValue))
            PushMarkedStringSyntaxRule(Index, StringValue)
        ||
            Fatal_InternalInconsistency("invalid constant type for marked variable")
        |)

--------------------------------------------------------------------------------
