#include <position.h>
#define yysetpos() { GetCurrentPosition(&yylval.attr[0]); AdvanceCurrentPosition(yyleng); }