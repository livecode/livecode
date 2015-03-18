/* --PATCH-- */ #include <stdlib.h>
/* --PATCH-- */ #include <stdio.h>
/* --PATCH-- */ #include <string.h>
/*
   GENTLE 97 CAMPUS EDITION

   COPYRIGHT (C) 1992, 1997. All rights reserved.

   Metarga GmbH, Joachim-Friedrich-Str. 54, D-10711 Berlin

   gentle-97-v-4-1-0
*/


int ErrorOccured = 0;

static scanargs();

main (argc, argv)
   int argc;
   char ** argv;
{
   
   scanargs (argc, argv);

   init_scanner();
   init_idtab();
   
   ROOT ();
   
   /* --PATCH-- */ exit(ErrorOccured);
}

/*----------------------------------------------------------------------------*/

static int TraceFlag = 0;

int TraceOption ()
{
   return TraceFlag;
}

static int SymbolFileFlag = 0;

int SymbolFileOption ()
{
   return SymbolFileFlag;
}

/* --BEGIN-PATCH-- */
struct FileMapping
{
    struct FileMapping *next;
    char *name;
    char *replacement;
    int used;
};

static struct FileMapping *FileMappings;

void DefFileMapping(const char *p_map_str)
{
    struct FileMapping *t_mapping;
    t_mapping = (struct FileMapping *)malloc(sizeof(struct FileMapping));
    t_mapping -> next = FileMappings;
    t_mapping -> name = strndup(p_map_str, strchr(p_map_str, '=') - p_map_str);
    t_mapping -> replacement = strdup(strchr(p_map_str, '=') + 1);
    t_mapping -> used = 0;
    FileMappings = t_mapping;
}

const char *MapFile(const char *p_input)
{
    struct FileMapping *t_mapping;
    for(t_mapping = FileMappings; t_mapping != NULL; t_mapping = t_mapping -> next)
        if (strcmp(t_mapping -> name, p_input) == 0)
            return t_mapping -> replacement;
    
    return p_input;
}

/* --END-PATCH-- */

/*----------------------------------------------------------------------------*/

static scanargs (argc, argv)
int argc;
char ** argv;
{
    int i;
    int source_defined = 0;
    
    i = 1;
    while (i < argc) {
        /* -- PATCH -- */ if (strcmp (argv[i], "-subdir") == 0) { if (argv[i+1] == NULL) { printf("Invalid option: parameter expected for -subdir"); exit(1); } SetOption_SUBDIR(argv[++i]); }
        else if (strcmp (argv[i], "-alert") == 0) SetOption_ALERT();
        else if (strcmp (argv[i], "-if") == 0) SymbolFileFlag = 1;
        else if (strcmp (argv[i], "-trace") == 0) TraceFlag = 1;
        else {
            int len;
            
            len = strlen(argv[i]);
            
            if (len > 0 && argv[i][0] == '-') {
                printf ("Invalid option: %s\n", argv[i]);
                exit(1);
            }
            
            /* --BEGIN-PATCH-- */
            if (strchr(argv[i], '=') != NULL)
            {
                DefFileMapping(argv[i]);
                goto continue_args;
            }
            /* --END-PATCH-- */
            
            if (len <= 2 || argv[i][len-2] != '.' || argv[i][len-1] != 'g') {
                printf ("Invalid filename: %s\n", argv[i]);
                exit(1);
            }
            
            DefSourceName (argv[i]);
            source_defined = 1;
        }
        
    continue_args:
        i++;
    }
    if (! source_defined) {
        printf("Missing file name\n");
        exit(1);
    }
}

/*----------------------------------------------------------------------------*/
