[0-9]+ {
  MakeIntegerLiteral(yytext, &yylval.attr[1]);
  yysetpos();
   return INTEGER_LITERAL;
}

