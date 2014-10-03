#define PRIVATE static

/*--------------------------------------------------------------------*/

PRIVATE int initialized = 0;

PRIVATE char *p;     /* points to first free position of current buffer */
PRIVATE char *first; /* points to first char of current string */
PRIVATE char *start; /* points to first position of current buffer */
PRIVATE char *stop;  /* points to position after last char of current buffer */

PRIVATE long SIZE = 500;

#define INCR 500

/*--------------------------------------------------------------------*/

PRIVATE Allocate ()
{
   p = (char *) malloc (SIZE);
   if (p == 0) {
      printf("memory allocation failed\n");
      exit(1);
   }
   first = p;
   start = p;
   stop = p+SIZE;
}

/*--------------------------------------------------------------------*/

PRIVATE Initialize()
{
   Allocate();
   initialized = 1;
}

/*--------------------------------------------------------------------*/
AppendToString (ch)
   char ch;
{
   if ( ! initialized) Initialize();

   if (p == stop) { /* buffer full */
      char *q;
      char *qfirst = first;
      char *qstop = stop;

      if (first == start) SIZE += INCR;

      Allocate();
      for (q = qfirst; q != qstop; q++) {
	 *p++ = *q;
      }
   }
   *p++ = ch;
}

/*--------------------------------------------------------------------*/
GetStringRef(ref_string)
   char **ref_string;
{
   if ( ! initialized) Initialize();
   AppendToString(0);
   *ref_string = first;
   first = p;
}

/*--------------------------------------------------------------------*/

