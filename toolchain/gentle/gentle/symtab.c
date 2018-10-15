/* --PATCH-- */ #include <stdlib.h>
/* --PATCH-- */ #include <stdio.h>
/* --PATCH-- */ #include <stdint.h>
/*
   GENTLE 97 CAMPUS EDITION

   COPYRIGHT (C) 1992, 1997. All rights reserved.

   Metarga GmbH, Joachim-Friedrich-Str. 54, D-10711 Berlin

   gentle-97-v-4-1-0
*/


#include "gen.h"

typedef struct IdentRec *Ident;

struct IdentRec
{
   char  *firstposptr;
   intptr_t  length;
   Ident next;

   intptr_t  symbol_class;
     
   intptr_t  GlobalMeaning;
   intptr_t  LocalMeaning;
   intptr_t  LocalMeaning2;
   intptr_t  FunctorMeaning;
   intptr_t  ExportFlag;

};

char *idstringtab_ptr;
char *idstringtab_endptr;

struct IdentRec *idtab_ptr;
struct IdentRec *idtab_endptr;

#define HashTabSize   2048 /* should be a power of two */

Ident HashTab [HashTabSize];

#define STRINGTAB_PIECE  10000
#define STRINGTAB_EXTRA    500
#define IDTABSIZE 500

typedef struct IdentRec IDTAB [IDTABSIZE]; /* preliminary */

/*----------------------------------------------------------------------------*/

static allocate_idstringtab ()
{
   idstringtab_ptr =
      (char *) malloc (STRINGTAB_PIECE + STRINGTAB_EXTRA);
   if (idstringtab_ptr == 0) {
      Fatal("Running out of storage (Id String Table).");
   }
   idstringtab_endptr = idstringtab_ptr + STRINGTAB_PIECE - 1;
}

/*----------------------------------------------------------------------------*/

static allocate_idtab ()
{
   idtab_ptr =
      (struct IdentRec *)
      malloc (sizeof (IDTAB /*struct IdentRec [IDTABSIZE]*/ ) );
   if (idtab_ptr == 0) {
      Fatal("Running out of storage (Id Table).");
   }
   idtab_endptr = & idtab_ptr[IDTABSIZE - 1];
}

/*----------------------------------------------------------------------------*/

slice_to_id (idstart, idstop, ref_id)
   char *idstart; /* position of first character */
   char *idstop;  /* position  a f t e r  last character */
   Ident *ref_id;
{
	   intptr_t  hash, length;
   Ident chain;
   Ident  NewId;

   length = idstop-idstart;
   hash = ( length*256 + ((*idstart)&0xf)*16 + (*(idstop-1)&0xf) ) 
   & (HashTabSize-1);
   chain = HashTab[hash];

   for(;;) {
      if (chain == 0) {
      
	 /* not in table */
	 
	 NewId = idtab_ptr;
	    
	 if (idtab_ptr == idtab_endptr)
	    allocate_idtab();
         else
	    idtab_ptr++;
	    

	 /* copy id representation  id into idstringtab */
	 {
	    register char *i, *freeptr, *stop;

	    if (idstringtab_ptr > idstringtab_endptr)
	       allocate_idstringtab();
	    freeptr = idstringtab_ptr;

	    NewId->firstposptr = idstringtab_ptr;

	    i = idstart;
	    stop = idstop;
	    while (i < stop) {
	       *freeptr++ = *i++;
	    }
	    *freeptr = '\0';
	    freeptr++;
	    idstringtab_ptr= freeptr;
	 }

	 NewId->length = length;
	 NewId->next = HashTab[hash];
	    
	 HashTab[hash] = NewId;

	 
	 init_id_attributes (NewId);
   
	 break;
      }

      /* current token == ident at chain ? */
      
      if (chain->length == length) {
         register char *i, *j;
	 i = idstart; j = chain->firstposptr;
	 while (i != idstop && *i == *j) {
	    i++; j++;
         }

	 if (i == idstop && *j == '\0') {
	    
	    /* found */
	    
	    NewId = chain;
	    break;
	 }
      }

      chain = chain->next;
   }

   *ref_id = NewId;
}

/*----------------------------------------------------------------------------*/

string_to_id (idstart, ref_id)
   char *idstart;
   Ident *ref_id;
{
   char *idstop;

   idstop = idstart;
   while (*idstop != '\0') idstop++;
   slice_to_id (idstart, idstop, ref_id);
}

/*----------------------------------------------------------------------------*/

id_to_string (id, repr)
   Ident id;
   char **repr;
{
   *repr = id->firstposptr;
}

/*----------------------------------------------------------------------------*/

void init_idtab ()
{
	intptr_t i;

   for (i = 0; i<=HashTabSize-1; i++) HashTab[i] = 0;

   allocate_idtab ();
   allocate_idstringtab ();
}

/*----------------------------------------------------------------------------*/

init_id_attributes (new)
   Ident new;
{
   	       
   new-> LocalMeaning = 0;
   new-> GlobalMeaning = 0;
   new-> FunctorMeaning = 0;

   if ('a' <= *(new->firstposptr) && *(new->firstposptr) <= 'z') {
      new->symbol_class = SMALLID;
   }
   else {
      new->symbol_class = LARGEID;
   }

}

/*----------------------------------------------------------------------------*/

DefGlobalMeaning (id, m, f) Ident id; intptr_t m; intptr_t f;
{
   id->GlobalMeaning = m;
   id->ExportFlag = f;
}

/*----------------------------------------------------------------------------*/

DefFunctorMeaning (id, m) Ident id; intptr_t m;
{
   id->FunctorMeaning = m;
}

/*----------------------------------------------------------------------------*/

DefLocalMeaning (id, m1, m2) Ident id; intptr_t m1; intptr_t m2;
{
   id->LocalMeaning = m1;
   id->LocalMeaning2 = m2;
}

/*----------------------------------------------------------------------------*/

ForgetLocalMeaning (id) Ident id;
{
   id->LocalMeaning = 0;
}

/*----------------------------------------------------------------------------*/

long GetGlobalMeaning (id, m) Ident id; intptr_t *m;
{
   if (id->GlobalMeaning == 0)
      return 0;
   *m = id->GlobalMeaning;
   return 1;
}

/*----------------------------------------------------------------------------*/

long GetExportFlag (id, f) Ident id; intptr_t *f;
{
   if (id->GlobalMeaning == 0)
      return 0;
   *f = id->ExportFlag;
   return 1;
}

/*----------------------------------------------------------------------------*/

long GetFunctorMeaning (id, m) Ident id; intptr_t *m;
{
   if (id->FunctorMeaning == 0)
      return 0;
   *m = id->FunctorMeaning;
   return 1;
}

/*----------------------------------------------------------------------------*/

long GetLocalMeaning (id, m) Ident id; intptr_t *m;
{
   if (id->LocalMeaning == 0)
      return 0;
   *m = id->LocalMeaning;
   return 1;
}

/*----------------------------------------------------------------------------*/

long GetLocalMeaning2 (id, m) Ident id; intptr_t *m;
{
   if (id->LocalMeaning == 0)
      return 0;
   *m = id->LocalMeaning2;
   return 1;
}

/*----------------------------------------------------------------------------*/

long HasGlobalMeaning (id)
   Ident id;
{
   return id->GlobalMeaning != 0;
}

/*----------------------------------------------------------------------------*/

long HasLocalMeaning (id)
   Ident id;
{
   return id->LocalMeaning != 0;
}

/*----------------------------------------------------------------------------*/

long get_symbol_class (Id) Ident Id;
{
   return Id->symbol_class;
}

/*----------------------------------------------------------------------------*/

enter_keyword (class, idstart)
   int class;
   char *idstart;
{
   Ident NewId;
   
   string_to_id (idstart, &NewId);
   NewId->symbol_class = class;
}

/*----------------------------------------------------------------------------*/

static int tokencount = 0;

UniqueTokenId (ref_id)
   Ident *ref_id;
{
   char buffer[100];
   tokencount++;
   sprintf(buffer, "yytoken_%d", tokencount);
   string_to_id(buffer, ref_id);
}

/*----------------------------------------------------------------------------*/

int is_word(str)
   char *str;
{
   char *p;
   p = str;
   while (*p != 0) {
      if (('a' <= *p) && (*p <= 'z')) ;
      else if (('A' <= *p) && (*p <= 'Z')) ;
      else if (('0' <= *p) && (*p <= '9')) ;
      else return 0;
      p++;
   }
   return 1;
}

int namebufsize;

char *start_namebuf;
char *stop_namebuf;

char *namebufptr; /* next position for insertion */

#define NAMEBUFPIECE 100

resize_namebuf()
{
   char *new, *p;

   new = (char *) malloc(namebufsize+NAMEBUFPIECE);
   namebufsize += NAMEBUFPIECE;
   namebufptr = new;

   for (p = start_namebuf; p <= stop_namebuf; p++) {
      *namebufptr = *p;
      namebufptr++;
   }

   start_namebuf = new;
   stop_namebuf = start_namebuf+namebufsize-1;
}

static initialized = 0;

clear_buffer()
{
   if (! initialized) {
      initialized = 1;

      start_namebuf = (char *) malloc(NAMEBUFPIECE);
      namebufsize = NAMEBUFPIECE;
      stop_namebuf = start_namebuf+namebufsize-1;
   }
   namebufptr = start_namebuf;
}

append_to_buf(str)
   char *str;
{
   char *p;
   p = str;
   while(*p != 0) {
      *namebufptr = *p;
      if (namebufptr == stop_namebuf) resize_namebuf();
      else namebufptr++;
      p++;
   }
   *namebufptr = 0;
}
   
CreateNameForToken (Str, ref_id)
   char *Str;
   Ident *ref_id;
{


   clear_buffer();

   if (is_word(Str)) {
      append_to_buf("yykw_");
      append_to_buf(Str);
   }
   else {
      char *p, *b;

      append_to_buf("yytk");
      p = Str;
      while (*p != 0) {
	 switch (*p) {
         case '0': case '1': case '2': case '3': case '4': case '5':
         case '6': case '7': case '8': case '9': case 'A': case 'B':
         case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
         case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
         case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
         case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
         case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
         case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
         case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
         case 's': case 't': case 'u': case 'v': case 'w': case 'x':
         case 'y': case 'z':
	    {
	       char rep[5];
	       sprintf(rep, "_%c", *p);
	       append_to_buf(rep);
	    }
	 case ' ': append_to_buf("_BLANK"); break;
	 case '!': append_to_buf("_EXCLAM"); break;
	 case '"': append_to_buf("_DQUOTE"); break;
	 case '#': append_to_buf("_SHARP"); break;
	 case '$': append_to_buf("_DOLLAR"); break;
	 case '%': append_to_buf("_PERCENT"); break;
	 case '&': append_to_buf("_AMPERSAND"); break;
	 case '\'': append_to_buf("_BACKSLAH"); break;
	 case '(': append_to_buf("_LPAREN"); break;
	 case ')': append_to_buf("_RPAREN"); break;
	 case '*': append_to_buf("_ASTERISK"); break;
	 case '+': append_to_buf("_PLUS"); break;
	 case ',': append_to_buf("_COMMA"); break;
	 case '-': append_to_buf("_MINUS"); break;
	 case '.': append_to_buf("_DOT"); break;
	 case '/': append_to_buf("_SLASH"); break;
	 case ':': append_to_buf("_COLON"); break;
	 case ';': append_to_buf("_SEMICOLON"); break;
	 case '<': append_to_buf("_LESS"); break;
	 case '=': append_to_buf("_EQUAL"); break;
	 case '>': append_to_buf("_GREATER"); break;
	 case '?': append_to_buf("_QUESTIONM"); break;
	 case '@': append_to_buf("_ATSIGN"); break;
	 case '[': append_to_buf("_LBRACKET"); break;
	 case '\\': append_to_buf("_BACKSLASH"); break;
	 case ']': append_to_buf("_RBRACKET"); break;
	 case '^': append_to_buf("_POWER"); break;
	 case '_': append_to_buf("_UNDERSCORE"); break;
	 case '`': append_to_buf("_BACKQUOTE"); break;
	 case '{': append_to_buf("_LBRACE"); break;
	 case '|': append_to_buf("_BAR"); break;
	 case '}': append_to_buf("_RBRACE"); break;
	 case '~': append_to_buf("_TILDE"); break;
	 default:
	    {
	       char rep[5];
	       sprintf(rep, "_%d", *p);
	       append_to_buf(rep);
	    }
	 }
	 p++;
      }
   }
   string_to_id(start_namebuf, ref_id);
}
/*----------------------------------------------------------------------------*/
