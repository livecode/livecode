/*****************************************************************************/
/*                                                                           */
/*  A compiler for PL/0                                                      */
/*                                                                           */
/*  Source language and target machine of this compiler are described in     */
/*                                                                           */
/*  Niklaus Wirth                                                            */
/*  Compilerbau                                                              */
/*  B.G. Teubner, Stuttgart 1986                                             */
/*                                                                           */
/*****************************************************************************/

-- Abstract syntax

'type' Block
   block(ConstDecls, VarDecls, ProcDecls, Statement)

'type' ConstDecls
   list(ConstDecl, ConstDecls), nil

'type' ConstDecl
   const(IDENT, INT, POS)

'type' VarDecls
   list(VarDecl, VarDecls), nil

'type' VarDecl
   var(IDENT, POS)

'type' ProcDecls
   list(ProcDecl, ProcDecls), nil

'type' ProcDecl
   proc(IDENT, Block, POS)

'type' Statement
   assign(IDENT, Expression, POS)
   call(IDENT, POS)
   seq(Statement, Statement)
   if(Condition, Statement)
   while(Condition, Statement)
   null
   put(Expression)

'type' Condition
   odd(Expression)
   rel(Op, Expression, Expression)

'type' Expression
   neg(Expression)
   binary(Op, Expression, Expression)
   id(IDENT, POS)
   int(INT)

'type' Op
   plus, minus, times, div, eq, ne, lt, le, gt, ge

'type' IDENT

-- Concrete syntax

'root'
   program(-> B)
   trafo(B)

'nonterm' program(-> Block)
'rule' program(-> B):
   block(-> B) "."

'nonterm' block(-> Block)
'rule' block(-> block(C, V, P, S)):
   constpart(-> C) varpart(-> V) procpart(-> P) statement(-> S)

'nonterm' constpart(-> ConstDecls)
'rule' constpart(-> C): "const" constdeflist(-> C) ";"
'rule' constpart(-> nil):

'nonterm' constdeflist(-> ConstDecls)
'rule' constdeflist(-> list(D, L)): constdef(-> D) "," constdeflist(-> L)
'rule' constdeflist(-> list(D, nil)): constdef(-> D)

'nonterm'constdef(-> ConstDecl)
'rule' constdef(-> const(I, N, Pos)): Ident(-> I) @(-> Pos) "=" Number(-> N)

'nonterm' varpart(-> VarDecls)
'rule' varpart(-> V): "var" vardecllist(-> V) ";"
'rule' varpart(-> nil):

'nonterm' vardecllist(-> VarDecls)
'rule' vardecllist(-> list(D, L)): vardecl(-> D) "," vardecllist(-> L)
'rule' vardecllist(-> list(D, nil)): vardecl(-> D)

'nonterm' vardecl(-> VarDecl)
'rule' vardecl(-> var(I, Pos)): Ident(-> I) @(-> Pos)

'nonterm' procpart(-> ProcDecls)
'rule' procpart(-> list(D, L)): procdef(-> D) procpart(-> L)
'rule' procpart(-> nil):

'nonterm' procdef(-> ProcDecl)
'rule' procdef(-> proc(I, B, Pos)):
   "procedure" Ident(-> I) @(-> Pos) ";" block(-> B) ";"

'nonterm' statement(-> Statement)
'rule' statement(-> assign(I, E, P)): Ident(-> I) @(-> P) ":=" expression(-> E)
'rule' statement(-> call(I, P)): "call" Ident(-> I) @(-> P)
'rule' statement(-> S): "begin" statementlist(-> S) "end"
'rule' statement(-> if(C, S)): "if" condition(-> C) "then" statement(-> S)
'rule' statement(-> while(C, S)): "while" condition(-> C) "do" statement(-> S)
'rule' statement(-> put(E)): "put" expression(-> E)
'rule' statement(-> null):

'nonterm' statementlist(-> Statement)
'rule' statementlist(-> seq(S, L)): statement(-> S) ";" statementlist(-> L)
'rule' statementlist(-> S): statement(-> S)

'nonterm' condition(-> Condition)
'rule' condition(-> odd(E)): "odd" expression(-> E)
'rule' condition(-> rel(Op, E1, E2)):
   expression(-> E1) relop(-> Op) expression(-> E2)

'nonterm' relop(-> Op)
'rule' relop(-> eq): "="
'rule' relop(-> ne): "#"
'rule' relop(-> lt): "<"
'rule' relop(-> le): "<="
'rule' relop(-> gt): ">"
'rule' relop(-> ge): ">="

'nonterm' expression(-> Expression)
'rule' expression(-> E): "+" term(-> E)
'rule' expression(-> neg(E)): "-" term(-> E)
'rule' expression(-> E): expr2(-> E)

'nonterm' expr2(-> Expression)
'rule' expr2(-> binary(Op, E1, E2)): expr2(-> E1) addop(-> Op) term(-> E2)
'rule' expr2(-> E): term(-> E)

'nonterm' addop(-> Op)
'rule' addop(-> plus): "+"
'rule' addop(-> minus): "-"

'nonterm' term(-> Expression)
'rule' term(-> binary(Op, E1, E2)): term(-> E1) mulop(-> Op) factor(-> E2)
'rule' term(-> E): factor(-> E)

'nonterm' mulop(-> Op)
'rule' mulop(-> times): "*"
'rule' mulop(-> div): "/"

'nonterm' factor(-> Expression)
'rule' factor(-> id(I, P)): Ident(-> I) @(-> P)
'rule' factor(-> int(N)): Number(-> N)
'rule' factor(-> E): "(" expression(-> E) ")"

'token' Ident(-> IDENT)
'token' Number(-> INT)
-- Trafo
'action' trafo(Block)
'rule' trafo(B)
   InitEnv
   trBlock(B)
   execute

'var' Offset: INT

'action' trBlock(Block)
'rule' trBlock(block(ConstDecls, VarDecls, ProcDecls, Statement)):
   EnterScope
   trConstDecls(ConstDecls)
   Offset <- 3
   trVarDecls(VarDecls)
   Offset -> N
   PC(-> X)
   JMP(0)
   trProcDecls(ProcDecls)
   PATCH(X)
   INT_(N)
   trStatement(Statement)
   OPR(0)
   LeaveScope

'action' trConstDecls(ConstDecls)
'rule' trConstDecls(list(ConstDecl, ConstDecls)):
   trConstDecl(ConstDecl)
   trConstDecls(ConstDecls)
'rule' trConstDecls(nil):

'action' trConstDecl(ConstDecl)
'rule' trConstDecl(const(Id, N, Pos)):
   Define(Id, const(N), Pos)

'action' trVarDecls(VarDecls)
'rule' trVarDecls(list(VarDecl, VarDecls)):
   trVarDecl(VarDecl)
   trVarDecls(VarDecls)
'rule' trVarDecls(nil):

'action' trVarDecl(VarDecl)
'rule' trVarDecl(var(Id, Pos)):
   CurrentLevel -> L
   Offset -> N
   Offset <- N+1
   Define(Id, var(L, N), Pos)

'action' trProcDecls(ProcDecls)
'rule' trProcDecls(list(ProcDecl, ProcDecls)):
   trProcDecl(ProcDecl)
   trProcDecls(ProcDecls)
'rule' trProcDecls(nil):

'action' trProcDecl(ProcDecl)
'rule' trProcDecl(proc(Id, Block, Pos)):
   CurrentLevel -> Lev
   PC(-> X)
   Define(Id, proc(Lev, X), Pos)
   trBlock(Block)

'action' trStatement(Statement)
'rule' trStatement(assign(Id, Expression, Pos)):
   trExpression(Expression)
   (| Apply(Id -> var(L, A))
      CurrentLevel -> CurLev
      STO(CurLev-L,A)
   || ErrorI("[1] '", Id, "' not defined as variable", Pos)
   |)
'rule' trStatement(call(Id, Pos)):
   (| Apply(Id -> proc(L, A))
      CurrentLevel -> CurLev
      CAL(CurLev-L,A)
   || ErrorI("[2] '", Id, "' not defined as procedure", Pos)
   |)
'rule' trStatement(seq(Statement1, Statement2)):
   trStatement(Statement1)
   trStatement(Statement2)
'rule' trStatement(if(Condition, Statement)):
   trCondition(Condition)
   PC(-> X)
   JPC(0)
   trStatement(Statement)
   PATCH(X)
'rule' trStatement(while(Condition, Statement)):
   PC(-> X1)
   trCondition(Condition)
   PC(-> X2)
   JPC(0)
   trStatement(Statement)
   JMP(X1)
   PATCH(X2)
'rule' trStatement(null):
'rule' trStatement(put(Expression)):
   trExpression(Expression)
   OPR(14)

'action' trCondition(Condition)
'rule' trCondition(odd(Expression)):
   trExpression(Expression)
   OPR(6)
'rule' trCondition(rel(Op, Expression1, Expression2)):
   trExpression(Expression1)
   trExpression(Expression2)
   OpCode(Op -> N)
   OPR(N)

'action' trExpression(Expression)
'rule' trExpression(neg(Expression)):
   trExpression(Expression)
   OPR(1)
'rule' trExpression(binary(Op, Expression1, Expression2)):
   trExpression(Expression1)
   trExpression(Expression2)
   OpCode(Op -> N)
   OPR(N)
'rule' trExpression(id(Id, Pos)):
   (| Apply(Id -> const(N))
      LIT(N)
   || Apply(Id -> var(L, A))
      CurrentLevel -> CurLev
      LOD(CurLev-L,A)
   || ErrorI("[3] '", Id, "' not defined as constant or variable", Pos)
   |)
'rule' trExpression(int(N)):
   LIT(N) 

'action' OpCode(Op -> INT)
'rule' OpCode(plus -> 2)
'rule' OpCode(minus -> 3)
'rule' OpCode(times -> 4)
'rule' OpCode(div -> 5)
'rule' OpCode(eq -> 8)
'rule' OpCode(ne -> 9)
'rule' OpCode(lt -> 10)
'rule' OpCode(ge -> 11)
'rule' OpCode(gt -> 12)
'rule' OpCode(le -> 13)

-- Error messages

'action' ErrorI (STRING, IDENT, STRING, POS)
-- Instructions

'action' LIT(INT)
'action' OPR(INT)
'action' LOD(L: INT, A: INT)
'action' STO(L: INT, A: INT)
'action' CAL(L: INT, A: INT)
'action' INT_(INT)
'action' JMP(INT)
'action' JPC(INT)

'action' PC(-> INT)
'action' PATCH(INT)

'action' execute

-- Scopes

'type' Object
   const(INT)
   var(Level: INT, Addr: INT)
   proc(Level: INT, Addr: INT)

'action' DefMeaning (IDENT, ASSOC)
'condition' HasMeaning (IDENT -> ASSOC)

'type' ENV
   env (NEST, ENV), emptyenv 

'type' NEST
   nest (IDENT, NEST), emptynest

'type' ASSOC
   assoc(Obj: Object, Level: INT, Hidden: ASSOC), noassoc

'var' CurrentEnv: ENV
'var' CurrentNest: NEST
'var' CurrentLevel: INT

'action' InitEnv
'rule' InitEnv :
   CurrentEnv <- emptyenv
   CurrentNest <- emptynest
   CurrentLevel <- 0

'action' EnterScope
'rule' EnterScope :
   CurrentEnv -> OldEnv
   CurrentNest -> OldNest
   CurrentEnv <- env(OldNest, OldEnv)
   CurrentNest <- emptynest
   CurrentLevel -> N
   CurrentLevel <- N+1

'action' LeaveScope
'rule' LeaveScope :
   CurrentEnv -> env (OldNest, OldEnv)
   CurrentNest -> Nest
   ForgetNest (Nest)
   CurrentEnv <- OldEnv
   CurrentNest <- OldNest
   CurrentLevel -> N
   CurrentLevel <- N-1

'action' Define (IDENT, Object, POS)
'rule' Define (Ident, Object, Pos) :
   CurrentLevel -> ThisLev
   (| HasMeaning(Ident -> Hidden)
      where(Hidden -> assoc(_,L,_))
      [| eq(L, ThisLev)
	 ErrorI("[4] multiple declaration of '", Ident, "'", Pos)
      |]
   || let(noassoc -> Hidden)
   |)
   DefMeaning (Ident, assoc(Object, ThisLev, Hidden))
   CurrentNest -> Nest
   CurrentNest <- nest(Ident, Nest)

'condition' Apply (IDENT -> Object)
'rule' Apply (Ident -> Object) :
   HasMeaning (Ident -> assoc(Object,Lev,Hidden))

'action' ForgetNest (NEST)
'rule' ForgetNest (nest(Id, Nest)) :
   HasMeaning (Id -> assoc(Obj, Lev, Hidden))
   DefMeaning (Id, Hidden)
   ForgetNest (Nest)
'rule' ForgetNest (emptynest) :
