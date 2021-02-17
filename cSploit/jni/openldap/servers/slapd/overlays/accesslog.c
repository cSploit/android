/* accesslog.c - log operations for audit/history purposes */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
 * Portions copyright 2004-2005 Symas Corporation.
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
 * This work was initially developed by Howard Chu for inclusion in
 * OpenLDAP Software.
 */

#include "portable.h"

#ifdef SLAPD_OVER_ACCESSLOG

#include <stdio.h>

#include <ac/string.h>
#include <ac/ctype.h>

#include "slap.h"
#include "config.h"
#include "lutil.h"
#include "ldap_rq.h"

#define LOG_OP_ADD	0x001
#define LOG_OP_DELETE	0x002
#define	LOG_OP_MODIFY	0x004
#define LOG_OP_MODRDN	0x008
#define	LOG_OP_COMPARE	0x010
#define	LOG_OP_SEARCH	0x020
#define LOG_OP_BIND	0x040
#define LOG_OP_UNBIND	0x080
#define	LOG_OP_ABANDON	0x100
#define	LOG_OP_EXTENDED	0x200
#define LOG_OP_UNKNOWN	0x400

#define	LOG_OP_WRITES	(LOG_OP_ADD|LOG_OP_DELETE|LOG_OP_MODIFY|LOG_OP_MODRDN)
#define	LOG_OP_READS	(LOG_OP_COMPARE|LOG_OP_SEARCH)
#define	LOG_OP_SESSION	(LOG_OP_BIND|LOG_OP_UNBIND|LOG_OP_ABANDON)
#define LOG_OP_ALL		(LOG_OP_READS|LOG_OP_WRITES|LOG_OP_SESSION| \
	LOG_OP_EXTENDED|LOG_OP_UNKNOWN)

typedef struct log_attr {
	struct log_attr *next;
	AttributeDescription *attr;
} log_attr;

typedef struct log_base {
	struct log_base *lb_next;
	slap_mask_t lb_ops;
	struct berval lb_base;
	struct berval lb_line;
} log_base;

typedef struct log_info {
	BackendDB *li_db;
	struct berval li_db_suffix;
	slap_mask_t li_ops;
	int li_age;
	int li_cycle;
	struct re_s *li_task;
	Filter *li_oldf;
	Entry *li_old;
	log_attr *li_oldattrs;
	struct berval li_uuid;
	int li_success;
	log_base *li_bases;
	ldap_pvt_thread_rmutex_t li_op_rmutex;
	ldap_pvt_thread_mutex_t li_log_mutex;
} log_info;

static ConfigDriver log_cf_gen;

enum {
	LOG_DB = 1,
	LOG_OPS,
	LOG_PURGE,
	LOG_SUCCESS,
	LOG_OLD,
	LOG_OLDATTR,
	LOG_BASE
};

static ConfigTable log_cfats[] = {
	{ "logdb", "suffix", 2, 2, 0, ARG_DN|ARG_MAGIC|LOG_DB,
		log_cf_gen, "( OLcfgOvAt:4.1 NAME 'olcAccessLogDB' "
			"DESC 'Suffix of database for log content' "
			"SUP distinguishedName SINGLE-VALUE )", NULL, NULL },
	{ "logops", "op|writes|reads|session|all", 2, 0, 0,
		ARG_MAGIC|LOG_OPS,
		log_cf_gen, "( OLcfgOvAt:4.2 NAME 'olcAccessLogOps' "
			"DESC 'Operation types to log' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "logpurge", "age> <interval", 3, 3, 0, ARG_MAGIC|LOG_PURGE,
		log_cf_gen, "( OLcfgOvAt:4.3 NAME 'olcAccessLogPurge' "
			"DESC 'Log cleanup parameters' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "logsuccess", NULL, 2, 2, 0, ARG_MAGIC|ARG_ON_OFF|LOG_SUCCESS,
		log_cf_gen, "( OLcfgOvAt:4.4 NAME 'olcAccessLogSuccess' "
			"DESC 'Log successful ops only' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "logold", "filter", 2, 2, 0, ARG_MAGIC|LOG_OLD,
		log_cf_gen, "( OLcfgOvAt:4.5 NAME 'olcAccessLogOld' "
			"DESC 'Log old values when modifying entries matching the filter' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "logoldattr", "attrs", 2, 0, 0, ARG_MAGIC|LOG_OLDATTR,
		log_cf_gen, "( OLcfgOvAt:4.6 NAME 'olcAccessLogOldAttr' "
			"DESC 'Log old values of these attributes even if unmodified' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "logbase", "op|writes|reads|session|all< <baseDN", 3, 3, 0,
		ARG_MAGIC|LOG_BASE,
		log_cf_gen, "( OLcfgOvAt:4.7 NAME 'olcAccessLogBase' "
			"DESC 'Operation types to log under a specific branch' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL }
};

static ConfigOCs log_cfocs[] = {
	{ "( OLcfgOvOc:4.1 "
		"NAME 'olcAccessLogConfig' "
		"DESC 'Access log configuration' "
		"SUP olcOverlayConfig "
		"MUST olcAccessLogDB "
		"MAY ( olcAccessLogOps $ olcAccessLogPurge $ olcAccessLogSuccess $ "
			"olcAccessLogOld $ olcAccessLogOldAttr $ olcAccessLogBase ) )",
			Cft_Overlay, log_cfats },
	{ NULL }
};

static slap_verbmasks logops[] = {
	{ BER_BVC("all"),		LOG_OP_ALL },
	{ BER_BVC("writes"),	LOG_OP_WRITES },
	{ BER_BVC("session"),	LOG_OP_SESSION },
	{ BER_BVC("reads"),		LOG_OP_READS },
	{ BER_BVC("add"),		LOG_OP_ADD },
	{ BER_BVC("delete"),	LOG_OP_DELETE },
	{ BER_BVC("modify"),	LOG_OP_MODIFY },
	{ BER_BVC("modrdn"),	LOG_OP_MODRDN },
	{ BER_BVC("compare"),	LOG_OP_COMPARE },
	{ BER_BVC("search"),	LOG_OP_SEARCH },
	{ BER_BVC("bind"),		LOG_OP_BIND },
	{ BER_BVC("unbind"),	LOG_OP_UNBIND },
	{ BER_BVC("abandon"),	LOG_OP_ABANDON },
	{ BER_BVC("extended"),	LOG_OP_EXTENDED },
	{ BER_BVC("unknown"),	LOG_OP_UNKNOWN },
	{ BER_BVNULL, 0 }
};

/* Start with "add" in logops */
#define EN_OFFSET	4

enum {
	LOG_EN_ADD = 0,
	LOG_EN_DELETE,
	LOG_EN_MODIFY,
	LOG_EN_MODRDN,
	LOG_EN_COMPARE,
	LOG_EN_SEARCH,
	LOG_EN_BIND,
	LOG_EN_UNBIND,
	LOG_EN_ABANDON,
	LOG_EN_EXTENDED,
	LOG_EN_UNKNOWN,
	LOG_EN__COUNT
};

static ObjectClass *log_ocs[LOG_EN__COUNT], *log_container,
	*log_oc_read, *log_oc_write;

#define LOG_SCHEMA_ROOT	"1.3.6.1.4.1.4203.666.11.5"

#define LOG_SCHEMA_AT LOG_SCHEMA_ROOT ".1"
#define LOG_SCHEMA_OC LOG_SCHEMA_ROOT ".2"
#define LOG_SCHEMA_SYN LOG_SCHEMA_ROOT ".3"

static AttributeDescription *ad_reqDN, *ad_reqStart, *ad_reqEnd, *ad_reqType,
	*ad_reqSession, *ad_reqResult, *ad_reqAuthzID, *ad_reqControls,
	*ad_reqRespControls, *ad_reqMethod, *ad_reqAssertion, *ad_reqNewRDN,
	*ad_reqNewSuperior, *ad_reqDeleteOldRDN, *ad_reqMod,
	*ad_reqScope, *ad_reqFilter, *ad_reqAttr, *ad_reqEntries,
	*ad_reqSizeLimit, *ad_reqTimeLimit, *ad_reqAttrsOnly, *ad_reqData,
	*ad_reqId, *ad_reqMessage, *ad_reqVersion, *ad_reqDerefAliases,
	*ad_reqReferral, *ad_reqOld, *ad_auditContext, *ad_reqEntryUUID;

static int
logSchemaControlValidate(
	Syntax		*syntax,
	struct berval	*val );

char	*mrControl[] = {
	"objectIdentifierFirstComponentMatch",
	NULL
};

static struct {
	char			*oid;
	slap_syntax_defs_rec	syn;
	char			**mrs;
} lsyntaxes[] = {
	{ LOG_SCHEMA_SYN ".1" ,
		{ "( " LOG_SCHEMA_SYN ".1 DESC 'Control' )",
			SLAP_SYNTAX_HIDE,
			NULL,
			logSchemaControlValidate,
			NULL },
		mrControl },
	{ NULL }
};

static struct {
	char *at;
	AttributeDescription **ad;
} lattrs[] = {
	{ "( " LOG_SCHEMA_AT ".1 NAME 'reqDN' "
		"DESC 'Target DN of request' "
		"EQUALITY distinguishedNameMatch "
		"SYNTAX OMsDN "
		"SINGLE-VALUE )", &ad_reqDN },
	{ "( " LOG_SCHEMA_AT ".2 NAME 'reqStart' "
		"DESC 'Start time of request' "
		"EQUALITY generalizedTimeMatch "
		"ORDERING generalizedTimeOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
		"SINGLE-VALUE )", &ad_reqStart },
	{ "( " LOG_SCHEMA_AT ".3 NAME 'reqEnd' "
		"DESC 'End time of request' "
		"EQUALITY generalizedTimeMatch "
		"ORDERING generalizedTimeOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
		"SINGLE-VALUE )", &ad_reqEnd },
	{ "( " LOG_SCHEMA_AT ".4 NAME 'reqType' "
		"DESC 'Type of request' "
		"EQUALITY caseIgnoreMatch "
		"SYNTAX OMsDirectoryString "
		"SINGLE-VALUE )", &ad_reqType },
	{ "( " LOG_SCHEMA_AT ".5 NAME 'reqSession' "
		"DESC 'Session ID of request' "
		"EQUALITY caseIgnoreMatch "
		"SYNTAX OMsDirectoryString "
		"SINGLE-VALUE )", &ad_reqSession },
	{ "( " LOG_SCHEMA_AT ".6 NAME 'reqAuthzID' "
		"DESC 'Authorization ID of requestor' "
		"EQUALITY distinguishedNameMatch "
		"SYNTAX OMsDN "
		"SINGLE-VALUE )", &ad_reqAuthzID },
	{ "( " LOG_SCHEMA_AT ".7 NAME 'reqResult' "
		"DESC 'Result code of request' "
		"EQUALITY integerMatch "
		"ORDERING integerOrderingMatch "
		"SYNTAX OMsInteger "
		"SINGLE-VALUE )", &ad_reqResult },
	{ "( " LOG_SCHEMA_AT ".8 NAME 'reqMessage' "
		"DESC 'Error text of request' "
		"EQUALITY caseIgnoreMatch "
		"SUBSTR caseIgnoreSubstringsMatch "
		"SYNTAX OMsDirectoryString "
		"SINGLE-VALUE )", &ad_reqMessage },
	{ "( " LOG_SCHEMA_AT ".9 NAME 'reqReferral' "
		"DESC 'Referrals returned for request' "
		"SUP labeledURI )", &ad_reqReferral },
	{ "( " LOG_SCHEMA_AT ".10 NAME 'reqControls' "
		"DESC 'Request controls' "
		"EQUALITY objectIdentifierFirstComponentMatch "
		"SYNTAX " LOG_SCHEMA_SYN ".1 "
		"X-ORDERED 'VALUES' )", &ad_reqControls },
	{ "( " LOG_SCHEMA_AT ".11 NAME 'reqRespControls' "
		"DESC 'Response controls of request' "
		"EQUALITY objectIdentifierFirstComponentMatch "
		"SYNTAX " LOG_SCHEMA_SYN ".1 "
		"X-ORDERED 'VALUES' )", &ad_reqRespControls },
	{ "( " LOG_SCHEMA_AT ".12 NAME 'reqId' "
		"DESC 'ID of Request to Abandon' "
		"EQUALITY integerMatch "
		"ORDERING integerOrderingMatch "
		"SYNTAX OMsInteger "
		"SINGLE-VALUE )", &ad_reqId },
	{ "( " LOG_SCHEMA_AT ".13 NAME 'reqVersion' "
		"DESC 'Protocol version of Bind request' "
		"EQUALITY integerMatch "
		"ORDERING integerOrderingMatch "
		"SYNTAX OMsInteger "
		"SINGLE-VALUE )", &ad_reqVersion },
	{ "( " LOG_SCHEMA_AT ".14 NAME 'reqMethod' "
		"DESC 'Bind method of request' "
		"EQUALITY caseIgnoreMatch "
		"SYNTAX OMsDirectoryString "
		"SINGLE-VALUE )", &ad_reqMethod },
	{ "( " LOG_SCHEMA_AT ".15 NAME 'reqAssertion' "
		"DESC 'Compare Assertion of request' "
		"SYNTAX OMsDirectoryString "
		"SINGLE-VALUE )", &ad_reqAssertion },
	{ "( " LOG_SCHEMA_AT ".16 NAME 'reqMod' "
		"DESC 'Modifications of request' "
		"EQUALITY octetStringMatch "
		"SUBSTR octetStringSubstringsMatch "
		"SYNTAX OMsOctetString )", &ad_reqMod },
	{ "( " LOG_SCHEMA_AT ".17 NAME 'reqOld' "
		"DESC 'Old values of entry before request completed' "
		"EQUALITY octetStringMatch "
		"SUBSTR octetStringSubstringsMatch "
		"SYNTAX OMsOctetString )", &ad_reqOld },
	{ "( " LOG_SCHEMA_AT ".18 NAME 'reqNewRDN' "
		"DESC 'New RDN of request' "
		"EQUALITY distinguishedNameMatch "
		"SYNTAX OMsDN "
		"SINGLE-VALUE )", &ad_reqNewRDN },
	{ "( " LOG_SCHEMA_AT ".19 NAME 'reqDeleteOldRDN' "
		"DESC 'Delete old RDN' "
		"EQUALITY booleanMatch "
		"SYNTAX OMsBoolean "
		"SINGLE-VALUE )", &ad_reqDeleteOldRDN },
	{ "( " LOG_SCHEMA_AT ".20 NAME 'reqNewSuperior' "
		"DESC 'New superior DN of request' "
		"EQUALITY distinguishedNameMatch "
		"SYNTAX OMsDN "
		"SINGLE-VALUE )", &ad_reqNewSuperior },
	{ "( " LOG_SCHEMA_AT ".21 NAME 'reqScope' "
		"DESC 'Scope of request' "
		"EQUALITY caseIgnoreMatch "
		"SYNTAX OMsDirectoryString "
		"SINGLE-VALUE )", &ad_reqScope },
	{ "( " LOG_SCHEMA_AT ".22 NAME 'reqDerefAliases' "
		"DESC 'Disposition of Aliases in request' "
		"EQUALITY caseIgnoreMatch "
		"SYNTAX OMsDirectoryString "
		"SINGLE-VALUE )", &ad_reqDerefAliases },
	{ "( " LOG_SCHEMA_AT ".23 NAME 'reqAttrsOnly' "
		"DESC 'Attributes and values of request' "
		"EQUALITY booleanMatch "
		"SYNTAX OMsBoolean "
		"SINGLE-VALUE )", &ad_reqAttrsOnly },
	{ "( " LOG_SCHEMA_AT ".24 NAME 'reqFilter' "
		"DESC 'Filter of request' "
		"EQUALITY caseIgnoreMatch "
		"SUBSTR caseIgnoreSubstringsMatch "
		"SYNTAX OMsDirectoryString "
		"SINGLE-VALUE )", &ad_reqFilter },
	{ "( " LOG_SCHEMA_AT ".25 NAME 'reqAttr' "
		"DESC 'Attributes of request' "
		"EQUALITY caseIgnoreMatch "
		"SYNTAX OMsDirectoryString )", &ad_reqAttr },
	{ "( " LOG_SCHEMA_AT ".26 NAME 'reqSizeLimit' "
		"DESC 'Size limit of request' "
		"EQUALITY integerMatch "
		"ORDERING integerOrderingMatch "
		"SYNTAX OMsInteger "
		"SINGLE-VALUE )", &ad_reqSizeLimit },
	{ "( " LOG_SCHEMA_AT ".27 NAME 'reqTimeLimit' "
		"DESC 'Time limit of request' "
		"EQUALITY integerMatch "
		"ORDERING integerOrderingMatch "
		"SYNTAX OMsInteger "
		"SINGLE-VALUE )", &ad_reqTimeLimit },
	{ "( " LOG_SCHEMA_AT ".28 NAME 'reqEntries' "
		"DESC 'Number of entries returned' "
		"EQUALITY integerMatch "
		"ORDERING integerOrderingMatch "
		"SYNTAX OMsInteger "
		"SINGLE-VALUE )", &ad_reqEntries },
	{ "( " LOG_SCHEMA_AT ".29 NAME 'reqData' "
		"DESC 'Data of extended request' "
		"EQUALITY octetStringMatch "
		"SUBSTR octetStringSubstringsMatch "
		"SYNTAX OMsOctetString "
		"SINGLE-VALUE )", &ad_reqData },

	/*
	 * from <draft-chu-ldap-logschema-01.txt>:
	 *

   ( LOG_SCHEMA_AT .30 NAME 'auditContext'
   DESC 'DN of auditContainer'
   EQUALITY distinguishedNameMatch
   SYNTAX 1.3.6.1.4.1.1466.115.121.1.12
   SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )

	 * - removed EQUALITY matchingRule
	 * - changed directoryOperation in dSAOperation
	 */
	{ "( " LOG_SCHEMA_AT ".30 NAME 'auditContext' "
		"DESC 'DN of auditContainer' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
		"SINGLE-VALUE "
		"NO-USER-MODIFICATION "
		"USAGE dSAOperation )", &ad_auditContext },

	/*
	 * ITS#6656
	 */
	{ "( " LOG_SCHEMA_AT ".31 NAME 'reqEntryUUID' "
		"DESC 'UUID of entry' "
		"EQUALITY UUIDMatch "
		"ORDERING UUIDOrderingMatch "
		"SYNTAX 1.3.6.1.1.16.1 "
		"SINGLE-VALUE )", &ad_reqEntryUUID },
	{ NULL, NULL }
};

static struct {
	char *ot;
	ObjectClass **oc;
} locs[] = {
	{ "( " LOG_SCHEMA_OC ".0 NAME 'auditContainer' "
		"DESC 'AuditLog container' "
		"SUP top STRUCTURAL "
		"MAY ( cn $ reqStart $ reqEnd ) )", &log_container },
	{ "( " LOG_SCHEMA_OC ".1 NAME 'auditObject' "
		"DESC 'OpenLDAP request auditing' "
		"SUP top STRUCTURAL "
		"MUST ( reqStart $ reqType $ reqSession ) "
		"MAY ( reqDN $ reqAuthzID $ reqControls $ reqRespControls $ reqEnd $ "
			"reqResult $ reqMessage $ reqReferral $ reqEntryUUID ) )",
				&log_ocs[LOG_EN_UNBIND] },
	{ "( " LOG_SCHEMA_OC ".2 NAME 'auditReadObject' "
		"DESC 'OpenLDAP read request record' "
		"SUP auditObject STRUCTURAL )", &log_oc_read },
	{ "( " LOG_SCHEMA_OC ".3 NAME 'auditWriteObject' "
		"DESC 'OpenLDAP write request record' "
		"SUP auditObject STRUCTURAL )", &log_oc_write },
	{ "( " LOG_SCHEMA_OC ".4 NAME 'auditAbandon' "
		"DESC 'Abandon operation' "
		"SUP auditObject STRUCTURAL "
		"MUST reqId )", &log_ocs[LOG_EN_ABANDON] },
	{ "( " LOG_SCHEMA_OC ".5 NAME 'auditAdd' "
		"DESC 'Add operation' "
		"SUP auditWriteObject STRUCTURAL "
		"MUST reqMod )", &log_ocs[LOG_EN_ADD] },
	{ "( " LOG_SCHEMA_OC ".6 NAME 'auditBind' "
		"DESC 'Bind operation' "
		"SUP auditObject STRUCTURAL "
		"MUST ( reqVersion $ reqMethod ) )", &log_ocs[LOG_EN_BIND] },
	{ "( " LOG_SCHEMA_OC ".7 NAME 'auditCompare' "
		"DESC 'Compare operation' "
		"SUP auditReadObject STRUCTURAL "
		"MUST reqAssertion )", &log_ocs[LOG_EN_COMPARE] },
	{ "( " LOG_SCHEMA_OC ".8 NAME 'auditDelete' "
		"DESC 'Delete operation' "
		"SUP auditWriteObject STRUCTURAL "
		"MAY reqOld )", &log_ocs[LOG_EN_DELETE] },
	{ "( " LOG_SCHEMA_OC ".9 NAME 'auditModify' "
		"DESC 'Modify operation' "
		"SUP auditWriteObject STRUCTURAL "
		"MAY reqOld MUST reqMod )", &log_ocs[LOG_EN_MODIFY] },
	{ "( " LOG_SCHEMA_OC ".10 NAME 'auditModRDN' "
		"DESC 'ModRDN operation' "
		"SUP auditWriteObject STRUCTURAL "
		"MUST ( reqNewRDN $ reqDeleteOldRDN ) "
		"MAY ( reqNewSuperior $ reqMod $ reqOld ) )", &log_ocs[LOG_EN_MODRDN] },
	{ "( " LOG_SCHEMA_OC ".11 NAME 'auditSearch' "
		"DESC 'Search operation' "
		"SUP auditReadObject STRUCTURAL "
		"MUST ( reqScope $ reqDerefAliases $ reqAttrsonly ) "
		"MAY ( reqFilter $ reqAttr $ reqEntries $ reqSizeLimit $ "
			"reqTimeLimit ) )", &log_ocs[LOG_EN_SEARCH] },
	{ "( " LOG_SCHEMA_OC ".12 NAME 'auditExtended' "
		"DESC 'Extended operation' "
		"SUP auditObject STRUCTURAL "
		"MAY reqData )", &log_ocs[LOG_EN_EXTENDED] },
	{ NULL, NULL }
};

#define	RDNEQ	"reqStart="

/* Our time intervals are of the form [ddd+]hh:mm[:ss]
 * If a field is present, it must be two digits. (Except for
 * days, which can be arbitrary width.)
 */
static int
log_age_parse(char *agestr)
{
	int t1, t2;
	int gotdays = 0;
	char *endptr;

	t1 = strtol( agestr, &endptr, 10 );
	/* Is there a days delimiter? */
	if ( *endptr == '+' ) {
		/* 32 bit time only covers about 68 years */
		if ( t1 < 0 || t1 > 25000 )
			return -1;
		t1 *= 24;
		gotdays = 1;
		agestr = endptr + 1;
	} else {
		if ( agestr[2] != ':' ) {
			/* No valid delimiter found, fail */
			return -1;
		}
		t1 *= 60;
		agestr += 3;
	}

	t2 = atoi( agestr );
	t1 += t2;

	if ( agestr[2] ) {
		/* if there's a delimiter, it can only be a colon */
		if ( agestr[2] != ':' )
			return -1;
	} else {
		/* If we're at the end of the string, and we started with days,
		 * fail because we expected to find minutes too.
		 */
		return gotdays ? -1 : t1 * 60;
	}

	agestr += 3;
	t2 = atoi( agestr );

	/* last field can only be seconds */
	if ( agestr[2] && ( agestr[2] != ':' || !gotdays ))
		return -1;
	t1 *= 60;
	t1 += t2;

	if ( agestr[2] ) {
		agestr += 3;
		if ( agestr[2] )
			return -1;
		t1 *= 60;
		t1 += atoi( agestr );
	} else if ( gotdays ) {
		/* only got days+hh:mm */
		t1 *= 60;
	}
	return t1;
}

static void
log_age_unparse( int age, struct berval *agebv, size_t size )
{
	int dd, hh, mm, ss, len;
	char *ptr;

	assert( size > 0 );

	ss = age % 60;
	age /= 60;
	mm = age % 60;
	age /= 60;
	hh = age % 24;
	age /= 24;
	dd = age;

	ptr = agebv->bv_val;

	if ( dd ) {
		len = snprintf( ptr, size, "%d+", dd );
		assert( len >= 0 && (unsigned) len < size );
		size -= len;
		ptr += len;
	}
	len = snprintf( ptr, size, "%02d:%02d", hh, mm );
	assert( len >= 0 && (unsigned) len < size );
	size -= len;
	ptr += len;
	if ( ss ) {
		len = snprintf( ptr, size, ":%02d", ss );
		assert( len >= 0 && (unsigned) len < size );
		size -= len;
		ptr += len;
	}

	agebv->bv_len = ptr - agebv->bv_val;
}

static slap_callback nullsc = { NULL, NULL, NULL, NULL };

#define PURGE_INCREMENT	100

typedef struct purge_data {
	int slots;
	int used;
	BerVarray dn;
	BerVarray ndn;
	struct berval csn;	/* an arbitrary old CSN */
} purge_data;

static int
log_old_lookup( Operation *op, SlapReply *rs )
{
	purge_data *pd = op->o_callback->sc_private;
	Attribute *a;

	if ( rs->sr_type != REP_SEARCH) return 0;

	if ( slapd_shutdown ) return 0;

	/* Remember max CSN: should always be the last entry
	 * seen, since log entries are ordered chronologically...
	 */
	a = attr_find( rs->sr_entry->e_attrs,
		slap_schema.si_ad_entryCSN );
	if ( a ) {
		ber_len_t len = a->a_nvals[0].bv_len;
		/* Paranoid len check, normalized CSNs are always the same length */
		if ( len > LDAP_PVT_CSNSTR_BUFSIZE )
			len = LDAP_PVT_CSNSTR_BUFSIZE;
		if ( memcmp( a->a_nvals[0].bv_val, pd->csn.bv_val, len ) > 0 ) {
			AC_MEMCPY( pd->csn.bv_val, a->a_nvals[0].bv_val, len );
			pd->csn.bv_len = len;
		}
	}
	if ( pd->used >= pd->slots ) {
		pd->slots += PURGE_INCREMENT;
		pd->dn = ch_realloc( pd->dn, pd->slots * sizeof( struct berval ));
		pd->ndn = ch_realloc( pd->ndn, pd->slots * sizeof( struct berval ));
	}
	ber_dupbv( &pd->dn[pd->used], &rs->sr_entry->e_name );
	ber_dupbv( &pd->ndn[pd->used], &rs->sr_entry->e_nname );
	pd->used++;
	return 0;
}

/* Periodically search for old entries in the log database and delete them */
static void *
accesslog_purge( void *ctx, void *arg )
{
	struct re_s *rtask = arg;
	struct log_info *li = rtask->arg;

	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;
	SlapReply rs = {REP_RESULT};
	slap_callback cb = { NULL, log_old_lookup, NULL, NULL };
	Filter f;
	AttributeAssertion ava = ATTRIBUTEASSERTION_INIT;
	purge_data pd = {0};
	char timebuf[LDAP_LUTIL_GENTIME_BUFSIZE];
	char csnbuf[LDAP_PVT_CSNSTR_BUFSIZE];
	time_t old = slap_get_time();

	connection_fake_init( &conn, &opbuf, ctx );
	op = &opbuf.ob_op;

	f.f_choice = LDAP_FILTER_LE;
	f.f_ava = &ava;
	f.f_next = NULL;

	ava.aa_desc = ad_reqStart;
	ava.aa_value.bv_val = timebuf;
	ava.aa_value.bv_len = sizeof(timebuf);

	old -= li->li_age;
	slap_timestamp( &old, &ava.aa_value );

	op->o_tag = LDAP_REQ_SEARCH;
	op->o_bd = li->li_db;
	op->o_dn = li->li_db->be_rootdn;
	op->o_ndn = li->li_db->be_rootndn;
	op->o_req_dn = li->li_db->be_suffix[0];
	op->o_req_ndn = li->li_db->be_nsuffix[0];
	op->o_callback = &cb;
	op->ors_scope = LDAP_SCOPE_ONELEVEL;
	op->ors_deref = LDAP_DEREF_NEVER;
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_slimit = SLAP_NO_LIMIT;
	op->ors_filter = &f;
	filter2bv_x( op, &f, &op->ors_filterstr );
	op->ors_attrs = slap_anlist_no_attrs;
	op->ors_attrsonly = 1;
	
	pd.csn.bv_len = sizeof( csnbuf );
	pd.csn.bv_val = csnbuf;
	csnbuf[0] = '\0';
	cb.sc_private = &pd;

	op->o_bd->be_search( op, &rs );
	op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );

	if ( pd.used ) {
		int i;

		/* delete the expired entries */
		op->o_tag = LDAP_REQ_DELETE;
		op->o_callback = &nullsc;
		op->o_csn = pd.csn;
		op->o_dont_replicate = 1;

		for (i=0; i<pd.used; i++) {
			op->o_req_dn = pd.dn[i];
			op->o_req_ndn = pd.ndn[i];
			if ( !slapd_shutdown ) {
				rs_reinit( &rs, REP_RESULT );
				op->o_bd->be_delete( op, &rs );
			}
			ch_free( pd.ndn[i].bv_val );
			ch_free( pd.dn[i].bv_val );
		}
		ch_free( pd.ndn );
		ch_free( pd.dn );

		{
			Modifications mod;
			struct berval bv[2];
			rs_reinit( &rs, REP_RESULT );
			/* update context's entryCSN to reflect oldest CSN */
			mod.sml_numvals = 1;
			mod.sml_values = bv;
			bv[0] = pd.csn;
			BER_BVZERO(&bv[1]);
			mod.sml_nvalues = NULL;
			mod.sml_desc = slap_schema.si_ad_entryCSN;
			mod.sml_op = LDAP_MOD_REPLACE;
			mod.sml_flags = SLAP_MOD_INTERNAL;
			mod.sml_next = NULL;

			op->o_tag = LDAP_REQ_MODIFY;
			op->orm_modlist = &mod;
			op->orm_no_opattrs = 1;
			op->o_req_dn = li->li_db->be_suffix[0];
			op->o_req_ndn = li->li_db->be_nsuffix[0];
			op->o_no_schema_check = 1;
			op->o_managedsait = SLAP_CONTROL_NONCRITICAL;
			op->o_bd->be_modify( op, &rs );
			if ( mod.sml_next ) {
				slap_mods_free( mod.sml_next, 1 );
			}
		}
	}

	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

	return NULL;
}

static int
log_cf_gen(ConfigArgs *c)
{
	slap_overinst *on = (slap_overinst *)c->bi;
	struct log_info *li = on->on_bi.bi_private;
	int rc = 0;
	slap_mask_t tmask = 0;
	char agebuf[2*STRLENOF("ddddd+hh:mm:ss  ")];
	struct berval agebv, cyclebv;

	switch( c->op ) {
	case SLAP_CONFIG_EMIT:
		switch( c->type ) {
		case LOG_DB:
			if ( !BER_BVISEMPTY( &li->li_db_suffix )) {
				value_add_one( &c->rvalue_vals, &li->li_db_suffix );
				value_add_one( &c->rvalue_nvals, &li->li_db_suffix );
			} else if ( li->li_db ) {
				value_add_one( &c->rvalue_vals, li->li_db->be_suffix );
				value_add_one( &c->rvalue_nvals, li->li_db->be_nsuffix );
			} else {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"accesslog: \"logdb <suffix>\" must be specified" );
				Debug( LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
					c->log, c->cr_msg, c->value_dn.bv_val );
				rc = 1;
				break;
			}
			break;
		case LOG_OPS:
			rc = mask_to_verbs( logops, li->li_ops, &c->rvalue_vals );
			break;
		case LOG_PURGE:
			if ( !li->li_age ) {
				rc = 1;
				break;
			}
			agebv.bv_val = agebuf;
			log_age_unparse( li->li_age, &agebv, sizeof( agebuf ) );
			agebv.bv_val[agebv.bv_len] = ' ';
			agebv.bv_len++;
			cyclebv.bv_val = agebv.bv_val + agebv.bv_len;
			log_age_unparse( li->li_cycle, &cyclebv, sizeof( agebuf ) - agebv.bv_len );
			agebv.bv_len += cyclebv.bv_len;
			value_add_one( &c->rvalue_vals, &agebv );
			break;
		case LOG_SUCCESS:
			if ( li->li_success )
				c->value_int = li->li_success;
			else
				rc = 1;
			break;
		case LOG_OLD:
			if ( li->li_oldf ) {
				filter2bv( li->li_oldf, &agebv );
				ber_bvarray_add( &c->rvalue_vals, &agebv );
			}
			else
				rc = 1;
			break;
		case LOG_OLDATTR:
			if ( li->li_oldattrs ) {
				log_attr *la;

				for ( la = li->li_oldattrs; la; la=la->next )
					value_add_one( &c->rvalue_vals, &la->attr->ad_cname );
			}
			else
				rc = 1;
			break;
		case LOG_BASE:
			if ( li->li_bases ) {
				log_base *lb;

				for ( lb = li->li_bases; lb; lb=lb->lb_next )
					value_add_one( &c->rvalue_vals, &lb->lb_line );
			}
			else
				rc = 1;
			break;
		}
		break;
	case LDAP_MOD_DELETE:
		switch( c->type ) {
		case LOG_DB:
			/* noop. this should always be a valid backend. */
			break;
		case LOG_OPS:
			if ( c->valx < 0 ) {
				li->li_ops = 0;
			} else {
				rc = verbs_to_mask( 1, &c->line, logops, &tmask );
				if ( rc == 0 )
					li->li_ops &= ~tmask;
			}
			break;
		case LOG_PURGE:
			if ( li->li_task ) {
				struct re_s *re = li->li_task;
				li->li_task = NULL;
				ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
				if ( ldap_pvt_runqueue_isrunning( &slapd_rq, re ))
					ldap_pvt_runqueue_stoptask( &slapd_rq, re );
				ldap_pvt_runqueue_remove( &slapd_rq, re );
				ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
			}
			li->li_age = 0;
			li->li_cycle = 0;
			break;
		case LOG_SUCCESS:
			li->li_success = 0;
			break;
		case LOG_OLD:
			if ( li->li_oldf ) {
				filter_free( li->li_oldf );
				li->li_oldf = NULL;
			}
			break;
		case LOG_OLDATTR:
			if ( c->valx < 0 ) {
				log_attr *la, *ln;

				for ( la = li->li_oldattrs; la; la = ln ) {
					ln = la->next;
					ch_free( la );
				}
			} else {
				log_attr *la = NULL, **lp;
				int i;

				for ( lp = &li->li_oldattrs, i=0; i < c->valx; i++ ) {
					la = *lp;
					lp = &la->next;
				}
				*lp = la->next;
				ch_free( la );
			}
			break;
		case LOG_BASE:
			if ( c->valx < 0 ) {
				log_base *lb, *ln;

				for ( lb = li->li_bases; lb; lb = ln ) {
					ln = lb->lb_next;
					ch_free( lb );
				}
			} else {
				log_base *lb = NULL, **lp;
				int i;

				for ( lp = &li->li_bases, i=0; i < c->valx; i++ ) {
					lb = *lp;
					lp = &lb->lb_next;
				}
				*lp = lb->lb_next;
				ch_free( lb );
			}
			break;
		}
		break;
	default:
		switch( c->type ) {
		case LOG_DB:
			if ( CONFIG_ONLINE_ADD( c )) {
				li->li_db = select_backend( &c->value_ndn, 0 );
				if ( !li->li_db ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"<%s> no matching backend found for suffix",
						c->argv[0] );
					Debug( LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
						c->log, c->cr_msg, c->value_dn.bv_val );
					rc = 1;
				}
				ch_free( c->value_ndn.bv_val );
			} else {
				li->li_db_suffix = c->value_ndn;
			}
			ch_free( c->value_dn.bv_val );
			break;
		case LOG_OPS:
			rc = verbs_to_mask( c->argc, c->argv, logops, &tmask );
			if ( rc == 0 )
				li->li_ops |= tmask;
			break;
		case LOG_PURGE:
			li->li_age = log_age_parse( c->argv[1] );
			if ( li->li_age < 1 ) {
				rc = 1;
			} else {
				li->li_cycle = log_age_parse( c->argv[2] );
				if ( li->li_cycle < 1 ) {
					rc = 1;
				} else if ( slapMode & SLAP_SERVER_MODE ) {
					struct re_s *re = li->li_task;
					if ( re )
						re->interval.tv_sec = li->li_cycle;
					else {
						ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
						li->li_task = ldap_pvt_runqueue_insert( &slapd_rq,
							li->li_cycle, accesslog_purge, li,
							"accesslog_purge", li->li_db ?
								li->li_db->be_suffix[0].bv_val :
								c->be->be_suffix[0].bv_val );
						ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
					}
				}
			}
			break;
		case LOG_SUCCESS:
			li->li_success = c->value_int;
			break;
		case LOG_OLD:
			li->li_oldf = str2filter( c->argv[1] );
			if ( !li->li_oldf ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "bad filter!" );
				rc = 1;
			}
			break;
		case LOG_OLDATTR: {
			int i;
			AttributeDescription *ad;
			const char *text;

			for ( i=1; i< c->argc; i++ ) {
				ad = NULL;
				if ( slap_str2ad( c->argv[i], &ad, &text ) == LDAP_SUCCESS ) {
					log_attr *la = ch_malloc( sizeof( log_attr ));
					la->attr = ad;
					la->next = li->li_oldattrs;
					li->li_oldattrs = la;
				} else {
					snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s <%s>: %s",
						c->argv[0], c->argv[i], text );
					Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
						"%s: %s\n", c->log, c->cr_msg, 0 );
					rc = ARG_BAD_CONF;
					break;
				}
			}
			}
			break;
		case LOG_BASE: {
			slap_mask_t m = 0;
			rc = verbstring_to_mask( logops, c->argv[1], '|', &m );
			if ( rc == 0 ) {
				struct berval dn, ndn;
				ber_str2bv( c->argv[2], 0, 0, &dn );
				rc = dnNormalize( 0, NULL, NULL, &dn, &ndn, NULL );
				if ( rc == 0 ) {
					log_base *lb;
					struct berval mbv;
					char *ptr;
					mask_to_verbstring( logops, m, '|', &mbv );
					lb = ch_malloc( sizeof( log_base ) + mbv.bv_len + ndn.bv_len + 3 + 1 );
					lb->lb_line.bv_val = (char *)(lb + 1);
					lb->lb_line.bv_len = mbv.bv_len + ndn.bv_len + 3;
					ptr = lutil_strcopy( lb->lb_line.bv_val, mbv.bv_val );
					*ptr++ = ' ';
					*ptr++ = '"';
					lb->lb_base.bv_val = ptr;
					lb->lb_base.bv_len = ndn.bv_len;
					ptr = lutil_strcopy( ptr, ndn.bv_val );
					*ptr++ = '"';
					lb->lb_ops = m;
					lb->lb_next = li->li_bases;
					li->li_bases = lb;
				} else {
					snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s invalid DN: %s",
						c->argv[0], c->argv[2] );
					Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
						"%s: %s\n", c->log, c->cr_msg, 0 );
					rc = ARG_BAD_CONF;
				}
			} else {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s invalid ops: %s",
					c->argv[0], c->argv[1] );
				Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
					"%s: %s\n", c->log, c->cr_msg, 0 );
				rc = ARG_BAD_CONF;
			}
			}
			break;
		}
		break;
	}
	return rc;
}

static int
logSchemaControlValidate(
	Syntax		*syntax,
	struct berval	*valp )
{
	struct berval	val, bv;
	ber_len_t		i;
	int		rc = LDAP_SUCCESS;

	assert( valp != NULL );

	val = *valp;

	/* check minimal size */
	if ( val.bv_len < STRLENOF( "{*}" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	val.bv_len--;

	/* check SEQUENCE boundaries */
	if ( val.bv_val[ 0 ] != '{' /*}*/ ||
		val.bv_val[ val.bv_len ] != /*{*/ '}' )
	{
		return LDAP_INVALID_SYNTAX;
	}

	/* extract and check OID */
	for ( i = 1; i < val.bv_len; i++ ) {
		if ( !ASCII_SPACE( val.bv_val[ i ] ) ) {
			break;
		}
	}

	bv.bv_val = &val.bv_val[ i ];

	for ( i++; i < val.bv_len; i++ ) {
		if ( ASCII_SPACE( val.bv_val[ i ] ) )
		{
			break;
		}
	}

	bv.bv_len = &val.bv_val[ i ] - bv.bv_val;

	rc = numericoidValidate( NULL, &bv );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	if ( i == val.bv_len ) {
		return LDAP_SUCCESS;
	}

	if ( val.bv_val[ i ] != ' ' ) {
		return LDAP_INVALID_SYNTAX;
	}

	for ( i++; i < val.bv_len; i++ ) {
		if ( !ASCII_SPACE( val.bv_val[ i ] ) ) {
			break;
		}
	}

	if ( i == val.bv_len ) {
		return LDAP_SUCCESS;
	}

	/* extract and check criticality */
	if ( strncasecmp( &val.bv_val[ i ], "criticality ", STRLENOF( "criticality " ) ) == 0 )
	{
		i += STRLENOF( "criticality " );
		for ( ; i < val.bv_len; i++ ) {
			if ( !ASCII_SPACE( val.bv_val[ i ] ) ) {
				break;
			}
		}

		if ( i == val.bv_len ) {
			return LDAP_INVALID_SYNTAX;
		}

		bv.bv_val = &val.bv_val[ i ];

		for ( ; i < val.bv_len; i++ ) {
			if ( ASCII_SPACE( val.bv_val[ i ] ) ) {
				break;
			}
		}

		bv.bv_len = &val.bv_val[ i ] - bv.bv_val;

		if ( !bvmatch( &bv, &slap_true_bv ) && !bvmatch( &bv, &slap_false_bv ) ) 
		{
			return LDAP_INVALID_SYNTAX;
		}

		if ( i == val.bv_len ) {
			return LDAP_SUCCESS;
		}

		if ( val.bv_val[ i ] != ' ' ) {
			return LDAP_INVALID_SYNTAX;
		}

		for ( i++; i < val.bv_len; i++ ) {
			if ( !ASCII_SPACE( val.bv_val[ i ] ) ) {
				break;
			}
		}

		if ( i == val.bv_len ) {
			return LDAP_SUCCESS;
		}
	}

	/* extract and check controlValue */
	if ( strncasecmp( &val.bv_val[ i ], "controlValue ", STRLENOF( "controlValue " ) ) == 0 )
	{
		ber_len_t valueStart, valueLen;

		i += STRLENOF( "controlValue " );
		for ( ; i < val.bv_len; i++ ) {
			if ( !ASCII_SPACE( val.bv_val[ i ] ) ) {
				break;
			}
		}

		if ( i == val.bv_len ) {
			return LDAP_INVALID_SYNTAX;
		}

		if ( val.bv_val[ i ] != '"' ) {
			return LDAP_INVALID_SYNTAX;
		}

		i++;
		valueStart = i;

		for ( ; i < val.bv_len; i++ ) {
			if ( val.bv_val[ i ] == '"' ) {
				break;
			}

			if ( !ASCII_HEX( val.bv_val[ i ] ) ) {
				return LDAP_INVALID_SYNTAX;
			}
		}

		if ( val.bv_val[ i ] != '"' ) {
			return LDAP_INVALID_SYNTAX;
		}

		valueLen = i - valueStart;
		if ( (valueLen/2)*2 != valueLen ) {
			return LDAP_INVALID_SYNTAX;
		}

		for ( i++; i < val.bv_len; i++ ) {
			if ( !ASCII_SPACE( val.bv_val[ i ] ) ) {
				break;
			}
		}

		if ( i == val.bv_len ) {
			return LDAP_SUCCESS;
		}
	}

	return LDAP_INVALID_SYNTAX;
}

static int
accesslog_ctrls(
	LDAPControl **ctrls,
	BerVarray *valsp,
	BerVarray *nvalsp,
	void *memctx )
{
	long		i, rc = 0;

	assert( valsp != NULL );
	assert( ctrls != NULL );

	*valsp = NULL;
	*nvalsp = NULL;

	for ( i = 0; ctrls[ i ] != NULL; i++ ) {
		struct berval	idx,
				oid,
				noid,
				bv;
		char		*ptr,
				buf[ 32 ];

		if ( ctrls[ i ]->ldctl_oid == NULL ) {
			return LDAP_PROTOCOL_ERROR;
		}

		idx.bv_len = snprintf( buf, sizeof( buf ), "{%ld}", i );
		idx.bv_val = buf;

		ber_str2bv( ctrls[ i ]->ldctl_oid, 0, 0, &oid );
		noid.bv_len = idx.bv_len + oid.bv_len;
		ptr = noid.bv_val = ber_memalloc_x( noid.bv_len + 1, memctx );
		ptr = lutil_strcopy( ptr, idx.bv_val );
		ptr = lutil_strcopy( ptr, oid.bv_val );

		bv.bv_len = idx.bv_len + STRLENOF( "{}" ) + oid.bv_len;

		if ( ctrls[ i ]->ldctl_iscritical ) {
			bv.bv_len += STRLENOF( " criticality TRUE" );
		}

		if ( !BER_BVISNULL( &ctrls[ i ]->ldctl_value ) ) {
			bv.bv_len += STRLENOF( " controlValue \"\"" )
				+ 2 * ctrls[ i ]->ldctl_value.bv_len;
		}

		ptr = bv.bv_val = ber_memalloc_x( bv.bv_len + 1, memctx );
		if ( ptr == NULL ) {
			ber_bvarray_free( *valsp );
			*valsp = NULL;
			ber_bvarray_free( *nvalsp );
			*nvalsp = NULL;
			return LDAP_OTHER;
		}

		ptr = lutil_strcopy( ptr, idx.bv_val );

		*ptr++ = '{' /*}*/ ;
		ptr = lutil_strcopy( ptr, oid.bv_val );

		if ( ctrls[ i ]->ldctl_iscritical ) {
			ptr = lutil_strcopy( ptr, " criticality TRUE" );
		}
		
		if ( !BER_BVISNULL( &ctrls[ i ]->ldctl_value ) ) {
			ber_len_t	j;

			ptr = lutil_strcopy( ptr, " controlValue \"" );
			for ( j = 0; j < ctrls[ i ]->ldctl_value.bv_len; j++ ) {
				*ptr++ = SLAP_ESCAPE_HI(ctrls[ i ]->ldctl_value.bv_val[ j ]);
				*ptr++ = SLAP_ESCAPE_LO(ctrls[ i ]->ldctl_value.bv_val[ j ]);
			}

			*ptr++ = '"';
		}

		*ptr++ = '}';
		*ptr = '\0';

		ber_bvarray_add_x( valsp, &bv, memctx );
		ber_bvarray_add_x( nvalsp, &noid, memctx );
	}

	return rc;
	
}

static Entry *accesslog_entry( Operation *op, SlapReply *rs, int logop,
	Operation *op2 ) {
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	log_info *li = on->on_bi.bi_private;

	char rdnbuf[STRLENOF(RDNEQ)+LDAP_LUTIL_GENTIME_BUFSIZE+8];
	char nrdnbuf[STRLENOF(RDNEQ)+LDAP_LUTIL_GENTIME_BUFSIZE+8];

	struct berval rdn, nrdn, timestamp, ntimestamp, bv;
	slap_verbmasks *lo = logops+logop+EN_OFFSET;

	Entry *e = entry_alloc();

	strcpy( rdnbuf, RDNEQ );
	rdn.bv_val = rdnbuf;
	strcpy( nrdnbuf, RDNEQ );
	nrdn.bv_val = nrdnbuf;

	timestamp.bv_val = rdnbuf+STRLENOF(RDNEQ);
	timestamp.bv_len = sizeof(rdnbuf) - STRLENOF(RDNEQ);
	slap_timestamp( &op->o_time, &timestamp );
	snprintf( timestamp.bv_val + timestamp.bv_len-1, sizeof(".123456Z"), ".%06dZ", op->o_tincr );
	timestamp.bv_len += STRLENOF(".123456");

	rdn.bv_len = STRLENOF(RDNEQ)+timestamp.bv_len;
	ad_reqStart->ad_type->sat_equality->smr_normalize(
		SLAP_MR_VALUE_OF_ASSERTION_SYNTAX, ad_reqStart->ad_type->sat_syntax,
		ad_reqStart->ad_type->sat_equality, &timestamp, &ntimestamp,
		op->o_tmpmemctx );

	strcpy( nrdn.bv_val + STRLENOF(RDNEQ), ntimestamp.bv_val );
	nrdn.bv_len = STRLENOF(RDNEQ)+ntimestamp.bv_len;
	build_new_dn( &e->e_name, li->li_db->be_suffix, &rdn, NULL );
	build_new_dn( &e->e_nname, li->li_db->be_nsuffix, &nrdn, NULL );

	attr_merge_one( e, slap_schema.si_ad_objectClass,
		&log_ocs[logop]->soc_cname, NULL );
	attr_merge_one( e, slap_schema.si_ad_structuralObjectClass,
		&log_ocs[logop]->soc_cname, NULL );
	attr_merge_one( e, ad_reqStart, &timestamp, &ntimestamp );
	op->o_tmpfree( ntimestamp.bv_val, op->o_tmpmemctx );

	slap_op_time( &op2->o_time, &op2->o_tincr );

	timestamp.bv_len = sizeof(rdnbuf) - STRLENOF(RDNEQ);
	slap_timestamp( &op2->o_time, &timestamp );
	snprintf( timestamp.bv_val + timestamp.bv_len-1, sizeof(".123456Z"), ".%06dZ", op2->o_tincr );
	timestamp.bv_len += STRLENOF(".123456");

	attr_merge_normalize_one( e, ad_reqEnd, &timestamp, op->o_tmpmemctx );

	/* Exops have OID appended */
	if ( logop == LOG_EN_EXTENDED ) {
		bv.bv_len = lo->word.bv_len + op->ore_reqoid.bv_len + 2;
		bv.bv_val = ch_malloc( bv.bv_len + 1 );
		AC_MEMCPY( bv.bv_val, lo->word.bv_val, lo->word.bv_len );
		bv.bv_val[lo->word.bv_len] = '{';
		AC_MEMCPY( bv.bv_val+lo->word.bv_len+1, op->ore_reqoid.bv_val,
			op->ore_reqoid.bv_len );
		bv.bv_val[bv.bv_len-1] = '}';
		bv.bv_val[bv.bv_len] = '\0';
		attr_merge_one( e, ad_reqType, &bv, NULL );
	} else {
		attr_merge_one( e, ad_reqType, &lo->word, NULL );
	}

	rdn.bv_len = snprintf( rdn.bv_val, sizeof( rdnbuf ), "%lu", op->o_connid );
	if ( rdn.bv_len < sizeof( rdnbuf ) ) {
		attr_merge_one( e, ad_reqSession, &rdn, NULL );
	} /* else? */

	if ( BER_BVISNULL( &op->o_dn ) ) {
		attr_merge_one( e, ad_reqAuthzID, (struct berval *)&slap_empty_bv,
			(struct berval *)&slap_empty_bv );
	} else {
		attr_merge_one( e, ad_reqAuthzID, &op->o_dn, &op->o_ndn );
	}

	/* FIXME: need to add reqControls and reqRespControls */
	if ( op->o_ctrls ) {
		BerVarray	vals = NULL,
				nvals = NULL;

		if ( accesslog_ctrls( op->o_ctrls, &vals, &nvals,
			op->o_tmpmemctx ) == LDAP_SUCCESS && vals )
		{
			attr_merge( e, ad_reqControls, vals, nvals );
			ber_bvarray_free_x( vals, op->o_tmpmemctx );
			ber_bvarray_free_x( nvals, op->o_tmpmemctx );
		}
	}

	if ( rs->sr_ctrls ) {
		BerVarray	vals = NULL,
				nvals = NULL;

		if ( accesslog_ctrls( rs->sr_ctrls, &vals, &nvals,
			op->o_tmpmemctx ) == LDAP_SUCCESS && vals )
		{
			attr_merge( e, ad_reqRespControls, vals, nvals );
			ber_bvarray_free_x( vals, op->o_tmpmemctx );
			ber_bvarray_free_x( nvals, op->o_tmpmemctx );
		}

	}

	return e;
}

static struct berval scopes[] = {
	BER_BVC("base"),
	BER_BVC("one"),
	BER_BVC("sub"),
	BER_BVC("subord")
};

static struct berval derefs[] = {
	BER_BVC("never"),
	BER_BVC("searching"),
	BER_BVC("finding"),
	BER_BVC("always")
};

static struct berval simple = BER_BVC("SIMPLE");

static void accesslog_val2val(AttributeDescription *ad, struct berval *val,
	char c_op, struct berval *dst) {
	char *ptr;

	dst->bv_len = ad->ad_cname.bv_len + val->bv_len + 2;
	if ( c_op ) dst->bv_len++;

	dst->bv_val = ch_malloc( dst->bv_len+1 );

	ptr = lutil_strcopy( dst->bv_val, ad->ad_cname.bv_val );
	*ptr++ = ':';
	if ( c_op )
		*ptr++ = c_op;
	*ptr++ = ' ';
	AC_MEMCPY( ptr, val->bv_val, val->bv_len );
	dst->bv_val[dst->bv_len] = '\0';
}

static int
accesslog_op2logop( Operation *op )
{
	switch ( op->o_tag ) {
	case LDAP_REQ_ADD:		return LOG_EN_ADD;
	case LDAP_REQ_DELETE:	return LOG_EN_DELETE;
	case LDAP_REQ_MODIFY:	return LOG_EN_MODIFY;
	case LDAP_REQ_MODRDN:	return LOG_EN_MODRDN;
	case LDAP_REQ_COMPARE:	return LOG_EN_COMPARE;
	case LDAP_REQ_SEARCH:	return LOG_EN_SEARCH;
	case LDAP_REQ_BIND:		return LOG_EN_BIND;
	case LDAP_REQ_EXTENDED:	return LOG_EN_EXTENDED;
	default:	/* unknown operation type */
		break;
	}	/* Unbind and Abandon never reach here */
	return LOG_EN_UNKNOWN;
}

static int accesslog_response(Operation *op, SlapReply *rs) {
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	log_info *li = on->on_bi.bi_private;
	Attribute *a, *last_attr;
	Modifications *m;
	struct berval *b, uuid = BER_BVNULL;
	int i;
	int logop, do_graduate = 0;
	slap_verbmasks *lo;
	Entry *e = NULL, *old = NULL, *e_uuid = NULL;
	char timebuf[LDAP_LUTIL_GENTIME_BUFSIZE+8];
	struct berval bv;
	char *ptr;
	BerVarray vals;
	Operation op2 = {0};
	SlapReply rs2 = {REP_RESULT};

	if ( rs->sr_type != REP_RESULT && rs->sr_type != REP_EXTENDED )
		return SLAP_CB_CONTINUE;

	logop = accesslog_op2logop( op );
	lo = logops+logop+EN_OFFSET;
	if ( !( li->li_ops & lo->mask )) {
		log_base *lb;

		i = 0;
		for ( lb = li->li_bases; lb; lb=lb->lb_next )
			if (( lb->lb_ops & lo->mask ) && dnIsSuffix( &op->o_req_ndn, &lb->lb_base )) {
				i = 1;
				break;
			}
		if ( !i )
			return SLAP_CB_CONTINUE;
	}

	/* mutex and so were only set for write operations;
	 * if we got here, the operation must be logged */
	if ( lo->mask & LOG_OP_WRITES ) {
		slap_callback *cb;

		/* These internal ops are not logged */
		if ( op->o_dont_replicate && op->orm_no_opattrs )
			return SLAP_CB_CONTINUE;

		ldap_pvt_thread_mutex_lock( &li->li_log_mutex );
		old = li->li_old;
		uuid = li->li_uuid;
		li->li_old = NULL;
		BER_BVZERO( &li->li_uuid );
		/* Disarm mod_cleanup */
		for ( cb = op->o_callback; cb; cb = cb->sc_next ) {
			if ( cb->sc_private == (void *)on ) {
				cb->sc_private = NULL;
				break;
			}
		}
		ldap_pvt_thread_rmutex_unlock( &li->li_op_rmutex, op->o_tid );
	}

	if ( li->li_success && rs->sr_err != LDAP_SUCCESS )
		goto done;

	e = accesslog_entry( op, rs, logop, &op2 );

	attr_merge_one( e, ad_reqDN, &op->o_req_dn, &op->o_req_ndn );

	if ( rs->sr_text ) {
		ber_str2bv( rs->sr_text, 0, 0, &bv );
		attr_merge_one( e, ad_reqMessage, &bv, NULL );
	}
	bv.bv_len = snprintf( timebuf, sizeof( timebuf ), "%d", rs->sr_err );
	if ( bv.bv_len < sizeof( timebuf ) ) {
		bv.bv_val = timebuf;
		attr_merge_one( e, ad_reqResult, &bv, NULL );
	}

	last_attr = attr_find( e->e_attrs, ad_reqResult );

	e_uuid = old;
	switch( logop ) {
	case LOG_EN_ADD:
	case LOG_EN_DELETE: {
		char c_op;
		Entry *e2;

		if ( logop == LOG_EN_ADD ) {
			e2 = op->ora_e;
			e_uuid = op->ora_e;
			c_op = '+';

		} else {
			if ( !old )
				break;
			e2 = old;
			c_op = 0;
		}
		/* count all the vals */
		i = 0;
		for ( a=e2->e_attrs; a; a=a->a_next ) {
			i += a->a_numvals;
		}
		vals = ch_malloc( (i+1) * sizeof( struct berval ));
		i = 0;
		for ( a=e2->e_attrs; a; a=a->a_next ) {
			if ( a->a_vals ) {
				for (b=a->a_vals; !BER_BVISNULL( b ); b++,i++) {
					accesslog_val2val( a->a_desc, b, c_op, &vals[i] );
				}
			}
		}
		vals[i].bv_val = NULL;
		vals[i].bv_len = 0;
		a = attr_alloc( logop == LOG_EN_ADD ? ad_reqMod : ad_reqOld );
		a->a_numvals = i;
		a->a_vals = vals;
		a->a_nvals = vals;
		last_attr->a_next = a;
		break;
	}

	case LOG_EN_MODRDN:
	case LOG_EN_MODIFY:
		/* count all the mods */
		i = 0;
		for ( m = op->orm_modlist; m; m = m->sml_next ) {
			if ( m->sml_values ) {
				i += m->sml_numvals;
			} else if ( m->sml_op == LDAP_MOD_DELETE ||
				m->sml_op == LDAP_MOD_REPLACE )
			{
				i++;
			}
		}
		vals = ch_malloc( (i+1) * sizeof( struct berval ));
		i = 0;

		/* init flags on old entry */
		if ( old ) {
			for ( a = old->e_attrs; a; a = a->a_next ) {
				log_attr *la;
				a->a_flags = 0;

				/* look for attrs that are always logged */
				for ( la = li->li_oldattrs; la; la = la->next ) {
					if ( a->a_desc == la->attr ) {
						a->a_flags = 1;
					}
				}
			}
		}

		for ( m = op->orm_modlist; m; m = m->sml_next ) {
			/* Mark this attribute as modified */
			if ( old ) {
				a = attr_find( old->e_attrs, m->sml_desc );
				if ( a ) {
					a->a_flags = 1;
				}
			}

			/* don't log the RDN mods; they're explicitly logged later */
			if ( logop == LOG_EN_MODRDN &&
			 	( m->sml_op == SLAP_MOD_SOFTADD ||
				  m->sml_op == LDAP_MOD_DELETE ) )
			{
				continue;
			}

			if ( m->sml_values ) {
				for ( b = m->sml_values; !BER_BVISNULL( b ); b++, i++ ) {
					char c_op;

					switch ( m->sml_op ) {
					case LDAP_MOD_ADD: c_op = '+'; break;
					case LDAP_MOD_DELETE:	c_op = '-'; break;
					case LDAP_MOD_REPLACE:	c_op = '='; break;
					case LDAP_MOD_INCREMENT:	c_op = '#'; break;

					/* unknown op. there shouldn't be any of these. we
					 * don't know what to do with it, but we shouldn't just
					 * ignore it.
					 */
					default: c_op = '?'; break;
					}
					accesslog_val2val( m->sml_desc, b, c_op, &vals[i] );
				}
			} else if ( m->sml_op == LDAP_MOD_DELETE ||
				m->sml_op == LDAP_MOD_REPLACE )
			{
				vals[i].bv_len = m->sml_desc->ad_cname.bv_len + 2;
				vals[i].bv_val = ch_malloc( vals[i].bv_len + 1 );
				ptr = lutil_strcopy( vals[i].bv_val,
					m->sml_desc->ad_cname.bv_val );
				*ptr++ = ':';
				if ( m->sml_op == LDAP_MOD_DELETE ) {
					*ptr++ = '-';
				} else {
					*ptr++ = '=';
				}
				*ptr = '\0';
				i++;
			}
		}

		if ( i > 0 ) {
			BER_BVZERO( &vals[i] );
			a = attr_alloc( ad_reqMod );
			a->a_numvals = i;
			a->a_vals = vals;
			a->a_nvals = vals;
			last_attr->a_next = a;
			last_attr = a;

		} else {
			ch_free( vals );
		}

		if ( old ) {
			/* count all the vals */
			i = 0;
			for ( a = old->e_attrs; a != NULL; a = a->a_next ) {
				if ( a->a_vals && a->a_flags ) {
					i += a->a_numvals;
				}
			}
			if ( i ) {
				vals = ch_malloc( (i + 1) * sizeof( struct berval ) );
				i = 0;
				for ( a=old->e_attrs; a; a=a->a_next ) {
					if ( a->a_vals && a->a_flags ) {
						for (b=a->a_vals; !BER_BVISNULL( b ); b++,i++) {
							accesslog_val2val( a->a_desc, b, 0, &vals[i] );
						}
					}
				}
				vals[i].bv_val = NULL;
				vals[i].bv_len = 0;
				a = attr_alloc( ad_reqOld );
				a->a_numvals = i;
				a->a_vals = vals;
				a->a_nvals = vals;
				last_attr->a_next = a;
			}
		}
		if ( logop == LOG_EN_MODIFY ) {
			break;
		}

		/* Now log the actual modRDN info */
		attr_merge_one( e, ad_reqNewRDN, &op->orr_newrdn, &op->orr_nnewrdn );
		attr_merge_one( e, ad_reqDeleteOldRDN, op->orr_deleteoldrdn ?
			(struct berval *)&slap_true_bv : (struct berval *)&slap_false_bv,
			NULL );
		if ( op->orr_newSup ) {
			attr_merge_one( e, ad_reqNewSuperior, op->orr_newSup, op->orr_nnewSup );
		}
		break;

	case LOG_EN_COMPARE:
		bv.bv_len = op->orc_ava->aa_desc->ad_cname.bv_len + 1 +
			op->orc_ava->aa_value.bv_len;
		bv.bv_val = op->o_tmpalloc( bv.bv_len+1, op->o_tmpmemctx );
		ptr = lutil_strcopy( bv.bv_val, op->orc_ava->aa_desc->ad_cname.bv_val );
		*ptr++ = '=';
		AC_MEMCPY( ptr, op->orc_ava->aa_value.bv_val, op->orc_ava->aa_value.bv_len );
		bv.bv_val[bv.bv_len] = '\0';
		attr_merge_one( e, ad_reqAssertion, &bv, NULL );
		op->o_tmpfree( bv.bv_val, op->o_tmpmemctx );
		break;

	case LOG_EN_SEARCH:
		attr_merge_one( e, ad_reqScope, &scopes[op->ors_scope], NULL );
		attr_merge_one( e, ad_reqDerefAliases, &derefs[op->ors_deref], NULL );
		attr_merge_one( e, ad_reqAttrsOnly, op->ors_attrsonly ?
			(struct berval *)&slap_true_bv : (struct berval *)&slap_false_bv,
			NULL );
		if ( !BER_BVISEMPTY( &op->ors_filterstr ))
			attr_merge_one( e, ad_reqFilter, &op->ors_filterstr, NULL );
		if ( op->ors_attrs ) {
			int j;
			/* count them */
			for (i=0; !BER_BVISNULL(&op->ors_attrs[i].an_name );i++)
				;
			vals = op->o_tmpalloc( (i+1) * sizeof(struct berval),
				op->o_tmpmemctx );
			for (i=0, j=0; !BER_BVISNULL(&op->ors_attrs[i].an_name );i++) {
				if (!BER_BVISEMPTY(&op->ors_attrs[i].an_name)) {
					vals[j] = op->ors_attrs[i].an_name;
					j++;
				}
			}
			BER_BVZERO(&vals[j]);
			attr_merge( e, ad_reqAttr, vals, NULL );
			op->o_tmpfree( vals, op->o_tmpmemctx );
		}
		bv.bv_val = timebuf;
		bv.bv_len = snprintf( bv.bv_val, sizeof( timebuf ), "%d", rs->sr_nentries );
		if ( bv.bv_len < sizeof( timebuf ) ) {
			attr_merge_one( e, ad_reqEntries, &bv, NULL );
		} /* else? */

		bv.bv_len = snprintf( bv.bv_val, sizeof( timebuf ), "%d", op->ors_tlimit );
		if ( bv.bv_len < sizeof( timebuf ) ) {
			attr_merge_one( e, ad_reqTimeLimit, &bv, NULL );
		} /* else? */

		bv.bv_len = snprintf( bv.bv_val, sizeof( timebuf ), "%d", op->ors_slimit );
		if ( bv.bv_len < sizeof( timebuf ) ) {
			attr_merge_one( e, ad_reqSizeLimit, &bv, NULL );
		} /* else? */
		break;

	case LOG_EN_BIND:
		bv.bv_val = timebuf;
		bv.bv_len = snprintf( bv.bv_val, sizeof( timebuf ), "%d", op->o_protocol );
		if ( bv.bv_len < sizeof( timebuf ) ) {
			attr_merge_one( e, ad_reqVersion, &bv, NULL );
		} /* else? */
		if ( op->orb_method == LDAP_AUTH_SIMPLE ) {
			attr_merge_one( e, ad_reqMethod, &simple, NULL );
		} else {
			bv.bv_len = STRLENOF("SASL()") + op->orb_mech.bv_len;
			bv.bv_val = op->o_tmpalloc( bv.bv_len + 1, op->o_tmpmemctx );
			ptr = lutil_strcopy( bv.bv_val, "SASL(" );
			ptr = lutil_strcopy( ptr, op->orb_mech.bv_val );
			*ptr++ = ')';
			*ptr = '\0';
			attr_merge_one( e, ad_reqMethod, &bv, NULL );
			op->o_tmpfree( bv.bv_val, op->o_tmpmemctx );
		}

		break;

	case LOG_EN_EXTENDED:
		if ( op->ore_reqdata ) {
			attr_merge_one( e, ad_reqData, op->ore_reqdata, NULL );
		}
		break;

	case LOG_EN_UNKNOWN:
		/* we don't know its parameters, don't add any */
		break;
	}

	if ( e_uuid || !BER_BVISNULL( &uuid ) ) {
		struct berval *pbv = NULL;

		if ( !BER_BVISNULL( &uuid ) ) {
			pbv = &uuid;

		} else {
			a = attr_find( e_uuid->e_attrs, slap_schema.si_ad_entryUUID );
			if ( a ) {
				pbv = &a->a_vals[0];
			}
		} 

		if ( pbv ) {
			attr_merge_normalize_one( e, ad_reqEntryUUID, pbv, op->o_tmpmemctx );
		}

		if ( !BER_BVISNULL( &uuid ) ) {
			ber_memfree( uuid.bv_val );
			BER_BVZERO( &uuid );
		}
	}

	op2.o_hdr = op->o_hdr;
	op2.o_tag = LDAP_REQ_ADD;
	op2.o_bd = li->li_db;
	op2.o_dn = li->li_db->be_rootdn;
	op2.o_ndn = li->li_db->be_rootndn;
	op2.o_req_dn = e->e_name;
	op2.o_req_ndn = e->e_nname;
	op2.ora_e = e;
	op2.o_callback = &nullsc;

	if (( lo->mask & LOG_OP_WRITES ) && !BER_BVISEMPTY( &op->o_csn )) {
		slap_queue_csn( &op2, &op->o_csn );
		do_graduate = 1;
	}

	op2.o_bd->be_add( &op2, &rs2 );
	if ( e == op2.ora_e ) entry_free( e );
	e = NULL;
	if ( do_graduate ) {
		slap_graduate_commit_csn( &op2 );
		if ( op2.o_csn.bv_val )
			op->o_tmpfree( op2.o_csn.bv_val, op->o_tmpmemctx );
	}

done:
	if ( lo->mask & LOG_OP_WRITES )
		ldap_pvt_thread_mutex_unlock( &li->li_log_mutex );
	if ( old ) entry_free( old );
	return SLAP_CB_CONTINUE;
}

/* Since Bind success is sent by the frontend, it won't normally enter
 * the overlay response callback. Add another callback to make sure it
 * gets here.
 */
static int
accesslog_bind_resp( Operation *op, SlapReply *rs )
{
	BackendDB *be, db;
	int rc;
	slap_callback *sc;

	be = op->o_bd;
	db = *be;
	op->o_bd = &db;
	db.bd_info = op->o_callback->sc_private;
	rc = accesslog_response( op, rs );
	op->o_bd = be;
	sc = op->o_callback;
	op->o_callback = sc->sc_next;
	op->o_tmpfree( sc, op->o_tmpmemctx );
	return rc;
}

static int
accesslog_op_bind( Operation *op, SlapReply *rs )
{
	slap_callback *sc;

	sc = op->o_tmpcalloc( 1, sizeof(slap_callback), op->o_tmpmemctx );
	sc->sc_response = accesslog_bind_resp;
	sc->sc_private = op->o_bd->bd_info;

	if ( op->o_callback ) {
		sc->sc_next = op->o_callback->sc_next;
		op->o_callback->sc_next = sc;
	} else {
		op->o_callback = sc;
	}
	return SLAP_CB_CONTINUE;
}

static int
accesslog_mod_cleanup( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;
	slap_overinst *on = sc->sc_private;
	op->o_callback = sc->sc_next;

	op->o_tmpfree( sc, op->o_tmpmemctx );

	if ( on ) {
		BackendInfo *bi = op->o_bd->bd_info;
		op->o_bd->bd_info = (BackendInfo *)on;
		accesslog_response( op, rs );
		op->o_bd->bd_info = bi;
	}
	return 0;
}

static int
accesslog_op_mod( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	log_info *li = on->on_bi.bi_private;
	slap_verbmasks *lo;
	int logop;
	int doit = 0;

	/* These internal ops are not logged */
	if ( op->o_dont_replicate && op->orm_no_opattrs )
		return SLAP_CB_CONTINUE;

	logop = accesslog_op2logop( op );
	lo = logops+logop+EN_OFFSET;

	if ( li->li_ops & lo->mask ) {
		doit = 1;
	} else {
		log_base *lb;
		for ( lb = li->li_bases; lb; lb = lb->lb_next )
			if (( lb->lb_ops & lo->mask ) && dnIsSuffix( &op->o_req_ndn, &lb->lb_base )) {
				doit = 1;
				break;
			}
	}
			
	if ( doit ) {
		slap_callback *cb = op->o_tmpalloc( sizeof( slap_callback ), op->o_tmpmemctx ), *cb2;
		cb->sc_cleanup = accesslog_mod_cleanup;
		cb->sc_response = NULL;
		cb->sc_private = on;
		cb->sc_next = NULL;
		for ( cb2 = op->o_callback; cb2->sc_next; cb2 = cb2->sc_next );
		cb2->sc_next = cb;

		ldap_pvt_thread_rmutex_lock( &li->li_op_rmutex, op->o_tid );
		if ( li->li_oldf && ( op->o_tag == LDAP_REQ_DELETE ||
			op->o_tag == LDAP_REQ_MODIFY ||
			( op->o_tag == LDAP_REQ_MODRDN && li->li_oldattrs )))
		{
			int rc;
			Entry *e;

			op->o_bd->bd_info = (BackendInfo *)on->on_info;
			rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &e );
			if ( e ) {
				if ( test_filter( op, e, li->li_oldf ) == LDAP_COMPARE_TRUE )
					li->li_old = entry_dup( e );
				be_entry_release_rw( op, e, 0 );
			}
			op->o_bd->bd_info = (BackendInfo *)on;

		} else {
			int rc;
			Entry *e;

			op->o_bd->bd_info = (BackendInfo *)on->on_info;
			rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &e );
			if ( e ) {
				Attribute *a = attr_find( e->e_attrs, slap_schema.si_ad_entryUUID );
				if ( a ) {
					ber_dupbv( &li->li_uuid, &a->a_vals[0] );
				}
				be_entry_release_rw( op, e, 0 );
			}
			op->o_bd->bd_info = (BackendInfo *)on;
		}
	}
	return SLAP_CB_CONTINUE;
}

/* unbinds are broadcast to all backends; we only log it if this
 * backend was used for the original bind.
 */
static int
accesslog_unbind( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	if ( op->o_conn->c_authz_backend == on->on_info->oi_origdb ) {
		log_info *li = on->on_bi.bi_private;
		Operation op2 = {0};
		void *cids[SLAP_MAX_CIDS];
		SlapReply rs2 = {REP_RESULT};
		Entry *e;

		if ( !( li->li_ops & LOG_OP_UNBIND )) {
			log_base *lb;
			int i = 0;

			for ( lb = li->li_bases; lb; lb=lb->lb_next )
				if (( lb->lb_ops & LOG_OP_UNBIND ) && dnIsSuffix( &op->o_ndn, &lb->lb_base )) {
					i = 1;
					break;
				}
			if ( !i )
				return SLAP_CB_CONTINUE;
		}

		e = accesslog_entry( op, rs, LOG_EN_UNBIND, &op2 );
		op2.o_hdr = op->o_hdr;
		op2.o_tag = LDAP_REQ_ADD;
		op2.o_bd = li->li_db;
		op2.o_dn = li->li_db->be_rootdn;
		op2.o_ndn = li->li_db->be_rootndn;
		op2.o_req_dn = e->e_name;
		op2.o_req_ndn = e->e_nname;
		op2.ora_e = e;
		op2.o_callback = &nullsc;
		op2.o_controls = cids;
		memset(cids, 0, sizeof( cids ));

		op2.o_bd->be_add( &op2, &rs2 );
		if ( e == op2.ora_e )
			entry_free( e );
	}
	return SLAP_CB_CONTINUE;
}

static int
accesslog_abandon( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	log_info *li = on->on_bi.bi_private;
	Operation op2 = {0};
	void *cids[SLAP_MAX_CIDS];
	SlapReply rs2 = {REP_RESULT};
	Entry *e;
	char buf[64];
	struct berval bv;

	if ( !op->o_time )
		return SLAP_CB_CONTINUE;

	if ( !( li->li_ops & LOG_OP_ABANDON )) {
		log_base *lb;
		int i = 0;

		for ( lb = li->li_bases; lb; lb=lb->lb_next )
			if (( lb->lb_ops & LOG_OP_ABANDON ) && dnIsSuffix( &op->o_ndn, &lb->lb_base )) {
				i = 1;
				break;
			}
		if ( !i )
			return SLAP_CB_CONTINUE;
	}

	e = accesslog_entry( op, rs, LOG_EN_ABANDON, &op2 );
	bv.bv_val = buf;
	bv.bv_len = snprintf( buf, sizeof( buf ), "%d", op->orn_msgid );
	if ( bv.bv_len < sizeof( buf ) ) {
		attr_merge_one( e, ad_reqId, &bv, NULL );
	} /* else? */

	op2.o_hdr = op->o_hdr;
	op2.o_tag = LDAP_REQ_ADD;
	op2.o_bd = li->li_db;
	op2.o_dn = li->li_db->be_rootdn;
	op2.o_ndn = li->li_db->be_rootndn;
	op2.o_req_dn = e->e_name;
	op2.o_req_ndn = e->e_nname;
	op2.ora_e = e;
	op2.o_callback = &nullsc;
	op2.o_controls = cids;
	memset(cids, 0, sizeof( cids ));

	op2.o_bd->be_add( &op2, &rs2 );
	if ( e == op2.ora_e )
		entry_free( e );

	return SLAP_CB_CONTINUE;
}

static int
accesslog_operational( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	log_info *li = on->on_bi.bi_private;

	if ( op->o_sync != SLAP_CONTROL_NONE )
		return SLAP_CB_CONTINUE;

	if ( rs->sr_entry != NULL
		&& dn_match( &op->o_bd->be_nsuffix[0], &rs->sr_entry->e_nname ) )
	{
		Attribute	**ap;

		for ( ap = &rs->sr_operational_attrs; *ap; ap = &(*ap)->a_next )
			/* just count */ ;

		if ( SLAP_OPATTRS( rs->sr_attr_flags ) ||
				ad_inlist( ad_auditContext, rs->sr_attrs ) )
		{
			*ap = attr_alloc( ad_auditContext );
			attr_valadd( *ap,
				&li->li_db->be_suffix[0],
				&li->li_db->be_nsuffix[0], 1 );
		}
	}

	return SLAP_CB_CONTINUE;
}

static slap_overinst accesslog;

static int
accesslog_db_init(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	log_info *li = ch_calloc(1, sizeof(log_info));

	on->on_bi.bi_private = li;
	ldap_pvt_thread_rmutex_init( &li->li_op_rmutex );
	ldap_pvt_thread_mutex_init( &li->li_log_mutex );
	return 0;
}

static int
accesslog_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	log_info *li = on->on_bi.bi_private;
	log_attr *la;

	if ( li->li_oldf )
		filter_free( li->li_oldf );
	for ( la=li->li_oldattrs; la; la=li->li_oldattrs ) {
		li->li_oldattrs = la->next;
		ch_free( la );
	}
	ldap_pvt_thread_mutex_destroy( &li->li_log_mutex );
	ldap_pvt_thread_rmutex_destroy( &li->li_op_rmutex );
	free( li );
	return LDAP_SUCCESS;
}

/* Create the logdb's root entry if it's missing */
static void *
accesslog_db_root(
	void *ctx,
	void *arg )
{
	struct re_s *rtask = arg;
	slap_overinst *on = rtask->arg;
	log_info *li = on->on_bi.bi_private;

	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;

	Entry *e;
	int rc;

	connection_fake_init( &conn, &opbuf, ctx );
	op = &opbuf.ob_op;
	op->o_bd = li->li_db;
	op->o_dn = li->li_db->be_rootdn;
	op->o_ndn = li->li_db->be_rootndn;
	rc = be_entry_get_rw( op, li->li_db->be_nsuffix, NULL, NULL, 0, &e );

	if ( e ) {
		be_entry_release_rw( op, e, 0 );

	} else {
		SlapReply rs = {REP_RESULT};
		struct berval rdn, nrdn, attr;
		char *ptr;
		AttributeDescription *ad = NULL;
		const char *text = NULL;
		Entry *e_ctx;
		BackendDB db;

		e = entry_alloc();
		ber_dupbv( &e->e_name, li->li_db->be_suffix );
		ber_dupbv( &e->e_nname, li->li_db->be_nsuffix );

		attr_merge_one( e, slap_schema.si_ad_objectClass,
			&log_container->soc_cname, NULL );

		dnRdn( &e->e_name, &rdn );
		dnRdn( &e->e_nname, &nrdn );
		ptr = ber_bvchr( &rdn, '=' );

		assert( ptr != NULL );

		attr.bv_val = rdn.bv_val;
		attr.bv_len = ptr - rdn.bv_val;

		slap_bv2ad( &attr, &ad, &text );

		rdn.bv_val = ptr+1;
		rdn.bv_len -= attr.bv_len + 1;
		ptr = ber_bvchr( &nrdn, '=' );
		nrdn.bv_len -= ptr - nrdn.bv_val + 1;
		nrdn.bv_val = ptr+1;
		attr_merge_one( e, ad, &rdn, &nrdn );

		/* Get contextCSN from main DB */
		op->o_bd = on->on_info->oi_origdb;
		rc = be_entry_get_rw( op, op->o_bd->be_nsuffix, NULL,
			slap_schema.si_ad_contextCSN, 0, &e_ctx );

		if ( e_ctx ) {
			Attribute *a;

			a = attr_find( e_ctx->e_attrs, slap_schema.si_ad_contextCSN );
			if ( a ) {
				/* FIXME: contextCSN could have multiple values!
				 * should select the one with the server's SID */
				attr_merge_one( e, slap_schema.si_ad_entryCSN,
					&a->a_vals[0], &a->a_nvals[0] );
				attr_merge( e, a->a_desc, a->a_vals, a->a_nvals );
			}
			be_entry_release_rw( op, e_ctx, 0 );
		}
		db = *li->li_db;
		op->o_bd = &db;

		op->ora_e = e;
		op->o_req_dn = e->e_name;
		op->o_req_ndn = e->e_nname;
		op->o_callback = &nullsc;
		SLAP_DBFLAGS( op->o_bd ) |= SLAP_DBFLAG_NOLASTMOD;
		rc = op->o_bd->be_add( op, &rs );
		if ( e == op->ora_e )
			entry_free( e );
	}
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	ldap_pvt_runqueue_remove( &slapd_rq, rtask );
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

	return NULL;
}

static int
accesslog_db_open(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	log_info *li = on->on_bi.bi_private;


	if ( !BER_BVISEMPTY( &li->li_db_suffix )) {
		li->li_db = select_backend( &li->li_db_suffix, 0 );
		ch_free( li->li_db_suffix.bv_val );
		BER_BVZERO( &li->li_db_suffix );
	}
	if ( li->li_db == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"accesslog: \"logdb <suffix>\" missing or invalid.\n",
			0, 0, 0 );
		return 1;
	}

	if ( slapMode & SLAP_TOOL_MODE )
		return 0;

	if ( BER_BVISEMPTY( &li->li_db->be_rootndn )) {
		ber_dupbv( &li->li_db->be_rootdn, li->li_db->be_suffix );
		ber_dupbv( &li->li_db->be_rootndn, li->li_db->be_nsuffix );
	}

	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	ldap_pvt_runqueue_insert( &slapd_rq, 3600, accesslog_db_root, on,
		"accesslog_db_root", li->li_db->be_suffix[0].bv_val );
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

	return 0;
}

int accesslog_initialize()
{
	int i, rc;

	accesslog.on_bi.bi_type = "accesslog";
	accesslog.on_bi.bi_db_init = accesslog_db_init;
	accesslog.on_bi.bi_db_destroy = accesslog_db_destroy;
	accesslog.on_bi.bi_db_open = accesslog_db_open;

	accesslog.on_bi.bi_op_add = accesslog_op_mod;
	accesslog.on_bi.bi_op_bind = accesslog_op_bind;
	accesslog.on_bi.bi_op_delete = accesslog_op_mod;
	accesslog.on_bi.bi_op_modify = accesslog_op_mod;
	accesslog.on_bi.bi_op_modrdn = accesslog_op_mod;
	accesslog.on_bi.bi_op_unbind = accesslog_unbind;
	accesslog.on_bi.bi_op_abandon = accesslog_abandon;
	accesslog.on_bi.bi_operational = accesslog_operational;
	accesslog.on_response = accesslog_response;

	accesslog.on_bi.bi_cf_ocs = log_cfocs;

	nullsc.sc_response = slap_null_cb;

	rc = config_register_schema( log_cfats, log_cfocs );
	if ( rc ) return rc;

	/* log schema integration */
	for ( i=0; lsyntaxes[i].oid; i++ ) {
		int code;

		code = register_syntax( &lsyntaxes[ i ].syn );
		if ( code != 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"accesslog_init: register_syntax failed\n",
				0, 0, 0 );
			return code;
		}

		if ( lsyntaxes[i].mrs != NULL ) {
			code = mr_make_syntax_compat_with_mrs(
				lsyntaxes[i].oid, lsyntaxes[i].mrs );
			if ( code < 0 ) {
				Debug( LDAP_DEBUG_ANY,
					"accesslog_init: "
					"mr_make_syntax_compat_with_mrs "
					"failed\n",
					0, 0, 0 );
				return code;
			}
		}
	}

	for ( i=0; lattrs[i].at; i++ ) {
		int code;

		code = register_at( lattrs[i].at, lattrs[i].ad, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"accesslog_init: register_at failed\n",
				0, 0, 0 );
			return -1;
		}
	}

	for ( i=0; locs[i].ot; i++ ) {
		int code;

		code = register_oc( locs[i].ot, locs[i].oc, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"accesslog_init: register_oc failed\n",
				0, 0, 0 );
			return -1;
		}
	}

	return overlay_register(&accesslog);
}

#if SLAPD_OVER_ACCESSLOG == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return accesslog_initialize();
}
#endif

#endif /* SLAPD_OVER_ACCESSLOG */
