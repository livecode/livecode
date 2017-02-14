"--"[^\n\r]*        { yysetpos(); }
"//"[^\n\r]*        { yysetpos(); }
"/*"                { yysetpos(); BEGIN(COMMENT); }
<COMMENT>"*/"       { yysetpos(); BEGIN(0); }
<COMMENT>[^*\n\r]+  { yysetpos(); }
<COMMENT>\n|\r\n|\r { AdvanceCurrentPositionToNextRow(); return SEPARATOR; }
<COMMENT>"*"        { yysetpos(); }
