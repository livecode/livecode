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
    GetUndefinedPosition

    InitializeLiterals
    FinalizeLiterals
    MakeIntegerLiteral
    MakeDoubleLiteral
    MakeStringLiteral
    MakeNameLiteral
    GetStringOfNameLiteral

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

    ReorderOperatorExpression
    PopOperatorExpression
    PopOperatorExpressionArgument
    PushOperatorExpressionPrefix
    PushOperatorExpressionPostfix
    PushOperatorExpressionLeftBinary
    PushOperatorExpressionRightBinary
    PushOperatorExpressionNeutralBinary
    PushOperatorExpressionArgument
    PushOperatorExpressionOperand

    GenerateSyntaxRules
    DumpSyntaxRules
    BeginPhraseSyntaxRule
    BeginStatementSyntaxRule
    BeginExpressionSyntaxRule
    BeginPrefixOperatorSyntaxRule
    BeginPostfixOperatorSyntaxRule
    BeginLeftBinaryOperatorSyntaxRule
    BeginRightBinaryOperatorSyntaxRule
    BeginNeutralBinaryOperatorSyntaxRule
    EndSyntaxRule
    BeginSyntaxGrammar
    EndSyntaxGrammar
    ConcatenateSyntaxGrammar
    AlternateSyntaxGrammar
    RepeatSyntaxGrammar
    PushEmptySyntaxGrammar
    PushKeywordSyntaxGrammar
    PushMarkedDescentSyntaxGrammar
    PushDescentSyntaxGrammar
    PushMarkedTrueSyntaxGrammar
    PushMarkedFalseSyntaxGrammar
    PushMarkedIntegerSyntaxGrammar
    PushMarkedStringSyntaxGrammar
    BeginSyntaxMappings
    EndSyntaxMappings
    BeginMethodSyntaxMapping
    EndMethodSyntaxMapping
    PushUndefinedArgumentSyntaxMapping
    PushTrueArgumentSyntaxMapping
    PushFalseArgumentSyntaxMapping
    PushIntegerArgumentSyntaxMapping
    PushStringArgumentSyntaxMapping
    PushMarkArgumentSyntaxMapping

    ErrorsDidOccur
    Fatal_OutOfMemory
    Fatal_InternalInconsistency
    Error_MalformedToken
    Error_MalformedSyntax
    Error_IdentifierPreviouslyDeclared
    Error_IdentifierNotDeclared
    Error_InvalidNameForSyntaxMarkVariable
    Error_SyntaxMarkVariableAlreadyDefined
    Error_ExpressionSyntaxCannotStartWithExpression
    Error_ExpressionSyntaxCannotFinishWithExpression
    Error_PrefixSyntaxCannotStartWithExpression
    Error_PrefixSyntaxMustFinishWithExpression
    Error_PostfixSyntaxMustStartWithExpression
    Error_PostfixSyntaxCannotFinishWithExpression
    Error_BinarySyntaxMustStartWithExpression
    Error_BinarySyntaxMustFinishWithExpression
    Error_ElementSyntaxCannotBeNullable
    Error_OnlyKeywordsAllowedInDelimiterSyntax
    Error_OptionalSyntaxCannotContainOnlyMarks
    Error_SyntaxMarksMustBeConstant
    Error_NotBoundToAType
    Error_NotBoundToAPhrase
    Error_NotBoundToASyntaxRule
    Error_NotBoundToASyntaxMark
    Error_NotBoundToAHandler
    Error_NotBoundToAVariable
    Error_NotBoundToAConstantSyntaxValue
    Error_NotBoundToAVariableOrHandler
    Error_TooManyArgumentsPassedToHandler
    Error_TooFewArgumentsPassedToHandler
    Error_HandlersBoundToSyntaxMustNotReturnAValue
    Error_SyntaxMarkVariableAlreadyDefinedWithDifferentType

--------------------------------------------------------------------------------

'condition' IsBootstrapCompile()

--------------------------------------------------------------------------------

'action' InitializePosition()
'action' FinalizePosition()

'action' AdvanceCurrentPosition(Delta: INT)
'action' AdvanceCurrentPositionToNextLine()

'action' GetColumnOfCurrentPosition(Position: POS -> Column: INT)

'action' GetUndefinedPosition(-> Position: POS)

--------------------------------------------------------------------------------

'action' InitializeLiterals()
'action' FinalizeLiterals()

'action' MakeIntegerLiteral(Token: STRING -> Literal: INT)
'action' MakeDoubleLiteral(Token: STRING -> Literal: DOUBLE)
'action' MakeStringLiteral(Token: STRING -> Literal: STRING)
'action' MakeNameLiteral(Token: STRING -> Literal: NAME)

'action' GetStringOfNameLiteral(Name: NAME -> String: STRING)

--------------------------------------------------------------------------------

'action' ReorderOperatorExpression
'condition' PopOperatorExpression(-> Position: POS, Method: INT, Arity: INT)
'action' PopOperatorExpressionArgument(-> EXPRESSION)

'action' PushOperatorExpressionPrefix(Position: POS, Precedence: INT, Method: INT)
'action' PushOperatorExpressionPostfix(Position: POS, Precedence: INT, Method: INT)
'action' PushOperatorExpressionLeftBinary(Position: POS, Precedence: INT, Method: INT)
'action' PushOperatorExpressionRightBinary(Position: POS, Precedence: INT, Method: INT)
'action' PushOperatorExpressionNeutralBinary(Position: POS, Precedence: INT, Method: INT)
'action' PushOperatorExpressionArgument(Operand: EXPRESSION)
'action' PushOperatorExpressionOperand(Argument: EXPRESSION)

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

'action' DumpSyntaxRules()

'action' GenerateSyntaxRules()

'action' BeginPhraseSyntaxRule(NAME)
'action' BeginStatementSyntaxRule(NAME)
'action' BeginExpressionSyntaxRule(NAME)
'action' BeginPrefixOperatorSyntaxRule(NAME, INT)
'action' BeginPostfixOperatorSyntaxRule(NAME, INT)
'action' BeginLeftBinaryOperatorSyntaxRule(NAME, INT)
'action' BeginRightBinaryOperatorSyntaxRule(NAME, INT)
'action' BeginNeutralBinaryOperatorSyntaxRule(NAME, INT)
'action' EndSyntaxRule()

'action' BeginSyntaxGrammar()
'action' EndSyntaxGrammar()

'action' ConcatenateSyntaxGrammar()
'action' AlternateSyntaxGrammar()
'action' RepeatSyntaxGrammar()
'action' PushEmptySyntaxGrammar()
'action' PushKeywordSyntaxGrammar(Token: STRING)
'action' PushMarkedDescentSyntaxGrammar(Index: INT, Rule: NAME)
'action' PushDescentSyntaxGrammar(Rule: NAME)
'action' PushMarkedTrueSyntaxGrammar(Index: INT)
'action' PushMarkedFalseSyntaxGrammar(Index: INT)
'action' PushMarkedIntegerSyntaxGrammar(Index: INT, Value: INT)
'action' PushMarkedStringSyntaxGrammar(Index: INT, Value: STRING)

'action' BeginSyntaxMappings()
'action' EndSyntaxMappings()

'action' BeginMethodSyntaxMapping(Name: NAME)
'action' EndMethodSyntaxMapping()
'action' PushUndefinedArgumentSyntaxMapping()
'action' PushTrueArgumentSyntaxMapping()
'action' PushFalseArgumentSyntaxMapping()
'action' PushIntegerArgumentSyntaxMapping(Value: INT)
'action' PushStringArgumentSyntaxMapping(Value: STRING)
'action' PushMarkArgumentSyntaxMapping(MarkIndex: INT)
'action' PushIndexedMarkArgumentSyntaxMapping(MarkIndex: INT, Index: INT)

--------------------------------------------------------------------------------

'condition' ErrorsDidOccur()

'action' Fatal_OutOfMemory()
'action' Fatal_InternalInconsistency(Message: STRING)
'action' Error_MalformedToken(Position: POS, Token: STRING)
'action' Error_MalformedSyntax(Position: POS)
'action' Error_IdentifierPreviouslyDeclared(Position: POS, Identifier: NAME, PreviousPosition: POS)
'action' Error_IdentifierNotDeclared(Position: POS, Identifier: NAME)
'action' Error_InvalidNameForSyntaxMarkVariable(Position: POS, Name: NAME)
'action' Error_SyntaxMarkVariableAlreadyDefined(Position: POS, Name: NAME)

'action' Error_ExpressionSyntaxCannotStartWithExpression(Position: POS)
'action' Error_ExpressionSyntaxCannotFinishWithExpression(Position: POS)
'action' Error_PrefixSyntaxCannotStartWithExpression(Position: POS)
'action' Error_PrefixSyntaxMustFinishWithExpression(Position: POS)
'action' Error_PostfixSyntaxMustStartWithExpression(Position: POS)
'action' Error_PostfixSyntaxCannotFinishWithExpression(Position: POS)
'action' Error_BinarySyntaxMustStartWithExpression(Position: POS)
'action' Error_BinarySyntaxMustFinishWithExpression(Position: POS)
'action' Error_ElementSyntaxCannotBeNullable(Position: POS)
'action' Error_OnlyKeywordsAllowedInDelimiterSyntax(Position: POS)
'action' Error_OptionalSyntaxCannotContainOnlyMarks(Position: POS)
'action' Error_SyntaxMarksMustBeConstant(Position: POS)

'action' Error_NotBoundToAType(Position: POS, Name: NAME)
'action' Error_NotBoundToAPhrase(Position: POS, Name: NAME)
'action' Error_NotBoundToASyntaxRule(Position: POS, Name: NAME)
'action' Error_NotBoundToASyntaxMark(Position: POS, Name: NAME)
'action' Error_NotBoundToAConstantSyntaxValue(Position: POS, Name: NAME)
'action' Error_NotBoundToAHandler(Position: POS, Name: NAME)
'action' Error_NotBoundToAVariable(Position: POS, Name: NAME)
'action' Error_NotBoundToAVariableOrHandler(Position: POS, Name: NAME)

'action' Error_TooManyArgumentsPassedToHandler(Position: POS)
'action' Error_TooFewArgumentsPassedToHandler(Position: POS)
'action' Error_HandlersBoundToSyntaxMustNotReturnAValue(Position: POS)
'action' Error_SyntaxMarkVariableAlreadyDefinedWithDifferentType(Position: POS, Name: NAME)

--------------------------------------------------------------------------------
