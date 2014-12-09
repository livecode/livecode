#ifdef _WIN32
#  include "gen.h"
#else
#  ifdef BOOTSTRAP
#    include "gen-bootstrap.h"
#  else
#    include "gen-full.h"
#  endif
#endif
extern YYSTYPE yylval;
