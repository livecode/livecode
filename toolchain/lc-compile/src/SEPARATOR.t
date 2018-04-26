\\[ \t]*(\n|\r|\r\n) {
    AdvanceCurrentPosition(1);
    AdvanceCurrentPositionToNextRow();
}
[\n\r]+ {
    int i;
    GetCurrentPosition(&yylval.attr[0]);
    for(i = 0; i < yyleng; i++)
    {
        if (yytext[i] == '\n')
            AdvanceCurrentPositionToNextRow();
        else if (yytext[i] == '\r')
        {
            if (i + 1 < yyleng && yytext[i + 1] == '\n')
                i += 1;
            AdvanceCurrentPositionToNextRow();
        }
        else
            AdvanceCurrentPosition(1);
    }
    if (strchr(yytext, '\n') != NULL || strchr(yytext, '\r') != NULL)
        return SEPARATOR;
}
