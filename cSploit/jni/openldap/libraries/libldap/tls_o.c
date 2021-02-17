/* tls_o.c - Handle tls/ssl using OpenSSL */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENTS: Rewritten by Howard Chu
 */

#include "portable.h"

#ifdef HAVE_OPENSSL

#include "ldap_config.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/time.h>
#include <ac/unistd.h>
#include <ac/param.h>
#include <ac/dirent.h>

#include "ldap-int.h"
#include "ldap-tls.h"

#ifdef HAVE_OPENSSL_SSL_H
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/safestack.h>
#elif defined( HAVE_SSL_H )
#include <ssl.h>
#endif

#if defined(ANDROID) || defined(__BIONIC__)
#include <openssl/bn.h>
#include <openssl/rsa.h>
#endif

typedef SSL_CTX tlso_ctx;
typedef SSL tlso_session;

static int  tlso_opt_trace = 1;

static void tlso_report_error( void );

static void tlso_info_cb( const SSL *ssl, int where, int ret );
static int tlso_verify_cb( int ok, X509_STORE_CTX *ctx );
static int tlso_verify_ok( int ok, X509_STORE_CTX *ctx );
static RSA * tlso_tmp_rsa_cb( SSL *ssl, int is_export, int key_length );

static int tlso_seed_PRNG( const char *randfile );

#ifdef LDAP_R_COMPILE
/*
 * provide mutexes for the OpenSSL library.
 */
static ldap_pvt_thread_mutex_t	tlso_mutexes[CRYPTO_NUM_LOCKS];

static void tlso_locking_cb( int mode, int type, const char *file, int line )
{
	if ( mode & CRYPTO_LOCK ) {
		ldap_pvt_thread_mutex_lock( &tlso_mutexes[type] );
	} else {
		ldap_pvt_thread_mutex_unlock( &tlso_mutexes[type] );
	}
}

static unsigned long tlso_thread_self( void )
{
	/* FIXME: CRYPTO_set_id_callback only works when ldap_pvt_thread_t
	 * is an integral type that fits in an unsigned long
	 */

	/* force an error if the ldap_pvt_thread_t type is too large */
	enum { ok = sizeof( ldap_pvt_thread_t ) <= sizeof( unsigned long ) };
	typedef struct { int dummy: ok ? 1 : -1; } Check[ok ? 1 : -1];

	return (unsigned long) ldap_pvt_thread_self();
}

static void tlso_thr_init( void )
{
	int i;

	for( i=0; i< CRYPTO_NUM_LOCKS ; i++ ) {
		ldap_pvt_thread_mutex_init( &tlso_mutexes[i] );
	}
	CRYPTO_set_locking_callback( tlso_locking_cb );
	CRYPTO_set_id_callback( tlso_thread_self );
}
#endif /* LDAP_R_COMPILE */

static STACK_OF(X509_NAME) *
tlso_ca_list( char * bundle, char * dir )
{
	STACK_OF(X509_NAME) *ca_list = NULL;

	if ( bundle ) {
		ca_list = SSL_load_client_CA_file( bundle );
	}
#if defined(HAVE_DIRENT_H) || defined(dirent)
	if ( dir ) {
		int freeit = 0;

		if ( !ca_list ) {
			ca_list = sk_X509_NAME_new_null();
			freeit = 1;
		}
		if ( !SSL_add_dir_cert_subjects_to_stack( ca_list, dir ) &&
			freeit ) {
			sk_X509_NAME_free( ca_list );
			ca_list = NULL;
		}
	}
#endif
	return ca_list;
}

/*
 * Initialize TLS subsystem. Should be called only once.
 */
static int
tlso_init( void )
{
	struct ldapoptions *lo = LDAP_INT_GLOBAL_OPT();   
#ifdef HAVE_EBCDIC
	{
		char *file = LDAP_STRDUP( lo->ldo_tls_randfile );
		if ( file ) __atoe( file );
		(void) tlso_seed_PRNG( file );
		LDAP_FREE( file );
	}
#else
	(void) tlso_seed_PRNG( lo->ldo_tls_randfile );
#endif

	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_digests();

	/* FIXME: mod_ssl does this */
	X509V3_add_standard_extensions();

	return 0;
}

/*
 * Tear down the TLS subsystem. Should only be called once.
 */
static void
tlso_destroy( void )
{
	struct ldapoptions *lo = LDAP_INT_GLOBAL_OPT();   

	EVP_cleanup();
	ERR_remove_state(0);
	ERR_free_strings();

	if ( lo->ldo_tls_randfile ) {
		LDAP_FREE( lo->ldo_tls_randfile );
		lo->ldo_tls_randfile = NULL;
	}
}

static tls_ctx *
tlso_ctx_new( struct ldapoptions *lo )
{
	return (tls_ctx *) SSL_CTX_new( SSLv23_method() );
}

static void
tlso_ctx_ref( tls_ctx *ctx )
{
	tlso_ctx *c = (tlso_ctx *)ctx;
	CRYPTO_add( &c->references, 1, CRYPTO_LOCK_SSL_CTX );
}

static void
tlso_ctx_free ( tls_ctx *ctx )
{
	tlso_ctx *c = (tlso_ctx *)ctx;
	SSL_CTX_free( c );
}

/*
 * initialize a new TLS context
 */
static int
tlso_ctx_init( struct ldapoptions *lo, struct ldaptls *lt, int is_server )
{
	tlso_ctx *ctx = (tlso_ctx *)lo->ldo_tls_ctx;
	int i;

	if ( is_server ) {
		SSL_CTX_set_session_id_context( ctx,
			(const unsigned char *) "OpenLDAP", sizeof("OpenLDAP")-1 );
	}

#ifdef SSL_OP_NO_TLSv1
#ifdef SSL_OP_NO_TLSv1_1
#ifdef SSL_OP_NO_TLSv1_2
	if ( lo->ldo_tls_protocol_min > LDAP_OPT_X_TLS_PROTOCOL_TLS1_2)
		SSL_CTX_set_options( ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
			SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 |
			SSL_OP_NO_TLSv1_2 );
	else
#endif
	if ( lo->ldo_tls_protocol_min > LDAP_OPT_X_TLS_PROTOCOL_TLS1_1)
		SSL_CTX_set_options( ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
			SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 );
	else
#endif
	if ( lo->ldo_tls_protocol_min > LDAP_OPT_X_TLS_PROTOCOL_TLS1_0)
		SSL_CTX_set_options( ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
			SSL_OP_NO_TLSv1);
	else
#endif
	if ( lo->ldo_tls_protocol_min > LDAP_OPT_X_TLS_PROTOCOL_SSL3 )
		SSL_CTX_set_options( ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 );
	else if ( lo->ldo_tls_protocol_min > LDAP_OPT_X_TLS_PROTOCOL_SSL2 )
		SSL_CTX_set_options( ctx, SSL_OP_NO_SSLv2 );

	if ( lo->ldo_tls_ciphersuite &&
		!SSL_CTX_set_cipher_list( ctx, lt->lt_ciphersuite ) )
	{
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not set cipher list %s.\n",
			   lo->ldo_tls_ciphersuite, 0, 0 );
		tlso_report_error();
		return -1;
	}

	if (lo->ldo_tls_cacertfile != NULL || lo->ldo_tls_cacertdir != NULL) {
		if ( !SSL_CTX_load_verify_locations( ctx,
				lt->lt_cacertfile, lt->lt_cacertdir ) ||
			!SSL_CTX_set_default_verify_paths( ctx ) )
		{
			Debug( LDAP_DEBUG_ANY, "TLS: "
				"could not load verify locations (file:`%s',dir:`%s').\n",
				lo->ldo_tls_cacertfile ? lo->ldo_tls_cacertfile : "",
				lo->ldo_tls_cacertdir ? lo->ldo_tls_cacertdir : "",
				0 );
			tlso_report_error();
			return -1;
		}

		if ( is_server ) {
			STACK_OF(X509_NAME) *calist;
			/* List of CA names to send to a client */
			calist = tlso_ca_list( lt->lt_cacertfile, lt->lt_cacertdir );
			if ( !calist ) {
				Debug( LDAP_DEBUG_ANY, "TLS: "
					"could not load client CA list (file:`%s',dir:`%s').\n",
					lo->ldo_tls_cacertfile ? lo->ldo_tls_cacertfile : "",
					lo->ldo_tls_cacertdir ? lo->ldo_tls_cacertdir : "",
					0 );
				tlso_report_error();
				return -1;
			}

			SSL_CTX_set_client_CA_list( ctx, calist );
		}
	}

	if ( lo->ldo_tls_certfile &&
		!SSL_CTX_use_certificate_file( ctx,
			lt->lt_certfile, SSL_FILETYPE_PEM ) )
	{
		Debug( LDAP_DEBUG_ANY,
			"TLS: could not use certificate `%s'.\n",
			lo->ldo_tls_certfile,0,0);
		tlso_report_error();
		return -1;
	}

	/* Key validity is checked automatically if cert has already been set */
	if ( lo->ldo_tls_keyfile &&
		!SSL_CTX_use_PrivateKey_file( ctx,
			lt->lt_keyfile, SSL_FILETYPE_PEM ) )
	{
		Debug( LDAP_DEBUG_ANY,
			"TLS: could not use key file `%s'.\n",
			lo->ldo_tls_keyfile,0,0);
		tlso_report_error();
		return -1;
	}

	if ( is_server && lo->ldo_tls_dhfile ) {
		DH *dh;
		BIO *bio;

		if (( bio=BIO_new_file( lt->lt_dhfile,"r" )) == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"TLS: could not use DH parameters file `%s'.\n",
				lo->ldo_tls_dhfile,0,0);
			tlso_report_error();
			return -1;
		}
		if (!( dh=PEM_read_bio_DHparams( bio, NULL, NULL, NULL ))) {
			Debug( LDAP_DEBUG_ANY,
				"TLS: could not read DH parameters file `%s'.\n",
				lo->ldo_tls_dhfile,0,0);
			tlso_report_error();
			BIO_free( bio );
			return -1;
		}
		BIO_free( bio );
		SSL_CTX_set_tmp_dh( ctx, dh );
		SSL_CTX_set_options( ctx, SSL_OP_SINGLE_DH_USE );
		DH_free( dh );
	}

	if ( is_server && lo->ldo_tls_ecname ) {
#ifdef OPENSSL_NO_EC
		Debug( LDAP_DEBUG_ANY,
			"TLS: Elliptic Curves not supported.\n", 0,0,0 );
		return -1;
#else
		EC_KEY *ecdh;

		int nid = OBJ_sn2nid( lt->lt_ecname );
		if ( nid == NID_undef ) {
			Debug( LDAP_DEBUG_ANY,
				"TLS: could not use EC name `%s'.\n",
				lo->ldo_tls_ecname,0,0);
			tlso_report_error();
			return -1;
		}
		ecdh = EC_KEY_new_by_curve_name( nid );
		if ( ecdh == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"TLS: could not generate key for EC name `%s'.\n",
				lo->ldo_tls_ecname,0,0);
			tlso_report_error();
			return -1;
		}
		SSL_CTX_set_tmp_ecdh( ctx, ecdh );
		SSL_CTX_set_options( ctx, SSL_OP_SINGLE_ECDH_USE );
		EC_KEY_free( ecdh );
#endif
	}

	if ( tlso_opt_trace ) {
		SSL_CTX_set_info_callback( ctx, tlso_info_cb );
	}

	i = SSL_VERIFY_NONE;
	if ( lo->ldo_tls_require_cert ) {
		i = SSL_VERIFY_PEER;
		if ( lo->ldo_tls_require_cert == LDAP_OPT_X_TLS_DEMAND ||
			 lo->ldo_tls_require_cert == LDAP_OPT_X_TLS_HARD ) {
			i |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
		}
	}

	SSL_CTX_set_verify( ctx, i,
		lo->ldo_tls_require_cert == LDAP_OPT_X_TLS_ALLOW ?
		tlso_verify_ok : tlso_verify_cb );
	SSL_CTX_set_tmp_rsa_callback( ctx, tlso_tmp_rsa_cb );
#ifdef HAVE_OPENSSL_CRL
	if ( lo->ldo_tls_crlcheck ) {
		X509_STORE *x509_s = SSL_CTX_get_cert_store( ctx );
		if ( lo->ldo_tls_crlcheck == LDAP_OPT_X_TLS_CRL_PEER ) {
			X509_STORE_set_flags( x509_s, X509_V_FLAG_CRL_CHECK );
		} else if ( lo->ldo_tls_crlcheck == LDAP_OPT_X_TLS_CRL_ALL ) {
			X509_STORE_set_flags( x509_s, 
					X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL  );
		}
	}
#endif
	return 0;
}

static tls_session *
tlso_session_new( tls_ctx *ctx, int is_server )
{
	tlso_ctx *c = (tlso_ctx *)ctx;
	return (tls_session *)SSL_new( c );
}

static int
tlso_session_connect( LDAP *ld, tls_session *sess )
{
	tlso_session *s = (tlso_session *)sess;

	/* Caller expects 0 = success, OpenSSL returns 1 = success */
	return SSL_connect( s ) - 1;
}

static int
tlso_session_accept( tls_session *sess )
{
	tlso_session *s = (tlso_session *)sess;

	/* Caller expects 0 = success, OpenSSL returns 1 = success */
	return SSL_accept( s ) - 1;
}

static int
tlso_session_upflags( Sockbuf *sb, tls_session *sess, int rc )
{
	tlso_session *s = (tlso_session *)sess;

	/* 1 was subtracted above, offset it back now */
	rc = SSL_get_error(s, rc+1);
	if (rc == SSL_ERROR_WANT_READ) {
		sb->sb_trans_needs_read  = 1;
		return 1;

	} else if (rc == SSL_ERROR_WANT_WRITE) {
		sb->sb_trans_needs_write = 1;
		return 1;

	} else if (rc == SSL_ERROR_WANT_CONNECT) {
		return 1;
	}
	return 0;
}

static char *
tlso_session_errmsg( tls_session *sess, int rc, char *buf, size_t len )
{
	char err[256] = "";
	const char *certerr=NULL;
	tlso_session *s = (tlso_session *)sess;

	rc = ERR_peek_error();
	if ( rc ) {
		ERR_error_string_n( rc, err, sizeof(err) );
		if ( ( ERR_GET_LIB(rc) == ERR_LIB_SSL ) && 
				( ERR_GET_REASON(rc) == SSL_R_CERTIFICATE_VERIFY_FAILED ) ) {
			int certrc = SSL_get_verify_result(s);
			certerr = (char *)X509_verify_cert_error_string(certrc);
		}
		snprintf(buf, len, "%s%s%s%s", err, certerr ? " (" :"", 
				certerr ? certerr : "", certerr ?  ")" : "" );
		return buf;
	}
	return NULL;
}

static int
tlso_session_my_dn( tls_session *sess, struct berval *der_dn )
{
	tlso_session *s = (tlso_session *)sess;
	X509 *x;
	X509_NAME *xn;

	x = SSL_get_certificate( s );

	if (!x) return LDAP_INVALID_CREDENTIALS;
	
	xn = X509_get_subject_name(x);
	der_dn->bv_len = i2d_X509_NAME( xn, NULL );
	der_dn->bv_val = xn->bytes->data;
	/* Don't X509_free, the session is still using it */
	return 0;
}

static X509 *
tlso_get_cert( SSL *s )
{
	/* If peer cert was bad, treat as if no cert was given */
	if (SSL_get_verify_result(s)) {
		return NULL;
	}
	return SSL_get_peer_certificate(s);
}

static int
tlso_session_peer_dn( tls_session *sess, struct berval *der_dn )
{
	tlso_session *s = (tlso_session *)sess;
	X509 *x = tlso_get_cert( s );
	X509_NAME *xn;

	if ( !x )
		return LDAP_INVALID_CREDENTIALS;

	xn = X509_get_subject_name(x);
	der_dn->bv_len = i2d_X509_NAME( xn, NULL );
	der_dn->bv_val = xn->bytes->data;
	X509_free(x);
	return 0;
}

/* what kind of hostname were we given? */
#define	IS_DNS	0
#define	IS_IP4	1
#define	IS_IP6	2

static int
tlso_session_chkhost( LDAP *ld, tls_session *sess, const char *name_in )
{
	tlso_session *s = (tlso_session *)sess;
	int i, ret = LDAP_LOCAL_ERROR;
	X509 *x;
	const char *name;
	char *ptr;
	int ntype = IS_DNS, nlen;
#ifdef LDAP_PF_INET6
	struct in6_addr addr;
#else
	struct in_addr addr;
#endif

	if( ldap_int_hostname &&
		( !name_in || !strcasecmp( name_in, "localhost" ) ) )
	{
		name = ldap_int_hostname;
	} else {
		name = name_in;
	}
	nlen = strlen(name);

	x = tlso_get_cert(s);
	if (!x) {
		Debug( LDAP_DEBUG_ANY,
			"TLS: unable to get peer certificate.\n",
			0, 0, 0 );
		/* If this was a fatal condition, things would have
		 * aborted long before now.
		 */
		return LDAP_SUCCESS;
	}

#ifdef LDAP_PF_INET6
	if (inet_pton(AF_INET6, name, &addr)) {
		ntype = IS_IP6;
	} else 
#endif
	if ((ptr = strrchr(name, '.')) && isdigit((unsigned char)ptr[1])) {
		if (inet_aton(name, (struct in_addr *)&addr)) ntype = IS_IP4;
	}
	
	i = X509_get_ext_by_NID(x, NID_subject_alt_name, -1);
	if (i >= 0) {
		X509_EXTENSION *ex;
		STACK_OF(GENERAL_NAME) *alt;

		ex = X509_get_ext(x, i);
		alt = X509V3_EXT_d2i(ex);
		if (alt) {
			int n, len2 = 0;
			char *domain = NULL;
			GENERAL_NAME *gn;

			if (ntype == IS_DNS) {
				domain = strchr(name, '.');
				if (domain) {
					len2 = nlen - (domain-name);
				}
			}
			n = sk_GENERAL_NAME_num(alt);
			for (i=0; i<n; i++) {
				char *sn;
				int sl;
				gn = sk_GENERAL_NAME_value(alt, i);
				if (gn->type == GEN_DNS) {
					if (ntype != IS_DNS) continue;

					sn = (char *) ASN1_STRING_data(gn->d.ia5);
					sl = ASN1_STRING_length(gn->d.ia5);

					/* ignore empty */
					if (sl == 0) continue;

					/* Is this an exact match? */
					if ((nlen == sl) && !strncasecmp(name, sn, nlen)) {
						break;
					}

					/* Is this a wildcard match? */
					if (domain && (sn[0] == '*') && (sn[1] == '.') &&
						(len2 == sl-1) && !strncasecmp(domain, &sn[1], len2))
					{
						break;
					}

				} else if (gn->type == GEN_IPADD) {
					if (ntype == IS_DNS) continue;

					sn = (char *) ASN1_STRING_data(gn->d.ia5);
					sl = ASN1_STRING_length(gn->d.ia5);

#ifdef LDAP_PF_INET6
					if (ntype == IS_IP6 && sl != sizeof(struct in6_addr)) {
						continue;
					} else
#endif
					if (ntype == IS_IP4 && sl != sizeof(struct in_addr)) {
						continue;
					}
					if (!memcmp(sn, &addr, sl)) {
						break;
					}
				}
			}

			GENERAL_NAMES_free(alt);
			if (i < n) {	/* Found a match */
				ret = LDAP_SUCCESS;
			}
		}
	}

	if (ret != LDAP_SUCCESS) {
		X509_NAME *xn;
		X509_NAME_ENTRY *ne;
		ASN1_OBJECT *obj;
		ASN1_STRING *cn = NULL;
		int navas;

		/* find the last CN */
		obj = OBJ_nid2obj( NID_commonName );
		if ( !obj ) goto no_cn;	/* should never happen */

		xn = X509_get_subject_name(x);
		navas = X509_NAME_entry_count( xn );
		for ( i=navas-1; i>=0; i-- ) {
			ne = X509_NAME_get_entry( xn, i );
			if ( !OBJ_cmp( ne->object, obj )) {
				cn = X509_NAME_ENTRY_get_data( ne );
				break;
			}
		}

		if( !cn )
		{
no_cn:
			Debug( LDAP_DEBUG_ANY,
				"TLS: unable to get common name from peer certificate.\n",
				0, 0, 0 );
			ret = LDAP_CONNECT_ERROR;
			if ( ld->ld_error ) {
				LDAP_FREE( ld->ld_error );
			}
			ld->ld_error = LDAP_STRDUP(
				_("TLS: unable to get CN from peer certificate"));

		} else if ( cn->length == nlen &&
			strncasecmp( name, (char *) cn->data, nlen ) == 0 ) {
			ret = LDAP_SUCCESS;

		} else if (( cn->data[0] == '*' ) && ( cn->data[1] == '.' )) {
			char *domain = strchr(name, '.');
			if( domain ) {
				int dlen;

				dlen = nlen - (domain-name);

				/* Is this a wildcard match? */
				if ((dlen == cn->length-1) &&
					!strncasecmp(domain, (char *) &cn->data[1], dlen)) {
					ret = LDAP_SUCCESS;
				}
			}
		}

		if( ret == LDAP_LOCAL_ERROR ) {
			Debug( LDAP_DEBUG_ANY, "TLS: hostname (%s) does not match "
				"common name in certificate (%.*s).\n", 
				name, cn->length, cn->data );
			ret = LDAP_CONNECT_ERROR;
			if ( ld->ld_error ) {
				LDAP_FREE( ld->ld_error );
			}
			ld->ld_error = LDAP_STRDUP(
				_("TLS: hostname does not match CN in peer certificate"));
		}
	}
	X509_free(x);
	return ret;
}

static int
tlso_session_strength( tls_session *sess )
{
	tlso_session *s = (tlso_session *)sess;

	return SSL_CIPHER_get_bits(SSL_get_current_cipher(s), NULL);
}

static int
tlso_session_unique( tls_session *sess, struct berval *buf, int is_server)
{
	tlso_session *s = (tlso_session *)sess;

	/* Usually the client sends the finished msg. But if the
	 * session was resumed, the server sent the msg.
	 */
	if (SSL_session_reused(s) ^ !is_server)
		buf->bv_len = SSL_get_finished(s, buf->bv_val, buf->bv_len);
	else
		buf->bv_len = SSL_get_peer_finished(s, buf->bv_val, buf->bv_len);
	return buf->bv_len;
}

static const char *
tlso_session_version( tls_session *sess )
{
	tlso_session *s = (tlso_session *)sess;
	return SSL_get_version(s);
}

static const char *
tlso_session_cipher( tls_session *sess )
{
	tlso_session *s = (tlso_session *)sess;
	return SSL_CIPHER_get_name(SSL_get_current_cipher(s));
}

static int
tlso_session_peercert( tls_session *sess, struct berval *der )
{
	tlso_session *s = (tlso_session *)sess;
	unsigned char *ptr;
	X509 *x = SSL_get_peer_certificate(s);
	der->bv_len = i2d_X509(x, NULL);
	der->bv_val = LDAP_MALLOC(der->bv_len);
	if ( !der->bv_val )
		return -1;
	ptr = der->bv_val;
	i2d_X509(x, &ptr);
	return 0;
}

/*
 * TLS support for LBER Sockbufs
 */

struct tls_data {
	tlso_session		*session;
	Sockbuf_IO_Desc		*sbiod;
};

static int
tlso_bio_create( BIO *b ) {
	b->init = 1;
	b->num = 0;
	b->ptr = NULL;
	b->flags = 0;
	return 1;
}

static int
tlso_bio_destroy( BIO *b )
{
	if ( b == NULL ) return 0;

	b->ptr = NULL;		/* sb_tls_remove() will free it */
	b->init = 0;
	b->flags = 0;
	return 1;
}

static int
tlso_bio_read( BIO *b, char *buf, int len )
{
	struct tls_data		*p;
	int			ret;
		
	if ( buf == NULL || len <= 0 ) return 0;

	p = (struct tls_data *)b->ptr;

	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	ret = LBER_SBIOD_READ_NEXT( p->sbiod, buf, len );

	BIO_clear_retry_flags( b );
	if ( ret < 0 ) {
		int err = sock_errno();
		if ( err == EAGAIN || err == EWOULDBLOCK ) {
			BIO_set_retry_read( b );
		}
	}

	return ret;
}

static int
tlso_bio_write( BIO *b, const char *buf, int len )
{
	struct tls_data		*p;
	int			ret;
	
	if ( buf == NULL || len <= 0 ) return 0;
	
	p = (struct tls_data *)b->ptr;

	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	ret = LBER_SBIOD_WRITE_NEXT( p->sbiod, (char *)buf, len );

	BIO_clear_retry_flags( b );
	if ( ret < 0 ) {
		int err = sock_errno();
		if ( err == EAGAIN || err == EWOULDBLOCK ) {
			BIO_set_retry_write( b );
		}
	}

	return ret;
}

static long
tlso_bio_ctrl( BIO *b, int cmd, long num, void *ptr )
{
	if ( cmd == BIO_CTRL_FLUSH ) {
		/* The OpenSSL library needs this */
		return 1;
	}
	return 0;
}

static int
tlso_bio_gets( BIO *b, char *buf, int len )
{
	return -1;
}

static int
tlso_bio_puts( BIO *b, const char *str )
{
	return tlso_bio_write( b, str, strlen( str ) );
}
	
static BIO_METHOD tlso_bio_method =
{
	( 100 | 0x400 ),		/* it's a source/sink BIO */
	"sockbuf glue",
	tlso_bio_write,
	tlso_bio_read,
	tlso_bio_puts,
	tlso_bio_gets,
	tlso_bio_ctrl,
	tlso_bio_create,
	tlso_bio_destroy
};

static int
tlso_sb_setup( Sockbuf_IO_Desc *sbiod, void *arg )
{
	struct tls_data		*p;
	BIO			*bio;

	assert( sbiod != NULL );

	p = LBER_MALLOC( sizeof( *p ) );
	if ( p == NULL ) {
		return -1;
	}
	
	p->session = arg;
	p->sbiod = sbiod;
	bio = BIO_new( &tlso_bio_method );
	bio->ptr = (void *)p;
	SSL_set_bio( p->session, bio, bio );
	sbiod->sbiod_pvt = p;
	return 0;
}

static int
tlso_sb_remove( Sockbuf_IO_Desc *sbiod )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	SSL_free( p->session );
	LBER_FREE( sbiod->sbiod_pvt );
	sbiod->sbiod_pvt = NULL;
	return 0;
}

static int
tlso_sb_close( Sockbuf_IO_Desc *sbiod )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	SSL_shutdown( p->session );
	return 0;
}

static int
tlso_sb_ctrl( Sockbuf_IO_Desc *sbiod, int opt, void *arg )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	
	if ( opt == LBER_SB_OPT_GET_SSL ) {
		*((tlso_session **)arg) = p->session;
		return 1;

	} else if ( opt == LBER_SB_OPT_DATA_READY ) {
		if( SSL_pending( p->session ) > 0 ) {
			return 1;
		}
	}
	
	return LBER_SBIOD_CTRL_NEXT( sbiod, opt, arg );
}

static ber_slen_t
tlso_sb_read( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data		*p;
	ber_slen_t		ret;
	int			err;

	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct tls_data *)sbiod->sbiod_pvt;

	ret = SSL_read( p->session, (char *)buf, len );
#ifdef HAVE_WINSOCK
	errno = WSAGetLastError();
#endif
	err = SSL_get_error( p->session, ret );
	if (err == SSL_ERROR_WANT_READ ) {
		sbiod->sbiod_sb->sb_trans_needs_read = 1;
		sock_errset(EWOULDBLOCK);
	}
	else
		sbiod->sbiod_sb->sb_trans_needs_read = 0;
	return ret;
}

static ber_slen_t
tlso_sb_write( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data		*p;
	ber_slen_t		ret;
	int			err;

	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct tls_data *)sbiod->sbiod_pvt;

	ret = SSL_write( p->session, (char *)buf, len );
#ifdef HAVE_WINSOCK
	errno = WSAGetLastError();
#endif
	err = SSL_get_error( p->session, ret );
	if (err == SSL_ERROR_WANT_WRITE ) {
		sbiod->sbiod_sb->sb_trans_needs_write = 1;
		sock_errset(EWOULDBLOCK);

	} else {
		sbiod->sbiod_sb->sb_trans_needs_write = 0;
	}
	return ret;
}

static Sockbuf_IO tlso_sbio =
{
	tlso_sb_setup,		/* sbi_setup */
	tlso_sb_remove,		/* sbi_remove */
	tlso_sb_ctrl,		/* sbi_ctrl */
	tlso_sb_read,		/* sbi_read */
	tlso_sb_write,		/* sbi_write */
	tlso_sb_close		/* sbi_close */
};

/* Derived from openssl/apps/s_cb.c */
static void
tlso_info_cb( const SSL *ssl, int where, int ret )
{
	int w;
	char *op;
	char *state = (char *) SSL_state_string_long( (SSL *)ssl );

	w = where & ~SSL_ST_MASK;
	if ( w & SSL_ST_CONNECT ) {
		op = "SSL_connect";
	} else if ( w & SSL_ST_ACCEPT ) {
		op = "SSL_accept";
	} else {
		op = "undefined";
	}

#ifdef HAVE_EBCDIC
	if ( state ) {
		state = LDAP_STRDUP( state );
		__etoa( state );
	}
#endif
	if ( where & SSL_CB_LOOP ) {
		Debug( LDAP_DEBUG_TRACE,
			   "TLS trace: %s:%s\n",
			   op, state, 0 );

	} else if ( where & SSL_CB_ALERT ) {
		char *atype = (char *) SSL_alert_type_string_long( ret );
		char *adesc = (char *) SSL_alert_desc_string_long( ret );
		op = ( where & SSL_CB_READ ) ? "read" : "write";
#ifdef HAVE_EBCDIC
		if ( atype ) {
			atype = LDAP_STRDUP( atype );
			__etoa( atype );
		}
		if ( adesc ) {
			adesc = LDAP_STRDUP( adesc );
			__etoa( adesc );
		}
#endif
		Debug( LDAP_DEBUG_TRACE,
			   "TLS trace: SSL3 alert %s:%s:%s\n",
			   op, atype, adesc );
#ifdef HAVE_EBCDIC
		if ( atype ) LDAP_FREE( atype );
		if ( adesc ) LDAP_FREE( adesc );
#endif
	} else if ( where & SSL_CB_EXIT ) {
		if ( ret == 0 ) {
			Debug( LDAP_DEBUG_TRACE,
				   "TLS trace: %s:failed in %s\n",
				   op, state, 0 );
		} else if ( ret < 0 ) {
			Debug( LDAP_DEBUG_TRACE,
				   "TLS trace: %s:error in %s\n",
				   op, state, 0 );
		}
	}
#ifdef HAVE_EBCDIC
	if ( state ) LDAP_FREE( state );
#endif
}

static int
tlso_verify_cb( int ok, X509_STORE_CTX *ctx )
{
	X509 *cert;
	int errnum;
	int errdepth;
	X509_NAME *subject;
	X509_NAME *issuer;
	char *sname;
	char *iname;
	char *certerr = NULL;

	cert = X509_STORE_CTX_get_current_cert( ctx );
	errnum = X509_STORE_CTX_get_error( ctx );
	errdepth = X509_STORE_CTX_get_error_depth( ctx );

	/*
	 * X509_get_*_name return pointers to the internal copies of
	 * those things requested.  So do not free them.
	 */
	subject = X509_get_subject_name( cert );
	issuer = X509_get_issuer_name( cert );
	/* X509_NAME_oneline, if passed a NULL buf, allocate memomry */
	sname = X509_NAME_oneline( subject, NULL, 0 );
	iname = X509_NAME_oneline( issuer, NULL, 0 );
	if ( !ok ) certerr = (char *)X509_verify_cert_error_string( errnum );
#ifdef HAVE_EBCDIC
	if ( sname ) __etoa( sname );
	if ( iname ) __etoa( iname );
	if ( certerr ) {
		certerr = LDAP_STRDUP( certerr );
		__etoa( certerr );
	}
#endif
	Debug( LDAP_DEBUG_TRACE,
		   "TLS certificate verification: depth: %d, err: %d, subject: %s,",
		   errdepth, errnum,
		   sname ? sname : "-unknown-" );
	Debug( LDAP_DEBUG_TRACE, " issuer: %s\n", iname ? iname : "-unknown-", 0, 0 );
	if ( !ok ) {
		Debug( LDAP_DEBUG_ANY,
			"TLS certificate verification: Error, %s\n",
			certerr, 0, 0 );
	}
	if ( sname )
		CRYPTO_free ( sname );
	if ( iname )
		CRYPTO_free ( iname );
#ifdef HAVE_EBCDIC
	if ( certerr ) LDAP_FREE( certerr );
#endif
	return ok;
}

static int
tlso_verify_ok( int ok, X509_STORE_CTX *ctx )
{
	(void) tlso_verify_cb( ok, ctx );
	return 1;
}

/* Inspired by ERR_print_errors in OpenSSL */
static void
tlso_report_error( void )
{
	unsigned long l;
	char buf[200];
	const char *file;
	int line;

	while ( ( l = ERR_get_error_line( &file, &line ) ) != 0 ) {
		ERR_error_string_n( l, buf, sizeof( buf ) );
#ifdef HAVE_EBCDIC
		if ( file ) {
			file = LDAP_STRDUP( file );
			__etoa( (char *)file );
		}
		__etoa( buf );
#endif
		Debug( LDAP_DEBUG_ANY, "TLS: %s %s:%d\n",
			buf, file, line );
#ifdef HAVE_EBCDIC
		if ( file ) LDAP_FREE( (void *)file );
#endif
	}
}

static RSA *
tlso_tmp_rsa_cb( SSL *ssl, int is_export, int key_length )
{
	RSA *tmp_rsa;
	/* FIXME:  Pregenerate the key on startup */
	/* FIXME:  Who frees the key? */
#if OPENSSL_VERSION_NUMBER >= 0x00908000
	BIGNUM *bn = BN_new();
	tmp_rsa = NULL;
	if ( bn ) {
		if ( BN_set_word( bn, RSA_F4 )) {
			tmp_rsa = RSA_new();
			if ( tmp_rsa && !RSA_generate_key_ex( tmp_rsa, key_length, bn, NULL )) {
				RSA_free( tmp_rsa );
				tmp_rsa = NULL;
			}
		}
		BN_free( bn );
	}
#else
	tmp_rsa = RSA_generate_key( key_length, RSA_F4, NULL, NULL );
#endif

	if ( !tmp_rsa ) {
		Debug( LDAP_DEBUG_ANY,
			"TLS: Failed to generate temporary %d-bit %s RSA key\n",
			key_length, is_export ? "export" : "domestic", 0 );
	}
	return tmp_rsa;
}

static int
tlso_seed_PRNG( const char *randfile )
{
#ifndef URANDOM_DEVICE
	/* no /dev/urandom (or equiv) */
	long total=0;
	char buffer[MAXPATHLEN];

	if (randfile == NULL) {
		/* The seed file is $RANDFILE if defined, otherwise $HOME/.rnd.
		 * If $HOME is not set or buffer too small to hold the pathname,
		 * an error occurs.	- From RAND_file_name() man page.
		 * The fact is that when $HOME is NULL, .rnd is used.
		 */
		randfile = RAND_file_name( buffer, sizeof( buffer ) );

	} else if (RAND_egd(randfile) > 0) {
		/* EGD socket */
		return 0;
	}

	if (randfile == NULL) {
		Debug( LDAP_DEBUG_ANY,
			"TLS: Use configuration file or $RANDFILE to define seed PRNG\n",
			0, 0, 0);
		return -1;
	}

	total = RAND_load_file(randfile, -1);

	if (RAND_status() == 0) {
		Debug( LDAP_DEBUG_ANY,
			"TLS: PRNG not been seeded with enough data\n",
			0, 0, 0);
		return -1;
	}

	/* assume if there was enough bits to seed that it's okay
	 * to write derived bits to the file
	 */
	RAND_write_file(randfile);

#endif

	return 0;
}


tls_impl ldap_int_tls_impl = {
	"OpenSSL",

	tlso_init,
	tlso_destroy,

	tlso_ctx_new,
	tlso_ctx_ref,
	tlso_ctx_free,
	tlso_ctx_init,

	tlso_session_new,
	tlso_session_connect,
	tlso_session_accept,
	tlso_session_upflags,
	tlso_session_errmsg,
	tlso_session_my_dn,
	tlso_session_peer_dn,
	tlso_session_chkhost,
	tlso_session_strength,
	tlso_session_unique,
	tlso_session_version,
	tlso_session_cipher,
	tlso_session_peercert,

	&tlso_sbio,

#ifdef LDAP_R_COMPILE
	tlso_thr_init,
#else
	NULL,
#endif

	0
};

#endif /* HAVE_OPENSSL */
