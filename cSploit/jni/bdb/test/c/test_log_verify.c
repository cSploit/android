/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$ 
 */
#include "db_config.h"
#include "db_int.h"
#include "db.h"
#include "dbinc/log_verify.h"

static int put_get_cmp_ckp __P((DB_LOG_VRFY_INFO *, VRFY_CKP_INFO *, 
    DB_LSN));

static int put_get_cmp_ts __P((DB_LOG_VRFY_INFO *, VRFY_TIMESTAMP_INFO *, 
    DB_LSN));
static int put_get_cmp_freg __P((DB_LOG_VRFY_INFO *, VRFY_FILEREG_INFO *, 
    const DBT *));
static int put_get_cmp __P((DB_LOG_VRFY_INFO *, VRFY_TXN_INFO *, u_int32_t));
static int dbt_cmp __P((const DBT *, const DBT *));
static int dbtarr_cmp __P((const DBT *, const DBT *, u_int32_t));
/* 
 * __rem_last_recycle_lsn , clear_fileups __put_filelife, __del_filelife
 * __get_filelife __get_filereg_by_dbregid __add_dbregid __get_last_ckp_info
 * __get_latest_timestamp_info _find_lsnrg_by_timerg __add_txnrange
 * __get_aborttxn __txn_started __add_page_to_txn __del_txn_pages
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	int i, ret;
	DB_LOG_VERIFY_CONFIG cfg;
	DB_LOG_VRFY_INFO *lvinfop;
	VRFY_TXN_INFO txninfo;
	VRFY_FILEREG_INFO freginfo;
	VRFY_CKP_INFO ckpinfo;
	VRFY_TIMESTAMP_INFO tsinfo;
	DB_LSN rlsn;
	char *buf;
	u_int32_t bufsz;
	DBT fid, *qdbt;
	DB_THREAD_INFO *ip;
	DB_ENV *dbenv;

	memset(&cfg, 0, sizeof(cfg));
	buf = malloc(bufsz = 2048);// trash bytes to make DBT fileids.
	cfg.temp_envhome = NULL;
	cfg.cachesize = 8 * 1024 * 1024;

	lvinfop = NULL;
	memset(&txninfo, 0, sizeof(txninfo));
	memset(&freginfo, 0, sizeof(freginfo));
	memset(&ckpinfo, 0, sizeof(ckpinfo));
	memset(&tsinfo, 0, sizeof(tsinfo));
	memset(&fid, 0, sizeof(fid));
	db_env_create(&dbenv, 0);
	dbenv->open(dbenv, NULL, DB_CREATE | DB_INIT_MPOOL, 0644);

	ENV_ENTER(dbenv->env, ip);
	if (__create_log_vrfy_info(&cfg, &lvinfop, ip))
		return -1;
	

	txninfo.txnid = 80000001;
	rlsn.file = 1;
	put_get_cmp(lvinfop, &txninfo, txninfo.txnid);
	for (i = 1000; i <= 2000; i += 100) {
		rlsn.offset = i;
		if (ret = __add_recycle_lsn_range(lvinfop, &rlsn, 80000000, 80000300))
			goto err;
		if (ret = put_get_cmp(lvinfop, &txninfo, txninfo.txnid))
			goto err;
		if (i % 200) {
			fid.data = buf + abs(rand()) % (bufsz / 2);
			fid.size = (char *)fid.data - buf;
			if (ret = __add_file_updated(&txninfo, &fid, i))
				goto err;
		}
		if ((i % 200 == 0) && (ret = __del_file_updated(&txninfo, &fid)))
			goto err;
		if (ret = put_get_cmp(lvinfop, &txninfo, txninfo.txnid))
			goto err;
	}
	freginfo.fileid = fid;
	freginfo.fname = "mydb.db";
	if (ret = put_get_cmp_freg(lvinfop, &freginfo, &freginfo.fileid))
		goto err;

	ckpinfo.lsn.file = 2;
	ckpinfo.lsn.offset = 3201;
	ckpinfo.ckplsn.file = 2;
	ckpinfo.ckplsn.offset = 2824;
	if (ret = put_get_cmp_ckp(lvinfop, &ckpinfo, ckpinfo.lsn))
		goto err;

	tsinfo.lsn.file = 1;
	tsinfo.lsn.offset = 829013;
	tsinfo.timestamp = time(NULL);
	tsinfo.logtype = 123;
	if (ret = put_get_cmp_ts(lvinfop, &tsinfo, tsinfo.lsn))
		goto err;

err:
	__destroy_log_vrfy_info(lvinfop);
	ENV_LEAVE(dbenv->env, ip);
	dbenv->close(dbenv, 0);
	return ret;
}

static int
put_get_cmp_ckp(lvinfop, ckp, lsn)
	DB_LOG_VRFY_INFO *lvinfop;
	VRFY_CKP_INFO *ckp;
	DB_LSN lsn;
{
	int ret;
	VRFY_CKP_INFO *ckppp;

	ckppp = NULL;
	if (ret = __put_ckp_info(lvinfop, ckp))
		goto err;

	if (ret = __get_ckp_info(lvinfop, lsn, &ckppp))
		goto err;
	if (memcmp(ckp, ckppp, sizeof(VRFY_CKP_INFO))) {
		fprintf(stderr, 
"\n__get_ckp_info got different ckp info than the one put by __put_ckp_info");
		goto err;
	}
err:
	if (ckppp)
		__os_free(NULL, ckppp);
	if (ret)
		printf("\nError in put_get_cmp_ckp");
	return ret;
}

static int
put_get_cmp_ts(lvinfop, ts, lsn)
	DB_LOG_VRFY_INFO *lvinfop;
	VRFY_TIMESTAMP_INFO *ts;
	DB_LSN lsn;
{
	int ret;
	VRFY_TIMESTAMP_INFO *tsp;

	tsp = NULL;
	if (ret = __put_timestamp_info(lvinfop, ts))
		goto err;

	if (ret = __get_timestamp_info(lvinfop, lsn, &tsp))
		goto err;
	if (memcmp(ts, tsp, sizeof(VRFY_TIMESTAMP_INFO))) {
		fprintf(stderr, 
"\n__get_timestamp_info got different timestamp info than the one put by __put_timestamp_info");
		goto err;
	}
err:
	if (tsp)
		__os_free(NULL, tsp);
	if (ret)
		printf("\nError in put_get_cmp_ts");
	return ret;
}

static int
put_get_cmp_freg(lvinfop, freg, fid)
	DB_LOG_VRFY_INFO *lvinfop;
	VRFY_FILEREG_INFO *freg;
	const DBT *fid;
{
	int ret;
	VRFY_FILEREG_INFO *freginfop;

	freginfop = NULL;
	if (ret = __put_filereg_info(lvinfop, freg))
		goto err;

	if (ret = __get_filereg_info(lvinfop, fid, &freginfop))
		goto err;
	if (memcmp(freg, freginfop, FILE_REG_INFO_FIXSIZE) || 
	    dbt_cmp(&(freg->fileid), &(freginfop->fileid)) ||
	    strcmp(freg->fname, freginfop->fname)) {
		fprintf(stderr, 
"\n__get_filereg_info got different filereg info than the one put by __put_filereg_info");
		goto err;
	}
err:
	
	if (freginfop)
		__free_filereg_info(freginfop);
	if (ret)
		printf("\nError in put_get_cmp_freg");
	return ret;
}


static int 
dbt_cmp(d1, d2)
	const DBT *d1;
	const DBT *d2;
{
	int ret;

	if (ret = d1->size - d2->size)
		return ret;

	if (ret = memcmp(d1->data, d2->data, d1->size))
		return ret;

	return 0;
}

static int 
dbtarr_cmp(a1, a2, len)
	const DBT *a1;
	const DBT *a2;
	u_int32_t len;
{
	int i, ret;

	for (i = 0; i < len; i++) {
		if (ret = a1[i].size - a2[i].size)
			return ret;
		if (ret = memcmp(a1[i].data, a2[i].data, a1[i].size))
			return ret;
	}

	return 0;
}

static int
put_get_cmp(lvinfop, txninfo, tid)
	DB_LOG_VRFY_INFO *lvinfop;
	VRFY_TXN_INFO *txninfo;
	u_int32_t tid;
{
	int ret;
	VRFY_TXN_INFO *txninfop;

	txninfop = NULL;
	if (ret = __put_txn_vrfy_info(lvinfop, txninfo))
		goto err;

	if (ret = __get_txn_vrfy_info(lvinfop, tid, &txninfop))
		goto err;
	if (memcmp(txninfo, txninfop, TXN_VERIFY_INFO_FIXSIZE) ||
	    memcmp(txninfo->recycle_lsns, txninfop->recycle_lsns, 
	    sizeof(DB_LSN) * txninfo->num_recycle) || 
	    dbtarr_cmp(txninfo->fileups, txninfop->fileups, 
	    txninfop->filenum)) {
		fprintf(stderr, 
"\n__get_txn_vrfy_info got different txinfo than the one put by __put_txn_vrfy_info");
		goto err;
	}
err:
	if (txninfop)
		__free_txninfo(txninfop);
	if (ret)
		printf("\nError in put_get_cmp");
	return ret;
}
