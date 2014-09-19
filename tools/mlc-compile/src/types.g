'module' types

'use'

'export'
    MODULE
    IMPORT
    DEFINITION
    ID IDLIST
    NAME DOUBLE

--------------------------------------------------------------------------------

'type' MODULE
    module(Position: POS, Name: ID, Imports: IMPORT, Definitions: DEFINITION)

'type' IMPORT
    sequence(Left: IMPORT, Right: IMPORT)
    import(Position: POS, Name: ID)
    nil
    
'type' DEFINITION
    sequence(Left: DEFINITION, Right: DEFINITION)
    nil

'type' IDLIST
    idlist(Head: ID, Tail: IDLIST)
    nil

'table' ID(Position: POS, Name: NAME)

'type' NAME
'type' DOUBLE

--------------------------------------------------------------------------------
