/* syncrepl.c -- Replication Engine which uses the LDAP Sync protocol */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
 * Portions Copyright 2003 by IBM Corporation.
 * Portions Copyright 2003-2008 by Howard Chu, Symas Corporation.
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

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "lutil_ldap.h"

#include "config.h"

#include "ldap_rq.h"

#ifdef ENABLE_REWRITE
#include "rewrite.h"
#define SUFFIXM_CTX	"<suffix massage>"
#endif

struct nonpresent_entry {
	struct berval *npe_name;
	struct berval *npe_nname;
	LDAP_LIST_ENTRY(nonpresent_entry) npe_link;
};

typedef struct cookie_state {
	ldap_pvt_thread_mutex_t	cs_mutex;
	struct berval *cs_vals;
	int *cs_sids;
	int	cs_num;
	int cs_age;
	int cs_ref;

	/* pending changes, not yet committed */
	ldap_pvt_thread_mutex_t	cs_pmutex;
	struct berval *cs_pvals;
	int *cs_psids;
	int	cs_pnum;
} cookie_state;

#define	SYNCDATA_DEFAULT	0	/* entries are plain LDAP entries */
#define	SYNCDATA_ACCESSLOG	1	/* entries are accesslog format */
#define	SYNCDATA_CHANGELOG	2	/* entries are changelog format */

#define	SYNCLOG_LOGGING		0	/* doing a log-based update */
#define	SYNCLOG_FALLBACK	1	/* doing a full refresh */

#define RETRYNUM_FOREVER	(-1)	/* retry forever */
#define RETRYNUM_TAIL		(-2)	/* end of retrynum array */
#define RETRYNUM_VALID(n)	((n) >= RETRYNUM_FOREVER)	/* valid retrynum */
#define RETRYNUM_FINITE(n)	((n) > RETRYNUM_FOREVER)	/* not forever */

typedef struct syncinfo_s {
	struct syncinfo_s	*si_next;
	BackendDB		*si_be;
	BackendDB		*si_wbe;
	struct re_s		*si_re;
	int			si_rid;
	char			si_ridtxt[ STRLENOF("rid=999") + 1 ];
	slap_bindconf		si_bindconf;
	struct berval		si_base;
	struct berval		si_logbase;
	struct berval		si_filterstr;
	Filter			*si_filter;
	struct berval		si_logfilterstr;
	struct berval		si_contextdn;
	int			si_scope;
	int			si_attrsonly;
	char			*si_anfile;
	AttributeName		*si_anlist;
	AttributeName		*si_exanlist;
	char 			**si_attrs;
	char			**si_exattrs;
	int			si_allattrs;
	int			si_allopattrs;
	int			si_schemachecking;
	int			si_type;	/* the active type */
	int			si_ctype;	/* the configured type */
	time_t			si_interval;
	time_t			*si_retryinterval;
	int			*si_retrynum_init;
	int			*si_retrynum;
	struct sync_cookie	si_syncCookie;
	cookie_state		*si_cookieState;
	int			si_cookieAge;
	int			si_manageDSAit;
	int			si_slimit;
	int			si_tlimit;
	int			si_refreshDelete;
	int			si_refreshPresent;
	int			si_refreshDone;
	int			si_syncdata;
	int			si_logstate;
	int			si_got;
	int			si_strict_refresh;	/* stop listening during fallback refresh */
	int			si_too_old;
	ber_int_t	si_msgid;
	Avlnode			*si_presentlist;
	LDAP			*si_ld;
	Connection		*si_conn;
	LDAP_LIST_HEAD(np, nonpresent_entry)	si_nonpresentlist;
#ifdef ENABLE_REWRITE
	struct rewrite_info *si_rewrite;
	struct berval	si_suffixm;
#endif
	ldap_pvt_thread_mutex_t	si_mutex;
} syncinfo_t;

static int syncuuid_cmp( const void *, const void * );
static int avl_presentlist_insert( syncinfo_t* si, struct berval *syncUUID );
static void syncrepl_del_nonpresent( Operation *, syncinfo_t *, BerVarray, struct sync_cookie *, int );
static int syncrepl_message_to_op(
					syncinfo_t *, Operation *, LDAPMessage * );
static int syncrepl_message_to_entry(
					syncinfo_t *, Operation *, LDAPMessage *,
					Modifications **, Entry **, int, struct berval* );
static int syncrepl_entry(
					syncinfo_t *, Operation*, Entry*,
					Modifications**,int, struct berval*,
					struct berval *cookieCSN );
static int syncrepl_updateCookie(
					syncinfo_t *, Operation *,
					struct sync_cookie * );
static struct berval * slap_uuidstr_from_normalized(
					struct berval *, struct berval *, void * );
static int syncrepl_add_glue_ancestors(
	Operation* op, Entry *e );

/* delta-mmr overlay handler */
static int syncrepl_op_modify( Operation *op, SlapReply *rs );

/* callback functions */
static int dn_callback( Operation *, SlapReply * );
static int nonpresent_callback( Operation *, SlapReply * );
static int null_callback( Operation *, SlapReply * );

static AttributeDescription *sync_descs[4];

/* delta-mmr */
static AttributeDescription *ad_reqMod, *ad_reqDN;

typedef struct logschema {
	struct berval ls_dn;
	struct berval ls_req;
	struct berval ls_mod;
	struct berval ls_newRdn;
	struct berval ls_delRdn;
	struct berval ls_newSup;
} logschema;

static logschema changelog_sc = {
	BER_BVC("targetDN"),
	BER_BVC("changeType"),
	BER_BVC("changes"),
	BER_BVC("newRDN"),
	BER_BVC("deleteOldRDN"),
	BER_BVC("newSuperior")
};

static logschema accesslog_sc = {
	BER_BVC("reqDN"),
	BER_BVC("reqType"),
	BER_BVC("reqMod"),
	BER_BVC("reqNewRDN"),
	BER_BVC("reqDeleteOldRDN"),
	BER_BVC("reqNewSuperior")
};

static const char *
syncrepl_state2str( int state )
{
	switch ( state ) {
	case LDAP_SYNC_PRESENT:
		return "PRESENT";

	case LDAP_SYNC_ADD:
		return "ADD";

	case LDAP_SYNC_MODIFY:
		return "MODIFY";

	case LDAP_SYNC_DELETE:
		return "DELETE";
	}

	return "UNKNOWN";
}

static slap_overinst syncrepl_ov;

static void
init_syncrepl(syncinfo_t *si)
{
	int i, j, k, l, n;
	char **attrs, **exattrs;

	if ( !syncrepl_ov.on_bi.bi_type ) {
		syncrepl_ov.on_bi.bi_type = "syncrepl";
		syncrepl_ov.on_bi.bi_op_modify = syncrepl_op_modify;
		overlay_register( &syncrepl_ov );
	}

	/* delta-MMR needs the overlay, nothing else does.
	 * This must happen before accesslog overlay is configured.
	 */
	if ( si->si_syncdata &&
		!overlay_is_inst( si->si_be, syncrepl_ov.on_bi.bi_type )) {
		overlay_config( si->si_be, syncrepl_ov.on_bi.bi_type, -1, NULL, NULL );
		if ( !ad_reqMod ) {
			const char *text;
			logschema *ls = &accesslog_sc;

			slap_bv2ad( &ls->ls_mod, &ad_reqMod, &text );
			slap_bv2ad( &ls->ls_dn, &ad_reqDN, &text );
		}
	}

	if ( !sync_descs[0] ) {
		sync_descs[0] = slap_schema.si_ad_objectClass;
		sync_descs[1] = slap_schema.si_ad_structuralObjectClass;
		sync_descs[2] = slap_schema.si_ad_entryCSN;
		sync_descs[3] = NULL;
	}

	if ( si->si_allattrs && si->si_allopattrs )
		attrs = NULL;
	else
		attrs = anlist2attrs( si->si_anlist );

	if ( attrs ) {
		if ( si->si_allattrs ) {
			i = 0;
			while ( attrs[i] ) {
				if ( !is_at_operational( at_find( attrs[i] ) ) ) {
					for ( j = i; attrs[j] != NULL; j++ ) {
						if ( j == i )
							ch_free( attrs[i] );
						attrs[j] = attrs[j+1];
					}
				} else {
					i++;
				}
			}
			attrs = ( char ** ) ch_realloc( attrs, (i + 2)*sizeof( char * ) );
			attrs[i] = ch_strdup("*");
			attrs[i + 1] = NULL;

		} else if ( si->si_allopattrs ) {
			i = 0;
			while ( attrs[i] ) {
				if ( is_at_operational( at_find( attrs[i] ) ) ) {
					for ( j = i; attrs[j] != NULL; j++ ) {
						if ( j == i )
							ch_free( attrs[i] );
						attrs[j] = attrs[j+1];
					}
				} else {
					i++;
				}
			}
			attrs = ( char ** ) ch_realloc( attrs, (i + 2)*sizeof( char * ) );
			attrs[i] = ch_strdup("+");
			attrs[i + 1] = NULL;
		}

		for ( i = 0; sync_descs[i] != NULL; i++ ) {
			j = 0;
			while ( attrs[j] ) {
				if ( !strcmp( attrs[j], sync_descs[i]->ad_cname.bv_val ) ) {
					for ( k = j; attrs[k] != NULL; k++ ) {
						if ( k == j )
							ch_free( attrs[k] );
						attrs[k] = attrs[k+1];
					}
				} else {
					j++;
				}
			}
		}

		for ( n = 0; attrs[ n ] != NULL; n++ ) /* empty */;

		if ( si->si_allopattrs ) {
			attrs = ( char ** ) ch_realloc( attrs, (n + 2)*sizeof( char * ) );
		} else {
			attrs = ( char ** ) ch_realloc( attrs, (n + 4)*sizeof( char * ) );
		}

		/* Add Attributes */
		if ( si->si_allopattrs ) {
			attrs[n++] = ch_strdup( sync_descs[0]->ad_cname.bv_val );
		} else {
			for ( i = 0; sync_descs[ i ] != NULL; i++ ) {
				attrs[ n++ ] = ch_strdup ( sync_descs[i]->ad_cname.bv_val );
			}
		}
		attrs[ n ] = NULL;

	} else {

		i = 0;
		if ( si->si_allattrs == si->si_allopattrs ) {
			attrs = (char**) ch_malloc( 3 * sizeof(char*) );
			attrs[i++] = ch_strdup( "*" );
			attrs[i++] = ch_strdup( "+" );
		} else if ( si->si_allattrs && !si->si_allopattrs ) {
			for ( n = 0; sync_descs[ n ] != NULL; n++ ) ;
			attrs = (char**) ch_malloc( (n+1)* sizeof(char*) );
			attrs[i++] = ch_strdup( "*" );
			for ( j = 1; sync_descs[ j ] != NULL; j++ ) {
				attrs[i++] = ch_strdup ( sync_descs[j]->ad_cname.bv_val );
			}
		} else if ( !si->si_allattrs && si->si_allopattrs ) {
			attrs = (char**) ch_malloc( 3 * sizeof(char*) );
			attrs[i++] = ch_strdup( "+" );
			attrs[i++] = ch_strdup( sync_descs[0]->ad_cname.bv_val );
		}
		attrs[i] = NULL;
	}
	
	si->si_attrs = attrs;

	exattrs = anlist2attrs( si->si_exanlist );

	if ( exattrs ) {
		for ( n = 0; exattrs[n] != NULL; n++ ) ;

		for ( i = 0; sync_descs[i] != NULL; i++ ) {
			j = 0;
			while ( exattrs[j] != NULL ) {
				if ( !strcmp( exattrs[j], sync_descs[i]->ad_cname.bv_val ) ) {
					ch_free( exattrs[j] );
					for ( k = j; exattrs[k] != NULL; k++ ) {
						exattrs[k] = exattrs[k+1];
					}
				} else {
					j++;
				}
			}
		}

		for ( i = 0; exattrs[i] != NULL; i++ ) {
			for ( j = 0; si->si_anlist[j].an_name.bv_val; j++ ) {
				ObjectClass	*oc;
				if ( ( oc = si->si_anlist[j].an_oc ) ) {
					k = 0;
					while ( oc->soc_required[k] ) {
						if ( !strcmp( exattrs[i],
							 oc->soc_required[k]->sat_cname.bv_val ) ) {
							ch_free( exattrs[i] );
							for ( l = i; exattrs[l]; l++ ) {
								exattrs[l] = exattrs[l+1];
							}
						} else {
							k++;
						}
					}
				}
			}
		}

		for ( i = 0; exattrs[i] != NULL; i++ ) ;

		if ( i != n )
			exattrs = (char **) ch_realloc( exattrs, (i + 1)*sizeof(char *) );
	}

	si->si_exattrs = exattrs;	
}

static int
ldap_sync_search(
	syncinfo_t *si,
	void *ctx )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	LDAPControl c[3], *ctrls[4];
	int rc;
	int rhint;
	char *base;
	char **attrs, *lattrs[8];
	char *filter;
	int attrsonly;
	int scope;

	/* setup LDAP SYNC control */
	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &ctx );

	/* If we're using a log but we have no state, then fallback to
	 * normal mode for a full refresh.
	 */
	if ( si->si_syncdata && !si->si_syncCookie.numcsns ) {
		si->si_logstate = SYNCLOG_FALLBACK;
	}

	/* Use the log parameters if we're in log mode */
	if ( si->si_syncdata && si->si_logstate == SYNCLOG_LOGGING ) {
		logschema *ls;
		if ( si->si_syncdata == SYNCDATA_ACCESSLOG )
			ls = &accesslog_sc;
		else
			ls = &changelog_sc;
		lattrs[0] = ls->ls_dn.bv_val;
		lattrs[1] = ls->ls_req.bv_val;
		lattrs[2] = ls->ls_mod.bv_val;
		lattrs[3] = ls->ls_newRdn.bv_val;
		lattrs[4] = ls->ls_delRdn.bv_val;
		lattrs[5] = ls->ls_newSup.bv_val;
		lattrs[6] = slap_schema.si_ad_entryCSN->ad_cname.bv_val;
		lattrs[7] = NULL;

		rhint = 0;
		base = si->si_logbase.bv_val;
		filter = si->si_logfilterstr.bv_val;
		attrs = lattrs;
		attrsonly = 0;
		scope = LDAP_SCOPE_SUBTREE;
	} else {
		rhint = 1;
		base = si->si_base.bv_val;
		filter = si->si_filterstr.bv_val;
		attrs = si->si_attrs;
		attrsonly = si->si_attrsonly;
		scope = si->si_scope;
	}
	if ( si->si_syncdata && si->si_logstate == SYNCLOG_FALLBACK ) {
		si->si_type = LDAP_SYNC_REFRESH_ONLY;
	} else {
		si->si_type = si->si_ctype;
	}

	if ( !BER_BVISNULL( &si->si_syncCookie.octet_str ) )
	{
		ber_printf( ber, "{eOb}",
			abs(si->si_type), &si->si_syncCookie.octet_str, rhint );
	} else {
		ber_printf( ber, "{eb}",
			abs(si->si_type), rhint );
	}

	if ( (rc = ber_flatten2( ber, &c[0].ldctl_value, 0 ) ) == -1 ) {
		ber_free_buf( ber );
		return rc;
	}

	c[0].ldctl_oid = LDAP_CONTROL_SYNC;
	c[0].ldctl_iscritical = si->si_type < 0;
	ctrls[0] = &c[0];

	c[1].ldctl_oid = LDAP_CONTROL_MANAGEDSAIT;
	BER_BVZERO( &c[1].ldctl_value );
	c[1].ldctl_iscritical = 1;
	ctrls[1] = &c[1];

	if ( !BER_BVISNULL( &si->si_bindconf.sb_authzId ) ) {
		c[2].ldctl_oid = LDAP_CONTROL_PROXY_AUTHZ;
		c[2].ldctl_value = si->si_bindconf.sb_authzId;
		c[2].ldctl_iscritical = 1;
		ctrls[2] = &c[2];
		ctrls[3] = NULL;
	} else {
		ctrls[2] = NULL;
	}

	rc = ldap_search_ext( si->si_ld, base, scope, filter, attrs, attrsonly,
		ctrls, NULL, NULL, si->si_slimit, &si->si_msgid );
	ber_free_buf( ber );
	return rc;
}

static int
check_syncprov(
	Operation *op,
	syncinfo_t *si )
{
	AttributeName at[2];
	Attribute a = {0};
	Entry e = {0};
	SlapReply rs = {REP_SEARCH};
	int i, j, changed = 0;

	/* Look for contextCSN from syncprov overlay. If
	 * there's no overlay, this will be a no-op. That means
	 * this is a pure consumer, so local changes will not be
	 * allowed, and all changes will already be reflected in
	 * the cookieState.
	 */
	a.a_desc = slap_schema.si_ad_contextCSN;
	e.e_attrs = &a;
	e.e_name = si->si_contextdn;
	e.e_nname = si->si_contextdn;
	at[0].an_name = a.a_desc->ad_cname;
	at[0].an_desc = a.a_desc;
	BER_BVZERO( &at[1].an_name );
	rs.sr_entry = &e;
	rs.sr_flags = REP_ENTRY_MODIFIABLE;
	rs.sr_attrs = at;
	op->o_req_dn = e.e_name;
	op->o_req_ndn = e.e_nname;

	ldap_pvt_thread_mutex_lock( &si->si_cookieState->cs_mutex );
	i = backend_operational( op, &rs );
	if ( i == LDAP_SUCCESS && a.a_nvals ) {
		int num = a.a_numvals;
		/* check for differences */
		if ( num != si->si_cookieState->cs_num ) {
			changed = 1;
		} else {
			for ( i=0; i<num; i++ ) {
				if ( ber_bvcmp( &a.a_nvals[i],
					&si->si_cookieState->cs_vals[i] )) {
					changed = 1;
					break;
				}
			}
		}
		if ( changed ) {
			ber_bvarray_free( si->si_cookieState->cs_vals );
			ch_free( si->si_cookieState->cs_sids );
			si->si_cookieState->cs_num = num;
			si->si_cookieState->cs_vals = a.a_nvals;
			si->si_cookieState->cs_sids = slap_parse_csn_sids( a.a_nvals,
				num, NULL );
			si->si_cookieState->cs_age++;
		} else {
			ber_bvarray_free( a.a_nvals );
		}
		ber_bvarray_free( a.a_vals );
	}
	/* See if the cookieState has changed due to anything outside
	 * this particular consumer. That includes other consumers in
	 * the same context, or local changes detected above.
	 */
	if ( si->si_cookieState->cs_num > 0 && si->si_cookieAge !=
		si->si_cookieState->cs_age ) {
		if ( !si->si_syncCookie.numcsns ) {
			ber_bvarray_free( si->si_syncCookie.ctxcsn );
			ber_bvarray_dup_x( &si->si_syncCookie.ctxcsn,
				si->si_cookieState->cs_vals, NULL );
			changed = 1;
		} else {
			for (i=0; !BER_BVISNULL( &si->si_syncCookie.ctxcsn[i] ); i++) {
				/* bogus, just dup everything */
				if ( si->si_syncCookie.sids[i] == -1 ) {
					ber_bvarray_free( si->si_syncCookie.ctxcsn );
					ber_bvarray_dup_x( &si->si_syncCookie.ctxcsn,
						si->si_cookieState->cs_vals, NULL );
					changed = 1;
					break;
				}
				for (j=0; j<si->si_cookieState->cs_num; j++) {
					if ( si->si_syncCookie.sids[i] !=
						si->si_cookieState->cs_sids[j] )
						continue;
					if ( bvmatch( &si->si_syncCookie.ctxcsn[i],
						&si->si_cookieState->cs_vals[j] ))
						break;
					ber_bvreplace( &si->si_syncCookie.ctxcsn[i],
						&si->si_cookieState->cs_vals[j] );
					changed = 1;
					break;
				}
			}
		}
	}
	if ( changed ) {
		si->si_cookieAge = si->si_cookieState->cs_age;
		ch_free( si->si_syncCookie.octet_str.bv_val );
		slap_compose_sync_cookie( NULL, &si->si_syncCookie.octet_str,
			si->si_syncCookie.ctxcsn, si->si_syncCookie.rid,
			si->si_syncCookie.sid );
		ch_free( si->si_syncCookie.sids );
		slap_reparse_sync_cookie( &si->si_syncCookie, op->o_tmpmemctx );
	}
	ldap_pvt_thread_mutex_unlock( &si->si_cookieState->cs_mutex );
	return changed;
}

static int
do_syncrep1(
	Operation *op,
	syncinfo_t *si )
{
	int	rc;
	int cmdline_cookie_found = 0;

	struct sync_cookie	*sc = NULL;
#ifdef HAVE_TLS
	void	*ssl;
#endif

	rc = slap_client_connect( &si->si_ld, &si->si_bindconf );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}
	op->o_protocol = LDAP_VERSION3;

	/* Set SSF to strongest of TLS, SASL SSFs */
	op->o_sasl_ssf = 0;
	op->o_tls_ssf = 0;
	op->o_transport_ssf = 0;
#ifdef HAVE_TLS
	if ( ldap_get_option( si->si_ld, LDAP_OPT_X_TLS_SSL_CTX, &ssl )
		== LDAP_SUCCESS && ssl != NULL )
	{
		op->o_tls_ssf = ldap_pvt_tls_get_strength( ssl );
	}
#endif /* HAVE_TLS */
	{
		ber_len_t ssf; /* ITS#5403, 3864 LDAP_OPT_X_SASL_SSF probably ought
						  to use sasl_ssf_t but currently uses ber_len_t */
		if ( ldap_get_option( si->si_ld, LDAP_OPT_X_SASL_SSF, &ssf )
			== LDAP_SUCCESS )
			op->o_sasl_ssf = ssf;
	}
	op->o_ssf = ( op->o_sasl_ssf > op->o_tls_ssf )
		?  op->o_sasl_ssf : op->o_tls_ssf;

	ldap_set_option( si->si_ld, LDAP_OPT_TIMELIMIT, &si->si_tlimit );

	rc = LDAP_DEREF_NEVER;	/* actually could allow DEREF_FINDING */
	ldap_set_option( si->si_ld, LDAP_OPT_DEREF, &rc );

	ldap_set_option( si->si_ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF );

	si->si_syncCookie.rid = si->si_rid;

	/* whenever there are multiple data sources possible, advertise sid */
	si->si_syncCookie.sid = ( SLAP_MULTIMASTER( si->si_be ) || si->si_be != si->si_wbe ) ?
		slap_serverID : -1;

	/* We've just started up, or the remote server hasn't sent us
	 * any meaningful state.
	 */
	if ( !si->si_syncCookie.ctxcsn ) {
		int i;

		LDAP_STAILQ_FOREACH( sc, &slap_sync_cookie, sc_next ) {
			if ( si->si_rid == sc->rid ) {
				cmdline_cookie_found = 1;
				break;
			}
		}

		if ( cmdline_cookie_found ) {
			/* cookie is supplied in the command line */

			LDAP_STAILQ_REMOVE( &slap_sync_cookie, sc, sync_cookie, sc_next );

			slap_sync_cookie_free( &si->si_syncCookie, 0 );
			si->si_syncCookie.octet_str = sc->octet_str;
			ch_free( sc );
			/* ctxcsn wasn't parsed yet, do it now */
			slap_parse_sync_cookie( &si->si_syncCookie, NULL );
		} else {
			ldap_pvt_thread_mutex_lock( &si->si_cookieState->cs_mutex );
			if ( !si->si_cookieState->cs_num ) {
				/* get contextCSN shadow replica from database */
				BerVarray csn = NULL;
				void *ctx = op->o_tmpmemctx;

				op->o_req_ndn = si->si_contextdn;
				op->o_req_dn = op->o_req_ndn;

				/* try to read stored contextCSN */
				op->o_tmpmemctx = NULL;
				backend_attribute( op, NULL, &op->o_req_ndn,
					slap_schema.si_ad_contextCSN, &csn, ACL_READ );
				op->o_tmpmemctx = ctx;
				if ( csn ) {
					si->si_cookieState->cs_vals = csn;
					for (i=0; !BER_BVISNULL( &csn[i] ); i++);
					si->si_cookieState->cs_num = i;
					si->si_cookieState->cs_sids = slap_parse_csn_sids( csn, i, NULL );
					slap_sort_csn_sids( csn, si->si_cookieState->cs_sids, i, NULL );
				}
			}
			if ( si->si_cookieState->cs_num ) {
				ber_bvarray_free( si->si_syncCookie.ctxcsn );
				if ( ber_bvarray_dup_x( &si->si_syncCookie.ctxcsn,
					si->si_cookieState->cs_vals, NULL )) {
					rc = LDAP_NO_MEMORY;
					ldap_pvt_thread_mutex_unlock( &si->si_cookieState->cs_mutex );
					goto done;
				}
				si->si_syncCookie.numcsns = si->si_cookieState->cs_num;
				si->si_syncCookie.sids = ch_malloc( si->si_cookieState->cs_num *
					sizeof(int) );
				for ( i=0; i<si->si_syncCookie.numcsns; i++ )
					si->si_syncCookie.sids[i] = si->si_cookieState->cs_sids[i];
			}
			ldap_pvt_thread_mutex_unlock( &si->si_cookieState->cs_mutex );
		}

		slap_compose_sync_cookie( NULL, &si->si_syncCookie.octet_str,
			si->si_syncCookie.ctxcsn, si->si_syncCookie.rid,
			si->si_syncCookie.sid );
	} else {
		/* ITS#6367: recreate the cookie so it has our SID, not our peer's */
		ch_free( si->si_syncCookie.octet_str.bv_val );
		BER_BVZERO( &si->si_syncCookie.octet_str );
		/* Look for contextCSN from syncprov overlay. */
		check_syncprov( op, si );
		if ( BER_BVISNULL( &si->si_syncCookie.octet_str ))
			slap_compose_sync_cookie( NULL, &si->si_syncCookie.octet_str,
				si->si_syncCookie.ctxcsn, si->si_syncCookie.rid,
				si->si_syncCookie.sid );
	}

	si->si_refreshDone = 0;

	rc = ldap_sync_search( si, op->o_tmpmemctx );

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "do_syncrep1: %s "
			"ldap_search_ext: %s (%d)\n",
			si->si_ridtxt, ldap_err2string( rc ), rc );
	}

done:
	if ( rc ) {
		if ( si->si_ld ) {
			ldap_unbind_ext( si->si_ld, NULL, NULL );
			si->si_ld = NULL;
		}
	}

	return rc;
}

static int
compare_csns( struct sync_cookie *sc1, struct sync_cookie *sc2, int *which )
{
	int i, j, match = 0;
	const char *text;

	*which = 0;

	if ( sc1->numcsns < sc2->numcsns ) {
		*which = sc1->numcsns;
		return -1;
	}

	for (j=0; j<sc2->numcsns; j++) {
		for (i=0; i<sc1->numcsns; i++) {
			if ( sc1->sids[i] != sc2->sids[j] )
				continue;
			value_match( &match, slap_schema.si_ad_entryCSN,
				slap_schema.si_ad_entryCSN->ad_type->sat_ordering,
				SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
				&sc1->ctxcsn[i], &sc2->ctxcsn[j], &text );
			if ( match < 0 ) {
				*which = j;
				return match;
			}
			break;
		}
		if ( i == sc1->numcsns ) {
			/* sc2 has a sid sc1 lacks */
			*which = j;
			return -1;
		}
	}
	return match;
}

#define	SYNC_PAUSED	-3

static int
do_syncrep2(
	Operation *op,
	syncinfo_t *si )
{
	BerElementBuffer berbuf;
	BerElement	*ber = (BerElement *)&berbuf;

	LDAPMessage	*msg = NULL;

	struct sync_cookie	syncCookie = { NULL };
	struct sync_cookie	syncCookie_req = { NULL };

	int		rc,
			err = LDAP_SUCCESS;

	Modifications	*modlist = NULL;

	int				m;

	struct timeval *tout_p = NULL;
	struct timeval tout = { 0, 0 };

	int		refreshDeletes = 0;
	char empty[6] = "empty";

	if ( slapd_shutdown ) {
		rc = -2;
		goto done;
	}

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	Debug( LDAP_DEBUG_TRACE, "=>do_syncrep2 %s\n", si->si_ridtxt, 0, 0 );

	slap_dup_sync_cookie( &syncCookie_req, &si->si_syncCookie );

	if ( abs(si->si_type) == LDAP_SYNC_REFRESH_AND_PERSIST && si->si_refreshDone ) {
		tout_p = &tout;
	} else {
		tout_p = NULL;
	}

	while ( ( rc = ldap_result( si->si_ld, si->si_msgid, LDAP_MSG_ONE,
		tout_p, &msg ) ) > 0 )
	{
		int				match, punlock, syncstate;
		struct berval	*retdata, syncUUID[2], cookie = BER_BVNULL;
		char			*retoid;
		LDAPControl		**rctrls = NULL, *rctrlp = NULL;
		BerVarray		syncUUIDs;
		ber_len_t		len;
		ber_tag_t		si_tag;
		Entry			*entry;
		struct berval	bdn;

		if ( slapd_shutdown ) {
			rc = -2;
			goto done;
		}
		switch( ldap_msgtype( msg ) ) {
		case LDAP_RES_SEARCH_ENTRY:
			ldap_get_entry_controls( si->si_ld, msg, &rctrls );
			ldap_get_dn_ber( si->si_ld, msg, NULL, &bdn );
			if (!bdn.bv_len) {
				bdn.bv_val = empty;
				bdn.bv_len = sizeof(empty)-1;
			}
			/* we can't work without the control */
			if ( rctrls ) {
				LDAPControl **next = NULL;
				/* NOTE: make sure we use the right one;
				 * a better approach would be to run thru
				 * the whole list and take care of all */
				/* NOTE: since we issue the search request,
				 * we should know what controls to expect,
				 * and there should be none apart from the
				 * sync-related control */
				rctrlp = ldap_control_find( LDAP_CONTROL_SYNC_STATE, rctrls, &next );
				if ( next && ldap_control_find( LDAP_CONTROL_SYNC_STATE, next, NULL ) )
				{
					bdn.bv_val[bdn.bv_len] = '\0';
					Debug( LDAP_DEBUG_ANY, "do_syncrep2: %s "
						"got search entry with multiple "
						"Sync State control (%s)\n", si->si_ridtxt, bdn.bv_val, 0 );
					ldap_controls_free( rctrls );
					rc = -1;
					goto done;
				}
			}
			if ( rctrlp == NULL ) {
				bdn.bv_val[bdn.bv_len] = '\0';
				Debug( LDAP_DEBUG_ANY, "do_syncrep2: %s "
					"got search entry without "
					"Sync State control (%s)\n", si->si_ridtxt, bdn.bv_val, 0 );
				rc = -1;
				goto done;
			}
			ber_init2( ber, &rctrlp->ldctl_value, LBER_USE_DER );
			if ( ber_scanf( ber, "{em" /*"}"*/, &syncstate, &syncUUID[0] )
					== LBER_ERROR ) {
				bdn.bv_val[bdn.bv_len] = '\0';
				Debug( LDAP_DEBUG_ANY, "do_syncrep2: %s malformed message (%s)\n",
					si->si_ridtxt, bdn.bv_val, 0 );
				ldap_controls_free( rctrls );
				rc = -1;
				goto done;
			}
			/* FIXME: what if syncUUID is NULL or empty?
			 * (happens with back-sql...) */
			if ( BER_BVISEMPTY( &syncUUID[0] ) ) {
				bdn.bv_val[bdn.bv_len] = '\0';
				Debug( LDAP_DEBUG_ANY, "do_syncrep2: %s "
					"got empty syncUUID with LDAP_SYNC_%s (%s)\n",
					si->si_ridtxt,
					syncrepl_state2str( syncstate ), bdn.bv_val );
				ldap_controls_free( rctrls );
				rc = -1;
				goto done;
			}
			punlock = -1;
			if ( ber_peek_tag( ber, &len ) == LDAP_TAG_SYNC_COOKIE ) {
				ber_scanf( ber, /*"{"*/ "m}", &cookie );

				Debug( LDAP_DEBUG_SYNC, "do_syncrep2: %s cookie=%s\n",
					si->si_ridtxt,
					BER_BVISNULL( &cookie ) ? "" : cookie.bv_val, 0 );

				if ( !BER_BVISNULL( &cookie ) ) {
					ch_free( syncCookie.octet_str.bv_val );
					ber_dupbv( &syncCookie.octet_str, &cookie );
				}
				if ( !BER_BVISNULL( &syncCookie.octet_str ) )
				{
					slap_parse_sync_cookie( &syncCookie, NULL );
					if ( syncCookie.ctxcsn ) {
						int i, sid = slap_parse_csn_sid( syncCookie.ctxcsn );
						check_syncprov( op, si );
						for ( i =0; i<si->si_cookieState->cs_num; i++ ) {
							/* new SID */
							if ( sid < si->si_cookieState->cs_sids[i] )
								break;
							if ( si->si_cookieState->cs_sids[i] == sid ) {
								if ( ber_bvcmp( syncCookie.ctxcsn, &si->si_cookieState->cs_vals[i] ) <= 0 ) {
									bdn.bv_val[bdn.bv_len] = '\0';
									Debug( LDAP_DEBUG_SYNC, "do_syncrep2: %s CSN too old, ignoring %s (%s)\n",
										si->si_ridtxt, syncCookie.ctxcsn->bv_val, bdn.bv_val );
									ldap_controls_free( rctrls );
									rc = 0;
									si->si_too_old = 1;
									goto done;
								}
								si->si_too_old = 0;
								break;
							}
						}
						/* check pending CSNs too */
						while ( ldap_pvt_thread_mutex_trylock( &si->si_cookieState->cs_pmutex )) {
							if ( slapd_shutdown ) {
								rc = -2;
								goto done;
							}
							if ( !ldap_pvt_thread_pool_pausecheck( &connection_pool ))
								ldap_pvt_thread_yield();
						}
						for ( i =0; i<si->si_cookieState->cs_pnum; i++ ) {
							if ( sid < si->si_cookieState->cs_psids[i] )
								break;
							if ( si->si_cookieState->cs_psids[i] == sid ) {
								if ( ber_bvcmp( syncCookie.ctxcsn, &si->si_cookieState->cs_pvals[i] ) <= 0 ) {
									bdn.bv_val[bdn.bv_len] = '\0';
									Debug( LDAP_DEBUG_SYNC, "do_syncrep2: %s CSN pending, ignoring %s (%s)\n",
										si->si_ridtxt, syncCookie.ctxcsn->bv_val, bdn.bv_val );
									ldap_controls_free( rctrls );
									rc = 0;
									ldap_pvt_thread_mutex_unlock( &si->si_cookieState->cs_pmutex );
									goto done;
								}
								ber_bvreplace( &si->si_cookieState->cs_pvals[i],
									syncCookie.ctxcsn );
								break;
							}
						}
						/* new SID, add it */
						if ( i == si->si_cookieState->cs_pnum ||
							sid != si->si_cookieState->cs_psids[i] ) {
							slap_insert_csn_sids(
								(struct sync_cookie *)&si->si_cookieState->cs_pvals,
								i, sid, syncCookie.ctxcsn );
						}
						assert( punlock < 0 );
						punlock = i;
					} else if (si->si_too_old) {
						bdn.bv_val[bdn.bv_len] = '\0';
						Debug( LDAP_DEBUG_SYNC, "do_syncrep2: %s CSN too old, ignoring (%s)\n",
							si->si_ridtxt, bdn.bv_val, 0 );
						ldap_controls_free( rctrls );
						rc = 0;
						goto done;
					}
					op->o_controls[slap_cids.sc_LDAPsync] = &syncCookie;
				}
			}
			rc = 0;
			if ( si->si_syncdata && si->si_logstate == SYNCLOG_LOGGING ) {
				modlist = NULL;
				if ( ( rc = syncrepl_message_to_op( si, op, msg ) ) == LDAP_SUCCESS &&
					syncCookie.ctxcsn )
				{
					rc = syncrepl_updateCookie( si, op, &syncCookie );
				} else switch ( rc ) {
					case LDAP_ALREADY_EXISTS:
					case LDAP_NO_SUCH_OBJECT:
					case LDAP_NO_SUCH_ATTRIBUTE:
					case LDAP_TYPE_OR_VALUE_EXISTS:
					case LDAP_NOT_ALLOWED_ON_NONLEAF:
						rc = LDAP_SYNC_REFRESH_REQUIRED;
						si->si_logstate = SYNCLOG_FALLBACK;
						ldap_abandon_ext( si->si_ld, si->si_msgid, NULL, NULL );
						bdn.bv_val[bdn.bv_len] = '\0';
						Debug( LDAP_DEBUG_SYNC, "do_syncrep2: %s delta-sync lost sync on (%s), switching to REFRESH\n",
							si->si_ridtxt, bdn.bv_val, 0 );
						if (si->si_strict_refresh) {
							slap_suspend_listeners();
							connections_drop();
						}
						break;
					default:
						break;
				}
			} else if ( ( rc = syncrepl_message_to_entry( si, op, msg,
				&modlist, &entry, syncstate, syncUUID ) ) == LDAP_SUCCESS )
			{
				if ( ( rc = syncrepl_entry( si, op, entry, &modlist,
					syncstate, syncUUID, syncCookie.ctxcsn ) ) == LDAP_SUCCESS &&
					syncCookie.ctxcsn )
				{
					rc = syncrepl_updateCookie( si, op, &syncCookie );
				}
			}
			if ( punlock >= 0 ) {
				/* on failure, revert pending CSN */
				if ( rc != LDAP_SUCCESS ) {
					int i;
					for ( i = 0; i<si->si_cookieState->cs_num; i++ ) {
						if ( si->si_cookieState->cs_sids[i] == si->si_cookieState->cs_psids[punlock] ) {
							ber_bvreplace( &si->si_cookieState->cs_pvals[punlock],
								&si->si_cookieState->cs_vals[i] );
							break;
						}
					}
					if ( i == si->si_cookieState->cs_num )
						si->si_cookieState->cs_pvals[punlock].bv_val[0] = '\0';
				}
				ldap_pvt_thread_mutex_unlock( &si->si_cookieState->cs_pmutex );
			}
			ldap_controls_free( rctrls );
			if ( modlist ) {
				slap_mods_free( modlist, 1 );
			}
			if ( rc )
				goto done;
			break;

		case LDAP_RES_SEARCH_REFERENCE:
			Debug( LDAP_DEBUG_ANY,
				"do_syncrep2: %s reference received error\n",
				si->si_ridtxt, 0, 0 );
			break;

		case LDAP_RES_SEARCH_RESULT:
			Debug( LDAP_DEBUG_SYNC,
				"do_syncrep2: %s LDAP_RES_SEARCH_RESULT\n",
				si->si_ridtxt, 0, 0 );
			err = LDAP_OTHER; /* FIXME check parse result properly */
			ldap_parse_result( si->si_ld, msg, &err, NULL, NULL, NULL,
				&rctrls, 0 );
#ifdef LDAP_X_SYNC_REFRESH_REQUIRED
			if ( err == LDAP_X_SYNC_REFRESH_REQUIRED ) {
				/* map old result code to registered code */
				err = LDAP_SYNC_REFRESH_REQUIRED;
			}
#endif
			if ( err == LDAP_SYNC_REFRESH_REQUIRED ) {
				if ( si->si_logstate == SYNCLOG_LOGGING ) {
					si->si_logstate = SYNCLOG_FALLBACK;
					Debug( LDAP_DEBUG_SYNC, "do_syncrep2: %s delta-sync lost sync, switching to REFRESH\n",
						si->si_ridtxt, 0, 0 );
					if (si->si_strict_refresh) {
						slap_suspend_listeners();
						connections_drop();
					}
				}
				rc = err;
				goto done;
			}
			if ( err ) {
				Debug( LDAP_DEBUG_ANY,
					"do_syncrep2: %s LDAP_RES_SEARCH_RESULT (%d) %s\n",
					si->si_ridtxt, err, ldap_err2string( err ) );
			}
			if ( rctrls ) {
				LDAPControl **next = NULL;
				/* NOTE: make sure we use the right one;
				 * a better approach would be to run thru
				 * the whole list and take care of all */
				/* NOTE: since we issue the search request,
				 * we should know what controls to expect,
				 * and there should be none apart from the
				 * sync-related control */
				rctrlp = ldap_control_find( LDAP_CONTROL_SYNC_DONE, rctrls, &next );
				if ( next && ldap_control_find( LDAP_CONTROL_SYNC_DONE, next, NULL ) )
				{
					Debug( LDAP_DEBUG_ANY, "do_syncrep2: %s "
						"got search result with multiple "
						"Sync State control\n", si->si_ridtxt, 0, 0 );
					ldap_controls_free( rctrls );
					rc = -1;
					goto done;
				}
			}
			if ( rctrlp ) {
				ber_init2( ber, &rctrlp->ldctl_value, LBER_USE_DER );

				ber_scanf( ber, "{" /*"}"*/);
				if ( ber_peek_tag( ber, &len ) == LDAP_TAG_SYNC_COOKIE ) {
					ber_scanf( ber, "m", &cookie );

					Debug( LDAP_DEBUG_SYNC, "do_syncrep2: %s cookie=%s\n",
						si->si_ridtxt, 
						BER_BVISNULL( &cookie ) ? "" : cookie.bv_val, 0 );

					if ( !BER_BVISNULL( &cookie ) ) {
						ch_free( syncCookie.octet_str.bv_val );
						ber_dupbv( &syncCookie.octet_str, &cookie);
					}
					if ( !BER_BVISNULL( &syncCookie.octet_str ) )
					{
						slap_parse_sync_cookie( &syncCookie, NULL );
						op->o_controls[slap_cids.sc_LDAPsync] = &syncCookie;
					}
				}
				if ( ber_peek_tag( ber, &len ) == LDAP_TAG_REFRESHDELETES )
				{
					ber_scanf( ber, "b", &refreshDeletes );
				}
				ber_scanf( ber, /*"{"*/ "}" );
			}
			if ( SLAP_MULTIMASTER( op->o_bd ) && check_syncprov( op, si )) {
				slap_sync_cookie_free( &syncCookie_req, 0 );
				slap_dup_sync_cookie( &syncCookie_req, &si->si_syncCookie );
			}
			if ( !syncCookie.ctxcsn ) {
				match = 1;
			} else if ( !syncCookie_req.ctxcsn ) {
				match = -1;
				m = 0;
			} else {
				match = compare_csns( &syncCookie_req, &syncCookie, &m );
			}
			if ( rctrls ) {
				ldap_controls_free( rctrls );
			}
			if (si->si_type != LDAP_SYNC_REFRESH_AND_PERSIST) {
				/* FIXME : different error behaviors according to
				 *	1) err code : LDAP_BUSY ...
				 *	2) on err policy : stop service, stop sync, retry
				 */
				if ( refreshDeletes == 0 && match < 0 &&
					err == LDAP_SUCCESS &&
					syncCookie_req.numcsns == syncCookie.numcsns )
				{
					syncrepl_del_nonpresent( op, si, NULL,
						&syncCookie, m );
				} else {
					avl_free( si->si_presentlist, ch_free );
					si->si_presentlist = NULL;
				}
			}
			if ( syncCookie.ctxcsn && match < 0 && err == LDAP_SUCCESS )
			{
				rc = syncrepl_updateCookie( si, op, &syncCookie );
			}
			if ( err == LDAP_SUCCESS
				&& si->si_logstate == SYNCLOG_FALLBACK ) {
				si->si_logstate = SYNCLOG_LOGGING;
				rc = LDAP_SYNC_REFRESH_REQUIRED;
				slap_resume_listeners();
			} else {
				rc = -2;
			}
			goto done;

		case LDAP_RES_INTERMEDIATE:
			retoid = NULL;
			retdata = NULL;
			rc = ldap_parse_intermediate( si->si_ld, msg,
				&retoid, &retdata, NULL, 0 );
			if ( !rc && !strcmp( retoid, LDAP_SYNC_INFO ) ) {
				ber_init2( ber, retdata, LBER_USE_DER );

				switch ( si_tag = ber_peek_tag( ber, &len ) ) {
				ber_tag_t tag;
				case LDAP_TAG_SYNC_NEW_COOKIE:
					Debug( LDAP_DEBUG_SYNC,
						"do_syncrep2: %s %s - %s\n", 
						si->si_ridtxt,
						"LDAP_RES_INTERMEDIATE", 
						"NEW_COOKIE" );
					ber_scanf( ber, "tm", &tag, &cookie );
					Debug( LDAP_DEBUG_SYNC,
						"do_syncrep2: %s NEW_COOKIE: %s\n",
						si->si_ridtxt,
						cookie.bv_val, 0);
					if ( !BER_BVISNULL( &cookie ) ) {
						ch_free( syncCookie.octet_str.bv_val );
						ber_dupbv( &syncCookie.octet_str, &cookie );
					}
					if (!BER_BVISNULL( &syncCookie.octet_str ) ) {
						slap_parse_sync_cookie( &syncCookie, NULL );
						op->o_controls[slap_cids.sc_LDAPsync] = &syncCookie;
					}
					break;
				case LDAP_TAG_SYNC_REFRESH_DELETE:
				case LDAP_TAG_SYNC_REFRESH_PRESENT:
					Debug( LDAP_DEBUG_SYNC,
						"do_syncrep2: %s %s - %s\n", 
						si->si_ridtxt,
						"LDAP_RES_INTERMEDIATE", 
						si_tag == LDAP_TAG_SYNC_REFRESH_PRESENT ?
						"REFRESH_PRESENT" : "REFRESH_DELETE" );
					if ( si_tag == LDAP_TAG_SYNC_REFRESH_DELETE ) {
						si->si_refreshDelete = 1;
					} else {
						si->si_refreshPresent = 1;
					}
					ber_scanf( ber, "t{" /*"}"*/, &tag );
					if ( ber_peek_tag( ber, &len ) == LDAP_TAG_SYNC_COOKIE )
					{
						ber_scanf( ber, "m", &cookie );

						Debug( LDAP_DEBUG_SYNC, "do_syncrep2: %s cookie=%s\n",
							si->si_ridtxt, 
							BER_BVISNULL( &cookie ) ? "" : cookie.bv_val, 0 );

						if ( !BER_BVISNULL( &cookie ) ) {
							ch_free( syncCookie.octet_str.bv_val );
							ber_dupbv( &syncCookie.octet_str, &cookie );
						}
						if ( !BER_BVISNULL( &syncCookie.octet_str ) )
						{
							slap_parse_sync_cookie( &syncCookie, NULL );
							op->o_controls[slap_cids.sc_LDAPsync] = &syncCookie;
						}
					}
					/* Defaults to TRUE */
					if ( ber_peek_tag( ber, &len ) ==
						LDAP_TAG_REFRESHDONE )
					{
						ber_scanf( ber, "b", &si->si_refreshDone );
					} else
					{
						si->si_refreshDone = 1;
					}
					ber_scanf( ber, /*"{"*/ "}" );
					if ( abs(si->si_type) == LDAP_SYNC_REFRESH_AND_PERSIST &&
						si->si_refreshDone )
						tout_p = &tout;
					break;
				case LDAP_TAG_SYNC_ID_SET:
					Debug( LDAP_DEBUG_SYNC,
						"do_syncrep2: %s %s - %s\n", 
						si->si_ridtxt,
						"LDAP_RES_INTERMEDIATE", 
						"SYNC_ID_SET" );
					ber_scanf( ber, "t{" /*"}"*/, &tag );
					if ( ber_peek_tag( ber, &len ) ==
						LDAP_TAG_SYNC_COOKIE )
					{
						ber_scanf( ber, "m", &cookie );

						Debug( LDAP_DEBUG_SYNC, "do_syncrep2: %s cookie=%s\n",
							si->si_ridtxt,
							BER_BVISNULL( &cookie ) ? "" : cookie.bv_val, 0 );

						if ( !BER_BVISNULL( &cookie ) ) {
							ch_free( syncCookie.octet_str.bv_val );
							ber_dupbv( &syncCookie.octet_str, &cookie );
						}
						if ( !BER_BVISNULL( &syncCookie.octet_str ) )
						{
							slap_parse_sync_cookie( &syncCookie, NULL );
							op->o_controls[slap_cids.sc_LDAPsync] = &syncCookie;
							compare_csns( &syncCookie_req, &syncCookie, &m );
						}
					}
					if ( ber_peek_tag( ber, &len ) ==
						LDAP_TAG_REFRESHDELETES )
					{
						ber_scanf( ber, "b", &refreshDeletes );
					}
					syncUUIDs = NULL;
					ber_scanf( ber, "[W]", &syncUUIDs );
					ber_scanf( ber, /*"{"*/ "}" );
					if ( refreshDeletes ) {
						syncrepl_del_nonpresent( op, si, syncUUIDs,
							&syncCookie, m );
						ber_bvarray_free_x( syncUUIDs, op->o_tmpmemctx );
					} else {
						int i;
						for ( i = 0; !BER_BVISNULL( &syncUUIDs[i] ); i++ ) {
							(void)avl_presentlist_insert( si, &syncUUIDs[i] );
							slap_sl_free( syncUUIDs[i].bv_val, op->o_tmpmemctx );
						}
						slap_sl_free( syncUUIDs, op->o_tmpmemctx );
					}
					slap_sync_cookie_free( &syncCookie, 0 );
					break;
				default:
					Debug( LDAP_DEBUG_ANY,
						"do_syncrep2: %s unknown syncinfo tag (%ld)\n",
						si->si_ridtxt, (long) si_tag, 0 );
					ldap_memfree( retoid );
					ber_bvfree( retdata );
					continue;
				}

				if ( SLAP_MULTIMASTER( op->o_bd ) && check_syncprov( op, si )) {
					slap_sync_cookie_free( &syncCookie_req, 0 );
					slap_dup_sync_cookie( &syncCookie_req, &si->si_syncCookie );
				}
				if ( !syncCookie.ctxcsn ) {
					match = 1;
				} else if ( !syncCookie_req.ctxcsn ) {
					match = -1;
					m = 0;
				} else {
					match = compare_csns( &syncCookie_req, &syncCookie, &m );
				}

				if ( match < 0 ) {
					if ( si->si_refreshPresent == 1 &&
						si_tag != LDAP_TAG_SYNC_NEW_COOKIE &&
						syncCookie_req.numcsns == syncCookie.numcsns ) {
						syncrepl_del_nonpresent( op, si, NULL,
							&syncCookie, m );
					}

					if ( syncCookie.ctxcsn )
					{
						rc = syncrepl_updateCookie( si, op, &syncCookie);
					}
				} 

				ldap_memfree( retoid );
				ber_bvfree( retdata );

				if ( rc )
					goto done;

			} else {
				Debug( LDAP_DEBUG_ANY, "do_syncrep2: %s "
					"unknown intermediate response (%d)\n",
					si->si_ridtxt, rc, 0 );
				ldap_memfree( retoid );
				ber_bvfree( retdata );
			}
			break;

		default:
			Debug( LDAP_DEBUG_ANY, "do_syncrep2: %s "
				"unknown message (0x%02lx)\n",
				si->si_ridtxt,
				(unsigned long)ldap_msgtype( msg ), 0 );
			break;

		}
		if ( !BER_BVISNULL( &syncCookie.octet_str ) ) {
			slap_sync_cookie_free( &syncCookie_req, 0 );
			syncCookie_req = syncCookie;
			memset( &syncCookie, 0, sizeof( syncCookie ));
		}
		ldap_msgfree( msg );
		msg = NULL;
		if ( ldap_pvt_thread_pool_pausing( &connection_pool )) {
			slap_sync_cookie_free( &syncCookie, 0 );
			slap_sync_cookie_free( &syncCookie_req, 0 );
			return SYNC_PAUSED;
		}
	}

	if ( rc == -1 ) {
		rc = LDAP_OTHER;
		ldap_get_option( si->si_ld, LDAP_OPT_ERROR_NUMBER, &rc );
		err = rc;
	}

done:
	if ( err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"do_syncrep2: %s (%d) %s\n",
			si->si_ridtxt, err, ldap_err2string( err ) );
	}

	slap_sync_cookie_free( &syncCookie, 0 );
	slap_sync_cookie_free( &syncCookie_req, 0 );

	if ( msg ) ldap_msgfree( msg );

	if ( rc && rc != LDAP_SYNC_REFRESH_REQUIRED && si->si_ld ) {
		if ( si->si_conn ) {
			connection_client_stop( si->si_conn );
			si->si_conn = NULL;
		}
		ldap_unbind_ext( si->si_ld, NULL, NULL );
		si->si_ld = NULL;
	}

	return rc;
}

static void *
do_syncrepl(
	void	*ctx,
	void	*arg )
{
	struct re_s* rtask = arg;
	syncinfo_t *si = ( syncinfo_t * ) rtask->arg;
	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;
	int rc = LDAP_SUCCESS;
	int dostop = 0;
	ber_socket_t s;
	int i, defer = 1, fail = 0, freeinfo = 0;
	Backend *be;

	if ( si == NULL )
		return NULL;
	if ( slapd_shutdown )
		return NULL;

	Debug( LDAP_DEBUG_TRACE, "=>do_syncrepl %s\n", si->si_ridtxt, 0, 0 );

	/* Don't get stuck here while a pause is initiated */
	while ( ldap_pvt_thread_mutex_trylock( &si->si_mutex )) {
		if ( slapd_shutdown )
			return NULL;
		if ( !ldap_pvt_thread_pool_pausecheck( &connection_pool ))
			ldap_pvt_thread_yield();
	}

	si->si_too_old = 0;

	if ( si->si_ctype < 1 ) {
		goto deleted;
	}

	switch( abs( si->si_type ) ) {
	case LDAP_SYNC_REFRESH_ONLY:
	case LDAP_SYNC_REFRESH_AND_PERSIST:
		break;
	default:
		ldap_pvt_thread_mutex_unlock( &si->si_mutex );
		return NULL;
	}

	if ( slapd_shutdown ) {
		if ( si->si_ld ) {
			if ( si->si_conn ) {
				connection_client_stop( si->si_conn );
				si->si_conn = NULL;
			}
			ldap_unbind_ext( si->si_ld, NULL, NULL );
			si->si_ld = NULL;
		}
		ldap_pvt_thread_mutex_unlock( &si->si_mutex );
		return NULL;
	}

	connection_fake_init( &conn, &opbuf, ctx );
	op = &opbuf.ob_op;
	/* o_connids must be unique for slap_graduate_commit_csn */
	op->o_connid = SLAPD_SYNC_RID2SYNCCONN(si->si_rid);

	op->o_managedsait = SLAP_CONTROL_NONCRITICAL;
	be = si->si_be;

	/* Coordinate contextCSN updates with any syncprov overlays
	 * in use. This may be complicated by the use of the glue
	 * overlay.
	 *
	 * Typically there is a single syncprov mastering the entire
	 * glued tree. In that case, our contextCSN updates should
	 * go to the master DB. But if there is no syncprov on the
	 * master DB, then nothing special is needed here.
	 *
	 * Alternatively, there may be individual syncprov overlays
	 * on each glued branch. In that case, each syncprov only
	 * knows about changes within its own branch. And so our
	 * contextCSN updates should only go to the local DB.
	 */
	if ( !si->si_wbe ) {
		if ( SLAP_GLUE_SUBORDINATE( be ) && !overlay_is_inst( be, "syncprov" )) {
			BackendDB * top_be = select_backend( &be->be_nsuffix[0], 1 );
			if ( overlay_is_inst( top_be, "syncprov" ))
				si->si_wbe = top_be;
			else
				si->si_wbe = be;
		} else {
			si->si_wbe = be;
		}
		if ( SLAP_SYNC_SUBENTRY( si->si_wbe )) {
			build_new_dn( &si->si_contextdn, &si->si_wbe->be_nsuffix[0],
				(struct berval *)&slap_ldapsync_cn_bv, NULL );
		} else {
			si->si_contextdn = si->si_wbe->be_nsuffix[0];
		}
	}
	if ( !si->si_schemachecking )
		op->o_no_schema_check = 1;

	/* Establish session, do search */
	if ( !si->si_ld ) {
		si->si_refreshDelete = 0;
		si->si_refreshPresent = 0;

		if ( si->si_presentlist ) {
		    avl_free( si->si_presentlist, ch_free );
		    si->si_presentlist = NULL;
		}

		/* use main DB when retrieving contextCSN */
		op->o_bd = si->si_wbe;
		op->o_dn = op->o_bd->be_rootdn;
		op->o_ndn = op->o_bd->be_rootndn;
		rc = do_syncrep1( op, si );
	}

reload:
	/* Process results */
	if ( rc == LDAP_SUCCESS ) {
		ldap_get_option( si->si_ld, LDAP_OPT_DESC, &s );

		/* use current DB */
		op->o_bd = be;
		op->o_dn = op->o_bd->be_rootdn;
		op->o_ndn = op->o_bd->be_rootndn;
		rc = do_syncrep2( op, si );
		if ( rc == LDAP_SYNC_REFRESH_REQUIRED )	{
			rc = ldap_sync_search( si, op->o_tmpmemctx );
			goto reload;
		}

deleted:
		/* We got deleted while running on cn=config */
		if ( si->si_ctype < 1 ) {
			if ( si->si_ctype == -1 ) {
				si->si_ctype = 0;
				freeinfo = 1;
			}
			if ( si->si_conn )
				dostop = 1;
			rc = -1;
		}

		if ( rc != SYNC_PAUSED ) {
			if ( abs(si->si_type) == LDAP_SYNC_REFRESH_AND_PERSIST ) {
				/* If we succeeded, enable the connection for further listening.
				 * If we failed, tear down the connection and reschedule.
				 */
				if ( rc == LDAP_SUCCESS ) {
					if ( si->si_conn ) {
						connection_client_enable( si->si_conn );
					} else {
						si->si_conn = connection_client_setup( s, do_syncrepl, arg );
					} 
				} else if ( si->si_conn ) {
					dostop = 1;
				}
			} else {
				if ( rc == -2 ) rc = 0;
			}
		}
	}

	/* At this point, we have 5 cases:
	 * 1) for any hard failure, give up and remove this task
	 * 2) for ServerDown, reschedule this task to run later
	 * 3) for threadpool pause, reschedule to run immediately
	 * 4) for Refresh and Success, reschedule to run
	 * 5) for Persist and Success, reschedule to defer
	 */
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );

	if ( ldap_pvt_runqueue_isrunning( &slapd_rq, rtask ) ) {
		ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	}

	if ( dostop ) {
		connection_client_stop( si->si_conn );
		si->si_conn = NULL;
	}

	if ( rc == SYNC_PAUSED ) {
		rtask->interval.tv_sec = 0;
		ldap_pvt_runqueue_resched( &slapd_rq, rtask, 0 );
		rtask->interval.tv_sec = si->si_interval;
		rc = 0;
	} else if ( rc == LDAP_SUCCESS ) {
		if ( si->si_type == LDAP_SYNC_REFRESH_ONLY ) {
			defer = 0;
		}
		rtask->interval.tv_sec = si->si_interval;
		ldap_pvt_runqueue_resched( &slapd_rq, rtask, defer );
		if ( si->si_retrynum ) {
			for ( i = 0; si->si_retrynum_init[i] != RETRYNUM_TAIL; i++ ) {
				si->si_retrynum[i] = si->si_retrynum_init[i];
			}
			si->si_retrynum[i] = RETRYNUM_TAIL;
		}
	} else {
		for ( i = 0; si->si_retrynum && si->si_retrynum[i] <= 0; i++ ) {
			if ( si->si_retrynum[i] == RETRYNUM_FOREVER || si->si_retrynum[i] == RETRYNUM_TAIL )
				break;
		}

		if ( si->si_ctype < 1
			|| !si->si_retrynum || si->si_retrynum[i] == RETRYNUM_TAIL ) {
			if ( si->si_re ) {
				ldap_pvt_runqueue_remove( &slapd_rq, rtask );
				si->si_re = NULL;
			}
			fail = RETRYNUM_TAIL;
		} else if ( RETRYNUM_VALID( si->si_retrynum[i] ) ) {
			if ( si->si_retrynum[i] > 0 )
				si->si_retrynum[i]--;
			fail = si->si_retrynum[i];
			rtask->interval.tv_sec = si->si_retryinterval[i];
			ldap_pvt_runqueue_resched( &slapd_rq, rtask, 0 );
			slap_wake_listener();
		}
	}

	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	ldap_pvt_thread_mutex_unlock( &si->si_mutex );

	if ( rc ) {
		if ( fail == RETRYNUM_TAIL ) {
			Debug( LDAP_DEBUG_ANY,
				"do_syncrepl: %s rc %d quitting\n",
				si->si_ridtxt, rc, 0 );
		} else if ( fail > 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"do_syncrepl: %s rc %d retrying (%d retries left)\n",
				si->si_ridtxt, rc, fail );
		} else {
			Debug( LDAP_DEBUG_ANY,
				"do_syncrepl: %s rc %d retrying\n",
				si->si_ridtxt, rc, 0 );
		}
	}

	/* Do final delete cleanup */
	if ( freeinfo ) {
		syncinfo_free( si, 0 );
	}
	return NULL;
}

#ifdef ENABLE_REWRITE
static int
syncrepl_rewrite_dn(
	syncinfo_t *si,
	struct berval *dn,
	struct berval *sdn )
{
	char nul;
	int rc;

	nul = dn->bv_val[dn->bv_len];
	dn->bv_val[dn->bv_len] = 0;
	rc = rewrite( si->si_rewrite, SUFFIXM_CTX, dn->bv_val, &sdn->bv_val );
	dn->bv_val[dn->bv_len] = nul;

	if ( sdn->bv_val == dn->bv_val )
		sdn->bv_val = NULL;
	else if ( rc == REWRITE_REGEXEC_OK && sdn->bv_val )
		sdn->bv_len = strlen( sdn->bv_val );
	return rc;
}
#define	REWRITE_VAL(si, ad, bv, bv2)	\
	BER_BVZERO( &bv2 );	\
	if ( si->si_rewrite && ad->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName) \
		syncrepl_rewrite_dn( si, &bv, &bv2); \
	if ( BER_BVISNULL( &bv2 ))  \
		ber_dupbv( &bv2, &bv )
#define REWRITE_DN(si, bv, bv2, dn, ndn) \
	BER_BVZERO( &bv2 );	\
	if (si->si_rewrite) \
		syncrepl_rewrite_dn(si, &bv, &bv2); \
	rc = dnPrettyNormal( NULL, bv2.bv_val ? &bv2 : &bv, &dn, &ndn, op->o_tmpmemctx ); \
	ch_free(bv2.bv_val)
#else
#define REWRITE_VAL(si, ad, bv, bv2)	ber_dupbv(&bv2, &bv)
#define REWRITE_DN(si, bv, bv2, dn, ndn) \
	rc = dnPrettyNormal( NULL, &bv, &dn, &ndn, op->o_tmpmemctx )
#endif


static slap_verbmasks modops[] = {
	{ BER_BVC("add"), LDAP_REQ_ADD },
	{ BER_BVC("delete"), LDAP_REQ_DELETE },
	{ BER_BVC("modify"), LDAP_REQ_MODIFY },
	{ BER_BVC("modrdn"), LDAP_REQ_MODRDN},
	{ BER_BVNULL, 0 }
};

static int
syncrepl_accesslog_mods(
	syncinfo_t *si,
	struct berval *vals,
	struct Modifications **modres
)
{
	char *colon;
	const char *text;
	AttributeDescription *ad;
	struct berval bv, bv2;
	short op;
	Modifications *mod = NULL, *modlist = NULL, **modtail;
	int i, rc = 0;

	modtail = &modlist;

	for (i=0; !BER_BVISNULL( &vals[i] ); i++) {
		ad = NULL;
		bv = vals[i];

		colon = ber_bvchr( &bv, ':' );
		if ( !colon ) {
			/* Invalid */
			continue;
		}

		bv.bv_len = colon - bv.bv_val;
		if ( slap_bv2ad( &bv, &ad, &text ) ) {
			/* Invalid */
			Debug( LDAP_DEBUG_ANY, "syncrepl_accesslog_mods: %s "
				"Invalid attribute %s, %s\n",
				si->si_ridtxt, bv.bv_val, text );
			slap_mods_free( modlist, 1 );
			modlist = NULL;
			rc = -1;
			break;
		}

		/* Ignore dynamically generated attrs */
		if ( ad->ad_type->sat_flags & SLAP_AT_DYNAMIC ) {
			continue;
		}

		/* Ignore excluded attrs */
		if ( ldap_charray_inlist( si->si_exattrs,
			ad->ad_type->sat_cname.bv_val ) )
		{
			continue;
		}

		switch(colon[1]) {
		case '+':	op = LDAP_MOD_ADD; break;
		case '-':	op = LDAP_MOD_DELETE; break;
		case '=':	op = LDAP_MOD_REPLACE; break;
		case '#':	op = LDAP_MOD_INCREMENT; break;
		default:	continue;
		}

		if ( !mod || ad != mod->sml_desc || op != mod->sml_op ) {
			mod = (Modifications *) ch_malloc( sizeof( Modifications ) );
			mod->sml_flags = 0;
			mod->sml_op = op;
			mod->sml_next = NULL;
			mod->sml_desc = ad;
			mod->sml_type = ad->ad_cname;
			mod->sml_values = NULL;
			mod->sml_nvalues = NULL;
			mod->sml_numvals = 0;

			*modtail = mod;
			modtail = &mod->sml_next;
		}
		if ( colon[2] == ' ' ) {
			bv.bv_val = colon + 3;
			bv.bv_len = vals[i].bv_len - ( bv.bv_val - vals[i].bv_val );
			REWRITE_VAL( si, ad, bv, bv2 );
			ber_bvarray_add( &mod->sml_values, &bv2 );
			mod->sml_numvals++;
		}
	}
	*modres = modlist;
	return rc;
}

static int
syncrepl_changelog_mods(
	syncinfo_t *si,
	struct berval *vals,
	struct Modifications **modres
)
{
	return -1;	/* FIXME */
}

typedef struct OpExtraSync {
	OpExtra oe;
	syncinfo_t *oe_si;
} OpExtraSync;

/* Copy the original modlist, split Replace ops into Delete/Add,
 * and drop mod opattrs since this modification is in the past.
 */
static Modifications *mods_dup( Operation *op, Modifications *modlist, int match )
{
	Modifications *mod, *modnew = NULL, *modtail = NULL;
	int size;
	for ( ; modlist; modlist = modlist->sml_next ) {
		/* older ops */
		if ( match < 0 ) {
			if ( modlist->sml_desc == slap_schema.si_ad_modifiersName ||
				modlist->sml_desc == slap_schema.si_ad_modifyTimestamp ||
				modlist->sml_desc == slap_schema.si_ad_entryCSN )
				continue;
			if ( modlist->sml_op == LDAP_MOD_REPLACE ) {
				mod = op->o_tmpalloc( sizeof(Modifications), op->o_tmpmemctx );
				mod->sml_desc = modlist->sml_desc;
				mod->sml_values = NULL;
				mod->sml_nvalues = NULL;
				mod->sml_op = LDAP_MOD_DELETE;
				mod->sml_numvals = 0;
				mod->sml_flags = 0;
				if ( !modnew )
					modnew = mod;
				if ( modtail )
					modtail->sml_next = mod;
				modtail = mod;
			}
		}
		if ( modlist->sml_numvals ) {
			size = (modlist->sml_numvals+1) * sizeof(struct berval);
			if ( modlist->sml_nvalues ) size *= 2;
		} else {
			size = 0;
		}
		size += sizeof(Modifications);
		mod = op->o_tmpalloc( size, op->o_tmpmemctx );
		if ( !modnew )
			modnew = mod;
		if ( modtail )
			modtail->sml_next = mod;
		modtail = mod;
		mod->sml_desc = modlist->sml_desc;
		mod->sml_numvals = modlist->sml_numvals;
		mod->sml_flags = 0;
		if ( modlist->sml_numvals ) {
			int i;
			mod->sml_values = (BerVarray)(mod+1);
			for (i=0; i<mod->sml_numvals; i++)
				mod->sml_values[i] = modlist->sml_values[i];
			BER_BVZERO(&mod->sml_values[i]);
			if ( modlist->sml_nvalues ) {
				mod->sml_nvalues = mod->sml_values + mod->sml_numvals + 1;
				for (i=0; i<mod->sml_numvals; i++)
					mod->sml_nvalues[i] = modlist->sml_nvalues[i];
				BER_BVZERO(&mod->sml_nvalues[i]);
			} else {
				mod->sml_nvalues = NULL;
			}
		} else {
			mod->sml_values = NULL;
			mod->sml_nvalues = NULL;
		}
		if ( match < 0 && modlist->sml_op == LDAP_MOD_REPLACE )
			mod->sml_op = LDAP_MOD_ADD;
		else
			mod->sml_op = modlist->sml_op;
		mod->sml_next = NULL;
	}
	return modnew;
}

typedef struct resolve_ctxt {
	syncinfo_t *rx_si;
	Modifications *rx_mods;
} resolve_ctxt;

static void
compare_vals( Modifications *m1, Modifications *m2 )
{
	int i, j;
	struct berval *bv1, *bv2;

	if ( m2->sml_nvalues ) {
		bv2 = m2->sml_nvalues;
		bv1 = m1->sml_nvalues;
	} else {
		bv2 = m2->sml_values;
		bv1 = m1->sml_values;
	}
	for ( j=0; j<m2->sml_numvals; j++ ) {
		for ( i=0; i<m1->sml_numvals; i++ ) {
			if ( !ber_bvcmp( &bv1[i], &bv2[j] )) {
				int k;
				for ( k=i; k<m1->sml_numvals-1; k++ ) {
					m1->sml_values[k] = m1->sml_values[k+1];
					if ( m1->sml_nvalues )
						m1->sml_nvalues[k] = m1->sml_nvalues[k+1];
				}
				BER_BVZERO(&m1->sml_values[k]);
				if ( m1->sml_nvalues ) {
					BER_BVZERO(&m1->sml_nvalues[k]);
				}
				m1->sml_numvals--;
				i--;
			}
		}
	}
}

static int
syncrepl_resolve_cb( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		resolve_ctxt *rx = op->o_callback->sc_private;
		Attribute *a = attr_find( rs->sr_entry->e_attrs, ad_reqMod );
		if ( a ) {
			Modifications *oldmods, *newmods, *m1, *m2, **prev;
			oldmods = rx->rx_mods;
			syncrepl_accesslog_mods( rx->rx_si, a->a_vals, &newmods );
			for ( m2 = newmods; m2; m2=m2->sml_next ) {
				for ( prev = &oldmods, m1 = *prev; m1; m1 = *prev ) {
					if ( m1->sml_desc != m2->sml_desc ) {
						prev = &m1->sml_next;
						continue;
					}
					if ( m2->sml_op == LDAP_MOD_DELETE ||
						m2->sml_op == LDAP_MOD_REPLACE ) {
						int numvals = m2->sml_numvals;
						if ( m2->sml_op == LDAP_MOD_REPLACE )
							numvals = 0;
						/* New delete All cancels everything */
						if ( numvals == 0 ) {
drop:
							*prev = m1->sml_next;
							op->o_tmpfree( m1, op->o_tmpmemctx );
							continue;
						}
						if ( m1->sml_op == LDAP_MOD_DELETE ) {
							if ( m1->sml_numvals == 0 ) {
								/* turn this to SOFTDEL later */
								m1->sml_flags = SLAP_MOD_INTERNAL;
							} else {
								compare_vals( m1, m2 );
								if ( !m1->sml_numvals )
									goto drop;
							}
						} else if ( m1->sml_op == LDAP_MOD_ADD ) {
							compare_vals( m1, m2 );
							if ( !m1->sml_numvals )
								goto drop;
						}
					}

					if ( m2->sml_op == LDAP_MOD_ADD ||
						m2->sml_op == LDAP_MOD_REPLACE ) {
						if ( m1->sml_op == LDAP_MOD_DELETE ) {
							if ( !m1->sml_numvals ) goto drop;
							compare_vals( m1, m2 );
							if ( !m1->sml_numvals )
								goto drop;
						}
						if ( m2->sml_desc->ad_type->sat_atype.at_single_value )
							goto drop;
						compare_vals( m1, m2 );
						if ( !m1->sml_numvals )
							goto drop;
					}
					prev = &m1->sml_next;
				}
			}
			slap_mods_free( newmods, 1 );
			rx->rx_mods = oldmods;
		}
	}
	return LDAP_SUCCESS;
}

typedef struct modify_ctxt {
	Modifications *mx_orig;
	Modifications *mx_free;
} modify_ctxt;

static int
syncrepl_modify_cb( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;
	modify_ctxt *mx = sc->sc_private;
	Modifications *ml;

	op->orm_no_opattrs = 0;
	op->orm_modlist = mx->mx_orig;
	for ( ml = mx->mx_free; ml; ml = mx->mx_free ) {
		mx->mx_free = ml->sml_next;
		op->o_tmpfree( ml, op->o_tmpmemctx );
	}
	op->o_callback = sc->sc_next;
	op->o_tmpfree( sc, op->o_tmpmemctx );
	return SLAP_CB_CONTINUE;
}

static int
syncrepl_op_modify( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	OpExtra *oex;
	syncinfo_t *si;
	Entry *e;
	int rc, match = 0;
	Modifications *mod, *newlist;

	LDAP_SLIST_FOREACH( oex, &op->o_extra, oe_next ) {
		if ( oex->oe_key == (void *)syncrepl_message_to_op )
			break;
	}
	if ( !oex )
		return SLAP_CB_CONTINUE;

	si = ((OpExtraSync *)oex)->oe_si;

	/* Check if entryCSN in modlist is newer than entryCSN in entry.
	 * We do it here because the op has been serialized by accesslog
	 * by the time we get here. If the CSN is new enough, just do the
	 * mod. If not, we need to resolve conflicts.
	 */

	for ( mod = op->orm_modlist; mod; mod=mod->sml_next ) {
		if ( mod->sml_desc == slap_schema.si_ad_entryCSN ) break;
	}
	/* FIXME: what should we do if entryCSN is missing from the mod? */
	if ( !mod )
		return SLAP_CB_CONTINUE;

	rc = overlay_entry_get_ov( op, &op->o_req_ndn, NULL, NULL, 0, &e, on );
	if ( rc == 0 ) {
		Attribute *a;
		const char *text;
		a = attr_find( e->e_attrs, slap_schema.si_ad_entryCSN );
		value_match( &match, slap_schema.si_ad_entryCSN,
			slap_schema.si_ad_entryCSN->ad_type->sat_ordering,
			SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
			&mod->sml_nvalues[0], &a->a_nvals[0], &text );
		overlay_entry_release_ov( op, e, 0, on );
	}
	/* equal? Should never happen */
	if ( match == 0 )
		return LDAP_SUCCESS;

	/* mod is older: resolve conflicts...
	 * 1. Save/copy original modlist. Split Replace to Del/Add.
	 * 2. Find all mods to this reqDN newer than the mod stamp.
	 * 3. Resolve any mods in this request that affect attributes
	 *    touched by newer mods.
	 *    old         new
	 *    delete all  delete all  drop
	 *    delete all  delete X    SOFTDEL
	 *    delete X    delete all  drop
	 *    delete X    delete X    drop
	 *    delete X    delete Y    OK
	 *    delete all  add X       drop
	 *    delete X    add X       drop
	 *    delete X    add Y       OK
	 *    add X       delete all  drop
	 *    add X       delete X    drop
	 *    add X       add X       drop
	 *    add X       add Y       if SV, drop else OK
	 *
	 * 4. Swap original modlist back in response callback so
	 *    that accesslog logs the original mod.
	 *
	 * Even if the mod is newer, other out-of-order changes may
	 * have been committed, forcing us to tweak the modlist:
	 * 1. Save/copy original modlist.
	 * 2. Change deletes to soft deletes.
	 * 3. Change Adds of single-valued attrs to Replace.
	 */

	newlist = mods_dup( op, op->orm_modlist, match );

	/* mod is older */
	if ( match < 0 ) {
		Operation op2 = *op;
		AttributeName an[2];
		const char *text;
		struct berval bv;
		char *ptr;
		Modifications *ml;
		int size, rc;
		SlapReply rs1 = {0};
		resolve_ctxt rx;
		slap_callback cb = { NULL, syncrepl_resolve_cb, NULL, NULL };

		rx.rx_si = si;
		rx.rx_mods = newlist;
		cb.sc_private = &rx;

		op2.o_tag = LDAP_REQ_SEARCH;
		op2.ors_scope = LDAP_SCOPE_SUBTREE;
		op2.ors_deref = LDAP_DEREF_NEVER;
		op2.o_req_dn = si->si_logbase;
		op2.o_req_ndn = si->si_logbase;
		op2.ors_tlimit = SLAP_NO_LIMIT;
		op2.ors_slimit = SLAP_NO_LIMIT;
		op2.ors_limit = NULL;
		memset( an, 0, sizeof(an));
		an[0].an_desc = ad_reqMod;
		an[0].an_name = ad_reqMod->ad_cname;
		op2.ors_attrs = an;
		op2.ors_attrsonly = 0;

		bv = mod->sml_nvalues[0];

		size = sizeof("(&(entryCSN>=)(reqDN=))");
		size += bv.bv_len + op->o_req_ndn.bv_len + si->si_logfilterstr.bv_len;
		op2.ors_filterstr.bv_val = op->o_tmpalloc( size, op->o_tmpmemctx );
		op2.ors_filterstr.bv_len = sprintf(op2.ors_filterstr.bv_val,
			"(&(entryCSN>=%s)(reqDN=%s)%s)",
			bv.bv_val, op->o_req_ndn.bv_val, si->si_logfilterstr.bv_val );
		op2.ors_filter = str2filter_x( op, op2.ors_filterstr.bv_val );

		op2.o_callback = &cb;
		op2.o_bd = select_backend( &op2.o_req_ndn, 1 );
		op2.o_bd->be_search( &op2, &rs1 );
		newlist = rx.rx_mods;
	}

	{
		slap_callback *sc = op->o_tmpalloc( sizeof(slap_callback) +
			sizeof(modify_ctxt), op->o_tmpmemctx );
		modify_ctxt *mx = (modify_ctxt *)(sc+1);
		Modifications *ml;

		sc->sc_response = syncrepl_modify_cb;
		sc->sc_private = mx;
		sc->sc_next = op->o_callback;
		sc->sc_cleanup = NULL;
		op->o_callback = sc;
		op->orm_no_opattrs = 1;
		mx->mx_orig = op->orm_modlist;
		mx->mx_free = newlist;
		for ( ml = newlist; ml; ml=ml->sml_next ) {
			if ( ml->sml_flags == SLAP_MOD_INTERNAL ) {
				ml->sml_flags = 0;
				ml->sml_op = SLAP_MOD_SOFTDEL;
			}
			else if ( ml->sml_op == LDAP_MOD_DELETE )
				ml->sml_op = SLAP_MOD_SOFTDEL;
			else if ( ml->sml_op == LDAP_MOD_ADD &&
				ml->sml_desc->ad_type->sat_atype.at_single_value )
				ml->sml_op = LDAP_MOD_REPLACE;
		}
		op->orm_modlist = newlist;
		op->o_csn = mod->sml_nvalues[0];
	}
	return SLAP_CB_CONTINUE;
}

static int
syncrepl_message_to_op(
	syncinfo_t	*si,
	Operation	*op,
	LDAPMessage	*msg
)
{
	BerElement	*ber = NULL;
	Modifications	*modlist = NULL;
	logschema *ls;
	SlapReply rs = { REP_RESULT };
	slap_callback cb = { NULL, null_callback, NULL, NULL };

	const char	*text;
	char txtbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof txtbuf;

	struct berval	bdn, dn = BER_BVNULL, ndn;
	struct berval	bv, bv2, *bvals = NULL;
	struct berval	rdn = BER_BVNULL, sup = BER_BVNULL,
		prdn = BER_BVNULL, nrdn = BER_BVNULL,
		psup = BER_BVNULL, nsup = BER_BVNULL;
	int		rc, deleteOldRdn = 0, freeReqDn = 0;
	int		do_graduate = 0;

	if ( ldap_msgtype( msg ) != LDAP_RES_SEARCH_ENTRY ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_op: %s "
			"Message type should be entry (%d)",
			si->si_ridtxt, ldap_msgtype( msg ), 0 );
		return -1;
	}

	if ( si->si_syncdata == SYNCDATA_ACCESSLOG )
		ls = &accesslog_sc;
	else
		ls = &changelog_sc;

	rc = ldap_get_dn_ber( si->si_ld, msg, &ber, &bdn );

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"syncrepl_message_to_op: %s dn get failed (%d)",
			si->si_ridtxt, rc, 0 );
		return rc;
	}

	op->o_tag = LBER_DEFAULT;
	op->o_bd = si->si_wbe;

	if ( BER_BVISEMPTY( &bdn )) {
		Debug( LDAP_DEBUG_ANY,
			"syncrepl_message_to_op: %s got empty dn",
			si->si_ridtxt, 0, 0 );
		return LDAP_OTHER;
	}

	while (( rc = ldap_get_attribute_ber( si->si_ld, msg, ber, &bv, &bvals ) )
		== LDAP_SUCCESS ) {
		if ( bv.bv_val == NULL )
			break;

		if ( !ber_bvstrcasecmp( &bv, &ls->ls_dn ) ) {
			bdn = bvals[0];
			REWRITE_DN( si, bdn, bv2, dn, ndn );
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY,
					"syncrepl_message_to_op: %s "
					"dn \"%s\" normalization failed (%d)",
					si->si_ridtxt, bdn.bv_val, rc );
				rc = -1;
				ch_free( bvals );
				goto done;
			}
			ber_dupbv( &op->o_req_dn, &dn );
			ber_dupbv( &op->o_req_ndn, &ndn );
			slap_sl_free( ndn.bv_val, op->o_tmpmemctx );
			slap_sl_free( dn.bv_val, op->o_tmpmemctx );
			freeReqDn = 1;
		} else if ( !ber_bvstrcasecmp( &bv, &ls->ls_req ) ) {
			int i = verb_to_mask( bvals[0].bv_val, modops );
			if ( i < 0 ) {
				Debug( LDAP_DEBUG_ANY,
					"syncrepl_message_to_op: %s unknown op %s",
					si->si_ridtxt, bvals[0].bv_val, 0 );
				ch_free( bvals );
				rc = -1;
				goto done;
			}
			op->o_tag = modops[i].mask;
		} else if ( !ber_bvstrcasecmp( &bv, &ls->ls_mod ) ) {
			/* Parse attribute into modlist */
			if ( si->si_syncdata == SYNCDATA_ACCESSLOG ) {
				rc = syncrepl_accesslog_mods( si, bvals, &modlist );
			} else {
				rc = syncrepl_changelog_mods( si, bvals, &modlist );
			}
			if ( rc ) goto done;
		} else if ( !ber_bvstrcasecmp( &bv, &ls->ls_newRdn ) ) {
			rdn = bvals[0];
		} else if ( !ber_bvstrcasecmp( &bv, &ls->ls_delRdn ) ) {
			if ( !ber_bvstrcasecmp( &slap_true_bv, bvals ) ) {
				deleteOldRdn = 1;
			}
		} else if ( !ber_bvstrcasecmp( &bv, &ls->ls_newSup ) ) {
			sup = bvals[0];
		} else if ( !ber_bvstrcasecmp( &bv,
			&slap_schema.si_ad_entryCSN->ad_cname ) )
		{
			slap_queue_csn( op, bvals );
			do_graduate = 1;
		}
		ch_free( bvals );
	}

	/* If we didn't get a mod type or a target DN, bail out */
	if ( op->o_tag == LBER_DEFAULT || BER_BVISNULL( &dn ) ) {
		rc = -1;
		goto done;
	}

	op->o_callback = &cb;
	slap_op_time( &op->o_time, &op->o_tincr );

	switch( op->o_tag ) {
	case LDAP_REQ_ADD:
	case LDAP_REQ_MODIFY:
		/* If we didn't get required data, bail */
		if ( !modlist ) goto done;

		rc = slap_mods_check( op, modlist, &text, txtbuf, textlen, NULL );

		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_op: %s "
				"mods check (%s)\n",
				si->si_ridtxt, text, 0 );
			goto done;
		}

		if ( op->o_tag == LDAP_REQ_ADD ) {
			Entry *e = entry_alloc();
			op->ora_e = e;
			op->ora_e->e_name = op->o_req_dn;
			op->ora_e->e_nname = op->o_req_ndn;
			freeReqDn = 0;
			rc = slap_mods2entry( modlist, &op->ora_e, 1, 0, &text, txtbuf, textlen);
			if( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_op: %s "
				"mods2entry (%s)\n",
					si->si_ridtxt, text, 0 );
			} else {
				rc = op->o_bd->be_add( op, &rs );
				Debug( LDAP_DEBUG_SYNC,
					"syncrepl_message_to_op: %s be_add %s (%d)\n", 
					si->si_ridtxt, op->o_req_dn.bv_val, rc );
				do_graduate = 0;
			}
			if ( e == op->ora_e )
				be_entry_release_w( op, op->ora_e );
		} else {
			OpExtraSync oes;
			op->orm_modlist = modlist;
			op->o_bd = si->si_wbe;
			/* delta-mmr needs additional checks in syncrepl_op_modify */
			if ( SLAP_MULTIMASTER( op->o_bd )) {
				oes.oe.oe_key = (void *)syncrepl_message_to_op;
				oes.oe_si = si;
				LDAP_SLIST_INSERT_HEAD( &op->o_extra, &oes.oe, oe_next );
			}
			rc = op->o_bd->be_modify( op, &rs );
			if ( SLAP_MULTIMASTER( op->o_bd )) {
				LDAP_SLIST_REMOVE( &op->o_extra, &oes.oe, OpExtra, oe_next );
				BER_BVZERO( &op->o_csn );
			}
			modlist = op->orm_modlist;
			Debug( rc ? LDAP_DEBUG_ANY : LDAP_DEBUG_SYNC,
				"syncrepl_message_to_op: %s be_modify %s (%d)\n", 
				si->si_ridtxt, op->o_req_dn.bv_val, rc );
			op->o_bd = si->si_be;
			do_graduate = 0;
		}
		break;
	case LDAP_REQ_MODRDN:
		if ( BER_BVISNULL( &rdn ) ) goto done;

		if ( rdnPretty( NULL, &rdn, &prdn, NULL ) ) {
			goto done;
		}
		if ( rdnNormalize( 0, NULL, NULL, &rdn, &nrdn, NULL ) ) {
			goto done;
		}
		if ( !BER_BVISNULL( &sup ) ) {
			REWRITE_DN( si, sup, bv2, psup, nsup );
			if ( rc )
				goto done;
			op->orr_newSup = &psup;
			op->orr_nnewSup = &nsup;
		} else {
			op->orr_newSup = NULL;
			op->orr_nnewSup = NULL;
		}
		op->orr_newrdn = prdn;
		op->orr_nnewrdn = nrdn;
		op->orr_deleteoldrdn = deleteOldRdn;
		op->orr_modlist = NULL;
		if ( slap_modrdn2mods( op, &rs ) ) {
			goto done;
		}

		/* Append modlist for operational attrs */
		{
			Modifications *m;

			for ( m = op->orr_modlist; m->sml_next; m = m->sml_next )
				;
			m->sml_next = modlist;
			modlist = NULL;
		}
		rc = op->o_bd->be_modrdn( op, &rs );
		slap_mods_free( op->orr_modlist, 1 );
		Debug( rc ? LDAP_DEBUG_ANY : LDAP_DEBUG_SYNC,
			"syncrepl_message_to_op: %s be_modrdn %s (%d)\n", 
			si->si_ridtxt, op->o_req_dn.bv_val, rc );
		do_graduate = 0;
		break;
	case LDAP_REQ_DELETE:
		rc = op->o_bd->be_delete( op, &rs );
		Debug( rc ? LDAP_DEBUG_ANY : LDAP_DEBUG_SYNC,
			"syncrepl_message_to_op: %s be_delete %s (%d)\n", 
			si->si_ridtxt, op->o_req_dn.bv_val, rc );
		do_graduate = 0;
		break;
	}
done:
	if ( do_graduate )
		slap_graduate_commit_csn( op );
	op->o_bd = si->si_be;
	op->o_tmpfree( op->o_csn.bv_val, op->o_tmpmemctx );
	BER_BVZERO( &op->o_csn );
	if ( modlist ) {
		slap_mods_free( modlist, op->o_tag != LDAP_REQ_ADD );
	}
	if ( !BER_BVISNULL( &rdn ) ) {
		if ( !BER_BVISNULL( &nsup ) ) {
			ch_free( nsup.bv_val );
		}
		if ( !BER_BVISNULL( &psup ) ) {
			ch_free( psup.bv_val );
		}
		if ( !BER_BVISNULL( &nrdn ) ) {
			ch_free( nrdn.bv_val );
		}
		if ( !BER_BVISNULL( &prdn ) ) {
			ch_free( prdn.bv_val );
		}
	}
	if ( freeReqDn ) {
		ch_free( op->o_req_ndn.bv_val );
		ch_free( op->o_req_dn.bv_val );
	}
	ber_free( ber, 0 );
	return rc;
}

static int
syncrepl_message_to_entry(
	syncinfo_t	*si,
	Operation	*op,
	LDAPMessage	*msg,
	Modifications	**modlist,
	Entry			**entry,
	int		syncstate,
	struct berval	*syncUUID
)
{
	Entry		*e = NULL;
	BerElement	*ber = NULL;
	Modifications	tmp;
	Modifications	*mod;
	Modifications	**modtail = modlist;

	const char	*text;
	char txtbuf[SLAP_TEXT_BUFLEN];
	size_t textlen = sizeof txtbuf;

	struct berval	bdn = BER_BVNULL, dn, ndn, bv2;
	int		rc, is_ctx;

	*modlist = NULL;

	if ( ldap_msgtype( msg ) != LDAP_RES_SEARCH_ENTRY ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_entry: %s "
			"Message type should be entry (%d)",
			si->si_ridtxt, ldap_msgtype( msg ), 0 );
		return -1;
	}

	op->o_tag = LDAP_REQ_ADD;

	rc = ldap_get_dn_ber( si->si_ld, msg, &ber, &bdn );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"syncrepl_message_to_entry: %s dn get failed (%d)",
			si->si_ridtxt, rc, 0 );
		return rc;
	}

	if ( BER_BVISEMPTY( &bdn ) && !BER_BVISEMPTY( &op->o_bd->be_nsuffix[0] ) ) {
		Debug( LDAP_DEBUG_ANY,
			"syncrepl_message_to_entry: %s got empty dn",
			si->si_ridtxt, 0, 0 );
		return LDAP_OTHER;
	}

	/* syncUUID[0] is normalized UUID received over the wire
	 * syncUUID[1] is denormalized UUID, generated here
	 */
	(void)slap_uuidstr_from_normalized( &syncUUID[1], &syncUUID[0], op->o_tmpmemctx );
	Debug( LDAP_DEBUG_SYNC,
		"syncrepl_message_to_entry: %s DN: %s, UUID: %s\n",
		si->si_ridtxt, bdn.bv_val, syncUUID[1].bv_val );

	if ( syncstate == LDAP_SYNC_PRESENT || syncstate == LDAP_SYNC_DELETE ) {
		/* NOTE: this could be done even before decoding the DN,
		 * although encoding errors wouldn't be detected */
		rc = LDAP_SUCCESS;
		goto done;
	}

	if ( entry == NULL ) {
		return -1;
	}

	REWRITE_DN( si, bdn, bv2, dn, ndn );
	if ( rc != LDAP_SUCCESS ) {
		/* One of the things that could happen is that the schema
		 * is not lined-up; this could result in unknown attributes.
		 * A value non conformant to the syntax should be unlikely,
		 * except when replicating between different versions
		 * of the software, or when syntax validation bugs are fixed
		 */
		Debug( LDAP_DEBUG_ANY,
			"syncrepl_message_to_entry: "
			"%s dn \"%s\" normalization failed (%d)",
			si->si_ridtxt, bdn.bv_val, rc );
		return rc;
	}

	ber_dupbv( &op->o_req_dn, &dn );
	ber_dupbv( &op->o_req_ndn, &ndn );
	slap_sl_free( ndn.bv_val, op->o_tmpmemctx );
	slap_sl_free( dn.bv_val, op->o_tmpmemctx );

	is_ctx = dn_match( &op->o_req_ndn, &op->o_bd->be_nsuffix[0] );

	e = entry_alloc();
	e->e_name = op->o_req_dn;
	e->e_nname = op->o_req_ndn;

	while ( ber_remaining( ber ) ) {
		if ( (ber_scanf( ber, "{mW}", &tmp.sml_type, &tmp.sml_values ) ==
			LBER_ERROR ) || BER_BVISNULL( &tmp.sml_type ) )
		{
			break;
		}

		/* Drop all updates to the contextCSN of the context entry
		 * (ITS#4622, etc.)
		 */
		if ( is_ctx && !strcasecmp( tmp.sml_type.bv_val,
			slap_schema.si_ad_contextCSN->ad_cname.bv_val )) {
			ber_bvarray_free( tmp.sml_values );
			continue;
		}

		mod  = (Modifications *) ch_malloc( sizeof( Modifications ) );

		mod->sml_op = LDAP_MOD_REPLACE;
		mod->sml_flags = 0;
		mod->sml_next = NULL;
		mod->sml_desc = NULL;
		mod->sml_type = tmp.sml_type;
		mod->sml_values = tmp.sml_values;
		mod->sml_nvalues = NULL;
		mod->sml_numvals = 0;	/* slap_mods_check will set this */

#ifdef ENABLE_REWRITE
		if (si->si_rewrite) {
			AttributeDescription *ad = NULL;
			slap_bv2ad( &tmp.sml_type, &ad, &text );
			if ( ad ) {
				mod->sml_desc = ad;
				mod->sml_type = ad->ad_cname;
				if ( ad->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName ) {
					int i;
					for ( i = 0; tmp.sml_values[i].bv_val; i++ ) {
						syncrepl_rewrite_dn( si, &tmp.sml_values[i], &bv2);
						if ( !BER_BVISNULL( &bv2 )) {
							ber_memfree( tmp.sml_values[i].bv_val );
							tmp.sml_values[i] = bv2;
						}
					}
				}
			}
		}
#endif
		*modtail = mod;
		modtail = &mod->sml_next;
	}

	if ( *modlist == NULL ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_entry: %s no attributes\n",
			si->si_ridtxt, 0, 0 );
		rc = -1;
		goto done;
	}

	rc = slap_mods_check( op, *modlist, &text, txtbuf, textlen, NULL );

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_entry: %s mods check (%s)\n",
			si->si_ridtxt, text, 0 );
		goto done;
	}

	/* Strip out dynamically generated attrs */
	for ( modtail = modlist; *modtail ; ) {
		mod = *modtail;
		if ( mod->sml_desc->ad_type->sat_flags & SLAP_AT_DYNAMIC ) {
			*modtail = mod->sml_next;
			slap_mod_free( &mod->sml_mod, 0 );
			ch_free( mod );
		} else {
			modtail = &mod->sml_next;
		}
	}

	/* Strip out attrs in exattrs list */
	for ( modtail = modlist; *modtail ; ) {
		mod = *modtail;
		if ( ldap_charray_inlist( si->si_exattrs,
			mod->sml_desc->ad_type->sat_cname.bv_val ) )
		{
			*modtail = mod->sml_next;
			slap_mod_free( &mod->sml_mod, 0 );
			ch_free( mod );
		} else {
			modtail = &mod->sml_next;
		}
	}

	rc = slap_mods2entry( *modlist, &e, 1, 1, &text, txtbuf, textlen);
	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl_message_to_entry: %s mods2entry (%s)\n",
			si->si_ridtxt, text, 0 );
	}

done:
	ber_free( ber, 0 );
	if ( rc != LDAP_SUCCESS ) {
		if ( e ) {
			entry_free( e );
			e = NULL;
		}
	}
	if ( entry )
		*entry = e;

	return rc;
}

static struct berval generic_filterstr = BER_BVC("(objectclass=*)");

/* During a refresh, we may get an LDAP_SYNC_ADD for an already existing
 * entry if a previous refresh was interrupted before sending us a new
 * context state. We try to compare the new entry to the existing entry
 * and ignore the new entry if they are the same.
 *
 * Also, we may get an update where the entryDN has changed, due to
 * a ModDn on the provider. We detect this as well, so we can issue
 * the corresponding operation locally.
 *
 * In the case of a modify, we get a list of all the attributes
 * in the original entry. Rather than deleting the entry and re-adding it,
 * we issue a Modify request that deletes all the attributes and adds all
 * the new ones. This avoids the issue of trying to delete/add a non-leaf
 * entry.
 *
 * We otherwise distinguish ModDN from Modify; in the case of
 * a ModDN we just use the CSN, modifyTimestamp and modifiersName
 * operational attributes from the entry, and do a regular ModDN.
 */
typedef struct dninfo {
	Entry *new_entry;
	struct berval dn;
	struct berval ndn;
	struct berval nnewSup;
	int renamed;	/* Was an existing entry renamed? */
	int delOldRDN;	/* Was old RDN deleted? */
	Modifications **modlist;	/* the modlist we received */
	Modifications *mods;	/* the modlist we compared */
	int oldNcount;		/* #values of old naming attr */
	AttributeDescription *oldDesc;	/* for renames */
	AttributeDescription *newDesc;	/* for renames */
} dninfo;

/* return 1 if inserted, 0 otherwise */
static int
avl_presentlist_insert(
	syncinfo_t* si,
	struct berval *syncUUID )
{
	struct berval *syncuuid_bv = ch_malloc( sizeof( struct berval ) + syncUUID->bv_len + 1 );

	syncuuid_bv->bv_len = syncUUID->bv_len;
	syncuuid_bv->bv_val = (char *)&syncuuid_bv[1];
	AC_MEMCPY( syncuuid_bv->bv_val, syncUUID->bv_val, syncUUID->bv_len );
	syncuuid_bv->bv_val[ syncuuid_bv->bv_len ] = '\0';

	if ( avl_insert( &si->si_presentlist, (caddr_t) syncuuid_bv,
		syncuuid_cmp, avl_dup_error ) )
	{
		ch_free( syncuuid_bv );
		return 0;
	}

	return 1;
}

static int
syncrepl_entry(
	syncinfo_t* si,
	Operation *op,
	Entry* entry,
	Modifications** modlist,
	int syncstate,
	struct berval* syncUUID,
	struct berval* syncCSN )
{
	Backend *be = op->o_bd;
	slap_callback	cb = { NULL, NULL, NULL, NULL };
	int syncuuid_inserted = 0;

	SlapReply	rs_search = {REP_RESULT};
	Filter f = {0};
	AttributeAssertion ava = ATTRIBUTEASSERTION_INIT;
	int rc = LDAP_SUCCESS;

	struct berval pdn = BER_BVNULL;
	dninfo dni = {0};
	int	retry = 1;
	int	freecsn = 1;

	Debug( LDAP_DEBUG_SYNC,
		"syncrepl_entry: %s LDAP_RES_SEARCH_ENTRY(LDAP_SYNC_%s)\n",
		si->si_ridtxt, syncrepl_state2str( syncstate ), 0 );

	if (( syncstate == LDAP_SYNC_PRESENT || syncstate == LDAP_SYNC_ADD ) ) {
		if ( !si->si_refreshPresent && !si->si_refreshDone ) {
			syncuuid_inserted = avl_presentlist_insert( si, syncUUID );
		}
	}

	if ( syncstate == LDAP_SYNC_PRESENT ) {
		return 0;
	} else if ( syncstate != LDAP_SYNC_DELETE ) {
		if ( entry == NULL ) {
			return 0;
		}
	}

	if ( syncstate != LDAP_SYNC_DELETE ) {
		Attribute	*a = attr_find( entry->e_attrs, slap_schema.si_ad_entryUUID );

		if ( a == NULL ) {
			/* add if missing */
			attr_merge_one( entry, slap_schema.si_ad_entryUUID,
				&syncUUID[1], syncUUID );

		} else if ( !bvmatch( &a->a_nvals[0], syncUUID ) ) {
			/* replace only if necessary */
			if ( a->a_nvals != a->a_vals ) {
				ber_memfree( a->a_nvals[0].bv_val );
				ber_dupbv( &a->a_nvals[0], syncUUID );
			}
			ber_memfree( a->a_vals[0].bv_val );
			ber_dupbv( &a->a_vals[0], &syncUUID[1] );
		}
	}

	f.f_choice = LDAP_FILTER_EQUALITY;
	f.f_ava = &ava;
	ava.aa_desc = slap_schema.si_ad_entryUUID;
	ava.aa_value = *syncUUID;

	if ( syncuuid_inserted ) {
		Debug( LDAP_DEBUG_SYNC, "syncrepl_entry: %s inserted UUID %s\n",
			si->si_ridtxt, syncUUID[1].bv_val, 0 );
	}
	op->ors_filter = &f;

	op->ors_filterstr.bv_len = STRLENOF( "(entryUUID=)" ) + syncUUID[1].bv_len;
	op->ors_filterstr.bv_val = (char *) slap_sl_malloc(
		op->ors_filterstr.bv_len + 1, op->o_tmpmemctx ); 
	AC_MEMCPY( op->ors_filterstr.bv_val, "(entryUUID=", STRLENOF( "(entryUUID=" ) );
	AC_MEMCPY( &op->ors_filterstr.bv_val[STRLENOF( "(entryUUID=" )],
		syncUUID[1].bv_val, syncUUID[1].bv_len );
	op->ors_filterstr.bv_val[op->ors_filterstr.bv_len - 1] = ')';
	op->ors_filterstr.bv_val[op->ors_filterstr.bv_len] = '\0';

	op->o_tag = LDAP_REQ_SEARCH;
	op->ors_scope = LDAP_SCOPE_SUBTREE;
	op->ors_deref = LDAP_DEREF_NEVER;

	/* get the entry for this UUID */
#ifdef ENABLE_REWRITE
	if ( si->si_rewrite ) {
		op->o_req_dn = si->si_suffixm;
		op->o_req_ndn = si->si_suffixm;
	} else
#endif
	{
		op->o_req_dn = si->si_base;
		op->o_req_ndn = si->si_base;
	}

	op->o_time = slap_get_time();
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_slimit = 1;
	op->ors_limit = NULL;

	op->ors_attrs = slap_anlist_all_attributes;
	op->ors_attrsonly = 0;

	/* set callback function */
	op->o_callback = &cb;
	cb.sc_response = dn_callback;
	cb.sc_private = &dni;
	dni.new_entry = entry;
	dni.modlist = modlist;

	rc = be->be_search( op, &rs_search );
	Debug( LDAP_DEBUG_SYNC,
			"syncrepl_entry: %s be_search (%d)\n", 
			si->si_ridtxt, rc, 0 );

	if ( !BER_BVISNULL( &op->ors_filterstr ) ) {
		slap_sl_free( op->ors_filterstr.bv_val, op->o_tmpmemctx );
	}

	cb.sc_response = null_callback;
	cb.sc_private = si;

	if ( entry && !BER_BVISNULL( &entry->e_name ) ) {
		Debug( LDAP_DEBUG_SYNC,
				"syncrepl_entry: %s %s\n",
				si->si_ridtxt, entry->e_name.bv_val, 0 );
	} else {
		Debug( LDAP_DEBUG_SYNC,
				"syncrepl_entry: %s %s\n",
				si->si_ridtxt, dni.dn.bv_val ? dni.dn.bv_val : "(null)", 0 );
	}

	assert( BER_BVISNULL( &op->o_csn ) );
	if ( syncCSN ) {
		slap_queue_csn( op, syncCSN );
	}

	slap_op_time( &op->o_time, &op->o_tincr );
	switch ( syncstate ) {
	case LDAP_SYNC_ADD:
	case LDAP_SYNC_MODIFY:
		if ( BER_BVISNULL( &op->o_csn ))
		{

			Attribute *a = attr_find( entry->e_attrs, slap_schema.si_ad_entryCSN );
			if ( a ) {
				/* FIXME: op->o_csn is assumed to be
				 * on the thread's slab; this needs
				 * to be cleared ASAP.
				 */
				op->o_csn = a->a_vals[0];
				freecsn = 0;
			}
		}
retry_add:;
		if ( BER_BVISNULL( &dni.dn ) ) {
			SlapReply	rs_add = {REP_RESULT};

			op->o_req_dn = entry->e_name;
			op->o_req_ndn = entry->e_nname;
			op->o_tag = LDAP_REQ_ADD;
			op->ora_e = entry;
			op->o_bd = si->si_wbe;

			rc = op->o_bd->be_add( op, &rs_add );
			Debug( LDAP_DEBUG_SYNC,
					"syncrepl_entry: %s be_add %s (%d)\n", 
					si->si_ridtxt, op->o_req_dn.bv_val, rc );
			switch ( rs_add.sr_err ) {
			case LDAP_SUCCESS:
				if ( op->ora_e == entry ) {
					be_entry_release_w( op, entry );
				}
				entry = NULL;
				break;

			case LDAP_REFERRAL:
			/* we assume that LDAP_NO_SUCH_OBJECT is returned 
			 * only if the suffix entry is not present.
			 * This should not happen during Persist phase.
			 */
			case LDAP_NO_SUCH_OBJECT:
				if ( abs(si->si_type) == LDAP_SYNC_REFRESH_AND_PERSIST &&
					si->si_refreshDone ) {
					/* Something's wrong, start over */
					ber_bvarray_free( si->si_syncCookie.ctxcsn );
					si->si_syncCookie.ctxcsn = NULL;
					ldap_pvt_thread_mutex_lock( &si->si_cookieState->cs_mutex );
					ber_bvarray_free( si->si_cookieState->cs_vals );
					ch_free( si->si_cookieState->cs_sids );
					si->si_cookieState->cs_vals = NULL;
					si->si_cookieState->cs_sids = 0;
					si->si_cookieState->cs_num = 0;
					ldap_pvt_thread_mutex_unlock( &si->si_cookieState->cs_mutex );
					return LDAP_NO_SUCH_OBJECT;
				}
				rc = syncrepl_add_glue( op, entry );
				entry = NULL;
				break;

			/* if an entry was added via syncrepl_add_glue(),
			 * it likely has no entryUUID, so the previous
			 * be_search() doesn't find it.  In this case,
			 * give syncrepl a chance to modify it. Also
			 * allow for entries that were recreated with the
			 * same DN but a different entryUUID.
			 */
			case LDAP_ALREADY_EXISTS:
				if ( retry ) {
					Operation	op2 = *op;
					SlapReply	rs2 = { REP_RESULT };
					slap_callback	cb2 = { 0 };

					op2.o_bd = be;
					op2.o_tag = LDAP_REQ_SEARCH;
					op2.o_req_dn = entry->e_name;
					op2.o_req_ndn = entry->e_nname;
					op2.ors_scope = LDAP_SCOPE_BASE;
					op2.ors_deref = LDAP_DEREF_NEVER;
					op2.ors_attrs = slap_anlist_all_attributes;
					op2.ors_attrsonly = 0;
					op2.ors_limit = NULL;
					op2.ors_slimit = 1;
					op2.ors_tlimit = SLAP_NO_LIMIT;

					f.f_choice = LDAP_FILTER_PRESENT;
					f.f_desc = slap_schema.si_ad_objectClass;
					op2.ors_filter = &f;
					op2.ors_filterstr = generic_filterstr;

					op2.o_callback = &cb2;
					cb2.sc_response = dn_callback;
					cb2.sc_private = &dni;

					rc = be->be_search( &op2, &rs2 );
					if ( rc ) goto done;

					retry = 0;
					slap_op_time( &op->o_time, &op->o_tincr );
					goto retry_add;
				}
				/* FALLTHRU */

			default:
				Debug( LDAP_DEBUG_ANY,
					"syncrepl_entry: %s be_add %s failed (%d)\n",
					si->si_ridtxt, op->o_req_dn.bv_val, rs_add.sr_err );
				break;
			}
			syncCSN = NULL;
			op->o_bd = be;
			goto done;
		}
		/* FALLTHRU */
		op->o_req_dn = dni.dn;
		op->o_req_ndn = dni.ndn;
		if ( dni.renamed ) {
			struct berval noldp, newp;
			Modifications *mod, **modtail, **ml, *m2;
			int i, got_replace = 0, just_rename = 0;
			SlapReply rs_modify = {REP_RESULT};

			op->o_tag = LDAP_REQ_MODRDN;
			dnRdn( &entry->e_name, &op->orr_newrdn );
			dnRdn( &entry->e_nname, &op->orr_nnewrdn );

			if ( !BER_BVISNULL( &dni.nnewSup )) {
				dnParent( &entry->e_name, &newp );
				op->orr_newSup = &newp;
				op->orr_nnewSup = &dni.nnewSup;
			} else {
				op->orr_newSup = NULL;
				op->orr_nnewSup = NULL;
			}
			op->orr_deleteoldrdn = dni.delOldRDN;
			op->orr_modlist = NULL;
			if ( ( rc = slap_modrdn2mods( op, &rs_modify ) ) ) {
				goto done;
			}

			/* Drop the RDN-related mods from this op, because their
			 * equivalents were just setup by slap_modrdn2mods.
			 *
			 * If delOldRDN is TRUE then we should see a delete modop
			 * for oldDesc. We might see a replace instead.
			 *  delete with no values: therefore newDesc != oldDesc.
			 *   if oldNcount == 1, then Drop this op.
			 *  delete with 1 value: can only be the oldRDN value. Drop op.
			 *  delete with N values: Drop oldRDN value, keep remainder.
			 *  replace with 1 value: if oldNcount == 1 and
			 *     newDesc == oldDesc, Drop this op.
			 * Any other cases must be left intact.
			 *
			 * We should also see an add modop for newDesc. (But not if
			 * we got a replace modop due to delOldRDN.) If it has
			 * multiple values, we'll have to drop the new RDN value.
			 */
			modtail = &op->orr_modlist;
			if ( dni.delOldRDN ) {
				for ( ml = &dni.mods; *ml; ml = &(*ml)->sml_next ) {
					if ( (*ml)->sml_desc == dni.oldDesc ) {
						mod = *ml;
						if ( mod->sml_op == LDAP_MOD_REPLACE &&
							dni.oldDesc != dni.newDesc ) {
							/* This Replace is due to other Mods.
							 * Just let it ride.
							 */
							continue;
						}
						if ( mod->sml_numvals <= 1 &&
							dni.oldNcount == 1 &&
							( mod->sml_op == LDAP_MOD_DELETE ||
							  mod->sml_op == LDAP_MOD_REPLACE )) {
							if ( mod->sml_op == LDAP_MOD_REPLACE )
								got_replace = 1;
							/* Drop this op */
							*ml = mod->sml_next;
							mod->sml_next = NULL;
							slap_mods_free( mod, 1 );
							break;
						}
						if ( mod->sml_op != LDAP_MOD_DELETE || mod->sml_numvals == 0 )
							continue;
						for ( m2 = op->orr_modlist; m2; m2=m2->sml_next ) {
							if ( m2->sml_desc == dni.oldDesc &&
								m2->sml_op == LDAP_MOD_DELETE ) break;
						}
						for ( i=0; i<mod->sml_numvals; i++ ) {
							if ( bvmatch( &mod->sml_values[i], &m2->sml_values[0] )) {
								mod->sml_numvals--;
								ch_free( mod->sml_values[i].bv_val );
								mod->sml_values[i] = mod->sml_values[mod->sml_numvals];
								BER_BVZERO( &mod->sml_values[mod->sml_numvals] );
								if ( mod->sml_nvalues ) {
									ch_free( mod->sml_nvalues[i].bv_val );
									mod->sml_nvalues[i] = mod->sml_nvalues[mod->sml_numvals];
									BER_BVZERO( &mod->sml_nvalues[mod->sml_numvals] );
								}
								break;
							}
						}
						if ( !mod->sml_numvals ) {
							/* Drop this op */
							*ml = mod->sml_next;
							mod->sml_next = NULL;
							slap_mods_free( mod, 1 );
						}
						break;
					}
				}
			}
			if ( !got_replace ) {
				for ( ml = &dni.mods; *ml; ml = &(*ml)->sml_next ) {
					if ( (*ml)->sml_desc == dni.newDesc ) {
						mod = *ml;
						if ( mod->sml_op != LDAP_MOD_ADD )
							continue;
						if ( mod->sml_numvals == 1 ) {
							/* Drop this op */
							*ml = mod->sml_next;
							mod->sml_next = NULL;
							slap_mods_free( mod, 1 );
							break;
						}
						for ( m2 = op->orr_modlist; m2; m2=m2->sml_next ) {
							if ( m2->sml_desc == dni.oldDesc &&
								m2->sml_op == SLAP_MOD_SOFTADD ) break;
						}
						for ( i=0; i<mod->sml_numvals; i++ ) {
							if ( bvmatch( &mod->sml_values[i], &m2->sml_values[0] )) {
								mod->sml_numvals--;
								ch_free( mod->sml_values[i].bv_val );
								mod->sml_values[i] = mod->sml_values[mod->sml_numvals];
								BER_BVZERO( &mod->sml_values[mod->sml_numvals] );
								if ( mod->sml_nvalues ) {
									ch_free( mod->sml_nvalues[i].bv_val );
									mod->sml_nvalues[i] = mod->sml_nvalues[mod->sml_numvals];
									BER_BVZERO( &mod->sml_nvalues[mod->sml_numvals] );
								}
								break;
							}
						}
						break;
					}
				}
			}

			/* RDNs must be NUL-terminated for back-ldap */
			noldp = op->orr_newrdn;
			ber_dupbv_x( &op->orr_newrdn, &noldp, op->o_tmpmemctx );
			noldp = op->orr_nnewrdn;
			ber_dupbv_x( &op->orr_nnewrdn, &noldp, op->o_tmpmemctx );

			/* Setup opattrs too */
			{
				static AttributeDescription *nullattr = NULL;
				static AttributeDescription **const opattrs[] = {
					&slap_schema.si_ad_entryCSN,
					&slap_schema.si_ad_modifiersName,
					&slap_schema.si_ad_modifyTimestamp,
					&nullattr
				};
				AttributeDescription *opattr;
				int i;

				modtail = &m2;
				/* pull mod off incoming modlist */
				for ( i = 0; (opattr = *opattrs[i]) != NULL; i++ ) {
					for ( ml = &dni.mods; *ml; ml = &(*ml)->sml_next )
					{
						if ( (*ml)->sml_desc == opattr ) {
							mod = *ml;
							*ml = mod->sml_next;
							mod->sml_next = NULL;
							*modtail = mod;
							modtail = &mod->sml_next;
							break;
						}
					}
				}
				/* If there are still Modifications left, put the opattrs
				 * back, and let be_modify run. Otherwise, append the opattrs
				 * to the orr_modlist.
				 */
				if ( dni.mods ) {
					mod = dni.mods;
					/* don't set a CSN for the rename op */
					if ( syncCSN )
						slap_graduate_commit_csn( op );
				} else {
					mod = op->orr_modlist;
					just_rename = 1;
				}
				for ( ; mod->sml_next; mod=mod->sml_next );
				mod->sml_next = m2;
			}
			op->o_bd = si->si_wbe;
retry_modrdn:;
			rs_reinit( &rs_modify, REP_RESULT );
			rc = op->o_bd->be_modrdn( op, &rs_modify );

			/* NOTE: noSuchObject should result because the new superior
			 * has not been added yet (ITS#6472) */
			if ( rc == LDAP_NO_SUCH_OBJECT && op->orr_nnewSup != NULL ) {
				Operation op2 = *op;
				rc = syncrepl_add_glue_ancestors( &op2, entry );
				if ( rc == LDAP_SUCCESS ) {
					goto retry_modrdn;
				}
			}
		
			op->o_tmpfree( op->orr_nnewrdn.bv_val, op->o_tmpmemctx );
			op->o_tmpfree( op->orr_newrdn.bv_val, op->o_tmpmemctx );

			slap_mods_free( op->orr_modlist, 1 );
			Debug( LDAP_DEBUG_SYNC,
					"syncrepl_entry: %s be_modrdn %s (%d)\n", 
					si->si_ridtxt, op->o_req_dn.bv_val, rc );
			op->o_bd = be;
			/* Renamed entries may still have other mods so just fallthru */
			op->o_req_dn = entry->e_name;
			op->o_req_ndn = entry->e_nname;
			/* Use CSN on the modify */
			if ( just_rename )
				syncCSN = NULL;
			else if ( syncCSN )
				slap_queue_csn( op, syncCSN );
		}
		if ( dni.mods ) {
			SlapReply rs_modify = {REP_RESULT};

			op->o_tag = LDAP_REQ_MODIFY;
			op->orm_modlist = dni.mods;
			op->orm_no_opattrs = 1;
			op->o_bd = si->si_wbe;

			rc = op->o_bd->be_modify( op, &rs_modify );
			slap_mods_free( op->orm_modlist, 1 );
			op->orm_no_opattrs = 0;
			Debug( LDAP_DEBUG_SYNC,
					"syncrepl_entry: %s be_modify %s (%d)\n", 
					si->si_ridtxt, op->o_req_dn.bv_val, rc );
			if ( rs_modify.sr_err != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY,
					"syncrepl_entry: %s be_modify failed (%d)\n",
					si->si_ridtxt, rs_modify.sr_err, 0 );
			}
			syncCSN = NULL;
			op->o_bd = be;
		} else if ( !dni.renamed ) {
			Debug( LDAP_DEBUG_SYNC,
					"syncrepl_entry: %s entry unchanged, ignored (%s)\n", 
					si->si_ridtxt, op->o_req_dn.bv_val, 0 );
			if ( syncCSN ) {
				slap_graduate_commit_csn( op );
				syncCSN = NULL;
			}
		}
		goto done;
	case LDAP_SYNC_DELETE :
		if ( !BER_BVISNULL( &dni.dn ) ) {
			SlapReply	rs_delete = {REP_RESULT};
			op->o_req_dn = dni.dn;
			op->o_req_ndn = dni.ndn;
			op->o_tag = LDAP_REQ_DELETE;
			op->o_bd = si->si_wbe;
			if ( !syncCSN ) {
				slap_queue_csn( op, si->si_syncCookie.ctxcsn );
			}
			rc = op->o_bd->be_delete( op, &rs_delete );
			Debug( LDAP_DEBUG_SYNC,
					"syncrepl_entry: %s be_delete %s (%d)\n", 
					si->si_ridtxt, op->o_req_dn.bv_val, rc );
			if ( rc == LDAP_NO_SUCH_OBJECT )
				rc = LDAP_SUCCESS;

			while ( rs_delete.sr_err == LDAP_SUCCESS
				&& op->o_delete_glue_parent ) {
				op->o_delete_glue_parent = 0;
				if ( !be_issuffix( be, &op->o_req_ndn ) ) {
					slap_callback cb = { NULL };
					cb.sc_response = slap_null_cb;
					dnParent( &op->o_req_ndn, &pdn );
					op->o_req_dn = pdn;
					op->o_req_ndn = pdn;
					op->o_callback = &cb;
					rs_reinit( &rs_delete, REP_RESULT );
					op->o_bd->be_delete( op, &rs_delete );
				} else {
					break;
				}
			}
			syncCSN = NULL;
			op->o_bd = be;
		}
		goto done;

	default :
		Debug( LDAP_DEBUG_ANY,
			"syncrepl_entry: %s unknown syncstate\n", si->si_ridtxt, 0, 0 );
		goto done;
	}

done:
	slap_sl_free( syncUUID[1].bv_val, op->o_tmpmemctx );
	BER_BVZERO( &syncUUID[1] );
	if ( !BER_BVISNULL( &dni.ndn ) ) {
		op->o_tmpfree( dni.ndn.bv_val, op->o_tmpmemctx );
	}
	if ( !BER_BVISNULL( &dni.dn ) ) {
		op->o_tmpfree( dni.dn.bv_val, op->o_tmpmemctx );
	}
	if ( entry ) {
		entry_free( entry );
	}
	if ( syncCSN ) {
		slap_graduate_commit_csn( op );
	}
	if ( !BER_BVISNULL( &op->o_csn ) && freecsn ) {
		op->o_tmpfree( op->o_csn.bv_val, op->o_tmpmemctx );
	}
	BER_BVZERO( &op->o_csn );
	return rc;
}

static struct berval gcbva[] = {
	BER_BVC("top"),
	BER_BVC("glue"),
	BER_BVNULL
};

#define NP_DELETE_ONE	2

static void
syncrepl_del_nonpresent(
	Operation *op,
	syncinfo_t *si,
	BerVarray uuids,
	struct sync_cookie *sc,
	int m )
{
	Backend* be = op->o_bd;
	slap_callback	cb = { NULL };
	struct nonpresent_entry *np_list, *np_prev;
	int rc;
	AttributeName	an[2];

	struct berval pdn = BER_BVNULL;
	struct berval csn;

#ifdef ENABLE_REWRITE
	if ( si->si_rewrite ) {
		op->o_req_dn = si->si_suffixm;
		op->o_req_ndn = si->si_suffixm;
	} else
#endif
	{
		op->o_req_dn = si->si_base;
		op->o_req_ndn = si->si_base;
	}

	cb.sc_response = nonpresent_callback;
	cb.sc_private = si;

	op->o_callback = &cb;
	op->o_tag = LDAP_REQ_SEARCH;
	op->ors_scope = si->si_scope;
	op->ors_deref = LDAP_DEREF_NEVER;
	op->o_time = slap_get_time();
	op->ors_tlimit = SLAP_NO_LIMIT;


	if ( uuids ) {
		Filter uf;
		AttributeAssertion eq = ATTRIBUTEASSERTION_INIT;
		int i;

		op->ors_attrsonly = 1;
		op->ors_attrs = slap_anlist_no_attrs;
		op->ors_limit = NULL;
		op->ors_filter = &uf;

		uf.f_ava = &eq;
		uf.f_av_desc = slap_schema.si_ad_entryUUID;
		uf.f_next = NULL;
		uf.f_choice = LDAP_FILTER_EQUALITY;
		si->si_refreshDelete |= NP_DELETE_ONE;

		for (i=0; uuids[i].bv_val; i++) {
			SlapReply rs_search = {REP_RESULT};

			op->ors_slimit = 1;
			uf.f_av_value = uuids[i];
			filter2bv_x( op, op->ors_filter, &op->ors_filterstr );
			rc = be->be_search( op, &rs_search );
			op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
		}
		si->si_refreshDelete ^= NP_DELETE_ONE;
	} else {
		Filter *cf, *of;
		Filter mmf[2];
		AttributeAssertion mmaa;
		SlapReply rs_search = {REP_RESULT};

		memset( &an[0], 0, 2 * sizeof( AttributeName ) );
		an[0].an_name = slap_schema.si_ad_entryUUID->ad_cname;
		an[0].an_desc = slap_schema.si_ad_entryUUID;
		op->ors_attrs = an;
		op->ors_slimit = SLAP_NO_LIMIT;
		op->ors_tlimit = SLAP_NO_LIMIT;
		op->ors_limit = NULL;
		op->ors_attrsonly = 0;
		op->ors_filter = filter_dup( si->si_filter, op->o_tmpmemctx );
		/* In multimaster, updates can continue to arrive while
		 * we're searching. Limit the search result to entries
		 * older than our newest cookie CSN.
		 */
		if ( SLAP_MULTIMASTER( op->o_bd )) {
			Filter *f;
			int i;

			f = mmf;
			f->f_choice = LDAP_FILTER_AND;
			f->f_next = op->ors_filter;
			f->f_and = f+1;
			of = f->f_and;
			f = of;
			f->f_choice = LDAP_FILTER_LE;
			f->f_ava = &mmaa;
			f->f_av_desc = slap_schema.si_ad_entryCSN;
			f->f_next = NULL;
			BER_BVZERO( &f->f_av_value );
			for ( i=0; i<sc->numcsns; i++ ) {
				if ( ber_bvcmp( &sc->ctxcsn[i], &f->f_av_value ) > 0 )
					f->f_av_value = sc->ctxcsn[i];
			}
			of = op->ors_filter;
			op->ors_filter = mmf;
			filter2bv_x( op, op->ors_filter, &op->ors_filterstr );
		} else {
			cf = NULL;
			op->ors_filterstr = si->si_filterstr;
		}
		op->o_nocaching = 1;


		rc = be->be_search( op, &rs_search );
		if ( SLAP_MULTIMASTER( op->o_bd )) {
			op->ors_filter = of;
		}
		if ( op->ors_filter ) filter_free_x( op, op->ors_filter, 1 );
		if ( op->ors_filterstr.bv_val != si->si_filterstr.bv_val ) {
			op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
		}

	}

	op->o_nocaching = 0;

	if ( !LDAP_LIST_EMPTY( &si->si_nonpresentlist ) ) {

		if ( sc->ctxcsn && !BER_BVISNULL( &sc->ctxcsn[m] ) ) {
			csn = sc->ctxcsn[m];
		} else {
			csn = si->si_syncCookie.ctxcsn[0];
		}

		op->o_bd = si->si_wbe;
		slap_queue_csn( op, &csn );

		np_list = LDAP_LIST_FIRST( &si->si_nonpresentlist );
		while ( np_list != NULL ) {
			SlapReply rs_delete = {REP_RESULT};

			LDAP_LIST_REMOVE( np_list, npe_link );
			np_prev = np_list;
			np_list = LDAP_LIST_NEXT( np_list, npe_link );
			op->o_tag = LDAP_REQ_DELETE;
			op->o_callback = &cb;
			cb.sc_response = null_callback;
			cb.sc_private = si;
			op->o_req_dn = *np_prev->npe_name;
			op->o_req_ndn = *np_prev->npe_nname;
			rc = op->o_bd->be_delete( op, &rs_delete );
			Debug( LDAP_DEBUG_SYNC,
				"syncrepl_del_nonpresent: %s be_delete %s (%d)\n", 
				si->si_ridtxt, op->o_req_dn.bv_val, rc );

			if ( rs_delete.sr_err == LDAP_NOT_ALLOWED_ON_NONLEAF ) {
				SlapReply rs_modify = {REP_RESULT};
				Modifications mod1, mod2;
				mod1.sml_op = LDAP_MOD_REPLACE;
				mod1.sml_flags = 0;
				mod1.sml_desc = slap_schema.si_ad_objectClass;
				mod1.sml_type = mod1.sml_desc->ad_cname;
				mod1.sml_numvals = 2;
				mod1.sml_values = &gcbva[0];
				mod1.sml_nvalues = NULL;
				mod1.sml_next = &mod2;

				mod2.sml_op = LDAP_MOD_REPLACE;
				mod2.sml_flags = 0;
				mod2.sml_desc = slap_schema.si_ad_structuralObjectClass;
				mod2.sml_type = mod2.sml_desc->ad_cname;
				mod2.sml_numvals = 1;
				mod2.sml_values = &gcbva[1];
				mod2.sml_nvalues = NULL;
				mod2.sml_next = NULL;

				op->o_tag = LDAP_REQ_MODIFY;
				op->orm_modlist = &mod1;

				rc = op->o_bd->be_modify( op, &rs_modify );
				if ( mod2.sml_next ) slap_mods_free( mod2.sml_next, 1 );
			}

			while ( rs_delete.sr_err == LDAP_SUCCESS &&
					op->o_delete_glue_parent ) {
				op->o_delete_glue_parent = 0;
				if ( !be_issuffix( be, &op->o_req_ndn ) ) {
					slap_callback cb = { NULL };
					cb.sc_response = slap_null_cb;
					dnParent( &op->o_req_ndn, &pdn );
					op->o_req_dn = pdn;
					op->o_req_ndn = pdn;
					op->o_callback = &cb;
					rs_reinit( &rs_delete, REP_RESULT );
					/* give it a root privil ? */
					op->o_bd->be_delete( op, &rs_delete );
				} else {
					break;
				}
			}

			op->o_delete_glue_parent = 0;

			ber_bvfree( np_prev->npe_name );
			ber_bvfree( np_prev->npe_nname );
			ch_free( np_prev );

			if ( slapd_shutdown ) {
				break;
			}
		}

		slap_graduate_commit_csn( op );
		op->o_bd = be;

		op->o_tmpfree( op->o_csn.bv_val, op->o_tmpmemctx );
		BER_BVZERO( &op->o_csn );
	}

	return;
}

static int
syncrepl_add_glue_ancestors(
	Operation* op,
	Entry *e )
{
	Backend *be = op->o_bd;
	slap_callback cb = { NULL };
	Attribute	*a;
	int	rc = LDAP_SUCCESS;
	int suffrdns;
	int i;
	struct berval dn = BER_BVNULL;
	struct berval ndn = BER_BVNULL;
	Entry	*glue;
	struct berval	ptr, nptr;
	char		*comma;

	op->o_tag = LDAP_REQ_ADD;
	op->o_callback = &cb;
	cb.sc_response = null_callback;
	cb.sc_private = NULL;

	dn = e->e_name;
	ndn = e->e_nname;

	/* count RDNs in suffix */
	if ( !BER_BVISEMPTY( &be->be_nsuffix[0] ) ) {
		for ( i = 0, ptr = be->be_nsuffix[0], comma = ptr.bv_val; comma != NULL; comma = ber_bvchr( &ptr, ',' ) ) {
			comma++;
			ptr.bv_len -= comma - ptr.bv_val;
			ptr.bv_val = comma;
			i++;
		}
		suffrdns = i;
	} else {
		/* suffix is "" */
		suffrdns = 0;
	}

	/* Start with BE suffix */
	ptr = dn;
	for ( i = 0; i < suffrdns; i++ ) {
		comma = ber_bvrchr( &ptr, ',' );
		if ( comma != NULL ) {
			ptr.bv_len = comma - ptr.bv_val;
		} else {
			ptr.bv_len = 0;
			break;
		}
	}
	
	if ( !BER_BVISEMPTY( &ptr ) ) {
		dn.bv_len -= ptr.bv_len + ( suffrdns != 0 );
		dn.bv_val += ptr.bv_len + ( suffrdns != 0 );
	}

	/* the normalizedDNs are always the same length, no counting
	 * required.
	 */
	nptr = ndn;
	if ( ndn.bv_len > be->be_nsuffix[0].bv_len ) {
		ndn.bv_val += ndn.bv_len - be->be_nsuffix[0].bv_len;
		ndn.bv_len = be->be_nsuffix[0].bv_len;

		nptr.bv_len = ndn.bv_val - nptr.bv_val - 1;

	} else {
		nptr.bv_len = 0;
	}

	while ( ndn.bv_val > e->e_nname.bv_val ) {
		SlapReply	rs_add = {REP_RESULT};

		glue = entry_alloc();
		ber_dupbv( &glue->e_name, &dn );
		ber_dupbv( &glue->e_nname, &ndn );

		a = attr_alloc( slap_schema.si_ad_objectClass );

		a->a_numvals = 2;
		a->a_vals = ch_calloc( 3, sizeof( struct berval ) );
		ber_dupbv( &a->a_vals[0], &gcbva[0] );
		ber_dupbv( &a->a_vals[1], &gcbva[1] );
		ber_dupbv( &a->a_vals[2], &gcbva[2] );

		a->a_nvals = a->a_vals;

		a->a_next = glue->e_attrs;
		glue->e_attrs = a;

		a = attr_alloc( slap_schema.si_ad_structuralObjectClass );

		a->a_numvals = 1;
		a->a_vals = ch_calloc( 2, sizeof( struct berval ) );
		ber_dupbv( &a->a_vals[0], &gcbva[1] );
		ber_dupbv( &a->a_vals[1], &gcbva[2] );

		a->a_nvals = a->a_vals;

		a->a_next = glue->e_attrs;
		glue->e_attrs = a;

		op->o_req_dn = glue->e_name;
		op->o_req_ndn = glue->e_nname;
		op->ora_e = glue;
		rc = be->be_add ( op, &rs_add );
		if ( rs_add.sr_err == LDAP_SUCCESS ) {
			if ( op->ora_e == glue )
				be_entry_release_w( op, glue );
		} else {
		/* incl. ALREADY EXIST */
			entry_free( glue );
			if ( rs_add.sr_err != LDAP_ALREADY_EXISTS ) {
				entry_free( e );
				return rc;
			}
		}

		/* Move to next child */
		comma = ber_bvrchr( &ptr, ',' );
		if ( comma == NULL ) {
			break;
		}
		ptr.bv_len = comma - ptr.bv_val;
		
		dn.bv_val = ++comma;
		dn.bv_len = e->e_name.bv_len - (dn.bv_val - e->e_name.bv_val);

		comma = ber_bvrchr( &nptr, ',' );
		assert( comma != NULL );
		nptr.bv_len = comma - nptr.bv_val;

		ndn.bv_val = ++comma;
		ndn.bv_len = e->e_nname.bv_len - (ndn.bv_val - e->e_nname.bv_val);
	}

	return rc;
}

int
syncrepl_add_glue(
	Operation* op,
	Entry *e )
{
	slap_callback cb = { NULL };
	int	rc;
	Backend *be = op->o_bd;
	SlapReply	rs_add = {REP_RESULT};

	rc = syncrepl_add_glue_ancestors( op, e );
	switch ( rc ) {
	case LDAP_SUCCESS:
	case LDAP_ALREADY_EXISTS:
		break;

	default:
		return rc;
	}

	op->o_tag = LDAP_REQ_ADD;
	op->o_callback = &cb;
	cb.sc_response = null_callback;
	cb.sc_private = NULL;

	op->o_req_dn = e->e_name;
	op->o_req_ndn = e->e_nname;
	op->ora_e = e;
	rc = be->be_add ( op, &rs_add );
	if ( rs_add.sr_err == LDAP_SUCCESS ) {
		if ( op->ora_e == e )
			be_entry_release_w( op, e );
	} else {
		entry_free( e );
	}

	return rc;
}

static int
syncrepl_updateCookie(
	syncinfo_t *si,
	Operation *op,
	struct sync_cookie *syncCookie )
{
	Backend *be = op->o_bd;
	Modifications mod;
	struct berval first = BER_BVNULL;
	struct sync_cookie sc;
#ifdef CHECK_CSN
	Syntax *syn = slap_schema.si_ad_contextCSN->ad_type->sat_syntax;
#endif

	int rc, i, j, changed = 0;
	ber_len_t len;

	slap_callback cb = { NULL };
	SlapReply	rs_modify = {REP_RESULT};

	mod.sml_op = LDAP_MOD_REPLACE;
	mod.sml_desc = slap_schema.si_ad_contextCSN;
	mod.sml_type = mod.sml_desc->ad_cname;
	mod.sml_flags = SLAP_MOD_INTERNAL;
	mod.sml_nvalues = NULL;
	mod.sml_next = NULL;

	ldap_pvt_thread_mutex_lock( &si->si_cookieState->cs_mutex );

#ifdef CHECK_CSN
	for ( i=0; i<syncCookie->numcsns; i++ ) {
		assert( !syn->ssyn_validate( syn, syncCookie->ctxcsn+i ));
	}
	for ( i=0; i<si->si_cookieState->cs_num; i++ ) {
		assert( !syn->ssyn_validate( syn, si->si_cookieState->cs_vals+i ));
	}
#endif

	/* clone the cookieState CSNs so we can Replace the whole thing */
	sc.numcsns = si->si_cookieState->cs_num;
	if ( sc.numcsns ) {
		ber_bvarray_dup_x( &sc.ctxcsn, si->si_cookieState->cs_vals, NULL );
		sc.sids = ch_malloc( sc.numcsns * sizeof(int));
		for ( i=0; i<sc.numcsns; i++ )
			sc.sids[i] = si->si_cookieState->cs_sids[i];
	} else {
		sc.ctxcsn = NULL;
		sc.sids = NULL;
	}

	/* find any CSNs in the syncCookie that are newer than the cookieState */
	for ( i=0; i<syncCookie->numcsns; i++ ) {
		for ( j=0; j<sc.numcsns; j++ ) {
			if ( syncCookie->sids[i] < sc.sids[j] )
				break;
			if ( syncCookie->sids[i] != sc.sids[j] )
				continue;
			len = syncCookie->ctxcsn[i].bv_len;
			if ( len > sc.ctxcsn[j].bv_len )
				len = sc.ctxcsn[j].bv_len;
			if ( memcmp( syncCookie->ctxcsn[i].bv_val,
				sc.ctxcsn[j].bv_val, len ) > 0 ) {
				ber_bvreplace( &sc.ctxcsn[j], &syncCookie->ctxcsn[i] );
				changed = 1;
				if ( BER_BVISNULL( &first ) ||
					memcmp( syncCookie->ctxcsn[i].bv_val, first.bv_val, first.bv_len ) > 0 ) {
					first = syncCookie->ctxcsn[i];
				}
			}
			break;
		}
		/* there was no match for this SID, it's a new CSN */
		if ( j == sc.numcsns ||
			syncCookie->sids[i] != sc.sids[j] ) {
			slap_insert_csn_sids( &sc, j, syncCookie->sids[i],
				&syncCookie->ctxcsn[i] );
			if ( BER_BVISNULL( &first ) ||
				memcmp( syncCookie->ctxcsn[i].bv_val, first.bv_val, first.bv_len ) > 0 ) {
				first = syncCookie->ctxcsn[i];
			}
			changed = 1;
		}
	}
	/* Should never happen, ITS#5065 */
	if ( BER_BVISNULL( &first ) || !changed ) {
		ldap_pvt_thread_mutex_unlock( &si->si_cookieState->cs_mutex );
		ber_bvarray_free( sc.ctxcsn );
		ch_free( sc.sids );
		return 0;
	}
	op->o_bd = si->si_wbe;
	slap_queue_csn( op, &first );

	op->o_tag = LDAP_REQ_MODIFY;

	cb.sc_response = null_callback;
	cb.sc_private = si;

	op->o_callback = &cb;
	op->o_req_dn = si->si_contextdn;
	op->o_req_ndn = si->si_contextdn;

	/* update contextCSN */
	op->o_dont_replicate = 1;

	mod.sml_numvals = sc.numcsns;
	mod.sml_values = sc.ctxcsn;

	op->orm_modlist = &mod;
	op->orm_no_opattrs = 1;
	rc = op->o_bd->be_modify( op, &rs_modify );

	if ( rs_modify.sr_err == LDAP_NO_SUCH_OBJECT &&
		SLAP_SYNC_SUBENTRY( op->o_bd )) {
		const char	*text;
		char txtbuf[SLAP_TEXT_BUFLEN];
		size_t textlen = sizeof txtbuf;
		Entry *e = slap_create_context_csn_entry( op->o_bd, NULL );
		rs_reinit( &rs_modify, REP_RESULT );
		rc = slap_mods2entry( &mod, &e, 0, 1, &text, txtbuf, textlen);
		op->ora_e = e;
		rc = op->o_bd->be_add( op, &rs_modify );
		if ( e == op->ora_e )
			be_entry_release_w( op, op->ora_e );
	}

	op->orm_no_opattrs = 0;
	op->o_dont_replicate = 0;

	if ( rs_modify.sr_err == LDAP_SUCCESS ) {
		slap_sync_cookie_free( &si->si_syncCookie, 0 );
		ber_bvarray_free( si->si_cookieState->cs_vals );
		ch_free( si->si_cookieState->cs_sids );
		si->si_cookieState->cs_vals = sc.ctxcsn;
		si->si_cookieState->cs_sids = sc.sids;
		si->si_cookieState->cs_num = sc.numcsns;

		/* Don't just dup the provider's cookie, recreate it */
		si->si_syncCookie.numcsns = si->si_cookieState->cs_num;
		ber_bvarray_dup_x( &si->si_syncCookie.ctxcsn, si->si_cookieState->cs_vals, NULL );
		si->si_syncCookie.sids = ch_malloc( si->si_cookieState->cs_num * sizeof(int) );
		for ( i=0; i<si->si_cookieState->cs_num; i++ )
			si->si_syncCookie.sids[i] = si->si_cookieState->cs_sids[i];

		si->si_cookieState->cs_age++;
		si->si_cookieAge = si->si_cookieState->cs_age;
	} else {
		Debug( LDAP_DEBUG_ANY,
			"syncrepl_updateCookie: %s be_modify failed (%d)\n",
			si->si_ridtxt, rs_modify.sr_err, 0 );
		ch_free( sc.sids );
		ber_bvarray_free( sc.ctxcsn );
	}
	ldap_pvt_thread_mutex_unlock( &si->si_cookieState->cs_mutex );

	op->o_bd = be;
	op->o_tmpfree( op->o_csn.bv_val, op->o_tmpmemctx );
	BER_BVZERO( &op->o_csn );
	if ( mod.sml_next ) slap_mods_free( mod.sml_next, 1 );

#ifdef CHECK_CSN
	for ( i=0; i<si->si_cookieState->cs_num; i++ ) {
		assert( !syn->ssyn_validate( syn, si->si_cookieState->cs_vals+i ));
	}
#endif

	return rc;
}

/* Compare the attribute from the old entry to the one in the new
 * entry. The Modifications from the new entry will either be left
 * in place, or changed to an Add or Delete as needed.
 */
static void
attr_cmp( Operation *op, Attribute *old, Attribute *new,
	Modifications ***mret, Modifications ***mcur )
{
	int i, j;
	Modifications *mod, **modtail;

	modtail = *mret;

	if ( old ) {
		int n, o, nn, no;
		struct berval **adds, **dels;
		/* count old and new */
		for ( o=0; old->a_vals[o].bv_val; o++ ) ;
		for ( n=0; new->a_vals[n].bv_val; n++ ) ;

		/* there MUST be both old and new values */
		assert( o != 0 );
		assert( n != 0 );
		j = 0;

		adds = op->o_tmpalloc( sizeof(struct berval *) * n, op->o_tmpmemctx );
		dels = op->o_tmpalloc( sizeof(struct berval *) * o, op->o_tmpmemctx );

		for ( i=0; i<o; i++ ) dels[i] = &old->a_vals[i];
		for ( i=0; i<n; i++ ) adds[i] = &new->a_vals[i];

		nn = n; no = o;

		for ( i=0; i<o; i++ ) {
			for ( j=0; j<n; j++ ) {
				if ( !adds[j] )
					continue;
				if ( bvmatch( dels[i], adds[j] ) ) {
					no--;
					nn--;
					adds[j] = NULL;
					dels[i] = NULL;
					break;
				}
			}
		}

		/* Don't delete/add an objectClass, always use the replace op.
		 * Modify would fail if provider has replaced entry with a new,
		 * and the new explicitly includes a superior of a class that was
		 * only included implicitly in the old entry.  Ref ITS#5517.
		 *
		 * Also use replace op if attr has no equality matching rule.
		 * (ITS#5781)
		 */
		if ( ( nn || ( no > 0 && no < o ) ) &&
			( old->a_desc == slap_schema.si_ad_objectClass ||
			 !old->a_desc->ad_type->sat_equality ) )
		{
			no = o;
		}

		i = j;
		/* all old values were deleted, just use the replace op */
		if ( no == o ) {
			i = j-1;
		} else if ( no ) {
		/* delete some values */
			mod = ch_malloc( sizeof( Modifications ) );
			mod->sml_op = LDAP_MOD_DELETE;
			mod->sml_flags = 0;
			mod->sml_desc = old->a_desc;
			mod->sml_type = mod->sml_desc->ad_cname;
			mod->sml_numvals = no;
			mod->sml_values = ch_malloc( ( no + 1 ) * sizeof(struct berval) );
			if ( old->a_vals != old->a_nvals ) {
				mod->sml_nvalues = ch_malloc( ( no + 1 ) * sizeof(struct berval) );
			} else {
				mod->sml_nvalues = NULL;
			}
			j = 0;
			for ( i = 0; i < o; i++ ) {
				if ( !dels[i] ) continue;
				ber_dupbv( &mod->sml_values[j], &old->a_vals[i] );
				if ( mod->sml_nvalues ) {
					ber_dupbv( &mod->sml_nvalues[j], &old->a_nvals[i] );
				}
				j++;
			}
			BER_BVZERO( &mod->sml_values[j] );
			if ( mod->sml_nvalues ) {
				BER_BVZERO( &mod->sml_nvalues[j] );
			}
			*modtail = mod;
			modtail = &mod->sml_next;
			i = j;
		}
		op->o_tmpfree( dels, op->o_tmpmemctx );
		/* some values were added */
		if ( nn && no < o ) {
			mod = ch_malloc( sizeof( Modifications ) );
			mod->sml_op = LDAP_MOD_ADD;
			mod->sml_flags = 0;
			mod->sml_desc = old->a_desc;
			mod->sml_type = mod->sml_desc->ad_cname;
			mod->sml_numvals = nn;
			mod->sml_values = ch_malloc( ( nn + 1 ) * sizeof(struct berval) );
			if ( old->a_vals != old->a_nvals ) {
				mod->sml_nvalues = ch_malloc( ( nn + 1 ) * sizeof(struct berval) );
			} else {
				mod->sml_nvalues = NULL;
			}
			j = 0;
			for ( i = 0; i < n; i++ ) {
				if ( !adds[i] ) continue;
				ber_dupbv( &mod->sml_values[j], &new->a_vals[i] );
				if ( mod->sml_nvalues ) {
					ber_dupbv( &mod->sml_nvalues[j], &new->a_nvals[i] );
				}
				j++;
			}
			BER_BVZERO( &mod->sml_values[j] );
			if ( mod->sml_nvalues ) {
				BER_BVZERO( &mod->sml_nvalues[j] );
			}
			*modtail = mod;
			modtail = &mod->sml_next;
			i = j;
		}
		op->o_tmpfree( adds, op->o_tmpmemctx );
	} else {
		/* new attr, just use the new mod */
		i = 0;
		j = 1;
	}
	/* advance to next element */
	mod = **mcur;
	if ( mod ) {
		if ( i != j ) {
			**mcur = mod->sml_next;
			*modtail = mod;
			modtail = &mod->sml_next;
		} else {
			*mcur = &mod->sml_next;
		}
	}
	*mret = modtail;
}

/* Generate a set of modifications to change the old entry into the
 * new one. On input ml is a list of modifications equivalent to
 * the new entry. It will be massaged and the result will be stored
 * in mods.
 */
void syncrepl_diff_entry( Operation *op, Attribute *old, Attribute *new,
	Modifications **mods, Modifications **ml, int is_ctx)
{
	Modifications **modtail = mods;

	/* We assume that attributes are saved in the same order
	 * in the remote and local databases. So if we walk through
	 * the attributeDescriptions one by one they should match in
	 * lock step. If not, look for an add or delete.
	 */
	while ( old && new )
	{
		/* If we've seen this before, use its mod now */
		if ( new->a_flags & SLAP_ATTR_IXADD ) {
			attr_cmp( op, NULL, new, &modtail, &ml );
			new = new->a_next;
			continue;
		}
		/* Skip contextCSN */
		if ( is_ctx && old->a_desc ==
			slap_schema.si_ad_contextCSN ) {
			old = old->a_next;
			continue;
		}

		if ( old->a_desc != new->a_desc ) {
			Modifications *mod;
			Attribute *tmp;

			/* If it's just been re-added later,
			 * remember that we've seen it.
			 */
			tmp = attr_find( new, old->a_desc );
			if ( tmp ) {
				tmp->a_flags |= SLAP_ATTR_IXADD;
			} else {
				/* If it's a new attribute, pull it in.
				 */
				tmp = attr_find( old, new->a_desc );
				if ( !tmp ) {
					attr_cmp( op, NULL, new, &modtail, &ml );
					new = new->a_next;
					continue;
				}
				/* Delete old attr */
				mod = ch_malloc( sizeof( Modifications ) );
				mod->sml_op = LDAP_MOD_DELETE;
				mod->sml_flags = 0;
				mod->sml_desc = old->a_desc;
				mod->sml_type = mod->sml_desc->ad_cname;
				mod->sml_numvals = 0;
				mod->sml_values = NULL;
				mod->sml_nvalues = NULL;
				*modtail = mod;
				modtail = &mod->sml_next;
			}
			old = old->a_next;
			continue;
		}
		/* kludge - always update modifiersName so that it
		 * stays co-located with the other mod opattrs. But only
		 * if we know there are other valid mods.
		 */
		if ( *mods && ( old->a_desc == slap_schema.si_ad_modifiersName ||
			old->a_desc == slap_schema.si_ad_modifyTimestamp ))
			attr_cmp( op, NULL, new, &modtail, &ml );
		else
			attr_cmp( op, old, new, &modtail, &ml );
		new = new->a_next;
		old = old->a_next;
	}
	*modtail = *ml;
	*ml = NULL;
}

static int
dn_callback(
	Operation*	op,
	SlapReply*	rs )
{
	dninfo *dni = op->o_callback->sc_private;

	if ( rs->sr_type == REP_SEARCH ) {
		if ( !BER_BVISNULL( &dni->dn ) ) {
			Debug( LDAP_DEBUG_ANY,
				"dn_callback : consistency error - "
				"entryUUID is not unique\n", 0, 0, 0 );
		} else {
			ber_dupbv_x( &dni->dn, &rs->sr_entry->e_name, op->o_tmpmemctx );
			ber_dupbv_x( &dni->ndn, &rs->sr_entry->e_nname, op->o_tmpmemctx );
			/* If there is a new entry, see if it differs from the old.
			 * We compare the non-normalized values so that cosmetic changes
			 * in the provider are always propagated.
			 */
			if ( dni->new_entry ) {
				Attribute *old, *new;
				struct berval old_rdn, new_rdn;
				struct berval old_p, new_p;
				int is_ctx, new_sup = 0;

				/* If old entry is not a glue entry, make sure new entry
				 * is actually newer than old entry
				 */
				if ( !is_entry_glue( rs->sr_entry )) {
					old = attr_find( rs->sr_entry->e_attrs,
						slap_schema.si_ad_entryCSN );
					new = attr_find( dni->new_entry->e_attrs,
						slap_schema.si_ad_entryCSN );
					if ( new && old ) {
						int rc;
						ber_len_t len = old->a_vals[0].bv_len;
						if ( len > new->a_vals[0].bv_len )
							len = new->a_vals[0].bv_len;
						rc = memcmp( old->a_vals[0].bv_val,
							new->a_vals[0].bv_val, len );
						if ( rc > 0 ) {
							Debug( LDAP_DEBUG_SYNC,
								"dn_callback : new entry is older than ours "
								"%s ours %s, new %s\n",
								rs->sr_entry->e_name.bv_val,
								old->a_vals[0].bv_val,
								new->a_vals[0].bv_val );
							return LDAP_SUCCESS;
						} else if ( rc == 0 ) {
							Debug( LDAP_DEBUG_SYNC,
								"dn_callback : entries have identical CSN "
								"%s %s\n",
								rs->sr_entry->e_name.bv_val,
								old->a_vals[0].bv_val, 0 );
							return LDAP_SUCCESS;
						}
					}
				}

				is_ctx = dn_match( &rs->sr_entry->e_nname,
					&op->o_bd->be_nsuffix[0] );

				/* Did the DN change?
				 * case changes in the parent are ignored,
				 * we only want to know if the RDN was
				 * actually changed.
				 */
				dnRdn( &rs->sr_entry->e_name, &old_rdn );
				dnRdn( &dni->new_entry->e_name, &new_rdn );
				dnParent( &rs->sr_entry->e_nname, &old_p );
				dnParent( &dni->new_entry->e_nname, &new_p );

				new_sup = !dn_match( &old_p, &new_p );
				if ( !dn_match( &old_rdn, &new_rdn ) || new_sup )
				{
					struct berval oldRDN, oldVal;
					AttributeDescription *ad = NULL;
					int oldpos, newpos;
					Attribute *a;

					dni->renamed = 1;
					if ( new_sup )
						dni->nnewSup = new_p;

					/* See if the oldRDN was deleted */
					dnRdn( &rs->sr_entry->e_nname, &oldRDN );
					oldVal.bv_val = strchr(oldRDN.bv_val, '=') + 1;
					oldVal.bv_len = oldRDN.bv_len - ( oldVal.bv_val -
						oldRDN.bv_val );
					oldRDN.bv_len -= oldVal.bv_len + 1;
					slap_bv2ad( &oldRDN, &ad, &rs->sr_text );
					dni->oldDesc = ad;
					for ( oldpos=0, a=rs->sr_entry->e_attrs;
						a && a->a_desc != ad; oldpos++, a=a->a_next );
					/* a should not be NULL but apparently it happens.
					 * ITS#7144
					 */
					dni->oldNcount = a ? a->a_numvals : 0;
					for ( newpos=0, a=dni->new_entry->e_attrs;
						a && a->a_desc != ad; newpos++, a=a->a_next );
					if ( !a || oldpos != newpos || attr_valfind( a,
						SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH |
						SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
						SLAP_MR_VALUE_OF_SYNTAX,
						&oldVal, NULL, op->o_tmpmemctx ) != LDAP_SUCCESS )
					{
						dni->delOldRDN = 1;
					}
					/* Get the newRDN's desc */
					dnRdn( &dni->new_entry->e_nname, &oldRDN );
					oldVal.bv_val = strchr(oldRDN.bv_val, '=');
					oldRDN.bv_len = oldVal.bv_val - oldRDN.bv_val;
					ad = NULL;
					slap_bv2ad( &oldRDN, &ad, &rs->sr_text );
					dni->newDesc = ad;

					/* A ModDN has happened, but in Refresh mode other
					 * changes may have occurred before we picked it up.
					 * So fallthru to regular Modify processing.
					 */
				}

				syncrepl_diff_entry( op, rs->sr_entry->e_attrs,
					dni->new_entry->e_attrs, &dni->mods, dni->modlist,
					is_ctx );
			}
		}
	} else if ( rs->sr_type == REP_RESULT ) {
		if ( rs->sr_err == LDAP_SIZELIMIT_EXCEEDED ) {
			Debug( LDAP_DEBUG_ANY,
				"dn_callback : consistency error - "
				"entryUUID is not unique\n", 0, 0, 0 );
		}
	}

	return LDAP_SUCCESS;
}

static int
nonpresent_callback(
	Operation*	op,
	SlapReply*	rs )
{
	syncinfo_t *si = op->o_callback->sc_private;
	Attribute *a;
	int count = 0;
	struct berval* present_uuid = NULL;
	struct nonpresent_entry *np_entry;

	if ( rs->sr_type == REP_RESULT ) {
		count = avl_free( si->si_presentlist, ch_free );
		si->si_presentlist = NULL;

	} else if ( rs->sr_type == REP_SEARCH ) {
		if ( !( si->si_refreshDelete & NP_DELETE_ONE ) ) {
			a = attr_find( rs->sr_entry->e_attrs, slap_schema.si_ad_entryUUID );

			if ( a ) {
				present_uuid = avl_find( si->si_presentlist, &a->a_nvals[0],
					syncuuid_cmp );
			}

			if ( LogTest( LDAP_DEBUG_SYNC ) ) {
				char buf[sizeof("rid=999 non")];

				snprintf( buf, sizeof(buf), "%s %s", si->si_ridtxt,
					present_uuid ? "" : "non" );

				Debug( LDAP_DEBUG_SYNC, "nonpresent_callback: %spresent UUID %s, dn %s\n",
					buf, a ? a->a_vals[0].bv_val : "<missing>", rs->sr_entry->e_name.bv_val );
			}

			if ( a == NULL ) return 0;
		}

		if ( present_uuid == NULL ) {
			np_entry = (struct nonpresent_entry *)
				ch_calloc( 1, sizeof( struct nonpresent_entry ) );
			np_entry->npe_name = ber_dupbv( NULL, &rs->sr_entry->e_name );
			np_entry->npe_nname = ber_dupbv( NULL, &rs->sr_entry->e_nname );
			LDAP_LIST_INSERT_HEAD( &si->si_nonpresentlist, np_entry, npe_link );

		} else {
			avl_delete( &si->si_presentlist,
				&a->a_nvals[0], syncuuid_cmp );
			ch_free( present_uuid );
		}
	}
	return LDAP_SUCCESS;
}

static int
null_callback(
	Operation*	op,
	SlapReply*	rs )
{
	if ( rs->sr_err != LDAP_SUCCESS &&
		rs->sr_err != LDAP_REFERRAL &&
		rs->sr_err != LDAP_ALREADY_EXISTS &&
		rs->sr_err != LDAP_NO_SUCH_OBJECT &&
		rs->sr_err != LDAP_NOT_ALLOWED_ON_NONLEAF )
	{
		Debug( LDAP_DEBUG_ANY,
			"null_callback : error code 0x%x\n",
			rs->sr_err, 0, 0 );
	}
	return LDAP_SUCCESS;
}

static struct berval *
slap_uuidstr_from_normalized(
	struct berval* uuidstr,
	struct berval* normalized,
	void *ctx )
{
#if 0
	struct berval *new;
	unsigned char nibble;
	int i, d = 0;

	if ( normalized == NULL ) return NULL;
	if ( normalized->bv_len != 16 ) return NULL;

	if ( uuidstr ) {
		new = uuidstr;
	} else {
		new = (struct berval *)slap_sl_malloc( sizeof(struct berval), ctx );
		if ( new == NULL ) {
			return NULL;
		}
	}

	new->bv_len = 36;

	if ( ( new->bv_val = slap_sl_malloc( new->bv_len + 1, ctx ) ) == NULL ) {
		if ( new != uuidstr ) {
			slap_sl_free( new, ctx );
		}
		return NULL;
	}

	for ( i = 0; i < 16; i++ ) {
		if ( i == 4 || i == 6 || i == 8 || i == 10 ) {
			new->bv_val[(i<<1)+d] = '-';
			d += 1;
		}

		nibble = (normalized->bv_val[i] >> 4) & 0xF;
		if ( nibble < 10 ) {
			new->bv_val[(i<<1)+d] = nibble + '0';
		} else {
			new->bv_val[(i<<1)+d] = nibble - 10 + 'a';
		}

		nibble = (normalized->bv_val[i]) & 0xF;
		if ( nibble < 10 ) {
			new->bv_val[(i<<1)+d+1] = nibble + '0';
		} else {
			new->bv_val[(i<<1)+d+1] = nibble - 10 + 'a';
		}
	}

	new->bv_val[new->bv_len] = '\0';
	return new;
#endif

	struct berval	*new;
	int		rc = 0;

	if ( normalized == NULL ) return NULL;
	if ( normalized->bv_len != 16 ) return NULL;

	if ( uuidstr ) {
		new = uuidstr;

	} else {
		new = (struct berval *)slap_sl_malloc( sizeof(struct berval), ctx );
		if ( new == NULL ) {
			return NULL;
		}
	}

	new->bv_len = 36;

	if ( ( new->bv_val = slap_sl_malloc( new->bv_len + 1, ctx ) ) == NULL ) {
		rc = 1;
		goto done;
	}

	rc = lutil_uuidstr_from_normalized( normalized->bv_val,
		normalized->bv_len, new->bv_val, new->bv_len + 1 );

done:;
	if ( rc == -1 ) {
		if ( new != NULL ) {
			if ( new->bv_val != NULL ) {
				slap_sl_free( new->bv_val, ctx );
			}

			if ( new != uuidstr ) {
				slap_sl_free( new, ctx );
			}
		}
		new = NULL;

	} else {
		new->bv_len = rc;
	}

	return new;
}

static int
syncuuid_cmp( const void* v_uuid1, const void* v_uuid2 )
{
	const struct berval *uuid1 = v_uuid1;
	const struct berval *uuid2 = v_uuid2;
	int rc = uuid1->bv_len - uuid2->bv_len;
	if ( rc ) return rc;
	return ( memcmp( uuid1->bv_val, uuid2->bv_val, uuid1->bv_len ) );
}

void
syncinfo_free( syncinfo_t *sie, int free_all )
{
	syncinfo_t *si_next;

	Debug( LDAP_DEBUG_TRACE, "syncinfo_free: %s\n",
		sie->si_ridtxt, 0, 0 );

	do {
		si_next = sie->si_next;

		if ( sie->si_ld ) {
			if ( sie->si_conn ) {
				connection_client_stop( sie->si_conn );
				sie->si_conn = NULL;
			}
			ldap_unbind_ext( sie->si_ld, NULL, NULL );
		}
	
		if ( sie->si_re ) {
			struct re_s		*re = sie->si_re;
			sie->si_re = NULL;

			ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
			if ( ldap_pvt_runqueue_isrunning( &slapd_rq, re ) )
				ldap_pvt_runqueue_stoptask( &slapd_rq, re );
			ldap_pvt_runqueue_remove( &slapd_rq, re );
			ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
		}

		ldap_pvt_thread_mutex_destroy( &sie->si_mutex );

		bindconf_free( &sie->si_bindconf );

		if ( sie->si_filterstr.bv_val ) {
			ch_free( sie->si_filterstr.bv_val );
		}
		if ( sie->si_filter ) {
			filter_free( sie->si_filter );
		}
		if ( sie->si_logfilterstr.bv_val ) {
			ch_free( sie->si_logfilterstr.bv_val );
		}
		if ( sie->si_base.bv_val ) {
			ch_free( sie->si_base.bv_val );
		}
		if ( sie->si_logbase.bv_val ) {
			ch_free( sie->si_logbase.bv_val );
		}
		if ( sie->si_be && SLAP_SYNC_SUBENTRY( sie->si_be )) {
			ch_free( sie->si_contextdn.bv_val );
		}
		if ( sie->si_attrs ) {
			int i = 0;
			while ( sie->si_attrs[i] != NULL ) {
				ch_free( sie->si_attrs[i] );
				i++;
			}
			ch_free( sie->si_attrs );
		}
		if ( sie->si_exattrs ) {
			int i = 0;
			while ( sie->si_exattrs[i] != NULL ) {
				ch_free( sie->si_exattrs[i] );
				i++;
			}
			ch_free( sie->si_exattrs );
		}
		if ( sie->si_anlist ) {
			int i = 0;
			while ( sie->si_anlist[i].an_name.bv_val != NULL ) {
				ch_free( sie->si_anlist[i].an_name.bv_val );
				i++;
			}
			ch_free( sie->si_anlist );
		}
		if ( sie->si_exanlist ) {
			int i = 0;
			while ( sie->si_exanlist[i].an_name.bv_val != NULL ) {
				ch_free( sie->si_exanlist[i].an_name.bv_val );
				i++;
			}
			ch_free( sie->si_exanlist );
		}
		if ( sie->si_retryinterval ) {
			ch_free( sie->si_retryinterval );
		}
		if ( sie->si_retrynum ) {
			ch_free( sie->si_retrynum );
		}
		if ( sie->si_retrynum_init ) {
			ch_free( sie->si_retrynum_init );
		}
		slap_sync_cookie_free( &sie->si_syncCookie, 0 );
		if ( sie->si_presentlist ) {
		    avl_free( sie->si_presentlist, ch_free );
		}
		while ( !LDAP_LIST_EMPTY( &sie->si_nonpresentlist ) ) {
			struct nonpresent_entry* npe;
			npe = LDAP_LIST_FIRST( &sie->si_nonpresentlist );
			LDAP_LIST_REMOVE( npe, npe_link );
			if ( npe->npe_name ) {
				if ( npe->npe_name->bv_val ) {
					ch_free( npe->npe_name->bv_val );
				}
				ch_free( npe->npe_name );
			}
			if ( npe->npe_nname ) {
				if ( npe->npe_nname->bv_val ) {
					ch_free( npe->npe_nname->bv_val );
				}
				ch_free( npe->npe_nname );
			}
			ch_free( npe );
		}
		if ( sie->si_cookieState ) {
			sie->si_cookieState->cs_ref--;
			if ( !sie->si_cookieState->cs_ref ) {
				ch_free( sie->si_cookieState->cs_sids );
				ber_bvarray_free( sie->si_cookieState->cs_vals );
				ldap_pvt_thread_mutex_destroy( &sie->si_cookieState->cs_mutex );
				ch_free( sie->si_cookieState->cs_psids );
				ber_bvarray_free( sie->si_cookieState->cs_pvals );
				ldap_pvt_thread_mutex_destroy( &sie->si_cookieState->cs_pmutex );
				ch_free( sie->si_cookieState );
			}
		}
#ifdef ENABLE_REWRITE
		if ( sie->si_rewrite )
			rewrite_info_delete( &sie->si_rewrite );
		if ( sie->si_suffixm.bv_val )
			ch_free( sie->si_suffixm.bv_val );
#endif
		ch_free( sie );
		sie = si_next;
	} while ( free_all && si_next );
}

#ifdef ENABLE_REWRITE
static int
config_suffixm( ConfigArgs *c, syncinfo_t *si )
{
	char *argvEngine[] = { "rewriteEngine", "on", NULL };
	char *argvContext[] = { "rewriteContext", SUFFIXM_CTX, NULL };
	char *argvRule[] = { "rewriteRule", NULL, NULL, ":", NULL };
	char *vnc, *rnc;
	int rc;

	if ( si->si_rewrite )
		rewrite_info_delete( &si->si_rewrite );
	si->si_rewrite = rewrite_info_init( REWRITE_MODE_USE_DEFAULT );

	rc = rewrite_parse( si->si_rewrite, c->fname, c->lineno, 2, argvEngine );
	if ( rc != LDAP_SUCCESS )
		return rc;

	rc = rewrite_parse( si->si_rewrite, c->fname, c->lineno, 2, argvContext );
	if ( rc != LDAP_SUCCESS )
		return rc;

	vnc = ch_malloc( si->si_base.bv_len + 6 );
	strcpy( vnc, "(.*)" );
	lutil_strcopy( lutil_strcopy( vnc+4, si->si_base.bv_val ), "$" );
	argvRule[1] = vnc;

	rnc = ch_malloc( si->si_suffixm.bv_len + 3 );
	strcpy( rnc, "%1" );
	strcpy( rnc+2, si->si_suffixm.bv_val );
	argvRule[2] = rnc;

	rc = rewrite_parse( si->si_rewrite, c->fname, c->lineno, 4, argvRule );
	ch_free( vnc );
	ch_free( rnc );
	return rc;
}
#endif

/* NOTE: used & documented in slapd.conf(5) */
#define IDSTR			"rid"
#define PROVIDERSTR		"provider"
#define SCHEMASTR		"schemachecking"
#define FILTERSTR		"filter"
#define SEARCHBASESTR		"searchbase"
#define SCOPESTR		"scope"
#define ATTRSONLYSTR		"attrsonly"
#define ATTRSSTR		"attrs"
#define TYPESTR			"type"
#define INTERVALSTR		"interval"
#define RETRYSTR		"retry"
#define SLIMITSTR		"sizelimit"
#define TLIMITSTR		"timelimit"
#define SYNCDATASTR		"syncdata"
#define LOGBASESTR		"logbase"
#define LOGFILTERSTR	"logfilter"
#define SUFFIXMSTR		"suffixmassage"
#define	STRICT_REFRESH	"strictrefresh"

/* FIXME: undocumented */
#define EXATTRSSTR		"exattrs"
#define MANAGEDSAITSTR		"manageDSAit"

/* mandatory */
enum {
	GOT_RID			= 0x00000001U,
	GOT_PROVIDER		= 0x00000002U,
	GOT_SCHEMACHECKING	= 0x00000004U,
	GOT_FILTER		= 0x00000008U,
	GOT_SEARCHBASE		= 0x00000010U,
	GOT_SCOPE		= 0x00000020U,
	GOT_ATTRSONLY		= 0x00000040U,
	GOT_ATTRS		= 0x00000080U,
	GOT_TYPE		= 0x00000100U,
	GOT_INTERVAL		= 0x00000200U,
	GOT_RETRY		= 0x00000400U,
	GOT_SLIMIT		= 0x00000800U,
	GOT_TLIMIT		= 0x00001000U,
	GOT_SYNCDATA		= 0x00002000U,
	GOT_LOGBASE		= 0x00004000U,
	GOT_LOGFILTER		= 0x00008000U,
	GOT_EXATTRS		= 0x00010000U,
	GOT_MANAGEDSAIT		= 0x00020000U,
	GOT_BINDCONF		= 0x00040000U,
	GOT_SUFFIXM		= 0x00080000U,

/* check */
	GOT_REQUIRED		= (GOT_RID|GOT_PROVIDER|GOT_SEARCHBASE)
};

static slap_verbmasks datamodes[] = {
	{ BER_BVC("default"), SYNCDATA_DEFAULT },
	{ BER_BVC("accesslog"), SYNCDATA_ACCESSLOG },
	{ BER_BVC("changelog"), SYNCDATA_CHANGELOG },
	{ BER_BVNULL, 0 }
};

static int
parse_syncrepl_retry(
	ConfigArgs	*c,
	char		*arg,
	syncinfo_t	*si )
{
	char **retry_list;
	int j, k, n;
	int use_default = 0;

	char *val = arg + STRLENOF( RETRYSTR "=" );
	if ( strcasecmp( val, "undefined" ) == 0 ) {
		val = "3600 +";
		use_default = 1;
	}

	retry_list = (char **) ch_calloc( 1, sizeof( char * ) );
	retry_list[0] = NULL;

	slap_str2clist( &retry_list, val, " ,\t" );

	for ( k = 0; retry_list && retry_list[k]; k++ ) ;
	n = k / 2;
	if ( k % 2 ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			"Error: incomplete syncrepl retry list" );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		for ( k = 0; retry_list && retry_list[k]; k++ ) {
			ch_free( retry_list[k] );
		}
		ch_free( retry_list );
		return 1;
	}
	si->si_retryinterval = (time_t *) ch_calloc( n + 1, sizeof( time_t ) );
	si->si_retrynum = (int *) ch_calloc( n + 1, sizeof( int ) );
	si->si_retrynum_init = (int *) ch_calloc( n + 1, sizeof( int ) );
	for ( j = 0; j < n; j++ ) {
		unsigned long	t;
		if ( lutil_atoul( &t, retry_list[j*2] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"Error: invalid retry interval \"%s\" (#%d)",
				retry_list[j*2], j );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			/* do some cleanup */
			return 1;
		}
		si->si_retryinterval[j] = (time_t)t;
		if ( *retry_list[j*2+1] == '+' ) {
			si->si_retrynum_init[j] = RETRYNUM_FOREVER;
			si->si_retrynum[j] = RETRYNUM_FOREVER;
			j++;
			break;
		} else {
			if ( lutil_atoi( &si->si_retrynum_init[j], retry_list[j*2+1] ) != 0
					|| si->si_retrynum_init[j] <= 0 )
			{
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"Error: invalid initial retry number \"%s\" (#%d)",
					retry_list[j*2+1], j );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				/* do some cleanup */
				return 1;
			}
			if ( lutil_atoi( &si->si_retrynum[j], retry_list[j*2+1] ) != 0
					|| si->si_retrynum[j] <= 0 )
			{
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"Error: invalid retry number \"%s\" (#%d)",
					retry_list[j*2+1], j );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				/* do some cleanup */
				return 1;
			}
		}
	}
	if ( j < 1 || si->si_retrynum_init[j-1] != RETRYNUM_FOREVER ) {
		Debug( LDAP_DEBUG_CONFIG,
			"%s: syncrepl will eventually stop retrying; the \"retry\" parameter should end with a '+'.\n",
			c->log, 0, 0 );
	}

	si->si_retrynum_init[j] = RETRYNUM_TAIL;
	si->si_retrynum[j] = RETRYNUM_TAIL;
	si->si_retryinterval[j] = 0;
	
	for ( k = 0; retry_list && retry_list[k]; k++ ) {
		ch_free( retry_list[k] );
	}
	ch_free( retry_list );
	if ( !use_default ) {
		si->si_got |= GOT_RETRY;
	}

	return 0;
}

static int
parse_syncrepl_line(
	ConfigArgs	*c,
	syncinfo_t	*si )
{
	int	i;
	char	*val;

	for ( i = 1; i < c->argc; i++ ) {
		if ( !strncasecmp( c->argv[ i ], IDSTR "=",
					STRLENOF( IDSTR "=" ) ) )
		{
			int tmp;
			/* '\0' string terminator accounts for '=' */
			val = c->argv[ i ] + STRLENOF( IDSTR "=" );
			if ( lutil_atoi( &tmp, val ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"Error: parse_syncrepl_line: "
					"unable to parse syncrepl id \"%s\"", val );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return -1;
			}
			if ( tmp > SLAP_SYNC_RID_MAX || tmp < 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"Error: parse_syncrepl_line: "
					"syncrepl id %d is out of range [0..%d]", tmp, SLAP_SYNC_RID_MAX );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return -1;
			}
			si->si_rid = tmp;
			sprintf( si->si_ridtxt, IDSTR "=%03d", si->si_rid );
			si->si_got |= GOT_RID;
		} else if ( !strncasecmp( c->argv[ i ], PROVIDERSTR "=",
					STRLENOF( PROVIDERSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( PROVIDERSTR "=" );
			ber_str2bv( val, 0, 1, &si->si_bindconf.sb_uri );
#ifdef HAVE_TLS
			if ( ldap_is_ldaps_url( val ))
				si->si_bindconf.sb_tls_do_init = 1;
#endif
			si->si_got |= GOT_PROVIDER;
		} else if ( !strncasecmp( c->argv[ i ], SCHEMASTR "=",
					STRLENOF( SCHEMASTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( SCHEMASTR "=" );
			if ( !strncasecmp( val, "on", STRLENOF( "on" ) ) ) {
				si->si_schemachecking = 1;
			} else if ( !strncasecmp( val, "off", STRLENOF( "off" ) ) ) {
				si->si_schemachecking = 0;
			} else {
				si->si_schemachecking = 1;
			}
			si->si_got |= GOT_SCHEMACHECKING;
		} else if ( !strncasecmp( c->argv[ i ], FILTERSTR "=",
					STRLENOF( FILTERSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( FILTERSTR "=" );
			if ( si->si_filterstr.bv_val )
				ch_free( si->si_filterstr.bv_val );
			ber_str2bv( val, 0, 1, &si->si_filterstr );
			si->si_got |= GOT_FILTER;
		} else if ( !strncasecmp( c->argv[ i ], LOGFILTERSTR "=",
					STRLENOF( LOGFILTERSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( LOGFILTERSTR "=" );
			if ( si->si_logfilterstr.bv_val )
				ch_free( si->si_logfilterstr.bv_val );
			ber_str2bv( val, 0, 1, &si->si_logfilterstr );
			si->si_got |= GOT_LOGFILTER;
		} else if ( !strncasecmp( c->argv[ i ], SEARCHBASESTR "=",
					STRLENOF( SEARCHBASESTR "=" ) ) )
		{
			struct berval	bv;
			int		rc;

			val = c->argv[ i ] + STRLENOF( SEARCHBASESTR "=" );
			if ( si->si_base.bv_val ) {
				ch_free( si->si_base.bv_val );
			}
			ber_str2bv( val, 0, 0, &bv );
			rc = dnNormalize( 0, NULL, NULL, &bv, &si->si_base, NULL );
			if ( rc != LDAP_SUCCESS ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"Invalid base DN \"%s\": %d (%s)",
					val, rc, ldap_err2string( rc ) );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return -1;
			}
			si->si_got |= GOT_SEARCHBASE;
#ifdef ENABLE_REWRITE
		} else if ( !strncasecmp( c->argv[ i ], SUFFIXMSTR "=",
					STRLENOF( SUFFIXMSTR "=" ) ) )
		{
			struct berval	bv;
			int		rc;

			val = c->argv[ i ] + STRLENOF( SUFFIXMSTR "=" );
			if ( si->si_suffixm.bv_val ) {
				ch_free( si->si_suffixm.bv_val );
			}
			ber_str2bv( val, 0, 0, &bv );
			rc = dnNormalize( 0, NULL, NULL, &bv, &si->si_suffixm, NULL );
			if ( rc != LDAP_SUCCESS ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"Invalid massage DN \"%s\": %d (%s)",
					val, rc, ldap_err2string( rc ) );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return -1;
			}
			if ( !be_issubordinate( c->be, &si->si_suffixm )) {
				ch_free( si->si_suffixm.bv_val );
				BER_BVZERO( &si->si_suffixm );
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"Massage DN \"%s\" is not within the database naming context",
					val );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return -1;
			}
			si->si_got |= GOT_SUFFIXM;
#endif
		} else if ( !strncasecmp( c->argv[ i ], LOGBASESTR "=",
					STRLENOF( LOGBASESTR "=" ) ) )
		{
			struct berval	bv;
			int		rc;

			val = c->argv[ i ] + STRLENOF( LOGBASESTR "=" );
			if ( si->si_logbase.bv_val ) {
				ch_free( si->si_logbase.bv_val );
			}
			ber_str2bv( val, 0, 0, &bv );
			rc = dnNormalize( 0, NULL, NULL, &bv, &si->si_logbase, NULL );
			if ( rc != LDAP_SUCCESS ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"Invalid logbase DN \"%s\": %d (%s)",
					val, rc, ldap_err2string( rc ) );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return -1;
			}
			si->si_got |= GOT_LOGBASE;
		} else if ( !strncasecmp( c->argv[ i ], SCOPESTR "=",
					STRLENOF( SCOPESTR "=" ) ) )
		{
			int j;
			val = c->argv[ i ] + STRLENOF( SCOPESTR "=" );
			j = ldap_pvt_str2scope( val );
			if ( j < 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"Error: parse_syncrepl_line: "
					"unknown scope \"%s\"", val);
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return -1;
			}
			si->si_scope = j;
			si->si_got |= GOT_SCOPE;
		} else if ( !strncasecmp( c->argv[ i ], ATTRSONLYSTR,
					STRLENOF( ATTRSONLYSTR ) ) )
		{
			si->si_attrsonly = 1;
			si->si_got |= GOT_ATTRSONLY;
		} else if ( !strncasecmp( c->argv[ i ], ATTRSSTR "=",
					STRLENOF( ATTRSSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( ATTRSSTR "=" );
			if ( !strncasecmp( val, ":include:", STRLENOF(":include:") ) ) {
				char *attr_fname;
				attr_fname = ch_strdup( val + STRLENOF(":include:") );
				si->si_anlist = file2anlist( si->si_anlist, attr_fname, " ,\t" );
				if ( si->si_anlist == NULL ) {
					ch_free( attr_fname );
					return -1;
				}
				si->si_anfile = attr_fname;
			} else {
				char *str, *s, *next;
				const char *delimstr = " ,\t";
				str = ch_strdup( val );
				for ( s = ldap_pvt_strtok( str, delimstr, &next );
						s != NULL;
						s = ldap_pvt_strtok( NULL, delimstr, &next ) )
				{
					if ( strlen(s) == 1 && *s == '*' ) {
						si->si_allattrs = 1;
						val[ s - str ] = delimstr[0];
					}
					if ( strlen(s) == 1 && *s == '+' ) {
						si->si_allopattrs = 1;
						val [ s - str ] = delimstr[0];
					}
				}
				ch_free( str );
				si->si_anlist = str2anlist( si->si_anlist, val, " ,\t" );
				if ( si->si_anlist == NULL ) {
					return -1;
				}
			}
			si->si_got |= GOT_ATTRS;
		} else if ( !strncasecmp( c->argv[ i ], EXATTRSSTR "=",
					STRLENOF( EXATTRSSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( EXATTRSSTR "=" );
			if ( !strncasecmp( val, ":include:", STRLENOF(":include:") ) ) {
				char *attr_fname;
				attr_fname = ch_strdup( val + STRLENOF(":include:") );
				si->si_exanlist = file2anlist(
					si->si_exanlist, attr_fname, " ,\t" );
				if ( si->si_exanlist == NULL ) {
					ch_free( attr_fname );
					return -1;
				}
				ch_free( attr_fname );
			} else {
				si->si_exanlist = str2anlist( si->si_exanlist, val, " ,\t" );
				if ( si->si_exanlist == NULL ) {
					return -1;
				}
			}
			si->si_got |= GOT_EXATTRS;
		} else if ( !strncasecmp( c->argv[ i ], TYPESTR "=",
					STRLENOF( TYPESTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( TYPESTR "=" );
			if ( !strncasecmp( val, "refreshOnly",
						STRLENOF("refreshOnly") ) )
			{
				si->si_type = si->si_ctype = LDAP_SYNC_REFRESH_ONLY;
			} else if ( !strncasecmp( val, "refreshAndPersist",
						STRLENOF("refreshAndPersist") ) )
			{
				si->si_type = si->si_ctype = LDAP_SYNC_REFRESH_AND_PERSIST;
				si->si_interval = 60;
			} else {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"Error: parse_syncrepl_line: "
					"unknown sync type \"%s\"", val);
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return -1;
			}
			si->si_got |= GOT_TYPE;
		} else if ( !strncasecmp( c->argv[ i ], INTERVALSTR "=",
					STRLENOF( INTERVALSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( INTERVALSTR "=" );
			if ( si->si_type == LDAP_SYNC_REFRESH_AND_PERSIST ) {
				si->si_interval = 0;
			} else if ( strchr( val, ':' ) != NULL ) {
				char *next, *ptr = val;
				int dd, hh, mm, ss;

				dd = strtol( ptr, &next, 10 );
				if ( next == ptr || next[0] != ':' || dd < 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"Error: parse_syncrepl_line: "
						"invalid interval \"%s\", unable to parse days", val );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					return -1;
				}
				ptr = next + 1;
				hh = strtol( ptr, &next, 10 );
				if ( next == ptr || next[0] != ':' || hh < 0 || hh > 24 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"Error: parse_syncrepl_line: "
						"invalid interval \"%s\", unable to parse hours", val );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					return -1;
				}
				ptr = next + 1;
				mm = strtol( ptr, &next, 10 );
				if ( next == ptr || next[0] != ':' || mm < 0 || mm > 60 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"Error: parse_syncrepl_line: "
						"invalid interval \"%s\", unable to parse minutes", val );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					return -1;
				}
				ptr = next + 1;
				ss = strtol( ptr, &next, 10 );
				if ( next == ptr || next[0] != '\0' || ss < 0 || ss > 60 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"Error: parse_syncrepl_line: "
						"invalid interval \"%s\", unable to parse seconds", val );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					return -1;
				}
				si->si_interval = (( dd * 24 + hh ) * 60 + mm ) * 60 + ss;
			} else {
				unsigned long	t;

				if ( lutil_parse_time( val, &t ) != 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"Error: parse_syncrepl_line: "
						"invalid interval \"%s\"", val );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					return -1;
				}
				si->si_interval = (time_t)t;
			}
			if ( si->si_interval < 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"Error: parse_syncrepl_line: "
					"invalid interval \"%ld\"",
					(long) si->si_interval);
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return -1;
			}
			si->si_got |= GOT_INTERVAL;
		} else if ( !strncasecmp( c->argv[ i ], RETRYSTR "=",
					STRLENOF( RETRYSTR "=" ) ) )
		{
			if ( parse_syncrepl_retry( c, c->argv[ i ], si ) ) {
				return 1;
			}
		} else if ( !strncasecmp( c->argv[ i ], MANAGEDSAITSTR "=",
					STRLENOF( MANAGEDSAITSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( MANAGEDSAITSTR "=" );
			if ( lutil_atoi( &si->si_manageDSAit, val ) != 0
				|| si->si_manageDSAit < 0 || si->si_manageDSAit > 1 )
			{
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"invalid manageDSAit value \"%s\".\n",
					val );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
			si->si_got |= GOT_MANAGEDSAIT;
		} else if ( !strncasecmp( c->argv[ i ], SLIMITSTR "=",
					STRLENOF( SLIMITSTR "=") ) )
		{
			val = c->argv[ i ] + STRLENOF( SLIMITSTR "=" );
			if ( strcasecmp( val, "unlimited" ) == 0 ) {
				si->si_slimit = 0;

			} else if ( lutil_atoi( &si->si_slimit, val ) != 0 || si->si_slimit < 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"invalid size limit value \"%s\".\n",
					val );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
			si->si_got |= GOT_SLIMIT;
		} else if ( !strncasecmp( c->argv[ i ], TLIMITSTR "=",
					STRLENOF( TLIMITSTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( TLIMITSTR "=" );
			if ( strcasecmp( val, "unlimited" ) == 0 ) {
				si->si_tlimit = 0;

			} else if ( lutil_atoi( &si->si_tlimit, val ) != 0 || si->si_tlimit < 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"invalid time limit value \"%s\".\n",
					val );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
			si->si_got |= GOT_TLIMIT;
		} else if ( !strncasecmp( c->argv[ i ], SYNCDATASTR "=",
					STRLENOF( SYNCDATASTR "=" ) ) )
		{
			val = c->argv[ i ] + STRLENOF( SYNCDATASTR "=" );
			si->si_syncdata = verb_to_mask( val, datamodes );
			si->si_got |= GOT_SYNCDATA;
		} else if ( !strncasecmp( c->argv[ i ], STRICT_REFRESH,
					STRLENOF( STRICT_REFRESH ) ) )
		{
			si->si_strict_refresh = 1;
		} else if ( !bindconf_parse( c->argv[i], &si->si_bindconf ) ) {
			si->si_got |= GOT_BINDCONF;
		} else {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"Error: parse_syncrepl_line: "
				"unable to parse \"%s\"\n", c->argv[ i ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return -1;
		}
	}

	if ( ( si->si_got & GOT_REQUIRED ) != GOT_REQUIRED ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			"Error: Malformed \"syncrepl\" line in slapd config file, missing%s%s%s",
			si->si_got & GOT_RID ? "" : " "IDSTR,
			si->si_got & GOT_PROVIDER ? "" : " "PROVIDERSTR,
			si->si_got & GOT_SEARCHBASE ? "" : " "SEARCHBASESTR );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		return -1;
	}

	if ( !be_issubordinate( c->be, &si->si_base ) && !( si->si_got & GOT_SUFFIXM )) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			"Base DN \"%s\" is not within the database naming context",
			si->si_base.bv_val );
		ch_free( si->si_base.bv_val );
		BER_BVZERO( &si->si_base );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		return -1;
	}

#ifdef ENABLE_REWRITE
	if ( si->si_got & GOT_SUFFIXM ) {
		if (config_suffixm( c, si )) {
			ch_free( si->si_suffixm.bv_val );
			BER_BVZERO( &si->si_suffixm );
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"Error configuring rewrite engine" );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return -1;
		}
	}
#endif

	if ( !( si->si_got & GOT_RETRY ) ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl %s " SEARCHBASESTR "=\"%s\": no retry defined, using default\n", 
			si->si_ridtxt, c->be->be_suffix ? c->be->be_suffix[ 0 ].bv_val : "(null)", 0 );
		if ( si->si_retryinterval == NULL ) {
			if ( parse_syncrepl_retry( c, "retry=undefined", si ) ) {
				return 1;
			}
		}
	}

	si->si_filter = str2filter( si->si_filterstr.bv_val );
	if ( si->si_filter == NULL ) {
		Debug( LDAP_DEBUG_ANY, "syncrepl %s " SEARCHBASESTR "=\"%s\": unable to parse filter=\"%s\"\n", 
			si->si_ridtxt, c->be->be_suffix ? c->be->be_suffix[ 0 ].bv_val : "(null)", si->si_filterstr.bv_val );
		return 1;
	}

	return 0;
}

static int
add_syncrepl(
	ConfigArgs *c )
{
	syncinfo_t *si;
	int	rc = 0;

	if ( !( c->be->be_search && c->be->be_add && c->be->be_modify && c->be->be_delete ) ) {
		snprintf( c->cr_msg, sizeof(c->cr_msg), "database %s does not support "
			"operations required for syncrepl", c->be->be_type );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg, 0 );
		return 1;
	}
	if ( BER_BVISEMPTY( &c->be->be_rootdn ) ) {
		strcpy( c->cr_msg, "rootDN must be defined before syncrepl may be used" );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg, 0 );
		return 1;
	}
	si = (syncinfo_t *) ch_calloc( 1, sizeof( syncinfo_t ) );

	if ( si == NULL ) {
		Debug( LDAP_DEBUG_ANY, "out of memory in add_syncrepl\n", 0, 0, 0 );
		return 1;
	}

	si->si_bindconf.sb_tls = SB_TLS_OFF;
	si->si_bindconf.sb_method = LDAP_AUTH_SIMPLE;
	si->si_schemachecking = 0;
	ber_str2bv( "(objectclass=*)", STRLENOF("(objectclass=*)"), 1,
		&si->si_filterstr );
	si->si_base.bv_val = NULL;
	si->si_scope = LDAP_SCOPE_SUBTREE;
	si->si_attrsonly = 0;
	si->si_anlist = (AttributeName *) ch_calloc( 1, sizeof( AttributeName ) );
	si->si_exanlist = (AttributeName *) ch_calloc( 1, sizeof( AttributeName ) );
	si->si_attrs = NULL;
	si->si_allattrs = 0;
	si->si_allopattrs = 0;
	si->si_exattrs = NULL;
	si->si_type = si->si_ctype = LDAP_SYNC_REFRESH_ONLY;
	si->si_interval = 86400;
	si->si_retryinterval = NULL;
	si->si_retrynum_init = NULL;
	si->si_retrynum = NULL;
	si->si_manageDSAit = 0;
	si->si_tlimit = 0;
	si->si_slimit = 0;

	si->si_presentlist = NULL;
	LDAP_LIST_INIT( &si->si_nonpresentlist );
	ldap_pvt_thread_mutex_init( &si->si_mutex );

	rc = parse_syncrepl_line( c, si );

	if ( rc == 0 ) {
		LDAPURLDesc *lud;

		/* Must be LDAPv3 because we need controls */
		switch ( si->si_bindconf.sb_version ) {
		case 0:
			/* not explicitly set */
			si->si_bindconf.sb_version = LDAP_VERSION3;
			break;
		case 3:
			/* explicitly set */
			break;
		default:
			Debug( LDAP_DEBUG_ANY,
				"version %d incompatible with syncrepl\n",
				si->si_bindconf.sb_version, 0, 0 );
			syncinfo_free( si, 0 );	
			return 1;
		}

		if ( ldap_url_parse( si->si_bindconf.sb_uri.bv_val, &lud )) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"<%s> invalid URL", c->argv[0] );
			Debug( LDAP_DEBUG_ANY, "%s: %s %s\n",
				c->log, c->cr_msg, si->si_bindconf.sb_uri.bv_val );
			return 1;
		}

		si->si_be = c->be;
		if ( slapMode & SLAP_SERVER_MODE ) {
			int isMe = 0;
			/* check if consumer points to current server and database.
			 * If so, ignore this configuration.
			 */
			if ( !SLAP_DBHIDDEN( c->be ) ) {
				int i;
				/* if searchbase doesn't match current DB suffix,
				 * assume it's different
				 */
				for ( i=0; !BER_BVISNULL( &c->be->be_nsuffix[i] ); i++ ) {
					if ( bvmatch( &si->si_base, &c->be->be_nsuffix[i] )) {
						isMe = 1;
						break;
					}
				}
				/* if searchbase matches, see if URLs match */
				if ( isMe && config_check_my_url( si->si_bindconf.sb_uri.bv_val,
						lud ) == NULL )
					isMe = 0;
			}

			if ( !isMe ) {
				init_syncrepl( si );
				ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
				si->si_re = ldap_pvt_runqueue_insert( &slapd_rq,
					si->si_interval, do_syncrepl, si, "do_syncrepl",
					si->si_ridtxt );
				ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
				if ( si->si_re )
					rc = config_sync_shadow( c ) ? -1 : 0;
				else
					rc = -1;
			}
		} else {
			/* mirrormode still needs to see this flag in tool mode */
			rc = config_sync_shadow( c ) ? -1 : 0;
		}
		ldap_free_urldesc( lud );
	}

#ifdef HAVE_TLS
	/* Use main slapd defaults */
	bindconf_tls_defaults( &si->si_bindconf );
#endif
	if ( rc < 0 ) {
		Debug( LDAP_DEBUG_ANY, "failed to add syncinfo\n", 0, 0, 0 );
		syncinfo_free( si, 0 );	
		return 1;
	} else {
		Debug( LDAP_DEBUG_CONFIG,
			"Config: ** successfully added syncrepl %s \"%s\"\n",
			si->si_ridtxt,
			BER_BVISNULL( &si->si_bindconf.sb_uri ) ?
			"(null)" : si->si_bindconf.sb_uri.bv_val, 0 );
		if ( c->be->be_syncinfo ) {
			syncinfo_t *sip;

			si->si_cookieState = c->be->be_syncinfo->si_cookieState;

			/* add new syncrepl to end of list (same order as when deleting) */
			for ( sip = c->be->be_syncinfo; sip->si_next; sip = sip->si_next );
			sip->si_next = si;
		} else {
			si->si_cookieState = ch_calloc( 1, sizeof( cookie_state ));
			ldap_pvt_thread_mutex_init( &si->si_cookieState->cs_mutex );
			ldap_pvt_thread_mutex_init( &si->si_cookieState->cs_pmutex );

			c->be->be_syncinfo = si;
		}
		si->si_cookieState->cs_ref++;

		si->si_next = NULL;

		return 0;
	}
}

static void
syncrepl_unparse( syncinfo_t *si, struct berval *bv )
{
	struct berval bc, uri, bs;
	char buf[BUFSIZ*2], *ptr;
	ber_len_t len;
	int i;
#	define WHATSLEFT	((ber_len_t) (&buf[sizeof( buf )] - ptr))

	BER_BVZERO( bv );

	/* temporarily inhibit bindconf from printing URI */
	uri = si->si_bindconf.sb_uri;
	BER_BVZERO( &si->si_bindconf.sb_uri );
	si->si_bindconf.sb_version = 0;
	bindconf_unparse( &si->si_bindconf, &bc );
	si->si_bindconf.sb_uri = uri;
	si->si_bindconf.sb_version = LDAP_VERSION3;

	ptr = buf;
	assert( si->si_rid >= 0 && si->si_rid <= SLAP_SYNC_RID_MAX );
	len = snprintf( ptr, WHATSLEFT, IDSTR "=%03d " PROVIDERSTR "=%s",
		si->si_rid, si->si_bindconf.sb_uri.bv_val );
	if ( len >= sizeof( buf ) ) return;
	ptr += len;
	if ( !BER_BVISNULL( &bc ) ) {
		if ( WHATSLEFT <= bc.bv_len ) {
			free( bc.bv_val );
			return;
		}
		ptr = lutil_strcopy( ptr, bc.bv_val );
		free( bc.bv_val );
	}
	if ( !BER_BVISEMPTY( &si->si_filterstr ) ) {
		if ( WHATSLEFT <= STRLENOF( " " FILTERSTR "=\"" "\"" ) + si->si_filterstr.bv_len ) return;
		ptr = lutil_strcopy( ptr, " " FILTERSTR "=\"" );
		ptr = lutil_strcopy( ptr, si->si_filterstr.bv_val );
		*ptr++ = '"';
	}
	if ( !BER_BVISNULL( &si->si_base ) ) {
		if ( WHATSLEFT <= STRLENOF( " " SEARCHBASESTR "=\"" "\"" ) + si->si_base.bv_len ) return;
		ptr = lutil_strcopy( ptr, " " SEARCHBASESTR "=\"" );
		ptr = lutil_strcopy( ptr, si->si_base.bv_val );
		*ptr++ = '"';
	}
#ifdef ENABLE_REWRITE
	if ( !BER_BVISNULL( &si->si_suffixm ) ) {
		if ( WHATSLEFT <= STRLENOF( " " SUFFIXMSTR "=\"" "\"" ) + si->si_suffixm.bv_len ) return;
		ptr = lutil_strcopy( ptr, " " SUFFIXMSTR "=\"" );
		ptr = lutil_strcopy( ptr, si->si_suffixm.bv_val );
		*ptr++ = '"';
	}
#endif
	if ( !BER_BVISEMPTY( &si->si_logfilterstr ) ) {
		if ( WHATSLEFT <= STRLENOF( " " LOGFILTERSTR "=\"" "\"" ) + si->si_logfilterstr.bv_len ) return;
		ptr = lutil_strcopy( ptr, " " LOGFILTERSTR "=\"" );
		ptr = lutil_strcopy( ptr, si->si_logfilterstr.bv_val );
		*ptr++ = '"';
	}
	if ( !BER_BVISNULL( &si->si_logbase ) ) {
		if ( WHATSLEFT <= STRLENOF( " " LOGBASESTR "=\"" "\"" ) + si->si_logbase.bv_len ) return;
		ptr = lutil_strcopy( ptr, " " LOGBASESTR "=\"" );
		ptr = lutil_strcopy( ptr, si->si_logbase.bv_val );
		*ptr++ = '"';
	}
	if ( ldap_pvt_scope2bv( si->si_scope, &bs ) == LDAP_SUCCESS ) {
		if ( WHATSLEFT <= STRLENOF( " " SCOPESTR "=" ) + bs.bv_len ) return;
		ptr = lutil_strcopy( ptr, " " SCOPESTR "=" );
		ptr = lutil_strcopy( ptr, bs.bv_val );
	}
	if ( si->si_attrsonly ) {
		if ( WHATSLEFT <= STRLENOF( " " ATTRSONLYSTR "=\"" "\"" ) ) return;
		ptr = lutil_strcopy( ptr, " " ATTRSONLYSTR );
	}
	if ( si->si_anfile ) {
		if ( WHATSLEFT <= STRLENOF( " " ATTRSSTR "=\":include:" "\"" ) + strlen( si->si_anfile ) ) return;
		ptr = lutil_strcopy( ptr, " " ATTRSSTR "=:include:\"" );
		ptr = lutil_strcopy( ptr, si->si_anfile );
		*ptr++ = '"';
	} else if ( si->si_allattrs || si->si_allopattrs ||
		( si->si_anlist && !BER_BVISNULL(&si->si_anlist[0].an_name) ) )
	{
		char *old;

		if ( WHATSLEFT <= STRLENOF( " " ATTRSONLYSTR "=\"" "\"" ) ) return;
		ptr = lutil_strcopy( ptr, " " ATTRSSTR "=\"" );
		old = ptr;
		ptr = anlist_unparse( si->si_anlist, ptr, WHATSLEFT );
		if ( ptr == NULL ) return;
		if ( si->si_allattrs ) {
			if ( WHATSLEFT <= STRLENOF( ",*\"" ) ) return;
			if ( old != ptr ) *ptr++ = ',';
			*ptr++ = '*';
		}
		if ( si->si_allopattrs ) {
			if ( WHATSLEFT <= STRLENOF( ",+\"" ) ) return;
			if ( old != ptr ) *ptr++ = ',';
			*ptr++ = '+';
		}
		*ptr++ = '"';
	}
	if ( si->si_exanlist && !BER_BVISNULL(&si->si_exanlist[0].an_name) ) {
		if ( WHATSLEFT <= STRLENOF( " " EXATTRSSTR "=" ) ) return;
		ptr = lutil_strcopy( ptr, " " EXATTRSSTR "=" );
		ptr = anlist_unparse( si->si_exanlist, ptr, WHATSLEFT );
		if ( ptr == NULL ) return;
	}
	if ( WHATSLEFT <= STRLENOF( " " SCHEMASTR "=" ) + STRLENOF( "off" ) ) return;
	ptr = lutil_strcopy( ptr, " " SCHEMASTR "=" );
	ptr = lutil_strcopy( ptr, si->si_schemachecking ? "on" : "off" );
	
	if ( WHATSLEFT <= STRLENOF( " " TYPESTR "=" ) + STRLENOF( "refreshAndPersist" ) ) return;
	ptr = lutil_strcopy( ptr, " " TYPESTR "=" );
	ptr = lutil_strcopy( ptr, si->si_type == LDAP_SYNC_REFRESH_AND_PERSIST ?
		"refreshAndPersist" : "refreshOnly" );

	if ( si->si_type == LDAP_SYNC_REFRESH_ONLY ) {
		int dd, hh, mm, ss;

		dd = si->si_interval;
		ss = dd % 60;
		dd /= 60;
		mm = dd % 60;
		dd /= 60;
		hh = dd % 24;
		dd /= 24;
		len = snprintf( ptr, WHATSLEFT, " %s=%02d:%02d:%02d:%02d",
			INTERVALSTR, dd, hh, mm, ss );
		if ( len >= WHATSLEFT ) return;
		ptr += len;
	}

	if ( si->si_got & GOT_RETRY ) {
		const char *space = "";
		if ( WHATSLEFT <= STRLENOF( " " RETRYSTR "=\"" "\"" ) ) return;
		ptr = lutil_strcopy( ptr, " " RETRYSTR "=\"" );
		for (i=0; si->si_retryinterval[i]; i++) {
			len = snprintf( ptr, WHATSLEFT, "%s%ld ", space,
				(long) si->si_retryinterval[i] );
			space = " ";
			if ( WHATSLEFT - 1 <= len ) return;
			ptr += len;
			if ( si->si_retrynum_init[i] == RETRYNUM_FOREVER )
				*ptr++ = '+';
			else {
				len = snprintf( ptr, WHATSLEFT, "%d", si->si_retrynum_init[i] );
				if ( WHATSLEFT <= len ) return;
				ptr += len;
			}
		}
		if ( WHATSLEFT <= STRLENOF( "\"" ) ) return;
		*ptr++ = '"';
	} else {
		ptr = lutil_strcopy( ptr, " " RETRYSTR "=undefined" );
	}

	if ( si->si_slimit ) {
		len = snprintf( ptr, WHATSLEFT, " " SLIMITSTR "=%d", si->si_slimit );
		if ( WHATSLEFT <= len ) return;
		ptr += len;
	}

	if ( si->si_tlimit ) {
		len = snprintf( ptr, WHATSLEFT, " " TLIMITSTR "=%d", si->si_tlimit );
		if ( WHATSLEFT <= len ) return;
		ptr += len;
	}

	if ( si->si_syncdata ) {
		if ( enum_to_verb( datamodes, si->si_syncdata, &bc ) >= 0 ) {
			if ( WHATSLEFT <= STRLENOF( " " SYNCDATASTR "=" ) + bc.bv_len ) return;
			ptr = lutil_strcopy( ptr, " " SYNCDATASTR "=" );
			ptr = lutil_strcopy( ptr, bc.bv_val );
		}
	}
	bc.bv_len = ptr - buf;
	bc.bv_val = buf;
	ber_dupbv( bv, &bc );
}

int
syncrepl_config( ConfigArgs *c )
{
	if (c->op == SLAP_CONFIG_EMIT) {
		if ( c->be->be_syncinfo ) {
			struct berval bv;
			syncinfo_t *si;

			for ( si = c->be->be_syncinfo; si; si=si->si_next ) {
				syncrepl_unparse( si, &bv ); 
				ber_bvarray_add( &c->rvalue_vals, &bv );
			}
			return 0;
		}
		return 1;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		int isrunning = 0;
		if ( c->be->be_syncinfo ) {
			syncinfo_t *si, **sip;
			int i;

			for ( sip = &c->be->be_syncinfo, i=0; *sip; i++ ) {
				si = *sip;
				if ( c->valx == -1 || i == c->valx ) {
					*sip = si->si_next;
					si->si_ctype = -1;
					si->si_next = NULL;
					/* If the task is currently active, we have to leave
					 * it running. It will exit on its own. This will only
					 * happen when running on the cn=config DB.
					 */
					if ( si->si_re ) {
						if ( ldap_pvt_thread_mutex_trylock( &si->si_mutex )) {
							isrunning = 1;
						} else {
							/* There is no active thread, but we must still
							 * ensure that no thread is (or will be) queued
							 * while we removes the task.
							 */
							struct re_s *re = si->si_re;
							si->si_re = NULL;

							if ( si->si_conn ) {
								connection_client_stop( si->si_conn );
								si->si_conn = NULL;
							}

							ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
							if ( ldap_pvt_runqueue_isrunning( &slapd_rq, re ) ) {
								ldap_pvt_runqueue_stoptask( &slapd_rq, re );
								isrunning = 1;
							}
							if ( ldap_pvt_thread_pool_retract( &connection_pool,
									re->routine, re ) > 0 )
								isrunning = 0;

							ldap_pvt_runqueue_remove( &slapd_rq, re );
							ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

							ldap_pvt_thread_mutex_unlock( &si->si_mutex );
						}
					}
					if ( !isrunning ) {
						syncinfo_free( si, 0 );
					}
					if ( i == c->valx )
						break;
				} else {
					sip = &si->si_next;
				}
			}
		}
		if ( !c->be->be_syncinfo ) {
			SLAP_DBFLAGS( c->be ) &= ~SLAP_DBFLAG_SHADOW_MASK;
		}
		return 0;
	}
	if ( SLAP_SLURP_SHADOW( c->be ) ) {
		Debug(LDAP_DEBUG_ANY, "%s: "
			"syncrepl: database already shadowed.\n",
			c->log, 0, 0);
		return(1);
	} else {
		return add_syncrepl( c );
	}
}
