/* auditlog.c - log modifications for audit/history purposes */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
 * Portions copyright 2004-2005 Symas Corporation.
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

#ifdef SLAPD_OVER_AUDITLOG

#include <stdio.h>

#include <ac/string.h>
#include <ac/ctype.h>

#include "slap.h"
#include "config.h"
#include "ldif.h"

typedef struct auditlog_data {
	ldap_pvt_thread_mutex_t ad_mutex;
	char *ad_logfile;
} auditlog_data;

static ConfigTable auditlogcfg[] = {
	{ "auditlog", "filename", 2, 2, 0,
	  ARG_STRING|ARG_OFFSET,
	  (void *)offsetof(auditlog_data, ad_logfile),
	  "( OLcfgOvAt:15.1 NAME 'olcAuditlogFile' "
	  "DESC 'Filename for auditlogging' "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs auditlogocs[] = {
	{ "( OLcfgOvOc:15.1 "
	  "NAME 'olcAuditlogConfig' "
	  "DESC 'Auditlog configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcAuditlogFile ) )",
	  Cft_Overlay, auditlogcfg },
	{ NULL, 0, NULL }
};

static int fprint_ldif(FILE *f, char *name, char *val, ber_len_t len) {
	char *s;
	if((s = ldif_put(LDIF_PUT_VALUE, name, val, len)) == NULL)
		return(-1);
	fputs(s, f);
	ber_memfree(s);
	return(0);
}

static int auditlog_response(Operation *op, SlapReply *rs) {
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	auditlog_data *ad = on->on_bi.bi_private;
	FILE *f;
	Attribute *a;
	Modifications *m;
	struct berval *b, *who = NULL, peername;
	char *what, *whatm, *suffix;
	time_t stamp;
	int i;

	if ( rs->sr_err != LDAP_SUCCESS ) return SLAP_CB_CONTINUE;

	if ( !ad->ad_logfile ) return SLAP_CB_CONTINUE;

/*
** add or modify: use modifiersName if present
**
*/
	switch(op->o_tag) {
		case LDAP_REQ_MODRDN:	what = "modrdn";	break;
		case LDAP_REQ_DELETE:	what = "delete";	break;
		case LDAP_REQ_ADD:
			what = "add";
			for(a = op->ora_e->e_attrs; a; a = a->a_next)
				if( a->a_desc == slap_schema.si_ad_modifiersName ) {
					who = &a->a_vals[0];
					break;
				}
			break;
		case LDAP_REQ_MODIFY:
			what = "modify";
			for(m = op->orm_modlist; m; m = m->sml_next)
				if( m->sml_desc == slap_schema.si_ad_modifiersName &&
					( m->sml_op == LDAP_MOD_ADD ||
					m->sml_op == LDAP_MOD_REPLACE )) {
					who = &m->sml_values[0];
					break;
				}
			break;
		default:
			return SLAP_CB_CONTINUE;
	}

	suffix = op->o_bd->be_suffix[0].bv_len ? op->o_bd->be_suffix[0].bv_val :
		"global";

/*
** note: this means requestor's dn when modifiersName is null
*/
	if ( !who )
		who = &op->o_dn;

	peername = op->o_conn->c_peer_name;
	ldap_pvt_thread_mutex_lock(&ad->ad_mutex);
	if((f = fopen(ad->ad_logfile, "a")) == NULL) {
		ldap_pvt_thread_mutex_unlock(&ad->ad_mutex);
		return SLAP_CB_CONTINUE;
	}

	stamp = slap_get_time();
	fprintf(f, "# %s %ld %s%s%s %s conn=%ld\n",
		what, (long)stamp, suffix, who ? " " : "", who ? who->bv_val : "",
		peername.bv_val ? peername.bv_val: "", op->o_conn->c_connid);

	if ( !BER_BVISEMPTY( &op->o_conn->c_dn ) &&
		(!who || !dn_match( who, &op->o_conn->c_dn )))
		fprintf(f, "# realdn: %s\n", op->o_conn->c_dn.bv_val );

	fprintf(f, "dn: %s\nchangetype: %s\n",
		op->o_req_dn.bv_val, what);

	switch(op->o_tag) {
	  case LDAP_REQ_ADD:
		for(a = op->ora_e->e_attrs; a; a = a->a_next)
		  if((b = a->a_vals) != NULL)
			for(i = 0; b[i].bv_val; i++)
				fprint_ldif(f, a->a_desc->ad_cname.bv_val, b[i].bv_val, b[i].bv_len);
		break;

	  case LDAP_REQ_MODIFY:
		for(m = op->orm_modlist; m; m = m->sml_next) {
			switch(m->sml_op & LDAP_MOD_OP) {
				case LDAP_MOD_ADD:	 whatm = "add";		break;
				case LDAP_MOD_REPLACE:	 whatm = "replace";	break;
				case LDAP_MOD_DELETE:	 whatm = "delete";	break;
				case LDAP_MOD_INCREMENT: whatm = "increment";	break;
				default:
					fprintf(f, "# MOD_TYPE_UNKNOWN:%02x\n", m->sml_op & LDAP_MOD_OP);
					continue;
			}
			fprintf(f, "%s: %s\n", whatm, m->sml_desc->ad_cname.bv_val);
			if((b = m->sml_values) != NULL)
			  for(i = 0; b[i].bv_val; i++)
				fprint_ldif(f, m->sml_desc->ad_cname.bv_val, b[i].bv_val, b[i].bv_len);
			fprintf(f, "-\n");
		}
		break;

	  case LDAP_REQ_MODRDN:
		fprintf(f, "newrdn: %s\ndeleteoldrdn: %s\n",
			op->orr_newrdn.bv_val, op->orr_deleteoldrdn ? "1" : "0");
		if(op->orr_newSup) fprintf(f, "newsuperior: %s\n", op->orr_newSup->bv_val);
		break;

	  case LDAP_REQ_DELETE:
		/* nothing else needed */
		break;
	}

	fprintf(f, "# end %s %ld\n\n", what, (long)stamp);

	fclose(f);
	ldap_pvt_thread_mutex_unlock(&ad->ad_mutex);
	return SLAP_CB_CONTINUE;
}

static slap_overinst auditlog;

static int
auditlog_db_init(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	auditlog_data *ad = ch_calloc(1, sizeof(auditlog_data));

	on->on_bi.bi_private = ad;
	ldap_pvt_thread_mutex_init( &ad->ad_mutex );
	return 0;
}

static int
auditlog_db_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	auditlog_data *ad = on->on_bi.bi_private;

	free( ad->ad_logfile );
	ad->ad_logfile = NULL;
	return 0;
}

static int
auditlog_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	auditlog_data *ad = on->on_bi.bi_private;

	ldap_pvt_thread_mutex_destroy( &ad->ad_mutex );
	free( ad );
	return 0;
}

int auditlog_initialize() {
	int rc;

	auditlog.on_bi.bi_type = "auditlog";
	auditlog.on_bi.bi_db_init = auditlog_db_init;
	auditlog.on_bi.bi_db_close = auditlog_db_close;
	auditlog.on_bi.bi_db_destroy = auditlog_db_destroy;
	auditlog.on_response = auditlog_response;

	auditlog.on_bi.bi_cf_ocs = auditlogocs;
	rc = config_register_schema( auditlogcfg, auditlogocs );
	if ( rc ) return rc;

	return overlay_register(&auditlog);
}

#if SLAPD_OVER_AUDITLOG == SLAPD_MOD_DYNAMIC && defined(PIC)
int
init_module( int argc, char *argv[] )
{
	return auditlog_initialize();
}
#endif

#endif /* SLAPD_OVER_AUDITLOG */
