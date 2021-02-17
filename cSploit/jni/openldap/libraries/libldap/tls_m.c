/* tls_m.c - Handle tls/ssl using Mozilla NSS. */
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
/* ACKNOWLEDGEMENTS: Initial version written by Howard Chu. 
 * Additional support by Rich Megginson.
 */

#include "portable.h"

#ifdef HAVE_MOZNSS

#include "ldap_config.h"

#include <stdio.h>

#if defined( HAVE_FCNTL_H )
#include <fcntl.h>
#endif

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

#define READ_PASSWORD_FROM_STDIN
#define READ_PASSWORD_FROM_FILE

#ifdef READ_PASSWORD_FROM_STDIN
#include <termios.h> /* for echo on/off */
#endif

#include <nspr/nspr.h>
#include <nspr/private/pprio.h>
#include <nss/nss.h>
#include <nss/ssl.h>
#include <nss/sslerr.h>
#include <nss/sslproto.h>
#include <nss/pk11pub.h>
#include <nss/secerr.h>
#include <nss/keyhi.h>
#include <nss/secmod.h>
#include <nss/cert.h>

#undef NSS_VERSION_INT
#define	NSS_VERSION_INT	((NSS_VMAJOR << 24) | (NSS_VMINOR << 16) | \
	(NSS_VPATCH << 8) | NSS_VBUILD)

/* NSS 3.12.5 and later have NSS_InitContext */
#if NSS_VERSION_INT >= 0x030c0500
#define HAVE_NSS_INITCONTEXT 1
#endif

/* NSS 3.12.9 and later have SECMOD_RestartModules */
#if NSS_VERSION_INT >= 0x030c0900
#define HAVE_SECMOD_RESTARTMODULES 1
#endif

/* InitContext does not currently work in server mode */
/* #define INITCONTEXT_HACK 1 */

typedef struct tlsm_ctx {
	PRFileDesc *tc_model;
	int tc_refcnt;
	int tc_unique; /* unique number associated with this ctx */
	PRBool tc_verify_cert;
	CERTCertDBHandle *tc_certdb;
	PK11SlotInfo *tc_certdb_slot;
	CERTCertificate *tc_certificate;
	SECKEYPrivateKey *tc_private_key;
	char *tc_pin_file;
	struct ldaptls *tc_config;
	int tc_is_server;
	int tc_require_cert;
	PRCallOnceType tc_callonce;
	PRBool tc_using_pem;
#ifdef HAVE_NSS_INITCONTEXT
	NSSInitContext *tc_initctx; /* the NSS context */
#endif
	PK11GenericObject **tc_pem_objs; /* array of objects to free */
	int tc_n_pem_objs; /* number of objects */
	PRBool tc_warn_only; /* only warn of errors in validation */
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_t tc_refmutex;
#endif
} tlsm_ctx;

typedef PRFileDesc tlsm_session;

static int tlsm_ctx_count;
#define TLSM_CERTDB_DESC_FMT "ldap(%d)"

static PRDescIdentity	tlsm_layer_id;

static const PRIOMethods tlsm_PR_methods;

#define CERTDB_NONE NULL
#define PREFIX_NONE NULL

#define PEM_LIBRARY	"nsspem"
#define PEM_MODULE	"PEM"
/* hash files for use with cacertdir have this file name suffix */
#define PEM_CA_HASH_FILE_SUFFIX	".0"
#define PEM_CA_HASH_FILE_SUFFIX_LEN 2

static SECMODModule *pem_module;

#define DEFAULT_TOKEN_NAME "default"
#define TLSM_PEM_SLOT_CACERTS "PEM Token #0"
#define TLSM_PEM_SLOT_CERTS "PEM Token #1"

#define PK11_SETATTRS(x,id,v,l) (x).type = (id); \
                (x).pValue=(v); (x).ulValueLen = (l);

/* forward declaration */
static int tlsm_init( void );

#ifdef LDAP_R_COMPILE

/* it doesn't seem guaranteed that a client will call
   tlsm_thr_init in a non-threaded context - so we have
   to wrap the mutex creation in a prcallonce
*/
static ldap_pvt_thread_mutex_t tlsm_ctx_count_mutex;
static ldap_pvt_thread_mutex_t tlsm_init_mutex;
static ldap_pvt_thread_mutex_t tlsm_pem_mutex;
static PRCallOnceType tlsm_init_mutex_callonce = {0,0};

static PRStatus PR_CALLBACK
tlsm_thr_init_callonce( void )
{
	if ( ldap_pvt_thread_mutex_init( &tlsm_ctx_count_mutex ) ) {
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not create mutex for context counter: %d\n", errno, 0, 0 );
		return PR_FAILURE;
	}

	if ( ldap_pvt_thread_mutex_init( &tlsm_init_mutex ) ) {
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not create mutex for moznss initialization: %d\n", errno, 0, 0 );
		return PR_FAILURE;
	}

	if ( ldap_pvt_thread_mutex_init( &tlsm_pem_mutex ) ) {
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not create mutex for PEM module: %d\n", errno, 0, 0 );
		return PR_FAILURE;
	}

	return PR_SUCCESS;
}

static void
tlsm_thr_init( void )
{
    ( void )PR_CallOnce( &tlsm_init_mutex_callonce, tlsm_thr_init_callonce );
}

#endif /* LDAP_R_COMPILE */

static const char *
tlsm_dump_cipher_info(PRFileDesc *fd)
{
	PRUint16 ii;

	for (ii = 0; ii < SSL_NumImplementedCiphers; ++ii) {
		PRInt32 cipher = (PRInt32)SSL_ImplementedCiphers[ii];
		PRBool enabled = PR_FALSE;
		PRInt32 policy = 0;
		SSLCipherSuiteInfo info;

		if (fd) {
			SSL_CipherPrefGet(fd, cipher, &enabled);
		} else {
			SSL_CipherPrefGetDefault(cipher, &enabled);
		}
		SSL_CipherPolicyGet(cipher, &policy);
		SSL_GetCipherSuiteInfo(cipher, &info, (PRUintn)sizeof(info));
		Debug( LDAP_DEBUG_TRACE,
			   "TLS: cipher: %d - %s, enabled: %d, ",
			   info.cipherSuite, info.cipherSuiteName, enabled );
		Debug( LDAP_DEBUG_TRACE,
			   "policy: %d\n", policy, 0, 0 );
	}

	return "";
}

/* Cipher definitions */
typedef struct {
	char *ossl_name;    /* The OpenSSL cipher name */
	int num;            /* The cipher id */
	int attr;           /* cipher attributes: algorithms, etc */
	int version;        /* protocol version valid for this cipher */
	int bits;           /* bits of strength */
	int alg_bits;       /* bits of the algorithm */
	int strength;       /* LOW, MEDIUM, HIGH */
	int enabled;        /* Enabled by default? */
} cipher_properties;

/* cipher attributes  */
#define SSL_kRSA  0x00000001L
#define SSL_aRSA  0x00000002L
#define SSL_aDSS  0x00000004L
#define SSL_DSS   SSL_aDSS
#define SSL_eNULL 0x00000008L
#define SSL_DES   0x00000010L
#define SSL_3DES  0x00000020L
#define SSL_RC4   0x00000040L
#define SSL_RC2   0x00000080L
#define SSL_AES   0x00000100L
#define SSL_MD5   0x00000200L
#define SSL_SHA1  0x00000400L
#define SSL_SHA   SSL_SHA1
#define SSL_RSA   (SSL_kRSA|SSL_aRSA)

/* cipher strength */
#define SSL_NULL      0x00000001L
#define SSL_EXPORT40  0x00000002L
#define SSL_EXPORT56  0x00000004L
#define SSL_LOW       0x00000008L
#define SSL_MEDIUM    0x00000010L
#define SSL_HIGH      0x00000020L

#define SSL2  0x00000001L
#define SSL3  0x00000002L
/* OpenSSL treats SSL3 and TLSv1 the same */
#define TLS1  SSL3

/* Cipher translation */
static cipher_properties ciphers_def[] = {
	/* SSL 2 ciphers */
	{"DES-CBC3-MD5", SSL_EN_DES_192_EDE3_CBC_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_3DES|SSL_MD5, SSL2, 168, 168, SSL_HIGH, SSL_ALLOWED},
	{"RC2-CBC-MD5", SSL_EN_RC2_128_CBC_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_RC2|SSL_MD5, SSL2, 128, 128, SSL_MEDIUM, SSL_ALLOWED},
	{"RC4-MD5", SSL_EN_RC4_128_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_MD5, SSL2, 128, 128, SSL_MEDIUM, SSL_ALLOWED},
	{"DES-CBC-MD5", SSL_EN_DES_64_CBC_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_DES|SSL_MD5, SSL2, 56, 56, SSL_LOW, SSL_ALLOWED},
	{"EXP-RC2-CBC-MD5", SSL_EN_RC2_128_CBC_EXPORT40_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_RC2|SSL_MD5, SSL2, 40, 128, SSL_EXPORT40, SSL_ALLOWED},
	{"EXP-RC4-MD5", SSL_EN_RC4_128_EXPORT40_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_MD5, SSL2, 40, 128, SSL_EXPORT40, SSL_ALLOWED},

	/* SSL3 ciphers */
	{"RC4-MD5", SSL_RSA_WITH_RC4_128_MD5, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_MD5, SSL3, 128, 128, SSL_MEDIUM, SSL_ALLOWED},
	{"RC4-SHA", SSL_RSA_WITH_RC4_128_SHA, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_SHA1, SSL3, 128, 128, SSL_MEDIUM, SSL_ALLOWED},
	{"DES-CBC3-SHA", SSL_RSA_WITH_3DES_EDE_CBC_SHA, SSL_kRSA|SSL_aRSA|SSL_3DES|SSL_SHA1, SSL3, 168, 168, SSL_HIGH, SSL_ALLOWED},
	{"DES-CBC-SHA", SSL_RSA_WITH_DES_CBC_SHA, SSL_kRSA|SSL_aRSA|SSL_DES|SSL_SHA1, SSL3, 56, 56, SSL_LOW, SSL_ALLOWED},
	{"EXP-RC4-MD5", SSL_RSA_EXPORT_WITH_RC4_40_MD5, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_MD5, SSL3, 40, 128, SSL_EXPORT40, SSL_ALLOWED},
	{"EXP-RC2-CBC-MD5", SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5, SSL_kRSA|SSL_aRSA|SSL_RC2|SSL_MD5, SSL3, 0, 0, SSL_EXPORT40, SSL_ALLOWED},
	{"NULL-MD5", SSL_RSA_WITH_NULL_MD5, SSL_kRSA|SSL_aRSA|SSL_eNULL|SSL_MD5, SSL3, 0, 0, SSL_NULL, SSL_NOT_ALLOWED},
	{"NULL-SHA", SSL_RSA_WITH_NULL_SHA, SSL_kRSA|SSL_aRSA|SSL_eNULL|SSL_SHA1, SSL3, 0, 0, SSL_NULL, SSL_NOT_ALLOWED},

	/* TLSv1 ciphers */
	{"EXP1024-DES-CBC-SHA", TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA, SSL_kRSA|SSL_aRSA|SSL_DES|SSL_SHA, TLS1, 56, 56, SSL_EXPORT56, SSL_ALLOWED},
	{"EXP1024-RC4-SHA", TLS_RSA_EXPORT1024_WITH_RC4_56_SHA, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_SHA, TLS1, 56, 56, SSL_EXPORT56, SSL_ALLOWED},
	{"AES128-SHA", TLS_RSA_WITH_AES_128_CBC_SHA, SSL_kRSA|SSL_aRSA|SSL_AES|SSL_SHA, TLS1, 128, 128, SSL_HIGH, SSL_ALLOWED},
	{"AES256-SHA", TLS_RSA_WITH_AES_256_CBC_SHA, SSL_kRSA|SSL_aRSA|SSL_AES|SSL_SHA, TLS1, 256, 256, SSL_HIGH, SSL_ALLOWED},
};

#define ciphernum (sizeof(ciphers_def)/sizeof(cipher_properties))

/* given err which is the current errno, calls PR_SetError with
   the corresponding NSPR error code */
static void 
tlsm_map_error(int err)
{
	PRErrorCode prError;

	switch ( err ) {
	case EACCES:
		prError = PR_NO_ACCESS_RIGHTS_ERROR;
		break;
	case EADDRINUSE:
		prError = PR_ADDRESS_IN_USE_ERROR;
		break;
	case EADDRNOTAVAIL:
		prError = PR_ADDRESS_NOT_AVAILABLE_ERROR;
		break;
	case EAFNOSUPPORT:
		prError = PR_ADDRESS_NOT_SUPPORTED_ERROR;
		break;
	case EAGAIN:
		prError = PR_WOULD_BLOCK_ERROR;
		break;
	/*
	 * On QNX and Neutrino, EALREADY is defined as EBUSY.
	 */
#if EALREADY != EBUSY
	case EALREADY:
		prError = PR_ALREADY_INITIATED_ERROR;
		break;
#endif
	case EBADF:
		prError = PR_BAD_DESCRIPTOR_ERROR;
		break;
#ifdef EBADMSG
	case EBADMSG:
		prError = PR_IO_ERROR;
		break;
#endif
	case EBUSY:
		prError = PR_FILESYSTEM_MOUNTED_ERROR;
		break;
	case ECONNABORTED:
		prError = PR_CONNECT_ABORTED_ERROR;
		break;
	case ECONNREFUSED:
		prError = PR_CONNECT_REFUSED_ERROR;
		break;
	case ECONNRESET:
		prError = PR_CONNECT_RESET_ERROR;
		break;
	case EDEADLK:
		prError = PR_DEADLOCK_ERROR;
		break;
#ifdef EDIRCORRUPTED
	case EDIRCORRUPTED:
		prError = PR_DIRECTORY_CORRUPTED_ERROR;
		break;
#endif
#ifdef EDQUOT
	case EDQUOT:
		prError = PR_NO_DEVICE_SPACE_ERROR;
		break;
#endif
	case EEXIST:
		prError = PR_FILE_EXISTS_ERROR;
		break;
	case EFAULT:
		prError = PR_ACCESS_FAULT_ERROR;
		break;
	case EFBIG:
		prError = PR_FILE_TOO_BIG_ERROR;
		break;
	case EHOSTUNREACH:
		prError = PR_HOST_UNREACHABLE_ERROR;
		break;
	case EINPROGRESS:
		prError = PR_IN_PROGRESS_ERROR;
		break;
	case EINTR:
		prError = PR_PENDING_INTERRUPT_ERROR;
		break;
	case EINVAL:
		prError = PR_INVALID_ARGUMENT_ERROR;
		break;
	case EIO:
		prError = PR_IO_ERROR;
		break;
	case EISCONN:
		prError = PR_IS_CONNECTED_ERROR;
		break;
	case EISDIR:
		prError = PR_IS_DIRECTORY_ERROR;
		break;
	case ELOOP:
		prError = PR_LOOP_ERROR;
		break;
	case EMFILE:
		prError = PR_PROC_DESC_TABLE_FULL_ERROR;
		break;
	case EMLINK:
		prError = PR_MAX_DIRECTORY_ENTRIES_ERROR;
		break;
	case EMSGSIZE:
		prError = PR_INVALID_ARGUMENT_ERROR;
		break;
#ifdef EMULTIHOP
	case EMULTIHOP:
		prError = PR_REMOTE_FILE_ERROR;
		break;
#endif
	case ENAMETOOLONG:
		prError = PR_NAME_TOO_LONG_ERROR;
		break;
	case ENETUNREACH:
		prError = PR_NETWORK_UNREACHABLE_ERROR;
		break;
	case ENFILE:
		prError = PR_SYS_DESC_TABLE_FULL_ERROR;
		break;
	/*
	 * On SCO OpenServer 5, ENOBUFS is defined as ENOSR.
	 */
#if defined(ENOBUFS) && (ENOBUFS != ENOSR)
	case ENOBUFS:
		prError = PR_INSUFFICIENT_RESOURCES_ERROR;
		break;
#endif
	case ENODEV:
		prError = PR_FILE_NOT_FOUND_ERROR;
		break;
	case ENOENT:
		prError = PR_FILE_NOT_FOUND_ERROR;
		break;
	case ENOLCK:
		prError = PR_FILE_IS_LOCKED_ERROR;
		break;
#ifdef ENOLINK 
	case ENOLINK:
		prError = PR_REMOTE_FILE_ERROR;
		break;
#endif
	case ENOMEM:
		prError = PR_OUT_OF_MEMORY_ERROR;
		break;
	case ENOPROTOOPT:
		prError = PR_INVALID_ARGUMENT_ERROR;
		break;
	case ENOSPC:
		prError = PR_NO_DEVICE_SPACE_ERROR;
		break;
#ifdef ENOSR
	case ENOSR:
		prError = PR_INSUFFICIENT_RESOURCES_ERROR;
		break;
#endif
	case ENOTCONN:
		prError = PR_NOT_CONNECTED_ERROR;
		break;
	case ENOTDIR:
		prError = PR_NOT_DIRECTORY_ERROR;
		break;
	case ENOTSOCK:
		prError = PR_NOT_SOCKET_ERROR;
		break;
	case ENXIO:
		prError = PR_FILE_NOT_FOUND_ERROR;
		break;
	case EOPNOTSUPP:
		prError = PR_NOT_TCP_SOCKET_ERROR;
		break;
#ifdef EOVERFLOW
	case EOVERFLOW:
		prError = PR_BUFFER_OVERFLOW_ERROR;
		break;
#endif
	case EPERM:
		prError = PR_NO_ACCESS_RIGHTS_ERROR;
		break;
	case EPIPE:
		prError = PR_CONNECT_RESET_ERROR;
		break;
#ifdef EPROTO
	case EPROTO:
		prError = PR_IO_ERROR;
		break;
#endif
	case EPROTONOSUPPORT:
		prError = PR_PROTOCOL_NOT_SUPPORTED_ERROR;
		break;
	case EPROTOTYPE:
		prError = PR_ADDRESS_NOT_SUPPORTED_ERROR;
		break;
	case ERANGE:
		prError = PR_INVALID_METHOD_ERROR;
		break;
	case EROFS:
		prError = PR_READ_ONLY_FILESYSTEM_ERROR;
		break;
	case ESPIPE:
		prError = PR_INVALID_METHOD_ERROR;
		break;
	case ETIMEDOUT:
		prError = PR_IO_TIMEOUT_ERROR;
		break;
#if EWOULDBLOCK != EAGAIN
	case EWOULDBLOCK:
		prError = PR_WOULD_BLOCK_ERROR;
		break;
#endif
	case EXDEV:
		prError = PR_NOT_SAME_DEVICE_ERROR;
		break;
	default:
		prError = PR_UNKNOWN_ERROR;
		break;
	}
	PR_SetError( prError, err );
}

/*
 * cipher_list is an integer array with the following values:
 *   -1: never enable this cipher
 *    0: cipher disabled
 *    1: cipher enabled
 */
static int
nss_parse_ciphers(const char *cipherstr, int cipher_list[ciphernum])
{
	int i;
	char *cipher;
	char *ciphers;
	char *ciphertip;
	int action;
	int rv;

	/* All disabled to start */
	for (i=0; i<ciphernum; i++)
		cipher_list[i] = 0;

	ciphertip = strdup(cipherstr);
	cipher = ciphers = ciphertip;

	while (ciphers && (strlen(ciphers))) {
		while ((*cipher) && (isspace(*cipher)))
			++cipher;

		action = 1;
		switch(*cipher) {
		case '+': /* Add something */
			action = 1;
			cipher++;
			break;
		case '-': /* Subtract something */
			action = 0;
			cipher++;
			break;
		case '!':  /* Disable something */
			action = -1;
			cipher++;
			break;
		default:
			/* do nothing */
			break;
		}

		if ((ciphers = strchr(cipher, ':'))) {
			*ciphers++ = '\0';
		}

		/* Do the easy one first */
		if (!strcmp(cipher, "ALL")) {
			for (i=0; i<ciphernum; i++) {
				if (!(ciphers_def[i].attr & SSL_eNULL))
					cipher_list[i] = action;
			}
		} else if (!strcmp(cipher, "COMPLEMENTOFALL")) {
			for (i=0; i<ciphernum; i++) {
				if ((ciphers_def[i].attr & SSL_eNULL))
					cipher_list[i] = action;
			}
		} else if (!strcmp(cipher, "DEFAULT")) {
			for (i=0; i<ciphernum; i++) {
				cipher_list[i] = ciphers_def[i].enabled == SSL_ALLOWED ? 1 : 0;
			}
		} else {
			int mask = 0;
			int strength = 0;
			int protocol = 0;
			char *c;

			c = cipher;
			while (c && (strlen(c))) {

				if ((c = strchr(cipher, '+'))) {
					*c++ = '\0';
				}

				if (!strcmp(cipher, "RSA")) {
					mask |= SSL_RSA;
				} else if ((!strcmp(cipher, "NULL")) || (!strcmp(cipher, "eNULL"))) {
					mask |= SSL_eNULL;
				} else if (!strcmp(cipher, "AES")) {
					mask |= SSL_AES;
				} else if (!strcmp(cipher, "3DES")) {
					mask |= SSL_3DES;
				} else if (!strcmp(cipher, "DES")) {
					mask |= SSL_DES;
				} else if (!strcmp(cipher, "RC4")) {
					mask |= SSL_RC4;
				} else if (!strcmp(cipher, "RC2")) {
					mask |= SSL_RC2;
				} else if (!strcmp(cipher, "MD5")) {
					mask |= SSL_MD5;
				} else if ((!strcmp(cipher, "SHA")) || (!strcmp(cipher, "SHA1"))) {
					mask |= SSL_SHA1;
				} else if (!strcmp(cipher, "SSLv2")) {
					protocol |= SSL2;
				} else if (!strcmp(cipher, "SSLv3")) {
					protocol |= SSL3;
				} else if (!strcmp(cipher, "TLSv1")) {
					protocol |= TLS1;
				} else if (!strcmp(cipher, "HIGH")) {
					strength |= SSL_HIGH;
				} else if (!strcmp(cipher, "MEDIUM")) {
					strength |= SSL_MEDIUM;
				} else if (!strcmp(cipher, "LOW")) {
					strength |= SSL_LOW;
				} else if ((!strcmp(cipher, "EXPORT")) || (!strcmp(cipher, "EXP"))) {
					strength |= SSL_EXPORT40|SSL_EXPORT56;
				} else if (!strcmp(cipher, "EXPORT40")) {
					strength |= SSL_EXPORT40;
				} else if (!strcmp(cipher, "EXPORT56")) {
					strength |= SSL_EXPORT56;
				}

				if (c)
					cipher = c;

			} /* while */

			/* If we have a mask, apply it. If not then perhaps they provided
			 * a specific cipher to enable.
			 */
			if (mask || strength || protocol) {
				for (i=0; i<ciphernum; i++) {
					if (((ciphers_def[i].attr & mask) ||
						 (ciphers_def[i].strength & strength) ||
						 (ciphers_def[i].version & protocol)) &&
						(cipher_list[i] != -1)) {
						/* Enable the NULL ciphers only if explicity
						 * requested */
						if (ciphers_def[i].attr & SSL_eNULL) {
							if (mask & SSL_eNULL)
								cipher_list[i] = action;
						} else
							cipher_list[i] = action;
					}
				}
			} else {
				for (i=0; i<ciphernum; i++) {
					if (!strcmp(ciphers_def[i].ossl_name, cipher) &&
						cipher_list[i] != -1)
						cipher_list[i] = action;
				}
			}
		}

		if (ciphers)
			cipher = ciphers;
	}

	/* See if any ciphers were enabled */
	rv = 0;
	for (i=0; i<ciphernum; i++) {
		if (cipher_list[i] == 1)
			rv = 1;
	}

	free(ciphertip);

	return rv;
}

static int
tlsm_parse_ciphers(tlsm_ctx *ctx, const char *str)
{
	int cipher_state[ciphernum];
	int rv, i;

	if (!ctx)
		return 0;

	rv = nss_parse_ciphers(str, cipher_state);

	if (rv) {
		/* First disable everything */
		for (i = 0; i < SSL_NumImplementedCiphers; i++)
			SSL_CipherPrefSet(ctx->tc_model, SSL_ImplementedCiphers[i], SSL_NOT_ALLOWED);

		/* Now enable what was requested */
		for (i=0; i<ciphernum; i++) {
			SSLCipherSuiteInfo suite;
			PRBool enabled;

			if (SSL_GetCipherSuiteInfo(ciphers_def[i].num, &suite, sizeof suite)
				== SECSuccess) {
				enabled = cipher_state[i] < 0 ? 0 : cipher_state[i];
				if (enabled == SSL_ALLOWED) {
					if (PK11_IsFIPS() && !suite.isFIPS)    
						enabled = SSL_NOT_ALLOWED;
				}
				SSL_CipherPrefSet(ctx->tc_model, ciphers_def[i].num, enabled);
			}
		}
	}

	return rv == 1 ? 0 : -1;
}

static SECStatus
tlsm_bad_cert_handler(void *arg, PRFileDesc *ssl)
{
	SECStatus success = SECSuccess;
	PRErrorCode err;
	tlsm_ctx *ctx = (tlsm_ctx *)arg;

	if (!ssl || !ctx) {
		return SECFailure;
	}

	err = PORT_GetError();

	switch (err) {
	case SEC_ERROR_UNTRUSTED_ISSUER:
	case SEC_ERROR_UNKNOWN_ISSUER:
	case SEC_ERROR_EXPIRED_CERTIFICATE:
	case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
		if (ctx->tc_verify_cert) {
			success = SECFailure;
		}
		break;
	/* we bypass NSS's hostname checks and do our own */
	case SSL_ERROR_BAD_CERT_DOMAIN:
		break;
	default:
		success = SECFailure;
		break;
	}

	return success;
}

static const char *
tlsm_dump_security_status(PRFileDesc *fd)
{
	char * cp;	/* bulk cipher name */
	char * ip;	/* cert issuer DN */
	char * sp;	/* cert subject DN */
	int    op;	/* High, Low, Off */
	int    kp0;	/* total key bits */
	int    kp1;	/* secret key bits */
	SSL3Statistics * ssl3stats = SSL_GetStatistics();

	SSL_SecurityStatus( fd, &op, &cp, &kp0, &kp1, &ip, &sp );
	Debug( LDAP_DEBUG_TRACE,
		   "TLS certificate verification: subject: %s, issuer: %s, cipher: %s, ",
		   sp ? sp : "-unknown-", ip ? ip : "-unknown-", cp ? cp : "-unknown-" );
	PR_Free(cp);
	PR_Free(ip);
	PR_Free(sp);
	Debug( LDAP_DEBUG_TRACE,
		   "security level: %s, secret key bits: %d, total key bits: %d, ",
		   ((op == SSL_SECURITY_STATUS_ON_HIGH) ? "high" :
			((op == SSL_SECURITY_STATUS_ON_LOW) ? "low" : "off")),
		   kp1, kp0 );

	Debug( LDAP_DEBUG_TRACE,
		   "cache hits: %ld, cache misses: %ld, cache not reusable: %ld\n",
		   ssl3stats->hch_sid_cache_hits, ssl3stats->hch_sid_cache_misses,
		   ssl3stats->hch_sid_cache_not_ok );

	return "";
}

static void
tlsm_handshake_complete_cb( PRFileDesc *fd, void *client_data )
{
	tlsm_dump_security_status( fd );
}

#ifdef READ_PASSWORD_FROM_FILE
static char *
tlsm_get_pin_from_file(const char *token_name, tlsm_ctx *ctx)
{
	char *pwdstr = NULL;
	char *contents = NULL;
	char *lasts = NULL;
	char *line = NULL;
	char *candidate = NULL;
	PRFileInfo file_info;
	PRFileDesc *pwd_fileptr = PR_Open( ctx->tc_pin_file, PR_RDONLY, 00400 );

	/* open the password file */
	if ( !pwd_fileptr ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not open security pin file %s - error %d:%s.\n",
		       ctx->tc_pin_file, errcode,
		       PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		goto done;
	}

	/* get the file size */
	if ( PR_SUCCESS != PR_GetFileInfo( ctx->tc_pin_file, &file_info ) ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not get file info from pin file %s - error %d:%s.\n",
		       ctx->tc_pin_file, errcode,
		       PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		goto done;
	}

	/* create a buffer to hold the file contents */
	if ( !( contents = PR_CALLOC( file_info.size + 1 ) ) ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not alloc a buffer for contents of pin file %s - error %d:%s.\n",
		       ctx->tc_pin_file, errcode,
		       PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		goto done;
	}

	/* read file into the buffer */
	if( PR_Read( pwd_fileptr, contents, file_info.size ) <= 0 ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not read the file contents from pin file %s - error %d:%s.\n",
		       ctx->tc_pin_file, errcode,
		       PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		goto done;
	}

	/* format is [tokenname:]password EOL [tokenname:]password EOL ... */
	/* if you want to use a password containing a colon character, use
	   the special tokenname "default" */
	for ( line = PL_strtok_r( contents, "\r\n", &lasts ); line;
	      line = PL_strtok_r( NULL, "\r\n", &lasts ) ) {
		char *colon;

		if ( !*line ) {
			continue; /* skip blank lines */
		}
		colon = PL_strchr( line, ':' );
		if ( colon ) {
			if ( *(colon + 1) && token_name &&
			     !PL_strncmp( token_name, line, colon-line ) ) {
				candidate = colon + 1; /* found a definite match */
				break;
			} else if ( !PL_strncmp( DEFAULT_TOKEN_NAME, line, colon-line ) ) {
				candidate = colon + 1; /* found possible match */
			}
		} else { /* no token name */
			candidate = line;
		}
	}
done:
	if ( pwd_fileptr ) {
		PR_Close( pwd_fileptr );
	}
	if ( candidate ) {
		pwdstr = PL_strdup( candidate );
	}
	PL_strfree( contents );

	return pwdstr;
}
#endif /* READ_PASSWORD_FROM_FILE */

#ifdef READ_PASSWORD_FROM_STDIN
/*
 * Turn the echoing off on a tty.
 */
static void
echoOff(int fd)
{
	if ( isatty( fd ) ) {
		struct termios tio;
		tcgetattr( fd, &tio );
		tio.c_lflag &= ~ECHO;
		tcsetattr( fd, TCSAFLUSH, &tio );
	}
}

/*
 * Turn the echoing on on a tty.
 */
static void
echoOn(int fd)
{
	if ( isatty( fd ) ) {
		struct termios tio;
		tcgetattr( fd, &tio );
		tio.c_lflag |= ECHO;
		tcsetattr( fd, TCSAFLUSH, &tio );
		tcsetattr( fd, TCSAFLUSH, &tio );
	}
}
#endif /* READ_PASSWORD_FROM_STDIN */

/*
 * This does the actual work of reading the pin/password/pass phrase
 */
static char *
tlsm_get_pin(PK11SlotInfo *slot, PRBool retry, tlsm_ctx *ctx)
{
	char *token_name = NULL;
	char *pwdstr = NULL;

	token_name = PK11_GetTokenName( slot );
#ifdef READ_PASSWORD_FROM_FILE
	/* Try to get the passwords from the password file if it exists.
	 * THIS IS UNSAFE and is provided for convenience only. Without this
	 * capability the server would have to be started in foreground mode
	 * if using an encrypted key.
	 */
	if ( ctx && ctx->tc_pin_file ) {
		pwdstr = tlsm_get_pin_from_file( token_name, ctx );
		if ( retry && pwdstr != NULL )
			return NULL;
	}
#endif /* RETRIEVE_PASSWORD_FROM_FILE */
#ifdef READ_PASSWORD_FROM_STDIN
	if ( !pwdstr ) {
		int infd = PR_FileDesc2NativeHandle( PR_STDIN );
		int isTTY = isatty( infd );
		unsigned char phrase[200];
		char *dummy;
		/* Prompt for password */
		if ( isTTY ) {
			fprintf( stdout,
				 "Please enter pin, password, or pass phrase for security token '%s': ",
				 token_name ? token_name : DEFAULT_TOKEN_NAME );
			echoOff( infd );
		}
		dummy = fgets( (char*)phrase, sizeof(phrase), stdin );
		(void) dummy;
		if ( isTTY ) {
			fprintf( stdout, "\n" );
			echoOn( infd );
		}
		/* stomp on newline */
		phrase[strlen((char*)phrase)-1] = 0;

		pwdstr = PL_strdup( (char*)phrase );
	}

#endif /* READ_PASSWORD_FROM_STDIN */
	return pwdstr;
}

/*
 * PKCS11 devices (including the internal softokn cert/key database)
 * may be protected by a pin or password or even pass phrase
 * MozNSS needs a way for the user to provide that
 */
static char *
tlsm_pin_prompt(PK11SlotInfo *slot, PRBool retry, void *arg)
{
	tlsm_ctx *ctx = (tlsm_ctx *)arg;

	return tlsm_get_pin( slot, retry, ctx );
}

static char *
tlsm_ctx_subject_name(tlsm_ctx *ctx)
{
	if ( !ctx || !ctx->tc_certificate )
		return "(unknown)";

	return ctx->tc_certificate->subjectName;
}

static SECStatus
tlsm_get_basic_constraint_extension( CERTCertificate *cert,
									 CERTBasicConstraints *cbcval )
{
	SECItem encodedVal = { 0, NULL };
	SECStatus rc;

	rc = CERT_FindCertExtension( cert, SEC_OID_X509_BASIC_CONSTRAINTS,
								 &encodedVal);
	if ( rc != SECSuccess ) {
		return rc;
	}

	rc = CERT_DecodeBasicConstraintValue( cbcval, &encodedVal );

	/* free the raw extension data */
	PORT_Free( encodedVal.data );

	return rc;
}

static PRBool
tlsm_cert_is_self_issued( CERTCertificate *cert )
{
	/* A cert is self-issued if its subject and issuer are equal and
	 * both are of non-zero length. 
	 */
	PRBool is_self_issued = cert &&
		(PRBool)SECITEM_ItemsAreEqual( &cert->derIssuer, 
									   &cert->derSubject ) &&
		cert->derSubject.len > 0;
	return is_self_issued;
}

/*
 * The private key for used certificate can be already unlocked by other
 * thread or library. Find the unlocked key if possible.
 */
static SECKEYPrivateKey *
tlsm_find_unlocked_key( tlsm_ctx *ctx, void *pin_arg )
{
	SECKEYPrivateKey *result = NULL;

	PK11SlotList *slots = PK11_GetAllSlotsForCert( ctx->tc_certificate, NULL );
	if ( !slots ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
				"TLS: cannot get all slots for certificate '%s' (error %d: %s)",
				tlsm_ctx_subject_name( ctx ), errcode,
				PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		return result;
	}

	PK11SlotListElement *le;
	for ( le = slots->head; le; le = le->next ) {
		PK11SlotInfo *slot = le->slot;
		if ( PK11_IsLoggedIn( slot, NULL ) ) {
			result = PK11_FindKeyByDERCert( slot, ctx->tc_certificate, pin_arg );
			break;
		}
	}

	PK11_FreeSlotList( slots );
	return result;
}

static SECStatus
tlsm_verify_cert(CERTCertDBHandle *handle, CERTCertificate *cert, void *pinarg,
				 PRBool checksig, SECCertificateUsage certUsage, PRBool warn_only,
				 PRBool ignore_issuer )
{
	CERTVerifyLog verifylog;
	SECStatus ret = SECSuccess;
	const char *name;
	int debug_level = LDAP_DEBUG_ANY;

	if ( warn_only ) {
		debug_level = LDAP_DEBUG_TRACE;
	}

	/* the log captures information about every cert in the chain, so we can tell
	   which cert caused the problem and what the problem was */
	memset( &verifylog, 0, sizeof( verifylog ) );
	verifylog.arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE );
	if ( verifylog.arena == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			   "TLS certificate verification: Out of memory for certificate verification logger\n",
			   0, 0, 0 );
		return SECFailure;
	}
	ret = CERT_VerifyCertificate( handle, cert, checksig, certUsage, PR_Now(), pinarg, &verifylog,
								  NULL );
	if ( ( name = cert->subjectName ) == NULL ) {
		name = cert->nickname;
	}
	if ( verifylog.head == NULL ) {
		/* it is possible for CERT_VerifyCertificate return with an error with no logging */
		if ( ret != SECSuccess ) {
			PRErrorCode errcode = PR_GetError();
			Debug( debug_level,
				   "TLS: certificate [%s] is not valid - error %d:%s.\n",
				   name ? name : "(unknown)",
				   errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		}
	} else {
		const char *name;
		CERTVerifyLogNode *node;

		ret = SECSuccess; /* reset */
		node = verifylog.head;
		while ( node ) {
			if ( ( name = node->cert->subjectName ) == NULL ) {
				name = node->cert->nickname;
			}
			if ( node->error ) {
				/* NSS does not like CA certs that have the basic constraints extension
				   with the CA flag set to FALSE - openssl doesn't check if the cert
				   is self issued */
				if ( ( node->error == SEC_ERROR_CA_CERT_INVALID ) &&
					 tlsm_cert_is_self_issued( node->cert ) ) {

					PRErrorCode orig_error = PR_GetError();
					PRInt32 orig_oserror = PR_GetOSError();

					CERTBasicConstraints basicConstraint;
					SECStatus rv = tlsm_get_basic_constraint_extension( node->cert, &basicConstraint );
					if ( ( rv == SECSuccess ) && ( basicConstraint.isCA == PR_FALSE ) ) {
						Debug( LDAP_DEBUG_TRACE,
							   "TLS: certificate [%s] is not correct because it is a CA cert and the "
							   "BasicConstraint CA flag is set to FALSE - allowing for now, but "
							   "please fix your certs if possible\n", name, 0, 0 );
					} else { /* does not have basicconstraint, or some other error */
						ret = SECFailure;
						Debug( debug_level,
							   "TLS: certificate [%s] is not valid - CA cert is not valid\n",
							   name, 0, 0 );
					}

					PR_SetError( orig_error, orig_oserror );

				} else if ( warn_only || ( ignore_issuer && (
					node->error == SEC_ERROR_UNKNOWN_ISSUER ||
					node->error == SEC_ERROR_UNTRUSTED_ISSUER )
				) ) {
					ret = SECSuccess;
					Debug( debug_level,
						   "TLS: Warning: ignoring error for certificate [%s] - error %ld:%s.\n",
						   name, node->error, PR_ErrorToString( node->error, PR_LANGUAGE_I_DEFAULT ) );
				} else {
					ret = SECFailure;
					Debug( debug_level,
						   "TLS: certificate [%s] is not valid - error %ld:%s.\n",
						   name, node->error, PR_ErrorToString( node->error, PR_LANGUAGE_I_DEFAULT ) );
				}
			}
			CERT_DestroyCertificate( node->cert );
			node = node->next;
		}
	}

	PORT_FreeArena( verifylog.arena, PR_FALSE );

	if ( ret == SECSuccess ) {
		Debug( LDAP_DEBUG_TRACE,
			   "TLS: certificate [%s] is valid\n", name, 0, 0 );
	}

	return ret;
}

static SECStatus
tlsm_auth_cert_handler(void *arg, PRFileDesc *fd,
                       PRBool checksig, PRBool isServer)
{
	SECCertificateUsage certUsage = isServer ? certificateUsageSSLClient : certificateUsageSSLServer;
	SECStatus ret = SECSuccess;
	CERTCertificate *peercert = SSL_PeerCertificate( fd );
	tlsm_ctx *ctx = (tlsm_ctx *)arg;

	ret = tlsm_verify_cert( ctx->tc_certdb, peercert,
							SSL_RevealPinArg( fd ),
							checksig, certUsage, ctx->tc_warn_only, PR_FALSE );
	CERT_DestroyCertificate( peercert );

	return ret;
}

static SECStatus
tlsm_nss_shutdown_cb( void *appData, void *nssData )
{
	SECStatus rc = SECSuccess;

	SSL_ShutdownServerSessionIDCache();

	if ( pem_module ) {
		SECMOD_UnloadUserModule( pem_module );
		SECMOD_DestroyModule( pem_module );
		pem_module = NULL;
	}
	return rc;
}

static PRCallOnceType tlsm_register_shutdown_callonce = {0,0};
static PRStatus PR_CALLBACK
tlsm_register_nss_shutdown_cb( void )
{
	if ( SECSuccess == NSS_RegisterShutdown( tlsm_nss_shutdown_cb,
											 NULL ) ) {
		return PR_SUCCESS;
	}
	return PR_FAILURE;
}

static PRStatus
tlsm_register_nss_shutdown( void )
{
	return PR_CallOnce( &tlsm_register_shutdown_callonce,
						tlsm_register_nss_shutdown_cb );
}

static int
tlsm_init_pem_module( void )
{
	int rc = 0;
	char *fullname = NULL;
	char *configstring = NULL;

	if ( pem_module ) {
		return rc;
	}

	/* not loaded - load it */
	/* get the system dependent library name */
	fullname = PR_GetLibraryName( NULL, PEM_LIBRARY );
	/* Load our PKCS#11 module */
	configstring = PR_smprintf( "library=%s name=" PEM_MODULE " parameters=\"\"", fullname );
	PL_strfree( fullname );

	pem_module = SECMOD_LoadUserModule( configstring, NULL, PR_FALSE );
	PR_smprintf_free( configstring );

	if ( !pem_module || !pem_module->loaded ) {
		if ( pem_module ) {
			SECMOD_DestroyModule( pem_module );
			pem_module = NULL;
		}
		rc = -1;
	}

	return rc;
}

static void
tlsm_add_pem_obj( tlsm_ctx *ctx, PK11GenericObject *obj )
{
	int idx = ctx->tc_n_pem_objs;
	ctx->tc_n_pem_objs++;
	ctx->tc_pem_objs = (PK11GenericObject **)
		PORT_Realloc( ctx->tc_pem_objs, ctx->tc_n_pem_objs * sizeof( PK11GenericObject * ) );
	ctx->tc_pem_objs[idx] = obj;														  
}

static void
tlsm_free_pem_objs( tlsm_ctx *ctx )
{
	/* free in reverse order of allocation */
	while ( ctx->tc_n_pem_objs-- ) {
		PK11_DestroyGenericObject( ctx->tc_pem_objs[ctx->tc_n_pem_objs] );
		ctx->tc_pem_objs[ctx->tc_n_pem_objs] = NULL;
	}
	PORT_Free(ctx->tc_pem_objs);
	ctx->tc_pem_objs = NULL;
	ctx->tc_n_pem_objs = 0;
}

static int
tlsm_add_cert_from_file( tlsm_ctx *ctx, const char *filename, PRBool isca )
{
	PK11SlotInfo *slot;
	PK11GenericObject *cert;
	CK_ATTRIBUTE attrs[4];
	CK_BBOOL cktrue = CK_TRUE;
	CK_BBOOL ckfalse = CK_FALSE;
	CK_OBJECT_CLASS objClass = CKO_CERTIFICATE;
	char *slotname;
	PRFileInfo fi;
	PRStatus status;
	SECItem certDER = { 0, NULL, 0 };

	memset( &fi, 0, sizeof(fi) );
	status = PR_GetFileInfo( filename, &fi );
	if ( PR_SUCCESS != status) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not read certificate file %s - error %d:%s.\n",
			   filename, errcode,
			   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		return -1;
	}

	if ( fi.type != PR_FILE_FILE ) {
		PR_SetError(PR_IS_DIRECTORY_ERROR, 0);
		Debug( LDAP_DEBUG_ANY,
			   "TLS: error: the certificate file %s is not a file.\n",
			   filename, 0 ,0 );
		return -1;
	}

	slotname = isca ? TLSM_PEM_SLOT_CACERTS : TLSM_PEM_SLOT_CERTS;
	slot = PK11_FindSlotByName( slotname );

	if ( !slot ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not find the slot for the certificate '%s' - error %d:%s.\n",
			   filename, errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		return -1;
	}

	PK11_SETATTRS( attrs[0], CKA_CLASS, &objClass, sizeof( objClass ) );
	PK11_SETATTRS( attrs[1], CKA_TOKEN, &cktrue, sizeof( CK_BBOOL ) );
	PK11_SETATTRS( attrs[2], CKA_LABEL, (unsigned char *) filename, strlen( filename ) + 1 );
	PK11_SETATTRS( attrs[3], CKA_TRUST, isca ? &cktrue : &ckfalse, sizeof( CK_BBOOL ) );

	cert = PK11_CreateGenericObject( slot, attrs, 4, PR_FALSE /* isPerm */ );

	if ( !cert ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not add the certificate '%s' - error %d:%s.\n",
			   filename, errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		PK11_FreeSlot( slot );
		return -1;
	}

	/* if not CA, we store the certificate in ctx->tc_certificate */
	if ( !isca ) {
		if ( PK11_ReadRawAttribute( PK11_TypeGeneric, cert, CKA_VALUE, &certDER ) != SECSuccess ) {
			PRErrorCode errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
					"TLS: could not get DER of the '%s' certificate - error %d:%s.\n",
					filename, errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
			PK11_DestroyGenericObject( cert );
			PK11_FreeSlot( slot );
			return -1;
		}

		ctx->tc_certificate = PK11_FindCertFromDERCertItem( slot, &certDER, NULL );
		SECITEM_FreeItem( &certDER, PR_FALSE );

		if ( !ctx->tc_certificate ) {
			PRErrorCode errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
					"TLS: could not get certificate '%s' using DER - error %d:%s.\n",
					filename, errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
			PK11_DestroyGenericObject( cert );
			PK11_FreeSlot( slot );
			return -1;
		}
	}

	tlsm_add_pem_obj( ctx, cert );

	PK11_FreeSlot( slot );

	return 0;
}

static int
tlsm_ctx_load_private_key( tlsm_ctx *ctx )
{
	if ( !ctx->tc_certificate )
		return -1;

	if ( ctx->tc_private_key )
		return 0;

	void *pin_arg = SSL_RevealPinArg( ctx->tc_model );

	SECKEYPrivateKey *unlocked_key = tlsm_find_unlocked_key( ctx, pin_arg );
	Debug( LDAP_DEBUG_ANY,
			"TLS: %s unlocked certificate for certificate '%s'.\n",
			unlocked_key ? "found" : "no", tlsm_ctx_subject_name( ctx ), 0 );

	/* prefer unlocked key, then key from opened certdb, then any other */
	if ( unlocked_key )
		ctx->tc_private_key = unlocked_key;
	else if ( ctx->tc_certdb_slot )
		ctx->tc_private_key = PK11_FindKeyByDERCert( ctx->tc_certdb_slot, ctx->tc_certificate, pin_arg );
	else
		ctx->tc_private_key = PK11_FindKeyByAnyCert( ctx->tc_certificate, pin_arg );

	if ( !ctx->tc_private_key ) {
		PRErrorCode errcode = PR_GetError();
		Debug(LDAP_DEBUG_ANY,
				"TLS: cannot find private key for certificate '%s' (error %d: %s)",
				tlsm_ctx_subject_name( ctx ), errcode,
				PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		return -1;
	}

	return 0;
}

static int
tlsm_add_key_from_file( tlsm_ctx *ctx, const char *filename )
{
	PK11SlotInfo * slot = NULL;
	PK11GenericObject *key;
	CK_ATTRIBUTE attrs[3];
	CK_BBOOL cktrue = CK_TRUE;
	CK_OBJECT_CLASS objClass = CKO_PRIVATE_KEY;
	int retcode = 0;
	PRFileInfo fi;
	PRStatus status;

	memset( &fi, 0, sizeof(fi) );
	status = PR_GetFileInfo( filename, &fi );
	if ( PR_SUCCESS != status) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not read key file %s - error %d:%s.\n",
			   filename, errcode,
			   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		return -1;
	}

	if ( fi.type != PR_FILE_FILE ) {
		PR_SetError(PR_IS_DIRECTORY_ERROR, 0);
		Debug( LDAP_DEBUG_ANY,
			   "TLS: error: the key file %s is not a file.\n",
			   filename, 0 ,0 );
		return -1;
	}

	slot = PK11_FindSlotByName( TLSM_PEM_SLOT_CERTS );

	if ( !slot ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not find the slot for the private key '%s' - error %d:%s.\n",
			   filename, errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		return -1;
	}

	PK11_SETATTRS( attrs[0], CKA_CLASS, &objClass, sizeof( objClass ) );
	PK11_SETATTRS( attrs[1], CKA_TOKEN, &cktrue, sizeof( CK_BBOOL ) );
	PK11_SETATTRS( attrs[2], CKA_LABEL, (unsigned char *)filename, strlen( filename ) + 1 );

	key = PK11_CreateGenericObject( slot, attrs, 3, PR_FALSE /* isPerm */ );

	if ( !key ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not add the private key '%s' - error %d:%s.\n",
			   filename, errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		retcode = -1;
	} else {
		tlsm_add_pem_obj( ctx, key );
		retcode = 0;

		/* When adding an encrypted key the PKCS#11 will be set as removed */
		/* This will force the token to be seen as re-inserted */
		SECMOD_WaitForAnyTokenEvent( pem_module, 0, 0 );
		PK11_IsPresent( slot );
	}

	PK11_FreeSlot( slot );

	return retcode;
}

static int
tlsm_init_ca_certs( tlsm_ctx *ctx, const char *cacertfile, const char *cacertdir )
{
	PRBool isca = PR_TRUE;
	PRStatus status = PR_SUCCESS;
	PRErrorCode errcode = PR_SUCCESS;

	if ( !cacertfile && !cacertdir ) {
		/* no checking - not good, but allowed */
		return 0;
	}

	if ( cacertfile ) {
		int rc = tlsm_add_cert_from_file( ctx, cacertfile, isca );
		if ( rc ) {
			errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: %s is not a valid CA certificate file - error %d:%s.\n",
				   cacertfile, errcode,
				   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
			/* failure with cacertfile is a hard failure even if cacertdir is
			   also specified and contains valid CA cert files */
			status = PR_FAILURE;
		} else {
			Debug( LDAP_DEBUG_TRACE,
				   "TLS: loaded CA certificate file %s.\n",
				   cacertfile, 0, 0 );
		}
	}

	/* if cacertfile above failed, we will return failure, even
	   if there is a valid CA cert in cacertdir - but we still
	   process cacertdir in case the user has enabled trace level
	   debugging so they can see the processing for cacertdir too */
	/* any cacertdir failures are "soft" failures - if the user specifies
	   no cert checking, then we allow the tls/ssl to continue, no matter
	   what was specified for cacertdir, or the contents of the directory
	   - this is different behavior than that of cacertfile */
	if ( cacertdir ) {
		PRFileInfo fi;
		PRDir *dir;
		PRDirEntry *entry;
		PRStatus fistatus = PR_FAILURE;

		memset( &fi, 0, sizeof(fi) );
		fistatus = PR_GetFileInfo( cacertdir, &fi );
		if ( PR_SUCCESS != fistatus) {
			errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: could not get info about the CA certificate directory %s - error %d:%s.\n",
				   cacertdir, errcode,
				   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
			goto done;
		}

		if ( fi.type != PR_FILE_DIRECTORY ) {
			Debug( LDAP_DEBUG_ANY,
				   "TLS: error: the CA certificate directory %s is not a directory.\n",
				   cacertdir, 0 ,0 );
			goto done;
		}

		dir = PR_OpenDir( cacertdir );
		if ( NULL == dir ) {
			errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: could not open the CA certificate directory %s - error %d:%s.\n",
				   cacertdir, errcode,
				   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
			goto done;
		}

		do {
			entry = PR_ReadDir( dir, PR_SKIP_BOTH | PR_SKIP_HIDDEN );
			if ( ( NULL != entry ) && ( NULL != entry->name ) ) {
				char *fullpath = NULL;
				char *ptr;

				ptr = PL_strrstr( entry->name, PEM_CA_HASH_FILE_SUFFIX );
				if ( ( ptr == NULL ) || ( *(ptr + PEM_CA_HASH_FILE_SUFFIX_LEN) != '\0' ) ) {
					Debug( LDAP_DEBUG_TRACE,
						   "TLS: file %s does not end in [%s] - does not appear to be a CA certificate "
						   "directory file with a properly hashed file name - skipping.\n",
						   entry->name, PEM_CA_HASH_FILE_SUFFIX, 0 );
					continue;
				}
				fullpath = PR_smprintf( "%s/%s", cacertdir, entry->name );
				if ( !tlsm_add_cert_from_file( ctx, fullpath, isca ) ) {
					Debug( LDAP_DEBUG_TRACE,
						   "TLS: loaded CA certificate file %s from CA certificate directory %s.\n",
						   fullpath, cacertdir, 0 );
				} else {
					errcode = PR_GetError();
					Debug( LDAP_DEBUG_TRACE,
						   "TLS: %s is not a valid CA certificate file - error %d:%s.\n",
						   fullpath, errcode,
						   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
				}
				PR_smprintf_free( fullpath );
			}
		} while ( NULL != entry );
		PR_CloseDir( dir );
	}
done:
	if ( status != PR_SUCCESS ) {
		return -1;
	}

	return 0;
}

/*
 * NSS supports having multiple cert/key databases in the same
 * directory, each one having a unique string prefix e.g.
 * slapd-01-cert8.db - the prefix here is "slapd-01-"
 * this function examines the given certdir - if it looks like
 * /path/to/directory/prefix it will return the
 * /path/to/directory part in realcertdir, and the prefix in prefix
 */
static void
tlsm_get_certdb_prefix( const char *certdir, char **realcertdir, char **prefix )
{
	char sep = PR_GetDirectorySeparator();
	char *ptr = NULL;
	struct PRFileInfo prfi;
	PRStatus prc;

	*realcertdir = (char *)certdir; /* default is the one passed in */

	/* if certdir is not given, just return */
	if ( !certdir ) {
		return;
	}

	prc = PR_GetFileInfo( certdir, &prfi );
	/* if certdir exists (file or directory) then it cannot specify a prefix */
	if ( prc == PR_SUCCESS ) {
		return;
	}

	/* if certdir was given, and there is a '/' in certdir, see if there
	   is anything after the last '/' - if so, assume it is the prefix */
	if ( ( ( ptr = strrchr( certdir, sep ) ) ) && *(ptr+1) ) {
		*realcertdir = PL_strndup( certdir, ptr-certdir );
		*prefix = PL_strdup( ptr+1 );
	}

	return;
}

/*
 * Currently mutiple MozNSS contexts share one certificate storage. When the
 * certdb is being opened, only new certificates are added to the storage.
 * When different databases are used, conflicting nicknames make the
 * certificate lookup by the nickname impossible. In addition a token
 * description might be prepended in certain conditions.
 *
 * In order to make the certificate lookup by nickname possible, we explicitly
 * open each database using SECMOD_OpenUserDB and assign it the token
 * description. The token description is generated using ctx->tc_unique value,
 * which is unique for each context.
 */
static PK11SlotInfo *
tlsm_init_open_certdb( tlsm_ctx *ctx, const char *dbdir, const char *prefix )
{
	PK11SlotInfo *slot = NULL;
	char *token_desc = NULL;
	char *config = NULL;

	token_desc = PR_smprintf( TLSM_CERTDB_DESC_FMT, ctx->tc_unique );
	config = PR_smprintf( "configDir='%s' tokenDescription='%s' certPrefix='%s' keyPrefix='%s' flags=readOnly",
										dbdir, token_desc, prefix, prefix );
	Debug( LDAP_DEBUG_TRACE, "TLS: certdb config: %s\n", config, 0, 0 );

	slot = SECMOD_OpenUserDB( config );
	if ( !slot ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_TRACE, "TLS: cannot open certdb '%s', error %d:%s\n", dbdir, errcode,
							PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
	}

	if ( token_desc )
		PR_smprintf_free( token_desc );
	if ( config )
		PR_smprintf_free( config );

	return slot;
}

/*
 * This is the part of the init we defer until we get the
 * actual security configuration information.  This is
 * only called once, protected by a PRCallOnce
 * NOTE: This must be done before the first call to SSL_ImportFD,
 * especially the setting of the policy
 * NOTE: This must be called after fork()
 */
static int
tlsm_deferred_init( void *arg )
{
	tlsm_ctx *ctx = (tlsm_ctx *)arg;
	struct ldaptls *lt = ctx->tc_config;
	const char *securitydirs[3];
	int ii;
	int nn;
	PRErrorCode errcode = 1;
#ifdef HAVE_NSS_INITCONTEXT
	NSSInitParameters initParams;
	NSSInitContext *initctx = NULL;
	PK11SlotInfo *certdb_slot = NULL;
#endif
	SECStatus rc;
	int done = 0;

#ifdef HAVE_SECMOD_RESTARTMODULES
	/* NSS enforces the pkcs11 requirement that modules should be unloaded after
	   a fork() - since there is no portable way to determine if NSS has been
	   already initialized in a parent process, we just call SECMOD_RestartModules
	   with force == FALSE - if the module has been unloaded due to a fork, it will
	   be reloaded, otherwise, it is a no-op */
	if ( SECFailure == ( rc = SECMOD_RestartModules(PR_FALSE /* do not force */) ) ) {
		errcode = PORT_GetError();
		if ( errcode != SEC_ERROR_NOT_INITIALIZED ) {
			Debug( LDAP_DEBUG_TRACE,
				   "TLS: could not restart the security modules: %d:%s\n",
				   errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ), 0 );
		} else {
			errcode = 1;
		}
	}
#endif

#ifdef HAVE_NSS_INITCONTEXT
	memset( &initParams, 0, sizeof( initParams ) );
	initParams.length = sizeof( initParams );
#endif /* HAVE_NSS_INITCONTEXT */

#ifdef LDAP_R_COMPILE
	if ( PR_CallOnce( &tlsm_init_mutex_callonce, tlsm_thr_init_callonce ) ) {
		return -1;
	}
#endif /* LDAP_R_COMPILE */

#ifndef HAVE_NSS_INITCONTEXT
	if ( !NSS_IsInitialized() ) {
#endif /* HAVE_NSS_INITCONTEXT */
		/*
		  MOZNSS_DIR will override everything else - you can
		  always set MOZNSS_DIR to force the use of this
		  directory
		  If using MOZNSS, specify the location of the moznss db dir
		  in the cacertdir directive of the OpenLDAP configuration.
		  DEFAULT_MOZNSS_DIR will only be used if the code cannot
		  find a security dir to use based on the current
		  settings
		*/
		nn = 0;
		securitydirs[nn++] = PR_GetEnv( "MOZNSS_DIR" );
		securitydirs[nn++] = lt->lt_cacertdir;
		securitydirs[nn++] = PR_GetEnv( "DEFAULT_MOZNSS_DIR" );
		for ( ii = 0; !done && ( ii < nn ); ++ii ) {
			char *realcertdir = NULL;
			const char *defprefix = "";
			char *prefix = (char *)defprefix;
			const char *securitydir = securitydirs[ii];
			if ( NULL == securitydir ) {
				continue;
			}

			tlsm_get_certdb_prefix( securitydir, &realcertdir, &prefix );

			/* initialize only moddb; certdb will be initialized explicitly */
#ifdef HAVE_NSS_INITCONTEXT
#ifdef INITCONTEXT_HACK
			if ( !NSS_IsInitialized() && ctx->tc_is_server ) {
				rc = NSS_Initialize( realcertdir, prefix, prefix, SECMOD_DB, NSS_INIT_READONLY );
			} else {
				initctx = NSS_InitContext( realcertdir, prefix, prefix, SECMOD_DB,
								   &initParams, NSS_INIT_READONLY|NSS_INIT_NOCERTDB );
			}
#else
			initctx = NSS_InitContext( realcertdir, prefix, prefix, SECMOD_DB,
								   &initParams, NSS_INIT_READONLY|NSS_INIT_NOCERTDB );
#endif
			rc = SECFailure;

			if ( initctx != NULL ) {
				certdb_slot = tlsm_init_open_certdb( ctx, realcertdir, prefix );
				if ( certdb_slot ) {
					rc = SECSuccess;
					ctx->tc_initctx = initctx;
					ctx->tc_certdb_slot = certdb_slot;
				} else {
					NSS_ShutdownContext( initctx );
					initctx = NULL;
				}
			}
#else
			rc = NSS_Initialize( realcertdir, prefix, prefix, SECMOD_DB, NSS_INIT_READONLY );
#endif

			if ( rc != SECSuccess ) {
				errcode = PORT_GetError();
				if ( securitydirs[ii] != lt->lt_cacertdir) {
					Debug( LDAP_DEBUG_TRACE,
						   "TLS: could not initialize moznss using security dir %s prefix %s - error %d.\n",
						   realcertdir, prefix, errcode );
				}
			} else {
				/* success */
				Debug( LDAP_DEBUG_TRACE, "TLS: using moznss security dir %s prefix %s.\n",
					   realcertdir, prefix, 0 );
				errcode = 0;
				done = 1;
			}
			if ( realcertdir != securitydir ) {
				PL_strfree( realcertdir );
			}
			if ( prefix != defprefix ) {
				PL_strfree( prefix );
			}
		}

		if ( errcode ) { /* no moznss db found, or not using moznss db */
#ifdef HAVE_NSS_INITCONTEXT
			int flags = NSS_INIT_READONLY|NSS_INIT_NOCERTDB|NSS_INIT_NOMODDB;
#ifdef INITCONTEXT_HACK
			if ( !NSS_IsInitialized() && ctx->tc_is_server ) {
				rc = NSS_NoDB_Init( NULL );
			} else {
				initctx = NSS_InitContext( CERTDB_NONE, PREFIX_NONE, PREFIX_NONE, SECMOD_DB,
										   &initParams, flags );
				rc = (initctx == NULL) ? SECFailure : SECSuccess;
			}
#else
			initctx = NSS_InitContext( CERTDB_NONE, PREFIX_NONE, PREFIX_NONE, SECMOD_DB,
									   &initParams, flags );
			if ( initctx ) {
				ctx->tc_initctx = initctx;
				rc = SECSuccess;
			} else {
				rc = SECFailure;
			}
#endif
#else
			rc = NSS_NoDB_Init( NULL );
#endif
			if ( rc != SECSuccess ) {
				errcode = PORT_GetError();
				Debug( LDAP_DEBUG_ANY,
					   "TLS: could not initialize moznss - error %d:%s.\n",
					   errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ), 0 );
				return -1;
			}
		}

		if ( errcode || lt->lt_cacertfile ) {
			/* initialize the PEM module */
			if ( tlsm_init_pem_module() ) {
				int pem_errcode = PORT_GetError();
				Debug( LDAP_DEBUG_ANY,
					   "TLS: could not initialize moznss PEM module - error %d:%s.\n",
					   pem_errcode, PR_ErrorToString( pem_errcode, PR_LANGUAGE_I_DEFAULT ), 0 );

				if ( errcode ) /* PEM is required */
					return -1;

			} else if ( !errcode ) {
				tlsm_init_ca_certs( ctx, lt->lt_cacertfile, NULL );
			}
		}

		if ( errcode ) {
			if ( tlsm_init_ca_certs( ctx, lt->lt_cacertfile, lt->lt_cacertdir ) ) {
				/* if we tried to use lt->lt_cacertdir as an NSS key/cert db, errcode 
				   will be a value other than 1 - print an error message so that the
				   user will know that failed too */
				if ( ( errcode != 1 ) && ( lt->lt_cacertdir ) ) {
					char *realcertdir = NULL;
					char *prefix = NULL;
					tlsm_get_certdb_prefix( lt->lt_cacertdir, &realcertdir, &prefix );
					Debug( LDAP_DEBUG_TRACE,
						   "TLS: could not initialize moznss using security dir %s prefix %s - error %d.\n",
						   realcertdir, prefix ? prefix : "", errcode );
					if ( realcertdir != lt->lt_cacertdir ) {
						PL_strfree( realcertdir );
					}
					PL_strfree( prefix );
				}
				return -1;
			}

			ctx->tc_using_pem = PR_TRUE;
		}

		NSS_SetDomesticPolicy();

		PK11_SetPasswordFunc( tlsm_pin_prompt );

		/* register cleanup function */
		if ( tlsm_register_nss_shutdown() ) {
			errcode = PORT_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: could not register NSS shutdown function: %d:%s\n",
				   errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ), 0 );
			return -1;
		}

		if  ( ctx->tc_is_server ) {
			/* 0 means use the defaults here */
			SSL_ConfigServerSessionIDCache( 0, 0, 0, NULL );
		}

#ifndef HAVE_NSS_INITCONTEXT
	}
#endif /* HAVE_NSS_INITCONTEXT */

	return 0;
}

/*
 * Find and verify the certificate.
 * The key is loaded and stored in ctx->tc_private_key
 */
static int
tlsm_find_and_verify_cert_key( tlsm_ctx *ctx )
{
	SECCertificateUsage certUsage;
	PRBool checkSig;
	SECStatus status;
	void *pin_arg;

	if ( tlsm_ctx_load_private_key( ctx ) )
		return -1;

	pin_arg = SSL_RevealPinArg( ctx->tc_model );
	certUsage = ctx->tc_is_server ? certificateUsageSSLServer : certificateUsageSSLClient;
	checkSig = ctx->tc_verify_cert ? PR_TRUE : PR_FALSE;

	status = tlsm_verify_cert( ctx->tc_certdb, ctx->tc_certificate, pin_arg,
							   checkSig, certUsage, ctx->tc_warn_only, PR_TRUE );

	return status == SECSuccess ? 0 : -1;
}

static int
tlsm_get_client_auth_data( void *arg, PRFileDesc *fd,
						   CERTDistNames *caNames, CERTCertificate **pRetCert,
						   SECKEYPrivateKey **pRetKey )
{
	tlsm_ctx *ctx = (tlsm_ctx *)arg;

	if ( pRetCert )
		*pRetCert = CERT_DupCertificate( ctx->tc_certificate );

	if ( pRetKey )
		*pRetKey = SECKEY_CopyPrivateKey( ctx->tc_private_key );

	return SECSuccess;
}

/*
 * ctx must have a tc_model that is valid
*/
static int
tlsm_clientauth_init( tlsm_ctx *ctx )
{
	SECStatus status = SECFailure;
	int rc;
	PRBool saveval;

	saveval = ctx->tc_warn_only;
	ctx->tc_warn_only = PR_TRUE;
	rc = tlsm_find_and_verify_cert_key(ctx);
	ctx->tc_warn_only = saveval;
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "TLS: error: unable to set up client certificate authentication for "
			   "certificate named %s\n", tlsm_ctx_subject_name(ctx), 0, 0 );
		return -1;
	}

	status = SSL_GetClientAuthDataHook( ctx->tc_model,
										tlsm_get_client_auth_data,
										(void *)ctx );

	return ( status == SECSuccess ? 0 : -1 );
}

/*
 * Tear down the TLS subsystem. Should only be called once.
 */
static void
tlsm_destroy( void )
{
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_destroy( &tlsm_ctx_count_mutex );
	ldap_pvt_thread_mutex_destroy( &tlsm_init_mutex );
	ldap_pvt_thread_mutex_destroy( &tlsm_pem_mutex );
#endif
}

static struct ldaptls *
tlsm_copy_config ( const struct ldaptls *config )
{
	struct ldaptls *copy;

	assert( config );

	copy = LDAP_MALLOC( sizeof( *copy ) );
	if ( !copy )
		return NULL;

	memset( copy, 0, sizeof( *copy ) );

	if ( config->lt_certfile )
		copy->lt_certfile = LDAP_STRDUP( config->lt_certfile );
	if ( config->lt_keyfile )
		copy->lt_keyfile = LDAP_STRDUP( config->lt_keyfile );
	if ( config->lt_dhfile )
		copy->lt_dhfile = LDAP_STRDUP( config->lt_dhfile );
	if ( config->lt_cacertfile )
		copy->lt_cacertfile = LDAP_STRDUP( config->lt_cacertfile );
	if ( config->lt_cacertdir )
		copy->lt_cacertdir = LDAP_STRDUP( config->lt_cacertdir );
	if ( config->lt_ciphersuite )
		copy->lt_ciphersuite = LDAP_STRDUP( config->lt_ciphersuite );
	if ( config->lt_crlfile )
		copy->lt_crlfile = LDAP_STRDUP( config->lt_crlfile );
	if ( config->lt_randfile )
		copy->lt_randfile = LDAP_STRDUP( config->lt_randfile );

	copy->lt_protocol_min = config->lt_protocol_min;

	return copy;
}

static void
tlsm_free_config ( struct ldaptls *config )
{
	assert( config );

	if ( config->lt_certfile )
		LDAP_FREE( config->lt_certfile );
	if ( config->lt_keyfile )
		LDAP_FREE( config->lt_keyfile );
	if ( config->lt_dhfile )
		LDAP_FREE( config->lt_dhfile );
	if ( config->lt_cacertfile )
		LDAP_FREE( config->lt_cacertfile );
	if ( config->lt_cacertdir )
		LDAP_FREE( config->lt_cacertdir );
	if ( config->lt_ciphersuite )
		LDAP_FREE( config->lt_ciphersuite );
	if ( config->lt_crlfile )
		LDAP_FREE( config->lt_crlfile );
	if ( config->lt_randfile )
		LDAP_FREE( config->lt_randfile );

	LDAP_FREE( config );
}

static tls_ctx *
tlsm_ctx_new ( struct ldapoptions *lo )
{
	tlsm_ctx *ctx;

	ctx = LDAP_MALLOC( sizeof (*ctx) );
	if ( ctx ) {
		ctx->tc_refcnt = 1;
#ifdef LDAP_R_COMPILE
		ldap_pvt_thread_mutex_init( &ctx->tc_refmutex );
#endif
		LDAP_MUTEX_LOCK( &tlsm_ctx_count_mutex );
		ctx->tc_unique = tlsm_ctx_count++;
		LDAP_MUTEX_UNLOCK( &tlsm_ctx_count_mutex );
		ctx->tc_config = NULL; /* populated later by tlsm_ctx_init */
		ctx->tc_certdb = NULL;
		ctx->tc_certdb_slot = NULL;
		ctx->tc_certificate = NULL;
		ctx->tc_private_key = NULL;
		ctx->tc_pin_file = NULL;
		ctx->tc_model = NULL;
		memset(&ctx->tc_callonce, 0, sizeof(ctx->tc_callonce));
		ctx->tc_require_cert = lo->ldo_tls_require_cert;
		ctx->tc_verify_cert = PR_FALSE;
		ctx->tc_using_pem = PR_FALSE;
#ifdef HAVE_NSS_INITCONTEXT
		ctx->tc_initctx = NULL;
#endif /* HAVE_NSS_INITCONTEXT */
		ctx->tc_pem_objs = NULL;
		ctx->tc_n_pem_objs = 0;
		ctx->tc_warn_only = PR_FALSE;
	}
	return (tls_ctx *)ctx;
}

static void
tlsm_ctx_ref( tls_ctx *ctx )
{
	tlsm_ctx *c = (tlsm_ctx *)ctx;
	LDAP_MUTEX_LOCK( &c->tc_refmutex );
	c->tc_refcnt++;
	LDAP_MUTEX_UNLOCK( &c->tc_refmutex );
}

static void
tlsm_ctx_free ( tls_ctx *ctx )
{
	tlsm_ctx *c = (tlsm_ctx *)ctx;
	int refcount;

	if ( !c ) return;

	LDAP_MUTEX_LOCK( &c->tc_refmutex );
	refcount = --c->tc_refcnt;
	LDAP_MUTEX_UNLOCK( &c->tc_refmutex );
	if ( refcount )
		return;

	LDAP_MUTEX_LOCK( &tlsm_init_mutex );
	if ( c->tc_model )
		PR_Close( c->tc_model );
	if ( c->tc_certificate )
		CERT_DestroyCertificate( c->tc_certificate );
	if ( c->tc_private_key )
		SECKEY_DestroyPrivateKey( c->tc_private_key );
	c->tc_certdb = NULL; /* if not the default, may have to clean up */
	if ( c->tc_certdb_slot ) {
		if ( SECMOD_CloseUserDB( c->tc_certdb_slot ) ) {
			PRErrorCode errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: could not close certdb slot - error %d:%s.\n",
				   errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ), 0 );
		}
	}
	if ( c->tc_pin_file ) {
		PL_strfree( c->tc_pin_file );
		c->tc_pin_file = NULL;
	}
	tlsm_free_pem_objs( c );
#ifdef HAVE_NSS_INITCONTEXT
	if ( c->tc_initctx ) {
		if ( NSS_ShutdownContext( c->tc_initctx ) ) {
			PRErrorCode errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: could not shutdown NSS - error %d:%s.\n",
				   errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ), 0 );
 		}
	}
	c->tc_initctx = NULL;
#endif /* HAVE_NSS_INITCONTEXT */
	LDAP_MUTEX_UNLOCK( &tlsm_init_mutex );
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_destroy( &c->tc_refmutex );
#endif

	if ( c->tc_config )
		tlsm_free_config( c->tc_config );

	LDAP_FREE( c );
}

/*
 * initialize a new TLS context
 */
static int
tlsm_ctx_init( struct ldapoptions *lo, struct ldaptls *lt, int is_server )
{
	tlsm_ctx *ctx = (tlsm_ctx *)lo->ldo_tls_ctx;
	ctx->tc_config = tlsm_copy_config( lt );
	ctx->tc_is_server = is_server;

	return 0;
}

/* returns true if the given string looks like
   "tokenname" ":" "certnickname"
   This is true if there is a ':' colon character
   in the string and the colon is not the first
   or the last character in the string
*/
static int
tlsm_is_tokenname_certnick( const char *certfile )
{
	if ( certfile ) {
		const char *ptr = PL_strchr( certfile, ':' );
		return ptr && (ptr != certfile) && (*(ptr+1));
	}
	return 0;
}

static int
tlsm_deferred_ctx_init( void *arg )
{
	tlsm_ctx *ctx = (tlsm_ctx *)arg;
	PRBool sslv2 = PR_FALSE;
	PRBool sslv3 = PR_TRUE;
	PRBool tlsv1 = PR_TRUE;
	PRBool request_cert = PR_FALSE;
	PRInt32 require_cert = PR_FALSE;
	PRFileDesc *fd;
	struct ldaptls *lt;

	if ( tlsm_deferred_init( ctx ) ) {
	    Debug( LDAP_DEBUG_ANY,
			   "TLS: could not perform TLS system initialization.\n",
			   0, 0, 0 );
	    return -1;
	}

	ctx->tc_certdb = CERT_GetDefaultCertDB(); /* If there is ever a per-context db, change this */

	fd = PR_CreateIOLayerStub( tlsm_layer_id, &tlsm_PR_methods );
	if ( fd ) {
		ctx->tc_model = SSL_ImportFD( NULL, fd );
	}

	if ( !ctx->tc_model ) {
		PRErrorCode err = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could perform TLS socket I/O layer initialization - error %d:%s.\n",
			   err, PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ), NULL );

		if ( fd ) {
			PR_Close( fd );
		}
		return -1;
	}

	if ( SSL_SetPKCS11PinArg(ctx->tc_model, ctx) ) {
		Debug( LDAP_DEBUG_ANY,
				"TLS: could not set pin prompt argument\n", 0, 0, 0);
		return -1;
	}

	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_SECURITY, PR_TRUE ) ) {
		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set secure mode on.\n",
		       0, 0, 0 );
		return -1;
	}

	lt = ctx->tc_config;

	/* default is sslv3 and tlsv1 */
	if ( lt->lt_protocol_min ) {
		if ( lt->lt_protocol_min > LDAP_OPT_X_TLS_PROTOCOL_SSL3 ) {
			sslv3 = PR_FALSE;
		} else if ( lt->lt_protocol_min <= LDAP_OPT_X_TLS_PROTOCOL_SSL2 ) {
			sslv2 = PR_TRUE;
			Debug( LDAP_DEBUG_ANY,
			       "TLS: warning: minimum TLS protocol level set to "
			       "include SSLv2 - SSLv2 is insecure - do not use\n", 0, 0, 0 );
		}
	}
	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_ENABLE_SSL2, sslv2 ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set SSLv2 mode on.\n",
		       0, 0, 0 );
		return -1;
	}
	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_ENABLE_SSL3, sslv3 ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set SSLv3 mode on.\n",
		       0, 0, 0 );
		return -1;
	}
	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_ENABLE_TLS, tlsv1 ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set TLSv1 mode on.\n",
		       0, 0, 0 );
		return -1;
	}

	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_HANDSHAKE_AS_CLIENT, !ctx->tc_is_server ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set handshake as client.\n",
		       0, 0, 0 );
		return -1;
	}
	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_HANDSHAKE_AS_SERVER, ctx->tc_is_server ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set handshake as server.\n",
		       0, 0, 0 );
		return -1;
	}

	if ( lt->lt_ciphersuite ) {
		if ( tlsm_parse_ciphers( ctx, lt->lt_ciphersuite ) ) {
			Debug( LDAP_DEBUG_ANY,
			       "TLS: could not set cipher list %s.\n",
			       lt->lt_ciphersuite, 0, 0 );
			return -1;
		}
	} else if ( tlsm_parse_ciphers( ctx, "DEFAULT" ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set cipher list DEFAULT.\n",
		       0, 0, 0 );
		return -1;
	}

	if ( !ctx->tc_require_cert ) {
		ctx->tc_verify_cert = PR_FALSE;
	} else if ( !ctx->tc_is_server ) {
		request_cert = PR_TRUE;
		require_cert = SSL_REQUIRE_NO_ERROR;
		if ( ctx->tc_require_cert == LDAP_OPT_X_TLS_DEMAND ||
		     ctx->tc_require_cert == LDAP_OPT_X_TLS_HARD ) {
			require_cert = SSL_REQUIRE_ALWAYS;
		}
		if ( ctx->tc_require_cert != LDAP_OPT_X_TLS_ALLOW )
			ctx->tc_verify_cert = PR_TRUE;
	} else { /* server */
		/* server does not request certs by default */
		/* if allow - client may send cert, server will ignore if errors */
		/* if try - client may send cert, server will error if bad cert */
		/* if hard or demand - client must send cert, server will error if bad cert */
		request_cert = PR_TRUE;
		require_cert = SSL_REQUIRE_NO_ERROR;
		if ( ctx->tc_require_cert == LDAP_OPT_X_TLS_DEMAND ||
		     ctx->tc_require_cert == LDAP_OPT_X_TLS_HARD ) {
			require_cert = SSL_REQUIRE_ALWAYS;
		}
		if ( ctx->tc_require_cert != LDAP_OPT_X_TLS_ALLOW ) {
			ctx->tc_verify_cert = PR_TRUE;
		} else {
			ctx->tc_warn_only = PR_TRUE;
		}
	}

	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_REQUEST_CERTIFICATE, request_cert ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set request certificate mode.\n",
		       0, 0, 0 );
		return -1;
	}
		
	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_REQUIRE_CERTIFICATE, require_cert ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set require certificate mode.\n",
		       0, 0, 0 );
		return -1;
	}

	/* set up our cert and key, if any */
	if ( lt->lt_certfile ) {
		/* if using the PEM module, load the PEM file specified by lt_certfile */
		/* otherwise, assume this is the name of a cert already in the db */
		if ( ctx->tc_using_pem ) {
			/* this sets ctx->tc_certificate to the correct value */
			int rc = tlsm_add_cert_from_file( ctx, lt->lt_certfile, PR_FALSE );
			if ( rc ) {
				return rc;
			}
		} else {
			char *tmp_certname;

			if ( tlsm_is_tokenname_certnick( lt->lt_certfile )) {
				/* assume already in form tokenname:certnickname */
				tmp_certname = PL_strdup( lt->lt_certfile );
			} else if ( ctx->tc_certdb_slot ) {
				tmp_certname = PR_smprintf( TLSM_CERTDB_DESC_FMT ":%s", ctx->tc_unique, lt->lt_certfile );
			} else {
				tmp_certname = PR_smprintf( "%s", lt->lt_certfile );
			}

			ctx->tc_certificate = PK11_FindCertFromNickname( tmp_certname, SSL_RevealPinArg( ctx->tc_model ) );
			PR_smprintf_free( tmp_certname );

			if ( !ctx->tc_certificate ) {
				PRErrorCode errcode = PR_GetError();
				Debug( LDAP_DEBUG_ANY,
					   "TLS: error: the certificate '%s' could not be found in the database - error %d:%s.\n",
					   lt->lt_certfile, errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
				return -1;
			}
		}
	}

	if ( lt->lt_keyfile ) {
		/* if using the PEM module, load the PEM file specified by lt_keyfile */
		/* otherwise, assume this is the pininfo for the key */
		if ( ctx->tc_using_pem ) {
			int rc = tlsm_add_key_from_file( ctx, lt->lt_keyfile );
			if ( rc ) {
				return rc;
			}
		} else {
			if ( ctx->tc_pin_file )
				PL_strfree( ctx->tc_pin_file );
			ctx->tc_pin_file = PL_strdup( lt->lt_keyfile );
		}
	}

	/* Set up callbacks for use by clients */
	if ( !ctx->tc_is_server ) {
		if ( SSL_OptionSet( ctx->tc_model, SSL_NO_CACHE, PR_TRUE ) != SECSuccess ) {
			PRErrorCode err = PR_GetError();
			Debug( LDAP_DEBUG_ANY, 
			       "TLS: error: could not set nocache option for moznss - error %d:%s\n",
			       err, PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ), NULL );
			return -1;
		}

		if ( SSL_BadCertHook( ctx->tc_model, tlsm_bad_cert_handler, ctx ) != SECSuccess ) {
			PRErrorCode err = PR_GetError();
			Debug( LDAP_DEBUG_ANY, 
			       "TLS: error: could not set bad cert handler for moznss - error %d:%s\n",
			       err, PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ), NULL );
			return -1;
		}

		/* 
		   since a cert has been specified, assume the client wants to do cert auth
		*/
		if ( ctx->tc_certificate ) {
			if ( tlsm_clientauth_init( ctx ) ) {
				Debug( LDAP_DEBUG_ANY, 
				       "TLS: error: unable to set up client certificate authentication using '%s'\n",
				       tlsm_ctx_subject_name(ctx), 0, 0 );
				return -1;
			}
		}
	} else { /* set up secure server */
		SSLKEAType certKEA;
		SECStatus status;

		/* must have a certificate for the server to use */
		if ( !ctx->tc_certificate ) {
			Debug( LDAP_DEBUG_ANY, 
			       "TLS: error: no server certificate: must specify a certificate for the server to use\n",
			       0, 0, 0 );
			return -1;
		}

		if ( tlsm_find_and_verify_cert_key( ctx ) ) {
			Debug( LDAP_DEBUG_ANY, 
			       "TLS: error: unable to find and verify server's cert and key for certificate %s\n",
			       tlsm_ctx_subject_name(ctx), 0, 0 );
			return -1;
		}

		/* configure the socket to be a secure server socket */
		certKEA = NSS_FindCertKEAType( ctx->tc_certificate );
		status = SSL_ConfigSecureServer( ctx->tc_model, ctx->tc_certificate, ctx->tc_private_key, certKEA );

		if ( SECSuccess != status ) {
			PRErrorCode err = PR_GetError();
			Debug( LDAP_DEBUG_ANY, 
			       "TLS: error: unable to configure secure server using certificate '%s' - error %d:%s\n",
			       tlsm_ctx_subject_name(ctx), err, PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ) );
			return -1;
		}
	}

	/* Callback for authenticating certificate */
	if ( SSL_AuthCertificateHook( ctx->tc_model, tlsm_auth_cert_handler,
                                  ctx ) != SECSuccess ) {
		PRErrorCode err = PR_GetError();
		Debug( LDAP_DEBUG_ANY, 
		       "TLS: error: could not set auth cert handler for moznss - error %d:%s\n",
		       err, PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ), NULL );
		return -1;
	}

	if ( SSL_HandshakeCallback( ctx->tc_model, tlsm_handshake_complete_cb, ctx ) ) {
		PRErrorCode err = PR_GetError();
		Debug( LDAP_DEBUG_ANY, 
		       "TLS: error: could not set handshake callback for moznss - error %d:%s\n",
		       err, PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ), NULL );
		return -1;
	}

	tlsm_free_config( ctx->tc_config );
	ctx->tc_config = NULL;

	return 0;
}

struct tls_data {
	tlsm_session		*session;
	Sockbuf_IO_Desc		*sbiod;
	/* there seems to be no portable way to determine if the
	   sockbuf sd has been set to nonblocking mode - the
	   call to ber_pvt_socket_set_nonblock() takes place
	   before the tls socket is set up, so we cannot
	   intercept that call either.
	   On systems where fcntl is available, we can just
	   F_GETFL and test for O_NONBLOCK.  On other systems,
	   we will just see if the IO op returns EAGAIN or EWOULDBLOCK,
	   and just set this flag */
	PRBool              nonblock;
	/*
	 * NSS tries hard to be backwards compatible with SSLv2 clients, or
	 * clients that send an SSLv2 client hello.  This message is not
	 * tagged in any way, so NSS has no way to know if the incoming
	 * message is a valid SSLv2 client hello or just some bogus data
	 * (or cleartext LDAP).  We store the first byte read from the
	 * client here.  The most common case will be a client sending
	 * LDAP data instead of SSL encrypted LDAP data.  This can happen,
	 * for example, if using ldapsearch -Z - if the starttls fails,
	 * the client will fallback to plain cleartext LDAP.  So if we
	 * see that the firstbyte is a valid LDAP tag, we can be
	 * pretty sure this is happening.
	 */
	ber_tag_t           firsttag;
	/*
	 * NSS doesn't return SSL_ERROR_WANT_READ, SSL_ERROR_WANT_WRITE, etc.
	 * when it is blocked, so we have to set a flag in the wrapped send
	 * and recv calls that tells us what operation NSS was last blocked
	 * on
	 */
#define TLSM_READ  1
#define TLSM_WRITE 2
	int io_flag;
};

static struct tls_data *
tlsm_get_pvt_tls_data( PRFileDesc *fd )
{
	struct tls_data		*p;
	PRFileDesc *myfd;

	if ( !fd ) {
		return NULL;
	}

	myfd = PR_GetIdentitiesLayer( fd, tlsm_layer_id );

	if ( !myfd ) {
		return NULL;
	}

	p = (struct tls_data *)myfd->secret;

	return p;
}

static int
tlsm_is_non_ssl_message( PRFileDesc *fd, ber_tag_t *thebyte )
{
	struct tls_data		*p;

	if ( thebyte ) {
		*thebyte = LBER_DEFAULT;
	}

	p = tlsm_get_pvt_tls_data( fd );
	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	if ( p->firsttag == LBER_SEQUENCE ) {
		if ( thebyte ) {
			*thebyte = p->firsttag;
		}
		return 1;
	}

	return 0;
}

static tls_session *
tlsm_session_new ( tls_ctx * ctx, int is_server )
{
	tlsm_ctx *c = (tlsm_ctx *)ctx;
	tlsm_session *session;
	PRFileDesc *fd;
	PRStatus status;
	int rc;

	c->tc_is_server = is_server;
	LDAP_MUTEX_LOCK( &tlsm_init_mutex );
	status = PR_CallOnceWithArg( &c->tc_callonce, tlsm_deferred_ctx_init, c );
	LDAP_MUTEX_UNLOCK( &tlsm_init_mutex );
	if ( PR_SUCCESS != status ) {
		PRErrorCode err = PR_GetError();
		Debug( LDAP_DEBUG_ANY, 
		       "TLS: error: could not initialize moznss security context - error %d:%s\n",
		       err, PR_ErrorToString(err, PR_LANGUAGE_I_DEFAULT), NULL );
		return NULL;
	}

	fd = PR_CreateIOLayerStub( tlsm_layer_id, &tlsm_PR_methods );
	if ( !fd ) {
		return NULL;
	}

	session = SSL_ImportFD( c->tc_model, fd );
	if ( !session ) {
		PR_DELETE( fd );
		return NULL;
	}

	rc = SSL_ResetHandshake( session, is_server );
	if ( rc ) {
		PRErrorCode err = PR_GetError();
		Debug( LDAP_DEBUG_TRACE, 
			   "TLS: error: new session - reset handshake failure %d - error %d:%s\n",
			   rc, err,
			   err ? PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ) : "unknown" );
		PR_DELETE( fd );
		PR_Close( session );
		session = NULL;
	}

	return (tls_session *)session;
} 

static int
tlsm_session_accept_or_connect( tls_session *session, int is_accept )
{
	tlsm_session *s = (tlsm_session *)session;
	int rc;
	const char *op = is_accept ? "accept" : "connect";

	if ( pem_module ) {
		LDAP_MUTEX_LOCK( &tlsm_pem_mutex );
	}
	rc = SSL_ForceHandshake( s );
	if ( pem_module ) {
		LDAP_MUTEX_UNLOCK( &tlsm_pem_mutex );
	}
	if ( rc ) {
		PRErrorCode err = PR_GetError();
		rc = -1;
		if ( err == PR_WOULD_BLOCK_ERROR ) {
			ber_tag_t thetag = LBER_DEFAULT;
			/* see if we are blocked because of a bogus packet */
			if ( tlsm_is_non_ssl_message( s, &thetag ) ) { /* see if we received a non-SSL message */
				Debug( LDAP_DEBUG_ANY, 
					   "TLS: error: %s - error - received non-SSL message [0x%x]\n",
					   op, (unsigned int)thetag, 0 );
				/* reset error to something more descriptive */
				PR_SetError( SSL_ERROR_RX_MALFORMED_HELLO_REQUEST, EPROTO );
			}
		} else {
			Debug( LDAP_DEBUG_ANY, 
				   "TLS: error: %s - force handshake failure: errno %d - moznss error %d\n",
				   op, errno, err );
		}
	}

	return rc;
}
static int
tlsm_session_accept( tls_session *session )
{
	return tlsm_session_accept_or_connect( session, 1 );
}

static int
tlsm_session_connect( LDAP *ld, tls_session *session )
{
	return tlsm_session_accept_or_connect( session, 0 );
}

static int
tlsm_session_upflags( Sockbuf *sb, tls_session *session, int rc )
{
	int prerror = PR_GetError();

	if ( ( prerror == PR_PENDING_INTERRUPT_ERROR ) || ( prerror == PR_WOULD_BLOCK_ERROR ) ) {
		tlsm_session *s = (tlsm_session *)session;
		struct tls_data *p = tlsm_get_pvt_tls_data( s );

		if ( p && ( p->io_flag == TLSM_READ ) ) {
			sb->sb_trans_needs_read = 1;
			return 1;
		} else if ( p && ( p->io_flag == TLSM_WRITE ) ) {
			sb->sb_trans_needs_write = 1;
			return 1;
		}
	}

	return 0;
}

static char *
tlsm_session_errmsg( tls_session *sess, int rc, char *buf, size_t len )
{
	int i;
	int prerror = PR_GetError();

	i = PR_GetErrorTextLength();
	if ( i > len ) {
		char *msg = LDAP_MALLOC( i+1 );
		PR_GetErrorText( msg );
		memcpy( buf, msg, len );
		LDAP_FREE( msg );
	} else if ( i ) {
		PR_GetErrorText( buf );
	} else if ( prerror ) {
		i = PR_snprintf( buf, len, "TLS error %d:%s",
						 prerror, PR_ErrorToString( prerror, PR_LANGUAGE_I_DEFAULT ) );
	}

	return ( i > 0 ) ? buf : NULL;
}

static int
tlsm_session_my_dn( tls_session *session, struct berval *der_dn )
{
	tlsm_session *s = (tlsm_session *)session;
	CERTCertificate *cert;

	cert = SSL_LocalCertificate( s );
	if (!cert) return LDAP_INVALID_CREDENTIALS;

	der_dn->bv_val = (char *)cert->derSubject.data;
	der_dn->bv_len = cert->derSubject.len;
	CERT_DestroyCertificate( cert );
	return 0;
}

static int
tlsm_session_peer_dn( tls_session *session, struct berval *der_dn )
{
	tlsm_session *s = (tlsm_session *)session;
	CERTCertificate *cert;

	cert = SSL_PeerCertificate( s );
	if (!cert) return LDAP_INVALID_CREDENTIALS;
	
	der_dn->bv_val = (char *)cert->derSubject.data;
	der_dn->bv_len = cert->derSubject.len;
	CERT_DestroyCertificate( cert );
	return 0;
}

/* what kind of hostname were we given? */
#define	IS_DNS	0
#define	IS_IP4	1
#define	IS_IP6	2

static int
tlsm_session_chkhost( LDAP *ld, tls_session *session, const char *name_in )
{
	tlsm_session *s = (tlsm_session *)session;
	CERTCertificate *cert;
	const char *name, *domain = NULL, *ptr;
	int ret, ntype = IS_DNS, nlen, dlen;
#ifdef LDAP_PF_INET6
	struct in6_addr addr;
#else
	struct in_addr addr;
#endif
	SECItem altname;
	SECStatus rv;

	if( ldap_int_hostname &&
		( !name_in || !strcasecmp( name_in, "localhost" ) ) )
	{
		name = ldap_int_hostname;
	} else {
		name = name_in;
	}
	nlen = strlen( name );

	cert = SSL_PeerCertificate( s );
	if (!cert) {
		Debug( LDAP_DEBUG_ANY,
			"TLS: unable to get peer certificate.\n",
			0, 0, 0 );
		/* if this was a fatal condition, things would have
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
	if (ntype == IS_DNS ) {
		domain = strchr( name, '.' );
		if ( domain )
			dlen = nlen - ( domain - name );
	}

	ret = LDAP_LOCAL_ERROR;

	rv = CERT_FindCertExtension( cert, SEC_OID_X509_SUBJECT_ALT_NAME,
		&altname );
	if ( rv == SECSuccess && altname.data ) {
		PRArenaPool *arena;
		CERTGeneralName *names, *cur;

		arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
		if ( !arena ) {
			ret = LDAP_NO_MEMORY;
			goto fail;
		}

		names = cur = CERT_DecodeAltNameExtension(arena, &altname);
		if ( !cur )
			goto altfail;

		do {
			char *host;
			int hlen;

			/* ignore empty */
			if ( !cur->name.other.len ) continue;

			host = (char *)cur->name.other.data;
			hlen = cur->name.other.len;

			if ( cur->type == certDNSName ) {
				if ( ntype != IS_DNS )	continue;

				/* is this an exact match? */
				if ( nlen == hlen && !strncasecmp( name, host, nlen )) {
					ret = LDAP_SUCCESS;
					break;
				}

				/* is this a wildcard match? */
				if ( domain && host[0] == '*' && host[1] == '.' &&
					dlen == hlen-1 && !strncasecmp( domain, host+1, dlen )) {
					ret = LDAP_SUCCESS;
					break;
				}
			} else if ( cur->type == certIPAddress ) {
				if ( ntype == IS_DNS )	continue;
				
#ifdef LDAP_PF_INET6
				if (ntype == IS_IP6 && hlen != sizeof(struct in6_addr)) {
					continue;
				} else
#endif
				if (ntype == IS_IP4 && hlen != sizeof(struct in_addr)) {
					continue;
				}
				if (!memcmp(host, &addr, hlen)) {
					ret = LDAP_SUCCESS;
					break;
				}
			}
		} while (( cur = CERT_GetNextGeneralName( cur )) != names );
altfail:
		PORT_FreeArena( arena, PR_FALSE );
		SECITEM_FreeItem( &altname, PR_FALSE );
	}
	/* no altnames matched, try the CN */
	if ( ret != LDAP_SUCCESS ) {
		/* find the last CN */
		CERTRDN *rdn, **rdns;
		CERTAVA *lastava = NULL;
		char buf[2048];

		buf[0] = '\0';
		rdns = cert->subject.rdns;
		while ( rdns && ( rdn = *rdns++ )) {
			CERTAVA *ava, **avas = rdn->avas;
			while ( avas && ( ava = *avas++ )) {
				if ( CERT_GetAVATag( ava ) == SEC_OID_AVA_COMMON_NAME )
					lastava = ava;
			}
		}
		if ( lastava ) {
			SECItem *av = CERT_DecodeAVAValue( &lastava->value );
			if ( av ) {
				if ( av->len == nlen && !strncasecmp( name, (char *)av->data, nlen )) {
					ret = LDAP_SUCCESS;
				} else if ( av->data[0] == '*' && av->data[1] == '.' &&
					domain && dlen == av->len - 1 && !strncasecmp( domain,
						(char *)(av->data+1), dlen )) {
					ret = LDAP_SUCCESS;
				} else {
					int len = av->len;
					if ( len >= sizeof(buf) )
						len = sizeof(buf)-1;
					memcpy( buf, av->data, len );
					buf[len] = '\0';
				}
				SECITEM_FreeItem( av, PR_TRUE );
			}
		}
		if ( ret != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "TLS: hostname (%s) does not match "
				"common name in certificate (%s).\n", 
				name, buf, 0 );
			ret = LDAP_CONNECT_ERROR;
			if ( ld->ld_error ) {
				LDAP_FREE( ld->ld_error );
			}
			ld->ld_error = LDAP_STRDUP(
				_("TLS: hostname does not match CN in peer certificate"));
		}
	}

fail:
	CERT_DestroyCertificate( cert );
	return ret;
}

static int
tlsm_session_strength( tls_session *session )
{
	tlsm_session *s = (tlsm_session *)session;
	int rc, keySize;

	rc = SSL_SecurityStatus( s, NULL, NULL, NULL, &keySize,
		NULL, NULL );
	return rc ? 0 : keySize;
}

static int
tlsm_session_unique( tls_session *sess, struct berval *buf, int is_server)
{
	/* Need upstream support https://bugzilla.mozilla.org/show_bug.cgi?id=563276 */
	return 0;
}

/* Yet again, we're pasting in glue that MozNSS ought to provide itself. */
static struct {
	const char *name;
	int num;
} pvers[] = {
	{ "SSLv2", SSL_LIBRARY_VERSION_2 },
	{ "SSLv3", SSL_LIBRARY_VERSION_3_0 },
	{ "TLSv1", SSL_LIBRARY_VERSION_TLS_1_0 },
	{ "TLSv1.1", SSL_LIBRARY_VERSION_TLS_1_1 },
	{ NULL, 0 }
};

static const char *
tlsm_session_version( tls_session *sess )
{
	tlsm_session *s = (tlsm_session *)sess;
	SSLChannelInfo info;
	int rc;
	rc = SSL_GetChannelInfo( s, &info, sizeof( info ));
	if ( rc == 0 ) {
		int i;
		for (i=0; pvers[i].name; i++)
			if (pvers[i].num == info.protocolVersion)
				return pvers[i].name;
	}
	return "unknown";
}

static const char *
tlsm_session_cipher( tls_session *sess )
{
	tlsm_session *s = (tlsm_session *)sess;
	SSLChannelInfo info;
	int rc;
	rc = SSL_GetChannelInfo( s, &info, sizeof( info ));
	if ( rc == 0 ) {
		SSLCipherSuiteInfo csinfo;
		rc = SSL_GetCipherSuiteInfo( info.cipherSuite, &csinfo, sizeof( csinfo ));
		if ( rc == 0 )
			return csinfo.cipherSuiteName;
	}
	return "unknown";
}

static int
tlsm_session_peercert( tls_session *sess, struct berval *der )
{
	tlsm_session *s = (tlsm_session *)sess;
	CERTCertificate *cert;
	cert = SSL_PeerCertificate( s );
	if (!cert)
		return -1;
	der->bv_len = cert->derCert.len;
	der->bv_val = LDAP_MALLOC( der->bv_len );
	if (!der->bv_val)
		return -1;
	memcpy( der->bv_val, cert->derCert.data, der->bv_len );
	return 0;
}

/*
 * TLS support for LBER Sockbufs
 */

static PRStatus PR_CALLBACK
tlsm_PR_Close(PRFileDesc *fd)
{
	int rc = PR_SUCCESS;

	/* we don't need to actually close anything here, just
	   pop our io layer off the stack */
	fd->secret = NULL; /* must have been freed before calling PR_Close */
	if ( fd->lower ) {
		fd = PR_PopIOLayer( fd, tlsm_layer_id );
		/* if we are not the last layer, pass the close along */
		if ( fd ) {
			if ( fd->dtor ) {
				fd->dtor( fd );
			}
			rc = fd->methods->close( fd );
		}
	} else {
		/* we are the last layer - just call our dtor */
		fd->dtor(fd);
	}

	return rc;
}

static PRStatus PR_CALLBACK
tlsm_PR_Shutdown(PRFileDesc *fd, PRShutdownHow how)
{
	int rc = PR_SUCCESS;

	if ( fd->lower ) {
		rc = PR_Shutdown( fd->lower, how );
	}

	return rc;
}

static int PR_CALLBACK
tlsm_PR_Recv(PRFileDesc *fd, void *buf, PRInt32 len, PRIntn flags,
	 PRIntervalTime timeout)
{
	struct tls_data		*p;
	int rc;

	if ( buf == NULL || len <= 0 ) return 0;

	p = tlsm_get_pvt_tls_data( fd );

	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	rc = LBER_SBIOD_READ_NEXT( p->sbiod, buf, len );
	if (rc <= 0) {
		tlsm_map_error( errno );
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			p->nonblock = PR_TRUE; /* fd is using non-blocking io */
		} else if ( errno ) { /* real error */
			Debug( LDAP_DEBUG_TRACE, 
			       "TLS: error: tlsm_PR_Recv returned %d - error %d:%s\n",
			       rc, errno, STRERROR(errno) );
		}
	} else if ( ( rc > 0 ) && ( len > 0 ) && ( p->firsttag == LBER_DEFAULT ) ) {
		p->firsttag = (ber_tag_t)*((char *)buf);
	}
	p->io_flag = TLSM_READ;

	return rc;
}

static int PR_CALLBACK
tlsm_PR_Send(PRFileDesc *fd, const void *buf, PRInt32 len, PRIntn flags,
	 PRIntervalTime timeout)
{
	struct tls_data		*p;
	int rc;

	if ( buf == NULL || len <= 0 ) return 0;

	p = tlsm_get_pvt_tls_data( fd );

	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	rc = LBER_SBIOD_WRITE_NEXT( p->sbiod, (char *)buf, len );
	if (rc <= 0) {
		tlsm_map_error( errno );
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			p->nonblock = PR_TRUE;
		} else if ( errno ) { /* real error */
			Debug( LDAP_DEBUG_TRACE, 
			       "TLS: error: tlsm_PR_Send returned %d - error %d:%s\n",
			       rc, errno, STRERROR(errno) );
		}
	}
	p->io_flag = TLSM_WRITE;

	return rc;
}

static int PR_CALLBACK
tlsm_PR_Read(PRFileDesc *fd, void *buf, PRInt32 len)
{
	return tlsm_PR_Recv( fd, buf, len, 0, PR_INTERVAL_NO_TIMEOUT );
}

static int PR_CALLBACK
tlsm_PR_Write(PRFileDesc *fd, const void *buf, PRInt32 len)
{
	return tlsm_PR_Send( fd, buf, len, 0, PR_INTERVAL_NO_TIMEOUT );
}

static PRStatus PR_CALLBACK
tlsm_PR_GetPeerName(PRFileDesc *fd, PRNetAddr *addr)
{
	struct tls_data		*p;
	ber_socklen_t len;

 	p = tlsm_get_pvt_tls_data( fd );

	if ( p == NULL || p->sbiod == NULL ) {
		return PR_FAILURE;
	}
	len = sizeof(PRNetAddr);
	return getpeername( p->sbiod->sbiod_sb->sb_fd, (struct sockaddr *)addr, &len );
}

static PRStatus PR_CALLBACK
tlsm_PR_GetSocketOption(PRFileDesc *fd, PRSocketOptionData *data)
{
	struct tls_data		*p;
 	p = tlsm_get_pvt_tls_data( fd );

	if ( p == NULL || data == NULL ) {
		return PR_FAILURE;
	}

	/* only the nonblocking option is supported at this time
	   MozNSS SSL code needs it */
	if ( data->option != PR_SockOpt_Nonblocking ) {
		PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
		return PR_FAILURE;
	}
#ifdef HAVE_FCNTL
	int flags = fcntl( p->sbiod->sbiod_sb->sb_fd, F_GETFL );
	data->value.non_blocking = (flags & O_NONBLOCK) ? PR_TRUE : PR_FALSE;		
#else /* punt :P */
	data->value.non_blocking = p->nonblock;
#endif
	return PR_SUCCESS;
}

static PRStatus PR_CALLBACK
tlsm_PR_prs_unimp()
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

static PRFileDesc * PR_CALLBACK
tlsm_PR_pfd_unimp()
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}

static PRInt16 PR_CALLBACK
tlsm_PR_i16_unimp()
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return SECFailure;
}

static PRInt32 PR_CALLBACK
tlsm_PR_i32_unimp()
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return SECFailure;
}

static PRInt64 PR_CALLBACK
tlsm_PR_i64_unimp()
{
    PRInt64 res;

    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    LL_I2L(res, -1L);
    return res;
}

static const PRIOMethods tlsm_PR_methods = {
    PR_DESC_LAYERED,
    tlsm_PR_Close,			/* close        */
    tlsm_PR_Read,			/* read         */
    tlsm_PR_Write,			/* write        */
    tlsm_PR_i32_unimp,		/* available    */
    tlsm_PR_i64_unimp,		/* available64  */
    tlsm_PR_prs_unimp,		/* fsync        */
    tlsm_PR_i32_unimp,		/* seek         */
    tlsm_PR_i64_unimp,		/* seek64       */
    tlsm_PR_prs_unimp,		/* fileInfo     */
    tlsm_PR_prs_unimp,		/* fileInfo64   */
    tlsm_PR_i32_unimp,		/* writev       */
    tlsm_PR_prs_unimp,		/* connect      */
    tlsm_PR_pfd_unimp,		/* accept       */
    tlsm_PR_prs_unimp,		/* bind         */
    tlsm_PR_prs_unimp,		/* listen       */
    (PRShutdownFN)tlsm_PR_Shutdown,			/* shutdown     */
    tlsm_PR_Recv,			/* recv         */
    tlsm_PR_Send,			/* send         */
    tlsm_PR_i32_unimp,		/* recvfrom     */
    tlsm_PR_i32_unimp,		/* sendto       */
    (PRPollFN)tlsm_PR_i16_unimp,	/* poll         */
    tlsm_PR_i32_unimp,		/* acceptread   */
    tlsm_PR_i32_unimp,		/* transmitfile */
    tlsm_PR_prs_unimp,		/* getsockname  */
    tlsm_PR_GetPeerName,	/* getpeername  */
    tlsm_PR_i32_unimp,		/* getsockopt   OBSOLETE */
    tlsm_PR_i32_unimp,		/* setsockopt   OBSOLETE */
    tlsm_PR_GetSocketOption,		/* getsocketoption   */
    tlsm_PR_i32_unimp,		/* setsocketoption   */
    tlsm_PR_i32_unimp,		/* Send a (partial) file with header/trailer*/
    (PRConnectcontinueFN)tlsm_PR_prs_unimp,		/* connectcontinue */
    tlsm_PR_i32_unimp,		/* reserved for future use */
    tlsm_PR_i32_unimp,		/* reserved for future use */
    tlsm_PR_i32_unimp,		/* reserved for future use */
    tlsm_PR_i32_unimp		/* reserved for future use */
};

/*
 * Initialize TLS subsystem. Should be called only once.
 * See tlsm_deferred_init for the bulk of the init process
 */
static int
tlsm_init( void )
{
	char *nofork = PR_GetEnv( "NSS_STRICT_NOFORK" );

	PR_Init(0, 0, 0);

	tlsm_layer_id = PR_GetUniqueIdentity( "OpenLDAP" );

	/*
	 * There are some applications that acquire a crypto context in the parent process
	 * and expect that crypto context to work after a fork().  This does not work
	 * with NSS using strict PKCS11 compliance mode.  We set this environment
	 * variable here to tell the software encryption module/token to allow crypto
	 * contexts to persist across a fork().  However, if you are using some other
	 * module or encryption device that supports and expects full PKCS11 semantics,
	 * the only recourse is to rewrite the application with atfork() handlers to save
	 * the crypto context in the parent and restore (and SECMOD_RestartModules) the
	 * context in the child.
	 */
	if ( !nofork ) {
		/* will leak one time */
		char *noforkenvvar = PL_strdup( "NSS_STRICT_NOFORK=DISABLED" );
		PR_SetEnv( noforkenvvar );
	}

	return 0;
}

static int
tlsm_sb_setup( Sockbuf_IO_Desc *sbiod, void *arg )
{
	struct tls_data		*p;
	tlsm_session	*session = arg;
	PRFileDesc *fd;

	assert( sbiod != NULL );

	p = LBER_MALLOC( sizeof( *p ) );
	if ( p == NULL ) {
		return -1;
	}

	fd = PR_GetIdentitiesLayer( session, tlsm_layer_id );
	if ( !fd ) {
		LBER_FREE( p );
		return -1;
	}

	fd->secret = (PRFilePrivate *)p;
	p->session = session;
	p->sbiod = sbiod;
	p->firsttag = LBER_DEFAULT;
	sbiod->sbiod_pvt = p;
	return 0;
}

static int
tlsm_sb_remove( Sockbuf_IO_Desc *sbiod )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	PR_Close( p->session );
	LBER_FREE( sbiod->sbiod_pvt );
	sbiod->sbiod_pvt = NULL;
	return 0;
}

static int
tlsm_sb_close( Sockbuf_IO_Desc *sbiod )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	PR_Shutdown( p->session, PR_SHUTDOWN_BOTH );
	return 0;
}

static int
tlsm_sb_ctrl( Sockbuf_IO_Desc *sbiod, int opt, void *arg )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	
	if ( opt == LBER_SB_OPT_GET_SSL ) {
		*((tlsm_session **)arg) = p->session;
		return 1;
		
	} else if ( opt == LBER_SB_OPT_DATA_READY ) {
		if ( p && ( SSL_DataPending( p->session ) > 0 ) ) {
			return 1;
		}
		
	}
	
	return LBER_SBIOD_CTRL_NEXT( sbiod, opt, arg );
}

static ber_slen_t
tlsm_sb_read( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data		*p;
	ber_slen_t		ret;
	int			err;

	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct tls_data *)sbiod->sbiod_pvt;

	ret = PR_Recv( p->session, buf, len, 0, PR_INTERVAL_NO_TIMEOUT );
	if ( ret < 0 ) {
		err = PR_GetError();
		if ( err == PR_PENDING_INTERRUPT_ERROR || err == PR_WOULD_BLOCK_ERROR ) {
			sbiod->sbiod_sb->sb_trans_needs_read = 1;
			sock_errset(EWOULDBLOCK);
		}
	} else {
		sbiod->sbiod_sb->sb_trans_needs_read = 0;
	}
	return ret;
}

static ber_slen_t
tlsm_sb_write( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data		*p;
	ber_slen_t		ret;
	int			err;

	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct tls_data *)sbiod->sbiod_pvt;

	ret = PR_Send( p->session, (char *)buf, len, 0, PR_INTERVAL_NO_TIMEOUT );
	if ( ret < 0 ) {
		err = PR_GetError();
		if ( err == PR_PENDING_INTERRUPT_ERROR || err == PR_WOULD_BLOCK_ERROR ) {
			sbiod->sbiod_sb->sb_trans_needs_write = 1;
			sock_errset(EWOULDBLOCK);
			ret = 0;
		}
	} else {
		sbiod->sbiod_sb->sb_trans_needs_write = 0;
	}
	return ret;
}

static Sockbuf_IO tlsm_sbio =
{
	tlsm_sb_setup,		/* sbi_setup */
	tlsm_sb_remove,		/* sbi_remove */
	tlsm_sb_ctrl,		/* sbi_ctrl */
	tlsm_sb_read,		/* sbi_read */
	tlsm_sb_write,		/* sbi_write */
	tlsm_sb_close		/* sbi_close */
};

tls_impl ldap_int_tls_impl = {
	"MozNSS",

	tlsm_init,
	tlsm_destroy,

	tlsm_ctx_new,
	tlsm_ctx_ref,
	tlsm_ctx_free,
	tlsm_ctx_init,

	tlsm_session_new,
	tlsm_session_connect,
	tlsm_session_accept,
	tlsm_session_upflags,
	tlsm_session_errmsg,
	tlsm_session_my_dn,
	tlsm_session_peer_dn,
	tlsm_session_chkhost,
	tlsm_session_strength,
	tlsm_session_unique,
	tlsm_session_version,
	tlsm_session_cipher,
	tlsm_session_peercert,

	&tlsm_sbio,

#ifdef LDAP_R_COMPILE
	tlsm_thr_init,
#else
	NULL,
#endif

	0
};

#endif /* HAVE_MOZNSS */
/*
  emacs settings
  Local Variables:
  indent-tabs-mode: t
  tab-width: 4
  End:
*/
