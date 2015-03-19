0|([1-9][0-9]*) {
    yysetpos();
    if (MakeIntegerLiteral(yytext, &yylval.attr[1]) == 0)
    {
        long t_position;
        GetCurrentPosition(&t_position);
        Error_IntegerLiteralOutOfRange(t_position);
        yylval.attr[1] = 0;
    }
    return INTEGER_LITERAL;
}

