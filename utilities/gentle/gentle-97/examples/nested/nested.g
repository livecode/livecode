-- a language with constant definitions, write statements, and nested scopes
-- (identifiers may be used before being declared)

'root'
   seq(-> S)

   CurrentScope <- 0
   process_declarations(S)
   process_statements(S)

-- Abstract Syntax ------------------------------------------------------------

'type' SEQ
   list(ITEM, SEQ)
   nil

'type' ITEM
   declare(IDENT, INT)
   write(IDENT)
   block(SEQ)

'type' IDENT

-- Concrete Grammar -----------------------------------------------------------

'nonterm' seq(-> SEQ)
   'rule' seq(-> list(H, T)): item(-> H) seq(-> T)
   'rule' seq(-> nil):

'nonterm' item(-> ITEM)
   'rule' item(-> declare(I, N)): "declare" Ident(-> I) "=" Number(-> N)
   'rule' item(-> write(I)): "write" Ident(-> I)
   'rule' item(-> block(S)): "begin" seq(-> S) "end"

'token' Ident(-> IDENT)
'token' Number(-> INT)

-- Processing Declarations and Statements -------------------------------------

'action' process_declarations(SEQ)
   -- process the declaration inside (without declarations of inner blocks)
   -- make defined identifiers visible

   'rule' process_declarations(list(H, T)):
      process_declaration(H) process_declarations(T)
   'rule' process_declarations(nil)

'action' process_declaration(ITEM)
   'rule' process_declaration(declare(I, N)):
      Define(I, N)
   'rule' process_declaration(write(I))
   'rule' process_declaration(block(S))

'action' process_statements(SEQ)
   -- process the statements inside SEQ, visit inner blocks
   -- execute write statements

   'rule' process_statements(list(H, T)):
      process_statement(H) process_statements(T)
   'rule' process_statements(nil)

'action' process_statement(ITEM)
   'rule' process_statement(write(I)):
      Apply(I -> N)
      print(N)
   'rule' process_statement(declare(I,N))
   'rule' process_statement(block(S)):
      CurrentScope -> N
      CurrentScope <- N+1
      process_declarations(S)
      process_statements(S)
      forget_declarations(S)
      CurrentScope <- N

'action' forget_declarations(SEQ)
   -- reset the identifiers defined in SEQ to their privious values

   'rule' forget_declarations(list(H, T))
      forget_declaration(H) forget_declarations(T)
   'rule' forget_declarations(nil)

'action' forget_declaration(ITEM)
   'rule' forget_declaration(declare(I, N))
      Lookup(I -> assoc(X, L, Hidden))
      DefMeaning(I, Hidden)
   'rule' forget_declaration(Other)

'var' CurrentScope: INT

-- Symbol Table ---------------------------------------------------------------

'type' ASSOC
   assoc(Meaning: INT, DeclarationScope: INT, HiddenAssoc: ASSOC)
   none

'action' DefMeaning (IDENT, ASSOC)
   -- associate ASSOC with IDENT

'condition' HasMeaning (IDENT -> ASSOC)
   -- return ASSOC associated with IDENT if there was a previous DefMeaning

'action' Define (IDENT, INT)
   -- define IDENT as INT, thereby hiding a possible global definition 
   -- emit a message if the IDENT is already defined in the actual scope

   'rule' Define (I, N):
      CurrentScope -> ThisScope
      Lookup(I -> Assoc)
      [|
         where(Assoc -> assoc(X, ThatScope, Hidden))
         eq(ThisScope, ThatScope)
	 print("identifier already declared")
      |]
      DefMeaning(I, assoc(N, ThisScope, Assoc))

'action' Apply(IDENT -> INT)
   -- return INT which has been defined as the value of IDENT
   -- emit a message if IDENT is not defined

   'rule' Apply(I -> N):
      (| Lookup(I -> assoc(N, Scope, Hidden))
      || print("undeclared identifier") where(0 -> N)
      |)

'action' Lookup(IDENT -> ASSOC)
   -- return the ASSOC currently associated with IDENT
   -- return none if there was no previous DefMeaning

   'rule' Lookup(I -> A):
      (| HasMeaning(I -> A) || where(none -> A) |)
