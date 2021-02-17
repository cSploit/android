/* distproc.c - implement distributed procedures */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
 * Portions Copyright 2003 Howard Chu.
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
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 * Based on back-ldap and slapo-chain, developed by Howard Chu
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"

#ifdef SLAP_DISTPROC

#include "back-ldap.h"

#include "config.h"

/*
 * From <draft-sermersheim-ldap-distproc>
 *

      ContinuationReference ::= SET {
         referralURI      [0] SET SIZE (1..MAX) OF URI,
         localReference   [2] LDAPDN,
         referenceType    [3] ReferenceType,
         remainingName    [4] RelativeLDAPDN OPTIONAL,
         searchScope      [5] SearchScope OPTIONAL,
         searchedSubtrees [6] SearchedSubtrees OPTIONAL,
         failedName       [7] LDAPDN OPTIONAL,
         ...  }

      ReferenceType ::= ENUMERATED {
         superior               (0),
         subordinate            (1),
         cross                  (2),
         nonSpecificSubordinate (3),
         supplier               (4),
         master                 (5),
         immediateSuperior      (6),
         self                   (7),
         ...  }

      SearchScope ::= ENUMERATED {
         baseObject         (0),
         singleLevel        (1),
         wholeSubtree       (2),
         subordinateSubtree (3),
         ...  }

   SearchedSubtrees ::= SET OF RelativeLDAPDN

   LDAPDN, RelativeLDAPDN, and LDAPString, are defined in [RFC2251].

 */

typedef enum ReferenceType_t {
	LDAP_DP_RT_UNKNOWN			= -1,
	LDAP_DP_RT_SUPERIOR			= 0,
	LDAP_DP_RT_SUBORDINATE			= 1,
	LDAP_DP_RT_CROSS			= 2,
	LDAP_DP_RT_NONSPECIFICSUBORDINATE	= 3,
	LDAP_DP_RT_SUPPLIER			= 4,
	LDAP_DP_RT_MASTER			= 5,
	LDAP_DP_RT_IMMEDIATESUPERIOR		= 6,
	LDAP_DP_RT_SELF				= 7,
	LDAP_DP_RT_LAST
} ReferenceType_t;

typedef enum SearchScope_t {
	LDAP_DP_SS_UNKNOWN			= -1,
	LDAP_DP_SS_BASEOBJECT			= 0,
	LDAP_DP_SS_SINGLELEVEL			= 1,
	LDAP_DP_SS_WHOLESUBTREE			= 2,
	LDAP_DP_SS_SUBORDINATESUBTREE		= 3,
	LDAP_DP_SS_LAST
} SearchScope_t;

typedef struct ContinuationReference_t {
	BerVarray		cr_referralURI;
	/* ?			[1] ? */
	struct berval		cr_localReference;
	ReferenceType_t		cr_referenceType;
	struct berval		cr_remainingName;
	SearchScope_t		cr_searchScope;
	BerVarray		cr_searchedSubtrees;
	struct berval		cr_failedName;
} ContinuationReference_t;
#define	CR_INIT		{ NULL, BER_BVNULL, LDAP_DP_RT_UNKNOWN, BER_BVNULL, LDAP_DP_SS_UNKNOWN, NULL, BER_BVNULL }

#ifdef unused
static struct berval	bv2rt[] = {
	BER_BVC( "superior" ),
	BER_BVC( "subordinate" ),
	BER_BVC( "cross" ),
	BER_BVC( "nonSpecificSubordinate" ),
	BER_BVC( "supplier" ),
	BER_BVC( "master" ),
	BER_BVC( "immediateSuperior" ),
	BER_BVC( "self" ),
	BER_BVNULL
};

static struct berval	bv2ss[] = {
	BER_BVC( "baseObject" ),
	BER_BVC( "singleLevel" ),
	BER_BVC( "wholeSubtree" ),
	BER_BVC( "subordinateSubtree" ),
	BER_BVNULL
};

static struct berval *
ldap_distproc_rt2bv( ReferenceType_t rt )
{
	return &bv2rt[ rt ];
}

static const char *
ldap_distproc_rt2str( ReferenceType_t rt )
{
	return bv2rt[ rt ].bv_val;
}

static ReferenceType_t
ldap_distproc_bv2rt( struct berval *bv )
{
	ReferenceType_t		rt;

	for ( rt = 0; !BER_BVISNULL( &bv2rt[ rt ] ); rt++ ) {
		if ( ber_bvstrcasecmp( bv, &bv2rt[ rt ] ) == 0 ) {
			return rt;
		}
	}

	return LDAP_DP_RT_UNKNOWN;
}

static ReferenceType_t
ldap_distproc_str2rt( const char *s )
{
	struct berval	bv;

	ber_str2bv( s, 0, 0, &bv );
	return ldap_distproc_bv2rt( &bv );
}

static struct berval *
ldap_distproc_ss2bv( SearchScope_t ss )
{
	return &bv2ss[ ss ];
}

static const char *
ldap_distproc_ss2str( SearchScope_t ss )
{
	return bv2ss[ ss ].bv_val;
}

static SearchScope_t
ldap_distproc_bv2ss( struct berval *bv )
{
	ReferenceType_t		ss;

	for ( ss = 0; !BER_BVISNULL( &bv2ss[ ss ] ); ss++ ) {
		if ( ber_bvstrcasecmp( bv, &bv2ss[ ss ] ) == 0 ) {
			return ss;
		}
	}

	return LDAP_DP_SS_UNKNOWN;
}

static SearchScope_t
ldap_distproc_str2ss( const char *s )
{
	struct berval	bv;

	ber_str2bv( s, 0, 0, &bv );
	return ldap_distproc_bv2ss( &bv );
}
#endif /* unused */

/*
 * NOTE: this overlay assumes that the chainingBehavior control
 * is registered by the chain overlay; it may move here some time.
 * This overlay provides support for that control as well.
 */


static int		sc_returnContRef;
#define o_returnContRef			o_ctrlflag[sc_returnContRef]
#define get_returnContRef(op)		((op)->o_returnContRef & SLAP_CONTROL_MASK)

static struct berval	slap_EXOP_CHAINEDREQUEST = BER_BVC( LDAP_EXOP_X_CHAINEDREQUEST );
static struct berval	slap_FEATURE_CANCHAINOPS = BER_BVC( LDAP_FEATURE_X_CANCHAINOPS );

static BackendInfo	*lback;

typedef struct ldap_distproc_t {
	/* "common" configuration info (anything occurring before an "uri") */
	ldapinfo_t		*lc_common_li;

	/* current configuration info */
	ldapinfo_t		*lc_cfg_li;

	/* tree of configured[/generated?] "uri" info */
	ldap_avl_info_t		lc_lai;

	unsigned		lc_flags;
#define LDAP_DISTPROC_F_NONE		(0x00U)
#define	LDAP_DISTPROC_F_CHAINING	(0x01U)
#define	LDAP_DISTPROC_F_CACHE_URI	(0x10U)

#define	LDAP_DISTPROC_CHAINING( lc )	( ( (lc)->lc_flags & LDAP_DISTPROC_F_CHAINING ) == LDAP_DISTPROC_F_CHAINING )
#define	LDAP_DISTPROC_CACHE_URI( lc )	( ( (lc)->lc_flags & LDAP_DISTPROC_F_CACHE_URI ) == LDAP_DISTPROC_F_CACHE_URI )

} ldap_distproc_t;

static int ldap_distproc_db_init_common( BackendDB	*be );
static int ldap_distproc_db_init_one( BackendDB *be );
#define	ldap_distproc_db_open_one(be)		(lback)->bi_db_open( (be) )
#define	ldap_distproc_db_close_one(be)		(0)
#define	ldap_distproc_db_destroy_one(be, ca)	(lback)->bi_db_destroy( (be), (ca) )

static int
ldap_distproc_uri_cmp( const void *c1, const void *c2 )
{
	const ldapinfo_t	*li1 = (const ldapinfo_t *)c1;
	const ldapinfo_t	*li2 = (const ldapinfo_t *)c2;

	assert( li1->li_bvuri != NULL );
	assert( !BER_BVISNULL( &li1->li_bvuri[ 0 ] ) );
	assert( BER_BVISNULL( &li1->li_bvuri[ 1 ] ) );

	assert( li2->li_bvuri != NULL );
	assert( !BER_BVISNULL( &li2->li_bvuri[ 0 ] ) );
	assert( BER_BVISNULL( &li2->li_bvuri[ 1 ] ) );

	/* If local DNs don't match, it is definitely not a match */
	return ber_bvcmp( &li1->li_bvuri[ 0 ], &li2->li_bvuri[ 0 ] );
}

static int
ldap_distproc_uri_dup( void *c1, void *c2 )
{
	ldapinfo_t	*li1 = (ldapinfo_t *)c1;
	ldapinfo_t	*li2 = (ldapinfo_t *)c2;

	assert( li1->li_bvuri != NULL );
	assert( !BER_BVISNULL( &li1->li_bvuri[ 0 ] ) );
	assert( BER_BVISNULL( &li1->li_bvuri[ 1 ] ) );

	assert( li2->li_bvuri != NULL );
	assert( !BER_BVISNULL( &li2->li_bvuri[ 0 ] ) );
	assert( BER_BVISNULL( &li2->li_bvuri[ 1 ] ) );

	/* Cannot have more than one shared session with same DN */
	if ( ber_bvcmp( &li1->li_bvuri[ 0 ], &li2->li_bvuri[ 0 ] ) == 0 ) {
		return -1;
	}
		
	return 0;
}

static int
ldap_distproc_operational( Operation *op, SlapReply *rs )
{
	/* Trap entries generated by back-ldap.
	 * 
	 * FIXME: we need a better way to recognize them; a cleaner
	 * solution would be to be able to intercept the response
	 * of be_operational(), so that we can divert only those
	 * calls that fail because operational attributes were
	 * requested for entries that do not belong to the underlying
	 * database.  This fix is likely to intercept also entries
	 * generated by back-perl and so. */
	if ( rs->sr_entry->e_private == NULL ) {
		return LDAP_SUCCESS;
	}

	return SLAP_CB_CONTINUE;
}

static int
ldap_distproc_response( Operation *op, SlapReply *rs )
{
	return SLAP_CB_CONTINUE;
}

/*
 * configuration...
 */

enum {
	/* NOTE: the chaining behavior control is registered
	 * by the chain overlay; it may move here some time */
	DP_CHAINING = 1,
	DP_CACHE_URI,

	DP_LAST
};

static ConfigDriver distproc_cfgen;
static ConfigCfAdd distproc_cfadd;
static ConfigLDAPadd distproc_ldadd;

static ConfigTable distproc_cfg[] = {
	{ "distproc-chaining", "args",
		2, 4, 0, ARG_MAGIC|ARG_BERVAL|DP_CHAINING, distproc_cfgen,
		/* NOTE: using the same attributeTypes defined
		 * for the "chain" overlay */
		"( OLcfgOvAt:3.1 NAME 'olcChainingBehavior' "
			"DESC 'Chaining behavior control parameters (draft-sermersheim-ldap-chaining)' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "distproc-cache-uri", "TRUE/FALSE",
		2, 2, 0, ARG_MAGIC|ARG_ON_OFF|DP_CACHE_URI, distproc_cfgen,
		"( OLcfgOvAt:3.2 NAME 'olcChainCacheURI' "
			"DESC 'Enables caching of URIs not present in configuration' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs distproc_ocs[] = {
	{ "( OLcfgOvOc:7.1 "
		"NAME 'olcDistProcConfig' "
		"DESC 'Distributed procedures <draft-sermersheim-ldap-distproc> configuration' "
		"SUP olcOverlayConfig "
		"MAY ( "
			"olcChainingBehavior $ "
			"olcChainCacheURI "
			") )",
		Cft_Overlay, distproc_cfg, NULL, distproc_cfadd },
	{ "( OLcfgOvOc:7.2 "
		"NAME 'olcDistProcDatabase' "
		"DESC 'Distributed procedure remote server configuration' "
		"AUXILIARY )",
		Cft_Misc, distproc_cfg, distproc_ldadd },
	{ NULL, 0, NULL }
};

static int
distproc_ldadd( CfEntryInfo *p, Entry *e, ConfigArgs *ca )
{
	slap_overinst		*on;
	ldap_distproc_t		*lc;

	ldapinfo_t		*li;

	AttributeDescription	*ad = NULL;
	Attribute		*at;
	const char		*text;

	int			rc;

	if ( p->ce_type != Cft_Overlay
		|| !p->ce_bi
		|| p->ce_bi->bi_cf_ocs != distproc_ocs )
	{
		return LDAP_CONSTRAINT_VIOLATION;
	}

	on = (slap_overinst *)p->ce_bi;
	lc = (ldap_distproc_t *)on->on_bi.bi_private;

	assert( ca->be == NULL );
	ca->be = (BackendDB *)ch_calloc( 1, sizeof( BackendDB ) );

	ca->be->bd_info = (BackendInfo *)on;

	rc = slap_str2ad( "olcDbURI", &ad, &text );
	assert( rc == LDAP_SUCCESS );

	at = attr_find( e->e_attrs, ad );
	if ( lc->lc_common_li == NULL && at != NULL ) {
		/* FIXME: we should generate an empty default entry
		 * if none is supplied */
		Debug( LDAP_DEBUG_ANY, "slapd-distproc: "
			"first underlying database \"%s\" "
			"cannot contain attribute \"%s\".\n",
			e->e_name.bv_val, ad->ad_cname.bv_val, 0 );
		rc = LDAP_CONSTRAINT_VIOLATION;
		goto done;

	} else if ( lc->lc_common_li != NULL && at == NULL ) {
		/* FIXME: we should generate an empty default entry
		 * if none is supplied */
		Debug( LDAP_DEBUG_ANY, "slapd-distproc: "
			"subsequent underlying database \"%s\" "
			"must contain attribute \"%s\".\n",
			e->e_name.bv_val, ad->ad_cname.bv_val, 0 );
		rc = LDAP_CONSTRAINT_VIOLATION;
		goto done;
	}

	if ( lc->lc_common_li == NULL ) {
		rc = ldap_distproc_db_init_common( ca->be );

	} else {
		rc = ldap_distproc_db_init_one( ca->be );
	}

	if ( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY, "slapd-distproc: "
			"unable to init %sunderlying database \"%s\".\n",
			lc->lc_common_li == NULL ? "common " : "", e->e_name.bv_val, 0 );
		return LDAP_CONSTRAINT_VIOLATION;
	}

	li = ca->be->be_private;

	if ( lc->lc_common_li == NULL ) {
		lc->lc_common_li = li;

	} else if ( avl_insert( &lc->lc_lai.lai_tree, (caddr_t)li,
		ldap_distproc_uri_cmp, ldap_distproc_uri_dup ) )
	{
		Debug( LDAP_DEBUG_ANY, "slapd-distproc: "
			"database \"%s\" insert failed.\n",
			e->e_name.bv_val, 0, 0 );
		rc = LDAP_CONSTRAINT_VIOLATION;
		goto done;
	}

done:;
	if ( rc != LDAP_SUCCESS ) {
		(void)ldap_distproc_db_destroy_one( ca->be, NULL );
		ch_free( ca->be );
		ca->be = NULL;
	}

	return rc;
}

typedef struct ldap_distproc_cfadd_apply_t {
	Operation	*op;
	SlapReply	*rs;
	Entry		*p;
	ConfigArgs	*ca;
	int		count;
} ldap_distproc_cfadd_apply_t;

static int
ldap_distproc_cfadd_apply( void *datum, void *arg )
{
	ldapinfo_t			*li = (ldapinfo_t *)datum;
	ldap_distproc_cfadd_apply_t	*lca = (ldap_distproc_cfadd_apply_t *)arg;

	struct berval			bv;

	/* FIXME: should not hardcode "olcDatabase" here */
	bv.bv_len = snprintf( lca->ca->cr_msg, sizeof( lca->ca->cr_msg ),
		"olcDatabase={%d}%s", lca->count, lback->bi_type );
	bv.bv_val = lca->ca->cr_msg;

	lca->ca->be->be_private = (void *)li;
	config_build_entry( lca->op, lca->rs, lca->p->e_private, lca->ca,
		&bv, lback->bi_cf_ocs, &distproc_ocs[ 1 ] );

	lca->count++;

	return 0;
}

static int
distproc_cfadd( Operation *op, SlapReply *rs, Entry *p, ConfigArgs *ca )
{
	CfEntryInfo	*pe = p->e_private;
	slap_overinst	*on = (slap_overinst *)pe->ce_bi;
	ldap_distproc_t	*lc = (ldap_distproc_t *)on->on_bi.bi_private;
	void		*priv = (void *)ca->be->be_private;

	if ( lback->bi_cf_ocs ) {
		ldap_distproc_cfadd_apply_t	lca = { 0 };

		lca.op = op;
		lca.rs = rs;
		lca.p = p;
		lca.ca = ca;
		lca.count = 0;

		(void)ldap_distproc_cfadd_apply( (void *)lc->lc_common_li, (void *)&lca );

		(void)avl_apply( lc->lc_lai.lai_tree, ldap_distproc_cfadd_apply,
			&lca, 1, AVL_INORDER );

		ca->be->be_private = priv;
	}

	return 0;
}

static int
distproc_cfgen( ConfigArgs *c )
{
	slap_overinst	*on = (slap_overinst *)c->bi;
	ldap_distproc_t	*lc = (ldap_distproc_t *)on->on_bi.bi_private;

	int		rc = 0;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch( c->type ) {
		case DP_CACHE_URI:
			c->value_int = LDAP_DISTPROC_CACHE_URI( lc );
			break;

		default:
			assert( 0 );
			rc = 1;
		}
		return rc;

	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch( c->type ) {
		case DP_CHAINING:
			return 1;

		case DP_CACHE_URI:
			lc->lc_flags &= ~LDAP_DISTPROC_F_CACHE_URI;
			break;

		default:
			return 1;
		}
		return rc;
	}

	switch( c->type ) {
	case DP_CACHE_URI:
		if ( c->value_int ) {
			lc->lc_flags |= LDAP_DISTPROC_F_CACHE_URI;
		} else {
			lc->lc_flags &= ~LDAP_DISTPROC_F_CACHE_URI;
		}
		break;

	default:
		assert( 0 );
		return 1;
	}

	return rc;
}

static int
ldap_distproc_db_init(
	BackendDB *be,
	ConfigReply *cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	ldap_distproc_t	*lc = NULL;

	if ( lback == NULL ) {
		lback = backend_info( "ldap" );

		if ( lback == NULL ) {
			return 1;
		}
	}

	lc = ch_malloc( sizeof( ldap_distproc_t ) );
	if ( lc == NULL ) {
		return 1;
	}
	memset( lc, 0, sizeof( ldap_distproc_t ) );
	ldap_pvt_thread_mutex_init( &lc->lc_lai.lai_mutex );

	on->on_bi.bi_private = (void *)lc;

	return 0;
}

static int
ldap_distproc_db_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	ldap_distproc_t	*lc = (ldap_distproc_t *)on->on_bi.bi_private;

	int		rc = SLAP_CONF_UNKNOWN;
		
	if ( lc->lc_common_li == NULL ) {
		void	*be_private = be->be_private;
		ldap_distproc_db_init_common( be );
		lc->lc_common_li = lc->lc_cfg_li = (ldapinfo_t *)be->be_private;
		be->be_private = be_private;
	}

	/* Something for the distproc database? */
	if ( strncasecmp( argv[ 0 ], "distproc-", STRLENOF( "distproc-" ) ) == 0 ) {
		char		*save_argv0 = argv[ 0 ];
		BackendInfo	*bd_info = be->bd_info;
		void		*be_private = be->be_private;
		ConfigOCs	*be_cf_ocs = be->be_cf_ocs;
		int		is_uri = 0;

		argv[ 0 ] += STRLENOF( "distproc-" );

		if ( strcasecmp( argv[ 0 ], "uri" ) == 0 ) {
			rc = ldap_distproc_db_init_one( be );
			if ( rc != 0 ) {
				Debug( LDAP_DEBUG_ANY, "%s: line %d: "
					"underlying slapd-ldap initialization failed.\n.",
					fname, lineno, 0 );
				return 1;
			}
			lc->lc_cfg_li = be->be_private;
			is_uri = 1;
		}

		/* TODO: add checks on what other slapd-ldap(5) args
		 * should be put in the template; this is not quite
		 * harmful, because attributes that shouldn't don't
		 * get actually used, but the user should at least
		 * be warned.
		 */

		be->bd_info = lback;
		be->be_private = (void *)lc->lc_cfg_li;
		be->be_cf_ocs = lback->bi_cf_ocs;

		rc = config_generic_wrapper( be, fname, lineno, argc, argv );

		argv[ 0 ] = save_argv0;
		be->be_cf_ocs = be_cf_ocs;
		be->be_private = be_private;
		be->bd_info = bd_info;

		if ( is_uri ) {
private_destroy:;
			if ( rc != 0 ) {
				BackendDB		db = *be;

				db.bd_info = lback;
				db.be_private = (void *)lc->lc_cfg_li;
				ldap_distproc_db_destroy_one( &db, NULL );
				lc->lc_cfg_li = NULL;

			} else {
				if ( lc->lc_cfg_li->li_bvuri == NULL
					|| BER_BVISNULL( &lc->lc_cfg_li->li_bvuri[ 0 ] )
					|| !BER_BVISNULL( &lc->lc_cfg_li->li_bvuri[ 1 ] ) )
				{
					Debug( LDAP_DEBUG_ANY, "%s: line %d: "
						"no URI list allowed in slapo-distproc.\n",
						fname, lineno, 0 );
					rc = 1;
					goto private_destroy;
				}

				if ( avl_insert( &lc->lc_lai.lai_tree,
					(caddr_t)lc->lc_cfg_li,
					ldap_distproc_uri_cmp, ldap_distproc_uri_dup ) )
				{
					Debug( LDAP_DEBUG_ANY, "%s: line %d: "
						"duplicate URI in slapo-distproc.\n",
						fname, lineno, 0 );
					rc = 1;
					goto private_destroy;
				}
			}
		}
	}
	
	return rc;
}

enum db_which {
	db_open = 0,
	db_close,
	db_destroy,

	db_last
};

typedef struct ldap_distproc_db_apply_t {
	BackendDB	*be;
	BI_db_func	*func;
} ldap_distproc_db_apply_t;

static int
ldap_distproc_db_apply( void *datum, void *arg )
{
	ldapinfo_t		*li = (ldapinfo_t *)datum;
	ldap_distproc_db_apply_t	*lca = (ldap_distproc_db_apply_t *)arg;

	lca->be->be_private = (void *)li;

	return lca->func( lca->be, NULL );
}

static int
ldap_distproc_db_func(
	BackendDB *be,
	enum db_which which
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	ldap_distproc_t	*lc = (ldap_distproc_t *)on->on_bi.bi_private;

	int		rc = 0;

	if ( lc ) {
		BI_db_func	*func = (&lback->bi_db_open)[ which ];

		if ( func != NULL && lc->lc_common_li != NULL ) {
			BackendDB		db = *be;

			db.bd_info = lback;
			db.be_private = lc->lc_common_li;

			rc = func( &db, NULL );

			if ( rc != 0 ) {
				return rc;
			}

			if ( lc->lc_lai.lai_tree != NULL ) {
				ldap_distproc_db_apply_t	lca;

				lca.be = &db;
				lca.func = func;

				rc = avl_apply( lc->lc_lai.lai_tree,
					ldap_distproc_db_apply, (void *)&lca,
					1, AVL_INORDER ) != AVL_NOMORE;
			}
		}
	}

	return rc;
}

static int
ldap_distproc_db_open(
	BackendDB	*be,
	ConfigReply	*cr )
{
	return ldap_distproc_db_func( be, db_open );
}

static int
ldap_distproc_db_close(
	BackendDB	*be,
	ConfigReply	*cr )
{
	return ldap_distproc_db_func( be, db_close );
}

static int
ldap_distproc_db_destroy(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst	*on = (slap_overinst *) be->bd_info;
	ldap_distproc_t	*lc = (ldap_distproc_t *)on->on_bi.bi_private;

	int		rc;

	rc = ldap_distproc_db_func( be, db_destroy );

	if ( lc ) {
		avl_free( lc->lc_lai.lai_tree, NULL );
		ldap_pvt_thread_mutex_destroy( &lc->lc_lai.lai_mutex );
		ch_free( lc );
	}

	return rc;
}

/*
 * inits one instance of the slapd-ldap backend, and stores
 * the private info in be_private of the arg
 */
static int
ldap_distproc_db_init_common(
	BackendDB	*be )
{
	BackendInfo	*bi = be->bd_info;
	int		t;

	be->bd_info = lback;
	be->be_private = NULL;
	t = lback->bi_db_init( be, NULL );
	if ( t != 0 ) {
		return t;
	}
	be->bd_info = bi;

	return 0;
}

/*
 * inits one instance of the slapd-ldap backend, stores
 * the private info in be_private of the arg and fills
 * selected fields with data from the template.
 *
 * NOTE: add checks about the other fields of the template,
 * which are ignored and SHOULD NOT be configured by the user.
 */
static int
ldap_distproc_db_init_one(
	BackendDB	*be )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	ldap_distproc_t	*lc = (ldap_distproc_t *)on->on_bi.bi_private;

	BackendInfo	*bi = be->bd_info;
	ldapinfo_t	*li;

	slap_op_t	t;

	be->bd_info = lback;
	be->be_private = NULL;
	t = lback->bi_db_init( be, NULL );
	if ( t != 0 ) {
		return t;
	}
	li = (ldapinfo_t *)be->be_private;

	/* copy common data */
	li->li_nretries = lc->lc_common_li->li_nretries;
	li->li_flags = lc->lc_common_li->li_flags;
	li->li_version = lc->lc_common_li->li_version;
	for ( t = 0; t < SLAP_OP_LAST; t++ ) {
		li->li_timeout[ t ] = lc->lc_common_li->li_timeout[ t ];
	}
	be->bd_info = bi;

	return 0;
}

typedef struct ldap_distproc_conn_apply_t {
	BackendDB	*be;
	Connection	*conn;
} ldap_distproc_conn_apply_t;

static int
ldap_distproc_conn_apply( void *datum, void *arg )
{
	ldapinfo_t		*li = (ldapinfo_t *)datum;
	ldap_distproc_conn_apply_t	*lca = (ldap_distproc_conn_apply_t *)arg;

	lca->be->be_private = (void *)li;

	return lback->bi_connection_destroy( lca->be, lca->conn );
}

static int
ldap_distproc_connection_destroy(
	BackendDB *be,
	Connection *conn
)
{
	slap_overinst		*on = (slap_overinst *) be->bd_info;
	ldap_distproc_t		*lc = (ldap_distproc_t *)on->on_bi.bi_private;
	void			*private = be->be_private;
	ldap_distproc_conn_apply_t	lca;
	int			rc;

	be->be_private = NULL;
	lca.be = be;
	lca.conn = conn;
	ldap_pvt_thread_mutex_lock( &lc->lc_lai.lai_mutex );
	rc = avl_apply( lc->lc_lai.lai_tree, ldap_distproc_conn_apply,
		(void *)&lca, 1, AVL_INORDER ) != AVL_NOMORE;
	ldap_pvt_thread_mutex_unlock( &lc->lc_lai.lai_mutex );
	be->be_private = private;

	return rc;
}

static int
ldap_distproc_parse_returnContRef_ctrl(
	Operation	*op,
	SlapReply	*rs,
	LDAPControl	*ctrl )
{
	if ( get_returnContRef( op ) != SLAP_CONTROL_NONE ) {
		rs->sr_text = "returnContinuationReference control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( op->o_pagedresults != SLAP_CONTROL_NONE ) {
		rs->sr_text = "returnContinuationReference control specified with pagedResults control";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISEMPTY( &ctrl->ldctl_value ) ) {
		rs->sr_text = "returnContinuationReference control: value must be NULL";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_returnContRef = ctrl->ldctl_iscritical ? SLAP_CONTROL_CRITICAL : SLAP_CONTROL_NONCRITICAL;

	return LDAP_SUCCESS;
}

static int
ldap_exop_chained_request(
		Operation	*op,
		SlapReply	*rs )
{
	Statslog( LDAP_DEBUG_STATS, "%s CHAINED REQUEST\n",
	    op->o_log_prefix, 0, 0, 0, 0 );

	rs->sr_err = backend_check_restrictions( op, rs, 
			(struct berval *)&slap_EXOP_CHAINEDREQUEST );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		return rs->sr_err;
	}

	/* by now, just reject requests */
	rs->sr_text = "under development";
	return LDAP_UNWILLING_TO_PERFORM;
}


static slap_overinst distproc;

int
distproc_initialize( void )
{
	int	rc;

	/* Make sure we don't exceed the bits reserved for userland */
	config_check_userland( DP_LAST );

	rc = load_extop( (struct berval *)&slap_EXOP_CHAINEDREQUEST,
		SLAP_EXOP_HIDE, ldap_exop_chained_request );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "slapd-distproc: "
			"unable to register chainedRequest exop: %d.\n",
			rc, 0, 0 );
		return rc;
	}

	rc = supported_feature_load( &slap_FEATURE_CANCHAINOPS );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "slapd-distproc: "
			"unable to register canChainOperations supported feature: %d.\n",
			rc, 0, 0 );
		return rc;
	}

	rc = register_supported_control( LDAP_CONTROL_X_RETURNCONTREF,
			SLAP_CTRL_GLOBAL|SLAP_CTRL_ACCESS|SLAP_CTRL_HIDE, NULL,
			ldap_distproc_parse_returnContRef_ctrl, &sc_returnContRef );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "slapd-distproc: "
			"unable to register returnContinuationReference control: %d.\n",
			rc, 0, 0 );
		return rc;
	}

	distproc.on_bi.bi_type = "distproc";
	distproc.on_bi.bi_db_init = ldap_distproc_db_init;
	distproc.on_bi.bi_db_config = ldap_distproc_db_config;
	distproc.on_bi.bi_db_open = ldap_distproc_db_open;
	distproc.on_bi.bi_db_close = ldap_distproc_db_close;
	distproc.on_bi.bi_db_destroy = ldap_distproc_db_destroy;

	/* ... otherwise the underlying backend's function would be called,
	 * likely passing an invalid entry; on the contrary, the requested
	 * operational attributes should have been returned while chasing
	 * the referrals.  This all in all is a bit messy, because part
	 * of the operational attributes are generated by the backend;
	 * part by the frontend; back-ldap should receive all the available
	 * ones from the remote server, but then, on its own, it strips those
	 * it assumes will be (re)generated by the frontend (e.g.
	 * subschemaSubentry, entryDN, ...) */
	distproc.on_bi.bi_operational = ldap_distproc_operational;
	
	distproc.on_bi.bi_connection_destroy = ldap_distproc_connection_destroy;

	distproc.on_response = ldap_distproc_response;

	distproc.on_bi.bi_cf_ocs = distproc_ocs;

	rc = config_register_schema( distproc_cfg, distproc_ocs );
	if ( rc ) {
		return rc;
	}

	return overlay_register( &distproc );
}

#endif /* SLAP_DISTPROC */
