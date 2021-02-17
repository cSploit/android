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

#include "back-bdb.h"
#include "idl.h"

static int base_candidate(
	BackendDB	*be,
	Entry	*e,
	ID		*ids );

static int search_candidates(
	Operation *op,
	SlapReply *rs,
	Entry *e,
	DB_TXN *txn,
	ID	*ids,
	ID	*scopes );

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
	DB_TXN *txn,
	DB_LOCK *lock,
	ID	*tmp,
	ID	*visited )
{
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	struct berval ndn;
	EntryInfo *ei;
	DB_LOCK lockr;

	rs->sr_err = LDAP_ALIAS_DEREF_PROBLEM;
	rs->sr_text = "maximum deref depth exceeded";

	for (;;) {
		/* Remember the last entry we looked at, so we can
		 * report broken links
		 */
		*matched = e;

		if (BDB_IDL_N(tmp) >= op->o_bd->be_max_deref_depth) {
			e = NULL;
			break;
		}

		/* If this is part of a subtree or onelevel search,
		 * have we seen this ID before? If so, quit.
		 */
		if ( visited && bdb_idl_insert( visited, e->e_id ) ) {
			e = NULL;
			break;
		}

		/* If we've seen this ID during this deref iteration,
		 * we've hit a loop.
		 */
		if ( bdb_idl_insert( tmp, e->e_id ) ) {
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

		rs->sr_err = bdb_dn2entry( op, txn, &ndn, &ei,
			0, &lockr );
		if ( rs->sr_err == DB_LOCK_DEADLOCK )
			return NULL;

		if ( ei ) {
			e = ei->bei_e;
		} else {
			e = NULL;
		}

		if (!e) {
			rs->sr_err = LDAP_ALIAS_PROBLEM;
			rs->sr_text = "aliasedObject not found";
			break;
		}

		/* Free the previous entry, continue to work with the
		 * one we just retrieved.
		 */
		bdb_cache_return_entry_r( bdb, *matched, lock);
		*lock = lockr;

		/* We found a regular entry. Return this to the caller. The
		 * entry is still locked for Read.
		 */
		if (!is_entry_alias(e)) {
			rs->sr_err = LDAP_SUCCESS;
			rs->sr_text = NULL;
			break;
		}
	}
	return e;
}

/* Look for and dereference all aliases within the search scope. Adds
 * the dereferenced entries to the "ids" list. Requires "stack" to be
 * able to hold 8 levels of DB_SIZE IDLs. Of course we're hardcoded to
 * require a minimum of 8 UM_SIZE IDLs so this is never a problem.
 */
static int search_aliases(
	Operation *op,
	SlapReply *rs,
	Entry *e,
	DB_TXN *txn,
	ID *ids,
	ID *scopes,
	ID *stack )
{
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	ID *aliases, *curscop, *subscop, *visited, *newsubs, *oldsubs, *tmp;
	ID cursora, ida, cursoro, ido, *subscop2;
	Entry *matched, *a;
	EntryInfo *ei;
	struct berval bv_alias = BER_BVC( "alias" );
	AttributeAssertion aa_alias = ATTRIBUTEASSERTION_INIT;
	Filter	af;
	DB_LOCK locka, lockr;
	int first = 1;

	aliases = stack;	/* IDL of all aliases in the database */
	curscop = aliases + BDB_IDL_DB_SIZE;	/* Aliases in the current scope */
	subscop = curscop + BDB_IDL_DB_SIZE;	/* The current scope */
	visited = subscop + BDB_IDL_DB_SIZE;	/* IDs we've seen in this search */
	newsubs = visited + BDB_IDL_DB_SIZE;	/* New subtrees we've added */
	oldsubs = newsubs + BDB_IDL_DB_SIZE;	/* Subtrees added previously */
	tmp = oldsubs + BDB_IDL_DB_SIZE;	/* Scratch space for deref_base() */

	/* A copy of subscop, because subscop gets clobbered by
	 * the bdb_idl_union/intersection routines
	 */
	subscop2 = tmp + BDB_IDL_DB_SIZE;

	af.f_choice = LDAP_FILTER_EQUALITY;
	af.f_ava = &aa_alias;
	af.f_av_desc = slap_schema.si_ad_objectClass;
	af.f_av_value = bv_alias;
	af.f_next = NULL;

	/* Find all aliases in database */
	BDB_IDL_ZERO( aliases );
	rs->sr_err = bdb_filter_candidates( op, txn, &af, aliases,
		curscop, visited );
	if (rs->sr_err != LDAP_SUCCESS || BDB_IDL_IS_ZERO( aliases )) {
		return rs->sr_err;
	}
	oldsubs[0] = 1;
	oldsubs[1] = e->e_id;

	BDB_IDL_ZERO( ids );
	BDB_IDL_ZERO( visited );
	BDB_IDL_ZERO( newsubs );

	cursoro = 0;
	ido = bdb_idl_first( oldsubs, &cursoro );

	for (;;) {
		/* Set curscop to only the aliases in the current scope. Start with
		 * all the aliases, obtain the IDL for the current scope, and then
		 * get the intersection of these two IDLs. Add the current scope
		 * to the cumulative list of candidates.
		 */
		BDB_IDL_CPY( curscop, aliases );
		rs->sr_err = bdb_dn2idl( op, txn, &e->e_nname, BEI(e), subscop,
			subscop2+BDB_IDL_DB_SIZE );

		if (first) {
			first = 0;
		} else {
			bdb_cache_return_entry_r (bdb, e, &locka);
		}
		if ( rs->sr_err == DB_LOCK_DEADLOCK )
			return rs->sr_err;

		BDB_IDL_CPY(subscop2, subscop);
		rs->sr_err = bdb_idl_intersection(curscop, subscop);
		bdb_idl_union( ids, subscop2 );

		/* Dereference all of the aliases in the current scope. */
		cursora = 0;
		for (ida = bdb_idl_first(curscop, &cursora); ida != NOID;
			ida = bdb_idl_next(curscop, &cursora))
		{
			ei = NULL;
retry1:
			rs->sr_err = bdb_cache_find_id(op, txn,
				ida, &ei, 0, &lockr );
			if (rs->sr_err != LDAP_SUCCESS) {
				if ( rs->sr_err == DB_LOCK_DEADLOCK )
					return rs->sr_err;
				if ( rs->sr_err == DB_LOCK_NOTGRANTED )
					goto retry1;
				continue;
			}
			a = ei->bei_e;

			/* This should only happen if the curscop IDL has maxed out and
			 * turned into a range that spans IDs indiscriminately
			 */
			if (!is_entry_alias(a)) {
				bdb_cache_return_entry_r (bdb, a, &lockr);
				continue;
			}

			/* Actually dereference the alias */
			BDB_IDL_ZERO(tmp);
			a = deref_base( op, rs, a, &matched, txn, &lockr,
				tmp, visited );
			if (a) {
				/* If the target was not already in our current candidates,
				 * make note of it in the newsubs list. Also
				 * set it in the scopes list so that bdb_search
				 * can check it.
				 */
				if (bdb_idl_insert(ids, a->e_id) == 0) {
					bdb_idl_insert(newsubs, a->e_id);
					bdb_idl_insert(scopes, a->e_id);
				}
				bdb_cache_return_entry_r( bdb, a, &lockr);

			} else if ( rs->sr_err == DB_LOCK_DEADLOCK ) {
				return rs->sr_err;
			} else if (matched) {
				/* Alias could not be dereferenced, or it deref'd to
				 * an ID we've already seen. Ignore it.
				 */
				bdb_cache_return_entry_r( bdb, matched, &lockr );
				rs->sr_text = NULL;
			}
		}
		/* If this is a OneLevel search, we're done; oldsubs only had one
		 * ID in it. For a Subtree search, oldsubs may be a list of scope IDs.
		 */
		if ( op->ors_scope == LDAP_SCOPE_ONELEVEL ) break;
nextido:
		ido = bdb_idl_next( oldsubs, &cursoro );
		
		/* If we're done processing the old scopes, did we add any new
		 * scopes in this iteration? If so, go back and do those now.
		 */
		if (ido == NOID) {
			if (BDB_IDL_IS_ZERO(newsubs)) break;
			BDB_IDL_CPY(oldsubs, newsubs);
			BDB_IDL_ZERO(newsubs);
			cursoro = 0;
			ido = bdb_idl_first( oldsubs, &cursoro );
		}

		/* Find the entry corresponding to the next scope. If it can't
		 * be found, ignore it and move on. This should never happen;
		 * we should never see the ID of an entry that doesn't exist.
		 * Set the name so that the scope's IDL can be retrieved.
		 */
		ei = NULL;
sameido:
		rs->sr_err = bdb_cache_find_id(op, txn, ido, &ei,
			0, &locka );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			if ( rs->sr_err == DB_LOCK_DEADLOCK )
				return rs->sr_err;
			if ( rs->sr_err == DB_LOCK_NOTGRANTED )
				goto sameido;
			goto nextido;
		}
		e = ei->bei_e;
	}
	return rs->sr_err;
}

/* Get the next ID from the DB. Used if the candidate list is
 * a range and simple iteration hits missing entryIDs
 */
static int
bdb_get_nextid(struct bdb_info *bdb, DB_TXN *ltid, ID *cursor)
{
	DBC *curs;
	DBT key, data;
	ID id, nid;
	int rc;

	id = *cursor + 1;
	BDB_ID2DISK( id, &nid );
	rc = bdb->bi_id2entry->bdi_db->cursor(
		bdb->bi_id2entry->bdi_db, ltid, &curs, bdb->bi_db_opflags );
	if ( rc )
		return rc;
	key.data = &nid;
	key.size = key.ulen = sizeof(ID);
	key.flags = DB_DBT_USERMEM;
	data.flags = DB_DBT_USERMEM | DB_DBT_PARTIAL;
	data.dlen = data.ulen = 0;
	rc = curs->c_get( curs, &key, &data, DB_SET_RANGE );
	curs->c_close( curs );
	if ( rc )
		return rc;
	BDB_DISK2ID( &nid, cursor );
	return 0;
}

int
bdb_search( Operation *op, SlapReply *rs )
{
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	ID		id, cursor;
	ID		lastid = NOID;
	ID		candidates[BDB_IDL_UM_SIZE];
	ID		scopes[BDB_IDL_DB_SIZE];
	Entry		*e = NULL, base, *e_root;
	Entry		*matched = NULL;
	EntryInfo	*ei;
	AttributeName	*attrs;
	struct berval	realbase = BER_BVNULL;
	slap_mask_t	mask;
	time_t		stoptime;
	int		manageDSAit;
	int		tentries = 0;
	unsigned	nentries = 0;
	int		idflag = 0;

	DB_LOCK		lock;
	struct	bdb_op_info	*opinfo = NULL;
	DB_TXN			*ltid = NULL;
	OpExtra *oex;

	Debug( LDAP_DEBUG_TRACE, "=> " LDAP_XSTRING(bdb_search) "\n", 0, 0, 0);
	attrs = op->oq_search.rs_attrs;

	LDAP_SLIST_FOREACH( oex, &op->o_extra, oe_next ) {
		if ( oex->oe_key == bdb )
			break;
	}
	opinfo = (struct bdb_op_info *) oex;

	manageDSAit = get_manageDSAit( op );

	if ( opinfo && opinfo->boi_txn ) {
		ltid = opinfo->boi_txn;
	} else {
		rs->sr_err = bdb_reader_get( op, bdb->bi_dbenv, &ltid );

		switch(rs->sr_err) {
		case 0:
			break;
		default:
			send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
			return rs->sr_err;
		}
	}

	e_root = bdb->bi_cache.c_dntree.bei_e;
	if ( op->o_req_ndn.bv_len == 0 ) {
		/* DIT root special case */
		ei = e_root->e_private;
		rs->sr_err = LDAP_SUCCESS;
	} else {
		if ( op->ors_deref & LDAP_DEREF_FINDING ) {
			BDB_IDL_ZERO(candidates);
		}
dn2entry_retry:
		/* get entry with reader lock */
		rs->sr_err = bdb_dn2entry( op, ltid, &op->o_req_ndn, &ei,
			1, &lock );
	}

	switch(rs->sr_err) {
	case DB_NOTFOUND:
		matched = ei->bei_e;
		break;
	case 0:
		e = ei->bei_e;
		break;
	case DB_LOCK_DEADLOCK:
		if ( !opinfo ) {
			ltid->flags &= ~TXN_DEADLOCK;
			goto dn2entry_retry;
		}
		opinfo->boi_err = rs->sr_err;
		/* FALLTHRU */
	case LDAP_BUSY:
		send_ldap_error( op, rs, LDAP_BUSY, "ldap server busy" );
		return LDAP_BUSY;
	case DB_LOCK_NOTGRANTED:
		goto dn2entry_retry;
	default:
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		return rs->sr_err;
	}

	if ( op->ors_deref & LDAP_DEREF_FINDING ) {
		if ( matched && is_entry_alias( matched )) {
			struct berval stub;

			stub.bv_val = op->o_req_ndn.bv_val;
			stub.bv_len = op->o_req_ndn.bv_len - matched->e_nname.bv_len - 1;
			e = deref_base( op, rs, matched, &matched, ltid, &lock,
				candidates, NULL );
			if ( e ) {
				build_new_dn( &op->o_req_ndn, &e->e_nname, &stub,
					op->o_tmpmemctx );
				bdb_cache_return_entry_r (bdb, e, &lock);
				matched = NULL;
				goto dn2entry_retry;
			}
		} else if ( e && is_entry_alias( e )) {
			e = deref_base( op, rs, e, &matched, ltid, &lock,
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
				if ( rs->sr_err == DB_NOTFOUND )
					rs->sr_err = LDAP_REFERRAL;
				rs->sr_matched = matched_dn.bv_val;
			}

#ifdef SLAP_ZONE_ALLOC
			slap_zn_runlock(bdb->bi_cache.c_zctx, matched);
#endif
			bdb_cache_return_entry_r (bdb, matched, &lock);
			matched = NULL;

			if ( erefs ) {
				rs->sr_ref = referral_rewrite( erefs, &matched_dn,
					&op->o_req_dn, op->oq_search.rs_scope );
				ber_bvarray_free( erefs );
			}

		} else {
#ifdef SLAP_ZONE_ALLOC
			slap_zn_runlock(bdb->bi_cache.c_zctx, matched);
#endif
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
		return rs->sr_err;
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

#ifdef SLAP_ZONE_ALLOC
		slap_zn_runlock(bdb->bi_cache.c_zctx, e);
#endif
		if ( e != e_root ) {
			bdb_cache_return_entry_r(bdb, e, &lock);
		}
		send_ldap_result( op, rs );
		return rs->sr_err;
	}

	if ( !manageDSAit && e != e_root && is_entry_referral( e ) ) {
		/* entry is a referral, don't allow add */
		struct berval matched_dn = BER_BVNULL;
		BerVarray erefs = NULL;
		
		ber_dupbv( &matched_dn, &e->e_name );
		erefs = get_entry_referrals( op, e );

		rs->sr_err = LDAP_REFERRAL;

#ifdef SLAP_ZONE_ALLOC
		slap_zn_runlock(bdb->bi_cache.c_zctx, e);
#endif
		bdb_cache_return_entry_r( bdb, e, &lock );
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
			LDAP_XSTRING(bdb_search) ": entry is referral\n",
			0, 0, 0 );

		rs->sr_matched = matched_dn.bv_val;
		send_ldap_result( op, rs );

		ber_bvarray_free( rs->sr_ref );
		rs->sr_ref = NULL;
		ber_memfree( matched_dn.bv_val );
		rs->sr_matched = NULL;
		return 1;
	}

	if ( get_assert( op ) &&
		( test_filter( op, e, get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rs->sr_err = LDAP_ASSERTION_FAILED;
#ifdef SLAP_ZONE_ALLOC
		slap_zn_runlock(bdb->bi_cache.c_zctx, e);
#endif
		if ( e != e_root ) {
			bdb_cache_return_entry_r(bdb, e, &lock);
		}
		send_ldap_result( op, rs );
		return 1;
	}

	/* compute it anyway; root does not use it */
	stoptime = op->o_time + op->ors_tlimit;

	/* need normalized dn below */
	ber_dupbv( &realbase, &e->e_nname );

	/* Copy info to base, must free entry before accessing the database
	 * in search_candidates, to avoid deadlocks.
	 */
	base.e_private = e->e_private;
	base.e_nname = realbase;
	base.e_id = e->e_id;

#ifdef SLAP_ZONE_ALLOC
	slap_zn_runlock(bdb->bi_cache.c_zctx, e);
#endif
	if ( e != e_root ) {
		bdb_cache_return_entry_r(bdb, e, &lock);
	}
	e = NULL;

	/* select candidates */
	if ( op->oq_search.rs_scope == LDAP_SCOPE_BASE ) {
		rs->sr_err = base_candidate( op->o_bd, &base, candidates );

	} else {
cand_retry:
		BDB_IDL_ZERO( candidates );
		BDB_IDL_ZERO( scopes );
		rs->sr_err = search_candidates( op, rs, &base,
			ltid, candidates, scopes );
		if ( rs->sr_err == DB_LOCK_DEADLOCK ) {
			if ( !opinfo ) {
				ltid->flags &= ~TXN_DEADLOCK;
				goto cand_retry;
			}
			opinfo->boi_err = rs->sr_err;
			send_ldap_error( op, rs, LDAP_BUSY, "ldap server busy" );
			return LDAP_BUSY;
		}
	}

	/* start cursor at beginning of candidates.
	 */
	cursor = 0;

	if ( candidates[0] == 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(bdb_search) ": no candidates\n",
			0, 0, 0 );

		goto nochange;
	}

	/* if not root and candidates exceed to-be-checked entries, abort */
	if ( op->ors_limit	/* isroot == FALSE */ &&
		op->ors_limit->lms_s_unchecked != -1 &&
		BDB_IDL_N(candidates) > (unsigned) op->ors_limit->lms_s_unchecked )
	{
		rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
		send_ldap_result( op, rs );
		rs->sr_err = LDAP_SUCCESS;
		goto done;
	}

	if ( op->ors_limit == NULL	/* isroot == TRUE */ ||
		!op->ors_limit->lms_s_pr_hide )
	{
		tentries = BDB_IDL_N(candidates);
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
		id = bdb_idl_first( candidates, &cursor );
		if ( id == NOID ) {
			Debug( LDAP_DEBUG_TRACE, 
				LDAP_XSTRING(bdb_search)
				": no paged results candidates\n",
				0, 0, 0 );
			send_paged_response( op, rs, &lastid, 0 );

			rs->sr_err = LDAP_OTHER;
			goto done;
		}
		nentries = ps->ps_count;
		if ( id == (ID)ps->ps_cookie )
			id = bdb_idl_next( candidates, &cursor );
		goto loop_begin;
	}

	for ( id = bdb_idl_first( candidates, &cursor );
		  id != NOID ; id = bdb_idl_next( candidates, &cursor ) )
	{
		int scopeok;

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

		/* If we inspect more entries than will
		 * fit into the entry cache, stop caching
		 * any subsequent entries
		 */
		nentries++;
		if ( nentries > bdb->bi_cache.c_maxsize && !idflag ) {
			idflag = ID_NOCACHE;
		}

fetch_entry_retry:
		/* get the entry with reader lock */
		ei = NULL;
		rs->sr_err = bdb_cache_find_id( op, ltid,
			id, &ei, idflag, &lock );

		if (rs->sr_err == LDAP_BUSY) {
			rs->sr_text = "ldap server busy";
			send_ldap_result( op, rs );
			goto done;

		} else if ( rs->sr_err == DB_LOCK_DEADLOCK ) {
			if ( !opinfo ) {
				ltid->flags &= ~TXN_DEADLOCK;
				goto fetch_entry_retry;
			}
txnfail:
			opinfo->boi_err = rs->sr_err;
			send_ldap_error( op, rs, LDAP_BUSY, "ldap server busy" );
			goto done;

		} else if ( rs->sr_err == DB_LOCK_NOTGRANTED )
		{
			goto fetch_entry_retry;
		} else if ( rs->sr_err == LDAP_OTHER ) {
			rs->sr_text = "internal error";
			send_ldap_result( op, rs );
			goto done;
		}

		if ( ei && rs->sr_err == LDAP_SUCCESS ) {
			e = ei->bei_e;
		} else {
			e = NULL;
		}

		if ( e == NULL ) {
			if( !BDB_IDL_IS_RANGE(candidates) ) {
				/* only complain for non-range IDLs */
				Debug( LDAP_DEBUG_TRACE,
					LDAP_XSTRING(bdb_search)
					": candidate %ld not found\n",
					(long) id, 0, 0 );
			} else {
				/* get the next ID from the DB */
id_retry:
				rs->sr_err = bdb_get_nextid( bdb, ltid, &cursor );
				if ( rs->sr_err == DB_NOTFOUND ) {
					break;
				} else if ( rs->sr_err == DB_LOCK_DEADLOCK ) {
					if ( opinfo )
						goto txnfail;
					ltid->flags &= ~TXN_DEADLOCK;
					goto id_retry;
				} else if ( rs->sr_err == DB_LOCK_NOTGRANTED ) {
					goto id_retry;
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

		/* Does this candidate actually satisfy the search scope?
		 *
		 * Note that we don't lock access to the bei_parent pointer.
		 * Since only leaf nodes can be deleted, the parent of any
		 * node will always be a valid node. Also since we have
		 * a Read lock on the data, it cannot be renamed out of the
		 * scope while we are looking at it, and unless we're using
		 * BDB_HIER, its parents cannot be moved either.
		 */
		scopeok = 0;
		switch( op->ors_scope ) {
		case LDAP_SCOPE_BASE:
			/* This is always true, yes? */
			if ( id == base.e_id ) scopeok = 1;
			break;

		case LDAP_SCOPE_ONELEVEL:
			if ( ei->bei_parent->bei_id == base.e_id ) scopeok = 1;
			break;

#ifdef LDAP_SCOPE_CHILDREN
		case LDAP_SCOPE_CHILDREN:
			if ( id == base.e_id ) break;
			/* Fall-thru */
#endif
		case LDAP_SCOPE_SUBTREE: {
			EntryInfo *tmp;
			for ( tmp = BEI(e); tmp; tmp = tmp->bei_parent ) {
				if ( tmp->bei_id == base.e_id ) {
					scopeok = 1;
					break;
				}
			}
			} break;
		}

		/* aliases were already dereferenced in candidate list */
		if ( op->ors_deref & LDAP_DEREF_SEARCHING ) {
			/* but if the search base is an alias, and we didn't
			 * deref it when finding, return it.
			 */
			if ( is_entry_alias(e) &&
				((op->ors_deref & LDAP_DEREF_FINDING) ||
					!bvmatch(&e->e_nname, &op->o_req_ndn)))
			{
				goto loop_continue;
			}

			/* scopes is only non-empty for onelevel or subtree */
			if ( !scopeok && BDB_IDL_N(scopes) ) {
				unsigned x;
				if ( op->ors_scope == LDAP_SCOPE_ONELEVEL ) {
					x = bdb_idl_search( scopes, e->e_id );
					if ( scopes[x] == e->e_id ) scopeok = 1;
				} else {
					/* subtree, walk up the tree */
					EntryInfo *tmp = BEI(e);
					for (;tmp->bei_parent; tmp=tmp->bei_parent) {
						x = bdb_idl_search( scopes, tmp->bei_id );
						if ( scopes[x] == tmp->bei_id ) {
							scopeok = 1;
							break;
						}
					}
				}
			}
		}

		/* Not in scope, ignore it */
		if ( !scopeok )
		{
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(bdb_search)
				": %ld scope not okay\n",
				(long) id, 0, 0 );
			goto loop_continue;
		}

		/*
		 * if it's a referral, add it to the list of referrals. only do
		 * this for non-base searches, and don't check the filter
		 * explicitly here since it's only a candidate anyway.
		 */
		if ( !manageDSAit && op->oq_search.rs_scope != LDAP_SCOPE_BASE
			&& is_entry_referral( e ) )
		{
			struct bdb_op_info bois;
			struct bdb_lock_info blis;
			BerVarray erefs = get_entry_referrals( op, e );
			rs->sr_ref = referral_rewrite( erefs, &e->e_name, NULL,
				op->oq_search.rs_scope == LDAP_SCOPE_ONELEVEL
					? LDAP_SCOPE_BASE : LDAP_SCOPE_SUBTREE );

			/* Must set lockinfo so that entry_release will work */
			if (!opinfo) {
				bois.boi_oe.oe_key = bdb;
				bois.boi_txn = NULL;
				bois.boi_err = 0;
				bois.boi_acl_cache = op->o_do_not_cache;
				bois.boi_flag = BOI_DONTFREE;
				bois.boi_locks = &blis;
				blis.bli_next = NULL;
				LDAP_SLIST_INSERT_HEAD( &op->o_extra, &bois.boi_oe,
					oe_next );
			} else {
				blis.bli_next = opinfo->boi_locks;
				opinfo->boi_locks = &blis;
			}
			blis.bli_id = e->e_id;
			blis.bli_lock = lock;
			blis.bli_flag = BLI_DONTFREE;

			rs->sr_entry = e;
			rs->sr_flags = REP_ENTRY_MUSTRELEASE;

			send_search_reference( op, rs );

			if ( blis.bli_flag ) {
#ifdef SLAP_ZONE_ALLOC
				slap_zn_runlock(bdb->bi_cache.c_zctx, e);
#endif
				bdb_cache_return_entry_r(bdb, e, &lock);
				if ( opinfo ) {
					opinfo->boi_locks = blis.bli_next;
				} else {
					LDAP_SLIST_REMOVE( &op->o_extra, &bois.boi_oe,
						OpExtra, oe_next );
				}
			}
			rs->sr_entry = NULL;
			e = NULL;

			ber_bvarray_free( rs->sr_ref );
			ber_bvarray_free( erefs );
			rs->sr_ref = NULL;

			goto loop_continue;
		}

		if ( !manageDSAit && is_entry_glue( e )) {
			goto loop_continue;
		}

		/* if it matches the filter and scope, send it */
		rs->sr_err = test_filter( op, e, op->oq_search.rs_filter );

		if ( rs->sr_err == LDAP_COMPARE_TRUE ) {
			/* check size limit */
			if ( get_pagedresults(op) > SLAP_CONTROL_IGNORED ) {
				if ( rs->sr_nentries >= ((PagedResultsState *)op->o_pagedresults_state)->ps_size ) {
#ifdef SLAP_ZONE_ALLOC
					slap_zn_runlock(bdb->bi_cache.c_zctx, e);
#endif
					bdb_cache_return_entry_r( bdb, e, &lock );
					e = NULL;
					send_paged_response( op, rs, &lastid, tentries );
					goto done;
				}
				lastid = id;
			}

			if (e) {
				struct bdb_op_info bois;
				struct bdb_lock_info blis;

				/* Must set lockinfo so that entry_release will work */
				if (!opinfo) {
					bois.boi_oe.oe_key = bdb;
					bois.boi_txn = NULL;
					bois.boi_err = 0;
					bois.boi_acl_cache = op->o_do_not_cache;
					bois.boi_flag = BOI_DONTFREE;
					bois.boi_locks = &blis;
					blis.bli_next = NULL;
					LDAP_SLIST_INSERT_HEAD( &op->o_extra, &bois.boi_oe,
						oe_next );
				} else {
					blis.bli_next = opinfo->boi_locks;
					opinfo->boi_locks = &blis;
				}
				blis.bli_id = e->e_id;
				blis.bli_lock = lock;
				blis.bli_flag = BLI_DONTFREE;

				/* safe default */
				rs->sr_attrs = op->oq_search.rs_attrs;
				rs->sr_operational_attrs = NULL;
				rs->sr_ctrls = NULL;
				rs->sr_entry = e;
				RS_ASSERT( e->e_private != NULL );
				rs->sr_flags = REP_ENTRY_MUSTRELEASE;
				rs->sr_err = LDAP_SUCCESS;
				rs->sr_err = send_search_entry( op, rs );
				rs->sr_attrs = NULL;
				rs->sr_entry = NULL;

				/* send_search_entry will usually free it.
				 * an overlay might leave its own copy here;
				 * bli_flag will be 0 if lock was already released.
				 */
				if ( blis.bli_flag ) {
#ifdef SLAP_ZONE_ALLOC
					slap_zn_runlock(bdb->bi_cache.c_zctx, e);
#endif
					bdb_cache_return_entry_r(bdb, e, &lock);
					if ( opinfo ) {
						opinfo->boi_locks = blis.bli_next;
					} else {
						LDAP_SLIST_REMOVE( &op->o_extra, &bois.boi_oe,
							OpExtra, oe_next );
					}
				}
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
				LDAP_XSTRING(bdb_search)
				": %ld does not match filter\n",
				(long) id, 0, 0 );
		}

loop_continue:
		if( e != NULL ) {
			/* free reader lock */
#ifdef SLAP_ZONE_ALLOC
			slap_zn_runlock(bdb->bi_cache.c_zctx, e);
#endif
			bdb_cache_return_entry_r( bdb, e , &lock );
			RS_ASSERT( rs->sr_entry == NULL );
			e = NULL;
			rs->sr_entry = NULL;
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
	if( rs->sr_v2ref ) {
		ber_bvarray_free( rs->sr_v2ref );
		rs->sr_v2ref = NULL;
	}
	if( realbase.bv_val ) ch_free( realbase.bv_val );

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
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	void *ret = NULL;

	if ( op->o_threadctx ) {
		ldap_pvt_thread_pool_getkey( op->o_threadctx, (void *)search_stack,
			&ret, NULL );
	} else {
		ret = bdb->bi_search_stack;
	}

	if ( !ret ) {
		ret = ch_malloc( bdb->bi_search_stack_depth * BDB_IDL_UM_SIZE
			* sizeof( ID ) );
		if ( op->o_threadctx ) {
			ldap_pvt_thread_pool_setkey( op->o_threadctx, (void *)search_stack,
				ret, search_stack_free, NULL, NULL );
		} else {
			bdb->bi_search_stack = ret;
		}
	}
	return ret;
}

static int search_candidates(
	Operation *op,
	SlapReply *rs,
	Entry *e,
	DB_TXN *txn,
	ID	*ids,
	ID	*scopes )
{
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	int rc, depth = 1;
	Filter		f, rf, xf, nf;
	ID		*stack;
	AttributeAssertion aa_ref = ATTRIBUTEASSERTION_INIT;
	Filter	sf;
	AttributeAssertion aa_subentry = ATTRIBUTEASSERTION_INIT;

	/*
	 * This routine takes as input a filter (user-filter)
	 * and rewrites it as follows:
	 *	(&(scope=DN)[(objectClass=subentry)]
	 *		(|[(objectClass=referral)(objectClass=alias)](user-filter))
	 */

	Debug(LDAP_DEBUG_TRACE,
		"search_candidates: base=\"%s\" (0x%08lx) scope=%d\n",
		e->e_nname.bv_val, (long) e->e_id, op->oq_search.rs_scope );

	xf.f_or = op->oq_search.rs_filter;
	xf.f_choice = LDAP_FILTER_OR;
	xf.f_next = NULL;

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
			rf.f_next = xf.f_or;
			xf.f_or = &rf;
			depth++;
		}
	}

	f.f_next = NULL;
	f.f_choice = LDAP_FILTER_AND;
	f.f_and = &nf;
	/* Dummy; we compute scope separately now */
	nf.f_choice = SLAPD_FILTER_COMPUTED;
	nf.f_result = LDAP_SUCCESS;
	nf.f_next = ( xf.f_or == op->oq_search.rs_filter )
		? op->oq_search.rs_filter : &xf ;
	/* Filter depth increased again, adding dummy clause */
	depth++;

	if( get_subentries_visibility( op ) ) {
		struct berval bv_subentry = BER_BVC( "subentry" );
		sf.f_choice = LDAP_FILTER_EQUALITY;
		sf.f_ava = &aa_subentry;
		sf.f_av_desc = slap_schema.si_ad_objectClass;
		sf.f_av_value = bv_subentry;
		sf.f_next = nf.f_next;
		nf.f_next = &sf;
	}

	/* Allocate IDL stack, plus 1 more for former tmp */
	if ( depth+1 > bdb->bi_search_stack_depth ) {
		stack = ch_malloc( (depth + 1) * BDB_IDL_UM_SIZE * sizeof( ID ) );
	} else {
		stack = search_stack( op );
	}

	if( op->ors_deref & LDAP_DEREF_SEARCHING ) {
		rc = search_aliases( op, rs, e, txn, ids, scopes, stack );
		if ( BDB_IDL_IS_ZERO( ids ) && rc == LDAP_SUCCESS )
			rc = bdb_dn2idl( op, txn, &e->e_nname, BEI(e), ids, stack );
	} else {
		rc = bdb_dn2idl( op, txn, &e->e_nname, BEI(e), ids, stack );
	}

	if ( rc == LDAP_SUCCESS ) {
		rc = bdb_filter_candidates( op, txn, &f, ids,
			stack, stack+BDB_IDL_UM_SIZE );
	}

	if ( depth+1 > bdb->bi_search_stack_depth ) {
		ch_free( stack );
	}

	if( rc ) {
		Debug(LDAP_DEBUG_TRACE,
			"bdb_search_candidates: failed (rc=%d)\n",
			rc, NULL, NULL );

	} else {
		Debug(LDAP_DEBUG_TRACE,
			"bdb_search_candidates: id=%ld first=%ld last=%ld\n",
			(long) ids[0],
			(long) BDB_IDL_FIRST(ids),
			(long) BDB_IDL_LAST(ids) );
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
