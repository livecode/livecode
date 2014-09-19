#include <stdio.h>

/* ( 1) %{ */
/* ( 2) YYSTYPE block */
/* ( 3) SETPOS block */
/* ( 4) LITBLOCK block */
/* ( 5) %} */
/* ( 6) LEXDEF block */
/* ( 7) %% */
/* ( 8) gen.lit */
/* ( 9) <token>.t for each <token> in gen.tkn */
/* (10) COMMENTS block */
/* (11) LAYOUT block */
/* (12) ILLEGAL block */
/* (13) %% */
/* (14) LEXFUNC block */
/* (15) YYWRAP block */


/* ( 1) %{ */
char *leftpar[] = {
   "%{",
   ""
};
/* ( 2) YYSTYPE block */
char *yystype[] = {
   "#include \"gen.h\"",
   "extern YYSTYPE yylval;",
   ""
};
/* ( 3) SETPOS block */
char *setpos[] = {
   "extern long yypos;",
   "#define yysetpos() { yylval.attr[0] = yypos; yypos += yyleng; }",
   ""
};
/* ( 4) LITBLOCK block */
char *litblock[] = {
   ""
};
/* ( 5) %} */
char *rightpar[] = {
   "%}",
   ""
};
/* ( 6) LEXDEF block */
char *lexdef[] = {
   ""
};
/* ( 7) %% */
char *separator[] = {
   "%%",
   ""
};
/* ( 8) gen.lit */
/* ( 9) <token>.t for each <token> in gen.tkn */
/* (10) COMMENTS block */
char *comments[] = {
   ""
};
/* (11) LAYOUT block */
char *layout[] = {
   "\\  { yypos += 1; }",
   "\\t { yypos += 1; }",
   "\\r { yypos += 1; }",
   "\\n { yyPosToNextLine(); }",
   ""
};
/* (12) ILLEGAL block */
char *illegal[] = {
   ". { yysetpos(); yyerror(\"illegal token\"); }",
   ""
};
/* (14) LEXFUNC block */
char *lexfunc[] = {
   ""
};
/* (15) YYWRAP block */
char *yywrap[] = {
   "#ifndef yywrap",
   "yywrap() { return 1; }",
   "#endif",
   ""
};


FILE *OUTFILE;

struct info {
   char *name;
   char *replacement;
   int used;
   struct info *next;
};

struct info *info_list = 0;

/*----------------------------------------------------------------------------*/

FILE * OPEN(name)
   char *name;
{
   FILE *F;
   struct info *cur;

   for (cur = info_list; cur; cur = cur->next) {
      if (strcmp(name, cur->name) == 0) {
         F = fopen(cur->replacement, "r");
	 if (F == NULL) err("cannot open %s\n", cur->replacement);
	 cur-> used = 1;
	 return F;
      }
   }
   F = fopen(name, "r");
   return F;
}

/*----------------------------------------------------------------------------*/

err(fmt, a)
   char *fmt;
   char *a;
{
   printf(fmt, a);
   exit(1);
}

/*----------------------------------------------------------------------------*/

args(argc, argv)
   int argc;
   char **argv;
{
   int i;

   struct info *new;

   for (i = 1; i < argc; i++) {
      int len, j, eq, dot;
      len = strlen(argv[i]);

      eq = -1;
      for (j = 0; j < len; j++) {
	 if (argv[i][j] == '=') {
	    eq = j;
	    break;
	 }
      }
      if (eq == -1) err("missing '=' in argument\n","");

      dot = -1;
      for (j = len-1; j > eq; j--) {
	 if (argv[i][j] == '.') {
	    dot = j;
	    break;
	 }
      }
      if (dot == -1) err("missing '.' in filename\n","");

      new = (struct info *) malloc(sizeof (struct info));
      new->next = info_list;
      new->used = 0;
      info_list = new;

      new->name = (char *) malloc(eq+2);
      strncpy(new->name, argv[i], eq);
      new->name[eq] = '.';
      new->name[eq+1] = argv[i][len-1];
      new->name[eq+2] = '\0';

      new->replacement = (char *) malloc(len-eq-1);
      strncpy(new->replacement, argv[i]+eq+1, len-eq-1);
      new->replacement[len-eq-1] = '\0';
   }
}

/*----------------------------------------------------------------------------*/

main(argc, argv)
   int argc;
   char **argv;
{
   args(argc, argv);

   OUTFILE = fopen("gen.l", "w");
   if (OUTFILE == NULL) {
      printf("cannot open gen.l\n");
      exit(1);
   }

   /* ( 1) %{ */
   emit(leftpar);
   /* ( 2) YYSTYPE block */
   copy_or_text("YYSTYPE.b", yystype);
   /* ( 3) SETPOS block */
   copy_or_text("SETPOS.b", setpos);
   /* ( 4) LITBLOCK block */
   copy_or_text("LITBLOCK.b", litblock);
   /* ( 5) %} */
   emit(rightpar);
   /* ( 6) LEXDEF block */
   copy_or_text("LEXDEF.b", lexdef);
   /* ( 7) %% */
   emit(separator);
   /* ( 8) gen.lit */
   {
      FILE *F;

      F = OPEN("gen.lit");
      if (F == NULL) {
	 printf("cannot open gen.lit\n");
	 exit(1);
      }
      copy(F);
   }
   /* ( 9) <token>.t for each <token> in gen.tkn */
   filelist();
   /* (10) COMMENTS block */
   copy_or_text("COMMENTS.b", comments);
   /* (11) LAYOUT block */
   copy_or_text("LAYOUT.b", layout);
   /* (12) ILLEGAL block */
   copy_or_text("ILLEGAL.b", illegal);
   /* (13) %% */
   emit(separator);
   /* (14) LEXFUNC block */
   copy_or_text("LEXFUNC.b", lexfunc);
   /* (15) YYWRAP block */
   copy_or_text("YYWRAP.b", yywrap);

   fclose(OUTFILE);

   {
      struct info *cur;

      for (cur = info_list; cur; cur = cur->next) {
	 cur->name[strlen(cur->name)-2] = '\0';
	 if (! cur->used) err("invalid argument %s\n", cur->name);
      }

   }
   return 0;
}

/*----------------------------------------------------------------------------*/

emit(text)
   char *text[];
{
   int i;

   for (i = 0; text[i][0]; i++)
      fprintf(OUTFILE, "%s\n", text[i]);
}

/*----------------------------------------------------------------------------*/

copy(INFILE)
   FILE *INFILE;
{
   char prev = ' ' ;
   for(;;) {
      int ch = fgetc(INFILE);
      if (ch == EOF) break;
      prev = ch;
      fputc(ch, OUTFILE);
   }
   if (prev != '\n')
      fputc('\n', OUTFILE);
}

/*----------------------------------------------------------------------------*/

copy_or_text(filename, text)
   char *filename;
   char *text[];
{
   FILE *INFILE;

   INFILE = OPEN(filename);
   if (INFILE == NULL)
      emit(text);
   else {
      copy(INFILE);
      fclose(INFILE);
   }
}

/*----------------------------------------------------------------------------*/

filelist()
{
   FILE *LISTFILE;
   FILE *INFILE;

   LISTFILE = OPEN("gen.tkn");
   if (LISTFILE == NULL) {
      printf("cannot open gen.tkn\n");
      exit(1);
   }

   for(;;) {
      char name[500];
      char *p;
      int ch;

      ch = fgetc(LISTFILE);
      while (ch == '\n' || ch == ' ' )
	 ch = fgetc(LISTFILE);
      if (ch == EOF) break;

      p = &name[0];
      while ( ('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z') ||
	 ('0' <= ch && ch <= '9') || ch == '_' )
      {
	 *p++ = ch;
	 ch = fgetc(LISTFILE);
      }
      *p++ ='.';
      *p++ ='t';
      *p = '\0';

      INFILE = OPEN(name);
      if (INFILE == NULL) {
	 printf("cannot open %s\n", name);
	 exit(1);
      }
      else {
	 copy(INFILE);
	 fclose(INFILE);
      }
   }
   fclose(LISTFILE);
}

/*----------------------------------------------------------------------------*/
