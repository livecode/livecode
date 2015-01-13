#define PRIVATE static

/*--------------------------------------------------------------------*/

#define HashTabSize       2048
#define STRINGTAB_PIECE  10000
#define STRINGTAB_EXTRA    500

typedef struct IDENTSTRUCT *IDENT;

struct IDENTSTRUCT
{
   char  *firstposptr;
   long  length;
   IDENT next;
   long  meaning;
};

PRIVATE char *idstringtab_ptr;
PRIVATE char *idstringtab_endptr;

struct IDENTSTRUCT *idtab_ptr;
struct IDENTSTRUCT *idtab_endptr;

PRIVATE IDENT HashTab [HashTabSize];

PRIVATE int initialized = 0;

/*--------------------------------------------------------------------*/

PRIVATE allocate_idstringtab ()
{
   idstringtab_ptr =
      (char *) malloc (STRINGTAB_PIECE + STRINGTAB_EXTRA);
   if (idstringtab_ptr == 0) {
      printf("memory allocation failed\n");
      exit(1);
   }
   idstringtab_endptr = idstringtab_ptr + STRINGTAB_PIECE - 1;
}

/*--------------------------------------------------------------------*/

#define IDTABPIECESIZE 500
typedef struct IDENTSTRUCT IDTAB [IDTABPIECESIZE];

PRIVATE allocate_idtab ()
{
   idtab_ptr =
      (struct IDENTSTRUCT *)
      malloc (sizeof (IDTAB /*struct IDENTSTRUCT [IDTABPIECESIZE]*/ ) );
   if (idtab_ptr == 0) {
      printf("memory allocation failed\n");
      exit(1);
   }
   idtab_endptr = & idtab_ptr[IDTABPIECESIZE - 1];
}

/*--------------------------------------------------------------------*/

PRIVATE InitIdents ()
{
   long i;

   for (i = 0; i<=HashTabSize-1; i++) HashTab[i] = 0;

   allocate_idtab ();
   allocate_idstringtab ();

   initialized = 1;
}

/*--------------------------------------------------------------------*/

slice_to_id (idstart, idstop, ref_id)
   char *idstart; /* position of first character */
   char *idstop;  /* position  a f t e r  last character */
   IDENT *ref_id;

{
   long  hash, length;
   IDENT chain;
   IDENT  NewId;

   if (! initialized) InitIdents();

   length = idstop-idstart;
   hash = ( length*256 + ((*idstart)&0xf)*16 + (*(idstop-1)&0xf) ) 
   & (HashTabSize-1);
   chain = HashTab[hash];

   for(;;) {
      if (chain == 0) {
      
	 /* not in table */
	 
	 register char *i, *freeptr, *stop;

	 NewId = idtab_ptr;
	    
	 if (idtab_ptr == idtab_endptr)
	    allocate_idtab();
         else
	    idtab_ptr++;

	 /* copy id into string table */
	 i = idstart;
	 if (idstringtab_ptr > idstringtab_endptr)
	    allocate_idstringtab();
	 freeptr = idstringtab_ptr;
	 stop = idstop;
	 while (i < stop) {
	    *freeptr++ = *i++;
	 }
	 *freeptr = '\0';
	 freeptr++;
	    
	 NewId->firstposptr = idstringtab_ptr;
	 NewId->length = length;
	 NewId->next = HashTab[hash];
         NewId->meaning = 0;
	    
	 HashTab[hash] = NewId;

	 idstringtab_ptr= freeptr;
	 

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

/*--------------------------------------------------------------------*/
void string_to_id (string, ref_id)
   char *string;
   IDENT *ref_id;
{
   char *idstop;

   idstop = string;
   while (*idstop != '\0') idstop++;
   slice_to_id (string, idstop, ref_id);
}

/*--------------------------------------------------------------------*/
void id_to_string (id, ref_string)
   IDENT id;
   char **ref_string;
{
   *ref_string = id->firstposptr;
}

/*--------------------------------------------------------------------*/
void DefMeaning (id, m)
   IDENT id;
   long m;
{
   id->meaning = m;
}

/*--------------------------------------------------------------------*/
void UndefMeaning (id)
   IDENT id;
{
   id->meaning = 0;
}

/*--------------------------------------------------------------------*/
int HasMeaning (id, ref_meaning)
   IDENT id;
   long *ref_meaning;
{
   if (id->meaning == 0)
      return 0;
   *ref_meaning = id->meaning;
   return 1;
}

/*--------------------------------------------------------------------*/
ErrorI (str1, id, str2, pos)
   char *str1;
   IDENT id;
   char *str2;
   long pos;
{
   char *idrepr;
   char buf[300];

   id_to_string (id, &idrepr);
   sprintf(buf, "%s%s%s", str1, idrepr, str2);
   Error(buf, pos);
}

/*--------------------------------------------------------------------*/
