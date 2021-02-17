/* autogroup.c - automatic group overlay */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2007-2014 The OpenLDAP Foundation.
 * Portions Copyright 2007 Michał Szulczyński.
 * Portions Copyright 2009 Howard Chu.
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
 * This work was initially developed by Michał Szulczyński for inclusion in
 * OpenLDAP Software.  Additional significant contributors include:
 *   Howard Chu
 *   Raphael Ouazana
 *   Norbert Pueschel
 *   Christian Manal
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include "config.h"
#include "lutil.h"

#ifndef SLAPD_MEMBEROF_ATTR
#define	SLAPD_MEMBEROF_ATTR	"memberOf"
#endif

/* Filter represents the memberURL of a group. */
typedef struct autogroup_filter_t {
	struct berval			agf_dn;	/* The base DN in memberURL */
	struct berval			agf_ndn;
	struct berval			agf_filterstr;
	Filter				*agf_filter;
	int				agf_scope;
	AttributeName			*agf_anlist;
	struct autogroup_filter_t	*agf_next;
} autogroup_filter_t;

/* Description of group attributes. */
typedef struct autogroup_def_t {
	ObjectClass		*agd_oc;
	AttributeDescription	*agd_member_url_ad;
	AttributeDescription	*agd_member_ad;
	struct autogroup_def_t	*agd_next;
} autogroup_def_t;

/* Represents the group entry. */
typedef struct autogroup_entry_t {
	BerValue		age_dn;
	BerValue		age_ndn;
	autogroup_filter_t	*age_filter; /* List of filters made from memberURLs */
	autogroup_def_t		*age_def; /* Attribute definition */
	ldap_pvt_thread_mutex_t age_mutex;
	int			age_mustrefresh; /* Defined in request to refresh in response */
	int			age_modrdn_olddnmodified; /* Defined in request to refresh in response */
	struct autogroup_entry_t	*age_next;
} autogroup_entry_t;

/* Holds pointers to attribute definitions and groups. */
typedef struct autogroup_info_t {
	autogroup_def_t		*agi_def;	/* Group attributes definitions. */
	autogroup_entry_t	*agi_entry;	/* Group entries.  */
	AttributeDescription	*agi_memberof_ad;	/* memberOf attribute description  */
	ldap_pvt_thread_mutex_t agi_mutex;
} autogroup_info_t;

/* Search callback for adding groups initially. */
typedef struct autogroup_sc_t {
	autogroup_info_t		*ags_info;	/* Group definitions and entries.  */
	autogroup_def_t		*ags_def;	/* Attributes definition of the group being added. */
} autogroup_sc_t;

/* Used for adding members, found when searching, to a group. */
typedef struct autogroup_ga_t {
	autogroup_entry_t	*agg_group;	/* The group to which the members will be added. */
	autogroup_filter_t	*agg_filter;	/* Current filter */
	Entry			*agg_entry;	/* Used in autogroup_member_search_cb to modify 
						this entry with the search results. */

	Modifications		*agg_mod;	/* Used in autogroup_member_search_modify_cb to hold the 
						search results which will be added to the group. */

	Modifications		*agg_mod_last; /* Used in autogroup_member_search_modify_cb so we don't 
						have to search for the last mod added. */
} autogroup_ga_t;


/* 
**	dn, ndn	- the DN of the member to add
**	age	- the group to which the member DN will be added
*/
static int
autogroup_add_member_to_group( Operation *op, BerValue *dn, BerValue *ndn, autogroup_entry_t *age )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	Modifications	*modlist = (Modifications *)ch_calloc( 1, sizeof( Modifications ) );
	SlapReply	sreply = {REP_RESULT};
	BerValue	*vals, *nvals;
	slap_callback	cb = { NULL, slap_null_cb, NULL, NULL };
	Operation	o = *op;

	assert( dn != NULL );
	assert( ndn != NULL );
	Debug(LDAP_DEBUG_TRACE, "==> autogroup_add_member_to_group adding <%s> to <%s>\n",
		dn->bv_val, age->age_dn.bv_val, 0);

	vals = (BerValue *)ch_calloc( 2, sizeof( BerValue ) );
	nvals = (BerValue *)ch_calloc( 2, sizeof( BerValue ) );
	ber_dupbv( vals, dn );
	BER_BVZERO( &vals[ 1 ] );
	ber_dupbv( nvals, ndn );
	BER_BVZERO( &nvals[ 1 ] );

	modlist->sml_op = LDAP_MOD_ADD;
	modlist->sml_desc = age->age_def->agd_member_ad;
	modlist->sml_type = age->age_def->agd_member_ad->ad_cname;
	modlist->sml_values = vals;
	modlist->sml_nvalues = nvals;
	modlist->sml_numvals = 1;
	modlist->sml_flags = SLAP_MOD_INTERNAL;
	modlist->sml_next = NULL;

	o.o_tag = LDAP_REQ_MODIFY;
	o.o_callback = &cb;
	o.orm_modlist = modlist;
	o.o_req_dn = age->age_dn;
	o.o_req_ndn = age->age_ndn;
	o.o_permissive_modify = 1;
	o.o_managedsait = SLAP_CONTROL_CRITICAL;
	o.o_relax = SLAP_CONTROL_CRITICAL;

	o.o_bd->bd_info = (BackendInfo *)on->on_info;
	(void)op->o_bd->be_modify( &o, &sreply );
	o.o_bd->bd_info = (BackendInfo *)on;

	slap_mods_free( modlist, 1 );

	return sreply.sr_err;
}

/* 
**	e	- the entry where to get the attribute values
**	age	- the group to which the values will be added
*/
static int
autogroup_add_member_values_to_group( Operation *op, Entry *e, autogroup_entry_t *age, AttributeDescription *attrdesc )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	Modifications	modlist;
	SlapReply	sreply = {REP_RESULT};
	Attribute	*attr;
	slap_callback	cb = { NULL, slap_null_cb, NULL, NULL };
	Operation	o = *op;

	assert( e != NULL );
	Debug(LDAP_DEBUG_TRACE, "==> autogroup_add_member_values_to_group adding <%s> to <%s>\n",
		e->e_name.bv_val, age->age_dn.bv_val, 0);

	attr = attrs_find( e->e_attrs, attrdesc );
	if (!attr) {
		// Nothing to add
		return LDAP_SUCCESS;
	}

	modlist.sml_op = LDAP_MOD_ADD;
	modlist.sml_desc = age->age_def->agd_member_ad;
	modlist.sml_type = age->age_def->agd_member_ad->ad_cname;
	modlist.sml_values = attr->a_vals;
	modlist.sml_nvalues = attr->a_nvals;
	modlist.sml_numvals = attr->a_numvals;
	modlist.sml_flags = SLAP_MOD_INTERNAL;
	modlist.sml_next = NULL;

	o.o_tag = LDAP_REQ_MODIFY;
	o.o_callback = &cb;
	o.orm_modlist = &modlist;
	o.o_req_dn = age->age_dn;
	o.o_req_ndn = age->age_ndn;
	o.o_permissive_modify = 1;
	o.o_managedsait = SLAP_CONTROL_CRITICAL;
	o.o_relax = SLAP_CONTROL_CRITICAL;

	o.o_bd->bd_info = (BackendInfo *)on->on_info;
	(void)op->o_bd->be_modify( &o, &sreply );
	o.o_bd->bd_info = (BackendInfo *)on;

	return sreply.sr_err;
}

/*
** dn,ndn	- the DN to be deleted
** age		- the group from which the DN will be deleted
** If we pass a NULL dn and ndn, all members are deleted from the group. 
*/
static int
autogroup_delete_member_from_group( Operation *op, BerValue *dn, BerValue *ndn, autogroup_entry_t *age )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	Modifications	*modlist = (Modifications *)ch_calloc( 1, sizeof( Modifications ) );
	SlapReply	sreply = {REP_RESULT};
	BerValue	*vals, *nvals;
	slap_callback	cb = { NULL, slap_null_cb, NULL, NULL };
	Operation	o = *op;

	if ( dn == NULL || ndn == NULL ) {
		Debug(LDAP_DEBUG_TRACE, "==> autogroup_delete_member_from_group removing all members from <%s>\n",
			age->age_dn.bv_val, 0 ,0);

		modlist->sml_values = NULL;
		modlist->sml_nvalues = NULL;
		modlist->sml_numvals = 0;
	} else {
		Debug(LDAP_DEBUG_TRACE, "==> autogroup_delete_member_from_group removing <%s> from <%s>\n",
			dn->bv_val, age->age_dn.bv_val, 0);

		vals = (BerValue *)ch_calloc( 2, sizeof( BerValue ) );
		nvals = (BerValue *)ch_calloc( 2, sizeof( BerValue ) );
		ber_dupbv( vals, dn );
		BER_BVZERO( &vals[ 1 ] );
		ber_dupbv( nvals, ndn );
		BER_BVZERO( &nvals[ 1 ] );

		modlist->sml_values = vals;
		modlist->sml_nvalues = nvals;
		modlist->sml_numvals = 1;
	}


	modlist->sml_op = LDAP_MOD_DELETE;
	modlist->sml_desc = age->age_def->agd_member_ad;
	modlist->sml_type = age->age_def->agd_member_ad->ad_cname;
	modlist->sml_flags = SLAP_MOD_INTERNAL;
	modlist->sml_next = NULL;

	o.o_callback = &cb;
	o.o_tag = LDAP_REQ_MODIFY;
	o.orm_modlist = modlist;
	o.o_req_dn = age->age_dn;
	o.o_req_ndn = age->age_ndn;
	o.o_relax = SLAP_CONTROL_CRITICAL;
	o.o_managedsait = SLAP_CONTROL_CRITICAL;
	o.o_permissive_modify = 1;

	o.o_bd->bd_info = (BackendInfo *)on->on_info;
	(void)op->o_bd->be_modify( &o, &sreply );
	o.o_bd->bd_info = (BackendInfo *)on;

	slap_mods_free( modlist, 1 );

	return sreply.sr_err;
}

/*
**      e       - the entry where to get the attribute values
**      age     - the group from which the values will be deleted
*/
static int
autogroup_delete_member_values_from_group( Operation *op, Entry *e, autogroup_entry_t *age, AttributeDescription *attrdesc )
{
        slap_overinst   *on = (slap_overinst *)op->o_bd->bd_info;
        Modifications   modlist;
        SlapReply       sreply = {REP_RESULT};
        Attribute       *attr;
        slap_callback   cb = { NULL, slap_null_cb, NULL, NULL };
        Operation       o = *op;

        assert( e != NULL );
        Debug(LDAP_DEBUG_TRACE, "==> autogroup_delete_member_values_from_group removing <%s> from <%s>\n",
                e->e_name.bv_val, age->age_dn.bv_val, 0);

        attr = attrs_find( e->e_attrs, attrdesc );
        if (!attr) {
                // Nothing to add
                return LDAP_SUCCESS;
        }

        modlist.sml_op = LDAP_MOD_DELETE;
        modlist.sml_desc = age->age_def->agd_member_ad;
        modlist.sml_type = age->age_def->agd_member_ad->ad_cname;
        modlist.sml_values = attr->a_vals;
        modlist.sml_nvalues = attr->a_nvals;
        modlist.sml_numvals = attr->a_numvals;
        modlist.sml_flags = SLAP_MOD_INTERNAL;
        modlist.sml_next = NULL;

        o.o_tag = LDAP_REQ_MODIFY;
        o.o_callback = &cb;
        o.orm_modlist = &modlist;
        o.o_req_dn = age->age_dn;
        o.o_req_ndn = age->age_ndn;
        o.o_permissive_modify = 1;
        o.o_managedsait = SLAP_CONTROL_CRITICAL;
        o.o_relax = SLAP_CONTROL_CRITICAL;

        o.o_bd->bd_info = (BackendInfo *)on->on_info;
        (void)op->o_bd->be_modify( &o, &sreply );
        o.o_bd->bd_info = (BackendInfo *)on;

        return sreply.sr_err;
}

/* 
** Callback used to add entries to a group, 
** which are going to be written in the database
** (used in bi_op_add)
** The group is passed in autogroup_ga_t->agg_group
*/
static int
autogroup_member_search_cb( Operation *op, SlapReply *rs )
{
	assert( op->o_tag == LDAP_REQ_SEARCH );

	if ( rs->sr_type == REP_SEARCH ) {
		autogroup_ga_t		*agg = (autogroup_ga_t *)op->o_callback->sc_private;
		autogroup_entry_t	*age = agg->agg_group;
		autogroup_filter_t	*agf = agg->agg_filter;
		Modification		mod;
		const char		*text = NULL;
		char			textbuf[1024];
		struct berval		*vals, *nvals;
		struct berval		lvals[ 2 ], lnvals[ 2 ];
		int			numvals;

		Debug(LDAP_DEBUG_TRACE, "==> autogroup_member_search_cb <%s>\n",
			rs->sr_entry ? rs->sr_entry->e_name.bv_val : "UNKNOWN_DN", 0, 0);

		if ( agf->agf_anlist ) {
			Attribute *attr = attrs_find( rs->sr_entry->e_attrs, agf->agf_anlist[0].an_desc );
			if (attr) {
				vals = attr->a_vals;
				nvals = attr->a_nvals;
				numvals = attr->a_numvals;
			} else {
				// Nothing to add
				return 0;
			}
		} else {
			lvals[ 0 ] = rs->sr_entry->e_name;
			BER_BVZERO( &lvals[ 1 ] );
			lnvals[ 0 ] = rs->sr_entry->e_nname;
			BER_BVZERO( &lnvals[ 1 ] );
			vals = lvals;
			nvals = lnvals;
			numvals = 1;
		}

		mod.sm_op = LDAP_MOD_ADD;
		mod.sm_desc = age->age_def->agd_member_ad;
		mod.sm_type = age->age_def->agd_member_ad->ad_cname;
		mod.sm_values = vals;
		mod.sm_nvalues = nvals;
		mod.sm_numvals = numvals;

		modify_add_values( agg->agg_entry, &mod, /* permissive */ 1, &text, textbuf, sizeof( textbuf ) );
	}

	return 0;
}

/* 
** Callback used to add entries to a group, which is already in the database.
** (used in on_response)
** The group is passed in autogroup_ga_t->agg_group
** NOTE: Very slow.
*/
static int
autogroup_member_search_modify_cb( Operation *op, SlapReply *rs )
{
	assert( op->o_tag == LDAP_REQ_SEARCH );

	if ( rs->sr_type == REP_SEARCH ) {
		autogroup_ga_t		*agg = (autogroup_ga_t *)op->o_callback->sc_private;
		autogroup_entry_t	*age = agg->agg_group;
		autogroup_filter_t	*agf = agg->agg_filter;
		Modifications		*modlist;
		struct berval		*vals, *nvals;
		struct berval		lvals[ 2 ], lnvals[ 2 ];
		int			numvals;

		Debug(LDAP_DEBUG_TRACE, "==> autogroup_member_search_modify_cb <%s>\n",
			rs->sr_entry ? rs->sr_entry->e_name.bv_val : "UNKNOWN_DN", 0, 0);

		if ( agf->agf_anlist ) {
			Attribute *attr = attrs_find( rs->sr_entry->e_attrs, agf->agf_anlist[0].an_desc );
			if (attr) {
				vals = attr->a_vals;
				nvals = attr->a_nvals;
				numvals = attr->a_numvals;
			} else {
				// Nothing to add
				return 0;
			}
		} else {
			lvals[ 0 ] = rs->sr_entry->e_name;
			BER_BVZERO( &lvals[ 1 ] );
			lnvals[ 0 ] = rs->sr_entry->e_nname;
			BER_BVZERO( &lnvals[ 1 ] );
			vals = lvals;
			nvals = lnvals;
			numvals = 1;
		}

		if ( numvals ) {
			modlist = (Modifications *)ch_calloc( 1, sizeof( Modifications ) );

			modlist->sml_op = LDAP_MOD_ADD;
			modlist->sml_desc = age->age_def->agd_member_ad;
			modlist->sml_type = age->age_def->agd_member_ad->ad_cname;

			ber_bvarray_dup_x( &modlist->sml_values, vals, NULL );
			ber_bvarray_dup_x( &modlist->sml_nvalues, nvals, NULL );
			modlist->sml_numvals = numvals;

			modlist->sml_flags = SLAP_MOD_INTERNAL;
			modlist->sml_next = NULL;

			if ( agg->agg_mod == NULL ) {
				agg->agg_mod = modlist;
				agg->agg_mod_last = modlist;
			} else {
				agg->agg_mod_last->sml_next = modlist;
				agg->agg_mod_last = modlist;
			}
		}

	}

	return 0;
}


/*
** Adds all entries matching the passed filter to the specified group.
** If modify == 1, then we modify the group's entry in the database using be_modify.
** If modify == 0, then, we must supply a rw entry for the group, 
**	because we only modify the entry, without calling be_modify.
** e	- the group entry, to which the members will be added
** age	- the group
** agf	- the filter
*/
static int
autogroup_add_members_from_filter( Operation *op, Entry *e, autogroup_entry_t *age, autogroup_filter_t *agf, int modify)
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	Operation		o = *op;
	SlapReply		rs = { REP_SEARCH };
	slap_callback		cb = { 0 };
	slap_callback		null_cb = { NULL, slap_null_cb, NULL, NULL };
	autogroup_ga_t		agg;

	Debug(LDAP_DEBUG_TRACE, "==> autogroup_add_members_from_filter <%s>\n",
		age->age_dn.bv_val, 0, 0);

	o.ors_attrsonly = 0;
	o.o_tag = LDAP_REQ_SEARCH;

	o.o_req_dn = agf->agf_dn;
	o.o_req_ndn = agf->agf_ndn;

	o.ors_filterstr = agf->agf_filterstr;
	o.ors_filter = agf->agf_filter;

	o.ors_scope = agf->agf_scope;
	o.ors_deref = LDAP_DEREF_NEVER;
	o.ors_limit = NULL;
	o.ors_tlimit = SLAP_NO_LIMIT;
	o.ors_slimit = SLAP_NO_LIMIT;
	o.ors_attrs =  agf->agf_anlist ? agf->agf_anlist : slap_anlist_no_attrs;

	agg.agg_group = age;
	agg.agg_filter = agf;
	agg.agg_mod = NULL;
	agg.agg_mod_last = NULL;
	agg.agg_entry = e;
	cb.sc_private = &agg;

	if ( modify == 1 ) {
		cb.sc_response = autogroup_member_search_modify_cb;
	} else {
		cb.sc_response = autogroup_member_search_cb;
	}

	cb.sc_cleanup = NULL;
	cb.sc_next = NULL;

	o.o_callback = &cb;

	o.o_bd->bd_info = (BackendInfo *)on->on_info;
	op->o_bd->be_search( &o, &rs );
	o.o_bd->bd_info = (BackendInfo *)on;	

	if ( modify == 1 && agg.agg_mod ) {
		rs_reinit( &rs, REP_RESULT );

		o = *op;
		o.o_callback = &null_cb;
		o.o_tag = LDAP_REQ_MODIFY;
		o.orm_modlist = agg.agg_mod;
		o.o_req_dn = age->age_dn;
		o.o_req_ndn = age->age_ndn;
		o.o_relax = SLAP_CONTROL_CRITICAL;
		o.o_managedsait = SLAP_CONTROL_NONCRITICAL;
		o.o_permissive_modify = 1;

		o.o_bd->bd_info = (BackendInfo *)on->on_info;
		(void)op->o_bd->be_modify( &o, &rs );
		o.o_bd->bd_info = (BackendInfo *)on;	

		slap_mods_free(agg.agg_mod, 1);
	}

	return 0;
}

/* 
** Adds a group to the internal list from the passed entry.
** scan specifies whether to add all maching members to the group.
** modify specifies whether to modify the given group entry (when modify == 0),
**	or to modify the group entry in the database (when modify == 1 and e = NULL and ndn != NULL).
** agi	- pointer to the groups and the attribute definitions
** agd - the attribute definition of the added group
** e	- the entry representing the group, can be NULL if the ndn is specified, and modify == 1
** ndn	- the DN of the group, can be NULL if we give a non-NULL e
*/
static int
autogroup_add_group( Operation *op, autogroup_info_t *agi, autogroup_def_t *agd, Entry *e, BerValue *ndn, int scan, int modify)
{
	autogroup_entry_t	**agep = &agi->agi_entry;
	autogroup_filter_t	*agf, *agf_prev = NULL;
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	LDAPURLDesc		*lud = NULL;
	Attribute		*a;
	BerValue		*bv, dn;
	int			rc = 0, match = 1, null_entry = 0;

	if ( e == NULL ) {
		if ( overlay_entry_get_ov( op, ndn, NULL, NULL, 0, &e, on ) !=
			LDAP_SUCCESS || e == NULL ) {
			Debug( LDAP_DEBUG_TRACE, "autogroup_add_group: cannot get entry for <%s>\n", ndn->bv_val, 0, 0);
			return 1;
		}

		null_entry = 1;
	}

	Debug(LDAP_DEBUG_TRACE, "==> autogroup_add_group <%s>\n",
		e->e_name.bv_val, 0, 0);

	if ( agi->agi_entry != NULL ) {
		for ( ; *agep ; agep = &(*agep)->age_next ) {
			dnMatch( &match, 0, NULL, NULL, &e->e_nname, &(*agep)->age_ndn );
			if ( match == 0 ) {
				Debug( LDAP_DEBUG_TRACE, "autogroup_add_group: group already exists: <%s>\n", e->e_name.bv_val,0,0);
				return 1;
			}
			/* goto last */;
		}
	}


	*agep = (autogroup_entry_t *)ch_calloc( 1, sizeof( autogroup_entry_t ) );
	ldap_pvt_thread_mutex_init( &(*agep)->age_mutex );
	(*agep)->age_def = agd;
	(*agep)->age_filter = NULL;
	(*agep)->age_mustrefresh = 0;
	(*agep)->age_modrdn_olddnmodified = 0;

	ber_dupbv( &(*agep)->age_dn, &e->e_name );
	ber_dupbv( &(*agep)->age_ndn, &e->e_nname );

	a = attrs_find( e->e_attrs, agd->agd_member_url_ad );

	if ( null_entry == 1 ) {
		a = attrs_dup( a );
		overlay_entry_release_ov( op, e, 0, on );
	}

	if( a == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "autogroup_add_group: group has no memberURL\n", 0,0,0);
	} else {
		for ( bv = a->a_nvals; !BER_BVISNULL( bv ); bv++ ) {

			agf = (autogroup_filter_t*)ch_calloc( 1, sizeof( autogroup_filter_t ) );

			if ( ldap_url_parse( bv->bv_val, &lud ) != LDAP_URL_SUCCESS ) {
				Debug( LDAP_DEBUG_TRACE, "autogroup_add_group: cannot parse url <%s>\n", bv->bv_val,0,0);
				/* FIXME: error? */
				ch_free( agf ); 
				continue;
			}

			agf->agf_scope = lud->lud_scope;

			if ( lud->lud_dn == NULL ) {
				BER_BVSTR( &dn, "" );
			} else {
				ber_str2bv( lud->lud_dn, 0, 0, &dn );
			}

			rc = dnPrettyNormal( NULL, &dn, &agf->agf_dn, &agf->agf_ndn, NULL );
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_TRACE, "autogroup_add_group: cannot normalize DN <%s>\n", dn.bv_val,0,0);
				/* FIXME: error? */
				goto cleanup;
			}

			if ( lud->lud_filter != NULL ) {
				ber_str2bv( lud->lud_filter, 0, 1, &agf->agf_filterstr);
				agf->agf_filter = str2filter( lud->lud_filter );
			}			

			if ( lud->lud_attrs != NULL ) {
				int i;

				for ( i=0 ; lud->lud_attrs[i]!=NULL ; i++) {
					/* Just counting */;
				}

				if ( i > 1 ) {
					Debug( LDAP_DEBUG_ANY, "autogroup_add_group: too many attributes specified in url <%s>\n",
						bv->bv_val, 0, 0);
					/* FIXME: error? */
					ldap_free_urldesc( lud );
					ch_free( agf ); 
					continue;
				}
					
				agf->agf_anlist = str2anlist( NULL, lud->lud_attrs[0], "," );

				if ( agf->agf_anlist == NULL ) {
					Debug( LDAP_DEBUG_ANY, "autogroup_add_group: unable to find AttributeDescription \"%s\".\n",
						lud->lud_attrs[0], 0, 0 );		
					/* FIXME: error? */
					ldap_free_urldesc( lud );
					ch_free( agf ); 
					continue;
				}
			}

			agf->agf_next = NULL;


			if( (*agep)->age_filter == NULL ) {
				(*agep)->age_filter = agf;
			}

			if( agf_prev != NULL ) {
				agf_prev->agf_next = agf;
			}

			agf_prev = agf;

			if ( scan == 1 ){
				autogroup_add_members_from_filter( op, e, (*agep), agf, modify );
			}

			Debug( LDAP_DEBUG_TRACE, "autogroup_add_group: added memberURL DN <%s> with filter <%s>\n",
				agf->agf_ndn.bv_val, agf->agf_filterstr.bv_val, 0);

			ldap_free_urldesc( lud );

			continue;


cleanup:;

			ldap_free_urldesc( lud );				
			ch_free( agf ); 
		}
	}

	if ( null_entry == 1 ) {
		attrs_free( a );
	}
	return rc;
}

/* 
** Used when opening the database to add all existing 
** groups from the database to our internal list.
*/
static int
autogroup_group_add_cb( Operation *op, SlapReply *rs )
{
	assert( op->o_tag == LDAP_REQ_SEARCH );

	if ( rs->sr_type == REP_SEARCH ) {
		autogroup_sc_t		*ags = (autogroup_sc_t *)op->o_callback->sc_private;

		Debug(LDAP_DEBUG_TRACE, "==> autogroup_group_add_cb <%s>\n",
			rs->sr_entry ? rs->sr_entry->e_name.bv_val : "UNKNOWN_DN", 0, 0);

		autogroup_add_group( op, ags->ags_info, ags->ags_def, rs->sr_entry, NULL, 0, 0);
	}

	return 0;
}


/*
** When adding a group, we first strip any existing members,
** and add all which match the filters ourselfs.
*/
static int
autogroup_add_entry( Operation *op, SlapReply *rs)
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	autogroup_info_t	*agi = (autogroup_info_t *)on->on_bi.bi_private;
	autogroup_def_t		*agd = agi->agi_def;
	autogroup_entry_t	*age;
	autogroup_filter_t	*agf;
	int			rc = 0;

	Debug( LDAP_DEBUG_TRACE, "==> autogroup_add_entry <%s>\n", 
		op->ora_e->e_name.bv_val, 0, 0);

	ldap_pvt_thread_mutex_lock( &agi->agi_mutex );		

	/* Check if it's a group. */
	for ( ; agd ; agd = agd->agd_next ) {
		if ( is_entry_objectclass_or_sub( op->ora_e, agd->agd_oc ) ) {
			Modification		mod;
			const char		*text = NULL;
			char			textbuf[1024];

			mod.sm_op = LDAP_MOD_DELETE;
			mod.sm_desc = agd->agd_member_ad;
			mod.sm_type = agd->agd_member_ad->ad_cname;
			mod.sm_values = NULL;
			mod.sm_nvalues = NULL;

			/* We don't want any member attributes added by the user. */
			modify_delete_values( op->ora_e, &mod, /* permissive */ 1, &text, textbuf, sizeof( textbuf ) );

			autogroup_add_group( op, agi, agd, op->ora_e, NULL, 1 , 0);
			ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );		
			return SLAP_CB_CONTINUE;
		}
	}

	
	for ( age = agi->agi_entry; age ; age = age->age_next ) {
		ldap_pvt_thread_mutex_lock( &age->age_mutex );		

		/* Check if any of the filters are the suffix to the entry DN. 
		   If yes, we can test that filter against the entry. */

		for ( agf = age->age_filter; agf ; agf = agf->agf_next ) {
			if ( dnIsSuffix( &op->o_req_ndn, &agf->agf_ndn ) ) {
				rc = test_filter( op, op->ora_e, agf->agf_filter );
				if ( rc == LDAP_COMPARE_TRUE ) {
					if ( agf->agf_anlist ) {
						autogroup_add_member_values_to_group( op, op->ora_e, age, agf->agf_anlist[0].an_desc );
					} else {
						autogroup_add_member_to_group( op, &op->ora_e->e_name, &op->ora_e->e_nname, age );
					}
					break;
				}
			}
		}
		ldap_pvt_thread_mutex_unlock( &age->age_mutex );		
	}

	ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );		

	return SLAP_CB_CONTINUE;
}

/*
** agi	- internal group and attribute definitions list
** e	- the group to remove from the internal list
*/
static int
autogroup_delete_group( autogroup_info_t *agi, autogroup_entry_t *e )
{
	autogroup_entry_t	*age = agi->agi_entry,
				*age_prev = NULL,
				*age_next;
	int			rc = 1;

	Debug( LDAP_DEBUG_TRACE, "==> autogroup_delete_group <%s>\n", 
		age->age_dn.bv_val, 0, 0);

	for ( age_next = age ; age_next ; age_prev = age, age = age_next ) {
		age_next = age->age_next;

		if ( age == e ) {
			autogroup_filter_t	*agf = age->age_filter,
							*agf_next;

			if ( age_prev != NULL ) {
				age_prev->age_next = age_next;
			} else {
				agi->agi_entry = NULL;
			}

			ch_free( age->age_dn.bv_val );
			ch_free( age->age_ndn.bv_val );

			for( agf_next = agf ; agf_next ; agf = agf_next ){
				agf_next = agf->agf_next;

				filter_free( agf->agf_filter );
				ch_free( agf->agf_filterstr.bv_val );
				ch_free( agf->agf_dn.bv_val );
				ch_free( agf->agf_ndn.bv_val );
				anlist_free( agf->agf_anlist, 1, NULL );
				ch_free( agf );
			}

			ldap_pvt_thread_mutex_unlock( &age->age_mutex );		
			ldap_pvt_thread_mutex_destroy( &age->age_mutex );
			ch_free( age );

			rc = 0;	
			return rc;

		}
	}

	Debug( LDAP_DEBUG_TRACE, "autogroup_delete_group: group <%s> not found, should not happen\n", age->age_dn.bv_val, 0, 0);

	return rc;

}

static int
autogroup_delete_entry( Operation *op, SlapReply *rs)
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	autogroup_info_t	*agi = (autogroup_info_t *)on->on_bi.bi_private;
	autogroup_entry_t	*age, *age_prev, *age_next;
	autogroup_filter_t	*agf;
	Entry			*e;
	int			matched_group = 0, rc = 0;

	Debug( LDAP_DEBUG_TRACE, "==> autogroup_delete_entry <%s>\n", op->o_req_dn.bv_val, 0, 0);

	ldap_pvt_thread_mutex_lock( &agi->agi_mutex );

	if ( overlay_entry_get_ov( op, &op->o_req_ndn, NULL, NULL, 0, &e, on ) !=
		LDAP_SUCCESS || e == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "autogroup_delete_entry: cannot get entry for <%s>\n", op->o_req_dn.bv_val, 0, 0);
		ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );			
		return SLAP_CB_CONTINUE;
	}

	/* Check if the entry to be deleted is one of our groups. */
	for ( age_next = agi->agi_entry ; age_next ; age_prev = age ) {
		age = age_next;
		ldap_pvt_thread_mutex_lock( &age->age_mutex );
		age_next = age->age_next;

		if ( is_entry_objectclass_or_sub( e, age->age_def->agd_oc ) ) {
			int match = 1;

			matched_group = 1;

			dnMatch( &match, 0, NULL, NULL, &e->e_nname, &age->age_ndn );

			if ( match == 0 ) {
				autogroup_delete_group( agi, age );
				break;
			}
		}

		ldap_pvt_thread_mutex_unlock( &age->age_mutex );			
	}

	if ( matched_group == 1 ) {
		overlay_entry_release_ov( op, e, 0, on );
		ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );		
		return SLAP_CB_CONTINUE;
	}

	/* Check if the entry matches any of the groups.
	   If yes, we can delete the entry from that group. */

	for ( age = agi->agi_entry ; age ; age = age->age_next ) {
		ldap_pvt_thread_mutex_lock( &age->age_mutex );		

		for ( agf = age->age_filter; agf ; agf = agf->agf_next ) {
			if ( dnIsSuffix( &op->o_req_ndn, &agf->agf_ndn ) ) {
				rc = test_filter( op, e, agf->agf_filter );
				if ( rc == LDAP_COMPARE_TRUE ) {
					/* If the attribute is retrieved from the entry, we don't know what to delete
					** So the group must be entirely refreshed
					** But the refresh can't be done now because the entry is not deleted
					** So the group is marked as mustrefresh
					*/
					if ( agf->agf_anlist ) {
						age->age_mustrefresh = 1;
					} else {
						autogroup_delete_member_from_group( op, &e->e_name, &e->e_nname, age );
					}
					break;
				}
			}
		}
		ldap_pvt_thread_mutex_unlock( &age->age_mutex );
	}

	overlay_entry_release_ov( op, e, 0, on );
	ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );		

	return SLAP_CB_CONTINUE;
}

static int
autogroup_response( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	autogroup_info_t	*agi = (autogroup_info_t *)on->on_bi.bi_private;
	autogroup_def_t		*agd = agi->agi_def;
	autogroup_entry_t	*age;
	autogroup_filter_t	*agf;
	BerValue		new_dn, new_ndn, pdn;
	Entry			*e, *group;
	Attribute		*a, *ea;
	int			is_olddn, is_newdn, is_value_refresh, dn_equal;

	/* Handle all cases where a refresh of the group is needed */
	if ( op->o_tag == LDAP_REQ_DELETE || op->o_tag == LDAP_REQ_MODIFY ) {
		if ( rs->sr_type == REP_RESULT && rs->sr_err == LDAP_SUCCESS && !get_manageDSAit( op ) ) {

			ldap_pvt_thread_mutex_lock( &agi->agi_mutex );

			for ( age = agi->agi_entry ; age ; age = age->age_next ) {
				/* Request detected that the group must be refreshed */

				ldap_pvt_thread_mutex_lock( &age->age_mutex );

				if ( age->age_mustrefresh ) {
					autogroup_delete_member_from_group( op, NULL, NULL, age) ;

					for ( agf = age->age_filter ; agf ; agf = agf->agf_next ) {
						autogroup_add_members_from_filter( op, NULL, age, agf, 1 );
					}
				}

				ldap_pvt_thread_mutex_unlock( &age->age_mutex );
			}

			ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
		}
	} else if ( op->o_tag == LDAP_REQ_MODRDN ) {
		if ( rs->sr_type == REP_RESULT && rs->sr_err == LDAP_SUCCESS && !get_manageDSAit( op )) {

			Debug( LDAP_DEBUG_TRACE, "==> autogroup_response MODRDN from <%s>\n", op->o_req_dn.bv_val, 0, 0);

			ldap_pvt_thread_mutex_lock( &agi->agi_mutex );			

			if ( op->oq_modrdn.rs_newSup ) {
				pdn = *op->oq_modrdn.rs_newSup;
			} else {
				dnParent( &op->o_req_dn, &pdn );
			}
			build_new_dn( &new_dn, &pdn, &op->orr_newrdn, op->o_tmpmemctx );

			if ( op->oq_modrdn.rs_nnewSup ) {
				pdn = *op->oq_modrdn.rs_nnewSup;
			} else {
				dnParent( &op->o_req_ndn, &pdn );
			}
			build_new_dn( &new_ndn, &pdn, &op->orr_nnewrdn, op->o_tmpmemctx );

			Debug( LDAP_DEBUG_TRACE, "autogroup_response MODRDN to <%s>\n", new_dn.bv_val, 0, 0);

			dnMatch( &dn_equal, 0, NULL, NULL, &op->o_req_ndn, &new_ndn );

			if ( overlay_entry_get_ov( op, &new_ndn, NULL, NULL, 0, &e, on ) !=
				LDAP_SUCCESS || e == NULL ) {
				Debug( LDAP_DEBUG_TRACE, "autogroup_response MODRDN cannot get entry for <%s>\n", new_dn.bv_val, 0, 0);
				ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
				return SLAP_CB_CONTINUE;
			}

			a = attrs_find( e->e_attrs, slap_schema.si_ad_objectClass );


			if ( a == NULL ) {
				Debug( LDAP_DEBUG_TRACE, "autogroup_response MODRDN entry <%s> has no objectClass\n", new_dn.bv_val, 0, 0);
				overlay_entry_release_ov( op, e, 0, on );
				ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );		
				return SLAP_CB_CONTINUE;
			}


			/* If a groups DN is modified, just update age_dn/ndn of that group with the new DN. */
			for ( ; agd; agd = agd->agd_next ) {

				if ( value_find_ex( slap_schema.si_ad_objectClass,
						SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
						SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
						a->a_nvals, &agd->agd_oc->soc_cname,
						op->o_tmpmemctx ) == 0 )
				{		
					for ( age = agi->agi_entry ; age ; age = age->age_next ) {
						int match = 1;

						dnMatch( &match, 0, NULL, NULL, &age->age_ndn, &op->o_req_ndn );
						if ( match == 0 ) {
							Debug( LDAP_DEBUG_TRACE, "autogroup_response MODRDN updating group's DN to <%s>\n", new_dn.bv_val, 0, 0);
							ber_dupbv( &age->age_dn, &new_dn );
							ber_dupbv( &age->age_ndn, &new_ndn );

							op->o_tmpfree( new_dn.bv_val, op->o_tmpmemctx  );
							op->o_tmpfree( new_ndn.bv_val, op->o_tmpmemctx );
							overlay_entry_release_ov( op, e, 0, on );
							ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );		
							return SLAP_CB_CONTINUE;
						}
					}

				}
			}

			/* For each group: 
			   1. check if the orginal entry's DN is in the group.
			   2. chceck if the any of the group filter's base DN is a suffix of the new DN 

			   If 1 and 2 are both false, we do nothing.
			   If 1 and 2 is true, we remove the old DN from the group, and add the new DN.
			   If 1 is false, and 2 is true, we check the entry against the group's filters,
				and add it's DN to the group.
			   If 1 is true, and 2 is false, we delete the entry's DN from the group.
			*/
			for ( age = agi->agi_entry ; age ; age = age->age_next ) {
				is_olddn = 0;
				is_newdn = 0;
				is_value_refresh = 0;


				ldap_pvt_thread_mutex_lock( &age->age_mutex );

				if ( age->age_filter && age->age_filter->agf_anlist ) {
					ea = attrs_find( e->e_attrs, age->age_filter->agf_anlist[0].an_desc );
				}
				else {
					ea = NULL;
				}

				if ( age->age_modrdn_olddnmodified ) {
					/* Resquest already marked this group to be updated */
					is_olddn = 1;
					is_value_refresh = 1;
					age->age_modrdn_olddnmodified = 0;
				} else {

					if ( overlay_entry_get_ov( op, &age->age_ndn, NULL, NULL, 0, &group, on ) !=
						LDAP_SUCCESS || group == NULL ) {
						Debug( LDAP_DEBUG_TRACE, "autogroup_response MODRDN cannot get group entry <%s>\n", age->age_dn.bv_val, 0, 0);

						op->o_tmpfree( new_dn.bv_val, op->o_tmpmemctx );
						op->o_tmpfree( new_ndn.bv_val, op->o_tmpmemctx );

						overlay_entry_release_ov( op, e, 0, on );
						ldap_pvt_thread_mutex_unlock( &age->age_mutex );
						ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
						return SLAP_CB_CONTINUE;
					}

					a = attrs_find( group->e_attrs, age->age_def->agd_member_ad );

					if ( a != NULL ) {
						if ( value_find_ex( age->age_def->agd_member_ad,
								SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
								SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
								a->a_nvals, ea ? ea->a_nvals : &op->o_req_ndn, op->o_tmpmemctx ) == 0 ) 
						{
							is_olddn = 1;
						}

					}

					overlay_entry_release_ov( op, group, 0, on );

				}

				for ( agf = age->age_filter ; agf ; agf = agf->agf_next ) {
					if ( dnIsSuffix( &new_ndn, &agf->agf_ndn ) ) {
						/* TODO: should retest filter as it could imply conditions on the dn */
						is_newdn = 1;
						break;
					}
				}


				if ( is_value_refresh ) {
					if ( is_olddn != is_newdn ) {
						/* group refresh */
						autogroup_delete_member_from_group( op, NULL, NULL, age) ;

						for ( agf = age->age_filter ; agf ; agf = agf->agf_next ) {
							autogroup_add_members_from_filter( op, NULL, age, agf, 1 );
						}
					}
					ldap_pvt_thread_mutex_unlock( &age->age_mutex );
					continue;
				}
				if ( is_olddn == 1 && is_newdn == 0 ) {
					if ( ea )
						autogroup_delete_member_values_from_group( op, e, age, age->age_filter->agf_anlist[0].an_desc );
					else
						autogroup_delete_member_from_group( op, &op->o_req_dn, &op->o_req_ndn, age );
				} else
				if ( is_olddn == 0 && is_newdn == 1 ) {
					for ( agf = age->age_filter; agf; agf = agf->agf_next ) {
						if ( test_filter( op, e, agf->agf_filter ) == LDAP_COMPARE_TRUE ) {
							if ( ea )
								autogroup_add_member_values_to_group( op, e, age, age->age_filter->agf_anlist[0].an_desc );
							else
								autogroup_add_member_to_group( op, &new_dn, &new_ndn, age );
							break;
						}
					}
				} else
				if ( is_olddn == 1 && is_newdn == 1 && dn_equal != 0 ) {
					if ( ea ) {
						/* group refresh */
						autogroup_delete_member_from_group( op, NULL, NULL, age) ;

						for ( agf = age->age_filter ; agf ; agf = agf->agf_next ) {
							autogroup_add_members_from_filter( op, NULL, age, agf, 1 );
						}
					}
					else {
						autogroup_delete_member_from_group( op, &op->o_req_dn, &op->o_req_ndn, age );
						autogroup_add_member_to_group( op, &new_dn, &new_ndn, age );
					}
				}

				ldap_pvt_thread_mutex_unlock( &age->age_mutex );
			}

			op->o_tmpfree( new_dn.bv_val, op->o_tmpmemctx );
			op->o_tmpfree( new_ndn.bv_val, op->o_tmpmemctx );

			overlay_entry_release_ov( op, e, 0, on );

			ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );			
		}
	}

	if ( op->o_tag == LDAP_REQ_MODIFY ) {
		if ( rs->sr_type == REP_RESULT && rs->sr_err == LDAP_SUCCESS  && !get_manageDSAit( op ) ) {
			Debug( LDAP_DEBUG_TRACE, "==> autogroup_response MODIFY <%s>\n", op->o_req_dn.bv_val, 0, 0);

			ldap_pvt_thread_mutex_lock( &agi->agi_mutex );			

			if ( overlay_entry_get_ov( op, &op->o_req_ndn, NULL, NULL, 0, &e, on ) !=
				LDAP_SUCCESS || e == NULL ) {
				Debug( LDAP_DEBUG_TRACE, "autogroup_response MODIFY cannot get entry for <%s>\n", op->o_req_dn.bv_val, 0, 0);
				ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
				return SLAP_CB_CONTINUE;
			}

			a = attrs_find( e->e_attrs, slap_schema.si_ad_objectClass );


			if ( a == NULL ) {
				Debug( LDAP_DEBUG_TRACE, "autogroup_response MODIFY entry <%s> has no objectClass\n", op->o_req_dn.bv_val, 0, 0);
				overlay_entry_release_ov( op, e, 0, on );
				ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );		
				return SLAP_CB_CONTINUE;
			}

			/* If we modify a group's memberURL, we have to delete all of it's members,
			   and add them anew, because we cannot tell from which memberURL a member was added. */
			for ( ; agd; agd = agd->agd_next ) {

				if ( value_find_ex( slap_schema.si_ad_objectClass,
						SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
						SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
						a->a_nvals, &agd->agd_oc->soc_cname,
						op->o_tmpmemctx ) == 0 )
				{
					Modifications	*m;
					int		match = 1;

					m = op->orm_modlist;

					for ( age = agi->agi_entry ; age ; age = age->age_next ) {
						ldap_pvt_thread_mutex_lock( &age->age_mutex );

						dnMatch( &match, 0, NULL, NULL, &op->o_req_ndn, &age->age_ndn );

						if ( match == 0 ) {
							for ( ; m ; m = m->sml_next ) {
								if ( m->sml_desc == age->age_def->agd_member_url_ad ) {
									autogroup_def_t	*group_agd = age->age_def;
									Debug( LDAP_DEBUG_TRACE, "autogroup_response MODIFY changing memberURL for group <%s>\n", 
										op->o_req_dn.bv_val, 0, 0);

									overlay_entry_release_ov( op, e, 0, on );

									autogroup_delete_member_from_group( op, NULL, NULL, age );
									autogroup_delete_group( agi, age );

									autogroup_add_group( op, agi, group_agd, NULL, &op->o_req_ndn, 1, 1);

									ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
									return SLAP_CB_CONTINUE;
								}
							}

							ldap_pvt_thread_mutex_unlock( &age->age_mutex );
							break;
						}

						ldap_pvt_thread_mutex_unlock( &age->age_mutex );
					}

					overlay_entry_release_ov( op, e, 0, on );
					ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
					return SLAP_CB_CONTINUE;
				}
			}

			/* When modifing any of the attributes of an entry, we must
			   check if the entry is in any of our groups, and if
			   the modified entry maches any of the filters of that group.

			   If the entry exists in a group, but the modified attributes do
				not match any of the group's filters, we delete the entry from that group.
			   If the entry doesn't exist in a group, but matches a filter, 
				we add it to that group.
			*/
			for ( age = agi->agi_entry ; age ; age = age->age_next ) {
				is_olddn = 0;
				is_newdn = 0;


				ldap_pvt_thread_mutex_lock( &age->age_mutex );

				if ( age->age_filter && age->age_filter->agf_anlist ) {
					ea = attrs_find( e->e_attrs, age->age_filter->agf_anlist[0].an_desc );
				}
				else {
					ea = NULL;
				}

				if ( overlay_entry_get_ov( op, &age->age_ndn, NULL, NULL, 0, &group, on ) !=
					LDAP_SUCCESS || group == NULL ) {
					Debug( LDAP_DEBUG_TRACE, "autogroup_response MODIFY cannot get entry for <%s>\n", 
						age->age_dn.bv_val, 0, 0);

					overlay_entry_release_ov( op, e, 0, on );
					ldap_pvt_thread_mutex_unlock( &age->age_mutex );
					ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
					return SLAP_CB_CONTINUE;
				}

				a = attrs_find( group->e_attrs, age->age_def->agd_member_ad );

				if ( a != NULL ) {
					if ( value_find_ex( age->age_def->agd_member_ad,
							SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
							SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
							a->a_nvals, ea ? ea->a_nvals : &op->o_req_ndn, op->o_tmpmemctx ) == 0 ) 
					{
						is_olddn = 1;
					}

				}

				overlay_entry_release_ov( op, group, 0, on );

				for ( agf = age->age_filter ; agf ; agf = agf->agf_next ) {
					if ( dnIsSuffix( &op->o_req_ndn, &agf->agf_ndn ) ) {
						if ( test_filter( op, e, agf->agf_filter ) == LDAP_COMPARE_TRUE ) {
							is_newdn = 1;
							break;
						}
					}
				}

				if ( is_olddn == 1 && is_newdn == 0 ) {
					if(ea)
						autogroup_delete_member_values_from_group( op, e, age, age->age_filter->agf_anlist[0].an_desc );
					else
						autogroup_delete_member_from_group( op, &op->o_req_dn, &op->o_req_ndn, age );
				} else
				if ( is_olddn == 0 && is_newdn == 1 ) {
					if(ea)
						autogroup_add_member_values_to_group( op, e, age, age->age_filter->agf_anlist[0].an_desc );
					else
						autogroup_add_member_to_group( op, &op->o_req_dn, &op->o_req_ndn, age );
				} 

				ldap_pvt_thread_mutex_unlock( &age->age_mutex );
			}

			overlay_entry_release_ov( op, e, 0, on );

			ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
		}
	}

	return SLAP_CB_CONTINUE;
}

/*
** Detect if filter contains a memberOf check for dn
*/
static int
autogroup_memberOf_filter( Filter *f, BerValue *dn, AttributeDescription *memberof_ad )
{
	int result = 0;
	if ( f == NULL ) return 0;

  	switch ( f->f_choice & SLAPD_FILTER_MASK ) {
		case LDAP_FILTER_AND:
		case LDAP_FILTER_OR:
		case LDAP_FILTER_NOT:
			for ( f = f->f_un.f_un_complex; f && !result; f = f->f_next ) {
				result = result || autogroup_memberOf_filter( f, dn, memberof_ad );
			}
			break;
		case LDAP_FILTER_EQUALITY:
			result = ( f->f_ava->aa_desc == memberof_ad &&
			           ber_bvcmp( &f->f_ava->aa_value, dn ) == 0 );
			break;
		default:
			break;
	}

	return result;
}

/*
** When modifing a group, we must deny any modifications to the member attribute,
** because the group would be inconsistent.
*/
static int
autogroup_modify_entry( Operation *op, SlapReply *rs)
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	autogroup_info_t		*agi = (autogroup_info_t *)on->on_bi.bi_private;
	autogroup_def_t		*agd = agi->agi_def;
	autogroup_entry_t	*age;
	Entry			*e;
	Attribute		*a;

	if ( get_manageDSAit( op ) ) {
		return SLAP_CB_CONTINUE;
	}

	Debug( LDAP_DEBUG_TRACE, "==> autogroup_modify_entry <%s>\n", op->o_req_dn.bv_val, 0, 0);
	ldap_pvt_thread_mutex_lock( &agi->agi_mutex );			

	if ( overlay_entry_get_ov( op, &op->o_req_ndn, NULL, NULL, 0, &e, on ) !=
		LDAP_SUCCESS || e == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "autogroup_modify_entry cannot get entry for <%s>\n", op->o_req_dn.bv_val, 0, 0);
		ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
		return SLAP_CB_CONTINUE;
	}

	/* Must refresh groups if a matching member value is modified OR filter contains memberOf=DN */
	for ( age = agi->agi_entry; age ; age = age->age_next ) {
		autogroup_filter_t	*agf;
		for ( agf = age->age_filter ; agf ; agf = agf->agf_next ) {
			if ( agf->agf_anlist ) {
				Modifications	*m;
				for ( m = op->orm_modlist ; m ; m = m->sml_next ) {
					if ( m->sml_desc == agf->agf_anlist[0].an_desc ) {
						if ( dnIsSuffix( &op->o_req_ndn, &agf->agf_ndn ) ) {
							int rc = test_filter( op, e, agf->agf_filter );
							if ( rc == LDAP_COMPARE_TRUE ) {
								age->age_mustrefresh = 1;
							}
						}
					}
				}
			}

			if ( autogroup_memberOf_filter( agf->agf_filter, &op->o_req_ndn, agi->agi_memberof_ad ) ) {
				age->age_mustrefresh = 1;
			}
		}
	}

	a = attrs_find( e->e_attrs, slap_schema.si_ad_objectClass );

	if ( a == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "autogroup_modify_entry entry <%s> has no objectClass\n", op->o_req_dn.bv_val, 0, 0);
		ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );		
		return SLAP_CB_CONTINUE;
	}


	for ( ; agd; agd = agd->agd_next ) {

		if ( value_find_ex( slap_schema.si_ad_objectClass,
				SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
				SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
				a->a_nvals, &agd->agd_oc->soc_cname,
				op->o_tmpmemctx ) == 0 )
		{
			Modifications	*m;
			int		match = 1;

			m = op->orm_modlist;

			for ( age = agi->agi_entry ; age ; age = age->age_next ) {
				dnMatch( &match, 0, NULL, NULL, &op->o_req_ndn, &age->age_ndn );

				if ( match == 0 ) {
					for ( ; m ; m = m->sml_next ) {
						if ( m->sml_desc == age->age_def->agd_member_ad ) {
							overlay_entry_release_ov( op, e, 0, on );
							ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
							Debug( LDAP_DEBUG_TRACE, "autogroup_modify_entry attempted to modify group's <%s> member attribute\n", op->o_req_dn.bv_val, 0, 0);
							send_ldap_error(op, rs, LDAP_CONSTRAINT_VIOLATION, "attempt to modify dynamic group member attribute");
							return LDAP_CONSTRAINT_VIOLATION;
						}
					}
					break;
				}
			}

			overlay_entry_release_ov( op, e, 0, on );
			ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
			return SLAP_CB_CONTINUE;
		}
	}

	overlay_entry_release_ov( op, e, 0, on );
	ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );			
	return SLAP_CB_CONTINUE;
}

/*
** Detect if the olddn is part of a group and so if the group should be refreshed
*/
static int
autogroup_modrdn_entry( Operation *op, SlapReply *rs)
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	autogroup_info_t	*agi = (autogroup_info_t *)on->on_bi.bi_private;
	autogroup_entry_t	*age;
	Entry			*e;

	if ( get_manageDSAit( op ) ) {
		return SLAP_CB_CONTINUE;
	}

	Debug( LDAP_DEBUG_TRACE, "==> autogroup_modrdn_entry <%s>\n", op->o_req_dn.bv_val, 0, 0);
	ldap_pvt_thread_mutex_lock( &agi->agi_mutex );			

	if ( overlay_entry_get_ov( op, &op->o_req_ndn, NULL, NULL, 0, &e, on ) !=
		LDAP_SUCCESS || e == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "autogroup_modrdn_entry cannot get entry for <%s>\n", op->o_req_dn.bv_val, 0, 0);
		ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );
		return SLAP_CB_CONTINUE;
	}

	/* Must check if a dn is modified */
	for ( age = agi->agi_entry; age ; age = age->age_next ) {
		autogroup_filter_t	*agf;
		for ( agf = age->age_filter ; agf ; agf = agf->agf_next ) {
			if ( agf->agf_anlist ) {
				if ( dnIsSuffix( &op->o_req_ndn, &agf->agf_ndn ) ) {
					int rc = test_filter( op, e, agf->agf_filter );
					if ( rc == LDAP_COMPARE_TRUE ) {
						age->age_modrdn_olddnmodified = 1;
					}
				}
			}
		}
	}

	overlay_entry_release_ov( op, e, 0, on );
	ldap_pvt_thread_mutex_unlock( &agi->agi_mutex );			
	return SLAP_CB_CONTINUE;
}

/* 
** Builds a filter for searching for the 
** group entries, according to the objectClass. 
*/
static int
autogroup_build_def_filter( autogroup_def_t *agd, Operation *op )
{
	char	*ptr;

	Debug( LDAP_DEBUG_TRACE, "==> autogroup_build_def_filter\n", 0, 0, 0);

	op->ors_filterstr.bv_len = STRLENOF( "(=)" ) 
			+ slap_schema.si_ad_objectClass->ad_cname.bv_len
			+ agd->agd_oc->soc_cname.bv_len;
	ptr = op->ors_filterstr.bv_val = op->o_tmpalloc( op->ors_filterstr.bv_len + 1, op->o_tmpmemctx );
	*ptr++ = '(';
	ptr = lutil_strcopy( ptr, slap_schema.si_ad_objectClass->ad_cname.bv_val );
	*ptr++ = '=';
	ptr = lutil_strcopy( ptr, agd->agd_oc->soc_cname.bv_val );
	*ptr++ = ')';
	*ptr = '\0';

	op->ors_filter = str2filter_x( op, op->ors_filterstr.bv_val );

	assert( op->ors_filterstr.bv_len == ptr - op->ors_filterstr.bv_val );

	return 0;
}

enum {
	AG_ATTRSET = 1,
	AG_MEMBER_OF_AD,
	AG_LAST
};

static ConfigDriver	ag_cfgen;

static ConfigTable agcfg[] = {
	{ "autogroup-attrset", "group-oc> <URL-ad> <member-ad",
		3, 4, 0, ARG_MAGIC|AG_ATTRSET, ag_cfgen,
		"( OLcfgCtAt:2.1 NAME 'olcAGattrSet' "
			"DESC 'Automatic groups: <group objectClass>, <URL attributeDescription>, <member attributeDescription>' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )",
			NULL, NULL },
	
	{ "autogroup-memberof-ad", "memberOf attribute",
		2, 2, 0, ARG_MAGIC|AG_MEMBER_OF_AD, ag_cfgen,
		"( OLcfgCtAt:2.2 NAME 'olcAGmemberOfAd' "
			"DESC 'memberOf attribute' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
			NULL, NULL },

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs agocs[] = {
	{ "( OLcfgCtOc:2.1 "
		"NAME 'olcAutomaticGroups' "
		"DESC 'Automatic groups configuration' "
		"SUP olcOverlayConfig "
		"MAY ( "
			"olcAGattrSet "
			"$ olcAGmemberOfAd "
		    ")"
	  ")",
		Cft_Overlay, agcfg, NULL, NULL },
	{ NULL, 0, NULL }
};


static int
ag_cfgen( ConfigArgs *c )
{
	slap_overinst		*on = (slap_overinst *)c->bi;
	autogroup_info_t		*agi = (autogroup_info_t *)on->on_bi.bi_private;
	autogroup_def_t		*agd;
	autogroup_entry_t	*age;

	int rc = 0, i;

	Debug( LDAP_DEBUG_TRACE, "==> autogroup_cfgen\n", 0, 0, 0);

	if( agi == NULL ) {
		agi = (autogroup_info_t*)ch_calloc( 1, sizeof(autogroup_info_t) );
		ldap_pvt_thread_mutex_init( &agi->agi_mutex );
		agi->agi_def = NULL;
		agi->agi_entry = NULL;
		on->on_bi.bi_private = (void *)agi;
	}

	agd = agi->agi_def;
	age = agi->agi_entry;

	if ( c->op == SLAP_CONFIG_EMIT ) {

		switch( c->type ){
		case AG_ATTRSET:
			for ( i = 0 ; agd ; i++, agd = agd->agd_next ) {
				struct berval	bv;
				char		*ptr = c->cr_msg;
	
				assert(agd->agd_oc != NULL);
				assert(agd->agd_member_url_ad != NULL);
				assert(agd->agd_member_ad != NULL);
	
				ptr += snprintf( c->cr_msg, sizeof( c->cr_msg ),
					SLAP_X_ORDERED_FMT "%s %s %s", i,
					agd->agd_oc->soc_cname.bv_val,
					agd->agd_member_url_ad->ad_cname.bv_val,
					agd->agd_member_ad->ad_cname.bv_val );
	
				bv.bv_val = c->cr_msg;
				bv.bv_len = ptr - bv.bv_val;
				value_add_one ( &c->rvalue_vals, &bv );
	
			}
			break;

		case AG_MEMBER_OF_AD:
			if ( agi->agi_memberof_ad != NULL ){
				value_add_one( &c->rvalue_vals, &agi->agi_memberof_ad->ad_cname );
			}
			break;

		default:
			assert( 0 );
			return 1;
      }

		return rc;

	}else if ( c->op == LDAP_MOD_DELETE ) {
		if ( c->valx < 0) {
			autogroup_def_t 		*agd_next;
			autogroup_entry_t	*age_next;
			autogroup_filter_t	*agf = age->age_filter,
						*agf_next;

			for ( agd_next = agd; agd_next; agd = agd_next ) {
				agd_next = agd->agd_next;

				ch_free( agd );
			}

			for ( age_next = age ; age_next ; age = age_next ) {
				age_next = age->age_next;

				ch_free( age->age_dn.bv_val );
				ch_free( age->age_ndn.bv_val );

				for( agf_next = agf ; agf_next ; agf = agf_next ){
					agf_next = agf->agf_next;

					filter_free( agf->agf_filter );
					ch_free( agf->agf_filterstr.bv_val );
					ch_free( agf->agf_dn.bv_val );
					ch_free( agf->agf_ndn.bv_val );
					anlist_free( agf->agf_anlist, 1, NULL );
					ch_free( agf );
				}

				ldap_pvt_thread_mutex_init( &age->age_mutex );
				ch_free( age );
			}

			ch_free( agi );
			on->on_bi.bi_private = NULL;

		} else {
			autogroup_def_t		**agdp;
			autogroup_entry_t	*age_next, *age_prev;
			autogroup_filter_t	*agf,
						*agf_next;

			for ( i = 0, agdp = &agi->agi_def;
				i < c->valx; i++ ) 
			{
				if ( *agdp == NULL) {
					return 1;
				}
				agdp = &(*agdp)->agd_next;
			}

			agd = *agdp;
			*agdp = agd->agd_next;

			for ( age_next = age , age_prev = NULL ; age_next ; age_prev = age, age = age_next ) {
				age_next = age->age_next;

				if( age->age_def == agd ) {
					agf = age->age_filter;

					ch_free( age->age_dn.bv_val );
					ch_free( age->age_ndn.bv_val );

					for ( agf_next = agf; agf_next ; agf = agf_next ) {
						agf_next = agf->agf_next;
						filter_free( agf->agf_filter );
						ch_free( agf->agf_filterstr.bv_val );
						ch_free( agf->agf_dn.bv_val );
						ch_free( agf->agf_ndn.bv_val );
						anlist_free( agf->agf_anlist, 1, NULL );
						ch_free( agf );
					}

					ldap_pvt_thread_mutex_destroy( &age->age_mutex );
					ch_free( age );

					age = age_prev;

					if( age_prev != NULL ) {
						age_prev->age_next = age_next;
					}
				}
			}

			ch_free( agd );
			agd = agi->agi_def;

		}

		return rc;
	}

	switch(c->type){
	case AG_ATTRSET: {
		autogroup_def_t		**agdp,
					*agd_next = NULL;
		ObjectClass		*oc = NULL;
		AttributeDescription	*member_url_ad = NULL,
					*member_ad = NULL;
		const char		*text;


		oc = oc_find( c->argv[ 1 ] );
		if( oc == NULL ){
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"autogroup-attrset <oc> <URL-ad> <member-ad>\": "
				"unable to find ObjectClass \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}


		rc = slap_str2ad( c->argv[ 2 ], &member_url_ad, &text );
		if( rc != LDAP_SUCCESS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"autogroup-attrset <oc> <URL-ad> <member-ad>\": "
				"unable to find AttributeDescription \"%s\"",
				c->argv[ 2 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );		
			return 1;
		}

		if( !is_at_subtype( member_url_ad->ad_type, slap_schema.si_ad_labeledURI->ad_type ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"autogroup-attrset <oc> <URL-ad> <member-ad>\": "
				"AttributeDescription \"%s\" ",
				"must be of a subtype \"labeledURI\"",
				c->argv[ 2 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		rc = slap_str2ad( c->argv[3], &member_ad, &text );
		if( rc != LDAP_SUCCESS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"autogroup-attrset <oc> <URL-ad> <member-ad>\": "
				"unable to find AttributeDescription \"%s\"",
				c->argv[ 3 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		for ( agdp = &agi->agi_def ; *agdp ; agdp = &(*agdp)->agd_next ) {
			/* The same URL attribute / member attribute pair
			* cannot be repeated */

			if ( (*agdp)->agd_member_url_ad == member_url_ad && (*agdp)->agd_member_ad == member_ad ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"\"autogroup-attrset <oc> <URL-ad> <member-ad>\": "
					"URL attributeDescription \"%s\" already mapped",
					member_ad->ad_cname.bv_val );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
/*				return 1; //warning*/
			}
		}

		if ( c->valx > 0 ) {
			int	i;

			for ( i = 0, agdp = &agi->agi_def ;
				i < c->valx; i++ )
			{
				if ( *agdp == NULL ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"\"autogroup-attrset <oc> <URL-ad> <member-ad>\": "
						"invalid index {%d}",
						c->valx );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
						c->log, c->cr_msg, 0 );

					return 1;
				}
				agdp = &(*agdp)->agd_next;
			}
			agd_next = *agdp;

		} else {
			for ( agdp = &agi->agi_def; *agdp;
				agdp = &(*agdp)->agd_next )
				/* goto last */;
		}

		*agdp = (autogroup_def_t *)ch_calloc( 1, sizeof(autogroup_info_t));

		(*agdp)->agd_oc = oc;
		(*agdp)->agd_member_url_ad = member_url_ad;
		(*agdp)->agd_member_ad = member_ad;
		(*agdp)->agd_next = agd_next;

		} break;
	
	case AG_MEMBER_OF_AD: {
		AttributeDescription *memberof_ad = NULL;
		const char     *text;

		rc = slap_str2ad( c->argv[ 1 ], &memberof_ad, &text );
		if( rc != LDAP_SUCCESS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"autogroup-memberof-ad <memberof-ad>\": "
				"unable to find AttributeDescription \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		if ( !is_at_syntax( memberof_ad->ad_type, SLAPD_DN_SYNTAX )    /* e.g. "member" */
		     && !is_at_syntax( memberof_ad->ad_type, SLAPD_NAMEUID_SYNTAX ) )  /* e.g. "uniqueMember" */
		{
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"memberof attribute=\"%s\" must either "
				"have DN (%s) or nameUID (%s) syntax",
				c->argv[ 1 ], SLAPD_DN_SYNTAX, SLAPD_NAMEUID_SYNTAX );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		agi->agi_memberof_ad = memberof_ad;

		} break;

	default:
		rc = 1;
		break;
	}

	return rc;
}

extern int slapMode;

/* 
** Do a search for all the groups in the
** database, and add them to out internal list.
*/
static int
autogroup_db_open(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst			*on = (slap_overinst *) be->bd_info;
	autogroup_info_t		*agi = on->on_bi.bi_private;
	autogroup_def_t		*agd;
	autogroup_sc_t		ags;
	Operation		*op;
	slap_callback		cb = { 0 };

	void				*thrctx = ldap_pvt_thread_pool_context();
	Connection			conn = { 0 };
	OperationBuffer 	opbuf;

	Debug( LDAP_DEBUG_TRACE, "==> autogroup_db_open\n", 0, 0, 0);

	if ( agi == NULL || !( slapMode & SLAP_SERVER_MODE )) {
		return 0;
	}

	connection_fake_init( &conn, &opbuf, thrctx );
	op = &opbuf.ob_op;

	op->ors_attrsonly = 0;
	op->o_tag = LDAP_REQ_SEARCH;
	op->o_dn = be->be_rootdn;
	op->o_ndn = be->be_rootndn;

	op->o_req_dn = be->be_suffix[0];
	op->o_req_ndn = be->be_nsuffix[0];

	op->ors_scope = LDAP_SCOPE_SUBTREE;
	op->ors_deref = LDAP_DEREF_NEVER;
	op->ors_limit = NULL;
	op->ors_tlimit = SLAP_NO_LIMIT;
	op->ors_slimit = SLAP_NO_LIMIT;
	op->ors_attrs =  slap_anlist_no_attrs;

	op->o_bd = be;
	op->o_bd->bd_info = (BackendInfo *)on->on_info;

	ags.ags_info = agi;
	cb.sc_private = &ags;
	cb.sc_response = autogroup_group_add_cb;
	cb.sc_cleanup = NULL;
	cb.sc_next = NULL;

	op->o_callback = &cb;

	for (agd = agi->agi_def ; agd ; agd = agd->agd_next) {
		SlapReply	rs = { REP_RESULT };

		autogroup_build_def_filter(agd, op);

		ags.ags_def = agd;

		op->o_bd->be_search( op, &rs );

		filter_free_x( op, op->ors_filter, 1 );
		op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
	}		

	if( ! agi->agi_memberof_ad ){
		int			rc;
		const char		*text = NULL;
		
		rc = slap_str2ad( SLAPD_MEMBEROF_ATTR, &agi->agi_memberof_ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "autogroup_db_open: "
			"unable to find attribute=\"%s\": %s (%d)\n",
			SLAPD_MEMBEROF_ATTR, text, rc );
			return rc;
		}
	}

	return 0;
}

static int
autogroup_db_close(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst			*on = (slap_overinst *) be->bd_info;

	Debug( LDAP_DEBUG_TRACE, "==> autogroup_db_close\n", 0, 0, 0);

	if ( on->on_bi.bi_private ) {
		autogroup_info_t		*agi = on->on_bi.bi_private;
		autogroup_entry_t	*age = agi->agi_entry,
					*age_next;
		autogroup_filter_t	*agf, *agf_next;

		for ( age_next = age; age_next; age = age_next ) {
			age_next = age->age_next;

			ch_free( age->age_dn.bv_val );
			ch_free( age->age_ndn.bv_val );

			agf = age->age_filter;

			for ( agf_next = agf; agf_next; agf = agf_next ) {
				agf_next = agf->agf_next;

				filter_free( agf->agf_filter );
				ch_free( agf->agf_filterstr.bv_val );
				ch_free( agf->agf_dn.bv_val );
				ch_free( agf->agf_ndn.bv_val );	
				anlist_free( agf->agf_anlist, 1, NULL );
				ch_free( agf );
			}

			ldap_pvt_thread_mutex_destroy( &age->age_mutex );
			ch_free( age );
		}
	}

	return 0;
}

static int
autogroup_db_destroy(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst			*on = (slap_overinst *) be->bd_info;

	Debug( LDAP_DEBUG_TRACE, "==> autogroup_db_destroy\n", 0, 0, 0);

	if ( on->on_bi.bi_private ) {
		autogroup_info_t		*agi = on->on_bi.bi_private;
		autogroup_def_t		*agd = agi->agi_def,
					*agd_next;

		for ( agd_next = agd; agd_next; agd = agd_next ) {
			agd_next = agd->agd_next;

			ch_free( agd );
		}

		ldap_pvt_thread_mutex_destroy( &agi->agi_mutex );
		ch_free( agi );
	}

	return 0;
}

static slap_overinst	autogroup = { { NULL } };

static
int
autogroup_initialize(void)
{
	int		rc = 0;
	autogroup.on_bi.bi_type = "autogroup";

	autogroup.on_bi.bi_db_open = autogroup_db_open;
	autogroup.on_bi.bi_db_close = autogroup_db_close;
	autogroup.on_bi.bi_db_destroy = autogroup_db_destroy;

	autogroup.on_bi.bi_op_add = autogroup_add_entry;
	autogroup.on_bi.bi_op_delete = autogroup_delete_entry;
	autogroup.on_bi.bi_op_modify = autogroup_modify_entry;
	autogroup.on_bi.bi_op_modrdn = autogroup_modrdn_entry;

	autogroup.on_response = autogroup_response;

	autogroup.on_bi.bi_cf_ocs = agocs;

	rc = config_register_schema( agcfg, agocs );
	if ( rc ) {
		return rc;
	}

	return overlay_register( &autogroup );
}

int
init_module( int argc, char *argv[] )
{
	return autogroup_initialize();
}
