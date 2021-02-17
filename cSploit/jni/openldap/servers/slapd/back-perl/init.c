/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 John C. Quillan.
 * Portions Copyright 2002 myinternet Limited.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "perl_back.h"
#include "../config.h"

#ifdef PERL_SYS_INIT3
#include <ac/unistd.h>		/* maybe get environ */
extern char **environ;
#endif

static void perl_back_xs_init LDAP_P((PERL_BACK_XS_INIT_PARAMS));
EXT void boot_DynaLoader LDAP_P((PERL_BACK_BOOT_DYNALOADER_PARAMS));

PerlInterpreter *PERL_INTERPRETER = NULL;
ldap_pvt_thread_mutex_t	perl_interpreter_mutex;


/**********************************************************
 *
 * Init
 *
 **********************************************************/

int
perl_back_initialize(
	BackendInfo	*bi
)
{
	char *embedding[] = { "", "-e", "0", NULL }, **argv = embedding;
	int argc = 3;
#ifdef PERL_SYS_INIT3
	char **env = environ;
#else
	char **env = NULL;
#endif

	bi->bi_open = NULL;
	bi->bi_config = 0;
	bi->bi_close = perl_back_close;
	bi->bi_destroy = 0;

	bi->bi_db_init = perl_back_db_init;
	bi->bi_db_config = perl_back_db_config;
	bi->bi_db_open = perl_back_db_open;
	bi->bi_db_close = 0;
	bi->bi_db_destroy = perl_back_db_destroy;

	bi->bi_op_bind = perl_back_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = perl_back_search;
	bi->bi_op_compare = perl_back_compare;
	bi->bi_op_modify = perl_back_modify;
	bi->bi_op_modrdn = perl_back_modrdn;
	bi->bi_op_add = perl_back_add;
	bi->bi_op_delete = perl_back_delete;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_chk_referrals = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	/* injecting code from perl_back_open, because using fonction reference  (bi->bi_open) is not functional */
	Debug( LDAP_DEBUG_TRACE, "perl backend open\n", 0, 0, 0 );

	if( PERL_INTERPRETER != NULL ) {
		Debug( LDAP_DEBUG_ANY, "perl backend open: already opened\n",
			0, 0, 0 );
		return 1;
	}
	
	ldap_pvt_thread_mutex_init( &perl_interpreter_mutex );

#ifdef PERL_SYS_INIT3
	PERL_SYS_INIT3(&argc, &argv, &env);
#endif
	PERL_INTERPRETER = perl_alloc();
	perl_construct(PERL_INTERPRETER);
#ifdef PERL_EXIT_DESTRUCT_END
	PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
#endif
	perl_parse(PERL_INTERPRETER, perl_back_xs_init, argc, argv, env);
	perl_run(PERL_INTERPRETER);
	return perl_back_init_cf( bi );
}

int
perl_back_db_init(
	BackendDB	*be,
	ConfigReply	*cr
)
{
	be->be_private = (PerlBackend *) ch_malloc( sizeof(PerlBackend) );
	memset( be->be_private, '\0', sizeof(PerlBackend));

	((PerlBackend *)be->be_private)->pb_filter_search_results = 0;

	Debug( LDAP_DEBUG_TRACE, "perl backend db init\n", 0, 0, 0 );

	be->be_cf_ocs = be->bd_info->bi_cf_ocs;

	return 0;
}

int
perl_back_db_open(
	BackendDB	*be,
	ConfigReply	*cr
)
{
	int count;
	int return_code;

	PerlBackend *perl_back = (PerlBackend *) be->be_private;

	ldap_pvt_thread_mutex_lock( &perl_interpreter_mutex );

	{
		dSP; ENTER; SAVETMPS;

		PUSHMARK(sp);
		XPUSHs( perl_back->pb_obj_ref );

		PUTBACK;

		count = call_method("init", G_SCALAR);

		SPAGAIN;

		if (count != 1) {
			croak("Big trouble in perl_back_db_open\n");
		}

		return_code = POPi;

		PUTBACK; FREETMPS; LEAVE;
	}

	ldap_pvt_thread_mutex_unlock( &perl_interpreter_mutex );

	return return_code;
}


static void
perl_back_xs_init(PERL_BACK_XS_INIT_PARAMS)
{
	char *file = __FILE__;
	dXSUB_SYS;
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
}

#if SLAPD_PERL == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( perl )

#endif /* SLAPD_PERL == SLAPD_MOD_DYNAMIC */


