/*-------------------------------------------------------------------------
 *
 * var.h
 *	  prototypes for optimizer/util/var.c.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/optimizer/var.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef VAR_H
#define VAR_H

#include "nodes/relation.h"

typedef enum
{
	PVC_REJECT_AGGREGATES,		/* throw error if Aggref found */
	PVC_INCLUDE_AGGREGATES,		/* include Aggrefs in output list */
	PVC_RECURSE_AGGREGATES		/* recurse into Aggref arguments */
} PVCAggregateBehavior;

typedef enum
{
	PVC_REJECT_PLACEHOLDERS,	/* throw error if PlaceHolderVar found */
	PVC_INCLUDE_PLACEHOLDERS,	/* include PlaceHolderVars in output list */
	PVC_RECURSE_PLACEHOLDERS	/* recurse into PlaceHolderVar arguments */
} PVCPlaceHolderBehavior;

extern Relids pull_varnos(Node *node);
extern void pull_varattnos(Node *node, Bitmapset **varattnos);
extern bool contain_var_clause(Node *node);
extern bool contain_vars_of_level(Node *node, int levelsup);
extern int	locate_var_of_level(Node *node, int levelsup);
extern int	locate_var_of_relation(Node *node, int relid, int levelsup);
extern int	find_minimum_var_level(Node *node);
extern List *pull_var_clause(Node *node, PVCAggregateBehavior aggbehavior,
				PVCPlaceHolderBehavior phbehavior);
extern Node *flatten_join_alias_vars(PlannerInfo *root, Node *node);

#endif   /* VAR_H */
