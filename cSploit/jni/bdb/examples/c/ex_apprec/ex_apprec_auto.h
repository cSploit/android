/* Do not edit: automatically built by gen_rec.awk. */

#ifndef	ex_apprec_AUTO_H
#define	ex_apprec_AUTO_H
#define	DB_ex_apprec_mkdir	10000
typedef struct _ex_apprec_mkdir_args {
	u_int32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	dirname;
} ex_apprec_mkdir_args;

extern __DB_IMPORT DB_LOG_RECSPEC ex_apprec_mkdir_desc[];
static inline int
ex_apprec_mkdir_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, u_int32_t flags,
    const DBT *dirname)
{
	return (dbenv->log_put_record(dbenv, NULL, txnp, ret_lsnp,
	    flags, DB_ex_apprec_mkdir, 0,
	    sizeof(u_int32_t) + sizeof(u_int32_t) + sizeof(DB_LSN) +
	    LOG_DBT_SIZE(dirname),
	    ex_apprec_mkdir_desc,
	    dirname));
}

static inline int ex_apprec_mkdir_read(DB_ENV *dbenv, 
    void *data, ex_apprec_mkdir_args **arg)
{
	return (dbenv->log_read_record(dbenv, 
	    NULL, NULL, data, ex_apprec_mkdir_desc, sizeof(ex_apprec_mkdir_args), (void**)arg));
}
#endif
