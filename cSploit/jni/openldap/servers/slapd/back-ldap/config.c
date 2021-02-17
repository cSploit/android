/* config.c - ldap backend configuration file routine */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999-2003 Howard Chu.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
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

#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/socket.h>

#include "slap.h"
#include "config.h"
#include "back-ldap.h"
#include "lutil.h"
#include "ldif.h"

static SLAP_EXTOP_MAIN_FN ldap_back_exop_whoami;

static ConfigDriver ldap_back_cf_gen;
static ConfigDriver ldap_pbind_cf_gen;

enum {
	LDAP_BACK_CFG_URI = 1,
	LDAP_BACK_CFG_TLS,
	LDAP_BACK_CFG_ACL_AUTHCDN,
	LDAP_BACK_CFG_ACL_PASSWD,
	LDAP_BACK_CFG_ACL_METHOD,
	LDAP_BACK_CFG_ACL_BIND,
	LDAP_BACK_CFG_IDASSERT_MODE,
	LDAP_BACK_CFG_IDASSERT_AUTHCDN,
	LDAP_BACK_CFG_IDASSERT_PASSWD,
	LDAP_BACK_CFG_IDASSERT_AUTHZFROM,
	LDAP_BACK_CFG_IDASSERT_PASSTHRU,
	LDAP_BACK_CFG_IDASSERT_METHOD,
	LDAP_BACK_CFG_IDASSERT_BIND,
	LDAP_BACK_CFG_REBIND,
	LDAP_BACK_CFG_CHASE,
	LDAP_BACK_CFG_T_F,
	LDAP_BACK_CFG_WHOAMI,
	LDAP_BACK_CFG_TIMEOUT,
	LDAP_BACK_CFG_IDLE_TIMEOUT,
	LDAP_BACK_CFG_CONN_TTL,
	LDAP_BACK_CFG_NETWORK_TIMEOUT,
	LDAP_BACK_CFG_VERSION,
	LDAP_BACK_CFG_SINGLECONN,
	LDAP_BACK_CFG_USETEMP,
	LDAP_BACK_CFG_CONNPOOLMAX,
	LDAP_BACK_CFG_CANCEL,
	LDAP_BACK_CFG_QUARANTINE,
	LDAP_BACK_CFG_ST_REQUEST,
	LDAP_BACK_CFG_NOREFS,
	LDAP_BACK_CFG_NOUNDEFFILTER,
	LDAP_BACK_CFG_ONERR,

	LDAP_BACK_CFG_REWRITE,
	LDAP_BACK_CFG_KEEPALIVE,

	LDAP_BACK_CFG_LAST
};

static ConfigTable ldapcfg[] = {
	{ "uri", "uri", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_URI,
		ldap_back_cf_gen, "( OLcfgDbAt:0.14 "
			"NAME 'olcDbURI' "
			"DESC 'URI (list) for remote DSA' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "tls", "what", 2, 0, 0,
		ARG_MAGIC|LDAP_BACK_CFG_TLS,
		ldap_back_cf_gen, "( OLcfgDbAt:3.1 "
			"NAME 'olcDbStartTLS' "
			"DESC 'StartTLS' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "acl-authcDN", "DN", 2, 2, 0,
		ARG_DN|ARG_MAGIC|LDAP_BACK_CFG_ACL_AUTHCDN,
		ldap_back_cf_gen, "( OLcfgDbAt:3.2 "
			"NAME 'olcDbACLAuthcDn' "
			"DESC 'Remote ACL administrative identity' "
			"OBSOLETE "
			"SYNTAX OMsDN "
			"SINGLE-VALUE )",
		NULL, NULL },
	/* deprecated, will be removed; aliases "acl-authcDN" */
	{ "binddn", "DN", 2, 2, 0,
		ARG_DN|ARG_MAGIC|LDAP_BACK_CFG_ACL_AUTHCDN,
		ldap_back_cf_gen, NULL, NULL, NULL },
	{ "acl-passwd", "cred", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_ACL_PASSWD,
		ldap_back_cf_gen, "( OLcfgDbAt:3.3 "
			"NAME 'olcDbACLPasswd' "
			"DESC 'Remote ACL administrative identity credentials' "
			"OBSOLETE "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	/* deprecated, will be removed; aliases "acl-passwd" */
	{ "bindpw", "cred", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_ACL_PASSWD,
		ldap_back_cf_gen, NULL, NULL, NULL },
	/* deprecated, will be removed; aliases "acl-bind" */
	{ "acl-method", "args", 2, 0, 0,
		ARG_MAGIC|LDAP_BACK_CFG_ACL_METHOD,
		ldap_back_cf_gen, NULL, NULL, NULL },
	{ "acl-bind", "args", 2, 0, 0,
		ARG_MAGIC|LDAP_BACK_CFG_ACL_BIND,
		ldap_back_cf_gen, "( OLcfgDbAt:3.4 "
			"NAME 'olcDbACLBind' "
			"DESC 'Remote ACL administrative identity auth bind configuration' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "idassert-authcDN", "DN", 2, 2, 0,
		ARG_DN|ARG_MAGIC|LDAP_BACK_CFG_IDASSERT_AUTHCDN,
		ldap_back_cf_gen, "( OLcfgDbAt:3.5 "
			"NAME 'olcDbIDAssertAuthcDn' "
			"DESC 'Remote Identity Assertion administrative identity' "
			"OBSOLETE "
			"SYNTAX OMsDN "
			"SINGLE-VALUE )",
		NULL, NULL },
	/* deprecated, will be removed; partially aliases "idassert-authcDN" */
	{ "proxyauthzdn", "DN", 2, 2, 0,
		ARG_DN|ARG_MAGIC|LDAP_BACK_CFG_IDASSERT_AUTHCDN,
		ldap_back_cf_gen, NULL, NULL, NULL },
	{ "idassert-passwd", "cred", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_IDASSERT_PASSWD,
		ldap_back_cf_gen, "( OLcfgDbAt:3.6 "
			"NAME 'olcDbIDAssertPasswd' "
			"DESC 'Remote Identity Assertion administrative identity credentials' "
			"OBSOLETE "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	/* deprecated, will be removed; partially aliases "idassert-passwd" */
	{ "proxyauthzpw", "cred", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_IDASSERT_PASSWD,
		ldap_back_cf_gen, NULL, NULL, NULL },
	{ "idassert-bind", "args", 2, 0, 0,
		ARG_MAGIC|LDAP_BACK_CFG_IDASSERT_BIND,
		ldap_back_cf_gen, "( OLcfgDbAt:3.7 "
			"NAME 'olcDbIDAssertBind' "
			"DESC 'Remote Identity Assertion administrative identity auth bind configuration' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "idassert-method", "args", 2, 0, 0,
		ARG_MAGIC|LDAP_BACK_CFG_IDASSERT_METHOD,
		ldap_back_cf_gen, NULL, NULL, NULL },
	{ "idassert-mode", "mode>|u:<user>|[dn:]<DN", 2, 0, 0,
		ARG_STRING|ARG_MAGIC|LDAP_BACK_CFG_IDASSERT_MODE,
		ldap_back_cf_gen, "( OLcfgDbAt:3.8 "
			"NAME 'olcDbIDAssertMode' "
			"DESC 'Remote Identity Assertion mode' "
			"OBSOLETE "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE)",
		NULL, NULL },
	{ "idassert-authzFrom", "authzRule", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_IDASSERT_AUTHZFROM,
		ldap_back_cf_gen, "( OLcfgDbAt:3.9 "
			"NAME 'olcDbIDAssertAuthzFrom' "
			"DESC 'Remote Identity Assertion authz rules' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )",
		NULL, NULL },
	{ "rebind-as-user", "true|FALSE", 1, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_REBIND,
		ldap_back_cf_gen, "( OLcfgDbAt:3.10 "
			"NAME 'olcDbRebindAsUser' "
			"DESC 'Rebind as user' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "chase-referrals", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_CHASE,
		ldap_back_cf_gen, "( OLcfgDbAt:3.11 "
			"NAME 'olcDbChaseReferrals' "
			"DESC 'Chase referrals' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "t-f-support", "true|FALSE|discover", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_T_F,
		ldap_back_cf_gen, "( OLcfgDbAt:3.12 "
			"NAME 'olcDbTFSupport' "
			"DESC 'Absolute filters support' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "proxy-whoami", "true|FALSE", 1, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_WHOAMI,
		ldap_back_cf_gen, "( OLcfgDbAt:3.13 "
			"NAME 'olcDbProxyWhoAmI' "
			"DESC 'Proxy whoAmI exop' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "timeout", "timeout(list)", 2, 0, 0,
		ARG_MAGIC|LDAP_BACK_CFG_TIMEOUT,
		ldap_back_cf_gen, "( OLcfgDbAt:3.14 "
			"NAME 'olcDbTimeout' "
			"DESC 'Per-operation timeouts' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "idle-timeout", "timeout", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_IDLE_TIMEOUT,
		ldap_back_cf_gen, "( OLcfgDbAt:3.15 "
			"NAME 'olcDbIdleTimeout' "
			"DESC 'connection idle timeout' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "conn-ttl", "ttl", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_CONN_TTL,
		ldap_back_cf_gen, "( OLcfgDbAt:3.16 "
			"NAME 'olcDbConnTtl' "
			"DESC 'connection ttl' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "network-timeout", "timeout", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_NETWORK_TIMEOUT,
		ldap_back_cf_gen, "( OLcfgDbAt:3.17 "
			"NAME 'olcDbNetworkTimeout' "
			"DESC 'connection network timeout' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "protocol-version", "version", 2, 2, 0,
		ARG_MAGIC|ARG_INT|LDAP_BACK_CFG_VERSION,
		ldap_back_cf_gen, "( OLcfgDbAt:3.18 "
			"NAME 'olcDbProtocolVersion' "
			"DESC 'protocol version' "
			"SYNTAX OMsInteger "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "single-conn", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_SINGLECONN,
		ldap_back_cf_gen, "( OLcfgDbAt:3.19 "
			"NAME 'olcDbSingleConn' "
			"DESC 'cache a single connection per identity' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "cancel", "ABANDON|ignore|exop", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_CANCEL,
		ldap_back_cf_gen, "( OLcfgDbAt:3.20 "
			"NAME 'olcDbCancel' "
			"DESC 'abandon/ignore/exop operations when appropriate' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "quarantine", "retrylist", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_QUARANTINE,
		ldap_back_cf_gen, "( OLcfgDbAt:3.21 "
			"NAME 'olcDbQuarantine' "
			"DESC 'Quarantine database if connection fails and retry according to rule' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "use-temporary-conn", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_USETEMP,
		ldap_back_cf_gen, "( OLcfgDbAt:3.22 "
			"NAME 'olcDbUseTemporaryConn' "
			"DESC 'Use temporary connections if the cached one is busy' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "conn-pool-max", "<n>", 2, 2, 0,
		ARG_MAGIC|ARG_INT|LDAP_BACK_CFG_CONNPOOLMAX,
		ldap_back_cf_gen, "( OLcfgDbAt:3.23 "
			"NAME 'olcDbConnectionPoolMax' "
			"DESC 'Max size of privileged connections pool' "
			"SYNTAX OMsInteger "
			"SINGLE-VALUE )",
		NULL, NULL },
#ifdef SLAP_CONTROL_X_SESSION_TRACKING
	{ "session-tracking-request", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_ST_REQUEST,
		ldap_back_cf_gen, "( OLcfgDbAt:3.24 "
			"NAME 'olcDbSessionTrackingRequest' "
			"DESC 'Add session tracking control to proxied requests' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */
	{ "norefs", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_NOREFS,
		ldap_back_cf_gen, "( OLcfgDbAt:3.25 "
			"NAME 'olcDbNoRefs' "
			"DESC 'Do not return search reference responses' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "noundeffilter", "true|FALSE", 2, 2, 0,
		ARG_MAGIC|ARG_ON_OFF|LDAP_BACK_CFG_NOUNDEFFILTER,
		ldap_back_cf_gen, "( OLcfgDbAt:3.26 "
			"NAME 'olcDbNoUndefFilter' "
			"DESC 'Do not propagate undefined search filters' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "onerr", "CONTINUE|report|stop", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_ONERR,
		ldap_back_cf_gen, "( OLcfgDbAt:3.108 "
			"NAME 'olcDbOnErr' "
			"DESC 'error handling' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "idassert-passThru", "authzRule", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_IDASSERT_PASSTHRU,
		ldap_back_cf_gen, "( OLcfgDbAt:3.27 "
			"NAME 'olcDbIDAssertPassThru' "
			"DESC 'Remote Identity Assertion passthru rules' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )",
		NULL, NULL },

	{ "suffixmassage", "[virtual]> <real", 2, 3, 0,
		ARG_STRING|ARG_MAGIC|LDAP_BACK_CFG_REWRITE,
		ldap_back_cf_gen, NULL, NULL, NULL },
	{ "map", "attribute|objectClass> [*|<local>] *|<remote", 3, 4, 0,
		ARG_STRING|ARG_MAGIC|LDAP_BACK_CFG_REWRITE,
		ldap_back_cf_gen, NULL, NULL, NULL },
	{ "rewrite", "<arglist>", 2, 4, STRLENOF( "rewrite" ),
		ARG_STRING|ARG_MAGIC|LDAP_BACK_CFG_REWRITE,
		ldap_back_cf_gen, NULL, NULL, NULL },
	{ "keepalive", "keepalive", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_KEEPALIVE,
		ldap_back_cf_gen, "( OLcfgDbAt:3.29 "
			"NAME 'olcDbKeepalive' "
			"DESC 'TCP keepalive' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

static ConfigOCs ldapocs[] = {
	{ "( OLcfgDbOc:3.1 "
		"NAME 'olcLDAPConfig' "
		"DESC 'LDAP backend configuration' "
		"SUP olcDatabaseConfig "
		"MAY ( olcDbURI "
			"$ olcDbStartTLS "
			"$ olcDbACLAuthcDn "
			"$ olcDbACLPasswd "
			"$ olcDbACLBind "
			"$ olcDbIDAssertAuthcDn "
			"$ olcDbIDAssertPasswd "
			"$ olcDbIDAssertBind "
			"$ olcDbIDAssertMode "
			"$ olcDbIDAssertAuthzFrom "
			"$ olcDbIDAssertPassThru "
			"$ olcDbRebindAsUser "
			"$ olcDbChaseReferrals "
			"$ olcDbTFSupport "
			"$ olcDbProxyWhoAmI "
			"$ olcDbTimeout "
			"$ olcDbIdleTimeout "
			"$ olcDbConnTtl "
			"$ olcDbNetworkTimeout "
			"$ olcDbProtocolVersion "
			"$ olcDbSingleConn "
			"$ olcDbCancel "
			"$ olcDbQuarantine "
			"$ olcDbUseTemporaryConn "
			"$ olcDbConnectionPoolMax "
#ifdef SLAP_CONTROL_X_SESSION_TRACKING
			"$ olcDbSessionTrackingRequest "
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */
			"$ olcDbNoRefs "
			"$ olcDbNoUndefFilter "
			"$ olcDbOnErr "
			"$ olcDbKeepalive "
		") )",
		 	Cft_Database, ldapcfg},
	{ NULL, 0, NULL }
};

static ConfigTable pbindcfg[] = {
	{ "uri", "uri", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_URI,
		ldap_pbind_cf_gen, "( OLcfgDbAt:0.14 "
			"NAME 'olcDbURI' "
			"DESC 'URI (list) for remote DSA' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "tls", "what", 2, 0, 0,
		ARG_MAGIC|LDAP_BACK_CFG_TLS,
		ldap_pbind_cf_gen, "( OLcfgDbAt:3.1 "
			"NAME 'olcDbStartTLS' "
			"DESC 'StartTLS' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "network-timeout", "timeout", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_NETWORK_TIMEOUT,
		ldap_pbind_cf_gen, "( OLcfgDbAt:3.17 "
			"NAME 'olcDbNetworkTimeout' "
			"DESC 'connection network timeout' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ "quarantine", "retrylist", 2, 2, 0,
		ARG_MAGIC|LDAP_BACK_CFG_QUARANTINE,
		ldap_pbind_cf_gen, "( OLcfgDbAt:3.21 "
			"NAME 'olcDbQuarantine' "
			"DESC 'Quarantine database if connection fails and retry according to rule' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

static ConfigOCs pbindocs[] = {
	{ "( OLcfgOvOc:3.3 "
		"NAME 'olcPBindConfig' "
		"DESC 'Proxy Bind configuration' "
		"SUP olcOverlayConfig "
		"MUST olcDbURI "
		"MAY ( olcDbStartTLS "
			"$ olcDbNetworkTimeout "
			"$ olcDbQuarantine "
		") )",
		 	Cft_Overlay, pbindcfg},
	{ NULL, 0, NULL }
};

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
	{ BER_BVC( "stop" ),		LDAP_BACK_F_ONERR_STOP },
	{ BER_BVC( "report" ),		LDAP_BACK_F_ONERR_STOP }, /* same behavior */
	{ BER_BVC( "continue" ),	LDAP_BACK_F_NONE },
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

int
slap_retry_info_parse(
	char			*in,
	slap_retry_info_t 	*ri,
	char			*buf,
	ber_len_t		buflen )
{
	char			**retrylist = NULL;
	int			rc = 0;
	int			i;

	slap_str2clist( &retrylist, in, " ;" );
	if ( retrylist == NULL ) {
		return 1;
	}

	for ( i = 0; retrylist[ i ] != NULL; i++ )
		/* count */ ;

	ri->ri_interval = ch_calloc( sizeof( time_t ), i + 1 );
	ri->ri_num = ch_calloc( sizeof( int ), i + 1 );

	for ( i = 0; retrylist[ i ] != NULL; i++ ) {
		unsigned long	t;
		char		*sep = strchr( retrylist[ i ], ',' );

		if ( sep == NULL ) {
			snprintf( buf, buflen,
				"missing comma in retry pattern #%d \"%s\"",
				i, retrylist[ i ] );
			rc = 1;
			goto done;
		}

		*sep++ = '\0';

		if ( lutil_parse_time( retrylist[ i ], &t ) ) {
			snprintf( buf, buflen,
				"unable to parse interval #%d \"%s\"",
				i, retrylist[ i ] );
			rc = 1;
			goto done;
		}
		ri->ri_interval[ i ] = (time_t)t;

		if ( strcmp( sep, "+" ) == 0 ) {
			if ( retrylist[ i + 1 ] != NULL ) {
				snprintf( buf, buflen,
					"extra cruft after retry pattern "
					"#%d \"%s,+\" with \"forever\" mark",
					i, retrylist[ i ] );
				rc = 1;
				goto done;
			}
			ri->ri_num[ i ] = SLAP_RETRYNUM_FOREVER;
			
		} else if ( lutil_atoi( &ri->ri_num[ i ], sep ) ) {
			snprintf( buf, buflen,
				"unable to parse retry num #%d \"%s\"",
				i, sep );
			rc = 1;
			goto done;
		}
	}

	ri->ri_num[ i ] = SLAP_RETRYNUM_TAIL;

	ri->ri_idx = 0;
	ri->ri_count = 0;
	ri->ri_last = (time_t)(-1);

done:;
	ldap_charray_free( retrylist );

	if ( rc ) {
		slap_retry_info_destroy( ri );
	}

	return rc;
}

int
slap_retry_info_unparse(
	slap_retry_info_t	*ri,
	struct berval		*bvout )
{
	char		buf[ BUFSIZ * 2 ],
			*ptr = buf;
	int		i, len, restlen = (int) sizeof( buf );
	struct berval	bv;

	assert( ri != NULL );
	assert( bvout != NULL );

	BER_BVZERO( bvout );

	for ( i = 0; ri->ri_num[ i ] != SLAP_RETRYNUM_TAIL; i++ ) {
		if ( i > 0 ) {
			if ( --restlen <= 0 ) {
				return 1;
			}
			*ptr++ = ';';
		}

		if ( lutil_unparse_time( ptr, restlen, ri->ri_interval[i] ) < 0 ) {
			return 1;
		}
		len = (int) strlen( ptr );
		if ( (restlen -= len + 1) <= 0 ) {
			return 1;
		}
		ptr += len;
		*ptr++ = ',';

		if ( ri->ri_num[i] == SLAP_RETRYNUM_FOREVER ) {
			if ( --restlen <= 0 ) {
				return 1;
			}
			*ptr++ = '+';

		} else {
			len = snprintf( ptr, restlen, "%d", ri->ri_num[i] );
			if ( (restlen -= len) <= 0 || len < 0 ) {
				return 1;
			}
			ptr += len;
		}
	}

	bv.bv_val = buf;
	bv.bv_len = ptr - buf;
	ber_dupbv( bvout, &bv );

	return 0;
}

void
slap_retry_info_destroy(
	slap_retry_info_t	*ri )
{
	assert( ri != NULL );

	assert( ri->ri_interval != NULL );
	ch_free( ri->ri_interval );
	ri->ri_interval = NULL;

	assert( ri->ri_num != NULL );
	ch_free( ri->ri_num );
	ri->ri_num = NULL;
}

int
slap_idassert_authzfrom_parse( ConfigArgs *c, slap_idassert_t *si )
{
	struct berval	bv;
	struct berval	in;
	int		rc;

 	if ( strcmp( c->argv[ 1 ], "*" ) == 0
 		|| strcmp( c->argv[ 1 ], "dn:*" ) == 0
 		|| strcasecmp( c->argv[ 1 ], "dn.regex:.*" ) == 0 )
 	{
 		if ( si->si_authz != NULL ) {
 			snprintf( c->cr_msg, sizeof( c->cr_msg ),
 				"\"%s <authz>\": "
 				"\"%s\" conflicts with existing authz rules",
 				c->argv[ 0 ], c->argv[ 1 ] );
 			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
 			return 1;
 		}
 
 		si->si_flags |= LDAP_BACK_AUTH_AUTHZ_ALL;
 
 		return 0;
 
 	} else if ( ( si->si_flags & LDAP_BACK_AUTH_AUTHZ_ALL ) ) {
  		snprintf( c->cr_msg, sizeof( c->cr_msg ),
  			"\"%s <authz>\": "
 			"\"<authz>\" conflicts with \"*\"", c->argv[0] );
  		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
  		return 1;
  	}
 	
 	ber_str2bv( c->argv[ 1 ], 0, 0, &in );
 	rc = authzNormalize( 0, NULL, NULL, &in, &bv, NULL );
 	if ( rc != LDAP_SUCCESS ) {
 		snprintf( c->cr_msg, sizeof( c->cr_msg ),
 			"\"%s <authz>\": "
 			"invalid syntax", c->argv[0] );
 		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
 		return 1;
 	}

	if ( c->valx == -1 ) {
		ber_bvarray_add( &si->si_authz, &bv );

	} else {
		int i = 0;
		if ( si->si_authz != NULL ) {
			for ( ; !BER_BVISNULL( &si->si_authz[ i ] ); i++ )
				;
		}

		if ( i <= c->valx ) {
			ber_bvarray_add( &si->si_authz, &bv );

		} else {
			BerVarray tmp = ber_memrealloc( si->si_authz,
				sizeof( struct berval )*( i + 2 ) );
			if ( tmp == NULL ) {
				return -1;
			}
			si->si_authz = tmp;
			for ( ; i > c->valx; i-- ) {
				si->si_authz[ i ] = si->si_authz[ i - 1 ];
			}
			si->si_authz[ c->valx ] = bv;
		}
	}

	return 0;
}

static int
slap_idassert_passthru_parse( ConfigArgs *c, slap_idassert_t *si )
{
	struct berval	bv;
	struct berval	in;
	int		rc;

 	ber_str2bv( c->argv[ 1 ], 0, 0, &in );
 	rc = authzNormalize( 0, NULL, NULL, &in, &bv, NULL );
 	if ( rc != LDAP_SUCCESS ) {
 		snprintf( c->cr_msg, sizeof( c->cr_msg ),
 			"\"%s <authz>\": "
 			"invalid syntax", c->argv[0] );
 		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
 		return 1;
 	}
  
	if ( c->valx == -1 ) {
		ber_bvarray_add( &si->si_passthru, &bv );

	} else {
		int i = 0;
		if ( si->si_passthru != NULL ) {
			for ( ; !BER_BVISNULL( &si->si_passthru[ i ] ); i++ )
				;
		}

		if ( i <= c->valx ) {
			ber_bvarray_add( &si->si_passthru, &bv );

		} else {
			BerVarray tmp = ber_memrealloc( si->si_passthru,
				sizeof( struct berval )*( i + 2 ) );
			if ( tmp == NULL ) {
				return -1;
			}
			si->si_passthru = tmp;
			for ( ; i > c->valx; i-- ) {
				si->si_passthru[ i ] = si->si_passthru[ i - 1 ];
			}
			si->si_passthru[ c->valx ] = bv;
		}
	}

	return 0;
}

int
slap_idassert_parse( ConfigArgs *c, slap_idassert_t *si )
{
	int		i;

	for ( i = 1; i < c->argc; i++ ) {
		if ( strncasecmp( c->argv[ i ], "mode=", STRLENOF( "mode=" ) ) == 0 ) {
			char	*argvi = c->argv[ i ] + STRLENOF( "mode=" );
			int	j;

			j = verb_to_mask( argvi, idassert_mode );
			if ( BER_BVISNULL( &idassert_mode[ j ].word ) ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"\"%s <args>\": "
					"unknown mode \"%s\"",
					c->argv[0], argvi );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}

			si->si_mode = idassert_mode[ j ].mask;

		} else if ( strncasecmp( c->argv[ i ], "authz=", STRLENOF( "authz=" ) ) == 0 ) {
			char	*argvi = c->argv[ i ] + STRLENOF( "authz=" );

			if ( strcasecmp( argvi, "native" ) == 0 ) {
				if ( si->si_bc.sb_method != LDAP_AUTH_SASL ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"\"%s <args>\": "
						"authz=\"native\" incompatible "
						"with auth method", c->argv[0] );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					return 1;
				}
				si->si_flags |= LDAP_BACK_AUTH_NATIVE_AUTHZ;

			} else if ( strcasecmp( argvi, "proxyAuthz" ) == 0 ) {
				si->si_flags &= ~LDAP_BACK_AUTH_NATIVE_AUTHZ;

			} else {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"\"%s <args>\": "
					"unknown authz \"%s\"",
					c->argv[0], argvi );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}

		} else if ( strncasecmp( c->argv[ i ], "flags=", STRLENOF( "flags=" ) ) == 0 ) {
			char	*argvi = c->argv[ i ] + STRLENOF( "flags=" );
			char	**flags = ldap_str2charray( argvi, "," );
			int	j, err = 0;

			if ( flags == NULL ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"\"%s <args>\": "
					"unable to parse flags \"%s\"",
					c->argv[0], argvi );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}

			for ( j = 0; flags[ j ] != NULL; j++ ) {

				if ( strcasecmp( flags[ j ], "override" ) == 0 ) {
					si->si_flags |= LDAP_BACK_AUTH_OVERRIDE;

				} else if ( strcasecmp( flags[ j ], "prescriptive" ) == 0 ) {
					si->si_flags |= LDAP_BACK_AUTH_PRESCRIPTIVE;

				} else if ( strcasecmp( flags[ j ], "non-prescriptive" ) == 0 ) {
					si->si_flags &= ( ~LDAP_BACK_AUTH_PRESCRIPTIVE );

				} else if ( strcasecmp( flags[ j ], "obsolete-proxy-authz" ) == 0 ) {
					if ( si->si_flags & LDAP_BACK_AUTH_OBSOLETE_ENCODING_WORKAROUND ) {
						snprintf( c->cr_msg, sizeof( c->cr_msg ),
							"\"%s <args>\": "
							"\"obsolete-proxy-authz\" flag "
							"incompatible with previously issued \"obsolete-encoding-workaround\" flag.",
							c->argv[0] );
						Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
						err = 1;
						break;

					} else {
						si->si_flags |= LDAP_BACK_AUTH_OBSOLETE_PROXY_AUTHZ;
					}

				} else if ( strcasecmp( flags[ j ], "obsolete-encoding-workaround" ) == 0 ) {
					if ( si->si_flags & LDAP_BACK_AUTH_OBSOLETE_PROXY_AUTHZ ) {
						snprintf( c->cr_msg, sizeof( c->cr_msg ),
							"\"%s <args>\": "
							"\"obsolete-encoding-workaround\" flag "
							"incompatible with previously issued \"obsolete-proxy-authz\" flag.",
							c->argv[0] );
						Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
						err = 1;
						break;

					} else {
						si->si_flags |= LDAP_BACK_AUTH_OBSOLETE_ENCODING_WORKAROUND;
					}

				} else if ( strcasecmp( flags[ j ], "proxy-authz-critical" ) == 0 ) {
					si->si_flags |= LDAP_BACK_AUTH_PROXYAUTHZ_CRITICAL;

				} else if ( strcasecmp( flags[ j ], "proxy-authz-non-critical" ) == 0 ) {
					si->si_flags &= ~LDAP_BACK_AUTH_PROXYAUTHZ_CRITICAL;

				} else if ( strcasecmp( flags[ j ], "dn-none" ) == 0 ) {
					si->si_flags &= ~LDAP_BACK_AUTH_DN_MASK;

				} else if ( strcasecmp( flags[ j ], "dn-authzid" ) == 0 ) {
					si->si_flags &= ~LDAP_BACK_AUTH_DN_MASK;
					si->si_flags |= LDAP_BACK_AUTH_DN_AUTHZID;

				} else if ( strcasecmp( flags[ j ], "dn-whoami" ) == 0 ) {
					si->si_flags &= ~LDAP_BACK_AUTH_DN_MASK;
					si->si_flags |= LDAP_BACK_AUTH_DN_WHOAMI;

				} else {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"\"%s <args>\": "
						"unknown flag \"%s\"",
						c->argv[0], flags[ j ] );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
					err = 1;
					break;
				}
			}

			ldap_charray_free( flags );
			if ( err ) {
				return 1;
			}

		} else if ( bindconf_parse( c->argv[ i ], &si->si_bc ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"%s <args>\": "
				"unable to parse field \"%s\"",
				c->argv[0], c->argv[ i ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
	}

	if ( si->si_bc.sb_method == LDAP_AUTH_SIMPLE ) {
		if ( BER_BVISNULL( &si->si_bc.sb_binddn )
			|| BER_BVISNULL( &si->si_bc.sb_cred ) )
		{
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"%s <args>\": "
				"SIMPLE needs \"binddn\" and \"credentials\"", c->argv[0] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}

	} else if ( si->si_bc.sb_method == LDAP_AUTH_SASL ) {
		if ( BER_BVISNULL( &si->si_bc.sb_binddn ) &&
			!(si->si_flags & LDAP_BACK_AUTH_DN_MASK) )
		{
			static struct berval authid = BER_BVC("cn=auth");
			ber_dupbv( &si->si_bc.sb_binddn, &authid );
		}
	}

	bindconf_tls_defaults( &si->si_bc );

	return 0;
}

/* NOTE: temporary, until back-meta is ported to back-config */
int
slap_idassert_passthru_parse_cf( const char *fname, int lineno, const char *arg, slap_idassert_t *si )
{
	ConfigArgs	c = { 0 };
	char		*argv[ 3 ];

	snprintf( c.log, sizeof( c.log ), "%s: line %d", fname, lineno );
	c.argc = 2;
	c.argv = argv;
	argv[ 0 ] = "idassert-passThru";
	argv[ 1 ] = (char *)arg;
	argv[ 2 ] = NULL;

	return slap_idassert_passthru_parse( &c, si );
}

static int
ldap_back_cf_gen( ConfigArgs *c )
{
	ldapinfo_t	*li = ( ldapinfo_t * )c->be->be_private;
	int		rc = 0;
	int		i;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		struct berval	bv = BER_BVNULL;

		if ( li == NULL ) {
			return 1;
		}

		switch( c->type ) {
		case LDAP_BACK_CFG_URI:
			if ( li->li_uri != NULL ) {
				struct berval	bv, bv2;

				ber_str2bv( li->li_uri, 0, 0, &bv );
				bv2.bv_len = bv.bv_len + STRLENOF( "\"\"" );
				bv2.bv_val = ch_malloc( bv2.bv_len + 1 );
				snprintf( bv2.bv_val, bv2.bv_len + 1,
					"\"%s\"", bv.bv_val );
				ber_bvarray_add( &c->rvalue_vals, &bv2 );

			} else {
				rc = 1;
			}
			break;

		case LDAP_BACK_CFG_TLS: {
			struct berval bc = BER_BVNULL, bv2;
			enum_to_verb( tls_mode, ( li->li_flags & LDAP_BACK_F_TLS_MASK ), &bv );
			assert( !BER_BVISNULL( &bv ) );
			bindconf_tls_unparse( &li->li_tls, &bc );

			if ( !BER_BVISEMPTY( &bc )) {
				bv2.bv_len = bv.bv_len + bc.bv_len + 1;
				bv2.bv_val = ch_malloc( bv2.bv_len + 1 );
				strcpy( bv2.bv_val, bv.bv_val );
				bv2.bv_val[bv.bv_len] = ' ';
				strcpy( &bv2.bv_val[bv.bv_len + 1], bc.bv_val );
				ber_bvarray_add( &c->rvalue_vals, &bv2 );

			} else {
				value_add_one( &c->rvalue_vals, &bv );
			}
			ber_memfree( bc.bv_val );
			}
			break;

		case LDAP_BACK_CFG_ACL_AUTHCDN:
		case LDAP_BACK_CFG_ACL_PASSWD:
		case LDAP_BACK_CFG_ACL_METHOD:
			/* handled by LDAP_BACK_CFG_ACL_BIND */
			rc = 1;
			break;

		case LDAP_BACK_CFG_ACL_BIND: {
			int	i;

			if ( li->li_acl_authmethod == LDAP_AUTH_NONE ) {
				return 1;
			}

			bindconf_unparse( &li->li_acl, &bv );

			for ( i = 0; isspace( (unsigned char) bv.bv_val[ i ] ); i++ )
				/* count spaces */ ;

			if ( i ) {
				bv.bv_len -= i;
				AC_MEMCPY( bv.bv_val, &bv.bv_val[ i ],
					bv.bv_len + 1 );
			}

			ber_bvarray_add( &c->rvalue_vals, &bv );
			break;
		}

		case LDAP_BACK_CFG_IDASSERT_MODE:
		case LDAP_BACK_CFG_IDASSERT_AUTHCDN:
		case LDAP_BACK_CFG_IDASSERT_PASSWD:
		case LDAP_BACK_CFG_IDASSERT_METHOD:
			/* handled by LDAP_BACK_CFG_IDASSERT_BIND */
			rc = 1;
			break;

		case LDAP_BACK_CFG_IDASSERT_AUTHZFROM:
		case LDAP_BACK_CFG_IDASSERT_PASSTHRU: {
			BerVarray	*bvp;
			int		i;
			struct berval	bv = BER_BVNULL;
			char		buf[SLAP_TEXT_BUFLEN];

			switch ( c->type ) {
			case LDAP_BACK_CFG_IDASSERT_AUTHZFROM: bvp = &li->li_idassert_authz; break;
			case LDAP_BACK_CFG_IDASSERT_PASSTHRU: bvp = &li->li_idassert_passthru; break;
			default: assert( 0 ); break;
			}

			if ( *bvp == NULL ) {
				if ( bvp == &li->li_idassert_authz
					&& ( li->li_idassert_flags & LDAP_BACK_AUTH_AUTHZ_ALL ) )
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

			if ( li->li_idassert_authmethod == LDAP_AUTH_NONE ) {
				return 1;
			}

			if ( li->li_idassert_authmethod != LDAP_AUTH_NONE ) {
				ber_len_t	len;

				switch ( li->li_idassert_mode ) {
				case LDAP_BACK_IDASSERT_OTHERID:
				case LDAP_BACK_IDASSERT_OTHERDN:
					break;

				default: {
					struct berval	mode = BER_BVNULL;

					enum_to_verb( idassert_mode, li->li_idassert_mode, &mode );
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

				if ( li->li_idassert_flags & LDAP_BACK_AUTH_NATIVE_AUTHZ ) {
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

				if ( li->li_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) {
					ptr = lutil_strcopy( ptr, "prescriptive" );
				} else {
					ptr = lutil_strcopy( ptr, "non-prescriptive" );
				}

				if ( li->li_idassert_flags & LDAP_BACK_AUTH_OVERRIDE ) {
					ptr = lutil_strcopy( ptr, ",override" );
				}

				if ( li->li_idassert_flags & LDAP_BACK_AUTH_OBSOLETE_PROXY_AUTHZ ) {
					ptr = lutil_strcopy( ptr, ",obsolete-proxy-authz" );

				} else if ( li->li_idassert_flags & LDAP_BACK_AUTH_OBSOLETE_ENCODING_WORKAROUND ) {
					ptr = lutil_strcopy( ptr, ",obsolete-encoding-workaround" );
				}

				if ( li->li_idassert_flags & LDAP_BACK_AUTH_PROXYAUTHZ_CRITICAL ) {
					ptr = lutil_strcopy( ptr, ",proxy-authz-critical" );

				} else {
					ptr = lutil_strcopy( ptr, ",proxy-authz-non-critical" );
				}

				switch ( li->li_idassert_flags & LDAP_BACK_AUTH_DN_MASK ) {
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

				bv.bv_len = ( ptr - bv.bv_val );
				/* end-of-flags */
			}

			bindconf_unparse( &li->li_idassert.si_bc, &bc );

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

		case LDAP_BACK_CFG_REBIND:
			c->value_int = LDAP_BACK_SAVECRED( li );
			break;

		case LDAP_BACK_CFG_CHASE:
			c->value_int = LDAP_BACK_CHASE_REFERRALS( li );
			break;

		case LDAP_BACK_CFG_T_F:
			enum_to_verb( t_f_mode, (li->li_flags & LDAP_BACK_F_T_F_MASK2), &bv );
			if ( BER_BVISNULL( &bv ) ) {
				/* there's something wrong... */
				assert( 0 );
				rc = 1;

			} else {
				value_add_one( &c->rvalue_vals, &bv );
			}
			break;

		case LDAP_BACK_CFG_WHOAMI:
			c->value_int = LDAP_BACK_PROXY_WHOAMI( li );
			break;

		case LDAP_BACK_CFG_TIMEOUT:
			BER_BVZERO( &bv );

			for ( i = 0; i < SLAP_OP_LAST; i++ ) {
				if ( li->li_timeout[ i ] != 0 ) {
					break;
				}
			}

			if ( i == SLAP_OP_LAST ) {
				return 1;
			}

			slap_cf_aux_table_unparse( li->li_timeout, &bv, timeout_table );

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

		case LDAP_BACK_CFG_IDLE_TIMEOUT: {
			char	buf[ SLAP_TEXT_BUFLEN ];

			if ( li->li_idle_timeout == 0 ) {
				return 1;
			}

			lutil_unparse_time( buf, sizeof( buf ), li->li_idle_timeout );
			ber_str2bv( buf, 0, 0, &bv );
			value_add_one( &c->rvalue_vals, &bv );
			} break;

		case LDAP_BACK_CFG_CONN_TTL: {
			char	buf[ SLAP_TEXT_BUFLEN ];

			if ( li->li_conn_ttl == 0 ) {
				return 1;
			}

			lutil_unparse_time( buf, sizeof( buf ), li->li_conn_ttl );
			ber_str2bv( buf, 0, 0, &bv );
			value_add_one( &c->rvalue_vals, &bv );
			} break;

		case LDAP_BACK_CFG_NETWORK_TIMEOUT: {
			char	buf[ SLAP_TEXT_BUFLEN ];

			if ( li->li_network_timeout == 0 ) {
				return 1;
			}

			snprintf( buf, sizeof( buf ), "%ld",
				(long)li->li_network_timeout );
			ber_str2bv( buf, 0, 0, &bv );
			value_add_one( &c->rvalue_vals, &bv );
			} break;

		case LDAP_BACK_CFG_VERSION:
			if ( li->li_version == 0 ) {
				return 1;
			}

			c->value_int = li->li_version;
			break;

		case LDAP_BACK_CFG_SINGLECONN:
			c->value_int = LDAP_BACK_SINGLECONN( li );
			break;

		case LDAP_BACK_CFG_USETEMP:
			c->value_int = LDAP_BACK_USE_TEMPORARIES( li );
			break;

		case LDAP_BACK_CFG_CONNPOOLMAX:
			c->value_int = li->li_conn_priv_max;
			break;

		case LDAP_BACK_CFG_CANCEL: {
			slap_mask_t	mask = LDAP_BACK_F_CANCEL_MASK2;

			if ( LDAP_BACK_CANCEL_DISCOVER( li ) ) {
				mask &= ~LDAP_BACK_F_CANCEL_EXOP;
			}
			enum_to_verb( cancel_mode, (li->li_flags & mask), &bv );
			if ( BER_BVISNULL( &bv ) ) {
				/* there's something wrong... */
				assert( 0 );
				rc = 1;

			} else {
				value_add_one( &c->rvalue_vals, &bv );
			}
			} break;

		case LDAP_BACK_CFG_QUARANTINE:
			if ( !LDAP_BACK_QUARANTINE( li ) ) {
				rc = 1;
				break;
			}

			rc = slap_retry_info_unparse( &li->li_quarantine, &bv );
			if ( rc == 0 ) {
				ber_bvarray_add( &c->rvalue_vals, &bv );
			}
			break;

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
		case LDAP_BACK_CFG_ST_REQUEST:
			c->value_int = LDAP_BACK_ST_REQUEST( li );
			break;
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

		case LDAP_BACK_CFG_NOREFS:
			c->value_int = LDAP_BACK_NOREFS( li );
			break;

		case LDAP_BACK_CFG_NOUNDEFFILTER:
			c->value_int = LDAP_BACK_NOUNDEFFILTER( li );
			break;

		case LDAP_BACK_CFG_ONERR:
			enum_to_verb( onerr_mode, li->li_flags & LDAP_BACK_F_ONERR_STOP, &bv );
			if ( BER_BVISNULL( &bv )) {
				rc = 1;
			} else {
				value_add_one( &c->rvalue_vals, &bv );
			}
			break;

		case LDAP_BACK_CFG_KEEPALIVE: {
			struct berval bv;
			char buf[AC_LINE_MAX];
			bv.bv_len = AC_LINE_MAX;
			bv.bv_val = &buf[0];
			slap_keepalive_parse(&bv, &li->li_tls.sb_keepalive, 0, 0, 1);
			value_add_one( &c->rvalue_vals, &bv );
			break;
			}

		default:
			/* FIXME: we need to handle all... */
			assert( 0 );
			break;
		}
		return rc;

	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch( c->type ) {
		case LDAP_BACK_CFG_URI:
			if ( li->li_uri != NULL ) {
				ch_free( li->li_uri );
				li->li_uri = NULL;

				assert( li->li_bvuri != NULL );
				ber_bvarray_free( li->li_bvuri );
				li->li_bvuri = NULL;
			}

			/* better cleanup the cached connections... */
			/* NOTE: don't worry about locking: if we got here,
			 * other threads are suspended. */
			if ( li->li_conninfo.lai_tree != NULL ) {
				avl_free( li->li_conninfo.lai_tree, ldap_back_conn_free );
				li->li_conninfo.lai_tree = NULL;
			}
			
			break;

		case LDAP_BACK_CFG_TLS:
			rc = 1;
			break;

		case LDAP_BACK_CFG_ACL_AUTHCDN:
		case LDAP_BACK_CFG_ACL_PASSWD:
		case LDAP_BACK_CFG_ACL_METHOD:
			/* handled by LDAP_BACK_CFG_ACL_BIND */
			rc = 1;
			break;

		case LDAP_BACK_CFG_ACL_BIND:
			bindconf_free( &li->li_acl );
			break;

		case LDAP_BACK_CFG_IDASSERT_MODE:
		case LDAP_BACK_CFG_IDASSERT_AUTHCDN:
		case LDAP_BACK_CFG_IDASSERT_PASSWD:
		case LDAP_BACK_CFG_IDASSERT_METHOD:
			/* handled by LDAP_BACK_CFG_IDASSERT_BIND */
			rc = 1;
			break;

		case LDAP_BACK_CFG_IDASSERT_AUTHZFROM:
		case LDAP_BACK_CFG_IDASSERT_PASSTHRU: {
			BerVarray *bvp;

			switch ( c->type ) {
			case LDAP_BACK_CFG_IDASSERT_AUTHZFROM: bvp = &li->li_idassert_authz; break;
			case LDAP_BACK_CFG_IDASSERT_PASSTHRU: bvp = &li->li_idassert_passthru; break;
			default: assert( 0 ); break;
			}

			if ( c->valx < 0 ) {
				if ( *bvp != NULL ) {
					ber_bvarray_free( *bvp );
					*bvp = NULL;
				}

			} else {
				int i;

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
			bindconf_free( &li->li_idassert.si_bc );
			memset( &li->li_idassert, 0, sizeof( slap_idassert_t ) );
			break;

		case LDAP_BACK_CFG_REBIND:
		case LDAP_BACK_CFG_CHASE:
		case LDAP_BACK_CFG_T_F:
		case LDAP_BACK_CFG_WHOAMI:
		case LDAP_BACK_CFG_CANCEL:
			rc = 1;
			break;

		case LDAP_BACK_CFG_TIMEOUT:
			for ( i = 0; i < SLAP_OP_LAST; i++ ) {
				li->li_timeout[ i ] = 0;
			}
			break;

		case LDAP_BACK_CFG_IDLE_TIMEOUT:
			li->li_idle_timeout = 0;
			break;

		case LDAP_BACK_CFG_CONN_TTL:
			li->li_conn_ttl = 0;
			break;

		case LDAP_BACK_CFG_NETWORK_TIMEOUT:
			li->li_network_timeout = 0;
			break;

		case LDAP_BACK_CFG_VERSION:
			li->li_version = 0;
			break;

		case LDAP_BACK_CFG_SINGLECONN:
			li->li_flags &= ~LDAP_BACK_F_SINGLECONN;
			break;

		case LDAP_BACK_CFG_USETEMP:
			li->li_flags &= ~LDAP_BACK_F_USE_TEMPORARIES;
			break;

		case LDAP_BACK_CFG_CONNPOOLMAX:
			li->li_conn_priv_max = LDAP_BACK_CONN_PRIV_MIN;
			break;

		case LDAP_BACK_CFG_QUARANTINE:
			if ( !LDAP_BACK_QUARANTINE( li ) ) {
				break;
			}

			slap_retry_info_destroy( &li->li_quarantine );
			ldap_pvt_thread_mutex_destroy( &li->li_quarantine_mutex );
			li->li_isquarantined = 0;
			li->li_flags &= ~LDAP_BACK_F_QUARANTINE;
			break;

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
		case LDAP_BACK_CFG_ST_REQUEST:
			li->li_flags &= ~LDAP_BACK_F_ST_REQUEST;
			break;
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

		case LDAP_BACK_CFG_NOREFS:
			li->li_flags &= ~LDAP_BACK_F_NOREFS;
			break;

		case LDAP_BACK_CFG_NOUNDEFFILTER:
			li->li_flags &= ~LDAP_BACK_F_NOUNDEFFILTER;
			break;

		case LDAP_BACK_CFG_ONERR:
			li->li_flags &= ~LDAP_BACK_F_ONERR_STOP;
			break;

		case LDAP_BACK_CFG_KEEPALIVE:
			li->li_tls.sb_keepalive.sk_idle = 0;
			li->li_tls.sb_keepalive.sk_probes = 0;
			li->li_tls.sb_keepalive.sk_interval = 0;
			break;

		default:
			/* FIXME: we need to handle all... */
			assert( 0 );
			break;
		}
		return rc;

	}

	switch( c->type ) {
	case LDAP_BACK_CFG_URI: {
		LDAPURLDesc	*tmpludp, *lud;
		char		**urllist = NULL;
		int		urlrc = LDAP_URL_SUCCESS, i;

		if ( li->li_uri != NULL ) {
			ch_free( li->li_uri );
			li->li_uri = NULL;

			assert( li->li_bvuri != NULL );
			ber_bvarray_free( li->li_bvuri );
			li->li_bvuri = NULL;
		}

		/* PARANOID: DN and more are not required nor allowed */
		urlrc = ldap_url_parselist_ext( &lud, c->argv[ 1 ], ", \t", LDAP_PVT_URL_PARSE_NONE );
		if ( urlrc != LDAP_URL_SUCCESS ) {
			char	*why;

			switch ( urlrc ) {
			case LDAP_URL_ERR_MEM:
				why = "no memory";
				break;
			case LDAP_URL_ERR_PARAM:
		  		why = "parameter is bad";
				break;
			case LDAP_URL_ERR_BADSCHEME:
				why = "URL doesn't begin with \"[c]ldap[si]://\"";
				break;
			case LDAP_URL_ERR_BADENCLOSURE:
				why = "URL is missing trailing \">\"";
				break;
			case LDAP_URL_ERR_BADURL:
				why = "URL is bad";
				break;
			case LDAP_URL_ERR_BADHOST:
				why = "host/port is bad";
				break;
			case LDAP_URL_ERR_BADATTRS:
				why = "bad (or missing) attributes";
				break;
			case LDAP_URL_ERR_BADSCOPE:
				why = "scope string is invalid (or missing)";
				break;
			case LDAP_URL_ERR_BADFILTER:
				why = "bad or missing filter";
				break;
			case LDAP_URL_ERR_BADEXTS:
				why = "bad or missing extensions";
				break;
			default:
				why = "unknown reason";
				break;
			}
			snprintf( c->cr_msg, sizeof( c->cr_msg),
					"unable to parse uri \"%s\" "
					"in \"uri <uri>\" line: %s",
					c->value_string, why );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			urlrc = 1;
			goto done_url;
		}

		for ( i = 0, tmpludp = lud;
				tmpludp;
				i++, tmpludp = tmpludp->lud_next )
		{
			if ( ( tmpludp->lud_dn != NULL
						&& tmpludp->lud_dn[0] != '\0' )
					|| tmpludp->lud_attrs != NULL
					/* || tmpludp->lud_scope != LDAP_SCOPE_DEFAULT */
					|| tmpludp->lud_filter != NULL
					|| tmpludp->lud_exts != NULL )
			{
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"warning, only protocol, "
						"host and port allowed "
						"in \"uri <uri>\" statement "
						"for uri #%d of \"%s\"",
						i, c->argv[ 1 ] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			}
		}

		for ( i = 0, tmpludp = lud;
				tmpludp;
				i++, tmpludp = tmpludp->lud_next )
			/* just count */
			;
		urllist = ch_calloc( sizeof( char * ), i + 1 );

		for ( i = 0, tmpludp = lud;
				tmpludp;
				i++, tmpludp = tmpludp->lud_next )
		{
			LDAPURLDesc	tmplud;

			tmplud = *tmpludp;
			tmplud.lud_dn = "";
			tmplud.lud_attrs = NULL;
			tmplud.lud_filter = NULL;
			if ( !ldap_is_ldapi_url( tmplud.lud_scheme ) ) {
				tmplud.lud_exts = NULL;
				tmplud.lud_crit_exts = 0;
			}

			urllist[ i ]  = ldap_url_desc2str( &tmplud );

			if ( urllist[ i ] == NULL ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg),
					"unable to rebuild uri "
					"in \"uri <uri>\" statement "
					"for \"%s\"",
					c->argv[ 1 ] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				urlrc = 1;
				goto done_url;
			}
		}

		li->li_uri = ldap_charray2str( urllist, " " );
		for ( i = 0; urllist[ i ] != NULL; i++ ) {
			struct berval	bv;

			ber_str2bv( urllist[ i ], 0, 0, &bv );
			ber_bvarray_add( &li->li_bvuri, &bv );
			urllist[ i ] = NULL;
		}
		ldap_memfree( urllist );
		urllist = NULL;

done_url:;
		if ( urllist ) {
			ldap_charray_free( urllist );
		}
		if ( lud ) {
			ldap_free_urllist( lud );
		}
		if ( urlrc != LDAP_URL_SUCCESS ) {
			return 1;
		}
		break;
	}

	case LDAP_BACK_CFG_TLS:
		i = verb_to_mask( c->argv[1], tls_mode );
		if ( BER_BVISNULL( &tls_mode[i].word ) ) {
			return 1;
		}
		li->li_flags &= ~LDAP_BACK_F_TLS_MASK;
		li->li_flags |= tls_mode[i].mask;
		if ( c->argc > 2 ) {
			for ( i=2; i<c->argc; i++ ) {
				if ( bindconf_tls_parse( c->argv[i], &li->li_tls ))
					return 1;
			}
			bindconf_tls_defaults( &li->li_tls );
		}
		break;

	case LDAP_BACK_CFG_ACL_AUTHCDN:
		switch ( li->li_acl_authmethod ) {
		case LDAP_AUTH_NONE:
			li->li_acl_authmethod = LDAP_AUTH_SIMPLE;
			break;

		case LDAP_AUTH_SIMPLE:
			break;

		default:
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"\"acl-authcDN <DN>\" incompatible "
				"with auth method %d",
				li->li_acl_authmethod );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		if ( !BER_BVISNULL( &li->li_acl_authcDN ) ) {
			free( li->li_acl_authcDN.bv_val );
		}
		ber_memfree_x( c->value_dn.bv_val, NULL );
		li->li_acl_authcDN = c->value_ndn;
		BER_BVZERO( &c->value_dn );
		BER_BVZERO( &c->value_ndn );
		break;

	case LDAP_BACK_CFG_ACL_PASSWD:
		switch ( li->li_acl_authmethod ) {
		case LDAP_AUTH_NONE:
			li->li_acl_authmethod = LDAP_AUTH_SIMPLE;
			break;

		case LDAP_AUTH_SIMPLE:
			break;

		default:
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"acl-passwd <cred>\" incompatible "
				"with auth method %d",
				li->li_acl_authmethod );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		if ( !BER_BVISNULL( &li->li_acl_passwd ) ) {
			free( li->li_acl_passwd.bv_val );
		}
		ber_str2bv( c->argv[ 1 ], 0, 1, &li->li_acl_passwd );
		break;

	case LDAP_BACK_CFG_ACL_METHOD:
	case LDAP_BACK_CFG_ACL_BIND:
		for ( i = 1; i < c->argc; i++ ) {
			if ( bindconf_parse( c->argv[ i ], &li->li_acl ) ) {
				return 1;
			}
		}
		bindconf_tls_defaults( &li->li_acl );
		break;

	case LDAP_BACK_CFG_IDASSERT_MODE:
		i = verb_to_mask( c->argv[1], idassert_mode );
		if ( BER_BVISNULL( &idassert_mode[i].word ) ) {
			if ( strncasecmp( c->argv[1], "u:", STRLENOF( "u:" ) ) == 0 ) {
				li->li_idassert_mode = LDAP_BACK_IDASSERT_OTHERID;
				ber_str2bv( c->argv[1], 0, 1, &li->li_idassert_authzID );
				li->li_idassert_authzID.bv_val[ 0 ] = 'u';
				
			} else {
				struct berval	id, ndn;

				ber_str2bv( c->argv[1], 0, 0, &id );

				if ( strncasecmp( c->argv[1], "dn:", STRLENOF( "dn:" ) ) == 0 ) {
					id.bv_val += STRLENOF( "dn:" );
					id.bv_len -= STRLENOF( "dn:" );
				}

				rc = dnNormalize( 0, NULL, NULL, &id, &ndn, NULL );
                                if ( rc != LDAP_SUCCESS ) {
                                        Debug( LDAP_DEBUG_ANY,
                                                "%s: line %d: idassert ID \"%s\" is not a valid DN\n",
                                                c->fname, c->lineno, c->argv[1] );
                                        return 1;
                                }

                                li->li_idassert_authzID.bv_len = STRLENOF( "dn:" ) + ndn.bv_len;
                                li->li_idassert_authzID.bv_val = ch_malloc( li->li_idassert_authzID.bv_len + 1 );
                                AC_MEMCPY( li->li_idassert_authzID.bv_val, "dn:", STRLENOF( "dn:" ) );
                                AC_MEMCPY( &li->li_idassert_authzID.bv_val[ STRLENOF( "dn:" ) ], ndn.bv_val, ndn.bv_len + 1 );
                                ch_free( ndn.bv_val );

                                li->li_idassert_mode = LDAP_BACK_IDASSERT_OTHERDN;
			}

		} else {
			li->li_idassert_mode = idassert_mode[i].mask;
		}

		if ( c->argc > 2 ) {
			int	i;

			for ( i = 2; i < c->argc; i++ ) {
				if ( strcasecmp( c->argv[ i ], "override" ) == 0 ) {
					li->li_idassert_flags |= LDAP_BACK_AUTH_OVERRIDE;

				} else if ( strcasecmp( c->argv[ i ], "prescriptive" ) == 0 ) {
					li->li_idassert_flags |= LDAP_BACK_AUTH_PRESCRIPTIVE;

				} else if ( strcasecmp( c->argv[ i ], "non-prescriptive" ) == 0 ) {
					li->li_idassert_flags &= ( ~LDAP_BACK_AUTH_PRESCRIPTIVE );

				} else if ( strcasecmp( c->argv[ i ], "obsolete-proxy-authz" ) == 0 ) {
					if ( li->li_idassert_flags & LDAP_BACK_AUTH_OBSOLETE_ENCODING_WORKAROUND ) {
						Debug( LDAP_DEBUG_ANY,
                                       	 		"%s: line %d: \"obsolete-proxy-authz\" flag "
                                        		"in \"idassert-mode <args>\" "
                                        		"incompatible with previously issued \"obsolete-encoding-workaround\" flag.\n",
                                        		c->fname, c->lineno, 0 );
                                		return 1;
					}
					li->li_idassert_flags |= LDAP_BACK_AUTH_OBSOLETE_PROXY_AUTHZ;

				} else if ( strcasecmp( c->argv[ i ], "obsolete-encoding-workaround" ) == 0 ) {
					if ( li->li_idassert_flags & LDAP_BACK_AUTH_OBSOLETE_PROXY_AUTHZ ) {
						Debug( LDAP_DEBUG_ANY,
                                       	 		"%s: line %d: \"obsolete-encoding-workaround\" flag "
                                        		"in \"idassert-mode <args>\" "
                                        		"incompatible with previously issued \"obsolete-proxy-authz\" flag.\n",
                                        		c->fname, c->lineno, 0 );
                                		return 1;
					}
					li->li_idassert_flags |= LDAP_BACK_AUTH_OBSOLETE_ENCODING_WORKAROUND;

				} else {
					Debug( LDAP_DEBUG_ANY,
                                        	"%s: line %d: unknown flag #%d "
                                        	"in \"idassert-mode <args> "
                                        	"[<flags>]\" line.\n",
                                        	c->fname, c->lineno, i - 2 );
                                	return 1;
				}
                        }
                }
		break;

	case LDAP_BACK_CFG_IDASSERT_AUTHCDN:
		switch ( li->li_idassert_authmethod ) {
		case LDAP_AUTH_NONE:
			li->li_idassert_authmethod = LDAP_AUTH_SIMPLE;
			break;

		case LDAP_AUTH_SIMPLE:
			break;

		default:
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"idassert-authcDN <DN>\" incompatible "
				"with auth method %d",
				li->li_idassert_authmethod );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		if ( !BER_BVISNULL( &li->li_idassert_authcDN ) ) {
			free( li->li_idassert_authcDN.bv_val );
		}
		ber_memfree_x( c->value_dn.bv_val, NULL );
		li->li_idassert_authcDN = c->value_ndn;
		BER_BVZERO( &c->value_dn );
		BER_BVZERO( &c->value_ndn );
		break;

	case LDAP_BACK_CFG_IDASSERT_PASSWD:
		switch ( li->li_idassert_authmethod ) {
		case LDAP_AUTH_NONE:
			li->li_idassert_authmethod = LDAP_AUTH_SIMPLE;
			break;

		case LDAP_AUTH_SIMPLE:
			break;

		default:
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"idassert-passwd <cred>\" incompatible "
				"with auth method %d",
				li->li_idassert_authmethod );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		if ( !BER_BVISNULL( &li->li_idassert_passwd ) ) {
			free( li->li_idassert_passwd.bv_val );
		}
		ber_str2bv( c->argv[ 1 ], 0, 1, &li->li_idassert_passwd );
		break;

	case LDAP_BACK_CFG_IDASSERT_AUTHZFROM:
		rc = slap_idassert_authzfrom_parse( c, &li->li_idassert );
		break;

	case LDAP_BACK_CFG_IDASSERT_PASSTHRU:
		rc = slap_idassert_passthru_parse( c, &li->li_idassert );
		break;

	case LDAP_BACK_CFG_IDASSERT_METHOD:
		/* no longer supported */
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			"\"idassert-method <args>\": "
			"no longer supported; use \"idassert-bind\"" );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		return 1;

	case LDAP_BACK_CFG_IDASSERT_BIND:
		rc = slap_idassert_parse( c, &li->li_idassert );
		break;

	case LDAP_BACK_CFG_REBIND:
		if ( c->argc == 1 || c->value_int ) {
			li->li_flags |= LDAP_BACK_F_SAVECRED;

		} else {
			li->li_flags &= ~LDAP_BACK_F_SAVECRED;
		}
		break;

	case LDAP_BACK_CFG_CHASE:
		if ( c->argc == 1 || c->value_int ) {
			li->li_flags |= LDAP_BACK_F_CHASE_REFERRALS;

		} else {
			li->li_flags &= ~LDAP_BACK_F_CHASE_REFERRALS;
		}
		break;

	case LDAP_BACK_CFG_T_F: {
		slap_mask_t		mask;

		i = verb_to_mask( c->argv[1], t_f_mode );
		if ( BER_BVISNULL( &t_f_mode[i].word ) ) {
			return 1;
		}

		mask = t_f_mode[i].mask;

		if ( LDAP_BACK_ISOPEN( li )
			&& mask == LDAP_BACK_F_T_F_DISCOVER
			&& !LDAP_BACK_T_F( li ) )
		{
			slap_bindconf	sb = { BER_BVNULL };
			int		rc;

			if ( li->li_uri == NULL ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"need URI to discover absolute filters support "
					"in \"t-f-support discover\"" );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}

			ber_str2bv( li->li_uri, 0, 0, &sb.sb_uri );
			sb.sb_version = li->li_version;
			sb.sb_method = LDAP_AUTH_SIMPLE;
			BER_BVSTR( &sb.sb_binddn, "" );

			rc = slap_discover_feature( &sb,
					slap_schema.si_ad_supportedFeatures->ad_cname.bv_val,
					LDAP_FEATURE_ABSOLUTE_FILTERS );
			if ( rc == LDAP_COMPARE_TRUE ) {
				mask |= LDAP_BACK_F_T_F;
			}
		}

		li->li_flags &= ~LDAP_BACK_F_T_F_MASK2;
		li->li_flags |= mask;
		} break;

	case LDAP_BACK_CFG_WHOAMI:
		if ( c->argc == 1 || c->value_int ) {
			li->li_flags |= LDAP_BACK_F_PROXY_WHOAMI;
			load_extop( (struct berval *)&slap_EXOP_WHOAMI,
					0, ldap_back_exop_whoami );

		} else {
			li->li_flags &= ~LDAP_BACK_F_PROXY_WHOAMI;
		}
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
					li->li_timeout[ j ] = u;
				}

				continue;
			}

			if ( slap_cf_aux_table_parse( c->argv[ i ], li->li_timeout, timeout_table, "slapd-ldap timeout" ) ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg),
					"unable to parse timeout \"%s\"",
					c->argv[ i ] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
		}
		break;

	case LDAP_BACK_CFG_IDLE_TIMEOUT: {
		unsigned long	t;

		if ( lutil_parse_time( c->argv[ 1 ], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"unable to parse idle timeout \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		li->li_idle_timeout = (time_t)t;
		} break;

	case LDAP_BACK_CFG_CONN_TTL: {
		unsigned long	t;

		if ( lutil_parse_time( c->argv[ 1 ], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"unable to parse conn ttl\"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		li->li_conn_ttl = (time_t)t;
		} break;

	case LDAP_BACK_CFG_NETWORK_TIMEOUT: {
		unsigned long	t;

		if ( lutil_parse_time( c->argv[ 1 ], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"unable to parse network timeout \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		li->li_network_timeout = (time_t)t;
		} break;

	case LDAP_BACK_CFG_VERSION:
		if ( c->value_int != 0 && ( c->value_int < LDAP_VERSION_MIN || c->value_int > LDAP_VERSION_MAX ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"unsupported version \"%s\" "
				"in \"protocol-version <version>\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}

		li->li_version = c->value_int;
		break;

	case LDAP_BACK_CFG_SINGLECONN:
		if ( c->value_int ) {
			li->li_flags |= LDAP_BACK_F_SINGLECONN;

		} else {
			li->li_flags &= ~LDAP_BACK_F_SINGLECONN;
		}
		break;

	case LDAP_BACK_CFG_USETEMP:
		if ( c->value_int ) {
			li->li_flags |= LDAP_BACK_F_USE_TEMPORARIES;

		} else {
			li->li_flags &= ~LDAP_BACK_F_USE_TEMPORARIES;
		}
		break;

	case LDAP_BACK_CFG_CONNPOOLMAX:
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
		li->li_conn_priv_max = c->value_int;
		break;

	case LDAP_BACK_CFG_CANCEL: {
		slap_mask_t		mask;

		i = verb_to_mask( c->argv[1], cancel_mode );
		if ( BER_BVISNULL( &cancel_mode[i].word ) ) {
			return 1;
		}

		mask = cancel_mode[i].mask;

		if ( LDAP_BACK_ISOPEN( li )
			&& mask == LDAP_BACK_F_CANCEL_EXOP_DISCOVER
			&& !LDAP_BACK_CANCEL( li ) )
		{
			slap_bindconf	sb = { BER_BVNULL };
			int		rc;

			if ( li->li_uri == NULL ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"need URI to discover \"cancel\" support "
					"in \"cancel exop-discover\"" );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}

			ber_str2bv( li->li_uri, 0, 0, &sb.sb_uri );
			sb.sb_version = li->li_version;
			sb.sb_method = LDAP_AUTH_SIMPLE;
			BER_BVSTR( &sb.sb_binddn, "" );

			rc = slap_discover_feature( &sb,
					slap_schema.si_ad_supportedExtension->ad_cname.bv_val,
					LDAP_EXOP_CANCEL );
			if ( rc == LDAP_COMPARE_TRUE ) {
				mask |= LDAP_BACK_F_CANCEL_EXOP;
			}
		}

		li->li_flags &= ~LDAP_BACK_F_CANCEL_MASK2;
		li->li_flags |= mask;
		} break;

	case LDAP_BACK_CFG_QUARANTINE:
		if ( LDAP_BACK_QUARANTINE( li ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"quarantine already defined" );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		rc = slap_retry_info_parse( c->argv[1], &li->li_quarantine,
			c->cr_msg, sizeof( c->cr_msg ) );
		if ( rc ) {
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );

		} else {
			ldap_pvt_thread_mutex_init( &li->li_quarantine_mutex );
			/* give it a chance to retry if the pattern gets reset
			 * via back-config */
			li->li_isquarantined = 0;
			li->li_flags |= LDAP_BACK_F_QUARANTINE;
		}
		break;

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
	case LDAP_BACK_CFG_ST_REQUEST:
		if ( c->value_int ) {
			li->li_flags |= LDAP_BACK_F_ST_REQUEST;

		} else {
			li->li_flags &= ~LDAP_BACK_F_ST_REQUEST;
		}
		break;
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

	case LDAP_BACK_CFG_NOREFS:
		if ( c->value_int ) {
			li->li_flags |= LDAP_BACK_F_NOREFS;

		} else {
			li->li_flags &= ~LDAP_BACK_F_NOREFS;
		}
		break;

	case LDAP_BACK_CFG_NOUNDEFFILTER:
		if ( c->value_int ) {
			li->li_flags |= LDAP_BACK_F_NOUNDEFFILTER;

		} else {
			li->li_flags &= ~LDAP_BACK_F_NOUNDEFFILTER;
		}
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
		li->li_flags &= ~LDAP_BACK_F_ONERR_STOP;
		li->li_flags |= onerr_mode[i].mask;
		break;

	case LDAP_BACK_CFG_REWRITE:
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			"rewrite/remap capabilities have been moved "
			"to the \"rwm\" overlay; see slapo-rwm(5) "
			"for details (hint: add \"overlay rwm\" "
			"and prefix all directives with \"rwm-\")" );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		return 1;

	case LDAP_BACK_CFG_KEEPALIVE:
		slap_keepalive_parse( ber_bvstrdup(c->argv[1]),
				 &li->li_tls.sb_keepalive, 0, 0, 0);
		break;
		
	default:
		/* FIXME: try to catch inconsistencies */
		assert( 0 );
		break;
	}

	return rc;
}

int
ldap_back_init_cf( BackendInfo *bi )
{
	int			rc;
	AttributeDescription	*ad = NULL;
	const char		*text;

	/* Make sure we don't exceed the bits reserved for userland */
	config_check_userland( LDAP_BACK_CFG_LAST );

	bi->bi_cf_ocs = ldapocs;

	rc = config_register_schema( ldapcfg, ldapocs );
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
ldap_pbind_cf_gen( ConfigArgs *c )
{
	slap_overinst	*on = (slap_overinst *)c->bi;
	void	*private = c->be->be_private;
	int		rc;

	c->be->be_private = on->on_bi.bi_private;
	rc = ldap_back_cf_gen( c );
	c->be->be_private = private;
	return rc;
}

int
ldap_pbind_init_cf( BackendInfo *bi )
{
	bi->bi_cf_ocs = pbindocs;

	return config_register_schema( pbindcfg, pbindocs );
}

static int
ldap_back_exop_whoami(
		Operation	*op,
		SlapReply	*rs )
{
	struct berval *bv = NULL;

	if ( op->oq_extended.rs_reqdata != NULL ) {
		/* no request data should be provided */
		rs->sr_text = "no request data expected";
		return rs->sr_err = LDAP_PROTOCOL_ERROR;
	}

	Statslog( LDAP_DEBUG_STATS, "%s WHOAMI\n",
	    op->o_log_prefix, 0, 0, 0, 0 );

	rs->sr_err = backend_check_restrictions( op, rs, 
			(struct berval *)&slap_EXOP_WHOAMI );
	if( rs->sr_err != LDAP_SUCCESS ) return rs->sr_err;

	/* if auth'd by back-ldap and request is proxied, forward it */
	if ( op->o_conn->c_authz_backend
		&& !strcmp( op->o_conn->c_authz_backend->be_type, "ldap" )
		&& !dn_match( &op->o_ndn, &op->o_conn->c_ndn ) )
	{
		ldapconn_t	*lc = NULL;
		LDAPControl c, *ctrls[2] = {NULL, NULL};
		LDAPMessage *res;
		Operation op2 = *op;
		ber_int_t msgid;
		int doretry = 1;
		char *ptr;

		ctrls[0] = &c;
		op2.o_ndn = op->o_conn->c_ndn;
		if ( !ldap_back_dobind( &lc, &op2, rs, LDAP_BACK_SENDERR ) ) {
			return -1;
		}
		c.ldctl_oid = LDAP_CONTROL_PROXY_AUTHZ;
		c.ldctl_iscritical = 1;
		c.ldctl_value.bv_val = op->o_tmpalloc(
			op->o_ndn.bv_len + STRLENOF( "dn:" ) + 1,
			op->o_tmpmemctx );
		c.ldctl_value.bv_len = op->o_ndn.bv_len + 3;
		ptr = c.ldctl_value.bv_val;
		ptr = lutil_strcopy( ptr, "dn:" );
		ptr = lutil_strncopy( ptr, op->o_ndn.bv_val, op->o_ndn.bv_len );
		ptr[ 0 ] = '\0';

retry:
		rs->sr_err = ldap_whoami( lc->lc_ld, ctrls, NULL, &msgid );
		if ( rs->sr_err == LDAP_SUCCESS ) {
			/* by now, make sure no timeout is used (ITS#6282) */
			struct timeval tv = { -1, 0 };
			if ( ldap_result( lc->lc_ld, msgid, LDAP_MSG_ALL, &tv, &res ) == -1 ) {
				ldap_get_option( lc->lc_ld, LDAP_OPT_ERROR_NUMBER,
					&rs->sr_err );
				if ( rs->sr_err == LDAP_SERVER_DOWN && doretry ) {
					doretry = 0;
					if ( ldap_back_retry( &lc, op, rs, LDAP_BACK_SENDERR ) ) {
						goto retry;
					}
				}

			} else {
				/* NOTE: are we sure "bv" will be malloc'ed
				 * with the appropriate memory? */
				rs->sr_err = ldap_parse_whoami( lc->lc_ld, res, &bv );
				ldap_msgfree(res);
			}
		}
		op->o_tmpfree( c.ldctl_value.bv_val, op->o_tmpmemctx );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			rs->sr_err = slap_map_api2result( rs );
		}

		if ( lc != NULL ) {
			ldap_back_release_conn( (ldapinfo_t *)op2.o_bd->be_private, lc );
		}

	} else {
		/* else just do the same as before */
		bv = (struct berval *) ch_malloc( sizeof( struct berval ) );
		if ( !BER_BVISEMPTY( &op->o_dn ) ) {
			bv->bv_len = op->o_dn.bv_len + STRLENOF( "dn:" );
			bv->bv_val = ch_malloc( bv->bv_len + 1 );
			AC_MEMCPY( bv->bv_val, "dn:", STRLENOF( "dn:" ) );
			AC_MEMCPY( &bv->bv_val[ STRLENOF( "dn:" ) ], op->o_dn.bv_val,
				op->o_dn.bv_len );
			bv->bv_val[ bv->bv_len ] = '\0';

		} else {
			bv->bv_len = 0;
			bv->bv_val = NULL;
		}
	}

	rs->sr_rspdata = bv;
	return rs->sr_err;
}


