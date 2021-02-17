/* bconfig.c - the config backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
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
 * This work was originally developed by Howard Chu for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/errno.h>
#include <sys/stat.h>
#include <ac/unistd.h>

#include "slap.h"

#ifdef LDAP_SLAPI
#include "slapi/slapi.h"
#endif

#include <ldif.h>
#include <lutil.h>

#include "config.h"

#define	CONFIG_RDN	"cn=config"
#define	SCHEMA_RDN	"cn=schema"

static struct berval config_rdn = BER_BVC(CONFIG_RDN);
static struct berval schema_rdn = BER_BVC(SCHEMA_RDN);

extern int slap_DN_strict;	/* dn.c */

#ifdef SLAPD_MODULES
typedef struct modpath_s {
	struct modpath_s *mp_next;
	struct berval mp_path;
	BerVarray mp_loads;
} ModPaths;

static ModPaths modpaths, *modlast = &modpaths, *modcur = &modpaths;
#endif

typedef struct ConfigFile {
	struct ConfigFile *c_sibs;
	struct ConfigFile *c_kids;
	struct berval c_file;
	AttributeType *c_at_head, *c_at_tail;
	ContentRule *c_cr_head, *c_cr_tail;
	ObjectClass *c_oc_head, *c_oc_tail;
	OidMacro *c_om_head, *c_om_tail;
	Syntax *c_syn_head, *c_syn_tail;
	BerVarray c_dseFiles;
} ConfigFile;

typedef struct {
	ConfigFile *cb_config;
	CfEntryInfo *cb_root;
	BackendDB	cb_db;	/* underlying database */
	int		cb_got_ldif;
	int		cb_use_ldif;
} CfBackInfo;

static CfBackInfo cfBackInfo;

static char	*passwd_salt;
static FILE *logfile;
static char	*logfileName;
#ifdef SLAP_AUTH_REWRITE
static BerVarray authz_rewrites;
#endif
static AccessControl *defacl_parsed = NULL;

static struct berval cfdir;

/* Private state */
static AttributeDescription *cfAd_backend, *cfAd_database, *cfAd_overlay,
	*cfAd_include, *cfAd_attr, *cfAd_oc, *cfAd_om, *cfAd_syntax;

static ConfigFile *cfn;

static Avlnode *CfOcTree;

/* System schema state */
extern AttributeType *at_sys_tail;	/* at.c */
extern ObjectClass *oc_sys_tail;	/* oc.c */
extern OidMacro *om_sys_tail;	/* oidm.c */
extern Syntax *syn_sys_tail;	/* syntax.c */
static AttributeType *cf_at_tail;
static ObjectClass *cf_oc_tail;
static OidMacro *cf_om_tail;
static Syntax *cf_syn_tail;

static int config_add_internal( CfBackInfo *cfb, Entry *e, ConfigArgs *ca,
	SlapReply *rs, int *renumber, Operation *op );

static int config_check_schema( Operation *op, CfBackInfo *cfb );

static ConfigDriver config_fname;
static ConfigDriver config_cfdir;
static ConfigDriver config_generic;
static ConfigDriver config_search_base;
static ConfigDriver config_passwd_hash;
static ConfigDriver config_schema_dn;
static ConfigDriver config_sizelimit;
static ConfigDriver config_timelimit;
static ConfigDriver config_overlay;
static ConfigDriver config_subordinate; 
static ConfigDriver config_suffix; 
#ifdef LDAP_TCP_BUFFER
static ConfigDriver config_tcp_buffer; 
#endif /* LDAP_TCP_BUFFER */
static ConfigDriver config_rootdn;
static ConfigDriver config_rootpw;
static ConfigDriver config_restrict;
static ConfigDriver config_allows;
static ConfigDriver config_disallows;
static ConfigDriver config_requires;
static ConfigDriver config_security;
static ConfigDriver config_referral;
static ConfigDriver config_loglevel;
static ConfigDriver config_updatedn;
static ConfigDriver config_updateref;
static ConfigDriver config_extra_attrs;
static ConfigDriver config_include;
static ConfigDriver config_obsolete;
#ifdef HAVE_TLS
static ConfigDriver config_tls_option;
static ConfigDriver config_tls_config;
#endif
extern ConfigDriver syncrepl_config;

enum {
	CFG_ACL = 1,
	CFG_BACKEND,
	CFG_DATABASE,
	CFG_TLS_RAND,
	CFG_TLS_CIPHER,
	CFG_TLS_PROTOCOL_MIN,
	CFG_TLS_CERT_FILE,
	CFG_TLS_CERT_KEY,
	CFG_TLS_CA_PATH,
	CFG_TLS_CA_FILE,
	CFG_TLS_DH_FILE,
	CFG_TLS_VERIFY,
	CFG_TLS_CRLCHECK,
	CFG_TLS_CRL_FILE,
	CFG_CONCUR,
	CFG_THREADS,
	CFG_SALT,
	CFG_LIMITS,
	CFG_RO,
	CFG_REWRITE,
	CFG_DEPTH,
	CFG_OID,
	CFG_OC,
	CFG_DIT,
	CFG_ATTR,
	CFG_ATOPT,
	CFG_ROOTDSE,
	CFG_LOGFILE,
	CFG_PLUGIN,
	CFG_MODLOAD,
	CFG_MODPATH,
	CFG_LASTMOD,
	CFG_AZPOLICY,
	CFG_AZREGEXP,
	CFG_AZDUC,
	CFG_AZDUC_IGNORE,
	CFG_SASLSECP,
	CFG_SSTR_IF_MAX,
	CFG_SSTR_IF_MIN,
	CFG_TTHREADS,
	CFG_MIRRORMODE,
	CFG_HIDDEN,
	CFG_MONITORING,
	CFG_SERVERID,
	CFG_SORTVALS,
	CFG_IX_INTLEN,
	CFG_SYNTAX,
	CFG_ACL_ADD,
	CFG_SYNC_SUBENTRY,
	CFG_LTHREADS,
	CFG_IX_HASH64,
	CFG_DISABLED,
	CFG_THREADQS,
	CFG_TLS_ECNAME,

	CFG_LAST
};

typedef struct {
	char *name, *oid;
} OidRec;

static OidRec OidMacros[] = {
	/* OpenLDAProot:1.12.2 */
	{ "OLcfg", "1.3.6.1.4.1.4203.1.12.2" },
	{ "OLcfgAt", "OLcfg:3" },
	{ "OLcfgGlAt", "OLcfgAt:0" },
	{ "OLcfgBkAt", "OLcfgAt:1" },
	{ "OLcfgDbAt", "OLcfgAt:2" },
	{ "OLcfgOvAt", "OLcfgAt:3" },
	{ "OLcfgCtAt", "OLcfgAt:4" },	/* contrib modules */
	{ "OLcfgOc", "OLcfg:4" },
	{ "OLcfgGlOc", "OLcfgOc:0" },
	{ "OLcfgBkOc", "OLcfgOc:1" },
	{ "OLcfgDbOc", "OLcfgOc:2" },
	{ "OLcfgOvOc", "OLcfgOc:3" },
	{ "OLcfgCtOc", "OLcfgOc:4" },	/* contrib modules */

	/* Syntaxes. We should just start using the standard names and
	 * document that they are predefined and available for users
	 * to reference in their own schema. Defining schema without
	 * OID macros is for masochists...
	 */
	{ "OMsyn", "1.3.6.1.4.1.1466.115.121.1" },
	{ "OMsBoolean", "OMsyn:7" },
	{ "OMsDN", "OMsyn:12" },
	{ "OMsDirectoryString", "OMsyn:15" },
	{ "OMsIA5String", "OMsyn:26" },
	{ "OMsInteger", "OMsyn:27" },
	{ "OMsOID", "OMsyn:38" },
	{ "OMsOctetString", "OMsyn:40" },
	{ NULL, NULL }
};

/*
 * Backend/Database registry
 *
 * OLcfg{Bk|Db}{Oc|At}:0		-> common
 * OLcfg{Bk|Db}{Oc|At}:1		-> back-bdb(/back-hdb)
 * OLcfg{Bk|Db}{Oc|At}:2		-> back-ldif
 * OLcfg{Bk|Db}{Oc|At}:3		-> back-ldap/meta
 * OLcfg{Bk|Db}{Oc|At}:4		-> back-monitor
 * OLcfg{Bk|Db}{Oc|At}:5		-> back-relay
 * OLcfg{Bk|Db}{Oc|At}:6		-> back-sql(/back-ndb)
 * OLcfg{Bk|Db}{Oc|At}:7		-> back-sock
 * OLcfg{Bk|Db}{Oc|At}:8		-> back-null
 * OLcfg{Bk|Db}{Oc|At}:9		-> back-passwd
 * OLcfg{Bk|Db}{Oc|At}:10		-> back-shell
 * OLcfg{Bk|Db}{Oc|At}:11		-> back-perl
 * OLcfg{Bk|Db}{Oc|At}:12		-> back-mdb
 */

/*
 * Overlay registry
 *
 * OLcfgOv{Oc|At}:1			-> syncprov
 * OLcfgOv{Oc|At}:2			-> pcache
 * OLcfgOv{Oc|At}:3			-> chain
 * OLcfgOv{Oc|At}:4			-> accesslog
 * OLcfgOv{Oc|At}:5			-> valsort
 * OLcfgOv{Oc|At}:7			-> distproc
 * OLcfgOv{Oc|At}:8			-> dynlist
 * OLcfgOv{Oc|At}:9			-> dds
 * OLcfgOv{Oc|At}:10			-> unique
 * OLcfgOv{Oc|At}:11			-> refint
 * OLcfgOv{Oc|At}:12 			-> ppolicy
 * OLcfgOv{Oc|At}:13			-> constraint
 * OLcfgOv{Oc|At}:14			-> translucent
 * OLcfgOv{Oc|At}:15			-> auditlog
 * OLcfgOv{Oc|At}:16			-> rwm
 * OLcfgOv{Oc|At}:17			-> dyngroup
 * OLcfgOv{Oc|At}:18			-> memberof
 * OLcfgOv{Oc|At}:19			-> collect
 * OLcfgOv{Oc|At}:20			-> retcode
 * OLcfgOv{Oc|At}:21			-> sssvlv
 */

/* alphabetical ordering */

static ConfigTable config_back_cf_table[] = {
	/* This attr is read-only */
	{ "", "", 0, 0, 0, ARG_MAGIC,
		&config_fname, "( OLcfgGlAt:78 NAME 'olcConfigFile' "
			"DESC 'File for slapd configuration directives' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "", "", 0, 0, 0, ARG_MAGIC,
		&config_cfdir, "( OLcfgGlAt:79 NAME 'olcConfigDir' "
			"DESC 'Directory for slapd configuration backend' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "access",	NULL, 0, 0, 0, ARG_MAY_DB|ARG_MAGIC|CFG_ACL,
		&config_generic, "( OLcfgGlAt:1 NAME 'olcAccess' "
			"DESC 'Access Control List' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString X-ORDERED 'VALUES' )", NULL, NULL },
	{ "add_content_acl",	NULL, 0, 0, 0, ARG_MAY_DB|ARG_ON_OFF|ARG_MAGIC|CFG_ACL_ADD,
		&config_generic, "( OLcfgGlAt:86 NAME 'olcAddContentAcl' "
			"DESC 'Check ACLs against content of Add ops' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "allows",	"features", 2, 0, 5, ARG_PRE_DB|ARG_MAGIC,
		&config_allows, "( OLcfgGlAt:2 NAME 'olcAllows' "
			"DESC 'Allowed set of deprecated features' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "argsfile", "file", 2, 2, 0, ARG_STRING,
		&slapd_args_file, "( OLcfgGlAt:3 NAME 'olcArgsFile' "
			"DESC 'File for slapd command line options' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "attributeoptions", NULL, 0, 0, 0, ARG_MAGIC|CFG_ATOPT,
		&config_generic, "( OLcfgGlAt:5 NAME 'olcAttributeOptions' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "attribute",	"attribute", 2, 0, STRLENOF( "attribute" ),
		ARG_PAREN|ARG_MAGIC|CFG_ATTR,
		&config_generic, "( OLcfgGlAt:4 NAME 'olcAttributeTypes' "
			"DESC 'OpenLDAP attributeTypes' "
			"EQUALITY caseIgnoreMatch "
			"SUBSTR caseIgnoreSubstringsMatch "
			"SYNTAX OMsDirectoryString X-ORDERED 'VALUES' )",
				NULL, NULL },
	{ "authid-rewrite", NULL, 2, 0, STRLENOF( "authid-rewrite" ),
#ifdef SLAP_AUTH_REWRITE
		ARG_MAGIC|CFG_REWRITE|ARG_NO_INSERT, &config_generic,
#else
		ARG_IGNORED, NULL,
#endif
		 "( OLcfgGlAt:6 NAME 'olcAuthIDRewrite' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString X-ORDERED 'VALUES' )", NULL, NULL },
	{ "authz-policy", "policy", 2, 2, 0, ARG_STRING|ARG_MAGIC|CFG_AZPOLICY,
		&config_generic, "( OLcfgGlAt:7 NAME 'olcAuthzPolicy' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "authz-regexp", "regexp> <DN", 3, 3, 0, ARG_MAGIC|CFG_AZREGEXP|ARG_NO_INSERT,
		&config_generic, "( OLcfgGlAt:8 NAME 'olcAuthzRegexp' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString X-ORDERED 'VALUES' )", NULL, NULL },
	{ "backend", "type", 2, 2, 0, ARG_PRE_DB|ARG_MAGIC|CFG_BACKEND,
		&config_generic, "( OLcfgGlAt:9 NAME 'olcBackend' "
			"DESC 'A type of backend' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE X-ORDERED 'SIBLINGS' )",
				NULL, NULL },
	{ "concurrency", "level", 2, 2, 0, ARG_INT|ARG_MAGIC|CFG_CONCUR,
		&config_generic, "( OLcfgGlAt:10 NAME 'olcConcurrency' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "conn_max_pending", "max", 2, 2, 0, ARG_INT,
		&slap_conn_max_pending, "( OLcfgGlAt:11 NAME 'olcConnMaxPending' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "conn_max_pending_auth", "max", 2, 2, 0, ARG_INT,
		&slap_conn_max_pending_auth, "( OLcfgGlAt:12 NAME 'olcConnMaxPendingAuth' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "database", "type", 2, 2, 0, ARG_MAGIC|CFG_DATABASE,
		&config_generic, "( OLcfgGlAt:13 NAME 'olcDatabase' "
			"DESC 'The backend type for a database instance' "
			"SUP olcBackend SINGLE-VALUE X-ORDERED 'SIBLINGS' )", NULL, NULL },
	{ "defaultSearchBase", "dn", 2, 2, 0, ARG_PRE_BI|ARG_PRE_DB|ARG_DN|ARG_QUOTE|ARG_MAGIC,
		&config_search_base, "( OLcfgGlAt:14 NAME 'olcDefaultSearchBase' "
			"SYNTAX OMsDN SINGLE-VALUE )", NULL, NULL },
	{ "disabled", "on|off", 2, 2, 0, ARG_DB|ARG_ON_OFF|ARG_MAGIC|CFG_DISABLED,
		&config_generic, "( OLcfgDbAt:0.21 NAME 'olcDisabled' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "disallows", "features", 2, 0, 8, ARG_PRE_DB|ARG_MAGIC,
		&config_disallows, "( OLcfgGlAt:15 NAME 'olcDisallows' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "ditcontentrule",	NULL, 0, 0, 0, ARG_MAGIC|CFG_DIT|ARG_NO_DELETE|ARG_NO_INSERT,
		&config_generic, "( OLcfgGlAt:16 NAME 'olcDitContentRules' "
			"DESC 'OpenLDAP DIT content rules' "
			"EQUALITY caseIgnoreMatch "
			"SUBSTR caseIgnoreSubstringsMatch "
			"SYNTAX OMsDirectoryString X-ORDERED 'VALUES' )",
			NULL, NULL },
	{ "extra_attrs", "attrlist", 2, 2, 0, ARG_DB|ARG_MAGIC,
		&config_extra_attrs, "( OLcfgDbAt:0.20 NAME 'olcExtraAttrs' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "gentlehup", "on|off", 2, 2, 0,
#ifdef SIGHUP
		ARG_ON_OFF, &global_gentlehup,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:17 NAME 'olcGentleHUP' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "hidden", "on|off", 2, 2, 0, ARG_DB|ARG_ON_OFF|ARG_MAGIC|CFG_HIDDEN,
		&config_generic, "( OLcfgDbAt:0.17 NAME 'olcHidden' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "idletimeout", "timeout", 2, 2, 0, ARG_INT,
		&global_idletimeout, "( OLcfgGlAt:18 NAME 'olcIdleTimeout' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "include", "file", 2, 2, 0, ARG_MAGIC,
		&config_include, "( OLcfgGlAt:19 NAME 'olcInclude' "
			"SUP labeledURI )", NULL, NULL },
	{ "index_hash64", "on|off", 2, 2, 0, ARG_ON_OFF|ARG_MAGIC|CFG_IX_HASH64,
		&config_generic, "( OLcfgGlAt:94 NAME 'olcIndexHash64' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "index_substr_if_minlen", "min", 2, 2, 0, ARG_UINT|ARG_NONZERO|ARG_MAGIC|CFG_SSTR_IF_MIN,
		&config_generic, "( OLcfgGlAt:20 NAME 'olcIndexSubstrIfMinLen' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "index_substr_if_maxlen", "max", 2, 2, 0, ARG_UINT|ARG_NONZERO|ARG_MAGIC|CFG_SSTR_IF_MAX,
		&config_generic, "( OLcfgGlAt:21 NAME 'olcIndexSubstrIfMaxLen' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "index_substr_any_len", "len", 2, 2, 0, ARG_UINT|ARG_NONZERO,
		&index_substr_any_len, "( OLcfgGlAt:22 NAME 'olcIndexSubstrAnyLen' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "index_substr_any_step", "step", 2, 2, 0, ARG_UINT|ARG_NONZERO,
		&index_substr_any_step, "( OLcfgGlAt:23 NAME 'olcIndexSubstrAnyStep' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "index_intlen", "len", 2, 2, 0, ARG_UINT|ARG_MAGIC|CFG_IX_INTLEN,
		&config_generic, "( OLcfgGlAt:84 NAME 'olcIndexIntLen' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "lastmod", "on|off", 2, 2, 0, ARG_DB|ARG_ON_OFF|ARG_MAGIC|CFG_LASTMOD,
		&config_generic, "( OLcfgDbAt:0.4 NAME 'olcLastMod' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "ldapsyntax",	"syntax", 2, 0, 0,
		ARG_PAREN|ARG_MAGIC|CFG_SYNTAX,
		&config_generic, "( OLcfgGlAt:85 NAME 'olcLdapSyntaxes' "
			"DESC 'OpenLDAP ldapSyntax' "
			"EQUALITY caseIgnoreMatch "
			"SUBSTR caseIgnoreSubstringsMatch "
			"SYNTAX OMsDirectoryString X-ORDERED 'VALUES' )",
				NULL, NULL },
	{ "limits", "limits", 2, 0, 0, ARG_DB|ARG_MAGIC|CFG_LIMITS,
		&config_generic, "( OLcfgDbAt:0.5 NAME 'olcLimits' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString X-ORDERED 'VALUES' )", NULL, NULL },
	{ "listener-threads", "count", 2, 0, 0,
#ifdef NO_THREADS
		ARG_IGNORED, NULL,
#else
		ARG_UINT|ARG_MAGIC|CFG_LTHREADS, &config_generic,
#endif
		"( OLcfgGlAt:93 NAME 'olcListenerThreads' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "localSSF", "ssf", 2, 2, 0, ARG_INT,
		&local_ssf, "( OLcfgGlAt:26 NAME 'olcLocalSSF' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "logfile", "file", 2, 2, 0, ARG_STRING|ARG_MAGIC|CFG_LOGFILE,
		&config_generic, "( OLcfgGlAt:27 NAME 'olcLogFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "loglevel", "level", 2, 0, 0, ARG_MAGIC,
		&config_loglevel, "( OLcfgGlAt:28 NAME 'olcLogLevel' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "maxDerefDepth", "depth", 2, 2, 0, ARG_DB|ARG_INT|ARG_MAGIC|CFG_DEPTH,
		&config_generic, "( OLcfgDbAt:0.6 NAME 'olcMaxDerefDepth' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "mirrormode", "on|off", 2, 2, 0, ARG_DB|ARG_ON_OFF|ARG_MAGIC|CFG_MIRRORMODE,
		&config_generic, "( OLcfgDbAt:0.16 NAME 'olcMirrorMode' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "moduleload",	"file", 2, 0, 0,
#ifdef SLAPD_MODULES
		ARG_MAGIC|CFG_MODLOAD|ARG_NO_DELETE, &config_generic,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:30 NAME 'olcModuleLoad' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString X-ORDERED 'VALUES' )", NULL, NULL },
	{ "modulepath", "path", 2, 2, 0,
#ifdef SLAPD_MODULES
		ARG_MAGIC|CFG_MODPATH|ARG_NO_DELETE|ARG_NO_INSERT, &config_generic,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:31 NAME 'olcModulePath' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "monitoring", "TRUE|FALSE", 2, 2, 0,
		ARG_MAGIC|CFG_MONITORING|ARG_DB|ARG_ON_OFF, &config_generic,
		"( OLcfgDbAt:0.18 NAME 'olcMonitoring' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "objectclass", "objectclass", 2, 0, 0, ARG_PAREN|ARG_MAGIC|CFG_OC,
		&config_generic, "( OLcfgGlAt:32 NAME 'olcObjectClasses' "
		"DESC 'OpenLDAP object classes' "
		"EQUALITY caseIgnoreMatch "
		"SUBSTR caseIgnoreSubstringsMatch "
		"SYNTAX OMsDirectoryString X-ORDERED 'VALUES' )",
			NULL, NULL },
	{ "objectidentifier", "name> <oid",	3, 3, 0, ARG_MAGIC|CFG_OID,
		&config_generic, "( OLcfgGlAt:33 NAME 'olcObjectIdentifier' "
			"EQUALITY caseIgnoreMatch "
			"SUBSTR caseIgnoreSubstringsMatch "
			"SYNTAX OMsDirectoryString X-ORDERED 'VALUES' )", NULL, NULL },
	{ "overlay", "overlay", 2, 2, 0, ARG_MAGIC,
		&config_overlay, "( OLcfgGlAt:34 NAME 'olcOverlay' "
			"SUP olcDatabase SINGLE-VALUE X-ORDERED 'SIBLINGS' )", NULL, NULL },
	{ "password-crypt-salt-format", "salt", 2, 2, 0, ARG_STRING|ARG_MAGIC|CFG_SALT,
		&config_generic, "( OLcfgGlAt:35 NAME 'olcPasswordCryptSaltFormat' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "password-hash", "hash", 2, 0, 0, ARG_MAGIC,
		&config_passwd_hash, "( OLcfgGlAt:36 NAME 'olcPasswordHash' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "pidfile", "file", 2, 2, 0, ARG_STRING,
		&slapd_pid_file, "( OLcfgGlAt:37 NAME 'olcPidFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "plugin", NULL, 0, 0, 0,
#ifdef LDAP_SLAPI
		ARG_MAGIC|CFG_PLUGIN, &config_generic,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:38 NAME 'olcPlugin' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "pluginlog", "filename", 2, 2, 0,
#ifdef LDAP_SLAPI
		ARG_STRING, &slapi_log_file,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:39 NAME 'olcPluginLogFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "readonly", "on|off", 2, 2, 0, ARG_MAY_DB|ARG_ON_OFF|ARG_MAGIC|CFG_RO,
		&config_generic, "( OLcfgGlAt:40 NAME 'olcReadOnly' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "referral", "url", 2, 2, 0, ARG_MAGIC,
		&config_referral, "( OLcfgGlAt:41 NAME 'olcReferral' "
			"SUP labeledURI SINGLE-VALUE )", NULL, NULL },
	{ "replica", "host or uri", 2, 0, 0, ARG_DB|ARG_MAGIC,
		&config_obsolete, "( OLcfgDbAt:0.7 NAME 'olcReplica' "
			"EQUALITY caseIgnoreMatch "
			"SUP labeledURI X-ORDERED 'VALUES' )", NULL, NULL },
	{ "replica-argsfile", NULL, 0, 0, 0, ARG_MAY_DB|ARG_MAGIC,
		&config_obsolete, "( OLcfgGlAt:43 NAME 'olcReplicaArgsFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "replica-pidfile", NULL, 0, 0, 0, ARG_MAY_DB|ARG_MAGIC,
		&config_obsolete, "( OLcfgGlAt:44 NAME 'olcReplicaPidFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "replicationInterval", NULL, 0, 0, 0, ARG_MAY_DB|ARG_MAGIC,
		&config_obsolete, "( OLcfgGlAt:45 NAME 'olcReplicationInterval' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "replogfile", "filename", 2, 2, 0, ARG_MAY_DB|ARG_MAGIC,
		&config_obsolete, "( OLcfgGlAt:46 NAME 'olcReplogFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "require", "features", 2, 0, 7, ARG_MAY_DB|ARG_MAGIC,
		&config_requires, "( OLcfgGlAt:47 NAME 'olcRequires' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "restrict", "op_list", 2, 0, 0, ARG_MAY_DB|ARG_MAGIC,
		&config_restrict, "( OLcfgGlAt:48 NAME 'olcRestrict' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "reverse-lookup", "on|off", 2, 2, 0,
#ifdef SLAPD_RLOOKUPS
		ARG_ON_OFF, &use_reverse_lookup,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:49 NAME 'olcReverseLookup' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "rootdn", "dn", 2, 2, 0, ARG_DB|ARG_DN|ARG_QUOTE|ARG_MAGIC,
		&config_rootdn, "( OLcfgDbAt:0.8 NAME 'olcRootDN' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX OMsDN SINGLE-VALUE )", NULL, NULL },
	{ "rootDSE", "file", 2, 2, 0, ARG_MAGIC|CFG_ROOTDSE,
		&config_generic, "( OLcfgGlAt:51 NAME 'olcRootDSE' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "rootpw", "password", 2, 2, 0, ARG_BERVAL|ARG_DB|ARG_MAGIC,
		&config_rootpw, "( OLcfgDbAt:0.9 NAME 'olcRootPW' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "sasl-authz-policy", NULL, 2, 2, 0, ARG_MAGIC|CFG_AZPOLICY,
		&config_generic, NULL, NULL, NULL },
	{ "sasl-auxprops", NULL, 2, 0, 0,
#ifdef HAVE_CYRUS_SASL
		ARG_STRING|ARG_UNIQUE, &slap_sasl_auxprops,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:89 NAME 'olcSaslAuxprops' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "sasl-auxprops-dontusecopy", NULL, 2, 0, 0,
#if defined(HAVE_CYRUS_SASL) && defined(SLAP_AUXPROP_DONTUSECOPY)
		ARG_MAGIC|CFG_AZDUC, &config_generic,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:91 NAME 'olcSaslAuxpropsDontUseCopy' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "sasl-auxprops-dontusecopy-ignore", "true|FALSE", 2, 0, 0,
#if defined(HAVE_CYRUS_SASL) && defined(SLAP_AUXPROP_DONTUSECOPY)
		ARG_ON_OFF|CFG_AZDUC_IGNORE, &slap_dontUseCopy_ignore,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:92 NAME 'olcSaslAuxpropsDontUseCopyIgnore' "
			"EQUALITY booleanMatch "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "sasl-host", "host", 2, 2, 0,
#ifdef HAVE_CYRUS_SASL
		ARG_STRING|ARG_UNIQUE, &sasl_host,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:53 NAME 'olcSaslHost' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "sasl-realm", "realm", 2, 2, 0,
#ifdef HAVE_CYRUS_SASL
		ARG_STRING|ARG_UNIQUE, &global_realm,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:54 NAME 'olcSaslRealm' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "sasl-regexp", NULL, 3, 3, 0, ARG_MAGIC|CFG_AZREGEXP,
		&config_generic, NULL, NULL, NULL },
	{ "sasl-secprops", "properties", 2, 2, 0,
#ifdef HAVE_CYRUS_SASL
		ARG_MAGIC|CFG_SASLSECP, &config_generic,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:56 NAME 'olcSaslSecProps' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "saslRegexp",	NULL, 3, 3, 0, ARG_MAGIC|CFG_AZREGEXP,
		&config_generic, NULL, NULL, NULL },
	{ "schemadn", "dn", 2, 2, 0, ARG_MAY_DB|ARG_DN|ARG_QUOTE|ARG_MAGIC,
		&config_schema_dn, "( OLcfgGlAt:58 NAME 'olcSchemaDN' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX OMsDN SINGLE-VALUE )", NULL, NULL },
	{ "security", "factors", 2, 0, 0, ARG_MAY_DB|ARG_MAGIC,
		&config_security, "( OLcfgGlAt:59 NAME 'olcSecurity' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "serverID", "number> <[URI]", 2, 3, 0, ARG_MAGIC|CFG_SERVERID,
		&config_generic, "( OLcfgGlAt:81 NAME 'olcServerID' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "sizelimit", "limit",	2, 0, 0, ARG_MAY_DB|ARG_MAGIC,
		&config_sizelimit, "( OLcfgGlAt:60 NAME 'olcSizeLimit' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "sockbuf_max_incoming", "max", 2, 2, 0, ARG_BER_LEN_T,
		&sockbuf_max_incoming, "( OLcfgGlAt:61 NAME 'olcSockbufMaxIncoming' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "sockbuf_max_incoming_auth", "max", 2, 2, 0, ARG_BER_LEN_T,
		&sockbuf_max_incoming_auth, "( OLcfgGlAt:62 NAME 'olcSockbufMaxIncomingAuth' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "sortvals", "attr", 2, 0, 0, ARG_MAGIC|CFG_SORTVALS,
		&config_generic, "( OLcfgGlAt:83 NAME 'olcSortVals' "
			"DESC 'Attributes whose values will always be sorted' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "subordinate", "[advertise]", 1, 2, 0, ARG_DB|ARG_MAGIC,
		&config_subordinate, "( OLcfgDbAt:0.15 NAME 'olcSubordinate' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "suffix",	"suffix", 2, 2, 0, ARG_DB|ARG_DN|ARG_QUOTE|ARG_MAGIC,
		&config_suffix, "( OLcfgDbAt:0.10 NAME 'olcSuffix' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX OMsDN )", NULL, NULL },
	{ "sync_use_subentry", NULL, 0, 0, 0, ARG_ON_OFF|ARG_DB|ARG_MAGIC|CFG_SYNC_SUBENTRY,
		&config_generic, "( OLcfgDbAt:0.19 NAME 'olcSyncUseSubentry' "
			"DESC 'Store sync context in a subentry' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "syncrepl", NULL, 0, 0, 0, ARG_DB|ARG_MAGIC,
		&syncrepl_config, "( OLcfgDbAt:0.11 NAME 'olcSyncrepl' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString X-ORDERED 'VALUES' )", NULL, NULL },
	{ "tcp-buffer", "[listener=<listener>] [{read|write}=]size", 0, 0, 0,
#ifndef LDAP_TCP_BUFFER
		ARG_IGNORED, NULL,
#else /* LDAP_TCP_BUFFER */
		ARG_MAGIC, &config_tcp_buffer,
#endif /* LDAP_TCP_BUFFER */
			"( OLcfgGlAt:90 NAME 'olcTCPBuffer' "
			"DESC 'Custom TCP buffer size' "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "threads", "count", 2, 2, 0,
#ifdef NO_THREADS
		ARG_IGNORED, NULL,
#else
		ARG_INT|ARG_MAGIC|CFG_THREADS, &config_generic,
#endif
		"( OLcfgGlAt:66 NAME 'olcThreads' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "threadqueues", "count", 2, 2, 0,
#ifdef NO_THREADS
		ARG_IGNORED, NULL,
#else
		ARG_INT|ARG_MAGIC|CFG_THREADQS, &config_generic,
#endif
		"( OLcfgGlAt:95 NAME 'olcThreadQueues' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "timelimit", "limit", 2, 0, 0, ARG_MAY_DB|ARG_MAGIC,
		&config_timelimit, "( OLcfgGlAt:67 NAME 'olcTimeLimit' "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "TLSCACertificateFile", NULL, 2, 2, 0,
#ifdef HAVE_TLS
		CFG_TLS_CA_FILE|ARG_STRING|ARG_MAGIC, &config_tls_option,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:68 NAME 'olcTLSCACertificateFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "TLSCACertificatePath", NULL,	2, 2, 0,
#ifdef HAVE_TLS
		CFG_TLS_CA_PATH|ARG_STRING|ARG_MAGIC, &config_tls_option,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:69 NAME 'olcTLSCACertificatePath' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "TLSCertificateFile", NULL, 2, 2, 0,
#ifdef HAVE_TLS
		CFG_TLS_CERT_FILE|ARG_STRING|ARG_MAGIC, &config_tls_option,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:70 NAME 'olcTLSCertificateFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "TLSCertificateKeyFile", NULL, 2, 2, 0,
#ifdef HAVE_TLS
		CFG_TLS_CERT_KEY|ARG_STRING|ARG_MAGIC, &config_tls_option,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:71 NAME 'olcTLSCertificateKeyFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "TLSCipherSuite",	NULL, 2, 2, 0,
#ifdef HAVE_TLS
		CFG_TLS_CIPHER|ARG_STRING|ARG_MAGIC, &config_tls_option,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:72 NAME 'olcTLSCipherSuite' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "TLSCRLCheck", NULL, 2, 2, 0,
#if defined(HAVE_TLS) && defined(HAVE_OPENSSL_CRL)
		CFG_TLS_CRLCHECK|ARG_STRING|ARG_MAGIC, &config_tls_config,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:73 NAME 'olcTLSCRLCheck' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "TLSCRLFile", NULL, 2, 2, 0,
#if defined(HAVE_GNUTLS)
		CFG_TLS_CRL_FILE|ARG_STRING|ARG_MAGIC, &config_tls_option,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:82 NAME 'olcTLSCRLFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "TLSRandFile", NULL, 2, 2, 0,
#ifdef HAVE_TLS
		CFG_TLS_RAND|ARG_STRING|ARG_MAGIC, &config_tls_option,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:74 NAME 'olcTLSRandFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "TLSVerifyClient", NULL, 2, 2, 0,
#ifdef HAVE_TLS
		CFG_TLS_VERIFY|ARG_STRING|ARG_MAGIC, &config_tls_config,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:75 NAME 'olcTLSVerifyClient' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "TLSDHParamFile", NULL, 2, 2, 0,
#ifdef HAVE_TLS
		CFG_TLS_DH_FILE|ARG_STRING|ARG_MAGIC, &config_tls_option,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:77 NAME 'olcTLSDHParamFile' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "TLSECName", NULL, 2, 2, 0,
#ifdef HAVE_TLS
		CFG_TLS_ECNAME|ARG_STRING|ARG_MAGIC, &config_tls_option,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:96 NAME 'olcTLSECName' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "TLSProtocolMin",	NULL, 2, 2, 0,
#ifdef HAVE_TLS
		CFG_TLS_PROTOCOL_MIN|ARG_STRING|ARG_MAGIC, &config_tls_config,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgGlAt:87 NAME 'olcTLSProtocolMin' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "tool-threads", "count", 2, 2, 0, ARG_INT|ARG_MAGIC|CFG_TTHREADS,
		&config_generic, "( OLcfgGlAt:80 NAME 'olcToolThreads' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "ucdata-path", "path", 2, 2, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL },
	{ "updatedn", "dn", 2, 2, 0, ARG_DB|ARG_DN|ARG_QUOTE|ARG_MAGIC,
		&config_updatedn, "( OLcfgDbAt:0.12 NAME 'olcUpdateDN' "
			"SYNTAX OMsDN SINGLE-VALUE )", NULL, NULL },
	{ "updateref", "url", 2, 2, 0, ARG_DB|ARG_MAGIC,
		&config_updateref, "( OLcfgDbAt:0.13 NAME 'olcUpdateRef' "
			"EQUALITY caseIgnoreMatch "
			"SUP labeledURI )", NULL, NULL },
	{ "writetimeout", "timeout", 2, 2, 0, ARG_INT,
		&global_writetimeout, "( OLcfgGlAt:88 NAME 'olcWriteTimeout' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ NULL,	NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

/* Need to no-op this keyword for dynamic config */
ConfigTable olcDatabaseDummy[] = {
	{ "", "", 0, 0, 0, ARG_IGNORED,
		NULL, "( OLcfgGlAt:13 NAME 'olcDatabase' "
			"DESC 'The backend type for a database instance' "
			"SUP olcBackend SINGLE-VALUE X-ORDERED 'SIBLINGS' )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

/* Routines to check if a child can be added to this type */
static ConfigLDAPadd cfAddSchema, cfAddInclude, cfAddDatabase,
	cfAddBackend, cfAddModule, cfAddOverlay;

/* NOTE: be careful when defining array members
 * that can be conditionally compiled */
#define CFOC_GLOBAL	cf_ocs[1]
#define CFOC_SCHEMA	cf_ocs[2]
#define CFOC_BACKEND	cf_ocs[3]
#define CFOC_DATABASE	cf_ocs[4]
#define CFOC_OVERLAY	cf_ocs[5]
#define CFOC_INCLUDE	cf_ocs[6]
#define CFOC_FRONTEND	cf_ocs[7]
#ifdef SLAPD_MODULES
#define CFOC_MODULE	cf_ocs[8]
#endif /* SLAPD_MODULES */

static ConfigOCs cf_ocs[] = {
	{ "( OLcfgGlOc:0 "
		"NAME 'olcConfig' "
		"DESC 'OpenLDAP configuration object' "
		"ABSTRACT SUP top )", Cft_Abstract, NULL },
	{ "( OLcfgGlOc:1 "
		"NAME 'olcGlobal' "
		"DESC 'OpenLDAP Global configuration options' "
		"SUP olcConfig STRUCTURAL "
		"MAY ( cn $ olcConfigFile $ olcConfigDir $ olcAllows $ olcArgsFile $ "
		 "olcAttributeOptions $ olcAuthIDRewrite $ "
		 "olcAuthzPolicy $ olcAuthzRegexp $ olcConcurrency $ "
		 "olcConnMaxPending $ olcConnMaxPendingAuth $ "
		 "olcDisallows $ olcGentleHUP $ olcIdleTimeout $ "
		 "olcIndexSubstrIfMaxLen $ olcIndexSubstrIfMinLen $ "
		 "olcIndexSubstrAnyLen $ olcIndexSubstrAnyStep $ olcIndexHash64 $ "
		 "olcIndexIntLen $ "
		 "olcListenerThreads $ olcLocalSSF $ olcLogFile $ olcLogLevel $ "
		 "olcPasswordCryptSaltFormat $ olcPasswordHash $ olcPidFile $ "
		 "olcPluginLogFile $ olcReadOnly $ olcReferral $ "
		 "olcReplogFile $ olcRequires $ olcRestrict $ olcReverseLookup $ "
		 "olcRootDSE $ "
		 "olcSaslAuxprops $ olcSaslAuxpropsDontUseCopy $ olcSaslAuxpropsDontUseCopyIgnore $ "
		 "olcSaslHost $ olcSaslRealm $ olcSaslSecProps $ "
		 "olcSecurity $ olcServerID $ olcSizeLimit $ "
		 "olcSockbufMaxIncoming $ olcSockbufMaxIncomingAuth $ "
		 "olcTCPBuffer $ "
		 "olcThreads $ olcThreadQueues $ "
		 "olcTimeLimit $ olcTLSCACertificateFile $ "
		 "olcTLSCACertificatePath $ olcTLSCertificateFile $ "
		 "olcTLSCertificateKeyFile $ olcTLSCipherSuite $ olcTLSCRLCheck $ "
		 "olcTLSRandFile $ olcTLSVerifyClient $ olcTLSDHParamFile $ olcTLSECName $ "
		 "olcTLSCRLFile $ olcTLSProtocolMin $ olcToolThreads $ olcWriteTimeout $ "
		 "olcObjectIdentifier $ olcAttributeTypes $ olcObjectClasses $ "
		 "olcDitContentRules $ olcLdapSyntaxes ) )", Cft_Global },
	{ "( OLcfgGlOc:2 "
		"NAME 'olcSchemaConfig' "
		"DESC 'OpenLDAP schema object' "
		"SUP olcConfig STRUCTURAL "
		"MAY ( cn $ olcObjectIdentifier $ olcLdapSyntaxes $ "
		 "olcAttributeTypes $ olcObjectClasses $ olcDitContentRules ) )",
		 	Cft_Schema, NULL, cfAddSchema },
	{ "( OLcfgGlOc:3 "
		"NAME 'olcBackendConfig' "
		"DESC 'OpenLDAP Backend-specific options' "
		"SUP olcConfig STRUCTURAL "
		"MUST olcBackend )", Cft_Backend, NULL, cfAddBackend },
	{ "( OLcfgGlOc:4 "
		"NAME 'olcDatabaseConfig' "
		"DESC 'OpenLDAP Database-specific options' "
		"SUP olcConfig STRUCTURAL "
		"MUST olcDatabase "
		"MAY ( olcDisabled $ olcHidden $ olcSuffix $ olcSubordinate $ olcAccess $ "
		 "olcAddContentAcl $ olcLastMod $ olcLimits $ "
		 "olcMaxDerefDepth $ olcPlugin $ olcReadOnly $ olcReplica $ "
		 "olcReplicaArgsFile $ olcReplicaPidFile $ olcReplicationInterval $ "
		 "olcReplogFile $ olcRequires $ olcRestrict $ olcRootDN $ olcRootPW $ "
		 "olcSchemaDN $ olcSecurity $ olcSizeLimit $ olcSyncUseSubentry $ olcSyncrepl $ "
		 "olcTimeLimit $ olcUpdateDN $ olcUpdateRef $ olcMirrorMode $ "
		 "olcMonitoring $ olcExtraAttrs ) )",
		 	Cft_Database, NULL, cfAddDatabase },
	{ "( OLcfgGlOc:5 "
		"NAME 'olcOverlayConfig' "
		"DESC 'OpenLDAP Overlay-specific options' "
		"SUP olcConfig STRUCTURAL "
		"MUST olcOverlay "
		"MAY olcDisabled )", Cft_Overlay, NULL, cfAddOverlay },
	{ "( OLcfgGlOc:6 "
		"NAME 'olcIncludeFile' "
		"DESC 'OpenLDAP configuration include file' "
		"SUP olcConfig STRUCTURAL "
		"MUST olcInclude "
		"MAY ( cn $ olcRootDSE ) )",
		/* Used to be Cft_Include, that def has been removed */
		Cft_Abstract, NULL, cfAddInclude },
	/* This should be STRUCTURAL like all the other database classes, but
	 * that would mean inheriting all of the olcDatabaseConfig attributes,
	 * which causes them to be merged twice in config_build_entry.
	 */
	{ "( OLcfgGlOc:7 "
		"NAME 'olcFrontendConfig' "
		"DESC 'OpenLDAP frontend configuration' "
		"AUXILIARY "
		"MAY ( olcDefaultSearchBase $ olcPasswordHash $ olcSortVals ) )",
		Cft_Database, NULL, NULL },
#ifdef SLAPD_MODULES
	{ "( OLcfgGlOc:8 "
		"NAME 'olcModuleList' "
		"DESC 'OpenLDAP dynamic module info' "
		"SUP olcConfig STRUCTURAL "
		"MAY ( cn $ olcModulePath $ olcModuleLoad ) )",
		Cft_Module, NULL, cfAddModule },
#endif
	{ NULL, 0, NULL }
};

typedef struct ServerID {
	struct ServerID *si_next;
	struct berval si_url;
	int si_num;
} ServerID;

static ServerID *sid_list;
static ServerID *sid_set;

typedef struct voidList {
	struct voidList *vl_next;
	void *vl_ptr;
} voidList;

typedef struct ADlist {
	struct ADlist *al_next;
	AttributeDescription *al_desc;
} ADlist;

static ADlist *sortVals;

static int
config_generic(ConfigArgs *c) {
	int i;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		int rc = 0;
		switch(c->type) {
		case CFG_CONCUR:
			c->value_int = ldap_pvt_thread_get_concurrency();
			break;
		case CFG_THREADS:
			c->value_int = connection_pool_max;
			break;
		case CFG_THREADQS:
			c->value_int = connection_pool_queues;
			break;
		case CFG_TTHREADS:
			c->value_int = slap_tool_thread_max;
			break;
		case CFG_LTHREADS:
			c->value_uint = slapd_daemon_threads;
			break;
		case CFG_SALT:
			if ( passwd_salt )
				c->value_string = ch_strdup( passwd_salt );
			else
				rc = 1;
			break;
		case CFG_LIMITS:
			if ( c->be->be_limits ) {
				char buf[4096*3];
				struct berval bv;

				for ( i=0; c->be->be_limits[i]; i++ ) {
					bv.bv_len = snprintf( buf, sizeof( buf ), SLAP_X_ORDERED_FMT, i );
					if ( bv.bv_len >= sizeof( buf ) ) {
						ber_bvarray_free_x( c->rvalue_vals, NULL );
						c->rvalue_vals = NULL;
						rc = 1;
						break;
					}
					bv.bv_val = buf + bv.bv_len;
					limits_unparse( c->be->be_limits[i], &bv,
							sizeof( buf ) - ( bv.bv_val - buf ) );
					bv.bv_len += bv.bv_val - buf;
					bv.bv_val = buf;
					value_add_one( &c->rvalue_vals, &bv );
				}
			}
			if ( !c->rvalue_vals ) rc = 1;
			break;
		case CFG_RO:
			c->value_int = (c->be->be_restrictops & SLAP_RESTRICT_READONLY);
			break;
		case CFG_AZPOLICY:
			c->value_string = ch_strdup( slap_sasl_getpolicy());
			break;
		case CFG_AZREGEXP:
			slap_sasl_regexp_unparse( &c->rvalue_vals );
			if ( !c->rvalue_vals ) rc = 1;
			break;
#ifdef HAVE_CYRUS_SASL
#ifdef SLAP_AUXPROP_DONTUSECOPY
		case CFG_AZDUC: {
			static int duc_done = 0;

			/* take the opportunity to initialize with known values */
			if ( !duc_done ) {
				struct berval duc[] = { BER_BVC("cmusaslsecretOTP"), BER_BVNULL };
				int i;
				
				for ( i = 0; !BER_BVISNULL( &duc[ i ] ); i++ ) {
					const char *text = NULL;
					AttributeDescription *ad = NULL;

					if ( slap_bv2ad( &duc[ i ], &ad, &text ) == LDAP_SUCCESS ) {
						int gotit = 0;
						if ( slap_dontUseCopy_propnames ) {
							int j;

							for ( j = 0; !BER_BVISNULL( &slap_dontUseCopy_propnames[ j ] ); j++ ) {
								if ( bvmatch( &slap_dontUseCopy_propnames[ j ], &ad->ad_cname ) ) {
									gotit = 1;
								}
							}
						}

						if ( !gotit ) {
							value_add_one( &slap_dontUseCopy_propnames, &ad->ad_cname );
						}
					}
				}

				duc_done = 1;
			}

			if ( slap_dontUseCopy_propnames != NULL ) {
				ber_bvarray_dup_x( &c->rvalue_vals, slap_dontUseCopy_propnames, NULL );
			} else {
				rc = 1;
			}
			} break;
#endif /* SLAP_AUXPROP_DONTUSECOPY */
		case CFG_SASLSECP: {
			struct berval bv = BER_BVNULL;
			slap_sasl_secprops_unparse( &bv );
			if ( !BER_BVISNULL( &bv )) {
				ber_bvarray_add( &c->rvalue_vals, &bv );
			} else {
				rc = 1;
			}
			}
			break;
#endif
		case CFG_DEPTH:
			c->value_int = c->be->be_max_deref_depth;
			break;
		case CFG_DISABLED:
			if ( c->bi ) {
				/* overlay */
				if ( c->bi->bi_flags & SLAPO_BFLAG_DISABLED ) {
					c->value_int = 1;
				} else {
					rc = 1;
				}
			} else {
				/* database */
				if ( SLAP_DBDISABLED( c->be )) {
					c->value_int = 1;
				} else {
					rc = 1;
				}
			}
			break;
		case CFG_HIDDEN:
			if ( SLAP_DBHIDDEN( c->be )) {
				c->value_int = 1;
			} else {
				rc = 1;
			}
			break;
		case CFG_OID: {
			ConfigFile *cf = c->ca_private;
			if ( !cf )
				oidm_unparse( &c->rvalue_vals, NULL, NULL, 1 );
			else if ( cf->c_om_head )
				oidm_unparse( &c->rvalue_vals, cf->c_om_head,
					cf->c_om_tail, 0 );
			if ( !c->rvalue_vals )
				rc = 1;
			}
			break;
		case CFG_ATOPT:
			ad_unparse_options( &c->rvalue_vals );
			break;
		case CFG_OC: {
			ConfigFile *cf = c->ca_private;
			if ( !cf )
				oc_unparse( &c->rvalue_vals, NULL, NULL, 1 );
			else if ( cf->c_oc_head )
				oc_unparse( &c->rvalue_vals, cf->c_oc_head,
					cf->c_oc_tail, 0 );
			if ( !c->rvalue_vals )
				rc = 1;
			}
			break;
		case CFG_ATTR: {
			ConfigFile *cf = c->ca_private;
			if ( !cf )
				at_unparse( &c->rvalue_vals, NULL, NULL, 1 );
			else if ( cf->c_at_head )
				at_unparse( &c->rvalue_vals, cf->c_at_head,
					cf->c_at_tail, 0 );
			if ( !c->rvalue_vals )
				rc = 1;
			}
			break;
		case CFG_SYNTAX: {
			ConfigFile *cf = c->ca_private;
			if ( !cf )
				syn_unparse( &c->rvalue_vals, NULL, NULL, 1 );
			else if ( cf->c_syn_head )
				syn_unparse( &c->rvalue_vals, cf->c_syn_head,
					cf->c_syn_tail, 0 );
			if ( !c->rvalue_vals )
				rc = 1;
			}
			break;
		case CFG_DIT: {
			ConfigFile *cf = c->ca_private;
			if ( !cf )
				cr_unparse( &c->rvalue_vals, NULL, NULL, 1 );
			else if ( cf->c_cr_head )
				cr_unparse( &c->rvalue_vals, cf->c_cr_head,
					cf->c_cr_tail, 0 );
			if ( !c->rvalue_vals )
				rc = 1;
			}
			break;
			
		case CFG_ACL: {
			AccessControl *a;
			char *src, *dst, ibuf[11];
			struct berval bv, abv;
			for (i=0, a=c->be->be_acl; a; i++,a=a->acl_next) {
				abv.bv_len = snprintf( ibuf, sizeof( ibuf ), SLAP_X_ORDERED_FMT, i );
				if ( abv.bv_len >= sizeof( ibuf ) ) {
					ber_bvarray_free_x( c->rvalue_vals, NULL );
					c->rvalue_vals = NULL;
					i = 0;
					break;
				}
				acl_unparse( a, &bv );
				abv.bv_val = ch_malloc( abv.bv_len + bv.bv_len + 1 );
				AC_MEMCPY( abv.bv_val, ibuf, abv.bv_len );
				/* Turn TAB / EOL into plain space */
				for (src=bv.bv_val,dst=abv.bv_val+abv.bv_len; *src; src++) {
					if (isspace((unsigned char)*src)) *dst++ = ' ';
					else *dst++ = *src;
				}
				*dst = '\0';
				if (dst[-1] == ' ') {
					dst--;
					*dst = '\0';
				}
				abv.bv_len = dst - abv.bv_val;
				ber_bvarray_add( &c->rvalue_vals, &abv );
			}
			rc = (!i);
			break;
		}
		case CFG_ACL_ADD:
			c->value_int = (SLAP_DBACL_ADD(c->be) != 0);
			break;
		case CFG_ROOTDSE: {
			ConfigFile *cf = c->ca_private;
			if ( cf->c_dseFiles ) {
				value_add( &c->rvalue_vals, cf->c_dseFiles );
			} else {
				rc = 1;
			}
			}
			break;
		case CFG_SERVERID:
			if ( sid_list ) {
				ServerID *si;
				struct berval bv;

				for ( si = sid_list; si; si=si->si_next ) {
					assert( si->si_num >= 0 && si->si_num <= SLAP_SYNC_SID_MAX );
					if ( !BER_BVISEMPTY( &si->si_url )) {
						bv.bv_len = si->si_url.bv_len + 6;
						bv.bv_val = ch_malloc( bv.bv_len );
						bv.bv_len = sprintf( bv.bv_val, "%d %s", si->si_num,
							si->si_url.bv_val );
						ber_bvarray_add( &c->rvalue_vals, &bv );
					} else {
						char buf[5];
						bv.bv_val = buf;
						bv.bv_len = sprintf( buf, "%d", si->si_num );
						value_add_one( &c->rvalue_vals, &bv );
					}
				}
			} else {
				rc = 1;
			}
			break;
		case CFG_LOGFILE:
			if ( logfileName )
				c->value_string = ch_strdup( logfileName );
			else
				rc = 1;
			break;
		case CFG_LASTMOD:
			c->value_int = (SLAP_NOLASTMOD(c->be) == 0);
			break;
		case CFG_SYNC_SUBENTRY:
			c->value_int = (SLAP_SYNC_SUBENTRY(c->be) != 0);
			break;
		case CFG_MIRRORMODE:
			if ( SLAP_SHADOW(c->be))
				c->value_int = (SLAP_MULTIMASTER(c->be) != 0);
			else
				rc = 1;
			break;
		case CFG_MONITORING:
			c->value_int = (SLAP_DBMONITORING(c->be) != 0);
			break;
		case CFG_SSTR_IF_MAX:
			c->value_uint = index_substr_if_maxlen;
			break;
		case CFG_SSTR_IF_MIN:
			c->value_uint = index_substr_if_minlen;
			break;
		case CFG_IX_HASH64:
			c->value_int = slap_hash64( -1 );
			break;
		case CFG_IX_INTLEN:
			c->value_int = index_intlen;
			break;
		case CFG_SORTVALS: {
			ADlist *sv;
			rc = 1;
			for ( sv = sortVals; sv; sv = sv->al_next ) {
				value_add_one( &c->rvalue_vals, &sv->al_desc->ad_cname );
				rc = 0;
			}
			} break;
#ifdef SLAPD_MODULES
		case CFG_MODLOAD: {
			ModPaths *mp = c->ca_private;
			if (mp->mp_loads) {
				int i;
				for (i=0; !BER_BVISNULL(&mp->mp_loads[i]); i++) {
					struct berval bv;
					bv.bv_val = c->log;
					bv.bv_len = snprintf( bv.bv_val, sizeof( c->log ),
						SLAP_X_ORDERED_FMT "%s", i,
						mp->mp_loads[i].bv_val );
					if ( bv.bv_len >= sizeof( c->log ) ) {
						ber_bvarray_free_x( c->rvalue_vals, NULL );
						c->rvalue_vals = NULL;
						break;
					}
					value_add_one( &c->rvalue_vals, &bv );
				}
			}

			rc = c->rvalue_vals ? 0 : 1;
			}
			break;
		case CFG_MODPATH: {
			ModPaths *mp = c->ca_private;
			if ( !BER_BVISNULL( &mp->mp_path ))
				value_add_one( &c->rvalue_vals, &mp->mp_path );

			rc = c->rvalue_vals ? 0 : 1;
			}
			break;
#endif
#ifdef LDAP_SLAPI
		case CFG_PLUGIN:
			slapi_int_plugin_unparse( c->be, &c->rvalue_vals );
			if ( !c->rvalue_vals ) rc = 1;
			break;
#endif
#ifdef SLAP_AUTH_REWRITE
		case CFG_REWRITE:
			if ( authz_rewrites ) {
				struct berval bv, idx;
				char ibuf[32];
				int i;

				idx.bv_val = ibuf;
				for ( i=0; !BER_BVISNULL( &authz_rewrites[i] ); i++ ) {
					idx.bv_len = snprintf( idx.bv_val, sizeof( ibuf ), SLAP_X_ORDERED_FMT, i );
					if ( idx.bv_len >= sizeof( ibuf ) ) {
						ber_bvarray_free_x( c->rvalue_vals, NULL );
						c->rvalue_vals = NULL;
						break;
					}
					bv.bv_len = idx.bv_len + authz_rewrites[i].bv_len;
					bv.bv_val = ch_malloc( bv.bv_len + 1 );
					AC_MEMCPY( bv.bv_val, idx.bv_val, idx.bv_len );
					AC_MEMCPY( &bv.bv_val[ idx.bv_len ],
						authz_rewrites[i].bv_val,
						authz_rewrites[i].bv_len + 1 );
					ber_bvarray_add( &c->rvalue_vals, &bv );
				}
			}
			if ( !c->rvalue_vals ) rc = 1;
			break;
#endif
		default:
			rc = 1;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		int rc = 0;
		switch(c->type) {
		/* single-valued attrs, no-ops */
		case CFG_CONCUR:
		case CFG_THREADS:
		case CFG_THREADQS:
		case CFG_TTHREADS:
		case CFG_LTHREADS:
		case CFG_RO:
		case CFG_AZPOLICY:
		case CFG_DEPTH:
		case CFG_LASTMOD:
		case CFG_MONITORING:
		case CFG_SASLSECP:
		case CFG_SSTR_IF_MAX:
		case CFG_SSTR_IF_MIN:
		case CFG_ACL_ADD:
		case CFG_SYNC_SUBENTRY:
			break;

		/* no-ops, requires slapd restart */
		case CFG_PLUGIN:
		case CFG_MODLOAD:
		case CFG_AZREGEXP:
		case CFG_REWRITE:
			snprintf(c->log, sizeof( c->log ), "change requires slapd restart");
			break;

		case CFG_MIRRORMODE:
			SLAP_DBFLAGS(c->be) &= ~SLAP_DBFLAG_MULTI_SHADOW;
			if(SLAP_SHADOW(c->be))
				SLAP_DBFLAGS(c->be) |= SLAP_DBFLAG_SINGLE_SHADOW;
			break;

#if defined(HAVE_CYRUS_SASL) && defined(SLAP_AUXPROP_DONTUSECOPY)
		case CFG_AZDUC:
			if ( c->valx < 0 ) {
				if ( slap_dontUseCopy_propnames != NULL ) {
					ber_bvarray_free( slap_dontUseCopy_propnames );
					slap_dontUseCopy_propnames = NULL;
				}
	
			} else {
				int i;

				if ( slap_dontUseCopy_propnames == NULL ) {
					rc = 1;
					break;
				}

				for ( i = 0; !BER_BVISNULL( &slap_dontUseCopy_propnames[ i ] ) && i < c->valx; i++ );
				if ( i < c->valx ) {
					rc = 1;
					break;
				}
				ber_memfree( slap_dontUseCopy_propnames[ i ].bv_val );
				for ( ; !BER_BVISNULL( &slap_dontUseCopy_propnames[ i + 1 ] ); i++ ) {
					slap_dontUseCopy_propnames[ i ] = slap_dontUseCopy_propnames[ i + 1 ];
				}
				BER_BVZERO( &slap_dontUseCopy_propnames[ i ] );
			}
			break;
#endif /* SLAP_AUXPROP_DONTUSECOPY */
		case CFG_SALT:
			ch_free( passwd_salt );
			passwd_salt = NULL;
			break;

		case CFG_LOGFILE:
			ch_free( logfileName );
			logfileName = NULL;
			if ( logfile ) {
				fclose( logfile );
				logfile = NULL;
			}
			break;

		case CFG_SERVERID: {
			ServerID *si, **sip;

			for ( i=0, si = sid_list, sip = &sid_list;
				si; si = *sip, i++ ) {
				if ( c->valx == -1 || i == c->valx ) {
					*sip = si->si_next;
					if ( sid_set == si )
						sid_set = NULL;
					ch_free( si );
					if ( c->valx >= 0 )
						break;
				} else {
					sip = &si->si_next;
				}
			}
			}
			break;
		case CFG_HIDDEN:
			c->be->be_flags &= ~SLAP_DBFLAG_HIDDEN;
			break;

		case CFG_DISABLED:
			if ( c->bi ) {
				c->bi->bi_flags &= ~SLAP_DBFLAG_DISABLED;
				if ( c->bi->bi_db_open ) {
					BackendInfo *bi_orig = c->be->bd_info;
					c->be->bd_info = c->bi;
					rc = c->bi->bi_db_open( c->be, &c->reply );
					c->be->bd_info = bi_orig;
				}
			} else {
				c->be->be_flags &= ~SLAP_DBFLAG_DISABLED;
				rc = backend_startup_one( c->be, &c->reply );
			}
			break;

		case CFG_IX_HASH64:
			slap_hash64( 0 );
			break;

		case CFG_IX_INTLEN:
			index_intlen = SLAP_INDEX_INTLEN_DEFAULT;
			index_intlen_strlen = SLAP_INDEX_INTLEN_STRLEN(
				SLAP_INDEX_INTLEN_DEFAULT );
			break;

		case CFG_ACL:
			if ( c->valx < 0 ) {
				acl_destroy( c->be->be_acl );
				c->be->be_acl = NULL;

			} else {
				AccessControl **prev, *a;
				int i;
				for (i=0, prev = &c->be->be_acl; i < c->valx;
					i++ ) {
					a = *prev;
					prev = &a->acl_next;
				}
				a = *prev;
				*prev = a->acl_next;
				acl_free( a );
			}
			if ( SLAP_CONFIG( c->be ) && !c->be->be_acl ) {
				Debug( LDAP_DEBUG_CONFIG, "config_generic (CFG_ACL): "
						"Last explicit ACL for back-config removed. "
						"Using hardcoded default\n", 0, 0, 0 );
				c->be->be_acl = defacl_parsed;
			}
			break;

		case CFG_OC: {
			CfEntryInfo *ce;
			/* Can be NULL when undoing a failed add */
			if ( c->ca_entry ) {
				ce = c->ca_entry->e_private;
				/* can't modify the hardcoded schema */
				if ( ce->ce_parent->ce_type == Cft_Global )
					return 1;
				}
			}
			cfn = c->ca_private;
			if ( c->valx < 0 ) {
				ObjectClass *oc;

				for( oc = cfn->c_oc_head; oc; oc_next( &oc )) {
					oc_delete( oc );
					if ( oc  == cfn->c_oc_tail )
						break;
				}
				cfn->c_oc_head = cfn->c_oc_tail = NULL;
			} else {
				ObjectClass *oc, *prev = NULL;

				for ( i=0, oc=cfn->c_oc_head; i<c->valx; i++) {
					prev = oc;
					oc_next( &oc );
				}
				oc_delete( oc );
				if ( cfn->c_oc_tail == oc ) {
					cfn->c_oc_tail = prev;
				}
				if ( cfn->c_oc_head == oc ) {
					oc_next( &oc );
					cfn->c_oc_head = oc;
				}
			}
			break;

		case CFG_ATTR: {
			CfEntryInfo *ce;
			/* Can be NULL when undoing a failed add */
			if ( c->ca_entry ) {
				ce = c->ca_entry->e_private;
				/* can't modify the hardcoded schema */
				if ( ce->ce_parent->ce_type == Cft_Global )
					return 1;
				}
			}
			cfn = c->ca_private;
			if ( c->valx < 0 ) {
				AttributeType *at;

				for( at = cfn->c_at_head; at; at_next( &at )) {
					at_delete( at );
					if ( at  == cfn->c_at_tail )
						break;
				}
				cfn->c_at_head = cfn->c_at_tail = NULL;
			} else {
				AttributeType *at, *prev = NULL;

				for ( i=0, at=cfn->c_at_head; i<c->valx; i++) {
					prev = at;
					at_next( &at );
				}
				at_delete( at );
				if ( cfn->c_at_tail == at ) {
					cfn->c_at_tail = prev;
				}
				if ( cfn->c_at_head == at ) {
					at_next( &at );
					cfn->c_at_head = at;
				}
			}
			break;

		case CFG_SYNTAX: {
			CfEntryInfo *ce;
			/* Can be NULL when undoing a failed add */
			if ( c->ca_entry ) {
				ce = c->ca_entry->e_private;
				/* can't modify the hardcoded schema */
				if ( ce->ce_parent->ce_type == Cft_Global )
					return 1;
				}
			}
			cfn = c->ca_private;
			if ( c->valx < 0 ) {
				Syntax *syn;

				for( syn = cfn->c_syn_head; syn; syn_next( &syn )) {
					syn_delete( syn );
					if ( syn == cfn->c_syn_tail )
						break;
				}
				cfn->c_syn_head = cfn->c_syn_tail = NULL;
			} else {
				Syntax *syn, *prev = NULL;

				for ( i = 0, syn = cfn->c_syn_head; i < c->valx; i++) {
					prev = syn;
					syn_next( &syn );
				}
				syn_delete( syn );
				if ( cfn->c_syn_tail == syn ) {
					cfn->c_syn_tail = prev;
				}
				if ( cfn->c_syn_head == syn ) {
					syn_next( &syn );
					cfn->c_syn_head = syn;
				}
			}
			break;
		case CFG_SORTVALS:
			if ( c->valx < 0 ) {
				ADlist *sv;
				for ( sv = sortVals; sv; sv = sortVals ) {
					sortVals = sv->al_next;
					sv->al_desc->ad_type->sat_flags &= ~SLAP_AT_SORTED_VAL;
					ch_free( sv );
				}
			} else {
				ADlist *sv, **prev;
				int i = 0;

				for ( prev = &sortVals, sv = sortVals; i < c->valx; i++ ) {
					prev = &sv->al_next;
					sv = sv->al_next;
				}
				sv->al_desc->ad_type->sat_flags &= ~SLAP_AT_SORTED_VAL;
				*prev = sv->al_next;
				ch_free( sv );
			}
			break;

		case CFG_LIMITS:
			/* FIXME: there is no limits_free function */
			if ( c->valx < 0 ) {
				limits_destroy( c->be->be_limits );
				c->be->be_limits = NULL;

			} else {
				int cnt, num = -1;

				if ( c->be->be_limits ) {
					for ( num = 0; c->be->be_limits[ num ]; num++ )
						/* just count */ ;
				}

				if ( c->valx >= num ) {
					return 1;
				}

				if ( num == 1 ) {
					limits_destroy( c->be->be_limits );
					c->be->be_limits = NULL;

				} else {
					limits_free_one( c->be->be_limits[ c->valx ] );

					for ( cnt = c->valx; cnt < num; cnt++ ) {
						c->be->be_limits[ cnt ] = c->be->be_limits[ cnt + 1 ];
					}
				}
			}
			break;

		case CFG_ATOPT:
			/* FIXME: there is no ad_option_free function */
		case CFG_ROOTDSE:
			/* FIXME: there is no way to remove attributes added by
				a DSE file */
		case CFG_OID:
		case CFG_DIT:
		case CFG_MODPATH:
		default:
			rc = 1;
			break;
		}
		return rc;
	}

	switch(c->type) {
		case CFG_BACKEND:
			if(!(c->bi = backend_info(c->argv[1]))) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> failed init", c->argv[0] );
				Debug(LDAP_DEBUG_ANY, "%s: %s (%s)!\n",
					c->log, c->cr_msg, c->argv[1] );
				return(1);
			}
			break;

		case CFG_DATABASE:
			c->bi = NULL;
			/* NOTE: config is always the first backend!
			 */
			if ( !strcasecmp( c->argv[1], "config" )) {
				c->be = LDAP_STAILQ_FIRST(&backendDB);
			} else if ( !strcasecmp( c->argv[1], "frontend" )) {
				c->be = frontendDB;
			} else {
				c->be = backend_db_init(c->argv[1], NULL, c->valx, &c->reply);
				if ( !c->be ) {
					if ( c->cr_msg[0] == 0 )
						snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> failed init", c->argv[0] );
					Debug(LDAP_DEBUG_ANY, "%s: %s (%s)\n", c->log, c->cr_msg, c->argv[1] );
					return(1);
				}
			}
			break;

		case CFG_CONCUR:
			ldap_pvt_thread_set_concurrency(c->value_int);
			break;

		case CFG_THREADS:
			if ( c->value_int < 2 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"threads=%d smaller than minimum value 2",
					c->value_int );
				Debug(LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
				return 1;

			} else if ( c->value_int > 2 * SLAP_MAX_WORKER_THREADS ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"warning, threads=%d larger than twice the default (2*%d=%d); YMMV",
					c->value_int, SLAP_MAX_WORKER_THREADS, 2 * SLAP_MAX_WORKER_THREADS );
				Debug(LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
			}
			if ( slapMode & SLAP_SERVER_MODE )
				ldap_pvt_thread_pool_maxthreads(&connection_pool, c->value_int);
			connection_pool_max = c->value_int;	/* save for reference */
			break;

		case CFG_THREADQS:
			if ( c->value_int < 1 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"threadqueuess=%d smaller than minimum value 1",
					c->value_int );
				Debug(LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
				return 1;
			}
			if ( slapMode & SLAP_SERVER_MODE )
				ldap_pvt_thread_pool_queues(&connection_pool, c->value_int);
			connection_pool_queues = c->value_int;	/* save for reference */
			break;

		case CFG_TTHREADS:
			if ( slapMode & SLAP_TOOL_MODE )
				ldap_pvt_thread_pool_maxthreads(&connection_pool, c->value_int);
			slap_tool_thread_max = c->value_int;	/* save for reference */
			break;

		case CFG_LTHREADS:
			{ int mask = 0;
			/* use a power of two */
			while (c->value_uint > 1) {
				c->value_uint >>= 1;
				mask <<= 1;
				mask |= 1;
			}
			slapd_daemon_mask = mask;
			slapd_daemon_threads = mask+1;
			}
			break;

		case CFG_SALT:
			if ( passwd_salt ) ch_free( passwd_salt );
			passwd_salt = c->value_string;
			lutil_salt_format(passwd_salt);
			break;

		case CFG_LIMITS:
			if(limits_parse(c->be, c->fname, c->lineno, c->argc, c->argv))
				return(1);
			break;

		case CFG_RO:
			if(c->value_int)
				c->be->be_restrictops |= SLAP_RESTRICT_READONLY;
			else
				c->be->be_restrictops &= ~SLAP_RESTRICT_READONLY;
			break;

		case CFG_AZPOLICY:
			ch_free(c->value_string);
			if (slap_sasl_setpolicy( c->argv[1] )) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unable to parse value", c->argv[0] );
				Debug(LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
					c->log, c->cr_msg, c->argv[1] );
				return(1);
			}
			break;
		
		case CFG_AZREGEXP:
			if (slap_sasl_regexp_config( c->argv[1], c->argv[2] ))
				return(1);
			break;
				
#ifdef HAVE_CYRUS_SASL
#ifdef SLAP_AUXPROP_DONTUSECOPY
		case CFG_AZDUC: {
			int arg, cnt;

			for ( arg = 1; arg < c->argc; arg++ ) {
				int duplicate = 0, err;
				AttributeDescription *ad = NULL;
				const char *text = NULL;

				err = slap_str2ad( c->argv[ arg ], &ad, &text );
				if ( err != LDAP_SUCCESS ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s>: attr #%d (\"%s\") unknown (err=%d \"%s\"; ignored)",
						c->argv[0], arg, c->argv[ arg ], err, text );
					Debug(LDAP_DEBUG_ANY, "%s: %s\n",
						c->log, c->cr_msg, 0 );

				} else {
					if ( slap_dontUseCopy_propnames != NULL ) {
						for ( cnt = 0; !BER_BVISNULL( &slap_dontUseCopy_propnames[ cnt ] ); cnt++ ) {
							if ( bvmatch( &slap_dontUseCopy_propnames[ cnt ], &ad->ad_cname ) ) {
								duplicate = 1;
								break;
							}
						}
					}

					if ( duplicate ) {
						snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s>: attr #%d (\"%s\") already defined (ignored)",
							c->argv[0], arg, ad->ad_cname.bv_val);
						Debug(LDAP_DEBUG_ANY, "%s: %s\n",
							c->log, c->cr_msg, 0 );
						continue;
					}

					value_add_one( &slap_dontUseCopy_propnames, &ad->ad_cname );
				}
			}
			
			} break;
#endif /* SLAP_AUXPROP_DONTUSECOPY */

		case CFG_SASLSECP:
			{
			char *txt = slap_sasl_secprops( c->argv[1] );
			if ( txt ) {
				snprintf( c->cr_msg, sizeof(c->cr_msg), "<%s> %s",
					c->argv[0], txt );
				Debug(LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg, 0 );
				return(1);
			}
			break;
			}
#endif

		case CFG_DEPTH:
			c->be->be_max_deref_depth = c->value_int;
			break;

		case CFG_OID: {
			OidMacro *om;

			if ( c->op == LDAP_MOD_ADD && c->ca_private && cfn != c->ca_private )
				cfn = c->ca_private;
			if(parse_oidm(c, 1, &om))
				return(1);
			if (!cfn->c_om_head) cfn->c_om_head = om;
			cfn->c_om_tail = om;
			}
			break;

		case CFG_OC: {
			ObjectClass *oc, *prev;

			if ( c->op == LDAP_MOD_ADD && c->ca_private && cfn != c->ca_private )
				cfn = c->ca_private;
			if ( c->valx < 0 ) {
				prev = cfn->c_oc_tail;
			} else {
				prev = NULL;
				/* If adding anything after the first, prev is easy */
				if ( c->valx ) {
					int i;
					for (i=0, oc = cfn->c_oc_head; i<c->valx; i++) {
						prev = oc;
						if ( !oc_next( &oc ))
							break;
					}
				} else
				/* If adding the first, and head exists, find its prev */
					if (cfn->c_oc_head) {
					for ( oc_start( &oc ); oc != cfn->c_oc_head; ) {
						prev = oc;
						oc_next( &oc );
					}
				}
				/* else prev is NULL, append to end of global list */
			}
			if(parse_oc(c, &oc, prev)) return(1);
			if (!cfn->c_oc_head || !c->valx) cfn->c_oc_head = oc;
			if (cfn->c_oc_tail == prev) cfn->c_oc_tail = oc;
			}
			break;

		case CFG_ATTR: {
			AttributeType *at, *prev;

			if ( c->op == LDAP_MOD_ADD && c->ca_private && cfn != c->ca_private )
				cfn = c->ca_private;
			if ( c->valx < 0 ) {
				prev = cfn->c_at_tail;
			} else {
				prev = NULL;
				/* If adding anything after the first, prev is easy */
				if ( c->valx ) {
					int i;
					for (i=0, at = cfn->c_at_head; i<c->valx; i++) {
						prev = at;
						if ( !at_next( &at ))
							break;
					}
				} else
				/* If adding the first, and head exists, find its prev */
					if (cfn->c_at_head) {
					for ( at_start( &at ); at != cfn->c_at_head; ) {
						prev = at;
						at_next( &at );
					}
				}
				/* else prev is NULL, append to end of global list */
			}
			if(parse_at(c, &at, prev)) return(1);
			if (!cfn->c_at_head || !c->valx) cfn->c_at_head = at;
			if (cfn->c_at_tail == prev) cfn->c_at_tail = at;
			}
			break;

		case CFG_SYNTAX: {
			Syntax *syn, *prev;

			if ( c->op == LDAP_MOD_ADD && c->ca_private && cfn != c->ca_private )
				cfn = c->ca_private;
			if ( c->valx < 0 ) {
				prev = cfn->c_syn_tail;
			} else {
				prev = NULL;
				/* If adding anything after the first, prev is easy */
				if ( c->valx ) {
					int i;
					for ( i = 0, syn = cfn->c_syn_head; i < c->valx; i++ ) {
						prev = syn;
						if ( !syn_next( &syn ))
							break;
					}
				} else
				/* If adding the first, and head exists, find its prev */
					if (cfn->c_syn_head) {
					for ( syn_start( &syn ); syn != cfn->c_syn_head; ) {
						prev = syn;
						syn_next( &syn );
					}
				}
				/* else prev is NULL, append to end of global list */
			}
			if ( parse_syn( c, &syn, prev ) ) return(1);
			if ( !cfn->c_syn_head || !c->valx ) cfn->c_syn_head = syn;
			if ( cfn->c_syn_tail == prev ) cfn->c_syn_tail = syn;
			}
			break;

		case CFG_DIT: {
			ContentRule *cr;

			if ( c->op == LDAP_MOD_ADD && c->ca_private && cfn != c->ca_private )
				cfn = c->ca_private;
			if(parse_cr(c, &cr)) return(1);
			if (!cfn->c_cr_head) cfn->c_cr_head = cr;
			cfn->c_cr_tail = cr;
			}
			break;

		case CFG_ATOPT:
			ad_define_option(NULL, NULL, 0);
			for(i = 1; i < c->argc; i++)
				if(ad_define_option(c->argv[i], c->fname, c->lineno))
					return(1);
			break;

		case CFG_IX_HASH64:
			if ( slap_hash64( c->value_int != 0 ))
				return 1;
			break;

		case CFG_IX_INTLEN:
			if ( c->value_int < SLAP_INDEX_INTLEN_DEFAULT )
				c->value_int = SLAP_INDEX_INTLEN_DEFAULT;
			else if ( c->value_int > 255 )
				c->value_int = 255;
			index_intlen = c->value_int;
			index_intlen_strlen = SLAP_INDEX_INTLEN_STRLEN(
				index_intlen );
			break;

		case CFG_SORTVALS: {
			ADlist *svnew = NULL, *svtail, *sv;

			for ( i = 1; i < c->argc; i++ ) {
				AttributeDescription *ad = NULL;
				const char *text;
				int rc;

				rc = slap_str2ad( c->argv[i], &ad, &text );
				if ( rc ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unknown attribute type #%d",
						c->argv[0], i );
sortval_reject:
					Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
						c->log, c->cr_msg, c->argv[i] );
					for ( sv = svnew; sv; sv = svnew ) {
						svnew = sv->al_next;
						ch_free( sv );
					}
					return 1;
				}
				if (( ad->ad_type->sat_flags & SLAP_AT_ORDERED ) ||
					ad->ad_type->sat_single_value ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> inappropriate attribute type #%d",
						c->argv[0], i );
					goto sortval_reject;
				}
				sv = ch_malloc( sizeof( ADlist ));
				sv->al_desc = ad;
				if ( !svnew ) {
					svnew = sv;
				} else {
					svtail->al_next = sv;
				}
				svtail = sv;
			}
			sv->al_next = NULL;
			for ( sv = svnew; sv; sv = sv->al_next )
				sv->al_desc->ad_type->sat_flags |= SLAP_AT_SORTED_VAL;
			for ( sv = sortVals; sv && sv->al_next; sv = sv->al_next );
			if ( sv )
				sv->al_next = svnew;
			else
				sortVals = svnew;
			}
			break;

		case CFG_ACL:
			if ( SLAP_CONFIG( c->be ) && c->be->be_acl == defacl_parsed) {
				c->be->be_acl = NULL;
			}
			/* Don't append to the global ACL if we're on a specific DB */
			i = c->valx;
			if ( c->valx == -1 ) {
				AccessControl *a;
				i = 0;
				for ( a=c->be->be_acl; a; a = a->acl_next )
					i++;
			}
			if ( parse_acl(c->be, c->fname, c->lineno, c->argc, c->argv, i ) ) {
				if ( SLAP_CONFIG( c->be ) && !c->be->be_acl) {
					c->be->be_acl = defacl_parsed;
				}
				return 1;
			}
			break;

		case CFG_ACL_ADD:
			if(c->value_int)
				SLAP_DBFLAGS(c->be) |= SLAP_DBFLAG_ACL_ADD;
			else
				SLAP_DBFLAGS(c->be) &= ~SLAP_DBFLAG_ACL_ADD;
			break;

		case CFG_ROOTDSE:
			if(root_dse_read_file(c->argv[1])) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> could not read file", c->argv[0] );
				Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
					c->log, c->cr_msg, c->argv[1] );
				return(1);
			}
			{
				struct berval bv;
				ber_str2bv( c->argv[1], 0, 1, &bv );
				if ( c->op == LDAP_MOD_ADD && c->ca_private && cfn != c->ca_private )
					cfn = c->ca_private;
				ber_bvarray_add( &cfn->c_dseFiles, &bv );
			}
			break;

		case CFG_SERVERID:
			{
				ServerID *si, **sip;
				LDAPURLDesc *lud;
				int num;
				if (( lutil_atoi( &num, c->argv[1] ) &&	
					lutil_atoix( &num, c->argv[1], 16 )) ||
					num < 0 || num > SLAP_SYNC_SID_MAX )
				{
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"<%s> illegal server ID", c->argv[0] );
					Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
						c->log, c->cr_msg, c->argv[1] );
					return 1;
				}
				/* only one value allowed if no URL is given */
				if ( c->argc > 2 ) {
					int len;

					if ( sid_list && BER_BVISEMPTY( &sid_list->si_url )) {
						snprintf( c->cr_msg, sizeof( c->cr_msg ),
							"<%s> only one server ID allowed now", c->argv[0] );
						Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
							c->log, c->cr_msg, c->argv[1] );
						return 1;
					}

					if ( ldap_url_parse( c->argv[2], &lud )) {
						snprintf( c->cr_msg, sizeof( c->cr_msg ),
							"<%s> invalid URL", c->argv[0] );
						Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
							c->log, c->cr_msg, c->argv[2] );
						return 1;
					}
					len = strlen( c->argv[2] );
					si = ch_malloc( sizeof(ServerID) + len + 1 );
					si->si_url.bv_val = (char *)(si+1);
					si->si_url.bv_len = len;
					strcpy( si->si_url.bv_val, c->argv[2] );
				} else {
					if ( sid_list ) {
						snprintf( c->cr_msg, sizeof( c->cr_msg ),
							"<%s> unqualified server ID not allowed now", c->argv[0] );
						Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
							c->log, c->cr_msg, c->argv[1] );
						return 1;
					}
					si = ch_malloc( sizeof(ServerID) );
					BER_BVZERO( &si->si_url );
					slap_serverID = num;
					Debug( LDAP_DEBUG_CONFIG,
						"%s: SID=0x%03x\n",
						c->log, slap_serverID, 0 );
					sid_set = si;
				}
				si->si_next = NULL;
				si->si_num = num;
				for ( sip = &sid_list; *sip; sip = &(*sip)->si_next );
				*sip = si;

				if (( slapMode & SLAP_SERVER_MODE ) && c->argc > 2 ) {
					Listener *l = config_check_my_url( c->argv[2], lud );
					if ( l ) {
						if ( sid_set ) {
							ldap_free_urldesc( lud );
							snprintf( c->cr_msg, sizeof( c->cr_msg ),
								"<%s> multiple server ID URLs matched, only one is allowed", c->argv[0] );
							Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
								c->log, c->cr_msg, c->argv[1] );
							return 1;
						}
						slap_serverID = si->si_num;
						Debug( LDAP_DEBUG_CONFIG,
							"%s: SID=0x%03x (listener=%s)\n",
							c->log, slap_serverID,
							l->sl_url.bv_val );
						sid_set = si;
					}
				}
				if ( c->argc > 2 )
					ldap_free_urldesc( lud );
			}
			break;
		case CFG_LOGFILE: {
				if ( logfileName ) ch_free( logfileName );
				logfileName = c->value_string;
				logfile = fopen(logfileName, "w");
				if(logfile) lutil_debug_file(logfile);
			} break;

		case CFG_LASTMOD:
			if(SLAP_NOLASTMODCMD(c->be)) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> not available for %s database",
					c->argv[0], c->be->bd_info->bi_type );
				Debug(LDAP_DEBUG_ANY, "%s: %s\n",
					c->log, c->cr_msg, 0 );
				return(1);
			}
			if(c->value_int)
				SLAP_DBFLAGS(c->be) &= ~SLAP_DBFLAG_NOLASTMOD;
			else
				SLAP_DBFLAGS(c->be) |= SLAP_DBFLAG_NOLASTMOD;
			break;

		case CFG_MIRRORMODE:
			if(c->value_int && !SLAP_SHADOW(c->be)) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> database is not a shadow",
					c->argv[0] );
				Debug(LDAP_DEBUG_ANY, "%s: %s\n",
					c->log, c->cr_msg, 0 );
				return(1);
			}
			if(c->value_int) {
				SLAP_DBFLAGS(c->be) &= ~SLAP_DBFLAG_SINGLE_SHADOW;
				SLAP_DBFLAGS(c->be) |= SLAP_DBFLAG_MULTI_SHADOW;
			} else {
				SLAP_DBFLAGS(c->be) |= SLAP_DBFLAG_SINGLE_SHADOW;
				SLAP_DBFLAGS(c->be) &= ~SLAP_DBFLAG_MULTI_SHADOW;
			}
			break;

		case CFG_MONITORING:
			if(c->value_int)
				SLAP_DBFLAGS(c->be) |= SLAP_DBFLAG_MONITORING;
			else
				SLAP_DBFLAGS(c->be) &= ~SLAP_DBFLAG_MONITORING;
			break;

		case CFG_DISABLED:
			if ( c->bi ) {
				if (c->value_int) {
					if ( c->bi->bi_db_close ) {
						BackendInfo *bi_orig = c->be->bd_info;
						c->be->bd_info = c->bi;
						c->bi->bi_db_close( c->be, &c->reply );
						c->be->bd_info = bi_orig;
					}
					c->bi->bi_flags |= SLAPO_BFLAG_DISABLED;
				} else {
					c->bi->bi_flags &= ~SLAPO_BFLAG_DISABLED;
				}
			} else {
				if (c->value_int) {
					backend_shutdown( c->be );
					SLAP_DBFLAGS(c->be) |= SLAP_DBFLAG_DISABLED;
				} else {
					SLAP_DBFLAGS(c->be) &= ~SLAP_DBFLAG_DISABLED;
				}
			}
			break;

		case CFG_HIDDEN:
			if (c->value_int)
				SLAP_DBFLAGS(c->be) |= SLAP_DBFLAG_HIDDEN;
			else
				SLAP_DBFLAGS(c->be) &= ~SLAP_DBFLAG_HIDDEN;
			break;

		case CFG_SYNC_SUBENTRY:
			if (c->value_int)
				SLAP_DBFLAGS(c->be) |= SLAP_DBFLAG_SYNC_SUBENTRY;
			else
				SLAP_DBFLAGS(c->be) &= ~SLAP_DBFLAG_SYNC_SUBENTRY;
			break;

		case CFG_SSTR_IF_MAX:
			if (c->value_uint < index_substr_if_minlen) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> invalid value", c->argv[0] );
				Debug(LDAP_DEBUG_ANY, "%s: %s (%d)\n",
					c->log, c->cr_msg, c->value_int );
				return(1);
			}
			index_substr_if_maxlen = c->value_uint;
			break;

		case CFG_SSTR_IF_MIN:
			if (c->value_uint > index_substr_if_maxlen) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> invalid value", c->argv[0] );
				Debug(LDAP_DEBUG_ANY, "%s: %s (%d)\n",
					c->log, c->cr_msg, c->value_int );
				return(1);
			}
			index_substr_if_minlen = c->value_uint;
			break;

#ifdef SLAPD_MODULES
		case CFG_MODLOAD:
			/* If we're just adding a module on an existing modpath,
			 * make sure we've selected the current path.
			 */
			if ( c->op == LDAP_MOD_ADD && c->ca_private && modcur != c->ca_private ) {
				modcur = c->ca_private;
				/* This should never fail */
				if ( module_path( modcur->mp_path.bv_val )) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> module path no longer valid",
						c->argv[0] );
					Debug(LDAP_DEBUG_ANY, "%s: %s (%s)\n",
						c->log, c->cr_msg, modcur->mp_path.bv_val );
					return(1);
				}
			}
			if(module_load(c->argv[1], c->argc - 2, (c->argc > 2) ? c->argv + 2 : NULL))
				return(1);
			/* Record this load on the current path */
			{
				struct berval bv;
				char *ptr;
				if ( c->op == SLAP_CONFIG_ADD ) {
					ptr = c->line + STRLENOF("moduleload");
					while (!isspace((unsigned char) *ptr)) ptr++;
					while (isspace((unsigned char) *ptr)) ptr++;
				} else {
					ptr = c->line;
				}
				ber_str2bv(ptr, 0, 1, &bv);
				ber_bvarray_add( &modcur->mp_loads, &bv );
			}
			/* Check for any new hardcoded schema */
			if ( c->op == LDAP_MOD_ADD && CONFIG_ONLINE_ADD( c )) {
				config_check_schema( NULL, &cfBackInfo );
			}
			break;

		case CFG_MODPATH:
			if(module_path(c->argv[1])) return(1);
			/* Record which path was used with each module */
			{
				ModPaths *mp;

				if (!modpaths.mp_loads) {
					mp = &modpaths;
				} else {
					mp = ch_malloc( sizeof( ModPaths ));
					modlast->mp_next = mp;
				}
				ber_str2bv(c->argv[1], 0, 1, &mp->mp_path);
				mp->mp_next = NULL;
				mp->mp_loads = NULL;
				modlast = mp;
				c->ca_private = mp;
				modcur = mp;
			}
			
			break;
#endif

#ifdef LDAP_SLAPI
		case CFG_PLUGIN:
			if(slapi_int_read_config(c->be, c->fname, c->lineno, c->argc, c->argv) != LDAP_SUCCESS)
				return(1);
			slapi_plugins_used++;
			break;
#endif

#ifdef SLAP_AUTH_REWRITE
		case CFG_REWRITE: {
			struct berval bv;
			char *line;
			int rc = 0;

			if ( c->op == LDAP_MOD_ADD ) {
				c->argv++;
				c->argc--;
			}
			if(slap_sasl_rewrite_config(c->fname, c->lineno, c->argc, c->argv))
				rc = 1;
			if ( rc == 0 ) {

				if ( c->argc > 1 ) {
					char	*s;

					/* quote all args but the first */
					line = ldap_charray2str( c->argv, "\" \"" );
					ber_str2bv( line, 0, 0, &bv );
					s = ber_bvchr( &bv, '"' );
					assert( s != NULL );
					/* move the trailing quote of argv[0] to the end */
					AC_MEMCPY( s, s + 1, bv.bv_len - ( s - bv.bv_val ) );
					bv.bv_val[ bv.bv_len - 1 ] = '"';

				} else {
					ber_str2bv( c->argv[ 0 ], 0, 1, &bv );
				}

				ber_bvarray_add( &authz_rewrites, &bv );
			}
			if ( c->op == LDAP_MOD_ADD ) {
				c->argv--;
				c->argc++;
			}
			return rc;
			}
#endif


		default:
			Debug( LDAP_DEBUG_ANY,
				"%s: unknown CFG_TYPE %d.\n",
				c->log, c->type, 0 );
			return 1;

	}
	return(0);
}


static int
config_fname(ConfigArgs *c) {
	if(c->op == SLAP_CONFIG_EMIT) {
		if (c->ca_private) {
			ConfigFile *cf = c->ca_private;
			value_add_one( &c->rvalue_vals, &cf->c_file );
			return 0;
		}
		return 1;
	}
	return(0);
}

static int
config_cfdir(ConfigArgs *c) {
	if(c->op == SLAP_CONFIG_EMIT) {
		if ( !BER_BVISEMPTY( &cfdir )) {
			value_add_one( &c->rvalue_vals, &cfdir );
			return 0;
		}
		return 1;
	}
	return(0);
}

static int
config_search_base(ConfigArgs *c) {
	if(c->op == SLAP_CONFIG_EMIT) {
		int rc = 1;
		if (!BER_BVISEMPTY(&default_search_base)) {
			value_add_one(&c->rvalue_vals, &default_search_base);
			value_add_one(&c->rvalue_nvals, &default_search_nbase);
			rc = 0;
		}
		return rc;
	} else if( c->op == LDAP_MOD_DELETE ) {
		ch_free( default_search_base.bv_val );
		ch_free( default_search_nbase.bv_val );
		BER_BVZERO( &default_search_base );
		BER_BVZERO( &default_search_nbase );
		return 0;
	}

	if(c->bi || c->be != frontendDB) {
		Debug(LDAP_DEBUG_ANY, "%s: defaultSearchBase line must appear "
			"prior to any backend or database definition\n",
			c->log, 0, 0);
		return(1);
	}

	if(default_search_nbase.bv_len) {
		free(default_search_base.bv_val);
		free(default_search_nbase.bv_val);
	}

	default_search_base = c->value_dn;
	default_search_nbase = c->value_ndn;
	return(0);
}

/* For RE23 compatibility we allow this in the global entry
 * but we now defer it to the frontend entry to allow modules
 * to load new hash types.
 */
static int
config_passwd_hash(ConfigArgs *c) {
	int i;
	if (c->op == SLAP_CONFIG_EMIT) {
		struct berval bv;
		/* Don't generate it in the global entry */
		if ( c->table == Cft_Global )
			return 1;
		for (i=0; default_passwd_hash && default_passwd_hash[i]; i++) {
			ber_str2bv(default_passwd_hash[i], 0, 0, &bv);
			value_add_one(&c->rvalue_vals, &bv);
		}
		return i ? 0 : 1;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		/* Deleting from global is a no-op, only the frontendDB entry matters */
		if ( c->table == Cft_Global )
			return 0;
		if ( c->valx < 0 ) {
			ldap_charray_free( default_passwd_hash );
			default_passwd_hash = NULL;
		} else {
			i = c->valx;
			ch_free( default_passwd_hash[i] );
			for (; default_passwd_hash[i]; i++ )
				default_passwd_hash[i] = default_passwd_hash[i+1];
		}
		return 0;
	}
	for(i = 1; i < c->argc; i++) {
		if(!lutil_passwd_scheme(c->argv[i])) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> scheme not available", c->argv[0] );
			Debug(LDAP_DEBUG_ANY, "%s: %s (%s)\n",
				c->log, c->cr_msg, c->argv[i]);
		} else {
			ldap_charray_add(&default_passwd_hash, c->argv[i]);
		}
	}
	if(!default_passwd_hash) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> no valid hashes found", c->argv[0] );
		Debug(LDAP_DEBUG_ANY, "%s: %s\n",
			c->log, c->cr_msg, 0 );
		return(1);
	}
	return(0);
}

static int
config_schema_dn(ConfigArgs *c) {
	if ( c->op == SLAP_CONFIG_EMIT ) {
		int rc = 1;
		if ( !BER_BVISEMPTY( &c->be->be_schemadn )) {
			value_add_one(&c->rvalue_vals, &c->be->be_schemadn);
			value_add_one(&c->rvalue_nvals, &c->be->be_schemandn);
			rc = 0;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		ch_free( c->be->be_schemadn.bv_val );
		ch_free( c->be->be_schemandn.bv_val );
		BER_BVZERO( &c->be->be_schemadn );
		BER_BVZERO( &c->be->be_schemandn );
		return 0;
	}
	ch_free( c->be->be_schemadn.bv_val );
	ch_free( c->be->be_schemandn.bv_val );
	c->be->be_schemadn = c->value_dn;
	c->be->be_schemandn = c->value_ndn;
	return(0);
}

static int
config_sizelimit(ConfigArgs *c) {
	int i, rc = 0;
	struct slap_limits_set *lim = &c->be->be_def_limit;
	if (c->op == SLAP_CONFIG_EMIT) {
		char buf[8192];
		struct berval bv;
		bv.bv_val = buf;
		bv.bv_len = 0;
		limits_unparse_one( lim, SLAP_LIMIT_SIZE, &bv, sizeof( buf ) );
		if ( !BER_BVISEMPTY( &bv ))
			value_add_one( &c->rvalue_vals, &bv );
		else
			rc = 1;
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		/* Reset to defaults or values from frontend */
		if ( c->be == frontendDB ) {
			lim->lms_s_soft = SLAPD_DEFAULT_SIZELIMIT;
			lim->lms_s_hard = 0;
			lim->lms_s_unchecked = -1;
			lim->lms_s_pr = 0;
			lim->lms_s_pr_hide = 0;
			lim->lms_s_pr_total = 0;
		} else {
			lim->lms_s_soft = frontendDB->be_def_limit.lms_s_soft;
			lim->lms_s_hard = frontendDB->be_def_limit.lms_s_hard;
			lim->lms_s_unchecked = frontendDB->be_def_limit.lms_s_unchecked;
			lim->lms_s_pr = frontendDB->be_def_limit.lms_s_pr;
			lim->lms_s_pr_hide = frontendDB->be_def_limit.lms_s_pr_hide;
			lim->lms_s_pr_total = frontendDB->be_def_limit.lms_s_pr_total;
		}
		goto ok;
	}
	for(i = 1; i < c->argc; i++) {
		if(!strncasecmp(c->argv[i], "size", 4)) {
			rc = limits_parse_one(c->argv[i], lim);
			if ( rc ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unable to parse value", c->argv[0] );
				Debug(LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
					c->log, c->cr_msg, c->argv[i]);
				return(1);
			}
		} else {
			if(!strcasecmp(c->argv[i], "unlimited")) {
				lim->lms_s_soft = -1;
			} else {
				if ( lutil_atoix( &lim->lms_s_soft, c->argv[i], 0 ) != 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unable to parse limit", c->argv[0]);
					Debug(LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
						c->log, c->cr_msg, c->argv[i]);
					return(1);
				}
			}
			lim->lms_s_hard = 0;
		}
	}

ok:
	if ( ( c->be == frontendDB ) && ( c->ca_entry ) ) {
		/* This is a modification to the global limits apply it to
		 * the other databases as needed */
		AttributeDescription *ad=NULL;
		const char *text = NULL;
		CfEntryInfo *ce = c->ca_entry->e_private;

		slap_str2ad(c->argv[0], &ad, &text);
		/* if we got here... */
		assert( ad != NULL );

		if ( ce->ce_type == Cft_Global ){
			ce = ce->ce_kids;
		}
		for (; ce; ce=ce->ce_sibs) {
			Entry *dbe = ce->ce_entry;
			if ( (ce->ce_type == Cft_Database) && (ce->ce_be != frontendDB)
					&& (!attr_find(dbe->e_attrs, ad)) ) {
				ce->ce_be->be_def_limit.lms_s_soft = lim->lms_s_soft;
				ce->ce_be->be_def_limit.lms_s_hard = lim->lms_s_hard;
				ce->ce_be->be_def_limit.lms_s_unchecked =lim->lms_s_unchecked;
				ce->ce_be->be_def_limit.lms_s_pr =lim->lms_s_pr;
				ce->ce_be->be_def_limit.lms_s_pr_hide =lim->lms_s_pr_hide;
				ce->ce_be->be_def_limit.lms_s_pr_total =lim->lms_s_pr_total;
			}
		}
	}
	return(0);
}

static int
config_timelimit(ConfigArgs *c) {
	int i, rc = 0;
	struct slap_limits_set *lim = &c->be->be_def_limit;
	if (c->op == SLAP_CONFIG_EMIT) {
		char buf[8192];
		struct berval bv;
		bv.bv_val = buf;
		bv.bv_len = 0;
		limits_unparse_one( lim, SLAP_LIMIT_TIME, &bv, sizeof( buf ) );
		if ( !BER_BVISEMPTY( &bv ))
			value_add_one( &c->rvalue_vals, &bv );
		else
			rc = 1;
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		/* Reset to defaults or values from frontend */
		if ( c->be == frontendDB ) {
			lim->lms_t_soft = SLAPD_DEFAULT_TIMELIMIT;
			lim->lms_t_hard = 0;
		} else {
			lim->lms_t_soft = frontendDB->be_def_limit.lms_t_soft;
			lim->lms_t_hard = frontendDB->be_def_limit.lms_t_hard;
		}
		goto ok;
	}
	for(i = 1; i < c->argc; i++) {
		if(!strncasecmp(c->argv[i], "time", 4)) {
			rc = limits_parse_one(c->argv[i], lim);
			if ( rc ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unable to parse value", c->argv[0] );
				Debug(LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
					c->log, c->cr_msg, c->argv[i]);
				return(1);
			}
		} else {
			if(!strcasecmp(c->argv[i], "unlimited")) {
				lim->lms_t_soft = -1;
			} else {
				if ( lutil_atoix( &lim->lms_t_soft, c->argv[i], 0 ) != 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unable to parse limit", c->argv[0]);
					Debug(LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
						c->log, c->cr_msg, c->argv[i]);
					return(1);
				}
			}
			lim->lms_t_hard = 0;
		}
	}

ok:
	if ( ( c->be == frontendDB ) && ( c->ca_entry ) ) {
		/* This is a modification to the global limits apply it to
		 * the other databases as needed */
		AttributeDescription *ad=NULL;
		const char *text = NULL;
		CfEntryInfo *ce = c->ca_entry->e_private;

		slap_str2ad(c->argv[0], &ad, &text);
		/* if we got here... */
		assert( ad != NULL );

		if ( ce->ce_type == Cft_Global ){
			ce = ce->ce_kids;
		}
		for (; ce; ce=ce->ce_sibs) {
			Entry *dbe = ce->ce_entry;
			if ( (ce->ce_type == Cft_Database) && (ce->ce_be != frontendDB)
					&& (!attr_find(dbe->e_attrs, ad)) ) {
				ce->ce_be->be_def_limit.lms_t_soft = lim->lms_t_soft;
				ce->ce_be->be_def_limit.lms_t_hard = lim->lms_t_hard;
			}
		}
	}
	return(0);
}

static int
config_overlay(ConfigArgs *c) {
	if (c->op == SLAP_CONFIG_EMIT) {
		return 1;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		assert(0);
	}
	if(c->argv[1][0] == '-' && overlay_config(c->be, &c->argv[1][1],
		c->valx, &c->bi, &c->reply)) {
		/* log error */
		Debug( LDAP_DEBUG_ANY,
			"%s: (optional) %s overlay \"%s\" configuration failed.\n",
			c->log, c->be == frontendDB ? "global " : "", &c->argv[1][1]);
		return 1;
	} else if(overlay_config(c->be, c->argv[1], c->valx, &c->bi, &c->reply)) {
		return(1);
	}
	return(0);
}

static int
config_subordinate(ConfigArgs *c)
{
	int rc = 1;
	int advertise = 0;

	switch( c->op ) {
	case SLAP_CONFIG_EMIT:
		if ( SLAP_GLUE_SUBORDINATE( c->be )) {
			struct berval bv;

			bv.bv_val = SLAP_GLUE_ADVERTISE( c->be ) ? "advertise" : "TRUE";
			bv.bv_len = SLAP_GLUE_ADVERTISE( c->be ) ? STRLENOF("advertise") :
				STRLENOF("TRUE");

			value_add_one( &c->rvalue_vals, &bv );
			rc = 0;
		}
		break;
	case LDAP_MOD_DELETE:
		if ( !c->line  || strcasecmp( c->line, "advertise" )) {
			glue_sub_del( c->be );
		} else {
			SLAP_DBFLAGS( c->be ) &= ~SLAP_DBFLAG_GLUE_ADVERTISE;
		}
		rc = 0;
		break;
	case LDAP_MOD_ADD:
	case SLAP_CONFIG_ADD:
		if ( c->be->be_nsuffix == NULL ) {
			/* log error */
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"subordinate configuration needs a suffix" );
			Debug( LDAP_DEBUG_ANY,
				"%s: %s.\n",
				c->log, c->cr_msg, 0 );
			rc = 1;
			break;
		}

		if ( c->argc == 2 ) {
			if ( strcasecmp( c->argv[1], "advertise" ) == 0 ) {
				advertise = 1;

			} else if ( strcasecmp( c->argv[1], "TRUE" ) != 0 ) {
				/* log error */
				snprintf( c->cr_msg, sizeof( c->cr_msg),
					"subordinate must be \"TRUE\" or \"advertise\"" );
				Debug( LDAP_DEBUG_ANY,
					"%s: suffix \"%s\": %s.\n",
					c->log, c->be->be_suffix[0].bv_val, c->cr_msg );
				rc = 1;
				break;
			}
		}

		rc = glue_sub_add( c->be, advertise, CONFIG_ONLINE_ADD( c ));
		break;
	}

	return rc;
}

/*
 * [listener=<listener>] [{read|write}=]<size>
 */

#ifdef LDAP_TCP_BUFFER
static BerVarray tcp_buffer;
int tcp_buffer_num;

#define SLAP_TCP_RMEM (0x1U)
#define SLAP_TCP_WMEM (0x2U)

static int
tcp_buffer_parse( struct berval *val, int argc, char **argv,
		int *size, int *rw, Listener **l )
{
	int i, rc = LDAP_SUCCESS;
	LDAPURLDesc *lud = NULL;
	char *ptr;

	if ( val != NULL && argv == NULL ) {
		char *s = val->bv_val;

		argv = ldap_str2charray( s, " \t" );
		if ( argv == NULL ) {
			return LDAP_OTHER;
		}
	}

	i = 0;
	if ( strncasecmp( argv[ i ], "listener=", STRLENOF( "listener=" ) )
		== 0 )
	{
		char *url = argv[ i ] + STRLENOF( "listener=" );
		
		if ( ldap_url_parse( url, &lud ) ) {
			rc = LDAP_INVALID_SYNTAX;
			goto done;
		}

		*l = config_check_my_url( url, lud );
		if ( *l == NULL ) {
			rc = LDAP_NO_SUCH_ATTRIBUTE;
			goto done;
		}

		i++;
	}

	ptr = argv[ i ];
	if ( strncasecmp( ptr, "read=", STRLENOF( "read=" ) ) == 0 ) {
		*rw |= SLAP_TCP_RMEM;
		ptr += STRLENOF( "read=" );

	} else if ( strncasecmp( ptr, "write=", STRLENOF( "write=" ) ) == 0 ) {
		*rw |= SLAP_TCP_WMEM;
		ptr += STRLENOF( "write=" );

	} else {
		*rw |= ( SLAP_TCP_RMEM | SLAP_TCP_WMEM );
	}

	/* accept any base */
	if ( lutil_atoix( size, ptr, 0 ) ) {
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}

done:;
	if ( val != NULL && argv != NULL ) {
		ldap_charray_free( argv );
	}

	if ( lud != NULL ) {
		ldap_free_urldesc( lud );
	}

	return rc;
}

static int
tcp_buffer_delete_one( struct berval *val )
{
	int rc = 0;
	int size = -1, rw = 0;
	Listener *l = NULL;

	rc = tcp_buffer_parse( val, 0, NULL, &size, &rw, &l );
	if ( rc != 0 ) {
		return rc;
	}

	if ( l != NULL ) {
		int i;
		Listener **ll = slapd_get_listeners();

		for ( i = 0; ll[ i ] != NULL; i++ ) {
			if ( ll[ i ] == l ) break;
		}

		if ( ll[ i ] == NULL ) {
			return LDAP_NO_SUCH_ATTRIBUTE;
		}

		if ( rw & SLAP_TCP_RMEM ) l->sl_tcp_rmem = -1;
		if ( rw & SLAP_TCP_WMEM ) l->sl_tcp_wmem = -1;

		for ( i++ ; ll[ i ] != NULL && bvmatch( &l->sl_url, &ll[ i ]->sl_url ); i++ ) {
			if ( rw & SLAP_TCP_RMEM ) ll[ i ]->sl_tcp_rmem = -1;
			if ( rw & SLAP_TCP_WMEM ) ll[ i ]->sl_tcp_wmem = -1;
		}

	} else {
		/* NOTE: this affects listeners without a specific setting,
		 * does not reset all listeners.  If a listener without
		 * specific settings was assigned a buffer because of
		 * a global setting, it will not be reset.  In any case,
		 * buffer changes will only take place at restart. */
		if ( rw & SLAP_TCP_RMEM ) slapd_tcp_rmem = -1;
		if ( rw & SLAP_TCP_WMEM ) slapd_tcp_wmem = -1;
	}

	return rc;
}

static int
tcp_buffer_delete( BerVarray vals )
{
	int i;

	for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
		tcp_buffer_delete_one( &vals[ i ] );
	}

	return 0;
}

static int
tcp_buffer_unparse( int size, int rw, Listener *l, struct berval *val )
{
	char buf[sizeof("2147483648")], *ptr;

	/* unparse for later use */
	val->bv_len = snprintf( buf, sizeof( buf ), "%d", size );
	if ( l != NULL ) {
		val->bv_len += STRLENOF( "listener=" " " ) + l->sl_url.bv_len;
	}

	if ( rw != ( SLAP_TCP_RMEM | SLAP_TCP_WMEM ) ) {
		if ( rw & SLAP_TCP_RMEM ) {
			val->bv_len += STRLENOF( "read=" );
		} else if ( rw & SLAP_TCP_WMEM ) {
			val->bv_len += STRLENOF( "write=" );
		}
	}

	val->bv_val = SLAP_MALLOC( val->bv_len + 1 );

	ptr = val->bv_val;

	if ( l != NULL ) {
		ptr = lutil_strcopy( ptr, "listener=" );
		ptr = lutil_strncopy( ptr, l->sl_url.bv_val, l->sl_url.bv_len );
		*ptr++ = ' ';
	}

	if ( rw != ( SLAP_TCP_RMEM | SLAP_TCP_WMEM ) ) {
		if ( rw & SLAP_TCP_RMEM ) {
			ptr = lutil_strcopy( ptr, "read=" );
		} else if ( rw & SLAP_TCP_WMEM ) {
			ptr = lutil_strcopy( ptr, "write=" );
		}
	}

	ptr = lutil_strcopy( ptr, buf );
	*ptr = '\0';

	assert( val->bv_val + val->bv_len == ptr );

	return LDAP_SUCCESS;
}

static int
tcp_buffer_add_one( int argc, char **argv )
{
	int rc = 0;
	int size = -1, rw = 0;
	Listener *l = NULL;

	struct berval val;

	/* parse */
	rc = tcp_buffer_parse( NULL, argc, argv, &size, &rw, &l );
	if ( rc != 0 ) {
		return rc;
	}

	/* unparse for later use */
	rc = tcp_buffer_unparse( size, rw, l, &val );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	/* use parsed values */
	if ( l != NULL ) {
		int i;
		Listener **ll = slapd_get_listeners();

		for ( i = 0; ll[ i ] != NULL; i++ ) {
			if ( ll[ i ] == l ) break;
		}

		if ( ll[ i ] == NULL ) {
			return LDAP_NO_SUCH_ATTRIBUTE;
		}

		/* buffer only applies to TCP listeners;
		 * we do not do any check here, and delegate them
		 * to setsockopt(2) */
		if ( rw & SLAP_TCP_RMEM ) l->sl_tcp_rmem = size;
		if ( rw & SLAP_TCP_WMEM ) l->sl_tcp_wmem = size;

		for ( i++ ; ll[ i ] != NULL && bvmatch( &l->sl_url, &ll[ i ]->sl_url ); i++ ) {
			if ( rw & SLAP_TCP_RMEM ) ll[ i ]->sl_tcp_rmem = size;
			if ( rw & SLAP_TCP_WMEM ) ll[ i ]->sl_tcp_wmem = size;
		}

	} else {
		/* NOTE: this affects listeners without a specific setting,
		 * does not set all listeners */
		if ( rw & SLAP_TCP_RMEM ) slapd_tcp_rmem = size;
		if ( rw & SLAP_TCP_WMEM ) slapd_tcp_wmem = size;
	}

	tcp_buffer = SLAP_REALLOC( tcp_buffer, sizeof( struct berval ) * ( tcp_buffer_num + 2 ) );
	/* append */
	tcp_buffer[ tcp_buffer_num ] = val;

	tcp_buffer_num++;
	BER_BVZERO( &tcp_buffer[ tcp_buffer_num ] );

	return rc;
}

static int
config_tcp_buffer( ConfigArgs *c )
{
	if ( c->op == SLAP_CONFIG_EMIT ) {
		if ( tcp_buffer == NULL || BER_BVISNULL( &tcp_buffer[ 0 ] ) ) {
			return 1;
		}
		value_add( &c->rvalue_vals, tcp_buffer );
		value_add( &c->rvalue_nvals, tcp_buffer );
		
	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( !c->line  ) {
			tcp_buffer_delete( tcp_buffer );
			ber_bvarray_free( tcp_buffer );
			tcp_buffer = NULL;
			tcp_buffer_num = 0;

		} else {
			int rc = 0;
			int size = -1, rw = 0;
			Listener *l = NULL;

			struct berval val = BER_BVNULL;

			int i;

			if ( tcp_buffer_num == 0 ) {
				return 1;
			}

			/* parse */
			rc = tcp_buffer_parse( NULL, c->argc - 1, &c->argv[ 1 ], &size, &rw, &l );
			if ( rc != 0 ) {
				return 1;
			}

			/* unparse for later use */
			rc = tcp_buffer_unparse( size, rw, l, &val );
			if ( rc != LDAP_SUCCESS ) {
				return 1;
			}

			for ( i = 0; !BER_BVISNULL( &tcp_buffer[ i ] ); i++ ) {
				if ( bvmatch( &tcp_buffer[ i ], &val ) ) {
					break;
				}
			}

			if ( BER_BVISNULL( &tcp_buffer[ i ] ) ) {
				/* not found */
				rc = 1;
				goto done;
			}

			tcp_buffer_delete_one( &tcp_buffer[ i ] );
			ber_memfree( tcp_buffer[ i ].bv_val );
			for ( ; i < tcp_buffer_num; i++ ) {
				tcp_buffer[ i ] = tcp_buffer[ i + 1 ];
			}
			tcp_buffer_num--;

done:;
			if ( !BER_BVISNULL( &val ) ) {
				SLAP_FREE( val.bv_val );
			}
	
		}

	} else {
		int rc;

		rc = tcp_buffer_add_one( c->argc - 1, &c->argv[ 1 ] );
		if ( rc ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"<%s> unable to add value #%d",
				c->argv[0], tcp_buffer_num );
			Debug( LDAP_DEBUG_ANY, "%s: %s\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}
	}

	return 0;
}
#endif /* LDAP_TCP_BUFFER */

static int
config_suffix(ConfigArgs *c)
{
	Backend *tbe;
	struct berval pdn, ndn;
	char	*notallowed = NULL;

	if ( c->be == frontendDB ) {
		notallowed = "frontend";

	} else if ( SLAP_MONITOR(c->be) ) {
		notallowed = "monitor";

	} else if ( SLAP_CONFIG(c->be) ) {
		notallowed = "config";
	}

	if ( notallowed != NULL ) {
		char	buf[ SLAP_TEXT_BUFLEN ] = { '\0' };

		switch ( c->op ) {
		case LDAP_MOD_ADD:
		case LDAP_MOD_DELETE:
		case LDAP_MOD_REPLACE:
		case LDAP_MOD_INCREMENT:
		case SLAP_CONFIG_ADD:
			if ( !BER_BVISNULL( &c->value_dn ) ) {
				snprintf( buf, sizeof( buf ), "<%s> ",
						c->value_dn.bv_val );
			}

			Debug(LDAP_DEBUG_ANY,
				"%s: suffix %snot allowed in %s database.\n",
				c->log, buf, notallowed );
			break;

		case SLAP_CONFIG_EMIT:
			/* don't complain when emitting... */
			break;

		default:
			/* FIXME: don't know what values may be valid;
			 * please remove assertion, or add legal values
			 * to either block */
			assert( 0 );
			break;
		}

		return 1;
	}

	if (c->op == SLAP_CONFIG_EMIT) {
		if ( c->be->be_suffix == NULL
				|| BER_BVISNULL( &c->be->be_suffix[0] ) )
		{
			return 1;
		} else {
			value_add( &c->rvalue_vals, c->be->be_suffix );
			value_add( &c->rvalue_nvals, c->be->be_nsuffix );
			return 0;
		}
	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( c->valx < 0 ) {
			ber_bvarray_free( c->be->be_suffix );
			ber_bvarray_free( c->be->be_nsuffix );
			c->be->be_suffix = NULL;
			c->be->be_nsuffix = NULL;
		} else {
			int i = c->valx;
			ch_free( c->be->be_suffix[i].bv_val );
			ch_free( c->be->be_nsuffix[i].bv_val );
			do {
				c->be->be_suffix[i] = c->be->be_suffix[i+1];
				c->be->be_nsuffix[i] = c->be->be_nsuffix[i+1];
				i++;
			} while ( !BER_BVISNULL( &c->be->be_suffix[i] ) );
		}
		return 0;
	}

#ifdef SLAPD_MONITOR_DN
	if(!strcasecmp(c->argv[1], SLAPD_MONITOR_DN)) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> DN is reserved for monitoring slapd",
			c->argv[0] );
		Debug(LDAP_DEBUG_ANY, "%s: %s (%s)\n",
			c->log, c->cr_msg, SLAPD_MONITOR_DN);
		return(1);
	}
#endif

	if (SLAP_DB_ONE_SUFFIX( c->be ) && c->be->be_suffix &&
		!BER_BVISNULL( &c->be->be_suffix[0] )) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> Only one suffix is allowed on this %s backend",
			c->argv[0], c->be->bd_info->bi_type );
		Debug(LDAP_DEBUG_ANY, "%s: %s\n",
			c->log, c->cr_msg, 0);
		return(1);
	}

	pdn = c->value_dn;
	ndn = c->value_ndn;

	if (SLAP_DBHIDDEN( c->be ))
		tbe = NULL;
	else
		tbe = select_backend(&ndn, 0);
	if(tbe == c->be) {
		Debug( LDAP_DEBUG_ANY, "%s: suffix already served by this backend!.\n",
			c->log, 0, 0);
		return 1;
		free(pdn.bv_val);
		free(ndn.bv_val);
	} else if(tbe) {
		BackendDB *b2 = tbe;

		/* Does tbe precede be? */
		while (( b2 = LDAP_STAILQ_NEXT(b2, be_next )) && b2 && b2 != c->be );

		if ( b2 ) {
			char	*type = tbe->bd_info->bi_type;

			if ( overlay_is_over( tbe ) ) {
				slap_overinfo	*oi = (slap_overinfo *)tbe->bd_info->bi_private;
				type = oi->oi_orig->bi_type;
			}

			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> namingContext \"%s\" "
				"already served by a preceding %s database",
				c->argv[0], pdn.bv_val, type );
			Debug(LDAP_DEBUG_ANY, "%s: %s serving namingContext \"%s\"\n",
				c->log, c->cr_msg, tbe->be_suffix[0].bv_val);
			free(pdn.bv_val);
			free(ndn.bv_val);
			return(1);
		}
	}
	if(pdn.bv_len == 0 && default_search_nbase.bv_len) {
		Debug(LDAP_DEBUG_ANY, "%s: suffix DN empty and default search "
			"base provided \"%s\" (assuming okay)\n",
			c->log, default_search_base.bv_val, 0);
	}
	ber_bvarray_add(&c->be->be_suffix, &pdn);
	ber_bvarray_add(&c->be->be_nsuffix, &ndn);
	return(0);
}

static int
config_rootdn(ConfigArgs *c) {
	if (c->op == SLAP_CONFIG_EMIT) {
		if ( !BER_BVISNULL( &c->be->be_rootdn )) {
			value_add_one(&c->rvalue_vals, &c->be->be_rootdn);
			value_add_one(&c->rvalue_nvals, &c->be->be_rootndn);
			return 0;
		} else {
			return 1;
		}
	} else if ( c->op == LDAP_MOD_DELETE ) {
		ch_free( c->be->be_rootdn.bv_val );
		ch_free( c->be->be_rootndn.bv_val );
		BER_BVZERO( &c->be->be_rootdn );
		BER_BVZERO( &c->be->be_rootndn );
		return 0;
	}
	if ( !BER_BVISNULL( &c->be->be_rootdn )) {
		ch_free( c->be->be_rootdn.bv_val );
		ch_free( c->be->be_rootndn.bv_val );
	}
	c->be->be_rootdn = c->value_dn;
	c->be->be_rootndn = c->value_ndn;
	return(0);
}

static int
config_rootpw(ConfigArgs *c) {
	Backend *tbe;

	if (c->op == SLAP_CONFIG_EMIT) {
		if (!BER_BVISEMPTY(&c->be->be_rootpw)) {
			/* don't copy, because "rootpw" is marked
			 * as CFG_BERVAL */
			c->value_bv = c->be->be_rootpw;
			return 0;
		}
		return 1;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		ch_free( c->be->be_rootpw.bv_val );
		BER_BVZERO( &c->be->be_rootpw );
		return 0;
	}

	tbe = select_backend(&c->be->be_rootndn, 0);
	if(tbe != c->be) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> can only be set when rootdn is under suffix",
			c->argv[0] );
		Debug(LDAP_DEBUG_ANY, "%s: %s\n",
			c->log, c->cr_msg, 0);
		return(1);
	}
	if ( !BER_BVISNULL( &c->be->be_rootpw ))
		ch_free( c->be->be_rootpw.bv_val );
	c->be->be_rootpw = c->value_bv;
	return(0);
}

static int
config_restrict(ConfigArgs *c) {
	slap_mask_t restrictops = 0;
	int i;
	slap_verbmasks restrictable_ops[] = {
		{ BER_BVC("bind"),		SLAP_RESTRICT_OP_BIND },
		{ BER_BVC("add"),		SLAP_RESTRICT_OP_ADD },
		{ BER_BVC("modify"),		SLAP_RESTRICT_OP_MODIFY },
		{ BER_BVC("rename"),		SLAP_RESTRICT_OP_RENAME },
		{ BER_BVC("modrdn"),		0 },
		{ BER_BVC("delete"),		SLAP_RESTRICT_OP_DELETE },
		{ BER_BVC("search"),		SLAP_RESTRICT_OP_SEARCH },
		{ BER_BVC("compare"),		SLAP_RESTRICT_OP_COMPARE },
		{ BER_BVC("read"),		SLAP_RESTRICT_OP_READS },
		{ BER_BVC("write"),		SLAP_RESTRICT_OP_WRITES },
		{ BER_BVC("extended"),		SLAP_RESTRICT_OP_EXTENDED },
		{ BER_BVC("extended=" LDAP_EXOP_START_TLS ),		SLAP_RESTRICT_EXOP_START_TLS },
		{ BER_BVC("extended=" LDAP_EXOP_MODIFY_PASSWD ),	SLAP_RESTRICT_EXOP_MODIFY_PASSWD },
		{ BER_BVC("extended=" LDAP_EXOP_X_WHO_AM_I ),		SLAP_RESTRICT_EXOP_WHOAMI },
		{ BER_BVC("extended=" LDAP_EXOP_X_CANCEL ),		SLAP_RESTRICT_EXOP_CANCEL },
		{ BER_BVC("all"),		SLAP_RESTRICT_OP_ALL },
		{ BER_BVNULL,	0 }
	};

	if (c->op == SLAP_CONFIG_EMIT) {
		return mask_to_verbs( restrictable_ops, c->be->be_restrictops,
			&c->rvalue_vals );
	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( !c->line ) {
			c->be->be_restrictops = 0;
		} else {
			i = verb_to_mask( c->line, restrictable_ops );
			c->be->be_restrictops &= ~restrictable_ops[i].mask;
		}
		return 0;
	}
	i = verbs_to_mask( c->argc, c->argv, restrictable_ops, &restrictops );
	if ( i ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unknown operation", c->argv[0] );
		Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
			c->log, c->cr_msg, c->argv[i]);
		return(1);
	}
	if ( restrictops & SLAP_RESTRICT_OP_EXTENDED )
		restrictops &= ~SLAP_RESTRICT_EXOP_MASK;
	c->be->be_restrictops |= restrictops;
	return(0);
}

static int
config_allows(ConfigArgs *c) {
	slap_mask_t allows = 0;
	int i;
	slap_verbmasks allowable_ops[] = {
		{ BER_BVC("bind_v2"),		SLAP_ALLOW_BIND_V2 },
		{ BER_BVC("bind_anon_cred"),	SLAP_ALLOW_BIND_ANON_CRED },
		{ BER_BVC("bind_anon_dn"),	SLAP_ALLOW_BIND_ANON_DN },
		{ BER_BVC("update_anon"),	SLAP_ALLOW_UPDATE_ANON },
		{ BER_BVC("proxy_authz_anon"),	SLAP_ALLOW_PROXY_AUTHZ_ANON },
		{ BER_BVNULL,	0 }
	};
	if (c->op == SLAP_CONFIG_EMIT) {
		return mask_to_verbs( allowable_ops, global_allows, &c->rvalue_vals );
	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( !c->line ) {
			global_allows = 0;
		} else {
			i = verb_to_mask( c->line, allowable_ops );
			global_allows &= ~allowable_ops[i].mask;
		}
		return 0;
	}
	i = verbs_to_mask(c->argc, c->argv, allowable_ops, &allows);
	if ( i ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unknown feature", c->argv[0] );
		Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
			c->log, c->cr_msg, c->argv[i]);
		return(1);
	}
	global_allows |= allows;
	return(0);
}

static int
config_disallows(ConfigArgs *c) {
	slap_mask_t disallows = 0;
	int i;
	slap_verbmasks disallowable_ops[] = {
		{ BER_BVC("bind_anon"),		SLAP_DISALLOW_BIND_ANON },
		{ BER_BVC("bind_simple"),	SLAP_DISALLOW_BIND_SIMPLE },
		{ BER_BVC("tls_2_anon"),		SLAP_DISALLOW_TLS_2_ANON },
		{ BER_BVC("tls_authc"),		SLAP_DISALLOW_TLS_AUTHC },
		{ BER_BVC("proxy_authz_non_critical"),	SLAP_DISALLOW_PROXY_AUTHZ_N_CRIT },
		{ BER_BVC("dontusecopy_non_critical"),	SLAP_DISALLOW_DONTUSECOPY_N_CRIT },
		{ BER_BVNULL, 0 }
	};
	if (c->op == SLAP_CONFIG_EMIT) {
		return mask_to_verbs( disallowable_ops, global_disallows, &c->rvalue_vals );
	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( !c->line ) {
			global_disallows = 0;
		} else {
			i = verb_to_mask( c->line, disallowable_ops );
			global_disallows &= ~disallowable_ops[i].mask;
		}
		return 0;
	}
	i = verbs_to_mask(c->argc, c->argv, disallowable_ops, &disallows);
	if ( i ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unknown feature", c->argv[0] );
		Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
			c->log, c->cr_msg, c->argv[i]);
		return(1);
	}
	global_disallows |= disallows;
	return(0);
}

static int
config_requires(ConfigArgs *c) {
	slap_mask_t requires = frontendDB->be_requires;
	int i, argc = c->argc;
	char **argv = c->argv;

	slap_verbmasks requires_ops[] = {
		{ BER_BVC("bind"),		SLAP_REQUIRE_BIND },
		{ BER_BVC("LDAPv3"),		SLAP_REQUIRE_LDAP_V3 },
		{ BER_BVC("authc"),		SLAP_REQUIRE_AUTHC },
		{ BER_BVC("sasl"),		SLAP_REQUIRE_SASL },
		{ BER_BVC("strong"),		SLAP_REQUIRE_STRONG },
		{ BER_BVNULL, 0 }
	};
	if (c->op == SLAP_CONFIG_EMIT) {
		return mask_to_verbs( requires_ops, c->be->be_requires, &c->rvalue_vals );
	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( !c->line ) {
			c->be->be_requires = 0;
		} else {
			i = verb_to_mask( c->line, requires_ops );
			c->be->be_requires &= ~requires_ops[i].mask;
		}
		return 0;
	}
	/* "none" can only be first, to wipe out default/global values */
	if ( strcasecmp( c->argv[ 1 ], "none" ) == 0 ) {
		argv++;
		argc--;
		requires = 0;
	}
	i = verbs_to_mask(argc, argv, requires_ops, &requires);
	if ( i ) {
		if (strcasecmp( c->argv[ i ], "none" ) == 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> \"none\" (#%d) must be listed first", c->argv[0], i - 1 );
			Debug(LDAP_DEBUG_ANY, "%s: %s\n",
				c->log, c->cr_msg, 0);
		} else {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unknown feature #%d", c->argv[0], i - 1 );
			Debug(LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
				c->log, c->cr_msg, c->argv[i]);
		}
		return(1);
	}
	c->be->be_requires = requires;
	return(0);
}

static int
config_extra_attrs(ConfigArgs *c)
{
	assert( c->be != NULL );

	if ( c->op == SLAP_CONFIG_EMIT ) {
		int i;

		if ( c->be->be_extra_anlist == NULL ) {
			return 1;
		}

		for ( i = 0; !BER_BVISNULL( &c->be->be_extra_anlist[i].an_name ); i++ ) {
			value_add_one( &c->rvalue_vals, &c->be->be_extra_anlist[i].an_name );
		}

	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( c->be->be_extra_anlist == NULL ) {
			return 1;
		}

		if ( c->valx < 0 ) {
			anlist_free( c->be->be_extra_anlist, 1, NULL );
			c->be->be_extra_anlist = NULL;

		} else {
			int i;

			for ( i = 0; i < c->valx && !BER_BVISNULL( &c->be->be_extra_anlist[i + 1].an_name ); i++ )
				;

			if ( BER_BVISNULL( &c->be->be_extra_anlist[i].an_name ) ) {
				return 1;
			}

			ch_free( c->be->be_extra_anlist[i].an_name.bv_val );

			for ( ; !BER_BVISNULL( &c->be->be_extra_anlist[i].an_name ); i++ ) {
				c->be->be_extra_anlist[i] = c->be->be_extra_anlist[i + 1];
			}
		}

	} else {
		c->be->be_extra_anlist = str2anlist( c->be->be_extra_anlist, c->argv[1], " ,\t" );
		if ( c->be->be_extra_anlist == NULL ) {
			return 1;
		}
	}

	return 0;
}

static slap_verbmasks	*loglevel_ops;

static int
loglevel_init( void )
{
	slap_verbmasks	lo[] = {
		{ BER_BVC("Any"),	(slap_mask_t) LDAP_DEBUG_ANY },
		{ BER_BVC("Trace"),	LDAP_DEBUG_TRACE },
		{ BER_BVC("Packets"),	LDAP_DEBUG_PACKETS },
		{ BER_BVC("Args"),	LDAP_DEBUG_ARGS },
		{ BER_BVC("Conns"),	LDAP_DEBUG_CONNS },
		{ BER_BVC("BER"),	LDAP_DEBUG_BER },
		{ BER_BVC("Filter"),	LDAP_DEBUG_FILTER },
		{ BER_BVC("Config"),	LDAP_DEBUG_CONFIG },
		{ BER_BVC("ACL"),	LDAP_DEBUG_ACL },
		{ BER_BVC("Stats"),	LDAP_DEBUG_STATS },
		{ BER_BVC("Stats2"),	LDAP_DEBUG_STATS2 },
		{ BER_BVC("Shell"),	LDAP_DEBUG_SHELL },
		{ BER_BVC("Parse"),	LDAP_DEBUG_PARSE },
#if 0	/* no longer used (nor supported) */
		{ BER_BVC("Cache"),	LDAP_DEBUG_CACHE },
		{ BER_BVC("Index"),	LDAP_DEBUG_INDEX },
#endif
		{ BER_BVC("Sync"),	LDAP_DEBUG_SYNC },
		{ BER_BVC("None"),	LDAP_DEBUG_NONE },
		{ BER_BVNULL,		0 }
	};

	return slap_verbmasks_init( &loglevel_ops, lo );
}

static void
loglevel_destroy( void )
{
	if ( loglevel_ops ) {
		(void)slap_verbmasks_destroy( loglevel_ops );
	}
	loglevel_ops = NULL;
}

static slap_mask_t	loglevel_ignore[] = { -1, 0 };

int
slap_loglevel_register( slap_mask_t m, struct berval *s )
{
	int	rc;

	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	rc = slap_verbmasks_append( &loglevel_ops, m, s, loglevel_ignore );

	if ( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY, "slap_loglevel_register(%lu, \"%s\") failed\n",
			m, s->bv_val, 0 );
	}

	return rc;
}

int
slap_loglevel_get( struct berval *s, int *l )
{
	int		rc;
	slap_mask_t	m, i;

	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	for ( m = 0, i = 1; !BER_BVISNULL( &loglevel_ops[ i ].word ); i++ ) {
		m |= loglevel_ops[ i ].mask;
	}

	for ( i = 1; m & i; i <<= 1 )
		;

	if ( i == 0 ) {
		return -1;
	}

	rc = slap_verbmasks_append( &loglevel_ops, i, s, loglevel_ignore );

	if ( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY, "slap_loglevel_get(%lu, \"%s\") failed\n",
			i, s->bv_val, 0 );

	} else {
		*l = i;
	}

	return rc;
}

int
str2loglevel( const char *s, int *l )
{
	int	i;

	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	i = verb_to_mask( s, loglevel_ops );

	if ( BER_BVISNULL( &loglevel_ops[ i ].word ) ) {
		return -1;
	}

	*l = loglevel_ops[ i ].mask;

	return 0;
}

const char *
loglevel2str( int l )
{
	struct berval	bv = BER_BVNULL;

	loglevel2bv( l, &bv );

	return bv.bv_val;
}

int
loglevel2bv( int l, struct berval *bv )
{
	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	BER_BVZERO( bv );

	return enum_to_verb( loglevel_ops, l, bv ) == -1;
}

int
loglevel2bvarray( int l, BerVarray *bva )
{
	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	if ( l == 0 ) {
		return value_add_one( bva, ber_bvstr( "0" ) );
	}

	return mask_to_verbs( loglevel_ops, l, bva );
}

int
loglevel_print( FILE *out )
{
	int	i;

	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	fprintf( out, "Installed log subsystems:\n\n" );
	for ( i = 0; !BER_BVISNULL( &loglevel_ops[ i ].word ); i++ ) {
		unsigned mask = loglevel_ops[ i ].mask & 0xffffffffUL;
		fprintf( out,
			(mask == ((slap_mask_t) -1 & 0xffffffffUL)
			 ? "\t%-30s (-1, 0xffffffff)\n" : "\t%-30s (%u, 0x%x)\n"),
			loglevel_ops[ i ].word.bv_val, mask, mask );
	}

	fprintf( out, "\nNOTE: custom log subsystems may be later installed "
		"by specific code\n\n" );

	return 0;
}

static int config_syslog;

static int
config_loglevel(ConfigArgs *c) {
	int i;

	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	if (c->op == SLAP_CONFIG_EMIT) {
		/* Get default or commandline slapd setting */
		if ( ldap_syslog && !config_syslog )
			config_syslog = ldap_syslog;
		return loglevel2bvarray( config_syslog, &c->rvalue_vals );

	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( !c->line ) {
			config_syslog = 0;
		} else {
			i = verb_to_mask( c->line, loglevel_ops );
			config_syslog &= ~loglevel_ops[i].mask;
		}
		if ( slapMode & SLAP_SERVER_MODE ) {
			ldap_syslog = config_syslog;
		}
		return 0;
	}

	for( i=1; i < c->argc; i++ ) {
		int	level;

		if ( isdigit((unsigned char)c->argv[i][0]) || c->argv[i][0] == '-' ) {
			if( lutil_atoix( &level, c->argv[i], 0 ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unable to parse level", c->argv[0] );
				Debug( LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
					c->log, c->cr_msg, c->argv[i]);
				return( 1 );
			}
		} else {
			if ( str2loglevel( c->argv[i], &level ) ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unknown level", c->argv[0] );
				Debug( LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
					c->log, c->cr_msg, c->argv[i]);
				return( 1 );
			}
		}
		/* Explicitly setting a zero clears all the levels */
		if ( level )
			config_syslog |= level;
		else
			config_syslog = 0;
	}
	if ( slapMode & SLAP_SERVER_MODE ) {
		ldap_syslog = config_syslog;
	}
	return(0);
}

static int
config_referral(ConfigArgs *c) {
	struct berval val;
	if (c->op == SLAP_CONFIG_EMIT) {
		if ( default_referral ) {
			value_add( &c->rvalue_vals, default_referral );
			return 0;
		} else {
			return 1;
		}
	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( c->valx < 0 ) {
			ber_bvarray_free( default_referral );
			default_referral = NULL;
		} else {
			int i = c->valx;
			ch_free( default_referral[i].bv_val );
			for (; default_referral[i].bv_val; i++ )
				default_referral[i] = default_referral[i+1];
		}
		return 0;
	}
	if(validate_global_referral(c->argv[1])) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> invalid URL", c->argv[0] );
		Debug(LDAP_DEBUG_ANY, "%s: %s (%s)\n",
			c->log, c->cr_msg, c->argv[1]);
		return(1);
	}

	ber_str2bv(c->argv[1], 0, 0, &val);
	if(value_add_one(&default_referral, &val)) return(LDAP_OTHER);
	return(0);
}

static struct {
	struct berval key;
	int off;
} sec_keys[] = {
	{ BER_BVC("ssf="), offsetof(slap_ssf_set_t, sss_ssf) },
	{ BER_BVC("transport="), offsetof(slap_ssf_set_t, sss_transport) },
	{ BER_BVC("tls="), offsetof(slap_ssf_set_t, sss_tls) },
	{ BER_BVC("sasl="), offsetof(slap_ssf_set_t, sss_sasl) },
	{ BER_BVC("update_ssf="), offsetof(slap_ssf_set_t, sss_update_ssf) },
	{ BER_BVC("update_transport="), offsetof(slap_ssf_set_t, sss_update_transport) },
	{ BER_BVC("update_tls="), offsetof(slap_ssf_set_t, sss_update_tls) },
	{ BER_BVC("update_sasl="), offsetof(slap_ssf_set_t, sss_update_sasl) },
	{ BER_BVC("simple_bind="), offsetof(slap_ssf_set_t, sss_simple_bind) },
	{ BER_BVNULL, 0 }
};

static int
config_security(ConfigArgs *c) {
	slap_ssf_set_t *set = &c->be->be_ssf_set;
	char *next;
	int i, j;
	if (c->op == SLAP_CONFIG_EMIT) {
		char numbuf[32];
		struct berval bv;
		slap_ssf_t *tgt;
		int rc = 1;

		for (i=0; !BER_BVISNULL( &sec_keys[i].key ); i++) {
			tgt = (slap_ssf_t *)((char *)set + sec_keys[i].off);
			if ( *tgt ) {
				rc = 0;
				bv.bv_len = snprintf( numbuf, sizeof( numbuf ), "%u", *tgt );
				if ( bv.bv_len >= sizeof( numbuf ) ) {
					ber_bvarray_free_x( c->rvalue_vals, NULL );
					c->rvalue_vals = NULL;
					rc = 1;
					break;
				}
				bv.bv_len += sec_keys[i].key.bv_len;
				bv.bv_val = ch_malloc( bv.bv_len + 1);
				next = lutil_strcopy( bv.bv_val, sec_keys[i].key.bv_val );
				strcpy( next, numbuf );
				ber_bvarray_add( &c->rvalue_vals, &bv );
			}
		}
		return rc;
	}
	for(i = 1; i < c->argc; i++) {
		slap_ssf_t *tgt = NULL;
		char *src = NULL;
		for ( j=0; !BER_BVISNULL( &sec_keys[j].key ); j++ ) {
			if(!strncasecmp(c->argv[i], sec_keys[j].key.bv_val,
				sec_keys[j].key.bv_len)) {
				src = c->argv[i] + sec_keys[j].key.bv_len;
				tgt = (slap_ssf_t *)((char *)set + sec_keys[j].off);
				break;
			}
		}
		if ( !tgt ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unknown factor", c->argv[0] );
			Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
				c->log, c->cr_msg, c->argv[i]);
			return(1);
		}

		if ( lutil_atou( tgt, src ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unable to parse factor", c->argv[0] );
			Debug(LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
				c->log, c->cr_msg, c->argv[i]);
			return(1);
		}
	}
	return(0);
}

char *
anlist_unparse( AttributeName *an, char *ptr, ber_len_t buflen ) {
	int comma = 0;
	char *start = ptr;

	for (; !BER_BVISNULL( &an->an_name ); an++) {
		/* if buflen == 0, assume the buffer size has been 
		 * already checked otherwise */
		if ( buflen > 0 && buflen - ( ptr - start ) < comma + an->an_name.bv_len ) return NULL;
		if ( comma ) *ptr++ = ',';
		ptr = lutil_strcopy( ptr, an->an_name.bv_val );
		comma = 1;
	}
	return ptr;
}

static int
config_updatedn(ConfigArgs *c) {
	if (c->op == SLAP_CONFIG_EMIT) {
		if (!BER_BVISEMPTY(&c->be->be_update_ndn)) {
			value_add_one(&c->rvalue_vals, &c->be->be_update_ndn);
			value_add_one(&c->rvalue_nvals, &c->be->be_update_ndn);
			return 0;
		}
		return 1;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		ch_free( c->be->be_update_ndn.bv_val );
		BER_BVZERO( &c->be->be_update_ndn );
		SLAP_DBFLAGS(c->be) ^= (SLAP_DBFLAG_SHADOW | SLAP_DBFLAG_SLURP_SHADOW);
		return 0;
	}
	if(SLAP_SHADOW(c->be)) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> database already shadowed", c->argv[0] );
		Debug(LDAP_DEBUG_ANY, "%s: %s\n",
			c->log, c->cr_msg, 0);
		return(1);
	}

	ber_memfree_x( c->value_dn.bv_val, NULL );
	if ( !BER_BVISNULL( &c->be->be_update_ndn ) ) {
		ber_memfree_x( c->be->be_update_ndn.bv_val, NULL );
	}
	c->be->be_update_ndn = c->value_ndn;
	BER_BVZERO( &c->value_dn );
	BER_BVZERO( &c->value_ndn );

	return config_slurp_shadow( c );
}

int
config_shadow( ConfigArgs *c, slap_mask_t flag )
{
	char	*notallowed = NULL;

	if ( c->be == frontendDB ) {
		notallowed = "frontend";

	} else if ( SLAP_MONITOR(c->be) ) {
		notallowed = "monitor";
	}

	if ( notallowed != NULL ) {
		Debug( LDAP_DEBUG_ANY, "%s: %s database cannot be shadow.\n", c->log, notallowed, 0 );
		return 1;
	}

	if ( SLAP_SHADOW(c->be) ) {
		/* if already shadow, only check consistency */
		if ( ( SLAP_DBFLAGS(c->be) & flag ) != flag ) {
			Debug( LDAP_DEBUG_ANY, "%s: inconsistent shadow flag 0x%lx.\n",
				c->log, flag, 0 );
			return 1;
		}

	} else {
		SLAP_DBFLAGS(c->be) |= (SLAP_DBFLAG_SHADOW | flag);
		if ( !SLAP_MULTIMASTER( c->be ))
			SLAP_DBFLAGS(c->be) |= SLAP_DBFLAG_SINGLE_SHADOW;
	}

	return 0;
}

static int
config_updateref(ConfigArgs *c) {
	struct berval val;
	if (c->op == SLAP_CONFIG_EMIT) {
		if ( c->be->be_update_refs ) {
			value_add( &c->rvalue_vals, c->be->be_update_refs );
			return 0;
		} else {
			return 1;
		}
	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( c->valx < 0 ) {
			ber_bvarray_free( c->be->be_update_refs );
			c->be->be_update_refs = NULL;
		} else {
			int i = c->valx;
			ch_free( c->be->be_update_refs[i].bv_val );
			for (; c->be->be_update_refs[i].bv_val; i++)
				c->be->be_update_refs[i] = c->be->be_update_refs[i+1];
		}
		return 0;
	}
	if(!SLAP_SHADOW(c->be) && !c->be->be_syncinfo) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> must appear after syncrepl or updatedn",
			c->argv[0] );
		Debug(LDAP_DEBUG_ANY, "%s: %s\n",
			c->log, c->cr_msg, 0);
		return(1);
	}

	if(validate_global_referral(c->argv[1])) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> invalid URL", c->argv[0] );
		Debug(LDAP_DEBUG_ANY, "%s: %s (%s)\n",
			c->log, c->cr_msg, c->argv[1]);
		return(1);
	}
	ber_str2bv(c->argv[1], 0, 0, &val);
	if(value_add_one(&c->be->be_update_refs, &val)) return(LDAP_OTHER);
	return(0);
}

static int
config_obsolete(ConfigArgs *c) {
	if (c->op == SLAP_CONFIG_EMIT)
		return 1;

	snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> keyword is obsolete (ignored)",
		c->argv[0] );
	Debug(LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg, 0);
	return(0);
}

static int
config_include(ConfigArgs *c) {
	int savelineno = c->lineno;
	int rc;
	ConfigFile *cf;
	ConfigFile *cfsave = cfn;
	ConfigFile *cf2 = NULL;

	/* Leftover from RE23. No dynamic config for include files */
	if ( c->op == SLAP_CONFIG_EMIT || c->op == LDAP_MOD_DELETE )
		return 1;

	cf = ch_calloc( 1, sizeof(ConfigFile));
	if ( cfn->c_kids ) {
		for (cf2=cfn->c_kids; cf2 && cf2->c_sibs; cf2=cf2->c_sibs) ;
		cf2->c_sibs = cf;
	} else {
		cfn->c_kids = cf;
	}
	cfn = cf;
	ber_str2bv( c->argv[1], 0, 1, &cf->c_file );
	rc = read_config_file(c->argv[1], c->depth + 1, c, config_back_cf_table);
	c->lineno = savelineno - 1;
	cfn = cfsave;
	if ( rc ) {
		if ( cf2 ) cf2->c_sibs = NULL;
		else cfn->c_kids = NULL;
		ch_free( cf->c_file.bv_val );
		ch_free( cf );
	} else {
		c->ca_private = cf;
	}
	return(rc);
}

#ifdef HAVE_TLS
static int
config_tls_cleanup(ConfigArgs *c) {
	int rc = 0;

	if ( slap_tls_ld ) {
		int opt = 1;

		ldap_pvt_tls_ctx_free( slap_tls_ctx );
		slap_tls_ctx = NULL;

		/* Force new ctx to be created */
		rc = ldap_pvt_tls_set_option( slap_tls_ld, LDAP_OPT_X_TLS_NEWCTX, &opt );
		if( rc == 0 ) {
			/* The ctx's refcount is bumped up here */
			ldap_pvt_tls_get_option( slap_tls_ld, LDAP_OPT_X_TLS_CTX, &slap_tls_ctx );
			/* This is a no-op if it's already loaded */
			load_extop( &slap_EXOP_START_TLS, 0, starttls_extop );
		} else {
			if ( rc == LDAP_NOT_SUPPORTED )
				rc = LDAP_UNWILLING_TO_PERFORM;
			else
				rc = LDAP_OTHER;
		}
	}
	return rc;
}

static int
config_tls_option(ConfigArgs *c) {
	int flag;
	LDAP *ld = slap_tls_ld;
	switch(c->type) {
	case CFG_TLS_RAND:	flag = LDAP_OPT_X_TLS_RANDOM_FILE;	ld = NULL; break;
	case CFG_TLS_CIPHER:	flag = LDAP_OPT_X_TLS_CIPHER_SUITE;	break;
	case CFG_TLS_CERT_FILE:	flag = LDAP_OPT_X_TLS_CERTFILE;		break;	
	case CFG_TLS_CERT_KEY:	flag = LDAP_OPT_X_TLS_KEYFILE;		break;
	case CFG_TLS_CA_PATH:	flag = LDAP_OPT_X_TLS_CACERTDIR;	break;
	case CFG_TLS_CA_FILE:	flag = LDAP_OPT_X_TLS_CACERTFILE;	break;
	case CFG_TLS_DH_FILE:	flag = LDAP_OPT_X_TLS_DHFILE;	break;
	case CFG_TLS_ECNAME:	flag = LDAP_OPT_X_TLS_ECNAME;	break;
#ifdef HAVE_GNUTLS
	case CFG_TLS_CRL_FILE:	flag = LDAP_OPT_X_TLS_CRLFILE;	break;
#endif
	default:		Debug(LDAP_DEBUG_ANY, "%s: "
					"unknown tls_option <0x%x>\n",
					c->log, c->type, 0);
		return 1;
	}
	if (c->op == SLAP_CONFIG_EMIT) {
		return ldap_pvt_tls_get_option( ld, flag, &c->value_string );
	} else if ( c->op == LDAP_MOD_DELETE ) {
		c->cleanup = config_tls_cleanup;
		return ldap_pvt_tls_set_option( ld, flag, NULL );
	}
	ch_free(c->value_string);
	c->cleanup = config_tls_cleanup;
	return(ldap_pvt_tls_set_option(ld, flag, c->argv[1]));
}

/* FIXME: this ought to be provided by libldap */
static int
config_tls_config(ConfigArgs *c) {
	int i, flag;
	switch(c->type) {
	case CFG_TLS_CRLCHECK:	flag = LDAP_OPT_X_TLS_CRLCHECK; break;
	case CFG_TLS_VERIFY:	flag = LDAP_OPT_X_TLS_REQUIRE_CERT; break;
	case CFG_TLS_PROTOCOL_MIN: flag = LDAP_OPT_X_TLS_PROTOCOL_MIN; break;
	default:
		Debug(LDAP_DEBUG_ANY, "%s: "
				"unknown tls_option <0x%x>\n",
				c->log, c->type, 0);
		return 1;
	}
	if (c->op == SLAP_CONFIG_EMIT) {
		return slap_tls_get_config( slap_tls_ld, flag, &c->value_string );
	} else if ( c->op == LDAP_MOD_DELETE ) {
		int i = 0;
		c->cleanup = config_tls_cleanup;
		return ldap_pvt_tls_set_option( slap_tls_ld, flag, &i );
	}
	ch_free( c->value_string );
	c->cleanup = config_tls_cleanup;
	if ( isdigit( (unsigned char)c->argv[1][0] ) && c->type != CFG_TLS_PROTOCOL_MIN ) {
		if ( lutil_atoi( &i, c->argv[1] ) != 0 ) {
			Debug(LDAP_DEBUG_ANY, "%s: "
				"unable to parse %s \"%s\"\n",
				c->log, c->argv[0], c->argv[1] );
			return 1;
		}
		return(ldap_pvt_tls_set_option(slap_tls_ld, flag, &i));
	} else {
		return(ldap_pvt_tls_config(slap_tls_ld, flag, c->argv[1]));
	}
}
#endif

static CfEntryInfo *
config_find_base( CfEntryInfo *root, struct berval *dn, CfEntryInfo **last )
{
	struct berval cdn;
	char *c;

	if ( !root ) {
		*last = NULL;
		return NULL;
	}

	if ( dn_match( &root->ce_entry->e_nname, dn ))
		return root;

	c = dn->bv_val+dn->bv_len;
	for (;*c != ',';c--);

	while(root) {
		*last = root;
		for (--c;c>dn->bv_val && *c != ',';c--);
		cdn.bv_val = c;
		if ( *c == ',' )
			cdn.bv_val++;
		cdn.bv_len = dn->bv_len - (cdn.bv_val - dn->bv_val);

		root = root->ce_kids;

		for (;root;root=root->ce_sibs) {
			if ( dn_match( &root->ce_entry->e_nname, &cdn )) {
				if ( cdn.bv_val == dn->bv_val ) {
					return root;
				}
				break;
			}
		}
	}
	return root;
}

typedef struct setup_cookie {
	CfBackInfo *cfb;
	ConfigArgs *ca;
	Entry *frontend;
	Entry *config;
	int got_frontend;
	int got_config;
} setup_cookie;

static int
config_ldif_resp( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		setup_cookie *sc = op->o_callback->sc_private;
		struct berval pdn;

		sc->cfb->cb_got_ldif = 1;
		/* Does the frontend exist? */
		if ( !sc->got_frontend ) {
			if ( !strncmp( rs->sr_entry->e_nname.bv_val,
				"olcDatabase", STRLENOF( "olcDatabase" )))
			{
				if ( strncmp( rs->sr_entry->e_nname.bv_val +
					STRLENOF( "olcDatabase" ), "={-1}frontend",
					STRLENOF( "={-1}frontend" )))
				{
					struct berval rdn;
					int i = op->o_noop;
					sc->ca->be = frontendDB;
					sc->ca->bi = frontendDB->bd_info;
					frontendDB->be_cf_ocs = &CFOC_FRONTEND;
					rdn.bv_val = sc->ca->log;
					rdn.bv_len = snprintf(rdn.bv_val, sizeof( sc->ca->log ),
						"%s=" SLAP_X_ORDERED_FMT "%s",
						cfAd_database->ad_cname.bv_val, -1,
						sc->ca->bi->bi_type);
					op->o_noop = 1;
					sc->frontend = config_build_entry( op, rs,
						sc->cfb->cb_root, sc->ca, &rdn, &CFOC_DATABASE,
						sc->ca->be->be_cf_ocs );
					op->o_noop = i;
					sc->got_frontend++;
				} else {
					sc->got_frontend++;
					goto ok;
				}
			}
		}

		dnParent( &rs->sr_entry->e_nname, &pdn );

		/* Does the configDB exist? */
		if ( sc->got_frontend && !sc->got_config &&
			!strncmp( rs->sr_entry->e_nname.bv_val,
			"olcDatabase", STRLENOF( "olcDatabase" )) &&
			dn_match( &config_rdn, &pdn ) )
		{
			if ( strncmp( rs->sr_entry->e_nname.bv_val +
				STRLENOF( "olcDatabase" ), "={0}config",
				STRLENOF( "={0}config" )))
			{
				struct berval rdn;
				int i = op->o_noop;
				sc->ca->be = LDAP_STAILQ_FIRST( &backendDB );
				sc->ca->bi = sc->ca->be->bd_info;
				rdn.bv_val = sc->ca->log;
				rdn.bv_len = snprintf(rdn.bv_val, sizeof( sc->ca->log ),
					"%s=" SLAP_X_ORDERED_FMT "%s",
					cfAd_database->ad_cname.bv_val, 0,
					sc->ca->bi->bi_type);
				op->o_noop = 1;
				sc->config = config_build_entry( op, rs, sc->cfb->cb_root,
					sc->ca, &rdn, &CFOC_DATABASE, sc->ca->be->be_cf_ocs );
				op->o_noop = i;
			}
			sc->got_config++;
		}

ok:
		rs->sr_err = config_add_internal( sc->cfb, rs->sr_entry, sc->ca, NULL, NULL, NULL );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "config error processing %s: %s\n",
				rs->sr_entry->e_name.bv_val, sc->ca->cr_msg, 0 );
		}
	}
	return rs->sr_err;
}

/* Configure and read the underlying back-ldif store */
static int
config_setup_ldif( BackendDB *be, const char *dir, int readit ) {
	CfBackInfo *cfb = be->be_private;
	ConfigArgs c = {0};
	ConfigTable *ct;
	char *argv[3];
	int rc = 0;
	setup_cookie sc;
	slap_callback cb = { NULL, config_ldif_resp, NULL, NULL };
	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;
	SlapReply rs = {REP_RESULT};
	Filter filter = { LDAP_FILTER_PRESENT };
	struct berval filterstr = BER_BVC("(objectclass=*)");
	struct stat st;

	/* Is the config directory available? */
	if ( stat( dir, &st ) < 0 ) {
		/* No, so don't bother using the backing store.
		 * All changes will be in-memory only.
		 */
		return 0;
	}
		
	cfb->cb_db.bd_info = backend_info( "ldif" );
	if ( !cfb->cb_db.bd_info )
		return 0;	/* FIXME: eventually this will be a fatal error */

	if ( backend_db_init( "ldif", &cfb->cb_db, -1, NULL ) == NULL )
		return 1;

	cfb->cb_db.be_suffix = be->be_suffix;
	cfb->cb_db.be_nsuffix = be->be_nsuffix;

	/* The suffix is always "cn=config". The underlying DB's rootdn
	 * is always the same as the suffix.
	 */
	cfb->cb_db.be_rootdn = be->be_suffix[0];
	cfb->cb_db.be_rootndn = be->be_nsuffix[0];

	ber_str2bv( dir, 0, 1, &cfdir );

	c.be = &cfb->cb_db;
	c.fname = "slapd";
	c.argc = 2;
	argv[0] = "directory";
	argv[1] = (char *)dir;
	argv[2] = NULL;
	c.argv = argv;
	c.reply.err = 0;
	c.reply.msg[0] = 0;
	c.table = Cft_Database;

	ct = config_find_keyword( c.be->be_cf_ocs->co_table, &c );
	if ( !ct )
		return 1;

	if ( config_add_vals( ct, &c ))
		return 1;

	if ( backend_startup_one( &cfb->cb_db, &c.reply ))
		return 1;

	if ( readit ) {
		void *thrctx = ldap_pvt_thread_pool_context();
		int prev_DN_strict;

		connection_fake_init( &conn, &opbuf, thrctx );
		op = &opbuf.ob_op;

		filter.f_desc = slap_schema.si_ad_objectClass;

		op->o_tag = LDAP_REQ_SEARCH;

		op->ors_filter = &filter;
		op->ors_filterstr = filterstr;
		op->ors_scope = LDAP_SCOPE_SUBTREE;

		op->o_dn = c.be->be_rootdn;
		op->o_ndn = c.be->be_rootndn;

		op->o_req_dn = be->be_suffix[0];
		op->o_req_ndn = be->be_nsuffix[0];

		op->ors_tlimit = SLAP_NO_LIMIT;
		op->ors_slimit = SLAP_NO_LIMIT;

		op->ors_attrs = slap_anlist_all_attributes;
		op->ors_attrsonly = 0;

		op->o_callback = &cb;
		sc.cfb = cfb;
		sc.ca = &c;
		cb.sc_private = &sc;
		sc.got_frontend = 0;
		sc.got_config = 0;
		sc.frontend = NULL;
		sc.config = NULL;

		op->o_bd = &cfb->cb_db;
		
		/* Allow unknown attrs in DNs */
		prev_DN_strict = slap_DN_strict;
		slap_DN_strict = 0;

		rc = op->o_bd->be_search( op, &rs );

		/* Restore normal DN validation */
		slap_DN_strict = prev_DN_strict;

		op->o_tag = LDAP_REQ_ADD;
		if ( rc == LDAP_SUCCESS && sc.frontend ) {
			rs_reinit( &rs, REP_RESULT );
			op->ora_e = sc.frontend;
			rc = op->o_bd->be_add( op, &rs );
		}
		if ( rc == LDAP_SUCCESS && sc.config ) {
			rs_reinit( &rs, REP_RESULT );
			op->ora_e = sc.config;
			rc = op->o_bd->be_add( op, &rs );
		}
		ldap_pvt_thread_pool_context_reset( thrctx );
	}

	/* ITS#4194 - only use if it's present, or we're converting. */
	if ( !readit || rc == LDAP_SUCCESS )
		cfb->cb_use_ldif = 1;

	return rc;
}

static int
CfOc_cmp( const void *c1, const void *c2 ) {
	const ConfigOCs *co1 = c1;
	const ConfigOCs *co2 = c2;

	return ber_bvcmp( co1->co_name, co2->co_name );
}

int
config_register_schema(ConfigTable *ct, ConfigOCs *ocs) {
	int i;

	i = init_config_attrs( ct );
	if ( i ) return i;

	/* set up the objectclasses */
	i = init_config_ocs( ocs );
	if ( i ) return i;

	for (i=0; ocs[i].co_def; i++) {
		if ( ocs[i].co_oc ) {
			ocs[i].co_name = &ocs[i].co_oc->soc_cname;
			if ( !ocs[i].co_table )
				ocs[i].co_table = ct;
			avl_insert( &CfOcTree, &ocs[i], CfOc_cmp, avl_dup_error );
		}
	}
	return 0;
}

int
read_config(const char *fname, const char *dir) {
	BackendDB *be;
	CfBackInfo *cfb;
	const char *cfdir, *cfname;
	int rc;

	/* Setup the config backend */
	be = backend_db_init( "config", NULL, 0, NULL );
	if ( !be )
		return 1;

	cfb = be->be_private;
	be->be_dfltaccess = ACL_NONE;

	/* If no .conf, or a dir was specified, setup the dir */
	if ( !fname || dir ) {
		if ( dir ) {
			/* If explicitly given, check for existence */
			struct stat st;

			if ( stat( dir, &st ) < 0 ) {
				Debug( LDAP_DEBUG_ANY,
					"invalid config directory %s, error %d\n",
						dir, errno, 0 );
				return 1;
			}
			cfdir = dir;
		} else {
			cfdir = SLAPD_DEFAULT_CONFIGDIR;
		}
		/* if fname is defaulted, try reading .d */
		rc = config_setup_ldif( be, cfdir, !fname );

		if ( rc ) {
			/* It may be OK if the base object doesn't exist yet. */
			if ( rc != LDAP_NO_SUCH_OBJECT )
				return 1;
			/* ITS#4194: But if dir was specified and no fname,
			 * then we were supposed to read the dir. Unless we're
			 * trying to slapadd the dir...
			 */
			if ( dir && !fname ) {
				if ( slapMode & (SLAP_SERVER_MODE|SLAP_TOOL_READMAIN|SLAP_TOOL_READONLY))
					return 1;
				/* Assume it's slapadd with a config dir, let it continue */
				rc = 0;
				cfb->cb_got_ldif = 1;
				cfb->cb_use_ldif = 1;
				goto done;
			}
		}

		/* If we read the config from back-ldif, nothing to do here */
		if ( cfb->cb_got_ldif ) {
			rc = 0;
			goto done;
		}
	}

	if ( fname )
		cfname = fname;
	else
		cfname = SLAPD_DEFAULT_CONFIGFILE;

	rc = read_config_file(cfname, 0, NULL, config_back_cf_table);

	if ( rc == 0 )
		ber_str2bv( cfname, 0, 1, &cfb->cb_config->c_file );

done:
	if ( rc == 0 && BER_BVISNULL( &frontendDB->be_schemadn ) ) {
		ber_str2bv( SLAPD_SCHEMA_DN, STRLENOF( SLAPD_SCHEMA_DN ), 1,
			&frontendDB->be_schemadn );
		rc = dnNormalize( 0, NULL, NULL, &frontendDB->be_schemadn, &frontendDB->be_schemandn, NULL );
		if ( rc != LDAP_SUCCESS ) {
			Debug(LDAP_DEBUG_ANY, "read_config: "
				"unable to normalize default schema DN \"%s\"\n",
				frontendDB->be_schemadn.bv_val, 0, 0 );
			/* must not happen */
			assert( 0 );
		}
	}
	if ( rc == 0 && ( slapMode & SLAP_SERVER_MODE ) && sid_list ) {
		if ( !BER_BVISEMPTY( &sid_list->si_url ) && !sid_set ) {
			Debug(LDAP_DEBUG_ANY, "read_config: no serverID / URL match found. "
				"Check slapd -h arguments.\n", 0,0,0 );
			rc = LDAP_OTHER;
		}
	}
	return rc;
}

static int
config_back_bind( Operation *op, SlapReply *rs )
{
	if ( be_isroot_pw( op ) ) {
		ber_dupbv( &op->orb_edn, be_root_dn( op->o_bd ));
		/* frontend sends result */
		return LDAP_SUCCESS;
	}

	rs->sr_err = LDAP_INVALID_CREDENTIALS;
	send_ldap_result( op, rs );

	return rs->sr_err;
}

static int
config_send( Operation *op, SlapReply *rs, CfEntryInfo *ce, int depth )
{
	int rc = 0;

	if ( test_filter( op, ce->ce_entry, op->ors_filter ) == LDAP_COMPARE_TRUE )
	{
		rs->sr_attrs = op->ors_attrs;
		rs->sr_entry = ce->ce_entry;
		rs->sr_flags = 0;
		rc = send_search_entry( op, rs );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}
	}
	if ( op->ors_scope == LDAP_SCOPE_SUBTREE ) {
		if ( ce->ce_kids ) {
			rc = config_send( op, rs, ce->ce_kids, 1 );
			if ( rc ) return rc;
		}
		if ( depth ) {
			for (ce=ce->ce_sibs; ce; ce=ce->ce_sibs) {
				rc = config_send( op, rs, ce, 0 );
				if ( rc ) break;
			}
		}
	}
	return rc;
}

static ConfigTable *
config_find_table( ConfigOCs **colst, int nocs, AttributeDescription *ad,
	ConfigArgs *ca )
{
	int i, j;

	for (j=0; j<nocs; j++) {
		for (i=0; colst[j]->co_table[i].name; i++)
			if ( colst[j]->co_table[i].ad == ad ) {
				ca->table = colst[j]->co_type;
				return &colst[j]->co_table[i];
			}
	}
	return NULL;
}

/* Sort the attributes of the entry according to the order defined
 * in the objectclass, with required attributes occurring before
 * allowed attributes. For any attributes with sequencing dependencies
 * (e.g., rootDN must be defined after suffix) the objectclass must
 * list the attributes in the desired sequence.
 */
static void
sort_attrs( Entry *e, ConfigOCs **colst, int nocs )
{
	Attribute *a, *head = NULL, *tail = NULL, **prev;
	int i, j;

	for (i=0; i<nocs; i++) {
		if ( colst[i]->co_oc->soc_required ) {
			AttributeType **at = colst[i]->co_oc->soc_required;
			for (j=0; at[j]; j++) {
				for (a=e->e_attrs, prev=&e->e_attrs; a;
					prev = &(*prev)->a_next, a=a->a_next) {
					if ( a->a_desc == at[j]->sat_ad ) {
						*prev = a->a_next;
						if (!head) {
							head = a;
							tail = a;
						} else {
							tail->a_next = a;
							tail = a;
						}
						break;
					}
				}
			}
		}
		if ( colst[i]->co_oc->soc_allowed ) {
			AttributeType **at = colst[i]->co_oc->soc_allowed;
			for (j=0; at[j]; j++) {
				for (a=e->e_attrs, prev=&e->e_attrs; a;
					prev = &(*prev)->a_next, a=a->a_next) {
					if ( a->a_desc == at[j]->sat_ad ) {
						*prev = a->a_next;
						if (!head) {
							head = a;
							tail = a;
						} else {
							tail->a_next = a;
							tail = a;
						}
						break;
					}
				}
			}
		}
	}
	if ( tail ) {
		tail->a_next = e->e_attrs;
		e->e_attrs = head;
	}
}

static int
check_vals( ConfigTable *ct, ConfigArgs *ca, void *ptr, int isAttr )
{
	Attribute *a = NULL;
	AttributeDescription *ad;
	BerVarray vals;

	int i, rc = 0;

	if ( isAttr ) {
		a = ptr;
		ad = a->a_desc;
		vals = a->a_vals;
	} else {
		Modifications *ml = ptr;
		ad = ml->sml_desc;
		vals = ml->sml_values;
	}

	if ( a && ( ad->ad_type->sat_flags & SLAP_AT_ORDERED_VAL )) {
		rc = ordered_value_sort( a, 1 );
		if ( rc ) {
			snprintf(ca->cr_msg, sizeof( ca->cr_msg ), "ordered_value_sort failed on attr %s\n",
				ad->ad_cname.bv_val );
			return rc;
		}
	}
	for ( i=0; vals[i].bv_val; i++ ) {
		ca->line = vals[i].bv_val;
		if (( ad->ad_type->sat_flags & SLAP_AT_ORDERED_VAL ) &&
			ca->line[0] == '{' ) {
			char *idx = strchr( ca->line, '}' );
			if ( idx ) ca->line = idx+1;
		}
		rc = config_parse_vals( ct, ca, i );
		if ( rc ) {
			break;
		}
	}
	return rc;
}

static int
config_rename_attr( SlapReply *rs, Entry *e, struct berval *rdn,
	Attribute **at )
{
	struct berval rtype, rval;
	Attribute *a;
	AttributeDescription *ad = NULL;

	dnRdn( &e->e_name, rdn );
	rval.bv_val = strchr(rdn->bv_val, '=' ) + 1;
	rval.bv_len = rdn->bv_len - (rval.bv_val - rdn->bv_val);
	rtype.bv_val = rdn->bv_val;
	rtype.bv_len = rval.bv_val - rtype.bv_val - 1;

	/* Find attr */
	slap_bv2ad( &rtype, &ad, &rs->sr_text );
	a = attr_find( e->e_attrs, ad );
	if (!a ) return LDAP_NAMING_VIOLATION;
	*at = a;

	return 0;
}

static void
config_rename_kids( CfEntryInfo *ce )
{
	CfEntryInfo *ce2;
	struct berval rdn, nrdn;

	for (ce2 = ce->ce_kids; ce2; ce2 = ce2->ce_sibs) {
		struct berval newdn, newndn;
		dnRdn ( &ce2->ce_entry->e_name, &rdn );
		dnRdn ( &ce2->ce_entry->e_nname, &nrdn );
		build_new_dn( &newdn, &ce->ce_entry->e_name, &rdn, NULL );
		build_new_dn( &newndn, &ce->ce_entry->e_nname, &nrdn, NULL );
		free( ce2->ce_entry->e_name.bv_val );
		free( ce2->ce_entry->e_nname.bv_val );
		ce2->ce_entry->e_name = newdn;
		ce2->ce_entry->e_nname = newndn;
		config_rename_kids( ce2 );
	}
}

static int
config_rename_one( Operation *op, SlapReply *rs, Entry *e,
	CfEntryInfo *parent, Attribute *a, struct berval *newrdn,
	struct berval *nnewrdn, int use_ldif )
{
	char *ptr1;
	int rc = 0;
	struct berval odn, ondn;

	odn = e->e_name;
	ondn = e->e_nname;
	build_new_dn( &e->e_name, &parent->ce_entry->e_name, newrdn, NULL );
	build_new_dn( &e->e_nname, &parent->ce_entry->e_nname, nnewrdn, NULL );

	/* Replace attr */
	free( a->a_vals[0].bv_val );
	ptr1 = strchr( newrdn->bv_val, '=' ) + 1;
	a->a_vals[0].bv_len = newrdn->bv_len - (ptr1 - newrdn->bv_val);
	a->a_vals[0].bv_val = ch_malloc( a->a_vals[0].bv_len + 1 );
	strcpy( a->a_vals[0].bv_val, ptr1 );

	if ( a->a_nvals != a->a_vals ) {
		free( a->a_nvals[0].bv_val );
		ptr1 = strchr( nnewrdn->bv_val, '=' ) + 1;
		a->a_nvals[0].bv_len = nnewrdn->bv_len - (ptr1 - nnewrdn->bv_val);
		a->a_nvals[0].bv_val = ch_malloc( a->a_nvals[0].bv_len + 1 );
		strcpy( a->a_nvals[0].bv_val, ptr1 );
	}
	if ( use_ldif ) {
		CfBackInfo *cfb = (CfBackInfo *)op->o_bd->be_private;
		BackendDB *be = op->o_bd;
		slap_callback sc = { NULL, slap_null_cb, NULL, NULL }, *scp;
		struct berval dn, ndn, xdn, xndn;

		op->o_bd = &cfb->cb_db;

		/* Save current rootdn; use the underlying DB's rootdn */
		dn = op->o_dn;
		ndn = op->o_ndn;
		xdn = op->o_req_dn;
		xndn = op->o_req_ndn;
		op->o_dn = op->o_bd->be_rootdn;
		op->o_ndn = op->o_bd->be_rootndn;
		op->o_req_dn = odn;
		op->o_req_ndn = ondn;

		scp = op->o_callback;
		op->o_callback = &sc;
		op->orr_newrdn = *newrdn;
		op->orr_nnewrdn = *nnewrdn;
		op->orr_newSup = NULL;
		op->orr_nnewSup = NULL;
		op->orr_deleteoldrdn = 1;
		op->orr_modlist = NULL;
		slap_modrdn2mods( op, rs );
		slap_mods_opattrs( op, &op->orr_modlist, 1 );
		rc = op->o_bd->be_modrdn( op, rs );
		slap_mods_free( op->orr_modlist, 1 );

		op->o_bd = be;
		op->o_callback = scp;
		op->o_dn = dn;
		op->o_ndn = ndn;
		op->o_req_dn = xdn;
		op->o_req_ndn = xndn;
	}
	free( odn.bv_val );
	free( ondn.bv_val );
	if ( e->e_private )
		config_rename_kids( e->e_private );
	return rc;
}

static int
config_renumber_one( Operation *op, SlapReply *rs, CfEntryInfo *parent, 
	Entry *e, int idx, int tailindex, int use_ldif )
{
	struct berval ival, newrdn, nnewrdn;
	struct berval rdn;
	Attribute *a;
	char ibuf[32], *ptr1, *ptr2 = NULL;
	int rc = 0;

	rc = config_rename_attr( rs, e, &rdn, &a );
	if ( rc ) return rc;

	ival.bv_val = ibuf;
	ival.bv_len = snprintf( ibuf, sizeof( ibuf ), SLAP_X_ORDERED_FMT, idx );
	if ( ival.bv_len >= sizeof( ibuf ) ) {
		return LDAP_NAMING_VIOLATION;
	}
	
	newrdn.bv_len = rdn.bv_len + ival.bv_len;
	newrdn.bv_val = ch_malloc( newrdn.bv_len+1 );

	if ( tailindex ) {
		ptr1 = lutil_strncopy( newrdn.bv_val, rdn.bv_val, rdn.bv_len );
		ptr1 = lutil_strcopy( ptr1, ival.bv_val );
	} else {
		int xlen;
		ptr2 = ber_bvchr( &rdn, '}' );
		if ( ptr2 ) {
			ptr2++;
		} else {
			ptr2 = rdn.bv_val + a->a_desc->ad_cname.bv_len + 1;
		}
		xlen = rdn.bv_len - (ptr2 - rdn.bv_val);
		ptr1 = lutil_strncopy( newrdn.bv_val, a->a_desc->ad_cname.bv_val,
			a->a_desc->ad_cname.bv_len );
		*ptr1++ = '=';
		ptr1 = lutil_strcopy( ptr1, ival.bv_val );
		ptr1 = lutil_strncopy( ptr1, ptr2, xlen );
		*ptr1 = '\0';
	}

	/* Do the equivalent of ModRDN */
	/* Replace DN / NDN */
	newrdn.bv_len = ptr1 - newrdn.bv_val;
	rdnNormalize( 0, NULL, NULL, &newrdn, &nnewrdn, NULL );
	rc = config_rename_one( op, rs, e, parent, a, &newrdn, &nnewrdn, use_ldif );

	free( nnewrdn.bv_val );
	free( newrdn.bv_val );
	return rc;
}

static int
check_name_index( CfEntryInfo *parent, ConfigType ce_type, Entry *e,
	SlapReply *rs, int *renum, int *ibase )
{
	CfEntryInfo *ce;
	int index = -1, gotindex = 0, nsibs, rc = 0;
	int renumber = 0, tailindex = 0, isfrontend = 0, isconfig = 0;
	char *ptr1, *ptr2 = NULL;
	struct berval rdn;

	if ( renum ) *renum = 0;

	/* These entries don't get indexed/renumbered */
	if ( ce_type == Cft_Global ) return 0;
	if ( ce_type == Cft_Schema && parent->ce_type == Cft_Global ) return 0;

	if ( ce_type == Cft_Module )
		tailindex = 1;

	/* See if the rdn has an index already */
	dnRdn( &e->e_name, &rdn );
	if ( ce_type == Cft_Database ) {
		if ( !strncmp( rdn.bv_val + rdn.bv_len - STRLENOF("frontend"),
				"frontend", STRLENOF("frontend") )) 
			isfrontend = 1;
		else if ( !strncmp( rdn.bv_val + rdn.bv_len - STRLENOF("config"),
				"config", STRLENOF("config") )) 
			isconfig = 1;
	}
	ptr1 = ber_bvchr( &e->e_name, '{' );
	if ( ptr1 && ptr1 < &e->e_name.bv_val[ rdn.bv_len ] ) {
		char	*next;
		ptr2 = strchr( ptr1, '}' );
		if ( !ptr2 || ptr2 > &e->e_name.bv_val[ rdn.bv_len ] )
			return LDAP_NAMING_VIOLATION;
		if ( ptr2-ptr1 == 1)
			return LDAP_NAMING_VIOLATION;
		gotindex = 1;
		index = strtol( ptr1 + 1, &next, 10 );
		if ( next == ptr1 + 1 || next[ 0 ] != '}' ) {
			return LDAP_NAMING_VIOLATION;
		}
		if ( index < 0 ) {
			/* Special case, we allow -1 for the frontendDB */
			if ( index != -1 || !isfrontend )
				return LDAP_NAMING_VIOLATION;
		}
		if ( isconfig && index != 0 ){
			return LDAP_NAMING_VIOLATION;
		}
	}

	/* count related kids.
	 * For entries of type Cft_Misc, only count siblings with same RDN type
	 */
	if ( ce_type == Cft_Misc ) {
		rdn.bv_val = e->e_nname.bv_val;
		ptr1 = strchr( rdn.bv_val, '=' );
		assert( ptr1 != NULL );

		rdn.bv_len = ptr1 - rdn.bv_val;

		for (nsibs=0, ce=parent->ce_kids; ce; ce=ce->ce_sibs) {
			struct berval rdn2;
			if ( ce->ce_type != ce_type )
				continue;

			dnRdn( &ce->ce_entry->e_nname, &rdn2 );

			ptr1 = strchr( rdn2.bv_val, '=' );
			assert( ptr1 != NULL );

			rdn2.bv_len = ptr1 - rdn2.bv_val;
			if ( bvmatch( &rdn, &rdn2 ))
				nsibs++;
		}
	} else {
		for (nsibs=0, ce=parent->ce_kids; ce; ce=ce->ce_sibs) {
			if ( ce->ce_type == ce_type ) nsibs++;
		}
	}

	/* account for -1 frontend */
	if ( ce_type == Cft_Database )
		nsibs--;

	if ( index != nsibs ) {
		if ( gotindex ) {
			if ( index < nsibs ) {
				if ( tailindex ) return LDAP_NAMING_VIOLATION;
				/* Siblings need to be renumbered */
				if ( index != -1 || !isfrontend )
					renumber = 1;
			}
		}
		/* config DB is always "0" */
		if ( isconfig && index == -1 ) {
			index = 0;
		}
		if (( !isfrontend && index == -1 ) || ( index > nsibs ) ){
			index = nsibs;
		}

		/* just make index = nsibs */
		if ( !renumber ) {
			rc = config_renumber_one( NULL, rs, parent, e, index, tailindex, 0 );
		}
	}
	if ( ibase ) *ibase = index;
	if ( renum ) *renum = renumber;
	return rc;
}

/* Insert all superior classes of the given class */
static int
count_oc( ObjectClass *oc, ConfigOCs ***copp, int *nocs )
{
	ConfigOCs	co, *cop;
	ObjectClass	**sups;

	for ( sups = oc->soc_sups; sups && *sups; sups++ ) {
		if ( count_oc( *sups, copp, nocs ) ) {
			return -1;
		}
	}

	co.co_name = &oc->soc_cname;
	cop = avl_find( CfOcTree, &co, CfOc_cmp );
	if ( cop ) {
		int	i;

		/* check for duplicates */
		for ( i = 0; i < *nocs; i++ ) {
			if ( *copp && (*copp)[i] == cop ) {
				break;
			}
		}

		if ( i == *nocs ) {
			ConfigOCs **tmp = ch_realloc( *copp, (*nocs + 1)*sizeof( ConfigOCs * ) );
			if ( tmp == NULL ) {
				return -1;
			}
			*copp = tmp;
			(*copp)[*nocs] = cop;
			(*nocs)++;
		}
	}

	return 0;
}

/* Find all superior classes of the given objectclasses,
 * return list in order of most-subordinate first.
 *
 * Special / auxiliary / Cft_Misc classes always take precedence.
 */
static ConfigOCs **
count_ocs( Attribute *oc_at, int *nocs )
{
	int		i, j, misc = -1;
	ConfigOCs	**colst = NULL;

	*nocs = 0;

	for ( i = oc_at->a_numvals; i--; ) {
		ObjectClass	*oc = oc_bvfind( &oc_at->a_nvals[i] );

		assert( oc != NULL );
		if ( count_oc( oc, &colst, nocs ) ) {
			ch_free( colst );
			return NULL;
		}
	}

	/* invert order */
	i = 0;
	j = *nocs - 1;
	while ( i < j ) {
		ConfigOCs *tmp = colst[i];
		colst[i] = colst[j];
		colst[j] = tmp;
		if (tmp->co_type == Cft_Misc)
			misc = j;
		i++; j--;
	}
	/* Move misc class to front of list */
	if (misc > 0) {
		ConfigOCs *tmp = colst[misc];
		for (i=misc; i>0; i--)
			colst[i] = colst[i-1];
		colst[0] = tmp;
	}

	return colst;
}

static int
cfAddInclude( CfEntryInfo *p, Entry *e, ConfigArgs *ca )
{
	/* Leftover from RE23. Never parse this entry */
	return LDAP_COMPARE_TRUE;
}

static int
cfAddSchema( CfEntryInfo *p, Entry *e, ConfigArgs *ca )
{
	ConfigFile *cfo;

	/* This entry is hardcoded, don't re-parse it */
	if ( p->ce_type == Cft_Global ) {
		cfn = p->ce_private;
		ca->ca_private = cfn;
		return LDAP_COMPARE_TRUE;
	}
	if ( p->ce_type != Cft_Schema )
		return LDAP_CONSTRAINT_VIOLATION;

	cfn = ch_calloc( 1, sizeof(ConfigFile) );
	ca->ca_private = cfn;
	cfo = p->ce_private;
	cfn->c_sibs = cfo->c_kids;
	cfo->c_kids = cfn;
	return LDAP_SUCCESS;
}

static int
cfAddDatabase( CfEntryInfo *p, Entry *e, struct config_args_s *ca )
{
	if ( p->ce_type != Cft_Global ) {
		return LDAP_CONSTRAINT_VIOLATION;
	}
	/* config must be {0}, nothing else allowed */
	if ( !strncmp( e->e_nname.bv_val, "olcDatabase={0}", STRLENOF("olcDatabase={0}")) &&
		strncmp( e->e_nname.bv_val + STRLENOF("olcDatabase={0}"), "config,", STRLENOF("config,") )) {
		return LDAP_CONSTRAINT_VIOLATION;
	}
	ca->be = frontendDB;	/* just to get past check_vals */
	return LDAP_SUCCESS;
}

static int
cfAddBackend( CfEntryInfo *p, Entry *e, struct config_args_s *ca )
{
	if ( p->ce_type != Cft_Global ) {
		return LDAP_CONSTRAINT_VIOLATION;
	}
	return LDAP_SUCCESS;
}

static int
cfAddModule( CfEntryInfo *p, Entry *e, struct config_args_s *ca )
{
	if ( p->ce_type != Cft_Global ) {
		return LDAP_CONSTRAINT_VIOLATION;
	}
	return LDAP_SUCCESS;
}

static int
cfAddOverlay( CfEntryInfo *p, Entry *e, struct config_args_s *ca )
{
	if ( p->ce_type != Cft_Database ) {
		return LDAP_CONSTRAINT_VIOLATION;
	}
	ca->be = p->ce_be;
	return LDAP_SUCCESS;
}

static void
schema_destroy_one( ConfigArgs *ca, ConfigOCs **colst, int nocs,
	CfEntryInfo *p )
{
	ConfigTable *ct;
	ConfigFile *cfo;
	AttributeDescription *ad;
	const char *text;

	ca->valx = -1;
	ca->line = NULL;
	ca->argc = 1;
	if ( cfn->c_cr_head ) {
		struct berval bv = BER_BVC("olcDitContentRules");
		ad = NULL;
		slap_bv2ad( &bv, &ad, &text );
		ct = config_find_table( colst, nocs, ad, ca );
		config_del_vals( ct, ca );
	}
	if ( cfn->c_oc_head ) {
		struct berval bv = BER_BVC("olcObjectClasses");
		ad = NULL;
		slap_bv2ad( &bv, &ad, &text );
		ct = config_find_table( colst, nocs, ad, ca );
		config_del_vals( ct, ca );
	}
	if ( cfn->c_at_head ) {
		struct berval bv = BER_BVC("olcAttributeTypes");
		ad = NULL;
		slap_bv2ad( &bv, &ad, &text );
		ct = config_find_table( colst, nocs, ad, ca );
		config_del_vals( ct, ca );
	}
	if ( cfn->c_syn_head ) {
		struct berval bv = BER_BVC("olcLdapSyntaxes");
		ad = NULL;
		slap_bv2ad( &bv, &ad, &text );
		ct = config_find_table( colst, nocs, ad, ca );
		config_del_vals( ct, ca );
	}
	if ( cfn->c_om_head ) {
		struct berval bv = BER_BVC("olcObjectIdentifier");
		ad = NULL;
		slap_bv2ad( &bv, &ad, &text );
		ct = config_find_table( colst, nocs, ad, ca );
		config_del_vals( ct, ca );
	}
	cfo = p->ce_private;
	cfo->c_kids = cfn->c_sibs;
	ch_free( cfn );
}

static int
config_add_oc( ConfigOCs **cop, CfEntryInfo *last, Entry *e, ConfigArgs *ca )
{
	int		rc = LDAP_CONSTRAINT_VIOLATION;
	ObjectClass	**ocp;

	if ( (*cop)->co_ldadd ) {
		rc = (*cop)->co_ldadd( last, e, ca );
		if ( rc != LDAP_CONSTRAINT_VIOLATION ) {
			return rc;
		}
	}

	for ( ocp = (*cop)->co_oc->soc_sups; ocp && *ocp; ocp++ ) {
		ConfigOCs	co = { 0 };

		co.co_name = &(*ocp)->soc_cname;
		*cop = avl_find( CfOcTree, &co, CfOc_cmp );
		if ( *cop == NULL ) {
			return rc;
		}

		rc = config_add_oc( cop, last, e, ca );
		if ( rc != LDAP_CONSTRAINT_VIOLATION ) {
			return rc;
		}
	}

	return rc;
}

/* Parse an LDAP entry into config directives */
static int
config_add_internal( CfBackInfo *cfb, Entry *e, ConfigArgs *ca, SlapReply *rs,
	int *renum, Operation *op )
{
	CfEntryInfo	*ce, *last = NULL;
	ConfigOCs	co, *coptr, **colst;
	Attribute	*a, *oc_at, *soc_at;
	int		i, ibase = -1, nocs, rc = 0;
	struct berval	pdn;
	ConfigTable	*ct;
	char		*ptr, *log_prefix = op ? op->o_log_prefix : "";

	memset( ca, 0, sizeof(ConfigArgs));

	/* Make sure parent exists and entry does not. But allow
	 * Databases and Overlays to be inserted. Don't do any
	 * auto-renumbering if manageDSAit control is present.
	 */
	ce = config_find_base( cfb->cb_root, &e->e_nname, &last );
	if ( ce ) {
		if ( ( op && op->o_managedsait ) ||
			( ce->ce_type != Cft_Database && ce->ce_type != Cft_Overlay &&
			  ce->ce_type != Cft_Module ) )
		{
			Debug( LDAP_DEBUG_TRACE, "%s: config_add_internal: "
				"DN=\"%s\" already exists\n",
				log_prefix, e->e_name.bv_val, 0 );
			/* global schema ignores all writes */
			if ( ce->ce_type == Cft_Schema && ce->ce_parent->ce_type == Cft_Global )
				return LDAP_COMPARE_TRUE;
			return LDAP_ALREADY_EXISTS;
		}
	}

	dnParent( &e->e_nname, &pdn );

	/* If last is NULL, the new entry is the root/suffix entry, 
	 * otherwise last should be the parent.
	 */
	if ( last && !dn_match( &last->ce_entry->e_nname, &pdn ) ) {
		if ( rs ) {
			rs->sr_matched = last->ce_entry->e_name.bv_val;
		}
		Debug( LDAP_DEBUG_TRACE, "%s: config_add_internal: "
			"DN=\"%s\" not child of DN=\"%s\"\n",
			log_prefix, e->e_name.bv_val,
			last->ce_entry->e_name.bv_val );
		return LDAP_NO_SUCH_OBJECT;
	}

	if ( op ) {
		/* No parent, must be root. This will never happen... */
		if ( !last && !be_isroot( op ) && !be_shadow_update( op ) ) {
			return LDAP_NO_SUCH_OBJECT;
		}

		if ( last && !access_allowed( op, last->ce_entry,
			slap_schema.si_ad_children, NULL, ACL_WADD, NULL ) )
		{
			Debug( LDAP_DEBUG_TRACE, "%s: config_add_internal: "
				"DN=\"%s\" no write access to \"children\" of parent\n",
				log_prefix, e->e_name.bv_val, 0 );
			return LDAP_INSUFFICIENT_ACCESS;
		}
	}

	oc_at = attr_find( e->e_attrs, slap_schema.si_ad_objectClass );
	if ( !oc_at ) {
		Debug( LDAP_DEBUG_TRACE, "%s: config_add_internal: "
			"DN=\"%s\" no objectClass\n",
			log_prefix, e->e_name.bv_val, 0 );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	soc_at = attr_find( e->e_attrs, slap_schema.si_ad_structuralObjectClass );
	if ( !soc_at ) {
		ObjectClass	*soc = NULL;
		char		textbuf[ SLAP_TEXT_BUFLEN ];
		const char	*text = textbuf;

		/* FIXME: check result */
		rc = structural_class( oc_at->a_nvals, &soc, NULL,
			&text, textbuf, sizeof(textbuf), NULL );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "%s: config_add_internal: "
				"DN=\"%s\" no structural objectClass (%s)\n",
				log_prefix, e->e_name.bv_val, text );
			return rc;
		}
		attr_merge_one( e, slap_schema.si_ad_structuralObjectClass, &soc->soc_cname, NULL );
		soc_at = attr_find( e->e_attrs, slap_schema.si_ad_structuralObjectClass );
		if ( soc_at == NULL ) {
			Debug( LDAP_DEBUG_TRACE, "%s: config_add_internal: "
				"DN=\"%s\" no structural objectClass; "
				"unable to merge computed class %s\n",
				log_prefix, e->e_name.bv_val,
				soc->soc_cname.bv_val );
			return LDAP_OBJECT_CLASS_VIOLATION;
		}

		Debug( LDAP_DEBUG_TRACE, "%s: config_add_internal: "
			"DN=\"%s\" no structural objectClass; "
			"computed objectClass %s merged\n",
			log_prefix, e->e_name.bv_val,
			soc->soc_cname.bv_val );
	}

	/* Fake the coordinates based on whether we're part of an
	 * LDAP Add or if reading the config dir
	 */
	if ( rs ) {
		ca->fname = "slapd";
		ca->lineno = 0;
	} else {
		ca->fname = cfdir.bv_val;
		ca->lineno = 1;
	}
	ca->ca_op = op;

	co.co_name = &soc_at->a_nvals[0];
	coptr = avl_find( CfOcTree, &co, CfOc_cmp );
	if ( coptr == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "%s: config_add_internal: "
			"DN=\"%s\" no structural objectClass in configuration table\n",
			log_prefix, e->e_name.bv_val, 0 );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	/* Only the root can be Cft_Global, everything else must
	 * have a parent. Only limited nesting arrangements are allowed.
	 */
	rc = LDAP_CONSTRAINT_VIOLATION;
	if ( coptr->co_type == Cft_Global && !last ) {
		cfn = cfb->cb_config;
		ca->ca_private = cfn;
		ca->be = frontendDB;	/* just to get past check_vals */
		rc = LDAP_SUCCESS;
	}

	colst = count_ocs( oc_at, &nocs );

	/* Check whether the Add is allowed by its parent, and do
	 * any necessary arg setup
	 */
	if ( last ) {
		rc = config_add_oc( &coptr, last, e, ca );
		if ( rc == LDAP_CONSTRAINT_VIOLATION ) {
			for ( i = 0; i<nocs; i++ ) {
				/* Already checked these */
				if ( colst[i]->co_oc->soc_kind == LDAP_SCHEMA_STRUCTURAL )
					continue;
				if ( colst[i]->co_ldadd &&
					( rc = colst[i]->co_ldadd( last, e, ca ))
						!= LDAP_CONSTRAINT_VIOLATION ) {
					coptr = colst[i];
					break;
				}
			}
		}
		if ( rc == LDAP_CONSTRAINT_VIOLATION ) {
			Debug( LDAP_DEBUG_TRACE, "%s: config_add_internal: "
				"DN=\"%s\" no structural objectClass add function\n",
				log_prefix, e->e_name.bv_val, 0 );
			return LDAP_OBJECT_CLASS_VIOLATION;
		}
	}

	/* Add the entry but don't parse it, we already have its contents */
	if ( rc == LDAP_COMPARE_TRUE ) {
		rc = LDAP_SUCCESS;
		goto ok;
	}

	if ( rc != LDAP_SUCCESS )
		goto done_noop;

	/* Parse all the values and check for simple syntax errors before
	 * performing any set actions.
	 *
	 * If doing an LDAPadd, check for indexed names and any necessary
	 * renaming/renumbering. Entries that don't need indexed names are
	 * ignored. Entries that need an indexed name and arrive without one
	 * are assigned to the end. Entries that arrive with an index may
	 * cause the following entries to be renumbered/bumped down.
	 *
	 * Note that "pseudo-indexed" entries (cn=Include{xx}, cn=Module{xx})
	 * don't allow Adding an entry with an index that's already in use.
	 * This is flagged as an error (LDAP_ALREADY_EXISTS) up above.
	 *
	 * These entries can have auto-assigned indexes (appended to the end)
	 * but only the other types support auto-renumbering of siblings.
	 */
	{
		rc = check_name_index( last, coptr->co_type, e, rs, renum,
			&ibase );
		if ( rc ) {
			goto done_noop;
		}
		if ( renum && *renum && coptr->co_type != Cft_Database &&
			coptr->co_type != Cft_Overlay )
		{
			snprintf( ca->cr_msg, sizeof( ca->cr_msg ),
				"operation requires sibling renumbering" );
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto done_noop;
		}
	}

	init_config_argv( ca );

	/* Make sure we process attrs in the required order */
	sort_attrs( e, colst, nocs );

	for ( a = e->e_attrs; a; a = a->a_next ) {
		if ( a == oc_at ) continue;
		ct = config_find_table( colst, nocs, a->a_desc, ca );
		if ( !ct ) continue;	/* user data? */
		rc = check_vals( ct, ca, a, 1 );
		if ( rc ) goto done_noop;
	}

	/* Basic syntax checks are OK. Do the actual settings. */
	for ( a=e->e_attrs; a; a=a->a_next ) {
		if ( a == oc_at ) continue;
		ct = config_find_table( colst, nocs, a->a_desc, ca );
		if ( !ct ) continue;	/* user data? */
		for (i=0; a->a_vals[i].bv_val; i++) {
			char *iptr = NULL;
			ca->valx = -1;
			ca->line = a->a_vals[i].bv_val;
			if ( a->a_desc->ad_type->sat_flags & SLAP_AT_ORDERED ) {
				ptr = strchr( ca->line, '}' );
				if ( ptr ) {
					iptr = strchr( ca->line, '{' );
					ca->line = ptr+1;
				}
			}
			if ( a->a_desc->ad_type->sat_flags & SLAP_AT_ORDERED_SIB ) {
				if ( iptr ) {
					ca->valx = strtol( iptr+1, NULL, 0 );
				}
			} else {
				ca->valx = i;
			}
			rc = config_parse_add( ct, ca, i );
			if ( rc ) {
				rc = LDAP_OTHER;
				goto done;
			}
		}
	}
ok:
	/* Newly added databases and overlays need to be started up */
	if ( CONFIG_ONLINE_ADD( ca )) {
		if ( coptr->co_type == Cft_Database ) {
			rc = backend_startup_one( ca->be, &ca->reply );

		} else if ( coptr->co_type == Cft_Overlay ) {
			if ( ca->bi->bi_db_open ) {
				BackendInfo *bi_orig = ca->be->bd_info;
				ca->be->bd_info = ca->bi;
				rc = ca->bi->bi_db_open( ca->be, &ca->reply );
				ca->be->bd_info = bi_orig;
			}
		} else if ( ca->cleanup ) {
			rc = ca->cleanup( ca );
		}
		if ( rc ) {
			if (ca->cr_msg[0] == '\0')
				snprintf( ca->cr_msg, sizeof( ca->cr_msg ), "<%s> failed startup", ca->argv[0] );

			Debug(LDAP_DEBUG_ANY, "%s: %s (%s)!\n",
				ca->log, ca->cr_msg, ca->argv[1] );
			rc = LDAP_OTHER;
			goto done;
		}
	}

	ca->valx = ibase;
	ce = ch_calloc( 1, sizeof(CfEntryInfo) );
	ce->ce_parent = last;
	ce->ce_entry = entry_dup( e );
	ce->ce_entry->e_private = ce;
	ce->ce_type = coptr->co_type;
	ce->ce_be = ca->be;
	ce->ce_bi = ca->bi;
	ce->ce_private = ca->ca_private;
	ca->ca_entry = ce->ce_entry;
	if ( !last ) {
		cfb->cb_root = ce;
	} else if ( last->ce_kids ) {
		CfEntryInfo *c2, **cprev;

		/* Advance to first of this type */
		cprev = &last->ce_kids;
		for ( c2 = *cprev; c2 && c2->ce_type < ce->ce_type; ) {
			cprev = &c2->ce_sibs;
			c2 = c2->ce_sibs;
		}
		/* Account for the (-1) frontendDB entry */
		if ( ce->ce_type == Cft_Database ) {
			if ( ca->be == frontendDB )
				ibase = 0;
			else if ( ibase != -1 )
				ibase++;
		}
		/* Append */
		if ( ibase < 0 ) {
			for (c2 = *cprev; c2 && c2->ce_type == ce->ce_type;) {
				cprev = &c2->ce_sibs;
				c2 = c2->ce_sibs;
			}
		} else {
		/* Insert */
			int i;
			for ( i=0; i<ibase; i++ ) {
				c2 = *cprev;
				cprev = &c2->ce_sibs;
			}
		}
		ce->ce_sibs = *cprev;
		*cprev = ce;
	} else {
		last->ce_kids = ce;
	}

done:
	if ( rc ) {
		if ( (coptr->co_type == Cft_Database) && ca->be ) {
			if ( ca->be != frontendDB )
				backend_destroy_one( ca->be, 1 );
		} else if ( (coptr->co_type == Cft_Overlay) && ca->bi ) {
			overlay_destroy_one( ca->be, (slap_overinst *)ca->bi );
		} else if ( coptr->co_type == Cft_Schema ) {
			schema_destroy_one( ca, colst, nocs, last );
		}
	}
done_noop:

	ch_free( ca->argv );
	if ( colst ) ch_free( colst );
	return rc;
}

#define	BIGTMP	10000
static int
config_rename_add( Operation *op, SlapReply *rs, CfEntryInfo *ce,
	int base, int rebase, int max, int use_ldif )
{
	CfEntryInfo *ce2, *ce3, *cetmp = NULL, *cerem = NULL;
	ConfigType etype = ce->ce_type;
	int count = 0, rc = 0;

	/* Reverse ce list */
	for (ce2 = ce->ce_sibs;ce2;ce2 = ce3) {
		if (ce2->ce_type != etype) {
			cerem = ce2;
			break;
		}
		ce3 = ce2->ce_sibs;
		ce2->ce_sibs = cetmp;
		cetmp = ce2;
		count++;
		if ( max && count >= max ) {
			cerem = ce3;
			break;
		}
	}

	/* Move original to a temp name until increments are done */
	if ( rebase ) {
		ce->ce_entry->e_private = NULL;
		rc = config_renumber_one( op, rs, ce->ce_parent, ce->ce_entry,
			base+BIGTMP, 0, use_ldif );
		ce->ce_entry->e_private = ce;
	}
	/* start incrementing */
	for (ce2=cetmp; ce2; ce2=ce3) {
		ce3 = ce2->ce_sibs;
		ce2->ce_sibs = cerem;
		cerem = ce2;
		if ( rc == 0 ) 
			rc = config_renumber_one( op, rs, ce2->ce_parent, ce2->ce_entry,
				count+base, 0, use_ldif );
		count--;
	}
	if ( rebase )
		rc = config_renumber_one( op, rs, ce->ce_parent, ce->ce_entry,
			base, 0, use_ldif );
	return rc;
}

static int
config_rename_del( Operation *op, SlapReply *rs, CfEntryInfo *ce,
	CfEntryInfo *ce2, int old, int use_ldif )
{
	int count = 0;

	/* Renumber original to a temp value */
	ce->ce_entry->e_private = NULL;
	config_renumber_one( op, rs, ce->ce_parent, ce->ce_entry,
		old+BIGTMP, 0, use_ldif );
	ce->ce_entry->e_private = ce;

	/* start decrementing */
	for (; ce2 != ce; ce2=ce2->ce_sibs) {
		config_renumber_one( op, rs, ce2->ce_parent, ce2->ce_entry,
			count+old, 0, use_ldif );
		count++;
	}
	return config_renumber_one( op, rs, ce->ce_parent, ce->ce_entry,
		count+old, 0, use_ldif );
}

/* Parse an LDAP entry into config directives, then store in underlying
 * database.
 */
static int
config_back_add( Operation *op, SlapReply *rs )
{
	CfBackInfo *cfb;
	int renumber;
	ConfigArgs ca;

	if ( !access_allowed( op, op->ora_e, slap_schema.si_ad_entry,
		NULL, ACL_WADD, NULL )) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto out;
	}

	/*
	 * Check for attribute ACL
	 */
	if ( !acl_check_modlist( op, op->ora_e, op->orm_modlist )) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "no write access to attribute";
		goto out;
	}

	cfb = (CfBackInfo *)op->o_bd->be_private;

	/* add opattrs for syncprov */
	{
		char textbuf[SLAP_TEXT_BUFLEN];
		size_t textlen = sizeof textbuf;
		rs->sr_err = entry_schema_check(op, op->ora_e, NULL, 0, 1, NULL,
			&rs->sr_text, textbuf, sizeof( textbuf ) );
		if ( rs->sr_err != LDAP_SUCCESS )
			goto out;
		rs->sr_err = slap_add_opattrs( op, &rs->sr_text, textbuf, textlen, 1 );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(config_back_add) ": entry failed op attrs add: "
				"%s (%d)\n", rs->sr_text, rs->sr_err, 0 );
			goto out;
		}
	}

	if ( op->o_abandon ) {
		rs->sr_err = SLAPD_ABANDON;
		goto out;
	}
	ldap_pvt_thread_pool_pause( &connection_pool );

	/* Strategy:
	 * 1) check for existence of entry
	 * 2) check for sibling renumbering
	 * 3) perform internal add
	 * 4) perform any necessary renumbering
	 * 5) store entry in underlying database
	 */
	rs->sr_err = config_add_internal( cfb, op->ora_e, &ca, rs, &renumber, op );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		rs->sr_text = ca.cr_msg;
		goto out2;
	}

	if ( renumber ) {
		CfEntryInfo *ce = ca.ca_entry->e_private;
		req_add_s addr = op->oq_add;
		op->o_tag = LDAP_REQ_MODRDN;
		rs->sr_err = config_rename_add( op, rs, ce, ca.valx, 0, 0, cfb->cb_use_ldif );
		op->o_tag = LDAP_REQ_ADD;
		op->oq_add = addr;
		if ( rs->sr_err != LDAP_SUCCESS ) {
			goto out2;
		}
	}

	if ( cfb->cb_use_ldif ) {
		BackendDB *be = op->o_bd;
		slap_callback sc = { NULL, slap_null_cb, NULL, NULL }, *scp;
		struct berval dn, ndn;

		op->o_bd = &cfb->cb_db;

		/* Save current rootdn; use the underlying DB's rootdn */
		dn = op->o_dn;
		ndn = op->o_ndn;
		op->o_dn = op->o_bd->be_rootdn;
		op->o_ndn = op->o_bd->be_rootndn;

		scp = op->o_callback;
		op->o_callback = &sc;
		op->o_bd->be_add( op, rs );
		op->o_bd = be;
		op->o_callback = scp;
		op->o_dn = dn;
		op->o_ndn = ndn;
	}

out2:;
	ldap_pvt_thread_pool_resume( &connection_pool );

out:;
	{	int repl = op->o_dont_replicate;
		if ( rs->sr_err == LDAP_COMPARE_TRUE ) {
			rs->sr_text = NULL; /* Set after config_add_internal */
			rs->sr_err = LDAP_SUCCESS;
			op->o_dont_replicate = 1;
		}
		send_ldap_result( op, rs );
		op->o_dont_replicate = repl;
	}
	slap_graduate_commit_csn( op );
	return rs->sr_err;
}

typedef struct delrec {
	struct delrec *next;
	int nidx;
	int idx[1];
} delrec;

static int
config_modify_add( ConfigTable *ct, ConfigArgs *ca, AttributeDescription *ad,
	int i )
{
	int rc;

	ca->valx = -1;
	if (ad->ad_type->sat_flags & SLAP_AT_ORDERED &&
		ca->line[0] == '{' )
	{
		char *ptr = strchr( ca->line + 1, '}' );
		if ( ptr ) {
			char	*next;

			ca->valx = strtol( ca->line + 1, &next, 0 );
			if ( next == ca->line + 1 || next[ 0 ] != '}' ) {
				return LDAP_OTHER;
			}
			ca->line = ptr+1;
		}
	}
	rc = config_parse_add( ct, ca, i );
	if ( rc ) {
		rc = LDAP_OTHER;
	}
	return rc;
}

static int
config_modify_internal( CfEntryInfo *ce, Operation *op, SlapReply *rs,
	ConfigArgs *ca )
{
	int rc = LDAP_UNWILLING_TO_PERFORM;
	Modifications *ml;
	Entry *e = ce->ce_entry;
	Attribute *save_attrs = e->e_attrs, *oc_at, *s, *a;
	ConfigTable *ct;
	ConfigOCs **colst;
	int i, nocs;
	char *ptr;
	delrec *dels = NULL, *deltail = NULL;

	oc_at = attr_find( e->e_attrs, slap_schema.si_ad_objectClass );
	if ( !oc_at ) return LDAP_OBJECT_CLASS_VIOLATION;

	for (ml = op->orm_modlist; ml; ml=ml->sml_next) {
		if (ml->sml_desc == slap_schema.si_ad_objectClass)
			return rc;
	}

	colst = count_ocs( oc_at, &nocs );

	/* make sure add/del flags are clear; should always be true */
	for ( s = save_attrs; s; s = s->a_next ) {
		s->a_flags &= ~(SLAP_ATTR_IXADD|SLAP_ATTR_IXDEL);
	}

	e->e_attrs = attrs_dup( e->e_attrs );

	init_config_argv( ca );
	ca->be = ce->ce_be;
	ca->bi = ce->ce_bi;
	ca->ca_private = ce->ce_private;
	ca->ca_entry = e;
	ca->fname = "slapd";
	ca->ca_op = op;
	strcpy( ca->log, "back-config" );

	for (ml = op->orm_modlist; ml; ml=ml->sml_next) {
		ct = config_find_table( colst, nocs, ml->sml_desc, ca );
		switch (ml->sml_op) {
		case LDAP_MOD_DELETE:
		case LDAP_MOD_REPLACE:
		case SLAP_MOD_SOFTDEL:
		{
			BerVarray vals = NULL, nvals = NULL;
			int *idx = NULL;
			if ( ct && ( ct->arg_type & ARG_NO_DELETE )) {
				rc = LDAP_OTHER;
				snprintf(ca->cr_msg, sizeof(ca->cr_msg), "cannot delete %s",
					ml->sml_desc->ad_cname.bv_val );
				goto out_noop;
			}
			if ( ml->sml_op == LDAP_MOD_REPLACE ) {
				vals = ml->sml_values;
				nvals = ml->sml_nvalues;
				ml->sml_values = NULL;
				ml->sml_nvalues = NULL;
			}
			/* If we're deleting by values, remember the indexes of the
			 * values we deleted.
			 */
			if ( ct && ml->sml_values ) {
				delrec *d;
				i = ml->sml_numvals;
				d = ch_malloc( sizeof(delrec) + (i - 1)* sizeof(int));
				d->nidx = i;
				d->next = NULL;
				if ( dels ) {
					deltail->next = d;
				} else {
					dels = d;
				}
				deltail = d;
				idx = d->idx;
			}
			rc = modify_delete_vindex(e, &ml->sml_mod,
				get_permissiveModify(op),
				&rs->sr_text, ca->cr_msg, sizeof(ca->cr_msg), idx );
			if ( ml->sml_op == LDAP_MOD_REPLACE ) {
				ml->sml_values = vals;
				ml->sml_nvalues = nvals;
			}
			if ( rc == LDAP_NO_SUCH_ATTRIBUTE && ml->sml_op == SLAP_MOD_SOFTDEL )
			{
				rc = LDAP_SUCCESS;
			}
			/* FIXME: check rc before fallthru? */
			if ( !vals )
				break;
		}
			/* FALLTHRU: LDAP_MOD_REPLACE && vals */

		case SLAP_MOD_ADD_IF_NOT_PRESENT:
			if ( ml->sml_op == SLAP_MOD_ADD_IF_NOT_PRESENT
				&& attr_find( e->e_attrs, ml->sml_desc ) )
			{
				rc = LDAP_SUCCESS;
				break;
			}

		case LDAP_MOD_ADD:
		case SLAP_MOD_SOFTADD: {
			int mop = ml->sml_op;
			int navals = -1;
			ml->sml_op = LDAP_MOD_ADD;
			if ( ct ) {
				if ( ct->arg_type & ARG_NO_INSERT ) {
					Attribute *a = attr_find( e->e_attrs, ml->sml_desc );
					if ( a ) {
						navals = a->a_numvals;
					}
				}
				for ( i=0; !BER_BVISNULL( &ml->sml_values[i] ); i++ ) {
					if ( ml->sml_values[i].bv_val[0] == '{' &&
						navals >= 0 )
					{
						char	*next, *val = ml->sml_values[i].bv_val + 1;
						int	j;

						j = strtol( val, &next, 0 );
						if ( next == val || next[ 0 ] != '}' || j < navals ) {
							rc = LDAP_OTHER;
							snprintf(ca->cr_msg, sizeof(ca->cr_msg), "cannot insert %s",
								ml->sml_desc->ad_cname.bv_val );
							goto out_noop;
						}
					}
					rc = check_vals( ct, ca, ml, 0 );
					if ( rc ) goto out_noop;
				}
			}
			rc = modify_add_values(e, &ml->sml_mod,
				   get_permissiveModify(op),
				   &rs->sr_text, ca->cr_msg, sizeof(ca->cr_msg) );

			/* If value already exists, show success here
			 * and ignore this operation down below.
			 */
			if ( mop == SLAP_MOD_SOFTADD ) {
				if ( rc == LDAP_TYPE_OR_VALUE_EXISTS )
					rc = LDAP_SUCCESS;
				else
					mop = LDAP_MOD_ADD;
			}
			ml->sml_op = mop;
			break;
			}

			break;
		case LDAP_MOD_INCREMENT:	/* FIXME */
			break;
		default:
			break;
		}
		if(rc != LDAP_SUCCESS) break;
	}
	
	if ( rc == LDAP_SUCCESS) {
		/* check that the entry still obeys the schema */
		rc = entry_schema_check(op, e, NULL, 0, 0, NULL,
			&rs->sr_text, ca->cr_msg, sizeof(ca->cr_msg) );
	}
	if ( rc ) goto out_noop;

	/* Basic syntax checks are OK. Do the actual settings. */
	for ( ml = op->orm_modlist; ml; ml = ml->sml_next ) {
		ct = config_find_table( colst, nocs, ml->sml_desc, ca );
		if ( !ct ) continue;

		s = attr_find( save_attrs, ml->sml_desc );
		a = attr_find( e->e_attrs, ml->sml_desc );

		switch (ml->sml_op) {
		case LDAP_MOD_DELETE:
		case LDAP_MOD_REPLACE: {
			BerVarray vals = NULL, nvals = NULL;
			delrec *d = NULL;

			if ( ml->sml_op == LDAP_MOD_REPLACE ) {
				vals = ml->sml_values;
				nvals = ml->sml_nvalues;
				ml->sml_values = NULL;
				ml->sml_nvalues = NULL;
			}

			if ( ml->sml_values )
				d = dels;

			/* If we didn't delete the whole attribute */
			if ( ml->sml_values && a ) {
				struct berval *mvals;
				int j;

				if ( ml->sml_nvalues )
					mvals = ml->sml_nvalues;
				else
					mvals = ml->sml_values;

				/* use the indexes we saved up above */
				for (i=0; i < d->nidx; i++) {
					struct berval bv = *mvals++;
					if ( a->a_desc->ad_type->sat_flags & SLAP_AT_ORDERED &&
						bv.bv_val[0] == '{' ) {
						ptr = strchr( bv.bv_val, '}' ) + 1;
						bv.bv_len -= ptr - bv.bv_val;
						bv.bv_val = ptr;
					}
					ca->line = bv.bv_val;
					ca->valx = d->idx[i];
					config_parse_vals(ct, ca, d->idx[i] );
					rc = config_del_vals( ct, ca );
					if ( rc != LDAP_SUCCESS ) break;
					if ( s )
						s->a_flags |= SLAP_ATTR_IXDEL;
					for (j=i+1; j < d->nidx; j++)
						if ( d->idx[j] >d->idx[i] )
							d->idx[j]--;
				}
			} else {
				ca->valx = -1;
				ca->line = NULL;
				ca->argc = 1;
				rc = config_del_vals( ct, ca );
				if ( rc ) rc = LDAP_OTHER;
				if ( s )
					s->a_flags |= SLAP_ATTR_IXDEL;
			}
			if ( ml->sml_values ) {
				d = d->next;
				ch_free( dels );
				dels = d;
			}
			if ( ml->sml_op == LDAP_MOD_REPLACE ) {
				ml->sml_values = vals;
				ml->sml_nvalues = nvals;
			}
			if ( !vals || rc != LDAP_SUCCESS )
				break;
			}
			/* FALLTHRU: LDAP_MOD_REPLACE && vals */

		case LDAP_MOD_ADD:
			if ( !a )
				break;
			for (i=0; ml->sml_values[i].bv_val; i++) {
				ca->line = ml->sml_values[i].bv_val;
				ca->valx = -1;
				rc = config_modify_add( ct, ca, ml->sml_desc, i );
				if ( rc )
					goto out;
				a->a_flags |= SLAP_ATTR_IXADD;
			}
			break;
		}
	}

out:
	/* Undo for a failed operation */
	if ( rc != LDAP_SUCCESS ) {
		ConfigReply msg = ca->reply;
		for ( s = save_attrs; s; s = s->a_next ) {
			if ( s->a_flags & SLAP_ATTR_IXDEL ) {
				s->a_flags &= ~(SLAP_ATTR_IXDEL|SLAP_ATTR_IXADD);
				ct = config_find_table( colst, nocs, s->a_desc, ca );
				a = attr_find( e->e_attrs, s->a_desc );
				if ( a ) {
					/* clear the flag so the add check below will skip it */
					a->a_flags &= ~(SLAP_ATTR_IXDEL|SLAP_ATTR_IXADD);
					ca->valx = -1;
					ca->line = NULL;
					ca->argc = 1;
					config_del_vals( ct, ca );
				}
				for ( i=0; !BER_BVISNULL( &s->a_vals[i] ); i++ ) {
					ca->line = s->a_vals[i].bv_val;
					ca->valx = -1;
					config_modify_add( ct, ca, s->a_desc, i );
				}
			}
		}
		for ( a = e->e_attrs; a; a = a->a_next ) {
			if ( a->a_flags & SLAP_ATTR_IXADD ) {
				ct = config_find_table( colst, nocs, a->a_desc, ca );
				ca->valx = -1;
				ca->line = NULL;
				ca->argc = 1;
				config_del_vals( ct, ca );
				s = attr_find( save_attrs, a->a_desc );
				if ( s ) {
					s->a_flags &= ~(SLAP_ATTR_IXDEL|SLAP_ATTR_IXADD);
					for ( i=0; !BER_BVISNULL( &s->a_vals[i] ); i++ ) {
						ca->line = s->a_vals[i].bv_val;
						ca->valx = -1;
						config_modify_add( ct, ca, s->a_desc, i );
					}
				}
			}
		}
		ca->reply = msg;
	}

	if ( ca->cleanup ) {
		i = ca->cleanup( ca );
		if (rc == LDAP_SUCCESS)
			rc = i;
	}
out_noop:
	if ( rc == LDAP_SUCCESS ) {
		attrs_free( save_attrs );
		rs->sr_text = NULL;
	} else {
		attrs_free( e->e_attrs );
		e->e_attrs = save_attrs;
	}
	ch_free( ca->argv );
	if ( colst ) ch_free( colst );
	while( dels ) {
		deltail = dels->next;
		ch_free( dels );
		dels = deltail;
	}

	return rc;
}

static int
config_back_modify( Operation *op, SlapReply *rs )
{
	CfBackInfo *cfb;
	CfEntryInfo *ce, *last;
	Modifications *ml;
	ConfigArgs ca = {0};
	struct berval rdn;
	char *ptr;
	AttributeDescription *rad = NULL;
	int do_pause = 1;

	cfb = (CfBackInfo *)op->o_bd->be_private;

	ce = config_find_base( cfb->cb_root, &op->o_req_ndn, &last );
	if ( !ce ) {
		if ( last )
			rs->sr_matched = last->ce_entry->e_name.bv_val;
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		goto out;
	}

	if ( !acl_check_modlist( op, ce->ce_entry, op->orm_modlist )) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto out;
	}

	/* Get type of RDN */
	rdn = ce->ce_entry->e_nname;
	ptr = strchr( rdn.bv_val, '=' );
	rdn.bv_len = ptr - rdn.bv_val;
	rs->sr_err = slap_bv2ad( &rdn, &rad, &rs->sr_text );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		goto out;
	}

	/* Some basic validation... */
	for ( ml = op->orm_modlist; ml; ml = ml->sml_next ) {
		/* Don't allow Modify of RDN; must use ModRdn for that. */
		if ( ml->sml_desc == rad ) {
			rs->sr_err = LDAP_NOT_ALLOWED_ON_RDN;
			rs->sr_text = "Use modrdn to change the entry name";
			goto out;
		}
		/* Internal update of contextCSN? */
		if ( ml->sml_desc == slap_schema.si_ad_contextCSN && op->o_conn->c_conn_idx == -1 ) {
			do_pause = 0;
			break;
		}
	}

	slap_mods_opattrs( op, &op->orm_modlist, 1 );

	if ( do_pause ) {
		if ( op->o_abandon ) {
			rs->sr_err = SLAPD_ABANDON;
			goto out;
		}
		ldap_pvt_thread_pool_pause( &connection_pool );
	}

	/* Strategy:
	 * 1) perform the Modify on the cached Entry.
	 * 2) verify that the Entry still satisfies the schema.
	 * 3) perform the individual config operations.
	 * 4) store Modified entry in underlying LDIF backend.
	 */
	rs->sr_err = config_modify_internal( ce, op, rs, &ca );
	if ( rs->sr_err ) {
		rs->sr_text = ca.cr_msg;
	} else if ( cfb->cb_use_ldif ) {
		BackendDB *be = op->o_bd;
		slap_callback sc = { NULL, slap_null_cb, NULL, NULL }, *scp;
		struct berval dn, ndn;

		op->o_bd = &cfb->cb_db;

		dn = op->o_dn;
		ndn = op->o_ndn;
		op->o_dn = op->o_bd->be_rootdn;
		op->o_ndn = op->o_bd->be_rootndn;

		scp = op->o_callback;
		op->o_callback = &sc;
		op->o_bd->be_modify( op, rs );
		op->o_bd = be;
		op->o_callback = scp;
		op->o_dn = dn;
		op->o_ndn = ndn;
	}

	if ( do_pause )
		ldap_pvt_thread_pool_resume( &connection_pool );
out:
	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );
	return rs->sr_err;
}

static int
config_back_modrdn( Operation *op, SlapReply *rs )
{
	CfBackInfo *cfb;
	CfEntryInfo *ce, *last;
	struct berval rdn;
	int ixold, ixnew;

	cfb = (CfBackInfo *)op->o_bd->be_private;

	ce = config_find_base( cfb->cb_root, &op->o_req_ndn, &last );
	if ( !ce ) {
		if ( last )
			rs->sr_matched = last->ce_entry->e_name.bv_val;
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		goto out;
	}
	if ( !access_allowed( op, ce->ce_entry, slap_schema.si_ad_entry,
		NULL, ACL_WRITE, NULL )) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto out;
	}
	{ Entry *parent;
		if ( ce->ce_parent )
			parent = ce->ce_parent->ce_entry;
		else
			parent = (Entry *)&slap_entry_root;
		if ( !access_allowed( op, parent, slap_schema.si_ad_children,
			NULL, ACL_WRITE, NULL )) {
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
			goto out;
		}
	}

	/* We don't allow moving objects to new parents.
	 * Generally we only allow reordering a set of ordered entries.
	 */
	if ( op->orr_newSup ) {
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		goto out;
	}

	/* If newRDN == oldRDN, quietly succeed */
	dnRdn( &op->o_req_ndn, &rdn );
	if ( dn_match( &rdn, &op->orr_nnewrdn )) {
		rs->sr_err = LDAP_SUCCESS;
		goto out;
	}

	/* Current behavior, subject to change as needed:
	 *
	 * For backends and overlays, we only allow renumbering.
	 * For schema, we allow renaming with the same number.
	 * Otherwise, the op is not allowed.
	 */

	if ( ce->ce_type == Cft_Schema ) {
		char *ptr1, *ptr2;
		int len;

		/* Can't alter the main cn=schema entry */
		if ( ce->ce_parent->ce_type == Cft_Global ) {
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "renaming not allowed for this entry";
			goto out;
		}

		/* We could support this later if desired */
		ptr1 = ber_bvchr( &rdn, '}' );
		ptr2 = ber_bvchr( &op->orr_newrdn, '}' );
		len = ptr1 - rdn.bv_val;
		if ( len != ptr2 - op->orr_newrdn.bv_val ||
			strncmp( rdn.bv_val, op->orr_newrdn.bv_val, len )) {
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "schema reordering not supported";
			goto out;
		}
	} else if ( ce->ce_type == Cft_Database ||
		ce->ce_type == Cft_Overlay ) {
		char *ptr1, *ptr2, *iptr1, *iptr2;
		int len1, len2;

		iptr2 = ber_bvchr( &op->orr_newrdn, '=' ) + 1;
		if ( *iptr2 != '{' ) {
			rs->sr_err = LDAP_NAMING_VIOLATION;
			rs->sr_text = "new ordering index is required";
			goto out;
		}
		iptr2++;
		iptr1 = ber_bvchr( &rdn, '{' ) + 1;
		ptr1 = ber_bvchr( &rdn, '}' );
		ptr2 = ber_bvchr( &op->orr_newrdn, '}' );
		if ( !ptr2 ) {
			rs->sr_err = LDAP_NAMING_VIOLATION;
			rs->sr_text = "new ordering index is required";
			goto out;
		}

		len1 = ptr1 - rdn.bv_val;
		len2 = ptr2 - op->orr_newrdn.bv_val;

		if ( rdn.bv_len - len1 != op->orr_newrdn.bv_len - len2 ||
			strncmp( ptr1, ptr2, rdn.bv_len - len1 )) {
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "changing database/overlay type not allowed";
			goto out;
		}
		ixold = strtol( iptr1, NULL, 0 );
		ixnew = strtol( iptr2, &ptr1, 0 );
		if ( ptr1 != ptr2 || ixold < 0 || ixnew < 0 ) {
			rs->sr_err = LDAP_NAMING_VIOLATION;
			goto out;
		}
		/* config DB is always 0, cannot be changed */
		if ( ce->ce_type == Cft_Database && ( ixold == 0 || ixnew == 0 )) {
			rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
			goto out;
		}
	} else {
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		rs->sr_text = "renaming not supported for this entry";
		goto out;
	}

	if ( op->o_abandon ) {
		rs->sr_err = SLAPD_ABANDON;
		goto out;
	}
	ldap_pvt_thread_pool_pause( &connection_pool );

	if ( ce->ce_type == Cft_Schema ) {
		req_modrdn_s modr = op->oq_modrdn;
		struct berval rdn;
		Attribute *a;
		rs->sr_err = config_rename_attr( rs, ce->ce_entry, &rdn, &a );
		if ( rs->sr_err == LDAP_SUCCESS ) {
			rs->sr_err = config_rename_one( op, rs, ce->ce_entry,
				ce->ce_parent, a, &op->orr_newrdn, &op->orr_nnewrdn,
				cfb->cb_use_ldif );
		}
		op->oq_modrdn = modr;
	} else {
		CfEntryInfo *ce2, *cebase, **cprev, **cbprev, *ceold;
		req_modrdn_s modr = op->oq_modrdn;
		int i;

		/* Advance to first of this type */
		cprev = &ce->ce_parent->ce_kids;
		for ( ce2 = *cprev; ce2 && ce2->ce_type != ce->ce_type; ) {
			cprev = &ce2->ce_sibs;
			ce2 = ce2->ce_sibs;
		}
		/* Skip the -1 entry */
		if ( ce->ce_type == Cft_Database ) {
			cprev = &ce2->ce_sibs;
			ce2 = ce2->ce_sibs;
		}
		cebase = ce2;
		cbprev = cprev;

		/* Remove from old slot */
		for ( ce2 = *cprev; ce2 && ce2 != ce; ce2 = ce2->ce_sibs )
			cprev = &ce2->ce_sibs;
		*cprev = ce->ce_sibs;
		ceold = ce->ce_sibs;

		/* Insert into new slot */
		cprev = cbprev;
		for ( i=0; i<ixnew; i++ ) {
			ce2 = *cprev;
			if ( !ce2 )
				break;
			cprev = &ce2->ce_sibs;
		}
		ce->ce_sibs = *cprev;
		*cprev = ce;

		ixnew = i;

		/* NOTE: These should be encoded in the OC tables, not inline here */
		if ( ce->ce_type == Cft_Database )
			backend_db_move( ce->ce_be, ixnew );
		else if ( ce->ce_type == Cft_Overlay )
			overlay_move( ce->ce_be, (slap_overinst *)ce->ce_bi, ixnew );
			
		if ( ixold < ixnew ) {
			rs->sr_err = config_rename_del( op, rs, ce, ceold, ixold,
				cfb->cb_use_ldif );
		} else {
			rs->sr_err = config_rename_add( op, rs, ce, ixnew, 1,
				ixold - ixnew, cfb->cb_use_ldif );
		}
		op->oq_modrdn = modr;
	}

	ldap_pvt_thread_pool_resume( &connection_pool );
out:
	send_ldap_result( op, rs );
	return rs->sr_err;
}

static int
config_back_delete( Operation *op, SlapReply *rs )
{
#ifdef SLAP_CONFIG_DELETE
	CfBackInfo *cfb;
	CfEntryInfo *ce, *last, *ce2;

	cfb = (CfBackInfo *)op->o_bd->be_private;

	ce = config_find_base( cfb->cb_root, &op->o_req_ndn, &last );
	if ( !ce ) {
		if ( last )
			rs->sr_matched = last->ce_entry->e_name.bv_val;
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
	} else if ( ce->ce_kids ) {
		rs->sr_err = LDAP_NOT_ALLOWED_ON_NONLEAF;
	} else if ( op->o_abandon ) {
		rs->sr_err = SLAPD_ABANDON;
	} else if ( ce->ce_type == Cft_Overlay ||
			ce->ce_type == Cft_Database ||
			ce->ce_type == Cft_Misc ){
		char *iptr;
		int count, ixold;

		ldap_pvt_thread_pool_pause( &connection_pool );

		if ( ce->ce_type == Cft_Overlay ){
			overlay_remove( ce->ce_be, (slap_overinst *)ce->ce_bi, op );
		} else if ( ce->ce_type == Cft_Misc ) {
			/*
			 * only Cft_Misc objects that have a co_lddel handler set in
			 * the ConfigOCs struct can be deleted. This code also
			 * assumes that the entry can be only have one objectclass
			 * with co_type == Cft_Misc
			 */
			ConfigOCs co, *coptr;
			Attribute *oc_at;
			int i;

			oc_at = attr_find( ce->ce_entry->e_attrs,
					slap_schema.si_ad_objectClass );
			if ( !oc_at ) {
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "objectclass not found";
				ldap_pvt_thread_pool_resume( &connection_pool );
				goto out;
			}
			for ( i=0; !BER_BVISNULL(&oc_at->a_nvals[i]); i++ ) {
				co.co_name = &oc_at->a_nvals[i];
				coptr = avl_find( CfOcTree, &co, CfOc_cmp );
				if ( coptr == NULL || coptr->co_type != Cft_Misc ) {
					continue;
				}
				if ( ! coptr->co_lddel || coptr->co_lddel( ce, op ) ){
					rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
					if ( ! coptr->co_lddel ) {
						rs->sr_text = "No delete handler found";
					} else {
						rs->sr_err = LDAP_OTHER;
						/* FIXME: We should return a helpful error message
						 * here */
					}
					ldap_pvt_thread_pool_resume( &connection_pool );
					goto out;
				}
				break;
			}
		} else if (ce->ce_type == Cft_Database ) {
			if ( ce->ce_be == frontendDB || ce->ce_be == op->o_bd ){
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				rs->sr_text = "Cannot delete config or frontend database";
				ldap_pvt_thread_pool_resume( &connection_pool );
				goto out;
			}
			if ( ce->ce_be->bd_info->bi_db_close ) {
				ce->ce_be->bd_info->bi_db_close( ce->ce_be, NULL );
			}
			backend_destroy_one( ce->ce_be, 1);
		}

		/* remove CfEntryInfo from the siblings list */
		if ( ce->ce_parent->ce_kids == ce ) {
			ce->ce_parent->ce_kids = ce->ce_sibs;
		} else {
			for ( ce2 = ce->ce_parent->ce_kids ; ce2; ce2 = ce2->ce_sibs ) {
				if ( ce2->ce_sibs == ce ) {
					ce2->ce_sibs = ce->ce_sibs;
					break;
				}
			}
		}

		/* remove from underlying database */
		if ( cfb->cb_use_ldif ) {
			BackendDB *be = op->o_bd;
			slap_callback sc = { NULL, slap_null_cb, NULL, NULL }, *scp;
			struct berval dn, ndn, req_dn, req_ndn;

			op->o_bd = &cfb->cb_db;

			dn = op->o_dn;
			ndn = op->o_ndn;
			req_dn = op->o_req_dn;
			req_ndn = op->o_req_ndn;

			op->o_dn = op->o_bd->be_rootdn;
			op->o_ndn = op->o_bd->be_rootndn;
			op->o_req_dn = ce->ce_entry->e_name;
			op->o_req_ndn = ce->ce_entry->e_nname;

			scp = op->o_callback;
			op->o_callback = &sc;
			op->o_bd->be_delete( op, rs );
			op->o_bd = be;
			op->o_callback = scp;
			op->o_dn = dn;
			op->o_ndn = ndn;
			op->o_req_dn = req_dn;
			op->o_req_ndn = req_ndn;
		}

		/* renumber siblings */
		iptr = ber_bvchr( &op->o_req_ndn, '{' ) + 1;
		ixold = strtol( iptr, NULL, 0 );
		for (ce2 = ce->ce_sibs, count=0; ce2; ce2=ce2->ce_sibs) {
			config_renumber_one( op, rs, ce2->ce_parent, ce2->ce_entry,
				count+ixold, 0, cfb->cb_use_ldif );
			count++;
		}

		ce->ce_entry->e_private=NULL;
		entry_free(ce->ce_entry);
		ch_free(ce);
		ldap_pvt_thread_pool_resume( &connection_pool );
	} else {
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
	}
out:
#else
	rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
#endif /* SLAP_CONFIG_DELETE */
	send_ldap_result( op, rs );
	return rs->sr_err;
}

static int
config_back_search( Operation *op, SlapReply *rs )
{
	CfBackInfo *cfb;
	CfEntryInfo *ce, *last;
	slap_mask_t mask;

	cfb = (CfBackInfo *)op->o_bd->be_private;

	ce = config_find_base( cfb->cb_root, &op->o_req_ndn, &last );
	if ( !ce ) {
		if ( last )
			rs->sr_matched = last->ce_entry->e_name.bv_val;
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		goto out;
	}
	if ( !access_allowed_mask( op, ce->ce_entry, slap_schema.si_ad_entry, NULL,
		ACL_SEARCH, NULL, &mask ))
	{
		if ( !ACL_GRANT( mask, ACL_DISCLOSE )) {
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		} else {
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		}
		goto out;
	}
	switch ( op->ors_scope ) {
	case LDAP_SCOPE_BASE:
	case LDAP_SCOPE_SUBTREE:
		rs->sr_err = config_send( op, rs, ce, 0 );
		break;
		
	case LDAP_SCOPE_ONELEVEL:
		for (ce = ce->ce_kids; ce; ce=ce->ce_sibs) {
			rs->sr_err = config_send( op, rs, ce, 1 );
			if ( rs->sr_err ) {
				break;
			}
		}
		break;
	}

out:
	send_ldap_result( op, rs );
	return rs->sr_err;
}

/* no-op, we never free entries */
int config_entry_release(
	Operation *op,
	Entry *e,
	int rw )
{
	if ( !e->e_private ) {
		entry_free( e );
	}
	return LDAP_SUCCESS;
}

/* return LDAP_SUCCESS IFF we can retrieve the specified entry.
 */
int config_back_entry_get(
	Operation *op,
	struct berval *ndn,
	ObjectClass *oc,
	AttributeDescription *at,
	int rw,
	Entry **ent )
{
	CfBackInfo *cfb;
	CfEntryInfo *ce, *last;
	int rc = LDAP_NO_SUCH_OBJECT;

	cfb = (CfBackInfo *)op->o_bd->be_private;

	ce = config_find_base( cfb->cb_root, ndn, &last );
	if ( ce ) {
		*ent = ce->ce_entry;
		if ( *ent ) {
			rc = LDAP_SUCCESS;
			if ( oc && !is_entry_objectclass_or_sub( *ent, oc ) ) {
				rc = LDAP_NO_SUCH_ATTRIBUTE;
				*ent = NULL;
			}
		}
	}

	return rc;
}

static int
config_build_attrs( Entry *e, AttributeType **at, AttributeDescription *ad,
	ConfigTable *ct, ConfigArgs *c )
{
	int i, rc;

	for (; at && *at; at++) {
		/* Skip the naming attr */
		if ((*at)->sat_ad == ad || (*at)->sat_ad == slap_schema.si_ad_cn )
			continue;
		for (i=0;ct[i].name;i++) {
			if (ct[i].ad == (*at)->sat_ad) {
				rc = config_get_vals(&ct[i], c);
				/* NOTE: tolerate that config_get_vals()
				 * returns success with no values */
				if (rc == LDAP_SUCCESS && c->rvalue_vals != NULL ) {
					if ( c->rvalue_nvals )
						rc = attr_merge(e, ct[i].ad, c->rvalue_vals,
							c->rvalue_nvals);
					else {
						slap_syntax_validate_func *validate =
							ct[i].ad->ad_type->sat_syntax->ssyn_validate;
						if ( validate ) {
							int j;
							for ( j=0; c->rvalue_vals[j].bv_val; j++ ) {
								rc = ordered_value_validate( ct[i].ad,
									&c->rvalue_vals[j], LDAP_MOD_ADD );
								if ( rc ) {
									Debug( LDAP_DEBUG_ANY,
										"config_build_attrs: error %d on %s value #%d\n",
										rc, ct[i].ad->ad_cname.bv_val, j );
									return rc;
								}
							}
						}
							
						rc = attr_merge_normalize(e, ct[i].ad,
							c->rvalue_vals, NULL);
					}
					ber_bvarray_free( c->rvalue_nvals );
					ber_bvarray_free( c->rvalue_vals );
					if ( rc ) {
						Debug( LDAP_DEBUG_ANY,
							"config_build_attrs: error %d on %s\n",
							rc, ct[i].ad->ad_cname.bv_val, 0 );
						return rc;
					}
				}
				break;
			}
		}
	}
	return 0;
}

/* currently (2010) does not access rs except possibly writing rs->sr_err */

Entry *
config_build_entry( Operation *op, SlapReply *rs, CfEntryInfo *parent,
	ConfigArgs *c, struct berval *rdn, ConfigOCs *main, ConfigOCs *extra )
{
	Entry *e = entry_alloc();
	CfEntryInfo *ce = ch_calloc( 1, sizeof(CfEntryInfo) );
	struct berval val;
	struct berval ad_name;
	AttributeDescription *ad = NULL;
	int rc;
	char *ptr;
	const char *text = "";
	Attribute *oc_at;
	struct berval pdn;
	ObjectClass *oc;
	CfEntryInfo *ceprev = NULL;

	Debug( LDAP_DEBUG_TRACE, "config_build_entry: \"%s\"\n", rdn->bv_val, 0, 0);
	e->e_private = ce;
	ce->ce_entry = e;
	ce->ce_type = main->co_type;
	ce->ce_parent = parent;
	if ( parent ) {
		pdn = parent->ce_entry->e_nname;
		if ( parent->ce_kids && parent->ce_kids->ce_type <= ce->ce_type )
			for ( ceprev = parent->ce_kids; ceprev->ce_sibs &&
				ceprev->ce_type <= ce->ce_type;
				ceprev = ceprev->ce_sibs );
	} else {
		BER_BVZERO( &pdn );
	}

	ce->ce_private = c->ca_private;
	ce->ce_be = c->be;
	ce->ce_bi = c->bi;

	build_new_dn( &e->e_name, &pdn, rdn, NULL );
	ber_dupbv( &e->e_nname, &e->e_name );

	attr_merge_normalize_one(e, slap_schema.si_ad_objectClass,
		main->co_name, NULL );
	if ( extra )
		attr_merge_normalize_one(e, slap_schema.si_ad_objectClass,
			extra->co_name, NULL );
	ptr = strchr(rdn->bv_val, '=');
	ad_name.bv_val = rdn->bv_val;
	ad_name.bv_len = ptr - rdn->bv_val;
	rc = slap_bv2ad( &ad_name, &ad, &text );
	if ( rc ) {
		goto fail;
	}
	val.bv_val = ptr+1;
	val.bv_len = rdn->bv_len - (val.bv_val - rdn->bv_val);
	attr_merge_normalize_one(e, ad, &val, NULL );

	oc = main->co_oc;
	c->table = main->co_type;
	if ( oc->soc_required ) {
		rc = config_build_attrs( e, oc->soc_required, ad, main->co_table, c );
		if ( rc ) goto fail;
	}

	if ( oc->soc_allowed ) {
		rc = config_build_attrs( e, oc->soc_allowed, ad, main->co_table, c );
		if ( rc ) goto fail;
	}

	if ( extra ) {
		oc = extra->co_oc;
		c->table = extra->co_type;
		if ( oc->soc_required ) {
			rc = config_build_attrs( e, oc->soc_required, ad, extra->co_table, c );
			if ( rc ) goto fail;
		}

		if ( oc->soc_allowed ) {
			rc = config_build_attrs( e, oc->soc_allowed, ad, extra->co_table, c );
			if ( rc ) goto fail;
		}
	}

	oc_at = attr_find( e->e_attrs, slap_schema.si_ad_objectClass );
	rc = structural_class(oc_at->a_vals, &oc, NULL, &text, c->cr_msg,
		sizeof(c->cr_msg), op ? op->o_tmpmemctx : NULL );
	if ( rc != LDAP_SUCCESS ) {
fail:
		Debug( LDAP_DEBUG_ANY,
			"config_build_entry: build \"%s\" failed: \"%s\"\n",
			rdn->bv_val, text, 0);
		return NULL;
	}
	attr_merge_normalize_one(e, slap_schema.si_ad_structuralObjectClass, &oc->soc_cname, NULL );
	if ( op ) {
		op->ora_e = e;
		op->ora_modlist = NULL;
		slap_add_opattrs( op, NULL, NULL, 0, 0 );
		if ( !op->o_noop ) {
			SlapReply rs2 = {REP_RESULT};
			op->o_bd->be_add( op, &rs2 );
			rs->sr_err = rs2.sr_err;
			rs_assert_done( &rs2 );
			if ( ( rs2.sr_err != LDAP_SUCCESS ) 
					&& (rs2.sr_err != LDAP_ALREADY_EXISTS) ) {
				goto fail;
			}
		}
	}
	if ( ceprev ) {
		ce->ce_sibs = ceprev->ce_sibs;
		ceprev->ce_sibs = ce;
	} else if ( parent ) {
		ce->ce_sibs = parent->ce_kids;
		parent->ce_kids = ce;
	}

	return e;
}

static int
config_build_schema_inc( ConfigArgs *c, CfEntryInfo *ceparent,
	Operation *op, SlapReply *rs )
{
	Entry *e;
	ConfigFile *cf = c->ca_private;
	char *ptr;
	struct berval bv, rdn;

	for (; cf; cf=cf->c_sibs, c->depth++) {
		if ( !cf->c_at_head && !cf->c_cr_head && !cf->c_oc_head &&
			!cf->c_om_head && !cf->c_syn_head && !cf->c_kids ) continue;
		c->value_dn.bv_val = c->log;
		LUTIL_SLASHPATH( cf->c_file.bv_val );
		bv.bv_val = strrchr(cf->c_file.bv_val, LDAP_DIRSEP[0]);
		if ( !bv.bv_val ) {
			bv = cf->c_file;
		} else {
			bv.bv_val++;
			bv.bv_len = cf->c_file.bv_len - (bv.bv_val - cf->c_file.bv_val);
		}
		ptr = strchr( bv.bv_val, '.' );
		if ( ptr )
			bv.bv_len = ptr - bv.bv_val;
		c->value_dn.bv_len = snprintf(c->value_dn.bv_val, sizeof( c->log ), "cn=" SLAP_X_ORDERED_FMT, c->depth);
		if ( c->value_dn.bv_len >= sizeof( c->log ) ) {
			/* FIXME: how can indicate error? */
			return -1;
		}
		strncpy( c->value_dn.bv_val + c->value_dn.bv_len, bv.bv_val,
			bv.bv_len );
		c->value_dn.bv_len += bv.bv_len;
		c->value_dn.bv_val[c->value_dn.bv_len] ='\0';
		rdnNormalize( 0, NULL, NULL, &c->value_dn, &rdn, NULL );

		c->ca_private = cf;
		e = config_build_entry( op, rs, ceparent, c, &rdn,
			&CFOC_SCHEMA, NULL );
		ch_free( rdn.bv_val );
		if ( !e ) {
			return -1;
		} else if ( e && cf->c_kids ) {
			c->ca_private = cf->c_kids;
			config_build_schema_inc( c, e->e_private, op, rs );
		}
	}
	return 0;
}

#ifdef SLAPD_MODULES

static int
config_build_modules( ConfigArgs *c, CfEntryInfo *ceparent,
	Operation *op, SlapReply *rs )
{
	int i;
	ModPaths *mp;

	for (i=0, mp=&modpaths; mp; mp=mp->mp_next, i++) {
		if ( BER_BVISNULL( &mp->mp_path ) && !mp->mp_loads )
			continue;
		c->value_dn.bv_val = c->log;
		c->value_dn.bv_len = snprintf(c->value_dn.bv_val, sizeof( c->log ), "cn=module" SLAP_X_ORDERED_FMT, i);
		if ( c->value_dn.bv_len >= sizeof( c->log ) ) {
			/* FIXME: how can indicate error? */
			return -1;
		}
		c->ca_private = mp;
		if ( ! config_build_entry( op, rs, ceparent, c, &c->value_dn, &CFOC_MODULE, NULL )) {
			return -1;
		}
	}
        return 0;
}
#endif

static int
config_check_schema(Operation *op, CfBackInfo *cfb)
{
	struct berval schema_dn = BER_BVC(SCHEMA_RDN "," CONFIG_RDN);
	ConfigArgs c = {0};
	CfEntryInfo *ce, *last;
	Entry *e;

	/* If there's no root entry, we must be in the midst of converting */
	if ( !cfb->cb_root )
		return 0;

	/* Make sure the main schema entry exists */
	ce = config_find_base( cfb->cb_root, &schema_dn, &last );
	if ( ce ) {
		Attribute *a;
		struct berval *bv;

		e = ce->ce_entry;

		/* Make sure it's up to date */
		if ( cf_om_tail != om_sys_tail ) {
			a = attr_find( e->e_attrs, cfAd_om );
			if ( a ) {
				if ( a->a_nvals != a->a_vals )
					ber_bvarray_free( a->a_nvals );
				ber_bvarray_free( a->a_vals );
				a->a_vals = NULL;
				a->a_nvals = NULL;
				a->a_numvals = 0;
			}
			oidm_unparse( &bv, NULL, NULL, 1 );
			attr_merge_normalize( e, cfAd_om, bv, NULL );
			ber_bvarray_free( bv );
			cf_om_tail = om_sys_tail;
		}
		if ( cf_at_tail != at_sys_tail ) {
			a = attr_find( e->e_attrs, cfAd_attr );
			if ( a ) {
				if ( a->a_nvals != a->a_vals )
					ber_bvarray_free( a->a_nvals );
				ber_bvarray_free( a->a_vals );
				a->a_vals = NULL;
				a->a_nvals = NULL;
				a->a_numvals = 0;
			}
			at_unparse( &bv, NULL, NULL, 1 );
			attr_merge_normalize( e, cfAd_attr, bv, NULL );
			ber_bvarray_free( bv );
			cf_at_tail = at_sys_tail;
		}
		if ( cf_oc_tail != oc_sys_tail ) {
			a = attr_find( e->e_attrs, cfAd_oc );
			if ( a ) {
				if ( a->a_nvals != a->a_vals )
					ber_bvarray_free( a->a_nvals );
				ber_bvarray_free( a->a_vals );
				a->a_vals = NULL;
				a->a_nvals = NULL;
				a->a_numvals = 0;
			}
			oc_unparse( &bv, NULL, NULL, 1 );
			attr_merge_normalize( e, cfAd_oc, bv, NULL );
			ber_bvarray_free( bv );
			cf_oc_tail = oc_sys_tail;
		}
		if ( cf_syn_tail != syn_sys_tail ) {
			a = attr_find( e->e_attrs, cfAd_syntax );
			if ( a ) {
				if ( a->a_nvals != a->a_vals )
					ber_bvarray_free( a->a_nvals );
				ber_bvarray_free( a->a_vals );
				a->a_vals = NULL;
				a->a_nvals = NULL;
				a->a_numvals = 0;
			}
			syn_unparse( &bv, NULL, NULL, 1 );
			attr_merge_normalize( e, cfAd_syntax, bv, NULL );
			ber_bvarray_free( bv );
			cf_syn_tail = syn_sys_tail;
		}
	} else {
		SlapReply rs = {REP_RESULT};
		c.ca_private = NULL;
		e = config_build_entry( op, &rs, cfb->cb_root, &c, &schema_rdn,
			&CFOC_SCHEMA, NULL );
		if ( !e ) {
			return -1;
		}
		ce = e->e_private;
		ce->ce_private = cfb->cb_config;
		cf_at_tail = at_sys_tail;
		cf_oc_tail = oc_sys_tail;
		cf_om_tail = om_sys_tail;
		cf_syn_tail = syn_sys_tail;
	}
	return 0;
}

static const char *defacl[] = {
	NULL, "to", "*", "by", "*", "none", NULL
};

static int
config_back_db_open( BackendDB *be, ConfigReply *cr )
{
	CfBackInfo *cfb = be->be_private;
	struct berval rdn;
	Entry *e, *parent;
	CfEntryInfo *ce, *ceparent;
	int i, unsupp = 0;
	BackendInfo *bi;
	ConfigArgs c;
	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;
	slap_callback cb = { NULL, slap_null_cb, NULL, NULL };
	SlapReply rs = {REP_RESULT};
	void *thrctx = NULL;
	AccessControl *save_access;

	Debug( LDAP_DEBUG_TRACE, "config_back_db_open\n", 0, 0, 0);

	/* If we have no explicitly configured ACLs, don't just use
	 * the global ACLs. Explicitly deny access to everything.
	 */
	save_access = be->bd_self->be_acl;
	be->bd_self->be_acl = NULL;
	parse_acl(be->bd_self, "config_back_db_open", 0, 6, (char **)defacl, 0 );
	defacl_parsed = be->bd_self->be_acl;
	if ( save_access ) {
		be->bd_self->be_acl = save_access;
	} else {
		Debug( LDAP_DEBUG_CONFIG, "config_back_db_open: "
				"No explicit ACL for back-config configured. "
				"Using hardcoded default\n", 0, 0, 0 );
	}

	thrctx = ldap_pvt_thread_pool_context();
	connection_fake_init( &conn, &opbuf, thrctx );
	op = &opbuf.ob_op;

	op->o_tag = LDAP_REQ_ADD;
	op->o_callback = &cb;
	op->o_bd = &cfb->cb_db;
	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;

	if ( !cfb->cb_use_ldif ) {
		op->o_noop = 1;
	}

	/* If we read the config from back-ldif, do some quick sanity checks */
	if ( cfb->cb_got_ldif ) {
		return config_check_schema( op, cfb );
	}

	/* create root of tree */
	rdn = config_rdn;
	c.ca_private = cfb->cb_config;
	c.be = frontendDB;
	e = config_build_entry( op, &rs, NULL, &c, &rdn, &CFOC_GLOBAL, NULL );
	if ( !e ) {
		return -1;
	}
	ce = e->e_private;
	cfb->cb_root = ce;

	parent = e;
	ceparent = ce;

#ifdef SLAPD_MODULES
	/* Create Module nodes... */
	if ( modpaths.mp_loads ) {
		if ( config_build_modules( &c, ceparent, op, &rs ) ){
			return -1;
		}
	}
#endif

	/* Create schema nodes... cn=schema will contain the hardcoded core
	 * schema, read-only. Child objects will contain runtime loaded schema
	 * files.
	 */
	rdn = schema_rdn;
	c.ca_private = NULL;
	e = config_build_entry( op, &rs, ceparent, &c, &rdn, &CFOC_SCHEMA, NULL );
	if ( !e ) {
		return -1;
	}
	ce = e->e_private;
	ce->ce_private = cfb->cb_config;
	cf_at_tail = at_sys_tail;
	cf_oc_tail = oc_sys_tail;
	cf_om_tail = om_sys_tail;
	cf_syn_tail = syn_sys_tail;

	/* Create schema nodes for included schema... */
	if ( cfb->cb_config->c_kids ) {
		int rc;
		c.depth = 0;
		c.ca_private = cfb->cb_config->c_kids;
		rc = config_build_schema_inc( &c, ce, op, &rs );
		if ( rc ) {
			return -1;
		}
	}

	/* Create backend nodes. Skip if they don't provide a cf_table.
	 * There usually aren't any of these.
	 */
	
	c.line = 0;
	LDAP_STAILQ_FOREACH( bi, &backendInfo, bi_next) {
		if (!bi->bi_cf_ocs) {
			/* If it only supports the old config mech, complain. */
			if ( bi->bi_config ) {
				Debug( LDAP_DEBUG_ANY,
					"WARNING: No dynamic config support for backend %s.\n",
					bi->bi_type, 0, 0 );
				unsupp++;
			}
			continue;
		}
		if (!bi->bi_private) continue;

		rdn.bv_val = c.log;
		rdn.bv_len = snprintf(rdn.bv_val, sizeof( c.log ),
			"%s=%s", cfAd_backend->ad_cname.bv_val, bi->bi_type);
		if ( rdn.bv_len >= sizeof( c.log ) ) {
			/* FIXME: holler ... */ ;
		}
		c.bi = bi;
		e = config_build_entry( op, &rs, ceparent, &c, &rdn, &CFOC_BACKEND,
			bi->bi_cf_ocs );
		if ( !e ) {
			return -1;
		}
	}

	/* Create database nodes... */
	frontendDB->be_cf_ocs = &CFOC_FRONTEND;
	LDAP_STAILQ_NEXT(frontendDB, be_next) = LDAP_STAILQ_FIRST(&backendDB);
	for ( i = -1, be = frontendDB ; be;
		i++, be = LDAP_STAILQ_NEXT( be, be_next )) {
		slap_overinfo *oi = NULL;

		if ( overlay_is_over( be )) {
			oi = be->bd_info->bi_private;
			bi = oi->oi_orig;
		} else {
			bi = be->bd_info;
		}

		/* If this backend supports the old config mechanism, but not
		 * the new mech, complain.
		 */
		if ( !be->be_cf_ocs && bi->bi_db_config ) {
			Debug( LDAP_DEBUG_ANY,
				"WARNING: No dynamic config support for database %s.\n",
				bi->bi_type, 0, 0 );
			unsupp++;
		}
		rdn.bv_val = c.log;
		rdn.bv_len = snprintf(rdn.bv_val, sizeof( c.log ),
			"%s=" SLAP_X_ORDERED_FMT "%s", cfAd_database->ad_cname.bv_val,
			i, bi->bi_type);
		if ( rdn.bv_len >= sizeof( c.log ) ) {
			/* FIXME: holler ... */ ;
		}
		c.be = be;
		c.bi = bi;
		e = config_build_entry( op, &rs, ceparent, &c, &rdn, &CFOC_DATABASE,
			be->be_cf_ocs );
		if ( !e ) {
			return -1;
		}
		ce = e->e_private;
		if ( be->be_cf_ocs && be->be_cf_ocs->co_cfadd ) {
			rs_reinit( &rs, REP_RESULT );
			be->be_cf_ocs->co_cfadd( op, &rs, e, &c );
		}
		/* Iterate through overlays */
		if ( oi ) {
			slap_overinst *on;
			Entry *oe;
			int j;
			voidList *vl, *v0 = NULL;

			/* overlays are in LIFO order, must reverse stack */
			for (on=oi->oi_list; on; on=on->on_next) {
				vl = ch_malloc( sizeof( voidList ));
				vl->vl_next = v0;
				v0 = vl;
				vl->vl_ptr = on;
			}
			for (j=0; vl; j++,vl=v0) {
				on = vl->vl_ptr;
				v0 = vl->vl_next;
				ch_free( vl );
				if ( on->on_bi.bi_db_config && !on->on_bi.bi_cf_ocs ) {
					Debug( LDAP_DEBUG_ANY,
						"WARNING: No dynamic config support for overlay %s.\n",
						on->on_bi.bi_type, 0, 0 );
					unsupp++;
				}
				rdn.bv_val = c.log;
				rdn.bv_len = snprintf(rdn.bv_val, sizeof( c.log ),
					"%s=" SLAP_X_ORDERED_FMT "%s",
					cfAd_overlay->ad_cname.bv_val, j, on->on_bi.bi_type );
				if ( rdn.bv_len >= sizeof( c.log ) ) {
					/* FIXME: holler ... */ ;
				}
				c.be = be;
				c.bi = &on->on_bi;
				oe = config_build_entry( op, &rs, ce, &c, &rdn,
					&CFOC_OVERLAY, c.bi->bi_cf_ocs );
				if ( !oe ) {
					return -1;
				}
				if ( c.bi->bi_cf_ocs && c.bi->bi_cf_ocs->co_cfadd ) {
					rs_reinit( &rs, REP_RESULT );
					c.bi->bi_cf_ocs->co_cfadd( op, &rs, oe, &c );
				}
			}
		}
	}
	if ( thrctx )
		ldap_pvt_thread_pool_context_reset( thrctx );

	if ( unsupp  && cfb->cb_use_ldif ) {
		Debug( LDAP_DEBUG_ANY, "\nWARNING: The converted cn=config "
			"directory is incomplete and may not work.\n\n", 0, 0, 0 );
	}

	return 0;
}

static void
cfb_free_cffile( ConfigFile *cf )
{
	ConfigFile *next;

	for (; cf; cf=next) {
		next = cf->c_sibs;
		if ( cf->c_kids )
			cfb_free_cffile( cf->c_kids );
		ch_free( cf->c_file.bv_val );
		ber_bvarray_free( cf->c_dseFiles );
		ch_free( cf );
	}
}

static void
cfb_free_entries( CfEntryInfo *ce )
{
	CfEntryInfo *next;

	for (; ce; ce=next) {
		next = ce->ce_sibs;
		if ( ce->ce_kids )
			cfb_free_entries( ce->ce_kids );
		ce->ce_entry->e_private = NULL;
		entry_free( ce->ce_entry );
		ch_free( ce );
	}
}

static int
config_back_db_close( BackendDB *be, ConfigReply *cr )
{
	CfBackInfo *cfb = be->be_private;

	cfb_free_entries( cfb->cb_root );
	cfb->cb_root = NULL;

	if ( cfb->cb_db.bd_info ) {
		backend_shutdown( &cfb->cb_db );
	}

	if ( defacl_parsed && be->be_acl != defacl_parsed ) {
		acl_free( defacl_parsed );
		defacl_parsed = NULL;
	}

	return 0;
}

static int
config_back_db_destroy( BackendDB *be, ConfigReply *cr )
{
	CfBackInfo *cfb = be->be_private;

	cfb_free_cffile( cfb->cb_config );

	ch_free( cfdir.bv_val );

	avl_free( CfOcTree, NULL );

	if ( cfb->cb_db.bd_info ) {
		cfb->cb_db.be_suffix = NULL;
		cfb->cb_db.be_nsuffix = NULL;
		BER_BVZERO( &cfb->cb_db.be_rootdn );
		BER_BVZERO( &cfb->cb_db.be_rootndn );

		backend_destroy_one( &cfb->cb_db, 0 );
	}

	loglevel_destroy();

	return 0;
}

static int
config_back_db_init( BackendDB *be, ConfigReply* cr )
{
	struct berval dn;
	CfBackInfo *cfb;

	cfb = &cfBackInfo;
	cfb->cb_config = ch_calloc( 1, sizeof(ConfigFile));
	cfn = cfb->cb_config;
	be->be_private = cfb;

	ber_dupbv( &be->be_rootdn, &config_rdn );
	ber_dupbv( &be->be_rootndn, &be->be_rootdn );
	ber_dupbv( &dn, &be->be_rootdn );
	ber_bvarray_add( &be->be_suffix, &dn );
	ber_dupbv( &dn, &be->be_rootdn );
	ber_bvarray_add( &be->be_nsuffix, &dn );

	/* Hide from namingContexts */
	SLAP_BFLAGS(be) |= SLAP_BFLAG_CONFIG;

	/* Check ACLs on content of Adds by default */
	SLAP_DBFLAGS(be) |= SLAP_DBFLAG_ACL_ADD;

	return 0;
}

static int
config_back_destroy( BackendInfo *bi )
{
	ldif_must_b64_encode_release();
	return 0;
}

static int
config_tool_entry_open( BackendDB *be, int mode )
{
	CfBackInfo *cfb = be->be_private;
	BackendInfo *bi = cfb->cb_db.bd_info;

	if ( bi && bi->bi_tool_entry_open )
		return bi->bi_tool_entry_open( &cfb->cb_db, mode );
	else
		return -1;
	
}

static int
config_tool_entry_close( BackendDB *be )
{
	CfBackInfo *cfb = be->be_private;
	BackendInfo *bi = cfb->cb_db.bd_info;

	if ( bi && bi->bi_tool_entry_close )
		return bi->bi_tool_entry_close( &cfb->cb_db );
	else
		return -1;
}

static ID
config_tool_entry_first( BackendDB *be )
{
	CfBackInfo *cfb = be->be_private;
	BackendInfo *bi = cfb->cb_db.bd_info;

	if ( bi && bi->bi_tool_entry_first ) {
		return bi->bi_tool_entry_first( &cfb->cb_db );
	}
	if ( bi && bi->bi_tool_entry_first_x ) {
		return bi->bi_tool_entry_first_x( &cfb->cb_db,
			NULL, LDAP_SCOPE_DEFAULT, NULL );
	}
	return NOID;
}

static ID
config_tool_entry_first_x(
	BackendDB *be,
	struct berval *base,
	int scope,
	Filter *f )
{
	CfBackInfo *cfb = be->be_private;
	BackendInfo *bi = cfb->cb_db.bd_info;

	if ( bi && bi->bi_tool_entry_first_x ) {
		return bi->bi_tool_entry_first_x( &cfb->cb_db, base, scope, f );
	}
	return NOID;
}

static ID
config_tool_entry_next( BackendDB *be )
{
	CfBackInfo *cfb = be->be_private;
	BackendInfo *bi = cfb->cb_db.bd_info;

	if ( bi && bi->bi_tool_entry_next )
		return bi->bi_tool_entry_next( &cfb->cb_db );
	else
		return NOID;
}

static Entry *
config_tool_entry_get( BackendDB *be, ID id )
{
	CfBackInfo *cfb = be->be_private;
	BackendInfo *bi = cfb->cb_db.bd_info;

	if ( bi && bi->bi_tool_entry_get )
		return bi->bi_tool_entry_get( &cfb->cb_db, id );
	else
		return NULL;
}

static int entry_put_got_frontend=0;
static int entry_put_got_config=0;
static ID
config_tool_entry_put( BackendDB *be, Entry *e, struct berval *text )
{
	CfBackInfo *cfb = be->be_private;
	BackendInfo *bi = cfb->cb_db.bd_info;
	int rc;
	struct berval rdn, vals[ 2 ];
	ConfigArgs ca;
	OperationBuffer opbuf;
	Entry *ce;
	Connection conn = {0};
	Operation *op = NULL;
	void *thrctx;
	int isFrontend = 0;
	int isFrontendChild = 0;

	/* Create entry for frontend database if it does not exist already */
	if ( !entry_put_got_frontend ) {
		if ( !strncmp( e->e_nname.bv_val, "olcDatabase", 
				STRLENOF( "olcDatabase" ))) {
			if ( strncmp( e->e_nname.bv_val + 
					STRLENOF( "olcDatabase" ), "={-1}frontend",
					STRLENOF( "={-1}frontend" )) && 
					strncmp( e->e_nname.bv_val + 
					STRLENOF( "olcDatabase" ), "=frontend",
					STRLENOF( "=frontend" ))) {
				vals[1].bv_len = 0;
				vals[1].bv_val = NULL;
				memset( &ca, 0, sizeof(ConfigArgs));
				ca.be = frontendDB;
				ca.bi = frontendDB->bd_info;
				ca.be->be_cf_ocs = &CFOC_FRONTEND;
				rdn.bv_val = ca.log;
				rdn.bv_len = snprintf(rdn.bv_val, sizeof( ca.log ),
					"%s=" SLAP_X_ORDERED_FMT "%s",
					cfAd_database->ad_cname.bv_val, -1,
					ca.bi->bi_type);
				ce = config_build_entry( NULL, NULL, cfb->cb_root, &ca, &rdn,
						&CFOC_DATABASE, ca.be->be_cf_ocs );
				thrctx = ldap_pvt_thread_pool_context();
				connection_fake_init2( &conn, &opbuf, thrctx,0 );
				op = &opbuf.ob_op;
				op->o_bd = &cfb->cb_db;
				op->o_tag = LDAP_REQ_ADD;
				op->ora_e = ce;
				op->o_dn = be->be_rootdn;
				op->o_ndn = be->be_rootndn;
				rc = slap_add_opattrs(op, NULL, NULL, 0, 0);
				if ( rc != LDAP_SUCCESS ) {
					text->bv_val = "autocreation of \"olcDatabase={-1}frontend\" failed";
					text->bv_len = STRLENOF("autocreation of \"olcDatabase={-1}frontend\" failed");
					return NOID;
				}

				if ( ce && bi && bi->bi_tool_entry_put && 
						bi->bi_tool_entry_put( &cfb->cb_db, ce, text ) != NOID ) {
					entry_put_got_frontend++;
				} else {
					text->bv_val = "autocreation of \"olcDatabase={-1}frontend\" failed";
					text->bv_len = STRLENOF("autocreation of \"olcDatabase={-1}frontend\" failed");
					return NOID;
				}
			} else {
				if ( !strncmp( e->e_nname.bv_val + 
					STRLENOF( "olcDatabase" ), "=frontend",
					STRLENOF( "=frontend" ) ) )
				{
					struct berval rdn, pdn, ndn;
					dnParent( &e->e_nname, &pdn );
					rdn.bv_val = ca.log;
					rdn.bv_len = snprintf(rdn.bv_val, sizeof( ca.log ),
						"%s=" SLAP_X_ORDERED_FMT "%s",
						cfAd_database->ad_cname.bv_val, -1,
						frontendDB->bd_info->bi_type );
					build_new_dn( &ndn, &pdn, &rdn, NULL );
					ber_memfree( e->e_name.bv_val );
					e->e_name = ndn;
					ber_bvreplace( &e->e_nname, &e->e_name );
				}
				entry_put_got_frontend++;
				isFrontend = 1;
			}
		}
	}

	/* Child entries of the frontend database, e.g. slapo-chain's back-ldap
	 * instances, may appear before the config database entry in the ldif, skip
	 * auto-creation of olcDatabase={0}config in such a case */
	if ( !entry_put_got_config &&
			!strncmp( e->e_nname.bv_val, "olcDatabase", STRLENOF( "olcDatabase" ))) {
		struct berval pdn;
		dnParent( &e->e_nname, &pdn );
		while ( pdn.bv_len ) {
			if ( !strncmp( pdn.bv_val, "olcDatabase",
					STRLENOF( "olcDatabase" ))) {
				if ( !strncmp( pdn.bv_val +
						STRLENOF( "olcDatabase" ), "={-1}frontend",
						STRLENOF( "={-1}frontend" )) ||
						!strncmp( pdn.bv_val +
						STRLENOF( "olcDatabase" ), "=frontend",
						STRLENOF( "=frontend" ))) {

					isFrontendChild = 1;
					break;
				}
			}
			dnParent( &pdn, &pdn );
		}
	}

	/* Create entry for config database if it does not exist already */
	if ( !entry_put_got_config && !isFrontend && !isFrontendChild ) {
		if ( !strncmp( e->e_nname.bv_val, "olcDatabase",
				STRLENOF( "olcDatabase" ))) {
			if ( strncmp( e->e_nname.bv_val +
					STRLENOF( "olcDatabase" ), "={0}config",
					STRLENOF( "={0}config" )) &&
					strncmp( e->e_nname.bv_val +
					STRLENOF( "olcDatabase" ), "=config",
					STRLENOF( "=config" )) ) {
				vals[1].bv_len = 0;
				vals[1].bv_val = NULL;
				memset( &ca, 0, sizeof(ConfigArgs));
				ca.be = LDAP_STAILQ_FIRST( &backendDB );
				ca.bi = ca.be->bd_info;
				rdn.bv_val = ca.log;
				rdn.bv_len = snprintf(rdn.bv_val, sizeof( ca.log ),
					"%s=" SLAP_X_ORDERED_FMT "%s",
					cfAd_database->ad_cname.bv_val, 0,
					ca.bi->bi_type);
				ce = config_build_entry( NULL, NULL, cfb->cb_root, &ca, &rdn, &CFOC_DATABASE,
						ca.be->be_cf_ocs );
				if ( ! op ) {
					thrctx = ldap_pvt_thread_pool_context();
					connection_fake_init2( &conn, &opbuf, thrctx,0 );
					op = &opbuf.ob_op;
					op->o_bd = &cfb->cb_db;
					op->o_tag = LDAP_REQ_ADD;
					op->o_dn = be->be_rootdn;
					op->o_ndn = be->be_rootndn;
				}
				op->ora_e = ce;
				rc = slap_add_opattrs(op, NULL, NULL, 0, 0);
				if ( rc != LDAP_SUCCESS ) {
					text->bv_val = "autocreation of \"olcDatabase={0}config\" failed";
					text->bv_len = STRLENOF("autocreation of \"olcDatabase={0}config\" failed");
					return NOID;
				}
				if (ce && bi && bi->bi_tool_entry_put &&
						bi->bi_tool_entry_put( &cfb->cb_db, ce, text ) != NOID ) {
					entry_put_got_config++;
				} else {
					text->bv_val = "autocreation of \"olcDatabase={0}config\" failed";
					text->bv_len = STRLENOF("autocreation of \"olcDatabase={0}config\" failed");
					return NOID;
				}
			} else {
				entry_put_got_config++;
			}
		}
	}
	if ( bi && bi->bi_tool_entry_put &&
		config_add_internal( cfb, e, &ca, NULL, NULL, NULL ) == 0 )
		return bi->bi_tool_entry_put( &cfb->cb_db, e, text );
	else
		return NOID;
}

static struct {
	char *name;
	AttributeDescription **desc;
} ads[] = {
	{ "attribute", &cfAd_attr },
	{ "backend", &cfAd_backend },
	{ "database", &cfAd_database },
	{ "include", &cfAd_include },
	{ "ldapsyntax", &cfAd_syntax },
	{ "objectclass", &cfAd_oc },
	{ "objectidentifier", &cfAd_om },
	{ "overlay", &cfAd_overlay },
	{ NULL, NULL }
};

/* Notes:
 *   add / delete: all types that may be added or deleted must use an
 * X-ORDERED attributeType for their RDN. Adding and deleting entries
 * should automatically renumber the index of any siblings as needed,
 * so that no gaps in the numbering sequence exist after the add/delete
 * is completed.
 *   What can be added:
 *     schema objects
 *     backend objects for backend-specific config directives
 *     database objects
 *     overlay objects
 *
 *   delete: probably no support this time around.
 *
 *   modrdn: generally not done. Will be invoked automatically by add/
 * delete to update numbering sequence. Perform as an explicit operation
 * so that the renumbering effect may be replicated. Subtree rename must
 * be supported, since renumbering a database will affect all its child
 * overlays.
 *
 *  modify: must be fully supported. 
 */

int
config_back_initialize( BackendInfo *bi )
{
	ConfigTable		*ct = config_back_cf_table;
	ConfigArgs ca;
	char			*argv[4];
	int			i;
	AttributeDescription	*ad = NULL;
	const char		*text;
	static char		*controls[] = {
		LDAP_CONTROL_MANAGEDSAIT,
		NULL
	};

	/* Make sure we don't exceed the bits reserved for userland */
	config_check_userland( CFG_LAST );

	bi->bi_controls = controls;

	bi->bi_open = 0;
	bi->bi_close = 0;
	bi->bi_config = 0;
	bi->bi_destroy = config_back_destroy;

	bi->bi_db_init = config_back_db_init;
	bi->bi_db_config = 0;
	bi->bi_db_open = config_back_db_open;
	bi->bi_db_close = config_back_db_close;
	bi->bi_db_destroy = config_back_db_destroy;

	bi->bi_op_bind = config_back_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = config_back_search;
	bi->bi_op_compare = 0;
	bi->bi_op_modify = config_back_modify;
	bi->bi_op_modrdn = config_back_modrdn;
	bi->bi_op_add = config_back_add;
	bi->bi_op_delete = config_back_delete;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_chk_referrals = 0;

	bi->bi_access_allowed = slap_access_allowed;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	bi->bi_entry_release_rw = config_entry_release;
	bi->bi_entry_get_rw = config_back_entry_get;

	bi->bi_tool_entry_open = config_tool_entry_open;
	bi->bi_tool_entry_close = config_tool_entry_close;
	bi->bi_tool_entry_first = config_tool_entry_first;
	bi->bi_tool_entry_first_x = config_tool_entry_first_x;
	bi->bi_tool_entry_next = config_tool_entry_next;
	bi->bi_tool_entry_get = config_tool_entry_get;
	bi->bi_tool_entry_put = config_tool_entry_put;

	ca.argv = argv;
	argv[ 0 ] = "slapd";
	ca.argv = argv;
	ca.argc = 3;
	ca.fname = argv[0];

	argv[3] = NULL;
	for (i=0; OidMacros[i].name; i++ ) {
		argv[1] = OidMacros[i].name;
		argv[2] = OidMacros[i].oid;
		parse_oidm( &ca, 0, NULL );
	}

	bi->bi_cf_ocs = cf_ocs;

	i = config_register_schema( ct, cf_ocs );
	if ( i ) return i;

	i = slap_str2ad( "olcDatabase", &olcDatabaseDummy[0].ad, &text );
	if ( i ) return i;

	/* setup olcRootPW to be base64-encoded when written in LDIF form;
	 * basically, we don't care if it fails */
	i = slap_str2ad( "olcRootPW", &ad, &text );
	if ( i ) {
		Debug( LDAP_DEBUG_ANY, "config_back_initialize: "
			"warning, unable to get \"olcRootPW\" "
			"attribute description: %d: %s\n",
			i, text, 0 );
	} else {
		(void)ldif_must_b64_encode_register( ad->ad_cname.bv_val,
			ad->ad_type->sat_oid );
	}

	/* set up the notable AttributeDescriptions */
	i = 0;
	for (;ct->name;ct++) {
		if (strcmp(ct->name, ads[i].name)) continue;
		*ads[i].desc = ct->ad;
		i++;
		if (!ads[i].name) break;
	}

	return 0;
}
