#ifndef DES_H
#define DES_H

/* $Id: des.h,v 1.13 2010-01-25 23:05:58 freddy77 Exp $ */

#if defined(__GNUC__) && __GNUC__ >= 4 && !defined(__MINGW32__)
#pragma GCC visibility push(hidden)
#endif

typedef unsigned char des_cblock[8];

typedef struct des_key
{
	unsigned char kn[16][8];
	TDS_UINT sp[8][64];
	unsigned char iperm[16][16][8];
	unsigned char fperm[16][16][8];
} DES_KEY;

void tds_des_set_odd_parity(des_cblock key);
int tds_des_ecb_encrypt(const void *plaintext, int len, DES_KEY * akey, des_cblock output);
int tds_des_set_key(DES_KEY * dkey, const des_cblock user_key, int len);
void tds_des_encrypt(DES_KEY * key, des_cblock block);
void _mcrypt_decrypt(DES_KEY * key, unsigned char *block);

#if defined(__GNUC__) && __GNUC__ >= 4 && !defined(__MINGW32__)
#pragma GCC visibility pop
#endif

#endif /* !DES_H */
