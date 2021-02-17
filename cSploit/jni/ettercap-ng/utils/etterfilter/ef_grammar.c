/* A Bison parser, made by GNU Bison 1.875d.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

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
     TOKEN_EOL = 258,
     TOKEN_CONST = 259,
     TOKEN_OFFSET = 260,
     TOKEN_STRING = 261,
     TOKEN_FUNCTION = 262,
     TOKEN_IF = 263,
     TOKEN_ELSE = 264,
     TOKEN_OP_AND = 265,
     TOKEN_OP_OR = 266,
     TOKEN_OP_ASSIGN = 267,
     TOKEN_OP_INC = 268,
     TOKEN_OP_DEC = 269,
     TOKEN_OP_CMP_NEQ = 270,
     TOKEN_OP_CMP_EQ = 271,
     TOKEN_OP_CMP_LT = 272,
     TOKEN_OP_CMP_GT = 273,
     TOKEN_OP_CMP_LEQ = 274,
     TOKEN_OP_CMP_GEQ = 275,
     TOKEN_OP_END = 276,
     TOKEN_PAR_OPEN = 277,
     TOKEN_PAR_CLOSE = 278,
     TOKEN_BLK_BEGIN = 279,
     TOKEN_BLK_END = 280,
     TOKEN_UNKNOWN = 281,
     TOKEN_OP_ADD = 282,
     TOKEN_OP_SUB = 283,
     TOKEN_OP_DIV = 284,
     TOKEN_OP_MUL = 285,
     TOKEN_UMINUS = 286,
     TOKET_OP_AND = 287,
     TOKET_OP_OR = 288,
     TOKET_OP_NOT = 289
   };
#endif
#define TOKEN_EOL 258
#define TOKEN_CONST 259
#define TOKEN_OFFSET 260
#define TOKEN_STRING 261
#define TOKEN_FUNCTION 262
#define TOKEN_IF 263
#define TOKEN_ELSE 264
#define TOKEN_OP_AND 265
#define TOKEN_OP_OR 266
#define TOKEN_OP_ASSIGN 267
#define TOKEN_OP_INC 268
#define TOKEN_OP_DEC 269
#define TOKEN_OP_CMP_NEQ 270
#define TOKEN_OP_CMP_EQ 271
#define TOKEN_OP_CMP_LT 272
#define TOKEN_OP_CMP_GT 273
#define TOKEN_OP_CMP_LEQ 274
#define TOKEN_OP_CMP_GEQ 275
#define TOKEN_OP_END 276
#define TOKEN_PAR_OPEN 277
#define TOKEN_PAR_CLOSE 278
#define TOKEN_BLK_BEGIN 279
#define TOKEN_BLK_END 280
#define TOKEN_UNKNOWN 281
#define TOKEN_OP_ADD 282
#define TOKEN_OP_SUB 283
#define TOKEN_OP_DIV 284
#define TOKEN_OP_MUL 285
#define TOKEN_UMINUS 286
#define TOKET_OP_AND 287
#define TOKET_OP_OR 288
#define TOKET_OP_NOT 289




/* Copy the first part of user declarations.  */
#line 23 "ef_grammar.y"


#include <ef.h>
#include <ef_functions.h>
#include <ec_filter.h>

#define YYERROR_VERBOSE



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
#line 40 "ef_grammar.y"
typedef union YYSTYPE {
   char *string;     
   struct filter_op fop;
   /* used to create the compiler tree */
   struct block *blk;
   struct instruction *ins;
   struct ifblock *ifb;
   struct condition *cnd;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 164 "ef_grammar.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 176 "ef_grammar.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
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
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
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
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   71

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  35
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  11
/* YYNRULES -- Number of rules. */
#define YYNRULES  36
/* YYNRULES -- Number of states. */
#define YYNSTATES  69

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   289

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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     4,     7,     8,    11,    14,    17,    20,
      22,    26,    30,    34,    38,    46,    58,    60,    64,    68,
      72,    76,    80,    84,    88,    92,    96,   100,   102,   104,
     108,   112,   114,   118,   122,   126,   130
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      36,     0,    -1,    -1,    36,    37,    -1,    -1,    38,    37,
      -1,    40,    37,    -1,    41,    37,    -1,    39,    21,    -1,
       7,    -1,    44,    12,     6,    -1,    44,    12,    45,    -1,
      44,    13,     4,    -1,    44,    14,     4,    -1,     8,    22,
      42,    23,    24,    37,    25,    -1,     8,    22,    42,    23,
      24,    37,    25,     9,    24,    37,    25,    -1,    43,    -1,
      42,    10,    42,    -1,    42,    11,    42,    -1,    44,    16,
       6,    -1,    44,    15,     6,    -1,    44,    16,     4,    -1,
      44,    15,     4,    -1,    44,    17,     4,    -1,    44,    18,
       4,    -1,    44,    19,     4,    -1,    44,    20,     4,    -1,
       7,    -1,     5,    -1,    44,    27,    45,    -1,    44,    28,
      45,    -1,     4,    -1,    45,    27,    45,    -1,    45,    28,
      45,    -1,    45,    30,    45,    -1,    45,    29,    45,    -1,
      28,    45,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   114,   114,   115,   124,   127,   132,   137,   145,   152,
     158,   169,   177,   185,   196,   205,   214,   220,   226,   235,
     246,   257,   265,   273,   281,   289,   297,   305,   313,   318,
     330,   341,   345,   349,   353,   357,   361
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOKEN_EOL", "TOKEN_CONST",
  "TOKEN_OFFSET", "TOKEN_STRING", "TOKEN_FUNCTION", "TOKEN_IF",
  "TOKEN_ELSE", "TOKEN_OP_AND", "TOKEN_OP_OR", "TOKEN_OP_ASSIGN",
  "TOKEN_OP_INC", "TOKEN_OP_DEC", "TOKEN_OP_CMP_NEQ", "TOKEN_OP_CMP_EQ",
  "TOKEN_OP_CMP_LT", "TOKEN_OP_CMP_GT", "TOKEN_OP_CMP_LEQ",
  "TOKEN_OP_CMP_GEQ", "TOKEN_OP_END", "TOKEN_PAR_OPEN", "TOKEN_PAR_CLOSE",
  "TOKEN_BLK_BEGIN", "TOKEN_BLK_END", "TOKEN_UNKNOWN", "TOKEN_OP_ADD",
  "TOKEN_OP_SUB", "TOKEN_OP_DIV", "TOKEN_OP_MUL", "TOKEN_UMINUS",
  "TOKET_OP_AND", "TOKET_OP_OR", "TOKET_OP_NOT", "$accept", "input",
  "block", "single_instruction", "instruction", "if_statement",
  "if_else_statement", "conditions_block", "condition", "offset",
  "math_expr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    35,    36,    36,    37,    37,    37,    37,    38,    39,
      39,    39,    39,    39,    40,    41,    42,    42,    42,    43,
      43,    43,    43,    43,    43,    43,    43,    43,    44,    44,
      44,    45,    45,    45,    45,    45,    45
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     2,     0,     2,     2,     2,     2,     1,
       3,     3,     3,     3,     7,    11,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     1,     3,
       3,     1,     3,     3,     3,     3,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       2,     0,     1,    28,     9,     0,     3,     4,     0,     4,
       4,     0,     0,     5,     8,     6,     7,     0,     0,     0,
       0,     0,    27,     0,    16,     0,    31,    10,     0,    11,
      12,    13,    29,    30,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    36,     0,     0,     0,     0,    17,    18,
       4,    22,    20,    21,    19,    23,    24,    25,    26,    32,
      33,    35,    34,     0,    14,     0,     4,     0,    15
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     1,     6,     7,     8,     9,    10,    23,    24,    11,
      29
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -17
static const yysigned_char yypact[] =
{
     -17,     9,   -17,   -17,   -17,    -9,   -17,    19,    -6,    19,
      19,     8,    46,   -17,   -17,   -17,   -17,     4,    14,    44,
      -3,    -3,   -17,    -4,   -17,    22,   -17,   -17,    -3,    17,
     -17,   -17,    31,    31,    46,    46,    32,    48,    51,    54,
      62,    63,    64,   -17,    -3,    -3,    -3,    -3,    52,    52,
      19,   -17,   -17,   -17,   -17,   -17,   -17,   -17,   -17,    31,
      31,   -17,   -17,    -2,    60,    47,    19,    45,   -17
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -17,   -17,    -7,   -17,   -17,   -17,   -17,    30,   -17,    -1,
     -16
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      13,    26,    15,    16,    32,    33,    34,    35,    26,     2,
      27,    25,    43,    12,     3,    14,     4,     5,    30,    36,
      17,    18,    19,    64,     3,    28,     4,     5,    59,    60,
      61,    62,    28,    25,    25,    20,    21,    37,    38,    39,
      40,    41,    42,    63,    44,    45,    46,    47,    31,    20,
      21,     3,    51,    22,    52,    53,    50,    54,    55,    67,
      46,    47,    34,    35,    48,    49,    56,    57,    58,    65,
      68,    66
};

static const unsigned char yycheck[] =
{
       7,     4,     9,    10,    20,    21,    10,    11,     4,     0,
       6,    12,    28,    22,     5,    21,     7,     8,     4,    23,
      12,    13,    14,    25,     5,    28,     7,     8,    44,    45,
      46,    47,    28,    34,    35,    27,    28,    15,    16,    17,
      18,    19,    20,    50,    27,    28,    29,    30,     4,    27,
      28,     5,     4,     7,     6,     4,    24,     6,     4,    66,
      29,    30,    10,    11,    34,    35,     4,     4,     4,     9,
      25,    24
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    36,     0,     5,     7,     8,    37,    38,    39,    40,
      41,    44,    22,    37,    21,    37,    37,    12,    13,    14,
      27,    28,     7,    42,    43,    44,     4,     6,    28,    45,
       4,     4,    45,    45,    10,    11,    23,    15,    16,    17,
      18,    19,    20,    45,    27,    28,    29,    30,    42,    42,
      24,     4,     6,     4,     6,     4,     4,     4,     4,    45,
      45,    45,    45,    37,    25,     9,    24,    37,    25
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
#define YYERROR		goto yyerrorlab


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
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
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
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
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

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
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
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  register short int *yyssp;

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
	short int *yyss1 = yyss;


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
	short int *yyss1 = yyss;
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
        case 3:
#line 115 "ef_grammar.y"
    { 
         /* 
          * at this point the meta-tree is completed,
          * we only have to link it to the entry point
          */
         compiler_set_root(yyvsp[0].blk);
      }
    break;

  case 4:
#line 124 "ef_grammar.y"
    {
            yyval.blk = NULL;
      }
    break;

  case 5:
#line 127 "ef_grammar.y"
    { 
            ef_debug(2, "\t\t block_add single\n"); 
            yyval.blk = compiler_add_instr(yyvsp[-1].ins, yyvsp[0].blk);
         }
    break;

  case 6:
#line 132 "ef_grammar.y"
    { 
            ef_debug(2, "\t\t block_add if\n"); 
            yyval.blk = compiler_add_ifblk(yyvsp[-1].ifb, yyvsp[0].blk);
         }
    break;

  case 7:
#line 137 "ef_grammar.y"
    { 
            ef_debug(2, "\t\t block_add if_else\n"); 
            yyval.blk = compiler_add_ifblk(yyvsp[-1].ifb, yyvsp[0].blk);
         }
    break;

  case 8:
#line 145 "ef_grammar.y"
    {
            yyval.ins = compiler_create_instruction(&yyvsp[-1].fop);
         }
    break;

  case 9:
#line 152 "ef_grammar.y"
    { 
            ef_debug(1, ".");
            ef_debug(3, "\tfunction\n"); 
            /* functions are encoded by the lexycal analyzer */
         }
    break;

  case 10:
#line 158 "ef_grammar.y"
    { 
            ef_debug(1, "=");
            ef_debug(3, "\tassignment string\n"); 
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_ASSIGN;
            yyval.fop.op.assign.string = strdup(yyvsp[0].fop.op.assign.string);
            yyval.fop.op.assign.slen = yyvsp[0].fop.op.assign.slen;
            /* this is a string */
            yyval.fop.op.assign.size = 0;
         }
    break;

  case 11:
#line 169 "ef_grammar.y"
    { 
            ef_debug(1, "=");
            ef_debug(3, "\tassignment\n"); 
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_ASSIGN;
            yyval.fop.op.assign.value = yyvsp[0].fop.op.assign.value;
         }
    break;

  case 12:
#line 177 "ef_grammar.y"
    { 
            ef_debug(1, "+=");
            ef_debug(3, "\tincrement\n"); 
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_INC;
            yyval.fop.op.assign.value = yyvsp[0].fop.op.assign.value;
         }
    break;

  case 13:
#line 185 "ef_grammar.y"
    { 
            ef_debug(1, "-=");
            ef_debug(3, "\tdecrement\n"); 
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_DEC;
            yyval.fop.op.assign.value = yyvsp[0].fop.op.assign.value;
         }
    break;

  case 14:
#line 196 "ef_grammar.y"
    { 
            ef_debug(1, "#");
            ef_debug(3, "\t\t IF BLOCK\n"); 
            yyval.ifb = compiler_create_ifblock(yyvsp[-4].cnd, yyvsp[-1].blk);
         }
    break;

  case 15:
#line 205 "ef_grammar.y"
    { 
            ef_debug(1, "@");
            ef_debug(3, "\t\t IF ELSE BLOCK\n"); 
            yyval.ifb = compiler_create_ifelseblock(yyvsp[-8].cnd, yyvsp[-5].blk, yyvsp[-1].blk);
         }
    break;

  case 16:
#line 214 "ef_grammar.y"
    {
            ef_debug(1, "?");
            ef_debug(3, "\t\t CONDITION\n"); 
            yyval.cnd = compiler_create_condition(&yyvsp[0].fop);
         }
    break;

  case 17:
#line 220 "ef_grammar.y"
    { 
            ef_debug(1, "&");
            ef_debug(3, "\t\t AND\n"); 
            yyval.cnd = compiler_concat_conditions(yyvsp[-2].cnd, COND_AND, yyvsp[0].cnd);
         }
    break;

  case 18:
#line 226 "ef_grammar.y"
    { 
            ef_debug(1, "|");
            ef_debug(3, "\t\t OR\n"); 
            yyval.cnd = compiler_concat_conditions(yyvsp[-2].cnd, COND_OR, yyvsp[0].cnd);
         }
    break;

  case 19:
#line 235 "ef_grammar.y"
    { 
            ef_debug(4, "\tcondition cmp eq string\n"); 
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_TEST;
            yyval.fop.op.test.op = FTEST_EQ;
            yyval.fop.op.test.string = strdup(yyvsp[0].fop.op.test.string);
            yyval.fop.op.test.slen = yyvsp[0].fop.op.assign.slen;
            /* this is a string */
            yyval.fop.op.test.size = 0;
         }
    break;

  case 20:
#line 246 "ef_grammar.y"
    { 
            ef_debug(4, "\tcondition cmp not eq string\n"); 
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_TEST;
            yyval.fop.op.test.op = FTEST_NEQ;
            yyval.fop.op.test.string = strdup(yyvsp[0].fop.op.test.string);
            yyval.fop.op.test.slen = yyvsp[0].fop.op.assign.slen;
            /* this is a string */
            yyval.fop.op.test.size = 0;
         }
    break;

  case 21:
#line 257 "ef_grammar.y"
    { 
            ef_debug(4, "\tcondition cmp eq\n");
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_TEST;
            yyval.fop.op.test.op = FTEST_EQ;
            yyval.fop.op.test.value = yyvsp[0].fop.op.test.value;
         }
    break;

  case 22:
#line 265 "ef_grammar.y"
    { 
            ef_debug(4, "\tcondition cmp not eq\n");
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_TEST;
            yyval.fop.op.test.op = FTEST_NEQ;
            yyval.fop.op.test.value = yyvsp[0].fop.op.test.value;
         }
    break;

  case 23:
#line 273 "ef_grammar.y"
    { 
            ef_debug(4, "\tcondition cmp lt\n"); 
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_TEST;
            yyval.fop.op.test.op = FTEST_LT;
            yyval.fop.op.test.value = yyvsp[0].fop.op.test.value;
         }
    break;

  case 24:
#line 281 "ef_grammar.y"
    { 
            ef_debug(4, "\tcondition cmp gt\n");
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_TEST;
            yyval.fop.op.test.op = FTEST_GT;
            yyval.fop.op.test.value = yyvsp[0].fop.op.test.value;
         }
    break;

  case 25:
#line 289 "ef_grammar.y"
    { 
            ef_debug(4, "\tcondition cmp leq\n");
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_TEST;
            yyval.fop.op.test.op = FTEST_LEQ;
            yyval.fop.op.test.value = yyvsp[0].fop.op.test.value;
         }
    break;

  case 26:
#line 297 "ef_grammar.y"
    { 
            ef_debug(4, "\tcondition cmp geq\n"); 
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.opcode = FOP_TEST;
            yyval.fop.op.test.op = FTEST_GEQ;
            yyval.fop.op.test.value = yyvsp[0].fop.op.test.value;
         }
    break;

  case 27:
#line 305 "ef_grammar.y"
    { 
            ef_debug(4, "\tcondition func\n"); 
            /* functions are encoded by the lexycal analyzer */
         }
    break;

  case 28:
#line 313 "ef_grammar.y"
    {
            ef_debug(4, "\toffset\n"); 
            memcpy(&yyval.fop, &yyvsp[0].fop, sizeof(struct filter_op));
         }
    break;

  case 29:
#line 318 "ef_grammar.y"
    {
            ef_debug(4, "\toffset add\n"); 
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            /* 
             * we are lying here, but math_expr operates
             * only on values, so we can add it to offset
             */
            yyval.fop.op.test.offset = yyvsp[-2].fop.op.test.offset + yyvsp[0].fop.op.test.value; 
            if (yyval.fop.op.test.offset % yyval.fop.op.test.size)
               WARNING("Unaligned offset");
         }
    break;

  case 30:
#line 330 "ef_grammar.y"
    {
            ef_debug(4, "\toffset sub\n"); 
            memcpy(&yyval.fop, &yyvsp[-2].fop, sizeof(struct filter_op));
            yyval.fop.op.test.offset = yyvsp[-2].fop.op.test.offset - yyvsp[0].fop.op.test.value; 
            if (yyval.fop.op.test.offset % yyval.fop.op.test.size)
               WARNING("Unaligned offset");
         }
    break;

  case 31:
#line 341 "ef_grammar.y"
    { 
            yyval.fop.op.test.value = yyvsp[0].fop.op.test.value; 
         }
    break;

  case 32:
#line 345 "ef_grammar.y"
    { 
            yyval.fop.op.test.value = yyvsp[-2].fop.op.test.value + yyvsp[0].fop.op.test.value;
         }
    break;

  case 33:
#line 349 "ef_grammar.y"
    {
            yyval.fop.op.test.value = yyvsp[-2].fop.op.test.value - yyvsp[0].fop.op.test.value;
         }
    break;

  case 34:
#line 353 "ef_grammar.y"
    {
            yyval.fop.op.test.value = yyvsp[-2].fop.op.test.value * yyvsp[0].fop.op.test.value;
         }
    break;

  case 35:
#line 357 "ef_grammar.y"
    {
            yyval.fop.op.test.value = yyvsp[-2].fop.op.test.value / yyvsp[0].fop.op.test.value;
         }
    break;

  case 36:
#line 361 "ef_grammar.y"
    {
            yyval.fop.op.test.value = -yyvsp[0].fop.op.test.value;
         }
    break;


    }

/* Line 1010 of yacc.c.  */
#line 1448 "ef_grammar.c"

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

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
	  yychar = YYEMPTY;

	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
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
      YYPOPSTACK;
      yystate = *yyssp;
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


#line 366 "ef_grammar.y"


/* 
 * ==========================================
 *                C Code  
 * ==========================================
 */

/*
 * name of the tokens as they should be presented to the user
 */
struct {
   char *name;
   char *string;
} errors_array[] = 
   {
      { "TOKEN_CONST", "integer or ip address" },
      { "TOKEN_OFFSET", "offset" },
      { "TOKEN_FUNCTION", "function" },
      { "TOKEN_STRING", "string" },
      { "TOKEN_IF", "'if'" },
      { "TOKEN_ELSE", "'else'" },
      { "TOKEN_OP_AND", "'&&'" },
      { "TOKEN_OP_OR", "'||'" },
      { "TOKEN_OP_ASSIGN", "'='" },
      { "TOKEN_OP_INC", "'+='" },
      { "TOKEN_OP_DEC", "'-='" },
      { "TOKEN_OP_CMP_NEQ", "'!='" },
      { "TOKEN_OP_CMP_EQ", "'=='" },
      { "TOKEN_OP_CMP_LT", "'<'" },
      { "TOKEN_OP_CMP_GT", "'>'" },
      { "TOKEN_OP_CMP_LEQ", "'<='" },
      { "TOKEN_OP_CMP_GEQ", "'>='" },
      { "TOKEN_OP_END", "';'" },
      { "TOKEN_OP_ADD", "'+'" },
      { "TOKEN_OP_MUL", "'*'" },
      { "TOKEN_OP_DIV", "'/'" },
      { "TOKEN_OP_SUB", "'-'" },
      { "TOKEN_PAR_OPEN", "'('" },
      { "TOKEN_PAR_CLOSE", "')'" },
      { "TOKEN_BLK_BEGIN", "'{'" },
      { "TOKEN_BLK_END", "'}'" },
      { "$end", "end of file" },
      { NULL, NULL }
   };

/*
 * This function is needed by bison. so it MUST exist.
 * It is the error handler.
 */
int yyerror(char *s)  
{ 
   char *error;
   int i = 0;

   /* make a copy to manipulate it */
   error = strdup(s);

   /* subsitute the error code with frendly messages */
   do {
      str_replace(&error, errors_array[i].name, errors_array[i].string);
   } while(errors_array[++i].name != NULL);

   /* special case for UNKNOWN */
   if (strstr(error, "TOKEN_UNKNOWN")) {
      str_replace(&error, "TOKEN_UNKNOWN", "'TOKEN_UNKNOWN'");
      str_replace(&error, "TOKEN_UNKNOWN", yylval.string);
   }
 
   /* print the actual error message */
   SCRIPT_ERROR("%s", error);

   SAFE_FREE(error);

   /* return the error */
   return 1;
}

/* EOF */

// vim:ts=3:expandtab


