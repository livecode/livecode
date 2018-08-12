/* --PATCH-- */ #include <stdlib.h>
/* --PATCH-- */ #include <stdio.h>
/* --PATCH-- */ #include <stdint.h>
/*
   GENTLE 97 CAMPUS EDITION

   COPYRIGHT (C) 1992, 1997. All rights reserved.

   Metarga GmbH, Joachim-Friedrich-Str. 54, D-10711 Berlin

   gentle-97-v-4-1-0
*/


/* Gentle Runtime System */

char *THIS_RUNTIME_SYSTEM =
"Gentle 3.0 01100401 (C) 1992, 1997";

typedef intptr_t * yyt;

yyt yyh;
yyt yyhx;

#define HEAPPIECE 20000

yyExtend()
{
   yyh = (yyt) malloc(HEAPPIECE * sizeof(intptr_t));
   yyhx = yyh + HEAPPIECE - 100;
   if (yyh == 0) {
      printf ("HEAP STORAGE FULL\n");
      exit(1);
   }
}

#define yyCntlMax 500

static yyt CURBLOCK;
static yyt CURPOS;
static yyt FIRSTBLOCK;

static yyt FREELIST = 0;

static yyt NEWBLOCK()
{
   yyt p;

   if (FREELIST == 0) {
      p = (yyt) malloc (sizeof (yyt) * (yyCntlMax + 200));
      if (p == 0) {
	 printf("NEWBLOCK: running out of memory\n");
	 exit(1);
      }
   }
   else {
      p = FREELIST;
      FREELIST = (yyt) *p;
   }

   return p;
}

static FREEBLOCK (p)
   yyt p;
{
   *p = (intptr_t) FREELIST;
   FREELIST = p;
}

yyt yyAllocCntl(n)
{
   yyt p;
   p = CURPOS;
   CURPOS += n;
   if (CURPOS >= CURBLOCK+yyCntlMax) {
      yyt b;
      b = NEWBLOCK();
      *b = 0;
      *CURBLOCK = (intptr_t) b;
      CURBLOCK = b;
      CURPOS = CURBLOCK + 1;
      p = CURPOS;
      CURPOS += n;
   }
   return p;
}

typedef struct {
   yyt firstblock;
   yyt curblock;
   yyt curpos;
} yysave;

yyBeginChoice(ref_saved)
   yysave *ref_saved;
{
   ref_saved->curblock = CURBLOCK;
   ref_saved->curpos = CURPOS;
   ref_saved->firstblock = FIRSTBLOCK;

   FIRSTBLOCK = NEWBLOCK();
   *FIRSTBLOCK = 0;
   CURBLOCK = FIRSTBLOCK;
   CURPOS = CURBLOCK + 1;
}

yyEndChoice(saved)
   yysave saved;
{
   yyt p;

   p = FIRSTBLOCK;
   while (p != (yyt) 0) {
      yyt next;
      next = (yyt) *p;
      FREEBLOCK(p);
      p = next;
   }

   CURBLOCK = saved.curblock;
   CURPOS = saved.curpos;
   FIRSTBLOCK = saved.firstblock;
}

yyAbort (Code, FileName, Line)
   int Code;
   char * FileName;
   int Line;
{
   switch(Code) {
   case 1:
      printf ("Undefined value in \"%s.g\", line %d\n", FileName, Line);
      exit(1);
   case 2:
      printf ("No rule applicable in \"%s.g\", line %d\n", FileName, Line);
      exit(1);
   case 3:
      printf ("Selected grammar rule failed in \"%s.g\", line %d\n",
	 FileName, Line);
      exit(1);
   case 4:
      printf ("Selected CHOICE rule failed in \"%s.g\", line %d\n",
	 FileName, Line);
      exit(1);
   default:
      printf ("Error %d (?) in \"%s.g\", line %d\n", Code, FileName, Line);
      exit(1);
   }
}

yyPrintOpaque (i)
   long i;
{
/* --PATCH-- */   printf("<<%ld>>", i);
}

yyPrintIndex (i)
   long i;
{
/* --PATCH-- */   printf("#%ld", i);
}

yyPrint_INT (i)
   long i;
{
/* --PATCH-- */   printf("%ld", i);
}

yyPrint_POS (i)
   long i;
{
/* --PATCH-- */   printf("%ld", i);
}

#define STRINGLENGTH 40

yyPrint_STRING (Str)
   char *Str;
{
   char OutBuf[STRINGLENGTH];
   char * OutBufPtr;
   OutBufPtr = &OutBuf[0];

   while(*Str) {
      if (OutBufPtr > &OutBuf[STRINGLENGTH-4]) {
	 *OutBufPtr++ = '.';
	 *OutBufPtr++ = '.';
	 *OutBufPtr++ = '.';
	 break;
      }

      switch (*Str) {

      case '\\':
	 *OutBufPtr++ = '\\';
	 *OutBufPtr++ = '\\';
	 Str++;
	 break;
      case '\n':
	 *OutBufPtr++ = '\\';
	 *OutBufPtr++ = 'n';
	 Str++;
	 break;
      case '\"':
	 *OutBufPtr++ = '\\';
	 *OutBufPtr++ = '"';
	 Str++;
	 break;

      default:
         *OutBufPtr++ = *Str++; 
      }
   }
   *OutBufPtr = '\0';

   printf("\"%s\"", OutBuf);
}

static long yyIndentation = 0;

static yyIndent()
{
   int i;

   for (i = 1; i <= yyIndentation;  i++) {
      printf("   ");
   }
}

yyTerm(f)
/* --PATCH-- */ long f;
{
/* --PATCH-- */   printf("%s", (const char *)f);
}

yyFirstArg()
{
   printf("(\n");
   yyIndentation++;
   yyIndent();
}

yyNextArg()
{
   printf(",\n");
   yyIndent();
}

yyEndArgs()
{
   yyIndentation--;
   printf("\n");
   yyIndent();
   printf(")");
}

yyNoArgs()
{
   ;
}

yyEndPrint()
{
   printf("\n");
}
