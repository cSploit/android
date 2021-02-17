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

static ConfigDriver perl_cf;

enum {
	PERL_MODULE = 1,
	PERL_PATH,
	PERL_CONFIG
};

static ConfigTable perlcfg[] = {
	{ "perlModule", "module", 2, 2, 0,
		ARG_STRING|ARG_MAGIC|PERL_MODULE, perl_cf, 
		"( OLcfgDbAt:11.1 NAME 'olcPerlModule' "
			"DESC 'Perl module name' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "perlModulePath", "path", 2, 2, 0,
		ARG_MAGIC|PERL_PATH, perl_cf, 
		"( OLcfgDbAt:11.2 NAME 'olcPerlModulePath' "
			"DESC 'Perl module path' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "filterSearchResults", "on|off", 2, 2, 0, ARG_ON_OFF|ARG_OFFSET,
		(void *)offsetof(PerlBackend, pb_filter_search_results),
		"( OLcfgDbAt:11.3 NAME 'olcPerlFilterSearchResults' "
			"DESC 'Filter search results before returning to client' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "perlModuleConfig", "args", 2, 0, 0,
		ARG_MAGIC|PERL_CONFIG, perl_cf, 
		"( OLcfgDbAt:11.4 NAME 'olcPerlModuleConfig' "
			"DESC 'Perl module config directives' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL }
};

static ConfigOCs perlocs[] = {
	{ "( OLcfgDbOc:11.1 "
		"NAME 'olcDbPerlConfig' "
		"DESC 'Perl DB configuration' "
		"SUP olcDatabaseConfig "
		"MUST ( olcPerlModulePath $ olcPerlModule ) "
		"MAY ( olcPerlFilterSearchResults $ olcPerlModuleConfig ) )",
			Cft_Database, perlcfg, NULL, NULL },
	{ NULL }
};

static ConfigOCs ovperlocs[] = {
	{ "( OLcfgDbOc:11.2 "
		"NAME 'olcovPerlConfig' "
		"DESC 'Perl overlay configuration' "
		"SUP olcOverlayConfig "
		"MUST ( olcPerlModulePath $ olcPerlModule ) "
		"MAY ( olcPerlFilterSearchResults $ olcPerlModuleConfig ) )",
			Cft_Overlay, perlcfg, NULL, NULL },
	{ NULL }
};

/**********************************************************
 *
 * Config
 *
 **********************************************************/
int
perl_back_db_config(
	BackendDB *be,
	const char *fname,
	int lineno,
	int argc,
	char **argv
)
{
	int rc = config_generic_wrapper( be, fname, lineno, argc, argv );
	/* backward compatibility: map unknown directives to perlModuleConfig */
	if ( rc == SLAP_CONF_UNKNOWN ) {
		char **av = ch_malloc( (argc+2) * sizeof(char *));
		int i;
		av[0] = "perlModuleConfig";
		av++;
		for ( i=0; i<argc; i++ )
			av[i] = argv[i];
		av[i] = NULL;
		av--;
		rc = config_generic_wrapper( be, fname, lineno, argc+1, av );
		ch_free( av );
	}
	return rc;
}

static int
perl_cf(
	ConfigArgs *c
)
{
	PerlBackend *pb = (PerlBackend *) c->be->be_private;
	SV* loc_sv;
	int count ;
	int args;
	int rc = 0;
	char eval_str[EVAL_BUF_SIZE];
	struct berval bv;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch( c-> type ) {
		case PERL_MODULE:
			if ( !pb->pb_module_name )
				return 1;
			c->value_string = ch_strdup( pb->pb_module_name );
			break;
		case PERL_PATH:
			if ( !pb->pb_module_path )
				return 1;
			ber_bvarray_dup_x( &c->rvalue_vals, pb->pb_module_path, NULL );
			break;
		case PERL_CONFIG:
			if ( !pb->pb_module_config )
				return 1;
			ber_bvarray_dup_x( &c->rvalue_vals, pb->pb_module_config, NULL );
			break;
		}
	} else if ( c->op == LDAP_MOD_DELETE ) {
		/* FIXME: none of this affects the state of the perl
		 * interpreter at all. We should probably destroy it
		 * and recreate it...
		 */
		switch( c-> type ) {
		case PERL_MODULE:
			ch_free( pb->pb_module_name );
			pb->pb_module_name = NULL;
			break;
		case PERL_PATH:
			if ( c->valx < 0 ) {
				ber_bvarray_free( pb->pb_module_path );
				pb->pb_module_path = NULL;
			} else {
				int i = c->valx;
				ch_free( pb->pb_module_path[i].bv_val );
				for (; pb->pb_module_path[i].bv_val; i++ )
					pb->pb_module_path[i] = pb->pb_module_path[i+1];
			}
			break;
		case PERL_CONFIG:
			if ( c->valx < 0 ) {
				ber_bvarray_free( pb->pb_module_config );
				pb->pb_module_config = NULL;
			} else {
				int i = c->valx;
				ch_free( pb->pb_module_config[i].bv_val );
				for (; pb->pb_module_config[i].bv_val; i++ )
					pb->pb_module_config[i] = pb->pb_module_config[i+1];
			}
			break;
		}
	} else {
		switch( c->type ) {
		case PERL_MODULE:
			snprintf( eval_str, EVAL_BUF_SIZE, "use %s;", c->argv[1] );
			eval_pv( eval_str, 0 );

			if (SvTRUE(ERRSV)) {
				STRLEN len;

				snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: error %s",
					c->log, SvPV(ERRSV, len ));
				Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
				rc = 1;
			} else {
				dSP; ENTER; SAVETMPS;
				PUSHMARK(sp);
				XPUSHs(sv_2mortal(newSVpv(c->argv[1], 0)));
				PUTBACK;

				count = call_method("new", G_SCALAR);

				SPAGAIN;

				if (count != 1) {
					croak("Big trouble in config\n") ;
				}

				pb->pb_obj_ref = newSVsv(POPs);

				PUTBACK; FREETMPS; LEAVE ;
				pb->pb_module_name = ch_strdup( c->argv[1] );
			}
			break;

		case PERL_PATH:
			snprintf( eval_str, EVAL_BUF_SIZE, "push @INC, '%s';", c->argv[1] );
			loc_sv = eval_pv( eval_str, 0 );
			/* XXX loc_sv return value is ignored. */
			ber_str2bv( c->argv[1], 0, 0, &bv );
			value_add_one( &pb->pb_module_path, &bv );
			break;

		case PERL_CONFIG: {
			dSP ;  ENTER ; SAVETMPS;

			PUSHMARK(sp) ;
			XPUSHs( pb->pb_obj_ref );

			/* Put all arguments on the perl stack */
			for( args = 1; args < c->argc; args++ ) {
				XPUSHs(sv_2mortal(newSVpv(c->argv[args], 0)));
			}

			PUTBACK ;

			count = call_method("config", G_SCALAR);

			SPAGAIN ;

			if (count != 1) {
				croak("Big trouble in config\n") ;
			}

			rc = POPi;

			PUTBACK ; FREETMPS ;  LEAVE ;
			}
			break;
		}
	}
	return rc;
}

int
perl_back_init_cf( BackendInfo *bi )
{
	bi->bi_cf_ocs = perlocs;

	return config_register_schema( perlcfg, perlocs );
}
