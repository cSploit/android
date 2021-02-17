/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-1999  Brian Bruns
 * Copyright (C) 2005-2011  Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STDDEF_H
#include <stddef.h>
#endif /* HAVE_STDDEF_H */

#include <ctype.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <freetds/tds.h>
#include <freetds/bytes.h>
#include <freetds/string.h>
#include "md4.h"
#include "md5.h"
#include "hmac_md5.h"
#include "des.h"
#include <freetds/iconv.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#elif defined(HAVE_OPENSSL)
#include <openssl/rand.h>
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif

/**
 * \ingroup libtds
 * \defgroup auth Authentication
 * Functions for handling authentication.
 */

/**
 * \addtogroup auth
 * @{ 
 */

/*
 * The following code is based on some psuedo-C code from ronald@innovation.ch
 */

typedef struct tds_answer
{
	unsigned char lm_resp[24];
	unsigned char nt_resp[24];
} TDSANSWER;


typedef struct
{
	TDS_TINYINT     response_type;
	TDS_TINYINT     max_response_type;
	TDS_SMALLINT    reserved1;
	TDS_UINT        reserved2;
	TDS_UINT8       timestamp;
	TDS_UCHAR       challenge[8];
	TDS_UINT        unknown;
	/* target info block - variable length */
	TDS_UCHAR	target_info[4];
} names_blob_prefix_t;

static TDSRET
tds_answer_challenge(TDSSOCKET * tds,
		     TDSLOGIN * login,
		     const unsigned char *challenge,
		     TDS_UINT * flags,
		     const unsigned char *names_blob, TDS_INT names_blob_len, TDSANSWER * answer, unsigned char **ntlm_v2_response);
static void tds_encrypt_answer(const unsigned char *hash, const unsigned char *challenge, unsigned char *answer);
static void tds_convert_key(const unsigned char *key_56, DES_KEY * ks);

static void
convert_to_upper(char *buf, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		buf[i] = toupper((unsigned char) buf[i]);
}

static size_t
convert_to_usc2le_string(TDSSOCKET * tds, const char *s, size_t len, char *out)
{
	const char *ib;
	char *ob;
	size_t il, ol;

	TDSICONV * char_conv = tds->conn->char_convs[client2ucs2];

	/* char_conv is only mostly const */
	TDS_ERRNO_MESSAGE_FLAGS *suppress = (TDS_ERRNO_MESSAGE_FLAGS *) & char_conv->suppress;

	if (char_conv->flags == TDS_ENCODING_MEMCPY) {
		memcpy(out, s, len);
		return len;
	}

	/* convert */
	ib = s;
	il = len;
	ob = out;
	ol = len * 2;
	memset(suppress, 0, sizeof(char_conv->suppress));
	if (tds_iconv(tds, char_conv, to_server, &ib, &il, &ob, &ol) == (size_t) - 1)
		return (size_t) -1;

	return ob - out;
}


static void
generate_random_buffer(unsigned char *out, int len)
{
	int i;

#ifdef HAVE_GNUTLS
	if (gnutls_rnd(GNUTLS_RND_RANDOM, out, len) >= 0)
		return;
	if (gnutls_rnd(GNUTLS_RND_NONCE, out, len) >= 0)
		return;
#elif defined(HAVE_OPENSSL)
	if (RAND_bytes(out, len) == 1)
		return;
	if (RAND_pseudo_bytes(out, len) >= 0)
		return;
#endif

	/* TODO find a better random... */
	for (i = 0; i < len; ++i)
		out[i] = rand() / (RAND_MAX / 256);
}

static TDSRET
make_ntlm_hash(TDSSOCKET * tds, const char *passwd, unsigned char ntlm_hash[16])
{
	MD4_CTX context;
	size_t passwd_len = 0;
	char passwd_usc2le[256];
	size_t passwd_usc2le_len = 0;

	passwd_len = strlen(passwd);

	if (passwd_len > 128)
		passwd_len = 128;

	passwd_usc2le_len = convert_to_usc2le_string(tds, passwd, passwd_len, passwd_usc2le);
	if (passwd_usc2le_len == (size_t) -1) {
		memset((char *) passwd_usc2le, 0, sizeof(passwd_usc2le));
		return TDS_FAIL;
	}

	/* compute NTLM hash */
	MD4Init(&context);
	MD4Update(&context, (unsigned char *) passwd_usc2le, passwd_usc2le_len);
	MD4Final(&context, ntlm_hash);

	/* with security is best be pedantic */
	memset((char *) passwd_usc2le, 0, passwd_usc2le_len);
	memset(&context, 0, sizeof(context));
	return TDS_SUCCESS;
}


static TDSRET
make_ntlm_v2_hash(TDSSOCKET * tds, const char *passwd, unsigned char ntlm_v2_hash[16])
{
	const char *user_name, *domain;
	size_t domain_len, user_name_len, len, buf_usc2le_len = 0;
	const char *p;

	unsigned char ntlm_hash[16];
	char buf[128];
	char buf_usc2le[512];
	TDSRET res;

	user_name = tds_dstr_cstr(&tds->login->user_name);

	/* parse domain\username */
	p = strchr(user_name, '\\');

	domain = user_name;
	domain_len = p - user_name;

	user_name = p + 1;
	user_name_len = strlen(user_name);

	if (user_name_len > 128)
		user_name_len = 128;
	memcpy(buf, user_name, user_name_len);
	convert_to_upper(buf, user_name_len);

	len = convert_to_usc2le_string(tds, buf, user_name_len, buf_usc2le);
	if (len == (size_t) -1)
		return TDS_FAIL;
	buf_usc2le_len = len;

	if (domain_len > 128)
		domain_len = 128;
	/* Target is supposed to be case-sensitive */

	len = convert_to_usc2le_string(tds, domain, domain_len, buf_usc2le + len);
	if (len == (size_t) -1)
		return TDS_FAIL;
	buf_usc2le_len += len;


	res = make_ntlm_hash(tds, passwd, ntlm_hash);
	hmac_md5(ntlm_hash, (const unsigned char *) buf_usc2le, buf_usc2le_len, ntlm_v2_hash);

	/* with security is best be pedantic */
	memset(&ntlm_hash, 0, sizeof(ntlm_hash));
	memset(buf, 0, sizeof(buf));
	memset((char *) buf_usc2le, 0, buf_usc2le_len);
	return res;
}


/*
 * hash - The NTLMv2 Hash.
 * client_data - The client data (blob or client nonce).
 * challenge - The server challenge from the Type 2 message.
 */
static unsigned char *
make_lm_v2_response(const unsigned char ntlm_v2_hash[16],
		    const unsigned char *client_data, TDS_INT client_data_len, const unsigned char challenge[8])
{
	int mac_len = 16 + client_data_len;
	unsigned char *mac;

	mac = (unsigned char*) malloc(mac_len);
	if (!mac)
		return NULL;

	memcpy(mac + 8, challenge, 8);
	memcpy(mac + 16, client_data, client_data_len);
	hmac_md5(ntlm_v2_hash, mac + 8, client_data_len + 8, mac);

	return mac;
}

static TDSRET
tds_answer_challenge_ntlmv2(TDSSOCKET * tds,
		     TDSLOGIN * login,
		     const unsigned char *challenge,
		     TDS_UINT * flags,
		     const unsigned char *names_blob, TDS_INT names_blob_len, TDSANSWER * answer, unsigned char **ntlm_v2_response)
{
	TDSRET res;
	const char *passwd = tds_dstr_cstr(&login->password);

	/* NTLMv2 */
	unsigned char *lm_v2_response;
	unsigned char ntlm_v2_hash[16];
	const names_blob_prefix_t *names_blob_prefix;

	res = make_ntlm_v2_hash(tds, passwd, ntlm_v2_hash);
	if (TDS_FAILED(res))
		return res;

	/* LMv2 response */
	/* Take client's challenge from names_blob */
	names_blob_prefix = (const names_blob_prefix_t *) names_blob;
	lm_v2_response = make_lm_v2_response(ntlm_v2_hash, names_blob_prefix->challenge, 8, challenge);
	if (!lm_v2_response)
		return TDS_FAIL;
	memcpy(answer->lm_resp, lm_v2_response, 24);
	free(lm_v2_response);

	/* NTLMv2 response */
	/* Size of lm_v2_response is 16 + names_blob_len */
	*ntlm_v2_response = make_lm_v2_response(ntlm_v2_hash, names_blob, names_blob_len, challenge);
	if (!*ntlm_v2_response)
		return TDS_FAIL;

	memset(ntlm_v2_hash, 0, sizeof(ntlm_v2_hash));

	/* local not supported */
	*flags &= ~0x4000;
	return TDS_SUCCESS;
}

/**
 * Crypt a given password using schema required for NTLMv1 or NTLM2 authentication
 * @param passwd clear text domain password
 * @param challenge challenge data given by server
 * @param flags NTLM flags from server side
 * @param answer buffer where to store crypted password
 */
static TDSRET
tds_answer_challenge(TDSSOCKET * tds,
		     TDSLOGIN * login,
		     const unsigned char *challenge,
		     TDS_UINT * flags,
		     const unsigned char *names_blob, TDS_INT names_blob_len, TDSANSWER * answer, unsigned char **ntlm_v2_response)
{
#define MAX_PW_SZ 14
	const char *passwd = tds_dstr_cstr(&login->password);
	DES_KEY ks;
	unsigned char hash[24], ntlm2_challenge[16];
	TDSRET res;

	memset(answer, 0, sizeof(TDSANSWER));

	if (login->use_ntlmv2) {
		return tds_answer_challenge_ntlmv2(tds, login, challenge, flags,
						   names_blob, names_blob_len, answer, ntlm_v2_response);
	} else if ((*flags & 0x80000) != 0) {
		/* NTLM2 */
		MD5_CTX md5_ctx;

		generate_random_buffer(hash, 8);
		memset(hash + 8, 0, 16);
		memcpy(answer->lm_resp, hash, 24);

		MD5Init(&md5_ctx);
		MD5Update(&md5_ctx, challenge, 8);
		MD5Update(&md5_ctx, hash, 8);
		MD5Final(&md5_ctx, ntlm2_challenge);
		challenge = ntlm2_challenge;
		memset(&md5_ctx, 0, sizeof(md5_ctx));
	} else if (login->use_lanman) {
		/* LM */
		size_t len, i;
		unsigned char passwd_buf[MAX_PW_SZ];
		static const des_cblock magic = { 0x4B, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25 };

		/* convert password to upper and pad to 14 chars */
		memset(passwd_buf, 0, MAX_PW_SZ);
		len = strlen(passwd);
		if (len > MAX_PW_SZ)
			len = MAX_PW_SZ;
		for (i = 0; i < len; i++)
			passwd_buf[i] = toupper((unsigned char) passwd[i]);

		/* hash the first 7 characters */
		tds_convert_key(passwd_buf, &ks);
		tds_des_ecb_encrypt(&magic, sizeof(magic), &ks, (hash + 0));

		/* hash the second 7 characters */
		tds_convert_key(passwd_buf + 7, &ks);
		tds_des_ecb_encrypt(&magic, sizeof(magic), &ks, (hash + 8));

		memset(hash + 16, 0, 5);

		tds_encrypt_answer(hash, challenge, answer->lm_resp);
		memset(passwd_buf, 0, sizeof(passwd_buf));
	} else {
		memset(answer->lm_resp, 0, sizeof(answer->lm_resp));
	}
	*flags = 0x8201;

	/* NTLM/NTLM2 response */
	res = make_ntlm_hash(tds, passwd, hash);
	memset(hash + 16, 0, 5);

	tds_encrypt_answer(hash, challenge, answer->nt_resp);

	/* with security is best be pedantic */
	memset(&ks, 0, sizeof(ks));
	memset(hash, 0, sizeof(hash));
	memset(ntlm2_challenge, 0, sizeof(ntlm2_challenge));
	return res;
}


/*
* takes a 21 byte array and treats it as 3 56-bit DES keys. The
* 8 byte plaintext is encrypted with each key and the resulting 24
* bytes are stored in the results array.
*/
static void
tds_encrypt_answer(const unsigned char *hash, const unsigned char *challenge, unsigned char *answer)
{
	DES_KEY ks;

	tds_convert_key(hash, &ks);
	tds_des_ecb_encrypt(challenge, 8, &ks, answer);

	tds_convert_key(&hash[7], &ks);
	tds_des_ecb_encrypt(challenge, 8, &ks, &answer[8]);

	tds_convert_key(&hash[14], &ks);
	tds_des_ecb_encrypt(challenge, 8, &ks, &answer[16]);

	memset(&ks, 0, sizeof(ks));
}


/*
* turns a 56 bit key into the 64 bit, odd parity key and sets the key.
* The key schedule ks is also set.
*/
static void
tds_convert_key(const unsigned char *key_56, DES_KEY * ks)
{
	des_cblock key;

	key[0] = key_56[0];
	key[1] = ((key_56[0] << 7) & 0xFF) | (key_56[1] >> 1);
	key[2] = ((key_56[1] << 6) & 0xFF) | (key_56[2] >> 2);
	key[3] = ((key_56[2] << 5) & 0xFF) | (key_56[3] >> 3);
	key[4] = ((key_56[3] << 4) & 0xFF) | (key_56[4] >> 4);
	key[5] = ((key_56[4] << 3) & 0xFF) | (key_56[5] >> 5);
	key[6] = ((key_56[5] << 2) & 0xFF) | (key_56[6] >> 6);
	key[7] = (key_56[6] << 1) & 0xFF;

	tds_des_set_odd_parity(key);
	tds_des_set_key(ks, key, sizeof(key));

	memset(&key, 0, sizeof(key));
}

static TDSRET
tds7_send_auth(TDSSOCKET * tds,
	       const unsigned char *challenge, TDS_UINT flags, const unsigned char *names_blob, TDS_INT names_blob_len)
{
	size_t current_pos;
	TDSANSWER answer;

	/* FIXME: stuff duplicate in tds7_send_login */
	const char *domain;
	const char *user_name;
	const char *p;
	size_t user_name_len, host_name_len, domain_len;
	TDSRET rc;

	unsigned char *ntlm_v2_response = NULL;
	unsigned int ntlm_response_len = 24;
	const unsigned int lm_response_len = 24;

	TDSLOGIN *login = tds->login;

	/* check connection */
	if (!login)
		return TDS_FAIL;

	/* parse a bit of config */
	user_name = tds_dstr_cstr(&login->user_name);
	host_name_len = tds_dstr_len(&login->client_host_name);

	/* parse domain\username */
	if ((p = strchr(user_name, '\\')) == NULL)
		return TDS_FAIL;

	domain = user_name;
	domain_len = p - user_name;

	user_name = p + 1;
	user_name_len = strlen(user_name);

	rc = tds_answer_challenge(tds, login, challenge, &flags, names_blob, names_blob_len, &answer, &ntlm_v2_response);
	if (TDS_FAILED(rc))
		return rc;

	ntlm_response_len = ntlm_v2_response ? 16 + names_blob_len : 24;
	/* ntlm_response_len = 0; */

	tds->out_flag = TDS7_AUTH;
	tds_put_n(tds, "NTLMSSP", 8);
	tds_put_int(tds, 3);	/* sequence 3 */

	/* FIXME *2 work only for single byte encodings */
	current_pos = 64u + (domain_len + user_name_len + host_name_len) * 2u;

	/* LM/LMv2 Response */
	tds_put_smallint(tds, lm_response_len);	/* lan man resp length */
	tds_put_smallint(tds, lm_response_len);	/* lan man resp length */
	TDS_PUT_INT(tds, current_pos);	/* resp offset */
	current_pos += lm_response_len;

	/* NTLM/NTLMv2 Response */
	tds_put_smallint(tds, ntlm_response_len);	/* nt resp length */
	tds_put_smallint(tds, ntlm_response_len);	/* nt resp length */
	TDS_PUT_INT(tds, current_pos);	/* nt resp offset */

	current_pos = 64;

	/* Target Name - domain or server name */
	TDS_PUT_SMALLINT(tds, domain_len * 2);
	TDS_PUT_SMALLINT(tds, domain_len * 2);
	TDS_PUT_INT(tds, current_pos);
	current_pos += domain_len * 2u;

	/* username */
	TDS_PUT_SMALLINT(tds, user_name_len * 2);
	TDS_PUT_SMALLINT(tds, user_name_len * 2);
	TDS_PUT_INT(tds, current_pos);
	current_pos += user_name_len * 2u;

	/* Workstation Name */
	TDS_PUT_SMALLINT(tds, host_name_len * 2);
	TDS_PUT_SMALLINT(tds, host_name_len * 2);
	TDS_PUT_INT(tds, current_pos);
	current_pos += host_name_len * 2u;

	/* Session Key (optional) */
	tds_put_smallint(tds, 0);
	tds_put_smallint(tds, 0);
	TDS_PUT_INT(tds, current_pos + lm_response_len + ntlm_response_len);

	/* flags */
	/* "challenge" is 8 bytes long */
	/* tds_answer_challenge(tds_dstr_cstr(&login->password), challenge, &flags, &answer); */
	tds_put_int(tds, flags);

	/* OS Version Structure (Optional) */

	/* Data itself */
	tds_put_string(tds, domain, (int)domain_len);
	tds_put_string(tds, user_name, (int)user_name_len);
	tds_put_string(tds, tds_dstr_cstr(&login->client_host_name), (int)host_name_len);

	/* data block */
	tds_put_n(tds, answer.lm_resp, lm_response_len);

	if (ntlm_v2_response == NULL) {
		/* NTLMv1 */
		tds_put_n(tds, answer.nt_resp, ntlm_response_len);
	} else {
		/* NTLMv2 */
		tds_put_n(tds, ntlm_v2_response, ntlm_response_len);
		memset(ntlm_v2_response, 0, ntlm_response_len);
		free(ntlm_v2_response);
	}

	/* for security reason clear structure */
	memset(&answer, 0, sizeof(TDSANSWER));

	return tds_flush_packet(tds);
}

typedef struct tds_ntlm_auth
{
	TDSAUTHENTICATION tds_auth;
} TDSNTLMAUTH;

static TDSRET
tds_ntlm_free(TDSCONNECTION * conn, TDSAUTHENTICATION * tds_auth)
{
	TDSNTLMAUTH *auth = (TDSNTLMAUTH *) tds_auth;

	free(auth->tds_auth.packet);
	free(auth);

	return TDS_SUCCESS;
}

static const unsigned char ntlm_id[] = "NTLMSSP";

/**
 * put a 8 byte filetime from a time_t
 * This takes GMT as input
 */
static void
unix_to_nt_time(TDS_UINT8 * nt, time_t t)
{
#define TIME_FIXUP_CONSTANT (((TDS_UINT8) 134774U) * 86400U)

	TDS_UINT8 t2;

	if (t == (time_t) - 1) {
		*nt = (TDS_UINT8) - ((TDS_INT8) 1);
		return;
	}
	if (t == 0) {
		*nt = 0;
		return;
	}

	t2 = t;
	t2 += TIME_FIXUP_CONSTANT;
	t2 *= 1000 * 1000 * 10;

	*nt = t2;
}

static void
fill_names_blob_prefix(names_blob_prefix_t * prefix)
{
	TDS_UINT8 nttime = 0;

	/* TODO use more precision, not only seconds */
	unix_to_nt_time(&nttime, time(NULL));

	prefix->response_type = 0x01;
	prefix->max_response_type = 0x01;
	prefix->reserved1 = 0x0000;
	prefix->reserved2 = 0x00000000;
#ifdef WORDS_BIGENDIAN
	tds_swap_bytes((unsigned char *) &nttime, 8);
#endif
	prefix->timestamp = nttime;
	generate_random_buffer(prefix->challenge, sizeof(prefix->challenge));

	prefix->unknown = 0x00000000;
}

static TDSRET
tds_ntlm_handle_next(TDSSOCKET * tds, struct tds_authentication * auth, size_t len)
{
	const int length = (int)len;
	unsigned char nonce[8];
	TDS_UINT flags;
	int where;

	int data_block_offset;

	int names_blob_len = 0;
	unsigned char *names_blob = NULL;

	TDSRET rc;

	/* at least 32 bytes (till context) */
	if (len < 32)
		return TDS_FAIL;

	tds_get_n(tds, nonce, 8);	/* NTLMSSP\0 */
	if (memcmp(nonce, ntlm_id, 8) != 0)
		return TDS_FAIL;
	if (tds_get_int(tds) != 2)	/* sequence -> 2 */
		return TDS_FAIL;
	tds_get_smallint(tds);	/* domain len */
	tds_get_smallint(tds);	/* domain len */
	data_block_offset = tds_get_int(tds);	/* domain offset */
	flags = tds_get_int(tds);	/* flags */
	tds_get_n(tds, nonce, 8);
	tdsdump_dump_buf(TDS_DBG_INFO1, "TDS_AUTH_TOKEN nonce", nonce, 8);
	where = 32;

	/*data_block_offset == 32 */
	/* Version 1 -- The Context, Target Information, and OS Version structure are all omitted */

	if (data_block_offset >= 48 && where + 16 <= length) {
		int target_info_len, target_info_offset;

		/* Version 2 -- The Context and Target Information fields are present, but the OS Version structure is not. */
		tds_get_n(tds, NULL, 8);	/* Context (two consecutive longs) */

		target_info_len = tds_get_smallint(tds);	/* Target Information len */
		target_info_len = tds_get_smallint(tds);	/* Target Information len */
		target_info_offset = tds_get_int(tds);	/* Target Information offset */

		where += 16;

		if (data_block_offset >= 56 && where + 8 <= length) {
			/* Version 3 -- The Context, Target Information, and OS Version structure are all present. */
			tds_get_n(tds, NULL, 8);	/* OS Version Structure */
#if 0
			/* if we have a version server handle NTLMv2 */
			if (target_info_len > 0)
				flags &= ~0x80000;
#endif
			where += 8;
		}

		/* read Target Info if possible */
		if (target_info_len > 0 && target_info_offset >= where && target_info_offset + target_info_len <= length) {
			tds_get_n(tds, NULL, target_info_offset - where);
			where = target_info_offset;

			/*
			 * the + 4 came from blob structure, after Target Info 4
			 * additional reserved bytes must be present
			 * Search "davenport port"
			 * (currently http://davenport.sourceforge.net/ntlm.html)
			 */
			names_blob_len = TDS_OFFSET(names_blob_prefix_t, target_info) + target_info_len + 4;

			/* read Target Info */
			names_blob = (unsigned char *) calloc(names_blob_len, 1);
			if (!names_blob)
				return TDS_FAIL;

			fill_names_blob_prefix((names_blob_prefix_t *) names_blob);
			tds_get_n(tds, names_blob + TDS_OFFSET(names_blob_prefix_t, target_info), target_info_len);
			where += target_info_len;
		}
	}
	/* discard anything left */
	tds_get_n(tds, NULL, length - where);
	tdsdump_log(TDS_DBG_INFO1, "Draining %d bytes\n", (int) (len - where));

	rc = tds7_send_auth(tds, nonce, flags, names_blob, names_blob_len);

	free(names_blob);

	return rc;
}

/**
 * Build a NTLMSPP packet to send to server
 * @param tds     A pointer to the TDSSOCKET structure managing a client/server operation.
 * @return authentication info
 */
TDSAUTHENTICATION * 
tds_ntlm_get_auth(TDSSOCKET * tds)
{
	const char *domain;
	const char *user_name;
	const char *p;
	TDS_UCHAR *packet;
	int host_name_len;
	int domain_len;
	int auth_len;
	struct tds_ntlm_auth *auth;

	if (!tds->login)
		return NULL;

	user_name = tds_dstr_cstr(&tds->login->user_name);
	host_name_len = (int)tds_dstr_len(&tds->login->client_host_name);

	/* check override of domain */
	if ((p = strchr(user_name, '\\')) == NULL)
		return NULL;

	domain = user_name;
	domain_len = (int)(p - user_name);

	auth = (struct tds_ntlm_auth *) calloc(1, sizeof(struct tds_ntlm_auth));

	if (!auth)
		return NULL;

	auth->tds_auth.free = tds_ntlm_free;
	auth->tds_auth.handle_next = tds_ntlm_handle_next;

	auth->tds_auth.packet_len = auth_len = 40 + host_name_len + domain_len;
	auth->tds_auth.packet = packet = (TDS_UCHAR*) malloc(auth_len);
	if (!packet) {
		free(auth);
		return NULL;
	}

	/* built NTLMSSP authentication packet */
	memcpy(packet, ntlm_id, 8);
	/* sequence 1 client -> server */
	TDS_PUT_A4(packet + 8, TDS_HOST4LE(1));
	/* flags */
	TDS_PUT_A4(packet + 12, TDS_HOST4LE(0x08b201));

	/* domain info */
	TDS_PUT_A2LE(packet + 16, domain_len);
	TDS_PUT_A2LE(packet + 18, domain_len);
	TDS_PUT_A4LE(packet + 20, 40 + host_name_len);

	/* hostname info */
	TDS_PUT_A2LE(packet + 24, host_name_len);
	TDS_PUT_A2LE(packet + 26, host_name_len);
	TDS_PUT_A4  (packet + 28, TDS_HOST4LE(40));

	/*
	 * here XP put version like 05 01 28 0a (5.1.2600),
	 * similar to GetVersion result
	 * and some unknown bytes like 00 00 00 0f
	 */
	TDS_PUT_A4(packet + 32, TDS_HOST4LE(0x0a280105));
	TDS_PUT_A4(packet + 36, TDS_HOST4LE(0x0f000000));

	/* hostname and domain */
	memcpy(packet + 40, tds_dstr_cstr(&tds->login->client_host_name), host_name_len);
	memcpy(packet + 40 + host_name_len, domain, domain_len);

	return (TDSAUTHENTICATION *) auth;
}

/** @} */

