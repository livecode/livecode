/* --PATCH-- */ #include <stdlib.h>
/*
   GENTLE 97 CAMPUS EDITION

   COPYRIGHT (C) 1992, 1997. All rights reserved.

   Metarga GmbH, Joachim-Friedrich-Str. 54, D-10711 Berlin

   gentle-97-v-4-1-0
*/


#include <stdio.h>

/*----------------------------------------------------------------------------*/

static printpos (pos)
   long pos;
{
   if (pos == 0)
      printf("at unknown position: ");
   else {
      extern char *GetFileName();
      extern long GetLine();
      extern long GetCol();
/* --PATCH-- */      printf("%s.g:%ld:%ld: ",
         GetFileName(pos), GetLine(pos), GetCol(pos));
   }      
}

/*----------------------------------------------------------------------------*/

int Option_ALERT = 0;

SetOption_ALERT()
{
   Option_ALERT= 1;
}

/*----------------------------------------------------------------------------*/

/* --PATCH-- */ extern int ErrorOccurred;

MESSAGE(msg, pos)
   char *msg;
   long pos;
{


#ifdef MAC

   {
      extern char *GetFileName();
      extern long GetLine();
      extern long GetCol();
	  
      if (Option_ALERT) {
	 FILE *CmdFile;

         CmdFile = fopen ("CmdFile", "w");
         if (CmdFile < 0) {
            Fatal("Cannot open CmdFile");
         }
	 fprintf(CmdFile, "Open \"%s.g\"; Find %ld \"%s.g\"\n",
	    GetFileName(pos), GetLine(pos), GetFileName(pos));
	 fprintf(CmdFile, "Alert -s %s\n", msg);
	 exit (2);

      }
      else {
	 printf("### Error: %s\n", msg);
         printf("    File \"%s.g\"; Line %ld\n", GetFileName(pos), GetLine(pos));
      }
	
      exit(1);
   }

#else

   printpos(pos);
   printf("%s\n", msg);
    /* --PATCH-- */ // exit(1);
    /* --PATCH-- */ ErrorOccurred = 1;
    
#endif

   
}

/*----------------------------------------------------------------------------*/

MESSAGE1(msg1, id, msg2, pos)
   char *msg1;
   long id;
   char *msg2;
   long pos;
{
   char buf [200];
   char *str;
   id_to_string (id, &str);
   sprintf(buf, "%s%s%s", msg1, str, msg2);
   MESSAGE (buf, pos);
}

/*----------------------------------------------------------------------------*/

MESSAGE2(msg1, id1, msg2, id2, msg3, pos)
   char *msg1;
   long id1;
   char *msg2;
   long id2;
   char *msg3;
   long pos;
{
   char buf[200];
   char *str1;
   char *str2;
   id_to_string (id1, &str1);
   id_to_string (id2, &str2);
   sprintf(buf, "%s%s%s%s%s", msg1, str1, msg2, str2, msg3);
   MESSAGE (buf, pos);
}

/*----------------------------------------------------------------------------*/

yyerror(msg)
   char *msg;
{
   long pos;
   yyGetPos(&pos);
   MESSAGE(msg, pos);
}

/*----------------------------------------------------------------------------*/

yyerrorexit(rc)
   int rc;
{
   exit(1);
}

/*----------------------------------------------------------------------------*/

ScanError (msg)
   char * msg;
{
   long pos;   
   yyGetPos(&pos);
   MESSAGE (msg, pos);
}

/*----------------------------------------------------------------------------*/

Fatal (msg)
   char * msg;
{
   printf("Fatal Error: %s\n", msg);
   exit(1);
}

/*----------------------------------------------------------------------------*/
