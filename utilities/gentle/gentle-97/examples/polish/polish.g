-- Read an expression in infix notation
-- and emit the corresponding expression in reverse polish notation

'root'
   expression(-> X) code(X)


-- Term Type Definition for Abstract Syntax
-- (Each line is a template for an alternative)

'type' Expr
   plus(Expr, Expr)
   minus(Expr, Expr)
   mult(Expr, Expr)
   div(Expr, Expr)
   neg(Expr)
   num(INT)


-- Concrete Syntax and Mapping to Abstract Syntax Terms
-- (Rule Selection by Parsing)

'nonterm' expression(-> Expr)

   'rule' expression(-> X): expr2(-> X)
   'rule' expression(-> plus(X,Y)): expression(-> X) "+" expr2(-> Y)
   'rule' expression(-> minus(X,Y)): expression(-> X) "-" expr2(-> Y)

'nonterm' expr2(-> Expr)

   'rule' expr2(-> X): expr3(-> X)
   'rule' expr2(-> mult(X,Y)): expr2(-> X) "*" expr3(-> Y)
   'rule' expr2(-> div(X,Y)): expr2(-> X) "/" expr3(-> Y)

'nonterm' expr3(-> Expr)

   'rule' expr3(-> num(X)): Number(-> X)
   'rule' expr3(-> neg(X)): "-" expr3(-> X)
   'rule' expr3(-> X): "+" expr3(-> X)
   'rule' expr3(-> X): "(" expression(-> X) ")"

'token' Number(-> INT)


-- Processing Abstract Syntax Terms
-- (Rule Selection by Pattern Matching on Arguments)

'action' code (Expr)

   'rule' code(plus(X1, X2)): code(X1) code(X2) print("plus")
   'rule' code(minus(X1, X2)): code(X1) code(X2) print("minus")
   'rule' code(mult(X1, X2)): code(X1) code(X2) print("mult")
   'rule' code(div(X1, X2)): code(X1) code(X2) print("div")
   'rule' code(neg(X)): code(X) print("neg")
   'rule' code(num(N)): print(N)
