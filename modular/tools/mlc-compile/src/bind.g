'module' bind

'use'
    types
    support
    
'export'
    InitializeBind
    Bind

--------------------------------------------------------------------------------

'action' Bind(MODULE)

    'rule' Bind(module(Position, Name, Imports, Definitions)):
        -- Step 1: Ensure all id's referencing definitions point to the definition.
        --         and no duplicate definitions have been attempted.
        EnterScope
        -- Assign the defining id to all top-level names.
        Declare(Definitions)
        -- Resolve all references to id's.
        Apply(Definitions)
        LeaveScope
        
        -- Step 2: Ensure all definitions have their appropriate meaning

--------------------------------------------------------------------------------

-- The 'Declare' phase associates the meanings at the current scope with the
-- identifier which defines them.
--
-- It catches multiple definition errors and shadow warnings.
--
'action' Declare(DEFINITION)

    'rule' Declare(sequence(Left, Right)):
        Declare(Left)
        Declare(Right)
        
    'rule' Declare(type(Position, _, Name, _)):
        DeclareId(Name)
    
    'rule' Declare(constant(Position, _, Name, _)):
        DeclareId(Name)
    
    'rule' Declare(variable(Position, _, Name, _)):
        DeclareId(Name)
    
    'rule' Declare(handler(Position, _, Name, _, _, _)):
        DeclareId(Name)
    
    'rule' Declare(foreignhandler(Position, _, Name, _, _)):
        DeclareId(Name)
    
    'rule' Declare(property(Position, _, Name)):
        DeclareId(Name)
    
    'rule' Declare(syntax(Position, _, Name, _, _, _)):
        DeclareId(Name)
    
    'rule' Declare(nil):
        -- do nothing

'action' DeclareParameters(PARAMETERLIST)

    'rule' DeclareParameters(parameterlist(parameter(_, _, Name, _), Tail)):
        DeclareId(Name)
        DeclareParameters(Tail)
        
    'rule' DeclareParameters(nil):
        -- do nothing

--------------------------------------------------------------------------------

'var' LastSyntaxMarkIndexVar : INT

'sweep' Apply(ANY)

    'rule' Apply(DEFINITION'handler(_, _, _, signature(Parameters, Type), _, Body)):
        -- The type of the handler is resolved in the current scope
        Apply(Type)
        
        -- Now enter a new scope for parameters and local variables.
        EnterScope
        
        -- Declare the parameters first
        DeclareParameters(Parameters)
        
        -- Apply all ids in the parameters (covers types)
        Apply(Parameters)

        -- Now apply all ids in the body. This will also declare local variables as
        -- it goes along
        Apply(Body)

        LeaveScope
        
    'rule' Apply(DEFINITION'foreignhandler(_, _, _, signature(Parameters, Type), _)):
        -- The type of the foreign handler is resolved in the current scope.
        Apply(Type)
        
        -- Enter a new scope to check parameters.
        EnterScope
        DeclareParameters(Parameters)
        Apply(Parameters)
        LeaveScope

    'rule' Apply(TYPE'named(_, Name)):
        ApplyId(Name)
        
    'rule' Apply(STATEMENT'call(_, Handler, Arguments)):
        ApplyId(Handler)
        Apply(Arguments)

    ---------

    'rule' Apply(DEFINITION'syntax(_, _, _, _, Syntax, Methods)):
        -- We index all mark variabless starting at 0.
        LastSyntaxMarkIndexVar <- 0

        -- Mark variables have their own local scope
        EnterScope
        
        -- Process the mark variables in the syntax (definiton side).
        -- all variables with the same name get the same index and we
        -- check no reserved mark variables are specified ('input' and
        -- 'output').
        Apply(Syntax)

        -- Define appropriate meaning for 'input' and 'output' before we
        -- bind the use of the ids in the methods.
        DeclarePredefinedSyntaxMarks
        Apply(Methods)

        LeaveScope
        
        -- Now we've bound all mark variable ids to an index, we can check to
        -- see if there are any cases where they are defined twice.
        PushEmptySet()
        CheckSyntaxMarkDefinitions(Syntax)

    'rule' Apply(SYNTAX'markedrule(_, Variable, _)):
        ApplySyntaxMarkId(Variable)

    'rule' Apply(SYNTAX'mark(_, Variable, _)):
        ApplySyntaxMarkId(Variable)

    'rule' Apply(SYNTAXMETHOD'method(_, Name, Arguments)):
        ApplyId(Name)
        Apply(Arguments)
        
    'rule' Apply(SYNTAXCONSTANT'name(_, Value)):
        ApplyLocalId(Value)

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
        CheckNoSyntaxMarkDefinitionsInDelimiter(Delimiter)
        
    'rule' CheckSyntaxMarkDefinitions(optional(_, Operand)):
        CheckSyntaxMarkDefinitions(Operand)

    'rule' CheckSyntaxMarkDefinitions(markedrule(_, Variable, _)):
        CheckSyntaxMarkVariableNotDefined(Variable)

    'rule' CheckSyntaxMarkDefinitions(mark(_, Variable, _)):
        CheckSyntaxMarkVariableNotDefined(Variable)

    'rule' CheckSyntaxMarkDefinitions(_):
        -- do nothing

'action' CheckSyntaxMarkVariableNotDefined(ID)

    'rule' CheckSyntaxMarkVariableNotDefined(Variable):
        Variable'Meaning -> syntaxmark(Index)
        (|
            IsIndexInSet(Index)
            Variable'Position -> Position
            Variable'Name -> Name
            Error_SyntaxMarkVariableAlreadyDefined(Position, Name)
        ||
            IncludeIndexInSet(Index)
        |)

'sweep' CheckNoSyntaxMarkDefinitionsInDelimiter(ANY)

    'rule' CheckNoSyntaxMarkDefinitionsInDelimiter(markedrule(_, Variable, _)):
        Variable'Position -> Position
        Error_SyntaxMarkVariableNotAllowedInDelimiter(Position)

    'rule' CheckNoSyntaxMarkDefinitionsInDelimiter(mark(_, Variable, _)):
        Variable'Position -> Position
        Error_SyntaxMarkVariableNotAllowedInDelimiter(Position)

--------------------------------------------------------------------------------

'action' DeclareId(ID)

    'rule' DeclareId(Id):
        Id'Name -> Name
        HasLocalMeaning(Name -> definingid(DefiningId))
        Id'Position -> Position
        DefiningId'Position -> DefiningPosition
        Error_IdentifierPreviouslyDeclared(Position, Name, DefiningPosition)
        
    'rule' DeclareId(Id):
        Id'Name -> Name
        DefineMeaning(Name, definingid(Id))

'action' ApplyId(ID)

    'rule' ApplyId(Id):
        Id'Name -> Name
        HasMeaning(Name -> Meaning)
        Id'Meaning <- Meaning
        
    'rule' ApplyId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_IdentifierNotDeclared(Position, Name)

'action' ApplyLocalId(ID)

    'rule' ApplyLocalId(Id):
        Id'Name -> Name
        HasLocalMeaning(Name -> Meaning)
        Id'Meaning <- Meaning

    'rule' ApplyLocalId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_IdentifierNotDeclared(Position, Name)

'action' ApplySyntaxMarkId(ID)

    'rule' ApplySyntaxMarkId(Id):
        Id'Name -> Name
        IsValidNameForSyntaxMark(Name)
        (|
            HasLocalMeaning(Name -> Meaning)
        ||
            LastSyntaxMarkIndexVar -> Index
            where(syntaxmark(Index) -> Meaning)
            DefineMeaning(Name, Meaning)
            LastSyntaxMarkIndexVar <- Index + 1
        |)
        Id'Meaning <- Meaning
        
    'rule' ApplySyntaxMarkId(Id):
        Id'Position -> Position
        Id'Name -> Name
        Error_InvalidNameForSyntaxMarkVariable(Position, Name)

--------------------------------------------------------------------------------

'var' OutputNameLiteralVar : NAME
'var' InputNameLiteralVar : NAME
'var' OutputSyntaxMarkMeaningVar : MEANING
'var' InputSyntaxMarkMeaningVar : MEANING

'action' InitializeBind

    'rule' InitializeBind:
        MakeNameLiteral("output" -> Name)
        OutputNameLiteralVar <- Name
        OutputSyntaxMarkMeaningVar <- syntaxoutputmark

        MakeNameLiteral("input" -> Name2)
        InputNameLiteralVar <- Name2
        InputSyntaxMarkMeaningVar <- syntaxinputmark

'action' DeclarePredefinedSyntaxMarks

    'rule' DeclarePredefinedSyntaxMarks:
        OutputNameLiteralVar -> Name1
        OutputSyntaxMarkMeaningVar -> Meaning1
        DefineMeaning(Name1, Meaning1)
        InputNameLiteralVar -> Name2
        InputSyntaxMarkMeaningVar -> Meaning2
        DefineMeaning(Name2, Meaning2)

'condition' IsValidNameForSyntaxMark(NAME)

    'rule' IsValidNameForSyntaxMark(Name):
        OutputNameLiteralVar -> OtherName
        ne(Name, OtherName)
        InputNameLiteralVar -> OtherName2
        ne(Name, OtherName2)

--------------------------------------------------------------------------------
