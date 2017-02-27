/* Copyright (C) 2016 LiveCode Ltd.
 
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

#include <position.h>
#include "literal.h"
#include "report.h"

extern void ROOT(void);
extern void yyExtend(void);
extern void InitializeCustomInvokeLists(void);

int IsDependencyCompile(void)
{
    return 0;
}

/* Print some sort of helpful message if the user doesn't pass sane arguments */
static void
usage(int status)
{
    fprintf(stderr,
"Usage: lc-compile-ffi-java [OPTION ...] --check OUTFILE --output OUTFILE [--] SOURCEFILES\n"
"\n"
"Parse a Java FFI DSL source file.\n"
"\n"
"Options:\n"
"      --check OUTFILE        Filename for reconstructed output.\n"
"      --output OUTFILE       Filename for generated LCB output.\n"
"      --modulename NAME      Name for output LCB module.\n"
"      -Werror                Turn all warnings into errors.\n"
"  -v, --verbose              Output extra debugging information.\n"
"  -h, --help                 Print this message.\n"
"  --                         Treat all remaining arguments as filenames.\n"
"\n"
"Report bugs to <http://quality.livecode.com/>\n"
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
        const char *optarg = (argi + 1 < argc) && 0 != strncmp(argv[argi+1], "--", 2) ? argv[argi+1] : NULL;
        if (!end_of_args)
        {
            if (0 == strcmp(opt, "--check") && optarg)
            {
                SetOutputFile(argv[++argi]);
                continue;
            }
            if (0 == strcmp(opt, "--output") && optarg)
            {
                SetOutputLCBFile(argv[++argi]);
                continue;
            }
            if (0 == strcmp(opt, "--modulename") && optarg)
            {
                SetOutputLCBModuleName(argv[++argi]);
                continue;
            }
            /* FIXME This should be expanded to support "-W error",
             * "--warn error", "--warn=error", etc.  Also options for
             * enabling/disabling/errorifying particular warning
             * types. */
            if (0 == strcmp(opt, "-Werror"))
            {
                s_is_werror_enabled = 1;
                continue;
            }
            if (0 == strcmp(opt, "-v") || 0 == strcmp(opt, "--verbose"))
            {
                ++s_verbose_level;
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

        AddFile(opt);
        have_input_file = 1;
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
