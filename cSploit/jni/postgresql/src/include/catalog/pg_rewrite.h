/*-------------------------------------------------------------------------
 *
 * pg_rewrite.h
 *	  definition of the system "rewrite-rule" relation (pg_rewrite)
 *	  along with the relation's initial contents.
 *
 * As of Postgres 7.3, the primary key for this table is <ev_class, rulename>
 * --- ie, rule names are only unique among the rules of a given table.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/pg_rewrite.h
 *
 * NOTES
 *	  the genbki.pl script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_REWRITE_H
#define PG_REWRITE_H

#include "catalog/genbki.h"

/* ----------------
 *		pg_rewrite definition.	cpp turns this into
 *		typedef struct FormData_pg_rewrite
 * ----------------
 */
#define RewriteRelationId  2618

CATALOG(pg_rewrite,2618)
{
	NameData	rulename;
	Oid			ev_class;
	int2		ev_attr;
	char		ev_type;
	char		ev_enabled;
	bool		is_instead;

	/* NB: remaining fields must be accessed via heap_getattr */
	pg_node_tree ev_qual;
	pg_node_tree ev_action;
} FormData_pg_rewrite;

/* ----------------
 *		Form_pg_rewrite corresponds to a pointer to a tuple with
 *		the format of pg_rewrite relation.
 * ----------------
 */
typedef FormData_pg_rewrite *Form_pg_rewrite;

/* ----------------
 *		compiler constants for pg_rewrite
 * ----------------
 */
#define Natts_pg_rewrite				8
#define Anum_pg_rewrite_rulename		1
#define Anum_pg_rewrite_ev_class		2
#define Anum_pg_rewrite_ev_attr			3
#define Anum_pg_rewrite_ev_type			4
#define Anum_pg_rewrite_ev_enabled		5
#define Anum_pg_rewrite_is_instead		6
#define Anum_pg_rewrite_ev_qual			7
#define Anum_pg_rewrite_ev_action		8

#endif   /* PG_REWRITE_H */
