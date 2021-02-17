/* backglue.c - backend glue */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2001-2014 The OpenLDAP Foundation.
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

/*
 * Functions to glue a bunch of other backends into a single tree.
 * All of the glued backends must share a common suffix. E.g., you
 * can glue o=foo and ou=bar,o=foo but you can't glue o=foo and o=bar.
 *
 * The purpose of these functions is to allow you to split a single database
 * into pieces (for load balancing purposes, whatever) but still be able
 * to treat it as a single database after it's been split. As such, each
 * of the glued backends should have identical rootdn.
 *  -- Howard Chu
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#define SLAPD_TOOLS
#include "slap.h"
#include "lutil.h"
#include "config.h"

typedef struct gluenode {
	BackendDB *gn_be;
	struct berval gn_pdn;
} gluenode;

typedef struct glueinfo {
	int gi_nodes;
	struct berval gi_pdn;
	gluenode gi_n[1];
} glueinfo;

static slap_overinst	glue;

static int glueMode;
static BackendDB *glueBack;
static BackendDB glueBackDone;
#define GLUEBACK_DONE (&glueBackDone)

static slap_overinst * glue_tool_inst( BackendInfo *bi);

static slap_response glue_op_response;

/* Just like select_backend, but only for our backends */
static BackendDB *
glue_back_select (
	BackendDB *be,
	struct berval *dn
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	glueinfo		*gi = (glueinfo *)on->on_bi.bi_private;
	int i;

	for (i = gi->gi_nodes-1; i >= 0; i--) {
		assert( gi->gi_n[i].gn_be->be_nsuffix != NULL );

		if (dnIsSuffix(dn, &gi->gi_n[i].gn_be->be_nsuffix[0])) {
			return gi->gi_n[i].gn_be;
		}
	}
	be->bd_info = on->on_info->oi_orig;
	return be;
}


typedef struct glue_state {
	char *matched;
	BerVarray refs;
	LDAPControl **ctrls;
	int err;
	int matchlen;
	int nrefs;
	int nctrls;
} glue_state;

static int
glue_op_cleanup( Operation *op, SlapReply *rs )
{
	/* This is not a final result */
	if (rs->sr_type == REP_RESULT )
		rs->sr_type = REP_GLUE_RESULT;
	return SLAP_CB_CONTINUE;
}

static int
glue_op_response ( Operation *op, SlapReply *rs )
{
	glue_state *gs = op->o_callback->sc_private;

	switch(rs->sr_type) {
	case REP_SEARCH:
	case REP_SEARCHREF:
	case REP_INTERMEDIATE:
		return SLAP_CB_CONTINUE;

	default:
		if (rs->sr_err == LDAP_SUCCESS ||
			rs->sr_err == LDAP_SIZELIMIT_EXCEEDED ||
			rs->sr_err == LDAP_TIMELIMIT_EXCEEDED ||
			rs->sr_err == LDAP_ADMINLIMIT_EXCEEDED ||
			rs->sr_err == LDAP_NO_SUCH_OBJECT ||
			gs->err != LDAP_SUCCESS)
			gs->err = rs->sr_err;
		if (gs->err == LDAP_SUCCESS && gs->matched) {
			ch_free (gs->matched);
			gs->matched = NULL;
			gs->matchlen = 0;
		}
		if (gs->err != LDAP_SUCCESS && rs->sr_matched) {
			int len;
			len = strlen (rs->sr_matched);
			if (len > gs->matchlen) {
				if (gs->matched)
					ch_free (gs->matched);
				gs->matched = ch_strdup (rs->sr_matched);
				gs->matchlen = len;
			}
		}
		if (rs->sr_ref) {
			int i, j, k;
			BerVarray new;

			for (i=0; rs->sr_ref[i].bv_val; i++);

			j = gs->nrefs;
			if (!j) {
				new = ch_malloc ((i+1)*sizeof(struct berval));
			} else {
				new = ch_realloc(gs->refs,
					(j+i+1)*sizeof(struct berval));
			}
			for (k=0; k<i; j++,k++) {
				ber_dupbv( &new[j], &rs->sr_ref[k] );
			}
			new[j].bv_val = NULL;
			gs->nrefs = j;
			gs->refs = new;
		}
		if (rs->sr_ctrls) {
			int i, j, k;
			LDAPControl **newctrls;

			for (i=0; rs->sr_ctrls[i]; i++);

			j = gs->nctrls;
			if (!j) {
				newctrls = op->o_tmpalloc((i+1)*sizeof(LDAPControl *),
					op->o_tmpmemctx);
			} else {
				/* Forget old pagedResults response if we're sending
				 * a new one now
				 */
				if ( get_pagedresults( op ) > SLAP_CONTROL_IGNORED ) {
					int newpage = 0;
					for ( k=0; k<i; k++ ) {
						if ( !strcmp(rs->sr_ctrls[k]->ldctl_oid,
							LDAP_CONTROL_PAGEDRESULTS )) {
							newpage = 1;
							break;
						}
					}
					if ( newpage ) {
						for ( k=0; k<j; k++ ) {
							if ( !strcmp(gs->ctrls[k]->ldctl_oid,
								LDAP_CONTROL_PAGEDRESULTS ))
							{
								op->o_tmpfree(gs->ctrls[k], op->o_tmpmemctx);
								gs->ctrls[k] = gs->ctrls[--j];
								gs->ctrls[j] = NULL;
								break;
							}
						}
					}
				}
				newctrls = op->o_tmprealloc(gs->ctrls,
					(j+i+1)*sizeof(LDAPControl *), op->o_tmpmemctx);
			}
			for (k=0; k<i; j++,k++) {
				ber_len_t oidlen = strlen( rs->sr_ctrls[k]->ldctl_oid );
				newctrls[j] = op->o_tmpalloc(sizeof(LDAPControl) + oidlen + 1 + rs->sr_ctrls[k]->ldctl_value.bv_len + 1,
					op->o_tmpmemctx);
				newctrls[j]->ldctl_iscritical = rs->sr_ctrls[k]->ldctl_iscritical;
				newctrls[j]->ldctl_oid = (char *)&newctrls[j][1];
				lutil_strcopy( newctrls[j]->ldctl_oid, rs->sr_ctrls[k]->ldctl_oid );
				if ( !BER_BVISNULL( &rs->sr_ctrls[k]->ldctl_value ) ) {
					newctrls[j]->ldctl_value.bv_val = &newctrls[j]->ldctl_oid[oidlen + 1];
					newctrls[j]->ldctl_value.bv_len = rs->sr_ctrls[k]->ldctl_value.bv_len;
					lutil_memcopy( newctrls[j]->ldctl_value.bv_val,
						rs->sr_ctrls[k]->ldctl_value.bv_val,
						rs->sr_ctrls[k]->ldctl_value.bv_len + 1 );
				} else {
					BER_BVZERO( &newctrls[j]->ldctl_value );
				}
			}
			newctrls[j] = NULL;
			gs->nctrls = j;
			gs->ctrls = newctrls;
		}
	}
	return 0;
}

static int
glue_op_func ( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	BackendDB *b0 = op->o_bd;
	BackendInfo *bi0 = op->o_bd->bd_info, *bi1;
	slap_operation_t which = op_bind;
	int rc;

	op->o_bd = glue_back_select (b0, &op->o_req_ndn);

	/* If we're on the master backend, let overlay framework handle it */
	if ( op->o_bd == b0 )
		return SLAP_CB_CONTINUE;

	b0->bd_info = on->on_info->oi_orig;

	switch(op->o_tag) {
	case LDAP_REQ_ADD: which = op_add; break;
	case LDAP_REQ_DELETE: which = op_delete; break;
	case LDAP_REQ_MODIFY: which = op_modify; break;
	case LDAP_REQ_MODRDN: which = op_modrdn; break;
	case LDAP_REQ_EXTENDED: which = op_extended; break;
	default: assert( 0 ); break;
	}

	bi1 = op->o_bd->bd_info;
	rc = (&bi1->bi_op_bind)[ which ] ?
		(&bi1->bi_op_bind)[ which ]( op, rs ) : SLAP_CB_BYPASS;

	op->o_bd = b0;
	op->o_bd->bd_info = bi0;
	return rc;
}

static int
glue_op_abandon( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	glueinfo		*gi = (glueinfo *)on->on_bi.bi_private;
	BackendDB *b0 = op->o_bd;
	BackendInfo *bi0 = op->o_bd->bd_info;
	int i;

	b0->bd_info = on->on_info->oi_orig;

	for (i = gi->gi_nodes-1; i >= 0; i--) {
		assert( gi->gi_n[i].gn_be->be_nsuffix != NULL );
		op->o_bd = gi->gi_n[i].gn_be;
		if ( op->o_bd == b0 )
			continue;
		if ( op->o_bd->bd_info->bi_op_abandon )
			op->o_bd->bd_info->bi_op_abandon( op, rs );
	}
	op->o_bd = b0;
	op->o_bd->bd_info = bi0;
	return SLAP_CB_CONTINUE;
}

static int
glue_response ( Operation *op, SlapReply *rs )
{
	BackendDB *be = op->o_bd;
	be = glue_back_select (op->o_bd, &op->o_req_ndn);

	/* If we're on the master backend, let overlay framework handle it.
	 * Otherwise, bail out.
	 */
	return ( op->o_bd == be ) ? SLAP_CB_CONTINUE : SLAP_CB_BYPASS;
}

static int
glue_chk_referrals ( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	BackendDB *b0 = op->o_bd;
	BackendInfo *bi0 = op->o_bd->bd_info;
	int rc;

	op->o_bd = glue_back_select (b0, &op->o_req_ndn);
	if ( op->o_bd == b0 )
		return SLAP_CB_CONTINUE;

	b0->bd_info = on->on_info->oi_orig;

	if ( op->o_bd->bd_info->bi_chk_referrals )
		rc = ( *op->o_bd->bd_info->bi_chk_referrals )( op, rs );
	else
		rc = SLAP_CB_CONTINUE;

	op->o_bd = b0;
	op->o_bd->bd_info = bi0;
	return rc;
}

static int
glue_chk_controls ( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	BackendDB *b0 = op->o_bd;
	BackendInfo *bi0 = op->o_bd->bd_info;
	int rc = SLAP_CB_CONTINUE;

	op->o_bd = glue_back_select (b0, &op->o_req_ndn);
	if ( op->o_bd == b0 )
		return SLAP_CB_CONTINUE;

	b0->bd_info = on->on_info->oi_orig;

	/* if the subordinate database has overlays, the bi_chk_controls()
	 * hook is actually over_aux_chk_controls(); in case it actually
	 * wraps a missing hok, we need to mimic the behavior
	 * of the frontend applied to that database */
	if ( op->o_bd->bd_info->bi_chk_controls ) {
		rc = ( *op->o_bd->bd_info->bi_chk_controls )( op, rs );
	}

	
	if ( rc == SLAP_CB_CONTINUE ) {
		rc = backend_check_controls( op, rs );
	}

	op->o_bd = b0;
	op->o_bd->bd_info = bi0;
	return rc;
}

/* ITS#4615 - overlays configured above the glue overlay should be
 * invoked for the entire glued tree. Overlays configured below the
 * glue overlay should only be invoked on the master backend.
 * So, if we're searching on any subordinates, we need to force the
 * current overlay chain to stop processing, without stopping the
 * overall callback flow.
 */
static int
glue_sub_search( Operation *op, SlapReply *rs, BackendDB *b0,
	slap_overinst *on )
{
	/* Process any overlays on the master backend */
	if ( op->o_bd == b0 && on->on_next ) {
		BackendInfo *bi = op->o_bd->bd_info;
		int rc = SLAP_CB_CONTINUE;
		for ( on=on->on_next; on; on=on->on_next ) {
			op->o_bd->bd_info = (BackendInfo *)on;
			if ( on->on_bi.bi_op_search ) {
				rc = on->on_bi.bi_op_search( op, rs );
				if ( rc != SLAP_CB_CONTINUE )
					break;
			}
		}
		op->o_bd->bd_info = bi;
		if ( rc != SLAP_CB_CONTINUE )
			return rc;
	}
	return op->o_bd->be_search( op, rs );
}

static const ID glueID = NOID;
static const struct berval gluecookie = { sizeof( glueID ), (char *)&glueID };

static int
glue_op_search ( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	glueinfo		*gi = (glueinfo *)on->on_bi.bi_private;
	BackendDB *b0 = op->o_bd;
	BackendDB *b1 = NULL, *btmp;
	BackendInfo *bi0 = op->o_bd->bd_info;
	int i;
	long stoptime = 0, starttime;
	glue_state gs = {NULL, NULL, NULL, 0, 0, 0, 0};
	slap_callback cb = { NULL, glue_op_response, glue_op_cleanup, NULL };
	int scope0, tlimit0;
	struct berval dn, ndn, *pdn;

	cb.sc_private = &gs;

	cb.sc_next = op->o_callback;

	starttime = op->o_time;
	stoptime = slap_get_time () + op->ors_tlimit;

	/* reset dummy cookie used to keep paged results going across databases */
	if ( get_pagedresults( op ) > SLAP_CONTROL_IGNORED
		&& bvmatch( &((PagedResultsState *)op->o_pagedresults_state)->ps_cookieval, &gluecookie ) )
	{
		PagedResultsState *ps = op->o_pagedresults_state;
		BerElementBuffer berbuf;
		BerElement *ber = (BerElement *)&berbuf;
		struct berval cookie = BER_BVC(""), value;
		int c;

		for (c = 0; op->o_ctrls[c] != NULL; c++) {
			if (strcmp(op->o_ctrls[c]->ldctl_oid, LDAP_CONTROL_PAGEDRESULTS) == 0)
				break;
		}

		assert( op->o_ctrls[c] != NULL );

		ber_init2( ber, NULL, LBER_USE_DER );
		ber_printf( ber, "{iO}", ps->ps_size, &cookie );
		ber_flatten2( ber, &value, 0 );
		assert( op->o_ctrls[c]->ldctl_value.bv_len >= value.bv_len );
		op->o_ctrls[c]->ldctl_value.bv_len = value.bv_len;
		lutil_memcopy( op->o_ctrls[c]->ldctl_value.bv_val,
			value.bv_val, value.bv_len );
		ber_free_buf( ber );

		ps->ps_cookie = (PagedResultsCookie)0;
		BER_BVZERO( &ps->ps_cookieval );
	}

	op->o_bd = glue_back_select (b0, &op->o_req_ndn);
	b0->bd_info = on->on_info->oi_orig;

	switch (op->ors_scope) {
	case LDAP_SCOPE_BASE:
		if ( op->o_bd == b0 )
			return SLAP_CB_CONTINUE;

		if (op->o_bd && op->o_bd->be_search) {
			rs->sr_err = op->o_bd->be_search( op, rs );
		} else {
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		}
		return rs->sr_err;

	case LDAP_SCOPE_ONELEVEL:
	case LDAP_SCOPE_SUBTREE:
	case LDAP_SCOPE_SUBORDINATE: /* FIXME */
		op->o_callback = &cb;
		rs->sr_err = gs.err = LDAP_UNWILLING_TO_PERFORM;
		scope0 = op->ors_scope;
		tlimit0 = op->ors_tlimit;
		dn = op->o_req_dn;
		ndn = op->o_req_ndn;
		b1 = op->o_bd;

		/*
		 * Execute in reverse order, most specific first 
		 */
		for (i = gi->gi_nodes; i >= 0; i--) {
			if ( i == gi->gi_nodes ) {
				btmp = b0;
				pdn = &gi->gi_pdn;
			} else {
				btmp = gi->gi_n[i].gn_be;
				pdn = &gi->gi_n[i].gn_pdn;
			}
			if (!btmp || !btmp->be_search)
				continue;
			if (!dnIsSuffix(&btmp->be_nsuffix[0], &b1->be_nsuffix[0]))
				continue;
			if (get_no_subordinate_glue(op) && btmp != b1)
				continue;
			/* If we remembered which backend we were on before,
			 * skip down to it now
			 */
			if ( get_pagedresults( op ) > SLAP_CONTROL_IGNORED &&
				op->o_conn->c_pagedresults_state.ps_be &&
				op->o_conn->c_pagedresults_state.ps_be != btmp )
				continue;

			if (tlimit0 != SLAP_NO_LIMIT) {
				op->o_time = slap_get_time();
				op->ors_tlimit = stoptime - op->o_time;
				if (op->ors_tlimit <= 0) {
					rs->sr_err = gs.err = LDAP_TIMELIMIT_EXCEEDED;
					break;
				}
			}
			rs->sr_err = 0;
			/*
			 * check for abandon 
			 */
			if (op->o_abandon) {
				goto end_of_loop;
			}
			op->o_bd = btmp;

			assert( op->o_bd->be_suffix != NULL );
			assert( op->o_bd->be_nsuffix != NULL );
			
			if (scope0 == LDAP_SCOPE_ONELEVEL && 
				dn_match(pdn, &ndn))
			{
				struct berval mdn, mndn;
				op->ors_scope = LDAP_SCOPE_BASE;
				mdn = op->o_req_dn = op->o_bd->be_suffix[0];
				mndn = op->o_req_ndn = op->o_bd->be_nsuffix[0];
				rs->sr_err = op->o_bd->be_search(op, rs);
				if ( rs->sr_err == LDAP_NO_SUCH_OBJECT ) {
					gs.err = LDAP_SUCCESS;
				}
				op->ors_scope = LDAP_SCOPE_ONELEVEL;
				if ( op->o_req_dn.bv_val == mdn.bv_val )
					op->o_req_dn = dn;
				if ( op->o_req_ndn.bv_val == mndn.bv_val )
					op->o_req_ndn = ndn;

			} else if (scope0 == LDAP_SCOPE_SUBTREE &&
				dn_match(&op->o_bd->be_nsuffix[0], &ndn))
			{
				rs->sr_err = glue_sub_search( op, rs, b0, on );

			} else if (scope0 == LDAP_SCOPE_SUBTREE &&
				dnIsSuffix(&op->o_bd->be_nsuffix[0], &ndn))
			{
				struct berval mdn, mndn;
				mdn = op->o_req_dn = op->o_bd->be_suffix[0];
				mndn = op->o_req_ndn = op->o_bd->be_nsuffix[0];
				rs->sr_err = glue_sub_search( op, rs, b0, on );
				if ( rs->sr_err == LDAP_NO_SUCH_OBJECT ) {
					gs.err = LDAP_SUCCESS;
				}
				if ( op->o_req_dn.bv_val == mdn.bv_val )
					op->o_req_dn = dn;
				if ( op->o_req_ndn.bv_val == mndn.bv_val )
					op->o_req_ndn = ndn;

			} else if (dnIsSuffix(&ndn, &op->o_bd->be_nsuffix[0])) {
				rs->sr_err = glue_sub_search( op, rs, b0, on );
			}

			switch ( gs.err ) {

			/*
			 * Add errors that should result in dropping
			 * the search
			 */
			case LDAP_SIZELIMIT_EXCEEDED:
			case LDAP_TIMELIMIT_EXCEEDED:
			case LDAP_ADMINLIMIT_EXCEEDED:
			case LDAP_NO_SUCH_OBJECT:
#ifdef LDAP_CONTROL_X_CHAINING_BEHAVIOR
			case LDAP_X_CANNOT_CHAIN:
#endif /* LDAP_CONTROL_X_CHAINING_BEHAVIOR */
				goto end_of_loop;

			case LDAP_SUCCESS:
				if ( get_pagedresults( op ) > SLAP_CONTROL_IGNORED ) {
					PagedResultsState *ps = op->o_pagedresults_state;

					/* Assume this backend can be forgotten now */
					op->o_conn->c_pagedresults_state.ps_be = NULL;

					/* If we have a full page, exit the loop. We may
					 * need to remember this backend so we can continue
					 * from here on a subsequent request.
					 */
					if ( rs->sr_nentries >= ps->ps_size ) {
						PagedResultsState *cps = &op->o_conn->c_pagedresults_state;
						
						/* Don't bother to remember the first backend.
						 * Only remember the last one if there's more state left.
						 */
						if ( op->o_bd != b0 &&
							( cps->ps_cookie != NOID
								|| !BER_BVISNULL( &cps->ps_cookieval )
								|| op->o_bd != gi->gi_n[0].gn_be ) )
						{
							op->o_conn->c_pagedresults_state.ps_be = op->o_bd;
						}

						/* Check whether the cookie is empty,
						 * and give remaining databases a chance
						 */
						if ( op->o_bd != gi->gi_n[0].gn_be || cps->ps_cookie == NOID ) {
							int		c;

							for ( c = 0; gs.ctrls[c] != NULL; c++ ) {
								if ( strcmp( gs.ctrls[c]->ldctl_oid, LDAP_CONTROL_PAGEDRESULTS ) == 0 ) {
									break;
								}
							}

							if ( gs.ctrls[c] != NULL ) {
								BerElementBuffer berbuf;
								BerElement	*ber = (BerElement *)&berbuf;
								ber_tag_t	tag;
								ber_int_t	size;
								struct berval	cookie, value;
							
								ber_init2( ber, &gs.ctrls[c]->ldctl_value, LBER_USE_DER );

								tag = ber_scanf( ber, "{im}", &size, &cookie );
								assert( tag != LBER_ERROR );

								if ( BER_BVISEMPTY( &cookie ) && op->o_bd != gi->gi_n[0].gn_be ) {
									/* delete old, create new cookie with NOID */
									PagedResultsCookie respcookie = (PagedResultsCookie)NOID;
									ber_len_t oidlen = strlen( gs.ctrls[c]->ldctl_oid );
									LDAPControl *newctrl;

									/* it's next database's turn */
									if ( btmp == b0 ) {
										op->o_conn->c_pagedresults_state.ps_be = gi->gi_n[gi->gi_nodes - 1].gn_be;

									} else {
										op->o_conn->c_pagedresults_state.ps_be = gi->gi_n[(i > 0 ? i - 1: 0)].gn_be;
									}

									cookie.bv_val = (char *)&respcookie;
									cookie.bv_len = sizeof( PagedResultsCookie );

									ber_init2( ber, NULL, LBER_USE_DER );
									ber_printf( ber, "{iO}", 0, &cookie );
									ber_flatten2( ber, &value, 0 );

									newctrl = op->o_tmprealloc( gs.ctrls[c],
										sizeof(LDAPControl) + oidlen + 1 + value.bv_len + 1,
										op->o_tmpmemctx);
									newctrl->ldctl_iscritical = gs.ctrls[c]->ldctl_iscritical;
									newctrl->ldctl_oid = (char *)&newctrl[1];
									lutil_strcopy( newctrl->ldctl_oid, gs.ctrls[c]->ldctl_oid );
									newctrl->ldctl_value.bv_len = value.bv_len;
									lutil_memcopy( newctrl->ldctl_value.bv_val,
										value.bv_val, value.bv_len );

									gs.ctrls[c] = newctrl;

									ber_free_buf( ber );

								} else if ( !BER_BVISEMPTY( &cookie ) && op->o_bd != b0 ) {
									/* if cookie not empty, it's again this database's turn */
									op->o_conn->c_pagedresults_state.ps_be = op->o_bd;
								}
							}
						}

						goto end_of_loop;
					}

					/* This backend has run out of entries, but more responses
					 * can fit in the page. Fake a reset of the state so the
					 * next backend will start up properly. Only back-[bh]db
					 * and back-sql look at this state info.
					 */
					ps->ps_cookie = (PagedResultsCookie)0;
					BER_BVZERO( &ps->ps_cookieval );

					{
						/* change the size of the page in the request
						 * that will be propagated, and reset the cookie */
						BerElementBuffer berbuf;
						BerElement *ber = (BerElement *)&berbuf;
						int size = ps->ps_size - rs->sr_nentries;
						struct berval cookie = BER_BVC(""), value;
						int c;

						for (c = 0; op->o_ctrls[c] != NULL; c++) {
							if (strcmp(op->o_ctrls[c]->ldctl_oid, LDAP_CONTROL_PAGEDRESULTS) == 0)
								break;
						}

						assert( op->o_ctrls[c] != NULL );

						ber_init2( ber, NULL, LBER_USE_DER );
						ber_printf( ber, "{iO}", size, &cookie );
						ber_flatten2( ber, &value, 0 );
						assert( op->o_ctrls[c]->ldctl_value.bv_len >= value.bv_len );
						op->o_ctrls[c]->ldctl_value.bv_len = value.bv_len;
						lutil_memcopy( op->o_ctrls[c]->ldctl_value.bv_val,
							value.bv_val, value.bv_len );
						ber_free_buf( ber );
					}
				}
				
			default:
				break;
			}
		}
end_of_loop:;
		op->ors_scope = scope0;
		op->ors_tlimit = tlimit0;
		op->o_time = starttime;

		break;
	}

	op->o_callback = cb.sc_next;
	if ( op->o_abandon ) {
		rs->sr_err = SLAPD_ABANDON;
	} else {
		rs->sr_err = gs.err;
		rs->sr_matched = gs.matched;
		rs->sr_ref = gs.refs;
	}
	rs->sr_ctrls = gs.ctrls;

	send_ldap_result( op, rs );

	op->o_bd = b0;
	op->o_bd->bd_info = bi0;
	if (gs.matched)
		free (gs.matched);
	if (gs.refs)
		ber_bvarray_free(gs.refs);
	if (gs.ctrls) {
		for (i = gs.nctrls; --i >= 0; ) {
			op->o_tmpfree(gs.ctrls[i], op->o_tmpmemctx);
		}
		op->o_tmpfree(gs.ctrls, op->o_tmpmemctx);
	}
	return rs->sr_err;
}

static BackendDB toolDB;

static int
glue_tool_entry_open (
	BackendDB *b0,
	int mode
)
{
	slap_overinfo	*oi = (slap_overinfo *)b0->bd_info;

	/* We don't know which backend to talk to yet, so just
	 * remember the mode and move on...
	 */

	glueMode = mode;
	glueBack = NULL;
	toolDB = *b0;
	toolDB.bd_info = oi->oi_orig;

	/* Sanity checks */
	{
		slap_overinst *on = glue_tool_inst( b0->bd_info );
		glueinfo	*gi = on->on_bi.bi_private;

		int i;
		for (i = 0; i < gi->gi_nodes; i++) {
			BackendDB *bd;
			struct berval pdn;
	
			dnParent( &gi->gi_n[i].gn_be->be_nsuffix[0], &pdn );
			bd = select_backend( &pdn, 0 );
			if ( bd ) {
				ID id;
				BackendDB db;
	
				if ( overlay_is_over( bd ) ) {
					slap_overinfo *oi = (slap_overinfo *)bd->bd_info;
					db = *bd;
					db.bd_info = oi->oi_orig;
					bd = &db;
				}
	
				if ( !bd->bd_info->bi_tool_dn2id_get
					|| !bd->bd_info->bi_tool_entry_open
					|| !bd->bd_info->bi_tool_entry_close )
				{
					continue;
				}
	
				bd->bd_info->bi_tool_entry_open( bd, 0 );
				id = bd->bd_info->bi_tool_dn2id_get( bd, &gi->gi_n[i].gn_be->be_nsuffix[0] );
				bd->bd_info->bi_tool_entry_close( bd );
				if ( id != NOID ) {
					Debug( LDAP_DEBUG_ANY,
						"glue_tool_entry_open: subordinate database suffix entry DN=\"%s\" also present in superior database rooted at DN=\"%s\"\n",
						gi->gi_n[i].gn_be->be_suffix[0].bv_val, bd->be_suffix[0].bv_val, 0 );
					return LDAP_OTHER;
				}
			}
		}
	}
	
	return 0;
}

static int
glue_tool_entry_close (
	BackendDB *b0
)
{
	int rc = 0;

	if (glueBack && glueBack != GLUEBACK_DONE) {
		if (!glueBack->be_entry_close)
			return 0;
		rc = glueBack->be_entry_close (glueBack);
	}
	return rc;
}

static slap_overinst *
glue_tool_inst(
	BackendInfo *bi
)
{
	slap_overinfo	*oi = (slap_overinfo *)bi;
	slap_overinst	*on;

	for ( on = oi->oi_list; on; on=on->on_next ) {
		if ( !strcmp( on->on_bi.bi_type, glue.on_bi.bi_type ))
			return on;
	}
	return NULL;
}

/* This function will only be called in tool mode */
static int
glue_open (
	BackendInfo *bi
)
{
	slap_overinst *on = glue_tool_inst( bi );
	glueinfo		*gi = on->on_bi.bi_private;
	static int glueOpened = 0;
	int i, j, same, bsame = 0, rc = 0;
	ConfigReply cr = {0};

	if (glueOpened) return 0;

	glueOpened = 1;

	/* If we were invoked in tool mode, open all the underlying backends */
	if (slapMode & SLAP_TOOL_MODE) {
		for (i = 0; i<gi->gi_nodes; i++) {
			same = 0;
			/* Same bi_open as our main backend? */
			if ( gi->gi_n[i].gn_be->bd_info->bi_open ==
				on->on_info->oi_orig->bi_open )
				bsame = 1;

			/* Loop thru the bd_info's and make sure we only
			 * invoke their bi_open functions once each.
			 */
			for ( j = 0; j<i; j++ ) {
				if ( gi->gi_n[i].gn_be->bd_info->bi_open ==
					gi->gi_n[j].gn_be->bd_info->bi_open ) {
					same = 1;
					break;
				}
			}
			/* OK, it's unique and non-NULL, call it. */
			if ( !same && gi->gi_n[i].gn_be->bd_info->bi_open )
				rc = gi->gi_n[i].gn_be->bd_info->bi_open(
					gi->gi_n[i].gn_be->bd_info );
			/* Let backend.c take care of the rest of startup */
			if ( !rc )
				rc = backend_startup_one( gi->gi_n[i].gn_be, &cr );
			if ( rc ) break;
		}
		if ( !rc && !bsame && on->on_info->oi_orig->bi_open )
			rc = on->on_info->oi_orig->bi_open( on->on_info->oi_orig );

	} /* other case is impossible */
	return rc;
}

/* This function will only be called in tool mode */
static int
glue_close (
	BackendInfo *bi
)
{
	static int glueClosed = 0;
	int rc = 0;

	if (glueClosed) return 0;

	glueClosed = 1;

	if (slapMode & SLAP_TOOL_MODE) {
		rc = backend_shutdown( NULL );
	}
	return rc;
}

static int
glue_entry_get_rw (
	Operation		*op,
	struct berval	*dn,
	ObjectClass		*oc,
	AttributeDescription	*ad,
	int	rw,
	Entry	**e )
{
	int rc;
	BackendDB *b0 = op->o_bd;
	op->o_bd = glue_back_select( b0, dn );

	if ( op->o_bd->be_fetch ) {
		rc = op->o_bd->be_fetch( op, dn, oc, ad, rw, e );
	} else {
		rc = LDAP_UNWILLING_TO_PERFORM;
	}
	op->o_bd =b0;
	return rc;
}

static int
glue_entry_release_rw (
	Operation *op,
	Entry *e,
	int rw
)
{
	BackendDB *b0 = op->o_bd;
	int rc = -1;

	op->o_bd = glue_back_select (b0, &e->e_nname);

	if ( op->o_bd->be_release ) {
		rc = op->o_bd->be_release( op, e, rw );

	} else {
		/* FIXME: mimic be_entry_release_rw
		 * when no be_release() available */
		/* free entry */
		entry_free( e );
		rc = 0;
	}
	op->o_bd = b0;
	return rc;
}

static struct berval *glue_base;
static int glue_scope;
static Filter *glue_filter;

static ID
glue_tool_entry_first (
	BackendDB *b0
)
{
	slap_overinst	*on = glue_tool_inst( b0->bd_info );
	glueinfo		*gi = on->on_bi.bi_private;
	int i;
	ID rc;

	/* If we're starting from scratch, start at the most general */
	if (!glueBack) {
		if ( toolDB.be_entry_open && toolDB.be_entry_first ) {
			glueBack = &toolDB;
		} else {
			for (i = gi->gi_nodes-1; i >= 0; i--) {
				if (gi->gi_n[i].gn_be->be_entry_open &&
					gi->gi_n[i].gn_be->be_entry_first) {
						glueBack = gi->gi_n[i].gn_be;
					break;
				}
			}
		}
	}
	if (!glueBack || !glueBack->be_entry_open || !glueBack->be_entry_first ||
		glueBack->be_entry_open (glueBack, glueMode) != 0)
		return NOID;

	rc = glueBack->be_entry_first (glueBack);
	while ( rc == NOID ) {
		if ( glueBack && glueBack->be_entry_close )
			glueBack->be_entry_close (glueBack);
		for (i=0; i<gi->gi_nodes; i++) {
			if (gi->gi_n[i].gn_be == glueBack)
				break;
		}
		if (i == 0) {
			glueBack = GLUEBACK_DONE;
			break;
		} else {
			glueBack = gi->gi_n[i-1].gn_be;
			rc = glue_tool_entry_first (b0);
			if ( glueBack == GLUEBACK_DONE ) {
				break;
			}
		}
	}
	return rc;
}

static ID
glue_tool_entry_first_x (
	BackendDB *b0,
	struct berval *base,
	int scope,
	Filter *f
)
{
	slap_overinst	*on = glue_tool_inst( b0->bd_info );
	glueinfo		*gi = on->on_bi.bi_private;
	int i;
	ID rc;

	glue_base = base;
	glue_scope = scope;
	glue_filter = f;

	/* If we're starting from scratch, start at the most general */
	if (!glueBack) {
		if ( toolDB.be_entry_open && toolDB.be_entry_first_x ) {
			glueBack = &toolDB;
		} else {
			for (i = gi->gi_nodes-1; i >= 0; i--) {
				if (gi->gi_n[i].gn_be->be_entry_open &&
					gi->gi_n[i].gn_be->be_entry_first_x)
				{
					glueBack = gi->gi_n[i].gn_be;
					break;
				}
			}
		}
	}
	if (!glueBack || !glueBack->be_entry_open || !glueBack->be_entry_first_x ||
		glueBack->be_entry_open (glueBack, glueMode) != 0)
		return NOID;

	rc = glueBack->be_entry_first_x (glueBack,
		glue_base, glue_scope, glue_filter);
	while ( rc == NOID ) {
		if ( glueBack && glueBack->be_entry_close )
			glueBack->be_entry_close (glueBack);
		for (i=0; i<gi->gi_nodes; i++) {
			if (gi->gi_n[i].gn_be == glueBack)
				break;
		}
		if (i == 0) {
			glueBack = GLUEBACK_DONE;
			break;
		} else {
			glueBack = gi->gi_n[i-1].gn_be;
			rc = glue_tool_entry_first_x (b0,
				glue_base, glue_scope, glue_filter);
			if ( glueBack == GLUEBACK_DONE ) {
				break;
			}
		}
	}
	return rc;
}

static ID
glue_tool_entry_next (
	BackendDB *b0
)
{
	slap_overinst	*on = glue_tool_inst( b0->bd_info );
	glueinfo		*gi = on->on_bi.bi_private;
	int i;
	ID rc;

	if (!glueBack || !glueBack->be_entry_next)
		return NOID;

	rc = glueBack->be_entry_next (glueBack);

	/* If we ran out of entries in one database, move on to the next */
	while (rc == NOID) {
		if ( glueBack && glueBack->be_entry_close )
			glueBack->be_entry_close (glueBack);
		for (i=0; i<gi->gi_nodes; i++) {
			if (gi->gi_n[i].gn_be == glueBack)
				break;
		}
		if (i == 0) {
			glueBack = GLUEBACK_DONE;
			break;
		} else {
			glueBack = gi->gi_n[i-1].gn_be;
			if ( glue_base || glue_filter ) {
				/* using entry_first_x() */
				rc = glue_tool_entry_first_x (b0,
					glue_base, glue_scope, glue_filter);

			} else {
				/* using entry_first() */
				rc = glue_tool_entry_first (b0);
			}
			if ( glueBack == GLUEBACK_DONE ) {
				break;
			}
		}
	}
	return rc;
}

static ID
glue_tool_dn2id_get (
	BackendDB *b0,
	struct berval *dn
)
{
	BackendDB *be, b2;
	int rc = -1;

	b2 = *b0;
	b2.bd_info = (BackendInfo *)glue_tool_inst( b0->bd_info );
	be = glue_back_select (&b2, dn);
	if ( be == &b2 ) be = &toolDB;

	if (!be->be_dn2id_get)
		return NOID;

	if (!glueBack) {
		if ( be->be_entry_open ) {
			rc = be->be_entry_open (be, glueMode);
		}
		if (rc != 0) {
			return NOID;
		}
	} else if (be != glueBack) {
		/* If this entry belongs in a different branch than the
		 * previous one, close the current database and open the
		 * new one.
		 */
		if ( glueBack->be_entry_close ) {
			glueBack->be_entry_close (glueBack);
		}
		if ( be->be_entry_open ) {
			rc = be->be_entry_open (be, glueMode);
		}
		if (rc != 0) {
			return NOID;
		}
	}
	glueBack = be;
	return be->be_dn2id_get (be, dn);
}

static Entry *
glue_tool_entry_get (
	BackendDB *b0,
	ID id
)
{
	if (!glueBack || !glueBack->be_entry_get)
		return NULL;

	return glueBack->be_entry_get (glueBack, id);
}

static ID
glue_tool_entry_put (
	BackendDB *b0,
	Entry *e,
	struct berval *text
)
{
	BackendDB *be, b2;
	int rc = -1;

	b2 = *b0;
	b2.bd_info = (BackendInfo *)glue_tool_inst( b0->bd_info );
	be = glue_back_select (&b2, &e->e_nname);
	if ( be == &b2 ) be = &toolDB;

	if (!be->be_entry_put)
		return NOID;

	if (!glueBack) {
		if ( be->be_entry_open ) {
			rc = be->be_entry_open (be, glueMode);
		}
		if (rc != 0) {
			return NOID;
		}
	} else if (be != glueBack) {
		/* If this entry belongs in a different branch than the
		 * previous one, close the current database and open the
		 * new one.
		 */
		if ( glueBack->be_entry_close ) {
			glueBack->be_entry_close (glueBack);
		}
		if ( be->be_entry_open ) {
			rc = be->be_entry_open (be, glueMode);
		}
		if (rc != 0) {
			return NOID;
		}
	}
	glueBack = be;
	return be->be_entry_put (be, e, text);
}

static ID
glue_tool_entry_modify (
	BackendDB *b0,
	Entry *e,
	struct berval *text
)
{
	if (!glueBack || !glueBack->be_entry_modify)
		return NOID;

	return glueBack->be_entry_modify (glueBack, e, text);
}

static int
glue_tool_entry_reindex (
	BackendDB *b0,
	ID id,
	AttributeDescription **adv
)
{
	if (!glueBack || !glueBack->be_entry_reindex)
		return -1;

	return glueBack->be_entry_reindex (glueBack, id, adv);
}

static int
glue_tool_sync (
	BackendDB *b0
)
{
	slap_overinst	*on = glue_tool_inst( b0->bd_info );
	glueinfo		*gi = on->on_bi.bi_private;
	BackendInfo		*bi = b0->bd_info;
	int i;

	/* just sync everyone */
	for (i = 0; i<gi->gi_nodes; i++)
		if (gi->gi_n[i].gn_be->be_sync)
			gi->gi_n[i].gn_be->be_sync (gi->gi_n[i].gn_be);
	b0->bd_info = on->on_info->oi_orig;
	if ( b0->be_sync )
		b0->be_sync( b0 );
	b0->bd_info = bi;
	return 0;
}

typedef struct glue_Addrec {
	struct glue_Addrec *ga_next;
	BackendDB *ga_be;
} glue_Addrec;

/* List of added subordinates */
static glue_Addrec *ga_list;
static int ga_adding;

static int
glue_db_init(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	slap_overinfo	*oi = on->on_info;
	BackendInfo	*bi = oi->oi_orig;
	glueinfo *gi;

	if ( SLAP_GLUE_SUBORDINATE( be )) {
		Debug( LDAP_DEBUG_ANY, "glue: backend %s is already subordinate, "
			"cannot have glue overlay!\n",
			be->be_suffix[0].bv_val, 0, 0 );
		return LDAP_OTHER;
	}

	gi = ch_calloc( 1, sizeof(glueinfo));
	on->on_bi.bi_private = gi;
	dnParent( be->be_nsuffix, &gi->gi_pdn );

	/* Currently the overlay framework doesn't handle these entry points
	 * but we need them....
	 */
	oi->oi_bi.bi_open = glue_open;
	oi->oi_bi.bi_close = glue_close;

	/* Only advertise these if the root DB supports them */
	if ( bi->bi_tool_entry_open )
		oi->oi_bi.bi_tool_entry_open = glue_tool_entry_open;
	if ( bi->bi_tool_entry_close )
		oi->oi_bi.bi_tool_entry_close = glue_tool_entry_close;
	if ( bi->bi_tool_entry_first )
		oi->oi_bi.bi_tool_entry_first = glue_tool_entry_first;
	/* FIXME: check whether all support bi_tool_entry_first_x() ? */
	if ( bi->bi_tool_entry_first_x )
		oi->oi_bi.bi_tool_entry_first_x = glue_tool_entry_first_x;
	if ( bi->bi_tool_entry_next )
		oi->oi_bi.bi_tool_entry_next = glue_tool_entry_next;
	if ( bi->bi_tool_entry_get )
		oi->oi_bi.bi_tool_entry_get = glue_tool_entry_get;
	if ( bi->bi_tool_dn2id_get )
		oi->oi_bi.bi_tool_dn2id_get = glue_tool_dn2id_get;
	if ( bi->bi_tool_entry_put )
		oi->oi_bi.bi_tool_entry_put = glue_tool_entry_put;
	if ( bi->bi_tool_entry_reindex )
		oi->oi_bi.bi_tool_entry_reindex = glue_tool_entry_reindex;
	if ( bi->bi_tool_entry_modify )
		oi->oi_bi.bi_tool_entry_modify = glue_tool_entry_modify;
	if ( bi->bi_tool_sync )
		oi->oi_bi.bi_tool_sync = glue_tool_sync;

	SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_GLUE_INSTANCE;

	if ( ga_list ) {
		be->bd_info = (BackendInfo *)oi;
		glue_sub_attach( 1 );
	}

	return 0;
}

static int
glue_db_destroy (
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	glueinfo		*gi = (glueinfo *)on->on_bi.bi_private;

	free (gi);
	return SLAP_CB_CONTINUE;
}

static int
glue_db_close( 
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;

	on->on_info->oi_bi.bi_db_close = 0;
	return 0;
}

int
glue_sub_del( BackendDB *b0 )
{
	BackendDB *be;
	int rc = 0;

	/* Find the top backend for this subordinate */
	be = b0;
	while ( (be=LDAP_STAILQ_NEXT( be, be_next )) != NULL ) {
		slap_overinfo *oi;
		slap_overinst *on;
		glueinfo *gi;
		int i;

		if ( SLAP_GLUE_SUBORDINATE( be ))
			continue;
		if ( !SLAP_GLUE_INSTANCE( be ))
			continue;
		if ( !dnIsSuffix( &b0->be_nsuffix[0], &be->be_nsuffix[0] ))
			continue;

		/* OK, got the right backend, find the overlay */
		oi = (slap_overinfo *)be->bd_info;
		for ( on=oi->oi_list; on; on=on->on_next ) {
			if ( on->on_bi.bi_type == glue.on_bi.bi_type )
				break;
		}
		assert( on != NULL );
		gi = on->on_bi.bi_private;
		for ( i=0; i < gi->gi_nodes; i++ ) {
			if ( gi->gi_n[i].gn_be == b0 ) {
				int j;

				for (j=i+1; j < gi->gi_nodes; j++)
					gi->gi_n[j-1] = gi->gi_n[j];

				gi->gi_nodes--;
			}
		}
	}
	if ( be == NULL )
		rc = LDAP_NO_SUCH_OBJECT;

	return rc;
}


/* Attach all the subordinate backends to their superior */
int
glue_sub_attach( int online )
{
	glue_Addrec *ga, *gnext = NULL;
	int rc = 0;

	if ( ga_adding )
		return 0;

	ga_adding = 1;

	/* For all the subordinate backends */
	for ( ga=ga_list; ga != NULL; ga = gnext ) {
		BackendDB *be;

		gnext = ga->ga_next;

		/* Find the top backend for this subordinate */
		be = ga->ga_be;
		while ( (be=LDAP_STAILQ_NEXT( be, be_next )) != NULL ) {
			slap_overinfo *oi;
			slap_overinst *on;
			glueinfo *gi;

			if ( SLAP_GLUE_SUBORDINATE( be ))
				continue;
			if ( !dnIsSuffix( &ga->ga_be->be_nsuffix[0], &be->be_nsuffix[0] ))
				continue;

			/* If it's not already configured, set up the overlay */
			if ( !SLAP_GLUE_INSTANCE( be )) {
				rc = overlay_config( be, glue.on_bi.bi_type, -1, NULL, NULL);
				if ( rc )
					break;
			}
			/* Find the overlay instance */
			oi = (slap_overinfo *)be->bd_info;
			for ( on=oi->oi_list; on; on=on->on_next ) {
				if ( on->on_bi.bi_type == glue.on_bi.bi_type )
					break;
			}
			assert( on != NULL );
			gi = on->on_bi.bi_private;
			gi = (glueinfo *)ch_realloc( gi, sizeof(glueinfo) +
				gi->gi_nodes * sizeof(gluenode));
			gi->gi_n[gi->gi_nodes].gn_be = ga->ga_be;
			dnParent( &ga->ga_be->be_nsuffix[0],
				&gi->gi_n[gi->gi_nodes].gn_pdn );
			gi->gi_nodes++;
			on->on_bi.bi_private = gi;
			ga->ga_be->be_flags |= SLAP_DBFLAG_GLUE_LINKED;
			break;
		}
		if ( !be ) {
			Debug( LDAP_DEBUG_ANY, "glue: no superior found for sub %s!\n",
				ga->ga_be->be_suffix[0].bv_val, 0, 0 );
			/* allow this for now, assume a superior will
			 * be added later
			 */
			if ( online ) {
				rc = 0;
				gnext = ga_list;
				break;
			}
			rc = LDAP_NO_SUCH_OBJECT;
		}
		ch_free( ga );
		if ( rc ) break;
	}

	ga_list = gnext;

	ga_adding = 0;

	return rc;
}

int
glue_sub_add( BackendDB *be, int advert, int online )
{
	glue_Addrec *ga;
	int rc = 0;

	if ( overlay_is_inst( be, "glue" )) {
		Debug( LDAP_DEBUG_ANY, "glue: backend %s already has glue overlay, "
			"cannot be a subordinate!\n",
			be->be_suffix[0].bv_val, 0, 0 );
		return LDAP_OTHER;
	}
	SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_GLUE_SUBORDINATE;
	if ( advert )
		SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_GLUE_ADVERTISE;

	ga = ch_malloc( sizeof( glue_Addrec ));
	ga->ga_next = ga_list;
	ga->ga_be = be;
	ga_list = ga;

	if ( online )
		rc = glue_sub_attach( online );

	return rc;
}

static int
glue_access_allowed(
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	slap_access_t		access,
	AccessControlState	*state,
	slap_mask_t		*maskp )
{
	BackendDB *b0, *be = glue_back_select( op->o_bd, &e->e_nname );
	int rc;

	if ( be == NULL || be == op->o_bd || be->bd_info->bi_access_allowed == NULL )
		return SLAP_CB_CONTINUE;

	b0 = op->o_bd;
	op->o_bd = be;
	rc = be->bd_info->bi_access_allowed ( op, e, desc, val, access, state, maskp );
	op->o_bd = b0;
	return rc;
}

int
glue_sub_init()
{
	glue.on_bi.bi_type = "glue";

	glue.on_bi.bi_db_init = glue_db_init;
	glue.on_bi.bi_db_close = glue_db_close;
	glue.on_bi.bi_db_destroy = glue_db_destroy;

	glue.on_bi.bi_op_search = glue_op_search;
	glue.on_bi.bi_op_modify = glue_op_func;
	glue.on_bi.bi_op_modrdn = glue_op_func;
	glue.on_bi.bi_op_add = glue_op_func;
	glue.on_bi.bi_op_delete = glue_op_func;
	glue.on_bi.bi_op_abandon = glue_op_abandon;
	glue.on_bi.bi_extended = glue_op_func;

	glue.on_bi.bi_chk_referrals = glue_chk_referrals;
	glue.on_bi.bi_chk_controls = glue_chk_controls;
	glue.on_bi.bi_entry_get_rw = glue_entry_get_rw;
	glue.on_bi.bi_entry_release_rw = glue_entry_release_rw;
	glue.on_bi.bi_access_allowed = glue_access_allowed;

	glue.on_response = glue_response;

	return overlay_register( &glue );
}
