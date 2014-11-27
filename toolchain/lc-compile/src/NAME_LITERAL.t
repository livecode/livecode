[A-Za-z_][A-Za-z0-9_\.]* { 
  MakeNameLiteral(yytext, &yylval.attr[1]);
  yysetpos();
   return NAME_LITERAL;
}
