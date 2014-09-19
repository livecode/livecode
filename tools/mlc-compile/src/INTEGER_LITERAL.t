[0-9]+ {
   yylval.attr[1] = atol (yytext);
   yysetpos();
   return INTEGER_LITERAL;
}

