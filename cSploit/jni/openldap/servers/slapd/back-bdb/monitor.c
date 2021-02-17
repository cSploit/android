/* monitor.c - monitor bdb backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
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
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/stdlib.h>
#include <ac/errno.h>
#include <sys/stat.h>
#include "lutil.h"
#include "back-bdb.h"

#include "../back-monitor/back-monitor.h"

#include "config.h"

static ObjectClass		*oc_olmBDBDatabase;

static AttributeDescription	*ad_olmBDBEntryCache,
	*ad_olmBDBDNCache, *ad_olmBDBIDLCache,
	*ad_olmDbDirectory;

#ifdef BDB_MONITOR_IDX
static int
bdb_monitor_idx_entry_add(
	struct bdb_info	*bdb,
	Entry		*e );

static AttributeDescription	*ad_olmDbNotIndexed;
#endif /* BDB_MONITOR_IDX */

/*
 * NOTE: there's some confusion in monitor OID arc;
 * by now, let's consider:
 * 
 * Subsystems monitor attributes	1.3.6.1.4.1.4203.666.1.55.0
 * Databases monitor attributes		1.3.6.1.4.1.4203.666.1.55.0.1
 * BDB database monitor attributes	1.3.6.1.4.1.4203.666.1.55.0.1.1
 *
 * Subsystems monitor objectclasses	1.3.6.1.4.1.4203.666.3.16.0
 * Databases monitor objectclasses	1.3.6.1.4.1.4203.666.3.16.0.1
 * BDB database monitor objectclasses	1.3.6.1.4.1.4203.666.3.16.0.1.1
 */

static struct {
	char			*name;
	char			*oid;
}		s_oid[] = {
	{ "olmBDBAttributes",			"olmDatabaseAttributes:1" },
	{ "olmBDBObjectClasses",		"olmDatabaseObjectClasses:1" },

	{ NULL }
};

static struct {
	char			*desc;
	AttributeDescription	**ad;
}		s_at[] = {
	{ "( olmBDBAttributes:1 "
		"NAME ( 'olmBDBEntryCache' ) "
		"DESC 'Number of items in Entry Cache' "
		"SUP monitorCounter "
		"NO-USER-MODIFICATION "
		"USAGE dSAOperation )",
		&ad_olmBDBEntryCache },

	{ "( olmBDBAttributes:2 "
		"NAME ( 'olmBDBDNCache' ) "
		"DESC 'Number of items in DN Cache' "
		"SUP monitorCounter "
		"NO-USER-MODIFICATION "
		"USAGE dSAOperation )",
		&ad_olmBDBDNCache },

	{ "( olmBDBAttributes:3 "
		"NAME ( 'olmBDBIDLCache' ) "
		"DESC 'Number of items in IDL Cache' "
		"SUP monitorCounter "
		"NO-USER-MODIFICATION "
		"USAGE dSAOperation )",
		&ad_olmBDBIDLCache },

	{ "( olmDatabaseAttributes:1 "
		"NAME ( 'olmDbDirectory' ) "
		"DESC 'Path name of the directory "
			"where the database environment resides' "
		"SUP monitoredInfo "
		"NO-USER-MODIFICATION "
		"USAGE dSAOperation )",
		&ad_olmDbDirectory },

#ifdef BDB_MONITOR_IDX
	{ "( olmDatabaseAttributes:2 "
		"NAME ( 'olmDbNotIndexed' ) "
		"DESC 'Missing indexes resulting from candidate selection' "
		"SUP monitoredInfo "
		"NO-USER-MODIFICATION "
		"USAGE dSAOperation )",
		&ad_olmDbNotIndexed },
#endif /* BDB_MONITOR_IDX */

	{ NULL }
};

static struct {
	char		*desc;
	ObjectClass	**oc;
}		s_oc[] = {
	/* augments an existing object, so it must be AUXILIARY
	 * FIXME: derive from some ABSTRACT "monitoredEntity"? */
	{ "( olmBDBObjectClasses:1 "
		"NAME ( 'olmBDBDatabase' ) "
		"SUP top AUXILIARY "
		"MAY ( "
			"olmBDBEntryCache "
			"$ olmBDBDNCache "
			"$ olmBDBIDLCache "
			"$ olmDbDirectory "
#ifdef BDB_MONITOR_IDX
			"$ olmDbNotIndexed "
#endif /* BDB_MONITOR_IDX */
			") )",
		&oc_olmBDBDatabase },

	{ NULL }
};

static int
bdb_monitor_update(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e,
	void		*priv )
{
	struct bdb_info		*bdb = (struct bdb_info *) priv;
	Attribute		*a;

	char			buf[ BUFSIZ ];
	struct berval		bv;

	assert( ad_olmBDBEntryCache != NULL );

	a = attr_find( e->e_attrs, ad_olmBDBEntryCache );
	assert( a != NULL );
	bv.bv_val = buf;
	bv.bv_len = snprintf( buf, sizeof( buf ), "%lu", bdb->bi_cache.c_cursize );
	ber_bvreplace( &a->a_vals[ 0 ], &bv );

	a = attr_find( e->e_attrs, ad_olmBDBDNCache );
	assert( a != NULL );
	bv.bv_len = snprintf( buf, sizeof( buf ), "%lu", bdb->bi_cache.c_eiused );
	ber_bvreplace( &a->a_vals[ 0 ], &bv );

	a = attr_find( e->e_attrs, ad_olmBDBIDLCache );
	assert( a != NULL );
	bv.bv_len = snprintf( buf, sizeof( buf ), "%lu", bdb->bi_idl_cache_size );
	ber_bvreplace( &a->a_vals[ 0 ], &bv );
	
#ifdef BDB_MONITOR_IDX
	bdb_monitor_idx_entry_add( bdb, e );
#endif /* BDB_MONITOR_IDX */

	return SLAP_CB_CONTINUE;
}

#if 0	/* uncomment if required */
static int
bdb_monitor_modify(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e,
	void		*priv )
{
	return SLAP_CB_CONTINUE;
}
#endif

static int
bdb_monitor_free(
	Entry		*e,
	void		**priv )
{
	struct berval	values[ 2 ];
	Modification	mod = { 0 };

	const char	*text;
	char		textbuf[ SLAP_TEXT_BUFLEN ];

	int		i, rc;

	/* NOTE: if slap_shutdown != 0, priv might have already been freed */
	*priv = NULL;

	/* Remove objectClass */
	mod.sm_op = LDAP_MOD_DELETE;
	mod.sm_desc = slap_schema.si_ad_objectClass;
	mod.sm_values = values;
	mod.sm_numvals = 1;
	values[ 0 ] = oc_olmBDBDatabase->soc_cname;
	BER_BVZERO( &values[ 1 ] );

	rc = modify_delete_values( e, &mod, 1, &text,
		textbuf, sizeof( textbuf ) );
	/* don't care too much about return code... */

	/* remove attrs */
	mod.sm_values = NULL;
	mod.sm_numvals = 0;
	for ( i = 0; s_at[ i ].desc != NULL; i++ ) {
		mod.sm_desc = *s_at[ i ].ad;
		rc = modify_delete_values( e, &mod, 1, &text,
			textbuf, sizeof( textbuf ) );
		/* don't care too much about return code... */
	}
	
	return SLAP_CB_CONTINUE;
}

#define	bdb_monitor_initialize	BDB_SYMBOL(monitor_initialize)

/*
 * call from within bdb_initialize()
 */
static int
bdb_monitor_initialize( void )
{
	int		i, code;
	ConfigArgs c;
	char	*argv[ 3 ];

	static int	bdb_monitor_initialized = 0;

	/* set to 0 when successfully initialized; otherwise, remember failure */
	static int	bdb_monitor_initialized_failure = 1;

	if ( bdb_monitor_initialized++ ) {
		return bdb_monitor_initialized_failure;
	}

	if ( backend_info( "monitor" ) == NULL ) {
		return -1;
	}

	/* register schema here */

	argv[ 0 ] = "back-bdb/back-hdb monitor";
	c.argv = argv;
	c.argc = 3;
	c.fname = argv[0];

	for ( i = 0; s_oid[ i ].name; i++ ) {
		c.lineno = i;
		argv[ 1 ] = s_oid[ i ].name;
		argv[ 2 ] = s_oid[ i ].oid;

		if ( parse_oidm( &c, 0, NULL ) != 0 ) {
			Debug( LDAP_DEBUG_ANY, LDAP_XSTRING(bdb_monitor_initialize)
				": unable to add "
				"objectIdentifier \"%s=%s\"\n",
				s_oid[ i ].name, s_oid[ i ].oid, 0 );
			return 2;
		}
	}

	for ( i = 0; s_at[ i ].desc != NULL; i++ ) {
		code = register_at( s_at[ i ].desc, s_at[ i ].ad, 1 );
		if ( code != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, LDAP_XSTRING(bdb_monitor_initialize)
				": register_at failed for attributeType (%s)\n",
				s_at[ i ].desc, 0, 0 );
			return 3;

		} else {
			(*s_at[ i ].ad)->ad_type->sat_flags |= SLAP_AT_HIDE;
		}
	}

	for ( i = 0; s_oc[ i ].desc != NULL; i++ ) {
		code = register_oc( s_oc[ i ].desc, s_oc[ i ].oc, 1 );
		if ( code != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, LDAP_XSTRING(bdb_monitor_initialize)
				": register_oc failed for objectClass (%s)\n",
				s_oc[ i ].desc, 0, 0 );
			return 4;

		} else {
			(*s_oc[ i ].oc)->soc_flags |= SLAP_OC_HIDE;
		}
	}

	return ( bdb_monitor_initialized_failure = LDAP_SUCCESS );
}

/*
 * call from within bdb_db_init()
 */
int
bdb_monitor_db_init( BackendDB *be )
{
	struct bdb_info		*bdb = (struct bdb_info *) be->be_private;

	if ( bdb_monitor_initialize() == LDAP_SUCCESS ) {
		/* monitoring in back-bdb is on by default */
		SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_MONITORING;
	}

#ifdef BDB_MONITOR_IDX
	bdb->bi_idx = NULL;
	ldap_pvt_thread_mutex_init( &bdb->bi_idx_mutex );
#endif /* BDB_MONITOR_IDX */

	return 0;
}

/*
 * call from within bdb_db_open()
 */
int
bdb_monitor_db_open( BackendDB *be )
{
	struct bdb_info		*bdb = (struct bdb_info *) be->be_private;
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
			Debug( LDAP_DEBUG_ANY, LDAP_XSTRING(bdb_monitor_db_open)
				": monitoring disabled; "
				"configure monitor database to enable\n",
				0, 0, 0 );
		}

		return 0;
	}

	/* alloc as many as required (plus 1 for objectClass) */
	a = attrs_alloc( 1 + 4 );
	if ( a == NULL ) {
		rc = 1;
		goto cleanup;
	}

	a->a_desc = slap_schema.si_ad_objectClass;
	attr_valadd( a, &oc_olmBDBDatabase->soc_cname, NULL, 1 );
	next = a->a_next;

	{
		struct berval	bv = BER_BVC( "0" );

		next->a_desc = ad_olmBDBEntryCache;
		attr_valadd( next, &bv, NULL, 1 );
		next = next->a_next;

		next->a_desc = ad_olmBDBDNCache;
		attr_valadd( next, &bv, NULL, 1 );
		next = next->a_next;

		next->a_desc = ad_olmBDBIDLCache;
		attr_valadd( next, &bv, NULL, 1 );
		next = next->a_next;
	}

	{
		struct berval	bv, nbv;
		ber_len_t	pathlen = 0, len = 0;
		char		path[ MAXPATHLEN ] = { '\0' };
		char		*fname = bdb->bi_dbenv_home,
				*ptr;

		len = strlen( fname );
		if ( fname[ 0 ] != '/' ) {
			/* get full path name */
			getcwd( path, sizeof( path ) );
			pathlen = strlen( path );

			if ( fname[ 0 ] == '.' && fname[ 1 ] == '/' ) {
				fname += 2;
				len -= 2;
			}
		}

		bv.bv_len = pathlen + STRLENOF( "/" ) + len;
		ptr = bv.bv_val = ch_malloc( bv.bv_len + STRLENOF( "/" ) + 1 );
		if ( pathlen ) {
			ptr = lutil_strncopy( ptr, path, pathlen );
			ptr[ 0 ] = '/';
			ptr++;
		}
		ptr = lutil_strncopy( ptr, fname, len );
		if ( ptr[ -1 ] != '/' ) {
			ptr[ 0 ] = '/';
			ptr++;
		}
		ptr[ 0 ] = '\0';
		
		attr_normalize_one( ad_olmDbDirectory, &bv, &nbv, NULL );

		next->a_desc = ad_olmDbDirectory;
		next->a_vals = ch_calloc( sizeof( struct berval ), 2 );
		next->a_vals[ 0 ] = bv;
		next->a_numvals = 1;

		if ( BER_BVISNULL( &nbv ) ) {
			next->a_nvals = next->a_vals;

		} else {
			next->a_nvals = ch_calloc( sizeof( struct berval ), 2 );
			next->a_nvals[ 0 ] = nbv;
		}

		next = next->a_next;
	}

	cb = ch_calloc( sizeof( monitor_callback_t ), 1 );
	cb->mc_update = bdb_monitor_update;
#if 0	/* uncomment if required */
	cb->mc_modify = bdb_monitor_modify;
#endif
	cb->mc_free = bdb_monitor_free;
	cb->mc_private = (void *)bdb;

	/* make sure the database is registered; then add monitor attributes */
	rc = mbe->register_database( be, &bdb->bi_monitor.bdm_ndn );
	if ( rc == 0 ) {
		rc = mbe->register_entry_attrs( &bdb->bi_monitor.bdm_ndn, a, cb,
			NULL, 0, NULL );
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
	bdb->bi_monitor.bdm_cb = (void *)cb;

	/* we don't need to keep track of the attributes, because
	 * bdb_monitor_free() takes care of everything */
	if ( a != NULL ) {
		attrs_free( a );
	}

	return rc;
}

/*
 * call from within bdb_db_close()
 */
int
bdb_monitor_db_close( BackendDB *be )
{
	struct bdb_info		*bdb = (struct bdb_info *) be->be_private;

	if ( !BER_BVISNULL( &bdb->bi_monitor.bdm_ndn ) ) {
		BackendInfo		*mi = backend_info( "monitor" );
		monitor_extra_t		*mbe;

		if ( mi && &mi->bi_extra ) {
			mbe = mi->bi_extra;
			mbe->unregister_entry_callback( &bdb->bi_monitor.bdm_ndn,
				(monitor_callback_t *)bdb->bi_monitor.bdm_cb,
				NULL, 0, NULL );
		}

		memset( &bdb->bi_monitor, 0, sizeof( bdb->bi_monitor ) );
	}

	return 0;
}

/*
 * call from within bdb_db_destroy()
 */
int
bdb_monitor_db_destroy( BackendDB *be )
{
#ifdef BDB_MONITOR_IDX
	struct bdb_info		*bdb = (struct bdb_info *) be->be_private;

	/* TODO: free tree */
	ldap_pvt_thread_mutex_destroy( &bdb->bi_idx_mutex );
	avl_free( bdb->bi_idx, ch_free );
#endif /* BDB_MONITOR_IDX */

	return 0;
}

#ifdef BDB_MONITOR_IDX

#define BDB_MONITOR_IDX_TYPES	(4)

typedef struct monitor_idx_t monitor_idx_t;

struct monitor_idx_t {
	AttributeDescription	*idx_ad;
	unsigned long		idx_count[BDB_MONITOR_IDX_TYPES];
};

static int
bdb_monitor_bitmask2key( slap_mask_t bitmask )
{
	int	key;

	for ( key = 0; key < 8 * (int)sizeof(slap_mask_t) && !( bitmask & 0x1U );
			key++ )
		bitmask >>= 1;

	return key;
}

static struct berval idxbv[] = {
	BER_BVC( "present=" ),
	BER_BVC( "equality=" ),
	BER_BVC( "approx=" ),
	BER_BVC( "substr=" ),
	BER_BVNULL
};

static ber_len_t
bdb_monitor_idx2len( monitor_idx_t *idx )
{
	int		i;
	ber_len_t	len = 0;

	for ( i = 0; i < BDB_MONITOR_IDX_TYPES; i++ ) {
		if ( idx->idx_count[ i ] != 0 ) {
			len += idxbv[i].bv_len;
		}
	}

	return len;
}

static int
monitor_idx_cmp( const void *p1, const void *p2 )
{
	const monitor_idx_t	*idx1 = (const monitor_idx_t *)p1;
	const monitor_idx_t	*idx2 = (const monitor_idx_t *)p2;

	return SLAP_PTRCMP( idx1->idx_ad, idx2->idx_ad );
}

static int
monitor_idx_dup( void *p1, void *p2 )
{
	monitor_idx_t	*idx1 = (monitor_idx_t *)p1;
	monitor_idx_t	*idx2 = (monitor_idx_t *)p2;

	return SLAP_PTRCMP( idx1->idx_ad, idx2->idx_ad ) == 0 ? -1 : 0;
}

int
bdb_monitor_idx_add(
	struct bdb_info		*bdb,
	AttributeDescription	*desc,
	slap_mask_t		type )
{
	monitor_idx_t		idx_dummy = { 0 },
				*idx;
	int			rc = 0, key;

	idx_dummy.idx_ad = desc;
	key = bdb_monitor_bitmask2key( type ) - 1;
	if ( key >= BDB_MONITOR_IDX_TYPES ) {
		/* invalid index type */
		return -1;
	}

	ldap_pvt_thread_mutex_lock( &bdb->bi_idx_mutex );

	idx = (monitor_idx_t *)avl_find( bdb->bi_idx,
		(caddr_t)&idx_dummy, monitor_idx_cmp );
	if ( idx == NULL ) {
		idx = (monitor_idx_t *)ch_calloc( sizeof( monitor_idx_t ), 1 );
		idx->idx_ad = desc;
		idx->idx_count[ key ] = 1;

		switch ( avl_insert( &bdb->bi_idx, (caddr_t)idx, 
			monitor_idx_cmp, monitor_idx_dup ) )
		{
		case 0:
			break;

		default:
			ch_free( idx );
			rc = -1;
		}

	} else {
		idx->idx_count[ key ]++;
	}

	ldap_pvt_thread_mutex_unlock( &bdb->bi_idx_mutex );

	return rc;
}

static int
bdb_monitor_idx_apply( void *v_idx, void *v_valp )
{
	monitor_idx_t	*idx = (monitor_idx_t *)v_idx;
	BerVarray	*valp = (BerVarray *)v_valp;

	struct berval	bv;
	char		*ptr;
	char		count_buf[ BDB_MONITOR_IDX_TYPES ][ SLAP_TEXT_BUFLEN ];
	ber_len_t	count_len[ BDB_MONITOR_IDX_TYPES ],
			idx_len;
	int		i, num = 0;

	idx_len = bdb_monitor_idx2len( idx );

	bv.bv_len = 0;
	for ( i = 0; i < BDB_MONITOR_IDX_TYPES; i++ ) {
		if ( idx->idx_count[ i ] == 0 ) {
			continue;
		}

		count_len[ i ] = snprintf( count_buf[ i ],
			sizeof( count_buf[ i ] ), "%lu", idx->idx_count[ i ] );
		bv.bv_len += count_len[ i ];
		num++;
	}

	bv.bv_len += idx->idx_ad->ad_cname.bv_len
		+ num
		+ idx_len;
	ptr = bv.bv_val = ch_malloc( bv.bv_len + 1 );
	ptr = lutil_strcopy( ptr, idx->idx_ad->ad_cname.bv_val );
	for ( i = 0; i < BDB_MONITOR_IDX_TYPES; i++ ) {
		if ( idx->idx_count[ i ] == 0 ) {
			continue;
		}

		ptr[ 0 ] = '#';
		++ptr;
		ptr = lutil_strcopy( ptr, idxbv[ i ].bv_val );
		ptr = lutil_strcopy( ptr, count_buf[ i ] );
	}

	ber_bvarray_add( valp, &bv );

	return 0;
}

static int
bdb_monitor_idx_entry_add(
	struct bdb_info	*bdb,
	Entry		*e )
{
	BerVarray	vals = NULL;
	Attribute	*a;

	a = attr_find( e->e_attrs, ad_olmDbNotIndexed );

	ldap_pvt_thread_mutex_lock( &bdb->bi_idx_mutex );

	avl_apply( bdb->bi_idx, bdb_monitor_idx_apply,
		&vals, -1, AVL_INORDER );

	ldap_pvt_thread_mutex_unlock( &bdb->bi_idx_mutex );

	if ( vals != NULL ) {
		if ( a != NULL ) {
			assert( a->a_nvals == a->a_vals );

			ber_bvarray_free( a->a_vals );

		} else {
			Attribute	**ap;

			for ( ap = &e->e_attrs; *ap != NULL; ap = &(*ap)->a_next )
				;
			*ap = attr_alloc( ad_olmDbNotIndexed );
			a = *ap;
		}
		a->a_vals = vals;
		a->a_nvals = a->a_vals;
	}

	return 0;
}

#endif /* BDB_MONITOR_IDX */
