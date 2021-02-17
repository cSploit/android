/* smbk5pwd.c - Overlay for managing Samba and Heimdal passwords */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
 * Portions Copyright 2004-2005 by Howard Chu, Symas Corp.
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
 * Support for table-driven configuration added by Pierangelo Masarati.
 * Support for sambaPwdMustChange and sambaPwdCanChange added by Marco D'Ettorre.
 * Support for shadowLastChange added by SATOH Fumiyasu @ OSS Technology, Inc.
 */

#include <portable.h>

#ifndef SLAPD_OVER_SMBK5PWD
#define SLAPD_OVER_SMBK5PWD SLAPD_MOD_DYNAMIC
#endif

#ifdef SLAPD_OVER_SMBK5PWD

#include <slap.h>
#include <ac/errno.h>
#include <ac/string.h>

#include "config.h"

#ifdef DO_KRB5
#include <lber.h>
#include <lber_pvt.h>
#include <lutil.h>

/* make ASN1_MALLOC_ENCODE use our allocator */
#define malloc	ch_malloc

#include <krb5.h>
#include <kadm5/admin.h>
#include <hdb.h>

#ifndef HDB_INTERFACE_VERSION
#define	HDB_MASTER_KEY_SET	master_key_set
#else
#define	HDB_MASTER_KEY_SET	hdb_master_key_set
#endif

static krb5_context context;
static void *kadm_context;
static kadm5_config_params conf;
static HDB *db;

static AttributeDescription *ad_krb5Key;
static AttributeDescription *ad_krb5KeyVersionNumber;
static AttributeDescription *ad_krb5PrincipalName;
static AttributeDescription *ad_krb5ValidEnd;
static ObjectClass *oc_krb5KDCEntry;
#endif

#ifdef DO_SAMBA
#ifdef HAVE_GNUTLS
#include <gcrypt.h>
typedef unsigned char DES_cblock[8];
#elif HAVE_OPENSSL
#include <openssl/des.h>
#include <openssl/md4.h>
#else
#error Unsupported crypto backend.
#endif
#include "ldap_utf8.h"

static AttributeDescription *ad_sambaLMPassword;
static AttributeDescription *ad_sambaNTPassword;
static AttributeDescription *ad_sambaPwdLastSet;
static AttributeDescription *ad_sambaPwdMustChange;
static AttributeDescription *ad_sambaPwdCanChange;
static ObjectClass *oc_sambaSamAccount;
#endif

#ifdef DO_SHADOW
static AttributeDescription *ad_shadowLastChange;
static ObjectClass *oc_shadowAccount;
#endif

/* Per-instance configuration information */
typedef struct smbk5pwd_t {
	unsigned	mode;
#define	SMBK5PWD_F_KRB5		(0x1U)
#define	SMBK5PWD_F_SAMBA	(0x2U)
#define	SMBK5PWD_F_SHADOW	(0x4U)

#define SMBK5PWD_DO_KRB5(pi)	((pi)->mode & SMBK5PWD_F_KRB5)
#define SMBK5PWD_DO_SAMBA(pi)	((pi)->mode & SMBK5PWD_F_SAMBA)
#define SMBK5PWD_DO_SHADOW(pi)	((pi)->mode & SMBK5PWD_F_SHADOW)

#ifdef DO_KRB5
	/* nothing yet */
#endif

#ifdef DO_SAMBA
	/* How many seconds before forcing a password change? */
	time_t	smb_must_change;
	/* How many seconds after allowing a password change? */
	time_t  smb_can_change;
#endif

#ifdef DO_SHADOW
	/* nothing yet */
#endif
} smbk5pwd_t;

static const unsigned SMBK5PWD_F_ALL	=
	0
#ifdef DO_KRB5
	| SMBK5PWD_F_KRB5
#endif
#ifdef DO_SAMBA
	| SMBK5PWD_F_SAMBA
#endif
#ifdef DO_SHADOW
	| SMBK5PWD_F_SHADOW
#endif
;

static int smbk5pwd_modules_init( smbk5pwd_t *pi );

#ifdef DO_SAMBA
static const char hex[] = "0123456789abcdef";

/* From liblutil/passwd.c... */
static void lmPasswd_to_key(
	const char *lmPasswd,
	DES_cblock *key)
{
	const unsigned char *lpw = (const unsigned char *)lmPasswd;
	unsigned char *k = (unsigned char *)key;

	/* make room for parity bits */
	k[0] = lpw[0];
	k[1] = ((lpw[0]&0x01)<<7) | (lpw[1]>>1);
	k[2] = ((lpw[1]&0x03)<<6) | (lpw[2]>>2);
	k[3] = ((lpw[2]&0x07)<<5) | (lpw[3]>>3);
	k[4] = ((lpw[3]&0x0F)<<4) | (lpw[4]>>4);
	k[5] = ((lpw[4]&0x1F)<<3) | (lpw[5]>>5);
	k[6] = ((lpw[5]&0x3F)<<2) | (lpw[6]>>6);
	k[7] = ((lpw[6]&0x7F)<<1);

#ifdef HAVE_OPENSSL
	des_set_odd_parity( key );
#endif
}

#define MAX_PWLEN 256
#define	HASHLEN	16

static void hexify(
	const char in[HASHLEN],
	struct berval *out
)
{
	int i;
	char *a;
	unsigned char *b;

	out->bv_val = ch_malloc(HASHLEN*2 + 1);
	out->bv_len = HASHLEN*2;

	a = out->bv_val;
	b = (unsigned char *)in;
	for (i=0; i<HASHLEN; i++) {
		*a++ = hex[*b >> 4];
		*a++ = hex[*b++ & 0x0f];
	}
	*a++ = '\0';
}

static void lmhash(
	struct berval *passwd,
	struct berval *hash
)
{
	char UcasePassword[15];
	DES_cblock key;
	DES_cblock StdText = "KGS!@#$%";
	DES_cblock hbuf[2];
#ifdef HAVE_OPENSSL
	DES_key_schedule schedule;
#elif defined(HAVE_GNUTLS)
	gcry_cipher_hd_t h = NULL;
	gcry_error_t err;

	err = gcry_cipher_open( &h, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_CBC, 0 );
	if ( err ) return;
#endif

	strncpy( UcasePassword, passwd->bv_val, 14 );
	UcasePassword[14] = '\0';
	ldap_pvt_str2upper( UcasePassword );

	lmPasswd_to_key( UcasePassword, &key );
#ifdef HAVE_GNUTLS
	err = gcry_cipher_setkey( h, &key, sizeof(key) );
	if ( err == 0 ) {
		err = gcry_cipher_encrypt( h, &hbuf[0], sizeof(key), &StdText, sizeof(key) );
		if ( err == 0 ) {
			gcry_cipher_reset( h );
			lmPasswd_to_key( &UcasePassword[7], &key );
			err = gcry_cipher_setkey( h, &key, sizeof(key) );
			if ( err == 0 ) {
				err = gcry_cipher_encrypt( h, &hbuf[1], sizeof(key), &StdText, sizeof(key) );
			}
		}
		gcry_cipher_close( h );
	}
#elif defined(HAVE_OPENSSL)
	des_set_key_unchecked( &key, schedule );
	des_ecb_encrypt( &StdText, &hbuf[0], schedule , DES_ENCRYPT );

	lmPasswd_to_key( &UcasePassword[7], &key );
	des_set_key_unchecked( &key, schedule );
	des_ecb_encrypt( &StdText, &hbuf[1], schedule , DES_ENCRYPT );
#endif

	hexify( (char *)hbuf, hash );
}

static void nthash(
	struct berval *passwd,
	struct berval *hash
)
{
	/* Windows currently only allows 14 character passwords, but
	 * may support up to 256 in the future. We assume this means
	 * 256 UCS2 characters, not 256 bytes...
	 */
	char hbuf[HASHLEN];
#ifdef HAVE_OPENSSL
	MD4_CTX ctx;
#endif

	if (passwd->bv_len > MAX_PWLEN*2)
		passwd->bv_len = MAX_PWLEN*2;

#ifdef HAVE_OPENSSL
	MD4_Init( &ctx );
	MD4_Update( &ctx, passwd->bv_val, passwd->bv_len );
	MD4_Final( (unsigned char *)hbuf, &ctx );
#elif defined(HAVE_GNUTLS)
	gcry_md_hash_buffer(GCRY_MD_MD4, hbuf, passwd->bv_val, passwd->bv_len );
#endif

	hexify( hbuf, hash );
}
#endif /* DO_SAMBA */

#ifdef DO_KRB5

static int smbk5pwd_op_cleanup(
	Operation *op,
	SlapReply *rs )
{
	slap_callback *cb;

	/* clear out the current key */
	ldap_pvt_thread_pool_setkey( op->o_threadctx, smbk5pwd_op_cleanup,
		NULL, 0, NULL, NULL );

	/* free the callback */
	cb = op->o_callback;
	op->o_callback = cb->sc_next;
	op->o_tmpfree( cb, op->o_tmpmemctx );
	return 0;
}

static int smbk5pwd_op_bind(
	Operation *op,
	SlapReply *rs )
{
	/* If this is a simple Bind, stash the Op pointer so our chk
	 * function can find it. Set a cleanup callback to clear it
	 * out when the Bind completes.
	 */
	if ( op->oq_bind.rb_method == LDAP_AUTH_SIMPLE ) {
		slap_callback *cb;
		ldap_pvt_thread_pool_setkey( op->o_threadctx,
			smbk5pwd_op_cleanup, op, 0, NULL, NULL );
		cb = op->o_tmpcalloc( 1, sizeof(slap_callback), op->o_tmpmemctx );
		cb->sc_cleanup = smbk5pwd_op_cleanup;
		cb->sc_next = op->o_callback;
		op->o_callback = cb;
	}
	return SLAP_CB_CONTINUE;
}

static LUTIL_PASSWD_CHK_FUNC k5key_chk;
static LUTIL_PASSWD_HASH_FUNC k5key_hash;
static const struct berval k5key_scheme = BER_BVC("{K5KEY}");

/* This password scheme stores no data in the userPassword attribute
 * other than the scheme name. It assumes the invoking entry is a
 * krb5KDCentry and compares the passed-in credentials against the
 * krb5Key attribute. The krb5Key may be multi-valued, but they are
 * simply multiple keytypes generated from the same input string, so
 * only the first value needs to be compared here.
 *
 * Since the lutil_passwd API doesn't pass the Entry object in, we
 * have to fetch it ourselves in order to get access to the other
 * attributes. We accomplish this with the help of the overlay's Bind
 * function, which stores the current Operation pointer in thread-specific
 * storage so we can retrieve it here. The Operation provides all
 * the necessary context for us to get Entry from the database.
 */
static int k5key_chk(
	const struct berval *sc,
	const struct berval *passwd,
	const struct berval *cred,
	const char **text )
{
	void *ctx, *op_tmp;
	Operation *op;
	int rc;
	Entry *e;
	Attribute *a;
	krb5_error_code ret;
	krb5_keyblock key;
	krb5_salt salt;
	hdb_entry ent;

	/* Find our thread context, find our Operation */
	ctx = ldap_pvt_thread_pool_context();

	if ( ldap_pvt_thread_pool_getkey( ctx, smbk5pwd_op_cleanup, &op_tmp, NULL )
		 || !op_tmp )
		return LUTIL_PASSWD_ERR;
	op = op_tmp;

	rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &e );
	if ( rc != LDAP_SUCCESS ) return LUTIL_PASSWD_ERR;

	rc = LUTIL_PASSWD_ERR;
	do {
		size_t l;
		Key ekey = {0};

		a = attr_find( e->e_attrs, ad_krb5PrincipalName );
		if (!a ) break;

		memset( &ent, 0, sizeof(ent) );
		ret = krb5_parse_name(context, a->a_vals[0].bv_val, &ent.principal);
		if ( ret ) break;

		a = attr_find( e->e_attrs, ad_krb5ValidEnd );
		if (a) {
			struct lutil_tm tm;
			struct lutil_timet tt;
			if ( lutil_parsetime( a->a_vals[0].bv_val, &tm ) == 0 &&
				lutil_tm2time( &tm, &tt ) == 0 && tt.tt_usec < op->o_time ) {
				/* Account is expired */
				rc = LUTIL_PASSWD_ERR;
				break;
			}
		}

		krb5_get_pw_salt( context, ent.principal, &salt );
		krb5_free_principal( context, ent.principal );

		a = attr_find( e->e_attrs, ad_krb5Key );
		if ( !a ) break;

		ent.keys.len = 1;
		ent.keys.val = &ekey;
		decode_Key((unsigned char *) a->a_vals[0].bv_val,
			(size_t) a->a_vals[0].bv_len, &ent.keys.val[0], &l);
		if ( db->HDB_MASTER_KEY_SET )
			hdb_unseal_keys( context, db, &ent );

		krb5_string_to_key_salt( context, ekey.key.keytype, cred->bv_val,
			salt, &key );

		krb5_free_salt( context, salt );

		if ( memcmp( ekey.key.keyvalue.data, key.keyvalue.data,
			key.keyvalue.length ) == 0 ) rc = LUTIL_PASSWD_OK;

		krb5_free_keyblock_contents( context, &key );
		krb5_free_keyblock_contents( context, &ekey.key );

	} while(0);
	be_entry_release_r( op, e );
	return rc;
}

static int k5key_hash(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text )
{
	ber_dupbv( hash, (struct berval *)&k5key_scheme );
	return LUTIL_PASSWD_OK;
}
#endif /* DO_KRB5 */

static int smbk5pwd_exop_passwd(
	Operation *op,
	SlapReply *rs )
{
	int rc;
	req_pwdexop_s *qpw = &op->oq_pwdexop;
	Entry *e;
	Modifications *ml;
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	smbk5pwd_t *pi = on->on_bi.bi_private;
	char term;

	/* Not the operation we expected, pass it on... */
	if ( ber_bvcmp( &slap_EXOP_MODIFY_PASSWD, &op->ore_reqoid ) ) {
		return SLAP_CB_CONTINUE;
	}

	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &e );
	if ( rc != LDAP_SUCCESS ) return rc;

	term = qpw->rs_new.bv_val[qpw->rs_new.bv_len];
	qpw->rs_new.bv_val[qpw->rs_new.bv_len] = '\0';

#ifdef DO_KRB5
	/* Kerberos stuff */
	do {
		krb5_error_code ret;
		hdb_entry ent;
		struct berval *keys;
		size_t nkeys;
		int kvno, i;
		Attribute *a;

		if ( !SMBK5PWD_DO_KRB5( pi ) ) break;

		if ( !is_entry_objectclass(e, oc_krb5KDCEntry, 0 ) ) break;

		a = attr_find( e->e_attrs, ad_krb5PrincipalName );
		if ( !a ) break;

		memset( &ent, 0, sizeof(ent) );
		ret = krb5_parse_name(context, a->a_vals[0].bv_val, &ent.principal);
		if ( ret ) break;

		a = attr_find( e->e_attrs, ad_krb5KeyVersionNumber );
		kvno = 0;
		if ( a ) {
			if ( lutil_atoi( &kvno, a->a_vals[0].bv_val ) != 0 ) {
				Debug( LDAP_DEBUG_ANY, "%s smbk5pwd EXOP: "
					"dn=\"%s\" unable to parse krb5KeyVersionNumber=\"%s\"\n",
					op->o_log_prefix, e->e_name.bv_val, a->a_vals[0].bv_val );
			}

		} else {
			/* shouldn't happen, this is a required attr */
			Debug( LDAP_DEBUG_ANY, "%s smbk5pwd EXOP: "
				"dn=\"%s\" missing krb5KeyVersionNumber\n",
				op->o_log_prefix, e->e_name.bv_val, 0 );
		}

		ret = hdb_generate_key_set_password(context, ent.principal,
			qpw->rs_new.bv_val, &ent.keys.val, &nkeys);
		ent.keys.len = nkeys;
		hdb_seal_keys(context, db, &ent);
		krb5_free_principal( context, ent.principal );

		keys = ch_malloc( (ent.keys.len + 1) * sizeof(struct berval));

		for (i = 0; i < ent.keys.len; i++) {
			unsigned char *buf;
			size_t len;

			ASN1_MALLOC_ENCODE(Key, buf, len, &ent.keys.val[i], &len, ret);
			if (ret != 0)
				break;
			
			keys[i].bv_val = (char *)buf;
			keys[i].bv_len = len;
		}
		BER_BVZERO( &keys[i] );

		hdb_free_keys(context, ent.keys.len, ent.keys.val);

		if ( i != ent.keys.len ) {
			ber_bvarray_free( keys );
			break;
		}

		ml = ch_malloc(sizeof(Modifications));
		if (!qpw->rs_modtail) qpw->rs_modtail = &ml->sml_next;
		ml->sml_next = qpw->rs_mods;
		qpw->rs_mods = ml;

		ml->sml_desc = ad_krb5Key;
		ml->sml_op = LDAP_MOD_REPLACE;
#ifdef SLAP_MOD_INTERNAL
		ml->sml_flags = SLAP_MOD_INTERNAL;
#endif
		ml->sml_numvals = i;
		ml->sml_values = keys;
		ml->sml_nvalues = NULL;
		
		ml = ch_malloc(sizeof(Modifications));
		ml->sml_next = qpw->rs_mods;
		qpw->rs_mods = ml;
		
		ml->sml_desc = ad_krb5KeyVersionNumber;
		ml->sml_op = LDAP_MOD_REPLACE;
#ifdef SLAP_MOD_INTERNAL
		ml->sml_flags = SLAP_MOD_INTERNAL;
#endif
		ml->sml_numvals = 1;
		ml->sml_values = ch_malloc( 2 * sizeof(struct berval));
		ml->sml_values[0].bv_val = ch_malloc( 64 );
		ml->sml_values[0].bv_len = sprintf(ml->sml_values[0].bv_val,
			"%d", kvno+1 );
		BER_BVZERO( &ml->sml_values[1] );
		ml->sml_nvalues = NULL;
	} while ( 0 );
#endif /* DO_KRB5 */

#ifdef DO_SAMBA
	/* Samba stuff */
	if ( SMBK5PWD_DO_SAMBA( pi ) && is_entry_objectclass(e, oc_sambaSamAccount, 0 ) ) {
		struct berval *keys;
		ber_len_t j,l;
		wchar_t *wcs, wc;
		char *c, *d;
		struct berval pwd;
		
		/* Expand incoming UTF8 string to UCS4 */
		l = ldap_utf8_chars(qpw->rs_new.bv_val);
		wcs = ch_malloc((l+1) * sizeof(wchar_t));

		ldap_x_utf8s_to_wcs( wcs, qpw->rs_new.bv_val, l );
		
		/* Truncate UCS4 to UCS2 */
		c = (char *)wcs;
		for (j=0; j<l; j++) {
			wc = wcs[j];
			*c++ = wc & 0xff;
			*c++ = (wc >> 8) & 0xff;
		}
		*c++ = 0;
		pwd.bv_val = (char *)wcs;
		pwd.bv_len = l * 2;

		ml = ch_malloc(sizeof(Modifications));
		if (!qpw->rs_modtail) qpw->rs_modtail = &ml->sml_next;
		ml->sml_next = qpw->rs_mods;
		qpw->rs_mods = ml;

		keys = ch_malloc( 2 * sizeof(struct berval) );
		BER_BVZERO( &keys[1] );
		nthash( &pwd, keys );
		
		ml->sml_desc = ad_sambaNTPassword;
		ml->sml_op = LDAP_MOD_REPLACE;
#ifdef SLAP_MOD_INTERNAL
		ml->sml_flags = SLAP_MOD_INTERNAL;
#endif
		ml->sml_numvals = 1;
		ml->sml_values = keys;
		ml->sml_nvalues = NULL;

		/* Truncate UCS2 to 8-bit ASCII */
		c = pwd.bv_val+1;
		d = pwd.bv_val+2;
		for (j=1; j<l; j++) {
			*c++ = *d++;
			d++;
		}
		pwd.bv_len /= 2;
		pwd.bv_val[pwd.bv_len] = '\0';

		ml = ch_malloc(sizeof(Modifications));
		ml->sml_next = qpw->rs_mods;
		qpw->rs_mods = ml;

		keys = ch_malloc( 2 * sizeof(struct berval) );
		BER_BVZERO( &keys[1] );
		lmhash( &pwd, keys );
		
		ml->sml_desc = ad_sambaLMPassword;
		ml->sml_op = LDAP_MOD_REPLACE;
#ifdef SLAP_MOD_INTERNAL
		ml->sml_flags = SLAP_MOD_INTERNAL;
#endif
		ml->sml_numvals = 1;
		ml->sml_values = keys;
		ml->sml_nvalues = NULL;

		ch_free(wcs);

		ml = ch_malloc(sizeof(Modifications));
		ml->sml_next = qpw->rs_mods;
		qpw->rs_mods = ml;

		keys = ch_malloc( 2 * sizeof(struct berval) );
		keys[0].bv_val = ch_malloc( LDAP_PVT_INTTYPE_CHARS(long) );
		keys[0].bv_len = snprintf(keys[0].bv_val,
			LDAP_PVT_INTTYPE_CHARS(long),
			"%ld", slap_get_time());
		BER_BVZERO( &keys[1] );
		
		ml->sml_desc = ad_sambaPwdLastSet;
		ml->sml_op = LDAP_MOD_REPLACE;
#ifdef SLAP_MOD_INTERNAL
		ml->sml_flags = SLAP_MOD_INTERNAL;
#endif
		ml->sml_numvals = 1;
		ml->sml_values = keys;
		ml->sml_nvalues = NULL;

		if (pi->smb_must_change)
		{
			ml = ch_malloc(sizeof(Modifications));
			ml->sml_next = qpw->rs_mods;
			qpw->rs_mods = ml;

			keys = ch_malloc( 2 * sizeof(struct berval) );
			keys[0].bv_val = ch_malloc( LDAP_PVT_INTTYPE_CHARS(long) );
			keys[0].bv_len = snprintf(keys[0].bv_val,
					LDAP_PVT_INTTYPE_CHARS(long),
					"%ld", slap_get_time() + pi->smb_must_change);
			BER_BVZERO( &keys[1] );

			ml->sml_desc = ad_sambaPwdMustChange;
			ml->sml_op = LDAP_MOD_REPLACE;
#ifdef SLAP_MOD_INTERNAL
			ml->sml_flags = SLAP_MOD_INTERNAL;
#endif
			ml->sml_numvals = 1;
			ml->sml_values = keys;
			ml->sml_nvalues = NULL;
		}

		if (pi->smb_can_change)
		{
			ml = ch_malloc(sizeof(Modifications));
			ml->sml_next = qpw->rs_mods;
			qpw->rs_mods = ml;

			keys = ch_malloc( 2 * sizeof(struct berval) );
			keys[0].bv_val = ch_malloc( LDAP_PVT_INTTYPE_CHARS(long) );
			keys[0].bv_len = snprintf(keys[0].bv_val,
					LDAP_PVT_INTTYPE_CHARS(long),
					"%ld", slap_get_time() + pi->smb_can_change);
			BER_BVZERO( &keys[1] );

			ml->sml_desc = ad_sambaPwdCanChange;
			ml->sml_op = LDAP_MOD_REPLACE;
#ifdef SLAP_MOD_INTERNAL
			ml->sml_flags = SLAP_MOD_INTERNAL;
#endif
			ml->sml_numvals = 1;
			ml->sml_values = keys;
			ml->sml_nvalues = NULL;
		}
	}
#endif /* DO_SAMBA */

#ifdef DO_SHADOW
	/* shadow stuff */
	if ( SMBK5PWD_DO_SHADOW( pi ) && is_entry_objectclass(e, oc_shadowAccount, 0 ) ) {
		struct berval *keys;

		ml = ch_malloc(sizeof(Modifications));
		if (!qpw->rs_modtail) qpw->rs_modtail = &ml->sml_next;
		ml->sml_next = qpw->rs_mods;
		qpw->rs_mods = ml;

		keys = ch_malloc( sizeof(struct berval) * 2);
		BER_BVZERO( &keys[1] );
		keys[0].bv_val = ch_malloc( LDAP_PVT_INTTYPE_CHARS(long) );
		keys[0].bv_len = snprintf(keys[0].bv_val,
			LDAP_PVT_INTTYPE_CHARS(long),
			"%ld", (long)(slap_get_time() / (60 * 60 * 24)));

		ml->sml_desc = ad_shadowLastChange;
		ml->sml_op = LDAP_MOD_REPLACE;
#ifdef SLAP_MOD_INTERNAL
		ml->sml_flags = SLAP_MOD_INTERNAL;
#endif
		ml->sml_numvals = 1;
		ml->sml_values = keys;
		ml->sml_nvalues = NULL;
	}
#endif /* DO_SHADOW */

	be_entry_release_r( op, e );
	qpw->rs_new.bv_val[qpw->rs_new.bv_len] = term;

	return SLAP_CB_CONTINUE;
}

static slap_overinst smbk5pwd;

/* back-config stuff */
enum {
	PC_SMB_MUST_CHANGE = 1,
	PC_SMB_CAN_CHANGE,
	PC_SMB_ENABLE
};

static ConfigDriver smbk5pwd_cf_func;

/*
 * NOTE: uses OID arcs OLcfgCtAt:1 and OLcfgCtOc:1
 */

static ConfigTable smbk5pwd_cfats[] = {
	{ "smbk5pwd-enable", "arg",
		2, 0, 0, ARG_MAGIC|PC_SMB_ENABLE, smbk5pwd_cf_func,
		"( OLcfgCtAt:1.1 NAME 'olcSmbK5PwdEnable' "
		"DESC 'Modules to be enabled' "
		"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "smbk5pwd-must-change", "time",
		2, 2, 0, ARG_MAGIC|ARG_INT|PC_SMB_MUST_CHANGE, smbk5pwd_cf_func,
		"( OLcfgCtAt:1.2 NAME 'olcSmbK5PwdMustChange' "
		"DESC 'Credentials validity interval' "
		"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "smbk5pwd-can-change", "time",
		2, 2, 0, ARG_MAGIC|ARG_INT|PC_SMB_CAN_CHANGE, smbk5pwd_cf_func,
		"( OLcfgCtAt:1.3 NAME 'olcSmbK5PwdCanChange' "
		"DESC 'Credentials minimum validity interval' "
		"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs smbk5pwd_cfocs[] = {
	{ "( OLcfgCtOc:1.1 "
		"NAME 'olcSmbK5PwdConfig' "
		"DESC 'smbk5pwd overlay configuration' "
		"SUP olcOverlayConfig "
		"MAY ( "
			"olcSmbK5PwdEnable "
			"$ olcSmbK5PwdMustChange "
			"$ olcSmbK5PwdCanChange "
		") )", Cft_Overlay, smbk5pwd_cfats },

	{ NULL, 0, NULL }
};

/*
 * add here other functionalities; handle their initialization
 * as appropriate in smbk5pwd_modules_init().
 */
static slap_verbmasks smbk5pwd_modules[] = {
	{ BER_BVC( "krb5" ),		SMBK5PWD_F_KRB5	},
	{ BER_BVC( "samba" ),		SMBK5PWD_F_SAMBA },
	{ BER_BVC( "shadow" ),		SMBK5PWD_F_SHADOW },
	{ BER_BVNULL,			-1 }
};

static int
smbk5pwd_cf_func( ConfigArgs *c )
{
	slap_overinst	*on = (slap_overinst *)c->bi;

	int		rc = 0;
	smbk5pwd_t	*pi = on->on_bi.bi_private;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch( c->type ) {
		case PC_SMB_MUST_CHANGE:
#ifdef DO_SAMBA
			c->value_int = pi->smb_must_change;
#else /* ! DO_SAMBA */
			c->value_int = 0;
#endif /* ! DO_SAMBA */
			break;

		case PC_SMB_CAN_CHANGE:
#ifdef DO_SAMBA
			c->value_int = pi->smb_can_change;
#else /* ! DO_SAMBA */
			c->value_int = 0;
#endif /* ! DO_SAMBA */
			break;

		case PC_SMB_ENABLE:
			c->rvalue_vals = NULL;
			if ( pi->mode ) {
				mask_to_verbs( smbk5pwd_modules, pi->mode, &c->rvalue_vals );
				if ( c->rvalue_vals == NULL ) {
					rc = 1;
				}
			}
			break;

		default:
			assert( 0 );
			rc = 1;
		}
		return rc;

	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch( c->type ) {
		case PC_SMB_MUST_CHANGE:
			break;

                case PC_SMB_CAN_CHANGE:
                        break;

		case PC_SMB_ENABLE:
			if ( !c->line ) {
				pi->mode = 0;

			} else {
				int i;

				i = verb_to_mask( c->line, smbk5pwd_modules );
				pi->mode &= ~smbk5pwd_modules[i].mask;
			}
			break;

		default:
			assert( 0 );
			rc = 1;
		}
		return rc;
	}

	switch( c->type ) {
	case PC_SMB_MUST_CHANGE:
#ifdef DO_SAMBA
		if ( c->value_int < 0 ) {
			Debug( LDAP_DEBUG_ANY, "%s: smbk5pwd: "
				"<%s> invalid negative value \"%d\".",
				c->log, c->argv[ 0 ], 0 );
			return 1;
		}
		pi->smb_must_change = c->value_int;
#else /* ! DO_SAMBA */
		Debug( LDAP_DEBUG_ANY, "%s: smbk5pwd: "
			"<%s> only meaningful "
			"when compiled with -DDO_SAMBA.\n",
			c->log, c->argv[ 0 ], 0 );
		return 1;
#endif /* ! DO_SAMBA */
		break;

        case PC_SMB_CAN_CHANGE:
#ifdef DO_SAMBA
                if ( c->value_int < 0 ) {
                        Debug( LDAP_DEBUG_ANY, "%s: smbk5pwd: "
                                "<%s> invalid negative value \"%d\".",
                                c->log, c->argv[ 0 ], 0 );
                        return 1;
                }
                pi->smb_can_change = c->value_int;
#else /* ! DO_SAMBA */
                Debug( LDAP_DEBUG_ANY, "%s: smbk5pwd: "
                        "<%s> only meaningful "
                        "when compiled with -DDO_SAMBA.\n",
                        c->log, c->argv[ 0 ], 0 );
                return 1;
#endif /* ! DO_SAMBA */
                break;

	case PC_SMB_ENABLE: {
		slap_mask_t	mode = pi->mode, m = 0;

		rc = verbs_to_mask( c->argc, c->argv, smbk5pwd_modules, &m );
		if ( rc > 0 ) {
			Debug( LDAP_DEBUG_ANY, "%s: smbk5pwd: "
				"<%s> unknown module \"%s\".\n",
				c->log, c->argv[ 0 ], c->argv[ rc ] );
			return 1;
		}

		/* we can hijack the smbk5pwd_t structure because
		 * from within the configuration, this is the only
		 * active thread. */
		pi->mode |= m;

#ifndef DO_KRB5
		if ( SMBK5PWD_DO_KRB5( pi ) ) {
			Debug( LDAP_DEBUG_ANY, "%s: smbk5pwd: "
				"<%s> module \"%s\" only allowed when compiled with -DDO_KRB5.\n",
				c->log, c->argv[ 0 ], c->argv[ rc ] );
			pi->mode = mode;
			return 1;
		}
#endif /* ! DO_KRB5 */

#ifndef DO_SAMBA
		if ( SMBK5PWD_DO_SAMBA( pi ) ) {
			Debug( LDAP_DEBUG_ANY, "%s: smbk5pwd: "
				"<%s> module \"%s\" only allowed when compiled with -DDO_SAMBA.\n",
				c->log, c->argv[ 0 ], c->argv[ rc ] );
			pi->mode = mode;
			return 1;
		}
#endif /* ! DO_SAMBA */

#ifndef DO_SHADOW
		if ( SMBK5PWD_DO_SHADOW( pi ) ) {
			Debug( LDAP_DEBUG_ANY, "%s: smbk5pwd: "
				"<%s> module \"%s\" only allowed when compiled with -DDO_SHADOW.\n",
				c->log, c->argv[ 0 ], c->argv[ rc ] );
			pi->mode = mode;
			return 1;
		}
#endif /* ! DO_SHADOW */

		{
			BackendDB	db = *c->be;

			/* Re-initialize the module, because
			 * the configuration might have changed */
			db.bd_info = (BackendInfo *)on;
			rc = smbk5pwd_modules_init( pi );
			if ( rc ) {
				pi->mode = mode;
				return 1;
			}
		}

		} break;

	default:
		assert( 0 );
		return 1;
	}
	return rc;
}

static int
smbk5pwd_modules_init( smbk5pwd_t *pi )
{
	static struct {
		const char		*name;
		AttributeDescription	**adp;
	}
#ifdef DO_KRB5
	krb5_ad[] = {
		{ "krb5Key",			&ad_krb5Key },
		{ "krb5KeyVersionNumber",	&ad_krb5KeyVersionNumber },
		{ "krb5PrincipalName",		&ad_krb5PrincipalName },
		{ "krb5ValidEnd",		&ad_krb5ValidEnd },
		{ NULL }
	},
#endif /* DO_KRB5 */
#ifdef DO_SAMBA
	samba_ad[] = {
		{ "sambaLMPassword",		&ad_sambaLMPassword },
		{ "sambaNTPassword",		&ad_sambaNTPassword },
		{ "sambaPwdLastSet",		&ad_sambaPwdLastSet },
		{ "sambaPwdMustChange",		&ad_sambaPwdMustChange },
		{ "sambaPwdCanChange",		&ad_sambaPwdCanChange },
		{ NULL }
	},
#endif /* DO_SAMBA */
#ifdef DO_SHADOW
	shadow_ad[] = {
		{ "shadowLastChange",		&ad_shadowLastChange },
		{ NULL }
	},
#endif /* DO_SHADOW */
	dummy_ad;

	/* this is to silence the unused var warning */
	dummy_ad.name = NULL;

#ifdef DO_KRB5
	if ( SMBK5PWD_DO_KRB5( pi ) && oc_krb5KDCEntry == NULL ) {
		krb5_error_code	ret;
		extern HDB 	*_kadm5_s_get_db(void *);

		int		i, rc;

		/* Make sure all of our necessary schema items are loaded */
		oc_krb5KDCEntry = oc_find( "krb5KDCEntry" );
		if ( !oc_krb5KDCEntry ) {
			Debug( LDAP_DEBUG_ANY, "smbk5pwd: "
				"unable to find \"krb5KDCEntry\" objectClass.\n",
				0, 0, 0 );
			return -1;
		}

		for ( i = 0; krb5_ad[ i ].name != NULL; i++ ) {
			const char	*text;

			*(krb5_ad[ i ].adp) = NULL;

			rc = slap_str2ad( krb5_ad[ i ].name, krb5_ad[ i ].adp, &text );
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY, "smbk5pwd: "
					"unable to find \"%s\" attributeType: %s (%d).\n",
					krb5_ad[ i ].name, text, rc );
				oc_krb5KDCEntry = NULL;
				return rc;
			}
		}

		/* Initialize Kerberos context */
		ret = krb5_init_context(&context);
		if (ret) {
			Debug( LDAP_DEBUG_ANY, "smbk5pwd: "
				"unable to initialize krb5 context (%d).\n",
				ret, 0, 0 );
			oc_krb5KDCEntry = NULL;
			return -1;
		}

		ret = kadm5_s_init_with_password_ctx( context,
			KADM5_ADMIN_SERVICE,
			NULL,
			KADM5_ADMIN_SERVICE,
			&conf, 0, 0, &kadm_context );
		if (ret) {
			char *err_str, *err_msg = "<unknown error>";
			err_str = krb5_get_error_string( context );
			if (!err_str)
				err_msg = (char *)krb5_get_err_text( context, ret );
			Debug( LDAP_DEBUG_ANY, "smbk5pwd: "
				"unable to initialize krb5 admin context: %s (%d).\n",
				err_str ? err_str : err_msg, ret, 0 );
			if (err_str)
				krb5_free_error_string( context, err_str );
			krb5_free_context( context );
			oc_krb5KDCEntry = NULL;
			return -1;
		}

		db = _kadm5_s_get_db( kadm_context );
	}
#endif /* DO_KRB5 */

#ifdef DO_SAMBA
	if ( SMBK5PWD_DO_SAMBA( pi ) && oc_sambaSamAccount == NULL ) {
		int		i, rc;

		oc_sambaSamAccount = oc_find( "sambaSamAccount" );
		if ( !oc_sambaSamAccount ) {
			Debug( LDAP_DEBUG_ANY, "smbk5pwd: "
				"unable to find \"sambaSamAccount\" objectClass.\n",
				0, 0, 0 );
			return -1;
		}

		for ( i = 0; samba_ad[ i ].name != NULL; i++ ) {
			const char	*text;

			*(samba_ad[ i ].adp) = NULL;

			rc = slap_str2ad( samba_ad[ i ].name, samba_ad[ i ].adp, &text );
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY, "smbk5pwd: "
					"unable to find \"%s\" attributeType: %s (%d).\n",
					samba_ad[ i ].name, text, rc );
				oc_sambaSamAccount = NULL;
				return rc;
			}
		}
	}
#endif /* DO_SAMBA */

#ifdef DO_SHADOW
	if ( SMBK5PWD_DO_SHADOW( pi ) && oc_shadowAccount == NULL ) {
		int		i, rc;

		oc_shadowAccount = oc_find( "shadowAccount" );
		if ( !oc_shadowAccount ) {
			Debug( LDAP_DEBUG_ANY, "smbk5pwd: "
				"unable to find \"shadowAccount\" objectClass.\n",
				0, 0, 0 );
			return -1;
		}

		for ( i = 0; shadow_ad[ i ].name != NULL; i++ ) {
			const char	*text;

			*(shadow_ad[ i ].adp) = NULL;

			rc = slap_str2ad( shadow_ad[ i ].name, shadow_ad[ i ].adp, &text );
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY, "smbk5pwd: "
					"unable to find \"%s\" attributeType: %s (%d).\n",
					shadow_ad[ i ].name, text, rc );
				oc_shadowAccount = NULL;
				return rc;
			}
		}
	}
#endif /* DO_SHADOW */

	return 0;
}

static int
smbk5pwd_db_init(BackendDB *be, ConfigReply *cr)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	smbk5pwd_t	*pi;

	pi = ch_calloc( 1, sizeof( smbk5pwd_t ) );
	if ( pi == NULL ) {
		return 1;
	}
	on->on_bi.bi_private = (void *)pi;

	return 0;
}

static int
smbk5pwd_db_open(BackendDB *be, ConfigReply *cr)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	smbk5pwd_t	*pi = (smbk5pwd_t *)on->on_bi.bi_private;

	int	rc;

	if ( pi->mode == 0 ) {
		pi->mode = SMBK5PWD_F_ALL;
	}

	rc = smbk5pwd_modules_init( pi );
	if ( rc ) {
		return rc;
	}

	return 0;
}

static int
smbk5pwd_db_destroy(BackendDB *be, ConfigReply *cr)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	smbk5pwd_t	*pi = (smbk5pwd_t *)on->on_bi.bi_private;

	if ( pi ) {
		ch_free( pi );
	}

	return 0;
}

int
smbk5pwd_initialize(void)
{
	int		rc;

	smbk5pwd.on_bi.bi_type = "smbk5pwd";

	smbk5pwd.on_bi.bi_db_init = smbk5pwd_db_init;
	smbk5pwd.on_bi.bi_db_open = smbk5pwd_db_open;
	smbk5pwd.on_bi.bi_db_destroy = smbk5pwd_db_destroy;

	smbk5pwd.on_bi.bi_extended = smbk5pwd_exop_passwd;
    
#ifdef DO_KRB5
	smbk5pwd.on_bi.bi_op_bind = smbk5pwd_op_bind;

	lutil_passwd_add( (struct berval *)&k5key_scheme, k5key_chk, k5key_hash );
#endif

	smbk5pwd.on_bi.bi_cf_ocs = smbk5pwd_cfocs;

	rc = config_register_schema( smbk5pwd_cfats, smbk5pwd_cfocs );
	if ( rc ) {
		return rc;
	}

	return overlay_register( &smbk5pwd );
}

#if SLAPD_OVER_SMBK5PWD == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return smbk5pwd_initialize();
}
#endif

#endif /* defined(SLAPD_OVER_SMBK5PWD) */
