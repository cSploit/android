/* schema_prep.c - load builtin schema */
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

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"

#define OCDEBUG 0

int schema_init_done = 0;

struct slap_internal_schema slap_schema;

static int
oidValidate(
	Syntax *syntax,
	struct berval *in )
{
	struct berval val = *in;

	if( val.bv_len == 0 ) {
		/* disallow empty strings */
		return LDAP_INVALID_SYNTAX;
	}

	if( DESC_LEADCHAR( val.bv_val[0] ) ) {
		val.bv_val++;
		val.bv_len--;
		if ( val.bv_len == 0 ) return LDAP_SUCCESS;

		while( DESC_CHAR( val.bv_val[0] ) ) {
			val.bv_val++;
			val.bv_len--;

			if ( val.bv_len == 0 ) return LDAP_SUCCESS;
		}

	} else {
		int sep = 0;
		while( OID_LEADCHAR( val.bv_val[0] ) ) {
			val.bv_val++;
			val.bv_len--;

			if ( val.bv_val[-1] != '0' ) {
				while ( OID_LEADCHAR( val.bv_val[0] )) {
					val.bv_val++;
					val.bv_len--;
				}
			}

			if( val.bv_len == 0 ) {
				if( sep == 0 ) break;
				return LDAP_SUCCESS;
			}

			if( !OID_SEPARATOR( val.bv_val[0] )) break;

			sep++;
			val.bv_val++;
			val.bv_len--;
		}
	}

	return LDAP_INVALID_SYNTAX;
}


static int objectClassPretty(
	Syntax *syntax,
	struct berval *in,
	struct berval *out,
	void *ctx )
{
	ObjectClass *oc;

	if( oidValidate( NULL, in )) return LDAP_INVALID_SYNTAX;

	oc = oc_bvfind( in );
	if( oc == NULL ) return LDAP_INVALID_SYNTAX;

	ber_dupbv_x( out, &oc->soc_cname, ctx );
	return LDAP_SUCCESS;
}

static int
attributeTypeMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	struct berval *a = (struct berval *) assertedValue;
	AttributeType *at = at_bvfind( value );
	AttributeType *asserted = at_bvfind( a );

	if( asserted == NULL ) {
		if( OID_LEADCHAR( *a->bv_val ) ) {
			/* OID form, return FALSE */
			*matchp = 1;
			return LDAP_SUCCESS;
		}

		/* desc form, return undefined */
		return LDAP_INVALID_SYNTAX;
	}

	if ( at == NULL ) {
		/* unrecognized stored value */
		return LDAP_INVALID_SYNTAX;
	}

	*matchp = ( asserted != at );
	return LDAP_SUCCESS;
}

static int
matchingRuleMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	struct berval *a = (struct berval *) assertedValue;
	MatchingRule *mrv = mr_bvfind( value );
	MatchingRule *asserted = mr_bvfind( a );

	if( asserted == NULL ) {
		if( OID_LEADCHAR( *a->bv_val ) ) {
			/* OID form, return FALSE */
			*matchp = 1;
			return LDAP_SUCCESS;
		}

		/* desc form, return undefined */
		return LDAP_INVALID_SYNTAX;
	}

	if ( mrv == NULL ) {
		/* unrecognized stored value */
		return LDAP_INVALID_SYNTAX;
	}

	*matchp = ( asserted != mrv );
	return LDAP_SUCCESS;
}

static int
objectClassMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	struct berval *a = (struct berval *) assertedValue;
	ObjectClass *oc = oc_bvfind( value );
	ObjectClass *asserted = oc_bvfind( a );

	if( asserted == NULL ) {
		if( OID_LEADCHAR( *a->bv_val ) ) {
			/* OID form, return FALSE */
			*matchp = 1;
			return LDAP_SUCCESS;
		}

		/* desc form, return undefined */
		return LDAP_INVALID_SYNTAX;
	}

	if ( oc == NULL ) {
		/* unrecognized stored value */
		return LDAP_INVALID_SYNTAX;
	}

	*matchp = ( asserted != oc );
	return LDAP_SUCCESS;
}

static int
objectSubClassMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	struct berval *a = (struct berval *) assertedValue;
	ObjectClass *oc = oc_bvfind( value );
	ObjectClass *asserted = oc_bvfind( a );

	if( asserted == NULL ) {
		if( OID_LEADCHAR( *a->bv_val ) ) {
			/* OID form, return FALSE */
			*matchp = 1;
			return LDAP_SUCCESS;
		}

		/* desc form, return undefined */
		return LDAP_INVALID_SYNTAX;
	}

	if ( oc == NULL ) {
		/* unrecognized stored value */
		return LDAP_INVALID_SYNTAX;
	}

	if( SLAP_MR_IS_VALUE_OF_ATTRIBUTE_SYNTAX( flags ) ) {
		*matchp = ( asserted != oc );
	} else {
		*matchp = !is_object_subclass( asserted, oc );
	}

	return LDAP_SUCCESS;
}

static int objectSubClassIndexer( 
	slap_mask_t use,
	slap_mask_t mask,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	BerVarray values,
	BerVarray *keysp,
	void *ctx )
{
	int rc, noc, i;
	BerVarray ocvalues;
	ObjectClass **socs;
	
	for( noc=0; values[noc].bv_val != NULL; noc++ ) {
		/* just count em */;
	}

	/* over allocate */
	socs = slap_sl_malloc( (noc+16) * sizeof( ObjectClass * ), ctx );

	/* initialize */
	for( i=0; i<noc; i++ ) {
		socs[i] = oc_bvfind( &values[i] );
	}

	/* expand values */
	for( i=0; i<noc; i++ ) {
		int j;
		ObjectClass *oc = socs[i];
		if( oc == NULL || oc->soc_sups == NULL ) continue;
		
		for( j=0; oc->soc_sups[j] != NULL; j++ ) {
			int found = 0;
			ObjectClass *sup = oc->soc_sups[j];
			int k;

			for( k=0; k<noc; k++ ) {
				if( sup == socs[k] ) {
					found++;
					break;
				}
			}

			if( !found ) {
				socs = slap_sl_realloc( socs,
					sizeof( ObjectClass * ) * (noc+2), ctx );

				assert( k == noc );
				socs[noc++] = sup;
			}
		}
	}

	ocvalues = slap_sl_malloc( sizeof( struct berval ) * (noc+1), ctx );
	/* copy values */
	for( i=0; i<noc; i++ ) {
		if ( socs[i] )
			ocvalues[i] = socs[i]->soc_cname;
		else
			ocvalues[i] = values[i];
	}
	BER_BVZERO( &ocvalues[i] );

	rc = octetStringIndexer( use, mask, syntax, mr,
		prefix, ocvalues, keysp, ctx );

	slap_sl_free( ocvalues, ctx );
	slap_sl_free( socs, ctx );
	return rc;
}

#define objectSubClassFilter octetStringFilter

static ObjectClassSchemaCheckFN rootDseObjectClass;
static ObjectClassSchemaCheckFN aliasObjectClass;
static ObjectClassSchemaCheckFN referralObjectClass;
static ObjectClassSchemaCheckFN subentryObjectClass;
#ifdef LDAP_DYNAMIC_OBJECTS
static ObjectClassSchemaCheckFN dynamicObjectClass;
#endif

static struct slap_schema_oc_map {
	char *ssom_name;
	char *ssom_defn;
	ObjectClassSchemaCheckFN *ssom_check;
	slap_mask_t ssom_flags;
	size_t ssom_offset;
} oc_map[] = {
	{ "top", "( 2.5.6.0 NAME 'top' "
			"DESC 'top of the superclass chain' "
			"ABSTRACT MUST objectClass )",
		0, 0, offsetof(struct slap_internal_schema, si_oc_top) },
	{ "extensibleObject", "( 1.3.6.1.4.1.1466.101.120.111 "
			"NAME 'extensibleObject' "
			"DESC 'RFC4512: extensible object' "
			"SUP top AUXILIARY )",
		0, SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_extensibleObject) },
	{ "alias", "( 2.5.6.1 NAME 'alias' "
			"DESC 'RFC4512: an alias' "
			"SUP top STRUCTURAL "
			"MUST aliasedObjectName )",
		aliasObjectClass, SLAP_OC_ALIAS|SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_alias) },
	{ "referral", "( 2.16.840.1.113730.3.2.6 NAME 'referral' "
			"DESC 'namedref: named subordinate referral' "
			"SUP top STRUCTURAL MUST ref )",
		referralObjectClass, SLAP_OC_REFERRAL|SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_referral) },
	{ "LDAProotDSE", "( 1.3.6.1.4.1.4203.1.4.1 "
			"NAME ( 'OpenLDAProotDSE' 'LDAProotDSE' ) "
			"DESC 'OpenLDAP Root DSE object' "
			"SUP top STRUCTURAL MAY cn )",
		rootDseObjectClass, SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_rootdse) },
	{ "subentry", "( 2.5.17.0 NAME 'subentry' "
			"DESC 'RFC3672: subentry' "
			"SUP top STRUCTURAL "
			"MUST ( cn $ subtreeSpecification ) )",
		subentryObjectClass, SLAP_OC_SUBENTRY|SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_subentry) },
	{ "subschema", "( 2.5.20.1 NAME 'subschema' "
		"DESC 'RFC4512: controlling subschema (sub)entry' "
		"AUXILIARY "
		"MAY ( dITStructureRules $ nameForms $ dITContentRules $ "
			"objectClasses $ attributeTypes $ matchingRules $ "
			"matchingRuleUse ) )",
		subentryObjectClass, SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_subschema) },
#ifdef LDAP_COLLECTIVE_ATTRIBUTES
	{ "collectiveAttributeSubentry", "( 2.5.17.2 "
			"NAME 'collectiveAttributeSubentry' "
			"DESC 'RFC3671: collective attribute subentry' "
			"AUXILIARY )",
		subentryObjectClass,
		SLAP_OC_COLLECTIVEATTRIBUTESUBENTRY|SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
		offsetof( struct slap_internal_schema,
			si_oc_collectiveAttributeSubentry) },
#endif
#ifdef LDAP_DYNAMIC_OBJECTS
	{ "dynamicObject", "( 1.3.6.1.4.1.1466.101.119.2 "
			"NAME 'dynamicObject' "
			"DESC 'RFC2589: Dynamic Object' "
			"SUP top AUXILIARY )",
		dynamicObjectClass, SLAP_OC_DYNAMICOBJECT,
		offsetof(struct slap_internal_schema, si_oc_dynamicObject) },
#endif
	{ "glue", "( 1.3.6.1.4.1.4203.666.3.4 "
			"NAME 'glue' "
			"DESC 'Glue Entry' "
			"SUP top STRUCTURAL )",
		0, SLAP_OC_GLUE|SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
		offsetof(struct slap_internal_schema, si_oc_glue) },
	{ "syncConsumerSubentry", "( 1.3.6.1.4.1.4203.666.3.5 "
			"NAME 'syncConsumerSubentry' "
			"DESC 'Persistent Info for SyncRepl Consumer' "
			"AUXILIARY "
			"MAY syncreplCookie )",
		0, SLAP_OC_SYNCCONSUMERSUBENTRY|SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
		offsetof(struct slap_internal_schema, si_oc_syncConsumerSubentry) },
	{ "syncProviderSubentry", "( 1.3.6.1.4.1.4203.666.3.6 "
			"NAME 'syncProviderSubentry' "
			"DESC 'Persistent Info for SyncRepl Producer' "
			"AUXILIARY "
			"MAY contextCSN )",
		0, SLAP_OC_SYNCPROVIDERSUBENTRY|SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
		offsetof(struct slap_internal_schema, si_oc_syncProviderSubentry) },

	{ NULL, NULL, NULL, 0, 0 }
};

static AttributeTypeSchemaCheckFN rootDseAttribute;
static AttributeTypeSchemaCheckFN aliasAttribute;
static AttributeTypeSchemaCheckFN referralAttribute;
static AttributeTypeSchemaCheckFN subentryAttribute;
static AttributeTypeSchemaCheckFN administrativeRoleAttribute;
#ifdef LDAP_DYNAMIC_OBJECTS
static AttributeTypeSchemaCheckFN dynamicAttribute;
#endif

static struct slap_schema_ad_map {
	char *ssam_name;
	char *ssam_defn;
	AttributeTypeSchemaCheckFN *ssam_check;
	slap_mask_t ssam_flags;
	slap_syntax_validate_func *ssam_syn_validate;
	slap_syntax_transform_func *ssam_syn_pretty;
	slap_mr_convert_func *ssam_mr_convert;
	slap_mr_normalize_func *ssam_mr_normalize;
	slap_mr_match_func *ssam_mr_match;
	slap_mr_indexer_func *ssam_mr_indexer;
	slap_mr_filter_func *ssam_mr_filter;
	size_t ssam_offset;
} ad_map[] = {
	{ "objectClass", "( 2.5.4.0 NAME 'objectClass' "
			"DESC 'RFC4512: object classes of the entity' "
			"EQUALITY objectIdentifierMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 )",
		NULL, SLAP_AT_FINAL,
		oidValidate, objectClassPretty,
		NULL, NULL, objectSubClassMatch,
			objectSubClassIndexer, objectSubClassFilter,
		offsetof(struct slap_internal_schema, si_ad_objectClass) },

	/* user entry operational attributes */
	{ "structuralObjectClass", "( 2.5.21.9 NAME 'structuralObjectClass' "
			"DESC 'RFC4512: structural object class of entry' "
			"EQUALITY objectIdentifierMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, 0,
		oidValidate, objectClassPretty,
		NULL, NULL, objectSubClassMatch,
			objectSubClassIndexer, objectSubClassFilter,
		offsetof(struct slap_internal_schema, si_ad_structuralObjectClass) },
	{ "createTimestamp", "( 2.5.18.1 NAME 'createTimestamp' "
			"DESC 'RFC4512: time which object was created' "
			"EQUALITY generalizedTimeMatch "
			"ORDERING generalizedTimeOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_MANAGEABLE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_createTimestamp) },
	{ "modifyTimestamp", "( 2.5.18.2 NAME 'modifyTimestamp' "
			"DESC 'RFC4512: time which object was last modified' "
			"EQUALITY generalizedTimeMatch "
			"ORDERING generalizedTimeOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_MANAGEABLE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_modifyTimestamp) },
	{ "creatorsName", "( 2.5.18.3 NAME 'creatorsName' "
			"DESC 'RFC4512: name of creator' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_MANAGEABLE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_creatorsName) },
	{ "modifiersName", "( 2.5.18.4 NAME 'modifiersName' "
			"DESC 'RFC4512: name of last modifier' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_MANAGEABLE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_modifiersName) },
	{ "hasSubordinates", "( 2.5.18.9 NAME 'hasSubordinates' "
			"DESC 'X.501: entry has children' "
			"EQUALITY booleanMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.7 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_DYNAMIC,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_hasSubordinates) },
	{ "subschemaSubentry", "( 2.5.18.10 NAME 'subschemaSubentry' "
			"DESC 'RFC4512: name of controlling subschema entry' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 SINGLE-VALUE "
			"NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_DYNAMIC,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_subschemaSubentry) },
#ifdef LDAP_COLLECTIVE_ATTRIBUTES
	{ "collectiveAttributeSubentries", "( 2.5.18.12 "
			"NAME 'collectiveAttributeSubentries' "
			"DESC 'RFC3671: collective attribute subentries' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_collectiveSubentries) },
	{ "collectiveExclusions", "( 2.5.18.7 NAME 'collectiveExclusions' "
			"DESC 'RFC3671: collective attribute exclusions' "
			"EQUALITY objectIdentifierMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 "
			"USAGE directoryOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_collectiveExclusions) },
#endif

	{ "entryDN", "( 1.3.6.1.1.20 NAME 'entryDN' "   
			"DESC 'DN of the entry' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_DYNAMIC,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_entryDN) },
	{ "entryUUID", "( 1.3.6.1.1.16.4 NAME 'entryUUID' "   
			"DESC 'UUID of the entry' "
			"EQUALITY UUIDMatch "
			"ORDERING UUIDOrderingMatch "
			"SYNTAX 1.3.6.1.1.16.1 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_MANAGEABLE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_entryUUID) },
	{ "entryCSN", "( 1.3.6.1.4.1.4203.666.1.7 NAME 'entryCSN' "
			"DESC 'change sequence number of the entry content' "
			"EQUALITY CSNMatch "
			"ORDERING CSNOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.4203.666.11.2.1{64} "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_entryCSN) },
	{ "namingCSN", "( 1.3.6.1.4.1.4203.666.1.13 NAME 'namingCSN' "
			"DESC 'change sequence number of the entry naming (RDN)' "
			"EQUALITY CSNMatch "
			"ORDERING CSNOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.4203.666.11.2.1{64} "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_namingCSN) },

#ifdef LDAP_SUPERIOR_UUID
	{ "superiorUUID", "( 1.3.6.1.4.1.4203.666.1.11 NAME 'superiorUUID' "   
			"DESC 'UUID of the superior entry' "
			"EQUALITY UUIDMatch "
			"ORDERING UUIDOrderingMatch "
			"SYNTAX 1.3.6.1.1.16.1 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_superiorUUID) },
#endif

	{ "syncreplCookie", "( 1.3.6.1.4.1.4203.666.1.23 "
			"NAME 'syncreplCookie' "
			"DESC 'syncrepl Cookie for shadow copy' "
			"EQUALITY octetStringMatch "
			"ORDERING octetStringOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE dSAOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_syncreplCookie) },

	{ "contextCSN", "( 1.3.6.1.4.1.4203.666.1.25 "
			"NAME 'contextCSN' "
			"DESC 'the largest committed CSN of a context' "
			"EQUALITY CSNMatch "
			"ORDERING CSNOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.4203.666.11.2.1{64} "
			"NO-USER-MODIFICATION USAGE dSAOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_contextCSN) },

#ifdef LDAP_SYNC_TIMESTAMP
	{ "syncTimestamp", "( 1.3.6.1.4.1.4203.666.1.26 NAME 'syncTimestamp' "
			"DESC 'Time which object was replicated' "
			"EQUALITY generalizedTimeMatch "
			"ORDERING generalizedTimeOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE dSAOperation )",
		NULL, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_syncTimestamp) },
#endif

	/* root DSE attributes */
	{ "altServer", "( 1.3.6.1.4.1.1466.101.120.6 NAME 'altServer' "
			"DESC 'RFC4512: alternative servers' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.26 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_altServer) },
	{ "namingContexts", "( 1.3.6.1.4.1.1466.101.120.5 "
			"NAME 'namingContexts' "
			"DESC 'RFC4512: naming contexts' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_namingContexts) },
	{ "supportedControl", "( 1.3.6.1.4.1.1466.101.120.13 "
			"NAME 'supportedControl' "
			"DESC 'RFC4512: supported controls' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_supportedControl) },
	{ "supportedExtension", "( 1.3.6.1.4.1.1466.101.120.7 "
			"NAME 'supportedExtension' "
			"DESC 'RFC4512: supported extended operations' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_supportedExtension) },
	{ "supportedLDAPVersion", "( 1.3.6.1.4.1.1466.101.120.15 "
			"NAME 'supportedLDAPVersion' "
			"DESC 'RFC4512: supported LDAP versions' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_supportedLDAPVersion) },
	{ "supportedSASLMechanisms", "( 1.3.6.1.4.1.1466.101.120.14 "
			"NAME 'supportedSASLMechanisms' "
			"DESC 'RFC4512: supported SASL mechanisms'"
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_supportedSASLMechanisms) },
	{ "supportedFeatures", "( 1.3.6.1.4.1.4203.1.3.5 "
			"NAME 'supportedFeatures' "
			"DESC 'RFC4512: features supported by the server' "
			"EQUALITY objectIdentifierMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 "
			"USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_supportedFeatures) },
	{ "monitorContext", "( 1.3.6.1.4.1.4203.666.1.10 "
			"NAME 'monitorContext' "
			"DESC 'monitor context' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"EQUALITY distinguishedNameMatch "
			"SINGLE-VALUE NO-USER-MODIFICATION "
			"USAGE dSAOperation )",
		rootDseAttribute, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_monitorContext) },
	{ "configContext", "( 1.3.6.1.4.1.4203.1.12.2.1 "
			"NAME 'configContext' "
			"DESC 'config context' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"EQUALITY distinguishedNameMatch "
			"SINGLE-VALUE NO-USER-MODIFICATION "
			"USAGE dSAOperation )",
		rootDseAttribute, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_configContext) },
	{ "vendorName", "( 1.3.6.1.1.4 NAME 'vendorName' "
			"DESC 'RFC3045: name of implementation vendor' "
			"EQUALITY caseExactMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
			"SINGLE-VALUE NO-USER-MODIFICATION "
			"USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_vendorName) },
	{ "vendorVersion", "( 1.3.6.1.1.5 NAME 'vendorVersion' "
			"DESC 'RFC3045: version of implementation' "
			"EQUALITY caseExactMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
			"SINGLE-VALUE NO-USER-MODIFICATION "
			"USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_vendorVersion) },

	/* subentry attributes */
	{ "administrativeRole", "( 2.5.18.5 NAME 'administrativeRole' "
			"DESC 'RFC3672: administrative role' "
			"EQUALITY objectIdentifierMatch "
			"USAGE directoryOperation "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 )",
		administrativeRoleAttribute, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_administrativeRole) },
	{ "subtreeSpecification", "( 2.5.18.6 NAME 'subtreeSpecification' "
			"DESC 'RFC3672: subtree specification' "
			"SINGLE-VALUE "
			"USAGE directoryOperation "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.45 )",
		subentryAttribute, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_subtreeSpecification) },

	/* subschema subentry attributes */
	{ "dITStructureRules", "( 2.5.21.1 NAME 'dITStructureRules' "
			"DESC 'RFC4512: DIT structure rules' "
			"EQUALITY integerFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.17 "
			"USAGE directoryOperation ) ",
		subentryAttribute, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_ditStructureRules) },
	{ "dITContentRules", "( 2.5.21.2 NAME 'dITContentRules' "
			"DESC 'RFC4512: DIT content rules' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.16 USAGE directoryOperation )",
		subentryAttribute, SLAP_AT_HIDE,
		oidValidate, NULL,
		NULL, NULL, objectClassMatch, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_ditContentRules) },
	{ "matchingRules", "( 2.5.21.4 NAME 'matchingRules' "
			"DESC 'RFC4512: matching rules' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.30 USAGE directoryOperation )",
		subentryAttribute, 0,
		oidValidate, NULL,
		NULL, NULL, matchingRuleMatch, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_matchingRules) },
	{ "attributeTypes", "( 2.5.21.5 NAME 'attributeTypes' "
			"DESC 'RFC4512: attribute types' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.3 USAGE directoryOperation )",
		subentryAttribute, 0,
		oidValidate, NULL,
		NULL, NULL, attributeTypeMatch, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_attributeTypes) },
	{ "objectClasses", "( 2.5.21.6 NAME 'objectClasses' "
			"DESC 'RFC4512: object classes' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.37 USAGE directoryOperation )",
		subentryAttribute, 0,
		oidValidate, NULL,
		NULL, NULL, objectClassMatch, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_objectClasses) },
	{ "nameForms", "( 2.5.21.7 NAME 'nameForms' "
			"DESC 'RFC4512: name forms ' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.35 USAGE directoryOperation )",
		subentryAttribute, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_nameForms) },
	{ "matchingRuleUse", "( 2.5.21.8 NAME 'matchingRuleUse' "
			"DESC 'RFC4512: matching rule uses' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.31 USAGE directoryOperation )",
		subentryAttribute, 0,
		oidValidate, NULL,
		NULL, NULL, matchingRuleMatch, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_matchingRuleUse) },

	{ "ldapSyntaxes", "( 1.3.6.1.4.1.1466.101.120.16 NAME 'ldapSyntaxes' "
			"DESC 'RFC4512: LDAP syntaxes' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.54 USAGE directoryOperation )",
		subentryAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_ldapSyntaxes) },

	/* knowledge information */
	{ "aliasedObjectName", "( 2.5.4.1 "
			"NAME ( 'aliasedObjectName' 'aliasedEntryName' ) "
			"DESC 'RFC4512: name of aliased object' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 SINGLE-VALUE )",
		aliasAttribute, SLAP_AT_FINAL,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_aliasedObjectName) },
	{ "ref", "( 2.16.840.1.113730.3.1.34 NAME 'ref' "
			"DESC 'RFC3296: subordinate referral URL' "
			"EQUALITY caseExactMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
			"USAGE distributedOperation )",
		referralAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_ref) },

	/* access control internals */
	{ "entry", "( 1.3.6.1.4.1.4203.1.3.1 "
			"NAME 'entry' "
			"DESC 'OpenLDAP ACL entry pseudo-attribute' "
			"SYNTAX 1.3.6.1.4.1.4203.1.1.1 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE dSAOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_entry) },
	{ "children", "( 1.3.6.1.4.1.4203.1.3.2 "
			"NAME 'children' "
			"DESC 'OpenLDAP ACL children pseudo-attribute' "
			"SYNTAX 1.3.6.1.4.1.4203.1.1.1 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE dSAOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_children) },

	/* access control externals */
	{ "authzTo", "( 1.3.6.1.4.1.4203.666.1.8 "
			"NAME ( 'authzTo' 'saslAuthzTo' ) "
			"DESC 'proxy authorization targets' "
			"EQUALITY authzMatch "
			"SYNTAX 1.3.6.1.4.1.4203.666.2.7 "
			"X-ORDERED 'VALUES' "
			"USAGE distributedOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_saslAuthzTo) },
	{ "authzFrom", "( 1.3.6.1.4.1.4203.666.1.9 "
			"NAME ( 'authzFrom' 'saslAuthzFrom' ) "
			"DESC 'proxy authorization sources' "
			"EQUALITY authzMatch "
			"SYNTAX 1.3.6.1.4.1.4203.666.2.7 "
			"X-ORDERED 'VALUES' "
			"USAGE distributedOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_saslAuthzFrom) },

#ifdef LDAP_DYNAMIC_OBJECTS
	{ "entryTtl", "( 1.3.6.1.4.1.1466.101.119.3 NAME 'entryTtl' "
			"DESC 'RFC2589: entry time-to-live' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 SINGLE-VALUE "
			"NO-USER-MODIFICATION USAGE dSAOperation )",
		dynamicAttribute, SLAP_AT_MANAGEABLE,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_entryTtl) },
	{ "dynamicSubtrees", "( 1.3.6.1.4.1.1466.101.119.4 "
			"NAME 'dynamicSubtrees' "
			"DESC 'RFC2589: dynamic subtrees' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 NO-USER-MODIFICATION "
			"USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_dynamicSubtrees) },
#endif

	/* userApplication attributes (which system schema depends upon) */
	{ "distinguishedName", "( 2.5.4.49 NAME 'distinguishedName' "
			"DESC 'RFC4519: common supertype of DN attributes' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 )",
		NULL, SLAP_AT_ABSTRACT,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_distinguishedName) },
	{ "name", "( 2.5.4.41 NAME 'name' "
			"DESC 'RFC4519: common supertype of name attributes' "
			"EQUALITY caseIgnoreMatch "
			"SUBSTR caseIgnoreSubstringsMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15{32768} )",
		NULL, SLAP_AT_ABSTRACT,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_name) },
	{ "cn", "( 2.5.4.3 NAME ( 'cn' 'commonName' ) "
			"DESC 'RFC4519: common name(s) for which the entity is known by' "
			"SUP name )",
		NULL, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_cn) },
	{ "uid", "( 0.9.2342.19200300.100.1.1 NAME ( 'uid' 'userid' ) "
			"DESC 'RFC4519: user identifier' "
			"EQUALITY caseIgnoreMatch "
			"SUBSTR caseIgnoreSubstringsMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15{256} )",
		NULL, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_uid) },
	{ "uidNumber", /* for ldapi:// */
		"( 1.3.6.1.1.1.1.0 NAME 'uidNumber' "
    		"DESC 'RFC2307: An integer uniquely identifying a user "
				"in an administrative domain' "
    		"EQUALITY integerMatch "
    		"ORDERING integerOrderingMatch "
    		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 SINGLE-VALUE )",
		NULL, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_uidNumber) },
	{ "gidNumber", /* for ldapi:// */
		"( 1.3.6.1.1.1.1.1 NAME 'gidNumber' "
    		"DESC 'RFC2307: An integer uniquely identifying a group "
				"in an administrative domain' "
    		"EQUALITY integerMatch "
    		"ORDERING integerOrderingMatch "
    		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 SINGLE-VALUE )",
		NULL, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_gidNumber) },
	{ "userPassword", "( 2.5.4.35 NAME 'userPassword' "
			"DESC 'RFC4519/2307: password of user' "
			"EQUALITY octetStringMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40{128} )",
		NULL, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_userPassword) },

	{ "labeledURI", "( 1.3.6.1.4.1.250.1.57 NAME 'labeledURI' "
			"DESC 'RFC2079: Uniform Resource Identifier with optional label' "
			"EQUALITY caseExactMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 )",
		NULL, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_labeledURI) },

#ifdef SLAPD_AUTHPASSWD
	{ "authPassword", "( 1.3.6.1.4.1.4203.1.3.4 "
			"NAME 'authPassword' "
			"DESC 'RFC3112: authentication password attribute' "
			"EQUALITY 1.3.6.1.4.1.4203.1.2.2 "
			"SYNTAX 1.3.6.1.4.1.4203.1.1.2 )",
		NULL, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_authPassword) },
	{ "supportedAuthPasswordSchemes", "( 1.3.6.1.4.1.4203.1.3.3 "
			"NAME 'supportedAuthPasswordSchemes' "
			"DESC 'RFC3112: supported authPassword schemes' "
			"EQUALITY caseExactIA5Match "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.26{32} "
			"USAGE dSAOperation )",
		subschemaAttribute, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_authPasswordSchemes) },
#endif

	{ "description", "( 2.5.4.13 NAME 'description' "
			"DESC 'RFC4519: descriptive information' "
			"EQUALITY caseIgnoreMatch "
			"SUBSTR caseIgnoreSubstringsMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15{1024} )",
		NULL, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_description) },

	{ "seeAlso", "( 2.5.4.34 NAME 'seeAlso' "
			"DESC 'RFC4519: DN of related object' "
			"SUP distinguishedName )",
		NULL, 0,
		NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_seeAlso) },

	{ NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

static AttributeType slap_at_undefined = {
	{ "1.1.1", NULL, "Catchall for undefined attribute types", 1, NULL,
		NULL, NULL, NULL, NULL,
		0, 0, 0, 1, LDAP_SCHEMA_DSA_OPERATION, NULL }, /* LDAPAttributeType */
	BER_BVC("UNDEFINED"), /* cname */
	NULL, /* sup */
	NULL, /* subtypes */
	NULL, NULL, NULL, NULL,	/* matching rules routines */
	NULL, /* syntax (will be set later to "octetString") */
	NULL, /* schema check function */
	NULL, /* oidmacro */
	NULL, /* soidmacro */
	SLAP_AT_ABSTRACT|SLAP_AT_FINAL,	/* mask */
	{ NULL }, /* next */
	NULL /* attribute description */
	/* mutex (don't know how to initialize it :) */
};

static AttributeType slap_at_proxied = {
	{ "1.1.1", NULL, "Catchall for undefined proxied attribute types", 1, NULL,
		NULL, NULL, NULL, NULL,
		0, 0, 0, 0, LDAP_SCHEMA_USER_APPLICATIONS, NULL }, /* LDAPAttributeType */
	BER_BVC("PROXIED"), /* cname */
	NULL, /* sup */
	NULL, /* subtypes */
	NULL, NULL, NULL, NULL,	/* matching rules routines (will be set later) */
	NULL, /* syntax (will be set later to "octetString") */
	NULL, /* schema check function */
	NULL, /* oidmacro */
	NULL, /* soidmacro */
	SLAP_AT_ABSTRACT|SLAP_AT_FINAL,	/* mask */
	{ NULL }, /* next */
	NULL /* attribute description */
	/* mutex (don't know how to initialize it :) */
};

static struct slap_schema_mr_map {
	char *ssmm_name;
	size_t ssmm_offset;
} mr_map[] = {
	{ "caseExactIA5Match",
		offsetof(struct slap_internal_schema, si_mr_caseExactIA5Match) },
	{ "caseExactMatch",
		offsetof(struct slap_internal_schema, si_mr_caseExactMatch) },
	{ "caseExactSubstringsMatch",
		offsetof(struct slap_internal_schema, si_mr_caseExactSubstringsMatch) },
	{ "distinguishedNameMatch",
		offsetof(struct slap_internal_schema, si_mr_distinguishedNameMatch) },
	{ "dnSubtreeMatch",
		offsetof(struct slap_internal_schema, si_mr_dnSubtreeMatch) },
	{ "dnOneLevelMatch",
		offsetof(struct slap_internal_schema, si_mr_dnOneLevelMatch) },
	{ "dnSubordinateMatch",
		offsetof(struct slap_internal_schema, si_mr_dnSubordinateMatch) },
	{ "dnSuperiorMatch",
		offsetof(struct slap_internal_schema, si_mr_dnSuperiorMatch) },
	{ "integerMatch",
		offsetof(struct slap_internal_schema, si_mr_integerMatch) },
	{ "integerFirstComponentMatch",
		offsetof(struct slap_internal_schema,
			si_mr_integerFirstComponentMatch) },
	{ "objectIdentifierFirstComponentMatch",
		offsetof(struct slap_internal_schema,
			si_mr_objectIdentifierFirstComponentMatch) },
	{ "caseIgnoreMatch",
		offsetof(struct slap_internal_schema, si_mr_caseIgnoreMatch) },
	{ "caseIgnoreListMatch",
		offsetof(struct slap_internal_schema, si_mr_caseIgnoreListMatch) },
	{ NULL, 0 }
};

static struct slap_schema_syn_map {
	char *sssm_name;
	size_t sssm_offset;
} syn_map[] = {
	{ "1.3.6.1.4.1.1466.115.121.1.15",
		offsetof(struct slap_internal_schema, si_syn_directoryString) },
	{ "1.3.6.1.4.1.1466.115.121.1.12",
		offsetof(struct slap_internal_schema, si_syn_distinguishedName) },
	{ "1.3.6.1.4.1.1466.115.121.1.27",
		offsetof(struct slap_internal_schema, si_syn_integer) },
	{ "1.3.6.1.4.1.1466.115.121.1.40",
		offsetof(struct slap_internal_schema, si_syn_octetString) },
	{ "1.3.6.1.4.1.1466.115.121.1.3",
		offsetof(struct slap_internal_schema, si_syn_attributeTypeDesc) },
	{ "1.3.6.1.4.1.1466.115.121.1.16",
		offsetof(struct slap_internal_schema, si_syn_ditContentRuleDesc) },
	{ "1.3.6.1.4.1.1466.115.121.1.54",
		offsetof(struct slap_internal_schema, si_syn_ldapSyntaxDesc) },
	{ "1.3.6.1.4.1.1466.115.121.1.30",
		offsetof(struct slap_internal_schema, si_syn_matchingRuleDesc) },
	{ "1.3.6.1.4.1.1466.115.121.1.31",
		offsetof(struct slap_internal_schema, si_syn_matchingRuleUseDesc) },
	{ "1.3.6.1.4.1.1466.115.121.1.35",
		offsetof(struct slap_internal_schema, si_syn_nameFormDesc) },
	{ "1.3.6.1.4.1.1466.115.121.1.37",
		offsetof(struct slap_internal_schema, si_syn_objectClassDesc) },
	{ "1.3.6.1.4.1.1466.115.121.1.17",
		offsetof(struct slap_internal_schema, si_syn_ditStructureRuleDesc) },
	{ NULL, 0 }
};

int
slap_schema_load( void )
{
	int i;

	for( i=0; syn_map[i].sssm_name; i++ ) {
		Syntax ** synp = (Syntax **)
			&(((char *) &slap_schema)[syn_map[i].sssm_offset]);

		assert( *synp == NULL );

		*synp = syn_find( syn_map[i].sssm_name );

		if( *synp == NULL ) {
			fprintf( stderr, "slap_schema_load: Syntax: "
				"No syntax \"%s\" defined in schema\n",
				syn_map[i].sssm_name );
			return LDAP_INVALID_SYNTAX;
		}
	}

	for( i=0; mr_map[i].ssmm_name; i++ ) {
		MatchingRule ** mrp = (MatchingRule **)
			&(((char *) &slap_schema)[mr_map[i].ssmm_offset]);

		assert( *mrp == NULL );

		*mrp = mr_find( mr_map[i].ssmm_name );

		if( *mrp == NULL ) {
			fprintf( stderr, "slap_schema_load: MatchingRule: "
				"No matching rule \"%s\" defined in schema\n",
				mr_map[i].ssmm_name );
			return LDAP_INAPPROPRIATE_MATCHING;
		}
	}

	slap_at_undefined.sat_syntax = slap_schema.si_syn_octetString;
	slap_schema.si_at_undefined = &slap_at_undefined;

	slap_at_proxied.sat_equality = mr_find( "octetStringMatch" );
	slap_at_proxied.sat_approx = mr_find( "octetStringMatch" );
	slap_at_proxied.sat_ordering = mr_find( "octetStringOrderingMatch" );
	slap_at_proxied.sat_substr = mr_find( "octetStringSubstringsMatch" );
	slap_at_proxied.sat_syntax = slap_schema.si_syn_octetString;
	slap_schema.si_at_proxied = &slap_at_proxied;

	ldap_pvt_thread_mutex_init( &ad_index_mutex );
	ldap_pvt_thread_mutex_init( &ad_undef_mutex );
	ldap_pvt_thread_mutex_init( &oc_undef_mutex );

	for( i=0; ad_map[i].ssam_name; i++ ) {
		assert( ad_map[i].ssam_defn != NULL );
		{
			LDAPAttributeType *at;
			int		code;
			const char	*err;

			at = ldap_str2attributetype( ad_map[i].ssam_defn,
				&code, &err, LDAP_SCHEMA_ALLOW_ALL );
			if ( !at ) {
				fprintf( stderr,
					"slap_schema_load: AttributeType \"%s\": %s before %s\n",
					 ad_map[i].ssam_name, ldap_scherr2str(code), err );
				return code;
			}

			if ( at->at_oid == NULL ) {
				fprintf( stderr, "slap_schema_load: "
					"AttributeType \"%s\": no OID\n",
					ad_map[i].ssam_name );
				ldap_attributetype_free( at );
				return LDAP_OTHER;
			}

			code = at_add( at, 0, NULL, NULL, &err );
			if ( code ) {
				ldap_attributetype_free( at );
				fprintf( stderr, "slap_schema_load: AttributeType "
					"\"%s\": %s: \"%s\"\n",
					 ad_map[i].ssam_name, scherr2str(code), err );
				return code;
			}
			ldap_memfree( at );
		}
		{
			int rc;
			const char *text;
			Syntax *syntax = NULL;

			AttributeDescription ** adp = (AttributeDescription **)
				&(((char *) &slap_schema)[ad_map[i].ssam_offset]);

			assert( *adp == NULL );

			rc = slap_str2ad( ad_map[i].ssam_name, adp, &text );
			if( rc != LDAP_SUCCESS ) {
				fprintf( stderr, "slap_schema_load: AttributeType \"%s\": "
					"not defined in schema\n",
					ad_map[i].ssam_name );
				return rc;
			}

			if( ad_map[i].ssam_check ) {
				/* install check routine */
				(*adp)->ad_type->sat_check = ad_map[i].ssam_check;
			}
			/* install flags */
			(*adp)->ad_type->sat_flags |= ad_map[i].ssam_flags;

			/* install custom syntax routines */
			if( ad_map[i].ssam_syn_validate ||
				ad_map[i].ssam_syn_pretty )
			{
				Syntax *syn;

				syntax = (*adp)->ad_type->sat_syntax;

				syn = ch_malloc( sizeof( Syntax ) );
				*syn = *syntax;

				if( ad_map[i].ssam_syn_validate ) {
					syn->ssyn_validate = ad_map[i].ssam_syn_validate;
				}
				if( ad_map[i].ssam_syn_pretty ) {
					syn->ssyn_pretty = ad_map[i].ssam_syn_pretty;
				}

				(*adp)->ad_type->sat_syntax = syn;
			}

			/* install custom rule routines */
			if( syntax != NULL ||
				ad_map[i].ssam_mr_convert ||
				ad_map[i].ssam_mr_normalize ||
				ad_map[i].ssam_mr_match ||
				ad_map[i].ssam_mr_indexer ||
				ad_map[i].ssam_mr_filter )
			{
				MatchingRule *mr = ch_malloc( sizeof( MatchingRule ) );
				*mr = *(*adp)->ad_type->sat_equality;

				if ( syntax != NULL ) {
					mr->smr_syntax = (*adp)->ad_type->sat_syntax;
				}
				if ( ad_map[i].ssam_mr_convert ) {
					mr->smr_convert = ad_map[i].ssam_mr_convert;
				}
				if ( ad_map[i].ssam_mr_normalize ) {
					mr->smr_normalize = ad_map[i].ssam_mr_normalize;
				}
				if ( ad_map[i].ssam_mr_match ) {
					mr->smr_match = ad_map[i].ssam_mr_match;
				}
				if ( ad_map[i].ssam_mr_indexer ) {
					mr->smr_indexer = ad_map[i].ssam_mr_indexer;
				}
				if ( ad_map[i].ssam_mr_filter ) {
					mr->smr_filter = ad_map[i].ssam_mr_filter;
				}

				(*adp)->ad_type->sat_equality = mr;
			}
		}
	}

	for( i=0; oc_map[i].ssom_name; i++ ) {
		assert( oc_map[i].ssom_defn != NULL );
		{
			LDAPObjectClass *oc;
			int		code;
			const char	*err;

			oc = ldap_str2objectclass( oc_map[i].ssom_defn, &code, &err,
				LDAP_SCHEMA_ALLOW_ALL );
			if ( !oc ) {
				fprintf( stderr, "slap_schema_load: ObjectClass "
					"\"%s\": %s before %s\n",
				 	oc_map[i].ssom_name, ldap_scherr2str(code), err );
				return code;
			}

			if ( oc->oc_oid == NULL ) {
				fprintf( stderr, "slap_schema_load: ObjectClass "
					"\"%s\": no OID\n",
					oc_map[i].ssom_name );
				ldap_objectclass_free( oc );
				return LDAP_OTHER;
			}

			code = oc_add(oc,0,NULL,NULL,&err);
			if ( code ) {
				ldap_objectclass_free( oc );
				fprintf( stderr, "slap_schema_load: ObjectClass "
					"\"%s\": %s: \"%s\"\n",
				 	oc_map[i].ssom_name, scherr2str(code), err);
				return code;
			}
			ldap_memfree(oc);

		}
		{
			ObjectClass ** ocp = (ObjectClass **)
				&(((char *) &slap_schema)[oc_map[i].ssom_offset]);

			assert( *ocp == NULL );

			*ocp = oc_find( oc_map[i].ssom_name );
			if( *ocp == NULL ) {
				fprintf( stderr, "slap_schema_load: "
					"ObjectClass \"%s\": not defined in schema\n",
					oc_map[i].ssom_name );
				return LDAP_OBJECT_CLASS_VIOLATION;
			}

			if( oc_map[i].ssom_check ) {
				/* install check routine */
				(*ocp)->soc_check = oc_map[i].ssom_check;
			}
			/* install flags */
			(*ocp)->soc_flags |= oc_map[i].ssom_flags;
		}
	}

	return LDAP_SUCCESS;
}

int
slap_schema_check( void )
{
	/* we should only be called once after schema_init() was called */
	assert( schema_init_done == 1 );

	/*
	 * cycle thru attributeTypes to build matchingRuleUse
	 */
	if ( matching_rule_use_init() ) {
		return LDAP_OTHER;
	}

	++schema_init_done;
	return LDAP_SUCCESS;
}

static int rootDseObjectClass (
	Backend *be,
	Entry *e,
	ObjectClass *oc,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( e->e_nname.bv_len ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" only allowed in the root DSE",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	/* we should not be called for the root DSE */
	assert( 0 );
	return LDAP_SUCCESS;
}

static int aliasObjectClass (
	Backend *be,
	Entry *e,
	ObjectClass *oc,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_ALIASES(be) ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" not supported in context",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int referralObjectClass (
	Backend *be,
	Entry *e,
	ObjectClass *oc,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_REFERRALS(be) ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" not supported in context",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int subentryObjectClass (
	Backend *be,
	Entry *e,
	ObjectClass *oc,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_SUBENTRIES(be) ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" not supported in context",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	if( oc != slap_schema.si_oc_subentry && !is_entry_subentry( e ) ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" only allowed in subentries",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

#ifdef LDAP_DYNAMIC_OBJECTS
static int dynamicObjectClass (
	Backend *be,
	Entry *e,
	ObjectClass *oc,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_DYNAMIC(be) ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" not supported in context",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}
#endif /* LDAP_DYNAMIC_OBJECTS */

static int rootDseAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( e->e_nname.bv_len ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" only allowed in the root DSE",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	/* we should not be called for the root DSE */
	assert( 0 );
	return LDAP_SUCCESS;
}

static int aliasAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_ALIASES(be) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" not supported in context",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	if( !is_entry_alias( e ) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" only allowed in the alias",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int referralAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_REFERRALS(be) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" not supported in context",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	if( !is_entry_referral( e ) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" only allowed in the referral",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int subentryAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_SUBENTRIES(be) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" not supported in context",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	if( !is_entry_subentry( e ) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" only allowed in the subentry",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int administrativeRoleAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_SUBENTRIES(be) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" not supported in context",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	snprintf( textbuf, textlen,
		"attribute \"%s\" not supported!",
		attr->a_desc->ad_cname.bv_val );
	return LDAP_OBJECT_CLASS_VIOLATION;
}

#ifdef LDAP_DYNAMIC_OBJECTS
static int dynamicAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_DYNAMIC(be) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" not supported in context",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	if( !is_entry_dynamicObject( e ) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" only allowed in dynamic object",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}
#endif /* LDAP_DYNAMIC_OBJECTS */
