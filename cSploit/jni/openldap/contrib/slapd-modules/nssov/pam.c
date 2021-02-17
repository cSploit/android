/* pam.c - pam processing routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>. 
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
 * Portions Copyright 2008 by Howard Chu, Symas Corp.
 * Portions Copyright 2013 by Ted C. Cheng, Symas Corp.
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

#include "nssov.h"
#include "lutil.h"

static int ppolicy_cid;
static AttributeDescription *ad_loginStatus;

struct paminfo {
	struct berval uid;
	struct berval dn;
	struct berval svc;
	struct berval pwd;
	int authz;
	struct berval msg;
	int ispwdmgr;
};

static int pam_bindcb(
	Operation *op, SlapReply *rs)
{
	struct paminfo *pi = op->o_callback->sc_private;
	LDAPControl *ctrl = ldap_control_find(LDAP_CONTROL_PASSWORDPOLICYRESPONSE,
		rs->sr_ctrls, NULL);
	if (ctrl) {
		LDAP *ld;
		ber_int_t expire, grace;
		LDAPPasswordPolicyError error;

		ldap_create(&ld);
		if (ld) {
			int rc = ldap_parse_passwordpolicy_control(ld,ctrl,
				&expire,&grace,&error);
			if (rc == LDAP_SUCCESS) {
				if (expire >= 0) {
					char *unit = "seconds";
					if (expire > 60) {
						expire /= 60;
						unit = "minutes";
					}
					if (expire > 60) {
						expire /= 60;
						unit = "hours";
					}
					if (expire > 24) {
						expire /= 24;
						unit = "days";
					}
#if 0	/* Who warns about expiration so far in advance? */
					if (expire > 7) {
						expire /= 7;
						unit = "weeks";
					}
					if (expire > 4) {
						expire /= 4;
						unit = "months";
					}
					if (expire > 12) {
						expire /= 12;
						unit = "years";
					}
#endif
					pi->msg.bv_len = sprintf(pi->msg.bv_val,
						"\nWARNING: Password expires in %d %s\n", expire, unit);
				} else if (grace > 0) {
					pi->msg.bv_len = sprintf(pi->msg.bv_val,
						"Password expired; %d grace logins remaining",
						grace);
					pi->authz = NSLCD_PAM_NEW_AUTHTOK_REQD;
				} else if (error != PP_noError) {
					ber_str2bv(ldap_passwordpolicy_err2txt(error), 0, 0,
						&pi->msg);
					switch (error) {
					case PP_passwordExpired:
						/* report this during authz */
						rs->sr_err = LDAP_SUCCESS;
						/* fallthru */
					case PP_changeAfterReset:
						pi->authz = NSLCD_PAM_NEW_AUTHTOK_REQD;
					}
				}
			}
			ldap_ld_free(ld,0,NULL,NULL);
		}
	}
	return LDAP_SUCCESS;
}

static int pam_uid2dn(nssov_info *ni, Operation *op,
	struct paminfo *pi)
{
	struct berval sdn;

	BER_BVZERO(&pi->dn);

	if (!isvalidusername(&pi->uid)) {
		Debug(LDAP_DEBUG_ANY,"nssov_pam_uid2dn(%s): invalid user name\n",
			pi->uid.bv_val ? pi->uid.bv_val : "NULL",0,0);
		return NSLCD_PAM_USER_UNKNOWN;
	}

	if (ni->ni_pam_opts & NI_PAM_SASL2DN) {
		int hlen = global_host_bv.bv_len;

		/* cn=<service>+uid=<user>,cn=<host>,cn=pam,cn=auth */
		sdn.bv_len = pi->uid.bv_len + pi->svc.bv_len + hlen +
			STRLENOF( "cn=+uid=,cn=,cn=pam,cn=auth" );
		sdn.bv_val = op->o_tmpalloc( sdn.bv_len + 1, op->o_tmpmemctx );
		sprintf(sdn.bv_val, "cn=%s+uid=%s,cn=%s,cn=pam,cn=auth",
			pi->svc.bv_val, pi->uid.bv_val, global_host_bv.bv_val);
		slap_sasl2dn(op, &sdn, &pi->dn, 0);
		op->o_tmpfree( sdn.bv_val, op->o_tmpmemctx );
	}

	/* If no luck, do a basic uid search */
	if (BER_BVISEMPTY(&pi->dn) && (ni->ni_pam_opts & NI_PAM_UID2DN)) {
		nssov_uid2dn(op, ni, &pi->uid, &pi->dn);
		if (!BER_BVISEMPTY(&pi->dn)) {
			sdn = pi->dn;
			dnNormalize( 0, NULL, NULL, &sdn, &pi->dn, op->o_tmpmemctx );
		}
	}
	if (BER_BVISEMPTY(&pi->dn)) {
		return NSLCD_PAM_USER_UNKNOWN;
	}
	return 0;
}

int pam_do_bind(nssov_info *ni,TFILE *fp,Operation *op,
	struct paminfo *pi)
{
	int rc;
	slap_callback cb = {0};
	SlapReply rs = {REP_RESULT};

	pi->msg.bv_val = pi->pwd.bv_val;
	pi->msg.bv_len = 0;
	pi->authz = NSLCD_PAM_SUCCESS;

	if (!pi->ispwdmgr) {

		BER_BVZERO(&pi->dn);
		rc = pam_uid2dn(ni, op, pi);
		if (rc) goto finish;

		if (BER_BVISEMPTY(&pi->pwd)) {
			rc = NSLCD_PAM_PERM_DENIED;
			goto finish;
		}

		/* Should only need to do this once at open time, but there's always
		 * the possibility that ppolicy will get loaded later.
		 */
		if (!ppolicy_cid) {
			rc = slap_find_control_id(LDAP_CONTROL_PASSWORDPOLICYREQUEST,
				&ppolicy_cid);
		}
		/* of course, 0 is a valid cid, but it won't be ppolicy... */
		if (ppolicy_cid) {
			op->o_ctrlflag[ppolicy_cid] = SLAP_CONTROL_NONCRITICAL;
		}
	}

	cb.sc_response = pam_bindcb;
	cb.sc_private = pi;
	op->o_callback = &cb;
	op->o_dn.bv_val[0] = 0;
	op->o_dn.bv_len = 0;
	op->o_ndn.bv_val[0] = 0;
	op->o_ndn.bv_len = 0;
	op->o_tag = LDAP_REQ_BIND;
	op->o_protocol = LDAP_VERSION3;
	op->orb_method = LDAP_AUTH_SIMPLE;
	op->orb_cred = pi->pwd;
	op->o_req_dn = pi->dn;
	op->o_req_ndn = pi->dn;
	slap_op_time( &op->o_time, &op->o_tincr );
	rc = op->o_bd->be_bind( op, &rs );
	memset(pi->pwd.bv_val,0,pi->pwd.bv_len);
	/* quirk: on successful bind, caller has to send result. we need
	 * to make sure callbacks run.
	 */
	if (rc == LDAP_SUCCESS)
		send_ldap_result(op, &rs);
	switch(rs.sr_err) {
	case LDAP_SUCCESS: rc = NSLCD_PAM_SUCCESS; break;
	case LDAP_INVALID_CREDENTIALS: rc = NSLCD_PAM_AUTH_ERR; break;
	default: rc = NSLCD_PAM_AUTH_ERR; break;
	}
finish:
	Debug(LDAP_DEBUG_ANY,"pam_do_bind (%s): rc (%d)\n",
		pi->dn.bv_val ? pi->dn.bv_val : "NULL", rc, 0);
	return rc;
}

int pam_authc(nssov_info *ni,TFILE *fp,Operation *op)
{
	int32_t tmpint32;
	int rc;
	slap_callback cb = {0};
	char dnc[1024];
	char uidc[32];
	char svcc[256];
	char pwdc[256];
	struct berval sdn, dn;
	struct paminfo pi;


	READ_STRING(fp,uidc);
	pi.uid.bv_val = uidc;
	pi.uid.bv_len = tmpint32;
	READ_STRING(fp,dnc);
	pi.dn.bv_val = dnc;
	pi.dn.bv_len = tmpint32;
	READ_STRING(fp,svcc);
	pi.svc.bv_val = svcc;
	pi.svc.bv_len = tmpint32;
	READ_STRING(fp,pwdc);
	pi.pwd.bv_val = pwdc;
	pi.pwd.bv_len = tmpint32;

	Debug(LDAP_DEBUG_TRACE,"nssov_pam_authc(%s)\n",
			pi.uid.bv_val ? pi.uid.bv_val : "NULL",0,0);

	pi.ispwdmgr = 0;

	/* if service is "passwd" and "nssov-pam-password-prohibit-message */
	/* is set, deny the auth request */
	if (!strcmp(svcc, "passwd") &&
		!BER_BVISEMPTY(&ni->ni_pam_password_prohibit_message)) {
		Debug(LDAP_DEBUG_TRACE,"nssov_pam_authc(): %s (%s)\n",
			"password_prohibit_message for passwd",
			ni->ni_pam_password_prohibit_message.bv_val,0);
		ber_str2bv(ni->ni_pam_password_prohibit_message.bv_val, 0, 0, &pi.msg);
		pi.authz = NSLCD_PAM_PERM_DENIED;
		rc = NSLCD_PAM_PERM_DENIED;
		goto finish;
	}

	/* if username is null, pwdmgr password preliminary check */
	if (BER_BVISEMPTY(&pi.uid)) {
		if (BER_BVISEMPTY(&ni->ni_pam_pwdmgr_dn)) {
			/* pwdmgr dn not configured */
			Debug(LDAP_DEBUG_TRACE,"nssov_pam_authc(prelim check): %s\n",
				"pwdmgr dn not configured", 0, 0);
			ber_str2bv("pwdmgr dn not configured", 0, 0, &pi.msg);
			pi.authz = NSLCD_PAM_PERM_DENIED;
			rc = NSLCD_PAM_PERM_DENIED;
			goto finish;
		} else {
			/* use pwdmgr dn */
			ber_str2bv(ni->ni_pam_pwdmgr_dn.bv_val, 0, 0, &pi.dn);
		}

		/* use pwdmgr pwd if configured */
		if (BER_BVISEMPTY(&pi.pwd)) {
			if (BER_BVISEMPTY(&ni->ni_pam_pwdmgr_pwd)) {
				Debug(LDAP_DEBUG_TRACE,"nssov_pam_authc(prelim check): %s\n",
					"no pwdmgr pwd", 0, 0);
				ber_str2bv("pwdmgr pwd not configured", 0, 0, &pi.msg);
				pi.authz = NSLCD_PAM_PERM_DENIED;
				rc = NSLCD_PAM_PERM_DENIED;
				goto finish;
			}
			/* use configured pwdmgr pwd */
			memset((void *) pwdc, 0, 256);
			strncpy(pi.pwd.bv_val, ni->ni_pam_pwdmgr_pwd.bv_val,
					ni->ni_pam_pwdmgr_pwd.bv_len);
			pi.pwd.bv_len = ni->ni_pam_pwdmgr_pwd.bv_len;
		}
		pi.ispwdmgr = 1;
	}


	rc = pam_do_bind(ni, fp, op, &pi);

finish:
	Debug(LDAP_DEBUG_TRACE,"nssov_pam_authc(%s): rc (%d)\n",
		pi.dn.bv_val ? pi.dn.bv_val : "NULL",rc,0);
	WRITE_INT32(fp,NSLCD_VERSION);
	WRITE_INT32(fp,NSLCD_ACTION_PAM_AUTHC);
	WRITE_INT32(fp,NSLCD_RESULT_BEGIN);
	WRITE_BERVAL(fp,&pi.uid);
	WRITE_BERVAL(fp,&pi.dn);
	WRITE_INT32(fp,rc);
	WRITE_INT32(fp,pi.authz);	/* authz */
	WRITE_BERVAL(fp,&pi.msg);	/* authzmsg */
	return 0;
}

static struct berval grpmsg =
	BER_BVC("Access denied by group check");
static struct berval hostmsg =
	BER_BVC("Access denied for this host");
static struct berval svcmsg =
	BER_BVC("Access denied for this service");
static struct berval uidmsg =
	BER_BVC("Access denied by UID check");

static int pam_compare_cb(Operation *op, SlapReply *rs)
{
	if (rs->sr_err == LDAP_COMPARE_TRUE)
		op->o_callback->sc_private = (void *)1;
	return LDAP_SUCCESS;
}

int pam_authz(nssov_info *ni,TFILE *fp,Operation *op)
{
	struct berval dn, uid, svc, ruser, rhost, tty;
	struct berval authzmsg = BER_BVNULL;
	int32_t tmpint32;
	char dnc[1024];
	char uidc[32];
	char svcc[256];
	char ruserc[32];
	char rhostc[256];
	char ttyc[256];
	int rc;
	Entry *e = NULL;
	Attribute *a;
	slap_callback cb = {0};

	READ_STRING(fp,uidc);
	uid.bv_val = uidc;
	uid.bv_len = tmpint32;
	READ_STRING(fp,dnc);
	dn.bv_val = dnc;
	dn.bv_len = tmpint32;
	READ_STRING(fp,svcc);
	svc.bv_val = svcc;
	svc.bv_len = tmpint32;
	READ_STRING(fp,ruserc);
	ruser.bv_val = ruserc;
	ruser.bv_len = tmpint32;
	READ_STRING(fp,rhostc);
	rhost.bv_val = rhostc;
	rhost.bv_len = tmpint32;
	READ_STRING(fp,ttyc);
	tty.bv_val = ttyc;
	tty.bv_len = tmpint32;

	Debug(LDAP_DEBUG_TRACE,"nssov_pam_authz(%s)\n",
			dn.bv_val ? dn.bv_val : "NULL",0,0);

	/* If we didn't do authc, we don't have a DN yet */
	if (BER_BVISEMPTY(&dn)) {
		struct paminfo pi;
		pi.uid = uid;
		pi.svc = svc;

		rc = pam_uid2dn(ni, op, &pi);
		if (rc) goto finish;
		dn = pi.dn;
	}

	/* See if they have access to the host and service */
	if ((ni->ni_pam_opts & NI_PAM_HOSTSVC) && nssov_pam_svc_ad) {
		AttributeAssertion ava = ATTRIBUTEASSERTION_INIT;
		struct berval hostdn = BER_BVNULL;
		struct berval odn = op->o_ndn;
		SlapReply rs = {REP_RESULT};
		op->o_dn = dn;
		op->o_ndn = dn;
		{
			nssov_mapinfo *mi = &ni->ni_maps[NM_host];
			char fbuf[1024];
			struct berval filter = {sizeof(fbuf),fbuf};
			SlapReply rs2 = {REP_RESULT};

			/* Lookup the host entry */
			nssov_filter_byname(mi,0,&global_host_bv,&filter);
			cb.sc_private = &hostdn;
			cb.sc_response = nssov_name2dn_cb;
			op->o_callback = &cb;
			op->o_req_dn = mi->mi_base;
			op->o_req_ndn = mi->mi_base;
			op->ors_scope = mi->mi_scope;
			op->ors_filterstr = filter;
			op->ors_filter = str2filter_x(op, filter.bv_val);
			op->ors_attrs = slap_anlist_no_attrs;
			op->ors_tlimit = SLAP_NO_LIMIT;
			op->ors_slimit = 2;
			rc = op->o_bd->be_search(op, &rs2);
			filter_free_x(op, op->ors_filter, 1);

			if (BER_BVISEMPTY(&hostdn) &&
				!BER_BVISEMPTY(&ni->ni_pam_defhost)) {
				filter.bv_len = sizeof(fbuf);
				filter.bv_val = fbuf;
				rs_reinit(&rs2, REP_RESULT);
				nssov_filter_byname(mi,0,&ni->ni_pam_defhost,&filter);
				op->ors_filterstr = filter;
				op->ors_filter = str2filter_x(op, filter.bv_val);
				rc = op->o_bd->be_search(op, &rs2);
				filter_free_x(op, op->ors_filter, 1);
			}

			/* no host entry, no default host -> deny */
			if (BER_BVISEMPTY(&hostdn)) {
				rc = NSLCD_PAM_PERM_DENIED;
				authzmsg = hostmsg;
				goto finish;
			}
		}

		cb.sc_response = pam_compare_cb;
		cb.sc_private = NULL;
		op->o_tag = LDAP_REQ_COMPARE;
		op->o_req_dn = hostdn;
		op->o_req_ndn = hostdn;
		ava.aa_desc = nssov_pam_svc_ad;
		ava.aa_value = svc;
		op->orc_ava = &ava;
		rc = op->o_bd->be_compare( op, &rs );
		if ( cb.sc_private == NULL ) {
			authzmsg = svcmsg;
			rc = NSLCD_PAM_PERM_DENIED;
			goto finish;
		}
		op->o_dn = odn;
		op->o_ndn = odn;
	}

	/* See if they're a member of the group */
	if ((ni->ni_pam_opts & NI_PAM_USERGRP) &&
		!BER_BVISEMPTY(&ni->ni_pam_group_dn) &&
		ni->ni_pam_group_ad) {
		AttributeAssertion ava = ATTRIBUTEASSERTION_INIT;
		SlapReply rs = {REP_RESULT};
		op->o_callback = &cb;
		cb.sc_response = slap_null_cb;
		op->o_tag = LDAP_REQ_COMPARE;
		op->o_req_dn = ni->ni_pam_group_dn;
		op->o_req_ndn = ni->ni_pam_group_dn;
		ava.aa_desc = ni->ni_pam_group_ad;
		ava.aa_value = dn;
		op->orc_ava = &ava;
		rc = op->o_bd->be_compare( op, &rs );
		if ( rs.sr_err != LDAP_COMPARE_TRUE ) {
			authzmsg = grpmsg;
			rc = NSLCD_PAM_PERM_DENIED;
			goto finish;
		}
	}

	/* We need to check the user's entry for these bits */
	if ((ni->ni_pam_opts & (NI_PAM_USERHOST|NI_PAM_USERSVC)) ||
		ni->ni_pam_template_ad ||
		ni->ni_pam_min_uid || ni->ni_pam_max_uid ) {
		rc = be_entry_get_rw( op, &dn, NULL, NULL, 0, &e );
		if (rc != LDAP_SUCCESS) {
			rc = NSLCD_PAM_USER_UNKNOWN;
			goto finish;
		}
	}
	if ((ni->ni_pam_opts & NI_PAM_USERHOST) && nssov_pam_host_ad) {
		a = attr_find(e->e_attrs, nssov_pam_host_ad);
		if (!a || attr_valfind( a,
			SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
			SLAP_MR_VALUE_OF_SYNTAX,
			&global_host_bv, NULL, op->o_tmpmemctx )) {
			rc = NSLCD_PAM_PERM_DENIED;
			authzmsg = hostmsg;
			goto finish;
		}
	}
	if ((ni->ni_pam_opts & NI_PAM_USERSVC) && nssov_pam_svc_ad) {
		a = attr_find(e->e_attrs, nssov_pam_svc_ad);
		if (!a || attr_valfind( a,
			SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
			SLAP_MR_VALUE_OF_SYNTAX,
			&svc, NULL, op->o_tmpmemctx )) {
			rc = NSLCD_PAM_PERM_DENIED;
			authzmsg = svcmsg;
			goto finish;
		}
	}

/* from passwd.c */
#define UIDN_KEY	2

	if (ni->ni_pam_min_uid || ni->ni_pam_max_uid) {
		int id;
		char *tmp;
		nssov_mapinfo *mi = &ni->ni_maps[NM_passwd];
		a = attr_find(e->e_attrs, mi->mi_attrs[UIDN_KEY].an_desc);
		if (!a) {
			rc = NSLCD_PAM_PERM_DENIED;
			authzmsg = uidmsg;
			goto finish;
		}
		id = (int)strtol(a->a_vals[0].bv_val,&tmp,0);
		if (a->a_vals[0].bv_val[0] == '\0' || *tmp != '\0') {
			rc = NSLCD_PAM_PERM_DENIED;
			authzmsg = uidmsg;
			goto finish;
		}
		if ((ni->ni_pam_min_uid && id < ni->ni_pam_min_uid) ||
			(ni->ni_pam_max_uid && id > ni->ni_pam_max_uid)) {
			rc = NSLCD_PAM_PERM_DENIED;
			authzmsg = uidmsg;
			goto finish;
		}
	}

	if (ni->ni_pam_template_ad) {
		a = attr_find(e->e_attrs, ni->ni_pam_template_ad);
		if (a)
			uid = a->a_vals[0];
		else if (!BER_BVISEMPTY(&ni->ni_pam_template))
			uid = ni->ni_pam_template;
	}
	rc = NSLCD_PAM_SUCCESS;

finish:
	WRITE_INT32(fp,NSLCD_VERSION);
	WRITE_INT32(fp,NSLCD_ACTION_PAM_AUTHZ);
	WRITE_INT32(fp,NSLCD_RESULT_BEGIN);
	WRITE_BERVAL(fp,&uid);
	WRITE_BERVAL(fp,&dn);
	WRITE_INT32(fp,rc);
	WRITE_BERVAL(fp,&authzmsg);
	if (e) {
		be_entry_release_r(op, e);
	}
	switch (rc) {
	case NSLCD_PAM_SUCCESS:
		Debug(LDAP_DEBUG_TRACE,"nssov_pam_authz(): success\n", 0,0,0);
		break;
	case NSLCD_PAM_PERM_DENIED:
		Debug(LDAP_DEBUG_TRACE,"nssov_pam_authz(): %s\n",
			authzmsg.bv_val ? authzmsg.bv_val : "NULL",0,0);
		break;
	default:
		Debug(LDAP_DEBUG_TRACE,
			"nssov_pam_authz(): permission denied, rc (%d)\n",
			rc, 0, 0);
	}
	return 0;
}

static int pam_sess(nssov_info *ni,TFILE *fp,Operation *op,int action)
{
	struct berval dn, uid, svc, tty, rhost, ruser;
	int32_t tmpint32;
	char dnc[1024];
	char svcc[256];
	char uidc[32];
	char ttyc[32];
	char rhostc[256];
	char ruserc[32];
	slap_callback cb = {0};
	SlapReply rs = {REP_RESULT};
	char timebuf[LDAP_LUTIL_GENTIME_BUFSIZE];
	struct berval timestamp, bv[2], *nbv;
	time_t stamp;
	Modifications mod;
	int rc = 0;
	int sessionID = -1;

	READ_STRING(fp,uidc);
	uid.bv_val = uidc;
	uid.bv_len = tmpint32;
	READ_STRING(fp,dnc);
	dn.bv_val = dnc;
	dn.bv_len = tmpint32;
	READ_STRING(fp,svcc);
	svc.bv_val = svcc;
	svc.bv_len = tmpint32;
	READ_STRING(fp,ttyc);
	tty.bv_val = ttyc;
	tty.bv_len = tmpint32;
	READ_STRING(fp,rhostc);
	rhost.bv_val = rhostc;
	rhost.bv_len = tmpint32;
	READ_STRING(fp,ruserc);
	ruser.bv_val = ruserc;
	ruser.bv_len = tmpint32;
	READ_INT32(fp,stamp);

	Debug(LDAP_DEBUG_TRACE,"nssov_pam_sess_%c(%s)\n",
		action==NSLCD_ACTION_PAM_SESS_O ? 'o' : 'c', dn.bv_val,0);

	if (!dn.bv_len) {
		Debug(LDAP_DEBUG_TRACE,"nssov_pam_sess_%c(): %s\n",
			action==NSLCD_ACTION_PAM_SESS_O ? 'o' : 'c', "null DN",0);
		rc = -1;
		goto done;
	}

	if (!ni->ni_pam_sessions) {
		Debug(LDAP_DEBUG_TRACE,"nssov_pam_sess_%c(): %s\n",
			action==NSLCD_ACTION_PAM_SESS_O ? 'o' : 'c',
			"pam session(s) not configured, ignored",0);
		rc = -1;
		goto done;
	}

	{
		int i, found=0;
		for (i=0; !BER_BVISNULL(&ni->ni_pam_sessions[i]); i++) {
			if (ni->ni_pam_sessions[i].bv_len != svc.bv_len)
				continue;
			if (!strcasecmp(ni->ni_pam_sessions[i].bv_val, svc.bv_val)) {
				found = 1;
				break;
			}
		}
		if (!found) {
			Debug(LDAP_DEBUG_TRACE,
				"nssov_pam_sess_%c(): service(%s) not configured, ignored\n",
				action==NSLCD_ACTION_PAM_SESS_O ? 'o' : 'c',
				svc.bv_val,0);
			rc = -1;
			goto done;
		}
	}

	slap_op_time( &op->o_time, &op->o_tincr );
	timestamp.bv_len = sizeof(timebuf);
	timestamp.bv_val = timebuf;
	if (action == NSLCD_ACTION_PAM_SESS_O )
		stamp = op->o_time;
	slap_timestamp( &stamp, &timestamp );
	bv[0].bv_len = timestamp.bv_len + global_host_bv.bv_len + svc.bv_len +
		tty.bv_len + ruser.bv_len + rhost.bv_len + STRLENOF("    (@)");
	bv[0].bv_val = op->o_tmpalloc( bv[0].bv_len+1, op->o_tmpmemctx );
	sprintf(bv[0].bv_val, "%s %s %s %s (%s@%s)",
		timestamp.bv_val, global_host_bv.bv_val, svc.bv_val, tty.bv_val,
		ruser.bv_val, rhost.bv_val);

	Debug(LDAP_DEBUG_TRACE, "nssov_pam_sess_%c(): loginStatus (%s) \n",
			action==NSLCD_ACTION_PAM_SESS_O ? 'o' : 'c', bv[0].bv_val,0);
	
	mod.sml_numvals = 1;
	mod.sml_values = bv;
	BER_BVZERO(&bv[1]);
	attr_normalize( ad_loginStatus, bv, &nbv, op->o_tmpmemctx );
	mod.sml_nvalues = nbv;
	mod.sml_desc = ad_loginStatus;
	mod.sml_op = action == NSLCD_ACTION_PAM_SESS_O ? LDAP_MOD_ADD :
		LDAP_MOD_DELETE;
	mod.sml_flags = SLAP_MOD_INTERNAL;
	mod.sml_next = NULL;

	cb.sc_response = slap_null_cb;
	op->o_callback = &cb;
	op->o_tag = LDAP_REQ_MODIFY;
	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;
	op->orm_modlist = &mod;
	op->orm_no_opattrs = 1;
	op->o_req_dn = dn;
	op->o_req_ndn = dn;
	if (op->o_bd->be_modify( op, &rs ) != LDAP_SUCCESS) {
		Debug(LDAP_DEBUG_TRACE,
			"nssov_pam_sess_%c(): modify op failed\n",
			action==NSLCD_ACTION_PAM_SESS_O ? 'o' : 'c',
			0,0);
		rc = -1;
	}

	if ( mod.sml_next ) {
		slap_mods_free( mod.sml_next, 1 );
	}
	ber_bvarray_free_x( nbv, op->o_tmpmemctx );

done:;

	if (rc == 0) {
		Debug(LDAP_DEBUG_TRACE,
			"nssov_pam_sess_%c(): success\n",
			action==NSLCD_ACTION_PAM_SESS_O ? 'o' : 'c',
			0,0);
		sessionID = op->o_time;
	}
	WRITE_INT32(fp,NSLCD_VERSION);
	WRITE_INT32(fp,action);
	WRITE_INT32(fp,NSLCD_RESULT_BEGIN);
	WRITE_INT32(fp,sessionID);
	return 0;
}

int pam_sess_o(nssov_info *ni,TFILE *fp,Operation *op)
{
	return pam_sess(ni,fp,op,NSLCD_ACTION_PAM_SESS_O);
}

int pam_sess_c(nssov_info *ni,TFILE *fp,Operation *op)
{
	return pam_sess(ni,fp,op,NSLCD_ACTION_PAM_SESS_C);
}

int pam_pwmod(nssov_info *ni,TFILE *fp,Operation *op)
{
	struct berval npw;
	int32_t tmpint32;
	char dnc[1024];
	char uidc[32];
	char opwc[256];
	char npwc[256];
	char svcc[256];
	struct paminfo pi;
	int rc;

	READ_STRING(fp,uidc);
	pi.uid.bv_val = uidc;
	pi.uid.bv_len = tmpint32;
	READ_STRING(fp,dnc);
	pi.dn.bv_val = dnc;
	pi.dn.bv_len = tmpint32;
	READ_STRING(fp,svcc);
	pi.svc.bv_val = svcc;
	pi.svc.bv_len = tmpint32;
	READ_STRING(fp,opwc);
	pi.pwd.bv_val = opwc;
	pi.pwd.bv_len = tmpint32;
	READ_STRING(fp,npwc);
	npw.bv_val = npwc;
	npw.bv_len = tmpint32;

	Debug(LDAP_DEBUG_TRACE,"nssov_pam_pwmod(%s), %s\n",
		pi.dn.bv_val ? pi.dn.bv_val : "NULL",
		pi.uid.bv_val ? pi.uid.bv_val : "NULL" ,0);

	BER_BVZERO(&pi.msg);
	pi.ispwdmgr = 0;

	/* nssov_pam prohibits password mod */
	if (!BER_BVISEMPTY(&ni->ni_pam_password_prohibit_message)) {
		Debug(LDAP_DEBUG_TRACE,"nssov_pam_pwmod(): %s (%s)\n",
			"password_prohibit_message",
			ni->ni_pam_password_prohibit_message.bv_val,0);
		ber_str2bv(ni->ni_pam_password_prohibit_message.bv_val, 0, 0, &pi.msg);
		rc = NSLCD_PAM_PERM_DENIED;
		goto done;
	}

	if (BER_BVISEMPTY(&pi.dn)) {
		/* should not be here at all, pam_authc() should have returned */
		/* error */
		Debug(LDAP_DEBUG_TRACE,"nssov_pam_pwmod(), %s\n",
			"prelim checking failed", 0, 0);
		ber_str2bv("no pwmod requesting dn", 0, 0, &pi.msg);
		rc = NSLCD_PAM_PERM_DENIED;
		goto done;
	}

	if (BER_BVISEMPTY(&ni->ni_pam_pwdmgr_dn)) {
		Debug(LDAP_DEBUG_TRACE,"nssov_pam_pwmod(), %s\n",
			"pwdmgr not configured", 0, 0);
		ber_str2bv("pwdmgr not configured", 0, 0, &pi.msg);
		rc = NSLCD_PAM_PERM_DENIED;
		goto done;
	} else if (!ber_bvcmp(&pi.dn, &ni->ni_pam_pwdmgr_dn)) {
		/* root user requesting pwmod, convert uid to dn */
		pi.ispwdmgr = 1;
		rc = pam_uid2dn(ni, op, &pi);
		if (rc) {
			ber_str2bv("unable to convert uid to dn", 0, 0, &pi.msg);
			rc = NSLCD_PAM_PERM_DENIED;
			goto done;
		}
	}

	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	struct berval bv;
	SlapReply rs = {REP_RESULT};
	slap_callback cb = {0};

	ber_init_w_nullc(ber, LBER_USE_DER);
	ber_printf(ber, "{");
	if (!BER_BVISEMPTY(&pi.dn))
		ber_printf(ber, "tO", LDAP_TAG_EXOP_MODIFY_PASSWD_ID,
			&pi.dn);
	/* supply old pwd only when end-user changing pwd */
	if (!BER_BVISEMPTY(&pi.pwd) && pi.ispwdmgr == 0)
		ber_printf(ber, "tO", LDAP_TAG_EXOP_MODIFY_PASSWD_OLD,
			&pi.pwd);
	if (!BER_BVISEMPTY(&npw))
		ber_printf(ber, "tO", LDAP_TAG_EXOP_MODIFY_PASSWD_NEW,
			&npw);
	ber_printf(ber, "N}");
	ber_flatten2(ber, &bv, 0);
	op->o_tag = LDAP_REQ_EXTENDED;
	op->ore_reqoid = slap_EXOP_MODIFY_PASSWD;
	op->ore_reqdata = &bv;

	if (pi.ispwdmgr) {
		/* root user changing end-user passwords */
		op->o_dn = ni->ni_pam_pwdmgr_dn;
		op->o_ndn = ni->ni_pam_pwdmgr_dn;
	} else {
		/* end-user self-pwd-mod */
		op->o_dn = pi.dn;
		op->o_ndn = pi.dn;
	}
	op->o_callback = &cb;
	op->o_conn->c_authz_backend = op->o_bd;
	cb.sc_response = slap_null_cb;
	op->o_bd = frontendDB;
	rc = op->o_bd->be_extended(op, &rs);
	if (rs.sr_text)
		ber_str2bv(rs.sr_text, 0, 0, &pi.msg);
	if (rc == LDAP_SUCCESS)
		rc = NSLCD_PAM_SUCCESS;
	else
		rc = NSLCD_PAM_PERM_DENIED;

done:;
	Debug(LDAP_DEBUG_TRACE,"nssov_pam_pwmod(), rc (%d)\n", rc, 0, 0);
	WRITE_INT32(fp,NSLCD_VERSION);
	WRITE_INT32(fp,NSLCD_ACTION_PAM_PWMOD);
	WRITE_INT32(fp,NSLCD_RESULT_BEGIN);
	WRITE_BERVAL(fp,&pi.uid);
	WRITE_BERVAL(fp,&pi.dn);
	WRITE_INT32(fp,rc);
	WRITE_BERVAL(fp,&pi.msg);
	return 0;
}

int nssov_pam_init()
{
	int code = 0;
	const char *text;
	if (!ad_loginStatus)
		code = slap_str2ad("loginStatus", &ad_loginStatus, &text);

	return code;
}
