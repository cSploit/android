/* monitor.c - monitor ldap backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999-2003 Howard Chu.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
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
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/stdlib.h>
#include <ac/errno.h>
#include <sys/stat.h>
#include "lutil.h"
#include "back-ldap.h"

#include "config.h"

static ObjectClass		*oc_olmLDAPDatabase;
static ObjectClass		*oc_olmLDAPConnection;

static ObjectClass		*oc_monitorContainer;
static ObjectClass		*oc_monitorCounterObject;

static AttributeDescription	*ad_olmDbURIList;
static AttributeDescription	*ad_olmDbOperations;
static AttributeDescription	*ad_olmDbBoundDN;
static AttributeDescription	*ad_olmDbConnFlags;
static AttributeDescription	*ad_olmDbConnURI;
static AttributeDescription	*ad_olmDbPeerAddress;

/*
 * Stolen from back-monitor/operations.c
 * We don't need the normalized rdn's though.
 */
struct ldap_back_monitor_ops_t {
	struct berval		rdn;
} ldap_back_monitor_op[] = {
	{ BER_BVC( "cn=Bind" ) },
	{ BER_BVC( "cn=Unbind" ) },
	{ BER_BVC( "cn=Search" ) },
	{ BER_BVC( "cn=Compare" ) },
	{ BER_BVC( "cn=Modify" ) },
	{ BER_BVC( "cn=Modrdn" ) },
	{ BER_BVC( "cn=Add" ) },
	{ BER_BVC( "cn=Delete" ) },
	{ BER_BVC( "cn=Abandon" ) },
	{ BER_BVC( "cn=Extended" ) },

	{ BER_BVNULL }
};

/* Corresponds to connection flags in back-ldap.h */
static struct {
	unsigned	flag;
	struct berval	name;
}		s_flag[] = {
	{ LDAP_BACK_FCONN_ISBOUND,	BER_BVC( "bound" ) },
	{ LDAP_BACK_FCONN_ISANON,	BER_BVC( "anonymous" ) },
	{ LDAP_BACK_FCONN_ISPRIV,	BER_BVC( "privileged" ) },
	{ LDAP_BACK_FCONN_ISTLS,	BER_BVC( "TLS" ) },
	{ LDAP_BACK_FCONN_BINDING,	BER_BVC( "binding" ) },
	{ LDAP_BACK_FCONN_TAINTED,	BER_BVC( "tainted" ) },
	{ LDAP_BACK_FCONN_ABANDON,	BER_BVC( "abandon" ) },
	{ LDAP_BACK_FCONN_ISIDASR,	BER_BVC( "idassert" ) },
	{ LDAP_BACK_FCONN_CACHED,	BER_BVC( "cached" ) },

	{ 0 }
};


/*
 * NOTE: there's some confusion in monitor OID arc;
 * by now, let's consider:
 * 
 * Subsystems monitor attributes	1.3.6.1.4.1.4203.666.1.55.0
 * Databases monitor attributes		1.3.6.1.4.1.4203.666.1.55.0.1
 * LDAP database monitor attributes	1.3.6.1.4.1.4203.666.1.55.0.1.2
 *
 * Subsystems monitor objectclasses	1.3.6.1.4.1.4203.666.3.16.0
 * Databases monitor objectclasses	1.3.6.1.4.1.4203.666.3.16.0.1
 * LDAP database monitor objectclasses	1.3.6.1.4.1.4203.666.3.16.0.1.2
 */

static struct {
	char			*name;
	char			*oid;
}		s_oid[] = {
	{ "olmLDAPAttributes",			"olmDatabaseAttributes:2" },
	{ "olmLDAPObjectClasses",		"olmDatabaseObjectClasses:2" },

	{ NULL }
};

static struct {
	char			*desc;
	AttributeDescription	**ad;
}		s_at[] = {
	{ "( olmLDAPAttributes:1 "
		"NAME ( 'olmDbURIList' ) "
		"DESC 'List of URIs a proxy is serving; can be modified run-time' "
		"SUP managedInfo )",
		&ad_olmDbURIList },
	{ "( olmLDAPAttributes:2 "
		"NAME ( 'olmDbOperation' ) "
		"DESC 'monitor operations performed' "
		"SUP monitorCounter "
		"NO-USER-MODIFICATION "
		"USAGE dSAOperation )",
		&ad_olmDbOperations },
	{ "( olmLDAPAttributes:3 "
		"NAME ( 'olmDbBoundDN' ) "
		"DESC 'monitor connection authorization DN' "
		"SUP monitorConnectionAuthzDN "
		"NO-USER-MODIFICATION "
		"USAGE dSAOperation )",
		&ad_olmDbBoundDN },
	{ "( olmLDAPAttributes:4 "
		"NAME ( 'olmDbConnFlags' ) "
		"DESC 'monitor connection flags' "
		"SUP monitoredInfo "
		"NO-USER-MODIFICATION "
		"USAGE dSAOperation )",
		&ad_olmDbConnFlags },
	{ "( olmLDAPAttributes:5 "
		"NAME ( 'olmDbConnURI' ) "
		"DESC 'monitor connection URI' "
		"SUP monitorConnectionPeerAddress "
		"NO-USER-MODIFICATION "
		"USAGE dSAOperation )",
		&ad_olmDbConnURI },
	{ "( olmLDAPAttributes:6 "
		"NAME ( 'olmDbConnPeerAddress' ) "
		"DESC 'monitor connection peer address' "
		"SUP monitorConnectionPeerAddress "
		"NO-USER-MODIFICATION "
		"USAGE dSAOperation )",
		&ad_olmDbPeerAddress },

	{ NULL }
};

static struct {
	char		*name;
	ObjectClass	**oc;
}		s_moc[] = {
	{ "monitorContainer", &oc_monitorContainer },
	{ "monitorCounterObject", &oc_monitorCounterObject },

	{ NULL }
};

static struct {
	char		*desc;
	ObjectClass	**oc;
}		s_oc[] = {
	/* augments an existing object, so it must be AUXILIARY
	 * FIXME: derive from some ABSTRACT "monitoredEntity"? */
	{ "( olmLDAPObjectClasses:1 "
		"NAME ( 'olmLDAPDatabase' ) "
		"SUP top AUXILIARY "
		"MAY ( "
			"olmDbURIList "
			") )",
		&oc_olmLDAPDatabase },
	{ "( olmLDAPObjectClasses:2 "
		"NAME ( 'olmLDAPConnection' ) "
		"SUP monitorConnection STRUCTURAL "
		"MAY ( "
			"olmDbBoundDN "
			"$ olmDbConnFlags "
			"$ olmDbConnURI "
			"$ olmDbConnPeerAddress "
			") )",
		&oc_olmLDAPConnection },

	{ NULL }
};

static int
ldap_back_monitor_update(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e,
	void		*priv )
{
	ldapinfo_t		*li = (ldapinfo_t *)priv;

	Attribute		*a;

	/* update olmDbURIList */
	a = attr_find( e->e_attrs, ad_olmDbURIList );
	if ( a != NULL ) {
		struct berval	bv;

		assert( a->a_vals != NULL );
		assert( !BER_BVISNULL( &a->a_vals[ 0 ] ) );
		assert( BER_BVISNULL( &a->a_vals[ 1 ] ) );

		ldap_pvt_thread_mutex_lock( &li->li_uri_mutex );
		if ( li->li_uri ) {
			ber_str2bv( li->li_uri, 0, 0, &bv );
			if ( !bvmatch( &a->a_vals[ 0 ], &bv ) ) {
				ber_bvreplace( &a->a_vals[ 0 ], &bv );
			}
		}
		ldap_pvt_thread_mutex_unlock( &li->li_uri_mutex );
	}

	return SLAP_CB_CONTINUE;
}

static int
ldap_back_monitor_modify(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e,
	void		*priv )
{
	ldapinfo_t		*li = (ldapinfo_t *) priv;
	
	Attribute		*save_attrs = NULL;
	Modifications		*ml,
				*ml_olmDbURIList = NULL;
	struct berval		ul = BER_BVNULL;
	int			got = 0;

	for ( ml = op->orm_modlist; ml; ml = ml->sml_next ) {
		if ( ml->sml_desc == ad_olmDbURIList ) {
			if ( ml_olmDbURIList != NULL ) {
				rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
				rs->sr_text = "conflicting modifications";
				goto done;
			}

			if ( ml->sml_op != LDAP_MOD_REPLACE ) {
				rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
				rs->sr_text = "modification not allowed";
				goto done;
			}

			ml_olmDbURIList = ml;
			got++;
			continue;
		}
	}

	if ( got == 0 ) {
		return SLAP_CB_CONTINUE;
	}

	save_attrs = attrs_dup( e->e_attrs );

	if ( ml_olmDbURIList != NULL ) {
		Attribute	*a = NULL;
		LDAPURLDesc	*ludlist = NULL;
		int		rc;

		ml = ml_olmDbURIList;
		assert( ml->sml_nvalues != NULL );

		if ( BER_BVISNULL( &ml->sml_nvalues[ 0 ] ) ) {
			rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
			rs->sr_text = "no value provided";
			goto done;
		}

		if ( !BER_BVISNULL( &ml->sml_nvalues[ 1 ] ) ) {
			rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
			rs->sr_text = "multiple values provided";
			goto done;
		}

		rc = ldap_url_parselist_ext( &ludlist,
			ml->sml_nvalues[ 0 ].bv_val, NULL,
			LDAP_PVT_URL_PARSE_NOEMPTY_HOST
				| LDAP_PVT_URL_PARSE_DEF_PORT );
		if ( rc != LDAP_URL_SUCCESS ) {
			rs->sr_err = LDAP_INVALID_SYNTAX;
			rs->sr_text = "unable to parse URI list";
			goto done;
		}

		ul.bv_val = ldap_url_list2urls( ludlist );
		ldap_free_urllist( ludlist );
		if ( ul.bv_val == NULL ) {
			rs->sr_err = LDAP_OTHER;
			goto done;
		}
		ul.bv_len = strlen( ul.bv_val );
		
		a = attr_find( e->e_attrs, ad_olmDbURIList );
		if ( a != NULL ) {
			if ( a->a_nvals == a->a_vals ) {
				a->a_nvals = ch_calloc( sizeof( struct berval ), 2 );
			}

			ber_bvreplace( &a->a_vals[ 0 ], &ul );
			ber_bvreplace( &a->a_nvals[ 0 ], &ul );

		} else {
			attr_merge_normalize_one( e, ad_olmDbURIList, &ul, NULL );
		}
	}

	/* apply changes */
	if ( !BER_BVISNULL( &ul ) ) {
		ldap_pvt_thread_mutex_lock( &li->li_uri_mutex );
		if ( li->li_uri ) {
			ch_free( li->li_uri );
		}
		li->li_uri = ul.bv_val;
		ldap_pvt_thread_mutex_unlock( &li->li_uri_mutex );

		BER_BVZERO( &ul );
	}

done:;
	if ( !BER_BVISNULL( &ul ) ) {
		ldap_memfree( ul.bv_val );
	}

	if ( rs->sr_err == LDAP_SUCCESS ) {
		attrs_free( save_attrs );
		return SLAP_CB_CONTINUE;
	}

	attrs_free( e->e_attrs );
	e->e_attrs = save_attrs;

	return rs->sr_err;
}

static int
ldap_back_monitor_free(
	Entry		*e,
	void		**priv )
{
	ldapinfo_t		*li = (ldapinfo_t *)(*priv);

	*priv = NULL;

	if ( !slapd_shutdown ) {
		memset( &li->li_monitor_info, 0, sizeof( li->li_monitor_info ) );
	}

	return SLAP_CB_CONTINUE;
}

static int
ldap_back_monitor_subsystem_destroy(
	BackendDB		*be,
	monitor_subsys_t	*ms)
{
	free(ms->mss_dn.bv_val);
	BER_BVZERO(&ms->mss_dn);

	free(ms->mss_ndn.bv_val);
	BER_BVZERO(&ms->mss_ndn);

	return LDAP_SUCCESS;
}

/*
 * Connection monitoring subsystem:
 * Tries to mimick what the cn=connections,cn=monitor subsystem does
 * by creating volatile entries for each connection and populating them
 * according to the information attached to the connection.
 * At this moment the only exposed information is the DN used to bind it.
 * Also note that the connection IDs are not and probably never will be
 * stable.
 */

struct ldap_back_monitor_conn_arg {
	Operation *op;
	monitor_subsys_t *ms;
	Entry **ep;
};

/* code stolen from daemon.c */
static int
ldap_back_monitor_conn_peername(
	LDAP		*ld,
	struct berval	*bv)
{
	Sockbuf *sockbuf;
	ber_socket_t socket;
	Sockaddr sa;
	socklen_t salen = sizeof(sa);
	const char *peeraddr = NULL;
	/* we assume INET6_ADDRSTRLEN > INET_ADDRSTRLEN */
	char addr[INET6_ADDRSTRLEN];
#ifdef LDAP_PF_LOCAL
	char peername[MAXPATHLEN + sizeof("PATH=")];
#elif defined(LDAP_PF_INET6)
	char peername[sizeof("IP=[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]:65535")];
#else /* ! LDAP_PF_LOCAL && ! LDAP_PF_INET6 */
	char peername[sizeof("IP=255.255.255.255:65336")];
#endif /* LDAP_PF_LOCAL */

	assert( bv != NULL );

	ldap_get_option( ld, LDAP_OPT_SOCKBUF, (void **)&sockbuf );
	ber_sockbuf_ctrl( sockbuf, LBER_SB_OPT_GET_FD, &socket );
	getpeername( socket, (struct sockaddr *)&sa, &salen );

	switch ( sa.sa_addr.sa_family ) {
#ifdef LDAP_PF_LOCAL
		case AF_LOCAL:
			sprintf( peername, "PATH=%s", sa.sa_un_addr.sun_path );
			break;
#endif /* LDAP_PF_LOCAL */

#ifdef LDAP_PF_INET6
		case AF_INET6:
			if ( IN6_IS_ADDR_V4MAPPED(&sa.sa_in6_addr.sin6_addr) ) {
#if defined( HAVE_GETADDRINFO ) && defined( HAVE_INET_NTOP )
				peeraddr = inet_ntop( AF_INET,
						((struct in_addr *)&sa.sa_in6_addr.sin6_addr.s6_addr[12]),
						addr, sizeof(addr) );
#else /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
				peeraddr = inet_ntoa( *((struct in_addr *)
							&sa.sa_in6_addr.sin6_addr.s6_addr[12]) );
#endif /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
				if ( !peeraddr ) peeraddr = SLAP_STRING_UNKNOWN;
				sprintf( peername, "IP=%s:%d", peeraddr,
						(unsigned) ntohs( sa.sa_in6_addr.sin6_port ) );
			} else {
				peeraddr = inet_ntop( AF_INET6,
						&sa.sa_in6_addr.sin6_addr,
						addr, sizeof addr );
				if ( !peeraddr ) peeraddr = SLAP_STRING_UNKNOWN;
				sprintf( peername, "IP=[%s]:%d", peeraddr,
						(unsigned) ntohs( sa.sa_in6_addr.sin6_port ) );
			}
			break;
#endif /* LDAP_PF_INET6 */

		case AF_INET: {
#if defined( HAVE_GETADDRINFO ) && defined( HAVE_INET_NTOP )
				      peeraddr = inet_ntop( AF_INET, &sa.sa_in_addr.sin_addr,
						      addr, sizeof(addr) );
#else /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
				      peeraddr = inet_ntoa( sa.sa_in_addr.sin_addr );
#endif /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
				      if ( !peeraddr ) peeraddr = SLAP_STRING_UNKNOWN;
				      sprintf( peername, "IP=%s:%d", peeraddr,
						      (unsigned) ntohs( sa.sa_in_addr.sin_port ) );
			      } break;

		default:
			      sprintf( peername, SLAP_STRING_UNKNOWN );
	}

	ber_str2bv( peername, 0, 1, bv );
	return LDAP_SUCCESS;
}

static int
ldap_back_monitor_conn_entry(
	ldapconn_t *lc,
	struct ldap_back_monitor_conn_arg *arg )
{
	Entry *e;
	monitor_entry_t		*mp;
	monitor_extra_t	*mbe = arg->op->o_bd->bd_info->bi_extra;
	char buf[SLAP_TEXT_BUFLEN];
	char *ptr;
	struct berval bv;
	int i;

	bv.bv_val = buf;
	bv.bv_len = snprintf( bv.bv_val, SLAP_TEXT_BUFLEN,
		"cn=Connection %lu", lc->lc_connid );

	e = mbe->entry_stub( &arg->ms->mss_dn, &arg->ms->mss_ndn, &bv,
		oc_monitorContainer, NULL, NULL );

	attr_merge_normalize_one( e, ad_olmDbBoundDN, &lc->lc_bound_ndn, NULL );

	for ( i = 0; s_flag[i].flag; i++ )
	{
		if ( lc->lc_flags & s_flag[i].flag )
		{
			attr_merge_normalize_one( e, ad_olmDbConnFlags, &s_flag[i].name, NULL );
		}
	}

	ldap_get_option( lc->lc_ld, LDAP_OPT_URI, &bv.bv_val );
	ptr = strchr( bv.bv_val, ' ' );
	bv.bv_len = ptr ? ptr - bv.bv_val : strlen(bv.bv_val);
	attr_merge_normalize_one( e, ad_olmDbConnURI, &bv, NULL );
	ch_free( bv.bv_val );

	ldap_back_monitor_conn_peername( lc->lc_ld, &bv );
	attr_merge_normalize_one( e, ad_olmDbPeerAddress, &bv, NULL );
	ch_free( bv.bv_val );

	mp = mbe->entrypriv_create();
	e->e_private = mp;
	mp->mp_info = arg->ms;
	mp->mp_flags = MONITOR_F_SUB | MONITOR_F_VOLATILE;

	*arg->ep = e;
	arg->ep = &mp->mp_next;

	return 0;
}

static int
ldap_back_monitor_conn_create(
	Operation	*op,
	SlapReply	*rs,
	struct berval	*ndn,
	Entry 		*e_parent,
	Entry		**ep )
{
	monitor_entry_t		*mp_parent;
	monitor_subsys_t	*ms;
	ldapinfo_t		*li;
	ldapconn_t		*lc;

	struct ldap_back_monitor_conn_arg *arg;
	int conn_type;

	assert( e_parent->e_private != NULL );

	mp_parent = e_parent->e_private;
	ms = (monitor_subsys_t *)mp_parent->mp_info;
	li = (ldapinfo_t *)ms->mss_private;

	arg = ch_calloc( 1, sizeof(struct ldap_back_monitor_conn_arg) );
	arg->op = op;
	arg->ep = ep;
	arg->ms = ms;

	for ( conn_type = LDAP_BACK_PCONN_FIRST;
		conn_type < LDAP_BACK_PCONN_LAST;
		conn_type++ )
	{
		LDAP_TAILQ_FOREACH( lc,
			&li->li_conn_priv[ conn_type ].lic_priv,
			lc_q )
		{
			ldap_back_monitor_conn_entry( lc, arg );
		}
	}

	avl_apply( li->li_conninfo.lai_tree, (AVL_APPLY)ldap_back_monitor_conn_entry,
		arg, -1, AVL_INORDER );

	ch_free( arg );

	return 0;
}

static int
ldap_back_monitor_conn_init(
	BackendDB		*be,
	monitor_subsys_t	*ms )
{
	ldapinfo_t	*li = (ldapinfo_t *) ms->mss_private;
	monitor_extra_t	*mbe;

	Entry		*e;
	int		rc;

	assert( be != NULL );
	mbe = (monitor_extra_t *) be->bd_info->bi_extra;

	ms->mss_dn = ms->mss_ndn = li->li_monitor_info.lmi_ndn;
	ms->mss_rdn = li->li_monitor_info.lmi_conn_rdn;
	ms->mss_create = ldap_back_monitor_conn_create;
	ms->mss_destroy = ldap_back_monitor_subsystem_destroy;

	e = mbe->entry_stub( &ms->mss_dn, &ms->mss_ndn,
		&ms->mss_rdn, oc_monitorContainer, NULL, NULL );
	if ( e == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"ldap_back_monitor_conn_init: "
			"unable to create entry \"%s,%s\"\n",
			li->li_monitor_info.lmi_conn_rdn.bv_val,
			ms->mss_ndn.bv_val, 0 );
		return( -1 );
	}

	ber_dupbv( &ms->mss_dn, &e->e_name );
	ber_dupbv( &ms->mss_ndn, &e->e_nname );

	rc = mbe->register_entry( e, NULL, ms, MONITOR_F_VOLATILE_CH );

	/* add labeledURI and special, modifiable URI value */
	if ( rc == LDAP_SUCCESS && li->li_uri != NULL ) {
		struct berval		bv;
		Attribute		*a;
		LDAPURLDesc		*ludlist = NULL;
		monitor_callback_t	*cb = NULL;

		a = attr_alloc( ad_olmDbURIList );

		ber_str2bv( li->li_uri, 0, 0, &bv );
		attr_valadd( a, &bv, NULL, 1 );
		attr_normalize( a->a_desc, a->a_vals, &a->a_nvals, NULL );

		rc = ldap_url_parselist_ext( &ludlist,
			li->li_uri, NULL,
			LDAP_PVT_URL_PARSE_NOEMPTY_HOST
				| LDAP_PVT_URL_PARSE_DEF_PORT );
		if ( rc != LDAP_URL_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"ldap_back_monitor_db_open: "
				"unable to parse URI list (ignored)\n",
				0, 0, 0 );
		} else {
			Attribute *a2 = attr_alloc( slap_schema.si_ad_labeledURI );

			a->a_next = a2;

			for ( ; ludlist != NULL; ) {
				LDAPURLDesc	*next = ludlist->lud_next;

				bv.bv_val = ldap_url_desc2str( ludlist );
				assert( bv.bv_val != NULL );
				ldap_free_urldesc( ludlist );
				bv.bv_len = strlen( bv.bv_val );
				attr_valadd( a2, &bv, NULL, 1 );
				ch_free( bv.bv_val );

				ludlist = next;
			}

			attr_normalize( a2->a_desc, a2->a_vals, &a2->a_nvals, NULL );
		}

		cb = ch_calloc( sizeof( monitor_callback_t ), 1 );
		cb->mc_update = ldap_back_monitor_update;
		cb->mc_modify = ldap_back_monitor_modify;
		cb->mc_free = ldap_back_monitor_free;
		cb->mc_private = (void *)li;

		rc = mbe->register_entry_attrs( &ms->mss_ndn, a, cb, NULL, -1, NULL );

		attr_free( a->a_next );
		attr_free( a );

		if ( rc != LDAP_SUCCESS )
		{
			ch_free( cb );
		}
	}

	entry_free( e );

	return rc;
}

/*
 * Operation monitoring subsystem:
 * Looks a lot like the cn=operations,cn=monitor subsystem except that at this
 * moment, only completed operations are counted. Each entry has a separate
 * callback with all the needed information linked there in the structure
 * below so that the callback need not locate it over and over again.
 */

struct ldap_back_monitor_op_counter {
	ldap_pvt_mp_t		*data;
	ldap_pvt_thread_mutex_t	*mutex;
};

static void
ldap_back_monitor_ops_dispose(
	void	**priv)
{
	struct ldap_back_monitor_op_counter *counter = *priv;

	ch_free( counter );
	counter = NULL;
}

static int
ldap_back_monitor_ops_free(
	Entry *e,
	void **priv)
{
	ldap_back_monitor_ops_dispose( priv );
	return LDAP_SUCCESS;
}

static int
ldap_back_monitor_ops_update(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e,
	void		*priv )
{
	struct ldap_back_monitor_op_counter *counter = priv;
	Attribute *a;

	/*TODO
	 * what about initiated/completed?
	 */
	a = attr_find( e->e_attrs, ad_olmDbOperations );
	assert( a != NULL );

	ldap_pvt_thread_mutex_lock( counter->mutex );
	UI2BV( &a->a_vals[ 0 ], *counter->data );
	ldap_pvt_thread_mutex_unlock( counter->mutex );

	return SLAP_CB_CONTINUE;
}

static int
ldap_back_monitor_ops_init(
	BackendDB		*be,
	monitor_subsys_t	*ms )
{
	ldapinfo_t	*li = (ldapinfo_t *) ms->mss_private;

	monitor_extra_t	*mbe;
	Entry		*e, *parent;
	int		rc;
	slap_op_t	op;
	struct berval	value = BER_BVC( "0" );

	assert( be != NULL );

	mbe = (monitor_extra_t *) be->bd_info->bi_extra;

	ms->mss_dn = ms->mss_ndn = li->li_monitor_info.lmi_ndn;
	ms->mss_rdn = li->li_monitor_info.lmi_ops_rdn;
	ms->mss_destroy = ldap_back_monitor_subsystem_destroy;

	parent = mbe->entry_stub( &ms->mss_dn, &ms->mss_ndn,
		&ms->mss_rdn, oc_monitorContainer, NULL, NULL );
	if ( parent == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"ldap_back_monitor_ops_init: "
			"unable to create entry \"%s,%s\"\n",
			li->li_monitor_info.lmi_ops_rdn.bv_val,
			ms->mss_ndn.bv_val, 0 );
		return( -1 );
	}

	ber_dupbv( &ms->mss_dn, &parent->e_name );
	ber_dupbv( &ms->mss_ndn, &parent->e_nname );

	rc = mbe->register_entry( parent, NULL, ms, MONITOR_F_PERSISTENT_CH );
	if ( rc != LDAP_SUCCESS )
	{
		Debug( LDAP_DEBUG_ANY,
			"ldap_back_monitor_ops_init: "
			"unable to register entry \"%s\" for monitoring\n",
			parent->e_name.bv_val, 0, 0 );
		goto done;
	}

	for ( op = 0; op < SLAP_OP_LAST; op++ )
	{
		monitor_callback_t *cb;
		struct ldap_back_monitor_op_counter *counter;

		e = mbe->entry_stub( &parent->e_name, &parent->e_nname,
			&ldap_back_monitor_op[op].rdn,
			oc_monitorCounterObject, NULL, NULL );
		if ( e == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"ldap_back_monitor_ops_init: "
				"unable to create entry \"%s,%s\"\n",
				ldap_back_monitor_op[op].rdn.bv_val,
				parent->e_nname.bv_val, 0 );
			return( -1 );
		}

		attr_merge_normalize_one( e, ad_olmDbOperations, &value, NULL );

		counter = ch_malloc( sizeof( struct ldap_back_monitor_op_counter ) );
		counter->data = &li->li_ops_completed[ op ];
		counter->mutex = &li->li_counter_mutex;

		/*
		 * We cannot share a single callback between entries.
		 *
		 * monitor_cache_destroy() tries to free all callbacks and it's called
		 * before mss_destroy() so we have no chance of handling it ourselves
		 */
		cb = ch_calloc( sizeof( monitor_callback_t ), 1 );
		cb->mc_update = ldap_back_monitor_ops_update;
		cb->mc_free = ldap_back_monitor_ops_free;
		cb->mc_dispose = ldap_back_monitor_ops_dispose;
		cb->mc_private = (void *)counter;

		rc = mbe->register_entry( e, cb, ms, 0 );

		/* TODO: register_entry has stored a duplicate so we might actually reuse it
		 * instead of recreating it every time... */
		entry_free( e );

		if ( rc != LDAP_SUCCESS )
		{
			Debug( LDAP_DEBUG_ANY,
				"ldap_back_monitor_ops_init: "
				"unable to register entry \"%s\" for monitoring\n",
				e->e_name.bv_val, 0, 0 );
			ch_free( cb );
			break;
		}
	}

done:
	entry_free( parent );

	return rc;
}

/*
 * call from within ldap_back_initialize()
 */
static int
ldap_back_monitor_initialize( void )
{
	int		i, code;
	ConfigArgs c;
	char	*argv[ 3 ];

	static int	ldap_back_monitor_initialized = 0;

	/* set to 0 when successfully initialized; otherwise, remember failure */
	static int	ldap_back_monitor_initialized_failure = 1;

	/* register schema here; if compiled as dynamic object,
	 * must be loaded __after__ back_monitor.la */

	if ( ldap_back_monitor_initialized++ ) {
		return ldap_back_monitor_initialized_failure;
	}

	if ( backend_info( "monitor" ) == NULL ) {
		return -1;
	}

	argv[ 0 ] = "back-ldap monitor";
	c.argv = argv;
	c.argc = 3;
	c.fname = argv[0];
	for ( i = 0; s_oid[ i ].name; i++ ) {
	
		argv[ 1 ] = s_oid[ i ].name;
		argv[ 2 ] = s_oid[ i ].oid;

		if ( parse_oidm( &c, 0, NULL ) != 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"ldap_back_monitor_initialize: unable to add "
				"objectIdentifier \"%s=%s\"\n",
				s_oid[ i ].name, s_oid[ i ].oid, 0 );
			return 2;
		}
	}

	for ( i = 0; s_at[ i ].desc != NULL; i++ ) {
		code = register_at( s_at[ i ].desc, s_at[ i ].ad, 1 );
		if ( code != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"ldap_back_monitor_initialize: register_at failed for attributeType (%s)\n",
				s_at[ i ].desc, 0, 0 );
			return 3;

		} else {
			(*s_at[ i ].ad)->ad_type->sat_flags |= SLAP_AT_HIDE;
		}
	}

	for ( i = 0; s_oc[ i ].desc != NULL; i++ ) {
		code = register_oc( s_oc[ i ].desc, s_oc[ i ].oc, 1 );
		if ( code != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"ldap_back_monitor_initialize: register_oc failed for objectClass (%s)\n",
				s_oc[ i ].desc, 0, 0 );
			return 4;

		} else {
			(*s_oc[ i ].oc)->soc_flags |= SLAP_OC_HIDE;
		}
	}

	for ( i = 0; s_moc[ i ].name != NULL; i++ ) {
		*s_moc[i].oc = oc_find( s_moc[ i ].name );
		if ( ! *s_moc[i].oc ) {
			Debug( LDAP_DEBUG_ANY,
				"ldap_back_monitor_initialize: failed to find objectClass (%s)\n",
				s_moc[ i ].name, 0, 0 );
			return 5;

		}
	}

	return ( ldap_back_monitor_initialized_failure = LDAP_SUCCESS );
}

/*
 * call from within ldap_back_db_init()
 */
int
ldap_back_monitor_db_init( BackendDB *be )
{
	int	rc;

	rc = ldap_back_monitor_initialize();
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

#if 0	/* uncomment to turn monitoring on by default */
	SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_MONITORING;
#endif

	return 0;
}

/*
 * call from within ldap_back_db_open()
 */
int
ldap_back_monitor_db_open( BackendDB *be )
{
	ldapinfo_t		*li = (ldapinfo_t *) be->be_private;
	monitor_subsys_t	*mss = li->li_monitor_info.lmi_mss;
	int			rc = 0;
	BackendInfo		*mi;
	monitor_extra_t		*mbe;

	if ( !SLAP_DBMONITORING( be ) ) {
		return 0;
	}

	/* check if monitor is configured and usable */
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
			Debug( LDAP_DEBUG_ANY, "ldap_back_monitor_db_open: "
				"monitoring disabled; "
				"configure monitor database to enable\n",
				0, 0, 0 );
		}

		return 0;
	}

	/* caller (e.g. an overlay based on back-ldap) may want to use
	 * a different DN and RDNs... */
	if ( BER_BVISNULL( &li->li_monitor_info.lmi_ndn ) ) {
		rc = mbe->register_database( be, &li->li_monitor_info.lmi_ndn );
		if ( rc != 0 ) {
			Debug( LDAP_DEBUG_ANY, "ldap_back_monitor_db_open: "
				"failed to register the databse with back-monitor\n",
				0, 0, 0 );
		}
	}
	if ( BER_BVISNULL( &li->li_monitor_info.lmi_conn_rdn ) ) {
		ber_str2bv( "cn=Connections", 0, 1,
			&li->li_monitor_info.lmi_conn_rdn );
	}
	if ( BER_BVISNULL( &li->li_monitor_info.lmi_ops_rdn ) ) {
		ber_str2bv( "cn=Operations", 0, 1,
			&li->li_monitor_info.lmi_ops_rdn );
	}

	/* set up the subsystems used to create the operation and
	 * volatile connection entries */

	mss->mss_name = "back-ldap connections";
	mss->mss_flags = MONITOR_F_VOLATILE_CH;
	mss->mss_open = ldap_back_monitor_conn_init;
	mss->mss_private = li;

	if ( mbe->register_subsys_late( mss ) )
	{
		Debug( LDAP_DEBUG_ANY,
			"ldap_back_monitor_db_open: "
			"failed to register connection subsystem", 0, 0, 0 );
		return -1;
	}

	mss++;

	mss->mss_name = "back-ldap operations";
	mss->mss_flags = MONITOR_F_PERSISTENT_CH;
	mss->mss_open = ldap_back_monitor_ops_init;
	mss->mss_private = li;

	if ( mbe->register_subsys_late( mss ) )
	{
		Debug( LDAP_DEBUG_ANY,
			"ldap_back_monitor_db_open: "
			"failed to register operation subsystem", 0, 0, 0 );
		return -1;
	}

	return rc;
}

/*
 * call from within ldap_back_db_close()
 */
int
ldap_back_monitor_db_close( BackendDB *be )
{
	ldapinfo_t		*li = (ldapinfo_t *) be->be_private;

	if ( li && !BER_BVISNULL( &li->li_monitor_info.lmi_ndn ) ) {
		BackendInfo		*mi;
		monitor_extra_t		*mbe;

		/* check if monitor is configured and usable */
		mi = backend_info( "monitor" );
		if ( mi && mi->bi_extra ) {
 			mbe = mi->bi_extra;

			/*TODO
			 * Unregister all entries our subsystems have created.
			 * Will only really be necessary when
			 * SLAPD_CONFIG_DELETE is enabled.
			 *
			 * Might need a way to unregister subsystems instead.
			 */
		}
	}

	return 0;
}

/*
 * call from within ldap_back_db_destroy()
 */
int
ldap_back_monitor_db_destroy( BackendDB *be )
{
	ldapinfo_t		*li = (ldapinfo_t *) be->be_private;

	if ( li ) {
		memset( &li->li_monitor_info, 0, sizeof( li->li_monitor_info ) );
	}

	return 0;
}

