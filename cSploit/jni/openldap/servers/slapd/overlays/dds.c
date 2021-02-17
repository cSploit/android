/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
 * Portions Copyright 2005-2006 SysNet s.n.c.
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
 * in OpenLDAP Software, sponsored by SysNet s.n.c.
 */

#include "portable.h"

#ifdef SLAPD_OVER_DDS

#include <stdio.h>

#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "lutil.h"
#include "ldap_rq.h"

#include "config.h"

#define	DDS_RF2589_MAX_TTL		(31557600)	/* 1 year + 6 hours */
#define	DDS_RF2589_DEFAULT_TTL		(86400)		/* 1 day */
#define	DDS_DEFAULT_INTERVAL		(3600)		/* 1 hour */

typedef struct dds_info_t {
	unsigned		di_flags;
#define	DDS_FOFF		(0x1U)		/* is this really needed? */
#define	DDS_SET(di, f)		( (di)->di_flags & (f) )

#define DDS_OFF(di)		DDS_SET( (di), DDS_FOFF )

	time_t			di_max_ttl;
	time_t			di_min_ttl;
	time_t			di_default_ttl;
#define	DDS_DEFAULT_TTL(di)	\
	( (di)->di_default_ttl ? (di)->di_default_ttl : (di)->di_max_ttl )

	time_t			di_tolerance;

	/* expire check interval and task */
	time_t			di_interval;
#define	DDS_INTERVAL(di)	\
	( (di)->di_interval ? (di)->di_interval : DDS_DEFAULT_INTERVAL )
	struct re_s		*di_expire_task;

	/* allows to limit the maximum number of dynamic objects */
	ldap_pvt_thread_mutex_t	di_mutex;
	int			di_num_dynamicObjects;
	int			di_max_dynamicObjects;

	/* used to advertize the dynamicSubtrees in the root DSE,
	 * and to select the database in the expiration task */
	BerVarray		di_suffix;
	BerVarray		di_nsuffix;
} dds_info_t;

static struct berval slap_EXOP_REFRESH = BER_BVC( LDAP_EXOP_REFRESH );
static AttributeDescription	*ad_entryExpireTimestamp;

/* list of expired DNs */
typedef struct dds_expire_t {
	struct berval		de_ndn;
	struct dds_expire_t	*de_next;
} dds_expire_t;

typedef struct dds_cb_t {
	dds_expire_t	*dc_ndnlist;
} dds_cb_t;

static int
dds_expire_cb( Operation *op, SlapReply *rs )
{
	dds_cb_t	*dc = (dds_cb_t *)op->o_callback->sc_private;
	dds_expire_t	*de;
	int		rc;

	switch ( rs->sr_type ) {
	case REP_SEARCH:
		/* alloc list and buffer for berval all in one */
		de = op->o_tmpalloc( sizeof( dds_expire_t ) + rs->sr_entry->e_nname.bv_len + 1,
			op->o_tmpmemctx );

		de->de_next = dc->dc_ndnlist;
		dc->dc_ndnlist = de;

		de->de_ndn.bv_len = rs->sr_entry->e_nname.bv_len;
		de->de_ndn.bv_val = (char *)&de[ 1 ];
		AC_MEMCPY( de->de_ndn.bv_val, rs->sr_entry->e_nname.bv_val,
			rs->sr_entry->e_nname.bv_len + 1 );
		rc = 0;
		break;

	case REP_SEARCHREF:
	case REP_RESULT:
		rc = rs->sr_err;
		break;

	default:
		assert( 0 );
	}

	return rc;
}

static int
dds_expire( void *ctx, dds_info_t *di )
{
	Connection	conn = { 0 };
	OperationBuffer opbuf;
	Operation	*op;
	slap_callback	sc = { 0 };
	dds_cb_t	dc = { 0 };
	dds_expire_t	*de = NULL, **dep;
	SlapReply	rs = { REP_RESULT };

	time_t		expire;
	char		tsbuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
	struct berval	ts;

	int		ndeletes, ntotdeletes;

	int		rc;
	char		*extra = "";

	connection_fake_init2( &conn, &opbuf, ctx, 0 );
	op = &opbuf.ob_op;

	op->o_tag = LDAP_REQ_SEARCH;
	memset( &op->oq_search, 0, sizeof( op->oq_search ) );

	op->o_bd = select_backend( &di->di_nsuffix[ 0 ], 0 );

	op->o_req_dn = op->o_bd->be_suffix[ 0 ];
	op->o_req_ndn = op->o_bd->be_nsuffix[ 0 ];

	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;

	op->ors_scope = LDAP_SCOPE_SUBTREE;
	op->ors_tlimit = DDS_INTERVAL( di )/2 + 1;
	op->ors_slimit = SLAP_NO_LIMIT;
	op->ors_attrs = slap_anlist_no_attrs;

	expire = slap_get_time() - di->di_tolerance;
	ts.bv_val = tsbuf;
	ts.bv_len = sizeof( tsbuf );
	slap_timestamp( &expire, &ts );

	op->ors_filterstr.bv_len = STRLENOF( "(&(objectClass=" ")(" "<=" "))" )
		+ slap_schema.si_oc_dynamicObject->soc_cname.bv_len
		+ ad_entryExpireTimestamp->ad_cname.bv_len
		+ ts.bv_len;
	op->ors_filterstr.bv_val = op->o_tmpalloc( op->ors_filterstr.bv_len + 1, op->o_tmpmemctx );
	snprintf( op->ors_filterstr.bv_val, op->ors_filterstr.bv_len + 1,
		"(&(objectClass=%s)(%s<=%s))",
		slap_schema.si_oc_dynamicObject->soc_cname.bv_val,
		ad_entryExpireTimestamp->ad_cname.bv_val, ts.bv_val );

	op->ors_filter = str2filter_x( op, op->ors_filterstr.bv_val );
	if ( op->ors_filter == NULL ) {
		rs.sr_err = LDAP_OTHER;
		goto done_search;
	}
	
	op->o_callback = &sc;
	sc.sc_response = dds_expire_cb;
	sc.sc_private = &dc;

	(void)op->o_bd->bd_info->bi_op_search( op, &rs );

done_search:;
	op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
	filter_free_x( op, op->ors_filter, 1 );

	rc = rs.sr_err;
	switch ( rs.sr_err ) {
	case LDAP_SUCCESS:
		break;

	case LDAP_NO_SUCH_OBJECT:
		/* (ITS#5267) database not created yet? */
		rs.sr_err = LDAP_SUCCESS;
		extra = " (ignored)";
		/* fallthru */

	default:
		Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"DDS expired objects lookup failed err=%d%s\n",
			rc, extra );
		goto done;
	}

	op->o_tag = LDAP_REQ_DELETE;
	op->o_callback = &sc;
	sc.sc_response = slap_null_cb;
	sc.sc_private = NULL;

	for ( ntotdeletes = 0, ndeletes = 1; dc.dc_ndnlist != NULL  && ndeletes > 0; ) {
		ndeletes = 0;

		for ( dep = &dc.dc_ndnlist; *dep != NULL; ) {
			de = *dep;

			op->o_req_dn = de->de_ndn;
			op->o_req_ndn = de->de_ndn;
			(void)op->o_bd->bd_info->bi_op_delete( op, &rs );
			switch ( rs.sr_err ) {
			case LDAP_SUCCESS:
				Log1( LDAP_DEBUG_STATS, LDAP_LEVEL_INFO,
					"DDS dn=\"%s\" expired.\n",
					de->de_ndn.bv_val );
				ndeletes++;
				break;

			case LDAP_NOT_ALLOWED_ON_NONLEAF:
				Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_NOTICE,
					"DDS dn=\"%s\" is non-leaf; "
					"deferring.\n",
					de->de_ndn.bv_val );
				dep = &de->de_next;
				de = NULL;
				break;
	
			default:
				Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_NOTICE,
					"DDS dn=\"%s\" err=%d; "
					"deferring.\n",
					de->de_ndn.bv_val, rs.sr_err );
				break;
			}

			if ( de != NULL ) {
				*dep = de->de_next;
				op->o_tmpfree( de, op->o_tmpmemctx );
			}
		}

		ntotdeletes += ndeletes;
	}

	rs.sr_err = LDAP_SUCCESS;

	Log1( LDAP_DEBUG_STATS, LDAP_LEVEL_INFO,
		"DDS expired=%d\n", ntotdeletes );

done:;
	return rs.sr_err;
}

static void *
dds_expire_fn( void *ctx, void *arg )
{
	struct re_s     *rtask = arg;
	dds_info_t	*di = rtask->arg;

	assert( di->di_expire_task == rtask );

	(void)dds_expire( ctx, di );
	
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	if ( ldap_pvt_runqueue_isrunning( &slapd_rq, rtask )) {
		ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
	}
	ldap_pvt_runqueue_resched( &slapd_rq, rtask, 0 );
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

	return NULL;
}

/* frees the callback */
static int
dds_freeit_cb( Operation *op, SlapReply *rs )
{
	op->o_tmpfree( op->o_callback, op->o_tmpmemctx );
	op->o_callback = NULL;

	return SLAP_CB_CONTINUE;
}

/* updates counter - installed on add/delete only if required */
static int
dds_counter_cb( Operation *op, SlapReply *rs )
{
	assert( rs->sr_type == REP_RESULT );

	if ( rs->sr_err == LDAP_SUCCESS ) {
		dds_info_t	*di = op->o_callback->sc_private;

		ldap_pvt_thread_mutex_lock( &di->di_mutex );
		switch ( op->o_tag ) {
		case LDAP_REQ_DELETE:
			assert( di->di_num_dynamicObjects > 0 );
			di->di_num_dynamicObjects--;
			break;

		case LDAP_REQ_ADD:
			assert( di->di_num_dynamicObjects < di->di_max_dynamicObjects );
			di->di_num_dynamicObjects++;
			break;

		default:
			assert( 0 );
		}
		ldap_pvt_thread_mutex_unlock( &di->di_mutex );
	}

	return dds_freeit_cb( op, rs );
}

static int
dds_op_add( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	dds_info_t	*di = on->on_bi.bi_private;
	int		is_dynamicObject;

	if ( DDS_OFF( di ) ) {
		return SLAP_CB_CONTINUE;
	}

	is_dynamicObject = is_entry_dynamicObject( op->ora_e );

	/* FIXME: do not allow this right now, pending clarification */
	if ( is_dynamicObject ) {
		rs->sr_err = LDAP_SUCCESS;

		if ( is_entry_referral( op->ora_e ) ) {
			rs->sr_err = LDAP_OBJECT_CLASS_VIOLATION;
			rs->sr_text = "a referral cannot be a dynamicObject";

		} else if ( is_entry_alias( op->ora_e ) ) {
			rs->sr_err = LDAP_OBJECT_CLASS_VIOLATION;
			rs->sr_text = "an alias cannot be a dynamicObject";
		}

		if ( rs->sr_err != LDAP_SUCCESS ) {
			op->o_bd->bd_info = (BackendInfo *)on->on_info;
			send_ldap_result( op, rs );
			return rs->sr_err;
		}
	}

	/* we don't allow dynamicObjects to have static subordinates */
	if ( !dn_match( &op->o_req_ndn, &op->o_bd->be_nsuffix[ 0 ] ) ) {
		struct berval	p_ndn;
		Entry		*e = NULL;
		int		rc;
		BackendInfo	*bi = op->o_bd->bd_info;

		dnParent( &op->o_req_ndn, &p_ndn );
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		rc = be_entry_get_rw( op, &p_ndn,
			slap_schema.si_oc_dynamicObject, NULL, 0, &e );
		if ( rc == LDAP_SUCCESS && e != NULL ) {
			if ( !is_dynamicObject ) {
				/* return referral only if "disclose"
				 * is granted on the object */
				if ( ! access_allowed( op, e,
						slap_schema.si_ad_entry,
						NULL, ACL_DISCLOSE, NULL ) )
				{
					rc = rs->sr_err = LDAP_NO_SUCH_OBJECT;
					send_ldap_result( op, rs );

				} else {
					rc = rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
					send_ldap_error( op, rs, rc, "no static subordinate entries allowed for dynamicObject" );
				}
			}

			be_entry_release_r( op, e );
			if ( rc != LDAP_SUCCESS ) {
				return rc;
			}
		}
		op->o_bd->bd_info = bi;
	}

	/* handle dynamic object operational attr(s) */
	if ( is_dynamicObject ) {
		time_t		ttl, expire;
		char		ttlbuf[STRLENOF("31557600") + 1];
		char		tsbuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
		struct berval	bv;

		if ( !be_isroot_dn( op->o_bd, &op->o_req_ndn ) ) {
			ldap_pvt_thread_mutex_lock( &di->di_mutex );
			rs->sr_err = ( di->di_max_dynamicObjects && 
				di->di_num_dynamicObjects >= di->di_max_dynamicObjects );
			ldap_pvt_thread_mutex_unlock( &di->di_mutex );
			if ( rs->sr_err ) {
				op->o_bd->bd_info = (BackendInfo *)on->on_info;
				send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
					"too many dynamicObjects in context" );
				return rs->sr_err;
			}
		}

		ttl = DDS_DEFAULT_TTL( di );

		/* assert because should be checked at configure */
		assert( ttl <= DDS_RF2589_MAX_TTL );

		bv.bv_val = ttlbuf;
		bv.bv_len = snprintf( ttlbuf, sizeof( ttlbuf ), "%ld", ttl );
		assert( bv.bv_len < sizeof( ttlbuf ) );

		/* FIXME: apparently, values in op->ora_e are malloc'ed
		 * on the thread's slab; works fine by chance,
		 * only because the attribute doesn't exist yet. */
		assert( attr_find( op->ora_e->e_attrs, slap_schema.si_ad_entryTtl ) == NULL );
		attr_merge_one( op->ora_e, slap_schema.si_ad_entryTtl, &bv, &bv );

		expire = slap_get_time() + ttl;
		bv.bv_val = tsbuf;
		bv.bv_len = sizeof( tsbuf );
		slap_timestamp( &expire, &bv );
		assert( attr_find( op->ora_e->e_attrs, ad_entryExpireTimestamp ) == NULL );
		attr_merge_one( op->ora_e, ad_entryExpireTimestamp, &bv, &bv );

		/* if required, install counter callback */
		if ( di->di_max_dynamicObjects > 0) {
			slap_callback	*sc;

			sc = op->o_tmpalloc( sizeof( slap_callback ), op->o_tmpmemctx );
			sc->sc_cleanup = dds_freeit_cb;
			sc->sc_response = dds_counter_cb;
			sc->sc_private = di;
			sc->sc_next = op->o_callback;

			op->o_callback = sc;
		}
	}

	return SLAP_CB_CONTINUE;
}

static int
dds_op_delete( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	dds_info_t	*di = on->on_bi.bi_private;

	/* if required, install counter callback */
	if ( !DDS_OFF( di ) && di->di_max_dynamicObjects > 0 ) {
		Entry		*e = NULL;
		BackendInfo	*bi = op->o_bd->bd_info;

		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		rs->sr_err = be_entry_get_rw( op, &op->o_req_ndn,
			slap_schema.si_oc_dynamicObject, NULL, 0, &e );

		/* FIXME: couldn't the entry be added before deletion? */
		if ( rs->sr_err == LDAP_SUCCESS && e != NULL ) {
			slap_callback	*sc;
	
			be_entry_release_r( op, e );
			e = NULL;
	
			sc = op->o_tmpalloc( sizeof( slap_callback ), op->o_tmpmemctx );
			sc->sc_cleanup = dds_freeit_cb;
			sc->sc_response = dds_counter_cb;
			sc->sc_private = di;
			sc->sc_next = op->o_callback;
	
			op->o_callback = sc;
		}
		op->o_bd->bd_info = bi;
	}

	return SLAP_CB_CONTINUE;
}

static int
dds_op_modify( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	dds_info_t	*di = (dds_info_t *)on->on_bi.bi_private;
	Modifications	*mod;
	Entry		*e = NULL;
	BackendInfo	*bi = op->o_bd->bd_info;
	int		was_dynamicObject = 0,
			is_dynamicObject = 0;
	struct berval	bv_entryTtl = BER_BVNULL;
	time_t		entryTtl = 0;
	char		textbuf[ SLAP_TEXT_BUFLEN ];

	if ( DDS_OFF( di ) ) {
		return SLAP_CB_CONTINUE;
	}

	/* bv_entryTtl stores the string representation of the entryTtl
	 * across modifies for consistency checks of the final value;
	 * the bv_val points to a static buffer; the bv_len is zero when
	 * the attribute is deleted.
	 * entryTtl stores the integer representation of the entryTtl;
	 * its value is -1 when the attribute is deleted; it is 0 only
	 * if no modifications of the entryTtl occurred, as an entryTtl
	 * of 0 is invalid. */
	bv_entryTtl.bv_val = textbuf;

	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	rs->sr_err = be_entry_get_rw( op, &op->o_req_ndn,
		slap_schema.si_oc_dynamicObject, slap_schema.si_ad_entryTtl, 0, &e );
	if ( rs->sr_err == LDAP_SUCCESS && e != NULL ) {
		Attribute	*a = attr_find( e->e_attrs, slap_schema.si_ad_entryTtl );

		/* the value of the entryTtl is saved for later checks */
		if ( a != NULL ) {
			unsigned long	ttl;
			int		rc;

			bv_entryTtl.bv_len = a->a_nvals[ 0 ].bv_len;
			AC_MEMCPY( bv_entryTtl.bv_val, a->a_nvals[ 0 ].bv_val, bv_entryTtl.bv_len );
			bv_entryTtl.bv_val[ bv_entryTtl.bv_len ] = '\0';
			rc = lutil_atoul( &ttl, bv_entryTtl.bv_val );
			assert( rc == 0 );
			entryTtl = (time_t)ttl;
		}

		be_entry_release_r( op, e );
		e = NULL;
		was_dynamicObject = is_dynamicObject = 1;
	}
	op->o_bd->bd_info = bi;

	rs->sr_err = LDAP_SUCCESS;
	for ( mod = op->orm_modlist; mod; mod = mod->sml_next ) {
		if ( mod->sml_desc == slap_schema.si_ad_objectClass ) {
			int		i;
			ObjectClass	*oc;

			switch ( mod->sml_op ) {
			case LDAP_MOD_DELETE:
				if ( mod->sml_values == NULL ) {
					is_dynamicObject = 0;
					break;
				}
	
				for ( i = 0; !BER_BVISNULL( &mod->sml_values[ i ] ); i++ ) {
					oc = oc_bvfind( &mod->sml_values[ i ] );
					if ( oc == slap_schema.si_oc_dynamicObject ) {
						is_dynamicObject = 0;
						break;
					}
				}
	
				break;
	
			case LDAP_MOD_REPLACE:
				if ( mod->sml_values == NULL ) {
					is_dynamicObject = 0;
					break;
				}
				/* fallthru */
	
			case LDAP_MOD_ADD:
				for ( i = 0; !BER_BVISNULL( &mod->sml_values[ i ] ); i++ ) {
					oc = oc_bvfind( &mod->sml_values[ i ] );
					if ( oc == slap_schema.si_oc_dynamicObject ) {
						is_dynamicObject = 1;
						break;
					}
				}
				break;
			}

		} else if ( mod->sml_desc == slap_schema.si_ad_entryTtl ) {
			unsigned long	uttl;
			time_t		ttl;
			int		rc;

			switch ( mod->sml_op ) {
			case LDAP_MOD_DELETE:
			case SLAP_MOD_SOFTDEL: /* FIXME? */
				if ( mod->sml_values != NULL ) {
					if ( BER_BVISEMPTY( &bv_entryTtl ) 
						|| !bvmatch( &bv_entryTtl, &mod->sml_values[ 0 ] ) )
					{
						rs->sr_err = backend_attribute( op, NULL, &op->o_req_ndn, 
							slap_schema.si_ad_entry, NULL, ACL_DISCLOSE );
						if ( rs->sr_err == LDAP_INSUFFICIENT_ACCESS ) {
							rs->sr_err = LDAP_NO_SUCH_OBJECT;

						} else {
							rs->sr_err = LDAP_NO_SUCH_ATTRIBUTE;
						}
						goto done;
					}
				}
				bv_entryTtl.bv_len = 0;
				entryTtl = -1;
				break;

			case LDAP_MOD_REPLACE:
				bv_entryTtl.bv_len = 0;
				entryTtl = -1;
				/* fallthru */

			case LDAP_MOD_ADD:
			case SLAP_MOD_SOFTADD: /* FIXME? */
			case SLAP_MOD_ADD_IF_NOT_PRESENT: /* FIXME? */
				assert( mod->sml_values != NULL );
				assert( BER_BVISNULL( &mod->sml_values[ 1 ] ) );

				if ( !BER_BVISEMPTY( &bv_entryTtl ) ) {
					rs->sr_err = backend_attribute( op, NULL, &op->o_req_ndn, 
						slap_schema.si_ad_entry, NULL, ACL_DISCLOSE );
					if ( rs->sr_err == LDAP_INSUFFICIENT_ACCESS ) {
						rs->sr_err = LDAP_NO_SUCH_OBJECT;

					} else {
						rs->sr_text = "attribute 'entryTtl' cannot have multiple values";
						rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
					}
					goto done;
				}

				rc = lutil_atoul( &uttl, mod->sml_values[ 0 ].bv_val );
				ttl = (time_t)uttl;
				assert( rc == 0 );
				if ( ttl > DDS_RF2589_MAX_TTL ) {
					rs->sr_err = LDAP_PROTOCOL_ERROR;
					rs->sr_text = "invalid time-to-live for dynamicObject";
					goto done;
				}

				if ( ttl <= 0 || ttl > di->di_max_ttl ) {
					/* FIXME: I don't understand if this has to be an error,
					 * or an indication that the requested Ttl has been
					 * shortened to di->di_max_ttl >= 1 day */
					rs->sr_err = LDAP_SIZELIMIT_EXCEEDED;
					rs->sr_text = "time-to-live for dynamicObject exceeds administrative limit";
					goto done;
				}

				entryTtl = ttl;
				bv_entryTtl.bv_len = mod->sml_values[ 0 ].bv_len;
				AC_MEMCPY( bv_entryTtl.bv_val, mod->sml_values[ 0 ].bv_val, bv_entryTtl.bv_len );
				bv_entryTtl.bv_val[ bv_entryTtl.bv_len ] = '\0';
				break;

			case LDAP_MOD_INCREMENT:
				if ( BER_BVISEMPTY( &bv_entryTtl ) ) {
					rs->sr_err = backend_attribute( op, NULL, &op->o_req_ndn, 
						slap_schema.si_ad_entry, NULL, ACL_DISCLOSE );
					if ( rs->sr_err == LDAP_INSUFFICIENT_ACCESS ) {
						rs->sr_err = LDAP_NO_SUCH_OBJECT;

					} else {
						rs->sr_err = LDAP_NO_SUCH_ATTRIBUTE;
						rs->sr_text = "modify/increment: entryTtl: no such attribute";
					}
					goto done;
				}

				entryTtl++;
				if ( entryTtl > DDS_RF2589_MAX_TTL ) {
					rs->sr_err = LDAP_PROTOCOL_ERROR;
					rs->sr_text = "invalid time-to-live for dynamicObject";

				} else if ( entryTtl <= 0 || entryTtl > di->di_max_ttl ) {
					/* FIXME: I don't understand if this has to be an error,
					 * or an indication that the requested Ttl has been
					 * shortened to di->di_max_ttl >= 1 day */
					rs->sr_err = LDAP_SIZELIMIT_EXCEEDED;
					rs->sr_text = "time-to-live for dynamicObject exceeds administrative limit";
				}

				if ( rs->sr_err != LDAP_SUCCESS ) {
					rc = backend_attribute( op, NULL, &op->o_req_ndn, 
						slap_schema.si_ad_entry, NULL, ACL_DISCLOSE );
					if ( rc == LDAP_INSUFFICIENT_ACCESS ) {
						rs->sr_text = NULL;
						rs->sr_err = LDAP_NO_SUCH_OBJECT;

					}
					goto done;
				}

				bv_entryTtl.bv_len = snprintf( textbuf, sizeof( textbuf ), "%ld", entryTtl );
				break;

			default:
				assert( 0 );
				break;
			}

		} else if ( mod->sml_desc == ad_entryExpireTimestamp ) {
			/* should have been trapped earlier */
			assert( mod->sml_flags & SLAP_MOD_INTERNAL );
		}
	}

done:;
	if ( rs->sr_err == LDAP_SUCCESS ) {
		int	rc;

		/* FIXME: this could be allowed when the Relax control is used...
		 * in that case:
		 *
		 * TODO
		 * 
		 *	static => dynamic:
		 *		entryTtl must be provided; add
		 *		entryExpireTimestamp accordingly
		 *
		 *	dynamic => static:
		 *		entryTtl must be removed; remove
		 *		entryTimestamp accordingly
		 *
		 * ... but we need to make sure that there are no subordinate 
		 * issues...
		 */
		rc = is_dynamicObject - was_dynamicObject;
		if ( rc ) {
#if 0 /* fix subordinate issues first */
			if ( get_relax( op ) ) {
				switch ( rc ) {
				case -1:
					/* need to delete entryTtl to have a consistent entry */
					if ( entryTtl != -1 ) {
						rs->sr_text = "objectClass modification from dynamicObject to static entry requires entryTtl deletion";
						rs->sr_err = LDAP_OBJECT_CLASS_VIOLATION;
					}
					break;

				case 1:
					/* need to add entryTtl to have a consistent entry */
					if ( entryTtl <= 0 ) {
						rs->sr_text = "objectClass modification from static entry to dynamicObject requires entryTtl addition";
						rs->sr_err = LDAP_OBJECT_CLASS_VIOLATION;
					}
					break;
				}

			} else
#endif
			{
				switch ( rc ) {
				case -1:
					rs->sr_text = "objectClass modification cannot turn dynamicObject into static entry";
					break;

				case 1:
					rs->sr_text = "objectClass modification cannot turn static entry into dynamicObject";
					break;
				}
				rs->sr_err = LDAP_OBJECT_CLASS_VIOLATION;
			}

			if ( rc != LDAP_SUCCESS ) {
				rc = backend_attribute( op, NULL, &op->o_req_ndn, 
					slap_schema.si_ad_entry, NULL, ACL_DISCLOSE );
				if ( rc == LDAP_INSUFFICIENT_ACCESS ) {
					rs->sr_text = NULL;
					rs->sr_err = LDAP_NO_SUCH_OBJECT;
				}
			}
		}
	}

	if ( rs->sr_err == LDAP_SUCCESS && entryTtl != 0 ) {
		Modifications	*tmpmod = NULL, **modp;

		for ( modp = &op->orm_modlist; *modp; modp = &(*modp)->sml_next )
			;
	
		tmpmod = ch_calloc( 1, sizeof( Modifications ) );
		tmpmod->sml_flags = SLAP_MOD_INTERNAL;
		tmpmod->sml_type = ad_entryExpireTimestamp->ad_cname;
		tmpmod->sml_desc = ad_entryExpireTimestamp;

		*modp = tmpmod;

		if ( entryTtl == -1 ) {
			/* delete entryExpireTimestamp */
			tmpmod->sml_op = LDAP_MOD_DELETE;

		} else {
			time_t		expire;
			char		tsbuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
			struct berval	bv;

			/* keep entryExpireTimestamp consistent
			 * with entryTtl */
			expire = slap_get_time() + entryTtl;
			bv.bv_val = tsbuf;
			bv.bv_len = sizeof( tsbuf );
			slap_timestamp( &expire, &bv );

			tmpmod->sml_op = LDAP_MOD_REPLACE;
			value_add_one( &tmpmod->sml_values, &bv );
			value_add_one( &tmpmod->sml_nvalues, &bv );
			tmpmod->sml_numvals = 1;
		}
	}

	if ( rs->sr_err ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		send_ldap_result( op, rs );
		return rs->sr_err;
	}

	return SLAP_CB_CONTINUE;
}

static int
dds_op_rename( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	dds_info_t	*di = on->on_bi.bi_private;

	if ( DDS_OFF( di ) ) {
		return SLAP_CB_CONTINUE;
	}

	/* we don't allow dynamicObjects to have static subordinates */
	if ( op->orr_nnewSup != NULL ) {
		Entry		*e = NULL;
		BackendInfo	*bi = op->o_bd->bd_info;
		int		is_dynamicObject = 0,
				rc;

		rs->sr_err = LDAP_SUCCESS;

		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		rc = be_entry_get_rw( op, &op->o_req_ndn,
			slap_schema.si_oc_dynamicObject, NULL, 0, &e );
		if ( rc == LDAP_SUCCESS && e != NULL ) {
			be_entry_release_r( op, e );
			e = NULL;
			is_dynamicObject = 1;
		}

		rc = be_entry_get_rw( op, op->orr_nnewSup,
			slap_schema.si_oc_dynamicObject, NULL, 0, &e );
		if ( rc == LDAP_SUCCESS && e != NULL ) {
			if ( !is_dynamicObject ) {
				/* return referral only if "disclose"
				 * is granted on the object */
				if ( ! access_allowed( op, e,
						slap_schema.si_ad_entry,
						NULL, ACL_DISCLOSE, NULL ) )
				{
					rs->sr_err = LDAP_NO_SUCH_OBJECT;
					send_ldap_result( op, rs );

				} else {
					send_ldap_error( op, rs, LDAP_CONSTRAINT_VIOLATION,
						"static entry cannot have dynamicObject as newSuperior" );
				}
			}
			be_entry_release_r( op, e );
		}
		op->o_bd->bd_info = bi;
		if ( rs->sr_err != LDAP_SUCCESS ) {
			return rs->sr_err;
		}
	}

	return SLAP_CB_CONTINUE;
}

static int
slap_parse_refresh(
	struct berval	*in,
	struct berval	*ndn,
	time_t		*ttl,
	const char	**text,
	void		*ctx )
{
	int			rc = LDAP_SUCCESS;
	ber_tag_t		tag;
	ber_len_t		len = -1;
	BerElementBuffer	berbuf;
	BerElement		*ber = (BerElement *)&berbuf;
	struct berval		reqdata = BER_BVNULL;
	int			tmp;

	*text = NULL;

	if ( ndn ) {
		BER_BVZERO( ndn );
	}

	if ( in == NULL || in->bv_len == 0 ) {
		*text = "empty request data field in refresh exop";
		return LDAP_PROTOCOL_ERROR;
	}

	ber_dupbv_x( &reqdata, in, ctx );

	/* ber_init2 uses reqdata directly, doesn't allocate new buffers */
	ber_init2( ber, &reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );

	if ( tag == LBER_ERROR ) {
		Log0( LDAP_DEBUG_TRACE, LDAP_LEVEL_ERR,
			"slap_parse_refresh: decoding error.\n" );
		goto decoding_error;
	}

	tag = ber_peek_tag( ber, &len );
	if ( tag != LDAP_TAG_EXOP_REFRESH_REQ_DN ) {
		Log0( LDAP_DEBUG_TRACE, LDAP_LEVEL_ERR,
			"slap_parse_refresh: decoding error.\n" );
		goto decoding_error;
	}

	if ( ndn ) {
		struct berval	dn;

		tag = ber_scanf( ber, "m", &dn );
		if ( tag == LBER_ERROR ) {
			Log0( LDAP_DEBUG_TRACE, LDAP_LEVEL_ERR,
				"slap_parse_refresh: DN parse failed.\n" );
			goto decoding_error;
		}

		rc = dnNormalize( 0, NULL, NULL, &dn, ndn, ctx );
		if ( rc != LDAP_SUCCESS ) {
			*text = "invalid DN in refresh exop request data";
			goto done;
		}

	} else {
		tag = ber_scanf( ber, "x" /* "m" */ );
		if ( tag == LBER_DEFAULT ) {
			goto decoding_error;
		}
	}

	tag = ber_peek_tag( ber, &len );

	if ( tag != LDAP_TAG_EXOP_REFRESH_REQ_TTL ) {
		Log0( LDAP_DEBUG_TRACE, LDAP_LEVEL_ERR,
			"slap_parse_refresh: decoding error.\n" );
		goto decoding_error;
	}

	tag = ber_scanf( ber, "i", &tmp );
	if ( tag == LBER_ERROR ) {
		Log0( LDAP_DEBUG_TRACE, LDAP_LEVEL_ERR,
			"slap_parse_refresh: TTL parse failed.\n" );
		goto decoding_error;
	}

	if ( ttl ) {
		*ttl = tmp;
	}

	tag = ber_peek_tag( ber, &len );

	if ( tag != LBER_DEFAULT || len != 0 ) {
decoding_error:;
		Log1( LDAP_DEBUG_TRACE, LDAP_LEVEL_ERR,
			"slap_parse_refresh: decoding error, len=%ld\n",
			(long)len );
		rc = LDAP_PROTOCOL_ERROR;
		*text = "data decoding error";

done:;
		if ( ndn && !BER_BVISNULL( ndn ) ) {
			slap_sl_free( ndn->bv_val, ctx );
			BER_BVZERO( ndn );
		}
	}

	if ( !BER_BVISNULL( &reqdata ) ) {
		ber_memfree_x( reqdata.bv_val, ctx );
	}

	return rc;
}

static int
dds_op_extended( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	dds_info_t	*di = on->on_bi.bi_private;

	if ( DDS_OFF( di ) ) {
		return SLAP_CB_CONTINUE;
	}

	if ( bvmatch( &op->ore_reqoid, &slap_EXOP_REFRESH ) ) {
		Entry		*e = NULL;
		time_t		ttl;
		BackendDB	db = *op->o_bd;
		SlapReply	rs2 = { REP_RESULT };
		Operation	op2 = *op;
		slap_callback	sc = { 0 };
		Modifications	ttlmod = { { 0 } };
		struct berval	ttlvalues[ 2 ];
		char		ttlbuf[STRLENOF("31557600") + 1];

		rs->sr_err = slap_parse_refresh( op->ore_reqdata, NULL, &ttl,
			&rs->sr_text, NULL );
		assert( rs->sr_err == LDAP_SUCCESS );

		if ( ttl <= 0 || ttl > DDS_RF2589_MAX_TTL ) {
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			rs->sr_text = "invalid time-to-live for dynamicObject";
			return rs->sr_err;
		}

		if ( ttl > di->di_max_ttl ) {
			/* FIXME: I don't understand if this has to be an error,
			 * or an indication that the requested Ttl has been
			 * shortened to di->di_max_ttl >= 1 day */
			rs->sr_err = LDAP_SIZELIMIT_EXCEEDED;
			rs->sr_text = "time-to-live for dynamicObject exceeds limit";
			return rs->sr_err;
		}

		if ( di->di_min_ttl && ttl < di->di_min_ttl ) {
			ttl = di->di_min_ttl;
		}

		/* This does not apply to multi-master case */
		if ( !( !SLAP_SINGLE_SHADOW( op->o_bd ) || be_isupdate( op ) ) ) {
			/* we SHOULD return a referral in this case */
			BerVarray defref = op->o_bd->be_update_refs
				? op->o_bd->be_update_refs : default_referral; 

			if ( defref != NULL ) {
				rs->sr_ref = referral_rewrite( op->o_bd->be_update_refs,
					NULL, NULL, LDAP_SCOPE_DEFAULT );
				if ( rs->sr_ref ) {
					rs->sr_flags |= REP_REF_MUSTBEFREED;
				} else {
					rs->sr_ref = defref;
				}
				rs->sr_err = LDAP_REFERRAL;

			} else {
				rs->sr_text = "shadow context; no update referral";
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			}

			return rs->sr_err;
		}

		assert( !BER_BVISNULL( &op->o_req_ndn ) );



		/* check if exists but not dynamicObject */
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		rs->sr_err = be_entry_get_rw( op, &op->o_req_ndn,
			slap_schema.si_oc_dynamicObject, NULL, 0, &e );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			rs->sr_err = be_entry_get_rw( op, &op->o_req_ndn,
				NULL, NULL, 0, &e );
			if ( rs->sr_err == LDAP_SUCCESS && e != NULL ) {
				/* return referral only if "disclose"
				 * is granted on the object */
				if ( ! access_allowed( op, e,
						slap_schema.si_ad_entry,
						NULL, ACL_DISCLOSE, NULL ) )
				{
					rs->sr_err = LDAP_NO_SUCH_OBJECT;

				} else {
					rs->sr_err = LDAP_OBJECT_CLASS_VIOLATION;
					rs->sr_text = "refresh operation only applies to dynamic objects";
				}
				be_entry_release_r( op, e );

			} else {
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
			}
			return rs->sr_err;

		} else if ( e != NULL ) {
			be_entry_release_r( op, e );
		}

		/* we require manage privileges on the entryTtl,
		 * and fake a Relax control */
		op2.o_tag = LDAP_REQ_MODIFY;
		op2.o_bd = &db;
		db.bd_info = (BackendInfo *)on->on_info;
		op2.o_callback = &sc;
		sc.sc_response = slap_null_cb;
		op2.o_relax = SLAP_CONTROL_CRITICAL;
		op2.orm_modlist = &ttlmod;

		ttlmod.sml_op = LDAP_MOD_REPLACE;
		ttlmod.sml_flags = SLAP_MOD_MANAGING;
		ttlmod.sml_desc = slap_schema.si_ad_entryTtl;
		ttlmod.sml_values = ttlvalues;
		ttlmod.sml_numvals = 1;
		ttlvalues[ 0 ].bv_val = ttlbuf;
		ttlvalues[ 0 ].bv_len = snprintf( ttlbuf, sizeof( ttlbuf ), "%ld", ttl );
		BER_BVZERO( &ttlvalues[ 1 ] );

		/* the entryExpireTimestamp is added by modify */
		rs->sr_err = op2.o_bd->be_modify( &op2, &rs2 );

		if ( ttlmod.sml_next != NULL ) {
			slap_mods_free( ttlmod.sml_next, 1 );
		}

		if ( rs->sr_err == LDAP_SUCCESS ) {
			int			rc;
			BerElementBuffer	berbuf;
			BerElement		*ber = (BerElement *)&berbuf;

			ber_init_w_nullc( ber, LBER_USE_DER );

			rc = ber_printf( ber, "{tiN}", LDAP_TAG_EXOP_REFRESH_RES_TTL, (int)ttl );

			if ( rc < 0 ) {
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "internal error";

			} else {
				(void)ber_flatten( ber, &rs->sr_rspdata );
				rs->sr_rspoid = ch_strdup( slap_EXOP_REFRESH.bv_val );

				Log3( LDAP_DEBUG_TRACE, LDAP_LEVEL_INFO,
					"%s REFRESH dn=\"%s\" TTL=%ld\n",
					op->o_log_prefix, op->o_req_ndn.bv_val, ttl );
			}

			ber_free_buf( ber );
		}

		return rs->sr_err;
	}

	return SLAP_CB_CONTINUE;
}

enum {
	DDS_STATE = 1,
	DDS_MAXTTL,
	DDS_MINTTL,
	DDS_DEFAULTTTL,
	DDS_INTERVAL,
	DDS_TOLERANCE,
	DDS_MAXDYNAMICOBJS,

	DDS_LAST
};

static ConfigDriver dds_cfgen;
#if 0
static ConfigLDAPadd dds_ldadd;
static ConfigCfAdd dds_cfadd;
#endif

static ConfigTable dds_cfg[] = {
	{ "dds-state", "on|off",
		2, 2, 0, ARG_MAGIC|ARG_ON_OFF|DDS_STATE, dds_cfgen,
		"( OLcfgOvAt:9.1 NAME 'olcDDSstate' "
			"DESC 'RFC2589 Dynamic directory services state' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )", NULL, NULL },
	{ "dds-max-ttl", "ttl",
		2, 2, 0, ARG_MAGIC|DDS_MAXTTL, dds_cfgen,
		"( OLcfgOvAt:9.2 NAME 'olcDDSmaxTtl' "
			"DESC 'RFC2589 Dynamic directory services max TTL' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )", NULL, NULL },
	{ "dds-min-ttl", "ttl",
		2, 2, 0, ARG_MAGIC|DDS_MINTTL, dds_cfgen,
		"( OLcfgOvAt:9.3 NAME 'olcDDSminTtl' "
			"DESC 'RFC2589 Dynamic directory services min TTL' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )", NULL, NULL },
	{ "dds-default-ttl", "ttl",
		2, 2, 0, ARG_MAGIC|DDS_DEFAULTTTL, dds_cfgen,
		"( OLcfgOvAt:9.4 NAME 'olcDDSdefaultTtl' "
			"DESC 'RFC2589 Dynamic directory services default TTL' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )", NULL, NULL },
	{ "dds-interval", "interval",
		2, 2, 0, ARG_MAGIC|DDS_INTERVAL, dds_cfgen,
		"( OLcfgOvAt:9.5 NAME 'olcDDSinterval' "
			"DESC 'RFC2589 Dynamic directory services expiration "
				"task run interval' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )", NULL, NULL },
	{ "dds-tolerance", "ttl",
		2, 2, 0, ARG_MAGIC|DDS_TOLERANCE, dds_cfgen,
		"( OLcfgOvAt:9.6 NAME 'olcDDStolerance' "
			"DESC 'RFC2589 Dynamic directory services additional "
				"TTL in expiration scheduling' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )", NULL, NULL },
	{ "dds-max-dynamicObjects", "num",
		2, 2, 0, ARG_MAGIC|ARG_INT|DDS_MAXDYNAMICOBJS, dds_cfgen,
		"( OLcfgOvAt:9.7 NAME 'olcDDSmaxDynamicObjects' "
			"DESC 'RFC2589 Dynamic directory services max number of dynamic objects' "
			"SYNTAX OMsInteger "
			"SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs dds_ocs[] = {
	{ "( OLcfgOvOc:9.1 "
		"NAME 'olcDDSConfig' "
		"DESC 'RFC2589 Dynamic directory services configuration' "
		"SUP olcOverlayConfig "
		"MAY ( "
			"olcDDSstate "
			"$ olcDDSmaxTtl "
			"$ olcDDSminTtl "
			"$ olcDDSdefaultTtl "
			"$ olcDDSinterval "
			"$ olcDDStolerance "
			"$ olcDDSmaxDynamicObjects "
		" ) "
		")", Cft_Overlay, dds_cfg, NULL, NULL /* dds_cfadd */ },
	{ NULL, 0, NULL }
};

#if 0
static int
dds_ldadd( CfEntryInfo *p, Entry *e, ConfigArgs *ca )
{
	return LDAP_SUCCESS;
}

static int
dds_cfadd( Operation *op, SlapReply *rs, Entry *p, ConfigArgs *ca )
{
	return 0;
}
#endif

static int
dds_cfgen( ConfigArgs *c )
{
	slap_overinst	*on = (slap_overinst *)c->bi;
	dds_info_t	*di = on->on_bi.bi_private;
	int		rc = 0;
	unsigned long	t;


	if ( c->op == SLAP_CONFIG_EMIT ) {
		char		buf[ SLAP_TEXT_BUFLEN ];
		struct berval	bv;

		switch( c->type ) {
		case DDS_STATE: 
			c->value_int = !DDS_OFF( di );
			break;

		case DDS_MAXTTL: 
			lutil_unparse_time( buf, sizeof( buf ), di->di_max_ttl );
			ber_str2bv( buf, 0, 0, &bv );
			value_add_one( &c->rvalue_vals, &bv );
			break;

		case DDS_MINTTL:
			if ( di->di_min_ttl ) {
				lutil_unparse_time( buf, sizeof( buf ), di->di_min_ttl );
				ber_str2bv( buf, 0, 0, &bv );
				value_add_one( &c->rvalue_vals, &bv );

			} else {
				rc = 1;
			}
			break;

		case DDS_DEFAULTTTL:
			if ( di->di_default_ttl ) {
				lutil_unparse_time( buf, sizeof( buf ), di->di_default_ttl );
				ber_str2bv( buf, 0, 0, &bv );
				value_add_one( &c->rvalue_vals, &bv );

			} else {
				rc = 1;
			}
			break;

		case DDS_INTERVAL:
			if ( di->di_interval ) {
				lutil_unparse_time( buf, sizeof( buf ), di->di_interval );
				ber_str2bv( buf, 0, 0, &bv );
				value_add_one( &c->rvalue_vals, &bv );

			} else {
				rc = 1;
			}
			break;

		case DDS_TOLERANCE:
			if ( di->di_tolerance ) {
				lutil_unparse_time( buf, sizeof( buf ), di->di_tolerance );
				ber_str2bv( buf, 0, 0, &bv );
				value_add_one( &c->rvalue_vals, &bv );

			} else {
				rc = 1;
			}
			break;

		case DDS_MAXDYNAMICOBJS:
			if ( di->di_max_dynamicObjects > 0 ) {
				c->value_int = di->di_max_dynamicObjects;

			} else {
				rc = 1;
			}
			break;

		default:
			rc = 1;
			break;
		}

		return rc;

	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch( c->type ) {
		case DDS_STATE:
			di->di_flags &= ~DDS_FOFF;
			break;

		case DDS_MAXTTL:
			di->di_min_ttl = DDS_RF2589_DEFAULT_TTL;
			break;

		case DDS_MINTTL:
			di->di_min_ttl = 0;
			break;

		case DDS_DEFAULTTTL:
			di->di_default_ttl = 0;
			break;

		case DDS_INTERVAL:
			di->di_interval = 0;
			break;

		case DDS_TOLERANCE:
			di->di_tolerance = 0;
			break;

		case DDS_MAXDYNAMICOBJS:
			di->di_max_dynamicObjects = 0;
			break;

		default:
			rc = 1;
			break;
		}

		return rc;
	}

	switch ( c->type ) {
	case DDS_STATE:
		if ( c->value_int ) {
			di->di_flags &= ~DDS_FOFF;

		} else {
			di->di_flags |= DDS_FOFF;
		}
		break;

	case DDS_MAXTTL:
		if ( lutil_parse_time( c->argv[ 1 ], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"DDS unable to parse dds-max-ttl \"%s\"",
				c->argv[ 1 ] );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			return 1;
		}

		if ( t < DDS_RF2589_DEFAULT_TTL || t > DDS_RF2589_MAX_TTL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"DDS invalid dds-max-ttl=%lu; must be between %d and %d",
				t, DDS_RF2589_DEFAULT_TTL, DDS_RF2589_MAX_TTL );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			return 1;
		}

		di->di_max_ttl = (time_t)t;
		break;

	case DDS_MINTTL:
		if ( lutil_parse_time( c->argv[ 1 ], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"DDS unable to parse dds-min-ttl \"%s\"",
				c->argv[ 1 ] );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			return 1;
		}

		if ( t > DDS_RF2589_MAX_TTL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"DDS invalid dds-min-ttl=%lu",
				t );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			return 1;
		}

		if ( t == 0 ) {
			di->di_min_ttl = DDS_RF2589_DEFAULT_TTL;

		} else {
			di->di_min_ttl = (time_t)t;
		}
		break;

	case DDS_DEFAULTTTL:
		if ( lutil_parse_time( c->argv[ 1 ], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"DDS unable to parse dds-default-ttl \"%s\"",
				c->argv[ 1 ] );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			return 1;
		}

		if ( t > DDS_RF2589_MAX_TTL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"DDS invalid dds-default-ttl=%lu",
				t );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			return 1;
		}

		if ( t == 0 ) {
			di->di_default_ttl = DDS_RF2589_DEFAULT_TTL;

		} else {
			di->di_default_ttl = (time_t)t;
		}
		break;

	case DDS_INTERVAL:
		if ( lutil_parse_time( c->argv[ 1 ], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"DDS unable to parse dds-interval \"%s\"",
				c->argv[ 1 ] );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			return 1;
		}

		if ( t <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"DDS invalid dds-interval=%lu",
				t );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			return 1;
		}

		if ( t < 60 ) {
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_NOTICE,
				"%s: dds-interval=%lu may be too small.\n",
				c->log, t );
		}

		di->di_interval = (time_t)t;
		if ( di->di_expire_task ) {
			ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
			if ( ldap_pvt_runqueue_isrunning( &slapd_rq, di->di_expire_task ) ) {
				ldap_pvt_runqueue_stoptask( &slapd_rq, di->di_expire_task );
			}
			di->di_expire_task->interval.tv_sec = DDS_INTERVAL( di );
			ldap_pvt_runqueue_resched( &slapd_rq, di->di_expire_task, 0 );
			ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
		}
		break;

	case DDS_TOLERANCE:
		if ( lutil_parse_time( c->argv[ 1 ], &t ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg),
				"DDS unable to parse dds-tolerance \"%s\"",
				c->argv[ 1 ] );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			return 1;
		}

		if ( t > DDS_RF2589_MAX_TTL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"DDS invalid dds-tolerance=%lu",
				t );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			return 1;
		}

		di->di_tolerance = (time_t)t;
		break;

	case DDS_MAXDYNAMICOBJS:
		if ( c->value_int < 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"DDS invalid dds-max-dynamicObjects=%d", c->value_int );
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"%s: %s.\n", c->log, c->cr_msg );
			return 1;
		}
		di->di_max_dynamicObjects = c->value_int;
		break;

	default:
		rc = 1;
		break;
	}

	return rc;
}

static int
dds_db_init(
	BackendDB	*be,
	ConfigReply	*cr)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	dds_info_t	*di;
	BackendInfo	*bi = on->on_info->oi_orig;

	if ( SLAP_ISGLOBALOVERLAY( be ) ) {
		Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"DDS cannot be used as global overlay.\n" );
		return 1;
	}

	/* check support for required functions */
	/* FIXME: some could be provided by other overlays in between */
	if ( bi->bi_op_add == NULL			/* object creation */
		|| bi->bi_op_delete == NULL		/* object deletion */
		|| bi->bi_op_modify == NULL		/* object refresh */
		|| bi->bi_op_search == NULL		/* object expiration */
		|| bi->bi_entry_get_rw == NULL )	/* object type/existence checking */
	{
		Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"DDS backend \"%s\" does not provide "
			"required functionality.\n",
			bi->bi_type );
		return 1;
	}

	di = (dds_info_t *)ch_calloc( 1, sizeof( dds_info_t ) );
	on->on_bi.bi_private = di;

	di->di_max_ttl = DDS_RF2589_DEFAULT_TTL;
	di->di_max_ttl = DDS_RF2589_DEFAULT_TTL;

	ldap_pvt_thread_mutex_init( &di->di_mutex );

	SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_DYNAMIC;

	return 0;
}

/* adds dynamicSubtrees to root DSE */
static int
dds_entry_info( void *arg, Entry *e )
{
	dds_info_t	*di = (dds_info_t *)arg;

	attr_merge( e, slap_schema.si_ad_dynamicSubtrees,
		di->di_suffix, di->di_nsuffix );

	return 0;
}

/* callback that counts the returned entries, since the search
 * does not get to the point in slap_send_search_entries where
 * the actual count occurs */
static int
dds_count_cb( Operation *op, SlapReply *rs )
{
	int	*nump = (int *)op->o_callback->sc_private;

	switch ( rs->sr_type ) {
	case REP_SEARCH:
		(*nump)++;
		break;

	case REP_SEARCHREF:
	case REP_RESULT:
		break;

	default:
		assert( 0 );
	}

	return 0;
}

/* count dynamic objects existing in the database at startup */
static int
dds_count( void *ctx, BackendDB *be )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	dds_info_t	*di = (dds_info_t *)on->on_bi.bi_private;
	
	Connection	conn = { 0 };
	OperationBuffer opbuf;
	Operation	*op;
	slap_callback	sc = { 0 };
	SlapReply	rs = { REP_RESULT };

	int		rc;
	char		*extra = "";

	connection_fake_init2( &conn, &opbuf, ctx, 0 );
	op = &opbuf.ob_op;

	op->o_tag = LDAP_REQ_SEARCH;
	memset( &op->oq_search, 0, sizeof( op->oq_search ) );

	op->o_bd = be;

	op->o_req_dn = op->o_bd->be_suffix[ 0 ];
	op->o_req_ndn = op->o_bd->be_nsuffix[ 0 ];

	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;

	op->ors_scope = LDAP_SCOPE_SUBTREE;
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_slimit = SLAP_NO_LIMIT;
	op->ors_attrs = slap_anlist_no_attrs;

	op->ors_filterstr.bv_len = STRLENOF( "(objectClass=" ")" )
		+ slap_schema.si_oc_dynamicObject->soc_cname.bv_len;
	op->ors_filterstr.bv_val = op->o_tmpalloc( op->ors_filterstr.bv_len + 1, op->o_tmpmemctx );
	snprintf( op->ors_filterstr.bv_val, op->ors_filterstr.bv_len + 1,
		"(objectClass=%s)",
		slap_schema.si_oc_dynamicObject->soc_cname.bv_val );

	op->ors_filter = str2filter_x( op, op->ors_filterstr.bv_val );
	if ( op->ors_filter == NULL ) {
		rs.sr_err = LDAP_OTHER;
		goto done_search;
	}
	
	op->o_callback = &sc;
	sc.sc_response = dds_count_cb;
	sc.sc_private = &di->di_num_dynamicObjects;
	di->di_num_dynamicObjects = 0;

	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	(void)op->o_bd->bd_info->bi_op_search( op, &rs );
	op->o_bd->bd_info = (BackendInfo *)on;

done_search:;
	op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
	filter_free_x( op, op->ors_filter, 1 );

	rc = rs.sr_err;
	switch ( rs.sr_err ) {
	case LDAP_SUCCESS:
		Log1( LDAP_DEBUG_STATS, LDAP_LEVEL_INFO,
			"DDS non-expired=%d\n",
			di->di_num_dynamicObjects );
		break;

	case LDAP_NO_SUCH_OBJECT:
		/* (ITS#5267) database not created yet? */
		rs.sr_err = LDAP_SUCCESS;
		extra = " (ignored)";
		/* fallthru */

	default:
		Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"DDS non-expired objects lookup failed err=%d%s\n",
			rc, extra );
		break;
	}

	return rs.sr_err;
}

static int
dds_db_open(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	dds_info_t	*di = on->on_bi.bi_private;
	int		rc = 0;
	void		*thrctx = ldap_pvt_thread_pool_context();

	if ( slapMode & SLAP_TOOL_MODE )
		return 0;

	if ( DDS_OFF( di ) ) {
		goto done;
	}

	if ( SLAP_SINGLE_SHADOW( be ) ) {
		Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
			"DDS incompatible with shadow database \"%s\".\n",
			be->be_suffix[ 0 ].bv_val );
		return 1;
	}

	if ( di->di_max_ttl == 0 ) {
		di->di_max_ttl = DDS_RF2589_DEFAULT_TTL;
	}

	if ( di->di_min_ttl == 0 ) {
		di->di_max_ttl = DDS_RF2589_DEFAULT_TTL;
	}

	di->di_suffix = be->be_suffix;
	di->di_nsuffix = be->be_nsuffix;

	/* count the dynamic objects first */
	rc = dds_count( thrctx, be );
	if ( rc != LDAP_SUCCESS ) {
		rc = 1;
		goto done;
	}

	/* ... if there are dynamic objects, delete those expired */
	if ( di->di_num_dynamicObjects > 0 ) {
		/* force deletion of expired entries... */
		be->bd_info = (BackendInfo *)on->on_info;
		rc = dds_expire( thrctx, di );
		be->bd_info = (BackendInfo *)on;
		if ( rc != LDAP_SUCCESS ) {
			rc = 1;
			goto done;
		}
	}

	/* start expire task */
	ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
	di->di_expire_task = ldap_pvt_runqueue_insert( &slapd_rq,
		DDS_INTERVAL( di ),
		dds_expire_fn, di, "dds_expire_fn",
		be->be_suffix[ 0 ].bv_val );
	ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

	/* register dinamicSubtrees root DSE info support */
	rc = entry_info_register( dds_entry_info, (void *)di );

done:;

	return rc;
}

static int
dds_db_close(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	dds_info_t	*di = on->on_bi.bi_private;

	/* stop expire task */
	if ( di && di->di_expire_task ) {
		ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
		if ( ldap_pvt_runqueue_isrunning( &slapd_rq, di->di_expire_task ) ) {
			ldap_pvt_runqueue_stoptask( &slapd_rq, di->di_expire_task );
		}
		ldap_pvt_runqueue_remove( &slapd_rq, di->di_expire_task );
		ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	}

	(void)entry_info_unregister( dds_entry_info, (void *)di );

	return 0;
}

static int
dds_db_destroy(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	dds_info_t	*di = on->on_bi.bi_private;

	if ( di != NULL ) {
		ldap_pvt_thread_mutex_destroy( &di->di_mutex );

		free( di );
	}

	return 0;
}

static int
slap_exop_refresh(
		Operation	*op,
		SlapReply	*rs )
{
	BackendDB		*bd = op->o_bd;

	rs->sr_err = slap_parse_refresh( op->ore_reqdata, &op->o_req_ndn, NULL,
		&rs->sr_text, op->o_tmpmemctx );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		return rs->sr_err;
	}

	Log2( LDAP_DEBUG_STATS, LDAP_LEVEL_INFO,
		"%s REFRESH dn=\"%s\"\n",
		op->o_log_prefix, op->o_req_ndn.bv_val );
	op->o_req_dn = op->o_req_ndn;

	op->o_bd = select_backend( &op->o_req_ndn, 0 );
	if ( op->o_bd == NULL ) {
		send_ldap_error( op, rs, LDAP_NO_SUCH_OBJECT,
			"no global superior knowledge" );
		goto done;
	}

	if ( !SLAP_DYNAMIC( op->o_bd ) ) {
		send_ldap_error( op, rs, LDAP_UNAVAILABLE_CRITICAL_EXTENSION,
			"backend does not support dynamic directory services" );
		goto done;
	}

	rs->sr_err = backend_check_restrictions( op, rs,
		(struct berval *)&slap_EXOP_REFRESH );
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
	op->o_bd = bd;

        return rs->sr_err;
}

static slap_overinst dds;

static int do_not_load_exop;
static int do_not_replace_exop;
static int do_not_load_schema;

#if SLAPD_OVER_DDS == SLAPD_MOD_DYNAMIC
static
#endif /* SLAPD_OVER_DDS == SLAPD_MOD_DYNAMIC */
int
dds_initialize()
{
	int		rc = 0;
	int		i, code;

	/* Make sure we don't exceed the bits reserved for userland */
	config_check_userland( DDS_LAST );

	if ( !do_not_load_schema ) {
		static struct {
			char			*desc;
			slap_mask_t		flags;
			AttributeDescription	**ad;
		}		s_at[] = {
			{ "( 1.3.6.1.4.1.4203.666.1.57 "
				"NAME ( 'entryExpireTimestamp' ) "
				"DESC 'RFC2589 OpenLDAP extension: expire time of a dynamic object, "
					"computed as now + entryTtl' "
				"EQUALITY generalizedTimeMatch "
				"ORDERING generalizedTimeOrderingMatch "
				"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
				"SINGLE-VALUE "
				"NO-USER-MODIFICATION "
				"USAGE dSAOperation )",
				SLAP_AT_HIDE,
				&ad_entryExpireTimestamp },
			{ NULL }
		};

		for ( i = 0; s_at[ i ].desc != NULL; i++ ) {
			code = register_at( s_at[ i ].desc, s_at[ i ].ad, 0 );
			if ( code ) {
				Debug( LDAP_DEBUG_ANY,
					"dds_initialize: register_at failed\n", 0, 0, 0 );
				return code;
			}
			(*s_at[ i ].ad)->ad_type->sat_flags |= SLAP_AT_HIDE;
		}
	}

	if ( !do_not_load_exop ) {
		rc = load_extop2( (struct berval *)&slap_EXOP_REFRESH,
			SLAP_EXOP_WRITES|SLAP_EXOP_HIDE, slap_exop_refresh,
			!do_not_replace_exop );
		if ( rc != LDAP_SUCCESS ) {
			Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"DDS unable to register refresh exop: %d.\n",
				rc );
			return rc;
		}
	}

	dds.on_bi.bi_type = "dds";

	dds.on_bi.bi_db_init = dds_db_init;
	dds.on_bi.bi_db_open = dds_db_open;
	dds.on_bi.bi_db_close = dds_db_close;
	dds.on_bi.bi_db_destroy = dds_db_destroy;

	dds.on_bi.bi_op_add = dds_op_add;
	dds.on_bi.bi_op_delete = dds_op_delete;
	dds.on_bi.bi_op_modify = dds_op_modify;
	dds.on_bi.bi_op_modrdn = dds_op_rename;
	dds.on_bi.bi_extended = dds_op_extended;

	dds.on_bi.bi_cf_ocs = dds_ocs;

	rc = config_register_schema( dds_cfg, dds_ocs );
	if ( rc ) {
		return rc;
	}

	return overlay_register( &dds );
}

#if SLAPD_OVER_DDS == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	int	i;

	for ( i = 0; i < argc; i++ ) {
		char	*arg = argv[ i ];
		int	no = 0;

		if ( strncasecmp( arg, "no-", STRLENOF( "no-" ) ) == 0 ) {
			arg += STRLENOF( "no-" );
			no = 1;
		}

		if ( strcasecmp( arg, "exop" ) == 0 ) {
			do_not_load_exop = no;

		} else if ( strcasecmp( arg, "replace" ) == 0 ) {
			do_not_replace_exop = no;

		} else if ( strcasecmp( arg, "schema" ) == 0 ) {
			do_not_load_schema = no;

		} else {
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR,
				"DDS unknown module arg[#%d]=\"%s\".\n",
				i, argv[ i ] );
			return 1;
		}
	}

	return dds_initialize();
}
#endif /* SLAPD_OVER_DDS == SLAPD_MOD_DYNAMIC */

#endif	/* defined(SLAPD_OVER_DDS) */
