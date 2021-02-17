/*
    avl.c - AVL tree implementation
    Copyright (C) 1999  Bruno Haible <haible@ma2s2.mathematik.uni-karlsruhe.de>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This AVL tree implementation is based on mm/mmap_avl.c from Linux kernel.
    Written by Bruno Haible <haible@ma2s2.mathematik.uni-karlsruhe.de>.

    Revision history:

	1.00  2001, March 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

 */

//#include "avl_i.h"
#include <stdio.h>
#include <stdlib.h>

struct ncp_avl_node {
	struct ncp_avl_node *avl_left;
	struct ncp_avl_node *avl_right;
	struct ncp_avl_node *next;
	size_t		 avl_height;
};

#define ncp_avl_empty	NULL

/* We keep the list and tree sorted by address. */
#define avl_key		ncp_end
#define ncp_avl_key_t	unsigned long	/* typeof(vma->avl_key) */

/*
 * task->mm->mmap_avl is the AVL tree corresponding to task->mm->mmap
 * or, more exactly, its root.
 * A vm_area_struct has the following fields:
 *   avl_left     left son of a tree node
 *   avl_right    right son of a tree node
 *   avl_height   1+max(heightof(left),heightof(right))
 * The empty tree is represented as NULL.
 */

/* Since the trees are balanced, their height will never be large. */
#define ncp_avl_maxheight	41	/* why this? a small exercise */
#define heightof(tree)	((tree) == ncp_avl_empty ? 0 : (tree)->avl_height)

/*
 * Consistency and balancing rules:
 * 1. tree->avl_height == 1+max(heightof(tree->avl_left),heightof(tree->avl_right))
 * 2. abs( heightof(tree->avl_left) - heightof(tree->avl_right) ) <= 1
 * 3. foreach node in tree->avl_left: node->avl_key <= tree->avl_key,
 *    foreach node in tree->avl_right: node->avl_key >= tree->avl_key.
 */

#ifdef DEBUG_AVL

/* Look up the nodes at the left and at the right of a given node. */
static void avl_neighbours (struct ncp_avl_node * node, struct ncp_avl_node * tree, 
		struct ncp_avl_node ** to_the_left, struct ncp_avl_node ** to_the_right)
{
	ncp_avl_key_t key = node->avl_key;

	*to_the_left = *to_the_right = NULL;
	for (;;) {
		if (tree == ncp_avl_empty) {
			printf("avl_neighbours: node not found in the tree\n");
			return;
		}
		if (key == tree->avl_key)
			break;
		if (key < tree->avl_key) {
			*to_the_right = tree;
			tree = tree->avl_left;
		} else {
			*to_the_left = tree;
			tree = tree->avl_right;
		}
	}
	if (tree != node) {
		printf("avl_neighbours: node not exactly found in the tree\n");
		return;
	}
	if (tree->ncp_avl_left != ncp_avl_empty) {
		struct ncp_avl_node * node;
		for (node = tree->avl_left; node->avl_right != ncp_avl_empty; node = node->avl_right)
			continue;
		*to_the_left = node;
	}
	if (tree->avl_right != ncp_avl_empty) {
		struct ncp_avl_node * node;
		for (node = tree->avl_right; node->avl_left != ncp_avl_empty; node = node->avl_left)
			continue;
		*to_the_right = node;
	}
	if ((*to_the_left && ((*to_the_left)->next != node)) || (node->next != *to_the_right))
		printf("avl_neighbours: tree inconsistent with list\n");
}

#endif

/*
 * Rebalance a tree.
 * After inserting or deleting a node of a tree we have a sequence of subtrees
 * nodes[0]..nodes[k-1] such that
 * nodes[0] is the root and nodes[i+1] = nodes[i]->{avl_left,avl_right}.
 */
static void avl_rebalance (struct ncp_avl_node *** nodeplaces_ptr, int count)
{
	for ( ; count > 0 ; count--) {
		struct ncp_avl_node ** nodeplace = *--nodeplaces_ptr;
		struct ncp_avl_node * node = *nodeplace;
		struct ncp_avl_node * nodeleft = node->avl_left;
		struct ncp_avl_node * noderight = node->avl_right;
		int heightleft = heightof(nodeleft);
		int heightright = heightof(noderight);
		if (heightright + 1 < heightleft) {
			/*                                                      */
			/*                            *                         */
			/*                          /   \                       */
			/*                       n+2      n                     */
			/*                                                      */
			struct ncp_avl_node * nodeleftleft = nodeleft->avl_left;
			struct ncp_avl_node * nodeleftright = nodeleft->avl_right;
			int heightleftright = heightof(nodeleftright);
			if (heightof(nodeleftleft) >= heightleftright) {
				/*                                                        */
				/*                *                    n+2|n+3            */
				/*              /   \                  /    \             */
				/*           n+2      n      -->      /   n+1|n+2         */
				/*           / \                      |    /    \         */
				/*         n+1 n|n+1                 n+1  n|n+1  n        */
				/*                                                        */
				node->avl_left = nodeleftright; nodeleft->avl_right = node;
				nodeleft->avl_height = 1 + (node->avl_height = 1 + heightleftright);
				*nodeplace = nodeleft;
			} else {
				/*                                                        */
				/*                *                     n+2               */
				/*              /   \                 /     \             */
				/*           n+2      n      -->    n+1     n+1           */
				/*           / \                    / \     / \           */
				/*          n  n+1                 n   L   R   n          */
				/*             / \                                        */
				/*            L   R                                       */
				/*                                                        */
				nodeleft->avl_right = nodeleftright->avl_left;
				node->avl_left = nodeleftright->avl_right;
				nodeleftright->avl_left = nodeleft;
				nodeleftright->avl_right = node;
				nodeleft->avl_height = node->avl_height = heightleftright;
				nodeleftright->avl_height = heightleft;
				*nodeplace = nodeleftright;
			}
		}
		else if (heightleft + 1 < heightright) {
			/* similar to the above, just interchange 'left' <--> 'right' */
			struct ncp_avl_node * noderightright = noderight->avl_right;
			struct ncp_avl_node * noderightleft = noderight->avl_left;
			int heightrightleft = heightof(noderightleft);
			if (heightof(noderightright) >= heightrightleft) {
				node->avl_right = noderightleft; noderight->avl_left = node;
				noderight->avl_height = 1 + (node->avl_height = 1 + heightrightleft);
				*nodeplace = noderight;
			} else {
				noderight->avl_left = noderightleft->avl_right;
				node->avl_right = noderightleft->avl_left;
				noderightleft->avl_right = noderight;
				noderightleft->avl_left = node;
				noderight->avl_height = node->avl_height = heightrightleft;
				noderightleft->avl_height = heightright;
				*nodeplace = noderightleft;
			}
		}
		else {
			int height = (heightleft<heightright ? heightright : heightleft) + 1;
			if (height == node->avl_height)
				break;
			node->avl_height = height;
		}
	}
}

/* Insert a node into a tree. */
static inline void avl_insert (struct ncp_avl_node * new_node, struct ncp_avl_node ** ptree)
{
	ncp_avl_key_t key = new_node->avl_key;
	struct ncp_avl_node ** nodeplace = ptree;
	struct ncp_avl_node ** stack[ncp_avl_maxheight];
	int stack_count = 0;
	struct ncp_avl_node *** stack_ptr = &stack[0]; /* = &stack[stackcount] */
	for (;;) {
		struct ncp_avl_node * node = *nodeplace;
		if (node == ncp_avl_empty)
			break;
		*stack_ptr++ = nodeplace; stack_count++;
		if (key < node->avl_key)
			nodeplace = &node->avl_left;
		else
			nodeplace = &node->avl_right;
	}
	new_node->avl_left = ncp_avl_empty;
	new_node->avl_right = ncp_avl_empty;
	new_node->avl_height = 1;
	*nodeplace = new_node;
	avl_rebalance(stack_ptr,stack_count);
}

/* Insert a node into a tree, and
 * return the node to the left of it and the node to the right of it.
 */
static inline void avl_insert_neighbours (struct ncp_avl_node * new_node, struct ncp_avl_node ** ptree,
	struct ncp_avl_node ** to_the_left, struct ncp_avl_node ** to_the_right)
{
	ncp_avl_key_t key = new_node->avl_key;
	struct ncp_avl_node ** nodeplace = ptree;
	struct ncp_avl_node ** stack[ncp_avl_maxheight];
	int stack_count = 0;
	struct ncp_avl_node *** stack_ptr = &stack[0]; /* = &stack[stackcount] */
	*to_the_left = *to_the_right = NULL;
	for (;;) {
		struct ncp_avl_node * node = *nodeplace;
		if (node == ncp_avl_empty)
			break;
		*stack_ptr++ = nodeplace; stack_count++;
		if (key < node->avl_key) {
			*to_the_right = node;
			nodeplace = &node->avl_left;
		} else {
			*to_the_left = node;
			nodeplace = &node->avl_right;
		}
	}
	new_node->avl_left = ncp_avl_empty;
	new_node->avl_right = ncp_avl_empty;
	new_node->avl_height = 1;
	*nodeplace = new_node;
	avl_rebalance(stack_ptr,stack_count);
}

/* Removes a node out of a tree. */
static void avl_remove (struct ncp_avl_node * node_to_delete, struct ncp_avl_node ** ptree)
{
	ncp_avl_key_t key = node_to_delete->avl_key;
	struct ncp_avl_node ** nodeplace = ptree;
	struct ncp_avl_node ** stack[ncp_avl_maxheight];
	int stack_count = 0;
	struct ncp_avl_node *** stack_ptr = &stack[0]; /* = &stack[stackcount] */
	struct ncp_avl_node ** nodeplace_to_delete;
	for (;;) {
		struct ncp_avl_node * node = *nodeplace;
#ifdef DEBUG_AVL
		if (node == ncp_avl_empty) {
			/* what? node_to_delete not found in tree? */
			printf("avl_remove: node to delete not found in tree\n");
			return;
		}
#endif
		*stack_ptr++ = nodeplace; stack_count++;
		if (key == node->avl_key)
			break;
		if (key < node->avl_key)
			nodeplace = &node->avl_left;
		else
			nodeplace = &node->avl_right;
	}
	nodeplace_to_delete = nodeplace;
	/* Have to remove node_to_delete = *nodeplace_to_delete. */
	if (node_to_delete->avl_left == ncp_avl_empty) {
		*nodeplace_to_delete = node_to_delete->avl_right;
		stack_ptr--; stack_count--;
	} else {
		struct ncp_avl_node *** stack_ptr_to_delete = stack_ptr;
		struct ncp_avl_node ** nodeplace = &node_to_delete->avl_left;
		struct ncp_avl_node * node;
		for (;;) {
			node = *nodeplace;
			if (node->avl_right == ncp_avl_empty)
				break;
			*stack_ptr++ = nodeplace; stack_count++;
			nodeplace = &node->avl_right;
		}
		*nodeplace = node->avl_left;
		/* node replaces node_to_delete */
		node->avl_left = node_to_delete->avl_left;
		node->avl_right = node_to_delete->avl_right;
		node->avl_height = node_to_delete->avl_height;
		*nodeplace_to_delete = node; /* replace node_to_delete */
		*stack_ptr_to_delete = &node->avl_left; /* replace &node_to_delete->avl_left */
	}
	avl_rebalance(stack_ptr,stack_count);
}

#ifdef DEBUG_AVL

/* print a list */
static void printf_list (struct ncp_avl_node * vma)
{
	printf("[");
	while (vma) {
		printf("%08lX-%08lX", vma->vm_start, vma->vm_end);
		vma = vma->vm_next;
		if (!vma)
			break;
		printf(" ");
	}
	printf("]");
}

/* print a tree */
static void printf_avl (struct ncp_avl_node * tree)
{
	if (tree != ncp_avl_empty) {
		printf("(");
		if (tree->avl_left != ncp_avl_empty) {
			printf_avl(tree->avl_left);
			printf("<");
		}
		printf("%08lX-%08lX", tree->vm_start, tree->vm_end);
		if (tree->avl_right != ncp_avl_empty) {
			printf(">");
			printf_avl(tree->avl_right);
		}
		printf(")");
	}
}

static char *avl_check_point = "somewhere";

/* check a tree's consistency and balancing */
static void avl_checkheights (struct ncp_avl_node * tree)
{
	int h, hl, hr;

	if (tree == ncp_avl_empty)
		return;
	avl_checkheights(tree->avl_left);
	avl_checkheights(tree->avl_right);
	h = tree->avl_height;
	hl = heightof(tree->avl_left);
	hr = heightof(tree->avl_right);
	if ((h == hl+1) && (hr <= hl) && (hl <= hr+1))
		return;
	if ((h == hr+1) && (hl <= hr) && (hr <= hl+1))
		return;
	printf("%s: avl_checkheights: heights inconsistent\n",avl_check_point);
}

/* check that all values stored in a tree are < key */
static void avl_checkleft (struct ncp_avl_node * tree, ncp_avl_key_t key)
{
	if (tree == ncp_avl_empty)
		return;
	avl_checkleft(tree->avl_left,key);
	avl_checkleft(tree->avl_right,key);
	if (tree->avl_key < key)
		return;
	printf("%s: avl_checkleft: left key %lu >= top key %lu\n",avl_check_point,tree->avl_key,key);
}

/* check that all values stored in a tree are > key */
static void avl_checkright (struct ncp_avl_node * tree, ncp_avl_key_t key)
{
	if (tree == ncp_avl_empty)
		return;
	avl_checkright(tree->avl_left,key);
	avl_checkright(tree->avl_right,key);
	if (tree->avl_key > key)
		return;
	printf("%s: avl_checkright: right key %lu <= top key %lu\n",avl_check_point,tree->avl_key,key);
}

/* check that all values are properly increasing */
static void avl_checkorder (struct ncp_avl_node * tree)
{
	if (tree == ncp_avl_empty)
		return;
	avl_checkorder(tree->avl_left);
	avl_checkorder(tree->avl_right);
	avl_checkleft(tree->avl_left,tree->avl_key);
	avl_checkright(tree->avl_right,tree->avl_key);
}

/* all checks */
static void avl_check (struct task_struct * task, char *caller)
{
	avl_check_point = caller;
/*	printf("task \"%s\", %s\n",task->comm,caller); */
/*	printf("task \"%s\" list: ",task->comm); printf_list(task->mm->mmap); printf("\n"); */
/*	printf("task \"%s\" tree: ",task->comm); printf_avl(task->mm->mmap_avl); printf("\n"); */
	avl_checkheights(task->mm->mmap_avl);
	avl_checkorder(task->mm->mmap_avl);
}

#endif
