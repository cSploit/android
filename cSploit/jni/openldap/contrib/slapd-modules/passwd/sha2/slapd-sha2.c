/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2009-2014 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENT:
 * This work was initially developed by Jeff Turner for inclusion
 * in OpenLDAP Software.
 *
 * Hash methods for passwords generation added by Cédric Delfosse.
 *
 * SSHA256 / SSHA384 / SSHA512 support added, and chk_sha*() replaced
 * with libraries/liblutil/passwd.c:chk_sha1() implementation to
 * fix a race by SATOH Fumiyasu @ OSS Technology, Inc.
 */

#include "portable.h"

#include <ac/string.h>

#include "lber_pvt.h"
#include "lutil.h"
#include "sha2.h"

#ifdef SLAPD_SHA2_DEBUG
#include <stdio.h>
#endif

#define SHA2_SALT_SIZE 8

static int hash_ssha256(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text )
{
	SHA256_CTX ct;
	unsigned char hash256[SHA256_DIGEST_LENGTH];
	char          saltdata[SHA2_SALT_SIZE];
	struct berval digest;
	struct berval salt;

	digest.bv_val = (char *) hash256;
	digest.bv_len = sizeof(hash256);
	salt.bv_val = saltdata;
	salt.bv_len = sizeof(saltdata);

	if (lutil_entropy((unsigned char *)salt.bv_val, salt.bv_len) < 0) {
		return LUTIL_PASSWD_ERR;
	}

	SHA256_Init(&ct);
	SHA256_Update(&ct, (const uint8_t*)passwd->bv_val, passwd->bv_len);
	SHA256_Update(&ct, (const uint8_t*)salt.bv_val, salt.bv_len);
	SHA256_Final(hash256, &ct);

	return lutil_passwd_string64(scheme, &digest, hash, &salt);
}

static int hash_sha256(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text )
{
	SHA256_CTX ct;
	unsigned char hash256[SHA256_DIGEST_LENGTH];
	struct berval digest;
	digest.bv_val = (char *) hash256;
	digest.bv_len = sizeof(hash256);

	SHA256_Init(&ct);
	SHA256_Update(&ct, (const uint8_t*)passwd->bv_val, passwd->bv_len);
	SHA256_Final(hash256, &ct);

	return lutil_passwd_string64(scheme, &digest, hash, NULL);
}

static int hash_ssha384(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text )
{
	SHA384_CTX ct;
	unsigned char hash384[SHA384_DIGEST_LENGTH];
	char          saltdata[SHA2_SALT_SIZE];
	struct berval digest;
	struct berval salt;

	digest.bv_val = (char *) hash384;
	digest.bv_len = sizeof(hash384);
	salt.bv_val = saltdata;
	salt.bv_len = sizeof(saltdata);

	if (lutil_entropy((unsigned char *)salt.bv_val, salt.bv_len) < 0) {
		return LUTIL_PASSWD_ERR;
	}

	SHA384_Init(&ct);
	SHA384_Update(&ct, (const uint8_t*)passwd->bv_val, passwd->bv_len);
	SHA384_Update(&ct, (const uint8_t*)salt.bv_val, salt.bv_len);
	SHA384_Final(hash384, &ct);

	return lutil_passwd_string64(scheme, &digest, hash, &salt);
}

static int hash_sha384(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text )
{
	SHA384_CTX ct;
	unsigned char hash384[SHA384_DIGEST_LENGTH];
	struct berval digest;
	digest.bv_val = (char *) hash384;
	digest.bv_len = sizeof(hash384);

	SHA384_Init(&ct);
	SHA384_Update(&ct, (const uint8_t*)passwd->bv_val, passwd->bv_len);
	SHA384_Final(hash384, &ct);

	return lutil_passwd_string64(scheme, &digest, hash, NULL);
}

static int hash_ssha512(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text )
{
	SHA512_CTX ct;
	unsigned char hash512[SHA512_DIGEST_LENGTH];
	char          saltdata[SHA2_SALT_SIZE];
	struct berval digest;
	struct berval salt;

	digest.bv_val = (char *) hash512;
	digest.bv_len = sizeof(hash512);
	salt.bv_val = saltdata;
	salt.bv_len = sizeof(saltdata);

	if (lutil_entropy((unsigned char *)salt.bv_val, salt.bv_len) < 0) {
		return LUTIL_PASSWD_ERR;
	}

	SHA512_Init(&ct);
	SHA512_Update(&ct, (const uint8_t*)passwd->bv_val, passwd->bv_len);
	SHA512_Update(&ct, (const uint8_t*)salt.bv_val, salt.bv_len);
	SHA512_Final(hash512, &ct);

	return lutil_passwd_string64(scheme, &digest, hash, &salt);
}

static int hash_sha512(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *hash,
	const char **text )
{
	SHA512_CTX ct;
	unsigned char hash512[SHA512_DIGEST_LENGTH];
	struct berval digest;
	digest.bv_val = (char *) hash512;
	digest.bv_len = sizeof(hash512);

	SHA512_Init(&ct);
	SHA512_Update(&ct, (const uint8_t*)passwd->bv_val, passwd->bv_len);
	SHA512_Final(hash512, &ct);

	return lutil_passwd_string64(scheme, &digest, hash, NULL);
}

#ifdef SLAPD_SHA2_DEBUG
static void chk_sha_debug(
	const struct berval *scheme,
	const struct berval *passwd,
	const struct berval *cred,
	const char *cred_hash,
	size_t cred_len,
	int cmp_rc)
{
	int rc;
	struct berval cred_b64;

	cred_b64.bv_len = LUTIL_BASE64_ENCODE_LEN(cred_len) + 1;
	cred_b64.bv_val = ber_memalloc(cred_b64.bv_len + 1);

	if( cred_b64.bv_val == NULL ) {
		return;
	}

	rc = lutil_b64_ntop(
		(unsigned char *) cred_hash, cred_len,
		cred_b64.bv_val, cred_b64.bv_len );

	if( rc < 0 ) {
		ber_memfree(cred_b64.bv_val);
		return;
	}

	fprintf(stderr, "Validating password\n");
	fprintf(stderr, "  Hash scheme:\t\t%s\n", scheme->bv_val);
	fprintf(stderr, "  Password to validate: %s\n", cred->bv_val);
	fprintf(stderr, "  Password hash:\t%s\n", cred_b64.bv_val);
	fprintf(stderr, "  Stored password hash:\t%s\n", passwd->bv_val);
	fprintf(stderr, "  Result:\t\t%s\n", cmp_rc ?  "do not match" : "match");

	ber_memfree(cred_b64.bv_val);
}
#endif

static int chk_ssha256(
	const struct berval *scheme, /* Scheme of hashed reference password */
	const struct berval *passwd, /* Hashed reference password to check against */
	const struct berval *cred, /* user-supplied password to check */
	const char **text )
{
	SHA256_CTX SHAcontext;
	unsigned char SHAdigest[SHA256_DIGEST_LENGTH];
	int rc;
	unsigned char *orig_pass = NULL;

	/* safety check */
	if (LUTIL_BASE64_DECODE_LEN(passwd->bv_len) <= sizeof(SHAdigest)) {
		return LUTIL_PASSWD_ERR;
	}

	/* base64 un-encode password */
	orig_pass = (unsigned char *) ber_memalloc( (size_t) (
		LUTIL_BASE64_DECODE_LEN(passwd->bv_len) + 1) );

	if( orig_pass == NULL ) return LUTIL_PASSWD_ERR;

	rc = lutil_b64_pton(passwd->bv_val, orig_pass, passwd->bv_len);

	if( rc <= sizeof(SHAdigest) ) {
		ber_memfree(orig_pass);
		return LUTIL_PASSWD_ERR;
	}

	/* hash credentials with salt */
	SHA256_Init(&SHAcontext);
	SHA256_Update(&SHAcontext,
		(const unsigned char *) cred->bv_val, cred->bv_len);
	SHA256_Update(&SHAcontext,
		(const unsigned char *) &orig_pass[sizeof(SHAdigest)],
		rc - sizeof(SHAdigest));
	SHA256_Final(SHAdigest, &SHAcontext);

	/* compare */
	rc = memcmp((char *)orig_pass, (char *)SHAdigest, sizeof(SHAdigest));
	ber_memfree(orig_pass);
	return rc ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;
}

static int chk_sha256(
	const struct berval *scheme, /* Scheme of hashed reference password */
	const struct berval *passwd, /* Hashed reference password to check against */
	const struct berval *cred, /* user-supplied password to check */
	const char **text )
{
	SHA256_CTX SHAcontext;
	unsigned char SHAdigest[SHA256_DIGEST_LENGTH];
	int rc;
	unsigned char *orig_pass = NULL;

	/* safety check */
	if (LUTIL_BASE64_DECODE_LEN(passwd->bv_len) < sizeof(SHAdigest)) {
		return LUTIL_PASSWD_ERR;
	}

	/* base64 un-encode password */
	orig_pass = (unsigned char *) ber_memalloc( (size_t) (
		LUTIL_BASE64_DECODE_LEN(passwd->bv_len) + 1) );

	if( orig_pass == NULL ) return LUTIL_PASSWD_ERR;

	rc = lutil_b64_pton(passwd->bv_val, orig_pass, passwd->bv_len);

	if( rc != sizeof(SHAdigest) ) {
		ber_memfree(orig_pass);
		return LUTIL_PASSWD_ERR;
	}

	/* hash credentials with salt */
	SHA256_Init(&SHAcontext);
	SHA256_Update(&SHAcontext,
		(const unsigned char *) cred->bv_val, cred->bv_len);
	SHA256_Final(SHAdigest, &SHAcontext);

	/* compare */
	rc = memcmp((char *)orig_pass, (char *)SHAdigest, sizeof(SHAdigest));
#ifdef SLAPD_SHA2_DEBUG
	chk_sha_debug(scheme, passwd, cred, (char *)SHAdigest, sizeof(SHAdigest), rc);
#endif
	ber_memfree(orig_pass);
	return rc ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;
}

static int chk_ssha384(
	const struct berval *scheme, /* Scheme of hashed reference password */
	const struct berval *passwd, /* Hashed reference password to check against */
	const struct berval *cred, /* user-supplied password to check */
	const char **text )
{
	SHA384_CTX SHAcontext;
	unsigned char SHAdigest[SHA384_DIGEST_LENGTH];
	int rc;
	unsigned char *orig_pass = NULL;

	/* safety check */
	if (LUTIL_BASE64_DECODE_LEN(passwd->bv_len) <= sizeof(SHAdigest)) {
		return LUTIL_PASSWD_ERR;
	}

	/* base64 un-encode password */
	orig_pass = (unsigned char *) ber_memalloc( (size_t) (
		LUTIL_BASE64_DECODE_LEN(passwd->bv_len) + 1) );

	if( orig_pass == NULL ) return LUTIL_PASSWD_ERR;

	rc = lutil_b64_pton(passwd->bv_val, orig_pass, passwd->bv_len);

	if( rc <= sizeof(SHAdigest) ) {
		ber_memfree(orig_pass);
		return LUTIL_PASSWD_ERR;
	}

	/* hash credentials with salt */
	SHA384_Init(&SHAcontext);
	SHA384_Update(&SHAcontext,
		(const unsigned char *) cred->bv_val, cred->bv_len);
	SHA384_Update(&SHAcontext,
		(const unsigned char *) &orig_pass[sizeof(SHAdigest)],
		rc - sizeof(SHAdigest));
	SHA384_Final(SHAdigest, &SHAcontext);

	/* compare */
	rc = memcmp((char *)orig_pass, (char *)SHAdigest, sizeof(SHAdigest));
	ber_memfree(orig_pass);
	return rc ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;
}

static int chk_sha384(
	const struct berval *scheme, /* Scheme of hashed reference password */
	const struct berval *passwd, /* Hashed reference password to check against */
	const struct berval *cred, /* user-supplied password to check */
	const char **text )
{
	SHA384_CTX SHAcontext;
	unsigned char SHAdigest[SHA384_DIGEST_LENGTH];
	int rc;
	unsigned char *orig_pass = NULL;

	/* safety check */
	if (LUTIL_BASE64_DECODE_LEN(passwd->bv_len) < sizeof(SHAdigest)) {
		return LUTIL_PASSWD_ERR;
	}

	/* base64 un-encode password */
	orig_pass = (unsigned char *) ber_memalloc( (size_t) (
		LUTIL_BASE64_DECODE_LEN(passwd->bv_len) + 1) );

	if( orig_pass == NULL ) return LUTIL_PASSWD_ERR;

	rc = lutil_b64_pton(passwd->bv_val, orig_pass, passwd->bv_len);

	if( rc != sizeof(SHAdigest) ) {
		ber_memfree(orig_pass);
		return LUTIL_PASSWD_ERR;
	}

	/* hash credentials with salt */
	SHA384_Init(&SHAcontext);
	SHA384_Update(&SHAcontext,
		(const unsigned char *) cred->bv_val, cred->bv_len);
	SHA384_Final(SHAdigest, &SHAcontext);

	/* compare */
	rc = memcmp((char *)orig_pass, (char *)SHAdigest, sizeof(SHAdigest));
#ifdef SLAPD_SHA2_DEBUG
	chk_sha_debug(scheme, passwd, cred, (char *)SHAdigest, sizeof(SHAdigest), rc);
#endif
	ber_memfree(orig_pass);
	return rc ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;
}

static int chk_ssha512(
	const struct berval *scheme, /* Scheme of hashed reference password */
	const struct berval *passwd, /* Hashed reference password to check against */
	const struct berval *cred, /* user-supplied password to check */
	const char **text )
{
	SHA512_CTX SHAcontext;
	unsigned char SHAdigest[SHA512_DIGEST_LENGTH];
	int rc;
	unsigned char *orig_pass = NULL;

	/* safety check */
	if (LUTIL_BASE64_DECODE_LEN(passwd->bv_len) <= sizeof(SHAdigest)) {
		return LUTIL_PASSWD_ERR;
	}

	/* base64 un-encode password */
	orig_pass = (unsigned char *) ber_memalloc( (size_t) (
		LUTIL_BASE64_DECODE_LEN(passwd->bv_len) + 1) );

	if( orig_pass == NULL ) return LUTIL_PASSWD_ERR;

	rc = lutil_b64_pton(passwd->bv_val, orig_pass, passwd->bv_len);

	if( rc <= sizeof(SHAdigest) ) {
		ber_memfree(orig_pass);
		return LUTIL_PASSWD_ERR;
	}

	/* hash credentials with salt */
	SHA512_Init(&SHAcontext);
	SHA512_Update(&SHAcontext,
		(const unsigned char *) cred->bv_val, cred->bv_len);
	SHA512_Update(&SHAcontext,
		(const unsigned char *) &orig_pass[sizeof(SHAdigest)],
		rc - sizeof(SHAdigest));
	SHA512_Final(SHAdigest, &SHAcontext);

	/* compare */
	rc = memcmp((char *)orig_pass, (char *)SHAdigest, sizeof(SHAdigest));
	ber_memfree(orig_pass);
	return rc ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;
}

static int chk_sha512(
	const struct berval *scheme, /* Scheme of hashed reference password */
	const struct berval *passwd, /* Hashed reference password to check against */
	const struct berval *cred, /* user-supplied password to check */
	const char **text )
{
	SHA512_CTX SHAcontext;
	unsigned char SHAdigest[SHA512_DIGEST_LENGTH];
	int rc;
	unsigned char *orig_pass = NULL;

	/* safety check */
	if (LUTIL_BASE64_DECODE_LEN(passwd->bv_len) < sizeof(SHAdigest)) {
		return LUTIL_PASSWD_ERR;
	}

	/* base64 un-encode password */
	orig_pass = (unsigned char *) ber_memalloc( (size_t) (
		LUTIL_BASE64_DECODE_LEN(passwd->bv_len) + 1) );

	if( orig_pass == NULL ) return LUTIL_PASSWD_ERR;

	rc = lutil_b64_pton(passwd->bv_val, orig_pass, passwd->bv_len);

	if( rc != sizeof(SHAdigest) ) {
		ber_memfree(orig_pass);
		return LUTIL_PASSWD_ERR;
	}

	/* hash credentials with salt */
	SHA512_Init(&SHAcontext);
	SHA512_Update(&SHAcontext,
		(const unsigned char *) cred->bv_val, cred->bv_len);
	SHA512_Final(SHAdigest, &SHAcontext);

	/* compare */
	rc = memcmp((char *)orig_pass, (char *)SHAdigest, sizeof(SHAdigest));
#ifdef SLAPD_SHA2_DEBUG
	chk_sha_debug(scheme, passwd, cred, (char *)SHAdigest, sizeof(SHAdigest), rc);
#endif
	ber_memfree(orig_pass);
	return rc ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;
}

const struct berval ssha256scheme = BER_BVC("{SSHA256}");
const struct berval sha256scheme = BER_BVC("{SHA256}");
const struct berval ssha384scheme = BER_BVC("{SSHA384}");
const struct berval sha384scheme = BER_BVC("{SHA384}");
const struct berval ssha512scheme = BER_BVC("{SSHA512}");
const struct berval sha512scheme = BER_BVC("{SHA512}");

int init_module(int argc, char *argv[]) {
	int result = 0;
	result = lutil_passwd_add( (struct berval *)&ssha256scheme, chk_ssha256, hash_ssha256 );
	if (result != 0) return result;
	result = lutil_passwd_add( (struct berval *)&sha256scheme, chk_sha256, hash_sha256 );
	if (result != 0) return result;
	result = lutil_passwd_add( (struct berval *)&ssha384scheme, chk_ssha384, hash_ssha384 );
	if (result != 0) return result;
	result = lutil_passwd_add( (struct berval *)&sha384scheme, chk_sha384, hash_sha384 );
	if (result != 0) return result;
	result = lutil_passwd_add( (struct berval *)&ssha512scheme, chk_ssha512, hash_ssha512 );
	if (result != 0) return result;
	result = lutil_passwd_add( (struct berval *)&sha512scheme, chk_sha512, hash_sha512 );
	return result;
}
