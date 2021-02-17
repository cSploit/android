/*
    etterfilter -- grammar for filter source files

    Copyright (C) ALoR & NaGA

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    $Id: ef_grammar.y,v 1.24 2005/01/04 14:30:49 alor Exp $
*/

%{

#include <ef.h>
#include <ef_functions.h>
#include <ec_filter.h>

#define YYERROR_VERBOSE

%}
 
/* 
 * ==========================================
 *          BISON Declarations 
 * ==========================================
 */
 
/* definition for the yylval (global variable) */
%union {
   char *string;     
   struct filter_op fop;
   /* used to create the compiler tree */
   struct block *blk;
   struct instruction *ins;
   struct ifblock *ifb;
   struct condition *cnd;
}

/* token definitions */
%token TOKEN_EOL

%token <fop> TOKEN_CONST     /* an integer number */
%token <fop> TOKEN_OFFSET    /* an offset in the form xxxx.yyy.zzzzz */
%token <fop> TOKEN_STRING    /* a string "xxxxxx" */
%token <fop> TOKEN_FUNCTION  /* a function */

%token TOKEN_IF          /*  if ( ) {  */
%token TOKEN_ELSE        /*  } else {  */

%token TOKEN_OP_AND      /*  &&  */
%token TOKEN_OP_OR       /*  ||  */

%token TOKEN_OP_ASSIGN   /*  =   */
%token TOKEN_OP_INC      /*  +=  */
%token TOKEN_OP_DEC      /*  -=  */
%token TOKEN_OP_CMP_NEQ  /*  !=  */
%token TOKEN_OP_CMP_EQ   /*  ==  */
%token TOKEN_OP_CMP_LT   /*  <   */
%token TOKEN_OP_CMP_GT   /*  >   */
%token TOKEN_OP_CMP_LEQ  /*  <=  */
%token TOKEN_OP_CMP_GEQ  /*  >=  */

%token TOKEN_OP_END      /*  ;  */

%token TOKEN_PAR_OPEN    /*  (  */
%token TOKEN_PAR_CLOSE   /*  )  */

%token TOKEN_BLK_BEGIN   /*  {  */
%token TOKEN_BLK_END     /*  }  */

%token TOKEN_UNKNOWN

/* non terminals */
%type <fop> instruction
%type <fop> condition
%type <fop> offset
%type <fop> math_expr

%type <blk> block
%type <ins> single_instruction
%type <ifb> if_statement
%type <ifb> if_else_statement
%type <cnd> conditions_block

/* precedences */
%left TOKEN_OP_SUB TOKEN_OP_ADD
%left TOKEN_OP_MUL TOKEN_OP_DIV
%left TOKEN_UMINUS /* unary minus */

%left TOKET_OP_AND
%left TOKET_OP_OR
%left TOKET_OP_NOT

%%

/* 
 * ==========================================
 *          GRAMMAR Definitions 
 * ==========================================
*/

/* general line, can be empty or not */ 
input: /* empty line */ 
      | input block { 
         /* 
          * at this point the meta-tree is completed,
          * we only have to link it to the entry point
          */
         compiler_set_root($2);
      }
      ;
     
block:   /* empty block */ {
            $$ = NULL;
      }
      |  single_instruction block { 
            ef_debug(2, "\t\t block_add single\n"); 
            $$ = compiler_add_instr($1, $2);
         }
         
      |  if_statement block { 
            ef_debug(2, "\t\t block_add if\n"); 
            $$ = compiler_add_ifblk($1, $2);
         }

      |  if_else_statement block { 
            ef_debug(2, "\t\t block_add if_else\n"); 
            $$ = compiler_add_ifblk($1, $2);
         }
      ;
      
/* every instruction must be terminated with ; */      
single_instruction: 
         instruction TOKEN_OP_END {
            $$ = compiler_create_instruction(&$1);
         }
      ;

/* instructions are functions or assignments */
instruction: 
         TOKEN_FUNCTION { 
            ef_debug(1, ".");
            ef_debug(3, "\tfunction\n"); 
            /* functions are encoded by the lexycal analyzer */
         }
      
      |  offset TOKEN_OP_ASSIGN TOKEN_STRING { 
            ef_debug(1, "=");
            ef_debug(3, "\tassignment string\n"); 
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_ASSIGN;
            $$.op.assign.string = strdup($3.op.assign.string);
            $$.op.assign.slen = $3.op.assign.slen;
            /* this is a string */
            $$.op.assign.size = 0;
         }

      |  offset TOKEN_OP_ASSIGN math_expr { 
            ef_debug(1, "=");
            ef_debug(3, "\tassignment\n"); 
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_ASSIGN;
            $$.op.assign.value = $3.op.assign.value;
         }
      
      |  offset TOKEN_OP_INC TOKEN_CONST { 
            ef_debug(1, "+=");
            ef_debug(3, "\tincrement\n"); 
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_INC;
            $$.op.assign.value = $3.op.assign.value;
         }
      
      |  offset TOKEN_OP_DEC TOKEN_CONST { 
            ef_debug(1, "-=");
            ef_debug(3, "\tdecrement\n"); 
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_DEC;
            $$.op.assign.value = $3.op.assign.value;
         }
      ;

/* the if statement */
if_statement: 
         TOKEN_IF TOKEN_PAR_OPEN conditions_block TOKEN_PAR_CLOSE TOKEN_BLK_BEGIN block TOKEN_BLK_END { 
            ef_debug(1, "#");
            ef_debug(3, "\t\t IF BLOCK\n"); 
            $$ = compiler_create_ifblock($3, $6);
         }
      ;
      
/* if {} else {} */      
if_else_statement: 
         TOKEN_IF TOKEN_PAR_OPEN conditions_block TOKEN_PAR_CLOSE TOKEN_BLK_BEGIN block TOKEN_BLK_END TOKEN_ELSE TOKEN_BLK_BEGIN block TOKEN_BLK_END { 
            ef_debug(1, "@");
            ef_debug(3, "\t\t IF ELSE BLOCK\n"); 
            $$ = compiler_create_ifelseblock($3, $6, $10);
         }
      ;

/* conditions used by the if statement */
conditions_block:
         condition {
            ef_debug(1, "?");
            ef_debug(3, "\t\t CONDITION\n"); 
            $$ = compiler_create_condition(&$1);
         }

      |  conditions_block TOKEN_OP_AND conditions_block { 
            ef_debug(1, "&");
            ef_debug(3, "\t\t AND\n"); 
            $$ = compiler_concat_conditions($1, COND_AND, $3);
         }
         
      |  conditions_block TOKEN_OP_OR conditions_block { 
            ef_debug(1, "|");
            ef_debug(3, "\t\t OR\n"); 
            $$ = compiler_concat_conditions($1, COND_OR, $3);
         } 
      ;
     
/* a single condition */     
condition: 
         offset TOKEN_OP_CMP_EQ TOKEN_STRING { 
            ef_debug(4, "\tcondition cmp eq string\n"); 
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_TEST;
            $$.op.test.op = FTEST_EQ;
            $$.op.test.string = strdup($3.op.test.string);
            $$.op.test.slen = $3.op.assign.slen;
            /* this is a string */
            $$.op.test.size = 0;
         }
      
      |  offset TOKEN_OP_CMP_NEQ TOKEN_STRING { 
            ef_debug(4, "\tcondition cmp not eq string\n"); 
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_TEST;
            $$.op.test.op = FTEST_NEQ;
            $$.op.test.string = strdup($3.op.test.string);
            $$.op.test.slen = $3.op.assign.slen;
            /* this is a string */
            $$.op.test.size = 0;
         }

      |  offset TOKEN_OP_CMP_EQ TOKEN_CONST { 
            ef_debug(4, "\tcondition cmp eq\n");
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_TEST;
            $$.op.test.op = FTEST_EQ;
            $$.op.test.value = $3.op.test.value;
         }
      
      |  offset TOKEN_OP_CMP_NEQ TOKEN_CONST { 
            ef_debug(4, "\tcondition cmp not eq\n");
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_TEST;
            $$.op.test.op = FTEST_NEQ;
            $$.op.test.value = $3.op.test.value;
         }
         
      |  offset TOKEN_OP_CMP_LT TOKEN_CONST { 
            ef_debug(4, "\tcondition cmp lt\n"); 
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_TEST;
            $$.op.test.op = FTEST_LT;
            $$.op.test.value = $3.op.test.value;
         }

      |  offset TOKEN_OP_CMP_GT TOKEN_CONST { 
            ef_debug(4, "\tcondition cmp gt\n");
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_TEST;
            $$.op.test.op = FTEST_GT;
            $$.op.test.value = $3.op.test.value;
         }

      |  offset TOKEN_OP_CMP_LEQ TOKEN_CONST { 
            ef_debug(4, "\tcondition cmp leq\n");
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_TEST;
            $$.op.test.op = FTEST_LEQ;
            $$.op.test.value = $3.op.test.value;
         }

      |  offset TOKEN_OP_CMP_GEQ TOKEN_CONST { 
            ef_debug(4, "\tcondition cmp geq\n"); 
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.opcode = FOP_TEST;
            $$.op.test.op = FTEST_GEQ;
            $$.op.test.value = $3.op.test.value;
         }
      
      |  TOKEN_FUNCTION { 
            ef_debug(4, "\tcondition func\n"); 
            /* functions are encoded by the lexycal analyzer */
         }
      ;

/* offsets definitions */
offset:
         TOKEN_OFFSET {
            ef_debug(4, "\toffset\n"); 
            memcpy(&$$, &$1, sizeof(struct filter_op));
         }
         
      |  offset TOKEN_OP_ADD math_expr {
            ef_debug(4, "\toffset add\n"); 
            memcpy(&$$, &$1, sizeof(struct filter_op));
            /* 
             * we are lying here, but math_expr operates
             * only on values, so we can add it to offset
             */
            $$.op.test.offset = $1.op.test.offset + $3.op.test.value; 
            if ($$.op.test.offset % $$.op.test.size)
               WARNING("Unaligned offset");
         }
         
      |  offset TOKEN_OP_SUB math_expr {
            ef_debug(4, "\toffset sub\n"); 
            memcpy(&$$, &$1, sizeof(struct filter_op));
            $$.op.test.offset = $1.op.test.offset - $3.op.test.value; 
            if ($$.op.test.offset % $$.op.test.size)
               WARNING("Unaligned offset");
         }
      ;

/* math expression */
math_expr: 
         TOKEN_CONST  { 
            $$.op.test.value = $1.op.test.value; 
         }
         
      |  math_expr TOKEN_OP_ADD math_expr { 
            $$.op.test.value = $1.op.test.value + $3.op.test.value;
         }
         
      |  math_expr TOKEN_OP_SUB math_expr {
            $$.op.test.value = $1.op.test.value - $3.op.test.value;
         }
         
      |  math_expr TOKEN_OP_MUL math_expr {
            $$.op.test.value = $1.op.test.value * $3.op.test.value;
         }
         
      |  math_expr TOKEN_OP_DIV math_expr {
            $$.op.test.value = $1.op.test.value / $3.op.test.value;
         }
         
      |  TOKEN_OP_SUB math_expr %prec TOKEN_UMINUS {
            $$.op.test.value = -$2.op.test.value;
         }
      ;

%%

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

