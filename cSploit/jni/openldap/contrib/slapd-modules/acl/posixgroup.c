/* posixgroup.c */
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

#include <portable.h>

#include <ac/string.h>
#include <slap.h>
#include <lutil.h>

/* Need dynacl... */

#ifdef SLAP_DYNACL

typedef struct pg_t {
	slap_style_t		pg_style;
	struct berval		pg_pat;
} pg_t;

static ObjectClass		*pg_posixGroup;
static AttributeDescription	*pg_memberUid;
static ObjectClass		*pg_posixAccount;
static AttributeDescription	*pg_uidNumber;

static int pg_dynacl_destroy( void *priv );

static int
pg_dynacl_parse(
	const char	*fname,
	int 		lineno,
	const char	*opts,
	slap_style_t	style,
	const char	*pattern,
	void		**privp )
{
	pg_t		*pg;
	int		rc;
	const char	*text = NULL;
	struct berval	pat;

	ber_str2bv( pattern, 0, 0, &pat );

	pg = ch_calloc( 1, sizeof( pg_t ) );

	pg->pg_style = style;

	switch ( pg->pg_style ) {
	case ACL_STYLE_BASE:
		rc = dnNormalize( 0, NULL, NULL, &pat, &pg->pg_pat, NULL );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s line %d: posixGroup ACL: "
				"unable to normalize DN \"%s\".\n",
				fname, lineno, pattern );
			goto cleanup;
		}
		break;

	case ACL_STYLE_EXPAND:
		ber_dupbv( &pg->pg_pat, &pat );
		break;

	default:
		fprintf( stderr, "%s line %d: posixGroup ACL: "
			"unsupported style \"%s\".\n",
			fname, lineno, style_strings[ pg->pg_style ] );
		goto cleanup;
	}

	/* TODO: use opts to allow the use of different
	 * group objects and member attributes */
	if ( pg_posixGroup == NULL ) {
		pg_posixGroup = oc_find( "posixGroup" );
		if ( pg_posixGroup == NULL ) {
			fprintf( stderr, "%s line %d: posixGroup ACL: "
				"unable to lookup \"posixGroup\" "
				"objectClass.\n",
				fname, lineno );
			goto cleanup;
		}

		pg_posixAccount = oc_find( "posixAccount" );
		if ( pg_posixGroup == NULL ) {
			fprintf( stderr, "%s line %d: posixGroup ACL: "
				"unable to lookup \"posixAccount\" "
				"objectClass.\n",
				fname, lineno );
			goto cleanup;
		}

		rc = slap_str2ad( "memberUid", &pg_memberUid, &text );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s line %d: posixGroup ACL: "
				"unable to lookup \"memberUid\" "
				"attributeDescription (%d: %s).\n",
				fname, lineno, rc, text );
			goto cleanup;
		}

		rc = slap_str2ad( "uidNumber", &pg_uidNumber, &text );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s line %d: posixGroup ACL: "
				"unable to lookup \"uidNumber\" "
				"attributeDescription (%d: %s).\n",
				fname, lineno, rc, text );
			goto cleanup;
		}
	}

	*privp = (void *)pg;
	return 0;

cleanup:
	(void)pg_dynacl_destroy( (void *)pg );

	return 1;
}

static int
pg_dynacl_unparse(
	void		*priv,
	struct berval	*bv )
{
	pg_t		*pg = (pg_t *)priv;
	char		*ptr;

	bv->bv_len = STRLENOF( " dynacl/posixGroup.expand=" ) + pg->pg_pat.bv_len;
	bv->bv_val = ch_malloc( bv->bv_len + 1 );

	ptr = lutil_strcopy( bv->bv_val, " dynacl/posixGroup" );

	switch ( pg->pg_style ) {
	case ACL_STYLE_BASE:
		ptr = lutil_strcopy( ptr, ".exact=" );
		break;

	case ACL_STYLE_EXPAND:
		ptr = lutil_strcopy( ptr, ".expand=" );
		break;

	default:
		assert( 0 );
	}

	ptr = lutil_strncopy( ptr, pg->pg_pat.bv_val, pg->pg_pat.bv_len );
	ptr[ 0 ] = '\0';

	bv->bv_len = ptr - bv->bv_val;

	return 0;
}

static int
pg_dynacl_mask(
	void			*priv,
	Operation		*op,
	Entry			*target,
	AttributeDescription	*desc,
	struct berval		*val,
	int			nmatch,
	regmatch_t		*matches,
	slap_access_t		*grant,
	slap_access_t		*deny )
{
	pg_t		*pg = (pg_t *)priv;
	Entry		*group = NULL,
			*user = NULL;
	int		rc;
	Backend		*be = op->o_bd,
			*group_be = NULL,
			*user_be = NULL;
	struct berval	group_ndn;

	ACL_INVALIDATE( *deny );

	/* get user */
	if ( target && dn_match( &target->e_nname, &op->o_ndn ) ) {
		user = target;
		rc = LDAP_SUCCESS;

	} else {
		user_be = op->o_bd = select_backend( &op->o_ndn, 0 );
		if ( op->o_bd == NULL ) {
			op->o_bd = be;
			return 0;
		}
		rc = be_entry_get_rw( op, &op->o_ndn, pg_posixAccount, pg_uidNumber, 0, &user );
	}

	if ( rc != LDAP_SUCCESS || user == NULL ) {
		op->o_bd = be;
		return 0;
	}

	/* get target */
	if ( pg->pg_style == ACL_STYLE_EXPAND ) {
		char		buf[ 1024 ];
		struct berval	bv;
		AclRegexMatches amatches = { 0 };

		amatches.dn_count = nmatch;
		AC_MEMCPY( amatches.dn_data, matches, sizeof( amatches.dn_data ) );

		bv.bv_len = sizeof( buf ) - 1;
		bv.bv_val = buf;

		if ( acl_string_expand( &bv, &pg->pg_pat,
				&target->e_nname,
				NULL, &amatches ) )
		{
			goto cleanup;
		}

		if ( dnNormalize( 0, NULL, NULL, &bv, &group_ndn,
				op->o_tmpmemctx ) != LDAP_SUCCESS )
		{
			/* did not expand to a valid dn */
			goto cleanup;
		}
		
	} else {
		group_ndn = pg->pg_pat;
	}

	if ( target && dn_match( &target->e_nname, &group_ndn ) ) {
		group = target;
		rc = LDAP_SUCCESS;

	} else {
		group_be = op->o_bd = select_backend( &group_ndn, 0 );
		if ( op->o_bd == NULL ) {
			goto cleanup;
		}
		rc = be_entry_get_rw( op, &group_ndn, pg_posixGroup, pg_memberUid, 0, &group );
	}

	if ( group_ndn.bv_val != pg->pg_pat.bv_val ) {
		op->o_tmpfree( group_ndn.bv_val, op->o_tmpmemctx );
	}

	if ( rc == LDAP_SUCCESS && group != NULL ) {
		Attribute	*a_uid,
				*a_member;

		a_uid = attr_find( user->e_attrs, pg_uidNumber );
		if ( !a_uid || !BER_BVISNULL( &a_uid->a_nvals[ 1 ] ) ) {
			rc = LDAP_NO_SUCH_ATTRIBUTE;

		} else {
			a_member = attr_find( group->e_attrs, pg_memberUid );
			if ( !a_member ) {
				rc = LDAP_NO_SUCH_ATTRIBUTE;

			} else {
				rc = value_find_ex( pg_memberUid,
					SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
					SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
					a_member->a_nvals, &a_uid->a_nvals[ 0 ],
					op->o_tmpmemctx );
			}
		}

	} else {
		rc = LDAP_NO_SUCH_OBJECT;
	}


	if ( rc == LDAP_SUCCESS ) {
		ACL_LVL_ASSIGN_WRITE( *grant );
	}

cleanup:;
	if ( group != NULL && group != target ) {
		op->o_bd = group_be;
		be_entry_release_r( op, group );
		op->o_bd = be;
	}

	if ( user != NULL && user != target ) {
		op->o_bd = user_be;
		be_entry_release_r( op, user );
		op->o_bd = be;
	}

	return 0;
}

static int
pg_dynacl_destroy(
	void		*priv )
{
	pg_t		*pg = (pg_t *)priv;

	if ( pg != NULL ) {
		if ( !BER_BVISNULL( &pg->pg_pat ) ) {
			ber_memfree( pg->pg_pat.bv_val );
		}
		ch_free( pg );
	}

	return 0;
}

static struct slap_dynacl_t pg_dynacl = {
	"posixGroup",
	pg_dynacl_parse,
	pg_dynacl_unparse,
	pg_dynacl_mask,
	pg_dynacl_destroy
};

int
init_module( int argc, char *argv[] )
{
	return slap_dynacl_register( &pg_dynacl );
}

#endif /* SLAP_DYNACL */
