/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "db_int.h"
#include "dbinc/log.h"
#include "ex_apprec.h"
/*
 * PUBLIC: int ex_apprec_mkdir_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops));
 */
int
ex_apprec_mkdir_print(dbenv, dbtp, lsnp, notused2)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
{
	notused2 = DB_TXN_PRINT;

	return (__log_print_record(dbenv->env, dbtp, lsnp, "ex_apprec_mkdir", ex_apprec_mkdir_desc, NULL));
}

/*
 * PUBLIC: int ex_apprec_init_print __P((DB_ENV *, DB_DISTAB *));
 */
int
ex_apprec_init_print(dbenv, dtabp)
	DB_ENV *dbenv;
	DB_DISTAB *dtabp;
{
	int __db_add_recovery __P((DB_ENV *, DB_DISTAB *,
	    int (*)(DB_ENV *, DBT *, DB_LSN *, db_recops), u_int32_t));
	int ret;

	if ((ret = __db_add_recovery(dbenv, dtabp,
	    ex_apprec_mkdir_print, DB_ex_apprec_mkdir)) != 0)
		return (ret);
	return (0);
}
