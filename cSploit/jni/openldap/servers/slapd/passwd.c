/* passwd.c - password extended operation routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
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

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/unistd.h>

#ifdef SLAPD_CRYPT
#include <ac/crypt.h>
#endif

#include "slap.h"

#include <lber_pvt.h>
#include <lutil.h>
#include <lutil_sha1.h>

const struct berval slap_EXOP_MODIFY_PASSWD = BER_BVC(LDAP_EXOP_MODIFY_PASSWD);

static const char *defhash[] = {
#ifdef LUTIL_SHA1_BYTES
	"{SSHA}",
#else
	"{SMD5}",
#endif
	NULL
};

int passwd_extop(
	Operation *op,
	SlapReply *rs )
{
	struct berval id = {0, NULL}, hash, *rsp = NULL;
	req_pwdexop_s *qpw = &op->oq_pwdexop;
	req_extended_s qext = op->oq_extended;
	Modifications *ml;
	slap_callback cb = { NULL, slap_null_cb, NULL, NULL };
	int i, nhash;
	char **hashes, idNul;
	int rc;
	BackendDB *op_be;
	int freenewpw = 0;
	struct berval dn = BER_BVNULL, ndn = BER_BVNULL;

	assert( ber_bvcmp( &slap_EXOP_MODIFY_PASSWD, &op->ore_reqoid ) == 0 );

	if( op->o_dn.bv_len == 0 ) {
		Statslog( LDAP_DEBUG_STATS, "%s PASSMOD\n",
			op->o_log_prefix, 0, 0, 0, 0 );
		rs->sr_text = "only authenticated users may change passwords";
		return LDAP_STRONG_AUTH_REQUIRED;
	}

	qpw->rs_old.bv_len = 0;
	qpw->rs_old.bv_val = NULL;
	qpw->rs_new.bv_len = 0;
	qpw->rs_new.bv_val = NULL;
	qpw->rs_mods = NULL;
	qpw->rs_modtail = NULL;

	rs->sr_err = slap_passwd_parse( op->ore_reqdata, &id,
		&qpw->rs_old, &qpw->rs_new, &rs->sr_text );

	if ( !BER_BVISNULL( &id )) {
		idNul = id.bv_val[id.bv_len];
		id.bv_val[id.bv_len] = '\0';
	}
	if ( rs->sr_err == LDAP_SUCCESS && !BER_BVISEMPTY( &id ) ) {
		Statslog( LDAP_DEBUG_STATS, "%s PASSMOD id=\"%s\"%s%s\n",
			op->o_log_prefix, id.bv_val,
			qpw->rs_old.bv_val ? " old" : "",
			qpw->rs_new.bv_val ? " new" : "", 0 );
	} else {
		Statslog( LDAP_DEBUG_STATS, "%s PASSMOD%s%s\n",
			op->o_log_prefix,
			qpw->rs_old.bv_val ? " old" : "",
			qpw->rs_new.bv_val ? " new" : "", 0, 0 );
	}

	if ( rs->sr_err != LDAP_SUCCESS ) {
		if ( !BER_BVISNULL( &id ))
			id.bv_val[id.bv_len] = idNul;
		return rs->sr_err;
	}

	if ( !BER_BVISEMPTY( &id ) ) {
		rs->sr_err = dnPrettyNormal( NULL, &id, &dn, &ndn, op->o_tmpmemctx );
		id.bv_val[id.bv_len] = idNul;
		if ( rs->sr_err != LDAP_SUCCESS ) {
			rs->sr_text = "Invalid DN";
			rc = rs->sr_err;
			goto error_return;
		}
		op->o_req_dn = dn;
		op->o_req_ndn = ndn;
		op->o_bd = select_backend( &op->o_req_ndn, 1 );

	} else {
		ber_dupbv_x( &dn, &op->o_dn, op->o_tmpmemctx );
		ber_dupbv_x( &ndn, &op->o_ndn, op->o_tmpmemctx );
		op->o_req_dn = dn;
		op->o_req_ndn = ndn;
		ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );
		op->o_bd = op->o_conn->c_authz_backend;
		ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );
	}

	if( op->o_bd == NULL ) {
		if ( qpw->rs_old.bv_val != NULL ) {
			rs->sr_text = "unwilling to verify old password";
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto error_return;
		}

#ifdef HAVE_CYRUS_SASL
		rc = slap_sasl_setpass( op, rs );
#else
		rs->sr_text = "no authz backend";
		rc = LDAP_OTHER;
#endif
		goto error_return;
	}

	if ( op->o_req_ndn.bv_len == 0 ) {
		rs->sr_text = "no password is associated with the Root DSE";
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto error_return;
	}

	/* If we've got a glued backend, check the real backend */
	op_be = op->o_bd;
	if ( SLAP_GLUE_INSTANCE( op->o_bd )) {
		op->o_bd = select_backend( &op->o_req_ndn, 0 );
	}

	if (backend_check_restrictions( op, rs,
			(struct berval *)&slap_EXOP_MODIFY_PASSWD ) != LDAP_SUCCESS) {
		rc = rs->sr_err;
		goto error_return;
	}

	/* check for referrals */
	if ( backend_check_referrals( op, rs ) != LDAP_SUCCESS ) {
		rc = rs->sr_err;
		goto error_return;
	}

	/* This does not apply to multi-master case */
	if(!( !SLAP_SINGLE_SHADOW( op->o_bd ) || be_isupdate( op ))) {
		/* we SHOULD return a referral in this case */
		BerVarray defref = op->o_bd->be_update_refs
			? op->o_bd->be_update_refs : default_referral; 

		if( defref != NULL ) {
			rs->sr_ref = referral_rewrite( op->o_bd->be_update_refs,
				NULL, NULL, LDAP_SCOPE_DEFAULT );
			if(rs->sr_ref) {
				rs->sr_flags |= REP_REF_MUSTBEFREED;
			} else {
				rs->sr_ref = defref;
			}
			rc = LDAP_REFERRAL;
			goto error_return;

		}

		rs->sr_text = "shadow context; no update referral";
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto error_return;
	}

	/* generate a new password if none was provided */
	if ( qpw->rs_new.bv_len == 0 ) {
		slap_passwd_generate( &qpw->rs_new );
		if ( qpw->rs_new.bv_len ) {
			rsp = slap_passwd_return( &qpw->rs_new );
			freenewpw = 1;
		}
	}
	if ( qpw->rs_new.bv_len == 0 ) {
		rs->sr_text = "password generation failed";
		rc = LDAP_OTHER;
		goto error_return;
	}

	op->o_bd = op_be;

	/* Give the backend a chance to handle this itself */
	if ( op->o_bd->be_extended ) {
		rs->sr_err = op->o_bd->be_extended( op, rs );
		if ( rs->sr_err != LDAP_UNWILLING_TO_PERFORM &&
			rs->sr_err != SLAP_CB_CONTINUE )
		{
			rc = rs->sr_err;
			if ( rsp ) {
				rs->sr_rspdata = rsp;
				rsp = NULL;
			}
			goto error_return;
		}
	}

	/* The backend didn't handle it, so try it here */
	if( op->o_bd && !op->o_bd->be_modify ) {
		rs->sr_text = "operation not supported for current user";
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto error_return;
	}

	if ( qpw->rs_old.bv_val != NULL ) {
		Entry *e = NULL;

		rc = be_entry_get_rw( op, &op->o_req_ndn, NULL,
			slap_schema.si_ad_userPassword, 0, &e );
		if ( rc == LDAP_SUCCESS && e ) {
			Attribute *a = attr_find( e->e_attrs,
				slap_schema.si_ad_userPassword );
			if ( a )
				rc = slap_passwd_check( op, e, a, &qpw->rs_old, &rs->sr_text );
			else
				rc = 1;
			be_entry_release_r( op, e );
			if ( rc == LDAP_SUCCESS )
				goto old_good;
		}
		rs->sr_text = "unwilling to verify old password";
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto error_return;
	}

old_good:
	ml = ch_malloc( sizeof(Modifications) );
	if ( !qpw->rs_modtail ) qpw->rs_modtail = &ml->sml_next;

	if ( default_passwd_hash ) {
		for ( nhash = 0; default_passwd_hash[nhash]; nhash++ );
		hashes = default_passwd_hash;
	} else {
		nhash = 1;
		hashes = (char **)defhash;
	}
	ml->sml_numvals = nhash;
	ml->sml_values = ch_malloc( (nhash+1)*sizeof(struct berval) );
	for ( i=0; hashes[i]; i++ ) {
		slap_passwd_hash_type( &qpw->rs_new, &hash, hashes[i], &rs->sr_text );
		if ( hash.bv_len == 0 ) {
			if ( !rs->sr_text ) {
				rs->sr_text = "password hash failed";
			}
			break;
		}
		ml->sml_values[i] = hash;
	}
	ml->sml_values[i].bv_val = NULL;
	ml->sml_nvalues = NULL;
	ml->sml_desc = slap_schema.si_ad_userPassword;
	ml->sml_type = ml->sml_desc->ad_cname;
	ml->sml_op = LDAP_MOD_REPLACE;
	ml->sml_flags = 0;
	ml->sml_next = qpw->rs_mods;
	qpw->rs_mods = ml;

	if ( hashes[i] ) {
		rs->sr_err = LDAP_OTHER;

	} else {
		slap_callback *sc = op->o_callback;

		op->o_tag = LDAP_REQ_MODIFY;
		op->o_callback = &cb;
		op->orm_modlist = qpw->rs_mods;
		op->orm_no_opattrs = 0;
		
		cb.sc_private = qpw;	/* let Modify know this was pwdMod,
					 * if it cares... */

		rs->sr_err = op->o_bd->be_modify( op, rs );

		/* be_modify() might have shuffled modifications */
		qpw->rs_mods = op->orm_modlist;

		if ( rs->sr_err == LDAP_SUCCESS ) {
			rs->sr_rspdata = rsp;

		} else if ( rsp ) {
			ber_bvfree( rsp );
			rsp = NULL;
		}
		op->o_tag = LDAP_REQ_EXTENDED;
		op->o_callback = sc;
	}

	rc = rs->sr_err;
	op->oq_extended = qext;

error_return:;
	if ( qpw->rs_mods ) {
		slap_mods_free( qpw->rs_mods, 1 );
	}
	if ( freenewpw ) {
		free( qpw->rs_new.bv_val );
	}
	if ( !BER_BVISNULL( &dn ) ) {
		op->o_tmpfree( dn.bv_val, op->o_tmpmemctx );
		BER_BVZERO( &op->o_req_dn );
	}
	if ( !BER_BVISNULL( &ndn ) ) {
		op->o_tmpfree( ndn.bv_val, op->o_tmpmemctx );
		BER_BVZERO( &op->o_req_ndn );
	}

	return rc;
}

/* NOTE: The DN in *id is NOT NUL-terminated here. dnNormalize will
 * reject it in this condition, the caller must NUL-terminate it.
 * FIXME: should dnNormalize still be complaining about that?
 */
int slap_passwd_parse( struct berval *reqdata,
	struct berval *id,
	struct berval *oldpass,
	struct berval *newpass,
	const char **text )
{
	int rc = LDAP_SUCCESS;
	ber_tag_t tag;
	ber_len_t len = -1;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;

	if( reqdata == NULL ) {
		return LDAP_SUCCESS;
	}

	if( reqdata->bv_len == 0 ) {
		*text = "empty request data field";
		return LDAP_PROTOCOL_ERROR;
	}

	/* ber_init2 uses reqdata directly, doesn't allocate new buffers */
	ber_init2( ber, reqdata, 0 );

	tag = ber_skip_tag( ber, &len );

	if( tag != LBER_SEQUENCE ) {
		Debug( LDAP_DEBUG_TRACE,
			"slap_passwd_parse: decoding error\n", 0, 0, 0 );
		rc = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	tag = ber_peek_tag( ber, &len );
	if( tag == LDAP_TAG_EXOP_MODIFY_PASSWD_ID ) {
		if( id == NULL ) {
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: ID not allowed.\n",
				0, 0, 0 );

			*text = "user must change own password";
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto done;
		}

		tag = ber_get_stringbv( ber, id, LBER_BV_NOTERM );

		if( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: ID parse failed.\n",
				0, 0, 0 );

			goto decoding_error;
		}

		tag = ber_peek_tag( ber, &len );
	}

	if( tag == LDAP_TAG_EXOP_MODIFY_PASSWD_OLD ) {
		if( oldpass == NULL ) {
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: OLD not allowed.\n",
				0, 0, 0 );

			*text = "use bind to verify old password";
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto done;
		}

		tag = ber_get_stringbv( ber, oldpass, LBER_BV_NOTERM );

		if( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: OLD parse failed.\n",
				0, 0, 0 );

			goto decoding_error;
		}

		if( oldpass->bv_len == 0 ) {
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: OLD empty.\n",
				0, 0, 0 );

			*text = "old password value is empty";
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto done;
		}

		tag = ber_peek_tag( ber, &len );
	}

	if( tag == LDAP_TAG_EXOP_MODIFY_PASSWD_NEW ) {
		if( newpass == NULL ) {
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: NEW not allowed.\n",
				0, 0, 0 );

			*text = "user specified passwords disallowed";
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto done;
		}

		tag = ber_get_stringbv( ber, newpass, LBER_BV_NOTERM );

		if( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: NEW parse failed.\n",
				0, 0, 0 );

			goto decoding_error;
		}

		if( newpass->bv_len == 0 ) {
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: NEW empty.\n",
				0, 0, 0 );

			*text = "new password value is empty";
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto done;
		}

		tag = ber_peek_tag( ber, &len );
	}

	if( len != 0 ) {
decoding_error:
		Debug( LDAP_DEBUG_TRACE,
			"slap_passwd_parse: decoding error, len=%ld\n",
			(long) len, 0, 0 );

		*text = "data decoding error";
		rc = LDAP_PROTOCOL_ERROR;
	}

done:
	return rc;
}

struct berval * slap_passwd_return(
	struct berval		*cred )
{
	int rc;
	struct berval *bv = NULL;
	BerElementBuffer berbuf;
	/* opaque structure, size unknown but smaller than berbuf */
	BerElement *ber = (BerElement *)&berbuf;

	assert( cred != NULL );

	Debug( LDAP_DEBUG_TRACE, "slap_passwd_return: %ld\n",
		(long) cred->bv_len, 0, 0 );
	
	ber_init_w_nullc( ber, LBER_USE_DER );

	rc = ber_printf( ber, "{tON}",
		LDAP_TAG_EXOP_MODIFY_PASSWD_GEN, cred );

	if( rc >= 0 ) {
		(void) ber_flatten( ber, &bv );
	}

	ber_free_buf( ber );

	return bv;
}

/*
 * if "e" is provided, access to each value of the password is checked first
 */
int
slap_passwd_check(
	Operation	*op,
	Entry		*e,
	Attribute	*a,
	struct berval	*cred,
	const char	**text )
{
	int			result = 1;
	struct berval		*bv;
	AccessControlState	acl_state = ACL_STATE_INIT;
	char		credNul = cred->bv_val[cred->bv_len];

#ifdef SLAPD_SPASSWD
	void		*old_authctx = NULL;

	ldap_pvt_thread_pool_setkey( op->o_threadctx, (void *)slap_sasl_bind,
		op->o_conn->c_sasl_authctx, 0, &old_authctx, NULL );
#endif

	if ( credNul ) cred->bv_val[cred->bv_len] = 0;

	for ( bv = a->a_vals; bv->bv_val != NULL; bv++ ) {
		/* if e is provided, check access */
		if ( e && access_allowed( op, e, a->a_desc, bv,
					ACL_AUTH, &acl_state ) == 0 )
		{
			continue;
		}
		
		if ( !lutil_passwd( bv, cred, NULL, text ) ) {
			result = 0;
			break;
		}
	}

	if ( credNul ) cred->bv_val[cred->bv_len] = credNul;

#ifdef SLAPD_SPASSWD
	ldap_pvt_thread_pool_setkey( op->o_threadctx, (void *)slap_sasl_bind,
		old_authctx, 0, NULL, NULL );
#endif

	return result;
}

void
slap_passwd_generate( struct berval *pass )
{
	Debug( LDAP_DEBUG_TRACE, "slap_passwd_generate\n", 0, 0, 0 );
	BER_BVZERO( pass );

	/*
	 * generate passwords of only 8 characters as some getpass(3)
	 * implementations truncate at 8 characters.
	 */
	lutil_passwd_generate( pass, 8 );
}

void
slap_passwd_hash_type(
	struct berval * cred,
	struct berval * new,
	char *hash,
	const char **text )
{
	new->bv_len = 0;
	new->bv_val = NULL;

	assert( hash != NULL );

	lutil_passwd_hash( cred , hash, new, text );
}
void
slap_passwd_hash(
	struct berval * cred,
	struct berval * new,
	const char **text )
{
	char *hash = NULL;
	if ( default_passwd_hash ) {
		hash = default_passwd_hash[0];
	}
	if ( !hash ) {
		hash = (char *)defhash[0];
	}

	slap_passwd_hash_type( cred, new, hash, text );
}

#ifdef SLAPD_CRYPT
static ldap_pvt_thread_mutex_t passwd_mutex;
static lutil_cryptfunc slapd_crypt;

static int slapd_crypt( const char *key, const char *salt, char **hash )
{
	char *cr;
	int rc;

	ldap_pvt_thread_mutex_lock( &passwd_mutex );

	cr = crypt( key, salt );
	if ( cr == NULL || cr[0] == '\0' ) {
		/* salt must have been invalid */
		rc = LUTIL_PASSWD_ERR;
	} else {
		if ( hash ) {
			*hash = ber_strdup( cr );
			rc = LUTIL_PASSWD_OK;

		} else {
			rc = strcmp( salt, cr ) ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;
		}
	}

	ldap_pvt_thread_mutex_unlock( &passwd_mutex );
	return rc;
}
#endif /* SLAPD_CRYPT */

void slap_passwd_init()
{
#ifdef SLAPD_CRYPT
	ldap_pvt_thread_mutex_init( &passwd_mutex );
	lutil_cryptptr = slapd_crypt;
#endif
}

