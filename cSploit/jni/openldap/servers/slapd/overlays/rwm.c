/* rwm.c - rewrite/remap operations */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
 * Portions Copyright 2003 Pierangelo Masarati.
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

#ifdef SLAPD_OVER_RWM

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include "config.h"
#include "lutil.h"
#include "rwm.h"

typedef struct rwm_op_state {
	ber_tag_t r_tag;
	struct berval ro_dn;
	struct berval ro_ndn;
	struct berval r_dn;
	struct berval r_ndn;
	struct berval rx_dn;
	struct berval rx_ndn;
	AttributeName *mapped_attrs;
	OpRequest o_request;
} rwm_op_state;

typedef struct rwm_op_cb {
	slap_callback cb;
	rwm_op_state ros;
} rwm_op_cb;

static int
rwm_db_destroy( BackendDB *be, ConfigReply *cr );

static int
rwm_send_entry( Operation *op, SlapReply *rs );

static void
rwm_op_rollback( Operation *op, SlapReply *rs, rwm_op_state *ros )
{
	/* in case of successful extended operation cleanup
	 * gets called *after* (ITS#6632); this hack counts
	 * on others to cleanup our o_req_dn/o_req_ndn,
	 * while we cleanup theirs. */
	if ( ros->r_tag == LDAP_REQ_EXTENDED && rs->sr_err == LDAP_SUCCESS ) {
		if ( !BER_BVISNULL( &ros->rx_dn ) ) {
			ch_free( ros->rx_dn.bv_val );
		}
		if ( !BER_BVISNULL( &ros->rx_ndn ) ) {
			ch_free( ros->rx_ndn.bv_val );
		}

	} else {
		if ( !BER_BVISNULL( &ros->ro_dn ) ) {
			op->o_req_dn = ros->ro_dn;
		}
		if ( !BER_BVISNULL( &ros->ro_ndn ) ) {
			op->o_req_ndn = ros->ro_ndn;
		}

		if ( !BER_BVISNULL( &ros->r_dn )
			&& ros->r_dn.bv_val != ros->ro_dn.bv_val )
		{
			assert( ros->r_dn.bv_val != ros->r_ndn.bv_val );
			ch_free( ros->r_dn.bv_val );
		}

		if ( !BER_BVISNULL( &ros->r_ndn )
			&& ros->r_ndn.bv_val != ros->ro_ndn.bv_val )
		{
			ch_free( ros->r_ndn.bv_val );
		}
	}

	BER_BVZERO( &ros->r_dn );
	BER_BVZERO( &ros->r_ndn );
	BER_BVZERO( &ros->ro_dn );
	BER_BVZERO( &ros->ro_ndn );
	BER_BVZERO( &ros->rx_dn );
	BER_BVZERO( &ros->rx_ndn );

	switch( ros->r_tag ) {
	case LDAP_REQ_COMPARE:
		if ( op->orc_ava->aa_value.bv_val != ros->orc_ava->aa_value.bv_val )
			op->o_tmpfree( op->orc_ava->aa_value.bv_val, op->o_tmpmemctx );
		op->orc_ava = ros->orc_ava;
		break;
	case LDAP_REQ_MODIFY:
		slap_mods_free( op->orm_modlist, 1 );
		op->orm_modlist = ros->orm_modlist;
		break;
	case LDAP_REQ_MODRDN:
		if ( op->orr_newSup != ros->orr_newSup ) {
			if ( op->orr_newSup ) {
				ch_free( op->orr_newSup->bv_val );
				ch_free( op->orr_nnewSup->bv_val );
				op->o_tmpfree( op->orr_newSup, op->o_tmpmemctx );
				op->o_tmpfree( op->orr_nnewSup, op->o_tmpmemctx );
			}
			op->orr_newSup = ros->orr_newSup;
			op->orr_nnewSup = ros->orr_nnewSup;
		}
		if ( op->orr_newrdn.bv_val != ros->orr_newrdn.bv_val ) {
			ch_free( op->orr_newrdn.bv_val );
			ch_free( op->orr_nnewrdn.bv_val );
			op->orr_newrdn = ros->orr_newrdn;
			op->orr_nnewrdn = ros->orr_nnewrdn;
		}
		break;
	case LDAP_REQ_SEARCH:
		op->o_tmpfree( ros->mapped_attrs, op->o_tmpmemctx );
		filter_free_x( op, op->ors_filter, 1 );
		op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
		op->ors_attrs = ros->ors_attrs;
		op->ors_filter = ros->ors_filter;
		op->ors_filterstr = ros->ors_filterstr;
		break;
	case LDAP_REQ_EXTENDED:
		if ( op->ore_reqdata != ros->ore_reqdata ) {
			ber_bvfree( op->ore_reqdata );
			op->ore_reqdata = ros->ore_reqdata;
		}
		break;
	case LDAP_REQ_BIND:
		if ( rs->sr_err == LDAP_SUCCESS ) {
#if 0
			ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );
			/* too late, c_mutex released */
			Debug( LDAP_DEBUG_ANY, "*** DN: \"%s\" => \"%s\"\n",
				op->o_conn->c_ndn.bv_val,
				op->o_req_ndn.bv_val );
			ber_bvreplace( &op->o_conn->c_ndn,
				&op->o_req_ndn );
			ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );
#endif
		}
		break;
	default:	break;
	}
}

static int
rwm_op_cleanup( Operation *op, SlapReply *rs )
{
	slap_callback	*cb = op->o_callback;
	rwm_op_state *ros = cb->sc_private;

	if ( rs->sr_type == REP_RESULT || rs->sr_type == REP_EXTENDED ||
		op->o_abandon || rs->sr_err == SLAPD_ABANDON )
	{
		rwm_op_rollback( op, rs, ros );

		op->o_callback = op->o_callback->sc_next;
		op->o_tmpfree( cb, op->o_tmpmemctx );
	}

	return SLAP_CB_CONTINUE;
}

static rwm_op_cb *
rwm_callback_get( Operation *op )
{
	rwm_op_cb	*roc;

	roc = op->o_tmpalloc( sizeof( struct rwm_op_cb ), op->o_tmpmemctx );
	roc->cb.sc_cleanup = rwm_op_cleanup;
	roc->cb.sc_response = NULL;
	roc->cb.sc_next = op->o_callback;
	roc->cb.sc_private = &roc->ros;
	roc->ros.r_tag = op->o_tag;
	roc->ros.ro_dn = op->o_req_dn;
	roc->ros.ro_ndn = op->o_req_ndn;
	BER_BVZERO( &roc->ros.r_dn );
	BER_BVZERO( &roc->ros.r_ndn );
	BER_BVZERO( &roc->ros.rx_dn );
	BER_BVZERO( &roc->ros.rx_ndn );
	roc->ros.mapped_attrs = NULL;
	roc->ros.o_request = op->o_request;

	return roc;
}


static int
rwm_op_dn_massage( Operation *op, SlapReply *rs, void *cookie,
	rwm_op_state *ros )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	struct berval		dn = BER_BVNULL,
				ndn = BER_BVNULL;
	int			rc = 0;
	dncookie		dc;

	/*
	 * Rewrite the dn if needed
	 */
	dc.rwmap = rwmap;
	dc.conn = op->o_conn;
	dc.rs = rs;
	dc.ctx = (char *)cookie;

	/* NOTE: in those cases where only the ndn is available,
	 * and the caller sets op->o_req_dn = op->o_req_ndn,
	 * only rewrite the op->o_req_ndn and use it as 
	 * op->o_req_dn as well */
	ndn = op->o_req_ndn;
	if ( op->o_req_dn.bv_val != op->o_req_ndn.bv_val ) {
		dn = op->o_req_dn;
		rc = rwm_dn_massage_pretty_normalize( &dc, &op->o_req_dn, &dn, &ndn );
	} else {
		rc = rwm_dn_massage_normalize( &dc, &op->o_req_ndn, &ndn );
	}

	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	if ( ( op->o_req_dn.bv_val != op->o_req_ndn.bv_val && dn.bv_val == op->o_req_dn.bv_val )
			|| ndn.bv_val == op->o_req_ndn.bv_val )
	{
		return LDAP_SUCCESS;
	}

	if ( op->o_req_dn.bv_val != op->o_req_ndn.bv_val ) {
		op->o_req_dn = dn;
		assert( BER_BVISNULL( &ros->r_dn ) );
		ros->r_dn = dn;
	} else {
		op->o_req_dn = ndn;
	}
	op->o_req_ndn = ndn;
	assert( BER_BVISNULL( &ros->r_ndn ) );
	ros->r_ndn = ndn;

	if ( ros->r_tag == LDAP_REQ_EXTENDED ) {
		ros->rx_dn = ros->r_dn;
		ros->rx_ndn = ros->r_ndn;
	}

	return LDAP_SUCCESS;
}

static int
rwm_op_add( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	int			rc,
				i;
	Attribute		**ap = NULL;
	char			*olddn = op->o_req_dn.bv_val;
	int			isupdate;

	rwm_op_cb		*roc = rwm_callback_get( op );

	rc = rwm_op_dn_massage( op, rs, "addDN", &roc->ros );
	if ( rc != LDAP_SUCCESS ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		send_ldap_error( op, rs, rc, "addDN massage error" );
		return -1;
	}

	if ( olddn != op->o_req_dn.bv_val ) {
		ber_bvreplace( &op->ora_e->e_name, &op->o_req_dn );
		ber_bvreplace( &op->ora_e->e_nname, &op->o_req_ndn );
	}

	/* Count number of attributes in entry */ 
	isupdate = be_shadow_update( op );
	for ( i = 0, ap = &op->oq_add.rs_e->e_attrs; *ap; ) {
		Attribute	*a;

		if ( (*ap)->a_desc == slap_schema.si_ad_objectClass ||
				(*ap)->a_desc == slap_schema.si_ad_structuralObjectClass )
		{
			int		j, last;

			last = (*ap)->a_numvals - 1;
			for ( j = 0; !BER_BVISNULL( &(*ap)->a_vals[ j ] ); j++ ) {
				struct ldapmapping	*mapping = NULL;

				( void )rwm_mapping( &rwmap->rwm_oc, &(*ap)->a_vals[ j ],
						&mapping, RWM_MAP );
				if ( mapping == NULL ) {
					if ( rwmap->rwm_at.drop_missing ) {
						/* FIXME: we allow to remove objectClasses as well;
						 * if the resulting entry is inconsistent, that's
						 * the relayed database's business...
						 */
						ch_free( (*ap)->a_vals[ j ].bv_val );
						if ( last > j ) {
							(*ap)->a_vals[ j ] = (*ap)->a_vals[ last ];
						}
						BER_BVZERO( &(*ap)->a_vals[ last ] );
						(*ap)->a_numvals--;
						last--;
						j--;
					}

				} else {
					ch_free( (*ap)->a_vals[ j ].bv_val );
					ber_dupbv( &(*ap)->a_vals[ j ], &mapping->m_dst );
				}
			}

		} else if ( !isupdate && !get_relax( op ) && (*ap)->a_desc->ad_type->sat_no_user_mod )
		{
			goto next_attr;

		} else {
			struct ldapmapping	*mapping = NULL;

			( void )rwm_mapping( &rwmap->rwm_at, &(*ap)->a_desc->ad_cname,
					&mapping, RWM_MAP );
			if ( mapping == NULL ) {
				if ( rwmap->rwm_at.drop_missing ) {
					goto cleanup_attr;
				}
			}

			if ( (*ap)->a_desc->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName
					|| ( mapping != NULL && mapping->m_dst_ad->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName ) )
			{
				/*
				 * FIXME: rewrite could fail; in this case
				 * the operation should give up, right?
				 */
				rc = rwm_dnattr_rewrite( op, rs, "addAttrDN",
						(*ap)->a_vals,
						(*ap)->a_nvals ? &(*ap)->a_nvals : NULL );
				if ( rc ) {
					goto cleanup_attr;
				}

			} else if ( (*ap)->a_desc == slap_schema.si_ad_ref ) {
				rc = rwm_referral_rewrite( op, rs, "referralAttrDN",
						(*ap)->a_vals,
						(*ap)->a_nvals ? &(*ap)->a_nvals : NULL );
				if ( rc != LDAP_SUCCESS ) {
					goto cleanup_attr;
				}
			}
		
			if ( mapping != NULL ) {
				assert( mapping->m_dst_ad != NULL );
				(*ap)->a_desc = mapping->m_dst_ad;
			}
		}

next_attr:;
		ap = &(*ap)->a_next;
		continue;

cleanup_attr:;
		/* FIXME: leaking attribute/values? */
		a = *ap;

		*ap = (*ap)->a_next;
		attr_free( a );
	}

	op->o_callback = &roc->cb;

	return SLAP_CB_CONTINUE;
}

static int
rwm_conn_init( BackendDB *be, Connection *conn )
{
	slap_overinst		*on = (slap_overinst *) be->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	( void )rewrite_session_init( rwmap->rwm_rw, conn );

	return SLAP_CB_CONTINUE;
}

static int
rwm_conn_destroy( BackendDB *be, Connection *conn )
{
	slap_overinst		*on = (slap_overinst *) be->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	( void )rewrite_session_delete( rwmap->rwm_rw, conn );

	return SLAP_CB_CONTINUE;
}

static int
rwm_op_bind( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	int			rc;

	rwm_op_cb		*roc = rwm_callback_get( op );

	rc = rwm_op_dn_massage( op, rs, "bindDN", &roc->ros );
	if ( rc != LDAP_SUCCESS ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		send_ldap_error( op, rs, rc, "bindDN massage error" );
		return -1;
	}

	overlay_callback_after_backover( op, &roc->cb, 1 );

	return SLAP_CB_CONTINUE;
}

static int
rwm_op_unbind( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	rewrite_session_delete( rwmap->rwm_rw, op->o_conn );

	return SLAP_CB_CONTINUE;
}

static int
rwm_op_compare( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	int			rc;
	struct berval		mapped_vals[2] = { BER_BVNULL, BER_BVNULL };

	rwm_op_cb		*roc = rwm_callback_get( op );

	rc = rwm_op_dn_massage( op, rs, "compareDN", &roc->ros );
	if ( rc != LDAP_SUCCESS ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		send_ldap_error( op, rs, rc, "compareDN massage error" );
		return -1;
	}

	/* if the attribute is an objectClass, try to remap its value */
	if ( op->orc_ava->aa_desc == slap_schema.si_ad_objectClass
			|| op->orc_ava->aa_desc == slap_schema.si_ad_structuralObjectClass )
	{
		rwm_map( &rwmap->rwm_oc, &op->orc_ava->aa_value,
				&mapped_vals[0], RWM_MAP );
		if ( BER_BVISNULL( &mapped_vals[0] ) || BER_BVISEMPTY( &mapped_vals[0] ) )
		{
			op->o_bd->bd_info = (BackendInfo *)on->on_info;
			send_ldap_error( op, rs, LDAP_OTHER, "compare objectClass map error" );
			return -1;

		} else if ( mapped_vals[0].bv_val != op->orc_ava->aa_value.bv_val ) {
			ber_dupbv_x( &op->orc_ava->aa_value, &mapped_vals[0],
				op->o_tmpmemctx );
		}

	} else {
		struct ldapmapping	*mapping = NULL;
		AttributeDescription	*ad = op->orc_ava->aa_desc;

		( void )rwm_mapping( &rwmap->rwm_at, &op->orc_ava->aa_desc->ad_cname,
				&mapping, RWM_MAP );
		if ( mapping == NULL ) {
			if ( rwmap->rwm_at.drop_missing ) {
				op->o_bd->bd_info = (BackendInfo *)on->on_info;
				send_ldap_error( op, rs, LDAP_OTHER, "compare attributeType map error" );
				return -1;
			}

		} else {
			assert( mapping->m_dst_ad != NULL );
			ad = mapping->m_dst_ad;
		}

		if ( op->orc_ava->aa_desc->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName
				|| ( mapping != NULL && mapping->m_dst_ad->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName ) )
		{
			struct berval	*mapped_valsp[2];
			
			mapped_valsp[0] = &mapped_vals[0];
			mapped_valsp[1] = &mapped_vals[1];

			mapped_vals[0] = op->orc_ava->aa_value;

			rc = rwm_dnattr_rewrite( op, rs, "compareAttrDN", NULL, mapped_valsp );

			if ( rc != LDAP_SUCCESS ) {
				op->o_bd->bd_info = (BackendInfo *)on->on_info;
				send_ldap_error( op, rs, rc, "compareAttrDN massage error" );
				return -1;
			}

			if ( mapped_vals[ 0 ].bv_val != op->orc_ava->aa_value.bv_val ) {
				/* NOTE: if we get here, rwm_dnattr_rewrite()
				 * already freed the old value, so now 
				 * it's invalid */
				ber_dupbv_x( &op->orc_ava->aa_value, &mapped_vals[0],
					op->o_tmpmemctx );
				ber_memfree_x( mapped_vals[ 0 ].bv_val, NULL );
			}
		}
		op->orc_ava->aa_desc = ad;
	}

	op->o_callback = &roc->cb;

	return SLAP_CB_CONTINUE;
}

static int
rwm_op_delete( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	int			rc;

	rwm_op_cb		*roc = rwm_callback_get( op );

	rc = rwm_op_dn_massage( op, rs, "deleteDN", &roc->ros );
	if ( rc != LDAP_SUCCESS ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		send_ldap_error( op, rs, rc, "deleteDN massage error" );
		return -1;
	}

	op->o_callback = &roc->cb;

	return SLAP_CB_CONTINUE;
}

static int
rwm_op_modify( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	int			isupdate;
	Modifications		**mlp;
	int			rc;

	rwm_op_cb		*roc = rwm_callback_get( op );

	rc = rwm_op_dn_massage( op, rs, "modifyDN", &roc->ros );
	if ( rc != LDAP_SUCCESS ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		send_ldap_error( op, rs, rc, "modifyDN massage error" );
		return -1;
	}

	isupdate = be_shadow_update( op );
	for ( mlp = &op->orm_modlist; *mlp; ) {
		int			is_oc = 0;
		Modifications		*ml = *mlp;
		struct ldapmapping	*mapping = NULL;

		/* ml points to a temporary mod until needs duplication */
		if ( ml->sml_desc == slap_schema.si_ad_objectClass 
				|| ml->sml_desc == slap_schema.si_ad_structuralObjectClass )
		{
			is_oc = 1;

		} else if ( !isupdate && !get_relax( op ) && ml->sml_desc->ad_type->sat_no_user_mod  )
		{
			ml = ch_malloc( sizeof( Modifications ) );
			*ml = **mlp;
			if ( (*mlp)->sml_values ) {
				ber_bvarray_dup_x( &ml->sml_values, (*mlp)->sml_values, NULL );
				if ( (*mlp)->sml_nvalues ) {
					ber_bvarray_dup_x( &ml->sml_nvalues, (*mlp)->sml_nvalues, NULL );
				}
			}
			*mlp = ml;
			goto next_mod;

		} else {
			int			drop_missing;

			drop_missing = rwm_mapping( &rwmap->rwm_at,
					&ml->sml_desc->ad_cname,
					&mapping, RWM_MAP );
			if ( drop_missing || ( mapping != NULL && BER_BVISNULL( &mapping->m_dst ) ) )
			{
				goto skip_mod;
			}
		}

		/* duplicate the modlist */
		ml = ch_malloc( sizeof( Modifications ));
		*ml = **mlp;
		*mlp = ml;

		if ( ml->sml_values != NULL ) {
			int i, num;
			struct berval *bva;

			for ( num = 0; !BER_BVISNULL( &ml->sml_values[ num ] ); num++ )
				/* count values */ ;

			bva = ch_malloc( (num+1) * sizeof( struct berval ));
			for (i=0; i<num; i++)
				ber_dupbv( &bva[i], &ml->sml_values[i] );
			BER_BVZERO( &bva[i] );
			ml->sml_values = bva;

			if ( ml->sml_nvalues ) {
				bva = ch_malloc( (num+1) * sizeof( struct berval ));
				for (i=0; i<num; i++)
					ber_dupbv( &bva[i], &ml->sml_nvalues[i] );
				BER_BVZERO( &bva[i] );
				ml->sml_nvalues = bva;
			}

			if ( is_oc ) {
				int	last, j;

				last = num-1;

				for ( j = 0; !BER_BVISNULL( &ml->sml_values[ j ] ); j++ ) {
					struct ldapmapping	*oc_mapping = NULL;
		
					( void )rwm_mapping( &rwmap->rwm_oc, &ml->sml_values[ j ],
							&oc_mapping, RWM_MAP );
					if ( oc_mapping == NULL ) {
						if ( rwmap->rwm_at.drop_missing ) {
							/* FIXME: we allow to remove objectClasses as well;
							 * if the resulting entry is inconsistent, that's
							 * the relayed database's business...
							 */
							if ( last > j ) {
								ch_free( ml->sml_values[ j ].bv_val );
								ml->sml_values[ j ] = ml->sml_values[ last ];
							}
							BER_BVZERO( &ml->sml_values[ last ] );
							last--;
							j--;
						}
	
					} else {
						ch_free( ml->sml_values[ j ].bv_val );
						ber_dupbv( &ml->sml_values[ j ], &oc_mapping->m_dst );
					}
				}

			} else {
				if ( ml->sml_desc->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName
						|| ( mapping != NULL && mapping->m_dst_ad->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName ) )
				{
					rc = rwm_dnattr_rewrite( op, rs, "modifyAttrDN",
							ml->sml_values,
							ml->sml_nvalues ? &ml->sml_nvalues : NULL );

				} else if ( ml->sml_desc == slap_schema.si_ad_ref ) {
					rc = rwm_referral_rewrite( op, rs,
							"referralAttrDN",
							ml->sml_values,
							ml->sml_nvalues ? &ml->sml_nvalues : NULL );
					if ( rc != LDAP_SUCCESS ) {
						goto cleanup_mod;
					}
				}

				if ( rc != LDAP_SUCCESS ) {
					goto cleanup_mod;
				}
			}
		}

next_mod:;
		if ( mapping != NULL ) {
			/* use new attribute description */
			assert( mapping->m_dst_ad != NULL );
			ml->sml_desc = mapping->m_dst_ad;
		}

		mlp = &ml->sml_next;
		continue;

skip_mod:;
		*mlp = (*mlp)->sml_next;
		continue;

cleanup_mod:;
		ml = *mlp;
		*mlp = (*mlp)->sml_next;
		slap_mod_free( &ml->sml_mod, 0 );
		free( ml );
	}

	op->o_callback = &roc->cb;

	return SLAP_CB_CONTINUE;
}

static int
rwm_op_modrdn( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;
	
	int			rc;
	dncookie		dc;

	rwm_op_cb		*roc = rwm_callback_get( op );

	if ( op->orr_newSup ) {
		struct berval	nnewSup = BER_BVNULL;
		struct berval	newSup = BER_BVNULL;

		/*
		 * Rewrite the new superior, if defined and required
	 	 */
		dc.rwmap = rwmap;
		dc.conn = op->o_conn;
		dc.rs = rs;
		dc.ctx = "newSuperiorDN";
		newSup = *op->orr_newSup;
		nnewSup = *op->orr_nnewSup;
		rc = rwm_dn_massage_pretty_normalize( &dc, op->orr_newSup, &newSup, &nnewSup );
		if ( rc != LDAP_SUCCESS ) {
			op->o_bd->bd_info = (BackendInfo *)on->on_info;
			send_ldap_error( op, rs, rc, "newSuperiorDN massage error" );
			return -1;
		}

		if ( op->orr_newSup->bv_val != newSup.bv_val ) {
			op->orr_newSup = op->o_tmpalloc( sizeof( struct berval ),
				op->o_tmpmemctx );
			op->orr_nnewSup = op->o_tmpalloc( sizeof( struct berval ),
				op->o_tmpmemctx );
			*op->orr_newSup = newSup;
			*op->orr_nnewSup = nnewSup;
		}
	}

	/*
	 * Rewrite the newRDN, if needed
 	 */
	{
		struct berval	newrdn = BER_BVNULL;
		struct berval	nnewrdn = BER_BVNULL;

		dc.rwmap = rwmap;
		dc.conn = op->o_conn;
		dc.rs = rs;
		dc.ctx = "newRDN";
		newrdn = op->orr_newrdn;
		nnewrdn = op->orr_nnewrdn;
		rc = rwm_dn_massage_pretty_normalize( &dc, &op->orr_newrdn, &newrdn, &nnewrdn );
		if ( rc != LDAP_SUCCESS ) {
			op->o_bd->bd_info = (BackendInfo *)on->on_info;
			send_ldap_error( op, rs, rc, "newRDN massage error" );
			goto err;
		}

		if ( op->orr_newrdn.bv_val != newrdn.bv_val ) {
			op->orr_newrdn = newrdn;
			op->orr_nnewrdn = nnewrdn;
		}
	}

	/*
	 * Rewrite the dn, if needed
 	 */
	rc = rwm_op_dn_massage( op, rs, "renameDN", &roc->ros );
	if ( rc != LDAP_SUCCESS ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		send_ldap_error( op, rs, rc, "renameDN massage error" );
		goto err;
	}

	op->o_callback = &roc->cb;

	rc = SLAP_CB_CONTINUE;

	if ( 0 ) {
err:;
		if ( op->orr_newSup != roc->ros.orr_newSup ) {
			ch_free( op->orr_newSup->bv_val );
			ch_free( op->orr_nnewSup->bv_val );
			op->o_tmpfree( op->orr_newSup, op->o_tmpmemctx );
			op->o_tmpfree( op->orr_nnewSup, op->o_tmpmemctx );
			op->orr_newSup = roc->ros.orr_newSup;
			op->orr_nnewSup = roc->ros.orr_nnewSup;
		}

		if ( op->orr_newrdn.bv_val != roc->ros.orr_newrdn.bv_val ) {
			ch_free( op->orr_newrdn.bv_val );
			ch_free( op->orr_nnewrdn.bv_val );
			op->orr_newrdn = roc->ros.orr_newrdn;
			op->orr_nnewrdn = roc->ros.orr_nnewrdn;
		}
	}

	return rc;
}


static int
rwm_swap_attrs( Operation *op, SlapReply *rs )
{
	slap_callback	*cb = op->o_callback;
	rwm_op_state *ros = cb->sc_private;

	rs->sr_attrs = ros->ors_attrs;

	/* other overlays might have touched op->ors_attrs, 
	 * so we restore the original version here, otherwise
	 * attribute-mapping might fail */
	op->ors_attrs = ros->mapped_attrs; 
	
 	return SLAP_CB_CONTINUE;
}

/*
 * NOTE: this implementation of get/release entry is probably far from
 * optimal.  The rationale consists in intercepting the request directed
 * to the underlying database, in order to rewrite/remap the request,
 * perform it using the modified data, duplicate the resulting entry
 * and finally free it when release is called.
 * This implies that subsequent overlays are not called, as the request
 * is directly shunted to the underlying database.
 */
static int
rwm_entry_release_rw( Operation *op, Entry *e, int rw )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;

	/* can't be ours */
	if ( ((BackendInfo *)on->on_info->oi_orig)->bi_entry_get_rw == NULL ) {
		return SLAP_CB_CONTINUE;
	}

	/* just free entry if (probably) ours */
	if ( e->e_private == NULL && BER_BVISNULL( &e->e_bv ) ) {
		entry_free( e );
		return LDAP_SUCCESS;
	}

	return SLAP_CB_CONTINUE;
}

static int
rwm_entry_get_rw( Operation *op, struct berval *ndn,
	ObjectClass *oc, AttributeDescription *at, int rw, Entry **ep )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	int			rc;
	BackendDB		db;
	Operation		op2;
	SlapReply		rs = { REP_SEARCH };

	rwm_op_state		ros = { 0 };
	struct berval		mndn = BER_BVNULL;

	if ( ((BackendInfo *)on->on_info->oi_orig)->bi_entry_get_rw == NULL ) {
		return SLAP_CB_CONTINUE;
	}

	/* massage DN */
	op2.o_tag = LDAP_REQ_SEARCH;
	op2 = *op;
	op2.o_req_dn = *ndn;
	op2.o_req_ndn = *ndn;
	rc = rwm_op_dn_massage( &op2, &rs, "searchDN", &ros );
	if ( rc != LDAP_SUCCESS ) {
		return LDAP_OTHER;
	}

	mndn = BER_BVISNULL( &ros.r_ndn ) ? *ndn : ros.r_ndn;

	/* map attribute & objectClass */
	if ( at != NULL ) {
	}

	if ( oc != NULL ) {
	}

	/* fetch entry */
	db = *op->o_bd;
	op2.o_bd = &db;
	op2.o_bd->bd_info = (BackendInfo *)on->on_info->oi_orig;
	op2.ors_attrs = slap_anlist_all_attributes;
	rc = op2.o_bd->bd_info->bi_entry_get_rw( &op2, &mndn, oc, at, rw, ep );
	if ( rc == LDAP_SUCCESS && *ep != NULL ) {
		/* we assume be_entry_release() needs to be called */
		rs.sr_flags = REP_ENTRY_MUSTRELEASE;
		rs.sr_entry = *ep;

		/* duplicate & release */
		op2.o_bd->bd_info = (BackendInfo *)on;
		rc = rwm_send_entry( &op2, &rs );
		RS_ASSERT( rs.sr_flags & REP_ENTRY_MUSTFLUSH );
		if ( rc == SLAP_CB_CONTINUE ) {
			*ep = rs.sr_entry;
			rc = LDAP_SUCCESS;
		} else {
			assert( rc != LDAP_SUCCESS && rs.sr_entry == *ep );
			*ep = NULL;
			op2.o_bd->bd_info = (BackendInfo *)on->on_info;
			be_entry_release_r( &op2, rs.sr_entry );
			op2.o_bd->bd_info = (BackendInfo *)on;
		}
	}

	if ( !BER_BVISNULL( &ros.r_ndn) && ros.r_ndn.bv_val != ndn->bv_val ) {
		op->o_tmpfree( ros.r_ndn.bv_val, op->o_tmpmemctx );
	}

	return rc;
}

static int
rwm_op_search( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	int			rc;
	dncookie		dc;

	struct berval		fstr = BER_BVNULL;
	Filter			*f = NULL;

	AttributeName		*an = NULL;

	char			*text = NULL;

	rwm_op_cb		*roc = rwm_callback_get( op );

	rc = rewrite_session_var_set( rwmap->rwm_rw, op->o_conn,
		"searchFilter", op->ors_filterstr.bv_val );
	if ( rc == LDAP_SUCCESS )
		rc = rwm_op_dn_massage( op, rs, "searchDN", &roc->ros );
	if ( rc != LDAP_SUCCESS ) {
		text = "searchDN massage error";
		goto error_return;
	}

	/*
	 * Rewrite the dn if needed
	 */
	dc.rwmap = rwmap;
	dc.conn = op->o_conn;
	dc.rs = rs;
	dc.ctx = "searchFilterAttrDN";

	rc = rwm_filter_map_rewrite( op, &dc, op->ors_filter, &fstr );
	if ( rc != LDAP_SUCCESS ) {
		text = "searchFilter/searchFilterAttrDN massage error";
		goto error_return;
	}

	f = str2filter_x( op, fstr.bv_val );

	if ( f == NULL ) {
		text = "massaged filter parse error";
		goto error_return;
	}

	op->ors_filter = f;
	op->ors_filterstr = fstr;

	rc = rwm_map_attrnames( op, &rwmap->rwm_at, &rwmap->rwm_oc,
			op->ors_attrs, &an, RWM_MAP );
	if ( rc != LDAP_SUCCESS ) {
		text = "attribute list mapping error";
		goto error_return;
	}

	op->ors_attrs = an;
	/* store the mapped Attributes for later usage, in
	 * the case that other overlays change op->ors_attrs */
	roc->ros.mapped_attrs = an;
	roc->cb.sc_response = rwm_swap_attrs;

	op->o_callback = &roc->cb;

	return SLAP_CB_CONTINUE;

error_return:;
	if ( an != NULL ) {
		ch_free( an );
	}

	if ( f != NULL ) {
		filter_free_x( op, f, 1 );
	}

	if ( !BER_BVISNULL( &fstr ) ) {
		op->o_tmpfree( fstr.bv_val, op->o_tmpmemctx );
	}

	rwm_op_rollback( op, rs, &roc->ros );
	op->oq_search = roc->ros.oq_search;
	op->o_tmpfree( roc, op->o_tmpmemctx );

	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	send_ldap_error( op, rs, rc, text );

	return -1;

}

static int
rwm_exop_passwd( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	int			rc;
	rwm_op_cb *roc;

	struct berval	id = BER_BVNULL,
			pwold = BER_BVNULL,
			pwnew = BER_BVNULL;
	BerElement *ber = NULL;

	if ( !BER_BVISNULL( &op->o_req_ndn ) ) {
		return LDAP_SUCCESS;
	}

	if ( !SLAP_ISGLOBALOVERLAY( op->o_bd ) ) {
		rs->sr_err = LDAP_OTHER;
		return rs->sr_err;
	}

	rs->sr_err = slap_passwd_parse( op->ore_reqdata, &id,
		&pwold, &pwnew, &rs->sr_text );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		return rs->sr_err;
	}

	if ( !BER_BVISNULL( &id ) ) {
		char idNul = id.bv_val[id.bv_len];
		id.bv_val[id.bv_len] = '\0';
		rs->sr_err = dnPrettyNormal( NULL, &id, &op->o_req_dn,
				&op->o_req_ndn, op->o_tmpmemctx );
		id.bv_val[id.bv_len] = idNul;
		if ( rs->sr_err != LDAP_SUCCESS ) {
			rs->sr_text = "Invalid DN";
			return rs->sr_err;
		}

	} else {
		ber_dupbv_x( &op->o_req_dn, &op->o_dn, op->o_tmpmemctx );
		ber_dupbv_x( &op->o_req_ndn, &op->o_ndn, op->o_tmpmemctx );
	}

	roc = rwm_callback_get( op );

	rc = rwm_op_dn_massage( op, rs, "extendedDN", &roc->ros );
	if ( rc != LDAP_SUCCESS ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		send_ldap_error( op, rs, rc, "extendedDN massage error" );
		return -1;
	}

	ber = ber_alloc_t( LBER_USE_DER );
	if ( !ber ) {
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "No memory";
		return rs->sr_err;
	}
	ber_printf( ber, "{" );
	if ( !BER_BVISNULL( &id )) {
		ber_printf( ber, "tO", LDAP_TAG_EXOP_MODIFY_PASSWD_ID, 
			&op->o_req_dn );
	}
	if ( !BER_BVISNULL( &pwold )) {
		ber_printf( ber, "tO", LDAP_TAG_EXOP_MODIFY_PASSWD_OLD, &pwold );
	}
	if ( !BER_BVISNULL( &pwnew )) {
		ber_printf( ber, "tO", LDAP_TAG_EXOP_MODIFY_PASSWD_NEW, &pwnew );
	}
	ber_printf( ber, "N}" );
	ber_flatten( ber, &op->ore_reqdata );
	ber_free( ber, 1 );

	op->o_callback = &roc->cb;

	return SLAP_CB_CONTINUE;
}

static struct exop {
	struct berval	oid;
	BI_op_extended	*extended;
} exop_table[] = {
	{ BER_BVC(LDAP_EXOP_MODIFY_PASSWD),	rwm_exop_passwd },
	{ BER_BVNULL, NULL }
};

static int
rwm_extended( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	int			rc;
	rwm_op_cb *roc;

	int	i;

	for ( i = 0; exop_table[i].extended != NULL; i++ ) {
		if ( bvmatch( &exop_table[i].oid, &op->oq_extended.rs_reqoid ) )
		{
			rc = exop_table[i].extended( op, rs );
			switch ( rc ) {
			case LDAP_SUCCESS:
				break;

			case SLAP_CB_CONTINUE:
			case SLAPD_ABANDON:
				return rc;

			default:
				send_ldap_result( op, rs );
				return rc;
			}
			break;
		}
	}

	roc = rwm_callback_get( op );

	rc = rwm_op_dn_massage( op, rs, "extendedDN", &roc->ros );
	if ( rc != LDAP_SUCCESS ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		send_ldap_error( op, rs, rc, "extendedDN massage error" );
		return -1;
	}

	/* TODO: rewrite/map extended data ? ... */
	op->o_callback = &roc->cb;

	return SLAP_CB_CONTINUE;
}

static void
rwm_matched( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	struct berval		dn, mdn;
	dncookie		dc;
	int			rc;

	if ( rs->sr_matched == NULL ) {
		return;
	}

	dc.rwmap = rwmap;
	dc.conn = op->o_conn;
	dc.rs = rs;
	dc.ctx = "matchedDN";
	ber_str2bv( rs->sr_matched, 0, 0, &dn );
	mdn = dn;
	rc = rwm_dn_massage_pretty( &dc, &dn, &mdn );
	if ( rc != LDAP_SUCCESS ) {
		rs->sr_err = rc;
		rs->sr_text = "Rewrite error";

	} else if ( mdn.bv_val != dn.bv_val ) {
		if ( rs->sr_flags & REP_MATCHED_MUSTBEFREED ) {
			ch_free( (void *)rs->sr_matched );

		} else {
			rs->sr_flags |= REP_MATCHED_MUSTBEFREED;
		}
		rs->sr_matched = mdn.bv_val;
	}
}

static int
rwm_attrs( Operation *op, SlapReply *rs, Attribute** a_first, int stripEntryDN )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	dncookie		dc;
	int			rc;
	Attribute		**ap;
	int			isupdate;
	int			check_duplicate_attrs = 0;

	/*
	 * Rewrite the dn attrs, if needed
	 */
	dc.rwmap = rwmap;
	dc.conn = op->o_conn;
	dc.rs = NULL; 

	/* FIXME: the entries are in the remote mapping form;
	 * so we need to select those attributes we are willing
	 * to return, and remap them accordingly */

	/* FIXME: in principle, one could map an attribute
	 * on top of another, which already exists.
	 * As such, in the end there might exist more than
	 * one instance of an attribute.
	 * We should at least check if this occurs, and issue
	 * an error (because multiple instances of attrs in 
	 * response are not valid), or merge the values (what
	 * about duplicate values?) */
	isupdate = be_shadow_update( op );
	for ( ap = a_first; *ap; ) {
		struct ldapmapping	*mapping = NULL;
		int			drop_missing;
		int			last = -1;
		Attribute		*a;

		if ( ( rwmap->rwm_flags & RWM_F_DROP_UNREQUESTED_ATTRS ) &&
				op->ors_attrs != NULL && 
				!SLAP_USERATTRS( rs->sr_attr_flags ) &&
				!ad_inlist( (*ap)->a_desc, op->ors_attrs ) )
		{
			goto cleanup_attr;
		}

		drop_missing = rwm_mapping( &rwmap->rwm_at,
				&(*ap)->a_desc->ad_cname, &mapping, RWM_REMAP );
		if ( drop_missing || ( mapping != NULL && BER_BVISEMPTY( &mapping->m_dst ) ) )
		{
			goto cleanup_attr;
		}
		if ( mapping != NULL ) {
			assert( mapping->m_dst_ad != NULL );

			/* try to normalize mapped Attributes if the original 
			 * AttributeType was not normalized */
			if ( (!(*ap)->a_desc->ad_type->sat_equality || 
				!(*ap)->a_desc->ad_type->sat_equality->smr_normalize) &&
				mapping->m_dst_ad->ad_type->sat_equality &&
				mapping->m_dst_ad->ad_type->sat_equality->smr_normalize )
			{
				if ((rwmap->rwm_flags & RWM_F_NORMALIZE_MAPPED_ATTRS))
				{
					int i = 0;

					last = (*ap)->a_numvals;
					if ( last )
					{
						(*ap)->a_nvals = ch_malloc( (last+1) * sizeof(struct berval) );

						for ( i = 0; !BER_BVISNULL( &(*ap)->a_vals[i]); i++ ) {
							int		rc;
							/*
							 * check that each value is valid per syntax
							 * and pretty if appropriate
							 */
							rc = mapping->m_dst_ad->ad_type->sat_equality->smr_normalize(
								SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
								mapping->m_dst_ad->ad_type->sat_syntax,
								mapping->m_dst_ad->ad_type->sat_equality,
								&(*ap)->a_vals[i], &(*ap)->a_nvals[i],
								NULL );

							if ( rc != LDAP_SUCCESS ) {
								/* FIXME: this is wrong, putting a non-normalized value
								 * into nvals. But when a proxy sends us bogus data,
								 * we still need to give it to the client, even if it
								 * violates the syntax. I.e., we don't want to silently
								 * drop things and trigger an apparent data loss.
								 */
								ber_dupbv( &(*ap)->a_nvals[i], &(*ap)->a_vals[i] );
							}
						}
						BER_BVZERO( &(*ap)->a_nvals[i] );
					}

				} else {
					assert( (*ap)->a_nvals == (*ap)->a_vals );
					(*ap)->a_nvals = NULL;
					ber_bvarray_dup_x( &(*ap)->a_nvals, (*ap)->a_vals, NULL );
				}
			}

			/* rewrite the attribute description */
			(*ap)->a_desc = mapping->m_dst_ad;

			/* will need to check for duplicate attrs */
			check_duplicate_attrs++;
		}

		if ( (*ap)->a_desc == slap_schema.si_ad_entryDN ) {
			if ( stripEntryDN ) {
				/* will be generated by frontend */
				goto cleanup_attr;
			}
			
		} else if ( !isupdate
			&& !get_relax( op )
			&& (*ap)->a_desc->ad_type->sat_no_user_mod 
			&& (*ap)->a_desc->ad_type != slap_schema.si_at_undefined )
		{
			goto next_attr;
		}

		if ( last == -1 ) { /* not yet counted */ 
			last = (*ap)->a_numvals;
		}

		if ( last == 0 ) {
			/* empty? leave it in place because of attrsonly and vlv */
			goto next_attr;
		}
		last--;

		if ( (*ap)->a_desc == slap_schema.si_ad_objectClass
				|| (*ap)->a_desc == slap_schema.si_ad_structuralObjectClass )
		{
			struct berval	*bv;
			
			for ( bv = (*ap)->a_vals; !BER_BVISNULL( bv ); bv++ ) {
				struct berval	mapped;

				rwm_map( &rwmap->rwm_oc, &bv[0], &mapped, RWM_REMAP );
				if ( BER_BVISNULL( &mapped ) || BER_BVISEMPTY( &mapped ) ) {
remove_oc:;
					ch_free( bv[0].bv_val );
					BER_BVZERO( &bv[0] );
					if ( &(*ap)->a_vals[last] > &bv[0] ) {
						bv[0] = (*ap)->a_vals[last];
						BER_BVZERO( &(*ap)->a_vals[last] );
					}
					last--;
					bv--;

				} else if ( mapped.bv_val != bv[0].bv_val
					&& ber_bvstrcasecmp( &mapped, &bv[0] ) != 0 )
				{
					int	i;

					for ( i = 0; !BER_BVISNULL( &(*ap)->a_vals[ i ] ); i++ ) {
						if ( &(*ap)->a_vals[ i ] == bv ) {
							continue;
						}

						if ( ber_bvstrcasecmp( &mapped, &(*ap)->a_vals[ i ] ) == 0 ) {
							break;
						}
					}

					if ( !BER_BVISNULL( &(*ap)->a_vals[ i ] ) ) {
						goto remove_oc;
					}

					/*
					 * FIXME: after LBER_FREEing
					 * the value is replaced by
					 * ch_alloc'ed memory
					 */
					ber_bvreplace( &bv[0], &mapped );

					/* FIXME: will need to check
					 * if the structuralObjectClass
					 * changed */
				}
			}

		/*
		 * It is necessary to try to rewrite attributes with
		 * dn syntax because they might be used in ACLs as
		 * members of groups; since ACLs are applied to the
		 * rewritten stuff, no dn-based subject clause could
		 * be used at the ldap backend side (see
		 * http://www.OpenLDAP.org/faq/data/cache/452.html)
		 * The problem can be overcome by moving the dn-based
		 * ACLs to the target directory server, and letting
		 * everything pass thru the ldap backend. */
		/* FIXME: handle distinguishedName-like syntaxes, like
		 * nameAndOptionalUID */
		} else if ( (*ap)->a_desc->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName
				|| ( mapping != NULL && mapping->m_src_ad->ad_type->sat_syntax == slap_schema.si_syn_distinguishedName ) )
		{
			dc.ctx = "searchAttrDN";
			rc = rwm_dnattr_result_rewrite( &dc, (*ap)->a_vals, (*ap)->a_nvals );
			if ( rc != LDAP_SUCCESS ) {
				goto cleanup_attr;
			}

		} else if ( (*ap)->a_desc == slap_schema.si_ad_ref ) {
			dc.ctx = "searchAttrDN";
			rc = rwm_referral_result_rewrite( &dc, (*ap)->a_vals );
			if ( rc != LDAP_SUCCESS ) {
				goto cleanup_attr;
			}
		}


next_attr:;
		ap = &(*ap)->a_next;
		continue;

cleanup_attr:;
		a = *ap;
		*ap = (*ap)->a_next;

		attr_free( a );
	}

	/* only check if some mapping occurred */
	if ( check_duplicate_attrs ) {
		for ( ap = a_first; *ap != NULL; ap = &(*ap)->a_next ) {
			Attribute	**tap;

			for ( tap = &(*ap)->a_next; *tap != NULL; ) {
				if ( (*tap)->a_desc == (*ap)->a_desc ) {
					Entry		e = { 0 };
					Modification	mod = { 0 };
					const char	*text = NULL;
					char		textbuf[ SLAP_TEXT_BUFLEN ];
					Attribute	*next = (*tap)->a_next;

					BER_BVSTR( &e.e_name, "" );
					BER_BVSTR( &e.e_nname, "" );
					e.e_attrs = *ap;
					mod.sm_op = LDAP_MOD_ADD;
					mod.sm_desc = (*ap)->a_desc;
					mod.sm_type = mod.sm_desc->ad_cname;
					mod.sm_numvals = (*tap)->a_numvals;
					mod.sm_values = (*tap)->a_vals;
					if ( (*tap)->a_nvals != (*tap)->a_vals ) {
						mod.sm_nvalues = (*tap)->a_nvals;
					}

					(void)modify_add_values( &e, &mod,
						/* permissive */ 1,
						&text, textbuf, sizeof( textbuf ) );

					/* should not insert new attrs! */
					assert( e.e_attrs == *ap );

					attr_free( *tap );
					*tap = next;

				} else {
					tap = &(*tap)->a_next;
				}
			}
		}
	}

	return 0;
}

/* Should return SLAP_CB_CONTINUE or failure, never LDAP_SUCCESS. */
static int
rwm_send_entry( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	Entry			*e = NULL;
	struct berval		dn = BER_BVNULL,
				ndn = BER_BVNULL;
	dncookie		dc;
	int			rc;

	assert( rs->sr_entry != NULL );

	/*
	 * Rewrite the dn of the result, if needed
	 */
	dc.rwmap = rwmap;
	dc.conn = op->o_conn;
	dc.rs = NULL; 
	dc.ctx = "searchEntryDN";

	e = rs->sr_entry;
	if ( !( rs->sr_flags & REP_ENTRY_MODIFIABLE ) ) {
		/* FIXME: all we need to duplicate are:
		 * - dn
		 * - ndn
		 * - attributes that are requested
		 * - no values if attrsonly is set
		 */
		e = entry_dup( e );
		if ( e == NULL ) {
			rc = LDAP_NO_MEMORY;
			goto fail;
		}
	} else if ( rs->sr_flags & REP_ENTRY_MUSTRELEASE ) {
		/* ITS#6423: REP_ENTRY_MUSTRELEASE incompatible
		 * with REP_ENTRY_MODIFIABLE */
		RS_ASSERT( 0 );
		rc = 1;
		goto fail;
	}

	/*
	 * Note: this may fail if the target host(s) schema differs
	 * from the one known to the meta, and a DN with unknown
	 * attributes is returned.
	 */
	dn = e->e_name;
	ndn = e->e_nname;
	rc = rwm_dn_massage_pretty_normalize( &dc, &e->e_name, &dn, &ndn );
	if ( rc != LDAP_SUCCESS ) {
		rc = 1;
		goto fail;
	}

	if ( e->e_name.bv_val != dn.bv_val ) {
		ch_free( e->e_name.bv_val );
		ch_free( e->e_nname.bv_val );

		e->e_name = dn;
		e->e_nname = ndn;
	}

	/* TODO: map entry attribute types, objectclasses 
	 * and dn-valued attribute values */

	/* FIXME: the entries are in the remote mapping form;
	 * so we need to select those attributes we are willing
	 * to return, and remap them accordingly */
	(void)rwm_attrs( op, rs, &e->e_attrs, 1 );

	if ( e != rs->sr_entry ) {
		/* Reimplementing rs_replace_entry(), I suppose to
		 * bypass our own dubious rwm_entry_release_rw() */
		if ( rs->sr_flags & REP_ENTRY_MUSTRELEASE ) {
			rs->sr_flags ^= REP_ENTRY_MUSTRELEASE;
			op->o_bd->bd_info = (BackendInfo *)on->on_info;
			be_entry_release_r( op, rs->sr_entry );
			op->o_bd->bd_info = (BackendInfo *)on;
		} else if ( rs->sr_flags & REP_ENTRY_MUSTBEFREED ) {
			entry_free( rs->sr_entry );
		}
		rs->sr_entry = e;
		rs->sr_flags |= REP_ENTRY_MODIFIABLE | REP_ENTRY_MUSTBEFREED;
	}

	return SLAP_CB_CONTINUE;

fail:;
	if ( e != NULL && e != rs->sr_entry ) {
		if ( e->e_name.bv_val == dn.bv_val ) {
			BER_BVZERO( &e->e_name );
		}

		if ( e->e_nname.bv_val == ndn.bv_val ) {
			BER_BVZERO( &e->e_nname );
		}

		entry_free( e );
	}

	if ( !BER_BVISNULL( &dn ) ) {
		ch_free( dn.bv_val );
	}

	if ( !BER_BVISNULL( &ndn ) ) {
		ch_free( ndn.bv_val );
	}

	return rc;
}

static int
rwm_operational( Operation *op, SlapReply *rs )
{
	/* FIXME: the entries are in the remote mapping form;
	 * so we need to select those attributes we are willing
	 * to return, and remap them accordingly */
	if ( rs->sr_operational_attrs ) {
		rwm_attrs( op, rs, &rs->sr_operational_attrs, 1 );
	}

	return SLAP_CB_CONTINUE;
}

#if 0
/* don't use this; it cannot be reverted, and leaves op->o_req_dn
 * rewritten for subsequent operations; fine for plain suffixmassage,
 * but destroys everything else */
static int
rwm_chk_referrals( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	int			rc;

	rc = rwm_op_dn_massage( op, rs, "referralCheckDN" );
	if ( rc != LDAP_SUCCESS ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		send_ldap_error( op, rs, rc, "referralCheckDN massage error" );
		return -1;
	}

	return SLAP_CB_CONTINUE;
}
#endif

static int
rwm_rw_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv )
{
	slap_overinst		*on = (slap_overinst *) be->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	return rewrite_parse( rwmap->rwm_rw,
				fname, lineno, argc, argv );

	return 0;
}

static int
rwm_suffixmassage_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv )
{
	slap_overinst		*on = (slap_overinst *) be->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	struct berval		bvnc, nvnc, pvnc, brnc, nrnc, prnc;
	int			massaged;
	int			rc;
		
	/*
	 * syntax:
	 * 
	 * 	suffixmassage [<suffix>] <massaged suffix>
	 *
	 * the [<suffix>] field must be defined as a valid suffix
	 * for the current database;
	 * the <massaged suffix> shouldn't have already been
	 * defined as a valid suffix for the current server
	 */
	if ( argc == 2 ) {
		if ( be->be_suffix == NULL ) {
 			Debug( LDAP_DEBUG_ANY, "%s: line %d: "
				       " \"suffixMassage [<suffix>]"
				       " <massaged suffix>\" without "
				       "<suffix> part requires database "
				       "suffix be defined first.\n",
				fname, lineno, 0 );
			return 1;
		}
		bvnc = be->be_suffix[ 0 ];
		massaged = 1;

	} else if ( argc == 3 ) {
		ber_str2bv( argv[ 1 ], 0, 0, &bvnc );
		massaged = 2;

	} else  {
 		Debug( LDAP_DEBUG_ANY, "%s: line %d: syntax is"
			       " \"suffixMassage [<suffix>]"
			       " <massaged suffix>\"\n",
			fname, lineno, 0 );
		return 1;
	}

	if ( dnPrettyNormal( NULL, &bvnc, &pvnc, &nvnc, NULL ) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s: line %d: suffix DN %s is invalid\n",
			fname, lineno, bvnc.bv_val );
		return 1;
	}

	ber_str2bv( argv[ massaged ], 0, 0, &brnc );
	if ( dnPrettyNormal( NULL, &brnc, &prnc, &nrnc, NULL ) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s: line %d: suffix DN %s is invalid\n",
				fname, lineno, brnc.bv_val );
		free( nvnc.bv_val );
		free( pvnc.bv_val );
		return 1;
	}

	/*
	 * The suffix massaging is emulated 
	 * by means of the rewrite capabilities
	 */
 	rc = rwm_suffix_massage_config( rwmap->rwm_rw,
			&pvnc, &nvnc, &prnc, &nrnc );
	free( nvnc.bv_val );
	free( pvnc.bv_val );
	free( nrnc.bv_val );
	free( prnc.bv_val );

	return rc;
}

static int
rwm_m_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv )
{
	slap_overinst		*on = (slap_overinst *) be->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	/* objectclass/attribute mapping */
	return rwm_map_config( &rwmap->rwm_oc,
			&rwmap->rwm_at,
			fname, lineno, argc, argv );
}

static int
rwm_response( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	int		rc;

	if ( op->o_tag == LDAP_REQ_SEARCH && rs->sr_type == REP_SEARCH ) {
		return rwm_send_entry( op, rs );
	}

	switch( op->o_tag ) {
	case LDAP_REQ_SEARCH:
	case LDAP_REQ_BIND:
	case LDAP_REQ_ADD:
	case LDAP_REQ_DELETE:
	case LDAP_REQ_MODRDN:
	case LDAP_REQ_MODIFY:
	case LDAP_REQ_COMPARE:
	case LDAP_REQ_EXTENDED:
		if ( rs->sr_ref ) {
			dncookie		dc;

			/*
			 * Rewrite the dn of the referrals, if needed
			 */
			dc.rwmap = rwmap;
			dc.conn = op->o_conn;
			dc.rs = NULL; 
			dc.ctx = "referralDN";
			rc = rwm_referral_result_rewrite( &dc, rs->sr_ref );
			/* FIXME: impossible, so far */
			if ( rc != LDAP_SUCCESS ) {
				rs->sr_err = rc;
				break;
			}
		}

		rwm_matched( op, rs );
		break;
	}

	return SLAP_CB_CONTINUE;
}

static int
rwm_db_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv )
{
	slap_overinst		*on = (slap_overinst *) be->bd_info;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	int		rc = 0;
	char		*argv0 = NULL;

	if ( strncasecmp( argv[ 0 ], "rwm-", STRLENOF( "rwm-" ) ) == 0 ) {
		argv0 = argv[ 0 ];
		argv[ 0 ] = &argv0[ STRLENOF( "rwm-" ) ];
	}

	if ( strncasecmp( argv[0], "rewrite", STRLENOF("rewrite") ) == 0 ) {
		rc = rwm_rw_config( be, fname, lineno, argc, argv );

	} else if ( strcasecmp( argv[0], "map" ) == 0 ) {
		rc = rwm_m_config( be, fname, lineno, argc, argv );

	} else if ( strcasecmp( argv[0], "suffixmassage" ) == 0 ) {
		rc = rwm_suffixmassage_config( be, fname, lineno, argc, argv );

	} else if ( strcasecmp( argv[0], "t-f-support" ) == 0 ) {
		if ( argc != 2 ) {
			Debug( LDAP_DEBUG_ANY,
		"%s: line %d: \"t-f-support {no|yes|discover}\" needs 1 argument.\n",
					fname, lineno, 0 );
			return( 1 );
		}

		if ( strcasecmp( argv[ 1 ], "no" ) == 0 ) {
			rwmap->rwm_flags &= ~(RWM_F_SUPPORT_T_F_MASK2);

		} else if ( strcasecmp( argv[ 1 ], "yes" ) == 0 ) {
			rwmap->rwm_flags |= RWM_F_SUPPORT_T_F;

		/* TODO: not implemented yet */
		} else if ( strcasecmp( argv[ 1 ], "discover" ) == 0 ) {
			Debug( LDAP_DEBUG_ANY,
		"%s: line %d: \"discover\" not supported yet "
		"in \"t-f-support {no|yes|discover}\".\n",
					fname, lineno, 0 );
			return( 1 );
#if 0
			rwmap->rwm_flags |= RWM_F_SUPPORT_T_F_DISCOVER;
#endif

		} else {
			Debug( LDAP_DEBUG_ANY,
	"%s: line %d: unknown value \"%s\" for \"t-f-support {no|yes|discover}\".\n",
				fname, lineno, argv[ 1 ] );
			return 1;
		}

	} else if ( strcasecmp( argv[0], "normalize-mapped-attrs" ) ==  0 ) {
		if ( argc !=2 ) { 
			Debug( LDAP_DEBUG_ANY,
		"%s: line %d: \"normalize-mapped-attrs {no|yes}\" needs 1 argument.\n",
					fname, lineno, 0 );
			return( 1 );
		}

		if ( strcasecmp( argv[ 1 ], "no" ) == 0 ) {
			rwmap->rwm_flags &= ~(RWM_F_NORMALIZE_MAPPED_ATTRS);

		} else if ( strcasecmp( argv[ 1 ], "yes" ) == 0 ) {
			rwmap->rwm_flags |= RWM_F_NORMALIZE_MAPPED_ATTRS;
		}

	} else {
		rc = SLAP_CONF_UNKNOWN;
	}

	if ( argv0 ) {
		argv[ 0 ] = argv0;
	}

	return rc;
}

/*
 * dynamic configuration...
 */

enum {
	/* rewrite */
	RWM_CF_REWRITE = 1,

	/* map */
	RWM_CF_MAP,
	RWM_CF_T_F_SUPPORT,
	RWM_CF_NORMALIZE_MAPPED,
	RWM_CF_DROP_UNREQUESTED,

	RWM_CF_LAST
};

static slap_verbmasks t_f_mode[] = {
	{ BER_BVC( "true" ),		RWM_F_SUPPORT_T_F },
	{ BER_BVC( "yes" ),		RWM_F_SUPPORT_T_F },
	{ BER_BVC( "discover" ),	RWM_F_SUPPORT_T_F_DISCOVER },
	{ BER_BVC( "false" ),		RWM_F_NONE },
	{ BER_BVC( "no" ),		RWM_F_NONE },
	{ BER_BVNULL,			0 }
};

static ConfigDriver rwm_cf_gen;

static ConfigTable rwmcfg[] = {
	{ "rwm-rewrite", "rewrite",
		2, 0, STRLENOF("rwm-rewrite"),
		ARG_MAGIC|RWM_CF_REWRITE, rwm_cf_gen,
		"( OLcfgOvAt:16.1 NAME 'olcRwmRewrite' "
			"DESC 'Rewrites strings' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )",
		NULL, NULL },

	{ "rwm-suffixmassage", "[virtual]> <real",
		2, 3, 0, ARG_MAGIC|RWM_CF_REWRITE, rwm_cf_gen,
		NULL, NULL, NULL },
		
	{ "rwm-t-f-support", "true|false|discover",
		2, 2, 0, ARG_MAGIC|RWM_CF_T_F_SUPPORT, rwm_cf_gen,
		"( OLcfgOvAt:16.2 NAME 'olcRwmTFSupport' "
			"DESC 'Absolute filters support' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL },

	{ "rwm-map", "{objectClass|attribute}",
		2, 4, 0, ARG_MAGIC|RWM_CF_MAP, rwm_cf_gen,
		"( OLcfgOvAt:16.3 NAME 'olcRwmMap' "
			"DESC 'maps attributes/objectClasses' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )",
		NULL, NULL },

	{ "rwm-normalize-mapped-attrs", "true|false",
		2, 2, 0, ARG_MAGIC|ARG_ON_OFF|RWM_CF_NORMALIZE_MAPPED, rwm_cf_gen,
		"( OLcfgOvAt:16.4 NAME 'olcRwmNormalizeMapped' "
			"DESC 'Normalize mapped attributes/objectClasses' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },

	{ "rwm-drop-unrequested-attrs", "true|false",
		2, 2, 0, ARG_MAGIC|ARG_ON_OFF|RWM_CF_DROP_UNREQUESTED, rwm_cf_gen,
		"( OLcfgOvAt:16.5 NAME 'olcRwmDropUnrequested' "
			"DESC 'Drop unrequested attributes' "
			"SYNTAX OMsBoolean "
			"SINGLE-VALUE )",
		NULL, NULL },

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs rwmocs[] = {
	{ "( OLcfgOvOc:16.1 "
		"NAME 'olcRwmConfig' "
		"DESC 'Rewrite/remap configuration' "
		"SUP olcOverlayConfig "
		"MAY ( "
			"olcRwmRewrite $ "
			"olcRwmTFSupport $ "
			"olcRwmMap $ "
			"olcRwmNormalizeMapped "
			") )",
		Cft_Overlay, rwmcfg, NULL, NULL },
	{ NULL, 0, NULL }
};

static void
slap_bv_x_ordered_unparse( BerVarray in, BerVarray *out )
{
	int		i;
	BerVarray	bva = NULL;
	char		ibuf[32], *ptr;
	struct berval	idx;

	assert( in != NULL );

	for ( i = 0; !BER_BVISNULL( &in[i] ); i++ )
		/* count'em */ ;

	if ( i == 0 ) {
		return;
	}

	idx.bv_val = ibuf;

	bva = ch_malloc( ( i + 1 ) * sizeof(struct berval) );
	BER_BVZERO( &bva[ 0 ] );

	for ( i = 0; !BER_BVISNULL( &in[i] ); i++ ) {
		idx.bv_len = snprintf( idx.bv_val, sizeof( ibuf ), "{%d}", i );
		if ( idx.bv_len >= sizeof( ibuf ) ) {
			ber_bvarray_free( bva );
			return;
		}

		bva[i].bv_len = idx.bv_len + in[i].bv_len;
		bva[i].bv_val = ch_malloc( bva[i].bv_len + 1 );
		ptr = lutil_strcopy( bva[i].bv_val, ibuf );
		ptr = lutil_strcopy( ptr, in[i].bv_val );
		*ptr = '\0';
		BER_BVZERO( &bva[ i + 1 ] );
	}

	*out = bva;
}

static int
rwm_bva_add(
	BerVarray		*bva,
	int			idx,
	char			**argv )
{
	char		*line;
	struct berval	bv;

	line = ldap_charray2str( argv, "\" \"" );
	if ( line != NULL ) {
		int	len = strlen( argv[ 0 ] );

		ber_str2bv( line, 0, 0, &bv );
		AC_MEMCPY( &bv.bv_val[ len ], &bv.bv_val[ len + 1 ],
			bv.bv_len - ( len + 1 ) );
		bv.bv_val[ bv.bv_len - 1 ] = '"';

		if ( idx == -1 ) {
			ber_bvarray_add( bva, &bv );

		} else {
			(*bva)[ idx ] = bv;
		}

		return 0;
	}

	return -1;
}

static int
rwm_bva_rewrite_add(
	struct ldaprwmap	*rwmap,
	int			idx,
	char			**argv )
{
	return rwm_bva_add( &rwmap->rwm_bva_rewrite, idx, argv );
}

#ifdef unused
static int
rwm_bva_map_add(
	struct ldaprwmap	*rwmap,
	int			idx,
	char			**argv )
{
	return rwm_bva_add( &rwmap->rwm_bva_map, idx, argv );
}
#endif /* unused */

static int
rwm_info_init( struct rewrite_info ** rwm_rw )
{
	char			*rargv[ 3 ];

 	*rwm_rw = rewrite_info_init( REWRITE_MODE_USE_DEFAULT );
	if ( *rwm_rw == NULL ) {
 		return -1;
 	}

	/* this rewriteContext by default must be null;
	 * rules can be added if required */
	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "searchFilter";
	rargv[ 2 ] = NULL;
	rewrite_parse( *rwm_rw, "<suffix massage>", 1, 2, rargv );

	rargv[ 0 ] = "rewriteContext";
	rargv[ 1 ] = "default";
	rargv[ 2 ] = NULL;
	rewrite_parse( *rwm_rw, "<suffix massage>", 2, 2, rargv );

	return 0;
}

static int
rwm_cf_gen( ConfigArgs *c )
{
	slap_overinst		*on = (slap_overinst *)c->bi;
	struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

	BackendDB		db;
	char			*argv0;
	int			idx0 = 0;
	int			rc = 0;

	db = *c->be;
	db.bd_info = c->bi;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		struct berval	bv = BER_BVNULL;

		switch ( c->type ) {
		case RWM_CF_REWRITE:
			if ( rwmap->rwm_bva_rewrite == NULL ) {
				rc = 1;

			} else {
				slap_bv_x_ordered_unparse( rwmap->rwm_bva_rewrite, &c->rvalue_vals );
				if ( !c->rvalue_vals ) {
					rc = 1;
				}
			}
			break;

		case RWM_CF_T_F_SUPPORT:
			enum_to_verb( t_f_mode, (rwmap->rwm_flags & RWM_F_SUPPORT_T_F_MASK2), &bv );
			if ( BER_BVISNULL( &bv ) ) {
				/* there's something wrong... */
				assert( 0 );
				rc = 1;

			} else {
				value_add_one( &c->rvalue_vals, &bv );
			}
			break;

		case RWM_CF_MAP:
			if ( rwmap->rwm_bva_map == NULL ) {
				rc = 1;

			} else {
				slap_bv_x_ordered_unparse( rwmap->rwm_bva_map, &c->rvalue_vals );
				if ( !c->rvalue_vals ) {
					rc = 1;
				}
			}
			break;

		case RWM_CF_NORMALIZE_MAPPED:
			c->value_int = ( rwmap->rwm_flags & RWM_F_NORMALIZE_MAPPED_ATTRS );
			break;

		case RWM_CF_DROP_UNREQUESTED:
			c->value_int = ( rwmap->rwm_flags & RWM_F_DROP_UNREQUESTED_ATTRS );
			break;

		default:
			assert( 0 );
			rc = 1;
		}

		return rc;

	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch ( c->type ) {
		case RWM_CF_REWRITE:
			if ( c->valx >= 0 ) {
				int i;

				for ( i = 0; !BER_BVISNULL( &rwmap->rwm_bva_rewrite[ i ] ); i++ )
					/* count'em */ ;

				if ( c->valx >= i ) {
					rc = 1;
					break;
				}

				ber_memfree( rwmap->rwm_bva_rewrite[ c->valx ].bv_val );
				for ( i = c->valx; !BER_BVISNULL( &rwmap->rwm_bva_rewrite[ i + 1 ] ); i++ )
				{
					rwmap->rwm_bva_rewrite[ i ] = rwmap->rwm_bva_rewrite[ i + 1 ];
				}
				BER_BVZERO( &rwmap->rwm_bva_rewrite[ i ] );

				rewrite_info_delete( &rwmap->rwm_rw );
				assert( rwmap->rwm_rw == NULL );

				rc = rwm_info_init( &rwmap->rwm_rw );

				for ( i = 0; !BER_BVISNULL( &rwmap->rwm_bva_rewrite[ i ] ); i++ )
				{
					ConfigArgs ca = { 0 };

					ca.line = rwmap->rwm_bva_rewrite[ i ].bv_val;
					ca.argc = 0;
					config_fp_parse_line( &ca );
					
					if ( strcasecmp( ca.argv[ 0 ], "suffixmassage" ) == 0 ) {
						rc = rwm_suffixmassage_config( &db, c->fname, c->lineno,
							ca.argc, ca.argv );

					} else {
						rc = rwm_rw_config( &db, c->fname, c->lineno,
							ca.argc, ca.argv );
					}

					ch_free( ca.tline );
					ch_free( ca.argv );

					assert( rc == 0 );
				}

			} else if ( rwmap->rwm_rw != NULL ) {
				rewrite_info_delete( &rwmap->rwm_rw );
				assert( rwmap->rwm_rw == NULL );

				ber_bvarray_free( rwmap->rwm_bva_rewrite );
				rwmap->rwm_bva_rewrite = NULL;

				rc = rwm_info_init( &rwmap->rwm_rw );
			}
			break;

		case RWM_CF_T_F_SUPPORT:
			rwmap->rwm_flags &= ~RWM_F_SUPPORT_T_F_MASK2;
			break;

		case RWM_CF_MAP:
			if ( c->valx >= 0 ) {
				struct ldapmap rwm_oc = rwmap->rwm_oc;
				struct ldapmap rwm_at = rwmap->rwm_at;
				char *argv[5];
				int cnt = 0;

				if ( rwmap->rwm_bva_map ) {
					for ( ; !BER_BVISNULL( &rwmap->rwm_bva_map[ cnt ] ); cnt++ )
						/* count */ ;
				}

				if ( c->valx >= cnt ) {
					rc = 1;
					break;
				}

				memset( &rwmap->rwm_oc, 0, sizeof( rwmap->rwm_oc ) );
				memset( &rwmap->rwm_at, 0, sizeof( rwmap->rwm_at ) );

				/* re-parse all mappings except the one
				 * that needs to be eliminated */
				argv[0] = "map";
				for ( cnt = 0; !BER_BVISNULL( &rwmap->rwm_bva_map[ cnt ] ); cnt++ ) {
					ConfigArgs ca = { 0 };

					if ( cnt == c->valx ) {
						continue;
					}

					ca.line = rwmap->rwm_bva_map[ cnt ].bv_val;
					ca.argc = 0;
					config_fp_parse_line( &ca );
					
					argv[1] = ca.argv[0];
					argv[2] = ca.argv[1];
					argv[3] = ca.argv[2];
					argv[4] = ca.argv[3];
			
					rc = rwm_m_config( &db, c->fname, c->lineno, ca.argc + 1, argv );

					ch_free( ca.tline );
					ch_free( ca.argv );

					/* in case of failure, restore
					 * the existing mapping */
					if ( rc ) {
						avl_free( rwmap->rwm_oc.remap, rwm_mapping_dst_free );
						avl_free( rwmap->rwm_oc.map, rwm_mapping_free );
						avl_free( rwmap->rwm_at.remap, rwm_mapping_dst_free );
						avl_free( rwmap->rwm_at.map, rwm_mapping_free );
						rwmap->rwm_oc = rwm_oc;
						rwmap->rwm_at = rwm_at;
						break;
					}
				}

				/* in case of success, destroy the old mapping
				 * and eliminate the deleted one */
				if ( rc == 0 ) {
					avl_free( rwm_oc.remap, rwm_mapping_dst_free );
					avl_free( rwm_oc.map, rwm_mapping_free );
					avl_free( rwm_at.remap, rwm_mapping_dst_free );
					avl_free( rwm_at.map, rwm_mapping_free );

					ber_memfree( rwmap->rwm_bva_map[ c->valx ].bv_val );
					for ( cnt = c->valx; !BER_BVISNULL( &rwmap->rwm_bva_map[ cnt ] ); cnt++ ) {
						rwmap->rwm_bva_map[ cnt ] = rwmap->rwm_bva_map[ cnt + 1 ];
					}
				}

			} else {
				avl_free( rwmap->rwm_oc.remap, rwm_mapping_dst_free );
				avl_free( rwmap->rwm_oc.map, rwm_mapping_free );
				avl_free( rwmap->rwm_at.remap, rwm_mapping_dst_free );
				avl_free( rwmap->rwm_at.map, rwm_mapping_free );

				rwmap->rwm_oc.remap = NULL;
				rwmap->rwm_oc.map = NULL;
				rwmap->rwm_at.remap = NULL;
				rwmap->rwm_at.map = NULL;

				ber_bvarray_free( rwmap->rwm_bva_map );
				rwmap->rwm_bva_map = NULL;
			}
			break;

		case RWM_CF_NORMALIZE_MAPPED:
			rwmap->rwm_flags &= ~RWM_F_NORMALIZE_MAPPED_ATTRS;
			break;

		case RWM_CF_DROP_UNREQUESTED:
			rwmap->rwm_flags &= ~RWM_F_DROP_UNREQUESTED_ATTRS;
			break;

		default:
			return 1;
		}
		return rc;
	}

	if ( strncasecmp( c->argv[ 0 ], "olcRwm", STRLENOF( "olcRwm" ) ) == 0 ) {
		idx0 = 1;
	}

	switch ( c->type ) {
	case RWM_CF_REWRITE:
		if ( c->valx >= 0 ) {
			struct rewrite_info *rwm_rw = rwmap->rwm_rw;
			int i, last;

			for ( last = 0; rwmap->rwm_bva_rewrite && !BER_BVISNULL( &rwmap->rwm_bva_rewrite[ last ] ); last++ )
				/* count'em */ ;

			if ( c->valx > last ) {
				c->valx = last;
			}

			rwmap->rwm_rw = NULL;
			rc = rwm_info_init( &rwmap->rwm_rw );

			for ( i = 0; i < c->valx; i++ ) {
				ConfigArgs ca = { 0 };

				ca.line = rwmap->rwm_bva_rewrite[ i ].bv_val;
				ca.argc = 0;
				config_fp_parse_line( &ca );

				argv0 = ca.argv[ 0 ];
				ca.argv[ 0 ] += STRLENOF( "rwm-" );
				
				if ( strcasecmp( ca.argv[ 0 ], "suffixmassage" ) == 0 ) {
					rc = rwm_suffixmassage_config( &db, c->fname, c->lineno,
						ca.argc, ca.argv );

				} else {
					rc = rwm_rw_config( &db, c->fname, c->lineno,
						ca.argc, ca.argv );
				}

				ca.argv[ 0 ] = argv0;

				ch_free( ca.tline );
				ch_free( ca.argv );

				assert( rc == 0 );
			}

			argv0 = c->argv[ idx0 ];
			if ( strncasecmp( argv0, "rwm-", STRLENOF( "rwm-" ) ) != 0 ) {
				return 1;
			}
			c->argv[ idx0 ] += STRLENOF( "rwm-" );
			if ( strcasecmp( c->argv[ idx0 ], "suffixmassage" ) == 0 ) {
				rc = rwm_suffixmassage_config( &db, c->fname, c->lineno,
					c->argc - idx0, &c->argv[ idx0 ] );

			} else {
				rc = rwm_rw_config( &db, c->fname, c->lineno,
					c->argc - idx0, &c->argv[ idx0 ] );
			}
			c->argv[ idx0 ] = argv0;
			if ( rc != 0 ) {
				rewrite_info_delete( &rwmap->rwm_rw );
				assert( rwmap->rwm_rw == NULL );

				rwmap->rwm_rw = rwm_rw;
				return 1;
			}

			for ( i = c->valx; rwmap->rwm_bva_rewrite && !BER_BVISNULL( &rwmap->rwm_bva_rewrite[ i ] ); i++ )
			{
				ConfigArgs ca = { 0 };

				ca.line = rwmap->rwm_bva_rewrite[ i ].bv_val;
				ca.argc = 0;
				config_fp_parse_line( &ca );
				
				argv0 = ca.argv[ 0 ];
				ca.argv[ 0 ] += STRLENOF( "rwm-" );
				
				if ( strcasecmp( ca.argv[ 0 ], "suffixmassage" ) == 0 ) {
					rc = rwm_suffixmassage_config( &db, c->fname, c->lineno,
						ca.argc, ca.argv );

				} else {
					rc = rwm_rw_config( &db, c->fname, c->lineno,
						ca.argc, ca.argv );
				}

				ca.argv[ 0 ] = argv0;

				ch_free( ca.tline );
				ch_free( ca.argv );

				assert( rc == 0 );
			}

			rwmap->rwm_bva_rewrite = ch_realloc( rwmap->rwm_bva_rewrite,
				( last + 2 )*sizeof( struct berval ) );
			BER_BVZERO( &rwmap->rwm_bva_rewrite[last+1] );

			for ( i = last - 1; i >= c->valx; i-- )
			{
				rwmap->rwm_bva_rewrite[ i + 1 ] = rwmap->rwm_bva_rewrite[ i ];
			}

			rwm_bva_rewrite_add( rwmap, c->valx, &c->argv[ idx0 ] );

			rewrite_info_delete( &rwm_rw );
			assert( rwm_rw == NULL );

			break;
		}

		argv0 = c->argv[ idx0 ];
		if ( strncasecmp( argv0, "rwm-", STRLENOF( "rwm-" ) ) != 0 ) {
			return 1;
		}
		c->argv[ idx0 ] += STRLENOF( "rwm-" );
		if ( strcasecmp( c->argv[ idx0 ], "suffixmassage" ) == 0 ) {
			rc = rwm_suffixmassage_config( &db, c->fname, c->lineno,
				c->argc - idx0, &c->argv[ idx0 ] );

		} else {
			rc = rwm_rw_config( &db, c->fname, c->lineno,
				c->argc - idx0, &c->argv[ idx0 ] );
		}
		c->argv[ idx0 ] = argv0;
		if ( rc ) {
			return 1;

		} else {
			rwm_bva_rewrite_add( rwmap, -1, &c->argv[ idx0 ] );
		}
		break;

	case RWM_CF_T_F_SUPPORT:
		rc = verb_to_mask( c->argv[ 1 ], t_f_mode );
		if ( BER_BVISNULL( &t_f_mode[ rc ].word ) ) {
			return 1;
		}

		rwmap->rwm_flags &= ~RWM_F_SUPPORT_T_F_MASK2;
		rwmap->rwm_flags |= t_f_mode[ rc ].mask;
		rc = 0;
		break;

	case RWM_CF_MAP:
		if ( c->valx >= 0 ) {
			struct ldapmap rwm_oc = rwmap->rwm_oc;
			struct ldapmap rwm_at = rwmap->rwm_at;
			char *argv[5];
			int cnt = 0;

			if ( rwmap->rwm_bva_map ) {
				for ( ; !BER_BVISNULL( &rwmap->rwm_bva_map[ cnt ] ); cnt++ )
					/* count */ ;
			}

			if ( c->valx >= cnt ) {
				c->valx = cnt;
			}

			memset( &rwmap->rwm_oc, 0, sizeof( rwmap->rwm_oc ) );
			memset( &rwmap->rwm_at, 0, sizeof( rwmap->rwm_at ) );

			/* re-parse all mappings, including the one
			 * that needs to be added */
			argv[0] = "map";
			for ( cnt = 0; cnt < c->valx; cnt++ ) {
				ConfigArgs ca = { 0 };

				ca.line = rwmap->rwm_bva_map[ cnt ].bv_val;
				ca.argc = 0;
				config_fp_parse_line( &ca );

				argv[1] = ca.argv[0];
				argv[2] = ca.argv[1];
				argv[3] = ca.argv[2];
				argv[4] = ca.argv[3];
			
				rc = rwm_m_config( &db, c->fname, c->lineno, ca.argc + 1, argv );

				ch_free( ca.tline );
				ch_free( ca.argv );

				/* in case of failure, restore
				 * the existing mapping */
				if ( rc ) {
					goto rwmmap_fail;
				}
			}

			argv0 = c->argv[0];
			c->argv[0] = "map";
			rc = rwm_m_config( &db, c->fname, c->lineno, c->argc, c->argv );
			c->argv[0] = argv0;
			if ( rc ) {
				goto rwmmap_fail;
			}

			if ( rwmap->rwm_bva_map ) {
				for ( ; !BER_BVISNULL( &rwmap->rwm_bva_map[ cnt ] ); cnt++ ) {
					ConfigArgs ca = { 0 };

					ca.line = rwmap->rwm_bva_map[ cnt ].bv_val;
					ca.argc = 0;
					config_fp_parse_line( &ca );
			
					argv[1] = ca.argv[0];
					argv[2] = ca.argv[1];
					argv[3] = ca.argv[2];
					argv[4] = ca.argv[3];
			
					rc = rwm_m_config( &db, c->fname, c->lineno, ca.argc + 1, argv );

					ch_free( ca.tline );
					ch_free( ca.argv );

					/* in case of failure, restore
					 * the existing mapping */
					if ( rc ) {
						goto rwmmap_fail;
					}
				}
			}

			/* in case of success, destroy the old mapping
			 * and add the new one */
			if ( rc == 0 ) {
				BerVarray tmp;
				struct berval bv, *bvp = &bv;

				if ( rwm_bva_add( &bvp, 0, &c->argv[ idx0 ] ) ) {
					rc = 1;
					goto rwmmap_fail;
				}
					
				tmp = ber_memrealloc( rwmap->rwm_bva_map,
					sizeof( struct berval )*( cnt + 2 ) );
				if ( tmp == NULL ) {
					ber_memfree( bv.bv_val );
					rc = 1;
					goto rwmmap_fail;
				}
				rwmap->rwm_bva_map = tmp;
				BER_BVZERO( &rwmap->rwm_bva_map[ cnt + 1 ] );

				avl_free( rwm_oc.remap, rwm_mapping_dst_free );
				avl_free( rwm_oc.map, rwm_mapping_free );
				avl_free( rwm_at.remap, rwm_mapping_dst_free );
				avl_free( rwm_at.map, rwm_mapping_free );

				for ( ; cnt-- > c->valx; ) {
					rwmap->rwm_bva_map[ cnt + 1 ] = rwmap->rwm_bva_map[ cnt ];
				}
				rwmap->rwm_bva_map[ c->valx ] = bv;

			} else {
rwmmap_fail:;
				avl_free( rwmap->rwm_oc.remap, rwm_mapping_dst_free );
				avl_free( rwmap->rwm_oc.map, rwm_mapping_free );
				avl_free( rwmap->rwm_at.remap, rwm_mapping_dst_free );
				avl_free( rwmap->rwm_at.map, rwm_mapping_free );
				rwmap->rwm_oc = rwm_oc;
				rwmap->rwm_at = rwm_at;
			}

			break;
		}

		argv0 = c->argv[ 0 ];
		c->argv[ 0 ] += STRLENOF( "rwm-" );
		rc = rwm_m_config( &db, c->fname, c->lineno, c->argc, c->argv );
		c->argv[ 0 ] = argv0;
		if ( rc ) {
			return 1;

		} else {
			char		*line;
			struct berval	bv;

			line = ldap_charray2str( &c->argv[ 1 ], " " );
			if ( line != NULL ) {
				ber_str2bv( line, 0, 0, &bv );
				ber_bvarray_add( &rwmap->rwm_bva_map, &bv );
			}
		}
		break;

	case RWM_CF_NORMALIZE_MAPPED:
		if ( c->value_int ) {
			rwmap->rwm_flags |= RWM_F_NORMALIZE_MAPPED_ATTRS;
		} else {
			rwmap->rwm_flags &= ~RWM_F_NORMALIZE_MAPPED_ATTRS;
		}
		break;

	case RWM_CF_DROP_UNREQUESTED:
		if ( c->value_int ) {
			rwmap->rwm_flags |= RWM_F_DROP_UNREQUESTED_ATTRS;
		} else {
			rwmap->rwm_flags &= ~RWM_F_DROP_UNREQUESTED_ATTRS;
		}
		break;

	default:
		assert( 0 );
		return 1;
	}

	return rc;
}

static int
rwm_db_init(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst		*on = (slap_overinst *) be->bd_info;
	struct ldaprwmap	*rwmap;
	int			rc = 0;

	rwmap = (struct ldaprwmap *)ch_calloc( 1, sizeof( struct ldaprwmap ) );

	/* default */
	rwmap->rwm_flags = RWM_F_DROP_UNREQUESTED_ATTRS;

	rc = rwm_info_init( &rwmap->rwm_rw );

	on->on_bi.bi_private = (void *)rwmap;

	if ( rc ) {
		(void)rwm_db_destroy( be, NULL );
	}

	return rc;
}

static int
rwm_db_destroy(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst	*on = (slap_overinst *) be->bd_info;
	int		rc = 0;

	if ( on->on_bi.bi_private ) {
		struct ldaprwmap	*rwmap = 
			(struct ldaprwmap *)on->on_bi.bi_private;

		if ( rwmap->rwm_rw ) {
			rewrite_info_delete( &rwmap->rwm_rw );
			if ( rwmap->rwm_bva_rewrite )
				ber_bvarray_free( rwmap->rwm_bva_rewrite );
		}

		avl_free( rwmap->rwm_oc.remap, rwm_mapping_dst_free );
		avl_free( rwmap->rwm_oc.map, rwm_mapping_free );
		avl_free( rwmap->rwm_at.remap, rwm_mapping_dst_free );
		avl_free( rwmap->rwm_at.map, rwm_mapping_free );
		ber_bvarray_free( rwmap->rwm_bva_map );

		ch_free( rwmap );
	}

	return rc;
}

static slap_overinst rwm = { { NULL } };

#if SLAPD_OVER_RWM == SLAPD_MOD_DYNAMIC
static
#endif /* SLAPD_OVER_RWM == SLAPD_MOD_DYNAMIC */
int
rwm_initialize( void )
{
	int		rc;

	/* Make sure we don't exceed the bits reserved for userland */
	config_check_userland( RWM_CF_LAST );

	memset( &rwm, 0, sizeof( slap_overinst ) );

	rwm.on_bi.bi_type = "rwm";
	rwm.on_bi.bi_flags =
		SLAPO_BFLAG_SINGLE |
		0;

	rwm.on_bi.bi_db_init = rwm_db_init;
	rwm.on_bi.bi_db_config = rwm_db_config;
	rwm.on_bi.bi_db_destroy = rwm_db_destroy;

	rwm.on_bi.bi_op_bind = rwm_op_bind;
	rwm.on_bi.bi_op_search = rwm_op_search;
	rwm.on_bi.bi_op_compare = rwm_op_compare;
	rwm.on_bi.bi_op_modify = rwm_op_modify;
	rwm.on_bi.bi_op_modrdn = rwm_op_modrdn;
	rwm.on_bi.bi_op_add = rwm_op_add;
	rwm.on_bi.bi_op_delete = rwm_op_delete;
	rwm.on_bi.bi_op_unbind = rwm_op_unbind;
	rwm.on_bi.bi_extended = rwm_extended;
#if 1 /* TODO */
	rwm.on_bi.bi_entry_release_rw = rwm_entry_release_rw;
	rwm.on_bi.bi_entry_get_rw = rwm_entry_get_rw;
#endif

	rwm.on_bi.bi_operational = rwm_operational;
	rwm.on_bi.bi_chk_referrals = 0 /* rwm_chk_referrals */ ;

	rwm.on_bi.bi_connection_init = rwm_conn_init;
	rwm.on_bi.bi_connection_destroy = rwm_conn_destroy;

	rwm.on_response = rwm_response;

	rwm.on_bi.bi_cf_ocs = rwmocs;

	rc = config_register_schema( rwmcfg, rwmocs );
	if ( rc ) {
		return rc;
	}

	return overlay_register( &rwm );
}

#if SLAPD_OVER_RWM == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return rwm_initialize();
}
#endif /* SLAPD_OVER_RWM == SLAPD_MOD_DYNAMIC */

#endif /* SLAPD_OVER_RWM */
