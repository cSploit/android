#include "common.h"

/* TODO place comment here */

static char software_version[] = "$Id: base.c,v 1.2 2010-07-05 09:20:32 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char *argv[])
{
	/* TODO remove if not neeeded */
	odbc_use_version3 = 1;
	odbc_connect();

	/* TODO write your test */

	odbc_disconnect();
	return 0;
}
