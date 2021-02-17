/* retcode.c - customizable response for client testing purposes */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
 * Portions Copyright 2005 Pierangelo Masarati <ando@sys-net.it>
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
 */

#include "portable.h"

#ifdef SLAPD_OVER_RETCODE

#include <stdio.h>

#include <ac/unistd.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/socket.h>

#include "slap.h"
#include "config.h"
#include "lutil.h"
#include "ldif.h"

static slap_overinst		retcode;

static AttributeDescription	*ad_errCode;
static AttributeDescription	*ad_errText;
static AttributeDescription	*ad_errOp;
static AttributeDescription	*ad_errSleepTime;
static AttributeDescription	*ad_errMatchedDN;
static AttributeDescription	*ad_errUnsolicitedOID;
static AttributeDescription	*ad_errUnsolicitedData;
static AttributeDescription	*ad_errDisconnect;

static ObjectClass		*oc_errAbsObject;
static ObjectClass		*oc_errObject;
static ObjectClass		*oc_errAuxObject;

typedef enum retcode_op_e {
	SN_DG_OP_NONE		= 0x0000,
	SN_DG_OP_ADD		= 0x0001,
	SN_DG_OP_BIND		= 0x0002,
	SN_DG_OP_COMPARE	= 0x0004,
	SN_DG_OP_DELETE		= 0x0008,
	SN_DG_OP_MODIFY		= 0x0010,
	SN_DG_OP_RENAME		= 0x0020,
	SN_DG_OP_SEARCH		= 0x0040,
	SN_DG_EXTENDED		= 0x0080,
	SN_DG_OP_AUTH		= SN_DG_OP_BIND,
	SN_DG_OP_READ		= (SN_DG_OP_COMPARE|SN_DG_OP_SEARCH),
	SN_DG_OP_WRITE		= (SN_DG_OP_ADD|SN_DG_OP_DELETE|SN_DG_OP_MODIFY|SN_DG_OP_RENAME),
	SN_DG_OP_ALL		= (SN_DG_OP_AUTH|SN_DG_OP_READ|SN_DG_OP_WRITE|SN_DG_EXTENDED)
} retcode_op_e;

typedef struct retcode_item_t {
	struct berval		rdi_line;
	struct berval		rdi_dn;
	struct berval		rdi_ndn;
	struct berval		rdi_text;
	struct berval		rdi_matched;
	int			rdi_err;
	BerVarray		rdi_ref;
	int			rdi_sleeptime;
	Entry			rdi_e;
	slap_mask_t		rdi_mask;
	struct berval		rdi_unsolicited_oid;
	struct berval		rdi_unsolicited_data;

	unsigned		rdi_flags;
#define	RDI_PRE_DISCONNECT	(0x1U)
#define	RDI_POST_DISCONNECT	(0x2U)

	struct retcode_item_t	*rdi_next;
} retcode_item_t;

typedef struct retcode_t {
	struct berval		rd_pdn;
	struct berval		rd_npdn;

	int			rd_sleep;

	retcode_item_t		*rd_item;

	int			rd_indir;
#define	RETCODE_FINDIR		0x01
#define	RETCODE_INDIR( rd )	( (rd)->rd_indir )
} retcode_t;

static int
retcode_entry_response( Operation *op, SlapReply *rs, BackendInfo *bi, Entry *e );

static unsigned int
retcode_sleep( int s )
{
	unsigned int r = 0;

	/* sleep as required */
	if ( s < 0 ) {
#if 0	/* use high-order bits for better randomness (Numerical Recipes in "C") */
		r = rand() % (-s);
#endif
		r = ((double)(-s))*rand()/(RAND_MAX + 1.0);
	} else if ( s > 0 ) {
		r = (unsigned int)s;
	}
	if ( r ) {
		sleep( r );
	}

	return r;
}

static int
retcode_cleanup_cb( Operation *op, SlapReply *rs )
{
	rs->sr_matched = NULL;
	rs->sr_text = NULL;

	if ( rs->sr_ref != NULL ) {
		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
	}

	ch_free( op->o_callback );
	op->o_callback = NULL;

	return SLAP_CB_CONTINUE;
}

static int
retcode_send_onelevel( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	retcode_t	*rd = (retcode_t *)on->on_bi.bi_private;

	retcode_item_t	*rdi;
	
	for ( rdi = rd->rd_item; rdi != NULL; rdi = rdi->rdi_next ) {
		if ( op->o_abandon ) {
			return rs->sr_err = SLAPD_ABANDON;
		}

		rs->sr_err = test_filter( op, &rdi->rdi_e, op->ors_filter );
		if ( rs->sr_err == LDAP_COMPARE_TRUE ) {
			/* safe default */
			rs->sr_attrs = op->ors_attrs;
			rs->sr_operational_attrs = NULL;
			rs->sr_ctrls = NULL;
			rs->sr_flags = 0;
			rs->sr_err = LDAP_SUCCESS;
			rs->sr_entry = &rdi->rdi_e;

			rs->sr_err = send_search_entry( op, rs );
			rs->sr_flags = 0;
			rs->sr_entry = NULL;
			rs->sr_attrs = NULL;

			switch ( rs->sr_err ) {
			case LDAP_UNAVAILABLE:	/* connection closed */
				rs->sr_err = LDAP_OTHER;
				/* fallthru */
			case LDAP_SIZELIMIT_EXCEEDED:
				goto done;
			}
		}
		rs->sr_err = LDAP_SUCCESS;
	}

done:;

	send_ldap_result( op, rs );

	return rs->sr_err;
}

static int
retcode_op_add( Operation *op, SlapReply *rs )
{
	return retcode_entry_response( op, rs, NULL, op->ora_e );
}

typedef struct retcode_cb_t {
	BackendInfo	*rdc_info;
	unsigned	rdc_flags;
	ber_tag_t	rdc_tag;
	AttributeName	*rdc_attrs;
} retcode_cb_t;

static int
retcode_cb_response( Operation *op, SlapReply *rs )
{
	retcode_cb_t	*rdc = (retcode_cb_t *)op->o_callback->sc_private;

	op->o_tag = rdc->rdc_tag;
	if ( rs->sr_type == REP_SEARCH ) {
		ber_tag_t	o_tag = op->o_tag;
		int		rc;

		if ( op->o_tag == LDAP_REQ_SEARCH ) {
			rs->sr_attrs = rdc->rdc_attrs;
		}
		rc = retcode_entry_response( op, rs, rdc->rdc_info, rs->sr_entry );
		op->o_tag = o_tag;

		return rc;
	}

	switch ( rs->sr_err ) {
	case LDAP_SUCCESS:
	case LDAP_NO_SUCH_OBJECT:
		/* in case of noSuchObject, stop the internal search
		 * for in-directory error stuff */
		if ( !op->o_abandon ) {
			rdc->rdc_flags = SLAP_CB_CONTINUE;
		}
		return 0;
	}

	return SLAP_CB_CONTINUE;
}

static int
retcode_op_internal( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;

	Operation	op2 = *op;
	BackendDB	db = *op->o_bd;
	slap_callback	sc = { 0 };
	retcode_cb_t	rdc;

	int		rc;

	op2.o_tag = LDAP_REQ_SEARCH;
	op2.ors_scope = LDAP_SCOPE_BASE;
	op2.ors_deref = LDAP_DEREF_NEVER;
	op2.ors_tlimit = SLAP_NO_LIMIT;
	op2.ors_slimit = SLAP_NO_LIMIT;
	op2.ors_limit = NULL;
	op2.ors_attrsonly = 0;
	op2.ors_attrs = slap_anlist_all_attributes;

	ber_str2bv_x( "(objectClass=errAbsObject)",
		STRLENOF( "(objectClass=errAbsObject)" ),
		1, &op2.ors_filterstr, op2.o_tmpmemctx );
	op2.ors_filter = str2filter_x( &op2, op2.ors_filterstr.bv_val );

	/* errAbsObject is defined by this overlay! */
	assert( op2.ors_filter != NULL );

	db.bd_info = on->on_info->oi_orig;
	op2.o_bd = &db;

	rdc.rdc_info = on->on_info->oi_orig;
	rdc.rdc_flags = RETCODE_FINDIR;
	if ( op->o_tag == LDAP_REQ_SEARCH ) {
		rdc.rdc_attrs = op->ors_attrs;
	}
	rdc.rdc_tag = op->o_tag;
	sc.sc_response = retcode_cb_response;
	sc.sc_private = &rdc;
	op2.o_callback = &sc;

	rc = op2.o_bd->be_search( &op2, rs );
	op->o_abandon = op2.o_abandon;

	filter_free_x( &op2, op2.ors_filter, 1 );
	ber_memfree_x( op2.ors_filterstr.bv_val, op2.o_tmpmemctx );

	if ( rdc.rdc_flags == SLAP_CB_CONTINUE ) {
		return SLAP_CB_CONTINUE;
	}

	return rc;
}

static int
retcode_op_func( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	retcode_t	*rd = (retcode_t *)on->on_bi.bi_private;

	retcode_item_t	*rdi;
	struct berval		nrdn, npdn;

	slap_callback		*cb = NULL;

	/* sleep as required */
	retcode_sleep( rd->rd_sleep );

	if ( !dnIsSuffix( &op->o_req_ndn, &rd->rd_npdn ) ) {
		if ( RETCODE_INDIR( rd ) ) {
			switch ( op->o_tag ) {
			case LDAP_REQ_ADD:
				return retcode_op_add( op, rs );

			case LDAP_REQ_BIND:
				/* skip if rootdn */
				/* FIXME: better give the db a chance? */
				if ( be_isroot_pw( op ) ) {
					return LDAP_SUCCESS;
				}
				return retcode_op_internal( op, rs );

			case LDAP_REQ_SEARCH:
				if ( op->ors_scope == LDAP_SCOPE_BASE ) {
					rs->sr_err = retcode_op_internal( op, rs );
					switch ( rs->sr_err ) {
					case SLAP_CB_CONTINUE:
						if ( rs->sr_nentries == 0 ) {
							break;
						}
						rs->sr_err = LDAP_SUCCESS;
						/* fallthru */

					default:
						send_ldap_result( op, rs );
						break;
					}
					return rs->sr_err;
				}
				break;

			case LDAP_REQ_MODIFY:
			case LDAP_REQ_DELETE:
			case LDAP_REQ_MODRDN:
			case LDAP_REQ_COMPARE:
				return retcode_op_internal( op, rs );
			}
		}

		return SLAP_CB_CONTINUE;
	}

	if ( op->o_tag == LDAP_REQ_SEARCH
			&& op->ors_scope != LDAP_SCOPE_BASE
			&& op->o_req_ndn.bv_len == rd->rd_npdn.bv_len )
	{
		return retcode_send_onelevel( op, rs );
	}

	dnParent( &op->o_req_ndn, &npdn );
	if ( npdn.bv_len != rd->rd_npdn.bv_len ) {
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		rs->sr_matched = rd->rd_pdn.bv_val;
		send_ldap_result( op, rs );
		rs->sr_matched = NULL;
		return rs->sr_err;
	}

	dnRdn( &op->o_req_ndn, &nrdn );

	for ( rdi = rd->rd_item; rdi != NULL; rdi = rdi->rdi_next ) {
		struct berval	rdi_nrdn;

		dnRdn( &rdi->rdi_ndn, &rdi_nrdn );
		if ( dn_match( &nrdn, &rdi_nrdn ) ) {
			break;
		}
	}

	if ( rdi != NULL && rdi->rdi_mask != SN_DG_OP_ALL ) {
		retcode_op_e	o_tag = SN_DG_OP_NONE;

		switch ( op->o_tag ) {
		case LDAP_REQ_ADD:
			o_tag = SN_DG_OP_ADD;
			break;

		case LDAP_REQ_BIND:
			o_tag = SN_DG_OP_BIND;
			break;

		case LDAP_REQ_COMPARE:
			o_tag = SN_DG_OP_COMPARE;
			break;

		case LDAP_REQ_DELETE:
			o_tag = SN_DG_OP_DELETE;
			break;

		case LDAP_REQ_MODIFY:
			o_tag = SN_DG_OP_MODIFY;
			break;

		case LDAP_REQ_MODRDN:
			o_tag = SN_DG_OP_RENAME;
			break;

		case LDAP_REQ_SEARCH:
			o_tag = SN_DG_OP_SEARCH;
			break;

		case LDAP_REQ_EXTENDED:
			o_tag = SN_DG_EXTENDED;
			break;

		default:
			/* Should not happen */
			break;
		}

		if ( !( o_tag & rdi->rdi_mask ) ) {
			return SLAP_CB_CONTINUE;
		}
	}

	if ( rdi == NULL ) {
		rs->sr_matched = rd->rd_pdn.bv_val;
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		rs->sr_text = "retcode not found";

	} else {
		if ( rdi->rdi_flags & RDI_PRE_DISCONNECT ) {
			return rs->sr_err = SLAPD_DISCONNECT;
		}

		rs->sr_err = rdi->rdi_err;
		rs->sr_text = rdi->rdi_text.bv_val;
		rs->sr_matched = rdi->rdi_matched.bv_val;

		/* FIXME: we only honor the rdi_ref field in case rdi_err
		 * is LDAP_REFERRAL otherwise send_ldap_result() bails out */
		if ( rs->sr_err == LDAP_REFERRAL ) {
			BerVarray	ref;

			if ( rdi->rdi_ref != NULL ) {
				ref = rdi->rdi_ref;
			} else {
				ref = default_referral;
			}

			if ( ref != NULL ) {
				rs->sr_ref = referral_rewrite( ref,
					NULL, &op->o_req_dn, LDAP_SCOPE_DEFAULT );

			} else {
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "bad referral object";
			}
		}

		retcode_sleep( rdi->rdi_sleeptime );
	}

	switch ( op->o_tag ) {
	case LDAP_REQ_EXTENDED:
		if ( rdi == NULL ) {
			break;
		}
		cb = ( slap_callback * )ch_malloc( sizeof( slap_callback ) );
		memset( cb, 0, sizeof( slap_callback ) );
		cb->sc_cleanup = retcode_cleanup_cb;
		op->o_callback = cb;
		break;

	default:
		if ( rdi && !BER_BVISNULL( &rdi->rdi_unsolicited_oid ) ) {
			ber_int_t	msgid = op->o_msgid;

			/* RFC 4511 unsolicited response */

			op->o_msgid = 0;
			if ( strcmp( rdi->rdi_unsolicited_oid.bv_val, "0" ) == 0 ) {
				send_ldap_result( op, rs );

			} else {
				ber_tag_t	tag = op->o_tag;

				op->o_tag = LDAP_REQ_EXTENDED;
				rs->sr_rspoid = rdi->rdi_unsolicited_oid.bv_val;
				if ( !BER_BVISNULL( &rdi->rdi_unsolicited_data ) ) {
					rs->sr_rspdata = &rdi->rdi_unsolicited_data;
				}
				send_ldap_extended( op, rs );
				rs->sr_rspoid = NULL;
				rs->sr_rspdata = NULL;
				op->o_tag = tag;

			}
			op->o_msgid = msgid;

		} else {
			send_ldap_result( op, rs );
		}

		if ( rs->sr_ref != NULL ) {
			ber_bvarray_free( rs->sr_ref );
			rs->sr_ref = NULL;
		}
		rs->sr_matched = NULL;
		rs->sr_text = NULL;

		if ( rdi && rdi->rdi_flags & RDI_POST_DISCONNECT ) {
			return rs->sr_err = SLAPD_DISCONNECT;
		}
		break;
	}

	return rs->sr_err;
}

static int
retcode_op2str( ber_tag_t op, struct berval *bv )
{
	switch ( op ) {
	case LDAP_REQ_BIND:
		BER_BVSTR( bv, "bind" );
		return 0;
	case LDAP_REQ_ADD:
		BER_BVSTR( bv, "add" );
		return 0;
	case LDAP_REQ_DELETE:
		BER_BVSTR( bv, "delete" );
		return 0;
	case LDAP_REQ_MODRDN:
		BER_BVSTR( bv, "modrdn" );
		return 0;
	case LDAP_REQ_MODIFY:
		BER_BVSTR( bv, "modify" );
		return 0;
	case LDAP_REQ_COMPARE:
		BER_BVSTR( bv, "compare" );
		return 0;
	case LDAP_REQ_SEARCH:
		BER_BVSTR( bv, "search" );
		return 0;
	case LDAP_REQ_EXTENDED:
		BER_BVSTR( bv, "extended" );
		return 0;
	}
	return -1;
}

static int
retcode_entry_response( Operation *op, SlapReply *rs, BackendInfo *bi, Entry *e )
{
	Attribute	*a;
	int		err;
	char		*next;
	int		disconnect = 0;

	if ( get_manageDSAit( op ) ) {
		return SLAP_CB_CONTINUE;
	}

	if ( !is_entry_objectclass_or_sub( e, oc_errAbsObject ) ) {
		return SLAP_CB_CONTINUE;
	}

	/* operation */
	a = attr_find( e->e_attrs, ad_errOp );
	if ( a != NULL ) {
		int		i,
				gotit = 0;
		struct berval	bv = BER_BVNULL;

		(void)retcode_op2str( op->o_tag, &bv );

		if ( BER_BVISNULL( &bv ) ) {
			return SLAP_CB_CONTINUE;
		}

		for ( i = 0; !BER_BVISNULL( &a->a_nvals[ i ] ); i++ ) {
			if ( bvmatch( &a->a_nvals[ i ], &bv ) ) {
				gotit = 1;
				break;
			}
		}

		if ( !gotit ) {
			return SLAP_CB_CONTINUE;
		}
	}

	/* disconnect */
	a = attr_find( e->e_attrs, ad_errDisconnect );
	if ( a != NULL ) {
		if ( bvmatch( &a->a_nvals[ 0 ], &slap_true_bv ) ) {
			return rs->sr_err = SLAPD_DISCONNECT;
		}
		disconnect = 1;
	}

	/* error code */
	a = attr_find( e->e_attrs, ad_errCode );
	if ( a == NULL ) {
		return SLAP_CB_CONTINUE;
	}
	err = strtol( a->a_nvals[ 0 ].bv_val, &next, 0 );
	if ( next == a->a_nvals[ 0 ].bv_val || next[ 0 ] != '\0' ) {
		return SLAP_CB_CONTINUE;
	}
	rs->sr_err = err;

	/* sleep time */
	a = attr_find( e->e_attrs, ad_errSleepTime );
	if ( a != NULL && a->a_nvals[ 0 ].bv_val[ 0 ] != '-' ) {
		int	sleepTime;

		if ( lutil_atoi( &sleepTime, a->a_nvals[ 0 ].bv_val ) == 0 ) {
			retcode_sleep( sleepTime );
		}
	}

	if ( rs->sr_err != LDAP_SUCCESS && !LDAP_API_ERROR( rs->sr_err )) {
		BackendDB	db = *op->o_bd,
				*o_bd = op->o_bd;
		void		*o_callback = op->o_callback;

		/* message text */
		a = attr_find( e->e_attrs, ad_errText );
		if ( a != NULL ) {
			rs->sr_text = a->a_vals[ 0 ].bv_val;
		}

		/* matched DN */
		a = attr_find( e->e_attrs, ad_errMatchedDN );
		if ( a != NULL ) {
			rs->sr_matched = a->a_vals[ 0 ].bv_val;
		}

		if ( bi == NULL ) {
			slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;

			bi = on->on_info->oi_orig;
		}

		db.bd_info = bi;
		op->o_bd = &db;
		op->o_callback = NULL;

		/* referral */
		if ( rs->sr_err == LDAP_REFERRAL ) {
			BerVarray	refs = default_referral;

			a = attr_find( e->e_attrs, slap_schema.si_ad_ref );
			if ( a != NULL ) {
				refs = a->a_vals;
			}
			rs->sr_ref = referral_rewrite( refs,
				NULL, &op->o_req_dn, op->oq_search.rs_scope );
	
			send_search_reference( op, rs );
			ber_bvarray_free( rs->sr_ref );
			rs->sr_ref = NULL;

		} else {
			a = attr_find( e->e_attrs, ad_errUnsolicitedOID );
			if ( a != NULL ) {
				struct berval	oid = BER_BVNULL,
						data = BER_BVNULL;
				ber_int_t	msgid = op->o_msgid;

				/* RFC 4511 unsolicited response */

				op->o_msgid = 0;

				oid = a->a_nvals[ 0 ];

				a = attr_find( e->e_attrs, ad_errUnsolicitedData );
				if ( a != NULL ) {
					data = a->a_nvals[ 0 ];
				}

				if ( strcmp( oid.bv_val, "0" ) == 0 ) {
					send_ldap_result( op, rs );

				} else {
					ber_tag_t	tag = op->o_tag;

					op->o_tag = LDAP_REQ_EXTENDED;
					rs->sr_rspoid = oid.bv_val;
					if ( !BER_BVISNULL( &data ) ) {
						rs->sr_rspdata = &data;
					}
					send_ldap_extended( op, rs );
					rs->sr_rspoid = NULL;
					rs->sr_rspdata = NULL;
					op->o_tag = tag;
				}
				op->o_msgid = msgid;

			} else {
				send_ldap_result( op, rs );
			}
		}

		rs->sr_text = NULL;
		rs->sr_matched = NULL;
		op->o_bd = o_bd;
		op->o_callback = o_callback;
	}

	if ( rs->sr_err != LDAP_SUCCESS ) {
		if ( disconnect ) {
			return rs->sr_err = SLAPD_DISCONNECT;
		}
	
		op->o_abandon = 1;
		return rs->sr_err;
	}

	return SLAP_CB_CONTINUE;
}

static int
retcode_response( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	retcode_t	*rd = (retcode_t *)on->on_bi.bi_private;

	if ( rs->sr_type != REP_SEARCH || !RETCODE_INDIR( rd ) ) {
		return SLAP_CB_CONTINUE;
	}

	return retcode_entry_response( op, rs, NULL, rs->sr_entry );
}

static int
retcode_db_init( BackendDB *be, ConfigReply *cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	retcode_t	*rd;

	srand( getpid() );

	rd = (retcode_t *)ch_malloc( sizeof( retcode_t ) );
	memset( rd, 0, sizeof( retcode_t ) );

	on->on_bi.bi_private = (void *)rd;

	return 0;
}

static void
retcode_item_destroy( retcode_item_t *rdi )
{
	ber_memfree( rdi->rdi_line.bv_val );

	ber_memfree( rdi->rdi_dn.bv_val );
	ber_memfree( rdi->rdi_ndn.bv_val );

	if ( !BER_BVISNULL( &rdi->rdi_text ) ) {
		ber_memfree( rdi->rdi_text.bv_val );
	}

	if ( !BER_BVISNULL( &rdi->rdi_matched ) ) {
		ber_memfree( rdi->rdi_matched.bv_val );
	}

	if ( rdi->rdi_ref ) {
		ber_bvarray_free( rdi->rdi_ref );
	}

	BER_BVZERO( &rdi->rdi_e.e_name );
	BER_BVZERO( &rdi->rdi_e.e_nname );

	entry_clean( &rdi->rdi_e );

	if ( !BER_BVISNULL( &rdi->rdi_unsolicited_oid ) ) {
		ber_memfree( rdi->rdi_unsolicited_oid.bv_val );
		if ( !BER_BVISNULL( &rdi->rdi_unsolicited_data ) )
			ber_memfree( rdi->rdi_unsolicited_data.bv_val );
	}

	ch_free( rdi );
}

enum {
	RC_PARENT = 1,
	RC_ITEM
};

static ConfigDriver rc_cf_gen;

static ConfigTable rccfg[] = {
	{ "retcode-parent", "dn",
		2, 2, 0, ARG_MAGIC|ARG_DN|RC_PARENT, rc_cf_gen,
		"( OLcfgOvAt:20.1 NAME 'olcRetcodeParent' "
			"DESC '' "
			"SYNTAX OMsDN SINGLE-VALUE )", NULL, NULL },
	{ "retcode-item", "rdn> <retcode> <...",
		3, 0, 0, ARG_MAGIC|RC_ITEM, rc_cf_gen,
		"( OLcfgOvAt:20.2 NAME 'olcRetcodeItem' "
			"DESC '' "
	  		"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )", NULL, NULL },
	{ "retcode-indir", "on|off",
		1, 2, 0, ARG_OFFSET|ARG_ON_OFF,
			(void *)offsetof(retcode_t, rd_indir),
		"( OLcfgOvAt:20.3 NAME 'olcRetcodeInDir' "
			"DESC '' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },

	{ "retcode-sleep", "sleeptime",
		2, 2, 0, ARG_OFFSET|ARG_INT,
			(void *)offsetof(retcode_t, rd_sleep),
		"( OLcfgOvAt:20.4 NAME 'olcRetcodeSleep' "
			"DESC '' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs rcocs[] = {
	{ "( OLcfgOvOc:20.1 "
		"NAME 'olcRetcodeConfig' "
		"DESC 'Retcode configuration' "
		"SUP olcOverlayConfig "
		"MAY ( olcRetcodeParent "
			"$ olcRetcodeItem "
			"$ olcRetcodeInDir "
			"$ olcRetcodeSleep "
		") )",
		Cft_Overlay, rccfg, NULL, NULL },
	{ NULL, 0, NULL }
};

static int
rc_cf_gen( ConfigArgs *c )
{
	slap_overinst	*on = (slap_overinst *)c->bi;
	retcode_t	*rd = (retcode_t *)on->on_bi.bi_private;
	int		rc = ARG_BAD_CONF;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch( c->type ) {
		case RC_PARENT:
			if ( !BER_BVISEMPTY( &rd->rd_pdn )) {
				rc = value_add_one( &c->rvalue_vals,
						    &rd->rd_pdn );
				if ( rc == 0 ) {
					rc = value_add_one( &c->rvalue_nvals,
							    &rd->rd_npdn );
				}
				return rc;
			}
			rc = 0;
			break;

		case RC_ITEM: {
			retcode_item_t *rdi;
			int i;

			for ( rdi = rd->rd_item, i = 0; rdi; rdi = rdi->rdi_next, i++ ) {
				char buf[4096];
				struct berval bv;
				char *ptr;

				bv.bv_len = snprintf( buf, sizeof( buf ), SLAP_X_ORDERED_FMT, i );
				bv.bv_len += rdi->rdi_line.bv_len;
				ptr = bv.bv_val = ch_malloc( bv.bv_len + 1 );
				ptr = lutil_strcopy( ptr, buf );
				ptr = lutil_strncopy( ptr, rdi->rdi_line.bv_val, rdi->rdi_line.bv_len );
				ber_bvarray_add( &c->rvalue_vals, &bv );
			}
			rc = 0;
			} break;

		default:
			assert( 0 );
			break;
		}

		return rc;

	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch( c->type ) {
		case RC_PARENT:
			if ( rd->rd_pdn.bv_val ) {
				ber_memfree ( rd->rd_pdn.bv_val );
				rc = 0;
			}
			if ( rd->rd_npdn.bv_val ) {
				ber_memfree ( rd->rd_npdn.bv_val );
			}
			break;

		case RC_ITEM:
			if ( c->valx == -1 ) {
				retcode_item_t *rdi, *next;

				for ( rdi = rd->rd_item; rdi != NULL; rdi = next ) {
					next = rdi->rdi_next;
					retcode_item_destroy( rdi );
				}

			} else {
				retcode_item_t **rdip, *rdi;
				int i;

				for ( rdip = &rd->rd_item, i = 0; i <= c->valx && *rdip; i++, rdip = &(*rdip)->rdi_next )
					;
				if ( *rdip == NULL ) {
					return 1;
				}
				rdi = *rdip;
				*rdip = rdi->rdi_next;

				retcode_item_destroy( rdi );
			}
			rc = 0;
			break;

		default:
			assert( 0 );
			break;
		}
		return rc;	/* FIXME */
	}

	switch( c->type ) {
	case RC_PARENT:
		if ( rd->rd_pdn.bv_val ) {
			ber_memfree ( rd->rd_pdn.bv_val );
		}
		if ( rd->rd_npdn.bv_val ) {
			ber_memfree ( rd->rd_npdn.bv_val );
		}
		rd->rd_pdn = c->value_dn;
		rd->rd_npdn = c->value_ndn;
		rc = 0;
		break;

	case RC_ITEM: {
		retcode_item_t	rdi = { BER_BVNULL }, **rdip;
		struct berval		bv, rdn, nrdn;
		char			*next = NULL;
		int			i;

		if ( c->argc < 3 ) {
			snprintf( c->cr_msg, sizeof(c->cr_msg),
				"\"retcode-item <RDN> <retcode> [<text>]\": "
				"missing args" );
			Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
				c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}

		ber_str2bv( c->argv[ 1 ], 0, 0, &bv );
		
		rc = dnPrettyNormal( NULL, &bv, &rdn, &nrdn, NULL );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( c->cr_msg, sizeof(c->cr_msg),
				"unable to normalize RDN \"%s\": %d",
				c->argv[ 1 ], rc );
			Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
				c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}

		if ( !dnIsOneLevelRDN( &nrdn ) ) {
			snprintf( c->cr_msg, sizeof(c->cr_msg),
				"value \"%s\" is not a RDN",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
				c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}

		if ( BER_BVISNULL( &rd->rd_npdn ) ) {
			/* FIXME: we use the database suffix */
			if ( c->be->be_nsuffix == NULL ) {
				snprintf( c->cr_msg, sizeof(c->cr_msg),
					"either \"retcode-parent\" "
					"or \"suffix\" must be defined" );
				Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
					c->log, c->cr_msg, 0 );
				return ARG_BAD_CONF;
			}

			ber_dupbv( &rd->rd_pdn, &c->be->be_suffix[ 0 ] );
			ber_dupbv( &rd->rd_npdn, &c->be->be_nsuffix[ 0 ] );
		}

		build_new_dn( &rdi.rdi_dn, &rd->rd_pdn, &rdn, NULL );
		build_new_dn( &rdi.rdi_ndn, &rd->rd_npdn, &nrdn, NULL );

		ch_free( rdn.bv_val );
		ch_free( nrdn.bv_val );

		rdi.rdi_err = strtol( c->argv[ 2 ], &next, 0 );
		if ( next == c->argv[ 2 ] || next[ 0 ] != '\0' ) {
			snprintf( c->cr_msg, sizeof(c->cr_msg),
				"unable to parse return code \"%s\"",
				c->argv[ 2 ] );
			Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
				c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}

		rdi.rdi_mask = SN_DG_OP_ALL;

		if ( c->argc > 3 ) {
			for ( i = 3; i < c->argc; i++ ) {
				if ( strncasecmp( c->argv[ i ], "op=", STRLENOF( "op=" ) ) == 0 )
				{
					char		**ops;
					int		j;

					ops = ldap_str2charray( &c->argv[ i ][ STRLENOF( "op=" ) ], "," );
					assert( ops != NULL );

					rdi.rdi_mask = SN_DG_OP_NONE;

					for ( j = 0; ops[ j ] != NULL; j++ ) {
						if ( strcasecmp( ops[ j ], "add" ) == 0 ) {
							rdi.rdi_mask |= SN_DG_OP_ADD;

						} else if ( strcasecmp( ops[ j ], "bind" ) == 0 ) {
							rdi.rdi_mask |= SN_DG_OP_BIND;

						} else if ( strcasecmp( ops[ j ], "compare" ) == 0 ) {
							rdi.rdi_mask |= SN_DG_OP_COMPARE;

						} else if ( strcasecmp( ops[ j ], "delete" ) == 0 ) {
							rdi.rdi_mask |= SN_DG_OP_DELETE;

						} else if ( strcasecmp( ops[ j ], "modify" ) == 0 ) {
							rdi.rdi_mask |= SN_DG_OP_MODIFY;

						} else if ( strcasecmp( ops[ j ], "rename" ) == 0
							|| strcasecmp( ops[ j ], "modrdn" ) == 0 )
						{
							rdi.rdi_mask |= SN_DG_OP_RENAME;

						} else if ( strcasecmp( ops[ j ], "search" ) == 0 ) {
							rdi.rdi_mask |= SN_DG_OP_SEARCH;

						} else if ( strcasecmp( ops[ j ], "extended" ) == 0 ) {
							rdi.rdi_mask |= SN_DG_EXTENDED;

						} else if ( strcasecmp( ops[ j ], "auth" ) == 0 ) {
							rdi.rdi_mask |= SN_DG_OP_AUTH;

						} else if ( strcasecmp( ops[ j ], "read" ) == 0 ) {
							rdi.rdi_mask |= SN_DG_OP_READ;

						} else if ( strcasecmp( ops[ j ], "write" ) == 0 ) {
							rdi.rdi_mask |= SN_DG_OP_WRITE;

						} else if ( strcasecmp( ops[ j ], "all" ) == 0 ) {
							rdi.rdi_mask |= SN_DG_OP_ALL;

						} else {
							snprintf( c->cr_msg, sizeof(c->cr_msg),
								"unknown op \"%s\"",
								ops[ j ] );
							ldap_charray_free( ops );
							Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
								c->log, c->cr_msg, 0 );
							return ARG_BAD_CONF;
						}
					}

					ldap_charray_free( ops );

				} else if ( strncasecmp( c->argv[ i ], "text=", STRLENOF( "text=" ) ) == 0 )
				{
					if ( !BER_BVISNULL( &rdi.rdi_text ) ) {
						snprintf( c->cr_msg, sizeof(c->cr_msg),
							"\"text\" already provided" );
						Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
							c->log, c->cr_msg, 0 );
						return ARG_BAD_CONF;
					}
					ber_str2bv( &c->argv[ i ][ STRLENOF( "text=" ) ], 0, 1, &rdi.rdi_text );

				} else if ( strncasecmp( c->argv[ i ], "matched=", STRLENOF( "matched=" ) ) == 0 )
				{
					struct berval	dn;

					if ( !BER_BVISNULL( &rdi.rdi_matched ) ) {
						snprintf( c->cr_msg, sizeof(c->cr_msg),
							"\"matched\" already provided" );
						Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
							c->log, c->cr_msg, 0 );
						return ARG_BAD_CONF;
					}
					ber_str2bv( &c->argv[ i ][ STRLENOF( "matched=" ) ], 0, 0, &dn );
					if ( dnPretty( NULL, &dn, &rdi.rdi_matched, NULL ) != LDAP_SUCCESS ) {
						snprintf( c->cr_msg, sizeof(c->cr_msg),
							"unable to prettify matched DN \"%s\"",
							&c->argv[ i ][ STRLENOF( "matched=" ) ] );
						Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
							c->log, c->cr_msg, 0 );
						return ARG_BAD_CONF;
					}

				} else if ( strncasecmp( c->argv[ i ], "ref=", STRLENOF( "ref=" ) ) == 0 )
				{
					char		**refs;
					int		j;

					if ( rdi.rdi_ref != NULL ) {
						snprintf( c->cr_msg, sizeof(c->cr_msg),
							"\"ref\" already provided" );
						Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
							c->log, c->cr_msg, 0 );
						return ARG_BAD_CONF;
					}

					if ( rdi.rdi_err != LDAP_REFERRAL ) {
						snprintf( c->cr_msg, sizeof(c->cr_msg),
							"providing \"ref\" "
							"along with a non-referral "
							"resultCode may cause slapd failures "
							"related to internal checks" );
						Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
							c->log, c->cr_msg, 0 );
					}

					refs = ldap_str2charray( &c->argv[ i ][ STRLENOF( "ref=" ) ], " " );
					assert( refs != NULL );

					for ( j = 0; refs[ j ] != NULL; j++ ) {
						struct berval	bv;

						ber_str2bv( refs[ j ], 0, 1, &bv );
						ber_bvarray_add( &rdi.rdi_ref, &bv );
					}

					ldap_charray_free( refs );

				} else if ( strncasecmp( c->argv[ i ], "sleeptime=", STRLENOF( "sleeptime=" ) ) == 0 )
				{
					if ( rdi.rdi_sleeptime != 0 ) {
						snprintf( c->cr_msg, sizeof(c->cr_msg),
							"\"sleeptime\" already provided" );
						Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
							c->log, c->cr_msg, 0 );
						return ARG_BAD_CONF;
					}

					if ( lutil_atoi( &rdi.rdi_sleeptime, &c->argv[ i ][ STRLENOF( "sleeptime=" ) ] ) ) {
						snprintf( c->cr_msg, sizeof(c->cr_msg),
							"unable to parse \"sleeptime=%s\"",
							&c->argv[ i ][ STRLENOF( "sleeptime=" ) ] );
						Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
							c->log, c->cr_msg, 0 );
						return ARG_BAD_CONF;
					}

				} else if ( strncasecmp( c->argv[ i ], "unsolicited=", STRLENOF( "unsolicited=" ) ) == 0 )
				{
					char		*data;

					if ( !BER_BVISNULL( &rdi.rdi_unsolicited_oid ) ) {
						snprintf( c->cr_msg, sizeof(c->cr_msg),
							"\"unsolicited\" already provided" );
						Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
							c->log, c->cr_msg, 0 );
						return ARG_BAD_CONF;
					}

					data = strchr( &c->argv[ i ][ STRLENOF( "unsolicited=" ) ], ':' );
					if ( data != NULL ) {
						struct berval	oid;

						if ( ldif_parse_line2( &c->argv[ i ][ STRLENOF( "unsolicited=" ) ],
							&oid, &rdi.rdi_unsolicited_data, NULL ) )
						{
							snprintf( c->cr_msg, sizeof(c->cr_msg),
								"unable to parse \"unsolicited\"" );
							Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
								c->log, c->cr_msg, 0 );
							return ARG_BAD_CONF;
						}

						ber_dupbv( &rdi.rdi_unsolicited_oid, &oid );

					} else {
						ber_str2bv( &c->argv[ i ][ STRLENOF( "unsolicited=" ) ], 0, 1,
							&rdi.rdi_unsolicited_oid );
					}

				} else if ( strncasecmp( c->argv[ i ], "flags=", STRLENOF( "flags=" ) ) == 0 )
				{
					char *arg = &c->argv[ i ][ STRLENOF( "flags=" ) ];
					if ( strcasecmp( arg, "disconnect" ) == 0 ) {
						rdi.rdi_flags |= RDI_PRE_DISCONNECT;

					} else if ( strcasecmp( arg, "pre-disconnect" ) == 0 ) {
						rdi.rdi_flags |= RDI_PRE_DISCONNECT;

					} else if ( strcasecmp( arg, "post-disconnect" ) == 0 ) {
						rdi.rdi_flags |= RDI_POST_DISCONNECT;

					} else {
						snprintf( c->cr_msg, sizeof(c->cr_msg),
							"unknown flag \"%s\"", arg );
						Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
							c->log, c->cr_msg, 0 );
						return ARG_BAD_CONF;
					}

				} else {
					snprintf( c->cr_msg, sizeof(c->cr_msg),
						"unknown option \"%s\"",
						c->argv[ i ] );
					Debug( LDAP_DEBUG_CONFIG, "%s: retcode: %s\n",
						c->log, c->cr_msg, 0 );
					return ARG_BAD_CONF;
				}
			}
		}

		rdi.rdi_line.bv_len = 2*(c->argc - 1) + c->argc - 2;
		for ( i = 1; i < c->argc; i++ ) {
			rdi.rdi_line.bv_len += strlen( c->argv[ i ] );
		}
		next = rdi.rdi_line.bv_val = ch_malloc( rdi.rdi_line.bv_len + 1 );

		for ( i = 1; i < c->argc; i++ ) {
			*next++ = '"';
			next = lutil_strcopy( next, c->argv[ i ] );
			*next++ = '"';
			*next++ = ' ';
		}
		*--next = '\0';
		
		for ( rdip = &rd->rd_item; *rdip; rdip = &(*rdip)->rdi_next )
			/* go to last */ ;

		
		*rdip = ( retcode_item_t * )ch_malloc( sizeof( retcode_item_t ) );
		*(*rdip) = rdi;

		rc = 0;
		} break;

	default:
		rc = SLAP_CONF_UNKNOWN;
		break;
	}

	return rc;
}

static int
retcode_db_open( BackendDB *be, ConfigReply *cr)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	retcode_t	*rd = (retcode_t *)on->on_bi.bi_private;

	retcode_item_t	*rdi;

	for ( rdi = rd->rd_item; rdi; rdi = rdi->rdi_next ) {
		LDAPRDN			rdn = NULL;
		int			rc, j;
		char*			p;
		struct berval		val[ 3 ];
		char			buf[ SLAP_TEXT_BUFLEN ];

		/* DN */
		rdi->rdi_e.e_name = rdi->rdi_dn;
		rdi->rdi_e.e_nname = rdi->rdi_ndn;

		/* objectClass */
		val[ 0 ] = oc_errObject->soc_cname;
		val[ 1 ] = slap_schema.si_oc_extensibleObject->soc_cname;
		BER_BVZERO( &val[ 2 ] );

		attr_merge( &rdi->rdi_e, slap_schema.si_ad_objectClass, val, NULL );

		/* RDN avas */
		rc = ldap_bv2rdn( &rdi->rdi_dn, &rdn, (char **) &p,
				LDAP_DN_FORMAT_LDAP );

		assert( rc == LDAP_SUCCESS );

		for ( j = 0; rdn[ j ]; j++ ) {
			LDAPAVA			*ava = rdn[ j ];
			AttributeDescription	*ad = NULL;
			const char		*text;

			rc = slap_bv2ad( &ava->la_attr, &ad, &text );
			assert( rc == LDAP_SUCCESS );
			
			attr_merge_normalize_one( &rdi->rdi_e, ad,
					&ava->la_value, NULL );
		}

		ldap_rdnfree( rdn );

		/* error code */
		snprintf( buf, sizeof( buf ), "%d", rdi->rdi_err );
		ber_str2bv( buf, 0, 0, &val[ 0 ] );

		attr_merge_one( &rdi->rdi_e, ad_errCode, &val[ 0 ], NULL );

		if ( rdi->rdi_ref != NULL ) {
			attr_merge_normalize( &rdi->rdi_e, slap_schema.si_ad_ref,
				rdi->rdi_ref, NULL );
		}

		/* text */
		if ( !BER_BVISNULL( &rdi->rdi_text ) ) {
			val[ 0 ] = rdi->rdi_text;

			attr_merge_normalize_one( &rdi->rdi_e, ad_errText, &val[ 0 ], NULL );
		}

		/* matched */
		if ( !BER_BVISNULL( &rdi->rdi_matched ) ) {
			val[ 0 ] = rdi->rdi_matched;

			attr_merge_normalize_one( &rdi->rdi_e, ad_errMatchedDN, &val[ 0 ], NULL );
		}

		/* sleep time */
		if ( rdi->rdi_sleeptime ) {
			snprintf( buf, sizeof( buf ), "%d", rdi->rdi_sleeptime );
			ber_str2bv( buf, 0, 0, &val[ 0 ] );

			attr_merge_one( &rdi->rdi_e, ad_errSleepTime, &val[ 0 ], NULL );
		}

		/* operations */
		if ( rdi->rdi_mask & SN_DG_OP_ADD ) {
			BER_BVSTR( &val[ 0 ], "add" );
			attr_merge_normalize_one( &rdi->rdi_e, ad_errOp, &val[ 0 ], NULL );
		}

		if ( rdi->rdi_mask & SN_DG_OP_BIND ) {
			BER_BVSTR( &val[ 0 ], "bind" );
			attr_merge_normalize_one( &rdi->rdi_e, ad_errOp, &val[ 0 ], NULL );
		}

		if ( rdi->rdi_mask & SN_DG_OP_COMPARE ) {
			BER_BVSTR( &val[ 0 ], "compare" );
			attr_merge_normalize_one( &rdi->rdi_e, ad_errOp, &val[ 0 ], NULL );
		}

		if ( rdi->rdi_mask & SN_DG_OP_DELETE ) {
			BER_BVSTR( &val[ 0 ], "delete" );
			attr_merge_normalize_one( &rdi->rdi_e, ad_errOp, &val[ 0 ], NULL );
		}

		if ( rdi->rdi_mask & SN_DG_EXTENDED ) {
			BER_BVSTR( &val[ 0 ], "extended" );
			attr_merge_normalize_one( &rdi->rdi_e, ad_errOp, &val[ 0 ], NULL );
		}

		if ( rdi->rdi_mask & SN_DG_OP_MODIFY ) {
			BER_BVSTR( &val[ 0 ], "modify" );
			attr_merge_normalize_one( &rdi->rdi_e, ad_errOp, &val[ 0 ], NULL );
		}

		if ( rdi->rdi_mask & SN_DG_OP_RENAME ) {
			BER_BVSTR( &val[ 0 ], "rename" );
			attr_merge_normalize_one( &rdi->rdi_e, ad_errOp, &val[ 0 ], NULL );
		}

		if ( rdi->rdi_mask & SN_DG_OP_SEARCH ) {
			BER_BVSTR( &val[ 0 ], "search" );
			attr_merge_normalize_one( &rdi->rdi_e, ad_errOp, &val[ 0 ], NULL );
		}
	}

	return 0;
}

static int
retcode_db_destroy( BackendDB *be, ConfigReply *cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	retcode_t	*rd = (retcode_t *)on->on_bi.bi_private;

	if ( rd ) {
		retcode_item_t	*rdi, *next;

		for ( rdi = rd->rd_item; rdi != NULL; rdi = next ) {
			next = rdi->rdi_next;
			retcode_item_destroy( rdi );
		}

		if ( !BER_BVISNULL( &rd->rd_pdn ) ) {
			ber_memfree( rd->rd_pdn.bv_val );
		}

		if ( !BER_BVISNULL( &rd->rd_npdn ) ) {
			ber_memfree( rd->rd_npdn.bv_val );
		}

		ber_memfree( rd );
	}

	return 0;
}

#if SLAPD_OVER_RETCODE == SLAPD_MOD_DYNAMIC
static
#endif /* SLAPD_OVER_RETCODE == SLAPD_MOD_DYNAMIC */
int
retcode_initialize( void )
{
	int		i, code;

	static struct {
		char			*desc;
		AttributeDescription	**ad;
	} retcode_at[] = {
	        { "( 1.3.6.1.4.1.4203.666.11.4.1.1 "
		        "NAME ( 'errCode' ) "
		        "DESC 'LDAP error code' "
		        "EQUALITY integerMatch "
		        "ORDERING integerOrderingMatch "
		        "SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 "
			"SINGLE-VALUE )",
			&ad_errCode },
		{ "( 1.3.6.1.4.1.4203.666.11.4.1.2 "
			"NAME ( 'errOp' ) "
			"DESC 'Operations the errObject applies to' "
			"EQUALITY caseIgnoreMatch "
			"SUBSTR caseIgnoreSubstringsMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 )",
			&ad_errOp},
		{ "( 1.3.6.1.4.1.4203.666.11.4.1.3 "
			"NAME ( 'errText' ) "
			"DESC 'LDAP error textual description' "
			"EQUALITY caseIgnoreMatch "
			"SUBSTR caseIgnoreSubstringsMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
			"SINGLE-VALUE )",
			&ad_errText },
		{ "( 1.3.6.1.4.1.4203.666.11.4.1.4 "
			"NAME ( 'errSleepTime' ) "
			"DESC 'Time to wait before returning the error' "
			"EQUALITY integerMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 "
			"SINGLE-VALUE )",
			&ad_errSleepTime },
		{ "( 1.3.6.1.4.1.4203.666.11.4.1.5 "
			"NAME ( 'errMatchedDN' ) "
			"DESC 'Value to be returned as matched DN' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"SINGLE-VALUE )",
			&ad_errMatchedDN },
		{ "( 1.3.6.1.4.1.4203.666.11.4.1.6 "
			"NAME ( 'errUnsolicitedOID' ) "
			"DESC 'OID to be returned within unsolicited response' "
			"EQUALITY objectIdentifierMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 "
			"SINGLE-VALUE )",
			&ad_errUnsolicitedOID },
		{ "( 1.3.6.1.4.1.4203.666.11.4.1.7 "
			"NAME ( 'errUnsolicitedData' ) "
			"DESC 'Data to be returned within unsolicited response' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40 "
			"SINGLE-VALUE )",
			&ad_errUnsolicitedData },
		{ "( 1.3.6.1.4.1.4203.666.11.4.1.8 "
			"NAME ( 'errDisconnect' ) "
			"DESC 'Disconnect without notice' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.7 "
			"SINGLE-VALUE )",
			&ad_errDisconnect },
		{ NULL }
	};

	static struct {
		char		*desc;
		ObjectClass	**oc;
	} retcode_oc[] = {
		{ "( 1.3.6.1.4.1.4203.666.11.4.3.0 "
			"NAME ( 'errAbsObject' ) "
			"SUP top ABSTRACT "
			"MUST ( errCode ) "
			"MAY ( "
				"cn "
				"$ description "
				"$ errOp "
				"$ errText "
				"$ errSleepTime "
				"$ errMatchedDN "
				"$ errUnsolicitedOID "
				"$ errUnsolicitedData "
				"$ errDisconnect "
			") )",
			&oc_errAbsObject },
		{ "( 1.3.6.1.4.1.4203.666.11.4.3.1 "
			"NAME ( 'errObject' ) "
			"SUP errAbsObject STRUCTURAL "
			")",
			&oc_errObject },
		{ "( 1.3.6.1.4.1.4203.666.11.4.3.2 "
			"NAME ( 'errAuxObject' ) "
			"SUP errAbsObject AUXILIARY "
			")",
			&oc_errAuxObject },
		{ NULL }
	};


	for ( i = 0; retcode_at[ i ].desc != NULL; i++ ) {
		code = register_at( retcode_at[ i ].desc, retcode_at[ i ].ad, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"retcode: register_at failed\n", 0, 0, 0 );
			return code;
		}

		(*retcode_at[ i ].ad)->ad_type->sat_flags |= SLAP_AT_HIDE;
	}

	for ( i = 0; retcode_oc[ i ].desc != NULL; i++ ) {
		code = register_oc( retcode_oc[ i ].desc, retcode_oc[ i ].oc, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"retcode: register_oc failed\n", 0, 0, 0 );
			return code;
		}

		(*retcode_oc[ i ].oc)->soc_flags |= SLAP_OC_HIDE;
	}

	retcode.on_bi.bi_type = "retcode";

	retcode.on_bi.bi_db_init = retcode_db_init;
	retcode.on_bi.bi_db_open = retcode_db_open;
	retcode.on_bi.bi_db_destroy = retcode_db_destroy;

	retcode.on_bi.bi_op_add = retcode_op_func;
	retcode.on_bi.bi_op_bind = retcode_op_func;
	retcode.on_bi.bi_op_compare = retcode_op_func;
	retcode.on_bi.bi_op_delete = retcode_op_func;
	retcode.on_bi.bi_op_modify = retcode_op_func;
	retcode.on_bi.bi_op_modrdn = retcode_op_func;
	retcode.on_bi.bi_op_search = retcode_op_func;

	retcode.on_bi.bi_extended = retcode_op_func;

	retcode.on_response = retcode_response;

	retcode.on_bi.bi_cf_ocs = rcocs;

	code = config_register_schema( rccfg, rcocs );
	if ( code ) {
		return code;
	}

	return overlay_register( &retcode );
}

#if SLAPD_OVER_RETCODE == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return retcode_initialize();
}
#endif /* SLAPD_OVER_RETCODE == SLAPD_MOD_DYNAMIC */

#endif /* SLAPD_OVER_RETCODE */
