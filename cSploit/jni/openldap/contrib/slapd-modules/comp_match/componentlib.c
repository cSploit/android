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
#include <stdlib.h>

#include <string.h>

#ifndef SLAPD_COMP_MATCH
#define SLAPD_COMP_MATCH SLAPD_MOD_DYNAMIC
#endif

#ifdef SLAPD_COMP_MATCH
/*
 * Matching function : BIT STRING
 */
int
MatchingComponentBits ( char* oid, ComponentSyntaxInfo *csi_attr,
			ComponentSyntaxInfo *csi_assert )
{
	int rc;
        MatchingRule* mr;
        ComponentBits *a, *b;
                                                                          
        if ( oid ) {
                mr = retrieve_matching_rule(oid, (AsnTypeId)csi_attr->csi_comp_desc->cd_type_id );
                if ( mr )
                        return component_value_match( mr, csi_attr , csi_assert );
        }
        a = ((ComponentBits*)csi_attr);
        b = ((ComponentBits*)csi_assert);
	rc = ( a->value.bitLen == b->value.bitLen && 
		strncmp( a->value.bits,b->value.bits,a->value.bitLen ) == 0 );
        return rc ? LDAP_COMPARE_TRUE:LDAP_COMPARE_FALSE;
}

/*
 * Free function: BIT STRING
 */
void
FreeComponentBits ( ComponentBits* v ) {
	FreeAsnBits( &v->value );
}

/*
 * GSER Encoder : BIT STRING
 */
int
GEncComponentBits ( GenBuf *b, ComponentBits *in )
{
	GAsnBits bits = {0};

	bits.value = in->value;
	if ( !in )
		return (-1);
	return GEncAsnBitsContent ( b, &bits);
}


/*
 * GSER Decoder : BIT STRING
 */
int
GDecComponentBits ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen;
        void* component_values;
        ComponentBits* k, **k2;
	GAsnBits result;

        k = (ComponentBits*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentBits**) v;
                *k2 = (ComponentBits*) CompAlloc( mem_op, sizeof( ComponentBits ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }
        
	if ( GDecAsnBitsContent ( mem_op, b, &result, bytesDecoded ) < 0 ) {
		if ( k ) CompFree( mem_op,  k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result.value;
	k->comp_desc = get_component_description (BASICTYPE_BITSTRING);

	return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : BIT STRING
 */
int
BDecComponentBitsTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentBits ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentBits ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v,
			AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentBits* k, **k2;
	AsnBits result;
                                                                          
        k = (ComponentBits*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentBits**) v;
                *k2 = (ComponentBits*) CompAlloc( mem_op, sizeof( ComponentBits ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }
        
	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecAsnBits ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecAsnBitsContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}

	if ( rc < 0 ) {
		if ( k ) CompFree( mem_op,  k );
		return LDAP_DECODING_ERROR;
	}

	k->value = result;
	k->comp_desc = get_component_description (BASICTYPE_BITSTRING);
 
	return LDAP_SUCCESS;
}

/*
 * Component GSER BMPString Encoder
 */
int
GEncComponentBMPString ( GenBuf *b, ComponentBMPString *in )
{
	GBMPString t = {0};

	if ( !in || in->value.octetLen <= 0 )
		return (-1);
	t.value = in->value;
	return GEncBMPStringContent ( b, &t );
}

/*
 * Component GSER BMPString Decoder
 */
int
GDecComponentBMPString ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode)
{
        char* peek_head;
        int i, strLen;
        void* component_values;
        ComponentBMPString* k, **k2;
	GBMPString result;
                                                                          
        k = (ComponentBMPString*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentBMPString**) v;
                *k2 = (ComponentBMPString*) CompAlloc( mem_op, sizeof( ComponentBMPString ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

        *bytesDecoded = 0;

	if ( GDecBMPStringContent ( mem_op, b, &result, bytesDecoded ) < 0 ) {
		if ( k ) CompFree( mem_op, k );
		return LDAP_DECODING_ERROR;
	}

	k->value = result.value;
	k->comp_desc = get_component_description (BASICTYPE_BMP_STR);
 
	return LDAP_SUCCESS;

}

/*
 * Component BER BMPString Decoder
 */
int
BDecComponentBMPStringTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentBMPString ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentBMPString ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v,
			AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentBMPString* k, **k2;
	BMPString result;
                                                                          
        k = (ComponentBMPString*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentBMPString**) v;
                *k2 = (ComponentBMPString*) CompAlloc( mem_op, sizeof( ComponentBMPString ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecBMPString ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecBMPStringContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}

	if ( rc < 0 ) {
		if ( k ) CompFree( mem_op, k );
		return LDAP_DECODING_ERROR;
	}

	k->value = result;
	k->comp_desc = get_component_description (BASICTYPE_BMP_STR);
 
	return LDAP_SUCCESS;

}

/*
 * Component GSER Encoder : UTF8 String
 */
int
GEncComponentUTF8String ( GenBuf *b, ComponentUTF8String *in )
{
	GUTF8String t = {0};
	if ( !in || in->value.octetLen <= 0 )
		return (-1);
	t.value = in->value;
	return GEncUTF8StringContent ( b, &t );
}

/*
 * Component GSER Decoder :  UTF8 String
 */
int
GDecComponentUTF8String ( void* mem_op, GenBuf *b, void *v,
				AsnLen *bytesDecoded, int mode) {
        char* peek_head;
        int i, strLen;
        void* component_values;
        ComponentUTF8String* k, **k2;
	GUTF8String result;
                                                                          
        k = (ComponentUTF8String*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentUTF8String**) v;
                *k2 = (ComponentUTF8String*)CompAlloc( mem_op, sizeof( ComponentUTF8String ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

        *bytesDecoded = 0;

	if ( GDecUTF8StringContent ( mem_op, b, &result, bytesDecoded ) < 0 ) {
		if ( k ) CompFree( mem_op,  k );
		return LDAP_DECODING_ERROR;
	}
	
	k->value = result.value;
	k->comp_desc = get_component_description (BASICTYPE_UTF8_STR);
 
	return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : UTF8String
 */
int
BDecComponentUTF8StringTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentUTF8String ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentUTF8String ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len,
				void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentUTF8String* k, **k2;
	UTF8String result;
                                                                          
        k = (ComponentUTF8String*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentUTF8String**) v;
                *k2 = (ComponentUTF8String*) CompAlloc( mem_op, sizeof( ComponentUTF8String ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }
	
	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecUTF8String ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecUTF8StringContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	if ( rc < 0 ) {
		if ( k ) CompFree( mem_op,  k );
		return LDAP_DECODING_ERROR;
	}

	k->value = result;
	k->comp_desc = get_component_description (BASICTYPE_UTF8_STR);

	return LDAP_SUCCESS;
}

/*
 * Component GSER Encoder :  Teletex String
 */
int
GEncComponentTeletexString ( GenBuf *b, ComponentTeletexString *in )
{
	GTeletexString t = {0};

	if ( !in || in->value.octetLen <= 0 )
		return (-1);
	t.value = in->value;
	return GEncTeletexStringContent ( b, &t );
}

/*
 * Component GSER Decoder :  Teletex String
 */
int
GDecComponentTeletexString  ( void* mem_op, GenBuf *b, void *v,
					AsnLen *bytesDecoded, int mode) {
        char* peek_head;
        int i, strLen;
        void* component_values;
        ComponentTeletexString* k, **k2;
	GTeletexString result;
                                                                          
        k = (ComponentTeletexString*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentTeletexString**) v;
                *k2 = (ComponentTeletexString*)CompAlloc( mem_op, sizeof( ComponentTeletexString ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

        *bytesDecoded = 0;

	if ( GDecTeletexStringContent ( mem_op, b, &result, bytesDecoded ) < 0 ) {
		if ( k ) CompFree( mem_op,  k );
		return LDAP_DECODING_ERROR;
	}

	k->value = result.value;
	k->comp_desc = get_component_description (BASICTYPE_VIDEOTEX_STR);
 
	return LDAP_SUCCESS;
}


/*
 * Matching function : BOOLEAN
 */
int
MatchingComponentBool(char* oid, ComponentSyntaxInfo* csi_attr,
                        ComponentSyntaxInfo* csi_assert )
{
        MatchingRule* mr;
        ComponentBool *a, *b;
                                                                          
        if( oid ) {
                mr = retrieve_matching_rule(oid, csi_attr->csi_comp_desc->cd_type_id );
                if ( mr )
                        return component_value_match( mr, csi_attr , csi_assert );
        }

        a = ((ComponentBool*)csi_attr);
        b = ((ComponentBool*)csi_assert);

        return (a->value == b->value) ? LDAP_COMPARE_TRUE:LDAP_COMPARE_FALSE;
}

/*
 * GSER Encoder : BOOLEAN
 */
int
GEncComponentBool ( GenBuf *b, ComponentBool *in )
{
	GAsnBool t = {0};

	if ( !in )
		return (-1);
	t.value = in->value;
	return GEncAsnBoolContent ( b, &t );
}

/*
 * GSER Decoder : BOOLEAN
 */
int
GDecComponentBool ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen;
        ComponentBool* k, **k2;
	GAsnBool result;
                                                                          
        k = (ComponentBool*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentBool**) v;
                *k2 = (ComponentBool*) CompAlloc( mem_op, sizeof( ComponentBool ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( GDecAsnBoolContent( mem_op, b, &result, bytesDecoded ) < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}

	k->value = result.value;
	k->comp_desc = get_component_description (BASICTYPE_BOOLEAN);
 
        return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : BOOLEAN
 */
int
BDecComponentBoolTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentBool ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentBool ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v,
			AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        ComponentBool* k, **k2;
	AsnBool result;
                                                                          
        k = (ComponentBool*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentBool**) v;
                *k2 = (ComponentBool*) CompAlloc( mem_op, sizeof( ComponentBool ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecAsnBool ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecAsnBoolContent( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}

	k->value = result;
	k->comp_desc = get_component_description (BASICTYPE_BOOLEAN);

        return LDAP_SUCCESS;
}

/*
 * Matching function : ENUMERATE
 */
int
MatchingComponentEnum ( char* oid, ComponentSyntaxInfo *csi_attr,
			ComponentSyntaxInfo *csi_assert )
{
        int rc;
        MatchingRule* mr;
        ComponentEnum *a, *b;
                                                                          
        if( oid ) {
                mr = retrieve_matching_rule(oid, csi_attr->csi_comp_desc->cd_type_id );
                if ( mr )
                        return component_value_match( mr, csi_attr , csi_assert );
        }
        a = ((ComponentEnum*)csi_attr);
        b = ((ComponentEnum*)csi_assert);
        rc = (a->value == b->value);
                                                                          
        return rc ? LDAP_COMPARE_TRUE:LDAP_COMPARE_FALSE;
}

/*
 * GSER Encoder : ENUMERATE
 */
int
GEncComponentEnum ( GenBuf *b, ComponentEnum *in )
{
	GAsnEnum t = {0};

	if ( !in )
		return (-1);
	t.value = in->value;
	return GEncAsnEnumContent ( b, &t );
}

/*
 * GSER Decoder : ENUMERATE
 */
int
GDecComponentEnum ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen;
        void* component_values;
        ComponentEnum* k, **k2;
	GAsnEnum result;
                                                                          
        k = (ComponentEnum*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentEnum**) v;
                *k2 = (ComponentEnum*) CompAlloc( mem_op, sizeof( ComponentEnum ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( GDecAsnEnumContent ( mem_op, b, &result, bytesDecoded ) < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}

	k->value_identifier.bv_val = result.value_identifier;
	k->value_identifier.bv_len = result.len;

	k->comp_desc = CompAlloc( mem_op, sizeof( ComponentDesc ) );
	if ( !k->comp_desc )  {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->comp_desc->cd_gser_encoder = (encoder_func*)GEncComponentEnum;
	k->comp_desc->cd_gser_decoder = (gser_decoder_func*)GDecComponentEnum;
	k->comp_desc->cd_ber_decoder = (ber_decoder_func*)BDecComponentEnum;
	k->comp_desc->cd_free = (comp_free_func*)NULL;
	k->comp_desc->cd_extract_i = NULL;
	k->comp_desc->cd_type = ASN_BASIC;
	k->comp_desc->cd_type_id = BASICTYPE_ENUMERATED;
	k->comp_desc->cd_all_match = (allcomponent_matching_func*)MatchingComponentEnum;

	return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : ENUMERATE
 */
int
BDecComponentEnumTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentEnum ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentEnum ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v,
			AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentEnum* k, **k2;
	AsnEnum result;
                                                                          
        k = (ComponentEnum*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentEnum**) v;
                *k2 = (ComponentEnum*) CompAlloc( mem_op, sizeof( ComponentEnum ) );
		if ( k ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecAsnEnum ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecAsnEnumContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}

	k->value = result;

        k->comp_desc = CompAlloc( mem_op, sizeof( ComponentDesc ) );
	if ( !k->comp_desc )  {
		if ( k  ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->comp_desc->cd_gser_encoder = (encoder_func*)GEncComponentEnum;
        k->comp_desc->cd_gser_decoder = (gser_decoder_func*)GDecComponentEnum;
        k->comp_desc->cd_ber_decoder = (ber_decoder_func*)BDecComponentEnum;
	k->comp_desc->cd_free = (comp_free_func*)NULL;
        k->comp_desc->cd_extract_i = NULL;
        k->comp_desc->cd_type = ASN_BASIC;
        k->comp_desc->cd_type_id = BASICTYPE_ENUMERATED;
        k->comp_desc->cd_all_match = (allcomponent_matching_func*)MatchingComponentEnum;

	return LDAP_SUCCESS;
}

/*
 * Component GSER Encoder : IA5String
 */
int
GEncComponentIA5Stirng ( GenBuf *b, ComponentIA5String* in )
{
	GIA5String t = {0};
	t.value = in->value;
	if ( !in || in->value.octetLen <= 0 ) return (-1);
	return GEncIA5StringContent( b, &t );
}

/*
 * Component BER Decoder : IA5String
 */
int
BDecComponentIA5StringTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentIA5String ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentIA5String ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v,
			AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentIA5String* k, **k2;
	IA5String result;
                                                                          
        k = (ComponentIA5String*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentIA5String**) v;
                *k2 = (ComponentIA5String*) CompAlloc( mem_op, sizeof( ComponentIA5String ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecIA5String ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecIA5StringContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}

	k->value = result;

        k->comp_desc = CompAlloc( mem_op, sizeof( ComponentDesc ) );
	if ( !k->comp_desc )  {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->comp_desc->cd_gser_encoder = (encoder_func*)GEncComponentIA5String;
        k->comp_desc->cd_gser_decoder = (gser_decoder_func*)GDecComponentIA5String;
        k->comp_desc->cd_ber_decoder = (ber_decoder_func*)BDecComponentIA5String;
	k->comp_desc->cd_free = (comp_free_func*)FreeComponentIA5String;
        k->comp_desc->cd_extract_i = NULL;
        k->comp_desc->cd_type = ASN_BASIC;
        k->comp_desc->cd_type_id = BASICTYPE_IA5_STR;
        k->comp_desc->cd_all_match = (allcomponent_matching_func*)MatchingComponentIA5String;

	return LDAP_SUCCESS;
}

/*
 * Matching function : INTEGER
 */
int
MatchingComponentInt(char* oid, ComponentSyntaxInfo* csi_attr,
                        ComponentSyntaxInfo* csi_assert )
{
        MatchingRule* mr;
        ComponentInt *a, *b;
                                                                          
        if( oid ) {
                /* check if this ASN type's matching rule is overrided */
                mr = retrieve_matching_rule(oid, csi_attr->csi_comp_desc->cd_type_id );
                /* if existing function is overrided, call the overriding
function*/
                if ( mr )
                        return component_value_match( mr, csi_attr , csi_assert );
        }
        a = ((ComponentInt*)csi_attr);
        b = ((ComponentInt*)csi_assert);
                                                                          
        return ( a->value == b->value ) ? LDAP_COMPARE_TRUE:LDAP_COMPARE_FALSE;
}

/*
 * GSER Encoder : INTEGER
 */
int
GEncComponentInt ( GenBuf *b, ComponentInt* in )
{
	GAsnInt t = {0};

	if ( !in )
		return (-1);
	t.value = in->value;
	return GEncAsnIntContent ( b, &t );
}

/*
 * GSER Decoder : INTEGER 
 */
int
GDecComponentInt( void* mem_op, GenBuf * b, void *v, AsnLen *bytesDecoded, int mode)
{
        char* peek_head;
        int i, strLen;
        void* component_values;
        ComponentInt* k, **k2;
	GAsnInt result;
                                                                          
        k = (ComponentInt*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentInt**) v;
                *k2 = (ComponentInt*) CompAlloc( mem_op, sizeof( ComponentInt ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( GDecAsnIntContent ( mem_op, b, &result, bytesDecoded ) < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result.value;
	k->comp_desc = get_component_description (BASICTYPE_INTEGER );

        return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : INTEGER 
 */
int
BDecComponentIntTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentInt ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentInt ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v,
			AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentInt* k, **k2;
	AsnInt result;
                                                                          
        k = (ComponentInt*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentInt**) v;
                *k2 = (ComponentInt*) CompAlloc( mem_op, sizeof( ComponentInt ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecAsnInt ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecAsnIntContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	k->value = result;

	k->comp_desc = get_component_description (BASICTYPE_INTEGER );
        
        return LDAP_SUCCESS;
}

/*
 * Matching function : NULL
 */
int
MatchingComponentNull ( char *oid, ComponentSyntaxInfo *csi_attr,
			ComponentSyntaxInfo *csi_assert )
{
        MatchingRule* mr;
        ComponentNull *a, *b;
                                                                          
        if( oid ) {
                mr = retrieve_matching_rule(oid, csi_attr->csi_comp_desc->cd_type_id );
                if ( mr )
                        return component_value_match( mr, csi_attr , csi_assert );
        }
        a = ((ComponentNull*)csi_attr);
        b = ((ComponentNull*)csi_assert);
                                                                          
        return (a->value == b->value) ? LDAP_COMPARE_TRUE:LDAP_COMPARE_FALSE;
}

/*
 * GSER Encoder : NULL
 */
int
GEncComponentNull ( GenBuf *b, ComponentNull *in )
{
	GAsnNull t = {0};

	if ( !in )
		return (-1);
	t.value = in->value;
	return GEncAsnNullContent ( b, &t );
}

/*
 * GSER Decoder : NULL
 */
int
GDecComponentNull ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen;
        void* component_values;
        ComponentNull* k, **k2;
	GAsnNull result;
                                                                          
        k = (ComponentNull*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentNull**) v;
                *k2 = (ComponentNull*) CompAlloc( mem_op, sizeof( ComponentNull ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( GDecAsnNullContent ( mem_op, b, &result, bytesDecoded ) < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result.value;

	k->comp_desc = CompAlloc( mem_op, sizeof( ComponentDesc ) );
	if ( !k->comp_desc )  {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->comp_desc->cd_gser_encoder = (encoder_func*)GEncComponentNull;
	k->comp_desc->cd_gser_decoder = (gser_decoder_func*)GDecComponentNull;
	k->comp_desc->cd_ber_decoder = (ber_decoder_func*)BDecComponentNull;
	k->comp_desc->cd_free = (comp_free_func*)FreeComponentNull;
	k->comp_desc->cd_extract_i = NULL;
	k->comp_desc->cd_type = ASN_BASIC;
	k->comp_desc->cd_type_id = BASICTYPE_NULL;
	k->comp_desc->cd_all_match = (allcomponent_matching_func*)MatchingComponentNull;

	return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : NULL
 */
int
BDecComponentNullTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode )
{
	return BDecComponentNull ( mem_op, b, 0, 0, v,bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentNull ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v,
			AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentNull* k, **k2;
	AsnNull result;

        k = (ComponentNull*) v;
                                                                         
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentNull**) v;
                *k2 = (ComponentNull*) CompAlloc( mem_op, sizeof( ComponentNull ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecAsnNull ( mem_op, b, &result, bytesDecoded );
	}
	else {
		rc = BDecAsnNullContent ( mem_op, b, tagId, len, &result, bytesDecoded);
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result;

	k->comp_desc = CompAlloc( mem_op, sizeof( ComponentDesc ) );
	if ( !k->comp_desc )  {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->comp_desc->cd_gser_encoder = (encoder_func*)GEncComponentNull;
	k->comp_desc->cd_gser_decoder = (gser_decoder_func*)GDecComponentNull;
	k->comp_desc->cd_ber_decoder = (ber_decoder_func*)BDecComponentNull;
	k->comp_desc->cd_free = (comp_free_func*)FreeComponentNull;
	k->comp_desc->cd_extract_i = NULL;
	k->comp_desc->cd_type = ASN_BASIC;
	k->comp_desc->cd_type_id = BASICTYPE_NULL;
	k->comp_desc->cd_all_match = (allcomponent_matching_func*)MatchingComponentNull;
	return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : NumericString
 */
int
BDecComponentNumericStringTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentNumericString ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentNumericString ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentNumericString* k, **k2;
	NumericString result;

        k = (ComponentNumericString*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentNumericString**) v;
                *k2 = (ComponentNumericString*) CompAlloc( mem_op, sizeof( ComponentNumericString ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecNumericString ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecNumericStringContent ( mem_op, b, tagId, len, &result, bytesDecoded);
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result;

        k->comp_desc = CompAlloc( mem_op, sizeof( ComponentDesc ) );
	if ( !k->comp_desc )  {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->comp_desc->cd_gser_encoder = (encoder_func*)GEncComponentNumericString;
        k->comp_desc->cd_gser_decoder = (gser_decoder_func*)GDecComponentNumericString;
        k->comp_desc->cd_ber_decoder = (ber_decoder_func*)BDecComponentNumericString;
	k->comp_desc->cd_free = (comp_free_func*)FreeComponentNumericString;
        k->comp_desc->cd_extract_i = NULL;
        k->comp_desc->cd_type = ASN_BASIC;
        k->comp_desc->cd_type_id = BASICTYPE_NUMERIC_STR;
        k->comp_desc->cd_all_match = (allcomponent_matching_func*)MatchingComponentNumericString;

	return LDAP_SUCCESS;
}


/*
 * Free function : OCTET STRING
 */
void
FreeComponentOcts ( ComponentOcts* v) {
	FreeAsnOcts( &v->value );
}

/*
 * Matching function : OCTET STRING
 */
int
MatchingComponentOcts ( char* oid, ComponentSyntaxInfo* csi_attr,
			ComponentSyntaxInfo* csi_assert )
{
        int rc;
        MatchingRule* mr;
        ComponentOcts *a, *b;
                                                                          
        if( oid ) {
                mr = retrieve_matching_rule(oid, csi_attr->csi_comp_desc->cd_type_id );
                if ( mr )
                        return component_value_match( mr, csi_attr , csi_assert );
        }
        a = (ComponentOcts*) csi_attr;
        b = (ComponentOcts*) csi_assert;
	/* Assume that both of OCTET string has end of string character */
	if ( (a->value.octetLen == b->value.octetLen) &&
		strncmp ( a->value.octs, b->value.octs, a->value.octetLen ) == 0 )
        	return LDAP_COMPARE_TRUE;
	else
		return LDAP_COMPARE_FALSE;
}

/*
 * GSER Encoder : OCTET STRING
 */
int
GEncComponentOcts ( GenBuf* b, ComponentOcts *in )
{
	GAsnOcts t = {0};
	if ( !in || in->value.octetLen <= 0 )
		return (-1);

	t.value = in->value;
	return GEncAsnOctsContent ( b, &t );
}

/*
 * GSER Decoder : OCTET STRING
 */
int
GDecComponentOcts ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode )
{
        char *peek_head, *data;
        int i, j, strLen;
        void* component_values;
        ComponentOcts* k, **k2;
	GAsnOcts result;
                                                                          
        k = (ComponentOcts*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentOcts**) v;
                *k2 = (ComponentOcts*) CompAlloc( mem_op, sizeof( ComponentOcts ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( GDecAsnOctsContent ( mem_op, b, &result, bytesDecoded ) < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result.value;

	k->comp_desc = CompAlloc( mem_op, sizeof( ComponentDesc ) );
	if ( !k->comp_desc )  {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->comp_desc->cd_gser_encoder = (encoder_func*)GEncComponentOcts;
	k->comp_desc->cd_gser_decoder = (gser_decoder_func*)GDecComponentOcts;
	k->comp_desc->cd_ber_decoder = (ber_decoder_func*)BDecComponentOcts;
	k->comp_desc->cd_free = (comp_free_func*)FreeComponentOcts;
	k->comp_desc->cd_extract_i = NULL;
	k->comp_desc->cd_type = ASN_BASIC;
	k->comp_desc->cd_type_id = BASICTYPE_OCTETSTRING;
	k->comp_desc->cd_all_match = (allcomponent_matching_func*)MatchingComponentOcts;

	return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : OCTET STRING
 */
int
BDecComponentOctsTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentOcts ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentOcts ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v,
			AsnLen *bytesDecoded, int mode )
{
        char *peek_head, *data;
        int i, strLen, rc;
        void* component_values;
        ComponentOcts* k, **k2;
	AsnOcts result;
                                                                          
        k = (ComponentOcts*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentOcts**) v;
                *k2 = (ComponentOcts*) CompAlloc( mem_op, sizeof( ComponentOcts ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecAsnOcts ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecAsnOctsContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result;

        k->comp_desc = CompAlloc( mem_op, sizeof( ComponentDesc ) );
	if ( !k->comp_desc )  {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->comp_desc->cd_gser_encoder = (encoder_func*)GEncComponentOcts;
        k->comp_desc->cd_gser_decoder = (gser_decoder_func*)GDecComponentOcts;
        k->comp_desc->cd_ber_decoder = (ber_decoder_func*)BDecComponentOcts;
	k->comp_desc->cd_free = (comp_free_func*)FreeComponentOcts;
        k->comp_desc->cd_extract_i = NULL;
        k->comp_desc->cd_type = ASN_BASIC;
        k->comp_desc->cd_type_id = BASICTYPE_OCTETSTRING;
        k->comp_desc->cd_all_match = (allcomponent_matching_func*)MatchingComponentOcts;
	return LDAP_SUCCESS;
}

/*
 * Matching function : OBJECT IDENTIFIER
 */
int
MatchingComponentOid ( char *oid, ComponentSyntaxInfo *csi_attr ,
			ComponentSyntaxInfo *csi_assert )
{
        int rc;
        MatchingRule* mr;
        ComponentOid *a, *b;
                                                                          
        if( oid ) {
                mr = retrieve_matching_rule(oid, csi_attr->csi_comp_desc->cd_type_id );
                if ( mr )
                        return component_value_match( mr, csi_attr , csi_assert );
        }

        a = (ComponentOid*)csi_attr;
        b = (ComponentOid*)csi_assert;
	if ( a->value.octetLen != b->value.octetLen )
		return LDAP_COMPARE_FALSE;
        rc = ( strncmp( a->value.octs, b->value.octs, a->value.octetLen ) == 0 );
                                                                          
        return rc ? LDAP_COMPARE_TRUE:LDAP_COMPARE_FALSE;
}

/*
 * GSER Encoder : OID
 */
GEncComponentOid ( GenBuf *b, ComponentOid *in )
{
	GAsnOid t = {0};

	if ( !in || in->value.octetLen <= 0 )
		return (-1);
	t.value = in->value;
	return GEncAsnOidContent( b, (GAsnOcts*)&t );
}

/*
 * GSER Decoder : OID
 */
int
GDecAsnDescOidContent ( void* mem_op, GenBuf *b, GAsnOid *result, AsnLen *bytesDecoded ){
	AttributeType *ad_type;
	struct berval name;
	char* peek_head;
	int strLen;

	strLen = LocateNextGSERToken ( mem_op, b, &peek_head, GSER_NO_COPY );
	name.bv_val = peek_head;
	name.bv_len = strLen;

	ad_type = at_bvfind( &name );

	if ( !ad_type )
		return LDAP_DECODING_ERROR;

	peek_head = ad_type->sat_atype.at_oid;
	strLen = strlen ( peek_head );

	result->value.octs = (char*)EncodeComponentOid ( mem_op, peek_head , &strLen );
	result->value.octetLen = strLen;
	return LDAP_SUCCESS;
}

int
IsNumericOid ( char* peek_head , int strLen ) {
	int i;
	int num_dot;
	for ( i = 0, num_dot = 0 ; i < strLen ; i++ ) {
		if ( peek_head[i] == '.' ) num_dot++;
		else if ( peek_head[i] > '9' || peek_head[i] < '0' )
			return (-1);
	}
	if ( num_dot )
		return (1);
	else
		return (-1);
}

int
GDecComponentOid ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentOid* k, **k2;
	GAsnOid result;
                                                                          
        k = (ComponentOid*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentOid**) v;
                *k2 = (ComponentOid*) CompAlloc( mem_op, sizeof( ComponentOid ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	strLen = LocateNextGSERToken ( mem_op, b, &peek_head, GSER_PEEK );
	if ( IsNumericOid ( peek_head , strLen ) >= 1 ) {
		/* numeric-oid */
		if ( GDecAsnOidContent ( mem_op, b, &result, bytesDecoded ) < 0 ) {
			if ( k ) CompFree ( mem_op, k );
			return LDAP_DECODING_ERROR;
		}
	}
	else {
		/*descr*/
		if ( GDecAsnDescOidContent ( mem_op, b, &result, bytesDecoded ) < 0 ){
			if ( k ) CompFree ( mem_op, k );
			return LDAP_DECODING_ERROR;
		}
	}
	k->value = result.value;
	k->comp_desc = get_component_description (BASICTYPE_OID);

	return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : OID
 */
int
BDecComponentOidTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentOid ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentOid ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v,
			AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentOid* k, **k2;
	AsnOid result;
                                                                          
        k = (ComponentOid*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentOid**) v;
                *k2 = (ComponentOid*) CompAlloc( mem_op, sizeof( ComponentOid ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }
	
	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecAsnOid ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecAsnOidContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result;

	k->comp_desc = get_component_description (BASICTYPE_OID);

	return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : PrintiableString
 */

int
BDecComponentPrintableStringTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode )
{
	return BDecComponentPrintableString ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentPrintableString( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentPrintableString* k, **k2;
	AsnOid result;
                                                                          
        k = (ComponentPrintableString*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentPrintableString**) v;
                *k2 = (ComponentPrintableString*) CompAlloc( mem_op, sizeof( ComponentPrintableString ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( mode & CALL_TAG_DECODER ) {
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecPrintableString ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecPrintableStringContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result;

	k->comp_desc = get_component_description (BASICTYPE_PRINTABLE_STR);

	return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : TeletexString
 */

int
BDecComponentTeletexStringTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode )
{
	return BDecComponentTeletexString ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentTeletexString( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentTeletexString* k, **k2;
	AsnOid result;
                                                                          
        k = (ComponentTeletexString*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentTeletexString**) v;
                *k2 = (ComponentTeletexString*) CompAlloc( mem_op, sizeof( ComponentTeletexString ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( mode & CALL_TAG_DECODER ) {
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecTeletexString ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecTeletexStringContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result;

	k->comp_desc = get_component_description (BASICTYPE_T61_STR);

	return LDAP_SUCCESS;
}


/*
 * Matching function : Real
 */
int
MatchingComponentReal (char* oid, ComponentSyntaxInfo *csi_attr,
			ComponentSyntaxInfo *csi_assert )
{
        int rc;
        MatchingRule* mr;
        ComponentReal *a, *b;
                                                                          
        if( oid ) {
                mr = retrieve_matching_rule(oid, csi_attr->csi_comp_desc->cd_type_id );
                if ( mr )
                        return component_value_match( mr, csi_attr , csi_assert );
        }
        a = (ComponentReal*)csi_attr;
        b = (ComponentReal*)csi_assert;
        rc = (a->value == b->value);
                                                                          
        return rc ? LDAP_COMPARE_TRUE:LDAP_COMPARE_FALSE;
}

/*
 * GSER Encoder : Real
 */
int
GEncComponentReal ( GenBuf *b, ComponentReal *in )
{
	GAsnReal t = {0};
	if ( !in )
		return (-1);
	t.value = in->value;
	return GEncAsnRealContent ( b, &t );
}

/*
 * GSER Decoder : Real
 */
int
GDecComponentReal ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen;
        void* component_values;
        ComponentReal* k, **k2;
	GAsnReal result;
                                                                          
        k = (ComponentReal*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentReal**) v;
                *k2 = (ComponentReal*) CompAlloc( mem_op, sizeof( ComponentReal ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( GDecAsnRealContent ( mem_op, b, &result, bytesDecoded ) < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result.value;
	k->comp_desc = get_component_description (BASICTYPE_REAL);

        return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : Real
 */
int
BDecComponentRealTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentReal ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentReal ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentReal* k, **k2;
	AsnReal result;
                                                                          
        k = (ComponentReal*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentReal**) v;
                *k2 = (ComponentReal*) CompAlloc( mem_op, sizeof( ComponentReal ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }

	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecAsnReal ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecAsnRealContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result;
	k->comp_desc = get_component_description (BASICTYPE_REAL);

        return LDAP_SUCCESS;
}

/*
 * Matching function : Relative OID
 */
int
MatchingComponentRelativeOid ( char* oid, ComponentSyntaxInfo *csi_attr,
					ComponentSyntaxInfo *csi_assert )
{
        int rc;
        MatchingRule* mr;
        ComponentRelativeOid *a, *b;
                                                                          
        if( oid ) {
                mr = retrieve_matching_rule(oid, csi_attr->csi_comp_desc->cd_type_id );
                if ( mr )
                        return component_value_match( mr, csi_attr , csi_assert );
        }

        a = (ComponentRelativeOid*)csi_attr;
        b = (ComponentRelativeOid*)csi_assert;

	if ( a->value.octetLen != b->value.octetLen )
		return LDAP_COMPARE_FALSE;
        rc = ( strncmp( a->value.octs, b->value.octs, a->value.octetLen ) == 0 );
                                                                          
        return rc ? LDAP_COMPARE_TRUE:LDAP_COMPARE_FALSE;
}

/*
 * GSER Encoder : RELATIVE_OID.
 */
int
GEncComponentRelativeOid ( GenBuf *b, ComponentRelativeOid *in )
{
	GAsnRelativeOid t = {0};

	if ( !in || in->value.octetLen <= 0 )
		return (-1);
	t.value = in->value;
	return GEncAsnRelativeOidContent ( b , (GAsnOcts*)&t );
}

/*
 * GSER Decoder : RELATIVE_OID.
 */
int
GDecComponentRelativeOid ( void* mem_op, GenBuf *b,void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen;
        void* component_values;
        ComponentRelativeOid* k, **k2;
	GAsnRelativeOid result;
                                                                          
        k = (ComponentRelativeOid*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentRelativeOid**) v;
                *k2 = (ComponentRelativeOid*) CompAlloc( mem_op, sizeof( ComponentRelativeOid ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }
	
	if ( GDecAsnRelativeOidContent ( mem_op, b, &result, bytesDecoded ) < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result.value;
	k->comp_desc = get_component_description (BASICTYPE_OID);

	return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : RELATIVE_OID.
 */
int
BDecComponentRelativeOidTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentRelativeOid ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentRelativeOid ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentRelativeOid* k, **k2;
	AsnRelativeOid result;
                                                                          
        k = (ComponentRelativeOid*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentRelativeOid**) v;
                *k2 = (ComponentRelativeOid*) CompAlloc( mem_op, sizeof( ComponentRelativeOid ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }
	
	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecAsnRelativeOid ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecAsnRelativeOidContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result;
	k->comp_desc = get_component_description (BASICTYPE_OID);

	return LDAP_SUCCESS;
}

/*
 * GSER Encoder : UniversalString
 */
int
GEncComponentUniversalString ( GenBuf *b, ComponentUniversalString *in )
{
	GUniversalString t = {0};
	if ( !in || in->value.octetLen <= 0 )
		return (-1);
	t.value = in->value;
	return GEncUniversalStringContent( b, &t );
}

/*
 * GSER Decoder : UniversalString
 */
static int
UTF8toUniversalString( char* octs, int len){
	/* Need to be Implemented */
	return LDAP_SUCCESS;
}

int
GDecComponentUniversalString ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode )
{
	if ( GDecComponentUTF8String ( mem_op, b, v, bytesDecoded, mode) < 0 )
	UTF8toUniversalString( ((ComponentUniversalString*)v)->value.octs, ((ComponentUniversalString*)v)->value.octetLen );
		return LDAP_DECODING_ERROR;
}

/*
 * Component BER Decoder : UniverseString
 */
int
BDecComponentUniversalStringTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentUniversalString ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentUniversalString ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentUniversalString* k, **k2;
	UniversalString result;

        k = (ComponentUniversalString*) v;

        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentUniversalString**) v;
                *k2 = (ComponentUniversalString*) CompAlloc( mem_op, sizeof( ComponentUniversalString ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }
	
	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecUniversalString ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecUniversalStringContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	if ( rc < 0 ) {
		if ( k ) CompFree ( mem_op, k );
		return LDAP_DECODING_ERROR;
	}
	k->value = result;
	k->comp_desc = get_component_description (BASICTYPE_UNIVERSAL_STR);

	return LDAP_SUCCESS;
}

/*
 * Component BER Decoder : VisibleString
 */
int
BDecComponentVisibleStringTag ( void* mem_op, GenBuf *b, void *v, AsnLen *bytesDecoded, int mode ) {
	return BDecComponentVisibleString ( mem_op, b, 0, 0, v, bytesDecoded, mode|CALL_TAG_DECODER );
}

int
BDecComponentVisibleString ( void* mem_op, GenBuf *b, AsnTag tagId, AsnLen len, void *v, AsnLen *bytesDecoded, int mode )
{
        char* peek_head;
        int i, strLen, rc;
        void* component_values;
        ComponentVisibleString* k, **k2;
	VisibleString result;
                                                                          
        k = (ComponentVisibleString*) v;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentVisibleString**) v;
                *k2 = (ComponentVisibleString*) CompAlloc( mem_op, sizeof( ComponentVisibleString ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }
	
	if ( mode & CALL_TAG_DECODER ){
		mode = mode & CALL_CONTENT_DECODER;
		rc = BDecVisibleString ( mem_op, b, &result, bytesDecoded );
	} else {
		rc = BDecVisibleStringContent ( mem_op, b, tagId, len, &result, bytesDecoded );
	}
	k->value = result;
	k->comp_desc = get_component_description (BASICTYPE_VISIBLE_STR);

	return LDAP_SUCCESS;
}

/*
 * Routines for handling an ANY DEFINED Type
 */

/* Check if the <select> type CR and the OID of the given ANY type */
int
CheckSelectTypeCorrect ( void* mem_op, ComponentAnyInfo* cai, struct berval* select ) {
	int strLen;
	AttributeType* ad_type;
	char* oid;
	char* result;

	if ( IsNumericOid ( select->bv_val , select->bv_len ) ) {
		oid = select->bv_val;
		strLen = select->bv_len;
	} else {
		ad_type = at_bvfind( select );

		if ( !ad_type )
			return LDAP_DECODING_ERROR;

		oid = ad_type->sat_atype.at_oid;
		strLen = strlen ( oid );
	}
	result = EncodeComponentOid ( mem_op, oid , &strLen );
	if ( !result || strLen <= 0 ) return (-1);

	if ( cai->oid.octetLen == strLen &&
		strncmp ( cai->oid.octs, result, strLen ) == 0 )
		return (1);
	else
		return (-1);
}

int
SetAnyTypeByComponentOid ( ComponentAny *v, ComponentOid *id ) {
	Hash hash;
	void *anyInfo;

	/* use encoded oid as hash string */
	hash = MakeHash (id->value.octs, id->value.octetLen);
	if (CheckForAndReturnValue (anyOidHashTblG, hash, &anyInfo))
		v->cai = (ComponentAnyInfo*) anyInfo;
	else
		v->cai = NULL;

	if ( !v->cai ) {
	/*
	 * If not found, the data considered as octet chunk
	 * Yet-to-be-Implemented
	 */
	}
	return LDAP_SUCCESS;
}

void
SetAnyTypeByComponentInt( ComponentAny *v, ComponentInt id) {
	Hash hash;
	void *anyInfo;

	hash = MakeHash ((char*)&id, sizeof (id));
	if (CheckForAndReturnValue (anyIntHashTblG, hash, &anyInfo))
		v->cai = (ComponentAnyInfo*) anyInfo;
	else
		v->cai = NULL;
}

int
GEncComponentAny ( GenBuf *b, ComponentAny *in )
{
	if ( in->cai != NULL  && in->cai->Encode != NULL )
		return in->cai->Encode(b, &in->value );
	else
		return (-1);
}

int
BEncComponentAny ( void* mem_op, GenBuf *b, ComponentAny *result, AsnLen *bytesDecoded, int mode)
{
        ComponentAny *k, **k2;
                                                                          
        k = (ComponentAny*) result;

	if ( !k ) return (-1);
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentAny**) result;
                *k2 = (ComponentAny*) CompAlloc( mem_op, sizeof( ComponentAny ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }
	
	if ((result->cai != NULL) && (result->cai->BER_Decode != NULL)) {
		result->value = (void*) CompAlloc ( mem_op, result->cai->size );
		if ( !result->value ) return 0;
		result->cai->BER_Decode ( mem_op, b, result->value, (int*)bytesDecoded, DEC_ALLOC_MODE_1);

		k->comp_desc = CompAlloc( mem_op, sizeof( ComponentDesc ) );
		if ( !k->comp_desc )  {
			if ( k ) CompFree ( mem_op, k );
			return LDAP_DECODING_ERROR;
		}
		k->comp_desc->cd_gser_encoder = (encoder_func*)GEncComponentAny;
		k->comp_desc->cd_gser_decoder = (gser_decoder_func*)GDecComponentAny;
		k->comp_desc->cd_ber_decoder = (ber_decoder_func*)BDecComponentAny;
		k->comp_desc->cd_free = (comp_free_func*)FreeComponentAny;
		k->comp_desc->cd_extract_i = NULL;
		k->comp_desc->cd_type = ASN_BASIC;
		k->comp_desc->cd_type_id = BASICTYPE_ANY;
		k->comp_desc->cd_all_match = (allcomponent_matching_func*)MatchingComponentAny;
		return LDAP_SUCCESS;
	}
	else {
		Asn1Error ("ERROR - Component ANY Decode routine is NULL\n");
		return 0;
	}
}

int
BDecComponentAny ( void* mem_op, GenBuf *b, ComponentAny *result, AsnLen *bytesDecoded, int mode) {
	int rc;
        ComponentAny *k, **k2;
                                                                          
        k = (ComponentAny*) result;

	if ( !k ) return (-1);
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentAny**) result;
                *k2 = (ComponentAny*) CompAlloc( mem_op, sizeof( ComponentAny ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }
	
	if ((result->cai != NULL) && (result->cai->BER_Decode != NULL)) {
		result->cai->BER_Decode ( mem_op, b, (ComponentSyntaxInfo*)&result->value, (int*)bytesDecoded, DEC_ALLOC_MODE_0 );

		k->comp_desc = CompAlloc( mem_op, sizeof( ComponentDesc ) );
		if ( !k->comp_desc )  {
			if ( k ) CompFree ( mem_op, k );
			return LDAP_DECODING_ERROR;
		}
		k->comp_desc->cd_gser_encoder = (encoder_func*)GEncComponentAny;
		k->comp_desc->cd_gser_decoder = (gser_decoder_func*)GDecComponentAny;
		k->comp_desc->cd_ber_decoder = (ber_decoder_func*)BDecComponentAny;
		k->comp_desc->cd_free = (comp_free_func*)FreeComponentAny;
		k->comp_desc->cd_extract_i = NULL;
		k->comp_desc->cd_type = ASN_BASIC;
		k->comp_desc->cd_type_id = BASICTYPE_ANY;
		k->comp_desc->cd_all_match = (allcomponent_matching_func*)MatchingComponentAny;
		return LDAP_SUCCESS;
	}
	else {
		Asn1Error ("ERROR - Component ANY Decode routine is NULL\n");
		return 0;
	}
}

int
GDecComponentAny ( void* mem_op, GenBuf *b, ComponentAny *result, AsnLen *bytesDecoded, int mode) {
        ComponentAny *k, **k2;
                                                                          
        k = (ComponentAny*) result;
                                                                          
        if ( mode & DEC_ALLOC_MODE_0 ) {
                k2 = (ComponentAny**) result;
                *k2 = (ComponentAny*) CompAlloc( mem_op, sizeof( ComponentAny ) );
		if ( !*k2 ) return LDAP_DECODING_ERROR;
                k = *k2;
        }
	if ((result->cai != NULL) && (result->cai->GSER_Decode != NULL)) {
		result->value = (void*) CompAlloc ( mem_op, result->cai->size );
		if ( !result->value ) return 0;
		result->cai->GSER_Decode ( mem_op, b, result->value, (int*)bytesDecoded, DEC_ALLOC_MODE_1);
		k->comp_desc = CompAlloc( mem_op, sizeof( ComponentDesc ) );
		if ( !k->comp_desc )  {
			if ( k ) CompFree ( mem_op, k );
			return LDAP_DECODING_ERROR;
		}
		k->comp_desc->cd_gser_encoder = (encoder_func*)GEncComponentAny;
		k->comp_desc->cd_gser_decoder = (gser_decoder_func*)GDecComponentAny;
		k->comp_desc->cd_ber_decoder = (ber_decoder_func*)BDecComponentAny;
		k->comp_desc->cd_free = (comp_free_func*)FreeComponentAny;
		k->comp_desc->cd_type = ASN_BASIC;
		k->comp_desc->cd_extract_i = NULL;
		k->comp_desc->cd_type_id = BASICTYPE_ANY;
		k->comp_desc->cd_all_match = (allcomponent_matching_func*)MatchingComponentAny;
		return LDAP_SUCCESS;
	}
	else {
		Asn1Error ("ERROR - ANY Decode routine is NULL\n");
		return 0;
	}
}

int
MatchingComponentAny (char* oid, ComponentAny *result, ComponentAny *result2) {
	void *comp1, *comp2;

	if ( result->comp_desc->cd_type_id == BASICTYPE_ANY )
		comp1 = result->value;
	else
		comp1 = result;

	if ( result2->comp_desc->cd_type_id == BASICTYPE_ANY )
		comp2 = result2->value;
	else
		comp2 = result2;
		
	if ((result->cai != NULL) && (result->cai->Match != NULL)) {
		if ( result->comp_desc->cd_type_id == BASICTYPE_ANY )
			return result->cai->Match(oid, comp1, comp2 );
		else if ( result2->comp_desc->cd_type_id == BASICTYPE_ANY )
			return result2->cai->Match(oid, comp1, comp2);
		else 
			return LDAP_INVALID_SYNTAX;
	}
	else {
		Asn1Error ("ERROR - ANY Matching routine is NULL\n");
		return LDAP_INVALID_SYNTAX;
	}
}

void*
ExtractingComponentAny ( void* mem_op, ComponentReference* cr,  ComponentAny *result ) {
	if ((result->cai != NULL) && (result->cai->Extract != NULL)) {
		return (void*) result->cai->Extract( mem_op, cr , result->value );
	}
	else {
		Asn1Error ("ERROR - ANY Extracting routine is NULL\n");
		return (void*)NULL;
	}
}

void
FreeComponentAny (ComponentAny* any) {
	if ( any->cai != NULL && any->cai->Free != NULL ) {
		any->cai->Free( any->value );
		free ( ((ComponentSyntaxInfo*)any->value)->csi_comp_desc );
		free ( any->value );
	}
	else
		Asn1Error ("ERROR - ANY Free routine is NULL\n");
}

void
InstallAnyByComponentInt (int anyId, ComponentInt intId, unsigned int size,
			EncodeFcn encode, gser_decoder_func* G_decode,
			ber_tag_decoder_func* B_decode, ExtractFcn extract,
			MatchFcn match, FreeFcn free,
			PrintFcn print)
{
	ComponentAnyInfo *a;
	Hash h;

	a = (ComponentAnyInfo*) malloc(sizeof (ComponentAnyInfo));
	a->anyId = anyId;
	a->oid.octs = NULL;
	a->oid.octetLen = 0;
	a->intId = intId;
	a->size = size;
	a->Encode = encode;
	a->GSER_Decode = G_decode;
	a->BER_Decode = B_decode;
	a->Match = match;
	a->Extract = extract;
	a->Free = free;
	a->Print = print;

	if (anyIntHashTblG == NULL)
		anyIntHashTblG = InitHash();

	h = MakeHash ((char*)&intId, sizeof (intId));

	if(anyIntHashTblG != NULL)
		Insert(anyIntHashTblG, a, h);
}


/*
 * OID and its corresponding decoder can be registerd with this func.
 * If contained types constrained by <select> are used,
 * their OID and decoder MUST be registered, otherwise it will return no entry.
 * An open type(ANY type) also need be registered.
 */
void
InstallOidDecoderMapping ( char* ch_oid, EncodeFcn encode, gser_decoder_func* G_decode, ber_tag_decoder_func* B_decode, ExtractFcn extract, MatchFcn match ) {
	AsnOid oid;
	int strLen;
	void* mem_op;

	strLen = strlen( ch_oid );
	if( strLen <= 0 ) return;
	mem_op = comp_nibble_memory_allocator ( 128, 16 );
	oid.octs = EncodeComponentOid ( mem_op, ch_oid, &strLen );
	oid.octetLen = strLen;
	if( strLen <= 0 ) return;
	

	InstallAnyByComponentOid ( 0, &oid, 0, encode, G_decode, B_decode,
						extract, match, NULL, NULL);
	comp_nibble_memory_free(mem_op);
}

/*
 * Look up Oid-decoder mapping table by berval have either
 * oid or description
 */
OidDecoderMapping*
RetrieveOidDecoderMappingbyBV( struct berval* in ) {
	if ( IsNumericOid ( in->bv_val, in->bv_len ) )
		return RetrieveOidDecoderMappingbyOid( in->bv_val, in->bv_len );
	else
		return RetrieveOidDecoderMappingbyDesc( in->bv_val, in->bv_len );
}

/*
 * Look up Oid-decoder mapping table by dotted OID
 */
OidDecoderMapping*
RetrieveOidDecoderMappingbyOid( char* ch_oid, int oid_len ) {
	Hash hash;
	void *anyInfo;
	AsnOid oid;
	int strLen;
	void* mem_op;

	mem_op = comp_nibble_memory_allocator ( 128, 16 );
	oid.octs = EncodeComponentOid ( mem_op, ch_oid, &oid_len);
	oid.octetLen = oid_len;
	if( oid_len <= 0 ) {
		comp_nibble_memory_free( mem_op );
		return NULL;
	}
	
	/* use encoded oid as hash string */
	hash = MakeHash ( oid.octs, oid.octetLen);
	comp_nibble_memory_free( mem_op );
	if (CheckForAndReturnValue (anyOidHashTblG, hash, &anyInfo))
		return (OidDecoderMapping*) anyInfo;
	else
		return (OidDecoderMapping*) NULL;

}

/*
 * Look up Oid-decoder mapping table by description
 */
OidDecoderMapping*
RetrieveOidDecoderMappingbyDesc( char* desc, int desc_len ) {
	Hash hash;
	void *anyInfo;
	AsnOid oid;
	AttributeType* ad_type;
	struct berval bv;
	void* mem_op;

	bv.bv_val = desc;
	bv.bv_len = desc_len;
	ad_type = at_bvfind( &bv );

	oid.octs = ad_type->sat_atype.at_oid;
	oid.octetLen = strlen ( oid.octs );

	if ( !ad_type )
		return (OidDecoderMapping*) NULL;

	mem_op = comp_nibble_memory_allocator ( 128, 16 );

	oid.octs = EncodeComponentOid ( mem_op, oid.octs , (int*)&oid.octetLen );
	if( oid.octetLen <= 0 ) {
		comp_nibble_memory_free( mem_op );
		return (OidDecoderMapping*) NULL;
	}
	
	/* use encoded oid as hash string */
	hash = MakeHash ( oid.octs, oid.octetLen);
	comp_nibble_memory_free( mem_op );
	if (CheckForAndReturnValue (anyOidHashTblG, hash, &anyInfo))
		return (OidDecoderMapping*) anyInfo;
	else
		return (OidDecoderMapping*) NULL;

}
void
InstallAnyByComponentOid (int anyId, AsnOid *oid, unsigned int size,
			EncodeFcn encode, gser_decoder_func* G_decode,
			ber_tag_decoder_func* B_decode, ExtractFcn extract,
			 MatchFcn match, FreeFcn free, PrintFcn print)
{
	ComponentAnyInfo *a;
	Hash h;

	a = (ComponentAnyInfo*) malloc (sizeof (ComponentAnyInfo));
	a->anyId = anyId;
	if ( oid ) {
		a->oid.octs = malloc( oid->octetLen );
		memcpy ( a->oid.octs, oid->octs, oid->octetLen );
		a->oid.octetLen = oid->octetLen;
	}
	a->size = size;
	a->Encode = encode;
	a->GSER_Decode = G_decode;
	a->BER_Decode = B_decode;
	a->Match = match;
	a->Extract = extract;
	a->Free = free;
	a->Print = print;

	h = MakeHash (oid->octs, oid->octetLen);

	if (anyOidHashTblG == NULL)
		anyOidHashTblG = InitHash();

	if(anyOidHashTblG != NULL)
		Insert(anyOidHashTblG, a, h);
}

int
BDecComponentTop  (
ber_decoder_func *decoder _AND_
void* mem_op _AND_
GenBuf *b _AND_
AsnTag tag _AND_
AsnLen elmtLen _AND_
void **v _AND_
AsnLen *bytesDecoded _AND_
int mode) {
	tag = BDecTag ( b, bytesDecoded );
	elmtLen = BDecLen ( b, bytesDecoded );
	if ( elmtLen <= 0 ) return (-1);
	if ( tag != MAKE_TAG_ID (UNIV, CONS, SEQ_TAG_CODE) ) {
		return (-1);
	}
		
	return (*decoder)( mem_op, b, tag, elmtLen, (ComponentSyntaxInfo*)v,(int*)bytesDecoded, mode );
}

/*
 * ASN.1 specification of a distinguished name
 * DistinguishedName ::= RDNSequence
 * RDNSequence ::= SEQUENCE OF RelativeDistinguishedName
 * RelativeDistinguishedName ::= SET SIZE(1..MAX) OF AttributeTypeandValue
 * AttributeTypeandValue ::= SEQUENCE {
 * 	type	AttributeType
 *	value	AttributeValue
 * }
 * When dnMatch/rdnMatch is used in a component assertion value
 * the component in DistinguishedName/RelativeDistinguishedName
 * need to be converted to the LDAP encodings in RFC2253
 * in order to be matched against the assertion value
 * If allComponentMatch is used, the assertion value may be
 * decoded into the Internal Representation(Component Tree)
 * by the corresponding GSER or BER decoder
 * Following routine converts a component tree(DistinguishedName) into
 * LDAP encodings in RFC2253
 * Example)
 * IR : ComponentRDNSequence
 * GSER : { { type cn, value sang },{ type o, value ibm}, {type c, value us} }
 * LDAP Encodings : cn=sang,o=ibm,c=us 
 */

increment_bv_mem_by_size ( struct berval* in, int size ) {
	int new_size = in->bv_len + size;
	in->bv_val = realloc( in->bv_val, new_size );
	in->bv_len = new_size;
}

int
ConvertBER2Desc( char* in, int size, struct berval* out, int* pos ) {
	int desc_size;
	char* desc_ptr;
	unsigned int firstArcNum;
	unsigned int arcNum;
	int i, rc, start_pos = *pos;
	char buf[MAX_OID_LEN];
	AttributeType *at;
	struct berval bv_name;

	/*convert BER oid to desc*/
	for ( i = 0, arcNum = 0; (i < size) && (in[i] & 0x80 ); i++ )
		arcNum = (arcNum << 7) + (in[i] & 0x7f);
	arcNum = (arcNum << 7) + (in[i] & 0x7f);
	i++;
	firstArcNum = (unsigned short)(arcNum/40);
	if ( firstArcNum > 2 )
		firstArcNum = 2;
	
	arcNum = arcNum - (firstArcNum * 40 );

	rc = intToAscii ( arcNum, buf );

	/*check if the buffer can store the first/second arc and two dots*/
	if ( out->bv_len < *pos + 2 + 1 + rc )
		increment_bv_mem_by_size ( out, INCREMENT_SIZE );

	if ( firstArcNum == 1)
		out->bv_val[*pos] = '1';
	else
		out->bv_val[*pos] = '2';
	(*pos)++;
	out->bv_val[*pos] = '.';
	(*pos)++;

	memcpy( out->bv_val + *pos, buf, rc );
	*pos += rc;
	out->bv_val[*pos] = '.';
	(*pos)++;

	for ( ; i < size ; ) {
		for ( arcNum=0; (i < size) && (in[i] & 0x80) ; i++ )
			arcNum = (arcNum << 7) + (in[i] & 0x7f);
		arcNum = (arcNum << 7) + (in[i] & 0x7f);
		i++;

		rc = intToAscii ( arcNum, buf );

		if ( out->bv_len < *pos + rc + 1 )
			increment_bv_mem_by_size ( out, INCREMENT_SIZE );

		memcpy( out->bv_val + *pos, buf, rc );
		*pos += rc;
		out->bv_val[*pos] = '.';
		(*pos)++;
	}
	(*pos)--;/*remove the last '.'*/

	/*
	 * lookup OID database to locate desc
	 * then overwrite OID with desc in *out
	 * If failed to look up desc, OID form is used
	 */
	bv_name.bv_val = out->bv_val + start_pos;
	bv_name.bv_len = *pos - start_pos;
	at = at_bvfind( &bv_name );
	if ( !at )
		return LDAP_SUCCESS;
	desc_size = at->sat_cname.bv_len;
	memcpy( out->bv_val + start_pos, at->sat_cname.bv_val, desc_size );
	*pos = start_pos + desc_size;
	return LDAP_SUCCESS;
}

int
ConvertComponentAttributeTypeAndValue2RFC2253 ( irAttributeTypeAndValue* in, struct berval* out, int *pos ) {
	int rc;
	int value_size = ((ComponentUTF8String*)in->value.value)->value.octetLen;
	char* value_ptr =  ((ComponentUTF8String*)in->value.value)->value.octs;

	rc = ConvertBER2Desc( in->type.value.octs, in->type.value.octetLen, out, pos );
	if ( rc != LDAP_SUCCESS ) return rc;
	if ( out->bv_len < *pos + 1/*for '='*/  )
		increment_bv_mem_by_size ( out, INCREMENT_SIZE );
	/*Between type and value, put '='*/
	out->bv_val[*pos] = '=';
	(*pos)++;

	/*Assume it is string*/		
	if ( out->bv_len < *pos + value_size )
		increment_bv_mem_by_size ( out, INCREMENT_SIZE );
	memcpy( out->bv_val + *pos, value_ptr, value_size );
	out->bv_len += value_size;
	*pos += value_size;
	
	return LDAP_SUCCESS;
}

int
ConvertRelativeDistinguishedName2RFC2253 ( irRelativeDistinguishedName* in, struct berval *out , int* pos) {
	irAttributeTypeAndValue* attr_typeNvalue;
	int rc;


	FOR_EACH_LIST_ELMT( attr_typeNvalue, &in->comp_list)
	{
		rc = ConvertComponentAttributeTypeAndValue2RFC2253( attr_typeNvalue, out, pos );
		if ( rc != LDAP_SUCCESS ) return LDAP_INVALID_SYNTAX;

		if ( out->bv_len < *pos + 1/*for '+'*/  )
			increment_bv_mem_by_size ( out, INCREMENT_SIZE );
		/*between multivalued RDNs, put comma*/
		out->bv_val[(*pos)++] = '+';
	}
	(*pos)--;/*remove the last '+'*/
	return LDAP_SUCCESS;
}

int 
ConvertRDN2RFC2253 ( irRelativeDistinguishedName* in, struct berval *out ) {
	int rc, pos = 0;
	out->bv_val = (char*)malloc( INITIAL_DN_SIZE );
	out->bv_len = INITIAL_DN_SIZE;

	rc = ConvertRelativeDistinguishedName2RFC2253 ( in, out , &pos);
	if ( rc != LDAP_SUCCESS ) return rc;
	out->bv_val[pos] = '\0';
	out->bv_len = pos;
	return LDAP_SUCCESS;
}

int
ConvertRDNSequence2RFC2253( irRDNSequence *in, struct berval* out ) {
	irRelativeDistinguishedName* rdn_seq;
	AsnList* seq = &in->comp_list;
	int pos = 0, rc ;

	out->bv_val = (char*)malloc( INITIAL_DN_SIZE );
	out->bv_len = INITIAL_DN_SIZE;

	FOR_EACH_LIST_ELMT( rdn_seq, seq )
	{
		rc = ConvertRelativeDistinguishedName2RFC2253( rdn_seq, out, &pos );
		if ( rc != LDAP_SUCCESS ) return LDAP_INVALID_SYNTAX;

		if ( out->bv_len < pos + 1/*for ','*/ )
			increment_bv_mem_by_size ( out, INCREMENT_SIZE );
		/*Between RDN, put comma*/
		out->bv_val[pos++] = ',';
	}
	pos--;/*remove the last '+'*/
	out->bv_val[pos] = '\0';
	out->bv_len =pos;
	return LDAP_SUCCESS;
}

#endif
