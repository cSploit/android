/* lastbind.c - Record timestamp of the last successful bind to entries */
/* $OpenLDAP$ */
/*
 * Copyright 2009 Jonathan Clarke <jonathan@phillipoux.net>.
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
/* ACKNOWLEDGEMENTS:
 * This work is loosely derived from the ppolicy overlay.
 */

#include "portable.h"

/*
 * This file implements an overlay that stores the timestamp of the
 * last successful bind operation in a directory entry.
 *
 * Optimization: to avoid performing a write on each bind,
 * a precision for this timestamp may be configured, causing it to
 * only be updated if it is older than a given number of seconds.
 */

#ifdef SLAPD_OVER_LASTBIND

#include <ldap.h>
#include "lutil.h"
#include "slap.h"
#include <ac/errno.h>
#include <ac/time.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include "config.h"

/* Per-instance configuration information */
typedef struct lastbind_info {
	/* precision to update timestamp in authTimestamp attribute */
	int timestamp_precision;
} lastbind_info;

/* Operational attributes */
static AttributeDescription *ad_authTimestamp;

/* This is the definition used by ISODE, as supplied to us in
 * ITS#6238 Followup #9
 */
static struct schema_info {
	char *def;
	AttributeDescription **ad;
} lastBind_OpSchema[] = {
	{	"( 1.3.6.1.4.1.453.16.2.188 "
		"NAME 'authTimestamp' "
		"DESC 'last successful authentication using any method/mech' "
		"EQUALITY generalizedTimeMatch "
		"ORDERING generalizedTimeOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
		"SINGLE-VALUE NO-USER-MODIFICATION USAGE dsaOperation )",
		&ad_authTimestamp},
	{ NULL, NULL }
};

/* configuration attribute and objectclass */
static ConfigTable lastbindcfg[] = {
	{ "lastbind-precision", "seconds", 2, 2, 0,
	  ARG_INT|ARG_OFFSET,
	  (void *)offsetof(lastbind_info, timestamp_precision),
	  "( OLcfgAt:5.1 "
	  "NAME 'olcLastBindPrecision' "
	  "DESC 'Precision of authTimestamp attribute' "
	  "SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs lastbindocs[] = {
	{ "( OLcfgOc:5.1 "
	  "NAME 'olcLastBindConfig' "
	  "DESC 'Last Bind configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcLastBindPrecision ) )",
	  Cft_Overlay, lastbindcfg, NULL, NULL },
	{ NULL, 0, NULL }
};

static time_t
parse_time( char *atm )
{
	struct lutil_tm tm;
	struct lutil_timet tt;
	time_t ret = (time_t)-1;

	if ( lutil_parsetime( atm, &tm ) == 0) {
		lutil_tm2time( &tm, &tt );
		ret = tt.tt_sec;
	}
	return ret;
}

static int
lastbind_bind_response( Operation *op, SlapReply *rs )
{
	Modifications *mod = NULL;
	BackendInfo *bi = op->o_bd->bd_info;
	Entry *e;
	int rc;

	/* we're only interested if the bind was successful */
	if ( rs->sr_err != LDAP_SUCCESS )
		return SLAP_CB_CONTINUE;

	rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &e );
	op->o_bd->bd_info = bi;

	if ( rc != LDAP_SUCCESS ) {
		return SLAP_CB_CONTINUE;
	}

	{
		lastbind_info *lbi = (lastbind_info *) op->o_callback->sc_private;

		time_t now, bindtime = (time_t)-1;
		Attribute *a;
		Modifications *m;
		char nowstr[ LDAP_LUTIL_GENTIME_BUFSIZE ];
		struct berval timestamp;

		/* get the current time */
		now = slap_get_time();

		/* get authTimestamp attribute, if it exists */
		if ((a = attr_find( e->e_attrs, ad_authTimestamp)) != NULL) {
			bindtime = parse_time( a->a_nvals[0].bv_val );

			if (bindtime != (time_t)-1) {
				/* if the recorded bind time is within our precision, we're done
				 * it doesn't need to be updated (save a write for nothing) */
				if ((now - bindtime) < lbi->timestamp_precision) {
					goto done;
				}
			}
		}

		/* update the authTimestamp in the user's entry with the current time */
		timestamp.bv_val = nowstr;
		timestamp.bv_len = sizeof(nowstr);
		slap_timestamp( &now, &timestamp );

		m = ch_calloc( sizeof(Modifications), 1 );
		m->sml_op = LDAP_MOD_REPLACE;
		m->sml_flags = 0;
		m->sml_type = ad_authTimestamp->ad_cname;
		m->sml_desc = ad_authTimestamp;
		m->sml_numvals = 1;
		m->sml_values = ch_calloc( sizeof(struct berval), 2 );
		m->sml_nvalues = ch_calloc( sizeof(struct berval), 2 );

		ber_dupbv( &m->sml_values[0], &timestamp );
		ber_dupbv( &m->sml_nvalues[0], &timestamp );
		m->sml_next = mod;
		mod = m;
	}

done:
	be_entry_release_r( op, e );

	/* perform the update, if necessary */
	if ( mod ) {
		Operation op2 = *op;
		SlapReply r2 = { REP_RESULT };
		slap_callback cb = { NULL, slap_null_cb, NULL, NULL };

		/* This is a DSA-specific opattr, it never gets replicated. */
		op2.o_tag = LDAP_REQ_MODIFY;
		op2.o_callback = &cb;
		op2.orm_modlist = mod;
		op2.o_dn = op->o_bd->be_rootdn;
		op2.o_ndn = op->o_bd->be_rootndn;
		op2.o_dont_replicate = 1;
		rc = op->o_bd->be_modify( &op2, &r2 );
		slap_mods_free( mod, 1 );
	}

	op->o_bd->bd_info = bi;
	return SLAP_CB_CONTINUE;
}

static int
lastbind_bind( Operation *op, SlapReply *rs )
{
	slap_callback *cb;
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;

	/* setup a callback to intercept result of this bind operation
	 * and pass along the lastbind_info struct */
	cb = op->o_tmpcalloc( sizeof(slap_callback), 1, op->o_tmpmemctx );
	cb->sc_response = lastbind_bind_response;
	cb->sc_next = op->o_callback->sc_next;
	cb->sc_private = on->on_bi.bi_private;
	op->o_callback->sc_next = cb;

	return SLAP_CB_CONTINUE;
}

static int
lastbind_db_init(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;

	/* initialize private structure to store configuration */
	on->on_bi.bi_private = ch_calloc( 1, sizeof(lastbind_info) );

	return 0;
}

static int
lastbind_db_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	lastbind_info *lbi = (lastbind_info *) on->on_bi.bi_private;

	/* free private structure to store configuration */
	free( lbi );

	return 0;
}

static slap_overinst lastbind;

int lastbind_initialize()
{
	int i, code;

	/* register operational schema for this overlay (authTimestamp attribute) */
	for (i=0; lastBind_OpSchema[i].def; i++) {
		code = register_at( lastBind_OpSchema[i].def, lastBind_OpSchema[i].ad, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"lastbind_initialize: register_at failed\n", 0, 0, 0 );
			return code;
		}
	}

	ad_authTimestamp->ad_type->sat_flags |= SLAP_AT_MANAGEABLE;

	lastbind.on_bi.bi_type = "lastbind";
	lastbind.on_bi.bi_db_init = lastbind_db_init;
	lastbind.on_bi.bi_db_close = lastbind_db_close;
	lastbind.on_bi.bi_op_bind = lastbind_bind;

	/* register configuration directives */
	lastbind.on_bi.bi_cf_ocs = lastbindocs;
	code = config_register_schema( lastbindcfg, lastbindocs );
	if ( code ) return code;

	return overlay_register( &lastbind );
}

#if SLAPD_OVER_LASTBIND == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return lastbind_initialize();
}
#endif

#endif	/* defined(SLAPD_OVER_LASTBIND) */
