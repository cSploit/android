/* 
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 * 
 * $Id$
 * 
 * This utility choses a reasonable pagesize for a BTREE database.
 * 
 * Here we assume that:
 * 1) This set of records are already in a BTREE database, which may been
 *    configured with a unreasonable page size.
 * 2) Treat the database as if it were compacted.
 * 3) The goal is to optimize the database for the current content,
 *    rather than for ongoing insertions.
 * 
 * The page size of a BTREE can be 512, 1024, 2048, 4096, 8192, 16384,
 * 32768, 65536, totally 8 different cases. So we have 8 possible BTREE,
 * each with different pagesize to contain them. Without actually creating
 * those 8 databases, this utility tries to simulate situations, that is,
 * for each pagesize, how many leaf pages, over flow pages and duplicate
 * pages are needed, and what's the distribution of each kind of pages
 * based on their fill factor.
 * 
 * db_tuner contains 2 parts:
 * 
 * I) Simulation of 8 different pagesized databases.
 * This includes, the number of leaf pages, overflow pages caused by 
 * big key/data in leaf layers, and duplicate pages, the distribution
 * of each kind of pages in different fill factor ranges.
 * 
 * This is achieved by retrieving those records from existing btree and
 * inserting them into different kind of pages. Since the records from
 * the btree are sorted, they are inserted into the end of each page.
 * If this page become full, that is no enough space, only this new
 * record will be put into next new page.
 * 
 * II) Recommend the best page size.
 * From our simulation results, this utility choose a page size based on
 * the number of overflow pages and storage (on-disk space).
 * If there is no overflow pages, then choose the one resulting in
 * the smallest storage as the recommended page size. Otherwise,
 * choose the one that results in a reasonable small number of overflow pages.
 */

#include "db_config.h"

#include <assert.h>
#include <math.h>

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"

#ifndef lint
static const char copyright[] =
    "Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.\n";
#endif

/*
 * Fill factor distribution division,
 * e.g., [0.000 - 0.099], ..., [0.900 - 0.999], [1.000- 1.000]
 */
#define DIST_DIVISION			11

/* Error return code in db_tuner, different with those in src\dbinc\db.in. */
/* Dist >= DIST_DIVISION. */
#define EXIT_DIST_OUTRANGE		(-31000)
/* "Insert" zero needed. */
#define EXIT_INSERT_ZERO_NEED		(-31001)
/* On-page duplicate set = 0. */
#define EXIT_INSERT_ZERO_ONPGDUP	(-31002)

/* Follows are some special "insert" types based on the data. */
/* Insert as normal case. */
#define INSERT_NORMAL			0x0001
/* Nothing is inserted. */
#define INSERT_NOTHING			0x0002
/* No key but a slot is inserted. */
#define INSERT_SLOT			0x0003
/* A B_DUPLICATE point is inserted. */
#define INSERT_BDUPLICATE		0x0004

/*
 * Page size of BTREE can be from DB_MIN_PGSIZE (512) to DB_MAX_PGSIZE(64K),
 * and all the pagesize is the power of 2, so we have 8 possible cases.
 *
 * 8 is from ((int)(log(DB_MAX_PGSIZE) - log(DB_MIN_PGSIZE) + 1)).
 */
#define NUM_PGSIZES			8

/* Structure used to store statistics of the assessment. */
typedef struct __tuner_ff_stat {
	uintmax_t pgsize_leaf_dist[NUM_PGSIZES][DIST_DIVISION];
	uintmax_t pgsize_ovfl_dist[NUM_PGSIZES][DIST_DIVISION];
	uintmax_t pgsize_dup_dist[NUM_PGSIZES][DIST_DIVISION];

	/* Info used to track stats across page in a traverse. */
	u_int32_t pg_leaf_offset[NUM_PGSIZES];
	u_int32_t pg_dup_offset[NUM_PGSIZES];
}TUNER_FF_STAT;

static int __tuner_analyze_btree __P((DB_ENV *, DB *, u_int32_t));
static int __tuner_ff_stat_callback __P((DBC *, PAGE *, void *, int *));
static int __tuner_generate_fillfactor_stats __P((DB_ENV *, DB *,
    TUNER_FF_STAT *));
static int __tuner_insert_dupdata __P((DB *, u_int32_t, int, TUNER_FF_STAT *));
static int __tuner_insert_kvpair __P((DB *, u_int32_t, u_int32_t, int, int,
    int, TUNER_FF_STAT *));
static int __tuner_leaf_page __P((DBC *, PAGE *, TUNER_FF_STAT *));
static int __tuner_leaf_dupdata __P((DBC*, PAGE *, int, int, u_int32_t,
    TUNER_FF_STAT *));
static int __tuner_leaf_dupdata_entries __P((DBC *, PAGE *, int, int, int, int,
    TUNER_FF_STAT *));
static int __tuner_opd_data_entries __P((DBC *, PAGE *, int, int,
    TUNER_FF_STAT *));
static int __tuner_opd_data __P((DBC *, PAGE *, int, int, TUNER_FF_STAT *));
static int __tuner_opd_page __P((DBC *, PAGE *, TUNER_FF_STAT *));
static int __tuner_print_btree_fillfactor __P((u_int32_t, TUNER_FF_STAT *));
static int __tuner_record_dup_pg __P((int, TUNER_FF_STAT *));
static int __tuner_record_last_opd __P((int, TUNER_FF_STAT *));
static int __tuner_record_leaf_pg __P((int, TUNER_FF_STAT *));
static int __tuner_record_ovfl_pg __P((u_int32_t, int, TUNER_FF_STAT *));

static int get_opd_size __P((DBC*, PAGE*, u_int32_t*));
static int item_size __P((DB *, PAGE *, int));
static int item_space __P((DB *, PAGE *, int));
int main __P((int, char *[]));
static int open_db __P((DB **, DB_ENV *, char *, char *));
static int sum_opd_page_data_entries __P((DB *, PAGE *));
static int usage __P((void));
static int version_check __P((void));

const char *progname = "db_tuner";

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DB *dbp;
	DB_ENV *dbenv;
	DBTYPE dbtype;
	char *dbname, *home, *subdb;
	int ch, is_set_dbfile, ret;
	u_int32_t cachesize, verbose;

	if ((ret = version_check()) != 0)
		return (ret);

	dbenv = NULL;
	dbp = NULL;
	cachesize = 0;
	dbname = home = subdb = NULL;
	is_set_dbfile = verbose = 0;
	dbtype = DB_UNKNOWN;

	while ((ch = getopt(argc, argv, "c:d:h:vs:")) != EOF)
		switch (ch) {
		case 'c':
			cachesize = atoi(optarg);
			break;
		case 'd':
			dbname = optarg;
			is_set_dbfile = 1;
			break;
		case 'h':
			home = optarg;
			break;
		case 's':
			subdb = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* Handle possible interruptions. */
	__db_util_siginit();

	if (!is_set_dbfile)
		usage();

	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr, "%s: db_env_create: %s\n",
		    progname, db_strerror(ret));
		goto err;
	}

	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);

	if ((cachesize != 0) && (ret =
	    dbenv->set_cachesize(dbenv, (u_int32_t)0, cachesize, 1)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->set_cachesize:");
		goto err;
	}

	/*
	 * If attaching to a pre-existing environment fails, create a
	 * private one and try again.
	 */
	if ((ret = dbenv->open(dbenv, home, DB_USE_ENVIRON, 0)) != 0 &&
	    (ret == DB_VERSION_MISMATCH || ret == DB_REP_LOCKOUT ||
	    (ret = dbenv->open(dbenv, home,
	    DB_CREATE | DB_INIT_MPOOL | DB_USE_ENVIRON | DB_PRIVATE,
	    0)) != 0)) {
		dbenv->err(dbenv, ret, "DB_ENV->open:");
		goto err;
	}

	if ((ret = open_db(&dbp, dbenv, dbname, subdb)) != 0) {
		dbenv->err(dbenv, ret, "open_db:");
		goto err;
	}

	if ((ret = dbp->get_type(dbp, &dbtype)) != 0) {
		dbenv->err(dbenv, ret, "DB->get_type:");
		goto err;
	}

	switch (dbtype) {
	case DB_BTREE:
		if ((ret = __tuner_analyze_btree(dbenv, dbp, verbose)) != 0)
			dbenv->err(dbenv, ret, "__tuner_analyze_btree fails.");
		break;
	default:
		dbenv->errx(dbenv, DB_STR("5001",
		    "%s: Unsupported database type"), progname);
	}

err:
	if (dbp != NULL && (ret = dbp->close(dbp, 0)) != 0)
		dbenv->err(dbenv, ret, "DB->close: %s", dbname);

	if (dbenv != NULL && (ret = dbenv->close(dbenv, 0)) != 0)
		fprintf(stderr, "%s: dbenv->close: %s", progname,
		    db_strerror(ret));

	/* Resend any caught signal. */
	__db_util_sigresend();

	return (ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/*
 * Generate the simulated statistics for each different btree pagesize,
 * then print out this information if verbose enabled, finally make our
 * recommendation of our best pagesize based on our simulated results.
 */
static int
__tuner_analyze_btree(dbenv, dbp, verbose)
	DB_ENV *dbenv;
	DB *dbp;
	u_int32_t verbose;
{
	TUNER_FF_STAT stats;
	int ret;

	memset(&stats, 0, sizeof(TUNER_FF_STAT));

	if ((ret = __tuner_generate_fillfactor_stats(dbenv, dbp,
	    &stats)) != 0) {
		dbenv->err(dbenv, ret,
		    "__tuner_generate_fillfactor_stats fails.");
		return (ret);
	}

	(void)__tuner_print_btree_fillfactor(verbose, &stats);

	return (EXIT_SUCCESS);
}

/* Traverse the database to gather simulated statistics for each pagesize.*/
static int
__tuner_generate_fillfactor_stats(dbenv, dbp, stats)
	DB_ENV *dbenv;
	DB *dbp;
	TUNER_FF_STAT *stats;
{
	DBC *dbc;
	int i, ret, t_ret;

	ret = t_ret = 0;

	if ((ret = dbp->cursor(dbp, NULL, &dbc, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->cursor:");
		return (ret);
	}

	/*
	 * Call the internal Berkeley DB function, that triggers a callback
	 * for each page in a btree database.
	 */
	if ((ret = __bam_traverse(dbc, DB_LOCK_READ, PGNO_INVALID, 
	    __tuner_ff_stat_callback, (void *)stats)) != 0) {
		dbenv->err(dbenv, ret, "__bam_traverse:");
		goto err;
	}

	/*
	 * Record the last simulated page for leaf and dup page,
	 * which ensure at least one page is used.
	 */
	for (i = 0; i < NUM_PGSIZES; ++i) {
		if (stats->pg_leaf_offset[i] > 0 &&
		    (ret = __tuner_record_leaf_pg(i, stats)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_record_leaf");
			break;
		}

		if (stats->pg_dup_offset[i] > 0 &&
		    (ret = __tuner_record_dup_pg(i, stats)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_record_dup_pg");
			break;
		}
	}

err:
	if(dbc != NULL && (t_ret = dbc->close(dbc)) != 0)
		dbenv->err(dbenv, t_ret, "DBC->close:");

	if (ret == 0 && t_ret != 0)
		ret = t_ret;

	return (ret);
}

/*
 * This callback is used in __bam_traverse. When traversing each page in
 * the BTREE, it retrieves each record for simulation.
 */
static int 
__tuner_ff_stat_callback(dbc, h, cookie, putp)
	DBC *dbc;
	PAGE *h;
	void *cookie;
	int *putp;
{
	DB_ENV *dbenv;
	int ret;

	dbenv = dbc->dbenv;
	*putp = 0;

	switch (TYPE(h)) {
	case P_LBTREE:
		if ((ret = __tuner_leaf_page(dbc, h, cookie)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_leaf_page");
			return (ret);
		}
		break;
	case P_LDUP:
	case P_LRECNO:
		/* Coming a new off-page duplicate set.*/
		if (h->prev_pgno == PGNO_INVALID &&
		    (ret = __tuner_opd_page(dbc, h, cookie)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_opd_page:");
			return (ret);
		}
		break;
	case P_IBTREE:
	case P_IRECNO:
	case P_OVERFLOW:
		break;
	default:
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}

/*
 * Deal with the leaf page of existing database. This includes:
 * 1: determine the on-page duplicate set, and calculate its total size
 * 2: decise where should this set go (on-page or off-page) in the later
 *     simulation stage and do some "movement".
 * 3: "move" the unique key data pairs to the simulated leaf pages.
 */
static int
__tuner_leaf_page(dbc, h, cookie)
	DBC *dbc;
	PAGE *h;
	TUNER_FF_STAT *cookie;
{
	DB *dbp;
	DB_ENV *dbenv;
	db_indx_t findx, lindx, indx, *inp, top;
	u_int32_t data_sz, key_sz, onpd_sz;
	int i, ret, in_data_type;

	dbp = dbc->dbp;
	dbenv = dbp->dbenv;
	/*
	 * Use some macros from db_page.h to retrieve information from the 
	 * page. P_INP retrieves the offset to the start of page index array.
	 * NUM_ENT retrieves the number of items on the page.
	 */
	inp = P_INP(dbp, h);
	top = NUM_ENT(h);
	ret = 0;

	for (indx = 0; indx < top;) {
		/*
		 * If on-page duplicate, first calculate the total size,
		 * including one key and all data.
		 */
		onpd_sz = 0;
		if ((indx + P_INDX) < top && inp[indx] == inp[indx + P_INDX]) {

			/* Ignore deleted items. */
			if (B_DISSET(GET_BKEYDATA(dbp, h, indx)->type))
				continue;

			/*  Count the key once. */
			onpd_sz += item_space(dbp, h, indx);

			for (findx = indx;
			    indx < top && inp[findx] == inp[indx];
			    indx += P_INDX) {
				/* Ignore deleted items. */
				if (B_DISSET(GET_BKEYDATA(dbp, h,
				    indx + O_INDX)->type))
					continue;
				/* Count all the data items. */
				onpd_sz += item_space(dbp, h, indx + O_INDX);
			}

			/*Indx range of on-page duplicate set: [findx, lindx)*/
			lindx = indx;

			if (onpd_sz == 0)
				return (EXIT_INSERT_ZERO_ONPGDUP);

			/* "Move" on-page duplicate set to simualted pages.*/
			if ((ret = __tuner_leaf_dupdata(dbc, h, findx, lindx,
			    onpd_sz, cookie)) != 0) {
				dbenv->err(dbenv, ret, "__tuner_leaf_dupdata");
				return (ret);
			}
		} else {
			in_data_type = INSERT_NORMAL;
			/* Ignore deleted items. */
			if (B_DISSET(GET_BKEYDATA(dbp, h, indx)->type))
				continue;

			/* First consider key. */
			key_sz = item_size(dbp, h, indx);

			/* Ignore deleted items. */
			if (B_DISSET(GET_BKEYDATA(dbp, h, indx + O_INDX)->type))
				continue;

			/* next consider data.*/
			if (B_TYPE(GET_BKEYDATA(dbp, h,
			    indx + O_INDX)->type) == B_DUPLICATE) {
				/*
				 * Off-page duplicate set is not handled here
				 * but on the duplicate pages.
				 * Here the key is inserted into "simulated"
				 * leaf_page.
				 */
				in_data_type = INSERT_NOTHING;
				data_sz = 0;
			} else
				data_sz = item_size(dbp, h, indx + O_INDX);

			for (i = 0; i < NUM_PGSIZES; ++i) {
				if ((ret = __tuner_insert_kvpair(dbp, key_sz,
				    data_sz, i, INSERT_NORMAL, in_data_type,
				    cookie)) != 0) {
					dbenv->err(dbenv, ret,
					    "__tuner_insert_kvpair");
					break;
				}
			}

			indx += P_INDX;
		}
	}

	return (ret);
}

/*
 * "Move" the on-page duplicate data set from the specific page to our
 * simulated databases (Indx range of this on-page duplicate set:
 * [findx, lindx)), it includes following steps:
 * 
 * First check where should this set go (on-page or off-page duplicate tree).
 * 
 * This is determined as "If total size of duplicate data set is more than 25%
 * of a specific page size, then this set go to off-page duplicate tree.
 * Otherwise, it goes to on-page duplicate. "
 * 
 * Then "move" this duplicate set to our simulated pages in each simulated
 * database.
 */
static int
__tuner_leaf_dupdata(dbc, h, findx, lindx, dup_sz, cookie)
	DBC *dbc;
	PAGE *h;
	int findx, lindx;
	u_int32_t dup_sz;
	TUNER_FF_STAT *cookie;
{
	DB_ENV *dbenv;
	int i, is_opd, ret;
	u_int32_t pgsize;

	dbenv = dbc->dbenv;

	for (i = 0; i < NUM_PGSIZES; ++i) {
		pgsize = (1 << i) * DB_MIN_PGSIZE;

		/* Check whether this duplicate set go to opd? */
		is_opd = (dup_sz < (pgsize / 4)) ? 0 : 1;

		/* "Move" this on-page duplicate to our simulated pages. */
		if ((ret = __tuner_leaf_dupdata_entries(dbc, h, findx,
		    lindx, i, is_opd, cookie)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_leaf_dupdata_entries");
			return (ret);
		}

		/*
		 * Record the last simulated duplicate pages for a finished
		 * off-page duplicate set then reset the offset to zero
		 * for next opd set.
		 */
		if (is_opd &&
		    (ret = __tuner_record_last_opd(i, cookie)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_record_last_opd");
			return (ret);
		}
	}

	return (EXIT_SUCCESS);
}

/*
 * "Move" the on-page duplicate set [findx, lindx) on the specific page to
 * simulated database with pagesize = (1 << indx_pgsz) * DB_MIN_PGSIZE;
 */
static int
__tuner_leaf_dupdata_entries(dbc, h, findx, lindx, indx_pgsz, is_opd, cookie)
	DBC *dbc;
	PAGE *h;
	int findx, lindx, indx_pgsz, is_opd;
	TUNER_FF_STAT *cookie;
{
	DB *dbp;
	DB_ENV *dbenv;
	db_indx_t indx;
	u_int32_t data_sz, key_sz;
	int ret, in_key_type;

	dbp = dbc->dbp;
	dbenv = dbp->dbenv;

	for (indx = findx; indx < lindx; indx += P_INDX) {

		key_sz = 0;
		in_key_type = INSERT_SLOT;
		/*
		 * For on-page duplicate data, the key is inserted once, 
		 * then its corresponding data.
		 */
		if (indx == findx) {
			/* Ignore deleted items. */
			if (B_DISSET(GET_BKEYDATA(dbp, h, indx)->type))
				continue;

			key_sz = item_size(dbp, h, indx);
			in_key_type = INSERT_NORMAL;

			/* 
			 * If is_opd, then insert a key + B_DUPLICATE pair for
			 * this on-page duplicate to simulated leaf page.
			 * INSERT_BDUPLICATE: B_DUPLICATE point.
			 */
			if (is_opd && (ret =
			    __tuner_insert_kvpair(dbp, key_sz, 0, indx_pgsz,
			    INSERT_NORMAL, INSERT_BDUPLICATE, cookie)) != 0) {
				dbenv->err(dbenv, ret, "__tuner_insert_kvpair");
				return (ret);
			}
		}

		/* Ignore deleted items. */
		if (B_DISSET(GET_BKEYDATA(dbp, h, indx + O_INDX)->type))
			continue;

		data_sz = item_size(dbp, h, indx + O_INDX);

		if (is_opd) {
			ret = __tuner_insert_dupdata(dbp, data_sz,
			    indx_pgsz, cookie);
			if (ret != 0) {
				dbenv->err(dbenv, ret,
				    "__tuner_insert_dupdata");
				return (ret);
			}
		} else {
			ret = __tuner_insert_kvpair(dbp, key_sz, data_sz,
			    indx_pgsz, in_key_type, INSERT_NORMAL,
			    cookie);

			if (ret != 0) {
				dbenv->err(dbenv, ret, "__tuner_insert_kvpair");
				return (ret);
			}
		}
	}
	return (EXIT_SUCCESS);
}

/* Tuner the off-page duplicate pages from existing database. */
static int
__tuner_opd_page(dbc, h, cookie)
	DBC *dbc;
	PAGE *h;
	TUNER_FF_STAT *cookie;
{
	DB_ENV *dbenv;
	u_int32_t opd_sz, pgsize;
	int i, is_opd, ret;

	dbenv = dbc->dbenv;
	ret = opd_sz = 0;

	/* 1st calculate the total size of the duplicate set. */
	if ((ret = get_opd_size(dbc, h, &opd_sz)) != 0) {
		dbenv->err(dbenv, ret, "get_opd_size:");
		return (ret);
	}

	/* 2nd insert this set into "simulated" pages for each page size.*/
	for (i = 0; i < NUM_PGSIZES; ++i) {
		pgsize = (1 << i) * DB_MIN_PGSIZE;

		/* Check whether this duplicate set go to opd? */
		is_opd = (opd_sz < (pgsize / 4)) ? 0 : 1;

		if ((ret = __tuner_opd_data(dbc, h, i, is_opd, cookie)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_opd_data:");
			break;
		}
	}

	return (ret);
}

/* "Move" all the off-page duplicate data into simulated on-page or off-page.*/
static int
__tuner_opd_data(dbc, h, indx_pgsz, is_opd, cookie)
	DBC *dbc;
	PAGE *h;
	int indx_pgsz, is_opd;
	TUNER_FF_STAT *cookie;
{
	DB *dbp;
	DB_ENV *dbenv;
	DB_MPOOLFILE *mpf;
	PAGE *p;
	db_pgno_t next_pgno;
	int ret;

	dbp = dbc->dbp;
	dbenv = dbp->dbenv;
	mpf = dbp->mpf;

	p = h;

	/*
	 * __tuner_leaf_page has inserted one key for each opd already,
	 * so here only a B_DUPLICATE point is inserted into simulate
	 * leaf page if this duplicate set goes to on-page.
	 */
	if (is_opd) {
		ret = __tuner_insert_kvpair(dbp, 0, 0, indx_pgsz,
		    INSERT_NOTHING, INSERT_BDUPLICATE, cookie);

		if (ret!= 0) {
			dbenv->err(dbenv, ret, "__tuner_insert_kvpair");
			return (ret);
		}
	}

	/* Next insert all the data of this duplicate set. */
	while (1) {
		ret = __tuner_opd_data_entries(dbc, h, indx_pgsz, is_opd,
		    cookie);
		if (ret != 0) {
			dbenv->err(dbenv, ret, "__tuner_opd_data_entries");
			return (ret);
		}

		next_pgno = p->next_pgno;

		if (p != h && (ret = mpf->put(mpf, p, dbc->priority, 0)) != 0) {
			dbenv->err(dbenv, ret, "DB_MPOOLFILE->put:");
			return (ret);	
		}

		if (next_pgno == PGNO_INVALID)
			break;

		if ((ret = mpf->get(mpf, &next_pgno, dbc->txn, 0, &p)) != 0) {
			dbenv->err(dbenv, ret, "DB_MPOOLFILE->get:");
			return (ret);
		}
	}

	/* Record the last simulate duplicate page if goto off-page duplicate*/
	if (is_opd && (ret = __tuner_record_last_opd(indx_pgsz, cookie)) != 0) {
		dbenv->err(dbenv, ret, "__tuner_record_last_opd");
		return (ret);
	}

	return (EXIT_SUCCESS);
}

/*
 * "Move" the off-page duplicate data set to our simulated on-page or
 * off-page.
 */
static int
__tuner_opd_data_entries(dbc, h, indx_pgsz, is_opd, cookie)
	DBC *dbc;
	PAGE *h;
	int indx_pgsz, is_opd;
	TUNER_FF_STAT *cookie;
{
	DB *dbp;
	DB_ENV *dbenv;
	db_indx_t indx;
	u_int32_t data_sz;
	int ret;

	dbp = dbc->dbp;
	dbenv = dbp->dbenv;

	for (indx = 0; indx < NUM_ENT(h); indx += O_INDX) {
		/* Ignore deleted items. */
		if (B_DISSET(GET_BKEYDATA(dbp, h, indx)->type))
			continue;

		data_sz = item_size(dbp, h, indx);

		if (is_opd) {
			ret = __tuner_insert_dupdata(dbp, data_sz,
			    indx_pgsz, cookie);

			if (ret != 0) {
				dbenv->err(dbenv, ret,
				    "__tuner_insert_dupdata");
				return (ret);
			}
		} else {
			/*
			 * __tuner_leaf_page has inserted one key for each
			 * opd already (this will insert moment later),
			 * so only data items and key slots are inserted.
			 */
			ret = __tuner_insert_kvpair(dbp, 0, data_sz,
			    indx_pgsz, INSERT_SLOT, INSERT_NORMAL,
			    cookie);

			if (ret != 0) {
				dbenv->err(dbenv, ret,
				    "__tuner_insert_kvpair");
				return (ret);
			}
		}
	}
	return (EXIT_SUCCESS);
}

/*
 * Try to insert a key and data pair into simulated leaf pages. 
 * Key and data pairs are always stored (or referenced) on the same leaf page.
 */
static int
__tuner_insert_kvpair(dbp, key_sz, data_sz, indx_pgsz, in_key, in_data, stats)
	DB *dbp;
	u_int32_t key_sz, data_sz;
	int indx_pgsz, in_key, in_data;
	TUNER_FF_STAT *stats;
{
	DB_ENV *dbenv;
	int is_big_data, is_big_key, ret;
	u_int32_t needed, pgsize;

	dbenv = dbp->dbenv;
	is_big_data = is_big_key = 0;
	needed = 0;
	pgsize = (1 << indx_pgsz) * DB_MIN_PGSIZE;

	if (key_sz > B_MINKEY_TO_OVFLSIZE(dbp, 2, pgsize))
		is_big_key = 1;

	if (data_sz > B_MINKEY_TO_OVFLSIZE(dbp, 2, pgsize))
		is_big_data = 1;

	if (is_big_key) {
		needed += BOVERFLOW_PSIZE; 

		/* Add big key into ovfl pages. */
		if ((ret = 
		    __tuner_record_ovfl_pg(key_sz, indx_pgsz, stats)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_record_ovfl_pg:key_sz");
			return (ret);
		}
	} else {
		/*
		 * key_sz = INSERT_SLOT indicates no key is inserted
		 * but a slot in the inp array, e.g., on-page duplicate.
		 * key_sz = INSERT_NOTHING indicates no key no slot is
		 * inserted.
		 */
		if (in_key == INSERT_NOTHING)
			needed += 0;
		else if (in_key == INSERT_SLOT)
			needed += sizeof(db_indx_t);
		else if (in_key == INSERT_NORMAL)
			needed += BKEYDATA_PSIZE(key_sz);
	}

	if (is_big_data) {
		needed += BOVERFLOW_PSIZE;

		/* Add big data into ovfl pages. */
		if ((ret =
		    __tuner_record_ovfl_pg(data_sz, indx_pgsz, stats)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_record_ovfl_pg");
			return (ret);
		}
	} else {
		/*
		 * in_data = INSERT_BDUPLICATE indicates a B_DUPLICATE is
		 * inserted, e.g., off-page duplicate case.
		 * in_data = INSERT_NOTHING indicates nothing is inserted, 
		 * happens when there is a key + B_DUPLICATE pair in
		 * __tuner_leaf_page, in which case, only the key is inserted
		 * but no data because the data will considered in
		 * __tuner_opd_page when an off-page
		 * duplicate set is coming.
		 */
		if (in_data == INSERT_NOTHING)
			needed += 0;
		else if (in_data == INSERT_BDUPLICATE)
			needed += BOVERFLOW_PSIZE;
		else if (in_data == INSERT_NORMAL)
			needed += BKEYDATA_PSIZE(data_sz);
	}

	if (needed == 0)
		return (EXIT_INSERT_ZERO_NEED);

	/* 1st leaf page, add overhead size. */
	if (stats->pg_leaf_offset[indx_pgsz] == 0)
		stats->pg_leaf_offset[indx_pgsz] = SIZEOF_PAGE;

	if ((stats->pg_leaf_offset[indx_pgsz] + needed) > pgsize) {
		/* No enough space, then record current page info. */
		if ((ret = __tuner_record_leaf_pg(indx_pgsz, stats)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_record_leaf_pg");
			return (ret);
		}

		/* Insert pair into new page. */
		stats->pg_leaf_offset[indx_pgsz] = needed + SIZEOF_PAGE;
	} else 
		stats->pg_leaf_offset[indx_pgsz]  += needed;

	return (EXIT_SUCCESS);
}

/* Try to insert a duplicate data into simulated off duplicate pages. */
static int
__tuner_insert_dupdata(dbp, data_sz, indx_pgsz, stats)
	DB *dbp;
	u_int32_t data_sz;
	int indx_pgsz;
	TUNER_FF_STAT *stats;
{
	DB_ENV *dbenv;
	int is_big_data, ret;
	u_int32_t needed, pgsize;

	dbenv = dbp->dbenv;
	is_big_data = 0;
	needed = 0;
	pgsize = (1 << indx_pgsz) * DB_MIN_PGSIZE;

	if (data_sz > B_MINKEY_TO_OVFLSIZE(dbp, 2, pgsize))
		is_big_data = 1;

	if (is_big_data) {
		needed = BOVERFLOW_PSIZE;

		/* Add big data into ovfl pages. */
		if ((ret = 
		    __tuner_record_ovfl_pg(data_sz, indx_pgsz, stats)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_record_ovfl_pg");
			return (ret);
		}
	} else
		needed += BKEYDATA_PSIZE(data_sz);

	if (needed == 0)
		return (EXIT_INSERT_ZERO_NEED);

	/* 1st opd page, add overhead size. */
	if (stats->pg_dup_offset[indx_pgsz] == 0)
		stats->pg_dup_offset[indx_pgsz] = SIZEOF_PAGE;

	if ((stats->pg_dup_offset[indx_pgsz] + needed) > pgsize) {
		/* no enough space then record current page info. */
		if ((ret = __tuner_record_dup_pg(indx_pgsz, stats)) != 0) {
			dbenv->err(dbenv, ret, "__tuner_record_dup_pg");
			return (ret);
		}

		/* insert new item into new page. */
		stats->pg_dup_offset[indx_pgsz] = needed + SIZEOF_PAGE;
	} else
		stats->pg_dup_offset[indx_pgsz]  += needed;

	return (EXIT_SUCCESS);
}

/* Insert big item into simulated over flow pages. */
static int
__tuner_record_ovfl_pg(size, indx_pgsz, stats)
	u_int32_t size;
	int indx_pgsz;
	TUNER_FF_STAT *stats;
{
	u_int32_t pgsize = (1 << indx_pgsz) * DB_MIN_PGSIZE;
	int dist;

	/* Update OVFLPAGE list: 1. Add to "full" ovfl pages.*/
	stats->pgsize_ovfl_dist[indx_pgsz][DIST_DIVISION - 1] += 
	    (size / (pgsize - SIZEOF_PAGE));
	/* Update OVFLPAGE list: 2. Add the remainder.*/
	size = size % (pgsize - SIZEOF_PAGE);
	dist = (int)(((double)(size + SIZEOF_PAGE) *
	    (DIST_DIVISION - 1)) / pgsize);

	/* assert(dist < DIST_DIVISION); */
	if (dist >= DIST_DIVISION)
		return (EXIT_DIST_OUTRANGE);

	++stats->pgsize_ovfl_dist[indx_pgsz][dist];

	return (EXIT_SUCCESS);
}

/* Record simulated leaf page if it has no space to contain new record. */
static int
__tuner_record_leaf_pg(indx_pgsz, stats)
	int indx_pgsz;
	TUNER_FF_STAT *stats;
{
	int dist;
	u_int32_t pgsize;

	pgsize = (1 << indx_pgsz) * DB_MIN_PGSIZE;

	/* First calculate its fill factor. */
	dist = (int)(((double)stats->pg_leaf_offset[indx_pgsz] *
	    (DIST_DIVISION - 1)) / pgsize);

	/* assert(dist < DIST_DIVISION); */
	if (dist >= DIST_DIVISION)
		return (EXIT_DIST_OUTRANGE);

	/* Then add one page to its corresponding distribution. */
	++stats->pgsize_leaf_dist[indx_pgsz][dist];

	return (EXIT_SUCCESS);
}

/* Record simulated duplicate page if it has no enough space for new record. */
static int
__tuner_record_dup_pg(indx_pgsz, stats)
	int indx_pgsz;
	TUNER_FF_STAT *stats;
{
	int dist;
	u_int32_t pgsize;

	pgsize = (1 << indx_pgsz) * DB_MIN_PGSIZE;

	/* First calculate its fill factor. */
	dist = (int)(((double)stats->pg_dup_offset[indx_pgsz] *
	    (DIST_DIVISION - 1)) / pgsize);

	/* assert(dist < DIST_DIVISION); */
	if (dist >= DIST_DIVISION)
		return (EXIT_DIST_OUTRANGE);

	/* Then add one page to its corresponding distribution. */
	++stats->pgsize_dup_dist[indx_pgsz][dist];

	return (EXIT_SUCCESS);
}

/*
 * Record the last simulated duplicate page when an off-page duplicate set
 * is finished, also reset its offset to be zero for next set.
 */
static int
__tuner_record_last_opd(indx_pgsz, stats)
	int indx_pgsz;
	TUNER_FF_STAT *stats;
{
	int ret;
	if (stats->pg_dup_offset[indx_pgsz] != 0 && 
	    (ret = __tuner_record_dup_pg(indx_pgsz, stats)) != 0)
		return (ret);

	/* Reset offset to zero for new opd set. */
	stats->pg_dup_offset[indx_pgsz] = 0;

	return (EXIT_SUCCESS);
}

/*
 * When a new off-page duplicate set is coming, we first calculate its total
 * size, which will be used to determine whether this set should go to on-page
 * or off-page duplicate tree in our simulation part.
 * 
 * As a off-page duplicate set is in a linked pages, we simply traverse this
 * link and sum up all the size of each data in each page. 
 */
static int
get_opd_size(dbc, h, opd_sz)
	DBC *dbc;
	PAGE *h;
	u_int32_t *opd_sz;
{
	DB *dbp;
	DB_ENV *dbenv;
	DB_MPOOLFILE *mpf;
	PAGE *p;
	db_pgno_t next_pgno;
	int  ret;
	u_int32_t dup_sz;

	dbp = dbc->dbp;
	dbenv = dbp->dbenv;
	mpf = dbp->mpf;
	dup_sz = 0;
	ret = 0;
	p = h;

	while (1) {
		dup_sz += sum_opd_page_data_entries(dbp, p);

		next_pgno = p->next_pgno;
		if (p != h && (ret =
		    mpf->put(mpf, p, dbc->priority, 0)) != 0) {
			dbenv->err(dbenv, ret, "DB_MPOOLFILE->put:");
			return (ret);	
		}

		if (next_pgno == PGNO_INVALID)
			break;

		if ((ret =
		    mpf->get(mpf, &next_pgno, dbc->txn, 0, &p)) != 0) {
			dbenv->err(dbenv, ret, "DB_MPOOLFILE->get:");
			return (ret);
		}
	}

	*opd_sz = dup_sz;

	return (EXIT_SUCCESS);
}

/* Sum up the space used to contain all the data in a specific page.*/
static int
sum_opd_page_data_entries(dbp, h)
	DB *dbp;
	PAGE *h;
{
	db_indx_t i;
	u_int32_t sz;
	sz = 0;

	for (i = 0; i < NUM_ENT(h); i += O_INDX) {
		/* Ignore deleted items. */
		if (B_DISSET(GET_BKEYDATA(dbp, h, i)->type))
			continue;
		sz += item_space(dbp, h, i);
	}

	return sz;
}

/* The space used by one item in a page. */
static int
item_space(dbp, h, indx)
	DB *dbp;
	PAGE *h;
	int indx;
{
	return (B_TYPE(GET_BKEYDATA(dbp, h, indx)->type) == B_KEYDATA ?
	    BKEYDATA_PSIZE(GET_BKEYDATA(dbp, h, indx)->len) :
	    BKEYDATA_PSIZE(GET_BOVERFLOW(dbp, h, indx)->tlen));
}

/* The actual length of a item. */
static int
item_size(dbp, h, indx)
	DB *dbp;
	PAGE *h;
	int indx;
{
	return (B_TYPE(GET_BKEYDATA(dbp, h, indx)->type) == B_KEYDATA ?
	    GET_BKEYDATA(dbp, h, indx)->len : GET_BOVERFLOW(dbp, h,
	    indx)->tlen);
}

/* Print out the information according to user's options. */
static int
__tuner_print_btree_fillfactor(verbose, stats)
	u_int32_t verbose;
	TUNER_FF_STAT *stats; 
{
	const char * DIVIDE_LINE1 = "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=";
	const char * DIVIDE_LINE2 = "-----------|";
	const char * DIVIDE_LINE3 = "---------------------------------------";
	double shift_point;
	int best_indx, i, j;
	u_int32_t pgsize;
	u_int64_t minispace, ttpgcnt[NUM_PGSIZES], ttspace[NUM_PGSIZES];
	uintmax_t dup_cnt[NUM_PGSIZES], leaf_cnt[NUM_PGSIZES],
	    ovfl_cnt[NUM_PGSIZES];

	shift_point = 0.099;
	best_indx = 0;
	minispace = UINT64_MAX;

	for (i = 0; i < NUM_PGSIZES; ++i) {
		pgsize = (1 << i) * DB_MIN_PGSIZE;
		ovfl_cnt[i] = leaf_cnt[i] = dup_cnt[i] = ttpgcnt[i] = 0;

		for (j = 0; j < DIST_DIVISION; ++j) {
			ovfl_cnt[i] += stats->pgsize_ovfl_dist[i][j];
			leaf_cnt[i] += stats->pgsize_leaf_dist[i][j];
			dup_cnt[i] += stats->pgsize_dup_dist[i][j];
		}

		ttpgcnt[i] = ovfl_cnt[i] + leaf_cnt[i] + dup_cnt[i];
		ttspace[i] = pgsize * ttpgcnt[i];
	}

	if (verbose == 1) {
		printf("\n %50s \n",
		    "===========Simulation Results===========");
		printf("\n  %s\n  %s\n  %s\n",
		    "leaf_pg:\t percentage of leaf page in that range",
		    "dup_pg:\t percentage of duplicate page in that range",
		    "ovfl_pg:\t percentage of over flow page in that range");

		for (i = 0; i < NUM_PGSIZES; ++i) {
			printf("\n\n%s%s\n", DIVIDE_LINE1, DIVIDE_LINE1);
			printf("page size = %d\n", (1 << i) * DB_MIN_PGSIZE);
			printf("%s%s\n", DIVIDE_LINE1, DIVIDE_LINE1);
			printf("%s\n", DIVIDE_LINE3);
			printf("%s   %s   %s   %s\n", "fill factor",
			    "leaf_pg", "dup_pg", "ovfl_pg");

			for (j = 0; j < DIST_DIVISION; ++j) {
				if (j == (DIST_DIVISION - 1))
					shift_point = 0.000;
				else
					shift_point = 0.099;

				printf("\n[%2.1f-%4.3f]\t",
				    (double)j/(DIST_DIVISION - 1), 
				    ((double)j/(DIST_DIVISION - 1) +
				    shift_point));

				if (leaf_cnt[i] == 0 ||
				    stats->pgsize_leaf_dist[i][j] == 0)
					printf("%3.2f\t", (double)0);
				else
					printf("%3.2f%%\t", (double)
					    (stats->pgsize_leaf_dist[i][j] *
					    100) / leaf_cnt[i]);

				if (dup_cnt[i] == 0 ||
				    stats->pgsize_dup_dist[i][j] == 0)
					printf("%3.2f\t", (double)0);
				else
					printf("%3.2f%%\t", (double)
					    (stats->pgsize_dup_dist[i][j] *
					    100) / dup_cnt[i]);

				if (ovfl_cnt[i] == 0 ||
				    stats->pgsize_ovfl_dist[i][j] == 0)
					printf("%3.2f\t", (double)0);
				else
					printf("%3.2f%%\t", (double)
					    (stats->pgsize_ovfl_dist[i][j] *
					    100) / ovfl_cnt[i]);
			}
		}

		printf("\n\n\n\n %55s\n\n",
		    "=====Summary of simulated statistic=====");
		printf("  %s\n  %s\n  %s\n  %s\n  %s\n  %s\n\n",
		    "pgsize: \tpage size", "storage: \ton-disk space",
		    "pgcnt: \ttotal number of all pages "
		    "(e.g, sum of ovflcnt, leafcnt, dupcnt)",
		    "ovflcnt: \tnumber of over flow pages",
		    "leafcnt: \tnumber of leaf pages",
		    "dupcnt: \tnumber of duplicate pages");
		printf("%s%s%s%s%s%s\n", DIVIDE_LINE2, DIVIDE_LINE2,
		    DIVIDE_LINE2, DIVIDE_LINE2, DIVIDE_LINE2, DIVIDE_LINE2);
		printf(" %10s| %10s| %10s| %10s| %10s| %10s|\n", "pgsize",
		    "storage", "pgcnt", "ovflcnt", "leafcnt", "dupcnt");
		printf("%s%s%s%s%s%s\n", DIVIDE_LINE2, DIVIDE_LINE2,
		    DIVIDE_LINE2, DIVIDE_LINE2, DIVIDE_LINE2, DIVIDE_LINE2);
		for (i = 0; i < NUM_PGSIZES; ++i) {
			printf(" %10d|", (1 << i) * DB_MIN_PGSIZE);
			printf(" %10u|", (u_int32_t)ttspace[i]);
			if (ttspace[i] != (u_int32_t)ttspace[i])
				printf("(truncated value reported)");
			printf(" %10u|", (u_int32_t)ttpgcnt[i]);
			if (ttpgcnt[i] != (u_int32_t)ttpgcnt[i])
				printf("(truncated value reported)");
			printf(" %10u|", (u_int32_t)ovfl_cnt[i]);
			if (ovfl_cnt[i] != (u_int32_t)ovfl_cnt[i])
				printf("(truncated value reported)");
			printf(" %10u|", (u_int32_t)leaf_cnt[i]);
			if (leaf_cnt[i] != (u_int32_t)leaf_cnt[i])
				printf("(truncated value reported)");
			printf(" %10u|", (u_int32_t)dup_cnt[i]);
			if (dup_cnt[i] != (u_int32_t)dup_cnt[i])
				printf("(truncated value reported)");
			printf("\n");
		}
		printf("%s%s%s%s%s%s\n", DIVIDE_LINE2, DIVIDE_LINE2,
		    DIVIDE_LINE2, DIVIDE_LINE2, DIVIDE_LINE2, DIVIDE_LINE2);
	}

	/*
	 * Choose a page size based on the overflow calculation. If there
	 * is no overflow consideration, then use the smallest on-disk
	 * space as a recommended page size.
	 */
	if (ovfl_cnt[0] == 0) {
		minispace =  ttspace[0];
		for (i = 1; i < NUM_PGSIZES; ++i)
			if ((ttspace[i] != 0) && (minispace > ttspace[i])) {
				minispace = ttspace[i];
				best_indx = i;
			}
	} else
		for (i = 1; i < NUM_PGSIZES; ++i)
			if ((ovfl_cnt[i - 1] - ovfl_cnt[i]) > 0.02 * ttpgcnt[i])
				best_indx = i;

	printf("\n\nFor your input database, we recommend page size = %d \n \t"
	    "out of 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536 for you.\n",
	    (1 << best_indx) * DB_MIN_PGSIZE);

	return (EXIT_SUCCESS);
}

/* Open the specific existing database. */
static int
open_db(dbpp, dbenv, dbname, subdb)
	DB **dbpp;
	DB_ENV *dbenv;
	char *dbname;
	char *subdb;
{
	DB *dbp;
	int ret = 0;

	if ((ret = db_create(&dbp, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_create fails.\n");
		return (ret);
	}

	*dbpp = dbp;

	/* Open a database for read-only.*/
	if ((ret =
	    dbp->open(dbp, NULL, dbname, subdb, DB_UNKNOWN, DB_RDONLY, 0)) != 0)
		dbenv->err(dbenv, ret, "DB->open");

	return (ret);
}

/* Usage flag information to indicate what can user query for given database.*/
static int
usage()
{
	fprintf(stderr, "usage: %s %s\n", progname,
	    "[-c cachesize] -d file [-h home] [-s database] [-v verbose]");
	exit(EXIT_FAILURE);
}

/*Check the verion of Berkeley DB libaray, make sure it is the right version.*/
static int
version_check()
{
	int v_major, v_minor, v_patch;

	/* Make sure we're loaded with the right version of the DB library. */
	(void)db_version(&v_major, &v_minor, &v_patch);
	if (v_major != DB_VERSION_MAJOR || v_minor != DB_VERSION_MINOR) {
		fprintf(stderr, DB_STR_A("5002",
		    "%s: version %d.%d doesn't match library version %d.%d\n",
		    "%s %d %d %d %d"), progname, DB_VERSION_MAJOR,
		    DB_VERSION_MINOR, v_major, v_minor);

		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}
