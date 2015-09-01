/* --PATCH-- */ #include <stdlib.h>
/* --PATCH-- */ #include <string.h>
/*
   GENTLE 97 CAMPUS EDITION

   COPYRIGHT (C) 1992, 1997. All rights reserved.

   Metarga GmbH, Joachim-Friedrich-Str. 54, D-10711 Berlin

   gentle-97-v-4-1-0
*/

extern const char *MapFile(const char *);

static long is_defined();
static open_next_file ();
static next_file ();
static FillBuf ();

/*----------------------------------------------------------------------------*/
/*  Input Files                                                               */
/*----------------------------------------------------------------------------*/

/*
.include <fcntl.h>
*/
#include <stdio.h>

#define MAXPATH            500

#define PATHLENGTH          4096

static FILE   *InFile;
/* --PATCH-- */ static char   InFileName[PATHLENGTH];
/* --PATCH-- */ static char   CurFileName[PATHLENGTH];
/* --PATCH-- */ const char*   InputDir = NULL;
static long   filecount = 0;
static long   CurFile = 0;
static char * PATH[MAXPATH];

/*----------------------------------------------------------------------------*/

static FILE *SourceFile ()
{
   return InFile;
}

/*----------------------------------------------------------------------------*/

char * SourceName ()
{
   return InFileName;
}

/*----------------------------------------------------------------------------*/

DefSourceName (str)
   char *str;
{
   strcpy (InFileName, str);
   InFileName[strlen(InFileName)-2] = '\0'; /* strip extension */
}

/*----------------------------------------------------------------------------*/

GetSourceName (str)
   char ** str;
{
/* --PATCH-- */
#ifndef _WIN32
	char* file = strrchr(InFileName, '/');
#else
	char* file = strrchr(InFileName, '\\');
#endif
	if (file == NULL)
		file = InFileName;
	else
		file++;
	return *str = file;
}

/*----------------------------------------------------------------------------*/

static FILE * open_file (unit)
   char * unit;
{
   char buf[PATHLENGTH];
   FILE * InFile;
      
   sprintf (buf, "%s.g", unit);
   InFile = fopen (MapFile(buf), "r");
   if (InFile == NULL) {
      char msg[200];
      sprintf(msg, "Cannot open file '%s'\n", buf);
      Fatal (msg);
   }
   
   return InFile;
}

/*----------------------------------------------------------------------------*/

static open_next_file ()
{
   long i;
   char *p;

   if (CurFile > 0) fclose(InFile);

   CurFile++;
   
   p = PATH[CurFile];
   i = 0;
   while (*p) {
      CurFileName[i++] = *p++;
   }
   
   CurFileName[i++] = '\0';
   InFile = open_file(CurFileName);
}

/*----------------------------------------------------------------------------*/

define_file (path)
char * path;
{
    /* --PATCH-- */ char partpath[PATHLENGTH];
    /* --PATCH-- */ char fullpath[PATHLENGTH];
    
#ifdef _WIN32
#  define sep '\\'
#else
#  define sep '/'
#endif
    
/* --PATCH-- */ if (strchr(path, sep) == NULL)
/* --PATCH-- */ {
/* --PATCH-- */     if (InputDir != NULL)
/* --PATCH-- */         sprintf(partpath, "%s%c%s", InputDir, sep, path);
/* --PATCH-- */     else
/* --PATCH-- */         strncpy(partpath, path, PATHLENGTH);
/* --PATCH-- */ }
/* --PATCH-- */ else
/* --PATCH-- */ {
/* --PATCH-- */     strncpy(partpath, path, PATHLENGTH);
/* --PATCH-- */ }
/* --PATCH-- */
/* --PATCH-- */ if (partpath[0] != '/' && partpath[1] != ':')
/* --PATCH-- */ {
/* --PATCH-- */     char cwd[PATHLENGTH];
/* --PATCH-- */     getcwd(cwd, PATHLENGTH);
/* --PATCH-- */
/* --PATCH-- */     sprintf(fullpath, "%s%c%s", cwd, sep, partpath);
/* --PATCH-- */ }
/* --PATCH-- */ else
/* --PATCH-- */     sprintf(fullpath, "%s", partpath);
/* --PATCH-- */   if (! is_defined(fullpath)) {
        filecount++;
        if (filecount >= MAXPATH) {
            Fatal("pathtable overflow\n");
        }
/* --PATCH-- */      PATH[filecount] = strdup(fullpath);
    }
}

/*----------------------------------------------------------------------------*/

static long is_defined(path)
   char * path;
{
   long i;
   for (i = 1; i <= filecount; i++)
      if (strcmp(path, PATH[i]) == 0) return 1;
   return 0;
}

/*----------------------------------------------------------------------------*/

static long more_files()
{
   return (filecount > CurFile);
}

/*----------------------------------------------------------------------------*/

static init_FILE ()
{
   define_file (InFileName);
   next_file ();
}

/*----------------------------------------------------------------------------*/
/*  Input Buffer                                                              */
/*----------------------------------------------------------------------------*/

#ifdef MAC
#define EOL 13
#else
#define EOL '\012'
#endif

#define CR            '\015'
#define CTL_Z         '\032'

#define EOB           '\003'

#define maxpos            5000
#define bufsize           5002

static long   CurCol;
static long   CurLine;

static char*  bufptr;
static char*  lastptr;
static char*  sentinelptr;
static char*  firstcolptr;
static char   buf [bufsize];
static ResetBuffer ();

/*----------------------------------------------------------------------------*/

static next_file ()
{
   open_next_file ();
   ResetBuffer ();
}

/*----------------------------------------------------------------------------*/

static ResetBuffer ()
{
   lastptr = &buf[0];
   bufptr = &buf[0];
   sentinelptr = bufptr;

   CurLine = 1;
   firstcolptr = &buf[1];

   FillBuf();
}

/*----------------------------------------------------------------------------*/

static FillBuf ()
{
   register char *i, *p;
   long n;
   long nbytes;

   /*

   [1 .. curpos-1] [curpos] [curpos+1 .. lastpos] [lastpos+1 .. maxpos]
   already         sentinel not yet               free                 
   processed                processed                                  

   */
   

   /* copy rest of buffer to beginning */
   p = &buf[0]; i = bufptr;
   while (i < lastptr) {
      *++p = *++i;
   }

   /* fill buffer */
   nbytes = &buf[maxpos]-p;
   {
   FILE * s;
   s = SourceFile();
   n = fread(p+1, 1, nbytes, s);
   }
   lastptr = p+n;

   bufptr = &buf[1];

   /* set new sentinel */
   buf[0] = EOL;
   i = lastptr;
   while (*i != EOL) i--;     

   sentinelptr = i;
   *(lastptr+1) = EOB;
}

/*----------------------------------------------------------------------------*/
/* Macros to access input buffer                                              */
/*----------------------------------------------------------------------------*/

#define NEXTCH  BUFPTR++
#define PREVCH  BUFPTR--
#define CH      *BUFPTR
#define FOLLOWINGCH      *(BUFPTR+1)
#define AGAIN   goto again
#define MARK(p) p = BUFPTR

#define RECOG(x) {                                   \
   bufptr = BUFPTR;                                  \
   yylval.attr[0] =                                      \
	     CurFile * MaxLines * MaxCols            \
	   + CurLine * MaxCols                       \
           + (BUFPTR - firstcolptr);                 \
   return(x);                                        \
}

#define NEXTLINE {                                   \
   if (BUFPTR == sentinelptr) {                      \
      bufptr = BUFPTR; FillBuf(); BUFPTR = bufptr;   \
   }                                                 \
   else BUFPTR++;                                    \
   CurLine++; firstcolptr = BUFPTR;                  \
}

/*----------------------------------------------------------------------------*/
/*  Positions                                                                 */
/*----------------------------------------------------------------------------*/

#define MaxLines         10000
#define MaxCols            100

/*----------------------------------------------------------------------------*/

yyGetPos (n)
	long *n;
{
	*n = 
	     CurFile * MaxLines * MaxCols
	   + CurLine * MaxCols
	   + CurCol;
}

/*----------------------------------------------------------------------------*/

char * GetFileName (Pos)
	long Pos;
{
	if (Pos == 0)
	   return "?";
	else
	   return PATH [ Pos / ((long) MaxLines * (long) MaxCols) ];
}

/*----------------------------------------------------------------------------*/

long GetLine (Pos)
	long Pos;
{

	return ( Pos % ((long) MaxLines * (long) MaxCols) ) / (long) MaxCols;
}

/*----------------------------------------------------------------------------*/

PosToLineNumber (Pos, N)
   long Pos; long *N;
{
   *N = GetLine(Pos);
}

/*----------------------------------------------------------------------------*/

long GetCol (Pos)
	long Pos;
{
	return Pos % (long) MaxCols;
}

/*----------------------------------------------------------------------------*/
/*  Scanner                                                                   */
/*----------------------------------------------------------------------------*/

#include "gen.h"
extern YYSTYPE yylval; /* also declared in yytokens.h */

#define sy_UNDEF 9999
#define sy_eof 0

#define TAB '\011'
#define VTAB '\v'
#define FF '\f'

#define max_kw_length 50

static long    IsLetgit [256];
static long    IsDigit [256];

int FILESEP_recognized = 0;

/*----------------------------------------------------------------------------*/

long yylex ()
{
   register char *BUFPTR;

   BUFPTR = bufptr;

again:

   CurCol = BUFPTR - firstcolptr + 1;

   switch (*BUFPTR) {

   /*--- simple tokens -------------------------------------------------------*/

   case '.' : NEXTCH; RECOG(DOT)

   case ',' : NEXTCH; RECOG(COMMA)

   case ')' : NEXTCH; RECOG(RIGHTPAREN)

   case '+' : NEXTCH; RECOG(PLUS)

   case '_' : NEXTCH; RECOG(UNDERSCORE)

   case '@' : NEXTCH; RECOG(AMPERSAND)

   case ']' : NEXTCH; RECOG(RIGHTBRACKET)

   case '{' : NEXTCH; RECOG(BEGINDISJ)

   case '}' : NEXTCH; RECOG(ENDDISJ)

   case '$' : NEXTCH; RECOG(DOLLAR)


   /* "*", "*>" */

   case '*' :
      NEXTCH;
      if (CH == '>') {
	 NEXTCH;
	 RECOG(ENDLOOP);
      }
      else
         RECOG(TIMES)

   /* ">>" */

   case '>' :
      NEXTCH;
      if (CH == '>') {
	 NEXTCH; RECOG(ENDDISJ)
      } else {
	 ScanError("invalid character after '>'");
      }

   /* ":", ":=", ":-" */

   case ':' :
      NEXTCH;
      if (CH == '=') {
	 NEXTCH; RECOG(BECOMES)
      } else if (CH == '-') {
	 NEXTCH; RECOG(SMALLBECOMES)
      } else {
	 RECOG(COLON)
      }

   /* "=", "=:" */

   case '=' :
      NEXTCH;
      if (CH == ':') {
	 NEXTCH; RECOG(COMESBE)
      } else {
	 RECOG(EQ)
      }

   /* "(", "(|" */

   case '(' :
      NEXTCH;
      if (CH == '|') {
	 NEXTCH; RECOG(BEGINDISJ)
      } else {
	 RECOG(LEFTPAREN)
      }

   /* "[", "[|" */
   case '[' :
      NEXTCH;
      if (CH == '|') {
	 NEXTCH; RECOG(BEGINCOND)
      } else {
	 RECOG(LEFTBRACKET)
      }

   /* "|", "||"; "|]; "|)" */
   case '|' :
      NEXTCH;
      if (CH == '|') {
	 NEXTCH; RECOG(DISJDELIM)
      } else if (CH == ']') {
	 NEXTCH; RECOG(ENDCOND)
      } else if (CH == ')') {
	 NEXTCH; RECOG(ENDDISJ)
      }else {
         RECOG(DISJDELIM)
      }


   /* "-", "->", "-:", "--" */
   case '-' :
      NEXTCH;
      if (CH == '>') {
	 NEXTCH; RECOG(RIGHTARROW)
      } else if (CH == ':') {
	 NEXTCH; RECOG(SMALLCOMESBE)
      } else if (CH == '-') { /* line comment */
	 while (CH != EOL && CH != EOB)
	    NEXTCH;
	 AGAIN;
      } else {
	 RECOG(MINUS)
      }

   /* "<-", "<<", "<*" */
   case '<' :
      NEXTCH;
      if (CH == '-') {
	 NEXTCH;
	 RECOG(LEFTARROW)
      } else if (CH == '<') {
	 NEXTCH;
	 RECOG(BEGINDISJ)
      } else if (CH == '*') {
	 NEXTCH;
	 RECOG(BEGINLOOP)
      } else {
	 ScanError("invalid character after '<'");
      }

/* --PATCH-- */   /* "/", "/ *" */
   case '/' :
      NEXTCH;
      if (CH == '*') { /* comment */
	 long nest = 1;
	 NEXTCH;

	 for (;;) {
	    if (CH == EOL) {
	       NEXTLINE;
	    }
	    else if (CH == '*') {
	       NEXTCH;
	       if (CH == '/') {
		  NEXTCH;
		  nest--;
		  if (nest == 0)
		     AGAIN;
	       }
	       else if (CH == EOL) {
		  NEXTLINE;
	       }
	       else if (CH == EOB) {
		  ScanError("comment not closed");
	       }
	    }
	    else if (CH == '/') {
	       NEXTCH;
	       if (CH == '*') {
		  NEXTCH;
		  nest++;
	       }
	    }
	    else if (CH == EOB) {
	       ScanError("comment not closed");
	    }
	    else
	       NEXTCH;
	 }

      }
      else {
	 RECOG(DIV)
      }

   /*--- identifiers ---------------------------------------------------------*/

   case 'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' :
   case 'g' : case 'h' : case 'i' : case 'j' : case 'k' : case 'l' :
   case 'm' : case 'n' : case 'o' : case 'p' : case 'q' : case 'r' :
   case 's' : case 't' : case 'u' : case 'v' : case 'w' : case 'x' :
   case 'y' : case 'z' : case 'A' : case 'B' : case 'C' : case 'D' :
   case 'E' : case 'F' : case 'G' : case 'H' : case 'I' : case 'J' :
   case 'K' : case 'L' : case 'M' : case 'N' : case 'O' : case 'P' :
   case 'Q' : case 'R' : case 'S' : case 'T' : case 'U' : case 'V' :
   case 'W' : case 'X' : case 'Y' : case 'Z' :

      { 
	 char *idstart, *idstop;
	 char *CurId; /* Ident */
	 long SYM;
	 
	 MARK(idstart);
         while (IsLetgit[CH]) NEXTCH;
	 MARK(idstop);
         slice_to_id(idstart, idstop, &CurId);

	 SYM = get_symbol_class (CurId);
         yylval.attr[1] = (long) CurId;
         RECOG(SYM)
      }

   /*--- keywords ------------------------------------------------------------*/

   case '\'' :

      {
	 char string[max_kw_length-1];
	 long pos;
	 char * oldbufptr;

	 NEXTCH;
	 oldbufptr = BUFPTR;
	 pos = 0;

	 for (;;) {
	    if (CH == '\'') {
	       NEXTCH;
	       string[pos] = '\0';

               if (strcmp (string, "MODULE") == 0) RECOG(MODULE)
               else if (strcmp (string, "module") == 0) RECOG(MODULE)
               else if (strcmp (string, "EXPORT") == 0) RECOG(EXPORTTOKEN)
               else if (strcmp (string, "export") == 0) RECOG(EXPORTTOKEN)
               else if (strcmp (string, "IMPORT") == 0) RECOG(IMPORTTOKEN)
               else if (strcmp (string, "import") == 0) RECOG(IMPORTTOKEN)
               else if (strcmp (string, "USE") == 0) RECOG(USE)
               else if (strcmp (string, "use") == 0) RECOG(USE)
               else if (strcmp (string, "END") == 0) RECOG(END)
               else if (strcmp (string, "end") == 0) RECOG(END)
               else if (strcmp (string, "VAR") == 0) RECOG(VAR)
               else if (strcmp (string, "var") == 0) RECOG(VAR)
               else if (strcmp (string, "CLASS") == 0) RECOG(CLASSTOKEN)
               else if (strcmp (string, "class") == 0) RECOG(CLASSTOKEN)
               else if (strcmp (string, "TYPE") == 0) RECOG(TYPETOKEN)
               else if (strcmp (string, "type") == 0) RECOG(TYPETOKEN)
               else if (strcmp (string, "ACTION") == 0) RECOG(PROC)
               else if (strcmp (string, "action") == 0) RECOG(PROC)
               else if (strcmp (string, "CONDITION") == 0) RECOG(COND)
               else if (strcmp (string, "condition") == 0) RECOG(COND)
               else if (strcmp (string, "PREDICATE") == 0) RECOG(COND)
               else if (strcmp (string, "predicate") == 0) RECOG(COND)
               else if (strcmp (string, "SWEEP") == 0) RECOG(SWEEP)
               else if (strcmp (string, "sweep") == 0) RECOG(SWEEP)
               else if (strcmp (string, "NONTERM") == 0) RECOG(NONTERMTOKEN)
               else if (strcmp (string, "nonterm") == 0) RECOG(NONTERMTOKEN)
               else if (strcmp (string, "TOKEN") == 0) RECOG(TOKENTOKEN)
               else if (strcmp (string, "token") == 0) RECOG(TOKENTOKEN)
               else if (strcmp (string, "CHOICE") == 0) RECOG(CHOICE)
               else if (strcmp (string, "choice") == 0) RECOG(CHOICE)
               else if (strcmp (string, "RULE") == 0) RECOG(RULETOKEN)
               else if (strcmp (string, "rule") == 0) RECOG(RULETOKEN)
               else if (strcmp (string, "ROOT") == 0) RECOG(ROOT)
               else if (strcmp (string, "root") == 0) RECOG(ROOT)
               else if (strcmp (string, "TABLE") == 0) RECOG(TABLE)
               else if (strcmp (string, "table") == 0) RECOG(TABLE)
               else if (strcmp (string, "CLASS") == 0) RECOG(TABLE)
               else if (strcmp (string, "class") == 0) RECOG(TABLE)
               else if (strcmp (string, "KEY") == 0) RECOG(KEY)
               else if (strcmp (string, "key") == 0) RECOG(KEY)
               else if (strcmp (string, "cost") == 0) RECOG(DOLLAR)
	       else {
	          ScanError("unknown keyword");
	       }
	    }

	    else if (pos >= max_kw_length || !IsLetgit[CH]) {
	       BUFPTR = oldbufptr;
	       RECOG(QUOTE)
	    }
	    else {
	       string[pos] = CH;
	       NEXTCH;
	       pos++;
	    }
	 }
      }

      break;

   /*--- strings -------------------------------------------------------------*/

   case '"' :

      {
	 long length;
	 char CurChar;

	 NEXTCH;
	 length = 0;

	 for (;;) {
	    if (CH == '"') {
	       NEXTCH;
	       close_string ((char **) &yylval.attr[1]);
	       RECOG(STRINGCONST)
	    }
	    else if (CH == '\\') {
	       NEXTCH;
	       if (CH == EOL) {
		  /* Error : eol inside char or string constant */
		  ScanError("eol inside string const");
	       }
	       else if (CH == EOB) {
		  ScanError("eof inside string const");
	       }
	       else if (CH == 'n') {
		  CurChar = '\n';
	       }
	       else if (CH == 't') {
		  CurChar = '\t';
	       }
	       else {
	          CurChar = CH;
	       }
	       NEXTCH; length++;
	    }
	    else if (CH == EOL) {
	       ScanError("eol inside string const");
	    }
	    else if (CH == EOB) {
	       ScanError("eof inside string const");
	    }
	    else {
	       CurChar = CH; NEXTCH; length++;
	    }
	    app_to_string(CurChar);
	 }
      }

      break;

   /*--- numbers -------------------------------------------------------------*/

   case '0' : case '1' : case '2' : case '3' : case '4' :
   case '5' : case '6' : case '7' : case '8' : case '9' :

      {
	 long CurNumber;

	 CurNumber = 0;
	 while (1) {
	    switch (CH) {
	    case '0' : case '1' : case '2' : case '3' : case '4' :
	    case '5' : case '6' : case '7' : case '8' : case '9' :
	       CurNumber = (CurNumber * 10) + (CH - '0');
	       NEXTCH;
	       break;
	    default :
	       yylval.attr[1] = (long) CurNumber;
	       RECOG(INTEGERCONST)
	    }
	 }
      }

      break;

   /*--- layout --------------------------------------------------------------*/

   case ' ' : NEXTCH; AGAIN;
   case TAB : NEXTCH; AGAIN;
   case VTAB : NEXTCH; AGAIN;
   case FF : NEXTCH; AGAIN;
#ifndef MAC
   case CR : NEXTCH; AGAIN;
#endif
   case CTL_Z : NEXTCH; AGAIN;
   case EOL : NEXTLINE; AGAIN;

   /*--- eof -----------------------------------------------------------------*/

   case EOB :
      {
	 static long at_eof = 0;

         if (at_eof) {
	    RECOG(sy_eof)
         }
         else if (more_files()) {
	    /* first recognize FILESEP
	       then open next file
	       in order to produce correct source positions for errors at eof
	    */
	    if (FILESEP_recognized) {
	       bufptr = BUFPTR;
	       next_file();
	       BUFPTR = bufptr;
               FILESEP_recognized = 0;
	       AGAIN;
	    }
	    else {
	       FILESEP_recognized = 1;
	       RECOG(FILESEP);
	    }
         }
         else {
	    at_eof = 1;
	    RECOG(EOFTOKEN);
         }
      }

   default :
      CurCol = BUFPTR + 1 - firstcolptr; 
     
      BUFPTR++;
      RECOG(sy_UNDEF);
   };

   RECOG(sy_UNDEF) /* not reached */
}

/*----------------------------------------------------------------------------*/

init_scanner ()
{
   char ch;
   long i;
   
   for (i = 0; i <= 255; i++) {
      ch = i;
      switch (ch) {

      case '0' : case '1' : case '2' : case '3' : case '4' : case '5' :
      case '6' : case '7' : case '8' : case '9' :

	 IsDigit[i] = 1;

      case 'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' :
      case 'g' : case 'h' : case 'i' : case 'j' : case 'k' : case 'l' :
      case 'm' : case 'n' : case 'o' : case 'p' : case 'q' : case 'r' :
      case 's' : case 't' : case 'u' : case 'v' : case 'w' : case 'x' :
      case 'y' : case 'z' :

      case 'A' : case 'B' : case 'C' : case 'D' : case 'E' : case 'F' :
      case 'G' : case 'H' : case 'I' : case 'J' : case 'K' : case 'L' :
      case 'M' : case 'N' : case 'O' : case 'P' : case 'Q' : case 'R' :
      case 'S' : case 'T' : case 'U' : case 'V' : case 'W' : case 'X' :
      case 'Y' : case 'Z' :

      case '_' :

         IsLetgit[i] = 1;
         break;

      default:

	 IsDigit[i] = 0;
         IsLetgit[i] = 0;
      }
   }

   init_FILE ();
}

/*----------------------------------------------------------------------------*/
/*  Strings                                                                   */
/*----------------------------------------------------------------------------*/

#define MAXSTRING 300
#define STRINGTABPIECE 10000

char string_buffer [MAXSTRING];
int string_buf_index = 0;

app_to_string (ch)
   char ch;
{
   if (string_buf_index >= MAXSTRING-1) {
      ScanError("string too long");
   }
   string_buffer[string_buf_index] = ch;
   string_buf_index++;
}

char * string_tab_ptr = (char *) 222;
char * string_tab_endptr = (char *) 111; /* < 222 : triggers allocation */

close_string(ref_str)
   char **ref_str;
{
   int i;

   if (string_tab_ptr + string_buf_index + 1 > string_tab_endptr) {
      string_tab_ptr = (char *) malloc(STRINGTABPIECE);
      if (string_tab_ptr == 0) {
	 printf("String Table: running out of memory\n");
	 exit(1);
      }
      string_tab_endptr = string_tab_ptr + STRINGTABPIECE - 1;
   }

   *ref_str = string_tab_ptr;

   for (i = 0; i < string_buf_index; i++) {
      *(string_tab_ptr++) = string_buffer[i];
   }
   *(string_tab_ptr++) = '\0';

   string_buf_index = 0;
}

/*----------------------------------------------------------------------------*/
