/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2009-2013 The OpenLDAP Foundation.
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
 * This work was initially developed by HAMANO Tsukasa <hamano@osstech.co.jp>
 */

#define _GNU_SOURCE

#include "portable.h"
#include <ac/string.h>
#include "lber_pvt.h"
#include "lutil.h"

#include <openssl/evp.h>

#define PBKDF2_SALT_SIZE 16
#define PBKDF2_DK_SIZE   20
#define PBKDF2_ITERATION 60000

const struct berval pbkdf2scheme = BER_BVC("{PBKDF2}");

/*
 * Converting base64 string to adapted base64 string.
 * Adapted base64 encode is identical to general base64 encode except
 * that it uses '.' instead of '+', and omits trailing padding '=' and
 * whitepsace.
 * see http://pythonhosted.org/passlib/lib/passlib.utils.html
 * This is destructive function.
 */
static int b64_to_ab64(char *str)
{
	char *p = str;
	while(*p++){
		if(*p == '+'){
			*p = '.';
		}
		if(*p == '='){
			*p = '\0';
			break;
		}
	}
	return 0;
}

/*
 * Converting adapted base64 string to base64 string.
 * dstsize will require src length + 2, due to output string have
 * potential to append "=" or "==".
 * return -1 if few output buffer.
 */
static int ab64_to_b64(char *src, char *dst, size_t dstsize){
	int i;
	char *p = src;
	for(i=0; p[i] && p[i] != '$'; i++){
		if(i >= dstsize){
			dst[0] = '\0';
			return -1;
		}
		if(p[i] == '.'){
			dst[i] = '+';
		}else{
			dst[i] = p[i];
		}
	}
	for(;i%4;i++){
		if(i >= dstsize){
			dst[0] = '\0';
			return -1;
		}
		dst[i] = '=';
	}
	dst[i] = '\0';
	return 0;
}

static int pbkdf2_format(
	const struct berval *sc,
	int iteration,
	const struct berval *salt,
	const struct berval *dk,
	struct berval *msg)
{

	int rc;
	char salt_b64[LUTIL_BASE64_ENCODE_LEN(PBKDF2_SALT_SIZE) + 1];
	char dk_b64[LUTIL_BASE64_ENCODE_LEN(PBKDF2_DK_SIZE) + 1];

	rc = lutil_b64_ntop((unsigned char *)salt->bv_val, salt->bv_len,
						salt_b64, sizeof(salt_b64));
	if(rc < 0){
		return LUTIL_PASSWD_ERR;
	}
	b64_to_ab64(salt_b64);
	rc = lutil_b64_ntop((unsigned char *)dk->bv_val, dk->bv_len,
						dk_b64, sizeof(dk_b64));
	if(rc < 0){
		return LUTIL_PASSWD_ERR;
	}
	b64_to_ab64(dk_b64);
	msg->bv_len = asprintf(&msg->bv_val, "%s%d$%s$%s",
						   sc->bv_val, iteration,
						   salt_b64, dk_b64);
	if(msg->bv_len < 0){
		return LUTIL_PASSWD_ERR;
	}

#ifdef SLAPD_PBKDF2_DEBUG
	printf("  Output:\t%s\n", msg->bv_val);
#endif
	return LUTIL_PASSWD_OK;
}

static int pbkdf2_encrypt(
	const struct berval *scheme,
	const struct berval *passwd,
	struct berval *msg,
	const char **text)
{
	unsigned char salt_value[PBKDF2_SALT_SIZE];
	struct berval salt;
	unsigned char dk_value[PBKDF2_DK_SIZE];
	struct berval dk;
	int iteration = PBKDF2_ITERATION;

	dk.bv_val = (char *)dk_value;
	dk.bv_len = PBKDF2_DK_SIZE;
	salt.bv_val = (char *)salt_value;
	salt.bv_len = sizeof(salt_value);

	if(lutil_entropy((unsigned char *)salt.bv_val, salt.bv_len) < 0){
		return LUTIL_PASSWD_ERR;
	}

	if(!PKCS5_PBKDF2_HMAC_SHA1(passwd->bv_val, passwd->bv_len,
							   (unsigned char *)salt.bv_val, salt.bv_len,
							   iteration, PBKDF2_DK_SIZE, dk_value)){
		return LUTIL_PASSWD_ERR;
	}

#ifdef SLAPD_PBKDF2_DEBUG
	printf("DEBUG pbkdf2_encrypt()\n");
	printf("  Password:\t%s\n", passwd->bv_val);

	printf("  Salt:\t\t");
	int i;
	for(i=0; i<salt.bv_len; i++){
		printf("%02x", salt_value[i]);
	}
	printf("\n");
	printf("  Iteration:\t%d\n", iteration);

	printf("  DK:\t\t");
	for(i=0; i<PBKDF2_DK_SIZE; i++){
		printf("%02x", dk_value[i]);
	}
	printf("\n");
#endif

	return pbkdf2_format(scheme, iteration, &salt, &dk, msg);
}

static int pbkdf2_check(
	const struct berval *scheme,
	const struct berval *passwd,
	const struct berval *cred,
	const char **text)
{
	int rc;
	int iteration;

	/* salt_value require PBKDF2_SALT_SIZE + 1 in lutil_b64_pton. */
	unsigned char salt_value[PBKDF2_SALT_SIZE + 1];
	char salt_b64[LUTIL_BASE64_ENCODE_LEN(PBKDF2_SALT_SIZE) + 1];
	/* dk_value require PBKDF2_DK_SIZE + 1 in lutil_b64_pton. */
	unsigned char dk_value[PBKDF2_DK_SIZE + 1];
	char dk_b64[LUTIL_BASE64_ENCODE_LEN(PBKDF2_DK_SIZE) + 1];
	unsigned char input_dk_value[PBKDF2_DK_SIZE];

#ifdef SLAPD_PBKDF2_DEBUG
	printf("DEBUG pbkdf2_check()\n");
	printf("  Stored Value:\t%s\n", passwd->bv_val);
	printf("  Input Cred:\t%s\n", cred->bv_val);
#endif

	iteration = atoi(passwd->bv_val);
	if(iteration < 1){
		return LUTIL_PASSWD_ERR;
	}

	char *ptr;
	ptr = strchr(passwd->bv_val, '$');
	if(!ptr){
		return LUTIL_PASSWD_ERR;
	}
	ptr++; /* skip '$' */
	rc = ab64_to_b64(ptr, salt_b64, sizeof(salt_b64));
	if(rc < 0){
		return LUTIL_PASSWD_ERR;
	}

	ptr = strchr(ptr, '$');
	if(!ptr){
		return LUTIL_PASSWD_ERR;
	}
	ptr++; /* skip '$' */
	rc = ab64_to_b64(ptr, dk_b64, sizeof(dk_b64));
	if(rc < 0){
		return LUTIL_PASSWD_ERR;
	}

	/* The targetsize require PBKDF2_SALT_SIZE + 1 in lutil_b64_pton. */
	rc = lutil_b64_pton(salt_b64, salt_value, PBKDF2_SALT_SIZE + 1);
	if(rc < 0){
		return LUTIL_PASSWD_ERR;
	}

	/* consistency check */
	if(rc != PBKDF2_SALT_SIZE){
		return LUTIL_PASSWD_ERR;
	}

	/* The targetsize require PBKDF2_DK_SIZE + 1 in lutil_b64_pton. */
	rc = lutil_b64_pton(dk_b64, dk_value, PBKDF2_DK_SIZE + 1);
	if(rc < 0){
		return LUTIL_PASSWD_ERR;
	}

	/* consistency check */
	if(rc != PBKDF2_DK_SIZE){
		return LUTIL_PASSWD_ERR;
	}

	if(!PKCS5_PBKDF2_HMAC_SHA1(cred->bv_val, cred->bv_len,
							   salt_value, PBKDF2_SALT_SIZE,
							   iteration, PBKDF2_DK_SIZE, input_dk_value)){
		return LUTIL_PASSWD_ERR;
	}

	rc = memcmp(dk_value, input_dk_value, PBKDF2_DK_SIZE);
#ifdef SLAPD_PBKDF2_DEBUG
	printf("  Iteration:\t%d\n", iteration);
	printf("  Base64 Salt:\t%s\n", salt_b64);
	printf("  Base64 DK:\t%s\n", dk_b64);
	int i;
	printf("  Stored Salt:\t");
	for(i=0; i<PBKDF2_SALT_SIZE; i++){
		printf("%02x", salt_value[i]);
	}
	printf("\n");

	printf("  Stored DK:\t");
	for(i=0; i<PBKDF2_DK_SIZE; i++){
		printf("%02x", dk_value[i]);
	}
	printf("\n");

	printf("  Input DK:\t");
	for(i=0; i<PBKDF2_DK_SIZE; i++){
		printf("%02x", input_dk_value[i]);
	}
	printf("\n");
	printf("  Result:\t%d\n", rc);
#endif
	return rc?LUTIL_PASSWD_ERR:LUTIL_PASSWD_OK;
}

int init_module(int argc, char *argv[]) {
	int rc;
	rc = lutil_passwd_add((struct berval *)&pbkdf2scheme,
						  pbkdf2_check, pbkdf2_encrypt);
	if(!rc) return rc;

	/* TODO: add {PBKDF2-SHA256} and {PBKDF2-SHA512} schemes. */
	return rc;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
