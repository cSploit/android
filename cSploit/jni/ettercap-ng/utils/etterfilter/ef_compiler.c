/*
    etterfilter -- the actual compiler

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

    $Id: ef_compiler.c,v 1.7 2003/10/04 14:58:34 alor Exp $
*/

#include <ef.h>
#include <ef_functions.h>

/* globals */

/* 
 * the compiler works this way:
 *
 * while bison parses the input it calls the function to
 * create temporary lists of "blocks". a block is a compound
 * of virtual instruction. every block element can contain a
 * single instruction or a if block.
 * the if block contains a link to the conditions list and two
 * link for the two block to be executed if the condition is 
 * true or the else block (if any).
 *
 * so, after bison has finished its parsing we have a tree of 
 * virtual instructions like that:
 * 
 *   -----------
 *  | tree root |
 *   ----------- 
 *        |
 *   -----------         -------------
 *  | block elm |  -->  | instruction |
 *   -----------         -------------
 *        |
 *   -----------         -------------
 *  | block elm |  -->  | instruction |
 *   -----------         -------------
 *        |
 *   -----------         -------------       ------------
 *  | block elm |  -->  |   if block  | --> | conditions |
 *   -----------         -------------       ------------
 *        .               /         \
 *        .        -----------    -----------
 *        .       | block elm |  | block elm | . . .
 *                 -----------    -----------
 *                      .               .
 *                      .               .
 * 
 * to create a binary filter we have to unfold the tree by converting
 * the conditions into test, eliminating the virtual if block and 
 * create the right conditional jumps.
 * during the first unfolding the jumps are referencing virtual labels.
 * all the instructions are unfolded in a double-linked list.
 *
 * the last phase involves the tanslation of the labels into real offsets
 */

static struct block *tree_root;

struct unfold_elm {
   u_int32 label;
   struct filter_op fop;
   TAILQ_ENTRY (unfold_elm) next;
};

static TAILQ_HEAD(, unfold_elm) unfolded_tree = TAILQ_HEAD_INITIALIZER(unfolded_tree);

/* label = 0 means "no label" */
static u_int32 vlabel = 1;

/* protos */

int compiler_set_root(struct block *blk);

size_t compile_tree(struct filter_op **fop);
static void unfold_blk(struct block **blk);
static void unfold_ifblk(struct block **blk);
static void unfold_conds(struct condition *cnd, u_int32 a, u_int32 b);
static void labels_to_offsets(void);

struct block * compiler_add_instr(struct instruction *ins, struct block *blk);
struct block * compiler_add_ifblk(struct ifblock *ifb, struct block *blk);

struct instruction * compiler_create_instruction(struct filter_op *fop);
struct condition * compiler_create_condition(struct filter_op *fop);
struct condition * compiler_concat_conditions(struct condition *a, u_int16 op, struct condition *b);

struct ifblock * compiler_create_ifblock(struct condition *conds, struct block *blk);
struct ifblock * compiler_create_ifelseblock(struct condition *conds, struct block *blk, struct block *elseblk);


/*******************************************/

/*
 * set the entry point of the filter tree
 */
int compiler_set_root(struct block *blk)
{
   BUG_IF(blk == NULL);
   tree_root = blk;
   return ESUCCESS;
}

/*
 * allocate an instruction container for filter_op
 */
struct instruction * compiler_create_instruction(struct filter_op *fop)
{
   struct instruction *ins;

   SAFE_CALLOC(ins, 1, sizeof(struct instruction));
   
   /* copy the instruction */
   memcpy(&ins->fop, fop, sizeof(struct filter_op));

   return ins;
}


/*
 * allocate a condition container for filter_op
 */
struct condition * compiler_create_condition(struct filter_op *fop)
{
   struct condition *cnd;

   SAFE_CALLOC(cnd, 1, sizeof(struct condition));
   
   /* copy the instruction */
   memcpy(&cnd->fop, fop, sizeof(struct filter_op));

   return cnd;
}


/*
 * concatenates two conditions with a logical operator 
 */
struct condition * compiler_concat_conditions(struct condition *a, u_int16 op, struct condition *b)
{
   struct condition *head = a;
   
   /* go to the last conditions in 'a' */
   while(a->next != NULL)
      a = a->next;
   
   /* set the operation */
   a->op = op;

   /* contatenate the two block */
   a->next = b;
   
   /* return the head of the conditions */
   return head;
}

/*
 * allocate a ifblock container
 */
struct ifblock * compiler_create_ifblock(struct condition *conds, struct block *blk)
{
   struct ifblock *ifblk;

   SAFE_CALLOC(ifblk, 1, sizeof(struct ifblock));

   /* associate the pointers */
   ifblk->conds = conds;
   ifblk->blk = blk;

   return ifblk;
}


/*
 * allocate a if_else_block container
 */
struct ifblock * compiler_create_ifelseblock(struct condition *conds, struct block *blk, struct block *elseblk)
{
   struct ifblock *ifblk;

   SAFE_CALLOC(ifblk, 1, sizeof(struct ifblock));
   
   /* associate the pointers */
   ifblk->conds = conds;
   ifblk->blk = blk;
   ifblk->elseblk = elseblk;

   return ifblk;
}


/*
 * add an instruction to a block
 */
struct block * compiler_add_instr(struct instruction *ins, struct block *blk)
{
   struct block *bl;

   SAFE_CALLOC(bl, 1, sizeof(struct block));

   /* copy the current instruction in the block */
   bl->type = BLK_INSTR;
   bl->un.ins = ins;

   /* link it to the old block chain */
   bl->next = blk;

   return bl;
}


/* 
 * add an if block to a block
 */
struct block * compiler_add_ifblk(struct ifblock *ifb, struct block *blk)
{
   struct block *bl;

   SAFE_CALLOC(bl, 1, sizeof(struct block));

   /* copy the current instruction in the block */
   bl->type = BLK_IFBLK;
   bl->un.ifb = ifb;

   /* link it to the old block chain */
   bl->next = blk;

   return bl;
}


/*
 * parses the tree and produce a compiled
 * array of filter_op
 */
size_t compile_tree(struct filter_op **fop)
{
   int i = 1;
   struct filter_op *array = NULL;
   struct unfold_elm *ue;

   BUG_IF(tree_root == NULL);
  
   fprintf(stdout, " Unfolding the meta-tree ");
   fflush(stdout);
     
   /* start the recursion on the tree */
   unfold_blk(&tree_root);

   fprintf(stdout, " done.\n\n");

   /* substitute the virtual labels with real offsets */
   labels_to_offsets();
   
   /* convert the tailq into an array */
   TAILQ_FOREACH(ue, &unfolded_tree, next) {

      /* label == 0 means a real instruction */
      if (ue->label == 0) {
         SAFE_REALLOC(array, i * sizeof(struct filter_op));
         memcpy(&array[i - 1], &ue->fop, sizeof(struct filter_op));
         i++;
      }
   }
   
   /* always append the exit function to a script */
   SAFE_REALLOC(array, i * sizeof(struct filter_op));
   array[i - 1].opcode = FOP_EXIT;
   
   /* return the pointer to the array */
   *fop = array;
   
   return (i);
}


/*
 * unfold a block putting it in the unfolded_tree list
 */
static void unfold_blk(struct block **blk)
{
   struct unfold_elm *ue = NULL;
  
   BUG_IF(*blk == NULL);

   /* the progress bar */
   ef_debug(1, "+"); 
   
   do {
      switch((*blk)->type) {
         case BLK_INSTR:
            /* insert the instruction as is */
            SAFE_CALLOC(ue, 1, sizeof(struct unfold_elm));
            memcpy(&ue->fop, (*blk)->un.ins, sizeof(struct filter_op));
            TAILQ_INSERT_TAIL(&unfolded_tree, ue, next);
            break;

         case BLK_IFBLK:
            unfold_ifblk(blk);
            break;
            
         default:
            BUG("undefined tree element");
            break;
      } 
   } while ((*blk = (*blk)->next));
  
}


/*
 * unfold an if block putting it in the unfolded_tree list
 */
static void unfold_ifblk(struct block **blk)
{
   struct ifblock *ifblk;
   struct unfold_elm *ue;
   u_int32 a = vlabel++; 
   u_int32 b = vlabel++; 
   u_int32 c = vlabel++; 

   /*
    * the virtual labels represent the three points of an if block:
    *
    *    if (conds) {
    * a ->
    *       ...
    *       jmp c;
    * b ->
    *    } else {
    *       ...
    *    }
    * c ->
    *
    * if the conds are true, jump to 'a'
    * if the conds are false, jump to 'b'
    * 'c' is used to skip the else if the conds were true
    */

   /* the progress bar */
   ef_debug(1, "#"); 
   
   /* cast the if block */
   ifblk = (*blk)->un.ifb;
  
   /* compile the conditions */
   unfold_conds(ifblk->conds, a, b);
   
   /* if the conditions are match, jump here */
   SAFE_CALLOC(ue, 1, sizeof(struct unfold_elm));
   ue->label = a;
   TAILQ_INSERT_TAIL(&unfolded_tree, ue, next);

   /* recursively compile the main block */
   unfold_blk(&ifblk->blk);

   /* 
    * if there is the else block, we have to skip it
    * if the condition was true
    */
   if (ifblk->elseblk != NULL) {
      SAFE_CALLOC(ue, 1, sizeof(struct unfold_elm));
      ue->fop.opcode = FOP_JMP;
      ue->fop.op.jmp = c;
      TAILQ_INSERT_TAIL(&unfolded_tree, ue, next);
   }
   
   /* if the conditions are NOT match, jump here (after the block) */
   SAFE_CALLOC(ue, 1, sizeof(struct unfold_elm));
   ue->label = b;
   TAILQ_INSERT_TAIL(&unfolded_tree, ue, next);
   
   /* recursively compile the else block */
   if (ifblk->elseblk != NULL) {
      unfold_blk(&ifblk->elseblk);
      /* this is the label to skip the else if the condition was true */
      SAFE_CALLOC(ue, 1, sizeof(struct unfold_elm));
      ue->label = c;
      TAILQ_INSERT_TAIL(&unfolded_tree, ue, next);
   }

}


/*
 * unfold a conditions block putting it in the unfolded_tree list
 */
static void unfold_conds(struct condition *cnd, u_int32 a, u_int32 b)
{
   struct unfold_elm *ue = NULL;
 
   do {
   
      /* the progress bar */
      ef_debug(1, "?"); 
   
      /* insert the condition as is */
      SAFE_CALLOC(ue, 1, sizeof(struct unfold_elm));
      memcpy(&ue->fop, &cnd->fop, sizeof(struct filter_op));
      TAILQ_INSERT_TAIL(&unfolded_tree, ue, next);
      
      /* insert the conditional jump */
      SAFE_CALLOC(ue, 1, sizeof(struct unfold_elm));
      
      if (cnd->op == COND_OR) {
         ue->fop.opcode = FOP_JTRUE;
         ue->fop.op.jmp = a;
      } else {
         /* AND and single instructions behave equally */
         ue->fop.opcode = FOP_JFALSE;
         ue->fop.op.jmp = b;
      }
      
      TAILQ_INSERT_TAIL(&unfolded_tree, ue, next);
      
   } while ((cnd = cnd->next));
   
}

/*
 * converts the virtual labels to real offsets
 */
static void labels_to_offsets(void)
{
   struct unfold_elm *ue;
   struct unfold_elm *s;
   u_int32 offset = 0;

   fprintf(stdout, " Converting labels to real offsets ");
   fflush(stdout);
   
   TAILQ_FOREACH(ue, &unfolded_tree, next) {
      /* search only for jumps */
      if (ue->fop.opcode == FOP_JMP || 
          ue->fop.opcode == FOP_JTRUE ||
          ue->fop.opcode == FOP_JFALSE) {
        
         switch (ue->fop.opcode) {
            case FOP_JMP:
               ef_debug(1, "*"); 
               break;
            case FOP_JTRUE:
               ef_debug(1, "+");
               break;
            case FOP_JFALSE:
               ef_debug(1, "-");
               break;
         }
         
         /* search the offset associated with the label */
         TAILQ_FOREACH(s, &unfolded_tree, next) {
            if (s->label == ue->fop.op.jmp) {
               ue->fop.op.jmp = offset;
               /* reset the offset */
               offset = 0;
               break;
            }
            /* if it is an instruction, increment the offset */
            if (s->label == 0)
               offset++;
         }
      }
   }

   fprintf(stdout, " done.\n\n");
}

/* EOF */

// vim:ts=3:expandtab

