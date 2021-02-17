/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include <errno.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <db_cxx.h>

#define	DATABASE	"excxx_bulk.db"                   /* Database name */
#define	DATALEN		20                           /* The length of data */
#define	NS_PER_MS	1000000            /* Nanoseconds in a millisecond */
#define	NS_PER_US	1000               /* Nanoseconds in a microsecond */
#define	STRLEN		DATALEN - sizeof(int)      /* The length of string */
#define	UPDATES_PER_BULK_PUT	100

#ifdef _WIN32
#include <sys/timeb.h>
extern "C" {
	int getopt(int, char * const *, const char *);
	char *optarg;
}
/* Implement a basic high res timer with a POSIX interface for Windows. */
struct timeval {
	time_t tv_sec;
	long tv_usec;
};
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	struct _timeb now;
	_ftime(&now);
	tv->tv_sec = now.time;
	tv->tv_usec = now.millitm * NS_PER_US;
	return (0);
}
#define CLEANUP_CMD "rmdir EXCXX_BULK /q/s"
#else
#include <sys/time.h>
#include <unistd.h>
#define CLEANUP_CMD "rm -rf EXCXX_BULK"
#endif

int	compare_int(Db *, const Dbt *, const Dbt *, size_t *);
int	get_first_str(Db *, const Dbt *, const Dbt *, Dbt *);
int	get_string(const char *, char *, int);
static void	usage();

const char	*progname = "BulkExample";
const char	tstring[STRLEN] = "0123456789abcde";

struct data {
	int	id;
	char	str[STRLEN];
};

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

class BulkExample
{
public:
	BulkExample();
	~BulkExample();
	void bulkDelete(
	    int num, int dups, int *countp, int *iterp, int verbose);
	void bulkRead(int num, int dups, int iter, int *countp, int verbose);
	void bulkSecondaryDelete(
	    int num, int pair, int *countp, int *iterp, int verbose);
	void bulkUpdate(
	    int num, int dups, int *countp, int *iterp, int verbose);
	void initDb(int, int, int);
	void initDbenv(const char *, u_int);
	void run(int, char *[]);
	void throwException(DbEnv *, DbTxn *, int, const char *);
private:
	DbEnv *dbenv;
	Db *dbp, *sdbp;
	struct data *data_val;
	void *dbuf, *kbuf;
	u_int32_t dlen, klen;
};

void BulkExample::throwException(
    DbEnv *dbenvp, DbTxn *txn, int ret, const char *msg)
{
	if (txn != NULL) {
		(void)txn->abort();
		txn = NULL;
	}
	if (dbenvp != NULL && msg != NULL)
		dbenvp->err(ret, msg);
	throw DbException(ret);
}

void BulkExample::run(int argc, char *argv[])
{
	struct timeval start_time, end_time;
	double secs;
	int biter, ch, count, dflag, dups, init, iter, num, pair;
	int rflag, sflag, verbose;
	u_int32_t cachesize, pagesize;

	dflag = dups = init = rflag = sflag = verbose = 0;
	iter = num = 1000000;
	pagesize = 65536;
	cachesize = 1000 * pagesize;

	while ((ch = getopt(argc, argv, "c:Dd:Ii:n:p:RSv")) != EOF)
		switch (ch) {
		case 'c':
			cachesize = (u_int32_t)atoi(optarg);
			break;
		case 'D':
			dflag = 1;
			break;
		case 'd':
			dups = atoi(optarg);
			if (dups < 0)
				usage();
			break;
		case 'I':
			init = 1;
			break;
		case 'i':
			iter = atoi(optarg);
			break;
		case 'n':
			num = atoi(optarg);
			break;
		case 'p':
			pagesize = (u_int32_t)atoi(optarg);
			break;
		case 'R':
			rflag = 1;
			break;
		case 'S':
			sflag = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			usage();
		}

	/* Remove the previous environment and database(s). */
	if (!rflag) {
		system(CLEANUP_CMD);
		system("mkdir EXCXX_BULK");
	}

	initDbenv("EXCXX_BULK", cachesize);

	if (init)
		return;

	initDb(dups, sflag, pagesize);

	srand((int)time(NULL));
	if (rflag) {
		/* Time the get loop. */
		(void)gettimeofday(&start_time, NULL);
		bulkRead(num, dups, iter, &count, verbose);
		(void)gettimeofday(&end_time, NULL);
		secs =
		    (((double)end_time.tv_sec * 1000000 + 
		    end_time.tv_usec) -
		    ((double)start_time.tv_sec * 1000000 + 
		    start_time.tv_usec)) / 1000000;
		printf("[STAT] Read %d records read using %d batches",
		    count, iter);
		printf(" in %.2f seconds: ", secs);
		printf("%.0f records/second\n", (double)count / secs);
	} else {
		/* Time the fill loop. */
		(void)gettimeofday(&start_time, NULL);
		bulkUpdate(num, dups, &count, &biter, verbose);
		(void)gettimeofday(&end_time, NULL);
		secs = (((double)end_time.tv_sec * 1000000 + 
		    end_time.tv_usec) -
		    ((double)start_time.tv_sec * 1000000 +
		    start_time.tv_usec)) / 1000000;
		printf("[STAT] Insert %d records using %d batches",
		    count, biter);
		printf(" in %.2f seconds: ", secs);
		printf("%.0f records/second\n", (double)count / secs);
		if (dflag) {
			if (sflag) {
				pair = rand() % 2;
				/* Time the delete loop in secondary db */
				(void)gettimeofday(&start_time, NULL);
				bulkSecondaryDelete(num,
				    pair, &count, &iter, verbose);
				(void)gettimeofday(&end_time, NULL);
				secs = (((double)end_time.tv_sec * 1000000 + 
				    end_time.tv_usec) -
				    ((double)start_time.tv_sec * 1000000 + 
				    start_time.tv_usec)) / 1000000;
				printf("[STAT] Delete %d %s using %d batches",
				    count, (pair) ? "records" : "keys", iter);
				printf(" in %.2f seconds: ", secs);
				printf("%.0f records/second\n", 
				    (double)count / secs);
			} else {
				/* Time the delete loop in primary db */
				(void)gettimeofday(&start_time, NULL);
				bulkDelete(num, dups, &count, &iter, verbose);
				(void)gettimeofday(&end_time, NULL);
				secs = (((double)end_time.tv_sec * 1000000 + 
				    end_time.tv_usec) -
				    ((double)start_time.tv_sec * 1000000 + 
				    start_time.tv_usec)) / 1000000;
				printf(
"[STAT] Delete %d records using %d batches",
				    count, iter);
				printf(" in %.2f seconds: ",  secs);
				printf("%.0f records/second\n",  
				    (double)count / secs);
			}
		}
	}
}

/* Bulk delete from a database. */
void BulkExample::bulkDelete(
    int num, int dups, int *countp, int *iterp, int verbose)
{
	Dbt key;
	DbTxn *txnp;
	DbMultipleDataBuilder *ptrd;
	DbMultipleKeyDataBuilder *ptrkd;
	u_int32_t flag;
	int count, i, j, iter, ret;

	txnp = NULL;
	count = flag = iter = ret = 0;
	memset(&key, 0, sizeof(Dbt));

	j = rand() % num;

	/*
	 * The buffer must be at least as large as the page size of the
	 * underlying database and aligned for unsigned integer access.
	 * Its size must be a multiple of 1024 bytes.
	 */
	if (klen != (u_int32_t)UPDATES_PER_BULK_PUT *
	    (sizeof(u_int32_t) + DATALEN) * 1024) {
		klen = (u_int32_t)UPDATES_PER_BULK_PUT *
		    (sizeof(u_int32_t) + DATALEN) * 1024;
		kbuf = realloc(kbuf, klen);
	}
	memset(kbuf, 0, klen);
	key.set_ulen(klen);
	key.set_flags(DB_DBT_USERMEM | DB_DBT_BULK);
	key.set_data(kbuf);
	if (data_val == NULL)
		data_val = (data *)malloc(DATALEN);
	memset(data_val, 0, DATALEN);

	/*
	 * Bulk delete all records of a specific set of keys which includes all
	 * non-negative integers smaller than the random key. The random key is
	 * a random non-negative integer smaller than "num".
	 * If DB_MULTIPLE, construct the key Dbt by the DbMultipleDataBuilder
	 * with the specific set of keys. If DB_MULTIPLE_KEY, construct the key
	 * Dbt by the DbMultipleKeyDataBuilder with all key/data pairs of the
	 * specific set of keys.
	 */
	flag |= (dups) ? DB_MULTIPLE_KEY : DB_MULTIPLE;
	if (dups)
		ptrkd = new DbMultipleKeyDataBuilder(key);
	else
		ptrd = new DbMultipleDataBuilder(key);
	try {
		for (i = 0; i < j; i++) {
			if (i % UPDATES_PER_BULK_PUT == 0) {
				if (txnp != NULL) {
					ret = txnp->commit(0);
					txnp = NULL;
					if (ret != 0)
						throwException(dbenv, NULL,
						    ret, "DB_TXN->commit");
				}
				if ((ret = dbenv->txn_begin(NULL,
				    &txnp, 0)) != 0)
					throwException(dbenv, NULL,
					    ret, "DB_ENV->txn_begin");
			}

			if (dups) {
				data_val->id = 0;
				get_string(tstring, data_val->str, i);
				do {
					if(ptrkd->append(&i,sizeof(i),
					    data_val, DATALEN) == false)
						throwException(dbenv,
						    txnp, EXIT_FAILURE,
"DbMultipleKeyDataBuilder->append");
					count++;
					if (verbose)
						printf(
"Delete key: %d, \tdata: (id %d, str %s)\n",
						    i, data_val->id,
						    data_val->str);
				} while (data_val->id++ < dups);
			} else {
				if(ptrd->append(&i,sizeof(i)) == false)
					throwException(dbenv, txnp, ret,
					    "DbMultipleDataBuilder->append");
				count++;
				if (verbose)
					printf("Delete key: %d\n", i);
			}

			if ((i + 1) % UPDATES_PER_BULK_PUT == 0) {
				if ((ret = dbp->del(txnp, &key, flag)) != 0)
					throwException(dbenv,
					    txnp, ret, "Bulk DB->del");
				iter++;
				if (dups)
					ptrkd = new
					    DbMultipleKeyDataBuilder(key);
				else
					ptrd = new DbMultipleDataBuilder(key);
			}
		}
		if ((j % UPDATES_PER_BULK_PUT) != 0) {
			if ((ret = dbp->del(txnp, &key, flag)) != 0)
				throwException(dbenv,
				    txnp, ret, "Bulk DB->del");
			iter++;
		}

		ret = txnp->commit(0);
		txnp = NULL;
		if (ret != 0)
			throwException(dbenv, NULL, ret, "DB_TXN->commit");

		*countp = count;
		*iterp = iter;
	} catch (DbException &dbe) {
		cerr << "bulkDelete " << dbe.what() << endl;
		if (txnp != NULL)
			(void)txnp->abort();
		throw dbe;
	}
}

/* Bulk delete from a secondary db. */
void BulkExample::bulkSecondaryDelete(
    int num, int pair, int *countp, int *iterp, int verbose)
{
	Dbt key;
	DbTxn *txnp;
	DbMultipleDataBuilder *ptrd;
	DbMultipleKeyDataBuilder *ptrkd;
	u_int32_t flag;
	int count, i, iter, j, k, rc, ret;
	char ch;

	memset(&key, 0, sizeof(Dbt));
	txnp = NULL;
	count = flag = iter = ret = 0;
	rc = rand() % (STRLEN - 1);

	/*
	 * The buffer must be at least as large as the page size of the
	 * underlying database and aligned for unsigned integer access.
	 * Its size must be a multiple of 1024 bytes.
	 */
	if (klen != (u_int32_t)UPDATES_PER_BULK_PUT *
	    (sizeof(u_int32_t) + DATALEN) * 1024) {
		klen = (u_int32_t)UPDATES_PER_BULK_PUT *
		    (sizeof(u_int32_t) + DATALEN) * 1024;
		kbuf = realloc(kbuf, klen);
	}
	memset(kbuf, 0, klen);
	key.set_ulen(klen);
	key.set_flags(DB_DBT_USERMEM | DB_DBT_BULK);
	key.set_data(kbuf);

	/*
	 * Bulk delete all records of a specific set of keys which includes all
	 * characters before the random key in the tstring. The random key is
	 * one of the characters in the tstring.
	 * If DB_MULTIPLE, construct the key Dbt by the DbMultipleDataBuilder
	 * with the specific set of keys. If DB_MULTIPLE_KEY, construct the key
	 * Dbt by the DbMultipleKeyDataBuilder with all key/data pairs of the
	 * specific set of keys.
	 */
	flag |= (pair) ? DB_MULTIPLE_KEY : DB_MULTIPLE;
	if (pair)
		ptrkd = new DbMultipleKeyDataBuilder(key);
	else
		ptrd = new DbMultipleDataBuilder(key);
	try {
		for (i = 0; i <= rc; i++) {
			if (i % UPDATES_PER_BULK_PUT == 0) {
				if (txnp != NULL) {
					ret = txnp->commit(0);
					txnp = NULL;
					if (ret != 0)
						throwException(dbenv, NULL,
						    ret, "DB_TXN->commit");
				}
				if ((ret = dbenv->txn_begin(NULL,
				    &txnp, 0)) != 0)
					throwException(dbenv,
					    NULL, ret, "DB_ENV->txn_begin");
			}

			ch = tstring[i];
			if (!pair) {
				if (ptrd->append(&ch, sizeof(ch)) == false)
					throwException(dbenv,
					    txnp, EXIT_FAILURE,
					    "DbMultipleDataBuilder->append");
				count++;
				if (verbose)
					printf("Delete key: %c\n", ch);
			} else {
				j = 0;
				do {
					k = j * (STRLEN - 1) + i;
					if (ptrkd->append(&ch, sizeof(ch),
					    &k, sizeof(k)) == false)
						throwException(dbenv,
						    txnp, EXIT_FAILURE,
"DbMultipleKeyDataBuilder->append");
					count++;
					if (verbose)
						printf(
"Delete secondary key: %c, \tdata: %d\n", 
						    ch, k);
				} while (++j < (int)(num / (STRLEN - 1)));
			}

			if ((i + 1) % UPDATES_PER_BULK_PUT == 0) {
				if ((ret = sdbp->del(txnp, &key, flag)) != 0)
					throwException(dbenv,
					    txnp, ret, "Bulk DB->del");
				iter++;
				if (pair)
					ptrkd = new
					    DbMultipleKeyDataBuilder(key);
				else
					ptrd = new DbMultipleDataBuilder(key);
			}
		}
		if ((rc % UPDATES_PER_BULK_PUT) != 0) {
			if ((ret = sdbp->del(txnp, &key, flag)) != 0)
				throwException(dbenv,
				    txnp, ret, "Bulk DB->del");
			iter++;
		}

		ret = txnp->commit(0);
		txnp = NULL;
		if (ret != 0)
			throwException(dbenv, NULL, ret, "DB_TXN->commit");

		*countp = count;
		*iterp = iter;
	} catch (DbException &dbe) {
		cerr << "bulkSecondaryDelete " << dbe.what() << endl;
		if (txnp != NULL)
			(void)txnp->abort();
		throw dbe;
	}
}

/* Bulk fill a database. */
void BulkExample::bulkUpdate(
    int num, int dups, int *countp, int *iterp, int verbose)
{
	Dbt key, data;
	u_int32_t flag;
	DbTxn *txnp;
	int count, i, iter, ret;
	DbMultipleDataBuilder *ptrd, *ptrk;
	DbMultipleKeyDataBuilder *ptrkd;

	txnp = NULL;
	count = flag = iter = ret = 0;
	ptrk = ptrd = NULL;
	if (data_val == NULL)
		data_val = (struct data *)malloc(DATALEN);
	memset(data_val, 0, DATALEN);
	memset(&key, 0, sizeof(Dbt));
	memset(&data, 0, sizeof(Dbt));

	/* 
	 * The buffer must be at least as large as the page size of
	 * the underlying database and aligned for unsigned integer
	 * access. Its size must be a multiple of 1024 bytes.
	 */
	if (klen != (u_int32_t) UPDATES_PER_BULK_PUT *
	    (sizeof(u_int32_t) + DATALEN) * 1024) {
		klen = (u_int32_t) UPDATES_PER_BULK_PUT *
		    (sizeof(u_int32_t) + DATALEN) * 1024;
		kbuf = realloc(kbuf, klen);
	}
	memset(kbuf, 0, klen);
	key.set_ulen(klen);
	key.set_flags(DB_DBT_USERMEM | DB_DBT_BULK);
	key.set_data(kbuf);

	if (dlen != (u_int32_t) UPDATES_PER_BULK_PUT *
	    (sizeof(u_int32_t) + DATALEN) * 1024) {
		dlen = (u_int32_t) UPDATES_PER_BULK_PUT *
		    (sizeof(u_int32_t) + DATALEN) * 1024;
		dbuf = realloc(dbuf, dlen);
	}
	memset(dbuf, 0, dlen);
	data.set_ulen(dlen);
	data.set_flags(DB_DBT_USERMEM | DB_DBT_BULK);
	data.set_data(dbuf);

	/*
	 * Bulk insert with either DB_MULTIPLE in two buffers or
	 * DB_MULTIPLE_KEY in a single buffer. With DB_MULTIPLE, all keys are
	 * constructed in the key Dbt, and all data is constructed in the data
	 * Dbt. With DB_MULTIPLE_KEY, all key/data pairs are constructed in the
	 * key Dbt. We use DB_MULTIPLE mode when there are duplicate records.
	 */
	flag |= (dups) ? DB_MULTIPLE : DB_MULTIPLE_KEY;
	if (dups) {
		ptrk = new DbMultipleDataBuilder(key);
		ptrd = new DbMultipleDataBuilder(data);
	} else
		ptrkd = new DbMultipleKeyDataBuilder(key);

	try {
		for (i = 0; i < num; i++) {
			if (i % UPDATES_PER_BULK_PUT == 0) {
				if (txnp != NULL) {
					ret = txnp->commit(0);
					txnp = NULL;
					if (ret != 0)
						throwException(dbenv, NULL,
						    ret, "DB_TXN->commit");
				}
				if ((ret = dbenv->txn_begin(NULL,
				    &txnp, 0)) != 0)
					throwException(dbenv,
					    NULL, ret, "DB_ENV->txn_begin");
			}
			data_val->id = 0;
			get_string(tstring, data_val->str, i);
			do {
				if (dups) {
					if (ptrk->append(&i,
					    sizeof(i)) == false)
						throwException(dbenv,
						    txnp, EXIT_FAILURE,
"DbMultipleDataBuilder->append");
					if (ptrd->append(data_val,
					    DATALEN) == false)
						throwException(dbenv,
						    txnp, EXIT_FAILURE,
"DbMultipleDataBuilder->append");
				} else {
					if (ptrkd->append(&i, sizeof(i),
					    data_val, DATALEN) == false)
						throwException(dbenv,
						    txnp, EXIT_FAILURE,
"DbMultipleKeyDataBuilder->append");
				}
				if (verbose)
					printf(
"Insert key: %d, \t data: (id %d, str %s)\n",
					    i, data_val->id, data_val->str);
				count++;
			} while (data_val->id++ < dups);
			if ((i + 1) % UPDATES_PER_BULK_PUT == 0) {
				if ((ret = dbp->put(txnp,
					    &key, &data, flag)) != 0)
					throwException(dbenv,
					    txnp, ret, "Bulk DB->put");
				iter++;
				if (dups) {
					ptrk = new DbMultipleDataBuilder(key);
					ptrd = new DbMultipleDataBuilder(data);
				} else
					ptrkd = new
					    DbMultipleKeyDataBuilder(key);
			}
		} 
		if ((num % UPDATES_PER_BULK_PUT) != 0) {
			if ((ret = dbp->put(txnp, &key, &data, flag)) != 0)
				throwException(dbenv,
				    txnp, ret, "Bulk DB->put");
			iter++;
		}

		ret = txnp->commit(0);
		txnp = NULL;
		if (ret != 0)
			throwException(dbenv, NULL, ret, "DB_TXN->commit");

		*countp = count;
		*iterp = iter;
	} catch (DbException &dbe) {
		cerr << "bulkUpdate " << dbe.what() << endl;
		if (txnp != NULL)
			(void)txnp->abort();
		throw dbe;
	}
}

/* Loop get batches of records. */
void BulkExample::bulkRead(
    int num, int dups, int iter, int *countp, int verbose)
{
	Dbc *dbcp;
	Dbt data, dp, key, kp;
	DbTxn *txnp;
	DbMultipleDataIterator *ptrd;
	DbMultipleKeyDataIterator *ptrkd;
	u_int32_t flags;
	int count, i, j, ret;

	count = klen = ret = 0;
	dbcp = NULL;
	txnp = NULL;

	/* Initialize key Dbt and data Dbt, malloc bulk buffer. */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.set_size(sizeof(j));
	if (dlen != DATALEN * 16 * 1024) {
		dlen = DATALEN * 16 * 1024;
		dbuf = realloc(dbuf, dlen);
	}
	memset(dbuf, 0, dlen);
	data.set_flags(DB_DBT_USERMEM);
	data.set_data(dbuf);
	data.set_ulen(dlen);
	data.set_size(dlen);

	flags = DB_SET;
	flags |= (dups) ? DB_MULTIPLE: DB_MULTIPLE_KEY;
	try {
		for (i = 0; i < iter; i++) {
			if ((ret =
			    dbenv->txn_begin(NULL, &txnp, 0)) != 0)
				throwException(dbenv, NULL,
				    ret, "DB_ENV->txn_begin");
			if ((ret = dbp->cursor(txnp, &dbcp, 0)) != 0)
				throwException(dbenv, txnp,
				    ret, "DB->cursor");

			/*
			 * Bulk retrieve by a random key which is a random
			 * non-negative integer smaller than "num".
			 * If there are duplicates in the database, retrieve
			 * with DB_MULTIPLE and use the DbMultipleDataIterator
			 * to iterate the data of the random key in the data
			 * Dbt. Otherwise retrieve with DB_MULTIPLE_KEY and use
			 * the DbMultipleKeyDataIterator to iterate the
			 * key/data pairs of the specific set of keys which
			 * includes all integers >= the random key and < "num".
			 */
			j = rand() % num;
			key.set_data(&j);
			if ((ret = dbcp->get(&key, &data, flags)) != 0) 
				throwException(dbenv, NULL, ret, "DBC->get");

			if (dups) {
				ptrd = new DbMultipleDataIterator(data);
				while (ptrd->next(dp) == true) {
					count++;
					if (verbose)
						printf(
"Retrieve key: %d, \tdata: (id %d, str %s)\n", j, ((struct data *)(
						   dp.get_data()))->id,
						   (char *)((struct data *)(
						   dp.get_data()))->str);
				}
			} else {
				ptrkd = new DbMultipleKeyDataIterator(data);
				while (ptrkd->next(kp, dp) == true) {
					count++;
					if (verbose)
						printf(
"Retrieve key: %d, \tdata: (id %d, str %s)\n", *((int *)kp.get_data()),
						    ((struct data *)(
						    dp.get_data()))->id,
						    (char *)((struct data *)(
						    dp.get_data()))->str);
				}
			}

			ret = dbcp->close();
			dbcp = NULL;
			if (ret != 0)
				throwException(dbenv, txnp, ret, "DBC->close");

			ret = txnp->commit(0);
			txnp = NULL;
			if (ret != 0)
				throwException(dbenv, NULL,
				    ret, "DB_TXN->commit");
		} 

		*countp = count;
	} catch (DbException &dbe) {
		cerr << "bulkRead " << dbe.what() << endl;
		if (dbcp != NULL)
			(void)dbcp->close();
		if (txnp != NULL)
			(void)txnp->abort();
		throw dbe;
	}
}

/* Initialize the database. */
void BulkExample::initDb(int dups, int sflag, int pagesize) {

	DbTxn *txnp;
	int ret;

	txnp = NULL;
	ret = 0;

	dbp = new Db(dbenv, 0);

	dbp->set_error_stream(&cerr);
	dbp->set_errpfx(progname);

	try{
		if ((ret = dbp->set_bt_compare(compare_int)) != 0)
			throwException(dbenv, NULL, ret, "DB->set_bt_compare");

		if ((ret = dbp->set_pagesize(pagesize)) != 0)
			throwException(dbenv, NULL, ret, "DB->set_pagesize");

		if (dups && (ret = dbp->set_flags(DB_DUP)) != 0)
			throwException(dbenv, NULL, ret, "DB->set_flags");

		if ((ret = dbenv->txn_begin(NULL, &txnp, 0)) != 0)
			throwException(dbenv, NULL, ret, "DB_ENV->txn_begin");

		if ((ret = dbp->open(txnp, DATABASE, "primary", DB_BTREE,
		    DB_CREATE, 0664)) != 0)
			throwException(dbenv, txnp, ret, "DB->open");

		if (sflag) {
			sdbp = new Db(dbenv, 0);

			if ((ret = sdbp->set_flags(DB_DUPSORT)) != 0)
				throwException(dbenv, txnp,
				    ret, "DB->set_flags");

			if ((ret = sdbp->open(txnp, DATABASE, "secondary",
			    DB_BTREE, DB_CREATE, 0664)) != 0)
				throwException(dbenv, txnp, ret, "DB->open");

			if ((ret =  dbp->associate(
			    txnp, sdbp, get_first_str, 0)) != 0)
				throwException(dbenv, txnp,
				    ret, "DB->associate");
		}

		ret = txnp->commit(0);
		txnp = NULL;
		if (ret != 0)
			throwException(dbenv, NULL, ret, "DB_TXN->commit");
	} catch(DbException &dbe) {
		cerr << "initDb " << dbe.what() << endl;
		if (txnp != NULL)
			(void)txnp->abort();
		throw dbe;
	}
}

/* Initialize the environment. */
void BulkExample::initDbenv(const char *home, u_int32_t cachesize)
{
	int ret;

	ret = 0;

	dbenv = new DbEnv(0);

	dbenv->set_error_stream(&cerr);
	dbenv->set_errpfx(progname);

	try {
		if ((ret = dbenv->set_cachesize(0, cachesize, 0)) != 0)
			throwException(dbenv,
			    NULL, ret, "DB_ENV->set_cachesize");

		/* Open the environment with full transactional support. */
		if ((ret = dbenv->open(home, DB_CREATE | DB_INIT_LOCK |
		    DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN, 0)) != 0)
			throwException(dbenv, NULL, ret, "DB_ENV->open");
	} catch (DbException &dbe) {
		cerr << "initDbenv " << dbe.what() << endl;
		throw dbe;
	}
}

BulkExample::BulkExample() : dbenv(NULL), dbp(NULL), sdbp(NULL),
     data_val(NULL), dbuf(NULL), kbuf(NULL), dlen(0), klen(0) {}

BulkExample::~BulkExample()
{
	if (sdbp != NULL) {
		sdbp->close(0);
		delete sdbp;
	}
	if (dbp != NULL) {
		dbp->close(0);
		delete dbp;
	}
	if (dbenv != NULL) {
		dbenv->close(0);
		delete dbenv;
	}
	if (dbuf != NULL)
		free(dbuf);
	if (kbuf != NULL)
		free(kbuf);
	if (data_val != NULL)
		free(data_val);
}

int
compare_int(Db *dbp, const Dbt *a, const Dbt *b, size_t *locp)
{
	int ai, bi;

	locp = NULL;
	/*
	 * Returns:
	 * 	< 0 if a < b
	 * 	= 0 if a = b
	 * 	> 0 if a > b
	 */
	memcpy(&ai, a->get_data(), sizeof(int));
	memcpy(&bi, b->get_data(), sizeof(int));
	return (ai - bi);
}

int
get_first_str(Db *sdbp, const Dbt *key, const Dbt *data, Dbt *skey)
{
	memset(skey, 0, sizeof(Dbt));
	skey->set_data(((struct data *)(data->get_data()))->str);
	skey->set_size(sizeof(char));
	return (0);
}

int
get_string(const char *src, char *des, int off)
{
	int i;

	for (i = 0; i < (int)(STRLEN - 1); i++) 
		des[i] = src[(off + i) % (STRLEN - 1)];
	des[STRLEN - 1] = '\0';
	return (0);
}

static void
usage()
{
	cerr << "Usage: BulkExample \n"
	     << "-c	cachesize [1000 * pagesize] \n"
	     << "-D	perform bulk delete \n"
	     << "-d	number of duplicates [none] \n"
	     << "-I	just initialize the environment \n"
	     << "-i	number of read iterations [1000000] \n"
	     << "-n	number of keys [1000000] \n"
	     << "-p	pagesize [65536] \n"
	     << "-R	perform bulk read \n"
	     << "-S	perform bulk operation in secondary database \n"
	     << "-v	verbose output \n";
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	/*
	 * Use a try block just to report any errors.
	 * An alternate approach to using exceptions is to
	 * use error models (see DbEnv::set_error_model()) so
	 * that error codes are returned for all Berkeley DB methods.
	 */
	try {
		BulkExample app;
		app.run(argc, argv);
		return (EXIT_SUCCESS);
	} catch (DbException &dbe) {
		cerr << "BulkExample: " << dbe.what() << endl;
		return (EXIT_FAILURE);
	}
}
