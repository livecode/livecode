/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "literal.h"
#include "position.h"
#include "literal.h"
#include "report.h"

extern void ROOT(void);
extern void yyExtend(void);
extern void InitializeCustomInvokeLists(void);

static int s_is_bootstrap = 0;

int IsBootstrapCompile(void)
{
    return s_is_bootstrap;
}

void bootstrap_main(int argc, char *argv[])
{
    int i;
	
	// If there is no filename, error.
    if (argc == 0)
    {
        fprintf(stderr, "Invalid arguments\n");
        return;
    }
    
    s_is_bootstrap = 1;
    
    for(i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--template") == 0 && i + 1 < argc)
           SetTemplateFile(argv[++i]);
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            SetOutputFile(argv[++i]);
        else if (strcmp(argv[i], "--modulepath") == 0 && i + 1 < argc)
            AddImportedModuleDir(argv[++i]);
        else
            AddFile(argv[i]);
    }
    
    if (MoveToNextFile())
    {
        yyExtend();
        InitializeCustomInvokeLists();
        ROOT();
    }
    
    /*
    // Get the filename.
    const char *t_in_file;
    t_in_file = argv[0];
    
    // Compute the output filename.
    const char *t_in_leaf;
    t_in_leaf = strrchr(t_in_file, '/');
    if (t_in_leaf == NULL)
        t_in_leaf = t_in_file;
    
    char t_out_file[256];
    sprintf(t_out_file, "_G_/%s.g", t_in_leaf);
    extern FILE *yyout;
    yyout = fopen(t_out_file, "w");
    if (yyout == NULL)
    {
        fprintf(stderr, "Could not open output file '%s'\n", t_out_file);
        return 1;
    }
    
    extern FILE *yyin;
    yyin = fopen(t_in_file, "r");
    if (yyin == NULL)
    {
        fprintf(stderr, "Could not open file '%s'\n", argv[0]);
        return 1;
    }
    
    Run();
    
    return 0;*/
}

extern int OutputFileAsC;

/* Print some sort of helpful message if the user doesn't pass sane arguments */
static void
usage(int status)
{
    fprintf(stderr,
"Usage: lc-compile [OPTION ...] -o OUTFILE [--] LCBFILE\n"
"       lc-compile [OPTION ...] --outputc OUTFILE [--] LCBFILE\n"
"\n"
"Compile a LiveCode Builder source file.\n"
"\n"
"Options:\n"
"  -I, --modulepath PATH    Search PATH for module interface files.\n"
"  -o, --output OUTFILE     Filename for bytecode output.\n"
"      --outputc OUTFILE    Filename for C source code output.\n"
"      --manifest MANIFEST  Filename for generated manifest.\n"
"  -h, --help               Print this message.\n"
"  --                       Treat all remaining arguments as filenames.\n"
"\n"
"More than one `--modulepath' option may be specified.  The PATHs are\n"
"searched in the order they appear.  An interface file may be generated in\n"
"the first PATH specified.\n"
"\n"
"Report bugs to <http://quality.runrev.com/>\n"
            );
    exit (status);
}

static void full_main(int argc, char *argv[])
{
    /* Process options. */
    int have_input_file = 0;
    int end_of_args = 0;
    int argi;

    /* FIXME maybe we should use getopt? */
    for (argi = 0; argi < argc; ++argi)
    {
        const char *opt = argv[argi];
        const char *optarg = (argi + 1 < argc) ? argv[argi+1] : NULL;
        if (!end_of_args)
        {
	        if ((0 == strcmp(opt, "--modulepath") || 0 == strcmp(opt, "-I")) &&
	            optarg)
            {
                AddImportedModuleDir(argv[++argi]);
                continue;
            }
            if ((0 == strcmp(opt, "--output") || 0 == strcmp(opt, "-o")) &&
                optarg)
            {
                SetOutputFile(argv[++argi]);
                continue;
            }
            if (0 == strcmp(opt, "--outputc") && optarg)
            {
                SetOutputFile(argv[++argi]);
                OutputFileAsC = 1;
                continue;
            }
            if (0 == strcmp(opt, "--manifest") && optarg)
            {
                SetManifestOutputFile(argv[++argi]);
                continue;
            }
            if (0 == strcmp(opt, "-h") || 0 == strcmp(opt, "--help"))
            {
                usage(0);
                continue;
            }
            if (0 == strcmp(opt, "--"))
            {
                end_of_args = 1;
                continue;
            }

            /* Detect unrecognized arguments */
            if ('-' == opt[0])
            {
                fprintf(stderr, "ERROR: Invalid option '%s'.\n\n",
                        argv[argi]);
                usage(1);
            }
        }

        /* Accept only one input file */
        if (!have_input_file)
        {
            AddFile(opt);
            have_input_file = 1;
            continue;
        }
        else
        {
            fprintf(stderr, "WARNING: Ignoring multiple input filenames.\n");
            continue;
        }

        break; /* Doesn't match any option */
    }

	// If there is no filename, error.
    if (!have_input_file)
    {
        fprintf(stderr, "ERROR: You must specify an input filename.\n\n");
        usage(1);
    }

    if (MoveToNextFile())
    {
        yyExtend();
        InitializeCustomInvokeLists();
        ROOT();
    }
}

// No built-in modules for the compiler
void* g_builtin_modules[1] = {NULL};
unsigned int g_builtin_module_count = 0;

extern int yydebug;

int main(int argc, char *argv[])
{
    //extern int yydebug;
	int t_return_code;
	
	// Skip command arg.
    argc -= 1;
    argv += 1;
    
    // Check for debug mode.
    if (argc > 1 && strcmp(argv[0], "--debug") == 0)
    {
        argc -= 1;
        argv += 1;
        
#ifdef YYDEBUG
        yydebug = 1;
#endif
    }
    
    InitializeFiles();
    InitializePosition();
    InitializeLiterals();
    InitializeReports();
    InitializeScopes();
    
    // Check for bootstrap mode.
    if (argc > 1 && strcmp(argv[0], "--bootstrap") == 0)
        bootstrap_main(argc - 1, argv + 1);
    else
        full_main(argc, argv);

    if (ErrorsDidOccur())
        t_return_code = 1;
    else
        t_return_code = 0;
    
    FinalizeScopes();
    FinalizeReports();
    FinalizeLiterals();
    FinalizePosition();
    FinalizeFiles();
    
    return t_return_code;
    
    /*extern FILE *yyin;
    yyin = fopen(argv[0], "r");
    if (yyin == NULL)
    {
        fprintf(stderr, "Could not open file '%s'\n", argv[0]);
        return 1;
    }
    
    Run();
    
    return 0;*/
}
