/* search.c - search operation */
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

#include "back-mdb.h"
#include "idl.h"

static int base_candidate(
	BackendDB	*be,
	Entry	*e,
	ID		*ids );

static int search_candidates(
	Operation *op,
	SlapReply *rs,
	Entry *e,
	MDB_txn *txn,
	MDB_cursor *mci,
	ID	*ids,
	ID2L	scopes );

static int parse_paged_cookie( Operation *op, SlapReply *rs );

static void send_paged_response( 
	Operation *op,
	SlapReply *rs,
	ID  *lastid,
	int tentries );

/* Dereference aliases for a single alias entry. Return the final
 * dereferenced entry on success, NULL on any failure.
 */
static Entry * deref_base (
	Operation *op,
	SlapReply *rs,
	Entry *e,
	Entry **matched,
	MDB_txn *txn,
	ID	*tmp,
	ID	*visited )
{
	struct berval ndn;

	rs->sr_err = LDAP_ALIAS_DEREF_PROBLEM;
	rs->sr_text = "maximum deref depth exceeded";

	for (;;) {
		/* Remember the last entry we looked at, so we can
		 * report broken links
		 */
		*matched = e;

		if (MDB_IDL_N(tmp) >= op->o_bd->be_max_deref_depth) {
			e = NULL;
			break;
		}

		/* If this is part of a subtree or onelevel search,
		 * have we seen this ID before? If so, quit.
		 */
		if ( visited && mdb_idl_insert( visited, e->e_id ) ) {
			e = NULL;
			break;
		}

		/* If we've seen this ID during this deref iteration,
		 * we've hit a loop.
		 */
		if ( mdb_idl_insert( tmp, e->e_id ) ) {
			rs->sr_err = LDAP_ALIAS_PROBLEM;
			rs->sr_text = "circular alias";
			e = NULL;
			break;
		}

		/* If there was a problem getting the aliasedObjectName,
		 * get_alias_dn will have set the error status.
		 */
		if ( get_alias_dn(e, &ndn, &rs->sr_err, &rs->sr_text) ) {
			e = NULL;
			break;
		}

		rs->sr_err = mdb_dn2entry( op, txn, NULL, &ndn, &e, NULL, 0 );
		if (rs->sr_err) {
			rs->sr_err = LDAP_ALIAS_PROBLEM;
			rs->sr_text = "aliasedObject not found";
			break;
		}

		/* Free the previous entry, continue to work with the
		 * one we just retrieved.
		 */
		mdb_entry_return( op, *matched );

		/* We found a regular entry. Return this to the caller.
		 */
		if (!is_entry_alias(e)) {
			rs->sr_err = LDAP_SUCCESS;
			rs->sr_text = NULL;
			break;
		}
	}
	return e;
}

/* Look for and dereference all aliases within the search scope.
 * Requires "stack" to be able to hold 6 levels of DB_SIZE IDLs.
 * Of course we're hardcoded to require a minimum of 8 UM_SIZE
 * IDLs so this is never a problem.
 */
static int search_aliases(
	Operation *op,
	SlapReply *rs,
	ID e_id,
	MDB_txn *txn,
	MDB_cursor *mci,
	ID2L scopes,
	ID *stack )
{
	ID *aliases, *curscop, *visited, *newsubs, *oldsubs, *tmp;
	ID cursora, ida, cursoro, ido;
	Entry *matched, *a;
	struct berval bv_alias = BER_BVC( "alias" );
	AttributeAssertion aa_alias = ATTRIBUTEASSERTION_INIT;
	Filter	af;
	int first = 1;

	aliases = stack;	/* IDL of all aliases in the database */
	curscop = aliases + MDB_IDL_DB_SIZE;	/* Aliases in the current scope */
	visited = curscop + MDB_IDL_DB_SIZE;	/* IDs we've seen in this search */
	newsubs = visited + MDB_IDL_DB_SIZE;	/* New subtrees we've added */
	oldsubs = newsubs + MDB_IDL_DB_SIZE;	/* Subtrees added previously */
	tmp = oldsubs + MDB_IDL_DB_SIZE;	/* Scratch space for deref_base() */

	af.f_choice = LDAP_FILTER_EQUALITY;
	af.f_ava = &aa_alias;
	af.f_av_desc = slap_schema.si_ad_objectClass;
	af.f_av_value = bv_alias;
	af.f_next = NULL;

	/* Find all aliases in database */
	MDB_IDL_ZERO( aliases );
	rs->sr_err = mdb_filter_candidates( op, txn, &af, aliases,
		curscop, visited );
	if (rs->sr_err != LDAP_SUCCESS || MDB_IDL_IS_ZERO( aliases )) {
		return rs->sr_err;
	}
	oldsubs[0] = 1;
	oldsubs[1] = e_id;

	MDB_IDL_ZERO( visited );
	MDB_IDL_ZERO( newsubs );

	cursoro = 0;
	ido = mdb_idl_first( oldsubs, &cursoro );

	for (;;) {
		/* Set curscop to only the aliases in the current scope. Start with
		 * all the aliases, then get the intersection with the scope.
		 */
		rs->sr_err = mdb_idscope( op, txn, e_id, aliases, curscop );

		/* Dereference all of the aliases in the current scope. */
		cursora = 0;
		for (ida = mdb_idl_first(curscop, &cursora); ida != NOID;
			ida = mdb_idl_next(curscop, &cursora))
		{
			rs->sr_err = mdb_id2entry(op, mci, ida, &a);
			if (rs->sr_err != LDAP_SUCCESS) {
				continue;
			}

			/* This should only happen if the curscop IDL has maxed out and
			 * turned into a range that spans IDs indiscriminately
			 */
			if (!is_entry_alias(a)) {
				mdb_entry_return(op, a);
				continue;
			}

			/* Actually dereference the alias */
			MDB_IDL_ZERO(tmp);
			a = deref_base( op, rs, a, &matched, txn,
				tmp, visited );
			if (a) {
				/* If the target was not already in our current scopes,
				 * make note of it in the newsubs list.
				 */
				ID2 mid;
				mid.mid = a->e_id;
				mid.mval.mv_data = NULL;
				if (mdb_id2l_insert(scopes, &mid) == 0) {
					mdb_idl_insert(newsubs, a->e_id);
				}
				mdb_entry_return( op, a );

			} else if (matched) {
				/* Alias could not be dereferenced, or it deref'd to
				 * an ID we've already seen. Ignore it.
				 */
				mdb_entry_return( op, matched );
				rs->sr_text = NULL;
				rs->sr_err = 0;
			}
		}
		/* If this is a OneLevel search, we're done; oldsubs only had one
		 * ID in it. For a Subtree search, oldsubs may be a list of scope IDs.
		 */
		if ( op->ors_scope == LDAP_SCOPE_ONELEVEL ) break;
nextido:
		ido = mdb_idl_next( oldsubs, &cursoro );
		
		/* If we're done processing the old scopes, did we add any new
		 * scopes in this iteration? If so, go back and do those now.
		 */
		if (ido == NOID) {
			if (MDB_IDL_IS_ZERO(newsubs)) break;
			MDB_IDL_CPY(oldsubs, newsubs);
			MDB_IDL_ZERO(newsubs);
			cursoro = 0;
			ido = mdb_idl_first( oldsubs, &cursoro );
		}

		/* Find the entry corresponding to the next scope. If it can't
		 * be found, ignore it and move on. This should never happen;
		 * we should never see the ID of an entry that doesn't exist.
		 */
		{
			MDB_val edata;
			rs->sr_err = mdb_id2edata(op, mci, ido, &edata);
			if ( rs->sr_err != MDB_SUCCESS ) {
				goto nextido;
			}
			e_id = ido;
		}
	}
	return rs->sr_err;
}

/* Get the next ID from the DB. Used if the candidate list is
 * a range and simple iteration hits missing entryIDs
 */
static int
mdb_get_nextid(MDB_cursor *mci, ID *cursor)
{
	MDB_val key;
	ID id;
	int rc;

	id = *cursor + 1;
	key.mv_data = &id;
	key.mv_size = sizeof(ID);
	rc = mdb_cursor_get( mci, &key, NULL, MDB_SET_RANGE );
	if ( rc )
		return rc;
	memcpy( cursor, key.mv_data, sizeof(ID));
	return 0;
}

static void scope_chunk_free( void *key, void *data )
{
	ID2 *p1, *p2;
	for (p1 = data; p1; p1 = p2) {
		p2 = p1[0].mval.mv_data;
		ber_memfree_x(p1, NULL);
	}
}

static ID2 *scope_chunk_get( Operation *op )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	ID2 *ret = NULL;

	ldap_pvt_thread_pool_getkey( op->o_threadctx, (void *)scope_chunk_get,
			(void *)&ret, NULL );
	if ( !ret ) {
		ret = ch_malloc( MDB_IDL_UM_SIZE * sizeof( ID2 ));
	} else {
		void *r2 = ret[0].mval.mv_data;
		ldap_pvt_thread_pool_setkey( op->o_threadctx, (void *)scope_chunk_get,
			r2, scope_chunk_free, NULL, NULL );
	}
	return ret;
}

static void scope_chunk_ret( Operation *op, ID2 *scopes )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	void *ret = NULL;

	ldap_pvt_thread_pool_getkey( op->o_threadctx, (void *)scope_chunk_get,
			&ret, NULL );
	scopes[0].mval.mv_data = ret;
	ldap_pvt_thread_pool_setkey( op->o_threadctx, (void *)scope_chunk_get,
			(void *)scopes, scope_chunk_free, NULL, NULL );
}

int
mdb_search( Operation *op, SlapReply *rs )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	ID		id, cursor, nsubs, ncand, cscope;
	ID		lastid = NOID;
	ID		candidates[MDB_IDL_UM_SIZE];
	ID		iscopes[MDB_IDL_DB_SIZE];
	ID2		*scopes;
	Entry		*e = NULL, *base = NULL;
	Entry		*matched = NULL;
	AttributeName	*attrs;
	slap_mask_t	mask;
	time_t		stoptime;
	int		manageDSAit;
	int		tentries = 0;
	IdScopes	isc;
	MDB_cursor	*mci, *mcd;

	mdb_op_info	opinfo = {{{0}}}, *moi = &opinfo;
	MDB_txn			*ltid = NULL;

	Debug( LDAP_DEBUG_TRACE, "=> " LDAP_XSTRING(mdb_search) "\n", 0, 0, 0);
	attrs = op->oq_search.rs_attrs;

	manageDSAit = get_manageDSAit( op );

	rs->sr_err = mdb_opinfo_get( op, mdb, 1, &moi );
	switch(rs->sr_err) {
	case 0:
		break;
	default:
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		return rs->sr_err;
	}

	ltid = moi->moi_txn;

	rs->sr_err = mdb_cursor_open( ltid, mdb->mi_id2entry, &mci );
	if ( rs->sr_err ) {
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		return rs->sr_err;
	}

	rs->sr_err = mdb_cursor_open( ltid, mdb->mi_dn2id, &mcd );
	if ( rs->sr_err ) {
		mdb_cursor_close( mci );
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		return rs->sr_err;
	}

	scopes = scope_chunk_get( op );
	isc.mt = ltid;
	isc.mc = mcd;
	isc.scopes = scopes;
	isc.oscope = op->ors_scope;

	if ( op->ors_deref & LDAP_DEREF_FINDING ) {
		MDB_IDL_ZERO(candidates);
	}
dn2entry_retry:
	/* get entry with reader lock */
	rs->sr_err = mdb_dn2entry( op, ltid, mcd, &op->o_req_ndn, &e, &nsubs, 1 );

	switch(rs->sr_err) {
	case MDB_NOTFOUND:
		matched = e;
		e = NULL;
		break;
	case 0:
		break;
	case LDAP_BUSY:
		send_ldap_error( op, rs, LDAP_BUSY, "ldap server busy" );
		goto done;
	default:
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		goto done;
	}

	if ( op->ors_deref & LDAP_DEREF_FINDING ) {
		if ( matched && is_entry_alias( matched )) {
			struct berval stub;

			stub.bv_val = op->o_req_ndn.bv_val;
			stub.bv_len = op->o_req_ndn.bv_len - matched->e_nname.bv_len - 1;
			e = deref_base( op, rs, matched, &matched, ltid,
				candidates, NULL );
			if ( e ) {
				build_new_dn( &op->o_req_ndn, &e->e_nname, &stub,
					op->o_tmpmemctx );
				mdb_entry_return(op, e);
				matched = NULL;
				goto dn2entry_retry;
			}
		} else if ( e && is_entry_alias( e )) {
			e = deref_base( op, rs, e, &matched, ltid,
				candidates, NULL );
		}
	}

	if ( e == NULL ) {
		struct berval matched_dn = BER_BVNULL;

		if ( matched != NULL ) {
			BerVarray erefs = NULL;

			/* return referral only if "disclose"
			 * is granted on the object */
			if ( ! access_allowed( op, matched,
						slap_schema.si_ad_entry,
						NULL, ACL_DISCLOSE, NULL ) )
			{
				rs->sr_err = LDAP_NO_SUCH_OBJECT;

			} else {
				ber_dupbv( &matched_dn, &matched->e_name );

				erefs = is_entry_referral( matched )
					? get_entry_referrals( op, matched )
					: NULL;
				if ( rs->sr_err == MDB_NOTFOUND )
					rs->sr_err = LDAP_REFERRAL;
				rs->sr_matched = matched_dn.bv_val;
			}

			mdb_entry_return(op, matched);
			matched = NULL;

			if ( erefs ) {
				rs->sr_ref = referral_rewrite( erefs, &matched_dn,
					&op->o_req_dn, op->oq_search.rs_scope );
				ber_bvarray_free( erefs );
			}

		} else {
			rs->sr_ref = referral_rewrite( default_referral,
				NULL, &op->o_req_dn, op->oq_search.rs_scope );
			rs->sr_err = rs->sr_ref != NULL ? LDAP_REFERRAL : LDAP_NO_SUCH_OBJECT;
		}

		send_ldap_result( op, rs );

		if ( rs->sr_ref ) {
			ber_bvarray_free( rs->sr_ref );
			rs->sr_ref = NULL;
		}
		if ( !BER_BVISNULL( &matched_dn ) ) {
			ber_memfree( matched_dn.bv_val );
			rs->sr_matched = NULL;
		}
		goto done;
	}

	/* NOTE: __NEW__ "search" access is required
	 * on searchBase object */
	if ( ! access_allowed_mask( op, e, slap_schema.si_ad_entry,
				NULL, ACL_SEARCH, NULL, &mask ) )
	{
		if ( !ACL_GRANT( mask, ACL_DISCLOSE ) ) {
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		} else {
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		}

		mdb_entry_return( op,e);
		send_ldap_result( op, rs );
		goto done;
	}

	if ( !manageDSAit && is_entry_referral( e ) ) {
		/* entry is a referral */
		struct berval matched_dn = BER_BVNULL;
		BerVarray erefs = NULL;
		
		ber_dupbv( &matched_dn, &e->e_name );
		erefs = get_entry_referrals( op, e );

		rs->sr_err = LDAP_REFERRAL;

		mdb_entry_return( op, e );
		e = NULL;

		if ( erefs ) {
			rs->sr_ref = referral_rewrite( erefs, &matched_dn,
				&op->o_req_dn, op->oq_search.rs_scope );
			ber_bvarray_free( erefs );

			if ( !rs->sr_ref ) {
				rs->sr_text = "bad_referral object";
			}
		}

		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_search) ": entry is referral\n",
			0, 0, 0 );

		rs->sr_matched = matched_dn.bv_val;
		send_ldap_result( op, rs );

		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
		ber_memfree( matched_dn.bv_val );
		rs->sr_matched = NULL;
		goto done;
	}

	if ( get_assert( op ) &&
		( test_filter( op, e, get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
		mdb_entry_return( op,e);
		send_ldap_result( op, rs );
		goto done;
	}

	/* compute it anyway; root does not use it */
	stoptime = op->o_time + op->ors_tlimit;

	base = e;

	e = NULL;

	/* select candidates */
	if ( op->oq_search.rs_scope == LDAP_SCOPE_BASE ) {
		rs->sr_err = base_candidate( op->o_bd, base, candidates );
		scopes[0].mid = 0;
		ncand = 1;
	} else {
		if ( op->ors_scope == LDAP_SCOPE_ONELEVEL ) {
			size_t nkids;
			MDB_val key, data;
			key.mv_data = &base->e_id;
			key.mv_size = sizeof( ID );
			mdb_cursor_get( mcd, &key, &data, MDB_SET );
			mdb_cursor_count( mcd, &nkids );
			nsubs = nkids - 1;
		} else if ( !base->e_id ) {
			/* we don't maintain nsubs for entryID 0.
			 * just grab entry count from id2entry stat
			 */
			MDB_stat ms;
			mdb_stat( ltid, mdb->mi_id2entry, &ms );
			nsubs = ms.ms_entries;
		}
		MDB_IDL_ZERO( candidates );
		scopes[0].mid = 1;
		scopes[1].mid = base->e_id;
		scopes[1].mval.mv_data = NULL;
		rs->sr_err = search_candidates( op, rs, base,
			ltid, mci, candidates, scopes );
		ncand = MDB_IDL_N( candidates );
		if ( !base->e_id || ncand == NOID ) {
			/* grab entry count from id2entry stat
			 */
			MDB_stat ms;
			mdb_stat( ltid, mdb->mi_id2entry, &ms );
			if ( !base->e_id )
				nsubs = ms.ms_entries;
			if ( ncand == NOID )
				ncand = ms.ms_entries;
		}
	}

	/* start cursor at beginning of candidates.
	 */
	cursor = 0;

	if ( candidates[0] == 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_search) ": no candidates\n",
			0, 0, 0 );

		goto nochange;
	}

	/* if not root and candidates exceed to-be-checked entries, abort */
	if ( op->ors_limit	/* isroot == FALSE */ &&
		op->ors_limit->lms_s_unchecked != -1 &&
		ncand > (unsigned) op->ors_limit->lms_s_unchecked )
	{
		rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
		send_ldap_result( op, rs );
		rs->sr_err = LDAP_SUCCESS;
		goto done;
	}

	if ( op->ors_limit == NULL	/* isroot == TRUE */ ||
		!op->ors_limit->lms_s_pr_hide )
	{
		tentries = ncand;
	}

	if ( get_pagedresults( op ) > SLAP_CONTROL_IGNORED ) {
		PagedResultsState *ps = op->o_pagedresults_state;
		/* deferred cookie parsing */
		rs->sr_err = parse_paged_cookie( op, rs );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			send_ldap_result( op, rs );
			goto done;
		}

		cursor = (ID) ps->ps_cookie;
		if ( cursor && ps->ps_size == 0 ) {
			rs->sr_err = LDAP_SUCCESS;
			rs->sr_text = "search abandoned by pagedResult size=0";
			send_ldap_result( op, rs );
			goto done;
		}
		id = mdb_idl_first( candidates, &cursor );
		if ( id == NOID ) {
			Debug( LDAP_DEBUG_TRACE, 
				LDAP_XSTRING(mdb_search)
				": no paged results candidates\n",
				0, 0, 0 );
			send_paged_response( op, rs, &lastid, 0 );

			rs->sr_err = LDAP_OTHER;
			goto done;
		}
		if ( id == (ID)ps->ps_cookie )
			id = mdb_idl_next( candidates, &cursor );
		nsubs = ncand;	/* always bypass scope'd search */
		goto loop_begin;
	}
	if ( nsubs < ncand ) {
		int rc;
		/* Do scope-based search */

		/* if any alias scopes were set, save them */
		if (scopes[0].mid > 1) {
			cursor = 1;
			for (cscope = 1; cscope <= scopes[0].mid; cscope++) {
				/* Ignore the original base */
				if (scopes[cscope].mid == base->e_id)
					continue;
				iscopes[cursor++] = scopes[cscope].mid;
			}
			iscopes[0] = scopes[0].mid - 1;
		} else {
			iscopes[0] = 0;
		}

		isc.id = base->e_id;
		isc.numrdns = 0;
		rc = mdb_dn2id_walk( op, &isc );
		if ( rc )
			id = NOID;
		else
			id = isc.id;
		cscope = 0;
	} else {
		id = mdb_idl_first( candidates, &cursor );
	}

	while (id != NOID)
	{
		int scopeok;
		MDB_val edata;

loop_begin:

		/* check for abandon */
		if ( op->o_abandon ) {
			rs->sr_err = SLAPD_ABANDON;
			send_ldap_result( op, rs );
			goto done;
		}

		/* mostly needed by internal searches,
		 * e.g. related to syncrepl, for whom
		 * abandon does not get set... */
		if ( slapd_shutdown ) {
			rs->sr_err = LDAP_UNAVAILABLE;
			send_ldap_disconnect( op, rs );
			goto done;
		}

		/* check time limit */
		if ( op->ors_tlimit != SLAP_NO_LIMIT
				&& slap_get_time() > stoptime )
		{
			rs->sr_err = LDAP_TIMELIMIT_EXCEEDED;
			rs->sr_ref = rs->sr_v2ref;
			send_ldap_result( op, rs );
			rs->sr_err = LDAP_SUCCESS;
			goto done;
		}


		if ( nsubs < ncand ) {
			unsigned i;
			/* Is this entry in the candidate list? */
			scopeok = 0;
			if (MDB_IDL_IS_RANGE( candidates )) {
				if ( id >= MDB_IDL_RANGE_FIRST( candidates ) &&
					id <= MDB_IDL_RANGE_LAST( candidates ))
					scopeok = 1;
			} else {
				i = mdb_idl_search( candidates, id );
				if ( candidates[i] == id )
					scopeok = 1;
			}
			if ( scopeok )
				goto scopeok;
			goto loop_continue;
		}

		/* Does this candidate actually satisfy the search scope?
		 */
		scopeok = 0;
		isc.numrdns = 0;
		switch( op->ors_scope ) {
		case LDAP_SCOPE_BASE:
			/* This is always true, yes? */
			if ( id == base->e_id ) scopeok = 1;
			break;

#ifdef LDAP_SCOPE_CHILDREN
		case LDAP_SCOPE_CHILDREN:
			if ( id == base->e_id ) break;
			/* Fall-thru */
#endif
		case LDAP_SCOPE_SUBTREE:
			if ( id == base->e_id ) {
				scopeok = 1;
				break;
			}
			/* Fall-thru */
		case LDAP_SCOPE_ONELEVEL:
			isc.id = id;
			isc.nscope = 0;
			rs->sr_err = mdb_idscopes( op, &isc );
			if ( rs->sr_err == MDB_SUCCESS ) {
				if ( isc.nscope )
					scopeok = 1;
			} else {
				if ( rs->sr_err == MDB_NOTFOUND )
					goto notfound;
			}
			break;
		}

		/* Not in scope, ignore it */
		if ( !scopeok )
		{
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(mdb_search)
				": %ld scope not okay\n",
				(long) id, 0, 0 );
			goto loop_continue;
		}

scopeok:
		if ( id == base->e_id ) {
			e = base;
		} else {

			/* get the entry */
			rs->sr_err = mdb_id2edata( op, mci, id, &edata );
			if ( rs->sr_err == MDB_NOTFOUND ) {
notfound:
				if( nsubs < ncand )
					goto loop_continue;

				if( !MDB_IDL_IS_RANGE(candidates) ) {
					/* only complain for non-range IDLs */
					Debug( LDAP_DEBUG_TRACE,
						LDAP_XSTRING(mdb_search)
						": candidate %ld not found\n",
						(long) id, 0, 0 );
				} else {
					/* get the next ID from the DB */
					rs->sr_err = mdb_get_nextid( mci, &cursor );
					if ( rs->sr_err == MDB_NOTFOUND ) {
						break;
					}
					if ( rs->sr_err ) {
						rs->sr_err = LDAP_OTHER;
						rs->sr_text = "internal error in get_nextid";
						send_ldap_result( op, rs );
						goto done;
					}
					cursor--;
				}

				goto loop_continue;
			} else if ( rs->sr_err ) {
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "internal error in mdb_id2edata";
				send_ldap_result( op, rs );
				goto done;
			}

			rs->sr_err = mdb_entry_decode( op, ltid, &edata, &e );
			if ( rs->sr_err ) {
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = "internal error in mdb_entry_decode";
				send_ldap_result( op, rs );
				goto done;
			}
			e->e_id = id;
			e->e_name.bv_val = NULL;
			e->e_nname.bv_val = NULL;
		}

		if ( is_entry_subentry( e ) ) {
			if( op->oq_search.rs_scope != LDAP_SCOPE_BASE ) {
				if(!get_subentries_visibility( op )) {
					/* only subentries are visible */
					goto loop_continue;
				}

			} else if ( get_subentries( op ) &&
				!get_subentries_visibility( op ))
			{
				/* only subentries are visible */
				goto loop_continue;
			}

		} else if ( get_subentries_visibility( op )) {
			/* only subentries are visible */
			goto loop_continue;
		}

		/* aliases were already dereferenced in candidate list */
		if ( op->ors_deref & LDAP_DEREF_SEARCHING ) {
			/* but if the search base is an alias, and we didn't
			 * deref it when finding, return it.
			 */
			if ( is_entry_alias(e) &&
				((op->ors_deref & LDAP_DEREF_FINDING) || e != base ))
			{
				goto loop_continue;
			}
		}

		if ( !manageDSAit && is_entry_glue( e )) {
			goto loop_continue;
		}

		if (e != base) {
			struct berval pdn, pndn;
			char *d, *n;
			int i;

			/* child of base, just append RDNs to base->e_name */
			if ( nsubs < ncand || isc.scopes[isc.nscope].mid == base->e_id ) {
				pdn = base->e_name;
				pndn = base->e_nname;
			} else {
				mdb_id2name( op, ltid, &isc.mc, scopes[isc.nscope].mid, &pdn, &pndn );
			}
			e->e_name.bv_len = pdn.bv_len;
			e->e_nname.bv_len = pndn.bv_len;
			for (i=0; i<isc.numrdns; i++) {
				e->e_name.bv_len += isc.rdns[i].bv_len + 1;
				e->e_nname.bv_len += isc.nrdns[i].bv_len + 1;
			}
			e->e_name.bv_val = op->o_tmpalloc(e->e_name.bv_len + 1, op->o_tmpmemctx);
			e->e_nname.bv_val = op->o_tmpalloc(e->e_nname.bv_len + 1, op->o_tmpmemctx);
			d = e->e_name.bv_val;
			n = e->e_nname.bv_val;
			if (nsubs < ncand) {
				/* RDNs are in top-down order */
				for (i=isc.numrdns-1; i>=0; i--) {
					memcpy(d, isc.rdns[i].bv_val, isc.rdns[i].bv_len);
					d += isc.rdns[i].bv_len;
					*d++ = ',';
					memcpy(n, isc.nrdns[i].bv_val, isc.nrdns[i].bv_len);
					n += isc.nrdns[i].bv_len;
					*n++ = ',';
				}
			} else {
				/* RDNs are in bottom-up order */
				for (i=0; i<isc.numrdns; i++) {
					memcpy(d, isc.rdns[i].bv_val, isc.rdns[i].bv_len);
					d += isc.rdns[i].bv_len;
					*d++ = ',';
					memcpy(n, isc.nrdns[i].bv_val, isc.nrdns[i].bv_len);
					n += isc.nrdns[i].bv_len;
					*n++ = ',';
				}
			}

			if (pdn.bv_len) {
				memcpy(d, pdn.bv_val, pdn.bv_len+1);
				memcpy(n, pndn.bv_val, pndn.bv_len+1);
			} else {
				*--d = '\0';
				*--n = '\0';
				e->e_name.bv_len--;
				e->e_nname.bv_len--;
			}
			if (pndn.bv_val != base->e_nname.bv_val) {
				op->o_tmpfree(pndn.bv_val, op->o_tmpmemctx);
				op->o_tmpfree(pdn.bv_val, op->o_tmpmemctx);
			}
		}

		/*
		 * if it's a referral, add it to the list of referrals. only do
		 * this for non-base searches, and don't check the filter
		 * explicitly here since it's only a candidate anyway.
		 */
		if ( !manageDSAit && op->oq_search.rs_scope != LDAP_SCOPE_BASE
			&& is_entry_referral( e ) )
		{
			BerVarray erefs = get_entry_referrals( op, e );
			rs->sr_ref = referral_rewrite( erefs, &e->e_name, NULL,
				op->oq_search.rs_scope == LDAP_SCOPE_ONELEVEL
					? LDAP_SCOPE_BASE : LDAP_SCOPE_SUBTREE );

			rs->sr_entry = e;
			rs->sr_flags = 0;

			send_search_reference( op, rs );

			mdb_entry_return( op, e );
			rs->sr_entry = NULL;
			e = NULL;

			ber_bvarray_free( rs->sr_ref );
			ber_bvarray_free( erefs );
			rs->sr_ref = NULL;

			goto loop_continue;
		}

		/* if it matches the filter and scope, send it */
		rs->sr_err = test_filter( op, e, op->oq_search.rs_filter );

		if ( rs->sr_err == LDAP_COMPARE_TRUE ) {
			/* check size limit */
			if ( get_pagedresults(op) > SLAP_CONTROL_IGNORED ) {
				if ( rs->sr_nentries >= ((PagedResultsState *)op->o_pagedresults_state)->ps_size ) {
					mdb_entry_return( op, e );
					e = NULL;
					send_paged_response( op, rs, &lastid, tentries );
					goto done;
				}
				lastid = id;
			}

			if (e) {
				/* safe default */
				rs->sr_attrs = op->oq_search.rs_attrs;
				rs->sr_operational_attrs = NULL;
				rs->sr_ctrls = NULL;
				rs->sr_entry = e;
				RS_ASSERT( e->e_private != NULL );
				rs->sr_flags = 0;
				rs->sr_err = LDAP_SUCCESS;
				rs->sr_err = send_search_entry( op, rs );
				rs->sr_attrs = NULL;
				rs->sr_entry = NULL;
				if (e != base)
					mdb_entry_return( op, e );
				e = NULL;

				switch ( rs->sr_err ) {
				case LDAP_SUCCESS:	/* entry sent ok */
					break;
				default:		/* entry not sent */
					break;
				case LDAP_BUSY:
					send_ldap_result( op, rs );
					goto done;
				case LDAP_UNAVAILABLE:
				case LDAP_SIZELIMIT_EXCEEDED:
					if ( rs->sr_err == LDAP_SIZELIMIT_EXCEEDED ) {
						rs->sr_ref = rs->sr_v2ref;
						send_ldap_result( op, rs );
						rs->sr_err = LDAP_SUCCESS;

					} else {
						rs->sr_err = LDAP_OTHER;
					}
					goto done;
				}
			}

		} else {
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(mdb_search)
				": %ld does not match filter\n",
				(long) id, 0, 0 );
		}

loop_continue:
		if( e != NULL ) {
			if ( e != base )
				mdb_entry_return( op, e );
			RS_ASSERT( rs->sr_entry == NULL );
			e = NULL;
			rs->sr_entry = NULL;
		}

		if ( nsubs < ncand ) {
			int rc = mdb_dn2id_walk( op, &isc );
			if (rc) {
				id = NOID;
				/* We got to the end of a subtree. If there are any
				 * alias scopes left, search them too.
				 */
				while (iscopes[0] && cscope < iscopes[0]) {
					cscope++;
					isc.id = iscopes[cscope];
					if ( base )
						mdb_entry_return( op, base );
					rs->sr_err = mdb_id2entry(op, mci, isc.id, &base);
					if ( !rs->sr_err ) {
						mdb_id2name( op, ltid, &isc.mc, isc.id, &base->e_name, &base->e_nname );
						isc.numrdns = 0;
						if (isc.oscope == LDAP_SCOPE_ONELEVEL)
							isc.oscope = LDAP_SCOPE_BASE;
						rc = mdb_dn2id_walk( op, &isc );
						if ( !rc ) {
							id = isc.id;
							break;
						}
					}
				}
			} else
				id = isc.id;
		} else {
			id = mdb_idl_next( candidates, &cursor );
		}
	}

nochange:
	rs->sr_ctrls = NULL;
	rs->sr_ref = rs->sr_v2ref;
	rs->sr_err = (rs->sr_v2ref == NULL) ? LDAP_SUCCESS : LDAP_REFERRAL;
	rs->sr_rspoid = NULL;
	if ( get_pagedresults(op) > SLAP_CONTROL_IGNORED ) {
		send_paged_response( op, rs, NULL, 0 );
	} else {
		send_ldap_result( op, rs );
	}

	rs->sr_err = LDAP_SUCCESS;

done:
	mdb_cursor_close( mcd );
	mdb_cursor_close( mci );
	if ( moi == &opinfo ) {
		mdb_txn_reset( moi->moi_txn );
		LDAP_SLIST_REMOVE( &op->o_extra, &moi->moi_oe, OpExtra, oe_next );
	} else {
		moi->moi_ref--;
	}
	if( rs->sr_v2ref ) {
		ber_bvarray_free( rs->sr_v2ref );
		rs->sr_v2ref = NULL;
	}
	if (base)
		mdb_entry_return( op,base);
	scope_chunk_ret( op, scopes );

	return rs->sr_err;
}


static int base_candidate(
	BackendDB	*be,
	Entry	*e,
	ID		*ids )
{
	Debug(LDAP_DEBUG_ARGS, "base_candidates: base: \"%s\" (0x%08lx)\n",
		e->e_nname.bv_val, (long) e->e_id, 0);

	ids[0] = 1;
	ids[1] = e->e_id;
	return 0;
}

/* Look for "objectClass Present" in this filter.
 * Also count depth of filter tree while we're at it.
 */
static int oc_filter(
	Filter *f,
	int cur,
	int *max )
{
	int rc = 0;

	assert( f != NULL );

	if( cur > *max ) *max = cur;

	switch( f->f_choice ) {
	case LDAP_FILTER_PRESENT:
		if (f->f_desc == slap_schema.si_ad_objectClass) {
			rc = 1;
		}
		break;

	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
		cur++;
		for ( f=f->f_and; f; f=f->f_next ) {
			(void) oc_filter(f, cur, max);
		}
		break;

	default:
		break;
	}
	return rc;
}

static void search_stack_free( void *key, void *data )
{
	ber_memfree_x(data, NULL);
}

static void *search_stack( Operation *op )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	void *ret = NULL;

	if ( op->o_threadctx ) {
		ldap_pvt_thread_pool_getkey( op->o_threadctx, (void *)search_stack,
			&ret, NULL );
	} else {
		ret = mdb->mi_search_stack;
	}

	if ( !ret ) {
		ret = ch_malloc( mdb->mi_search_stack_depth * MDB_IDL_UM_SIZE
			* sizeof( ID ) );
		if ( op->o_threadctx ) {
			ldap_pvt_thread_pool_setkey( op->o_threadctx, (void *)search_stack,
				ret, search_stack_free, NULL, NULL );
		} else {
			mdb->mi_search_stack = ret;
		}
	}
	return ret;
}

static int search_candidates(
	Operation *op,
	SlapReply *rs,
	Entry *e,
	MDB_txn *txn,
	MDB_cursor *mci,
	ID	*ids,
	ID2L	scopes )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	int rc, depth = 1;
	Filter		*f, rf, xf, nf, sf;
	ID		*stack;
	AttributeAssertion aa_ref = ATTRIBUTEASSERTION_INIT;
	AttributeAssertion aa_subentry = ATTRIBUTEASSERTION_INIT;

	/*
	 * This routine takes as input a filter (user-filter)
	 * and rewrites it as follows:
	 *	(&(scope=DN)[(objectClass=subentry)]
	 *		(|[(objectClass=referral)](user-filter))
	 */

	Debug(LDAP_DEBUG_TRACE,
		"search_candidates: base=\"%s\" (0x%08lx) scope=%d\n",
		e->e_nname.bv_val, (long) e->e_id, op->oq_search.rs_scope );

	f = op->oq_search.rs_filter;

	/* If the user's filter uses objectClass=*,
	 * these clauses are redundant.
	 */
	if (!oc_filter(op->oq_search.rs_filter, 1, &depth)
		&& !get_subentries_visibility(op)) {
		if( !get_manageDSAit(op) && !get_domainScope(op) ) {
			/* match referral objects */
			struct berval bv_ref = BER_BVC( "referral" );
			rf.f_choice = LDAP_FILTER_EQUALITY;
			rf.f_ava = &aa_ref;
			rf.f_av_desc = slap_schema.si_ad_objectClass;
			rf.f_av_value = bv_ref;
			rf.f_next = f;
			xf.f_or = &rf;
			xf.f_choice = LDAP_FILTER_OR;
			xf.f_next = NULL;
			f = &xf;
			depth++;
		}
	}

	if( get_subentries_visibility( op ) ) {
		struct berval bv_subentry = BER_BVC( "subentry" );
		sf.f_choice = LDAP_FILTER_EQUALITY;
		sf.f_ava = &aa_subentry;
		sf.f_av_desc = slap_schema.si_ad_objectClass;
		sf.f_av_value = bv_subentry;
		sf.f_next = f;
		nf.f_choice = LDAP_FILTER_AND;
		nf.f_and = &sf;
		nf.f_next = NULL;
		f = &nf;
		depth++;
	}

	/* Allocate IDL stack, plus 1 more for former tmp */
	if ( depth+1 > mdb->mi_search_stack_depth ) {
		stack = ch_malloc( (depth + 1) * MDB_IDL_UM_SIZE * sizeof( ID ) );
	} else {
		stack = search_stack( op );
	}

	if( op->ors_deref & LDAP_DEREF_SEARCHING ) {
		rc = search_aliases( op, rs, e->e_id, txn, mci, scopes, stack );
	} else {
		rc = LDAP_SUCCESS;
	}

	if ( rc == LDAP_SUCCESS ) {
		rc = mdb_filter_candidates( op, txn, f, ids,
			stack, stack+MDB_IDL_UM_SIZE );
	}

	if ( depth+1 > mdb->mi_search_stack_depth ) {
		ch_free( stack );
	}

	if( rc ) {
		Debug(LDAP_DEBUG_TRACE,
			"mdb_search_candidates: failed (rc=%d)\n",
			rc, NULL, NULL );

	} else {
		Debug(LDAP_DEBUG_TRACE,
			"mdb_search_candidates: id=%ld first=%ld last=%ld\n",
			(long) ids[0],
			(long) MDB_IDL_FIRST(ids),
			(long) MDB_IDL_LAST(ids) );
	}

	return rc;
}

static int
parse_paged_cookie( Operation *op, SlapReply *rs )
{
	int		rc = LDAP_SUCCESS;
	PagedResultsState *ps = op->o_pagedresults_state;

	/* this function must be invoked only if the pagedResults
	 * control has been detected, parsed and partially checked
	 * by the frontend */
	assert( get_pagedresults( op ) > SLAP_CONTROL_IGNORED );

	/* cookie decoding/checks deferred to backend... */
	if ( ps->ps_cookieval.bv_len ) {
		PagedResultsCookie reqcookie;
		if( ps->ps_cookieval.bv_len != sizeof( reqcookie ) ) {
			/* bad cookie */
			rs->sr_text = "paged results cookie is invalid";
			rc = LDAP_PROTOCOL_ERROR;
			goto done;
		}

		AC_MEMCPY( &reqcookie, ps->ps_cookieval.bv_val, sizeof( reqcookie ));

		if ( reqcookie > ps->ps_cookie ) {
			/* bad cookie */
			rs->sr_text = "paged results cookie is invalid";
			rc = LDAP_PROTOCOL_ERROR;
			goto done;

		} else if ( reqcookie < ps->ps_cookie ) {
			rs->sr_text = "paged results cookie is invalid or old";
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto done;
		}

	} else {
		/* we're going to use ps_cookie */
		op->o_conn->c_pagedresults_state.ps_cookie = 0;
	}

done:;

	return rc;
}

static void
send_paged_response( 
	Operation	*op,
	SlapReply	*rs,
	ID		*lastid,
	int		tentries )
{
	LDAPControl	*ctrls[2];
	BerElementBuffer berbuf;
	BerElement	*ber = (BerElement *)&berbuf;
	PagedResultsCookie respcookie;
	struct berval cookie;

	Debug(LDAP_DEBUG_ARGS,
		"send_paged_response: lastid=0x%08lx nentries=%d\n", 
		lastid ? *lastid : 0, rs->sr_nentries, NULL );

	ctrls[1] = NULL;

	ber_init2( ber, NULL, LBER_USE_DER );

	if ( lastid ) {
		respcookie = ( PagedResultsCookie )(*lastid);
		cookie.bv_len = sizeof( respcookie );
		cookie.bv_val = (char *)&respcookie;

	} else {
		respcookie = ( PagedResultsCookie )0;
		BER_BVSTR( &cookie, "" );
	}

	op->o_conn->c_pagedresults_state.ps_cookie = respcookie;
	op->o_conn->c_pagedresults_state.ps_count =
		((PagedResultsState *)op->o_pagedresults_state)->ps_count +
		rs->sr_nentries;

	/* return size of 0 -- no estimate */
	ber_printf( ber, "{iO}", 0, &cookie ); 

	ctrls[0] = op->o_tmpalloc( sizeof(LDAPControl), op->o_tmpmemctx );
	if ( ber_flatten2( ber, &ctrls[0]->ldctl_value, 0 ) == -1 ) {
		goto done;
	}

	ctrls[0]->ldctl_oid = LDAP_CONTROL_PAGEDRESULTS;
	ctrls[0]->ldctl_iscritical = 0;

	slap_add_ctrls( op, rs, ctrls );
	rs->sr_err = LDAP_SUCCESS;
	send_ldap_result( op, rs );

done:
	(void) ber_free_buf( ber );
}
