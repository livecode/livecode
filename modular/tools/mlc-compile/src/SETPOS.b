long yypos;
#define yysetpos() { yylval.attr[0] = yypos; yypos += yyleng; }
void yyGetPos(long *pos) {*pos = yypos - 1;}