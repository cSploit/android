/*
    filter.c - Filters for NWDSSearch function family
    Copyright (C) 1999, 2000  Petr Vandrovec

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

    Revision history:

	1.00  2000, February 6		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release

	1.01  2001, December 16		Petr Vandrovec <vandrove@vc.cvut.cz>, Hans Grobler <grobh@sun.ac.za>
		Fix NWDSAddFilterToken behavior.
		Fix coredumps, endless loops and so on in NWDSPutFilter.

 */

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "ncplib_i.h"
#include "nwnet_i.h"

NWDSCCODE NWDSAllocFilter(Filter_Cursor_T** pcur) {
	Filter_Cursor_T* cur;
	
	cur = (Filter_Cursor_T*)malloc(sizeof(*cur));
	if (!cur)
		return ERR_NOT_ENOUGH_MEMORY;
	cur->fn = NULL;
	cur->level = 0;
	cur->expect = FBIT_OPERAND;
	*pcur = cur;
	return 0;
}

static inline struct _filter_node* NWDSAllocFNode(nuint token) {
	struct _filter_node* x;
	
	x = (struct _filter_node*)malloc(sizeof(*x));
	if (x) {
		x->parent = x->left = x->right = x->value = NULL;
		x->token = token;
		x->syntax = 0;
	}
	return x;
}

static void __FilterNext(Filter_Cursor_T* cur, const struct _filter_node* fn) {
	switch (fn->token) {
		case FTOK_LPAREN:
		case FTOK_OR:
		case FTOK_AND:
		case FTOK_NOT:
			cur->expect = FBIT_OPERAND;
			break;
		case FTOK_EQ:
		case FTOK_GE:
		case FTOK_LE:
		case FTOK_APPROX:
		case FTOK_MODTIME:
		case FTOK_VALTIME:
			if (fn->right) {
				cur->expect = FBIT_BOOLOP | (cur->level ? FBIT_RPAREN : FBIT_END);
				break;
			}
			cur->expect = FBIT_AVAL;
			break;
		case FTOK_PRESENT:
		case FTOK_RDN:
		case FTOK_BASECLS:
			if (fn->right) {
				cur->expect = FBIT_BOOLOP | (cur->level ? FBIT_RPAREN : FBIT_END);
				break;
			}
			cur->expect = FBIT_ANAME;
			break;
		case FTOK_ANAME:
			cur->expect = FBIT_RELOP;
			break;
		case FTOK_AVAL:
		case FTOK_END:
		case FTOK_RPAREN:
		default:
			/* it is not possible! */
			cur->expect = 0;
			break;
	}
		
}

NWDSCCODE NWDSAddFilterToken(
		Filter_Cursor_T* cur,
		nuint token,
		const void* value,
		enum SYNTAX syntax) {
	struct _filter_node* x;
	
	if (!cur)
		return ERR_NULL_POINTER;
	if (!(cur->expect & (1 << token)))
		return ERR_BAD_SYNTAX;
	if (token == FTOK_END) {
		x = cur->fn;
		while (x->parent)
			x = x->parent;
		cur->expect = 0;
	} else if (token == FTOK_RPAREN) {
		for (x = cur->fn; x->token != FTOK_LPAREN; x = x->parent);
		while (x->parent && x->parent->token != FTOK_LPAREN)
			x = x->parent;
		cur->level--;
		cur->expect = FBIT_BOOLOP | (cur->level ? FBIT_RPAREN : FBIT_END);
	} else {
		struct _filter_node* fn;
		static const int prio_n[] =
			/* END OR AND NOT  (  )   val  = */
			/* >= <=  app  ?   ?   ?   name present */
			/* RDN BASECLS MODTIME VALTIME */
			{ -1,  3,  5,  5,  7, -1,  7,  7,
			   7,  7,  7,  7,  7,  7,  7,  7,
			   7,  7,  7,  7 };
		static const int prio_o[] =
			{ -1,  2,  4,  6,  0, -1,  0,  6,
			   6,  6,  6,  6,  6,  6,  8,  6,
			   6,  6,  6,  6 };

		x = NWDSAllocFNode(token);
		if (!x)
			return ERR_NOT_ENOUGH_MEMORY;
		fn = cur->fn;
		if (!fn || (prio_n[token] > prio_o[fn->token])) {
			x->parent = fn;
			if (fn) {
				x->left = fn->right;
				fn->right = x;
				if (x->left)
					x->left->parent = x;
			}
		} else {
			x->left = fn;
			x->parent = fn->parent;
			fn->parent = x;
			if (x->parent)
				x->parent->right = x;
		}
		switch (token) {
			case FTOK_LPAREN:
				cur->level++;
			default:
				__FilterNext(cur, x);
				break;
			case FTOK_ANAME:
				if (!x->parent || ((1 << x->parent->token) & ~(FBIT_PRESENT | FBIT_RDN | FBIT_BASECLS |
						FBIT_MODTIME | FBIT_VALTIME))) {
					x->value = ncp_const_cast(value);
					x->syntax = syntax;
					cur->expect = FBIT_RELOP;
					break;
				}	
			case FTOK_AVAL:
				x->value = ncp_const_cast(value);
				x->syntax = syntax;
				while (x->parent && x->parent->token != FTOK_LPAREN)
					x = x->parent;
				cur->expect = FBIT_BOOLOP | (cur->level ? FBIT_RPAREN : FBIT_END);
				break;
		}
	}
	cur->fn = x;
	return 0;			
}

static inline void NWDSFreeFNode(struct _filter_node* x, void (*freeVal)(enum SYNTAX, void*)) {
	switch (x->token) {
		case FTOK_ANAME:
			if (freeVal)
				freeVal(-1, x->value);
			break;
		case FTOK_AVAL:
			if (freeVal)
				freeVal(x->syntax, x->value);
			break;
	}
	free(x);
}

NWDSCCODE NWDSDelFilterToken(
		Filter_Cursor_T* cur,
		void (*freeVal)(enum SYNTAX, void*)) {
	struct _filter_node* x;
	
	if (!cur)
		return ERR_NULL_POINTER;
	x = cur->fn;
	if (!x)
		return ERR_FILTER_TREE_EMPTY;
        /* go down to non-closed node or LPAREN */
	while (x->right && x->right->token != FTOK_LPAREN)
		x = x->right;
	if (x->token == FTOK_LPAREN) {
		if (x->right) {
                        /* remove virtual FTOK_RPAREN */
			cur->fn = x->right;
			cur->level++;
			cur->expect = FBIT_OPERAND;
			return 0;
		}
                /* remove FTOK_LPAREN */
		cur->level--;
	}
        /*
            PARENT        PARENT
             /  \          /  \
           Y      X  =>  Y      LFT
                /
              LFT
         */
	if (x->left)
		x->left->parent = x->parent;
	if (x->parent)
		x->parent->right = x->left;
	cur->fn = x->left ? x->left : x->parent;
	if (!cur->fn) {
		cur->expect = FBIT_OPERAND;
	} else {
		__FilterNext(cur, cur->fn);
	}
	NWDSFreeFNode(x, freeVal);
	return 0;
}

/*
 * This converts trees of binary ANDs and ORs into n-ary ANDs and ORs.
 * We also eliminate LPAREN during this, as it is was needed only for
 *   creation phase to get NWDSDelFilterToken right.
 */
static void _PutFilterFirstPass(
		struct _filter_node* x,
		void (*freeVal)(enum SYNTAX, void*)) {
	struct _filter_node* q;
	
	while (x) {
		if ((x->token == FTOK_OR) || (x->token == FTOK_AND)) {
			struct _filter_node** last_q;
			
			x->syntax = 2;	/* count... */
			q = x->left;
			q->value = x->right; /* value contains list of same nodes of same type */
			last_q = &x->left;
			while (q) {
				if (x->token == q->token) {
					struct _filter_node* tmp;

                                        /*
                                               X(syntax=2)         X(syntax=3)
                                             /   \               /
                                           Q  ->   RGT  =>   LFT -> MID -> RGT
                                         /   \                ^ next Q
                                      LFT      MID
                                         */
					x->syntax += 1;
					*last_q = q->left;
					q->left->value = q->right;
					q->right->value = q->value;
					q->left->parent = x;
					q->right->parent = x;
					tmp = q;
					q = q->left;
					NWDSFreeFNode(tmp, freeVal);
				} else if (q->token == FTOK_LPAREN) {
					struct _filter_node* tmp;

                                        /*
                                                X                  X
                                              /   \              /   \
                                            Q  ->   RGT  =>  MID  ->   RGT
                                              \               ^ next Q
                                                MID
                                         */
					*last_q = q->right;
					q->right->value = q->value;
					q->right->parent = x;
					tmp = q;
					q = q->right;
					NWDSFreeFNode(tmp, freeVal);
				} else {
                                        /* different operand: go to next node in created
                                           chain */
					last_q = (struct _filter_node**)&q->value;
					q = q->value;
				}
			}
			x->right = x->left;
		} else if (x->token == FTOK_NOT) {
			for (q = x->right; q->token == FTOK_LPAREN; ) {
				struct _filter_node* tmp;
					
				tmp = q;
				q = q->right;
				NWDSFreeFNode(tmp, freeVal);
			}
			x->left = q;
			q->parent = x;
			x->right = x->left;
		}
		for (; x; x = x->parent) {
			if ((x->token == FTOK_AND || x->token == FTOK_OR || x->token == FTOK_NOT) && x->right) {
				q = x->right;
				x->right = x->right->value;
				x = q;
				break;
			}
		}
	}
}

#define FTAG_ITEM	0
#define FTAG_OR		1
#define FTAG_AND	2
#define FTAG_NOT	3

static NWDSCCODE _PutFilterSecondPass(
		NWDSContextHandle ctx,
		Buf_T* buf,
		struct _filter_node* x,
		void (*freeVal)(enum SYNTAX, void*)) {
	NWDSCCODE err;
	
	err = 0;
	while (x) {
		if (x->token == FTOK_AND || x->token == FTOK_OR || x->token == FTOK_NOT) {
			x->right = x->left;
			if (!err) {
				nuint32 op;

				if (x->token == FTOK_OR)
					op = FTAG_OR;
				else if (x->token == FTOK_AND)
					op = FTAG_AND;
				else
					op = FTAG_NOT;
				err = NWDSBufPutLE32(buf, op);
				if (x->token != FTOK_NOT) {
					if (!err)
						err = NWDSBufPutLE32(buf, x->syntax);
				}
			}
		} else {
			if (!err) {
				void* p = NWDSBufPutPtr(buf, 8);
				if (!p)
					err = ERR_BUFFER_FULL;
				else {
					DSET_LH(p, 0, FTAG_ITEM);
					DSET_LH(p, 4, x->token);
					switch (x->token) {
						case FTOK_BASECLS:
						case FTOK_RDN:
						case FTOK_PRESENT:
							err = NWDSPutAttrVal_XX_STRING(ctx, buf, x->right->value);
							break;
						case FTOK_MODTIME:
						case FTOK_VALTIME:
							err = NWDSPutAttrVal_TIMESTAMP(ctx, buf, x->right->value);
							break;
						default: /* RELOP */
							err = NWDSPutAttrNameAndVal(ctx, buf, x->left->value, x->right->syntax, x->right->value);
							break;
					}
				}
			}
		}
		while (x) {
			if (x->token == FTOK_AND || x->token == FTOK_OR || x->token == FTOK_NOT) {
				if (x->right) {
					struct _filter_node* tmp;
					tmp = x->right;
					x->right = x->right->value;
					x = tmp;
					break;
				} else {
					struct _filter_node* tmp;

					tmp = x;
					x = x->parent;
					NWDSFreeFNode(tmp, freeVal);
				}
			} else {
				struct _filter_node* tmp;
				if (x->left)
					NWDSFreeFNode(x->left, freeVal);
				if (x->right)
					NWDSFreeFNode(x->right, freeVal);
				tmp = x;
				x = x->parent;
				NWDSFreeFNode(tmp, freeVal);
			}
		}	
	}
	return err;
}

NWDSCCODE NWDSPutFilter(
		NWDSContextHandle ctx,
		Buf_T* buf,
		Filter_Cursor_T* cur,
		void (*freeVal)(enum SYNTAX, void*)) {
	struct _filter_node* x;
	NWDSCCODE err;

	if (!buf)
		return ERR_NULL_POINTER;
	if (buf->operation != DSV_SEARCH_FILTER)
		return ERR_BAD_VERB;
	if (!cur || !cur->fn) {
		void* p = NWDSBufPutPtr(buf, 8);
		if (!p)
			return ERR_BUFFER_FULL;
		DSET_LH(p, 0, FTAG_AND);
		DSET_LH(p, 0, 0);
		return 0;
	}
        /*
        if (cur->expect) {
                NWDSFreeFilter(cur, freeVal);
                return ERR_YOU_DID_NOT_CLOSE_FILTER_WITH_FTOK_END;
        }
        */
	for (x = cur->fn; x->parent; x = x->parent);
	while (x->token == FTOK_LPAREN) {
		struct _filter_node* tmp;

		tmp = x;
		x = x->right;
		NWDSFreeFNode(tmp, freeVal);
		x->parent = NULL;
	}
	_PutFilterFirstPass(x, freeVal);
	err = _PutFilterSecondPass(ctx, buf, x, freeVal);
	free(cur);
	return err;	
}

static void __NDSFreeFilter(
		struct _filter_node* x,
		void (*freeVal)(enum SYNTAX, void*)) {
	while (x) {
		struct _filter_node* tmp;
		
		__NDSFreeFilter(x->left, freeVal);
		tmp = x;
		x = x->right;
		NWDSFreeFNode(tmp, freeVal);
	}
}

NWDSCCODE NWDSFreeFilter(
		Filter_Cursor_T* cur,
		void (*freeVal)(enum SYNTAX, void*)) {
	struct _filter_node* x;
	
	for (x = cur->fn; x->parent; x = x->parent);
	__NDSFreeFilter(x, freeVal);
	free(cur);
	return 0;
}

