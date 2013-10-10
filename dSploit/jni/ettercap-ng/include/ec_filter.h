
/* $Id: ec_filter.h,v 1.22 2005/01/04 14:30:49 alor Exp $ */

#ifndef EC_FILTER_H
#define EC_FILTER_H

#include <ec_packet.h>

#include <regex.h>
#ifdef HAVE_PCRE
   #include <pcre.h>
#endif

/* 
 * this is the struct used by the filtering engine
 * it is the equivalent of a processor's instruction
 *
 * they are organized in an array and evaluated one 
 * at a time. the jump are absolute and the addressing
 * is done by the array position.
 *
 */

//#define MAX_FILTER_LEN  200

struct filter_op {
   char opcode;
      #define FOP_EXIT     0
      #define FOP_TEST     1
      #define FOP_ASSIGN   2
      #define FOP_INC      3
      #define FOP_DEC      4
      #define FOP_FUNC     5
      #define FOP_JMP      6
      #define FOP_JTRUE    7
      #define FOP_JFALSE   8

   /*
    * the first two field of the structs (op and level) must
    * overlap the same memory region. it is abused in ef_encode.c
    * encoding a function that uses an offset as an argument
    */
   union {
      /* functions */
      struct {
         char op;
            #define FFUNC_SEARCH    0
            #define FFUNC_REGEX     1
            #define FFUNC_PCRE      2
            #define FFUNC_REPLACE   3
            #define FFUNC_INJECT    4
            #define FFUNC_LOG       5
            #define FFUNC_DROP      6
            #define FFUNC_KILL      7
            #define FFUNC_MSG       8
            #define FFUNC_EXEC      9
         u_int8 level; 
         u_int8 *string;
         size_t slen;
         u_int8 *replace;
         size_t rlen;
         struct regex_opt *ropt;
      } func;
      
      /* tests */
      struct {
         u_int8   op;
            #define FTEST_EQ   0
            #define FTEST_NEQ  1
            #define FTEST_LT   2   
            #define FTEST_GT   3
            #define FTEST_LEQ  4
            #define FTEST_GEQ  5
         u_int8   level;
         u_int8   size;
         u_int16  offset;
         u_int32  value;
         u_int8   *string;
         size_t   slen;
      } test, assign;

      /* jumps */
      u_int16 jmp;
      
   } op;
};

/* the header for a binary filter file 
 *
 * a file is structured as follow:
 *    the header
 *    the data segment (containing all the strings)
 *    the code segment (containing all the instructions)
 *
 * when the file is loaded all the string must be referenced
 * by the instructions
 */
struct filter_header {
   /* magic number */
   u_int16 magic; 
      #define EC_FILTER_MAGIC 0xe77e
   /* ettercap version */
   char version[16];
   /* pointers to the segments */
   u_int16 data;
   u_int16 code;
};

/* filters header for mmapped region */
struct filter_env {
   void *map;
   struct filter_op *chain;
   size_t len;
};

/* uset to compile the regex while loading the file */
struct regex_opt {
   regex_t *regex;
#ifdef HAVE_PCRE
   pcre *pregex;
   pcre_extra *preg_extra;   
#endif
};

#define PCRE_OVEC_SIZE 100

/* exported functions */

EC_API_EXTERN int filter_engine(struct filter_op *fop, struct packet_object *po);
EC_API_EXTERN int filter_load_file(char *filename, struct filter_env *fenv);
EC_API_EXTERN void filter_unload(struct filter_env *fenv);

#endif

/* EOF */

// vim:ts=3:expandtab

