

#ifndef EF_FUNCTIONS_H
#define EF_FUNCTIONS_H

#include <ec_filter.h>

#define SCRIPT_ERROR(x, ...) FATAL_ERROR("\n[%s:%d]: "x, GBL_OPTIONS.source_file, GBL.lineno, ## __VA_ARGS__ );

#define WARNING(x) do {                   \
if (!GBL_OPTIONS.suppress_warnings)       \
   FATAL_ERROR("\n[%s:%ld]: WARNING "x, GBL_OPTIONS.source_file, (unsigned long)GBL.lineno);  \
else                                      \
   fprintf(stderr, "\n[%s:%ld]: WARNING "x, GBL_OPTIONS.source_file, (unsigned long)GBL.lineno);  \
} while(0)

/* ef_main */
EF_API_EXTERN void ef_debug(u_char level, const char *message, ...);

/* ef_parser */
EF_API_EXTERN void parse_options(int argc, char **argv);

/* ef_test */
EF_API_EXTERN void test_filter(char *filename);
EF_API_EXTERN void print_fop(struct filter_op *fop, u_int32 eip);

/* ef_syntax && ef_grammar */
EF_API_EXTERN int yyerror(char *);                                                                         
EF_API_EXTERN int yylex(void);

/* ef_tables */
EF_API_EXTERN void load_tables(void);
EF_API_EXTERN void load_constants(void);
EF_API_EXTERN int get_virtualpointer(char *name, char *offname, u_int8 *level, u_int16 *offset, u_int8 *size);
EF_API_EXTERN int get_constant(char *name, u_int32 *value);

/* ef_encode */
EF_API_EXTERN int encode_offset(char *string, struct filter_op *fop);
EF_API_EXTERN int encode_function(char *string, struct filter_op *fop);
EF_API_EXTERN int encode_const(char *string, struct filter_op *fop);

/* ef_output */

EF_API_EXTERN int write_output(void);

/* ef_compiler */

struct block {
   union {
      struct instruction *ins;
      struct ifblock *ifb;
   } un;
   struct block *next;
   u_int8 type;
      #define BLK_INSTR 0
      #define BLK_IFBLK 1
};

struct instruction {
   struct filter_op fop;
};

struct ifblock {
   struct condition *conds;
   struct block *blk;
   struct block *elseblk;
};

struct condition {
   struct filter_op fop;
   struct condition *next;
   u_int16 op;
      #define COND_AND  0
      #define COND_OR  1
};

EF_API_EXTERN int compiler_set_root(struct block *blk);
EF_API_EXTERN size_t compile_tree(struct filter_op **fop);
EF_API_EXTERN struct block * compiler_add_instr(struct instruction *ins, struct block *blk);
EF_API_EXTERN struct block * compiler_add_ifblk(struct ifblock *ifb, struct block *blk);
EF_API_EXTERN struct instruction * compiler_create_instruction(struct filter_op *fop);
EF_API_EXTERN struct condition * compiler_create_condition(struct filter_op *fop);
EF_API_EXTERN struct condition * compiler_concat_conditions(struct condition *a, u_int16 op, struct condition *b);
EF_API_EXTERN struct ifblock * compiler_create_ifblock(struct condition *conds, struct block *blk);
EF_API_EXTERN struct ifblock * compiler_create_ifelseblock(struct condition *conds, struct block *blk, struct block *elseblk);

#endif

/* EOF */

// vim:ts=3:expandtab

