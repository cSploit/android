/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2011  James K. Lowden
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdarg.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <assert.h>
#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_ERRNO_H
# include <errno.h>
#endif /* HAVE_ERRNO_H */

 
#include <freetds/tds.h>
#include <freetds/thread.h>
#include <freetds/convert.h>
#include <replacements.h>
#include <sybfront.h>
#include <sybdb.h>
#include <syberror.h>
#include <dblib.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: dbpivot.c,v 1.3 2012-03-09 19:20:30 freddy77 Exp $");

#define TDS_FIND(k,b,c) tds_find(k, b, sizeof(b)/sizeof(b[0]), sizeof(b[0]), c)

/* avoid conflicts */
#define boolean TDS_boolean
typedef enum { false, true } boolean;

typedef boolean (*compare_func)(const void *, const void *);

static void *
tds_find(const void *key, const void *base, size_t nelem, size_t width,
         compare_func compar)
{
	size_t i;
	for (i=0; i < nelem; i++) {
		char *p = (char*)base + width * i;
		if (true == compar(key, p)) {
			return p;
		}
	}
	return NULL;
}


struct col_t
{
	int type;
	size_t len;
	int null_indicator;
	char *s;
	union {
		DBTINYINT 	ti;
		DBSMALLINT	si;
		DBINT		i;
		DBREAL		r;
		DBFLT8		f;
	};
};

static int infer_col_type(int sybtype);

static struct col_t *
col_init(struct col_t *pcol, int sybtype, int collen) 
{
	assert(pcol);
	
	pcol->type = infer_col_type(sybtype);
	pcol->len = collen;

	switch(sybtype) {
	case 0:
		pcol->len = 0;
		return NULL;
	case SYBDATETIME:
	case SYBDATETIME4:
	case SYBDATETIMN:
		collen = 30;
	case SYBCHAR:
	case SYBVARCHAR:
	case SYBTEXT:
	case SYBNTEXT:
		pcol->len = collen;
		if ((pcol->s = malloc(1+collen)) == NULL) {
			return NULL;
		}
		break;
	}
	return pcol;
}

static void
col_free(struct col_t *p)
{
	free(p->s);
	memset(p, 0, sizeof(*p));
}

static boolean 
col_equal(const struct col_t *pc1, const struct col_t *pc2)
{
	assert( pc1 && pc2 );
	assert( pc1->type == pc2->type );
	
	switch(pc1->type) {
	
	case SYBCHAR:
	case SYBVARCHAR:
		if( pc1->len != pc2->len)
			return false;
		return strncmp(pc1->s, pc2->s, pc1->len) == 0? true : false;
	case SYBINT1:
		return pc1->ti == pc2->ti? true : false;
	case SYBINT2:
		return pc1->si == pc2->si? true : false;
	case SYBINT4:
		return pc1->i == pc2->i? true : false;
	case SYBFLT8:
		return pc1->f == pc2->f? true : false;
	case SYBREAL:
		return pc1->r == pc2->r? true : false;

	case SYBINTN:
	case SYBDATETIME:
	case SYBBIT:
	case SYBTEXT:
	case SYBNTEXT:
	case SYBIMAGE:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBDATETIME4:
	case SYBBINARY:
	case SYBVOID:
	case SYBVARBINARY:
	case SYBBITN:
	case SYBNUMERIC:
	case SYBDECIMAL:
	case SYBFLTN:
	case SYBMONEYN:
	case SYBDATETIMN:
		assert( false && pc1->type );
		break;
	}
	return false;
}

static void *
col_buffer(struct col_t *pcol) 
{
	switch(pcol->type) {
	
	case SYBCHAR:
	case SYBVARCHAR:
		return pcol->s;
	case SYBINT1:
		return &pcol->ti;
	case SYBINT2:
		return &pcol->si;
	case SYBINT4:
		return &pcol->i;
	case SYBFLT8:
		return &pcol->f;
	case SYBREAL:
		return &pcol->r;

	case SYBINTN:
	case SYBDATETIME:
	case SYBBIT:
	case SYBTEXT:
	case SYBNTEXT:
	case SYBIMAGE:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBDATETIME4:
	case SYBBINARY:
	case SYBVOID:
	case SYBVARBINARY:
	case SYBBITN:
	case SYBNUMERIC:
	case SYBDECIMAL:
	case SYBFLTN:
	case SYBMONEYN:
	case SYBDATETIMN:
		assert( false && pcol->type );
		break;
	}
	return false;

}

#if 0
static int
col_print(FILE* out, const struct col_t *pcol) 
{
	char *fmt;
	
	switch(pcol->type) {
	
	case SYBCHAR:
	case SYBVARCHAR:
		return (int) fwrite(pcol->s, pcol->len, 1, out);
	case SYBINT1:
		return fprintf(out, "%d", (int)pcol->ti);
	case SYBINT2:
		return fprintf(out, "%d", (int)pcol->si);
	case SYBINT4:
		return fprintf(out, "%d", (int)pcol->i);
	case SYBFLT8:
		return fprintf(out, "%f",      pcol->f);
	case SYBREAL:
		return fprintf(out, "%f", (double)pcol->r);

	case SYBINTN:
	case SYBDATETIME:
	case SYBBIT:
	case SYBTEXT:
	case SYBNTEXT:
	case SYBIMAGE:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBDATETIME4:
	case SYBBINARY:
	case SYBVOID:
	case SYBVARBINARY:
	case SYBBITN:
	case SYBNUMERIC:
	case SYBDECIMAL:
	case SYBFLTN:
	case SYBMONEYN:
	case SYBDATETIMN:
		assert( false && pcol->type );
		break;
	}
	return false;
}
#endif
static struct col_t *
col_cpy(struct col_t *pdest, const struct col_t *psrc)
{
	assert( pdest && psrc );
	assert( psrc->len > 0 || psrc->null_indicator == -1);
	
	memcpy(pdest, psrc, sizeof(*pdest));
	
	if (psrc->s) {
		assert(psrc->len >= 0);
		if ((pdest->s = malloc(psrc->len)) == NULL)
			return NULL;
		memcpy(pdest->s, psrc->s, psrc->len);
	}
	
	assert( pdest->len > 0 || pdest->null_indicator == -1);
	return pdest;
}

static boolean
col_null( const struct col_t *pcol )
{
	assert(pcol);
	return pcol->null_indicator == -1? true : false;
} 

static char *
string_value(const struct col_t *pcol)
{
	char *output = NULL;
	int len = -1;

	switch(pcol->type) {
	case SYBCHAR:
	case SYBVARCHAR:
		if ((output = calloc(1, 1 + pcol->len)) == NULL)
			return NULL;
		strncpy(output, pcol->s, pcol->len);
		return output;
		break;
	case SYBINT1:
		len = asprintf(&output, "%d", (int)pcol->ti);
		break;
	case SYBINT2:
		len = asprintf(&output, "%d", (int)pcol->si);
		break;
	case SYBINT4:
		len = asprintf(&output, "%d", (int)pcol->i);
		break;
	case SYBFLT8:
		len = asprintf(&output, "%f", pcol->f);
		break;
	case SYBREAL:
		len = asprintf(&output, "%f", (double)pcol->r);
		break;

	default:
	case SYBINTN:
	case SYBDATETIME:
	case SYBBIT:
	case SYBTEXT:
	case SYBNTEXT:
	case SYBIMAGE:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBDATETIME4:
	case SYBBINARY:
	case SYBVOID:
	case SYBVARBINARY:
	case SYBBITN:
	case SYBNUMERIC:
	case SYBDECIMAL:
	case SYBFLTN:
	case SYBMONEYN:
	case SYBDATETIMN:
		assert( false && pcol->type );
		return NULL;
		break;
	}

	return len >= 0? output : NULL;
}

static char *
join(int argc, char *argv[], const char sep[])
{
	size_t len = 0;
	char **p, *output;
		
	for (p=argv; p < argv + argc; p++) {
		len += strlen(*p);
	}
	
	len += 1 + argc * strlen(sep); /* allows one too many */ 
	
	output = calloc(1, len);
	
	for (p=argv; p < argv + argc; p++) {
		if (p != argv)
			strcat(output, sep);
		strcat(output, *p);
	}
	return output;
}

static int
infer_col_type(int sybtype) 
{
	switch(sybtype) {
	case SYBCHAR:
	case SYBVARCHAR:
	case SYBTEXT:
	case SYBNTEXT:
		return SYBCHAR;
	case SYBDATETIME:
	case SYBDATETIME4:
	case SYBDATETIMN:
		return SYBCHAR;
	case SYBINT1:
	case SYBBIT:
	case SYBBITN:
		return SYBINT1;
	case SYBINT2:
		return SYBINT2;
	case SYBINT4:
	case SYBINTN:
		return SYBINT4;
	case SYBFLT8:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBFLTN:
	case SYBMONEYN:
	case SYBNUMERIC:
	case SYBDECIMAL:
		return SYBFLT8;
	case SYBREAL:
		return SYBREAL;

	case SYBIMAGE:
	case SYBBINARY:
	case SYBVOID:
	case SYBVARBINARY:
		assert( false && sybtype );
		break;
	}
	return 0;
}

static int
bind_type(int sybtype)
{
	switch(sybtype) {
	case SYBCHAR:
	case SYBVARCHAR:
	case SYBTEXT:
	case SYBNTEXT:
	case SYBDATETIME:
	case SYBDATETIME4:
	case SYBDATETIMN:
		return NTBSTRINGBIND;
	case SYBINT1:
	case SYBBIT:
	case SYBBITN:
		return TINYBIND;
	case SYBINT2:
		return SMALLBIND;
	case SYBINT4:
	case SYBINTN:
		return INTBIND;
	case SYBFLT8:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBFLTN:
	case SYBMONEYN:
	case SYBNUMERIC:
	case SYBDECIMAL:
		return FLT8BIND;
	case SYBREAL:
		return REALBIND;

	case SYBIMAGE:
	case SYBBINARY:
	case SYBVOID:
	case SYBVARBINARY:
		assert( false && sybtype );
		break;
	}
	return 0;
}

struct key_t { int nkeys; struct col_t *keys; }; 

static boolean
key_equal(const void *a, const void *b)
{
	const struct key_t *p1 = a, *p2 = b;
	int i;
	
	assert(a && b);
	assert(p1->keys && p2->keys);
	assert(p1->nkeys == p2->nkeys);
	
	for( i=0; i < p1->nkeys; i++ ) {
		if (! col_equal(p1->keys+i, p2->keys+i))
			return false;
	}
	return true;
}


static void
key_free(struct key_t *p)
{
	col_free(p->keys);
	free(p->keys);
	memset(p, 0, sizeof(*p));
}

static struct key_t *
key_cpy(struct key_t *pdest, const struct key_t *psrc)
{
	int i;
	
	assert( pdest && psrc );
	
	if ((pdest->keys = calloc(psrc->nkeys, sizeof(*pdest->keys))) == NULL)
		return NULL;

	pdest->nkeys = psrc->nkeys;
	
	for( i=0; i < psrc->nkeys; i++) {
		if (NULL == col_cpy(pdest->keys+i, psrc->keys+i))
			return NULL;
	}

	return pdest;
}


static char *
make_col_name(const struct key_t *k)
{
	const struct col_t *pc;
	char **names, **s, *output;
	
	assert(k);
	assert(k->nkeys);
	assert(k->keys);
	
	s = names = calloc(k->nkeys, sizeof(char*));
	
	for(pc=k->keys; pc < k->keys + k->nkeys; pc++) {
		*s++ = strdup(string_value(pc));
	}
	
	output = join(k->nkeys, names, "/");
	
	for(s=names; s < names + k->nkeys; s++) {
		free(*s);
	}
	free(names);
	
	return output;
}
	

struct agg_t { struct key_t row_key, col_key; struct col_t value; }; 

#if 0
static boolean
agg_key_equal(const void *a, const void *b)
{
	int i;
	const struct agg_t *p1 = a, *p2 = b;
	
	assert(p1 && p2);
	assert(p1->row_key.keys  && p2->row_key.keys);
	assert(p1->row_key.nkeys == p2->row_key.nkeys);
	
	for( i=0; i < p1->row_key.nkeys; i++ ) {
		if (! col_equal(p1->row_key.keys+i, p2->row_key.keys+i))
			return false;
	}

	return true;
}
#endif

static boolean
agg_next(const void *a, const void *b)
{
	int i;
	const struct agg_t *p1 = a, *p2 = b;
	
	assert(p1 && p2);
	
	if (p1->row_key.keys == NULL || p2->row_key.keys == NULL)
		return false;
	
	assert(p1->row_key.keys  && p2->row_key.keys);
	assert(p1->row_key.nkeys == p2->row_key.nkeys);
	
	assert(p1->col_key.keys  && p2->col_key.keys);
	assert(p1->col_key.nkeys == p2->col_key.nkeys);
	
	for( i=0; i < p1->row_key.nkeys; i++ ) {
		assert(p1->row_key.keys[i].type);
		assert(p2->row_key.keys[i].type);
		if (p1->row_key.keys[i].type != p2->row_key.keys[i].type)
			return false;
	}

	for( i=0; i < p1->row_key.nkeys; i++ ) {
		if (! col_equal(p1->row_key.keys+i, p2->row_key.keys+i))
			return false;
	}

	for( i=0; i < p1->col_key.nkeys; i++ ) {
		if (p1->col_key.keys[i].type != p2->col_key.keys[i].type)
			return false;
	}

	for( i=0; i < p1->col_key.nkeys; i++ ) {
		if (! col_equal(p1->col_key.keys+i, p2->col_key.keys+i))
			return false;
	}

	return true;
}

static void
agg_free(struct agg_t *p)
{
	key_free(&p->row_key);
	key_free(&p->col_key);
	col_free(&p->value);
}

static boolean
agg_equal(const void *agg_t1, const void *agg_t2)
{
	const struct agg_t *p1 = agg_t1, *p2 = agg_t2;
	int i;
	
	assert(agg_t1 && agg_t2);
	assert(p1->row_key.keys && p1->col_key.keys);
	assert(p2->row_key.keys && p2->col_key.keys);

	assert(p1->row_key.nkeys == p2->row_key.nkeys);
	assert(p1->col_key.nkeys == p2->col_key.nkeys);
	
	/* todo: use key_equal */
	for( i=0; i < p1->row_key.nkeys; i++ ) {
		if (! col_equal(p1->row_key.keys+i, p2->row_key.keys+i))
			return false;
	}
	for( i=0; i < p1->col_key.nkeys; i++ ) {
		if (! col_equal(p1->col_key.keys+i, p2->col_key.keys+i))
			return false;
	}
	return true;
}

#undef TEST_MALLOC
#define TEST_MALLOC(dest,type) \
	{if (!(dest = (type*)calloc(1, sizeof(type)))) goto Cleanup;}

#undef TEST_CALLOC
#define TEST_CALLOC(dest,type,n) \
	{if (!(dest = (type*)calloc((n), sizeof(type)))) goto Cleanup;}

#define tds_alloc_column() ((TDSCOLUMN*) calloc(1, sizeof(TDSCOLUMN)))

static TDSRESULTINFO *
alloc_results(size_t num_cols)
{
	TDSRESULTINFO *res_info;
	TDSCOLUMN **ppcol;

	TEST_MALLOC(res_info, TDSRESULTINFO);
	res_info->ref_count = 1;
	TEST_CALLOC(res_info->columns, TDSCOLUMN *, num_cols);
	
	for (ppcol = res_info->columns; ppcol < res_info->columns + num_cols; ppcol++)
		if ((*ppcol = tds_alloc_column()) == NULL)
			goto Cleanup;
	res_info->num_cols = num_cols;
	res_info->row_size = 0;
	return res_info;

      Cleanup:
	tds_free_results(res_info);
	return NULL;
}

static TDSRET
set_result_column(TDSSOCKET * tds, TDSCOLUMN * curcol, const char name[], const struct col_t *pvalue)
{
	assert(curcol && pvalue);
	assert(name);

	curcol->column_usertype = pvalue->type;
	curcol->column_nullable = true;
	curcol->column_writeable = false;
	curcol->column_identity = false;

	tds_set_column_type(tds->conn, curcol, pvalue->type);	/* sets "cardinal" type */

	curcol->column_timestamp = (curcol->column_type == SYBBINARY && curcol->column_usertype == TDS_UT_TIMESTAMP);

#if 0
	curcol->funcs->get_info(tds, curcol);
#endif
	curcol->on_server.column_size = curcol->column_size;

	strcpy(curcol->column_name, name);
	curcol->column_namelen = strlen(name);

	tdsdump_log(TDS_DBG_INFO1, "tds7_get_data_info: \n"
		    "\tcolname = %s (%d bytes)\n"
		    "\ttype = %d (%s)\n"
		    "\tserver's type = %d (%s)\n"
		    "\tcolumn_varint_size = %d\n"
		    "\tcolumn_size = %d (%d on server)\n",
		    curcol->column_name, curcol->column_namelen, 
		    curcol->column_type, tds_prtype(curcol->column_type), 
		    curcol->on_server.column_type, tds_prtype(curcol->on_server.column_type), 
		    curcol->column_varint_size,
		    curcol->column_size, curcol->on_server.column_size);

	return TDS_SUCCESS;
}

struct metadata_t { struct key_t *pacross; char *name; struct col_t col; };


static boolean
reinit_results(TDSSOCKET * tds, size_t num_cols, const struct metadata_t meta[])
{
	TDSRESULTINFO *info;
	int i;

	assert(tds);
	assert(num_cols);
	assert(meta);
	
	tds_free_all_results(tds);
	tds->rows_affected = TDS_NO_COUNT;

	if ((info = alloc_results(num_cols)) == NULL)
		return false;

	tds_set_current_results(tds, info);
	if (tds->cur_cursor) {
		tds_free_results(tds->cur_cursor->res_info);
		tds->cur_cursor->res_info = info;
		tdsdump_log(TDS_DBG_INFO1, "set current_results to cursor->res_info\n");
	} else {
		tds->res_info = info;
		tdsdump_log(TDS_DBG_INFO1, "set current_results (%u column%s) to tds->res_info\n", (unsigned) num_cols, (num_cols==1? "":"s"));
	}

	tdsdump_log(TDS_DBG_INFO1, "setting up %u columns\n", (unsigned) num_cols);
	
	for (i = 0; i < num_cols; i++) {
		set_result_column(tds, info->columns[i], meta[i].name, &meta[i].col);
		info->columns[i]->bcp_terminator = (char*) meta[i].pacross;	/* overload available pointer */
	}
		
	if (num_cols > 0) {
		static char dashes[31] = "------------------------------";
		tdsdump_log(TDS_DBG_INFO1, " %-20s %-15s %-15s %-7s\n", "name", "size/wsize", "type/wtype", "utype");
		tdsdump_log(TDS_DBG_INFO1, " %-20s %15s %15s %7s\n", dashes+10, dashes+30-15, dashes+30-15, dashes+30-7);
	}
	for (i = 0; i < num_cols; i++) {
		char name[TDS_SYSNAME_SIZE] = {'\0'};
		TDSCOLUMN *curcol = info->columns[i];

		if (curcol->column_namelen > 0) {
			memcpy(name, curcol->column_name, curcol->column_namelen);
			name[curcol->column_namelen] = '\0';
		}
		tdsdump_log(TDS_DBG_INFO1, " %-20s %7d/%-7d %7d/%-7d %7d\n", 
						name, 
						curcol->column_size, curcol->on_server.column_size, 
						curcol->column_type, curcol->on_server.column_type, 
						curcol->column_usertype);
	}

#if 1
	/* all done now allocate a row for tds_process_row to use */
	if (TDS_FAILED(tds_alloc_row(info))) return false;
#endif
	return true;
}

struct pivot_t
{
	DBPROCESS *dbproc;
	STATUS status;
	DB_RESULT_STATE dbresults_state;
	
	struct agg_t *output;
	struct key_t *across;
	size_t nout, nacross;
};

static boolean 
pivot_key_equal(const void *a, const void *b)
{
	const struct pivot_t *pa = a, *pb = b;
	assert(a && b);
	
	return pa->dbproc == pb->dbproc? true : false;
}

static struct pivot_t *pivots = NULL;
static size_t npivots = 0;

struct pivot_t *
dbrows_pivoted(DBPROCESS *dbproc)
{
	struct pivot_t P;

	assert(dbproc);
	P.dbproc = dbproc;
	
	return tds_find(&P, pivots, npivots, sizeof(*pivots), pivot_key_equal); 
}

STATUS
dbnextrow_pivoted(DBPROCESS *dbproc, struct pivot_t *pp)
{
	int i;
	struct agg_t candidate, *pout;

	assert(pp);
	assert(dbproc && dbproc->tds_socket);
	assert(dbproc->tds_socket->res_info);
	assert(dbproc->tds_socket->res_info->columns || 0 == dbproc->tds_socket->res_info->num_cols);
	
	for (pout = pp->output; pout < pp->output + pp->nout; pout++) {
		if (pout->row_key.keys != NULL)
			break;
	}

	if (pout == pp->output + pp->nout) {
		dbproc->dbresults_state = _DB_RES_NEXT_RESULT;
		return NO_MORE_ROWS;
	}

	memset(&candidate, 0, sizeof(candidate));
	key_cpy(&candidate.row_key, &pout->row_key);
	
	/* "buffer_transfer_bound_data" */
	for (i = 0; i < dbproc->tds_socket->res_info->num_cols; i++) {
		struct col_t *pval = NULL;
		TDSCOLUMN *pcol = dbproc->tds_socket->res_info->columns[i];
		assert(pcol);
		
		if (pcol->column_nullbind) {
			if (pcol->column_cur_size < 0) {
				*(DBINT *)(pcol->column_nullbind) = -1;
			} else {
				*(DBINT *)(pcol->column_nullbind) = 0;
			}
		}
		if (!pcol->column_varaddr) {
			fprintf(stderr, "no pcol->column_varaddr in col %d\n", i);
			continue;
		}

		/* find column in output */
		if (pcol->bcp_terminator == NULL) { /* not a cross-tab column */
			pval = &candidate.row_key.keys[i];
		} else {
			struct agg_t *pcan;
			key_cpy(&candidate.col_key, (struct key_t *) pcol->bcp_terminator);
			if ((pcan = tds_find(&candidate, pout, pp->output + pp->nout - pout, 
						sizeof(*pp->output), agg_next)) != NULL) {
				/* flag this output as used */
				pout->row_key.keys = NULL;
				pval = &pcan->value;
			}
		}
		
		if (!pval || col_null(pval)) {  /* nothing in output for this x,y location */
			dbgetnull(dbproc, pcol->column_bindtype, pcol->column_bindlen, (BYTE *) pcol->column_varaddr);
			continue;
		}
		
		assert(pval);
		
#if 0
		printf("\ncopying col %d, type %d/%d, len %d to %p ", i, pval->type, pcol->column_type, pval->len, pcol->column_varaddr);
		switch (pval->type) {
		case 48:
			printf("value %d", (int)pval->ti);	break;
		case 56:
			printf("value %d", (int)pval->si);	break;
		}
		printf("\n");
#endif		
		pcol->column_size = pval->len;
		pcol->column_data = col_buffer(pval);
		
		copy_data_to_host_var(	dbproc, 
					pval->type, 
					col_buffer(pval), 
					pval->len, 
					dblib_bound_type(pcol->column_bindtype), 
					(BYTE *) pcol->column_varaddr,  
					pcol->column_bindlen,
					pcol->column_bindtype, 
					(DBINT*) pcol->column_nullbind
					);
	}

	return REG_ROW;
}

/** 
 * Pivot the rows, creating a new resultset
 *
 * Call dbpivot() immediately after dbresults().  It calls dbnextrow() as long as
 * it returns REG_ROW, transforming the results into a cross-tab report.  
 * dbpivot() modifies the metadata such that DB-Library can be used tranparently: 
 * retrieve the rows as usual with dbnumcols(), dbnextrow(), etc. 
 *
 * @dbproc, our old friend
 * @nkeys the number of left-edge columns to group by
 * @keys  an array of left-edge columns to group by
 * @ncols the number of top-edge columns to group by
 * @cols  an array of top-edge columns to group by
 * @func  the aggregation function to use
 * @val   the number of the column to which @func is applied
 *
 * @returns the return code from the final call to dbnextrow().  
 *  Success is normally indicated by NO_MORE_ROWS.  
 */
RETCODE
dbpivot(DBPROCESS *dbproc, int nkeys, int *keys, int ncols, int *cols, DBPIVOT_FUNC func, int val)
{
	enum { logalot = 1 };
	struct pivot_t P, *pp;
	struct agg_t input, *pout = NULL;
	struct key_t *pacross;
	struct metadata_t *metadata, *pmeta;
	size_t i, nmeta = 0;

	tdsdump_log(TDS_DBG_FUNC, "dbpivot(%p, %d,%p, %d,%p, %p, %d)\n", dbproc, nkeys, keys, ncols, cols, func, val);
	if (logalot) {
		char buffer[1024] = {'\0'}, *s = buffer;
		const static char *names[2] = { "\tkeys (down)", "\n\tcols (across)" };
		int *p = keys, *pend = p + nkeys;
		
		for (i=0; i < 2; i++) {
			const char *sep = "";
			s += sprintf(s, "%s: ", names[i]);
			for ( ; p < pend; p++) {
				s += sprintf(s, "%s%d", sep, *p);
				sep = ", ";
			}
			p = cols;
			pend = p + ncols;
			assert(s < buffer + sizeof(buffer));
		}
		tdsdump_log(TDS_DBG_FUNC, "%s\n", buffer);
	}
	
	memset(&input,  0, sizeof(input));
	
	P.dbproc = dbproc;
	if ((pp = tds_find(&P, pivots, npivots, sizeof(*pivots), pivot_key_equal)) == NULL ) {
		pp = realloc(pivots, (1 + npivots) * sizeof(*pivots)); 
		if (!pp)
			return FAIL;
		pivots = pp;
		pp += npivots++;
	} else {
		agg_free(pp->output);
		key_free(pp->across);		
	}
	memset(pp, 0, sizeof(*pp));

	if ((input.row_key.keys = calloc(nkeys, sizeof(*input.row_key.keys))) == NULL)
		return FAIL;
	input.row_key.nkeys = nkeys;
	for (i=0; i < nkeys; i++) {
		int type = dbcoltype(dbproc, keys[i]);
		int len = dbcollen(dbproc, keys[i]);
		assert(type && len);
		
		col_init(input.row_key.keys+i, type, len);
		if (FAIL == dbbind(dbproc, keys[i], bind_type(type), input.row_key.keys[i].len, col_buffer(input.row_key.keys+i)))
			return FAIL;
		if (FAIL == dbnullbind(dbproc, keys[i], &input.row_key.keys[i].null_indicator))
			return FAIL;
	}
	
	if ((input.col_key.keys = calloc(ncols, sizeof(*input.col_key.keys))) == NULL)
		return FAIL;
	input.col_key.nkeys = ncols;
	for (i=0; i < ncols; i++) {
		int type = dbcoltype(dbproc, cols[i]);
		int len = dbcollen(dbproc, cols[i]);
		assert(type && len);
		
		col_init(input.col_key.keys+i, type, len);
		if (FAIL == dbbind(dbproc, cols[i], bind_type(type), input.col_key.keys[i].len, col_buffer(input.col_key.keys+i)))
			return FAIL;
		if (FAIL == dbnullbind(dbproc, cols[i], &input.col_key.keys[i].null_indicator))
			return FAIL;
	}
	
	/* value */ {
		int type = dbcoltype(dbproc, val);
		int len = dbcollen(dbproc, val);
		assert(type && len);
		
		col_init(&input.value, type, len);
		if (FAIL == dbbind(dbproc, val, bind_type(type), input.value.len, col_buffer(&input.value)))
			return FAIL;
		if (FAIL == dbnullbind(dbproc, val, &input.value.null_indicator))
			return FAIL;
	}
	
	while ((pp->status = dbnextrow(dbproc)) == REG_ROW) {
		/* add to unique list of crosstab columns */
		if ((pacross = tds_find(&input.col_key, pp->across, pp->nacross, sizeof(*pp->across), key_equal)) == NULL ) {
			pacross = realloc(pp->across, (1 + pp->nacross) * sizeof(*pp->across)); 
			if (!pacross)
				return FAIL;
			pp->across = pacross;
			pacross += pp->nacross++;
			key_cpy(pacross, &input.col_key);
		}
		assert(pp->across);
		
		if ((pout = tds_find(&input, pp->output, pp->nout, sizeof(*pp->output), agg_equal)) == NULL ) {
			pout = realloc(pp->output, (1 + pp->nout) * sizeof(*pp->output)); 
			if (!pout)
				return FAIL;
			pp->output = pout;
			pout += pp->nout++;

			
			if ((pout->row_key.keys = calloc(input.row_key.nkeys, sizeof(*pout->row_key.keys))) == NULL)
				return FAIL;
			key_cpy(&pout->row_key, &input.row_key);

			if ((pout->col_key.keys = calloc(input.col_key.nkeys, sizeof(*pout->col_key.keys))) == NULL)
				return FAIL;
			key_cpy(&pout->col_key, &input.col_key);

			col_init(&pout->value, input.value.type, input.value.len);
		}
		
		func(&pout->value, &input.value);

	}

	/* Mark this proc as pivoted, so that dbnextrow() sees it when the application calls it */
	pp->dbproc = dbproc;
	pp->dbresults_state = dbproc->dbresults_state;
	dbproc->dbresults_state = pp->output < pout? _DB_RES_RESULTSET_ROWS : _DB_RES_RESULTSET_EMPTY;
	
	/*
	 * Initialize new metadata
	 */
	nmeta = input.row_key.nkeys + pp->nacross;	
	metadata = calloc(nmeta, sizeof(*metadata));
	
	assert(pp->across || pp->nacross == 0);
	
	/* key columns are passed through as-is, verbatim */
	for (i=0; i < input.row_key.nkeys; i++) {
		assert(i < nkeys);
		metadata[i].name = strdup(dbcolname(dbproc, keys[i]));
		metadata[i].pacross = NULL;
		col_cpy(&metadata[i].col, input.row_key.keys+i);
	}

	/* pivoted columms are found in the "across" data */
	for (i=0, pmeta = metadata + input.row_key.nkeys; i < pp->nacross; i++) {
		struct col_t col;
		col_init(&col, SYBFLT8, sizeof(double));
		assert(pmeta + i < metadata + nmeta);
		pmeta[i].name = make_col_name(pp->across+i);
		assert(pp->across);
		pmeta[i].pacross = pp->across + i;
		col_cpy(&pmeta[i].col, pp->nout? &pp->output[0].value : &col);
	}

	if (!reinit_results(dbproc->tds_socket, nmeta, metadata)) {
		return FAIL;
	}
	
	return SUCCEED;
	
#if 0
	for (pp->pout=pp->output; pp->pout < pp->output + pp->nout; pp->pout++) {
		char name[256] = {0};
	
		assert(pp->pout->col_key.keys[0].len < sizeof(name));
		memset(name, '\0', sizeof(name));
		memcpy(name, pp->pout->col_key.keys[0].s, pp->pout->col_key.keys[0].len), 
		printf("%5d  %-30s  %5d\n", pp->pout->row_key.keys[0].i, 
					    name, 
					    pp->pout->value.i );
	}
	exit(1);
#endif
}

/* 
 * Aggregation functions 
 */

void
dbpivot_count (struct col_t *tgt, const struct col_t *src)
{
	assert( tgt && src);
	assert (src->type);
	
	tgt->type = SYBINT4;
	
	if (! col_null(src))
		tgt->i++;
}

void
dbpivot_sum (struct col_t *tgt, const struct col_t *src)
{
	assert( tgt && src);
	assert (src->type);
	
	tgt->type = src->type;
	
	if (col_null(src))
		return;

	switch (src->type) {
	case SYBINT1:
		tgt->ti += src->ti;
		break;
	case SYBINT2:
		tgt->si += src->si;
		break;
	case SYBINT4:
		tgt->i += src->i;
		break;
	case SYBFLT8:
		tgt->f += src->f;
		break;
	case SYBREAL:
		tgt->r += src->r;
		break;

	case SYBCHAR:
	case SYBVARCHAR:
	case SYBINTN:
	case SYBDATETIME:
	case SYBBIT:
	case SYBTEXT:
	case SYBNTEXT:
	case SYBIMAGE:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBDATETIME4:
	case SYBBINARY:
	case SYBVOID:
	case SYBVARBINARY:
	case SYBBITN:
	case SYBNUMERIC:
	case SYBDECIMAL:
	case SYBFLTN:
	case SYBMONEYN:
	case SYBDATETIMN:
	default:
		tdsdump_log(TDS_DBG_INFO1, "dbpivot_sum(): invalid operand %d\n", src->type);
		tgt->type = SYBINT4;
		tgt->i = 0;
		break;
	}
}

void
dbpivot_min (struct col_t *tgt, const struct col_t *src)
{
	assert( tgt && src);
	assert (src->type);
	
	tgt->type = src->type;
	
	if (col_null(src))
		return;

	switch (src->type) {
	case SYBINT1:
		tgt->ti = tgt->ti < src->ti? tgt->ti : src->ti;
		break;
	case SYBINT2:
		tgt->si = tgt->si < src->si? tgt->si : src->si;
		break;
	case SYBINT4:
		tgt->i = tgt->i < src->i? tgt->i : src->i;
		break;
	case SYBFLT8:
		tgt->f = tgt->f < src->f? tgt->f : src->f;
		break;
	case SYBREAL:
		tgt->r = tgt->r < src->r? tgt->r : src->r;
		break;

	case SYBCHAR:
	case SYBVARCHAR:
	case SYBINTN:
	case SYBDATETIME:
	case SYBBIT:
	case SYBTEXT:
	case SYBNTEXT:
	case SYBIMAGE:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBDATETIME4:
	case SYBBINARY:
	case SYBVOID:
	case SYBVARBINARY:
	case SYBBITN:
	case SYBNUMERIC:
	case SYBDECIMAL:
	case SYBFLTN:
	case SYBMONEYN:
	case SYBDATETIMN:
	default:
		tdsdump_log(TDS_DBG_INFO1, "dbpivot_sum(): invalid operand %d\n", src->type);
		tgt->type = SYBINT4;
		tgt->i = 0;
		break;
	}
}

void
dbpivot_max (struct col_t *tgt, const struct col_t *src)
{
	assert( tgt && src);
	assert (src->type);
	
	tgt->type = src->type;
	
	if (col_null(src))
		return;

	switch (src->type) {
	case SYBINT1:
		tgt->ti = tgt->ti > src->ti? tgt->ti : src->ti;
		break;
	case SYBINT2:
		tgt->si = tgt->si > src->si? tgt->si : src->si;
		break;
	case SYBINT4:
		tgt->i = tgt->i > src->i? tgt->i : src->i;
		break;
	case SYBFLT8:
		tgt->f = tgt->f > src->f? tgt->f : src->f;
		break;
	case SYBREAL:
		tgt->r = tgt->r > src->r? tgt->r : src->r;
		break;

	case SYBCHAR:
	case SYBVARCHAR:
	case SYBINTN:
	case SYBDATETIME:
	case SYBBIT:
	case SYBTEXT:
	case SYBNTEXT:
	case SYBIMAGE:
	case SYBMONEY4:
	case SYBMONEY:
	case SYBDATETIME4:
	case SYBBINARY:
	case SYBVOID:
	case SYBVARBINARY:
	case SYBBITN:
	case SYBNUMERIC:
	case SYBDECIMAL:
	case SYBFLTN:
	case SYBMONEYN:
	case SYBDATETIMN:
	default:
		tdsdump_log(TDS_DBG_INFO1, "dbpivot_sum(): invalid operand %d\n", src->type);
		tgt->type = SYBINT4;
		tgt->i = 0;
		break;
	}
}

static const struct name_t {
	char name[14];
	DBPIVOT_FUNC func;
} names[] = 
	{ { "count", 	dbpivot_count }
	, { "sum", 	dbpivot_sum }
	, { "min", 	dbpivot_min }
	, { "max",	dbpivot_max }
	};

static boolean
name_equal( const struct name_t *n1, const struct name_t *n2 ) 
{
	assert(n1 && n2);
	return strcmp(n1->name, n2->name) == 0;
}

DBPIVOT_FUNC 
dbpivot_lookup_name( const char name[] )
{
	struct name_t *n = TDS_FIND(name, names, (compare_func) name_equal);
	
	return n ? n->func : NULL;
}
