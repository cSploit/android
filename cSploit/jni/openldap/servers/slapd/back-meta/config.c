/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * Portions Copyright 1999-2003 Howard Chu.
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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#include "portable.h"

#include <stdio.h>
#include <ctype.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "config.h"
#include "lutil.h"
#include "ldif.h"
#include "../back-ldap/back-ldap.h"
#include "back-meta.h"

#define SLAP_AUTH_DN	1

static ConfigDriver meta_back_cf_gen;
static ConfigLDAPadd meta_ldadd;
static ConfigCfAdd meta_cfadd;

static int ldap_back_map_config(
	ConfigArgs *c,
	struct ldapmap	*oc_map,
	struct ldapmap	*at_map );

/* Three sets of enums:
 *	1) attrs that are only valid in the base config
 *	2) attrs that are valid in base or target
 *	3) attrs that are only valid in a target
 */

/* Base attrs */
enum {
	LDAP_BACK_CFG_CONN_TTL = 1,
	LDAP_BACK_CFG_DNCACHE_TTL,
	LDAP_BACK_CFG_IDLE_TIMEOUT,
	LDAP_BACK_CFG_ONERR,
	LDAP_BACK_CFG_PSEUDOROOT_BIND_DEFER,
	LDAP_BACK_CFG_SINGLECONN,
	LDAP_BACK_CFG_USETEMP,
	LDAP_BACK_CFG_CONNPOOLMAX,
	LDAP_BACK_CFG_LAST_BASE
};

/* Base or target */
enum {
	LDAP_BACK_CFG_BIND_TIMEOUT = LDAP_BACK_CFG_LAST_BASE,
	LDAP_BACK_CFG_CANCEL,
	LDAP_BACK_CFG_CHASE,
	LDAP_BACK_CFG_CLIENT_PR,
	LDAP_BACK_CFG_DEFAULT_T,
	LDAP_BACK_CFG_NETWORK_TIMEOUT,
	LDAP_BACK_CFG_NOREFS,
	LDAP_BACK_CFG_NOUNDEFFILTER,
	LDAP_BACK_CFG_NRETRIES,
	LDAP_BACK_CFG_QUARANTINE,
	LDAP_BACK_CFG_REBIND,
	LDAP_BACK_CFG_TIMEOUT,
	LDAP_BACK_CFG_VERSION,
	LDAP_BACK_CFG_ST_REQUEST,
	LDAP_BACK_CFG_T_F,
	LDAP_BACK_CFG_TLS,
	LDAP_BACK_CFG_LAST_BOTH
};

/* Target attrs */
enum {
	LDAP_BACK_CFG_URI = LDAP_BACK_CFG_LAST_BOTH,
	LDAP_BACK_CFG_ACL_AUTHCDN,
	LDAP_BACK_CFG_ACL_PASSWD,
	LDAP_BACK_CFG_IDASSERT_AUTHZFROM,
	LDAP_BACK_CFG_IDASSERT_BIND,
	LDAP_BACK_CFG_REWRITE,
	LDAP_BACK_CFG_SUFFIXM,
	LDAP_BACK_CFG_MAP,
	LDAP_BACK_CFG_SUBTREE_EX,
	LDAP_BACK_CFG_SUBTREE_IN,
	LDAP_BACK_CFG_PSEUDOROOTDN,
	LDAP_BACK_CFG_PSEUDOROOTPW,
	LDAP_BACK_CFG_KEEPALIVE,
	LDAP_BACK_CFG_FILTER,

	LDAP_BACK_CFG_LAST
};

static ConfigTable metacfg[] = {
	{ "uri", "uri", 2, 0, 0,
		ARG_MAGIC|LDAP_BACK_CFG_URI,
		meta_back_cf_gen, "( OLcfgDbAt:0.14 "
			"NAME 'olcDbURI' "
			"DESC 'URI (list) for remote DSA' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "tls", "what", 2, 0, 0,
		ARG_MAGIC|LDAP_BACK_CFG_TLS,
		meta_back_cf_gen, "( OLcfgDbAt:3.1 "
			"NAME 'olcDbStartTLS' "
			"DESC 'StartTLS' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "acl-authcDN", "DN", 2, 2, 0,
		ARG_DN|ARG_MAGIC|LDAP_BACK_CFG_ACL_AUTHCDN,
		meta_back_cf_gen, "( OLcfgDbAt:3.2 "
			"NAME 'olcDbACLAuthcDn' "
			"DESC 'Remote ACL administrative identity' "
			"OBSOLETE "
			"SYNTAX OMsDN "
			"SINGLE-VALUE )",
		NULL, NULL },
	/* deprecated, will be removed; aliases "acl-authcDN" */
	{ "binddn", "DN", 2, 2, 0,
		ARG_DN|ARG_MAGIC|LDAP_BACK_CFG_ACL_AUTHCDN,
		meta_back_cf_gen, NULL, NULL, NULL },
	{ "acl-passwd", "cred", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_ACL_PASSWD,
		meta_back_cf_gen, "( OLcfgDbAt:3.3 "
			"NAME 'olcDbACLPasswd' "
			"DESC 'Remote ACL administrative identity credentials' "
			"OBSOLETE "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	/* deprecated, will be removed; aliases "acl-passwd" */
	{ "bindpw", "cred", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_ACL_PASSWD,
		meta_back_cf_gen, NULL, NULL, NULL },
	{ "idassert-bind", "args", 2, 0, 0,
		ARG_MAGIC|LDAP_BACK_CFG_IDASSERT_BIND,
		meta_back_cf_gen, "( OLcfgDbAt:3.7 "
			"NAME 'olcDbIDAssertBind' "
			"DESC 'Remote Identity Assertion administrative identity auth bind configuration' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "idassert-authzFrom", "authzRule", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_IDASSERT_AUTHZFROM,
		meta_back_cf_gen, "( OLcfgDbAt:3.9 "
			"NAME 'olcDbIDAssertAuthzFrom' "
			"DESC 'Remote Identity Assertion authz rules' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )",
		NULL, NULL },
	{ "rebind-as-user", "true|FALSE", 1, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_REBIND,
		meta_back_cf_gen, "( OLcfgDbAt:3.10 "
			"NAME 'olcDbRebindAsUser' "
			"DESC 'Rebind as user' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "chase-referrals", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_CHASE,
		meta_back_cf_gen, "( OLcfgDbAt:3.11 "
			"NAME 'olcDbChaseReferrals' "
			"DESC 'Chase referrals' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "t-f-support", "true|FALSE|discover", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_T_F,
		meta_back_cf_gen, "( OLcfgDbAt:3.12 "
			"NAME 'olcDbTFSupport' "
			"DESC 'Absolute filters support' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "timeout", "timeout(list)", 2, 0, 0,
		ARG_MAGIC|LDAP_BACK_CFG_TIMEOUT,
		meta_back_cf_gen, "( OLcfgDbAt:3.14 "
			"NAME 'olcDbTimeout' "
			"DESC 'Per-operation timeouts' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "idle-timeout", "timeout", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_IDLE_TIMEOUT,
		meta_back_cf_gen, "( OLcfgDbAt:3.15 "
			"NAME 'olcDbIdleTimeout' "
			"DESC 'connection idle timeout' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "conn-ttl", "ttl", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_CONN_TTL,
		meta_back_cf_gen, "( OLcfgDbAt:3.16 "
			"NAME 'olcDbConnTtl' "
			"DESC 'connection ttl' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "network-timeout", "timeout", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_NETWORK_TIMEOUT,
		meta_back_cf_gen, "( OLcfgDbAt:3.17 "
			"NAME 'olcDbNetworkTimeout' "
			"DESC 'connection network timeout' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "protocol-version", "version", 2, 2, 0,
		ARG_MAGIC|ARG_INT|LDAP_BACK_CFG_VERSION,
		meta_back_cf_gen, "( OLcfgDbAt:3.18 "
			"NAME 'olcDbProtocolVersion' "
			"DESC 'protocol version' "
			"SYNTAX OMsInteger "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "single-conn", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_SINGLECONN,
		meta_back_cf_gen, "( OLcfgDbAt:3.19 "
			"NAME 'olcDbSingleConn' "
			"DESC 'cache a single connection per identity' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "cancel", "ABANDON|ignore|exop", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_CANCEL,
		meta_back_cf_gen, "( OLcfgDbAt:3.20 "
			"NAME 'olcDbCancel' "
			"DESC 'abandon/ignore/exop operations when appropriate' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "quarantine", "retrylist", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_QUARANTINE,
		meta_back_cf_gen, "( OLcfgDbAt:3.21 "
			"NAME 'olcDbQuarantine' "
			"DESC 'Quarantine database if connection fails and retry according to rule' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "use-temporary-conn", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_USETEMP,
		meta_back_cf_gen, "( OLcfgDbAt:3.22 "
			"NAME 'olcDbUseTemporaryConn' "
			"DESC 'Use temporary connections if the cached one is busy' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "conn-pool-max", "<n>", 2, 2, 0,
		ARG_MAGIC|ARG_INT|LDAP_BACK_CFG_CONNPOOLMAX,
		meta_back_cf_gen, "( OLcfgDbAt:3.23 "
			"NAME 'olcDbConnectionPoolMax' "
			"DESC 'Max size of privileged connections pool' "
			"SYNTAX OMsInteger "
			"SINGLE-VALUE )",
		NULL, NULL },
#ifdef SLAP_CONTROL_X_SESSION_TRACKING
	{ "session-tracking-request", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_ST_REQUEST,
		meta_back_cf_gen, "( OLcfgDbAt:3.24 "
			"NAME 'olcDbSessionTrackingRequest' "
			"DESC 'Add session tracking control to proxied requests' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */
	{ "norefs", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_NOREFS,
		meta_back_cf_gen, "( OLcfgDbAt:3.25 "
			"NAME 'olcDbNoRefs' "
			"DESC 'Do not return search reference responses' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "noundeffilter", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_NOUNDEFFILTER,
		meta_back_cf_gen, "( OLcfgDbAt:3.26 "
			"NAME 'olcDbNoUndefFilter' "
			"DESC 'Do not propagate undefined search filters' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },

	{ "rewrite", "arglist", 2, 0, STRLENOF( "rewrite" ),
		ARG_MAGIC|LDAP_BACK_CFG_REWRITE,
		meta_back_cf_gen, "( OLcfgDbAt:3.101 "
			"NAME 'olcDbRewrite' "
			"DESC 'DN rewriting rules' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )",
		NULL, NULL },
	{ "suffixmassage", "virtual> <real", 2, 3, 0,
		ARG_MAGIC|LDAP_BACK_CFG_SUFFIXM,
		meta_back_cf_gen, NULL, NULL, NULL },

	{ "map", "attribute|objectClass> [*|<local>] *|<remote", 3, 4, 0,
		ARG_MAGIC|LDAP_BACK_CFG_MAP,
		meta_back_cf_gen, "( OLcfgDbAt:3.102 "
			"NAME 'olcDbMap' "
			"DESC 'Map attribute and objectclass names' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )",
		NULL, NULL },

	{ "subtree-exclude", "pattern", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_SUBTREE_EX,
		meta_back_cf_gen, "( OLcfgDbAt:3.103 "
			"NAME 'olcDbSubtreeExclude' "
			"DESC 'DN of subtree to exclude from target' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )",
		NULL, NULL },
	{ "subtree-include", "pattern", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_SUBTREE_IN,
		meta_back_cf_gen, "( OLcfgDbAt:3.104 "
			"NAME 'olcDbSubtreeInclude' "
			"DESC 'DN of subtree to include in target' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )",
		NULL, NULL },
	{ "default-target", "[none|<target ID>]", 1, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_DEFAULT_T,
		meta_back_cf_gen, "( OLcfgDbAt:3.105 "
			"NAME 'olcDbDefaultTarget' "
			"DESC 'Specify the default target' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "dncache-ttl", "ttl", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_DNCACHE_TTL,
		meta_back_cf_gen, "( OLcfgDbAt:3.106 "
			"NAME 'olcDbDnCacheTtl' "
			"DESC 'dncache ttl' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "bind-timeout", "microseconds", 2, 2, 0,
		ARG_MAGIC|ARG_ULONG|LDAP_BACK_CFG_BIND_TIMEOUT,
		meta_back_cf_gen, "( OLcfgDbAt:3.107 "
			"NAME 'olcDbBindTimeout' "
			"DESC 'bind timeout' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "onerr", "CONTINUE|report|stop", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_ONERR,
		meta_back_cf_gen, "( OLcfgDbAt:3.108 "
			"NAME 'olcDbOnErr' "
			"DESC 'error handling' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "pseudoroot-bind-defer", "TRUE|false", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_PSEUDOROOT_BIND_DEFER,
		meta_back_cf_gen, "( OLcfgDbAt:3.109 "
			"NAME 'olcDbPseudoRootBindDefer' "
			"DESC 'error handling' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "root-bind-defer", "TRUE|false", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_PSEUDOROOT_BIND_DEFER,
		meta_back_cf_gen, NULL, NULL, NULL },
	{ "pseudorootdn", "dn", 2, 2, 0,
		ARG_MAGIC|ARG_DN|LDAP_BACK_CFG_PSEUDOROOTDN,
		meta_back_cf_gen, NULL, NULL, NULL },
	{ "pseudorootpw", "password", 2, 2, 0,
		ARG_MAGIC|ARG_STRING|LDAP_BACK_CFG_PSEUDOROOTDN,
		meta_back_cf_gen, NULL, NULL, NULL },
	{ "nretries", "NEVER|forever|<number>", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_NRETRIES,
		meta_back_cf_gen, "( OLcfgDbAt:3.110 "
			"NAME 'olcDbNretries' "
			"DESC 'retry handling' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "client-pr", "accept-unsolicited|disable|<size>", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_CLIENT_PR,
		meta_back_cf_gen, "( OLcfgDbAt:3.111 "
			"NAME 'olcDbClientPr' "
			"DESC 'PagedResults handling' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },

	{ "", "", 0, 0, 0, ARG_IGNORED,
		NULL, "( OLcfgDbAt:3.100 NAME 'olcMetaSub' "
			"DESC 'Placeholder to name a Target entry' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE X-ORDERED 'SIBLINGS' )", NULL, NULL },

	{ "keepalive", "keepalive", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_KEEPALIVE,
		meta_back_cf_gen, "( OLcfgDbAt:3.29 "
			"NAME 'olcDbKeepalive' "
			"DESC 'TCP keepalive' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },

	{ "filter", "pattern", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_FILTER,
		meta_back_cf_gen, "( OLcfgDbAt:3.112 "
			"NAME 'olcDbFilter' "
			"DESC 'Filter regex pattern to include in target' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString )",
		NULL, NULL },

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
#define	ST_ATTR "$ olcDbSessionTrackingRequest "
#else
#define	ST_ATTR ""
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

#define COMMON_ATTRS	\
			"$ olcDbBindTimeout " \
			"$ olcDbCancel " \
			"$ olcDbChaseReferrals " \
			"$ olcDbClientPr " \
			"$ olcDbDefaultTarget " \
			"$ olcDbNetworkTimeout " \
			"$ olcDbNoRefs " \
			"$ olcDbNoUndefFilter " \
			"$ olcDbNretries " \
			"$ olcDbProtocolVersion " \
			"$ olcDbQuarantine " \
			"$ olcDbRebindAsUser " \
			ST_ATTR \
			"$ olcDbStartTLS " \
			"$ olcDbTFSupport "

static ConfigOCs metaocs[] = {
	{ "( OLcfgDbOc:3.2 "
		"NAME 'olcMetaConfig' "
		"DESC 'Meta backend configuration' "
		"SUP olcDatabaseConfig "
		"MAY ( olcDbConnTtl "
			"$ olcDbDnCacheTtl "
			"$ olcDbIdleTimeout "
			"$ olcDbOnErr "
			"$ olcDbPseudoRootBindDefer "
			"$ olcDbSingleConn "
			"$ olcDbUseTemporaryConn "
			"$ olcDbConnectionPoolMax "

			/* defaults, may be overridden per-target */
			COMMON_ATTRS
		") )",
			Cft_Database, metacfg, NULL, meta_cfadd },
	{ "( OLcfgDbOc:3.3 "
		"NAME 'olcMetaTargetConfig' "
		"DESC 'Meta target configuration' "
		"SUP olcConfig STRUCTURAL "
		"MUST ( olcMetaSub $ olcDbURI ) "
		"MAY ( olcDbACLAuthcDn "
			"$ olcDbACLPasswd "
			"$ olcDbIDAssertAuthzFrom "
			"$ olcDbIDAssertBind "
			"$ olcDbMap "
			"$ olcDbRewrite "
			"$ olcDbSubtreeExclude "
			"$ olcDbSubtreeInclude "
			"$ olcDbTimeout "
			"$ olcDbKeepalive "
			"$ olcDbFilter "

			/* defaults may be inherited */
			COMMON_ATTRS
		") )",
			Cft_Misc, metacfg, meta_ldadd },
	{ NULL, 0, NULL }
};

static int
meta_ldadd( CfEntryInfo *p, Entry *e, ConfigArgs *c )
{
	if ( p->ce_type != Cft_Database || !p->ce_be ||
		p->ce_be->be_cf_ocs != metaocs )
		return LDAP_CONSTRAINT_VIOLATION;

	c->be = p->ce_be;
	return LDAP_SUCCESS;
}

static int
meta_cfadd( Operation *op, SlapReply *rs, Entry *p, ConfigArgs *c )
{
	metainfo_t	*mi = ( metainfo_t * )c->be->be_private;
	struct berval bv;
	int i;

	bv.bv_val = c->cr_msg;
	for ( i=0; i<mi->mi_ntargets; i++ ) {
		bv.bv_len = snprintf( c->cr_msg, sizeof(c->cr_msg),
			"olcMetaSub=" SLAP_X_ORDERED_FMT "uri", i );
		c->ca_private = mi->mi_targets[i];
		c->valx = i;
		config_build_entry( op, rs, p->e_private, c,
			&bv, &metaocs[1], NULL );
	}

	return LDAP_SUCCESS;
}

static int
meta_rwi_init( struct rewrite_info **rwm_rw )
{
	char			*rargv[ 3 ];

	*rwm_rw = rewrite_info_init( REWRITE_MODE_USE_DEFAULT );
	if ( *rwm_rw == NULL ) {
		return -1;
	}
	/*
	 * the filter rewrite as a string must be disabled
	 * by default; it can be re-enabled by adding rules;
	 * this creates an empty rewriteContext
	 */
	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "searchFilter";
	rargv[ 2 ] = NULL;
	rewrite_parse( *rwm_rw, "<suffix massage>", 1, 2, rargv );

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "default";
	rargv[ 2 ] = NULL;
	rewrite_parse( *rwm_rw, "<suffix massage>", 1, 2, rargv );

	return 0;
}

static int
meta_back_new_target(
	metatarget_t	**mtp )
{
	metatarget_t		*mt;

	*mtp = NULL;

	mt = ch_calloc( sizeof( metatarget_t ), 1 );

	if ( meta_rwi_init( &mt->mt_rwmap.rwm_rw )) {
		ch_free( mt );
		return -1;
	}

	ldap_pvt_thread_mutex_init( &mt->mt_uri_mutex );

	mt->mt_idassert_mode = LDAP_BACK_IDASSERT_LEGACY;
	mt->mt_idassert_authmethod = LDAP_AUTH_NONE;
	mt->mt_idassert_tls = SB_TLS_DEFAULT;

	/* by default, use proxyAuthz control on each operation */
	mt->mt_idassert_flags = LDAP_BACK_AUTH_PRESCRIPTIVE;

	*mtp = mt;

	return 0;
}

/* Validation for suffixmassage_config */
static int
meta_suffixm_config(
	ConfigArgs *c,
	int argc,
	char **argv,
	metatarget_t *mt
)
{
	BackendDB 	*tmp_bd;
	struct berval	dn, nvnc, pvnc, nrnc, prnc;
	int j, rc;

	/*
	 * syntax:
	 *
	 * 	suffixmassage <suffix> <massaged suffix>
	 *
	 * the <suffix> field must be defined as a valid suffix
	 * (or suffixAlias?) for the current database;
	 * the <massaged suffix> shouldn't have already been
	 * defined as a valid suffix or suffixAlias for the
	 * current server
	 */

	ber_str2bv( argv[ 1 ], 0, 0, &dn );
	if ( dnPrettyNormal( NULL, &dn, &pvnc, &nvnc, NULL ) != LDAP_SUCCESS ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			"suffix \"%s\" is invalid",
			argv[1] );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		return 1;
	}

	for ( j = 0; !BER_BVISNULL( &c->be->be_nsuffix[ j ] ); j++ ) {
		if ( dnIsSuffix( &nvnc, &c->be->be_nsuffix[ 0 ] ) ) {
			break;
		}
	}

	if ( BER_BVISNULL( &c->be->be_nsuffix[ j ] ) ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			"suffix \"%s\" must be within the database naming context",
			argv[1] );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		free( pvnc.bv_val );
		free( nvnc.bv_val );
		return 1;
	}

	ber_str2bv( argv[ 2 ], 0, 0, &dn );
	if ( dnPrettyNormal( NULL, &dn, &prnc, &nrnc, NULL ) != LDAP_SUCCESS ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			"massaged suffix \"%s\" is invalid",
			argv[2] );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		free( pvnc.bv_val );
		free( nvnc.bv_val );
		return 1;
	}

	tmp_bd = select_backend( &nrnc, 0 );
	if ( tmp_bd != NULL && tmp_bd->be_private == c->be->be_private ) {
		Debug( LDAP_DEBUG_ANY,
	"%s: warning: <massaged suffix> \"%s\" resolves to this database, in "
	"\"suffixMassage <suffix> <massaged suffix>\"\n",
			c->log, prnc.bv_val, 0 );
	}

	/*
	 * The suffix massaging is emulated by means of the
	 * rewrite capabilities
	 */
	rc = suffix_massage_config( mt->mt_rwmap.rwm_rw,
			&pvnc, &nvnc, &prnc, &nrnc );

	free( pvnc.bv_val );
	free( nvnc.bv_val );
	free( prnc.bv_val );
	free( nrnc.bv_val );

	return rc;
}

static int
slap_bv_x_ordered_unparse( BerVarray in, BerVarray *out )
{
	int		i;
	BerVarray	bva = NULL;
	char		ibuf[32], *ptr;
	struct berval	idx;

	assert( in != NULL );

	for ( i = 0; !BER_BVISNULL( &in[i] ); i++ )
		/* count'em */ ;

	if ( i == 0 ) {
		return 1;
	}

	idx.bv_val = ibuf;

	bva = ch_malloc( ( i + 1 ) * sizeof(struct berval) );
	BER_BVZERO( &bva[ 0 ] );

	for ( i = 0; !BER_BVISNULL( &in[i] ); i++ ) {
		idx.bv_len = snprintf( idx.bv_val, sizeof( ibuf ), SLAP_X_ORDERED_FMT, i );
		if ( idx.bv_len >= sizeof( ibuf ) ) {
			ber_bvarray_free( bva );
			return 1;
		}

		bva[i].bv_len = idx.bv_len + in[i].bv_len;
		bva[i].bv_val = ch_malloc( bva[i].bv_len + 1 );
		ptr = lutil_strcopy( bva[i].bv_val, ibuf );
		ptr = lutil_strcopy( ptr, in[i].bv_val );
		*ptr = '\0';
		BER_BVZERO( &bva[ i + 1 ] );
	}

	*out = bva;
	return 0;
}

int
meta_subtree_free( metasubtree_t *ms )
{
	switch ( ms->ms_type ) {
	case META_ST_SUBTREE:
	case META_ST_SUBORDINATE:
		ber_memfree( ms->ms_dn.bv_val );
		break;

	case META_ST_REGEX:
		regfree( &ms->ms_regex );
		ber_memfree( ms->ms_regex_pattern.bv_val );
		break;

	default:
		return -1;
	}

	ch_free( ms );
	return 0;
}

int
meta_subtree_destroy( metasubtree_t *ms )
{
	if ( ms->ms_next ) {
		meta_subtree_destroy( ms->ms_next );
	}

	return meta_subtree_free( ms );
}

static void
meta_filter_free( metafilter_t *mf )
{
	regfree( &mf->mf_regex );
	ber_memfree( mf->mf_regex_pattern.bv_val );
	ch_free( mf );
}

void
meta_filter_destroy( metafilter_t *mf )
{
	if ( mf->mf_next )
		meta_filter_destroy( mf->mf_next );
	meta_filter_free( mf );
}

static struct berval st_styles[] = {
	BER_BVC("subtree"),
	BER_BVC("children"),
	BER_BVC("regex")
};

static int
meta_subtree_unparse(
	ConfigArgs *c,
	metatarget_t *mt )
{
	metasubtree_t	*ms;
	struct berval bv, *style;

	if ( !mt->mt_subtree )
		return 1;

	/* can only be one of exclude or include */
	if (( c->type == LDAP_BACK_CFG_SUBTREE_EX ) ^ mt->mt_subtree_exclude )
		return 1;

	bv.bv_val = c->cr_msg;
	for ( ms=mt->mt_subtree; ms; ms=ms->ms_next ) {
		if (ms->ms_type == META_ST_SUBTREE)
			style = &st_styles[0];
		else if ( ms->ms_type == META_ST_SUBORDINATE )
			style = &st_styles[1];
		else if ( ms->ms_type == META_ST_REGEX )
			style = &st_styles[2];
		else {
			assert(0);
			continue;
		}
		bv.bv_len = snprintf( c->cr_msg, sizeof(c->cr_msg),
			"dn.%s:%s", style->bv_val, ms->ms_dn.bv_val );
		value_add_one( &c->rvalue_vals, &bv );
	}
	return 0;
}

static int
meta_subtree_config(
	metatarget_t *mt,
	ConfigArgs *c )
{
	meta_st_t	type = META_ST_SUBTREE;
	char		*pattern;
	struct berval	ndn = BER_BVNULL;
	metasubtree_t	*ms = NULL;

	if ( c->type == LDAP_BACK_CFG_SUBTREE_EX ) {
		if ( mt->mt_subtree && !mt->mt_subtree_exclude ) {
			snprintf( c->cr_msg, sizeof(c->cr_msg),
				"\"subtree-exclude\" incompatible with previous \"subtree-include\" directives" );
			return 1;
		}

		mt->mt_subtree_exclude = 1;

	} else {
		if ( mt->mt_subtree && mt->mt_subtree_exclude ) {
			snprintf( c->cr_msg, sizeof(c->cr_msg),
				"\"subtree-include\" incompatible with previous \"subtree-exclude\" directives" );
			return 1;
		}
	}

	pattern = c->argv[1];
	if ( strncasecmp( pattern, "dn", STRLENOF( "dn" ) ) == 0 ) {
		char *style;

		pattern = &pattern[STRLENOF( "dn")];

		if ( pattern[0] == '.' ) {
			style = &pattern[1];

			if ( strncasecmp( style, "subtree", STRLENOF( "subtree" ) ) == 0 ) {
				type = META_ST_SUBTREE;
				pattern = &style[STRLENOF( "subtree" )];

			} else if ( strncasecmp( style, "children", STRLENOF( "children" ) ) == 0 ) {
				type = META_ST_SUBORDINATE;
				pattern = &style[STRLENOF( "children" )];

			} else if ( strncasecmp( style, "sub", STRLENOF( "sub" ) ) == 0 ) {
				type = META_ST_SUBTREE;
				pattern = &style[STRLENOF( "sub" )];

			} else if ( strncasecmp( style, "regex", STRLENOF( "regex" ) ) == 0 ) {
				type = META_ST_REGEX;
				pattern = &style[STRLENOF( "regex" )];

			} else {
				snprintf( c->cr_msg, sizeof(c->cr_msg), "unknown style in \"dn.<style>\"" );
				return 1;
			}
		}

		if ( pattern[0] != ':' ) {
			snprintf( c->cr_msg, sizeof(c->cr_msg), "missing colon after \"dn.<style>\"" );
			return 1;
		}
		pattern++;
	}

	switch ( type ) {
	case META_ST_SUBTREE:
	case META_ST_SUBORDINATE: {
		struct berval dn;

		ber_str2bv( pattern, 0, 0, &dn );
		if ( dnNormalize( 0, NULL, NULL, &dn, &ndn, NULL )
			!= LDAP_SUCCESS )
		{
			snprintf( c->cr_msg, sizeof(c->cr_msg), "DN=\"%s\" is invalid", pattern );
			return 1;
		}

		if ( !dnIsSuffix( &ndn, &mt->mt_nsuffix ) ) {
			snprintf( c->cr_msg, sizeof(c->cr_msg),
				"DN=\"%s\" is not a subtree of target \"%s\"",
				pattern, mt->mt_nsuffix.bv_val );
			ber_memfree( ndn.bv_val );
			return( 1 );
		}
		} break;

	default:
		/* silence warnings */
		break;
	}

	ms = ch_calloc( sizeof( metasubtree_t ), 1 );
	ms->ms_type = type;

	switch ( ms->ms_type ) {
	case META_ST_SUBTREE:
	case META_ST_SUBORDINATE:
		ms->ms_dn = ndn;
		break;

	case META_ST_REGEX: {
		int rc;

		rc = regcomp( &ms->ms_regex, pattern, REG_EXTENDED|REG_ICASE );
		if ( rc != 0 ) {
			char regerr[ SLAP_TEXT_BUFLEN ];

			regerror( rc, &ms->ms_regex, regerr, sizeof(regerr) );

			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"regular expression \"%s\" bad because of %s",
				pattern, regerr );
			ch_free( ms );
			return 1;
		}
		ber_str2bv( pattern, 0, 1, &ms->ms_regex_pattern );
		} break;
	}

	if ( mt->mt_subtree == NULL ) {
		 mt->mt_subtree = ms;

	} else {
		metasubtree_t **msp;

		for ( msp = &mt->mt_subtree; *msp; ) {
			switch ( ms->ms_type ) {
			case META_ST_SUBTREE:
				switch ( (*msp)->ms_type ) {
				case META_ST_SUBTREE:
					if ( dnIsSuffix( &(*msp)->ms_dn, &ms->ms_dn ) ) {
						metasubtree_t *tmp = *msp;
						Debug( LDAP_DEBUG_CONFIG,
							"%s: previous rule \"dn.subtree:%s\" is contained in rule \"dn.subtree:%s\" (replaced)\n",
							c->log, pattern, (*msp)->ms_dn.bv_val );
						*msp = (*msp)->ms_next;
						tmp->ms_next = NULL;
						meta_subtree_destroy( tmp );
						continue;

					} else if ( dnIsSuffix( &ms->ms_dn, &(*msp)->ms_dn ) ) {
						Debug( LDAP_DEBUG_CONFIG,
							"%s: previous rule \"dn.subtree:%s\" contains rule \"dn.subtree:%s\" (ignored)\n",
							c->log, (*msp)->ms_dn.bv_val, pattern );
						meta_subtree_destroy( ms );
						ms = NULL;
						return( 0 );
					}
					break;

				case META_ST_SUBORDINATE:
					if ( dnIsSuffix( &(*msp)->ms_dn, &ms->ms_dn ) ) {
						metasubtree_t *tmp = *msp;
						Debug( LDAP_DEBUG_CONFIG,
							"%s: previous rule \"dn.children:%s\" is contained in rule \"dn.subtree:%s\" (replaced)\n",
							c->log, pattern, (*msp)->ms_dn.bv_val );
						*msp = (*msp)->ms_next;
						tmp->ms_next = NULL;
						meta_subtree_destroy( tmp );
						continue;

					} else if ( dnIsSuffix( &ms->ms_dn, &(*msp)->ms_dn ) && ms->ms_dn.bv_len > (*msp)->ms_dn.bv_len ) {
						Debug( LDAP_DEBUG_CONFIG,
							"%s: previous rule \"dn.children:%s\" contains rule \"dn.subtree:%s\" (ignored)\n",
							c->log, (*msp)->ms_dn.bv_val, pattern );
						meta_subtree_destroy( ms );
						ms = NULL;
						return( 0 );
					}
					break;

				case META_ST_REGEX:
					if ( regexec( &(*msp)->ms_regex, ms->ms_dn.bv_val, 0, NULL, 0 ) == 0 ) {
						Debug( LDAP_DEBUG_CONFIG,
							"%s: previous rule \"dn.regex:%s\" may contain rule \"dn.subtree:%s\"\n",
							c->log, (*msp)->ms_regex_pattern.bv_val, ms->ms_dn.bv_val );
					}
					break;
				}
				break;

			case META_ST_SUBORDINATE:
				switch ( (*msp)->ms_type ) {
				case META_ST_SUBTREE:
					if ( dnIsSuffix( &(*msp)->ms_dn, &ms->ms_dn ) ) {
						metasubtree_t *tmp = *msp;
						Debug( LDAP_DEBUG_CONFIG,
							"%s: previous rule \"dn.children:%s\" is contained in rule \"dn.subtree:%s\" (replaced)\n",
							c->log, pattern, (*msp)->ms_dn.bv_val );
						*msp = (*msp)->ms_next;
						tmp->ms_next = NULL;
						meta_subtree_destroy( tmp );
						continue;

					} else if ( dnIsSuffix( &ms->ms_dn, &(*msp)->ms_dn ) && ms->ms_dn.bv_len > (*msp)->ms_dn.bv_len ) {
						Debug( LDAP_DEBUG_CONFIG,
							"%s: previous rule \"dn.children:%s\" contains rule \"dn.subtree:%s\" (ignored)\n",
							c->log, (*msp)->ms_dn.bv_val, pattern );
						meta_subtree_destroy( ms );
						ms = NULL;
						return( 0 );
					}
					break;

				case META_ST_SUBORDINATE:
					if ( dnIsSuffix( &(*msp)->ms_dn, &ms->ms_dn ) ) {
						metasubtree_t *tmp = *msp;
						Debug( LDAP_DEBUG_CONFIG,
							"%s: previous rule \"dn.children:%s\" is contained in rule \"dn.children:%s\" (replaced)\n",
							c->log, pattern, (*msp)->ms_dn.bv_val );
						*msp = (*msp)->ms_next;
						tmp->ms_next = NULL;
						meta_subtree_destroy( tmp );
						continue;

					} else if ( dnIsSuffix( &ms->ms_dn, &(*msp)->ms_dn ) ) {
						Debug( LDAP_DEBUG_CONFIG,
							"%s: previous rule \"dn.children:%s\" contains rule \"dn.children:%s\" (ignored)\n",
							c->log, (*msp)->ms_dn.bv_val, pattern );
						meta_subtree_destroy( ms );
						ms = NULL;
						return( 0 );
					}
					break;

				case META_ST_REGEX:
					if ( regexec( &(*msp)->ms_regex, ms->ms_dn.bv_val, 0, NULL, 0 ) == 0 ) {
						Debug( LDAP_DEBUG_CONFIG,
							"%s: previous rule \"dn.regex:%s\" may contain rule \"dn.subtree:%s\"\n",
							c->log, (*msp)->ms_regex_pattern.bv_val, ms->ms_dn.bv_val );
					}
					break;
				}
				break;

			case META_ST_REGEX:
				switch ( (*msp)->ms_type ) {
				case META_ST_SUBTREE:
				case META_ST_SUBORDINATE:
					if ( regexec( &ms->ms_regex, (*msp)->ms_dn.bv_val, 0, NULL, 0 ) == 0 ) {
						Debug( LDAP_DEBUG_CONFIG,
							"%s: previous rule \"dn.subtree:%s\" may be contained in rule \"dn.regex:%s\"\n",
							c->log, (*msp)->ms_dn.bv_val, ms->ms_regex_pattern.bv_val );
					}
					break;

				case META_ST_REGEX:
					/* no check possible */
					break;
				}
				break;
			}

			msp = &(*msp)->ms_next;
		}

		*msp = ms;
	}

	return 0;
}

static slap_verbmasks idassert_mode[] = {
	{ BER_BVC("self"),		LDAP_BACK_IDASSERT_SELF },
	{ BER_BVC("anonymous"),		LDAP_BACK_IDASSERT_ANONYMOUS },
	{ BER_BVC("none"),		LDAP_BACK_IDASSERT_NOASSERT },
	{ BER_BVC("legacy"),		LDAP_BACK_IDASSERT_LEGACY },
	{ BER_BVNULL,			0 }
};

static slap_verbmasks tls_mode[] = {
	{ BER_BVC( "propagate" ),	LDAP_BACK_F_TLS_PROPAGATE_MASK },
	{ BER_BVC( "try-propagate" ),	LDAP_BACK_F_PROPAGATE_TLS },
	{ BER_BVC( "start" ),		LDAP_BACK_F_TLS_USE_MASK },
	{ BER_BVC( "try-start" ),	LDAP_BACK_F_USE_TLS },
	{ BER_BVC( "ldaps" ),		LDAP_BACK_F_TLS_LDAPS },
	{ BER_BVC( "none" ),		LDAP_BACK_F_NONE },
	{ BER_BVNULL,			0 }
};

static slap_verbmasks t_f_mode[] = {
	{ BER_BVC( "yes" ),		LDAP_BACK_F_T_F },
	{ BER_BVC( "discover" ),	LDAP_BACK_F_T_F_DISCOVER },
	{ BER_BVC( "no" ),		LDAP_BACK_F_NONE },
	{ BER_BVNULL,			0 }
};

static slap_verbmasks cancel_mode[] = {
	{ BER_BVC( "ignore" ),		LDAP_BACK_F_CANCEL_IGNORE },
	{ BER_BVC( "exop" ),		LDAP_BACK_F_CANCEL_EXOP },
	{ BER_BVC( "exop-discover" ),	LDAP_BACK_F_CANCEL_EXOP_DISCOVER },
	{ BER_BVC( "abandon" ),		LDAP_BACK_F_CANCEL_ABANDON },
	{ BER_BVNULL,			0 }
};

static slap_verbmasks onerr_mode[] = {
	{ BER_BVC( "stop" ),		META_BACK_F_ONERR_STOP },
	{ BER_BVC( "report" ),	META_BACK_F_ONERR_REPORT },
	{ BER_BVC( "continue" ),		LDAP_BACK_F_NONE },
	{ BER_BVNULL,			0 }
};

/* see enum in slap.h */
static slap_cf_aux_table timeout_table[] = {
	{ BER_BVC("bind="),	SLAP_OP_BIND * sizeof( time_t ),	'u', 0, NULL },
	/* unbind makes no sense */
	{ BER_BVC("add="),	SLAP_OP_ADD * sizeof( time_t ),		'u', 0, NULL },
	{ BER_BVC("delete="),	SLAP_OP_DELETE * sizeof( time_t ),	'u', 0, NULL },
	{ BER_BVC("modrdn="),	SLAP_OP_MODRDN * sizeof( time_t ),	'u', 0, NULL },
	{ BER_BVC("modify="),	SLAP_OP_MODIFY * sizeof( time_t ),	'u', 0, NULL },
	{ BER_BVC("compare="),	SLAP_OP_COMPARE * sizeof( time_t ),	'u', 0, NULL },
	{ BER_BVC("search="),	SLAP_OP_SEARCH * sizeof( time_t ),	'u', 0, NULL },
	/* abandon makes little sense */
#if 0	/* not implemented yet */
	{ BER_BVC("extended="),	SLAP_OP_EXTENDED * sizeof( time_t ),	'u', 0, NULL },
#endif
	{ BER_BVNULL, 0, 0, 0, NULL }
};

static int
meta_cf_cleanup( ConfigArgs *c )
{
	metainfo_t	*mi = ( metainfo_t * )c->be->be_private;
	metatarget_t	*mt = c->ca_private;

	return meta_target_finish( mi, mt, c->log, c->cr_msg, sizeof( c->cr_msg ));
}

static int
meta_back_cf_gen( ConfigArgs *c )
{
	metainfo_t	*mi = ( metainfo_t * )c->be->be_private;
	metatarget_t	*mt;
	metacommon_t	*mc;

	int i, rc = 0;

	assert( mi != NULL );

	if ( c->op == SLAP_CONFIG_EMIT || c->op == LDAP_MOD_DELETE ) {
		if ( !mi )
			return 1;

		if ( c->table == Cft_Database ) {
			mt = NULL;
			mc = &mi->mi_mc;
		} else {
			mt = c->ca_private;
			mc = &mt->mt_mc;
		}
	}

	if ( c->op == SLAP_CONFIG_EMIT ) {
		struct berval bv = BER_BVNULL;

		switch( c->type ) {
		/* Base attrs */
		case LDAP_BACK_CFG_CONN_TTL:
			if ( mi->mi_conn_ttl == 0 ) {
				return 1;
			} else {
				char	buf[ SLAP_TEXT_BUFLEN ];

				lutil_unparse_time( buf, sizeof( buf ), mi->mi_conn_ttl );
				ber_str2bv( buf, 0, 0, &bv );
				value_add_one( &c->rvalue_vals, &bv );
			}
			break;

		case LDAP_BACK_CFG_DNCACHE_TTL:
			if ( mi->mi_cache.ttl == META_DNCACHE_DISABLED ) {
				return 1;
			} else if ( mi->mi_cache.ttl == META_DNCACHE_FOREVER ) {
				BER_BVSTR( &bv, "forever" );
			} else {
				char	buf[ SLAP_TEXT_BUFLEN ];

				lutil_unparse_time( buf, sizeof( buf ), mi->mi_cache.ttl );
				ber_str2bv( buf, 0, 0, &bv );
			}
			value_add_one( &c->rvalue_vals, &bv );
			break;

		case LDAP_BACK_CFG_IDLE_TIMEOUT:
			if ( mi->mi_idle_timeout == 0 ) {
				return 1;
			} else {
				char	buf[ SLAP_TEXT_BUFLEN ];

				lutil_unparse_time( buf, sizeof( buf ), mi->mi_idle_timeout );
				ber_str2bv( buf, 0, 0, &bv );
				value_add_one( &c->rvalue_vals, &bv );
			}
			break;

		case LDAP_BACK_CFG_ONERR:
			enum_to_verb( onerr_mode, mi->mi_flags & META_BACK_F_ONERR_MASK, &bv );
			if ( BER_BVISNULL( &bv )) {
				rc = 1;
			} else {
				value_add_one( &c->rvalue_vals, &bv );
			}
			break;

		case LDAP_BACK_CFG_PSEUDOROOT_BIND_DEFER:
			c->value_int = META_BACK_DEFER_ROOTDN_BIND( mi );
			break;

		case LDAP_BACK_CFG_SINGLECONN:
			c->value_int = LDAP_BACK_SINGLECONN( mi );
			break;

		case LDAP_BACK_CFG_USETEMP:
			c->value_int = LDAP_BACK_USE_TEMPORARIES( mi );
			break;

		case LDAP_BACK_CFG_CONNPOOLMAX:
			c->value_int = mi->mi_conn_priv_max;
			break;

		/* common attrs */
		case LDAP_BACK_CFG_BIND_TIMEOUT:
			if ( mc->mc_bind_timeout.tv_sec == 0 &&
				mc->mc_bind_timeout.tv_usec == 0 ) {
				return 1;
			} else {
				c->value_ulong = mc->mc_bind_timeout.tv_sec * 1000000UL +
					mc->mc_bind_timeout.tv_usec;
			}
			break;

		case LDAP_BACK_CFG_CANCEL: {
			slap_mask_t	mask = LDAP_BACK_F_CANCEL_MASK2;

			if ( mt && META_BACK_TGT_CANCEL_DISCOVER( mt ) ) {
				mask &= ~LDAP_BACK_F_CANCEL_EXOP;
			}
			enum_to_verb( cancel_mode, (mc->mc_flags & mask), &bv );
			if ( BER_BVISNULL( &bv ) ) {
				/* there's something wrong... */
				assert( 0 );
				rc = 1;

			} else {
				value_add_one( &c->rvalue_vals, &bv );
			}
			} break;

		case LDAP_BACK_CFG_CHASE:
			c->value_int = META_BACK_CMN_CHASE_REFERRALS(mc);
			break;

#ifdef SLAPD_META_CLIENT_PR
		case LDAP_BACK_CFG_CLIENT_PR:
			if ( mc->mc_ps == META_CLIENT_PR_DISABLE ) {
				return 1;
			} else if ( mc->mc_ps == META_CLIENT_PR_ACCEPT_UNSOLICITED ) {
				BER_BVSTR( &bv, "accept-unsolicited" );
			} else {
				bv.bv_len = snprintf( c->cr_msg, sizeof(c->cr_msg), "%d", mc->mc_ps );
				bv.bv_val = c->cr_msg;
			}
			value_add_one( &c->rvalue_vals, &bv );
			break;
#endif /* SLAPD_META_CLIENT_PR */

		case LDAP_BACK_CFG_DEFAULT_T:
			if ( mt || mi->mi_defaulttarget == META_DEFAULT_TARGET_NONE )
				return 1;
			bv.bv_len = snprintf( c->cr_msg, sizeof(c->cr_msg), "%d", mi->mi_defaulttarget );
			bv.bv_val = c->cr_msg;
			value_add_one( &c->rvalue_vals, &bv );
			break;

		case LDAP_BACK_CFG_NETWORK_TIMEOUT:
			if ( mc->mc_network_timeout == 0 ) {
				return 1;
			}
			bv.bv_len = snprintf( c->cr_msg, sizeof(c->cr_msg), "%ld",
				mc->mc_network_timeout );
			bv.bv_val = c->cr_msg;
			value_add_one( &c->rvalue_vals, &bv );
			break;

		case LDAP_BACK_CFG_NOREFS:
			c->value_int = META_BACK_CMN_NOREFS(mc);
			break;

		case LDAP_BACK_CFG_NOUNDEFFILTER:
			c->value_int = META_BACK_CMN_NOUNDEFFILTER(mc);
			break;

		case LDAP_BACK_CFG_NRETRIES:
			if ( mc->mc_nretries == META_RETRY_FOREVER ) {
				BER_BVSTR( &bv, "forever" );
			} else if ( mc->mc_nretries == META_RETRY_NEVER ) {
				BER_BVSTR( &bv, "never" );
			} else {
				bv.bv_len = snprintf( c->cr_msg, sizeof(c->cr_msg), "%d",
					mc->mc_nretries );
				bv.bv_val = c->cr_msg;
			}
			value_add_one( &c->rvalue_vals, &bv );
			break;

		case LDAP_BACK_CFG_QUARANTINE:
			if ( !META_BACK_CMN_QUARANTINE( mc )) {
				rc = 1;
				break;
			}
			rc = mi->mi_ldap_extra->retry_info_unparse( &mc->mc_quarantine, &bv );
			if ( rc == 0 ) {
				ber_bvarray_add( &c->rvalue_vals, &bv );
			}
			break;

		case LDAP_BACK_CFG_REBIND:
			c->value_int = META_BACK_CMN_SAVECRED(mc);
			break;

		case LDAP_BACK_CFG_TIMEOUT:
			for ( i = 0; i < SLAP_OP_LAST; i++ ) {
				if ( mc->mc_timeout[ i ] != 0 ) {
					break;
				}
			}

			if ( i == SLAP_OP_LAST ) {
				return 1;
			}

			BER_BVZERO( &bv );
			slap_cf_aux_table_unparse( mc->mc_timeout, &bv, timeout_table );

			if ( BER_BVISNULL( &bv ) ) {
				return 1;
			}

			for ( i = 0; isspace( (unsigned char) bv.bv_val[ i ] ); i++ )
				/* count spaces */ ;

			if ( i ) {
				bv.bv_len -= i;
				AC_MEMCPY( bv.bv_val, &bv.bv_val[ i ],
					bv.bv_len + 1 );
			}

			ber_bvarray_add( &c->rvalue_vals, &bv );
			break;

		case LDAP_BACK_CFG_VERSION:
			if ( mc->mc_version == 0 )
				return 1;
			c->value_int = mc->mc_version;
			break;

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
		case LDAP_BACK_CFG_ST_REQUEST:
			c->value_int = META_BACK_CMN_ST_REQUEST( mc );
			break;
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

		case LDAP_BACK_CFG_T_F:
			enum_to_verb( t_f_mode, (mc->mc_flags & LDAP_BACK_F_T_F_MASK2), &bv );
			if ( BER_BVISNULL( &bv ) ) {
				/* there's something wrong... */
				assert( 0 );
				rc = 1;

			} else {
				value_add_one( &c->rvalue_vals, &bv );
			}
			break;

		case LDAP_BACK_CFG_TLS: {
			struct berval bc = BER_BVNULL, bv2;

			if (( mc->mc_flags & LDAP_BACK_F_TLS_MASK ) == LDAP_BACK_F_NONE ) {
				rc = 1;
				break;
			}
			enum_to_verb( tls_mode, ( mc->mc_flags & LDAP_BACK_F_TLS_MASK ), &bv );
			assert( !BER_BVISNULL( &bv ) );

			if ( mt ) {
				bindconf_tls_unparse( &mt->mt_tls, &bc );
			}

			if ( !BER_BVISEMPTY( &bc )) {
				bv2.bv_len = bv.bv_len + bc.bv_len + 1;
				bv2.bv_val = ch_malloc( bv2.bv_len + 1 );
				strcpy( bv2.bv_val, bv.bv_val );
				bv2.bv_val[bv.bv_len] = ' ';
				strcpy( &bv2.bv_val[bv.bv_len + 1], bc.bv_val );
				ber_memfree( bc.bv_val );
				ber_bvarray_add( &c->rvalue_vals, &bv2 );
			} else {
				value_add_one( &c->rvalue_vals, &bv );
			}
			} break;

		/* target attrs */
		case LDAP_BACK_CFG_URI: {
			char *p2, *p1 = strchr( mt->mt_uri, ' ' );
			bv.bv_len = strlen( mt->mt_uri ) + 3 + mt->mt_psuffix.bv_len;
			bv.bv_val = ch_malloc( bv.bv_len + 1 );
			p2 = bv.bv_val;
			*p2++ = '"';
			if ( p1 ) {
				p2 = lutil_strncopy( p2, mt->mt_uri, p1 - mt->mt_uri );
			} else {
				p2 = lutil_strcopy( p2, mt->mt_uri );
			}
			*p2++ = '/';
			p2 = lutil_strcopy( p2, mt->mt_psuffix.bv_val );
			*p2++ = '"';
			if ( p1 ) {
				strcpy( p2, p1 );
			}
			ber_bvarray_add( &c->rvalue_vals, &bv );
			} break;

		case LDAP_BACK_CFG_ACL_AUTHCDN:
		case LDAP_BACK_CFG_ACL_PASSWD:
			/* FIXME no point here, there is no code implementing
			 * their features. Was this supposed to implement
			 * acl-bind like back-ldap?
			 */
			rc = 1;
			break;

		case LDAP_BACK_CFG_IDASSERT_AUTHZFROM: {
			BerVarray	*bvp;
			int		i;
			struct berval	bv = BER_BVNULL;
			char		buf[SLAP_TEXT_BUFLEN];

			bvp = &mt->mt_idassert_authz;
			if ( *bvp == NULL ) {
				if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_AUTHZ_ALL )
				{
					BER_BVSTR( &bv, "*" );
					value_add_one( &c->rvalue_vals, &bv );

				} else {
					rc = 1;
				}
				break;
			}

			for ( i = 0; !BER_BVISNULL( &((*bvp)[ i ]) ); i++ ) {
				char *ptr;
				int len = snprintf( buf, sizeof( buf ), SLAP_X_ORDERED_FMT, i );
				bv.bv_len = ((*bvp)[ i ]).bv_len + len;
				bv.bv_val = ber_memrealloc( bv.bv_val, bv.bv_len + 1 );
				ptr = bv.bv_val;
				ptr = lutil_strcopy( ptr, buf );
				ptr = lutil_strncopy( ptr, ((*bvp)[ i ]).bv_val, ((*bvp)[ i ]).bv_len );
				value_add_one( &c->rvalue_vals, &bv );
			}
			if ( bv.bv_val ) {
				ber_memfree( bv.bv_val );
			}
			break;
		}

		case LDAP_BACK_CFG_IDASSERT_BIND: {
			int		i;
			struct berval	bc = BER_BVNULL;
			char		*ptr;

			if ( mt->mt_idassert_authmethod == LDAP_AUTH_NONE ) {
				return 1;
			} else {
				ber_len_t	len;

				switch ( mt->mt_idassert_mode ) {
				case LDAP_BACK_IDASSERT_OTHERID:
				case LDAP_BACK_IDASSERT_OTHERDN:
					break;

				default: {
					struct berval	mode = BER_BVNULL;

					enum_to_verb( idassert_mode, mt->mt_idassert_mode, &mode );
					if ( BER_BVISNULL( &mode ) ) {
						/* there's something wrong... */
						assert( 0 );
						rc = 1;

					} else {
						bv.bv_len = STRLENOF( "mode=" ) + mode.bv_len;
						bv.bv_val = ch_malloc( bv.bv_len + 1 );

						ptr = lutil_strcopy( bv.bv_val, "mode=" );
						ptr = lutil_strcopy( ptr, mode.bv_val );
					}
					break;
				}
				}

				if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_NATIVE_AUTHZ ) {
					len = bv.bv_len + STRLENOF( "authz=native" );

					if ( !BER_BVISEMPTY( &bv ) ) {
						len += STRLENOF( " " );
					}

					bv.bv_val = ch_realloc( bv.bv_val, len + 1 );

					ptr = &bv.bv_val[ bv.bv_len ];

					if ( !BER_BVISEMPTY( &bv ) ) {
						ptr = lutil_strcopy( ptr, " " );
					}

					(void)lutil_strcopy( ptr, "authz=native" );
				}

				len = bv.bv_len + STRLENOF( "flags=non-prescriptive,override,obsolete-encoding-workaround,proxy-authz-non-critical,dn-authzid" );
				/* flags */
				if ( !BER_BVISEMPTY( &bv ) ) {
					len += STRLENOF( " " );
				}

				bv.bv_val = ch_realloc( bv.bv_val, len + 1 );

				ptr = &bv.bv_val[ bv.bv_len ];

				if ( !BER_BVISEMPTY( &bv ) ) {
					ptr = lutil_strcopy( ptr, " " );
				}

				ptr = lutil_strcopy( ptr, "flags=" );

				if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) {
					ptr = lutil_strcopy( ptr, "prescriptive" );
				} else {
					ptr = lutil_strcopy( ptr, "non-prescriptive" );
				}

				if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_OVERRIDE ) {
					ptr = lutil_strcopy( ptr, ",override" );
				}

				if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_OBSOLETE_PROXY_AUTHZ ) {
					ptr = lutil_strcopy( ptr, ",obsolete-proxy-authz" );

				} else if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_OBSOLETE_ENCODING_WORKAROUND ) {
					ptr = lutil_strcopy( ptr, ",obsolete-encoding-workaround" );
				}

				if ( mt->mt_idassert_flags & LDAP_BACK_AUTH_PROXYAUTHZ_CRITICAL ) {
					ptr = lutil_strcopy( ptr, ",proxy-authz-critical" );

				} else {
					ptr = lutil_strcopy( ptr, ",proxy-authz-non-critical" );
				}

#ifdef SLAP_AUTH_DN
				switch ( mt->mt_idassert_flags & LDAP_BACK_AUTH_DN_MASK ) {
				case LDAP_BACK_AUTH_DN_AUTHZID:
					ptr = lutil_strcopy( ptr, ",dn-authzid" );
					break;

				case LDAP_BACK_AUTH_DN_WHOAMI:
					ptr = lutil_strcopy( ptr, ",dn-whoami" );
					break;

				default:
#if 0 /* implicit */
					ptr = lutil_strcopy( ptr, ",dn-none" );
#endif
					break;
				}
#endif

				bv.bv_len = ( ptr - bv.bv_val );
				/* end-of-flags */
			}

			bindconf_unparse( &mt->mt_idassert.si_bc, &bc );

			if ( !BER_BVISNULL( &bv ) ) {
				ber_len_t	len = bv.bv_len + bc.bv_len;

				bv.bv_val = ch_realloc( bv.bv_val, len + 1 );

				assert( bc.bv_val[ 0 ] == ' ' );

				ptr = lutil_strcopy( &bv.bv_val[ bv.bv_len ], bc.bv_val );
				free( bc.bv_val );
				bv.bv_len = ptr - bv.bv_val;

			} else {
				for ( i = 0; isspace( (unsigned char) bc.bv_val[ i ] ); i++ )
					/* count spaces */ ;

				if ( i ) {
					bc.bv_len -= i;
					AC_MEMCPY( bc.bv_val, &bc.bv_val[ i ], bc.bv_len + 1 );
				}

				bv = bc;
			}

			ber_bvarray_add( &c->rvalue_vals, &bv );

			break;
		}

		case LDAP_BACK_CFG_SUFFIXM:	/* unused */
		case LDAP_BACK_CFG_REWRITE:
			if ( mt->mt_rwmap.rwm_bva_rewrite == NULL ) {
				rc = 1;
			} else {
				rc = slap_bv_x_ordered_unparse( mt->mt_rwmap.rwm_bva_rewrite, &c->rvalue_vals );
			}
			break;

		case LDAP_BACK_CFG_MAP:
			if ( mt->mt_rwmap.rwm_bva_map == NULL ) {
				rc = 1;
			} else {
				rc = slap_bv_x_ordered_unparse( mt->mt_rwmap.rwm_bva_map, &c->rvalue_vals );
			}
			break;

		case LDAP_BACK_CFG_SUBTREE_EX:
		case LDAP_BACK_CFG_SUBTREE_IN:
			rc = meta_subtree_unparse( c, mt );
			break;

		case LDAP_BACK_CFG_FILTER:
			if ( mt->mt_filter == NULL ) {
				rc = 1;
			} else {
				metafilter_t *mf;
				for ( mf = mt->mt_filter; mf; mf = mf->mf_next )
					value_add_one( &c->rvalue_vals, &mf->mf_regex_pattern );
			}
			break;

		/* replaced by idassert */
		case LDAP_BACK_CFG_PSEUDOROOTDN:
		case LDAP_BACK_CFG_PSEUDOROOTPW:
			rc = 1;
			break;

		case LDAP_BACK_CFG_KEEPALIVE: {
				struct berval bv;
				char buf[AC_LINE_MAX];
				bv.bv_len = AC_LINE_MAX;
				bv.bv_val = &buf[0];
				slap_keepalive_parse(&bv, &mt->mt_tls.sb_keepalive, 0, 0, 1);
				value_add_one( &c->rvalue_vals, &bv );
				break;
			}

		default:
			rc = 1;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch( c->type ) {
		/* Base attrs */
		case LDAP_BACK_CFG_CONN_TTL:
			mi->mi_conn_ttl = 0;
			break;

		case LDAP_BACK_CFG_DNCACHE_TTL:
			mi->mi_cache.ttl = META_DNCACHE_DISABLED;
			break;

		case LDAP_BACK_CFG_IDLE_TIMEOUT:
			mi->mi_idle_timeout = 0;
			break;

		case LDAP_BACK_CFG_ONERR:
			mi->mi_flags &= ~META_BACK_F_ONERR_MASK;
			break;

		case LDAP_BACK_CFG_PSEUDOROOT_BIND_DEFER:
			mi->mi_flags &= ~META_BACK_F_DEFER_ROOTDN_BIND;
			break;

		case LDAP_BACK_CFG_SINGLECONN:
			mi->mi_flags &= ~LDAP_BACK_F_SINGLECONN;
			break;

		case LDAP_BACK_CFG_USETEMP:
			mi->mi_flags &= ~LDAP_BACK_F_USE_TEMPORARIES;
			break;

		case LDAP_BACK_CFG_CONNPOOLMAX:
			mi->mi_conn_priv_max = LDAP_BACK_CONN_PRIV_MIN;
			break;

		/* common attrs */
		case LDAP_BACK_CFG_BIND_TIMEOUT:
			mc->mc_bind_timeout.tv_sec = 0;
			mc->mc_bind_timeout.tv_usec = 0;
			break;

		case LDAP_BACK_CFG_CANCEL:
			mc->mc_flags &= ~LDAP_BACK_F_CANCEL_MASK2;
			break;

		case LDAP_BACK_CFG_CHASE:
			mc->mc_flags &= ~LDAP_BACK_F_CHASE_REFERRALS;
			break;

#ifdef SLAPD_META_CLIENT_PR
		case LDAP_BACK_CFG_CLIENT_PR:
			mc->mc_ps = META_CLIENT_PR_DISABLE;
			break;
#endif /* SLAPD_META_CLIENT_PR */

		case LDAP_BACK_CFG_DEFAULT_T:
			mi->mi_defaulttarget = META_DEFAULT_TARGET_NONE;
			break;

		case LDAP_BACK_CFG_NETWORK_TIMEOUT:
			mc->mc_network_timeout = 0;
			break;

		case LDAP_BACK_CFG_NOREFS:
			mc->mc_flags &= ~LDAP_BACK_F_NOREFS;
			break;

		case LDAP_BACK_CFG_NOUNDEFFILTER:
			mc->mc_flags &= ~LDAP_BACK_F_NOUNDEFFILTER;
			break;

		case LDAP_BACK_CFG_NRETRIES:
			mc->mc_nretries = META_RETRY_DEFAULT;
			break;

		case LDAP_BACK_CFG_QUARANTINE:
			if ( META_BACK_CMN_QUARANTINE( mc )) {
				mi->mi_ldap_extra->retry_info_destroy( &mc->mc_quarantine );
				mc->mc_flags &= ~LDAP_BACK_F_QUARANTINE;
				if ( mc == &mt->mt_mc ) {
					ldap_pvt_thread_mutex_destroy( &mt->mt_quarantine_mutex );
					mt->mt_isquarantined = 0;
				}
			}
			break;

		case LDAP_BACK_CFG_REBIND:
			mc->mc_flags &= ~LDAP_BACK_F_SAVECRED;
			break;

		case LDAP_BACK_CFG_TIMEOUT:
			for ( i = 0; i < SLAP_OP_LAST; i++ ) {
				mc->mc_timeout[ i ] = 0;
			}
			break;

		case LDAP_BACK_CFG_VERSION:
			mc->mc_version = 0;
			break;

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
		case LDAP_BACK_CFG_ST_REQUEST:
			mc->mc_flags &= ~LDAP_BACK_F_ST_REQUEST;
			break;
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

		case LDAP_BACK_CFG_T_F:
			mc->mc_flags &= ~LDAP_BACK_F_T_F_MASK2;
			break;

		case LDAP_BACK_CFG_TLS:
			mc->mc_flags &= ~LDAP_BACK_F_TLS_MASK;
			if ( mt )
				bindconf_free( &mt->mt_tls );
			break;

		/* target attrs */
		case LDAP_BACK_CFG_URI:
			if ( mt->mt_uri ) {
				ch_free( mt->mt_uri );
				mt->mt_uri = NULL;
			}
			/* FIXME: should have a way to close all cached
			 * connections associated with this target.
			 */
			break;

		case LDAP_BACK_CFG_IDASSERT_AUTHZFROM: {
			BerVarray *bvp;

			bvp = &mt->mt_idassert_authz; break;
			if ( c->valx < 0 ) {
				if ( *bvp != NULL ) {
					ber_bvarray_free( *bvp );
					*bvp = NULL;
				}

			} else {
				if ( *bvp == NULL ) {
					rc = 1;
					break;
				}

				for ( i = 0; !BER_BVISNULL( &((*bvp)[ i ]) ); i++ )
					;

				if ( i >= c->valx ) {
					rc = 1;
					break;
				}
				ber_memfree( ((*bvp)[ c->valx ]).bv_val );
				for ( i = c->valx; !BER_BVISNULL( &((*bvp)[ i + 1 ]) ); i++ ) {
					(*bvp)[ i ] = (*bvp)[ i + 1 ];
				}
				BER_BVZERO( &((*bvp)[ i ]) );
			}
			} break;

		case LDAP_BACK_CFG_IDASSERT_BIND:
			bindconf_free( &mt->mt_idassert.si_bc );
			memset( &mt->mt_idassert, 0, sizeof( slap_idassert_t ) );
			break;

		case LDAP_BACK_CFG_SUFFIXM:	/* unused */
		case LDAP_BACK_CFG_REWRITE:
			if ( mt->mt_rwmap.rwm_bva_rewrite ) {
				ber_bvarray_free( mt->mt_rwmap.rwm_bva_rewrite );
				mt->mt_rwmap.rwm_bva_rewrite = NULL;
			}
			if ( mt->mt_rwmap.rwm_rw )
				rewrite_info_delete( &mt->mt_rwmap.rwm_rw );
			break;

		case LDAP_BACK_CFG_MAP:
			if ( mt->mt_rwmap.rwm_bva_map ) {
				ber_bvarray_free( mt->mt_rwmap.rwm_bva_map );
				mt->mt_rwmap.rwm_bva_map = NULL;
			}
			meta_back_map_free( &mt->mt_rwmap.rwm_oc );
			meta_back_map_free( &mt->mt_rwmap.rwm_at );
			mt->mt_rwmap.rwm_oc.drop_missing = 0;
			mt->mt_rwmap.rwm_at.drop_missing = 0;
			break;

		case LDAP_BACK_CFG_SUBTREE_EX:
		case LDAP_BACK_CFG_SUBTREE_IN:
			/* can only be one of exclude or include */
			if (( c->type == LDAP_BACK_CFG_SUBTREE_EX ) ^ mt->mt_subtree_exclude ) {
				rc = 1;
				break;
			}
			if ( c->valx < 0 ) {
				meta_subtree_destroy( mt->mt_subtree );
				mt->mt_subtree = NULL;
			} else {
				metasubtree_t *ms, **mprev;
				for (i=0, mprev = &mt->mt_subtree, ms = *mprev; ms; ms = *mprev) {
					if ( i == c->valx ) {
						*mprev = ms->ms_next;
						meta_subtree_free( ms );
						break;
					}
					i++;
					mprev = &ms->ms_next;
				}
				if ( i != c->valx )
					rc = 1;
			}
			break;

		case LDAP_BACK_CFG_FILTER:
			if ( c->valx < 0 ) {
				meta_filter_destroy( mt->mt_filter );
				mt->mt_filter = NULL;
			} else {
				metafilter_t *mf, **mprev;
				for (i=0, mprev = &mt->mt_filter, mf = *mprev; mf; mf = *mprev) {
					if ( i == c->valx ) {
						*mprev = mf->mf_next;
						meta_filter_free( mf );
						break;
					}
					i++;
					mprev = &mf->mf_next;
				}
				if ( i != c->valx )
					rc = 1;
			}
			break;

		case LDAP_BACK_CFG_KEEPALIVE:
			mt->mt_tls.sb_keepalive.sk_idle = 0;
			mt->mt_tls.sb_keepalive.sk_probes = 0;
			mt->mt_tls.sb_keepalive.sk_interval = 0;
			break;

		default:
			rc = 1;
			break;
		}

		return rc;
	}

	if ( c->op == SLAP_CONFIG_ADD ) {
		if ( c->type >= LDAP_BACK_CFG_LAST_BASE ) {
			/* exclude CFG_URI from this check */
			if ( c->type > LDAP_BACK_CFG_LAST_BOTH ) {
				if ( !mi->mi_ntargets ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"need \"uri\" directive first" );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					return 1;
				}
			}
			if ( mi->mi_ntargets ) {
				mt = mi->mi_targets[ mi->mi_ntargets-1 ];
				mc = &mt->mt_mc;
			} else {
				mt = NULL;
				mc = &mi->mi_mc;
			}
		}
	} else {
		if ( c->table == Cft_Database ) {
			mt = NULL;
			mc = &mi->mi_mc;
		} else {
			mt = c->ca_private;
			if ( mt )
				mc = &mt->mt_mc;
			else
				mc = NULL;
		}
	}

	switch( c->type ) {
	case LDAP_BACK_CFG_URI: {
		LDAPURLDesc 	*ludp;
		struct berval	dn;
		int		j;

		char		**uris = NULL;

		if ( c->be->be_nsuffix == NULL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"the suffix must be defined before any target" );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}

		i = mi->mi_ntargets++;

		mi->mi_targets = ( metatarget_t ** )ch_realloc( mi->mi_targets,
			sizeof( metatarget_t * ) * mi->mi_ntargets );
		if ( mi->mi_targets == NULL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"out of memory while storing server name"
				" in \"%s <protocol>://<server>[:port]/<naming context>\"",
				c->argv[0] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}

		if ( meta_back_new_target( &mi->mi_targets[ i ] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"unable to init server"
				" in \"%s <protocol>://<server>[:port]/<naming context>\"",
				c->argv[0] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}

		mt = mi->mi_targets[ i ];

		mt->mt_rebind_f = mi->mi_rebind_f;
		mt->mt_urllist_f = mi->mi_urllist_f;
		mt->mt_urllist_p = mt;

		if ( META_BACK_QUARANTINE( mi ) ) {
			ldap_pvt_thread_mutex_init( &mt->mt_quarantine_mutex );
		}
		mt->mt_mc = mi->mi_mc;

		for ( j = 1; j < c->argc; j++ ) {
			char	**tmpuris = ldap_str2charray( c->argv[ j ], "\t" );

			if ( tmpuris == NULL ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to parse URIs #%d"
					" in \"%s <protocol>://<server>[:port]/<naming context>\"",
					j-1, c->argv[0] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}

			if ( j == 1 ) {
				uris = tmpuris;

			} else {
				ldap_charray_merge( &uris, tmpuris );
				ldap_charray_free( tmpuris );
			}
		}

		for ( j = 0; uris[ j ] != NULL; j++ ) {
			char *tmpuri = NULL;

			/*
			 * uri MUST be legal!
			 */
			if ( ldap_url_parselist_ext( &ludp, uris[ j ], "\t",
					LDAP_PVT_URL_PARSE_NONE ) != LDAP_SUCCESS
				|| ludp->lud_next != NULL )
			{
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to parse URI #%d"
					" in \"%s <protocol>://<server>[:port]/<naming context>\"",
					j-1, c->argv[0] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				ldap_charray_free( uris );
				return 1;
			}

			if ( j == 0 ) {

				/*
				 * uri MUST have the <dn> part!
				 */
				if ( ludp->lud_dn == NULL ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"missing <naming context> "
						" in \"%s <protocol>://<server>[:port]/<naming context>\"",
						c->argv[0] );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					ldap_free_urllist( ludp );
					ldap_charray_free( uris );
					return 1;
				}

				/*
				 * copies and stores uri and suffix
				 */
				ber_str2bv( ludp->lud_dn, 0, 0, &dn );
				rc = dnPrettyNormal( NULL, &dn, &mt->mt_psuffix,
					&mt->mt_nsuffix, NULL );
				if ( rc != LDAP_SUCCESS ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"target DN is invalid \"%s\"",
						c->argv[1] );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					ldap_free_urllist( ludp );
					ldap_charray_free( uris );
					return( 1 );
				}

				ludp->lud_dn[ 0 ] = '\0';

				switch ( ludp->lud_scope ) {
				case LDAP_SCOPE_DEFAULT:
					mt->mt_scope = LDAP_SCOPE_SUBTREE;
					break;

				case LDAP_SCOPE_SUBTREE:
				case LDAP_SCOPE_SUBORDINATE:
					mt->mt_scope = ludp->lud_scope;
					break;

				default:
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"invalid scope for target \"%s\"",
						c->argv[1] );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					ldap_free_urllist( ludp );
					ldap_charray_free( uris );
					return( 1 );
				}

			} else {
				/* check all, to apply the scope check on the first one */
				if ( ludp->lud_dn != NULL && ludp->lud_dn[ 0 ] != '\0' ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"multiple URIs must have no DN part" );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					ldap_free_urllist( ludp );
					ldap_charray_free( uris );
					return( 1 );

				}
			}

			tmpuri = ldap_url_list2urls( ludp );
			ldap_free_urllist( ludp );
			if ( tmpuri == NULL ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "no memory?" );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				ldap_charray_free( uris );
				return( 1 );
			}
			ldap_memfree( uris[ j ] );
			uris[ j ] = tmpuri;
		}

		mt->mt_uri = ldap_charray2str( uris, " " );
		ldap_charray_free( uris );
		if ( mt->mt_uri == NULL) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "no memory?" );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		/*
		 * uri MUST be a branch of suffix!
		 */
		for ( j = 0; !BER_BVISNULL( &c->be->be_nsuffix[ j ] ); j++ ) {
			if ( dnIsSuffix( &mt->mt_nsuffix, &c->be->be_nsuffix[ j ] ) ) {
				break;
			}
		}

		if ( BER_BVISNULL( &c->be->be_nsuffix[ j ] ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"<naming context> of URI must be within the naming context of this database." );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		c->ca_private = mt;
		c->cleanup = meta_cf_cleanup;
	} break;
	case LDAP_BACK_CFG_SUBTREE_EX:
	case LDAP_BACK_CFG_SUBTREE_IN:
	/* subtree-exclude */
		if ( meta_subtree_config( mt, c )) {
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		break;

	case LDAP_BACK_CFG_FILTER: {
		metafilter_t *mf, **m2;
		mf = ch_malloc( sizeof( metafilter_t ));
		rc = regcomp( &mf->mf_regex, c->argv[1], REG_EXTENDED );
		if ( rc ) {
			char regerr[ SLAP_TEXT_BUFLEN ];
			regerror( rc, &mf->mf_regex, regerr, sizeof(regerr) );
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"regular expression \"%s\" bad because of %s",
				c->argv[1], regerr );
			ch_free( mf );
			return 1;
		}
		ber_str2bv( c->argv[1], 0, 1, &mf->mf_regex_pattern );
		for ( m2 = &mt->mt_filter; *m2; m2 = &(*m2)->mf_next )
			;
		*m2 = mf;
	} break;

	case LDAP_BACK_CFG_DEFAULT_T:
	/* default target directive */
		i = mi->mi_ntargets - 1;

		if ( c->argc == 1 ) {
 			if ( i < 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"\"%s\" alone must be inside a \"uri\" directive",
					c->argv[0] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
			mi->mi_defaulttarget = i;

		} else {
			if ( strcasecmp( c->argv[ 1 ], "none" ) == 0 ) {
				if ( i >= 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"\"%s none\" should go before uri definitions",
						c->argv[0] );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				}
				mi->mi_defaulttarget = META_DEFAULT_TARGET_NONE;

			} else {

				if ( lutil_atoi( &mi->mi_defaulttarget, c->argv[ 1 ] ) != 0
					|| mi->mi_defaulttarget < 0
					|| mi->mi_defaulttarget >= i - 1 )
				{
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"illegal target number %d",
						mi->mi_defaulttarget );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					return 1;
				}
			}
		}
		break;

	case LDAP_BACK_CFG_DNCACHE_TTL:
	/* ttl of dn cache */
		if ( strcasecmp( c->argv[ 1 ], "forever" ) == 0 ) {
			mi->mi_cache.ttl = META_DNCACHE_FOREVER;

		} else if ( strcasecmp( c->argv[ 1 ], "disabled" ) == 0 ) {
			mi->mi_cache.ttl = META_DNCACHE_DISABLED;

		} else {
			unsigned long	t;

			if ( lutil_parse_time( c->argv[ 1 ], &t ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to parse dncache ttl \"%s\"",
					c->argv[ 1 ] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
			mi->mi_cache.ttl = (time_t)t;
		}
		break;

	case LDAP_BACK_CFG_NETWORK_TIMEOUT: {
	/* network timeout when connecting to ldap servers */
		unsigned long t;

		if ( lutil_parse_time( c->argv[ 1 ], &t ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"unable to parse network timeout \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		mc->mc_network_timeout = (time_t)t;
		} break;

	case LDAP_BACK_CFG_IDLE_TIMEOUT: {
	/* idle timeout when connecting to ldap servers */
		unsigned long	t;

		if ( lutil_parse_time( c->argv[ 1 ], &t ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"unable to parse idle timeout \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;

		}
		mi->mi_idle_timeout = (time_t)t;
		} break;

	case LDAP_BACK_CFG_CONN_TTL: {
	/* conn ttl */
		unsigned long	t;

		if ( lutil_parse_time( c->argv[ 1 ], &t ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"unable to parse conn ttl \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;

		}
		mi->mi_conn_ttl = (time_t)t;
		} break;

	case LDAP_BACK_CFG_BIND_TIMEOUT:
	/* bind timeout when connecting to ldap servers */
		mc->mc_bind_timeout.tv_sec = c->value_ulong/1000000;
		mc->mc_bind_timeout.tv_usec = c->value_ulong%1000000;
		break;

	case LDAP_BACK_CFG_ACL_AUTHCDN:
	/* name to use for meta_back_group */
		if ( strcasecmp( c->argv[ 0 ], "binddn" ) == 0 ) {
			Debug( LDAP_DEBUG_ANY, "%s: "
				"\"binddn\" statement is deprecated; "
				"use \"acl-authcDN\" instead\n",
				c->log, 0, 0 );
			/* FIXME: some day we'll need to throw an error */
		}

		ber_memfree_x( c->value_dn.bv_val, NULL );
		mt->mt_binddn = c->value_ndn;
		BER_BVZERO( &c->value_dn );
		BER_BVZERO( &c->value_ndn );
		break;

	case LDAP_BACK_CFG_ACL_PASSWD:
	/* password to use for meta_back_group */
		if ( strcasecmp( c->argv[ 0 ], "bindpw" ) == 0 ) {
			Debug( LDAP_DEBUG_ANY, "%s "
				"\"bindpw\" statement is deprecated; "
				"use \"acl-passwd\" instead\n",
				c->log, 0, 0 );
			/* FIXME: some day we'll need to throw an error */
		}

		ber_str2bv( c->argv[ 1 ], 0L, 1, &mt->mt_bindpw );
		break;

	case LDAP_BACK_CFG_REBIND:
	/* save bind creds for referral rebinds? */
		if ( c->argc == 1 || c->value_int ) {
			mc->mc_flags |= LDAP_BACK_F_SAVECRED;
		} else {
			mc->mc_flags &= ~LDAP_BACK_F_SAVECRED;
		}
		break;

	case LDAP_BACK_CFG_CHASE:
		if ( c->argc == 1 || c->value_int ) {
			mc->mc_flags |= LDAP_BACK_F_CHASE_REFERRALS;
		} else {
			mc->mc_flags &= ~LDAP_BACK_F_CHASE_REFERRALS;
		}
		break;

	case LDAP_BACK_CFG_TLS:
		i = verb_to_mask( c->argv[1], tls_mode );
		if ( BER_BVISNULL( &tls_mode[i].word ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"%s unknown argument \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		mc->mc_flags &= ~LDAP_BACK_F_TLS_MASK;
		mc->mc_flags |= tls_mode[i].mask;

		if ( c->argc > 2 ) {
			if ( c->op == SLAP_CONFIG_ADD && mi->mi_ntargets == 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"need \"uri\" directive first" );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}

			for ( i = 2; i < c->argc; i++ ) {
				if ( bindconf_tls_parse( c->argv[i], &mt->mt_tls ))
					return 1;
			}
			bindconf_tls_defaults( &mt->mt_tls );
		}
		break;

	case LDAP_BACK_CFG_T_F:
		i = verb_to_mask( c->argv[1], t_f_mode );
		if ( BER_BVISNULL( &t_f_mode[i].word ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"%s unknown argument \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		mc->mc_flags &= ~LDAP_BACK_F_T_F_MASK2;
		mc->mc_flags |= t_f_mode[i].mask;
		break;

	case LDAP_BACK_CFG_ONERR:
	/* onerr? */
		i = verb_to_mask( c->argv[1], onerr_mode );
		if ( BER_BVISNULL( &onerr_mode[i].word ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"%s unknown argument \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		mi->mi_flags &= ~META_BACK_F_ONERR_MASK;
		mi->mi_flags |= onerr_mode[i].mask;
		break;

	case LDAP_BACK_CFG_PSEUDOROOT_BIND_DEFER:
	/* bind-defer? */
		if ( c->argc == 1 || c->value_int ) {
			mi->mi_flags |= META_BACK_F_DEFER_ROOTDN_BIND;
		} else {
			mi->mi_flags &= ~META_BACK_F_DEFER_ROOTDN_BIND;
		}
		break;

	case LDAP_BACK_CFG_SINGLECONN:
	/* single-conn? */
		if ( mi->mi_ntargets > 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"%s\" must appear before target definitions",
				c->argv[0] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( c->value_int ) {
			mi->mi_flags |= LDAP_BACK_F_SINGLECONN;
		} else {
			mi->mi_flags &= ~LDAP_BACK_F_SINGLECONN;
		}
		break;

	case LDAP_BACK_CFG_USETEMP:
	/* use-temporaries? */
		if ( mi->mi_ntargets > 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"%s\" must appear before target definitions",
				c->argv[0] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( c->value_int ) {
			mi->mi_flags |= LDAP_BACK_F_USE_TEMPORARIES;
		} else {
			mi->mi_flags &= ~LDAP_BACK_F_USE_TEMPORARIES;
		}
		break;

	case LDAP_BACK_CFG_CONNPOOLMAX:
	/* privileged connections pool max size ? */
		if ( mi->mi_ntargets > 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"%s\" must appear before target definitions",
				c->argv[0] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( c->value_int < LDAP_BACK_CONN_PRIV_MIN
			|| c->value_int > LDAP_BACK_CONN_PRIV_MAX )
		{
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"invalid max size " "of privileged "
				"connections pool \"%s\" "
				"in \"conn-pool-max <n> "
				"(must be between %d and %d)\"",
				c->argv[ 1 ],
				LDAP_BACK_CONN_PRIV_MIN,
				LDAP_BACK_CONN_PRIV_MAX );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		mi->mi_conn_priv_max = c->value_int;
		break;

	case LDAP_BACK_CFG_CANCEL:
		i = verb_to_mask( c->argv[1], cancel_mode );
		if ( BER_BVISNULL( &cancel_mode[i].word ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"%s unknown argument \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		mc->mc_flags &= ~LDAP_BACK_F_CANCEL_MASK2;
		mc->mc_flags |= t_f_mode[i].mask;
		break;

	case LDAP_BACK_CFG_TIMEOUT:
		for ( i = 1; i < c->argc; i++ ) {
			if ( isdigit( (unsigned char) c->argv[ i ][ 0 ] ) ) {
				int		j;
				unsigned	u;

				if ( lutil_atoux( &u, c->argv[ i ], 0 ) != 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg),
						"unable to parse timeout \"%s\"",
						c->argv[ i ] );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					return 1;
				}

				for ( j = 0; j < SLAP_OP_LAST; j++ ) {
					mc->mc_timeout[ j ] = u;
				}

				continue;
			}

			if ( slap_cf_aux_table_parse( c->argv[ i ], mc->mc_timeout, timeout_table, "slapd-meta timeout" ) ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg),
					"unable to parse timeout \"%s\"",
					c->argv[ i ] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
		}
		break;

	case LDAP_BACK_CFG_PSEUDOROOTDN:
	/* name to use as pseudo-root dn */
		/*
		 * exact replacement:
		 *

idassert-bind	bindmethod=simple
		binddn=<pseudorootdn>
		credentials=<pseudorootpw>
		mode=none
		flags=non-prescriptive
idassert-authzFrom	"dn:<rootdn>"

		 * so that only when authc'd as <rootdn> the proxying occurs
		 * rebinding as the <pseudorootdn> without proxyAuthz.
		 */

		Debug( LDAP_DEBUG_ANY,
			"%s: \"pseudorootdn\", \"pseudorootpw\" are no longer supported; "
			"use \"idassert-bind\" and \"idassert-authzFrom\" instead.\n",
			c->log, 0, 0 );

		{
			char	binddn[ SLAP_TEXT_BUFLEN ];
			char	*cargv[] = {
				"idassert-bind",
				"bindmethod=simple",
				NULL,
				"mode=none",
				"flags=non-prescriptive",
				NULL
			};
			char **oargv;
			int oargc;
			int	cargc = 5;
			int	rc;


			if ( BER_BVISNULL( &c->be->be_rootndn ) ) {
				Debug( LDAP_DEBUG_ANY, "%s: \"pseudorootpw\": \"rootdn\" must be defined first.\n",
					c->log, 0, 0 );
				return 1;
			}

			if ( sizeof( binddn ) <= (unsigned) snprintf( binddn,
					sizeof( binddn ), "binddn=%s", c->argv[ 1 ] ))
			{
				Debug( LDAP_DEBUG_ANY, "%s: \"pseudorootdn\" too long.\n",
					c->log, 0, 0 );
				return 1;
			}
			cargv[ 2 ] = binddn;

			oargv = c->argv;
			oargc = c->argc;
			c->argv = cargv;
			c->argc = cargc;
			rc = mi->mi_ldap_extra->idassert_parse( c, &mt->mt_idassert );
			c->argv = oargv;
			c->argc = oargc;
			if ( rc == 0 ) {
				struct berval	bv;

				if ( mt->mt_idassert_authz != NULL ) {
					Debug( LDAP_DEBUG_ANY, "%s: \"idassert-authzFrom\" already defined (discarded).\n",
						c->log, 0, 0 );
					ber_bvarray_free( mt->mt_idassert_authz );
					mt->mt_idassert_authz = NULL;
				}

				assert( !BER_BVISNULL( &mt->mt_idassert_authcDN ) );

				bv.bv_len = STRLENOF( "dn:" ) + c->be->be_rootndn.bv_len;
				bv.bv_val = ber_memalloc( bv.bv_len + 1 );
				AC_MEMCPY( bv.bv_val, "dn:", STRLENOF( "dn:" ) );
				AC_MEMCPY( &bv.bv_val[ STRLENOF( "dn:" ) ], c->be->be_rootndn.bv_val, c->be->be_rootndn.bv_len + 1 );

				ber_bvarray_add( &mt->mt_idassert_authz, &bv );
			}

			return rc;
		}
		break;

	case LDAP_BACK_CFG_PSEUDOROOTPW:
	/* password to use as pseudo-root */
		Debug( LDAP_DEBUG_ANY,
			"%s: \"pseudorootdn\", \"pseudorootpw\" are no longer supported; "
			"use \"idassert-bind\" and \"idassert-authzFrom\" instead.\n",
			c->log, 0, 0 );

		if ( BER_BVISNULL( &mt->mt_idassert_authcDN ) ) {
			Debug( LDAP_DEBUG_ANY, "%s: \"pseudorootpw\": \"pseudorootdn\" must be defined first.\n",
				c->log, 0, 0 );
			return 1;
		}

		if ( !BER_BVISNULL( &mt->mt_idassert_passwd ) ) {
			memset( mt->mt_idassert_passwd.bv_val, 0,
				mt->mt_idassert_passwd.bv_len );
			ber_memfree( mt->mt_idassert_passwd.bv_val );
		}
		ber_str2bv( c->argv[ 1 ], 0, 1, &mt->mt_idassert_passwd );
		break;

	case LDAP_BACK_CFG_IDASSERT_BIND:
	/* idassert-bind */
		rc = mi->mi_ldap_extra->idassert_parse( c, &mt->mt_idassert );
		break;

	case LDAP_BACK_CFG_IDASSERT_AUTHZFROM:
	/* idassert-authzFrom */
		rc = mi->mi_ldap_extra->idassert_authzfrom_parse( c, &mt->mt_idassert );
		break;

	case LDAP_BACK_CFG_QUARANTINE:
	/* quarantine */
		if ( META_BACK_CMN_QUARANTINE( mc ) )
		{
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"quarantine already defined" );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}

		if ( mt ) {
			mc->mc_quarantine.ri_interval = NULL;
			mc->mc_quarantine.ri_num = NULL;
			if ( !META_BACK_QUARANTINE( mi ) ) {
				ldap_pvt_thread_mutex_init( &mt->mt_quarantine_mutex );
			}
		}

		if ( mi->mi_ldap_extra->retry_info_parse( c->argv[ 1 ], &mc->mc_quarantine, c->cr_msg, sizeof( c->cr_msg ) ) ) {
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}

		mc->mc_flags |= LDAP_BACK_F_QUARANTINE;
		break;

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
	case LDAP_BACK_CFG_ST_REQUEST:
	/* session tracking request */
		if ( c->value_int ) {
			mc->mc_flags |= LDAP_BACK_F_ST_REQUEST;
		} else {
			mc->mc_flags &= ~LDAP_BACK_F_ST_REQUEST;
		}
		break;
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

	case LDAP_BACK_CFG_SUFFIXM:	/* FALLTHRU */
	case LDAP_BACK_CFG_REWRITE: {
	/* rewrite stuff ... */
		ConfigArgs ca = { 0 };
		char *line, **argv;
		struct rewrite_info *rwi;
		int cnt = 0, argc, ix = c->valx;

		if ( mt->mt_rwmap.rwm_bva_rewrite ) {
			for ( ; !BER_BVISNULL( &mt->mt_rwmap.rwm_bva_rewrite[ cnt ] ); cnt++ )
				/* count */ ;
		}

		if ( ix >= cnt || ix < 0 ) {
			ix = cnt;
		} else {
			rwi = mt->mt_rwmap.rwm_rw;

			mt->mt_rwmap.rwm_rw = NULL;
			rc = meta_rwi_init( &mt->mt_rwmap.rwm_rw );

			/* re-parse all rewrite rules, up to the one
			 * that needs to be added */
			ca.fname = c->fname;
			ca.lineno = c->lineno;
			for ( i = 0; i < ix; i++ ) {
				ca.line = mt->mt_rwmap.rwm_bva_rewrite[ i ].bv_val;
				ca.argc = 0;
				config_fp_parse_line( &ca );

				if ( !strcasecmp( ca.argv[0], "suffixmassage" )) {
					rc = meta_suffixm_config( &ca, ca.argc, ca.argv, mt );
				} else {
					rc = rewrite_parse( mt->mt_rwmap.rwm_rw,
						c->fname, c->lineno, ca.argc, ca.argv );
				}
				assert( rc == 0 );
				ch_free( ca.argv );
				ch_free( ca.tline );
			}
		}
		argc = c->argc;
		argv = c->argv;
		if ( c->op != SLAP_CONFIG_ADD ) {
			argc--;
			argv++;
		}
		/* add the new rule */
		if ( !strcasecmp( argv[0], "suffixmassage" )) {
			rc = meta_suffixm_config( c, argc, argv, mt );
		} else {
			rc = rewrite_parse( mt->mt_rwmap.rwm_rw,
						c->fname, c->lineno, argc, argv );
		}
		if ( rc ) {
			if ( ix < cnt ) {
				rewrite_info_delete( &mt->mt_rwmap.rwm_rw );
				mt->mt_rwmap.rwm_rw = rwi;
			}
			return 1;
		}
		if ( ix < cnt ) {
			for ( ; i < cnt; i++ ) {
				ca.line = mt->mt_rwmap.rwm_bva_rewrite[ i ].bv_val;
				ca.argc = 0;
				config_fp_parse_line( &ca );

				if ( !strcasecmp( ca.argv[0], "suffixmassage" )) {
					rc = meta_suffixm_config( &ca, ca.argc, ca.argv, mt );
				} else {
					rc = rewrite_parse( mt->mt_rwmap.rwm_rw,
						c->fname, c->lineno, ca.argc, argv );
				}
				assert( rc == 0 );
				ch_free( ca.argv );
				ch_free( ca.tline );
			}
		}

		/* save the rule info */
		line = ldap_charray2str( argv, "\" \"" );
		if ( line != NULL ) {
			struct berval bv;
			int len = strlen( argv[ 0 ] );

			ber_str2bv( line, 0, 0, &bv );
			AC_MEMCPY( &bv.bv_val[ len ], &bv.bv_val[ len + 1 ],
				bv.bv_len - ( len + 1 ));
			bv.bv_val[ bv.bv_len - 1] = '"';
			ber_bvarray_add( &mt->mt_rwmap.rwm_bva_rewrite, &bv );
			/* move it to the right slot */
			if ( ix < cnt ) {
				for ( i=cnt; i>ix; i-- )
					mt->mt_rwmap.rwm_bva_rewrite[i+1] = mt->mt_rwmap.rwm_bva_rewrite[i];
				mt->mt_rwmap.rwm_bva_rewrite[i] = bv;

				/* destroy old rules */
				rewrite_info_delete( &rwi );
			}
		}
		} break;

	case LDAP_BACK_CFG_MAP: {
	/* objectclass/attribute mapping */
		ConfigArgs ca = { 0 };
		char *argv[5];
		struct ldapmap rwm_oc;
		struct ldapmap rwm_at;
		int cnt = 0, ix = c->valx;

		if ( mt->mt_rwmap.rwm_bva_map ) {
			for ( ; !BER_BVISNULL( &mt->mt_rwmap.rwm_bva_map[ cnt ] ); cnt++ )
				/* count */ ;
		}

		if ( ix >= cnt || ix < 0 ) {
			ix = cnt;
		} else {
			rwm_oc = mt->mt_rwmap.rwm_oc;
			rwm_at = mt->mt_rwmap.rwm_at;

			memset( &mt->mt_rwmap.rwm_oc, 0, sizeof( mt->mt_rwmap.rwm_oc ) );
			memset( &mt->mt_rwmap.rwm_at, 0, sizeof( mt->mt_rwmap.rwm_at ) );

			/* re-parse all mappings, up to the one
			 * that needs to be added */
			argv[0] = c->argv[0];
			ca.fname = c->fname;
			ca.lineno = c->lineno;
			for ( i = 0; i < ix; i++ ) {
				ca.line = mt->mt_rwmap.rwm_bva_map[ i ].bv_val;
				ca.argc = 0;
				config_fp_parse_line( &ca );

				argv[1] = ca.argv[0];
				argv[2] = ca.argv[1];
				argv[3] = ca.argv[2];
				argv[4] = ca.argv[3];
				ch_free( ca.argv );
				ca.argv = argv;
				ca.argc++;
				rc = ldap_back_map_config( &ca, &mt->mt_rwmap.rwm_oc,
					&mt->mt_rwmap.rwm_at );

				ch_free( ca.tline );
				ca.tline = NULL;
				ca.argv = NULL;

				/* in case of failure, restore
				 * the existing mapping */
				if ( rc ) {
					goto map_fail;
				}
			}
		}
		/* add the new mapping */
		rc = ldap_back_map_config( c, &mt->mt_rwmap.rwm_oc,
					&mt->mt_rwmap.rwm_at );
		if ( rc ) {
			goto map_fail;
		}

		if ( ix < cnt ) {
			for ( ; i<cnt ; cnt++ ) {
				ca.line = mt->mt_rwmap.rwm_bva_map[ i ].bv_val;
				ca.argc = 0;
				config_fp_parse_line( &ca );

				argv[1] = ca.argv[0];
				argv[2] = ca.argv[1];
				argv[3] = ca.argv[2];
				argv[4] = ca.argv[3];

				ch_free( ca.argv );
				ca.argv = argv;
				ca.argc++;
				rc = ldap_back_map_config( &ca, &mt->mt_rwmap.rwm_oc,
					&mt->mt_rwmap.rwm_at );

				ch_free( ca.tline );
				ca.tline = NULL;
				ca.argv = NULL;

				/* in case of failure, restore
				 * the existing mapping */
				if ( rc ) {
					goto map_fail;
				}
			}
		}

		/* save the map info */
		argv[0] = ldap_charray2str( &c->argv[ 1 ], " " );
		if ( argv[0] != NULL ) {
			struct berval bv;
			ber_str2bv( argv[0], 0, 0, &bv );
			ber_bvarray_add( &mt->mt_rwmap.rwm_bva_map, &bv );
			/* move it to the right slot */
			if ( ix < cnt ) {
				for ( i=cnt; i>ix; i-- )
					mt->mt_rwmap.rwm_bva_map[i+1] = mt->mt_rwmap.rwm_bva_map[i];
				mt->mt_rwmap.rwm_bva_map[i] = bv;

				/* destroy old mapping */
				meta_back_map_free( &rwm_oc );
				meta_back_map_free( &rwm_at );
			}
		}
		break;

map_fail:;
		if ( ix < cnt ) {
			meta_back_map_free( &mt->mt_rwmap.rwm_oc );
			meta_back_map_free( &mt->mt_rwmap.rwm_at );
			mt->mt_rwmap.rwm_oc = rwm_oc;
			mt->mt_rwmap.rwm_at = rwm_at;
		}
		} break;

	case LDAP_BACK_CFG_NRETRIES: {
		int		nretries = META_RETRY_UNDEFINED;

		if ( strcasecmp( c->argv[ 1 ], "forever" ) == 0 ) {
			nretries = META_RETRY_FOREVER;

		} else if ( strcasecmp( c->argv[ 1 ], "never" ) == 0 ) {
			nretries = META_RETRY_NEVER;

		} else {
			if ( lutil_atoi( &nretries, c->argv[ 1 ] ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to parse nretries {never|forever|<retries>}: \"%s\"",
					c->argv[ 1 ] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
		}

		mc->mc_nretries = nretries;
		} break;

	case LDAP_BACK_CFG_VERSION:
		if ( c->value_int != 0 && ( c->value_int < LDAP_VERSION_MIN || c->value_int > LDAP_VERSION_MAX ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"unsupported protocol version \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		mc->mc_version = c->value_int;
		break;

	case LDAP_BACK_CFG_NOREFS:
	/* do not return search references */
		if ( c->value_int ) {
			mc->mc_flags |= LDAP_BACK_F_NOREFS;
		} else {
			mc->mc_flags &= ~LDAP_BACK_F_NOREFS;
		}
		break;

	case LDAP_BACK_CFG_NOUNDEFFILTER:
	/* do not propagate undefined search filters */
		if ( c->value_int ) {
			mc->mc_flags |= LDAP_BACK_F_NOUNDEFFILTER;
		} else {
			mc->mc_flags &= ~LDAP_BACK_F_NOUNDEFFILTER;
		}
		break;

#ifdef SLAPD_META_CLIENT_PR
	case LDAP_BACK_CFG_CLIENT_PR:
		if ( strcasecmp( c->argv[ 1 ], "accept-unsolicited" ) == 0 ) {
			mc->mc_ps = META_CLIENT_PR_ACCEPT_UNSOLICITED;

		} else if ( strcasecmp( c->argv[ 1 ], "disable" ) == 0 ) {
			mc->mc_ps = META_CLIENT_PR_DISABLE;

		} else if ( lutil_atoi( &mc->mc_ps, c->argv[ 1 ] ) || mc->mc_ps < -1 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"unable to parse client-pr {accept-unsolicited|disable|<size>}: \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		break;
#endif /* SLAPD_META_CLIENT_PR */

	case LDAP_BACK_CFG_KEEPALIVE:
		slap_keepalive_parse( ber_bvstrdup(c->argv[1]),
				 &mt->mt_tls.sb_keepalive, 0, 0, 0);
		break;

	/* anything else */
	default:
		return SLAP_CONF_UNKNOWN;
	}

	return rc;
}

int
meta_back_init_cf( BackendInfo *bi )
{
	int			rc;
	AttributeDescription	*ad = NULL;
	const char		*text;

	/* Make sure we don't exceed the bits reserved for userland */
	config_check_userland( LDAP_BACK_CFG_LAST );

	bi->bi_cf_ocs = metaocs;

	rc = config_register_schema( metacfg, metaocs );
	if ( rc ) {
		return rc;
	}

	/* setup olcDbAclPasswd and olcDbIDAssertPasswd
	 * to be base64-encoded when written in LDIF form;
	 * basically, we don't care if it fails */
	rc = slap_str2ad( "olcDbACLPasswd", &ad, &text );
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY, "config_back_initialize: "
			"warning, unable to get \"olcDbACLPasswd\" "
			"attribute description: %d: %s\n",
			rc, text, 0 );
	} else {
		(void)ldif_must_b64_encode_register( ad->ad_cname.bv_val,
			ad->ad_type->sat_oid );
	}

	ad = NULL;
	rc = slap_str2ad( "olcDbIDAssertPasswd", &ad, &text );
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY, "config_back_initialize: "
			"warning, unable to get \"olcDbIDAssertPasswd\" "
			"attribute description: %d: %s\n",
			rc, text, 0 );
	} else {
		(void)ldif_must_b64_encode_register( ad->ad_cname.bv_val,
			ad->ad_type->sat_oid );
	}

	return 0;
}

static int
ldap_back_map_config(
		ConfigArgs *c,
		struct ldapmap	*oc_map,
		struct ldapmap	*at_map )
{
	struct ldapmap		*map;
	struct ldapmapping	*mapping;
	char			*src, *dst;
	int			is_oc = 0;

	if ( strcasecmp( c->argv[ 1 ], "objectclass" ) == 0 ) {
		map = oc_map;
		is_oc = 1;

	} else if ( strcasecmp( c->argv[ 1 ], "attribute" ) == 0 ) {
		map = at_map;

	} else {
		snprintf( c->cr_msg, sizeof(c->cr_msg),
			"%s unknown argument \"%s\"",
			c->argv[0], c->argv[1] );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		return 1;
	}

	if ( !is_oc && map->map == NULL ) {
		/* only init if required */
		ldap_back_map_init( map, &mapping );
	}

	if ( strcmp( c->argv[ 2 ], "*" ) == 0 ) {
		if ( c->argc < 4 || strcmp( c->argv[ 3 ], "*" ) == 0 ) {
			map->drop_missing = ( c->argc < 4 );
			goto success_return;
		}
		src = dst = c->argv[ 3 ];

	} else if ( c->argc < 4 ) {
		src = "";
		dst = c->argv[ 2 ];

	} else {
		src = c->argv[ 2 ];
		dst = ( strcmp( c->argv[ 3 ], "*" ) == 0 ? src : c->argv[ 3 ] );
	}

	if ( ( map == at_map )
		&& ( strcasecmp( src, "objectclass" ) == 0
			|| strcasecmp( dst, "objectclass" ) == 0 ) )
	{
		snprintf( c->cr_msg, sizeof(c->cr_msg),
			"objectclass attribute cannot be mapped" );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		return 1;
	}

	mapping = (struct ldapmapping *)ch_calloc( 2,
		sizeof(struct ldapmapping) );
	if ( mapping == NULL ) {
		snprintf( c->cr_msg, sizeof(c->cr_msg),
			"out of memory" );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		return 1;
	}
	ber_str2bv( src, 0, 1, &mapping[ 0 ].src );
	ber_str2bv( dst, 0, 1, &mapping[ 0 ].dst );
	mapping[ 1 ].src = mapping[ 0 ].dst;
	mapping[ 1 ].dst = mapping[ 0 ].src;

	/*
	 * schema check
	 */
	if ( is_oc ) {
		if ( src[ 0 ] != '\0' ) {
			if ( oc_bvfind( &mapping[ 0 ].src ) == NULL ) {
				Debug( LDAP_DEBUG_ANY,
	"warning, source objectClass '%s' should be defined in schema\n",
					c->log, src, 0 );

				/*
				 * FIXME: this should become an err
				 */
				goto error_return;
			}
		}

		if ( oc_bvfind( &mapping[ 0 ].dst ) == NULL ) {
			Debug( LDAP_DEBUG_ANY,
	"warning, destination objectClass '%s' is not defined in schema\n",
				c->log, dst, 0 );
		}
	} else {
		int			rc;
		const char		*text = NULL;
		AttributeDescription	*ad = NULL;

		if ( src[ 0 ] != '\0' ) {
			rc = slap_bv2ad( &mapping[ 0 ].src, &ad, &text );
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY,
	"warning, source attributeType '%s' should be defined in schema\n",
					c->log, src, 0 );

				/*
				 * FIXME: this should become an err
				 */
				/*
				 * we create a fake "proxied" ad
				 * and add it here.
				 */

				rc = slap_bv2undef_ad( &mapping[ 0 ].src,
						&ad, &text, SLAP_AD_PROXIED );
				if ( rc != LDAP_SUCCESS ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"source attributeType \"%s\": %d (%s)",
						src, rc, text ? text : "" );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					goto error_return;
				}
			}

			ad = NULL;
		}

		rc = slap_bv2ad( &mapping[ 0 ].dst, &ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
	"warning, destination attributeType '%s' is not defined in schema\n",
				c->log, dst, 0 );

			/*
			 * we create a fake "proxied" ad
			 * and add it here.
			 */

			rc = slap_bv2undef_ad( &mapping[ 0 ].dst,
					&ad, &text, SLAP_AD_PROXIED );
			if ( rc != LDAP_SUCCESS ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"destination attributeType \"%s\": %d (%s)\n",
					dst, rc, text ? text : "" );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
		}
	}

	if ( (src[ 0 ] != '\0' && avl_find( map->map, (caddr_t)&mapping[ 0 ], mapping_cmp ) != NULL)
			|| avl_find( map->remap, (caddr_t)&mapping[ 1 ], mapping_cmp ) != NULL)
	{
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			"duplicate mapping found." );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		goto error_return;
	}

	if ( src[ 0 ] != '\0' ) {
		avl_insert( &map->map, (caddr_t)&mapping[ 0 ],
					mapping_cmp, mapping_dup );
	}
	avl_insert( &map->remap, (caddr_t)&mapping[ 1 ],
				mapping_cmp, mapping_dup );

success_return:;
	return 0;

error_return:;
	if ( mapping ) {
		ch_free( mapping[ 0 ].src.bv_val );
		ch_free( mapping[ 0 ].dst.bv_val );
		ch_free( mapping );
	}

	return 1;
}


#ifdef ENABLE_REWRITE
static char *
suffix_massage_regexize( const char *s )
{
	char *res, *ptr;
	const char *p, *r;
	int i;

	if ( s[ 0 ] == '\0' ) {
		return ch_strdup( "^(.+)$" );
	}

	for ( i = 0, p = s;
			( r = strchr( p, ',' ) ) != NULL;
			p = r + 1, i++ )
		;

	res = ch_calloc( sizeof( char ),
			strlen( s )
			+ STRLENOF( "((.+),)?" )
			+ STRLENOF( "[ ]?" ) * i
			+ STRLENOF( "$" ) + 1 );

	ptr = lutil_strcopy( res, "((.+),)?" );
	for ( i = 0, p = s;
			( r = strchr( p, ',' ) ) != NULL;
			p = r + 1 , i++ ) {
		ptr = lutil_strncopy( ptr, p, r - p + 1 );
		ptr = lutil_strcopy( ptr, "[ ]?" );

		if ( r[ 1 ] == ' ' ) {
			r++;
		}
	}
	ptr = lutil_strcopy( ptr, p );
	ptr[ 0 ] = '$';
	ptr++;
	ptr[ 0 ] = '\0';

	return res;
}

static char *
suffix_massage_patternize( const char *s, const char *p )
{
	ber_len_t	len;
	char		*res, *ptr;

	len = strlen( p );

	if ( s[ 0 ] == '\0' ) {
		len++;
	}

	res = ch_calloc( sizeof( char ), len + STRLENOF( "%1" ) + 1 );
	if ( res == NULL ) {
		return NULL;
	}

	ptr = lutil_strcopy( res, ( p[ 0 ] == '\0' ? "%2" : "%1" ) );
	if ( s[ 0 ] == '\0' ) {
		ptr[ 0 ] = ',';
		ptr++;
	}
	lutil_strcopy( ptr, p );

	return res;
}

int
suffix_massage_config(
		struct rewrite_info *info,
		struct berval *pvnc,
		struct berval *nvnc,
		struct berval *prnc,
		struct berval *nrnc
)
{
	char *rargv[ 5 ];
	int line = 0;

	rargv[ 0 ] = "rewriteEngine";
	rargv[ 1 ] = "on";
	rargv[ 2 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 2, rargv );

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "default";
	rargv[ 2 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 2, rargv );

	rargv[ 0 ] = "rewriteRule";
	rargv[ 1 ] = suffix_massage_regexize( pvnc->bv_val );
	rargv[ 2 ] = suffix_massage_patternize( pvnc->bv_val, prnc->bv_val );
	rargv[ 3 ] = ":";
	rargv[ 4 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );
	ch_free( rargv[ 1 ] );
	ch_free( rargv[ 2 ] );

	if ( BER_BVISEMPTY( pvnc ) ) {
		rargv[ 0 ] = "rewriteRule";
		rargv[ 1 ] = "^$";
		rargv[ 2 ] = prnc->bv_val;
		rargv[ 3 ] = ":";
		rargv[ 4 ] = NULL;
		rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );
	}

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "searchEntryDN";
	rargv[ 2 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 2, rargv );

	rargv[ 0 ] = "rewriteRule";
	rargv[ 1 ] = suffix_massage_regexize( prnc->bv_val );
	rargv[ 2 ] = suffix_massage_patternize( prnc->bv_val, pvnc->bv_val );
	rargv[ 3 ] = ":";
	rargv[ 4 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );
	ch_free( rargv[ 1 ] );
	ch_free( rargv[ 2 ] );

	if ( BER_BVISEMPTY( prnc ) ) {
		rargv[ 0 ] = "rewriteRule";
		rargv[ 1 ] = "^$";
		rargv[ 2 ] = pvnc->bv_val;
		rargv[ 3 ] = ":";
		rargv[ 4 ] = NULL;
		rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );
	}

	/* backward compatibility */
	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "searchResult";
	rargv[ 2 ] = "alias";
	rargv[ 3 ] = "searchEntryDN";
	rargv[ 4 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "matchedDN";
	rargv[ 2 ] = "alias";
	rargv[ 3 ] = "searchEntryDN";
	rargv[ 4 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "searchAttrDN";
	rargv[ 2 ] = "alias";
	rargv[ 3 ] = "searchEntryDN";
	rargv[ 4 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 4, rargv );

	/* NOTE: this corresponds to #undef'ining RWM_REFERRAL_REWRITE;
	 * see servers/slapd/overlays/rwm.h for details */
        rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "referralAttrDN";
	rargv[ 2 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 2, rargv );

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "referralDN";
	rargv[ 2 ] = NULL;
	rewrite_parse( info, "<suffix massage>", ++line, 2, rargv );

	return 0;
}
#endif /* ENABLE_REWRITE */

