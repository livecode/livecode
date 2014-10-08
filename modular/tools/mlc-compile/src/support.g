'module' support

'use'
    types

'export'
    IsBootstrapCompile

    InitializePosition
    FinalizePosition
    AdvanceCurrentPosition
    AdvanceCurrentPositionToNextLine
    GetColumnOfCurrentPosition

    InitializeLiterals
    FinalizeLiterals
    MakeIntegerLiteral
    MakeDoubleLiteral
    MakeStringLiteral
    MakeNameLiteral

    InitializeScopes
    FinalizeScopes
    DumpScopes
    EnterScope
    LeaveScope
    DefineMeaning
    UndefineMeaning
    HasLocalMeaning
    HasMeaning

    PushEmptySet
    DuplicateSet
    ExchangeSet
    UnionSet
    IsIndexInSet
    IncludeIndexInSet
    ExcludeIndexFromSet

    BeginStatementSyntaxRule
    BeginExpressionSyntaxRule
    BeginLeftUnaryOperatorSyntaxRule
    BeginRightUnaryOperatorSyntaxRule
    BeginLeftBinaryOperatorSyntaxRule
    BeginRightBinaryOperatorSyntaxRule
    BeginNeutralBinaryOperatorSyntaxRule
    EndSyntaxRule
    BeginSyntaxRuleGrammar
    EndSyntaxRuleGrammar
    ConcatenateSyntaxRule
    AlternateSyntaxRule
    RepeatSyntaxRule
    PushEmptySyntaxRule
    PushKeywordSyntaxRule
    PushMarkedDescentSyntaxRule
    PushDescentSyntaxRule
    PushMarkedTrueSyntaxRule
    PushMarkedFalseSyntaxRule
    PushMarkedIntegerSyntaxRule
    PushMarkedStringSyntaxRule
    BeginSyntaxRuleMappings
    EndSyntaxRuleMappings

    Fatal_OutOfMemory
    Fatal_InternalInconsistency
    Error_MalformedToken
    Error_MalformedSyntax
    Error_IdentifierPreviouslyDeclared
    Error_IdentifierNotDeclared
    Error_InvalidNameForSyntaxMarkVariable
    Error_SyntaxMarkVariableNotAllowedInDelimiter
    Error_SyntaxMarkVariableAlreadyDefined

--------------------------------------------------------------------------------

'condition' IsBootstrapCompile()

--------------------------------------------------------------------------------

'action' InitializePosition()
'action' FinalizePosition()

'action' AdvanceCurrentPosition(Delta: INT)
'action' AdvanceCurrentPositionToNextLine()

'action' GetColumnOfCurrentPosition(Position: POS -> Column: INT)

--------------------------------------------------------------------------------

'action' InitializeLiterals()
'action' FinalizeLiterals()

'action' MakeIntegerLiteral(Token: STRING -> Literal: INT)
'action' MakeDoubleLiteral(Token: STRING -> Literal: DOUBLE)
'action' MakeStringLiteral(Token: STRING -> Literal: STRING)
'action' MakeNameLiteral(Token: STRING -> Literal: NAME)

--------------------------------------------------------------------------------

'action' InitializeScopes()
'action' FinalizeScopes()

'action' DumpScopes()

'action' EnterScope()
'action' LeaveScope()

'action' DefineMeaning(Name: NAME, Meaning: MEANING)
'action' UndefineMeaning(Name: NAME)
'condition' HasLocalMeaning(Name: NAME -> Meaning: MEANING)
'condition' HasMeaning(Name: NAME -> Meaning: MEANING)

--------------------------------------------------------------------------------

'action' PushEmptySet()
'action' DuplicateSet()
'action' ExchangeSet()
'action' UnionSet()
'action' IncludeIndexInSet(Index: INT)
'action' ExcludeIndexFromSet(Index: INT)
'condition' IsIndexInSet(Index: INT)

--------------------------------------------------------------------------------

'action' BeginStatementSyntaxRule(NAME)
'action' BeginExpressionSyntaxRule(NAME)
'action' BeginLeftUnaryOperatorSyntaxRule(NAME, INT)
'action' BeginRightUnaryOperatorSyntaxRule(NAME, INT)
'action' BeginLeftBinaryOperatorSyntaxRule(NAME, INT)
'action' BeginRightBinaryOperatorSyntaxRule(NAME, INT)
'action' BeginNeutralBinaryOperatorSyntaxRule(NAME, INT)
'action' EndSyntaxRule()

'action' BeginSyntaxRuleGrammar()
'action' EndSyntaxRuleGrammar()

'action' ConcatenateSyntaxRule()
'action' AlternateSyntaxRule()
'action' RepeatSyntaxRule()
'action' PushEmptySyntaxRule()
'action' PushKeywordSyntaxRule(STRING)
'action' PushMarkedDescentSyntaxRule(INT, NAME)
'action' PushDescentSyntaxRule(NAME)
'action' PushMarkedTrueSyntaxRule(INT)
'action' PushMarkedFalseSyntaxRule(INT)
'action' PushMarkedIntegerSyntaxRule(INT, INT)
'action' PushMarkedStringSyntaxRule(INT, STRING)

'action' BeginSyntaxRuleMappings()
'action' EndSyntaxRuleMappings()

--------------------------------------------------------------------------------

'action' Fatal_OutOfMemory()
'action' Fatal_InternalInconsistency(Message: STRING)
'action' Error_MalformedToken(Position: POS, Token: STRING)
'action' Error_MalformedSyntax(Position: POS)
'action' Error_IdentifierPreviouslyDeclared(Position: POS, Identifier: NAME, PreviousPosition: POS)
'action' Error_IdentifierNotDeclared(Position: POS, Identifier: NAME)
'action' Error_InvalidNameForSyntaxMarkVariable(Position: POS, Name: NAME)
'action' Error_SyntaxMarkVariableNotAllowedInDelimiter(Position: POS)
'action' Error_SyntaxMarkVariableAlreadyDefined(Position: POS, Name: NAME)

--------------------------------------------------------------------------------
