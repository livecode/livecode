/* Copyright (C) 2003-2016 LiveCode Ltd.
 
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

'module' support

'use'
    types

'export'
    IsBootstrapCompile
    IsNotBytecodeOutput

    NegateReal

    InitializePosition
    FinalizePosition
    AdvanceCurrentPosition
    AdvanceCurrentPositionToNextLine
    GetColumnOfCurrentPosition
    GetUndefinedPosition
    AddImportedModuleFile
	AddImportedModuleName
    GetFilenameOfPosition

    InitializeLiterals
    FinalizeLiterals
    MakeIntegerLiteral
    MakeDoubleLiteral
    MakeStringLiteral
    UnescapeStringLiteral
    MakeNameLiteral
    GetStringOfNameLiteral
    IsNameEqualToName
    IsNameNotEqualToName
    IsNameEqualToString
    IsStringEqualToString

    IsNameSuitableForDefinition
    IsNameSuitableForNamespace
    IsNameValidForNamespace
    IsStringSuitableForKeyword

	ConcatenateNameParts
	ContainsNamespaceOperator
	SplitNamespace
	
    InitializeScopes
    FinalizeScopes
    DumpScopes
    EnterScope
    LeaveScope
    DefineMeaning
	DefineUnqualifiedMeaning
    UndefineMeaning
    HasLocalMeaning
    HasMeaning
	HasUnqualifiedMeaning

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
    BeginIteratorSyntaxRule
    BeginPrefixOperatorSyntaxRule
    BeginPostfixOperatorSyntaxRule
    BeginLeftBinaryOperatorSyntaxRule
    BeginRightBinaryOperatorSyntaxRule
    BeginNeutralBinaryOperatorSyntaxRule
    EndSyntaxRule
    DeprecateSyntaxRule
    BeginSyntaxGrammar
    EndSyntaxGrammar
    ConcatenateSyntaxGrammar
    AlternateSyntaxGrammar
    RepeatSyntaxGrammar
    PushEmptySyntaxGrammar
    PushKeywordSyntaxGrammar
    PushUnreservedKeywordSyntaxGrammar
    PushMarkedDescentSyntaxGrammar
    PushDescentSyntaxGrammar
    PushMarkedTrueSyntaxGrammar
    PushMarkedFalseSyntaxGrammar
    PushMarkedIntegerSyntaxGrammar
    PushMarkedRealSyntaxGrammar
    PushMarkedStringSyntaxGrammar
    BeginSyntaxMappings
    EndSyntaxMappings
    BeginExecuteMethodSyntaxMapping
    BeginEvaluateMethodSyntaxMapping
    BeginAssignMethodSyntaxMapping
    BeginIterateMethodSyntaxMapping
    EndMethodSyntaxMapping
    PushUndefinedArgumentSyntaxMapping
    PushTrueArgumentSyntaxMapping
    PushFalseArgumentSyntaxMapping
    PushIntegerArgumentSyntaxMapping
    PushRealArgumentSyntaxMapping
    PushStringArgumentSyntaxMapping
    PushInMarkArgumentSyntaxMapping
    PushOutMarkArgumentSyntaxMapping
    PushInOutMarkArgumentSyntaxMapping
    AddUnreservedSyntaxKeyword

    IsDependencyCompile
    DependStart
    DependFinish
    DependDefineMapping
    DependDefineDependency

    BytecodeEnumerate
    BytecodeLookup
    BytecodeDescribe
    BytecodeDescribeParameter
    BytecodeIsValidArgumentCount

    EmitStart
    EmitFinish
    EmitBeginModule
    EmitBeginLibraryModule
    EmitBeginWidgetModule
    EmitEndModule
    EmitDefinitionIndex
    EmitExportedDefinition
    EmitModuleDependency
    EmitImportedType
    EmitImportedConstant
    EmitImportedVariable
    EmitImportedHandler
    EmitImportedSyntax
    EmitTypeDefinition
    EmitConstantDefinition
    EmitVariableDefinition
    EmitBeginHandlerDefinition
    EmitEndHandlerDefinition
    EmitBeginUnsafeHandlerDefinition
    EmitForeignHandlerDefinition
    EmitPropertyDefinition
    EmitEventDefinition
    EmitBeginSyntaxDefinition
    EmitEndSyntaxDefinition
    EmitBeginSyntaxMethod
    EmitEndSyntaxMethod
    EmitUndefinedSyntaxMethodArgument
    EmitTrueSyntaxMethodArgument
    EmitFalseSyntaxMethodArgument
    EmitIntegerSyntaxMethodArgument
    EmitRealSyntaxMethodArgument
    EmitStringSyntaxMethodArgument
    EmitVariableSyntaxMethodArgument
    EmitIndexedVariableSyntaxMethodArgument
    EmitInputSyntaxMethodArgument
    EmitOutputSyntaxMethodArgument
    EmitContextSyntaxMethodArgument
    EmitContainerSyntaxMethodArgument
    EmitIteratorSyntaxMethodArgument
    EmitBeginDefinitionGroup
    EmitContinueDefinitionGroup
    EmitEndDefinitionGroup
    EmitDefinedType
    EmitForeignType
    EmitOptionalType
    EmitAnyType
    EmitBooleanType
    EmitIntegerType
    EmitRealType
    EmitNumberType
    EmitStringType
    EmitDataType
    EmitArrayType
    EmitListType
    EmitUndefinedType
    EmitBeginRecordType
    EmitRecordTypeField
    EmitEndRecordType
    EmitBeginHandlerType
    EmitBeginForeignHandlerType
    EmitHandlerTypeInParameter
    EmitHandlerTypeOutParameter
    EmitHandlerTypeInOutParameter
    EmitHandlerTypeVariadicParameter
    EmitEndHandlerType
    EmitHandlerParameter
    EmitHandlerVariable
    EmitDeferLabel
    EmitResolveLabel
    EmitCreateRegister
    EmitDestroyRegister
    EmitJump
    EmitJumpIfTrue
    EmitJumpIfFalse
    EmitPushRepeatLabels
    EmitPopRepeatLabels
    EmitCurrentRepeatLabels
    EmitBeginIndirectInvoke
    EmitBeginInvoke
    EmitContinueInvoke
    EmitEndInvoke
    EmitAssign
    EmitAssignConstant
    EmitUndefinedConstant
    EmitTrueConstant
    EmitFalseConstant
    EmitIntegerConstant
    EmitUnsignedIntegerConstant
    EmitRealConstant
    EmitStringConstant
    EmitBeginListConstant
    EmitContinueListConstant
    EmitEndListConstant
    EmitBeginArrayConstant
    EmitContinueArrayConstant
    EmitEndArrayConstant
    EmitBeginAssignList
    EmitContinueAssignList
    EmitEndAssignList
    EmitBeginAssignArray
    EmitContinueAssignArray
    EmitEndAssignArray
    EmitFetch
    EmitStore
    EmitReturn
    EmitReturnNothing
    EmitReset
    EmitAttachRegisterToExpression
    EmitDetachRegisterFromExpression
    EmitGetRegisterAttachedToExpression
    EmitPosition
    EmitBeginOpcode
    EmitContinueOpcode
    EmitEndOpcode

    OutputBeginManifest
    OutputEnd
    OutputWrite
    OutputWriteI
    OutputWriteS
    OutputWriteXmlS

    ErrorsDidOccur
    Fatal_OutOfMemory
    Fatal_InternalInconsistency
    Error_UnableToFindImportedModule
    Error_MalformedToken
    Error_MalformedSyntax
    Error_MalformedEscapedString
	Error_IllegalNamespaceOperator
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
    Error_NotBoundToAConstantOrVariableOrHandler
    Error_TooManyArgumentsPassedToHandler
    Error_TooFewArgumentsPassedToHandler
    Error_HandlersBoundToSyntaxMustNotReturnAValue
    Error_SyntaxMarkVariableAlreadyDefinedWithDifferentType
    Error_ConstantSyntaxArgumentMustBindToInParameter
    Error_ContextSyntaxArgumentMustBindToInParameter
    Error_InputSyntaxArgumentMustBindToInParameter
    Error_OutputSyntaxArgumentMustBindToOutParameter
    Error_ContainerSyntaxArgumentMustBindToInParameter
    Error_IteratorSyntaxArgumentMustBindToInOutParameter
    Error_PhraseBoundMarkSyntaxArgumentMustBindToInParameter
    Error_VariableSyntaxArgumentMustBindToConsistentMode
    Error_SyntaxMethodArgumentsMustMatch
    Error_LSyntaxMethodArgumentsDontConform
    Error_RSyntaxMethodArgumentsDontConform
    Error_ExpressionSyntaxMethodArgumentsDontConform
    Error_HandlersBoundToSyntaxMustBePublic
    Error_IterateSyntaxMethodArgumentsDontConform
    Error_IterateSyntaxMethodMustReturnBoolean
    Error_PhraseSyntaxMethodMustReturnAValue
    Error_NonAssignableExpressionUsedForOutContext
    Error_NonEvaluatableExpressionUsedForInContext
    Error_SyntaxNotAllowedInThisContext
    Error_ParameterMustHaveHighLevelType
    Error_VariableMustHaveHighLevelType
    Error_CannotAssignToHandlerId
    Error_CannotAssignToConstantId
    Error_ConstantsMustBeSimple
    Error_NonHandlerTypeVariablesCannotBeCalled
    Error_HandlerNotSuitableForPropertyGetter
    Error_HandlerNotSuitableForPropertySetter
    Error_UnsuitableStringForKeyword
    Error_IntegerLiteralOutOfRange
    Error_NextRepeatOutOfContext
    Error_ExitRepeatOutOfContext
    Error_DependentModuleNotIncludedWithInputs
    Error_InterfaceFileNameMismatch
    Error_NoReturnTypeSpecifiedForForeignHandler
    Error_NoTypeSpecifiedForForeignHandlerParameter
    Error_ConstantArrayKeyIsNotStringLiteral
    Error_ListExpressionTooLong
    Error_ArrayExpressionTooLong
    Error_UnknownOpcode
    Error_OpcodeArgumentMustBeLabel
    Error_OpcodeArgumentMustBeRegister
    Error_OpcodeArgumentMustBeConstant
    Error_OpcodeArgumentMustBeHandler
    Error_OpcodeArgumentMustBeVariable
    Error_OpcodeArgumentMustBeDefinition
    Error_IllegalNumberOfArgumentsForOpcode
    Error_BytecodeNotAllowedInSafeContext
    Error_UnsafeHandlerCallNotAllowedInSafeContext
    Error_InvalidNameForNamespace
    Error_VariadicParametersOnlyAllowedInForeignHandlers
    Error_VariadicParameterMustBeLast
    Error_VariadicArgumentNotExplicitlyTyped

    Warning_MetadataClausesShouldComeAfterUseClauses
    Warning_DeprecatedTypeName
    Warning_UnsuitableNameForDefinition
    Warning_UnsuitableNameForNamespace
    Warning_DeprecatedSyntax

--------------------------------------------------------------------------------

'condition' IsBootstrapCompile()
'condition' IsNotBytecodeOutput()

--------------------------------------------------------------------------------

'action' NegateReal(DOUBLE -> DOUBLE)

--------------------------------------------------------------------------------

'action' InitializePosition()
'action' FinalizePosition()

'action' AdvanceCurrentPosition(Delta: INT)
'action' AdvanceCurrentPositionToNextLine()

'action' GetColumnOfCurrentPosition(Position: POS -> Column: INT)

'action' GetUndefinedPosition(-> Position: POS)

'condition' AddImportedModuleFile(Name: STRING)
'action' AddImportedModuleName(Name: STRING)

'action' GetFilenameOfPosition(Position: POS -> Filename: STRING)

--------------------------------------------------------------------------------

'action' InitializeLiterals()
'action' FinalizeLiterals()

'condition' MakeIntegerLiteral(Token: STRING -> Literal: INT)
'condition' MakeDoubleLiteral(Token: STRING -> Literal: DOUBLE)
'action' MakeStringLiteral(Token: STRING -> Literal: STRING)
'condition' UnescapeStringLiteral(Position:POS, String: STRING -> UnescapedString: STRING)
'action' MakeNameLiteral(Token: STRING -> Literal: NAME)

'action' GetStringOfNameLiteral(Name: NAME -> String: STRING)
'condition' IsNameEqualToString(NAME, STRING)
'condition' IsStringEqualToString(STRING, STRING)
'condition' IsNameEqualToName(NAME, NAME)
'condition' IsNameNotEqualToName(NAME, NAME)

'condition' IsNameSuitableForDefinition(NAME)
'condition' IsNameSuitableForNamespace(NAME)
'condition' IsNameValidForNamespace(NAME)
'condition' IsStringSuitableForKeyword(STRING)

'action' ConcatenateNameParts(NAME, NAME -> NAME)
'action' SplitNamespace(NAME -> NAME, NAME)
'condition' ContainsNamespaceOperator(NAME)

--------------------------------------------------------------------------------

'action' ReorderOperatorExpression(Sentinal: INT)
'condition' PopOperatorExpression(-> Position: POS, Method: INT, Arity: INT)
'action' PopOperatorExpressionArgument(-> EXPRESSION)

'action' PushOperatorExpressionPrefix(Position: POS, Precedence: INT, Method: INT -> INT)
'action' PushOperatorExpressionPostfix(Position: POS, Precedence: INT, Method: INT -> INT)
'action' PushOperatorExpressionLeftBinary(Position: POS, Precedence: INT, Method: INT -> INT)
'action' PushOperatorExpressionRightBinary(Position: POS, Precedence: INT, Method: INT -> INT)
'action' PushOperatorExpressionNeutralBinary(Position: POS, Precedence: INT, Method: INT -> INT)
'action' PushOperatorExpressionArgument(Operand: EXPRESSION -> INT)
'action' PushOperatorExpressionOperand(Argument: EXPRESSION -> INT)

--------------------------------------------------------------------------------

'action' InitializeScopes()
'action' FinalizeScopes()

'action' DumpScopes()

'action' EnterScope()
'action' LeaveScope()

'action' DefineMeaning(Name: NAME, Namespace: NAME, Meaning: MEANING)
'action' DefineUnqualifiedMeaning(Name: NAME, Meaning: MEANING)
'action' UndefineMeaning(Name: NAME, Namespace: NAME)
'condition' HasLocalMeaning(Name: NAME -> Meaning: MEANING)
'condition' HasMeaning(Name: NAME, Namespace: NAME -> Meaning: MEANING)
'condition' HasUnqualifiedMeaning(Name: NAME -> Meaning: MEANING)

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

'action' BeginPhraseSyntaxRule(NAME, NAME)
'action' BeginStatementSyntaxRule(NAME, NAME)
'action' BeginIteratorSyntaxRule(NAME, NAME)
'action' BeginExpressionSyntaxRule(NAME, NAME)
'action' BeginPrefixOperatorSyntaxRule(NAME, NAME, INT)
'action' BeginPostfixOperatorSyntaxRule(NAME, NAME, INT)
'action' BeginLeftBinaryOperatorSyntaxRule(NAME, NAME, INT)
'action' BeginRightBinaryOperatorSyntaxRule(NAME, NAME, INT)
'action' BeginNeutralBinaryOperatorSyntaxRule(NAME, NAME, INT)
'action' EndSyntaxRule()

'action' DeprecateSyntaxRule(Message: STRING)

'action' BeginSyntaxGrammar()
'action' EndSyntaxGrammar()

'action' ConcatenateSyntaxGrammar()
'action' AlternateSyntaxGrammar()
'action' RepeatSyntaxGrammar()
'action' PushEmptySyntaxGrammar()
'action' PushKeywordSyntaxGrammar(Token: STRING)
'action' PushUnreservedKeywordSyntaxGrammar(Token: STRING)
'action' PushMarkedDescentSyntaxGrammar(Index: INT, Rule: NAME, LMode: INT, RMode: INT)
'action' PushDescentSyntaxGrammar(Rule: NAME)
'action' PushMarkedTrueSyntaxGrammar(Index: INT)
'action' PushMarkedFalseSyntaxGrammar(Index: INT)
'action' PushMarkedIntegerSyntaxGrammar(Index: INT, Value: INT)
'action' PushMarkedRealSyntaxGrammar(Index: INT, Value: DOUBLE)
'action' PushMarkedStringSyntaxGrammar(Index: INT, Value: STRING)

'action' SetLModeOfMarkToIn(Index: INT)
'action' SetRModeOfMarkToIn(Index: INT)
'action' SetLModeOfMarkToOut(Index: INT)
'action' SetRModeOfMarkToOut(Index: INT)
'action' SetLModeOfMarkToInOut(Index: INT)
'action' SetRModeOfMarkToInOut(Index: INT)

'action' BeginSyntaxMappings()
'action' EndSyntaxMappings()

'action' BeginExecuteMethodSyntaxMapping(Name: NAME)
'action' BeginEvaluateMethodSyntaxMapping(Name: NAME)
'action' BeginAssignMethodSyntaxMapping(Name: NAME)
'action' BeginIterateMethodSyntaxMapping(Name: NAME)
'action' EndMethodSyntaxMapping()
'action' PushInMarkArgumentSyntaxMapping(MarkIndex: INT)
'action' PushOutMarkArgumentSyntaxMapping(MarkIndex: INT)
'action' PushInOutMarkArgumentSyntaxMapping(MarkIndex: INT)

'action' PushUndefinedArgumentSyntaxMapping()
'action' PushTrueArgumentSyntaxMapping()
'action' PushFalseArgumentSyntaxMapping()
'action' PushIntegerArgumentSyntaxMapping(Value: INT)
'action' PushRealArgumentSyntaxMapping(Value: DOUBLE)
'action' PushStringArgumentSyntaxMapping(Value: STRING)
'action' PushIndexedMarkArgumentSyntaxMapping(MarkIndex: INT, Index: INT)

'action' AddUnreservedSyntaxKeyword(Token: NAME)

--------------------------------------------------------------------------------

'condition' IsDependencyCompile()
'action' DependStart()
'action' DependFinish()
'action' DependDefineMapping(ModuleName: NAME, SourceFile: STRING)
'action' DependDefineDependency(ModuleName: NAME, RequiredModuleName: NAME)

--------------------------------------------------------------------------------

'condition' BytecodeEnumerate(Index: INT -> Name: NAME)
'condition' BytecodeLookup(Name: STRING -> Opcode: INT)
'action' BytecodeDescribe(Opcode: INT -> Name: NAME)
'condition' BytecodeIsValidArgumentCount(Opcode: INT, Count: INT)
'action' BytecodeDescribeParameter(Opcode: INT, Index: INT -> Type: INT)

--------------------------------------------------------------------------------

'action' EmitStart()
'action' EmitFinish()

'action' EmitBeginModule(Name: NAME -> ModuleIndex: INT)
'action' EmitBeginWidgetModule(Name: NAME -> ModuleIndex: INT)
'action' EmitBeginLibraryModule(Name: NAME -> ModuleIndex: INT)
'action' EmitEndModule()

'action' EmitModuleDependency(Name: NAME -> ModuleIndex: INT)

'action' EmitImportedType(ModuleIndex: INT, Name: NAME, TypeIndex: INT -> Index: INT)
'action' EmitImportedConstant(ModuleIndex: INT, Name: NAME, TypeIndex: INT -> Index: INT)
'action' EmitImportedVariable(ModuleIndex: INT, Name: NAME, TypeIndex: INT -> Index: INT)
'action' EmitImportedHandler(ModuleIndex: INT, Name: NAME, TypeIndex: INT -> Index: INT)
'action' EmitImportedSyntax(ModuleIndex: INT, Name: NAME, TypeIndex: INT -> Index: INT)

'action' EmitExportedDefinition(Index: INT)

'action' EmitDefinitionIndex(Kind: STRING -> Index: INT)

'action' EmitTypeDefinition(Index: INT, Position: POS, Name: NAME, TypeIndex: INT)
'action' EmitConstantDefinition(Index: INT, Position: POS, Name: NAME, ConstIndex: INT)
'action' EmitVariableDefinition(Index: INT, Position: POS, Name: NAME, TypeIndex: INT)
'action' EmitBeginHandlerDefinition(Index: INT, Position: POS, Name: NAME, TypeIndex: INT)
'action' EmitEndHandlerDefinition()
'action' EmitBeginUnsafeHandlerDefinition(Index: INT, Position: POS, Name: NAME, TypeIndex: INT)
'action' EmitForeignHandlerDefinition(Index: INT, Position: POS, Name: NAME, TypeIndex: INT, Binding: STRING)
'action' EmitPropertyDefinition(Index: INT, Position: POS, Name: NAME, GetIndex: INT, SetIndex: INT)
'action' EmitEventDefinition(Index: INT, Position: POS, Name: NAME, TypeIndex: INT)

'action' EmitBeginSyntaxDefinition(Index: INT, Position: POS, Name: NAME)
'action' EmitEndSyntaxDefinition()
'action' EmitBeginSyntaxMethod(HandlerIndex: INT)
'action' EmitEndSyntaxMethod()
'action' EmitUndefinedSyntaxMethodArgument()
'action' EmitTrueSyntaxMethodArgument()
'action' EmitFalseSyntaxMethodArgument()
'action' EmitInputSyntaxMethodArgument()
'action' EmitOutputSyntaxMethodArgument()
'action' EmitContextSyntaxMethodArgument()
'action' EmitContainerSyntaxMethodArgument()
'action' EmitIteratorSyntaxMethodArgument()
'action' EmitIntegerSyntaxMethodArgument(Value: INT)
'action' EmitRealSyntaxMethodArgument(Value: DOUBLE)
'action' EmitStringSyntaxMethodArgument(Value: STRING)
'action' EmitVariableSyntaxMethodArgument(Index: INT)
'action' EmitIndexedVariableSyntaxMethodArgument(VarIndex: INT, Element: INT)

'action' EmitBeginDefinitionGroup()
'action' EmitContinueDefinitionGroup(Index: INT)
'action' EmitEndDefinitionGroup(-> Index: INT)

'action' EmitDefinedType(INT -> INT)
'action' EmitForeignType(STRING -> INT)
'action' EmitOptionalType(INT -> INT)
--'action' EmitNamedType(Module: NAME, Name: NAME -> INT)
--'action' EmitAliasType(Name: NAME, TypeIndex: INT -> INT)
'action' EmitAnyType(-> INT)
'action' EmitBooleanType(-> INT)
'action' EmitIntegerType(-> INT)
'action' EmitRealType(-> INT)
'action' EmitNumberType(-> INT)
'action' EmitStringType(-> INT)
'action' EmitDataType(-> INT)
'action' EmitArrayType(-> INT)
'action' EmitListType(-> INT)
'action' EmitUndefinedType(-> INT)

'action' EmitBeginRecordType()
'action' EmitRecordTypeField(Name: NAME, Type: INT)
'action' EmitEndRecordType(-> INT)

'action' EmitBeginHandlerType(ReturnType: INT)
'action' EmitBeginForeignHandlerType(ReturnType: INT)
'action' EmitHandlerTypeInParameter(Name: NAME, Type: INT)
'action' EmitHandlerTypeOutParameter(Name: NAME, Type: INT)
'action' EmitHandlerTypeInOutParameter(Name: NAME, Type: INT)
'action' EmitHandlerTypeVariadicParameter(Name: NAME)
'action' EmitEndHandlerType(-> INT)

'action' EmitHandlerParameter(Name: NAME, Type: INT -> Index: INT)
'action' EmitHandlerVariable(Name: NAME, Type: INT -> Index: INT)
'action' EmitDeferLabel(-> Label: INT)
'action' EmitResolveLabel(Label: INT)
'action' EmitCreateRegister(-> Register: INT)
'action' EmitDestroyRegister(Register: INT)
'action' EmitJump(Label: INT)
'action' EmitJumpIfTrue(Register: INT, Label: INT)
'action' EmitJumpIfFalse(Register: INT, Label: INT)
'action' EmitPushRepeatLabels(Head: INT, Tail: INT)
'action' EmitCurrentRepeatLabels(-> Next: INT, Exit: INT)
'action' EmitPopRepeatLabels()
'action' EmitBeginIndirectInvoke(Register: INT, ContextRegister: INT, ResultRegister: INT)
'action' EmitBeginInvoke(Index: INT, ContextRegister: INT, ResultRegister: INT)
'action' EmitContinueInvoke(Register: INT)
'action' EmitEndInvoke()
'action' EmitAssign(Dst: INT, Src: INT)
'action' EmitAssignConstant(Register: INT, ConstIndex: INT)
'action' EmitUndefinedConstant(-> ConstIndex: INT)
'action' EmitTrueConstant(-> ConstIndex: INT)
'action' EmitFalseConstant(-> ConstIndex: INT)
'action' EmitIntegerConstant(Value: INT -> ConstIndex: INT)
'action' EmitUnsignedIntegerConstant(Value: INT -> ConstIndex: INT)
'action' EmitRealConstant(Value: DOUBLE -> ConstIndex: INT)
'action' EmitStringConstant(Value: STRING -> ConstIndex: INT)
'action' EmitBeginListConstant()
'action' EmitContinueListConstant(ConstIndex: INT)
'action' EmitEndListConstant(-> ConstIndex: INT)
'action' EmitBeginArrayConstant()
'action' EmitContinueArrayConstant(ConstKeyIndex: INT, ConstValueIndex: INT)
'action' EmitEndArrayConstant(-> ConstIndex: INT)
'action' EmitBeginAssignList(Register: INT)
'action' EmitContinueAssignList(Register: INT)
'action' EmitEndAssignList()
'action' EmitBeginAssignArray(Register: INT)
'action' EmitContinueAssignArray(Register: INT)
'action' EmitEndAssignArray()
'action' EmitFetch(Register: INT, Var: INT, Level: INT)
'action' EmitStore(Register: INT, Var: INT, Level: INT)
'action' EmitReturn(Register: INT)
'action' EmitReturnNothing()
'action' EmitReset(Register: INT)
'action' EmitPosition(Position: POS)
'action' EmitBeginOpcode(Opcode: STRING)
'action' EmitContinueOpcode(Output: INT)
'action' EmitEndOpcode()

'action' EmitAttachRegisterToExpression(INT, EXPRESSION)
'action' EmitDetachRegisterFromExpression(EXPRESSION)
'condition' EmitGetRegisterAttachedToExpression(EXPRESSION -> INT)

'action' OutputBeginManifest()
'action' OutputEnd()
'action' OutputWrite(STRING)
'action' OutputWriteI(STRING, NAME, STRING)
'action' OutputWriteS(STRING, STRING, STRING)
'action' OutputWriteXmlS(STRING, STRING, STRING)

--------------------------------------------------------------------------------

'condition' ErrorsDidOccur()

'action' Fatal_OutOfMemory()
'action' Fatal_InternalInconsistency(Message: STRING)

'action' Error_UnableToFindImportedModule(Position: POS, Name: NAME)

'action' Error_MalformedToken(Position: POS, Token: STRING)
'action' Error_MalformedSyntax(Position: POS)
'action' Error_MalformedEscapedString(Position: POS, Token: STRING)
'action' Error_IllegalNamespaceOperator(Position: POS)
'action' Error_IdentifierPreviouslyDeclared(Position: POS, Identifier: NAME, PreviousPosition: POS)
'action' Error_IdentifierNotDeclared(Position: POS, Identifier: NAME)
'action' Error_InvalidNameForSyntaxMarkVariable(Position: POS, Name: NAME)
'action' Error_SyntaxMarkVariableAlreadyDefined(Position: POS, Name: NAME)
'action' Error_InvalidNameForNamespace(Position: POS, Identifier: NAME)

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
'action' Error_NotBoundToAConstantOrVariableOrHandler(Position: POS, Name: NAME)

'action' Error_NonAssignableExpressionUsedForOutContext(Position: POS)
'action' Error_NonEvaluatableExpressionUsedForInContext(Position: POS)
'action' Error_SyntaxNotAllowedInThisContext(Position: POS)

'action' Error_TooManyArgumentsPassedToHandler(Position: POS)
'action' Error_TooFewArgumentsPassedToHandler(Position: POS)
'action' Error_HandlersBoundToSyntaxMustNotReturnAValue(Position: POS)
'action' Error_SyntaxMarkVariableAlreadyDefinedWithDifferentType(Position: POS, Name: NAME)
'action' Error_ConstantSyntaxArgumentMustBindToInParameter(Position: POS)
'action' Error_ContextSyntaxArgumentMustBindToInParameter(Position: POS)
'action' Error_InputSyntaxArgumentMustBindToInParameter(Position: POS)
'action' Error_OutputSyntaxArgumentMustBindToOutParameter(Position: POS)
'action' Error_ContainerSyntaxArgumentMustBindToInParameter(Position: POS)
'action' Error_IteratorSyntaxArgumentMustBindToInOutParameter(Position: POS)
'action' Error_PhraseBoundMarkSyntaxArgumentMustBindToInParameter(Position: POS)
'action' Error_VariableSyntaxArgumentMustBindToConsistentMode(Positiobn: POS)

'action' Error_SyntaxMethodArgumentsMustMatch(Position: POS)
'action' Error_LSyntaxMethodArgumentsDontConform(Position: POS)
'action' Error_RSyntaxMethodArgumentsDontConform(Position: POS)
'action' Error_ExpressionSyntaxMethodArgumentsDontConform(Position: POS)
'action' Error_HandlersBoundToSyntaxMustBePublic(Position: POS)
'action' Error_IterateSyntaxMethodArgumentsDontConform(Position: POS)
'action' Error_IterateSyntaxMethodMustReturnBoolean(Position: POS)
'action' Error_PhraseSyntaxMethodMustReturnAValue(Position: POS)

'action' Error_ParameterMustHaveHighLevelType(Position: POS)
'action' Error_VariableMustHaveHighLevelType(Position: POS)

'action' Error_CannotAssignToHandlerId(Position: POS, Identifier: NAME)
'action' Error_CannotAssignToConstantId(Position: POS, Identifier: NAME)
'action' Error_NonHandlerTypeVariablesCannotBeCalled(Position: POS)

'action' Error_ConstantsMustBeSimple(Position: POS)

'action' Error_HandlerNotSuitableForPropertyGetter(Position: POS, Identifier: NAME)
'action' Error_HandlerNotSuitableForPropertySetter(Position: POS, Identifier: NAME)

'action' Error_UnsuitableStringForKeyword(Position: POS, Token: STRING)

'action' Error_IntegerLiteralOutOfRange(Position: POS)
'action' Error_NextRepeatOutOfContext(Position: POS)
'action' Error_ExitRepeatOutOfContext(Position: POS)

'action' Error_DependentModuleNotIncludedWithInputs(Position: POS, Module: NAME)
'action' Error_InterfaceFileNameMismatch(Position: POS, Module: NAME)

'action' Error_NoReturnTypeSpecifiedForForeignHandler(Position: POS)
'action' Error_NoTypeSpecifiedForForeignHandlerParameter(Position: POS)

'action' Error_ConstantArrayKeyIsNotStringLiteral(Position: POS)
'action' Error_ListExpressionTooLong(Position: POS)
'action' Error_ArrayExpressionTooLong(Position: POS)

'action' Error_UnknownOpcode(Position: POS, Opcode: NAME)
'action' Error_OpcodeArgumentMustBeLabel(Position: POS)
'action' Error_OpcodeArgumentMustBeRegister(Position: POS)
'action' Error_OpcodeArgumentMustBeConstant(Position: POS)
'action' Error_OpcodeArgumentMustBeHandler(Position: POS)
'action' Error_OpcodeArgumentMustBeVariable(Position: POS)
'action' Error_OpcodeArgumentMustBeDefinition(Position: POS)
'action' Error_IllegalNumberOfArgumentsForOpcode(Position: POS)

'action' Error_BytecodeNotAllowedInSafeContext(Position: POS)
'action' Error_UnsafeHandlerCallNotAllowedInSafeContext(Position: POS, Identifier: NAME)

'action' Error_VariadicParametersOnlyAllowedInForeignHandlers(Position: POS)
'action' Error_VariadicParameterMustBeLast(Position: POS)
'action' Error_VariadicArgumentNotExplicitlyTyped(Position: POS)

'action' Warning_MetadataClausesShouldComeAfterUseClauses(Position: POS)
'action' Warning_DeprecatedTypeName(Position: POS, NewType: STRING)
'action' Warning_UnsuitableNameForDefinition(Position: POS, Identifier: NAME)
'action' Warning_UnsuitableNameForNamespace(Position: POS, Identifier: NAME)
'action' Warning_DeprecatedSyntax(Position: POS, Message: STRING)

--------------------------------------------------------------------------------
