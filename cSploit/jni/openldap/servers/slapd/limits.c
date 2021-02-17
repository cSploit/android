/* limits.c - routines to handle regex-based size and time limits */
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

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/regex.h>
#include <ac/string.h>

#include "slap.h"
#include "lutil.h"

/* define to get an error if requesting limit higher than hard */
#undef ABOVE_HARD_LIMIT_IS_ERROR

static const struct berval lmpats[] = {
	BER_BVC( "base" ),
	BER_BVC( "base" ),
	BER_BVC( "onelevel" ),
	BER_BVC( "subtree" ),
	BER_BVC( "children" ),
	BER_BVC( "regex" ),
	BER_BVC( "anonymous" ),
	BER_BVC( "users" ),
	BER_BVC( "*" )
};

#ifdef LDAP_DEBUG
static const char *const dn_source[2] = { "DN", "DN.THIS" };
static const char *const lmpats_out[] = {
	"UNDEFINED",
	"EXACT",
	"ONELEVEL",
	"SUBTREE",
	"CHILDREN",
	"REGEX",
	"ANONYMOUS",
	"USERS",
	"ANY"
};

static const char *
limits2str( unsigned i )
{
	return i < (sizeof( lmpats_out ) / sizeof( lmpats_out[0] ))
		? lmpats_out[i] : "UNKNOWN";
}
#endif /* LDAP_DEBUG */

static int
limits_get( 
	Operation		*op,
	struct slap_limits_set 	**limit
)
{
	static struct berval empty_dn = BER_BVC( "" );
	struct slap_limits **lm;
	struct berval		*ndns[2];

	assert( op != NULL );
	assert( limit != NULL );

	ndns[0] = &op->o_ndn;
	ndns[1] = &op->o_req_ndn;

	Debug( LDAP_DEBUG_TRACE, "==> limits_get: %s self=\"%s\" this=\"%s\"\n",
			op->o_log_prefix,
			BER_BVISNULL( ndns[0] ) ? "[anonymous]" : ndns[0]->bv_val,
			BER_BVISNULL( ndns[1] ) ? "" : ndns[1]->bv_val );
	/*
	 * default values
	 */
	*limit = &op->o_bd->be_def_limit;

	if ( op->o_bd->be_limits == NULL ) {
		return( 0 );
	}

	for ( lm = op->o_bd->be_limits; lm[0] != NULL; lm++ ) {
		unsigned	style = lm[0]->lm_flags & SLAP_LIMITS_MASK;
		unsigned	type = lm[0]->lm_flags & SLAP_LIMITS_TYPE_MASK;
		unsigned	isthis = type == SLAP_LIMITS_TYPE_THIS;
		struct berval *ndn = ndns[isthis];

		if ( style == SLAP_LIMITS_ANY )
			goto found_any;

		if ( BER_BVISEMPTY( ndn ) ) {
			if ( style == SLAP_LIMITS_ANONYMOUS )
				goto found_nodn;
			if ( !isthis )
				continue;
			ndn = &empty_dn;
		}

		switch ( style ) {
		case SLAP_LIMITS_EXACT:
			if ( type == SLAP_LIMITS_TYPE_GROUP ) {
				int	rc = backend_group( op, NULL,
						&lm[0]->lm_pat, ndn,
						lm[0]->lm_group_oc,
						lm[0]->lm_group_ad );
				if ( rc == 0 ) {
					goto found_group;
				}
			} else {
				if ( dn_match( &lm[0]->lm_pat, ndn ) ) {
					goto found_dn;
				}
			}
			break;

		case SLAP_LIMITS_ONE:
		case SLAP_LIMITS_SUBTREE:
		case SLAP_LIMITS_CHILDREN: {
			ber_len_t d;
			
			/* ndn shorter than lm_pat */
			if ( ndn->bv_len < lm[0]->lm_pat.bv_len ) {
				break;
			}
			d = ndn->bv_len - lm[0]->lm_pat.bv_len;

			if ( d == 0 ) {
				/* allow exact match for SUBTREE only */
				if ( style != SLAP_LIMITS_SUBTREE ) {
					break;
				}
			} else {
				/* check for unescaped rdn separator */
				if ( !DN_SEPARATOR( ndn->bv_val[d - 1] ) ) {
					break;
				}
			}

			/* check that ndn ends with lm_pat */
			if ( strcmp( lm[0]->lm_pat.bv_val, &ndn->bv_val[d] ) != 0 ) {
				break;
			}

			/* in case of ONE, require exactly one rdn below lm_pat */
			if ( style == SLAP_LIMITS_ONE ) {
				if ( dn_rdnlen( NULL, ndn ) != d - 1 ) {
					break;
				}
			}

			goto found_dn;
		}

		case SLAP_LIMITS_REGEX:
			if ( regexec( &lm[0]->lm_regex, ndn->bv_val, 0, NULL, 0 ) == 0 ) {
				goto found_dn;
			}
			break;

		case SLAP_LIMITS_ANONYMOUS:
			break;

		case SLAP_LIMITS_USERS:
		found_nodn:
			Debug( LDAP_DEBUG_TRACE, "<== limits_get: type=%s match=%s\n",
				dn_source[isthis], limits2str( style ), 0 );
		found_any:
			*limit = &lm[0]->lm_limits;
			return( 0 );

		found_dn:
			Debug( LDAP_DEBUG_TRACE,
				"<== limits_get: type=%s match=%s dn=\"%s\"\n",
				dn_source[isthis], limits2str( style ), lm[0]->lm_pat.bv_val );
			*limit = &lm[0]->lm_limits;
			return( 0 );

		found_group:
			Debug( LDAP_DEBUG_TRACE, "<== limits_get: type=GROUP match=EXACT "
				"dn=\"%s\" oc=\"%s\" ad=\"%s\"\n",
				lm[0]->lm_pat.bv_val,
				lm[0]->lm_group_oc->soc_cname.bv_val,
				lm[0]->lm_group_ad->ad_cname.bv_val );
			*limit = &lm[0]->lm_limits;
			return( 0 );

		default:
			assert( 0 );	/* unreachable */
			return( -1 );
		}
	}

	return( 0 );
}

static int
limits_add(
	Backend 	        *be,
	unsigned		flags,
	const char		*pattern,
	ObjectClass		*group_oc,
	AttributeDescription	*group_ad,
	struct slap_limits_set	*limit
)
{
	int 			i;
	struct slap_limits	*lm;
	unsigned		type, style;
	
	assert( be != NULL );
	assert( limit != NULL );

	type = flags & SLAP_LIMITS_TYPE_MASK;
	style = flags & SLAP_LIMITS_MASK;

	switch ( style ) {
	case SLAP_LIMITS_ANONYMOUS:
	case SLAP_LIMITS_USERS:
	case SLAP_LIMITS_ANY:
		/* For these styles, type == 0 (SLAP_LIMITS_TYPE_SELF). */
		for ( i = 0; be->be_limits && be->be_limits[ i ]; i++ ) {
			if ( be->be_limits[ i ]->lm_flags == style ) {
				return( -1 );
			}
		}
		break;
	}


	lm = ( struct slap_limits * )ch_calloc( sizeof( struct slap_limits ), 1 );

	switch ( style ) {
	case SLAP_LIMITS_UNDEFINED:
		style = SLAP_LIMITS_EXACT;
		/* continue to next cases */
	case SLAP_LIMITS_EXACT:
	case SLAP_LIMITS_ONE:
	case SLAP_LIMITS_SUBTREE:
	case SLAP_LIMITS_CHILDREN:
		{
			int rc;
			struct berval bv;

			ber_str2bv( pattern, 0, 0, &bv );

			rc = dnNormalize( 0, NULL, NULL, &bv, &lm->lm_pat, NULL );
			if ( rc != LDAP_SUCCESS ) {
				ch_free( lm );
				return( -1 );
			}
		}
		break;
		
	case SLAP_LIMITS_REGEX:
		ber_str2bv( pattern, 0, 1, &lm->lm_pat );
		if ( regcomp( &lm->lm_regex, lm->lm_pat.bv_val, 
					REG_EXTENDED | REG_ICASE ) ) {
			free( lm->lm_pat.bv_val );
			ch_free( lm );
			return( -1 );
		}
		break;

	case SLAP_LIMITS_ANONYMOUS:
	case SLAP_LIMITS_USERS:
	case SLAP_LIMITS_ANY:
		BER_BVZERO( &lm->lm_pat );
		break;
	}

	switch ( type ) {
	case SLAP_LIMITS_TYPE_GROUP:
		assert( group_oc != NULL );
		assert( group_ad != NULL );
		lm->lm_group_oc = group_oc;
		lm->lm_group_ad = group_ad;
		break;
	}

	lm->lm_flags = style | type;
	lm->lm_limits = *limit;

	i = 0;
	if ( be->be_limits != NULL ) {
		for ( ; be->be_limits[i]; i++ );
	}

	be->be_limits = ( struct slap_limits ** )ch_realloc( be->be_limits,
			sizeof( struct slap_limits * ) * ( i + 2 ) );
	be->be_limits[i] = lm;
	be->be_limits[i+1] = NULL;
	
	return( 0 );
}

#define STRSTART( s, m ) (strncasecmp( s, m, STRLENOF( "" m "" )) == 0)

int
limits_parse(
	Backend     *be,
	const char  *fname,
	int         lineno,
	int         argc,
	char        **argv
)
{
	int			flags = SLAP_LIMITS_UNDEFINED;
	char			*pattern;
	struct slap_limits_set 	limit;
	int 			i, rc = 0;
	ObjectClass		*group_oc = NULL;
	AttributeDescription	*group_ad = NULL;

	assert( be != NULL );

	if ( argc < 3 ) {
		Debug( LDAP_DEBUG_ANY,
			"%s : line %d: missing arg(s) in "
			"\"limits <pattern> <limits>\" line.\n%s",
			fname, lineno, "" );
		return( -1 );
	}

	limit = be->be_def_limit;

	/*
	 * syntax:
	 *
	 * "limits" <pattern> <limit> [ ... ]
	 * 
	 * 
	 * <pattern>:
	 * 
	 * "anonymous"
	 * "users"
	 * [ "dn" [ "." { "this" | "self" } ] [ "." { "exact" | "base" |
	 *	"onelevel" | "subtree" | "children" | "regex" | "anonymous" } ]
	 *	"=" ] <dn pattern>
	 *
	 * Note:
	 *	"this" is the baseobject, "self" (the default) is the bound DN
	 *	"exact" and "base" are the same (exact match);
	 *	"onelevel" means exactly one rdn below, NOT including pattern
	 *	"subtree" means any rdn below, including pattern
	 *	"children" means any rdn below, NOT including pattern
	 *	
	 *	"anonymous" may be deprecated in favour 
	 *	of the pattern = "anonymous" form
	 *
	 * "group[/objectClass[/attributeType]]" "=" "<dn pattern>"
	 *
	 * <limit>:
	 *
	 * "time" [ "." { "soft" | "hard" } ] "=" <integer>
	 *
	 * "size" [ "." { "soft" | "hard" | "unchecked" } ] "=" <integer>
	 */
	
	pattern = argv[1];
	if ( strcmp( pattern, "*" ) == 0) {
		flags = SLAP_LIMITS_ANY;

	} else if ( strcasecmp( pattern, "anonymous" ) == 0 ) {
		flags = SLAP_LIMITS_ANONYMOUS;

	} else if ( strcasecmp( pattern, "users" ) == 0 ) {
		flags = SLAP_LIMITS_USERS;
		
	} else if ( STRSTART( pattern, "dn" ) ) {
		pattern += STRLENOF( "dn" );
		flags = SLAP_LIMITS_TYPE_SELF;
		if ( pattern[0] == '.' ) {
			pattern++;
			if ( STRSTART( pattern, "this" ) ) {
				flags = SLAP_LIMITS_TYPE_THIS;
				pattern += STRLENOF( "this" );
			} else if ( STRSTART( pattern, "self" ) ) {
				pattern += STRLENOF( "self" );
			} else {
				goto got_dn_dot;
			}
		}
		if ( pattern[0] == '.' ) {
			pattern++;
		got_dn_dot:
			if ( STRSTART( pattern, "exact" ) ) {
				flags |= SLAP_LIMITS_EXACT;
				pattern += STRLENOF( "exact" );

			} else if ( STRSTART( pattern, "base" ) ) {
				flags |= SLAP_LIMITS_BASE;
				pattern += STRLENOF( "base" );

			} else if ( STRSTART( pattern, "one" ) ) {
				flags |= SLAP_LIMITS_ONE;
				pattern += STRLENOF( "one" );
				if ( STRSTART( pattern, "level" ) ) {
					pattern += STRLENOF( "level" );

				} else {
					Debug( LDAP_DEBUG_ANY,
						"%s : line %d: deprecated \"one\" style "
						"\"limits <pattern> <limits>\" line; "
						"use \"onelevel\" instead.\n", fname, lineno, 0 );
				}

			} else if ( STRSTART( pattern, "sub" ) ) {
				flags |= SLAP_LIMITS_SUBTREE;
				pattern += STRLENOF( "sub" );
				if ( STRSTART( pattern, "tree" ) ) {
					pattern += STRLENOF( "tree" );

				} else {
					Debug( LDAP_DEBUG_ANY,
						"%s : line %d: deprecated \"sub\" style "
						"\"limits <pattern> <limits>\" line; "
						"use \"subtree\" instead.\n", fname, lineno, 0 );
				}

			} else if ( STRSTART( pattern, "children" ) ) {
				flags |= SLAP_LIMITS_CHILDREN;
				pattern += STRLENOF( "children" );

			} else if ( STRSTART( pattern, "regex" ) ) {
				flags |= SLAP_LIMITS_REGEX;
				pattern += STRLENOF( "regex" );

			/* 
			 * this could be deprecated in favour
			 * of the pattern = "anonymous" form
			 */
			} else if ( STRSTART( pattern, "anonymous" )
					&& flags == SLAP_LIMITS_TYPE_SELF )
			{
				flags = SLAP_LIMITS_ANONYMOUS;
				pattern = NULL;

			} else {
				/* force error below */
				if ( *pattern == '=' )
					--pattern;
			}
		}

		/* pre-check the data */
		if ( pattern != NULL ) {
			if ( pattern[0] != '=' ) {
				Debug( LDAP_DEBUG_ANY,
					"%s : line %d: %s in "
					"\"dn[.{this|self}][.{exact|base"
					"|onelevel|subtree|children|regex"
					"|anonymous}]=<pattern>\" in "
					"\"limits <pattern> <limits>\" line.\n",
					fname, lineno,
					isalnum( (unsigned char)pattern[0] )
					? "unknown DN modifier" : "missing '='" );
				return( -1 );
			}

			/* skip '=' (required) */
			pattern++;

			/* trim obvious cases */
			if ( strcmp( pattern, "*" ) == 0 ) {
				flags = SLAP_LIMITS_ANY;
				pattern = NULL;

			} else if ( (flags & SLAP_LIMITS_MASK) == SLAP_LIMITS_REGEX
					&& strcmp( pattern, ".*" ) == 0 ) {
				flags = SLAP_LIMITS_ANY;
				pattern = NULL;
			}
		}

	} else if (STRSTART( pattern, "group" ) ) {
		pattern += STRLENOF( "group" );

		if ( pattern[0] == '/' ) {
			struct berval	oc, ad;

			oc.bv_val = pattern + 1;
			pattern = strchr( pattern, '=' );
			if ( pattern == NULL ) {
				return -1;
			}

			ad.bv_val = strchr( oc.bv_val, '/' );
			if ( ad.bv_val != NULL ) {
				const char	*text = NULL;

				oc.bv_len = ad.bv_val - oc.bv_val;

				ad.bv_val++;
				ad.bv_len = pattern - ad.bv_val;
				rc = slap_bv2ad( &ad, &group_ad, &text );
				if ( rc != LDAP_SUCCESS ) {
					goto no_ad;
				}

			} else {
				oc.bv_len = pattern - oc.bv_val;
			}

			group_oc = oc_bvfind( &oc );
			if ( group_oc == NULL ) {
				goto no_oc;
			}
		}

		if ( group_oc == NULL ) {
			group_oc = oc_find( SLAPD_GROUP_CLASS );
			if ( group_oc == NULL ) {
no_oc:;
				return( -1 );
			}
		}

		if ( group_ad == NULL ) {
			const char	*text = NULL;
			
			rc = slap_str2ad( SLAPD_GROUP_ATTR, &group_ad, &text );

			if ( rc != LDAP_SUCCESS ) {
no_ad:;
				return( -1 );
			}
		}

		flags = SLAP_LIMITS_TYPE_GROUP | SLAP_LIMITS_EXACT;

		if ( pattern[0] != '=' ) {
			Debug( LDAP_DEBUG_ANY,
				"%s : line %d: missing '=' in "
				"\"group[/objectClass[/attributeType]]"
				"=<pattern>\" in "
				"\"limits <pattern> <limits>\" line.\n",
				fname, lineno, 0 );
			return( -1 );
		}

		/* skip '=' (required) */
		pattern++;
	}

	/* get the limits */
	for ( i = 2; i < argc; i++ ) {
		if ( limits_parse_one( argv[i], &limit ) ) {

			Debug( LDAP_DEBUG_ANY,
				"%s : line %d: unknown limit values \"%s\" in "
				"\"limits <pattern> <limits>\" line.\n",
			fname, lineno, argv[i] );

			return( 1 );
		}
	}

	/*
	 * sanity checks ...
	 *
	 * FIXME: add warnings?
	 */
	if ( limit.lms_t_hard > 0 && 
			( limit.lms_t_hard < limit.lms_t_soft 
			  || limit.lms_t_soft == -1 ) ) {
		limit.lms_t_hard = limit.lms_t_soft;
	}
	
	if ( limit.lms_s_hard > 0 && 
			( limit.lms_s_hard < limit.lms_s_soft 
			  || limit.lms_s_soft == -1 ) ) {
		limit.lms_s_hard = limit.lms_s_soft;
	}

	/*
	 * defaults ...
	 * 
	 * lms_t_hard:
	 * 	-1	=> no limits
	 * 	0	=> same as soft
	 * 	> 0	=> limit (in seconds)
	 *
	 * lms_s_hard:
	 * 	-1	=> no limits
	 * 	0	0> same as soft
	 * 	> 0	=> limit (in entries)
	 *
	 * lms_s_pr_total:
	 * 	-2	=> disable the control
	 * 	-1	=> no limits
	 * 	0	=> same as soft
	 * 	> 0	=> limit (in entries)
	 *
	 * lms_s_pr:
	 * 	-1	=> no limits
	 * 	0	=> no limits?
	 * 	> 0	=> limit size (in entries)
	 */
	if ( limit.lms_s_pr_total > 0 &&
			limit.lms_s_pr > limit.lms_s_pr_total ) {
		limit.lms_s_pr = limit.lms_s_pr_total;
	}

	rc = limits_add( be, flags, pattern, group_oc, group_ad, &limit );
	if ( rc ) {

		Debug( LDAP_DEBUG_ANY,
			"%s : line %d: unable to add limit in "
			"\"limits <pattern> <limits>\" line.\n",
		fname, lineno, 0 );
	}

	return( rc );
}

int
limits_parse_one(
	const char 		*arg,
	struct slap_limits_set 	*limit
)
{
	assert( arg != NULL );
	assert( limit != NULL );

	if ( STRSTART( arg, "time" ) ) {
		arg += STRLENOF( "time" );

		if ( arg[0] == '.' ) {
			arg++;
			if ( STRSTART( arg, "soft=" ) ) {
				arg += STRLENOF( "soft=" );
				if ( strcasecmp( arg, "unlimited" ) == 0
					|| strcasecmp( arg, "none" ) == 0 )
				{
					limit->lms_t_soft = -1;

				} else {
					int	soft;

					if ( lutil_atoi( &soft, arg ) != 0 || soft < -1 ) {
						return( 1 );
					}

					if ( soft == -1 ) {
						/* FIXME: use "unlimited" instead; issue warning? */
					}

					limit->lms_t_soft = soft;
				}
				
			} else if ( STRSTART( arg, "hard=" ) ) {
				arg += STRLENOF( "hard=" );
				if ( strcasecmp( arg, "soft" ) == 0 ) {
					limit->lms_t_hard = 0;

				} else if ( strcasecmp( arg, "unlimited" ) == 0
						|| strcasecmp( arg, "none" ) == 0 )
				{
					limit->lms_t_hard = -1;

				} else {
					int	hard;

					if ( lutil_atoi( &hard, arg ) != 0 || hard < -1 ) {
						return( 1 );
					}

					if ( hard == -1 ) {
						/* FIXME: use "unlimited" instead */
					}

					if ( hard == 0 ) {
						/* FIXME: use "soft" instead */
					}

					limit->lms_t_hard = hard;
				}
				
			} else {
				return( 1 );
			}
			
		} else if ( arg[0] == '=' ) {
			arg++;
			if ( strcasecmp( arg, "unlimited" ) == 0
				|| strcasecmp( arg, "none" ) == 0 )
			{
				limit->lms_t_soft = -1;

			} else {
				if ( lutil_atoi( &limit->lms_t_soft, arg ) != 0 
					|| limit->lms_t_soft < -1 )
				{
					return( 1 );
				}
			}
			limit->lms_t_hard = 0;
			
		} else {
			return( 1 );
		}

	} else if ( STRSTART( arg, "size" ) ) {
		arg += STRLENOF( "size" );
		
		if ( arg[0] == '.' ) {
			arg++;
			if ( STRSTART( arg, "soft=" ) ) {
				arg += STRLENOF( "soft=" );
				if ( strcasecmp( arg, "unlimited" ) == 0
					|| strcasecmp( arg, "none" ) == 0 )
				{
					limit->lms_s_soft = -1;

				} else {
					int	soft;

					if ( lutil_atoi( &soft, arg ) != 0 || soft < -1 ) {
						return( 1 );
					}

					if ( soft == -1 ) {
						/* FIXME: use "unlimited" instead */
					}

					limit->lms_s_soft = soft;
				}
				
			} else if ( STRSTART( arg, "hard=" ) ) {
				arg += STRLENOF( "hard=" );
				if ( strcasecmp( arg, "soft" ) == 0 ) {
					limit->lms_s_hard = 0;

				} else if ( strcasecmp( arg, "unlimited" ) == 0
						|| strcasecmp( arg, "none" ) == 0 )
				{
					limit->lms_s_hard = -1;

				} else {
					int	hard;

					if ( lutil_atoi( &hard, arg ) != 0 || hard < -1 ) {
						return( 1 );
					}

					if ( hard == -1 ) {
						/* FIXME: use "unlimited" instead */
					}

					if ( hard == 0 ) {
						/* FIXME: use "soft" instead */
					}

					limit->lms_s_hard = hard;
				}
				
			} else if ( STRSTART( arg, "unchecked=" ) ) {
				arg += STRLENOF( "unchecked=" );
				if ( strcasecmp( arg, "unlimited" ) == 0
					|| strcasecmp( arg, "none" ) == 0 )
				{
					limit->lms_s_unchecked = -1;

				} else if ( strcasecmp( arg, "disabled" ) == 0 ) {
					limit->lms_s_unchecked = 0;

				} else {
					int	unchecked;

					if ( lutil_atoi( &unchecked, arg ) != 0 || unchecked < -1 ) {
						return( 1 );
					}

					if ( unchecked == -1 ) {
						/*  FIXME: use "unlimited" instead */
					}

					limit->lms_s_unchecked = unchecked;
				}

			} else if ( STRSTART( arg, "pr=" ) ) {
				arg += STRLENOF( "pr=" );
				if ( strcasecmp( arg, "noEstimate" ) == 0 ) {
					limit->lms_s_pr_hide = 1;

				} else if ( strcasecmp( arg, "unlimited" ) == 0
						|| strcasecmp( arg, "none" ) == 0 )
				{
					limit->lms_s_pr = -1;

				} else {
					int	pr;

					if ( lutil_atoi( &pr, arg ) != 0 || pr < -1 ) {
						return( 1 );
					}

					if ( pr == -1 ) {
						/* FIXME: use "unlimited" instead */
					}

					limit->lms_s_pr = pr;
				}

			} else if ( STRSTART( arg, "prtotal=" ) ) {
				arg += STRLENOF( "prtotal=" );

				if ( strcasecmp( arg, "unlimited" ) == 0
					|| strcasecmp( arg, "none" ) == 0 )
				{
					limit->lms_s_pr_total = -1;

				} else if ( strcasecmp( arg, "disabled" ) == 0 ) {
					limit->lms_s_pr_total = -2;

				} else if ( strcasecmp( arg, "hard" ) == 0 ) {
					limit->lms_s_pr_total = 0;

				} else {
					int	total;

					if ( lutil_atoi( &total, arg ) != 0 || total < -1 ) {
						return( 1 );
					}

					if ( total == -1 ) {
						/* FIXME: use "unlimited" instead */
					}

					if ( total == 0 ) {
						/* FIXME: use "pr=disable" instead */
					}

					limit->lms_s_pr_total = total;
				}

			} else {
				return( 1 );
			}
			
		} else if ( arg[0] == '=' ) {
			arg++;
			if ( strcasecmp( arg, "unlimited" ) == 0
				|| strcasecmp( arg, "none" ) == 0 )
			{
				limit->lms_s_soft = -1;

			} else {
				if ( lutil_atoi( &limit->lms_s_soft, arg ) != 0
					|| limit->lms_s_soft < -1 )
				{
					return( 1 );
				}
			}
			limit->lms_s_hard = 0;
			
		} else {
			return( 1 );
		}
	}

	return 0;
}

/* Helper macros for limits_unparse() and limits_unparse_one():
 * Write to ptr, but not past bufEnd.  Move ptr past the new text.
 * Return (success && enough room ? 0 : -1).
 */
#define ptr_APPEND_BV(bv) /* Append a \0-terminated berval */ \
	(WHATSLEFT <= (bv).bv_len ? -1 : \
	 ((void) (ptr = lutil_strcopy( ptr, (bv).bv_val )), 0))
#define ptr_APPEND_LIT(str) /* Append a string literal */ \
	(WHATSLEFT <= STRLENOF( "" str "" ) ? -1 : \
	 ((void) (ptr = lutil_strcopy( ptr, str )), 0))
#define ptr_APPEND_FMT(args) /* Append formatted text */ \
	(WHATSLEFT <= (tmpLen = snprintf args) ? -1 : ((void) (ptr += tmpLen), 0))
#define ptr_APPEND_FMT1(fmt, arg) ptr_APPEND_FMT(( ptr, WHATSLEFT, fmt, arg ))
#define WHATSLEFT ((ber_len_t) (bufEnd - ptr))

/* Caller must provide an adequately sized buffer in bv */
int
limits_unparse( struct slap_limits *lim, struct berval *bv, ber_len_t buflen )
{
	struct berval btmp;
	char *ptr, *bufEnd;			/* Updated/used by ptr_APPEND_*()/WHATSLEFT */
	ber_len_t tmpLen;			/* Used by ptr_APPEND_FMT*() */
	unsigned type, style;
	int rc = 0;

	if ( !bv || !bv->bv_val ) return -1;

	ptr = bv->bv_val;
	bufEnd = ptr + buflen;
	type = lim->lm_flags & SLAP_LIMITS_TYPE_MASK;

	if ( type == SLAP_LIMITS_TYPE_GROUP ) {
		rc = ptr_APPEND_FMT(( ptr, WHATSLEFT, "group/%s/%s=\"%s\"",
			lim->lm_group_oc->soc_cname.bv_val,
			lim->lm_group_ad->ad_cname.bv_val,
			lim->lm_pat.bv_val ));
	} else {
		style = lim->lm_flags & SLAP_LIMITS_MASK;
		switch( style ) {
		case SLAP_LIMITS_ANONYMOUS:
		case SLAP_LIMITS_USERS:
		case SLAP_LIMITS_ANY:
			rc = ptr_APPEND_BV( lmpats[style] );
			break;
		case SLAP_LIMITS_UNDEFINED:
		case SLAP_LIMITS_EXACT:
		case SLAP_LIMITS_ONE:
		case SLAP_LIMITS_SUBTREE:
		case SLAP_LIMITS_CHILDREN:
		case SLAP_LIMITS_REGEX:
			rc = ptr_APPEND_FMT(( ptr, WHATSLEFT, "dn.%s%s=\"%s\"",
				type == SLAP_LIMITS_TYPE_SELF ? "" : "this.",
				lmpats[style].bv_val, lim->lm_pat.bv_val ));
			break;
		}
	}
	if ( rc == 0 ) {
		bv->bv_len = ptr - bv->bv_val;
		btmp.bv_val = ptr;
		btmp.bv_len = 0;
		rc = limits_unparse_one( &lim->lm_limits,
			SLAP_LIMIT_SIZE | SLAP_LIMIT_TIME,
			&btmp, WHATSLEFT );
		if ( rc == 0 )
			bv->bv_len += btmp.bv_len;
	}
	return rc;
}

/* Caller must provide an adequately sized buffer in bv */
int
limits_unparse_one(
	struct slap_limits_set	*lim,
	int				which,
	struct berval	*bv,
	ber_len_t		buflen )
{
	char *ptr, *bufEnd;			/* Updated/used by ptr_APPEND_*()/WHATSLEFT */
	ber_len_t tmpLen;			/* Used by ptr_APPEND_FMT*() */

	if ( !bv || !bv->bv_val ) return -1;

	ptr = bv->bv_val;
	bufEnd = ptr + buflen;

	if ( which & SLAP_LIMIT_SIZE ) {
		if ( lim->lms_s_soft != SLAPD_DEFAULT_SIZELIMIT ) {

			/* If same as global limit, drop it */
			if ( lim != &frontendDB->be_def_limit &&
				lim->lms_s_soft == frontendDB->be_def_limit.lms_s_soft )
			{
				goto s_hard;
			/* If there's also a hard limit, fully qualify this one */
			} else if ( lim->lms_s_hard ) {
				if ( ptr_APPEND_LIT( " size.soft=" ) ) return -1;

			/* If doing both size & time, qualify this */
			} else if ( which & SLAP_LIMIT_TIME ) {
				if ( ptr_APPEND_LIT( " size=" ) ) return -1;
			}

			if ( lim->lms_s_soft == -1
					? ptr_APPEND_LIT( "unlimited " )
					: ptr_APPEND_FMT1( "%d ", lim->lms_s_soft ) )
				return -1;
		}
s_hard:
		if ( lim->lms_s_hard ) {
			if ( ptr_APPEND_LIT( " size.hard=" ) ) return -1;
			if ( lim->lms_s_hard == -1
					? ptr_APPEND_LIT( "unlimited " )
					: ptr_APPEND_FMT1( "%d ", lim->lms_s_hard ) )
				return -1;
		}
		if ( lim->lms_s_unchecked != -1 ) {
			if ( ptr_APPEND_LIT( " size.unchecked=" ) ) return -1;
			if ( lim->lms_s_unchecked == 0
					? ptr_APPEND_LIT( "disabled " )
					: ptr_APPEND_FMT1( "%d ", lim->lms_s_unchecked ) )
				return -1;
		}
		if ( lim->lms_s_pr_hide ) {
			if ( ptr_APPEND_LIT( " size.pr=noEstimate " ) ) return -1;
		}
		if ( lim->lms_s_pr ) {
			if ( ptr_APPEND_LIT( " size.pr=" ) ) return -1;
			if ( lim->lms_s_pr == -1
					? ptr_APPEND_LIT( "unlimited " )
					: ptr_APPEND_FMT1( "%d ", lim->lms_s_pr ) )
				return -1;
		}
		if ( lim->lms_s_pr_total ) {
			if ( ptr_APPEND_LIT( " size.prtotal=" ) ) return -1;
			if ( lim->lms_s_pr_total  == -1 ? ptr_APPEND_LIT( "unlimited " )
				: lim->lms_s_pr_total == -2 ? ptr_APPEND_LIT( "disabled " )
				: ptr_APPEND_FMT1( "%d ", lim->lms_s_pr_total ) )
				return -1;
		}
	}

	if ( which & SLAP_LIMIT_TIME ) {
		if ( lim->lms_t_soft != SLAPD_DEFAULT_TIMELIMIT ) {

			/* If same as global limit, drop it */
			if ( lim != &frontendDB->be_def_limit &&
				lim->lms_t_soft == frontendDB->be_def_limit.lms_t_soft )
			{
				goto t_hard;

			/* If there's also a hard limit, fully qualify this one */
			} else if ( lim->lms_t_hard ) {
				if ( ptr_APPEND_LIT( " time.soft=" ) ) return -1;

			/* If doing both size & time, qualify this */
			} else if ( which & SLAP_LIMIT_SIZE ) {
				if ( ptr_APPEND_LIT( " time=" ) ) return -1;
			}

			if ( lim->lms_t_soft == -1
					? ptr_APPEND_LIT( "unlimited " )
					: ptr_APPEND_FMT1( "%d ", lim->lms_t_soft ) )
				return -1;
		}
t_hard:
		if ( lim->lms_t_hard ) {
			if ( ptr_APPEND_LIT( " time.hard=" ) ) return -1;
			if ( lim->lms_t_hard == -1
					? ptr_APPEND_LIT( "unlimited " )
					: ptr_APPEND_FMT1( "%d ", lim->lms_t_hard ) )
				return -1;
		}
	}
	if ( ptr != bv->bv_val ) {
		ptr--;
		*ptr = '\0';
		bv->bv_len = ptr - bv->bv_val;
	}

	return 0;
}

int
limits_check( Operation *op, SlapReply *rs )
{
	assert( op != NULL );
	assert( rs != NULL );
	/* FIXME: should this be always true? */
	assert( op->o_tag == LDAP_REQ_SEARCH);

	/* protocol only allows 0..maxInt;
	 *
	 * internal searches:
	 * - may use SLAP_NO_LIMIT ( = -1 ) to indicate no limits;
	 * - should use slimit = N and tlimit = SLAP_NO_LIMIT to
	 *   indicate searches that should return exactly N matches,
	 *   and handle errors thru a callback (see for instance
	 *   slap_sasl_match() and slap_sasl2dn())
	 */
	if ( op->ors_tlimit == SLAP_NO_LIMIT && op->ors_slimit == SLAP_NO_LIMIT ) {
		return 0;
	}

	/* allow root to set no limit */
	if ( be_isroot( op ) ) {
		op->ors_limit = NULL;

		if ( op->ors_tlimit == 0 ) {
			op->ors_tlimit = SLAP_NO_LIMIT;
		}

		if ( op->ors_slimit == 0 ) {
			op->ors_slimit = SLAP_NO_LIMIT;
		}

		/* if paged results and slimit are requested */	
		if ( get_pagedresults( op ) > SLAP_CONTROL_IGNORED &&
			op->ors_slimit != SLAP_NO_LIMIT ) {
			PagedResultsState *ps = op->o_pagedresults_state;
			int total = op->ors_slimit - ps->ps_count;
			if ( total > 0 ) {
				op->ors_slimit = total;
			} else {
				op->ors_slimit = 0;
			}
		}

	/* if not root, get appropriate limits */
	} else {
		( void ) limits_get( op, &op->ors_limit );

		assert( op->ors_limit != NULL );

		/* if no limit is required, use soft limit */
		if ( op->ors_tlimit == 0 ) {
			op->ors_tlimit = op->ors_limit->lms_t_soft;

		/* limit required: check if legal */
		} else {
			if ( op->ors_limit->lms_t_hard == 0 ) {
				if ( op->ors_limit->lms_t_soft > 0
						&& ( op->ors_tlimit > op->ors_limit->lms_t_soft ) ) {
					op->ors_tlimit = op->ors_limit->lms_t_soft;
				}

			} else if ( op->ors_limit->lms_t_hard > 0 ) {
#ifdef ABOVE_HARD_LIMIT_IS_ERROR
				if ( op->ors_tlimit == SLAP_MAX_LIMIT ) {
					op->ors_tlimit = op->ors_limit->lms_t_hard;

				} else if ( op->ors_tlimit > op->ors_limit->lms_t_hard ) {
					/* error if exceeding hard limit */
					rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
					send_ldap_result( op, rs );
					rs->sr_err = LDAP_SUCCESS;
					return -1;
				}
#else /* ! ABOVE_HARD_LIMIT_IS_ERROR */
				if ( op->ors_tlimit > op->ors_limit->lms_t_hard ) {
					op->ors_tlimit = op->ors_limit->lms_t_hard;
				}
#endif /* ! ABOVE_HARD_LIMIT_IS_ERROR */
			}
		}

		/* else leave as is */

		/* don't even get to backend if candidate check is disabled */
		if ( op->ors_limit->lms_s_unchecked == 0 ) {
			rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
			send_ldap_result( op, rs );
			rs->sr_err = LDAP_SUCCESS;
			return -1;
		}

		/* if paged results is requested */	
		if ( get_pagedresults( op ) > SLAP_CONTROL_IGNORED ) {
			int	slimit = -2;
			int	pr_total;
			PagedResultsState *ps = op->o_pagedresults_state;

			/* paged results is not allowed */
			if ( op->ors_limit->lms_s_pr_total == -2 ) {
				rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
				rs->sr_text = "pagedResults control not allowed";
				send_ldap_result( op, rs );
				rs->sr_err = LDAP_SUCCESS;
				rs->sr_text = NULL;
				return -1;
			}
			
			if ( op->ors_limit->lms_s_pr > 0
				&& ps->ps_size > op->ors_limit->lms_s_pr )
			{
				rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
				rs->sr_text = "illegal pagedResults page size";
				send_ldap_result( op, rs );
				rs->sr_err = LDAP_SUCCESS;
				rs->sr_text = NULL;
				return -1;
			}

			if ( op->ors_limit->lms_s_pr_total == 0 ) {
				if ( op->ors_limit->lms_s_hard == 0 ) {
					pr_total = op->ors_limit->lms_s_soft;
				} else {
					pr_total = op->ors_limit->lms_s_hard;
				}
			} else {
				pr_total = op->ors_limit->lms_s_pr_total;
			}

			if ( pr_total == -1 ) {
				if ( op->ors_slimit == 0 || op->ors_slimit == SLAP_MAX_LIMIT ) {
					slimit = -1;

				} else {
					slimit = op->ors_slimit - ps->ps_count;
				}

#ifdef ABOVE_HARD_LIMIT_IS_ERROR
			} else if ( pr_total > 0 && op->ors_slimit != SLAP_MAX_LIMIT
					&& ( op->ors_slimit == SLAP_NO_LIMIT
						|| op->ors_slimit > pr_total ) )
			{
				rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
				send_ldap_result( op, rs );
				rs->sr_err = LDAP_SUCCESS;
				return -1;
#endif /* ! ABOVE_HARD_LIMIT_IS_ERROR */
	
			} else {
				/* if no limit is required, use soft limit */
				int	total;
				int	slimit2;

				/* first round of pagedResults:
				 * set count to any appropriate limit */

				/* if the limit is set, check that it does
				 * not violate any server-side limit */
#ifdef ABOVE_HARD_LIMIT_IS_ERROR
				if ( op->ors_slimit == SLAP_MAX_LIMIT )
#else /* ! ABOVE_HARD_LIMIT_IS_ERROR */
				if ( op->ors_slimit == SLAP_MAX_LIMIT
					|| op->ors_slimit > pr_total )
#endif /* ! ABOVE_HARD_LIMIT_IS_ERROR */
				{
					slimit2 = op->ors_slimit = pr_total;

				} else if ( op->ors_slimit == 0 ) {
					slimit2 = pr_total;

				} else {
					slimit2 = op->ors_slimit;
				}

				total = slimit2 - ps->ps_count;

				if ( total >= 0 ) {
					if ( op->ors_limit->lms_s_pr > 0 ) {
						/* use the smallest limit set by total/per page */
						if ( total < op->ors_limit->lms_s_pr ) {
							slimit = total;
	
						} else {
							/* use the perpage limit if any 
							 * NOTE: + 1 because given value must be legal */
							slimit = op->ors_limit->lms_s_pr + 1;
						}

					} else {
						/* use the total limit if any */
						slimit = total;
					}

				} else if ( op->ors_limit->lms_s_pr > 0 ) {
					/* use the perpage limit if any 
					 * NOTE: + 1 because the given value must be legal */
					slimit = op->ors_limit->lms_s_pr + 1;

				} else {
					/* use the standard hard/soft limit if any */
					slimit = op->ors_limit->lms_s_hard;
				}
			}
		
			/* if got any limit, use it */
			if ( slimit != -2 ) {
				if ( op->ors_slimit == 0 ) {
					op->ors_slimit = slimit;

				} else if ( slimit > 0 ) {
					if ( op->ors_slimit - ps->ps_count > slimit ) {
						rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
						send_ldap_result( op, rs );
						rs->sr_err = LDAP_SUCCESS;
						return -1;
					}
					op->ors_slimit = slimit;

				} else if ( slimit == 0 ) {
					op->ors_slimit = 0;
				}

			} else {
				/* use the standard hard/soft limit if any */
				op->ors_slimit = pr_total;
			}

		/* no limit requested: use soft, whatever it is */
		} else if ( op->ors_slimit == 0 ) {
			op->ors_slimit = op->ors_limit->lms_s_soft;

		/* limit requested: check if legal */
		} else {
			/* hard limit as soft (traditional behavior) */
			if ( op->ors_limit->lms_s_hard == 0 ) {
				if ( op->ors_limit->lms_s_soft > 0
						&& op->ors_slimit > op->ors_limit->lms_s_soft ) {
					op->ors_slimit = op->ors_limit->lms_s_soft;
				}

			/* explicit hard limit: error if violated */
			} else if ( op->ors_limit->lms_s_hard > 0 ) {
#ifdef ABOVE_HARD_LIMIT_IS_ERROR
				if ( op->ors_slimit == SLAP_MAX_LIMIT ) {
					op->ors_slimit = op->ors_limit->lms_s_hard;

				} else if ( op->ors_slimit > op->ors_limit->lms_s_hard ) {
					/* if limit exceeds hard, error */
					rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
					send_ldap_result( op, rs );
					rs->sr_err = LDAP_SUCCESS;
					return -1;
				}
#else /* ! ABOVE_HARD_LIMIT_IS_ERROR */
				if ( op->ors_slimit > op->ors_limit->lms_s_hard ) {
					op->ors_slimit = op->ors_limit->lms_s_hard;
				}
#endif /* ! ABOVE_HARD_LIMIT_IS_ERROR */
			}
		}

		/* else leave as is */
	}

	return 0;
}

void
limits_free_one( 
	struct slap_limits	*lm )
{
	if ( ( lm->lm_flags & SLAP_LIMITS_MASK ) == SLAP_LIMITS_REGEX )
		regfree( &lm->lm_regex );

	if ( !BER_BVISNULL( &lm->lm_pat ) )
		ch_free( lm->lm_pat.bv_val );

	ch_free( lm );
}

void
limits_destroy( 
	struct slap_limits	**lm )
{
	int		i;

	if ( lm == NULL ) {
		return;
	}

	for ( i = 0; lm[ i ]; i++ ) {
		limits_free_one( lm[ i ] );
	}

	ch_free( lm );
}
