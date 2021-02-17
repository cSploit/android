/* allop.c - returns all operational attributes when appropriate */
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
 * This work was initially developed by Pierangelo Masarati for inclusion in
 * OpenLDAP Software.
 */

/*
 * The intended usage is as a global overlay for use with those clients
 * that do not make use of the RFC3673 allOp ("+") in the requested 
 * attribute list, but expect all operational attributes to be returned.
 * Usage: add
 *

overlay		allop
allop-URI	<ldapURI>

 *
 * if the allop-URI is not given, the rootDSE, i.e. "ldap:///??base",
 * is assumed.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "slap.h"
#include "config.h"

#define	SLAP_OVER_VERSION_REQUIRE(major,minor,patch) \
	( \
		( LDAP_VENDOR_VERSION_MAJOR == X || LDAP_VENDOR_VERSION_MAJOR >= (major) ) \
		&& ( LDAP_VENDOR_VERSION_MINOR == X || LDAP_VENDOR_VERSION_MINOR >= (minor) ) \
		&& ( LDAP_VENDOR_VERSION_PATCH == X || LDAP_VENDOR_VERSION_PATCH >= (patch) ) \
	)

#if !SLAP_OVER_VERSION_REQUIRE(2,3,0)
#error "version mismatch"
#endif

typedef struct allop_t {
	struct berval	ao_ndn;
	int		ao_scope;
} allop_t;

static int
allop_db_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	allop_t		*ao = (allop_t *)on->on_bi.bi_private;

	if ( strcasecmp( argv[ 0 ], "allop-uri" ) == 0 ) {
		LDAPURLDesc	*lud;
		struct berval	dn,
				ndn;
		int		scope,
				rc = LDAP_SUCCESS;

		if ( argc != 2 ) {
			fprintf( stderr, "%s line %d: "
				"need exactly 1 arg "
				"in \"allop-uri <ldapURI>\" "
				"directive.\n",
				fname, lineno );
			return 1;
		}

		if ( ldap_url_parse( argv[ 1 ], &lud ) != LDAP_URL_SUCCESS ) {
			return -1;
		}

		scope = lud->lud_scope;
		if ( scope == LDAP_SCOPE_DEFAULT ) {
			scope = LDAP_SCOPE_BASE;
		}

		if ( lud->lud_dn == NULL || lud->lud_dn[ 0 ] == '\0' ) {
			if ( scope == LDAP_SCOPE_BASE ) {
				BER_BVZERO( &ndn );

			} else {
				ber_str2bv( "", 0, 1, &ndn );
			}

		} else {

			ber_str2bv( lud->lud_dn, 0, 0, &dn );
			rc = dnNormalize( 0, NULL, NULL, &dn, &ndn, NULL );
		}

		ldap_free_urldesc( lud );
		if ( rc != LDAP_SUCCESS ) {
			return -1;
		}

		if ( BER_BVISNULL( &ndn ) ) {
			/* rootDSE */
			if ( ao != NULL ) {
				ch_free( ao->ao_ndn.bv_val );
				ch_free( ao );
				on->on_bi.bi_private = NULL;
			}

		} else {
			if ( ao == NULL ) {
				ao = ch_calloc( 1, sizeof( allop_t ) );
				on->on_bi.bi_private = (void *)ao;

			} else {
				ch_free( ao->ao_ndn.bv_val );
			}

			ao->ao_ndn = ndn;
			ao->ao_scope = scope;
		}

	} else {
		return SLAP_CONF_UNKNOWN;
	}

	return 0;
}

static int
allop_db_destroy( BackendDB *be, ConfigReply *cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	allop_t		*ao = (allop_t *)on->on_bi.bi_private;

	if ( ao != NULL ) {
		assert( !BER_BVISNULL( &ao->ao_ndn ) );

		ch_free( ao->ao_ndn.bv_val );
		ch_free( ao );
		on->on_bi.bi_private = NULL;
	}

	return 0;
}

static int
allop_op_search( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	allop_t		*ao = (allop_t *)on->on_bi.bi_private;

	slap_mask_t	mask;
	int		i,
			add_allUser = 0;

	if ( ao == NULL ) {
		if ( !BER_BVISEMPTY( &op->o_req_ndn )
			|| op->ors_scope != LDAP_SCOPE_BASE )
		{
			return SLAP_CB_CONTINUE;
		}

	} else {
		if ( !dnIsSuffix( &op->o_req_ndn, &ao->ao_ndn ) ) {
			return SLAP_CB_CONTINUE;
		}

		switch ( ao->ao_scope ) {
		case LDAP_SCOPE_BASE:
			if ( op->o_req_ndn.bv_len != ao->ao_ndn.bv_len ) {
				return SLAP_CB_CONTINUE;
			}
			break;

		case LDAP_SCOPE_ONELEVEL:
			if ( op->ors_scope == LDAP_SCOPE_BASE ) {
				struct berval	rdn = op->o_req_ndn;

				rdn.bv_len -= ao->ao_ndn.bv_len + STRLENOF( "," );
				if ( !dnIsOneLevelRDN( &rdn ) ) {
					return SLAP_CB_CONTINUE;
				}

				break;
			}
			return SLAP_CB_CONTINUE;

		case LDAP_SCOPE_SUBTREE:
			break;
		}
	}

	mask = slap_attr_flags( op->ors_attrs );
	if ( SLAP_OPATTRS( mask ) ) {
		return SLAP_CB_CONTINUE;
	}

	if ( !SLAP_USERATTRS( mask ) ) {
		return SLAP_CB_CONTINUE;
	}

	i = 0;
	if ( op->ors_attrs == NULL ) {
		add_allUser = 1;

	} else {
		for ( ; !BER_BVISNULL( &op->ors_attrs[ i ].an_name ); i++ )
			;
	}

	op->ors_attrs = op->o_tmprealloc( op->ors_attrs,
		sizeof( AttributeName ) * ( i + add_allUser + 2 ),
		op->o_tmpmemctx );

	if ( add_allUser ) {
		op->ors_attrs[ i ] = slap_anlist_all_user_attributes[ 0 ];
		i++;
	}

	op->ors_attrs[ i ] = slap_anlist_all_operational_attributes[ 0 ];

	BER_BVZERO( &op->ors_attrs[ i + 1 ].an_name );

	return SLAP_CB_CONTINUE;
}

static slap_overinst 		allop;

int
allop_init()
{
	allop.on_bi.bi_type = "allop";

	allop.on_bi.bi_db_config = allop_db_config;
	allop.on_bi.bi_db_destroy = allop_db_destroy;

	allop.on_bi.bi_op_search = allop_op_search;

	return overlay_register( &allop );
}

int
init_module( int argc, char *argv[] )
{
	return allop_init();
}

