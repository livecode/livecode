'module' generate

'use'
    types
    support

'export'
    Generate

--------------------------------------------------------------------------------

'action' Generate(MODULE)

    'rule' Generate(module(_, Id, Imports, Definitions)):
        QueryModuleId(Id -> Info)
        Id'Name -> ModuleName
        EmitBeginModule(ModuleName -> ModuleIndex)
        Info'Index <- ModuleIndex

        -- Emit all imported declarations and dependent modules.
        GenerateImportedDefinitions(Definitions)

        -- Declare indices for all definitions in this module (so mutally referential
        -- definitions work).
        GenerateDefinitionIndexes(Definitions)
        
        -- Generate all definitions from this module.
        GenerateDefinitions(Definitions)
        
        -- Generate all the exported definitions
        GenerateExportedDefinitions(Definitions)
        
        EmitEndModule()

----------

-- Iterate over the tree, generating declarations for all used imported
-- definitions.
'sweep' GenerateImportedDefinitions(ANY)

    'rule' GenerateImportedDefinitions(TYPE'named(Position, Name)):
        QuerySymbolId(Name -> Info)
        Info'Type -> Type
        GenerateImportedDefinitions(Type)
        GenerateImportedDefinition(Name)
        
    'rule' GenerateImportedDefinitions(STATEMENT'call(_, Handler, Arguments)):
        GenerateImportedDefinition(Handler)
        GenerateImportedDefinitions(Arguments)

    --'rule' GenerateImportedDefinitions(STATEMENT'invoke(_, Info, Arguments)):
    --    GenerateImportedInvokeDefinition(Info)
    --    GenerateImportedDefinitions(Arguments)
        
    'rule' GenerateImportedDefinitions(EXPRESSION'slot(_, Name)):
        GenerateImportedDefinition(Name)

    'rule' GenerateImportedDefinitions(EXPRESSION'call(_, Handler, Arguments)):
        GenerateImportedDefinition(Handler)
        GenerateImportedDefinitions(Arguments)

    --'rule' GenerateImportedDefinitions(EXPRESSION'invoke(_, Info, Arguments)):
    --    GenerateImportedInvokeDefinition(Info)
    --    GenerateImportedDefinitions(Arguments)

'action' GenerateImportedDefinition(ID)

    'rule' GenerateImportedDefinition(Id):
        -- Only generate the an imported definition if it comes from another module.
        IsUngeneratedExternalId(Id)
        
        -- Get the module in which the definition is defined
        QueryModuleOfId(Id -> ModuleId)
        
        -- Ensure we have a dependency for the module
        GenerateModuleDependency(ModuleId -> ModuleIndex)
        
        -- Fetch the info about the symbol.
        QuerySymbolId(Id -> SymbolInfo)
        Id'Name -> SymbolName
        SymbolInfo'Kind -> SymbolKind
        SymbolInfo'Type -> SymbolType
        
        -- Generate a type for the symbol (this is unused for now so just use undefined)
        -- GenerateType(SymbolType -> SymbolTypeIndex)
        EmitUndefinedType(-> SymbolTypeIndex)

        -- Now add the import definition to the module
        (|
            where(SymbolKind -> type)
            EmitImportedType(ModuleIndex, SymbolName, SymbolTypeIndex -> SymbolIndex)
        ||
            where(SymbolKind -> constant)
            EmitImportedConstant(ModuleIndex, SymbolName, SymbolTypeIndex -> SymbolIndex)
        ||
            where(SymbolKind -> variable)
            EmitImportedVariable(ModuleIndex, SymbolName, SymbolTypeIndex -> SymbolIndex)
        ||
            where(SymbolKind -> handler)
            EmitImportedHandler(ModuleIndex, SymbolName, SymbolTypeIndex -> SymbolIndex)
        |)
        
        -- We now have an index of an 'external definition' to use when referencing it.
        SymbolInfo'Index <- SymbolIndex
        
    'rule' GenerateImportedDefinition(Id):
        -- If we get here then either the id isn't imported, or we have previously
        -- generated it.

/*
'action' GenerateImportedInvokeDefinition(INVOKELIST)

    'rule' GenerateImportedInvokeDefinition(invokelist(Info, Tail)):
        GenerateImportedInvokeInfo(Info)
        GenerateImportedInvokeDefinition(Tail)
        
    'rule' GenerateImportedInvokeDefinition(nil):
        -- do nothing

'action' GenerateImportedInvokeInfo(INVOKEINFO)

    'rule' GenerateImportedInvokeInfo(Info):
        Info'Index -> Index
        eq(Index, -1)
        
        Info'ModuleName -> ModuleNameString
        MakeNameLiteral(ModuleNameString -> ModuleName)
        EmitModuleDependency(ModuleName -> ModuleIndex)
        Info'ModuleIndex <- ModuleIndex
        
        Info'Name -> NameString
        MakeNameLiteral(NameString -> Name)
        EmitUndefinedType(-> SymbolTypeIndex)
        EmitImportedSyntax(ModuleIndex, Name, SymbolTypeIndex -> SymbolIndex)
        Info'Index <- SymbolIndex

    'rule' GenerateImportedInvokeInfo(Info):
        -- If we get here then we've previously processed this invoke.
*/

----

'action' GenerateModuleDependency(ID -> INT)
    
    'rule' GenerateModuleDependency(Id -> ModuleIndex):
        -- Get info about the module
        QueryModuleId(Id -> ModuleInfo)
        
        -- Fetch the module index
        ModuleInfo'Index -> CurrentModuleIndex
        [|
            -- If the module has been depended on yet, it will have index -1
            eq(CurrentModuleIndex, -1)
            Id'Name -> ModuleName
            
            -- Emit a dependency for the module and get its index
            EmitModuleDependency(ModuleName -> NewModuleIndex)
            ModuleInfo'Index <- NewModuleIndex
        |]
        
        -- Now return the updated module index
        ModuleInfo'Index -> ModuleIndex

----

'action' GenerateDefinitionIndexes(DEFINITION)

    'rule' GenerateDefinitionIndexes(sequence(Left, Right)):
        GenerateDefinitionIndexes(Left)
        GenerateDefinitionIndexes(Right)
        
    'rule' GenerateDefinitionIndexes(type(_, _, Name, _)):
        GenerateDefinitionIndex(Name)
    
    'rule' GenerateDefinitionIndexes(constant(_, _, Name, _)):
        -- GenerateDefinitionIndex(Name)
    
    'rule' GenerateDefinitionIndexes(variable(_, _, Name, _)):
        GenerateDefinitionIndex(Name)
    
    'rule' GenerateDefinitionIndexes(handler(_, _, Name, _, _, _)):
        GenerateDefinitionIndex(Name)

    'rule' GenerateDefinitionIndexes(foreignhandler(_, _, Name, _, _)):
        GenerateDefinitionIndex(Name)

    'rule' GenerateDefinitionIndexes(syntax(_, _, Name, _, _, _)):
        GenerateDefinitionIndex(Name)
    
    'rule' GenerateDefinitionIndexes(_):
        -- nothing

'action' GenerateDefinitionIndex(ID)

    'rule' GenerateDefinitionIndex(Id):
        QuerySyntaxId(Id -> Info)
        EmitDefinitionIndex(-> Index)
        Info'Index <- Index

    'rule' GenerateDefinitionIndex(Id):
        QuerySymbolId(Id -> Info)
        EmitDefinitionIndex(-> Index)
        Info'Index <- Index

'condition' IsRValueMethodPresent(SYNTAXMETHODLIST)

    'rule' IsRValueMethodPresent(methodlist(method(_, _, Arguments), Tail)):
        (|
            IsMarkTypePresentInArguments(output, Arguments)
        ||
            IsRValueMethodPresent(Tail)
        |)
        
'condition' IsLValueMethodPresent(SYNTAXMETHODLIST)

    'rule' IsLValueMethodPresent(methodlist(method(_, _, Arguments), Tail)):
        (|
            IsMarkTypePresentInArguments(input, Arguments)
        ||
            IsRValueMethodPresent(Tail)
        |)

'condition' IsMarkTypePresentInArguments(SYNTAXMARKTYPE, SYNTAXCONSTANTLIST)

    'rule' IsMarkTypePresentInArguments(MarkType, constantlist(variable(_, Name), _)):
        QuerySyntaxMarkId(Name -> Info)
        Info'Type -> Type
        eq(MarkType, Type)
        
    'rule' IsMarkTypePresentInArguments(MarkType, constantlist(indexedvariable(_, Name, _), _)):
        QuerySyntaxMarkId(Name -> Info)
        Info'Type -> Type
        eq(MarkType, Type)

    'rule' IsMarkTypePresentInArguments(MarkType, constantlist(_, Tail)):
        IsMarkTypePresentInArguments(MarkType, Tail)

----

'action' GenerateExportedDefinitions(DEFINITION)

    'rule' GenerateExportedDefinitions(sequence(Left, Right)):
        GenerateExportedDefinitions(Left)
        GenerateExportedDefinitions(Right)
        
    'rule' GenerateExportedDefinitions(type(_, public, Id, _)):
        GenerateExportedDefinition(Id)
        
    'rule' GenerateExportedDefinitions(constant(_, public, Id, _)):
        GenerateExportedDefinition(Id)
        
    'rule' GenerateExportedDefinitions(variable(_, public, Id, _)):
        GenerateExportedDefinition(Id)

    'rule' GenerateExportedDefinitions(handler(_, public, Id, _, _, _)):
        GenerateExportedDefinition(Id)

    'rule' GenerateExportedDefinitions(foreignhandler(_, public, Id, _, _)):
        GenerateExportedDefinition(Id)
        
    'rule' GenerateExportedDefinitions(syntax(_, _, Id, _, _, _)):
        GenerateExportedDefinition(Id)
        
    'rule' GenerateExportedDefinitions(_):
        -- Non-public, non-exportable definition fallthrough.

'action' GenerateExportedDefinition(ID)

    'rule' GenerateExportedDefinition(Id):
        QuerySyntaxId(Id -> Info)
        Info'Index -> Index
        ne(Index, -1)
        EmitExportedDefinition(Index)

    'rule' GenerateExportedDefinition(Id):
        QuerySymbolId(Id -> Info)
        Info'Index -> Index
        EmitExportedDefinition(Index)

----

'action' GenerateDefinitions(DEFINITION)

    'rule' GenerateDefinitions(sequence(Left, Right)):
        GenerateDefinitions(Left)
        GenerateDefinitions(Right)
        
    'rule' GenerateDefinitions(type(Position, _, Id, Type)):
        GenerateType(Type -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitTypeDefinition(DefIndex, Position, Name, TypeIndex)
        
    'rule' GenerateDefinitions(constant(Position, _, Id, Value)):
        QuerySymbolId(Id -> Info)
        -- Do something
        
    'rule' GenerateDefinitions(variable(Position, _, Id, Type)):
        GenerateType(Type -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitVariableDefinition(DefIndex, Position, Name, TypeIndex)

    'rule' GenerateDefinitions(handler(Position, _, Id, Signature:signature(Parameters, _), _, Body)):
        GenerateType(handler(Position, Signature) -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitBeginHandlerDefinition(DefIndex, Position, Name, TypeIndex)
        GenerateParameters(Parameters)
        EmitCreateRegister(-> ContextReg)
        EmitCreateRegister(-> ResultReg)
        GenerateBody(ResultReg, ContextReg, Body)
        EmitDestroyRegister(ContextReg)
        EmitDestroyRegister(ResultReg)
        EmitReturnNothing()
        EmitEndHandlerDefinition()

    'rule' GenerateDefinitions(foreignhandler(Position, _, Id, Signature, Binding)):
        GenerateType(handler(Position, Signature) -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitForeignHandlerDefinition(DefIndex, Position, Name, TypeIndex, Binding)
        
    'rule' GenerateDefinitions(property(Position, _, Id, Getter, Setter)):
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        QuerySymbolId(Getter -> GetInfo)
        GetInfo'Index -> GetIndex
        QuerySymbolId(Setter -> SetInfo)
        SetInfo'Index -> SetIndex
        EmitPropertyDefinition(DefIndex, Position, Name, GetIndex, SetIndex)
        
    'rule' GenerateDefinitions(event(Position, _, Id, Signature)):
        GenerateType(handler(Position, Signature) -> TypeIndex)

        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitEventDefinition(DefIndex, Position, Name, TypeIndex)
        
    'rule' GenerateDefinitions(syntax(Position, _, Id, Class, _, _)):
        QuerySyntaxId(Id -> Info)
        Id'Name -> Name
        Info'Methods -> Methods
        Info'Index -> Index
        EmitBeginSyntaxDefinition(Index, Position, Name)
        GenerateSyntaxMethods(Methods)
        EmitEndSyntaxDefinition()
        
    'rule' GenerateDefinitions(nil):
        -- nothing

----

'action' GenerateSyntaxMethods(SYNTAXMETHODLIST)

    'rule' GenerateSyntaxMethods(methodlist(method(Position, Id, Arguments), Tail)):
        QuerySymbolId(Id -> Info)
        Info'Index -> HandlerIndex
        EmitBeginSyntaxMethod(HandlerIndex)
        GenerateSyntaxMethodArguments(Arguments)
        EmitEndSyntaxMethod()
        
    'rule' GenerateSyntaxMethods(nil):
        -- do nothing

'action' GenerateSyntaxMethodArguments(SYNTAXCONSTANTLIST)

    'rule' GenerateSyntaxMethodArguments(constantlist(Head, Tail)):
        GenerateSyntaxMethodArgument(Head)
        GenerateSyntaxMethodArguments(Tail)
        
    'rule' GenerateSyntaxMethodArguments(nil):
        -- do nothing

'action' GenerateSyntaxMethodArgument(SYNTAXCONSTANT)

    'rule' GenerateSyntaxMethodArgument(undefined(_)):
        EmitUndefinedSyntaxMethodArgument()

    'rule' GenerateSyntaxMethodArgument(true(_)):
        EmitTrueSyntaxMethodArgument()

    'rule' GenerateSyntaxMethodArgument(false(_)):
        EmitFalseSyntaxMethodArgument()

    'rule' GenerateSyntaxMethodArgument(integer(_, Value)):
        EmitIntegerSyntaxMethodArgument(Value)

    'rule' GenerateSyntaxMethodArgument(real(_, Value)):
        EmitRealSyntaxMethodArgument(Value)

    'rule' GenerateSyntaxMethodArgument(string(_, Value)):
        EmitStringSyntaxMethodArgument(Value)

    'rule' GenerateSyntaxMethodArgument(variable(_, Name)):
        QuerySyntaxMarkId(Name -> Info)
        (|
            Info'Type -> context
            EmitContextSyntaxMethodArgument()
        ||
            Info'Type -> input
            EmitInputSyntaxMethodArgument()
        ||
            Info'Type -> output
            EmitOutputSyntaxMethodArgument()
        ||
            Info'Type -> iterator
            EmitIteratorSyntaxMethodArgument()
        ||
            Info'Type -> container
            EmitContainerSyntaxMethodArgument()
        ||
            Info'Index -> Index
            EmitVariableSyntaxMethodArgument(Index)
        |)

    'rule' GenerateSyntaxMethodArgument(indexedvariable(_, Name, ElementIndex)):
        QuerySyntaxMarkId(Name -> Info)
        Info'Index -> Index
        EmitIndexedVariableSyntaxMethodArgument(Index, ElementIndex)

----

'action' GenerateParameters(PARAMETERLIST)

    'rule' GenerateParameters(parameterlist(parameter(_, _, Id, Type), Rest)):
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        GenerateType(Type -> TypeIndex)
        EmitHandlerParameter(Name, TypeIndex -> Index)
        Info'Index <- Index
        GenerateParameters(Rest)

    'rule' GenerateParameters(nil):
        -- nothing
        

'action' GenerateBody(INT, INT, STATEMENT)

    'rule' GenerateBody(Result, Context, sequence(Left, Right)):
        GenerateBody(Result, Context, Left)
        GenerateBody(Result, Context, Right)

    'rule' GenerateBody(Result, Context, variable(Position, Id, Type)):
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        GenerateType(Type -> TypeIndex)
        EmitHandlerVariable(Name, TypeIndex -> Index)
        Info'Index <- Index
    
    'rule' GenerateBody(Result, Context, if(Position, Condition, Consequent, Alternate)):
        EmitDeferLabel(-> AlternateLabel)
        EmitDeferLabel(-> EndIfLabel)
        EmitPosition(Position)
        GenerateExpression(Context, Condition -> ResultRegister)
        EmitJumpIfFalse(ResultRegister, AlternateLabel)
        EmitDestroyRegister(ResultRegister)
        GenerateBody(Result, Context, Consequent)
        EmitJump(EndIfLabel)
        EmitResolveLabel(AlternateLabel)
        GenerateBody(Result, Context, Alternate)
        EmitResolveLabel(EndIfLabel)
        
    'rule' GenerateBody(Result, Context, repeatforever(Position, Body)):
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPosition(Position)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)
        EmitResolveLabel(RepeatHead)
        GenerateBody(Result, Context, Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitPopRepeatLabels()
        
    'rule' GenerateBody(Result, Context, repeatcounted(Position, Count, Body)):
        -- repeat n times uses a builtin invoke:
        -- bool RepeatCountedIterator(inout count)
        --   if count == 0 then return false
        --   count -= 1
        --   return true
        GenerateExpression(Context, Count -> CountRegister)
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)

        EmitPosition(Position)
        EmitResolveLabel(RepeatHead)
        EmitCreateRegister(-> ContinueRegister)
        EmitBeginBuiltinInvoke("RepeatCounted", ContinueRegister)
        EmitContinueInvoke(CountRegister)
        EmitEndInvoke()
        EmitJumpIfFalse(CountRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)
        GenerateBody(Result, Context, Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitDestroyRegister(CountRegister)
        EmitPopRepeatLabels()
        
    'rule' GenerateBody(Result, Context, repeatwhile(Position, Condition, Body)):
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)
        
        EmitPosition(Position)
        EmitResolveLabel(RepeatHead)
        GenerateExpression(Context, Condition -> ContinueRegister)
        EmitJumpIfFalse(ContinueRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)
        GenerateBody(Result, Context, Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitPopRepeatLabels()

    'rule' GenerateBody(Result, Context, repeatuntil(Position, Condition, Body)):
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)
        
        EmitPosition(Position)
        EmitResolveLabel(RepeatHead)
        GenerateExpression(Context, Condition -> ContinueRegister)
        EmitJumpIfTrue(ContinueRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)
        GenerateBody(Result, Context, Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitPopRepeatLabels()
        
    'rule' GenerateBody(Result, Context, repeatupto(Position, Slot, Start, Finish, Step, Body)):
        QuerySymbolId(Slot -> Info)
        Info'Index -> VarIndex
        Info'Kind -> VarKind

        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatNext)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatNext, RepeatTail)
        
        EmitPosition(Position)
        GenerateExpression(Context, Start -> CounterRegister)
        GenerateExpression(Context, Finish -> LimitRegister)
        (|
            where(Step -> nil)
            EmitCreateRegister(-> StepRegister)
            EmitAssignInteger(StepRegister, 1)
        ||
            GenerateExpression(Context, Step -> StepRegister)
        |)

        EmitResolveLabel(RepeatHead)
        EmitCreateRegister(-> ContinueRegister)
        EmitBeginBuiltinInvoke("RepeatUpToCondition", ContinueRegister)
        EmitContinueInvoke(CounterRegister)
        EmitContinueInvoke(LimitRegister)
        EmitEndInvoke()

        EmitStoreVar(VarKind, CounterRegister, VarIndex)
        EmitJumpIfFalse(ContinueRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)

        GenerateBody(Result, Context, Body)

        EmitResolveLabel(RepeatNext)
        EmitFetchVar(VarKind, CounterRegister, VarIndex)
        EmitBeginBuiltinInvoke("RepeatUpToIterate", CounterRegister)
        EmitContinueInvoke(CounterRegister)
        EmitContinueInvoke(StepRegister)
        EmitEndInvoke()

        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitDestroyRegister(CounterRegister)
        EmitDestroyRegister(LimitRegister)
        EmitDestroyRegister(StepRegister)

    'rule' GenerateBody(Result, Context, repeatdownto(Position, Slot, Start, Finish, Step, Body)):
        QuerySymbolId(Slot -> Info)
        Info'Index -> VarIndex
        Info'Kind -> VarKind

        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatNext)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatNext, RepeatTail)
        
        EmitPosition(Position)
        GenerateExpression(Context, Start -> CounterRegister)
        GenerateExpression(Context, Finish -> LimitRegister)
        (|
            where(Step -> nil)
            EmitCreateRegister(-> StepRegister)
            EmitAssignInteger(StepRegister, -1)
        ||
            GenerateExpression(Context, Step -> StepRegister)
        |)

        EmitResolveLabel(RepeatHead)
        EmitCreateRegister(-> ContinueRegister)
        EmitBeginBuiltinInvoke("RepeatDownToCondition", ContinueRegister)
        EmitContinueInvoke(CounterRegister)
        EmitContinueInvoke(LimitRegister)
        EmitEndInvoke()

        EmitStoreVar(VarKind, CounterRegister, VarIndex)
        EmitJumpIfFalse(ContinueRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)

        GenerateBody(Result, Context, Body)

        EmitResolveLabel(RepeatNext)
        EmitFetchVar(VarKind, CounterRegister, VarIndex)
        EmitBeginBuiltinInvoke("RepeatDownToIterate", CounterRegister)
        EmitContinueInvoke(CounterRegister)
        EmitContinueInvoke(StepRegister)
        EmitEndInvoke()
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitDestroyRegister(CounterRegister)
        EmitDestroyRegister(LimitRegister)
        EmitDestroyRegister(StepRegister)
        
    'rule' GenerateBody(Result, Context, repeatforeach(Position, Iterator, Slot, Container, Body)):
        -- TODO
        
    'rule' GenerateBody(Result, Context, nextrepeat(Position)):
        EmitCurrentRepeatLabels(-> Next, _)
        EmitPosition(Position)
        EmitJump(Next)
        
    'rule' GenerateBody(Result, Context, exitrepeat(Position)):
        EmitCurrentRepeatLabels(-> _, Exit)
        EmitPosition(Position)
        EmitJump(Exit)
        
    'rule' GenerateBody(Result, Context, return(Position, Value)):
        EmitPosition(Position)
        GenerateExpression(Context, Value -> ReturnReg)
        EmitReturn(ReturnReg)
        
    'rule' GenerateBody(Result, Context, call(Position, Handler, Arguments)):
        EmitPosition(Position)
        GenerateCallInRegister(Result, Context, Position, Handler, Arguments)

    'rule' GenerateBody(Result, Context, put(Position, Source, Target)):
        EmitPosition(Position)
        GenerateInvoke_EvaluateArgumentForIn(Context, Source)
        GenerateInvoke_EvaluateArgumentForOut(Context, Target)
        EmitGetRegisterAttachedToExpression(Source -> SrcReg)
        EmitGetRegisterAttachedToExpression(Target -> DstReg)
        EmitAssign(DstReg, SrcReg)
        GenerateInvoke_AssignArgument(Context, Target)
        GenerateInvoke_FreeArgument(Source)
        GenerateInvoke_FreeArgument(Target)

    'rule' GenerateBody(Result, Context, invoke(Position, Invokes, Arguments)):
        EmitPosition(Position)
        GenerateDefinitionGroupForInvokes(Invokes, execute, Arguments -> Index, Signature)
        GenerateInvoke_EvaluateArguments(Context, Signature, Arguments)
        EmitBeginExecuteInvoke(Index, Context, Result)
        GenerateInvoke_EmitInvokeArguments(Arguments)
        EmitEndInvoke()
        GenerateInvoke_AssignArguments(Context, Signature, Arguments)
        GenerateInvoke_FreeArguments(Arguments)
        
    'rule' GenerateBody(Result, Context, nil):
        -- nothing

----

-- Evaluate the arguments for the invoke with the given signature. This attaches
-- a register to each expression which is used to perform the invoke.
-- For 'in' arguments the expression is evaluated as normal as no assign-invoke
-- is required.
-- For 'inout' arguments the expression is evaluated as normal but if the expr is
-- an invoke, its argument registers are retained for use in the corresponding
-- assign-invoke.
-- For 'out' arguments the expression is not evaluated, but if the expr is an invoke
-- then its arguments are evaluated ready for the corresponding assign-invoke.
--
'action' GenerateInvoke_EvaluateArguments(INT, INVOKESIGNATURE, EXPRESSIONLIST)

    'rule' GenerateInvoke_EvaluateArguments(ContextReg, invokesignature(in, _, SigRest), expressionlist(Expr, ArgRest)):
        GenerateInvoke_EvaluateArgumentForIn(ContextReg, Expr)
        GenerateInvoke_EvaluateArguments(ContextReg, SigRest, ArgRest)
        
    'rule' GenerateInvoke_EvaluateArguments(ContextReg, invokesignature(inout, _, SigRest), expressionlist(Expr, ArgRest)):
        GenerateInvoke_EvaluateArgumentForInOut(ContextReg, Expr)
        GenerateInvoke_EvaluateArguments(ContextReg, SigRest, ArgRest)

    'rule' GenerateInvoke_EvaluateArguments(ContextReg, invokesignature(out, _, SigRest), expressionlist(Expr, ArgRest)):
        GenerateInvoke_EvaluateArgumentForOut(ContextReg, Expr)
        GenerateInvoke_EvaluateArguments(ContextReg, SigRest, ArgRest)
        
    'rule' GenerateInvoke_EvaluateArguments(_, nil, nil):
        -- do nothing.

-- In arguments are simple, just evaluate the expr into a register attached to the
-- node.
'action' GenerateInvoke_EvaluateArgumentForIn(INT, EXPRESSION)

    'rule' GenerateInvoke_EvaluateArgumentForIn(ContextReg, Expr):
        EmitCreateRegister(-> ResultReg)
        EmitAttachRegisterToExpression(ResultReg, Expr)
        GenerateExpressionInRegister(ResultReg, ContextReg, Expr)

-- Out arguments are a little bit trickier. If the expr is an invoke then we must
-- evaluate its arguments, but do no more. Otherwise, it has to be something we
-- can store in (i.e. slot). If it is neither of these things then our compiler has
-- not checked things properly!
'action' GenerateInvoke_EvaluateArgumentForOut(INT, EXPRESSION)

    'rule' GenerateInvoke_EvaluateArgumentForOut(ContextReg, Invoke:invoke(_, Invokes, Arguments)):
        EmitCreateRegister(-> ResultReg)
        EmitAttachRegisterToExpression(ResultReg, Invoke)
        GenerateDefinitionGroupForInvokes(Invokes, assign, Arguments -> Index, Signature)
        GenerateInvoke_EvaluateArguments(ContextReg, Signature, Arguments)

    'rule' GenerateInvoke_EvaluateArgumentForOut(ContextReg, Slot:slot(_, _)):
        EmitCreateRegister(-> ResultReg)
        EmitAttachRegisterToExpression(ResultReg, Slot)
        
    'rule' GenerateInvoke_EvaluateArgumentForOut(ContextReg, _):
        Fatal_InternalInconsistency("Invalid expression for out argument not checked properly!")

-- Inout arguments are a combination of 'in' and 'out'. If the expr is an invoke then
-- we must evaluate its arguments and call its evaluate side. Otherwise the expr has to
-- be a slot which we must also evaluate. Anything else is an inconsistency.
'action' GenerateInvoke_EvaluateArgumentForInOut(INT, EXPRESSION)

    'rule' GenerateInvoke_EvaluateArgumentForInOut(ContextReg, Invoke:invoke(_, Invokes, Arguments)):
        EmitCreateRegister(-> OutputReg)
        EmitAttachRegisterToExpression(OutputReg, Invoke)
        GenerateDefinitionGroupForInvokes(Invokes, evaluate, Arguments -> Index, Signature)
        GenerateInvoke_EvaluateArguments(ContextReg, Signature, Arguments)
        EmitCreateRegister(-> IgnoredResultReg)
        EmitBeginExecuteInvoke(Index, ContextReg, IgnoredResultReg)
        GenerateInvoke_EmitInvokeArguments(Arguments)
        EmitContinueInvoke(OutputReg)
        EmitEndInvoke()
        EmitDestroyRegister(IgnoredResultReg)
        GenerateInvoke_AssignArguments(ContextReg, Signature, Arguments)

    'rule' GenerateInvoke_EvaluateArgumentForInOut(ContextReg, Slot:slot(_, _)):
        EmitCreateRegister(-> ResultReg)
        EmitAttachRegisterToExpression(ResultReg, Slot)
        GenerateExpressionInRegister(ResultReg, ContextReg, Slot)
        
    'rule' GenerateInvoke_EvaluateArgumentForInOut(ContextReg, _):
        Fatal_InternalInconsistency("Invalid expression for inout argument not checked properly!")

----

'action' GenerateInvoke_AssignArguments(INT, INVOKESIGNATURE, EXPRESSIONLIST)

    'rule' GenerateInvoke_AssignArguments(ContextReg, invokesignature(in, _, SigRest), expressionlist(Expr, ArgRest)):
        -- nothing to do for in arguments
        GenerateInvoke_AssignArguments(ContextReg, SigRest, ArgRest)
        
    'rule' GenerateInvoke_AssignArguments(ContextReg, invokesignature(_, _, SigRest), expressionlist(Expr, ArgRest)):
        -- out and inout are the same
        GenerateInvoke_AssignArgument(ContextReg, Expr)
        GenerateInvoke_AssignArguments(ContextReg, SigRest, ArgRest)

    'rule' GenerateInvoke_AssignArguments(_, nil, nil):
        -- do nothing.
        
'action' GenerateInvoke_AssignArgument(INT, EXPRESSION)

    'rule' GenerateInvoke_AssignArgument(ContextReg, Invoke:invoke(_, Invokes, Arguments)):
        EmitGetRegisterAttachedToExpression(Invoke -> InputReg)
        GenerateDefinitionGroupForInvokes(Invokes, evaluate, Arguments -> Index, Signature)
        EmitCreateRegister(-> IgnoredResultReg)
        EmitBeginExecuteInvoke(Index, ContextReg, IgnoredResultReg)
        EmitContinueInvoke(InputReg)
        GenerateInvoke_EmitInvokeArguments(Arguments)
        EmitEndInvoke()
        EmitDestroyRegister(IgnoredResultReg)
        
    'rule' GenerateInvoke_AssignArgument(ContextReg, Slot:slot(_, Id)):
        EmitGetRegisterAttachedToExpression(Slot -> InputReg)
        QuerySymbolId(Id -> Info)
        Info'Kind -> Kind
        Info'Index -> Index
        EmitStoreVar(Kind, InputReg, Index)

----

'action' GenerateInvoke_FreeArguments(EXPRESSIONLIST)

    'rule' GenerateInvoke_FreeArguments(expressionlist(Expr, Rest)):
        GenerateInvoke_FreeArgument(Expr)
        GenerateInvoke_FreeArguments(Rest)
        
    'rule' GenerateInvoke_FreeArguments(nil):
        -- do nothing
        
'action' GenerateInvoke_FreeArgument(EXPRESSION)

    'rule' GenerateInvoke_FreeArgument(Expr:invoke(_, _, Arguments)):
        [|
            EmitGetRegisterAttachedToExpression(Expr -> Reg)
            EmitDestroyRegister(Reg)
            EmitDetachRegisterFromExpression(Expr)
        |]
        GenerateInvoke_FreeArguments(Arguments)
    
    'rule' GenerateInvoke_FreeArgument(Expr):
        [|
            EmitGetRegisterAttachedToExpression(Expr -> Reg)
            EmitDestroyRegister(Reg)
            EmitDetachRegisterFromExpression(Expr)
        |]

    'rule' GenerateInvoke_FreeArgument(_):
        -- nothing to do

----

'action' GenerateInvoke_EmitInvokeArguments(EXPRESSIONLIST)

    'rule' GenerateInvoke_EmitInvokeArguments(expressionlist(Head, Tail)):
        EmitGetRegisterAttachedToExpression(Head -> Reg)
        EmitContinueInvoke(Reg)
        GenerateInvoke_EmitInvokeArguments(Tail)

    'rule' GenerateInvoke_EmitInvokeArguments(nil):
        -- do nothings

----

/*
'action' GenerateInvoke_GetExecuteSignature(INVOKELIST -> INVOKESIGNATURE)

    'rule' GenerateInvoke_GetExecuteSignature(invokelist(Head, _) -> Signature):
        Head'RSignature -> Signature

'action' GenerateInvoke_GetEvaluateSignature(INVOKELIST -> INVOKESIGNATURE)

    'rule' GenerateInvoke_GetEvaluateSignature(invokelist(Head, _) -> Signature):
        Head'RSignature -> Signature

'action' GenerateInvoke_GetAssignSignature(INVOKELIST -> INVOKESIGNATURE)

    'rule' GenerateInvoke_GetAssignSignature(invokelist(Head, _) -> Signature):
        Head'LSignature -> Signature
*/

----

'action' GenerateDefinitionGroupForInvokes(INVOKELIST, INVOKEMETHODTYPE, EXPRESSIONLIST -> INT, INVOKESIGNATURE)

    'rule' GenerateDefinitionGroupForInvokes(InvokeList, Type, Arguments -> Index, Signature)
        EmitBeginDefinitionGroup()
        GenerateDefinitionGroupForInvokeList(InvokeList, Type, Arguments -> Signature)
        EmitEndDefinitionGroup(-> Index)

'action' GenerateDefinitionGroupForInvokeList(INVOKELIST, INVOKEMETHODTYPE, EXPRESSIONLIST -> INVOKESIGNATURE)

    'rule' GenerateDefinitionGroupForInvokeList(invokelist(Head, Tail), Type, Arguments -> Signature):
        Head'ModuleName -> ModuleName
        Head'Methods -> Methods
        GenerateDefinitionGroupForInvokeMethodList(ModuleName, Type, Arguments, Methods -> HeadSig)
        GenerateDefinitionGroupForInvokeList(Tail, Type, Arguments -> TailSig)
        (|
            eq(HeadSig, nil)
            where(TailSig -> Signature)
        ||
            where(HeadSig -> Signature)
        |)
        
    'rule' GenerateDefinitionGroupForInvokeList(nil, _, _ -> nil):
        -- do nothing

'action' GenerateDefinitionGroupForInvokeMethodList(STRING, INVOKEMETHODTYPE, EXPRESSIONLIST, INVOKEMETHODLIST -> INVOKESIGNATURE)
    
    'rule' GenerateDefinitionGroupForInvokeMethodList(ModuleNameString, WantType, Arguments, methodlist(SymbolNameString, IsType, Signature, Tail) -> OutSig):
        (|
            eq(WantType, IsType)
            AreAllArgumentsDefinedForInvokeMethod(Arguments, Signature)

            MakeNameLiteral(ModuleNameString -> ModuleName)
            EmitModuleDependency(ModuleName -> ModuleIndex)
            
            MakeNameLiteral(SymbolNameString -> SymbolName)
            EmitUndefinedType(-> SymbolTypeIndex)
            EmitImportedHandler(ModuleIndex, SymbolName, SymbolTypeIndex -> SymbolIndex)
            
            EmitContinueDefinitionGroup(SymbolIndex)
            
            where(Signature -> HeadSig)
        ||
            where(INVOKESIGNATURE'nil -> HeadSig)
        |)
        GenerateDefinitionGroupForInvokeMethodList(ModuleNameString, WantType, Arguments, Tail -> TailSig)
        (|
            eq(HeadSig, nil)
            where(TailSig -> OutSig)
        ||
            where(HeadSig -> OutSig)
        |)

        
     'rule' GenerateDefinitionGroupForInvokeMethodList(_, _, _, nil -> nil):
        -- do nothing

'condition' AreAllArgumentsDefinedForInvokeMethod(EXPRESSIONLIST, INVOKESIGNATURE)

    'rule' AreAllArgumentsDefinedForInvokeMethod(Arguments, invokesignature(_, Index, Tail)):
        eq(Index, -1)
        AreAllArgumentsDefinedForInvokeMethod(Arguments, Tail)
        
    'rule' AreAllArgumentsDefinedForInvokeMethod(Arguments, invokesignature(_, Index, Tail)):
        GetExpressionAtIndex(Arguments, Index -> Arg)
        ne(Arg, nil)
        AreAllArgumentsDefinedForInvokeMethod(Arguments, Tail)

    'rule' AreAllArgumentsDefinedForInvokeMethod(Arguments, nil):
        -- do nothing

'action' GetExpressionAtIndex(EXPRESSIONLIST, INT -> EXPRESSION)

    'rule' GetExpressionAtIndex(expressionlist(Head, Tail), Index -> Head):
        eq(Index, 0)
        
    'rule' GetExpressionAtIndex(expressionlist(_, Tail), Index -> Head):
        GetExpressionAtIndex(Tail, Index - 1 -> Head)

----

'action' GenerateCallInRegister(INT, INT, POS, ID, EXPRESSIONLIST)

    'rule' GenerateCallInRegister(ResultRegister, Context, Position, Handler, Arguments):
        QuerySymbolId(Handler -> Info)
        Info'Index -> Index
        Info'Kind -> Kind
        Info'Type -> handler(_, signature(HandlerSig, _))
        GenerateCall_GetInvokeSignature(HandlerSig -> InvokeSig)

        GenerateInvoke_EvaluateArguments(Context, InvokeSig, Arguments)
        EmitBeginExecuteInvoke(Index, Context, ResultRegister)
        GenerateInvoke_EmitInvokeArguments(Arguments)
        EmitEndInvoke
        GenerateInvoke_AssignArguments(Context, InvokeSig, Arguments)
        GenerateInvoke_FreeArguments(Arguments)

'action' GenerateCall_GetInvokeSignature(PARAMETERLIST -> INVOKESIGNATURE)

    'rule' GenerateCall_GetInvokeSignature(parameterlist(parameter(_, Mode, _, _), ParamRest) -> invokesignature(Mode, 0, InvokeRest)):
        GenerateCall_GetInvokeSignature(ParamRest -> InvokeRest)
    
    'rule' GenerateCall_GetInvokeSignature(nil -> nil):
        -- do nothing

----

'action' GenerateExpression(INT, EXPRESSION -> INT)

    'rule' GenerateExpression(Context, Expr -> Reg):
        EmitCreateRegister(-> Reg)
        GenerateExpressionInRegister(Reg, Context, Expr)

'action' GenerateExpressionInRegister(INT, INT, EXPRESSION)

    'rule' GenerateExpressionInRegister(Result, Context, undefined(_)):
        EmitAssignUndefined(Result)

    'rule' GenerateExpressionInRegister(Result, Context, true(_)):
        EmitAssignTrue(Result)
        
    'rule' GenerateExpressionInRegister(Result, Context, false(_)):
        EmitAssignFalse(Result)
        
    'rule' GenerateExpressionInRegister(Result, Context, integer(_, Value)):
        EmitAssignInteger(Result, Value)
        
    'rule' GenerateExpressionInRegister(Result, Context, real(_, Value)):
        EmitAssignReal(Result, Value)
        
    'rule' GenerateExpressionInRegister(Result, Context, string(_, Value)):
        EmitAssignString(Result, Value)
        
    'rule' GenerateExpressionInRegister(Result, Context, slot(_, Id)):
        QuerySymbolId(Id -> Info)
        Info'Kind -> Kind
        Info'Index -> Index
        EmitFetchVar(Kind, Result, Index)
        
    'rule' GenerateExpressionInRegister(Result, Context, as(_, _, _)):
        -- TODO
    
    'rule' GenerateExpressionInRegister(Result, Context, list(_, _)):
        -- TODO
    
    'rule' GenerateExpressionInRegister(Result, Context, call(Position, Handler, Arguments)):
        GenerateCallInRegister(Result, Context, Position, Handler, Arguments)
    
    'rule' GenerateExpressionInRegister(Result, Context, invoke(_, Invokes, Arguments)):
        GenerateDefinitionGroupForInvokes(Invokes, evaluate, Arguments -> Index, Signature)
        GenerateInvoke_EvaluateArguments(Context, Signature, Arguments)
        EmitCreateRegister(-> IgnoredReg)
        EmitBeginExecuteInvoke(Index, Context, IgnoredReg)
        GenerateInvoke_EmitInvokeArguments(Arguments)
        EmitContinueInvoke(Result)
        EmitEndInvoke()
        EmitDestroyRegister(IgnoredReg)
        GenerateInvoke_AssignArguments(Context, Signature, Arguments)
        GenerateInvoke_FreeArguments(Arguments)

----

'action' EmitStoreVar(SYMBOLKIND, INT, INT)

    'rule' EmitStoreVar(variable, Reg, Var):
        EmitStoreGlobal(Reg, Var)
        
    'rule' EmitStoreVar(_, Reg, Var):
        EmitStoreLocal(Reg, Var)

'action' EmitFetchVar(SYMBOLKIND, INT, INT)

    'rule' EmitFetchVar(variable, Reg, Var):
        EmitFetchGlobal(Reg, Var)
        
    'rule' EmitFetchVar(_, Reg, Var):
        EmitFetchLocal(Reg, Var)

'action' EmitInvokeRegisterList(INTLIST)

    'rule' EmitInvokeRegisterList(intlist(Head, Tail)):
        EmitContinueInvoke(Head)
        EmitInvokeRegisterList(Tail)
        
    'rule' EmitInvokeRegisterList(nil):
        -- nothing

'action' EmitDestroyRegisterList(INTLIST)

    'rule' EmitDestroyRegisterList(intlist(Head, Tail)):
        EmitDestroyRegister(Head)
        EmitDestroyRegisterList(Tail)
        
    'rule' EmitDestroyRegisterList(nil):
        -- nothing

'action' GenerateExpressionList(INT, EXPRESSIONLIST -> INTLIST)

    'rule' GenerateExpressionList(Context, expressionlist(Head, Tail) -> intlist(HeadReg, TailRegs)):
        GenerateExpression(Context, Head -> HeadReg)
        GenerateExpressionList(Context, Tail -> TailRegs)
        
    'rule' GenerateExpressionList(_, nil -> nil)
        -- nothing

--------------------------------------------------------------------------------

'condition' IsNamedTypeId(ID)

    'rule' IsNamedTypeId(Id):
        -- If the id is defined in another module then it is a named type.
        -- If the id is defined in this module and is public then it is a named type.
        -- Otherwise it is an alias to an 'unnamed' type.
        QuerySymbolId(Id -> Info)
        Info'Parent -> ModuleId
        QueryModuleId(ModuleId -> ModuleInfo)
        ModuleInfo'Index -> Index
        (|
            ne(Index, 0)
        ||
            Info'Access -> public
        |)


'action' GenerateType(TYPE -> INT)

    'rule' GenerateType(optional(_, Base) -> Index):
        GenerateType(Base -> BaseTypeIndex)
        EmitOptionalType(BaseTypeIndex -> Index)

    'rule' GenerateType(named(_, Id) -> Index):
        QuerySymbolId(Id -> Info)
        Info'Index -> DefinedIndex
        EmitDefinedType(DefinedIndex -> Index)
        
    'rule' GenerateType(foreign(_, Binding) -> Index):
        EmitForeignType(Binding -> Index)

    'rule' GenerateType(record(_, Base, Fields) -> Index):
        GenerateType(Base -> BaseTypeIndex)
        EmitBeginRecordType(BaseTypeIndex)
        GenerateRecordTypeFields(Fields)
        EmitEndRecordType(-> Index)

    'rule' GenerateType(handler(_, signature(Parameters, ReturnType)) -> Index):
        GenerateType(ReturnType -> ReturnTypeIndex)
        EmitBeginHandlerType(ReturnTypeIndex)
        GenerateHandlerTypeParameters(Parameters)
        EmitEndHandlerType(-> Index)

    'rule' GenerateType(opaque(_, _, _) -> Index):
        -- TODO
        EmitUndefinedType(-> Index)

    'rule' GenerateType(any(_) -> Index):
        EmitAnyType(-> Index)
    'rule' GenerateType(undefined(_) -> Index):
        EmitUndefinedType(-> Index)

    'rule' GenerateType(boolean(_) -> Index):
        EmitBooleanType(-> Index)
    'rule' GenerateType(integer(_) -> Index):
        EmitIntegerType(-> Index)
    'rule' GenerateType(real(_) -> Index):
        EmitRealType(-> Index)
    'rule' GenerateType(number(_) -> Index):
        EmitNumberType(-> Index)
    'rule' GenerateType(string(_) -> Index):
        EmitStringType(-> Index)
    'rule' GenerateType(data(_) -> Index):
        EmitDataType(-> Index)
    'rule' GenerateType(array(_) -> Index):
        EmitArrayType(-> Index)
    'rule' GenerateType(list(_, _) -> Index):
        EmitListType(-> Index)
        
    'rule' GenerateType(pointer(_) -> Index):
        EmitPointerType(-> Index)
    'rule' GenerateType(bool(_) -> Index):
        EmitBoolType(-> Index)
    'rule' GenerateType(int(_) -> Index):
        EmitIntType(-> Index)
    'rule' GenerateType(uint(_) -> Index):
        EmitUIntType(-> Index)
    'rule' GenerateType(float(_) -> Index):
        EmitFloatType(-> Index)
    'rule' GenerateType(double(_) -> Index):
        EmitDoubleType(-> Index)

    'rule' GenerateType(Type -> 0):
        print(Type)
        Fatal_InternalInconsistency("attempt to generate uncoded type")

'action' GenerateRecordTypeFields(FIELDLIST)

    'rule' GenerateRecordTypeFields(fieldlist(slot(_, Id, Type), Tail)):
        GenerateType(Type -> TypeIndex)
        Id'Name -> Name
        EmitRecordTypeField(Name, TypeIndex)
        GenerateRecordTypeFields(Tail)
        
    'rule' GenerateRecordTypeFields(nil):
        -- nothing

'action' GenerateHandlerTypeParameters(PARAMETERLIST)

    'rule' GenerateHandlerTypeParameters(parameterlist(parameter(_, Mode, Id, Type), Rest)):
        GenerateType(Type -> TypeIndex)
        Id'Name -> Name
        (|
            where(Mode -> in)
            EmitHandlerTypeInParameter(Name, TypeIndex)
        ||
            where(Mode -> out)
            EmitHandlerTypeOutParameter(Name, TypeIndex)
        ||
            where(Mode -> inout)
            EmitHandlerTypeInOutParameter(Name, TypeIndex)
        |)
        GenerateHandlerTypeParameters(Rest)
        
    'rule' GenerateHandlerTypeParameters(nil):
        -- nothing

--------------------------------------------------------------------------------

'condition' IsUngeneratedExternalId(ID)

    'rule' IsUngeneratedExternalId(Id):
        -- Ungenerated if index is -1
        QuerySymbolId(Id -> Info)
        Info'Index -> Index
        eq(Index, -1)

        -- Extenal if module index is not 0
        Info'Parent -> ModuleId
        QueryModuleId(ModuleId -> ModuleInfo)
        ModuleInfo'Index -> ModuleIndex
        ne(ModuleIndex, 0)

'condition' IsExternalId(ID)

    'rule' IsExternalId(Id):
        -- Extenal if module index is not 0
        QuerySymbolId(Id -> Info)
        Info'Parent -> ModuleId
        QueryModuleId(ModuleId -> ModuleInfo)
        ModuleInfo'Index -> ModuleIndex
        ne(ModuleIndex, 0)

'action' QueryModuleOfId(ID -> ID)

    'rule' QueryModuleOfId(Id -> ModuleId):
        QuerySymbolId(Id -> Info)
        Info'Parent -> ModuleId

'action' QuerySymbolId(ID -> SYMBOLINFO)

    'rule' QuerySymbolId(Id -> Info):
        QueryId(Id -> symbol(Info))

'condition' QueryModuleId(ID -> MODULEINFO)

    'rule' QueryModuleId(Id -> Info):
        QueryId(Id -> module(Info))
        
-- Defined in check.g
'action' QueryId(ID -> MEANING)
'condition' QuerySyntaxId(ID -> SYNTAXINFO)
'condition' QuerySyntaxMarkId(ID -> SYNTAXMARKINFO)

--------------------------------------------------------------------------------
