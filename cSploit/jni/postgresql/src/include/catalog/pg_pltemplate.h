/*-------------------------------------------------------------------------
 *
 * pg_pltemplate.h
 *	  definition of the system "PL template" relation (pg_pltemplate)
 *	  along with the relation's initial contents.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/pg_pltemplate.h
 *
 * NOTES
 *	  the genbki.pl script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_PLTEMPLATE_H
#define PG_PLTEMPLATE_H

#include "catalog/genbki.h"

/* ----------------
 *		pg_pltemplate definition.  cpp turns this into
 *		typedef struct FormData_pg_pltemplate
 * ----------------
 */
#define PLTemplateRelationId	1136

CATALOG(pg_pltemplate,1136) BKI_SHARED_RELATION BKI_WITHOUT_OIDS
{
	NameData	tmplname;		/* name of PL */
	bool		tmpltrusted;	/* PL is trusted? */
	bool		tmpldbacreate;	/* PL is installable by db owner? */
	text		tmplhandler;	/* name of call handler function */
	text		tmplinline;		/* name of anonymous-block handler, or NULL */
	text		tmplvalidator;	/* name of validator function, or NULL */
	text		tmpllibrary;	/* path of shared library */
	aclitem		tmplacl[1];		/* access privileges for template */
} FormData_pg_pltemplate;

/* ----------------
 *		Form_pg_pltemplate corresponds to a pointer to a row with
 *		the format of pg_pltemplate relation.
 * ----------------
 */
typedef FormData_pg_pltemplate *Form_pg_pltemplate;

/* ----------------
 *		compiler constants for pg_pltemplate
 * ----------------
 */
#define Natts_pg_pltemplate					8
#define Anum_pg_pltemplate_tmplname			1
#define Anum_pg_pltemplate_tmpltrusted		2
#define Anum_pg_pltemplate_tmpldbacreate	3
#define Anum_pg_pltemplate_tmplhandler		4
#define Anum_pg_pltemplate_tmplinline		5
#define Anum_pg_pltemplate_tmplvalidator	6
#define Anum_pg_pltemplate_tmpllibrary		7
#define Anum_pg_pltemplate_tmplacl			8


/* ----------------
 *		initial contents of pg_pltemplate
 * ----------------
 */

DATA(insert ( "plpgsql"		t t "plpgsql_call_handler" "plpgsql_inline_handler" "plpgsql_validator" "$libdir/plpgsql" _null_ ));
DATA(insert ( "pltcl"		t t "pltcl_call_handler" _null_ _null_ "$libdir/pltcl" _null_ ));
DATA(insert ( "pltclu"		f f "pltclu_call_handler" _null_ _null_ "$libdir/pltcl" _null_ ));
DATA(insert ( "plperl"		t t "plperl_call_handler" "plperl_inline_handler" "plperl_validator" "$libdir/plperl" _null_ ));
DATA(insert ( "plperlu"		f f "plperlu_call_handler" "plperlu_inline_handler" "plperlu_validator" "$libdir/plperl" _null_ ));
DATA(insert ( "plpythonu"	f f "plpython_call_handler" "plpython_inline_handler" "plpython_validator" "$libdir/plpython2" _null_ ));
DATA(insert ( "plpython2u"	f f "plpython2_call_handler" "plpython2_inline_handler" "plpython2_validator" "$libdir/plpython2" _null_ ));
DATA(insert ( "plpython3u"	f f "plpython3_call_handler" "plpython3_inline_handler" "plpython3_validator" "$libdir/plpython3" _null_ ));

#endif   /* PG_PLTEMPLATE_H */
