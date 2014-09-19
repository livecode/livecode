/*
   GENTLE 97 CAMPUS EDITION

   COPYRIGHT (C) 1992, 1997. All rights reserved.

   Metarga GmbH, Joachim-Friedrich-Str. 54, D-10711 Berlin

   gentle-97-v-4-1-0
*/


#include <stdio.h>

#define OutBufSize 6000
#define FlushPos 5000


static char OutBuf[OutBufSize];
static char *OutBufPtr;
static FILE *OutFile;
static long OutFileIsOpen = 0;

/*----------------------------------------------------------------------------*/

Tell(Name)
   char *Name;
{
   Told();
   OutFile = fopen(Name, "w");
   if (OutFile == NULL) {
      char msg[200];
      sprintf(msg, "cannot open %s\n", Name);
      exit(1);
   }
   OutBufPtr = &OutBuf[0];
   OutFileIsOpen = 1;
}

/*----------------------------------------------------------------------------*/

Told()
{
   if (OutFileIsOpen) {
      fwrite(OutBuf, 1, OutBufPtr - &OutBuf[0], OutFile);
      fclose(OutFile);
      OutFileIsOpen = 0;
   }
}

/*----------------------------------------------------------------------------*/

s(Str)
   char *Str;
{
   while(*Str) {
      *OutBufPtr++ = *Str++; 
   }
}

/*----------------------------------------------------------------------------*/

qu_s(Str)
   char *Str;
{
   *OutBufPtr++ = '\"';

   while(*Str) {
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

   *OutBufPtr++ = '\"';
}

/*----------------------------------------------------------------------------*/

doublequote ()
{
   s("\"");
}

/*----------------------------------------------------------------------------*/

i(N)
   long N;
{
   long butlast;
   long last;
   if (N < 0) {
      *OutBufPtr++ = '-';
      N = - N;
   }
   last = N % 10;
   butlast = N / 10;
   if (butlast > 0) i(butlast);
   *OutBufPtr++ = last + '0';
}

/*----------------------------------------------------------------------------*/

nl()
{
#ifdef EMIT_CR
   *OutBufPtr++ = '\r';
#endif
   *OutBufPtr++ = '\n';
   if (OutBufPtr > &OutBuf[FlushPos]) {
      fwrite(OutBuf, 1, OutBufPtr - &OutBuf[0], OutFile);
      OutBufPtr = &OutBuf[0];
   }
}

/*----------------------------------------------------------------------------*/

static long SUBDIR = 0;

/*----------------------------------------------------------------------------*/

SetOption_SUBDIR()
{
   SUBDIR = 1;
}

/*----------------------------------------------------------------------------*/

TellFile(Name)
   char *Name;
{
   char buf[200];

   if (SUBDIR)
      sprintf(buf, "_G_/%s", Name);
   else
      sprintf(buf, "%s", Name);
   Tell(buf);
}

/*----------------------------------------------------------------------------*/

TellClauseFile()
{
   char name[100];
   extern char *SourceName();
   
   sprintf(name, "%s.c", SourceName());

   TellFile(name);
}

/*----------------------------------------------------------------------------*/

TellSymbolFile()
{
   char name[100];
   extern char *SourceName();
   
   sprintf(name, "%s.if", SourceName());

   TellFile(name);
}

/*----------------------------------------------------------------------------*/

TellXRefFile()
{
   char name[100];
   extern char *SourceName();
   
   sprintf(name, "%s.nst", SourceName());

   TellFile(name);
}

/*----------------------------------------------------------------------------*/
