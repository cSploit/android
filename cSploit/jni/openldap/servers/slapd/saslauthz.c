/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 2000 Mark Adamson, Carnegie Mellon.
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
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/ctype.h>

#include "slap.h"

#include "lutil.h"

#define SASLREGEX_REPLACE 10

#define LDAP_X_SCOPE_EXACT	((ber_int_t) 0x0010)
#define LDAP_X_SCOPE_REGEX	((ber_int_t) 0x0020)
#define LDAP_X_SCOPE_CHILDREN	((ber_int_t) 0x0030)
#define LDAP_X_SCOPE_SUBTREE	((ber_int_t) 0x0040)
#define LDAP_X_SCOPE_ONELEVEL	((ber_int_t) 0x0050)
#define LDAP_X_SCOPE_GROUP	((ber_int_t) 0x0060)
#define LDAP_X_SCOPE_USERS	((ber_int_t) 0x0070)

/*
 * IDs in DNauthzid form can now have a type specifier, that
 * influences how they are used in related operations.
 *
 * syntax: dn[.{exact|regex}]:<val>
 *
 * dn.exact:	the value must pass normalization and is used 
 *		in exact DN match.
 * dn.regex:	the value is treated as a regular expression 
 *		in matching DN values in authz{To|From}
 *		attributes.
 * dn:		for backwards compatibility reasons, the value 
 *		is treated as a regular expression, and thus 
 *		it is not normalized nor validated; it is used
 *		in exact or regex comparisons based on the 
 *		context.
 *
 * IDs in DNauthzid form can now have a type specifier, that
 * influences how they are used in related operations.
 *
 * syntax: u[.mech[/realm]]:<val>
 * 
 * where mech is a SIMPLE, AUTHZ, or a SASL mechanism name
 * and realm is mechanism specific realm (separate to those
 * which are representable as part of the principal).
 */

typedef struct sasl_regexp {
  char *sr_match;						/* regexp match pattern */
  char *sr_replace; 					/* regexp replace pattern */
  regex_t sr_workspace;					/* workspace for regexp engine */
  int sr_offset[SASLREGEX_REPLACE+2];	/* offsets of $1,$2... in *replace */
} SaslRegexp_t;

static int nSaslRegexp = 0;
static SaslRegexp_t *SaslRegexp = NULL;

#ifdef SLAP_AUTH_REWRITE
#include "rewrite.h"
struct rewrite_info	*sasl_rwinfo = NULL;
#define AUTHID_CONTEXT	"authid"
#endif /* SLAP_AUTH_REWRITE */

/* What SASL proxy authorization policies are allowed? */
#define	SASL_AUTHZ_NONE	0x00
#define	SASL_AUTHZ_FROM	0x01
#define	SASL_AUTHZ_TO	0x02
#define SASL_AUTHZ_AND	0x10

static const char *policy_txt[] = {
	"none", "from", "to", "any"
};

static int authz_policy = SASL_AUTHZ_NONE;

static int
slap_sasl_match( Operation *opx, struct berval *rule,
	struct berval *assertDN, struct berval *authc );

int slap_sasl_setpolicy( const char *arg )
{
	int rc = LDAP_SUCCESS;

	if ( strcasecmp( arg, "none" ) == 0 ) {
		authz_policy = SASL_AUTHZ_NONE;
	} else if ( strcasecmp( arg, "from" ) == 0 ) {
		authz_policy = SASL_AUTHZ_FROM;
	} else if ( strcasecmp( arg, "to" ) == 0 ) {
		authz_policy = SASL_AUTHZ_TO;
	} else if ( strcasecmp( arg, "both" ) == 0 || strcasecmp( arg, "any" ) == 0 ) {
		authz_policy = SASL_AUTHZ_FROM | SASL_AUTHZ_TO;
	} else if ( strcasecmp( arg, "all" ) == 0 ) {
		authz_policy = SASL_AUTHZ_FROM | SASL_AUTHZ_TO | SASL_AUTHZ_AND;
	} else {
		rc = LDAP_OTHER;
	}
	return rc;
}

const char * slap_sasl_getpolicy()
{
	if ( authz_policy == (SASL_AUTHZ_FROM | SASL_AUTHZ_TO | SASL_AUTHZ_AND) )
		return "all";
	else
		return policy_txt[authz_policy];
}

int slap_parse_user( struct berval *id, struct berval *user,
		struct berval *realm, struct berval *mech )
{
	char	u;
	
	assert( id != NULL );
	assert( !BER_BVISNULL( id ) );
	assert( user != NULL );
	assert( realm != NULL );
	assert( mech != NULL );

	u = id->bv_val[ 0 ];
	
	if ( u != 'u' && u != 'U' ) {
		/* called with something other than u: */
		return LDAP_PROTOCOL_ERROR;
	}

	/* uauthzid form:
	 *		u[.mech[/realm]]:user
	 */
	
	user->bv_val = ber_bvchr( id, ':' );
	if ( BER_BVISNULL( user ) ) {
		return LDAP_PROTOCOL_ERROR;
	}
	user->bv_val[ 0 ] = '\0';
	user->bv_val++;
	user->bv_len = id->bv_len - ( user->bv_val - id->bv_val );

	mech->bv_val = ber_bvchr( id, '.' );
	if ( !BER_BVISNULL( mech ) ) {
		mech->bv_val[ 0 ] = '\0';
		mech->bv_val++;
		mech->bv_len = user->bv_val - mech->bv_val - 1;

		realm->bv_val = ber_bvchr( mech, '/' );

		if ( !BER_BVISNULL( realm ) ) {
			realm->bv_val[ 0 ] = '\0';
			realm->bv_val++;
			mech->bv_len = realm->bv_val - mech->bv_val - 1;
			realm->bv_len = user->bv_val - realm->bv_val - 1;
		}

	} else {
		BER_BVZERO( realm );
	}

	if ( id->bv_val[ 1 ] != '\0' ) {
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( mech ) ) {
		assert( mech->bv_val == id->bv_val + 2 );

		AC_MEMCPY( mech->bv_val - 2, mech->bv_val, mech->bv_len + 1 );
		mech->bv_val -= 2;
	}

	if ( !BER_BVISNULL( realm ) ) {
		assert( realm->bv_val >= id->bv_val + 2 );

		AC_MEMCPY( realm->bv_val - 2, realm->bv_val, realm->bv_len + 1 );
		realm->bv_val -= 2;
	}

	/* leave "u:" before user */
	user->bv_val -= 2;
	user->bv_len += 2;
	user->bv_val[ 0 ] = u;
	user->bv_val[ 1 ] = ':';

	return LDAP_SUCCESS;
}

int
authzValidate(
	Syntax *syntax,
	struct berval *in )
{
	struct berval	bv;
	int		rc = LDAP_INVALID_SYNTAX;
	LDAPURLDesc	*ludp = NULL;
	int		scope = -1;

	/*
	 * 1) <DN>
	 * 2) dn[.{exact|children|subtree|onelevel}]:{*|<DN>}
	 * 3) dn.regex:<pattern>
	 * 4) u[.mech[/realm]]:<ID>
	 * 5) group[/<groupClass>[/<memberAttr>]]:<DN>
	 * 6) <URL>
	 */

	assert( in != NULL );
	assert( !BER_BVISNULL( in ) );

	Debug( LDAP_DEBUG_TRACE,
		"authzValidate: parsing %s\n", in->bv_val, 0, 0 );

	/*
	 * 2) dn[.{exact|children|subtree|onelevel}]:{*|<DN>}
	 * 3) dn.regex:<pattern>
	 *
	 * <DN> must pass DN normalization
	 */
	if ( !strncasecmp( in->bv_val, "dn", STRLENOF( "dn" ) ) ) {
		bv.bv_val = in->bv_val + STRLENOF( "dn" );

		if ( bv.bv_val[ 0 ] == '.' ) {
			bv.bv_val++;

			if ( !strncasecmp( bv.bv_val, "exact:", STRLENOF( "exact:" ) ) ) {
				bv.bv_val += STRLENOF( "exact:" );
				scope = LDAP_X_SCOPE_EXACT;

			} else if ( !strncasecmp( bv.bv_val, "regex:", STRLENOF( "regex:" ) ) ) {
				bv.bv_val += STRLENOF( "regex:" );
				scope = LDAP_X_SCOPE_REGEX;

			} else if ( !strncasecmp( bv.bv_val, "children:", STRLENOF( "children:" ) ) ) {
				bv.bv_val += STRLENOF( "children:" );
				scope = LDAP_X_SCOPE_CHILDREN;

			} else if ( !strncasecmp( bv.bv_val, "subtree:", STRLENOF( "subtree:" ) ) ) {
				bv.bv_val += STRLENOF( "subtree:" );
				scope = LDAP_X_SCOPE_SUBTREE;

			} else if ( !strncasecmp( bv.bv_val, "onelevel:", STRLENOF( "onelevel:" ) ) ) {
				bv.bv_val += STRLENOF( "onelevel:" );
				scope = LDAP_X_SCOPE_ONELEVEL;

			} else {
				return LDAP_INVALID_SYNTAX;
			}

		} else {
			if ( bv.bv_val[ 0 ] != ':' ) {
				return LDAP_INVALID_SYNTAX;
			}
			scope = LDAP_X_SCOPE_EXACT;
			bv.bv_val++;
		}

		bv.bv_val += strspn( bv.bv_val, " " );
		/* jump here in case no type specification was present
		 * and uri was not an URI... HEADS-UP: assuming EXACT */
is_dn:		bv.bv_len = in->bv_len - ( bv.bv_val - in->bv_val );

		/* a single '*' means any DN without using regexes */
		if ( ber_bvccmp( &bv, '*' ) ) {
			/* LDAP_X_SCOPE_USERS */
			return LDAP_SUCCESS;
		}

		switch ( scope ) {
		case LDAP_X_SCOPE_EXACT:
		case LDAP_X_SCOPE_CHILDREN:
		case LDAP_X_SCOPE_SUBTREE:
		case LDAP_X_SCOPE_ONELEVEL:
			return dnValidate( NULL, &bv );

		case LDAP_X_SCOPE_REGEX:
			return LDAP_SUCCESS;
		}

		return rc;

	/*
	 * 4) u[.mech[/realm]]:<ID>
	 */
	} else if ( ( in->bv_val[ 0 ] == 'u' || in->bv_val[ 0 ] == 'U' )
			&& ( in->bv_val[ 1 ] == ':' 
				|| in->bv_val[ 1 ] == '/' 
				|| in->bv_val[ 1 ] == '.' ) )
	{
		char		buf[ SLAP_LDAPDN_MAXLEN ];
		struct berval	id,
				user = BER_BVNULL,
				realm = BER_BVNULL,
				mech = BER_BVNULL;

		if ( sizeof( buf ) <= in->bv_len ) {
			return LDAP_INVALID_SYNTAX;
		}

		id.bv_len = in->bv_len;
		id.bv_val = buf;
		strncpy( buf, in->bv_val, sizeof( buf ) );

		rc = slap_parse_user( &id, &user, &realm, &mech );
		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}

		return rc;

	/*
	 * 5) group[/groupClass[/memberAttr]]:<DN>
	 *
	 * <groupClass> defaults to "groupOfNames"
	 * <memberAttr> defaults to "member"
	 * 
	 * <DN> must pass DN normalization
	 */
	} else if ( strncasecmp( in->bv_val, "group", STRLENOF( "group" ) ) == 0 )
	{
		struct berval	group_dn = BER_BVNULL,
				group_oc = BER_BVNULL,
				member_at = BER_BVNULL;

		bv.bv_val = in->bv_val + STRLENOF( "group" );
		bv.bv_len = in->bv_len - STRLENOF( "group" );
		group_dn.bv_val = ber_bvchr( &bv, ':' );
		if ( group_dn.bv_val == NULL ) {
			/* last chance: assume it's a(n exact) DN ... */
			bv.bv_val = in->bv_val;
			scope = LDAP_X_SCOPE_EXACT;
			goto is_dn;
		}
		
		/*
		 * FIXME: we assume that "member" and "groupOfNames"
		 * are present in schema...
		 */
		if ( bv.bv_val[ 0 ] == '/' ) {
			group_oc.bv_val = &bv.bv_val[ 1 ];
			group_oc.bv_len = group_dn.bv_val - group_oc.bv_val;

			member_at.bv_val = ber_bvchr( &group_oc, '/' );
			if ( member_at.bv_val ) {
				AttributeDescription	*ad = NULL;
				const char		*text = NULL;

				group_oc.bv_len = member_at.bv_val - group_oc.bv_val;
				member_at.bv_val++;
				member_at.bv_len = group_dn.bv_val - member_at.bv_val;
				rc = slap_bv2ad( &member_at, &ad, &text );
				if ( rc != LDAP_SUCCESS ) {
					return rc;
				}
			}

			if ( oc_bvfind( &group_oc ) == NULL ) {
				return LDAP_INVALID_SYNTAX;
			}
		}

		group_dn.bv_val++;
		group_dn.bv_len = in->bv_len - ( group_dn.bv_val - in->bv_val );

		rc = dnValidate( NULL, &group_dn );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}

		return rc;
	}

	/*
	 * ldap:///<base>??<scope>?<filter>
	 * <scope> ::= {base|one|subtree}
	 *
	 * <scope> defaults to "base"
	 * <base> must pass DN normalization
	 * <filter> must pass str2filter()
	 */
	rc = ldap_url_parse( in->bv_val, &ludp );
	switch ( rc ) {
	case LDAP_URL_SUCCESS:
		/* FIXME: the check is pedantic, but I think it's necessary,
		 * because people tend to use things like ldaps:// which
		 * gives the idea SSL is being used.  Maybe we could
		 * accept ldapi:// as well, but the point is that we use
		 * an URL as an easy means to define bits of a search with
		 * little parsing.
		 */
		if ( strcasecmp( ludp->lud_scheme, "ldap" ) != 0 ) {
			/*
			 * must be ldap:///
			 */
			rc = LDAP_INVALID_SYNTAX;
			goto done;
		}
		break;

	case LDAP_URL_ERR_BADSCHEME:
		/*
		 * last chance: assume it's a(n exact) DN ...
		 *
		 * NOTE: must pass DN normalization
		 */
		ldap_free_urldesc( ludp );
		bv.bv_val = in->bv_val;
		scope = LDAP_X_SCOPE_EXACT;
		goto is_dn;

	default:
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}

	if ( ( ludp->lud_host && *ludp->lud_host )
		|| ludp->lud_attrs || ludp->lud_exts )
	{
		/* host part must be empty */
		/* attrs and extensions parts must be empty */
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}

	/* Grab the filter */
	if ( ludp->lud_filter ) {
		Filter	*f = str2filter( ludp->lud_filter );
		if ( f == NULL ) {
			rc = LDAP_INVALID_SYNTAX;
			goto done;
		}
		filter_free( f );
	}

	/* Grab the searchbase */
	assert( ludp->lud_dn != NULL );
	ber_str2bv( ludp->lud_dn, 0, 0, &bv );
	rc = dnValidate( NULL, &bv );

done:
	ldap_free_urldesc( ludp );
	return( rc );
}

static int
authzPrettyNormal(
	struct berval	*val,
	struct berval	*normalized,
	void		*ctx,
	int		normalize )
{
	struct berval	bv;
	int		rc = LDAP_INVALID_SYNTAX;
	LDAPURLDesc	*ludp = NULL;
	char		*lud_dn = NULL,
			*lud_filter = NULL;
	int		scope = -1;

	/*
	 * 1) <DN>
	 * 2) dn[.{exact|children|subtree|onelevel}]:{*|<DN>}
	 * 3) dn.regex:<pattern>
	 * 4) u[.mech[/realm]]:<ID>
	 * 5) group[/<groupClass>[/<memberAttr>]]:<DN>
	 * 6) <URL>
	 */

	assert( val != NULL );
	assert( !BER_BVISNULL( val ) );

	/*
	 * 2) dn[.{exact|children|subtree|onelevel}]:{*|<DN>}
	 * 3) dn.regex:<pattern>
	 *
	 * <DN> must pass DN normalization
	 */
	if ( !strncasecmp( val->bv_val, "dn", STRLENOF( "dn" ) ) ) {
		struct berval	out = BER_BVNULL,
				prefix = BER_BVNULL;
		char		*ptr;

		bv.bv_val = val->bv_val + STRLENOF( "dn" );

		if ( bv.bv_val[ 0 ] == '.' ) {
			bv.bv_val++;

			if ( !strncasecmp( bv.bv_val, "exact:", STRLENOF( "exact:" ) ) ) {
				bv.bv_val += STRLENOF( "exact:" );
				scope = LDAP_X_SCOPE_EXACT;

			} else if ( !strncasecmp( bv.bv_val, "regex:", STRLENOF( "regex:" ) ) ) {
				bv.bv_val += STRLENOF( "regex:" );
				scope = LDAP_X_SCOPE_REGEX;

			} else if ( !strncasecmp( bv.bv_val, "children:", STRLENOF( "children:" ) ) ) {
				bv.bv_val += STRLENOF( "children:" );
				scope = LDAP_X_SCOPE_CHILDREN;

			} else if ( !strncasecmp( bv.bv_val, "subtree:", STRLENOF( "subtree:" ) ) ) {
				bv.bv_val += STRLENOF( "subtree:" );
				scope = LDAP_X_SCOPE_SUBTREE;

			} else if ( !strncasecmp( bv.bv_val, "onelevel:", STRLENOF( "onelevel:" ) ) ) {
				bv.bv_val += STRLENOF( "onelevel:" );
				scope = LDAP_X_SCOPE_ONELEVEL;

			} else {
				return LDAP_INVALID_SYNTAX;
			}

		} else {
			if ( bv.bv_val[ 0 ] != ':' ) {
				return LDAP_INVALID_SYNTAX;
			}
			scope = LDAP_X_SCOPE_EXACT;
			bv.bv_val++;
		}

		bv.bv_val += strspn( bv.bv_val, " " );
		/* jump here in case no type specification was present
		 * and uri was not an URI... HEADS-UP: assuming EXACT */
is_dn:		bv.bv_len = val->bv_len - ( bv.bv_val - val->bv_val );

		/* a single '*' means any DN without using regexes */
		if ( ber_bvccmp( &bv, '*' ) ) {
			ber_str2bv_x( "dn:*", STRLENOF( "dn:*" ), 1, normalized, ctx );
			return LDAP_SUCCESS;
		}

		switch ( scope ) {
		case LDAP_X_SCOPE_EXACT:
		case LDAP_X_SCOPE_CHILDREN:
		case LDAP_X_SCOPE_SUBTREE:
		case LDAP_X_SCOPE_ONELEVEL:
			if ( normalize ) {
				rc = dnNormalize( 0, NULL, NULL, &bv, &out, ctx );
			} else {
				rc = dnPretty( NULL, &bv, &out, ctx );
			}
			if( rc != LDAP_SUCCESS ) {
				return LDAP_INVALID_SYNTAX;
			}
			break;

		case LDAP_X_SCOPE_REGEX:
			normalized->bv_len = STRLENOF( "dn.regex:" ) + bv.bv_len;
			normalized->bv_val = ber_memalloc_x( normalized->bv_len + 1, ctx );
			ptr = lutil_strcopy( normalized->bv_val, "dn.regex:" );
			ptr = lutil_strncopy( ptr, bv.bv_val, bv.bv_len );
			ptr[ 0 ] = '\0';
			return LDAP_SUCCESS;

		default:
			return LDAP_INVALID_SYNTAX;
		}

		/* prepare prefix */
		switch ( scope ) {
		case LDAP_X_SCOPE_EXACT:
			BER_BVSTR( &prefix, "dn:" );
			break;

		case LDAP_X_SCOPE_CHILDREN:
			BER_BVSTR( &prefix, "dn.children:" );
			break;

		case LDAP_X_SCOPE_SUBTREE:
			BER_BVSTR( &prefix, "dn.subtree:" );
			break;

		case LDAP_X_SCOPE_ONELEVEL:
			BER_BVSTR( &prefix, "dn.onelevel:" );
			break;

		default:
			assert( 0 );
			break;
		}

		normalized->bv_len = prefix.bv_len + out.bv_len;
		normalized->bv_val = ber_memalloc_x( normalized->bv_len + 1, ctx );
		
		ptr = lutil_strcopy( normalized->bv_val, prefix.bv_val );
		ptr = lutil_strncopy( ptr, out.bv_val, out.bv_len );
		ptr[ 0 ] = '\0';
		ber_memfree_x( out.bv_val, ctx );

		return LDAP_SUCCESS;

	/*
	 * 4) u[.mech[/realm]]:<ID>
	 */
	} else if ( ( val->bv_val[ 0 ] == 'u' || val->bv_val[ 0 ] == 'U' )
			&& ( val->bv_val[ 1 ] == ':' 
				|| val->bv_val[ 1 ] == '/' 
				|| val->bv_val[ 1 ] == '.' ) )
	{
		char		buf[ SLAP_LDAPDN_MAXLEN ];
		struct berval	id,
				user = BER_BVNULL,
				realm = BER_BVNULL,
				mech = BER_BVNULL;

		if ( sizeof( buf ) <= val->bv_len ) {
			return LDAP_INVALID_SYNTAX;
		}

		id.bv_len = val->bv_len;
		id.bv_val = buf;
		strncpy( buf, val->bv_val, sizeof( buf ) );

		rc = slap_parse_user( &id, &user, &realm, &mech );
		if ( rc != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}

		ber_dupbv_x( normalized, val, ctx );

		return rc;

	/*
	 * 5) group[/groupClass[/memberAttr]]:<DN>
	 *
	 * <groupClass> defaults to "groupOfNames"
	 * <memberAttr> defaults to "member"
	 * 
	 * <DN> must pass DN normalization
	 */
	} else if ( strncasecmp( val->bv_val, "group", STRLENOF( "group" ) ) == 0 )
	{
		struct berval	group_dn = BER_BVNULL,
				group_oc = BER_BVNULL,
				member_at = BER_BVNULL,
				out = BER_BVNULL;
		char		*ptr;

		bv.bv_val = val->bv_val + STRLENOF( "group" );
		bv.bv_len = val->bv_len - STRLENOF( "group" );
		group_dn.bv_val = ber_bvchr( &bv, ':' );
		if ( group_dn.bv_val == NULL ) {
			/* last chance: assume it's a(n exact) DN ... */
			bv.bv_val = val->bv_val;
			scope = LDAP_X_SCOPE_EXACT;
			goto is_dn;
		}

		/*
		 * FIXME: we assume that "member" and "groupOfNames"
		 * are present in schema...
		 */
		if ( bv.bv_val[ 0 ] == '/' ) {
			ObjectClass		*oc = NULL;

			group_oc.bv_val = &bv.bv_val[ 1 ];
			group_oc.bv_len = group_dn.bv_val - group_oc.bv_val;

			member_at.bv_val = ber_bvchr( &group_oc, '/' );
			if ( member_at.bv_val ) {
				AttributeDescription	*ad = NULL;
				const char		*text = NULL;

				group_oc.bv_len = member_at.bv_val - group_oc.bv_val;
				member_at.bv_val++;
				member_at.bv_len = group_dn.bv_val - member_at.bv_val;
				rc = slap_bv2ad( &member_at, &ad, &text );
				if ( rc != LDAP_SUCCESS ) {
					return rc;
				}

				member_at = ad->ad_cname;

			}

			oc = oc_bvfind( &group_oc );
			if ( oc == NULL ) {
				return LDAP_INVALID_SYNTAX;
			}

			group_oc = oc->soc_cname;
		}

		group_dn.bv_val++;
		group_dn.bv_len = val->bv_len - ( group_dn.bv_val - val->bv_val );

		if ( normalize ) {
			rc = dnNormalize( 0, NULL, NULL, &group_dn, &out, ctx );
		} else {
			rc = dnPretty( NULL, &group_dn, &out, ctx );
		}
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}

		normalized->bv_len = STRLENOF( "group" ":" ) + out.bv_len;
		if ( !BER_BVISNULL( &group_oc ) ) {
			normalized->bv_len += STRLENOF( "/" ) + group_oc.bv_len;
			if ( !BER_BVISNULL( &member_at ) ) {
				normalized->bv_len += STRLENOF( "/" ) + member_at.bv_len;
			}
		}

		normalized->bv_val = ber_memalloc_x( normalized->bv_len + 1, ctx );
		ptr = lutil_strcopy( normalized->bv_val, "group" );
		if ( !BER_BVISNULL( &group_oc ) ) {
			ptr[ 0 ] = '/';
			ptr++;
			ptr = lutil_strncopy( ptr, group_oc.bv_val, group_oc.bv_len );
			if ( !BER_BVISNULL( &member_at ) ) {
				ptr[ 0 ] = '/';
				ptr++;
				ptr = lutil_strncopy( ptr, member_at.bv_val, member_at.bv_len );
			}
		}
		ptr[ 0 ] = ':';
		ptr++;
		ptr = lutil_strncopy( ptr, out.bv_val, out.bv_len );
		ptr[ 0 ] = '\0';
		ber_memfree_x( out.bv_val, ctx );

		return rc;
	}

	/*
	 * ldap:///<base>??<scope>?<filter>
	 * <scope> ::= {base|one|subtree}
	 *
	 * <scope> defaults to "base"
	 * <base> must pass DN normalization
	 * <filter> must pass str2filter()
	 */
	rc = ldap_url_parse( val->bv_val, &ludp );
	switch ( rc ) {
	case LDAP_URL_SUCCESS:
		/* FIXME: the check is pedantic, but I think it's necessary,
		 * because people tend to use things like ldaps:// which
		 * gives the idea SSL is being used.  Maybe we could
		 * accept ldapi:// as well, but the point is that we use
		 * an URL as an easy means to define bits of a search with
		 * little parsing.
		 */
		if ( strcasecmp( ludp->lud_scheme, "ldap" ) != 0 ) {
			/*
			 * must be ldap:///
			 */
			rc = LDAP_INVALID_SYNTAX;
			goto done;
		}

		AC_MEMCPY( ludp->lud_scheme, "ldap", STRLENOF( "ldap" ) );
		break;

	case LDAP_URL_ERR_BADSCHEME:
		/*
		 * last chance: assume it's a(n exact) DN ...
		 *
		 * NOTE: must pass DN normalization
		 */
		ldap_free_urldesc( ludp );
		bv.bv_val = val->bv_val;
		scope = LDAP_X_SCOPE_EXACT;
		goto is_dn;

	default:
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}

	if ( ( ludp->lud_host && *ludp->lud_host )
		|| ludp->lud_attrs || ludp->lud_exts )
	{
		/* host part must be empty */
		/* attrs and extensions parts must be empty */
		rc = LDAP_INVALID_SYNTAX;
		goto done;
	}

	/* Grab the filter */
	if ( ludp->lud_filter ) {
		struct berval	filterstr;
		Filter		*f;

		lud_filter = ludp->lud_filter;

		f = str2filter( lud_filter );
		if ( f == NULL ) {
			rc = LDAP_INVALID_SYNTAX;
			goto done;
		}
		filter2bv( f, &filterstr );
		filter_free( f );
		if ( BER_BVISNULL( &filterstr ) ) {
			rc = LDAP_INVALID_SYNTAX;
			goto done;
		}

		ludp->lud_filter = filterstr.bv_val;
	}

	/* Grab the searchbase */
	assert( ludp->lud_dn != NULL );
	if ( ludp->lud_dn ) {
		struct berval	out = BER_BVNULL;

		lud_dn = ludp->lud_dn;

		ber_str2bv( lud_dn, 0, 0, &bv );
		if ( normalize ) {
			rc = dnNormalize( 0, NULL, NULL, &bv, &out, ctx );
		} else {
			rc = dnPretty( NULL, &bv, &out, ctx );
		}

		if ( rc != LDAP_SUCCESS ) {
			goto done;
		}

		ludp->lud_dn = out.bv_val;
	}

	ludp->lud_port = 0;
	normalized->bv_val = ldap_url_desc2str( ludp );
	if ( normalized->bv_val ) {
		normalized->bv_len = strlen( normalized->bv_val );

	} else {
		rc = LDAP_INVALID_SYNTAX;
	}

done:
	if ( lud_filter ) {
		if ( ludp->lud_filter != lud_filter ) {
			ber_memfree( ludp->lud_filter );
		}
		ludp->lud_filter = lud_filter;
	}

	if ( lud_dn ) {
		if ( ludp->lud_dn != lud_dn ) {
			ber_memfree( ludp->lud_dn );
		}
		ludp->lud_dn = lud_dn;
	}

	ldap_free_urldesc( ludp );

	return( rc );
}

int
authzNormalize(
	slap_mask_t	usage,
	Syntax		*syntax,
	MatchingRule	*mr,
	struct berval	*val,
	struct berval	*normalized,
	void		*ctx )
{
	int		rc;

	Debug( LDAP_DEBUG_TRACE, ">>> authzNormalize: <%s>\n",
		val->bv_val, 0, 0 );

	rc = authzPrettyNormal( val, normalized, ctx, 1 );

	Debug( LDAP_DEBUG_TRACE, "<<< authzNormalize: <%s> (%d)\n",
		normalized->bv_val, rc, 0 );

	return rc;
}

int
authzPretty(
	Syntax *syntax,
	struct berval *val,
	struct berval *out,
	void *ctx)
{
	int		rc;

	Debug( LDAP_DEBUG_TRACE, ">>> authzPretty: <%s>\n",
		val->bv_val, 0, 0 );

	rc = authzPrettyNormal( val, out, ctx, 0 );

	Debug( LDAP_DEBUG_TRACE, "<<< authzPretty: <%s> (%d)\n",
		out->bv_val, rc, 0 );

	return rc;
}


static int
slap_parseURI(
	Operation	*op,
	struct berval	*uri,
	struct berval	*base,
	struct berval	*nbase,
	int		*scope,
	Filter		**filter,
	struct berval	*fstr,
	int		normalize )
{
	struct berval	bv;
	int		rc;
	LDAPURLDesc	*ludp;

	struct berval	idx;

	assert( uri != NULL && !BER_BVISNULL( uri ) );
	BER_BVZERO( base );
	BER_BVZERO( nbase );
	BER_BVZERO( fstr );
	*scope = -1;
	*filter = NULL;

	Debug( LDAP_DEBUG_TRACE,
		"slap_parseURI: parsing %s\n", uri->bv_val, 0, 0 );

	rc = LDAP_PROTOCOL_ERROR;

	idx = *uri;
	if ( idx.bv_val[ 0 ] == '{' ) {
		char	*ptr;

		ptr = ber_bvchr( &idx, '}' ) + 1;

		assert( ptr != (void *)1 );

		idx.bv_len -= ptr - idx.bv_val;
		idx.bv_val = ptr;
		uri = &idx;
	}

	/*
	 * dn[.<dnstyle>]:<dnpattern>
	 * <dnstyle> ::= {exact|regex|children|subtree|onelevel}
	 *
	 * <dnstyle> defaults to "exact"
	 * if <dnstyle> is not "regex", <dnpattern> must pass DN normalization
	 */
	if ( !strncasecmp( uri->bv_val, "dn", STRLENOF( "dn" ) ) ) {
		bv.bv_val = uri->bv_val + STRLENOF( "dn" );

		if ( bv.bv_val[ 0 ] == '.' ) {
			bv.bv_val++;

			if ( !strncasecmp( bv.bv_val, "exact:", STRLENOF( "exact:" ) ) ) {
				bv.bv_val += STRLENOF( "exact:" );
				*scope = LDAP_X_SCOPE_EXACT;

			} else if ( !strncasecmp( bv.bv_val, "regex:", STRLENOF( "regex:" ) ) ) {
				bv.bv_val += STRLENOF( "regex:" );
				*scope = LDAP_X_SCOPE_REGEX;

			} else if ( !strncasecmp( bv.bv_val, "children:", STRLENOF( "children:" ) ) ) {
				bv.bv_val += STRLENOF( "children:" );
				*scope = LDAP_X_SCOPE_CHILDREN;

			} else if ( !strncasecmp( bv.bv_val, "subtree:", STRLENOF( "subtree:" ) ) ) {
				bv.bv_val += STRLENOF( "subtree:" );
				*scope = LDAP_X_SCOPE_SUBTREE;

			} else if ( !strncasecmp( bv.bv_val, "onelevel:", STRLENOF( "onelevel:" ) ) ) {
				bv.bv_val += STRLENOF( "onelevel:" );
				*scope = LDAP_X_SCOPE_ONELEVEL;

			} else {
				return LDAP_PROTOCOL_ERROR;
			}

		} else {
			if ( bv.bv_val[ 0 ] != ':' ) {
				return LDAP_PROTOCOL_ERROR;
			}
			*scope = LDAP_X_SCOPE_EXACT;
			bv.bv_val++;
		}

		bv.bv_val += strspn( bv.bv_val, " " );
		/* jump here in case no type specification was present
		 * and uri was not an URI... HEADS-UP: assuming EXACT */
is_dn:		bv.bv_len = uri->bv_len - (bv.bv_val - uri->bv_val);

		/* a single '*' means any DN without using regexes */
		if ( ber_bvccmp( &bv, '*' ) ) {
			*scope = LDAP_X_SCOPE_USERS;
		}

		switch ( *scope ) {
		case LDAP_X_SCOPE_EXACT:
		case LDAP_X_SCOPE_CHILDREN:
		case LDAP_X_SCOPE_SUBTREE:
		case LDAP_X_SCOPE_ONELEVEL:
			if ( normalize ) {
				rc = dnNormalize( 0, NULL, NULL, &bv, nbase, op->o_tmpmemctx );
				if( rc != LDAP_SUCCESS ) {
					*scope = -1;
				}
			} else {
				ber_dupbv_x( nbase, &bv, op->o_tmpmemctx );
				rc = LDAP_SUCCESS;
			}
			break;

		case LDAP_X_SCOPE_REGEX:
			ber_dupbv_x( nbase, &bv, op->o_tmpmemctx );

		case LDAP_X_SCOPE_USERS:
			rc = LDAP_SUCCESS;
			break;

		default:
			*scope = -1;
			break;
		}

		return rc;

	/*
	 * u:<uid>
	 */
	} else if ( ( uri->bv_val[ 0 ] == 'u' || uri->bv_val[ 0 ] == 'U' )
			&& ( uri->bv_val[ 1 ] == ':' 
				|| uri->bv_val[ 1 ] == '/' 
				|| uri->bv_val[ 1 ] == '.' ) )
	{
		Connection	c = *op->o_conn;
		char		buf[ SLAP_LDAPDN_MAXLEN ];
		struct berval	id,
				user = BER_BVNULL,
				realm = BER_BVNULL,
				mech = BER_BVNULL;

		if ( sizeof( buf ) <= uri->bv_len ) {
			return LDAP_INVALID_SYNTAX;
		}

		id.bv_len = uri->bv_len;
		id.bv_val = buf;
		strncpy( buf, uri->bv_val, sizeof( buf ) );

		rc = slap_parse_user( &id, &user, &realm, &mech );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}

		if ( !BER_BVISNULL( &mech ) ) {
			c.c_sasl_bind_mech = mech;
		} else {
			BER_BVSTR( &c.c_sasl_bind_mech, "AUTHZ" );
		}
		
		rc = slap_sasl_getdn( &c, op, &user,
				realm.bv_val, nbase, SLAP_GETDN_AUTHZID );

		if ( rc == LDAP_SUCCESS ) {
			*scope = LDAP_X_SCOPE_EXACT;
		}

		return rc;

	/*
	 * group[/<groupoc>[/<groupat>]]:<groupdn>
	 *
	 * groupoc defaults to "groupOfNames"
	 * groupat defaults to "member"
	 * 
	 * <groupdn> must pass DN normalization
	 */
	} else if ( strncasecmp( uri->bv_val, "group", STRLENOF( "group" ) ) == 0 )
	{
		struct berval	group_dn = BER_BVNULL,
				group_oc = BER_BVNULL,
				member_at = BER_BVNULL;
		char		*tmp;

		bv.bv_val = uri->bv_val + STRLENOF( "group" );
		bv.bv_len = uri->bv_len - STRLENOF( "group" );
		group_dn.bv_val = ber_bvchr( &bv, ':' );
		if ( group_dn.bv_val == NULL ) {
			/* last chance: assume it's a(n exact) DN ... */
			bv.bv_val = uri->bv_val;
			*scope = LDAP_X_SCOPE_EXACT;
			goto is_dn;
		}
		
		if ( bv.bv_val[ 0 ] == '/' ) {
			group_oc.bv_val = &bv.bv_val[ 1 ];
			group_oc.bv_len = group_dn.bv_val - group_oc.bv_val;

			member_at.bv_val = ber_bvchr( &group_oc, '/' );
			if ( member_at.bv_val ) {
				group_oc.bv_len = member_at.bv_val - group_oc.bv_val;
				member_at.bv_val++;
				member_at.bv_len = group_dn.bv_val - member_at.bv_val;

			} else {
				BER_BVSTR( &member_at, SLAPD_GROUP_ATTR );
			}

		} else {
			BER_BVSTR( &group_oc, SLAPD_GROUP_CLASS );
			BER_BVSTR( &member_at, SLAPD_GROUP_ATTR );
		}
		group_dn.bv_val++;
		group_dn.bv_len = uri->bv_len - ( group_dn.bv_val - uri->bv_val );

		if ( normalize ) {
			rc = dnNormalize( 0, NULL, NULL, &group_dn, nbase, op->o_tmpmemctx );
			if ( rc != LDAP_SUCCESS ) {
				*scope = -1;
				return rc;
			}
		} else {
			ber_dupbv_x( nbase, &group_dn, op->o_tmpmemctx );
			rc = LDAP_SUCCESS;
		}
		*scope = LDAP_X_SCOPE_GROUP;

		/* FIXME: caller needs to add value of member attribute
		 * and close brackets twice */
		fstr->bv_len = STRLENOF( "(&(objectClass=)(=" /* )) */ )
			+ group_oc.bv_len + member_at.bv_len;
		fstr->bv_val = ch_malloc( fstr->bv_len + 1 );

		tmp = lutil_strncopy( fstr->bv_val, "(&(objectClass=" /* )) */ ,
				STRLENOF( "(&(objectClass=" /* )) */ ) );
		tmp = lutil_strncopy( tmp, group_oc.bv_val, group_oc.bv_len );
		tmp = lutil_strncopy( tmp, /* ( */ ")(" /* ) */ ,
				STRLENOF( /* ( */ ")(" /* ) */ ) );
		tmp = lutil_strncopy( tmp, member_at.bv_val, member_at.bv_len );
		tmp = lutil_strncopy( tmp, "=", STRLENOF( "=" ) );

		return rc;
	}

	/*
	 * ldap:///<base>??<scope>?<filter>
	 * <scope> ::= {base|one|subtree}
	 *
	 * <scope> defaults to "base"
	 * <base> must pass DN normalization
	 * <filter> must pass str2filter()
	 */
	rc = ldap_url_parse( uri->bv_val, &ludp );
	switch ( rc ) {
	case LDAP_URL_SUCCESS:
		/* FIXME: the check is pedantic, but I think it's necessary,
		 * because people tend to use things like ldaps:// which
		 * gives the idea SSL is being used.  Maybe we could
		 * accept ldapi:// as well, but the point is that we use
		 * an URL as an easy means to define bits of a search with
		 * little parsing.
		 */
		if ( strcasecmp( ludp->lud_scheme, "ldap" ) != 0 ) {
			/*
			 * must be ldap:///
			 */
			rc = LDAP_PROTOCOL_ERROR;
			goto done;
		}
		break;

	case LDAP_URL_ERR_BADSCHEME:
		/*
		 * last chance: assume it's a(n exact) DN ...
		 *
		 * NOTE: must pass DN normalization
		 */
		ldap_free_urldesc( ludp );
		bv.bv_val = uri->bv_val;
		*scope = LDAP_X_SCOPE_EXACT;
		goto is_dn;

	default:
		rc = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	if ( ( ludp->lud_host && *ludp->lud_host )
		|| ludp->lud_attrs || ludp->lud_exts )
	{
		/* host part must be empty */
		/* attrs and extensions parts must be empty */
		rc = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	/* Grab the scope */
	*scope = ludp->lud_scope;

	/* Grab the filter */
	if ( ludp->lud_filter ) {
		*filter = str2filter_x( op, ludp->lud_filter );
		if ( *filter == NULL ) {
			rc = LDAP_PROTOCOL_ERROR;
			goto done;
		}
		ber_str2bv( ludp->lud_filter, 0, 0, fstr );
	}

	/* Grab the searchbase */
	ber_str2bv( ludp->lud_dn, 0, 0, base );
	if ( normalize ) {
		rc = dnNormalize( 0, NULL, NULL, base, nbase, op->o_tmpmemctx );
	} else {
		ber_dupbv_x( nbase, base, op->o_tmpmemctx );
		rc = LDAP_SUCCESS;
	}

done:
	if( rc != LDAP_SUCCESS ) {
		if( *filter ) filter_free_x( op, *filter, 1 );
		BER_BVZERO( base );
		BER_BVZERO( fstr );
	} else {
		/* Don't free these, return them to caller */
		ludp->lud_filter = NULL;
		ludp->lud_dn = NULL;
	}

	ldap_free_urldesc( ludp );
	return( rc );
}

#ifndef SLAP_AUTH_REWRITE
static int slap_sasl_rx_off(char *rep, int *off)
{
	const char *c;
	int n;

	/* Precompile replace pattern. Find the $<n> placeholders */
	off[0] = -2;
	n = 1;
	for ( c = rep;	 *c;  c++ ) {
		if ( *c == '\\' && c[1] ) {
			c++;
			continue;
		}
		if ( *c == '$' ) {
			if ( n == SASLREGEX_REPLACE ) {
				Debug( LDAP_DEBUG_ANY,
					"SASL replace pattern %s has too many $n "
						"placeholders (max %d)\n",
					rep, SASLREGEX_REPLACE, 0 );

				return( LDAP_OTHER );
			}
			off[n] = c - rep;
			n++;
		}
	}

	/* Final placeholder, after the last $n */
	off[n] = c - rep;
	n++;
	off[n] = -1;
	return( LDAP_SUCCESS );
}
#endif /* ! SLAP_AUTH_REWRITE */

#ifdef SLAP_AUTH_REWRITE
int slap_sasl_rewrite_config( 
		const char	*fname,
		int		lineno,
		int		argc,
		char		**argv
)
{
	int	rc;
	char	*savearg0;

	/* init at first call */
	if ( sasl_rwinfo == NULL ) {
 		sasl_rwinfo = rewrite_info_init( REWRITE_MODE_USE_DEFAULT );
	}

	/* strip "authid-" prefix for parsing */
	savearg0 = argv[0];
	argv[0] += STRLENOF( "authid-" );
 	rc = rewrite_parse( sasl_rwinfo, fname, lineno, argc, argv );
	argv[0] = savearg0;

	return rc;
}

static int
slap_sasl_rewrite_destroy( void )
{
	if ( sasl_rwinfo ) {
		rewrite_info_delete( &sasl_rwinfo );
		sasl_rwinfo = NULL;
	}

	return 0;
}

int slap_sasl_regexp_rewrite_config(
		const char	*fname,
		int		lineno,
		const char	*match,
		const char	*replace,
		const char	*context )
{
	int	rc;
	char	*argvRule[] = { "rewriteRule", NULL, NULL, ":@", NULL };

	/* init at first call */
	if ( sasl_rwinfo == NULL ) {
		char *argvEngine[] = { "rewriteEngine", "on", NULL };
		char *argvContext[] = { "rewriteContext", NULL, NULL };

		/* initialize rewrite engine */
 		sasl_rwinfo = rewrite_info_init( REWRITE_MODE_USE_DEFAULT );

		/* switch on rewrite engine */
 		rc = rewrite_parse( sasl_rwinfo, fname, lineno, 2, argvEngine );
 		if (rc != LDAP_SUCCESS) {
			return rc;
		}

		/* create generic authid context */
		argvContext[1] = AUTHID_CONTEXT;
 		rc = rewrite_parse( sasl_rwinfo, fname, lineno, 2, argvContext );
 		if (rc != LDAP_SUCCESS) {
			return rc;
		}
	}

	argvRule[1] = (char *)match;
	argvRule[2] = (char *)replace;
 	rc = rewrite_parse( sasl_rwinfo, fname, lineno, 4, argvRule );

	return rc;
}
#endif /* SLAP_AUTH_REWRITE */

int slap_sasl_regexp_config( const char *match, const char *replace )
{
	int rc;
	SaslRegexp_t *reg;

	SaslRegexp = (SaslRegexp_t *) ch_realloc( (char *) SaslRegexp,
	  (nSaslRegexp + 1) * sizeof(SaslRegexp_t) );

	reg = &SaslRegexp[nSaslRegexp];

#ifdef SLAP_AUTH_REWRITE
	rc = slap_sasl_regexp_rewrite_config( "sasl-regexp", 0,
			match, replace, AUTHID_CONTEXT );
#else /* ! SLAP_AUTH_REWRITE */

	/* Precompile matching pattern */
	rc = regcomp( &reg->sr_workspace, match, REG_EXTENDED|REG_ICASE );
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			"SASL match pattern %s could not be compiled by regexp engine\n",
			match, 0, 0 );

#ifdef ENABLE_REWRITE
		/* Dummy block to force symbol references in librewrite */
		if ( slapMode == ( SLAP_SERVER_MODE|SLAP_TOOL_MODE )) {
			rewrite_info_init( 0 );
		}
#endif
		return( LDAP_OTHER );
	}

	rc = slap_sasl_rx_off( replace, reg->sr_offset );
#endif /* ! SLAP_AUTH_REWRITE */
	if ( rc == LDAP_SUCCESS ) {
		reg->sr_match = ch_strdup( match );
		reg->sr_replace = ch_strdup( replace );

		nSaslRegexp++;
	}

	return rc;
}

void
slap_sasl_regexp_destroy( void )
{
	if ( SaslRegexp ) {
		int	n;

		for ( n = 0; n < nSaslRegexp; n++ ) {
			ch_free( SaslRegexp[ n ].sr_match );
			ch_free( SaslRegexp[ n ].sr_replace );
#ifndef SLAP_AUTH_REWRITE
			regfree( &SaslRegexp[ n ].sr_workspace );
#endif /* SLAP_AUTH_REWRITE */
		}

		ch_free( SaslRegexp );
	}

#ifdef SLAP_AUTH_REWRITE
	slap_sasl_rewrite_destroy();
#endif /* SLAP_AUTH_REWRITE */
}

void slap_sasl_regexp_unparse( BerVarray *out )
{
	int i;
	BerVarray bva = NULL;
	char ibuf[32], *ptr;
	struct berval idx;

	if ( !nSaslRegexp ) return;

	idx.bv_val = ibuf;
	bva = ch_malloc( (nSaslRegexp+1) * sizeof(struct berval) );
	BER_BVZERO(bva+nSaslRegexp);
	for ( i=0; i<nSaslRegexp; i++ ) {
		idx.bv_len = sprintf( idx.bv_val, "{%d}", i);
		bva[i].bv_len = idx.bv_len + strlen( SaslRegexp[i].sr_match ) +
			strlen( SaslRegexp[i].sr_replace ) + 5;
		bva[i].bv_val = ch_malloc( bva[i].bv_len+1 );
		ptr = lutil_strcopy( bva[i].bv_val, ibuf );
		*ptr++ = '"';
		ptr = lutil_strcopy( ptr, SaslRegexp[i].sr_match );
		ptr = lutil_strcopy( ptr, "\" \"" );
		ptr = lutil_strcopy( ptr, SaslRegexp[i].sr_replace );
		*ptr++ = '"';
		*ptr = '\0';
	}
	*out = bva;
}

#ifndef SLAP_AUTH_REWRITE
/* Perform replacement on regexp matches */
static void slap_sasl_rx_exp(
	const char *rep,
	const int *off,
	regmatch_t *str,
	const char *saslname,
	struct berval *out,
	void *ctx )
{
	int i, n, len, insert;

	/* Get the total length of the final URI */

	n=1;
	len = 0;
	while( off[n] >= 0 ) {
		/* Len of next section from replacement string (x,y,z above) */
		len += off[n] - off[n-1] - 2;
		if( off[n+1] < 0)
			break;

		/* Len of string from saslname that matched next $i  (b,d above) */
		i = rep[ off[n] + 1 ]	- '0';
		len += str[i].rm_eo - str[i].rm_so;
		n++;
	}
	out->bv_val = slap_sl_malloc( len + 1, ctx );
	out->bv_len = len;

	/* Fill in URI with replace string, replacing $i as we go */
	n=1;
	insert = 0;
	while( off[n] >= 0) {
		/* Paste in next section from replacement string (x,y,z above) */
		len = off[n] - off[n-1] - 2;
		strncpy( out->bv_val+insert, rep + off[n-1] + 2, len);
		insert += len;
		if( off[n+1] < 0)
			break;

		/* Paste in string from saslname that matched next $i  (b,d above) */
		i = rep[ off[n] + 1 ]	- '0';
		len = str[i].rm_eo - str[i].rm_so;
		strncpy( out->bv_val+insert, saslname + str[i].rm_so, len );
		insert += len;

		n++;
	}

	out->bv_val[insert] = '\0';
}
#endif /* ! SLAP_AUTH_REWRITE */

/* Take the passed in SASL name and attempt to convert it into an
   LDAP URI to find the matching LDAP entry, using the pattern matching
   strings given in the saslregexp config file directive(s) */

static int slap_authz_regexp( struct berval *in, struct berval *out,
		int flags, void *ctx )
{
#ifdef SLAP_AUTH_REWRITE
	const char	*context = AUTHID_CONTEXT;

	if ( sasl_rwinfo == NULL || BER_BVISNULL( in ) ) {
		return 0;
	}

	/* FIXME: if aware of authc/authz mapping, 
	 * we could use different contexts ... */
	switch ( rewrite_session( sasl_rwinfo, context, in->bv_val, NULL, 
				&out->bv_val ) )
	{
	case REWRITE_REGEXEC_OK:
		if ( !BER_BVISNULL( out ) ) {
			char *val = out->bv_val;
			ber_str2bv_x( val, 0, 1, out, ctx );
			if ( val != in->bv_val ) {
				free( val );
			}
		} else {
			ber_dupbv_x( out, in, ctx );
		}
		Debug( LDAP_DEBUG_ARGS,
			"[rw] %s: \"%s\" -> \"%s\"\n",
			context, in->bv_val, out->bv_val );		
		return 1;
 		
 	case REWRITE_REGEXEC_UNWILLING:
	case REWRITE_REGEXEC_ERR:
	default:
		return 0;
	}

#else /* ! SLAP_AUTH_REWRITE */
	char *saslname = in->bv_val;
	SaslRegexp_t *reg;
  	regmatch_t sr_strings[SASLREGEX_REPLACE];	/* strings matching $1,$2 ... */
	int i;

	memset( out, 0, sizeof( *out ) );

	Debug( LDAP_DEBUG_TRACE, "slap_authz_regexp: converting SASL name %s\n",
	   saslname, 0, 0 );

	if (( saslname == NULL ) || ( nSaslRegexp == 0 )) {
		return( 0 );
	}

	/* Match the normalized SASL name to the saslregexp patterns */
	for( reg = SaslRegexp,i=0;  i<nSaslRegexp;  i++,reg++ ) {
		if ( regexec( &reg->sr_workspace, saslname, SASLREGEX_REPLACE,
		  sr_strings, 0)  == 0 )
			break;
	}

	if( i >= nSaslRegexp ) return( 0 );

	/*
	 * The match pattern may have been of the form "a(b.*)c(d.*)e" and the
	 * replace pattern of the form "x$1y$2z". The returned string needs
	 * to replace the $1,$2 with the strings that matched (b.*) and (d.*)
	 */
	slap_sasl_rx_exp( reg->sr_replace, reg->sr_offset,
		sr_strings, saslname, out, ctx );

	Debug( LDAP_DEBUG_TRACE,
		"slap_authz_regexp: converted SASL name to %s\n",
		BER_BVISEMPTY( out ) ? "" : out->bv_val, 0, 0 );

	return( 1 );
#endif /* ! SLAP_AUTH_REWRITE */
}

/* This callback actually does some work...*/
static int sasl_sc_sasl2dn( Operation *op, SlapReply *rs )
{
	struct berval *ndn = op->o_callback->sc_private;

	if ( rs->sr_type != REP_SEARCH ) return LDAP_SUCCESS;

	/* We only want to be called once */
	if ( !BER_BVISNULL( ndn ) ) {
		op->o_tmpfree( ndn->bv_val, op->o_tmpmemctx );
		BER_BVZERO( ndn );

		Debug( LDAP_DEBUG_TRACE,
			"%s: slap_sc_sasl2dn: search DN returned more than 1 entry\n",
			op->o_log_prefix, 0, 0 );
		return LDAP_UNAVAILABLE; /* short-circuit the search */
	}

	ber_dupbv_x( ndn, &rs->sr_entry->e_nname, op->o_tmpmemctx );
	return LDAP_SUCCESS;
}


typedef struct smatch_info {
	struct berval *dn;
	int match;
} smatch_info;

static int sasl_sc_smatch( Operation *o, SlapReply *rs )
{
	smatch_info *sm = o->o_callback->sc_private;

	if (rs->sr_type != REP_SEARCH) return 0;

	if (dn_match(sm->dn, &rs->sr_entry->e_nname)) {
		sm->match = 1;
		return LDAP_UNAVAILABLE;	/* short-circuit the search */
	}

	return 0;
}

int
slap_sasl_matches( Operation *op, BerVarray rules,
		struct berval *assertDN, struct berval *authc )
{
	int	rc = LDAP_INAPPROPRIATE_AUTH;

	if ( rules != NULL ) {
		int	i;

		for( i = 0; !BER_BVISNULL( &rules[i] ); i++ ) {
			rc = slap_sasl_match( op, &rules[i], assertDN, authc );
			if ( rc == LDAP_SUCCESS ) break;
		}
	}
	
	return rc;
}

/*
 * Map a SASL regexp rule to a DN. If the rule is just a DN or a scope=base
 * URI, just strcmp the rule (or its searchbase) to the *assertDN. Otherwise,
 * the rule must be used as an internal search for entries. If that search
 * returns the *assertDN entry, the match is successful.
 *
 * The assertDN should not have the dn: prefix
 */

static int
slap_sasl_match( Operation *opx, struct berval *rule,
	struct berval *assertDN, struct berval *authc )
{
	int rc; 
	regex_t reg;
	smatch_info sm;
	slap_callback cb = { NULL, sasl_sc_smatch, NULL, NULL };
	Operation op = {0};
	SlapReply rs = {REP_RESULT};
	struct berval base = BER_BVNULL;

	sm.dn = assertDN;
	sm.match = 0;
	cb.sc_private = &sm;

	Debug( LDAP_DEBUG_TRACE,
	   "===>slap_sasl_match: comparing DN %s to rule %s\n",
		assertDN->bv_len ? assertDN->bv_val : "(null)", rule->bv_val, 0 );

	/* NOTE: don't normalize rule if authz syntax is enabled */
	rc = slap_parseURI( opx, rule, &base, &op.o_req_ndn,
		&op.ors_scope, &op.ors_filter, &op.ors_filterstr, 0 );

	if( rc != LDAP_SUCCESS ) goto CONCLUDED;

	switch ( op.ors_scope ) {
	case LDAP_X_SCOPE_EXACT:
exact_match:
		if ( dn_match( &op.o_req_ndn, assertDN ) ) {
			rc = LDAP_SUCCESS;
		} else {
			rc = LDAP_INAPPROPRIATE_AUTH;
		}
		goto CONCLUDED;

	case LDAP_X_SCOPE_CHILDREN:
	case LDAP_X_SCOPE_SUBTREE:
	case LDAP_X_SCOPE_ONELEVEL:
	{
		int	d = assertDN->bv_len - op.o_req_ndn.bv_len;

		rc = LDAP_INAPPROPRIATE_AUTH;

		if ( d == 0 && op.ors_scope == LDAP_X_SCOPE_SUBTREE ) {
			goto exact_match;

		} else if ( d > 0 ) {
			struct berval bv;

			/* leave room for at least one char of attributeType,
			 * one for '=' and one for ',' */
			if ( d < (int) STRLENOF( "x=,") ) {
				goto CONCLUDED;
			}

			bv.bv_len = op.o_req_ndn.bv_len;
			bv.bv_val = assertDN->bv_val + d;

			if ( bv.bv_val[ -1 ] == ',' && dn_match( &op.o_req_ndn, &bv ) ) {
				switch ( op.ors_scope ) {
				case LDAP_X_SCOPE_SUBTREE:
				case LDAP_X_SCOPE_CHILDREN:
					rc = LDAP_SUCCESS;
					break;

				case LDAP_X_SCOPE_ONELEVEL:
				{
					struct berval	pdn;

					dnParent( assertDN, &pdn );
					/* the common portion of the DN
					 * already matches, so only check
					 * if parent DN of assertedDN 
					 * is all the pattern */
					if ( pdn.bv_len == op.o_req_ndn.bv_len ) {
						rc = LDAP_SUCCESS;
					}
					break;
				}
				default:
					/* at present, impossible */
					assert( 0 );
				}
			}
		}
		goto CONCLUDED;
	}

	case LDAP_X_SCOPE_REGEX:
		rc = regcomp(&reg, op.o_req_ndn.bv_val,
			REG_EXTENDED|REG_ICASE|REG_NOSUB);
		if ( rc == 0 ) {
			rc = regexec(&reg, assertDN->bv_val, 0, NULL, 0);
			regfree( &reg );
		}
		if ( rc == 0 ) {
			rc = LDAP_SUCCESS;
		} else {
			rc = LDAP_INAPPROPRIATE_AUTH;
		}
		goto CONCLUDED;

	case LDAP_X_SCOPE_GROUP: {
		char	*tmp;

		/* Now filterstr looks like "(&(objectClass=<group_oc>)(<member_at>="
		 * we need to append the <assertDN> so that the <group_dn> is searched
		 * with scope "base", and the filter ensures that <assertDN> is
		 * member of the group */
		tmp = ch_realloc( op.ors_filterstr.bv_val, op.ors_filterstr.bv_len +
			assertDN->bv_len + STRLENOF( /*"(("*/ "))" ) + 1 );
		if ( tmp == NULL ) {
			rc = LDAP_NO_MEMORY;
			goto CONCLUDED;
		}
		op.ors_filterstr.bv_val = tmp;
		
		tmp = lutil_strcopy( &tmp[op.ors_filterstr.bv_len], assertDN->bv_val );
		tmp = lutil_strcopy( tmp, /*"(("*/ "))" );

		/* pass opx because str2filter_x may (and does) use o_tmpmfuncs */
		op.ors_filter = str2filter_x( opx, op.ors_filterstr.bv_val );
		if ( op.ors_filter == NULL ) {
			rc = LDAP_PROTOCOL_ERROR;
			goto CONCLUDED;
		}
		op.ors_scope = LDAP_SCOPE_BASE;

		/* hijack match DN: use that of the group instead of the assertDN;
		 * assertDN is now in the filter */
		sm.dn = &op.o_req_ndn;

		/* do the search */
		break;
		}

	case LDAP_X_SCOPE_USERS:
		if ( !BER_BVISEMPTY( assertDN ) ) {
			rc = LDAP_SUCCESS;
		} else {
			rc = LDAP_INAPPROPRIATE_AUTH;
		}
		goto CONCLUDED;

	default:
		break;
	}

	/* Must run an internal search. */
	if ( op.ors_filter == NULL ) {
		rc = LDAP_FILTER_ERROR;
		goto CONCLUDED;
	}

	Debug( LDAP_DEBUG_TRACE,
	   "slap_sasl_match: performing internal search (base=%s, scope=%d)\n",
	   op.o_req_ndn.bv_val, op.ors_scope, 0 );

	op.o_bd = select_backend( &op.o_req_ndn, 1 );
	if(( op.o_bd == NULL ) || ( op.o_bd->be_search == NULL)) {
		rc = LDAP_INAPPROPRIATE_AUTH;
		goto CONCLUDED;
	}

	op.o_hdr = opx->o_hdr;
	op.o_tag = LDAP_REQ_SEARCH;
	op.o_ndn = *authc;
	op.o_callback = &cb;
	slap_op_time( &op.o_time, &op.o_tincr );
	op.o_do_not_cache = 1;
	op.o_is_auth_check = 1;
	/* use req_ndn as req_dn instead of non-pretty base of uri */
	if( !BER_BVISNULL( &base ) ) {
		ch_free( base.bv_val );
		/* just in case... */
		BER_BVZERO( &base );
	}
	ber_dupbv_x( &op.o_req_dn, &op.o_req_ndn, op.o_tmpmemctx );
	op.ors_deref = LDAP_DEREF_NEVER;
	op.ors_slimit = 1;
	op.ors_tlimit = SLAP_NO_LIMIT;
	op.ors_attrs = slap_anlist_no_attrs;
	op.ors_attrsonly = 1;

	op.o_bd->be_search( &op, &rs );

	if (sm.match) {
		rc = LDAP_SUCCESS;
	} else {
		rc = LDAP_INAPPROPRIATE_AUTH;
	}

CONCLUDED:
	if( !BER_BVISNULL( &op.o_req_dn ) ) slap_sl_free( op.o_req_dn.bv_val, opx->o_tmpmemctx );
	if( !BER_BVISNULL( &op.o_req_ndn ) ) slap_sl_free( op.o_req_ndn.bv_val, opx->o_tmpmemctx );
	if( op.ors_filter ) filter_free_x( opx, op.ors_filter, 1 );
	if( !BER_BVISNULL( &op.ors_filterstr ) ) ch_free( op.ors_filterstr.bv_val );

	Debug( LDAP_DEBUG_TRACE,
	   "<===slap_sasl_match: comparison returned %d\n", rc, 0, 0);

	return( rc );
}


/*
 * This function answers the question, "Can this ID authorize to that ID?",
 * based on authorization rules. The rules are stored in the *searchDN, in the
 * attribute named by *attr. If any of those rules map to the *assertDN, the
 * authorization is approved.
 *
 * The DNs should not have the dn: prefix
 */
static int
slap_sasl_check_authz( Operation *op,
	struct berval *searchDN,
	struct berval *assertDN,
	AttributeDescription *ad,
	struct berval *authc )
{
	int		rc,
			do_not_cache = op->o_do_not_cache;
	BerVarray	vals = NULL;

	Debug( LDAP_DEBUG_TRACE,
	   "==>slap_sasl_check_authz: does %s match %s rule in %s?\n",
	   assertDN->bv_val, ad->ad_cname.bv_val, searchDN->bv_val);

	/* ITS#4760: don't cache group access */
	op->o_do_not_cache = 1;
	rc = backend_attribute( op, NULL, searchDN, ad, &vals, ACL_AUTH );
	op->o_do_not_cache = do_not_cache;
	if( rc != LDAP_SUCCESS ) goto COMPLETE;

	/* Check if the *assertDN matches any *vals */
	rc = slap_sasl_matches( op, vals, assertDN, authc );

COMPLETE:
	if( vals ) ber_bvarray_free_x( vals, op->o_tmpmemctx );

	Debug( LDAP_DEBUG_TRACE,
	   "<==slap_sasl_check_authz: %s check returning %d\n",
		ad->ad_cname.bv_val, rc, 0);

	return( rc );
}

/*
 * Given a SASL name (e.g. "UID=name,cn=REALM,cn=MECH,cn=AUTH")
 * return the LDAP DN to which it matches. The SASL regexp rules in the config
 * file turn the SASL name into an LDAP URI. If the URI is just a DN (or a
 * search with scope=base), just return the URI (or its searchbase). Otherwise
 * an internal search must be done, and if that search returns exactly one
 * entry, return the DN of that one entry.
 */
void
slap_sasl2dn(
	Operation	*opx,
	struct berval	*saslname,
	struct berval	*sasldn,
	int		flags )
{
	int rc;
	slap_callback cb = { NULL, sasl_sc_sasl2dn, NULL, NULL };
	Operation op = {0};
	SlapReply rs = {REP_RESULT};
	struct berval regout = BER_BVNULL;
	struct berval base = BER_BVNULL;

	Debug( LDAP_DEBUG_TRACE, "==>slap_sasl2dn: "
		"converting SASL name %s to a DN\n",
		saslname->bv_val, 0,0 );

	BER_BVZERO( sasldn );
	cb.sc_private = sasldn;

	/* Convert the SASL name into a minimal URI */
	if( !slap_authz_regexp( saslname, &regout, flags, opx->o_tmpmemctx ) ) {
		goto FINISHED;
	}

	/* NOTE: always normalize regout because it results
	 * from string submatch expansion */
	rc = slap_parseURI( opx, &regout, &base, &op.o_req_ndn,
		&op.ors_scope, &op.ors_filter, &op.ors_filterstr, 1 );
	if ( !BER_BVISNULL( &regout ) ) slap_sl_free( regout.bv_val, opx->o_tmpmemctx );
	if ( rc != LDAP_SUCCESS ) {
		goto FINISHED;
	}

	/* Must do an internal search */
	op.o_bd = select_backend( &op.o_req_ndn, 1 );

	switch ( op.ors_scope ) {
	case LDAP_X_SCOPE_EXACT:
		*sasldn = op.o_req_ndn;
		BER_BVZERO( &op.o_req_ndn );
		/* intentionally continue to next case */

	case LDAP_X_SCOPE_REGEX:
	case LDAP_X_SCOPE_SUBTREE:
	case LDAP_X_SCOPE_CHILDREN:
	case LDAP_X_SCOPE_ONELEVEL:
	case LDAP_X_SCOPE_GROUP:
	case LDAP_X_SCOPE_USERS:
		/* correctly parsed, but illegal */
		goto FINISHED;

	case LDAP_SCOPE_BASE:
	case LDAP_SCOPE_ONELEVEL:
	case LDAP_SCOPE_SUBTREE:
	case LDAP_SCOPE_SUBORDINATE:
		/* do a search */
		break;

	default:
		/* catch unhandled cases (there shouldn't be) */
		assert( 0 );
	}

	Debug( LDAP_DEBUG_TRACE,
		"slap_sasl2dn: performing internal search (base=%s, scope=%d)\n",
		op.o_req_ndn.bv_val, op.ors_scope, 0 );

	if ( ( op.o_bd == NULL ) || ( op.o_bd->be_search == NULL) ) {
		goto FINISHED;
	}

	/* Must run an internal search. */
	if ( op.ors_filter == NULL ) {
		rc = LDAP_FILTER_ERROR;
		goto FINISHED;
	}

	op.o_hdr = opx->o_hdr;
	op.o_tag = LDAP_REQ_SEARCH;
	op.o_ndn = opx->o_conn->c_ndn;
	op.o_callback = &cb;
	slap_op_time( &op.o_time, &op.o_tincr );
	op.o_do_not_cache = 1;
	op.o_is_auth_check = 1;
	op.ors_deref = LDAP_DEREF_NEVER;
	op.ors_slimit = 1;
	op.ors_tlimit = SLAP_NO_LIMIT;
	op.ors_attrs = slap_anlist_no_attrs;
	op.ors_attrsonly = 1;
	/* use req_ndn as req_dn instead of non-pretty base of uri */
	if( !BER_BVISNULL( &base ) ) {
		ch_free( base.bv_val );
		/* just in case... */
		BER_BVZERO( &base );
	}
	ber_dupbv_x( &op.o_req_dn, &op.o_req_ndn, op.o_tmpmemctx );

	op.o_bd->be_search( &op, &rs );
	
FINISHED:
	if( opx == opx->o_conn->c_sasl_bindop && !BER_BVISEMPTY( sasldn ) ) {
		opx->o_conn->c_authz_backend = op.o_bd;
	}
	if( !BER_BVISNULL( &op.o_req_dn ) ) {
		slap_sl_free( op.o_req_dn.bv_val, opx->o_tmpmemctx );
	}
	if( !BER_BVISNULL( &op.o_req_ndn ) ) {
		slap_sl_free( op.o_req_ndn.bv_val, opx->o_tmpmemctx );
	}
	if( op.ors_filter ) {
		filter_free_x( opx, op.ors_filter, 1 );
	}
	if( !BER_BVISNULL( &op.ors_filterstr ) ) {
		ch_free( op.ors_filterstr.bv_val );
	}

	Debug( LDAP_DEBUG_TRACE, "<==slap_sasl2dn: Converted SASL name to %s\n",
		!BER_BVISEMPTY( sasldn ) ? sasldn->bv_val : "<nothing>", 0, 0 );

	return;
}


/* Check if a bind can SASL authorize to another identity.
 * The DNs should not have the dn: prefix
 */

int slap_sasl_authorized( Operation *op,
	struct berval *authcDN, struct berval *authzDN )
{
	int rc = LDAP_INAPPROPRIATE_AUTH;

	/* User binding as anonymous */
	if ( !authzDN || !authzDN->bv_len || !authzDN->bv_val ) {
		rc = LDAP_SUCCESS;
		goto DONE;
	}

	/* User is anonymous */
	if ( !authcDN || !authcDN->bv_len || !authcDN->bv_val ) {
		goto DONE;
	}

	Debug( LDAP_DEBUG_TRACE,
	   "==>slap_sasl_authorized: can %s become %s?\n",
		authcDN->bv_len ? authcDN->bv_val : "(null)",
		authzDN->bv_len ? authzDN->bv_val : "(null)",  0 );

	/* If person is authorizing to self, succeed */
	if ( dn_match( authcDN, authzDN ) ) {
		rc = LDAP_SUCCESS;
		goto DONE;
	}

	/* Allow the manager to authorize as any DN. */
	if( op->o_conn->c_authz_backend &&
		be_isroot_dn( op->o_conn->c_authz_backend, authcDN ))
	{
		rc = LDAP_SUCCESS;
		goto DONE;
	}

	/* Check source rules */
	if( authz_policy & SASL_AUTHZ_TO ) {
		rc = slap_sasl_check_authz( op, authcDN, authzDN,
			slap_schema.si_ad_saslAuthzTo, authcDN );
		if( rc == LDAP_SUCCESS && !(authz_policy & SASL_AUTHZ_AND) ) {
			goto DONE;
		}
	}

	/* Check destination rules */
	if( authz_policy & SASL_AUTHZ_FROM ) {
		rc = slap_sasl_check_authz( op, authzDN, authcDN,
			slap_schema.si_ad_saslAuthzFrom, authcDN );
		if( rc == LDAP_SUCCESS ) {
			goto DONE;
		}
	}

	rc = LDAP_INAPPROPRIATE_AUTH;

DONE:

	Debug( LDAP_DEBUG_TRACE,
		"<== slap_sasl_authorized: return %d\n", rc, 0, 0 );

	return( rc );
}
