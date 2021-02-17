/*
 * internal.c
 *		Wrapper for builtin functions
 *
 * Copyright (c) 2001 Marko Kreen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * contrib/pgcrypto/internal.c
 */

#include "postgres.h"

#include <time.h>

#include "px.h"
#include "md5.h"
#include "sha1.h"
#include "sha2.h"
#include "blf.h"
#include "rijndael.h"
#include "fortuna.h"

/*
 * System reseeds should be separated at least this much.
 */
#define SYSTEM_RESEED_MIN			(20*60)		/* 20 min */
/*
 * How often to roll dice.
 */
#define SYSTEM_RESEED_CHECK_TIME	(10*60)		/* 10 min */
/*
 * The chance is x/256 that the reseed happens.
 */
#define SYSTEM_RESEED_CHANCE		(4) /* 256/4 * 10min ~ 10h */

/*
 * If this much time has passed, force reseed.
 */
#define SYSTEM_RESEED_MAX			(12*60*60)	/* 12h */


#ifndef MD5_DIGEST_LENGTH
#define MD5_DIGEST_LENGTH 16
#endif

#ifndef SHA1_DIGEST_LENGTH
#ifdef SHA1_RESULTLEN
#define SHA1_DIGEST_LENGTH SHA1_RESULTLEN
#else
#define SHA1_DIGEST_LENGTH 20
#endif
#endif

#define SHA1_BLOCK_SIZE 64
#define MD5_BLOCK_SIZE 64

static void init_md5(PX_MD *h);
static void init_sha1(PX_MD *h);

void		init_sha224(PX_MD *h);
void		init_sha256(PX_MD *h);
void		init_sha384(PX_MD *h);
void		init_sha512(PX_MD *h);

struct int_digest
{
	char	   *name;
	void		(*init) (PX_MD *h);
};

static const struct int_digest
			int_digest_list[] = {
	{"md5", init_md5},
	{"sha1", init_sha1},
	{"sha224", init_sha224},
	{"sha256", init_sha256},
	{"sha384", init_sha384},
	{"sha512", init_sha512},
	{NULL, NULL}
};

/* MD5 */

static unsigned
int_md5_len(PX_MD *h)
{
	return MD5_DIGEST_LENGTH;
}

static unsigned
int_md5_block_len(PX_MD *h)
{
	return MD5_BLOCK_SIZE;
}

static void
int_md5_update(PX_MD *h, const uint8 *data, unsigned dlen)
{
	MD5_CTX    *ctx = (MD5_CTX *) h->p.ptr;

	MD5Update(ctx, data, dlen);
}

static void
int_md5_reset(PX_MD *h)
{
	MD5_CTX    *ctx = (MD5_CTX *) h->p.ptr;

	MD5Init(ctx);
}

static void
int_md5_finish(PX_MD *h, uint8 *dst)
{
	MD5_CTX    *ctx = (MD5_CTX *) h->p.ptr;

	MD5Final(dst, ctx);
}

static void
int_md5_free(PX_MD *h)
{
	MD5_CTX    *ctx = (MD5_CTX *) h->p.ptr;

	memset(ctx, 0, sizeof(*ctx));
	px_free(ctx);
	px_free(h);
}

/* SHA1 */

static unsigned
int_sha1_len(PX_MD *h)
{
	return SHA1_DIGEST_LENGTH;
}

static unsigned
int_sha1_block_len(PX_MD *h)
{
	return SHA1_BLOCK_SIZE;
}

static void
int_sha1_update(PX_MD *h, const uint8 *data, unsigned dlen)
{
	SHA1_CTX   *ctx = (SHA1_CTX *) h->p.ptr;

	SHA1Update(ctx, data, dlen);
}

static void
int_sha1_reset(PX_MD *h)
{
	SHA1_CTX   *ctx = (SHA1_CTX *) h->p.ptr;

	SHA1Init(ctx);
}

static void
int_sha1_finish(PX_MD *h, uint8 *dst)
{
	SHA1_CTX   *ctx = (SHA1_CTX *) h->p.ptr;

	SHA1Final(dst, ctx);
}

static void
int_sha1_free(PX_MD *h)
{
	SHA1_CTX   *ctx = (SHA1_CTX *) h->p.ptr;

	memset(ctx, 0, sizeof(*ctx));
	px_free(ctx);
	px_free(h);
}

/* init functions */

static void
init_md5(PX_MD *md)
{
	MD5_CTX    *ctx;

	ctx = px_alloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));

	md->p.ptr = ctx;

	md->result_size = int_md5_len;
	md->block_size = int_md5_block_len;
	md->reset = int_md5_reset;
	md->update = int_md5_update;
	md->finish = int_md5_finish;
	md->free = int_md5_free;

	md->reset(md);
}

static void
init_sha1(PX_MD *md)
{
	SHA1_CTX   *ctx;

	ctx = px_alloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));

	md->p.ptr = ctx;

	md->result_size = int_sha1_len;
	md->block_size = int_sha1_block_len;
	md->reset = int_sha1_reset;
	md->update = int_sha1_update;
	md->finish = int_sha1_finish;
	md->free = int_sha1_free;

	md->reset(md);
}

/*
 * ciphers generally
 */

#define INT_MAX_KEY		(512/8)
#define INT_MAX_IV		(128/8)

struct int_ctx
{
	uint8		keybuf[INT_MAX_KEY];
	uint8		iv[INT_MAX_IV];
	union
	{
		BlowfishContext bf;
		rijndael_ctx rj;
	}			ctx;
	unsigned	keylen;
	int			is_init;
	int			mode;
};

static void
intctx_free(PX_Cipher *c)
{
	struct int_ctx *cx = (struct int_ctx *) c->ptr;

	if (cx)
	{
		memset(cx, 0, sizeof *cx);
		px_free(cx);
	}
	px_free(c);
}

/*
 * AES/rijndael
 */

#define MODE_ECB 0
#define MODE_CBC 1

static unsigned
rj_block_size(PX_Cipher *c)
{
	return 128 / 8;
}

static unsigned
rj_key_size(PX_Cipher *c)
{
	return 256 / 8;
}

static unsigned
rj_iv_size(PX_Cipher *c)
{
	return 128 / 8;
}

static int
rj_init(PX_Cipher *c, const uint8 *key, unsigned klen, const uint8 *iv)
{
	struct int_ctx *cx = (struct int_ctx *) c->ptr;

	if (klen <= 128 / 8)
		cx->keylen = 128 / 8;
	else if (klen <= 192 / 8)
		cx->keylen = 192 / 8;
	else if (klen <= 256 / 8)
		cx->keylen = 256 / 8;
	else
		return PXE_KEY_TOO_BIG;

	memcpy(&cx->keybuf, key, klen);

	if (iv)
		memcpy(cx->iv, iv, 128 / 8);

	return 0;
}

static int
rj_real_init(struct int_ctx * cx, int dir)
{
	aes_set_key(&cx->ctx.rj, cx->keybuf, cx->keylen * 8, dir);
	return 0;
}

static int
rj_encrypt(PX_Cipher *c, const uint8 *data, unsigned dlen, uint8 *res)
{
	struct int_ctx *cx = (struct int_ctx *) c->ptr;

	if (!cx->is_init)
	{
		if (rj_real_init(cx, 1))
			return PXE_CIPHER_INIT;
	}

	if (dlen == 0)
		return 0;

	if (dlen & 15)
		return PXE_NOTBLOCKSIZE;

	memcpy(res, data, dlen);

	if (cx->mode == MODE_CBC)
	{
		aes_cbc_encrypt(&cx->ctx.rj, cx->iv, res, dlen);
		memcpy(cx->iv, res + dlen - 16, 16);
	}
	else
		aes_ecb_encrypt(&cx->ctx.rj, res, dlen);

	return 0;
}

static int
rj_decrypt(PX_Cipher *c, const uint8 *data, unsigned dlen, uint8 *res)
{
	struct int_ctx *cx = (struct int_ctx *) c->ptr;

	if (!cx->is_init)
		if (rj_real_init(cx, 0))
			return PXE_CIPHER_INIT;

	if (dlen == 0)
		return 0;

	if (dlen & 15)
		return PXE_NOTBLOCKSIZE;

	memcpy(res, data, dlen);

	if (cx->mode == MODE_CBC)
	{
		aes_cbc_decrypt(&cx->ctx.rj, cx->iv, res, dlen);
		memcpy(cx->iv, data + dlen - 16, 16);
	}
	else
		aes_ecb_decrypt(&cx->ctx.rj, res, dlen);

	return 0;
}

/*
 * initializers
 */

static PX_Cipher *
rj_load(int mode)
{
	PX_Cipher  *c;
	struct int_ctx *cx;

	c = px_alloc(sizeof *c);
	memset(c, 0, sizeof *c);

	c->block_size = rj_block_size;
	c->key_size = rj_key_size;
	c->iv_size = rj_iv_size;
	c->init = rj_init;
	c->encrypt = rj_encrypt;
	c->decrypt = rj_decrypt;
	c->free = intctx_free;

	cx = px_alloc(sizeof *cx);
	memset(cx, 0, sizeof *cx);
	cx->mode = mode;

	c->ptr = cx;
	return c;
}

/*
 * blowfish
 */

static unsigned
bf_block_size(PX_Cipher *c)
{
	return 8;
}

static unsigned
bf_key_size(PX_Cipher *c)
{
	return 448 / 8;
}

static unsigned
bf_iv_size(PX_Cipher *c)
{
	return 8;
}

static int
bf_init(PX_Cipher *c, const uint8 *key, unsigned klen, const uint8 *iv)
{
	struct int_ctx *cx = (struct int_ctx *) c->ptr;

	blowfish_setkey(&cx->ctx.bf, key, klen);
	if (iv)
		blowfish_setiv(&cx->ctx.bf, iv);

	return 0;
}

static int
bf_encrypt(PX_Cipher *c, const uint8 *data, unsigned dlen, uint8 *res)
{
	struct int_ctx *cx = (struct int_ctx *) c->ptr;
	BlowfishContext *bfctx = &cx->ctx.bf;

	if (dlen == 0)
		return 0;

	if (dlen & 7)
		return PXE_NOTBLOCKSIZE;

	memcpy(res, data, dlen);
	switch (cx->mode)
	{
		case MODE_ECB:
			blowfish_encrypt_ecb(res, dlen, bfctx);
			break;
		case MODE_CBC:
			blowfish_encrypt_cbc(res, dlen, bfctx);
			break;
	}
	return 0;
}

static int
bf_decrypt(PX_Cipher *c, const uint8 *data, unsigned dlen, uint8 *res)
{
	struct int_ctx *cx = (struct int_ctx *) c->ptr;
	BlowfishContext *bfctx = &cx->ctx.bf;

	if (dlen == 0)
		return 0;

	if (dlen & 7)
		return PXE_NOTBLOCKSIZE;

	memcpy(res, data, dlen);
	switch (cx->mode)
	{
		case MODE_ECB:
			blowfish_decrypt_ecb(res, dlen, bfctx);
			break;
		case MODE_CBC:
			blowfish_decrypt_cbc(res, dlen, bfctx);
			break;
	}
	return 0;
}

static PX_Cipher *
bf_load(int mode)
{
	PX_Cipher  *c;
	struct int_ctx *cx;

	c = px_alloc(sizeof *c);
	memset(c, 0, sizeof *c);

	c->block_size = bf_block_size;
	c->key_size = bf_key_size;
	c->iv_size = bf_iv_size;
	c->init = bf_init;
	c->encrypt = bf_encrypt;
	c->decrypt = bf_decrypt;
	c->free = intctx_free;

	cx = px_alloc(sizeof *cx);
	memset(cx, 0, sizeof *cx);
	cx->mode = mode;
	c->ptr = cx;
	return c;
}

/* ciphers */

static PX_Cipher *
rj_128_ecb(void)
{
	return rj_load(MODE_ECB);
}

static PX_Cipher *
rj_128_cbc(void)
{
	return rj_load(MODE_CBC);
}

static PX_Cipher *
bf_ecb_load(void)
{
	return bf_load(MODE_ECB);
}

static PX_Cipher *
bf_cbc_load(void)
{
	return bf_load(MODE_CBC);
}

struct int_cipher
{
	char	   *name;
	PX_Cipher  *(*load) (void);
};

static const struct int_cipher
			int_ciphers[] = {
	{"bf-cbc", bf_cbc_load},
	{"bf-ecb", bf_ecb_load},
	{"aes-128-cbc", rj_128_cbc},
	{"aes-128-ecb", rj_128_ecb},
	{NULL, NULL}
};

static const PX_Alias int_aliases[] = {
	{"bf", "bf-cbc"},
	{"blowfish", "bf-cbc"},
	{"aes", "aes-128-cbc"},
	{"aes-ecb", "aes-128-ecb"},
	{"aes-cbc", "aes-128-cbc"},
	{"aes-128", "aes-128-cbc"},
	{"rijndael", "aes-128-cbc"},
	{"rijndael-128", "aes-128-cbc"},
	{NULL, NULL}
};

/* PUBLIC FUNCTIONS */

int
px_find_digest(const char *name, PX_MD **res)
{
	const struct int_digest *p;
	PX_MD	   *h;

	for (p = int_digest_list; p->name; p++)
		if (pg_strcasecmp(p->name, name) == 0)
		{
			h = px_alloc(sizeof(*h));
			p->init(h);

			*res = h;

			return 0;
		}
	return PXE_NO_HASH;
}

int
px_find_cipher(const char *name, PX_Cipher **res)
{
	int			i;
	PX_Cipher  *c = NULL;

	name = px_resolve_alias(int_aliases, name);

	for (i = 0; int_ciphers[i].name; i++)
		if (!strcmp(int_ciphers[i].name, name))
		{
			c = int_ciphers[i].load();
			break;
		}

	if (c == NULL)
		return PXE_NO_CIPHER;

	*res = c;
	return 0;
}

/*
 * Randomness provider
 */

/*
 * Use always strong randomness.
 */
int
px_get_pseudo_random_bytes(uint8 *dst, unsigned count)
{
	return px_get_random_bytes(dst, count);
}

static time_t seed_time = 0;
static time_t check_time = 0;

static void
system_reseed(void)
{
	uint8		buf[1024];
	int			n;
	time_t		t;
	int			skip = 1;

	t = time(NULL);

	if (seed_time == 0)
		skip = 0;
	else if ((t - seed_time) < SYSTEM_RESEED_MIN)
		skip = 1;
	else if ((t - seed_time) > SYSTEM_RESEED_MAX)
		skip = 0;
	else if (check_time == 0 ||
			 (t - check_time) > SYSTEM_RESEED_CHECK_TIME)
	{
		check_time = t;

		/* roll dice */
		px_get_random_bytes(buf, 1);
		skip = buf[0] >= SYSTEM_RESEED_CHANCE;
	}
	/* clear 1 byte */
	memset(buf, 0, sizeof(buf));

	if (skip)
		return;

	n = px_acquire_system_randomness(buf);
	if (n > 0)
		fortuna_add_entropy(buf, n);

	seed_time = t;
	memset(buf, 0, sizeof(buf));
}

int
px_get_random_bytes(uint8 *dst, unsigned count)
{
	system_reseed();
	fortuna_get_bytes(count, dst);
	return 0;
}

int
px_add_entropy(const uint8 *data, unsigned count)
{
	system_reseed();
	fortuna_add_entropy(data, count);
	return 0;
}
