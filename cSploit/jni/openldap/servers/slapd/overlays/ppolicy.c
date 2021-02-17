/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
 * Portions Copyright 2004-2005 Howard Chu, Symas Corporation.
 * Portions Copyright 2004 Hewlett-Packard Company.
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
 * This work was developed by Howard Chu for inclusion in
 * OpenLDAP Software, based on prior work by Neil Dunbar (HP).
 * This work was sponsored by the Hewlett-Packard Company.
 */

#include "portable.h"

/* This file implements "Password Policy for LDAP Directories",
 * based on draft behera-ldap-password-policy-09
 */

#ifdef SLAPD_OVER_PPOLICY

#include <ldap.h>
#include "lutil.h"
#include "slap.h"
#ifdef SLAPD_MODULES
#define LIBLTDL_DLL_IMPORT	/* Win32: don't re-export libltdl's symbols */
#include <ltdl.h>
#endif
#include <ac/errno.h>
#include <ac/time.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include "config.h"

#ifndef MODULE_NAME_SZ
#define MODULE_NAME_SZ 256
#endif

/* Per-instance configuration information */
typedef struct pp_info {
	struct berval def_policy;	/* DN of default policy subentry */
	int use_lockout;		/* send AccountLocked result? */
	int hash_passwords;		/* transparently hash cleartext pwds */
	int forward_updates;	/* use frontend for policy state updates */
} pp_info;

/* Our per-connection info - note, it is not per-instance, it is 
 * used by all instances
 */
typedef struct pw_conn {
	struct berval dn;	/* DN of restricted user */
} pw_conn;

static pw_conn *pwcons;
static int ppolicy_cid;
static int ov_count;

typedef struct pass_policy {
	AttributeDescription *ad; /* attribute to which the policy applies */
	int pwdMinAge; /* minimum time (seconds) until passwd can change */
	int pwdMaxAge; /* time in seconds until pwd will expire after change */
	int pwdInHistory; /* number of previous passwords kept */
	int pwdCheckQuality; /* 0 = don't check quality, 1 = check if possible,
						   2 = check mandatory; fail if not possible */
	int pwdMinLength; /* minimum number of chars in password */
	int pwdExpireWarning; /* number of seconds that warning controls are
							sent before a password expires */
	int pwdGraceAuthNLimit; /* number of times you can log in with an
							expired password */
	int pwdLockout; /* 0 = do not lockout passwords, 1 = lock them out */
	int pwdLockoutDuration; /* time in seconds a password is locked out for */
	int pwdMaxFailure; /* number of failed binds allowed before lockout */
	int pwdFailureCountInterval; /* number of seconds before failure
									counts are zeroed */
	int pwdMustChange; /* 0 = users can use admin set password
							1 = users must change password after admin set */
	int pwdAllowUserChange; /* 0 = users cannot change their passwords
								1 = users can change them */
	int pwdSafeModify; /* 0 = old password doesn't need to come
								with password change request
							1 = password change must supply existing pwd */
	char pwdCheckModule[MODULE_NAME_SZ]; /* name of module to dynamically
										    load to check password */
} PassPolicy;

typedef struct pw_hist {
	time_t t;	/* timestamp of history entry */
	struct berval pw;	/* old password hash */
	struct berval bv;	/* text of entire entry */
	struct pw_hist *next;
} pw_hist;

/* Operational attributes */
static AttributeDescription *ad_pwdChangedTime, *ad_pwdAccountLockedTime,
	*ad_pwdFailureTime, *ad_pwdHistory, *ad_pwdGraceUseTime, *ad_pwdReset,
	*ad_pwdPolicySubentry;

static struct schema_info {
	char *def;
	AttributeDescription **ad;
} pwd_OpSchema[] = {
	{	"( 1.3.6.1.4.1.42.2.27.8.1.16 "
		"NAME ( 'pwdChangedTime' ) "
		"DESC 'The time the password was last changed' "
		"EQUALITY generalizedTimeMatch "
		"ORDERING generalizedTimeOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
		"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		&ad_pwdChangedTime },
	{	"( 1.3.6.1.4.1.42.2.27.8.1.17 "
		"NAME ( 'pwdAccountLockedTime' ) "
		"DESC 'The time an user account was locked' "
		"EQUALITY generalizedTimeMatch "
		"ORDERING generalizedTimeOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
		"SINGLE-VALUE "
#if 0
		/* Not until Relax control is released */
		"NO-USER-MODIFICATION "
#endif
		"USAGE directoryOperation )",
		&ad_pwdAccountLockedTime },
	{	"( 1.3.6.1.4.1.42.2.27.8.1.19 "
		"NAME ( 'pwdFailureTime' ) "
		"DESC 'The timestamps of the last consecutive authentication failures' "
		"EQUALITY generalizedTimeMatch "
		"ORDERING generalizedTimeOrderingMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
		"NO-USER-MODIFICATION USAGE directoryOperation )",
		&ad_pwdFailureTime },
	{	"( 1.3.6.1.4.1.42.2.27.8.1.20 "
		"NAME ( 'pwdHistory' ) "
		"DESC 'The history of users passwords' "
		"EQUALITY octetStringMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40 "
		"NO-USER-MODIFICATION USAGE directoryOperation )",
		&ad_pwdHistory },
	{	"( 1.3.6.1.4.1.42.2.27.8.1.21 "
		"NAME ( 'pwdGraceUseTime' ) "
		"DESC 'The timestamps of the grace login once the password has expired' "
		"EQUALITY generalizedTimeMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
		"NO-USER-MODIFICATION USAGE directoryOperation )",
		&ad_pwdGraceUseTime }, 
	{	"( 1.3.6.1.4.1.42.2.27.8.1.22 "
		"NAME ( 'pwdReset' ) "
		"DESC 'The indication that the password has been reset' "
		"EQUALITY booleanMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.7 "
		"SINGLE-VALUE USAGE directoryOperation )",
		&ad_pwdReset },
	{	"( 1.3.6.1.4.1.42.2.27.8.1.23 "
		"NAME ( 'pwdPolicySubentry' ) "
		"DESC 'The pwdPolicy subentry in effect for this object' "
		"EQUALITY distinguishedNameMatch "
		"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
		"SINGLE-VALUE "
#if 0
		/* Not until Relax control is released */
		"NO-USER-MODIFICATION "
#endif
		"USAGE directoryOperation )",
		&ad_pwdPolicySubentry },
	{ NULL, NULL }
};

/* User attributes */
static AttributeDescription *ad_pwdMinAge, *ad_pwdMaxAge, *ad_pwdInHistory,
	*ad_pwdCheckQuality, *ad_pwdMinLength, *ad_pwdMaxFailure, 
	*ad_pwdGraceAuthNLimit, *ad_pwdExpireWarning, *ad_pwdLockoutDuration,
	*ad_pwdFailureCountInterval, *ad_pwdCheckModule, *ad_pwdLockout,
	*ad_pwdMustChange, *ad_pwdAllowUserChange, *ad_pwdSafeModify,
	*ad_pwdAttribute;

#define TAB(name)	{ #name, &ad_##name }

static struct schema_info pwd_UsSchema[] = {
	TAB(pwdAttribute),
	TAB(pwdMinAge),
	TAB(pwdMaxAge),
	TAB(pwdInHistory),
	TAB(pwdCheckQuality),
	TAB(pwdMinLength),
	TAB(pwdMaxFailure),
	TAB(pwdGraceAuthNLimit),
	TAB(pwdExpireWarning),
	TAB(pwdLockout),
	TAB(pwdLockoutDuration),
	TAB(pwdFailureCountInterval),
	TAB(pwdCheckModule),
	TAB(pwdMustChange),
	TAB(pwdAllowUserChange),
	TAB(pwdSafeModify),
	{ NULL, NULL }
};

static ldap_pvt_thread_mutex_t chk_syntax_mutex;

enum {
	PPOLICY_DEFAULT = 1,
	PPOLICY_HASH_CLEARTEXT,
	PPOLICY_USE_LOCKOUT
};

static ConfigDriver ppolicy_cf_default;

static ConfigTable ppolicycfg[] = {
	{ "ppolicy_default", "policyDN", 2, 2, 0,
	  ARG_DN|ARG_QUOTE|ARG_MAGIC|PPOLICY_DEFAULT, ppolicy_cf_default,
	  "( OLcfgOvAt:12.1 NAME 'olcPPolicyDefault' "
	  "DESC 'DN of a pwdPolicy object for uncustomized objects' "
	  "SYNTAX OMsDN SINGLE-VALUE )", NULL, NULL },
	{ "ppolicy_hash_cleartext", "on|off", 1, 2, 0,
	  ARG_ON_OFF|ARG_OFFSET|PPOLICY_HASH_CLEARTEXT,
	  (void *)offsetof(pp_info,hash_passwords),
	  "( OLcfgOvAt:12.2 NAME 'olcPPolicyHashCleartext' "
	  "DESC 'Hash passwords on add or modify' "
	  "SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "ppolicy_forward_updates", "on|off", 1, 2, 0,
	  ARG_ON_OFF|ARG_OFFSET,
	  (void *)offsetof(pp_info,forward_updates),
	  "( OLcfgOvAt:12.4 NAME 'olcPPolicyForwardUpdates' "
	  "DESC 'Allow policy state updates to be forwarded via updateref' "
	  "SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "ppolicy_use_lockout", "on|off", 1, 2, 0,
	  ARG_ON_OFF|ARG_OFFSET|PPOLICY_USE_LOCKOUT,
	  (void *)offsetof(pp_info,use_lockout),
	  "( OLcfgOvAt:12.3 NAME 'olcPPolicyUseLockout' "
	  "DESC 'Warn clients with AccountLocked' "
	  "SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs ppolicyocs[] = {
	{ "( OLcfgOvOc:12.1 "
	  "NAME 'olcPPolicyConfig' "
	  "DESC 'Password Policy configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcPPolicyDefault $ olcPPolicyHashCleartext $ "
	  "olcPPolicyUseLockout $ olcPPolicyForwardUpdates ) )",
	  Cft_Overlay, ppolicycfg },
	{ NULL, 0, NULL }
};

static int
ppolicy_cf_default( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	pp_info *pi = (pp_info *)on->on_bi.bi_private;
	int rc = ARG_BAD_CONF;

	assert ( c->type == PPOLICY_DEFAULT );
	Debug(LDAP_DEBUG_TRACE, "==> ppolicy_cf_default\n", 0, 0, 0);

	switch ( c->op ) {
	case SLAP_CONFIG_EMIT:
		Debug(LDAP_DEBUG_TRACE, "==> ppolicy_cf_default emit\n", 0, 0, 0);
		rc = 0;
		if ( !BER_BVISEMPTY( &pi->def_policy )) {
			rc = value_add_one( &c->rvalue_vals,
					    &pi->def_policy );
			if ( rc ) return rc;
			rc = value_add_one( &c->rvalue_nvals,
					    &pi->def_policy );
		}
		break;
	case LDAP_MOD_DELETE:
		Debug(LDAP_DEBUG_TRACE, "==> ppolicy_cf_default delete\n", 0, 0, 0);
		if ( pi->def_policy.bv_val ) {
			ber_memfree ( pi->def_policy.bv_val );
			pi->def_policy.bv_val = NULL;
		}
		pi->def_policy.bv_len = 0;
		rc = 0;
		break;
	case SLAP_CONFIG_ADD:
		/* fallthrough to LDAP_MOD_ADD */
	case LDAP_MOD_ADD:
		Debug(LDAP_DEBUG_TRACE, "==> ppolicy_cf_default add\n", 0, 0, 0);
		if ( pi->def_policy.bv_val ) {
			ber_memfree ( pi->def_policy.bv_val );
		}
		pi->def_policy = c->value_ndn;
		ber_memfree( c->value_dn.bv_val );
		BER_BVZERO( &c->value_dn );
		BER_BVZERO( &c->value_ndn );
		rc = 0;
		break;
	default:
		abort ();
	}

	return rc;
}

static time_t
parse_time( char *atm )
{
	struct lutil_tm tm;
	struct lutil_timet tt;
	time_t ret = (time_t)-1;

	if ( lutil_parsetime( atm, &tm ) == 0) {
		lutil_tm2time( &tm, &tt );
		ret = tt.tt_sec;
	}
	return ret;
}

static int
account_locked( Operation *op, Entry *e,
		PassPolicy *pp, Modifications **mod ) 
{
	Attribute       *la;

	assert(mod != NULL);

	if ( !pp->pwdLockout )
		return 0;

	if ( (la = attr_find( e->e_attrs, ad_pwdAccountLockedTime )) != NULL ) {
		BerVarray vals = la->a_nvals;

		/*
		 * there is a lockout stamp - we now need to know if it's
		 * a valid one.
		 */
		if (vals[0].bv_val != NULL) {
			time_t then, now;
			Modifications *m;

			if ((then = parse_time( vals[0].bv_val )) == (time_t)0)
				return 1;

			now = slap_get_time();

			/* Still in the future? not yet in effect */
			if (now < then)
				return 0;

			if (!pp->pwdLockoutDuration)
				return 1;

			if (now < then + pp->pwdLockoutDuration)
				return 1;

			m = ch_calloc( sizeof(Modifications), 1 );
			m->sml_op = LDAP_MOD_DELETE;
			m->sml_flags = 0;
			m->sml_type = ad_pwdAccountLockedTime->ad_cname;
			m->sml_desc = ad_pwdAccountLockedTime;
			m->sml_next = *mod;
			*mod = m;
		}
	}

	return 0;
}

/* IMPLICIT TAGS, all context-specific */
#define PPOLICY_WARNING 0xa0L	/* constructed + 0 */
#define PPOLICY_ERROR 0x81L		/* primitive + 1 */
 
#define PPOLICY_EXPIRE 0x80L	/* primitive + 0 */
#define PPOLICY_GRACE  0x81L	/* primitive + 1 */

static const char ppolicy_ctrl_oid[] = LDAP_CONTROL_PASSWORDPOLICYRESPONSE;

static LDAPControl *
create_passcontrol( Operation *op, int exptime, int grace, LDAPPasswordPolicyError err )
{
	BerElementBuffer berbuf, bb2;
	BerElement *ber = (BerElement *) &berbuf, *b2 = (BerElement *) &bb2;
	LDAPControl c = { 0 }, *cp;
	struct berval bv;

	BER_BVZERO( &c.ldctl_value );

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_printf( ber, "{" /*}*/ );

	if ( exptime >= 0 ) {
		ber_init2( b2, NULL, LBER_USE_DER );
		ber_printf( b2, "ti", PPOLICY_EXPIRE, exptime );
		ber_flatten2( b2, &bv, 1 );
		(void)ber_free_buf(b2);
		ber_printf( ber, "tO", PPOLICY_WARNING, &bv );
		ch_free( bv.bv_val );
	} else if ( grace > 0 ) {
		ber_init2( b2, NULL, LBER_USE_DER );
		ber_printf( b2, "ti", PPOLICY_GRACE, grace );
		ber_flatten2( b2, &bv, 1 );
		(void)ber_free_buf(b2);
		ber_printf( ber, "tO", PPOLICY_WARNING, &bv );
		ch_free( bv.bv_val );
	}

	if (err != PP_noError ) {
		ber_printf( ber, "te", PPOLICY_ERROR, err );
	}
	ber_printf( ber, /*{*/ "N}" );

	if (ber_flatten2( ber, &c.ldctl_value, 0 ) == -1) {
		return NULL;
	}
	cp = op->o_tmpalloc( sizeof( LDAPControl ) + c.ldctl_value.bv_len, op->o_tmpmemctx );
	cp->ldctl_oid = (char *)ppolicy_ctrl_oid;
	cp->ldctl_iscritical = 0;
	cp->ldctl_value.bv_val = (char *)&cp[1];
	cp->ldctl_value.bv_len = c.ldctl_value.bv_len;
	AC_MEMCPY( cp->ldctl_value.bv_val, c.ldctl_value.bv_val, c.ldctl_value.bv_len );
	(void)ber_free_buf(ber);
	
	return cp;
}

static LDAPControl **
add_passcontrol( Operation *op, SlapReply *rs, LDAPControl *ctrl )
{
	LDAPControl **ctrls, **oldctrls = rs->sr_ctrls;
	int n;

	n = 0;
	if ( oldctrls ) {
		for ( ; oldctrls[n]; n++ )
			;
	}
	n += 2;

	ctrls = op->o_tmpcalloc( sizeof( LDAPControl * ), n, op->o_tmpmemctx );

	n = 0;
	if ( oldctrls ) {
		for ( ; oldctrls[n]; n++ ) {
			ctrls[n] = oldctrls[n];
		}
	}
	ctrls[n] = ctrl;
	ctrls[n+1] = NULL;

	rs->sr_ctrls = ctrls;

	return oldctrls;
}

static void
ppolicy_get( Operation *op, Entry *e, PassPolicy *pp )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	pp_info *pi = on->on_bi.bi_private;
	Attribute *a;
	BerVarray vals;
	int rc;
	Entry *pe = NULL;
#if 0
	const char *text;
#endif

	memset( pp, 0, sizeof(PassPolicy) );

	pp->ad = slap_schema.si_ad_userPassword;

	/* Users can change their own password by default */
    	pp->pwdAllowUserChange = 1;

	if ((a = attr_find( e->e_attrs, ad_pwdPolicySubentry )) == NULL) {
		/*
		 * entry has no password policy assigned - use default
		 */
		vals = &pi->def_policy;
		if ( !vals->bv_val )
			goto defaultpol;
	} else {
		vals = a->a_nvals;
		if (vals[0].bv_val == NULL) {
			Debug( LDAP_DEBUG_ANY,
				"ppolicy_get: NULL value for policySubEntry\n", 0, 0, 0 );
			goto defaultpol;
		}
	}

	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	rc = be_entry_get_rw( op, vals, NULL, NULL, 0, &pe );
	op->o_bd->bd_info = (BackendInfo *)on;

	if ( rc ) goto defaultpol;

#if 0	/* Only worry about userPassword for now */
	if ((a = attr_find( pe->e_attrs, ad_pwdAttribute )))
		slap_bv2ad( &a->a_vals[0], &pp->ad, &text );
#endif

	if ( ( a = attr_find( pe->e_attrs, ad_pwdMinAge ) )
			&& lutil_atoi( &pp->pwdMinAge, a->a_vals[0].bv_val ) != 0 )
		goto defaultpol;
	if ( ( a = attr_find( pe->e_attrs, ad_pwdMaxAge ) )
			&& lutil_atoi( &pp->pwdMaxAge, a->a_vals[0].bv_val ) != 0 )
		goto defaultpol;
	if ( ( a = attr_find( pe->e_attrs, ad_pwdInHistory ) )
			&& lutil_atoi( &pp->pwdInHistory, a->a_vals[0].bv_val ) != 0 )
		goto defaultpol;
	if ( ( a = attr_find( pe->e_attrs, ad_pwdCheckQuality ) )
			&& lutil_atoi( &pp->pwdCheckQuality, a->a_vals[0].bv_val ) != 0 )
		goto defaultpol;
	if ( ( a = attr_find( pe->e_attrs, ad_pwdMinLength ) )
			&& lutil_atoi( &pp->pwdMinLength, a->a_vals[0].bv_val ) != 0 )
		goto defaultpol;
	if ( ( a = attr_find( pe->e_attrs, ad_pwdMaxFailure ) )
			&& lutil_atoi( &pp->pwdMaxFailure, a->a_vals[0].bv_val ) != 0 )
		goto defaultpol;
	if ( ( a = attr_find( pe->e_attrs, ad_pwdGraceAuthNLimit ) )
			&& lutil_atoi( &pp->pwdGraceAuthNLimit, a->a_vals[0].bv_val ) != 0 )
		goto defaultpol;
	if ( ( a = attr_find( pe->e_attrs, ad_pwdExpireWarning ) )
			&& lutil_atoi( &pp->pwdExpireWarning, a->a_vals[0].bv_val ) != 0 )
		goto defaultpol;
	if ( ( a = attr_find( pe->e_attrs, ad_pwdFailureCountInterval ) )
			&& lutil_atoi( &pp->pwdFailureCountInterval, a->a_vals[0].bv_val ) != 0 )
		goto defaultpol;
	if ( ( a = attr_find( pe->e_attrs, ad_pwdLockoutDuration ) )
			&& lutil_atoi( &pp->pwdLockoutDuration, a->a_vals[0].bv_val ) != 0 )
		goto defaultpol;

	if ( ( a = attr_find( pe->e_attrs, ad_pwdCheckModule ) ) ) {
		strncpy( pp->pwdCheckModule, a->a_vals[0].bv_val,
			sizeof(pp->pwdCheckModule) );
		pp->pwdCheckModule[sizeof(pp->pwdCheckModule)-1] = '\0';
	}

	if ((a = attr_find( pe->e_attrs, ad_pwdLockout )))
    		pp->pwdLockout = bvmatch( &a->a_nvals[0], &slap_true_bv );
	if ((a = attr_find( pe->e_attrs, ad_pwdMustChange )))
    		pp->pwdMustChange = bvmatch( &a->a_nvals[0], &slap_true_bv );
	if ((a = attr_find( pe->e_attrs, ad_pwdAllowUserChange )))
	    	pp->pwdAllowUserChange = bvmatch( &a->a_nvals[0], &slap_true_bv );
	if ((a = attr_find( pe->e_attrs, ad_pwdSafeModify )))
	    	pp->pwdSafeModify = bvmatch( &a->a_nvals[0], &slap_true_bv );
    
	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	be_entry_release_r( op, pe );
	op->o_bd->bd_info = (BackendInfo *)on;

	return;

defaultpol:
	Debug( LDAP_DEBUG_TRACE,
		"ppolicy_get: using default policy\n", 0, 0, 0 );
	return;
}

static int
password_scheme( struct berval *cred, struct berval *sch )
{
	int e;
    
	assert( cred != NULL );

	if (sch) {
		sch->bv_val = NULL;
		sch->bv_len = 0;
	}
    
	if ((cred->bv_len == 0) || (cred->bv_val == NULL) ||
		(cred->bv_val[0] != '{')) return LDAP_OTHER;

	for(e = 1; cred->bv_val[e] && cred->bv_val[e] != '}'; e++);
	if (cred->bv_val[e]) {
		int rc;
		rc = lutil_passwd_scheme( cred->bv_val );
		if (rc) {
			if (sch) {
				sch->bv_val = cred->bv_val;
				sch->bv_len = e;
			}
			return LDAP_SUCCESS;
		}
	}
	return LDAP_OTHER;
}

static int
check_password_quality( struct berval *cred, PassPolicy *pp, LDAPPasswordPolicyError *err, Entry *e, char **txt )
{
	int rc = LDAP_SUCCESS, ok = LDAP_SUCCESS;
	char *ptr;
	struct berval sch;

	assert( cred != NULL );
	assert( pp != NULL );
	assert( txt != NULL );

	ptr = cred->bv_val;

	*txt = NULL;

	if ((cred->bv_len == 0) || (pp->pwdMinLength > cred->bv_len)) {
		rc = LDAP_CONSTRAINT_VIOLATION;
		if ( err ) *err = PP_passwordTooShort;
		return rc;
	}

        /*
         * We need to know if the password is already hashed - if so
         * what scheme is it. The reason being that the "hash" of
         * {cleartext} still allows us to check the password.
         */
	rc = password_scheme( cred, &sch );
	if (rc == LDAP_SUCCESS) {
		if ((sch.bv_val) && (strncasecmp( sch.bv_val, "{cleartext}",
			sch.bv_len ) == 0)) {
			/*
			 * We can check the cleartext "hash"
			 */
			ptr = cred->bv_val + sch.bv_len;
		} else {
			/* everything else, we can't check */
			if (pp->pwdCheckQuality == 2) {
				rc = LDAP_CONSTRAINT_VIOLATION;
				if (err) *err = PP_insufficientPasswordQuality;
				return rc;
			}
			/*
			 * We can't check the syntax of the password, but it's not
			 * mandatory (according to the policy), so we return success.
			 */
		    
			return LDAP_SUCCESS;
		}
	}

	rc = LDAP_SUCCESS;

	if (pp->pwdCheckModule[0]) {
#ifdef SLAPD_MODULES
		lt_dlhandle mod;
		const char *err;
		
		if ((mod = lt_dlopen( pp->pwdCheckModule )) == NULL) {
			err = lt_dlerror();

			Debug(LDAP_DEBUG_ANY,
			"check_password_quality: lt_dlopen failed: (%s) %s.\n",
				pp->pwdCheckModule, err, 0 );
			ok = LDAP_OTHER; /* internal error */
		} else {
			/* FIXME: the error message ought to be passed thru a
			 * struct berval, with preallocated buffer and size
			 * passed in. Module can still allocate a buffer for
			 * it if the provided one is too small.
			 */
			int (*prog)( char *passwd, char **text, Entry *ent );

			if ((prog = lt_dlsym( mod, "check_password" )) == NULL) {
				err = lt_dlerror();
			    
				Debug(LDAP_DEBUG_ANY,
					"check_password_quality: lt_dlsym failed: (%s) %s.\n",
					pp->pwdCheckModule, err, 0 );
				ok = LDAP_OTHER;
			} else {
				ldap_pvt_thread_mutex_lock( &chk_syntax_mutex );
				ok = prog( ptr, txt, e );
				ldap_pvt_thread_mutex_unlock( &chk_syntax_mutex );
				if (ok != LDAP_SUCCESS) {
					Debug(LDAP_DEBUG_ANY,
						"check_password_quality: module error: (%s) %s.[%d]\n",
						pp->pwdCheckModule, *txt ? *txt : "", ok );
				}
			}
			    
			lt_dlclose( mod );
		}
#else
	Debug(LDAP_DEBUG_ANY, "check_password_quality: external modules not "
		"supported. pwdCheckModule ignored.\n", 0, 0, 0);
#endif /* SLAPD_MODULES */
	}
		
		    
	if (ok != LDAP_SUCCESS) {
		rc = LDAP_CONSTRAINT_VIOLATION;
		if (err) *err = PP_insufficientPasswordQuality;
	}
	
	return rc;
}

static int
parse_pwdhistory( struct berval *bv, char **oid, time_t *oldtime, struct berval *oldpw )
{
	char *ptr;
	struct berval nv, npw;
	ber_len_t i, j;
	
	assert (bv && (bv->bv_len > 0) && (bv->bv_val) && oldtime && oldpw );

	if ( oid ) {
		*oid = 0;
	}
	*oldtime = (time_t)-1;
	BER_BVZERO( oldpw );
	
	ber_dupbv( &nv, bv );

	/* first get the time field */
	for ( i = 0; (i < nv.bv_len) && (nv.bv_val[i] != '#'); i++ )
		;
	if ( i == nv.bv_len ) {
		goto exit_failure; /* couldn't locate the '#' separator */
	}
	nv.bv_val[i++] = '\0'; /* terminate the string & move to next field */
	ptr = nv.bv_val;
	*oldtime = parse_time( ptr );
	if (*oldtime == (time_t)-1) {
		goto exit_failure;
	}

	/* get the OID field */
	for (ptr = &(nv.bv_val[i]); (i < nv.bv_len) && (nv.bv_val[i] != '#'); i++ )
		;
	if ( i == nv.bv_len ) {
		goto exit_failure; /* couldn't locate the '#' separator */
	}
	nv.bv_val[i++] = '\0'; /* terminate the string & move to next field */
	if ( oid ) {
		*oid = ber_strdup( ptr );
	}
	
	/* get the length field */
	for ( ptr = &(nv.bv_val[i]); (i < nv.bv_len) && (nv.bv_val[i] != '#'); i++ )
		;
	if ( i == nv.bv_len ) {
		goto exit_failure; /* couldn't locate the '#' separator */
	}
	nv.bv_val[i++] = '\0'; /* terminate the string & move to next field */
	oldpw->bv_len = strtol( ptr, NULL, 10 );
	if (errno == ERANGE) {
		goto exit_failure;
	}

	/* lastly, get the octets of the string */
	for ( j = i, ptr = &(nv.bv_val[i]); i < nv.bv_len; i++ )
		;
	if ( i - j != oldpw->bv_len) {
		goto exit_failure; /* length is wrong */
	}

	npw.bv_val = ptr;
	npw.bv_len = oldpw->bv_len;
	ber_dupbv( oldpw, &npw );
	ber_memfree( nv.bv_val );
	
	return LDAP_SUCCESS;

exit_failure:;
	if ( oid && *oid ) {
		ber_memfree(*oid);
		*oid = NULL;
	}
	if ( oldpw->bv_val ) {
		ber_memfree( oldpw->bv_val);
		BER_BVZERO( oldpw );
	}
	ber_memfree( nv.bv_val );

	return LDAP_OTHER;
}

static void
add_to_pwd_history( pw_hist **l, time_t t,
                    struct berval *oldpw, struct berval *bv )
{
	pw_hist *p, *p1, *p2;
    
	if (!l) return;

	p = ch_malloc( sizeof( pw_hist ));
	p->pw = *oldpw;
	ber_dupbv( &p->bv, bv );
	p->t = t;
	p->next = NULL;
	
	if (*l == NULL) {
		/* degenerate case */
		*l = p;
		return;
	}
	/*
	 * advance p1 and p2 such that p1 is the node before the
	 * new one, and p2 is the node after it
	 */
	for (p1 = NULL, p2 = *l; p2 && p2->t <= t; p1 = p2, p2=p2->next );
	p->next = p2;
	if (p1 == NULL) { *l = p; return; }
	p1->next = p;
}

#ifndef MAX_PWD_HISTORY_SZ
#define MAX_PWD_HISTORY_SZ 1024
#endif /* MAX_PWD_HISTORY_SZ */

static void
make_pwd_history_value( char *timebuf, struct berval *bv, Attribute *pa )
{
	char str[ MAX_PWD_HISTORY_SZ ];
	int nlen;

	snprintf( str, MAX_PWD_HISTORY_SZ,
		  "%s#%s#%lu#", timebuf,
		  pa->a_desc->ad_type->sat_syntax->ssyn_oid,
		  (unsigned long) pa->a_nvals[0].bv_len );
	str[MAX_PWD_HISTORY_SZ-1] = 0;
	nlen = strlen(str);

        /*
         * We have to assume that the string is a string of octets,
         * not readable characters. In reality, yes, it probably is
         * a readable (ie, base64) string, but we can't count on that
         * Hence, while the first 3 fields of the password history
         * are definitely readable (a timestamp, an OID and an integer
         * length), the remaining octets of the actual password
         * are deemed to be binary data.
         */
	AC_MEMCPY( str + nlen, pa->a_nvals[0].bv_val, pa->a_nvals[0].bv_len );
	nlen += pa->a_nvals[0].bv_len;
	bv->bv_val = ch_malloc( nlen + 1 );
	AC_MEMCPY( bv->bv_val, str, nlen );
	bv->bv_val[nlen] = '\0';
	bv->bv_len = nlen;
}

static void
free_pwd_history_list( pw_hist **l )
{
	pw_hist *p;
    
	if (!l) return;
	p = *l;
	while (p) {
		pw_hist *pp = p->next;

		free(p->pw.bv_val);
		free(p->bv.bv_val);
		free(p);
		p = pp;
	}
	*l = NULL;
}

typedef struct ppbind {
	slap_overinst *on;
	int send_ctrl;
	int set_restrict;
	LDAPControl **oldctrls;
	Modifications *mod;
	LDAPPasswordPolicyError pErr;
	PassPolicy pp;
} ppbind;

static void
ctrls_cleanup( Operation *op, SlapReply *rs, LDAPControl **oldctrls )
{
	int n;

	assert( rs->sr_ctrls != NULL );
	assert( rs->sr_ctrls[0] != NULL );

	for ( n = 0; rs->sr_ctrls[n]; n++ ) {
		if ( rs->sr_ctrls[n]->ldctl_oid == ppolicy_ctrl_oid ) {
			op->o_tmpfree( rs->sr_ctrls[n], op->o_tmpmemctx );
			rs->sr_ctrls[n] = (LDAPControl *)(-1);
			break;
		}
	}

	if ( rs->sr_ctrls[n] == NULL ) {
		/* missed? */
	}

	op->o_tmpfree( rs->sr_ctrls, op->o_tmpmemctx );

	rs->sr_ctrls = oldctrls;
}

static int
ppolicy_ctrls_cleanup( Operation *op, SlapReply *rs )
{
	ppbind *ppb = op->o_callback->sc_private;
	if ( ppb->send_ctrl ) {
		ctrls_cleanup( op, rs, ppb->oldctrls );
	}
	return SLAP_CB_CONTINUE;
}

static int
ppolicy_bind_response( Operation *op, SlapReply *rs )
{
	ppbind *ppb = op->o_callback->sc_private;
	slap_overinst *on = ppb->on;
	Modifications *mod = ppb->mod, *m;
	int pwExpired = 0;
	int ngut = -1, warn = -1, age, rc;
	Attribute *a;
	time_t now, pwtime = (time_t)-1;
	char nowstr[ LDAP_LUTIL_GENTIME_BUFSIZE ];
	struct berval timestamp;
	BackendInfo *bi = op->o_bd->bd_info;
	Entry *e;

	/* If we already know it's locked, just get on with it */
	if ( ppb->pErr != PP_noError ) {
		goto locked;
	}

	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &e );
	op->o_bd->bd_info = bi;

	if ( rc != LDAP_SUCCESS ) {
		return SLAP_CB_CONTINUE;
	}

	now = slap_get_time(); /* stored for later consideration */
	timestamp.bv_val = nowstr;
	timestamp.bv_len = sizeof(nowstr);
	slap_timestamp( &now, &timestamp );

	if ( rs->sr_err == LDAP_INVALID_CREDENTIALS ) {
		int i = 0, fc = 0;

		m = ch_calloc( sizeof(Modifications), 1 );
		m->sml_op = LDAP_MOD_ADD;
		m->sml_flags = 0;
		m->sml_type = ad_pwdFailureTime->ad_cname;
		m->sml_desc = ad_pwdFailureTime;
		m->sml_numvals = 1;
		m->sml_values = ch_calloc( sizeof(struct berval), 2 );
		m->sml_nvalues = ch_calloc( sizeof(struct berval), 2 );

		ber_dupbv( &m->sml_values[0], &timestamp );
		ber_dupbv( &m->sml_nvalues[0], &timestamp );
		m->sml_next = mod;
		mod = m;

		/*
		 * Count the pwdFailureTimes - if it's
		 * greater than the policy pwdMaxFailure,
		 * then lock the account.
		 */
		if ((a = attr_find( e->e_attrs, ad_pwdFailureTime )) != NULL) {
			for(i=0; a->a_nvals[i].bv_val; i++) {

				/*
				 * If the interval is 0, then failures
				 * stay on the record until explicitly
				 * reset by successful authentication.
				 */
				if (ppb->pp.pwdFailureCountInterval == 0) {
					fc++;
				} else if (now <=
							parse_time(a->a_nvals[i].bv_val) +
							ppb->pp.pwdFailureCountInterval) {

					fc++;
				}
				/*
				 * We only count those failures
				 * which are not due to expire.
				 */
			}
		}
		
		if ((ppb->pp.pwdMaxFailure > 0) &&
			(fc >= ppb->pp.pwdMaxFailure - 1)) {

			/*
			 * We subtract 1 from the failure max
			 * because the new failure entry hasn't
			 * made it to the entry yet.
			 */
			m = ch_calloc( sizeof(Modifications), 1 );
			m->sml_op = LDAP_MOD_REPLACE;
			m->sml_flags = 0;
			m->sml_type = ad_pwdAccountLockedTime->ad_cname;
			m->sml_desc = ad_pwdAccountLockedTime;
			m->sml_numvals = 1;
			m->sml_values = ch_calloc( sizeof(struct berval), 2 );
			m->sml_nvalues = ch_calloc( sizeof(struct berval), 2 );
			ber_dupbv( &m->sml_values[0], &timestamp );
			ber_dupbv( &m->sml_nvalues[0], &timestamp );
			m->sml_next = mod;
			mod = m;
		}
	} else if ( rs->sr_err == LDAP_SUCCESS ) {
		if ((a = attr_find( e->e_attrs, ad_pwdChangedTime )) != NULL)
			pwtime = parse_time( a->a_nvals[0].bv_val );

		/* delete all pwdFailureTimes */
		if ( attr_find( e->e_attrs, ad_pwdFailureTime )) {
			m = ch_calloc( sizeof(Modifications), 1 );
			m->sml_op = LDAP_MOD_DELETE;
			m->sml_flags = 0;
			m->sml_type = ad_pwdFailureTime->ad_cname;
			m->sml_desc = ad_pwdFailureTime;
			m->sml_next = mod;
			mod = m;
		}

		/*
		 * check to see if the password must be changed
		 */
		if ( ppb->pp.pwdMustChange &&
			(a = attr_find( e->e_attrs, ad_pwdReset )) &&
			bvmatch( &a->a_nvals[0], &slap_true_bv ) )
		{
			/*
			 * need to inject client controls here to give
			 * more information. For the moment, we ensure
			 * that we are disallowed from doing anything
			 * other than change password.
			 */
			if ( ppb->set_restrict ) {
				ber_dupbv( &pwcons[op->o_conn->c_conn_idx].dn,
					&op->o_conn->c_ndn );
			}

			ppb->pErr = PP_changeAfterReset;

		} else {
			/*
			 * the password does not need to be changed, so
			 * we now check whether the password has expired.
			 *
			 * We can skip this bit if passwords don't age in
			 * the policy. Also, if there was no pwdChangedTime
			 * attribute in the entry, the password never expires.
			 */
			if (ppb->pp.pwdMaxAge == 0) goto grace;

			if (pwtime != (time_t)-1) {
				/*
				 * Check: was the last change time of
				 * the password older than the maximum age
				 * allowed. (Ignore case 2 from I-D, it's just silly.)
				 */
				if (now - pwtime > ppb->pp.pwdMaxAge ) pwExpired = 1;
			}
		}

grace:
		if (!pwExpired) goto check_expiring_password;
		
		if ((a = attr_find( e->e_attrs, ad_pwdGraceUseTime )) == NULL)
			ngut = ppb->pp.pwdGraceAuthNLimit;
		else {
			for(ngut=0; a->a_nvals[ngut].bv_val; ngut++);
			ngut = ppb->pp.pwdGraceAuthNLimit - ngut;
		}

		/*
		 * ngut is the number of remaining grace logins
		 */
		Debug( LDAP_DEBUG_ANY,
			"ppolicy_bind: Entry %s has an expired password: %d grace logins\n",
			e->e_name.bv_val, ngut, 0);
		
		if (ngut < 1) {
			ppb->pErr = PP_passwordExpired;
			rs->sr_err = LDAP_INVALID_CREDENTIALS;
			goto done;
		}

		/*
		 * Add a grace user time to the entry
		 */
		m = ch_calloc( sizeof(Modifications), 1 );
		m->sml_op = LDAP_MOD_ADD;
		m->sml_flags = 0;
		m->sml_type = ad_pwdGraceUseTime->ad_cname;
		m->sml_desc = ad_pwdGraceUseTime;
		m->sml_numvals = 1;
		m->sml_values = ch_calloc( sizeof(struct berval), 2 );
		m->sml_nvalues = ch_calloc( sizeof(struct berval), 2 );
		ber_dupbv( &m->sml_values[0], &timestamp );
		ber_dupbv( &m->sml_nvalues[0], &timestamp );
		m->sml_next = mod;
		mod = m;

check_expiring_password:
		/*
		 * Now we need to check to see
		 * if it is about to expire, and if so, should the user
		 * be warned about it in the password policy control.
		 *
		 * If the password has expired, and we're in the grace period, then
		 * we don't need to do this bit. Similarly, if we don't have password
		 * aging, then there's no need to do this bit either.
		 */
		if ((ppb->pp.pwdMaxAge < 1) || (pwExpired) || (ppb->pp.pwdExpireWarning < 1))
			goto done;

		age = (int)(now - pwtime);
		
		/*
		 * We know that there is a password Change Time attribute - if
		 * there wasn't, then the pwdExpired value would be true, unless
		 * there is no password aging - and if there is no password aging,
		 * then this section isn't called anyway - you can't have an
		 * expiring password if there's no limit to expire.
		 */
		if (ppb->pp.pwdMaxAge - age < ppb->pp.pwdExpireWarning ) {
			/*
			 * Set the warning value.
			 */
			warn = ppb->pp.pwdMaxAge - age; /* seconds left until expiry */
			if (warn < 0) warn = 0; /* something weird here - why is pwExpired not set? */
			
			Debug( LDAP_DEBUG_ANY,
				"ppolicy_bind: Setting warning for password expiry for %s = %d seconds\n",
				op->o_req_dn.bv_val, warn, 0 );
		}
	}

done:
	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	be_entry_release_r( op, e );

locked:
	if ( mod ) {
		Operation op2 = *op;
		SlapReply r2 = { REP_RESULT };
		slap_callback cb = { NULL, slap_null_cb, NULL, NULL };
		pp_info *pi = on->on_bi.bi_private;
		LDAPControl c, *ca[2];

		op2.o_tag = LDAP_REQ_MODIFY;
		op2.o_callback = &cb;
		op2.orm_modlist = mod;
		op2.orm_no_opattrs = 0;
		op2.o_dn = op->o_bd->be_rootdn;
		op2.o_ndn = op->o_bd->be_rootndn;

		/* If this server is a shadow and forward_updates is true,
		 * use the frontend to perform this modify. That will trigger
		 * the update referral, which can then be forwarded by the
		 * chain overlay. Obviously the updateref and chain overlay
		 * must be configured appropriately for this to be useful.
		 */
		if ( SLAP_SHADOW( op->o_bd ) && pi->forward_updates ) {
			op2.o_bd = frontendDB;

			/* Must use Relax control since these are no-user-mod */
			op2.o_relax = SLAP_CONTROL_CRITICAL;
			op2.o_ctrls = ca;
			ca[0] = &c;
			ca[1] = NULL;
			BER_BVZERO( &c.ldctl_value );
			c.ldctl_iscritical = 1;
			c.ldctl_oid = LDAP_CONTROL_RELAX;
		} else {
			/* If not forwarding, don't update opattrs and don't replicate */
			if ( SLAP_SINGLE_SHADOW( op->o_bd )) {
				op2.orm_no_opattrs = 1;
				op2.o_dont_replicate = 1;
			}
			op2.o_bd->bd_info = (BackendInfo *)on->on_info;
		}
		rc = op2.o_bd->be_modify( &op2, &r2 );
		slap_mods_free( mod, 1 );
	}

	if ( ppb->send_ctrl ) {
		LDAPControl *ctrl = NULL;
		pp_info *pi = on->on_bi.bi_private;

		/* Do we really want to tell that the account is locked? */
		if ( ppb->pErr == PP_accountLocked && !pi->use_lockout ) {
			ppb->pErr = PP_noError;
		}
		ctrl = create_passcontrol( op, warn, ngut, ppb->pErr );
		ppb->oldctrls = add_passcontrol( op, rs, ctrl );
		op->o_callback->sc_cleanup = ppolicy_ctrls_cleanup;
	}
	op->o_bd->bd_info = bi;
	return SLAP_CB_CONTINUE;
}

static int
ppolicy_bind( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;

	/* Reset lockout status on all Bind requests */
	if ( !BER_BVISEMPTY( &pwcons[op->o_conn->c_conn_idx].dn )) {
		ch_free( pwcons[op->o_conn->c_conn_idx].dn.bv_val );
		BER_BVZERO( &pwcons[op->o_conn->c_conn_idx].dn );
	}

	/* Root bypasses policy */
	if ( !be_isroot_dn( op->o_bd, &op->o_req_ndn )) {
		Entry *e;
		int rc;
		ppbind *ppb;
		slap_callback *cb;

		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &e );

		if ( rc != LDAP_SUCCESS ) {
			return SLAP_CB_CONTINUE;
		}

		cb = op->o_tmpcalloc( sizeof(ppbind)+sizeof(slap_callback),
			1, op->o_tmpmemctx );
		ppb = (ppbind *)(cb+1);
		ppb->on = on;
		ppb->pErr = PP_noError;
		ppb->set_restrict = 1;

		/* Setup a callback so we can munge the result */

		cb->sc_response = ppolicy_bind_response;
		cb->sc_next = op->o_callback->sc_next;
		cb->sc_private = ppb;
		op->o_callback->sc_next = cb;

		/* Did we receive a password policy request control? */
		if ( op->o_ctrlflag[ppolicy_cid] ) {
			ppb->send_ctrl = 1;
		}

		op->o_bd->bd_info = (BackendInfo *)on;
		ppolicy_get( op, e, &ppb->pp );

		rc = account_locked( op, e, &ppb->pp, &ppb->mod );

		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		be_entry_release_r( op, e );

		if ( rc ) {
			ppb->pErr = PP_accountLocked;
			send_ldap_error( op, rs, LDAP_INVALID_CREDENTIALS, NULL );
			return rs->sr_err;
		}

	}

	return SLAP_CB_CONTINUE;
}

/* Reset the restricted info for the next session on this connection */
static int
ppolicy_connection_destroy( BackendDB *bd, Connection *conn )
{
	if ( !BER_BVISEMPTY( &pwcons[conn->c_conn_idx].dn )) {
		ch_free( pwcons[conn->c_conn_idx].dn.bv_val );
		BER_BVZERO( &pwcons[conn->c_conn_idx].dn );
	}
	return SLAP_CB_CONTINUE;
}

/* Check if this connection is restricted */
static int
ppolicy_restrict(
	Operation *op,
	SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	int send_ctrl = 0;

	/* Did we receive a password policy request control? */
	if ( op->o_ctrlflag[ppolicy_cid] ) {
		send_ctrl = 1;
	}

	if ( op->o_conn && !BER_BVISEMPTY( &pwcons[op->o_conn->c_conn_idx].dn )) {
		LDAPControl **oldctrls;
		/* if the current authcDN doesn't match the one we recorded,
		 * then an intervening Bind has succeeded and the restriction
		 * no longer applies. (ITS#4516)
		 */
		if ( !dn_match( &op->o_conn->c_ndn,
				&pwcons[op->o_conn->c_conn_idx].dn )) {
			ch_free( pwcons[op->o_conn->c_conn_idx].dn.bv_val );
			BER_BVZERO( &pwcons[op->o_conn->c_conn_idx].dn );
			return SLAP_CB_CONTINUE;
		}

		Debug( LDAP_DEBUG_TRACE,
			"connection restricted to password changing only\n", 0, 0, 0);
		if ( send_ctrl ) {
			LDAPControl *ctrl = NULL;
			ctrl = create_passcontrol( op, -1, -1, PP_changeAfterReset );
			oldctrls = add_passcontrol( op, rs, ctrl );
		}
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		send_ldap_error( op, rs, LDAP_INSUFFICIENT_ACCESS, 
			"Operations are restricted to bind/unbind/abandon/StartTLS/modify password" );
		if ( send_ctrl ) {
			ctrls_cleanup( op, rs, oldctrls );
		}
		return rs->sr_err;
	}

	return SLAP_CB_CONTINUE;
}

static int
ppolicy_compare_response(
	Operation *op,
	SlapReply *rs )
{
	/* map compare responses to bind responses */
	if ( rs->sr_err == LDAP_COMPARE_TRUE )
		rs->sr_err = LDAP_SUCCESS;
	else if ( rs->sr_err == LDAP_COMPARE_FALSE )
		rs->sr_err = LDAP_INVALID_CREDENTIALS;

	ppolicy_bind_response( op, rs );

	/* map back to compare */
	if ( rs->sr_err == LDAP_SUCCESS )
		rs->sr_err = LDAP_COMPARE_TRUE;
	else if ( rs->sr_err == LDAP_INVALID_CREDENTIALS )
		rs->sr_err = LDAP_COMPARE_FALSE;

	return SLAP_CB_CONTINUE;
}

static int
ppolicy_compare(
	Operation *op,
	SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;

	if ( ppolicy_restrict( op, rs ) != SLAP_CB_CONTINUE )
		return rs->sr_err;

	/* Did we receive a password policy request control?
	 * Are we testing the userPassword?
	 */
	if ( op->o_ctrlflag[ppolicy_cid] && 
		op->orc_ava->aa_desc == slap_schema.si_ad_userPassword ) {
		Entry *e;
		int rc;
		ppbind *ppb;
		slap_callback *cb;

		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &e );

		if ( rc != LDAP_SUCCESS ) {
			return SLAP_CB_CONTINUE;
		}

		cb = op->o_tmpcalloc( sizeof(ppbind)+sizeof(slap_callback),
			1, op->o_tmpmemctx );
		ppb = (ppbind *)(cb+1);
		ppb->on = on;
		ppb->pErr = PP_noError;
		ppb->send_ctrl = 1;
		/* failures here don't lockout the connection */
		ppb->set_restrict = 0;

		/* Setup a callback so we can munge the result */

		cb->sc_response = ppolicy_compare_response;
		cb->sc_next = op->o_callback->sc_next;
		cb->sc_private = ppb;
		op->o_callback->sc_next = cb;

		op->o_bd->bd_info = (BackendInfo *)on;
		ppolicy_get( op, e, &ppb->pp );

		rc = account_locked( op, e, &ppb->pp, &ppb->mod );

		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		be_entry_release_r( op, e );

		if ( rc ) {
			ppb->pErr = PP_accountLocked;
			send_ldap_error( op, rs, LDAP_COMPARE_FALSE, NULL );
			return rs->sr_err;
		}
	}
	return SLAP_CB_CONTINUE;
}

static int
ppolicy_add(
	Operation *op,
	SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	pp_info *pi = on->on_bi.bi_private;
	PassPolicy pp;
	Attribute *pa;
	const char *txt;

	if ( ppolicy_restrict( op, rs ) != SLAP_CB_CONTINUE )
		return rs->sr_err;

	/* If this is a replica, assume the master checked everything */
	if ( be_shadow_update( op ))
		return SLAP_CB_CONTINUE;

	/* Check for password in entry */
	if ((pa = attr_find( op->oq_add.rs_e->e_attrs,
		slap_schema.si_ad_userPassword )))
	{
		assert( pa->a_vals != NULL );
		assert( !BER_BVISNULL( &pa->a_vals[ 0 ] ) );

		if ( !BER_BVISNULL( &pa->a_vals[ 1 ] ) ) {
			send_ldap_error( op, rs, LDAP_CONSTRAINT_VIOLATION, "Password policy only allows one password value" );
			return rs->sr_err;
		}

		/*
		 * new entry contains a password - if we're not the root user
		 * then we need to check that the password fits in with the
		 * security policy for the new entry.
		 */
		ppolicy_get( op, op->ora_e, &pp );
		if (pp.pwdCheckQuality > 0 && !be_isroot( op )) {
			struct berval *bv = &(pa->a_vals[0]);
			int rc, send_ctrl = 0;
			LDAPPasswordPolicyError pErr = PP_noError;
			char *txt;

			/* Did we receive a password policy request control? */
			if ( op->o_ctrlflag[ppolicy_cid] ) {
				send_ctrl = 1;
			}
			rc = check_password_quality( bv, &pp, &pErr, op->ora_e, &txt );
			if (rc != LDAP_SUCCESS) {
				LDAPControl **oldctrls = NULL;
				op->o_bd->bd_info = (BackendInfo *)on->on_info;
				if ( send_ctrl ) {
					LDAPControl *ctrl = NULL;
					ctrl = create_passcontrol( op, -1, -1, pErr );
					oldctrls = add_passcontrol( op, rs, ctrl );
				}
				send_ldap_error( op, rs, rc, txt ? txt : "Password fails quality checking policy" );
				if ( txt ) {
					free( txt );
				}
				if ( send_ctrl ) {
					ctrls_cleanup( op, rs, oldctrls );
				}
				return rs->sr_err;
			}
		}
			/*
			 * A controversial bit. We hash cleartext
			 * passwords provided via add and modify operations
			 * You're not really supposed to do this, since
			 * the X.500 model says "store attributes" as they
			 * get provided. By default, this is what we do
			 *
			 * But if the hash_passwords flag is set, we hash
			 * any cleartext password attribute values via the
			 * default password hashing scheme.
			 */
		if ((pi->hash_passwords) &&
			(password_scheme( &(pa->a_vals[0]), NULL ) != LDAP_SUCCESS)) {
			struct berval hpw;

			slap_passwd_hash( &(pa->a_vals[0]), &hpw, &txt );
			if (hpw.bv_val == NULL) {
				/*
				 * hashing didn't work. Emit an error.
				 */
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = txt;
				send_ldap_error( op, rs, LDAP_OTHER, "Password hashing failed" );
				return rs->sr_err;
			}

			memset( pa->a_vals[0].bv_val, 0, pa->a_vals[0].bv_len);
			ber_memfree( pa->a_vals[0].bv_val );
			pa->a_vals[0].bv_val = hpw.bv_val;
			pa->a_vals[0].bv_len = hpw.bv_len;
		}

		/* If password aging is in effect, set the pwdChangedTime */
		if ( pp.pwdMaxAge || pp.pwdMinAge ) {
			struct berval timestamp;
			char timebuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
			time_t now = slap_get_time();

			timestamp.bv_val = timebuf;
			timestamp.bv_len = sizeof(timebuf);
			slap_timestamp( &now, &timestamp );

			attr_merge_one( op->ora_e, ad_pwdChangedTime, &timestamp, &timestamp );
		}
	}
	return SLAP_CB_CONTINUE;
}

static int
ppolicy_mod_cb( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;
	op->o_callback = sc->sc_next;
	if ( rs->sr_err == LDAP_SUCCESS ) {
		ch_free( pwcons[op->o_conn->c_conn_idx].dn.bv_val );
		BER_BVZERO( &pwcons[op->o_conn->c_conn_idx].dn );
	}
	op->o_tmpfree( sc, op->o_tmpmemctx );
	return SLAP_CB_CONTINUE;
}

static int
ppolicy_modify( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	pp_info			*pi = on->on_bi.bi_private;
	int			i, rc, mod_pw_only, pwmod, pwmop = -1, deladd,
				hsize = 0;
	PassPolicy		pp;
	Modifications		*mods = NULL, *modtail = NULL,
				*ml, *delmod, *addmod;
	Attribute		*pa, *ha, at;
	const char		*txt;
	pw_hist			*tl = NULL, *p;
	int			zapReset, send_ctrl = 0, free_txt = 0;
	Entry			*e;
	struct berval		newpw = BER_BVNULL, oldpw = BER_BVNULL,
				*bv, cr[2];
	LDAPPasswordPolicyError pErr = PP_noError;
	LDAPControl		*ctrl = NULL;
	LDAPControl 		**oldctrls = NULL;
	int			is_pwdexop = 0;

	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &e );
	op->o_bd->bd_info = (BackendInfo *)on;

	if ( rc != LDAP_SUCCESS ) return SLAP_CB_CONTINUE;

	/* If this is a replica, we may need to tweak some of the
	 * master's modifications. Otherwise, just pass it through.
	 */
	if ( be_shadow_update( op )) {
		Modifications **prev;
		int got_del_grace = 0, got_del_lock = 0, got_pw = 0, got_del_fail = 0;
		Attribute *a_grace, *a_lock, *a_fail;

		a_grace = attr_find( e->e_attrs, ad_pwdGraceUseTime );
		a_lock = attr_find( e->e_attrs, ad_pwdAccountLockedTime );
		a_fail = attr_find( e->e_attrs, ad_pwdFailureTime );

		for( prev = &op->orm_modlist, ml = *prev; ml; ml = *prev ) {

			if ( ml->sml_desc == slap_schema.si_ad_userPassword )
				got_pw = 1;

			/* If we're deleting an attr that didn't exist,
			 * drop this delete op
			 */
			if ( ml->sml_op == LDAP_MOD_DELETE ) {
				int drop = 0;

				if ( ml->sml_desc == ad_pwdGraceUseTime ) {
					got_del_grace = 1;
					if ( !a_grace )
						drop = 1;
				} else
				if ( ml->sml_desc == ad_pwdAccountLockedTime ) {
					got_del_lock = 1;
					if ( !a_lock )
						drop = 1;
				} else
				if ( ml->sml_desc == ad_pwdFailureTime ) {
					got_del_fail = 1;
					if ( !a_fail )
						drop = 1;
				}
				if ( drop ) {
					*prev = ml->sml_next;
					ml->sml_next = NULL;
					slap_mods_free( ml, 1 );
					continue;
				}
			}
			prev = &ml->sml_next;
		}

		/* If we're resetting the password, make sure grace, accountlock,
		 * and failure also get removed.
		 */
		if ( got_pw ) {
			if ( a_grace && !got_del_grace ) {
				ml = (Modifications *) ch_malloc( sizeof( Modifications ) );
				ml->sml_op = LDAP_MOD_DELETE;
				ml->sml_flags = SLAP_MOD_INTERNAL;
				ml->sml_type.bv_val = NULL;
				ml->sml_desc = ad_pwdGraceUseTime;
				ml->sml_numvals = 0;
				ml->sml_values = NULL;
				ml->sml_nvalues = NULL;
				ml->sml_next = NULL;
				*prev = ml;
				prev = &ml->sml_next;
			}
			if ( a_lock && !got_del_lock ) {
				ml = (Modifications *) ch_malloc( sizeof( Modifications ) );
				ml->sml_op = LDAP_MOD_DELETE;
				ml->sml_flags = SLAP_MOD_INTERNAL;
				ml->sml_type.bv_val = NULL;
				ml->sml_desc = ad_pwdAccountLockedTime;
				ml->sml_numvals = 0;
				ml->sml_values = NULL;
				ml->sml_nvalues = NULL;
				ml->sml_next = NULL;
				*prev = ml;
			}
			if ( a_fail && !got_del_fail ) {
				ml = (Modifications *) ch_malloc( sizeof( Modifications ) );
				ml->sml_op = LDAP_MOD_DELETE;
				ml->sml_flags = SLAP_MOD_INTERNAL;
				ml->sml_type.bv_val = NULL;
				ml->sml_desc = ad_pwdFailureTime;
				ml->sml_numvals = 0;
				ml->sml_values = NULL;
				ml->sml_nvalues = NULL;
				ml->sml_next = NULL;
				*prev = ml;
			}
		}
		op->o_bd->bd_info = (BackendInfo *)on->on_info;
		be_entry_release_r( op, e );
		return SLAP_CB_CONTINUE;
	}

	/* Did we receive a password policy request control? */
	if ( op->o_ctrlflag[ppolicy_cid] ) {
		send_ctrl = 1;
	}

	/* See if this is a pwdModify exop. If so, we can
	 * access the plaintext passwords from that request.
	 */
	{
		slap_callback *sc;

		for ( sc = op->o_callback; sc; sc=sc->sc_next ) {
			if ( sc->sc_response == slap_null_cb &&
				sc->sc_private ) {
				req_pwdexop_s *qpw = sc->sc_private;
				newpw = qpw->rs_new;
				oldpw = qpw->rs_old;
				is_pwdexop = 1;
			   	break;
			}
		}
	}

	ppolicy_get( op, e, &pp );

	for ( ml = op->orm_modlist,
			pwmod = 0, mod_pw_only = 1,
			deladd = 0, delmod = NULL,
			addmod = NULL,
			zapReset = 1;
		ml != NULL; modtail = ml, ml = ml->sml_next )
	{
		if ( ml->sml_desc == pp.ad ) {
			pwmod = 1;
			pwmop = ml->sml_op;
			if ((deladd == 0) && (ml->sml_op == LDAP_MOD_DELETE) &&
				(ml->sml_values) && !BER_BVISNULL( &ml->sml_values[0] ))
			{
				deladd = 1;
				delmod = ml;
			}

			if ((ml->sml_op == LDAP_MOD_ADD) ||
				(ml->sml_op == LDAP_MOD_REPLACE))
			{
				if ( ml->sml_values && !BER_BVISNULL( &ml->sml_values[0] )) {
					if ( deladd == 1 )
						deladd = 2;

					/* FIXME: there's no easy way to ensure
					 * that add does not cause multiple
					 * userPassword values; one way (that 
					 * would be consistent with the single
					 * password constraint) would be to turn
					 * add into replace); another would be
					 * to disallow add.
					 *
					 * Let's check at least that a single value
					 * is being added
					 */
					if ( addmod || !BER_BVISNULL( &ml->sml_values[ 1 ] ) ) {
						rs->sr_err = LDAP_CONSTRAINT_VIOLATION; 
						rs->sr_text = "Password policy only allows one password value";
						goto return_results;
					}

					addmod = ml;
				} else {
					/* replace can have no values, add cannot */
					assert( ml->sml_op == LDAP_MOD_REPLACE );
				}
			}

		} else if ( !(ml->sml_flags & SLAP_MOD_INTERNAL) && !is_at_operational( ml->sml_desc->ad_type ) ) {
			mod_pw_only = 0;
			/* modifying something other than password */
		}

		/*
		 * If there is a request to explicitly add a pwdReset
		 * attribute, then we suppress the normal behaviour on
		 * password change, which is to remove the pwdReset
		 * attribute.
		 *
		 * This enables an administrator to assign a new password
		 * and place a "must reset" flag on the entry, which will
		 * stay until the user explicitly changes his/her password.
		 */
		if (ml->sml_desc == ad_pwdReset ) {
			if ((ml->sml_op == LDAP_MOD_ADD) ||
				(ml->sml_op == LDAP_MOD_REPLACE))
				zapReset = 0;
		}
	}
	
	if (!BER_BVISEMPTY( &pwcons[op->o_conn->c_conn_idx].dn ) && !mod_pw_only ) {
		if ( dn_match( &op->o_conn->c_ndn,
				&pwcons[op->o_conn->c_conn_idx].dn )) {
			Debug( LDAP_DEBUG_TRACE,
				"connection restricted to password changing only\n", 0, 0, 0 );
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS; 
			rs->sr_text = "Operations are restricted to bind/unbind/abandon/StartTLS/modify password";
			pErr = PP_changeAfterReset;
			goto return_results;
		} else {
			ch_free( pwcons[op->o_conn->c_conn_idx].dn.bv_val );
			BER_BVZERO( &pwcons[op->o_conn->c_conn_idx].dn );
		}
	}

	/*
	 * if we have a "safe password modify policy", then we need to check if we're doing
	 * a delete (with the old password), followed by an add (with the new password).
	 *
	 * If we got just a delete with nothing else, just let it go. We also skip all the checks if
	 * the root user is bound. Root can do anything, including avoid the policies.
	 */

	if (!pwmod) goto do_modify;

	/*
	 * Build the password history list in ascending time order
	 * We need this, even if the user is root, in order to maintain
	 * the pwdHistory operational attributes properly.
	 */
	if (addmod && pp.pwdInHistory > 0 && (ha = attr_find( e->e_attrs, ad_pwdHistory ))) {
		struct berval oldpw;
		time_t oldtime;

		for(i=0; ha->a_nvals[i].bv_val; i++) {
			rc = parse_pwdhistory( &(ha->a_nvals[i]), NULL,
				&oldtime, &oldpw );

			if (rc != LDAP_SUCCESS) continue; /* invalid history entry */

			if (oldpw.bv_val) {
				add_to_pwd_history( &tl, oldtime, &oldpw,
					&(ha->a_nvals[i]) );
				oldpw.bv_val = NULL;
				oldpw.bv_len = 0;
			}
		}
		for(p=tl; p; p=p->next, hsize++); /* count history size */
	}

	if (be_isroot( op )) goto do_modify;

	/* NOTE: according to draft-behera-ldap-password-policy
	 * pwdAllowUserChange == FALSE must only prevent pwd changes
	 * by the user the pwd belongs to (ITS#7021) */
	if (!pp.pwdAllowUserChange && dn_match(&op->o_req_ndn, &op->o_ndn)) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "User alteration of password is not allowed";
		pErr = PP_passwordModNotAllowed;
		goto return_results;
	}

	/* Just deleting? */
	if (!addmod) {
		/* skip everything else */
		pwmod = 0;
		goto do_modify;
	}

	/* This is a pwdModify exop that provided the old pw.
	 * We need to create a Delete mod for this old pw and 
	 * let the matching value get found later
	 */
	if (pp.pwdSafeModify && oldpw.bv_val ) {
		ml = (Modifications *)ch_calloc( sizeof( Modifications ), 1 );
		ml->sml_op = LDAP_MOD_DELETE;
		ml->sml_flags = SLAP_MOD_INTERNAL;
		ml->sml_desc = pp.ad;
		ml->sml_type = pp.ad->ad_cname;
		ml->sml_numvals = 1;
		ml->sml_values = (BerVarray) ch_malloc( 2 * sizeof( struct berval ) );
		ber_dupbv( &ml->sml_values[0], &oldpw );
		BER_BVZERO( &ml->sml_values[1] );
		ml->sml_next = op->orm_modlist;
		op->orm_modlist = ml;
		delmod = ml;
		deladd = 2;
	}

	if (pp.pwdSafeModify && deladd != 2) {
		Debug( LDAP_DEBUG_TRACE,
			"change password must use DELETE followed by ADD/REPLACE\n",
			0, 0, 0 );
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		rs->sr_text = "Must supply old password to be changed as well as new one";
		pErr = PP_mustSupplyOldPassword;
		goto return_results;
	}

	/* Check age, but only if pwdReset is not TRUE */
	pa = attr_find( e->e_attrs, ad_pwdReset );
	if ((!pa || !bvmatch( &pa->a_nvals[0], &slap_true_bv )) &&
		pp.pwdMinAge > 0) {
		time_t pwtime = (time_t)-1, now;
		int age;

		if ((pa = attr_find( e->e_attrs, ad_pwdChangedTime )) != NULL)
			pwtime = parse_time( pa->a_nvals[0].bv_val );
		now = slap_get_time();
		age = (int)(now - pwtime);
		if ((pwtime != (time_t)-1) && (age < pp.pwdMinAge)) {
			rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
			rs->sr_text = "Password is too young to change";
			pErr = PP_passwordTooYoung;
			goto return_results;
		}
	}

	/* pa is used in password history check below, be sure it's set */
	if ((pa = attr_find( e->e_attrs, pp.ad )) != NULL && delmod) {
		/*
		 * we have a password to check
		 */
		bv = oldpw.bv_val ? &oldpw : delmod->sml_values;
		/* FIXME: no access checking? */
		rc = slap_passwd_check( op, NULL, pa, bv, &txt );
		if (rc != LDAP_SUCCESS) {
			Debug( LDAP_DEBUG_TRACE,
				"old password check failed: %s\n", txt, 0, 0 );
			
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "Must supply correct old password to change to new one";
			pErr = PP_mustSupplyOldPassword;
			goto return_results;

		} else {
			int i;
			
			/*
			 * replace the delete value with the (possibly hashed)
			 * value which is currently in the password.
			 */
			for ( i = 0; !BER_BVISNULL( &delmod->sml_values[i] ); i++ ) {
				free( delmod->sml_values[i].bv_val );
				BER_BVZERO( &delmod->sml_values[i] );
			}
			free( delmod->sml_values );
			delmod->sml_values = ch_calloc( sizeof(struct berval), 2 );
			BER_BVZERO( &delmod->sml_values[1] );
			ber_dupbv( &(delmod->sml_values[0]),  &(pa->a_nvals[0]) );
		}
	}

	bv = newpw.bv_val ? &newpw : &addmod->sml_values[0];
	if (pp.pwdCheckQuality > 0) {

		rc = check_password_quality( bv, &pp, &pErr, e, (char **)&txt );
		if (rc != LDAP_SUCCESS) {
			rs->sr_err = rc;
			if ( txt ) {
				rs->sr_text = txt;
				free_txt = 1;
			} else {
				rs->sr_text = "Password fails quality checking policy";
			}
			goto return_results;
		}
	}

	/* If pwdInHistory is zero, passwords may be reused */
	if (pa && pp.pwdInHistory > 0) {
		/*
		 * Last check - the password history.
		 */
		/* FIXME: no access checking? */
		if (slap_passwd_check( op, NULL, pa, bv, &txt ) == LDAP_SUCCESS) {
			/*
			 * This is bad - it means that the user is attempting
			 * to set the password to the same as the old one.
			 */
			rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
			rs->sr_text = "Password is not being changed from existing value";
			pErr = PP_passwordInHistory;
			goto return_results;
		}
	
		/*
		 * Iterate through the password history, and fail on any
		 * password matches.
		 */
		at = *pa;
		at.a_vals = cr;
		cr[1].bv_val = NULL;
		for(p=tl; p; p=p->next) {
			cr[0] = p->pw;
			/* FIXME: no access checking? */
			rc = slap_passwd_check( op, NULL, &at, bv, &txt );
			
			if (rc != LDAP_SUCCESS) continue;
			
			rs->sr_err = LDAP_CONSTRAINT_VIOLATION;
			rs->sr_text = "Password is in history of old passwords";
			pErr = PP_passwordInHistory;
			goto return_results;
		}
	}

do_modify:
	if (pwmod) {
		struct berval timestamp;
		char timebuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
		time_t now = slap_get_time();

		/* If the conn is restricted, set a callback to clear it
		 * if the pwmod succeeds
		 */
		if (!BER_BVISEMPTY( &pwcons[op->o_conn->c_conn_idx].dn )) {
			slap_callback *sc = op->o_tmpcalloc( 1, sizeof( slap_callback ),
				op->o_tmpmemctx );
			sc->sc_next = op->o_callback;
			/* Must use sc_response to insure we reset on success, before
			 * the client sees the response. Must use sc_cleanup to insure
			 * that it gets cleaned up if sc_response is not called.
			 */
			sc->sc_response = ppolicy_mod_cb;
			sc->sc_cleanup = ppolicy_mod_cb;
			op->o_callback = sc;
		}

		/*
		 * keep the necessary pwd.. operational attributes
		 * up to date.
		 */

		timestamp.bv_val = timebuf;
		timestamp.bv_len = sizeof(timebuf);
		slap_timestamp( &now, &timestamp );

		mods = NULL;
		if (pwmop != LDAP_MOD_DELETE) {
			mods = (Modifications *) ch_calloc( sizeof( Modifications ), 1 );
			mods->sml_op = LDAP_MOD_REPLACE;
			mods->sml_numvals = 1;
			mods->sml_values = (BerVarray) ch_malloc( 2 * sizeof( struct berval ) );
			ber_dupbv( &mods->sml_values[0], &timestamp );
			BER_BVZERO( &mods->sml_values[1] );
			assert( !BER_BVISNULL( &mods->sml_values[0] ) );
		} else if (attr_find(e->e_attrs, ad_pwdChangedTime )) {
			mods = (Modifications *) ch_calloc( sizeof( Modifications ), 1 );
			mods->sml_op = LDAP_MOD_DELETE;
		}
		if (mods) {
			mods->sml_desc = ad_pwdChangedTime;
			mods->sml_flags = SLAP_MOD_INTERNAL;
			mods->sml_next = NULL;
			modtail->sml_next = mods;
			modtail = mods;
		}

		if (attr_find(e->e_attrs, ad_pwdGraceUseTime )) {
			mods = (Modifications *) ch_calloc( sizeof( Modifications ), 1 );
			mods->sml_op = LDAP_MOD_DELETE;
			mods->sml_desc = ad_pwdGraceUseTime;
			mods->sml_flags = SLAP_MOD_INTERNAL;
			mods->sml_next = NULL;
			modtail->sml_next = mods;
			modtail = mods;
		}

		if (attr_find(e->e_attrs, ad_pwdAccountLockedTime )) {
			mods = (Modifications *) ch_calloc( sizeof( Modifications ), 1 );
			mods->sml_op = LDAP_MOD_DELETE;
			mods->sml_desc = ad_pwdAccountLockedTime;
			mods->sml_flags = SLAP_MOD_INTERNAL;
			mods->sml_next = NULL;
			modtail->sml_next = mods;
			modtail = mods;
		}

		if (attr_find(e->e_attrs, ad_pwdFailureTime )) {
			mods = (Modifications *) ch_calloc( sizeof( Modifications ), 1 );
			mods->sml_op = LDAP_MOD_DELETE;
			mods->sml_desc = ad_pwdFailureTime;
			mods->sml_flags = SLAP_MOD_INTERNAL;
			mods->sml_next = NULL;
			modtail->sml_next = mods;
			modtail = mods;
		}

		/* Delete the pwdReset attribute, since it's being reset */
		if ((zapReset) && (attr_find(e->e_attrs, ad_pwdReset ))) {
			mods = (Modifications *) ch_calloc( sizeof( Modifications ), 1 );
			mods->sml_op = LDAP_MOD_DELETE;
			mods->sml_desc = ad_pwdReset;
			mods->sml_flags = SLAP_MOD_INTERNAL;
			mods->sml_next = NULL;
			modtail->sml_next = mods;
			modtail = mods;
		}

		if (pp.pwdInHistory > 0) {
			if (hsize >= pp.pwdInHistory) {
				/*
				 * We use the >= operator, since we are going to add
				 * the existing password attribute value into the
				 * history - thus the cardinality of history values is
				 * about to rise by one.
				 *
				 * If this would push it over the limit of history
				 * values (remembering - the password policy could have
				 * changed since the password was last altered), we must
				 * delete at least 1 value from the pwdHistory list.
				 *
				 * In fact, we delete '(#pwdHistory attrs - max pwd
				 * history length) + 1' values, starting with the oldest.
				 * This is easily evaluated, since the linked list is
				 * created in ascending time order.
				 */
				mods = (Modifications *) ch_calloc( sizeof( Modifications ), 1 );
				mods->sml_op = LDAP_MOD_DELETE;
				mods->sml_flags = SLAP_MOD_INTERNAL;
				mods->sml_desc = ad_pwdHistory;
				mods->sml_numvals = hsize - pp.pwdInHistory + 1;
				mods->sml_values = ch_calloc( sizeof( struct berval ),
					hsize - pp.pwdInHistory + 2 );
				BER_BVZERO( &mods->sml_values[ hsize - pp.pwdInHistory + 1 ] );
				for(i=0,p=tl; i < (hsize - pp.pwdInHistory + 1); i++, p=p->next) {
					BER_BVZERO( &mods->sml_values[i] );
					ber_dupbv( &(mods->sml_values[i]), &p->bv );
				}
				mods->sml_next = NULL;
				modtail->sml_next = mods;
				modtail = mods;
			}
			free_pwd_history_list( &tl );

			/*
			 * Now add the existing password into the history list.
			 * This will be executed even if the operation is to delete
			 * the password entirely.
			 *
			 * This isn't in the spec explicitly, but it seems to make
			 * sense that the password history list is the list of all
			 * previous passwords - even if they were deleted. Thus, if
			 * someone tries to add a historical password at some future
			 * point, it will fail.
			 */
			if ((pa = attr_find( e->e_attrs, pp.ad )) != NULL) {
				mods = (Modifications *) ch_malloc( sizeof( Modifications ) );
				mods->sml_op = LDAP_MOD_ADD;
				mods->sml_flags = SLAP_MOD_INTERNAL;
				mods->sml_type.bv_val = NULL;
				mods->sml_desc = ad_pwdHistory;
				mods->sml_nvalues = NULL;
				mods->sml_numvals = 1;
				mods->sml_values = ch_calloc( sizeof( struct berval ), 2 );
				mods->sml_values[ 1 ].bv_val = NULL;
				mods->sml_values[ 1 ].bv_len = 0;
				make_pwd_history_value( timebuf, &mods->sml_values[0], pa );
				mods->sml_next = NULL;
				modtail->sml_next = mods;
				modtail = mods;

			} else {
				Debug( LDAP_DEBUG_TRACE,
				"ppolicy_modify: password attr lookup failed\n", 0, 0, 0 );
			}
		}

		/*
		 * Controversial bit here. If the new password isn't hashed
		 * (ie, is cleartext), we probably should hash it according
		 * to the default hash. The reason for this is that we want
		 * to use the policy if possible, but if we hash the password
		 * before, then we're going to run into trouble when it
		 * comes time to check the password.
		 *
		 * Now, the right thing to do is to use the extended password
		 * modify operation, but not all software can do this,
		 * therefore it makes sense to hash the new password, now
		 * we know it passes the policy requirements.
		 *
		 * Of course, if the password is already hashed, then we
		 * leave it alone.
		 */

		if ((pi->hash_passwords) && (addmod) && !newpw.bv_val && 
			(password_scheme( &(addmod->sml_values[0]), NULL ) != LDAP_SUCCESS))
		{
			struct berval hpw, bv;
			
			slap_passwd_hash( &(addmod->sml_values[0]), &hpw, &txt );
			if (hpw.bv_val == NULL) {
					/*
					 * hashing didn't work. Emit an error.
					 */
				rs->sr_err = LDAP_OTHER;
				rs->sr_text = txt;
				goto return_results;
			}
			bv = addmod->sml_values[0];
				/* clear and discard the clear password */
			memset(bv.bv_val, 0, bv.bv_len);
			ber_memfree(bv.bv_val);
			addmod->sml_values[0] = hpw;
		}
	}
	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	be_entry_release_r( op, e );
	return SLAP_CB_CONTINUE;

return_results:
	free_pwd_history_list( &tl );
	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	be_entry_release_r( op, e );
	if ( send_ctrl ) {
		ctrl = create_passcontrol( op, -1, -1, pErr );
		oldctrls = add_passcontrol( op, rs, ctrl );
	}
	send_ldap_result( op, rs );
	if ( free_txt ) {
		free( (char *)txt );
		rs->sr_text = NULL;
	}
	if ( send_ctrl ) {
		if ( is_pwdexop ) {
			if ( rs->sr_flags & REP_CTRLS_MUSTBEFREED ) {
				op->o_tmpfree( oldctrls, op->o_tmpmemctx );
			}
			oldctrls = NULL;
			rs->sr_flags |= REP_CTRLS_MUSTBEFREED;

		} else {
			ctrls_cleanup( op, rs, oldctrls );
		}
	}
	return rs->sr_err;
}

static int
ppolicy_parseCtrl(
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	if ( !BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "passwordPolicyRequest control value not absent";
		return LDAP_PROTOCOL_ERROR;
	}
	op->o_ctrlflag[ppolicy_cid] = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	return LDAP_SUCCESS;
}

static int
attrPretty(
	Syntax *syntax,
	struct berval *val,
	struct berval *out,
	void *ctx )
{
	AttributeDescription *ad = NULL;
	const char *err;
	int code;

	code = slap_bv2ad( val, &ad, &err );
	if ( !code ) {
		ber_dupbv_x( out, &ad->ad_type->sat_cname, ctx );
	}
	return code;
}

static int
attrNormalize(
	slap_mask_t use,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *out,
	void *ctx )
{
	AttributeDescription *ad = NULL;
	const char *err;
	int code;

	code = slap_bv2ad( val, &ad, &err );
	if ( !code ) {
		ber_str2bv_x( ad->ad_type->sat_oid, 0, 1, out, ctx );
	}
	return code;
}

static int
ppolicy_db_init(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;

	if ( SLAP_ISGLOBALOVERLAY( be ) ) {
		/* do not allow slapo-ppolicy to be global by now (ITS#5858) */
		if ( cr ){
			snprintf( cr->msg, sizeof(cr->msg), 
				"slapo-ppolicy cannot be global" );
			Debug( LDAP_DEBUG_ANY, "%s\n", cr->msg, 0, 0 );
		}
		return 1;
	}

	/* Has User Schema been initialized yet? */
	if ( !pwd_UsSchema[0].ad[0] ) {
		const char *err;
		int i, code;

		for (i=0; pwd_UsSchema[i].def; i++) {
			code = slap_str2ad( pwd_UsSchema[i].def, pwd_UsSchema[i].ad, &err );
			if ( code ) {
				if ( cr ){
					snprintf( cr->msg, sizeof(cr->msg), 
						"User Schema load failed for attribute \"%s\". Error code %d: %s",
						pwd_UsSchema[i].def, code, err );
					Debug( LDAP_DEBUG_ANY, "%s\n", cr->msg, 0, 0 );
				}
				return code;
			}
		}
		{
			Syntax *syn;
			MatchingRule *mr;

			syn = ch_malloc( sizeof( Syntax ));
			*syn = *ad_pwdAttribute->ad_type->sat_syntax;
			syn->ssyn_pretty = attrPretty;
			ad_pwdAttribute->ad_type->sat_syntax = syn;

			mr = ch_malloc( sizeof( MatchingRule ));
			*mr = *ad_pwdAttribute->ad_type->sat_equality;
			mr->smr_normalize = attrNormalize;
			ad_pwdAttribute->ad_type->sat_equality = mr;
		}
	}

	on->on_bi.bi_private = ch_calloc( sizeof(pp_info), 1 );

	if ( dtblsize && !pwcons ) {
		/* accommodate for c_conn_idx == -1 */
		pwcons = ch_calloc( sizeof(pw_conn), dtblsize + 1 );
		pwcons++;
	}

	return 0;
}

static int
ppolicy_db_open(
	BackendDB *be,
	ConfigReply *cr
)
{
	ov_count++;
	return overlay_register_control( be, LDAP_CONTROL_PASSWORDPOLICYREQUEST );
}

static int
ppolicy_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	pp_info *pi = on->on_bi.bi_private;

#ifdef SLAP_CONFIG_DELETE
	overlay_unregister_control( be, LDAP_CONTROL_PASSWORDPOLICYREQUEST );
#endif /* SLAP_CONFIG_DELETE */

	/* Perhaps backover should provide bi_destroy hooks... */
	ov_count--;
	if ( ov_count <=0 && pwcons ) {
		pwcons--;
		free( pwcons );
		pwcons = NULL;
	}
	free( pi->def_policy.bv_val );
	free( pi );

	return 0;
}

static char *extops[] = {
	LDAP_EXOP_MODIFY_PASSWD,
	NULL
};

static slap_overinst ppolicy;

int ppolicy_initialize()
{
	int i, code;

	for (i=0; pwd_OpSchema[i].def; i++) {
		code = register_at( pwd_OpSchema[i].def, pwd_OpSchema[i].ad, 0 );
		if ( code ) {
			Debug( LDAP_DEBUG_ANY,
				"ppolicy_initialize: register_at failed\n", 0, 0, 0 );
			return code;
		}
		/* Allow Manager to set these as needed */
		if ( is_at_no_user_mod( (*pwd_OpSchema[i].ad)->ad_type )) {
			(*pwd_OpSchema[i].ad)->ad_type->sat_flags |=
				SLAP_AT_MANAGEABLE;
		}
	}

	code = register_supported_control( LDAP_CONTROL_PASSWORDPOLICYREQUEST,
		SLAP_CTRL_ADD|SLAP_CTRL_BIND|SLAP_CTRL_MODIFY|SLAP_CTRL_HIDE, extops,
		ppolicy_parseCtrl, &ppolicy_cid );
	if ( code != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "Failed to register control %d\n", code, 0, 0 );
		return code;
	}

	ldap_pvt_thread_mutex_init( &chk_syntax_mutex );

	ppolicy.on_bi.bi_type = "ppolicy";
	ppolicy.on_bi.bi_db_init = ppolicy_db_init;
	ppolicy.on_bi.bi_db_open = ppolicy_db_open;
	ppolicy.on_bi.bi_db_close = ppolicy_close;

	ppolicy.on_bi.bi_op_add = ppolicy_add;
	ppolicy.on_bi.bi_op_bind = ppolicy_bind;
	ppolicy.on_bi.bi_op_compare = ppolicy_compare;
	ppolicy.on_bi.bi_op_delete = ppolicy_restrict;
	ppolicy.on_bi.bi_op_modify = ppolicy_modify;
	ppolicy.on_bi.bi_op_search = ppolicy_restrict;
	ppolicy.on_bi.bi_connection_destroy = ppolicy_connection_destroy;

	ppolicy.on_bi.bi_cf_ocs = ppolicyocs;
	code = config_register_schema( ppolicycfg, ppolicyocs );
	if ( code ) return code;

	return overlay_register( &ppolicy );
}

#if SLAPD_OVER_PPOLICY == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return ppolicy_initialize();
}
#endif

#endif	/* defined(SLAPD_OVER_PPOLICY) */
