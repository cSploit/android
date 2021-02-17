/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * Portions Copyright 1999-2003 Howard Chu.
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
#include "ac/string.h"

#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-meta.h"

/*
 * The meta-directory has one suffix, called <suffix>.
 * It handles a pool of target servers, each with a branch suffix
 * of the form <branch X>,<suffix>, where <branch X> may be empty.
 *
 * When the meta-directory receives a request with a request DN that belongs
 * to a branch, the corresponding target is invoked. When the request DN
 * does not belong to a specific branch, all the targets that
 * are compatible with the request DN are selected as candidates, and
 * the request is spawned to all the candidate targets
 *
 * A request is characterized by a request DN. The following cases are
 * handled:
 * 	- the request DN is the suffix: <dn> == <suffix>,
 * 		all the targets are candidates (search ...)
 * 	- the request DN is a branch suffix: <dn> == <branch X>,<suffix>, or
 * 	- the request DN is a subtree of a branch suffix:
 * 		<dn> == <rdn>,<branch X>,<suffix>,
 * 		the target is the only candidate.
 *
 * A possible extension will include the handling of multiple suffixes
 */

static metasubtree_t *
meta_subtree_match( metatarget_t *mt, struct berval *ndn, int scope )
{
	metasubtree_t *ms = mt->mt_subtree;

	for ( ms = mt->mt_subtree; ms; ms = ms->ms_next ) {
		switch ( ms->ms_type ) {
		case META_ST_SUBTREE:
			if ( dnIsSuffix( ndn, &ms->ms_dn ) ) {
				return ms;
			}
			break;

		case META_ST_SUBORDINATE:
			if ( dnIsSuffix( ndn, &ms->ms_dn ) &&
				( ndn->bv_len > ms->ms_dn.bv_len || scope != LDAP_SCOPE_BASE ) )
			{
				return ms;
			}
			break;

		case META_ST_REGEX:
			/* NOTE: cannot handle scope */
			if ( regexec( &ms->ms_regex, ndn->bv_val, 0, NULL, 0 ) == 0 ) {
				return ms;
			}
			break;
		}
	}

	return NULL;
}

/*
 * returns 1 if suffix is candidate for dn, otherwise 0
 *
 * Note: this function should never be called if dn is the <suffix>.
 */
int 
meta_back_is_candidate(
	metatarget_t	*mt,
	struct berval	*ndn,
	int		scope )
{
	struct berval rdn;
	int d = ndn->bv_len - mt->mt_nsuffix.bv_len;

	if ( d >= 0 ) {
		if ( !dnIsSuffix( ndn, &mt->mt_nsuffix ) ) {
			return META_NOT_CANDIDATE;
		}

		/*
		 * |  match  | exclude |
		 * +---------+---------+-------------------+
		 * |    T    |    T    | not candidate     |
		 * |    F    |    T    | continue checking |
		 * +---------+---------+-------------------+
		 * |    T    |    F    | candidate         |
		 * |    F    |    F    | not candidate     |
		 * +---------+---------+-------------------+
		 */
			
		if ( mt->mt_subtree ) {
			int match = ( meta_subtree_match( mt, ndn, scope ) != NULL );

			if ( !mt->mt_subtree_exclude ) {
				return match ? META_CANDIDATE : META_NOT_CANDIDATE;
			}

			if ( match /* && mt->mt_subtree_exclude */ ) {
				return META_NOT_CANDIDATE;
			}
		}

		switch ( mt->mt_scope ) {
		case LDAP_SCOPE_SUBTREE:
		default:
			return META_CANDIDATE;

		case LDAP_SCOPE_SUBORDINATE:
			if ( d > 0 ) {
				return META_CANDIDATE;
			}
			break;

		/* nearly useless; not allowed by config */
		case LDAP_SCOPE_ONELEVEL:
			if ( d > 0 ) {
				rdn.bv_val = ndn->bv_val;
				rdn.bv_len = (ber_len_t)d - STRLENOF( "," );
				if ( dnIsOneLevelRDN( &rdn ) ) {
					return META_CANDIDATE;
				}
			}
			break;

		/* nearly useless; not allowed by config */
		case LDAP_SCOPE_BASE:
			if ( d == 0 ) {
				return META_CANDIDATE;
			}
			break;
		}

	} else /* if ( d < 0 ) */ {
		if ( !dnIsSuffix( &mt->mt_nsuffix, ndn ) ) {
			return META_NOT_CANDIDATE;
		}

		switch ( scope ) {
		case LDAP_SCOPE_SUBTREE:
		case LDAP_SCOPE_SUBORDINATE:
			/*
			 * suffix longer than dn, but common part matches
			 */
			return META_CANDIDATE;

		case LDAP_SCOPE_ONELEVEL:
			rdn.bv_val = mt->mt_nsuffix.bv_val;
			rdn.bv_len = (ber_len_t)(-d) - STRLENOF( "," );
			if ( dnIsOneLevelRDN( &rdn ) ) {
				return META_CANDIDATE;
			}
			break;
		}
	}

	return META_NOT_CANDIDATE;
}

/*
 * meta_back_select_unique_candidate
 *
 * returns the index of the candidate in case it is unique, otherwise
 * META_TARGET_NONE if none matches, or
 * META_TARGET_MULTIPLE if more than one matches
 * Note: ndn MUST be normalized.
 */
int
meta_back_select_unique_candidate(
	metainfo_t	*mi,
	struct berval	*ndn )
{
	int	i, candidate = META_TARGET_NONE;

	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		metatarget_t	*mt = mi->mi_targets[ i ];

		if ( meta_back_is_candidate( mt, ndn, LDAP_SCOPE_BASE ) ) {
			if ( candidate == META_TARGET_NONE ) {
				candidate = i;

			} else {
				return META_TARGET_MULTIPLE;
			}
		}
	}

	return candidate;
}

/*
 * meta_clear_unused_candidates
 *
 * clears all candidates except candidate
 */
int
meta_clear_unused_candidates(
	Operation	*op,
	int		candidate )
{
	metainfo_t	*mi = ( metainfo_t * )op->o_bd->be_private;
	int		i;
	SlapReply	*candidates = meta_back_candidates_get( op );
	
	for ( i = 0; i < mi->mi_ntargets; ++i ) {
		if ( i == candidate ) {
			continue;
		}
		META_CANDIDATE_RESET( &candidates[ i ] );
	}

	return 0;
}

/*
 * meta_clear_one_candidate
 *
 * clears the selected candidate
 */
int
meta_clear_one_candidate(
	Operation	*op,
	metaconn_t	*mc,
	int		candidate )
{
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];

	if ( msc->msc_ld != NULL ) {

#ifdef DEBUG_205
		char	buf[ BUFSIZ ];

		snprintf( buf, sizeof( buf ), "meta_clear_one_candidate ldap_unbind_ext[%d] mc=%p ld=%p",
			candidate, (void *)mc, (void *)msc->msc_ld );
		Debug( LDAP_DEBUG_ANY, "### %s %s\n",
			op ? op->o_log_prefix : "", buf, 0 );
#endif /* DEBUG_205 */

		ldap_unbind_ext( msc->msc_ld, NULL, NULL );
		msc->msc_ld = NULL;
	}

	if ( !BER_BVISNULL( &msc->msc_bound_ndn ) ) {
		ber_memfree_x( msc->msc_bound_ndn.bv_val, NULL );
		BER_BVZERO( &msc->msc_bound_ndn );
	}

	if ( !BER_BVISNULL( &msc->msc_cred ) ) {
		memset( msc->msc_cred.bv_val, 0, msc->msc_cred.bv_len );
		ber_memfree_x( msc->msc_cred.bv_val, NULL );
		BER_BVZERO( &msc->msc_cred );
	}

	msc->msc_mscflags = 0;

	return 0;
}

