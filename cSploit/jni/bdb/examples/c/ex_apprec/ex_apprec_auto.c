/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "db_int.h"
#include "dbinc/db_swap.h"
#include "ex_apprec.h"
DB_LOG_RECSPEC ex_apprec_mkdir_desc[] = {
	{LOGREC_DBT, SSZ(ex_apprec_mkdir_args, dirname), "dirname", ""},
	{LOGREC_Done, 0, "", ""}
};
