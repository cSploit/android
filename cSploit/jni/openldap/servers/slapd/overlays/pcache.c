/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
 * Portions Copyright 2003 IBM Corporation.
 * Portions Copyright 2003-2009 Symas Corporation.
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
 * This work was initially developed by Apurva Kumar for inclusion
 * in OpenLDAP Software and subsequently rewritten by Howard Chu.
 */

#include "portable.h"

#ifdef SLAPD_OVER_PROXYCACHE

#include <stdio.h>

#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "lutil.h"
#include "ldap_rq.h"
#include "avl.h"

#include "../back-monitor/back-monitor.h"

#include "config.h"

/*
 * Control that allows to access the private DB
 * instead of the public one
 */
#define	PCACHE_CONTROL_PRIVDB		"1.3.6.1.4.1.4203.666.11.9.5.1"

/*
 * Extended Operation that allows to remove a query from the cache
 */
#define PCACHE_EXOP_QUERY_DELETE	"1.3.6.1.4.1.4203.666.11.9.6.1"

/*
 * Monitoring
 */
#define PCACHE_MONITOR

/* query cache structs */
/* query */

typedef struct Query_s {
	Filter* 	filter; 	/* Search Filter */
	struct berval 	base; 		/* Search Base */
	int 		scope;		/* Search scope */
} Query;

struct query_template_s;

typedef struct Qbase_s {
	Avlnode *scopes[4];		/* threaded AVL trees of cached queries */
	struct berval base;
	int queries;
} Qbase;

/* struct representing a cached query */
typedef struct cached_query_s {
	Filter					*filter;
	Filter					*first;
	Qbase					*qbase;
	int						scope;
	struct berval			q_uuid;		/* query identifier */
	int						q_sizelimit;
	struct query_template_s		*qtemp;	/* template of the query */
	time_t						expiry_time;	/* time till the query is considered invalid */
	time_t						refresh_time;	/* time till the query is refreshed */
	time_t						bindref_time;	/* time till the bind is refreshed */
	int						bind_refcnt;	/* number of bind operation referencing this query */
	unsigned long			answerable_cnt; /* how many times it was answerable */
	int						refcnt;	/* references since last refresh */
	ldap_pvt_thread_mutex_t		answerable_cnt_mutex;
	struct cached_query_s  		*next;  	/* next query in the template */
	struct cached_query_s  		*prev;  	/* previous query in the template */
	struct cached_query_s		*lru_up;	/* previous query in the LRU list */
	struct cached_query_s		*lru_down;	/* next query in the LRU list */
	ldap_pvt_thread_rdwr_t		rwlock;
} CachedQuery;

/*
 * URL representation:
 *
 * ldap:///<base>??<scope>?<filter>?x-uuid=<uid>,x-template=<template>,x-attrset=<attrset>,x-expiry=<expiry>,x-refresh=<refresh>
 *
 * <base> ::= CachedQuery.qbase->base
 * <scope> ::= CachedQuery.scope
 * <filter> ::= filter2bv(CachedQuery.filter)
 * <uuid> ::= CachedQuery.q_uuid
 * <attrset> ::= CachedQuery.qtemp->attr_set_index
 * <expiry> ::= CachedQuery.expiry_time
 * <refresh> ::= CachedQuery.refresh_time
 *
 * quick hack: parse URI, call add_query() and then fix
 * CachedQuery.expiry_time and CachedQuery.q_uuid
 *
 * NOTE: if the <attrset> changes, all stored URLs will be invalidated.
 */

/*
 * Represents a set of projected attributes.
 */

struct attr_set {
	struct query_template_s *templates;
	AttributeName*	attrs; 		/* specifies the set */
	unsigned	flags;
#define	PC_CONFIGURED	(0x1)
#define	PC_REFERENCED	(0x2)
#define	PC_GOT_OC		(0x4)
	int 		count;		/* number of attributes */
};

/* struct representing a query template
 * e.g. template string = &(cn=)(mail=)
 */
typedef struct query_template_s {
	struct query_template_s *qtnext;
	struct query_template_s *qmnext;

	Avlnode*		qbase;
	CachedQuery* 	query;	        /* most recent query cached for the template */
	CachedQuery* 	query_last;     /* oldest query cached for the template */
	ldap_pvt_thread_rdwr_t t_rwlock; /* Rd/wr lock for accessing queries in the template */
	struct berval	querystr;	/* Filter string corresponding to the QT */
	struct berval	bindbase;	/* base DN for Bind request */
	struct berval	bindfilterstr;	/* Filter string for Bind request */
	struct berval	bindftemp;	/* bind filter template */
	Filter		*bindfilter;
	AttributeDescription **bindfattrs;	/* attrs to substitute in ftemp */

	int			bindnattrs;		/* number of bindfattrs */
	int			bindscope;
	int 		attr_set_index; /* determines the projected attributes */
	int 		no_of_queries;  /* Total number of queries in the template */
	time_t		ttl;		/* TTL for the queries of this template */
	time_t		negttl;		/* TTL for negative results */
	time_t		limitttl;	/* TTL for sizelimit exceeding results */
	time_t		ttr;	/* time to refresh */
	time_t		bindttr;	/* TTR for cached binds */
	struct attr_set t_attrs;	/* filter attrs + attr_set */
} QueryTemplate;

typedef enum {
	PC_IGNORE = 0,
	PC_POSITIVE,
	PC_NEGATIVE,
	PC_SIZELIMIT
} pc_caching_reason_t;

static const char *pc_caching_reason_str[] = {
	"IGNORE",
	"POSITIVE",
	"NEGATIVE",
	"SIZELIMIT",

	NULL
};

struct query_manager_s;

/* prototypes for functions for 1) query containment
 * 2) query addition, 3) cache replacement
 */
typedef CachedQuery *(QCfunc)(Operation *op, struct query_manager_s*,
	Query*, QueryTemplate*);
typedef CachedQuery *(AddQueryfunc)(Operation *op, struct query_manager_s*,
	Query*, QueryTemplate*, pc_caching_reason_t, int wlock);
typedef void (CRfunc)(struct query_manager_s*, struct berval*);

/* LDAP query cache */
typedef struct query_manager_s {
	struct attr_set* 	attr_sets;		/* possible sets of projected attributes */
	QueryTemplate*	  	templates;		/* cacheable templates */

	CachedQuery*		lru_top;		/* top and bottom of LRU list */
	CachedQuery*		lru_bottom;

	ldap_pvt_thread_mutex_t		lru_mutex;	/* mutex for accessing LRU list */

	/* Query cache methods */
	QCfunc			*qcfunc;			/* Query containment*/
	CRfunc 			*crfunc;			/* cache replacement */
	AddQueryfunc	*addfunc;			/* add query */
} query_manager;

/* LDAP query cache manager */
typedef struct cache_manager_s {
	BackendDB	db;	/* underlying database */
	unsigned long	num_cached_queries; 		/* total number of cached queries */
	unsigned long   max_queries;			/* upper bound on # of cached queries */
	int		save_queries;			/* save cached queries across restarts */
	int	check_cacheability;		/* check whether a query is cacheable */
	int 	numattrsets;			/* number of attribute sets */
	int 	cur_entries;			/* current number of entries cached */
	int 	max_entries;			/* max number of entries cached */
	int 	num_entries_limit;		/* max # of entries in a cacheable query */

	char	response_cb;			/* install the response callback
						 * at the tail of the callback list */
#define PCACHE_RESPONSE_CB_HEAD	0
#define PCACHE_RESPONSE_CB_TAIL	1
	char	defer_db_open;			/* defer open for online add */
	char	cache_binds;			/* cache binds or just passthru */

	time_t	cc_period;		/* interval between successive consistency checks (sec) */
#define PCACHE_CC_PAUSED	1
#define PCACHE_CC_OFFLINE	2
	int 	cc_paused;
	void	*cc_arg;

	ldap_pvt_thread_mutex_t		cache_mutex;

	query_manager*   qm;	/* query cache managed by the cache manager */

#ifdef PCACHE_MONITOR
	void		*monitor_cb;
	struct berval	monitor_ndn;
#endif /* PCACHE_MONITOR */
} cache_manager;

#ifdef PCACHE_MONITOR
static int pcache_monitor_db_init( BackendDB *be );
static int pcache_monitor_db_open( BackendDB *be );
static int pcache_monitor_db_close( BackendDB *be );
static int pcache_monitor_db_destroy( BackendDB *be );
#endif /* PCACHE_MONITOR */

static int pcache_debug;

#ifdef PCACHE_CONTROL_PRIVDB
static int privDB_cid;
#endif /* PCACHE_CONTROL_PRIVDB */

static AttributeDescription	*ad_queryId, *ad_cachedQueryURL;

#ifdef PCACHE_MONITOR
static AttributeDescription	*ad_numQueries, *ad_numEntries;
static ObjectClass		*oc_olmPCache;
#endif /* PCACHE_MONITOR */

static struct {
	char			*name;
	char			*oid;
}		s_oid[] = {
	{ "PCacheOID",			"1.3.6.1.4.1.4203.666.11.9.1" },
	{ "PCacheAttributes",		"PCacheOID:1" },
	{ "PCacheObjectClasses",	"PCacheOID:2" },

	{ NULL }
};

static struct {
	char	*desc;
	AttributeDescription **adp;
} s_ad[] = {
	{ "( PCacheAttributes:1 "
		"NAME 'pcacheQueryID' "
		"DESC 'ID of query the entry belongs to, formatted as a UUID' "
		"EQUALITY octetStringMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40{64} "
		"NO-USER-MODIFICATION "
		"USAGE directoryOperation )",
		&ad_queryId },
	{ "( PCacheAttributes:2 "
		"NAME 'pcacheQueryURL' "
		"DESC 'URI describing a cached query' "
		"EQUALITY caseExactMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
		"NO-USER-MODIFICATION "
		"USAGE directoryOperation )",
		&ad_cachedQueryURL },
#ifdef PCACHE_MONITOR
	{ "( PCacheAttributes:3 "
		"NAME 'pcacheNumQueries' "
		"DESC 'Number of cached queries' "
		"EQUALITY integerMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 "
		"NO-USER-MODIFICATION "
		"USAGE directoryOperation )",
		&ad_numQueries },
	{ "( PCacheAttributes:4 "
		"NAME 'pcacheNumEntries' "
		"DESC 'Number of cached entries' "
		"EQUALITY integerMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 "
		"NO-USER-MODIFICATION "
		"USAGE directoryOperation )",
		&ad_numEntries },
#endif /* PCACHE_MONITOR */

	{ NULL }
};

static struct {
	char		*desc;
	ObjectClass	**ocp;
}		s_oc[] = {
#ifdef PCACHE_MONITOR
	/* augments an existing object, so it must be AUXILIARY */
	{ "( PCacheObjectClasses:1 "
		"NAME ( 'olmPCache' ) "
		"SUP top AUXILIARY "
		"MAY ( "
			"pcacheQueryURL "
			"$ pcacheNumQueries "
			"$ pcacheNumEntries "
			" ) )",
		&oc_olmPCache },
#endif /* PCACHE_MONITOR */

	{ NULL }
};

static int
filter2template(
	Operation		*op,
	Filter			*f,
	struct			berval *fstr );

static CachedQuery *
add_query(
	Operation *op,
	query_manager* qm,
	Query* query,
	QueryTemplate *templ,
	pc_caching_reason_t why,
	int wlock);

static int
remove_query_data(
	Operation	*op,
	struct berval	*query_uuid );

/*
 * Turn a cached query into its URL representation
 */
static int
query2url( Operation *op, CachedQuery *q, struct berval *urlbv, int dolock )
{
	struct berval	bv_scope,
			bv_filter;
	char		attrset_buf[ LDAP_PVT_INTTYPE_CHARS( unsigned long ) ],
			expiry_buf[ LDAP_PVT_INTTYPE_CHARS( unsigned long ) ],
			refresh_buf[ LDAP_PVT_INTTYPE_CHARS( unsigned long ) ],
			answerable_buf[ LDAP_PVT_INTTYPE_CHARS( unsigned long ) ],
			*ptr;
	ber_len_t	attrset_len,
			expiry_len,
			refresh_len,
			answerable_len;

	if ( dolock ) {
		ldap_pvt_thread_rdwr_rlock( &q->rwlock );
	}

	ldap_pvt_scope2bv( q->scope, &bv_scope );
	filter2bv_x( op, q->filter, &bv_filter );
	attrset_len = sprintf( attrset_buf,
		"%lu", (unsigned long)q->qtemp->attr_set_index );
	expiry_len = sprintf( expiry_buf,
		"%lu", (unsigned long)q->expiry_time );
	answerable_len = snprintf( answerable_buf, sizeof( answerable_buf ),
		"%lu", q->answerable_cnt );
	if ( q->refresh_time )
		refresh_len = sprintf( refresh_buf,
			"%lu", (unsigned long)q->refresh_time );
	else
		refresh_len = 0;

	urlbv->bv_len = STRLENOF( "ldap:///" )
		+ q->qbase->base.bv_len
		+ STRLENOF( "??" )
		+ bv_scope.bv_len
		+ STRLENOF( "?" )
		+ bv_filter.bv_len
		+ STRLENOF( "?x-uuid=" )
		+ q->q_uuid.bv_len
		+ STRLENOF( ",x-attrset=" )
		+ attrset_len
		+ STRLENOF( ",x-expiry=" )
		+ expiry_len
		+ STRLENOF( ",x-answerable=" )
		+ answerable_len;
	if ( refresh_len )
		urlbv->bv_len += STRLENOF( ",x-refresh=" )
		+ refresh_len;

	ptr = urlbv->bv_val = ber_memalloc_x( urlbv->bv_len + 1, op->o_tmpmemctx );
	ptr = lutil_strcopy( ptr, "ldap:///" );
	ptr = lutil_strcopy( ptr, q->qbase->base.bv_val );
	ptr = lutil_strcopy( ptr, "??" );
	ptr = lutil_strcopy( ptr, bv_scope.bv_val );
	ptr = lutil_strcopy( ptr, "?" );
	ptr = lutil_strcopy( ptr, bv_filter.bv_val );
	ptr = lutil_strcopy( ptr, "?x-uuid=" );
	ptr = lutil_strcopy( ptr, q->q_uuid.bv_val );
	ptr = lutil_strcopy( ptr, ",x-attrset=" );
	ptr = lutil_strcopy( ptr, attrset_buf );
	ptr = lutil_strcopy( ptr, ",x-expiry=" );
	ptr = lutil_strcopy( ptr, expiry_buf );
	ptr = lutil_strcopy( ptr, ",x-answerable=" );
	ptr = lutil_strcopy( ptr, answerable_buf );
	if ( refresh_len ) {
		ptr = lutil_strcopy( ptr, ",x-refresh=" );
		ptr = lutil_strcopy( ptr, refresh_buf );
	}

	ber_memfree_x( bv_filter.bv_val, op->o_tmpmemctx );

	if ( dolock ) {
		ldap_pvt_thread_rdwr_runlock( &q->rwlock );
	}

	return 0;
}

/* Find and record the empty filter clauses */

static int
ftemp_attrs( struct berval *ftemp, struct berval *template,
	AttributeDescription ***ret, const char **text )
{
	int i;
	int attr_cnt=0;
	struct berval bv;
	char *p1, *p2, *t1;
	AttributeDescription *ad;
	AttributeDescription **descs = NULL;
	char *temp2;

	temp2 = ch_malloc( ftemp->bv_len + 1 );
	p1 = ftemp->bv_val;
	t1 = temp2;

	*ret = NULL;

	for (;;) {
		while ( *p1 == '(' || *p1 == '&' || *p1 == '|' || *p1 == ')' )
			*t1++ = *p1++;

		p2 = strchr( p1, '=' );
		if ( !p2 )
			break;
		i = p2 - p1;
		AC_MEMCPY( t1, p1, i );
		t1 += i;
		*t1++ = '=';

		if ( p2[-1] == '<' || p2[-1] == '>' ) p2--;
		bv.bv_val = p1;
		bv.bv_len = p2 - p1;
		ad = NULL;
		i = slap_bv2ad( &bv, &ad, text );
		if ( i ) {
			ch_free( descs );
			return -1;
		}
		if ( *p2 == '<' || *p2 == '>' ) p2++;
		if ( p2[1] != ')' ) {
			p2++;
			while ( *p2 != ')' ) p2++;
			p1 = p2;
			continue;
		}

		descs = (AttributeDescription **)ch_realloc(descs,
				(attr_cnt + 2)*sizeof(AttributeDescription *));

		descs[attr_cnt++] = ad;

		p1 = p2+1;
	}
	*t1 = '\0';
	descs[attr_cnt] = NULL;
	*ret = descs;
	template->bv_val = temp2;
	template->bv_len = t1 - temp2;
	return attr_cnt;
}

static int
template_attrs( char *template, struct attr_set *set, AttributeName **ret,
	const char **text )
{
	int got_oc = 0;
	int alluser = 0;
	int allop = 0;
	int i;
	int attr_cnt;
	int t_cnt = 0;
	struct berval bv;
	char *p1, *p2;
	AttributeDescription *ad;
	AttributeName *attrs;

	p1 = template;

	*ret = NULL;

	attrs = ch_calloc( set->count + 1, sizeof(AttributeName) );
	for ( i=0; i < set->count; i++ )
		attrs[i] = set->attrs[i];
	attr_cnt = i;
	alluser = an_find( attrs, slap_bv_all_user_attrs );
	allop = an_find( attrs, slap_bv_all_operational_attrs );

	for (;;) {
		while ( *p1 == '(' || *p1 == '&' || *p1 == '|' || *p1 == ')' ) p1++;
		p2 = strchr( p1, '=' );
		if ( !p2 )
			break;
		if ( p2[-1] == '<' || p2[-1] == '>' ) p2--;
		bv.bv_val = p1;
		bv.bv_len = p2 - p1;
		ad = NULL;
		i = slap_bv2ad( &bv, &ad, text );
		if ( i ) {
			ch_free( attrs );
			return -1;
		}
		t_cnt++;

		if ( ad == slap_schema.si_ad_objectClass )
			got_oc = 1;

		if ( is_at_operational(ad->ad_type)) {
			if ( allop ) {
				goto bottom;
			}
		} else if ( alluser ) {
			goto bottom;
		}
		if ( !ad_inlist( ad, attrs )) {
			attrs = (AttributeName *)ch_realloc(attrs,
					(attr_cnt + 2)*sizeof(AttributeName));

			attrs[attr_cnt].an_desc = ad;
			attrs[attr_cnt].an_name = ad->ad_cname;
			attrs[attr_cnt].an_oc = NULL;
			attrs[attr_cnt].an_flags = 0;
			BER_BVZERO( &attrs[attr_cnt+1].an_name );
			attr_cnt++;
		}

bottom:
		p1 = p2+2;
	}
	if ( !t_cnt ) {
		*text = "couldn't parse template";
		return -1;
	}
	if ( !got_oc && !( set->flags & PC_GOT_OC )) {
		attrs = (AttributeName *)ch_realloc(attrs,
				(attr_cnt + 2)*sizeof(AttributeName));

		ad = slap_schema.si_ad_objectClass;
		attrs[attr_cnt].an_desc = ad;
		attrs[attr_cnt].an_name = ad->ad_cname;
		attrs[attr_cnt].an_oc = NULL;
		attrs[attr_cnt].an_flags = 0;
		BER_BVZERO( &attrs[attr_cnt+1].an_name );
		attr_cnt++;
	}
	*ret = attrs;
	return attr_cnt;
}

/*
 * Turn an URL representing a formerly cached query into a cached query,
 * and try to cache it
 */
static int
url2query(
	char		*url,
	Operation	*op,
	query_manager	*qm )
{
	Query		query = { 0 };
	QueryTemplate	*qt;
	CachedQuery	*cq;
	LDAPURLDesc	*lud = NULL;
	struct berval	base,
			tempstr = BER_BVNULL,
			uuid = BER_BVNULL;
	int		attrset;
	time_t		expiry_time;
	time_t		refresh_time;
	unsigned long	answerable_cnt;
	int		i,
			got = 0,
#define GOT_UUID	0x1U
#define GOT_ATTRSET	0x2U
#define GOT_EXPIRY	0x4U
#define GOT_ANSWERABLE	0x8U
#define GOT_REFRESH	0x10U
#define GOT_ALL		(GOT_UUID|GOT_ATTRSET|GOT_EXPIRY|GOT_ANSWERABLE)
			rc = 0;

	rc = ldap_url_parse( url, &lud );
	if ( rc != LDAP_URL_SUCCESS ) {
		return -1;
	}

	/* non-allowed fields */
	if ( lud->lud_host != NULL ) {
		rc = 1;
		goto error;
	}

	if ( lud->lud_attrs != NULL ) {
		rc = 1;
		goto error;
	}

	/* be pedantic */
	if ( strcmp( lud->lud_scheme, "ldap" ) != 0 ) {
		rc = 1;
		goto error;
	}

	/* required fields */
	if ( lud->lud_dn == NULL || lud->lud_dn[ 0 ] == '\0' ) {
		rc = 1;
		goto error;
	}

	switch ( lud->lud_scope ) {
	case LDAP_SCOPE_BASE:
	case LDAP_SCOPE_ONELEVEL:
	case LDAP_SCOPE_SUBTREE:
	case LDAP_SCOPE_SUBORDINATE:
		break;

	default:
		rc = 1;
		goto error;
	}

	if ( lud->lud_filter == NULL || lud->lud_filter[ 0 ] == '\0' ) {
		rc = 1;
		goto error;
	}

	if ( lud->lud_exts == NULL ) {
		rc = 1;
		goto error;
	}

	for ( i = 0; lud->lud_exts[ i ] != NULL; i++ ) {
		if ( strncmp( lud->lud_exts[ i ], "x-uuid=", STRLENOF( "x-uuid=" ) ) == 0 ) {
			struct berval	tmpUUID;
			Syntax		*syn_UUID = slap_schema.si_ad_entryUUID->ad_type->sat_syntax;

			if ( got & GOT_UUID ) {
				rc = 1;
				goto error;
			}

			ber_str2bv( &lud->lud_exts[ i ][ STRLENOF( "x-uuid=" ) ], 0, 0, &tmpUUID );
			if ( !BER_BVISEMPTY( &tmpUUID ) ) {
				rc = syn_UUID->ssyn_pretty( syn_UUID, &tmpUUID, &uuid, NULL );
				if ( rc != LDAP_SUCCESS ) {
					goto error;
				}
			}
			got |= GOT_UUID;

		} else if ( strncmp( lud->lud_exts[ i ], "x-attrset=", STRLENOF( "x-attrset=" ) ) == 0 ) {
			if ( got & GOT_ATTRSET ) {
				rc = 1;
				goto error;
			}

			rc = lutil_atoi( &attrset, &lud->lud_exts[ i ][ STRLENOF( "x-attrset=" ) ] );
			if ( rc ) {
				goto error;
			}
			got |= GOT_ATTRSET;

		} else if ( strncmp( lud->lud_exts[ i ], "x-expiry=", STRLENOF( "x-expiry=" ) ) == 0 ) {
			unsigned long l;

			if ( got & GOT_EXPIRY ) {
				rc = 1;
				goto error;
			}

			rc = lutil_atoul( &l, &lud->lud_exts[ i ][ STRLENOF( "x-expiry=" ) ] );
			if ( rc ) {
				goto error;
			}
			expiry_time = (time_t)l;
			got |= GOT_EXPIRY;

		} else if ( strncmp( lud->lud_exts[ i ], "x-answerable=", STRLENOF( "x-answerable=" ) ) == 0 ) {
			if ( got & GOT_ANSWERABLE ) {
				rc = 1;
				goto error;
			}

			rc = lutil_atoul( &answerable_cnt, &lud->lud_exts[ i ][ STRLENOF( "x-answerable=" ) ] );
			if ( rc ) {
				goto error;
			}
			got |= GOT_ANSWERABLE;

		} else if ( strncmp( lud->lud_exts[ i ], "x-refresh=", STRLENOF( "x-refresh=" ) ) == 0 ) {
			unsigned long l;

			if ( got & GOT_REFRESH ) {
				rc = 1;
				goto error;
			}

			rc = lutil_atoul( &l, &lud->lud_exts[ i ][ STRLENOF( "x-refresh=" ) ] );
			if ( rc ) {
				goto error;
			}
			refresh_time = (time_t)l;
			got |= GOT_REFRESH;

		} else {
			rc = -1;
			goto error;
		}
	}

	if ( got != GOT_ALL ) {
		rc = 1;
		goto error;
	}

	if ( !(got & GOT_REFRESH ))
		refresh_time = 0;

	/* ignore expired queries */
	if ( expiry_time <= slap_get_time()) {
		Operation	op2 = *op;

		memset( &op2.oq_search, 0, sizeof( op2.oq_search ) );

		(void)remove_query_data( &op2, &uuid );

		rc = 0;

	} else {
		ber_str2bv( lud->lud_dn, 0, 0, &base );
		rc = dnNormalize( 0, NULL, NULL, &base, &query.base, NULL );
		if ( rc != LDAP_SUCCESS ) {
			goto error;
		}
		query.scope = lud->lud_scope;
		query.filter = str2filter( lud->lud_filter );
		if ( query.filter == NULL ) {
			rc = -1;
			goto error;
		}

		tempstr.bv_val = ch_malloc( strlen( lud->lud_filter ) + 1 );
		tempstr.bv_len = 0;
		if ( filter2template( op, query.filter, &tempstr ) ) {
			ch_free( tempstr.bv_val );
			rc = -1;
			goto error;
		}

		/* check for query containment */
		qt = qm->attr_sets[attrset].templates;
		for ( ; qt; qt = qt->qtnext ) {
			/* find if template i can potentially answer tempstr */
			if ( bvmatch( &qt->querystr, &tempstr ) ) {
				break;
			}
		}

		if ( qt == NULL ) {
			rc = 1;
			goto error;
		}

		cq = add_query( op, qm, &query, qt, PC_POSITIVE, 0 );
		if ( cq != NULL ) {
			cq->expiry_time = expiry_time;
			cq->refresh_time = refresh_time;
			cq->q_uuid = uuid;
			cq->answerable_cnt = answerable_cnt;
			cq->refcnt = 0;

			/* it's now into cq->filter */
			BER_BVZERO( &uuid );
			query.filter = NULL;

		} else {
			rc = 1;
		}
	}

error:;
	if ( query.filter != NULL ) filter_free( query.filter );
	if ( !BER_BVISNULL( &tempstr ) ) ch_free( tempstr.bv_val );
	if ( !BER_BVISNULL( &query.base ) ) ch_free( query.base.bv_val );
	if ( !BER_BVISNULL( &uuid ) ) ch_free( uuid.bv_val );
	if ( lud != NULL ) ldap_free_urldesc( lud );

	return rc;
}

/* Return 1 for an added entry, else 0 */
static int
merge_entry(
	Operation		*op,
	Entry			*e,
	int			dup,
	struct berval*		query_uuid )
{
	int		rc;
	Modifications* modlist = NULL;
	const char* 	text = NULL;
	Attribute		*attr;
	char			textbuf[SLAP_TEXT_BUFLEN];
	size_t			textlen = sizeof(textbuf);

	SlapReply sreply = {REP_RESULT};

	slap_callback cb = { NULL, slap_null_cb, NULL, NULL };

	if ( dup )
		e = entry_dup( e );
	attr = e->e_attrs;
	e->e_attrs = NULL;

	/* add queryId attribute */
	attr_merge_one( e, ad_queryId, query_uuid, NULL );

	/* append the attribute list from the fetched entry */
	e->e_attrs->a_next = attr;

	op->o_tag = LDAP_REQ_ADD;
	op->o_protocol = LDAP_VERSION3;
	op->o_callback = &cb;
	op->o_time = slap_get_time();
	op->o_do_not_cache = 1;

	op->ora_e = e;
	op->o_req_dn = e->e_name;
	op->o_req_ndn = e->e_nname;
	rc = op->o_bd->be_add( op, &sreply );

	if ( rc != LDAP_SUCCESS ) {
		if ( rc == LDAP_ALREADY_EXISTS ) {
			rs_reinit( &sreply, REP_RESULT );
			slap_entry2mods( e, &modlist, &text, textbuf, textlen );
			modlist->sml_op = LDAP_MOD_ADD;
			op->o_tag = LDAP_REQ_MODIFY;
			op->orm_modlist = modlist;
			op->o_managedsait = SLAP_CONTROL_CRITICAL;
			op->o_bd->be_modify( op, &sreply );
			slap_mods_free( modlist, 1 );
		} else if ( rc == LDAP_REFERRAL ||
					rc == LDAP_NO_SUCH_OBJECT ) {
			syncrepl_add_glue( op, e );
			e = NULL;
			rc = 1;
		}
		if ( e ) {
			entry_free( e );
			rc = 0;
		}
	} else {
		if ( op->ora_e == e )
			entry_free( e );
		rc = 1;
	}

	return rc;
}

/* Length-ordered sort on normalized DNs */
static int pcache_dn_cmp( const void *v1, const void *v2 )
{
	const Qbase *q1 = v1, *q2 = v2;

	int rc = q1->base.bv_len - q2->base.bv_len;
	if ( rc == 0 )
		rc = strncmp( q1->base.bv_val, q2->base.bv_val, q1->base.bv_len );
	return rc;
}

static int lex_bvcmp( struct berval *bv1, struct berval *bv2 )
{
	int len, dif;
	dif = bv1->bv_len - bv2->bv_len;
	len = bv1->bv_len;
	if ( dif > 0 ) len -= dif;
	len = memcmp( bv1->bv_val, bv2->bv_val, len );
	if ( !len )
		len = dif;
	return len;
}

/* compare the current value in each filter */
static int pcache_filter_cmp( Filter *f1, Filter *f2 )
{
	int rc, weight1, weight2;

	switch( f1->f_choice ) {
	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
		weight1 = 0;
		break;
	case LDAP_FILTER_PRESENT:
		weight1 = 1;
		break;
	case LDAP_FILTER_EQUALITY:
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
		weight1 = 2;
		break;
	default:
		weight1 = 3;
	}
	switch( f2->f_choice ) {
	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
		weight2 = 0;
		break;
	case LDAP_FILTER_PRESENT:
		weight2 = 1;
		break;
	case LDAP_FILTER_EQUALITY:
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
		weight2 = 2;
		break;
	default:
		weight2 = 3;
	}
	rc = weight1 - weight2;
	if ( !rc ) {
		switch( weight1 ) {
		case 0:
			rc = pcache_filter_cmp( f1->f_and, f2->f_and );
			break;
		case 1:
			break;
		case 2:
			rc = lex_bvcmp( &f1->f_av_value, &f2->f_av_value );
			break;
		case 3:
			if ( f1->f_choice == LDAP_FILTER_SUBSTRINGS ) {
				rc = 0;
				if ( !BER_BVISNULL( &f1->f_sub_initial )) {
					if ( !BER_BVISNULL( &f2->f_sub_initial )) {
						rc = lex_bvcmp( &f1->f_sub_initial,
							&f2->f_sub_initial );
					} else {
						rc = 1;
					}
				} else if ( !BER_BVISNULL( &f2->f_sub_initial )) {
					rc = -1;
				}
				if ( rc ) break;
				if ( f1->f_sub_any ) {
					if ( f2->f_sub_any ) {
						rc = lex_bvcmp( f1->f_sub_any,
							f2->f_sub_any );
					} else {
						rc = 1;
					}
				} else if ( f2->f_sub_any ) {
					rc = -1;
				}
				if ( rc ) break;
				if ( !BER_BVISNULL( &f1->f_sub_final )) {
					if ( !BER_BVISNULL( &f2->f_sub_final )) {
						rc = lex_bvcmp( &f1->f_sub_final,
							&f2->f_sub_final );
					} else {
						rc = 1;
					}
				} else if ( !BER_BVISNULL( &f2->f_sub_final )) {
					rc = -1;
				}
			} else {
				rc = lex_bvcmp( &f1->f_mr_value,
					&f2->f_mr_value );
			}
			break;
		}
		while ( !rc ) {
			f1 = f1->f_next;
			f2 = f2->f_next;
			if ( f1 || f2 ) {
				if ( !f1 )
					rc = -1;
				else if ( !f2 )
					rc = 1;
				else {
					rc = pcache_filter_cmp( f1, f2 );
				}
			} else {
				break;
			}
		}
	}
	return rc;
}

/* compare filters in each query */
static int pcache_query_cmp( const void *v1, const void *v2 )
{
	const CachedQuery *q1 = v1, *q2 =v2;
	return pcache_filter_cmp( q1->filter, q2->filter );
}

/* add query on top of LRU list */
static void
add_query_on_top (query_manager* qm, CachedQuery* qc)
{
	CachedQuery* top = qm->lru_top;

	qm->lru_top = qc;

	if (top)
		top->lru_up = qc;
	else
		qm->lru_bottom = qc;

	qc->lru_down = top;
	qc->lru_up = NULL;
	Debug( pcache_debug, "Base of added query = %s\n",
			qc->qbase->base.bv_val, 0, 0 );
}

/* remove_query from LRU list */

static void
remove_query (query_manager* qm, CachedQuery* qc)
{
	CachedQuery* up;
	CachedQuery* down;

	if (!qc)
		return;

	up = qc->lru_up;
	down = qc->lru_down;

	if (!up)
		qm->lru_top = down;

	if (!down)
		qm->lru_bottom = up;

	if (down)
		down->lru_up = up;

	if (up)
		up->lru_down = down;

	qc->lru_up = qc->lru_down = NULL;
}

/* find and remove string2 from string1
 * from start if position = 1,
 * from end if position = 3,
 * from anywhere if position = 2
 * string1 is overwritten if position = 2.
 */

static int
find_and_remove(struct berval* ber1, struct berval* ber2, int position)
{
	int ret=0;

	if ( !ber2->bv_val )
		return 1;
	if ( !ber1->bv_val )
		return 0;

	switch( position ) {
	case 1:
		if ( ber1->bv_len >= ber2->bv_len && !memcmp( ber1->bv_val,
			ber2->bv_val, ber2->bv_len )) {
			ret = 1;
			ber1->bv_val += ber2->bv_len;
			ber1->bv_len -= ber2->bv_len;
		}
		break;
	case 2: {
		char *temp;
		ber1->bv_val[ber1->bv_len] = '\0';
		temp = strstr( ber1->bv_val, ber2->bv_val );
		if ( temp ) {
			strcpy( temp, temp+ber2->bv_len );
			ber1->bv_len -= ber2->bv_len;
			ret = 1;
		}
		break;
		}
	case 3:
		if ( ber1->bv_len >= ber2->bv_len &&
			!memcmp( ber1->bv_val+ber1->bv_len-ber2->bv_len, ber2->bv_val,
				ber2->bv_len )) {
			ret = 1;
			ber1->bv_len -= ber2->bv_len;
		}
		break;
	}
	return ret;
}


static struct berval*
merge_init_final(Operation *op, struct berval* init, struct berval* any,
	struct berval* final)
{
	struct berval* merged, *temp;
	int i, any_count, count;

	for (any_count=0; any && any[any_count].bv_val; any_count++)
		;

	count = any_count;

	if (init->bv_val)
		count++;
	if (final->bv_val)
		count++;

	merged = (struct berval*)op->o_tmpalloc( (count+1)*sizeof(struct berval),
		op->o_tmpmemctx );
	temp = merged;

	if (init->bv_val) {
		ber_dupbv_x( temp, init, op->o_tmpmemctx );
		temp++;
	}

	for (i=0; i<any_count; i++) {
		ber_dupbv_x( temp, any, op->o_tmpmemctx );
		temp++; any++;
	}

	if (final->bv_val){
		ber_dupbv_x( temp, final, op->o_tmpmemctx );
		temp++;
	}
	BER_BVZERO( temp );
	return merged;
}

/* Each element in stored must be found in incoming. Incoming is overwritten.
 */
static int
strings_containment(struct berval* stored, struct berval* incoming)
{
	struct berval* element;
	int k=0;
	int j, rc = 0;

	for ( element=stored; element->bv_val != NULL; element++ ) {
		for (j = k; incoming[j].bv_val != NULL; j++) {
			if (find_and_remove(&(incoming[j]), element, 2)) {
				k = j;
				rc = 1;
				break;
			}
			rc = 0;
		}
		if ( rc ) {
			continue;
		} else {
			return 0;
		}
	}
	return 1;
}

static int
substr_containment_substr(Operation *op, Filter* stored, Filter* incoming)
{
	int rc = 0;

	struct berval init_incoming;
	struct berval final_incoming;
	struct berval *remaining_incoming = NULL;

	if ((!(incoming->f_sub_initial.bv_val) && (stored->f_sub_initial.bv_val))
	   || (!(incoming->f_sub_final.bv_val) && (stored->f_sub_final.bv_val)))
		return 0;

	init_incoming = incoming->f_sub_initial;
	final_incoming =  incoming->f_sub_final;

	if (find_and_remove(&init_incoming,
			&(stored->f_sub_initial), 1) && find_and_remove(&final_incoming,
			&(stored->f_sub_final), 3))
	{
		if (stored->f_sub_any == NULL) {
			rc = 1;
			goto final;
		}
		remaining_incoming = merge_init_final(op, &init_incoming,
						incoming->f_sub_any, &final_incoming);
		rc = strings_containment(stored->f_sub_any, remaining_incoming);
		ber_bvarray_free_x( remaining_incoming, op->o_tmpmemctx );
	}
final:
	return rc;
}

static int
substr_containment_equality(Operation *op, Filter* stored, Filter* incoming)
{
	struct berval incoming_val[2];
	int rc = 0;

	incoming_val[1] = incoming->f_av_value;

	if (find_and_remove(incoming_val+1,
			&(stored->f_sub_initial), 1) && find_and_remove(incoming_val+1,
			&(stored->f_sub_final), 3)) {
		if (stored->f_sub_any == NULL){
			rc = 1;
			goto final;
		}
		ber_dupbv_x( incoming_val, incoming_val+1, op->o_tmpmemctx );
		BER_BVZERO( incoming_val+1 );
		rc = strings_containment(stored->f_sub_any, incoming_val);
		op->o_tmpfree( incoming_val[0].bv_val, op->o_tmpmemctx );
	}
final:
	return rc;
}

static Filter *
filter_first( Filter *f )
{
	while ( f->f_choice == LDAP_FILTER_OR || f->f_choice == LDAP_FILTER_AND )
		f = f->f_and;
	return f;
}

typedef struct fstack {
	struct fstack *fs_next;
	Filter *fs_fs;
	Filter *fs_fi;
} fstack;

static CachedQuery *
find_filter( Operation *op, Avlnode *root, Filter *inputf, Filter *first )
{
	Filter* fs;
	Filter* fi;
	MatchingRule* mrule = NULL;
	int res=0, eqpass= 0;
	int ret, rc, dir;
	Avlnode *ptr;
	CachedQuery cq, *qc;
	fstack *stack = NULL, *fsp;

	cq.filter = inputf;
	cq.first = first;

	/* substring matches sort to the end, and we just have to
	 * walk the entire list.
	 */
	if ( first->f_choice == LDAP_FILTER_SUBSTRINGS ) {
		ptr = tavl_end( root, 1 );
		dir = TAVL_DIR_LEFT;
	} else {
		ptr = tavl_find3( root, &cq, pcache_query_cmp, &ret );
		dir = (first->f_choice == LDAP_FILTER_GE) ? TAVL_DIR_LEFT :
			TAVL_DIR_RIGHT;
	}

	while (ptr) {
		qc = ptr->avl_data;
		fi = inputf;
		fs = qc->filter;

		/* an incoming substr query can only be satisfied by a cached
		 * substr query.
		 */
		if ( first->f_choice == LDAP_FILTER_SUBSTRINGS &&
			qc->first->f_choice != LDAP_FILTER_SUBSTRINGS )
			break;

		/* an incoming eq query can be satisfied by a cached eq or substr
		 * query
		 */
		if ( first->f_choice == LDAP_FILTER_EQUALITY ) {
			if ( eqpass == 0 ) {
				if ( qc->first->f_choice != LDAP_FILTER_EQUALITY ) {
nextpass:			eqpass = 1;
					ptr = tavl_end( root, 1 );
					dir = TAVL_DIR_LEFT;
					continue;
				}
			} else {
				if ( qc->first->f_choice != LDAP_FILTER_SUBSTRINGS )
					break;
			}
		}
		do {
			res=0;
			switch (fs->f_choice) {
			case LDAP_FILTER_EQUALITY:
				if (fi->f_choice == LDAP_FILTER_EQUALITY)
					mrule = fs->f_ava->aa_desc->ad_type->sat_equality;
				else
					ret = 1;
				break;
			case LDAP_FILTER_GE:
			case LDAP_FILTER_LE:
				mrule = fs->f_ava->aa_desc->ad_type->sat_ordering;
				break;
			default:
				mrule = NULL; 
			}
			if (mrule) {
				const char *text;
				rc = value_match(&ret, fs->f_ava->aa_desc, mrule,
					SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
					&(fi->f_ava->aa_value),
					&(fs->f_ava->aa_value), &text);
				if (rc != LDAP_SUCCESS) {
					return NULL;
				}
				if ( fi==first && fi->f_choice==LDAP_FILTER_EQUALITY && ret )
					goto nextpass;
			}
			switch (fs->f_choice) {
			case LDAP_FILTER_OR:
			case LDAP_FILTER_AND:
				if ( fs->f_next ) {
					/* save our stack position */
					fsp = op->o_tmpalloc(sizeof(fstack), op->o_tmpmemctx);
					fsp->fs_next = stack;
					fsp->fs_fs = fs->f_next;
					fsp->fs_fi = fi->f_next;
					stack = fsp;
				}
				fs = fs->f_and;
				fi = fi->f_and;
				res=1;
				break;
			case LDAP_FILTER_SUBSTRINGS:
				/* check if the equality query can be
				* answered with cached substring query */
				if ((fi->f_choice == LDAP_FILTER_EQUALITY)
					&& substr_containment_equality( op,
					fs, fi))
					res=1;
				/* check if the substring query can be
				* answered with cached substring query */
				if ((fi->f_choice ==LDAP_FILTER_SUBSTRINGS
					) && substr_containment_substr( op,
					fs, fi))
					res= 1;
				fs=fs->f_next;
				fi=fi->f_next;
				break;
			case LDAP_FILTER_PRESENT:
				res=1;
				fs=fs->f_next;
				fi=fi->f_next;
				break;
			case LDAP_FILTER_EQUALITY:
				if (ret == 0)
					res = 1;
				fs=fs->f_next;
				fi=fi->f_next;
				break;
			case LDAP_FILTER_GE:
				if (mrule && ret >= 0)
					res = 1;
				fs=fs->f_next;
				fi=fi->f_next;
				break;
			case LDAP_FILTER_LE:
				if (mrule && ret <= 0)
					res = 1;
				fs=fs->f_next;
				fi=fi->f_next;
				break;
			case LDAP_FILTER_NOT:
				res=0;
				break;
			default:
				break;
			}
			if (!fs && !fi && stack) {
				/* pop the stack */
				fsp = stack;
				stack = fsp->fs_next;
				fs = fsp->fs_fs;
				fi = fsp->fs_fi;
				op->o_tmpfree(fsp, op->o_tmpmemctx);
			}
		} while((res) && (fi != NULL) && (fs != NULL));

		if ( res )
			return qc;
		ptr = tavl_next( ptr, dir );
	}
	return NULL;
}

/* check whether query is contained in any of
 * the cached queries in template
 */
static CachedQuery *
query_containment(Operation *op, query_manager *qm,
		  Query *query,
		  QueryTemplate *templa)
{
	CachedQuery* qc;
	int depth = 0, tscope;
	Qbase qbase, *qbptr = NULL;
	struct berval pdn;

	if (query->filter != NULL) {
		Filter *first;

		Debug( pcache_debug, "Lock QC index = %p\n",
				(void *) templa, 0, 0 );
		qbase.base = query->base;

		first = filter_first( query->filter );

		ldap_pvt_thread_rdwr_rlock(&templa->t_rwlock);
		for( ;; ) {
			/* Find the base */
			qbptr = avl_find( templa->qbase, &qbase, pcache_dn_cmp );
			if ( qbptr ) {
				tscope = query->scope;
				/* Find a matching scope:
				 * match at depth 0 OK
				 * scope is BASE,
				 *	one at depth 1 OK
				 *  subord at depth > 0 OK
				 *	subtree at any depth OK
				 * scope is ONE,
				 *  subtree or subord at any depth OK
				 * scope is SUBORD,
				 *  subtree or subord at any depth OK
				 * scope is SUBTREE,
				 *  subord at depth > 0 OK
				 *  subtree at any depth OK
				 */
				for ( tscope = 0 ; tscope <= LDAP_SCOPE_CHILDREN; tscope++ ) {
					switch ( query->scope ) {
					case LDAP_SCOPE_BASE:
						if ( tscope == LDAP_SCOPE_BASE && depth ) continue;
						if ( tscope == LDAP_SCOPE_ONE && depth != 1) continue;
						if ( tscope == LDAP_SCOPE_CHILDREN && !depth ) continue;
						break;
					case LDAP_SCOPE_ONE:
						if ( tscope == LDAP_SCOPE_BASE )
							tscope = LDAP_SCOPE_ONE;
						if ( tscope == LDAP_SCOPE_ONE && depth ) continue;
						if ( !depth ) break;
						if ( tscope < LDAP_SCOPE_SUBTREE )
							tscope = LDAP_SCOPE_SUBTREE;
						break;
					case LDAP_SCOPE_SUBTREE:
						if ( tscope < LDAP_SCOPE_SUBTREE )
							tscope = LDAP_SCOPE_SUBTREE;
						if ( tscope == LDAP_SCOPE_CHILDREN && !depth ) continue;
						break;
					case LDAP_SCOPE_CHILDREN:
						if ( tscope < LDAP_SCOPE_SUBTREE )
							tscope = LDAP_SCOPE_SUBTREE;
						break;
					}
					if ( !qbptr->scopes[tscope] ) continue;

					/* Find filter */
					qc = find_filter( op, qbptr->scopes[tscope],
							query->filter, first );
					if ( qc ) {
						if ( qc->q_sizelimit ) {
							ldap_pvt_thread_rdwr_runlock(&templa->t_rwlock);
							return NULL;
						}
						ldap_pvt_thread_mutex_lock(&qm->lru_mutex);
						if (qm->lru_top != qc) {
							remove_query(qm, qc);
							add_query_on_top(qm, qc);
						}
						ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);
						return qc;
					}
				}
			}
			if ( be_issuffix( op->o_bd, &qbase.base ))
				break;
			/* Up a level */
			dnParent( &qbase.base, &pdn );
			qbase.base = pdn;
			depth++;
		}

		Debug( pcache_debug,
			"Not answerable: Unlock QC index=%p\n",
			(void *) templa, 0, 0 );
		ldap_pvt_thread_rdwr_runlock(&templa->t_rwlock);
	}
	return NULL;
}

static void
free_query (CachedQuery* qc)
{
	free(qc->q_uuid.bv_val);
	filter_free(qc->filter);
	ldap_pvt_thread_mutex_destroy(&qc->answerable_cnt_mutex);
	ldap_pvt_thread_rdwr_destroy( &qc->rwlock );
	memset(qc, 0, sizeof(*qc));
	free(qc);
}


/* Add query to query cache, the returned Query is locked for writing */
static CachedQuery *
add_query(
	Operation *op,
	query_manager* qm,
	Query* query,
	QueryTemplate *templ,
	pc_caching_reason_t why,
	int wlock)
{
	CachedQuery* new_cached_query = (CachedQuery*) ch_malloc(sizeof(CachedQuery));
	Qbase *qbase, qb;
	Filter *first;
	int rc;
	time_t ttl = 0, ttr = 0;
	time_t now;

	new_cached_query->qtemp = templ;
	BER_BVZERO( &new_cached_query->q_uuid );
	new_cached_query->q_sizelimit = 0;

	now = slap_get_time();
	switch ( why ) {
	case PC_POSITIVE:
		ttl = templ->ttl;
		if ( templ->ttr )
			ttr = now + templ->ttr;
		break;

	case PC_NEGATIVE:
		ttl = templ->negttl;
		break;

	case PC_SIZELIMIT:
		ttl = templ->limitttl;
		break;

	default:
		assert( 0 );
		break;
	}
	new_cached_query->expiry_time = now + ttl;
	new_cached_query->refresh_time = ttr;
	new_cached_query->bindref_time = 0;

	new_cached_query->bind_refcnt = 0;
	new_cached_query->answerable_cnt = 0;
	new_cached_query->refcnt = 1;
	ldap_pvt_thread_mutex_init(&new_cached_query->answerable_cnt_mutex);

	new_cached_query->lru_up = NULL;
	new_cached_query->lru_down = NULL;
	Debug( pcache_debug, "Added query expires at %ld (%s)\n",
			(long) new_cached_query->expiry_time,
			pc_caching_reason_str[ why ], 0 );

	new_cached_query->scope = query->scope;
	new_cached_query->filter = query->filter;
	new_cached_query->first = first = filter_first( query->filter );
	
	ldap_pvt_thread_rdwr_init(&new_cached_query->rwlock);
	if (wlock)
		ldap_pvt_thread_rdwr_wlock(&new_cached_query->rwlock);

	qb.base = query->base;

	/* Adding a query    */
	Debug( pcache_debug, "Lock AQ index = %p\n",
			(void *) templ, 0, 0 );
	ldap_pvt_thread_rdwr_wlock(&templ->t_rwlock);
	qbase = avl_find( templ->qbase, &qb, pcache_dn_cmp );
	if ( !qbase ) {
		qbase = ch_calloc( 1, sizeof(Qbase) + qb.base.bv_len + 1 );
		qbase->base.bv_len = qb.base.bv_len;
		qbase->base.bv_val = (char *)(qbase+1);
		memcpy( qbase->base.bv_val, qb.base.bv_val, qb.base.bv_len );
		qbase->base.bv_val[qbase->base.bv_len] = '\0';
		avl_insert( &templ->qbase, qbase, pcache_dn_cmp, avl_dup_error );
	}
	new_cached_query->next = templ->query;
	new_cached_query->prev = NULL;
	new_cached_query->qbase = qbase;
	rc = tavl_insert( &qbase->scopes[query->scope], new_cached_query,
		pcache_query_cmp, avl_dup_error );
	if ( rc == 0 ) {
		qbase->queries++;
		if (templ->query == NULL)
			templ->query_last = new_cached_query;
		else
			templ->query->prev = new_cached_query;
		templ->query = new_cached_query;
		templ->no_of_queries++;
	} else {
		ldap_pvt_thread_mutex_destroy(&new_cached_query->answerable_cnt_mutex);
		if (wlock)
			ldap_pvt_thread_rdwr_wunlock(&new_cached_query->rwlock);
		ldap_pvt_thread_rdwr_destroy( &new_cached_query->rwlock );
		ch_free( new_cached_query );
		new_cached_query = find_filter( op, qbase->scopes[query->scope],
							query->filter, first );
		filter_free( query->filter );
		query->filter = NULL;
	}
	Debug( pcache_debug, "TEMPLATE %p QUERIES++ %d\n",
			(void *) templ, templ->no_of_queries, 0 );

	/* Adding on top of LRU list  */
	if ( rc == 0 ) {
		ldap_pvt_thread_mutex_lock(&qm->lru_mutex);
		add_query_on_top(qm, new_cached_query);
		ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);
	}
	Debug( pcache_debug, "Unlock AQ index = %p \n",
			(void *) templ, 0, 0 );
	ldap_pvt_thread_rdwr_wunlock(&templ->t_rwlock);

	return rc == 0 ? new_cached_query : NULL;
}

static void
remove_from_template (CachedQuery* qc, QueryTemplate* template)
{
	if (!qc->prev && !qc->next) {
		template->query_last = template->query = NULL;
	} else if (qc->prev == NULL) {
		qc->next->prev = NULL;
		template->query = qc->next;
	} else if (qc->next == NULL) {
		qc->prev->next = NULL;
		template->query_last = qc->prev;
	} else {
		qc->next->prev = qc->prev;
		qc->prev->next = qc->next;
	}
	tavl_delete( &qc->qbase->scopes[qc->scope], qc, pcache_query_cmp );
	qc->qbase->queries--;
	if ( qc->qbase->queries == 0 ) {
		avl_delete( &template->qbase, qc->qbase, pcache_dn_cmp );
		ch_free( qc->qbase );
		qc->qbase = NULL;
	}

	template->no_of_queries--;
}

/* remove bottom query of LRU list from the query cache */
/*
 * NOTE: slight change in functionality.
 *
 * - if result->bv_val is NULL, the query at the bottom of the LRU
 *   is removed
 * - otherwise, the query whose UUID is *result is removed
 *	- if not found, result->bv_val is zeroed
 */
static void
cache_replacement(query_manager* qm, struct berval *result)
{
	CachedQuery* bottom;
	QueryTemplate *temp;

	ldap_pvt_thread_mutex_lock(&qm->lru_mutex);
	if ( BER_BVISNULL( result ) ) {
		bottom = qm->lru_bottom;

		if (!bottom) {
			Debug ( pcache_debug,
				"Cache replacement invoked without "
				"any query in LRU list\n", 0, 0, 0 );
			ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);
			return;
		}

	} else {
		for ( bottom = qm->lru_bottom;
			bottom != NULL;
			bottom = bottom->lru_up )
		{
			if ( bvmatch( result, &bottom->q_uuid ) ) {
				break;
			}
		}

		if ( !bottom ) {
			Debug ( pcache_debug,
				"Could not find query with uuid=\"%s\""
				"in LRU list\n", result->bv_val, 0, 0 );
			ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);
			BER_BVZERO( result );
			return;
		}
	}

	temp = bottom->qtemp;
	remove_query(qm, bottom);
	ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);

	*result = bottom->q_uuid;
	BER_BVZERO( &bottom->q_uuid );

	Debug( pcache_debug, "Lock CR index = %p\n", (void *) temp, 0, 0 );
	ldap_pvt_thread_rdwr_wlock(&temp->t_rwlock);
	remove_from_template(bottom, temp);
	Debug( pcache_debug, "TEMPLATE %p QUERIES-- %d\n",
		(void *) temp, temp->no_of_queries, 0 );
	Debug( pcache_debug, "Unlock CR index = %p\n", (void *) temp, 0, 0 );
	ldap_pvt_thread_rdwr_wunlock(&temp->t_rwlock);
	free_query(bottom);
}

struct query_info {
	struct query_info *next;
	struct berval xdn;
	int del;
};

static int
remove_func (
	Operation	*op,
	SlapReply	*rs
)
{
	Attribute *attr;
	struct query_info *qi;
	int count = 0;

	if ( rs->sr_type != REP_SEARCH ) return 0;

	attr = attr_find( rs->sr_entry->e_attrs,  ad_queryId );
	if ( attr == NULL ) return 0;

	count = attr->a_numvals;
	assert( count > 0 );
	qi = op->o_tmpalloc( sizeof( struct query_info ), op->o_tmpmemctx );
	qi->next = op->o_callback->sc_private;
	op->o_callback->sc_private = qi;
	ber_dupbv_x( &qi->xdn, &rs->sr_entry->e_nname, op->o_tmpmemctx );
	qi->del = ( count == 1 );

	return 0;
}

static int
remove_query_data(
	Operation	*op,
	struct berval	*query_uuid )
{
	struct query_info	*qi, *qnext;
	char			filter_str[ LDAP_LUTIL_UUIDSTR_BUFSIZE + STRLENOF( "(pcacheQueryID=)" ) ];
	AttributeAssertion	ava = ATTRIBUTEASSERTION_INIT;
	Filter			filter = {LDAP_FILTER_EQUALITY};
	SlapReply 		sreply = {REP_RESULT};
	slap_callback cb = { NULL, remove_func, NULL, NULL };
	int deleted = 0;

	op->ors_filterstr.bv_len = snprintf(filter_str, sizeof(filter_str),
		"(%s=%s)", ad_queryId->ad_cname.bv_val, query_uuid->bv_val);
	filter.f_ava = &ava;
	filter.f_av_desc = ad_queryId;
	filter.f_av_value = *query_uuid;

	op->o_tag = LDAP_REQ_SEARCH;
	op->o_protocol = LDAP_VERSION3;
	op->o_callback = &cb;
	op->o_time = slap_get_time();
	op->o_do_not_cache = 1;

	op->o_req_dn = op->o_bd->be_suffix[0];
	op->o_req_ndn = op->o_bd->be_nsuffix[0];
	op->ors_scope = LDAP_SCOPE_SUBTREE;
	op->ors_deref = LDAP_DEREF_NEVER;
	op->ors_slimit = SLAP_NO_LIMIT;
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_limit = NULL;
	op->ors_filter = &filter;
	op->ors_filterstr.bv_val = filter_str;
	op->ors_filterstr.bv_len = strlen(filter_str);
	op->ors_attrs = NULL;
	op->ors_attrsonly = 0;

	op->o_bd->be_search( op, &sreply );

	for ( qi=cb.sc_private; qi; qi=qnext ) {
		qnext = qi->next;

		op->o_req_dn = qi->xdn;
		op->o_req_ndn = qi->xdn;
		rs_reinit( &sreply, REP_RESULT );

		if ( qi->del ) {
			Debug( pcache_debug, "DELETING ENTRY TEMPLATE=%s\n",
				query_uuid->bv_val, 0, 0 );

			op->o_tag = LDAP_REQ_DELETE;

			if (op->o_bd->be_delete(op, &sreply) == LDAP_SUCCESS) {
				deleted++;
			}

		} else {
			Modifications mod;
			struct berval vals[2];

			vals[0] = *query_uuid;
			vals[1].bv_val = NULL;
			vals[1].bv_len = 0;
			mod.sml_op = LDAP_MOD_DELETE;
			mod.sml_flags = 0;
			mod.sml_desc = ad_queryId;
			mod.sml_type = ad_queryId->ad_cname;
			mod.sml_values = vals;
			mod.sml_nvalues = NULL;
                        mod.sml_numvals = 1;
			mod.sml_next = NULL;
			Debug( pcache_debug,
				"REMOVING TEMP ATTR : TEMPLATE=%s\n",
				query_uuid->bv_val, 0, 0 );

			op->orm_modlist = &mod;

			op->o_bd->be_modify( op, &sreply );
		}
		op->o_tmpfree( qi->xdn.bv_val, op->o_tmpmemctx );
		op->o_tmpfree( qi, op->o_tmpmemctx );
	}
	return deleted;
}

static int
get_attr_set(
	AttributeName* attrs,
	query_manager* qm,
	int num
);

static int
filter2template(
	Operation		*op,
	Filter			*f,
	struct			berval *fstr )
{
	AttributeDescription *ad;
	int len, ret;

	switch ( f->f_choice ) {
	case LDAP_FILTER_EQUALITY:
		ad = f->f_av_desc;
		len = STRLENOF( "(=)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s=)", ad->ad_cname.bv_val );
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_GE:
		ad = f->f_av_desc;
		len = STRLENOF( "(>=)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s>=)", ad->ad_cname.bv_val);
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_LE:
		ad = f->f_av_desc;
		len = STRLENOF( "(<=)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s<=)", ad->ad_cname.bv_val);
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_APPROX:
		ad = f->f_av_desc;
		len = STRLENOF( "(~=)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s~=)", ad->ad_cname.bv_val);
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_SUBSTRINGS:
		ad = f->f_sub_desc;
		len = STRLENOF( "(=)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s=)", ad->ad_cname.bv_val );
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_PRESENT:
		ad = f->f_desc;
		len = STRLENOF( "(=*)" ) + ad->ad_cname.bv_len;
		ret = snprintf( fstr->bv_val+fstr->bv_len, len + 1, "(%s=*)", ad->ad_cname.bv_val );
		assert( ret == len );
		fstr->bv_len += len;
		break;

	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
	case LDAP_FILTER_NOT: {
		int rc = 0;
		fstr->bv_val[fstr->bv_len++] = '(';
		switch ( f->f_choice ) {
		case LDAP_FILTER_AND:
			fstr->bv_val[fstr->bv_len] = '&';
			break;
		case LDAP_FILTER_OR:
			fstr->bv_val[fstr->bv_len] = '|';
			break;
		case LDAP_FILTER_NOT:
			fstr->bv_val[fstr->bv_len] = '!';
			break;
		}
		fstr->bv_len++;

		for ( f = f->f_list; f != NULL; f = f->f_next ) {
			rc = filter2template( op, f, fstr );
			if ( rc ) break;
		}
		fstr->bv_val[fstr->bv_len++] = ')';
		fstr->bv_val[fstr->bv_len] = '\0';

		return rc;
		}

	default:
		/* a filter should at least have room for "()",
		 * an "=" and for a 1-char attr */
		strcpy( fstr->bv_val, "(?=)" );
		fstr->bv_len += STRLENOF("(?=)");
		return -1;
	}

	return 0;
}

#define	BI_HASHED	0x01
#define	BI_DIDCB	0x02
#define	BI_LOOKUP	0x04

struct search_info;

typedef struct bindinfo {
	cache_manager *bi_cm;
	CachedQuery *bi_cq;
	QueryTemplate *bi_templ;
	struct search_info *bi_si;
	int bi_flags;
	slap_callback bi_cb;
} bindinfo;

struct search_info {
	slap_overinst *on;
	Query query;
	QueryTemplate *qtemp;
	AttributeName*  save_attrs;	/* original attributes, saved for response */
	int swap_saved_attrs;
	int max;
	int over;
	int count;
	int slimit;
	int slimit_exceeded;
	pc_caching_reason_t caching_reason;
	Entry *head, *tail;
	bindinfo *pbi;
};

static void
remove_query_and_data(
	Operation	*op,
	cache_manager	*cm,
	struct berval	*uuid )
{
	query_manager*		qm = cm->qm;

	qm->crfunc( qm, uuid );
	if ( !BER_BVISNULL( uuid ) ) {
		int	return_val;

		Debug( pcache_debug,
			"Removing query UUID %s\n",
			uuid->bv_val, 0, 0 );
		return_val = remove_query_data( op, uuid );
		Debug( pcache_debug,
			"QUERY REMOVED, SIZE=%d\n",
			return_val, 0, 0);
		ldap_pvt_thread_mutex_lock( &cm->cache_mutex );
		cm->cur_entries -= return_val;
		cm->num_cached_queries--;
		Debug( pcache_debug,
			"STORED QUERIES = %lu\n",
			cm->num_cached_queries, 0, 0 );
		ldap_pvt_thread_mutex_unlock( &cm->cache_mutex );
		Debug( pcache_debug,
			"QUERY REMOVED, CACHE ="
			"%d entries\n",
			cm->cur_entries, 0, 0 );
	}
}

/*
 * Callback used to fetch queryId values based on entryUUID;
 * used by pcache_remove_entries_from_cache()
 */
static int
fetch_queryId_cb( Operation *op, SlapReply *rs )
{
	int		rc = 0;

	/* only care about searchEntry responses */
	if ( rs->sr_type != REP_SEARCH ) {
		return 0;
	}

	/* allow only one response per entryUUID */
	if ( op->o_callback->sc_private != NULL ) {
		rc = 1;

	} else {
		Attribute	*a;

		/* copy all queryId values into callback's private data */
		a = attr_find( rs->sr_entry->e_attrs, ad_queryId );
		if ( a != NULL ) {
			BerVarray	vals = NULL;

			ber_bvarray_dup_x( &vals, a->a_nvals, op->o_tmpmemctx );
			op->o_callback->sc_private = (void *)vals;
		}
	}

	/* clear entry if required */
	rs_flush_entry( op, rs, (slap_overinst *) op->o_bd->bd_info );

	return rc;
}

/*
 * Call that allows to remove a set of entries from the cache,
 * by forcing the removal of all the related queries.
 */
int
pcache_remove_entries_from_cache(
	Operation	*op,
	cache_manager	*cm,
	BerVarray	entryUUIDs )
{
	Connection	conn = { 0 };
	OperationBuffer opbuf;
	Operation	op2;
	slap_callback	sc = { 0 };
	Filter		f = { 0 };
	char		filtbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE + STRLENOF( "(entryUUID=)" ) ];
	AttributeAssertion ava = ATTRIBUTEASSERTION_INIT;
	AttributeName	attrs[ 2 ] = {{{ 0 }}};
	int		s, rc;

	if ( op == NULL ) {
		void	*thrctx = ldap_pvt_thread_pool_context();

		connection_fake_init( &conn, &opbuf, thrctx );
		op = &opbuf.ob_op;

	} else {
		op2 = *op;
		op = &op2;
	}

	memset( &op->oq_search, 0, sizeof( op->oq_search ) );
	op->ors_scope = LDAP_SCOPE_SUBTREE;
	op->ors_deref = LDAP_DEREF_NEVER;
	f.f_choice = LDAP_FILTER_EQUALITY;
	f.f_ava = &ava;
	ava.aa_desc = slap_schema.si_ad_entryUUID;
	op->ors_filter = &f;
	op->ors_slimit = 1;
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_limit = NULL;
	attrs[ 0 ].an_desc = ad_queryId;
	attrs[ 0 ].an_name = ad_queryId->ad_cname;
	op->ors_attrs = attrs;
	op->ors_attrsonly = 0;

	op->o_req_dn = cm->db.be_suffix[ 0 ];
	op->o_req_ndn = cm->db.be_nsuffix[ 0 ];

	op->o_tag = LDAP_REQ_SEARCH;
	op->o_protocol = LDAP_VERSION3;
	op->o_managedsait = SLAP_CONTROL_CRITICAL;
	op->o_bd = &cm->db;
	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;
	sc.sc_response = fetch_queryId_cb;
	op->o_callback = &sc;

	for ( s = 0; !BER_BVISNULL( &entryUUIDs[ s ] ); s++ ) {
		BerVarray	vals = NULL;
		SlapReply	rs = { REP_RESULT };

		op->ors_filterstr.bv_len = snprintf( filtbuf, sizeof( filtbuf ),
			"(entryUUID=%s)", entryUUIDs[ s ].bv_val );
		op->ors_filterstr.bv_val = filtbuf;
		ava.aa_value = entryUUIDs[ s ];

		rc = op->o_bd->be_search( op, &rs );
		if ( rc != LDAP_SUCCESS ) {
			continue;
		}

		vals = (BerVarray)op->o_callback->sc_private;
		if ( vals != NULL ) {
			int		i;

			for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
				struct berval	val = vals[ i ];

				remove_query_and_data( op, cm, &val );

				if ( !BER_BVISNULL( &val ) && val.bv_val != vals[ i ].bv_val ) {
					ch_free( val.bv_val );
				}
			}

			ber_bvarray_free_x( vals, op->o_tmpmemctx );
			op->o_callback->sc_private = NULL;
		}
	}

	return 0;
}

/*
 * Call that allows to remove a query from the cache.
 */
int
pcache_remove_query_from_cache(
	Operation	*op,
	cache_manager	*cm,
	struct berval	*queryid )
{
	Operation	op2 = *op;

	op2.o_bd = &cm->db;

	/* remove the selected query */
	remove_query_and_data( &op2, cm, queryid );

	return LDAP_SUCCESS;
}

/*
 * Call that allows to remove a set of queries related to an entry 
 * from the cache; if queryid is not null, the entry must belong to
 * the query indicated by queryid.
 */
int
pcache_remove_entry_queries_from_cache(
	Operation	*op,
	cache_manager	*cm,
	struct berval	*ndn,
	struct berval	*queryid )
{
	Connection		conn = { 0 };
	OperationBuffer 	opbuf;
	Operation		op2;
	slap_callback		sc = { 0 };
	SlapReply		rs = { REP_RESULT };
	Filter			f = { 0 };
	char			filter_str[ LDAP_LUTIL_UUIDSTR_BUFSIZE + STRLENOF( "(pcacheQueryID=)" ) ];
	AttributeAssertion	ava = ATTRIBUTEASSERTION_INIT;
	AttributeName		attrs[ 2 ] = {{{ 0 }}};
	int			rc;

	BerVarray		vals = NULL;

	if ( op == NULL ) {
		void	*thrctx = ldap_pvt_thread_pool_context();

		connection_fake_init( &conn, &opbuf, thrctx );
		op = &opbuf.ob_op;

	} else {
		op2 = *op;
		op = &op2;
	}

	memset( &op->oq_search, 0, sizeof( op->oq_search ) );
	op->ors_scope = LDAP_SCOPE_BASE;
	op->ors_deref = LDAP_DEREF_NEVER;
	if ( queryid == NULL || BER_BVISNULL( queryid ) ) {
		BER_BVSTR( &op->ors_filterstr, "(objectClass=*)" );
		f.f_choice = LDAP_FILTER_PRESENT;
		f.f_desc = slap_schema.si_ad_objectClass;

	} else {
		op->ors_filterstr.bv_len = snprintf( filter_str,
			sizeof( filter_str ), "(%s=%s)",
			ad_queryId->ad_cname.bv_val, queryid->bv_val );
		f.f_choice = LDAP_FILTER_EQUALITY;
		f.f_ava = &ava;
		f.f_av_desc = ad_queryId;
		f.f_av_value = *queryid;
	}
	op->ors_filter = &f;
	op->ors_slimit = 1;
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_limit = NULL;
	attrs[ 0 ].an_desc = ad_queryId;
	attrs[ 0 ].an_name = ad_queryId->ad_cname;
	op->ors_attrs = attrs;
	op->ors_attrsonly = 0;

	op->o_req_dn = *ndn;
	op->o_req_ndn = *ndn;

	op->o_tag = LDAP_REQ_SEARCH;
	op->o_protocol = LDAP_VERSION3;
	op->o_managedsait = SLAP_CONTROL_CRITICAL;
	op->o_bd = &cm->db;
	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;
	sc.sc_response = fetch_queryId_cb;
	op->o_callback = &sc;

	rc = op->o_bd->be_search( op, &rs );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	vals = (BerVarray)op->o_callback->sc_private;
	if ( vals != NULL ) {
		int		i;

		for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
			struct berval	val = vals[ i ];

			remove_query_and_data( op, cm, &val );

			if ( !BER_BVISNULL( &val ) && val.bv_val != vals[ i ].bv_val ) {
				ch_free( val.bv_val );
			}
		}

		ber_bvarray_free_x( vals, op->o_tmpmemctx );
	}

	return LDAP_SUCCESS;
}

static int
cache_entries(
	Operation	*op,
	struct berval *query_uuid )
{
	struct search_info *si = op->o_callback->sc_private;
	slap_overinst *on = si->on;
	cache_manager *cm = on->on_bi.bi_private;
	int		return_val = 0;
	Entry		*e;
	struct berval	crp_uuid;
	char		uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];
	Operation	*op_tmp;
	Connection	conn = {0};
	OperationBuffer opbuf;
	void		*thrctx = ldap_pvt_thread_pool_context();

	query_uuid->bv_len = lutil_uuidstr(uuidbuf, sizeof(uuidbuf));
	ber_str2bv(uuidbuf, query_uuid->bv_len, 1, query_uuid);

	connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	op_tmp = &opbuf.ob_op;
	op_tmp->o_bd = &cm->db;
	op_tmp->o_dn = cm->db.be_rootdn;
	op_tmp->o_ndn = cm->db.be_rootndn;

	Debug( pcache_debug, "UUID for query being added = %s\n",
			uuidbuf, 0, 0 );

	for ( e=si->head; e; e=si->head ) {
		si->head = e->e_private;
		e->e_private = NULL;
		while ( cm->cur_entries > (cm->max_entries) ) {
			BER_BVZERO( &crp_uuid );
			remove_query_and_data( op_tmp, cm, &crp_uuid );
		}

		return_val = merge_entry(op_tmp, e, 0, query_uuid);
		ldap_pvt_thread_mutex_lock(&cm->cache_mutex);
		cm->cur_entries += return_val;
		Debug( pcache_debug,
			"ENTRY ADDED/MERGED, CACHED ENTRIES=%d\n",
			cm->cur_entries, 0, 0 );
		return_val = 0;
		ldap_pvt_thread_mutex_unlock(&cm->cache_mutex);
	}

	return return_val;
}

static int
pcache_op_cleanup( Operation *op, SlapReply *rs ) {
	slap_callback	*cb = op->o_callback;
	struct search_info *si = cb->sc_private;
	slap_overinst *on = si->on;
	cache_manager *cm = on->on_bi.bi_private;
	query_manager*		qm = cm->qm;

	if ( rs->sr_type == REP_RESULT || 
		op->o_abandon || rs->sr_err == SLAPD_ABANDON )
	{
		if ( si->swap_saved_attrs ) {
			rs->sr_attrs = si->save_attrs;
			op->ors_attrs = si->save_attrs;
		}
		if ( (op->o_abandon || rs->sr_err == SLAPD_ABANDON) && 
				si->caching_reason == PC_IGNORE )
		{
			filter_free( si->query.filter );
			if ( si->count ) {
				/* duplicate query, free it */
				Entry *e;
				for (;si->head; si->head=e) {
					e = si->head->e_private;
					si->head->e_private = NULL;
					entry_free(si->head);
				}
			}

		} else if ( si->caching_reason != PC_IGNORE ) {
			CachedQuery *qc = qm->addfunc(op, qm, &si->query,
				si->qtemp, si->caching_reason, 1 );

			if ( qc != NULL ) {
				switch ( si->caching_reason ) {
				case PC_POSITIVE:
					cache_entries( op, &qc->q_uuid );
					if ( si->pbi ) {
						qc->bind_refcnt++;
						si->pbi->bi_cq = qc;
					}
					break;

				case PC_SIZELIMIT:
					qc->q_sizelimit = rs->sr_nentries;
					break;

				case PC_NEGATIVE:
					break;

				default:
					assert( 0 );
					break;
				}
				ldap_pvt_thread_rdwr_wunlock(&qc->rwlock);
				ldap_pvt_thread_mutex_lock(&cm->cache_mutex);
				cm->num_cached_queries++;
				Debug( pcache_debug, "STORED QUERIES = %lu\n",
						cm->num_cached_queries, 0, 0 );
				ldap_pvt_thread_mutex_unlock(&cm->cache_mutex);

				/* If the consistency checker suspended itself,
				 * wake it back up
				 */
				if ( cm->cc_paused == PCACHE_CC_PAUSED ) {
					ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
					if ( cm->cc_paused == PCACHE_CC_PAUSED ) {
						cm->cc_paused = 0;
						ldap_pvt_runqueue_resched( &slapd_rq, cm->cc_arg, 0 );
					}
					ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
				}

			} else if ( si->count ) {
				/* duplicate query, free it */
				Entry *e;
				for (;si->head; si->head=e) {
					e = si->head->e_private;
					si->head->e_private = NULL;
					entry_free(si->head);
				}
			}

		} else {
			filter_free( si->query.filter );
		}

		op->o_callback = op->o_callback->sc_next;
		op->o_tmpfree( cb, op->o_tmpmemctx );
	}

	return SLAP_CB_CONTINUE;
}

static int
pcache_response(
	Operation	*op,
	SlapReply	*rs )
{
	struct search_info *si = op->o_callback->sc_private;

	if ( si->swap_saved_attrs ) {
		rs->sr_attrs = si->save_attrs;
		rs->sr_attr_flags = slap_attr_flags( si->save_attrs );
		op->ors_attrs = si->save_attrs;
	}

	if ( rs->sr_type == REP_SEARCH ) {
		Entry *e;

		/* don't return more entries than requested by the client */
		if ( si->slimit > 0 && rs->sr_nentries >= si->slimit ) {
			si->slimit_exceeded = 1;
		}

		/* If we haven't exceeded the limit for this query,
		 * build a chain of answers to store. If we hit the
		 * limit, empty the chain and ignore the rest.
		 */
		if ( !si->over ) {
			slap_overinst *on = si->on;
			cache_manager *cm = on->on_bi.bi_private;

			/* check if the entry contains undefined
			 * attributes/objectClasses (ITS#5680) */
			if ( cm->check_cacheability && test_filter( op, rs->sr_entry, si->query.filter ) != LDAP_COMPARE_TRUE ) {
				Debug( pcache_debug, "%s: query not cacheable because of schema issues in DN \"%s\"\n",
					op->o_log_prefix, rs->sr_entry->e_name.bv_val, 0 );
				goto over;
			}

			/* check for malformed entries: attrs with no values */
			{
				Attribute *a = rs->sr_entry->e_attrs;
				for (; a; a=a->a_next) {
					if ( !a->a_numvals ) {
						Debug( pcache_debug, "%s: query not cacheable because of attrs without values in DN \"%s\" (%s)\n",
						op->o_log_prefix, rs->sr_entry->e_name.bv_val,
						a->a_desc->ad_cname.bv_val );
						goto over;
					}
				}
			}

			if ( si->count < si->max ) {
				si->count++;
				e = entry_dup( rs->sr_entry );
				if ( !si->head ) si->head = e;
				if ( si->tail ) si->tail->e_private = e;
				si->tail = e;

			} else {
over:;
				si->over = 1;
				si->count = 0;
				for (;si->head; si->head=e) {
					e = si->head->e_private;
					si->head->e_private = NULL;
					entry_free(si->head);
				}
				si->tail = NULL;
			}
		}
		if ( si->slimit_exceeded ) {
			return 0;
		}
	} else if ( rs->sr_type == REP_RESULT ) {

		if ( si->count ) {
			if ( rs->sr_err == LDAP_SUCCESS ) {
				si->caching_reason = PC_POSITIVE;

			} else if ( rs->sr_err == LDAP_SIZELIMIT_EXCEEDED
				&& si->qtemp->limitttl )
			{
				Entry *e;

				si->caching_reason = PC_SIZELIMIT;
				for (;si->head; si->head=e) {
					e = si->head->e_private;
					si->head->e_private = NULL;
					entry_free(si->head);
				}
			}

		} else if ( si->qtemp->negttl && !si->count && !si->over &&
				rs->sr_err == LDAP_SUCCESS )
		{
			si->caching_reason = PC_NEGATIVE;
		}


		if ( si->slimit_exceeded ) {
			rs->sr_err = LDAP_SIZELIMIT_EXCEEDED;
		}
	}

	return SLAP_CB_CONTINUE;
}

/* NOTE: this is a quick workaround to let pcache minimally interact
 * with pagedResults.  A more articulated solutions would be to
 * perform the remote query without control and cache all results,
 * performing the pagedResults search only within the client
 * and the proxy.  This requires pcache to understand pagedResults. */
static int
pcache_chk_controls(
	Operation	*op,
	SlapReply	*rs )
{
	const char	*non = "";
	const char	*stripped = "";

	switch( op->o_pagedresults ) {
	case SLAP_CONTROL_NONCRITICAL:
		non = "non-";
		stripped = "; stripped";
		/* fallthru */

	case SLAP_CONTROL_CRITICAL:
		Debug( pcache_debug, "%s: "
			"%scritical pagedResults control "
			"disabled with proxy cache%s.\n",
			op->o_log_prefix, non, stripped );
		
		slap_remove_control( op, rs, slap_cids.sc_pagedResults, NULL );
		break;

	default:
		rs->sr_err = SLAP_CB_CONTINUE;
		break;
	}

	return rs->sr_err;
}

static int
pc_setpw( Operation *op, struct berval *pwd, cache_manager *cm )
{
	struct berval vals[2];

	{
		const char *text = NULL;
		BER_BVZERO( &vals[0] );
		slap_passwd_hash( pwd, &vals[0], &text );
		if ( BER_BVISEMPTY( &vals[0] )) {
			Debug( pcache_debug, "pc_setpw: hash failed %s\n",
				text, 0, 0 );
			return LDAP_OTHER;
		}
	}

	BER_BVZERO( &vals[1] );

	{
		Modifications mod;
		SlapReply sr = { REP_RESULT };
		slap_callback cb = { 0, slap_null_cb, 0, 0 };
		int rc;

		mod.sml_op = LDAP_MOD_REPLACE;
		mod.sml_flags = 0;
		mod.sml_desc = slap_schema.si_ad_userPassword;
		mod.sml_type = mod.sml_desc->ad_cname;
		mod.sml_values = vals;
		mod.sml_nvalues = NULL;
		mod.sml_numvals = 1;
		mod.sml_next = NULL;

		op->o_tag = LDAP_REQ_MODIFY;
		op->orm_modlist = &mod;
		op->o_bd = &cm->db;
		op->o_dn = op->o_bd->be_rootdn;
		op->o_ndn = op->o_bd->be_rootndn;
		op->o_callback = &cb;
		Debug( pcache_debug, "pc_setpw: CACHING BIND for %s\n",
			op->o_req_dn.bv_val, 0, 0 );
		rc = op->o_bd->be_modify( op, &sr );
		ch_free( vals[0].bv_val );
		return rc;
	}
}

typedef struct bindcacheinfo {
	slap_overinst *on;
	CachedQuery *qc;
} bindcacheinfo;

static int
pc_bind_save( Operation *op, SlapReply *rs )
{
	if ( rs->sr_err == LDAP_SUCCESS ) {
		bindcacheinfo *bci = op->o_callback->sc_private;
		slap_overinst *on = bci->on;
		cache_manager *cm = on->on_bi.bi_private;
		CachedQuery *qc = bci->qc;
		int delete = 0;

		ldap_pvt_thread_rdwr_wlock( &qc->rwlock );
		if ( qc->bind_refcnt-- ) {
			Operation op2 = *op;
			if ( pc_setpw( &op2, &op->orb_cred, cm ) == LDAP_SUCCESS )
				bci->qc->bindref_time = op->o_time + bci->qc->qtemp->bindttr;
		} else {
			bci->qc = NULL;
			delete = 1;
		}
		ldap_pvt_thread_rdwr_wunlock( &qc->rwlock );
		if ( delete ) free_query(qc);
	}
	return SLAP_CB_CONTINUE;
}

static Filter *
pc_bind_attrs( Operation *op, Entry *e, QueryTemplate *temp,
	struct berval *fbv )
{
	int i, len = 0;
	struct berval *vals, pres = BER_BVC("*");
	char *p1, *p2;
	Attribute *a;

	vals = op->o_tmpalloc( temp->bindnattrs * sizeof( struct berval ),
		op->o_tmpmemctx );

	for ( i=0; i<temp->bindnattrs; i++ ) {
		a = attr_find( e->e_attrs, temp->bindfattrs[i] );
		if ( a && a->a_vals ) {
			vals[i] = a->a_vals[0];
			len += a->a_vals[0].bv_len;
		} else {
			vals[i] = pres;
		}
	}
	fbv->bv_len = len + temp->bindftemp.bv_len;
	fbv->bv_val = op->o_tmpalloc( fbv->bv_len + 1, op->o_tmpmemctx );

	p1 = temp->bindftemp.bv_val;
	p2 = fbv->bv_val;
	i = 0;
	while ( *p1 ) {
		*p2++ = *p1;
		if ( p1[0] == '=' && p1[1] == ')' ) {
			AC_MEMCPY( p2, vals[i].bv_val, vals[i].bv_len );
			p2 += vals[i].bv_len;
			i++;
		}
		p1++;
	}
	*p2 = '\0';
	op->o_tmpfree( vals, op->o_tmpmemctx );

	/* FIXME: are we sure str2filter_x can't fail?
	 * caller needs to check */
	{
		Filter *f = str2filter_x( op, fbv->bv_val );
		assert( f != NULL );
		return f;
	}
}

/* Check if the requested entry is from the cache and has a valid
 * ttr and password hash
 */
static int
pc_bind_search( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		bindinfo *pbi = op->o_callback->sc_private;

		/* We only care if this is an already cached result and we're
		 * below the refresh time, or we're offline.
		 */
		if ( pbi->bi_cq ) {
			if (( pbi->bi_cm->cc_paused & PCACHE_CC_OFFLINE ) ||
				op->o_time < pbi->bi_cq->bindref_time ) {
				Attribute *a;

				/* See if a recognized password is hashed here */
				a = attr_find( rs->sr_entry->e_attrs,
					slap_schema.si_ad_userPassword );
				if ( a && a->a_vals[0].bv_val[0] == '{' &&
					lutil_passwd_scheme( a->a_vals[0].bv_val ))
					pbi->bi_flags |= BI_HASHED;
			} else {
				Debug( pcache_debug, "pc_bind_search: cache is stale, "
					"reftime: %ld, current time: %ld\n",
					pbi->bi_cq->bindref_time, op->o_time, 0 );
			}
		} else if ( pbi->bi_si ) {
			/* This search result is going into the cache */
			struct berval fbv;
			Filter *f;

			filter_free( pbi->bi_si->query.filter );
			f = pc_bind_attrs( op, rs->sr_entry, pbi->bi_templ, &fbv );
			op->o_tmpfree( fbv.bv_val, op->o_tmpmemctx );
			pbi->bi_si->query.filter = filter_dup( f, NULL );
			filter_free_x( op, f, 1 );
		}
	}
	return 0;
}

/* We always want pc_bind_search to run after the search handlers */
static int
pc_bind_resp( Operation *op, SlapReply *rs )
{
	bindinfo *pbi = op->o_callback->sc_private;
	if ( !( pbi->bi_flags & BI_DIDCB )) {
		slap_callback *sc = op->o_callback;
		while ( sc && sc->sc_response != pcache_response )
			sc = sc->sc_next;
		if ( !sc )
			sc = op->o_callback;
		pbi->bi_cb.sc_next = sc->sc_next;
		sc->sc_next = &pbi->bi_cb;
		pbi->bi_flags |= BI_DIDCB;
	}
	return SLAP_CB_CONTINUE;
}

#ifdef PCACHE_CONTROL_PRIVDB
static int
pcache_op_privdb(
	Operation		*op,
	SlapReply		*rs )
{
	slap_overinst 	*on = (slap_overinst *)op->o_bd->bd_info;
	cache_manager 	*cm = on->on_bi.bi_private;
	slap_callback	*save_cb;
	slap_op_t	type;

	/* skip if control is unset */
	if ( op->o_ctrlflag[ privDB_cid ] != SLAP_CONTROL_CRITICAL ) {
		return SLAP_CB_CONTINUE;
	}

	/* The cache DB isn't open yet */
	if ( cm->defer_db_open ) {
		send_ldap_error( op, rs, LDAP_UNAVAILABLE,
			"pcachePrivDB: cacheDB not available" );
		return rs->sr_err;
	}

	/* FIXME: might be a little bit exaggerated... */
	if ( !be_isroot( op ) ) {
		save_cb = op->o_callback;
		op->o_callback = NULL;
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"pcachePrivDB: operation not allowed" );
		op->o_callback = save_cb;

		return rs->sr_err;
	}

	/* map tag to operation */
	type = slap_req2op( op->o_tag );
	if ( type != SLAP_OP_LAST ) {
		BackendInfo	*bi = cm->db.bd_info;
		int		rc;

		/* execute, if possible */
		if ( (&bi->bi_op_bind)[ type ] ) {
			Operation	op2 = *op;
	
			op2.o_bd = &cm->db;

			rc = (&bi->bi_op_bind)[ type ]( &op2, rs );
			if ( type == SLAP_OP_BIND && rc == LDAP_SUCCESS ) {
				op->o_conn->c_authz_cookie = cm->db.be_private;
			}

			return rs->sr_err;
		}
	}

	/* otherwise fall back to error */
	save_cb = op->o_callback;
	op->o_callback = NULL;
	send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
		"operation not supported with pcachePrivDB control" );
	op->o_callback = save_cb;

	return rs->sr_err;
}
#endif /* PCACHE_CONTROL_PRIVDB */

static int
pcache_op_bind(
	Operation		*op,
	SlapReply		*rs )
{
	slap_overinst 	*on = (slap_overinst *)op->o_bd->bd_info;
	cache_manager 	*cm = on->on_bi.bi_private;
	QueryTemplate *temp;
	Entry *e;
	slap_callback	cb = { 0 }, *sc;
	bindinfo bi;
	bindcacheinfo *bci;
	Operation op2;
	int rc;

#ifdef PCACHE_CONTROL_PRIVDB
	if ( op->o_ctrlflag[ privDB_cid ] == SLAP_CONTROL_CRITICAL )
		return pcache_op_privdb( op, rs );
#endif /* PCACHE_CONTROL_PRIVDB */

	/* Skip if we're not configured for Binds, or cache DB isn't open yet */
	if ( !cm->cache_binds || cm->defer_db_open )
		return SLAP_CB_CONTINUE;

	/* First find a matching template with Bind info */
	for ( temp=cm->qm->templates; temp; temp=temp->qmnext ) {
		if ( temp->bindttr && dnIsSuffix( &op->o_req_ndn, &temp->bindbase ))
			break;
	}
	/* Didn't find a suitable template, just passthru */
	if ( !temp )
		return SLAP_CB_CONTINUE;

	/* See if the entry is already locally cached. If so, we can
	 * populate the query filter to retrieve the cached query. We
	 * need to check the bindrefresh time in the query.
	 */
	op2 = *op;
	op2.o_dn = op->o_bd->be_rootdn;
	op2.o_ndn = op->o_bd->be_rootndn;
	bi.bi_flags = 0;

	op2.o_bd = &cm->db;
	e = NULL;
	rc = be_entry_get_rw( &op2, &op->o_req_ndn, NULL, NULL, 0, &e );
	if ( rc == LDAP_SUCCESS && e ) {
		bi.bi_flags |= BI_LOOKUP;
		op2.ors_filter = pc_bind_attrs( op, e, temp, &op2.ors_filterstr );
		be_entry_release_r( &op2, e );
	} else {
		op2.ors_filter = temp->bindfilter;
		op2.ors_filterstr = temp->bindfilterstr;
	}

	op2.o_bd = op->o_bd;
	op2.o_tag = LDAP_REQ_SEARCH;
	op2.ors_scope = LDAP_SCOPE_BASE;
	op2.ors_deref = LDAP_DEREF_NEVER;
	op2.ors_slimit = 1;
	op2.ors_tlimit = SLAP_NO_LIMIT;
	op2.ors_limit = NULL;
	op2.ors_attrs = cm->qm->attr_sets[temp->attr_set_index].attrs;
	op2.ors_attrsonly = 0;

	/* We want to invoke search at the same level of the stack
	 * as we're already at...
	 */
	bi.bi_cm = cm;
	bi.bi_templ = temp;
	bi.bi_cq = NULL;
	bi.bi_si = NULL;

	bi.bi_cb.sc_response = pc_bind_search;
	bi.bi_cb.sc_cleanup = NULL;
	bi.bi_cb.sc_private = &bi;
	cb.sc_private = &bi;
	cb.sc_response = pc_bind_resp;
	op2.o_callback = &cb;
	overlay_op_walk( &op2, rs, op_search, on->on_info, on );

	/* OK, just bind locally */
	if ( bi.bi_flags & BI_HASHED ) {
		int delete = 0;
		BackendDB *be = op->o_bd;
		op->o_bd = &cm->db;

		Debug( pcache_debug, "pcache_op_bind: CACHED BIND for %s\n",
			op->o_req_dn.bv_val, 0, 0 );

		if ( op->o_bd->be_bind( op, rs ) == LDAP_SUCCESS ) {
			op->o_conn->c_authz_cookie = cm->db.be_private;
		}
		op->o_bd = be;
		ldap_pvt_thread_rdwr_wlock( &bi.bi_cq->rwlock );
		if ( !bi.bi_cq->bind_refcnt-- ) {
			delete = 1;
		}
		ldap_pvt_thread_rdwr_wunlock( &bi.bi_cq->rwlock );
		if ( delete ) free_query( bi.bi_cq );
		return rs->sr_err;
	}

	/* We have a cached query to work with */
	if ( bi.bi_cq ) {
		sc = op->o_tmpalloc( sizeof(slap_callback) + sizeof(bindcacheinfo),
			op->o_tmpmemctx );
		sc->sc_response = pc_bind_save;
		sc->sc_cleanup = NULL;
		sc->sc_private = sc+1;
		bci = sc->sc_private;
		sc->sc_next = op->o_callback;
		op->o_callback = sc;
		bci->on = on;
		bci->qc = bi.bi_cq;
	}
	return SLAP_CB_CONTINUE;
}

static slap_response refresh_merge;

static int
pcache_op_search(
	Operation	*op,
	SlapReply	*rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	cache_manager *cm = on->on_bi.bi_private;
	query_manager*		qm = cm->qm;

	int i = -1;

	Query		query;
	QueryTemplate	*qtemp = NULL;
	bindinfo *pbi = NULL;

	int 		attr_set = -1;
	CachedQuery 	*answerable = NULL;
	int 		cacheable = 0;

	struct berval	tempstr;

#ifdef PCACHE_CONTROL_PRIVDB
	if ( op->o_ctrlflag[ privDB_cid ] == SLAP_CONTROL_CRITICAL ) {
		return pcache_op_privdb( op, rs );
	}
#endif /* PCACHE_CONTROL_PRIVDB */

	/* The cache DB isn't open yet */
	if ( cm->defer_db_open ) {
		send_ldap_error( op, rs, LDAP_UNAVAILABLE,
			"pcachePrivDB: cacheDB not available" );
		return rs->sr_err;
	}

	/* pickup runtime ACL changes */
	cm->db.be_acl = op->o_bd->be_acl;

	{
		/* See if we're processing a Bind request
		 * or a cache refresh */
		slap_callback *cb = op->o_callback;

		for ( ; cb; cb=cb->sc_next ) {
			if ( cb->sc_response == pc_bind_resp ) {
				pbi = cb->sc_private;
				break;
			}
			if ( cb->sc_response == refresh_merge ) {
				/* This is a refresh, do not search the cache */
				return SLAP_CB_CONTINUE;
			}
		}
	}

	/* FIXME: cannot cache/answer requests with pagedResults control */

	query.filter = op->ors_filter;

	if ( pbi ) {
		query.base = pbi->bi_templ->bindbase;
		query.scope = pbi->bi_templ->bindscope;
		attr_set = pbi->bi_templ->attr_set_index;
		cacheable = 1;
		qtemp = pbi->bi_templ;
		if ( pbi->bi_flags & BI_LOOKUP )
			answerable = qm->qcfunc(op, qm, &query, qtemp);

	} else {
		tempstr.bv_val = op->o_tmpalloc( op->ors_filterstr.bv_len+1,
			op->o_tmpmemctx );
		tempstr.bv_len = 0;
		if ( filter2template( op, op->ors_filter, &tempstr ))
		{
			op->o_tmpfree( tempstr.bv_val, op->o_tmpmemctx );
			return SLAP_CB_CONTINUE;
		}

		Debug( pcache_debug, "query template of incoming query = %s\n",
						tempstr.bv_val, 0, 0 );

		/* find attr set */
		attr_set = get_attr_set(op->ors_attrs, qm, cm->numattrsets);

		query.base = op->o_req_ndn;
		query.scope = op->ors_scope;

		/* check for query containment */
		if (attr_set > -1) {
			QueryTemplate *qt = qm->attr_sets[attr_set].templates;
			for (; qt; qt = qt->qtnext ) {
				/* find if template i can potentially answer tempstr */
				if ( ber_bvstrcasecmp( &qt->querystr, &tempstr ) != 0 )
					continue;
				cacheable = 1;
				qtemp = qt;
				Debug( pcache_debug, "Entering QC, querystr = %s\n",
						op->ors_filterstr.bv_val, 0, 0 );
				answerable = qm->qcfunc(op, qm, &query, qt);

				/* if != NULL, rlocks qtemp->t_rwlock */
				if (answerable)
					break;
			}
		}
		op->o_tmpfree( tempstr.bv_val, op->o_tmpmemctx );
	}

	if (answerable) {
		BackendDB	*save_bd = op->o_bd;

		ldap_pvt_thread_mutex_lock( &answerable->answerable_cnt_mutex );
		answerable->answerable_cnt++;
		/* we only care about refcnts if we're refreshing */
		if ( answerable->refresh_time )
			answerable->refcnt++;
		Debug( pcache_debug, "QUERY ANSWERABLE (answered %lu times)\n",
			answerable->answerable_cnt, 0, 0 );
		ldap_pvt_thread_mutex_unlock( &answerable->answerable_cnt_mutex );

		ldap_pvt_thread_rdwr_wlock(&answerable->rwlock);
		if ( BER_BVISNULL( &answerable->q_uuid )) {
			/* No entries cached, just an empty result set */
			i = rs->sr_err = 0;
			send_ldap_result( op, rs );
		} else {
			/* Let Bind know we used a cached query */
			if ( pbi ) {
				answerable->bind_refcnt++;
				pbi->bi_cq = answerable;
			}

			op->o_bd = &cm->db;
			if ( cm->response_cb == PCACHE_RESPONSE_CB_TAIL ) {
				slap_callback cb;
				/* The cached entry was already processed by any
				 * other overlays, so don't let it get processed again.
				 *
				 * This loop removes over_back_response from the stack.
				 */
				if ( overlay_callback_after_backover( op, &cb, 0) == 0 ) {
					slap_callback **scp;
					for ( scp = &op->o_callback; *scp != NULL;
						scp = &(*scp)->sc_next ) {
						if ( (*scp)->sc_next == &cb ) {
							*scp = cb.sc_next;
							break;
						}
					}
				}
			}
			i = cm->db.bd_info->bi_op_search( op, rs );
		}
		ldap_pvt_thread_rdwr_wunlock(&answerable->rwlock);
		/* locked by qtemp->qcfunc (query_containment) */
		ldap_pvt_thread_rdwr_runlock(&qtemp->t_rwlock);
		op->o_bd = save_bd;
		return i;
	}

	Debug( pcache_debug, "QUERY NOT ANSWERABLE\n", 0, 0, 0 );

	ldap_pvt_thread_mutex_lock(&cm->cache_mutex);
	if (cm->num_cached_queries >= cm->max_queries) {
		cacheable = 0;
	}
	ldap_pvt_thread_mutex_unlock(&cm->cache_mutex);

	if (op->ors_attrsonly)
		cacheable = 0;

	if (cacheable) {
		slap_callback		*cb;
		struct search_info	*si;

		Debug( pcache_debug, "QUERY CACHEABLE\n", 0, 0, 0 );
		query.filter = filter_dup(op->ors_filter, NULL);

		cb = op->o_tmpalloc( sizeof(*cb) + sizeof(*si), op->o_tmpmemctx );
		cb->sc_response = pcache_response;
		cb->sc_cleanup = pcache_op_cleanup;
		cb->sc_private = (cb+1);
		si = cb->sc_private;
		si->on = on;
		si->query = query;
		si->qtemp = qtemp;
		si->max = cm->num_entries_limit ;
		si->over = 0;
		si->count = 0;
		si->slimit = 0;
		si->slimit_exceeded = 0;
		si->caching_reason = PC_IGNORE;
		if ( op->ors_slimit > 0 && op->ors_slimit < cm->num_entries_limit ) {
			si->slimit = op->ors_slimit;
			op->ors_slimit = cm->num_entries_limit;
		}
		si->head = NULL;
		si->tail = NULL;
		si->swap_saved_attrs = 1;
		si->save_attrs = op->ors_attrs;
		si->pbi = pbi;
		if ( pbi )
			pbi->bi_si = si;

		op->ors_attrs = qtemp->t_attrs.attrs;

		if ( cm->response_cb == PCACHE_RESPONSE_CB_HEAD ) {
			cb->sc_next = op->o_callback;
			op->o_callback = cb;

		} else {
			slap_callback		**pcb;

			/* need to move the callback at the end, in case other
			 * overlays are present, so that the final entry is
			 * actually cached */
			cb->sc_next = NULL;
			for ( pcb = &op->o_callback; *pcb; pcb = &(*pcb)->sc_next );
			*pcb = cb;
		}

	} else {
		Debug( pcache_debug, "QUERY NOT CACHEABLE\n",
					0, 0, 0);
	}

	return SLAP_CB_CONTINUE;
}

static int
get_attr_set(
	AttributeName* attrs,
	query_manager* qm,
	int num )
{
	int i = 0;
	int count = 0;

	if ( attrs ) {
		for ( ; attrs[i].an_name.bv_val; i++ ) {
			/* only count valid attribute names
			 * (searches ignore others, this overlay does the same) */
			if ( attrs[i].an_desc ) {
				count++;
			}
		}
	}

	/* recognize default or explicit single "*" */
	if ( ! attrs ||
		( i == 1 && bvmatch( &attrs[0].an_name, slap_bv_all_user_attrs ) ) )
	{
		count = 1;
		attrs = slap_anlist_all_user_attributes;

	/* recognize implicit (no valid attributes) or explicit single "1.1" */
	} else if ( count == 0 ||
		( i == 1 && bvmatch( &attrs[0].an_name, slap_bv_no_attrs ) ) )
	{
		count = 0;
		attrs = NULL;
	}

	for ( i = 0; i < num; i++ ) {
		AttributeName *a2;
		int found = 1;

		if ( count > qm->attr_sets[i].count ) {
			if ( qm->attr_sets[i].count &&
				bvmatch( &qm->attr_sets[i].attrs[0].an_name, slap_bv_all_user_attrs )) {
				break;
			}
			continue;
		}

		if ( !count ) {
			if ( !qm->attr_sets[i].count ) {
				break;
			}
			continue;
		}

		for ( a2 = attrs; a2->an_name.bv_val; a2++ ) {
			if ( !a2->an_desc && !bvmatch( &a2->an_name, slap_bv_all_user_attrs ) ) continue;

			if ( !an_find( qm->attr_sets[i].attrs, &a2->an_name ) ) {
				found = 0;
				break;
			}
		}

		if ( found ) {
			break;
		}
	}

	if ( i == num ) {
		i = -1;
	}

	return i;
}

/* Refresh a cached query:
 * 1: Replay the query on the remote DB and merge each entry into
 * the local DB. Remember the DNs of each remote entry.
 * 2: Search the local DB for all entries matching this queryID.
 * Delete any entry whose DN is not in the list from (1).
 */
typedef struct dnlist {
	struct dnlist *next;
	struct berval dn;
	char delete;
} dnlist;

typedef struct refresh_info {
	dnlist *ri_dns;
	dnlist *ri_tail;
	dnlist *ri_dels;
	BackendDB *ri_be;
	CachedQuery *ri_q;
} refresh_info;

static dnlist *dnl_alloc( Operation *op, struct berval *bvdn )
{
	dnlist *dn = op->o_tmpalloc( sizeof(dnlist) + bvdn->bv_len + 1,
			op->o_tmpmemctx );
	dn->dn.bv_len = bvdn->bv_len;
	dn->dn.bv_val = (char *)(dn+1);
	AC_MEMCPY( dn->dn.bv_val, bvdn->bv_val, dn->dn.bv_len );
	dn->dn.bv_val[dn->dn.bv_len] = '\0';
	return dn;
}

static int
refresh_merge( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		refresh_info *ri = op->o_callback->sc_private;
		Entry *e;
		dnlist *dnl;
		slap_callback *ocb;
		int rc;

		ocb = op->o_callback;
		/* Find local entry, merge */
		op->o_bd = ri->ri_be;
		rc = be_entry_get_rw( op, &rs->sr_entry->e_nname, NULL, NULL, 0, &e );
		if ( rc != LDAP_SUCCESS || e == NULL ) {
			/* No local entry, just add it. FIXME: we are not checking
			 * the cache entry limit here
			 */
			 merge_entry( op, rs->sr_entry, 1, &ri->ri_q->q_uuid );
		} else {
			/* Entry exists, update it */
			Entry ne;
			Attribute *a, **b;
			Modifications *modlist, *mods = NULL;
			const char* 	text = NULL;
			char			textbuf[SLAP_TEXT_BUFLEN];
			size_t			textlen = sizeof(textbuf);
			slap_callback cb = { NULL, slap_null_cb, NULL, NULL };

			ne = *e;
			b = &ne.e_attrs;
			/* Get a copy of only the attrs we requested */
			for ( a=e->e_attrs; a; a=a->a_next ) {
				if ( ad_inlist( a->a_desc, rs->sr_attrs )) {
					*b = attr_alloc( a->a_desc );
					*(*b) = *a;
					/* The actual values still belong to e */
					(*b)->a_flags |= SLAP_ATTR_DONT_FREE_VALS |
						SLAP_ATTR_DONT_FREE_DATA;
					b = &((*b)->a_next);
				}
			}
			*b = NULL;
			slap_entry2mods( rs->sr_entry, &modlist, &text, textbuf, textlen );
			syncrepl_diff_entry( op, ne.e_attrs, rs->sr_entry->e_attrs,
				&mods, &modlist, 0 );
			be_entry_release_r( op, e );
			attrs_free( ne.e_attrs );
			slap_mods_free( modlist, 1 );
			/* mods is NULL if there are no changes */
			if ( mods ) {
				SlapReply rs2 = { REP_RESULT };
				struct berval dn = op->o_req_dn;
				struct berval ndn = op->o_req_ndn;
				op->o_tag = LDAP_REQ_MODIFY;
				op->orm_modlist = mods;
				op->o_req_dn = rs->sr_entry->e_name;
				op->o_req_ndn = rs->sr_entry->e_nname;
				op->o_callback = &cb;
				op->o_bd->be_modify( op, &rs2 );
				rs->sr_err = rs2.sr_err;
				rs_assert_done( &rs2 );
				slap_mods_free( mods, 1 );
				op->o_req_dn = dn;
				op->o_req_ndn = ndn;
			}
		}

		/* Add DN to list */
		dnl = dnl_alloc( op, &rs->sr_entry->e_nname );
		dnl->next = NULL;
		if ( ri->ri_tail ) {
			ri->ri_tail->next = dnl;
		} else {
			ri->ri_dns = dnl;
		}
		ri->ri_tail = dnl;
		op->o_callback = ocb;
	}
	return 0;
}

static int
refresh_purge( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		refresh_info *ri = op->o_callback->sc_private;
		dnlist **dn;
		int del = 1;

		/* Did the entry exist on the remote? */
		for ( dn=&ri->ri_dns; *dn; dn = &(*dn)->next ) {
			if ( dn_match( &(*dn)->dn, &rs->sr_entry->e_nname )) {
				dnlist *dnext = (*dn)->next;
				op->o_tmpfree( *dn, op->o_tmpmemctx );
				*dn = dnext;
				del = 0;
				break;
			}
		}
		/* No, so put it on the list to delete */
		if ( del ) {
			Attribute *a;
			dnlist *dnl = dnl_alloc( op, &rs->sr_entry->e_nname );
			dnl->next = ri->ri_dels;
			ri->ri_dels = dnl;
			a = attr_find( rs->sr_entry->e_attrs, ad_queryId );
			/* If ours is the only queryId, delete entry */
			dnl->delete = ( a->a_numvals == 1 );
		}
	}
	return 0;
}

static int
refresh_query( Operation *op, CachedQuery *query, slap_overinst *on )
{
	SlapReply rs = {REP_RESULT};
	slap_callback cb = { 0 };
	refresh_info ri = { 0 };
	char filter_str[ LDAP_LUTIL_UUIDSTR_BUFSIZE + STRLENOF( "(pcacheQueryID=)" ) ];
	AttributeAssertion	ava = ATTRIBUTEASSERTION_INIT;
	Filter filter = {LDAP_FILTER_EQUALITY};
	AttributeName attrs[ 2 ] = {{{ 0 }}};
	dnlist *dn;
	int rc;

	ldap_pvt_thread_mutex_lock( &query->answerable_cnt_mutex );
	query->refcnt = 0;
	ldap_pvt_thread_mutex_unlock( &query->answerable_cnt_mutex );

	cb.sc_response = refresh_merge;
	cb.sc_private = &ri;

	/* cache DB */
	ri.ri_be = op->o_bd;
	ri.ri_q = query;

	op->o_tag = LDAP_REQ_SEARCH;
	op->o_protocol = LDAP_VERSION3;
	op->o_callback = &cb;
	op->o_do_not_cache = 1;

	op->o_req_dn = query->qbase->base;
	op->o_req_ndn = query->qbase->base;
	op->ors_scope = query->scope;
	op->ors_deref = LDAP_DEREF_NEVER;
	op->ors_slimit = SLAP_NO_LIMIT;
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_limit = NULL;
	op->ors_filter = query->filter;
	filter2bv_x( op, query->filter, &op->ors_filterstr );
	op->ors_attrs = query->qtemp->t_attrs.attrs;
	op->ors_attrsonly = 0;

	op->o_bd = on->on_info->oi_origdb;
	rc = op->o_bd->be_search( op, &rs );
	if ( rc ) {
		op->o_bd = ri.ri_be;
		goto leave;
	}

	/* Get the DNs of all entries matching this query */
	cb.sc_response = refresh_purge;

	op->o_bd = ri.ri_be;
	op->o_req_dn = op->o_bd->be_suffix[0];
	op->o_req_ndn = op->o_bd->be_nsuffix[0];
	op->ors_scope = LDAP_SCOPE_SUBTREE;
	op->ors_deref = LDAP_DEREF_NEVER;
	op->ors_filterstr.bv_len = snprintf(filter_str, sizeof(filter_str),
		"(%s=%s)", ad_queryId->ad_cname.bv_val, query->q_uuid.bv_val);
	filter.f_ava = &ava;
	filter.f_av_desc = ad_queryId;
	filter.f_av_value = query->q_uuid;
	attrs[ 0 ].an_desc = ad_queryId;
	attrs[ 0 ].an_name = ad_queryId->ad_cname;
	op->ors_attrs = attrs;
	op->ors_attrsonly = 0;
	rs_reinit( &rs, REP_RESULT );
	rc = op->o_bd->be_search( op, &rs );
	if ( rc ) goto leave;

	while (( dn = ri.ri_dels )) {
		op->o_req_dn = dn->dn;
		op->o_req_ndn = dn->dn;
		rs_reinit( &rs, REP_RESULT );
		if ( dn->delete ) {
			op->o_tag = LDAP_REQ_DELETE;
			op->o_bd->be_delete( op, &rs );
		} else {
			Modifications mod;
			struct berval vals[2];

			vals[0] = query->q_uuid;
			BER_BVZERO( &vals[1] );
			mod.sml_op = LDAP_MOD_DELETE;
			mod.sml_flags = 0;
			mod.sml_desc = ad_queryId;
			mod.sml_type = ad_queryId->ad_cname;
			mod.sml_values = vals;
			mod.sml_nvalues = NULL;
			mod.sml_numvals = 1;
			mod.sml_next = NULL;

			op->o_tag = LDAP_REQ_MODIFY;
			op->orm_modlist = &mod;
			op->o_bd->be_modify( op, &rs );
		}
		ri.ri_dels = dn->next;
		op->o_tmpfree( dn, op->o_tmpmemctx );
	}

leave:
	/* reset our local heap, we're done with it */
	slap_sl_mem_create(SLAP_SLAB_SIZE, SLAP_SLAB_STACK, op->o_threadctx, 1 );
	return rc;
}

static void*
consistency_check(
	void *ctx,
	void *arg )
{
	struct re_s *rtask = arg;
	slap_overinst *on = rtask->arg;
	cache_manager *cm = on->on_bi.bi_private;
	query_manager *qm = cm->qm;
	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;

	CachedQuery *query, *qprev;
	int return_val, pause = PCACHE_CC_PAUSED;
	QueryTemplate *templ;

	/* Don't expire anything when we're offline */
	if ( cm->cc_paused & PCACHE_CC_OFFLINE ) {
		pause = PCACHE_CC_OFFLINE;
		goto leave;
	}

	connection_fake_init( &conn, &opbuf, ctx );
	op = &opbuf.ob_op;

	op->o_bd = &cm->db;
	op->o_dn = cm->db.be_rootdn;
	op->o_ndn = cm->db.be_rootndn;

	cm->cc_arg = arg;

	for (templ = qm->templates; templ; templ=templ->qmnext) {
		time_t ttl;
		if ( !templ->query_last ) continue;
		pause = 0;
		op->o_time = slap_get_time();
		if ( !templ->ttr ) {
			ttl = templ->ttl;
			if ( templ->negttl && templ->negttl < ttl )
				ttl = templ->negttl;
			if ( templ->limitttl && templ->limitttl < ttl )
				ttl = templ->limitttl;
			/* The oldest timestamp that needs expiration checking */
			ttl += op->o_time;
		}

		for ( query=templ->query_last; query; query=qprev ) {
			qprev = query->prev;
			if ( query->refresh_time && query->refresh_time < op->o_time ) {
				/* A refresh will extend the expiry if the query has been
				 * referenced, but not if it's unreferenced. If the
				 * expiration has been hit, then skip the refresh since
				 * we're just going to discard the result anyway.
				 */
				if ( query->refcnt )
					query->expiry_time = op->o_time + templ->ttl;
				if ( query->expiry_time > op->o_time ) {
					refresh_query( op, query, on );
					continue;
				}
			}

			if (query->expiry_time < op->o_time) {
				int rem = 0;
				Debug( pcache_debug, "Lock CR index = %p\n",
						(void *) templ, 0, 0 );
				ldap_pvt_thread_rdwr_wlock(&templ->t_rwlock);
				if ( query == templ->query_last ) {
					rem = 1;
					remove_from_template(query, templ);
					Debug( pcache_debug, "TEMPLATE %p QUERIES-- %d\n",
							(void *) templ, templ->no_of_queries, 0 );
					Debug( pcache_debug, "Unlock CR index = %p\n",
							(void *) templ, 0, 0 );
				}
				if ( !rem ) {
					ldap_pvt_thread_rdwr_wunlock(&templ->t_rwlock);
					continue;
				}
				ldap_pvt_thread_mutex_lock(&qm->lru_mutex);
				remove_query(qm, query);
				ldap_pvt_thread_mutex_unlock(&qm->lru_mutex);
				if ( BER_BVISNULL( &query->q_uuid ))
					return_val = 0;
				else
					return_val = remove_query_data(op, &query->q_uuid);
				Debug( pcache_debug, "STALE QUERY REMOVED, SIZE=%d\n",
							return_val, 0, 0 );
				ldap_pvt_thread_mutex_lock(&cm->cache_mutex);
				cm->cur_entries -= return_val;
				cm->num_cached_queries--;
				Debug( pcache_debug, "STORED QUERIES = %lu\n",
						cm->num_cached_queries, 0, 0 );
				ldap_pvt_thread_mutex_unlock(&cm->cache_mutex);
				Debug( pcache_debug,
					"STALE QUERY REMOVED, CACHE ="
					"%d entries\n",
					cm->cur_entries, 0, 0 );
				ldap_pvt_thread_rdwr_wlock( &query->rwlock );
				if ( query->bind_refcnt-- ) {
					rem = 0;
				} else {
					rem = 1;
				}
				ldap_pvt_thread_rdwr_wunlock( &query->rwlock );
				if ( rem ) free_query(query);
				ldap_pvt_thread_rdwr_wunlock(&templ->t_rwlock);
			} else if ( !templ->ttr && query->expiry_time > ttl ) {
				/* We don't need to check for refreshes, and this
				 * query's expiry is too new, and all subsequent queries
				 * will be newer yet. So stop looking.
				 *
				 * If we have refreshes, then we always have to walk the
				 * entire query list.
				 */
				break;
			}
		}
	}

leave:
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	if ( ldap_pvt_runqueue_isrunning( &slapd_rq, rtask )) {
		ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	}
	/* If there were no queries, defer processing for a while */
	if ( cm->cc_paused != pause )
		cm->cc_paused = pause;
	ldap_pvt_runqueue_resched( &slapd_rq, rtask, pause );

	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	return NULL;
}


#define MAX_ATTR_SETS 500

enum {
	PC_MAIN = 1,
	PC_ATTR,
	PC_TEMP,
	PC_RESP,
	PC_QUERIES,
	PC_OFFLINE,
	PC_BIND,
	PC_PRIVATE_DB
};

static ConfigDriver pc_cf_gen;
static ConfigLDAPadd pc_ldadd;
static ConfigCfAdd pc_cfadd;

static ConfigTable pccfg[] = {
	{ "pcache", "backend> <max_entries> <numattrsets> <entry limit> "
				"<cycle_time",
		6, 6, 0, ARG_MAGIC|ARG_NO_DELETE|PC_MAIN, pc_cf_gen,
		"( OLcfgOvAt:2.1 NAME ( 'olcPcache' 'olcProxyCache' ) "
			"DESC 'Proxy Cache basic parameters' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "pcacheAttrset", "index> <attributes...",
		2, 0, 0, ARG_MAGIC|PC_ATTR, pc_cf_gen,
		"( OLcfgOvAt:2.2 NAME ( 'olcPcacheAttrset' 'olcProxyAttrset' ) "
			"DESC 'A set of attributes to cache' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "pcacheTemplate", "filter> <attrset-index> <TTL> <negTTL> "
			"<limitTTL> <TTR",
		4, 7, 0, ARG_MAGIC|PC_TEMP, pc_cf_gen,
		"( OLcfgOvAt:2.3 NAME ( 'olcPcacheTemplate' 'olcProxyCacheTemplate' ) "
			"DESC 'Filter template, attrset, cache TTL, "
				"optional negative TTL, optional sizelimit TTL, "
				"optional TTR' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "pcachePosition", "head|tail(default)",
		2, 2, 0, ARG_MAGIC|PC_RESP, pc_cf_gen,
		"( OLcfgOvAt:2.4 NAME 'olcPcachePosition' "
			"DESC 'Response callback position in overlay stack' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "pcacheMaxQueries", "queries",
		2, 2, 0, ARG_INT|ARG_MAGIC|PC_QUERIES, pc_cf_gen,
		"( OLcfgOvAt:2.5 NAME ( 'olcPcacheMaxQueries' 'olcProxyCacheQueries' ) "
			"DESC 'Maximum number of queries to cache' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "pcachePersist", "TRUE|FALSE",
		2, 2, 0, ARG_ON_OFF|ARG_OFFSET, (void *)offsetof(cache_manager, save_queries),
		"( OLcfgOvAt:2.6 NAME ( 'olcPcachePersist' 'olcProxySaveQueries' ) "
			"DESC 'Save cached queries for hot restart' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "pcacheValidate", "TRUE|FALSE",
		2, 2, 0, ARG_ON_OFF|ARG_OFFSET, (void *)offsetof(cache_manager, check_cacheability),
		"( OLcfgOvAt:2.7 NAME ( 'olcPcacheValidate' 'olcProxyCheckCacheability' ) "
			"DESC 'Check whether the results of a query are cacheable, e.g. for schema issues' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "pcacheOffline", "TRUE|FALSE",
		2, 2, 0, ARG_ON_OFF|ARG_MAGIC|PC_OFFLINE, pc_cf_gen,
		"( OLcfgOvAt:2.8 NAME 'olcPcacheOffline' "
			"DESC 'Set cache to offline mode and disable expiration' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "pcacheBind", "filter> <attrset-index> <TTR> <scope> <base",
		6, 6, 0, ARG_MAGIC|PC_BIND, pc_cf_gen,
		"( OLcfgOvAt:2.9 NAME 'olcPcacheBind' "
			"DESC 'Parameters for caching Binds' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "pcache-", "private database args",
		1, 0, STRLENOF("pcache-"), ARG_MAGIC|PC_PRIVATE_DB, pc_cf_gen,
		NULL, NULL, NULL },

	/* Legacy keywords */
	{ "proxycache", "backend> <max_entries> <numattrsets> <entry limit> "
				"<cycle_time",
		6, 6, 0, ARG_MAGIC|ARG_NO_DELETE|PC_MAIN, pc_cf_gen,
		NULL, NULL, NULL },
	{ "proxyattrset", "index> <attributes...",
		2, 0, 0, ARG_MAGIC|PC_ATTR, pc_cf_gen,
		NULL, NULL, NULL },
	{ "proxytemplate", "filter> <attrset-index> <TTL> <negTTL",
		4, 7, 0, ARG_MAGIC|PC_TEMP, pc_cf_gen,
		NULL, NULL, NULL },
	{ "response-callback", "head|tail(default)",
		2, 2, 0, ARG_MAGIC|PC_RESP, pc_cf_gen,
		NULL, NULL, NULL },
	{ "proxyCacheQueries", "queries",
		2, 2, 0, ARG_INT|ARG_MAGIC|PC_QUERIES, pc_cf_gen,
		NULL, NULL, NULL },
	{ "proxySaveQueries", "TRUE|FALSE",
		2, 2, 0, ARG_ON_OFF|ARG_OFFSET, (void *)offsetof(cache_manager, save_queries),
		NULL, NULL, NULL },
	{ "proxyCheckCacheability", "TRUE|FALSE",
		2, 2, 0, ARG_ON_OFF|ARG_OFFSET, (void *)offsetof(cache_manager, check_cacheability),
		NULL, NULL, NULL },

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs pcocs[] = {
	{ "( OLcfgOvOc:2.1 "
		"NAME 'olcPcacheConfig' "
		"DESC 'ProxyCache configuration' "
		"SUP olcOverlayConfig "
		"MUST ( olcPcache $ olcPcacheAttrset $ olcPcacheTemplate ) "
		"MAY ( olcPcachePosition $ olcPcacheMaxQueries $ olcPcachePersist $ "
			"olcPcacheValidate $ olcPcacheOffline $ olcPcacheBind ) )",
		Cft_Overlay, pccfg, NULL, pc_cfadd },
	{ "( OLcfgOvOc:2.2 "
		"NAME 'olcPcacheDatabase' "
		"DESC 'Cache database configuration' "
		"AUXILIARY )", Cft_Misc, olcDatabaseDummy, pc_ldadd },
	{ NULL, 0, NULL }
};

static int pcache_db_open2( slap_overinst *on, ConfigReply *cr );

static int
pc_ldadd_cleanup( ConfigArgs *c )
{
	slap_overinst *on = c->ca_private;
	return pcache_db_open2( on, &c->reply );
}

static int
pc_ldadd( CfEntryInfo *p, Entry *e, ConfigArgs *ca )
{
	slap_overinst *on;
	cache_manager *cm;

	if ( p->ce_type != Cft_Overlay || !p->ce_bi ||
		p->ce_bi->bi_cf_ocs != pcocs )
		return LDAP_CONSTRAINT_VIOLATION;

	on = (slap_overinst *)p->ce_bi;
	cm = on->on_bi.bi_private;
	ca->be = &cm->db;
	/* Defer open if this is an LDAPadd */
	if ( CONFIG_ONLINE_ADD( ca ))
		ca->cleanup = pc_ldadd_cleanup;
	else
		cm->defer_db_open = 0;
	ca->ca_private = on;
	return LDAP_SUCCESS;
}

static int
pc_cfadd( Operation *op, SlapReply *rs, Entry *p, ConfigArgs *ca )
{
	CfEntryInfo *pe = p->e_private;
	slap_overinst *on = (slap_overinst *)pe->ce_bi;
	cache_manager *cm = on->on_bi.bi_private;
	struct berval bv;

	/* FIXME: should not hardcode "olcDatabase" here */
	bv.bv_len = snprintf( ca->cr_msg, sizeof( ca->cr_msg ),
		"olcDatabase=" SLAP_X_ORDERED_FMT "%s",
		0, cm->db.bd_info->bi_type );
	if ( bv.bv_len >= sizeof( ca->cr_msg ) ) {
		return -1;
	}
	bv.bv_val = ca->cr_msg;
	ca->be = &cm->db;
	cm->defer_db_open = 0;

	/* We can only create this entry if the database is table-driven
	 */
	if ( cm->db.bd_info->bi_cf_ocs )
		config_build_entry( op, rs, pe, ca, &bv, cm->db.bd_info->bi_cf_ocs,
			&pcocs[1] );

	return 0;
}

static int
pc_cf_gen( ConfigArgs *c )
{
	slap_overinst	*on = (slap_overinst *)c->bi;
	cache_manager* 	cm = on->on_bi.bi_private;
	query_manager*  qm = cm->qm;
	QueryTemplate* 	temp;
	AttributeName*  attr_name;
	AttributeName* 	attrarray;
	const char* 	text=NULL;
	int		i, num, rc = 0;
	char		*ptr;
	unsigned long	t;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		struct berval bv;
		switch( c->type ) {
		case PC_MAIN:
			bv.bv_len = snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s %d %d %d %ld",
				cm->db.bd_info->bi_type, cm->max_entries, cm->numattrsets,
				cm->num_entries_limit, cm->cc_period );
			bv.bv_val = c->cr_msg;
			value_add_one( &c->rvalue_vals, &bv );
			break;
		case PC_ATTR:
			for (i=0; i<cm->numattrsets; i++) {
				if ( !qm->attr_sets[i].count ) continue;

				bv.bv_len = snprintf( c->cr_msg, sizeof( c->cr_msg ), "%d", i );

				/* count the attr length */
				for ( attr_name = qm->attr_sets[i].attrs;
					attr_name->an_name.bv_val; attr_name++ )
				{
					bv.bv_len += attr_name->an_name.bv_len + 1;
					if ( attr_name->an_desc &&
							( attr_name->an_desc->ad_flags & SLAP_DESC_TEMPORARY ) ) {
						bv.bv_len += STRLENOF("undef:");
					}
				}

				bv.bv_val = ch_malloc( bv.bv_len+1 );
				ptr = lutil_strcopy( bv.bv_val, c->cr_msg );
				for ( attr_name = qm->attr_sets[i].attrs;
					attr_name->an_name.bv_val; attr_name++ ) {
					*ptr++ = ' ';
					if ( attr_name->an_desc &&
							( attr_name->an_desc->ad_flags & SLAP_DESC_TEMPORARY ) ) {
						ptr = lutil_strcopy( ptr, "undef:" );
					}
					ptr = lutil_strcopy( ptr, attr_name->an_name.bv_val );
				}
				ber_bvarray_add( &c->rvalue_vals, &bv );
			}
			if ( !c->rvalue_vals )
				rc = 1;
			break;
		case PC_TEMP:
			for (temp=qm->templates; temp; temp=temp->qmnext) {
				/* HEADS-UP: always print all;
				 * if optional == 0, ignore */
				bv.bv_len = snprintf( c->cr_msg, sizeof( c->cr_msg ),
					" %d %ld %ld %ld %ld",
					temp->attr_set_index,
					temp->ttl,
					temp->negttl,
					temp->limitttl,
					temp->ttr );
				bv.bv_len += temp->querystr.bv_len + 2;
				bv.bv_val = ch_malloc( bv.bv_len+1 );
				ptr = bv.bv_val;
				*ptr++ = '"';
				ptr = lutil_strcopy( ptr, temp->querystr.bv_val );
				*ptr++ = '"';
				strcpy( ptr, c->cr_msg );
				ber_bvarray_add( &c->rvalue_vals, &bv );
			}
			if ( !c->rvalue_vals )
				rc = 1;
			break;
		case PC_BIND:
			for (temp=qm->templates; temp; temp=temp->qmnext) {
				if ( !temp->bindttr ) continue;
				bv.bv_len = snprintf( c->cr_msg, sizeof( c->cr_msg ),
					" %d %ld %s ",
					temp->attr_set_index,
					temp->bindttr,
					ldap_pvt_scope2str( temp->bindscope ));
				bv.bv_len += temp->bindbase.bv_len + temp->bindftemp.bv_len + 4;
				bv.bv_val = ch_malloc( bv.bv_len + 1 );
				ptr = bv.bv_val;
				*ptr++ = '"';
				ptr = lutil_strcopy( ptr, temp->bindftemp.bv_val );
				*ptr++ = '"';
				ptr = lutil_strcopy( ptr, c->cr_msg );
				*ptr++ = '"';
				ptr = lutil_strcopy( ptr, temp->bindbase.bv_val );
				*ptr++ = '"';
				*ptr = '\0';
				ber_bvarray_add( &c->rvalue_vals, &bv );
			}
			if ( !c->rvalue_vals )
				rc = 1;
			break;
		case PC_RESP:
			if ( cm->response_cb == PCACHE_RESPONSE_CB_HEAD ) {
				BER_BVSTR( &bv, "head" );
			} else {
				BER_BVSTR( &bv, "tail" );
			}
			value_add_one( &c->rvalue_vals, &bv );
			break;
		case PC_QUERIES:
			c->value_int = cm->max_queries;
			break;
		case PC_OFFLINE:
			c->value_int = (cm->cc_paused & PCACHE_CC_OFFLINE) != 0;
			break;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		rc = 1;
		switch( c->type ) {
		case PC_ATTR: /* FIXME */
		case PC_TEMP:
		case PC_BIND:
			break;
		case PC_OFFLINE:
			cm->cc_paused &= ~PCACHE_CC_OFFLINE;
			/* If there were cached queries when we went offline,
			 * restart the checker now.
			 */
			if ( cm->num_cached_queries ) {
				ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
				cm->cc_paused = 0;
				ldap_pvt_runqueue_resched( &slapd_rq, cm->cc_arg, 0 );
				ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
			}
			rc = 0;
			break;
		}
		return rc;
	}

	switch( c->type ) {
	case PC_MAIN:
		if ( cm->numattrsets > 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "\"pcache\" directive already provided" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( lutil_atoi( &cm->numattrsets, c->argv[3] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse num attrsets=\"%s\" (arg #3)",
				c->argv[3] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( cm->numattrsets <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "numattrsets (arg #3) must be positive" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( cm->numattrsets > MAX_ATTR_SETS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "numattrsets (arg #3) must be <= %d", MAX_ATTR_SETS );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( !backend_db_init( c->argv[1], &cm->db, -1, NULL )) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unknown backend type (arg #1)" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( lutil_atoi( &cm->max_entries, c->argv[2] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse max entries=\"%s\" (arg #2)",
				c->argv[2] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( cm->max_entries <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "max entries (arg #2) must be positive.\n" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( lutil_atoi( &cm->num_entries_limit, c->argv[4] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse entry limit=\"%s\" (arg #4)",
				c->argv[4] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( cm->num_entries_limit <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "entry limit (arg #4) must be positive" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( cm->num_entries_limit > cm->max_entries ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "entry limit (arg #4) must be less than max entries %d (arg #2)", cm->max_entries );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( lutil_parse_time( c->argv[5], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse period=\"%s\" (arg #5)",
				c->argv[5] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		cm->cc_period = (time_t)t;
		Debug( pcache_debug,
				"Total # of attribute sets to be cached = %d.\n",
				cm->numattrsets, 0, 0 );
		qm->attr_sets = ( struct attr_set * )ch_calloc( cm->numattrsets,
			    			sizeof( struct attr_set ) );
		break;
	case PC_ATTR:
		if ( cm->numattrsets == 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "\"pcache\" directive not provided yet" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( lutil_atoi( &num, c->argv[1] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse attrset #=\"%s\"",
				c->argv[1] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( num < 0 || num >= cm->numattrsets ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "attrset index %d out of bounds (must be %s%d)",
				num, cm->numattrsets > 1 ? "0->" : "", cm->numattrsets - 1 );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		qm->attr_sets[num].flags |= PC_CONFIGURED;
		if ( c->argc == 2 ) {
			/* assume "1.1" */
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"need an explicit attr in attrlist; use \"*\" to indicate all attrs" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;

		} else if ( c->argc == 3 ) {
			if ( strcmp( c->argv[2], LDAP_ALL_USER_ATTRIBUTES ) == 0 ) {
				qm->attr_sets[num].count = 1;
				qm->attr_sets[num].attrs = (AttributeName*)ch_calloc( 2,
					sizeof( AttributeName ) );
				BER_BVSTR( &qm->attr_sets[num].attrs[0].an_name, LDAP_ALL_USER_ATTRIBUTES );
				break;

			} else if ( strcmp( c->argv[2], LDAP_ALL_OPERATIONAL_ATTRIBUTES ) == 0 ) {
				qm->attr_sets[num].count = 1;
				qm->attr_sets[num].attrs = (AttributeName*)ch_calloc( 2,
					sizeof( AttributeName ) );
				BER_BVSTR( &qm->attr_sets[num].attrs[0].an_name, LDAP_ALL_OPERATIONAL_ATTRIBUTES );
				break;

			} else if ( strcmp( c->argv[2], LDAP_NO_ATTRS ) == 0 ) {
				break;
			}
			/* else: fallthru */

		} else if ( c->argc == 4 ) {
			if ( ( strcmp( c->argv[2], LDAP_ALL_USER_ATTRIBUTES ) == 0 && strcmp( c->argv[3], LDAP_ALL_OPERATIONAL_ATTRIBUTES ) == 0 )
				|| ( strcmp( c->argv[2], LDAP_ALL_OPERATIONAL_ATTRIBUTES ) == 0 && strcmp( c->argv[3], LDAP_ALL_USER_ATTRIBUTES ) == 0 ) )
			{
				qm->attr_sets[num].count = 2;
				qm->attr_sets[num].attrs = (AttributeName*)ch_calloc( 3,
					sizeof( AttributeName ) );
				BER_BVSTR( &qm->attr_sets[num].attrs[0].an_name, LDAP_ALL_USER_ATTRIBUTES );
				BER_BVSTR( &qm->attr_sets[num].attrs[1].an_name, LDAP_ALL_OPERATIONAL_ATTRIBUTES );
				break;
			}
			/* else: fallthru */
		}

		if ( c->argc > 2 ) {
			int all_user = 0, all_op = 0;

			qm->attr_sets[num].count = c->argc - 2;
			qm->attr_sets[num].attrs = (AttributeName*)ch_calloc( c->argc - 1,
				sizeof( AttributeName ) );
			attr_name = qm->attr_sets[num].attrs;
			for ( i = 2; i < c->argc; i++ ) {
				attr_name->an_desc = NULL;
				if ( strcmp( c->argv[i], LDAP_NO_ATTRS ) == 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"invalid attr #%d \"%s\" in attrlist",
						i - 2, c->argv[i] );
					Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
					ch_free( qm->attr_sets[num].attrs );
					qm->attr_sets[num].attrs = NULL;
					qm->attr_sets[num].count = 0;
					return 1;
				}
				if ( strcmp( c->argv[i], LDAP_ALL_USER_ATTRIBUTES ) == 0 ) {
					all_user = 1;
					BER_BVSTR( &attr_name->an_name, LDAP_ALL_USER_ATTRIBUTES );
				} else if ( strcmp( c->argv[i], LDAP_ALL_OPERATIONAL_ATTRIBUTES ) == 0 ) {
					all_op = 1;
					BER_BVSTR( &attr_name->an_name, LDAP_ALL_OPERATIONAL_ATTRIBUTES );
				} else {
					if ( strncasecmp( c->argv[i], "undef:", STRLENOF("undef:") ) == 0 ) {
						struct berval bv;
						ber_str2bv( c->argv[i] + STRLENOF("undef:"), 0, 0, &bv );
						attr_name->an_desc = slap_bv2tmp_ad( &bv, NULL );

					} else if ( slap_str2ad( c->argv[i], &attr_name->an_desc, &text ) ) {
						strcpy( c->cr_msg, text );
						Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
						ch_free( qm->attr_sets[num].attrs );
						qm->attr_sets[num].attrs = NULL;
						qm->attr_sets[num].count = 0;
						return 1;
					}
					attr_name->an_name = attr_name->an_desc->ad_cname;
				}
				attr_name->an_oc = NULL;
				attr_name->an_flags = 0;
				if ( attr_name->an_desc == slap_schema.si_ad_objectClass )
					qm->attr_sets[num].flags |= PC_GOT_OC;
				attr_name++;
				BER_BVZERO( &attr_name->an_name );
			}

			/* warn if list contains both "*" and "+" */
			if ( i > 4 && all_user && all_op ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"warning: attribute list contains \"*\" and \"+\"" );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			}
		}
		break;
	case PC_TEMP:
		if ( cm->numattrsets == 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "\"pcache\" directive not provided yet" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( lutil_atoi( &i, c->argv[2] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse template #=\"%s\"",
				c->argv[2] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( i < 0 || i >= cm->numattrsets || 
			!(qm->attr_sets[i].flags & PC_CONFIGURED )) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "template index %d invalid (%s%d)",
				i, cm->numattrsets > 1 ? "0->" : "", cm->numattrsets - 1 );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		{
			AttributeName *attrs;
			int cnt;
			cnt = template_attrs( c->argv[1], &qm->attr_sets[i], &attrs, &text );
			if ( cnt < 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse template: %s",
					text );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
			temp = ch_calloc( 1, sizeof( QueryTemplate ));
			temp->qmnext = qm->templates;
			qm->templates = temp;
			temp->t_attrs.attrs = attrs;
			temp->t_attrs.count = cnt;
		}
		ldap_pvt_thread_rdwr_init( &temp->t_rwlock );
		temp->query = temp->query_last = NULL;
		if ( lutil_parse_time( c->argv[3], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"unable to parse template ttl=\"%s\"",
				c->argv[3] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
pc_temp_fail:
			ch_free( temp->t_attrs.attrs );
			ch_free( temp );
			return( 1 );
		}
		temp->ttl = (time_t)t;
		temp->negttl = (time_t)0;
		temp->limitttl = (time_t)0;
		temp->ttr = (time_t)0;
		switch ( c->argc ) {
		case 7:
			if ( lutil_parse_time( c->argv[6], &t ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to parse template ttr=\"%s\"",
					c->argv[6] );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
				goto pc_temp_fail;
			}
			temp->ttr = (time_t)t;
			/* fallthru */

		case 6:
			if ( lutil_parse_time( c->argv[5], &t ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to parse template sizelimit ttl=\"%s\"",
					c->argv[5] );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
				goto pc_temp_fail;
			}
			temp->limitttl = (time_t)t;
			/* fallthru */

		case 5:
			if ( lutil_parse_time( c->argv[4], &t ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to parse template negative ttl=\"%s\"",
					c->argv[4] );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
				goto pc_temp_fail;
			}
			temp->negttl = (time_t)t;
			break;
		}

		temp->no_of_queries = 0;

		ber_str2bv( c->argv[1], 0, 1, &temp->querystr );
		Debug( pcache_debug, "Template:\n", 0, 0, 0 );
		Debug( pcache_debug, "  query template: %s\n",
				temp->querystr.bv_val, 0, 0 );
		temp->attr_set_index = i;
		qm->attr_sets[i].flags |= PC_REFERENCED;
		temp->qtnext = qm->attr_sets[i].templates;
		qm->attr_sets[i].templates = temp;
		Debug( pcache_debug, "  attributes: \n", 0, 0, 0 );
		if ( ( attrarray = qm->attr_sets[i].attrs ) != NULL ) {
			for ( i=0; attrarray[i].an_name.bv_val; i++ )
				Debug( pcache_debug, "\t%s\n",
					attrarray[i].an_name.bv_val, 0, 0 );
		}
		break;
	case PC_BIND:
		if ( !qm->templates ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "\"pcacheTemplate\" directive not provided yet" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		if ( lutil_atoi( &i, c->argv[2] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse Bind index #=\"%s\"",
				c->argv[2] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( i < 0 || i >= cm->numattrsets || 
			!(qm->attr_sets[i].flags & PC_CONFIGURED )) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "Bind index %d invalid (%s%d)",
				i, cm->numattrsets > 1 ? "0->" : "", cm->numattrsets - 1 );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		{	struct berval bv, tempbv;
			AttributeDescription **descs;
			int ndescs;
			ber_str2bv( c->argv[1], 0, 0, &bv );
			ndescs = ftemp_attrs( &bv, &tempbv, &descs, &text );
			if ( ndescs < 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "unable to parse template: %s",
					text );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
			for ( temp = qm->templates; temp; temp=temp->qmnext ) {
				if ( temp->attr_set_index == i && bvmatch( &tempbv,
					&temp->querystr ))
					break;
			}
			ch_free( tempbv.bv_val );
			if ( !temp ) {
				ch_free( descs );
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "Bind template %s %d invalid",
					c->argv[1], i );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
				return 1;
			}
			ber_dupbv( &temp->bindftemp, &bv );
			temp->bindfattrs = descs;
			temp->bindnattrs = ndescs;
		}
		if ( lutil_parse_time( c->argv[3], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"unable to parse bind ttr=\"%s\"",
				c->argv[3] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
pc_bind_fail:
			ch_free( temp->bindfattrs );
			temp->bindfattrs = NULL;
			ch_free( temp->bindftemp.bv_val );
			BER_BVZERO( &temp->bindftemp );
			return( 1 );
		}
		num = ldap_pvt_str2scope( c->argv[4] );
		if ( num < 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"unable to parse bind scope=\"%s\"",
				c->argv[4] );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			goto pc_bind_fail;
		}
		{
			struct berval dn, ndn;
			ber_str2bv( c->argv[5], 0, 0, &dn );
			rc = dnNormalize( 0, NULL, NULL, &dn, &ndn, NULL );
			if ( rc ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"invalid bind baseDN=\"%s\"",
					c->argv[5] );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
				goto pc_bind_fail;
			}
			if ( temp->bindbase.bv_val )
				ch_free( temp->bindbase.bv_val );
			temp->bindbase = ndn;
		}
		{
			/* convert the template into dummy filter */
			struct berval bv;
			char *eq = temp->bindftemp.bv_val, *e2;
			Filter *f;
			i = 0;
			while ((eq = strchr(eq, '=' ))) {
				eq++;
				if ( eq[0] == ')' )
					i++;
			}
			bv.bv_len = temp->bindftemp.bv_len + i;
			bv.bv_val = ch_malloc( bv.bv_len + 1 );
			for ( e2 = bv.bv_val, eq = temp->bindftemp.bv_val;
				*eq; eq++ ) {
				if ( *eq == '=' ) {
					*e2++ = '=';
					if ( eq[1] == ')' )
						*e2++ = '*';
				} else {
					*e2++ = *eq;
				}
			}
			*e2 = '\0';
			f = str2filter( bv.bv_val );
			if ( !f ) {
				ch_free( bv.bv_val );
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to parse bindfilter=\"%s\"", bv.bv_val );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
				ch_free( temp->bindbase.bv_val );
				BER_BVZERO( &temp->bindbase );
				goto pc_bind_fail;
			}
			if ( temp->bindfilter )
				filter_free( temp->bindfilter );
			if ( temp->bindfilterstr.bv_val )
				ch_free( temp->bindfilterstr.bv_val );
			temp->bindfilterstr = bv;
			temp->bindfilter = f;
		}
		temp->bindttr = (time_t)t;
		temp->bindscope = num;
		cm->cache_binds = 1;
		break;

	case PC_RESP:
		if ( strcasecmp( c->argv[1], "head" ) == 0 ) {
			cm->response_cb = PCACHE_RESPONSE_CB_HEAD;

		} else if ( strcasecmp( c->argv[1], "tail" ) == 0 ) {
			cm->response_cb = PCACHE_RESPONSE_CB_TAIL;

		} else {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "unknown specifier" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		break;
	case PC_QUERIES:
		if ( c->value_int <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "max queries must be positive" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}
		cm->max_queries = c->value_int;
		break;
	case PC_OFFLINE:
		if ( c->value_int )
			cm->cc_paused |= PCACHE_CC_OFFLINE;
		else
			cm->cc_paused &= ~PCACHE_CC_OFFLINE;
		break;
	case PC_PRIVATE_DB:
		if ( cm->db.be_private == NULL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"private database must be defined before setting database specific options" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return( 1 );
		}

		if ( cm->db.bd_info->bi_cf_ocs ) {
			ConfigTable	*ct;
			ConfigArgs	c2 = *c;
			char		*argv0 = c->argv[ 0 ];

			c->argv[ 0 ] = &argv0[ STRLENOF( "pcache-" ) ];

			ct = config_find_keyword( cm->db.bd_info->bi_cf_ocs->co_table, c );
			if ( ct == NULL ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"private database does not recognize specific option '%s'",
					c->argv[ 0 ] );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
				rc = 1;

			} else {
				c->table = cm->db.bd_info->bi_cf_ocs->co_type;
				c->be = &cm->db;
				c->bi = c->be->bd_info;

				rc = config_add_vals( ct, c );

				c->bi = c2.bi;
				c->be = c2.be;
				c->table = c2.table;
			}

			c->argv[ 0 ] = argv0;

		} else if ( cm->db.be_config != NULL ) {
			char	*argv0 = c->argv[ 0 ];

			c->argv[ 0 ] = &argv0[ STRLENOF( "pcache-" ) ];
			rc = cm->db.be_config( &cm->db, c->fname, c->lineno, c->argc, c->argv );
			c->argv[ 0 ] = argv0;

		} else {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"no means to set private database specific options" );
			Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n", c->log, c->cr_msg, 0 );
			return 1;
		}
		break;
	default:
		rc = SLAP_CONF_UNKNOWN;
		break;
	}

	return rc;
}

static int
pcache_db_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	cache_manager* 	cm = on->on_bi.bi_private;

	/* Something for the cache database? */
	if ( cm->db.bd_info && cm->db.bd_info->bi_db_config )
		return cm->db.bd_info->bi_db_config( &cm->db, fname, lineno,
			argc, argv );
	return SLAP_CONF_UNKNOWN;
}

static int
pcache_db_init(
	BackendDB *be,
	ConfigReply *cr)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	cache_manager *cm;
	query_manager *qm;

	cm = (cache_manager *)ch_malloc(sizeof(cache_manager));
	on->on_bi.bi_private = cm;

	qm = (query_manager*)ch_malloc(sizeof(query_manager));

	cm->db = *be;
	SLAP_DBFLAGS(&cm->db) |= SLAP_DBFLAG_NO_SCHEMA_CHECK;
	cm->db.be_private = NULL;
	cm->db.bd_self = &cm->db;
	cm->qm = qm;
	cm->numattrsets = 0;
	cm->num_entries_limit = 5;
	cm->num_cached_queries = 0;
	cm->max_entries = 0;
	cm->cur_entries = 0;
	cm->max_queries = 10000;
	cm->save_queries = 0;
	cm->check_cacheability = 0;
	cm->response_cb = PCACHE_RESPONSE_CB_TAIL;
	cm->defer_db_open = 1;
	cm->cache_binds = 0;
	cm->cc_period = 1000;
	cm->cc_paused = 0;
	cm->cc_arg = NULL;
#ifdef PCACHE_MONITOR
	cm->monitor_cb = NULL;
#endif /* PCACHE_MONITOR */

	qm->attr_sets = NULL;
	qm->templates = NULL;
	qm->lru_top = NULL;
	qm->lru_bottom = NULL;

	qm->qcfunc = query_containment;
	qm->crfunc = cache_replacement;
	qm->addfunc = add_query;
	ldap_pvt_thread_mutex_init(&qm->lru_mutex);

	ldap_pvt_thread_mutex_init(&cm->cache_mutex);

#ifndef PCACHE_MONITOR
	return 0;
#else /* PCACHE_MONITOR */
	return pcache_monitor_db_init( be );
#endif /* PCACHE_MONITOR */
}

static int
pcache_cachedquery_open_cb( Operation *op, SlapReply *rs )
{
	assert( op->o_tag == LDAP_REQ_SEARCH );

	if ( rs->sr_type == REP_SEARCH ) {
		Attribute	*a;

		a = attr_find( rs->sr_entry->e_attrs, ad_cachedQueryURL );
		if ( a != NULL ) {
			BerVarray	*valsp;

			assert( a->a_nvals != NULL );

			valsp = op->o_callback->sc_private;
			assert( *valsp == NULL );

			ber_bvarray_dup_x( valsp, a->a_nvals, op->o_tmpmemctx );
		}
	}

	return 0;
}

static int
pcache_cachedquery_count_cb( Operation *op, SlapReply *rs )
{
	assert( op->o_tag == LDAP_REQ_SEARCH );

	if ( rs->sr_type == REP_SEARCH ) {
		int	*countp = (int *)op->o_callback->sc_private;

		(*countp)++;
	}

	return 0;
}

static int
pcache_db_open2(
	slap_overinst *on,
	ConfigReply *cr )
{
	cache_manager	*cm = on->on_bi.bi_private;
	query_manager*  qm = cm->qm;
	int rc;

	rc = backend_startup_one( &cm->db, cr );
	if ( rc == 0 ) {
		cm->defer_db_open = 0;
	}

	/* There is no runqueue in TOOL mode */
	if (( slapMode & SLAP_SERVER_MODE ) && rc == 0 ) {
		ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
		ldap_pvt_runqueue_insert( &slapd_rq, cm->cc_period,
			consistency_check, on,
			"pcache_consistency", cm->db.be_suffix[0].bv_val );
		ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

		/* Cached database must have the rootdn */
		if ( BER_BVISNULL( &cm->db.be_rootndn )
				|| BER_BVISEMPTY( &cm->db.be_rootndn ) )
		{
			Debug( LDAP_DEBUG_ANY, "pcache_db_open(): "
				"underlying database of type \"%s\"\n"
				"    serving naming context \"%s\"\n"
				"    has no \"rootdn\", required by \"pcache\".\n",
				on->on_info->oi_orig->bi_type,
				cm->db.be_suffix[0].bv_val, 0 );
			return 1;
		}

		if ( cm->save_queries ) {
			void		*thrctx = ldap_pvt_thread_pool_context();
			Connection	conn = { 0 };
			OperationBuffer	opbuf;
			Operation	*op;
			slap_callback	cb = { 0 };
			SlapReply	rs = { REP_RESULT };
			BerVarray	vals = NULL;
			Filter		f = { 0 }, f2 = { 0 };
			AttributeAssertion	ava = ATTRIBUTEASSERTION_INIT;
			AttributeName	attrs[ 2 ] = {{{ 0 }}};

			connection_fake_init2( &conn, &opbuf, thrctx, 0 );
			op = &opbuf.ob_op;

			op->o_bd = &cm->db;

			op->o_tag = LDAP_REQ_SEARCH;
			op->o_protocol = LDAP_VERSION3;
			cb.sc_response = pcache_cachedquery_open_cb;
			cb.sc_private = &vals;
			op->o_callback = &cb;
			op->o_time = slap_get_time();
			op->o_do_not_cache = 1;
			op->o_managedsait = SLAP_CONTROL_CRITICAL;

			op->o_dn = cm->db.be_rootdn;
			op->o_ndn = cm->db.be_rootndn;
			op->o_req_dn = cm->db.be_suffix[ 0 ];
			op->o_req_ndn = cm->db.be_nsuffix[ 0 ];

			op->ors_scope = LDAP_SCOPE_BASE;
			op->ors_deref = LDAP_DEREF_NEVER;
			op->ors_slimit = 1;
			op->ors_tlimit = SLAP_NO_LIMIT;
			op->ors_limit = NULL;
			ber_str2bv( "(pcacheQueryURL=*)", 0, 0, &op->ors_filterstr );
			f.f_choice = LDAP_FILTER_PRESENT;
			f.f_desc = ad_cachedQueryURL;
			op->ors_filter = &f;
			attrs[ 0 ].an_desc = ad_cachedQueryURL;
			attrs[ 0 ].an_name = ad_cachedQueryURL->ad_cname;
			op->ors_attrs = attrs;
			op->ors_attrsonly = 0;

			rc = op->o_bd->be_search( op, &rs );
			if ( rc == LDAP_SUCCESS && vals != NULL ) {
				int	i;

				for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
					if ( url2query( vals[ i ].bv_val, op, qm ) == 0 ) {
						cm->num_cached_queries++;
					}
				}

				ber_bvarray_free_x( vals, op->o_tmpmemctx );
			}

			/* count cached entries */
			f.f_choice = LDAP_FILTER_NOT;
			f.f_not = &f2;
			f2.f_choice = LDAP_FILTER_EQUALITY;
			f2.f_ava = &ava;
			f2.f_av_desc = slap_schema.si_ad_objectClass;
			BER_BVSTR( &f2.f_av_value, "glue" );
			ber_str2bv( "(!(objectClass=glue))", 0, 0, &op->ors_filterstr );

			op->ors_slimit = SLAP_NO_LIMIT;
			op->ors_scope = LDAP_SCOPE_SUBTREE;
			op->ors_attrs = slap_anlist_no_attrs;

			rs_reinit( &rs, REP_RESULT );
			op->o_callback->sc_response = pcache_cachedquery_count_cb;
			op->o_callback->sc_private = &rs.sr_nentries;

			rc = op->o_bd->be_search( op, &rs );

			cm->cur_entries = rs.sr_nentries;

			/* ignore errors */
			rc = 0;
		}
	}
	return rc;
}

static int
pcache_db_open(
	BackendDB *be,
	ConfigReply *cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	cache_manager	*cm = on->on_bi.bi_private;
	query_manager*  qm = cm->qm;
	int		i, ncf = 0, rf = 0, nrf = 0, rc = 0;

	/* check attr sets */
	for ( i = 0; i < cm->numattrsets; i++) {
		if ( !( qm->attr_sets[i].flags & PC_CONFIGURED ) ) {
			if ( qm->attr_sets[i].flags & PC_REFERENCED ) {
				Debug( LDAP_DEBUG_CONFIG, "pcache: attr set #%d not configured but referenced.\n", i, 0, 0 );
				rf++;

			} else {
				Debug( LDAP_DEBUG_CONFIG, "pcache: warning, attr set #%d not configured.\n", i, 0, 0 );
			}
			ncf++;

		} else if ( !( qm->attr_sets[i].flags & PC_REFERENCED ) ) {
			Debug( LDAP_DEBUG_CONFIG, "pcache: attr set #%d configured but not referenced.\n", i, 0, 0 );
			nrf++;
		}
	}

	if ( ncf || rf || nrf ) {
		Debug( LDAP_DEBUG_CONFIG, "pcache: warning, %d attr sets configured but not referenced.\n", nrf, 0, 0 );
		Debug( LDAP_DEBUG_CONFIG, "pcache: warning, %d attr sets not configured.\n", ncf, 0, 0 );
		Debug( LDAP_DEBUG_CONFIG, "pcache: %d attr sets not configured but referenced.\n", rf, 0, 0 );

		if ( rf > 0 ) {
			return 1;
		}
	}

	/* need to inherit something from the original database... */
	cm->db.be_def_limit = be->be_def_limit;
	cm->db.be_limits = be->be_limits;
	cm->db.be_acl = be->be_acl;
	cm->db.be_dfltaccess = be->be_dfltaccess;

	if ( SLAP_DBMONITORING( be ) ) {
		SLAP_DBFLAGS( &cm->db ) |= SLAP_DBFLAG_MONITORING;

	} else {
		SLAP_DBFLAGS( &cm->db ) &= ~SLAP_DBFLAG_MONITORING;
	}

	if ( !cm->defer_db_open ) {
		rc = pcache_db_open2( on, cr );
	}

#ifdef PCACHE_MONITOR
	if ( rc == LDAP_SUCCESS ) {
		rc = pcache_monitor_db_open( be );
	}
#endif /* PCACHE_MONITOR */

	return rc;
}

static void
pcache_free_qbase( void *v )
{
	Qbase *qb = v;
	int i;

	for (i=0; i<3; i++)
		tavl_free( qb->scopes[i], NULL );
	ch_free( qb );
}

static int
pcache_db_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	cache_manager *cm = on->on_bi.bi_private;
	query_manager *qm = cm->qm;
	QueryTemplate *tm;
	int i, rc = 0;

	/* stop the thread ... */
	if ( cm->cc_arg ) {
		ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
		if ( ldap_pvt_runqueue_isrunning( &slapd_rq, cm->cc_arg ) ) {
			ldap_pvt_runqueue_stoptask( &slapd_rq, cm->cc_arg );
		}
		ldap_pvt_runqueue_remove( &slapd_rq, cm->cc_arg );
		ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	}

	if ( cm->save_queries ) {
		CachedQuery	*qc;
		BerVarray	vals = NULL;

		void		*thrctx;
		Connection	conn = { 0 };
		OperationBuffer	opbuf;
		Operation	*op;
		slap_callback	cb = { 0 };

		SlapReply	rs = { REP_RESULT };
		Modifications	mod = {{ 0 }};

		thrctx = ldap_pvt_thread_pool_context();

		connection_fake_init2( &conn, &opbuf, thrctx, 0 );
		op = &opbuf.ob_op;

                mod.sml_numvals = 0;
		if ( qm->templates != NULL ) {
			for ( tm = qm->templates; tm != NULL; tm = tm->qmnext ) {
				for ( qc = tm->query; qc; qc = qc->next ) {
					struct berval	bv;

					if ( query2url( op, qc, &bv, 0 ) == 0 ) {
						ber_bvarray_add_x( &vals, &bv, op->o_tmpmemctx );
                				mod.sml_numvals++;
					}
				}
			}
		}

		op->o_bd = &cm->db;
		op->o_dn = cm->db.be_rootdn;
		op->o_ndn = cm->db.be_rootndn;

		op->o_tag = LDAP_REQ_MODIFY;
		op->o_protocol = LDAP_VERSION3;
		cb.sc_response = slap_null_cb;
		op->o_callback = &cb;
		op->o_time = slap_get_time();
		op->o_do_not_cache = 1;
		op->o_managedsait = SLAP_CONTROL_CRITICAL;

		op->o_req_dn = op->o_bd->be_suffix[0];
		op->o_req_ndn = op->o_bd->be_nsuffix[0];

		mod.sml_op = LDAP_MOD_REPLACE;
		mod.sml_flags = 0;
		mod.sml_desc = ad_cachedQueryURL;
		mod.sml_type = ad_cachedQueryURL->ad_cname;
		mod.sml_values = vals;
		mod.sml_nvalues = NULL;
		mod.sml_next = NULL;
		Debug( pcache_debug,
			"%sSETTING CACHED QUERY URLS\n",
			vals == NULL ? "RE" : "", 0, 0 );

		op->orm_modlist = &mod;

		op->o_bd->be_modify( op, &rs );

		ber_bvarray_free_x( vals, op->o_tmpmemctx );
	}

	/* cleanup stuff inherited from the original database... */
	cm->db.be_limits = NULL;
	cm->db.be_acl = NULL;


	if ( cm->db.bd_info->bi_db_close ) {
		rc = cm->db.bd_info->bi_db_close( &cm->db, NULL );
	}
	while ( (tm = qm->templates) != NULL ) {
		CachedQuery *qc, *qn;
		qm->templates = tm->qmnext;
		for ( qc = tm->query; qc; qc = qn ) {
			qn = qc->next;
			free_query( qc );
		}
		avl_free( tm->qbase, pcache_free_qbase );
		free( tm->querystr.bv_val );
		free( tm->bindfattrs );
		free( tm->bindftemp.bv_val );
		free( tm->bindfilterstr.bv_val );
		free( tm->bindbase.bv_val );
		filter_free( tm->bindfilter );
		ldap_pvt_thread_rdwr_destroy( &tm->t_rwlock );
		free( tm->t_attrs.attrs );
		free( tm );
	}

	for ( i = 0; i < cm->numattrsets; i++ ) {
		int j;

		/* Account of LDAP_NO_ATTRS */
		if ( !qm->attr_sets[i].count ) continue;

		for ( j = 0; !BER_BVISNULL( &qm->attr_sets[i].attrs[j].an_name ); j++ ) {
			if ( qm->attr_sets[i].attrs[j].an_desc &&
					( qm->attr_sets[i].attrs[j].an_desc->ad_flags &
					  SLAP_DESC_TEMPORARY ) ) {
				slap_sl_mfuncs.bmf_free( qm->attr_sets[i].attrs[j].an_desc, NULL );
			}
		}
		free( qm->attr_sets[i].attrs );
	}
	free( qm->attr_sets );
	qm->attr_sets = NULL;

#ifdef PCACHE_MONITOR
	if ( rc == LDAP_SUCCESS ) {
		rc = pcache_monitor_db_close( be );
	}
#endif /* PCACHE_MONITOR */

	return rc;
}

static int
pcache_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	cache_manager *cm = on->on_bi.bi_private;
	query_manager *qm = cm->qm;

	if ( cm->db.be_private != NULL ) {
		backend_stopdown_one( &cm->db );
	}

	ldap_pvt_thread_mutex_destroy( &qm->lru_mutex );
	ldap_pvt_thread_mutex_destroy( &cm->cache_mutex );
	free( qm );
	free( cm );

#ifdef PCACHE_MONITOR
	pcache_monitor_db_destroy( be );
#endif /* PCACHE_MONITOR */

	return 0;
}

#ifdef PCACHE_CONTROL_PRIVDB
/*
        Control ::= SEQUENCE {
             controlType             LDAPOID,
             criticality             BOOLEAN DEFAULT FALSE,
             controlValue            OCTET STRING OPTIONAL }

        controlType ::= 1.3.6.1.4.1.4203.666.11.9.5.1

 * criticality must be TRUE; controlValue must be absent.
 */
static int
parse_privdb_ctrl(
	Operation	*op,
	SlapReply	*rs,
	LDAPControl	*ctrl )
{
	if ( op->o_ctrlflag[ privDB_cid ] != SLAP_CONTROL_NONE ) {
		rs->sr_text = "privateDB control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "privateDB control value not absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !ctrl->ldctl_iscritical ) {
		rs->sr_text = "privateDB control criticality required";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_ctrlflag[ privDB_cid ] = SLAP_CONTROL_CRITICAL;

	return LDAP_SUCCESS;
}

static char *extops[] = {
	LDAP_EXOP_MODIFY_PASSWD,
	NULL
};
#endif /* PCACHE_CONTROL_PRIVDB */

static struct berval pcache_exop_MODIFY_PASSWD = BER_BVC( LDAP_EXOP_MODIFY_PASSWD );
#ifdef PCACHE_EXOP_QUERY_DELETE
static struct berval pcache_exop_QUERY_DELETE = BER_BVC( PCACHE_EXOP_QUERY_DELETE );

#define	LDAP_TAG_EXOP_QUERY_DELETE_BASE	((LBER_CLASS_CONTEXT|LBER_CONSTRUCTED) + 0)
#define	LDAP_TAG_EXOP_QUERY_DELETE_DN	((LBER_CLASS_CONTEXT|LBER_CONSTRUCTED) + 1)
#define	LDAP_TAG_EXOP_QUERY_DELETE_UUID	((LBER_CLASS_CONTEXT|LBER_CONSTRUCTED) + 2)

/*
        ExtendedRequest ::= [APPLICATION 23] SEQUENCE {
             requestName      [0] LDAPOID,
             requestValue     [1] OCTET STRING OPTIONAL }

        requestName ::= 1.3.6.1.4.1.4203.666.11.9.6.1

        requestValue ::= SEQUENCE { CHOICE {
                  baseDN           [0] LDAPDN
                  entryDN          [1] LDAPDN },
             queryID          [2] OCTET STRING (SIZE(16))
                  -- constrained to UUID }

 * Either baseDN or entryDN must be present, to allow database selection.
 *
 * 1. if baseDN and queryID are present, then the query corresponding
 *    to queryID is deleted;
 * 2. if baseDN is present and queryID is absent, then all queries
 *    are deleted;
 * 3. if entryDN is present and queryID is absent, then all queries
 *    corresponding to the queryID values present in entryDN are deleted;
 * 4. if entryDN and queryID are present, then all queries
 *    corresponding to the queryID values present in entryDN are deleted,
 *    but only if the value of queryID is contained in the entry;
 *
 * Currently, only 1, 3 and 4 are implemented.  2 can be obtained by either
 * recursively deleting the database (ldapdelete -r) with PRIVDB control,
 * or by removing the database files.

        ExtendedResponse ::= [APPLICATION 24] SEQUENCE {
             COMPONENTS OF LDAPResult,
             responseName     [10] LDAPOID OPTIONAL,
             responseValue    [11] OCTET STRING OPTIONAL }

 * responseName and responseValue must be absent.
 */

/*
 * - on success, *tagp is either LDAP_TAG_EXOP_QUERY_DELETE_BASE
 *   or LDAP_TAG_EXOP_QUERY_DELETE_DN.
 * - if ndn != NULL, it is set to the normalized DN in the request
 *   corresponding to either the baseDN or the entryDN, according
 *   to *tagp; memory is malloc'ed on the Operation's slab, and must
 *   be freed by the caller.
 * - if uuid != NULL, it is set to point to the normalized UUID;
 *   memory is malloc'ed on the Operation's slab, and must
 *   be freed by the caller.
 */
static int
pcache_parse_query_delete(
	struct berval	*in,
	ber_tag_t	*tagp,
	struct berval	*ndn,
	struct berval	*uuid,
	const char	**text,
	void		*ctx )
{
	int			rc = LDAP_SUCCESS;
	ber_tag_t		tag;
	ber_len_t		len = -1;
	BerElementBuffer	berbuf;
	BerElement		*ber = (BerElement *)&berbuf;
	struct berval		reqdata = BER_BVNULL;

	*text = NULL;

	if ( ndn ) {
		BER_BVZERO( ndn );
	}

	if ( uuid ) {
		BER_BVZERO( uuid );
	}

	if ( in == NULL || in->bv_len == 0 ) {
		*text = "empty request data field in queryDelete exop";
		return LDAP_PROTOCOL_ERROR;
	}

	ber_dupbv_x( &reqdata, in, ctx );

	/* ber_init2 uses reqdata directly, doesn't allocate new buffers */
	ber_init2( ber, &reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );

	if ( tag == LBER_ERROR ) {
		Debug( LDAP_DEBUG_TRACE,
			"pcache_parse_query_delete: decoding error.\n",
			0, 0, 0 );
		goto decoding_error;
	}

	tag = ber_peek_tag( ber, &len );
	if ( tag == LDAP_TAG_EXOP_QUERY_DELETE_BASE
		|| tag == LDAP_TAG_EXOP_QUERY_DELETE_DN )
	{
		*tagp = tag;

		if ( ndn != NULL ) {
			struct berval	dn;

			tag = ber_scanf( ber, "m", &dn );
			if ( tag == LBER_ERROR ) {
				Debug( LDAP_DEBUG_TRACE,
					"pcache_parse_query_delete: DN parse failed.\n",
					0, 0, 0 );
				goto decoding_error;
			}

			rc = dnNormalize( 0, NULL, NULL, &dn, ndn, ctx );
			if ( rc != LDAP_SUCCESS ) {
				*text = "invalid DN in queryDelete exop request data";
				goto done;
			}

		} else {
			tag = ber_scanf( ber, "x" /* "m" */ );
			if ( tag == LBER_DEFAULT ) {
				goto decoding_error;
			}
		}

		tag = ber_peek_tag( ber, &len );
	}

	if ( tag == LDAP_TAG_EXOP_QUERY_DELETE_UUID ) {
		if ( uuid != NULL ) {
			struct berval	bv;
			char		uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];

			tag = ber_scanf( ber, "m", &bv );
			if ( tag == LBER_ERROR ) {
				Debug( LDAP_DEBUG_TRACE,
					"pcache_parse_query_delete: UUID parse failed.\n",
					0, 0, 0 );
				goto decoding_error;
			}

			if ( bv.bv_len != 16 ) {
				Debug( LDAP_DEBUG_TRACE,
					"pcache_parse_query_delete: invalid UUID length %lu.\n",
					(unsigned long)bv.bv_len, 0, 0 );
				goto decoding_error;
			}

			rc = lutil_uuidstr_from_normalized(
				bv.bv_val, bv.bv_len,
				uuidbuf, sizeof( uuidbuf ) );
			if ( rc == -1 ) {
				goto decoding_error;
			}
			ber_str2bv( uuidbuf, rc, 1, uuid );
			rc = LDAP_SUCCESS;

		} else {
			tag = ber_skip_tag( ber, &len );
			if ( tag == LBER_DEFAULT ) {
				goto decoding_error;
			}

			if ( len != 16 ) {
				Debug( LDAP_DEBUG_TRACE,
					"pcache_parse_query_delete: invalid UUID length %lu.\n",
					(unsigned long)len, 0, 0 );
				goto decoding_error;
			}
		}

		tag = ber_peek_tag( ber, &len );
	}

	if ( tag != LBER_DEFAULT || len != 0 ) {
decoding_error:;
		Debug( LDAP_DEBUG_TRACE,
			"pcache_parse_query_delete: decoding error\n",
			0, 0, 0 );
		rc = LDAP_PROTOCOL_ERROR;
		*text = "queryDelete data decoding error";

done:;
		if ( ndn && !BER_BVISNULL( ndn ) ) {
			slap_sl_free( ndn->bv_val, ctx );
			BER_BVZERO( ndn );
		}

		if ( uuid && !BER_BVISNULL( uuid ) ) {
			slap_sl_free( uuid->bv_val, ctx );
			BER_BVZERO( uuid );
		}
	}

	if ( !BER_BVISNULL( &reqdata ) ) {
		ber_memfree_x( reqdata.bv_val, ctx );
	}

	return rc;
}

static int
pcache_exop_query_delete(
	Operation	*op,
	SlapReply	*rs )
{
	BackendDB	*bd = op->o_bd;

	struct berval	uuid = BER_BVNULL,
			*uuidp = NULL;
	char		buf[ SLAP_TEXT_BUFLEN ];
	unsigned	len;
	ber_tag_t	tag = LBER_DEFAULT;

	if ( LogTest( LDAP_DEBUG_STATS ) ) {
		uuidp = &uuid;
	}

	rs->sr_err = pcache_parse_query_delete( op->ore_reqdata,
		&tag, &op->o_req_ndn, uuidp,
		&rs->sr_text, op->o_tmpmemctx );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		return rs->sr_err;
	}

	if ( LogTest( LDAP_DEBUG_STATS ) ) {
		assert( !BER_BVISNULL( &op->o_req_ndn ) );
		len = snprintf( buf, sizeof( buf ), " dn=\"%s\"", op->o_req_ndn.bv_val );

		if ( !BER_BVISNULL( &uuid ) && len < sizeof( buf ) ) {
			snprintf( &buf[ len ], sizeof( buf ) - len, " pcacheQueryId=\"%s\"", uuid.bv_val );
		}

		Debug( LDAP_DEBUG_STATS, "%s QUERY DELETE%s\n",
			op->o_log_prefix, buf, 0 );
	}
	op->o_req_dn = op->o_req_ndn;

	op->o_bd = select_backend( &op->o_req_ndn, 0 );
	if ( op->o_bd == NULL ) {
		send_ldap_error( op, rs, LDAP_NO_SUCH_OBJECT,
			"no global superior knowledge" );
	}
	rs->sr_err = backend_check_restrictions( op, rs,
		(struct berval *)&pcache_exop_QUERY_DELETE );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		goto done;
	}

	if ( op->o_bd->be_extended == NULL ) {
		send_ldap_error( op, rs, LDAP_UNAVAILABLE_CRITICAL_EXTENSION,
			"backend does not support extended operations" );
		goto done;
	}

	op->o_bd->be_extended( op, rs );

done:;
	if ( !BER_BVISNULL( &op->o_req_ndn ) ) {
		op->o_tmpfree( op->o_req_ndn.bv_val, op->o_tmpmemctx );
		BER_BVZERO( &op->o_req_ndn );
		BER_BVZERO( &op->o_req_dn );
	}

	if ( !BER_BVISNULL( &uuid ) ) {
		op->o_tmpfree( uuid.bv_val, op->o_tmpmemctx );
	}

	op->o_bd = bd;

        return rs->sr_err;
}
#endif /* PCACHE_EXOP_QUERY_DELETE */

static int
pcache_op_extended( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	cache_manager	*cm = on->on_bi.bi_private;

#ifdef PCACHE_CONTROL_PRIVDB
	if ( op->o_ctrlflag[ privDB_cid ] == SLAP_CONTROL_CRITICAL ) {
		return pcache_op_privdb( op, rs );
	}
#endif /* PCACHE_CONTROL_PRIVDB */

#ifdef PCACHE_EXOP_QUERY_DELETE
	if ( bvmatch( &op->ore_reqoid, &pcache_exop_QUERY_DELETE ) ) {
		struct berval	uuid = BER_BVNULL;
		ber_tag_t	tag = LBER_DEFAULT;

		rs->sr_err = pcache_parse_query_delete( op->ore_reqdata,
			&tag, NULL, &uuid, &rs->sr_text, op->o_tmpmemctx );
		assert( rs->sr_err == LDAP_SUCCESS );

		if ( tag == LDAP_TAG_EXOP_QUERY_DELETE_DN ) {
			/* remove all queries related to the selected entry */
			rs->sr_err = pcache_remove_entry_queries_from_cache( op,
				cm, &op->o_req_ndn, &uuid );

		} else if ( tag == LDAP_TAG_EXOP_QUERY_DELETE_BASE ) {
			if ( !BER_BVISNULL( &uuid ) ) {
				/* remove the selected query */
				rs->sr_err = pcache_remove_query_from_cache( op,
					cm, &uuid );

			} else {
				/* TODO: remove all queries */
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				rs->sr_text = "deletion of all queries not implemented";
			}
		}

		op->o_tmpfree( uuid.bv_val, op->o_tmpmemctx );
		return rs->sr_err;
	}
#endif /* PCACHE_EXOP_QUERY_DELETE */

	/* We only care if we're configured for Bind caching */
	if ( bvmatch( &op->ore_reqoid, &pcache_exop_MODIFY_PASSWD ) &&
		cm->cache_binds ) {
		/* See if the local entry exists and has a password.
		 * It's too much work to find the matching query, so
		 * we just see if there's a hashed password to update.
		 */
		Operation op2 = *op;
		Entry *e = NULL;
		int rc;
		int doit = 0;

		op2.o_bd = &cm->db;
		op2.o_dn = op->o_bd->be_rootdn;
		op2.o_ndn = op->o_bd->be_rootndn;
		rc = be_entry_get_rw( &op2, &op->o_req_ndn, NULL,
			slap_schema.si_ad_userPassword, 0, &e );
		if ( rc == LDAP_SUCCESS && e ) {
			/* See if a recognized password is hashed here */
			Attribute *a = attr_find( e->e_attrs,
				slap_schema.si_ad_userPassword );
			if ( a && a->a_vals[0].bv_val[0] == '{' &&
				lutil_passwd_scheme( a->a_vals[0].bv_val )) {
				doit = 1;
			}
			be_entry_release_r( &op2, e );
		}

		if ( doit ) {
			rc = overlay_op_walk( op, rs, op_extended, on->on_info,
				on->on_next );
			if ( rc == LDAP_SUCCESS ) {
				req_pwdexop_s *qpw = &op->oq_pwdexop;

				/* We don't care if it succeeds or not */
				pc_setpw( &op2, &qpw->rs_new, cm );
			}
			return rc;
		}
	}
	return SLAP_CB_CONTINUE;
}

static int
pcache_entry_release( Operation  *op, Entry *e, int rw )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	cache_manager	*cm = on->on_bi.bi_private;
	BackendDB *db = op->o_bd;
	int rc;

	op->o_bd = &cm->db;
	rc = be_entry_release_rw( op, e, rw );
	op->o_bd = db;
	return rc;
}

#ifdef PCACHE_MONITOR

static int
pcache_monitor_update(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e,
	void		*priv )
{
	cache_manager	*cm = (cache_manager *) priv;
	query_manager	*qm = cm->qm;

	CachedQuery	*qc;
	BerVarray	vals = NULL;

	attr_delete( &e->e_attrs, ad_cachedQueryURL );
	if ( ( SLAP_OPATTRS( rs->sr_attr_flags ) || ad_inlist( ad_cachedQueryURL, rs->sr_attrs ) )
		&& qm->templates != NULL )
	{
		QueryTemplate *tm;

		for ( tm = qm->templates; tm != NULL; tm = tm->qmnext ) {
			for ( qc = tm->query; qc; qc = qc->next ) {
				struct berval	bv;

				if ( query2url( op, qc, &bv, 1 ) == 0 ) {
					ber_bvarray_add_x( &vals, &bv, op->o_tmpmemctx );
				}
			}
		}


		if ( vals != NULL ) {
			attr_merge_normalize( e, ad_cachedQueryURL, vals, NULL );
			ber_bvarray_free_x( vals, op->o_tmpmemctx );
		}
	}

	{
		Attribute	*a;
		char		buf[ SLAP_TEXT_BUFLEN ];
		struct berval	bv;

		/* number of cached queries */
		a = attr_find( e->e_attrs, ad_numQueries );
		assert( a != NULL );

		bv.bv_val = buf;
		bv.bv_len = snprintf( buf, sizeof( buf ), "%lu", cm->num_cached_queries );

		if ( a->a_nvals != a->a_vals ) {
			ber_bvreplace( &a->a_nvals[ 0 ], &bv );
		}
		ber_bvreplace( &a->a_vals[ 0 ], &bv );

		/* number of cached entries */
		a = attr_find( e->e_attrs, ad_numEntries );
		assert( a != NULL );

		bv.bv_val = buf;
		bv.bv_len = snprintf( buf, sizeof( buf ), "%d", cm->cur_entries );

		if ( a->a_nvals != a->a_vals ) {
			ber_bvreplace( &a->a_nvals[ 0 ], &bv );
		}
		ber_bvreplace( &a->a_vals[ 0 ], &bv );
	}

	return SLAP_CB_CONTINUE;
}

static int
pcache_monitor_free(
	Entry		*e,
	void		**priv )
{
	struct berval	values[ 2 ];
	Modification	mod = { 0 };

	const char	*text;
	char		textbuf[ SLAP_TEXT_BUFLEN ];

	int		rc;

	/* NOTE: if slap_shutdown != 0, priv might have already been freed */
	*priv = NULL;

	/* Remove objectClass */
	mod.sm_op = LDAP_MOD_DELETE;
	mod.sm_desc = slap_schema.si_ad_objectClass;
	mod.sm_values = values;
	mod.sm_numvals = 1;
	values[ 0 ] = oc_olmPCache->soc_cname;
	BER_BVZERO( &values[ 1 ] );

	rc = modify_delete_values( e, &mod, 1, &text,
		textbuf, sizeof( textbuf ) );
	/* don't care too much about return code... */

	/* remove attrs */
	mod.sm_values = NULL;
	mod.sm_desc = ad_cachedQueryURL;
	mod.sm_numvals = 0;
	rc = modify_delete_values( e, &mod, 1, &text,
		textbuf, sizeof( textbuf ) );
	/* don't care too much about return code... */

	/* remove attrs */
	mod.sm_values = NULL;
	mod.sm_desc = ad_numQueries;
	mod.sm_numvals = 0;
	rc = modify_delete_values( e, &mod, 1, &text,
		textbuf, sizeof( textbuf ) );
	/* don't care too much about return code... */

	/* remove attrs */
	mod.sm_values = NULL;
	mod.sm_desc = ad_numEntries;
	mod.sm_numvals = 0;
	rc = modify_delete_values( e, &mod, 1, &text,
		textbuf, sizeof( textbuf ) );
	/* don't care too much about return code... */

	return SLAP_CB_CONTINUE;
}

/*
 * call from within pcache_initialize()
 */
static int
pcache_monitor_initialize( void )
{
	static int	pcache_monitor_initialized = 0;

	if ( backend_info( "monitor" ) == NULL ) {
		return -1;
	}

	if ( pcache_monitor_initialized++ ) {
		return 0;
	}

	return 0;
}

static int
pcache_monitor_db_init( BackendDB *be )
{
	if ( pcache_monitor_initialize() == LDAP_SUCCESS ) {
		SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_MONITORING;
	}

	return 0;
}

static int
pcache_monitor_db_open( BackendDB *be )
{
	slap_overinst		*on = (slap_overinst *)be->bd_info;
	cache_manager		*cm = on->on_bi.bi_private;
	Attribute		*a, *next;
	monitor_callback_t	*cb = NULL;
	int			rc = 0;
	BackendInfo		*mi;
	monitor_extra_t		*mbe;

	if ( !SLAP_DBMONITORING( be ) ) {
		return 0;
	}

	mi = backend_info( "monitor" );
	if ( !mi || !mi->bi_extra ) {
		SLAP_DBFLAGS( be ) ^= SLAP_DBFLAG_MONITORING;
		return 0;
	}
	mbe = mi->bi_extra;

	/* don't bother if monitor is not configured */
	if ( !mbe->is_configured() ) {
		static int warning = 0;

		if ( warning++ == 0 ) {
			Debug( LDAP_DEBUG_ANY, "pcache_monitor_db_open: "
				"monitoring disabled; "
				"configure monitor database to enable\n",
				0, 0, 0 );
		}

		return 0;
	}

	/* alloc as many as required (plus 1 for objectClass) */
	a = attrs_alloc( 1 + 2 );
	if ( a == NULL ) {
		rc = 1;
		goto cleanup;
	}

	a->a_desc = slap_schema.si_ad_objectClass;
	attr_valadd( a, &oc_olmPCache->soc_cname, NULL, 1 );
	next = a->a_next;

	{
		struct berval	bv = BER_BVC( "0" );

		next->a_desc = ad_numQueries;
		attr_valadd( next, &bv, NULL, 1 );
		next = next->a_next;

		next->a_desc = ad_numEntries;
		attr_valadd( next, &bv, NULL, 1 );
		next = next->a_next;
	}

	cb = ch_calloc( sizeof( monitor_callback_t ), 1 );
	cb->mc_update = pcache_monitor_update;
	cb->mc_free = pcache_monitor_free;
	cb->mc_private = (void *)cm;

	/* make sure the database is registered; then add monitor attributes */
	BER_BVZERO( &cm->monitor_ndn );
	rc = mbe->register_overlay( be, on, &cm->monitor_ndn );
	if ( rc == 0 ) {
		rc = mbe->register_entry_attrs( &cm->monitor_ndn, a, cb,
			NULL, -1, NULL);
	}

cleanup:;
	if ( rc != 0 ) {
		if ( cb != NULL ) {
			ch_free( cb );
			cb = NULL;
		}

		if ( a != NULL ) {
			attrs_free( a );
			a = NULL;
		}
	}

	/* store for cleanup */
	cm->monitor_cb = (void *)cb;

	/* we don't need to keep track of the attributes, because
	 * bdb_monitor_free() takes care of everything */
	if ( a != NULL ) {
		attrs_free( a );
	}

	return rc;
}

static int
pcache_monitor_db_close( BackendDB *be )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	cache_manager *cm = on->on_bi.bi_private;

	if ( cm->monitor_cb != NULL ) {
		BackendInfo		*mi = backend_info( "monitor" );
		monitor_extra_t		*mbe;

		if ( mi && &mi->bi_extra ) {
			mbe = mi->bi_extra;
			mbe->unregister_entry_callback( &cm->monitor_ndn,
				(monitor_callback_t *)cm->monitor_cb,
				NULL, 0, NULL );
		}
	}

	return 0;
}

static int
pcache_monitor_db_destroy( BackendDB *be )
{
	return 0;
}

#endif /* PCACHE_MONITOR */

static slap_overinst pcache;

static char *obsolete_names[] = {
	"proxycache",
	NULL
};

#if SLAPD_OVER_PROXYCACHE == SLAPD_MOD_DYNAMIC
static
#endif /* SLAPD_OVER_PROXYCACHE == SLAPD_MOD_DYNAMIC */
int
pcache_initialize()
{
	int i, code;
	struct berval debugbv = BER_BVC("pcache");
	ConfigArgs c;
	char *argv[ 4 ];

	code = slap_loglevel_get( &debugbv, &pcache_debug );
	if ( code ) {
		return code;
	}

#ifdef PCACHE_CONTROL_PRIVDB
	code = register_supported_control( PCACHE_CONTROL_PRIVDB,
		SLAP_CTRL_BIND|SLAP_CTRL_ACCESS|SLAP_CTRL_HIDE, extops,
		parse_privdb_ctrl, &privDB_cid );
	if ( code != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"pcache_initialize: failed to register control %s (%d)\n",
			PCACHE_CONTROL_PRIVDB, code, 0 );
		return code;
	}
#endif /* PCACHE_CONTROL_PRIVDB */

#ifdef PCACHE_EXOP_QUERY_DELETE
	code = load_extop2( (struct berval *)&pcache_exop_QUERY_DELETE,
		SLAP_EXOP_WRITES|SLAP_EXOP_HIDE, pcache_exop_query_delete,
		0 );
	if ( code != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"pcache_initialize: unable to register queryDelete exop: %d.\n",
			code, 0, 0 );
		return code;
	}
#endif /* PCACHE_EXOP_QUERY_DELETE */

	argv[ 0 ] = "back-bdb/back-hdb monitor";
	c.argv = argv;
	c.argc = 3;
	c.fname = argv[0];

	for ( i = 0; s_oid[ i ].name; i++ ) {
		c.lineno = i;
		argv[ 1 ] = s_oid[ i ].name;
		argv[ 2 ] = s_oid[ i ].oid;

		if ( parse_oidm( &c, 0, NULL ) != 0 ) {
			Debug( LDAP_DEBUG_ANY, "pcache_initialize: "
				"unable to add objectIdentifier \"%s=%s\"\n",
				s_oid[ i ].name, s_oid[ i ].oid, 0 );
			return 1;
		}
	}

	for ( i = 0; s_ad[i].desc != NULL; i++ ) {
		code = register_at( s_ad[i].desc, s_ad[i].adp, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"pcache_initialize: register_at #%d failed\n", i, 0, 0 );
			return code;
		}
		(*s_ad[i].adp)->ad_type->sat_flags |= SLAP_AT_HIDE;
	}

	for ( i = 0; s_oc[i].desc != NULL; i++ ) {
		code = register_oc( s_oc[i].desc, s_oc[i].ocp, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"pcache_initialize: register_oc #%d failed\n", i, 0, 0 );
			return code;
		}
		(*s_oc[i].ocp)->soc_flags |= SLAP_OC_HIDE;
	}

	pcache.on_bi.bi_type = "pcache";
	pcache.on_bi.bi_obsolete_names = obsolete_names;
	pcache.on_bi.bi_db_init = pcache_db_init;
	pcache.on_bi.bi_db_config = pcache_db_config;
	pcache.on_bi.bi_db_open = pcache_db_open;
	pcache.on_bi.bi_db_close = pcache_db_close;
	pcache.on_bi.bi_db_destroy = pcache_db_destroy;

	pcache.on_bi.bi_op_search = pcache_op_search;
	pcache.on_bi.bi_op_bind = pcache_op_bind;
#ifdef PCACHE_CONTROL_PRIVDB
	pcache.on_bi.bi_op_compare = pcache_op_privdb;
	pcache.on_bi.bi_op_modrdn = pcache_op_privdb;
	pcache.on_bi.bi_op_modify = pcache_op_privdb;
	pcache.on_bi.bi_op_add = pcache_op_privdb;
	pcache.on_bi.bi_op_delete = pcache_op_privdb;
#endif /* PCACHE_CONTROL_PRIVDB */
	pcache.on_bi.bi_extended = pcache_op_extended;

	pcache.on_bi.bi_entry_release_rw = pcache_entry_release;
	pcache.on_bi.bi_chk_controls = pcache_chk_controls;

	pcache.on_bi.bi_cf_ocs = pcocs;

	code = config_register_schema( pccfg, pcocs );
	if ( code ) return code;

	return overlay_register( &pcache );
}

#if SLAPD_OVER_PROXYCACHE == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return pcache_initialize();
}
#endif

#endif	/* defined(SLAPD_OVER_PROXYCACHE) */
