\"[^\n\r\"]*\" {
  MakeStringLiteral(yytext, &yylval.attr[1]);
  yysetpos();
  return STRING_LITERAL;
}
