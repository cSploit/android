/* Copyright 2004 IBM Corporation
 * All rights reserved.
 * Redisribution and use in source and binary forms, with or without
 * modification, are permitted only as authorizd by the OpenLADP
 * Public License.
 */
/* ACKNOWLEDGEMENTS
 * This work originally developed by Sang Seok Lim
 * 2004/06/18	03:20:00	slim@OpenLDAP.org
 */

#include "portable.h"
#include <ac/string.h>
#include <ac/socket.h>
#include <ldap_pvt.h>
#include "lutil.h"
#include <ldap.h>
#include "slap.h"
#include "component.h"

#include "componentlib.h"
#include "asn.h"
#include <asn-gser.h>

#include <string.h>

#ifndef SLAPD_COMP_MATCH
#define SLAPD_COMP_MATCH SLAPD_MOD_DYNAMIC
#endif

/*
 * Attribute and MatchingRule aliasing table
 */
AttributeAliasing aa_table [ MAX_ALIASING_ENTRY ];
MatchingRuleAliasing mra_table [ MAX_ALIASING_ENTRY ];

OD_entry* gOD_table = NULL;
AsnTypetoMatchingRuleTable* gATMR_table = NULL;

int
load_derived_matching_rule ( char* cfg_path ){
}

AttributeAliasing*
comp_is_aliased_attribute( void *in  )
{
	AttributeAliasing* curr_aa;
	int i;
	AttributeDescription *ad = (AttributeDescription*)in;

	for ( i = 0; aa_table[i].aa_aliasing_ad && i < MAX_ALIASING_ENTRY; i++ ) {
		if ( strncmp(aa_table[i].aa_aliasing_ad->ad_cname.bv_val , ad->ad_cname.bv_val, ad->ad_cname.bv_len) == 0 )
			return &aa_table[i];
	}
	return NULL;
}

static int
add_aa_entry( int index, char* aliasing_at_name, char* aliased_at_name, char* mr_name, char* component_filter )
{
	char text[1][128];
	int rc;
	struct berval type;

	/* get and store aliasing AttributeDescription */
	type.bv_val = aliasing_at_name;
	type.bv_len = strlen ( aliasing_at_name );
	rc = slap_bv2ad ( &type, &aa_table[index].aa_aliasing_ad,(const char**)text );
	if ( rc != LDAP_SUCCESS ) return rc;

	/* get and store aliased AttributeDescription */
	type.bv_val = aliased_at_name;
	type.bv_len = strlen ( aliased_at_name );
	rc = slap_bv2ad ( &type, &aa_table[index].aa_aliased_ad,(const char**)text );
	if ( rc != LDAP_SUCCESS ) return rc;

	/* get and store componentFilterMatch */
	type.bv_val = mr_name;
	type.bv_len = strlen ( mr_name);
	aa_table[index].aa_mr = mr_bvfind ( &type );

	/* get and store a component filter */
	type.bv_val = component_filter;
	type.bv_len = strlen ( component_filter );
	rc = get_comp_filter( NULL, &type, &aa_table[index].aa_cf,(const char**)text);

	aa_table[index].aa_cf_str = component_filter;

	return rc;
}

/*
 * Initialize attribute aliasing table when this module is loaded
 * add_aa_entry ( index for the global table,
 *                name of the aliasing attribute,
 *                component filter with filling value parts "xxx"
 *              )
 * "xxx" will be replaced with effective values later.
 * See RFC3687 to understand the content of a component filter.
 */
char* pre_processed_comp_filter[] = {
/*1*/"item:{ component \"toBeSigned.issuer.rdnSequence\", rule distinguishedNameMatch, value xxx }",
/*2*/"item:{ component \"toBeSigned.serialNumber\", rule integerMatch, value xxx }",
/*3*/"and:{ item:{ component \"toBeSigned.serialNumber\", rule integerMatch, value xxx }, item:{ component \"toBeSigned.issuer.rdnSequence\", rule distinguishedNameMatch, value xxx } }"
};

static int
init_attribute_aliasing_table ()
{
	int rc;
	int index = 0 ;

	rc = add_aa_entry ( index, "x509CertificateIssuer", "userCertificate","componentFilterMatch", pre_processed_comp_filter[index] );
	if ( rc != LDAP_SUCCESS ) return LDAP_PARAM_ERROR;
	index++;

	rc = add_aa_entry ( index, "x509CertificateSerial","userCertificate", "componentFilterMatch", pre_processed_comp_filter[index] );
	if ( rc != LDAP_SUCCESS ) return LDAP_PARAM_ERROR;
	index++;

	rc = add_aa_entry ( index, "x509CertificateSerialAndIssuer", "userCertificate", "componentFilterMatch", pre_processed_comp_filter[index] );
	if ( rc != LDAP_SUCCESS ) return LDAP_PARAM_ERROR;
	index++;

	return LDAP_SUCCESS;
}

void
init_component_description_table () {
	AsnTypeId id;
	struct berval mr;
	AsnTypetoSyntax* asn_to_syn;
	Syntax* syn;

	for ( id = BASICTYPE_BOOLEAN; id != ASNTYPE_END ; id++ ) {
		asntype_to_compType_mapping_tbl[id].ac_comp_type.ct_subtypes = NULL;
		asntype_to_compType_mapping_tbl[id].ac_comp_type.ct_syntax =  NULL;

		/* Equality Matching Rule */
		if ( asntype_to_compMR_mapping_tbl[id].atc_equality ) {
			mr.bv_val = asntype_to_compMR_mapping_tbl[id].atc_equality;
			mr.bv_len = strlen(asntype_to_compMR_mapping_tbl[id].atc_equality);
			asntype_to_compType_mapping_tbl[id].ac_comp_type.ct_equality = mr_bvfind( &mr );
		}
		/* Approx Matching Rule */
		if ( asntype_to_compMR_mapping_tbl[id].atc_approx ) {
			mr.bv_val = asntype_to_compMR_mapping_tbl[id].atc_approx;
			mr.bv_len = strlen(asntype_to_compMR_mapping_tbl[id].atc_approx);
			asntype_to_compType_mapping_tbl[id].ac_comp_type.ct_approx = mr_bvfind( &mr );
		}

		/* Ordering Matching Rule */
		if ( asntype_to_compMR_mapping_tbl[id].atc_ordering ) {
			mr.bv_val = asntype_to_compMR_mapping_tbl[id].atc_ordering;
			mr.bv_len = strlen(asntype_to_compMR_mapping_tbl[id].atc_ordering);
			asntype_to_compType_mapping_tbl[id].ac_comp_type.ct_ordering= mr_bvfind( &mr );
		}

		/* Substr Matching Rule */
		if ( asntype_to_compMR_mapping_tbl[id].atc_substr ) {
			mr.bv_val = asntype_to_compMR_mapping_tbl[id].atc_substr;
			mr.bv_len = strlen(asntype_to_compMR_mapping_tbl[id].atc_substr);
			asntype_to_compType_mapping_tbl[id].ac_comp_type.ct_substr = mr_bvfind( &mr );
		}
		/* Syntax */

		asn_to_syn = &asn_to_syntax_mapping_tbl[ id ];
		if ( asn_to_syn->ats_syn_oid )
			syn = syn_find ( asn_to_syn->ats_syn_oid );
		else 
			syn = NULL;
		asntype_to_compType_mapping_tbl[id].ac_comp_type.ct_syntax = syn;

		/* Initialize Component Descriptions of primitive ASN.1 types */
		asntype_to_compdesc_mapping_tbl[id].atcd_cd.cd_comp_type = (AttributeType*)&asntype_to_compType_mapping_tbl[id].ac_comp_type;
	}
}

MatchingRule*
retrieve_matching_rule( char* mr_oid, AsnTypeId type ) {
	char* tmp;
	struct berval mr_name = BER_BVNULL;
	AsnTypetoMatchingRuleTable* atmr;

	for ( atmr = gATMR_table ; atmr ; atmr = atmr->atmr_table_next ) {
		if ( strcmp( atmr->atmr_oid, mr_oid ) == 0 ) {
			tmp = atmr->atmr_table[type].atmr_mr_name;
			if ( tmp ) {
				mr_name.bv_val = tmp;
				mr_name.bv_len = strlen( tmp );
				return mr_bvfind ( &mr_name );
			}
		}
	}
	return (MatchingRule*)NULL;
}

void* 
comp_convert_attr_to_comp LDAP_P (( Attribute* a, Syntax *syn, struct berval* bv ))
{
	char* peek_head;
        int mode, bytesDecoded, size, rc;
        void* component;
	char* oid = a->a_desc->ad_type->sat_atype.at_oid ;
        GenBuf* b = NULL;
        ExpBuf* buf = NULL;
	OidDecoderMapping* odm;
	
	/* look for the decoder registered for the given attribute */
	odm = RetrieveOidDecoderMappingbyOid( oid, strlen(oid) );

	if ( !odm || (!odm->BER_Decode && !odm->GSER_Decode) )
		return (void*)NULL;

	buf = ExpBufAllocBuf();
	ExpBuftoGenBuf( buf, &b );
	ExpBufInstallDataInBuf ( buf, bv->bv_val, bv->bv_len );
	BufResetInReadMode( b );

	mode = DEC_ALLOC_MODE_2;
	/*
	 * How can we decide which decoder will be called, GSER or BER?
	 * Currently BER decoder is called for a certificate.
	 * The flag of Attribute will say something about it in the future
	 */
	if ( syn && slap_syntax_is_ber ( syn ) ) {
#if 0
		rc =BDecComponentTop(odm->BER_Decode, a->a_comp_data->cd_mem_op, b, 0,0, &component,&bytesDecoded,mode ) ;
#endif
		rc = odm->BER_Decode ( a->a_comp_data->cd_mem_op, b, (ComponentSyntaxInfo*)&component, &bytesDecoded, mode );
	}
	else {
		rc = odm->GSER_Decode( a->a_comp_data->cd_mem_op, b, (ComponentSyntaxInfo**)component, &bytesDecoded, mode);
	}

	ExpBufFreeBuf( buf );
	GenBufFreeBuf( b );
	if ( rc == -1 ) {
#if 0
		ShutdownNibbleMemLocal ( a->a_comp_data->cd_mem_op );
		free ( a->a_comp_data );
		a->a_comp_data = NULL;
#endif
		return (void*)NULL;
	}
	else {
		return component;
	}
}

#include <nibble-alloc.h>
void
comp_free_component ( void* mem_op ) {
	ShutdownNibbleMemLocal( (NibbleMem*)mem_op );
	return;
}

void
comp_convert_assert_to_comp (
	void* mem_op,
	ComponentSyntaxInfo *csi_attr,
	struct berval* bv,
	ComponentSyntaxInfo** csi, int* len, int mode )
{
	int rc;
	GenBuf* genBuf;
	ExpBuf* buf;
	gser_decoder_func *decoder = csi_attr->csi_comp_desc->cd_gser_decoder;

	buf = ExpBufAllocBuf();
	ExpBuftoGenBuf( buf, &genBuf );
	ExpBufInstallDataInBuf ( buf, bv->bv_val, bv->bv_len );
	BufResetInReadMode( genBuf );

	if ( csi_attr->csi_comp_desc->cd_type_id == BASICTYPE_ANY )
		decoder = ((ComponentAny*)csi_attr)->cai->GSER_Decode;

	rc = (*decoder)( mem_op, genBuf, csi, len, mode );
	ExpBufFreeBuf ( buf );
	GenBufFreeBuf( genBuf );
}

int intToAscii( int value, char* buf ) {
	int minus=0,i,temp;
	int total_num_digits;

	if ( value == 0 ){
		buf[0] = '0';
		return 1;
	}

	if ( value < 0 ){
		minus = 1;
		value = value*(-1);
		buf[0] = '-';
	}
	
	/* How many digits */
	for ( temp = value, total_num_digits=0 ; temp ; total_num_digits++ )
		temp = temp/10;

	total_num_digits += minus;

	for ( i = minus ; value ; i++ ) {
		buf[ total_num_digits - i - 1 ]= (char)(value%10 + '0');
		value = value/10;
	}
	return i;
}

int
comp_convert_asn_to_ldap ( MatchingRule* mr, ComponentSyntaxInfo* csi, struct berval* bv, int *allocated )
{
	int rc;
	struct berval prettied;
	Syntax* syn;

	AsnTypetoSyntax* asn_to_syn =
		&asn_to_syntax_mapping_tbl[csi->csi_comp_desc->cd_type_id];
	if ( asn_to_syn->ats_syn_oid )
		csi->csi_syntax = syn_find ( asn_to_syn->ats_syn_oid );
	else 
		csi->csi_syntax = NULL;


        switch ( csi->csi_comp_desc->cd_type_id ) {
          case BASICTYPE_BOOLEAN :
		bv->bv_val = (char*)malloc( 5 );
		*allocated = 1;
		bv->bv_len = 5;
		if ( ((ComponentBool*)csi)->value > 0 ) {
			strcpy ( bv->bv_val , "TRUE" );
			bv->bv_len = 4;
		}
		else {
			strcpy ( bv->bv_val , "FALSE" );
			bv->bv_len = 5;
		}
                break ;
          case BASICTYPE_NULL :
                bv->bv_len = 0;
                break;
          case BASICTYPE_INTEGER :
		bv->bv_val = (char*)malloc( INITIAL_ATTR_SIZE );
		*allocated = 1;
		bv->bv_len = INITIAL_ATTR_SIZE;
		bv->bv_len = intToAscii(((ComponentInt*)csi)->value, bv->bv_val );
		if ( bv->bv_len <= 0 )
			return LDAP_INVALID_SYNTAX;
                break;
          case BASICTYPE_REAL :
		return LDAP_INVALID_SYNTAX;
          case BASICTYPE_ENUMERATED :
		bv->bv_val = (char*)malloc( INITIAL_ATTR_SIZE );
		*allocated = 1;
		bv->bv_len = INITIAL_ATTR_SIZE;
		bv->bv_len = intToAscii(((ComponentEnum*)csi)->value, bv->bv_val );
		if ( bv->bv_len <= 0 )
			return LDAP_INVALID_SYNTAX;
                break;
          case BASICTYPE_OID :
          case BASICTYPE_OCTETSTRING :
          case BASICTYPE_BITSTRING :
          case BASICTYPE_NUMERIC_STR :
          case BASICTYPE_PRINTABLE_STR :
          case BASICTYPE_UNIVERSAL_STR :
          case BASICTYPE_IA5_STR :
          case BASICTYPE_BMP_STR :
          case BASICTYPE_UTF8_STR :
          case BASICTYPE_UTCTIME :
          case BASICTYPE_GENERALIZEDTIME :
          case BASICTYPE_GRAPHIC_STR :
          case BASICTYPE_VISIBLE_STR :
          case BASICTYPE_GENERAL_STR :
          case BASICTYPE_OBJECTDESCRIPTOR :
          case BASICTYPE_VIDEOTEX_STR :
          case BASICTYPE_T61_STR :
          case BASICTYPE_OCTETCONTAINING :
          case BASICTYPE_BITCONTAINING :
          case BASICTYPE_RELATIVE_OID :
		bv->bv_val = ((ComponentOcts*)csi)->value.octs;
		bv->bv_len = ((ComponentOcts*)csi)->value.octetLen;
                break;
	  case BASICTYPE_ANY :
		csi = ((ComponentAny*)csi)->value;
		if ( csi->csi_comp_desc->cd_type != ASN_BASIC ||
			csi->csi_comp_desc->cd_type_id == BASICTYPE_ANY )
			return LDAP_INVALID_SYNTAX;
		return comp_convert_asn_to_ldap( mr, csi, bv, allocated );
          case COMPOSITE_ASN1_TYPE :
		break;
          case RDNSequence :
		/*dnMatch*/
		if( strncmp( mr->smr_mrule.mr_oid, DN_MATCH_OID, strlen(DN_MATCH_OID) ) != 0 )
			return LDAP_INVALID_SYNTAX;
		*allocated = 1;
		rc = ConvertRDNSequence2RFC2253( (irRDNSequence*)csi, bv );
		if ( rc != LDAP_SUCCESS ) return rc;
		break;
          case RelativeDistinguishedName :
		/*rdnMatch*/
		if( strncmp( mr->smr_mrule.mr_oid, RDN_MATCH_OID, strlen(RDN_MATCH_OID) ) != 0 )
			return LDAP_INVALID_SYNTAX;
		*allocated = 1;
		rc = ConvertRDN2RFC2253((irRelativeDistinguishedName*)csi,bv);
		if ( rc != LDAP_SUCCESS ) return rc;
		break;
          case TelephoneNumber :
          case FacsimileTelephoneNumber__telephoneNumber :
		break;
          case DirectoryString :
		return LDAP_INVALID_SYNTAX;
          case ASN_COMP_CERTIFICATE :
          case ASNTYPE_END :
		break;
          default :
                /*Only ASN Basic Type can be converted into LDAP string*/
		return LDAP_INVALID_SYNTAX;
        }

	if ( csi->csi_syntax ) {
		if ( csi->csi_syntax->ssyn_validate ) {
 			rc = csi->csi_syntax->ssyn_validate(csi->csi_syntax, bv);
			if ( rc != LDAP_SUCCESS )
				return LDAP_INVALID_SYNTAX;
		}
		if ( csi->csi_syntax->ssyn_pretty ) {
			rc = csi->csi_syntax->ssyn_pretty(csi->csi_syntax, bv, &prettied , NULL );
			if ( rc != LDAP_SUCCESS )
				return LDAP_INVALID_SYNTAX;
#if 0
			free ( bv->bv_val );/*potential memory leak?*/
#endif
			bv->bv_val = prettied.bv_val;
			bv->bv_len = prettied.bv_len;
		}
	}

	return LDAP_SUCCESS;
}

/*
 * If <all> type component referenced is used
 * more than one component will be tested
 */
#define IS_TERMINAL_COMPREF(cr) (cr->cr_curr->ci_next == NULL)
int
comp_test_all_components (
	void* attr_mem_op,
	void* assert_mem_op,
	ComponentSyntaxInfo *csi_attr,
	ComponentAssertion* ca )
{
	int rc;
	ComponentSyntaxInfo *csi_temp = NULL, *csi_assert = NULL, *comp_elmt = NULL;
	ComponentReference *cr = ca->ca_comp_ref;
	struct berval *ca_val = &ca->ca_ma_value;

	switch ( cr->cr_curr->ci_type ) {
	    case LDAP_COMPREF_ALL:
		if ( IS_TERMINAL_COMPREF(cr) ) {
			FOR_EACH_LIST_ELMT( comp_elmt, &((ComponentList*)csi_attr)->comp_list )
			{
				rc = comp_test_one_component( attr_mem_op, assert_mem_op, comp_elmt, ca );
				if ( rc == LDAP_COMPARE_TRUE ) {
					break;
				}
			}
		} else {
			ComponentId *start_compid = ca->ca_comp_ref->cr_curr->ci_next;
			FOR_EACH_LIST_ELMT( comp_elmt, &((ComponentList*)csi_attr)->comp_list )
			{
				cr->cr_curr = start_compid;
				rc = comp_test_components ( attr_mem_op, assert_mem_op, comp_elmt, ca );
				if ( rc != LDAP_COMPARE_FALSE ) {
					break;
				}
#if 0				
				if ( rc == LDAP_COMPARE_TRUE ) {
					break;
				}
#endif
			}
		}
		break;
	    case LDAP_COMPREF_CONTENT:
	    case LDAP_COMPREF_SELECT:
	    case LDAP_COMPREF_DEFINED:
	    case LDAP_COMPREF_UNDEFINED:
	    case LDAP_COMPREF_IDENTIFIER:
	    case LDAP_COMPREF_FROM_BEGINNING:
	    case LDAP_COMPREF_FROM_END:
	    case LDAP_COMPREF_COUNT:
		rc = LDAP_OPERATIONS_ERROR;
		break;
	    default:
		rc = LDAP_OPERATIONS_ERROR;
	}
	return rc;
}

void
eat_bv_whsp ( struct berval* in )
{
	char* end = in->bv_val + in->bv_len;
        for ( ; ( *in->bv_val == ' ' ) && ( in->bv_val < end ) ; ) {
                in->bv_val++;
        }
}

/*
 * Perform matching one referenced component against assertion
 * If the matching rule in a component filter is allComponentsMatch
 * or its derivatives the extracted component's ASN.1 specification
 * is applied to the assertion value as its syntax
 * Otherwise, the matching rule's syntax is applied to the assertion value
 * By RFC 3687
 */
int
comp_test_one_component (
	void* attr_mem_op,
	void* assert_mem_op,
	ComponentSyntaxInfo *csi_attr,
	ComponentAssertion *ca )
{
	int len, rc;
	ComponentSyntaxInfo *csi_assert = NULL;
	char* oid = NULL;
	MatchingRule* mr = ca->ca_ma_rule;

	if ( mr->smr_usage & SLAP_MR_COMPONENT ) {
		/* If allComponentsMatch or its derivatives */
		if ( !ca->ca_comp_data.cd_tree ) {
			comp_convert_assert_to_comp( assert_mem_op, csi_attr, &ca->ca_ma_value, &csi_assert, &len, DEC_ALLOC_MODE_0 );
			ca->ca_comp_data.cd_tree = (void*)csi_assert;
		} else {
			csi_assert = ca->ca_comp_data.cd_tree;
		}

		if ( !csi_assert )
			return LDAP_PROTOCOL_ERROR;

		if ( strcmp( mr->smr_mrule.mr_oid, OID_ALL_COMP_MATCH ) != 0 )
                {
                        /* allComponentMatch's derivatives */
			oid =  mr->smr_mrule.mr_oid;
                }
                        return csi_attr->csi_comp_desc->cd_all_match(
                               			 oid, csi_attr, csi_assert );

	} else {
		/* LDAP existing matching rules */
		struct berval attr_bv = BER_BVNULL;
		struct berval n_attr_bv = BER_BVNULL;
		struct berval* assert_bv = &ca->ca_ma_value;
		int allocated = 0;
		/*Attribute is converted to compatible LDAP encodings*/
		if ( comp_convert_asn_to_ldap( mr, csi_attr, &attr_bv, &allocated ) != LDAP_SUCCESS )
			return LDAP_INAPPROPRIATE_MATCHING;
		/* extracted component value is not normalized */
		if ( ca->ca_ma_rule->smr_normalize ) {
			rc = ca->ca_ma_rule->smr_normalize (
				SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
				NULL, ca->ca_ma_rule,
				&attr_bv, &n_attr_bv, NULL );
			if ( rc != LDAP_SUCCESS )
				return rc;
			if ( allocated && attr_bv.bv_val )
				free (attr_bv.bv_val);
		} else {
			n_attr_bv = attr_bv;
		}
#if 0
		/*Assertion value is validated by MR's syntax*/
		if ( !ca->ca_comp_data.cd_tree ) {
			ca->ca_comp_data.cd_tree = assert_bv;
		}
		else {
			assert_bv = ca->ca_comp_data.cd_tree;
		}
#endif
		if ( !n_attr_bv.bv_val )
			return LDAP_COMPARE_FALSE;
		rc = csi_value_match( mr, &n_attr_bv, assert_bv );
		if ( n_attr_bv.bv_val )
			free ( n_attr_bv.bv_val );
		return rc;
	}
}

int
comp_test_components( void* attr_nm, void* assert_nm, ComponentSyntaxInfo* csi_attr, ComponentAssertion* ca) {
	char* peek_head;
	int mode, bytesDecoded = 0, rc;
	GenBuf* b;
	ExpBuf* buf;
	OidDecoderMapping* odm;
	struct berval bv;
	char oid[MAX_OID_LEN];
	void* contained_comp, *anytype_comp;
	ComponentReference* cr = ca->ca_comp_ref;

	if ( !cr )
		return comp_test_one_component ( attr_nm, assert_nm, csi_attr, ca );
	/* Extracting the component refrenced by ca->ca_comp_ref */
	csi_attr = (ComponentSyntaxInfo*)csi_attr->csi_comp_desc->cd_extract_i( attr_nm, cr, csi_attr );
	if ( !csi_attr ) return LDAP_INVALID_SYNTAX;
	/* perform matching, considering the type of a Component Reference(CR)*/
	switch( cr->cr_curr->ci_type ) {
	   case LDAP_COMPREF_IDENTIFIER:
	   case LDAP_COMPREF_FROM_BEGINNING:
	   case LDAP_COMPREF_FROM_END:
	   case LDAP_COMPREF_COUNT:
		/*
		 * Exactly one component is referenced
		 * Fast Path for matching for this case
		 */
		rc = comp_test_one_component ( attr_nm, assert_nm, csi_attr, ca );
		break;
	   case LDAP_COMPREF_ALL:
		/*
		 * If <all> type CR is used
		 * more than one component will be tested
		 */
		rc = comp_test_all_components ( attr_nm, assert_nm, csi_attr, ca );
		break;

	   case LDAP_COMPREF_CONTENT:
		/*
		 * <content> type CR is used
		 * check if it is followed by <select> type CR.
		 * 1) If so, look up the corresponding decoder  in the mapping
		 * table(OID to decoder) by <select>
		 * and then decode the OCTET/BIT STRING with the decoder
		 * Finially, extreact the target component with the remaining CR.
		 * 2) If not, just return the current component, It SHOULD not be
		 * extracted further, because the component MUST be BIT/OCTET
                 * string.
                 */

		cr->cr_curr = cr->cr_curr->ci_next;
		if ( !cr->cr_curr ) {
			/* case 2) in above description */
			rc = comp_test_one_component ( attr_nm, assert_nm, csi_attr, ca );
			break;
		}

		if ( cr->cr_curr->ci_type == LDAP_COMPREF_SELECT ) {
			/* Look up OID mapping table */	
			odm = RetrieveOidDecoderMappingbyBV( &cr->cr_curr->ci_val.ci_select_value );
			
			if ( !odm || !odm->BER_Decode )
				return  LDAP_PROTOCOL_ERROR;

			/* current componet MUST be either BIT or OCTET STRING */
			if ( csi_attr->csi_comp_desc->cd_type_id != BASICTYPE_BITSTRING ) {
				bv.bv_val = ((ComponentBits*)csi_attr)->value.bits;
				bv.bv_len = ((ComponentBits*)csi_attr)->value.bitLen;
			}
			else if ( csi_attr->csi_comp_desc->cd_type_id != BASICTYPE_BITSTRING ) {
				bv.bv_val = ((ComponentOcts*)csi_attr)->value.octs;
				bv.bv_len = ((ComponentOcts*)csi_attr)->value.octetLen;
			}
			else
				return LDAP_PROTOCOL_ERROR;

			buf = ExpBufAllocBuf();
			ExpBuftoGenBuf( buf, &b );
			ExpBufInstallDataInBuf ( buf, bv.bv_val, bv.bv_len );
			BufResetInReadMode( b );
			mode = DEC_ALLOC_MODE_2;

			/* Try to decode with BER/DER decoder */
			rc = odm->BER_Decode ( attr_nm, b, (ComponentSyntaxInfo*)&contained_comp, &bytesDecoded, mode );

			ExpBufFreeBuf( buf );
			GenBufFreeBuf( b );

			if ( rc != LDAP_SUCCESS ) return LDAP_PROTOCOL_ERROR;

			/* xxx.content.(x.xy.xyz).rfc822Name */
			/* In the aboe Ex. move CR to the right to (x.xy.xyz)*/
			cr->cr_curr = cr->cr_curr->ci_next;
			if (!cr->cr_curr )
				rc = comp_test_one_component ( attr_nm, assert_nm, csi_attr, ca );
			else
				rc = comp_test_components( attr_nm, assert_nm, contained_comp, ca );
		}
		else {
			/* Ivalid Component reference */
			rc = LDAP_PROTOCOL_ERROR;
		}
		break;
	   case LDAP_COMPREF_SELECT:
		if (csi_attr->csi_comp_desc->cd_type_id != BASICTYPE_ANY )
			return LDAP_INVALID_SYNTAX;
		rc = CheckSelectTypeCorrect( attr_nm, ((ComponentAny*)csi_attr)->cai, &cr->cr_curr->ci_val.ci_select_value );
		if ( rc < 0 ) return LDAP_INVALID_SYNTAX;

		/* point to the real component, not any type component */
		csi_attr = ((ComponentAny*)csi_attr)->value;
		cr->cr_curr = cr->cr_curr->ci_next;
		if ( cr->cr_curr )
			rc =  comp_test_components( attr_nm, assert_nm, csi_attr, ca);
		else
			rc =  comp_test_one_component( attr_nm, assert_nm, csi_attr, ca);
		break;
	   default:
		rc = LDAP_INVALID_SYNTAX;
	}
	return rc;
}


void*
comp_nibble_memory_allocator ( int init_mem, int inc_mem ) {
	void* nm;
	nm = (void*)InitNibbleMemLocal( (unsigned long)init_mem, (unsigned long)inc_mem );
	if ( !nm ) return NULL;
	else return (void*)nm;
}

void
comp_nibble_memory_free ( void* nm ) {
	ShutdownNibbleMemLocal( nm );
}

void*
comp_get_component_description ( int id ) {
	if ( asntype_to_compdesc_mapping_tbl[id].atcd_typeId == id )
		return &asntype_to_compdesc_mapping_tbl[id].atcd_cd;
	else
		return NULL;
}

int
comp_component_encoder ( void* mem_op, ComponentSyntaxInfo* csi , struct berval* nval ) {
        int size, rc;
        GenBuf* b;
        ExpBuf* buf;
	struct berval bv;
	
	buf = ExpBufAllocBufAndData();
	ExpBufResetInWriteRvsMode(buf);
	ExpBuftoGenBuf( buf, &b );

	if ( !csi->csi_comp_desc->cd_gser_encoder && !csi->csi_comp_desc->cd_ldap_encoder )
		return (-1);

	/*
	 * if an LDAP specific encoder is provided :
	 * dn and rdn have their LDAP specific encoder
	 */
	if ( csi->csi_comp_desc->cd_ldap_encoder ) {
		rc = csi->csi_comp_desc->cd_ldap_encoder( csi, &bv );
		if ( rc != LDAP_SUCCESS )
			return rc;
		if ( mem_op )
			nval->bv_val = CompAlloc( mem_op, bv.bv_len );
		else
			nval->bv_val = malloc( size );
		memcpy( nval->bv_val, bv.bv_val, bv.bv_len );
		nval->bv_len = bv.bv_len;
		/*
		 * This free will be eliminated by making ldap_encoder
		 * use nibble memory in it 
		 */
		free ( bv.bv_val );
		GenBufFreeBuf( b );
		BufFreeBuf( buf );
		return LDAP_SUCCESS;
	}

	rc = csi->csi_comp_desc->cd_gser_encoder( b, csi );
	if ( rc < 0 ) {
		GenBufFreeBuf( b );
		BufFreeBuf( buf );
		return rc;
	}

	size = ExpBufDataSize( buf );
	if ( size > 0 ) {
		if ( mem_op )
			nval->bv_val = CompAlloc ( mem_op, size );
		else
			nval->bv_val = malloc( size );
		nval->bv_len = size;
		BufResetInReadMode(b);
		BufCopy( nval->bv_val, b, size );
	}
	ExpBufFreeBuf( buf );
	GenBufFreeBuf( b );

	return LDAP_SUCCESS;
}

#if SLAPD_COMP_MATCH == SLAPD_MOD_DYNAMIC

#include "certificate.h"

extern convert_attr_to_comp_func* attr_converter;
extern convert_assert_to_comp_func* assert_converter;
extern convert_asn_to_ldap_func* csi_converter;
extern free_component_func* component_destructor;
extern test_component_func* test_components;
extern alloc_nibble_func* nibble_mem_allocator;
extern free_nibble_func* nibble_mem_free;
extern test_membership_func* is_aliased_attribute;
extern get_component_info_func* get_component_description;
extern component_encoder_func* component_encoder;


int init_module(int argc, char *argv[]) {
	/*
	 * Initialize function pointers in slapd
	 */
	attr_converter = (convert_attr_to_comp_func*)comp_convert_attr_to_comp;
	assert_converter = (convert_assert_to_comp_func*)comp_convert_assert_to_comp;
	component_destructor = (free_component_func*)comp_free_component;
	test_components = (test_component_func*)comp_test_components;
	nibble_mem_allocator = (free_nibble_func*)comp_nibble_memory_allocator;
	nibble_mem_free = (free_nibble_func*)comp_nibble_memory_free;
	is_aliased_attribute = (test_membership_func*)comp_is_aliased_attribute;
	get_component_description = (get_component_info_func*)comp_get_component_description;
	component_encoder = (component_encoder_func*)comp_component_encoder;

	/* file path needs to be */
	load_derived_matching_rule ("derived_mr.cfg");

	/* the initialization for example X.509 certificate */
	init_module_AuthenticationFramework();
	init_module_AuthorityKeyIdentifierDefinition();
	init_module_CertificateRevokationList();
	init_attribute_aliasing_table ();
	init_component_description_table ();
	return 0;
}

#endif /* SLAPD_PASSWD */
