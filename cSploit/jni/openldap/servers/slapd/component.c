/* component.c -- Component Filter Match Routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
 * Portions Copyright 2004 by IBM Corporation.
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

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include <ldap.h>
#include "slap.h"

#ifdef LDAP_COMP_MATCH

#include "component.h"

/*
 * Following function pointers are initialized
 * when a component module is loaded
 */
alloc_nibble_func* nibble_mem_allocator = NULL;
free_nibble_func* nibble_mem_free = NULL;
convert_attr_to_comp_func* attr_converter = NULL;
convert_assert_to_comp_func* assert_converter = NULL ;
free_component_func* component_destructor = NULL ;
test_component_func* test_components = NULL;
test_membership_func* is_aliased_attribute = NULL;
component_encoder_func* component_encoder = NULL;
get_component_info_func* get_component_description = NULL;
#define OID_ALL_COMP_MATCH "1.2.36.79672281.1.13.6"
#define OID_COMP_FILTER_MATCH "1.2.36.79672281.1.13.2"
#define MAX_LDAP_STR_LEN 128

static int
peek_componentId_type( ComponentAssertionValue* cav );

static int
strip_cav_str( ComponentAssertionValue* cav, char* str);

static int
peek_cav_str( ComponentAssertionValue* cav, char* str );

static int
parse_comp_filter( Operation* op, ComponentAssertionValue* cav,
				ComponentFilter** filt, const char** text );

static void
free_comp_filter( ComponentFilter* f );

static int
test_comp_filter( Syntax *syn, ComponentSyntaxInfo *a, ComponentFilter *f );

int
componentCertificateValidate(
	Syntax *syntax,
	struct berval *val )
{
	return LDAP_SUCCESS;
}

int
componentFilterValidate(
	Syntax *syntax,
	struct berval *val )
{
	return LDAP_SUCCESS;
}

int
allComponentsValidate(
	Syntax *syntax,
	struct berval *val )
{
	return LDAP_SUCCESS;
}

int
componentFilterMatch ( 
	int *matchp, 
	slap_mask_t flags, 
	Syntax *syntax, 
	MatchingRule *mr,
	struct berval *value, 
	void *assertedValue )
{
	ComponentSyntaxInfo *csi_attr = (ComponentSyntaxInfo*)value;
	MatchingRuleAssertion * ma = (MatchingRuleAssertion*)assertedValue;
	int rc;

	if ( !mr || !ma->ma_cf ) return LDAP_INAPPROPRIATE_MATCHING;

	/* Check if the component module is loaded */
	if ( !attr_converter || !nibble_mem_allocator ) {
		return LDAP_OTHER;
	}

	rc = test_comp_filter( syntax, csi_attr, ma->ma_cf );

	if ( rc == LDAP_COMPARE_TRUE ) {
		*matchp = 0;
		return LDAP_SUCCESS;
	}
	else if ( rc == LDAP_COMPARE_FALSE ) {
		*matchp = 1;
		return LDAP_SUCCESS;
	}
	else {
		return LDAP_INAPPROPRIATE_MATCHING;
	}
}

int
directoryComponentsMatch( 
	int *matchp, 
	slap_mask_t flags, 
	Syntax *syntax, 
	MatchingRule *mr,
	struct berval *value, 
	void *assertedValue )
{
	/* Only for registration */
	*matchp = 0;
	return LDAP_SUCCESS;
}

int
allComponentsMatch( 
	int *matchp, 
	slap_mask_t flags, 
	Syntax *syntax, 
	MatchingRule *mr,
	struct berval *value, 
	void *assertedValue )
{
	/* Only for registration */
	*matchp = 0;
	return LDAP_SUCCESS;
}

static int
slapd_ber2cav( struct berval* bv, ComponentAssertionValue* cav )
{
	cav->cav_ptr = cav->cav_buf = bv->bv_val;
	cav->cav_end = bv->bv_val + bv->bv_len;

	return LDAP_SUCCESS;
}

ComponentReference*
dup_comp_ref ( Operation* op, ComponentReference* cr )
{
	ComponentReference* dup_cr;
	ComponentId* ci_curr;
	ComponentId** ci_temp;

	dup_cr = op->o_tmpalloc( sizeof( ComponentReference ), op->o_tmpmemctx );

	dup_cr->cr_len = cr->cr_len;
	dup_cr->cr_string = cr->cr_string;

	ci_temp = &dup_cr->cr_list;
	ci_curr = cr->cr_list;

	for ( ; ci_curr != NULL ;
		ci_curr = ci_curr->ci_next, ci_temp = &(*ci_temp)->ci_next )
	{
		*ci_temp = op->o_tmpalloc( sizeof( ComponentId ), op->o_tmpmemctx );
		if ( !*ci_temp ) return NULL;
		**ci_temp = *ci_curr;
	}

	dup_cr->cr_curr = dup_cr->cr_list;

	return dup_cr;
}

static int
dup_comp_filter_list (
	Operation *op,
	struct berval *bv,
	ComponentFilter* in_f,
	ComponentFilter** out_f )
{
	ComponentFilter **new, *f;
	int		rc;

	new = out_f;
	for ( f = in_f; f != NULL; f = f->cf_next ) {
		rc = dup_comp_filter( op, bv, f, new );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}
		new = &(*new)->cf_next;
	}
	return LDAP_SUCCESS;
}

int
get_len_of_next_assert_value ( struct berval* bv, char separator )
{
	ber_len_t i = 0;
	while (1) {
		if ( (bv->bv_val[ i ] == separator) || ( i >= bv->bv_len) )
			break;
		i++;
	}
	bv->bv_val += (i + 1);
	bv->bv_len -= (i + 1);
	return i;
}

int
dup_comp_filter_item (
	Operation *op,
	struct berval* assert_bv,
	ComponentAssertion* in_ca,
	ComponentAssertion** out_ca )
{
	int len;

	if ( !in_ca->ca_comp_ref ) return SLAPD_DISCONNECT;

	*out_ca = op->o_tmpalloc( sizeof( ComponentAssertion ), op->o_tmpmemctx );
	if ( !(*out_ca) ) return LDAP_NO_MEMORY;

	(*out_ca)->ca_comp_data.cd_tree = NULL;
	(*out_ca)->ca_comp_data.cd_mem_op = NULL;

	(*out_ca)->ca_comp_ref = dup_comp_ref ( op, in_ca->ca_comp_ref );
	(*out_ca)->ca_use_def = 0;
	(*out_ca)->ca_ma_rule = in_ca->ca_ma_rule;

	(*out_ca)->ca_ma_value.bv_val = assert_bv->bv_val;
	len = get_len_of_next_assert_value ( assert_bv, '$' );
	if ( len <= 0 ) return SLAPD_DISCONNECT;
	(*out_ca)->ca_ma_value.bv_len = len;
	
	return LDAP_SUCCESS;
}

int
dup_comp_filter (
	Operation* op,
	struct berval *bv,
	ComponentFilter *in_f,
	ComponentFilter **out_f )
{
	int	rc;
	ComponentFilter dup_f = {0};

	if ( !in_f ) return LDAP_PROTOCOL_ERROR;

	switch ( in_f->cf_choice ) {
	case LDAP_COMP_FILTER_AND:
		rc = dup_comp_filter_list( op, bv, in_f->cf_and, &dup_f.cf_and);
		dup_f.cf_choice = LDAP_COMP_FILTER_AND;
		break;
	case LDAP_COMP_FILTER_OR:
		rc = dup_comp_filter_list( op, bv, in_f->cf_or, &dup_f.cf_or);
		dup_f.cf_choice = LDAP_COMP_FILTER_OR;
		break;
	case LDAP_COMP_FILTER_NOT:
		rc = dup_comp_filter( op, bv, in_f->cf_not, &dup_f.cf_not);
		dup_f.cf_choice = LDAP_COMP_FILTER_NOT;
		break;
	case LDAP_COMP_FILTER_ITEM:
		rc = dup_comp_filter_item( op, bv, in_f->cf_ca ,&dup_f.cf_ca );
		dup_f.cf_choice = LDAP_COMP_FILTER_ITEM;
		break;
	default:
		rc = LDAP_PROTOCOL_ERROR;
	}

	if ( rc == LDAP_SUCCESS ) {
		*out_f = op->o_tmpalloc( sizeof(dup_f), op->o_tmpmemctx );
		**out_f = dup_f;
	}

	return( rc );
}

int
get_aliased_filter_aa ( Operation* op, AttributeAssertion* a_assert, AttributeAliasing* aa, const char** text )
{
	struct berval assert_bv;

	Debug( LDAP_DEBUG_FILTER, "get_aliased_filter\n", 0, 0, 0 );

	if ( !aa->aa_cf  )
		return LDAP_PROTOCOL_ERROR;

	assert_bv = a_assert->aa_value;
	/*
	 * Duplicate aa->aa_cf to ma->ma_cf by replacing the
	 * the component assertion value in assert_bv
	 * Multiple values may be separated with '$'
	 */
	return dup_comp_filter ( op, &assert_bv, aa->aa_cf, &a_assert->aa_cf );
}

int
get_aliased_filter( Operation* op,
	MatchingRuleAssertion* ma, AttributeAliasing* aa,
	const char** text )
{
	struct berval assert_bv;

	Debug( LDAP_DEBUG_FILTER, "get_aliased_filter\n", 0, 0, 0 );

	if ( !aa->aa_cf  ) return LDAP_PROTOCOL_ERROR;

	assert_bv = ma->ma_value;
	/* Attribute Description is replaced with aliased one */
	ma->ma_desc = aa->aa_aliased_ad;
	ma->ma_rule = aa->aa_mr;
	/*
	 * Duplicate aa->aa_cf to ma->ma_cf by replacing the
	 * the component assertion value in assert_bv
	 * Multiple values may be separated with '$'
	 */
	return dup_comp_filter ( op, &assert_bv, aa->aa_cf, &ma->ma_cf );
}

int
get_comp_filter( Operation* op, struct berval* bv,
	ComponentFilter** filt, const char **text )
{
	ComponentAssertionValue cav;
	int rc;

	Debug( LDAP_DEBUG_FILTER, "get_comp_filter\n", 0, 0, 0 );
	if ( (rc = slapd_ber2cav(bv, &cav) ) != LDAP_SUCCESS ) {
		return rc;
	}
	rc = parse_comp_filter( op, &cav, filt, text );
	bv->bv_val = cav.cav_ptr;

	return rc;
}

static void
eat_whsp( ComponentAssertionValue* cav )
{
	for ( ; ( *cav->cav_ptr == ' ' ) && ( cav->cav_ptr < cav->cav_end ) ; ) {
		cav->cav_ptr++;
	}
}

static int
cav_cur_len( ComponentAssertionValue* cav )
{
	return cav->cav_end - cav->cav_ptr;
}

static ber_tag_t
comp_first_element( ComponentAssertionValue* cav )
{
	eat_whsp( cav );
	if ( cav_cur_len( cav ) >= 8 && strncmp( cav->cav_ptr, "item", 4 ) == 0 ) {
		return LDAP_COMP_FILTER_ITEM;

	} else if ( cav_cur_len( cav ) >= 7 &&
		strncmp( cav->cav_ptr, "and", 3 ) == 0 )
	{
		return LDAP_COMP_FILTER_AND;

	} else if ( cav_cur_len( cav ) >= 6 &&
		strncmp( cav->cav_ptr, "or" , 2 ) == 0 )
	{
		return LDAP_COMP_FILTER_OR;

	} else if ( cav_cur_len( cav ) >= 7 &&
		strncmp( cav->cav_ptr, "not", 3 ) == 0 )
	{
		return LDAP_COMP_FILTER_NOT;

	} else {
		return LDAP_COMP_FILTER_UNDEFINED;
	}
}

static ber_tag_t
comp_next_element( ComponentAssertionValue* cav )
{
	eat_whsp( cav );
	if ( *(cav->cav_ptr) == ',' ) {
		/* move pointer to the next CA */
		cav->cav_ptr++;
		return comp_first_element( cav );
	}
	else return LDAP_COMP_FILTER_UNDEFINED;
}

static int
get_comp_filter_list( Operation *op, ComponentAssertionValue *cav,
	ComponentFilter** f, const char** text )
{
	ComponentFilter **new;
	int		err;
	ber_tag_t	tag;

	Debug( LDAP_DEBUG_FILTER, "get_comp_filter_list\n", 0, 0, 0 );
	new = f;
	for ( tag = comp_first_element( cav );
		tag != LDAP_COMP_FILTER_UNDEFINED;
		tag = comp_next_element( cav ) )
	{
		err = parse_comp_filter( op, cav, new, text );
		if ( err != LDAP_SUCCESS ) return ( err );
		new = &(*new)->cf_next;
	}
	*new = NULL;

	return( LDAP_SUCCESS );
}

static int
get_componentId( Operation *op, ComponentAssertionValue* cav,
	ComponentId ** cid, const char** text )
{
	ber_tag_t type;
	ComponentId _cid;
	int len;

	type = peek_componentId_type( cav );

	Debug( LDAP_DEBUG_FILTER, "get_compId [%lu]\n",
		(unsigned long) type, 0, 0 );
	len = 0;
	_cid.ci_type = type;
	_cid.ci_next = NULL;
	switch ( type ) {
	case LDAP_COMPREF_IDENTIFIER :
		_cid.ci_val.ci_identifier.bv_val = cav->cav_ptr;
		for( ;cav->cav_ptr[len] != ' ' && cav->cav_ptr[len] != '\0' &&
			cav->cav_ptr[len] != '.' && cav->cav_ptr[len] != '\"' ; len++ );
		_cid.ci_val.ci_identifier.bv_len = len;
		cav->cav_ptr += len;
		break;
	case LDAP_COMPREF_FROM_BEGINNING :
		for( ;cav->cav_ptr[len] != ' ' && cav->cav_ptr[len] != '\0' &&
			cav->cav_ptr[len] != '.' && cav->cav_ptr[len] != '\"' ; len++ );
		_cid.ci_val.ci_from_beginning = strtol( cav->cav_ptr, NULL, 0 );
		cav->cav_ptr += len;
		break;
	case LDAP_COMPREF_FROM_END :
		for( ;cav->cav_ptr[len] != ' ' && cav->cav_ptr[len] != '\0' &&
			cav->cav_ptr[len] != '.' && cav->cav_ptr[len] != '\"' ; len++ );
		_cid.ci_val.ci_from_end = strtol( cav->cav_ptr, NULL, 0 );
		cav->cav_ptr += len;
		break;
	case LDAP_COMPREF_COUNT :
		_cid.ci_val.ci_count = 0;
		cav->cav_ptr++;
		break;
	case LDAP_COMPREF_CONTENT :
		_cid.ci_val.ci_content = 1;
		cav->cav_ptr += strlen("content");
		break;
	case LDAP_COMPREF_SELECT :
		if ( cav->cav_ptr[len] != '(' ) return LDAP_COMPREF_UNDEFINED;
		for( ;cav->cav_ptr[len] != ' ' && cav->cav_ptr[len] != '\0' &&
	  	      cav->cav_ptr[len] != '\"' && cav->cav_ptr[len] != ')'
			; len++ );
		_cid.ci_val.ci_select_value.bv_val = cav->cav_ptr + 1;
		_cid.ci_val.ci_select_value.bv_len = len - 1 ;
		cav->cav_ptr += len + 1;
		break;
	case LDAP_COMPREF_ALL :
		_cid.ci_val.ci_all = '*';
		cav->cav_ptr++;
		break;
	default :
		return LDAP_COMPREF_UNDEFINED;
	}

	if ( op ) {
		*cid = op->o_tmpalloc( sizeof( ComponentId ), op->o_tmpmemctx );
	} else {
		*cid = SLAP_MALLOC( sizeof( ComponentId ) );
	}
	if (*cid == NULL) {
		return LDAP_NO_MEMORY;
	}
	**cid = _cid;
	return LDAP_SUCCESS;
}

static int
peek_componentId_type( ComponentAssertionValue* cav )
{
	eat_whsp( cav );

	if ( cav->cav_ptr[0] == '-' ) {
		return LDAP_COMPREF_FROM_END;

	} else if ( cav->cav_ptr[0] == '(' ) {
		return LDAP_COMPREF_SELECT;

	} else if ( cav->cav_ptr[0] == '*' ) {
		return LDAP_COMPREF_ALL;

	} else if ( cav->cav_ptr[0] == '0' ) {
		return LDAP_COMPREF_COUNT;

	} else if ( cav->cav_ptr[0] > '0' && cav->cav_ptr[0] <= '9' ) {
		return LDAP_COMPREF_FROM_BEGINNING;

	} else if ( (cav->cav_end - cav->cav_ptr) >= 7 &&
		strncmp(cav->cav_ptr,"content",7) == 0 )
	{
		return LDAP_COMPREF_CONTENT;
	} else if ( (cav->cav_ptr[0] >= 'a' && cav->cav_ptr[0] <= 'z') ||
			(cav->cav_ptr[0] >= 'A' && cav->cav_ptr[0] <= 'Z') )
	{
		return LDAP_COMPREF_IDENTIFIER;
	}

	return LDAP_COMPREF_UNDEFINED;
}

static ber_tag_t
comp_next_id( ComponentAssertionValue* cav )
{
	if ( *(cav->cav_ptr) == '.' ) {
		cav->cav_ptr++;
		return LDAP_COMPREF_DEFINED;
	}

	return LDAP_COMPREF_UNDEFINED;
}



static int
get_component_reference(
	Operation *op,
	ComponentAssertionValue* cav,
	ComponentReference** cr,
	const char** text )
{
	int rc, count = 0;
	ber_int_t type;
	ComponentReference* ca_comp_ref;
	ComponentId** cr_list;
	char* start, *end;

	eat_whsp( cav );

	start = cav->cav_ptr;
	if ( ( rc = strip_cav_str( cav,"\"") ) != LDAP_SUCCESS ) return rc;
	if ( op ) {
		ca_comp_ref = op->o_tmpalloc( sizeof( ComponentReference ),
			op->o_tmpmemctx );
	} else {
		ca_comp_ref = SLAP_MALLOC( sizeof( ComponentReference ) );
	}

	if ( !ca_comp_ref ) return LDAP_NO_MEMORY;

	cr_list = &ca_comp_ref->cr_list;

	for ( type = peek_componentId_type( cav ) ; type != LDAP_COMPREF_UNDEFINED
		; type = comp_next_id( cav ), count++ )
	{
		rc = get_componentId( op, cav, cr_list, text );
		if ( rc == LDAP_SUCCESS ) {
			if ( count == 0 ) ca_comp_ref->cr_curr = ca_comp_ref->cr_list;
			cr_list = &(*cr_list)->ci_next;

		} else if ( rc == LDAP_COMPREF_UNDEFINED ) {
			if ( op ) {
				op->o_tmpfree( ca_comp_ref , op->o_tmpmemctx );
			} else {
				free( ca_comp_ref );
			}
			return rc;
		}
	}
	ca_comp_ref->cr_len = count;
	end = cav->cav_ptr;
	if ( ( rc = strip_cav_str( cav,"\"") ) != LDAP_SUCCESS ) {
		if ( op ) {
			op->o_tmpfree( ca_comp_ref , op->o_tmpmemctx );
		} else {
			free( ca_comp_ref );
		}
		return rc;
	}

	*cr = ca_comp_ref;
	**cr = *ca_comp_ref;	

	(*cr)->cr_string.bv_val = start;
	(*cr)->cr_string.bv_len = end - start + 1;
	
	return rc;
}

int
insert_component_reference(
	ComponentReference *cr,
	ComponentReference** cr_list)
{
	if ( !cr ) return LDAP_PARAM_ERROR;

	if ( !(*cr_list) ) {
		*cr_list = cr;
		cr->cr_next = NULL;
	} else {
		cr->cr_next = *cr_list;
		*cr_list = cr;
	}
	return LDAP_SUCCESS;
}

/*
 * If there is '.' in the name of a given attribute
 * the first '.'- following characters are considered
 * as a component reference of the attribute
 * EX) userCertificate.toBeSigned.serialNumber
 * attribute : userCertificate
 * component reference : toBeSigned.serialNumber
 */
int
is_component_reference( char* attr ) {
	int i;
	for ( i=0; attr[i] != '\0' ; i++ ) {
		if ( attr[i] == '.' ) return (1);
	}
	return (0);
}

int
extract_component_reference(
	char* attr,
	ComponentReference** cr )
{
	int i, rc;
	char* cr_ptr;
	int cr_len;
	ComponentAssertionValue cav;
	char text[1][128];

	for ( i=0; attr[i] != '\0' ; i++ ) {
		if ( attr[i] == '.' ) break;
	}

	if (attr[i] != '.' ) return LDAP_PARAM_ERROR;
	attr[i] = '\0';

	cr_ptr = attr + i + 1 ;
	cr_len = strlen ( cr_ptr );
	if ( cr_len <= 0 ) return LDAP_PARAM_ERROR;

	/* enclosed between double quotes*/
	cav.cav_ptr = cav.cav_buf = ch_malloc (cr_len+2);
	memcpy( cav.cav_buf+1, cr_ptr, cr_len );
	cav.cav_buf[0] = '"';
	cav.cav_buf[cr_len+1] = '"';
	cav.cav_end = cr_ptr + cr_len + 2;

	rc = get_component_reference ( NULL, &cav, cr, (const char**)text );
	if ( rc != LDAP_SUCCESS ) return rc;
	(*cr)->cr_string.bv_val = cav.cav_buf;
	(*cr)->cr_string.bv_len = cr_len + 2;

	return LDAP_SUCCESS;
}

static int
get_ca_use_default( Operation *op,
	ComponentAssertionValue* cav,
	int* ca_use_def, const char**  text )
{
	strip_cav_str( cav, "useDefaultValues" );

	if ( peek_cav_str( cav, "TRUE" ) == LDAP_SUCCESS ) {
		strip_cav_str( cav, "TRUE" );
		*ca_use_def = 1;

	} else if ( peek_cav_str( cav, "FALSE" ) == LDAP_SUCCESS ) {
		strip_cav_str( cav, "FALSE" );
		*ca_use_def = 0;

	} else {
		return LDAP_INVALID_SYNTAX;
	}

	return LDAP_SUCCESS;
}

static int
get_matching_rule( Operation *op, ComponentAssertionValue* cav,
		MatchingRule** mr, const char**  text )
{
	int count = 0;
	struct berval rule_text = { 0L, NULL };

	eat_whsp( cav );

	for ( ; ; count++ ) {
		if ( cav->cav_ptr[count] == ' ' || cav->cav_ptr[count] == ',' ||
			cav->cav_ptr[count] == '\0' || cav->cav_ptr[count] == '{' ||
			cav->cav_ptr[count] == '}' || cav->cav_ptr[count] == '\n' )
		{
			break;
		}
	}

	if ( count == 0 ) {
		*text = "component matching rule not recognized";
		return LDAP_INAPPROPRIATE_MATCHING;
	}
	
	rule_text.bv_len = count;
	rule_text.bv_val = cav->cav_ptr;
	*mr = mr_bvfind( &rule_text );
	cav->cav_ptr += count;
	Debug( LDAP_DEBUG_FILTER, "get_matching_rule: %s\n",
		(*mr)->smr_mrule.mr_oid, 0, 0 );
	if ( *mr == NULL ) {
		*text = "component matching rule not recognized";
		return LDAP_INAPPROPRIATE_MATCHING;
	}
	return LDAP_SUCCESS;
}

static int
get_GSER_value( ComponentAssertionValue* cav, struct berval* bv )
{
	int count, sequent_dquote, unclosed_brace, succeed;

	eat_whsp( cav );
	/*
	 * Four cases of GSER <Values>
	 * 1) "..." :
	 *	StringVal, GeneralizedTimeVal, UTCTimeVal, ObjectDescriptorVal
	 * 2) '...'B or '...'H :
	 *	BitStringVal, OctetStringVal
	 * 3) {...} :
	 *	SEQUENCE, SEQUENCEOF, SETOF, SET, CHOICE
	 * 4) Between two white spaces
	 *	INTEGER, BOOLEAN, NULL,ENUMERATE, etc
	 */

	succeed = 0;
	if ( cav->cav_ptr[0] == '"' ) {
		for( count = 1, sequent_dquote = 0 ; ; count++ ) {
			/* In order to find escaped double quote */
			if ( cav->cav_ptr[count] == '"' ) sequent_dquote++;
			else sequent_dquote = 0;

			if ( cav->cav_ptr[count] == '\0' ||
				(cav->cav_ptr+count) > cav->cav_end )
			{
				break;
			}
				
			if ( ( cav->cav_ptr[count] == '"' &&
				cav->cav_ptr[count-1] != '"') ||
				( sequent_dquote > 2 && (sequent_dquote%2) == 1 ) )
			{
				succeed = 1;
				break;
			}
		}
		
		if ( !succeed || cav->cav_ptr[count] != '"' ) {
			return LDAP_FILTER_ERROR;
		}

		bv->bv_val = cav->cav_ptr + 1;
		bv->bv_len = count - 1; /* exclude '"' */

	} else if ( cav->cav_ptr[0] == '\'' ) {
		for( count = 1 ; ; count++ ) {
			if ( cav->cav_ptr[count] == '\0' ||
				(cav->cav_ptr+count) > cav->cav_end )
			{
				break;
			}
			if ((cav->cav_ptr[count-1] == '\'' && cav->cav_ptr[count] == 'B') ||
				(cav->cav_ptr[count-1] == '\'' && cav->cav_ptr[count] == 'H') )
			{
				succeed = 1;
				break;
			}
		}

		if ( !succeed ||
			!(cav->cav_ptr[count] == 'H' || cav->cav_ptr[count] == 'B') )
		{
			return LDAP_FILTER_ERROR;
		}

		bv->bv_val = cav->cav_ptr + 1;/*the next to '"' */
		bv->bv_len = count - 2;/* exclude "'H" or "'B" */
				
	} else if ( cav->cav_ptr[0] == '{' ) {
		for( count = 1, unclosed_brace = 1 ; ; count++ ) {
			if ( cav->cav_ptr[count] == '{' ) unclosed_brace++;
			if ( cav->cav_ptr[count] == '}' ) unclosed_brace--;

			if ( cav->cav_ptr[count] == '\0' ||
				(cav->cav_ptr+count) > cav->cav_end )
			{
				break;
			}
			if ( unclosed_brace == 0 ) {
				succeed = 1;
				break;
			}
		}

		if ( !succeed || cav->cav_ptr[count] != '}' ) return LDAP_FILTER_ERROR;

		bv->bv_val = cav->cav_ptr + 1;/*the next to '"' */
		bv->bv_len = count - 1;/* exclude  "'B" */

	} else {
		succeed = 1;
		/*Find  following white space where the value is ended*/
		for( count = 1 ; ; count++ ) {
			if ( cav->cav_ptr[count] == '\0' ||
				cav->cav_ptr[count] == ' ' || cav->cav_ptr[count] == '}' ||
				cav->cav_ptr[count] == '{' ||
				(cav->cav_ptr+count) > cav->cav_end )
			{
				break;
			}
		}
		bv->bv_val = cav->cav_ptr;
		bv->bv_len = count;
	}

	cav->cav_ptr += bv->bv_len;
	return LDAP_SUCCESS;
}

static int
get_matching_value( Operation *op, ComponentAssertion* ca,
	ComponentAssertionValue* cav, struct berval* bv,
	const char**  text )
{
	if ( !(ca->ca_ma_rule->smr_usage & (SLAP_MR_COMPONENT)) ) {
		if ( get_GSER_value( cav, bv ) != LDAP_SUCCESS ) {
			return LDAP_FILTER_ERROR;
		}

	} else {
		/* embeded componentFilterMatch Description */
		bv->bv_val = cav->cav_ptr;
		bv->bv_len = cav_cur_len( cav );
	}

	return LDAP_SUCCESS;
}

/* Don't move the position pointer, just peek given string */
static int
peek_cav_str( ComponentAssertionValue* cav, char* str )
{
	eat_whsp( cav );
	if ( cav_cur_len( cav ) >= strlen( str ) &&
		strncmp( cav->cav_ptr, str, strlen( str ) ) == 0 )
	{
		return LDAP_SUCCESS;
	}

	return LDAP_INVALID_SYNTAX;
}

static int
strip_cav_str( ComponentAssertionValue* cav, char* str)
{
	eat_whsp( cav );
	if ( cav_cur_len( cav ) >= strlen( str ) &&
		strncmp( cav->cav_ptr, str, strlen( str ) ) == 0 )
	{
		cav->cav_ptr += strlen( str );
		return LDAP_SUCCESS;
	}

	return LDAP_INVALID_SYNTAX;
}

/*
 * TAG : "item", "and", "or", "not"
 */
static ber_tag_t
strip_cav_tag( ComponentAssertionValue* cav )
{

	eat_whsp( cav );
	if ( cav_cur_len( cav ) >= 8 && strncmp( cav->cav_ptr, "item", 4 ) == 0 ) {
		strip_cav_str( cav , "item:" );
		return LDAP_COMP_FILTER_ITEM;

	} else if ( cav_cur_len( cav ) >= 7 &&
		strncmp( cav->cav_ptr, "and", 3 ) == 0 )
	{
		strip_cav_str( cav , "and:" );
		return LDAP_COMP_FILTER_AND;

	} else if ( cav_cur_len( cav ) >= 6 &&
		strncmp( cav->cav_ptr, "or" , 2 ) == 0 )
	{
		strip_cav_str( cav , "or:" );
		return LDAP_COMP_FILTER_OR;

	} else if ( cav_cur_len( cav ) >= 7 &&
		strncmp( cav->cav_ptr, "not", 3 ) == 0 )
	{
		strip_cav_str( cav , "not:" );
		return LDAP_COMP_FILTER_NOT;
	}

	return LBER_ERROR;
}

/*
 * when encoding, "item" is denotation of ComponentAssertion
 * ComponentAssertion :: SEQUENCE {
 *	component		ComponentReference (SIZE(1..MAX)) OPTIONAL,
 *	useDefaultValues	BOOLEAN DEFAULT TRUE,
 *	rule			MATCHING-RULE.&id,
 *	value			MATCHING-RULE.&AssertionType }
 */
static int
get_item( Operation *op, ComponentAssertionValue* cav, ComponentAssertion** ca,
		const char** text )
{
	int rc;
	ComponentAssertion* _ca;
	struct berval value;
	MatchingRule* mr;

	Debug( LDAP_DEBUG_FILTER, "get_item \n", 0, 0, 0 );
	if ( op )
		_ca = op->o_tmpalloc( sizeof( ComponentAssertion ), op->o_tmpmemctx );
	else
		_ca = SLAP_MALLOC( sizeof( ComponentAssertion ) );

	if ( !_ca ) return LDAP_NO_MEMORY;

	_ca->ca_comp_data.cd_tree = NULL;
	_ca->ca_comp_data.cd_mem_op = NULL;

	rc = peek_cav_str( cav, "component" );
	if ( rc == LDAP_SUCCESS ) {
		strip_cav_str( cav, "component" );
		rc = get_component_reference( op, cav, &_ca->ca_comp_ref, text );
		if ( rc != LDAP_SUCCESS ) {
			if ( op )
				op->o_tmpfree( _ca, op->o_tmpmemctx );
			else
				free( _ca );
			return LDAP_INVALID_SYNTAX;
		}
		if ( ( rc = strip_cav_str( cav,",") ) != LDAP_SUCCESS )
			return rc;
	} else {
		_ca->ca_comp_ref = NULL;
	}

	rc = peek_cav_str( cav, "useDefaultValues");
	if ( rc == LDAP_SUCCESS ) {
		rc = get_ca_use_default( op, cav, &_ca->ca_use_def, text );
		if ( rc != LDAP_SUCCESS ) {
			if ( op )
				op->o_tmpfree( _ca, op->o_tmpmemctx );
			else
				free( _ca );
			return LDAP_INVALID_SYNTAX;
		}
		if ( ( rc = strip_cav_str( cav,",") ) != LDAP_SUCCESS )
			return rc;
	}
	else _ca->ca_use_def = 1;

	if ( !( strip_cav_str( cav, "rule" ) == LDAP_SUCCESS &&
		get_matching_rule( op, cav , &_ca->ca_ma_rule, text ) == LDAP_SUCCESS )) {
		if ( op )
			op->o_tmpfree( _ca, op->o_tmpmemctx );
		else
			free( _ca );
		return LDAP_INAPPROPRIATE_MATCHING;
	}
	
	if ( ( rc = strip_cav_str( cav,",") ) != LDAP_SUCCESS )
		return rc;
	if ( !(strip_cav_str( cav, "value" ) == LDAP_SUCCESS &&
		get_matching_value( op, _ca, cav,&value ,text ) == LDAP_SUCCESS )) {
		if ( op )
			op->o_tmpfree( _ca, op->o_tmpmemctx );
		else
			free( _ca );
		return LDAP_INVALID_SYNTAX;
	}

	/*
	 * Normalize the value of this component assertion when the matching
	 * rule is one of existing matching rules
	 */
	mr = _ca->ca_ma_rule;
	if ( op && !(mr->smr_usage & (SLAP_MR_COMPONENT)) && mr->smr_normalize ) {

		value.bv_val[value.bv_len] = '\0';
		rc = mr->smr_normalize (
			SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
			NULL, mr,
			&value, &_ca->ca_ma_value, op->o_tmpmemctx );
		if ( rc != LDAP_SUCCESS )
			return rc;
	}
	else
		_ca->ca_ma_value = value;
	/*
	 * Validate the value of this component assertion
	 */
	if ( op && mr->smr_syntax->ssyn_validate( mr->smr_syntax, &_ca->ca_ma_value) != LDAP_SUCCESS ) {
		return LDAP_INVALID_SYNTAX;
	}


	/* componentFilterMatch contains componentFilterMatch in it */
	if ( strcmp(_ca->ca_ma_rule->smr_mrule.mr_oid, OID_COMP_FILTER_MATCH ) == 0) {
		struct berval bv;
		bv.bv_val = cav->cav_ptr;
		bv.bv_len = cav_cur_len( cav );
		rc = get_comp_filter( op, &bv,(ComponentFilter**)&_ca->ca_cf, text );
		if ( rc != LDAP_SUCCESS ) {
			if ( op )
				op->o_tmpfree( _ca, op->o_tmpmemctx );
			else
				free( _ca );
			return rc;
		}
		cav->cav_ptr = bv.bv_val;
		assert( cav->cav_end >= bv.bv_val );
	}

	*ca = _ca;
	return LDAP_SUCCESS;
}

static int
parse_comp_filter( Operation* op, ComponentAssertionValue* cav,
				ComponentFilter** filt, const char** text )
{
	/*
	 * A component filter looks like this coming in:
	 *	Filter ::= CHOICE {
	 *		item	[0]	ComponentAssertion,
	 *		and	[1]	SEQUENCE OF ComponentFilter,
	 *		or	[2]	SEQUENCE OF ComponentFilter,
	 *		not	[3]	ComponentFilter,
	 *	}
	 */

	ber_tag_t	tag;
	int		err;
	ComponentFilter	f;
	/* TAG : item, and, or, not in RFC 4515 */
	tag = strip_cav_tag( cav );

	if ( tag == LBER_ERROR ) {
		*text = "error decoding comp filter";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( tag != LDAP_COMP_FILTER_NOT )
		strip_cav_str( cav, "{");

	err = LDAP_SUCCESS;

	f.cf_next = NULL;
	f.cf_choice = tag; 

	switch ( f.cf_choice ) {
	case LDAP_COMP_FILTER_AND:
	Debug( LDAP_DEBUG_FILTER, "LDAP_COMP_FILTER_AND\n", 0, 0, 0 );
		err = get_comp_filter_list( op, cav, &f.cf_and, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}
		if ( f.cf_and == NULL ) {
			f.cf_choice = SLAPD_FILTER_COMPUTED;
			f.cf_result = LDAP_COMPARE_TRUE;
		}
		break;

	case LDAP_COMP_FILTER_OR:
	Debug( LDAP_DEBUG_FILTER, "LDAP_COMP_FILTER_OR\n", 0, 0, 0 );
		err = get_comp_filter_list( op, cav, &f.cf_or, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}
		if ( f.cf_or == NULL ) {
			f.cf_choice = SLAPD_FILTER_COMPUTED;
			f.cf_result = LDAP_COMPARE_FALSE;
		}
		/* no assert - list could be empty */
		break;

	case LDAP_COMP_FILTER_NOT:
	Debug( LDAP_DEBUG_FILTER, "LDAP_COMP_FILTER_NOT\n", 0, 0, 0 );
		err = parse_comp_filter( op, cav, &f.cf_not, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}

		assert( f.cf_not != NULL );
		if ( f.cf_not->cf_choice == SLAPD_FILTER_COMPUTED ) {
			int fresult = f.cf_not->cf_result;
			f.cf_choice = SLAPD_FILTER_COMPUTED;
			op->o_tmpfree( f.cf_not, op->o_tmpmemctx );
			f.cf_not = NULL;

			switch ( fresult ) {
			case LDAP_COMPARE_TRUE:
				f.cf_result = LDAP_COMPARE_FALSE;
				break;
			case LDAP_COMPARE_FALSE:
				f.cf_result = LDAP_COMPARE_TRUE;
				break;
			default: ;
				/* (!Undefined) is Undefined */
			}
		}
		break;

	case LDAP_COMP_FILTER_ITEM:
	Debug( LDAP_DEBUG_FILTER, "LDAP_COMP_FILTER_ITEM\n", 0, 0, 0 );
		err = get_item( op, cav, &f.cf_ca, text );
		if ( err != LDAP_SUCCESS ) {
			break;
		}

		assert( f.cf_ca != NULL );
		break;

	default:
		f.cf_choice = SLAPD_FILTER_COMPUTED;
		f.cf_result = SLAPD_COMPARE_UNDEFINED;
		break;
	}

	if ( err != LDAP_SUCCESS && err != SLAPD_DISCONNECT ) {
		*text = "Component Filter Syntax Error";
		return err;
	}

	if ( tag != LDAP_COMP_FILTER_NOT )
		strip_cav_str( cav, "}");

	if ( err == LDAP_SUCCESS ) {
		if ( op ) {
			*filt = op->o_tmpalloc( sizeof(f), op->o_tmpmemctx );
		} else {
			*filt = SLAP_MALLOC( sizeof(f) );
		}
		if ( *filt == NULL ) {
			return LDAP_NO_MEMORY;
		}
		**filt = f;
	}

	return( err );
}

static int
test_comp_filter_and(
	Syntax *syn,
	ComponentSyntaxInfo *a,
	ComponentFilter *flist )
{
	ComponentFilter *f;
	int rtn = LDAP_COMPARE_TRUE;

	for ( f = flist ; f != NULL; f = f->cf_next ) {
		int rc = test_comp_filter( syn, a, f );
		if ( rc == LDAP_COMPARE_FALSE ) {
			rtn = rc;
			break;
		}
	
		if ( rc != LDAP_COMPARE_TRUE ) {
			rtn = rc;
		}
	}

	return rtn;
}

static int
test_comp_filter_or(
	Syntax *syn,
	ComponentSyntaxInfo *a,
	ComponentFilter *flist )
{
	ComponentFilter *f;
	int rtn = LDAP_COMPARE_TRUE;

	for ( f = flist ; f != NULL; f = f->cf_next ) {
		int rc = test_comp_filter( syn, a, f );
		if ( rc == LDAP_COMPARE_TRUE ) {
			rtn = rc;
			break;
		}
	
		if ( rc != LDAP_COMPARE_FALSE ) {
			rtn = rc;
		}
	}

	return rtn;
}

int
csi_value_match( MatchingRule *mr, struct berval* bv_attr,
	struct berval* bv_assert )
{
	int rc;
	int match;

	assert( mr != NULL );
	assert( !(mr->smr_usage & SLAP_MR_COMPONENT) );

	if( !mr->smr_match ) return LDAP_INAPPROPRIATE_MATCHING;

	rc = (mr->smr_match)( &match, 0, NULL /*ad->ad_type->sat_syntax*/,
		mr, bv_attr, bv_assert );

	if ( rc != LDAP_SUCCESS ) return rc;

	return match ? LDAP_COMPARE_FALSE : LDAP_COMPARE_TRUE;
}

/*
 * return codes : LDAP_COMPARE_TRUE, LDAP_COMPARE_FALSE
 */
static int
test_comp_filter_item(
	Syntax *syn,
	ComponentSyntaxInfo *csi_attr,
	ComponentAssertion *ca )
{
	int rc;
	void *attr_nm, *assert_nm;

	if ( strcmp(ca->ca_ma_rule->smr_mrule.mr_oid,
		OID_COMP_FILTER_MATCH ) == 0 && ca->ca_cf ) {
		/* componentFilterMatch inside of componentFilterMatch */
		rc = test_comp_filter( syn, csi_attr, ca->ca_cf );
		return rc;
	}

	/* Memory for storing will-be-extracted attribute values */
	attr_nm = nibble_mem_allocator ( 1024*4 , 1024 );
	if ( !attr_nm ) return LDAP_PROTOCOL_ERROR;

	/* Memory for storing component assertion values */
	if( !ca->ca_comp_data.cd_mem_op ) {
		assert_nm = nibble_mem_allocator ( 256, 64 );
		if ( !assert_nm ) {
			nibble_mem_free ( attr_nm );
			return LDAP_PROTOCOL_ERROR;
		}
		ca->ca_comp_data.cd_mem_op = assert_nm;

	} else {
		assert_nm = ca->ca_comp_data.cd_mem_op;
	}

	/* component reference initialization */
	if ( ca->ca_comp_ref ) {
		ca->ca_comp_ref->cr_curr = ca->ca_comp_ref->cr_list;
	}
	rc = test_components( attr_nm, assert_nm, csi_attr, ca );

	/* free memory used for storing extracted attribute value */
	nibble_mem_free ( attr_nm );
	return rc;
}

static int
test_comp_filter(
    Syntax *syn,
    ComponentSyntaxInfo *a,
    ComponentFilter *f )
{
	int	rc;

	if ( !f ) return LDAP_PROTOCOL_ERROR;

	Debug( LDAP_DEBUG_FILTER, "test_comp_filter\n", 0, 0, 0 );
	switch ( f->cf_choice ) {
	case SLAPD_FILTER_COMPUTED:
		rc = f->cf_result;
		break;
	case LDAP_COMP_FILTER_AND:
		rc = test_comp_filter_and( syn, a, f->cf_and );
		break;
	case LDAP_COMP_FILTER_OR:
		rc = test_comp_filter_or( syn, a, f->cf_or );
		break;
	case LDAP_COMP_FILTER_NOT:
		rc = test_comp_filter( syn, a, f->cf_not );

		switch ( rc ) {
		case LDAP_COMPARE_TRUE:
			rc = LDAP_COMPARE_FALSE;
			break;
		case LDAP_COMPARE_FALSE:
			rc = LDAP_COMPARE_TRUE;
			break;
		}
		break;
	case LDAP_COMP_FILTER_ITEM:
		rc = test_comp_filter_item( syn, a, f->cf_ca );
		break;
	default:
		rc = LDAP_PROTOCOL_ERROR;
	}

	return( rc );
}

static void
free_comp_filter_list( ComponentFilter* f )
{
	ComponentFilter* tmp;
	for ( tmp = f; tmp; tmp = tmp->cf_next ) {
		free_comp_filter( tmp );
	}
}

static void
free_comp_filter( ComponentFilter* f )
{
	if ( !f ) {
		Debug( LDAP_DEBUG_FILTER,
			"free_comp_filter: Invalid filter so failed to release memory\n",
			0, 0, 0 );
		return;
	}
	switch ( f->cf_choice ) {
	case LDAP_COMP_FILTER_AND:
	case LDAP_COMP_FILTER_OR:
		free_comp_filter_list( f->cf_any );
		break;
	case LDAP_COMP_FILTER_NOT:
		free_comp_filter( f->cf_any );
		break;
	case LDAP_COMP_FILTER_ITEM:
		if ( nibble_mem_free && f->cf_ca->ca_comp_data.cd_mem_op ) {
			nibble_mem_free( f->cf_ca->ca_comp_data.cd_mem_op );
		}
		break;
	default:
		break;
	}
}

void
component_free( ComponentFilter *f ) {
	free_comp_filter( f );
}

void
free_ComponentData( Attribute *a ) {
	if ( a->a_comp_data->cd_mem_op )
		component_destructor( a->a_comp_data->cd_mem_op );
	free ( a->a_comp_data );
	a->a_comp_data = NULL;
}
#endif
