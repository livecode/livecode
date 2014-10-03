\"[^\n\"]*\" {
  yypos++; /* initial '"' */
  
  return STRING_LITERAL;
}

