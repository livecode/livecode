[0-9]+"."[0-9]+([eE][-+]?[0-9]+)? {
  double *p;
  
  p = (double *) malloc (sizeof (double));
  *p = atof (yytext);
  yylval.attr[1] = (long) p;
  yysetpos();
  return DOUBLE_LITERAL;
}

