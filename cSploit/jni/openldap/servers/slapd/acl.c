/* acl.c - routines to parse and check acl's */
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
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/regex.h>
#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "sets.h"
#include "lber_pvt.h"
#include "lutil.h"

#define ACL_BUF_SIZE 	1024	/* use most appropriate size */

static const struct berval	acl_bv_ip_eq = BER_BVC( "IP=" );
#ifdef LDAP_PF_INET6
static const struct berval	acl_bv_ipv6_eq = BER_BVC( "IP=[" );
#endif /* LDAP_PF_INET6 */
#ifdef LDAP_PF_LOCAL
static const struct berval	acl_bv_path_eq = BER_BVC("PATH=");
#endif /* LDAP_PF_LOCAL */

static AccessControl * slap_acl_get(
	AccessControl *ac, int *count,
	Operation *op, Entry *e,
	AttributeDescription *desc,
	struct berval *val,
	AclRegexMatches *matches,
	slap_mask_t *mask,
	AccessControlState *state );

static slap_control_t slap_acl_mask(
	AccessControl *ac,
	AccessControl *prev,
	slap_mask_t *mask,
	Operation *op, Entry *e,
	AttributeDescription *desc,
	struct berval *val,
	AclRegexMatches *matches,
	int count,
	AccessControlState *state,
	slap_access_t access );

static int	regex_matches(
	struct berval *pat, char *str,
	struct berval *dn_matches, struct berval *val_matches,
	AclRegexMatches *matches);

typedef	struct AclSetCookie {
	SetCookie	asc_cookie;
#define	asc_op		asc_cookie.set_op
	Entry		*asc_e;
} AclSetCookie;


SLAP_SET_GATHER acl_set_gather;
SLAP_SET_GATHER acl_set_gather2;

/*
 * access_allowed - check whether op->o_ndn is allowed the requested access
 * to entry e, attribute attr, value val.  if val is null, access to
 * the whole attribute is assumed (all values).
 *
 * This routine loops through all access controls and calls
 * slap_acl_mask() on each applicable access control.
 * The loop exits when a definitive answer is reached or
 * or no more controls remain.
 *
 * returns:
 *		0	access denied
 *		1	access granted
 *
 * Notes:
 * - can be legally called with op == NULL
 * - can be legally called with op->o_bd == NULL
 */

int
slap_access_always_allowed(
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	slap_access_t		access,
	AccessControlState	*state,
	slap_mask_t		*maskp )
{
	assert( maskp != NULL );

	/* assign all */
	ACL_LVL_ASSIGN_MANAGE( *maskp );

	return 1;
}

#define MATCHES_DNMAXCOUNT(m) 					\
	( sizeof ( (m)->dn_data ) / sizeof( *(m)->dn_data ) )
#define MATCHES_VALMAXCOUNT(m) 					\
	( sizeof ( (m)->val_data ) / sizeof( *(m)->val_data ) )
#define MATCHES_MEMSET(m) do {					\
	memset( (m)->dn_data, '\0', sizeof( (m)->dn_data ) );	\
	memset( (m)->val_data, '\0', sizeof( (m)->val_data ) );	\
	(m)->dn_count = MATCHES_DNMAXCOUNT( (m) );		\
	(m)->val_count = MATCHES_VALMAXCOUNT( (m) );		\
} while ( 0 /* CONSTCOND */ )

int
slap_access_allowed(
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	slap_access_t		access,
	AccessControlState	*state,
	slap_mask_t		*maskp )
{
	int				ret = 1;
	int				count;
	AccessControl			*a, *prev;

#ifdef LDAP_DEBUG
	char				accessmaskbuf[ACCESSMASK_MAXLEN];
#endif
	slap_mask_t			mask;
	slap_control_t			control;
	slap_access_t			access_level;
	const char			*attr;
	AclRegexMatches			matches;
	AccessControlState		acl_state = ACL_STATE_INIT;
	static AccessControlState	state_init = ACL_STATE_INIT;

	assert( op != NULL );
	assert( e != NULL );
	assert( desc != NULL );
	assert( maskp != NULL );

	access_level = ACL_LEVEL( access );
	attr = desc->ad_cname.bv_val;

	assert( attr != NULL );

	ACL_INIT( mask );

	/* grant database root access */
	if ( be_isroot( op ) ) {
		Debug( LDAP_DEBUG_ACL, "<= root access granted\n", 0, 0, 0 );
		mask = ACL_LVL_MANAGE;
		goto done;
	}

	/*
	 * no-user-modification operational attributes are ignored
	 * by ACL_WRITE checking as any found here are not provided
	 * by the user
	 *
	 * NOTE: but they are not ignored for ACL_MANAGE, because
	 * if we get here it means a non-root user is trying to 
	 * manage data, so we need to check its privileges.
	 */
	if ( access_level == ACL_WRITE
		&& is_at_no_user_mod( desc->ad_type )
		&& desc != slap_schema.si_ad_entry
		&& desc != slap_schema.si_ad_children )
	{
		Debug( LDAP_DEBUG_ACL, "NoUserMod Operational attribute:"
			" %s access granted\n",
			attr, 0, 0 );
		goto done;
	}

	/* use backend default access if no backend acls */
	if ( op->o_bd->be_acl == NULL && frontendDB->be_acl == NULL ) {
		int	i;

		Debug( LDAP_DEBUG_ACL,
			"=> slap_access_allowed: backend default %s "
			"access %s to \"%s\"\n",
			access2str( access ),
			op->o_bd->be_dfltaccess >= access_level ? "granted" : "denied",
			op->o_dn.bv_val ? op->o_dn.bv_val : "(anonymous)" );
		ret = op->o_bd->be_dfltaccess >= access_level;

		mask = ACL_PRIV_LEVEL;
		for ( i = ACL_NONE; i <= op->o_bd->be_dfltaccess; i++ ) {
			ACL_PRIV_SET( mask, ACL_ACCESS2PRIV( i ) );
		}

		goto done;
	}

	ret = 0;
	control = ACL_BREAK;

	if ( state == NULL )
		state = &acl_state;
	if ( state->as_desc == desc &&
		state->as_access == access &&
		state->as_vd_acl_present )
	{
		a = state->as_vd_acl;
		count = state->as_vd_acl_count;
		if ( state->as_fe_done )
			state->as_fe_done--;
		ACL_PRIV_ASSIGN( mask, state->as_vd_mask );
	} else {
		*state = state_init;

		a = NULL;
		count = 0;
		ACL_PRIV_ASSIGN( mask, *maskp );
	}

	MATCHES_MEMSET( &matches );
	prev = a;

	while ( ( a = slap_acl_get( a, &count, op, e, desc, val,
		&matches, &mask, state ) ) != NULL )
	{
		int i; 
		int dnmaxcount = MATCHES_DNMAXCOUNT( &matches );
		int valmaxcount = MATCHES_VALMAXCOUNT( &matches );
		regmatch_t *dn_data = matches.dn_data;
		regmatch_t *val_data = matches.val_data;

		/* DN matches */
		for ( i = 0; i < dnmaxcount && dn_data[i].rm_eo > 0; i++ ) {
			char *data = e->e_ndn;

			Debug( LDAP_DEBUG_ACL, "=> match[dn%d]: %d %d ", i,
				(int)dn_data[i].rm_so, 
				(int)dn_data[i].rm_eo );
			if ( dn_data[i].rm_so <= dn_data[0].rm_eo ) {
				int n;
				for ( n = dn_data[i].rm_so; 
				      n < dn_data[i].rm_eo; n++ ) {
					Debug( LDAP_DEBUG_ACL, "%c", 
					       data[n], 0, 0 );
				}
			}
			Debug( LDAP_DEBUG_ACL, "\n", 0, 0, 0 );
		}

		/* val matches */
		for ( i = 0; i < valmaxcount && val_data[i].rm_eo > 0; i++ ) {
			char *data = val->bv_val;

			Debug( LDAP_DEBUG_ACL, "=> match[val%d]: %d %d ", i,
				(int)val_data[i].rm_so, 
				(int)val_data[i].rm_eo );
			if ( val_data[i].rm_so <= val_data[0].rm_eo ) {
				int n;
				for ( n = val_data[i].rm_so; 
				      n < val_data[i].rm_eo; n++ ) {
					Debug( LDAP_DEBUG_ACL, "%c", 
					       data[n], 0, 0 );
				}
			}
			Debug( LDAP_DEBUG_ACL, "\n", 0, 0, 0 );
		}

		control = slap_acl_mask( a, prev, &mask, op,
			e, desc, val, &matches, count, state, access );

		if ( control != ACL_BREAK ) {
			break;
		}

		MATCHES_MEMSET( &matches );
		prev = a;
	}

	if ( ACL_IS_INVALID( mask ) ) {
		Debug( LDAP_DEBUG_ACL,
			"=> slap_access_allowed: \"%s\" (%s) invalid!\n",
			e->e_dn, attr, 0 );
		ACL_PRIV_ASSIGN( mask, *maskp );

	} else if ( control == ACL_BREAK ) {
		Debug( LDAP_DEBUG_ACL,
			"=> slap_access_allowed: no more rules\n", 0, 0, 0 );

		goto done;
	}

	ret = ACL_GRANT( mask, access );

	Debug( LDAP_DEBUG_ACL,
		"=> slap_access_allowed: %s access %s by %s\n",
		access2str( access ), ret ? "granted" : "denied",
		accessmask2str( mask, accessmaskbuf, 1 ) );

done:
	ACL_PRIV_ASSIGN( *maskp, mask );
	return ret;
}

int
fe_access_allowed(
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	slap_access_t		access,
	AccessControlState	*state,
	slap_mask_t		*maskp )
{
	BackendDB		*be_orig;
	int			rc;

	/*
	 * NOTE: control gets here if FIXME
	 * if an appropriate backend cannot be selected for the operation,
	 * we assume that the frontend should handle this
	 * FIXME: should select_backend() take care of this,
	 * and return frontendDB instead of NULL?  maybe for some value
	 * of the flags?
	 */
	be_orig = op->o_bd;

	if ( op->o_bd == NULL ) {
		op->o_bd = select_backend( &op->o_req_ndn, 0 );
		if ( op->o_bd == NULL )
			op->o_bd = frontendDB;
	}
	rc = slap_access_allowed( op, e, desc, val, access, state, maskp );
	op->o_bd = be_orig;

	return rc;
}

int
access_allowed_mask(
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	slap_access_t		access,
	AccessControlState	*state,
	slap_mask_t		*maskp )
{
	int				ret = 1;
	int				be_null = 0;

#ifdef LDAP_DEBUG
	char				accessmaskbuf[ACCESSMASK_MAXLEN];
#endif
	slap_mask_t			mask;
	slap_access_t			access_level;
	const char			*attr;

	assert( e != NULL );
	assert( desc != NULL );

	access_level = ACL_LEVEL( access );

	assert( access_level > ACL_NONE );

	ACL_INIT( mask );
	if ( maskp ) ACL_INVALIDATE( *maskp );

	attr = desc->ad_cname.bv_val;

	assert( attr != NULL );

	if ( op ) {
		if ( op->o_acl_priv != ACL_NONE ) {
			access = op->o_acl_priv;

		} else if ( op->o_is_auth_check &&
			( access_level == ACL_SEARCH || access_level == ACL_READ ) )
		{
			access = ACL_AUTH;

		} else if ( get_relax( op ) && access_level == ACL_WRITE &&
			desc == slap_schema.si_ad_entry )
		{
			access = ACL_MANAGE;
		}
	}

	if ( state != NULL ) {
		if ( state->as_desc == desc &&
			state->as_access == access &&
			state->as_result != -1 &&
			!state->as_vd_acl_present )
			{
			Debug( LDAP_DEBUG_ACL,
				"=> access_allowed: result was in cache (%s)\n",
				attr, 0, 0 );
				return state->as_result;
		} else {
			Debug( LDAP_DEBUG_ACL,
				"=> access_allowed: result not in cache (%s)\n",
				attr, 0, 0 );
		}
	}

	Debug( LDAP_DEBUG_ACL,
		"=> access_allowed: %s access to \"%s\" \"%s\" requested\n",
		access2str( access ), e->e_dn, attr );

	if ( op == NULL ) {
		/* no-op call */
		goto done;
	}

	if ( op->o_bd == NULL ) {
		op->o_bd = LDAP_STAILQ_FIRST( &backendDB );
		be_null = 1;

		/* FIXME: experimental; use first backend rules
		 * iff there is no global_acl (ITS#3100)
		 */
		if ( frontendDB->be_acl != NULL ) {
			op->o_bd = frontendDB;
		}
	}
	assert( op->o_bd != NULL );

	/* this is enforced in backend_add() */
	if ( op->o_bd->bd_info->bi_access_allowed ) {
		/* delegate to backend */
		ret = op->o_bd->bd_info->bi_access_allowed( op, e,
				desc, val, access, state, &mask );

	} else {
		/* use default (but pass through frontend
		 * for global ACL overlays) */
		ret = frontendDB->bd_info->bi_access_allowed( op, e,
				desc, val, access, state, &mask );
	}

	if ( !ret ) {
		if ( ACL_IS_INVALID( mask ) ) {
			Debug( LDAP_DEBUG_ACL,
				"=> access_allowed: \"%s\" (%s) invalid!\n",
				e->e_dn, attr, 0 );
			ACL_INIT( mask );

		} else {
			Debug( LDAP_DEBUG_ACL,
				"=> access_allowed: no more rules\n", 0, 0, 0 );

			goto done;
		}
	}

	Debug( LDAP_DEBUG_ACL,
		"=> access_allowed: %s access %s by %s\n",
		access2str( access ), ret ? "granted" : "denied",
		accessmask2str( mask, accessmaskbuf, 1 ) );

done:
	if ( state != NULL ) {
		state->as_access = access;
			state->as_result = ret;
		state->as_desc = desc;
	}
	if ( be_null ) op->o_bd = NULL;
	if ( maskp ) ACL_PRIV_ASSIGN( *maskp, mask );
	return ret;
}


/*
 * slap_acl_get - return the acl applicable to entry e, attribute
 * attr.  the acl returned is suitable for use in subsequent calls to
 * acl_access_allowed().
 */

static AccessControl *
slap_acl_get(
	AccessControl *a,
	int			*count,
	Operation	*op,
	Entry		*e,
	AttributeDescription *desc,
	struct berval	*val,
	AclRegexMatches	*matches,
	slap_mask_t *mask,
	AccessControlState *state )
{
	const char *attr;
	ber_len_t dnlen;
	AccessControl *prev;

	assert( e != NULL );
	assert( count != NULL );
	assert( desc != NULL );
	assert( state != NULL );

	attr = desc->ad_cname.bv_val;

	assert( attr != NULL );

	if( a == NULL ) {
		if( op->o_bd == NULL || op->o_bd->be_acl == NULL ) {
			a = frontendDB->be_acl;
		} else {
			a = op->o_bd->be_acl;
		}
		prev = NULL;

		assert( a != NULL );
		if ( a == frontendDB->be_acl )
			state->as_fe_done = 1;
	} else {
		prev = a;
		a = a->acl_next;
	}

	dnlen = e->e_nname.bv_len;

 retry:
	for ( ; a != NULL; prev = a, a = a->acl_next ) {
		(*count) ++;

		if ( a != frontendDB->be_acl && state->as_fe_done )
			state->as_fe_done++;

		if ( a->acl_dn_pat.bv_len || ( a->acl_dn_style != ACL_STYLE_REGEX )) {
			if ( a->acl_dn_style == ACL_STYLE_REGEX ) {
				Debug( LDAP_DEBUG_ACL, "=> dnpat: [%d] %s nsub: %d\n", 
					*count, a->acl_dn_pat.bv_val, (int) a->acl_dn_re.re_nsub );
				if ( regexec ( &a->acl_dn_re, 
					       e->e_ndn, 
				 	       matches->dn_count, 
					       matches->dn_data, 0 ) )
					continue;

			} else {
				ber_len_t patlen;

				Debug( LDAP_DEBUG_ACL, "=> dn: [%d] %s\n", 
					*count, a->acl_dn_pat.bv_val, 0 );
				patlen = a->acl_dn_pat.bv_len;
				if ( dnlen < patlen )
					continue;

				if ( a->acl_dn_style == ACL_STYLE_BASE ) {
					/* base dn -- entire object DN must match */
					if ( dnlen != patlen )
						continue;

				} else if ( a->acl_dn_style == ACL_STYLE_ONE ) {
					ber_len_t	rdnlen = 0;
					ber_len_t	sep = 0;

					if ( dnlen <= patlen )
						continue;

					if ( patlen > 0 ) {
						if ( !DN_SEPARATOR( e->e_ndn[dnlen - patlen - 1] ) )
							continue;
						sep = 1;
					}

					rdnlen = dn_rdnlen( NULL, &e->e_nname );
					if ( rdnlen + patlen + sep != dnlen )
						continue;

				} else if ( a->acl_dn_style == ACL_STYLE_SUBTREE ) {
					if ( dnlen > patlen && !DN_SEPARATOR( e->e_ndn[dnlen - patlen - 1] ) )
						continue;

				} else if ( a->acl_dn_style == ACL_STYLE_CHILDREN ) {
					if ( dnlen <= patlen )
						continue;
					if ( !DN_SEPARATOR( e->e_ndn[dnlen - patlen - 1] ) )
						continue;
				}

				if ( strcmp( a->acl_dn_pat.bv_val, e->e_ndn + dnlen - patlen ) != 0 )
					continue;
			}

			Debug( LDAP_DEBUG_ACL, "=> acl_get: [%d] matched\n",
				*count, 0, 0 );
		}

		if ( a->acl_attrs && !ad_inlist( desc, a->acl_attrs ) ) {
			matches->dn_data[0].rm_so = -1;
			matches->dn_data[0].rm_eo = -1;
			matches->val_data[0].rm_so = -1;
			matches->val_data[0].rm_eo = -1;
			continue;
		}

		/* Is this ACL only for a specific value? */
		if ( a->acl_attrval.bv_val ) {
			if ( val == NULL ) {
				continue;
			}

			if ( !state->as_vd_acl_present ) {
				state->as_vd_acl_present = 1;
				state->as_vd_acl = prev;
				state->as_vd_acl_count = *count - 1;
				ACL_PRIV_ASSIGN ( state->as_vd_mask, *mask );
			}

			if ( a->acl_attrval_style == ACL_STYLE_REGEX ) {
				Debug( LDAP_DEBUG_ACL,
					"acl_get: valpat %s\n",
					a->acl_attrval.bv_val, 0, 0 );
				if ( regexec ( &a->acl_attrval_re, 
						    val->bv_val, 
						    matches->val_count, 
						    matches->val_data, 0 ) )
				{
					continue;
				}

			} else {
				int match = 0;
				const char *text;
				Debug( LDAP_DEBUG_ACL,
					"acl_get: val %s\n",
					a->acl_attrval.bv_val, 0, 0 );
	
				if ( a->acl_attrs[0].an_desc->ad_type->sat_syntax != slap_schema.si_syn_distinguishedName ) {
					if (value_match( &match, desc,
						a->acl_attrval_mr, 0,
						val, &a->acl_attrval, &text ) != LDAP_SUCCESS ||
							match )
						continue;
					
				} else {
					ber_len_t	patlen, vdnlen;
	
					patlen = a->acl_attrval.bv_len;
					vdnlen = val->bv_len;
	
					if ( vdnlen < patlen )
						continue;
	
					if ( a->acl_attrval_style == ACL_STYLE_BASE ) {
						if ( vdnlen > patlen )
							continue;
	
					} else if ( a->acl_attrval_style == ACL_STYLE_ONE ) {
						ber_len_t	rdnlen = 0;
	
						if ( !DN_SEPARATOR( val->bv_val[vdnlen - patlen - 1] ) )
							continue;
	
						rdnlen = dn_rdnlen( NULL, val );
						if ( rdnlen + patlen + 1 != vdnlen )
							continue;
	
					} else if ( a->acl_attrval_style == ACL_STYLE_SUBTREE ) {
						if ( vdnlen > patlen && !DN_SEPARATOR( val->bv_val[vdnlen - patlen - 1] ) )
							continue;
	
					} else if ( a->acl_attrval_style == ACL_STYLE_CHILDREN ) {
						if ( vdnlen <= patlen )
							continue;
	
						if ( !DN_SEPARATOR( val->bv_val[vdnlen - patlen - 1] ) )
							continue;
					}
	
					if ( strcmp( a->acl_attrval.bv_val, val->bv_val + vdnlen - patlen ) )
						continue;
				}
			}
		}

		if ( a->acl_filter != NULL ) {
			ber_int_t rc = test_filter( NULL, e, a->acl_filter );
			if ( rc != LDAP_COMPARE_TRUE ) {
				continue;
			}
		}

		Debug( LDAP_DEBUG_ACL, "=> acl_get: [%d] attr %s\n",
		       *count, attr, 0);
		return a;
	}

	if ( !state->as_fe_done ) {
		state->as_fe_done = 1;
		a = frontendDB->be_acl;
		goto retry;
	}

	Debug( LDAP_DEBUG_ACL, "<= acl_get: done.\n", 0, 0, 0 );
	return( NULL );
}

/*
 * Record value-dependent access control state
 */
#define ACL_RECORD_VALUE_STATE do { \
		if( state && !state->as_vd_acl_present ) { \
			state->as_vd_acl_present = 1; \
			state->as_vd_acl = prev; \
			state->as_vd_acl_count = count - 1; \
			ACL_PRIV_ASSIGN( state->as_vd_mask, *mask ); \
		} \
	} while( 0 )

static int
acl_mask_dn(
	Operation		*op,
	Entry			*e,
	struct berval		*val,
	AccessControl		*a,
	AclRegexMatches		*matches,
	slap_dn_access		*bdn,
	struct berval		*opndn )
{
	/*
	 * if access applies to the entry itself, and the
	 * user is bound as somebody in the same namespace as
	 * the entry, OR the given dn matches the dn pattern
	 */
	/*
	 * NOTE: styles "anonymous", "users" and "self" 
	 * have been moved to enum slap_style_t, whose 
	 * value is set in a_dn_style; however, the string
	 * is maintained in a_dn_pat.
	 */

	if ( bdn->a_style == ACL_STYLE_ANONYMOUS ) {
		if ( !BER_BVISEMPTY( opndn ) ) {
			return 1;
		}

	} else if ( bdn->a_style == ACL_STYLE_USERS ) {
		if ( BER_BVISEMPTY( opndn ) ) {
			return 1;
		}

	} else if ( bdn->a_style == ACL_STYLE_SELF ) {
		struct berval	ndn, selfndn;
		int		level;

		if ( BER_BVISEMPTY( opndn ) || BER_BVISNULL( &e->e_nname ) ) {
			return 1;
		}

		level = bdn->a_self_level;
		if ( level < 0 ) {
			selfndn = *opndn;
			ndn = e->e_nname;
			level = -level;

		} else {
			ndn = *opndn;
			selfndn = e->e_nname;
		}

		for ( ; level > 0; level-- ) {
			if ( BER_BVISEMPTY( &ndn ) ) {
				break;
			}
			dnParent( &ndn, &ndn );
		}
			
		if ( BER_BVISEMPTY( &ndn ) || !dn_match( &ndn, &selfndn ) )
		{
			return 1;
		}

	} else if ( bdn->a_style == ACL_STYLE_REGEX ) {
		if ( !ber_bvccmp( &bdn->a_pat, '*' ) ) {
			AclRegexMatches	tmp_matches,
					*tmp_matchesp = &tmp_matches;
			int		rc = 0;
			regmatch_t 	*tmp_data;

			MATCHES_MEMSET( &tmp_matches );
			tmp_data = &tmp_matches.dn_data[0];

			if ( a->acl_attrval_style == ACL_STYLE_REGEX )
				tmp_matchesp = matches;
			else switch ( a->acl_dn_style ) {
			case ACL_STYLE_REGEX:
				if ( !BER_BVISNULL( &a->acl_dn_pat ) ) {
					tmp_matchesp = matches; 
					break;
				}
			/* FALLTHRU: applies also to ACL_STYLE_REGEX when pattern is "*" */

			case ACL_STYLE_BASE:
				tmp_data[0].rm_so = 0;
				tmp_data[0].rm_eo = e->e_nname.bv_len;
				tmp_matches.dn_count = 1;
				break;

			case ACL_STYLE_ONE:
			case ACL_STYLE_SUBTREE:
			case ACL_STYLE_CHILDREN:
				tmp_data[0].rm_so = 0;
				tmp_data[0].rm_eo = e->e_nname.bv_len;
				tmp_data[1].rm_so = e->e_nname.bv_len - a->acl_dn_pat.bv_len;
				tmp_data[1].rm_eo = e->e_nname.bv_len;
				tmp_matches.dn_count = 2;
				break;

			default:
				/* error */
				rc = 1;
				break;
			}

			if ( rc ) {
				return 1;
			}

			if ( !regex_matches( &bdn->a_pat, opndn->bv_val,
				&e->e_nname, NULL, tmp_matchesp ) )
			{
				return 1;
			}
		}

	} else {
		struct berval	pat;
		ber_len_t	patlen, odnlen;
		int		got_match = 0;

		if ( e->e_dn == NULL )
			return 1;

		if ( bdn->a_expand ) {
			struct berval	bv;
			char		buf[ACL_BUF_SIZE];
			
			AclRegexMatches	tmp_matches,
					*tmp_matchesp = &tmp_matches;
			int		rc = 0;
			regmatch_t 	*tmp_data;

			MATCHES_MEMSET( &tmp_matches );
			tmp_data = &tmp_matches.dn_data[0];

			bv.bv_len = sizeof( buf ) - 1;
			bv.bv_val = buf;

			/* Expand value regex */
			if ( a->acl_attrval_style == ACL_STYLE_REGEX )
				tmp_matchesp = matches;
			else switch ( a->acl_dn_style ) {
			case ACL_STYLE_REGEX:
				if ( !BER_BVISNULL( &a->acl_dn_pat ) ) {
					tmp_matchesp = matches;
					break;
				}
			/* FALLTHRU: applies also to ACL_STYLE_REGEX when pattern is "*" */

			case ACL_STYLE_BASE:
				tmp_data[0].rm_so = 0;
				tmp_data[0].rm_eo = e->e_nname.bv_len;
				tmp_matches.dn_count = 1;
				break;

			case ACL_STYLE_ONE:
			case ACL_STYLE_SUBTREE:
			case ACL_STYLE_CHILDREN:
				tmp_data[0].rm_so = 0;
				tmp_data[0].rm_eo = e->e_nname.bv_len;
				tmp_data[1].rm_so = e->e_nname.bv_len - a->acl_dn_pat.bv_len;
				tmp_data[1].rm_eo = e->e_nname.bv_len;
				tmp_matches.dn_count = 2;
				break;

			default:
				/* error */
				rc = 1;
				break;
			}

			if ( rc ) {
				return 1;
			}

			if ( acl_string_expand( &bv, &bdn->a_pat, 
						&e->e_nname, 
						val, tmp_matchesp ) )
			{
				return 1;
			}
			
			if ( dnNormalize(0, NULL, NULL, &bv,
					&pat, op->o_tmpmemctx )
					!= LDAP_SUCCESS )
			{
				/* did not expand to a valid dn */
				return 1;
			}

		} else {
			pat = bdn->a_pat;
		}

		patlen = pat.bv_len;
		odnlen = opndn->bv_len;
		if ( odnlen < patlen ) {
			goto dn_match_cleanup;

		}

		if ( bdn->a_style == ACL_STYLE_BASE ) {
			/* base dn -- entire object DN must match */
			if ( odnlen != patlen ) {
				goto dn_match_cleanup;
			}

		} else if ( bdn->a_style == ACL_STYLE_ONE ) {
			ber_len_t	rdnlen = 0;

			if ( odnlen <= patlen ) {
				goto dn_match_cleanup;
			}

			if ( !DN_SEPARATOR( opndn->bv_val[odnlen - patlen - 1] ) ) {
				goto dn_match_cleanup;
			}

			rdnlen = dn_rdnlen( NULL, opndn );
			if ( rdnlen - ( odnlen - patlen - 1 ) != 0 ) {
				goto dn_match_cleanup;
			}

		} else if ( bdn->a_style == ACL_STYLE_SUBTREE ) {
			if ( odnlen > patlen && !DN_SEPARATOR( opndn->bv_val[odnlen - patlen - 1] ) ) {
				goto dn_match_cleanup;
			}

		} else if ( bdn->a_style == ACL_STYLE_CHILDREN ) {
			if ( odnlen <= patlen ) {
				goto dn_match_cleanup;
			}

			if ( !DN_SEPARATOR( opndn->bv_val[odnlen - patlen - 1] ) ) {
				goto dn_match_cleanup;
			}

		} else if ( bdn->a_style == ACL_STYLE_LEVEL ) {
			int		level = bdn->a_level;
			struct berval	ndn;

			if ( odnlen <= patlen ) {
				goto dn_match_cleanup;
			}

			if ( level > 0 && !DN_SEPARATOR( opndn->bv_val[odnlen - patlen - 1] ) )
			{
				goto dn_match_cleanup;
			}
			
			ndn = *opndn;
			for ( ; level > 0; level-- ) {
				if ( BER_BVISEMPTY( &ndn ) ) {
					goto dn_match_cleanup;
				}
				dnParent( &ndn, &ndn );
				if ( ndn.bv_len < patlen ) {
					goto dn_match_cleanup;
				}
			}
			
			if ( ndn.bv_len != patlen ) {
				goto dn_match_cleanup;
			}
		}

		got_match = !strcmp( pat.bv_val, &opndn->bv_val[ odnlen - patlen ] );

dn_match_cleanup:;
		if ( pat.bv_val != bdn->a_pat.bv_val ) {
			slap_sl_free( pat.bv_val, op->o_tmpmemctx );
		}

		if ( !got_match ) {
			return 1;
		}
	}

	return 0;
}

static int
acl_mask_dnattr(
	Operation		*op,
	Entry			*e,
	struct berval		*val,
	AccessControl		*a,
	int			count,
	AccessControlState	*state,
	slap_mask_t			*mask,
	slap_dn_access		*bdn,
	struct berval		*opndn )
{
	Attribute	*at;
	struct berval	bv;
	int		rc, match = 0;
	const char	*text;
	const char	*attr = bdn->a_at->ad_cname.bv_val;

	assert( attr != NULL );

	if ( BER_BVISEMPTY( opndn ) ) {
		return 1;
	}

	Debug( LDAP_DEBUG_ACL, "<= check a_dn_at: %s\n", attr, 0, 0 );
	bv = *opndn;

	/* see if asker is listed in dnattr */
	for ( at = attrs_find( e->e_attrs, bdn->a_at );
		at != NULL;
		at = attrs_find( at->a_next, bdn->a_at ) )
	{
		if ( attr_valfind( at,
			SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
				SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
			&bv, NULL, op->o_tmpmemctx ) == 0 )
		{
			/* found it */
			match = 1;
			break;
		}
	}

	if ( match ) {
		/* have a dnattr match. if this is a self clause then
		 * the target must also match the op dn.
		 */
		if ( bdn->a_self ) {
			/* check if the target is an attribute. */
			if ( val == NULL ) return 1;

			/* target is attribute, check if the attribute value
			 * is the op dn.
			 */
			rc = value_match( &match, bdn->a_at,
				bdn->a_at->ad_type->sat_equality, 0,
				val, &bv, &text );
			/* on match error or no match, fail the ACL clause */
			if ( rc != LDAP_SUCCESS || match != 0 )
				return 1;
		}

	} else {
		/* no dnattr match, check if this is a self clause */
		if ( ! bdn->a_self )
			return 1;

		/* this is a self clause, check if the target is an
		 * attribute.
		 */
		if ( val == NULL )
			return 1;

		/* target is attribute, check if the attribute value
		 * is the op dn.
		 */
		rc = value_match( &match, bdn->a_at,
			bdn->a_at->ad_type->sat_equality, 0,
			val, &bv, &text );

		/* on match error or no match, fail the ACL clause */
		if ( rc != LDAP_SUCCESS || match != 0 )
			return 1;
	}

	return 0;
}


/*
 * slap_acl_mask - modifies mask based upon the given acl and the
 * requested access to entry e, attribute attr, value val.  if val
 * is null, access to the whole attribute is assumed (all values).
 *
 * returns	0	access NOT allowed
 *		1	access allowed
 */

static slap_control_t
slap_acl_mask(
	AccessControl		*a,
	AccessControl		*prev,
	slap_mask_t		*mask,
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	AclRegexMatches		*matches,
	int			count,
	AccessControlState	*state,
	slap_access_t	access )
{
	int		i;
	Access		*b;
#ifdef LDAP_DEBUG
	char		accessmaskbuf[ACCESSMASK_MAXLEN];
#endif /* DEBUG */
	const char	*attr;
#ifdef SLAP_DYNACL
	slap_mask_t	a2pmask = ACL_ACCESS2PRIV( access );
#endif /* SLAP_DYNACL */

	assert( a != NULL );
	assert( mask != NULL );
	assert( desc != NULL );

	attr = desc->ad_cname.bv_val;

	assert( attr != NULL );

	Debug( LDAP_DEBUG_ACL,
		"=> acl_mask: access to entry \"%s\", attr \"%s\" requested\n",
		e->e_dn, attr, 0 );

	Debug( LDAP_DEBUG_ACL,
		"=> acl_mask: to %s by \"%s\", (%s) \n",
		val ? "value" : "all values",
		op->o_ndn.bv_val ?  op->o_ndn.bv_val : "",
		accessmask2str( *mask, accessmaskbuf, 1 ) );


	b = a->acl_access;
	i = 1;

	for ( ; b != NULL; b = b->a_next, i++ ) {
		slap_mask_t oldmask, modmask;

		ACL_INVALIDATE( modmask );

		/* check for the "self" modifier in the <access> field */
		if ( b->a_dn.a_self ) {
			const char *dummy;
			int rc, match = 0;

			ACL_RECORD_VALUE_STATE;

			/* must have DN syntax */
			if ( desc->ad_type->sat_syntax != slap_schema.si_syn_distinguishedName &&
				!is_at_syntax( desc->ad_type, SLAPD_NAMEUID_SYNTAX )) continue;

			/* check if the target is an attribute. */
			if ( val == NULL ) continue;

			/* a DN must be present */
			if ( BER_BVISEMPTY( &op->o_ndn ) ) {
				continue;
			}

			/* target is attribute, check if the attribute value
			 * is the op dn.
			 */
			rc = value_match( &match, desc,
				desc->ad_type->sat_equality, 0,
				val, &op->o_ndn, &dummy );
			/* on match error or no match, fail the ACL clause */
			if ( rc != LDAP_SUCCESS || match != 0 )
				continue;
		}

		/* AND <who> clauses */
		if ( !BER_BVISEMPTY( &b->a_dn_pat ) ) {
			Debug( LDAP_DEBUG_ACL, "<= check a_dn_pat: %s\n",
				b->a_dn_pat.bv_val, 0, 0);
			/*
			 * if access applies to the entry itself, and the
			 * user is bound as somebody in the same namespace as
			 * the entry, OR the given dn matches the dn pattern
			 */
			/*
			 * NOTE: styles "anonymous", "users" and "self" 
			 * have been moved to enum slap_style_t, whose 
			 * value is set in a_dn_style; however, the string
			 * is maintained in a_dn_pat.
			 */

			if ( acl_mask_dn( op, e, val, a, matches,
				&b->a_dn, &op->o_ndn ) )
			{
				continue;
			}
		}

		if ( !BER_BVISEMPTY( &b->a_realdn_pat ) ) {
			struct berval	ndn;

			Debug( LDAP_DEBUG_ACL, "<= check a_realdn_pat: %s\n",
				b->a_realdn_pat.bv_val, 0, 0);
			/*
			 * if access applies to the entry itself, and the
			 * user is bound as somebody in the same namespace as
			 * the entry, OR the given dn matches the dn pattern
			 */
			/*
			 * NOTE: styles "anonymous", "users" and "self" 
			 * have been moved to enum slap_style_t, whose 
			 * value is set in a_dn_style; however, the string
			 * is maintained in a_dn_pat.
			 */

			if ( op->o_conn && !BER_BVISNULL( &op->o_conn->c_ndn ) )
			{
				ndn = op->o_conn->c_ndn;
			} else {
				ndn = op->o_ndn;
			}

			if ( acl_mask_dn( op, e, val, a, matches,
				&b->a_realdn, &ndn ) )
			{
				continue;
			}
		}

		if ( !BER_BVISEMPTY( &b->a_sockurl_pat ) ) {
			if ( ! op->o_conn->c_listener ) {
				continue;
			}
			Debug( LDAP_DEBUG_ACL, "<= check a_sockurl_pat: %s\n",
				b->a_sockurl_pat.bv_val, 0, 0 );

			if ( !ber_bvccmp( &b->a_sockurl_pat, '*' ) ) {
				if ( b->a_sockurl_style == ACL_STYLE_REGEX) {
					if ( !regex_matches( &b->a_sockurl_pat, op->o_conn->c_listener_url.bv_val,
							&e->e_nname, val, matches ) ) 
					{
						continue;
					}

				} else if ( b->a_sockurl_style == ACL_STYLE_EXPAND ) {
					struct berval	bv;
					char buf[ACL_BUF_SIZE];

					bv.bv_len = sizeof( buf ) - 1;
					bv.bv_val = buf;
					if ( acl_string_expand( &bv, &b->a_sockurl_pat, &e->e_nname, val, matches ) )
					{
						continue;
					}

					if ( ber_bvstrcasecmp( &bv, &op->o_conn->c_listener_url ) != 0 )
					{
						continue;
					}

				} else {
					if ( ber_bvstrcasecmp( &b->a_sockurl_pat, &op->o_conn->c_listener_url ) != 0 )
					{
						continue;
					}
				}
			}
		}

		if ( !BER_BVISEMPTY( &b->a_domain_pat ) ) {
			if ( !op->o_conn->c_peer_domain.bv_val ) {
				continue;
			}
			Debug( LDAP_DEBUG_ACL, "<= check a_domain_pat: %s\n",
				b->a_domain_pat.bv_val, 0, 0 );
			if ( !ber_bvccmp( &b->a_domain_pat, '*' ) ) {
				if ( b->a_domain_style == ACL_STYLE_REGEX) {
					if ( !regex_matches( &b->a_domain_pat, op->o_conn->c_peer_domain.bv_val,
							&e->e_nname, val, matches ) ) 
					{
						continue;
					}
				} else {
					char buf[ACL_BUF_SIZE];

					struct berval 	cmp = op->o_conn->c_peer_domain;
					struct berval 	pat = b->a_domain_pat;

					if ( b->a_domain_expand ) {
						struct berval bv;

						bv.bv_len = sizeof(buf) - 1;
						bv.bv_val = buf;

						if ( acl_string_expand(&bv, &b->a_domain_pat, &e->e_nname, val, matches) )
						{
							continue;
						}
						pat = bv;
					}

					if ( b->a_domain_style == ACL_STYLE_SUBTREE ) {
						int offset = cmp.bv_len - pat.bv_len;
						if ( offset < 0 ) {
							continue;
						}

						if ( offset == 1 || ( offset > 1 && cmp.bv_val[ offset - 1 ] != '.' ) ) {
							continue;
						}

						/* trim the domain */
						cmp.bv_val = &cmp.bv_val[ offset ];
						cmp.bv_len -= offset;
					}
					
					if ( ber_bvstrcasecmp( &pat, &cmp ) != 0 ) {
						continue;
					}
				}
			}
		}

		if ( !BER_BVISEMPTY( &b->a_peername_pat ) ) {
			if ( !op->o_conn->c_peer_name.bv_val ) {
				continue;
			}
			Debug( LDAP_DEBUG_ACL, "<= check a_peername_path: %s\n",
				b->a_peername_pat.bv_val, 0, 0 );
			if ( !ber_bvccmp( &b->a_peername_pat, '*' ) ) {
				if ( b->a_peername_style == ACL_STYLE_REGEX ) {
					if ( !regex_matches( &b->a_peername_pat, op->o_conn->c_peer_name.bv_val,
							&e->e_nname, val, matches ) ) 
					{
						continue;
					}

				} else {
					/* try exact match */
					if ( b->a_peername_style == ACL_STYLE_BASE ) {
						if ( ber_bvstrcasecmp( &b->a_peername_pat, &op->o_conn->c_peer_name ) != 0 ) {
							continue;
						}

					} else if ( b->a_peername_style == ACL_STYLE_EXPAND ) {
						struct berval	bv;
						char buf[ACL_BUF_SIZE];

						bv.bv_len = sizeof( buf ) - 1;
						bv.bv_val = buf;
						if ( acl_string_expand( &bv, &b->a_peername_pat, &e->e_nname, val, matches ) )
						{
							continue;
						}

						if ( ber_bvstrcasecmp( &bv, &op->o_conn->c_peer_name ) != 0 ) {
							continue;
						}

					/* extract IP and try exact match */
					} else if ( b->a_peername_style == ACL_STYLE_IP ) {
						char		*port;
						char		buf[STRLENOF("255.255.255.255") + 1];
						struct berval	ip;
						unsigned long	addr;
						int		port_number = -1;
						
						if ( strncasecmp( op->o_conn->c_peer_name.bv_val, 
									acl_bv_ip_eq.bv_val,
									acl_bv_ip_eq.bv_len ) != 0 ) 
							continue;

						ip.bv_val = op->o_conn->c_peer_name.bv_val + acl_bv_ip_eq.bv_len;
						ip.bv_len = op->o_conn->c_peer_name.bv_len - acl_bv_ip_eq.bv_len;

						port = strrchr( ip.bv_val, ':' );
						if ( port ) {
							ip.bv_len = port - ip.bv_val;
							++port;
							if ( lutil_atoi( &port_number, port ) != 0 )
								continue;
						}
						
						/* the port check can be anticipated here */
						if ( b->a_peername_port != -1 && port_number != b->a_peername_port )
							continue;
						
						/* address longer than expected? */
						if ( ip.bv_len >= sizeof(buf) )
							continue;

						AC_MEMCPY( buf, ip.bv_val, ip.bv_len );
						buf[ ip.bv_len ] = '\0';

						addr = inet_addr( buf );

						/* unable to convert? */
						if ( addr == (unsigned long)(-1) )
							continue;

						if ( (addr & b->a_peername_mask) != b->a_peername_addr )
							continue;

#ifdef LDAP_PF_INET6
					/* extract IPv6 and try exact match */
					} else if ( b->a_peername_style == ACL_STYLE_IPV6 ) {
						char		*port;
						char		buf[STRLENOF("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF") + 1];
						struct berval	ip;
						struct in6_addr	addr;
						int		port_number = -1;
						
						if ( strncasecmp( op->o_conn->c_peer_name.bv_val, 
									acl_bv_ipv6_eq.bv_val,
									acl_bv_ipv6_eq.bv_len ) != 0 ) 
							continue;

						ip.bv_val = op->o_conn->c_peer_name.bv_val + acl_bv_ipv6_eq.bv_len;
						ip.bv_len = op->o_conn->c_peer_name.bv_len - acl_bv_ipv6_eq.bv_len;

						port = strrchr( ip.bv_val, ']' );
						if ( port ) {
							ip.bv_len = port - ip.bv_val;
							++port;
							if ( port[0] == ':' && lutil_atoi( &port_number, ++port ) != 0 )
								continue;
						}
						
						/* the port check can be anticipated here */
						if ( b->a_peername_port != -1 && port_number != b->a_peername_port )
							continue;
						
						/* address longer than expected? */
						if ( ip.bv_len >= sizeof(buf) )
							continue;

						AC_MEMCPY( buf, ip.bv_val, ip.bv_len );
						buf[ ip.bv_len ] = '\0';

						if ( inet_pton( AF_INET6, buf, &addr ) != 1 )
							continue;

						/* check mask */
						if ( !slap_addr6_mask( &addr, &b->a_peername_mask6, &b->a_peername_addr6 ) )
							continue;
#endif /* LDAP_PF_INET6 */

#ifdef LDAP_PF_LOCAL
					/* extract path and try exact match */
					} else if ( b->a_peername_style == ACL_STYLE_PATH ) {
						struct berval path;
						
						if ( strncmp( op->o_conn->c_peer_name.bv_val,
									acl_bv_path_eq.bv_val,
									acl_bv_path_eq.bv_len ) != 0 )
							continue;

						path.bv_val = op->o_conn->c_peer_name.bv_val
							+ acl_bv_path_eq.bv_len;
						path.bv_len = op->o_conn->c_peer_name.bv_len
							- acl_bv_path_eq.bv_len;

						if ( ber_bvcmp( &b->a_peername_pat, &path ) != 0 )
							continue;

#endif /* LDAP_PF_LOCAL */

					/* exact match (very unlikely...) */
					} else if ( ber_bvcmp( &op->o_conn->c_peer_name, &b->a_peername_pat ) != 0 ) {
							continue;
					}
				}
			}
		}

		if ( !BER_BVISEMPTY( &b->a_sockname_pat ) ) {
			if ( BER_BVISNULL( &op->o_conn->c_sock_name ) ) {
				continue;
			}
			Debug( LDAP_DEBUG_ACL, "<= check a_sockname_path: %s\n",
				b->a_sockname_pat.bv_val, 0, 0 );
			if ( !ber_bvccmp( &b->a_sockname_pat, '*' ) ) {
				if ( b->a_sockname_style == ACL_STYLE_REGEX) {
					if ( !regex_matches( &b->a_sockname_pat, op->o_conn->c_sock_name.bv_val,
							&e->e_nname, val, matches ) ) 
					{
						continue;
					}

				} else if ( b->a_sockname_style == ACL_STYLE_EXPAND ) {
					struct berval	bv;
					char buf[ACL_BUF_SIZE];

					bv.bv_len = sizeof( buf ) - 1;
					bv.bv_val = buf;
					if ( acl_string_expand( &bv, &b->a_sockname_pat, &e->e_nname, val, matches ) )
					{
						continue;
					}

					if ( ber_bvstrcasecmp( &bv, &op->o_conn->c_sock_name ) != 0 ) {
						continue;
					}

				} else {
					if ( ber_bvstrcasecmp( &b->a_sockname_pat, &op->o_conn->c_sock_name ) != 0 ) {
						continue;
					}
				}
			}
		}

		if ( b->a_dn_at != NULL ) {
			if ( acl_mask_dnattr( op, e, val, a,
					count, state, mask,
					&b->a_dn, &op->o_ndn ) )
			{
				continue;
			}
		}

		if ( b->a_realdn_at != NULL ) {
			struct berval	ndn;

			if ( op->o_conn && !BER_BVISNULL( &op->o_conn->c_ndn ) )
			{
				ndn = op->o_conn->c_ndn;
			} else {
				ndn = op->o_ndn;
			}

			if ( acl_mask_dnattr( op, e, val, a,
					count, state, mask,
					&b->a_realdn, &ndn ) )
			{
				continue;
			}
		}

		if ( !BER_BVISEMPTY( &b->a_group_pat ) ) {
			struct berval bv;
			struct berval ndn = BER_BVNULL;
			int rc;

			if ( op->o_ndn.bv_len == 0 ) {
				continue;
			}

			Debug( LDAP_DEBUG_ACL, "<= check a_group_pat: %s\n",
				b->a_group_pat.bv_val, 0, 0 );

			/* b->a_group is an unexpanded entry name, expanded it should be an 
			 * entry with objectclass group* and we test to see if odn is one of
			 * the values in the attribute group
			 */
			/* see if asker is listed in dnattr */
			if ( b->a_group_style == ACL_STYLE_EXPAND ) {
				char		buf[ACL_BUF_SIZE];
				AclRegexMatches	tmp_matches,
						*tmp_matchesp = &tmp_matches;
				regmatch_t 	*tmp_data;

				MATCHES_MEMSET( &tmp_matches );
				tmp_data = &tmp_matches.dn_data[0];

				bv.bv_len = sizeof(buf) - 1;
				bv.bv_val = buf;

				rc = 0;

				if ( a->acl_attrval_style == ACL_STYLE_REGEX )
					tmp_matchesp = matches;
				else switch ( a->acl_dn_style ) {
				case ACL_STYLE_REGEX:
					if ( !BER_BVISNULL( &a->acl_dn_pat ) ) {
						tmp_matchesp = matches;
						break;
					}

				/* FALLTHRU: applies also to ACL_STYLE_REGEX when pattern is "*" */
				case ACL_STYLE_BASE:
					tmp_data[0].rm_so = 0;
					tmp_data[0].rm_eo = e->e_nname.bv_len;
					tmp_matches.dn_count = 1;
					break;

				case ACL_STYLE_ONE:
				case ACL_STYLE_SUBTREE:
				case ACL_STYLE_CHILDREN:
					tmp_data[0].rm_so = 0;
					tmp_data[0].rm_eo = e->e_nname.bv_len;

					tmp_data[1].rm_so = e->e_nname.bv_len - a->acl_dn_pat.bv_len;
					tmp_data[1].rm_eo = e->e_nname.bv_len;
					tmp_matches.dn_count = 2;
					break;

				default:
					/* error */
					rc = 1;
					break;
				}

				if ( rc ) {
					continue;
				}
				
				if ( acl_string_expand( &bv, &b->a_group_pat,
						&e->e_nname, val,
						tmp_matchesp ) )
				{
					continue;
				}

				if ( dnNormalize( 0, NULL, NULL, &bv, &ndn,
						op->o_tmpmemctx ) != LDAP_SUCCESS )
				{
					/* did not expand to a valid dn */
					continue;
				}

				bv = ndn;

			} else {
				bv = b->a_group_pat;
			}

			rc = backend_group( op, e, &bv, &op->o_ndn,
				b->a_group_oc, b->a_group_at );

			if ( ndn.bv_val ) {
				slap_sl_free( ndn.bv_val, op->o_tmpmemctx );
			}

			if ( rc != 0 ) {
				continue;
			}
		}

		if ( !BER_BVISEMPTY( &b->a_set_pat ) ) {
			struct berval	bv;
			char		buf[ACL_BUF_SIZE];

			Debug( LDAP_DEBUG_ACL, "<= check a_set_pat: %s\n",
				b->a_set_pat.bv_val, 0, 0 );

			if ( b->a_set_style == ACL_STYLE_EXPAND ) {
				AclRegexMatches	tmp_matches,
						*tmp_matchesp = &tmp_matches;
				int		rc = 0;
				regmatch_t 	*tmp_data;

				MATCHES_MEMSET( &tmp_matches );
				tmp_data = &tmp_matches.dn_data[0];

				bv.bv_len = sizeof( buf ) - 1;
				bv.bv_val = buf;

				rc = 0;

				if ( a->acl_attrval_style == ACL_STYLE_REGEX )
					tmp_matchesp = matches;
				else switch ( a->acl_dn_style ) {
				case ACL_STYLE_REGEX:
					if ( !BER_BVISNULL( &a->acl_dn_pat ) ) {
						tmp_matchesp = matches;
						break;
					}

				/* FALLTHRU: applies also to ACL_STYLE_REGEX when pattern is "*" */
				case ACL_STYLE_BASE:
					tmp_data[0].rm_so = 0;
					tmp_data[0].rm_eo = e->e_nname.bv_len;
					tmp_matches.dn_count = 1;
					break;

				case ACL_STYLE_ONE:
				case ACL_STYLE_SUBTREE:
				case ACL_STYLE_CHILDREN:
					tmp_data[0].rm_so = 0;
					tmp_data[0].rm_eo = e->e_nname.bv_len;
					tmp_data[1].rm_so = e->e_nname.bv_len - a->acl_dn_pat.bv_len;
					tmp_data[1].rm_eo = e->e_nname.bv_len; tmp_matches.dn_count = 2;
					break;

				default:
					/* error */
					rc = 1;
					break;
				}

				if ( rc ) {
					continue;
				}
				
				if ( acl_string_expand( &bv, &b->a_set_pat,
						&e->e_nname, val,
						tmp_matchesp ) )
				{
					continue;
				}

			} else {
				bv = b->a_set_pat;
			}
			
			if ( acl_match_set( &bv, op, e, NULL ) == 0 ) {
				continue;
			}
		}

		if ( b->a_authz.sai_ssf ) {
			Debug( LDAP_DEBUG_ACL, "<= check a_authz.sai_ssf: ACL %u > OP %u\n",
				b->a_authz.sai_ssf, op->o_ssf, 0 );
			if ( b->a_authz.sai_ssf >  op->o_ssf ) {
				continue;
			}
		}

		if ( b->a_authz.sai_transport_ssf ) {
			Debug( LDAP_DEBUG_ACL,
				"<= check a_authz.sai_transport_ssf: ACL %u > OP %u\n",
				b->a_authz.sai_transport_ssf, op->o_transport_ssf, 0 );
			if ( b->a_authz.sai_transport_ssf >  op->o_transport_ssf ) {
				continue;
			}
		}

		if ( b->a_authz.sai_tls_ssf ) {
			Debug( LDAP_DEBUG_ACL,
				"<= check a_authz.sai_tls_ssf: ACL %u > OP %u\n",
				b->a_authz.sai_tls_ssf, op->o_tls_ssf, 0 );
			if ( b->a_authz.sai_tls_ssf >  op->o_tls_ssf ) {
				continue;
			}
		}

		if ( b->a_authz.sai_sasl_ssf ) {
			Debug( LDAP_DEBUG_ACL,
				"<= check a_authz.sai_sasl_ssf: ACL %u > OP %u\n",
				b->a_authz.sai_sasl_ssf, op->o_sasl_ssf, 0 );
			if ( b->a_authz.sai_sasl_ssf >	op->o_sasl_ssf ) {
				continue;
			}
		}

#ifdef SLAP_DYNACL
		if ( b->a_dynacl ) {
			slap_dynacl_t	*da;
			slap_access_t	tgrant, tdeny;

			Debug( LDAP_DEBUG_ACL, "<= check a_dynacl\n",
				0, 0, 0 );

			/* this case works different from the others above.
			 * since dynamic ACL's themselves give permissions, we need
			 * to first check b->a_access_mask, the ACL's access level.
			 */
			/* first check if the right being requested
			 * is allowed by the ACL clause.
			 */
			if ( ! ACL_PRIV_ISSET( b->a_access_mask, a2pmask ) ) {
				continue;
			}

			/* start out with nothing granted, nothing denied */
			ACL_INVALIDATE(tgrant);
			ACL_INVALIDATE(tdeny);

			for ( da = b->a_dynacl; da; da = da->da_next ) {
				slap_access_t	grant,
						deny;

				ACL_INVALIDATE(grant);
				ACL_INVALIDATE(deny);

				Debug( LDAP_DEBUG_ACL, "    <= check a_dynacl: %s\n",
					da->da_name, 0, 0 );

				/*
				 * XXXmanu Only DN matches are supplied 
				 * sending attribute values matches require
				 * an API update
				 */
				(void)da->da_mask( da->da_private, op, e, desc,
					val, matches->dn_count, matches->dn_data, 
					&grant, &deny ); 

				tgrant |= grant;
				tdeny |= deny;
			}

			/* remove anything that the ACL clause does not allow */
			tgrant &= b->a_access_mask & ACL_PRIV_MASK;
			tdeny &= ACL_PRIV_MASK;

			/* see if we have anything to contribute */
			if( ACL_IS_INVALID(tgrant) && ACL_IS_INVALID(tdeny) ) { 
				continue;
			}

			/* this could be improved by changing slap_acl_mask so that it can deal with
			 * by clauses that return grant/deny pairs.  Right now, it does either
			 * additive or subtractive rights, but not both at the same time.  So,
			 * we need to combine the grant/deny pair into a single rights mask in
			 * a smart way:	 if either grant or deny is "empty", then we use the
			 * opposite as is, otherwise we remove any denied rights from the grant
			 * rights mask and construct an additive mask.
			 */
			if (ACL_IS_INVALID(tdeny)) {
				modmask = tgrant | ACL_PRIV_ADDITIVE;

			} else if (ACL_IS_INVALID(tgrant)) {
				modmask = tdeny | ACL_PRIV_SUBSTRACTIVE;

			} else {
				modmask = (tgrant & ~tdeny) | ACL_PRIV_ADDITIVE;
			}

		} else
#endif /* SLAP_DYNACL */
		{
			modmask = b->a_access_mask;
		}

		Debug( LDAP_DEBUG_ACL,
			"<= acl_mask: [%d] applying %s (%s)\n",
			i, accessmask2str( modmask, accessmaskbuf, 1 ), 
			b->a_type == ACL_CONTINUE
				? "continue"
				: b->a_type == ACL_BREAK
					? "break"
					: "stop" );
		/* save old mask */
		oldmask = *mask;

		if( ACL_IS_ADDITIVE(modmask) ) {
			/* add privs */
			ACL_PRIV_SET( *mask, modmask );

			/* cleanup */
			ACL_PRIV_CLR( *mask, ~ACL_PRIV_MASK );

		} else if( ACL_IS_SUBTRACTIVE(modmask) ) {
			/* substract privs */
			ACL_PRIV_CLR( *mask, modmask );

			/* cleanup */
			ACL_PRIV_CLR( *mask, ~ACL_PRIV_MASK );

		} else {
			/* assign privs */
			*mask = modmask;
		}

		Debug( LDAP_DEBUG_ACL,
			"<= acl_mask: [%d] mask: %s\n",
			i, accessmask2str(*mask, accessmaskbuf, 1), 0 );

		if( b->a_type == ACL_CONTINUE ) {
			continue;

		} else if ( b->a_type == ACL_BREAK ) {
			return ACL_BREAK;

		} else {
			return ACL_STOP;
		}
	}

	/* implicit "by * none" clause */
	ACL_INIT(*mask);

	Debug( LDAP_DEBUG_ACL,
		"<= acl_mask: no more <who> clauses, returning %s (stop)\n",
		accessmask2str(*mask, accessmaskbuf, 1), 0, 0 );
	return ACL_STOP;
}

/*
 * acl_check_modlist - check access control on the given entry to see if
 * it allows the given modifications by the user associated with op.
 * returns	1	if mods allowed ok
 *		0	mods not allowed
 */

int
acl_check_modlist(
	Operation	*op,
	Entry	*e,
	Modifications	*mlist )
{
	struct berval *bv;
	AccessControlState state = ACL_STATE_INIT;
	Backend *be;
	int be_null = 0;
	int ret = 1; /* default is access allowed */

	be = op->o_bd;
	if ( be == NULL ) {
		be = LDAP_STAILQ_FIRST(&backendDB);
		be_null = 1;
		op->o_bd = be;
	}
	assert( be != NULL );

	/* If ADD attribute checking is not enabled, just allow it */
	if ( op->o_tag == LDAP_REQ_ADD && !SLAP_DBACL_ADD( be ))
		return 1;

	/* short circuit root database access */
	if ( be_isroot( op ) ) {
		Debug( LDAP_DEBUG_ACL,
			"<= acl_access_allowed: granted to database root\n",
		    0, 0, 0 );
		goto done;
	}

	/* use backend default access if no backend acls */
	if( op->o_bd != NULL && op->o_bd->be_acl == NULL && frontendDB->be_acl == NULL ) {
		Debug( LDAP_DEBUG_ACL,
			"=> access_allowed: backend default %s access %s to \"%s\"\n",
			access2str( ACL_WRITE ),
			op->o_bd->be_dfltaccess >= ACL_WRITE
				? "granted" : "denied",
			op->o_dn.bv_val );
		ret = (op->o_bd->be_dfltaccess >= ACL_WRITE);
		goto done;
	}

	for ( ; mlist != NULL; mlist = mlist->sml_next ) {
		/*
		 * Internal mods are ignored by ACL_WRITE checking
		 */
		if ( mlist->sml_flags & SLAP_MOD_INTERNAL ) {
			Debug( LDAP_DEBUG_ACL, "acl: internal mod %s:"
				" modify access granted\n",
				mlist->sml_desc->ad_cname.bv_val, 0, 0 );
			continue;
		}

		/*
		 * no-user-modification operational attributes are ignored
		 * by ACL_WRITE checking as any found here are not provided
		 * by the user
		 */
		if ( is_at_no_user_mod( mlist->sml_desc->ad_type )
				&& ! ( mlist->sml_flags & SLAP_MOD_MANAGING ) )
		{
			Debug( LDAP_DEBUG_ACL, "acl: no-user-mod %s:"
				" modify access granted\n",
				mlist->sml_desc->ad_cname.bv_val, 0, 0 );
			continue;
		}

		switch ( mlist->sml_op ) {
		case LDAP_MOD_REPLACE:
		case LDAP_MOD_INCREMENT:
			/*
			 * We must check both permission to delete the whole
			 * attribute and permission to add the specific attributes.
			 * This prevents abuse from selfwriters.
			 */
			if ( ! access_allowed( op, e,
				mlist->sml_desc, NULL,
				( mlist->sml_flags & SLAP_MOD_MANAGING ) ? ACL_MANAGE : ACL_WDEL,
				&state ) )
			{
				ret = 0;
				goto done;
			}

			if ( mlist->sml_values == NULL ) break;

			/* fall thru to check value to add */

		case LDAP_MOD_ADD:
		case SLAP_MOD_ADD_IF_NOT_PRESENT:
			assert( mlist->sml_values != NULL );

			if ( mlist->sml_op == SLAP_MOD_ADD_IF_NOT_PRESENT
				&& attr_find( e->e_attrs, mlist->sml_desc ) )
			{
				break;
			}

			for ( bv = mlist->sml_nvalues
					? mlist->sml_nvalues : mlist->sml_values;
				bv->bv_val != NULL; bv++ )
			{
				if ( ! access_allowed( op, e,
					mlist->sml_desc, bv,
					( mlist->sml_flags & SLAP_MOD_MANAGING ) ? ACL_MANAGE : ACL_WADD,
					&state ) )
				{
					ret = 0;
					goto done;
				}
			}
			break;

		case LDAP_MOD_DELETE:
		case SLAP_MOD_SOFTDEL:
			if ( mlist->sml_values == NULL ) {
				if ( ! access_allowed( op, e,
					mlist->sml_desc, NULL,
					( mlist->sml_flags & SLAP_MOD_MANAGING ) ? ACL_MANAGE : ACL_WDEL,
					&state ) )
				{
					ret = 0;
					goto done;
				}
				break;
			}
			for ( bv = mlist->sml_nvalues
					? mlist->sml_nvalues : mlist->sml_values;
				bv->bv_val != NULL; bv++ )
			{
				if ( ! access_allowed( op, e,
					mlist->sml_desc, bv,
					( mlist->sml_flags & SLAP_MOD_MANAGING ) ? ACL_MANAGE : ACL_WDEL,
					&state ) )
				{
					ret = 0;
					goto done;
				}
			}
			break;

		case SLAP_MOD_SOFTADD:
			/* allow adding attribute via modrdn thru */
			break;

		default:
			assert( 0 );
			/* not reached */
			ret = 0;
			break;
		}
	}

done:
	if (be_null) op->o_bd = NULL;
	return( ret );
}

int
acl_get_part(
	struct berval	*list,
	int		ix,
	char		sep,
	struct berval	*bv )
{
	int	len;
	char	*p;

	if ( bv ) {
		BER_BVZERO( bv );
	}
	len = list->bv_len;
	p = list->bv_val;
	while ( len >= 0 && --ix >= 0 ) {
		while ( --len >= 0 && *p++ != sep )
			;
	}
	while ( len >= 0 && *p == ' ' ) {
		len--;
		p++;
	}
	if ( len < 0 ) {
		return -1;
	}

	if ( !bv ) {
		return 0;
	}

	bv->bv_val = p;
	while ( --len >= 0 && *p != sep ) {
		bv->bv_len++;
		p++;
	}
	while ( bv->bv_len > 0 && *--p == ' ' ) {
		bv->bv_len--;
	}
	
	return bv->bv_len;
}

typedef struct acl_set_gather_t {
	SetCookie		*cookie;
	BerVarray		bvals;
} acl_set_gather_t;

static int
acl_set_cb_gather( Operation *op, SlapReply *rs )
{
	acl_set_gather_t	*p = (acl_set_gather_t *)op->o_callback->sc_private;
	
	if ( rs->sr_type == REP_SEARCH ) {
		BerValue	bvals[ 2 ];
		BerVarray	bvalsp = NULL;
		int		j;

		for ( j = 0; !BER_BVISNULL( &rs->sr_attrs[ j ].an_name ); j++ ) {
			AttributeDescription	*desc = rs->sr_attrs[ j ].an_desc;

			if ( desc == NULL ) {
				continue;
			}
			
			if ( desc == slap_schema.si_ad_entryDN ) {
				bvalsp = bvals;
				bvals[ 0 ] = rs->sr_entry->e_nname;
				BER_BVZERO( &bvals[ 1 ] );

			} else {
				Attribute	*a;

				a = attr_find( rs->sr_entry->e_attrs, desc );
				if ( a != NULL ) {
					bvalsp = a->a_nvals;
				}
			}

			if ( bvalsp ) {
				p->bvals = slap_set_join( p->cookie, p->bvals,
						( '|' | SLAP_SET_RREF ), bvalsp );
			}
		}

	} else {
		switch ( rs->sr_type ) {
		case REP_SEARCHREF:
		case REP_INTERMEDIATE:
			/* ignore */
			break;

		default:
			assert( rs->sr_type == REP_RESULT );
			break;
		}
	}

	return 0;
}

BerVarray
acl_set_gather( SetCookie *cookie, struct berval *name, AttributeDescription *desc )
{
	AclSetCookie		*cp = (AclSetCookie *)cookie;
	int			rc = 0;
	LDAPURLDesc		*ludp = NULL;
	Operation		op2 = { 0 };
	SlapReply		rs = {REP_RESULT};
	AttributeName		anlist[ 2 ], *anlistp = NULL;
	int			nattrs = 0;
	slap_callback		cb = { NULL, acl_set_cb_gather, NULL, NULL };
	acl_set_gather_t	p = { 0 };

	/* this routine needs to return the bervals instead of
	 * plain strings, since syntax is not known.  It should
	 * also return the syntax or some "comparison cookie".
	 */
	if ( strncasecmp( name->bv_val, "ldap:///", STRLENOF( "ldap:///" ) ) != 0 ) {
		return acl_set_gather2( cookie, name, desc );
	}

	rc = ldap_url_parse( name->bv_val, &ludp );
	if ( rc != LDAP_URL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"%s acl_set_gather: unable to parse URL=\"%s\"\n",
			cp->asc_op->o_log_prefix, name->bv_val, 0 );

		rc = LDAP_PROTOCOL_ERROR;
		goto url_done;
	}
	
	if ( ( ludp->lud_host && ludp->lud_host[0] ) || ludp->lud_exts )
	{
		/* host part must be empty */
		/* extensions parts must be empty */
		Debug( LDAP_DEBUG_TRACE,
			"%s acl_set_gather: host/exts must be absent in URL=\"%s\"\n",
			cp->asc_op->o_log_prefix, name->bv_val, 0 );

		rc = LDAP_PROTOCOL_ERROR;
		goto url_done;
	}

	/* Grab the searchbase and see if an appropriate database can be found */
	ber_str2bv( ludp->lud_dn, 0, 0, &op2.o_req_dn );
	rc = dnNormalize( 0, NULL, NULL, &op2.o_req_dn,
			&op2.o_req_ndn, cp->asc_op->o_tmpmemctx );
	BER_BVZERO( &op2.o_req_dn );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE,
			"%s acl_set_gather: DN=\"%s\" normalize failed\n",
			cp->asc_op->o_log_prefix, ludp->lud_dn, 0 );

		goto url_done;
	}

	op2.o_bd = select_backend( &op2.o_req_ndn, 1 );
	if ( ( op2.o_bd == NULL ) || ( op2.o_bd->be_search == NULL ) ) {
		Debug( LDAP_DEBUG_TRACE,
			"%s acl_set_gather: no database could be selected for DN=\"%s\"\n",
			cp->asc_op->o_log_prefix, op2.o_req_ndn.bv_val, 0 );

		rc = LDAP_NO_SUCH_OBJECT;
		goto url_done;
	}

	/* Grab the filter */
	if ( ludp->lud_filter ) {
		ber_str2bv_x( ludp->lud_filter, 0, 0, &op2.ors_filterstr,
				cp->asc_op->o_tmpmemctx );
		op2.ors_filter = str2filter_x( cp->asc_op, op2.ors_filterstr.bv_val );
		if ( op2.ors_filter == NULL ) {
			Debug( LDAP_DEBUG_TRACE,
				"%s acl_set_gather: unable to parse filter=\"%s\"\n",
				cp->asc_op->o_log_prefix, op2.ors_filterstr.bv_val, 0 );

			rc = LDAP_PROTOCOL_ERROR;
			goto url_done;
		}
		
	} else {
		op2.ors_filterstr = *slap_filterstr_objectClass_pres;
		op2.ors_filter = (Filter *)slap_filter_objectClass_pres;
	}


	/* Grab the scope */
	op2.ors_scope = ludp->lud_scope;

	/* Grap the attributes */
	if ( ludp->lud_attrs ) {
		int i;

		for ( ; ludp->lud_attrs[ nattrs ]; nattrs++ )
			;

		anlistp = slap_sl_calloc( sizeof( AttributeName ), nattrs + 2,
				cp->asc_op->o_tmpmemctx );

		for ( i = 0, nattrs = 0; ludp->lud_attrs[ i ]; i++ ) {
			struct berval		name;
			AttributeDescription	*desc = NULL;
			const char		*text = NULL;

			ber_str2bv( ludp->lud_attrs[ i ], 0, 0, &name );
			rc = slap_bv2ad( &name, &desc, &text );
			if ( rc == LDAP_SUCCESS ) {
				anlistp[ nattrs ].an_name = name;
				anlistp[ nattrs ].an_desc = desc;
				nattrs++;
			}
		}

	} else {
		anlistp = anlist;
	}

	anlistp[ nattrs ].an_name = desc->ad_cname;
	anlistp[ nattrs ].an_desc = desc;

	BER_BVZERO( &anlistp[ nattrs + 1 ].an_name );
	
	p.cookie = cookie;
	
	op2.o_hdr = cp->asc_op->o_hdr;
	op2.o_tag = LDAP_REQ_SEARCH;
	op2.o_ndn = op2.o_bd->be_rootndn;
	op2.o_callback = &cb;
	slap_op_time( &op2.o_time, &op2.o_tincr );
	op2.o_do_not_cache = 1;
	op2.o_is_auth_check = 0;
	ber_dupbv_x( &op2.o_req_dn, &op2.o_req_ndn, cp->asc_op->o_tmpmemctx );
	op2.ors_slimit = SLAP_NO_LIMIT;
	op2.ors_tlimit = SLAP_NO_LIMIT;
	op2.ors_attrs = anlistp;
	op2.ors_attrsonly = 0;
	op2.o_private = cp->asc_op->o_private;
	op2.o_extra = cp->asc_op->o_extra;

	cb.sc_private = &p;

	rc = op2.o_bd->be_search( &op2, &rs );
	if ( rc != 0 ) {
		goto url_done;
	}

url_done:;
	if ( op2.ors_filter && op2.ors_filter != slap_filter_objectClass_pres ) {
		filter_free_x( cp->asc_op, op2.ors_filter, 1 );
	}
	if ( !BER_BVISNULL( &op2.o_req_ndn ) ) {
		slap_sl_free( op2.o_req_ndn.bv_val, cp->asc_op->o_tmpmemctx );
	}
	if ( !BER_BVISNULL( &op2.o_req_dn ) ) {
		slap_sl_free( op2.o_req_dn.bv_val, cp->asc_op->o_tmpmemctx );
	}
	if ( ludp ) {
		ldap_free_urldesc( ludp );
	}
	if ( anlistp && anlistp != anlist ) {
		slap_sl_free( anlistp, cp->asc_op->o_tmpmemctx );
	}

	return p.bvals;
}

BerVarray
acl_set_gather2( SetCookie *cookie, struct berval *name, AttributeDescription *desc )
{
	AclSetCookie	*cp = (AclSetCookie *)cookie;
	BerVarray	bvals = NULL;
	struct berval	ndn;
	int		rc = 0;

	/* this routine needs to return the bervals instead of
	 * plain strings, since syntax is not known.  It should
	 * also return the syntax or some "comparison cookie".
	 */
	rc = dnNormalize( 0, NULL, NULL, name, &ndn, cp->asc_op->o_tmpmemctx );
	if ( rc == LDAP_SUCCESS ) {
		if ( desc == slap_schema.si_ad_entryDN ) {
			bvals = (BerVarray)slap_sl_malloc( sizeof( BerValue ) * 2,
					cp->asc_op->o_tmpmemctx );
			bvals[ 0 ] = ndn;
			BER_BVZERO( &bvals[ 1 ] );
			BER_BVZERO( &ndn );

		} else {
			backend_attribute( cp->asc_op,
				cp->asc_e, &ndn, desc, &bvals, ACL_NONE );
		}

		if ( !BER_BVISNULL( &ndn ) ) {
			slap_sl_free( ndn.bv_val, cp->asc_op->o_tmpmemctx );
		}
	}

	return bvals;
}

int
acl_match_set (
	struct berval *subj,
	Operation *op,
	Entry *e,
	struct berval *default_set_attribute )
{
	struct berval	set = BER_BVNULL;
	int		rc = 0;
	AclSetCookie	cookie;

	if ( default_set_attribute == NULL ) {
		set = *subj;

	} else {
		struct berval		subjdn, ndn = BER_BVNULL;
		struct berval		setat;
		BerVarray		bvals = NULL;
		const char		*text;
		AttributeDescription	*desc = NULL;

		/* format of string is "entry/setAttrName" */
		if ( acl_get_part( subj, 0, '/', &subjdn ) < 0 ) {
			return 0;
		}

		if ( acl_get_part( subj, 1, '/', &setat ) < 0 ) {
			setat = *default_set_attribute;
		}

		/*
		 * NOTE: dnNormalize honors the ber_len field
		 * as the length of the dn to be normalized
		 */
		if ( slap_bv2ad( &setat, &desc, &text ) == LDAP_SUCCESS ) {
			if ( dnNormalize( 0, NULL, NULL, &subjdn, &ndn, op->o_tmpmemctx ) == LDAP_SUCCESS )
			{
				backend_attribute( op, e, &ndn, desc, &bvals, ACL_NONE );
				if ( bvals != NULL && !BER_BVISNULL( &bvals[0] ) ) {
					int	i;

					set = bvals[0];
					BER_BVZERO( &bvals[0] );
					for ( i = 1; !BER_BVISNULL( &bvals[i] ); i++ )
						/* count */ ;
					bvals[0].bv_val = bvals[i-1].bv_val;
					BER_BVZERO( &bvals[i-1] );
				}
				ber_bvarray_free_x( bvals, op->o_tmpmemctx );
				slap_sl_free( ndn.bv_val, op->o_tmpmemctx );
			}
		}
	}

	if ( !BER_BVISNULL( &set ) ) {
		cookie.asc_op = op;
		cookie.asc_e = e;
		rc = ( slap_set_filter(
			acl_set_gather,
			(SetCookie *)&cookie, &set,
			&op->o_ndn, &e->e_nname, NULL ) > 0 );
		if ( set.bv_val != subj->bv_val ) {
			slap_sl_free( set.bv_val, op->o_tmpmemctx );
		}
	}

	return(rc);
}

#ifdef SLAP_DYNACL

/*
 * dynamic ACL infrastructure
 */
static slap_dynacl_t	*da_list = NULL;

int
slap_dynacl_register( slap_dynacl_t *da )
{
	slap_dynacl_t	*tmp;

	for ( tmp = da_list; tmp; tmp = tmp->da_next ) {
		if ( strcasecmp( da->da_name, tmp->da_name ) == 0 ) {
			break;
		}
	}

	if ( tmp != NULL ) {
		return -1;
	}
	
	if ( da->da_mask == NULL ) {
		return -1;
	}
	
	da->da_private = NULL;
	da->da_next = da_list;
	da_list = da;

	return 0;
}

static slap_dynacl_t *
slap_dynacl_next( slap_dynacl_t *da )
{
	if ( da ) {
		return da->da_next;
	}
	return da_list;
}

slap_dynacl_t *
slap_dynacl_get( const char *name )
{
	slap_dynacl_t	*da;

	for ( da = slap_dynacl_next( NULL ); da; da = slap_dynacl_next( da ) ) {
		if ( strcasecmp( da->da_name, name ) == 0 ) {
			break;
		}
	}

	return da;
}
#endif /* SLAP_DYNACL */

/*
 * statically built-in dynamic ACL initialization
 */
static int (*acl_init_func[])( void ) = {
#ifdef SLAP_DYNACL
	/* TODO: remove when ACI will only be dynamic */
#if SLAPD_ACI_ENABLED == SLAPD_MOD_STATIC
	dynacl_aci_init,
#endif /* SLAPD_ACI_ENABLED */
#endif /* SLAP_DYNACL */

	NULL
};

int
acl_init( void )
{
	int	i, rc;

	for ( i = 0; acl_init_func[ i ] != NULL; i++ ) {
		rc = (*(acl_init_func[ i ]))();
		if ( rc != 0 ) {
			return rc;
		}
	}

	return 0;
}

int
acl_string_expand(
	struct berval	*bv,
	struct berval	*pat,
	struct berval	*dn_matches,
	struct berval	*val_matches,
	AclRegexMatches	*matches)
{
	ber_len_t	size;
	char   *sp;
	char   *dp;
	int	flag;
	enum { DN_FLAG, VAL_FLAG } tflag;

	size = 0;
	bv->bv_val[0] = '\0';
	bv->bv_len--; /* leave space for lone $ */

	flag = 0;
	tflag = DN_FLAG;
	for ( dp = bv->bv_val, sp = pat->bv_val; size < bv->bv_len &&
		sp < pat->bv_val + pat->bv_len ; sp++ )
	{
		/* did we previously see a $ */
		if ( flag ) {
			if ( flag == 1 && *sp == '$' ) {
				*dp++ = '$';
				size++;
				flag = 0;
				tflag = DN_FLAG;

			} else if ( flag == 2 && *sp == 'v' /*'}'*/) {
				tflag = VAL_FLAG;

			} else if ( flag == 2 && *sp == 'd' /*'}'*/) {
				tflag = DN_FLAG;

			} else if ( flag == 1 && *sp == '{' /*'}'*/) {
				flag = 2;

			} else if ( *sp >= '0' && *sp <= '9' ) {
				int	nm;
				regmatch_t *m;
				char *data;
				int	n;
				int	i;
				int	l;

				n = *sp - '0';

				if ( flag == 2 ) {
					for ( sp++; *sp != '\0' && *sp != /*'{'*/ '}'; sp++ ) {
						if ( *sp >= '0' && *sp <= '9' ) {
							n = 10*n + ( *sp - '0' );
						}
					}

					if ( *sp != /*'{'*/ '}' ) {
						/* FIXME: error */
						return 1;
					}
				}

				switch (tflag) {
				case DN_FLAG:
					nm = matches->dn_count;
					m = matches->dn_data;
					data = dn_matches ? dn_matches->bv_val : NULL;
					break;
				case VAL_FLAG:
					nm = matches->val_count;
					m = matches->val_data;
					data = val_matches ? val_matches->bv_val : NULL;
					break;
				default:
					assert( 0 );
				}
				if ( n >= nm ) {
					/* FIXME: error */
					return 1;
				}
				if ( data == NULL ) {
					/* FIXME: error */
					return 1;
				}
				
				*dp = '\0';
				i = m[n].rm_so;
				l = m[n].rm_eo; 
					
				for ( ; size < bv->bv_len && i < l; size++, i++ ) {
					*dp++ = data[i];
				}
				*dp = '\0';

				flag = 0;
				tflag = DN_FLAG;
			}
		} else {
			if (*sp == '$') {
				flag = 1;
			} else {
				*dp++ = *sp;
				size++;
			}
		}
	}

	if ( flag ) {
		/* must have ended with a single $ */
		*dp++ = '$';
		size++;
	}

	*dp = '\0';
	bv->bv_len = size;

	Debug( LDAP_DEBUG_ACL, "=> acl_string_expand: pattern:  %.*s\n", (int)pat->bv_len, pat->bv_val, 0 );
	Debug( LDAP_DEBUG_ACL, "=> acl_string_expand: expanded: %s\n", bv->bv_val, 0, 0 );

	return 0;
}

static int
regex_matches(
	struct berval	*pat,		/* pattern to expand and match against */
	char		*str,		/* string to match against pattern */
	struct berval	*dn_matches,	/* buffer with $N expansion variables from DN */
	struct berval	*val_matches,	/* buffer with $N expansion variables from val */
	AclRegexMatches	*matches	/* offsets in buffer for $N expansion variables */
)
{
	regex_t re;
	char newbuf[ACL_BUF_SIZE];
	struct berval bv;
	int	rc;

	bv.bv_len = sizeof( newbuf ) - 1;
	bv.bv_val = newbuf;

	if (str == NULL) {
		str = "";
	};

	acl_string_expand( &bv, pat, dn_matches, val_matches, matches );
	rc = regcomp( &re, newbuf, REG_EXTENDED|REG_ICASE );
	if ( rc ) {
		char error[ACL_BUF_SIZE];
		regerror( rc, &re, error, sizeof( error ) );

		Debug( LDAP_DEBUG_TRACE,
		    "compile( \"%s\", \"%s\") failed %s\n",
			pat->bv_val, str, error );
		return( 0 );
	}

	rc = regexec( &re, str, 0, NULL, 0 );
	regfree( &re );

	Debug( LDAP_DEBUG_TRACE,
	    "=> regex_matches: string:	 %s\n", str, 0, 0 );
	Debug( LDAP_DEBUG_TRACE,
	    "=> regex_matches: rc: %d %s\n",
		rc, !rc ? "matches" : "no matches", 0 );
	return( !rc );
}

