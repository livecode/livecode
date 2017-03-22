/* Copyright (C) 2016 LiveCode Ltd.
 
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
    NegateReal

    InitializePosition
    FinalizePosition
    AdvanceCurrentPosition
    AdvanceCurrentPositionToNextLine
    GetColumnOfCurrentPosition
    GetUndefinedPosition
    AddImportedModuleFile
    GetFilenameOfPosition

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

    InitializeLiterals
    FinalizeLiterals
    MakeIntegerLiteral
    MakeDoubleLiteral
    MakeStringLiteral
    UnescapeStringLiteral
    MakeNameLiteral
    GetStringOfNameLiteral
    ConcatenateNameParts
    JavaQualifiedNameToClassPath
    IsNameEqualToName
    IsNameNotEqualToName
    IsNameEqualToString
    IsStringEqualToString

    IsNameSuitableForDefinition
    IsStringSuitableForKeyword

    OutputBegin
    OutputLCBBegin
    OutputEnd
    OutputWrite
    OutputWriteI
    OutputWriteS
    OutputWriteN
    OutputWriteD

    GetOutputLCBModuleName

    ErrorsDidOccur
    Fatal_OutOfMemory
    Fatal_InternalInconsistency
    Error_UnableToFindImportedPackage
    Error_UnableToFindImportedDefinition

    Error_ClassesMayOnlyInheritFromClasses
    Error_ClassesMayOnlyImplementInterfaces
    Error_InterfacesMayOnlyInheritFromInterfaces

    Error_GenericTypeMismatch

    Error_ConstantsMustBeSimple

    Error_MalformedToken
    Error_MalformedSyntax
    Error_MalformedEscapedString
    Error_IdentifierPreviouslyDeclared
    Error_IdentifierNotDeclared
    Error_NotBoundToAType
    Error_NotBoundToAPhrase
    Error_NotBoundToASyntaxRule
    Error_NotBoundToASyntaxMark
    Error_NotBoundToAHandler
    Error_NotBoundToAVariable
    Error_NotBoundToAConstantSyntaxValue
    Error_NotBoundToAVariableOrHandler
    Error_NotBoundToAConstantOrVariableOrHandler

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

'action' ConcatenateNameParts(NAME, NAME -> NAME)
'action' JavaQualifiedNameToClassPath(NAME -> NAME)

'condition' IsNameEqualToString(NAME, STRING)
'condition' IsStringEqualToString(STRING, STRING)
'condition' IsNameEqualToName(NAME, NAME)
'condition' IsNameNotEqualToName(NAME, NAME)

'condition' IsNameSuitableForDefinition(NAME)
'condition' IsStringSuitableForKeyword(STRING)

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

'action' OutputBegin()
'action' OutputLCBBegin()
'action' OutputEnd()
'action' OutputWrite(STRING)
'action' OutputWriteI(STRING, NAME, STRING)
'action' OutputWriteS(STRING, STRING, STRING)
'action' OutputWriteN(STRING, INT, STRING)
'action' OutputWriteD(STRING, DOUBLE, STRING)

--------------------------------------------------------------------------------

'action' GetOutputLCBModuleName(-> STRING)

--------------------------------------------------------------------------------

'condition' ErrorsDidOccur()

'action' Fatal_OutOfMemory()
'action' Fatal_InternalInconsistency(Message: STRING)

'action' Error_UnableToFindImportedPackage(Position: POS, Name: NAME)
'action' Error_UnableToFindImportedDefinition(Position: POS, Name: NAME)

'action' Error_ClassesMayOnlyInheritFromClasses(Position: POS)
'action' Error_ClassesMayOnlyImplementInterfaces(Position: POS)
'action' Error_InterfacesMayOnlyInheritFromInterfaces(Position: POS)

'action' Error_GenericTypeMismatch(Position: POS)

'action' Error_MalformedToken(Position: POS, Token: STRING)
'action' Error_MalformedSyntax(Position: POS)
'action' Error_MalformedEscapedString(Position: POS, Token: STRING)
'action' Error_IdentifierPreviouslyDeclared(Position: POS, Identifier: NAME, PreviousPosition: POS)
'action' Error_IdentifierNotDeclared(Position: POS, Identifier: NAME)

'action' Error_ConstantsMustBeSimple(Position: POS)

'action' Error_NotBoundToAType(Position: POS, Name: NAME)
'action' Error_NotBoundToAPhrase(Position: POS, Name: NAME)
'action' Error_NotBoundToASyntaxRule(Position: POS, Name: NAME)
'action' Error_NotBoundToASyntaxMark(Position: POS, Name: NAME)
'action' Error_NotBoundToAConstantSyntaxValue(Position: POS, Name: NAME)
'action' Error_NotBoundToAHandler(Position: POS, Name: QUALIFIEDNAME)
'action' Error_NotBoundToAVariable(Position: POS, Name: NAME)
'action' Error_NotBoundToAVariableOrHandler(Position: POS, Name: NAME)
'action' Error_NotBoundToAConstantOrVariableOrHandler(Position: POS, Name: NAME)

--------------------------------------------------------------------------------
