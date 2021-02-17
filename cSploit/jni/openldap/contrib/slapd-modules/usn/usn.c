/* usn.c - Maintain Microsoft-style Update Sequence Numbers */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2007-2014 The OpenLDAP Foundation.
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

#ifdef SLAPD_OVER_USN

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "config.h"

/* This overlay intercepts write operations and adds a Microsoft-style
 * USN to the target entry.
 */

typedef struct usn_info {
	int ui_current;
	ldap_pvt_thread_mutex_t ui_mutex;
} usn_info_t;

static AttributeDescription *ad_usnCreated, *ad_usnChanged;

static struct {
	char *desc;
	AttributeDescription **adp;
} as[] = {
	{ "( 1.2.840.113556.1.2.19 "
	    "NAME 'uSNCreated' "
	    "SYNTAX '1.2.840.113556.1.4.906' "
		"SINGLE-VALUE "
		"NO-USER-MODIFICATION )",
		&ad_usnCreated },
	{ "( 1.2.840.113556.1.2.120 "
		"NAME 'uSNChanged' "
		"SYNTAX '1.2.840.113556.1.4.906' "
		"SINGLE-VALUE "
		"NO-USER-MODIFICATION )",
		&ad_usnChanged },
	{ NULL }
};

static int
usn_func( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	usn_info_t		*ui = on->on_bi.bi_private;
	int my_usn;
	char intbuf[64];
	struct berval bv[2];

	ldap_pvt_thread_mutex_lock( &ui->ui_mutex );
	ui->ui_current++;
	my_usn = ui->ui_current;
	ldap_pvt_thread_mutex_unlock( &ui->ui_mutex );

	BER_BVZERO(&bv[1]);
	bv[0].bv_val = intbuf;
	bv[0].bv_len = snprintf( intbuf, sizeof(intbuf), "%d", my_usn );
	switch(op->o_tag) {
	case LDAP_REQ_ADD:
		attr_merge( op->ora_e, ad_usnCreated, bv, NULL );
		attr_merge( op->ora_e, ad_usnChanged, bv, NULL );
		break;
	case LDAP_REQ_DELETE:
		/* Probably need to update root usnLastObjRem */
		break;
	default: {
		/* Modify, ModDN */
		Modifications *ml, *mod = ch_calloc( sizeof( Modifications ), 1 );
		for ( ml = op->orm_modlist; ml && ml->sml_next; ml = ml->sml_next );
		ml->sml_next = mod;
		mod->sml_desc = ad_usnChanged;
		mod->sml_numvals = 1;
		value_add_one( &mod->sml_values, &bv[0] );
		mod->sml_nvalues = NULL;
		mod->sml_op = LDAP_MOD_REPLACE;
		mod->sml_flags = 0;
		mod->sml_next = NULL;
		break;
		}
	}
	return SLAP_CB_CONTINUE;
}

static int
usn_operational(
	Operation *op,
	SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	usn_info_t		*ui = (usn_info_t *)on->on_bi.bi_private;

	if ( rs->sr_entry &&
		dn_match( &rs->sr_entry->e_nname, op->o_bd->be_nsuffix )) {

		if ( SLAP_OPATTRS( rs->sr_attr_flags ) ||
			ad_inlist( ad_usnChanged, rs->sr_attrs )) {
			Attribute *a, **ap = NULL;
			char intbuf[64];
			struct berval bv;
			int my_usn;

			for ( a=rs->sr_entry->e_attrs; a; a=a->a_next ) {
				if ( a->a_desc == ad_usnChanged )
					break;
			}

			if ( !a ) {
				for ( ap = &rs->sr_operational_attrs; *ap;
					ap=&(*ap)->a_next );

					a = attr_alloc( ad_usnChanged );
					*ap = a;
				}

			if ( !ap ) {
				if ( rs_entry2modifiable( op,rs, on )) {
					a = attr_find( rs->sr_entry->e_attrs,
						ad_usnChanged );
				}
				ber_bvarray_free( a->a_vals );
				a->a_vals = NULL;
				a->a_numvals = 0;
			}
			ldap_pvt_thread_mutex_lock( &ui->ui_mutex );
			my_usn = ui->ui_current;
			ldap_pvt_thread_mutex_unlock( &ui->ui_mutex );
			bv.bv_len = snprintf( intbuf, sizeof(intbuf), "%d", my_usn );
			bv.bv_val = intbuf;
			attr_valadd( a, &bv, NULL, 1 );
		}
	}
	return SLAP_CB_CONTINUE;
}

/* Read the old USN from the underlying DB. This code is
 * stolen from the syncprov overlay.
 */
static int
usn_db_open(
	BackendDB *be,
	ConfigReply *cr)
{
	slap_overinst   *on = (slap_overinst *) be->bd_info;
	usn_info_t *ui = (usn_info_t *)on->on_bi.bi_private;

	Connection conn = { 0 };
	OperationBuffer opbuf;
	Operation *op;
	Entry *e = NULL;
	Attribute *a;
	int rc;
	void *thrctx = NULL;

	thrctx = ldap_pvt_thread_pool_context();
	connection_fake_init( &conn, &opbuf, thrctx );
	op = &opbuf.ob_op;
	op->o_bd = be;
	op->o_dn = be->be_rootdn;
	op->o_ndn = be->be_rootndn;

	rc = overlay_entry_get_ov( op, be->be_nsuffix, NULL,
		slap_schema.si_ad_contextCSN, 0, &e, on );

	if ( e ) {
		a = attr_find( e->e_attrs, ad_usnChanged );
		if ( a ) {
			ui->ui_current = atoi( a->a_vals[0].bv_val );
		}
		overlay_entry_release_ov( op, e, 0, on );
	}
	return 0;
}

static int
usn_db_init(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	usn_info_t	*ui;

	if ( SLAP_ISGLOBALOVERLAY( be ) ) {
		Debug( LDAP_DEBUG_ANY,
			"usn must be instantiated within a database.\n",
			0, 0, 0 );
		return 1;
	}

	ui = ch_calloc(1, sizeof(usn_info_t));
	ldap_pvt_thread_mutex_init( &ui->ui_mutex );
	on->on_bi.bi_private = ui;
	return 0;
}

static int
usn_db_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	usn_info_t	*ui = on->on_bi.bi_private;
	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;
	SlapReply rs = {REP_RESULT};
	void *thrctx;

	Modifications mod;
	slap_callback cb = {0};
	char intbuf[64];
	struct berval bv[2];

	thrctx = ldap_pvt_thread_pool_context();
	connection_fake_init( &conn, &opbuf, thrctx );
	op = &opbuf.ob_op;
	op->o_bd = be;
	BER_BVZERO( &bv[1] );
	bv[0].bv_len = snprintf( intbuf, sizeof(intbuf), "%d", ui->ui_current );
	bv[0].bv_val = intbuf;
	mod.sml_numvals = 1;
	mod.sml_values = bv;
	mod.sml_nvalues = NULL;
	mod.sml_desc = ad_usnChanged;
	mod.sml_op = LDAP_MOD_REPLACE;
	mod.sml_flags = 0;
	mod.sml_next = NULL;

	cb.sc_response = slap_null_cb;
	op->o_tag = LDAP_REQ_MODIFY;
	op->o_callback = &cb;
	op->orm_modlist = &mod;
	op->orm_no_opattrs = 1;
	op->o_dn = be->be_rootdn;
	op->o_ndn = be->be_rootndn;
	op->o_req_dn = op->o_bd->be_suffix[0];
	op->o_req_ndn = op->o_bd->be_nsuffix[0];
	op->o_bd->bd_info = on->on_info->oi_orig;
	op->o_managedsait = SLAP_CONTROL_NONCRITICAL;
	op->o_no_schema_check = 1;
	op->o_bd->be_modify( op, &rs );
	if ( mod.sml_next != NULL ) {
		slap_mods_free( mod.sml_next, 1 );
	}
	return 0;
}

static int
usn_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	usn_info_t	*ui = on->on_bi.bi_private;

	ldap_pvt_thread_mutex_destroy( &ui->ui_mutex );
	ch_free( ui );
	on->on_bi.bi_private = NULL;
	return 0;
}

/* This overlay is set up for dynamic loading via moduleload. For static
 * configuration, you'll need to arrange for the slap_overinst to be
 * initialized and registered by some other function inside slapd.
 */

static slap_overinst usn;

int
usn_init( void )
{
	int i, code;

	memset( &usn, 0, sizeof( slap_overinst ) );
	usn.on_bi.bi_type = "usn";
	usn.on_bi.bi_db_init = usn_db_init;
	usn.on_bi.bi_db_destroy = usn_db_destroy;
	usn.on_bi.bi_db_open = usn_db_open;
	usn.on_bi.bi_db_close = usn_db_close;

	usn.on_bi.bi_op_modify = usn_func;
	usn.on_bi.bi_op_modrdn = usn_func;
	usn.on_bi.bi_op_add = usn_func;
	usn.on_bi.bi_op_delete = usn_func;
	usn.on_bi.bi_operational = usn_operational;

	for ( i = 0; as[i].desc; i++ ) {
		code = register_at( as[i].desc, as[i].adp, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"usn_init: register_at #%d failed\n", i, 0, 0 );
			return code;
		}
	}
	return overlay_register( &usn );
}

#if SLAPD_OVER_USN == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return usn_init();
}
#endif /* SLAPD_OVER_USN == SLAPD_MOD_DYNAMIC */

#endif /* defined(SLAPD_OVER_USN) */
