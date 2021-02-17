/* aci.c - routines to parse and check acl's */
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

#ifdef SLAPD_ACI_ENABLED

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/regex.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/unistd.h>

#include "slap.h"
#include "lber_pvt.h"
#include "lutil.h"

/* use most appropriate size */
#define ACI_BUF_SIZE 			1024

/* move to "stable" when no longer experimental */
#define SLAPD_ACI_SYNTAX		"1.3.6.1.4.1.4203.666.2.1"

/* change this to "OpenLDAPset" */
#define SLAPD_ACI_SET_ATTR		"template"

typedef enum slap_aci_scope_t {
	SLAP_ACI_SCOPE_ENTRY		= 0x1,
	SLAP_ACI_SCOPE_CHILDREN		= 0x2,
	SLAP_ACI_SCOPE_SUBTREE		= ( SLAP_ACI_SCOPE_ENTRY | SLAP_ACI_SCOPE_CHILDREN )
} slap_aci_scope_t;

enum {
	ACI_BV_ENTRY,
	ACI_BV_CHILDREN,
	ACI_BV_ONELEVEL,
	ACI_BV_SUBTREE,

	ACI_BV_BR_ENTRY,
	ACI_BV_BR_CHILDREN,
	ACI_BV_BR_ALL,

	ACI_BV_ACCESS_ID,
	ACI_BV_PUBLIC,
	ACI_BV_USERS,
	ACI_BV_SELF,
	ACI_BV_DNATTR,
	ACI_BV_GROUP,
	ACI_BV_ROLE,
	ACI_BV_SET,
	ACI_BV_SET_REF,

	ACI_BV_GRANT,
	ACI_BV_DENY,

	ACI_BV_GROUP_CLASS,
	ACI_BV_GROUP_ATTR,
	ACI_BV_ROLE_CLASS,
	ACI_BV_ROLE_ATTR,

	ACI_BV_SET_ATTR,

	ACI_BV_LAST
};

static const struct berval	aci_bv[] = {
	/* scope */
	BER_BVC("entry"),
	BER_BVC("children"),
	BER_BVC("onelevel"),
	BER_BVC("subtree"),

	/* */
	BER_BVC("[entry]"),
	BER_BVC("[children]"),
	BER_BVC("[all]"),

	/* type */
	BER_BVC("access-id"),
	BER_BVC("public"),
	BER_BVC("users"),
	BER_BVC("self"),
	BER_BVC("dnattr"),
	BER_BVC("group"),
	BER_BVC("role"),
	BER_BVC("set"),
	BER_BVC("set-ref"),

	/* actions */
	BER_BVC("grant"),
	BER_BVC("deny"),

	/* schema */
	BER_BVC(SLAPD_GROUP_CLASS),
	BER_BVC(SLAPD_GROUP_ATTR),
	BER_BVC(SLAPD_ROLE_CLASS),
	BER_BVC(SLAPD_ROLE_ATTR),

	BER_BVC(SLAPD_ACI_SET_ATTR),

	BER_BVNULL
};

static AttributeDescription	*slap_ad_aci;

static int
OpenLDAPaciValidate(
	Syntax		*syntax,
	struct berval	*val );

static int
OpenLDAPaciPretty(
	Syntax		*syntax,
	struct berval	*val,
	struct berval	*out,
	void		*ctx );

static int
OpenLDAPaciNormalize(
	slap_mask_t	use,
	Syntax		*syntax,
	MatchingRule	*mr,
	struct berval	*val,
	struct berval	*out,
	void		*ctx );

#define	OpenLDAPaciMatch			octetStringMatch

static int
aci_list_map_rights(
	struct berval	*list )
{
	struct berval	bv;
	slap_access_t	mask;
	int		i;

	ACL_INIT( mask );
	for ( i = 0; acl_get_part( list, i, ',', &bv ) >= 0; i++ ) {
		if ( bv.bv_len <= 0 ) {
			continue;
		}

		switch ( *bv.bv_val ) {
		case 'x':
			/* **** NOTE: draft-ietf-ldapext-aci-model-0.3.txt does not 
			 * define any equivalent to the AUTH right, so I've just used
			 * 'x' for now.
			 */
			ACL_PRIV_SET(mask, ACL_PRIV_AUTH);
			break;
		case 'd':
			/* **** NOTE: draft-ietf-ldapext-aci-model-0.3.txt defines
			 * the right 'd' to mean "delete"; we hijack it to mean
			 * "disclose" for consistency wuith the rest of slapd.
			 */
			ACL_PRIV_SET(mask, ACL_PRIV_DISCLOSE);
			break;
		case 'c':
			ACL_PRIV_SET(mask, ACL_PRIV_COMPARE);
			break;
		case 's':
			/* **** NOTE: draft-ietf-ldapext-aci-model-0.3.txt defines
			 * the right 's' to mean "set", but in the examples states
			 * that the right 's' means "search".  The latter definition
			 * is used here.
			 */
			ACL_PRIV_SET(mask, ACL_PRIV_SEARCH);
			break;
		case 'r':
			ACL_PRIV_SET(mask, ACL_PRIV_READ);
			break;
		case 'w':
			ACL_PRIV_SET(mask, ACL_PRIV_WRITE);
			break;
		default:
			break;
		}

	}

	return mask;
}

static int
aci_list_has_attr(
	struct berval		*list,
	const struct berval	*attr,
	struct berval		*val )
{
	struct berval	bv, left, right;
	int		i;

	for ( i = 0; acl_get_part( list, i, ',', &bv ) >= 0; i++ ) {
		if ( acl_get_part(&bv, 0, '=', &left ) < 0
			|| acl_get_part( &bv, 1, '=', &right ) < 0 )
		{
			if ( ber_bvstrcasecmp( attr, &bv ) == 0 ) {
				return(1);
			}

		} else if ( val == NULL ) {
			if ( ber_bvstrcasecmp( attr, &left ) == 0 ) {
				return(1);
			}

		} else {
			if ( ber_bvstrcasecmp( attr, &left ) == 0 ) {
				/* FIXME: this is also totally undocumented! */
				/* this is experimental code that implements a
				 * simple (prefix) match of the attribute value.
				 * the ACI draft does not provide for aci's that
				 * apply to specific values, but it would be
				 * nice to have.  If the <attr> part of an aci's
				 * rights list is of the form <attr>=<value>,
				 * that means the aci applies only to attrs with
				 * the given value.  Furthermore, if the attr is
				 * of the form <attr>=<value>*, then <value> is
				 * treated as a prefix, and the aci applies to 
				 * any value with that prefix.
				 *
				 * Ideally, this would allow r.e. matches.
				 */
				if ( acl_get_part( &right, 0, '*', &left ) < 0
					|| right.bv_len <= left.bv_len )
				{
					if ( ber_bvstrcasecmp( val, &right ) == 0 ) {
						return 1;
					}

				} else if ( val->bv_len >= left.bv_len ) {
					if ( strncasecmp( val->bv_val, left.bv_val, left.bv_len ) == 0 ) {
						return(1);
					}
				}
			}
		}
	}

	return 0;
}

static slap_access_t
aci_list_get_attr_rights(
	struct berval		*list,
	const struct berval	*attr,
	struct berval		*val )
{
	struct berval	bv;
	slap_access_t	mask;
	int		i;

	/* loop through each rights/attr pair, skip first part (action) */
	ACL_INIT(mask);
	for ( i = 1; acl_get_part( list, i + 1, ';', &bv ) >= 0; i += 2 ) {
		if ( aci_list_has_attr( &bv, attr, val ) == 0 ) {
			Debug( LDAP_DEBUG_ACL,
				"        <= aci_list_get_attr_rights "
				"test %s for %s -> failed\n",
				bv.bv_val, attr->bv_val, 0 );
			continue;
		}

		Debug( LDAP_DEBUG_ACL,
			"        <= aci_list_get_attr_rights "
			"test %s for %s -> ok\n",
			bv.bv_val, attr->bv_val, 0 );

		if ( acl_get_part( list, i, ';', &bv ) < 0 ) {
			Debug( LDAP_DEBUG_ACL,
				"        <= aci_list_get_attr_rights "
				"test no rights\n",
				0, 0, 0 );
			continue;
		}

		mask |= aci_list_map_rights( &bv );
		Debug( LDAP_DEBUG_ACL,
			"        <= aci_list_get_attr_rights "
			"rights %s to mask 0x%x\n",
			bv.bv_val, mask, 0 );
	}

	return mask;
}

static int
aci_list_get_rights(
	struct berval	*list,
	struct berval	*attr,
	struct berval	*val,
	slap_access_t	*grant,
	slap_access_t	*deny )
{
	struct berval	perm, actn, baseattr;
	slap_access_t	*mask;
	int		i, found;

	if ( attr == NULL || BER_BVISEMPTY( attr ) ) {
		attr = (struct berval *)&aci_bv[ ACI_BV_ENTRY ];

	} else if ( acl_get_part( attr, 0, ';', &baseattr ) > 0 ) {
		attr = &baseattr;
	}
	found = 0;
	ACL_INIT(*grant);
	ACL_INIT(*deny);
	/* loop through each permissions clause */
	for ( i = 0; acl_get_part( list, i, '$', &perm ) >= 0; i++ ) {
		if ( acl_get_part( &perm, 0, ';', &actn ) < 0 ) {
			continue;
		}

		if ( ber_bvstrcasecmp( &aci_bv[ ACI_BV_GRANT ], &actn ) == 0 ) {
			mask = grant;

		} else if ( ber_bvstrcasecmp( &aci_bv[ ACI_BV_DENY ], &actn ) == 0 ) {
			mask = deny;

		} else {
			continue;
		}

		*mask |= aci_list_get_attr_rights( &perm, attr, val );
		*mask |= aci_list_get_attr_rights( &perm, &aci_bv[ ACI_BV_BR_ALL ], NULL );

		if ( *mask != ACL_PRIV_NONE ) { 
			found = 1;
		}
	}

	return found;
}

static int
aci_group_member (
	struct berval		*subj,
	const struct berval	*defgrpoc,
	const struct berval	*defgrpat,
	Operation		*op,
	Entry			*e,
	int			nmatch,
	regmatch_t		*matches
)
{
	struct berval		subjdn;
	struct berval		grpoc;
	struct berval		grpat;
	ObjectClass		*grp_oc = NULL;
	AttributeDescription	*grp_ad = NULL;
	const char		*text;
	int			rc;

	/* format of string is "{group|role}/objectClassValue/groupAttrName" */
	if ( acl_get_part( subj, 0, '/', &subjdn ) < 0 ) {
		return 0;
	}

	if ( acl_get_part( subj, 1, '/', &grpoc ) < 0 ) {
		grpoc = *defgrpoc;
	}

	if ( acl_get_part( subj, 2, '/', &grpat ) < 0 ) {
		grpat = *defgrpat;
	}

	rc = slap_bv2ad( &grpat, &grp_ad, &text );
	if ( rc != LDAP_SUCCESS ) {
		rc = 0;
		goto done;
	}
	rc = 0;

	grp_oc = oc_bvfind( &grpoc );

	if ( grp_oc != NULL && grp_ad != NULL ) {
		char		buf[ ACI_BUF_SIZE ];
		struct berval	bv, ndn;
		AclRegexMatches amatches = { 0 };

		amatches.dn_count = nmatch;
		AC_MEMCPY( amatches.dn_data, matches, sizeof( amatches.dn_data ) );

		bv.bv_len = sizeof( buf ) - 1;
		bv.bv_val = (char *)&buf;
		if ( acl_string_expand( &bv, &subjdn,
				&e->e_nname, NULL, &amatches ) )
		{
			rc = LDAP_OTHER;
			goto done;
		}

		if ( dnNormalize( 0, NULL, NULL, &bv, &ndn, op->o_tmpmemctx ) == LDAP_SUCCESS )
		{
			rc = ( backend_group( op, e, &ndn, &op->o_ndn,
				grp_oc, grp_ad ) == 0 );
			slap_sl_free( ndn.bv_val, op->o_tmpmemctx );
		}
	}

done:
	return rc;
}

static int
aci_mask(
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	struct berval		*aci,
	int			nmatch,
	regmatch_t		*matches,
	slap_access_t		*grant,
	slap_access_t		*deny,
	slap_aci_scope_t	asserted_scope )
{
	struct berval		bv,
				scope,
				perms,
				type,
				opts,
				sdn;
	int			rc;

	ACL_INIT( *grant );
	ACL_INIT( *deny );

	assert( !BER_BVISNULL( &desc->ad_cname ) );

	/* parse an aci of the form:
		oid # scope # action;rights;attr;rights;attr 
			$ action;rights;attr;rights;attr # type # subject

	   [NOTE: the following comment is very outdated,
	   as the draft version it refers to (Ando, 2004-11-20)].

	   See draft-ietf-ldapext-aci-model-04.txt section 9.1 for
	   a full description of the format for this attribute.
	   Differences: "this" in the draft is "self" here, and
	   "self" and "public" is in the position of type.

	   <scope> = {entry|children|subtree}
	   <type> = {public|users|access-id|subtree|onelevel|children|
	             self|dnattr|group|role|set|set-ref}

	   This routine now supports scope={ENTRY,CHILDREN}
	   with the semantics:
	     - ENTRY applies to "entry" and "subtree";
	     - CHILDREN applies to "children" and "subtree"
	 */

	/* check that the aci has all 5 components */
	if ( acl_get_part( aci, 4, '#', NULL ) < 0 ) {
		return 0;
	}

	/* check that the aci family is supported */
	/* FIXME: the OID is ignored? */
	if ( acl_get_part( aci, 0, '#', &bv ) < 0 ) {
		return 0;
	}

	/* check that the scope matches */
	if ( acl_get_part( aci, 1, '#', &scope ) < 0 ) {
		return 0;
	}

	/* note: scope can be either ENTRY or CHILDREN;
	 * they respectively match "entry" and "children" in bv
	 * both match "subtree" */
	switch ( asserted_scope ) {
	case SLAP_ACI_SCOPE_ENTRY:
		if ( ber_bvcmp( &scope, &aci_bv[ ACI_BV_ENTRY ] ) != 0
				&& ber_bvstrcasecmp( &scope, &aci_bv[ ACI_BV_SUBTREE ] ) != 0 )
		{
			return 0;
		}
		break;

	case SLAP_ACI_SCOPE_CHILDREN:
		if ( ber_bvcmp( &scope, &aci_bv[ ACI_BV_CHILDREN ] ) != 0
				&& ber_bvstrcasecmp( &scope, &aci_bv[ ACI_BV_SUBTREE ] ) != 0 )
		{
			return 0;
		}
		break;

	case SLAP_ACI_SCOPE_SUBTREE:
		/* TODO: add assertion? */
		return 0;
	}

	/* get the list of permissions clauses, bail if empty */
	if ( acl_get_part( aci, 2, '#', &perms ) <= 0 ) {
		assert( 0 );
		return 0;
	}

	/* check if any permissions allow desired access */
	if ( aci_list_get_rights( &perms, &desc->ad_cname, val, grant, deny ) == 0 ) {
		return 0;
	}

	/* see if we have a DN match */
	if ( acl_get_part( aci, 3, '#', &type ) < 0 ) {
		assert( 0 );
		return 0;
	}

	/* see if we have a public (i.e. anonymous) access */
	if ( ber_bvcmp( &aci_bv[ ACI_BV_PUBLIC ], &type ) == 0 ) {
		return 1;
	}
	
	/* otherwise require an identity */
	if ( BER_BVISNULL( &op->o_ndn ) || BER_BVISEMPTY( &op->o_ndn ) ) {
		return 0;
	}

	/* see if we have a users access */
	if ( ber_bvcmp( &aci_bv[ ACI_BV_USERS ], &type ) == 0 ) {
		return 1;
	}
	
	/* NOTE: this may fail if a DN contains a valid '#' (unescaped);
	 * just grab all the berval up to its end (ITS#3303).
	 * NOTE: the problem could be solved by providing the DN with
	 * the embedded '#' encoded as hexpairs: "cn=Foo#Bar" would 
	 * become "cn=Foo\23Bar" and be safely used by aci_mask(). */
#if 0
	if ( acl_get_part( aci, 4, '#', &sdn ) < 0 ) {
		return 0;
	}
#endif
	sdn.bv_val = type.bv_val + type.bv_len + STRLENOF( "#" );
	sdn.bv_len = aci->bv_len - ( sdn.bv_val - aci->bv_val );

	/* get the type options, if any */
	if ( acl_get_part( &type, 1, '/', &opts ) > 0 ) {
		opts.bv_len = type.bv_len - ( opts.bv_val - type.bv_val );
		type.bv_len = opts.bv_val - type.bv_val - 1;

	} else {
		BER_BVZERO( &opts );
	}

	if ( ber_bvcmp( &aci_bv[ ACI_BV_ACCESS_ID ], &type ) == 0 ) {
		return dn_match( &op->o_ndn, &sdn );

	} else if ( ber_bvcmp( &aci_bv[ ACI_BV_SUBTREE ], &type ) == 0 ) {
		return dnIsSuffix( &op->o_ndn, &sdn );

	} else if ( ber_bvcmp( &aci_bv[ ACI_BV_ONELEVEL ], &type ) == 0 ) {
		struct berval pdn;
		
		dnParent( &sdn, &pdn );

		return dn_match( &op->o_ndn, &pdn );

	} else if ( ber_bvcmp( &aci_bv[ ACI_BV_CHILDREN ], &type ) == 0 ) {
		return ( !dn_match( &op->o_ndn, &sdn ) && dnIsSuffix( &op->o_ndn, &sdn ) );

	} else if ( ber_bvcmp( &aci_bv[ ACI_BV_SELF ], &type ) == 0 ) {
		return dn_match( &op->o_ndn, &e->e_nname );

	} else if ( ber_bvcmp( &aci_bv[ ACI_BV_DNATTR ], &type ) == 0 ) {
		Attribute		*at;
		AttributeDescription	*ad = NULL;
		const char		*text;

		rc = slap_bv2ad( &sdn, &ad, &text );
		assert( rc == LDAP_SUCCESS );

		rc = 0;
		for ( at = attrs_find( e->e_attrs, ad );
				at != NULL;
				at = attrs_find( at->a_next, ad ) )
		{
			if ( attr_valfind( at, 
				SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
					SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
				&op->o_ndn, NULL, op->o_tmpmemctx ) == 0 )
			{
				rc = 1;
				break;
			}
		}

		return rc;

	} else if ( ber_bvcmp( &aci_bv[ ACI_BV_GROUP ], &type ) == 0 ) {
		struct berval	oc,
				at;

		if ( BER_BVISNULL( &opts ) ) {
			oc = aci_bv[ ACI_BV_GROUP_CLASS ];
			at = aci_bv[ ACI_BV_GROUP_ATTR ];

		} else {
			if ( acl_get_part( &opts, 0, '/', &oc ) < 0 ) {
				assert( 0 );
			}

			if ( acl_get_part( &opts, 1, '/', &at ) < 0 ) {
				at = aci_bv[ ACI_BV_GROUP_ATTR ];
			}
		}

		if ( aci_group_member( &sdn, &oc, &at, op, e, nmatch, matches ) )
		{
			return 1;
		}

	} else if ( ber_bvcmp( &aci_bv[ ACI_BV_ROLE ], &type ) == 0 ) {
		struct berval	oc,
				at;

		if ( BER_BVISNULL( &opts ) ) {
			oc = aci_bv[ ACI_BV_ROLE_CLASS ];
			at = aci_bv[ ACI_BV_ROLE_ATTR ];

		} else {
			if ( acl_get_part( &opts, 0, '/', &oc ) < 0 ) {
				assert( 0 );
			}

			if ( acl_get_part( &opts, 1, '/', &at ) < 0 ) {
				at = aci_bv[ ACI_BV_ROLE_ATTR ];
			}
		}

		if ( aci_group_member( &sdn, &oc, &at, op, e, nmatch, matches ) )
		{
			return 1;
		}

	} else if ( ber_bvcmp( &aci_bv[ ACI_BV_SET ], &type ) == 0 ) {
		if ( acl_match_set( &sdn, op, e, NULL ) ) {
			return 1;
		}

	} else if ( ber_bvcmp( &aci_bv[ ACI_BV_SET_REF ], &type ) == 0 ) {
		if ( acl_match_set( &sdn, op, e, (struct berval *)&aci_bv[ ACI_BV_SET_ATTR ] ) ) {
			return 1;
		}

	} else {
		/* it passed normalization! */
		assert( 0 );
	}

	return 0;
}

static int
aci_init( void )
{
	/* OpenLDAP eXperimental Syntax */
	static slap_syntax_defs_rec aci_syntax_def = {
		"( 1.3.6.1.4.1.4203.666.2.1 DESC 'OpenLDAP Experimental ACI' )",
			SLAP_SYNTAX_HIDE,
			NULL,
			OpenLDAPaciValidate,
			OpenLDAPaciPretty
	};
	static slap_mrule_defs_rec aci_mr_def = {
		"( 1.3.6.1.4.1.4203.666.4.2 NAME 'OpenLDAPaciMatch' "
			"SYNTAX 1.3.6.1.4.1.4203.666.2.1 )",
			SLAP_MR_HIDE | SLAP_MR_EQUALITY, NULL,
			NULL, OpenLDAPaciNormalize, OpenLDAPaciMatch,
			NULL, NULL,
			NULL
	};
	static struct {
		char			*name;
		char			*desc;
		slap_mask_t		flags;
		AttributeDescription	**ad;
	}		aci_at = {
		"OpenLDAPaci", "( 1.3.6.1.4.1.4203.666.1.5 "
			"NAME 'OpenLDAPaci' "
			"DESC 'OpenLDAP access control information (experimental)' "
			"EQUALITY OpenLDAPaciMatch "
			"SYNTAX 1.3.6.1.4.1.4203.666.2.1 "
			"USAGE directoryOperation )",
		SLAP_AT_HIDE,
		&slap_ad_aci
	};

	int			rc;

	/* ACI syntax */
	rc = register_syntax( &aci_syntax_def );
	if ( rc != 0 ) {
		return rc;
	}
	
	/* ACI equality rule */
	rc = register_matching_rule( &aci_mr_def );
	if ( rc != 0 ) {
		return rc;
	}

	/* ACI attribute */
	rc = register_at( aci_at.desc, aci_at.ad, 0 );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"aci_init: at_register failed\n", 0, 0, 0 );
		return rc;
	}

	/* install flags */
	(*aci_at.ad)->ad_type->sat_flags |= aci_at.flags;

	return rc;
}

static int
dynacl_aci_parse(
	const char *fname,
	int lineno,
	const char *opts,
	slap_style_t sty,
	const char *right,
	void **privp )
{
	AttributeDescription	*ad = NULL;
	const char		*text = NULL;

	if ( sty != ACL_STYLE_REGEX && sty != ACL_STYLE_BASE ) {
		fprintf( stderr, "%s: line %d: "
			"inappropriate style \"%s\" in \"aci\" by clause\n",
			fname, lineno, style_strings[sty] );
		return -1;
	}

	if ( right != NULL && *right != '\0' ) {
		if ( slap_str2ad( right, &ad, &text ) != LDAP_SUCCESS ) {
			fprintf( stderr,
				"%s: line %d: aci \"%s\": %s\n",
				fname, lineno, right, text );
			return -1;
		}

	} else {
		ad = slap_ad_aci;
	}

	if ( !is_at_syntax( ad->ad_type, SLAPD_ACI_SYNTAX) ) {
		fprintf( stderr, "%s: line %d: "
			"aci \"%s\": inappropriate syntax: %s\n",
			fname, lineno, right,
			ad->ad_type->sat_syntax_oid );
		return -1;
	}

	*privp = (void *)ad;

	return 0;
}

static int
dynacl_aci_unparse( void *priv, struct berval *bv )
{
	AttributeDescription	*ad = ( AttributeDescription * )priv;
	char			*ptr;

	assert( ad != NULL );

	bv->bv_val = ch_malloc( STRLENOF(" aci=") + ad->ad_cname.bv_len + 1 );
	ptr = lutil_strcopy( bv->bv_val, " aci=" );
	ptr = lutil_strcopy( ptr, ad->ad_cname.bv_val );
	bv->bv_len = ptr - bv->bv_val;

	return 0;
}

static int
dynacl_aci_mask(
	void			*priv,
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	int			nmatch,
	regmatch_t		*matches,
	slap_access_t		*grantp,
	slap_access_t		*denyp )
{
	AttributeDescription	*ad = ( AttributeDescription * )priv;
	Attribute		*at;
	slap_access_t		tgrant, tdeny, grant, deny;
#ifdef LDAP_DEBUG
	char			accessmaskbuf[ACCESSMASK_MAXLEN];
	char			accessmaskbuf1[ACCESSMASK_MAXLEN];
#endif /* LDAP_DEBUG */

	if ( BER_BVISEMPTY( &e->e_nname ) ) {
		/* no ACIs in the root DSE */
		return -1;
	}

	/* start out with nothing granted, nothing denied */
	ACL_INIT(tgrant);
	ACL_INIT(tdeny);

	/* get the aci attribute */
	at = attr_find( e->e_attrs, ad );
	if ( at != NULL ) {
		int		i;

		/* the aci is an multi-valued attribute.  The
		 * rights are determined by OR'ing the individual
		 * rights given by the acis.
		 */
		for ( i = 0; !BER_BVISNULL( &at->a_nvals[i] ); i++ ) {
			if ( aci_mask( op, e, desc, val, &at->a_nvals[i],
					nmatch, matches, &grant, &deny,
					SLAP_ACI_SCOPE_ENTRY ) != 0 )
			{
				tgrant |= grant;
				tdeny |= deny;
			}
		}
		
		Debug( LDAP_DEBUG_ACL, "        <= aci_mask grant %s deny %s\n",
			  accessmask2str( tgrant, accessmaskbuf, 1 ), 
			  accessmask2str( tdeny, accessmaskbuf1, 1 ), 0 );
	}

	/* If the entry level aci didn't contain anything valid for the 
	 * current operation, climb up the tree and evaluate the
	 * acis with scope set to subtree
	 */
	if ( tgrant == ACL_PRIV_NONE && tdeny == ACL_PRIV_NONE ) {
		struct berval	parent_ndn;

		dnParent( &e->e_nname, &parent_ndn );
		while ( !BER_BVISEMPTY( &parent_ndn ) ){
			int		i;
			BerVarray	bvals = NULL;
			int		ret, stop;

			/* to solve the chicken'n'egg problem of accessing
			 * the OpenLDAPaci attribute, the direct access
			 * to the entry's attribute is unchecked; however,
			 * further accesses to OpenLDAPaci values in the 
			 * ancestors occur through backend_attribute(), i.e.
			 * with the identity of the operation, requiring
			 * further access checking.  For uniformity, this
			 * makes further requests occur as the rootdn, if
			 * any, i.e. searching for the OpenLDAPaci attribute
			 * is considered an internal search.  If this is not
			 * acceptable, then the same check needs be performed
			 * when accessing the entry's attribute. */
			struct berval	save_o_dn, save_o_ndn;
	
			if ( !BER_BVISNULL( &op->o_bd->be_rootndn ) ) {
				save_o_dn = op->o_dn;
				save_o_ndn = op->o_ndn;

				op->o_dn = op->o_bd->be_rootdn;
				op->o_ndn = op->o_bd->be_rootndn;
			}

			Debug( LDAP_DEBUG_ACL, "        checking ACI of \"%s\"\n", parent_ndn.bv_val, 0, 0 );
			ret = backend_attribute( op, NULL, &parent_ndn, ad, &bvals, ACL_AUTH );

			if ( !BER_BVISNULL( &op->o_bd->be_rootndn ) ) {
				op->o_dn = save_o_dn;
				op->o_ndn = save_o_ndn;
			}

			switch ( ret ) {
			case LDAP_SUCCESS :
				stop = 0;
				if ( !bvals ) {
					break;
				}

				for ( i = 0; !BER_BVISNULL( &bvals[i] ); i++ ) {
					if ( aci_mask( op, e, desc, val,
							&bvals[i],
							nmatch, matches,
							&grant, &deny,
							SLAP_ACI_SCOPE_CHILDREN ) != 0 )
					{
						tgrant |= grant;
						tdeny |= deny;
						/* evaluation stops as soon as either a "deny" or a 
						 * "grant" directive matches.
						 */
						if ( tgrant != ACL_PRIV_NONE || tdeny != ACL_PRIV_NONE ) {
							stop = 1;
						}
					}
					Debug( LDAP_DEBUG_ACL, "<= aci_mask grant %s deny %s\n", 
						accessmask2str( tgrant, accessmaskbuf, 1 ),
						accessmask2str( tdeny, accessmaskbuf1, 1 ), 0 );
				}
				break;

			case LDAP_NO_SUCH_ATTRIBUTE:
				/* just go on if the aci-Attribute is not present in
				 * the current entry 
				 */
				Debug( LDAP_DEBUG_ACL, "no such attribute\n", 0, 0, 0 );
				stop = 0;
				break;

			case LDAP_NO_SUCH_OBJECT:
				/* We have reached the base object */
				Debug( LDAP_DEBUG_ACL, "no such object\n", 0, 0, 0 );
				stop = 1;
				break;

			default:
				stop = 1;
				break;
			}

			if ( stop ) {
				break;
			}
			dnParent( &parent_ndn, &parent_ndn );
		}
	}

	*grantp = tgrant;
	*denyp = tdeny;

	return 0;
}

/* need to register this at some point */
static slap_dynacl_t	dynacl_aci = {
	"aci",
	dynacl_aci_parse,
	dynacl_aci_unparse,
	dynacl_aci_mask,
	NULL,
	NULL,
	NULL
};

int
dynacl_aci_init( void )
{
	int	rc;

	rc = aci_init();

	if ( rc == 0 ) {
		rc = slap_dynacl_register( &dynacl_aci );
	}
	
	return rc;
}


/* ACI syntax validation */

/*
 * Matches given berval to array of bervals
 * Returns:
 *      >=0 if one if the array elements equals to this berval
 *       -1 if string was not found in array
 */
static int 
bv_getcaseidx(
	struct berval *bv, 
	const struct berval *arr[] )
{
	int i;

	if ( BER_BVISEMPTY( bv ) ) {
		return -1;
	}

	for ( i = 0; arr[ i ] != NULL ; i++ ) {
		if ( ber_bvstrcasecmp( bv, arr[ i ] ) == 0 ) {
 			return i;
		}
	}

  	return -1;
}


/* Returns what have left in input berval after current sub */
static void
bv_get_tail(
	struct berval *val,
	struct berval *sub,
	struct berval *tail )
{
	int		head_len;

	tail->bv_val = sub->bv_val + sub->bv_len;
	head_len = (unsigned long) tail->bv_val - (unsigned long) val->bv_val;
  	tail->bv_len = val->bv_len - head_len;
}


/*
 * aci is accepted in following form:
 *    oid#scope#rights#type#subject
 * Where:
 *    oid       := numeric OID (currently ignored)
 *    scope     := entry|children|subtree
 *    rights    := right[[$right]...]
 *    right     := (grant|deny);action
 *    action    := perms;attrs[[;perms;attrs]...]
 *    perms     := perm[[,perm]...]
 *    perm      := c|s|r|w|x
 *    attrs     := attribute[[,attribute]..]|"[all]"
 *    attribute := attributeType|attributeType=attributeValue|attributeType=attributeValuePrefix*
 *    type      := public|users|self|dnattr|group|role|set|set-ref|
 *                 access_id|subtree|onelevel|children
 */
static int 
OpenLDAPaciValidatePerms(
	struct berval *perms ) 
{
	ber_len_t	i;

	for ( i = 0; i < perms->bv_len; ) {
		switch ( perms->bv_val[ i ] ) {
		case 'x':
		case 'd':
		case 'c':
		case 's':
		case 'r':
		case 'w':
			break;

		default:
		        Debug( LDAP_DEBUG_ACL, "aciValidatePerms: perms needs to be one of x,d,c,s,r,w in '%s'\n", perms->bv_val, 0, 0 );
			return LDAP_INVALID_SYNTAX;
		}

		if ( ++i == perms->bv_len ) {
			return LDAP_SUCCESS;
		}

		while ( i < perms->bv_len && perms->bv_val[ i ] == ' ' )
			i++;

		assert( i != perms->bv_len );

		if ( perms->bv_val[ i ] != ',' ) {
		        Debug( LDAP_DEBUG_ACL, "aciValidatePerms: missing comma in '%s'\n", perms->bv_val, 0, 0 );
			return LDAP_INVALID_SYNTAX;
		}

		do {
			i++;
		} while ( perms->bv_val[ i ] == ' ' );
	}

	return LDAP_SUCCESS;
}

static const struct berval *ACIgrantdeny[] = {
	&aci_bv[ ACI_BV_GRANT ],
	&aci_bv[ ACI_BV_DENY ],
	NULL
};

static int 
OpenLDAPaciValidateRight(
	struct berval *action )
{
	struct berval	bv = BER_BVNULL;
	int		i;

	/* grant|deny */
	if ( acl_get_part( action, 0, ';', &bv ) < 0 ||
		bv_getcaseidx( &bv, ACIgrantdeny ) == -1 )
	{
		Debug( LDAP_DEBUG_ACL, "aciValidateRight: '%s' must be either 'grant' or 'deny'\n", bv.bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}

	for ( i = 0; acl_get_part( action, i + 1, ';', &bv ) >= 0; i++ ) {
		if ( i & 1 ) {
			/* perms */
			if ( OpenLDAPaciValidatePerms( &bv ) != LDAP_SUCCESS )
			{
				return LDAP_INVALID_SYNTAX;
			}

		} else {
			/* attr */
			AttributeDescription	*ad;
			const char		*text;
			struct berval		attr, left, right;
			int			j;

			/* could be "[all]" or an attribute description */
			if ( ber_bvstrcasecmp( &bv, &aci_bv[ ACI_BV_BR_ALL ] ) == 0 ) {
				continue;
			}


			for ( j = 0; acl_get_part( &bv, j, ',', &attr ) >= 0; j++ ) 
			{
				ad = NULL;
				text = NULL;
				if ( acl_get_part( &attr, 0, '=', &left ) < 0
					|| acl_get_part( &attr, 1, '=', &right ) < 0 ) 
				{
					if ( slap_bv2ad( &attr, &ad, &text ) != LDAP_SUCCESS ) 
					{
						Debug( LDAP_DEBUG_ACL, "aciValidateRight: unknown attribute: '%s'\n", attr.bv_val, 0, 0 );
						return LDAP_INVALID_SYNTAX;
					}
				} else {
					if ( slap_bv2ad( &left, &ad, &text ) != LDAP_SUCCESS ) 
					{
						Debug( LDAP_DEBUG_ACL, "aciValidateRight: unknown attribute: '%s'\n", left.bv_val, 0, 0 );
						return LDAP_INVALID_SYNTAX;
					}
				}
			}
		}
	}
	
	/* "perms;attr" go in pairs */
	if ( i > 0 && ( i & 1 ) == 0 ) {
		return LDAP_SUCCESS;

	} else {
		Debug( LDAP_DEBUG_ACL, "aciValidateRight: perms:attr need to be pairs in '%s'\n", action->bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}

	return LDAP_SUCCESS;
}

static int
OpenLDAPaciNormalizeRight(
	struct berval	*action,
	struct berval	*naction,
	void		*ctx )
{
	struct berval	grantdeny,
			perms = BER_BVNULL,
			bv = BER_BVNULL;
	int		idx,
			i;

	/* grant|deny */
	if ( acl_get_part( action, 0, ';', &grantdeny ) < 0 ) {
	        Debug( LDAP_DEBUG_ACL, "aciNormalizeRight: missing ';' in '%s'\n", action->bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}
	idx = bv_getcaseidx( &grantdeny, ACIgrantdeny );
	if ( idx == -1 ) {
	        Debug( LDAP_DEBUG_ACL, "aciNormalizeRight: '%s' must be grant or deny\n", grantdeny.bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}

	ber_dupbv_x( naction, (struct berval *)ACIgrantdeny[ idx ], ctx );

	for ( i = 1; acl_get_part( action, i, ';', &bv ) >= 0; i++ ) {
		struct berval	nattrs = BER_BVNULL;
		int		freenattrs = 1;
		if ( i & 1 ) {
			/* perms */
			if ( OpenLDAPaciValidatePerms( &bv ) != LDAP_SUCCESS )
			{
				return LDAP_INVALID_SYNTAX;
			}
			perms = bv;

		} else {
			/* attr */
			char		*ptr;

			/* could be "[all]" or an attribute description */
			if ( ber_bvstrcasecmp( &bv, &aci_bv[ ACI_BV_BR_ALL ] ) == 0 ) {
				nattrs = aci_bv[ ACI_BV_BR_ALL ];
				freenattrs = 0;

			} else {
				AttributeDescription	*ad = NULL;
				AttributeDescription	adstatic= { 0 };
				const char		*text = NULL;
				struct berval		attr, left, right;
				int			j;
				int			len;

				for ( j = 0; acl_get_part( &bv, j, ',', &attr ) >= 0; j++ ) 
				{
					ad = NULL;
					text = NULL;
					/* openldap 2.1 aci compabitibility [entry] -> entry */
					if ( ber_bvstrcasecmp( &attr, &aci_bv[ ACI_BV_BR_ENTRY ] ) == 0 ) {
						ad = &adstatic;
						adstatic.ad_cname = aci_bv[ ACI_BV_ENTRY ];

					/* openldap 2.1 aci compabitibility [children] -> children */
					} else if ( ber_bvstrcasecmp( &attr, &aci_bv[ ACI_BV_BR_CHILDREN ] ) == 0 ) {
						ad = &adstatic;
						adstatic.ad_cname = aci_bv[ ACI_BV_CHILDREN ];

					/* openldap 2.1 aci compabitibility [all] -> only [all] */
					} else if ( ber_bvstrcasecmp( &attr, &aci_bv[ ACI_BV_BR_ALL ] ) == 0 ) {
						ber_memfree_x( nattrs.bv_val, ctx );
						nattrs = aci_bv[ ACI_BV_BR_ALL ];
						freenattrs = 0;
						break;

					} else if ( acl_get_part( &attr, 0, '=', &left ) < 0
				     		|| acl_get_part( &attr, 1, '=', &right ) < 0 ) 
					{
						if ( slap_bv2ad( &attr, &ad, &text ) != LDAP_SUCCESS ) 
						{
							ber_memfree_x( nattrs.bv_val, ctx );
							Debug( LDAP_DEBUG_ACL, "aciNormalizeRight: unknown attribute: '%s'\n", attr.bv_val, 0, 0 );
							return LDAP_INVALID_SYNTAX;
						}

					} else {
						if ( slap_bv2ad( &left, &ad, &text ) != LDAP_SUCCESS ) 
						{
							ber_memfree_x( nattrs.bv_val, ctx );
							Debug( LDAP_DEBUG_ACL, "aciNormalizeRight: unknown attribute: '%s'\n", left.bv_val, 0, 0 );
							return LDAP_INVALID_SYNTAX;
						}
					}
					
				
					len = nattrs.bv_len + ( !BER_BVISEMPTY( &nattrs ) ? STRLENOF( "," ) : 0 )
				      		+ ad->ad_cname.bv_len;
					nattrs.bv_val = ber_memrealloc_x( nattrs.bv_val, len + 1, ctx );
	                        	ptr = &nattrs.bv_val[ nattrs.bv_len ];
					if ( !BER_BVISEMPTY( &nattrs ) ) {
						*ptr++ = ',';
					}
					ptr = lutil_strncopy( ptr, ad->ad_cname.bv_val, ad->ad_cname.bv_len );
                                	ptr[ 0 ] = '\0';
                                	nattrs.bv_len = len;
				}

			}

			naction->bv_val = ber_memrealloc_x( naction->bv_val,
				naction->bv_len + STRLENOF( ";" )
				+ perms.bv_len + STRLENOF( ";" )
				+ nattrs.bv_len + 1,
				ctx );

			ptr = &naction->bv_val[ naction->bv_len ];
			ptr[ 0 ] = ';';
			ptr++;
			ptr = lutil_strncopy( ptr, perms.bv_val, perms.bv_len );
			ptr[ 0 ] = ';';
			ptr++;
			ptr = lutil_strncopy( ptr, nattrs.bv_val, nattrs.bv_len );
			ptr[ 0 ] = '\0';
			naction->bv_len += STRLENOF( ";" ) + perms.bv_len
				+ STRLENOF( ";" ) + nattrs.bv_len;
			if ( freenattrs ) {
				ber_memfree_x( nattrs.bv_val, ctx );
			}
		}
	}
	
	/* perms;attr go in pairs */
	if ( i > 1 && ( i & 1 ) ) {
		return LDAP_SUCCESS;

	} else {
		Debug( LDAP_DEBUG_ACL, "aciNormalizeRight: perms:attr need to be pairs in '%s'\n", action->bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}
}

static int 
OpenLDAPaciValidateRights(
	struct berval *actions )

{
	struct berval	bv = BER_BVNULL;
	int		i;

	for ( i = 0; acl_get_part( actions, i, '$', &bv ) >= 0; i++ ) {
		if ( OpenLDAPaciValidateRight( &bv ) != LDAP_SUCCESS ) {
			return LDAP_INVALID_SYNTAX;
		}
	}

	return LDAP_SUCCESS;
}

static int 
OpenLDAPaciNormalizeRights(
	struct berval	*actions,
	struct berval	*nactions,
	void		*ctx )

{
	struct berval	bv = BER_BVNULL;
	int		i;

	BER_BVZERO( nactions );
	for ( i = 0; acl_get_part( actions, i, '$', &bv ) >= 0; i++ ) {
		int		rc;
		struct berval	nbv;

		rc = OpenLDAPaciNormalizeRight( &bv, &nbv, ctx );
		if ( rc != LDAP_SUCCESS ) {
			ber_memfree_x( nactions->bv_val, ctx );
			BER_BVZERO( nactions );
			return LDAP_INVALID_SYNTAX;
		}

		if ( i == 0 ) {
			*nactions = nbv;

		} else {
			nactions->bv_val = ber_memrealloc_x( nactions->bv_val,
				nactions->bv_len + STRLENOF( "$" )
				+ nbv.bv_len + 1,
				ctx );
			nactions->bv_val[ nactions->bv_len ] = '$';
			AC_MEMCPY( &nactions->bv_val[ nactions->bv_len + 1 ],
				nbv.bv_val, nbv.bv_len + 1 );
			ber_memfree_x( nbv.bv_val, ctx );
			nactions->bv_len += STRLENOF( "$" ) + nbv.bv_len;
		}
		BER_BVZERO( &nbv );
	}

	return LDAP_SUCCESS;
}

static const struct berval *OpenLDAPaciscopes[] = {
	&aci_bv[ ACI_BV_ENTRY ],
	&aci_bv[ ACI_BV_CHILDREN ],
	&aci_bv[ ACI_BV_SUBTREE ],

	NULL
};

static const struct berval *OpenLDAPacitypes[] = {
	/* DN-valued */
	&aci_bv[ ACI_BV_GROUP ], 
	&aci_bv[ ACI_BV_ROLE ],

/* set to one past the last DN-valued type with options (/) */
#define	LAST_OPTIONAL	2

	&aci_bv[ ACI_BV_ACCESS_ID ],
	&aci_bv[ ACI_BV_SUBTREE ],
	&aci_bv[ ACI_BV_ONELEVEL ],
	&aci_bv[ ACI_BV_CHILDREN ],

/* set to one past the last DN-valued type */
#define LAST_DNVALUED	6

	/* non DN-valued */
	&aci_bv[ ACI_BV_DNATTR ],
	&aci_bv[ ACI_BV_PUBLIC ],
	&aci_bv[ ACI_BV_USERS ],
	&aci_bv[ ACI_BV_SELF ],
	&aci_bv[ ACI_BV_SET ],
	&aci_bv[ ACI_BV_SET_REF ],

	NULL
};

static int
OpenLDAPaciValidate(
	Syntax		*syntax,
	struct berval	*val )
{
	struct berval	oid = BER_BVNULL,
			scope = BER_BVNULL,
			rights = BER_BVNULL,
			type = BER_BVNULL,
			subject = BER_BVNULL;
	int		idx;
	int		rc;
	
	if ( BER_BVISEMPTY( val ) ) {
		Debug( LDAP_DEBUG_ACL, "aciValidatet: value is empty\n", 0, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}

	/* oid */
	if ( acl_get_part( val, 0, '#', &oid ) < 0 || 
		numericoidValidate( NULL, &oid ) != LDAP_SUCCESS )
	{
		/* NOTE: the numericoidValidate() is rather pedantic;
		 * I'd replace it with X-ORDERED VALUES so that
		 * it's guaranteed values are maintained and used
		 * in the desired order */
		Debug( LDAP_DEBUG_ACL, "aciValidate: invalid oid '%s'\n", oid.bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}

	/* scope */
	if ( acl_get_part( val, 1, '#', &scope ) < 0 || 
		bv_getcaseidx( &scope, OpenLDAPaciscopes ) == -1 )
	{
		Debug( LDAP_DEBUG_ACL, "aciValidate: invalid scope '%s'\n", scope.bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}

	/* rights */
	if ( acl_get_part( val, 2, '#', &rights ) < 0 ||
		OpenLDAPaciValidateRights( &rights ) != LDAP_SUCCESS ) 
	{
		return LDAP_INVALID_SYNTAX;
	}

	/* type */
	if ( acl_get_part( val, 3, '#', &type ) < 0 ) {
		Debug( LDAP_DEBUG_ACL, "aciValidate: missing type in '%s'\n", val->bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}
	idx = bv_getcaseidx( &type, OpenLDAPacitypes );
	if ( idx == -1 ) {
		struct berval	isgr;

		if ( acl_get_part( &type, 0, '/', &isgr ) < 0 ) {
			Debug( LDAP_DEBUG_ACL, "aciValidate: invalid type '%s'\n", type.bv_val, 0, 0 );
			return LDAP_INVALID_SYNTAX;
		}

		idx = bv_getcaseidx( &isgr, OpenLDAPacitypes );
		if ( idx == -1 || idx >= LAST_OPTIONAL ) {
			Debug( LDAP_DEBUG_ACL, "aciValidate: invalid type '%s'\n", isgr.bv_val, 0, 0 );
			return LDAP_INVALID_SYNTAX;
		}
	}

	/* subject */
	bv_get_tail( val, &type, &subject );
	if ( subject.bv_val[ 0 ] != '#' ) {
		Debug( LDAP_DEBUG_ACL, "aciValidate: missing subject in '%s'\n", val->bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}

	if ( idx >= LAST_DNVALUED ) {
		if ( OpenLDAPacitypes[ idx ] == &aci_bv[ ACI_BV_DNATTR ] ) {
			AttributeDescription	*ad = NULL;
			const char		*text = NULL;

			rc = slap_bv2ad( &subject, &ad, &text );
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ACL, "aciValidate: unknown dn attribute '%s'\n", subject.bv_val, 0, 0 );
				return LDAP_INVALID_SYNTAX;
			}

			if ( ad->ad_type->sat_syntax != slap_schema.si_syn_distinguishedName ) {
				/* FIXME: allow nameAndOptionalUID? */
				Debug( LDAP_DEBUG_ACL, "aciValidate: wrong syntax for dn attribute '%s'\n", subject.bv_val, 0, 0 );
				return LDAP_INVALID_SYNTAX;
			}
		}

		/* not a DN */
		return LDAP_SUCCESS;

	} else if ( OpenLDAPacitypes[ idx ] == &aci_bv[ ACI_BV_GROUP ]
			|| OpenLDAPacitypes[ idx ] == &aci_bv[ ACI_BV_ROLE ] )
	{
		/* do {group|role}/oc/at check */
		struct berval	ocbv = BER_BVNULL,
				atbv = BER_BVNULL;

		ocbv.bv_val = ber_bvchr( &type, '/' );
		if ( ocbv.bv_val != NULL ) {
			ocbv.bv_val++;
			ocbv.bv_len = type.bv_len
					- ( ocbv.bv_val - type.bv_val );

			atbv.bv_val = ber_bvchr( &ocbv, '/' );
			if ( atbv.bv_val != NULL ) {
				AttributeDescription	*ad = NULL;
				const char		*text = NULL;
				int			rc;

				atbv.bv_val++;
				atbv.bv_len = type.bv_len
					- ( atbv.bv_val - type.bv_val );
				ocbv.bv_len = atbv.bv_val - ocbv.bv_val - 1;

				rc = slap_bv2ad( &atbv, &ad, &text );
				if ( rc != LDAP_SUCCESS ) {
				        Debug( LDAP_DEBUG_ACL, "aciValidate: unknown group attribute '%s'\n", atbv.bv_val, 0, 0 );
					return LDAP_INVALID_SYNTAX;
				}
			}

			if ( oc_bvfind( &ocbv ) == NULL ) {
			        Debug( LDAP_DEBUG_ACL, "aciValidate: unknown group '%s'\n", ocbv.bv_val, 0, 0 );
				return LDAP_INVALID_SYNTAX;
			}
		}
	}

	if ( BER_BVISEMPTY( &subject ) ) {
		/* empty DN invalid */
	        Debug( LDAP_DEBUG_ACL, "aciValidate: missing dn in '%s'\n", val->bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}

	subject.bv_val++;
	subject.bv_len--;

	/* FIXME: pass DN syntax? */
	rc = dnValidate( NULL, &subject );
	if ( rc != LDAP_SUCCESS ) {
	        Debug( LDAP_DEBUG_ACL, "aciValidate: invalid dn '%s'\n", subject.bv_val, 0, 0 );
	}
	return rc;
}

static int
OpenLDAPaciPrettyNormal(
	struct berval	*val,
	struct berval	*out,
	void		*ctx,
	int		normalize )
{
	struct berval	oid = BER_BVNULL,
			scope = BER_BVNULL,
			rights = BER_BVNULL,
			nrights = BER_BVNULL,
			type = BER_BVNULL,
			ntype = BER_BVNULL,
			subject = BER_BVNULL,
			nsubject = BER_BVNULL;
	int		idx,
			rc = LDAP_SUCCESS,
			freesubject = 0,
			freetype = 0;
	char		*ptr;

	BER_BVZERO( out );

	if ( BER_BVISEMPTY( val ) ) {
		Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: value is empty\n", 0, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}

	/* oid: if valid, it's already normalized */
	if ( acl_get_part( val, 0, '#', &oid ) < 0 || 
		numericoidValidate( NULL, &oid ) != LDAP_SUCCESS )
	{
		Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: invalid oid '%s'\n", oid.bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}

	/* scope: normalize by replacing with OpenLDAPaciscopes */
	if ( acl_get_part( val, 1, '#', &scope ) < 0 ) {
		Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: missing scope in '%s'\n", val->bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}
	idx = bv_getcaseidx( &scope, OpenLDAPaciscopes );
	if ( idx == -1 ) {
		Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: invalid scope '%s'\n", scope.bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}
	scope = *OpenLDAPaciscopes[ idx ];

	/* rights */
	if ( acl_get_part( val, 2, '#', &rights ) < 0 ) {
		Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: missing rights in '%s'\n", val->bv_val, 0, 0 );
		return LDAP_INVALID_SYNTAX;
	}
	if ( OpenLDAPaciNormalizeRights( &rights, &nrights, ctx )
		!= LDAP_SUCCESS ) 
	{
		return LDAP_INVALID_SYNTAX;
	}

	/* type */
	if ( acl_get_part( val, 3, '#', &type ) < 0 ) {
		Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: missing type in '%s'\n", val->bv_val, 0, 0 );
		rc = LDAP_INVALID_SYNTAX;
		goto cleanup;
	}
	idx = bv_getcaseidx( &type, OpenLDAPacitypes );
	if ( idx == -1 ) {
		struct berval	isgr;

		if ( acl_get_part( &type, 0, '/', &isgr ) < 0 ) {
		        Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: invalid type '%s'\n", type.bv_val, 0, 0 );
			rc = LDAP_INVALID_SYNTAX;
			goto cleanup;
		}

		idx = bv_getcaseidx( &isgr, OpenLDAPacitypes );
		if ( idx == -1 || idx >= LAST_OPTIONAL ) {
		        Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: invalid type '%s'\n", isgr.bv_val, 0, 0 );
			rc = LDAP_INVALID_SYNTAX;
			goto cleanup;
		}
	}
	ntype = *OpenLDAPacitypes[ idx ];

	/* subject */
	bv_get_tail( val, &type, &subject );

	if ( BER_BVISEMPTY( &subject ) || subject.bv_val[ 0 ] != '#' ) {
	        Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: missing subject in '%s'\n", val->bv_val, 0, 0 );
		rc = LDAP_INVALID_SYNTAX;
		goto cleanup;
	}

	subject.bv_val++;
	subject.bv_len--;

	if ( idx < LAST_DNVALUED ) {
		/* FIXME: pass DN syntax? */
		if ( normalize ) {
			rc = dnNormalize( 0, NULL, NULL,
				&subject, &nsubject, ctx );
		} else {
			rc = dnPretty( NULL, &subject, &nsubject, ctx );
		}

		if ( rc == LDAP_SUCCESS ) {
			freesubject = 1;

		} else {
	                Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: invalid subject dn '%s'\n", subject.bv_val, 0, 0 );
			goto cleanup;
		}

		if ( OpenLDAPacitypes[ idx ] == &aci_bv[ ACI_BV_GROUP ]
			|| OpenLDAPacitypes[ idx ] == &aci_bv[ ACI_BV_ROLE ] )
		{
			/* do {group|role}/oc/at check */
			struct berval	ocbv = BER_BVNULL,
					atbv = BER_BVNULL;

			ocbv.bv_val = ber_bvchr( &type, '/' );
			if ( ocbv.bv_val != NULL ) {
				ObjectClass		*oc = NULL;
				AttributeDescription	*ad = NULL;
				const char		*text = NULL;
				int			rc;
				struct berval		bv;

				bv.bv_len = ntype.bv_len;

				ocbv.bv_val++;
				ocbv.bv_len = type.bv_len - ( ocbv.bv_val - type.bv_val );

				atbv.bv_val = ber_bvchr( &ocbv, '/' );
				if ( atbv.bv_val != NULL ) {
					atbv.bv_val++;
					atbv.bv_len = type.bv_len
						- ( atbv.bv_val - type.bv_val );
					ocbv.bv_len = atbv.bv_val - ocbv.bv_val - 1;
	
					rc = slap_bv2ad( &atbv, &ad, &text );
					if ( rc != LDAP_SUCCESS ) {
	                                        Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: unknown group attribute '%s'\n", atbv.bv_val, 0, 0 );
						rc = LDAP_INVALID_SYNTAX;
						goto cleanup;
					}

					bv.bv_len += STRLENOF( "/" ) + ad->ad_cname.bv_len;
				}

				oc = oc_bvfind( &ocbv );
				if ( oc == NULL ) {
                                        Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: invalid group '%s'\n", ocbv.bv_val, 0, 0 );
					rc = LDAP_INVALID_SYNTAX;
					goto cleanup;
				}

				bv.bv_len += STRLENOF( "/" ) + oc->soc_cname.bv_len;
				bv.bv_val = ber_memalloc_x( bv.bv_len + 1, ctx );

				ptr = bv.bv_val;
				ptr = lutil_strncopy( ptr, ntype.bv_val, ntype.bv_len );
				ptr[ 0 ] = '/';
				ptr++;
				ptr = lutil_strncopy( ptr,
					oc->soc_cname.bv_val,
					oc->soc_cname.bv_len );
				if ( ad != NULL ) {
					ptr[ 0 ] = '/';
					ptr++;
					ptr = lutil_strncopy( ptr,
						ad->ad_cname.bv_val,
						ad->ad_cname.bv_len );
				}
				ptr[ 0 ] = '\0';

				ntype = bv;
				freetype = 1;
			}
		}

	} else if ( OpenLDAPacitypes[ idx ] == &aci_bv[ ACI_BV_DNATTR ] ) {
		AttributeDescription	*ad = NULL;
		const char		*text = NULL;
		int			rc;

		rc = slap_bv2ad( &subject, &ad, &text );
		if ( rc != LDAP_SUCCESS ) {
                        Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: unknown dn attribute '%s'\n", subject.bv_val, 0, 0 );
			rc = LDAP_INVALID_SYNTAX;
			goto cleanup;
		}

		if ( ad->ad_type->sat_syntax != slap_schema.si_syn_distinguishedName ) {
			/* FIXME: allow nameAndOptionalUID? */
                        Debug( LDAP_DEBUG_ACL, "aciPrettyNormal: wrong syntax for dn attribute '%s'\n", subject.bv_val, 0, 0 );
			rc = LDAP_INVALID_SYNTAX;
			goto cleanup;
		}

		nsubject = ad->ad_cname;

	} else if ( OpenLDAPacitypes[ idx ] == &aci_bv[ ACI_BV_SET ]
		|| OpenLDAPacitypes[ idx ] == &aci_bv[ ACI_BV_SET_REF ] )
	{
		/* NOTE: dunno how to normalize it... */
		nsubject = subject;
	}


	out->bv_len = 
		oid.bv_len + STRLENOF( "#" )
		+ scope.bv_len + STRLENOF( "#" )
		+ nrights.bv_len + STRLENOF( "#" )
		+ ntype.bv_len + STRLENOF( "#" )
		+ nsubject.bv_len;

	out->bv_val = ber_memalloc_x( out->bv_len + 1, ctx );
	ptr = lutil_strncopy( out->bv_val, oid.bv_val, oid.bv_len );
	ptr[ 0 ] = '#';
	ptr++;
	ptr = lutil_strncopy( ptr, scope.bv_val, scope.bv_len );
	ptr[ 0 ] = '#';
	ptr++;
	ptr = lutil_strncopy( ptr, nrights.bv_val, nrights.bv_len );
	ptr[ 0 ] = '#';
	ptr++;
	ptr = lutil_strncopy( ptr, ntype.bv_val, ntype.bv_len );
	ptr[ 0 ] = '#';
	ptr++;
	if ( !BER_BVISNULL( &nsubject ) ) {
		ptr = lutil_strncopy( ptr, nsubject.bv_val, nsubject.bv_len );
	}
	ptr[ 0 ] = '\0';

cleanup:;
	if ( freesubject ) {
		ber_memfree_x( nsubject.bv_val, ctx );
	}

	if ( freetype ) {
		ber_memfree_x( ntype.bv_val, ctx );
	}

	if ( !BER_BVISNULL( &nrights ) ) {
		ber_memfree_x( nrights.bv_val, ctx );
	}

	return rc;
}

static int
OpenLDAPaciPretty(
	Syntax		*syntax,
	struct berval	*val,
	struct berval	*out,
	void		*ctx )
{
	return OpenLDAPaciPrettyNormal( val, out, ctx, 0 );
}

static int
OpenLDAPaciNormalize(
	slap_mask_t	use,
	Syntax		*syntax,
	MatchingRule	*mr,
	struct berval	*val,
	struct berval	*out,
	void		*ctx )
{
	return OpenLDAPaciPrettyNormal( val, out, ctx, 1 );
}

#if SLAPD_ACI_ENABLED == SLAPD_MOD_DYNAMIC
/*
 * FIXME: need config and Makefile.am code to ease building
 * as dynamic module
 */
int
init_module( int argc, char *argv[] )
{
	return dynacl_aci_init();
}
#endif /* SLAPD_ACI_ENABLED == SLAPD_MOD_DYNAMIC */

#endif /* SLAPD_ACI_ENABLED */

