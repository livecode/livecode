	/* This contains both INTEGER_LITERAL and DOUBLE_LITERAL rules */
	/* This ensures that they appear in this order, to ensure they match */
	/* correctly. */

[0-9]+"."[0-9]*([eE][-+]?[0-9]*)?([0-9a-zA-Z_\.]*) |
[0-9]*"."[0-9]+([eE][-+]?[0-9]*)?([0-9a-zA-Z_\.]*) |
[0-9]*"."[0-9]*([eE][-+]?[0-9]*)([0-9a-zA-Z_\.]*) |
[0-9]+([eE][-+]?[0-9]*)([0-9a-zA-Z_\.]*) {
	yysetpos();
	if (IsDoubleLiteral(yytext))
	{
		if (MakeDoubleLiteral(yytext, &yylval.attr[1]) == 0)
		{
			long t_position;
			GetCurrentPosition(&t_position);
			Error_DoubleLiteralOutOfRange(t_position);
			yylval.attr[1] = 0;
		}
	}
	else
	{
		long t_position;
        GetCurrentPosition(&t_position);
		Error_InvalidDoubleLiteral(t_position);
		yylval.attr[1] = 0;
	}
	return DOUBLE_LITERAL;
}

([0-9]+|0b[01]*|0x[0-9A-Fa-f]*)[0-9a-zA-Z_]* {
    yysetpos();
	if (IsIntegerLiteral(yytext))
	{
		if (MakeIntegerLiteral(yytext, &yylval.attr[1]) == 0)
		{
			long t_position;
			GetCurrentPosition(&t_position);
			Error_IntegerLiteralOutOfRange(t_position);
			yylval.attr[1] = 0;
		}
	}
	else
	{
		long t_position;
        GetCurrentPosition(&t_position);
		Error_InvalidIntegerLiteral(t_position);
		yylval.attr[1] = 0;
	}
    return INTEGER_LITERAL;
}

