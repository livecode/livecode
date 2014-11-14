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
        
    'rule' GenerateImportedDefinitions(EXPRESSION'slot(_, Name)):
        GenerateImportedDefinition(Name)

    'rule' GenerateImportedDefinitions(EXPRESSION'call(_, Handler, Arguments)):
        GenerateImportedDefinition(Handler)
        GenerateImportedDefinitions(Arguments)

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
        
        -- Generate a type for the symbol
        GenerateType(SymbolType -> SymbolTypeIndex)
        
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
    
    'rule' GenerateDefinitionIndexes(_):
        -- nothing

'action' GenerateDefinitionIndex(ID)

    'rule' GenerateDefinitionIndex(Id):
        QuerySymbolId(Id -> Info)
        EmitDefinitionIndex(-> Index)
        Info'Index <- Index

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
        
    'rule' GenerateExportedDefinitions(_):
        -- Non-public, non-exportable definition fallthrough.

'action' GenerateExportedDefinition(ID)

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
        GenerateBody(Body)
        EmitReturnNothing()
        EmitEndHandlerDefinition()

    'rule' GenerateDefinitions(foreignhandler(Position, _, Id, Signature, Binding)):
        GenerateType(handler(Position, Signature) -> TypeIndex)
        
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        Info'Index -> DefIndex
        EmitForeignHandlerDefinition(DefIndex, Position, Name, TypeIndex, Binding)
        
    'rule' GenerateDefinitions(property(_, _, _)):
        -- TODO
        
    'rule' GenerateDefinitions(event(_, _, _)):
        -- TODO
        
    'rule' GenerateDefinitions(syntax(_, _, _, _, _, _)):
        -- TODO
        
    'rule' GenerateDefinitions(nil):
        -- nothing

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
        

'action' GenerateBody(STATEMENT)

    'rule' GenerateBody(sequence(Left, Right)):
        GenerateBody(Left)
        GenerateBody(Right)

    'rule' GenerateBody(variable(Position, Id, Type)):
        QuerySymbolId(Id -> Info)
        Id'Name -> Name
        GenerateType(Type -> TypeIndex)
        EmitHandlerVariable(Name, TypeIndex -> Index)
        Info'Index <- Index
    
    'rule' GenerateBody(if(Position, Condition, Consequent, Alternate)):
        EmitDeferLabel(-> AlternateLabel)
        EmitDeferLabel(-> EndIfLabel)
        GenerateExpression(Condition -> ResultRegister)
        EmitJumpIfFalse(ResultRegister, AlternateLabel)
        EmitDestroyRegister(ResultRegister)
        GenerateBody(Consequent)
        EmitJump(EndIfLabel)
        EmitResolveLabel(AlternateLabel)
        GenerateBody(Alternate)
        EmitResolveLabel(EndIfLabel)
        
    'rule' GenerateBody(repeatforever(Position, Body)):
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)
        EmitResolveLabel(RepeatHead)
        GenerateBody(Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitPopRepeatLabels()
        
    'rule' GenerateBody(repeatcounted(Position, Count, Body)):
        -- repeat n times uses a builtin invoke:
        -- bool RepeatCountedIterator(inout count)
        --   if count == 0 then return false
        --   count -= 1
        --   return true
        GenerateExpression(Count -> CountRegister)
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)

        EmitResolveLabel(RepeatHead)
        EmitCreateRegister(-> ContinueRegister)
        EmitBeginBuiltinInvoke("RepeatCounted", ContinueRegister)
        EmitContinueInvoke(CountRegister)
        EmitEndInvoke()
        EmitJumpIfFalse(CountRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)
        GenerateBody(Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitDestroyRegister(CountRegister)
        EmitPopRepeatLabels()
        
    'rule' GenerateBody(repeatwhile(Position, Condition, Body)):
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)
        
        EmitResolveLabel(RepeatHead)
        GenerateExpression(Condition -> ContinueRegister)
        EmitJumpIfFalse(ContinueRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)
        GenerateBody(Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitPopRepeatLabels()

    'rule' GenerateBody(repeatuntil(Position, Condition, Body)):
        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatHead, RepeatTail)
        
        EmitResolveLabel(RepeatHead)
        GenerateExpression(Condition -> ContinueRegister)
        EmitJumpIfTrue(ContinueRegister, RepeatTail)
        EmitDestroyRegister(ContinueRegister)
        GenerateBody(Body)
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitPopRepeatLabels()
        
    'rule' GenerateBody(repeatupto(Position, Slot, Start, Finish, Step, Body)):
        QuerySymbolId(Slot -> Info)
        Info'Index -> VarIndex
        Info'Kind -> VarKind

        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatNext)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatNext, RepeatTail)
        
        GenerateExpression(Start -> CounterRegister)
        GenerateExpression(Finish -> LimitRegister)
        (|
            where(Step -> nil)
            EmitCreateRegister(-> StepRegister)
            EmitAssignInteger(StepRegister, 1)
        ||
            GenerateExpression(Step -> StepRegister)
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

        GenerateBody(Body)

        EmitResolveLabel(RepeatNext)
        EmitFetchVar(VarKind, CounterRegister, VarIndex)
        EmitBeginBuiltinInvoke("RepeatUpToIterate", CounterRegister)
        EmitContinueInvoke(CounterRegister)
        EmitContinueInvoke(LimitRegister)
        EmitContinueInvoke(StepRegister)
        EmitEndInvoke()

        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitDestroyRegister(CounterRegister)
        EmitDestroyRegister(LimitRegister)
        EmitDestroyRegister(StepRegister)

    'rule' GenerateBody(repeatdownto(Position, Slot, Start, Finish, Step, Body)):
        QuerySymbolId(Slot -> Info)
        Info'Index -> VarIndex
        Info'Kind -> VarKind

        EmitDeferLabel(-> RepeatHead)
        EmitDeferLabel(-> RepeatNext)
        EmitDeferLabel(-> RepeatTail)
        EmitPushRepeatLabels(RepeatNext, RepeatTail)
        
        GenerateExpression(Start -> CounterRegister)
        GenerateExpression(Finish -> LimitRegister)
        (|
            where(Step -> nil)
            EmitCreateRegister(-> StepRegister)
            EmitAssignInteger(StepRegister, -1)
        ||
            GenerateExpression(Step -> StepRegister)
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

        GenerateBody(Body)

        EmitResolveLabel(RepeatNext)
        EmitFetchVar(VarKind, CounterRegister, VarIndex)
        EmitBeginBuiltinInvoke("RepeatDownToIterate", CounterRegister)
        EmitContinueInvoke(CounterRegister)
        EmitContinueInvoke(LimitRegister)
        EmitContinueInvoke(StepRegister)
        EmitEndInvoke()
        EmitJump(RepeatHead)
        EmitResolveLabel(RepeatTail)
        EmitDestroyRegister(CounterRegister)
        EmitDestroyRegister(LimitRegister)
        EmitDestroyRegister(StepRegister)
        
    'rule' GenerateBody(repeatforeach(Position, Iterator, Slot, Container, Body)):
        -- TODO
        
    'rule' GenerateBody(nextrepeat(Position)):
        EmitCurrentRepeatLabels(-> Next, _)
        EmitJump(Next)
        
    'rule' GenerateBody(exitrepeat(Position)):
        EmitCurrentRepeatLabels(-> _, Exit)
        EmitJump(Exit)
        
    'rule' GenerateBody(return(Position, Value)):
        GenerateExpression(Value -> ReturnReg)
        EmitReturn(ReturnReg)
        
    'rule' GenerateBody(call(Position, Handler, Arguments)): -- NOT COMPLETE!
        QuerySymbolId(Handler -> Info)
        Info'Index -> Index
        Info'Kind -> Kind
        EmitCreateRegister(-> ResultRegister)
        GenerateExpressionList(Arguments -> ArgumentsList)
        (|
            where(Kind -> handler)
            EmitBeginInvoke(Index, ResultRegister)
            where(-1 -> HandlerRegister)
        ||
            EmitCreateRegister(-> HandlerRegister)
            EmitFetchVar(Kind, Index, HandlerRegister)
            EmitBeginIndirectInvoke(HandlerRegister, ResultRegister)
        |)
        EmitInvokeRegisterList(ArgumentsList)
        EmitEndInvoke
        EmitDestroyRegisterList(ArgumentsList)
        [|
            ne(HandlerRegister, -1)
            EmitDestroyRegister(HandlerRegister)
        |]
        EmitDestroyRegister(ResultRegister)

    'rule' GenerateBody(invoke(_, Method, Arguments)):
        -- TODO
        
    'rule' GenerateBody(nil):
        -- nothing

----

'action' GenerateExpression(EXPRESSION -> INT)

    'rule' GenerateExpression(Expr -> Reg):
        EmitCreateRegister(-> Reg)
        GenerateExpressionInRegister(Reg, Expr)

'action' GenerateExpressionInRegister(INT, EXPRESSION)

    'rule' GenerateExpressionInRegister(Result, undefined(_)):
        EmitAssignUndefined(Result)

    'rule' GenerateExpressionInRegister(Result, true(_)):
        EmitAssignTrue(Result)
        
    'rule' GenerateExpressionInRegister(Result, false(_)):
        EmitAssignFalse(Result)
        
    'rule' GenerateExpressionInRegister(Result, integer(_, Value)):
        EmitAssignInteger(Result, Value)
        
    'rule' GenerateExpressionInRegister(Result, real(_, Value)):
        EmitAssignReal(Result, Value)
        
    'rule' GenerateExpressionInRegister(Result, string(_, Value)):
        EmitAssignString(Result, Value)
        
    'rule' GenerateExpressionInRegister(Result, slot(_, Id)):
        QuerySymbolId(Id -> Info)
        Info'Kind -> Kind
        Info'Index -> Index
        EmitFetchVar(Kind, Result, Index)
        
    'rule' GenerateExpressionInRegister(Result, as(_, _, _)):
        -- TODO
    
    'rule' GenerateExpressionInRegister(Result, list(_, _)):
        -- TODO
    
    'rule' GenerateExpressionInRegister(Result, call(_, Handler, Arguments)):
        -- TODO
    
    'rule' GenerateExpressionInRegister(Result, invoke(_, _, _)):
        -- TODO

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

'action' GenerateExpressionList(EXPRESSIONLIST -> INTLIST)

    'rule' GenerateExpressionList(expressionlist(Head, Tail) -> intlist(HeadReg, TailRegs)):
        GenerateExpression(Head -> HeadReg)
        GenerateExpressionList(Tail -> TailRegs)
        
    'rule' GenerateExpressionList(nil -> nil)
        -- nothing

--------------------------------------------------------------------------------

'action' GenerateType(TYPE -> INT)

    'rule' GenerateType(optional(_, Base) -> Index):
        GenerateType(Base -> BaseTypeIndex)
        EmitOptionalType(BaseTypeIndex -> Index)

    'rule' GenerateType(named(_, Id) -> Index):
        QuerySymbolId(Id -> Info)
        Info'Index -> OtherTypeIndex
        EmitNamedType(OtherTypeIndex -> Index)

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
    'rule' GenerateType(list(_) -> Index):
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

--------------------------------------------------------------------------------
