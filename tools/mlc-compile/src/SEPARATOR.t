(\-\-[^\n\r]*|[\n\t\r ])+ {
    if (strchr(yytext, '\n') != NULL || strchr(yytext, '\r') != NULL)
        return SEPARATOR;
}
