/* translucent.c - translucent proxy module */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
 * Portions Copyright 2005 Symas Corporation.
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
 * This work was initially developed by Symas Corp. for inclusion in
 * OpenLDAP Software.  This work was sponsored by Hewlett-Packard.
 */

#include "portable.h"

#ifdef SLAPD_OVER_TRANSLUCENT

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "lutil.h"

#include "config.h"

/* config block */
typedef struct translucent_info {
	BackendDB db;			/* captive backend */
	AttributeName *local;	/* valid attrs for local filters */
	AttributeName *remote;	/* valid attrs for remote filters */
	int strict;
	int no_glue;
	int defer_db_open;
	int bind_local;
	int pwmod_local;
} translucent_info;

static ConfigLDAPadd translucent_ldadd;
static ConfigCfAdd translucent_cfadd;

static ConfigDriver translucent_cf_gen;

enum {
	TRANS_LOCAL = 1,
	TRANS_REMOTE
};

static ConfigTable translucentcfg[] = {
	{ "translucent_strict", "on|off", 1, 2, 0,
	  ARG_ON_OFF|ARG_OFFSET,
	  (void *)offsetof(translucent_info, strict),
	  "( OLcfgOvAt:14.1 NAME 'olcTranslucentStrict' "
	  "DESC 'Reveal attribute deletion constraint violations' "
	  "SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "translucent_no_glue", "on|off", 1, 2, 0,
	  ARG_ON_OFF|ARG_OFFSET,
	  (void *)offsetof(translucent_info, no_glue),
	  "( OLcfgOvAt:14.2 NAME 'olcTranslucentNoGlue' "
	  "DESC 'Disable automatic glue records for ADD and MODRDN' "
	  "SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "translucent_local", "attr[,attr...]", 1, 2, 0,
	  ARG_MAGIC|TRANS_LOCAL,
	  translucent_cf_gen,
	  "( OLcfgOvAt:14.3 NAME 'olcTranslucentLocal' "
	  "DESC 'Attributes to use in local search filter' "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "translucent_remote", "attr[,attr...]", 1, 2, 0,
	  ARG_MAGIC|TRANS_REMOTE,
	  translucent_cf_gen,
	  "( OLcfgOvAt:14.4 NAME 'olcTranslucentRemote' "
	  "DESC 'Attributes to use in remote search filter' "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "translucent_bind_local", "on|off", 1, 2, 0,
	  ARG_ON_OFF|ARG_OFFSET,
	  (void *)offsetof(translucent_info, bind_local),
	  "( OLcfgOvAt:14.5 NAME 'olcTranslucentBindLocal' "
	  "DESC 'Enable local bind' "
	  "SYNTAX OMsBoolean SINGLE-VALUE)", NULL, NULL },
	{ "translucent_pwmod_local", "on|off", 1, 2, 0,
	  ARG_ON_OFF|ARG_OFFSET,
	  (void *)offsetof(translucent_info, pwmod_local),
	  "( OLcfgOvAt:14.6 NAME 'olcTranslucentPwModLocal' "
	  "DESC 'Enable local RFC 3062 Password Modify extended operation' "
	  "SYNTAX OMsBoolean SINGLE-VALUE)", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs translucentocs[] = {
	{ "( OLcfgOvOc:14.1 "
	  "NAME 'olcTranslucentConfig' "
	  "DESC 'Translucent configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcTranslucentStrict $ olcTranslucentNoGlue $"
	  " olcTranslucentLocal $ olcTranslucentRemote $"
	  " olcTranslucentBindLocal $ olcTranslucentPwModLocal ) )",
	  Cft_Overlay, translucentcfg, NULL, translucent_cfadd },
	{ "( OLcfgOvOc:14.2 "
	  "NAME 'olcTranslucentDatabase' "
	  "DESC 'Translucent target database configuration' "
	  "AUXILIARY )", Cft_Misc, olcDatabaseDummy, translucent_ldadd },
	{ NULL, 0, NULL }
};
/* for translucent_init() */

static int
translucent_ldadd_cleanup( ConfigArgs *ca )
{
	slap_overinst *on = ca->ca_private;
	translucent_info *ov = on->on_bi.bi_private;

	ov->defer_db_open = 0;
	return backend_startup_one( ca->be, &ca->reply );
}

static int
translucent_ldadd( CfEntryInfo *cei, Entry *e, ConfigArgs *ca )
{
	slap_overinst *on;
	translucent_info *ov;

	Debug(LDAP_DEBUG_TRACE, "==> translucent_ldadd\n", 0, 0, 0);

	if ( cei->ce_type != Cft_Overlay || !cei->ce_bi ||
	     cei->ce_bi->bi_cf_ocs != translucentocs )
		return LDAP_CONSTRAINT_VIOLATION;

	on = (slap_overinst *)cei->ce_bi;
	ov = on->on_bi.bi_private;
	ca->be = &ov->db;
	ca->ca_private = on;
	if ( CONFIG_ONLINE_ADD( ca ))
		ca->cleanup = translucent_ldadd_cleanup;
	else
		ov->defer_db_open = 0;

	return LDAP_SUCCESS;
}

static int
translucent_cfadd( Operation *op, SlapReply *rs, Entry *e, ConfigArgs *ca )
{
	CfEntryInfo *cei = e->e_private;
	slap_overinst *on = (slap_overinst *)cei->ce_bi;
	translucent_info *ov = on->on_bi.bi_private;
	struct berval bv;

	Debug(LDAP_DEBUG_TRACE, "==> translucent_cfadd\n", 0, 0, 0);

	/* FIXME: should not hardcode "olcDatabase" here */
	bv.bv_len = snprintf( ca->cr_msg, sizeof( ca->cr_msg ),
		"olcDatabase=" SLAP_X_ORDERED_FMT "%s",
		0, ov->db.bd_info->bi_type );
	if ( bv.bv_len >= sizeof( ca->cr_msg ) ) {
		return -1;
	}
	bv.bv_val = ca->cr_msg;
	ca->be = &ov->db;
	ov->defer_db_open = 0;

	/* We can only create this entry if the database is table-driven
	 */
	if ( ov->db.bd_info->bi_cf_ocs )
		config_build_entry( op, rs, cei, ca, &bv,
				    ov->db.bd_info->bi_cf_ocs,
				    &translucentocs[1] );

	return 0;
}

static int
translucent_cf_gen( ConfigArgs *c )
{
	slap_overinst	*on = (slap_overinst *)c->bi;
	translucent_info *ov = on->on_bi.bi_private;
	AttributeName **an, *a2;
	int i;

	if ( c->type == TRANS_LOCAL )
		an = &ov->local;
	else
		an = &ov->remote;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		if ( !*an )
			return 1;
		for ( i = 0; !BER_BVISNULL(&(*an)[i].an_name); i++ ) {
			value_add_one( &c->rvalue_vals, &(*an)[i].an_name );
		}
		return ( i < 1 );
	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( c->valx < 0 ) {
			anlist_free( *an, 1, NULL );
			*an = NULL;
		} else {
			i = c->valx;
			ch_free( (*an)[i].an_name.bv_val );
			do {
				(*an)[i] = (*an)[i+1];
				i++;
			} while ( !BER_BVISNULL( &(*an)[i].an_name ));
		}
		return 0;
	}
	a2 = str2anlist( *an, c->argv[1], "," );
	if ( !a2 ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s unable to parse attribute %s",
			c->argv[0], c->argv[1] );
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s: %s\n", c->log, c->cr_msg, 0 );
		return ARG_BAD_CONF;
	}
	*an = a2;
	return 0;
}

static slap_overinst translucent;

/*
** glue_parent()
**	call syncrepl_add_glue() with the parent suffix;
**
*/

static struct berval glue[] = { BER_BVC("top"), BER_BVC("glue"), BER_BVNULL };

void glue_parent(Operation *op) {
	Operation nop = *op;
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	struct berval ndn = BER_BVNULL;
	Attribute *a;
	Entry *e;
	struct berval	pdn;

	dnParent( &op->o_req_ndn, &pdn );
	ber_dupbv_x( &ndn, &pdn, op->o_tmpmemctx );

	Debug(LDAP_DEBUG_TRACE, "=> glue_parent: fabricating glue for <%s>\n", ndn.bv_val, 0, 0);

	e = entry_alloc();
	e->e_id = NOID;
	ber_dupbv(&e->e_name, &ndn);
	ber_dupbv(&e->e_nname, &ndn);

	a = attr_alloc( slap_schema.si_ad_objectClass );
	a->a_numvals = 2;
	a->a_vals = ch_malloc(sizeof(struct berval) * 3);
	ber_dupbv(&a->a_vals[0], &glue[0]);
	ber_dupbv(&a->a_vals[1], &glue[1]);
	ber_dupbv(&a->a_vals[2], &glue[2]);
	a->a_nvals = a->a_vals;
	a->a_next = e->e_attrs;
	e->e_attrs = a;

	a = attr_alloc( slap_schema.si_ad_structuralObjectClass );
	a->a_numvals = 1;
	a->a_vals = ch_malloc(sizeof(struct berval) * 2);
	ber_dupbv(&a->a_vals[0], &glue[1]);
	ber_dupbv(&a->a_vals[1], &glue[2]);
	a->a_nvals = a->a_vals;
	a->a_next = e->e_attrs;
	e->e_attrs = a;

	nop.o_req_dn = ndn;
	nop.o_req_ndn = ndn;
	nop.ora_e = e;

	nop.o_bd->bd_info = (BackendInfo *) on->on_info->oi_orig;
	syncrepl_add_glue(&nop, e);
	nop.o_bd->bd_info = (BackendInfo *) on;

	op->o_tmpfree( ndn.bv_val, op->o_tmpmemctx );

	return;
}

/*
** free_attr_chain()
**	free only the Attribute*, not the contents;
**
*/
void free_attr_chain(Attribute *b) {
	Attribute *a;
	for(a=b; a; a=a->a_next) {
		a->a_vals = NULL;
		a->a_nvals = NULL;
	}
	attrs_free( b );
	return;
}

/*
** translucent_add()
**	if not bound as root, send ACCESS error;
**	if glue, glue_parent();
**	return CONTINUE;
**
*/

static int translucent_add(Operation *op, SlapReply *rs) {
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	Debug(LDAP_DEBUG_TRACE, "==> translucent_add: %s\n",
		op->o_req_dn.bv_val, 0, 0);
	if(!be_isroot(op)) {
		op->o_bd->bd_info = (BackendInfo *) on->on_info;
		send_ldap_error(op, rs, LDAP_INSUFFICIENT_ACCESS,
			"user modification of overlay database not permitted");
		op->o_bd->bd_info = (BackendInfo *) on;
		return(rs->sr_err);
	}
	if(!ov->no_glue) glue_parent(op);
	return(SLAP_CB_CONTINUE);
}

/*
** translucent_modrdn()
**	if not bound as root, send ACCESS error;
**	if !glue, glue_parent();
**	else return CONTINUE;
**
*/

static int translucent_modrdn(Operation *op, SlapReply *rs) {
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	Debug(LDAP_DEBUG_TRACE, "==> translucent_modrdn: %s -> %s\n",
		op->o_req_dn.bv_val, op->orr_newrdn.bv_val, 0);
	if(!be_isroot(op)) {
		op->o_bd->bd_info = (BackendInfo *) on->on_info;
		send_ldap_error(op, rs, LDAP_INSUFFICIENT_ACCESS,
			"user modification of overlay database not permitted");
		op->o_bd->bd_info = (BackendInfo *) on;
		return(rs->sr_err);
	}
	if(!ov->no_glue) {
		op->o_tag = LDAP_REQ_ADD;
		glue_parent(op);
		op->o_tag = LDAP_REQ_MODRDN;
	}
	return(SLAP_CB_CONTINUE);
}

/*
** translucent_delete()
**	if not bound as root, send ACCESS error;
**	else return CONTINUE;
**
*/

static int translucent_delete(Operation *op, SlapReply *rs) {
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	Debug(LDAP_DEBUG_TRACE, "==> translucent_delete: %s\n",
		op->o_req_dn.bv_val, 0, 0);
	if(!be_isroot(op)) {
		op->o_bd->bd_info = (BackendInfo *) on->on_info;
		send_ldap_error(op, rs, LDAP_INSUFFICIENT_ACCESS,
			"user modification of overlay database not permitted");
		op->o_bd->bd_info = (BackendInfo *) on;
		return(rs->sr_err);
	}
	return(SLAP_CB_CONTINUE);
}

static int
translucent_tag_cb( Operation *op, SlapReply *rs )
{
	op->o_tag = LDAP_REQ_MODIFY;
	op->orm_modlist = op->o_callback->sc_private;
	rs->sr_tag = slap_req2res( op->o_tag );

	return SLAP_CB_CONTINUE;
}

/*
** translucent_modify()
**	modify in local backend if exists in both;
**	otherwise, add to local backend;
**	fail if not defined in captive backend;
**
*/

static int translucent_modify(Operation *op, SlapReply *rs) {
	SlapReply nrs = { REP_RESULT };

	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	Entry *e = NULL, *re = NULL;
	Attribute *a, *ax;
	Modifications *m, **mm;
	BackendDB *db;
	int del, rc, erc = 0;
	slap_callback cb = { 0 };

	Debug(LDAP_DEBUG_TRACE, "==> translucent_modify: %s\n",
		op->o_req_dn.bv_val, 0, 0);

	if(ov->defer_db_open) {
		send_ldap_error(op, rs, LDAP_UNAVAILABLE,
			"remote DB not available");
		return(rs->sr_err);
	}
/*
** fetch entry from the captive backend;
** if it did not exist, fail;
** release it, if captive backend supports this;
**
*/

	db = op->o_bd;
	op->o_bd = &ov->db;
	ov->db.be_acl = op->o_bd->be_acl;
	rc = ov->db.bd_info->bi_entry_get_rw(op, &op->o_req_ndn, NULL, NULL, 0, &re);
	if(rc != LDAP_SUCCESS || re == NULL ) {
		send_ldap_error((op), rs, LDAP_NO_SUCH_OBJECT,
			"attempt to modify nonexistent local record");
		return(rs->sr_err);
	}
	op->o_bd = db;
/*
** fetch entry from local backend;
** if it exists:
**	foreach Modification:
**	    if attr not present in local:
**		if Mod == LDAP_MOD_DELETE:
**		    if remote attr not present, return NO_SUCH;
**		    if remote attr present, drop this Mod;
**		else force this Mod to LDAP_MOD_ADD;
**	return CONTINUE;
**
*/

	op->o_bd->bd_info = (BackendInfo *) on->on_info;
	rc = be_entry_get_rw(op, &op->o_req_ndn, NULL, NULL, 0, &e);
	op->o_bd->bd_info = (BackendInfo *) on;

	if(e && rc == LDAP_SUCCESS) {
		Debug(LDAP_DEBUG_TRACE, "=> translucent_modify: found local entry\n", 0, 0, 0);
		for(mm = &op->orm_modlist; *mm; ) {
			m = *mm;
			for(a = e->e_attrs; a; a = a->a_next)
				if(a->a_desc == m->sml_desc) break;
			if(a) {
				mm = &m->sml_next;
				continue;		/* found local attr */
			}
			if(m->sml_op == LDAP_MOD_DELETE) {
				for(a = re->e_attrs; a; a = a->a_next)
					if(a->a_desc == m->sml_desc) break;
				/* not found remote attr */
				if(!a) {
					erc = LDAP_NO_SUCH_ATTRIBUTE;
					goto release;
				}
				if(ov->strict) {
					erc = LDAP_CONSTRAINT_VIOLATION;
					goto release;
				}
				Debug(LDAP_DEBUG_TRACE,
					"=> translucent_modify: silently dropping delete: %s\n",
					m->sml_desc->ad_cname.bv_val, 0, 0);
				*mm = m->sml_next;
				m->sml_next = NULL;
				slap_mods_free(m, 1);
				continue;
			}
			m->sml_op = LDAP_MOD_ADD;
			mm = &m->sml_next;
		}
		erc = SLAP_CB_CONTINUE;
release:
		if(re) {
			if(ov->db.bd_info->bi_entry_release_rw) {
				op->o_bd = &ov->db;
				ov->db.bd_info->bi_entry_release_rw(op, re, 0);
				op->o_bd = db;
			} else
				entry_free(re);
		}
		op->o_bd->bd_info = (BackendInfo *) on->on_info;
		be_entry_release_r(op, e);
		op->o_bd->bd_info = (BackendInfo *) on;
		if(erc == SLAP_CB_CONTINUE) {
			return(erc);
		} else if(erc) {
			send_ldap_error(op, rs, erc,
				"attempt to delete nonexistent attribute");
			return(erc);
		}
	}

	/* don't leak remote entry copy */
	if(re) {
		if(ov->db.bd_info->bi_entry_release_rw) {
			op->o_bd = &ov->db;
			ov->db.bd_info->bi_entry_release_rw(op, re, 0);
			op->o_bd = db;
		} else
			entry_free(re);
	}
/*
** foreach Modification:
**	if MOD_ADD or MOD_REPLACE, add Attribute;
** if no Modifications were suitable:
**	if strict, throw CONSTRAINT_VIOLATION;
**	else, return early SUCCESS;
** fabricate Entry with new Attribute chain;
** glue_parent() for this Entry;
** call bi_op_add() in local backend;
**
*/

	Debug(LDAP_DEBUG_TRACE, "=> translucent_modify: fabricating local add\n", 0, 0, 0);
	a = NULL;
	for(del = 0, ax = NULL, m = op->orm_modlist; m; m = m->sml_next) {
		Attribute atmp;
		if(((m->sml_op & LDAP_MOD_OP) != LDAP_MOD_ADD) &&
		   ((m->sml_op & LDAP_MOD_OP) != LDAP_MOD_REPLACE)) {
			Debug(LDAP_DEBUG_ANY,
				"=> translucent_modify: silently dropped modification(%d): %s\n",
				m->sml_op, m->sml_desc->ad_cname.bv_val, 0);
			if((m->sml_op & LDAP_MOD_OP) == LDAP_MOD_DELETE) del++;
			continue;
		}
		atmp.a_desc = m->sml_desc;
		atmp.a_vals = m->sml_values;
		atmp.a_nvals = m->sml_nvalues ? m->sml_nvalues : atmp.a_vals;
		atmp.a_numvals = m->sml_numvals;
		atmp.a_flags = 0;
		a = attr_dup( &atmp );
		a->a_next  = ax;
		ax = a;
	}

	if(del && ov->strict) {
		attrs_free( a );
		send_ldap_error(op, rs, LDAP_CONSTRAINT_VIOLATION,
			"attempt to delete attributes from local database");
		return(rs->sr_err);
	}

	if(!ax) {
		if(ov->strict) {
			send_ldap_error(op, rs, LDAP_CONSTRAINT_VIOLATION,
				"modification contained other than ADD or REPLACE");
			return(rs->sr_err);
		}
		/* rs->sr_text = "no valid modification found"; */
		rs->sr_err = LDAP_SUCCESS;
		send_ldap_result(op, rs);
		return(rs->sr_err);
	}

	e = entry_alloc();
	ber_dupbv( &e->e_name, &op->o_req_dn );
	ber_dupbv( &e->e_nname, &op->o_req_ndn );
	e->e_attrs = a;

	op->o_tag	= LDAP_REQ_ADD;
	cb.sc_response = translucent_tag_cb;
	cb.sc_private = op->orm_modlist;
	op->oq_add.rs_e	= e;

	glue_parent(op);

	cb.sc_next = op->o_callback;
	op->o_callback = &cb;
	rc = on->on_info->oi_orig->bi_op_add(op, &nrs);
	if ( op->ora_e == e )
		entry_free( e );
	op->o_callback = cb.sc_next;

	return(rc);
}

static int translucent_compare(Operation *op, SlapReply *rs) {
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	AttributeAssertion *ava = op->orc_ava;
	Entry *e = NULL;
	BackendDB *db;
	int rc;

	Debug(LDAP_DEBUG_TRACE, "==> translucent_compare: <%s> %s:%s\n",
		op->o_req_dn.bv_val, ava->aa_desc->ad_cname.bv_val, ava->aa_value.bv_val);

/*
** if the local backend has an entry for this attribute:
**	CONTINUE and let it do the compare;
**
*/
	rc = overlay_entry_get_ov(op, &op->o_req_ndn, NULL, ava->aa_desc, 0, &e, on);
	if(rc == LDAP_SUCCESS && e) {
		overlay_entry_release_ov(op, e, 0, on);
		return(SLAP_CB_CONTINUE);
	}

	if(ov->defer_db_open) {
		send_ldap_error(op, rs, LDAP_UNAVAILABLE,
			"remote DB not available");
		return(rs->sr_err);
	}
/*
** call compare() in the captive backend;
** return the result;
**
*/
	db = op->o_bd;
	op->o_bd = &ov->db;
	ov->db.be_acl = op->o_bd->be_acl;
	rc = ov->db.bd_info->bi_op_compare(op, rs);
	op->o_bd = db;

	return(rc);
}

static int translucent_pwmod(Operation *op, SlapReply *rs) {
	SlapReply nrs = { REP_RESULT };
	Operation nop;

	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	Entry *e = NULL, *re = NULL;
	BackendDB *db;
	int rc = 0;
	slap_callback cb = { 0 };

	if (!ov->pwmod_local) {
		rs->sr_err = LDAP_CONSTRAINT_VIOLATION,
		rs->sr_text = "attempt to modify password in local database";
		return rs->sr_err;
	}

/*
** fetch entry from the captive backend;
** if it did not exist, fail;
** release it, if captive backend supports this;
**
*/
	db = op->o_bd;
	op->o_bd = &ov->db;
	ov->db.be_acl = op->o_bd->be_acl;
	rc = ov->db.bd_info->bi_entry_get_rw(op, &op->o_req_ndn, NULL, NULL, 0, &re);
	if(rc != LDAP_SUCCESS || re == NULL ) {
		send_ldap_error((op), rs, LDAP_NO_SUCH_OBJECT,
			"attempt to modify nonexistent local record");
		return(rs->sr_err);
	}
	op->o_bd = db;
/*
** fetch entry from local backend;
** if it exists:
**	return CONTINUE;
*/

	op->o_bd->bd_info = (BackendInfo *) on->on_info;
	rc = be_entry_get_rw(op, &op->o_req_ndn, NULL, NULL, 0, &e);
	op->o_bd->bd_info = (BackendInfo *) on;

	if(e && rc == LDAP_SUCCESS) {
		if(re) {
			if(ov->db.bd_info->bi_entry_release_rw) {
				op->o_bd = &ov->db;
				ov->db.bd_info->bi_entry_release_rw(op, re, 0);
				op->o_bd = db;
			} else {
				entry_free(re);
			}
		}
		op->o_bd->bd_info = (BackendInfo *) on->on_info;
		be_entry_release_r(op, e);
		op->o_bd->bd_info = (BackendInfo *) on;
		return SLAP_CB_CONTINUE;
	}

	/* don't leak remote entry copy */
	if(re) {
		if(ov->db.bd_info->bi_entry_release_rw) {
			op->o_bd = &ov->db;
			ov->db.bd_info->bi_entry_release_rw(op, re, 0);
			op->o_bd = db;
		} else {
			entry_free(re);
		}
	}
/*
** glue_parent() for this Entry;
** call bi_op_add() in local backend;
**
*/
	e = entry_alloc();
	ber_dupbv( &e->e_name, &op->o_req_dn );
	ber_dupbv( &e->e_nname, &op->o_req_ndn );
	e->e_attrs = NULL;

	nop = *op;
	nop.o_tag = LDAP_REQ_ADD;
	cb.sc_response = slap_null_cb;
	nop.oq_add.rs_e	= e;

	glue_parent(&nop);

	nop.o_callback = &cb;
	rc = on->on_info->oi_orig->bi_op_add(&nop, &nrs);
	if ( nop.ora_e == e ) {
		entry_free( e );
	}

	if ( rc == LDAP_SUCCESS ) {
		return SLAP_CB_CONTINUE;
	}

	return rc;
}

static int translucent_exop(Operation *op, SlapReply *rs) {
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	const struct berval bv_exop_pwmod = BER_BVC(LDAP_EXOP_MODIFY_PASSWD);

	Debug(LDAP_DEBUG_TRACE, "==> translucent_exop: %s\n",
		op->o_req_dn.bv_val, 0, 0);

	if(ov->defer_db_open) {
		send_ldap_error(op, rs, LDAP_UNAVAILABLE,
			"remote DB not available");
		return(rs->sr_err);
	}

	if ( bvmatch( &bv_exop_pwmod, &op->ore_reqoid ) ) {
		return translucent_pwmod( op, rs );
	}

	return SLAP_CB_CONTINUE;
}

/*
** translucent_search_cb()
**	merge local data with remote data
**
** Four cases:
** 1: remote search, no local filter
**	merge data and send immediately
** 2: remote search, with local filter
**	merge data and save
** 3: local search, no remote filter
**	merge data and send immediately
** 4: local search, with remote filter
**	check list, merge, send, delete
*/

#define	RMT_SIDE	0
#define	LCL_SIDE	1
#define	USE_LIST	2

typedef struct trans_ctx {
	BackendDB *db;
	slap_overinst *on;
	Filter *orig;
	Avlnode *list;
	int step;
	int slimit;
	AttributeName *attrs;
} trans_ctx;

static int translucent_search_cb(Operation *op, SlapReply *rs) {
	trans_ctx *tc;
	BackendDB *db;
	slap_overinst *on;
	translucent_info *ov;
	Entry *le, *re;
	Attribute *a, *ax, *an, *as = NULL;
	int rc;
	int test_f = 0;

	tc = op->o_callback->sc_private;

	/* Don't let the op complete while we're gathering data */
	if ( rs->sr_type == REP_RESULT && ( tc->step & USE_LIST ))
		return 0;

	if(!op || !rs || rs->sr_type != REP_SEARCH || !rs->sr_entry)
		return(SLAP_CB_CONTINUE);

	Debug(LDAP_DEBUG_TRACE, "==> translucent_search_cb: %s\n",
		rs->sr_entry->e_name.bv_val, 0, 0);

	op->ors_slimit = tc->slimit + ( tc->slimit > 0 ? 1 : 0 );
	if ( op->ors_attrs == slap_anlist_all_attributes ) {
		op->ors_attrs = tc->attrs;
		rs->sr_attrs = tc->attrs;
		rs->sr_attr_flags = slap_attr_flags( rs->sr_attrs );
	}

	on = tc->on;
	ov = on->on_bi.bi_private;

	db = op->o_bd;
	re = NULL;

	/* If we have local, get remote */
	if ( tc->step & LCL_SIDE ) {
		le = rs->sr_entry;
		/* If entry is already on list, use it */
		if ( tc->step & USE_LIST ) {
			re = tavl_delete( &tc->list, le, entry_dn_cmp );
			if ( re ) {
				rs_flush_entry( op, rs, on );
				rc = test_filter( op, re, tc->orig );
				if ( rc == LDAP_COMPARE_TRUE ) {
					rs->sr_flags |= REP_ENTRY_MUSTBEFREED;
					rs->sr_entry = re;

					if ( tc->slimit >= 0 && rs->sr_nentries >= tc->slimit ) {
						return LDAP_SIZELIMIT_EXCEEDED;
					}

					return SLAP_CB_CONTINUE;
				} else {
					entry_free( re );
					rs->sr_entry = NULL;
					return 0;
				}
			}
		}
		op->o_bd = &ov->db;
		rc = be_entry_get_rw( op, &rs->sr_entry->e_nname, NULL, NULL, 0, &re );
		if ( rc == LDAP_SUCCESS && re ) {
			Entry *tmp = entry_dup( re );
			be_entry_release_r( op, re );
			re = tmp;
			test_f = 1;
		}
	} else {
	/* Else we have remote, get local */
		op->o_bd = tc->db;
		le = NULL;
		rc = overlay_entry_get_ov(op, &rs->sr_entry->e_nname, NULL, NULL, 0, &le, on);
		if ( rc == LDAP_SUCCESS && le ) {
			re = entry_dup( rs->sr_entry );
			rs_flush_entry( op, rs, on );
		} else {
			le = NULL;
		}
	}

/*
** if we got remote and local entry:
**	foreach local attr:
**		foreach remote attr:
**			if match, remote attr with local attr;
**			if new local, add to list;
**	append new local attrs to remote;
**
*/

	if ( re && le ) {
		for(ax = le->e_attrs; ax; ax = ax->a_next) {
			for(a = re->e_attrs; a; a = a->a_next) {
				if(a->a_desc == ax->a_desc) {
					test_f = 1;
					if(a->a_vals != a->a_nvals)
						ber_bvarray_free(a->a_nvals);
					ber_bvarray_free(a->a_vals);
					ber_bvarray_dup_x( &a->a_vals, ax->a_vals, NULL );
					if ( ax->a_vals == ax->a_nvals ) {
						a->a_nvals = a->a_vals;
					} else {
						ber_bvarray_dup_x( &a->a_nvals, ax->a_nvals, NULL );
					}
					break;
				}
			}
			if(a) continue;
			an = attr_dup(ax);
			an->a_next = as;
			as = an;
		}
		/* Dispose of local entry */
		if ( tc->step & LCL_SIDE ) {
			rs_flush_entry(op, rs, on);
		} else {
			overlay_entry_release_ov(op, le, 0, on);
		}

		/* literally append, so locals are always last */
		if(as) {
			if(re->e_attrs) {
				for(ax = re->e_attrs; ax->a_next; ax = ax->a_next);
				ax->a_next = as;
			} else {
				re->e_attrs = as;
			}
		}
		/* If both filters, save entry for later */
		if ( tc->step == (USE_LIST|RMT_SIDE) ) {
			tavl_insert( &tc->list, re, entry_dn_cmp, avl_dup_error );
			rs->sr_entry = NULL;
			rc = 0;
		} else {
		/* send it now */
			rs->sr_entry = re;
			rs->sr_flags |= REP_ENTRY_MUSTBEFREED;
			if ( test_f ) {
				rc = test_filter( op, rs->sr_entry, tc->orig );
				if ( rc == LDAP_COMPARE_TRUE ) {
					rc = SLAP_CB_CONTINUE;
				} else {
					rc = 0;
				}
			} else {
				rc = SLAP_CB_CONTINUE;
			}
		}
	} else if ( le ) {
	/* Only a local entry: remote was deleted
	 * Ought to delete the local too...
	 */
	 	rc = 0;
	} else if ( tc->step & USE_LIST ) {
	/* Only a remote entry, but both filters:
	 * Test the complete filter
	 */
		rc = test_filter( op, rs->sr_entry, tc->orig );
		if ( rc == LDAP_COMPARE_TRUE ) {
			rc = SLAP_CB_CONTINUE;
		} else {
			rc = 0;
		}
	} else {
	/* Only a remote entry, only remote filter:
	 * just pass thru
	 */
		rc = SLAP_CB_CONTINUE;
	}

	op->o_bd = db;

	if ( rc == SLAP_CB_CONTINUE && tc->slimit >= 0 && rs->sr_nentries >= tc->slimit ) {
		return LDAP_SIZELIMIT_EXCEEDED;
	}

	return rc;
}

/* Dup the filter, excluding invalid elements */
static Filter *
trans_filter_dup(Operation *op, Filter *f, AttributeName *an)
{
	Filter *n = NULL;

	if ( !f )
		return NULL;

	switch( f->f_choice & SLAPD_FILTER_MASK ) {
	case SLAPD_FILTER_COMPUTED:
		n = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
		n->f_choice = f->f_choice;
		n->f_result = f->f_result;
		n->f_next = NULL;
		break;

	case LDAP_FILTER_PRESENT:
		if ( ad_inlist( f->f_desc, an )) {
			n = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
			n->f_choice = f->f_choice;
			n->f_desc = f->f_desc;
			n->f_next = NULL;
		}
		break;

	case LDAP_FILTER_EQUALITY:
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
	case LDAP_FILTER_APPROX:
	case LDAP_FILTER_SUBSTRINGS:
	case LDAP_FILTER_EXT:
		if ( !f->f_av_desc || ad_inlist( f->f_av_desc, an )) {
			n = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
			n->f_choice = f->f_choice;
			n->f_ava = f->f_ava;
			n->f_next = NULL;
		}
		break;

	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
	case LDAP_FILTER_NOT: {
		Filter **p;

		n = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
		n->f_choice = f->f_choice;
		n->f_next = NULL;

		for ( p = &n->f_list, f = f->f_list; f; f = f->f_next ) {
			*p = trans_filter_dup( op, f, an );
			if ( !*p )
				continue;
			p = &(*p)->f_next;
		}
		/* nothing valid in this list */
		if ( !n->f_list ) {
			op->o_tmpfree( n, op->o_tmpmemctx );
			return NULL;
		}
		/* Only 1 element in this list */
		if ((n->f_choice & SLAPD_FILTER_MASK) != LDAP_FILTER_NOT &&
			!n->f_list->f_next ) {
			f = n->f_list;
			*n = *f;
			op->o_tmpfree( f, op->o_tmpmemctx );
		}
		break;
	}
	}
	return n;
}

static void
trans_filter_free( Operation *op, Filter *f )
{
	Filter *n, *p, *next;

	f->f_choice &= SLAPD_FILTER_MASK;

	switch( f->f_choice ) {
	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
	case LDAP_FILTER_NOT:
		/* Free in reverse order */
		n = NULL;
		for ( p = f->f_list; p; p = next ) {
			next = p->f_next;
			p->f_next = n;
			n = p;
		}
		for ( p = n; p; p = next ) {
			next = p->f_next;
			trans_filter_free( op, p );
		}
		break;
	default:
		break;
	}
	op->o_tmpfree( f, op->o_tmpmemctx );
}

/*
** translucent_search()
**	search via captive backend;
**	override results with any local data;
**
*/

static int translucent_search(Operation *op, SlapReply *rs) {
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	slap_callback cb = { NULL, NULL, NULL, NULL };
	trans_ctx tc;
	Filter *fl, *fr;
	struct berval fbv;
	int rc = 0;

	Debug(LDAP_DEBUG_TRACE, "==> translucent_search: <%s> %s\n",
		op->o_req_dn.bv_val, op->ors_filterstr.bv_val, 0);

	if(ov->defer_db_open) {
		send_ldap_error(op, rs, LDAP_UNAVAILABLE,
			"remote DB not available");
		return(rs->sr_err);
	}

	fr = ov->remote ? trans_filter_dup( op, op->ors_filter, ov->remote ) : NULL;
	fl = ov->local ? trans_filter_dup( op, op->ors_filter, ov->local ) : NULL;
	cb.sc_response = (slap_response *) translucent_search_cb;
	cb.sc_private = &tc;
	cb.sc_next = op->o_callback;

	ov->db.be_acl = op->o_bd->be_acl;
	tc.db = op->o_bd;
	tc.on = on;
	tc.orig = op->ors_filter;
	tc.list = NULL;
	tc.step = 0;
	tc.slimit = op->ors_slimit;
	tc.attrs = NULL;
	fbv = op->ors_filterstr;

	op->o_callback = &cb;

	if ( fr || !fl ) {
		tc.attrs = op->ors_attrs;
		op->ors_slimit = SLAP_NO_LIMIT;
		op->ors_attrs = slap_anlist_all_attributes;
		op->o_bd = &ov->db;
		tc.step |= RMT_SIDE;
		if ( fl ) {
			tc.step |= USE_LIST;
			op->ors_filter = fr;
			filter2bv_x( op, fr, &op->ors_filterstr );
		}
		rc = ov->db.bd_info->bi_op_search(op, rs);
		if ( op->ors_attrs == slap_anlist_all_attributes )
			op->ors_attrs = tc.attrs;
		op->o_bd = tc.db;
		if ( fl ) {
			op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
		}
	}
	if ( fl && !rc ) {
		tc.step |= LCL_SIDE;
		op->ors_filter = fl;
		filter2bv_x( op, fl, &op->ors_filterstr );
		rc = overlay_op_walk( op, rs, op_search, on->on_info, on->on_next );
		op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
	}
	op->ors_filterstr = fbv;
	op->ors_filter = tc.orig;
	op->o_callback = cb.sc_next;
	rs->sr_attrs = op->ors_attrs;
	rs->sr_attr_flags = slap_attr_flags( rs->sr_attrs );

	/* Send out anything remaining on the list and finish */
	if ( tc.step & USE_LIST ) {
		if ( tc.list ) {
			Avlnode *av;

			av = tavl_end( tc.list, TAVL_DIR_LEFT );
			while ( av ) {
				rs->sr_entry = av->avl_data;
				if ( rc == LDAP_SUCCESS && LDAP_COMPARE_TRUE ==
					test_filter( op, rs->sr_entry, op->ors_filter ))
				{
					rs->sr_flags = REP_ENTRY_MUSTBEFREED;
					rc = send_search_entry( op, rs );
				} else {
					entry_free( rs->sr_entry );
				}
				av = tavl_next( av, TAVL_DIR_RIGHT );
			}
			tavl_free( tc.list, NULL );
			rs->sr_flags = 0;
			rs->sr_entry = NULL;
		}
		send_ldap_result( op, rs );
	}

	op->ors_slimit = tc.slimit;

	/* Free in reverse order */
	if ( fl )
		trans_filter_free( op, fl );
	if ( fr )
		trans_filter_free( op, fr );

	return rc;
}


/*
** translucent_bind()
**	pass bind request to captive backend;
**
*/

static int translucent_bind(Operation *op, SlapReply *rs) {
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	BackendDB *db;
	slap_callback sc = { 0 }, *save_cb;
	int rc;

	Debug(LDAP_DEBUG_TRACE, "translucent_bind: <%s> method %d\n",
		op->o_req_dn.bv_val, op->orb_method, 0);

	if(ov->defer_db_open) {
		send_ldap_error(op, rs, LDAP_UNAVAILABLE,
			"remote DB not available");
		return(rs->sr_err);
	}

	if (ov->bind_local) {
		sc.sc_response = slap_null_cb;
		save_cb = op->o_callback;
		op->o_callback = &sc;
	}

	db = op->o_bd;
	op->o_bd = &ov->db;
	ov->db.be_acl = op->o_bd->be_acl;
	rc = ov->db.bd_info->bi_op_bind(op, rs);
	op->o_bd = db;

	if (ov->bind_local) {
		op->o_callback = save_cb;
		if (rc != LDAP_SUCCESS) {
			rc = SLAP_CB_CONTINUE;
		}
	}

	return rc;
}

/*
** translucent_connection_destroy()
**	pass disconnect notification to captive backend;
**
*/

static int translucent_connection_destroy(BackendDB *be, Connection *conn) {
	slap_overinst *on = (slap_overinst *) be->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	int rc = 0;

	Debug(LDAP_DEBUG_TRACE, "translucent_connection_destroy\n", 0, 0, 0);

	rc = ov->db.bd_info->bi_connection_destroy(&ov->db, conn);

	return(rc);
}

/*
** translucent_db_config()
**	pass config directives to captive backend;
**	parse unrecognized directives ourselves;
**
*/

static int translucent_db_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	translucent_info *ov = on->on_bi.bi_private;

	Debug(LDAP_DEBUG_TRACE, "==> translucent_db_config: %s\n",
	      argc ? argv[0] : "", 0, 0);

	/* Something for the captive database? */
	if ( ov->db.bd_info && ov->db.bd_info->bi_db_config )
		return ov->db.bd_info->bi_db_config( &ov->db, fname, lineno,
			argc, argv );
	return SLAP_CONF_UNKNOWN;
}

/*
** translucent_db_init()
**	initialize the captive backend;
**
*/

static int translucent_db_init(BackendDB *be, ConfigReply *cr) {
	slap_overinst *on = (slap_overinst *) be->bd_info;
	translucent_info *ov;

	Debug(LDAP_DEBUG_TRACE, "==> translucent_db_init\n", 0, 0, 0);

	ov = ch_calloc(1, sizeof(translucent_info));
	on->on_bi.bi_private = ov;
	ov->db = *be;
	ov->db.be_private = NULL;
	ov->defer_db_open = 1;

	if ( !backend_db_init( "ldap", &ov->db, -1, NULL )) {
		Debug( LDAP_DEBUG_CONFIG, "translucent: unable to open captive back-ldap\n", 0, 0, 0);
		return 1;
	}
	SLAP_DBFLAGS(be) |= SLAP_DBFLAG_NO_SCHEMA_CHECK;
	SLAP_DBFLAGS(be) |= SLAP_DBFLAG_NOLASTMOD;

	return 0;
}

/*
** translucent_db_open()
**	if the captive backend has an open() method, call it;
**
*/

static int translucent_db_open(BackendDB *be, ConfigReply *cr) {
	slap_overinst *on = (slap_overinst *) be->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	int rc;

	Debug(LDAP_DEBUG_TRACE, "==> translucent_db_open\n", 0, 0, 0);

	/* need to inherit something from the original database... */
	ov->db.be_def_limit = be->be_def_limit;
	ov->db.be_limits = be->be_limits;
	ov->db.be_acl = be->be_acl;
	ov->db.be_dfltaccess = be->be_dfltaccess;

	if ( ov->defer_db_open )
		return 0;

	rc = backend_startup_one( &ov->db, cr );

	if(rc) Debug(LDAP_DEBUG_TRACE,
		"translucent: bi_db_open() returned error %d\n", rc, 0, 0);

	return(rc);
}

/*
** translucent_db_close()
**	if the captive backend has a close() method, call it
**
*/

static int
translucent_db_close( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	int rc = 0;

	Debug(LDAP_DEBUG_TRACE, "==> translucent_db_close\n", 0, 0, 0);

	if ( ov && ov->db.bd_info && ov->db.bd_info->bi_db_close ) {
		rc = ov->db.bd_info->bi_db_close(&ov->db, NULL);
	}

	return(rc);
}

/*
** translucent_db_destroy()
**	if the captive backend has a db_destroy() method, call it;
**	free any config data
**
*/

static int
translucent_db_destroy( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	translucent_info *ov = on->on_bi.bi_private;
	int rc = 0;

	Debug(LDAP_DEBUG_TRACE, "==> translucent_db_destroy\n", 0, 0, 0);

	if ( ov ) {
		if ( ov->remote )
			anlist_free( ov->remote, 1, NULL );
		if ( ov->local )
			anlist_free( ov->local, 1, NULL );
		if ( ov->db.be_private != NULL ) {
			backend_stopdown_one( &ov->db );
		}

		ldap_pvt_thread_mutex_destroy( &ov->db.be_pcl_mutex );
		ch_free(ov);
		on->on_bi.bi_private = NULL;
	}

	return(rc);
}

/*
** translucent_initialize()
**	initialize the slap_overinst with our entry points;
**
*/

int translucent_initialize() {

	int rc;

	Debug(LDAP_DEBUG_TRACE, "==> translucent_initialize\n", 0, 0, 0);

	translucent.on_bi.bi_type	= "translucent";
	translucent.on_bi.bi_db_init	= translucent_db_init;
	translucent.on_bi.bi_db_config	= translucent_db_config;
	translucent.on_bi.bi_db_open	= translucent_db_open;
	translucent.on_bi.bi_db_close	= translucent_db_close;
	translucent.on_bi.bi_db_destroy	= translucent_db_destroy;
	translucent.on_bi.bi_op_bind	= translucent_bind;
	translucent.on_bi.bi_op_add	= translucent_add;
	translucent.on_bi.bi_op_modify	= translucent_modify;
	translucent.on_bi.bi_op_modrdn	= translucent_modrdn;
	translucent.on_bi.bi_op_delete	= translucent_delete;
	translucent.on_bi.bi_op_search	= translucent_search;
	translucent.on_bi.bi_op_compare	= translucent_compare;
	translucent.on_bi.bi_connection_destroy = translucent_connection_destroy;
	translucent.on_bi.bi_extended	= translucent_exop;

	translucent.on_bi.bi_cf_ocs = translucentocs;
	rc = config_register_schema ( translucentcfg, translucentocs );
	if ( rc ) return rc;

	return(overlay_register(&translucent));
}

#if SLAPD_OVER_TRANSLUCENT == SLAPD_MOD_DYNAMIC && defined(PIC)
int init_module(int argc, char *argv[]) {
	return translucent_initialize();
}
#endif

#endif /* SLAPD_OVER_TRANSLUCENT */
