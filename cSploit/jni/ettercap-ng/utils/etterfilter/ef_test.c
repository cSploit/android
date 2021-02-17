/*
    etterfilter -- test module

    Copyright (C) ALoR & NaGA

    This program is free software; you can redfopibute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is dfopibuted in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    $Id: ef_test.c,v 1.22 2005/01/04 14:30:50 alor Exp $
*/

#include <ef.h>
#include <ec_filter.h>
#include <ec_version.h>

#ifndef OS_WINDOWS
    #include <sys/mman.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* protos */

void test_filter(char *filename);

void print_fop(struct filter_op *fop, u_int32 eip);
static void print_test(struct filter_op *fop, u_int32 eip);
static void print_assign(struct filter_op *fop, u_int32 eip);
static void print_inc(struct filter_op *fop, u_int32 eip);
static void print_dec(struct filter_op *fop, u_int32 eip);
static void print_function(struct filter_op *fop, u_int32 eip);

/*******************************************/

/*
 * test a binary filter against a given file 
 */
void test_filter(char *filename)
{
   struct filter_op *fop;
   struct filter_env fenv;
   u_int32 eip = 0;

   memset(&fenv, 0, sizeof(struct filter_env));
   
   /* load the file */
   if (filter_load_file(filename, &fenv) != ESUCCESS) {
      exit(-1);
   }

   /* skip the header in the file */
   fop = fenv.chain;
   
   fprintf(stdout, "Disassebling \"%s\" content...\n\n", filename);
  
   /* loop all the instructions and print their content */
   while (eip < (fenv.len / sizeof(struct filter_op)) ) {

      /* print the instruction */
      print_fop(&fop[eip], eip);
      
      /* autoincrement the instruction pointer */
      eip++;
   }

   printf("\n %d instructions decoded.\n\n", (int)(fenv.len / sizeof(struct filter_op)));

   exit(0);
}

/*
 * helper functions to print instructions
 */
void print_fop(struct filter_op *fop, u_int32 eip)
{
      switch (fop->opcode) {
         case FOP_TEST:
            print_test(fop, eip);
            break;
            
         case FOP_ASSIGN:
            print_assign(fop, eip);
            break;
            
         case FOP_INC:
            print_inc(fop, eip);
            break;
            
         case FOP_DEC:
            print_dec(fop, eip);
            break;
            
         case FOP_FUNC:
            print_function(fop, eip);
            break;
            
         case FOP_JMP:
            fprintf(stdout, "%04lu: JUMP ALWAYS to %04d\n", (unsigned long)eip, fop->op.jmp);
            break;
            
         case FOP_JTRUE:
            fprintf(stdout, "%04lu: JUMP IF TRUE to %04d\n", (unsigned long)eip, fop->op.jmp);
            break;
            
         case FOP_JFALSE:
            fprintf(stdout, "%04lu: JUMP IF FALSE to %04d\n", (unsigned long)eip, fop->op.jmp);
            break;
            
         case FOP_EXIT:
            fprintf(stdout, "%04lu: EXIT\n", (unsigned long)eip);
            break;
            
         default:
            fprintf(stderr, "UNDEFINED OPCODE (%d) !!\n", fop->opcode);
            exit(-1);
            break;
      }
}

void print_test(struct filter_op *fop, u_int32 eip)
{
   switch(fop->op.test.op) {
      case FTEST_EQ:
         if (fop->op.test.size != 0)
            fprintf(stdout, "%04lu: TEST level %d, offset %d, size %d, == %lu [%#x]\n", (unsigned long)eip,
               fop->op.test.level, fop->op.test.offset, fop->op.test.size, (unsigned long)fop->op.test.value, (unsigned int)fop->op.test.value);
         else
            fprintf(stdout, "%04lu: TEST level %d, offset %d, \"%s\"\n", (unsigned long)eip,
               fop->op.test.level, fop->op.test.offset, fop->op.test.string);
         break;
         
      case FTEST_NEQ:
         if (fop->op.test.size != 0)
            fprintf(stdout, "%04lu: TEST level %d, offset %d, size %d, != %lu [%#x]\n", (unsigned long)eip,
               fop->op.test.level, fop->op.test.offset, fop->op.test.size, (unsigned long)fop->op.test.value, (unsigned int)fop->op.test.value);
         else
            fprintf(stdout, "%04lu: TEST level %d, offset %d, not \"%s\"\n", (unsigned long)eip,
               fop->op.test.level, fop->op.test.offset, fop->op.test.string);
         break;

      case FTEST_LT:
         fprintf(stdout, "%04lu: TEST level %d, offset %d, size %d, < %lu [%#x]\n", (unsigned long)eip,
            fop->op.test.level, fop->op.test.offset, fop->op.test.size, (unsigned long)fop->op.test.value, (unsigned int)fop->op.test.value);
         break;
         
      case FTEST_GT:
         fprintf(stdout, "%04lu: TEST level %d, offset %d, size %d, > %lu [%#x]\n", (unsigned long)eip,
            fop->op.test.level, fop->op.test.offset, fop->op.test.size, (unsigned long)fop->op.test.value, (unsigned int)fop->op.test.value);
         break;
         
      case FTEST_LEQ:
         fprintf(stdout, "%04lu: TEST level %d, offset %d, size %d, <= %lu [%#x]\n", (unsigned long)eip,
            fop->op.test.level, fop->op.test.offset, fop->op.test.size, (unsigned long)fop->op.test.value, (unsigned int)fop->op.test.value);
         break;
         
      case FTEST_GEQ:
         fprintf(stdout, "%04lu: TEST level %d, offset %d, size %d, >= %lu [%#x]\n", (unsigned long)eip,
            fop->op.test.level, fop->op.test.offset, fop->op.test.size, (unsigned long)fop->op.test.value, (unsigned int)fop->op.test.value);
         break;

      default:
         fprintf(stderr, "%04lu: UNDEFINED TEST OPCODE (%d) !!\n", (unsigned long)eip, fop->op.test.op);
         break;
           
   }
}

void print_assign(struct filter_op *fop, u_int32 eip)
{
   if (fop->op.assign.size != 0)
      fprintf(stdout, "%04lu: ASSIGNMENT level %d, offset %d, size %d, value %lu [%#x]\n", (unsigned long)eip,
            fop->op.assign.level, fop->op.assign.offset, fop->op.assign.size, (unsigned long)fop->op.assign.value, (unsigned int)fop->op.assign.value);
   else
      fprintf(stdout, "%04lu: ASSIGNMENT level %d, offset %d, string \"%s\"\n", (unsigned long)eip, 
            fop->op.assign.level, fop->op.assign.offset, fop->op.assign.string);
      
}

void print_inc(struct filter_op *fop, u_int32 eip)
{
      fprintf(stdout, "%04lu: INCREMENT level %d, offset %d, size %d, value %lu [%#x]\n", (unsigned long)eip,
            fop->op.assign.level, fop->op.assign.offset, fop->op.assign.size, (unsigned long)fop->op.assign.value, (unsigned int)fop->op.assign.value);
}

void print_dec(struct filter_op *fop, u_int32 eip)
{
      fprintf(stdout, "%04lu: DECREMENT level %d, offset %d, size %d, value %lu [%#x]\n", (unsigned long)eip,
            fop->op.assign.level, fop->op.assign.offset, fop->op.assign.size, (unsigned long)fop->op.assign.value, (unsigned int)fop->op.assign.value);
}

void print_function(struct filter_op *fop, u_int32 eip)
{
   switch (fop->op.func.op) {
      case FFUNC_SEARCH:
         fprintf(stdout, "%04lu: SEARCH level %d, string \"%s\"\n", (unsigned long)eip, 
               fop->op.func.level, fop->op.func.string);
         break;
         
      case FFUNC_REGEX:
         fprintf(stdout, "%04lu: REGEX level %d, string \"%s\"\n", (unsigned long)eip, 
               fop->op.func.level, fop->op.func.string);
         break;
         
      case FFUNC_PCRE:
         if (fop->op.func.replace)
            fprintf(stdout, "%04lu: PCRE_REGEX level %d, string \"%s\", replace \"%s\"\n", (unsigned long)eip, 
               fop->op.func.level, fop->op.func.string, fop->op.func.replace);
         else
            fprintf(stdout, "%04lu: PCRE_REGEX level %d, string \"%s\"\n", (unsigned long)eip, 
               fop->op.func.level, fop->op.func.string);
         break;

      case FFUNC_REPLACE:
         fprintf(stdout, "%04lu: REPLACE \"%s\" --> \"%s\"\n", (unsigned long)eip, 
               fop->op.func.string, fop->op.func.replace);
         break;
         
      case FFUNC_INJECT:
         fprintf(stdout, "%04lu: INJECT \"%s\"\n", (unsigned long)eip, 
               fop->op.func.string);
         break;
         
      case FFUNC_LOG:
         fprintf(stdout, "%04lu: LOG to \"%s\"\n", (unsigned long)eip, fop->op.func.string);
         break;
         
      case FFUNC_DROP:
         fprintf(stdout, "%04lu: DROP\n", (unsigned long)eip);
         break;
         
      case FFUNC_KILL:
         fprintf(stdout, "%04lu: KILL\n", (unsigned long)eip);
         break;
         
      case FFUNC_MSG:
         fprintf(stdout, "%04lu: MSG \"%s\"\n", (unsigned long)eip, fop->op.func.string);
         break;
         
      case FFUNC_EXEC:
         fprintf(stdout, "%04lu: EXEC \"%s\"\n", (unsigned long)eip, fop->op.func.string);
         break;
         
      default:
         fprintf(stderr, "%04lu: UNDEFINED FUNCTION OPCODE (%d)!!\n", (unsigned long)eip, fop->op.func.op);
         break;
   }

}


/* EOF */

// vim:ts=3:expandtab

