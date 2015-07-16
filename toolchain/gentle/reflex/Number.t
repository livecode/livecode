[0-9]+ {
   yylval.attr[1] = atoi(yytext);
   yysetpos();
   return Number;
}
