/* sssvlv.c - server side sort / virtual list view */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2009-2014 The OpenLDAP Foundation.
 * Portions copyright 2009 Symas Corporation.
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
 * This work was initially developed by Howard Chu for inclusion in
 * OpenLDAP Software. Support for multiple sorts per connection added
 * by Raphael Ouazana.
 */

#include "portable.h"

#ifdef SLAPD_OVER_SSSVLV

#include <stdio.h>

#include <ac/string.h>
#include <ac/ctype.h>

#include <avl.h>

#include "slap.h"
#include "lutil.h"
#include "config.h"

#include "../../../libraries/liblber/lber-int.h"	/* ber_rewind */

/* RFC2891: Server Side Sorting
 * RFC2696: Paged Results
 */
#ifndef LDAP_MATCHRULE_IDENTIFIER
#define LDAP_MATCHRULE_IDENTIFIER      0x80L
#define LDAP_REVERSEORDER_IDENTIFIER   0x81L
#define LDAP_ATTRTYPES_IDENTIFIER      0x80L
#endif

/* draft-ietf-ldapext-ldapv3-vlv-09.txt: Virtual List Views
 */
#ifndef LDAP_VLVBYINDEX_IDENTIFIER
#define LDAP_VLVBYINDEX_IDENTIFIER	   0xa0L
#define LDAP_VLVBYVALUE_IDENTIFIER     0x81L
#define LDAP_VLVCONTEXT_IDENTIFIER     0x04L

#define LDAP_VLV_SSS_MISSING	0x4C
#define LDAP_VLV_RANGE_ERROR	0x4D
#endif

#define SAFESTR(macro_str, macro_def) ((macro_str) ? (macro_str) : (macro_def))

#define SSSVLV_DEFAULT_MAX_KEYS	5
#define SSSVLV_DEFAULT_MAX_REQUEST_PER_CONN 5

#define NO_PS_COOKIE (PagedResultsCookie) -1
#define NO_VC_CONTEXT (unsigned long) -1

typedef struct vlv_ctrl {
	int vc_before;
	int vc_after;
	int	vc_offset;
	int vc_count;
	struct berval vc_value;
	unsigned long vc_context;
} vlv_ctrl;

typedef struct sort_key
{
	AttributeDescription	*sk_ad;
	MatchingRule			*sk_ordering;
	int						sk_direction;	/* 1=normal, -1=reverse */
} sort_key;

typedef struct sort_ctrl {
	int sc_nkeys;
	sort_key sc_keys[1];
} sort_ctrl;


typedef struct sort_node
{
	int sn_conn;
	int sn_session;
	struct berval sn_dn;
	struct berval *sn_vals;
} sort_node;

typedef struct sssvlv_info
{
	int svi_max;	/* max concurrent sorts */
	int svi_num;	/* current # sorts */
	int svi_max_keys;	/* max sort keys per request */
	int svi_max_percon; /* max concurrent sorts per con */
} sssvlv_info;

typedef struct sort_op
{
	Avlnode	*so_tree;
	sort_ctrl *so_ctrl;
	sssvlv_info *so_info;
	int so_paged;
	int so_page_size;
	int so_nentries;
	int so_vlv;
	int so_vlv_rc;
	int so_vlv_target;
	int so_session;
	unsigned long so_vcontext;
} sort_op;

/* There is only one conn table for all overlay instances */
/* Each conn can handle one session by context */
static sort_op ***sort_conns;
static ldap_pvt_thread_mutex_t sort_conns_mutex;
static int ov_count;
static const char *debug_header = "sssvlv";

static int sss_cid;
static int vlv_cid;

/* RFC 2981 Section 2.2
 * If a sort key is a multi-valued attribute, and an entry happens to
 * have multiple values for that attribute and no other controls are
 * present that affect the sorting order, then the server SHOULD use the
 * least value (according to the ORDERING rule for that attribute).
 */
static struct berval* select_value(
	Attribute		*attr,
	sort_key			*key )
{
	struct berval* ber1, *ber2;
	MatchingRule *mr = key->sk_ordering;
	unsigned i;
	int cmp;

	ber1 = &(attr->a_nvals[0]);
	ber2 = ber1+1;
	for ( i = 1; i < attr->a_numvals; i++,ber2++ ) {
		mr->smr_match( &cmp, 0, mr->smr_syntax, mr, ber1, ber2 );
		if ( cmp > 0 ) {
			ber1 = ber2;
		}
	}

	Debug(LDAP_DEBUG_TRACE, "%s: value selected for compare: %s\n",
		debug_header,
		SAFESTR(ber1->bv_val, "<Empty>"),
		0);

	return ber1;
}

static int node_cmp( const void* val1, const void* val2 )
{
	sort_node *sn1 = (sort_node *)val1;
	sort_node *sn2 = (sort_node *)val2;
	sort_ctrl *sc;
	MatchingRule *mr;
	int i, cmp = 0;
	assert( sort_conns[sn1->sn_conn]
		&& sort_conns[sn1->sn_conn][sn1->sn_session]
		&& sort_conns[sn1->sn_conn][sn1->sn_session]->so_ctrl );
	sc = sort_conns[sn1->sn_conn][sn1->sn_session]->so_ctrl;

	for ( i=0; cmp == 0 && i<sc->sc_nkeys; i++ ) {
		if ( BER_BVISNULL( &sn1->sn_vals[i] )) {
			if ( BER_BVISNULL( &sn2->sn_vals[i] ))
				cmp = 0;
			else
				cmp = sc->sc_keys[i].sk_direction;
		} else if ( BER_BVISNULL( &sn2->sn_vals[i] )) {
			cmp = sc->sc_keys[i].sk_direction * -1;
		} else {
			mr = sc->sc_keys[i].sk_ordering;
			mr->smr_match( &cmp, 0, mr->smr_syntax, mr,
				&sn1->sn_vals[i], &sn2->sn_vals[i] );
			if ( cmp )
				cmp *= sc->sc_keys[i].sk_direction;
		}
	}
	return cmp;
}

static int node_insert( const void *val1, const void *val2 )
{
	/* Never return equal so that new entries are always inserted */
	return node_cmp( val1, val2 ) < 0 ? -1 : 1;
}

static int pack_vlv_response_control(
	Operation		*op,
	SlapReply		*rs,
	sort_op			*so,
	LDAPControl	**ctrlsp )
{
	LDAPControl			*ctrl;
	BerElementBuffer	berbuf;
	BerElement			*ber		= (BerElement *)&berbuf;
	struct berval		cookie, bv;
	int					rc;

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	rc = ber_printf( ber, "{iie", so->so_vlv_target, so->so_nentries,
		so->so_vlv_rc );

	if ( rc != -1 && so->so_vcontext ) {
		cookie.bv_val = (char *)&so->so_vcontext;
		cookie.bv_len = sizeof(so->so_vcontext);
		rc = ber_printf( ber, "tO", LDAP_VLVCONTEXT_IDENTIFIER, &cookie );
	}

	if ( rc != -1 ) {
		rc = ber_printf( ber, "}" );
	}

	if ( rc != -1 ) {
		rc = ber_flatten2( ber, &bv, 0 );
	}

	if ( rc != -1 ) {
		ctrl = (LDAPControl *)op->o_tmpalloc( sizeof(LDAPControl)+
			bv.bv_len, op->o_tmpmemctx );
		ctrl->ldctl_oid			= LDAP_CONTROL_VLVRESPONSE;
		ctrl->ldctl_iscritical	= 0;
		ctrl->ldctl_value.bv_val = (char *)(ctrl+1);
		ctrl->ldctl_value.bv_len = bv.bv_len;
		AC_MEMCPY( ctrl->ldctl_value.bv_val, bv.bv_val, bv.bv_len );
		ctrlsp[0] = ctrl;
	} else {
		ctrlsp[0] = NULL;
		rs->sr_err = LDAP_OTHER;
	}

	ber_free_buf( ber );

	return rs->sr_err;
}

static int pack_pagedresult_response_control(
	Operation		*op,
	SlapReply		*rs,
	sort_op			*so,
	LDAPControl	**ctrlsp )
{
	LDAPControl			*ctrl;
	BerElementBuffer	berbuf;
	BerElement			*ber		= (BerElement *)&berbuf;
	PagedResultsCookie	resp_cookie;
	struct berval		cookie, bv;
	int					rc;

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	if ( so->so_nentries > 0 ) {
		resp_cookie		= ( PagedResultsCookie )so->so_tree;
		cookie.bv_len	= sizeof( PagedResultsCookie );
		cookie.bv_val	= (char *)&resp_cookie;
	} else {
		resp_cookie		= ( PagedResultsCookie )0;
		BER_BVZERO( &cookie );
	}

	op->o_conn->c_pagedresults_state.ps_cookie = resp_cookie;
	op->o_conn->c_pagedresults_state.ps_count
		= ((PagedResultsState *)op->o_pagedresults_state)->ps_count
		  + rs->sr_nentries;

	rc = ber_printf( ber, "{iO}", so->so_nentries, &cookie );
	if ( rc != -1 ) {
		rc = ber_flatten2( ber, &bv, 0 );
	}

	if ( rc != -1 ) {
		ctrl = (LDAPControl *)op->o_tmpalloc( sizeof(LDAPControl)+
			bv.bv_len, op->o_tmpmemctx );
		ctrl->ldctl_oid			= LDAP_CONTROL_PAGEDRESULTS;
		ctrl->ldctl_iscritical	= 0;
		ctrl->ldctl_value.bv_val = (char *)(ctrl+1);
		ctrl->ldctl_value.bv_len = bv.bv_len;
		AC_MEMCPY( ctrl->ldctl_value.bv_val, bv.bv_val, bv.bv_len );
		ctrlsp[0] = ctrl;
	} else {
		ctrlsp[0] = NULL;
		rs->sr_err = LDAP_OTHER;
	}

	ber_free_buf( ber );

	return rs->sr_err;
}

static int pack_sss_response_control(
	Operation		*op,
	SlapReply		*rs,
	LDAPControl	**ctrlsp )
{
	LDAPControl			*ctrl;
	BerElementBuffer	berbuf;
	BerElement			*ber		= (BerElement *)&berbuf;
	struct berval		bv;
	int					rc;

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	/* Pack error code */
	rc = ber_printf(ber, "{e}", rs->sr_err);

	if ( rc != -1)
		rc = ber_flatten2( ber, &bv, 0 );

	if ( rc != -1 ) {
		ctrl = (LDAPControl *)op->o_tmpalloc( sizeof(LDAPControl)+
			bv.bv_len, op->o_tmpmemctx );
		ctrl->ldctl_oid			= LDAP_CONTROL_SORTRESPONSE;
		ctrl->ldctl_iscritical	= 0;
		ctrl->ldctl_value.bv_val = (char *)(ctrl+1);
		ctrl->ldctl_value.bv_len = bv.bv_len;
		AC_MEMCPY( ctrl->ldctl_value.bv_val, bv.bv_val, bv.bv_len );
		ctrlsp[0] = ctrl;
	} else {
		ctrlsp[0] = NULL;
		rs->sr_err = LDAP_OTHER;
	}

	ber_free_buf( ber );

	return rs->sr_err;
}

/* Return the session id or -1 if unknown */
static int find_session_by_so(
	int svi_max_percon,
	int conn_id,
	sort_op *so )
{
	int sess_id;
	if (so == NULL) {
		return -1;
	}
	for (sess_id = 0; sess_id < svi_max_percon; sess_id++) {
		if ( sort_conns[conn_id] && sort_conns[conn_id][sess_id] == so )
			return sess_id;
	}
	return -1;
}

/* Return the session id or -1 if unknown */
static int find_session_by_context(
	int svi_max_percon,
	int conn_id,
	unsigned long vc_context,
	PagedResultsCookie ps_cookie )
{
	int sess_id;
	for(sess_id = 0; sess_id < svi_max_percon; sess_id++) {
		if( sort_conns[conn_id] && sort_conns[conn_id][sess_id] &&
		    ( sort_conns[conn_id][sess_id]->so_vcontext == vc_context || 
                      (PagedResultsCookie) sort_conns[conn_id][sess_id]->so_tree == ps_cookie ) )
			return sess_id;
	}
	return -1;
}

static int find_next_session(
	int svi_max_percon,
	int conn_id )
{
	int sess_id;
	assert(sort_conns[conn_id] != NULL);
	for(sess_id = 0; sess_id < svi_max_percon; sess_id++) {
		if(!sort_conns[conn_id][sess_id]) {
			return sess_id;
		}
	}
	if (sess_id >= svi_max_percon) {
		return -1;
	} else {
		return sess_id;
	}
}
	
static void free_sort_op( Connection *conn, sort_op *so )
{
	int sess_id;
	if ( so->so_tree ) {
		if ( so->so_paged > SLAP_CONTROL_IGNORED ) {
			Avlnode *cur_node, *next_node;
			cur_node = so->so_tree;
			while ( cur_node ) {
				next_node = tavl_next( cur_node, TAVL_DIR_RIGHT );
				ch_free( cur_node->avl_data );
				ber_memfree( cur_node );

				cur_node = next_node;
			}
		} else {
			tavl_free( so->so_tree, ch_free );
		}
		so->so_tree = NULL;
	}

	ldap_pvt_thread_mutex_lock( &sort_conns_mutex );
	sess_id = find_session_by_so( so->so_info->svi_max_percon, conn->c_conn_idx, so );
	sort_conns[conn->c_conn_idx][sess_id] = NULL;
	so->so_info->svi_num--;
	ldap_pvt_thread_mutex_unlock( &sort_conns_mutex );

	ch_free( so );
}

static void free_sort_ops( Connection *conn, sort_op **sos, int svi_max_percon )
{
	int sess_id;
	sort_op *so;

	for( sess_id = 0; sess_id < svi_max_percon ; sess_id++ ) {
		so = sort_conns[conn->c_conn_idx][sess_id];
		if ( so ) {
			free_sort_op( conn, so );
			sort_conns[conn->c_conn_idx][sess_id] = NULL;
		}
	}
}
	
static void send_list(
	Operation		*op,
	SlapReply		*rs,
	sort_op			*so)
{
	Avlnode	*cur_node, *tmp_node;
	vlv_ctrl *vc = op->o_controls[vlv_cid];
	int i, j, dir, rc;
	BackendDB *be;
	Entry *e;
	LDAPControl *ctrls[2];

	rs->sr_attrs = op->ors_attrs;

	/* FIXME: it may be better to just flatten the tree into
	 * an array before doing all of this...
	 */

	/* Are we just counting an offset? */
	if ( BER_BVISNULL( &vc->vc_value )) {
		if ( vc->vc_offset == vc->vc_count ) {
			/* wants the last entry in the list */
			cur_node = tavl_end(so->so_tree, TAVL_DIR_RIGHT);
			so->so_vlv_target = so->so_nentries;
		} else if ( vc->vc_offset == 1 ) {
			/* wants the first entry in the list */
			cur_node = tavl_end(so->so_tree, TAVL_DIR_LEFT);
			so->so_vlv_target = 1;
		} else {
			int target;
			/* Just iterate to the right spot */
			if ( vc->vc_count && vc->vc_count != so->so_nentries ) {
				if ( vc->vc_offset > vc->vc_count )
					goto range_err;
				target = so->so_nentries * vc->vc_offset / vc->vc_count;
			} else {
				if ( vc->vc_offset > so->so_nentries ) {
range_err:
					so->so_vlv_rc = LDAP_VLV_RANGE_ERROR;
					pack_vlv_response_control( op, rs, so, ctrls );
					ctrls[1] = NULL;
					slap_add_ctrls( op, rs, ctrls );
					rs->sr_err = LDAP_VLV_ERROR;
					return;
				}
				target = vc->vc_offset;
			}
			so->so_vlv_target = target;
			/* Start at left and go right, or start at right and go left? */
			if ( target < so->so_nentries / 2 ) {
				cur_node = tavl_end(so->so_tree, TAVL_DIR_LEFT);
				dir = TAVL_DIR_RIGHT;
			} else {
				cur_node = tavl_end(so->so_tree, TAVL_DIR_RIGHT);
				dir = TAVL_DIR_LEFT;
				target = so->so_nentries - target + 1;
			}
			for ( i=1; i<target; i++ )
				cur_node = tavl_next( cur_node, dir );
		}
	} else {
	/* we're looking for a specific value */
		sort_ctrl *sc = so->so_ctrl;
		MatchingRule *mr = sc->sc_keys[0].sk_ordering;
		sort_node *sn;
		struct berval bv;

		if ( mr->smr_normalize ) {
			rc = mr->smr_normalize( SLAP_MR_VALUE_OF_SYNTAX,
				mr->smr_syntax, mr, &vc->vc_value, &bv, op->o_tmpmemctx );
			if ( rc ) {
				so->so_vlv_rc = LDAP_INAPPROPRIATE_MATCHING;
				pack_vlv_response_control( op, rs, so, ctrls );
				ctrls[1] = NULL;
				slap_add_ctrls( op, rs, ctrls );
				rs->sr_err = LDAP_VLV_ERROR;
				return;
			}
		} else {
			bv = vc->vc_value;
		}

		sn = op->o_tmpalloc( sizeof(sort_node) +
			sc->sc_nkeys * sizeof(struct berval), op->o_tmpmemctx );
		sn->sn_vals = (struct berval *)(sn+1);
		sn->sn_conn = op->o_conn->c_conn_idx;
		sn->sn_session = find_session_by_so( so->so_info->svi_max_percon, op->o_conn->c_conn_idx, so );
		sn->sn_vals[0] = bv;
		for (i=1; i<sc->sc_nkeys; i++) {
			BER_BVZERO( &sn->sn_vals[i] );
		}
		cur_node = tavl_find3( so->so_tree, sn, node_cmp, &j );
		/* didn't find >= match */
		if ( j > 0 ) {
			if ( cur_node )
				cur_node = tavl_next( cur_node, TAVL_DIR_RIGHT );
		}
		op->o_tmpfree( sn, op->o_tmpmemctx );

		if ( !cur_node ) {
			so->so_vlv_target = so->so_nentries + 1;
		} else {
			sort_node *sn = so->so_tree->avl_data;
			/* start from the left or the right side? */
			mr->smr_match( &i, 0, mr->smr_syntax, mr, &bv, &sn->sn_vals[0] );
			if ( i > 0 ) {
				tmp_node = tavl_end(so->so_tree, TAVL_DIR_RIGHT);
				dir = TAVL_DIR_LEFT;
			} else {
				tmp_node = tavl_end(so->so_tree, TAVL_DIR_LEFT);
				dir = TAVL_DIR_RIGHT;
			}
			for (i=0; tmp_node != cur_node;
				tmp_node = tavl_next( tmp_node, dir ), i++);
			so->so_vlv_target = (dir == TAVL_DIR_RIGHT) ? i+1 : so->so_nentries - i;
		}
		if ( bv.bv_val != vc->vc_value.bv_val )
			op->o_tmpfree( bv.bv_val, op->o_tmpmemctx );
	}
	if ( !cur_node ) {
		i = 1;
		cur_node = tavl_end(so->so_tree, TAVL_DIR_RIGHT);
	} else {
		i = 0;
	}
	for ( ; i<vc->vc_before; i++ ) {
		tmp_node = tavl_next( cur_node, TAVL_DIR_LEFT );
		if ( !tmp_node ) break;
		cur_node = tmp_node;
	}
	j = i + vc->vc_after + 1;
	be = op->o_bd;
	for ( i=0; i<j; i++ ) {
		sort_node *sn = cur_node->avl_data;

		if ( slapd_shutdown ) break;

		op->o_bd = select_backend( &sn->sn_dn, 0 );
		e = NULL;
		rc = be_entry_get_rw( op, &sn->sn_dn, NULL, NULL, 0, &e );

		if ( e && rc == LDAP_SUCCESS ) {
			rs->sr_entry = e;
			rs->sr_flags = REP_ENTRY_MUSTRELEASE;
			rs->sr_err = send_search_entry( op, rs );
			if ( rs->sr_err == LDAP_UNAVAILABLE )
				break;
		}
		cur_node = tavl_next( cur_node, TAVL_DIR_RIGHT );
		if ( !cur_node ) break;
	}
	so->so_vlv_rc = LDAP_SUCCESS;

	op->o_bd = be;
}

static void send_page( Operation *op, SlapReply *rs, sort_op *so )
{
	Avlnode		*cur_node		= so->so_tree;
	Avlnode		*next_node		= NULL;
	BackendDB *be = op->o_bd;
	Entry *e;
	int rc;

	rs->sr_attrs = op->ors_attrs;

	while ( cur_node && rs->sr_nentries < so->so_page_size ) {
		sort_node *sn = cur_node->avl_data;

		if ( slapd_shutdown ) break;

		next_node = tavl_next( cur_node, TAVL_DIR_RIGHT );

		op->o_bd = select_backend( &sn->sn_dn, 0 );
		e = NULL;
		rc = be_entry_get_rw( op, &sn->sn_dn, NULL, NULL, 0, &e );

		ch_free( cur_node->avl_data );
		ber_memfree( cur_node );

		cur_node = next_node;
		so->so_nentries--;

		if ( e && rc == LDAP_SUCCESS ) {
			rs->sr_entry = e;
			rs->sr_flags = REP_ENTRY_MUSTRELEASE;
			rs->sr_err = send_search_entry( op, rs );
			if ( rs->sr_err == LDAP_UNAVAILABLE )
				break;
		}
	}

	/* Set the first entry to send for the next page */
	so->so_tree = next_node;
	if ( next_node )
		next_node->avl_left = NULL;

	op->o_bd = be;
}

static void send_entry(
	Operation		*op,
	SlapReply		*rs,
	sort_op			*so)
{
	Debug(LDAP_DEBUG_TRACE,
		"%s: response control: status=%d, text=%s\n",
		debug_header, rs->sr_err, SAFESTR(rs->sr_text, "<None>"));

	if ( !so->so_tree )
		return;

	/* RFC 2891: If critical then send the entries iff they were
	 * succesfully sorted.  If non-critical send all entries
	 * whether they were sorted or not.
	 */
	if ( (op->o_ctrlflag[sss_cid] != SLAP_CONTROL_CRITICAL) ||
		 (rs->sr_err == LDAP_SUCCESS) )
	{
		if ( so->so_vlv > SLAP_CONTROL_IGNORED ) {
			send_list( op, rs, so );
		} else {
			/* Get the first node to send */
			Avlnode *start_node = tavl_end(so->so_tree, TAVL_DIR_LEFT);
			so->so_tree = start_node;

			if ( so->so_paged <= SLAP_CONTROL_IGNORED ) {
				/* Not paged result search.  Send all entries.
				 * Set the page size to the number of entries
				 * so that send_page() will send all entries.
				 */
				so->so_page_size = so->so_nentries;
			}

			send_page( op, rs, so );
		}
	}
}

static void send_result(
	Operation		*op,
	SlapReply		*rs,
	sort_op			*so)
{
	LDAPControl *ctrls[3];
	int rc, i = 0;

	rc = pack_sss_response_control( op, rs, ctrls );
	if ( rc == LDAP_SUCCESS ) {
		i++;
		rc = -1;
		if ( so->so_paged > SLAP_CONTROL_IGNORED ) {
			rc = pack_pagedresult_response_control( op, rs, so, ctrls+1 );
		} else if ( so->so_vlv > SLAP_CONTROL_IGNORED ) {
			rc = pack_vlv_response_control( op, rs, so, ctrls+1 );
		}
		if ( rc == LDAP_SUCCESS )
			i++;
	}
	ctrls[i] = NULL;

	if ( ctrls[0] != NULL )
		slap_add_ctrls( op, rs, ctrls );
	send_ldap_result( op, rs );

	if ( so->so_tree == NULL ) {
		/* Search finished, so clean up */
		free_sort_op( op->o_conn, so );
	}
}

static int sssvlv_op_response(
	Operation	*op,
	SlapReply	*rs )
{
	sort_ctrl *sc = op->o_controls[sss_cid];
	sort_op *so = op->o_callback->sc_private;

	if ( rs->sr_type == REP_SEARCH ) {
		int i;
		size_t len;
		sort_node *sn, *sn2;
		struct berval *bv;
		char *ptr;

		len = sizeof(sort_node) + sc->sc_nkeys * sizeof(struct berval) +
			rs->sr_entry->e_nname.bv_len + 1;
		sn = op->o_tmpalloc( len, op->o_tmpmemctx );
		sn->sn_vals = (struct berval *)(sn+1);

		/* Build tmp list of key values */
		for ( i=0; i<sc->sc_nkeys; i++ ) {
			Attribute *a = attr_find( rs->sr_entry->e_attrs,
				sc->sc_keys[i].sk_ad );
			if ( a ) {
				if ( a->a_numvals > 1 ) {
					bv = select_value( a, &sc->sc_keys[i] );
				} else {
					bv = a->a_nvals;
				}
				sn->sn_vals[i] = *bv;
				len += bv->bv_len + 1;
			} else {
				BER_BVZERO( &sn->sn_vals[i] );
			}
		}

		/* Now dup into regular memory */
		sn2 = ch_malloc( len );
		sn2->sn_vals = (struct berval *)(sn2+1);
		AC_MEMCPY( sn2->sn_vals, sn->sn_vals,
				sc->sc_nkeys * sizeof(struct berval));

		ptr = (char *)(sn2->sn_vals + sc->sc_nkeys);
		sn2->sn_dn.bv_val = ptr;
		sn2->sn_dn.bv_len = rs->sr_entry->e_nname.bv_len;
		AC_MEMCPY( ptr, rs->sr_entry->e_nname.bv_val,
			rs->sr_entry->e_nname.bv_len );
		ptr += rs->sr_entry->e_nname.bv_len;
		*ptr++ = '\0';
		for ( i=0; i<sc->sc_nkeys; i++ ) {
			if ( !BER_BVISNULL( &sn2->sn_vals[i] )) {
				AC_MEMCPY(ptr, sn2->sn_vals[i].bv_val, sn2->sn_vals[i].bv_len);
				sn2->sn_vals[i].bv_val = ptr;
				ptr += sn2->sn_vals[i].bv_len;
				*ptr++ = '\0';
			}
		}
		op->o_tmpfree( sn, op->o_tmpmemctx );
		sn = sn2;
		sn->sn_conn = op->o_conn->c_conn_idx;
		sn->sn_session = find_session_by_so( so->so_info->svi_max_percon, op->o_conn->c_conn_idx, so );

		/* Insert into the AVL tree */
		tavl_insert(&(so->so_tree), sn, node_insert, avl_dup_error);

		so->so_nentries++;

		/* Collected the keys so that they can be sorted.  Thus, stop
		 * the entry from propagating.
		 */
		rs->sr_err = LDAP_SUCCESS;
	}
	else if ( rs->sr_type == REP_RESULT ) {
		/* Remove serversort response callback.
		 * We don't want the entries that we are about to send to be
		 * processed by serversort response again.
		 */
		if ( op->o_callback->sc_response == sssvlv_op_response ) {
			op->o_callback = op->o_callback->sc_next;
		}

		send_entry( op, rs, so );
		send_result( op, rs, so );
	}

	return rs->sr_err;
}

static int sssvlv_op_search(
	Operation		*op,
	SlapReply		*rs)
{
	slap_overinst			*on			= (slap_overinst *)op->o_bd->bd_info;
	sssvlv_info				*si			= on->on_bi.bi_private;
	int						rc			= SLAP_CB_CONTINUE;
	int	ok;
	sort_op *so = NULL, so2;
	sort_ctrl *sc;
	PagedResultsState *ps;
	vlv_ctrl *vc;
	int sess_id;

	if ( op->o_ctrlflag[sss_cid] <= SLAP_CONTROL_IGNORED ) {
		if ( op->o_ctrlflag[vlv_cid] > SLAP_CONTROL_IGNORED ) {
			LDAPControl *ctrls[2];
			so2.so_vcontext = 0;
			so2.so_vlv_target = 0;
			so2.so_nentries = 0;
			so2.so_vlv_rc = LDAP_VLV_SSS_MISSING;
			so2.so_vlv = op->o_ctrlflag[vlv_cid];
			rc = pack_vlv_response_control( op, rs, &so2, ctrls );
			if ( rc == LDAP_SUCCESS ) {
				ctrls[1] = NULL;
				slap_add_ctrls( op, rs, ctrls );
			}
			rs->sr_err = LDAP_VLV_ERROR;
			rs->sr_text = "Sort control is required with VLV";
			goto leave;
		}
		/* Not server side sort so just continue */
		return SLAP_CB_CONTINUE;
	}

	Debug(LDAP_DEBUG_TRACE,
		"==> sssvlv_search: <%s> %s, control flag: %d\n",
		op->o_req_dn.bv_val, op->ors_filterstr.bv_val,
		op->o_ctrlflag[sss_cid]);

	sc = op->o_controls[sss_cid];
	if ( sc->sc_nkeys > si->svi_max_keys ) {
		rs->sr_text = "Too many sort keys";
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		goto leave;
	}

	ps = ( op->o_pagedresults > SLAP_CONTROL_IGNORED ) ?
		(PagedResultsState*)(op->o_pagedresults_state) : NULL;
	vc = op->o_ctrlflag[vlv_cid] > SLAP_CONTROL_IGNORED ?
		op->o_controls[vlv_cid] : NULL;

	if ( ps && vc ) {
		rs->sr_text = "VLV incompatible with PagedResults";
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		goto leave;
	}

	ok = 1;
	ldap_pvt_thread_mutex_lock( &sort_conns_mutex );
	/* Is there already a sort running on this conn? */
	sess_id = find_session_by_context( si->svi_max_percon, op->o_conn->c_conn_idx, vc ? vc->vc_context : NO_VC_CONTEXT, ps ? ps->ps_cookie : NO_PS_COOKIE );
	if ( sess_id >= 0 ) {
		so = sort_conns[op->o_conn->c_conn_idx][sess_id];
		/* Is it a continuation of a VLV search? */
		if ( !vc || so->so_vlv <= SLAP_CONTROL_IGNORED ||
			vc->vc_context != so->so_vcontext ) {
			/* Is it a continuation of a paged search? */
			if ( !ps || so->so_paged <= SLAP_CONTROL_IGNORED ||
				op->o_conn->c_pagedresults_state.ps_cookie != ps->ps_cookie ) {
				ok = 0;
			} else if ( !ps->ps_size ) {
			/* Abandoning current request */
				ok = 0;
				so->so_nentries = 0;
				rs->sr_err = LDAP_SUCCESS;
			}
		}
		if (( vc && so->so_paged > SLAP_CONTROL_IGNORED ) ||
			( ps && so->so_vlv > SLAP_CONTROL_IGNORED )) {
			/* changed from paged to vlv or vice versa, abandon */
			ok = 0;
			so->so_nentries = 0;
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		}
	/* Are there too many running overall? */
	} else if ( si->svi_num >= si->svi_max ) {
		ok = 0;
	} else if ( ( sess_id = find_next_session(si->svi_max_percon, op->o_conn->c_conn_idx ) ) < 0 ) {
		ok = 0;
	} else {
		/* OK, this connection now has a sort running */
		si->svi_num++;
		sort_conns[op->o_conn->c_conn_idx][sess_id] = &so2;
		sort_conns[op->o_conn->c_conn_idx][sess_id]->so_session = sess_id;
	}
	ldap_pvt_thread_mutex_unlock( &sort_conns_mutex );
	if ( ok ) {
		/* If we're a global overlay, this check got bypassed */
		if ( !op->ors_limit && limits_check( op, rs ))
			return rs->sr_err;
		/* are we continuing a VLV search? */
		if ( so && vc && vc->vc_context ) {
			so->so_ctrl = sc;
			send_list( op, rs, so );
			send_result( op, rs, so );
			rc = LDAP_SUCCESS;
		/* are we continuing a paged search? */
		} else if ( so && ps && ps->ps_cookie ) {
			so->so_ctrl = sc;
			send_page( op, rs, so );
			send_result( op, rs, so );
			rc = LDAP_SUCCESS;
		} else {
			slap_callback *cb = op->o_tmpalloc( sizeof(slap_callback),
				op->o_tmpmemctx );
			/* Install serversort response callback to handle a new search */
			if ( ps || vc ) {
				so = ch_calloc( 1, sizeof(sort_op));
			} else {
				so = op->o_tmpcalloc( 1, sizeof(sort_op), op->o_tmpmemctx );
			}
			sort_conns[op->o_conn->c_conn_idx][sess_id] = so;

			cb->sc_cleanup		= NULL;
			cb->sc_response		= sssvlv_op_response;
			cb->sc_next			= op->o_callback;
			cb->sc_private		= so;

			so->so_tree = NULL;
			so->so_ctrl = sc;
			so->so_info = si;
			if ( ps ) {
				so->so_paged = op->o_pagedresults;
				so->so_page_size = ps->ps_size;
				op->o_pagedresults = SLAP_CONTROL_IGNORED;
			} else {
				so->so_paged = 0;
				so->so_page_size = 0;
				if ( vc ) {
					so->so_vlv = op->o_ctrlflag[vlv_cid];
					so->so_vlv_target = 0;
					so->so_vlv_rc = 0;
				} else {
					so->so_vlv = SLAP_CONTROL_NONE;
				}
			}
			so->so_session = sess_id;
			so->so_vlv = op->o_ctrlflag[vlv_cid];
			so->so_vcontext = (unsigned long)so;
			so->so_nentries = 0;

			op->o_callback		= cb;
		}
	} else {
		if ( so && !so->so_nentries ) {
			free_sort_op( op->o_conn, so );
		} else {
			rs->sr_text = "Other sort requests already in progress";
			rs->sr_err = LDAP_BUSY;
		}
leave:
		rc = rs->sr_err;
		send_ldap_result( op, rs );
	}

	return rc;
}

static int get_ordering_rule(
	AttributeDescription	*ad,
	struct berval			*matchrule,
	SlapReply				*rs,
	MatchingRule			**ordering )
{
	MatchingRule* mr;

	if ( matchrule && matchrule->bv_val ) {
		mr = mr_find( matchrule->bv_val );
		if ( mr == NULL ) {
			rs->sr_err = LDAP_INAPPROPRIATE_MATCHING;
			rs->sr_text = "serverSort control: No ordering rule";
			Debug(LDAP_DEBUG_TRACE, "%s: no ordering rule function for %s\n",
				debug_header, matchrule->bv_val, 0);
		}
	}
	else {
		mr = ad->ad_type->sat_ordering;
		if ( mr == NULL ) {
			rs->sr_err = LDAP_INAPPROPRIATE_MATCHING;
			rs->sr_text = "serverSort control: No ordering rule";
			Debug(LDAP_DEBUG_TRACE,
				"%s: no ordering rule specified and no default ordering rule for attribute %s\n",
				debug_header, ad->ad_cname.bv_val, 0);
		}
	}

	*ordering = mr;
	return rs->sr_err;
}

static int count_key(BerElement *ber)
{
	char *end;
	ber_len_t len;
	ber_tag_t tag;
	int count = 0;

	/* Server Side Sort Control is a SEQUENCE of SEQUENCE */
	for ( tag = ber_first_element( ber, &len, &end );
		  tag == LBER_SEQUENCE;
		  tag = ber_next_element( ber, &len, end ))
	{
		tag = ber_skip_tag( ber, &len );
		ber_skip_data( ber, len );
		++count;
	}
	ber_rewind( ber );

	return count;
}

static int build_key(
	BerElement		*ber,
	SlapReply		*rs,
	sort_key			*key )
{
	struct berval attr;
	struct berval matchrule = BER_BVNULL;
	ber_int_t reverse = 0;
	ber_tag_t tag;
	ber_len_t len;
	MatchingRule *ordering = NULL;
	AttributeDescription *ad = NULL;
	const char *text;

	if (( tag = ber_scanf( ber, "{" )) == LBER_ERROR ) {
		rs->sr_text = "serverSort control: decoding error";
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		return rs->sr_err;
	}

	if (( tag = ber_scanf( ber, "m", &attr )) == LBER_ERROR ) {
		rs->sr_text = "serverSort control: attribute decoding error";
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		return rs->sr_err;
	}

	tag = ber_peek_tag( ber, &len );
	if ( tag == LDAP_MATCHRULE_IDENTIFIER ) {
		if (( tag = ber_scanf( ber, "m", &matchrule )) == LBER_ERROR ) {
			rs->sr_text = "serverSort control: matchrule decoding error";
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			return rs->sr_err;
		}
		tag = ber_peek_tag( ber, &len );
	}

	if ( tag == LDAP_REVERSEORDER_IDENTIFIER ) {
		if (( tag = ber_scanf( ber, "b", &reverse )) == LBER_ERROR ) {
			rs->sr_text = "serverSort control: reverse decoding error";
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			return rs->sr_err;
		}
	}

	if (( tag = ber_scanf( ber, "}" )) == LBER_ERROR ) {
		rs->sr_text = "serverSort control: decoding error";
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		return rs->sr_err;
	}

	if ( slap_bv2ad( &attr, &ad, &text ) != LDAP_SUCCESS ) {
		rs->sr_text =
			"serverSort control: Unrecognized attribute type in sort key";
		Debug(LDAP_DEBUG_TRACE,
			"%s: Unrecognized attribute type in sort key: %s\n",
			debug_header, SAFESTR(attr.bv_val, "<None>"), 0);
		rs->sr_err = LDAP_NO_SUCH_ATTRIBUTE;
		return rs->sr_err;
	}

	/* get_ordering_rule will set sr_err and sr_text */
	get_ordering_rule( ad, &matchrule, rs, &ordering );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		return rs->sr_err;
	}

	key->sk_ad = ad;
	key->sk_ordering = ordering;
	key->sk_direction = reverse ? -1 : 1;

	return rs->sr_err;
}

/* Conforms to RFC4510 re: Criticality, original RFC2891 spec is broken
 * Also see ITS#7253 for discussion
 */
static int sss_parseCtrl(
	Operation		*op,
	SlapReply		*rs,
	LDAPControl		*ctrl )
{
	BerElementBuffer	berbuf;
	BerElement			*ber;
	ber_tag_t		tag;
	ber_len_t		len;
	int					i;
	sort_ctrl	*sc;

	rs->sr_err = LDAP_PROTOCOL_ERROR;

	if ( op->o_ctrlflag[sss_cid] > SLAP_CONTROL_IGNORED ) {
		rs->sr_text = "sorted results control specified multiple times";
	} else if ( BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "sorted results control value is absent";
	} else if ( BER_BVISEMPTY( &ctrl->ldctl_value ) ) {
		rs->sr_text = "sorted results control value is empty";
	} else {
		rs->sr_err = LDAP_SUCCESS;
	}
	if ( rs->sr_err != LDAP_SUCCESS )
		return rs->sr_err;

	op->o_ctrlflag[sss_cid] = ctrl->ldctl_iscritical ?
		SLAP_CONTROL_CRITICAL : SLAP_CONTROL_NONCRITICAL;

	ber = (BerElement *)&berbuf;
	ber_init2( ber, &ctrl->ldctl_value, 0 );
	i = count_key( ber );

	sc = op->o_tmpalloc( sizeof(sort_ctrl) +
		(i-1) * sizeof(sort_key), op->o_tmpmemctx );
	sc->sc_nkeys = i;
	op->o_controls[sss_cid] = sc;

	/* peel off initial sequence */
	ber_scanf( ber, "{" );

	i = 0;
	do {
		if ( build_key( ber, rs, &sc->sc_keys[i] ) != LDAP_SUCCESS )
			break;
		i++;
		tag = ber_peek_tag( ber, &len );
	} while ( tag != LBER_DEFAULT );

	return rs->sr_err;
}

static int vlv_parseCtrl(
	Operation		*op,
	SlapReply		*rs,
	LDAPControl		*ctrl )
{
	BerElementBuffer	berbuf;
	BerElement			*ber;
	ber_tag_t		tag;
	ber_len_t		len;
	vlv_ctrl	*vc, vc2;

	rs->sr_err = LDAP_PROTOCOL_ERROR;
	rs->sr_text = NULL;

	if ( op->o_ctrlflag[vlv_cid] > SLAP_CONTROL_IGNORED ) {
		rs->sr_text = "vlv control specified multiple times";
	} else if ( BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "vlv control value is absent";
	} else if ( BER_BVISEMPTY( &ctrl->ldctl_value ) ) {
		rs->sr_text = "vlv control value is empty";
	}
	if ( rs->sr_text != NULL )
		return rs->sr_err;

	op->o_ctrlflag[vlv_cid] = ctrl->ldctl_iscritical ?
		SLAP_CONTROL_CRITICAL : SLAP_CONTROL_NONCRITICAL;

	ber = (BerElement *)&berbuf;
	ber_init2( ber, &ctrl->ldctl_value, 0 );

	rs->sr_err = LDAP_PROTOCOL_ERROR;

	tag = ber_scanf( ber, "{ii", &vc2.vc_before, &vc2.vc_after );
	if ( tag == LBER_ERROR ) {
		return rs->sr_err;
	}

	tag = ber_peek_tag( ber, &len );
	if ( tag == LDAP_VLVBYINDEX_IDENTIFIER ) {
		tag = ber_scanf( ber, "{ii}", &vc2.vc_offset, &vc2.vc_count );
		if ( tag == LBER_ERROR )
			return rs->sr_err;
		BER_BVZERO( &vc2.vc_value );
	} else if ( tag == LDAP_VLVBYVALUE_IDENTIFIER ) {
		tag = ber_scanf( ber, "m", &vc2.vc_value );
		if ( tag == LBER_ERROR || BER_BVISNULL( &vc2.vc_value ))
			return rs->sr_err;
	} else {
		return rs->sr_err;
	}
	tag = ber_peek_tag( ber, &len );
	if ( tag == LDAP_VLVCONTEXT_IDENTIFIER ) {
		struct berval bv;
		tag = ber_scanf( ber, "m", &bv );
		if ( tag == LBER_ERROR || bv.bv_len != sizeof(vc2.vc_context))
			return rs->sr_err;
		AC_MEMCPY( &vc2.vc_context, bv.bv_val, bv.bv_len );
	} else {
		vc2.vc_context = 0;
	}

	vc = op->o_tmpalloc( sizeof(vlv_ctrl), op->o_tmpmemctx );
	*vc = vc2;
	op->o_controls[vlv_cid] = vc;
	rs->sr_err = LDAP_SUCCESS;

	return rs->sr_err;
}

static int sssvlv_connection_destroy( BackendDB *be, Connection *conn )
{
	slap_overinst	*on		= (slap_overinst *)be->bd_info;
	sssvlv_info *si = on->on_bi.bi_private;

	if ( sort_conns[conn->c_conn_idx] ) {
		free_sort_ops( conn, sort_conns[conn->c_conn_idx], si->svi_max_percon );
	}

	return LDAP_SUCCESS;
}

static int sssvlv_db_open(
	BackendDB		*be,
	ConfigReply		*cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	sssvlv_info *si = on->on_bi.bi_private;
	int rc;
	int conn_index;

	/* If not set, default to 1/2 of available threads */
	if ( !si->svi_max )
		si->svi_max = connection_pool_max / 2;

	if ( dtblsize && !sort_conns ) {
		ldap_pvt_thread_mutex_init( &sort_conns_mutex );
		/* accommodate for c_conn_idx == -1 */
		sort_conns = ch_calloc( dtblsize + 1, sizeof(sort_op **) );
		for ( conn_index = 0 ; conn_index < dtblsize + 1 ; conn_index++ ) {
			sort_conns[conn_index] = ch_calloc( si->svi_max_percon, sizeof(sort_op *) );
		}
		sort_conns++;
	}

	rc = overlay_register_control( be, LDAP_CONTROL_SORTREQUEST );
	if ( rc == LDAP_SUCCESS )
		rc = overlay_register_control( be, LDAP_CONTROL_VLVREQUEST );
	return rc;
}

static ConfigTable sssvlv_cfg[] = {
	{ "sssvlv-max", "num",
		2, 2, 0, ARG_INT|ARG_OFFSET,
			(void *)offsetof(sssvlv_info, svi_max),
		"( OLcfgOvAt:21.1 NAME 'olcSssVlvMax' "
			"DESC 'Maximum number of concurrent Sort requests' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "sssvlv-maxkeys", "num",
		2, 2, 0, ARG_INT|ARG_OFFSET,
			(void *)offsetof(sssvlv_info, svi_max_keys),
		"( OLcfgOvAt:21.2 NAME 'olcSssVlvMaxKeys' "
			"DESC 'Maximum number of Keys in a Sort request' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "sssvlv-maxperconn", "num",
		2, 2, 0, ARG_INT|ARG_OFFSET,
			(void *)offsetof(sssvlv_info, svi_max_percon),
		"( OLcfgOvAt:21.3 NAME 'olcSssVlvMaxPerConn' "
			"DESC 'Maximum number of concurrent paged search requests per connection' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs sssvlv_ocs[] = {
	{ "( OLcfgOvOc:21.1 "
		"NAME 'olcSssVlvConfig' "
		"DESC 'SSS VLV configuration' "
		"SUP olcOverlayConfig "
		"MAY ( olcSssVlvMax $ olcSssVlvMaxKeys ) )",
		Cft_Overlay, sssvlv_cfg, NULL, NULL },
	{ NULL, 0, NULL }
};

static int sssvlv_db_init(
	BackendDB		*be,
	ConfigReply		*cr)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	sssvlv_info *si;

	if ( ov_count == 0 ) {
		int rc;

		rc = register_supported_control2( LDAP_CONTROL_SORTREQUEST,
			SLAP_CTRL_SEARCH,
			NULL,
			sss_parseCtrl,
			1 /* replace */,
			&sss_cid );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "Failed to register Sort Request control '%s' (%d)\n",
				LDAP_CONTROL_SORTREQUEST, rc, 0 );
			return rc;
		}

		rc = register_supported_control2( LDAP_CONTROL_VLVREQUEST,
			SLAP_CTRL_SEARCH,
			NULL,
			vlv_parseCtrl,
			1 /* replace */,
			&vlv_cid );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "Failed to register VLV Request control '%s' (%d)\n",
				LDAP_CONTROL_VLVREQUEST, rc, 0 );
#ifdef SLAP_CONFIG_DELETE
			overlay_unregister_control( be, LDAP_CONTROL_SORTREQUEST );
			unregister_supported_control( LDAP_CONTROL_SORTREQUEST );
#endif /* SLAP_CONFIG_DELETE */
			return rc;
		}
	}
	
	si = (sssvlv_info *)ch_malloc(sizeof(sssvlv_info));
	on->on_bi.bi_private = si;

	si->svi_max = 0;
	si->svi_num = 0;
	si->svi_max_keys = SSSVLV_DEFAULT_MAX_KEYS;
	si->svi_max_percon = SSSVLV_DEFAULT_MAX_REQUEST_PER_CONN;

	ov_count++;

	return LDAP_SUCCESS;
}

static int sssvlv_db_destroy(
	BackendDB		*be,
	ConfigReply		*cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	sssvlv_info *si = (sssvlv_info *)on->on_bi.bi_private;
	int conn_index;

	ov_count--;
	if ( !ov_count && sort_conns) {
		sort_conns--;
		for ( conn_index = 0 ; conn_index < dtblsize + 1 ; conn_index++ ) {
			ch_free(sort_conns[conn_index]);
		}
		ch_free(sort_conns);
		ldap_pvt_thread_mutex_destroy( &sort_conns_mutex );
	}

#ifdef SLAP_CONFIG_DELETE
	overlay_unregister_control( be, LDAP_CONTROL_SORTREQUEST );
	overlay_unregister_control( be, LDAP_CONTROL_VLVREQUEST );
	if ( ov_count == 0 ) {
		unregister_supported_control( LDAP_CONTROL_SORTREQUEST );
		unregister_supported_control( LDAP_CONTROL_VLVREQUEST );
	}
#endif /* SLAP_CONFIG_DELETE */

	if ( si ) {
		ch_free( si );
		on->on_bi.bi_private = NULL;
	}
	return LDAP_SUCCESS;
}

static slap_overinst sssvlv;

int sssvlv_initialize()
{
	int rc;

	sssvlv.on_bi.bi_type				= "sssvlv";
	sssvlv.on_bi.bi_db_init				= sssvlv_db_init;
	sssvlv.on_bi.bi_db_destroy			= sssvlv_db_destroy;
	sssvlv.on_bi.bi_db_open				= sssvlv_db_open;
	sssvlv.on_bi.bi_connection_destroy	= sssvlv_connection_destroy;
	sssvlv.on_bi.bi_op_search			= sssvlv_op_search;

	sssvlv.on_bi.bi_cf_ocs = sssvlv_ocs;

	rc = config_register_schema( sssvlv_cfg, sssvlv_ocs );
	if ( rc )
		return rc;

	rc = overlay_register( &sssvlv );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "Failed to register server side sort overlay\n", 0, 0, 0 );
	}

	return rc;
}

#if SLAPD_OVER_SSSVLV == SLAPD_MOD_DYNAMIC
int init_module( int argc, char *argv[])
{
	return sssvlv_initialize();
}
#endif

#endif /* SLAPD_OVER_SSSVLV */
