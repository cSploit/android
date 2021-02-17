/* memberof.c - back-reference for group membership */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2007 Pierangelo Masarati <ando@sys-net.it>
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
/* ACKNOWLEDGMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software, sponsored by SysNet s.r.l.
 */

#include "portable.h"

#ifdef SLAPD_OVER_MEMBEROF

#include <stdio.h>

#include "ac/string.h"
#include "ac/socket.h"

#include "slap.h"
#include "config.h"
#include "lutil.h"

/*
 *	Glossary:
 *
 *		GROUP		a group object (an entry with GROUP_OC
 *				objectClass)
 *		MEMBER		a member object (an entry whose DN is
 *				listed as MEMBER_AT value of a GROUP)
 *		GROUP_OC	the objectClass of the group object
 *				(default: groupOfNames)
 *		MEMBER_AT	the membership attribute, DN-valued;
 *				note: nameAndOptionalUID is tolerated
 *				as soon as the optionalUID is absent
 *				(default: member)
 *		MEMBER_OF	reverse membership attribute
 *				(default: memberOf)
 *
 * 	- add:
 *		- if the entry that is being added is a GROUP,
 *		  the MEMBER_AT defined as values of the add operation
 *		  get the MEMBER_OF value directly from the request.
 *
 *		  if configured to do so, the MEMBER objects do not exist,
 *		  and no relax control is issued, either:
 *			- fail
 *			- drop non-existing members
 *		  (by default: don't muck with values)
 *
 *		- if (configured to do so,) the referenced GROUP exists,
 *		  the relax control is set and the user has
 *		  "manage" privileges, allow to add MEMBER_OF values to
 *		  generic entries.
 *
 *	- modify:
 *		- if the entry being modified is a GROUP_OC and the 
 *		  MEMBER_AT attribute is modified, the MEMBER_OF value
 *		  of the (existing) MEMBER_AT entries that are affected
 *		  is modified according to the request:
 *			- if a MEMBER is removed from the group,
 *			  delete the corresponding MEMBER_OF
 *			- if a MEMBER is added to a group,
 *			  add the corresponding MEMBER_OF
 *
 *		  We need to determine, from the database, if it is
 *		  a GROUP_OC, and we need to check, from the
 *		  modification list, if the MEMBER_AT attribute is being
 *		  affected, and what MEMBER_AT values are affected.
 *
 *		  if configured to do so, the entries corresponding to
 *		  the MEMBER_AT values do not exist, and no relax control
 *		  is issued, either:
 *			- fail
 *			- drop non-existing members
 *		  (by default: don't muck with values)
 *
 *		- if configured to do so, the referenced GROUP exists,
 *		  (the relax control is set) and the user has
 *		  "manage" privileges, allow to add MEMBER_OF values to
 *		  generic entries; the change is NOT automatically reflected
 *		  in the MEMBER attribute of the GROUP referenced
 *		  by the value of MEMBER_OF; a separate modification,
 *		  with or without relax control, needs to be performed.
 *
 *	- modrdn:
 *		- if the entry being renamed is a GROUP, the MEMBER_OF
 *		  value of the (existing) MEMBER objects is modified
 *		  accordingly based on the newDN of the GROUP.
 *
 *		  We need to determine, from the database, if it is
 *		  a GROUP; the list of MEMBER objects is obtained from
 *		  the database.
 *
 *		  Non-existing MEMBER objects are ignored, since the
 *		  MEMBER_AT is not being addressed by the operation.
 *
 *		- if the entry being renamed has the MEMBER_OF attribute,
 *		  the corresponding MEMBER value must be modified in the
 *		  respective group entries.
 *		
 *
 *	- delete:
 *		- if the entry being deleted is a GROUP, the (existing)
 *		  MEMBER objects are modified accordingly; a copy of the 
 *		  values of the MEMBER_AT is saved and, if the delete 
 *		  succeeds, the MEMBER_OF value of the (existing) MEMBER
 *		  objects is deleted.
 *
 *		  We need to determine, from the database, if it is
 *		  a GROUP.
 *
 *		  Non-existing MEMBER objects are ignored, since the entry
 *		  is being deleted.
 *
 *		- if the entry being deleted has the MEMBER_OF attribute,
 *		  the corresponding value of the MEMBER_AT must be deleted
 *		  from the respective GROUP entries.
 */

#define	SLAPD_MEMBEROF_ATTR	"memberOf"

static slap_overinst		memberof;

typedef struct memberof_t {
	struct berval		mo_dn;
	struct berval		mo_ndn;

	ObjectClass		*mo_oc_group;
	AttributeDescription	*mo_ad_member;
	AttributeDescription	*mo_ad_memberof;
	
	struct berval		mo_groupFilterstr;
	AttributeAssertion	mo_groupAVA;
	Filter			mo_groupFilter;

	struct berval		mo_memberFilterstr;
	Filter			mo_memberFilter;

	unsigned		mo_flags;
#define	MEMBEROF_NONE		0x00U
#define	MEMBEROF_FDANGLING_DROP	0x01U
#define	MEMBEROF_FDANGLING_ERROR	0x02U
#define	MEMBEROF_FDANGLING_MASK	(MEMBEROF_FDANGLING_DROP|MEMBEROF_FDANGLING_ERROR)
#define	MEMBEROF_FREFINT	0x04U
#define	MEMBEROF_FREVERSE	0x08U

	ber_int_t		mo_dangling_err;

#define MEMBEROF_CHK(mo,f) \
	(((mo)->mo_flags & (f)) == (f))
#define MEMBEROF_DANGLING_CHECK(mo) \
	((mo)->mo_flags & MEMBEROF_FDANGLING_MASK)
#define MEMBEROF_DANGLING_DROP(mo) \
	MEMBEROF_CHK((mo),MEMBEROF_FDANGLING_DROP)
#define MEMBEROF_DANGLING_ERROR(mo) \
	MEMBEROF_CHK((mo),MEMBEROF_FDANGLING_ERROR)
#define MEMBEROF_REFINT(mo) \
	MEMBEROF_CHK((mo),MEMBEROF_FREFINT)
#define MEMBEROF_REVERSE(mo) \
	MEMBEROF_CHK((mo),MEMBEROF_FREVERSE)
} memberof_t;

typedef enum memberof_is_t {
	MEMBEROF_IS_NONE = 0x00,
	MEMBEROF_IS_GROUP = 0x01,
	MEMBEROF_IS_MEMBER = 0x02,
	MEMBEROF_IS_BOTH = (MEMBEROF_IS_GROUP|MEMBEROF_IS_MEMBER)
} memberof_is_t;

typedef struct memberof_cookie_t {
	AttributeDescription	*ad;
	BerVarray		vals;
	int			foundit;
} memberof_cookie_t;

typedef struct memberof_cbinfo_t {
	slap_overinst *on;
	BerVarray member;
	BerVarray memberof;
	memberof_is_t what;
} memberof_cbinfo_t;
	
static int
memberof_isGroupOrMember_cb( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		memberof_cookie_t	*mc;

		mc = (memberof_cookie_t *)op->o_callback->sc_private;
		mc->foundit = 1;
	}

	return 0;
}

/*
 * callback for internal search that saves the member attribute values
 * of groups being deleted.
 */
static int
memberof_saveMember_cb( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		memberof_cookie_t	*mc;
		Attribute		*a;

		mc = (memberof_cookie_t *)op->o_callback->sc_private;
		mc->foundit = 1;

		assert( rs->sr_entry != NULL );
		assert( rs->sr_entry->e_attrs != NULL );

		a = attr_find( rs->sr_entry->e_attrs, mc->ad );
		if ( a != NULL ) {
			ber_bvarray_dup_x( &mc->vals, a->a_nvals, op->o_tmpmemctx );

			assert( attr_find( a->a_next, mc->ad ) == NULL );
		}
	}

	return 0;
}

/*
 * the delete hook performs an internal search that saves the member
 * attribute values of groups being deleted.
 */
static int
memberof_isGroupOrMember( Operation *op, memberof_cbinfo_t *mci )
{
	slap_overinst		*on = mci->on;
	memberof_t		*mo = (memberof_t *)on->on_bi.bi_private;

	Operation		op2 = *op;
	slap_callback		cb = { 0 };
	BackendInfo	*bi = op->o_bd->bd_info;
	AttributeName		an[ 2 ];

	memberof_is_t		iswhat = MEMBEROF_IS_NONE;
	memberof_cookie_t	mc;

	assert( mci->what != MEMBEROF_IS_NONE );

	cb.sc_private = &mc;
	if ( op->o_tag == LDAP_REQ_DELETE ) {
		cb.sc_response = memberof_saveMember_cb;

	} else {
		cb.sc_response = memberof_isGroupOrMember_cb;
	}

	op2.o_tag = LDAP_REQ_SEARCH;
	op2.o_callback = &cb;
	op2.o_dn = op->o_bd->be_rootdn;
	op2.o_ndn = op->o_bd->be_rootndn;

	op2.ors_scope = LDAP_SCOPE_BASE;
	op2.ors_deref = LDAP_DEREF_NEVER;
	BER_BVZERO( &an[ 1 ].an_name );
	op2.ors_attrs = an;
	op2.ors_attrsonly = 0;
	op2.ors_limit = NULL;
	op2.ors_slimit = 1;
	op2.ors_tlimit = SLAP_NO_LIMIT;

	if ( mci->what & MEMBEROF_IS_GROUP ) {
		SlapReply	rs2 = { REP_RESULT };

		mc.ad = mo->mo_ad_member;
		mc.foundit = 0;
		mc.vals = NULL;
		an[ 0 ].an_desc = mo->mo_ad_member;
		an[ 0 ].an_name = an[ 0 ].an_desc->ad_cname;
		op2.ors_filterstr = mo->mo_groupFilterstr;
		op2.ors_filter = &mo->mo_groupFilter;

		op2.o_bd->bd_info = (BackendInfo *)on->on_info;
		(void)op->o_bd->be_search( &op2, &rs2 );
		op2.o_bd->bd_info = bi;

		if ( mc.foundit ) {
			iswhat |= MEMBEROF_IS_GROUP;
			if ( mc.vals ) mci->member = mc.vals;

		}
	}

	if ( mci->what & MEMBEROF_IS_MEMBER ) {
		SlapReply	rs2 = { REP_RESULT };

		mc.ad = mo->mo_ad_memberof;
		mc.foundit = 0;
		mc.vals = NULL;
		an[ 0 ].an_desc = mo->mo_ad_memberof;
		an[ 0 ].an_name = an[ 0 ].an_desc->ad_cname;
		op2.ors_filterstr = mo->mo_memberFilterstr;
		op2.ors_filter = &mo->mo_memberFilter;

		op2.o_bd->bd_info = (BackendInfo *)on->on_info;
		(void)op->o_bd->be_search( &op2, &rs2 );
		op2.o_bd->bd_info = bi;

		if ( mc.foundit ) {
			iswhat |= MEMBEROF_IS_MEMBER;
			if ( mc.vals ) mci->memberof = mc.vals;

		}
	}

	mci->what = iswhat;

	return LDAP_SUCCESS;
}

/*
 * response callback that adds memberof values when a group is modified.
 */
static void
memberof_value_modify(
	Operation		*op,
	struct berval		*ndn,
	AttributeDescription	*ad,
	struct berval		*old_dn,
	struct berval		*old_ndn,
	struct berval		*new_dn,
	struct berval		*new_ndn )
{
	memberof_cbinfo_t *mci = op->o_callback->sc_private;
	slap_overinst	*on = mci->on;
	memberof_t	*mo = (memberof_t *)on->on_bi.bi_private;

	Operation	op2 = *op;
	unsigned long opid = op->o_opid;
	SlapReply	rs2 = { REP_RESULT };
	slap_callback	cb = { NULL, slap_null_cb, NULL, NULL };
	Modifications	mod[ 2 ] = { { { 0 } } }, *ml;
	struct berval	values[ 4 ], nvalues[ 4 ];
	int		mcnt = 0;

	op2.o_tag = LDAP_REQ_MODIFY;

	op2.o_req_dn = *ndn;
	op2.o_req_ndn = *ndn;

	op2.o_callback = &cb;
	op2.o_dn = op->o_bd->be_rootdn;
	op2.o_ndn = op->o_bd->be_rootndn;
	op2.orm_modlist = NULL;

	/* Internal ops, never replicate these */
	op2.o_opid = 0;		/* shared with op, saved above */
	op2.orm_no_opattrs = 1;
	op2.o_dont_replicate = 1;

	if ( !BER_BVISNULL( &mo->mo_ndn ) ) {
		ml = &mod[ mcnt ];
		ml->sml_numvals = 1;
		ml->sml_values = &values[ 0 ];
		ml->sml_values[ 0 ] = mo->mo_dn;
		BER_BVZERO( &ml->sml_values[ 1 ] );
		ml->sml_nvalues = &nvalues[ 0 ];
		ml->sml_nvalues[ 0 ] = mo->mo_ndn;
		BER_BVZERO( &ml->sml_nvalues[ 1 ] );
		ml->sml_desc = slap_schema.si_ad_modifiersName;
		ml->sml_type = ml->sml_desc->ad_cname;
		ml->sml_op = LDAP_MOD_REPLACE;
		ml->sml_flags = SLAP_MOD_INTERNAL;
		ml->sml_next = op2.orm_modlist;
		op2.orm_modlist = ml;

		mcnt++;
	}

	ml = &mod[ mcnt ];
	ml->sml_numvals = 1;
	ml->sml_values = &values[ 2 ];
	BER_BVZERO( &ml->sml_values[ 1 ] );
	ml->sml_nvalues = &nvalues[ 2 ];
	BER_BVZERO( &ml->sml_nvalues[ 1 ] );
	ml->sml_desc = ad;
	ml->sml_type = ml->sml_desc->ad_cname;
	ml->sml_flags = SLAP_MOD_INTERNAL;
	ml->sml_next = op2.orm_modlist;
	op2.orm_modlist = ml;

	if ( new_ndn != NULL ) {
		BackendInfo *bi = op2.o_bd->bd_info;
		OpExtra	oex;

		assert( !BER_BVISNULL( new_dn ) );
		assert( !BER_BVISNULL( new_ndn ) );

		ml = &mod[ mcnt ];
		ml->sml_op = LDAP_MOD_ADD;

		ml->sml_values[ 0 ] = *new_dn;
		ml->sml_nvalues[ 0 ] = *new_ndn;

		oex.oe_key = (void *)&memberof;
		LDAP_SLIST_INSERT_HEAD(&op2.o_extra, &oex, oe_next);
		op2.o_bd->bd_info = (BackendInfo *)on->on_info;
		(void)op->o_bd->be_modify( &op2, &rs2 );
		op2.o_bd->bd_info = bi;
		LDAP_SLIST_REMOVE(&op2.o_extra, &oex, OpExtra, oe_next);
		if ( rs2.sr_err != LDAP_SUCCESS ) {
			char buf[ SLAP_TEXT_BUFLEN ];
			snprintf( buf, sizeof( buf ),
				"memberof_value_modify DN=\"%s\" add %s=\"%s\" failed err=%d",
				op2.o_req_dn.bv_val, ad->ad_cname.bv_val, new_dn->bv_val, rs2.sr_err );
			Debug( LDAP_DEBUG_ANY, "%s: %s\n",
				op->o_log_prefix, buf, 0 );
		}

		assert( op2.orm_modlist == &mod[ mcnt ] );
		assert( mcnt == 0 || op2.orm_modlist->sml_next == &mod[ 0 ] );
		ml = op2.orm_modlist->sml_next;
		if ( mcnt == 1 ) {
			assert( ml == &mod[ 0 ] );
			ml = ml->sml_next;
		}
		if ( ml != NULL ) {
			slap_mods_free( ml, 1 );
		}

		mod[ 0 ].sml_next = NULL;
	}

	if ( old_ndn != NULL ) {
		BackendInfo *bi = op2.o_bd->bd_info;
		OpExtra	oex;

		assert( !BER_BVISNULL( old_dn ) );
		assert( !BER_BVISNULL( old_ndn ) );

		ml = &mod[ mcnt ];
		ml->sml_op = LDAP_MOD_DELETE;

		ml->sml_values[ 0 ] = *old_dn;
		ml->sml_nvalues[ 0 ] = *old_ndn;

		oex.oe_key = (void *)&memberof;
		LDAP_SLIST_INSERT_HEAD(&op2.o_extra, &oex, oe_next);
		op2.o_bd->bd_info = (BackendInfo *)on->on_info;
		(void)op->o_bd->be_modify( &op2, &rs2 );
		op2.o_bd->bd_info = bi;
		LDAP_SLIST_REMOVE(&op2.o_extra, &oex, OpExtra, oe_next);
		if ( rs2.sr_err != LDAP_SUCCESS ) {
			char buf[ SLAP_TEXT_BUFLEN ];
			snprintf( buf, sizeof( buf ),
				"memberof_value_modify DN=\"%s\" delete %s=\"%s\" failed err=%d",
				op2.o_req_dn.bv_val, ad->ad_cname.bv_val, old_dn->bv_val, rs2.sr_err );
			Debug( LDAP_DEBUG_ANY, "%s: %s\n",
				op->o_log_prefix, buf, 0 );
		}

		assert( op2.orm_modlist == &mod[ mcnt ] );
		ml = op2.orm_modlist->sml_next;
		if ( mcnt == 1 ) {
			assert( ml == &mod[ 0 ] );
			ml = ml->sml_next;
		}
		if ( ml != NULL ) {
			slap_mods_free( ml, 1 );
		}
	}
	/* restore original opid */
	op->o_opid = opid;

	/* FIXME: if old_group_ndn doesn't exist, both delete __and__
	 * add will fail; better split in two operations, although
	 * not optimal in terms of performance.  At least it would
	 * move towards self-repairing capabilities. */
}

static int
memberof_cleanup( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;
	memberof_cbinfo_t *mci = sc->sc_private;

	op->o_callback = sc->sc_next;
	if ( mci->memberof )
		ber_bvarray_free_x( mci->memberof, op->o_tmpmemctx );
	if ( mci->member )
		ber_bvarray_free_x( mci->member, op->o_tmpmemctx );
	op->o_tmpfree( sc, op->o_tmpmemctx );
	return 0;
}

static int memberof_res_add( Operation *op, SlapReply *rs );
static int memberof_res_delete( Operation *op, SlapReply *rs );
static int memberof_res_modify( Operation *op, SlapReply *rs );
static int memberof_res_modrdn( Operation *op, SlapReply *rs );

static int
memberof_op_add( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	memberof_t	*mo = (memberof_t *)on->on_bi.bi_private;

	Attribute	**ap, **map = NULL;
	int		rc = SLAP_CB_CONTINUE;
	int		i;
	struct berval	save_dn, save_ndn;
	slap_callback *sc;
	memberof_cbinfo_t *mci;
	OpExtra		*oex;

	LDAP_SLIST_FOREACH( oex, &op->o_extra, oe_next ) {
		if ( oex->oe_key == (void *)&memberof )
			return SLAP_CB_CONTINUE;
	}

	if ( op->ora_e->e_attrs == NULL ) {
		/* FIXME: global overlay; need to deal with */
		Debug( LDAP_DEBUG_ANY, "%s: memberof_op_add(\"%s\"): "
			"consistency checks not implemented when overlay "
			"is instantiated as global.\n",
			op->o_log_prefix, op->o_req_dn.bv_val, 0 );
		return SLAP_CB_CONTINUE;
	}

	if ( MEMBEROF_REVERSE( mo ) ) {
		for ( ap = &op->ora_e->e_attrs; *ap; ap = &(*ap)->a_next ) {
			Attribute	*a = *ap;

			if ( a->a_desc == mo->mo_ad_memberof ) {
				map = ap;
				break;
			}
		}
	}

	save_dn = op->o_dn;
	save_ndn = op->o_ndn;

	if ( MEMBEROF_DANGLING_CHECK( mo )
			&& !get_relax( op )
			&& is_entry_objectclass_or_sub( op->ora_e, mo->mo_oc_group ) )
	{
		op->o_dn = op->o_bd->be_rootdn;
		op->o_ndn = op->o_bd->be_rootndn;
		op->o_bd->bd_info = (BackendInfo *)on->on_info;

		for ( ap = &op->ora_e->e_attrs; *ap; ) {
			Attribute	*a = *ap;

			if ( !is_ad_subtype( a->a_desc, mo->mo_ad_member ) ) {
				ap = &a->a_next;
				continue;
			}

			assert( a->a_nvals != NULL );

			for ( i = 0; !BER_BVISNULL( &a->a_nvals[ i ] ); i++ ) {
				Entry		*e = NULL;

				/* ITS#6670 Ignore member pointing to this entry */
				if ( dn_match( &a->a_nvals[i], &save_ndn ))
					continue;

				rc = be_entry_get_rw( op, &a->a_nvals[ i ],
						NULL, NULL, 0, &e );
				if ( rc == LDAP_SUCCESS ) {
					be_entry_release_r( op, e );
					continue;
				}

				if ( MEMBEROF_DANGLING_ERROR( mo ) ) {
					rc = rs->sr_err = mo->mo_dangling_err;
					rs->sr_text = "adding non-existing object "
						"as group member";
					send_ldap_result( op, rs );
					goto done;
				}

				if ( MEMBEROF_DANGLING_DROP( mo ) ) {
					int	j;
	
					Debug( LDAP_DEBUG_ANY, "%s: memberof_op_add(\"%s\"): "
						"member=\"%s\" does not exist (stripping...)\n",
						op->o_log_prefix, op->ora_e->e_name.bv_val,
						a->a_vals[ i ].bv_val );
	
					for ( j = i + 1; !BER_BVISNULL( &a->a_nvals[ j ] ); j++ );
					ber_memfree( a->a_vals[ i ].bv_val );
					BER_BVZERO( &a->a_vals[ i ] );
					if ( a->a_nvals != a->a_vals ) {
						ber_memfree( a->a_nvals[ i ].bv_val );
						BER_BVZERO( &a->a_nvals[ i ] );
					}
					if ( j - i == 1 ) {
						break;
					}
		
					AC_MEMCPY( &a->a_vals[ i ], &a->a_vals[ i + 1 ],
						sizeof( struct berval ) * ( j - i ) );
					if ( a->a_nvals != a->a_vals ) {
						AC_MEMCPY( &a->a_nvals[ i ], &a->a_nvals[ i + 1 ],
							sizeof( struct berval ) * ( j - i ) );
					}
					i--;
					a->a_numvals--;
				}
			}

			/* If all values have been removed,
			 * remove the attribute itself. */
			if ( BER_BVISNULL( &a->a_nvals[ 0 ] ) ) {
				*ap = a->a_next;
				attr_free( a );
	
			} else {
				ap = &a->a_next;
			}
		}
		op->o_dn = save_dn;
		op->o_ndn = save_ndn;
		op->o_bd->bd_info = (BackendInfo *)on;
	}

	if ( map != NULL ) {
		Attribute		*a = *map;
		AccessControlState	acl_state = ACL_STATE_INIT;

		for ( i = 0; !BER_BVISNULL( &a->a_nvals[ i ] ); i++ ) {
			Entry		*e;

			op->o_bd->bd_info = (BackendInfo *)on->on_info;
			/* access is checked with the original identity */
			rc = access_allowed( op, op->ora_e, mo->mo_ad_memberof,
					&a->a_nvals[ i ], ACL_WADD,
					&acl_state );
			if ( rc == 0 ) {
				rc = rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
				rs->sr_text = NULL;
				send_ldap_result( op, rs );
				goto done;
			}
			/* ITS#6670 Ignore member pointing to this entry */
			if ( dn_match( &a->a_nvals[i], &save_ndn ))
				continue;

			rc = be_entry_get_rw( op, &a->a_nvals[ i ],
					NULL, NULL, 0, &e );
			op->o_bd->bd_info = (BackendInfo *)on;
			if ( rc != LDAP_SUCCESS ) {
				if ( get_relax( op ) ) {
					continue;
				}

				if ( MEMBEROF_DANGLING_ERROR( mo ) ) {
					rc = rs->sr_err = mo->mo_dangling_err;
					rs->sr_text = "adding non-existing object "
						"as memberof";
					send_ldap_result( op, rs );
					goto done;
				}

				if ( MEMBEROF_DANGLING_DROP( mo ) ) {
					int	j;
	
					Debug( LDAP_DEBUG_ANY, "%s: memberof_op_add(\"%s\"): "
						"memberof=\"%s\" does not exist (stripping...)\n",
						op->o_log_prefix, op->ora_e->e_name.bv_val,
						a->a_nvals[ i ].bv_val );
	
					for ( j = i + 1; !BER_BVISNULL( &a->a_nvals[ j ] ); j++ );
					ber_memfree( a->a_vals[ i ].bv_val );
					BER_BVZERO( &a->a_vals[ i ] );
					if ( a->a_nvals != a->a_vals ) {
						ber_memfree( a->a_nvals[ i ].bv_val );
						BER_BVZERO( &a->a_nvals[ i ] );
					}
					if ( j - i == 1 ) {
						break;
					}
		
					AC_MEMCPY( &a->a_vals[ i ], &a->a_vals[ i + 1 ],
						sizeof( struct berval ) * ( j - i ) );
					if ( a->a_nvals != a->a_vals ) {
						AC_MEMCPY( &a->a_nvals[ i ], &a->a_nvals[ i + 1 ],
							sizeof( struct berval ) * ( j - i ) );
					}
					i--;
				}
				
				continue;
			}

			/* access is checked with the original identity */
			op->o_bd->bd_info = (BackendInfo *)on->on_info;
			rc = access_allowed( op, e, mo->mo_ad_member,
					&op->o_req_ndn, ACL_WADD, NULL );
			be_entry_release_r( op, e );
			op->o_bd->bd_info = (BackendInfo *)on;

			if ( !rc ) {
				rc = rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
				rs->sr_text = "insufficient access to object referenced by memberof";
				send_ldap_result( op, rs );
				goto done;
			}
		}

		if ( BER_BVISNULL( &a->a_nvals[ 0 ] ) ) {
			*map = a->a_next;
			attr_free( a );
		}
	}

	rc = SLAP_CB_CONTINUE;

	sc = op->o_tmpalloc( sizeof(slap_callback)+sizeof(*mci), op->o_tmpmemctx );
	sc->sc_private = sc+1;
	sc->sc_response = memberof_res_add;
	sc->sc_cleanup = memberof_cleanup;
	mci = sc->sc_private;
	mci->on = on;
	mci->member = NULL;
	mci->memberof = NULL;
	sc->sc_next = op->o_callback;
	op->o_callback = sc;

done:;
	op->o_dn = save_dn;
	op->o_ndn = save_ndn;
	op->o_bd->bd_info = (BackendInfo *)on;

	return rc;
}

static int
memberof_op_delete( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	memberof_t	*mo = (memberof_t *)on->on_bi.bi_private;

	slap_callback *sc;
	memberof_cbinfo_t *mci;
	OpExtra		*oex;

	LDAP_SLIST_FOREACH( oex, &op->o_extra, oe_next ) {
		if ( oex->oe_key == (void *)&memberof )
			return SLAP_CB_CONTINUE;
	}

	sc = op->o_tmpalloc( sizeof(slap_callback)+sizeof(*mci), op->o_tmpmemctx );
	sc->sc_private = sc+1;
	sc->sc_response = memberof_res_delete;
	sc->sc_cleanup = memberof_cleanup;
	mci = sc->sc_private;
	mci->on = on;
	mci->member = NULL;
	mci->memberof = NULL;
	mci->what = MEMBEROF_IS_GROUP;
	if ( MEMBEROF_REFINT( mo ) ) {
		mci->what = MEMBEROF_IS_BOTH;
	}

	memberof_isGroupOrMember( op, mci );

	sc->sc_next = op->o_callback;
	op->o_callback = sc;

	return SLAP_CB_CONTINUE;
}

static int
memberof_op_modify( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	memberof_t	*mo = (memberof_t *)on->on_bi.bi_private;

	Modifications	**mlp, **mmlp = NULL;
	int		rc = SLAP_CB_CONTINUE, save_member = 0;
	struct berval	save_dn, save_ndn;
	slap_callback *sc;
	memberof_cbinfo_t *mci, mcis;
	OpExtra		*oex;

	LDAP_SLIST_FOREACH( oex, &op->o_extra, oe_next ) {
		if ( oex->oe_key == (void *)&memberof )
			return SLAP_CB_CONTINUE;
	}

	if ( MEMBEROF_REVERSE( mo ) ) {
		for ( mlp = &op->orm_modlist; *mlp; mlp = &(*mlp)->sml_next ) {
			Modifications	*ml = *mlp;

			if ( ml->sml_desc == mo->mo_ad_memberof ) {
				mmlp = mlp;
				break;
			}
		}
	}

	save_dn = op->o_dn;
	save_ndn = op->o_ndn;
	mcis.on = on;
	mcis.what = MEMBEROF_IS_GROUP;

	if ( memberof_isGroupOrMember( op, &mcis ) == LDAP_SUCCESS
		&& ( mcis.what & MEMBEROF_IS_GROUP ) )
	{
		Modifications *ml;

		for ( ml = op->orm_modlist; ml; ml = ml->sml_next ) {
			if ( ml->sml_desc == mo->mo_ad_member ) {
				switch ( ml->sml_op ) {
				case LDAP_MOD_DELETE:
				case LDAP_MOD_REPLACE:
				case SLAP_MOD_SOFTDEL: /* ITS#7487: can be used by syncrepl (in mirror mode?) */
					save_member = 1;
					break;
				}
			}
		}


		if ( MEMBEROF_DANGLING_CHECK( mo )
				&& !get_relax( op ) )
		{
			op->o_dn = op->o_bd->be_rootdn;
			op->o_ndn = op->o_bd->be_rootndn;
			op->o_bd->bd_info = (BackendInfo *)on->on_info;
		
			assert( op->orm_modlist != NULL );
		
			for ( mlp = &op->orm_modlist; *mlp; ) {
				Modifications	*ml = *mlp;
				int		i;
		
				if ( !is_ad_subtype( ml->sml_desc, mo->mo_ad_member ) ) {
					mlp = &ml->sml_next;
					continue;
				}
		
				switch ( ml->sml_op ) {
				case LDAP_MOD_DELETE:
				case SLAP_MOD_SOFTDEL: /* ITS#7487: can be used by syncrepl (in mirror mode?) */
					/* we don't care about cancellations: if the value
					 * exists, fine; if it doesn't, we let the underlying
					 * database fail as appropriate; */
					mlp = &ml->sml_next;
					break;
		
				case LDAP_MOD_REPLACE:
 					/* Handle this just like a delete (see above) */
 					if ( !ml->sml_values ) {
 						mlp = &ml->sml_next;
 						break;
 					}
 
				case LDAP_MOD_ADD:
				case SLAP_MOD_SOFTADD: /* ITS#7487 */
				case SLAP_MOD_ADD_IF_NOT_PRESENT: /* ITS#7487 */
					/* NOTE: right now, the attributeType we use
					 * for member must have a normalized value */
					assert( ml->sml_nvalues != NULL );
		
					for ( i = 0; !BER_BVISNULL( &ml->sml_nvalues[ i ] ); i++ ) {
						int		rc;
						Entry		*e;
		
						/* ITS#6670 Ignore member pointing to this entry */
						if ( dn_match( &ml->sml_nvalues[i], &save_ndn ))
							continue;

						if ( be_entry_get_rw( op, &ml->sml_nvalues[ i ],
								NULL, NULL, 0, &e ) == LDAP_SUCCESS )
						{
							be_entry_release_r( op, e );
							continue;
						}
		
						if ( MEMBEROF_DANGLING_ERROR( mo ) ) {
							rc = rs->sr_err = mo->mo_dangling_err;
							rs->sr_text = "adding non-existing object "
								"as group member";
							send_ldap_result( op, rs );
							goto done;
						}
		
						if ( MEMBEROF_DANGLING_DROP( mo ) ) {
							int	j;
		
							Debug( LDAP_DEBUG_ANY, "%s: memberof_op_modify(\"%s\"): "
								"member=\"%s\" does not exist (stripping...)\n",
								op->o_log_prefix, op->o_req_dn.bv_val,
								ml->sml_nvalues[ i ].bv_val );
		
							for ( j = i + 1; !BER_BVISNULL( &ml->sml_nvalues[ j ] ); j++ );
							ber_memfree( ml->sml_values[ i ].bv_val );
							BER_BVZERO( &ml->sml_values[ i ] );
							ber_memfree( ml->sml_nvalues[ i ].bv_val );
							BER_BVZERO( &ml->sml_nvalues[ i ] );
							ml->sml_numvals--;
							if ( j - i == 1 ) {
								break;
							}
		
							AC_MEMCPY( &ml->sml_values[ i ], &ml->sml_values[ i + 1 ],
								sizeof( struct berval ) * ( j - i ) );
							AC_MEMCPY( &ml->sml_nvalues[ i ], &ml->sml_nvalues[ i + 1 ],
								sizeof( struct berval ) * ( j - i ) );
							i--;
						}
					}
		
					if ( BER_BVISNULL( &ml->sml_nvalues[ 0 ] ) ) {
						*mlp = ml->sml_next;
						slap_mod_free( &ml->sml_mod, 0 );
						free( ml );
		
					} else {
						mlp = &ml->sml_next;
					}
		
					break;
		
				default:
					assert( 0 );
				}
			}
		}
	}
	
	if ( mmlp != NULL ) {
		Modifications	*ml = *mmlp;
		int		i;
		Entry		*target;

		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		rc = be_entry_get_rw( op, &op->o_req_ndn,
				NULL, NULL, 0, &target );
		op->o_bd->bd_info = (BackendInfo *)on;
		if ( rc != LDAP_SUCCESS ) {
			rc = rs->sr_err = LDAP_NO_SUCH_OBJECT;
			send_ldap_result( op, rs );
			goto done;
		}

		switch ( ml->sml_op ) {
		case LDAP_MOD_DELETE:
		case SLAP_MOD_SOFTDEL: /* ITS#7487: can be used by syncrepl (in mirror mode?) */
			if ( ml->sml_nvalues != NULL ) {
				AccessControlState	acl_state = ACL_STATE_INIT;

				for ( i = 0; !BER_BVISNULL( &ml->sml_nvalues[ i ] ); i++ ) {
					Entry		*e;

					op->o_bd->bd_info = (BackendInfo *)on->on_info;
					/* access is checked with the original identity */
					rc = access_allowed( op, target,
							mo->mo_ad_memberof,
							&ml->sml_nvalues[ i ],
							ACL_WDEL,
							&acl_state );
					if ( rc == 0 ) {
						rc = rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
						rs->sr_text = NULL;
						send_ldap_result( op, rs );
						goto done2;
					}

					rc = be_entry_get_rw( op, &ml->sml_nvalues[ i ],
							NULL, NULL, 0, &e );
					op->o_bd->bd_info = (BackendInfo *)on;
					if ( rc != LDAP_SUCCESS ) {
						if ( get_relax( op ) ) {
							continue;
						}

						if ( MEMBEROF_DANGLING_ERROR( mo ) ) {
							rc = rs->sr_err = mo->mo_dangling_err;
							rs->sr_text = "deleting non-existing object "
								"as memberof";
							send_ldap_result( op, rs );
							goto done2;
						}

						if ( MEMBEROF_DANGLING_DROP( mo ) ) {
							int	j;
	
							Debug( LDAP_DEBUG_ANY, "%s: memberof_op_modify(\"%s\"): "
								"memberof=\"%s\" does not exist (stripping...)\n",
								op->o_log_prefix, op->o_req_ndn.bv_val,
								ml->sml_nvalues[ i ].bv_val );
	
							for ( j = i + 1; !BER_BVISNULL( &ml->sml_nvalues[ j ] ); j++ );
							ber_memfree( ml->sml_values[ i ].bv_val );
							BER_BVZERO( &ml->sml_values[ i ] );
							if ( ml->sml_nvalues != ml->sml_values ) {
								ber_memfree( ml->sml_nvalues[ i ].bv_val );
								BER_BVZERO( &ml->sml_nvalues[ i ] );
							}
							ml->sml_numvals--;
							if ( j - i == 1 ) {
								break;
							}
		
							AC_MEMCPY( &ml->sml_values[ i ], &ml->sml_values[ i + 1 ],
								sizeof( struct berval ) * ( j - i ) );
							if ( ml->sml_nvalues != ml->sml_values ) {
								AC_MEMCPY( &ml->sml_nvalues[ i ], &ml->sml_nvalues[ i + 1 ],
									sizeof( struct berval ) * ( j - i ) );
							}
							i--;
						}

						continue;
					}

					/* access is checked with the original identity */
					op->o_bd->bd_info = (BackendInfo *)on->on_info;
					rc = access_allowed( op, e, mo->mo_ad_member,
							&op->o_req_ndn,
							ACL_WDEL, NULL );
					be_entry_release_r( op, e );
					op->o_bd->bd_info = (BackendInfo *)on;

					if ( !rc ) {
						rc = rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
						rs->sr_text = "insufficient access to object referenced by memberof";
						send_ldap_result( op, rs );
						goto done;
					}
				}

				if ( BER_BVISNULL( &ml->sml_nvalues[ 0 ] ) ) {
					*mmlp = ml->sml_next;
					slap_mod_free( &ml->sml_mod, 0 );
					free( ml );
				}

				break;
			}
			/* fall thru */

		case LDAP_MOD_REPLACE:

			op->o_bd->bd_info = (BackendInfo *)on->on_info;
			/* access is checked with the original identity */
			rc = access_allowed( op, target,
					mo->mo_ad_memberof,
					NULL,
					ACL_WDEL, NULL );
			op->o_bd->bd_info = (BackendInfo *)on;
			if ( rc == 0 ) {
				rc = rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
				rs->sr_text = NULL;
				send_ldap_result( op, rs );
				goto done2;
			}

			if ( ml->sml_op == LDAP_MOD_DELETE || ml->sml_op == SLAP_MOD_SOFTDEL || !ml->sml_values ) {
				break;
			}
			/* fall thru */

		case LDAP_MOD_ADD:
		case SLAP_MOD_SOFTADD: /* ITS#7487 */
		case SLAP_MOD_ADD_IF_NOT_PRESENT: /* ITS#7487 */
			{
			AccessControlState	acl_state = ACL_STATE_INIT;

			for ( i = 0; !BER_BVISNULL( &ml->sml_nvalues[ i ] ); i++ ) {
				Entry		*e;

				op->o_bd->bd_info = (BackendInfo *)on->on_info;
				/* access is checked with the original identity */
				rc = access_allowed( op, target,
						mo->mo_ad_memberof,
						&ml->sml_nvalues[ i ],
						ACL_WADD,
						&acl_state );
				if ( rc == 0 ) {
					rc = rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
					rs->sr_text = NULL;
					send_ldap_result( op, rs );
					goto done2;
				}

				/* ITS#6670 Ignore member pointing to this entry */
				if ( dn_match( &ml->sml_nvalues[i], &save_ndn ))
					continue;

				rc = be_entry_get_rw( op, &ml->sml_nvalues[ i ],
						NULL, NULL, 0, &e );
				op->o_bd->bd_info = (BackendInfo *)on;
				if ( rc != LDAP_SUCCESS ) {
					if ( MEMBEROF_DANGLING_ERROR( mo ) ) {
						rc = rs->sr_err = mo->mo_dangling_err;
						rs->sr_text = "adding non-existing object "
							"as memberof";
						send_ldap_result( op, rs );
						goto done2;
					}

					if ( MEMBEROF_DANGLING_DROP( mo ) ) {
						int	j;

						Debug( LDAP_DEBUG_ANY, "%s: memberof_op_modify(\"%s\"): "
							"memberof=\"%s\" does not exist (stripping...)\n",
							op->o_log_prefix, op->o_req_ndn.bv_val,
							ml->sml_nvalues[ i ].bv_val );

						for ( j = i + 1; !BER_BVISNULL( &ml->sml_nvalues[ j ] ); j++ );
						ber_memfree( ml->sml_values[ i ].bv_val );
						BER_BVZERO( &ml->sml_values[ i ] );
						if ( ml->sml_nvalues != ml->sml_values ) {
							ber_memfree( ml->sml_nvalues[ i ].bv_val );
							BER_BVZERO( &ml->sml_nvalues[ i ] );
						}
						ml->sml_numvals--;
						if ( j - i == 1 ) {
							break;
						}
	
						AC_MEMCPY( &ml->sml_values[ i ], &ml->sml_values[ i + 1 ],
							sizeof( struct berval ) * ( j - i ) );
						if ( ml->sml_nvalues != ml->sml_values ) {
							AC_MEMCPY( &ml->sml_nvalues[ i ], &ml->sml_nvalues[ i + 1 ],
								sizeof( struct berval ) * ( j - i ) );
						}
						i--;
					}

					continue;
				}

				/* access is checked with the original identity */
				op->o_bd->bd_info = (BackendInfo *)on->on_info;
				rc = access_allowed( op, e, mo->mo_ad_member,
						&op->o_req_ndn,
						ACL_WDEL, NULL );
				be_entry_release_r( op, e );
				op->o_bd->bd_info = (BackendInfo *)on;

				if ( !rc ) {
					rc = rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
					rs->sr_text = "insufficient access to object referenced by memberof";
					send_ldap_result( op, rs );
					goto done;
				}
			}

			if ( BER_BVISNULL( &ml->sml_nvalues[ 0 ] ) ) {
				*mmlp = ml->sml_next;
				slap_mod_free( &ml->sml_mod, 0 );
				free( ml );
			}

			} break;

		default:
			assert( 0 );
		}

done2:;
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		be_entry_release_r( op, target );
		op->o_bd->bd_info = (BackendInfo *)on;
	}

	sc = op->o_tmpalloc( sizeof(slap_callback)+sizeof(*mci), op->o_tmpmemctx );
	sc->sc_private = sc+1;
	sc->sc_response = memberof_res_modify;
	sc->sc_cleanup = memberof_cleanup;
	mci = sc->sc_private;
	mci->on = on;
	mci->member = NULL;
	mci->memberof = NULL;
	mci->what = mcis.what;

	if ( save_member ) {
		op->o_dn = op->o_bd->be_rootdn;
		op->o_ndn = op->o_bd->be_rootndn;
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		rc = backend_attribute( op, NULL, &op->o_req_ndn,
				mo->mo_ad_member, &mci->member, ACL_READ );
		op->o_bd->bd_info = (BackendInfo *)on;
	}

	sc->sc_next = op->o_callback;
	op->o_callback = sc;

	rc = SLAP_CB_CONTINUE;

done:;
	op->o_dn = save_dn;
	op->o_ndn = save_ndn;
	op->o_bd->bd_info = (BackendInfo *)on;

	return rc;
}

static int
memberof_op_modrdn( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	slap_callback *sc;
	memberof_cbinfo_t *mci;
	OpExtra		*oex;

	LDAP_SLIST_FOREACH( oex, &op->o_extra, oe_next ) {
		if ( oex->oe_key == (void *)&memberof )
			return SLAP_CB_CONTINUE;
	}

	sc = op->o_tmpalloc( sizeof(slap_callback)+sizeof(*mci), op->o_tmpmemctx );
	sc->sc_private = sc+1;
	sc->sc_response = memberof_res_modrdn;
	sc->sc_cleanup = memberof_cleanup;
	mci = sc->sc_private;
	mci->on = on;
	mci->member = NULL;
	mci->memberof = NULL;

	sc->sc_next = op->o_callback;
	op->o_callback = sc;

	return SLAP_CB_CONTINUE;
}

/*
 * response callback that adds memberof values when a group is added.
 */
static int
memberof_res_add( Operation *op, SlapReply *rs )
{
	memberof_cbinfo_t *mci = op->o_callback->sc_private;
	slap_overinst	*on = mci->on;
	memberof_t	*mo = (memberof_t *)on->on_bi.bi_private;

	int		i;

	if ( rs->sr_err != LDAP_SUCCESS ) {
		return SLAP_CB_CONTINUE;
	}

	if ( MEMBEROF_REVERSE( mo ) ) {
		Attribute	*ma;

		ma = attr_find( op->ora_e->e_attrs, mo->mo_ad_memberof );
		if ( ma != NULL ) {
			/* relax is required to allow to add
			 * a non-existing member */
			op->o_relax = SLAP_CONTROL_CRITICAL;

			for ( i = 0; !BER_BVISNULL( &ma->a_nvals[ i ] ); i++ ) {
		
				/* ITS#6670 Ignore member pointing to this entry */
				if ( dn_match( &ma->a_nvals[i], &op->o_req_ndn ))
					continue;

				/* the modification is attempted
				 * with the original identity */
				memberof_value_modify( op,
					&ma->a_nvals[ i ], mo->mo_ad_member,
					NULL, NULL, &op->o_req_dn, &op->o_req_ndn );
			}
		}
	}

	if ( is_entry_objectclass_or_sub( op->ora_e, mo->mo_oc_group ) ) {
		Attribute	*a;

		for ( a = attrs_find( op->ora_e->e_attrs, mo->mo_ad_member );
				a != NULL;
				a = attrs_find( a->a_next, mo->mo_ad_member ) )
		{
			for ( i = 0; !BER_BVISNULL( &a->a_nvals[ i ] ); i++ ) {
				/* ITS#6670 Ignore member pointing to this entry */
				if ( dn_match( &a->a_nvals[i], &op->o_req_ndn ))
					continue;

				memberof_value_modify( op,
						&a->a_nvals[ i ],
						mo->mo_ad_memberof,
						NULL, NULL,
						&op->o_req_dn,
						&op->o_req_ndn );
			}
		}
	}

	return SLAP_CB_CONTINUE;
}

/*
 * response callback that deletes memberof values when a group is deleted.
 */
static int
memberof_res_delete( Operation *op, SlapReply *rs )
{
	memberof_cbinfo_t *mci = op->o_callback->sc_private;
	slap_overinst	*on = mci->on;
	memberof_t	*mo = (memberof_t *)on->on_bi.bi_private;

 	BerVarray	vals;
	int		i;

	if ( rs->sr_err != LDAP_SUCCESS ) {
		return SLAP_CB_CONTINUE;
	}

	vals = mci->member;
	if ( vals != NULL ) {
		for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
			memberof_value_modify( op,
					&vals[ i ], mo->mo_ad_memberof,
					&op->o_req_dn, &op->o_req_ndn,
					NULL, NULL );
		}
	}

	if ( MEMBEROF_REFINT( mo ) ) {
		vals = mci->memberof;
		if ( vals != NULL ) {
			for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
				memberof_value_modify( op,
						&vals[ i ], mo->mo_ad_member,
						&op->o_req_dn, &op->o_req_ndn,
						NULL, NULL );
			}
		}
	}

	return SLAP_CB_CONTINUE;
}

/*
 * response callback that adds/deletes memberof values when a group
 * is modified.
 */
static int
memberof_res_modify( Operation *op, SlapReply *rs )
{
	memberof_cbinfo_t *mci = op->o_callback->sc_private;
	slap_overinst	*on = mci->on;
	memberof_t	*mo = (memberof_t *)on->on_bi.bi_private;

	int		i, rc;
	Modifications	*ml, *mml = NULL;
	BerVarray	vals;

	if ( rs->sr_err != LDAP_SUCCESS ) {
		return SLAP_CB_CONTINUE;
	}

	if ( MEMBEROF_REVERSE( mo ) ) {
		for ( ml = op->orm_modlist; ml; ml = ml->sml_next ) {
			if ( ml->sml_desc == mo->mo_ad_memberof ) {
				mml = ml;
				break;
			}
		}
	}

	if ( mml != NULL ) {
		BerVarray	vals = mml->sml_nvalues;

		switch ( mml->sml_op ) {
		case LDAP_MOD_DELETE:
		case SLAP_MOD_SOFTDEL: /* ITS#7487: can be used by syncrepl (in mirror mode?) */
			if ( vals != NULL ) {
				for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
					memberof_value_modify( op,
							&vals[ i ], mo->mo_ad_member,
							&op->o_req_dn, &op->o_req_ndn,
							NULL, NULL );
				}
				break;
			}
			/* fall thru */

		case LDAP_MOD_REPLACE:
			/* delete all ... */
			op->o_bd->bd_info = (BackendInfo *)on->on_info;
			rc = backend_attribute( op, NULL, &op->o_req_ndn,
					mo->mo_ad_memberof, &vals, ACL_READ );
			op->o_bd->bd_info = (BackendInfo *)on;
			if ( rc == LDAP_SUCCESS ) {
				for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
					memberof_value_modify( op,
							&vals[ i ], mo->mo_ad_member,
							&op->o_req_dn, &op->o_req_ndn,
							NULL, NULL );
				}
				ber_bvarray_free_x( vals, op->o_tmpmemctx );
			}

			if ( ml->sml_op == LDAP_MOD_DELETE || !mml->sml_values ) {
				break;
			}
			/* fall thru */

		case LDAP_MOD_ADD:
		case SLAP_MOD_SOFTADD: /* ITS#7487 */
		case SLAP_MOD_ADD_IF_NOT_PRESENT: /* ITS#7487 */
			assert( vals != NULL );

			for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
				memberof_value_modify( op,
						&vals[ i ], mo->mo_ad_member,
						NULL, NULL,
						&op->o_req_dn, &op->o_req_ndn );
			}
			break;

		default:
			assert( 0 );
		}
	}

	if ( mci->what & MEMBEROF_IS_GROUP )
	{
		for ( ml = op->orm_modlist; ml; ml = ml->sml_next ) {
			if ( ml->sml_desc != mo->mo_ad_member ) {
				continue;
			}

			switch ( ml->sml_op ) {
			case LDAP_MOD_DELETE:
			case SLAP_MOD_SOFTDEL: /* ITS#7487: can be used by syncrepl (in mirror mode?) */
				vals = ml->sml_nvalues;
				if ( vals != NULL ) {
					for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
						memberof_value_modify( op,
								&vals[ i ], mo->mo_ad_memberof,
								&op->o_req_dn, &op->o_req_ndn,
								NULL, NULL );
					}
					break;
				}
				/* fall thru */
	
			case LDAP_MOD_REPLACE:
				vals = mci->member;

				/* delete all ... */
				if ( vals != NULL ) {
					for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
						memberof_value_modify( op,
								&vals[ i ], mo->mo_ad_memberof,
								&op->o_req_dn, &op->o_req_ndn,
								NULL, NULL );
					}
				}
	
				if ( ml->sml_op == LDAP_MOD_DELETE || ml->sml_op == SLAP_MOD_SOFTDEL || !ml->sml_values ) {
					break;
				}
				/* fall thru */
	
			case LDAP_MOD_ADD:
			case SLAP_MOD_SOFTADD: /* ITS#7487 */
			case SLAP_MOD_ADD_IF_NOT_PRESENT : /* ITS#7487 */
				assert( ml->sml_nvalues != NULL );
				vals = ml->sml_nvalues;
				for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
					memberof_value_modify( op,
							&vals[ i ], mo->mo_ad_memberof,
							NULL, NULL,
							&op->o_req_dn, &op->o_req_ndn );
				}
				break;
	
			default:
				assert( 0 );
			}
		}
	}

	return SLAP_CB_CONTINUE;
}

/*
 * response callback that adds/deletes member values when a group member
 * is renamed.
 */
static int
memberof_res_modrdn( Operation *op, SlapReply *rs )
{
	memberof_cbinfo_t *mci = op->o_callback->sc_private;
	slap_overinst	*on = mci->on;
	memberof_t	*mo = (memberof_t *)on->on_bi.bi_private;

	struct berval	newPDN, newDN = BER_BVNULL, newPNDN, newNDN;
	int		i, rc;
	BerVarray	vals;

	struct berval	save_dn, save_ndn;

	if ( rs->sr_err != LDAP_SUCCESS ) {
		return SLAP_CB_CONTINUE;
	}

	mci->what = MEMBEROF_IS_GROUP;
	if ( MEMBEROF_REFINT( mo ) ) {
		mci->what |= MEMBEROF_IS_MEMBER;
	}

	if ( op->orr_nnewSup ) {
		newPNDN = *op->orr_nnewSup;

	} else {
		dnParent( &op->o_req_ndn, &newPNDN );
	}

	build_new_dn( &newNDN, &newPNDN, &op->orr_nnewrdn, op->o_tmpmemctx ); 

	save_dn = op->o_req_dn;
	save_ndn = op->o_req_ndn;

	op->o_req_dn = newNDN;
	op->o_req_ndn = newNDN;
	rc = memberof_isGroupOrMember( op, mci );
	op->o_req_dn = save_dn;
	op->o_req_ndn = save_ndn;

	if ( rc != LDAP_SUCCESS || mci->what == MEMBEROF_IS_NONE ) {
		goto done;
	}

	if ( op->orr_newSup ) {
		newPDN = *op->orr_newSup;

	} else {
		dnParent( &op->o_req_dn, &newPDN );
	}

	build_new_dn( &newDN, &newPDN, &op->orr_newrdn, op->o_tmpmemctx ); 

	if ( mci->what & MEMBEROF_IS_GROUP ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		rc = backend_attribute( op, NULL, &newNDN,
				mo->mo_ad_member, &vals, ACL_READ );
		op->o_bd->bd_info = (BackendInfo *)on;

		if ( rc == LDAP_SUCCESS ) {
			for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
				memberof_value_modify( op,
						&vals[ i ], mo->mo_ad_memberof,
						&op->o_req_dn, &op->o_req_ndn,
						&newDN, &newNDN );
			}
			ber_bvarray_free_x( vals, op->o_tmpmemctx );
		}
	}

	if ( MEMBEROF_REFINT( mo ) && ( mci->what & MEMBEROF_IS_MEMBER ) ) {
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		rc = backend_attribute( op, NULL, &newNDN,
				mo->mo_ad_memberof, &vals, ACL_READ );
		op->o_bd->bd_info = (BackendInfo *)on;

		if ( rc == LDAP_SUCCESS ) {
			for ( i = 0; !BER_BVISNULL( &vals[ i ] ); i++ ) {
				memberof_value_modify( op,
						&vals[ i ], mo->mo_ad_member,
						&op->o_req_dn, &op->o_req_ndn,
						&newDN, &newNDN );
			}
			ber_bvarray_free_x( vals, op->o_tmpmemctx );
		}
	}

done:;
	if ( !BER_BVISNULL( &newDN ) ) {
		op->o_tmpfree( newDN.bv_val, op->o_tmpmemctx );
	}
	op->o_tmpfree( newNDN.bv_val, op->o_tmpmemctx );

	return SLAP_CB_CONTINUE;
}


static int
memberof_db_init(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	memberof_t		*mo;

	mo = (memberof_t *)ch_calloc( 1, sizeof( memberof_t ) );

	/* safe default */
	mo->mo_dangling_err = LDAP_CONSTRAINT_VIOLATION;

	on->on_bi.bi_private = (void *)mo;

	return 0;
}

enum {
	MO_DN = 1,
	MO_DANGLING,
	MO_REFINT,
	MO_GROUP_OC,
	MO_MEMBER_AD,
	MO_MEMBER_OF_AD,
#if 0
	MO_REVERSE,
#endif

	MO_DANGLING_ERROR,

	MO_LAST
};

static ConfigDriver mo_cf_gen;

#define OID		"1.3.6.1.4.1.7136.2.666.4"
#define	OIDAT		OID ".1.1"
#define	OIDCFGAT	OID ".1.2"
#define	OIDOC		OID ".2.1"
#define	OIDCFGOC	OID ".2.2"


static ConfigTable mo_cfg[] = {
	{ "memberof-dn", "modifiersName",
		2, 2, 0, ARG_MAGIC|ARG_DN|MO_DN, mo_cf_gen,
		"( OLcfgOvAt:18.0 NAME 'olcMemberOfDN' "
			"DESC 'DN to be used as modifiersName' "
			"SYNTAX OMsDN SINGLE-VALUE )",
		NULL, NULL },

	{ "memberof-dangling", "ignore|drop|error",
		2, 2, 0, ARG_MAGIC|MO_DANGLING, mo_cf_gen,
		"( OLcfgOvAt:18.1 NAME 'olcMemberOfDangling' "
			"DESC 'Behavior with respect to dangling members, "
				"constrained to ignore, drop, error' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
		NULL, NULL },

	{ "memberof-refint", "true|FALSE",
		2, 2, 0, ARG_MAGIC|ARG_ON_OFF|MO_REFINT, mo_cf_gen,
		"( OLcfgOvAt:18.2 NAME 'olcMemberOfRefInt' "
			"DESC 'Take care of referential integrity' "
			"SYNTAX OMsBoolean SINGLE-VALUE )",
		NULL, NULL },

	{ "memberof-group-oc", "objectClass",
		2, 2, 0, ARG_MAGIC|MO_GROUP_OC, mo_cf_gen,
		"( OLcfgOvAt:18.3 NAME 'olcMemberOfGroupOC' "
			"DESC 'Group objectClass' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
		NULL, NULL },

	{ "memberof-member-ad", "member attribute",
		2, 2, 0, ARG_MAGIC|MO_MEMBER_AD, mo_cf_gen,
		"( OLcfgOvAt:18.4 NAME 'olcMemberOfMemberAD' "
			"DESC 'member attribute' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
		NULL, NULL },

	{ "memberof-memberof-ad", "memberOf attribute",
		2, 2, 0, ARG_MAGIC|MO_MEMBER_OF_AD, mo_cf_gen,
		"( OLcfgOvAt:18.5 NAME 'olcMemberOfMemberOfAD' "
			"DESC 'memberOf attribute' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
		NULL, NULL },

#if 0
	{ "memberof-reverse", "true|FALSE",
		2, 2, 0, ARG_MAGIC|ARG_ON_OFF|MO_REVERSE, mo_cf_gen,
		"( OLcfgOvAt:18.6 NAME 'olcMemberOfReverse' "
			"DESC 'Take care of referential integrity "
				"also when directly modifying memberOf' "
			"SYNTAX OMsBoolean SINGLE-VALUE )",
		NULL, NULL },
#endif

	{ "memberof-dangling-error", "error code",
		2, 2, 0, ARG_MAGIC|MO_DANGLING_ERROR, mo_cf_gen,
		"( OLcfgOvAt:18.7 NAME 'olcMemberOfDanglingError' "
			"DESC 'Error code returned in case of dangling back reference' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
		NULL, NULL },

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs mo_ocs[] = {
	{ "( OLcfgOvOc:18.1 "
		"NAME 'olcMemberOf' "
		"DESC 'Member-of configuration' "
		"SUP olcOverlayConfig "
		"MAY ( "
			"olcMemberOfDN "
			"$ olcMemberOfDangling "
			"$ olcMemberOfDanglingError"
			"$ olcMemberOfRefInt "
			"$ olcMemberOfGroupOC "
			"$ olcMemberOfMemberAD "
			"$ olcMemberOfMemberOfAD "
#if 0
			"$ olcMemberOfReverse "
#endif
			") "
		")",
		Cft_Overlay, mo_cfg, NULL, NULL },
	{ NULL, 0, NULL }
};

static slap_verbmasks dangling_mode[] = {
	{ BER_BVC( "ignore" ),		MEMBEROF_NONE },
	{ BER_BVC( "drop" ),		MEMBEROF_FDANGLING_DROP },
	{ BER_BVC( "error" ),		MEMBEROF_FDANGLING_ERROR },
	{ BER_BVNULL,			0 }
};

static int
memberof_make_group_filter( memberof_t *mo )
{
	char		*ptr;

	if ( !BER_BVISNULL( &mo->mo_groupFilterstr ) ) {
		ch_free( mo->mo_groupFilterstr.bv_val );
	}

	mo->mo_groupFilter.f_choice = LDAP_FILTER_EQUALITY;
	mo->mo_groupFilter.f_ava = &mo->mo_groupAVA;
	
	mo->mo_groupFilter.f_av_desc = slap_schema.si_ad_objectClass;
	mo->mo_groupFilter.f_av_value = mo->mo_oc_group->soc_cname;

	mo->mo_groupFilterstr.bv_len = STRLENOF( "(=)" )
		+ slap_schema.si_ad_objectClass->ad_cname.bv_len
		+ mo->mo_oc_group->soc_cname.bv_len;
	ptr = mo->mo_groupFilterstr.bv_val = ch_malloc( mo->mo_groupFilterstr.bv_len + 1 );
	*ptr++ = '(';
	ptr = lutil_strcopy( ptr, slap_schema.si_ad_objectClass->ad_cname.bv_val );
	*ptr++ = '=';
	ptr = lutil_strcopy( ptr, mo->mo_oc_group->soc_cname.bv_val );
	*ptr++ = ')';
	*ptr = '\0';

	return 0;
}

static int
memberof_make_member_filter( memberof_t *mo )
{
	char		*ptr;

	if ( !BER_BVISNULL( &mo->mo_memberFilterstr ) ) {
		ch_free( mo->mo_memberFilterstr.bv_val );
	}

	mo->mo_memberFilter.f_choice = LDAP_FILTER_PRESENT;
	mo->mo_memberFilter.f_desc = mo->mo_ad_memberof;

	mo->mo_memberFilterstr.bv_len = STRLENOF( "(=*)" )
		+ mo->mo_ad_memberof->ad_cname.bv_len;
	ptr = mo->mo_memberFilterstr.bv_val = ch_malloc( mo->mo_memberFilterstr.bv_len + 1 );
	*ptr++ = '(';
	ptr = lutil_strcopy( ptr, mo->mo_ad_memberof->ad_cname.bv_val );
	ptr = lutil_strcopy( ptr, "=*)" );

	return 0;
}

static int
mo_cf_gen( ConfigArgs *c )
{
	slap_overinst	*on = (slap_overinst *)c->bi;
	memberof_t	*mo = (memberof_t *)on->on_bi.bi_private;

	int		i, rc = 0;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		struct berval bv = BER_BVNULL;

		switch( c->type ) {
		case MO_DN:
			if ( mo->mo_dn.bv_val != NULL) {
				value_add_one( &c->rvalue_vals, &mo->mo_dn );
				value_add_one( &c->rvalue_nvals, &mo->mo_ndn );
			}
			break;

		case MO_DANGLING:
			enum_to_verb( dangling_mode, (mo->mo_flags & MEMBEROF_FDANGLING_MASK), &bv );
			if ( BER_BVISNULL( &bv ) ) {
				/* there's something wrong... */
				assert( 0 );
				rc = 1;

			} else {
				value_add_one( &c->rvalue_vals, &bv );
			}
			break;

		case MO_DANGLING_ERROR:
			if ( mo->mo_flags & MEMBEROF_FDANGLING_ERROR ) {
				char buf[ SLAP_TEXT_BUFLEN ];
				enum_to_verb( slap_ldap_response_code, mo->mo_dangling_err, &bv );
				if ( BER_BVISNULL( &bv ) ) {
					bv.bv_len = snprintf( buf, sizeof( buf ), "0x%x", mo->mo_dangling_err );
					if ( bv.bv_len < sizeof( buf ) ) {
						bv.bv_val = buf;
					} else {
						rc = 1;
						break;
					}
				}
				value_add_one( &c->rvalue_vals, &bv );
			} else {
				rc = 1;
			}
			break;

		case MO_REFINT:
			c->value_int = MEMBEROF_REFINT( mo );
			break;

#if 0
		case MO_REVERSE:
			c->value_int = MEMBEROF_REVERSE( mo );
			break;
#endif

		case MO_GROUP_OC:
			if ( mo->mo_oc_group != NULL ){
				value_add_one( &c->rvalue_vals, &mo->mo_oc_group->soc_cname );
			}
			break;

		case MO_MEMBER_AD:
			if ( mo->mo_ad_member != NULL ){
				value_add_one( &c->rvalue_vals, &mo->mo_ad_member->ad_cname );
			}
			break;

		case MO_MEMBER_OF_AD:
			if ( mo->mo_ad_memberof != NULL ){
				value_add_one( &c->rvalue_vals, &mo->mo_ad_memberof->ad_cname );
			}
			break;

		default:
			assert( 0 );
			return 1;
		}

		return rc;

	} else if ( c->op == LDAP_MOD_DELETE ) {
		return 1;	/* FIXME */

	} else {
		switch( c->type ) {
		case MO_DN:
			if ( !BER_BVISNULL( &mo->mo_dn ) ) {
				ber_memfree( mo->mo_dn.bv_val );
				ber_memfree( mo->mo_ndn.bv_val );
			}
			mo->mo_dn = c->value_dn;
			mo->mo_ndn = c->value_ndn;
			break;

		case MO_DANGLING:
			i = verb_to_mask( c->argv[ 1 ], dangling_mode );
			if ( BER_BVISNULL( &dangling_mode[ i ].word ) ) {
				return 1;
			}

			mo->mo_flags &= ~MEMBEROF_FDANGLING_MASK;
			mo->mo_flags |= dangling_mode[ i ].mask;
			break;

		case MO_DANGLING_ERROR:
			i = verb_to_mask( c->argv[ 1 ], slap_ldap_response_code );
			if ( !BER_BVISNULL( &slap_ldap_response_code[ i ].word ) ) {
				mo->mo_dangling_err = slap_ldap_response_code[ i ].mask;
			} else if ( lutil_atoix( &mo->mo_dangling_err, c->argv[ 1 ], 0 ) ) {
				return 1;
			}
			break;

		case MO_REFINT:
			if ( c->value_int ) {
				mo->mo_flags |= MEMBEROF_FREFINT;

			} else {
				mo->mo_flags &= ~MEMBEROF_FREFINT;
			}
			break;

#if 0
		case MO_REVERSE:
			if ( c->value_int ) {
				mo->mo_flags |= MEMBEROF_FREVERSE;

			} else {
				mo->mo_flags &= ~MEMBEROF_FREVERSE;
			}
			break;
#endif

		case MO_GROUP_OC: {
			ObjectClass	*oc = NULL;

			oc = oc_find( c->argv[ 1 ] );
			if ( oc == NULL ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to find group objectClass=\"%s\"",
					c->argv[ 1 ] );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
				return 1;
			}

			mo->mo_oc_group = oc;
			memberof_make_group_filter( mo );
			} break;

		case MO_MEMBER_AD: {
			AttributeDescription	*ad = NULL;
			const char		*text = NULL;


			rc = slap_str2ad( c->argv[ 1 ], &ad, &text );
			if ( rc != LDAP_SUCCESS ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to find member attribute=\"%s\": %s (%d)",
					c->argv[ 1 ], text, rc );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
				return 1;
			}

			if ( !is_at_syntax( ad->ad_type, SLAPD_DN_SYNTAX )		/* e.g. "member" */
				&& !is_at_syntax( ad->ad_type, SLAPD_NAMEUID_SYNTAX ) )	/* e.g. "uniqueMember" */
			{
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"member attribute=\"%s\" must either "
					"have DN (%s) or nameUID (%s) syntax",
					c->argv[ 1 ], SLAPD_DN_SYNTAX, SLAPD_NAMEUID_SYNTAX );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
				return 1;
			}

			mo->mo_ad_member = ad;
			} break;

		case MO_MEMBER_OF_AD: {
			AttributeDescription	*ad = NULL;
			const char		*text = NULL;


			rc = slap_str2ad( c->argv[ 1 ], &ad, &text );
			if ( rc != LDAP_SUCCESS ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"unable to find memberof attribute=\"%s\": %s (%d)",
					c->argv[ 1 ], text, rc );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
				return 1;
			}

			if ( !is_at_syntax( ad->ad_type, SLAPD_DN_SYNTAX )		/* e.g. "member" */
				&& !is_at_syntax( ad->ad_type, SLAPD_NAMEUID_SYNTAX ) )	/* e.g. "uniqueMember" */
			{
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"memberof attribute=\"%s\" must either "
					"have DN (%s) or nameUID (%s) syntax",
					c->argv[ 1 ], SLAPD_DN_SYNTAX, SLAPD_NAMEUID_SYNTAX );
				Debug( LDAP_DEBUG_CONFIG, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
				return 1;
			}

			mo->mo_ad_memberof = ad;
			memberof_make_member_filter( mo );
			} break;

		default:
			assert( 0 );
			return 1;
		}
	}

	return 0;
}

static int
memberof_db_open(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	memberof_t	*mo = (memberof_t *)on->on_bi.bi_private;
	
	int		rc;
	const char	*text = NULL;

	if( ! mo->mo_ad_memberof ){
		rc = slap_str2ad( SLAPD_MEMBEROF_ATTR, &mo->mo_ad_memberof, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "memberof_db_open: "
					"unable to find attribute=\"%s\": %s (%d)\n",
					SLAPD_MEMBEROF_ATTR, text, rc );
			return rc;
		}
	}

	if( ! mo->mo_ad_member ){
		rc = slap_str2ad( SLAPD_GROUP_ATTR, &mo->mo_ad_member, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "memberof_db_open: "
					"unable to find attribute=\"%s\": %s (%d)\n",
					SLAPD_GROUP_ATTR, text, rc );
			return rc;
		}
	}

	if( ! mo->mo_oc_group ){
		mo->mo_oc_group = oc_find( SLAPD_GROUP_CLASS );
		if ( mo->mo_oc_group == NULL ) {
			Debug( LDAP_DEBUG_ANY,
					"memberof_db_open: "
					"unable to find objectClass=\"%s\"\n",
					SLAPD_GROUP_CLASS, 0, 0 );
			return 1;
		}
	}

	if ( BER_BVISNULL( &mo->mo_dn ) && !BER_BVISNULL( &be->be_rootdn ) ) {
		ber_dupbv( &mo->mo_dn, &be->be_rootdn );
		ber_dupbv( &mo->mo_ndn, &be->be_rootndn );
	}

	if ( BER_BVISNULL( &mo->mo_groupFilterstr ) ) {
		memberof_make_group_filter( mo );
	}

	if ( BER_BVISNULL( &mo->mo_memberFilterstr ) ) {
		memberof_make_member_filter( mo );
	}

	return 0;
}

static int
memberof_db_destroy(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	memberof_t	*mo = (memberof_t *)on->on_bi.bi_private;

	if ( mo ) {
		if ( !BER_BVISNULL( &mo->mo_dn ) ) {
			ber_memfree( mo->mo_dn.bv_val );
			ber_memfree( mo->mo_ndn.bv_val );
		}

		if ( !BER_BVISNULL( &mo->mo_groupFilterstr ) ) {
			ber_memfree( mo->mo_groupFilterstr.bv_val );
		}

		if ( !BER_BVISNULL( &mo->mo_memberFilterstr ) ) {
			ber_memfree( mo->mo_memberFilterstr.bv_val );
		}

		ber_memfree( mo );
	}

	return 0;
}

/* unused */
static AttributeDescription	*ad_memberOf;

static struct {
	char	*desc;
	AttributeDescription **adp;
} as[] = {
	{ "( 1.2.840.113556.1.2.102 "
		"NAME 'memberOf' "
		"DESC 'Group that the entry belongs to' "
		"SYNTAX '1.3.6.1.4.1.1466.115.121.1.12' "
		"EQUALITY distinguishedNameMatch "	/* added */
		"USAGE dSAOperation "			/* added; questioned */
		/* "NO-USER-MODIFICATION " */		/* add? */
		"X-ORIGIN 'iPlanet Delegated Administrator' )",
		&ad_memberOf },
	{ NULL }
};

#if SLAPD_OVER_MEMBEROF == SLAPD_MOD_DYNAMIC
static
#endif /* SLAPD_OVER_MEMBEROF == SLAPD_MOD_DYNAMIC */
int
memberof_initialize( void )
{
	int			code, i;

	for ( i = 0; as[ i ].desc != NULL; i++ ) {
		code = register_at( as[ i ].desc, as[ i ].adp, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"memberof_initialize: register_at #%d failed\n",
				i, 0, 0 );
			return code;
		}
	}

	memberof.on_bi.bi_type = "memberof";

	memberof.on_bi.bi_db_init = memberof_db_init;
	memberof.on_bi.bi_db_open = memberof_db_open;
	memberof.on_bi.bi_db_destroy = memberof_db_destroy;

	memberof.on_bi.bi_op_add = memberof_op_add;
	memberof.on_bi.bi_op_delete = memberof_op_delete;
	memberof.on_bi.bi_op_modify = memberof_op_modify;
	memberof.on_bi.bi_op_modrdn = memberof_op_modrdn;

	memberof.on_bi.bi_cf_ocs = mo_ocs;

	code = config_register_schema( mo_cfg, mo_ocs );
	if ( code ) return code;

	return overlay_register( &memberof );
}

#if SLAPD_OVER_MEMBEROF == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return memberof_initialize();
}
#endif /* SLAPD_OVER_MEMBEROF == SLAPD_MOD_DYNAMIC */

#endif /* SLAPD_OVER_MEMBEROF */
