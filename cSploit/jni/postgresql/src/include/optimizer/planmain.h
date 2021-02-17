/*-------------------------------------------------------------------------
 *
 * planmain.h
 *	  prototypes for various files in optimizer/plan
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/optimizer/planmain.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PLANMAIN_H
#define PLANMAIN_H

#include "nodes/plannodes.h"
#include "nodes/relation.h"

/* GUC parameters */
#define DEFAULT_CURSOR_TUPLE_FRACTION 0.1
extern double cursor_tuple_fraction;

/*
 * prototypes for plan/planmain.c
 */
extern void query_planner(PlannerInfo *root, List *tlist,
			  double tuple_fraction, double limit_tuples,
			  Path **cheapest_path, Path **sorted_path,
			  double *num_groups);

/*
 * prototypes for plan/planagg.c
 */
extern void preprocess_minmax_aggregates(PlannerInfo *root, List *tlist);
extern Plan *optimize_minmax_aggregates(PlannerInfo *root, List *tlist,
						   const AggClauseCosts *aggcosts, Path *best_path);

/*
 * prototypes for plan/createplan.c
 */
extern Plan *create_plan(PlannerInfo *root, Path *best_path);
extern SubqueryScan *make_subqueryscan(List *qptlist, List *qpqual,
				  Index scanrelid, Plan *subplan,
				  List *subrtable, List *subrowmark);
extern Append *make_append(List *appendplans, List *tlist);
extern RecursiveUnion *make_recursive_union(List *tlist,
					 Plan *lefttree, Plan *righttree, int wtParam,
					 List *distinctList, long numGroups);
extern Sort *make_sort_from_pathkeys(PlannerInfo *root, Plan *lefttree,
						List *pathkeys, double limit_tuples);
extern Sort *make_sort_from_sortclauses(PlannerInfo *root, List *sortcls,
						   Plan *lefttree);
extern Sort *make_sort_from_groupcols(PlannerInfo *root, List *groupcls,
						 AttrNumber *grpColIdx, Plan *lefttree);
extern Agg *make_agg(PlannerInfo *root, List *tlist, List *qual,
		 AggStrategy aggstrategy, const AggClauseCosts *aggcosts,
		 int numGroupCols, AttrNumber *grpColIdx, Oid *grpOperators,
		 long numGroups,
		 Plan *lefttree);
extern WindowAgg *make_windowagg(PlannerInfo *root, List *tlist,
			   List *windowFuncs, Index winref,
			   int partNumCols, AttrNumber *partColIdx, Oid *partOperators,
			   int ordNumCols, AttrNumber *ordColIdx, Oid *ordOperators,
			   int frameOptions, Node *startOffset, Node *endOffset,
			   Plan *lefttree);
extern Group *make_group(PlannerInfo *root, List *tlist, List *qual,
		   int numGroupCols, AttrNumber *grpColIdx, Oid *grpOperators,
		   double numGroups,
		   Plan *lefttree);
extern Plan *materialize_finished_plan(Plan *subplan);
extern Unique *make_unique(Plan *lefttree, List *distinctList);
extern LockRows *make_lockrows(Plan *lefttree, List *rowMarks, int epqParam);
extern Limit *make_limit(Plan *lefttree, Node *limitOffset, Node *limitCount,
		   int64 offset_est, int64 count_est);
extern SetOp *make_setop(SetOpCmd cmd, SetOpStrategy strategy, Plan *lefttree,
		   List *distinctList, AttrNumber flagColIdx, int firstFlag,
		   long numGroups, double outputRows);
extern Result *make_result(PlannerInfo *root, List *tlist,
			Node *resconstantqual, Plan *subplan);
extern ModifyTable *make_modifytable(CmdType operation, bool canSetTag,
				 List *resultRelations, List *subplans, List *returningLists,
				 List *rowMarks, int epqParam);
extern bool is_projection_capable_plan(Plan *plan);

/*
 * prototypes for plan/initsplan.c
 */
extern int	from_collapse_limit;
extern int	join_collapse_limit;

extern void add_base_rels_to_query(PlannerInfo *root, Node *jtnode);
extern void build_base_rel_tlists(PlannerInfo *root, List *final_tlist);
extern void add_vars_to_targetlist(PlannerInfo *root, List *vars,
					   Relids where_needed, bool create_new_ph);
extern List *deconstruct_jointree(PlannerInfo *root);
extern void distribute_restrictinfo_to_rels(PlannerInfo *root,
								RestrictInfo *restrictinfo);
extern void process_implied_equality(PlannerInfo *root,
						 Oid opno,
						 Oid collation,
						 Expr *item1,
						 Expr *item2,
						 Relids qualscope,
						 Relids nullable_relids,
						 bool below_outer_join,
						 bool both_const);
extern RestrictInfo *build_implied_join_equality(Oid opno,
							Oid collation,
							Expr *item1,
							Expr *item2,
							Relids qualscope,
							Relids nullable_relids);

/*
 * prototypes for plan/analyzejoins.c
 */
extern List *remove_useless_joins(PlannerInfo *root, List *joinlist);

/*
 * prototypes for plan/setrefs.c
 */
extern Plan *set_plan_references(PlannerGlobal *glob,
					Plan *plan,
					List *rtable,
					List *rowmarks);
extern void fix_opfuncids(Node *node);
extern void set_opfuncid(OpExpr *opexpr);
extern void set_sa_opfuncid(ScalarArrayOpExpr *opexpr);
extern void record_plan_function_dependency(PlannerGlobal *glob, Oid funcid);
extern void extract_query_dependencies(Node *query,
						   List **relationOids,
						   List **invalItems);

#endif   /* PLANMAIN_H */
