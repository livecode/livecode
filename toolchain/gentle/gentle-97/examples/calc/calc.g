-- A simple desk calculator

'root' expression(-> X) print(X)

'nonterm' expression(-> INT)

   'rule' expression(-> X): expr2(-> X)
   'rule' expression(-> X+Y): expression(-> X) "+" expr2(-> Y)
   'rule' expression(-> X-Y): expression(-> X) "-" expr2(-> Y)

'nonterm' expr2(-> INT)

   'rule' expr2(-> X): expr3(-> X)
   'rule' expr2(-> X*Y): expr2(-> X) "*" expr3(-> Y)
   'rule' expr2(-> X/Y): expr2(-> X) "/" expr3(-> Y)

'nonterm' expr3(-> INT)

   'rule' expr3(-> X): Number(-> X)
   'rule' expr3(-> - X): "-" expr3(-> X)
   'rule' expr3(-> + X): "+" expr3(-> X)
   'rule' expr3(-> X): "(" expression(-> X) ")"

'token' Number(-> INT)
