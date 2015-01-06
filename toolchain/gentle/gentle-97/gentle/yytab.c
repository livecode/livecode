/*
   GENTLE 97 CAMPUS EDITION

   COPYRIGHT (C) 1992, 1997. All rights reserved.

   Metarga GmbH, Joachim-Friedrich-Str. 54, D-10711 Berlin

   gentle-97-v-4-1-0
*/


/* A Bison parser, made by GNU Bison 1.875b.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     MODULE = 257,
     EXPORTTOKEN = 258,
     IMPORTTOKEN = 259,
     USE = 260,
     END = 261,
     VAR = 262,
     TYPETOKEN = 263,
     PROC = 264,
     COND = 265,
     NONTERMTOKEN = 266,
     TOKENTOKEN = 267,
     CHOICE = 268,
     CLASSTOKEN = 269,
     SWEEP = 270,
     ROOT = 271,
     RULETOKEN = 272,
     TABLE = 273,
     KEY = 274,
     EQ = 275,
     TIMES = 276,
     DIV = 277,
     PLUS = 278,
     MINUS = 279,
     UNDERSCORE = 280,
     QUOTE = 281,
     DOLLAR = 282,
     BEGINDISJ = 283,
     ENDDISJ = 284,
     BEGINLOOP = 285,
     ENDLOOP = 286,
     DISJDELIM = 287,
     BEGINCOND = 288,
     ENDCOND = 289,
     RIGHTARROW = 290,
     LEFTARROW = 291,
     BECOMES = 292,
     COMESBE = 293,
     SMALLBECOMES = 294,
     SMALLCOMESBE = 295,
     COLON = 296,
     LEFTPAREN = 297,
     RIGHTPAREN = 298,
     LEFTBRACKET = 299,
     RIGHTBRACKET = 300,
     COMMA = 301,
     DOT = 302,
     AMPERSAND = 303,
     EOFTOKEN = 304,
     FILESEP = 305,
     INTEGERCONST = 306,
     STRINGCONST = 307,
     SMALLID = 308,
     LARGEID = 309
   };
#endif
#define MODULE 257
#define EXPORTTOKEN 258
#define IMPORTTOKEN 259
#define USE 260
#define END 261
#define VAR 262
#define TYPETOKEN 263
#define PROC 264
#define COND 265
#define NONTERMTOKEN 266
#define TOKENTOKEN 267
#define CHOICE 268
#define CLASSTOKEN 269
#define SWEEP 270
#define ROOT 271
#define RULETOKEN 272
#define TABLE 273
#define KEY 274
#define EQ 275
#define TIMES 276
#define DIV 277
#define PLUS 278
#define MINUS 279
#define UNDERSCORE 280
#define QUOTE 281
#define DOLLAR 282
#define BEGINDISJ 283
#define ENDDISJ 284
#define BEGINLOOP 285
#define ENDLOOP 286
#define DISJDELIM 287
#define BEGINCOND 288
#define ENDCOND 289
#define RIGHTARROW 290
#define LEFTARROW 291
#define BECOMES 292
#define COMESBE 293
#define SMALLBECOMES 294
#define SMALLCOMESBE 295
#define COLON 296
#define LEFTPAREN 297
#define RIGHTPAREN 298
#define LEFTBRACKET 299
#define RIGHTBRACKET 300
#define COMMA 301
#define DOT 302
#define AMPERSAND 303
#define EOFTOKEN 304
#define FILESEP 305
#define INTEGERCONST 306
#define STRINGCONST 307
#define SMALLID 308
#define LARGEID 309




/* Copy the first part of user declarations.  */
#line 1 "gen.y"

typedef long * yy;
#define yyu (-2147483647L)
static yy yynull;
extern yy yyh;
extern yy yyhx;
static yyErr(n,l)
{
yyAbort(n,"cyfront", l);
}
/* start */
/* end */
extern yy yyglov_currentGroupIdent;
extern yy yyglov_containsGrammar;
extern yy yyglov_containsRoot;
extern yy yyglov_maxAttr;
extern yy yyglov_tokenList;
extern yy yyglov_errorId;
extern yy yyglov_id_INT;
extern yy yyglov_id_STRING;
extern yy yyglov_id_POS;
extern yy yyglov_importedDeclarations;
extern yy yyglov_declaredVars;
extern yy yyglov_spaceRequired;
extern yy yyglov_currentTokenCode;
extern yy yyglov_currentProcNumber;
extern yy yyglov_currentRuleNumber;
extern yy yyglov_localVars;
extern yy yyglov_env;
extern yy yyglov_currentFailLabel;
extern yy yyglov_currentFailLabelUsed;
extern yy yyglov_currentSuccessLabel;
extern yy yyglov_choice_Declarations;
extern yy yyglov_choice_Types;
extern yy yyglov_insideChoiceRule;
extern yy yyglov_current_choice_type;
extern yy yyglov_tryLabel;
extern yy yyglov_pathes;
typedef struct {long attr[2];} yyATTRIBUTES;
#define YYSTYPE yyATTRIBUTES
extern YYSTYPE yylval;



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 241 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   311

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  56
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  67
/* YYNRULES -- Number of rules. */
#define YYNRULES  169
/* YYNRULES -- Number of states. */
#define YYNSTATES  304

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   310

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
       2
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     8,     9,    15,    19,    24,    31,    33,
      34,    36,    37,    40,    41,    44,    47,    49,    51,    52,
      56,    59,    62,    64,    66,    67,    71,    75,    79,    80,
      83,    84,    86,    88,    90,    92,    94,    98,   104,   110,
     112,   113,   117,   118,   122,   123,   125,   126,   131,   133,
     138,   140,   144,   145,   147,   149,   153,   158,   162,   163,
     165,   167,   171,   175,   176,   178,   180,   184,   190,   197,
     204,   208,   210,   214,   218,   220,   223,   229,   234,   236,
     238,   239,   244,   245,   247,   249,   251,   253,   255,   258,
     259,   261,   263,   264,   267,   269,   272,   274,   280,   287,
     290,   291,   293,   294,   296,   302,   305,   306,   308,   311,
     312,   316,   320,   324,   328,   334,   341,   347,   354,   358,
     362,   368,   375,   381,   388,   393,   395,   401,   405,   409,
     413,   415,   421,   424,   425,   428,   429,   433,   434,   437,
     438,   442,   443,   447,   449,   453,   460,   462,   467,   469,
     473,   475,   479,   483,   485,   489,   493,   495,   497,   501,
     508,   510,   515,   517,   519,   521,   524,   527,   531,   533
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      57,     0,    -1,    58,    59,    64,    50,    -1,    -1,     3,
      61,    65,    71,    63,    -1,    62,    71,    63,    -1,    51,
      62,    71,    63,    -1,    51,     3,    61,    67,    71,    63,
      -1,   122,    -1,    -1,     7,    -1,    -1,    60,    64,    -1,
      -1,    69,    66,    -1,    66,    69,    -1,    66,    -1,    69,
      -1,    -1,     6,    70,    95,    -1,    69,    68,    -1,    68,
      69,    -1,    68,    -1,    69,    -1,    -1,     6,    70,    95,
      -1,     4,    70,    95,    -1,   122,    78,    70,    -1,    -1,
      72,    71,    -1,    -1,    73,    -1,    74,    -1,    93,    -1,
      87,    -1,    88,    -1,    17,   108,    95,    -1,     9,   122,
      75,    76,    95,    -1,    15,   122,    75,    77,    95,    -1,
      21,    -1,    -1,    79,    78,    76,    -1,    -1,    80,    78,
      77,    -1,    -1,    47,    -1,    -1,    54,    43,    81,    44,
      -1,    54,    -1,    54,    43,    83,    44,    -1,    54,    -1,
      82,    47,    81,    -1,    -1,    82,    -1,   122,    -1,   122,
      42,   122,    -1,     8,   122,    42,   122,    -1,    84,    47,
      83,    -1,    -1,    84,    -1,   122,    -1,   122,    42,   122,
      -1,    86,    47,    85,    -1,    -1,    86,    -1,   122,    -1,
     122,    42,   122,    -1,     8,   122,    42,   122,    95,    -1,
      19,   122,    43,    89,    44,    95,    -1,    19,    91,    45,
     122,    46,    95,    -1,    90,    47,    89,    -1,    90,    -1,
     122,    42,   122,    -1,    92,    47,    91,    -1,    92,    -1,
     122,   122,    -1,    97,    94,    96,    95,    99,    -1,    13,
      94,    96,    95,    -1,   122,    -1,    48,    -1,    -1,    43,
      85,    98,    44,    -1,    -1,    10,    -1,    11,    -1,    12,
      -1,    14,    -1,    16,    -1,    36,    85,    -1,    -1,   100,
      -1,   101,    -1,    -1,   102,   100,    -1,   102,    -1,   103,
     101,    -1,   103,    -1,   106,    42,   108,   104,    48,    -1,
      18,   106,   105,   108,   104,    95,    -1,    28,    52,    -1,
      -1,    42,    -1,    -1,   122,    -1,   122,    43,   114,   107,
      44,    -1,    36,   112,    -1,    -1,   109,    -1,   110,   109,
      -1,    -1,    55,    40,   118,    -1,    55,    41,   117,    -1,
     122,    38,   118,    -1,   122,    39,   117,    -1,   122,    27,
     122,    38,   118,    -1,   122,    45,   122,    46,    38,   118,
      -1,   122,    27,   122,    39,   117,    -1,   122,    45,   122,
      46,    39,   117,    -1,   122,    37,   118,    -1,   122,    36,
     117,    -1,   122,    27,   122,    37,   118,    -1,   122,    45,
     122,    46,    37,   118,    -1,   122,    27,   122,    36,   117,
      -1,   122,    45,   122,    46,    36,   117,    -1,   122,    42,
      42,   122,    -1,   122,    -1,   122,    43,   112,   111,    44,
      -1,    29,   116,    30,    -1,    34,   108,    35,    -1,    31,
     108,    32,    -1,    53,    -1,    49,    43,    36,   117,    44,
      -1,    36,   114,    -1,    -1,   118,   113,    -1,    -1,    47,
     118,   113,    -1,    -1,   117,   115,    -1,    -1,    47,   117,
     115,    -1,    -1,   108,    33,   116,    -1,   108,    -1,   122,
      27,    54,    -1,   122,    27,    54,    43,   114,    44,    -1,
      54,    -1,    54,    43,   114,    44,    -1,    55,    -1,    55,
      42,   117,    -1,    26,    -1,   118,    24,   119,    -1,   118,
      25,   119,    -1,   119,    -1,   119,    22,   120,    -1,   119,
      23,   120,    -1,   120,    -1,   121,    -1,   122,    27,    54,
      -1,   122,    27,    54,    43,   112,    44,    -1,    54,    -1,
      54,    43,   112,    44,    -1,    55,    -1,    52,    -1,    53,
      -1,    25,   121,    -1,    24,   121,    -1,    43,   118,    44,
      -1,    54,    -1,    55,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   100,   100,   125,   137,   183,   231,   280,   327,   343,
     362,   371,   377,   407,   419,   448,   477,   502,   531,   554,
     571,   600,   629,   654,   683,   706,   720,   737,   768,   780,
     810,   822,   837,   852,   867,   882,   897,   927,   974,  1021,
    1030,  1036,  1067,  1079,  1110,  1122,  1131,  1137,  1175,  1207,
    1245,  1277,  1315,  1327,  1359,  1388,  1426,  1465,  1503,  1515,
    1547,  1576,  1614,  1652,  1664,  1696,  1725,  1763,  1813,  1857,
    1901,  1931,  1956,  1993,  2023,  2048,  2084,  2148,  2205,  2223,
    2232,  2238,  2270,  2290,  2304,  2318,  2332,  2346,  2360,  2377,
    2389,  2404,  2420,  2432,  2461,  2486,  2515,  2540,  2579,  2619,
    2636,  2645,  2654,  2660,  2702,  2748,  2765,  2784,  2812,  2842,
    2854,  2905,  2956,  2993,  3030,  3076,  3123,  3169,  3216,  3253,
    3290,  3336,  3383,  3429,  3476,  3521,  3570,  3623,  3660,  3730,
    3760,  3794,  3826,  3843,  3862,  3899,  3918,  3956,  3975,  4012,
    4031,  4069,  4088,  4125,  4157,  4208,  4262,  4304,  4349,  4384,
    4428,  4456,  4504,  4552,  4567,  4615,  4663,  4678,  4693,  4762,
    4834,  4883,  4935,  4970,  5005,  5040,  5080,  5120,  5137,  5152
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "MODULE", "EXPORTTOKEN", "IMPORTTOKEN", 
  "USE", "END", "VAR", "TYPETOKEN", "PROC", "COND", "NONTERMTOKEN", 
  "TOKENTOKEN", "CHOICE", "CLASSTOKEN", "SWEEP", "ROOT", "RULETOKEN", 
  "TABLE", "KEY", "EQ", "TIMES", "DIV", "PLUS", "MINUS", "UNDERSCORE", 
  "QUOTE", "DOLLAR", "BEGINDISJ", "ENDDISJ", "BEGINLOOP", "ENDLOOP", 
  "DISJDELIM", "BEGINCOND", "ENDCOND", "RIGHTARROW", "LEFTARROW", 
  "BECOMES", "COMESBE", "SMALLBECOMES", "SMALLCOMESBE", "COLON", 
  "LEFTPAREN", "RIGHTPAREN", "LEFTBRACKET", "RIGHTBRACKET", "COMMA", 
  "DOT", "AMPERSAND", "EOFTOKEN", "FILESEP", "INTEGERCONST", 
  "STRINGCONST", "SMALLID", "LARGEID", "$accept", "ROOT_", "Initialize", 
  "OwnUnit", "OtherUnit", "ModuleIdent", "ImplicitModuleIdent", 
  "EndOption", "OtherUnitList", "OwnInterface", "OwnImport", 
  "OtherInterface", "OtherImport", "Export", "IdentList", 
  "DeclarationList", "Declaration", "RootDeclaration", "TypeDeclaration", 
  "EqualOption", "TFunctorSpecificationList", "CFunctorSpecificationList", 
  "CommaOption", "TFunctorSpecification", "CFunctorSpecification", 
  "TArgumentSpecificationList", "TArgumentSpecification", 
  "CArgumentSpecificationList", "CArgumentSpecification", 
  "PArgumentSpecificationList", "PArgumentSpecification", 
  "VarDeclaration", "TableDeclaration", "ClassFields", "ClassField", 
  "TableFields", "TableField", "PredicateDeclaration", "GroupIdent", 
  "DotOption", "Formals", "PredicateClass", "OutArgumentSpecifications", 
  "RuleList", "RuleList1", "RuleList2", "Rule1", "Rule2", "CostOption", 
  "ColonOption", "Lhs", "LhsOutArguments", "Members", "MemberList", 
  "Member", "RhsOutArguments", "UseArgumentList", "UseArgumentListTail", 
  "DefArgumentList", "DefArgumentListTail", "AlternativeList", 
  "DefArgument", "UseArgument", "UseArgument2", "UseArgument3", 
  "UseArgument9", "Ident", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   310,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    56,    57,    58,    59,    59,    60,    60,    61,    62,
      63,    63,    64,    64,    65,    65,    65,    65,    65,    66,
      67,    67,    67,    67,    67,    68,    69,    70,    70,    71,
      71,    72,    72,    72,    72,    72,    73,    74,    74,    75,
      75,    76,    76,    77,    77,    78,    78,    79,    79,    80,
      80,    81,    81,    81,    82,    82,    82,    83,    83,    83,
      84,    84,    85,    85,    85,    86,    86,    87,    88,    88,
      89,    89,    90,    91,    91,    92,    93,    93,    94,    95,
      95,    96,    96,    97,    97,    97,    97,    97,    98,    98,
      99,    99,    99,   100,   100,   101,   101,   102,   103,   104,
     104,   105,   105,   106,   106,   107,   107,   108,   109,   109,
     110,   110,   110,   110,   110,   110,   110,   110,   110,   110,
     110,   110,   110,   110,   110,   110,   110,   110,   110,   110,
     110,   110,   111,   111,   112,   112,   113,   113,   114,   114,
     115,   115,   116,   116,   117,   117,   117,   117,   117,   117,
     117,   118,   118,   118,   119,   119,   119,   120,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   122,   122
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     4,     0,     5,     3,     4,     6,     1,     0,
       1,     0,     2,     0,     2,     2,     1,     1,     0,     3,
       2,     2,     1,     1,     0,     3,     3,     3,     0,     2,
       0,     1,     1,     1,     1,     1,     3,     5,     5,     1,
       0,     3,     0,     3,     0,     1,     0,     4,     1,     4,
       1,     3,     0,     1,     1,     3,     4,     3,     0,     1,
       1,     3,     3,     0,     1,     1,     3,     5,     6,     6,
       3,     1,     3,     3,     1,     2,     5,     4,     1,     1,
       0,     4,     0,     1,     1,     1,     1,     1,     2,     0,
       1,     1,     0,     2,     1,     2,     1,     5,     6,     2,
       0,     1,     0,     1,     5,     2,     0,     1,     2,     0,
       3,     3,     3,     3,     5,     6,     5,     6,     3,     3,
       5,     6,     5,     6,     4,     1,     5,     3,     3,     3,
       1,     5,     2,     0,     2,     0,     3,     0,     2,     0,
       3,     0,     3,     1,     3,     6,     1,     4,     1,     3,
       1,     3,     3,     1,     3,     3,     1,     1,     3,     6,
       1,     4,     1,     1,     1,     2,     2,     3,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       3,     0,     9,     1,     0,    13,    30,   168,   169,    18,
       8,     9,    13,     0,     0,     0,    83,    84,    85,     0,
      86,     0,    87,   109,     0,    11,    30,    31,    32,    34,
      35,    33,     0,    28,    28,    30,    16,    17,     0,    30,
      12,     2,     0,    40,    82,    78,    40,   109,   109,   109,
       0,   130,   169,    80,   107,   109,   125,     0,    74,     0,
      10,     5,    29,    82,    80,    46,    80,    11,    15,    14,
      24,    11,     0,    39,    42,    63,    80,    44,   143,     0,
       0,     0,     0,     0,     0,    79,    36,   108,     0,     0,
       0,     0,     0,     0,   135,     0,     0,     0,     0,    75,
      80,    26,    45,    28,    19,     4,    28,    30,    22,    23,
       6,    80,    48,    80,    46,    89,    64,    65,    77,    50,
      80,    46,   109,   127,   129,   128,     0,     0,     0,     0,
     163,   164,   160,   162,   110,   153,   156,   157,     0,   150,
     146,   148,   111,     0,     0,   119,   118,   112,   113,     0,
     133,   137,     0,     0,    73,     0,     0,    71,     0,    92,
      27,    80,    11,    21,    20,    67,    52,    37,    42,    63,
       0,    63,     0,    58,    38,    44,   142,     0,   166,   165,
       0,   135,     0,     0,     0,     0,     0,   139,     0,     0,
       0,     0,     0,     0,   124,   139,     0,     0,   134,     0,
      80,    80,     0,     0,     0,    76,    90,    91,    94,    96,
       0,   103,    25,     7,     0,     0,    53,    54,    41,    88,
      81,    62,    66,     0,    59,    60,    43,   131,   167,     0,
     151,   152,   154,   155,   158,     0,   141,   149,   144,   122,
     120,   114,   116,   132,   126,   137,     0,     0,     0,     0,
      69,    68,    70,    72,   102,    93,    95,   109,   139,     0,
      47,    52,     0,    49,    58,     0,   161,   135,   147,     0,
     138,   139,   136,   123,   121,   115,   117,   101,   109,   100,
     106,     0,    51,    55,    57,    61,     0,   141,     0,   100,
       0,     0,   135,     0,    56,   159,   140,   145,    80,    99,
      97,   105,   104,    98
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,     5,    12,     9,     6,    61,    13,    35,
      36,   107,   108,    37,    64,    25,    26,    27,    28,    74,
     113,   120,   103,   114,   121,   215,   216,   223,   224,   115,
     116,    29,    30,   156,   157,    57,    58,    31,    44,    86,
      76,    32,   170,   205,   206,   207,   208,   209,   291,   278,
     210,   293,    78,    54,    55,   196,   150,   198,   235,   270,
      79,   236,   151,   135,   136,   137,   138
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -189
static const short yypact[] =
{
    -189,    21,    30,  -189,    -6,     5,   207,  -189,  -189,    92,
    -189,    35,     5,    26,    -6,    -6,  -189,  -189,  -189,    -6,
    -189,    -6,  -189,    76,    -6,    65,   207,  -189,  -189,  -189,
    -189,  -189,    -6,    -6,    -6,   207,    83,    89,    -6,   207,
    -189,  -189,    74,   102,    78,  -189,   102,    76,    76,    76,
      96,  -189,    79,    84,  -189,    76,   111,    99,   105,    19,
    -189,  -189,  -189,    78,    84,   114,    84,    65,  -189,  -189,
     100,    65,    -6,  -189,   112,    -6,    84,   123,   145,   149,
     148,   146,   151,    11,    27,  -189,  -189,  -189,    -6,    27,
      11,    11,    27,   140,    11,    -6,    -6,    -6,    -6,  -189,
      84,  -189,  -189,    -6,  -189,  -189,    -6,   207,    83,   179,
    -189,    84,   147,    84,   114,   152,   154,   150,  -189,   159,
      84,   114,    76,  -189,  -189,  -189,    27,    11,    11,    11,
    -189,  -189,   -18,   167,   103,   120,  -189,  -189,   168,  -189,
      14,    25,  -189,   176,    98,  -189,   103,   103,  -189,    -6,
     169,    22,   160,   161,  -189,    -6,   164,   162,   170,     6,
    -189,    84,    65,  -189,  -189,  -189,     4,  -189,   112,    -6,
     181,    -6,    -6,    -6,  -189,   123,  -189,   183,  -189,  -189,
      -2,    11,    11,    11,    11,    11,   157,    27,    27,   174,
      27,    11,    11,    27,  -189,    27,   185,    11,  -189,   121,
      84,    84,    -6,    -6,    -6,  -189,  -189,  -189,    -6,   212,
     189,   190,  -189,  -189,    -6,   188,   187,   194,  -189,  -189,
    -189,  -189,  -189,   193,   191,   197,  -189,  -189,  -189,   196,
     120,   120,  -189,  -189,   198,   199,   200,  -189,   201,  -189,
     103,   103,  -189,  -189,  -189,    22,    27,    11,    11,    27,
    -189,  -189,  -189,  -189,   204,  -189,  -189,    76,    27,   206,
    -189,     4,    -6,  -189,    -6,    -6,  -189,    11,  -189,    27,
    -189,    27,  -189,  -189,   103,   103,  -189,  -189,    76,   221,
     214,    -6,  -189,  -189,  -189,  -189,   208,   200,   211,   221,
     210,   203,    11,   215,  -189,  -189,  -189,  -189,    84,  -189,
    -189,  -189,  -189,  -189
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -189,  -189,  -189,  -189,  -189,   225,   253,   -65,   254,  -189,
     231,  -189,   163,   -33,    -3,   -21,  -189,  -189,  -189,   223,
     107,    95,   -13,  -189,  -189,    10,  -189,     9,  -189,   -60,
    -189,  -189,  -189,    77,  -189,   184,  -189,  -189,   244,    13,
     217,  -189,  -189,  -189,    70,    73,  -189,  -189,    -5,  -189,
      81,  -189,   -22,   228,  -189,  -189,  -177,    41,  -188,     0,
     166,   -76,   -51,   -19,    -9,    44,    -4
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -170
static const short yytable[] =
{
      10,    53,   105,    68,   229,    62,   110,   243,   142,  -168,
      42,    43,   214,   145,    67,    45,   148,    46,    71,    56,
      59,     3,   182,   183,   204,   181,    80,    81,    45,    65,
      65,    66,   134,     4,    10,   127,   128,   109,    38,   146,
     147,  -168,   228,    56,    56,    56,   182,   183,     7,     8,
     177,    56,  -169,   139,   129,    99,    11,   187,     7,     8,
       7,     8,    98,   130,   131,   132,   133,   188,   111,   197,
     280,   117,    60,     7,     8,   163,    41,   101,   180,   104,
     143,   140,   141,   288,   144,   143,   162,    33,   143,   118,
     286,   152,   153,   155,   158,    34,    33,   213,    34,    65,
     160,   168,    65,   161,    33,    47,   106,    48,   175,   219,
      49,   221,   237,   159,   239,   301,    72,   242,    56,    83,
      84,    75,   143,    73,   165,    50,   167,   182,   183,    51,
       7,    52,    85,   174,   190,   191,   192,   193,    88,    82,
     240,   241,   184,   185,    96,   194,   245,    89,    90,    91,
      92,    99,    97,    93,    94,   211,    95,   246,   247,   248,
     249,   102,   217,   230,   231,   117,   112,   117,   222,   225,
     273,   178,   179,   276,   212,   232,   233,   119,   122,   123,
     124,   125,   149,   143,   143,   106,   143,   126,   169,   143,
     166,   143,   172,   287,  -169,   186,   274,   275,   158,   253,
     211,   171,   173,   189,   211,   195,   199,   200,   201,   202,
     259,   234,   203,   250,   251,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,   220,    24,   227,   238,   244,
     204,   257,   260,   258,   261,   279,   262,   263,   264,   265,
     266,   267,   143,   268,   271,   143,   277,   269,   281,   290,
     292,   300,   295,    56,   143,   297,   289,   217,   283,   302,
     225,   285,   299,    70,    39,   143,    40,   143,    69,    77,
     226,   282,   164,   284,    56,   218,    63,   294,   255,   252,
     100,   154,   256,    87,   298,   254,   272,   296,   176,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   303
};

static const short yycheck[] =
{
       4,    23,    67,    36,   181,    26,    71,   195,    84,    27,
      14,    15,     8,    89,    35,    19,    92,    21,    39,    23,
      24,     0,    24,    25,    18,    43,    48,    49,    32,    33,
      34,    34,    83,     3,    38,    24,    25,    70,     3,    90,
      91,    27,    44,    47,    48,    49,    24,    25,    54,    55,
     126,    55,    27,    26,    43,    59,    51,    43,    54,    55,
      54,    55,    43,    52,    53,    54,    55,    42,    72,    47,
     258,    75,     7,    54,    55,   108,    50,    64,   129,    66,
      84,    54,    55,   271,    88,    89,   107,     4,    92,    76,
     267,    95,    96,    97,    98,     6,     4,   162,     6,   103,
     103,   114,   106,   106,     4,    29,     6,    31,   121,   169,
      34,   171,   188,   100,   190,   292,    42,   193,   122,    40,
      41,    43,   126,    21,   111,    49,   113,    24,    25,    53,
      54,    55,    48,   120,    36,    37,    38,    39,    27,    43,
     191,   192,    22,    23,    45,   149,   197,    36,    37,    38,
      39,   155,    47,    42,    43,   159,    45,    36,    37,    38,
      39,    47,   166,   182,   183,   169,    54,   171,   172,   173,
     246,   127,   128,   249,   161,   184,   185,    54,    33,    30,
      32,    35,    42,   187,   188,     6,   190,    36,    36,   193,
      43,   195,    42,   269,    27,    27,   247,   248,   202,   203,
     204,    47,    43,    27,   208,    36,    46,    46,    44,    47,
     214,    54,    42,   200,   201,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    44,    19,    44,    54,    44,
      18,    42,    44,    43,    47,   257,    42,    44,    47,    42,
      44,    43,   246,    44,    43,   249,    42,    47,    42,    28,
      36,    48,    44,   257,   258,    44,   278,   261,   262,    44,
     264,   265,    52,    38,    11,   269,    12,   271,    37,    46,
     175,   261,   109,   264,   278,   168,    32,   281,   208,   202,
      63,    97,   209,    55,   289,   204,   245,   287,   122,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   298
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    57,    58,     0,     3,    59,    62,    54,    55,    61,
     122,    51,    60,    64,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    19,    71,    72,    73,    74,    87,
      88,    93,    97,     4,     6,    65,    66,    69,     3,    62,
      64,    50,   122,   122,    94,   122,   122,    29,    31,    34,
      49,    53,    55,   108,   109,   110,   122,    91,    92,   122,
       7,    63,    71,    94,    70,   122,    70,    71,    69,    66,
      61,    71,    42,    21,    75,    43,    96,    75,   108,   116,
     108,   108,    43,    40,    41,    48,    95,   109,    27,    36,
      37,    38,    39,    42,    43,    45,    45,    47,    43,   122,
      96,    95,    47,    78,    95,    63,     6,    67,    68,    69,
      63,   122,    54,    76,    79,    85,    86,   122,    95,    54,
      77,    80,    33,    30,    32,    35,    36,    24,    25,    43,
      52,    53,    54,    55,   118,   119,   120,   121,   122,    26,
      54,    55,   117,   122,   122,   117,   118,   118,   117,    42,
     112,   118,   122,   122,    91,   122,    89,    90,   122,    95,
      70,    70,    71,    69,    68,    95,    43,    95,    78,    36,
      98,    47,    42,    43,    95,    78,   116,   117,   121,   121,
     118,    43,    24,    25,    22,    23,    27,    43,    42,    27,
      36,    37,    38,    39,   122,    36,   111,    47,   113,    46,
      46,    44,    47,    42,    18,    99,   100,   101,   102,   103,
     106,   122,    95,    63,     8,    81,    82,   122,    76,    85,
      44,    85,   122,    83,    84,   122,    77,    44,    44,   112,
     119,   119,   120,   120,    54,   114,   117,   117,    54,   117,
     118,   118,   117,   114,    44,   118,    36,    37,    38,    39,
      95,    95,    89,   122,   106,   100,   101,    42,    43,   122,
      44,    47,    42,    44,    47,    42,    44,    43,    44,    47,
     115,    43,   113,   117,   118,   118,   117,    42,   105,   108,
     114,    42,    81,   122,    83,   122,   112,   117,   114,   108,
      28,   104,    36,   107,   122,    44,   115,    44,   104,    52,
      48,   112,    44,    95
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 105 "gen.y"
    {
yy yyb;
yy yyv_U;
yy yy_2_1;
yy yyv_Imports;
yy yy_3_1;
yy yy_5_1;
yy yy_5_2;
yy_2_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[-1].attr[1]);
yyv_U = yy_2_1;
yyv_Imports = yy_3_1;
yy_5_1 = yyv_U;
yy_5_2 = yyv_Imports;
Trafo(yy_5_1, yy_5_2);
yyval.attr[0] = yyvsp[-3].attr[0];
}
    break;

  case 3:
#line 125 "gen.y"
    {
yy yyb;
yy yy_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1 = yyb + 0;
yy_1[0] = 2;
yyglov_tokenList = yy_1;
yyGetPos(&yyval.attr[0]);
}
    break;

  case 4:
#line 143 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_P;
yy yy_2;
yy yyv_I;
yy yy_3_1;
yy yyv_E;
yy yy_4_1;
yy yyv_D;
yy yy_5_1;
yy_2 = (yy)(yyvsp[-4].attr[0]);
yy_3_1 = (yy)(yyvsp[-3].attr[1]);
yy_4_1 = (yy)(yyvsp[-2].attr[1]);
yy_5_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yyv_I = yy_3_1;
yyv_E = yy_4_1;
yyv_D = yy_5_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_E;
yy_0_1_3 = yyv_D;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 5:
#line 187 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_2_1;
yy yy_0_1_2_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_I;
yy yy_1_1;
yy yyv_D;
yy yy_2_1;
yy yyv_P;
yy yy_3;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2_1 = (yy)(yyvsp[-1].attr[1]);
yy_3 = (yy)(yyvsp[-1].attr[0]);
yyb = yyh;
yyh += 9; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_D = yy_2_1;
yyv_P = yy_3;
yy_0_1_1 = yyv_I;
yy_0_1_2_1 = yyb + 8;
yy_0_1_2_1[0] = 2;
yy_0_1_2_2 = yyv_P;
yy_0_1_2 = yyb + 5;
yy_0_1_2[0] = 1;
yy_0_1_2[1] = ((long)yy_0_1_2_1);
yy_0_1_2[2] = ((long)yy_0_1_2_2);
yy_0_1_3 = yyv_D;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 6:
#line 236 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_2_1;
yy yy_0_1_2_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_I;
yy yy_2_1;
yy yyv_D;
yy yy_3_1;
yy yyv_P;
yy yy_4;
yy_2_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[-1].attr[1]);
yy_4 = (yy)(yyvsp[-1].attr[0]);
yyb = yyh;
yyh += 9; if (yyh > yyhx) yyExtend();
yyv_I = yy_2_1;
yyv_D = yy_3_1;
yyv_P = yy_4;
yy_0_1_1 = yyv_I;
yy_0_1_2_1 = yyb + 8;
yy_0_1_2_1[0] = 2;
yy_0_1_2_2 = yyv_P;
yy_0_1_2 = yyb + 5;
yy_0_1_2[0] = 1;
yy_0_1_2[1] = ((long)yy_0_1_2_1);
yy_0_1_2[2] = ((long)yy_0_1_2_2);
yy_0_1_3 = yyv_D;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-3].attr[0];
}
    break;

  case 7:
#line 287 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_P;
yy yy_3;
yy yyv_I;
yy yy_4_1;
yy yyv_F;
yy yy_5_1;
yy yyv_D;
yy yy_6_1;
yy_3 = (yy)(yyvsp[-4].attr[0]);
yy_4_1 = (yy)(yyvsp[-3].attr[1]);
yy_5_1 = (yy)(yyvsp[-2].attr[1]);
yy_6_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_P = yy_3;
yyv_I = yy_4_1;
yyv_F = yy_5_1;
yyv_D = yy_6_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_F;
yy_0_1_3 = yyv_D;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-5].attr[0];
}
    break;

  case 8:
#line 329 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_I;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_I = yy_1_1;
yy_0_1 = yyv_I;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 9:
#line 343 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_Str;
yy yy_1_1;
yy yy_2_1;
yy yyv_I;
yy yy_2_2;
GetSourceName(&yy_1_1);
yyv_Str = yy_1_1;
yy_2_1 = yyv_Str;
string_to_id(yy_2_1, &yy_2_2);
yyv_I = yy_2_2;
yy_0_1 = yyv_I;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 10:
#line 364 "gen.y"
    {
yy yyb;
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 11:
#line 371 "gen.y"
    {
yy yyb;
yyGetPos(&yyval.attr[0]);
}
    break;

  case 12:
#line 380 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy yyv_T;
yy yy_2_1;
yy_1_1 = (yy)(yyvsp[-1].attr[1]);
yy_2_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_T = yy_2_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 13:
#line 407 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 14:
#line 422 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_E;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy_1_1 = (yy)(yyvsp[-1].attr[1]);
yy_2 = (yy)(yyvsp[-1].attr[0]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_E = yy_1_1;
yyv_P = yy_2;
yy_0_1_1 = yyv_E;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 15:
#line 451 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_P;
yy yy_2;
yy yyv_E;
yy yy_3_1;
yy_2 = (yy)(yyvsp[-1].attr[0]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yyv_E = yy_3_1;
yy_0_1_1 = yyv_E;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 16:
#line 479 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_P;
yy yy_2;
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yy_0_1_1 = yyb + 3;
yy_0_1_1[0] = 2;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 17:
#line 504 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_E;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_E = yy_1_1;
yyv_P = yy_2;
yy_0_1_1 = yyv_E;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 18:
#line 531 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_P;
yy yy_1;
yyGetPos(&yy_1);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_P = yy_1;
yy_0_1_1 = yyb + 3;
yy_0_1_1[0] = 2;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 19:
#line 558 "gen.y"
    {
yy yyb;
yy yyv_L;
yy yy_2_1;
yy yy_4_1;
yy_2_1 = (yy)(yyvsp[-1].attr[1]);
yyv_L = yy_2_1;
yy_4_1 = yyv_L;
PrepareImport(yy_4_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 20:
#line 574 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_E;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy_1_1 = (yy)(yyvsp[-1].attr[1]);
yy_2 = (yy)(yyvsp[-1].attr[0]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_E = yy_1_1;
yyv_P = yy_2;
yy_0_1_1 = yyv_E;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 21:
#line 603 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_P;
yy yy_2;
yy yyv_E;
yy yy_3_1;
yy_2 = (yy)(yyvsp[-1].attr[0]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yyv_E = yy_3_1;
yy_0_1_1 = yyv_E;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 22:
#line 631 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_P;
yy yy_2;
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yy_0_1_1 = yyb + 3;
yy_0_1_1[0] = 2;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 23:
#line 656 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_E;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_E = yy_1_1;
yyv_P = yy_2;
yy_0_1_1 = yyv_E;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 24:
#line 683 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_P;
yy yy_1;
yyGetPos(&yy_1);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_P = yy_1;
yy_0_1_1 = yyb + 3;
yy_0_1_1[0] = 2;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 25:
#line 710 "gen.y"
    {
yy yyb;
yy yyv_L;
yy yy_2_1;
yy_2_1 = (yy)(yyvsp[-1].attr[1]);
yyv_L = yy_2_1;
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 26:
#line 724 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_E;
yy yy_2_1;
yy_2_1 = (yy)(yyvsp[-1].attr[1]);
yyv_E = yy_2_1;
yy_0_1 = yyv_E;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 27:
#line 741 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_I;
yy yy_1_1;
yy yyv_L;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_L = yy_3_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_L;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 28:
#line 768 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 29:
#line 783 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy yyv_T;
yy yy_2_1;
yy_1_1 = (yy)(yyvsp[-1].attr[1]);
yy_2_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_T = yy_2_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 30:
#line 810 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 31:
#line 824 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_D;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_D = yy_1_1;
yy_0_1 = yyv_D;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 32:
#line 839 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_D;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_D = yy_1_1;
yy_0_1 = yyv_D;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 33:
#line 854 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_D;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_D = yy_1_1;
yy_0_1 = yyv_D;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 34:
#line 869 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_D;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_D = yy_1_1;
yy_0_1 = yyv_D;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 35:
#line 884 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_D;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_D = yy_1_1;
yy_0_1 = yyv_D;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 36:
#line 901 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_P;
yy yy_2;
yy yyv_M;
yy yy_3_1;
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_3_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yyv_M = yy_3_1;
yy_0_1_1 = yyv_P;
yy_0_1_2 = yyv_M;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 37:
#line 933 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yy_0_1_3_2;
yy yyv_I;
yy yy_2_1;
yy yyv_P;
yy yy_3;
yy yyv_F;
yy yy_5_1;
yy_2_1 = (yy)(yyvsp[-3].attr[1]);
yy_3 = (yy)(yyvsp[-3].attr[0]);
yy_5_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 8; if (yyh > yyhx) yyExtend();
yyv_I = yy_2_1;
yyv_P = yy_3;
yyv_F = yy_5_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3_1 = yyb + 7;
yy_0_1_3_1[0] = 1;
yy_0_1_3_2 = yyv_F;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 1;
yy_0_1_3[1] = ((long)yy_0_1_3_1);
yy_0_1_3[2] = ((long)yy_0_1_3_2);
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 38:
#line 980 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yy_0_1_3_2;
yy yyv_I;
yy yy_2_1;
yy yyv_P;
yy yy_3;
yy yyv_F;
yy yy_5_1;
yy_2_1 = (yy)(yyvsp[-3].attr[1]);
yy_3 = (yy)(yyvsp[-3].attr[0]);
yy_5_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 8; if (yyh > yyhx) yyExtend();
yyv_I = yy_2_1;
yyv_P = yy_3;
yyv_F = yy_5_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3_1 = yyb + 7;
yy_0_1_3_1[0] = 2;
yy_0_1_3_2 = yyv_F;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 1;
yy_0_1_3[1] = ((long)yy_0_1_3_1);
yy_0_1_3[2] = ((long)yy_0_1_3_2);
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 39:
#line 1023 "gen.y"
    {
yy yyb;
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 40:
#line 1030 "gen.y"
    {
yy yyb;
yyGetPos(&yyval.attr[0]);
}
    break;

  case 41:
#line 1040 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy yyv_T;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_T = yy_3_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 42:
#line 1067 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 43:
#line 1083 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy yyv_T;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_T = yy_3_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 44:
#line 1110 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 45:
#line 1124 "gen.y"
    {
yy yyb;
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 46:
#line 1131 "gen.y"
    {
yy yyb;
yyGetPos(&yyval.attr[0]);
}
    break;

  case 47:
#line 1142 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_A;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-3].attr[1]);
yy_2 = (yy)(yyvsp[-3].attr[0]);
yy_4_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yyv_A = yy_4_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_A;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-3].attr[0];
}
    break;

  case 48:
#line 1177 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 49:
#line 1212 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_A;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-3].attr[1]);
yy_2 = (yy)(yyvsp[-3].attr[0]);
yy_4_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yyv_A = yy_4_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_A;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-3].attr[0];
}
    break;

  case 50:
#line 1247 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 51:
#line 1281 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_T;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_P = yy_2;
yyv_T = yy_4_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 52:
#line 1315 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 53:
#line 1329 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_P = yy_2;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 54:
#line 1361 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yy_0_1_1 = yyb + 4;
yy_0_1_1[0] = 2;
yy_0_1_2 = yyv_I;
yy_0_1_3 = yyb + 5;
yy_0_1_3[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 55:
#line 1392 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Name;
yy yy_1_1;
yy yyv_I;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yyv_Name = yy_1_1;
yyv_I = yy_3_1;
yy_0_1_1_1 = yyv_Name;
yy_0_1_1 = yyb + 4;
yy_0_1_1[0] = 1;
yy_0_1_1[1] = ((long)yy_0_1_1_1);
yy_0_1_2 = yyv_I;
yy_0_1_3 = yyb + 6;
yy_0_1_3[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 56:
#line 1431 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Name;
yy yy_2_1;
yy yyv_I;
yy yy_4_1;
yy_2_1 = (yy)(yyvsp[-2].attr[1]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yyv_Name = yy_2_1;
yyv_I = yy_4_1;
yy_0_1_1_1 = yyv_Name;
yy_0_1_1 = yyb + 4;
yy_0_1_1[0] = 1;
yy_0_1_1[1] = ((long)yy_0_1_1_1);
yy_0_1_2 = yyv_I;
yy_0_1_3 = yyb + 6;
yy_0_1_3[0] = 1;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-3].attr[0];
}
    break;

  case 57:
#line 1469 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_T;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_P = yy_2;
yyv_T = yy_4_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 58:
#line 1503 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 59:
#line 1517 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_P = yy_2;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 60:
#line 1549 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yy_0_1_1 = yyb + 4;
yy_0_1_1[0] = 2;
yy_0_1_2 = yyv_I;
yy_0_1_3 = yyb + 5;
yy_0_1_3[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 61:
#line 1580 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Name;
yy yy_1_1;
yy yyv_I;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yyv_Name = yy_1_1;
yyv_I = yy_3_1;
yy_0_1_1_1 = yyv_Name;
yy_0_1_1 = yyb + 4;
yy_0_1_1[0] = 1;
yy_0_1_1[1] = ((long)yy_0_1_1_1);
yy_0_1_2 = yyv_I;
yy_0_1_3 = yyb + 6;
yy_0_1_3[0] = 1;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 62:
#line 1618 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_T;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_P = yy_2;
yyv_T = yy_4_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 63:
#line 1652 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 64:
#line 1666 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_P = yy_2;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 65:
#line 1698 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yy_0_1_1 = yyb + 4;
yy_0_1_1[0] = 2;
yy_0_1_2 = yyv_I;
yy_0_1_3 = yyb + 5;
yy_0_1_3[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 66:
#line 1729 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Name;
yy yy_1_1;
yy yyv_I;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yyv_Name = yy_1_1;
yyv_I = yy_3_1;
yy_0_1_1_1 = yyv_Name;
yy_0_1_1 = yyb + 4;
yy_0_1_1[0] = 1;
yy_0_1_1[1] = ((long)yy_0_1_1_1);
yy_0_1_2 = yyv_I;
yy_0_1_3 = yyb + 6;
yy_0_1_3[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 67:
#line 1769 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yy_0_1_3_2;
yy yyv_V;
yy yy_2_1;
yy yyv_P;
yy yy_3;
yy yyv_T;
yy yy_5_1;
yy yyv_P2;
yy yy_6;
yy_2_1 = (yy)(yyvsp[-3].attr[1]);
yy_3 = (yy)(yyvsp[-3].attr[0]);
yy_5_1 = (yy)(yyvsp[-1].attr[1]);
yy_6 = (yy)(yyvsp[-1].attr[0]);
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yyv_V = yy_2_1;
yyv_P = yy_3;
yyv_T = yy_5_1;
yyv_P2 = yy_6;
yy_0_1_1 = yyv_V;
yy_0_1_2 = yyv_P;
yy_0_1_3_1 = yyv_T;
yy_0_1_3_2 = yyv_P2;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 2;
yy_0_1_3[1] = ((long)yy_0_1_3_1);
yy_0_1_3[2] = ((long)yy_0_1_3_2);
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 68:
#line 1820 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yyv_P;
yy yy_2;
yy yyv_Class;
yy yy_3_1;
yy yyv_F;
yy yy_5_1;
yy_2 = (yy)(yyvsp[-5].attr[0]);
yy_3_1 = (yy)(yyvsp[-4].attr[1]);
yy_5_1 = (yy)(yyvsp[-2].attr[1]);
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yyv_Class = yy_3_1;
yyv_F = yy_5_1;
yy_0_1_1 = yyv_Class;
yy_0_1_2 = yyv_P;
yy_0_1_3_1 = yyv_F;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 3;
yy_0_1_3[1] = ((long)yy_0_1_3_1);
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-5].attr[0];
}
    break;

  case 69:
#line 1864 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yyv_P;
yy yy_2;
yy yyv_F;
yy yy_3_1;
yy yyv_K;
yy yy_5_1;
yy_2 = (yy)(yyvsp[-5].attr[0]);
yy_3_1 = (yy)(yyvsp[-4].attr[1]);
yy_5_1 = (yy)(yyvsp[-2].attr[1]);
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yyv_F = yy_3_1;
yyv_K = yy_5_1;
yy_0_1_1 = yyv_K;
yy_0_1_2 = yyv_P;
yy_0_1_3_1 = yyv_F;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 3;
yy_0_1_3[1] = ((long)yy_0_1_3_1);
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-5].attr[0];
}
    break;

  case 70:
#line 1905 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy yyv_T;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_T = yy_3_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 71:
#line 1933 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyb + 3;
yy_0_1_2[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 72:
#line 1960 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Name;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Type;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_Name = yy_1_1;
yyv_P = yy_2;
yyv_Type = yy_4_1;
yy_0_1_1 = yyv_Name;
yy_0_1_2 = yyv_Type;
yy_0_1_3 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 73:
#line 1997 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy yyv_T;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_T = yy_3_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 74:
#line 2025 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyb + 3;
yy_0_1_2[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 75:
#line 2051 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Type;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Name;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[-1].attr[1]);
yy_2 = (yy)(yyvsp[-1].attr[0]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_Type = yy_1_1;
yyv_P = yy_2;
yyv_Name = yy_3_1;
yy_0_1_1 = yyv_Name;
yy_0_1_2 = yyv_Type;
yy_0_1_3 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 76:
#line 2090 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yy_0_1_3_1_1;
yy yy_0_1_3_2;
yy yy_0_1_3_3;
yy yy_0_1_3_4;
yy yyv_C;
yy yy_1_1;
yy yyv_I;
yy yy_2_1;
yy yyv_P;
yy yy_3;
yy yyv_F;
yy yy_4_1;
yy yyv_R;
yy yy_6_1;
yy_1_1 = (yy)(yyvsp[-4].attr[1]);
yy_2_1 = (yy)(yyvsp[-3].attr[1]);
yy_3 = (yy)(yyvsp[-3].attr[0]);
yy_4_1 = (yy)(yyvsp[-2].attr[1]);
yy_6_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 11; if (yyh > yyhx) yyExtend();
yyv_C = yy_1_1;
yyv_I = yy_2_1;
yyv_P = yy_3;
yyv_F = yy_4_1;
yyv_R = yy_6_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3_1_1 = yyv_I;
yy_0_1_3_1 = yyb + 9;
yy_0_1_3_1[0] = 1;
yy_0_1_3_1[1] = ((long)yy_0_1_3_1_1);
yy_0_1_3_2 = yyv_C;
yy_0_1_3_3 = yyv_F;
yy_0_1_3_4 = yyv_R;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 4;
yy_0_1_3[1] = ((long)yy_0_1_3_1);
yy_0_1_3[2] = ((long)yy_0_1_3_2);
yy_0_1_3[3] = ((long)yy_0_1_3_3);
yy_0_1_3[4] = ((long)yy_0_1_3_4);
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 77:
#line 2153 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yy_0_1_3_1_1;
yy yy_0_1_3_2;
yy yy_0_1_3_3;
yy yy_0_1_3_4;
yy yyv_P;
yy yy_2;
yy yyv_I;
yy yy_3_1;
yy yyv_F;
yy yy_4_1;
yy_2 = (yy)(yyvsp[-3].attr[0]);
yy_3_1 = (yy)(yyvsp[-2].attr[1]);
yy_4_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 13; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yyv_I = yy_3_1;
yyv_F = yy_4_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3_1_1 = yyv_I;
yy_0_1_3_1 = yyb + 9;
yy_0_1_3_1[0] = 1;
yy_0_1_3_1[1] = ((long)yy_0_1_3_1_1);
yy_0_1_3_2 = yyb + 11;
yy_0_1_3_2[0] = 4;
yy_0_1_3_3 = yyv_F;
yy_0_1_3_4 = yyb + 12;
yy_0_1_3_4[0] = 2;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 4;
yy_0_1_3[1] = ((long)yy_0_1_3_1);
yy_0_1_3[2] = ((long)yy_0_1_3_2);
yy_0_1_3[3] = ((long)yy_0_1_3_3);
yy_0_1_3[4] = ((long)yy_0_1_3_4);
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-3].attr[0];
}
    break;

  case 78:
#line 2207 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_I;
yy yy_1_1;
yy yy_2;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_I = yy_1_1;
yy_2 = yyv_I;
yyglov_currentGroupIdent = yy_2;
yy_0_1 = yyv_I;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 79:
#line 2225 "gen.y"
    {
yy yyb;
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 80:
#line 2232 "gen.y"
    {
yy yyb;
yyGetPos(&yyval.attr[0]);
}
    break;

  case 81:
#line 2243 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_In;
yy yy_2_1;
yy yyv_Out;
yy yy_3_1;
yy_2_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_In = yy_2_1;
yyv_Out = yy_3_1;
yy_0_1_1 = yyv_In;
yy_0_1_2 = yyv_Out;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-3].attr[0];
}
    break;

  case 82:
#line 2270 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yy_0_1_1 = yyb + 3;
yy_0_1_1[0] = 2;
yy_0_1_2 = yyb + 4;
yy_0_1_2[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 83:
#line 2292 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 84:
#line 2306 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 85:
#line 2320 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 3;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 86:
#line 2334 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 5;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 87:
#line 2348 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 6;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 88:
#line 2363 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_L;
yy yy_2_1;
yy_2_1 = (yy)(yyvsp[0].attr[1]);
yyv_L = yy_2_1;
yy_0_1 = yyv_L;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 89:
#line 2377 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 90:
#line 2391 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_L;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_L = yy_1_1;
yy_0_1 = yyv_L;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 91:
#line 2406 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_L;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_L = yy_1_1;
yy_0_1 = yyv_L;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 92:
#line 2420 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 93:
#line 2435 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy yyv_T;
yy yy_2_1;
yy_1_1 = (yy)(yyvsp[-1].attr[1]);
yy_2_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_T = yy_2_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 94:
#line 2463 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyb + 3;
yy_0_1_2[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 95:
#line 2489 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy yyv_T;
yy yy_2_1;
yy_1_1 = (yy)(yyvsp[-1].attr[1]);
yy_2_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_T = yy_2_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 96:
#line 2517 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyb + 3;
yy_0_1_2[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 97:
#line 2546 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_L;
yy yy_1_1;
yy yyv_M;
yy yy_3_1;
yy yyv_Cost;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-4].attr[1]);
yy_3_1 = (yy)(yyvsp[-2].attr[1]);
yy_4_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_L = yy_1_1;
yyv_M = yy_3_1;
yyv_Cost = yy_4_1;
yy_0_1_1 = yyv_L;
yy_0_1_2 = yyv_M;
yy_0_1_3 = yyv_Cost;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 98:
#line 2586 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_L;
yy yy_2_1;
yy yyv_M;
yy yy_4_1;
yy yyv_Cost;
yy yy_5_1;
yy_2_1 = (yy)(yyvsp[-4].attr[1]);
yy_4_1 = (yy)(yyvsp[-2].attr[1]);
yy_5_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_L = yy_2_1;
yyv_M = yy_4_1;
yyv_Cost = yy_5_1;
yy_0_1_1 = yyv_L;
yy_0_1_2 = yyv_M;
yy_0_1_3 = yyv_Cost;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-5].attr[0];
}
    break;

  case 99:
#line 2622 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_2_1;
yy_2_1 = (yy)(yyvsp[0].attr[1]);
yyv_N = yy_2_1;
yy_0_1 = yyv_N;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 100:
#line 2636 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy_0_1 = ((yy)0);
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 101:
#line 2647 "gen.y"
    {
yy yyb;
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 102:
#line 2654 "gen.y"
    {
yy yyb;
yyGetPos(&yyval.attr[0]);
}
    break;

  case 103:
#line 2662 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yy_0_1_4;
yy yy_0_1_4_1;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 9; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3_1 = yyv_P;
yy_0_1_3 = yyb + 5;
yy_0_1_3[0] = 2;
yy_0_1_3[1] = ((long)yy_0_1_3_1);
yy_0_1_4_1 = yyv_P;
yy_0_1_4 = yyb + 7;
yy_0_1_4[0] = 2;
yy_0_1_4[1] = ((long)yy_0_1_4_1);
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 104:
#line 2708 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_In;
yy yy_4_1;
yy yyv_Out;
yy yy_5_1;
yy_1_1 = (yy)(yyvsp[-4].attr[1]);
yy_2 = (yy)(yyvsp[-4].attr[0]);
yy_4_1 = (yy)(yyvsp[-2].attr[1]);
yy_5_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yyv_In = yy_4_1;
yyv_Out = yy_5_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_In;
yy_0_1_4 = yyv_Out;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 105:
#line 2751 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_L;
yy yy_2_1;
yy_2_1 = (yy)(yyvsp[0].attr[1]);
yyv_L = yy_2_1;
yy_0_1 = yyv_L;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 106:
#line 2765 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_P;
yy yy_1;
yyGetPos(&yy_1);
yyb = yyh;
yyh += 2; if (yyh > yyhx) yyExtend();
yyv_P = yy_1;
yy_0_1_1 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 107:
#line 2786 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_List;
yy yy_1_1;
yy yyv_Space;
yy yy_2_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_List = yy_1_1;
NewINT(&yy_2_1);
yyv_Space = yy_2_1;
yy_0_1_1 = yyv_List;
yy_0_1_2 = yyv_Space;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 108:
#line 2815 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_H;
yy yy_1_1;
yy yyv_T;
yy yy_2_1;
yy_1_1 = (yy)(yyvsp[-1].attr[1]);
yy_2_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_T = yy_2_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 109:
#line 2842 "gen.y"
    {
yy yyb;
yy yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 110:
#line 2858 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yy_0_1_1_2;
yy yy_0_1_1_3;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_A;
yy yy_4_1;
yy yyv_Tempo;
yy yy_5_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 8; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yyv_A = yy_4_1;
NewTempo(&yy_5_1);
yyv_Tempo = yy_5_1;
yy_0_1_1_1 = yyv_Tempo;
yy_0_1_1_2 = yyv_I;
yy_0_1_1_3 = yyv_P;
yy_0_1_1 = yyb + 4;
yy_0_1_1[0] = 3;
yy_0_1_1[1] = ((long)yy_0_1_1_1);
yy_0_1_1[2] = ((long)yy_0_1_1_2);
yy_0_1_1[3] = ((long)yy_0_1_1_3);
yy_0_1_2 = yyv_A;
yy_0_1_3 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 111:
#line 2909 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_2_1;
yy yy_0_1_2_2;
yy yy_0_1_2_3;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_A;
yy yy_4_1;
yy yyv_Tempo;
yy yy_5_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 8; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yyv_A = yy_4_1;
NewTempo(&yy_5_1);
yyv_Tempo = yy_5_1;
yy_0_1_1 = yyv_A;
yy_0_1_2_1 = yyv_Tempo;
yy_0_1_2_2 = yyv_I;
yy_0_1_2_3 = yyv_P;
yy_0_1_2 = yyb + 4;
yy_0_1_2[0] = 5;
yy_0_1_2[1] = ((long)yy_0_1_2_1);
yy_0_1_2[2] = ((long)yy_0_1_2_2);
yy_0_1_2[3] = ((long)yy_0_1_2_3);
yy_0_1_3 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 112:
#line 2960 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_A;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yyv_A = yy_4_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_A;
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 113:
#line 2997 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_A;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yyv_A = yy_4_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_A;
yy_0_1 = yyb + 0;
yy_0_1[0] = 3;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 114:
#line 3036 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_Key;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Field;
yy yy_4_1;
yy yyv_Val;
yy yy_6_1;
yy_1_1 = (yy)(yyvsp[-4].attr[1]);
yy_2 = (yy)(yyvsp[-4].attr[0]);
yy_4_1 = (yy)(yyvsp[-2].attr[1]);
yy_6_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_Key = yy_1_1;
yyv_P = yy_2;
yyv_Field = yy_4_1;
yyv_Val = yy_6_1;
yy_0_1_1 = yyv_Key;
yy_0_1_2 = yyv_Field;
yy_0_1_3 = yyv_Val;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 5;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 115:
#line 3083 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_Field;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Key;
yy yy_4_1;
yy yyv_Val;
yy yy_7_1;
yy_1_1 = (yy)(yyvsp[-5].attr[1]);
yy_2 = (yy)(yyvsp[-5].attr[0]);
yy_4_1 = (yy)(yyvsp[-3].attr[1]);
yy_7_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_Field = yy_1_1;
yyv_P = yy_2;
yyv_Key = yy_4_1;
yyv_Val = yy_7_1;
yy_0_1_1 = yyv_Key;
yy_0_1_2 = yyv_Field;
yy_0_1_3 = yyv_Val;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 5;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-5].attr[0];
}
    break;

  case 116:
#line 3129 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_Key;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Field;
yy yy_4_1;
yy yyv_Val;
yy yy_6_1;
yy_1_1 = (yy)(yyvsp[-4].attr[1]);
yy_2 = (yy)(yyvsp[-4].attr[0]);
yy_4_1 = (yy)(yyvsp[-2].attr[1]);
yy_6_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_Key = yy_1_1;
yyv_P = yy_2;
yyv_Field = yy_4_1;
yyv_Val = yy_6_1;
yy_0_1_1 = yyv_Key;
yy_0_1_2 = yyv_Field;
yy_0_1_3 = yyv_Val;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 6;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 117:
#line 3176 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_Field;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Key;
yy yy_4_1;
yy yyv_Val;
yy yy_7_1;
yy_1_1 = (yy)(yyvsp[-5].attr[1]);
yy_2 = (yy)(yyvsp[-5].attr[0]);
yy_4_1 = (yy)(yyvsp[-3].attr[1]);
yy_7_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_Field = yy_1_1;
yyv_P = yy_2;
yyv_Key = yy_4_1;
yyv_Val = yy_7_1;
yy_0_1_1 = yyv_Key;
yy_0_1_2 = yyv_Field;
yy_0_1_3 = yyv_Val;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 6;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-5].attr[0];
}
    break;

  case 118:
#line 3220 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_A;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yyv_A = yy_4_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_A;
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 119:
#line 3257 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_A;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yyv_A = yy_4_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_A;
yy_0_1 = yyb + 0;
yy_0_1[0] = 3;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 120:
#line 3296 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_Key;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Field;
yy yy_4_1;
yy yyv_Val;
yy yy_6_1;
yy_1_1 = (yy)(yyvsp[-4].attr[1]);
yy_2 = (yy)(yyvsp[-4].attr[0]);
yy_4_1 = (yy)(yyvsp[-2].attr[1]);
yy_6_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_Key = yy_1_1;
yyv_P = yy_2;
yyv_Field = yy_4_1;
yyv_Val = yy_6_1;
yy_0_1_1 = yyv_Key;
yy_0_1_2 = yyv_Field;
yy_0_1_3 = yyv_Val;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 5;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 121:
#line 3343 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_Field;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Key;
yy yy_4_1;
yy yyv_Val;
yy yy_7_1;
yy_1_1 = (yy)(yyvsp[-5].attr[1]);
yy_2 = (yy)(yyvsp[-5].attr[0]);
yy_4_1 = (yy)(yyvsp[-3].attr[1]);
yy_7_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_Field = yy_1_1;
yyv_P = yy_2;
yyv_Key = yy_4_1;
yyv_Val = yy_7_1;
yy_0_1_1 = yyv_Key;
yy_0_1_2 = yyv_Field;
yy_0_1_3 = yyv_Val;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 5;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-5].attr[0];
}
    break;

  case 122:
#line 3389 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_Key;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Field;
yy yy_4_1;
yy yyv_Val;
yy yy_6_1;
yy_1_1 = (yy)(yyvsp[-4].attr[1]);
yy_2 = (yy)(yyvsp[-4].attr[0]);
yy_4_1 = (yy)(yyvsp[-2].attr[1]);
yy_6_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_Key = yy_1_1;
yyv_P = yy_2;
yyv_Field = yy_4_1;
yyv_Val = yy_6_1;
yy_0_1_1 = yyv_Key;
yy_0_1_2 = yyv_Field;
yy_0_1_3 = yyv_Val;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 6;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 123:
#line 3436 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_Field;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Key;
yy yy_4_1;
yy yyv_Val;
yy yy_7_1;
yy_1_1 = (yy)(yyvsp[-5].attr[1]);
yy_2 = (yy)(yyvsp[-5].attr[0]);
yy_4_1 = (yy)(yyvsp[-3].attr[1]);
yy_7_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_Field = yy_1_1;
yyv_P = yy_2;
yyv_Key = yy_4_1;
yyv_Val = yy_7_1;
yy_0_1_1 = yyv_Key;
yy_0_1_2 = yyv_Field;
yy_0_1_3 = yyv_Val;
yy_0_1_4 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 6;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-5].attr[0];
}
    break;

  case 124:
#line 3481 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_Key;
yy yy_1_1;
yy yyv_P;
yy yy_4;
yy yyv_Type;
yy yy_5_1;
yy yyv_Offset;
yy yy_6_1;
yy_1_1 = (yy)(yyvsp[-3].attr[1]);
yy_4 = (yy)(yyvsp[-1].attr[0]);
yy_5_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_Key = yy_1_1;
yyv_P = yy_4;
yyv_Type = yy_5_1;
NewINT(&yy_6_1);
yyv_Offset = yy_6_1;
yy_0_1_1 = yyv_Type;
yy_0_1_2 = yyv_Key;
yy_0_1_3 = yyv_P;
yy_0_1_4 = yyv_Offset;
yy_0_1 = yyb + 0;
yy_0_1[0] = 4;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-3].attr[0];
}
    break;

  case 125:
#line 3523 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yy_0_1_4;
yy yy_0_1_4_1;
yy yy_0_1_5;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Offset;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 10; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
NewINT(&yy_3_1);
yyv_Offset = yy_3_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3_1 = yyv_P;
yy_0_1_3 = yyb + 6;
yy_0_1_3[0] = 2;
yy_0_1_3[1] = ((long)yy_0_1_3_1);
yy_0_1_4_1 = yyv_P;
yy_0_1_4 = yyb + 8;
yy_0_1_4[0] = 2;
yy_0_1_4[1] = ((long)yy_0_1_4_1);
yy_0_1_5 = yyv_Offset;
yy_0_1 = yyb + 0;
yy_0_1[0] = 7;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yy_0_1[5] = ((long)yy_0_1_5);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 126:
#line 3576 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yy_0_1_5;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_In;
yy yy_4_1;
yy yyv_Out;
yy yy_5_1;
yy yyv_Offset;
yy yy_7_1;
yy_1_1 = (yy)(yyvsp[-4].attr[1]);
yy_2 = (yy)(yyvsp[-4].attr[0]);
yy_4_1 = (yy)(yyvsp[-2].attr[1]);
yy_5_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yyv_In = yy_4_1;
yyv_Out = yy_5_1;
NewINT(&yy_7_1);
yyv_Offset = yy_7_1;
yy_0_1_1 = yyv_I;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_In;
yy_0_1_4 = yyv_Out;
yy_0_1_5 = yyv_Offset;
yy_0_1 = yyb + 0;
yy_0_1[0] = 7;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yy_0_1[5] = ((long)yy_0_1_5);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 127:
#line 3627 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_L;
yy yy_2_1;
yy yyv_Pos;
yy yy_3;
yy yyv_P;
yy yy_5_1;
yy_2_1 = (yy)(yyvsp[-1].attr[1]);
yy_3 = (yy)(yyvsp[-1].attr[0]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_L = yy_2_1;
yyv_Pos = yy_3;
New_REF_IDENTLIST(&yy_5_1);
yyv_P = yy_5_1;
yy_0_1_1 = yyv_L;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_Pos;
yy_0_1 = yyb + 0;
yy_0_1[0] = 10;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 128:
#line 3664 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yy_0_1_1_2;
yy yy_0_1_1_3;
yy yy_0_1_1_3_1;
yy yy_0_1_1_3_1_1;
yy yy_0_1_1_3_1_2;
yy yy_0_1_1_3_2;
yy yy_0_1_1_3_3;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_M;
yy yy_2_1;
yy yyv_Pos;
yy yy_3;
yy yyv_P;
yy yy_5_1;
yy yyv_Space;
yy yy_6_1;
yy_2_1 = (yy)(yyvsp[-1].attr[1]);
yy_3 = (yy)(yyvsp[-1].attr[0]);
yyb = yyh;
yyh += 17; if (yyh > yyhx) yyExtend();
yyv_M = yy_2_1;
yyv_Pos = yy_3;
New_REF_IDENTLIST(&yy_5_1);
yyv_P = yy_5_1;
NewINT(&yy_6_1);
yyv_Space = yy_6_1;
yy_0_1_1_1 = yyv_M;
yy_0_1_1_2 = yyv_Pos;
yy_0_1_1_3_1_1 = yyb + 15;
yy_0_1_1_3_1_1[0] = 2;
yy_0_1_1_3_1_2 = yyv_Space;
yy_0_1_1_3_1 = yyb + 12;
yy_0_1_1_3_1[0] = 1;
yy_0_1_1_3_1[1] = ((long)yy_0_1_1_3_1_1);
yy_0_1_1_3_1[2] = ((long)yy_0_1_1_3_1_2);
yy_0_1_1_3_2 = yyv_Pos;
yy_0_1_1_3_3 = yyb + 16;
yy_0_1_1_3_3[0] = 2;
yy_0_1_1_3 = yyb + 8;
yy_0_1_1_3[0] = 1;
yy_0_1_1_3[1] = ((long)yy_0_1_1_3_1);
yy_0_1_1_3[2] = ((long)yy_0_1_1_3_2);
yy_0_1_1_3[3] = ((long)yy_0_1_1_3_3);
yy_0_1_1 = yyb + 4;
yy_0_1_1[0] = 1;
yy_0_1_1[1] = ((long)yy_0_1_1_1);
yy_0_1_1[2] = ((long)yy_0_1_1_2);
yy_0_1_1[3] = ((long)yy_0_1_1_3);
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_Pos;
yy_0_1 = yyb + 0;
yy_0_1[0] = 10;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 129:
#line 3734 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_2;
yy yyv_M;
yy yy_3_1;
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_3_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_Pos = yy_2;
yyv_M = yy_3_1;
yy_0_1_1 = yyv_M;
yy_0_1_2 = yyv_Pos;
yy_0_1 = yyb + 0;
yy_0_1[0] = 11;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 130:
#line 3762 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_S;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yy_3_1;
yy yy_3_2;
yy yy_3_3;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_S = yy_1_1;
yyv_P = yy_2;
yy_3_1 = yyv_S;
yy_3_2 = yyv_P;
EnterLiteral(yy_3_1, yy_3_2, &yy_3_3);
yy_0_1_1 = yyv_S;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 8;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 131:
#line 3800 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_P;
yy yy_2;
yy yyv_Arg;
yy yy_5_1;
yy_2 = (yy)(yyvsp[-4].attr[0]);
yy_5_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yyv_Arg = yy_5_1;
yy_0_1_1 = yyv_Arg;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 9;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-4].attr[0];
}
    break;

  case 132:
#line 3829 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_L;
yy yy_2_1;
yy_2_1 = (yy)(yyvsp[0].attr[1]);
yyv_L = yy_2_1;
yy_0_1 = yyv_L;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 133:
#line 3843 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_P;
yy yy_1;
yyGetPos(&yy_1);
yyb = yyh;
yyh += 2; if (yyh > yyhx) yyExtend();
yyv_P = yy_1;
yy_0_1_1 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 134:
#line 3865 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_T;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[-1].attr[1]);
yy_2 = (yy)(yyvsp[-1].attr[0]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_P = yy_2;
yyv_T = yy_3_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 135:
#line 3899 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_P;
yy yy_1;
yyGetPos(&yy_1);
yyb = yyh;
yyh += 2; if (yyh > yyhx) yyExtend();
yyv_P = yy_1;
yy_0_1_1 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 136:
#line 3922 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_2_1;
yy yyv_P;
yy yy_3;
yy yyv_T;
yy yy_4_1;
yy_2_1 = (yy)(yyvsp[-1].attr[1]);
yy_3 = (yy)(yyvsp[-1].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_2_1;
yyv_P = yy_3;
yyv_T = yy_4_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 137:
#line 3956 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_P;
yy yy_1;
yyGetPos(&yy_1);
yyb = yyh;
yyh += 2; if (yyh > yyhx) yyExtend();
yyv_P = yy_1;
yy_0_1_1 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 138:
#line 3978 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_T;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[-1].attr[1]);
yy_2 = (yy)(yyvsp[-1].attr[0]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_P = yy_2;
yyv_T = yy_3_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 139:
#line 4012 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_P;
yy yy_1;
yyGetPos(&yy_1);
yyb = yyh;
yyh += 2; if (yyh > yyhx) yyExtend();
yyv_P = yy_1;
yy_0_1_1 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 140:
#line 4035 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_2_1;
yy yyv_P;
yy yy_3;
yy yyv_T;
yy yy_4_1;
yy_2_1 = (yy)(yyvsp[-1].attr[1]);
yy_3 = (yy)(yyvsp[-1].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_2_1;
yyv_P = yy_3;
yyv_T = yy_4_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 141:
#line 4069 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_P;
yy yy_1;
yyGetPos(&yy_1);
yyb = yyh;
yyh += 2; if (yyh > yyhx) yyExtend();
yyv_P = yy_1;
yy_0_1_1 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yyval.attr[1] = ((long)yy_0_1);
yyGetPos(&yyval.attr[0]);
}
    break;

  case 142:
#line 4092 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_1_1;
yy yyv_P;
yy yy_3;
yy yyv_T;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3 = (yy)(yyvsp[-1].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_P = yy_3;
yyv_T = yy_4_1;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyv_T;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 143:
#line 4127 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_H;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_H = yy_1_1;
yyv_P = yy_2;
yy_0_1_1 = yyv_H;
yy_0_1_2 = yyv_P;
yy_0_1_3 = yyb + 4;
yy_0_1_3[0] = 2;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 144:
#line 4161 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yy_0_1_5;
yy yy_0_1_5_1;
yy yyv_T;
yy yy_1_1;
yy yyv_F;
yy yy_3_1;
yy yyv_P;
yy yy_4;
yy yyv_Tempo;
yy yy_5_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yy_4 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 8; if (yyh > yyhx) yyExtend();
yyv_T = yy_1_1;
yyv_F = yy_3_1;
yyv_P = yy_4;
NewTempo(&yy_5_1);
yyv_Tempo = yy_5_1;
yy_0_1_1 = yyv_T;
yy_0_1_2 = yyv_Tempo;
yy_0_1_3 = yyv_F;
yy_0_1_4 = yyv_P;
yy_0_1_5_1 = yyv_P;
yy_0_1_5 = yyb + 6;
yy_0_1_5[0] = 2;
yy_0_1_5[1] = ((long)yy_0_1_5_1);
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yy_0_1[5] = ((long)yy_0_1_5);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 145:
#line 4215 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yy_0_1_5;
yy yyv_T;
yy yy_1_1;
yy yyv_F;
yy yy_3_1;
yy yyv_P;
yy yy_4;
yy yyv_A;
yy yy_6_1;
yy yyv_Tempo;
yy yy_8_1;
yy_1_1 = (yy)(yyvsp[-5].attr[1]);
yy_3_1 = (yy)(yyvsp[-3].attr[1]);
yy_4 = (yy)(yyvsp[-3].attr[0]);
yy_6_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yyv_T = yy_1_1;
yyv_F = yy_3_1;
yyv_P = yy_4;
yyv_A = yy_6_1;
NewTempo(&yy_8_1);
yyv_Tempo = yy_8_1;
yy_0_1_1 = yyv_T;
yy_0_1_2 = yyv_Tempo;
yy_0_1_3 = yyv_F;
yy_0_1_4 = yyv_P;
yy_0_1_5 = yyv_A;
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yy_0_1[5] = ((long)yy_0_1_5);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-5].attr[0];
}
    break;

  case 146:
#line 4264 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yy_0_1_4_1;
yy yyv_F;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Tempo;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yyv_F = yy_1_1;
yyv_P = yy_2;
NewTempo(&yy_3_1);
yyv_Tempo = yy_3_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyv_F;
yy_0_1_3 = yyv_P;
yy_0_1_4_1 = yyv_P;
yy_0_1_4 = yyb + 5;
yy_0_1_4[0] = 2;
yy_0_1_4[1] = ((long)yy_0_1_4_1);
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 147:
#line 4309 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_F;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_A;
yy yy_4_1;
yy yyv_Tempo;
yy yy_6_1;
yy_1_1 = (yy)(yyvsp[-3].attr[1]);
yy_2 = (yy)(yyvsp[-3].attr[0]);
yy_4_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_F = yy_1_1;
yyv_P = yy_2;
yyv_A = yy_4_1;
NewTempo(&yy_6_1);
yyv_Tempo = yy_6_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyv_F;
yy_0_1_3 = yyv_P;
yy_0_1_4 = yyv_A;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-3].attr[0];
}
    break;

  case 148:
#line 4351 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Tempo;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
NewTempo(&yy_3_1);
yyv_Tempo = yy_3_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyv_I;
yy_0_1_3 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 3;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 149:
#line 4388 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_I;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_A;
yy yy_4_1;
yy yyv_Tempo;
yy yy_5_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yyv_I = yy_1_1;
yyv_P = yy_2;
yyv_A = yy_4_1;
NewTempo(&yy_5_1);
yyv_Tempo = yy_5_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyv_I;
yy_0_1_3 = yyv_P;
yy_0_1_4 = yyv_A;
yy_0_1 = yyb + 0;
yy_0_1[0] = 4;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 150:
#line 4430 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_P;
yy yy_2;
yy yyv_Tempo;
yy yy_3_1;
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
NewTempo(&yy_3_1);
yyv_Tempo = yy_3_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 5;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 151:
#line 4460 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yy_0_1_5;
yy yyv_X;
yy yy_1_1;
yy yyv_P;
yy yy_3;
yy yyv_Y;
yy yy_4_1;
yy yyv_Tempo;
yy yy_5_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3 = (yy)(yyvsp[-1].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yyv_X = yy_1_1;
yyv_P = yy_3;
yyv_Y = yy_4_1;
NewTempo(&yy_5_1);
yyv_Tempo = yy_5_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyb + 6;
yy_0_1_2[0] = 1;
yy_0_1_3 = yyv_P;
yy_0_1_4 = yyv_X;
yy_0_1_5 = yyv_Y;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yy_0_1[5] = ((long)yy_0_1_5);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 152:
#line 4508 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yy_0_1_5;
yy yyv_X;
yy yy_1_1;
yy yyv_P;
yy yy_3;
yy yyv_Y;
yy yy_4_1;
yy yyv_Tempo;
yy yy_5_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3 = (yy)(yyvsp[-1].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yyv_X = yy_1_1;
yyv_P = yy_3;
yyv_Y = yy_4_1;
NewTempo(&yy_5_1);
yyv_Tempo = yy_5_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyb + 6;
yy_0_1_2[0] = 2;
yy_0_1_3 = yyv_P;
yy_0_1_4 = yyv_X;
yy_0_1_5 = yyv_Y;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yy_0_1[5] = ((long)yy_0_1_5);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 153:
#line 4554 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_X;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_X = yy_1_1;
yy_0_1 = yyv_X;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 154:
#line 4571 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yy_0_1_5;
yy yyv_X;
yy yy_1_1;
yy yyv_P;
yy yy_3;
yy yyv_Y;
yy yy_4_1;
yy yyv_Tempo;
yy yy_5_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3 = (yy)(yyvsp[-1].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yyv_X = yy_1_1;
yyv_P = yy_3;
yyv_Y = yy_4_1;
NewTempo(&yy_5_1);
yyv_Tempo = yy_5_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyb + 6;
yy_0_1_2[0] = 3;
yy_0_1_3 = yyv_P;
yy_0_1_4 = yyv_X;
yy_0_1_5 = yyv_Y;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yy_0_1[5] = ((long)yy_0_1_5);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 155:
#line 4619 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yy_0_1_5;
yy yyv_X;
yy yy_1_1;
yy yyv_P;
yy yy_3;
yy yyv_Y;
yy yy_4_1;
yy yyv_Tempo;
yy yy_5_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_3 = (yy)(yyvsp[-1].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yyv_X = yy_1_1;
yyv_P = yy_3;
yyv_Y = yy_4_1;
NewTempo(&yy_5_1);
yyv_Tempo = yy_5_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyb + 6;
yy_0_1_2[0] = 4;
yy_0_1_3 = yyv_P;
yy_0_1_4 = yyv_X;
yy_0_1_5 = yyv_Y;
yy_0_1 = yyb + 0;
yy_0_1[0] = 1;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yy_0_1[5] = ((long)yy_0_1_5);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 156:
#line 4665 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_X;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_X = yy_1_1;
yy_0_1 = yyv_X;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 157:
#line 4680 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_X;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_X = yy_1_1;
yy_0_1 = yyv_X;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 158:
#line 4697 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_2_1;
yy yy_0_1_2_2;
yy yy_0_1_2_3;
yy yy_0_1_2_4;
yy yy_0_1_2_5;
yy yy_0_1_2_5_1;
yy yy_0_1_3;
yy yyv_Type;
yy yy_1_1;
yy yyv_P1;
yy yy_2;
yy yyv_F;
yy yy_4_1;
yy yyv_P2;
yy yy_5;
yy yyv_Tempo;
yy yy_6_1;
yy yyv_Offset;
yy yy_7_1;
yy_1_1 = (yy)(yyvsp[-2].attr[1]);
yy_2 = (yy)(yyvsp[-2].attr[0]);
yy_4_1 = (yy)(yyvsp[0].attr[1]);
yy_5 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 12; if (yyh > yyhx) yyExtend();
yyv_Type = yy_1_1;
yyv_P1 = yy_2;
yyv_F = yy_4_1;
yyv_P2 = yy_5;
NewTempo(&yy_6_1);
yyv_Tempo = yy_6_1;
NewINT(&yy_7_1);
yyv_Offset = yy_7_1;
yy_0_1_1 = yyv_Type;
yy_0_1_2_1 = yyv_Tempo;
yy_0_1_2_2 = yyv_Offset;
yy_0_1_2_3 = yyv_F;
yy_0_1_2_4 = yyv_P2;
yy_0_1_2_5_1 = yyv_P2;
yy_0_1_2_5 = yyb + 10;
yy_0_1_2_5[0] = 2;
yy_0_1_2_5[1] = ((long)yy_0_1_2_5_1);
yy_0_1_2 = yyb + 4;
yy_0_1_2[0] = 3;
yy_0_1_2[1] = ((long)yy_0_1_2_1);
yy_0_1_2[2] = ((long)yy_0_1_2_2);
yy_0_1_2[3] = ((long)yy_0_1_2_3);
yy_0_1_2[4] = ((long)yy_0_1_2_4);
yy_0_1_2[5] = ((long)yy_0_1_2_5);
yy_0_1_3 = yyv_P1;
yy_0_1 = yyb + 0;
yy_0_1[0] = 4;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 159:
#line 4769 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_2_1;
yy yy_0_1_2_2;
yy yy_0_1_2_3;
yy yy_0_1_2_4;
yy yy_0_1_2_5;
yy yy_0_1_3;
yy yyv_Type;
yy yy_1_1;
yy yyv_P1;
yy yy_2;
yy yyv_F;
yy yy_4_1;
yy yyv_P2;
yy yy_5;
yy yyv_A;
yy yy_7_1;
yy yyv_Tempo;
yy yy_9_1;
yy yyv_Offset;
yy yy_10_1;
yy_1_1 = (yy)(yyvsp[-5].attr[1]);
yy_2 = (yy)(yyvsp[-5].attr[0]);
yy_4_1 = (yy)(yyvsp[-3].attr[1]);
yy_5 = (yy)(yyvsp[-3].attr[0]);
yy_7_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 10; if (yyh > yyhx) yyExtend();
yyv_Type = yy_1_1;
yyv_P1 = yy_2;
yyv_F = yy_4_1;
yyv_P2 = yy_5;
yyv_A = yy_7_1;
NewTempo(&yy_9_1);
yyv_Tempo = yy_9_1;
NewINT(&yy_10_1);
yyv_Offset = yy_10_1;
yy_0_1_1 = yyv_Type;
yy_0_1_2_1 = yyv_Tempo;
yy_0_1_2_2 = yyv_Offset;
yy_0_1_2_3 = yyv_F;
yy_0_1_2_4 = yyv_P2;
yy_0_1_2_5 = yyv_A;
yy_0_1_2 = yyb + 4;
yy_0_1_2[0] = 3;
yy_0_1_2[1] = ((long)yy_0_1_2_1);
yy_0_1_2[2] = ((long)yy_0_1_2_2);
yy_0_1_2[3] = ((long)yy_0_1_2_3);
yy_0_1_2[4] = ((long)yy_0_1_2_4);
yy_0_1_2[5] = ((long)yy_0_1_2_5);
yy_0_1_3 = yyv_P1;
yy_0_1 = yyb + 0;
yy_0_1[0] = 4;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-5].attr[0];
}
    break;

  case 160:
#line 4836 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yy_0_1_5;
yy yy_0_1_5_1;
yy yyv_F;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Tempo;
yy yy_3_1;
yy yyv_Offset;
yy yy_4_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 8; if (yyh > yyhx) yyExtend();
yyv_F = yy_1_1;
yyv_P = yy_2;
NewTempo(&yy_3_1);
yyv_Tempo = yy_3_1;
NewINT(&yy_4_1);
yyv_Offset = yy_4_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyv_Offset;
yy_0_1_3 = yyv_F;
yy_0_1_4 = yyv_P;
yy_0_1_5_1 = yyv_P;
yy_0_1_5 = yyb + 6;
yy_0_1_5[0] = 2;
yy_0_1_5[1] = ((long)yy_0_1_5_1);
yy_0_1 = yyb + 0;
yy_0_1[0] = 3;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yy_0_1[5] = ((long)yy_0_1_5);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 161:
#line 4888 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yy_0_1_5;
yy yyv_F;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_A;
yy yy_4_1;
yy yyv_Tempo;
yy yy_6_1;
yy yyv_Offset;
yy yy_7_1;
yy_1_1 = (yy)(yyvsp[-3].attr[1]);
yy_2 = (yy)(yyvsp[-3].attr[0]);
yy_4_1 = (yy)(yyvsp[-1].attr[1]);
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yyv_F = yy_1_1;
yyv_P = yy_2;
yyv_A = yy_4_1;
NewTempo(&yy_6_1);
yyv_Tempo = yy_6_1;
NewINT(&yy_7_1);
yyv_Offset = yy_7_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyv_Offset;
yy_0_1_3 = yyv_F;
yy_0_1_4 = yyv_P;
yy_0_1_5 = yyv_A;
yy_0_1 = yyb + 0;
yy_0_1[0] = 3;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yy_0_1[5] = ((long)yy_0_1_5);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-3].attr[0];
}
    break;

  case 162:
#line 4937 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_V;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Tempo;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_V = yy_1_1;
yyv_P = yy_2;
NewTempo(&yy_3_1);
yyv_Tempo = yy_3_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyv_V;
yy_0_1_3 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 5;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 163:
#line 4972 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_N;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Tempo;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_N = yy_1_1;
yyv_P = yy_2;
NewTempo(&yy_3_1);
yyv_Tempo = yy_3_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyv_N;
yy_0_1_3 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 6;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 164:
#line 5007 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_S;
yy yy_1_1;
yy yyv_P;
yy yy_2;
yy yyv_Tempo;
yy yy_3_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yy_2 = (yy)(yyvsp[0].attr[0]);
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yyv_S = yy_1_1;
yyv_P = yy_2;
NewTempo(&yy_3_1);
yyv_Tempo = yy_3_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyv_S;
yy_0_1_3 = yyv_P;
yy_0_1 = yyb + 0;
yy_0_1[0] = 7;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 165:
#line 5043 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_P;
yy yy_2;
yy yyv_X;
yy yy_3_1;
yy yyv_Tempo;
yy yy_4_1;
yy_2 = (yy)(yyvsp[-1].attr[0]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yyv_X = yy_3_1;
NewTempo(&yy_4_1);
yyv_Tempo = yy_4_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyb + 5;
yy_0_1_2[0] = 2;
yy_0_1_3 = yyv_P;
yy_0_1_4 = yyv_X;
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 166:
#line 5083 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_4;
yy yyv_P;
yy yy_2;
yy yyv_X;
yy yy_3_1;
yy yyv_Tempo;
yy yy_4_1;
yy_2 = (yy)(yyvsp[-1].attr[0]);
yy_3_1 = (yy)(yyvsp[0].attr[1]);
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yyv_P = yy_2;
yyv_X = yy_3_1;
NewTempo(&yy_4_1);
yyv_Tempo = yy_4_1;
yy_0_1_1 = yyv_Tempo;
yy_0_1_2 = yyb + 5;
yy_0_1_2[0] = 1;
yy_0_1_3 = yyv_P;
yy_0_1_4 = yyv_X;
yy_0_1 = yyb + 0;
yy_0_1[0] = 2;
yy_0_1[1] = ((long)yy_0_1_1);
yy_0_1[2] = ((long)yy_0_1_2);
yy_0_1[3] = ((long)yy_0_1_3);
yy_0_1[4] = ((long)yy_0_1_4);
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-1].attr[0];
}
    break;

  case 167:
#line 5124 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_X;
yy yy_2_1;
yy_2_1 = (yy)(yyvsp[-1].attr[1]);
yyv_X = yy_2_1;
yy_0_1 = yyv_X;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[-2].attr[0];
}
    break;

  case 168:
#line 5139 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_I;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_I = yy_1_1;
yy_0_1 = yyv_I;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;

  case 169:
#line 5154 "gen.y"
    {
yy yyb;
yy yy_0_1;
yy yyv_I;
yy yy_1_1;
yy_1_1 = (yy)(yyvsp[0].attr[1]);
yyv_I = yy_1_1;
yy_0_1 = yyv_I;
yyval.attr[1] = ((long)yy_0_1);
yyval.attr[0] = yyvsp[0].attr[0];
}
    break;


    }

/* Line 999 of yacc.c.  */
#line 6291 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}




