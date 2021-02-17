/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2010-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include <portable.h>

#ifndef SLAPD_MOD_KINIT
#define SLAPD_MOD_KINIT SLAPD_MOD_DYNAMIC
#endif

#ifdef SLAPD_MOD_KINIT

#include <slap.h>
#include "ldap_rq.h"
#include <ac/errno.h>
#include <ac/string.h>
#include <krb5/krb5.h>

typedef struct kinit_data {
	krb5_context ctx;
	krb5_ccache ccache;
	krb5_keytab keytab;
	krb5_principal princ;
	krb5_get_init_creds_opt *opts;
} kinit_data;

static char* principal;
static char* kt_name;
static kinit_data *kid;

static void
log_krb5_errmsg( krb5_context ctx, const char* func, krb5_error_code rc )
{
	const char* errmsg = krb5_get_error_message(ctx, rc);
	Log2(LDAP_DEBUG_ANY, LDAP_LEVEL_ERR, "slapd-kinit: %s: %s\n", func, errmsg);
	krb5_free_error_message(ctx, errmsg);
	return;
}

static int
kinit_check_tgt(kinit_data *kid, int *remaining)
{
	int ret=3;
	krb5_principal princ;
	krb5_error_code rc;
	krb5_cc_cursor cursor;
	krb5_creds creds;
	char *name;
	time_t now=time(NULL);

	rc = krb5_cc_get_principal(kid->ctx, kid->ccache, &princ);
	if (rc) {
		log_krb5_errmsg(kid->ctx, "krb5_cc_get_principal", rc);
		return 2;
	} else {
		if (!krb5_principal_compare(kid->ctx, kid->princ, princ)) {
			Log0(LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
					"Principal in ccache does not match requested principal\n");
			krb5_free_principal(kid->ctx, princ);
			return 2;
		}
	}

	rc = krb5_cc_start_seq_get(kid->ctx, kid->ccache, &cursor);
	if (rc) {
		log_krb5_errmsg(kid->ctx, "krb5_cc_start_seq_get", rc);
		krb5_free_principal(kid->ctx, princ);
		return -1;
	}

	while (!(rc = krb5_cc_next_cred(kid->ctx, kid->ccache, &cursor, &creds))) {
		if (krb5_is_config_principal(kid->ctx, creds.server)) {
			krb5_free_cred_contents(kid->ctx, &creds);
			continue;
		}

		if (creds.server->length==2 &&
				(!strcmp(creds.server->data[0].data, "krbtgt")) &&
				(!strcmp(creds.server->data[1].data, princ->realm.data))) {

			krb5_unparse_name(kid->ctx, creds.server, &name);

			*remaining = (time_t)creds.times.endtime-now;
			if ( *remaining <= 0) {
				Log1(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG,
						"kinit_qtask: TGT (%s) expired\n", name);
			} else {
				Log4(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG,
						"kinit_qtask: TGT (%s) expires in %dh:%02dm:%02ds\n",
						name, *remaining/3600, (*remaining%3600)/60, *remaining%60);
			}
			free(name);

			if (*remaining <= 30) {
				if (creds.times.renew_till-60 > now) {
					int renewal=creds.times.renew_till-now;
					Log3(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG,
							"kinit_qtask: Time remaining for renewal: %dh:%02dm:%02ds\n",
							renewal/3600, (renewal%3600)/60,  renewal%60);
					ret = 1;
				} else {
					Log0(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG,
							"kinit_qtask: Only short time left for renewal. "
							"Trying to re-init.\n");
					ret = 2;
				}
			} else {
				ret=0;
			}
			krb5_free_cred_contents(kid->ctx, &creds);
			break;
		}
		krb5_free_cred_contents(kid->ctx, &creds);

	}
	krb5_cc_end_seq_get(kid->ctx, kid->ccache, &cursor);
	krb5_free_principal(kid->ctx, princ);
	return ret;
}

void*
kinit_qtask( void *ctx, void *arg )
{
	struct re_s     *rtask = arg;
	kinit_data	*kid = (kinit_data*)rtask->arg;
	krb5_error_code rc;
	krb5_creds creds;
	int nextcheck, remaining, renew=0;
	Log0(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG, "kinit_qtask: running TGT check\n");

	memset(&creds, 0, sizeof(creds));

	renew = kinit_check_tgt(kid, &remaining);

	if (renew > 0) {
		if (renew==1) {
			Log0(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG,
					"kinit_qtask: Trying to renew TGT: ");
			rc = krb5_get_renewed_creds(kid->ctx, &creds, kid->princ, kid->ccache, NULL);
			if (rc!=0) {
				Log0(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG, "Failed\n");
				log_krb5_errmsg( kid->ctx,
						"kinit_qtask, Renewal failed: krb5_get_renewed_creds", rc );
				renew++;
			} else {
				Log0(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG, "Success\n");
				krb5_cc_initialize(kid->ctx, kid->ccache, creds.client);
				krb5_cc_store_cred(kid->ctx, kid->ccache, &creds);
				krb5_free_cred_contents(kid->ctx, &creds);
				renew=kinit_check_tgt(kid, &remaining);
			}
		}
		if (renew > 1) {
			Log0(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG,
					"kinit_qtask: Trying to get new TGT: ");
			rc = krb5_get_init_creds_keytab( kid->ctx, &creds, kid->princ,
					kid->keytab, 0, NULL, kid->opts);
			if (rc) {
				Log0(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG, "Failed\n");
				log_krb5_errmsg(kid->ctx, "krb5_get_init_creds_keytab", rc);
			} else {
				Log0(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG, "Success\n");
				renew=kinit_check_tgt(kid, &remaining);
			}
			krb5_free_cred_contents(kid->ctx, &creds);
		}
	}
	if (renew == 0) {
		nextcheck = remaining-30;
	} else {
		nextcheck = 60;
	}

	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	if ( ldap_pvt_runqueue_isrunning( &slapd_rq, rtask )) {
		ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	}
	Log3(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG,
			"kinit_qtask: Next TGT check in %dh:%02dm:%02ds\n",
			nextcheck/3600, (nextcheck%3600)/60,  nextcheck%60);
	rtask->interval.tv_sec = nextcheck;
	ldap_pvt_runqueue_resched( &slapd_rq, rtask, 0 );
	slap_wake_listener();
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	return NULL;
}

int
kinit_initialize(void)
{
	Log0( LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG, "kinit_initialize\n" );
	krb5_error_code rc;
	struct re_s *task = NULL;

	kid = ch_calloc(1, sizeof(kinit_data) );

	rc = krb5_init_context( &kid->ctx );
	if ( !rc )
		rc = krb5_cc_default(kid->ctx, &kid->ccache );

	if ( !rc ) {
		if (!principal) {
			int len=STRLENOF("ldap/")+global_host_bv.bv_len+1;
			principal=ch_calloc(len, 1);
			snprintf(principal, len, "ldap/%s", global_host_bv.bv_val);
			Log1(LDAP_DEBUG_TRACE, LDAP_LEVEL_DEBUG, "Principal <%s>\n", principal);

		}
		rc = krb5_parse_name(kid->ctx, principal, &kid->princ);
	}

	if ( !rc && kt_name) {
		rc = krb5_kt_resolve(kid->ctx, kt_name, &kid->keytab);
	}

	if ( !rc )
		rc = krb5_get_init_creds_opt_alloc(kid->ctx, &kid->opts);

	if ( !rc )
		rc = krb5_get_init_creds_opt_set_out_ccache( kid->ctx, kid->opts, kid->ccache);

	if ( !rc ) {
		ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
		task = ldap_pvt_runqueue_insert( &slapd_rq, 10, kinit_qtask, (void*)kid,
				"kinit_qtask", "ldap/bronsted.g17.lan@G17.LAN" );
		ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	}

	if (rc) {
		log_krb5_errmsg(kid->ctx, "kinit_initialize", rc);
		rc = -1;
	}
	return rc;
}

#if SLAPD_MOD_KINIT == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	if (argc > 0) {
		principal = ch_strdup(argv[0]);
	}
	if (argc > 1) {
		kt_name = ch_strdup(argv[1]);
	}
	if (argc > 2) {
		return -1;
	}
	return kinit_initialize();
}

int
term_module() {
	if (principal)
		ch_free(principal);
	if (kt_name)
		ch_free(kt_name);
	if (kid) {
		struct re_s *task;

		task=ldap_pvt_runqueue_find( &slapd_rq, kinit_qtask, (void*)kid);
		if (task) {
			if ( ldap_pvt_runqueue_isrunning(&slapd_rq, task) ) {
				ldap_pvt_runqueue_stoptask(&slapd_rq, task);
			}
			ldap_pvt_runqueue_remove(&slapd_rq, task);
		}
		if ( kid->ctx ) {
			if ( kid->princ )
				krb5_free_principal(kid->ctx, kid->princ);
			if ( kid->ccache )
				krb5_cc_close(kid->ctx, kid->ccache);
			if ( kid->keytab )
				krb5_kt_close(kid->ctx, kid->keytab);
			if ( kid->opts )
				krb5_get_init_creds_opt_free(kid->ctx, kid->opts);
			krb5_free_context(kid->ctx);
		}
		ch_free(kid);
	}
	return 0;
}
#endif

#endif /* SLAPD_MOD_KINIT */

