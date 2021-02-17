/* schema_init.c - init builtin schema */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
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

/*
 * Syntaxes - implementation notes:
 *
 * Validate function(syntax, value):
 *   Called before the other functions here to check if the value
 *   is valid according to the syntax.
 *
 * Pretty function(syntax, input value, output prettified...):
 *   If it exists, maps different notations of the same value to a
 *   unique representation which can be stored in the directory and
 *   possibly be passed to the Match/Indexer/Filter() functions.
 *
 *   E.g. DN "2.5.4.3 = foo\,bar, o = BAZ" -> "cn=foo\2Cbar,o=BAZ",
 *   but unlike DN normalization, "BAZ" is not mapped to "baz".
 */

/*
 * Matching rules - implementation notes:
 *
 * Matching rules match an attribute value (often from the directory)
 * against an asserted value (e.g. from a filter).
 *
 * Invoked with validated and commonly pretty/normalized arguments, thus
 * a number of matching rules can simply use the octetString functions.
 *
 * Normalize function(...input value, output normalized...):
 *   If it exists, maps matching values to a unique representation
 *   which is passed to the Match/Indexer/Filter() functions.
 *
 *   Different matching rules can normalize values of the same syntax
 *   differently.  E.g. caseIgnore rules normalize to lowercase,
 *   caseExact rules do not.
 *
 * Match function(*output matchp, ...value, asserted value):
 *   On success, set *matchp.  0 means match.  For ORDERING/most EQUALITY,
 *   less/greater than 0 means value less/greater than asserted.  However:
 *
 *   In extensible match filters, ORDERING rules match if value<asserted.
 *
 *   EQUALITY rules may order values differently than ORDERING rules for
 *   speed, since EQUALITY ordering is only used for SLAP_AT_SORTED_VAL.
 *   Some EQUALITY rules do not order values (ITS#6722).
 *
 * Indexer function(...attribute values, *output keysp,...):
 *   Generates index keys for the attribute values.  Backends can store
 *   them in an index, a {key->entry ID set} mapping, for the attribute.
 *
 *   A search can look up the DN/scope and asserted values in the
 *   indexes, if any, to narrow down the number of entires to check
 *   against the search criteria.
 *
 * Filter function(...asserted value, *output keysp,...):
 *   Generates index key(s) for the asserted value, to be looked up in
 *   the index from the Indexer function.  *keysp is an array because
 *   substring matching rules can generate multiple lookup keys.
 *
 * Index keys:
 *   A key is usually a hash of match type, attribute value and schema
 *   info, because one index can contain keys for many filtering types.
 *
 *   Some indexes instead have EQUALITY keys ordered so that if
 *   key(val1) < key(val2), then val1 < val2 by the ORDERING rule.
 *   That way the ORDERING rule can use the EQUALITY index.
 *
 * Substring indexing:
 *   This chops the attribute values up in small chunks and indexes all
 *   possible chunks of certain sizes.  Substring filtering looks up
 *   SOME of the asserted value's chunks, and the caller uses the
 *   intersection of the resulting entry ID sets.
 *   See the index_substr_* keywords in slapd.conf(5).
 */

#include "portable.h"

#include <stdio.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <ac/ctype.h>
#include <ac/errno.h>
#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "../../libraries/liblber/lber-int.h" /* get ber_ptrlen() */

#include "ldap_utf8.h"

#include "lutil.h"
#include "lutil_hash.h"

#ifdef LUTIL_HASH64_BYTES
#define HASH_BYTES				LUTIL_HASH64_BYTES
#define HASH_LEN	hashlen
static void (*hashinit)(lutil_HASH_CTX *ctx) = lutil_HASHInit;
static void (*hashupdate)(lutil_HASH_CTX *ctx,unsigned char const *buf, ber_len_t len) = lutil_HASHUpdate;
static void (*hashfinal)(unsigned char digest[HASH_BYTES], lutil_HASH_CTX *ctx) = lutil_HASHFinal;
static int hashlen = LUTIL_HASH_BYTES;
#define HASH_Init(c)			hashinit(c)
#define HASH_Update(c,buf,len)	hashupdate(c,buf,len)
#define HASH_Final(d,c)			hashfinal(d,c)

/* Toggle between 32 and 64 bit hashing, default to 32 for compatibility
   -1 to query, returns 1 if 64 bit, 0 if 32.
   0/1 to set 32/64, returns 0 on success, -1 on failure */
int slap_hash64( int onoff )
{
	if ( onoff < 0 ) {
		return hashlen == LUTIL_HASH64_BYTES;
	} else if ( onoff ) {
		hashinit = lutil_HASH64Init;
		hashupdate = lutil_HASH64Update;
		hashfinal = lutil_HASH64Final;
		hashlen = LUTIL_HASH64_BYTES;
	} else {
		hashinit = lutil_HASHInit;
		hashupdate = lutil_HASHUpdate;
		hashfinal = lutil_HASHFinal;
		hashlen = LUTIL_HASH_BYTES;
	}
	return 0;
}

#else
#define HASH_BYTES				LUTIL_HASH_BYTES
#define HASH_LEN				HASH_BYTES
#define HASH_Init(c)			lutil_HASHInit(c)
#define HASH_Update(c,buf,len)	lutil_HASHUpdate(c,buf,len)
#define HASH_Final(d,c)			lutil_HASHFinal(d,c)

int slap_has64( int onoff )
{
	if ( onoff < 0 )
		return 0;
	else
		return onoff ? -1 : 0;
}

#endif
#define HASH_CONTEXT			lutil_HASH_CTX

/* approx matching rules */
#define directoryStringApproxMatchOID	"1.3.6.1.4.1.4203.666.4.4"
#define directoryStringApproxMatch		approxMatch
#define directoryStringApproxIndexer	approxIndexer
#define directoryStringApproxFilter		approxFilter
#define IA5StringApproxMatchOID			"1.3.6.1.4.1.4203.666.4.5"
#define IA5StringApproxMatch			approxMatch
#define IA5StringApproxIndexer			approxIndexer
#define IA5StringApproxFilter			approxFilter

/* Change Sequence Number (CSN) - much of this will change */
#define csnMatch				octetStringMatch
#define csnOrderingMatch		octetStringOrderingMatch
#define csnIndexer				generalizedTimeIndexer
#define csnFilter				generalizedTimeFilter

#define authzMatch				octetStringMatch

/* X.509 PMI ldapSyntaxes */
/* FIXME: need to create temporary OIDs under OpenLDAP's arc;
 * these are currently hijacked
 *
 *	1.3.6.1.4.1.4203.666		OpenLDAP
 *	1.3.6.1.4.1.4203.666.11		self-contained works
 *	1.3.6.1.4.1.4203.666.11.10	X.509 PMI
 *	1.3.6.1.4.1.4203.666.11.10.2	X.509 PMI ldapSyntaxes
 *	1.3.6.1.4.1.4203.666.11.10.2.1	AttributeCertificate (supported)
 *	1.3.6.1.4.1.4203.666.11.10.2.2	AttributeCertificateExactAssertion (supported)
 *	1.3.6.1.4.1.4203.666.11.10.2.3	AttributeCertificateAssertion (not supported)
 *	1.3.6.1.4.1.4203.666.11.10.2.4	AttCertPath (X-SUBST'ed right now in pmi.schema)
 *	1.3.6.1.4.1.4203.666.11.10.2.5	PolicySyntax (X-SUBST'ed right now in pmi.schema)
 *	1.3.6.1.4.1.4203.666.11.10.2.6	RoleSyntax (X-SUBST'ed right now in pmi.schema)
 */
#if 0 /* from <draft-ietf-pkix-ldap-schema-02.txt> (expired) */
#define attributeCertificateSyntaxOID			"1.2.826.0.1.3344810.7.5"
#define attributeCertificateExactAssertionSyntaxOID	"1.2.826.0.1.3344810.7.6"
#define attributeCertificateAssertionSyntaxOID		"1.2.826.0.1.3344810.7.7"
#else /* from OpenLDAP's experimental oid arc */
#define X509_PMI_SyntaxOID				"1.3.6.1.4.1.4203.666.11.10.2"
#define attributeCertificateSyntaxOID			X509_PMI_SyntaxOID ".1"
#define attributeCertificateExactAssertionSyntaxOID	X509_PMI_SyntaxOID ".2"
#define attributeCertificateAssertionSyntaxOID		X509_PMI_SyntaxOID ".3"
#endif

unsigned int index_substr_if_minlen = SLAP_INDEX_SUBSTR_IF_MINLEN_DEFAULT;
unsigned int index_substr_if_maxlen = SLAP_INDEX_SUBSTR_IF_MAXLEN_DEFAULT;
unsigned int index_substr_any_len = SLAP_INDEX_SUBSTR_ANY_LEN_DEFAULT;
unsigned int index_substr_any_step = SLAP_INDEX_SUBSTR_ANY_STEP_DEFAULT;

unsigned int index_intlen = SLAP_INDEX_INTLEN_DEFAULT;
unsigned int index_intlen_strlen = SLAP_INDEX_INTLEN_STRLEN(
	SLAP_INDEX_INTLEN_DEFAULT );

ldap_pvt_thread_mutex_t	ad_index_mutex;
ldap_pvt_thread_mutex_t	ad_undef_mutex;
ldap_pvt_thread_mutex_t	oc_undef_mutex;

static int
generalizedTimeValidate(
	Syntax *syntax,
	struct berval *in );

#ifdef SUPPORT_OBSOLETE_UTC_SYNTAX
static int
utcTimeValidate(
	Syntax *syntax,
	struct berval *in );
#endif /* SUPPORT_OBSOLETE_UTC_SYNTAX */

static int
inValidate(
	Syntax *syntax,
	struct berval *in )
{
	/* no value allowed */
	return LDAP_INVALID_SYNTAX;
}

static int
blobValidate(
	Syntax *syntax,
	struct berval *in )
{
	/* any value allowed */
	return LDAP_SUCCESS;
}

#define berValidate blobValidate

static int
sequenceValidate(
	Syntax *syntax,
	struct berval *in )
{
	if ( in->bv_len < 2 ) return LDAP_INVALID_SYNTAX;
	if ( in->bv_val[0] != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;

	return LDAP_SUCCESS;
}

/* X.509 related stuff */

enum {
	SLAP_X509_V1		= 0,
	SLAP_X509_V2		= 1,
	SLAP_X509_V3		= 2
};

enum {
	SLAP_TAG_UTCTIME		= 0x17U,
	SLAP_TAG_GENERALIZEDTIME	= 0x18U
};


#define	SLAP_X509_OPTION	(LBER_CLASS_CONTEXT|LBER_CONSTRUCTED)

enum {
	SLAP_X509_OPT_C_VERSION		= SLAP_X509_OPTION + 0,
	SLAP_X509_OPT_C_ISSUERUNIQUEID	= LBER_CLASS_CONTEXT + 1,
	SLAP_X509_OPT_C_SUBJECTUNIQUEID	= LBER_CLASS_CONTEXT + 2,
	SLAP_X509_OPT_C_EXTENSIONS	= SLAP_X509_OPTION + 3
};

enum {
	SLAP_X509_OPT_CL_CRLEXTENSIONS	= SLAP_X509_OPTION + 0
};

/*
GeneralName ::= CHOICE {
  otherName                 [0] INSTANCE OF OTHER-NAME,
  rfc822Name                [1] IA5String,
  dNSName                   [2] IA5String,
  x400Address               [3] ORAddress,
  directoryName             [4] Name,
  ediPartyName              [5] EDIPartyName,
  uniformResourceIdentifier [6] IA5String,
  iPAddress                 [7] OCTET STRING,
  registeredID              [8] OBJECT IDENTIFIER }
*/
enum {
	SLAP_X509_GN_OTHERNAME		= SLAP_X509_OPTION + 0,
	SLAP_X509_GN_RFC822NAME		= SLAP_X509_OPTION + 1,
	SLAP_X509_GN_DNSNAME		= SLAP_X509_OPTION + 2,
	SLAP_X509_GN_X400ADDRESS	= SLAP_X509_OPTION + 3,
	SLAP_X509_GN_DIRECTORYNAME	= SLAP_X509_OPTION + 4,
	SLAP_X509_GN_EDIPARTYNAME	= SLAP_X509_OPTION + 5,
	SLAP_X509_GN_URI		= SLAP_X509_OPTION + 6,
	SLAP_X509_GN_IPADDRESS		= SLAP_X509_OPTION + 7,
	SLAP_X509_GN_REGISTEREDID	= SLAP_X509_OPTION + 8
};

/* X.509 PMI related stuff */
enum {
	SLAP_X509AC_V1		= 0,
	SLAP_X509AC_V2		= 1
};

enum {
	SLAP_X509AC_ISSUER	= SLAP_X509_OPTION + 0
};

/* X.509 certificate validation */
static int
certificateValidate( Syntax *syntax, struct berval *in )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len;
	ber_int_t version = SLAP_X509_V1;

	ber_init2( ber, in, LBER_USE_DER );
	tag = ber_skip_tag( ber, &len );	/* Signed wrapper */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	tag = ber_skip_tag( ber, &len );	/* Sequence */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	tag = ber_peek_tag( ber, &len );
	/* Optional version */
	if ( tag == SLAP_X509_OPT_C_VERSION ) {
		tag = ber_skip_tag( ber, &len );
		tag = ber_get_int( ber, &version );
		if ( tag != LBER_INTEGER ) return LDAP_INVALID_SYNTAX;
	}
	/* NOTE: don't try to parse Serial, because it might be longer
	 * than sizeof(ber_int_t); deferred to certificateExactNormalize() */
	tag = ber_skip_tag( ber, &len );	/* Serial */
	if ( tag != LBER_INTEGER ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );	/* Signature Algorithm */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );	/* Issuer DN */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );	/* Validity */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );	/* Subject DN */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );	/* Subject PublicKeyInfo */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );
	if ( tag == SLAP_X509_OPT_C_ISSUERUNIQUEID ) {	/* issuerUniqueID */
		if ( version < SLAP_X509_V2 ) return LDAP_INVALID_SYNTAX;
		ber_skip_data( ber, len );
		tag = ber_skip_tag( ber, &len );
	}
	if ( tag == SLAP_X509_OPT_C_SUBJECTUNIQUEID ) {	/* subjectUniqueID */
		if ( version < SLAP_X509_V2 ) return LDAP_INVALID_SYNTAX;
		ber_skip_data( ber, len );
		tag = ber_skip_tag( ber, &len );
	}
	if ( tag == SLAP_X509_OPT_C_EXTENSIONS ) {	/* Extensions */
		if ( version < SLAP_X509_V3 ) return LDAP_INVALID_SYNTAX;
		tag = ber_skip_tag( ber, &len );
		if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
		ber_skip_data( ber, len );
		tag = ber_skip_tag( ber, &len );
	}
	/* signatureAlgorithm */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );
	/* Signature */
	if ( tag != LBER_BITSTRING ) return LDAP_INVALID_SYNTAX; 
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );
	/* Must be at end now */
	if ( len || tag != LBER_DEFAULT ) return LDAP_INVALID_SYNTAX;
	return LDAP_SUCCESS;
}

/* X.509 certificate list validation */
static int
checkTime( struct berval *in, struct berval *out );

static int
certificateListValidate( Syntax *syntax, struct berval *in )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len, wrapper_len;
	char *wrapper_start;
	int wrapper_ok = 0;
	ber_int_t version = SLAP_X509_V1;
	struct berval bvdn, bvtu;

	ber_init2( ber, in, LBER_USE_DER );
	tag = ber_skip_tag( ber, &wrapper_len );	/* Signed wrapper */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	wrapper_start = ber->ber_ptr;
	tag = ber_skip_tag( ber, &len );	/* Sequence */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	tag = ber_peek_tag( ber, &len );
	/* Optional version */
	if ( tag == LBER_INTEGER ) {
		tag = ber_get_int( ber, &version );
		assert( tag == LBER_INTEGER );
		if ( version != SLAP_X509_V2 ) return LDAP_INVALID_SYNTAX;
	}
	tag = ber_skip_tag( ber, &len );	/* Signature Algorithm */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );
	tag = ber_peek_tag( ber, &len );	/* Issuer DN */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	len = ber_ptrlen( ber );
	bvdn.bv_val = in->bv_val + len;
	bvdn.bv_len = in->bv_len - len;
	tag = ber_skip_tag( ber, &len );
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );	/* thisUpdate */
	/* Time is a CHOICE { UTCTime, GeneralizedTime } */
	if ( tag != SLAP_TAG_UTCTIME && tag != SLAP_TAG_GENERALIZEDTIME ) return LDAP_INVALID_SYNTAX;
	bvtu.bv_val = (char *)ber->ber_ptr;
	bvtu.bv_len = len;
	ber_skip_data( ber, len );
	/* Optional nextUpdate */
	tag = ber_skip_tag( ber, &len );
	if ( tag == SLAP_TAG_UTCTIME || tag == SLAP_TAG_GENERALIZEDTIME ) {
		ber_skip_data( ber, len );
		tag = ber_skip_tag( ber, &len );
	}
	/* revokedCertificates - Sequence of Sequence, Optional */
	if ( tag == LBER_SEQUENCE ) {
		ber_len_t seqlen;
		ber_tag_t stag;
		stag = ber_peek_tag( ber, &seqlen );
		if ( stag == LBER_SEQUENCE || !len ) {
			/* RFC5280 requires non-empty, but X.509(2005) allows empty. */
			if ( len )
				ber_skip_data( ber, len );
			tag = ber_skip_tag( ber, &len );
		}
	}
	/* Optional Extensions - Sequence of Sequence */
	if ( tag == SLAP_X509_OPT_CL_CRLEXTENSIONS ) { /* ? */
		ber_len_t seqlen;
		if ( version != SLAP_X509_V2 ) return LDAP_INVALID_SYNTAX;
		tag = ber_peek_tag( ber, &seqlen );
		if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
		ber_skip_data( ber, len );
		tag = ber_skip_tag( ber, &len );
	}
	/* signatureAlgorithm */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );
	/* Signature */
	if ( tag != LBER_BITSTRING ) return LDAP_INVALID_SYNTAX; 
	ber_skip_data( ber, len );
	if ( ber->ber_ptr == wrapper_start + wrapper_len ) wrapper_ok = 1;
	tag = ber_skip_tag( ber, &len );
	/* Must be at end now */
	/* NOTE: OpenSSL tolerates CL with garbage past the end */
	if ( len || tag != LBER_DEFAULT ) {
		struct berval issuer_dn = BER_BVNULL, thisUpdate;
		char tubuf[STRLENOF("YYYYmmddHHMMSSZ") + 1];
		int rc;

		if ( ! wrapper_ok ) {
			return LDAP_INVALID_SYNTAX;
		}

		rc = dnX509normalize( &bvdn, &issuer_dn );
		if ( rc != LDAP_SUCCESS ) {
			rc = LDAP_INVALID_SYNTAX;
			goto done;
		}

		thisUpdate.bv_val = tubuf;
		thisUpdate.bv_len = sizeof(tubuf); 
		if ( checkTime( &bvtu, &thisUpdate ) ) {
			rc = LDAP_INVALID_SYNTAX;
			goto done;
		}

		Debug( LDAP_DEBUG_ANY,
			"certificateListValidate issuer=\"%s\", thisUpdate=%s: extra cruft past end of certificateList\n",
			issuer_dn.bv_val, thisUpdate.bv_val, 0 );

done:;
		if ( ! BER_BVISNULL( &issuer_dn ) ) {
			ber_memfree( issuer_dn.bv_val );
		}

		return rc;
	}

	return LDAP_SUCCESS;
}

/* X.509 PMI Attribute Certificate Validate */
static int
attributeCertificateValidate( Syntax *syntax, struct berval *in )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len;
	ber_int_t version;
	int cont = 0;

	ber_init2( ber, in, LBER_USE_DER );
	
	tag = ber_skip_tag( ber, &len );	/* Signed wrapper */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;

	tag = ber_skip_tag( ber, &len );	/* Sequence */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;

	tag = ber_peek_tag( ber, &len );	/* Version */
	if ( tag != LBER_INTEGER ) return LDAP_INVALID_SYNTAX;
	tag = ber_get_int( ber, &version );	/* X.509 only allows v2 */
	if ( version != SLAP_X509AC_V2 ) return LDAP_INVALID_SYNTAX;

	tag = ber_skip_tag( ber, &len );	/* Holder */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );

	tag = ber_skip_tag( ber, &len );	/* Issuer */
	if ( tag != SLAP_X509AC_ISSUER ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );

	tag = ber_skip_tag( ber, &len );	/* Signature */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );

	tag = ber_skip_tag( ber, &len );	/* Serial number */
	if ( tag != LBER_INTEGER ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );

	tag = ber_skip_tag( ber, &len );	/* AttCertValidityPeriod */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );

	tag = ber_skip_tag( ber, &len );	/* Attributes */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );

	tag = ber_peek_tag( ber, &len );

	if ( tag == LBER_BITSTRING ) {	/* issuerUniqueID */
		tag = ber_skip_tag( ber, &len );
		ber_skip_data( ber, len );
		tag = ber_peek_tag( ber, &len );
	}

	if ( tag == LBER_SEQUENCE ) {	/* extensions or signatureAlgorithm */
		tag = ber_skip_tag( ber, &len );
		ber_skip_data( ber, len );
		cont++;
		tag = ber_peek_tag( ber, &len );
	}

	if ( tag == LBER_SEQUENCE ) {	/* signatureAlgorithm */
		tag = ber_skip_tag( ber, &len );
		ber_skip_data( ber, len );
		cont++;
		tag = ber_peek_tag( ber, &len );
	}

	if ( tag == LBER_BITSTRING ) {	/* Signature */
		tag = ber_skip_tag( ber, &len );
		ber_skip_data( ber, len );
		cont++;
		tag = ber_peek_tag( ber, &len );
	}

	/* Must be at end now */
	if ( len != 0 || tag != LBER_DEFAULT || cont < 2 ) return LDAP_INVALID_SYNTAX;

	return LDAP_SUCCESS;
}

int
octetStringMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	struct berval *asserted = (struct berval *) assertedValue;
	ber_slen_t d = (ber_slen_t) value->bv_len - (ber_slen_t) asserted->bv_len;

	/* For speed, order first by length, then by contents */
	*matchp = d ? (sizeof(d) == sizeof(int) ? d : d < 0 ? -1 : 1)
		: memcmp( value->bv_val, asserted->bv_val, value->bv_len );

	return LDAP_SUCCESS;
}

int
octetStringOrderingMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	struct berval *asserted = (struct berval *) assertedValue;
	ber_len_t v_len  = value->bv_len;
	ber_len_t av_len = asserted->bv_len;

	int match = memcmp( value->bv_val, asserted->bv_val,
		(v_len < av_len ? v_len : av_len) );

	if( match == 0 )
		match = sizeof(v_len) == sizeof(int)
			? (int) v_len - (int) av_len
			: v_len < av_len ? -1 : v_len > av_len;

	/* If used in extensible match filter, match if value < asserted */
	if ( flags & SLAP_MR_EXT )
		match = (match >= 0);

	*matchp = match;
	return LDAP_SUCCESS;
}

/* Initialize HASHcontext from match type and schema info */
static void
hashPreset(
	HASH_CONTEXT *HASHcontext,
	struct berval *prefix,
	char pre,
	Syntax *syntax,
	MatchingRule *mr)
{
	HASH_Init(HASHcontext);
	if(prefix && prefix->bv_len > 0) {
		HASH_Update(HASHcontext,
			(unsigned char *)prefix->bv_val, prefix->bv_len);
	}
	if(pre) HASH_Update(HASHcontext, (unsigned char*)&pre, sizeof(pre));
	HASH_Update(HASHcontext, (unsigned char*)syntax->ssyn_oid, syntax->ssyn_oidlen);
	HASH_Update(HASHcontext, (unsigned char*)mr->smr_oid, mr->smr_oidlen);
	return;
}

/* Set HASHdigest from HASHcontext and value:len */
static void
hashIter(
	HASH_CONTEXT *HASHcontext,
	unsigned char *HASHdigest,
	unsigned char *value,
	int len)
{
	HASH_CONTEXT ctx = *HASHcontext;
	HASH_Update( &ctx, value, len );
	HASH_Final( HASHdigest, &ctx );
}

/* Index generation function: Attribute values -> index hash keys */
int octetStringIndexer(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	BerVarray values,
	BerVarray *keysp,
	void *ctx )
{
	int i;
	size_t slen, mlen;
	BerVarray keys;
	HASH_CONTEXT HASHcontext;
	unsigned char HASHdigest[HASH_BYTES];
	struct berval digest;
	digest.bv_val = (char *)HASHdigest;
	digest.bv_len = HASH_LEN;

	for( i=0; !BER_BVISNULL( &values[i] ); i++ ) {
		/* just count them */
	}

	/* we should have at least one value at this point */
	assert( i > 0 );

	keys = slap_sl_malloc( sizeof( struct berval ) * (i+1), ctx );

	slen = syntax->ssyn_oidlen;
	mlen = mr->smr_oidlen;

	hashPreset( &HASHcontext, prefix, 0, syntax, mr);
	for( i=0; !BER_BVISNULL( &values[i] ); i++ ) {
		hashIter( &HASHcontext, HASHdigest,
			(unsigned char *)values[i].bv_val, values[i].bv_len );
		ber_dupbv_x( &keys[i], &digest, ctx );
	}

	BER_BVZERO( &keys[i] );

	*keysp = keys;

	return LDAP_SUCCESS;
}

/* Index generation function: Asserted value -> index hash key */
int octetStringFilter(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	void * assertedValue,
	BerVarray *keysp,
	void *ctx )
{
	size_t slen, mlen;
	BerVarray keys;
	HASH_CONTEXT HASHcontext;
	unsigned char HASHdigest[HASH_BYTES];
	struct berval *value = (struct berval *) assertedValue;
	struct berval digest;
	digest.bv_val = (char *)HASHdigest;
	digest.bv_len = HASH_LEN;

	slen = syntax->ssyn_oidlen;
	mlen = mr->smr_oidlen;

	keys = slap_sl_malloc( sizeof( struct berval ) * 2, ctx );

	hashPreset( &HASHcontext, prefix, 0, syntax, mr );
	hashIter( &HASHcontext, HASHdigest,
		(unsigned char *)value->bv_val, value->bv_len );

	ber_dupbv_x( keys, &digest, ctx );
	BER_BVZERO( &keys[1] );

	*keysp = keys;

	return LDAP_SUCCESS;
}

static int
octetStringSubstringsMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	int match = 0;
	SubstringsAssertion *sub = assertedValue;
	struct berval left = *value;
	int i;
	ber_len_t inlen = 0;

	/* Add up asserted input length */
	if ( !BER_BVISNULL( &sub->sa_initial ) ) {
		inlen += sub->sa_initial.bv_len;
	}
	if ( sub->sa_any ) {
		for ( i = 0; !BER_BVISNULL( &sub->sa_any[i] ); i++ ) {
			inlen += sub->sa_any[i].bv_len;
		}
	}
	if ( !BER_BVISNULL( &sub->sa_final ) ) {
		inlen += sub->sa_final.bv_len;
	}

	if ( !BER_BVISNULL( &sub->sa_initial ) ) {
		if ( inlen > left.bv_len ) {
			match = 1;
			goto done;
		}

		match = memcmp( sub->sa_initial.bv_val, left.bv_val,
			sub->sa_initial.bv_len );

		if ( match != 0 ) {
			goto done;
		}

		left.bv_val += sub->sa_initial.bv_len;
		left.bv_len -= sub->sa_initial.bv_len;
		inlen -= sub->sa_initial.bv_len;
	}

	if ( !BER_BVISNULL( &sub->sa_final ) ) {
		if ( inlen > left.bv_len ) {
			match = 1;
			goto done;
		}

		match = memcmp( sub->sa_final.bv_val,
			&left.bv_val[left.bv_len - sub->sa_final.bv_len],
			sub->sa_final.bv_len );

		if ( match != 0 ) {
			goto done;
		}

		left.bv_len -= sub->sa_final.bv_len;
		inlen -= sub->sa_final.bv_len;
	}

	if ( sub->sa_any ) {
		for ( i = 0; !BER_BVISNULL( &sub->sa_any[i] ); i++ ) {
			ber_len_t idx;
			char *p;

retry:
			if ( inlen > left.bv_len ) {
				/* not enough length */
				match = 1;
				goto done;
			}

			if ( BER_BVISEMPTY( &sub->sa_any[i] ) ) {
				continue;
			}

			p = memchr( left.bv_val, *sub->sa_any[i].bv_val, left.bv_len );

			if( p == NULL ) {
				match = 1;
				goto done;
			}

			idx = p - left.bv_val;

			if ( idx >= left.bv_len ) {
				/* this shouldn't happen */
				return LDAP_OTHER;
			}

			left.bv_val = p;
			left.bv_len -= idx;

			if ( sub->sa_any[i].bv_len > left.bv_len ) {
				/* not enough left */
				match = 1;
				goto done;
			}

			match = memcmp( left.bv_val,
				sub->sa_any[i].bv_val,
				sub->sa_any[i].bv_len );

			if ( match != 0 ) {
				left.bv_val++;
				left.bv_len--;
				goto retry;
			}

			left.bv_val += sub->sa_any[i].bv_len;
			left.bv_len -= sub->sa_any[i].bv_len;
			inlen -= sub->sa_any[i].bv_len;
		}
	}

done:
	*matchp = match;
	return LDAP_SUCCESS;
}

/* Substring index generation function: Attribute values -> index hash keys */
static int
octetStringSubstringsIndexer(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	BerVarray values,
	BerVarray *keysp,
	void *ctx )
{
	ber_len_t i, nkeys;
	size_t slen, mlen;
	BerVarray keys;

	HASH_CONTEXT HCany, HCini, HCfin;
	unsigned char HASHdigest[HASH_BYTES];
	struct berval digest;
	digest.bv_val = (char *)HASHdigest;
	digest.bv_len = HASH_LEN;

	nkeys = 0;

	for ( i = 0; !BER_BVISNULL( &values[i] ); i++ ) {
		/* count number of indices to generate */
		if( flags & SLAP_INDEX_SUBSTR_INITIAL ) {
			if( values[i].bv_len >= index_substr_if_maxlen ) {
				nkeys += index_substr_if_maxlen -
					(index_substr_if_minlen - 1);
			} else if( values[i].bv_len >= index_substr_if_minlen ) {
				nkeys += values[i].bv_len - (index_substr_if_minlen - 1);
			}
		}

		if( flags & SLAP_INDEX_SUBSTR_ANY ) {
			if( values[i].bv_len >= index_substr_any_len ) {
				nkeys += values[i].bv_len - (index_substr_any_len - 1);
			}
		}

		if( flags & SLAP_INDEX_SUBSTR_FINAL ) {
			if( values[i].bv_len >= index_substr_if_maxlen ) {
				nkeys += index_substr_if_maxlen -
					(index_substr_if_minlen - 1);
			} else if( values[i].bv_len >= index_substr_if_minlen ) {
				nkeys += values[i].bv_len - (index_substr_if_minlen - 1);
			}
		}
	}

	if( nkeys == 0 ) {
		/* no keys to generate */
		*keysp = NULL;
		return LDAP_SUCCESS;
	}

	keys = slap_sl_malloc( sizeof( struct berval ) * (nkeys+1), ctx );

	slen = syntax->ssyn_oidlen;
	mlen = mr->smr_oidlen;

	if ( flags & SLAP_INDEX_SUBSTR_ANY )
		hashPreset( &HCany, prefix, SLAP_INDEX_SUBSTR_PREFIX, syntax, mr );
	if( flags & SLAP_INDEX_SUBSTR_INITIAL )
		hashPreset( &HCini, prefix, SLAP_INDEX_SUBSTR_INITIAL_PREFIX, syntax, mr );
	if( flags & SLAP_INDEX_SUBSTR_FINAL )
		hashPreset( &HCfin, prefix, SLAP_INDEX_SUBSTR_FINAL_PREFIX, syntax, mr );

	nkeys = 0;
	for ( i = 0; !BER_BVISNULL( &values[i] ); i++ ) {
		ber_len_t j,max;

		if( ( flags & SLAP_INDEX_SUBSTR_ANY ) &&
			( values[i].bv_len >= index_substr_any_len ) )
		{
			max = values[i].bv_len - (index_substr_any_len - 1);

			for( j=0; j<max; j++ ) {
				hashIter( &HCany, HASHdigest,
					(unsigned char *)&values[i].bv_val[j],
					index_substr_any_len );
				ber_dupbv_x( &keys[nkeys++], &digest, ctx );
			}
		}

		/* skip if too short */ 
		if( values[i].bv_len < index_substr_if_minlen ) continue;

		max = index_substr_if_maxlen < values[i].bv_len
			? index_substr_if_maxlen : values[i].bv_len;

		for( j=index_substr_if_minlen; j<=max; j++ ) {

			if( flags & SLAP_INDEX_SUBSTR_INITIAL ) {
				hashIter( &HCini, HASHdigest,
					(unsigned char *)values[i].bv_val, j );
				ber_dupbv_x( &keys[nkeys++], &digest, ctx );
			}

			if( flags & SLAP_INDEX_SUBSTR_FINAL ) {
				hashIter( &HCfin, HASHdigest,
					(unsigned char *)&values[i].bv_val[values[i].bv_len-j], j );
				ber_dupbv_x( &keys[nkeys++], &digest, ctx );
			}

		}
	}

	if( nkeys > 0 ) {
		BER_BVZERO( &keys[nkeys] );
		*keysp = keys;
	} else {
		ch_free( keys );
		*keysp = NULL;
	}

	return LDAP_SUCCESS;
}

/* Substring index generation function: Assertion value -> index hash keys */
static int
octetStringSubstringsFilter (
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	void * assertedValue,
	BerVarray *keysp,
	void *ctx)
{
	SubstringsAssertion *sa;
	char pre;
	ber_len_t nkeys = 0;
	size_t slen, mlen, klen;
	BerVarray keys;
	HASH_CONTEXT HASHcontext;
	unsigned char HASHdigest[HASH_BYTES];
	struct berval *value;
	struct berval digest;

	sa = (SubstringsAssertion *) assertedValue;

	if( flags & SLAP_INDEX_SUBSTR_INITIAL &&
		!BER_BVISNULL( &sa->sa_initial ) &&
		sa->sa_initial.bv_len >= index_substr_if_minlen )
	{
		nkeys++;
		if ( sa->sa_initial.bv_len > index_substr_if_maxlen &&
			( flags & SLAP_INDEX_SUBSTR_ANY ))
		{
			nkeys += 1 + (sa->sa_initial.bv_len - index_substr_if_maxlen) / index_substr_any_step;
		}
	}

	if ( flags & SLAP_INDEX_SUBSTR_ANY && sa->sa_any != NULL ) {
		ber_len_t i;
		for( i=0; !BER_BVISNULL( &sa->sa_any[i] ); i++ ) {
			if( sa->sa_any[i].bv_len >= index_substr_any_len ) {
				/* don't bother accounting with stepping */
				nkeys += sa->sa_any[i].bv_len -
					( index_substr_any_len - 1 );
			}
		}
	}

	if( flags & SLAP_INDEX_SUBSTR_FINAL &&
		!BER_BVISNULL( &sa->sa_final ) &&
		sa->sa_final.bv_len >= index_substr_if_minlen )
	{
		nkeys++;
		if ( sa->sa_final.bv_len > index_substr_if_maxlen &&
			( flags & SLAP_INDEX_SUBSTR_ANY ))
		{
			nkeys += 1 + (sa->sa_final.bv_len - index_substr_if_maxlen) / index_substr_any_step;
		}
	}

	if( nkeys == 0 ) {
		*keysp = NULL;
		return LDAP_SUCCESS;
	}

	digest.bv_val = (char *)HASHdigest;
	digest.bv_len = HASH_LEN;

	slen = syntax->ssyn_oidlen;
	mlen = mr->smr_oidlen;

	keys = slap_sl_malloc( sizeof( struct berval ) * (nkeys+1), ctx );
	nkeys = 0;

	if( flags & SLAP_INDEX_SUBSTR_INITIAL &&
		!BER_BVISNULL( &sa->sa_initial ) &&
		sa->sa_initial.bv_len >= index_substr_if_minlen )
	{
		pre = SLAP_INDEX_SUBSTR_INITIAL_PREFIX;
		value = &sa->sa_initial;

		klen = index_substr_if_maxlen < value->bv_len
			? index_substr_if_maxlen : value->bv_len;

		hashPreset( &HASHcontext, prefix, pre, syntax, mr );
		hashIter( &HASHcontext, HASHdigest,
			(unsigned char *)value->bv_val, klen );
		ber_dupbv_x( &keys[nkeys++], &digest, ctx );

		/* If initial is too long and we have subany indexed, use it
		 * to match the excess...
		 */
		if (value->bv_len > index_substr_if_maxlen && (flags & SLAP_INDEX_SUBSTR_ANY))
		{
			ber_len_t j;
			pre = SLAP_INDEX_SUBSTR_PREFIX;
			hashPreset( &HASHcontext, prefix, pre, syntax, mr);
			for ( j=index_substr_if_maxlen-1; j <= value->bv_len - index_substr_any_len; j+=index_substr_any_step )
			{
				hashIter( &HASHcontext, HASHdigest,
					(unsigned char *)&value->bv_val[j], index_substr_any_len );
				ber_dupbv_x( &keys[nkeys++], &digest, ctx );
			}
		}
	}

	if( flags & SLAP_INDEX_SUBSTR_ANY && sa->sa_any != NULL ) {
		ber_len_t i, j;
		pre = SLAP_INDEX_SUBSTR_PREFIX;
		klen = index_substr_any_len;

		for( i=0; !BER_BVISNULL( &sa->sa_any[i] ); i++ ) {
			if( sa->sa_any[i].bv_len < index_substr_any_len ) {
				continue;
			}

			value = &sa->sa_any[i];

			hashPreset( &HASHcontext, prefix, pre, syntax, mr);
			for(j=0;
				j <= value->bv_len - index_substr_any_len;
				j += index_substr_any_step )
			{
				hashIter( &HASHcontext, HASHdigest,
					(unsigned char *)&value->bv_val[j], klen ); 
				ber_dupbv_x( &keys[nkeys++], &digest, ctx );
			}
		}
	}

	if( flags & SLAP_INDEX_SUBSTR_FINAL &&
		!BER_BVISNULL( &sa->sa_final ) &&
		sa->sa_final.bv_len >= index_substr_if_minlen )
	{
		pre = SLAP_INDEX_SUBSTR_FINAL_PREFIX;
		value = &sa->sa_final;

		klen = index_substr_if_maxlen < value->bv_len
			? index_substr_if_maxlen : value->bv_len;

		hashPreset( &HASHcontext, prefix, pre, syntax, mr );
		hashIter( &HASHcontext, HASHdigest,
			(unsigned char *)&value->bv_val[value->bv_len-klen], klen );
		ber_dupbv_x( &keys[nkeys++], &digest, ctx );

		/* If final is too long and we have subany indexed, use it
		 * to match the excess...
		 */
		if (value->bv_len > index_substr_if_maxlen && (flags & SLAP_INDEX_SUBSTR_ANY))
		{
			ber_len_t j;
			pre = SLAP_INDEX_SUBSTR_PREFIX;
			hashPreset( &HASHcontext, prefix, pre, syntax, mr);
			for ( j=0; j <= value->bv_len - index_substr_if_maxlen; j+=index_substr_any_step )
			{
				hashIter( &HASHcontext, HASHdigest,
					(unsigned char *)&value->bv_val[j], index_substr_any_len );
				ber_dupbv_x( &keys[nkeys++], &digest, ctx );
			}
		}
	}

	if( nkeys > 0 ) {
		BER_BVZERO( &keys[nkeys] );
		*keysp = keys;
	} else {
		ch_free( keys );
		*keysp = NULL;
	}

	return LDAP_SUCCESS;
}

static int
bitStringValidate(
	Syntax *syntax,
	struct berval *in )
{
	ber_len_t i;

	/* very unforgiving validation, requires no normalization
	 * before simplistic matching
	 */
	if( in->bv_len < 3 ) {
		return LDAP_INVALID_SYNTAX;
	}

	/* RFC 4517 Section 3.3.2 Bit String:
	 *	BitString    = SQUOTE *binary-digit SQUOTE "B"
	 *	binary-digit = "0" / "1"
	 *
	 * where SQUOTE [RFC4512] is
	 *	SQUOTE  = %x27 ; single quote ("'")
	 *
	 * Example: '0101111101'B
	 */
	
	if( in->bv_val[0] != '\'' ||
		in->bv_val[in->bv_len - 2] != '\'' ||
		in->bv_val[in->bv_len - 1] != 'B' )
	{
		return LDAP_INVALID_SYNTAX;
	}

	for( i = in->bv_len - 3; i > 0; i-- ) {
		if( in->bv_val[i] != '0' && in->bv_val[i] != '1' ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	return LDAP_SUCCESS;
}

/*
 * Syntaxes from RFC 4517
 *

3.3.2.  Bit String

   A value of the Bit String syntax is a sequence of binary digits.  The
   LDAP-specific encoding of a value of this syntax is defined by the
   following ABNF:

      BitString    = SQUOTE *binary-digit SQUOTE "B"

      binary-digit = "0" / "1"

   The <SQUOTE> rule is defined in [MODELS].

      Example:
         '0101111101'B

   The LDAP definition for the Bit String syntax is:

      ( 1.3.6.1.4.1.1466.115.121.1.6 DESC 'Bit String' )

   This syntax corresponds to the BIT STRING ASN.1 type from [ASN.1].

   ...

3.3.21.  Name and Optional UID

   A value of the Name and Optional UID syntax is the distinguished name
   [MODELS] of an entity optionally accompanied by a unique identifier
   that serves to differentiate the entity from others with an identical
   distinguished name.

   The LDAP-specific encoding of a value of this syntax is defined by
   the following ABNF:

       NameAndOptionalUID = distinguishedName [ SHARP BitString ]

   The <BitString> rule is defined in Section 3.3.2.  The
   <distinguishedName> rule is defined in [LDAPDN].  The <SHARP> rule is
   defined in [MODELS].

   Note that although the '#' character may occur in the string
   representation of a distinguished name, no additional escaping of
   this character is performed when a <distinguishedName> is encoded in
   a <NameAndOptionalUID>.

      Example:
         1.3.6.1.4.1.1466.0=#04024869,O=Test,C=GB#'0101'B

   The LDAP definition for the Name and Optional UID syntax is:

      ( 1.3.6.1.4.1.1466.115.121.1.34 DESC 'Name And Optional UID' )

   This syntax corresponds to the NameAndOptionalUID ASN.1 type from
   [X.520].

 *
 * RFC 4512 says:
 *

1.4. Common ABNF Productions

  ...
      SHARP   = %x23 ; octothorpe (or sharp sign) ("#")
  ...
      SQUOTE  = %x27 ; single quote ("'")
  ...
      
 *
 * Note:
 * RFC 4514 clarifies that SHARP, i.e. "#", doesn't have to
 * be escaped except when at the beginning of a value, the
 * definition of Name and Optional UID appears to be flawed,
 * because there is no clear means to determine whether the
 * UID part is present or not.
 *
 * Example:
 *
 * 	cn=Someone,dc=example,dc=com#'1'B
 *
 * could be either a NameAndOptionalUID with trailing UID, i.e.
 *
 * 	DN = "cn=Someone,dc=example,dc=com"
 * 	UID = "'1'B"
 * 
 * or a NameAndOptionalUID with no trailing UID, and the AVA
 * in the last RDN made of
 *
 * 	attributeType = dc 
 * 	attributeValue = com#'1'B
 *
 * in fact "com#'1'B" is a valid IA5 string.
 *
 * As a consequence, current slapd code takes the presence of
 * #<valid BitString> at the end of the string representation
 * of a NameAndOptionalUID to mean this is indeed a BitString.
 * This is quite arbitrary - it has changed the past and might
 * change in the future.
 */


static int
nameUIDValidate(
	Syntax *syntax,
	struct berval *in )
{
	int rc;
	struct berval dn, uid;

	if( BER_BVISEMPTY( in ) ) return LDAP_SUCCESS;

	ber_dupbv( &dn, in );
	if( !dn.bv_val ) return LDAP_OTHER;

	/* if there's a "#", try bitStringValidate()... */
	uid.bv_val = strrchr( dn.bv_val, '#' );
	if ( !BER_BVISNULL( &uid ) ) {
		uid.bv_val++;
		uid.bv_len = dn.bv_len - ( uid.bv_val - dn.bv_val );

		rc = bitStringValidate( NULL, &uid );
		if ( rc == LDAP_SUCCESS ) {
			/* in case of success, trim the UID,
			 * otherwise treat it as part of the DN */
			dn.bv_len -= uid.bv_len + 1;
			uid.bv_val[-1] = '\0';
		}
	}

	rc = dnValidate( NULL, &dn );

	ber_memfree( dn.bv_val );
	return rc;
}

int
nameUIDPretty(
	Syntax *syntax,
	struct berval *val,
	struct berval *out,
	void *ctx )
{
	assert( val != NULL );
	assert( out != NULL );


	Debug( LDAP_DEBUG_TRACE, ">>> nameUIDPretty: <%s>\n", val->bv_val, 0, 0 );

	if( BER_BVISEMPTY( val ) ) {
		ber_dupbv_x( out, val, ctx );

	} else if ( val->bv_len > SLAP_LDAPDN_MAXLEN ) {
		return LDAP_INVALID_SYNTAX;

	} else {
		int		rc;
		struct berval	dnval = *val;
		struct berval	uidval = BER_BVNULL;

		uidval.bv_val = strrchr( val->bv_val, '#' );
		if ( !BER_BVISNULL( &uidval ) ) {
			uidval.bv_val++;
			uidval.bv_len = val->bv_len - ( uidval.bv_val - val->bv_val );

			rc = bitStringValidate( NULL, &uidval );

			if ( rc == LDAP_SUCCESS ) {
				ber_dupbv_x( &dnval, val, ctx );
				uidval.bv_val--;
				dnval.bv_len -= ++uidval.bv_len;
				dnval.bv_val[dnval.bv_len] = '\0';

			} else {
				BER_BVZERO( &uidval );
			}
		}

		rc = dnPretty( syntax, &dnval, out, ctx );
		if ( dnval.bv_val != val->bv_val ) {
			slap_sl_free( dnval.bv_val, ctx );
		}
		if( rc != LDAP_SUCCESS ) {
			return rc;
		}

		if( !BER_BVISNULL( &uidval ) ) {
			char	*tmp;

			tmp = slap_sl_realloc( out->bv_val, out->bv_len 
				+ uidval.bv_len + 1,
				ctx );
			if( tmp == NULL ) {
				ber_memfree_x( out->bv_val, ctx );
				return LDAP_OTHER;
			}
			out->bv_val = tmp;
			memcpy( out->bv_val + out->bv_len, uidval.bv_val, uidval.bv_len );
			out->bv_len += uidval.bv_len;
			out->bv_val[out->bv_len] = '\0';
		}
	}

	Debug( LDAP_DEBUG_TRACE, "<<< nameUIDPretty: <%s>\n", out->bv_val, 0, 0 );

	return LDAP_SUCCESS;
}

static int
uniqueMemberNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	struct berval out;
	int rc;

	assert( SLAP_MR_IS_VALUE_OF_SYNTAX( usage ) != 0 );

	ber_dupbv_x( &out, val, ctx );
	if ( BER_BVISEMPTY( &out ) ) {
		*normalized = out;

	} else {
		struct berval uid = BER_BVNULL;

		uid.bv_val = strrchr( out.bv_val, '#' );
		if ( !BER_BVISNULL( &uid ) ) {
			uid.bv_val++;
			uid.bv_len = out.bv_len - ( uid.bv_val - out.bv_val );

			rc = bitStringValidate( NULL, &uid );
			if ( rc == LDAP_SUCCESS ) {
				uid.bv_val[-1] = '\0';
				out.bv_len -= uid.bv_len + 1;
			} else {
				BER_BVZERO( &uid );
			}
		}

		rc = dnNormalize( 0, NULL, NULL, &out, normalized, ctx );

		if( rc != LDAP_SUCCESS ) {
			slap_sl_free( out.bv_val, ctx );
			return LDAP_INVALID_SYNTAX;
		}

		if( !BER_BVISNULL( &uid ) ) {
			char	*tmp;

			tmp = ch_realloc( normalized->bv_val,
				normalized->bv_len + uid.bv_len
				+ STRLENOF("#") + 1 );
			if ( tmp == NULL ) {
				ber_memfree_x( normalized->bv_val, ctx );
				return LDAP_OTHER;
			}

			normalized->bv_val = tmp;

			/* insert the separator */
			normalized->bv_val[normalized->bv_len++] = '#';

			/* append the UID */
			AC_MEMCPY( &normalized->bv_val[normalized->bv_len],
				uid.bv_val, uid.bv_len );
			normalized->bv_len += uid.bv_len;

			/* terminate */
			normalized->bv_val[normalized->bv_len] = '\0';
		}

		slap_sl_free( out.bv_val, ctx );
	}

	return LDAP_SUCCESS;
}

static int
uniqueMemberMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	int match;
	struct berval *asserted = (struct berval *) assertedValue;
	struct berval assertedDN = *asserted;
	struct berval assertedUID = BER_BVNULL;
	struct berval valueDN = *value;
	struct berval valueUID = BER_BVNULL;
	int approx = ((flags & SLAP_MR_EQUALITY_APPROX) == SLAP_MR_EQUALITY_APPROX);

	if ( !BER_BVISEMPTY( asserted ) ) {
		assertedUID.bv_val = strrchr( assertedDN.bv_val, '#' );
		if ( !BER_BVISNULL( &assertedUID ) ) {
			assertedUID.bv_val++;
			assertedUID.bv_len = assertedDN.bv_len
				- ( assertedUID.bv_val - assertedDN.bv_val );

			if ( bitStringValidate( NULL, &assertedUID ) == LDAP_SUCCESS ) {
				assertedDN.bv_len -= assertedUID.bv_len + 1;

			} else {
				BER_BVZERO( &assertedUID );
			}
		}
	}

	if ( !BER_BVISEMPTY( value ) ) {

		valueUID.bv_val = strrchr( valueDN.bv_val, '#' );
		if ( !BER_BVISNULL( &valueUID ) ) {
			valueUID.bv_val++;
			valueUID.bv_len = valueDN.bv_len
				- ( valueUID.bv_val - valueDN.bv_val );

			if ( bitStringValidate( NULL, &valueUID ) == LDAP_SUCCESS ) {
				valueDN.bv_len -= valueUID.bv_len + 1;

			} else {
				BER_BVZERO( &valueUID );
			}
		}
	}

	if( valueUID.bv_len && assertedUID.bv_len ) {
		ber_slen_t d;
		d = (ber_slen_t) valueUID.bv_len - (ber_slen_t) assertedUID.bv_len;
		if ( d ) {
			*matchp = sizeof(d) == sizeof(int) ? d : d < 0 ? -1 : 1;
			return LDAP_SUCCESS;
		}

		match = memcmp( valueUID.bv_val, assertedUID.bv_val, valueUID.bv_len );
		if( match ) {
			*matchp = match;
			return LDAP_SUCCESS;
		}

	} else if ( !approx && valueUID.bv_len ) {
		match = -1;
		*matchp = match;
		return LDAP_SUCCESS;

	} else if ( !approx && assertedUID.bv_len ) {
		match = 1;
		*matchp = match;
		return LDAP_SUCCESS;
	}

	return dnMatch( matchp, flags, syntax, mr, &valueDN, &assertedDN );
}

static int 
uniqueMemberIndexer(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	BerVarray values,
	BerVarray *keysp,
	void *ctx )
{
	BerVarray dnvalues;
	int rc;
	int i;
	for( i=0; !BER_BVISNULL( &values[i] ); i++ ) {
		/* just count them */                 
	}
	assert( i > 0 );

	dnvalues = slap_sl_malloc( sizeof( struct berval ) * (i+1), ctx );

	for( i=0; !BER_BVISNULL( &values[i] ); i++ ) {
		struct berval assertedDN = values[i];
		struct berval assertedUID = BER_BVNULL;

		if ( !BER_BVISEMPTY( &assertedDN ) ) {
			assertedUID.bv_val = strrchr( assertedDN.bv_val, '#' );
			if ( !BER_BVISNULL( &assertedUID ) ) {
				assertedUID.bv_val++;
				assertedUID.bv_len = assertedDN.bv_len
					- ( assertedUID.bv_val - assertedDN.bv_val );
	
				if ( bitStringValidate( NULL, &assertedUID ) == LDAP_SUCCESS ) {
					assertedDN.bv_len -= assertedUID.bv_len + 1;

				} else {
					BER_BVZERO( &assertedUID );
				}
			}
		}

		dnvalues[i] = assertedDN;
	}
	BER_BVZERO( &dnvalues[i] );

	rc = octetStringIndexer( use, flags, syntax, mr, prefix,
		dnvalues, keysp, ctx );

	slap_sl_free( dnvalues, ctx );
	return rc;
}

static int 
uniqueMemberFilter(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	void * assertedValue,
	BerVarray *keysp,
	void *ctx )
{
	struct berval *asserted = (struct berval *) assertedValue;
	struct berval assertedDN = *asserted;
	struct berval assertedUID = BER_BVNULL;

	if ( !BER_BVISEMPTY( asserted ) ) {
		assertedUID.bv_val = strrchr( assertedDN.bv_val, '#' );
		if ( !BER_BVISNULL( &assertedUID ) ) {
			assertedUID.bv_val++;
			assertedUID.bv_len = assertedDN.bv_len
				- ( assertedUID.bv_val - assertedDN.bv_val );

			if ( bitStringValidate( NULL, &assertedUID ) == LDAP_SUCCESS ) {
				assertedDN.bv_len -= assertedUID.bv_len + 1;

			} else {
				BER_BVZERO( &assertedUID );
			}
		}
	}

	return octetStringFilter( use, flags, syntax, mr, prefix,
		&assertedDN, keysp, ctx );
}


/*
 * Handling boolean syntax and matching is quite rigid.
 * A more flexible approach would be to allow a variety
 * of strings to be normalized and prettied into TRUE
 * and FALSE.
 */
static int
booleanValidate(
	Syntax *syntax,
	struct berval *in )
{
	/* very unforgiving validation, requires no normalization
	 * before simplistic matching
	 */

	if( in->bv_len == 4 ) {
		if( bvmatch( in, &slap_true_bv ) ) {
			return LDAP_SUCCESS;
		}
	} else if( in->bv_len == 5 ) {
		if( bvmatch( in, &slap_false_bv ) ) {
			return LDAP_SUCCESS;
		}
	}

	return LDAP_INVALID_SYNTAX;
}

static int
booleanMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	/* simplistic matching allowed by rigid validation */
	struct berval *asserted = (struct berval *) assertedValue;
	*matchp = (int) asserted->bv_len - (int) value->bv_len;
	return LDAP_SUCCESS;
}

/*-------------------------------------------------------------------
LDAP/X.500 string syntax / matching rules have a few oddities.  This
comment attempts to detail how slapd(8) treats them.

Summary:
  StringSyntax		X.500	LDAP	Matching/Comments
  DirectoryString	CHOICE	UTF8	i/e + ignore insignificant spaces
  PrintableString	subset	subset	i/e + ignore insignificant spaces
  PrintableString	subset	subset	i/e + ignore insignificant spaces
  NumericString		subset	subset	ignore all spaces
  IA5String			ASCII	ASCII	i/e + ignore insignificant spaces
  TeletexString		T.61	T.61	i/e + ignore insignificant spaces

  TelephoneNumber	subset	subset	i + ignore all spaces and "-"

  See RFC 4518 for details.


Directory String -
  In X.500(93), a directory string can be either a PrintableString,
  a bmpString, or a UniversalString (e.g., UCS (a subset of Unicode)).
  In later versions, more CHOICEs were added.  In all cases the string
  must be non-empty.

  In LDAPv3, a directory string is a UTF-8 encoded UCS string.
  A directory string cannot be zero length.

  For matching, there are both case ignore and exact rules.  Both
  also require that "insignificant" spaces be ignored.
	spaces before the first non-space are ignored;
	spaces after the last non-space are ignored;
	spaces after a space are ignored.
  Note: by these rules (and as clarified in X.520), a string of only
  spaces is to be treated as if held one space, not empty (which
  would be a syntax error).

NumericString
  In ASN.1, numeric string is just a string of digits and spaces
  and could be empty.  However, in X.500, all attribute values of
  numeric string carry a non-empty constraint.  For example:

	internationalISDNNumber ATTRIBUTE ::= {
		WITH SYNTAX InternationalISDNNumber
		EQUALITY MATCHING RULE numericStringMatch
		SUBSTRINGS MATCHING RULE numericStringSubstringsMatch
		ID id-at-internationalISDNNumber }
	InternationalISDNNumber ::=
	    NumericString (SIZE(1..ub-international-isdn-number))

  Unforunately, some assertion values are don't carry the same
  constraint (but its unclear how such an assertion could ever
  be true). In LDAP, there is one syntax (numericString) not two
  (numericString with constraint, numericString without constraint).
  This should be treated as numericString with non-empty constraint.
  Note that while someone may have no ISDN number, there are no ISDN
  numbers which are zero length.

  In matching, spaces are ignored.

PrintableString
  In ASN.1, Printable string is just a string of printable characters
  and can be empty.  In X.500, semantics much like NumericString (see
  serialNumber for a like example) excepting uses insignificant space
  handling instead of ignore all spaces.  They must be non-empty.

IA5String
  Basically same as PrintableString.  There are no examples in X.500,
  but same logic applies.  Empty strings are allowed.

-------------------------------------------------------------------*/

static int
UTF8StringValidate(
	Syntax *syntax,
	struct berval *in )
{
	ber_len_t count;
	int len;
	unsigned char *u = (unsigned char *)in->bv_val;

	if( BER_BVISEMPTY( in ) && syntax == slap_schema.si_syn_directoryString ) {
		/* directory strings cannot be empty */
		return LDAP_INVALID_SYNTAX;
	}

	for( count = in->bv_len; count > 0; count -= len, u += len ) {
		/* get the length indicated by the first byte */
		len = LDAP_UTF8_CHARLEN2( u, len );

		/* very basic checks */
		switch( len ) {
			case 6:
				if( (u[5] & 0xC0) != 0x80 ) {
					return LDAP_INVALID_SYNTAX;
				}
			case 5:
				if( (u[4] & 0xC0) != 0x80 ) {
					return LDAP_INVALID_SYNTAX;
				}
			case 4:
				if( (u[3] & 0xC0) != 0x80 ) {
					return LDAP_INVALID_SYNTAX;
				}
			case 3:
				if( (u[2] & 0xC0 )!= 0x80 ) {
					return LDAP_INVALID_SYNTAX;
				}
			case 2:
				if( (u[1] & 0xC0) != 0x80 ) {
					return LDAP_INVALID_SYNTAX;
				}
			case 1:
				/* CHARLEN already validated it */
				break;
			default:
				return LDAP_INVALID_SYNTAX;
		}

		/* make sure len corresponds with the offset
			to the next character */
		if( LDAP_UTF8_OFFSET( (char *)u ) != len ) return LDAP_INVALID_SYNTAX;
	}

	if( count != 0 ) {
		return LDAP_INVALID_SYNTAX;
	}

	return LDAP_SUCCESS;
}

static int
UTF8StringNormalize(
	slap_mask_t use,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	struct berval tmp, nvalue;
	int flags, wasspace;
	ber_len_t i;

	assert( SLAP_MR_IS_VALUE_OF_SYNTAX( use ) != 0 );

	if( BER_BVISNULL( val ) ) {
		/* assume we're dealing with a syntax (e.g., UTF8String)
		 * which allows empty strings
		 */
		BER_BVZERO( normalized );
		return LDAP_SUCCESS;
	}

	flags = SLAP_MR_ASSOCIATED( mr, slap_schema.si_mr_caseExactMatch )
		? LDAP_UTF8_NOCASEFOLD : LDAP_UTF8_CASEFOLD;
	flags |= ( ( use & SLAP_MR_EQUALITY_APPROX ) == SLAP_MR_EQUALITY_APPROX )
		? LDAP_UTF8_APPROX : 0;

	val = UTF8bvnormalize( val, &tmp, flags, ctx );
	/* out of memory or syntax error, the former is unlikely */
	if( val == NULL ) {
		return LDAP_INVALID_SYNTAX;
	}
	
	/* collapse spaces (in place) */
	nvalue.bv_len = 0;
	nvalue.bv_val = tmp.bv_val;

	/* trim leading spaces? */
	wasspace = !((( use & SLAP_MR_SUBSTR_ANY ) == SLAP_MR_SUBSTR_ANY ) ||
		(( use & SLAP_MR_SUBSTR_FINAL ) == SLAP_MR_SUBSTR_FINAL ));

	for( i = 0; i < tmp.bv_len; i++) {
		if ( ASCII_SPACE( tmp.bv_val[i] )) {
			if( wasspace++ == 0 ) {
				/* trim repeated spaces */
				nvalue.bv_val[nvalue.bv_len++] = tmp.bv_val[i];
			}
		} else {
			wasspace = 0;
			nvalue.bv_val[nvalue.bv_len++] = tmp.bv_val[i];
		}
	}

	if( !BER_BVISEMPTY( &nvalue ) ) {
		/* trim trailing space? */
		if( wasspace && (
			(( use & SLAP_MR_SUBSTR_INITIAL ) != SLAP_MR_SUBSTR_INITIAL ) &&
			( use & SLAP_MR_SUBSTR_ANY ) != SLAP_MR_SUBSTR_ANY ))
		{
			--nvalue.bv_len;
		}
		nvalue.bv_val[nvalue.bv_len] = '\0';

	} else if ( tmp.bv_len )  {
		/* string of all spaces is treated as one space */
		nvalue.bv_val[0] = ' ';
		nvalue.bv_val[1] = '\0';
		nvalue.bv_len = 1;
	}	/* should never be entered with 0-length val */

	*normalized = nvalue;
	return LDAP_SUCCESS;
}

static int
directoryStringSubstringsMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	int match = 0;
	SubstringsAssertion *sub = assertedValue;
	struct berval left = *value;
	ber_len_t i;
	int priorspace=0;

	if ( !BER_BVISNULL( &sub->sa_initial ) ) {
		if ( sub->sa_initial.bv_len > left.bv_len ) {
			/* not enough left */
			match = 1;
			goto done;
		}

		match = memcmp( sub->sa_initial.bv_val, left.bv_val,
			sub->sa_initial.bv_len );

		if ( match != 0 ) {
			goto done;
		}

		left.bv_val += sub->sa_initial.bv_len;
		left.bv_len -= sub->sa_initial.bv_len;

		priorspace = ASCII_SPACE(
			sub->sa_initial.bv_val[sub->sa_initial.bv_len] );
	}

	if ( sub->sa_any ) {
		for ( i = 0; !BER_BVISNULL( &sub->sa_any[i] ); i++ ) {
			ber_len_t idx;
			char *p;

			if( priorspace && !BER_BVISEMPTY( &sub->sa_any[i] ) 
				&& ASCII_SPACE( sub->sa_any[i].bv_val[0] ))
			{ 
				/* allow next space to match */
				left.bv_val--;
				left.bv_len++;
			}
			priorspace=0;

retry:
			if ( BER_BVISEMPTY( &sub->sa_any[i] ) ) {
				continue;
			}

			if ( sub->sa_any[i].bv_len > left.bv_len ) {
				/* not enough left */
				match = 1;
				goto done;
			}

			p = memchr( left.bv_val, *sub->sa_any[i].bv_val, left.bv_len );

			if( p == NULL ) {
				match = 1;
				goto done;
			}

			idx = p - left.bv_val;

			if ( idx >= left.bv_len ) {
				/* this shouldn't happen */
				return LDAP_OTHER;
			}

			left.bv_val = p;
			left.bv_len -= idx;

			if ( sub->sa_any[i].bv_len > left.bv_len ) {
				/* not enough left */
				match = 1;
				goto done;
			}

			match = memcmp( left.bv_val,
				sub->sa_any[i].bv_val,
				sub->sa_any[i].bv_len );

			if ( match != 0 ) {
				left.bv_val++;
				left.bv_len--;
				goto retry;
			}

			left.bv_val += sub->sa_any[i].bv_len;
			left.bv_len -= sub->sa_any[i].bv_len;

			priorspace = ASCII_SPACE(
				sub->sa_any[i].bv_val[sub->sa_any[i].bv_len] );
		}
	}

	if ( !BER_BVISNULL( &sub->sa_final ) ) {
		if( priorspace && !BER_BVISEMPTY( &sub->sa_final ) 
			&& ASCII_SPACE( sub->sa_final.bv_val[0] ))
		{ 
			/* allow next space to match */
			left.bv_val--;
			left.bv_len++;
		}

		if ( sub->sa_final.bv_len > left.bv_len ) {
			/* not enough left */
			match = 1;
			goto done;
		}

		match = memcmp( sub->sa_final.bv_val,
			&left.bv_val[left.bv_len - sub->sa_final.bv_len],
			sub->sa_final.bv_len );

		if ( match != 0 ) {
			goto done;
		}
	}

done:
	*matchp = match;
	return LDAP_SUCCESS;
}

#if defined(SLAPD_APPROX_INITIALS)
#	define SLAPD_APPROX_DELIMITER "._ "
#	define SLAPD_APPROX_WORDLEN 2
#else
#	define SLAPD_APPROX_DELIMITER " "
#	define SLAPD_APPROX_WORDLEN 1
#endif

static int
approxMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	struct berval *nval, *assertv;
	char *val, **values, **words, *c;
	int i, count, len, nextchunk=0, nextavail=0;

	/* Yes, this is necessary */
	nval = UTF8bvnormalize( value, NULL, LDAP_UTF8_APPROX, NULL );
	if( nval == NULL ) {
		*matchp = 1;
		return LDAP_SUCCESS;
	}

	/* Yes, this is necessary */
	assertv = UTF8bvnormalize( ((struct berval *)assertedValue),
		NULL, LDAP_UTF8_APPROX, NULL );
	if( assertv == NULL ) {
		ber_bvfree( nval );
		*matchp = 1;
		return LDAP_SUCCESS;
	}

	/* Isolate how many words there are */
	for ( c = nval->bv_val, count = 1; *c; c++ ) {
		c = strpbrk( c, SLAPD_APPROX_DELIMITER );
		if ( c == NULL ) break;
		*c = '\0';
		count++;
	}

	/* Get a phonetic copy of each word */
	words = (char **)ch_malloc( count * sizeof(char *) );
	values = (char **)ch_malloc( count * sizeof(char *) );
	for ( c = nval->bv_val, i = 0;  i < count; i++, c += strlen(c) + 1 ) {
		words[i] = c;
		values[i] = phonetic(c);
	}

	/* Work through the asserted value's words, to see if at least some
	 * of the words are there, in the same order. */
	len = 0;
	while ( (ber_len_t) nextchunk < assertv->bv_len ) {
		len = strcspn( assertv->bv_val + nextchunk, SLAPD_APPROX_DELIMITER);
		if( len == 0 ) {
			nextchunk++;
			continue;
		}
#if defined(SLAPD_APPROX_INITIALS)
		else if( len == 1 ) {
			/* Single letter words need to at least match one word's initial */
			for( i=nextavail; i<count; i++ )
				if( !strncasecmp( assertv->bv_val + nextchunk, words[i], 1 )) {
					nextavail=i+1;
					break;
				}
		}
#endif
		else {
			/* Isolate the next word in the asserted value and phonetic it */
			assertv->bv_val[nextchunk+len] = '\0';
			val = phonetic( assertv->bv_val + nextchunk );

			/* See if this phonetic chunk is in the remaining words of *value */
			for( i=nextavail; i<count; i++ ){
				if( !strcmp( val, values[i] ) ){
					nextavail = i+1;
					break;
				}
			}
			ch_free( val );
		}

		/* This chunk in the asserted value was NOT within the *value. */
		if( i >= count ) {
			nextavail=-1;
			break;
		}

		/* Go on to the next word in the asserted value */
		nextchunk += len+1;
	}

	/* If some of the words were seen, call it a match */
	if( nextavail > 0 ) {
		*matchp = 0;
	}
	else {
		*matchp = 1;
	}

	/* Cleanup allocs */
	ber_bvfree( assertv );
	for( i=0; i<count; i++ ) {
		ch_free( values[i] );
	}
	ch_free( values );
	ch_free( words );
	ber_bvfree( nval );

	return LDAP_SUCCESS;
}

static int 
approxIndexer(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	BerVarray values,
	BerVarray *keysp,
	void *ctx )
{
	char *c;
	int i,j, len, wordcount, keycount=0;
	struct berval *newkeys;
	BerVarray keys=NULL;

	for( j = 0; !BER_BVISNULL( &values[j] ); j++ ) {
		struct berval val = BER_BVNULL;
		/* Yes, this is necessary */
		UTF8bvnormalize( &values[j], &val, LDAP_UTF8_APPROX, NULL );
		assert( !BER_BVISNULL( &val ) );

		/* Isolate how many words there are. There will be a key for each */
		for( wordcount = 0, c = val.bv_val; *c; c++) {
			len = strcspn(c, SLAPD_APPROX_DELIMITER);
			if( len >= SLAPD_APPROX_WORDLEN ) wordcount++;
			c+= len;
			if (*c == '\0') break;
			*c = '\0';
		}

		/* Allocate/increase storage to account for new keys */
		newkeys = (struct berval *)ch_malloc( (keycount + wordcount + 1) 
			* sizeof(struct berval) );
		AC_MEMCPY( newkeys, keys, keycount * sizeof(struct berval) );
		if( keys ) ch_free( keys );
		keys = newkeys;

		/* Get a phonetic copy of each word */
		for( c = val.bv_val, i = 0; i < wordcount; c += len + 1 ) {
			len = strlen( c );
			if( len < SLAPD_APPROX_WORDLEN ) continue;
			ber_str2bv( phonetic( c ), 0, 0, &keys[keycount] );
			if( keys[keycount].bv_len ) {
				keycount++;
			} else {
				ch_free( keys[keycount].bv_val );
			}
			i++;
		}

		ber_memfree( val.bv_val );
	}
	BER_BVZERO( &keys[keycount] );
	*keysp = keys;

	return LDAP_SUCCESS;
}

static int 
approxFilter(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	void * assertedValue,
	BerVarray *keysp,
	void *ctx )
{
	char *c;
	int i, count, len;
	struct berval *val;
	BerVarray keys;

	/* Yes, this is necessary */
	val = UTF8bvnormalize( ((struct berval *)assertedValue),
		NULL, LDAP_UTF8_APPROX, NULL );
	if( val == NULL || BER_BVISNULL( val ) ) {
		keys = (struct berval *)ch_malloc( sizeof(struct berval) );
		BER_BVZERO( &keys[0] );
		*keysp = keys;
		ber_bvfree( val );
		return LDAP_SUCCESS;
	}

	/* Isolate how many words there are. There will be a key for each */
	for( count = 0,c = val->bv_val; *c; c++) {
		len = strcspn(c, SLAPD_APPROX_DELIMITER);
		if( len >= SLAPD_APPROX_WORDLEN ) count++;
		c+= len;
		if (*c == '\0') break;
		*c = '\0';
	}

	/* Allocate storage for new keys */
	keys = (struct berval *)ch_malloc( (count + 1) * sizeof(struct berval) );

	/* Get a phonetic copy of each word */
	for( c = val->bv_val, i = 0; i < count; c += len + 1 ) {
		len = strlen(c);
		if( len < SLAPD_APPROX_WORDLEN ) continue;
		ber_str2bv( phonetic( c ), 0, 0, &keys[i] );
		i++;
	}

	ber_bvfree( val );

	BER_BVZERO( &keys[count] );
	*keysp = keys;

	return LDAP_SUCCESS;
}

/* Remove all spaces and '-' characters */
static int
telephoneNumberNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	char *p, *q;

	assert( SLAP_MR_IS_VALUE_OF_SYNTAX( usage ) != 0 );

	/* validator should have refused an empty string */
	assert( !BER_BVISEMPTY( val ) );

	q = normalized->bv_val = slap_sl_malloc( val->bv_len + 1, ctx );

	for( p = val->bv_val; *p; p++ ) {
		if ( ! ( ASCII_SPACE( *p ) || *p == '-' )) {
			*q++ = *p;
		}
	}
	*q = '\0';

	normalized->bv_len = q - normalized->bv_val;

	if( BER_BVISEMPTY( normalized ) ) {
		slap_sl_free( normalized->bv_val, ctx );
		BER_BVZERO( normalized );
		return LDAP_INVALID_SYNTAX;
	}

	return LDAP_SUCCESS;
}

static int
postalAddressValidate(
	Syntax *syntax,
	struct berval *in )
{
	struct berval bv = *in;
	ber_len_t c;

	for ( c = 0; c < in->bv_len; c++ ) {
		if ( in->bv_val[c] == '\\' ) {
			c++;
			if ( strncasecmp( &in->bv_val[c], "24", STRLENOF( "24" ) ) != 0
				&& strncasecmp( &in->bv_val[c], "5C", STRLENOF( "5C" ) ) != 0 )
			{
				return LDAP_INVALID_SYNTAX;
			}
			continue;
		}

		if ( in->bv_val[c] == '$' ) {
			bv.bv_len = &in->bv_val[c] - bv.bv_val;
			if ( UTF8StringValidate( NULL, &bv ) != LDAP_SUCCESS ) {
				return LDAP_INVALID_SYNTAX;
			}
			bv.bv_val = &in->bv_val[c] + 1;
		}
	}

	bv.bv_len = &in->bv_val[c] - bv.bv_val;
	return UTF8StringValidate( NULL, &bv );
}

static int
postalAddressNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	BerVarray lines = NULL, nlines = NULL;
	ber_len_t l, c;
	int rc = LDAP_SUCCESS;
	MatchingRule *xmr = NULL;
	char *p;

	if ( SLAP_MR_ASSOCIATED( mr, slap_schema.si_mr_caseIgnoreListMatch ) ) {
		xmr = slap_schema.si_mr_caseIgnoreMatch;

	} else {
		xmr = slap_schema.si_mr_caseExactMatch;
	}

	for ( l = 0, c = 0; c < val->bv_len; c++ ) {
		if ( val->bv_val[c] == '$' ) {
			l++;
		}
	}

	lines = slap_sl_calloc( sizeof( struct berval ), 2 * ( l + 2 ), ctx );
	nlines = &lines[l + 2];

	lines[0].bv_val = val->bv_val;
	for ( l = 0, c = 0; c < val->bv_len; c++ ) {
		if ( val->bv_val[c] == '$' ) {
			lines[l].bv_len = &val->bv_val[c] - lines[l].bv_val;
			l++;
			lines[l].bv_val = &val->bv_val[c + 1];
		}
	}
	lines[l].bv_len = &val->bv_val[c] - lines[l].bv_val;

	normalized->bv_len = c = l;

	for ( l = 0; l <= c; l++ ) {
		/* NOTE: we directly normalize each line,
		 * without unescaping the values, since the special
		 * values '\24' ('$') and '\5C' ('\') are not affected
		 * by normalization */
		if ( !lines[l].bv_len ) {
			nlines[l].bv_len = 0;
			nlines[l].bv_val = NULL;
			continue;
		}
		rc = UTF8StringNormalize( usage, NULL, xmr, &lines[l], &nlines[l], ctx );
		if ( rc != LDAP_SUCCESS ) {
			rc = LDAP_INVALID_SYNTAX;
			goto done;
		}

		normalized->bv_len += nlines[l].bv_len;
	}

	normalized->bv_val = slap_sl_malloc( normalized->bv_len + 1, ctx );

	p = normalized->bv_val;
	for ( l = 0; l <= c ; l++ ) {
		p = lutil_strbvcopy( p, &nlines[l] );
		*p++ = '$';
	}
	*--p = '\0';

	assert( p == &normalized->bv_val[normalized->bv_len] );

done:;
	if ( nlines != NULL ) {
		for ( l = 0; !BER_BVISNULL( &nlines[ l ] ); l++ ) {
			slap_sl_free( nlines[l].bv_val, ctx );
		}

		slap_sl_free( lines, ctx );
	}

	return rc;
}

int
numericoidValidate(
	Syntax *syntax,
	struct berval *in )
{
	struct berval val = *in;

	if( BER_BVISEMPTY( &val ) ) {
		/* disallow empty strings */
		return LDAP_INVALID_SYNTAX;
	}

	while( OID_LEADCHAR( val.bv_val[0] ) ) {
		if ( val.bv_len == 1 ) {
			return LDAP_SUCCESS;
		}

		if ( val.bv_val[0] == '0' && !OID_SEPARATOR( val.bv_val[1] )) {
			break;
		}

		val.bv_val++;
		val.bv_len--;

		while ( OID_LEADCHAR( val.bv_val[0] )) {
			val.bv_val++;
			val.bv_len--;

			if ( val.bv_len == 0 ) {
				return LDAP_SUCCESS;
			}
		}

		if( !OID_SEPARATOR( val.bv_val[0] )) {
			break;
		}

		val.bv_val++;
		val.bv_len--;
	}

	return LDAP_INVALID_SYNTAX;
}

static int
integerValidate(
	Syntax *syntax,
	struct berval *in )
{
	ber_len_t i;
	struct berval val = *in;

	if ( BER_BVISEMPTY( &val ) ) return LDAP_INVALID_SYNTAX;

	if ( val.bv_val[0] == '-' ) {
		val.bv_len--;
		val.bv_val++;

		if( BER_BVISEMPTY( &val ) ) { /* bare "-" */
			return LDAP_INVALID_SYNTAX;
		}

		if( val.bv_val[0] == '0' ) { /* "-0" */
			return LDAP_INVALID_SYNTAX;
		}

	} else if ( val.bv_val[0] == '0' ) {
		if( val.bv_len > 1 ) { /* "0<more>" */
			return LDAP_INVALID_SYNTAX;
		}

		return LDAP_SUCCESS;
	}

	for( i=0; i < val.bv_len; i++ ) {
		if( !ASCII_DIGIT(val.bv_val[i]) ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	return LDAP_SUCCESS;
}

static int
integerMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	struct berval *asserted = (struct berval *) assertedValue;
	int vsign = 1, asign = 1;	/* default sign = '+' */
	struct berval v, a;
	int match;

	v = *value;
	if( v.bv_val[0] == '-' ) {
		vsign = -1;
		v.bv_val++;
		v.bv_len--;
	}

	if( BER_BVISEMPTY( &v ) ) vsign = 0;

	a = *asserted;
	if( a.bv_val[0] == '-' ) {
		asign = -1;
		a.bv_val++;
		a.bv_len--;
	}

	if( BER_BVISEMPTY( &a ) ) vsign = 0;

	match = vsign - asign;
	if( match == 0 ) {
		match = ( v.bv_len != a.bv_len
			? ( v.bv_len < a.bv_len ? -1 : 1 )
			: memcmp( v.bv_val, a.bv_val, v.bv_len ));
		if( vsign < 0 ) match = -match;
	}

	/* Ordering rule used in extensible match filter? */
	if ( (flags & SLAP_MR_EXT) && (mr->smr_usage & SLAP_MR_ORDERING) )
		match = (match >= 0);

	*matchp = match;
	return LDAP_SUCCESS;
}

/* 10**Chop < 256**Chopbytes and Chop > Chopbytes<<1 (for sign bit and itmp) */
#define INDEX_INTLEN_CHOP 7
#define INDEX_INTLEN_CHOPBYTES 3

static int
integerVal2Key(
	struct berval *in,
	struct berval *key,
	struct berval *tmp,
	void *ctx )
{
	/* Integer index key format, designed for memcmp to collate correctly:
	 * if too large: one's complement sign*<approx exponent=chopped bytes>,
	 * two's complement value (sign-extended or chopped as needed),
	 * however in first byte above, the top <number of exponent-bytes + 1>
	 * bits are the inverse sign and next bit is the sign as delimiter.
	 */
	ber_slen_t k = index_intlen_strlen;
	ber_len_t chop = 0;
	unsigned signmask = ~0x7fU;
	unsigned char lenbuf[sizeof(k) + 2], *lenp, neg = 0xff;
	struct berval val = *in, itmp = *tmp;

	if ( val.bv_val[0] != '-' ) {
		neg = 0;
		--k;
	}

	/* Chop least significant digits, increase length instead */
	if ( val.bv_len > (ber_len_t) k ) {
		chop = (val.bv_len-k+2)/INDEX_INTLEN_CHOP; /* 2 fewer digits */
		val.bv_len -= chop * INDEX_INTLEN_CHOP;	/* #digits chopped */
		chop *= INDEX_INTLEN_CHOPBYTES;		/* #bytes added */
	}

	if ( lutil_str2bin( &val, &itmp, ctx )) {
		return LDAP_INVALID_SYNTAX;
	}

	/* Omit leading sign byte */
	if ( itmp.bv_val[0] == neg ) {
		itmp.bv_val++;
		itmp.bv_len--;
	}

	k = (ber_slen_t) index_intlen - (ber_slen_t) (itmp.bv_len + chop);
	if ( k > 0 ) {
		assert( chop == 0 );
		memset( key->bv_val, neg, k );	/* sign-extend */
	} else if ( k != 0 || ((itmp.bv_val[0] ^ neg) & 0xc0) ) {
		/* Got exponent -k, or no room for 2 sign bits */
		lenp = lenbuf + sizeof(lenbuf);
		chop = - (ber_len_t) k;
		do {
			*--lenp = ((unsigned char) chop & 0xff) ^ neg;
			signmask >>= 1;
		} while ( (chop >>= 8) != 0 || (signmask >> 1) & (*lenp ^ neg) );
		/* With n bytes in lenbuf, the top n+1 bits of (signmask&0xff)
		 * are 1, and the top n+2 bits of lenp[0] are the sign bit. */
		k = (lenbuf + sizeof(lenbuf)) - lenp;
		if ( k > (ber_slen_t) index_intlen )
			k = index_intlen;
		memcpy( key->bv_val, lenp, k );
		itmp.bv_len = index_intlen - k;
	}
	memcpy( key->bv_val + k, itmp.bv_val, itmp.bv_len );
	key->bv_val[0] ^= (unsigned char) signmask & 0xff; /* invert sign */
	return 0;
}

/* Index generation function: Ordered index */
static int
integerIndexer(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	BerVarray values,
	BerVarray *keysp,
	void *ctx )
{
	char ibuf[64];
	struct berval itmp;
	BerVarray keys;
	ber_len_t vlen;
	int i, rc;
	unsigned maxstrlen = index_intlen_strlen + INDEX_INTLEN_CHOP-1;

	/* count the values and find max needed length */
	vlen = 0;
	for( i = 0; !BER_BVISNULL( &values[i] ); i++ ) {
		if ( vlen < values[i].bv_len )
			vlen = values[i].bv_len;
	}
	if ( vlen > maxstrlen )
		vlen = maxstrlen;

	/* we should have at least one value at this point */
	assert( i > 0 );

	keys = slap_sl_malloc( sizeof( struct berval ) * (i+1), ctx );
	for ( i = 0; !BER_BVISNULL( &values[i] ); i++ ) {
		keys[i].bv_len = index_intlen;
		keys[i].bv_val = slap_sl_malloc( index_intlen, ctx );
	}
	keys[i].bv_len = 0;
	keys[i].bv_val = NULL;

	if ( vlen > sizeof(ibuf) ) {
		itmp.bv_val = slap_sl_malloc( vlen, ctx );
	} else {
		itmp.bv_val = ibuf;
	}
	itmp.bv_len = sizeof(ibuf);

	for ( i=0; !BER_BVISNULL( &values[i] ); i++ ) {
		if ( itmp.bv_val != ibuf ) {
			itmp.bv_len = values[i].bv_len;
			if ( itmp.bv_len <= sizeof(ibuf) )
				itmp.bv_len = sizeof(ibuf);
			else if ( itmp.bv_len > maxstrlen )
				itmp.bv_len = maxstrlen;
		}
		rc = integerVal2Key( &values[i], &keys[i], &itmp, ctx );
		if ( rc )
			goto func_leave;
	}
	*keysp = keys;
func_leave:
	if ( itmp.bv_val != ibuf ) {
		slap_sl_free( itmp.bv_val, ctx );
	}
	return rc;
}

/* Index generation function: Ordered index */
static int
integerFilter(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	void * assertedValue,
	BerVarray *keysp,
	void *ctx )
{
	char ibuf[64];
	struct berval iv;
	BerVarray keys;
	struct berval *value;
	int rc;

	value = (struct berval *) assertedValue;

	keys = slap_sl_malloc( sizeof( struct berval ) * 2, ctx );

	keys[0].bv_len = index_intlen;
	keys[0].bv_val = slap_sl_malloc( index_intlen, ctx );
	keys[1].bv_len = 0;
	keys[1].bv_val = NULL;

	iv.bv_len = value->bv_len < index_intlen_strlen + INDEX_INTLEN_CHOP-1
		? value->bv_len : index_intlen_strlen + INDEX_INTLEN_CHOP-1;
	if ( iv.bv_len > (int) sizeof(ibuf) ) {
		iv.bv_val = slap_sl_malloc( iv.bv_len, ctx );
	} else {
		iv.bv_val = ibuf;
		iv.bv_len = sizeof(ibuf);
	}

	rc = integerVal2Key( value, keys, &iv, ctx );
	if ( rc == 0 )
		*keysp = keys;

	if ( iv.bv_val != ibuf ) {
		slap_sl_free( iv.bv_val, ctx );
	}
	return rc;
}

static int
countryStringValidate(
	Syntax *syntax,
	struct berval *val )
{
	if( val->bv_len != 2 ) return LDAP_INVALID_SYNTAX;

	if( !SLAP_PRINTABLE(val->bv_val[0]) ) {
		return LDAP_INVALID_SYNTAX;
	}
	if( !SLAP_PRINTABLE(val->bv_val[1]) ) {
		return LDAP_INVALID_SYNTAX;
	}

	return LDAP_SUCCESS;
}

static int
printableStringValidate(
	Syntax *syntax,
	struct berval *val )
{
	ber_len_t i;

	if( BER_BVISEMPTY( val ) ) return LDAP_INVALID_SYNTAX;

	for(i=0; i < val->bv_len; i++) {
		if( !SLAP_PRINTABLE(val->bv_val[i]) ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	return LDAP_SUCCESS;
}

static int
printablesStringValidate(
	Syntax *syntax,
	struct berval *val )
{
	ber_len_t i, len;

	if( BER_BVISEMPTY( val ) ) return LDAP_INVALID_SYNTAX;

	for(i=0,len=0; i < val->bv_len; i++) {
		int c = val->bv_val[i];

		if( c == '$' ) {
			if( len == 0 ) {
				return LDAP_INVALID_SYNTAX;
			}
			len = 0;

		} else if ( SLAP_PRINTABLE(c) ) {
			len++;
		} else {
			return LDAP_INVALID_SYNTAX;
		}
	}

	if( len == 0 ) {
		return LDAP_INVALID_SYNTAX;
	}

	return LDAP_SUCCESS;
}

static int
IA5StringValidate(
	Syntax *syntax,
	struct berval *val )
{
	ber_len_t i;

	for(i=0; i < val->bv_len; i++) {
		if( !LDAP_ASCII(val->bv_val[i]) ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	return LDAP_SUCCESS;
}

static int
IA5StringNormalize(
	slap_mask_t use,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	char *p, *q;
	int casefold = !SLAP_MR_ASSOCIATED( mr,
		slap_schema.si_mr_caseExactIA5Match );

	assert( SLAP_MR_IS_VALUE_OF_SYNTAX( use ) != 0 );

	p = val->bv_val;

	/* Ignore initial whitespace */
	while ( ASCII_SPACE( *p ) ) p++;

	normalized->bv_len = val->bv_len - ( p - val->bv_val );
	normalized->bv_val = slap_sl_malloc( normalized->bv_len + 1, ctx );
	AC_MEMCPY( normalized->bv_val, p, normalized->bv_len );
	normalized->bv_val[normalized->bv_len] = '\0';

	p = q = normalized->bv_val;

	while ( *p ) {
		if ( ASCII_SPACE( *p ) ) {
			*q++ = *p++;

			/* Ignore the extra whitespace */
			while ( ASCII_SPACE( *p ) ) {
				p++;
			}

		} else if ( casefold ) {
			/* Most IA5 rules require casefolding */
			*q++ = TOLOWER(*p); p++;

		} else {
			*q++ = *p++;
		}
	}

	assert( normalized->bv_val <= p );
	assert( q <= p );

	/*
	 * If the string ended in space, backup the pointer one
	 * position.  One is enough because the above loop collapsed
	 * all whitespace to a single space.
	 */
	if ( q > normalized->bv_val && ASCII_SPACE( q[-1] ) ) --q;

	/* null terminate */
	*q = '\0';

	normalized->bv_len = q - normalized->bv_val;

	return LDAP_SUCCESS;
}

static int
UUIDValidate(
	Syntax *syntax,
	struct berval *in )
{
	int i;
	if( in->bv_len != 36 ) {
		return LDAP_INVALID_SYNTAX;
	}

	for( i=0; i<36; i++ ) {
		switch(i) {
			case 8:
			case 13:
			case 18:
			case 23:
				if( in->bv_val[i] != '-' ) {
					return LDAP_INVALID_SYNTAX;
				}
				break;
			default:
				if( !ASCII_HEX( in->bv_val[i]) ) {
					return LDAP_INVALID_SYNTAX;
				}
		}
	}
	
	return LDAP_SUCCESS;
}

static int
UUIDPretty(
	Syntax *syntax,
	struct berval *in,
	struct berval *out,
	void *ctx )
{
	int i;
	int rc=LDAP_INVALID_SYNTAX;

	assert( in != NULL );
	assert( out != NULL );

	if( in->bv_len != 36 ) return LDAP_INVALID_SYNTAX;

	out->bv_len = 36;
	out->bv_val = slap_sl_malloc( out->bv_len + 1, ctx );

	for( i=0; i<36; i++ ) {
		switch(i) {
			case 8:
			case 13:
			case 18:
			case 23:
				if( in->bv_val[i] != '-' ) {
					goto handle_error;
				}
				out->bv_val[i] = '-';
				break;

			default:
				if( !ASCII_HEX( in->bv_val[i]) ) {
					goto handle_error;
				}
				out->bv_val[i] = TOLOWER( in->bv_val[i] );
		}
	}

	rc = LDAP_SUCCESS;
	out->bv_val[ out->bv_len ] = '\0';

	if( 0 ) {
handle_error:
		slap_sl_free( out->bv_val, ctx );
		out->bv_val = NULL;
	}

	return rc;
}

int
UUIDNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	unsigned char octet = '\0';
	int i;
	int j;

	if ( SLAP_MR_IS_DENORMALIZE( usage ) ) {
		/* NOTE: must be a normalized UUID */
		assert( val->bv_len == 16 );

		normalized->bv_val = slap_sl_malloc( LDAP_LUTIL_UUIDSTR_BUFSIZE, ctx );
		normalized->bv_len = lutil_uuidstr_from_normalized( val->bv_val,
			val->bv_len, normalized->bv_val, LDAP_LUTIL_UUIDSTR_BUFSIZE );
		assert( normalized->bv_len == STRLENOF( "BADBADBA-DBAD-0123-4567-BADBADBADBAD" ) );

		return LDAP_SUCCESS;
	}

	normalized->bv_len = 16;
	normalized->bv_val = slap_sl_malloc( normalized->bv_len + 1, ctx );

	for( i=0, j=0; i<36; i++ ) {
		unsigned char nibble;
		if( val->bv_val[i] == '-' ) {
			continue;

		} else if( ASCII_DIGIT( val->bv_val[i] ) ) {
			nibble = val->bv_val[i] - '0';

		} else if( ASCII_HEXLOWER( val->bv_val[i] ) ) {
			nibble = val->bv_val[i] - ('a'-10);

		} else if( ASCII_HEXUPPER( val->bv_val[i] ) ) {
			nibble = val->bv_val[i] - ('A'-10);

		} else {
			slap_sl_free( normalized->bv_val, ctx );
			BER_BVZERO( normalized );
			return LDAP_INVALID_SYNTAX;
		}

		if( j & 1 ) {
			octet |= nibble;
			normalized->bv_val[j>>1] = octet;
		} else {
			octet = nibble << 4;
		}
		j++;
	}

	normalized->bv_val[normalized->bv_len] = 0;
	return LDAP_SUCCESS;
}



int
numericStringValidate(
	Syntax *syntax,
	struct berval *in )
{
	ber_len_t i;

	if( BER_BVISEMPTY( in ) ) return LDAP_INVALID_SYNTAX;

	for(i=0; i < in->bv_len; i++) {
		if( !SLAP_NUMERIC(in->bv_val[i]) ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	return LDAP_SUCCESS;
}

static int
numericStringNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	/* removal all spaces */
	char *p, *q;

	assert( !BER_BVISEMPTY( val ) );

	normalized->bv_val = slap_sl_malloc( val->bv_len + 1, ctx );

	p = val->bv_val;
	q = normalized->bv_val;

	while ( *p ) {
		if ( ASCII_SPACE( *p ) ) {
			/* Ignore whitespace */
			p++;
		} else {
			*q++ = *p++;
		}
	}

	/* we should have copied no more than is in val */
	assert( (q - normalized->bv_val) <= (p - val->bv_val) );

	/* null terminate */
	*q = '\0';

	normalized->bv_len = q - normalized->bv_val;

	if( BER_BVISEMPTY( normalized ) ) {
		normalized->bv_val = slap_sl_realloc( normalized->bv_val, 2, ctx );
		normalized->bv_val[0] = ' ';
		normalized->bv_val[1] = '\0';
		normalized->bv_len = 1;
	}

	return LDAP_SUCCESS;
}

/*
 * Integer conversion macros that will use the largest available
 * type.
 */
#if defined(HAVE_STRTOLL) && defined(HAVE_LONG_LONG)
# define SLAP_STRTOL(n,e,b)  strtoll(n,e,b) 
# define SLAP_LONG           long long
#else
# define SLAP_STRTOL(n,e,b)  strtol(n,e,b)
# define SLAP_LONG           long
#endif /* HAVE_STRTOLL ... */

static int
integerBitAndMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	SLAP_LONG lValue, lAssertedValue;

	errno = 0;
	/* safe to assume integers are NUL terminated? */
	lValue = SLAP_STRTOL(value->bv_val, NULL, 10);
	if( errno == ERANGE )
	{
		return LDAP_CONSTRAINT_VIOLATION;
	}

	lAssertedValue = SLAP_STRTOL(((struct berval *)assertedValue)->bv_val,
		NULL, 10);
	if( errno == ERANGE )
	{
		return LDAP_CONSTRAINT_VIOLATION;
	}

	*matchp = ((lValue & lAssertedValue) == lAssertedValue) ? 0 : 1;
	return LDAP_SUCCESS;
}

static int
integerBitOrMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	SLAP_LONG lValue, lAssertedValue;

	errno = 0;
	/* safe to assume integers are NUL terminated? */
	lValue = SLAP_STRTOL(value->bv_val, NULL, 10);
	if( errno == ERANGE )
	{
		return LDAP_CONSTRAINT_VIOLATION;
	}

	lAssertedValue = SLAP_STRTOL( ((struct berval *)assertedValue)->bv_val,
		NULL, 10);
	if( errno == ERANGE )
	{
		return LDAP_CONSTRAINT_VIOLATION;
	}

	*matchp = ((lValue & lAssertedValue) != 0) ? 0 : -1;
	return LDAP_SUCCESS;
}

static int
checkNum( struct berval *in, struct berval *out )
{
	/* parse serialNumber */
	ber_len_t neg = 0, extra = 0;
	char first = '\0';

	out->bv_val = in->bv_val;
	out->bv_len = 0;

	if ( out->bv_val[0] == '-' ) {
		neg++;
		out->bv_len++;
	}

	if ( strncasecmp( out->bv_val, "0x", STRLENOF("0x") ) == 0 ) {
		first = out->bv_val[2];
		extra = 2;

		out->bv_len += STRLENOF("0x");
		for ( ; out->bv_len < in->bv_len; out->bv_len++ ) {
			if ( !ASCII_HEX( out->bv_val[out->bv_len] ) ) break;
		}

	} else if ( out->bv_val[0] == '\'' ) {
		first = out->bv_val[1];
		extra = 3;

		out->bv_len += STRLENOF("'");

		for ( ; out->bv_len < in->bv_len; out->bv_len++ ) {
			if ( !ASCII_HEX( out->bv_val[out->bv_len] ) ) break;
		}
		if ( strncmp( &out->bv_val[out->bv_len], "'H", STRLENOF("'H") ) != 0 ) {
			return -1;
		}
		out->bv_len += STRLENOF("'H");

	} else {
		first = out->bv_val[0];
		for ( ; out->bv_len < in->bv_len; out->bv_len++ ) {
			if ( !ASCII_DIGIT( out->bv_val[out->bv_len] ) ) break;
		}
	}

	if ( !( out->bv_len > neg ) ) {
		return -1;
	}

	if ( ( out->bv_len > extra + 1 + neg ) && ( first == '0' ) ) {
		return -1;
	}

	return 0;
}

static int
serialNumberAndIssuerCheck(
	struct berval *in,
	struct berval *sn,
	struct berval *is,
	void *ctx )
{
	ber_len_t n;

	if( in->bv_len < 3 ) return LDAP_INVALID_SYNTAX;

	if( in->bv_val[0] != '{' && in->bv_val[in->bv_len-1] != '}' ) {
		/* Parse old format */
		is->bv_val = ber_bvchr( in, '$' );
		if( BER_BVISNULL( is ) ) return LDAP_INVALID_SYNTAX;

		sn->bv_val = in->bv_val;
		sn->bv_len = is->bv_val - in->bv_val;

		is->bv_val++;
		is->bv_len = in->bv_len - (sn->bv_len + 1);

		/* eat leading zeros */
		for( n=0; n < (sn->bv_len-1); n++ ) {
			if( sn->bv_val[n] != '0' ) break;
		}
		sn->bv_val += n;
		sn->bv_len -= n;

		for( n=0; n < sn->bv_len; n++ ) {
			if( !ASCII_DIGIT(sn->bv_val[n]) ) return LDAP_INVALID_SYNTAX;
		}

	} else {
		/* Parse GSER format */ 
		enum {
			HAVE_NONE = 0x0,
			HAVE_ISSUER = 0x1,
			HAVE_SN = 0x2,
			HAVE_ALL = ( HAVE_ISSUER | HAVE_SN )
		} have = HAVE_NONE;

		int numdquotes = 0;
		struct berval x = *in;
		struct berval ni;
		x.bv_val++;
		x.bv_len -= 2;

		do {
			/* eat leading spaces */
			for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
				/* empty */;
			}

			/* should be at issuer or serialNumber NamedValue */
			if ( strncasecmp( x.bv_val, "issuer", STRLENOF("issuer") ) == 0 ) {
				if ( have & HAVE_ISSUER ) return LDAP_INVALID_SYNTAX;

				/* parse issuer */
				x.bv_val += STRLENOF("issuer");
				x.bv_len -= STRLENOF("issuer");

				if ( x.bv_val[0] != ' ' ) return LDAP_INVALID_SYNTAX;
				x.bv_val++;
				x.bv_len--;

				/* eat leading spaces */
				for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
					/* empty */;
				}

				/* For backward compatibility, this part is optional */
				if ( strncasecmp( x.bv_val, "rdnSequence:", STRLENOF("rdnSequence:") ) == 0 ) {
					x.bv_val += STRLENOF("rdnSequence:");
					x.bv_len -= STRLENOF("rdnSequence:");
				}

				if ( x.bv_val[0] != '"' ) return LDAP_INVALID_SYNTAX;
				x.bv_val++;
				x.bv_len--;

				is->bv_val = x.bv_val;
				is->bv_len = 0;

				for ( ; is->bv_len < x.bv_len; ) {
					if ( is->bv_val[is->bv_len] != '"' ) {
						is->bv_len++;
						continue;
					}
					if ( is->bv_val[is->bv_len+1] == '"' ) {
						/* double dquote */
						is->bv_len += 2;
						continue;
					}
					break;
				}
				x.bv_val += is->bv_len + 1;
				x.bv_len -= is->bv_len + 1;

				have |= HAVE_ISSUER;

			} else if ( strncasecmp( x.bv_val, "serialNumber", STRLENOF("serialNumber") ) == 0 )
			{
				if ( have & HAVE_SN ) return LDAP_INVALID_SYNTAX;

				/* parse serialNumber */
				x.bv_val += STRLENOF("serialNumber");
				x.bv_len -= STRLENOF("serialNumber");

				if ( x.bv_val[0] != ' ' ) return LDAP_INVALID_SYNTAX;
				x.bv_val++;
				x.bv_len--;

				/* eat leading spaces */
				for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
					/* empty */;
				}

				if ( checkNum( &x, sn ) ) {
					return LDAP_INVALID_SYNTAX;
				}

				x.bv_val += sn->bv_len;
				x.bv_len -= sn->bv_len;

				have |= HAVE_SN;

			} else {
				return LDAP_INVALID_SYNTAX;
			}

			/* eat leading spaces */
			for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
				/* empty */;
			}

			if ( have == HAVE_ALL ) {
				break;
			}

			if ( x.bv_val[0] != ',' ) {
				return LDAP_INVALID_SYNTAX;
			}

			x.bv_val++;
			x.bv_len--;
		} while ( 1 );

		/* should have no characters left... */
		if ( x.bv_len ) return LDAP_INVALID_SYNTAX;

		if ( numdquotes == 0 ) {
			ber_dupbv_x( &ni, is, ctx );

		} else {
			ber_len_t src, dst;

			ni.bv_len = is->bv_len - numdquotes;
			ni.bv_val = ber_memalloc_x( ni.bv_len + 1, ctx );
			for ( src = 0, dst = 0; src < is->bv_len; src++, dst++ ) {
				if ( is->bv_val[src] == '"' ) {
					src++;
				}
				ni.bv_val[dst] = is->bv_val[src];
			}
			ni.bv_val[dst] = '\0';
		}
			
		*is = ni;
	}

	return 0;
}
	
static int
serialNumberAndIssuerValidate(
	Syntax *syntax,
	struct berval *in )
{
	int rc;
	struct berval sn, i;

	Debug( LDAP_DEBUG_TRACE, ">>> serialNumberAndIssuerValidate: <%s>\n",
		in->bv_val, 0, 0 );

	rc = serialNumberAndIssuerCheck( in, &sn, &i, NULL );
	if ( rc ) {
		goto done;
	}

	/* validate DN -- doesn't handle double dquote */ 
	rc = dnValidate( NULL, &i );
	if ( rc ) {
		rc = LDAP_INVALID_SYNTAX;
	}

	if ( in->bv_val[0] == '{' && in->bv_val[in->bv_len-1] == '}' ) {
		slap_sl_free( i.bv_val, NULL );
	}

	Debug( LDAP_DEBUG_TRACE, "<<< serialNumberAndIssuerValidate: <%s> err=%d\n",
		in->bv_val, rc, 0 );

done:;
	return rc;
}

static int
serialNumberAndIssuerPretty(
	Syntax *syntax,
	struct berval *in,
	struct berval *out,
	void *ctx )
{
	int rc;
	struct berval sn, i, ni = BER_BVNULL;
	char *p;

	assert( in != NULL );
	assert( out != NULL );

	BER_BVZERO( out );

	Debug( LDAP_DEBUG_TRACE, ">>> serialNumberAndIssuerPretty: <%s>\n",
		in->bv_val, 0, 0 );

	rc = serialNumberAndIssuerCheck( in, &sn, &i, ctx );
	if ( rc ) {
		goto done;
	}

	rc = dnPretty( syntax, &i, &ni, ctx );

	if ( in->bv_val[0] == '{' && in->bv_val[in->bv_len-1] == '}' ) {
		slap_sl_free( i.bv_val, ctx );
	}

	if ( rc ) {
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}

	/* make room from sn + "$" */
	out->bv_len = STRLENOF("{ serialNumber , issuer rdnSequence:\"\" }")
		+ sn.bv_len + ni.bv_len;
	out->bv_val = slap_sl_malloc( out->bv_len + 1, ctx );

	if ( out->bv_val == NULL ) {
		out->bv_len = 0;
		rc = LDAP_OTHER;
		goto done;
	}

	p = out->bv_val;
	p = lutil_strcopy( p, "{ serialNumber " /*}*/ );
	p = lutil_strbvcopy( p, &sn );
	p = lutil_strcopy( p, ", issuer rdnSequence:\"" );
	p = lutil_strbvcopy( p, &ni );
	p = lutil_strcopy( p, /*{*/ "\" }" );

	assert( p == &out->bv_val[out->bv_len] );

done:;
	Debug( LDAP_DEBUG_TRACE, "<<< serialNumberAndIssuerPretty: <%s> => <%s>\n",
		in->bv_val, rc == LDAP_SUCCESS ? out->bv_val : "(err)", 0 );

	slap_sl_free( ni.bv_val, ctx );

	return LDAP_SUCCESS; 
}

static int
slap_bin2hex(
	struct berval *in,
	struct berval *out,
	void *ctx )

{	
	/* Use hex format. '123456789abcdef'H */
	unsigned char *ptr, zero = '\0';
	char *sptr;
	int first;
	ber_len_t i, len, nlen;

	assert( in != NULL );
	assert( !BER_BVISNULL( in ) );
	assert( out != NULL );
	assert( !BER_BVISNULL( out ) );

	ptr = (unsigned char *)in->bv_val;
	len = in->bv_len;

	/* Check for minimal encodings */
	if ( len > 1 ) {
		if ( ptr[0] & 0x80 ) {
			if ( ( ptr[0] == 0xff ) && ( ptr[1] & 0x80 ) ) {
				return -1;
			}

		} else if ( ptr[0] == 0 ) {
			if ( !( ptr[1] & 0x80 ) ) {
				return -1;
			}
			len--;
			ptr++;
		}

	} else if ( len == 0 ) {
		/* FIXME: this should not be possible,
		 * since a value of zero would have length 1 */
		len = 1;
		ptr = &zero;
	}

	first = !( ptr[0] & 0xf0U );
	nlen = len * 2 - first + STRLENOF("''H"); /* quotes, H */
	if ( nlen >= out->bv_len ) {
		out->bv_val = slap_sl_malloc( nlen + 1, ctx );
	}
	sptr = out->bv_val;
	*sptr++ = '\'';
	i = 0;
	if ( first ) {
		sprintf( sptr, "%01X", ( ptr[0] & 0x0fU ) );
		sptr++;
		i = 1;
	}
	for ( ; i < len; i++ ) {
		sprintf( sptr, "%02X", ptr[i] );
		sptr += 2;
	}
	*sptr++ = '\'';
	*sptr++ = 'H';
	*sptr = '\0';

	assert( sptr == &out->bv_val[nlen] );

	out->bv_len = nlen;

	return 0;
}

#define SLAP_SN_BUFLEN	(64)

/*
 * This routine is called by certificateExactNormalize when
 * certificateExactNormalize receives a search string instead of
 * a certificate. This routine checks if the search value is valid
 * and then returns the normalized value
 */
static int
serialNumberAndIssuerNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *in,
	struct berval *out,
	void *ctx )
{
	struct berval sn, sn2, sn3, i, ni;
	char sbuf2[SLAP_SN_BUFLEN];
	char sbuf3[SLAP_SN_BUFLEN];
	char *p;
	int rc;

	assert( in != NULL );
	assert( out != NULL );

	Debug( LDAP_DEBUG_TRACE, ">>> serialNumberAndIssuerNormalize: <%s>\n",
		in->bv_val, 0, 0 );

	rc = serialNumberAndIssuerCheck( in, &sn, &i, ctx );
	if ( rc ) {
		return rc;
	}

	rc = dnNormalize( usage, syntax, mr, &i, &ni, ctx );

	if ( in->bv_val[0] == '{' && in->bv_val[in->bv_len-1] == '}' ) {
		slap_sl_free( i.bv_val, ctx );
	}

	if ( rc ) {
		return LDAP_INVALID_SYNTAX;
	}

	/* Convert sn to canonical hex */
	sn2.bv_val = sbuf2;
	if ( sn.bv_len > sizeof( sbuf2 ) ) {
		sn2.bv_val = slap_sl_malloc( sn.bv_len, ctx );
	}
	sn2.bv_len = sn.bv_len;
	sn3.bv_val = sbuf3;
	sn3.bv_len = sizeof(sbuf3);
	if ( lutil_str2bin( &sn, &sn2, ctx ) || slap_bin2hex( &sn2, &sn3, ctx ) ) {
		rc = LDAP_INVALID_SYNTAX;
		goto func_leave;
	}

	out->bv_len = STRLENOF( "{ serialNumber , issuer rdnSequence:\"\" }" )
		+ sn3.bv_len + ni.bv_len;
	out->bv_val = slap_sl_malloc( out->bv_len + 1, ctx );
	if ( out->bv_val == NULL ) {
		out->bv_len = 0;
		rc = LDAP_OTHER;
		goto func_leave;
	}

	p = out->bv_val;

	p = lutil_strcopy( p, "{ serialNumber " /*}*/ );
	p = lutil_strbvcopy( p, &sn3 );
	p = lutil_strcopy( p, ", issuer rdnSequence:\"" );
	p = lutil_strbvcopy( p, &ni );
	p = lutil_strcopy( p, /*{*/ "\" }" );

	assert( p == &out->bv_val[out->bv_len] );

func_leave:
	Debug( LDAP_DEBUG_TRACE, "<<< serialNumberAndIssuerNormalize: <%s> => <%s>\n",
		in->bv_val, rc == LDAP_SUCCESS ? out->bv_val : "(err)", 0 );

	if ( sn2.bv_val != sbuf2 ) {
		slap_sl_free( sn2.bv_val, ctx );
	}

	if ( sn3.bv_val != sbuf3 ) {
		slap_sl_free( sn3.bv_val, ctx );
	}

	slap_sl_free( ni.bv_val, ctx );

	return rc;
}

static int
certificateExactNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len;
	ber_int_t i;
	char serialbuf2[SLAP_SN_BUFLEN];
	struct berval sn, sn2 = BER_BVNULL;
	struct berval issuer_dn = BER_BVNULL, bvdn;
	char *p;
	int rc = LDAP_INVALID_SYNTAX;

	assert( val != NULL );

	Debug( LDAP_DEBUG_TRACE, ">>> certificateExactNormalize: <%p, %lu>\n",
		val->bv_val, val->bv_len, 0 );

	if ( BER_BVISEMPTY( val ) ) goto done;

	if ( SLAP_MR_IS_VALUE_OF_ASSERTION_SYNTAX(usage) ) {
		return serialNumberAndIssuerNormalize( 0, NULL, NULL, val, normalized, ctx );
	}

	assert( SLAP_MR_IS_VALUE_OF_ATTRIBUTE_SYNTAX(usage) != 0 );

	ber_init2( ber, val, LBER_USE_DER );
	tag = ber_skip_tag( ber, &len );	/* Signed Sequence */
	tag = ber_skip_tag( ber, &len );	/* Sequence */
	tag = ber_peek_tag( ber, &len );	/* Optional version? */
	if ( tag == SLAP_X509_OPT_C_VERSION ) {
		tag = ber_skip_tag( ber, &len );
		tag = ber_get_int( ber, &i );	/* version */
	}

	/* NOTE: move the test here from certificateValidate,
	 * so that we can validate certs with serial longer
	 * than sizeof(ber_int_t) */
	tag = ber_skip_tag( ber, &len );	/* serial */
	sn.bv_len = len;
	sn.bv_val = (char *)ber->ber_ptr;
	sn2.bv_val = serialbuf2;
	sn2.bv_len = sizeof(serialbuf2);
	if ( slap_bin2hex( &sn, &sn2, ctx ) ) {
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}
	ber_skip_data( ber, len );

	tag = ber_skip_tag( ber, &len );	/* SignatureAlg */
	ber_skip_data( ber, len );
	tag = ber_peek_tag( ber, &len );	/* IssuerDN */
	if ( len ) {
		len = ber_ptrlen( ber );
		bvdn.bv_val = val->bv_val + len;
		bvdn.bv_len = val->bv_len - len;

		rc = dnX509normalize( &bvdn, &issuer_dn );
		if ( rc != LDAP_SUCCESS ) goto done;
	}

	normalized->bv_len = STRLENOF( "{ serialNumber , issuer rdnSequence:\"\" }" )
		+ sn2.bv_len + issuer_dn.bv_len;
	normalized->bv_val = ch_malloc( normalized->bv_len + 1 );

	p = normalized->bv_val;

	p = lutil_strcopy( p, "{ serialNumber " /*}*/ );
	p = lutil_strbvcopy( p, &sn2 );
	p = lutil_strcopy( p, ", issuer rdnSequence:\"" );
	p = lutil_strbvcopy( p, &issuer_dn );
	p = lutil_strcopy( p, /*{*/ "\" }" );

	rc = LDAP_SUCCESS;

done:
	Debug( LDAP_DEBUG_TRACE, "<<< certificateExactNormalize: <%p, %lu> => <%s>\n",
		val->bv_val, val->bv_len, rc == LDAP_SUCCESS ? normalized->bv_val : "(err)" );

	if ( issuer_dn.bv_val ) ber_memfree( issuer_dn.bv_val );
	if ( sn2.bv_val != serialbuf2 ) ber_memfree_x( sn2.bv_val, ctx );

	return rc;
}

/* X.509 PKI certificateList stuff */
static int
checkTime( struct berval *in, struct berval *out )
{
	int rc;
	ber_len_t i;
	char buf[STRLENOF("YYYYmmddHHMMSSZ") + 1];
	struct berval bv;

	assert( in != NULL );
	assert( !BER_BVISNULL( in ) );
	assert( !BER_BVISEMPTY( in ) );

	if ( in->bv_len < STRLENOF( "YYmmddHHMMSSZ" ) ) {
		return -1;
	}

	if ( out != NULL ) {
		assert( !BER_BVISNULL( out ) );
		assert( out->bv_len >= sizeof( buf ) );
		bv.bv_val = out->bv_val;

	} else {
		bv.bv_val = buf;
	}

	for ( i = 0; i < STRLENOF( "YYYYmmddHHMMSS" ); i++ ) {
		if ( !ASCII_DIGIT( in->bv_val[i] ) ) break;
	}

	if ( in->bv_val[i] != 'Z' ) {
		return -1;
	}
	i++;

	if ( i != in->bv_len ) {
		return -1;
	}

	if ( i == STRLENOF( "YYYYmmddHHMMSSZ" ) ) {
		lutil_strncopy( bv.bv_val, in->bv_val, i );
		bv.bv_len = i;
		
	} else if ( i == STRLENOF( "YYmmddHHMMSSZ" ) ) {
		char *p = bv.bv_val;
		if ( in->bv_val[0] < '7' ) {
			p = lutil_strcopy( p, "20" );

		} else {
			p = lutil_strcopy( p, "19" );
		}
		lutil_strncopy( p, in->bv_val, i );
		bv.bv_len = 2 + i;

	} else {
		return -1;
	}

	rc = generalizedTimeValidate( NULL, &bv );
	if ( rc == LDAP_SUCCESS && out != NULL ) {
		if ( out->bv_len > bv.bv_len ) {
			out->bv_val[ bv.bv_len ] = '\0';
		}
		out->bv_len = bv.bv_len;
	}

	return rc != LDAP_SUCCESS;
}

static int
issuerAndThisUpdateCheck(
	struct berval *in,
	struct berval *is,
	struct berval *tu,
	void *ctx )
{
	int numdquotes = 0;
	struct berval x = *in;
	struct berval ni = BER_BVNULL;
	/* Parse GSER format */ 
	enum {
		HAVE_NONE = 0x0,
		HAVE_ISSUER = 0x1,
		HAVE_THISUPDATE = 0x2,
		HAVE_ALL = ( HAVE_ISSUER | HAVE_THISUPDATE )
	} have = HAVE_NONE;


	if ( in->bv_len < STRLENOF( "{issuer \"\",thisUpdate \"YYMMDDhhmmssZ\"}" ) ) return LDAP_INVALID_SYNTAX;

	if ( in->bv_val[0] != '{' && in->bv_val[in->bv_len-1] != '}' ) {
		return LDAP_INVALID_SYNTAX;
	}

	x.bv_val++;
	x.bv_len -= STRLENOF("{}");

	do {
		/* eat leading spaces */
		for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
			/* empty */;
		}

		/* should be at issuer or thisUpdate */
		if ( strncasecmp( x.bv_val, "issuer", STRLENOF("issuer") ) == 0 ) {
			if ( have & HAVE_ISSUER ) return LDAP_INVALID_SYNTAX;

			/* parse issuer */
			x.bv_val += STRLENOF("issuer");
			x.bv_len -= STRLENOF("issuer");

			if ( x.bv_val[0] != ' ' ) return LDAP_INVALID_SYNTAX;
			x.bv_val++;
			x.bv_len--;

			/* eat leading spaces */
			for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
				/* empty */;
			}

			/* For backward compatibility, this part is optional */
			if ( strncasecmp( x.bv_val, "rdnSequence:", STRLENOF("rdnSequence:") ) != 0 ) {
				return LDAP_INVALID_SYNTAX;
			}
			x.bv_val += STRLENOF("rdnSequence:");
			x.bv_len -= STRLENOF("rdnSequence:");

			if ( x.bv_val[0] != '"' ) return LDAP_INVALID_SYNTAX;
			x.bv_val++;
			x.bv_len--;

			is->bv_val = x.bv_val;
			is->bv_len = 0;

			for ( ; is->bv_len < x.bv_len; ) {
				if ( is->bv_val[is->bv_len] != '"' ) {
					is->bv_len++;
					continue;
				}
				if ( is->bv_val[is->bv_len+1] == '"' ) {
					/* double dquote */
					is->bv_len += 2;
					continue;
				}
				break;
			}
			x.bv_val += is->bv_len + 1;
			x.bv_len -= is->bv_len + 1;

			have |= HAVE_ISSUER;

		} else if ( strncasecmp( x.bv_val, "thisUpdate", STRLENOF("thisUpdate") ) == 0 )
		{
			if ( have & HAVE_THISUPDATE ) return LDAP_INVALID_SYNTAX;

			/* parse thisUpdate */
			x.bv_val += STRLENOF("thisUpdate");
			x.bv_len -= STRLENOF("thisUpdate");

			if ( x.bv_val[0] != ' ' ) return LDAP_INVALID_SYNTAX;
			x.bv_val++;
			x.bv_len--;

			/* eat leading spaces */
			for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
				/* empty */;
			}

			if ( x.bv_val[0] != '"' ) return LDAP_INVALID_SYNTAX;
			x.bv_val++;
			x.bv_len--;

			tu->bv_val = x.bv_val;
			tu->bv_len = 0;

			for ( ; tu->bv_len < x.bv_len; tu->bv_len++ ) {
				if ( tu->bv_val[tu->bv_len] == '"' ) {
					break;
				}
			}
			x.bv_val += tu->bv_len + 1;
			x.bv_len -= tu->bv_len + 1;

			have |= HAVE_THISUPDATE;

		} else {
			return LDAP_INVALID_SYNTAX;
		}

		/* eat leading spaces */
		for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
			/* empty */;
		}

		if ( have == HAVE_ALL ) {
			break;
		}

		if ( x.bv_val[0] != ',' ) {
			return LDAP_INVALID_SYNTAX;
		}

		x.bv_val++;
		x.bv_len--;
	} while ( 1 );

	/* should have no characters left... */
	if ( x.bv_len ) return LDAP_INVALID_SYNTAX;

	if ( numdquotes == 0 ) {
		ber_dupbv_x( &ni, is, ctx );

	} else {
		ber_len_t src, dst;

		ni.bv_len = is->bv_len - numdquotes;
		ni.bv_val = ber_memalloc_x( ni.bv_len + 1, ctx );
		for ( src = 0, dst = 0; src < is->bv_len; src++, dst++ ) {
			if ( is->bv_val[src] == '"' ) {
				src++;
			}
			ni.bv_val[dst] = is->bv_val[src];
		}
		ni.bv_val[dst] = '\0';
	}
		
	*is = ni;

	return 0;
}

static int
issuerAndThisUpdateValidate(
	Syntax *syntax,
	struct berval *in )
{
	int rc;
	struct berval i, tu;

	Debug( LDAP_DEBUG_TRACE, ">>> issuerAndThisUpdateValidate: <%s>\n",
		in->bv_val, 0, 0 );

	rc = issuerAndThisUpdateCheck( in, &i, &tu, NULL );
	if ( rc ) {
		goto done;
	}

	/* validate DN -- doesn't handle double dquote */ 
	rc = dnValidate( NULL, &i );
	if ( rc ) {
		rc = LDAP_INVALID_SYNTAX;

	} else if ( checkTime( &tu, NULL ) ) {
		rc = LDAP_INVALID_SYNTAX;
	}

	if ( in->bv_val[0] == '{' && in->bv_val[in->bv_len-1] == '}' ) {
		slap_sl_free( i.bv_val, NULL );
	}

	Debug( LDAP_DEBUG_TRACE, "<<< issuerAndThisUpdateValidate: <%s> err=%d\n",
		in->bv_val, rc, 0 );

done:;
	return rc;
}

static int
issuerAndThisUpdatePretty(
	Syntax *syntax,
	struct berval *in,
	struct berval *out,
	void *ctx )
{
	int rc;
	struct berval i, tu, ni = BER_BVNULL;
	char *p;

	assert( in != NULL );
	assert( out != NULL );

	BER_BVZERO( out );

	Debug( LDAP_DEBUG_TRACE, ">>> issuerAndThisUpdatePretty: <%s>\n",
		in->bv_val, 0, 0 );

	rc = issuerAndThisUpdateCheck( in, &i, &tu, ctx );
	if ( rc ) {
		goto done;
	}

	rc = dnPretty( syntax, &i, &ni, ctx );

	if ( in->bv_val[0] == '{' && in->bv_val[in->bv_len-1] == '}' ) {
		slap_sl_free( i.bv_val, ctx );
	}

	if ( rc || checkTime( &tu, NULL ) ) {
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}

	/* make room */
	out->bv_len = STRLENOF("{ issuer rdnSequence:\"\", thisUpdate \"\" }")
		+ ni.bv_len + tu.bv_len;
	out->bv_val = slap_sl_malloc( out->bv_len + 1, ctx );

	if ( out->bv_val == NULL ) {
		out->bv_len = 0;
		rc = LDAP_OTHER;
		goto done;
	}

	p = out->bv_val;
	p = lutil_strcopy( p, "{ issuer rdnSequence:\"" /*}*/ );
	p = lutil_strbvcopy( p, &ni );
	p = lutil_strcopy( p, "\", thisUpdate \"" );
	p = lutil_strbvcopy( p, &tu );
	p = lutil_strcopy( p, /*{*/ "\" }" );

	assert( p == &out->bv_val[out->bv_len] );

done:;
	Debug( LDAP_DEBUG_TRACE, "<<< issuerAndThisUpdatePretty: <%s> => <%s>\n",
		in->bv_val, rc == LDAP_SUCCESS ? out->bv_val : "(err)", 0 );

	slap_sl_free( ni.bv_val, ctx );

	return rc; 
}

static int
issuerAndThisUpdateNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *in,
	struct berval *out,
	void *ctx )
{
	struct berval i, ni, tu, tu2;
	char sbuf[STRLENOF("YYYYmmddHHMMSSZ") + 1];
	char *p;
	int rc;

	assert( in != NULL );
	assert( out != NULL );

	Debug( LDAP_DEBUG_TRACE, ">>> issuerAndThisUpdateNormalize: <%s>\n",
		in->bv_val, 0, 0 );

	rc = issuerAndThisUpdateCheck( in, &i, &tu, ctx );
	if ( rc ) {
		return rc;
	}

	rc = dnNormalize( usage, syntax, mr, &i, &ni, ctx );

	if ( in->bv_val[0] == '{' && in->bv_val[in->bv_len-1] == '}' ) {
		slap_sl_free( i.bv_val, ctx );
	}

	tu2.bv_val = sbuf;
	tu2.bv_len = sizeof( sbuf );
	if ( rc || checkTime( &tu, &tu2 ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	out->bv_len = STRLENOF( "{ issuer rdnSequence:\"\", thisUpdate \"\" }" )
		+ ni.bv_len + tu2.bv_len;
	out->bv_val = slap_sl_malloc( out->bv_len + 1, ctx );

	if ( out->bv_val == NULL ) {
		out->bv_len = 0;
		rc = LDAP_OTHER;
		goto func_leave;
	}

	p = out->bv_val;

	p = lutil_strcopy( p, "{ issuer rdnSequence:\"" /*}*/ );
	p = lutil_strbvcopy( p, &ni );
	p = lutil_strcopy( p, "\", thisUpdate \"" );
	p = lutil_strbvcopy( p, &tu2 );
	p = lutil_strcopy( p, /*{*/ "\" }" );

	assert( p == &out->bv_val[out->bv_len] );

func_leave:
	Debug( LDAP_DEBUG_TRACE, "<<< issuerAndThisUpdateNormalize: <%s> => <%s>\n",
		in->bv_val, rc == LDAP_SUCCESS ? out->bv_val : "(err)", 0 );

	slap_sl_free( ni.bv_val, ctx );

	return rc;
}

static int
certificateListExactNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len;
	ber_int_t version;
	struct berval issuer_dn = BER_BVNULL, bvdn,
		thisUpdate, bvtu;
	char *p, tubuf[STRLENOF("YYYYmmddHHMMSSZ") + 1];
	int rc = LDAP_INVALID_SYNTAX;

	assert( val != NULL );

	Debug( LDAP_DEBUG_TRACE, ">>> certificateListExactNormalize: <%p, %lu>\n",
		val->bv_val, val->bv_len, 0 );

	if ( BER_BVISEMPTY( val ) ) goto done;

	if ( SLAP_MR_IS_VALUE_OF_ASSERTION_SYNTAX(usage) ) {
		return issuerAndThisUpdateNormalize( 0, NULL, NULL, val, normalized, ctx );
	}

	assert( SLAP_MR_IS_VALUE_OF_ATTRIBUTE_SYNTAX(usage) != 0 );

	ber_init2( ber, val, LBER_USE_DER );
	tag = ber_skip_tag( ber, &len );	/* Signed wrapper */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	tag = ber_skip_tag( ber, &len );	/* Sequence */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	tag = ber_peek_tag( ber, &len );
	/* Optional version */
	if ( tag == LBER_INTEGER ) {
		tag = ber_get_int( ber, &version );
		assert( tag == LBER_INTEGER );
		if ( version != SLAP_X509_V2 ) return LDAP_INVALID_SYNTAX;
	}
	tag = ber_skip_tag( ber, &len );	/* Signature Algorithm */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	ber_skip_data( ber, len );

	tag = ber_peek_tag( ber, &len );	/* IssuerDN */
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	len = ber_ptrlen( ber );
	bvdn.bv_val = val->bv_val + len;
	bvdn.bv_len = val->bv_len - len;
	tag = ber_skip_tag( ber, &len );
	ber_skip_data( ber, len );

	tag = ber_skip_tag( ber, &len );	/* thisUpdate */
	/* Time is a CHOICE { UTCTime, GeneralizedTime } */
	if ( tag != SLAP_TAG_UTCTIME && tag != SLAP_TAG_GENERALIZEDTIME ) return LDAP_INVALID_SYNTAX;
	bvtu.bv_val = (char *)ber->ber_ptr;
	bvtu.bv_len = len;

	rc = dnX509normalize( &bvdn, &issuer_dn );
	if ( rc != LDAP_SUCCESS ) goto done;

	thisUpdate.bv_val = tubuf;
	thisUpdate.bv_len = sizeof(tubuf);
	if ( checkTime( &bvtu, &thisUpdate ) ) {
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}

	normalized->bv_len = STRLENOF( "{ issuer rdnSequence:\"\", thisUpdate \"\" }" )
		+ issuer_dn.bv_len + thisUpdate.bv_len;
	normalized->bv_val = ch_malloc( normalized->bv_len + 1 );

	p = normalized->bv_val;

	p = lutil_strcopy( p, "{ issuer rdnSequence:\"" );
	p = lutil_strbvcopy( p, &issuer_dn );
	p = lutil_strcopy( p, "\", thisUpdate \"" );
	p = lutil_strbvcopy( p, &thisUpdate );
	p = lutil_strcopy( p, /*{*/ "\" }" );

	rc = LDAP_SUCCESS;

done:
	Debug( LDAP_DEBUG_TRACE, "<<< certificateListExactNormalize: <%p, %lu> => <%s>\n",
		val->bv_val, val->bv_len, rc == LDAP_SUCCESS ? normalized->bv_val : "(err)" );

	if ( issuer_dn.bv_val ) ber_memfree( issuer_dn.bv_val );

	return rc;
}

/* X.509 PMI serialNumberAndIssuerSerialCheck

AttributeCertificateExactAssertion     ::= SEQUENCE {
   serialNumber              CertificateSerialNumber,
   issuer                    AttCertIssuer }

CertificateSerialNumber ::= INTEGER

AttCertIssuer ::=    [0] SEQUENCE {
issuerName                     GeneralNames OPTIONAL,
baseCertificateID         [0] IssuerSerial OPTIONAL,
objectDigestInfo          [1] ObjectDigestInfo OPTIONAL }
-- At least one component shall be present

GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName

GeneralName ::= CHOICE {
  otherName                 [0] INSTANCE OF OTHER-NAME,
  rfc822Name                [1] IA5String,
  dNSName                   [2] IA5String,
  x400Address               [3] ORAddress,
  directoryName             [4] Name,
  ediPartyName              [5] EDIPartyName,
  uniformResourceIdentifier [6] IA5String,
  iPAddress                 [7] OCTET STRING,
  registeredID              [8] OBJECT IDENTIFIER }

IssuerSerial ::= SEQUENCE {
   issuer       GeneralNames,
   serial       CertificateSerialNumber,
   issuerUID UniqueIdentifier OPTIONAL }

ObjectDigestInfo ::= SEQUENCE {
   digestedObjectType ENUMERATED {
      publicKey           (0),
      publicKeyCert       (1),
      otherObjectTypes    (2) },
   otherObjectTypeID      OBJECT IDENTIFIER OPTIONAL,
   digestAlgorithm        AlgorithmIdentifier,
   objectDigest           BIT STRING }

 * The way I interpret it, an assertion should look like

 { serialNumber 'dd'H,
   issuer { issuerName { directoryName:rdnSequence:"cn=yyy" }, -- optional
            baseCertificateID { serial '1d'H,
                                issuer { directoryName:rdnSequence:"cn=zzz" },
                                issuerUID <value>              -- optional
                              },                               -- optional
            objectDigestInfo { ... }                           -- optional
          }
 }
 
 * with issuerName, baseCertificateID and objectDigestInfo optional,
 * at least one present; the way it's currently implemented, it is

 { serialNumber 'dd'H,
   issuer { baseCertificateID { serial '1d'H,
                                issuer { directoryName:rdnSequence:"cn=zzz" }
                              }
          }
 }

 * with all the above parts mandatory.
 */
static int
serialNumberAndIssuerSerialCheck(
	struct berval *in,
	struct berval *sn,
	struct berval *is,
	struct berval *i_sn,	/* contain serial of baseCertificateID */
	void *ctx )
{
	/* Parse GSER format */ 
	enum {
		HAVE_NONE = 0x0,
		HAVE_SN = 0x1,
		HAVE_ISSUER = 0x2,
		HAVE_ALL = ( HAVE_SN | HAVE_ISSUER )
	} have = HAVE_NONE, have2 = HAVE_NONE;
	int numdquotes = 0;
	struct berval x = *in;
	struct berval ni;

	if ( in->bv_len < 3 ) return LDAP_INVALID_SYNTAX;

	/* no old format */
	if ( in->bv_val[0] != '{' && in->bv_val[in->bv_len-1] != '}' ) return LDAP_INVALID_SYNTAX;

	x.bv_val++;
	x.bv_len -= 2;

	do {

		/* eat leading spaces */
		for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
			/* empty */;
		}

		/* should be at issuer or serialNumber NamedValue */
		if ( strncasecmp( x.bv_val, "issuer", STRLENOF("issuer") ) == 0 ) {
			if ( have & HAVE_ISSUER ) {
				return LDAP_INVALID_SYNTAX;
			}

			/* parse IssuerSerial */
			x.bv_val += STRLENOF("issuer");
			x.bv_len -= STRLENOF("issuer");

			if ( x.bv_val[0] != ' ' ) return LDAP_INVALID_SYNTAX;
			x.bv_val++;
			x.bv_len--;

			/* eat leading spaces */
			for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
				/* empty */;
			}

			if ( x.bv_val[0] != '{' /*}*/ ) return LDAP_INVALID_SYNTAX;
			x.bv_val++;
			x.bv_len--;

			/* eat leading spaces */
			for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
				/* empty */;
			}

			if ( strncasecmp( x.bv_val, "baseCertificateID ", STRLENOF("baseCertificateID ") ) != 0 ) {
				return LDAP_INVALID_SYNTAX;
			}
			x.bv_val += STRLENOF("baseCertificateID ");
			x.bv_len -= STRLENOF("baseCertificateID ");

			/* eat leading spaces */
			for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
				/* empty */;
			}

			if ( x.bv_val[0] != '{' /*}*/ ) return LDAP_INVALID_SYNTAX;
			x.bv_val++;
			x.bv_len--;

			do {
				/* eat leading spaces */
				for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
					/* empty */;
				}

				/* parse issuer of baseCertificateID */
				if ( strncasecmp( x.bv_val, "issuer ", STRLENOF("issuer ") ) == 0 ) {
					if ( have2 & HAVE_ISSUER ) {
						return LDAP_INVALID_SYNTAX;
					}

					x.bv_val += STRLENOF("issuer ");
					x.bv_len -= STRLENOF("issuer ");

					/* eat leading spaces */
					for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
						/* empty */;
					}

					if ( x.bv_val[0] != '{' /*}*/ ) return LDAP_INVALID_SYNTAX;
					x.bv_val++;
					x.bv_len--;

					/* eat leading spaces */
					for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
						/* empty */;
					}

					if ( strncasecmp( x.bv_val, "directoryName:rdnSequence:", STRLENOF("directoryName:rdnSequence:") ) != 0 ) {
						return LDAP_INVALID_SYNTAX;
					}
					x.bv_val += STRLENOF("directoryName:rdnSequence:");
					x.bv_len -= STRLENOF("directoryName:rdnSequence:");

					if ( x.bv_val[0] != '"' ) return LDAP_INVALID_SYNTAX;
					x.bv_val++;
					x.bv_len--;

					is->bv_val = x.bv_val;
					is->bv_len = 0;

					for ( ; is->bv_len < x.bv_len; ) {
						if ( is->bv_val[is->bv_len] != '"' ) {
							is->bv_len++;
							continue;
						}
						if ( is->bv_val[is->bv_len + 1] == '"' ) {
							/* double dquote */
							is->bv_len += 2;
							continue;
						}
						break;
					}
					x.bv_val += is->bv_len + 1;
					x.bv_len -= is->bv_len + 1;

					/* eat leading spaces */
					for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
						/* empty */;
					}

					if ( x.bv_val[0] != /*{*/ '}' ) return LDAP_INVALID_SYNTAX;
					x.bv_val++;
					x.bv_len--;

					have2 |= HAVE_ISSUER;

				} else if ( strncasecmp( x.bv_val, "serial ", STRLENOF("serial ") ) == 0 ) {
					if ( have2 & HAVE_SN ) {
						return LDAP_INVALID_SYNTAX;
					}

					x.bv_val += STRLENOF("serial ");
					x.bv_len -= STRLENOF("serial ");

					/* eat leading spaces */
					for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len--) {
						/* empty */;
					}

					if ( checkNum( &x, i_sn ) ) {
						return LDAP_INVALID_SYNTAX;
					}

					x.bv_val += i_sn->bv_len;
					x.bv_len -= i_sn->bv_len;

					have2 |= HAVE_SN;

				} else {
					return LDAP_INVALID_SYNTAX;
				}

				/* eat leading spaces */
				for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
					/* empty */;
				}

				if ( have2 == HAVE_ALL ) {
					break;
				}

				if ( x.bv_val[0] != ',' ) return LDAP_INVALID_SYNTAX;
				x.bv_val++;
				x.bv_len--;
			} while ( 1 );

			if ( x.bv_val[0] != /*{*/ '}' ) return LDAP_INVALID_SYNTAX;
			x.bv_val++;
			x.bv_len--;

			/* eat leading spaces */
			for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
				/* empty */;
			}

			if ( x.bv_val[0] != /*{*/ '}' ) return LDAP_INVALID_SYNTAX;
			x.bv_val++;
			x.bv_len--;

			have |= HAVE_ISSUER;

		} else if ( strncasecmp( x.bv_val, "serialNumber", STRLENOF("serialNumber") ) == 0 ) {
			if ( have & HAVE_SN ) {
				return LDAP_INVALID_SYNTAX;
			}

			/* parse serialNumber */
			x.bv_val += STRLENOF("serialNumber");
			x.bv_len -= STRLENOF("serialNumber");

			if ( x.bv_val[0] != ' ' ) return LDAP_INVALID_SYNTAX;
			x.bv_val++;
			x.bv_len--;

			/* eat leading spaces */
			for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
				/* empty */;
			}
			
			if ( checkNum( &x, sn ) ) {
				return LDAP_INVALID_SYNTAX;
			}

			x.bv_val += sn->bv_len;
			x.bv_len -= sn->bv_len;

			have |= HAVE_SN;

		} else {
			return LDAP_INVALID_SYNTAX;
		}

		/* eat spaces */
		for ( ; (x.bv_val[0] == ' ') && x.bv_len; x.bv_val++, x.bv_len-- ) {
			/* empty */;
		}

		if ( have == HAVE_ALL ) {
			break;
		}

		if ( x.bv_val[0] != ',' ) {
			return LDAP_INVALID_SYNTAX;
		}
		x.bv_val++ ;
		x.bv_len--;
	} while ( 1 );

	/* should have no characters left... */
	if( x.bv_len ) return LDAP_INVALID_SYNTAX;

	if ( numdquotes == 0 ) {
		ber_dupbv_x( &ni, is, ctx );

	} else {
		ber_len_t src, dst;

		ni.bv_len = is->bv_len - numdquotes;
		ni.bv_val = ber_memalloc_x( ni.bv_len + 1, ctx );
		for ( src = 0, dst = 0; src < is->bv_len; src++, dst++ ) {
			if ( is->bv_val[src] == '"' ) {
				src++;
			}
			ni.bv_val[dst] = is->bv_val[src];
		}
		ni.bv_val[dst] = '\0';
	}

	*is = ni;

	/* need to handle double dquotes here */
	return 0;
}

/* X.509 PMI serialNumberAndIssuerSerialValidate */
static int
serialNumberAndIssuerSerialValidate(
	Syntax *syntax,
	struct berval *in )
{
	int rc;
	struct berval sn, i, i_sn;

	Debug( LDAP_DEBUG_TRACE, ">>> serialNumberAndIssuerSerialValidate: <%s>\n",
		in->bv_val, 0, 0 );

	rc = serialNumberAndIssuerSerialCheck( in, &sn, &i, &i_sn, NULL );
	if ( rc ) {
		goto done;
	}

	/* validate DN -- doesn't handle double dquote */ 
	rc = dnValidate( NULL, &i );
	if ( rc ) {
		rc = LDAP_INVALID_SYNTAX;
	}

	if ( in->bv_val[0] == '{' && in->bv_val[in->bv_len-1] == '}' ) {
		slap_sl_free( i.bv_val, NULL );
	}

done:;
	Debug( LDAP_DEBUG_TRACE, "<<< serialNumberAndIssuerSerialValidate: <%s> err=%d\n",
		in->bv_val, rc, 0 );

	return rc;
}

/* X.509 PMI serialNumberAndIssuerSerialPretty */
static int
serialNumberAndIssuerSerialPretty(
	Syntax *syntax,
	struct berval *in,
	struct berval *out,
	void *ctx )
{
	struct berval sn, i, i_sn, ni = BER_BVNULL;
	char *p;
	int rc;

	assert( in != NULL );
	assert( out != NULL );

	Debug( LDAP_DEBUG_TRACE, ">>> serialNumberAndIssuerSerialPretty: <%s>\n",
		in->bv_val, 0, 0 );

	rc = serialNumberAndIssuerSerialCheck( in, &sn, &i, &i_sn, ctx );
	if ( rc ) {
		goto done;
	}

	rc = dnPretty( syntax, &i, &ni, ctx );

	if ( in->bv_val[0] == '{' && in->bv_val[in->bv_len-1] == '}' ) {
		slap_sl_free( i.bv_val, ctx );
	}

	if ( rc ) {
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}

	/* make room from sn + "$" */
	out->bv_len = STRLENOF("{ serialNumber , issuer { baseCertificateID { issuer { directoryName:rdnSequence:\"\" }, serial  } } }")
		+ sn.bv_len + ni.bv_len + i_sn.bv_len;
	out->bv_val = slap_sl_malloc( out->bv_len + 1, ctx );

	if ( out->bv_val == NULL ) {
		out->bv_len = 0;
		rc = LDAP_OTHER;
		goto done;
	}

	p = out->bv_val;
	p = lutil_strcopy( p, "{ serialNumber " );
	p = lutil_strbvcopy( p, &sn );
	p = lutil_strcopy( p, ", issuer { baseCertificateID { issuer { directoryName:rdnSequence:\"" );
	p = lutil_strbvcopy( p, &ni );
	p = lutil_strcopy( p, "\" }, serial " );
	p = lutil_strbvcopy( p, &i_sn );
	p = lutil_strcopy( p, " } } }" );

	assert( p == &out->bv_val[out->bv_len] );

done:;
	Debug( LDAP_DEBUG_TRACE, "<<< serialNumberAndIssuerSerialPretty: <%s> => <%s>\n",
		in->bv_val, rc == LDAP_SUCCESS ? out->bv_val : "(err)", 0 );

	slap_sl_free( ni.bv_val, ctx );

	return rc; 
}

/* X.509 PMI serialNumberAndIssuerSerialNormalize */
/*
 * This routine is called by attributeCertificateExactNormalize
 * when attributeCertificateExactNormalize receives a search 
 * string instead of a attribute certificate. This routine 
 * checks if the search value is valid and then returns the 
 * normalized value
 */
static int
serialNumberAndIssuerSerialNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *in,
	struct berval *out,
	void *ctx )
{
	struct berval i, ni = BER_BVNULL,
		sn, sn2 = BER_BVNULL, sn3 = BER_BVNULL,
		i_sn, i_sn2 = BER_BVNULL, i_sn3 = BER_BVNULL;
	char sbuf2[SLAP_SN_BUFLEN], i_sbuf2[SLAP_SN_BUFLEN],
		sbuf3[SLAP_SN_BUFLEN], i_sbuf3[SLAP_SN_BUFLEN];
	char *p;
	int rc;

	assert( in != NULL );
	assert( out != NULL );

	Debug( LDAP_DEBUG_TRACE, ">>> serialNumberAndIssuerSerialNormalize: <%s>\n",
		in->bv_val, 0, 0 );

	rc = serialNumberAndIssuerSerialCheck( in, &sn, &i, &i_sn, ctx );
	if ( rc ) {
		goto func_leave;
	}

	rc = dnNormalize( usage, syntax, mr, &i, &ni, ctx );

	if ( in->bv_val[0] == '{' && in->bv_val[in->bv_len-1] == '}' ) {
		slap_sl_free( i.bv_val, ctx );
	}

	if ( rc ) {
		rc = LDAP_INVALID_SYNTAX;
		goto func_leave;
	}

	/* Convert sn to canonical hex */
	sn2.bv_val = sbuf2;
	sn2.bv_len = sn.bv_len;
	if ( sn.bv_len > sizeof( sbuf2 ) ) {
		sn2.bv_val = slap_sl_malloc( sn.bv_len, ctx );
	}
	if ( lutil_str2bin( &sn, &sn2, ctx ) ) {
		rc = LDAP_INVALID_SYNTAX;
		goto func_leave;
	}

        /* Convert i_sn to canonical hex */
	i_sn2.bv_val = i_sbuf2;
	i_sn2.bv_len = i_sn.bv_len;
	if ( i_sn.bv_len > sizeof( i_sbuf2 ) ) {
		i_sn2.bv_val = slap_sl_malloc( i_sn.bv_len, ctx );
	}
	if ( lutil_str2bin( &i_sn, &i_sn2, ctx ) ) {
		rc = LDAP_INVALID_SYNTAX;
		goto func_leave;
	}

	sn3.bv_val = sbuf3;
	sn3.bv_len = sizeof(sbuf3);
	if ( slap_bin2hex( &sn2, &sn3, ctx ) ) {
		rc = LDAP_INVALID_SYNTAX;
		goto func_leave;
	}

	i_sn3.bv_val = i_sbuf3;
	i_sn3.bv_len = sizeof(i_sbuf3);
	if ( slap_bin2hex( &i_sn2, &i_sn3, ctx ) ) {
		rc = LDAP_INVALID_SYNTAX;
		goto func_leave;
	}

	out->bv_len = STRLENOF("{ serialNumber , issuer { baseCertificateID { issuer { directoryName:rdnSequence:\"\" }, serial  } } }")
		+ sn3.bv_len + ni.bv_len + i_sn3.bv_len;
	out->bv_val = slap_sl_malloc( out->bv_len + 1, ctx );

	if ( out->bv_val == NULL ) {
		out->bv_len = 0;
		rc = LDAP_OTHER;
		goto func_leave;
	}

	p = out->bv_val;

	p = lutil_strcopy( p, "{ serialNumber " );
	p = lutil_strbvcopy( p, &sn3 );
	p = lutil_strcopy( p, ", issuer { baseCertificateID { issuer { directoryName:rdnSequence:\"" );
	p = lutil_strbvcopy( p, &ni );
	p = lutil_strcopy( p, "\" }, serial " );
	p = lutil_strbvcopy( p, &i_sn3 );
	p = lutil_strcopy( p, " } } }" );

	assert( p == &out->bv_val[out->bv_len] );

func_leave:
	Debug( LDAP_DEBUG_TRACE, "<<< serialNumberAndIssuerSerialNormalize: <%s> => <%s>\n",
		in->bv_val, rc == LDAP_SUCCESS ? out->bv_val : "(err)", 0 );

	if ( sn2.bv_val != sbuf2 ) {
		slap_sl_free( sn2.bv_val, ctx );
	}

	if ( i_sn2.bv_val != i_sbuf2 ) {
		slap_sl_free( i_sn2.bv_val, ctx );
	}

	if ( sn3.bv_val != sbuf3 ) {
		slap_sl_free( sn3.bv_val, ctx );
	}

	if ( i_sn3.bv_val != i_sbuf3 ) {
		slap_sl_free( i_sn3.bv_val, ctx );
	}

	slap_sl_free( ni.bv_val, ctx );

	return rc;
}

/* X.509 PMI attributeCertificateExactNormalize */
static int
attributeCertificateExactNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len;
	char issuer_serialbuf[SLAP_SN_BUFLEN], serialbuf[SLAP_SN_BUFLEN];
	struct berval sn, i_sn, sn2 = BER_BVNULL, i_sn2 = BER_BVNULL;
	struct berval issuer_dn = BER_BVNULL, bvdn;
	char *p;
	int rc = LDAP_INVALID_SYNTAX;

	if ( BER_BVISEMPTY( val ) ) {
		return rc;
	}

	if ( SLAP_MR_IS_VALUE_OF_ASSERTION_SYNTAX(usage) ) {
		return serialNumberAndIssuerSerialNormalize( 0, NULL, NULL, val, normalized, ctx );
	}

	assert( SLAP_MR_IS_VALUE_OF_ATTRIBUTE_SYNTAX(usage) != 0 );

	ber_init2( ber, val, LBER_USE_DER );
	tag = ber_skip_tag( ber, &len );	/* Signed Sequence */
	tag = ber_skip_tag( ber, &len );	/* Sequence */
	tag = ber_skip_tag( ber, &len );	/* (Mandatory) version; must be v2(1) */
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );	/* Holder Sequence */
	ber_skip_data( ber, len );

	/* Issuer */
	tag = ber_skip_tag( ber, &len );	/* Sequence */
						/* issuerName (GeneralNames sequence; optional)? */
	tag = ber_skip_tag( ber, &len );	/* baseCertificateID (sequence; optional)? */
	tag = ber_skip_tag( ber, &len );	/* GeneralNames (sequence) */
	tag = ber_skip_tag( ber, &len );	/* directoryName (we only accept this form of GeneralName) */
	if ( tag != SLAP_X509_GN_DIRECTORYNAME ) { 
		return LDAP_INVALID_SYNTAX; 
	}
	tag = ber_peek_tag( ber, &len );	/* sequence of RDN */
	len = ber_ptrlen( ber );
	bvdn.bv_val = val->bv_val + len;
	bvdn.bv_len = val->bv_len - len;
	rc = dnX509normalize( &bvdn, &issuer_dn );
	if ( rc != LDAP_SUCCESS ) goto done;
	
	tag = ber_skip_tag( ber, &len );	/* sequence of RDN */
	ber_skip_data( ber, len ); 
	tag = ber_skip_tag( ber, &len );	/* serial number */
	if ( tag != LBER_INTEGER ) {
		rc = LDAP_INVALID_SYNTAX; 
		goto done;
	}
	i_sn.bv_val = (char *)ber->ber_ptr;
	i_sn.bv_len = len;
	i_sn2.bv_val = issuer_serialbuf;
	i_sn2.bv_len = sizeof(issuer_serialbuf);
	if ( slap_bin2hex( &i_sn, &i_sn2, ctx ) ) {
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}
	ber_skip_data( ber, len );

						/* issuerUID (bitstring; optional)? */
						/* objectDigestInfo (sequence; optional)? */

	tag = ber_skip_tag( ber, &len );	/* Signature (sequence) */
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );	/* serial number */ 
	if ( tag != LBER_INTEGER ) {
		rc = LDAP_INVALID_SYNTAX; 
		goto done;
	}
	sn.bv_val = (char *)ber->ber_ptr;
	sn.bv_len = len;
	sn2.bv_val = serialbuf;
	sn2.bv_len = sizeof(serialbuf);
	if ( slap_bin2hex( &sn, &sn2, ctx ) ) {
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}
	ber_skip_data( ber, len );

	normalized->bv_len = STRLENOF( "{ serialNumber , issuer { baseCertificateID { issuer { directoryName:rdnSequence:\"\" }, serial  } } }" )
		+ sn2.bv_len + issuer_dn.bv_len + i_sn2.bv_len;
	normalized->bv_val = ch_malloc( normalized->bv_len + 1 );

	p = normalized->bv_val;

	p = lutil_strcopy( p, "{ serialNumber " );
	p = lutil_strbvcopy( p, &sn2 );
	p = lutil_strcopy( p, ", issuer { baseCertificateID { issuer { directoryName:rdnSequence:\"" );
	p = lutil_strbvcopy( p, &issuer_dn );
	p = lutil_strcopy( p, "\" }, serial " );
	p = lutil_strbvcopy( p, &i_sn2 );
	p = lutil_strcopy( p, " } } }" );

	Debug( LDAP_DEBUG_TRACE, "attributeCertificateExactNormalize: %s\n",
		normalized->bv_val, NULL, NULL );

	rc = LDAP_SUCCESS;

done:
	if ( issuer_dn.bv_val ) ber_memfree( issuer_dn.bv_val );
	if ( i_sn2.bv_val != issuer_serialbuf ) ber_memfree_x( i_sn2.bv_val, ctx );
	if ( sn2.bv_val != serialbuf ) ber_memfree_x( sn2.bv_val, ctx );

	return rc;
}


static int
hexValidate(
	Syntax *syntax,
	struct berval *in )
{
	ber_len_t	i;

	assert( in != NULL );
	assert( !BER_BVISNULL( in ) );

	for ( i = 0; i < in->bv_len; i++ ) {
		if ( !ASCII_HEX( in->bv_val[ i ] ) ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	return LDAP_SUCCESS;
}

/* Normalize a SID as used inside a CSN:
 * three-digit numeric string */
static int
hexNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	ber_len_t	i;

	assert( val != NULL );
	assert( normalized != NULL );

	ber_dupbv_x( normalized, val, ctx );

	for ( i = 0; i < normalized->bv_len; i++ ) {
		if ( !ASCII_HEX( normalized->bv_val[ i ] ) ) {
			ber_memfree_x( normalized->bv_val, ctx );
			BER_BVZERO( normalized );
			return LDAP_INVALID_SYNTAX;
		}

		normalized->bv_val[ i ] = TOLOWER( normalized->bv_val[ i ] );
	}

	return LDAP_SUCCESS;
}

static int
sidValidate (
	Syntax *syntax,
	struct berval *in )
{
	assert( in != NULL );
	assert( !BER_BVISNULL( in ) );

	if ( in->bv_len != 3 ) {
		return LDAP_INVALID_SYNTAX;
	}

	return hexValidate( NULL, in );
}

/* Normalize a SID as used inside a CSN:
 * three-digit numeric string */
static int
sidNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	if ( val->bv_len != 3 ) {
		return LDAP_INVALID_SYNTAX;
	}

	return hexNormalize( 0, NULL, NULL, val, normalized, ctx );
}

static int
sidPretty(
	Syntax *syntax,
	struct berval *val,
	struct berval *out,
	void *ctx )
{
	return sidNormalize( SLAP_MR_VALUE_OF_SYNTAX, NULL, NULL, val, out, ctx );
}

/* Normalize a SID as used inside a CSN, either as-is
 * (assertion value) or extracted from the CSN
 * (attribute value) */
static int
csnSidNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	struct berval	bv;
	char		*ptr,
			buf[ 4 ];


	if ( BER_BVISEMPTY( val ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	if ( SLAP_MR_IS_VALUE_OF_ASSERTION_SYNTAX(usage) ) {
		return sidNormalize( 0, NULL, NULL, val, normalized, ctx );
	}

	assert( SLAP_MR_IS_VALUE_OF_ATTRIBUTE_SYNTAX(usage) != 0 );

	ptr = ber_bvchr( val, '#' );
	if ( ptr == NULL || ptr == &val->bv_val[val->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	bv.bv_val = ptr + 1;
	bv.bv_len = val->bv_len - ( ptr + 1 - val->bv_val );

	ptr = ber_bvchr( &bv, '#' );
	if ( ptr == NULL || ptr == &val->bv_val[val->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	bv.bv_val = ptr + 1;
	bv.bv_len = val->bv_len - ( ptr + 1 - val->bv_val );
		
	ptr = ber_bvchr( &bv, '#' );
	if ( ptr == NULL || ptr == &val->bv_val[val->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	bv.bv_len = ptr - bv.bv_val;

	if ( bv.bv_len == 2 ) {
		/* OpenLDAP 2.3 SID */
		buf[ 0 ] = '0';
		buf[ 1 ] = bv.bv_val[ 0 ];
		buf[ 2 ] = bv.bv_val[ 1 ];
		buf[ 3 ] = '\0';

		bv.bv_val = buf;
		bv.bv_len = 3;
	}

	return sidNormalize( 0, NULL, NULL, &bv, normalized, ctx );
}

static int
csnValidate(
	Syntax *syntax,
	struct berval *in )
{
	struct berval	bv;
	char		*ptr;
	int		rc;

	assert( in != NULL );
	assert( !BER_BVISNULL( in ) );

	if ( BER_BVISEMPTY( in ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	bv = *in;

	ptr = ber_bvchr( &bv, '#' );
	if ( ptr == NULL || ptr == &bv.bv_val[bv.bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	bv.bv_len = ptr - bv.bv_val;
	if ( bv.bv_len != STRLENOF( "YYYYmmddHHMMSS.uuuuuuZ" ) &&
		bv.bv_len != STRLENOF( "YYYYmmddHHMMSSZ" ) )
	{
		return LDAP_INVALID_SYNTAX;
	}

	rc = generalizedTimeValidate( NULL, &bv );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	bv.bv_val = ptr + 1;
	bv.bv_len = in->bv_len - ( bv.bv_val - in->bv_val );

	ptr = ber_bvchr( &bv, '#' );
	if ( ptr == NULL || ptr == &in->bv_val[in->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	bv.bv_len = ptr - bv.bv_val;
	if ( bv.bv_len != 6 ) {
		return LDAP_INVALID_SYNTAX;
	}

	rc = hexValidate( NULL, &bv );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	bv.bv_val = ptr + 1;
	bv.bv_len = in->bv_len - ( bv.bv_val - in->bv_val );

	ptr = ber_bvchr( &bv, '#' );
	if ( ptr == NULL || ptr == &in->bv_val[in->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	bv.bv_len = ptr - bv.bv_val;
	if ( bv.bv_len == 2 ) {
		/* tolerate old 2-digit replica-id */
		rc = hexValidate( NULL, &bv );

	} else {
		rc = sidValidate( NULL, &bv );
	}
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	bv.bv_val = ptr + 1;
	bv.bv_len = in->bv_len - ( bv.bv_val - in->bv_val );

	if ( bv.bv_len != 6 ) {
		return LDAP_INVALID_SYNTAX;
	}

	return hexValidate( NULL, &bv );
}

/* Normalize a CSN in OpenLDAP 2.1 format */
static int
csnNormalize21(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	struct berval	gt, cnt, sid, mod;
	struct berval	bv;
	char		buf[ STRLENOF( "YYYYmmddHHMMSS.uuuuuuZ#SSSSSS#SID#ssssss" ) + 1 ];
	char		*ptr;
	ber_len_t	i;

	assert( SLAP_MR_IS_VALUE_OF_SYNTAX( usage ) != 0 );
	assert( !BER_BVISEMPTY( val ) );

	gt = *val;

	ptr = ber_bvchr( &gt, '#' );
	if ( ptr == NULL || ptr == &gt.bv_val[gt.bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	gt.bv_len = ptr - gt.bv_val;
	if ( gt.bv_len != STRLENOF( "YYYYmmddHH:MM:SSZ" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	if ( gt.bv_val[ 10 ] != ':' || gt.bv_val[ 13 ] != ':' ) {
		return LDAP_INVALID_SYNTAX;
	}

	cnt.bv_val = ptr + 1;
	cnt.bv_len = val->bv_len - ( cnt.bv_val - val->bv_val );

	ptr = ber_bvchr( &cnt, '#' );
	if ( ptr == NULL || ptr == &val->bv_val[val->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	cnt.bv_len = ptr - cnt.bv_val;
	if ( cnt.bv_len != STRLENOF( "0x0000" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	if ( strncmp( cnt.bv_val, "0x", STRLENOF( "0x" ) ) != 0 ) {
		return LDAP_INVALID_SYNTAX;
	}

	cnt.bv_val += STRLENOF( "0x" );
	cnt.bv_len -= STRLENOF( "0x" );

	sid.bv_val = ptr + 1;
	sid.bv_len = val->bv_len - ( sid.bv_val - val->bv_val );
		
	ptr = ber_bvchr( &sid, '#' );
	if ( ptr == NULL || ptr == &val->bv_val[val->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	sid.bv_len = ptr - sid.bv_val;
	if ( sid.bv_len != STRLENOF( "0" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	mod.bv_val = ptr + 1;
	mod.bv_len = val->bv_len - ( mod.bv_val - val->bv_val );
	if ( mod.bv_len != STRLENOF( "0000" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	bv.bv_len = STRLENOF( "YYYYmmddHHMMSS.uuuuuuZ#SSSSSS#SID#ssssss" );
	bv.bv_val = buf;

	ptr = bv.bv_val;
	ptr = lutil_strncopy( ptr, gt.bv_val, STRLENOF( "YYYYmmddHH" ) );
	ptr = lutil_strncopy( ptr, &gt.bv_val[ STRLENOF( "YYYYmmddHH:" ) ],
		STRLENOF( "MM" ) );
	ptr = lutil_strncopy( ptr, &gt.bv_val[ STRLENOF( "YYYYmmddHH:MM:" ) ],
		STRLENOF( "SS" ) );
	ptr = lutil_strcopy( ptr, ".000000Z#00" );
	ptr = lutil_strbvcopy( ptr, &cnt );
	*ptr++ = '#';
	*ptr++ = '0';
	*ptr++ = '0';
	*ptr++ = sid.bv_val[ 0 ];
	*ptr++ = '#';
	*ptr++ = '0';
	*ptr++ = '0';
	for ( i = 0; i < mod.bv_len; i++ ) {
		*ptr++ = TOLOWER( mod.bv_val[ i ] );
	}
	*ptr = '\0';

	assert( ptr == &bv.bv_val[bv.bv_len] );

	if ( csnValidate( syntax, &bv ) != LDAP_SUCCESS ) {
		return LDAP_INVALID_SYNTAX;
	}

	ber_dupbv_x( normalized, &bv, ctx );

	return LDAP_SUCCESS;
}

/* Normalize a CSN in OpenLDAP 2.3 format */
static int
csnNormalize23(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	struct berval	gt, cnt, sid, mod;
	struct berval	bv;
	char		buf[ STRLENOF( "YYYYmmddHHMMSS.uuuuuuZ#SSSSSS#SID#ssssss" ) + 1 ];
	char		*ptr;
	ber_len_t	i;

	assert( SLAP_MR_IS_VALUE_OF_SYNTAX( usage ) != 0 );
	assert( !BER_BVISEMPTY( val ) );

	gt = *val;

	ptr = ber_bvchr( &gt, '#' );
	if ( ptr == NULL || ptr == &gt.bv_val[gt.bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	gt.bv_len = ptr - gt.bv_val;
	if ( gt.bv_len != STRLENOF( "YYYYmmddHHMMSSZ" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	cnt.bv_val = ptr + 1;
	cnt.bv_len = val->bv_len - ( cnt.bv_val - val->bv_val );

	ptr = ber_bvchr( &cnt, '#' );
	if ( ptr == NULL || ptr == &val->bv_val[val->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	cnt.bv_len = ptr - cnt.bv_val;
	if ( cnt.bv_len != STRLENOF( "000000" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	sid.bv_val = ptr + 1;
	sid.bv_len = val->bv_len - ( sid.bv_val - val->bv_val );
		
	ptr = ber_bvchr( &sid, '#' );
	if ( ptr == NULL || ptr == &val->bv_val[val->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	sid.bv_len = ptr - sid.bv_val;
	if ( sid.bv_len != STRLENOF( "00" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	mod.bv_val = ptr + 1;
	mod.bv_len = val->bv_len - ( mod.bv_val - val->bv_val );
	if ( mod.bv_len != STRLENOF( "000000" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	bv.bv_len = STRLENOF( "YYYYmmddHHMMSS.uuuuuuZ#SSSSSS#SID#ssssss" );
	bv.bv_val = buf;

	ptr = bv.bv_val;
	ptr = lutil_strncopy( ptr, gt.bv_val, gt.bv_len - 1 );
	ptr = lutil_strcopy( ptr, ".000000Z#" );
	ptr = lutil_strbvcopy( ptr, &cnt );
	*ptr++ = '#';
	*ptr++ = '0';
	for ( i = 0; i < sid.bv_len; i++ ) {
		*ptr++ = TOLOWER( sid.bv_val[ i ] );
	}
	*ptr++ = '#';
	for ( i = 0; i < mod.bv_len; i++ ) {
		*ptr++ = TOLOWER( mod.bv_val[ i ] );
	}
	*ptr = '\0';

	assert( ptr == &bv.bv_val[bv.bv_len] );
	if ( csnValidate( syntax, &bv ) != LDAP_SUCCESS ) {
		return LDAP_INVALID_SYNTAX;
	}

	ber_dupbv_x( normalized, &bv, ctx );

	return LDAP_SUCCESS;
}

/* Normalize a CSN */
static int
csnNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	struct berval	cnt, sid, mod;
	char		*ptr;
	ber_len_t	i;

	assert( val != NULL );
	assert( normalized != NULL );

	assert( SLAP_MR_IS_VALUE_OF_SYNTAX( usage ) != 0 );

	if ( BER_BVISEMPTY( val ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	if ( val->bv_len == STRLENOF( "YYYYmmddHHMMSSZ#SSSSSS#ID#ssssss" ) ) {
		/* Openldap <= 2.3 */

		return csnNormalize23( usage, syntax, mr, val, normalized, ctx );
	}

	if ( val->bv_len == STRLENOF( "YYYYmmddHH:MM:SSZ#0xSSSS#I#ssss" ) ) {
		/* Openldap 2.1 */

		return csnNormalize21( usage, syntax, mr, val, normalized, ctx );
	}

	if ( val->bv_len != STRLENOF( "YYYYmmddHHMMSS.uuuuuuZ#SSSSSS#SID#ssssss" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	ptr = ber_bvchr( val, '#' );
	if ( ptr == NULL || ptr == &val->bv_val[val->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	if ( ptr - val->bv_val != STRLENOF( "YYYYmmddHHMMSS.uuuuuuZ" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	cnt.bv_val = ptr + 1;
	cnt.bv_len = val->bv_len - ( cnt.bv_val - val->bv_val );

	ptr = ber_bvchr( &cnt, '#' );
	if ( ptr == NULL || ptr == &val->bv_val[val->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	if ( ptr - cnt.bv_val != STRLENOF( "000000" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	sid.bv_val = ptr + 1;
	sid.bv_len = val->bv_len - ( sid.bv_val - val->bv_val );
		
	ptr = ber_bvchr( &sid, '#' );
	if ( ptr == NULL || ptr == &val->bv_val[val->bv_len] ) {
		return LDAP_INVALID_SYNTAX;
	}

	sid.bv_len = ptr - sid.bv_val;
	if ( sid.bv_len != STRLENOF( "000" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	mod.bv_val = ptr + 1;
	mod.bv_len = val->bv_len - ( mod.bv_val - val->bv_val );

	if ( mod.bv_len != STRLENOF( "000000" ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	ber_dupbv_x( normalized, val, ctx );

	for ( i = STRLENOF( "YYYYmmddHHMMSS.uuuuuuZ#SSSSSS#" );
		i < normalized->bv_len; i++ )
	{
		/* assume it's already validated that's all hex digits */
		normalized->bv_val[ i ] = TOLOWER( normalized->bv_val[ i ] );
	}

	return LDAP_SUCCESS;
}

static int
csnPretty(
	Syntax *syntax,
	struct berval *val,
	struct berval *out,
	void *ctx )
{
	return csnNormalize( SLAP_MR_VALUE_OF_SYNTAX, NULL, NULL, val, out, ctx );
}

#ifndef SUPPORT_OBSOLETE_UTC_SYNTAX
/* slight optimization - does not need the start parameter */
#define check_time_syntax(v, start, p, f) (check_time_syntax)(v, p, f)
enum { start = 0 };
#endif

static int
check_time_syntax (struct berval *val,
	int start,
	int *parts,
	struct berval *fraction)
{
	/*
	 * start=0 GeneralizedTime YYYYmmddHH[MM[SS]][(./,)d...](Z|(+/-)HH[MM])
	 * start=1 UTCTime         YYmmddHHMM[SS][Z|(+/-)HHMM]
	 * GeneralizedTime supports leap seconds, UTCTime does not.
	 */
	static const int ceiling[9] = { 100, 100, 12, 31, 24, 60, 60, 24, 60 };
	static const int mdays[2][12] = {
		/* non-leap years */
		{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
		/* leap years */
		{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	};
	char *p, *e;
	int part, c, c1, c2, tzoffset, leapyear = 0;

	p = val->bv_val;
	e = p + val->bv_len;

#ifdef SUPPORT_OBSOLETE_UTC_SYNTAX
	parts[0] = 20; /* century - any multiple of 4 from 04 to 96 */
#endif
	for (part = start; part < 7 && p < e; part++) {
		c1 = *p;
		if (!ASCII_DIGIT(c1)) {
			break;
		}
		p++;
		if (p == e) {
			return LDAP_INVALID_SYNTAX;
		}
		c = *p++;
		if (!ASCII_DIGIT(c)) {
			return LDAP_INVALID_SYNTAX;
		}
		c += c1 * 10 - '0' * 11;
		if ((part | 1) == 3) {
			--c;
			if (c < 0) {
				return LDAP_INVALID_SYNTAX;
			}
		}
		if (c >= ceiling[part]) {
			if (! (c == 60 && part == 6 && start == 0))
				return LDAP_INVALID_SYNTAX;
		}
		parts[part] = c;
	}
	if (part < 5 + start) {
		return LDAP_INVALID_SYNTAX;
	}
	for (; part < 9; part++) {
		parts[part] = 0;
	}

	/* leapyear check for the Gregorian calendar (year>1581) */
	if (parts[parts[1] == 0 ? 0 : 1] % 4 == 0) {
		leapyear = 1;
	}

	if (parts[3] >= mdays[leapyear][parts[2]]) {
		return LDAP_INVALID_SYNTAX;
	}

	if (start == 0) {
		fraction->bv_val = p;
		fraction->bv_len = 0;
		if (p < e && (*p == '.' || *p == ',')) {
			char *end_num;
			while (++p < e && ASCII_DIGIT(*p)) {
				/* EMTPY */;
			}
			if (p - fraction->bv_val == 1) {
				return LDAP_INVALID_SYNTAX;
			}
			for (end_num = p; end_num[-1] == '0'; --end_num) {
				/* EMPTY */;
			}
			c = end_num - fraction->bv_val;
			if (c != 1) fraction->bv_len = c;
		}
	}

	if (p == e) {
		/* no time zone */
		return start == 0 ? LDAP_INVALID_SYNTAX : LDAP_SUCCESS;
	}

	tzoffset = *p++;
	switch (tzoffset) {
	default:
		return LDAP_INVALID_SYNTAX;
	case 'Z':
		/* UTC */
		break;
	case '+':
	case '-':
		for (part = 7; part < 9 && p < e; part++) {
			c1 = *p;
			if (!ASCII_DIGIT(c1)) {
				break;
			}
			p++;
			if (p == e) {
				return LDAP_INVALID_SYNTAX;
			}
			c2 = *p++;
			if (!ASCII_DIGIT(c2)) {
				return LDAP_INVALID_SYNTAX;
			}
			parts[part] = c1 * 10 + c2 - '0' * 11;
			if (parts[part] >= ceiling[part]) {
				return LDAP_INVALID_SYNTAX;
			}
		}
		if (part < 8 + start) {
			return LDAP_INVALID_SYNTAX;
		}

		if (tzoffset == '-') {
			/* negative offset to UTC, ie west of Greenwich */
			parts[4] += parts[7];
			parts[5] += parts[8];
			/* offset is just hhmm, no seconds */
			for (part = 6; --part >= 0; ) {
				if (part != 3) {
					c = ceiling[part];
				} else {
					c = mdays[leapyear][parts[2]];
				}
				if (parts[part] >= c) {
					if (part == 0) {
						return LDAP_INVALID_SYNTAX;
					}
					parts[part] -= c;
					parts[part - 1]++;
					continue;
				} else if (part != 5) {
					break;
				}
			}
		} else {
			/* positive offset to UTC, ie east of Greenwich */
			parts[4] -= parts[7];
			parts[5] -= parts[8];
			for (part = 6; --part >= 0; ) {
				if (parts[part] < 0) {
					if (part == 0) {
						return LDAP_INVALID_SYNTAX;
					}
					if (part != 3) {
						c = ceiling[part];
					} else {
						/* make first arg to % non-negative */
						c = mdays[leapyear][(parts[2] - 1 + 12) % 12];
					}
					parts[part] += c;
					parts[part - 1]--;
					continue;
				} else if (part != 5) {
					break;
				}
			}
		}
	}

	return p != e ? LDAP_INVALID_SYNTAX : LDAP_SUCCESS;
}

#ifdef SUPPORT_OBSOLETE_UTC_SYNTAX

#if 0
static int
xutcTimeNormalize(
	Syntax *syntax,
	struct berval *val,
	struct berval *normalized )
{
	int parts[9], rc;

	rc = check_time_syntax(val, 1, parts, NULL);
	if (rc != LDAP_SUCCESS) {
		return rc;
	}

	normalized->bv_val = ch_malloc( 14 );
	if ( normalized->bv_val == NULL ) {
		return LBER_ERROR_MEMORY;
	}

	sprintf( normalized->bv_val, "%02d%02d%02d%02d%02d%02dZ",
		parts[1], parts[2] + 1, parts[3] + 1,
		parts[4], parts[5], parts[6] );
	normalized->bv_len = 13;

	return LDAP_SUCCESS;
}
#endif /* 0 */

static int
utcTimeValidate(
	Syntax *syntax,
	struct berval *in )
{
	int parts[9];
	return check_time_syntax(in, 1, parts, NULL);
}

#endif /* SUPPORT_OBSOLETE_UTC_SYNTAX */

static int
generalizedTimeValidate(
	Syntax *syntax,
	struct berval *in )
{
	int parts[9];
	struct berval fraction;
	return check_time_syntax(in, 0, parts, &fraction);
}

static int
generalizedTimeNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	int parts[9], rc;
	unsigned int len;
	struct berval fraction;

	rc = check_time_syntax(val, 0, parts, &fraction);
	if (rc != LDAP_SUCCESS) {
		return rc;
	}

	len = STRLENOF("YYYYmmddHHMMSSZ") + fraction.bv_len;
	normalized->bv_val = slap_sl_malloc( len + 1, ctx );
	if ( BER_BVISNULL( normalized ) ) {
		return LBER_ERROR_MEMORY;
	}

	sprintf( normalized->bv_val, "%02d%02d%02d%02d%02d%02d%02d",
		parts[0], parts[1], parts[2] + 1, parts[3] + 1,
		parts[4], parts[5], parts[6] );
	if ( !BER_BVISEMPTY( &fraction ) ) {
		memcpy( normalized->bv_val + STRLENOF("YYYYmmddHHMMSSZ")-1,
			fraction.bv_val, fraction.bv_len );
		normalized->bv_val[STRLENOF("YYYYmmddHHMMSSZ")-1] = '.';
	}
	strcpy( normalized->bv_val + len-1, "Z" );
	normalized->bv_len = len;

	return LDAP_SUCCESS;
}

static int
generalizedTimeOrderingMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	struct berval *asserted = (struct berval *) assertedValue;
	ber_len_t v_len  = value->bv_len;
	ber_len_t av_len = asserted->bv_len;

	/* ignore trailing 'Z' when comparing */
	int match = memcmp( value->bv_val, asserted->bv_val,
		(v_len < av_len ? v_len : av_len) - 1 );
	if ( match == 0 ) match = v_len - av_len;

	/* If used in extensible match filter, match if value < asserted */
	if ( flags & SLAP_MR_EXT )
		match = (match >= 0);

	*matchp = match;
	return LDAP_SUCCESS;
}

/* Index generation function: Ordered index */
int generalizedTimeIndexer(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	BerVarray values,
	BerVarray *keysp,
	void *ctx )
{
	int i, j;
	BerVarray keys;
	char tmp[5];
	BerValue bvtmp; /* 40 bit index */
	struct lutil_tm tm;
	struct lutil_timet tt;

	bvtmp.bv_len = sizeof(tmp);
	bvtmp.bv_val = tmp;
	for( i=0; values[i].bv_val != NULL; i++ ) {
		/* just count them */
	}

	/* we should have at least one value at this point */
	assert( i > 0 );

	keys = slap_sl_malloc( sizeof( struct berval ) * (i+1), ctx );

	/* GeneralizedTime YYYYmmddHH[MM[SS]][(./,)d...](Z|(+/-)HH[MM]) */
	for( i=0, j=0; values[i].bv_val != NULL; i++ ) {
		assert(values[i].bv_val != NULL && values[i].bv_len >= 10);
		/* Use 40 bits of time for key */
		if ( lutil_parsetime( values[i].bv_val, &tm ) == 0 ) {
			lutil_tm2time( &tm, &tt );
			tmp[0] = tt.tt_gsec & 0xff;
			tmp[4] = tt.tt_sec & 0xff;
			tt.tt_sec >>= 8;
			tmp[3] = tt.tt_sec & 0xff;
			tt.tt_sec >>= 8;
			tmp[2] = tt.tt_sec & 0xff;
			tt.tt_sec >>= 8;
			tmp[1] = tt.tt_sec & 0xff;
			
			ber_dupbv_x(&keys[j++], &bvtmp, ctx );
		}
	}

	keys[j].bv_val = NULL;
	keys[j].bv_len = 0;

	*keysp = keys;

	return LDAP_SUCCESS;
}

/* Index generation function: Ordered index */
int generalizedTimeFilter(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	void * assertedValue,
	BerVarray *keysp,
	void *ctx )
{
	BerVarray keys;
	char tmp[5];
	BerValue bvtmp; /* 40 bit index */
	BerValue *value = (BerValue *) assertedValue;
	struct lutil_tm tm;
	struct lutil_timet tt;
	
	bvtmp.bv_len = sizeof(tmp);
	bvtmp.bv_val = tmp;
	/* GeneralizedTime YYYYmmddHH[MM[SS]][(./,)d...](Z|(+/-)HH[MM]) */
	/* Use 40 bits of time for key */
	if ( value->bv_val && value->bv_len >= 10 &&
		lutil_parsetime( value->bv_val, &tm ) == 0 ) {

		lutil_tm2time( &tm, &tt );
		tmp[0] = tt.tt_gsec & 0xff;
		tmp[4] = tt.tt_sec & 0xff;
		tt.tt_sec >>= 8;
		tmp[3] = tt.tt_sec & 0xff;
		tt.tt_sec >>= 8;
		tmp[2] = tt.tt_sec & 0xff;
		tt.tt_sec >>= 8;
		tmp[1] = tt.tt_sec & 0xff;

		keys = slap_sl_malloc( sizeof( struct berval ) * 2, ctx );
		ber_dupbv_x(keys, &bvtmp, ctx );
		keys[1].bv_val = NULL;
		keys[1].bv_len = 0;
	} else {
		keys = NULL;
	}

	*keysp = keys;

	return LDAP_SUCCESS;
}

static int
deliveryMethodValidate(
	Syntax *syntax,
	struct berval *val )
{
#undef LENOF
#define LENOF(s) (sizeof(s)-1)
	struct berval tmp = *val;
	/*
     *	DeliveryMethod = pdm *( WSP DOLLAR WSP DeliveryMethod )
	 *	pdm = "any" / "mhs" / "physical" / "telex" / "teletex" /
	 *		"g3fax" / "g4fax" / "ia5" / "videotex" / "telephone"
	 */
again:
	if( tmp.bv_len < 3 ) return LDAP_INVALID_SYNTAX;

	switch( tmp.bv_val[0] ) {
	case 'a':
	case 'A':
		if(( tmp.bv_len >= LENOF("any") ) &&
			( strncasecmp(tmp.bv_val, "any", LENOF("any")) == 0 ))
		{
			tmp.bv_len -= LENOF("any");
			tmp.bv_val += LENOF("any");
			break;
		}
		return LDAP_INVALID_SYNTAX;

	case 'm':
	case 'M':
		if(( tmp.bv_len >= LENOF("mhs") ) &&
			( strncasecmp(tmp.bv_val, "mhs", LENOF("mhs")) == 0 ))
		{
			tmp.bv_len -= LENOF("mhs");
			tmp.bv_val += LENOF("mhs");
			break;
		}
		return LDAP_INVALID_SYNTAX;

	case 'p':
	case 'P':
		if(( tmp.bv_len >= LENOF("physical") ) &&
			( strncasecmp(tmp.bv_val, "physical", LENOF("physical")) == 0 ))
		{
			tmp.bv_len -= LENOF("physical");
			tmp.bv_val += LENOF("physical");
			break;
		}
		return LDAP_INVALID_SYNTAX;

	case 't':
	case 'T': /* telex or teletex or telephone */
		if(( tmp.bv_len >= LENOF("telex") ) &&
			( strncasecmp(tmp.bv_val, "telex", LENOF("telex")) == 0 ))
		{
			tmp.bv_len -= LENOF("telex");
			tmp.bv_val += LENOF("telex");
			break;
		}
		if(( tmp.bv_len >= LENOF("teletex") ) &&
			( strncasecmp(tmp.bv_val, "teletex", LENOF("teletex")) == 0 ))
		{
			tmp.bv_len -= LENOF("teletex");
			tmp.bv_val += LENOF("teletex");
			break;
		}
		if(( tmp.bv_len >= LENOF("telephone") ) &&
			( strncasecmp(tmp.bv_val, "telephone", LENOF("telephone")) == 0 ))
		{
			tmp.bv_len -= LENOF("telephone");
			tmp.bv_val += LENOF("telephone");
			break;
		}
		return LDAP_INVALID_SYNTAX;

	case 'g':
	case 'G': /* g3fax or g4fax */
		if(( tmp.bv_len >= LENOF("g3fax") ) && (
			( strncasecmp(tmp.bv_val, "g3fax", LENOF("g3fax")) == 0 ) ||
			( strncasecmp(tmp.bv_val, "g4fax", LENOF("g4fax")) == 0 )))
		{
			tmp.bv_len -= LENOF("g3fax");
			tmp.bv_val += LENOF("g3fax");
			break;
		}
		return LDAP_INVALID_SYNTAX;

	case 'i':
	case 'I':
		if(( tmp.bv_len >= LENOF("ia5") ) &&
			( strncasecmp(tmp.bv_val, "ia5", LENOF("ia5")) == 0 ))
		{
			tmp.bv_len -= LENOF("ia5");
			tmp.bv_val += LENOF("ia5");
			break;
		}
		return LDAP_INVALID_SYNTAX;

	case 'v':
	case 'V':
		if(( tmp.bv_len >= LENOF("videotex") ) &&
			( strncasecmp(tmp.bv_val, "videotex", LENOF("videotex")) == 0 ))
		{
			tmp.bv_len -= LENOF("videotex");
			tmp.bv_val += LENOF("videotex");
			break;
		}
		return LDAP_INVALID_SYNTAX;

	default:
		return LDAP_INVALID_SYNTAX;
	}

	if( BER_BVISEMPTY( &tmp ) ) return LDAP_SUCCESS;

	while( !BER_BVISEMPTY( &tmp ) && ( tmp.bv_val[0] == ' ' ) ) {
		tmp.bv_len++;
		tmp.bv_val--;
	}
	if( !BER_BVISEMPTY( &tmp ) && ( tmp.bv_val[0] == '$' ) ) {
		tmp.bv_len++;
		tmp.bv_val--;
	} else {
		return LDAP_INVALID_SYNTAX;
	}
	while( !BER_BVISEMPTY( &tmp ) && ( tmp.bv_val[0] == ' ' ) ) {
		tmp.bv_len++;
		tmp.bv_val--;
	}

	goto again;
}

static int
nisNetgroupTripleValidate(
	Syntax *syntax,
	struct berval *val )
{
	char *p, *e;
	int commas = 0;

	if ( BER_BVISEMPTY( val ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	p = (char *)val->bv_val;
	e = p + val->bv_len;

	if ( *p != '(' /*')'*/ ) {
		return LDAP_INVALID_SYNTAX;
	}

	for ( p++; ( p < e ) && ( *p != /*'('*/ ')' ); p++ ) {
		if ( *p == ',' ) {
			commas++;
			if ( commas > 2 ) {
				return LDAP_INVALID_SYNTAX;
			}

		} else if ( !AD_CHAR( *p ) ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	if ( ( commas != 2 ) || ( *p != /*'('*/ ')' ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	p++;

	if (p != e) {
		return LDAP_INVALID_SYNTAX;
	}

	return LDAP_SUCCESS;
}

static int
bootParameterValidate(
	Syntax *syntax,
	struct berval *val )
{
	char *p, *e;

	if ( BER_BVISEMPTY( val ) ) {
		return LDAP_INVALID_SYNTAX;
	}

	p = (char *)val->bv_val;
	e = p + val->bv_len;

	/* key */
	for (; ( p < e ) && ( *p != '=' ); p++ ) {
		if ( !AD_CHAR( *p ) ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	if ( *p != '=' ) {
		return LDAP_INVALID_SYNTAX;
	}

	/* server */
	for ( p++; ( p < e ) && ( *p != ':' ); p++ ) {
		if ( !AD_CHAR( *p ) ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	if ( *p != ':' ) {
		return LDAP_INVALID_SYNTAX;
	}

	/* path */
	for ( p++; p < e; p++ ) {
		if ( !SLAP_PRINTABLE( *p ) ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	return LDAP_SUCCESS;
}

static int
firstComponentNormalize(
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx )
{
	int rc;
	struct berval comp;
	ber_len_t len;

	if( SLAP_MR_IS_VALUE_OF_ASSERTION_SYNTAX( usage )) {
		ber_dupbv_x( normalized, val, ctx );
		return LDAP_SUCCESS;
	}

	if( val->bv_len < 3 ) return LDAP_INVALID_SYNTAX;

	if( ! ( val->bv_val[0] == '(' /*')'*/
			&& val->bv_val[val->bv_len - 1] == /*'('*/ ')' )
		&& ! ( val->bv_val[0] == '{' /*'}'*/
			&& val->bv_val[val->bv_len - 1] == /*'('*/ '}' ) )
	{
		return LDAP_INVALID_SYNTAX;
	}

	/* trim leading white space */
	for( len=1;
		len < val->bv_len && ASCII_SPACE(val->bv_val[len]);
		len++ )
	{
		/* empty */
	}

	/* grab next word */
	comp.bv_val = &val->bv_val[len];
	len = val->bv_len - len - STRLENOF(/*"{"*/ "}");
	for( comp.bv_len = 0;
		!ASCII_SPACE(comp.bv_val[comp.bv_len]) && comp.bv_len < len;
		comp.bv_len++ )
	{
		/* empty */
	}

	if( mr == slap_schema.si_mr_objectIdentifierFirstComponentMatch ) {
		rc = numericoidValidate( NULL, &comp );
	} else if( mr == slap_schema.si_mr_integerFirstComponentMatch ) {
		rc = integerValidate( NULL, &comp );
	} else {
		rc = LDAP_INVALID_SYNTAX;
	}
	

	if( rc == LDAP_SUCCESS ) {
		ber_dupbv_x( normalized, &comp, ctx );
	}

	return rc;
}

static char *country_gen_syn[] = {
	"1.3.6.1.4.1.1466.115.121.1.15",	/* Directory String */
	"1.3.6.1.4.1.1466.115.121.1.26",	/* IA5 String */
	"1.3.6.1.4.1.1466.115.121.1.44",	/* Printable String */
	NULL
};

#define X_BINARY "X-BINARY-TRANSFER-REQUIRED 'TRUE' "
#define X_NOT_H_R "X-NOT-HUMAN-READABLE 'TRUE' "

static slap_syntax_defs_rec syntax_defs[] = {
	{"( 1.3.6.1.4.1.1466.115.121.1.1 DESC 'ACI Item' "
		X_BINARY X_NOT_H_R ")",
		SLAP_SYNTAX_BINARY|SLAP_SYNTAX_BER, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.2 DESC 'Access Point' " X_NOT_H_R ")",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.3 DESC 'Attribute Type Description' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.4 DESC 'Audio' "
		X_NOT_H_R ")",
		SLAP_SYNTAX_BLOB, NULL, blobValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.5 DESC 'Binary' "
		X_NOT_H_R ")",
		SLAP_SYNTAX_BER, NULL, berValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.6 DESC 'Bit String' )",
		0, NULL, bitStringValidate, NULL },
	{"( 1.3.6.1.4.1.1466.115.121.1.7 DESC 'Boolean' )",
		0, NULL, booleanValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.8 DESC 'Certificate' "
		X_BINARY X_NOT_H_R ")",
		SLAP_SYNTAX_BINARY|SLAP_SYNTAX_BER,
		NULL, certificateValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.9 DESC 'Certificate List' "
		X_BINARY X_NOT_H_R ")",
		SLAP_SYNTAX_BINARY|SLAP_SYNTAX_BER,
		NULL, certificateListValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.10 DESC 'Certificate Pair' "
		X_BINARY X_NOT_H_R ")",
		SLAP_SYNTAX_BINARY|SLAP_SYNTAX_BER,
		NULL, sequenceValidate, NULL},
	{"( " attributeCertificateSyntaxOID " DESC 'X.509 AttributeCertificate' "
		X_BINARY X_NOT_H_R ")",
		SLAP_SYNTAX_BINARY|SLAP_SYNTAX_BER,
		NULL, attributeCertificateValidate, NULL},
#if 0	/* need to go __after__ printableString */
	{"( 1.3.6.1.4.1.1466.115.121.1.11 DESC 'Country String' )",
		0, "1.3.6.1.4.1.1466.115.121.1.44",
		countryStringValidate, NULL},
#endif
	{"( 1.3.6.1.4.1.1466.115.121.1.12 DESC 'Distinguished Name' )",
		SLAP_SYNTAX_DN, NULL, dnValidate, dnPretty},
	{"( 1.2.36.79672281.1.5.0 DESC 'RDN' )",
		0, NULL, rdnValidate, rdnPretty},
#ifdef LDAP_COMP_MATCH
	{"( 1.2.36.79672281.1.5.3 DESC 'allComponents' )",
		0, NULL, allComponentsValidate, NULL},
 	{"( 1.2.36.79672281.1.5.2 DESC 'componentFilterMatch assertion') ",
		0, NULL, componentFilterValidate, NULL},
#endif
	{"( 1.3.6.1.4.1.1466.115.121.1.13 DESC 'Data Quality' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.14 DESC 'Delivery Method' )",
		0, NULL, deliveryMethodValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.15 DESC 'Directory String' )",
		0, NULL, UTF8StringValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.16 DESC 'DIT Content Rule Description' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.17 DESC 'DIT Structure Rule Description' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.19 DESC 'DSA Quality' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.20 DESC 'DSE Type' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.21 DESC 'Enhanced Guide' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.22 DESC 'Facsimile Telephone Number' )",
		0, NULL, printablesStringValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.23 DESC 'Fax' " X_NOT_H_R ")",
		SLAP_SYNTAX_BLOB, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.24 DESC 'Generalized Time' )",
		0, NULL, generalizedTimeValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.25 DESC 'Guide' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.26 DESC 'IA5 String' )",
		0, NULL, IA5StringValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.27 DESC 'Integer' )",
		0, NULL, integerValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.28 DESC 'JPEG' " X_NOT_H_R ")",
		SLAP_SYNTAX_BLOB, NULL, blobValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.29 DESC 'Master And Shadow Access Points' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.30 DESC 'Matching Rule Description' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.31 DESC 'Matching Rule Use Description' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.32 DESC 'Mail Preference' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.33 DESC 'MHS OR Address' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.34 DESC 'Name And Optional UID' )",
		SLAP_SYNTAX_DN, NULL, nameUIDValidate, nameUIDPretty },
	{"( 1.3.6.1.4.1.1466.115.121.1.35 DESC 'Name Form Description' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.36 DESC 'Numeric String' )",
		0, NULL, numericStringValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.37 DESC 'Object Class Description' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.38 DESC 'OID' )",
		0, NULL, numericoidValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.39 DESC 'Other Mailbox' )",
		0, NULL, IA5StringValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.40 DESC 'Octet String' )",
		0, NULL, blobValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.41 DESC 'Postal Address' )",
		0, NULL, postalAddressValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.42 DESC 'Protocol Information' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.43 DESC 'Presentation Address' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.44 DESC 'Printable String' )",
		0, NULL, printableStringValidate, NULL},
	/* moved here because now depends on Directory String, IA5 String 
	 * and Printable String */
	{"( 1.3.6.1.4.1.1466.115.121.1.11 DESC 'Country String' )",
		0, country_gen_syn, countryStringValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.45 DESC 'SubtreeSpecification' )",
#define subtreeSpecificationValidate UTF8StringValidate /* FIXME */
		0, NULL, subtreeSpecificationValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.49 DESC 'Supported Algorithm' "
		X_BINARY X_NOT_H_R ")",
		SLAP_SYNTAX_BINARY|SLAP_SYNTAX_BER, NULL, berValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.50 DESC 'Telephone Number' )",
		0, NULL, printableStringValidate, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.51 DESC 'Teletex Terminal Identifier' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.52 DESC 'Telex Number' )",
		0, NULL, printablesStringValidate, NULL},
#ifdef SUPPORT_OBSOLETE_UTC_SYNTAX
	{"( 1.3.6.1.4.1.1466.115.121.1.53 DESC 'UTC Time' )",
		0, NULL, utcTimeValidate, NULL},
#endif
	{"( 1.3.6.1.4.1.1466.115.121.1.54 DESC 'LDAP Syntax Description' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.55 DESC 'Modify Rights' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.56 DESC 'LDAP Schema Definition' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.57 DESC 'LDAP Schema Description' )",
		0, NULL, NULL, NULL},
	{"( 1.3.6.1.4.1.1466.115.121.1.58 DESC 'Substring Assertion' )",
		0, NULL, NULL, NULL},

	/* RFC 2307 NIS Syntaxes */
	{"( 1.3.6.1.1.1.0.0  DESC 'RFC2307 NIS Netgroup Triple' )",
		0, NULL, nisNetgroupTripleValidate, NULL},
	{"( 1.3.6.1.1.1.0.1  DESC 'RFC2307 Boot Parameter' )",
		0, NULL, bootParameterValidate, NULL},

	/* draft-zeilenga-ldap-x509 */
	{"( 1.3.6.1.1.15.1 DESC 'Certificate Exact Assertion' )",
		SLAP_SYNTAX_HIDE, NULL,
		serialNumberAndIssuerValidate,
		serialNumberAndIssuerPretty},
	{"( 1.3.6.1.1.15.2 DESC 'Certificate Assertion' )",
		SLAP_SYNTAX_HIDE, NULL, NULL, NULL},
	{"( 1.3.6.1.1.15.3 DESC 'Certificate Pair Exact Assertion' )",
		SLAP_SYNTAX_HIDE, NULL, NULL, NULL},
	{"( 1.3.6.1.1.15.4 DESC 'Certificate Pair Assertion' )",
		SLAP_SYNTAX_HIDE, NULL, NULL, NULL},
	{"( 1.3.6.1.1.15.5 DESC 'Certificate List Exact Assertion' )",
		SLAP_SYNTAX_HIDE, NULL,
		issuerAndThisUpdateValidate,
		issuerAndThisUpdatePretty},
	{"( 1.3.6.1.1.15.6 DESC 'Certificate List Assertion' )",
		SLAP_SYNTAX_HIDE, NULL, NULL, NULL},
	{"( 1.3.6.1.1.15.7 DESC 'Algorithm Identifier' )",
		SLAP_SYNTAX_HIDE, NULL, NULL, NULL},
	{"( " attributeCertificateExactAssertionSyntaxOID " DESC 'AttributeCertificate Exact Assertion' )",
		SLAP_SYNTAX_HIDE, NULL,
		serialNumberAndIssuerSerialValidate,
		serialNumberAndIssuerSerialPretty},
	{"( " attributeCertificateAssertionSyntaxOID " DESC 'AttributeCertificate Assertion' )",
		SLAP_SYNTAX_HIDE, NULL, NULL, NULL},

#ifdef SLAPD_AUTHPASSWD
	/* needs updating */
	{"( 1.3.6.1.4.1.4203.666.2.2 DESC 'OpenLDAP authPassword' )",
		SLAP_SYNTAX_HIDE, NULL, NULL, NULL},
#endif

	{"( 1.3.6.1.1.16.1 DESC 'UUID' )",
		0, NULL, UUIDValidate, UUIDPretty},

	{"( 1.3.6.1.4.1.4203.666.11.2.1 DESC 'CSN' )",
		SLAP_SYNTAX_HIDE, NULL, csnValidate, csnPretty },

	{"( 1.3.6.1.4.1.4203.666.11.2.4 DESC 'CSN SID' )",
		SLAP_SYNTAX_HIDE, NULL, sidValidate, sidPretty },

	/* OpenLDAP Void Syntax */
	{"( 1.3.6.1.4.1.4203.1.1.1 DESC 'OpenLDAP void' )" ,
		SLAP_SYNTAX_HIDE, NULL, inValidate, NULL},

	/* FIXME: OID is unused, but not registered yet */
	{"( 1.3.6.1.4.1.4203.666.2.7 DESC 'OpenLDAP authz' )",
		SLAP_SYNTAX_HIDE, NULL, authzValidate, authzPretty},

	{NULL, 0, NULL, NULL, NULL}
};

char *csnSIDMatchSyntaxes[] = {
	"1.3.6.1.4.1.4203.666.11.2.1" /* csn */,
	NULL
};
char *certificateExactMatchSyntaxes[] = {
	"1.3.6.1.4.1.1466.115.121.1.8" /* certificate */,
	NULL
};
char *certificateListExactMatchSyntaxes[] = {
	"1.3.6.1.4.1.1466.115.121.1.9" /* certificateList */,
	NULL
};
char *attributeCertificateExactMatchSyntaxes[] = {
	attributeCertificateSyntaxOID  /* attributeCertificate */,
	NULL
};

#ifdef LDAP_COMP_MATCH
char *componentFilterMatchSyntaxes[] = {
	"1.3.6.1.4.1.1466.115.121.1.8" /* certificate */,
	"1.3.6.1.4.1.1466.115.121.1.9" /* certificateList */,
	attributeCertificateSyntaxOID /* attributeCertificate */,
	NULL
};
#endif

char *directoryStringSyntaxes[] = {
	"1.3.6.1.4.1.1466.115.121.1.11" /* countryString */,
	"1.3.6.1.4.1.1466.115.121.1.44" /* printableString */,
	"1.3.6.1.4.1.1466.115.121.1.50" /* telephoneNumber */,
	NULL
};
char *integerFirstComponentMatchSyntaxes[] = {
	"1.3.6.1.4.1.1466.115.121.1.27" /* INTEGER */,
	"1.3.6.1.4.1.1466.115.121.1.17" /* dITStructureRuleDescription */,
	NULL
};
char *objectIdentifierFirstComponentMatchSyntaxes[] = {
	"1.3.6.1.4.1.1466.115.121.1.38" /* OID */,
	"1.3.6.1.4.1.1466.115.121.1.3"  /* attributeTypeDescription */,
	"1.3.6.1.4.1.1466.115.121.1.16" /* dITContentRuleDescription */,
	"1.3.6.1.4.1.1466.115.121.1.54" /* ldapSyntaxDescription */,
	"1.3.6.1.4.1.1466.115.121.1.30" /* matchingRuleDescription */,
	"1.3.6.1.4.1.1466.115.121.1.31" /* matchingRuleUseDescription */,
	"1.3.6.1.4.1.1466.115.121.1.35" /* nameFormDescription */,
	"1.3.6.1.4.1.1466.115.121.1.37" /* objectClassDescription */,
	NULL
};

/*
 * Other matching rules in X.520 that we do not use (yet):
 *
 * 2.5.13.25	uTCTimeMatch
 * 2.5.13.26	uTCTimeOrderingMatch
 * 2.5.13.31*	directoryStringFirstComponentMatch
 * 2.5.13.32*	wordMatch
 * 2.5.13.33*	keywordMatch
 * 2.5.13.36+	certificatePairExactMatch
 * 2.5.13.37+	certificatePairMatch
 * 2.5.13.40+	algorithmIdentifierMatch
 * 2.5.13.41*	storedPrefixMatch
 * 2.5.13.42	attributeCertificateMatch
 * 2.5.13.43	readerAndKeyIDMatch
 * 2.5.13.44	attributeIntegrityMatch
 *
 * (*) described in RFC 3698 (LDAP: Additional Matching Rules)
 * (+) described in draft-zeilenga-ldap-x509
 */
static slap_mrule_defs_rec mrule_defs[] = {
	/*
	 * EQUALITY matching rules must be listed after associated APPROX
	 * matching rules.  So, we list all APPROX matching rules first.
	 */
	{"( " directoryStringApproxMatchOID " NAME 'directoryStringApproxMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 )",
		SLAP_MR_HIDE | SLAP_MR_EQUALITY_APPROX | SLAP_MR_EXT, NULL,
		NULL, NULL, directoryStringApproxMatch,
		directoryStringApproxIndexer, directoryStringApproxFilter,
		NULL},

	{"( " IA5StringApproxMatchOID " NAME 'IA5StringApproxMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.26 )",
		SLAP_MR_HIDE | SLAP_MR_EQUALITY_APPROX | SLAP_MR_EXT, NULL,
		NULL, NULL, IA5StringApproxMatch,
		IA5StringApproxIndexer, IA5StringApproxFilter,
		NULL},

	/*
	 * Other matching rules
	 */
	
	{"( 2.5.13.0 NAME 'objectIdentifierMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, NULL, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.1 NAME 'distinguishedNameMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, dnNormalize, dnMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 1.3.6.1.4.1.4203.666.4.9 NAME 'dnSubtreeMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 )",
		SLAP_MR_HIDE | SLAP_MR_EXT, NULL,
		NULL, dnNormalize, dnRelativeMatch,
		NULL, NULL,
		NULL },

	{"( 1.3.6.1.4.1.4203.666.4.8 NAME 'dnOneLevelMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 )",
		SLAP_MR_HIDE | SLAP_MR_EXT, NULL,
		NULL, dnNormalize, dnRelativeMatch,
		NULL, NULL,
		NULL },

	{"( 1.3.6.1.4.1.4203.666.4.10 NAME 'dnSubordinateMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 )",
		SLAP_MR_HIDE | SLAP_MR_EXT, NULL,
		NULL, dnNormalize, dnRelativeMatch,
		NULL, NULL,
		NULL },

	{"( 1.3.6.1.4.1.4203.666.4.11 NAME 'dnSuperiorMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 )",
		SLAP_MR_HIDE | SLAP_MR_EXT, NULL,
		NULL, dnNormalize, dnRelativeMatch,
		NULL, NULL,
		NULL },

	{"( 1.2.36.79672281.1.13.3 NAME 'rdnMatch' "
		"SYNTAX 1.2.36.79672281.1.5.0 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, rdnNormalize, rdnMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

#ifdef LDAP_COMP_MATCH
	{"( 1.2.36.79672281.1.13.2 NAME 'componentFilterMatch' "
		"SYNTAX 1.2.36.79672281.1.5.2 )", /* componentFilterMatch assertion */
		SLAP_MR_EXT|SLAP_MR_COMPONENT, componentFilterMatchSyntaxes,
		NULL, NULL , componentFilterMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

        {"( 1.2.36.79672281.1.13.6 NAME 'allComponentsMatch' "
                "SYNTAX 1.2.36.79672281.1.5.3 )", /* allComponents */
                SLAP_MR_EQUALITY|SLAP_MR_EXT|SLAP_MR_COMPONENT, NULL,
                NULL, NULL , allComponentsMatch,
                octetStringIndexer, octetStringFilter,
                NULL },

        {"( 1.2.36.79672281.1.13.7 NAME 'directoryComponentsMatch' "
                "SYNTAX 1.2.36.79672281.1.5.3 )", /* allComponents */
                SLAP_MR_EQUALITY|SLAP_MR_EXT|SLAP_MR_COMPONENT, NULL,
                NULL, NULL , directoryComponentsMatch,
                octetStringIndexer, octetStringFilter,
                NULL },
#endif

	{"( 2.5.13.2 NAME 'caseIgnoreMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, directoryStringSyntaxes,
		NULL, UTF8StringNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		directoryStringApproxMatchOID },

	{"( 2.5.13.3 NAME 'caseIgnoreOrderingMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 )",
		SLAP_MR_ORDERING | SLAP_MR_EXT, directoryStringSyntaxes,
		NULL, UTF8StringNormalize, octetStringOrderingMatch,
		NULL, NULL,
		"caseIgnoreMatch" },

	{"( 2.5.13.4 NAME 'caseIgnoreSubstringsMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.58 )", /* Substring Assertion */
		SLAP_MR_SUBSTR, directoryStringSyntaxes,
		NULL, UTF8StringNormalize, directoryStringSubstringsMatch,
		octetStringSubstringsIndexer, octetStringSubstringsFilter,
		"caseIgnoreMatch" },

	{"( 2.5.13.5 NAME 'caseExactMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, directoryStringSyntaxes,
		NULL, UTF8StringNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		directoryStringApproxMatchOID },

	{"( 2.5.13.6 NAME 'caseExactOrderingMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 )",
		SLAP_MR_ORDERING | SLAP_MR_EXT, directoryStringSyntaxes,
		NULL, UTF8StringNormalize, octetStringOrderingMatch,
		NULL, NULL,
		"caseExactMatch" },

	{"( 2.5.13.7 NAME 'caseExactSubstringsMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.58 )", /* Substring Assertion */
		SLAP_MR_SUBSTR, directoryStringSyntaxes,
		NULL, UTF8StringNormalize, directoryStringSubstringsMatch,
		octetStringSubstringsIndexer, octetStringSubstringsFilter,
		"caseExactMatch" },

	{"( 2.5.13.8 NAME 'numericStringMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.36 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, numericStringNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.9 NAME 'numericStringOrderingMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.36 )",
		SLAP_MR_ORDERING | SLAP_MR_EXT, NULL,
		NULL, numericStringNormalize, octetStringOrderingMatch,
		NULL, NULL,
		"numericStringMatch" },

	{"( 2.5.13.10 NAME 'numericStringSubstringsMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.58 )", /* Substring Assertion */
		SLAP_MR_SUBSTR, NULL,
		NULL, numericStringNormalize, octetStringSubstringsMatch,
		octetStringSubstringsIndexer, octetStringSubstringsFilter,
		"numericStringMatch" },

	{"( 2.5.13.11 NAME 'caseIgnoreListMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.41 )", /* Postal Address */
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, postalAddressNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.12 NAME 'caseIgnoreListSubstringsMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.58 )", /* Substring Assertion */
		SLAP_MR_SUBSTR, NULL,
		NULL, NULL, NULL, NULL, NULL,
		"caseIgnoreListMatch" },

	{"( 2.5.13.13 NAME 'booleanMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.7 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, NULL, booleanMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.14 NAME 'integerMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT | SLAP_MR_ORDERED_INDEX, NULL,
		NULL, NULL, integerMatch,
		integerIndexer, integerFilter,
		NULL },

	{"( 2.5.13.15 NAME 'integerOrderingMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 )",
		SLAP_MR_ORDERING | SLAP_MR_EXT | SLAP_MR_ORDERED_INDEX, NULL,
		NULL, NULL, integerMatch,
		NULL, NULL,
		"integerMatch" },

	{"( 2.5.13.16 NAME 'bitStringMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.6 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, NULL, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.17 NAME 'octetStringMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, NULL, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.18 NAME 'octetStringOrderingMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40 )",
		SLAP_MR_ORDERING | SLAP_MR_EXT, NULL,
		NULL, NULL, octetStringOrderingMatch,
		NULL, NULL,
		"octetStringMatch" },

	{"( 2.5.13.19 NAME 'octetStringSubstringsMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40 )",
		SLAP_MR_SUBSTR, NULL,
		NULL, NULL, octetStringSubstringsMatch,
		octetStringSubstringsIndexer, octetStringSubstringsFilter,
		"octetStringMatch" },

	{"( 2.5.13.20 NAME 'telephoneNumberMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.50 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL,
		telephoneNumberNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.21 NAME 'telephoneNumberSubstringsMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.58 )", /* Substring Assertion */
		SLAP_MR_SUBSTR, NULL,
		NULL, telephoneNumberNormalize, octetStringSubstringsMatch,
		octetStringSubstringsIndexer, octetStringSubstringsFilter,
		"telephoneNumberMatch" },

	{"( 2.5.13.22 NAME 'presentationAddressMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.43 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL },

	{"( 2.5.13.23 NAME 'uniqueMemberMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.34 )", /* Name And Optional UID */
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, uniqueMemberNormalize, uniqueMemberMatch,
		uniqueMemberIndexer, uniqueMemberFilter,
		NULL },

	{"( 2.5.13.24 NAME 'protocolInformationMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.42 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL },

	{"( 2.5.13.27 NAME 'generalizedTimeMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT | SLAP_MR_ORDERED_INDEX, NULL,
		NULL, generalizedTimeNormalize, octetStringMatch,
		generalizedTimeIndexer, generalizedTimeFilter,
		NULL },

	{"( 2.5.13.28 NAME 'generalizedTimeOrderingMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 )",
		SLAP_MR_ORDERING | SLAP_MR_EXT | SLAP_MR_ORDERED_INDEX, NULL,
		NULL, generalizedTimeNormalize, generalizedTimeOrderingMatch,
		NULL, NULL,
		"generalizedTimeMatch" },

	{"( 2.5.13.29 NAME 'integerFirstComponentMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 )", /* Integer */
		SLAP_MR_EQUALITY | SLAP_MR_EXT,
			integerFirstComponentMatchSyntaxes,
		NULL, firstComponentNormalize, integerMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.30 NAME 'objectIdentifierFirstComponentMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 )", /* OID */
		SLAP_MR_EQUALITY | SLAP_MR_EXT,
			objectIdentifierFirstComponentMatchSyntaxes,
		NULL, firstComponentNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.34 NAME 'certificateExactMatch' "
		"SYNTAX 1.3.6.1.1.15.1 )", /* Certificate Exact Assertion */
		SLAP_MR_EQUALITY | SLAP_MR_EXT, certificateExactMatchSyntaxes,
		NULL, certificateExactNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.35 NAME 'certificateMatch' "
		"SYNTAX 1.3.6.1.1.15.2 )", /* Certificate Assertion */
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, NULL, NULL, NULL, NULL,
		NULL },

	{"( 2.5.13.38 NAME 'certificateListExactMatch' "
		"SYNTAX 1.3.6.1.1.15.5 )", /* Certificate List Exact Assertion */
		SLAP_MR_EQUALITY | SLAP_MR_EXT, certificateListExactMatchSyntaxes,
		NULL, certificateListExactNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.39 NAME 'certificateListMatch' "
		"SYNTAX 1.3.6.1.1.15.6 )", /* Certificate List Assertion */
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, NULL, NULL, NULL, NULL,
		NULL },

	{"( 2.5.13.45 NAME 'attributeCertificateExactMatch' "
		"SYNTAX " attributeCertificateExactAssertionSyntaxOID " )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT | SLAP_MR_HIDE, attributeCertificateExactMatchSyntaxes,
		NULL, attributeCertificateExactNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	{"( 2.5.13.46 NAME 'attributeCertificateMatch' "
		"SYNTAX " attributeCertificateAssertionSyntaxOID " )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT | SLAP_MR_HIDE, NULL,
		NULL, NULL, NULL, NULL, NULL,
		NULL },

	{"( 1.3.6.1.4.1.1466.109.114.1 NAME 'caseExactIA5Match' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.26 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, IA5StringNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		IA5StringApproxMatchOID },

	{"( 1.3.6.1.4.1.1466.109.114.2 NAME 'caseIgnoreIA5Match' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.26 )",
		SLAP_MR_EQUALITY | SLAP_MR_EXT, NULL,
		NULL, IA5StringNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		IA5StringApproxMatchOID },

	{"( 1.3.6.1.4.1.1466.109.114.3 NAME 'caseIgnoreIA5SubstringsMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.26 )",
		SLAP_MR_SUBSTR, NULL,
		NULL, IA5StringNormalize, directoryStringSubstringsMatch,
		octetStringSubstringsIndexer, octetStringSubstringsFilter,
		"caseIgnoreIA5Match" },

	{"( 1.3.6.1.4.1.4203.1.2.1 NAME 'caseExactIA5SubstringsMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.26 )",
		SLAP_MR_SUBSTR, NULL,
		NULL, IA5StringNormalize, directoryStringSubstringsMatch,
		octetStringSubstringsIndexer, octetStringSubstringsFilter,
		"caseExactIA5Match" },

#ifdef SLAPD_AUTHPASSWD
	/* needs updating */
	{"( 1.3.6.1.4.1.4203.666.4.1 NAME 'authPasswordMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40 )", /* Octet String */
		SLAP_MR_HIDE | SLAP_MR_EQUALITY, NULL,
		NULL, NULL, authPasswordMatch,
		NULL, NULL,
		NULL},
#endif

	{"( 1.2.840.113556.1.4.803 NAME 'integerBitAndMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 )", /* Integer */
		SLAP_MR_EXT, NULL,
		NULL, NULL, integerBitAndMatch,
		NULL, NULL,
		"integerMatch" },

	{"( 1.2.840.113556.1.4.804 NAME 'integerBitOrMatch' "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 )", /* Integer */
		SLAP_MR_EXT, NULL,
		NULL, NULL, integerBitOrMatch,
		NULL, NULL,
		"integerMatch" },

	{"( 1.3.6.1.1.16.2 NAME 'UUIDMatch' "
		"SYNTAX 1.3.6.1.1.16.1 )",
		SLAP_MR_EQUALITY | SLAP_MR_MUTATION_NORMALIZER, NULL,
		NULL, UUIDNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL},

	{"( 1.3.6.1.1.16.3 NAME 'UUIDOrderingMatch' "
		"SYNTAX 1.3.6.1.1.16.1 )",
		SLAP_MR_ORDERING | SLAP_MR_MUTATION_NORMALIZER, NULL,
		NULL, UUIDNormalize, octetStringOrderingMatch,
		octetStringIndexer, octetStringFilter,
		"UUIDMatch"},

	{"( 1.3.6.1.4.1.4203.666.11.2.2 NAME 'CSNMatch' "
		"SYNTAX 1.3.6.1.4.1.4203.666.11.2.1 )",
		SLAP_MR_HIDE | SLAP_MR_EQUALITY | SLAP_MR_ORDERED_INDEX, NULL,
		NULL, csnNormalize, csnMatch,
		csnIndexer, csnFilter,
		NULL},

	{"( 1.3.6.1.4.1.4203.666.11.2.3 NAME 'CSNOrderingMatch' "
		"SYNTAX 1.3.6.1.4.1.4203.666.11.2.1 )",
		SLAP_MR_HIDE | SLAP_MR_ORDERING | SLAP_MR_EXT | SLAP_MR_ORDERED_INDEX, NULL,
		NULL, csnNormalize, csnOrderingMatch,
		NULL, NULL,
		"CSNMatch" },

	{"( 1.3.6.1.4.1.4203.666.11.2.5 NAME 'CSNSIDMatch' "
		"SYNTAX 1.3.6.1.4.1.4203.666.11.2.4 )",
		SLAP_MR_HIDE | SLAP_MR_EQUALITY | SLAP_MR_EXT, csnSIDMatchSyntaxes,
		NULL, csnSidNormalize, octetStringMatch,
		octetStringIndexer, octetStringFilter,
		NULL },

	/* FIXME: OID is unused, but not registered yet */
	{"( 1.3.6.1.4.1.4203.666.4.12 NAME 'authzMatch' "
		"SYNTAX 1.3.6.1.4.1.4203.666.2.7 )", /* OpenLDAP authz */
		SLAP_MR_HIDE | SLAP_MR_EQUALITY, NULL,
		NULL, authzNormalize, authzMatch,
		NULL, NULL,
		NULL},

	{NULL, SLAP_MR_NONE, NULL,
		NULL, NULL, NULL, NULL, NULL,
		NULL }
};

int
slap_schema_init( void )
{
	int		res;
	int		i;

	/* we should only be called once (from main) */
	assert( schema_init_done == 0 );

	for ( i=0; syntax_defs[i].sd_desc != NULL; i++ ) {
		res = register_syntax( &syntax_defs[i] );

		if ( res ) {
			fprintf( stderr, "slap_schema_init: Error registering syntax %s\n",
				 syntax_defs[i].sd_desc );
			return LDAP_OTHER;
		}
	}

	for ( i=0; mrule_defs[i].mrd_desc != NULL; i++ ) {
		if( mrule_defs[i].mrd_usage == SLAP_MR_NONE &&
			mrule_defs[i].mrd_compat_syntaxes == NULL )
		{
			fprintf( stderr,
				"slap_schema_init: Ignoring unusable matching rule %s\n",
				 mrule_defs[i].mrd_desc );
			continue;
		}

		res = register_matching_rule( &mrule_defs[i] );

		if ( res ) {
			fprintf( stderr,
				"slap_schema_init: Error registering matching rule %s\n",
				 mrule_defs[i].mrd_desc );
			return LDAP_OTHER;
		}
	}

	res = slap_schema_load();
	schema_init_done = 1;
	return res;
}

void
schema_destroy( void )
{
	oidm_destroy();
	oc_destroy();
	at_destroy();
	mr_destroy();
	mru_destroy();
	syn_destroy();

	if( schema_init_done ) {
		ldap_pvt_thread_mutex_destroy( &ad_index_mutex );
		ldap_pvt_thread_mutex_destroy( &ad_undef_mutex );
		ldap_pvt_thread_mutex_destroy( &oc_undef_mutex );
	}
}
