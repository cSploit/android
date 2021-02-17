/* $OpenLDAP$ */
/* syncprov.c - syncrepl provider */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
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
 * OpenLDAP Software.
 */

#include "portable.h"

#ifdef SLAPD_OVER_SYNCPROV

#include <ac/string.h>
#include "lutil.h"
#include "slap.h"
#include "config.h"
#include "ldap_rq.h"

#define	CHECK_CSN	1

/* A modify request on a particular entry */
typedef struct modinst {
	struct modinst *mi_next;
	Operation *mi_op;
} modinst;

typedef struct modtarget {
	struct modinst *mt_mods;
	struct modinst *mt_tail;
	struct berval mt_dn;
	ldap_pvt_thread_mutex_t mt_mutex;
} modtarget;

/* A queued result of a persistent search */
typedef struct syncres {
	struct syncres *s_next;
	Entry *s_e;
	struct berval s_dn;
	struct berval s_ndn;
	struct berval s_uuid;
	struct berval s_csn;
	char s_mode;
	char s_isreference;
} syncres;

/* Record of a persistent search */
typedef struct syncops {
	struct syncops *s_next;
	struct berval	s_base;		/* ndn of search base */
	ID		s_eid;		/* entryID of search base */
	Operation	*s_op;		/* search op */
	int		s_rid;
	int		s_sid;
	struct berval s_filterstr;
	int		s_flags;	/* search status */
#define	PS_IS_REFRESHING	0x01
#define	PS_IS_DETACHED		0x02
#define	PS_WROTE_BASE		0x04
#define	PS_FIND_BASE		0x08
#define	PS_FIX_FILTER		0x10
#define	PS_TASK_QUEUED		0x20

	int		s_inuse;	/* reference count */
	struct syncres *s_res;
	struct syncres *s_restail;
	ldap_pvt_thread_mutex_t	s_mutex;
} syncops;

/* A received sync control */
typedef struct sync_control {
	struct sync_cookie sr_state;
	int sr_rhint;
} sync_control;

#if 0 /* moved back to slap.h */
#define	o_sync	o_ctrlflag[slap_cids.sc_LDAPsync]
#endif
/* o_sync_mode uses data bits of o_sync */
#define	o_sync_mode	o_ctrlflag[slap_cids.sc_LDAPsync]

#define SLAP_SYNC_NONE					(LDAP_SYNC_NONE<<SLAP_CONTROL_SHIFT)
#define SLAP_SYNC_REFRESH				(LDAP_SYNC_REFRESH_ONLY<<SLAP_CONTROL_SHIFT)
#define SLAP_SYNC_PERSIST				(LDAP_SYNC_RESERVED<<SLAP_CONTROL_SHIFT)
#define SLAP_SYNC_REFRESH_AND_PERSIST	(LDAP_SYNC_REFRESH_AND_PERSIST<<SLAP_CONTROL_SHIFT)

/* Record of which searches matched at premodify step */
typedef struct syncmatches {
	struct syncmatches *sm_next;
	syncops *sm_op;
} syncmatches;

/* Session log data */
typedef struct slog_entry {
	struct slog_entry *se_next;
	struct berval se_uuid;
	struct berval se_csn;
	int	se_sid;
	ber_tag_t	se_tag;
} slog_entry;

typedef struct sessionlog {
	BerVarray	sl_mincsn;
	int		*sl_sids;
	int		sl_numcsns;
	int		sl_num;
	int		sl_size;
	slog_entry *sl_head;
	slog_entry *sl_tail;
	ldap_pvt_thread_mutex_t sl_mutex;
} sessionlog;

/* The main state for this overlay */
typedef struct syncprov_info_t {
	syncops		*si_ops;
	struct berval	si_contextdn;
	BerVarray	si_ctxcsn;	/* ldapsync context */
	int		*si_sids;
	int		si_numcsns;
	int		si_chkops;	/* checkpointing info */
	int		si_chktime;
	int		si_numops;	/* number of ops since last checkpoint */
	int		si_nopres;	/* Skip present phase */
	int		si_usehint;	/* use reload hint */
	int		si_active;	/* True if there are active mods */
	int		si_dirty;	/* True if the context is dirty, i.e changes
						 * have been made without updating the csn. */
	time_t	si_chklast;	/* time of last checkpoint */
	Avlnode	*si_mods;	/* entries being modified */
	sessionlog	*si_logs;
	ldap_pvt_thread_rdwr_t	si_csn_rwlock;
	ldap_pvt_thread_mutex_t	si_ops_mutex;
	ldap_pvt_thread_mutex_t	si_mods_mutex;
	ldap_pvt_thread_mutex_t	si_resp_mutex;
} syncprov_info_t;

typedef struct opcookie {
	slap_overinst *son;
	syncmatches *smatches;
	modtarget *smt;
	Entry *se;
	struct berval sdn;	/* DN of entry, for deletes */
	struct berval sndn;
	struct berval suuid;	/* UUID of entry */
	struct berval sctxcsn;
	short osid;	/* sid of op csn */
	short rsid;	/* sid of relay */
	short sreference;	/* Is the entry a reference? */
} opcookie;

typedef struct mutexint {
	ldap_pvt_thread_mutex_t mi_mutex;
	int mi_int;
} mutexint;

typedef struct fbase_cookie {
	struct berval *fdn;	/* DN of a modified entry, for scope testing */
	syncops *fss;	/* persistent search we're testing against */
	int fbase;	/* if TRUE we found the search base and it's still valid */
	int fscope;	/* if TRUE then fdn is within the psearch scope */
} fbase_cookie;

static AttributeName csn_anlist[3];
static AttributeName uuid_anlist[2];

/* Build a LDAPsync intermediate state control */
static int
syncprov_state_ctrl(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e,
	int		entry_sync_state,
	LDAPControl	**ctrls,
	int		num_ctrls,
	int		send_cookie,
	struct berval	*cookie )
{
	Attribute* a;
	int ret;

	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	LDAPControl *cp;
	struct berval bv;
	struct berval	entryuuid_bv = BER_BVNULL;

	ber_init2( ber, 0, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
		AttributeDescription *desc = a->a_desc;
		if ( desc == slap_schema.si_ad_entryUUID ) {
			entryuuid_bv = a->a_nvals[0];
			break;
		}
	}

	/* FIXME: what if entryuuid is NULL or empty ? */

	if ( send_cookie && cookie ) {
		ber_printf( ber, "{eOON}",
			entry_sync_state, &entryuuid_bv, cookie );
	} else {
		ber_printf( ber, "{eON}",
			entry_sync_state, &entryuuid_bv );
	}

	ret = ber_flatten2( ber, &bv, 0 );
	if ( ret == 0 ) {
		cp = op->o_tmpalloc( sizeof( LDAPControl ) + bv.bv_len, op->o_tmpmemctx );
		cp->ldctl_oid = LDAP_CONTROL_SYNC_STATE;
		cp->ldctl_iscritical = (op->o_sync == SLAP_CONTROL_CRITICAL);
		cp->ldctl_value.bv_val = (char *)&cp[1];
		cp->ldctl_value.bv_len = bv.bv_len;
		AC_MEMCPY( cp->ldctl_value.bv_val, bv.bv_val, bv.bv_len );
		ctrls[num_ctrls] = cp;
	}
	ber_free_buf( ber );

	if ( ret < 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			"slap_build_sync_ctrl: ber_flatten2 failed (%d)\n",
			ret, 0, 0 );
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		return LDAP_OTHER;
	}

	return LDAP_SUCCESS;
}

/* Build a LDAPsync final state control */
static int
syncprov_done_ctrl(
	Operation	*op,
	SlapReply	*rs,
	LDAPControl	**ctrls,
	int			num_ctrls,
	int			send_cookie,
	struct berval *cookie,
	int			refreshDeletes )
{
	int ret;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	LDAPControl *cp;
	struct berval bv;

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	ber_printf( ber, "{" );
	if ( send_cookie && cookie ) {
		ber_printf( ber, "O", cookie );
	}
	if ( refreshDeletes == LDAP_SYNC_REFRESH_DELETES ) {
		ber_printf( ber, "b", refreshDeletes );
	}
	ber_printf( ber, "N}" );

	ret = ber_flatten2( ber, &bv, 0 );
	if ( ret == 0 ) {
		cp = op->o_tmpalloc( sizeof( LDAPControl ) + bv.bv_len, op->o_tmpmemctx );
		cp->ldctl_oid = LDAP_CONTROL_SYNC_DONE;
		cp->ldctl_iscritical = (op->o_sync == SLAP_CONTROL_CRITICAL);
		cp->ldctl_value.bv_val = (char *)&cp[1];
		cp->ldctl_value.bv_len = bv.bv_len;
		AC_MEMCPY( cp->ldctl_value.bv_val, bv.bv_val, bv.bv_len );
		ctrls[num_ctrls] = cp;
	}

	ber_free_buf( ber );

	if ( ret < 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			"syncprov_done_ctrl: ber_flatten2 failed (%d)\n",
			ret, 0, 0 );
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		return LDAP_OTHER;
	}

	return LDAP_SUCCESS;
}

static int
syncprov_sendinfo(
	Operation	*op,
	SlapReply	*rs,
	int			type,
	struct berval *cookie,
	int			refreshDone,
	BerVarray	syncUUIDs,
	int			refreshDeletes )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	struct berval rspdata;

	int ret;

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	if ( type ) {
		switch ( type ) {
		case LDAP_TAG_SYNC_NEW_COOKIE:
			ber_printf( ber, "tO", type, cookie );
			break;
		case LDAP_TAG_SYNC_REFRESH_DELETE:
		case LDAP_TAG_SYNC_REFRESH_PRESENT:
			ber_printf( ber, "t{", type );
			if ( cookie ) {
				ber_printf( ber, "O", cookie );
			}
			if ( refreshDone == 0 ) {
				ber_printf( ber, "b", refreshDone );
			}
			ber_printf( ber, "N}" );
			break;
		case LDAP_TAG_SYNC_ID_SET:
			ber_printf( ber, "t{", type );
			if ( cookie ) {
				ber_printf( ber, "O", cookie );
			}
			if ( refreshDeletes == 1 ) {
				ber_printf( ber, "b", refreshDeletes );
			}
			ber_printf( ber, "[W]", syncUUIDs );
			ber_printf( ber, "N}" );
			break;
		default:
			Debug( LDAP_DEBUG_TRACE,
				"syncprov_sendinfo: invalid syncinfo type (%d)\n",
				type, 0, 0 );
			return LDAP_OTHER;
		}
	}

	ret = ber_flatten2( ber, &rspdata, 0 );

	if ( ret < 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			"syncprov_sendinfo: ber_flatten2 failed (%d)\n",
			ret, 0, 0 );
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		return LDAP_OTHER;
	}

	rs->sr_rspoid = LDAP_SYNC_INFO;
	rs->sr_rspdata = &rspdata;
	send_ldap_intermediate( op, rs );
	rs->sr_rspdata = NULL;
	ber_free_buf( ber );

	return LDAP_SUCCESS;
}

/* Find a modtarget in an AVL tree */
static int
sp_avl_cmp( const void *c1, const void *c2 )
{
	const modtarget *m1, *m2;
	int rc;

	m1 = c1; m2 = c2;
	rc = m1->mt_dn.bv_len - m2->mt_dn.bv_len;

	if ( rc ) return rc;
	return ber_bvcmp( &m1->mt_dn, &m2->mt_dn );
}

/* syncprov_findbase:
 *   finds the true DN of the base of a search (with alias dereferencing) and
 * checks to make sure the base entry doesn't get replaced with a different
 * entry (e.g., swapping trees via ModDN, or retargeting an alias). If a
 * change is detected, any persistent search on this base must be terminated /
 * reloaded.
 *   On the first call, we just save the DN and entryID. On subsequent calls
 * we compare the DN and entryID with the saved values.
 */
static int
findbase_cb( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;

	if ( rs->sr_type == REP_SEARCH && rs->sr_err == LDAP_SUCCESS ) {
		fbase_cookie *fc = sc->sc_private;

		/* If no entryID, we're looking for the first time.
		 * Just store whatever we got.
		 */
		if ( fc->fss->s_eid == NOID ) {
			fc->fbase = 2;
			fc->fss->s_eid = rs->sr_entry->e_id;
			ber_dupbv( &fc->fss->s_base, &rs->sr_entry->e_nname );

		} else if ( rs->sr_entry->e_id == fc->fss->s_eid &&
			dn_match( &rs->sr_entry->e_nname, &fc->fss->s_base )) {

		/* OK, the DN is the same and the entryID is the same. */
			fc->fbase = 1;
		}
	}
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "findbase failed! %d\n", rs->sr_err,0,0 );
	}
	return LDAP_SUCCESS;
}

static Filter generic_filter = { LDAP_FILTER_PRESENT, { 0 }, NULL };
static struct berval generic_filterstr = BER_BVC("(objectclass=*)");

static int
syncprov_findbase( Operation *op, fbase_cookie *fc )
{
	/* Use basic parameters from syncrepl search, but use
	 * current op's threadctx / tmpmemctx
	 */
	ldap_pvt_thread_mutex_lock( &fc->fss->s_mutex );
	if ( fc->fss->s_flags & PS_FIND_BASE ) {
		slap_callback cb = {0};
		Operation fop;
		SlapReply frs = { REP_RESULT };
		int rc;

		fc->fss->s_flags ^= PS_FIND_BASE;
		ldap_pvt_thread_mutex_unlock( &fc->fss->s_mutex );

		fop = *fc->fss->s_op;

		fop.o_bd = fop.o_bd->bd_self;
		fop.o_hdr = op->o_hdr;
		fop.o_time = op->o_time;
		fop.o_tincr = op->o_tincr;
		fop.o_extra = op->o_extra;

		cb.sc_response = findbase_cb;
		cb.sc_private = fc;

		fop.o_sync_mode = 0;	/* turn off sync mode */
		fop.o_managedsait = SLAP_CONTROL_CRITICAL;
		fop.o_callback = &cb;
		fop.o_tag = LDAP_REQ_SEARCH;
		fop.ors_scope = LDAP_SCOPE_BASE;
		fop.ors_limit = NULL;
		fop.ors_slimit = 1;
		fop.ors_tlimit = SLAP_NO_LIMIT;
		fop.ors_attrs = slap_anlist_no_attrs;
		fop.ors_attrsonly = 1;
		fop.ors_filter = &generic_filter;
		fop.ors_filterstr = generic_filterstr;

		rc = fop.o_bd->be_search( &fop, &frs );
	} else {
		ldap_pvt_thread_mutex_unlock( &fc->fss->s_mutex );
		fc->fbase = 1;
	}

	/* After the first call, see if the fdn resides in the scope */
	if ( fc->fbase == 1 ) {
		switch ( fc->fss->s_op->ors_scope ) {
		case LDAP_SCOPE_BASE:
			fc->fscope = dn_match( fc->fdn, &fc->fss->s_base );
			break;
		case LDAP_SCOPE_ONELEVEL: {
			struct berval pdn;
			dnParent( fc->fdn, &pdn );
			fc->fscope = dn_match( &pdn, &fc->fss->s_base );
			break; }
		case LDAP_SCOPE_SUBTREE:
			fc->fscope = dnIsSuffix( fc->fdn, &fc->fss->s_base );
			break;
		case LDAP_SCOPE_SUBORDINATE:
			fc->fscope = dnIsSuffix( fc->fdn, &fc->fss->s_base ) &&
				!dn_match( fc->fdn, &fc->fss->s_base );
			break;
		}
	}

	if ( fc->fbase )
		return LDAP_SUCCESS;

	/* If entryID has changed, then the base of this search has
	 * changed. Invalidate the psearch.
	 */
	return LDAP_NO_SUCH_OBJECT;
}

/* syncprov_findcsn:
 *   This function has three different purposes, but they all use a search
 * that filters on entryCSN so they're combined here.
 * 1: at startup time, after a contextCSN has been read from the database,
 * we search for all entries with CSN >= contextCSN in case the contextCSN
 * was not checkpointed at the previous shutdown.
 *
 * 2: when the current contextCSN is known and we have a sync cookie, we search
 * for one entry with CSN = the cookie CSN. If not found, try <= cookie CSN.
 * If an entry is found, the cookie CSN is valid, otherwise it is stale.
 *
 * 3: during a refresh phase, we search for all entries with CSN <= the cookie
 * CSN, and generate Present records for them. We always collect this result
 * in SyncID sets, even if there's only one match.
 */
typedef enum find_csn_t {
	FIND_MAXCSN	= 1,
	FIND_CSN	= 2,
	FIND_PRESENT	= 3
} find_csn_t;

static int
findmax_cb( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH && rs->sr_err == LDAP_SUCCESS ) {
		struct berval *maxcsn = op->o_callback->sc_private;
		Attribute *a = attr_find( rs->sr_entry->e_attrs,
			slap_schema.si_ad_entryCSN );

		if ( a && ber_bvcmp( &a->a_vals[0], maxcsn ) > 0 &&
			slap_parse_csn_sid( &a->a_vals[0] ) == slap_serverID ) {
			maxcsn->bv_len = a->a_vals[0].bv_len;
			strcpy( maxcsn->bv_val, a->a_vals[0].bv_val );
		}
	}
	return LDAP_SUCCESS;
}

static int
findcsn_cb( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;

	/* We just want to know that at least one exists, so it's OK if
	 * we exceed the unchecked limit.
	 */
	if ( rs->sr_err == LDAP_ADMINLIMIT_EXCEEDED ||
		(rs->sr_type == REP_SEARCH && rs->sr_err == LDAP_SUCCESS )) {
		sc->sc_private = (void *)1;
	}
	return LDAP_SUCCESS;
}

/* Build a list of entryUUIDs for sending in a SyncID set */

#define UUID_LEN	16

typedef struct fpres_cookie {
	int num;
	BerVarray uuids;
	char *last;
} fpres_cookie;

static int
findpres_cb( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;
	fpres_cookie *pc = sc->sc_private;
	Attribute *a;
	int ret = SLAP_CB_CONTINUE;

	switch ( rs->sr_type ) {
	case REP_SEARCH:
		a = attr_find( rs->sr_entry->e_attrs, slap_schema.si_ad_entryUUID );
		if ( a ) {
			pc->uuids[pc->num].bv_val = pc->last;
			AC_MEMCPY( pc->uuids[pc->num].bv_val, a->a_nvals[0].bv_val,
				pc->uuids[pc->num].bv_len );
			pc->num++;
			pc->last = pc->uuids[pc->num].bv_val;
			pc->uuids[pc->num].bv_val = NULL;
		}
		ret = LDAP_SUCCESS;
		if ( pc->num != SLAP_SYNCUUID_SET_SIZE )
			break;
		/* FALLTHRU */
	case REP_RESULT:
		ret = rs->sr_err;
		if ( pc->num ) {
			ret = syncprov_sendinfo( op, rs, LDAP_TAG_SYNC_ID_SET, NULL,
				0, pc->uuids, 0 );
			pc->uuids[pc->num].bv_val = pc->last;
			pc->num = 0;
			pc->last = pc->uuids[0].bv_val;
		}
		break;
	default:
		break;
	}
	return ret;
}

static int
syncprov_findcsn( Operation *op, find_csn_t mode, struct berval *csn )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t		*si = on->on_bi.bi_private;

	slap_callback cb = {0};
	Operation fop;
	SlapReply frs = { REP_RESULT };
	char buf[LDAP_PVT_CSNSTR_BUFSIZE + STRLENOF("(entryCSN<=)")];
	char cbuf[LDAP_PVT_CSNSTR_BUFSIZE];
	struct berval maxcsn;
	Filter cf;
	AttributeAssertion eq = ATTRIBUTEASSERTION_INIT;
	fpres_cookie pcookie;
	sync_control *srs = NULL;
	struct slap_limits_set fc_limits;
	int i, rc = LDAP_SUCCESS, findcsn_retry = 1;
	int maxid;

	if ( mode != FIND_MAXCSN ) {
		srs = op->o_controls[slap_cids.sc_LDAPsync];
	}

	fop = *op;
	fop.o_sync_mode &= SLAP_CONTROL_MASK;	/* turn off sync_mode */
	/* We want pure entries, not referrals */
	fop.o_managedsait = SLAP_CONTROL_CRITICAL;

	cf.f_ava = &eq;
	cf.f_av_desc = slap_schema.si_ad_entryCSN;
	BER_BVZERO( &cf.f_av_value );
	cf.f_next = NULL;

	fop.o_callback = &cb;
	fop.ors_limit = NULL;
	fop.ors_tlimit = SLAP_NO_LIMIT;
	fop.ors_filter = &cf;
	fop.ors_filterstr.bv_val = buf;

again:
	switch( mode ) {
	case FIND_MAXCSN:
		cf.f_choice = LDAP_FILTER_GE;
		/* If there are multiple CSNs, use the one with our serverID */
		for ( i=0; i<si->si_numcsns; i++) {
			if ( slap_serverID == si->si_sids[i] ) {
				maxid = i;
				break;
			}
		}
		if ( i == si->si_numcsns ) {
			/* No match: this is multimaster, and none of the content in the DB
			 * originated locally. Treat like no CSN.
			 */
			return LDAP_NO_SUCH_OBJECT;
		}
		cf.f_av_value = si->si_ctxcsn[maxid];
		fop.ors_filterstr.bv_len = snprintf( buf, sizeof( buf ),
			"(entryCSN>=%s)", cf.f_av_value.bv_val );
		if ( fop.ors_filterstr.bv_len >= sizeof( buf ) ) {
			return LDAP_OTHER;
		}
		fop.ors_attrsonly = 0;
		fop.ors_attrs = csn_anlist;
		fop.ors_slimit = SLAP_NO_LIMIT;
		cb.sc_private = &maxcsn;
		cb.sc_response = findmax_cb;
		strcpy( cbuf, cf.f_av_value.bv_val );
		maxcsn.bv_val = cbuf;
		maxcsn.bv_len = cf.f_av_value.bv_len;
		break;
	case FIND_CSN:
		if ( BER_BVISEMPTY( &cf.f_av_value )) {
			cf.f_av_value = *csn;
		}
		fop.o_dn = op->o_bd->be_rootdn;
		fop.o_ndn = op->o_bd->be_rootndn;
		fop.o_req_dn = op->o_bd->be_suffix[0];
		fop.o_req_ndn = op->o_bd->be_nsuffix[0];
		/* Look for exact match the first time */
		if ( findcsn_retry ) {
			cf.f_choice = LDAP_FILTER_EQUALITY;
			fop.ors_filterstr.bv_len = snprintf( buf, sizeof( buf ),
				"(entryCSN=%s)", cf.f_av_value.bv_val );
		/* On retry, look for <= */
		} else {
			cf.f_choice = LDAP_FILTER_LE;
			fop.ors_limit = &fc_limits;
			memset( &fc_limits, 0, sizeof( fc_limits ));
			fc_limits.lms_s_unchecked = 1;
			fop.ors_filterstr.bv_len = snprintf( buf, sizeof( buf ),
				"(entryCSN<=%s)", cf.f_av_value.bv_val );
		}
		if ( fop.ors_filterstr.bv_len >= sizeof( buf ) ) {
			return LDAP_OTHER;
		}
		fop.ors_attrsonly = 1;
		fop.ors_attrs = slap_anlist_no_attrs;
		fop.ors_slimit = 1;
		cb.sc_private = NULL;
		cb.sc_response = findcsn_cb;
		break;
	case FIND_PRESENT:
		fop.ors_filter = op->ors_filter;
		fop.ors_filterstr = op->ors_filterstr;
		fop.ors_attrsonly = 0;
		fop.ors_attrs = uuid_anlist;
		fop.ors_slimit = SLAP_NO_LIMIT;
		cb.sc_private = &pcookie;
		cb.sc_response = findpres_cb;
		pcookie.num = 0;

		/* preallocate storage for a full set */
		pcookie.uuids = op->o_tmpalloc( (SLAP_SYNCUUID_SET_SIZE+1) *
			sizeof(struct berval) + SLAP_SYNCUUID_SET_SIZE * UUID_LEN,
			op->o_tmpmemctx );
		pcookie.last = (char *)(pcookie.uuids + SLAP_SYNCUUID_SET_SIZE+1);
		pcookie.uuids[0].bv_val = pcookie.last;
		pcookie.uuids[0].bv_len = UUID_LEN;
		for (i=1; i<SLAP_SYNCUUID_SET_SIZE; i++) {
			pcookie.uuids[i].bv_val = pcookie.uuids[i-1].bv_val + UUID_LEN;
			pcookie.uuids[i].bv_len = UUID_LEN;
		}
		break;
	}

	fop.o_bd->bd_info = (BackendInfo *)on->on_info;
	fop.o_bd->be_search( &fop, &frs );
	fop.o_bd->bd_info = (BackendInfo *)on;

	switch( mode ) {
	case FIND_MAXCSN:
		if ( ber_bvcmp( &si->si_ctxcsn[maxid], &maxcsn )) {
#ifdef CHECK_CSN
			Syntax *syn = slap_schema.si_ad_contextCSN->ad_type->sat_syntax;
			assert( !syn->ssyn_validate( syn, &maxcsn ));
#endif
			ber_bvreplace( &si->si_ctxcsn[maxid], &maxcsn );
			si->si_numops++;	/* ensure a checkpoint */
		}
		break;
	case FIND_CSN:
		/* If matching CSN was not found, invalidate the context. */
		if ( !cb.sc_private ) {
			/* If we didn't find an exact match, then try for <= */
			if ( findcsn_retry ) {
				findcsn_retry = 0;
				rs_reinit( &frs, REP_RESULT );
				goto again;
			}
			rc = LDAP_NO_SUCH_OBJECT;
		}
		break;
	case FIND_PRESENT:
		op->o_tmpfree( pcookie.uuids, op->o_tmpmemctx );
		break;
	}

	return rc;
}

/* Should find a place to cache these */
static mutexint *get_mutexint()
{
	mutexint *mi = ch_malloc( sizeof( mutexint ));
	ldap_pvt_thread_mutex_init( &mi->mi_mutex );
	mi->mi_int = 1;
	return mi;
}

static void inc_mutexint( mutexint *mi )
{
	ldap_pvt_thread_mutex_lock( &mi->mi_mutex );
	mi->mi_int++;
	ldap_pvt_thread_mutex_unlock( &mi->mi_mutex );
}

/* return resulting counter */
static int dec_mutexint( mutexint *mi )
{
	int i;
	ldap_pvt_thread_mutex_lock( &mi->mi_mutex );
	i = --mi->mi_int;
	ldap_pvt_thread_mutex_unlock( &mi->mi_mutex );
	if ( !i ) {
		ldap_pvt_thread_mutex_destroy( &mi->mi_mutex );
		ch_free( mi );
	}
	return i;
}

static void
syncprov_free_syncop( syncops *so )
{
	syncres *sr, *srnext;
	GroupAssertion *ga, *gnext;

	ldap_pvt_thread_mutex_lock( &so->s_mutex );
	/* already being freed, or still in use */
	if ( !so->s_inuse || --so->s_inuse > 0 ) {
		ldap_pvt_thread_mutex_unlock( &so->s_mutex );
		return;
	}
	ldap_pvt_thread_mutex_unlock( &so->s_mutex );
	if ( so->s_flags & PS_IS_DETACHED ) {
		filter_free( so->s_op->ors_filter );
		for ( ga = so->s_op->o_groups; ga; ga=gnext ) {
			gnext = ga->ga_next;
			ch_free( ga );
		}
		ch_free( so->s_op );
	}
	ch_free( so->s_base.bv_val );
	for ( sr=so->s_res; sr; sr=srnext ) {
		srnext = sr->s_next;
		if ( sr->s_e ) {
			if ( !dec_mutexint( sr->s_e->e_private )) {
				sr->s_e->e_private = NULL;
				entry_free( sr->s_e );
			}
		}
		ch_free( sr );
	}
	ldap_pvt_thread_mutex_destroy( &so->s_mutex );
	ch_free( so );
}

/* Send a persistent search response */
static int
syncprov_sendresp( Operation *op, opcookie *opc, syncops *so, int mode )
{
	SlapReply rs = { REP_SEARCH };
	struct berval cookie, csns[2];
	Entry e_uuid = {0};
	Attribute a_uuid = {0};

	if ( so->s_op->o_abandon )
		return SLAPD_ABANDON;

	rs.sr_ctrls = op->o_tmpalloc( sizeof(LDAPControl *)*2, op->o_tmpmemctx );
	rs.sr_ctrls[1] = NULL;
	rs.sr_flags = REP_CTRLS_MUSTBEFREED;
	csns[0] = opc->sctxcsn;
	BER_BVZERO( &csns[1] );
	slap_compose_sync_cookie( op, &cookie, csns, so->s_rid, slap_serverID ? slap_serverID : -1 );

#ifdef LDAP_DEBUG
	if ( so->s_sid > 0 ) {
		Debug( LDAP_DEBUG_SYNC, "syncprov_sendresp: to=%03x, cookie=%s\n",
			so->s_sid, cookie.bv_val, 0 );
	} else {
		Debug( LDAP_DEBUG_SYNC, "syncprov_sendresp: cookie=%s\n",
			cookie.bv_val, 0, 0 );
	}
#endif

	e_uuid.e_attrs = &a_uuid;
	a_uuid.a_desc = slap_schema.si_ad_entryUUID;
	a_uuid.a_nvals = &opc->suuid;
	rs.sr_err = syncprov_state_ctrl( op, &rs, &e_uuid,
		mode, rs.sr_ctrls, 0, 1, &cookie );
	op->o_tmpfree( cookie.bv_val, op->o_tmpmemctx );

	rs.sr_entry = &e_uuid;
	if ( mode == LDAP_SYNC_ADD || mode == LDAP_SYNC_MODIFY ) {
		e_uuid = *opc->se;
		e_uuid.e_private = NULL;
	}

	switch( mode ) {
	case LDAP_SYNC_ADD:
		if ( opc->sreference && so->s_op->o_managedsait <= SLAP_CONTROL_IGNORED ) {
			rs.sr_ref = get_entry_referrals( op, rs.sr_entry );
			rs.sr_err = send_search_reference( op, &rs );
			ber_bvarray_free( rs.sr_ref );
			break;
		}
		/* fallthru */
	case LDAP_SYNC_MODIFY:
		rs.sr_attrs = op->ors_attrs;
		rs.sr_err = send_search_entry( op, &rs );
		break;
	case LDAP_SYNC_DELETE:
		e_uuid.e_attrs = NULL;
		e_uuid.e_name = opc->sdn;
		e_uuid.e_nname = opc->sndn;
		if ( opc->sreference && so->s_op->o_managedsait <= SLAP_CONTROL_IGNORED ) {
			struct berval bv = BER_BVNULL;
			rs.sr_ref = &bv;
			rs.sr_err = send_search_reference( op, &rs );
		} else {
			rs.sr_err = send_search_entry( op, &rs );
		}
		break;
	default:
		assert(0);
	}
	return rs.sr_err;
}

static void
syncprov_qstart( syncops *so );

/* Play back queued responses */
static int
syncprov_qplay( Operation *op, syncops *so )
{
	slap_overinst *on = LDAP_SLIST_FIRST(&so->s_op->o_extra)->oe_key;
	syncres *sr;
	opcookie opc;
	int rc = 0;

	opc.son = on;

	do {
		ldap_pvt_thread_mutex_lock( &so->s_mutex );
		sr = so->s_res;
		/* Exit loop with mutex held */
		if ( !sr )
			break;
		so->s_res = sr->s_next;
		if ( !so->s_res )
			so->s_restail = NULL;
		ldap_pvt_thread_mutex_unlock( &so->s_mutex );

		if ( !so->s_op->o_abandon ) {

			if ( sr->s_mode == LDAP_SYNC_NEW_COOKIE ) {
				SlapReply rs = { REP_INTERMEDIATE };

				rc = syncprov_sendinfo( op, &rs, LDAP_TAG_SYNC_NEW_COOKIE,
					&sr->s_csn, 0, NULL, 0 );
			} else {
				opc.sdn = sr->s_dn;
				opc.sndn = sr->s_ndn;
				opc.suuid = sr->s_uuid;
				opc.sctxcsn = sr->s_csn;
				opc.sreference = sr->s_isreference;
				opc.se = sr->s_e;

				rc = syncprov_sendresp( op, &opc, so, sr->s_mode );
			}
		}

		if ( sr->s_e ) {
			if ( !dec_mutexint( sr->s_e->e_private )) {
				sr->s_e->e_private = NULL;
				entry_free ( sr->s_e );
			}
		}
		ch_free( sr );

		if ( so->s_op->o_abandon )
			continue;

		/* Exit loop with mutex held */
		ldap_pvt_thread_mutex_lock( &so->s_mutex );
		break;

	} while (1);

	/* We now only send one change at a time, to prevent one
	 * psearch from hogging all the CPU. Resubmit this task if
	 * there are more responses queued and no errors occurred.
	 */

	if ( rc == 0 && so->s_res ) {
		syncprov_qstart( so );
	} else {
		so->s_flags ^= PS_TASK_QUEUED;
	}

	ldap_pvt_thread_mutex_unlock( &so->s_mutex );
	return rc;
}

/* task for playing back queued responses */
static void *
syncprov_qtask( void *ctx, void *arg )
{
	syncops *so = arg;
	OperationBuffer opbuf;
	Operation *op;
	BackendDB be;
	int rc;

	op = &opbuf.ob_op;
	*op = *so->s_op;
	op->o_hdr = &opbuf.ob_hdr;
	op->o_controls = opbuf.ob_controls;
	memset( op->o_controls, 0, sizeof(opbuf.ob_controls) );
	op->o_sync = SLAP_CONTROL_IGNORED;

	*op->o_hdr = *so->s_op->o_hdr;

	op->o_tmpmemctx = slap_sl_mem_create(SLAP_SLAB_SIZE, SLAP_SLAB_STACK, ctx, 1);
	op->o_tmpmfuncs = &slap_sl_mfuncs;
	op->o_threadctx = ctx;

	/* syncprov_qplay expects a fake db */
	be = *so->s_op->o_bd;
	be.be_flags |= SLAP_DBFLAG_OVERLAY;
	op->o_bd = &be;
	LDAP_SLIST_FIRST(&op->o_extra) = NULL;
	op->o_callback = NULL;

	rc = syncprov_qplay( op, so );

	/* decrement use count... */
	syncprov_free_syncop( so );

	return NULL;
}

/* Start the task to play back queued psearch responses */
static void
syncprov_qstart( syncops *so )
{
	so->s_flags |= PS_TASK_QUEUED;
	so->s_inuse++;
	ldap_pvt_thread_pool_submit( &connection_pool, 
		syncprov_qtask, so );
}

/* Queue a persistent search response */
static int
syncprov_qresp( opcookie *opc, syncops *so, int mode )
{
	syncres *sr;
	int srsize;
	struct berval cookie = opc->sctxcsn;

	if ( mode == LDAP_SYNC_NEW_COOKIE ) {
		syncprov_info_t	*si = opc->son->on_bi.bi_private;

		slap_compose_sync_cookie( NULL, &cookie, si->si_ctxcsn,
			so->s_rid, slap_serverID ? slap_serverID : -1);
	}

	srsize = sizeof(syncres) + opc->suuid.bv_len + 1 +
		opc->sdn.bv_len + 1 + opc->sndn.bv_len + 1;
	if ( cookie.bv_len )
		srsize += cookie.bv_len + 1;
	sr = ch_malloc( srsize );
	sr->s_next = NULL;
	sr->s_e = opc->se;
	/* bump refcount on this entry */
	if ( opc->se )
		inc_mutexint( opc->se->e_private );
	sr->s_dn.bv_val = (char *)(sr + 1);
	sr->s_dn.bv_len = opc->sdn.bv_len;
	sr->s_mode = mode;
	sr->s_isreference = opc->sreference;
	sr->s_ndn.bv_val = lutil_strcopy( sr->s_dn.bv_val,
		 opc->sdn.bv_val ) + 1;
	sr->s_ndn.bv_len = opc->sndn.bv_len;
	sr->s_uuid.bv_val = lutil_strcopy( sr->s_ndn.bv_val,
		 opc->sndn.bv_val ) + 1;
	sr->s_uuid.bv_len = opc->suuid.bv_len;
	AC_MEMCPY( sr->s_uuid.bv_val, opc->suuid.bv_val, opc->suuid.bv_len );
	if ( cookie.bv_len ) {
		sr->s_csn.bv_val = sr->s_uuid.bv_val + sr->s_uuid.bv_len + 1;
		strcpy( sr->s_csn.bv_val, cookie.bv_val );
	} else {
		sr->s_csn.bv_val = NULL;
	}
	sr->s_csn.bv_len = cookie.bv_len;

	if ( mode == LDAP_SYNC_NEW_COOKIE && cookie.bv_val ) {
		ch_free( cookie.bv_val );
	}

	ldap_pvt_thread_mutex_lock( &so->s_mutex );
	if ( !so->s_res ) {
		so->s_res = sr;
	} else {
		so->s_restail->s_next = sr;
	}
	so->s_restail = sr;

	/* If the base of the psearch was modified, check it next time round */
	if ( so->s_flags & PS_WROTE_BASE ) {
		so->s_flags ^= PS_WROTE_BASE;
		so->s_flags |= PS_FIND_BASE;
	}
	if (( so->s_flags & (PS_IS_DETACHED|PS_TASK_QUEUED)) == PS_IS_DETACHED ) {
		syncprov_qstart( so );
	}
	ldap_pvt_thread_mutex_unlock( &so->s_mutex );
	return LDAP_SUCCESS;
}

static int
syncprov_drop_psearch( syncops *so, int lock )
{
	if ( so->s_flags & PS_IS_DETACHED ) {
		if ( lock )
			ldap_pvt_thread_mutex_lock( &so->s_op->o_conn->c_mutex );
		so->s_op->o_conn->c_n_ops_executing--;
		so->s_op->o_conn->c_n_ops_completed++;
		LDAP_STAILQ_REMOVE( &so->s_op->o_conn->c_ops, so->s_op, Operation,
			o_next );
		if ( lock )
			ldap_pvt_thread_mutex_unlock( &so->s_op->o_conn->c_mutex );
	}
	syncprov_free_syncop( so );

	return 0;
}

static int
syncprov_ab_cleanup( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;
	op->o_callback = sc->sc_next;
	syncprov_drop_psearch( op->o_callback->sc_private, 0 );
	op->o_tmpfree( sc, op->o_tmpmemctx );
	return 0;
}

static int
syncprov_op_abandon( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t		*si = on->on_bi.bi_private;
	syncops *so, *soprev;

	ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
	for ( so=si->si_ops, soprev = (syncops *)&si->si_ops; so;
		soprev=so, so=so->s_next ) {
		if ( so->s_op->o_connid == op->o_connid &&
			so->s_op->o_msgid == op->orn_msgid ) {
				so->s_op->o_abandon = 1;
				soprev->s_next = so->s_next;
				break;
		}
	}
	ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
	if ( so ) {
		/* Is this really a Cancel exop? */
		if ( op->o_tag != LDAP_REQ_ABANDON ) {
			so->s_op->o_cancel = SLAP_CANCEL_ACK;
			rs->sr_err = LDAP_CANCELLED;
			send_ldap_result( so->s_op, rs );
			if ( so->s_flags & PS_IS_DETACHED ) {
				slap_callback *cb;
				cb = op->o_tmpcalloc( 1, sizeof(slap_callback), op->o_tmpmemctx );
				cb->sc_cleanup = syncprov_ab_cleanup;
				cb->sc_next = op->o_callback;
				cb->sc_private = so;
				op->o_callback = cb;
				return SLAP_CB_CONTINUE;
			}
		}
		syncprov_drop_psearch( so, 0 );
	}
	return SLAP_CB_CONTINUE;
}

/* Find which persistent searches are affected by this operation */
static void
syncprov_matchops( Operation *op, opcookie *opc, int saveit )
{
	slap_overinst *on = opc->son;
	syncprov_info_t		*si = on->on_bi.bi_private;

	fbase_cookie fc;
	syncops *ss, *sprev, *snext;
	Entry *e = NULL;
	Attribute *a;
	int rc;
	struct berval newdn;
	int freefdn = 0;
	BackendDB *b0 = op->o_bd, db;

	fc.fdn = &op->o_req_ndn;
	/* compute new DN */
	if ( op->o_tag == LDAP_REQ_MODRDN && !saveit ) {
		struct berval pdn;
		if ( op->orr_nnewSup ) pdn = *op->orr_nnewSup;
		else dnParent( fc.fdn, &pdn );
		build_new_dn( &newdn, &pdn, &op->orr_nnewrdn, op->o_tmpmemctx );
		fc.fdn = &newdn;
		freefdn = 1;
	}
	if ( op->o_tag != LDAP_REQ_ADD ) {
		if ( !SLAP_ISOVERLAY( op->o_bd )) {
			db = *op->o_bd;
			op->o_bd = &db;
		}
		rc = overlay_entry_get_ov( op, fc.fdn, NULL, NULL, 0, &e, on );
		/* If we're sending responses now, make a copy and unlock the DB */
		if ( e && !saveit ) {
			if ( !opc->se ) {
				opc->se = entry_dup( e );
				opc->se->e_private = get_mutexint();
			}
			overlay_entry_release_ov( op, e, 0, on );
			e = opc->se;
		}
		if ( rc ) {
			op->o_bd = b0;
			return;
		}
	} else {
		e = op->ora_e;
		if ( !saveit ) {
			if ( !opc->se ) {
				opc->se = entry_dup( e );
				opc->se->e_private = get_mutexint();
			}
			e = opc->se;
		}
	}

	if ( saveit || op->o_tag == LDAP_REQ_ADD ) {
		ber_dupbv_x( &opc->sdn, &e->e_name, op->o_tmpmemctx );
		ber_dupbv_x( &opc->sndn, &e->e_nname, op->o_tmpmemctx );
		opc->sreference = is_entry_referral( e );
		a = attr_find( e->e_attrs, slap_schema.si_ad_entryUUID );
		if ( a )
			ber_dupbv_x( &opc->suuid, &a->a_nvals[0], op->o_tmpmemctx );
	} else if ( op->o_tag == LDAP_REQ_MODRDN && !saveit ) {
		op->o_tmpfree( opc->sndn.bv_val, op->o_tmpmemctx );
		op->o_tmpfree( opc->sdn.bv_val, op->o_tmpmemctx );
		ber_dupbv_x( &opc->sdn, &e->e_name, op->o_tmpmemctx );
		ber_dupbv_x( &opc->sndn, &e->e_nname, op->o_tmpmemctx );
	}

	ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
	for (ss = si->si_ops, sprev = (syncops *)&si->si_ops; ss;
		sprev = ss, ss=snext)
	{
		Operation op2;
		Opheader oh;
		syncmatches *sm;
		int found = 0;

		snext = ss->s_next;
		if ( ss->s_op->o_abandon )
			continue;

		/* Don't send ops back to the originator */
		if ( opc->osid > 0 && opc->osid == ss->s_sid ) {
			Debug( LDAP_DEBUG_SYNC, "syncprov_matchops: skipping original sid %03x\n",
				opc->osid, 0, 0 );
			continue;
		}

		/* Don't send ops back to the messenger */
		if ( opc->rsid > 0 && opc->rsid == ss->s_sid ) {
			Debug( LDAP_DEBUG_SYNC, "syncprov_matchops: skipping relayed sid %03x\n",
				opc->rsid, 0, 0 );
			continue;
		}

		/* validate base */
		fc.fss = ss;
		fc.fbase = 0;
		fc.fscope = 0;

		/* If the base of the search is missing, signal a refresh */
		rc = syncprov_findbase( op, &fc );
		if ( rc != LDAP_SUCCESS ) {
			SlapReply rs = {REP_RESULT};
			send_ldap_error( ss->s_op, &rs, LDAP_SYNC_REFRESH_REQUIRED,
				"search base has changed" );
			sprev->s_next = snext;
			syncprov_drop_psearch( ss, 1 );
			ss = sprev;
			continue;
		}

		/* If we're sending results now, look for this op in old matches */
		if ( !saveit ) {
			syncmatches *old;

			/* Did we modify the search base? */
			if ( dn_match( &op->o_req_ndn, &ss->s_base )) {
				ldap_pvt_thread_mutex_lock( &ss->s_mutex );
				ss->s_flags |= PS_WROTE_BASE;
				ldap_pvt_thread_mutex_unlock( &ss->s_mutex );
			}

			for ( sm=opc->smatches, old=(syncmatches *)&opc->smatches; sm;
				old=sm, sm=sm->sm_next ) {
				if ( sm->sm_op == ss ) {
					found = 1;
					old->sm_next = sm->sm_next;
					op->o_tmpfree( sm, op->o_tmpmemctx );
					break;
				}
			}
		}

		if ( fc.fscope ) {
			ldap_pvt_thread_mutex_lock( &ss->s_mutex );
			op2 = *ss->s_op;
			oh = *op->o_hdr;
			oh.oh_conn = ss->s_op->o_conn;
			oh.oh_connid = ss->s_op->o_connid;
			op2.o_bd = op->o_bd->bd_self;
			op2.o_hdr = &oh;
			op2.o_extra = op->o_extra;
			op2.o_callback = NULL;
			if (ss->s_flags & PS_FIX_FILTER) {
				/* Skip the AND/GE clause that we stuck on in front. We
				   would lose deletes/mods that happen during the refresh
				   phase otherwise (ITS#6555) */
				op2.ors_filter = ss->s_op->ors_filter->f_and->f_next;
			}
			ldap_pvt_thread_mutex_unlock( &ss->s_mutex );
			rc = test_filter( &op2, e, op2.ors_filter );
		}

		Debug( LDAP_DEBUG_TRACE, "syncprov_matchops: sid %03x fscope %d rc %d\n",
			ss->s_sid, fc.fscope, rc );

		/* check if current o_req_dn is in scope and matches filter */
		if ( fc.fscope && rc == LDAP_COMPARE_TRUE ) {
			if ( saveit ) {
				sm = op->o_tmpalloc( sizeof(syncmatches), op->o_tmpmemctx );
				sm->sm_next = opc->smatches;
				sm->sm_op = ss;
				ldap_pvt_thread_mutex_lock( &ss->s_mutex );
				++ss->s_inuse;
				ldap_pvt_thread_mutex_unlock( &ss->s_mutex );
				opc->smatches = sm;
			} else {
				/* if found send UPDATE else send ADD */
				syncprov_qresp( opc, ss,
					found ? LDAP_SYNC_MODIFY : LDAP_SYNC_ADD );
			}
		} else if ( !saveit && found ) {
			/* send DELETE */
			syncprov_qresp( opc, ss, LDAP_SYNC_DELETE );
		} else if ( !saveit ) {
			syncprov_qresp( opc, ss, LDAP_SYNC_NEW_COOKIE );
		}
		if ( !saveit && found ) {
			/* Decrement s_inuse, was incremented when called
			 * with saveit == TRUE
			 */
			syncprov_free_syncop( ss );
		}
	}
	ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );

	if ( op->o_tag != LDAP_REQ_ADD && e ) {
		if ( !SLAP_ISOVERLAY( op->o_bd )) {
			op->o_bd = &db;
		}
		if ( saveit )
			overlay_entry_release_ov( op, e, 0, on );
		op->o_bd = b0;
	}
	if ( opc->se && !saveit ) {
		if ( !dec_mutexint( opc->se->e_private )) {
			opc->se->e_private = NULL;
			entry_free( opc->se );
			opc->se = NULL;
		}
	}
	if ( freefdn ) {
		op->o_tmpfree( fc.fdn->bv_val, op->o_tmpmemctx );
	}
	op->o_bd = b0;
}

static int
syncprov_op_cleanup( Operation *op, SlapReply *rs )
{
	slap_callback *cb = op->o_callback;
	opcookie *opc = cb->sc_private;
	slap_overinst *on = opc->son;
	syncprov_info_t		*si = on->on_bi.bi_private;
	syncmatches *sm, *snext;
	modtarget *mt;

	ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
	if ( si->si_active )
		si->si_active--;
	ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );

	for (sm = opc->smatches; sm; sm=snext) {
		snext = sm->sm_next;
		syncprov_free_syncop( sm->sm_op );
		op->o_tmpfree( sm, op->o_tmpmemctx );
	}

	/* Remove op from lock table */
	mt = opc->smt;
	if ( mt ) {
		ldap_pvt_thread_mutex_lock( &mt->mt_mutex );
		mt->mt_mods = mt->mt_mods->mi_next;
		/* If there are more, promote the next one */
		if ( mt->mt_mods ) {
			ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
		} else {
			ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
			ldap_pvt_thread_mutex_lock( &si->si_mods_mutex );
			avl_delete( &si->si_mods, mt, sp_avl_cmp );
			ldap_pvt_thread_mutex_unlock( &si->si_mods_mutex );
			ldap_pvt_thread_mutex_destroy( &mt->mt_mutex );
			ch_free( mt->mt_dn.bv_val );
			ch_free( mt );
		}
	}
	if ( !BER_BVISNULL( &opc->suuid ))
		op->o_tmpfree( opc->suuid.bv_val, op->o_tmpmemctx );
	if ( !BER_BVISNULL( &opc->sndn ))
		op->o_tmpfree( opc->sndn.bv_val, op->o_tmpmemctx );
	if ( !BER_BVISNULL( &opc->sdn ))
		op->o_tmpfree( opc->sdn.bv_val, op->o_tmpmemctx );
	op->o_callback = cb->sc_next;
	op->o_tmpfree(cb, op->o_tmpmemctx);

	return 0;
}

static void
syncprov_checkpoint( Operation *op, slap_overinst *on )
{
	syncprov_info_t *si = (syncprov_info_t *)on->on_bi.bi_private;
	Modifications mod;
	Operation opm;
	SlapReply rsm = {REP_RESULT};
	slap_callback cb = {0};
	BackendDB be;
	BackendInfo *bi;

#ifdef CHECK_CSN
	Syntax *syn = slap_schema.si_ad_contextCSN->ad_type->sat_syntax;

	int i;
	for ( i=0; i<si->si_numcsns; i++ ) {
		assert( !syn->ssyn_validate( syn, si->si_ctxcsn+i ));
	}
#endif
	mod.sml_numvals = si->si_numcsns;
	mod.sml_values = si->si_ctxcsn;
	mod.sml_nvalues = NULL;
	mod.sml_desc = slap_schema.si_ad_contextCSN;
	mod.sml_op = LDAP_MOD_REPLACE;
	mod.sml_flags = SLAP_MOD_INTERNAL;
	mod.sml_next = NULL;

	cb.sc_response = slap_null_cb;
	opm = *op;
	opm.o_tag = LDAP_REQ_MODIFY;
	opm.o_callback = &cb;
	opm.orm_modlist = &mod;
	opm.orm_no_opattrs = 1;
	if ( SLAP_GLUE_SUBORDINATE( op->o_bd )) {
		be = *on->on_info->oi_origdb;
		opm.o_bd = &be;
	}
	opm.o_req_dn = si->si_contextdn;
	opm.o_req_ndn = si->si_contextdn;
	bi = opm.o_bd->bd_info;
	opm.o_bd->bd_info = on->on_info->oi_orig;
	opm.o_managedsait = SLAP_CONTROL_NONCRITICAL;
	opm.o_no_schema_check = 1;
	opm.o_bd->be_modify( &opm, &rsm );

	if ( rsm.sr_err == LDAP_NO_SUCH_OBJECT &&
		SLAP_SYNC_SUBENTRY( opm.o_bd )) {
		const char	*text;
		char txtbuf[SLAP_TEXT_BUFLEN];
		size_t textlen = sizeof txtbuf;
		Entry *e = slap_create_context_csn_entry( opm.o_bd, NULL );
		rs_reinit( &rsm, REP_RESULT );
		slap_mods2entry( &mod, &e, 0, 1, &text, txtbuf, textlen);
		opm.ora_e = e;
		opm.o_bd->be_add( &opm, &rsm );
		if ( e == opm.ora_e )
			be_entry_release_w( &opm, opm.ora_e );
	}
	opm.o_bd->bd_info = bi;

	if ( mod.sml_next != NULL ) {
		slap_mods_free( mod.sml_next, 1 );
	}
#ifdef CHECK_CSN
	for ( i=0; i<si->si_numcsns; i++ ) {
		assert( !syn->ssyn_validate( syn, si->si_ctxcsn+i ));
	}
#endif
}

static void
syncprov_add_slog( Operation *op )
{
	opcookie *opc = op->o_callback->sc_private;
	slap_overinst *on = opc->son;
	syncprov_info_t		*si = on->on_bi.bi_private;
	sessionlog *sl;
	slog_entry *se;

	sl = si->si_logs;
	{
		if ( BER_BVISEMPTY( &op->o_csn ) ) {
			/* During the syncrepl refresh phase we can receive operations
			 * without a csn.  We cannot reliably determine the consumers
			 * state with respect to such operations, so we ignore them and
			 * wipe out anything in the log if we see them.
			 */
			ldap_pvt_thread_mutex_lock( &sl->sl_mutex );
			while ( se = sl->sl_head ) {
				sl->sl_head = se->se_next;
				ch_free( se );
			}
			sl->sl_tail = NULL;
			sl->sl_num = 0;
			ldap_pvt_thread_mutex_unlock( &sl->sl_mutex );
			return;
		}

		/* Allocate a record. UUIDs are not NUL-terminated. */
		se = ch_malloc( sizeof( slog_entry ) + opc->suuid.bv_len + 
			op->o_csn.bv_len + 1 );
		se->se_next = NULL;
		se->se_tag = op->o_tag;

		se->se_uuid.bv_val = (char *)(&se[1]);
		AC_MEMCPY( se->se_uuid.bv_val, opc->suuid.bv_val, opc->suuid.bv_len );
		se->se_uuid.bv_len = opc->suuid.bv_len;

		se->se_csn.bv_val = se->se_uuid.bv_val + opc->suuid.bv_len;
		AC_MEMCPY( se->se_csn.bv_val, op->o_csn.bv_val, op->o_csn.bv_len );
		se->se_csn.bv_val[op->o_csn.bv_len] = '\0';
		se->se_csn.bv_len = op->o_csn.bv_len;
		se->se_sid = slap_parse_csn_sid( &se->se_csn );

		ldap_pvt_thread_mutex_lock( &sl->sl_mutex );
		if ( sl->sl_head ) {
			/* Keep the list in csn order. */
			if ( ber_bvcmp( &sl->sl_tail->se_csn, &se->se_csn ) <= 0 ) {
				sl->sl_tail->se_next = se;
				sl->sl_tail = se;
			} else {
				slog_entry **sep;

				for ( sep = &sl->sl_head; *sep; sep = &(*sep)->se_next ) {
					if ( ber_bvcmp( &se->se_csn, &(*sep)->se_csn ) < 0 ) {
						se->se_next = *sep;
						*sep = se;
						break;
					}
				}
			}
		} else {
			sl->sl_head = se;
			sl->sl_tail = se;
			if ( !sl->sl_mincsn ) {
				sl->sl_numcsns = 1;
				sl->sl_mincsn = ch_malloc( 2*sizeof( struct berval ));
				sl->sl_sids = ch_malloc( sizeof( int ));
				sl->sl_sids[0] = se->se_sid;
				ber_dupbv( sl->sl_mincsn, &se->se_csn );
				BER_BVZERO( &sl->sl_mincsn[1] );
			}
		}
		sl->sl_num++;
		while ( sl->sl_num > sl->sl_size ) {
			int i, j;
			se = sl->sl_head;
			sl->sl_head = se->se_next;
			for ( i=0; i<sl->sl_numcsns; i++ )
				if ( sl->sl_sids[i] >= se->se_sid )
					break;
			if  ( i == sl->sl_numcsns || sl->sl_sids[i] != se->se_sid ) {
				slap_insert_csn_sids( (struct sync_cookie *)sl,
					i, se->se_sid, &se->se_csn );
			} else {
				ber_bvreplace( &sl->sl_mincsn[i], &se->se_csn );
			}
			ch_free( se );
			sl->sl_num--;
		}
		ldap_pvt_thread_mutex_unlock( &sl->sl_mutex );
	}
}

/* Just set a flag if we found the matching entry */
static int
playlog_cb( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		op->o_callback->sc_private = (void *)1;
	}
	return rs->sr_err;
}

/* enter with sl->sl_mutex locked, release before returning */
static void
syncprov_playlog( Operation *op, SlapReply *rs, sessionlog *sl,
	sync_control *srs, BerVarray ctxcsn, int numcsns, int *sids )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	slog_entry *se;
	int i, j, ndel, num, nmods, mmods;
	char cbuf[LDAP_PVT_CSNSTR_BUFSIZE];
	BerVarray uuids;
	struct berval delcsn[2];

	if ( !sl->sl_num ) {
		ldap_pvt_thread_mutex_unlock( &sl->sl_mutex );
		return;
	}

	num = sl->sl_num;
	i = 0;
	nmods = 0;

	uuids = op->o_tmpalloc( (num+1) * sizeof( struct berval ) +
		num * UUID_LEN, op->o_tmpmemctx );
	uuids[0].bv_val = (char *)(uuids + num + 1);

	delcsn[0].bv_len = 0;
	delcsn[0].bv_val = cbuf;
	BER_BVZERO(&delcsn[1]);

	/* Make a copy of the relevant UUIDs. Put the Deletes up front
	 * and everything else at the end. Do this first so we can
	 * unlock the list mutex.
	 */
	Debug( LDAP_DEBUG_SYNC, "srs csn %s\n",
		srs->sr_state.ctxcsn[0].bv_val, 0, 0 );
	for ( se=sl->sl_head; se; se=se->se_next ) {
		int k;
		Debug( LDAP_DEBUG_SYNC, "log csn %s\n", se->se_csn.bv_val, 0, 0 );
		ndel = 1;
		for ( k=0; k<srs->sr_state.numcsns; k++ ) {
			if ( se->se_sid == srs->sr_state.sids[k] ) {
				ndel = ber_bvcmp( &se->se_csn, &srs->sr_state.ctxcsn[k] );
				break;
			}
		}
		if ( ndel <= 0 ) {
			Debug( LDAP_DEBUG_SYNC, "cmp %d, too old\n", ndel, 0, 0 );
			continue;
		}
		ndel = 0;
		for ( k=0; k<numcsns; k++ ) {
			if ( se->se_sid == sids[k] ) {
				ndel = ber_bvcmp( &se->se_csn, &ctxcsn[k] );
				break;
			}
		}
		if ( ndel > 0 ) {
			Debug( LDAP_DEBUG_SYNC, "cmp %d, too new\n", ndel, 0, 0 );
			break;
		}
		if ( se->se_tag == LDAP_REQ_DELETE ) {
			j = i;
			i++;
			AC_MEMCPY( cbuf, se->se_csn.bv_val, se->se_csn.bv_len );
			delcsn[0].bv_len = se->se_csn.bv_len;
			delcsn[0].bv_val[delcsn[0].bv_len] = '\0';
		} else {
			if ( se->se_tag == LDAP_REQ_ADD )
				continue;
			nmods++;
			j = num - nmods;
		}
		uuids[j].bv_val = uuids[0].bv_val + (j * UUID_LEN);
		AC_MEMCPY(uuids[j].bv_val, se->se_uuid.bv_val, UUID_LEN);
		uuids[j].bv_len = UUID_LEN;
	}
	ldap_pvt_thread_mutex_unlock( &sl->sl_mutex );

	ndel = i;

	/* Zero out unused slots */
	for ( i=ndel; i < num - nmods; i++ )
		uuids[i].bv_len = 0;

	/* Mods must be validated to see if they belong in this delete set.
	 */

	mmods = nmods;
	/* Strip any duplicates */
	for ( i=0; i<nmods; i++ ) {
		for ( j=0; j<ndel; j++ ) {
			if ( bvmatch( &uuids[j], &uuids[num - 1 - i] )) {
				uuids[num - 1 - i].bv_len = 0;
				mmods --;
				break;
			}
		}
		if ( uuids[num - 1 - i].bv_len == 0 ) continue;
		for ( j=0; j<i; j++ ) {
			if ( bvmatch( &uuids[num - 1 - j], &uuids[num - 1 - i] )) {
				uuids[num - 1 - i].bv_len = 0;
				mmods --;
				break;
			}
		}
	}

	if ( mmods ) {
		Operation fop;
		int rc;
		Filter mf, af;
		AttributeAssertion eq = ATTRIBUTEASSERTION_INIT;
		slap_callback cb = {0};

		fop = *op;

		fop.o_sync_mode = 0;
		fop.o_callback = &cb;
		fop.ors_limit = NULL;
		fop.ors_tlimit = SLAP_NO_LIMIT;
		fop.ors_attrs = slap_anlist_all_attributes;
		fop.ors_attrsonly = 0;
		fop.o_managedsait = SLAP_CONTROL_CRITICAL;

		af.f_choice = LDAP_FILTER_AND;
		af.f_next = NULL;
		af.f_and = &mf;
		mf.f_choice = LDAP_FILTER_EQUALITY;
		mf.f_ava = &eq;
		mf.f_av_desc = slap_schema.si_ad_entryUUID;
		mf.f_next = fop.ors_filter;

		fop.ors_filter = &af;

		cb.sc_response = playlog_cb;
		fop.o_bd->bd_info = (BackendInfo *)on->on_info;

		for ( i=ndel; i<num; i++ ) {
		  if ( uuids[i].bv_len != 0 ) {
			SlapReply frs = { REP_RESULT };

			mf.f_av_value = uuids[i];
			cb.sc_private = NULL;
			fop.ors_slimit = 1;
			rc = fop.o_bd->be_search( &fop, &frs );

			/* If entry was not found, add to delete list */
			if ( !cb.sc_private ) {
				uuids[ndel++] = uuids[i];
			}
		  }
		}
		fop.o_bd->bd_info = (BackendInfo *)on;
	}
	if ( ndel ) {
		struct berval cookie;

		if ( delcsn[0].bv_len ) {
			slap_compose_sync_cookie( op, &cookie, delcsn, srs->sr_state.rid,
				slap_serverID ? slap_serverID : -1 );

			Debug( LDAP_DEBUG_SYNC, "syncprov_playlog: cookie=%s\n", cookie.bv_val, 0, 0 );
		}

		uuids[ndel].bv_val = NULL;
		syncprov_sendinfo( op, rs, LDAP_TAG_SYNC_ID_SET,
			delcsn[0].bv_len ? &cookie : NULL, 0, uuids, 1 );
		if ( delcsn[0].bv_len ) {
			op->o_tmpfree( cookie.bv_val, op->o_tmpmemctx );
		}
	}
	op->o_tmpfree( uuids, op->o_tmpmemctx );
}

static int
syncprov_op_response( Operation *op, SlapReply *rs )
{
	opcookie *opc = op->o_callback->sc_private;
	slap_overinst *on = opc->son;
	syncprov_info_t		*si = on->on_bi.bi_private;
	syncmatches *sm;

	if ( rs->sr_err == LDAP_SUCCESS )
	{
		struct berval maxcsn;
		char cbuf[LDAP_PVT_CSNSTR_BUFSIZE];
		int do_check = 0, have_psearches, foundit, csn_changed = 0;

		ldap_pvt_thread_mutex_lock( &si->si_resp_mutex );

		/* Update our context CSN */
		cbuf[0] = '\0';
		maxcsn.bv_val = cbuf;
		maxcsn.bv_len = sizeof(cbuf);
		ldap_pvt_thread_rdwr_wlock( &si->si_csn_rwlock );

		slap_get_commit_csn( op, &maxcsn, &foundit );
		if ( BER_BVISEMPTY( &maxcsn ) && SLAP_GLUE_SUBORDINATE( op->o_bd )) {
			/* syncrepl queues the CSN values in the db where
			 * it is configured , not where the changes are made.
			 * So look for a value in the glue db if we didn't
			 * find any in this db.
			 */
			BackendDB *be = op->o_bd;
			op->o_bd = select_backend( &be->be_nsuffix[0], 1);
			maxcsn.bv_val = cbuf;
			maxcsn.bv_len = sizeof(cbuf);
			slap_get_commit_csn( op, &maxcsn, &foundit );
			op->o_bd = be;
		}
		if ( !BER_BVISEMPTY( &maxcsn ) ) {
			int i, sid;
#ifdef CHECK_CSN
			Syntax *syn = slap_schema.si_ad_contextCSN->ad_type->sat_syntax;
			assert( !syn->ssyn_validate( syn, &maxcsn ));
#endif
			sid = slap_parse_csn_sid( &maxcsn );
			for ( i=0; i<si->si_numcsns; i++ ) {
				if ( sid < si->si_sids[i] )
					break;
				if ( sid == si->si_sids[i] ) {
					if ( ber_bvcmp( &maxcsn, &si->si_ctxcsn[i] ) > 0 ) {
						ber_bvreplace( &si->si_ctxcsn[i], &maxcsn );
						csn_changed = 1;
					}
					break;
				}
			}
			/* It's a new SID for us */
			if ( i == si->si_numcsns || sid != si->si_sids[i] ) {
				slap_insert_csn_sids((struct sync_cookie *)&(si->si_ctxcsn),
					i, sid, &maxcsn );
				csn_changed = 1;
			}
		}

		/* Don't do any processing for consumer contextCSN updates */
		if ( op->o_dont_replicate ) {
			if ( op->o_tag == LDAP_REQ_MODIFY &&
				op->orm_modlist->sml_op == LDAP_MOD_REPLACE &&
				op->orm_modlist->sml_desc == slap_schema.si_ad_contextCSN ) {
			/* Catch contextCSN updates from syncrepl. We have to look at
			 * all the attribute values, as there may be more than one csn
			 * that changed, and only one can be passed in the csn queue.
			 */
			Modifications *mod = op->orm_modlist;
			unsigned i;
			int j, sid;

			for ( i=0; i<mod->sml_numvals; i++ ) {
				sid = slap_parse_csn_sid( &mod->sml_values[i] );
				for ( j=0; j<si->si_numcsns; j++ ) {
					if ( sid < si->si_sids[j] )
						break;
					if ( sid == si->si_sids[j] ) {
						if ( ber_bvcmp( &mod->sml_values[i], &si->si_ctxcsn[j] ) > 0 ) {
							ber_bvreplace( &si->si_ctxcsn[j], &mod->sml_values[i] );
							csn_changed = 1;
						}
						break;
					}
				}

				if ( j == si->si_numcsns || sid != si->si_sids[j] ) {
					slap_insert_csn_sids( (struct sync_cookie *)&si->si_ctxcsn,
						j, sid, &mod->sml_values[i] );
					csn_changed = 1;
				}
			}
			if ( csn_changed )
				si->si_dirty = 0;
			ldap_pvt_thread_rdwr_wunlock( &si->si_csn_rwlock );

			if ( csn_changed ) {
				syncops *ss;
				ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
				for ( ss = si->si_ops; ss; ss = ss->s_next ) {
					if ( ss->s_op->o_abandon )
						continue;
					/* Send the updated csn to all syncrepl consumers,
					 * including the server from which it originated.
					 * The syncrepl consumer and syncprov provider on
					 * the originating server may be configured to store
					 * their csn values in different entries.
					 */
					syncprov_qresp( opc, ss, LDAP_SYNC_NEW_COOKIE );
				}
				ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
			}
			} else {
			ldap_pvt_thread_rdwr_wunlock( &si->si_csn_rwlock );
			}
			goto leave;
		}

		si->si_numops++;
		if ( si->si_chkops || si->si_chktime ) {
			/* Never checkpoint adding the context entry,
			 * it will deadlock
			 */
			if ( op->o_tag != LDAP_REQ_ADD ||
				!dn_match( &op->o_req_ndn, &si->si_contextdn )) {
				if ( si->si_chkops && si->si_numops >= si->si_chkops ) {
					do_check = 1;
					si->si_numops = 0;
				}
				if ( si->si_chktime &&
					(op->o_time - si->si_chklast >= si->si_chktime )) {
					if ( si->si_chklast ) {
						do_check = 1;
						si->si_chklast = op->o_time;
					} else {
						si->si_chklast = 1;
					}
				}
			}
		}
		si->si_dirty = !csn_changed;
		ldap_pvt_thread_rdwr_wunlock( &si->si_csn_rwlock );

		if ( do_check ) {
			ldap_pvt_thread_rdwr_rlock( &si->si_csn_rwlock );
			syncprov_checkpoint( op, on );
			ldap_pvt_thread_rdwr_runlock( &si->si_csn_rwlock );
		}

		/* only update consumer ctx if this is a newer csn */
		if ( csn_changed ) {
			opc->sctxcsn = maxcsn;
		}

		/* Handle any persistent searches */
		ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
		have_psearches = ( si->si_ops != NULL );
		ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
		if ( have_psearches ) {
			switch(op->o_tag) {
			case LDAP_REQ_ADD:
			case LDAP_REQ_MODIFY:
			case LDAP_REQ_MODRDN:
			case LDAP_REQ_EXTENDED:
				syncprov_matchops( op, opc, 0 );
				break;
			case LDAP_REQ_DELETE:
				/* for each match in opc->smatches:
				 *   send DELETE msg
				 */
				for ( sm = opc->smatches; sm; sm=sm->sm_next ) {
					if ( sm->sm_op->s_op->o_abandon )
						continue;
					syncprov_qresp( opc, sm->sm_op, LDAP_SYNC_DELETE );
				}
				break;
			}
		}

		/* Add any log records */
		if ( si->si_logs ) {
			syncprov_add_slog( op );
		}
leave:		ldap_pvt_thread_mutex_unlock( &si->si_resp_mutex );
	}
	return SLAP_CB_CONTINUE;
}

/* We don't use a subentry to store the context CSN any more.
 * We expose the current context CSN as an operational attribute
 * of the suffix entry.
 */
static int
syncprov_op_compare( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t		*si = on->on_bi.bi_private;
	int rc = SLAP_CB_CONTINUE;

	if ( dn_match( &op->o_req_ndn, &si->si_contextdn ) &&
		op->oq_compare.rs_ava->aa_desc == slap_schema.si_ad_contextCSN )
	{
		Entry e = {0};
		Attribute a = {0};

		e.e_name = si->si_contextdn;
		e.e_nname = si->si_contextdn;
		e.e_attrs = &a;

		a.a_desc = slap_schema.si_ad_contextCSN;

		ldap_pvt_thread_rdwr_rlock( &si->si_csn_rwlock );

		a.a_vals = si->si_ctxcsn;
		a.a_nvals = a.a_vals;
		a.a_numvals = si->si_numcsns;

		rs->sr_err = access_allowed( op, &e, op->oq_compare.rs_ava->aa_desc,
			&op->oq_compare.rs_ava->aa_value, ACL_COMPARE, NULL );
		if ( ! rs->sr_err ) {
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
			goto return_results;
		}

		if ( get_assert( op ) &&
			( test_filter( op, &e, get_assertion( op ) ) != LDAP_COMPARE_TRUE ) )
		{
			rs->sr_err = LDAP_ASSERTION_FAILED;
			goto return_results;
		}


		rs->sr_err = LDAP_COMPARE_FALSE;

		if ( attr_valfind( &a,
			SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
				SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
				&op->oq_compare.rs_ava->aa_value, NULL, op->o_tmpmemctx ) == 0 )
		{
			rs->sr_err = LDAP_COMPARE_TRUE;
		}

return_results:;

		ldap_pvt_thread_rdwr_runlock( &si->si_csn_rwlock );

		send_ldap_result( op, rs );

		if( rs->sr_err == LDAP_COMPARE_FALSE || rs->sr_err == LDAP_COMPARE_TRUE ) {
			rs->sr_err = LDAP_SUCCESS;
		}
		rc = rs->sr_err;
	}

	return rc;
}

static int
syncprov_op_mod( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t		*si = on->on_bi.bi_private;
	slap_callback *cb;
	opcookie *opc;
	int have_psearches, cbsize;

	ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
	have_psearches = ( si->si_ops != NULL );
	si->si_active++;
	ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );

	cbsize = sizeof(slap_callback) + sizeof(opcookie) +
		(have_psearches ? sizeof(modinst) : 0 );

	cb = op->o_tmpcalloc(1, cbsize, op->o_tmpmemctx);
	opc = (opcookie *)(cb+1);
	opc->son = on;
	cb->sc_response = syncprov_op_response;
	cb->sc_cleanup = syncprov_op_cleanup;
	cb->sc_private = opc;
	cb->sc_next = op->o_callback;
	op->o_callback = cb;

	opc->osid = -1;
	opc->rsid = -1;
	if ( op->o_csn.bv_val ) {
		opc->osid = slap_parse_csn_sid( &op->o_csn );
	}
	if ( op->o_controls ) {
		struct sync_cookie *scook =
		op->o_controls[slap_cids.sc_LDAPsync];
		if ( scook )
			opc->rsid = scook->sid;
	}

	if ( op->o_dont_replicate )
		return SLAP_CB_CONTINUE;

	/* If there are active persistent searches, lock this operation.
	 * See seqmod.c for the locking logic on its own.
	 */
	if ( have_psearches ) {
		modtarget *mt, mtdummy;
		modinst *mi;

		mi = (modinst *)(opc+1);
		mi->mi_op = op;

		/* See if we're already modifying this entry... */
		mtdummy.mt_dn = op->o_req_ndn;
		ldap_pvt_thread_mutex_lock( &si->si_mods_mutex );
		mt = avl_find( si->si_mods, &mtdummy, sp_avl_cmp );
		if ( mt ) {
			ldap_pvt_thread_mutex_lock( &mt->mt_mutex );
			if ( mt->mt_mods == NULL ) {
				/* Cannot reuse this mt, as another thread is about
				 * to release it in syncprov_op_cleanup.
				 */
				ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
				mt = NULL;
			}
		}
		if ( mt ) {
			ldap_pvt_thread_mutex_unlock( &si->si_mods_mutex );
			mt->mt_tail->mi_next = mi;
			mt->mt_tail = mi;
			/* wait for this op to get to head of list */
			while ( mt->mt_mods != mi ) {
				ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
				/* FIXME: if dynamic config can delete overlays or
				 * databases we'll have to check for cleanup here.
				 * Currently it's not an issue because there are
				 * no dynamic config deletes...
				 */
				if ( slapd_shutdown )
					return SLAPD_ABANDON;

				if ( !ldap_pvt_thread_pool_pausecheck( &connection_pool ))
					ldap_pvt_thread_yield();
				ldap_pvt_thread_mutex_lock( &mt->mt_mutex );

				/* clean up if the caller is giving up */
				if ( op->o_abandon ) {
					modinst *m2;
					for ( m2 = mt->mt_mods; m2 && m2->mi_next != mi;
						m2 = m2->mi_next );
					if ( m2 ) {
						m2->mi_next = mi->mi_next;
						if ( mt->mt_tail == mi ) mt->mt_tail = m2;
					}
					op->o_tmpfree( cb, op->o_tmpmemctx );
					ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
					return SLAPD_ABANDON;
				}
			}
			ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
		} else {
			/* Record that we're modifying this entry now */
			mt = ch_malloc( sizeof(modtarget) );
			mt->mt_mods = mi;
			mt->mt_tail = mi;
			ber_dupbv( &mt->mt_dn, &mi->mi_op->o_req_ndn );
			ldap_pvt_thread_mutex_init( &mt->mt_mutex );
			avl_insert( &si->si_mods, mt, sp_avl_cmp, avl_dup_error );
			ldap_pvt_thread_mutex_unlock( &si->si_mods_mutex );
		}
		opc->smt = mt;
	}

	if (( have_psearches || si->si_logs ) && op->o_tag != LDAP_REQ_ADD )
		syncprov_matchops( op, opc, 1 );

	return SLAP_CB_CONTINUE;
}

static int
syncprov_op_extended( Operation *op, SlapReply *rs )
{
	if ( exop_is_write( op ))
		return syncprov_op_mod( op, rs );

	return SLAP_CB_CONTINUE;
}

typedef struct searchstate {
	slap_overinst *ss_on;
	syncops *ss_so;
	BerVarray ss_ctxcsn;
	int *ss_sids;
	int ss_numcsns;
#define	SS_PRESENT	0x01
#define	SS_CHANGED	0x02
	int ss_flags;
} searchstate;

typedef struct SyncOperationBuffer {
	Operation		sob_op;
	Opheader		sob_hdr;
	OpExtra			sob_oe;
	AttributeName	sob_extra;	/* not always present */
	/* Further data allocated here */
} SyncOperationBuffer;

static void
syncprov_detach_op( Operation *op, syncops *so, slap_overinst *on )
{
	SyncOperationBuffer *sopbuf2;
	Operation *op2;
	int i, alen = 0;
	size_t size;
	char *ptr;
	GroupAssertion *g1, *g2;

	/* count the search attrs */
	for (i=0; op->ors_attrs && !BER_BVISNULL( &op->ors_attrs[i].an_name ); i++) {
		alen += op->ors_attrs[i].an_name.bv_len + 1;
	}
	/* Make a new copy of the operation */
	size = offsetof( SyncOperationBuffer, sob_extra ) +
		(i ? ( (i+1) * sizeof(AttributeName) + alen) : 0) +
		op->o_req_dn.bv_len + 1 +
		op->o_req_ndn.bv_len + 1 +
		op->o_ndn.bv_len + 1 +
		so->s_filterstr.bv_len + 1;
	sopbuf2 = ch_calloc( 1, size );
	op2 = &sopbuf2->sob_op;
	op2->o_hdr = &sopbuf2->sob_hdr;
	LDAP_SLIST_FIRST(&op2->o_extra) = &sopbuf2->sob_oe;

	/* Copy the fields we care about explicitly, leave the rest alone */
	*op2->o_hdr = *op->o_hdr;
	op2->o_tag = op->o_tag;
	op2->o_time = op->o_time;
	op2->o_bd = on->on_info->oi_origdb;
	op2->o_request = op->o_request;
	op2->o_managedsait = op->o_managedsait;
	LDAP_SLIST_FIRST(&op2->o_extra)->oe_key = on;
	LDAP_SLIST_NEXT(LDAP_SLIST_FIRST(&op2->o_extra), oe_next) = NULL;

	ptr = (char *) sopbuf2 + offsetof( SyncOperationBuffer, sob_extra );
	if ( i ) {
		op2->ors_attrs = (AttributeName *) ptr;
		ptr = (char *) &op2->ors_attrs[i+1];
		for (i=0; !BER_BVISNULL( &op->ors_attrs[i].an_name ); i++) {
			op2->ors_attrs[i] = op->ors_attrs[i];
			op2->ors_attrs[i].an_name.bv_val = ptr;
			ptr = lutil_strcopy( ptr, op->ors_attrs[i].an_name.bv_val ) + 1;
		}
		BER_BVZERO( &op2->ors_attrs[i].an_name );
	}

	op2->o_authz = op->o_authz;
	op2->o_ndn.bv_val = ptr;
	ptr = lutil_strcopy(ptr, op->o_ndn.bv_val) + 1;
	op2->o_dn = op2->o_ndn;
	op2->o_req_dn.bv_len = op->o_req_dn.bv_len;
	op2->o_req_dn.bv_val = ptr;
	ptr = lutil_strcopy(ptr, op->o_req_dn.bv_val) + 1;
	op2->o_req_ndn.bv_len = op->o_req_ndn.bv_len;
	op2->o_req_ndn.bv_val = ptr;
	ptr = lutil_strcopy(ptr, op->o_req_ndn.bv_val) + 1;
	op2->ors_filterstr.bv_val = ptr;
	strcpy( ptr, so->s_filterstr.bv_val );
	op2->ors_filterstr.bv_len = so->s_filterstr.bv_len;

	/* Skip the AND/GE clause that we stuck on in front */
	if ( so->s_flags & PS_FIX_FILTER ) {
		op2->ors_filter = op->ors_filter->f_and->f_next;
		so->s_flags ^= PS_FIX_FILTER;
	} else {
		op2->ors_filter = op->ors_filter;
	}
	op2->ors_filter = filter_dup( op2->ors_filter, NULL );
	so->s_op = op2;

	/* Copy any cached group ACLs individually */
	op2->o_groups = NULL;
	for ( g1=op->o_groups; g1; g1=g1->ga_next ) {
		g2 = ch_malloc( sizeof(GroupAssertion) + g1->ga_len );
		*g2 = *g1;
		strcpy( g2->ga_ndn, g1->ga_ndn );
		g2->ga_next = op2->o_groups;
		op2->o_groups = g2;
	}
	/* Don't allow any further group caching */
	op2->o_do_not_cache = 1;

	/* Add op2 to conn so abandon will find us */
	op->o_conn->c_n_ops_executing++;
	op->o_conn->c_n_ops_completed--;
	LDAP_STAILQ_INSERT_TAIL( &op->o_conn->c_ops, op2, o_next );
	so->s_flags |= PS_IS_DETACHED;

	/* Prevent anyone else from trying to send a result for this op */
	op->o_abandon = 1;
}

static int
syncprov_search_response( Operation *op, SlapReply *rs )
{
	searchstate *ss = op->o_callback->sc_private;
	slap_overinst *on = ss->ss_on;
	syncprov_info_t *si = (syncprov_info_t *)on->on_bi.bi_private;
	sync_control *srs = op->o_controls[slap_cids.sc_LDAPsync];

	if ( rs->sr_type == REP_SEARCH || rs->sr_type == REP_SEARCHREF ) {
		Attribute *a;
		/* If we got a referral without a referral object, there's
		 * something missing that we cannot replicate. Just ignore it.
		 * The consumer will abort because we didn't send the expected
		 * control.
		 */
		if ( !rs->sr_entry ) {
			assert( rs->sr_entry != NULL );
			Debug( LDAP_DEBUG_ANY, "bogus referral in context\n",0,0,0 );
			return SLAP_CB_CONTINUE;
		}
		a = attr_find( rs->sr_entry->e_attrs, slap_schema.si_ad_entryCSN );
		if ( a == NULL && rs->sr_operational_attrs != NULL ) {
			a = attr_find( rs->sr_operational_attrs, slap_schema.si_ad_entryCSN );
		}
		if ( a ) {
			int i, sid;
			sid = slap_parse_csn_sid( &a->a_nvals[0] );

			/* Don't send changed entries back to the originator */
			if ( sid == srs->sr_state.sid && srs->sr_state.numcsns ) {
				Debug( LDAP_DEBUG_SYNC,
					"Entry %s changed by peer, ignored\n",
					rs->sr_entry->e_name.bv_val, 0, 0 );
				return LDAP_SUCCESS;
			}

			/* If not a persistent search */
			if ( !ss->ss_so ) {
				/* Make sure entry is less than the snapshot'd contextCSN */
				for ( i=0; i<ss->ss_numcsns; i++ ) {
					if ( sid == ss->ss_sids[i] && ber_bvcmp( &a->a_nvals[0],
						&ss->ss_ctxcsn[i] ) > 0 ) {
						Debug( LDAP_DEBUG_SYNC,
							"Entry %s CSN %s greater than snapshot %s\n",
							rs->sr_entry->e_name.bv_val,
							a->a_nvals[0].bv_val,
							ss->ss_ctxcsn[i].bv_val );
						return LDAP_SUCCESS;
					}
				}
			}

			/* Don't send old entries twice */
			if ( srs->sr_state.ctxcsn ) {
				for ( i=0; i<srs->sr_state.numcsns; i++ ) {
					if ( sid == srs->sr_state.sids[i] &&
						ber_bvcmp( &a->a_nvals[0],
							&srs->sr_state.ctxcsn[i] )<= 0 ) {
						Debug( LDAP_DEBUG_SYNC,
							"Entry %s CSN %s older or equal to ctx %s\n",
							rs->sr_entry->e_name.bv_val,
							a->a_nvals[0].bv_val,
							srs->sr_state.ctxcsn[i].bv_val );
						return LDAP_SUCCESS;
					}
				}
			}
		}
		rs->sr_ctrls = op->o_tmpalloc( sizeof(LDAPControl *)*2,
			op->o_tmpmemctx );
		rs->sr_ctrls[1] = NULL;
		rs->sr_flags |= REP_CTRLS_MUSTBEFREED;
		/* If we're in delta-sync mode, always send a cookie */
		if ( si->si_nopres && si->si_usehint && a ) {
			struct berval cookie;
			slap_compose_sync_cookie( op, &cookie, a->a_nvals, srs->sr_state.rid, slap_serverID ? slap_serverID : -1 );
			rs->sr_err = syncprov_state_ctrl( op, rs, rs->sr_entry,
				LDAP_SYNC_ADD, rs->sr_ctrls, 0, 1, &cookie );
			op->o_tmpfree( cookie.bv_val, op->o_tmpmemctx );
		} else {
			rs->sr_err = syncprov_state_ctrl( op, rs, rs->sr_entry,
				LDAP_SYNC_ADD, rs->sr_ctrls, 0, 0, NULL );
		}
	} else if ( rs->sr_type == REP_RESULT && rs->sr_err == LDAP_SUCCESS ) {
		struct berval cookie = BER_BVNULL;

		if ( ( ss->ss_flags & SS_CHANGED ) &&
			ss->ss_ctxcsn && !BER_BVISNULL( &ss->ss_ctxcsn[0] )) {
			slap_compose_sync_cookie( op, &cookie, ss->ss_ctxcsn,
				srs->sr_state.rid, slap_serverID ? slap_serverID : -1 );

			Debug( LDAP_DEBUG_SYNC, "syncprov_search_response: cookie=%s\n", cookie.bv_val, 0, 0 );
		}

		/* Is this a regular refresh?
		 * Note: refresh never gets here if there were no changes
		 */
		if ( !ss->ss_so ) {
			rs->sr_ctrls = op->o_tmpalloc( sizeof(LDAPControl *)*2,
				op->o_tmpmemctx );
			rs->sr_ctrls[1] = NULL;
			rs->sr_flags |= REP_CTRLS_MUSTBEFREED;
			rs->sr_err = syncprov_done_ctrl( op, rs, rs->sr_ctrls,
				0, 1, &cookie, ( ss->ss_flags & SS_PRESENT ) ?  LDAP_SYNC_REFRESH_PRESENTS :
					LDAP_SYNC_REFRESH_DELETES );
			op->o_tmpfree( cookie.bv_val, op->o_tmpmemctx );
		} else {
		/* It's RefreshAndPersist, transition to Persist phase */
			syncprov_sendinfo( op, rs, ( ss->ss_flags & SS_PRESENT ) ?
	 			LDAP_TAG_SYNC_REFRESH_PRESENT : LDAP_TAG_SYNC_REFRESH_DELETE,
				( ss->ss_flags & SS_CHANGED ) ? &cookie : NULL,
				1, NULL, 0 );
			if ( !BER_BVISNULL( &cookie ))
				op->o_tmpfree( cookie.bv_val, op->o_tmpmemctx );

			/* Detach this Op from frontend control */
			ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );

			/* But not if this connection was closed along the way */
			if ( op->o_abandon ) {
				ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );
				/* syncprov_ab_cleanup will free this syncop */
				return SLAPD_ABANDON;

			} else {
				ldap_pvt_thread_mutex_lock( &ss->ss_so->s_mutex );
				/* Turn off the refreshing flag */
				ss->ss_so->s_flags ^= PS_IS_REFRESHING;

				syncprov_detach_op( op, ss->ss_so, on );

				ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );

				/* If there are queued responses, fire them off */
				if ( ss->ss_so->s_res )
					syncprov_qstart( ss->ss_so );
				ldap_pvt_thread_mutex_unlock( &ss->ss_so->s_mutex );
			}

			return LDAP_SUCCESS;
		}
	}

	return SLAP_CB_CONTINUE;
}

static int
syncprov_op_search( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t		*si = (syncprov_info_t *)on->on_bi.bi_private;
	slap_callback	*cb;
	int gotstate = 0, changed = 0, do_present = 0;
	syncops *sop = NULL;
	searchstate *ss;
	sync_control *srs;
	BerVarray ctxcsn;
	int i, *sids, numcsns;
	struct berval mincsn, maxcsn;
	int minsid, maxsid;
	int dirty = 0;

	if ( !(op->o_sync_mode & SLAP_SYNC_REFRESH) ) return SLAP_CB_CONTINUE;

	if ( op->ors_deref & LDAP_DEREF_SEARCHING ) {
		send_ldap_error( op, rs, LDAP_PROTOCOL_ERROR, "illegal value for derefAliases" );
		return rs->sr_err;
	}

	srs = op->o_controls[slap_cids.sc_LDAPsync];

	/* If this is a persistent search, set it up right away */
	if ( op->o_sync_mode & SLAP_SYNC_PERSIST ) {
		syncops so = {0};
		fbase_cookie fc;
		opcookie opc;
		slap_callback sc;

		fc.fss = &so;
		fc.fbase = 0;
		so.s_eid = NOID;
		so.s_op = op;
		so.s_flags = PS_IS_REFRESHING | PS_FIND_BASE;
		/* syncprov_findbase expects to be called as a callback... */
		sc.sc_private = &opc;
		opc.son = on;
		ldap_pvt_thread_mutex_init( &so.s_mutex );
		cb = op->o_callback;
		op->o_callback = &sc;
		rs->sr_err = syncprov_findbase( op, &fc );
		op->o_callback = cb;
		ldap_pvt_thread_mutex_destroy( &so.s_mutex );

		if ( rs->sr_err != LDAP_SUCCESS ) {
			send_ldap_result( op, rs );
			return rs->sr_err;
		}
		sop = ch_malloc( sizeof( syncops ));
		*sop = so;
		ldap_pvt_thread_mutex_init( &sop->s_mutex );
		sop->s_rid = srs->sr_state.rid;
		sop->s_sid = srs->sr_state.sid;
		/* set refcount=2 to prevent being freed out from under us
		 * by abandons that occur while we're running here
		 */
		sop->s_inuse = 2;

		ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
		while ( si->si_active ) {
			/* Wait for active mods to finish before proceeding, as they
			 * may already have inspected the si_ops list looking for
			 * consumers to replicate the change to.  Using the log
			 * doesn't help, as we may finish playing it before the
			 * active mods gets added to it.
			 */
			ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
			if ( slapd_shutdown ) {
				ch_free( sop );
				return SLAPD_ABANDON;
			}
			if ( !ldap_pvt_thread_pool_pausecheck( &connection_pool ))
				ldap_pvt_thread_yield();
			ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
		}
		sop->s_next = si->si_ops;
		si->si_ops = sop;
		ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
	}

	/* snapshot the ctxcsn */
	ldap_pvt_thread_rdwr_rlock( &si->si_csn_rwlock );
	numcsns = si->si_numcsns;
	if ( numcsns ) {
		ber_bvarray_dup_x( &ctxcsn, si->si_ctxcsn, op->o_tmpmemctx );
		sids = op->o_tmpalloc( numcsns * sizeof(int), op->o_tmpmemctx );
		for ( i=0; i<numcsns; i++ )
			sids[i] = si->si_sids[i];
	} else {
		ctxcsn = NULL;
		sids = NULL;
	}
	dirty = si->si_dirty;
	ldap_pvt_thread_rdwr_runlock( &si->si_csn_rwlock );
	
	/* If we have a cookie, handle the PRESENT lookups */
	if ( srs->sr_state.ctxcsn ) {
		sessionlog *sl;
		int i, j;

		/* If we don't have any CSN of our own yet, pretend nothing
		 * has changed.
		 */
		if ( !numcsns )
			goto no_change;

		if ( !si->si_nopres )
			do_present = SS_PRESENT;

		/* If there are SIDs we don't recognize in the cookie, drop them */
		for (i=0; i<srs->sr_state.numcsns; ) {
			for (j=i; j<numcsns; j++) {
				if ( srs->sr_state.sids[i] <= sids[j] ) {
					break;
				}
			}
			/* not found */
			if ( j == numcsns || srs->sr_state.sids[i] != sids[j] ) {
				char *tmp = srs->sr_state.ctxcsn[i].bv_val;
				srs->sr_state.numcsns--;
				for ( j=i; j<srs->sr_state.numcsns; j++ ) {
					srs->sr_state.ctxcsn[j] = srs->sr_state.ctxcsn[j+1];
					srs->sr_state.sids[j] = srs->sr_state.sids[j+1];
				}
				srs->sr_state.ctxcsn[j].bv_val = tmp;
				srs->sr_state.ctxcsn[j].bv_len = 0;
				continue;
			}
			i++;
		}

		if (srs->sr_state.numcsns != numcsns) {
			/* consumer doesn't have the right number of CSNs */
			changed = SS_CHANGED;
			if ( srs->sr_state.ctxcsn ) {
				ber_bvarray_free_x( srs->sr_state.ctxcsn, op->o_tmpmemctx );
				srs->sr_state.ctxcsn = NULL;
			}
			if ( srs->sr_state.sids ) {
				slap_sl_free( srs->sr_state.sids, op->o_tmpmemctx );
				srs->sr_state.sids = NULL;
			}
			srs->sr_state.numcsns = 0;
			goto shortcut;
		}

		/* Find the smallest CSN which differs from contextCSN */
		mincsn.bv_len = 0;
		maxcsn.bv_len = 0;
		for ( i=0,j=0; i<srs->sr_state.numcsns; i++ ) {
			int newer;
			while ( srs->sr_state.sids[i] != sids[j] ) j++;
			if ( BER_BVISEMPTY( &maxcsn ) || ber_bvcmp( &maxcsn,
				&srs->sr_state.ctxcsn[i] ) < 0 ) {
				maxcsn = srs->sr_state.ctxcsn[i];
				maxsid = sids[j];
			}
			newer = ber_bvcmp( &srs->sr_state.ctxcsn[i], &ctxcsn[j] );
			/* If our state is newer, tell consumer about changes */
			if ( newer < 0) {
				changed = SS_CHANGED;
				if ( BER_BVISEMPTY( &mincsn ) || ber_bvcmp( &mincsn,
					&srs->sr_state.ctxcsn[i] ) > 0 ) {
					mincsn = srs->sr_state.ctxcsn[i];
					minsid = sids[j];
				}
			} else if ( newer > 0 && sids[j] == slap_serverID ) {
			/* our state is older, complain to consumer */
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				rs->sr_text = "consumer state is newer than provider!";
bailout:
				if ( sop ) {
					syncops **sp = &si->si_ops;

					ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
					while ( *sp != sop )
						sp = &(*sp)->s_next;
					*sp = sop->s_next;
					ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
					ch_free( sop );
				}
				rs->sr_ctrls = NULL;
				send_ldap_result( op, rs );
				return rs->sr_err;
			}
 		}
		if ( BER_BVISEMPTY( &mincsn )) {
			mincsn = maxcsn;
			minsid = maxsid;
		}

		/* If nothing has changed, shortcut it */
		if ( !changed && !dirty ) {
			do_present = 0;
no_change:		if ( !(op->o_sync_mode & SLAP_SYNC_PERSIST) ) {
				LDAPControl	*ctrls[2];

				ctrls[0] = NULL;
				ctrls[1] = NULL;
				syncprov_done_ctrl( op, rs, ctrls, 0, 0,
					NULL, LDAP_SYNC_REFRESH_DELETES );
				rs->sr_ctrls = ctrls;
				rs->sr_err = LDAP_SUCCESS;
				send_ldap_result( op, rs );
				rs->sr_ctrls = NULL;
				return rs->sr_err;
			}
			goto shortcut;
		}

		/* Do we have a sessionlog for this search? */
		sl=si->si_logs;
		if ( sl ) {
			int do_play = 0;
			ldap_pvt_thread_mutex_lock( &sl->sl_mutex );
			/* Are there any log entries, and is the consumer state
			 * present in the session log?
			 */
			if ( sl->sl_num > 0 ) {
				int i;
				for ( i=0; i<sl->sl_numcsns; i++ ) {
					/* SID not present == new enough */
					if ( minsid < sl->sl_sids[i] ) {
						do_play = 1;
						break;
					}
					/* SID present */
					if ( minsid == sl->sl_sids[i] ) {
						/* new enough? */
						if ( ber_bvcmp( &mincsn, &sl->sl_mincsn[i] ) >= 0 )
							do_play = 1;
						break;
					}
				}
				/* SID not present == new enough */
				if ( i == sl->sl_numcsns )
					do_play = 1;
			}
			if ( do_play ) {
				do_present = 0;
				/* mutex is unlocked in playlog */
				syncprov_playlog( op, rs, sl, srs, ctxcsn, numcsns, sids );
			} else {
				ldap_pvt_thread_mutex_unlock( &sl->sl_mutex );
			}
		}
		/* Is the CSN still present in the database? */
		if ( syncprov_findcsn( op, FIND_CSN, &mincsn ) != LDAP_SUCCESS ) {
			/* No, so a reload is required */
			/* the 2.2 consumer doesn't send this hint */
			if ( si->si_usehint && srs->sr_rhint == 0 ) {
				if ( ctxcsn )
					ber_bvarray_free_x( ctxcsn, op->o_tmpmemctx );
				if ( sids )
					op->o_tmpfree( sids, op->o_tmpmemctx );
				rs->sr_err = LDAP_SYNC_REFRESH_REQUIRED;
				rs->sr_text = "sync cookie is stale";
				goto bailout;
			}
			if ( srs->sr_state.ctxcsn ) {
				ber_bvarray_free_x( srs->sr_state.ctxcsn, op->o_tmpmemctx );
				srs->sr_state.ctxcsn = NULL;
			}
			if ( srs->sr_state.sids ) {
				slap_sl_free( srs->sr_state.sids, op->o_tmpmemctx );
				srs->sr_state.sids = NULL;
			}
			srs->sr_state.numcsns = 0;
		} else {
			gotstate = 1;
			/* If changed and doing Present lookup, send Present UUIDs */
			if ( do_present && syncprov_findcsn( op, FIND_PRESENT, 0 ) !=
				LDAP_SUCCESS ) {
				if ( ctxcsn )
					ber_bvarray_free_x( ctxcsn, op->o_tmpmemctx );
				if ( sids )
					op->o_tmpfree( sids, op->o_tmpmemctx );
				goto bailout;
			}
		}
	} else {
		/* No consumer state, assume something has changed */
		changed = SS_CHANGED;
	}

shortcut:
	/* Append CSN range to search filter, save original filter
	 * for persistent search evaluation
	 */
	if ( sop ) {
		ldap_pvt_thread_mutex_lock( &sop->s_mutex );
		sop->s_filterstr = op->ors_filterstr;
		/* correct the refcount that was set to 2 before */
		sop->s_inuse--;
	}

	/* If something changed, find the changes */
	if ( gotstate && ( changed || dirty ) ) {
		Filter *fand, *fava;

		fand = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
		fand->f_choice = LDAP_FILTER_AND;
		fand->f_next = NULL;
		fava = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
		fand->f_and = fava;
		fava->f_choice = LDAP_FILTER_GE;
		fava->f_ava = op->o_tmpalloc( sizeof(AttributeAssertion), op->o_tmpmemctx );
		fava->f_ava->aa_desc = slap_schema.si_ad_entryCSN;
#ifdef LDAP_COMP_MATCH
		fava->f_ava->aa_cf = NULL;
#endif
		ber_dupbv_x( &fava->f_ava->aa_value, &mincsn, op->o_tmpmemctx );
		fava->f_next = op->ors_filter;
		op->ors_filter = fand;
		filter2bv_x( op, op->ors_filter, &op->ors_filterstr );
		if ( sop ) {
			sop->s_flags |= PS_FIX_FILTER;
		}
	}
	if ( sop ) {
		ldap_pvt_thread_mutex_unlock( &sop->s_mutex );
	}

	/* Let our callback add needed info to returned entries */
	cb = op->o_tmpcalloc(1, sizeof(slap_callback)+sizeof(searchstate), op->o_tmpmemctx);
	ss = (searchstate *)(cb+1);
	ss->ss_on = on;
	ss->ss_so = sop;
	ss->ss_flags = do_present | changed;
	ss->ss_ctxcsn = ctxcsn;
	ss->ss_numcsns = numcsns;
	ss->ss_sids = sids;
	cb->sc_response = syncprov_search_response;
	cb->sc_private = ss;
	cb->sc_next = op->o_callback;
	op->o_callback = cb;

	/* If this is a persistent search and no changes were reported during
	 * the refresh phase, just invoke the response callback to transition
	 * us into persist phase
	 */
	if ( !changed && !dirty ) {
		rs->sr_err = LDAP_SUCCESS;
		rs->sr_nentries = 0;
		send_ldap_result( op, rs );
		return rs->sr_err;
	}
	return SLAP_CB_CONTINUE;
}

static int
syncprov_operational(
	Operation *op,
	SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t		*si = (syncprov_info_t *)on->on_bi.bi_private;

	/* This prevents generating unnecessarily; frontend will strip
	 * any statically stored copy.
	 */
	if ( op->o_sync != SLAP_CONTROL_NONE )
		return SLAP_CB_CONTINUE;

	if ( rs->sr_entry &&
		dn_match( &rs->sr_entry->e_nname, &si->si_contextdn )) {

		if ( SLAP_OPATTRS( rs->sr_attr_flags ) ||
			ad_inlist( slap_schema.si_ad_contextCSN, rs->sr_attrs )) {
			Attribute *a, **ap = NULL;

			for ( a=rs->sr_entry->e_attrs; a; a=a->a_next ) {
				if ( a->a_desc == slap_schema.si_ad_contextCSN )
					break;
			}

			ldap_pvt_thread_rdwr_rlock( &si->si_csn_rwlock );
			if ( si->si_ctxcsn ) {
				if ( !a ) {
					for ( ap = &rs->sr_operational_attrs; *ap;
						ap=&(*ap)->a_next );

					a = attr_alloc( slap_schema.si_ad_contextCSN );
					*ap = a;
				}

				if ( !ap ) {
					if ( rs_entry2modifiable( op, rs, on )) {
						a = attr_find( rs->sr_entry->e_attrs,
							slap_schema.si_ad_contextCSN );
					}
					if ( a->a_nvals != a->a_vals ) {
						ber_bvarray_free( a->a_nvals );
					}
					a->a_nvals = NULL;
					ber_bvarray_free( a->a_vals );
					a->a_vals = NULL;
					a->a_numvals = 0;
				}
				attr_valadd( a, si->si_ctxcsn, si->si_ctxcsn, si->si_numcsns );
			}
			ldap_pvt_thread_rdwr_runlock( &si->si_csn_rwlock );
		}
	}
	return SLAP_CB_CONTINUE;
}

enum {
	SP_CHKPT = 1,
	SP_SESSL,
	SP_NOPRES,
	SP_USEHINT
};

static ConfigDriver sp_cf_gen;

static ConfigTable spcfg[] = {
	{ "syncprov-checkpoint", "ops> <minutes", 3, 3, 0, ARG_MAGIC|SP_CHKPT,
		sp_cf_gen, "( OLcfgOvAt:1.1 NAME 'olcSpCheckpoint' "
			"DESC 'ContextCSN checkpoint interval in ops and minutes' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "syncprov-sessionlog", "ops", 2, 2, 0, ARG_INT|ARG_MAGIC|SP_SESSL,
		sp_cf_gen, "( OLcfgOvAt:1.2 NAME 'olcSpSessionlog' "
			"DESC 'Session log size in ops' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "syncprov-nopresent", NULL, 2, 2, 0, ARG_ON_OFF|ARG_MAGIC|SP_NOPRES,
		sp_cf_gen, "( OLcfgOvAt:1.3 NAME 'olcSpNoPresent' "
			"DESC 'Omit Present phase processing' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "syncprov-reloadhint", NULL, 2, 2, 0, ARG_ON_OFF|ARG_MAGIC|SP_USEHINT,
		sp_cf_gen, "( OLcfgOvAt:1.4 NAME 'olcSpReloadHint' "
			"DESC 'Observe Reload Hint in Request control' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs spocs[] = {
	{ "( OLcfgOvOc:1.1 "
		"NAME 'olcSyncProvConfig' "
		"DESC 'SyncRepl Provider configuration' "
		"SUP olcOverlayConfig "
		"MAY ( olcSpCheckpoint "
			"$ olcSpSessionlog "
			"$ olcSpNoPresent "
			"$ olcSpReloadHint "
		") )",
			Cft_Overlay, spcfg },
	{ NULL, 0, NULL }
};

static int
sp_cf_gen(ConfigArgs *c)
{
	slap_overinst		*on = (slap_overinst *)c->bi;
	syncprov_info_t		*si = (syncprov_info_t *)on->on_bi.bi_private;
	int rc = 0;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch ( c->type ) {
		case SP_CHKPT:
			if ( si->si_chkops || si->si_chktime ) {
				struct berval bv;
				/* we assume si_chktime is a multiple of 60
				 * because the parsed value was originally
				 * multiplied by 60 */
				bv.bv_len = snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"%d %d", si->si_chkops, si->si_chktime/60 );
				if ( bv.bv_len >= sizeof( c->cr_msg ) ) {
					rc = 1;
				} else {
					bv.bv_val = c->cr_msg;
					value_add_one( &c->rvalue_vals, &bv );
				}
			} else {
				rc = 1;
			}
			break;
		case SP_SESSL:
			if ( si->si_logs ) {
				c->value_int = si->si_logs->sl_size;
			} else {
				rc = 1;
			}
			break;
		case SP_NOPRES:
			if ( si->si_nopres ) {
				c->value_int = 1;
			} else {
				rc = 1;
			}
			break;
		case SP_USEHINT:
			if ( si->si_usehint ) {
				c->value_int = 1;
			} else {
				rc = 1;
			}
			break;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch ( c->type ) {
		case SP_CHKPT:
			si->si_chkops = 0;
			si->si_chktime = 0;
			break;
		case SP_SESSL:
			if ( si->si_logs )
				si->si_logs->sl_size = 0;
			else
				rc = LDAP_NO_SUCH_ATTRIBUTE;
			break;
		case SP_NOPRES:
			if ( si->si_nopres )
				si->si_nopres = 0;
			else
				rc = LDAP_NO_SUCH_ATTRIBUTE;
			break;
		case SP_USEHINT:
			if ( si->si_usehint )
				si->si_usehint = 0;
			else
				rc = LDAP_NO_SUCH_ATTRIBUTE;
			break;
		}
		return rc;
	}
	switch ( c->type ) {
	case SP_CHKPT:
		if ( lutil_atoi( &si->si_chkops, c->argv[1] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s unable to parse checkpoint ops # \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}
		if ( si->si_chkops <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s invalid checkpoint ops # \"%d\"",
				c->argv[0], si->si_chkops );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}
		if ( lutil_atoi( &si->si_chktime, c->argv[2] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s unable to parse checkpoint time \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}
		if ( si->si_chktime <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s invalid checkpoint time \"%d\"",
				c->argv[0], si->si_chkops );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}
		si->si_chktime *= 60;
		break;
	case SP_SESSL: {
		sessionlog *sl;
		int size = c->value_int;

		if ( size < 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s size %d is negative",
				c->argv[0], size );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}
		sl = si->si_logs;
		if ( !sl ) {
			sl = ch_malloc( sizeof( sessionlog ));
			sl->sl_mincsn = NULL;
			sl->sl_sids = NULL;
			sl->sl_num = 0;
			sl->sl_numcsns = 0;
			sl->sl_head = sl->sl_tail = NULL;
			ldap_pvt_thread_mutex_init( &sl->sl_mutex );
			si->si_logs = sl;
		}
		sl->sl_size = size;
		}
		break;
	case SP_NOPRES:
		si->si_nopres = c->value_int;
		break;
	case SP_USEHINT:
		si->si_usehint = c->value_int;
		break;
	}
	return rc;
}

/* ITS#3456 we cannot run this search on the main thread, must use a
 * child thread in order to insure we have a big enough stack.
 */
static void *
syncprov_db_otask(
	void *ptr
)
{
	syncprov_findcsn( ptr, FIND_MAXCSN, 0 );
	return NULL;
}


/* Read any existing contextCSN from the underlying db.
 * Then search for any entries newer than that. If no value exists,
 * just generate it. Cache whatever result.
 */
static int
syncprov_db_open(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst   *on = (slap_overinst *) be->bd_info;
	syncprov_info_t *si = (syncprov_info_t *)on->on_bi.bi_private;

	Connection conn = { 0 };
	OperationBuffer opbuf;
	Operation *op;
	Entry *e = NULL;
	Attribute *a;
	int rc;
	void *thrctx = NULL;

	if ( !SLAP_LASTMOD( be )) {
		Debug( LDAP_DEBUG_ANY,
			"syncprov_db_open: invalid config, lastmod must be enabled\n", 0, 0, 0 );
		return -1;
	}

	if ( slapMode & SLAP_TOOL_MODE ) {
		return 0;
	}

	rc = overlay_register_control( be, LDAP_CONTROL_SYNC );
	if ( rc ) {
		return rc;
	}

	thrctx = ldap_pvt_thread_pool_context();
	connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	op = &opbuf.ob_op;
	op->o_bd = be;
	op->o_dn = be->be_rootdn;
	op->o_ndn = be->be_rootndn;

	if ( SLAP_SYNC_SUBENTRY( be )) {
		build_new_dn( &si->si_contextdn, be->be_nsuffix,
			(struct berval *)&slap_ldapsync_cn_bv, NULL );
	} else {
		si->si_contextdn = be->be_nsuffix[0];
	}
	rc = overlay_entry_get_ov( op, &si->si_contextdn, NULL,
		slap_schema.si_ad_contextCSN, 0, &e, on );

	if ( e ) {
		ldap_pvt_thread_t tid;

		a = attr_find( e->e_attrs, slap_schema.si_ad_contextCSN );
		if ( a ) {
			ber_bvarray_dup_x( &si->si_ctxcsn, a->a_vals, NULL );
			si->si_numcsns = a->a_numvals;
			si->si_sids = slap_parse_csn_sids( si->si_ctxcsn, a->a_numvals, NULL );
			slap_sort_csn_sids( si->si_ctxcsn, si->si_sids, si->si_numcsns, NULL );
		}
		overlay_entry_release_ov( op, e, 0, on );
		if ( si->si_ctxcsn && !SLAP_DBCLEAN( be )) {
			op->o_tag = LDAP_REQ_SEARCH;
			op->o_req_dn = be->be_suffix[0];
			op->o_req_ndn = be->be_nsuffix[0];
			op->ors_scope = LDAP_SCOPE_SUBTREE;
			ldap_pvt_thread_create( &tid, 0, syncprov_db_otask, op );
			ldap_pvt_thread_join( tid, NULL );
		}
	}

	/* Didn't find a contextCSN, should we generate one? */
	if ( !si->si_ctxcsn ) {
		char csnbuf[ LDAP_PVT_CSNSTR_BUFSIZE ];
		struct berval csn;

		if ( SLAP_SYNC_SHADOW( op->o_bd )) {
		/* If we're also a consumer, then don't generate anything.
		 * Wait for our provider to send it to us, or for a local
		 * modify if we have multimaster.
		 */
			goto out;
		}
		csn.bv_val = csnbuf;
		csn.bv_len = sizeof( csnbuf );
		slap_get_csn( op, &csn, 0 );
		value_add_one( &si->si_ctxcsn, &csn );
		si->si_numcsns = 1;
		si->si_sids = ch_malloc( sizeof(int) );
		si->si_sids[0] = slap_serverID;

		/* make sure we do a checkpoint on close */
		si->si_numops++;
	}

	/* Initialize the sessionlog mincsn */
	if ( si->si_logs && si->si_numcsns ) {
		sessionlog *sl = si->si_logs;
		int i;
		ber_bvarray_dup_x( &sl->sl_mincsn, si->si_ctxcsn, NULL );
		sl->sl_numcsns = si->si_numcsns;
		sl->sl_sids = ch_malloc( si->si_numcsns * sizeof(int) );
		for ( i=0; i < si->si_numcsns; i++ )
			sl->sl_sids[i] = si->si_sids[i];
	}

out:
	op->o_bd->bd_info = (BackendInfo *)on;
	return 0;
}

/* Write the current contextCSN into the underlying db.
 */
static int
syncprov_db_close(
	BackendDB *be,
	ConfigReply *cr
)
{
    slap_overinst   *on = (slap_overinst *) be->bd_info;
    syncprov_info_t *si = (syncprov_info_t *)on->on_bi.bi_private;
#ifdef SLAP_CONFIG_DELETE
	syncops *so, *sonext;
#endif /* SLAP_CONFIG_DELETE */

	if ( slapMode & SLAP_TOOL_MODE ) {
		return 0;
	}
	if ( si->si_numops ) {
		Connection conn = {0};
		OperationBuffer opbuf;
		Operation *op;
		void *thrctx;

		thrctx = ldap_pvt_thread_pool_context();
		connection_fake_init2( &conn, &opbuf, thrctx, 0 );
		op = &opbuf.ob_op;
		op->o_bd = be;
		op->o_dn = be->be_rootdn;
		op->o_ndn = be->be_rootndn;
		syncprov_checkpoint( op, on );
	}

#ifdef SLAP_CONFIG_DELETE
	if ( !slapd_shutdown ) {
		ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
		for ( so=si->si_ops, sonext=so;  so; so=sonext  ) {
			SlapReply rs = {REP_RESULT};
			rs.sr_err = LDAP_UNAVAILABLE;
			send_ldap_result( so->s_op, &rs );
			sonext=so->s_next;
			syncprov_drop_psearch( so, 0);
		}
		si->si_ops=NULL;
		ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
	}
	overlay_unregister_control( be, LDAP_CONTROL_SYNC );
#endif /* SLAP_CONFIG_DELETE */

    return 0;
}

static int
syncprov_db_init(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	syncprov_info_t	*si;

	if ( SLAP_ISGLOBALOVERLAY( be ) ) {
		Debug( LDAP_DEBUG_ANY,
			"syncprov must be instantiated within a database.\n",
			0, 0, 0 );
		return 1;
	}

	si = ch_calloc(1, sizeof(syncprov_info_t));
	on->on_bi.bi_private = si;
	ldap_pvt_thread_rdwr_init( &si->si_csn_rwlock );
	ldap_pvt_thread_mutex_init( &si->si_ops_mutex );
	ldap_pvt_thread_mutex_init( &si->si_mods_mutex );
	ldap_pvt_thread_mutex_init( &si->si_resp_mutex );

	csn_anlist[0].an_desc = slap_schema.si_ad_entryCSN;
	csn_anlist[0].an_name = slap_schema.si_ad_entryCSN->ad_cname;
	csn_anlist[1].an_desc = slap_schema.si_ad_entryUUID;
	csn_anlist[1].an_name = slap_schema.si_ad_entryUUID->ad_cname;

	uuid_anlist[0].an_desc = slap_schema.si_ad_entryUUID;
	uuid_anlist[0].an_name = slap_schema.si_ad_entryUUID->ad_cname;

	return 0;
}

static int
syncprov_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	syncprov_info_t	*si = (syncprov_info_t *)on->on_bi.bi_private;

	if ( si ) {
		if ( si->si_logs ) {
			sessionlog *sl = si->si_logs;
			slog_entry *se = sl->sl_head;

			while ( se ) {
				slog_entry *se_next = se->se_next;
				ch_free( se );
				se = se_next;
			}
			if ( sl->sl_mincsn )
				ber_bvarray_free( sl->sl_mincsn );
			if ( sl->sl_sids )
				ch_free( sl->sl_sids );

			ldap_pvt_thread_mutex_destroy(&si->si_logs->sl_mutex);
			ch_free( si->si_logs );
		}
		if ( si->si_ctxcsn )
			ber_bvarray_free( si->si_ctxcsn );
		if ( si->si_sids )
			ch_free( si->si_sids );
		ldap_pvt_thread_mutex_destroy( &si->si_resp_mutex );
		ldap_pvt_thread_mutex_destroy( &si->si_mods_mutex );
		ldap_pvt_thread_mutex_destroy( &si->si_ops_mutex );
		ldap_pvt_thread_rdwr_destroy( &si->si_csn_rwlock );
		ch_free( si );
	}

	return 0;
}

static int syncprov_parseCtrl (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	ber_tag_t tag;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_int_t mode;
	ber_len_t len;
	struct berval cookie = BER_BVNULL;
	sync_control *sr;
	int rhint = 0;

	if ( op->o_sync != SLAP_CONTROL_NONE ) {
		rs->sr_text = "Sync control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( op->o_pagedresults != SLAP_CONTROL_NONE ) {
		rs->sr_text = "Sync control specified with pagedResults control";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "Sync control value is absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value ) ) {
		rs->sr_text = "Sync control value is empty";
		return LDAP_PROTOCOL_ERROR;
	}

	/* Parse the control value
	 *      syncRequestValue ::= SEQUENCE {
	 *              mode   ENUMERATED {
	 *                      -- 0 unused
	 *                      refreshOnly		(1),
	 *                      -- 2 reserved
	 *                      refreshAndPersist	(3)
	 *              },
	 *              cookie  syncCookie OPTIONAL
	 *      }
	 */

	ber_init2( ber, &ctrl->ldctl_value, 0 );

	if ( (tag = ber_scanf( ber, "{i" /*}*/, &mode )) == LBER_ERROR ) {
		rs->sr_text = "Sync control : mode decoding error";
		return LDAP_PROTOCOL_ERROR;
	}

	switch( mode ) {
	case LDAP_SYNC_REFRESH_ONLY:
		mode = SLAP_SYNC_REFRESH;
		break;
	case LDAP_SYNC_REFRESH_AND_PERSIST:
		mode = SLAP_SYNC_REFRESH_AND_PERSIST;
		break;
	default:
		rs->sr_text = "Sync control : unknown update mode";
		return LDAP_PROTOCOL_ERROR;
	}

	tag = ber_peek_tag( ber, &len );

	if ( tag == LDAP_TAG_SYNC_COOKIE ) {
		if (( ber_scanf( ber, /*{*/ "m", &cookie )) == LBER_ERROR ) {
			rs->sr_text = "Sync control : cookie decoding error";
			return LDAP_PROTOCOL_ERROR;
		}
		tag = ber_peek_tag( ber, &len );
	}
	if ( tag == LDAP_TAG_RELOAD_HINT ) {
		if (( ber_scanf( ber, /*{*/ "b", &rhint )) == LBER_ERROR ) {
			rs->sr_text = "Sync control : rhint decoding error";
			return LDAP_PROTOCOL_ERROR;
		}
	}
	if (( ber_scanf( ber, /*{*/ "}")) == LBER_ERROR ) {
			rs->sr_text = "Sync control : decoding error";
			return LDAP_PROTOCOL_ERROR;
	}
	sr = op->o_tmpcalloc( 1, sizeof(struct sync_control), op->o_tmpmemctx );
	sr->sr_rhint = rhint;
	if (!BER_BVISNULL(&cookie)) {
		ber_dupbv_x( &sr->sr_state.octet_str, &cookie, op->o_tmpmemctx );
		/* If parse fails, pretend no cookie was sent */
		if ( slap_parse_sync_cookie( &sr->sr_state, op->o_tmpmemctx ) ||
			sr->sr_state.rid == -1 ) {
			if ( sr->sr_state.ctxcsn ) {
				ber_bvarray_free_x( sr->sr_state.ctxcsn, op->o_tmpmemctx );
				sr->sr_state.ctxcsn = NULL;
			}
			sr->sr_state.numcsns = 0;
		}
	}

	op->o_controls[slap_cids.sc_LDAPsync] = sr;

	op->o_sync = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	op->o_sync_mode |= mode;	/* o_sync_mode shares o_sync */

	return LDAP_SUCCESS;
}

/* This overlay is set up for dynamic loading via moduleload. For static
 * configuration, you'll need to arrange for the slap_overinst to be
 * initialized and registered by some other function inside slapd.
 */

static slap_overinst 		syncprov;

int
syncprov_initialize()
{
	int rc;

	rc = register_supported_control( LDAP_CONTROL_SYNC,
		SLAP_CTRL_SEARCH, NULL,
		syncprov_parseCtrl, &slap_cids.sc_LDAPsync );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"syncprov_init: Failed to register control %d\n", rc, 0, 0 );
		return rc;
	}

	syncprov.on_bi.bi_type = "syncprov";
	syncprov.on_bi.bi_db_init = syncprov_db_init;
	syncprov.on_bi.bi_db_destroy = syncprov_db_destroy;
	syncprov.on_bi.bi_db_open = syncprov_db_open;
	syncprov.on_bi.bi_db_close = syncprov_db_close;

	syncprov.on_bi.bi_op_abandon = syncprov_op_abandon;
	syncprov.on_bi.bi_op_cancel = syncprov_op_abandon;

	syncprov.on_bi.bi_op_add = syncprov_op_mod;
	syncprov.on_bi.bi_op_compare = syncprov_op_compare;
	syncprov.on_bi.bi_op_delete = syncprov_op_mod;
	syncprov.on_bi.bi_op_modify = syncprov_op_mod;
	syncprov.on_bi.bi_op_modrdn = syncprov_op_mod;
	syncprov.on_bi.bi_op_search = syncprov_op_search;
	syncprov.on_bi.bi_extended = syncprov_op_extended;
	syncprov.on_bi.bi_operational = syncprov_operational;

	syncprov.on_bi.bi_cf_ocs = spocs;

	generic_filter.f_desc = slap_schema.si_ad_objectClass;

	rc = config_register_schema( spcfg, spocs );
	if ( rc ) return rc;

	return overlay_register( &syncprov );
}

#if SLAPD_OVER_SYNCPROV == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return syncprov_initialize();
}
#endif /* SLAPD_OVER_SYNCPROV == SLAPD_MOD_DYNAMIC */

#endif /* defined(SLAPD_OVER_SYNCPROV) */
