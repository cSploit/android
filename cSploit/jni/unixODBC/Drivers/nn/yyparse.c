
/*  A Bison parser, made from yyparse.y with Bison version GNU Bison version 1.22
  */

#define YYBISON 1  /* Identify Bison output.  */

#define kwd_select	258
#define kwd_all 259
#define kwd_news	260
#define kwd_xnews	261
#define kwd_distinct	262
#define kwd_count	263
#define kwd_from	264
#define kwd_where	265
#define kwd_in	266
#define kwd_between	267
#define kwd_like	268
#define kwd_escape	269
#define kwd_group	270
#define kwd_by	271
#define kwd_having	272
#define kwd_order	273
#define kwd_for 274
#define kwd_insert	275
#define kwd_into	276
#define kwd_values	277
#define kwd_delete	278
#define kwd_update	279
#define kwd_create	280
#define kwd_alter	281
#define kwd_drop	282
#define kwd_table	283
#define kwd_column	284
#define kwd_view	285
#define kwd_index	286
#define kwd_of	287
#define kwd_current	288
#define kwd_grant	289
#define kwd_revoke	290
#define kwd_is	291
#define kwd_null	292
#define kwd_call	293
#define kwd_uncase	294
#define kwd_case	295
#define kwd_fn	296
#define kwd_d	297
#define QSTRING 298
#define NUM	299
#define NAME	300
#define PARAM	301
#define kwd_or	302
#define kwd_and 303
#define kwd_not 304
#define CMPOP	305

#line 1 "yyparse.y"

/** source of nntp odbc sql parser -- yyparse.y

    Copyright (C) 1995, 1996 by Ke Jin <kejin@visigenic.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
**/

static	char	sccsid[]
	= "@(#)SQL parser for NNSQL(NetNews SQL), Copyright(c) 1995, 1996 by Ke Jin";

#include <config.h>
#include	<nnconfig.h>

#include	<nncol.h>
#include	<yyenv.h>
#include	<yystmt.h>
#include	<yylex.h>
#include	<yyerr.h>
#include	<nndate.h>

# ifdef YYLSP_NEEDED
#  undef YYLSP_NEEDED
# endif

#ifdef	YYBISON
# define yylex(pyylval) 	nnsql_yylex(pyylval, pyyenv)
#else
# define yylex()		nnsql_yylex(&yylval, pyyenv)
#endif

#define yyparse()		nnsql_yyparse	(yyenv_t* pyyenv)
#define yyerror(msg)		nnsql_yyerror (pyyenv, msg)
#define SETYYERROR(env, code)	{ env->pstmt->errcode = code; \
				  env->pstmt->errpos = env->errpos;}

typedef struct
{
	char*	schema_tab_name;
	char*	column_name;
} column_name_t;

static void	unpack_col_name(char* schema_tab_column_name, column_name_t* ptr);
static void*	add_node(yystmt_t* pstmt, node_t* node);
static void	srchtree_reloc(node_t* srchtree, int num);
static int	add_attr(yystmt_t* pstmt, int idx, int wstat);
static void*	add_all_attr(yystmt_t* pstmt);
static void*	add_news_attr(yystmt_t* pstmt);
static void*	add_xnews_attr(yystmt_t* pstmt);
static void*	add_column(yystmt_t* pstmt, yycol_t* pcol);
static void	nnsql_yyerror(yyenv_t* pyyenv, char* msg);
static int	table_check(yystmt_t* pstmt);
static int	column_name(yystmt_t* pstmt, char* name);
static void*	attr_name(yystmt_t* pstmt, char* name);
static int	add_ins_head(yystmt_t* pstmt, char* head, int idx);
static int	add_ins_value(yystmt_t* pstmt, node_t node, int idx);
static char*	get_unpacked_attrname(yystmt_t* pstmt, char* name);

#define 	ERROR_PTR		((void*)(-1L))
#define 	EMPTY_PTR		ERROR_PTR


#line 74 "yyparse.y"
typedef union {
	char*	qstring;
	char*	name;
	long	number;

	int	ipar;		/* parameter index */
	int	cmpop;		/* for comparsion operators */
	char	esc;
	int	flag;		/* for not_opt and case_opt */
	int	idx;

	void*	offset; 	/* actually, it is used as a 'int' offset */

	node_t node;		/* a node haven't add to tree */
} YYSTYPE;

#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define YYFINAL 	219
#define YYFLAG		-32768
#define YYNTBASE	59

#define YYTRANSLATE(x) ((unsigned)(x) <= 305 ? yytranslate[x] : 92)

static const char yytranslate[] = {	0,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,    53,
    54,    52,	   2,	 57,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,    51,     2,
    58,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	  55,	  2,	56,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	2,     2,     2,     2,     2,
     2,     2,	   2,	  2,	 2,	1,     2,     3,     4,     5,
     6,     7,	   8,	  9,	10,    11,    12,    13,    14,    15,
    16,    17,	  18,	 19,	20,    21,    22,    23,    24,    25,
    26,    27,	  28,	 29,	30,    31,    32,    33,    34,    35,
    36,    37,	  38,	 39,	40,    41,    42,    43,    44,    45,
    46,    47,	  48,	 49,	50
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     3,	   4,	 11,	13,    15,    17,    19,    25,    26,
    31,    32,	  34,	 36,	37,    39,    41,    43,    45,    47,
    51,    58,	  60,	 64,	71,    73,    77,    80,    82,    87,
    95,    97,	  99,	104,   106,   107,   110,   114,   118,   122,
   126,   128,	 132,	134,   139,   147,   149,   150,   153,   157,
   160,   164,	 168,	175,   182,   189,   194,   198,   199,   201,
   203,   206,	 208,	213,   221,   223,   227,   229,   231,   233,
   235,   240,	 241,	244,   249,   251,   253,   254,   256,   257,
   261,   262,	 265,	266,   270,   281,   283,   287,   289,   294,
   302,   304,	 308,	310,   312,   314,   319,   327,   329,   331,
   333,   335,	 338,	343,   345
};

static const short yyrhs[] = {	  60,
    51,     0,	   0,	  3,	63,    64,     9,    70,    61,     0,
    84,     0,	  89,	  0,	90,	0,    91,     0,    72,    81,
    82,    83,	  62,	  0,	 0,    19,    24,    32,    67,     0,
     0,     4,	   0,	  7,	 0,	0,    52,     0,    65,     0,
    66,     0,	  67,	  0,	 5,	0,     5,    53,    54,     0,
    55,    41,	   5,	 53,	54,    56,     0,     6,     0,     6,
    53,    54,	   0,	 55,	41,	6,    53,    54,    56,     0,
    68,     0,	  67,	 57,	68,	0,     8,    69,     0,    45,
     0,    29,	  53,	 43,	54,	0,    55,    41,    29,    53,
    43,    54,	  56,	  0,	43,	0,    44,     0,    55,    42,
    43,    56,	   0,	 46,	 0,	0,    53,    54,     0,    53,
    52,    54,	   0,	 53,	45,    54,     0,    53,    43,    54,
     0,    53,	  44,	 54,	 0,    71,     0,    70,    57,    71,
     0,    45,	   0,	 28,	53,    43,    54,     0,    55,    41,
    28,    53,	  43,	 54,	56,	0,    46,     0,     0,    10,
    73,     0,	  53,	 73,	54,	0,    49,    73,     0,    73,
    47,    73,	   0,	 73,	48,    73,     0,    75,    80,    11,
    53,    76,	  54,	  0,	75,    80,    12,    77,    48,    77,
     0,    75,	  80,	 74,	13,    79,    78,     0,    75,    36,
    80,    37,	   0,	 75,	50,    77,     0,     0,    40,     0,
    39,     0,	   4,	 40,	 0,    45,     0,    29,    53,    43,
    54,     0,	  55,	 41,	29,    53,    43,    54,    56,     0,
    77,     0,	  76,	 57,	77,	0,    37,     0,    46,     0,
    43,     0,	  44,	  0,	55,    42,    43,    56,     0,     0,
    14,    43,	   0,	 55,	14,    43,    56,     0,    46,     0,
    43,     0,	   0,	 49,	 0,	0,    15,    16,    67,     0,
     0,    17,	  73,	  0,	 0,    18,    16,    67,     0,    20,
    21,    71,	  53,	 85,	54,    22,    53,    87,    54,     0,
    86,     0,	  85,	 57,	86,	0,    45,     0,    29,    53,
    43,    54,	   0,	 55,	41,    29,    53,    43,    54,    56,
     0,    88,	   0,	 87,	57,    88,     0,    37,     0,    43,
     0,    46,	   0,	 23,	 9,    71,    72,     0,    23,     9,
    71,    10,	  33,	 32,	45,	0,    25,     0,    26,     0,
    27,     0,	  24,	  0,	55,    38,     0,    55,    46,    58,
    38,     0,	  34,	  0,	35,	0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   162,   166,	 167,	177,   181,   185,   186,   194,   198,   199,
   207,   208,	 209,	217,   218,   219,   220,   221,   225,   226,
   227,   231,	 232,	233,   237,   238,   242,   253,   255,   257,
   259,   270,	 281,	294,   299,   300,   301,   302,   303,   304,
   308,   309,	 322,	324,   326,   328,   336,   340,   354,   355,
   367,   380,	 392,	417,   447,   476,   501,   516,   518,   520,
   522,   527,	 529,	531,   536,   541,   555,   565,   578,   589,
   600,   616,	 617,	618,   622,   636,   651,   652,   656,   657,
   665,   666,	 674,	675,   683,   733,   738,   746,   748,   750,
   755,   760,	 768,	772,   777,   785,   797,   805,   806,   807,
   808,   809,	 810,	812,   813
};

static const char * const yytname[] = {   "$","error","$illegal.","kwd_select",
"kwd_all","kwd_news","kwd_xnews","kwd_distinct","kwd_count","kwd_from","kwd_where",
"kwd_in","kwd_between","kwd_like","kwd_escape","kwd_group","kwd_by","kwd_having",
"kwd_order","kwd_for","kwd_insert","kwd_into","kwd_values","kwd_delete","kwd_update",
"kwd_create","kwd_alter","kwd_drop","kwd_table","kwd_column","kwd_view","kwd_index",
"kwd_of","kwd_current","kwd_grant","kwd_revoke","kwd_is","kwd_null","kwd_call",
"kwd_uncase","kwd_case","kwd_fn","kwd_d","QSTRING","NUM","NAME","PARAM","kwd_or",
"kwd_and","kwd_not","CMPOP","';'","'*'","'('","')'","'{'","'}'","','","'='",
"sql_stmt","stmt_body","select_clauses","for_stmt","distinct_opt","select_list",
"news_hotlist","news_xhotlist","col_name_list","col_name","count_sub_opt","tab_list",
"tab_name","where_clause","search_condition","case_opt","attr_name","value_list",
"value","escape_desc","pattern","not_opt","group_clause","having_clause","order_clause",
"insert_stmt","ins_head_list","ins_head","ins_value_list","ins_value","srch_delete_stmt",
"posi_delete_stmt","other_stmt",""
};
#endif

static const short yyr1[] = {	  0,
    59,    60,	  60,	 60,	60,    60,    60,    61,    62,    62,
    63,    63,	  63,	 64,	64,    64,    64,    64,    65,    65,
    65,    66,	  66,	 66,	67,    67,    68,    68,    68,    68,
    68,    68,	  68,	 68,	69,    69,    69,    69,    69,    69,
    70,    70,	  71,	 71,	71,    71,    72,    72,    73,    73,
    73,    73,	  73,	 73,	73,    73,    73,    74,    74,    74,
    74,    75,	  75,	 75,	76,    76,    77,    77,    77,    77,
    77,    78,	  78,	 78,	79,    79,    80,    80,    81,    81,
    82,    82,	  83,	 83,	84,    85,    85,    86,    86,    86,
    87,    87,	  88,	 88,	88,    89,    90,    91,    91,    91,
    91,    91,	  91,	 91,	91
};

static const short yyr2[] = {	  0,
     2,     0,	   6,	  1,	 1,	1,     1,     5,     0,     4,
     0,     1,	   1,	  0,	 1,	1,     1,     1,     1,     3,
     6,     1,	   3,	  6,	 1,	3,     2,     1,     4,     7,
     1,     1,	   4,	  1,	 0,	2,     3,     3,     3,     3,
     1,     3,	   1,	  4,	 7,	1,     0,     2,     3,     2,
     3,     3,	   6,	  6,	 6,	4,     3,     0,     1,     1,
     2,     1,	   4,	  7,	 1,	3,     1,     1,     1,     1,
     4,     0,	   2,	  4,	 1,	1,     0,     1,     0,     3,
     0,     2,	   0,	  3,	10,	1,     3,     1,     4,     7,
     1,     3,	   1,	  1,	 1,	4,     7,     1,     1,     1,
     1,     2,	   4,	  1,	 1
};

static const short yydefact[] = {     2,
    11,     0,	   0,	101,	98,    99,   100,   104,   105,     0,
     0,     4,	   5,	  6,	 7,    12,    13,    14,     0,     0,
   102,     0,	   1,	 19,	22,    35,     0,    31,    32,    28,
    34,    15,	   0,	  0,	16,    17,    18,    25,     0,    43,
    46,     0,	   0,	 47,	 0,	0,     0,     0,    27,     0,
     0,     0,	   0,	  0,	 0,	0,     0,     0,    96,   103,
    20,    23,	   0,	  0,	 0,	0,    36,     0,     0,     0,
     0,     0,	  47,	 41,	 0,    26,     0,     0,     0,    88,
     0,     0,	  86,	  0,	 0,    62,     0,     0,     0,    48,
    77,    39,	  40,	 38,	37,    29,     0,     0,     0,    33,
     0,     0,	   3,	 79,	 0,    44,     0,     0,     0,     0,
     0,     0,	   0,	 50,	 0,	0,     0,     0,    77,    78,
     0,    58,	   0,	  0,	 0,    42,     0,    81,     0,     0,
     0,     0,	  87,	  0,	97,    49,     0,    51,    52,     0,
    67,    69,	  70,	 68,	 0,    57,     0,     0,     0,    60,
    59,     0,	  21,	 24,	 0,	0,     0,    83,     0,    89,
     0,     0,	  63,	  0,	56,	0,    61,     0,     0,     0,
    30,    80,	  82,	  0,	 9,    45,     0,    93,    94,    95,
     0,    91,	   0,	  0,	 0,    65,     0,    76,    75,    72,
     0,     0,	   8,	  0,	85,	0,     0,    71,    53,     0,
    54,     0,	   0,	 55,	84,	0,    90,    92,    64,    66,
    73,     0,	   0,	  0,	10,    74,     0,     0,     0
};

static const short yydefgoto[] = {   217,
    11,   103,	 193,	 18,	34,    35,    36,    37,    38,    49,
    73,    43,	  59,	 90,   152,    91,   185,   146,   204,   190,
   122,   128,	 158,	175,	12,    82,    83,   181,   182,    13,
    14,    15
};

static const short yypact[] = {     1,
     7,   -14,	  25,-32768,-32768,-32768,-32768,-32768,-32768,   -26,
   -38,-32768,-32768,-32768,-32768,-32768,-32768,    34,    21,    21,
-32768,   -39,-32768,	-23,	45,    47,    53,-32768,-32768,-32768,
-32768,-32768,	  28,	102,-32768,-32768,    56,-32768,    62,-32768,
-32768,    77,	  67,	111,	84,    69,    70,    10,-32768,    82,
    12,    83,	  21,	 29,	86,    99,    54,    52,-32768,-32768,
-32768,-32768,	  76,	 78,	79,    80,-32768,    81,    85,    87,
    88,    75,	  -5,-32768,	61,-32768,    89,    91,    92,-32768,
    95,    33,-32768,	 93,   105,-32768,    59,    59,    98,    -2,
   -27,-32768,-32768,-32768,-32768,-32768,    94,    96,   104,-32768,
    59,    21,-32768,	127,   120,-32768,   108,   109,   124,   132,
    54,   112,	 113,-32768,	 3,   128,    59,    59,   107,-32768,
    73,     4,	 103,	106,   110,-32768,   144,   146,   114,   115,
   117,   118,-32768,	119,-32768,-32768,   121,   129,-32768,   130,
-32768,-32768,-32768,-32768,   123,-32768,   126,   122,    73,-32768,
-32768,   148,-32768,-32768,   116,    29,    59,   158,   125,-32768,
   135,    22,-32768,	136,-32768,   137,-32768,    73,   134,    48,
-32768,    56,	  -2,	167,   165,-32768,   131,-32768,-32768,-32768,
    38,-32768,	 133,	138,	39,-32768,    73,-32768,-32768,    -8,
    29,   162,-32768,	139,-32768,    22,   140,-32768,-32768,    73,
-32768,   145,	 175,-32768,	56,   159,-32768,-32768,-32768,-32768,
-32768,   147,	  29,	141,	56,-32768,   192,   193,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,  -153,   149,-32768,
-32768,   -20,	 142,	-86,-32768,-32768,-32768,  -139,-32768,-32768,
    90,-32768,-32768,-32768,-32768,-32768,    97,-32768,     2,-32768,
-32768,-32768
};


#define YYLAST		215


static const short yytable[] = {    44,
   114,   115,	 172,	  1,   101,   202,    19,   147,   119,   169,
    16,    21,	  23,	 17,   148,   149,    69,    70,    45,    22,
     2,   120,	 121,	  3,	 4,	5,     6,     7,   186,    46,
   138,   139,	  74,	 20,	 8,	9,    26,   205,    24,    25,
    71,    26,	 150,	151,   117,   118,   203,   201,    39,   117,
   118,   102,	  63,	 64,	65,    10,   136,    27,   178,   215,
   210,    66,	  27,	 67,   179,    40,    41,   180,    51,    52,
   173,    28,	  29,	 30,	31,    42,    28,    29,    30,    31,
    84,   126,	  79,	 75,	85,    32,   110,    84,    33,   111,
   188,   195,	 199,	189,   196,   200,    86,    47,    80,    48,
    87,   105,	  52,	 86,	88,    50,    89,    87,    81,   141,
    53,    88,	  54,	 89,	55,   142,   143,    56,   144,    57,
    58,    60,	  61,	 62,	68,    72,    78,   145,    77,    92,
   100,    93,	  94,	 95,	96,   109,   113,    97,   116,    98,
    99,   127,	 106,	107,   108,   112,   125,   123,    71,   124,
   129,   130,	 131,	132,   134,   120,   137,   135,   153,   156,
   170,   154,	 157,	155,   166,   167,   165,   159,   160,   161,
   162,   171,	 163,	164,   168,   174,   118,   177,   183,   184,
   176,   187,	 191,	192,   194,   206,   197,   211,   212,   214,
   213,   218,	 219,	198,   207,   209,   216,   208,     0,     0,
     0,     0,	  76,	  0,	 0,	0,     0,   133,   140,     0,
     0,     0,	   0,	  0,   104
};

static const short yycheck[] = {    20,
    87,    88,	 156,	  3,	10,    14,    21,     4,    36,   149,
     4,    38,	  51,	  7,	11,    12,     5,     6,    58,    46,
    20,    49,	  50,	 23,	24,    25,    26,    27,   168,    53,
   117,   118,	  53,	  9,	34,    35,     8,   191,     5,     6,
    29,     8,	  39,	 40,	47,    48,    55,   187,    28,    47,
    48,    57,	  43,	 44,	45,    55,    54,    29,    37,   213,
   200,    52,	  29,	 54,	43,    45,    46,    46,    41,    42,
   157,    43,	  44,	 45,	46,    55,    43,    44,    45,    46,
    29,   102,	  29,	 55,	33,    52,    54,    29,    55,    57,
    43,    54,	  54,	 46,	57,    57,    45,    53,    45,    53,
    49,    41,	  42,	 45,	53,    53,    55,    49,    55,    37,
     9,    53,	  57,	 55,	53,    43,    44,    41,    46,    53,
    10,    38,	  54,	 54,	43,    43,    28,    55,    43,    54,
    56,    54,	  54,	 54,	54,    41,    32,    53,    41,    53,
    53,    15,	  54,	 53,	53,    53,    43,    54,    29,    54,
    43,    43,	  29,	 22,	43,    49,    29,    45,    56,    16,
    13,    56,	  17,	 54,	42,    40,    37,    54,    54,    53,
    53,    56,	  54,	 53,	53,    18,    48,    43,    43,    43,
    56,    48,	  16,	 19,	54,    24,    54,    43,    14,    43,
    32,     0,	   0,	 56,	56,    56,    56,   196,    -1,    -1,
    -1,    -1,	  54,	 -1,	-1,    -1,    -1,   111,   119,    -1,
    -1,    -1,	  -1,	 -1,	73
};
#define YYPURE 1

/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Bob Corbett and Richard Stallman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok 	(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY 	-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR 	goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()	(!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#define YYLEX		yylex(&yylval, &yylloc)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar; 		/*  the lookahead symbol		*/
YYSTYPE yylval; 		/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc; 		/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far	*/
#endif	/* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse ();
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_bcopy(FROM,TO,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.	*/
static void
__yy_bcopy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.	*/
static void
__yy_bcopy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 184 "/usr/lib/bison.simple"
int
yyparse()
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return 	*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in	yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.	*/
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.	*/
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_bcopy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_bcopy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_bcopy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.	*/
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 162 "yyparse.y"
{ YYACCEPT; ;
    break;}
case 3:
#line 168 "yyparse.y"
{
		if( ! table_check( pyyenv->pstmt ) )
		{
			SETYYERROR(pyyenv, NOT_SUPPORT_MULTITABLE_QUERY);
			YYABORT;
		}

		pyyenv->pstmt->type = en_stmt_select;
	  ;
    break;}
case 4:
#line 178 "yyparse.y"
{
		pyyenv->pstmt->type = en_stmt_insert;
	  ;
    break;}
case 5:
#line 182 "yyparse.y"
{
		pyyenv->pstmt->type = en_stmt_srch_delete;
	  ;
    break;}
case 7:
#line 187 "yyparse.y"
{
		pyyenv->pstmt->type = en_stmt_alloc;
		YYABORT;
	  ;
    break;}
case 10:
#line 200 "yyparse.y"
{
		SETYYERROR(pyyenv, NOT_SUPPORT_UPDATEABLE_CURSOR) ;
		YYABORT;
	  ;
    break;}
case 13:
#line 210 "yyparse.y"
{
		SETYYERROR(pyyenv, NOT_SUPPORT_DISTINCT_SELECT);
		YYABORT;
	  ;
    break;}
case 14:
#line 217 "yyparse.y"
{ if(add_all_attr(pyyenv->pstmt)) YYABORT; ;
    break;}
case 15:
#line 218 "yyparse.y"
{ if(add_all_attr(pyyenv->pstmt)) YYABORT; ;
    break;}
case 16:
#line 219 "yyparse.y"
{ if(add_news_attr(pyyenv->pstmt)) YYABORT; ;
    break;}
case 17:
#line 220 "yyparse.y"
{ if(add_xnews_attr(pyyenv->pstmt)) YYABORT; ;
    break;}
case 18:
#line 221 "yyparse.y"
{ if(add_attr(pyyenv->pstmt, 0, 0))  YYABORT; ;
    break;}
case 27:
#line 243 "yyparse.y"
{
		yycol_t col;

		col.iattr = en_sql_count;
		col.table = 0;

		if( add_column(pyyenv->pstmt, &col) )
			YYABORT;

	  ;
    break;}
case 28:
#line 254 "yyparse.y"
{ if( column_name( pyyenv->pstmt, yyvsp[0].name ) ) YYABORT; ;
    break;}
case 29:
#line 256 "yyparse.y"
{ if( column_name( pyyenv->pstmt, yyvsp[-1].qstring ) ) YYABORT; ;
    break;}
case 30:
#line 258 "yyparse.y"
{ if( column_name( pyyenv->pstmt, yyvsp[-2].qstring ) ) YYABORT; ;
    break;}
case 31:
#line 260 "yyparse.y"
{
		yycol_t col;

		col.iattr = en_sql_qstr;
		col.value.qstr = yyvsp[0].qstring;
		col.table = 0;

		if( add_column(pyyenv->pstmt, &col) )
			YYABORT;
	  ;
    break;}
case 32:
#line 271 "yyparse.y"
{
		yycol_t col;

		col.iattr = en_sql_num;
		col.value.num = yyvsp[0].number;
		col.table = 0;

		if( add_column(pyyenv->pstmt, &col) )
			YYABORT;
	  ;
    break;}
case 33:
#line 282 "yyparse.y"
{
		yycol_t col;

		col.iattr = en_sql_date;
		if( nnsql_odbcdatestr2date(yyvsp[-1].qstring, &(col.value.date)) )
			YYABORT;

		col.table = 0;

		if( add_column(pyyenv->pstmt, &col) )
			YYABORT;
	  ;
    break;}
case 34:
#line 295 "yyparse.y"
{	SETYYERROR(pyyenv, VARIABLE_IN_SELECT_LIST); YYABORT; ;
    break;}
case 41:
#line 308 "yyparse.y"
{ pyyenv->pstmt->table = yyvsp[0].name; ;
    break;}
case 42:
#line 310 "yyparse.y"
{
		if( ! pyyenv->pstmt->table )
			pyyenv->pstmt->table = yyvsp[0].name;
		else if( !STREQ(pyyenv->pstmt->table, yyvsp[0].name) )
		{
			SETYYERROR(pyyenv, NOT_SUPPORT_MULTITABLE_QUERY);
			YYABORT;
		}
	  ;
    break;}
case 43:
#line 323 "yyparse.y"
{ yyval.name = yyvsp[0].name; ;
    break;}
case 44:
#line 325 "yyparse.y"
{ yyval.name = yyvsp[-1].qstring; ;
    break;}
case 45:
#line 327 "yyparse.y"
{ yyval.name = yyvsp[-2].qstring; ;
    break;}
case 46:
#line 329 "yyparse.y"
{
		SETYYERROR(pyyenv, VARIABLE_IN_TABLE_LIST);
		YYABORT;
	  ;
    break;}
case 47:
#line 337 "yyparse.y"
{
		pyyenv->pstmt->srchtree = 0;
	  ;
    break;}
case 48:
#line 341 "yyparse.y"
{
		yystmt_t*	pstmt;
		int		offset;

		pstmt = pyyenv->pstmt;
		offset = (int)(yyvsp[0].offset);

		pstmt->srchtree = pstmt->node_buf + offset;
		srchtree_reloc (	pstmt->node_buf, pstmt->srchtreenum);
	  ;
    break;}
case 49:
#line 354 "yyparse.y"
{ yyval.offset = yyvsp[-1].offset; ;
    break;}
case 50:
#line 356 "yyparse.y"
{
		node_t	node;

		node.type = en_nt_logop;
		node.value.logop = en_logop_not;
		node.left = EMPTY_PTR;
		node.right= yyvsp[0].offset;

		if((yyval.offset = add_node(pyyenv->pstmt, &node)) == ERROR_PTR )
			YYABORT;
	  ;
    break;}
case 51:
#line 368 "yyparse.y"
{
		node_t	node;
		void*		p;

		node.type = en_nt_logop;
		node.value.logop = en_logop_or;
		node.left = yyvsp[-2].offset;
		node.right= yyvsp[0].offset;

		if((yyval.offset = add_node(pyyenv->pstmt, &node)) == ERROR_PTR )
			YYABORT;
	  ;
    break;}
case 52:
#line 381 "yyparse.y"
{
		node_t	node;

		node.type = en_nt_logop;
		node.value.logop = en_logop_and;
		node.left = yyvsp[-2].offset;
		node.right= yyvsp[0].offset;

		if((yyval.offset = add_node(pyyenv->pstmt, &node)) == ERROR_PTR )
			YYABORT;
	  ;
    break;}
case 53:
#line 393 "yyparse.y"
{
		node_t	node;
		void*		ptr;

		node.type = en_nt_in;
		node.left = yyvsp[-5].offset;
		node.right= yyvsp[-1].offset;

		if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
			YYABORT;

		if( yyvsp[-4].flag )
		{
			node.type = en_nt_logop;
			node.value.logop = en_logop_not;
			node.left = EMPTY_PTR;
			node.right = ptr;

			if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
				YYABORT;
		}

		yyval.offset = ptr;
	  ;
    break;}
case 54:
#line 418 "yyparse.y"
{
		node_t	node;
		void*		ptr;

		if( ((node.left = add_node(pyyenv->pstmt, &(yyvsp[-2].node))) == ERROR_PTR)
		 || ((node.right= add_node(pyyenv->pstmt, &(yyvsp[0].node))) == ERROR_PTR)
		 || ((ptr	= add_node(pyyenv->pstmt, &node)) == ERROR_PTR) )
			YYABORT;

		node.type = en_nt_between;
		node.left = yyvsp[-5].offset;
		node.right = ptr;

		if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
			YYABORT;

		if( yyvsp[-4].flag )
		{
			node.type = en_nt_logop;
			node.value.logop = en_logop_not;
			node.left = EMPTY_PTR;
			node.right= ptr;

			if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
				YYABORT;
		}

		yyval.offset = ptr;
	  ;
    break;}
case 55:
#line 448 "yyparse.y"
{
		node_t	node;
		void*		ptr;

		if( yyvsp[-3].flag )
			node.type = en_nt_caselike;
		else
			node.type = en_nt_like;
		node.value.esc = yyvsp[0].esc;
		node.left = yyvsp[-5].offset;
		node.right= yyvsp[-1].offset;

		if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
			YYABORT;

		if( yyvsp[-4].flag )
		{
			node.type = en_nt_logop;
			node.value.logop = en_logop_not;
			node.left = EMPTY_PTR;
			node.right= ptr;

			if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
				YYABORT;
		}

		yyval.offset = ptr;
	  ;
    break;}
case 56:
#line 477 "yyparse.y"
{
		node_t	node;
		void*		ptr;

		node.type = en_nt_isnull;
		node.left = yyvsp[-3].offset;
		node.right= EMPTY_PTR;

		if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
			YYABORT;

		if( yyvsp[-1].flag )
		{
			node.type = en_nt_logop;
			node.value.logop = en_logop_not;
			node.left = EMPTY_PTR;
			node.right= ptr;

			if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
				YYABORT;
		}

		yyval.offset = ptr;
	  ;
    break;}
case 57:
#line 502 "yyparse.y"
{
		node_t	node;

		node.type = en_nt_cmpop;
		node.value.cmpop = yyvsp[-1].cmpop;
		node.left = yyvsp[-2].offset;

		if( ((node.right= add_node(pyyenv->pstmt, &(yyvsp[0].node))) == ERROR_PTR)
		 || ((yyval.offset	  = add_node(pyyenv->pstmt, &node)) == ERROR_PTR) )
			YYABORT;
	  ;
    break;}
case 58:
#line 517 "yyparse.y"
{ yyval.flag = 0; ;
    break;}
case 59:
#line 519 "yyparse.y"
{ yyval.flag = 0; ;
    break;}
case 60:
#line 521 "yyparse.y"
{ yyval.flag = 1; ;
    break;}
case 61:
#line 523 "yyparse.y"
{ yyval.flag = 1; ;
    break;}
case 62:
#line 528 "yyparse.y"
{ yyval.offset = attr_name( pyyenv->pstmt, yyvsp[0].name ); if( yyval.offset == ERROR_PTR ) YYABORT; ;
    break;}
case 63:
#line 530 "yyparse.y"
{ yyval.offset = attr_name( pyyenv->pstmt, yyvsp[-1].qstring ); if( yyval.offset == ERROR_PTR ) YYABORT; ;
    break;}
case 64:
#line 532 "yyparse.y"
{ yyval.offset = attr_name( pyyenv->pstmt, yyvsp[-2].qstring ); if( yyval.offset == ERROR_PTR ) YYABORT; ;
    break;}
case 65:
#line 537 "yyparse.y"
{
		if( (yyval.offset = add_node( pyyenv->pstmt, &(yyvsp[0].node))) == ERROR_PTR )
			YYABORT;
	  ;
    break;}
case 66:
#line 542 "yyparse.y"
{
		node_t	node;

		node = yyvsp[0].node;
		node.left = EMPTY_PTR;
		node.right= yyvsp[-2].offset;

		if( (yyval.offset = add_node(pyyenv->pstmt, &node)) == ERROR_PTR )
			YYABORT;
	  ;
    break;}
case 67:
#line 556 "yyparse.y"
{
		node_t	node;

		node.type = en_nt_null;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		yyval.node = node;
	  ;
    break;}
case 68:
#line 566 "yyparse.y"
{
		node_t	node;

		node.type = en_nt_param,
		node.value.ipar = yyvsp[0].ipar;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		pyyenv->pstmt->npar ++;

		yyval.node = node;
	  ;
    break;}
case 69:
#line 579 "yyparse.y"
{
		node_t	node;

		node.type = en_nt_qstr;
		node.value.qstr = yyvsp[0].qstring;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		yyval.node = node;
	  ;
    break;}
case 70:
#line 590 "yyparse.y"
{
		node_t	node;

		node.type = en_nt_num;
		node.value.num = yyvsp[0].number;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		yyval.node = node;
	  ;
    break;}
case 71:
#line 601 "yyparse.y"
{
		node_t	node;

		node.type = en_nt_date;
		if( nnsql_odbcdatestr2date(yyvsp[-1].qstring, &(node.value.date)) )
			YYABORT;

		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		yyval.node = node;
	  ;
    break;}
case 72:
#line 616 "yyparse.y"
{ yyval.esc = 0; ;
    break;}
case 73:
#line 617 "yyparse.y"
{ yyval.esc = yyvsp[0].qstring[0]; ;
    break;}
case 74:
#line 618 "yyparse.y"
{ yyval.esc = yyvsp[-1].qstring[0]; ;
    break;}
case 75:
#line 623 "yyparse.y"
{
		node_t	node;

		node.type = en_nt_param;
		node.value.ipar = yyvsp[0].ipar;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		pyyenv->pstmt->npar ++;

		if((yyval.offset = add_node(pyyenv->pstmt, &node)) == EMPTY_PTR)
			YYABORT;
	  ;
    break;}
case 76:
#line 637 "yyparse.y"
{
		node_t	node;

		node.type = en_nt_qstr;
		node.value.qstr = yyvsp[0].qstring;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		if((yyval.offset = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
			YYABORT;
	  ;
    break;}
case 77:
#line 651 "yyparse.y"
{ yyval.flag = 0; ;
    break;}
case 78:
#line 652 "yyparse.y"
{ yyval.flag = 1; ;
    break;}
case 80:
#line 658 "yyparse.y"
{
		SETYYERROR(pyyenv, NOT_SUPPORT_GROUP_CLAUSE);
		YYABORT;
	  ;
    break;}
case 82:
#line 667 "yyparse.y"
{
		SETYYERROR(pyyenv, NOT_SUPPORT_HAVING_CLAUSE);
		YYABORT;
	  ;
    break;}
case 84:
#line 676 "yyparse.y"
{
		SETYYERROR(pyyenv, NOT_SUPPORT_ORDER_CLAUSE);
		YYABORT;
	  ;
    break;}
case 85:
#line 684 "yyparse.y"
{
		int	i;
		char*	head = 0;

		if( yyvsp[-5].idx > yyvsp[-1].idx )
		{
			SETYYERROR(pyyenv, INSERT_VALUE_LESS);
			YYABORT;
		}

		if( yyvsp[-5].idx < yyvsp[-1].idx )
		{
			SETYYERROR(pyyenv, INSERT_VALUE_MORE);
			YYABORT;
		}

		for(i=0;;i++)
		{
			head = (pyyenv->pstmt->ins_heads)[i];

			if( ! head )
			{
				SETYYERROR(pyyenv, POST_WITHOUT_BODY);
				YYABORT;
			}

			if( nnsql_getcolidxbyname(head) == en_body)
				break;
		}

		if( add_ins_head(pyyenv->pstmt, 0, yyvsp[-5].idx) == -1 )
			YYABORT;

		pyyenv->pstmt->table = yyvsp[-7].name;
		/* we will not check table(i.e. group)name here.
		 * According to RFC1036, it is totally legal to
		 * post an articl to a group which is not access
		 * able locally.
		 * table name here is actually newsgroups name.
		 * it should look like:
		 *	<group_name>,<group_name>,...
		 * Be aware, there are must no space between the
		 * ',' and the two <group_name>s. Also, group
		 * names are case sensitive.
		 */
	  ;
    break;}
case 86:
#line 734 "yyparse.y"
{
		if( (yyval.idx = add_ins_head(pyyenv->pstmt, yyvsp[0].qstring, 0)) == -1)
			YYABORT;
	  ;
    break;}
case 87:
#line 739 "yyparse.y"
{
		if( (yyval.idx = add_ins_head(pyyenv->pstmt, yyvsp[0].qstring, yyvsp[-2].idx))== -1)
			YYABORT;
	  ;
    break;}
case 88:
#line 747 "yyparse.y"
{ yyval.qstring = get_unpacked_attrname(pyyenv->pstmt, yyvsp[0].name); ;
    break;}
case 89:
#line 749 "yyparse.y"
{ yyval.qstring = get_unpacked_attrname(pyyenv->pstmt, yyvsp[-1].qstring); ;
    break;}
case 90:
#line 751 "yyparse.y"
{ yyval.qstring = yyvsp[-2].qstring; ;
    break;}
case 91:
#line 756 "yyparse.y"
{
		if( (yyval.idx = add_ins_value(pyyenv->pstmt, yyvsp[0].node, 0))== -1)
			YYABORT;
	  ;
    break;}
case 92:
#line 761 "yyparse.y"
{
		if( (yyval.idx = add_ins_value(pyyenv->pstmt, yyvsp[0].node, yyvsp[-2].idx))==-1)
			YYABORT;
	  ;
    break;}
case 93:
#line 769 "yyparse.y"
{
		yyval.node.type = en_nt_null;
	  ;
    break;}
case 94:
#line 773 "yyparse.y"
{
		yyval.node.type = en_nt_qstr;
		yyval.node.value.qstr = yyvsp[0].qstring;
	  ;
    break;}
case 95:
#line 778 "yyparse.y"
{
		yyval.node.type = en_nt_param;
		yyval.node.value.ipar= yyvsp[0].ipar;
	  ;
    break;}
case 96:
#line 786 "yyparse.y"
{
		pyyenv->pstmt->table = yyvsp[-1].name;

		if( add_attr( pyyenv->pstmt, en_sender, 1 )
		 || add_attr( pyyenv->pstmt, en_from, 1 )
		 || add_attr( pyyenv->pstmt, en_msgid, 1 ) )
			YYABORT;
	  ;
    break;}
case 97:
#line 798 "yyparse.y"
{
		SETYYERROR( pyyenv, NOT_SUPPORT_POSITION_DELETE );
		YYABORT;
	  ;
    break;}
case 98:
#line 805 "yyparse.y"
{ SETYYERROR(pyyenv, NOT_SUPPORT_DDL_DCL); ;
    break;}
case 99:
#line 806 "yyparse.y"
{ SETYYERROR(pyyenv, NOT_SUPPORT_DDL_DCL); ;
    break;}
case 100:
#line 807 "yyparse.y"
{ SETYYERROR(pyyenv, NOT_SUPPORT_DDL_DCL); ;
    break;}
case 101:
#line 808 "yyparse.y"
{ SETYYERROR(pyyenv, NOT_SUPPORT_UPDATE); ;
    break;}
case 102:
#line 809 "yyparse.y"
{ SETYYERROR(pyyenv, NOT_SUPPORT_SPROC); ;
    break;}
case 103:
#line 811 "yyparse.y"
{ SETYYERROR(pyyenv, NOT_SUPPORT_SPROC); ;
    break;}
case 104:
#line 812 "yyparse.y"
{ SETYYERROR(pyyenv, NOT_SUPPORT_DDL_DCL); ;
    break;}
case 105:
#line 813 "yyparse.y"
{ SETYYERROR(pyyenv, NOT_SUPPORT_DDL_DCL); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 465 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.	*/

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 815 "yyparse.y"


static void	unpack_col_name(char* schema_tab_column_name, column_name_t* ptr)
{
	int	len, i;
	char	c;

	len = STRLEN(schema_tab_column_name);

	for(i=len; i; i--)
	{
		if( schema_tab_column_name[i-1] == '.' )
		{
			schema_tab_column_name[i-1] = 0;
			break;
		}
	}

	ptr->column_name = ( schema_tab_column_name + i );

	if( i )
		ptr->schema_tab_name = schema_tab_column_name;
	else
		ptr->schema_tab_name = schema_tab_column_name + len;
}

static char*	get_unpacked_attrname(yystmt_t* pstmt, char* name)
{
	column_name_t	attrname;

	unpack_col_name( name, &attrname );

	return attrname.column_name;
}

#define FILTER_CHUNK_SIZE	16

static	void*	add_node(yystmt_t* pstmt, node_t* node)
{
	int		i;
	node_t* 	srchtree;

	if (!pstmt->node_buf)
	{
		pstmt->node_buf = (node_t*)MEM_ALLOC(
			sizeof(node_t)*FILTER_CHUNK_SIZE);

		if( ! pstmt->node_buf )
		{
			pstmt->errcode = -1;
			return ERROR_PTR;
		}

		pstmt->srchtreesize = FILTER_CHUNK_SIZE;
		pstmt->srchtreenum  = 0;
	}

	if( pstmt->srchtreenum == pstmt->srchtreesize )
	{
		pstmt->node_buf = (node_t*)MEM_REALLOC(
			pstmt->node_buf,
			sizeof(node_t)*(pstmt->srchtreesize + FILTER_CHUNK_SIZE));

		if( ! pstmt->node_buf )
		{
			pstmt->errcode = -1;
			return ERROR_PTR;
		}

		pstmt->srchtreesize += FILTER_CHUNK_SIZE;
	}

	srchtree = pstmt->node_buf;

	srchtree[pstmt->srchtreenum] = *node;
	pstmt->srchtreenum ++;

	for(i=pstmt->srchtreenum;i<pstmt->srchtreesize;i++)
	{
		srchtree[i].left = EMPTY_PTR;
		srchtree[i].right= EMPTY_PTR;
	}

	return (void*)(pstmt->srchtreenum - 1);
}

static	void	srchtree_reloc(node_t* buf, int num)
/* The 'where' srchtree is build on a re-allocateable array with addnode(). The purpose
 * of using an array rather than a link list is to easy the job of freeing it. Thus,
 * the left and right pointer of a node point is an offset to the array address not
 * the virtual address of the subtree. However, an evaluate function would be easy to
 * work by using virtual address of subtree rather than an offset. Because, in the first
 * case, a recrusive evaluate function only need to pass virtual address of (sub)tree
 * without worry about the offset of subtree. The purpose of this function, srchtree_reloc(),
 * is to convert all subnodes' offset into virtual address. It will called only once
 * after the whole tree has been built.
 */
{
	int	i, offset;
	node_t* ptr = buf;

	for(i=0; ptr && i<num; ptr++, i++)
	{
		if( ptr->left == EMPTY_PTR )
			ptr->left = 0;
		else
		{
			offset = (int)(ptr->left);
			ptr->left = buf + offset;
		}

		if( ptr->right== EMPTY_PTR )
			ptr->right= 0;
		else
		{
			offset = (int)(ptr->right);
			ptr->right= buf+ offset;
		}
	}
}

static	int	add_attr(yystmt_t* pstmt, int idx, int wstat)
{
	if( ! pstmt->pattr )
	{
		pstmt->pattr = (yyattr_t*)MEM_ALLOC((en_body+1)*sizeof(yyattr_t));

		if( ! pstmt->pattr )
		{
			pstmt->errcode = -1;
			return -1;
		}

		MEM_SET(pstmt->pattr, 0, (en_body+1)*sizeof(yyattr_t));
	}

	(pstmt->pattr)[0].stat	= 1;
	(pstmt->pattr)[0].wstat = 1;
	(pstmt->pattr)[0].article = 0;
	(pstmt->pattr)[0].nntp_hand = 0;

	(pstmt->pattr)[idx].stat = 1;
	(pstmt->pattr)[idx].wstat |= wstat;

	return 0;
}

static void*	add_all_attr(yystmt_t* pstmt)
{
	int	i;
	yycol_t col;

	for(i=en_article_num + 1; i < en_body + 1; i++)
	{
		col.iattr = i;
		col.table = 0;

		if( add_column(pstmt, &col)
		 || add_attr  (pstmt, i, 0) )
			return ERROR_PTR;
	}

	return 0;
}

static const int	news_attr[] = {
	en_subject,
	en_date,
	en_from,
	en_organization,
	en_msgid,
	en_body
};

static void*	add_news_attr(yystmt_t* pstmt)
{
	int	i, n;
	yycol_t col;

	n = sizeof(news_attr)/sizeof(news_attr[0]);

	for(i=0; i<n ;i++)
	{
		col.iattr = news_attr[i];
		col.table = 0;

		if( add_column(pstmt, &col)
		 || add_attr  (pstmt, col.iattr, 0) )
			return ERROR_PTR;
	}

	return 0;
}

static const int	xnews_attr[] = {
	en_newsgroups,
	en_subject,
	en_date,
	en_from,
	en_organization,
	en_sender,
	en_msgid,
	en_summary,
	en_keywords,
	en_host,
	en_x_newsreader,
	en_body
};

static void*	add_xnews_attr(yystmt_t*	pstmt)
{
	int	i, n;
	yycol_t col;

	n = sizeof(xnews_attr)/sizeof(xnews_attr[0]);

	for(i=0; i<n ;i++)
	{
		col.iattr = xnews_attr[i];
		col.table = 0;

		if( add_column(pstmt, &col)
		 || add_attr  (pstmt, col.iattr, 0) )
			return ERROR_PTR;
	}

	return 0;
}

static void* add_column(yystmt_t* pstmt, yycol_t* column)
{
	yycol_t*	pcol;

	if( ! pstmt->pcol )
	{
		pstmt->pcol = (yycol_t*)MEM_ALLOC((MAX_COLUMN_NUMBER + 1)*sizeof(yycol_t));

		if( ! pstmt->pcol )
		{
			pstmt->errcode = -1;
			return ERROR_PTR;
		}

		MEM_SET( pstmt->pcol, 0, (MAX_COLUMN_NUMBER + 1)*sizeof(yycol_t));
	}

	if( ! pstmt->ncol )
	{
		pstmt->ncol = 1;
		pstmt->pcol->iattr = en_article_num;
		pstmt->pcol->table = 0;
	}

	if( pstmt->ncol > MAX_COLUMN_NUMBER + 1)
	{
		pstmt->errcode = TOO_MANY_COLUMNS;
		return ERROR_PTR;
	}

	pcol = pstmt->pcol + pstmt->ncol;
	pstmt->ncol++;

	*pcol = *column;

	return 0;
}

static void nnsql_yyerror(yyenv_t* pyyenv, char* msg)
{
	yystmt_t*	pstmt = pyyenv->pstmt;

	pstmt->errcode = PARSER_ERROR;
	pstmt->errpos  = pyyenv->errpos;

	sprintf(pstmt->msgbuf, NNSQL_ERRHEAD "%s", msg);
}

static	int	table_check(yystmt_t* pstmt)
{
	int	i;
	char	*table, *table1;

	table = pstmt->table;

	if( ! (table && *table) )
		return 0;

	for(i=1;pstmt->pcol && i<pstmt->ncol;i++)
	{
		table1 = (pstmt->pcol)[i].table;

		if( table1 && *table1 && !STREQ( table, table1 ) )
			return 0;
	}

	return 1;
}

static void*	attr_name(yystmt_t* pstmt, char* name)
{
	node_t	node;
	column_name_t	attrname;
	int		attridx;
	void*		offset;

	unpack_col_name( name, &attrname );

	attridx = nnsql_getcolidxbyname( attrname.column_name );
	if( attridx == -1 )
	{
		pstmt->errcode = INVALID_COLUMN_NAME;
		return ERROR_PTR;
	}

	if( attridx == en_body )
	{
		pstmt->errcode = UNSEARCHABLE_ATTR;
		return ERROR_PTR;
	}

	node.type = en_nt_attr;
	node.value.iattr =  attridx;
	node.left = EMPTY_PTR;
	node.right= EMPTY_PTR;

	if( (offset = add_node(pstmt, &node)) == ERROR_PTR )
		return ERROR_PTR;

	if( add_attr(pstmt, attridx, 1) )
		return ERROR_PTR;

	return offset;
}

static int	column_name(yystmt_t* pstmt, char* name)
{
	column_name_t	colname;
	int		colidx;
	yycol_t 	col;

	unpack_col_name( name, &colname );

	colidx = nnsql_getcolidxbyname( colname.column_name );
	if( colidx == -1 )
	{
		pstmt->errcode = INVALID_COLUMN_NAME;
		return -1;
	}

	col.iattr = colidx;
	col.table = colname.schema_tab_name;

	if( add_column(pstmt, &col)
	 || add_attr(pstmt, colidx, 0) )
		return -1;

	return 0;
}

static int	add_ins_head( yystmt_t* pstmt, char* head, int idx)
{
	if( !idx )
	{
		MEM_FREE(pstmt->ins_heads);
		pstmt->ins_heads = (char**)MEM_ALLOC( FILTER_CHUNK_SIZE * sizeof(char*));
	}
	else if( ! idx%FILTER_CHUNK_SIZE )
	{
		pstmt->ins_heads = (char**)MEM_REALLOC( pstmt->ins_heads,
			(idx+FILTER_CHUNK_SIZE)*sizeof(char*));
	}

	if( ! pstmt->ins_heads )
		return -1;

	(pstmt->ins_heads)[idx] = head;

	return idx + 1;
}

static int	add_ins_value( yystmt_t* pstmt, node_t node, int idx)
{
	if( !idx )
	{
		MEM_FREE(pstmt->ins_values)
		pstmt->ins_values = (node_t*)MEM_ALLOC( FILTER_CHUNK_SIZE * sizeof(node_t));
	}
	else if( ! idx%FILTER_CHUNK_SIZE )
	{
		pstmt->ins_values = (node_t*)MEM_REALLOC( pstmt->ins_values,
			(idx+FILTER_CHUNK_SIZE)*sizeof(node_t));
	}

	if( ! pstmt->ins_values )
		return -1;

	(pstmt->ins_values)[idx] = node;

	return idx + 1;
}
