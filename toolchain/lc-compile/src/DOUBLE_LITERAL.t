[0-9]+"."[0-9]+([eE][-+]?[0-9]+)? {
  MakeDoubleLiteral(yytext, &yylval.attr[1]);
  yysetpos();
  return DOUBLE_LITERAL;
}

