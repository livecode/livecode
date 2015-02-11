#define PRIVATE static

/*--------------------------------------------------------------------*/
/* Source Positions                                                   */
/*--------------------------------------------------------------------*/

/*
 define yyLCODE 16384L
 define yyFCODE 16777216L
*/
#define yyLCODE 1000L
#define yyFCODE 1000000000L

long yypos
   = (yyFCODE+yyLCODE+1);

PRIVATE long yyLineCount = 1;
PRIVATE long yyFileCount = 1;

/*--------------------------------------------------------------------*/
void yyGetPos(ref_pos)
   long *ref_pos;
{
   *ref_pos = yypos-1;
}

/*--------------------------------------------------------------------*/

void yyPosToNextLine()
{
   yyLineCount++;
   yypos = yyFileCount*yyFCODE+yyLineCount*yyLCODE+1;
}

/*--------------------------------------------------------------------*/

void yyPosToNextFile()
{
   yyLineCount = 1;
   yyFileCount++;
   yypos = yyFileCount*yyFCODE+yyLineCount*yyLCODE+1;
}

/*--------------------------------------------------------------------*/

PRIVATE  yyFileAtPos(pos)
   long pos;
{
   long f;
   f = pos / yyFCODE;
   return f;
}

/*--------------------------------------------------------------------*/

PRIVATE  yyLineAtPos(pos)
   long pos;
{
   long l;
   l = (pos % yyFCODE) / yyLCODE;
   return l;
}

/*--------------------------------------------------------------------*/

PRIVATE yyColAtPos(pos)
   long pos;
{
   long c;
   c = pos % yyLCODE;
   return c;
}

/*--------------------------------------------------------------------*/
/* Error Messages                                                     */
/*--------------------------------------------------------------------*/

void Error(msg, pos)
   char *msg;
   long pos;
{
   printf("line %d, col %d: %s\n",
      yyLineAtPos(pos), yyColAtPos(pos), msg);
   exit(1);
}

/*--------------------------------------------------------------------*/

yyerror(msg)
   char *msg;
{
   long pos;

   yyGetPos(& pos);
   Error(msg, pos);
}

/*--------------------------------------------------------------------*/
/*
yylexerror (msg)
   char *msg;
{
   long pos;

   yyGetPos(& pos);
   Error(msg, pos);
}
*/

/*--------------------------------------------------------------------*/
