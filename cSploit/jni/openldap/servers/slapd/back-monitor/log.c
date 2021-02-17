/* log.c - deal with log subsystem */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2001-2014 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include <lber_pvt.h>
#include "lutil.h"
#include "ldif.h"
#include "back-monitor.h"

static int 
monitor_subsys_log_open( 
	BackendDB		*be,
	monitor_subsys_t	*ms );

static int 
monitor_subsys_log_modify( 
	Operation		*op,
	SlapReply		*rs,
	Entry 			*e );

/*
 * log mutex
 */
ldap_pvt_thread_mutex_t		monitor_log_mutex;

static int add_values( Operation *op, Entry *e, Modification *mod, int *newlevel );
static int delete_values( Operation *op, Entry *e, Modification *mod, int *newlevel );
static int replace_values( Operation *op, Entry *e, Modification *mod, int *newlevel );

/*
 * initializes log subentry
 */
int
monitor_subsys_log_init(
	BackendDB		*be,
	monitor_subsys_t	*ms )
{
	ms->mss_open = monitor_subsys_log_open;
	ms->mss_modify = monitor_subsys_log_modify;

	ldap_pvt_thread_mutex_init( &monitor_log_mutex );

	return( 0 );
}

/*
 * opens log subentry
 */
int
monitor_subsys_log_open(
	BackendDB		*be,
	monitor_subsys_t	*ms )
{
	BerVarray	bva = NULL;

	if ( loglevel2bvarray( ldap_syslog, &bva ) == 0 && bva != NULL ) {
		monitor_info_t	*mi;
		Entry		*e;

		mi = ( monitor_info_t * )be->be_private;

		if ( monitor_cache_get( mi, &ms->mss_ndn, &e ) ) {
			Debug( LDAP_DEBUG_ANY,
				"monitor_subsys_log_init: "
				"unable to get entry \"%s\"\n",
				ms->mss_ndn.bv_val, 0, 0 );
			ber_bvarray_free( bva );
			return( -1 );
		}

		attr_merge_normalize( e, mi->mi_ad_managedInfo, bva, NULL );
		ber_bvarray_free( bva );

		monitor_cache_release( mi, e );
	}

	return( 0 );
}

static int 
monitor_subsys_log_modify( 
	Operation		*op,
	SlapReply		*rs,
	Entry 			*e )
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;
	int		rc = LDAP_OTHER;
	int		newlevel = ldap_syslog;
	Attribute	*save_attrs;
	Modifications	*modlist = op->orm_modlist;
	Modifications	*ml;

	ldap_pvt_thread_mutex_lock( &monitor_log_mutex );

	save_attrs = e->e_attrs;
	e->e_attrs = attrs_dup( e->e_attrs );

	for ( ml = modlist; ml != NULL; ml = ml->sml_next ) {
		Modification	*mod = &ml->sml_mod;

		/*
		 * accept all operational attributes;
		 * this includes modifersName and modifyTimestamp
		 * if lastmod is "on"
		 */
		if ( is_at_operational( mod->sm_desc->ad_type ) ) {
			( void ) attr_delete( &e->e_attrs, mod->sm_desc );
			rc = rs->sr_err = attr_merge( e, mod->sm_desc,
					mod->sm_values, mod->sm_nvalues );
			if ( rc != LDAP_SUCCESS ) {
				break;
			}
			continue;

		/*
		 * only the "managedInfo" attribute can be modified
		 */
		} else if ( mod->sm_desc != mi->mi_ad_managedInfo ) {
			rc = rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			break;
		}

		switch ( mod->sm_op ) {
		case LDAP_MOD_ADD:
			rc = add_values( op, e, mod, &newlevel );
			break;
			
		case LDAP_MOD_DELETE:
			rc = delete_values( op, e, mod, &newlevel );
			break;

		case LDAP_MOD_REPLACE:
			rc = replace_values( op, e, mod, &newlevel );
			break;

		default:
			rc = LDAP_OTHER;
			break;
		}

		if ( rc != LDAP_SUCCESS ) {
			rs->sr_err = rc;
			break;
		}
	}

	/* set the new debug level */
	if ( rc == LDAP_SUCCESS ) {
		const char	*text;
		static char	textbuf[ BACKMONITOR_BUFSIZE ];

		/* check for abandon */
		if ( op->o_abandon ) {
			rc = rs->sr_err = SLAPD_ABANDON;

			goto cleanup;
		}

		/* check that the entry still obeys the schema */
		rc = entry_schema_check( op, e, save_attrs, 0, 0, NULL,
			&text, textbuf, sizeof( textbuf ) );
		if ( rc != LDAP_SUCCESS ) {
			rs->sr_err = rc;
			goto cleanup;
		}

		/*
		 * Do we need to protect this with a mutex?
		 */
		ldap_syslog = newlevel;

#if 0	/* debug rather than log */
		slap_debug = newlevel;
		lutil_set_debug_level( "slapd", slap_debug );
		ber_set_option(NULL, LBER_OPT_DEBUG_LEVEL, &slap_debug);
		ldap_set_option(NULL, LDAP_OPT_DEBUG_LEVEL, &slap_debug);
		ldif_debug = slap_debug;
#endif
	}

cleanup:;
	if ( rc == LDAP_SUCCESS ) {
		attrs_free( save_attrs );

	} else {
		attrs_free( e->e_attrs );
		e->e_attrs = save_attrs;
	}
	
	ldap_pvt_thread_mutex_unlock( &monitor_log_mutex );

	if ( rc == LDAP_SUCCESS ) {
		rc = SLAP_CB_CONTINUE;
	}

	return rc;
}

static int
check_constraints( Modification *mod, int *newlevel )
{
	int		i;

	if ( mod->sm_nvalues != NULL ) {
		ber_bvarray_free( mod->sm_nvalues );
		mod->sm_nvalues = NULL;
	}

	for ( i = 0; !BER_BVISNULL( &mod->sm_values[ i ] ); i++ ) {
		int		l;
		struct berval	bv;

		if ( str2loglevel( mod->sm_values[ i ].bv_val, &l ) ) {
			return LDAP_CONSTRAINT_VIOLATION;
		}

		if ( loglevel2bv( l, &bv ) ) {
			return LDAP_CONSTRAINT_VIOLATION;
		}
		
		assert( bv.bv_len == mod->sm_values[ i ].bv_len );
		
		AC_MEMCPY( mod->sm_values[ i ].bv_val,
				bv.bv_val, bv.bv_len );

		*newlevel |= l;
	}

	return LDAP_SUCCESS;
}	

static int 
add_values( Operation *op, Entry *e, Modification *mod, int *newlevel )
{
	Attribute	*a;
	int		i, rc;
	MatchingRule 	*mr = mod->sm_desc->ad_type->sat_equality;

	assert( mod->sm_values != NULL );

	rc = check_constraints( mod, newlevel );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	a = attr_find( e->e_attrs, mod->sm_desc );

	if ( a != NULL ) {
		/* "managedInfo" SHOULD have appropriate rules ... */
		if ( mr == NULL || !mr->smr_match ) {
			return LDAP_INAPPROPRIATE_MATCHING;
		}

		for ( i = 0; !BER_BVISNULL( &mod->sm_values[ i ] ); i++ ) {
			int rc;
			int j;
			const char *text = NULL;
			struct berval asserted;

			rc = asserted_value_validate_normalize(
				mod->sm_desc, mr, SLAP_MR_EQUALITY,
				&mod->sm_values[ i ], &asserted, &text,
				op->o_tmpmemctx );

			if ( rc != LDAP_SUCCESS ) {
				return rc;
			}

			for ( j = 0; !BER_BVISNULL( &a->a_vals[ j ] ); j++ ) {
				int match;
				int rc = value_match( &match, mod->sm_desc, mr,
					0, &a->a_nvals[ j ], &asserted, &text );

				if ( rc == LDAP_SUCCESS && match == 0 ) {
					free( asserted.bv_val );
					return LDAP_TYPE_OR_VALUE_EXISTS;
				}
			}

			free( asserted.bv_val );
		}
	}

	/* no - add them */
	rc = attr_merge_normalize( e, mod->sm_desc, mod->sm_values,
		op->o_tmpmemctx );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	return LDAP_SUCCESS;
}

static int
delete_values( Operation *op, Entry *e, Modification *mod, int *newlevel )
{
	int             i, j, k, found, rc, nl = 0;
	Attribute       *a;
	MatchingRule 	*mr = mod->sm_desc->ad_type->sat_equality;

	/* delete the entire attribute */
	if ( mod->sm_values == NULL ) {
		int rc = attr_delete( &e->e_attrs, mod->sm_desc );

		if ( rc ) {
			rc = LDAP_NO_SUCH_ATTRIBUTE;

		} else {
			*newlevel = 0;
			rc = LDAP_SUCCESS;
		}
		return rc;
	}

	rc = check_constraints( mod, &nl );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	*newlevel &= ~nl;

	if ( mr == NULL || !mr->smr_match ) {
		/* disallow specific attributes from being deleted if
		 * no equality rule */
		return LDAP_INAPPROPRIATE_MATCHING;
	}

	/* delete specific values - find the attribute first */
	if ( (a = attr_find( e->e_attrs, mod->sm_desc )) == NULL ) {
		return( LDAP_NO_SUCH_ATTRIBUTE );
	}

	/* find each value to delete */
	for ( i = 0; !BER_BVISNULL( &mod->sm_values[ i ] ); i++ ) {
		int rc;
		const char *text = NULL;

		struct berval asserted;

		rc = asserted_value_validate_normalize(
				mod->sm_desc, mr, SLAP_MR_EQUALITY,
				&mod->sm_values[ i ], &asserted, &text,
				op->o_tmpmemctx );

		if( rc != LDAP_SUCCESS ) return rc;

		found = 0;
		for ( j = 0; !BER_BVISNULL( &a->a_vals[ j ] ); j++ ) {
			int match;
			int rc = value_match( &match, mod->sm_desc, mr,
				0, &a->a_nvals[ j ], &asserted, &text );

			if( rc == LDAP_SUCCESS && match != 0 ) {
				continue;
			}

			/* found a matching value */
			found = 1;

			/* delete it */
			if ( a->a_nvals != a->a_vals ) {
				free( a->a_nvals[ j ].bv_val );
				for ( k = j + 1; !BER_BVISNULL( &a->a_nvals[ k ] ); k++ ) {
					a->a_nvals[ k - 1 ] = a->a_nvals[ k ];
				}
				BER_BVZERO( &a->a_nvals[ k - 1 ] );
			}

			free( a->a_vals[ j ].bv_val );
			for ( k = j + 1; !BER_BVISNULL( &a->a_vals[ k ] ); k++ ) {
				a->a_vals[ k - 1 ] = a->a_vals[ k ];
			}
			BER_BVZERO( &a->a_vals[ k - 1 ] );
			a->a_numvals--;

			break;
		}

		free( asserted.bv_val );

		/* looked through them all w/o finding it */
		if ( ! found ) {
			return LDAP_NO_SUCH_ATTRIBUTE;
		}
	}

	/* if no values remain, delete the entire attribute */
	if ( BER_BVISNULL( &a->a_vals[ 0 ] ) ) {
		assert( a->a_numvals == 0 );

		/* should already be zero */
		*newlevel = 0;
		
		if ( attr_delete( &e->e_attrs, mod->sm_desc ) ) {
			return LDAP_NO_SUCH_ATTRIBUTE;
		}
	}

	return LDAP_SUCCESS;
}

static int
replace_values( Operation *op, Entry *e, Modification *mod, int *newlevel )
{
	int rc;

	if ( mod->sm_values != NULL ) {
		*newlevel = 0;
		rc = check_constraints( mod, newlevel );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}
	}

	rc = attr_delete( &e->e_attrs, mod->sm_desc );

	if ( rc != LDAP_SUCCESS && rc != LDAP_NO_SUCH_ATTRIBUTE ) {
		return rc;
	}

	if ( mod->sm_values != NULL ) {
		rc = attr_merge_normalize( e, mod->sm_desc, mod->sm_values,
				op->o_tmpmemctx );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}
	}

	return LDAP_SUCCESS;
}

