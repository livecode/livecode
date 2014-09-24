[A-Za-z_][A-Za-z0-9_\.]* { 
   //NAME id;
   //string_to_name (yytext, &id);
   //yylval.attr[1] = (long) id;
   yysetpos();
   return NAME_LITERAL;
}
