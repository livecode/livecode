#include <stdio.h>

extern FILE *yyin;

main (argc, argv)
   int argc;
   char **argv;

{

   /* INITIALIZE */

   if (argc > 2) {
      printf("too many arguments\n");
      exit(1);
   }

   if (argc == 2) {
      yyin = fopen (argv[1], "r");
      if (yyin == NULL) {
	 printf("cannot open '%s'\n", argv[1]);
	 exit(1);
      }
   }


   /* INVOKE GENERATED PROGRAM */

   ROOT();

   

   /* FINALIZE */

   exit(0);
}
