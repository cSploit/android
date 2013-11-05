/*
    NDS client for ncpfs
    Copyright (C) 1997  Arne de Bruijn
    Copyright (C) 1999, 2000  Petr Vandrovec

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    
    Revision history:
    
	0.00  1997			Arne de Bruijn
		Initial release.
					
	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Moved file to new API.
		Added NWIsDSServer, key storing	functions, NWDSAuthenticateConn.

	1.01  2000, April 15		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added new functions needed for NWDSGenerateObjectKeyPair.
		
	1.02  2000, April 23		Petr Vandrovec <vandrove@vc.cvut.cz>
		Moved nds_login to ds/setkeys.c, added __NWDSGetPrivateKey.

	1.03  2000, July 6		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added DCV_DEREF_ALIASES into authentication functions.
		
	1.04  2000, July 10		Gerhard Lausser <GerhardLausser@KirchPayTV.de>
	        Fixed strcpy_wc call in nds_login_auth.

	1.05  2001, March 9		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWIsDSServerW. For now ISO-8859-1 only.

	1.06  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Use correct parameter (nuint32) to GetLE32().

*/

#define RANDBUF  /* if defined: read random data once from /dev/urandom */
/*#define ERR_MSG*/ /* if defined: show error messages in nds_login_auth */
/*#define DEBUG_PRINT*/
/*#define FIND_ISR */ /* if defined: show reasons for -330 invalid response */

#define NCP_OBSOLETE

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ncp/ndslib.h>
#include <ncp/kernel/ipx.h>
#include <errno.h>
#include <sys/mman.h>
#include "ncplib_i.h"
#include "ndscrypt.h"
#include "nwnet_i.h"
#include "ndslib_i.h"

#define USUALS
typedef u_int32_t word32;
typedef u_int16_t word16;
typedef unsigned char boolean;

#include "mpilib.h"

#ifdef ERR_MSG
#include <stdio.h>

#include "private/libintl.h"
#define _(X) dgettext(PACKAGE, (X))
#define N_(X) (X)
#endif

#ifdef FIND_ISR
#include <stdio.h>
#define ISRPrint(X...) do { fprintf(stderr, __FILE__ ":%d: ", __LINE__); fprintf(stderr, X); } while (0)
#else
#define ISRPrint(X...) do {} while (0)
#endif

NWDSCCODE NWIsDSServer(NWCONN_HANDLE conn, char* treename) {
	char buf[128];
	size_t size;
	NWDSCCODE err;
	size_t namelen;
	
	err = ncp_send_nds(conn, 1, "\0\0\0", 3, buf, sizeof(buf), &size);
	if (err)
		return 0;
	if (size < 8)
		return 0;
	namelen = DVAL_LH(buf, 4);
	if (namelen > size - 8)
		return 0;
	if (namelen > MAX_TREE_NAME_CHARS + 1)
		return 0;
	if (buf[8 + namelen - 1])
		return 0;
	if (treename)
		memcpy(treename, buf + 8, namelen);
	return 1;	
}

NWDSCCODE NWIsDSServerW(NWCONN_HANDLE conn, wchar_t* treename) {
	char treechar[MAX_TREE_NAME_CHARS + 1];
	NWDSCCODE err;
	
	err = NWIsDSServer(conn, treechar);
	if (err && treename) {
		char* treesrc = treechar;
		do { } while ((*treename++ = *treesrc++ & 0xFF) != 0);
	}
	return err;
}

static NWDSCCODE nds_read_pk(NWDSContextHandle ctx, const wchar_t* objname, 
		Octet_String_T** post) {
	char attrname[256];
	char reply_b[DEFAULT_MESSAGE_LEN];
	Buf_T bufattrname;
	nuint32 iterhandle = NO_MORE_ITERATIONS;
	Buf_T replybuf;
	NWObjectCount cnt;
	wchar_t name[MAX_DN_CHARS+1];
	NWObjectCount valcnt;
	enum SYNTAX synt;
	size_t size;
	Octet_String_T* ost;
	NWDSCCODE err;
	
	NWDSSetupBuf(&bufattrname, attrname, sizeof(attrname));
	NWDSInitBuf(ctx, DSV_READ, &bufattrname);
	NWDSSetupBuf(&replybuf, reply_b, sizeof(reply_b));
	NWDSPutAttrName(ctx, &bufattrname, (const NWDSChar*)L"Public Key");
	err = NWDSRead(ctx, (const NWDSChar*)objname, DS_ATTRIBUTE_VALUES, 0, &bufattrname, &iterhandle, &replybuf);
	if (err)
		return err;
	err = NWDSGetAttrCount(ctx, &replybuf, &cnt);
	if (err)
		return err;
	if (cnt != 1) {
		ISRPrint("Attr count != 1 (%d)\n", cnt);
		return ERR_INVALID_SERVER_RESPONSE;
	}
	err = NWDSGetAttrName(ctx, &replybuf, (NWDSChar*)name, &valcnt, &synt);
	if (err)
		return err;
	if ((synt != SYN_OCTET_STRING) ||
	    (wcscmp(name, L"Public Key")) ||
	    (valcnt < 1)) {
	    	ISRPrint("Attr type wrong (synt=%d, valcnt=%d)\n",
	    		synt, valcnt);
	    	return ERR_INVALID_SERVER_RESPONSE;
	}
	err = NWDSComputeAttrValSize(ctx, &replybuf, SYN_OCTET_STRING, &size);
	if (err)
		return err;
	ost = (Octet_String_T*)malloc(size);
	if (!ost)
		return ENOMEM;
	err = NWDSGetAttrVal(ctx, &replybuf, SYN_OCTET_STRING, ost);
	if (err) {
		free(ost);
		return err;
	}
	*post = ost;
	return 0;
}

#ifdef RANDBUF
#define RANDBUFSIZE 1236  /* total size of all fillrandom's for login+auth */
static char global_randbuf[RANDBUFSIZE];
static char *g_rndp = global_randbuf + RANDBUFSIZE;

static ncpt_mutex_t randbuflock = NCPT_MUTEX_INITIALIZER;

static void fillrandom(char *buf, int buflen) {
    ncpt_mutex_lock(&randbuflock);
    do {
	int i;
	
        if (g_rndp == global_randbuf + RANDBUFSIZE) {
	    int fh;
	    
            if ((fh = open("/dev/urandom", O_RDONLY)) >=0) {
                read(fh, global_randbuf, RANDBUFSIZE);
                close(fh);
            } else {
                g_rndp = global_randbuf;
                while (g_rndp - global_randbuf < RANDBUFSIZE)
                    *(g_rndp++) = rand() / ((((unsigned)RAND_MAX)+255) / 256);
            }
            g_rndp = global_randbuf;
        }
        if ((i = RANDBUFSIZE - (g_rndp - global_randbuf)) > buflen) i = buflen;
        memcpy(buf, g_rndp, i);
        buf += i;
        g_rndp += i;
        buflen -= i;
    } while (buflen);
    ncpt_mutex_unlock(&randbuflock);
}
#else
static void fillrandom(char *buf, int buflen) {
    int fh;
    char *p;
    
    if (((fh = open("/dev/urandom", O_RDONLY)) >= 0) {
        read(fh, buf, buflen);
        close(fh);
    } else {
        p = buf;
        while (p - buf < buflen)
            *(p++) = rand() / ((((unsigned)RAND_MAX)+255) / 256);
    }
}
#endif

static int countbits_l(char *buf, int bufsize) {
    unsigned char b;

    while ((--bufsize) && (!buf[bufsize]));
    b = (unsigned char)buf[bufsize];
    bufsize <<= 3;
    while (b) {
        b >>= 2; bufsize++;
    }
    return bufsize;
}

static void copyfill(void *outbuf, int outsize, const void *inbuf, int insize) {
    if (outsize < insize) insize = outsize;
    memcpy(outbuf, inbuf, insize);
    memset((char *)outbuf + insize, 0, outsize - insize);
}

static char keyprefix[] = {1, 0, 0, 0, 3, 0, 1, 0}; 

static int initkey(const char *key, char **keyptr, size_t *keylen) { /* 1=ok, 0=err */
    if (!memcmp(key, keyprefix, 8)) {
        if (keylen) *keylen = WVAL_LH(key, 8);
        if (keyptr) *keyptr = (char*)key + 10;
        return 1;
    } else
        return 0;
}

static void clearkey(char *key) {
    char *keyptr;
    size_t keylen;
    if (initkey(key, &keyptr, &keylen))
        memset(key, 0, keylen + 10);
}

static int findchunk(const char *keyptr, int keylen, const char *chunk, 
 char **chunkptr) {
    const char *p;
    
    if ((p = keyptr)) {
        while (p - keyptr < keylen) {
            if ((p[0] != chunk[0]) || (p[1] != chunk[1]))
                p += 4 + (unsigned char)p[2] + (unsigned char)p[3];
            else {
                if (chunkptr) *chunkptr = (char*)p + 4;
                return (unsigned char)p[2] + (unsigned char)p[3];
            }
        }
    }
    if (chunkptr) *chunkptr = NULL;
    return 0;
}

#ifdef DEBUG_PRINT
static void printnumber(const char* m, const unit* p) {
	int i;
	
	printf("%s: ", m);
	for (i = global_precision; i--; ) {
		printf("%02X", p[i]);
		if ((i & 1) == 0)
			printf(" ");
	}
	printf("\n");
}

static void dumpkey(const nuint8 *key, size_t keylen) {
	while (keylen > 8 + 4) {
		size_t kl = WVAL_LH(key, 2);
		const nuint8 *k2;
		nuint16 type = WVAL_HL(key, 0);
		size_t kl2 = kl;
		printf("El %c%c(%u): ", key[0], key[1], kl);
		key += 4;
		keylen -= kl + 4;
		k2 = key + kl;
		while (kl--) {
			printf("%02X", key[kl]);
		}
		printf("\n");
		if (type == 0x4E4E) {
			unit yy[2000];
			unit xx[2000];
			
			memset(xx, 0, sizeof(xx));
			memcpy(xx, key, kl2);
			set_precision(bytes2units(kl2));
			mp_move_and_shift_left_bits(yy, xx, 8);
			mp_recip(xx, yy);
			printnumber("MyNI", xx);
		}
		key = k2;
	}
}
#endif

static int checkkey(const char *key) { /* 0 - wrong key, != 0 - key ok */
	char temp[8];
	char *keyptr, *p;
	size_t keylen;
    
	if ((initkey(key, &keyptr, &keylen)) && 
	    (findchunk(keyptr, keylen, "MA", &p))) {
#ifdef DEBUG_PRINT
		mpkey(keyptr, keylen);
#endif
		nwhash1init(temp, 8);
		nwhash1(temp, 8, key + 10, WVAL_LH(key, 8) - 20);
		return (!memcmp(p, temp, 8));
	}
	return 0;
}

static ncpt_mutex_t mpilock = NCPT_MUTEX_INITIALIZER;

static long modexpkey(const char *s_key, char *buf, char *outbuf, int bufsize) {
	char *s_keyptr;
	size_t s_keylen;
	int i, nbits, nblocksize;
	int err = -1;
	unitptr nmod, nexp, nin, nout;
	char *p;

	nmod = nexp = nin = nout = NULL;
    
	if (!initkey(s_key, &s_keyptr, &s_keylen)) {
    		ISRPrint("Initkey failed\n");
		return ERR_INVALID_SERVER_RESPONSE;
	}
	i = findchunk(s_keyptr, s_keylen, "NN", &p);
	if (!p) {
		ISRPrint("NN chunk not found\n");
		return ERR_INVALID_SERVER_RESPONSE;
	}
	nbits = countbits_l(p, i);
	nblocksize = ((nbits + 31) & (~31)) >> 3;
	if (!(nmod = malloc(nblocksize * 4)))
		return ENOMEM;
	copyfill(nmod, nblocksize, p, i);
	i = findchunk(s_keyptr, s_keylen, "EN", &p);
	err = ERR_INVALID_SERVER_RESPONSE;
	if (!p) {
		ISRPrint("EN chunk not found\n");
		goto end1;
	}
	nexp = (unitptr)(((char*)nmod) + nblocksize);
	nin = (unitptr)(((char*)nexp) + nblocksize);
	nout = (unitptr)(((char*)nin) + nblocksize);
	copyfill(nexp, nblocksize, p, i);
	copyfill(nin, nblocksize, buf, bufsize);
	ncpt_mutex_lock(&mpilock);
	set_precision(bytes2units(nblocksize));
	i = mp_modexp(nout, nin, nexp, nmod);
	ncpt_mutex_unlock(&mpilock);
	if (i) {
		ISRPrint("mp_modexp failed\n");
		err = ERR_INVALID_SERVER_RESPONSE;
	} else {
		copyfill(outbuf, bufsize, nout, nblocksize);
		err = 0;
	}
end1:    
	free(nmod);
	return err;
}

/* ctx must be in WCHAR_T mode, without DCV_CANONICALIZE_NAMES */
static NWDSCCODE get_public_key(NWDSContextHandle ctx, const wchar_t* objname, nuint8 **key) {
	char *keybuf, *kptr;
	NWDSCCODE err;
	size_t keylen, ofs, klen;
	Octet_String_T* ost;

	err = nds_read_pk(ctx, objname, &ost);
	if (err)
		return err;
	keybuf = ost->data;
	keylen = ost->length;
	ofs = WVAL_LH(keybuf, 10) + 0x1a;
	if ((ofs > keylen) || (!initkey(keybuf + ofs, &kptr, &klen)) || 
		(klen + ofs > keylen) || (!checkkey(keybuf + ofs))) {
		err = ERR_INVALID_SERVER_RESPONSE;
		ISRPrint("Wrong key\n");
		goto err_exit;
	}
	if (key) {
		if (!(kptr = malloc(klen + 10))) {
			err = ENOMEM;
			goto err_exit;
		}
		memcpy(kptr, keybuf + ofs, klen + 10);
		*key = kptr;
	}
	err = 0;
err_exit:
	free(ost);
	return err;
}

static NWDSCCODE __AlignAndEncryptBlockWithSK(
		const nuint8* hash, 
		const nuint8* idata, 
		size_t ilen, 
		char* odata, 
		size_t* olen
) {
	size_t	filler;

	memcpy(odata, idata, ilen);
	filler = 8 - (ilen + 5) % 8;
	memset(odata + ilen, filler, filler);
	ilen += filler;
	nwhash1init(odata + ilen, 5);
	nwhash1(odata + ilen, 5, odata, ilen);
	ilen += 5;
	nwencryptblock(hash, odata, ilen, odata);
	*olen = ilen;
	return 0;
}

NWDSCCODE __NWEncryptWithSK(
		const void* hashsrc,
		size_t hashsrclen,
		const void* idata,
		size_t ilen,
		void* odata,
		size_t* olen
) {
	int	err;
	size_t	len;
	nuint8	hash[8];
	int	i;

	if (!idata || !ilen || !odata || !olen)
		return ERR_NULL_POINTER;
	if (!hashsrc)
		hashsrclen = 0;
	nwhash1init(hash, 8);
	for (i = 0; i < 10; i++) {
		nwhash1(hash, 8, hashsrc, hashsrclen);
	}
	err = __AlignAndEncryptBlockWithSK(hash, idata, ilen, (char*)odata + 12, &len);
	if (err)
		return err;
	*olen = len + 12;
	DSET_LH(odata, 0, 1);
	WSET_LH(odata, 4, 1);
	WSET_LH(odata, 6, 6);
	WSET_LH(odata, 8, len);
	WSET_LH(odata, 10, ilen);
	return 0;
}

static const char buf2str1[8] =  { 1,0,0,0, 9,0, 2,0};
static const char buf2str2[16] = {65,0,0,0, 1,0,0,0, 1,0, 9,0, 53,0, 28,0};
NWDSCCODE rsa_crypt2(
		const nuint8* s_key,
		Buf_T* input,
		Buf_T* output) {
	char hashrand[8], temp[8];
	unsigned short cryptbuf[128];
	char buf2[56];
	NWDSCCODE err;
	void* data;
    	size_t datalen;
	nuint8* outp;
	char b2[28];
	nuint8* ln1;
	nuint8* sp1;
	size_t outlen;
	
	fillrandom(b2, 28);
	buf2[0] = 11;
	memcpy(buf2 + 1, b2, 28);
	memset(buf2 + 29, 11, 11);
	memset(buf2 + 40, 0, 16);
	
	nwhash1(buf2 + 40, 5, buf2 + 1, 39);
	nwhash1(buf2 + 45, 2, buf2, 45);
	fillrandom(buf2 + 47, 5);

	err = modexpkey(s_key, buf2, buf2, 56);
	if (err)
		return err;
	
	err = NWDSBufPut(output, buf2str1, sizeof(buf2str1));
	if (err)
		return err;
	sp1 = NWDSBufPutPtr(output, 4);
	if (!sp1)
		return ERR_BUFFER_FULL;
	err = NWDSBufPut(output, buf2str2, sizeof(buf2str2));
	if (err)
		return err;
	err = NWDSBufPut(output, buf2, 56);
	if (err)
		return err;
	memset(buf2, 0, sizeof(buf2));
	
	ln1 = NWDSBufPutPtr(output, 4);
	if (!ln1)
		return ERR_BUFFER_FULL;
	outp = NWDSBufPeekPtr(output);
	if (!outp)
		return ERR_BUFFER_FULL;
	data = NWDSBufRetrieve(input, &datalen);
	err = __NWEncryptWithSK(b2, 28, data, datalen, outp, &outlen);
	if (err)
		return err;
	DSET_LH(ln1, 0, outlen);
	NWDSBufPutSkip(output, outlen);
	DSET_LH(sp1, 0, outlen + 76);

	memset(hashrand, 0, sizeof(hashrand));
	memset(temp, 0, sizeof(temp));
	memset(cryptbuf, 0, sizeof(cryptbuf));
	return 0;
}

NWDSCCODE rsa_crypt(
		NWDSContextHandle ctx,
		const wchar_t* servername,
		Buf_T* input,
		Buf_T* output) {
	NWDSCCODE err;
	nuint8 *s_key;
	
	if ((err = get_public_key(ctx, servername, &s_key)))
		return err;
	err = rsa_crypt2(s_key, input, output);
	free(s_key);
	return err;
}

static NWDSCCODE NWDXSetKeysOnConns(
		NWDS_HANDLE dxh,
		const void* authinfo,
		size_t authinfo_len) {
	struct list_head* stop = &dxh->conns;
	struct list_head* current;
	
	for (current = stop->next; current != stop; current = current->next) {
		NWCONN_HANDLE conn = list_entry(current, struct ncp_conn, nds_ring);
	
		ncp_set_private_key(conn, authinfo, authinfo_len);
	}
	return 0;
}

static NWDSCCODE NWDXSetKeys(
		NWDS_HANDLE dxh,
		const nuint8 logindata[8],
		const wchar_t* username,
		const nuint8* pkey,
		size_t pkey_len) {
	union __NWDSAuthInfo* ndai;
	size_t slen;
	size_t tlen;
	size_t cpos;
	
	if (!dxh || !logindata || !username || !pkey)
		return ERR_NULL_POINTER;
	slen = (wcslen(username) + 1) * sizeof(wchar_t);
	cpos = ROUNDBUFF(sizeof(*ndai));
	tlen = cpos + ROUNDBUFF(slen) + ROUNDBUFF(pkey_len);
	ndai = (union __NWDSAuthInfo*)malloc(tlen);
	if (!ndai)
		return ENOMEM;
	ndai->header.total = tlen;
	ndai->header.version = 0;
	ndai->header.hdrlen = sizeof(*ndai);
	memcpy(ndai->header.logindata, logindata, 8);
	ndai->header.name_len = slen;
	memcpy(ndai->data + cpos, username, slen);
	ndai->header.privkey_len = pkey_len;
	cpos += ROUNDBUFF(slen);
	memcpy(ndai->data + cpos, pkey, pkey_len);
	/* ignore errors... there is nothing we can do around */
	mlock(ndai, tlen);
	NWDXSetKeysOnConns(dxh, ndai, tlen);
	if (dxh->authinfo) {
		tlen = dxh->authinfo->header.total;
		memset(dxh->authinfo, 0, tlen);
		munlock(dxh->authinfo, tlen);
		free(dxh->authinfo);
	}
	dxh->authinfo = ndai;
	return 0;
}

NWDSCCODE NWDSSetKeys(
		NWDSContextHandle ctx,
		const nuint8 logindata[8],
		const wchar_t* username,
		const nuint8* pkey,
		size_t pkey_len) {
	NWDSCCODE err;
	
	err = NWDSIsContextValid(ctx);
	if (err)
		return err;
	return NWDXSetKeys(ctx->ds_connection,
		logindata, username, pkey, pkey_len);
}

NWDSCCODE NWDSGetKeys(
		NWDSContextHandle ctx,
		union __NWDSAuthInfo** pndai,
		size_t* pndailen) {
	NWDS_HANDLE dxh;
	union __NWDSAuthInfo* ndai;
	
	if (!ctx || !pndai)
		return ERR_NULL_POINTER;
	dxh = ctx->ds_connection;
	if (!dxh)
		return ERR_NOT_LOGGED_IN;

	ndai = dxh->authinfo;
	if (!ndai) {
		/* OK, we do not have authentication info     */
		/* but maybe some permanent/shared connection */
		/* does have authentication information       */
		struct list_head* stop = &dxh->conns;
		struct list_head* current;
		size_t authinfo_len = 0;
		
		for (current = stop->next; current != stop; current = current->next) {
			NWDSCCODE err;
			NWCONN_HANDLE conn = list_entry(current, struct ncp_conn, nds_ring);
	
			err = ncp_get_private_key(conn, NULL, &authinfo_len);
			if (err)
				continue;	/* kernel without private key support */
			if (!authinfo_len)
				continue;	/* no private key on this connection */
			ndai = (union __NWDSAuthInfo*)malloc(authinfo_len);
			if (!ndai)
				continue;	/* not enough memory */
			err = ncp_get_private_key(conn, ndai, &authinfo_len);
			if (!err)
				break;		/* we have it */
			free(ndai);
			ndai = NULL;
		}
		if (!ndai)
			return ERR_NOT_LOGGED_IN;

		mlock(ndai, authinfo_len);
		NWDXSetKeysOnConns(dxh, ndai, authinfo_len);
		if (dxh->authinfo) {
			size_t tlen = dxh->authinfo->header.total;
			memset(dxh->authinfo, 0, tlen);
			munlock(dxh->authinfo, tlen);
			free(dxh->authinfo);
		}
		dxh->authinfo = ndai;
	}
	*pndai = ndai;
	*pndailen = ndai->header.total;
	return 0;
}

static NWDSCCODE NWDXUpdateKey(
		NWDS_HANDLE dxh,
		NWCONN_HANDLE conn
) {
	size_t authinfo_len;
	union __NWDSAuthInfo* ndai;
	NWDSCCODE err;

	if (conn->nds_conn != dxh) {
		return NWE_PARAM_INVALID;
	}
	if (dxh->authinfo) {
		return EBUSY;
	}
	err = ncp_get_private_key(conn, NULL, &authinfo_len);
	if (err)
		return err;	/* kernel without private key support */
	if (!authinfo_len)
		return ERR_NOT_LOGGED_IN;	/* no private key on this connection */
	ndai = (union __NWDSAuthInfo*)malloc(authinfo_len);
	if (!ndai)
		return ENOMEM;	/* not enough memory */
	err = ncp_get_private_key(conn, ndai, &authinfo_len);
	if (err) {
		free(ndai);
		return err;
	}
	mlock(ndai, authinfo_len);
	NWDXSetKeysOnConns(dxh, ndai, authinfo_len);
	if (dxh->authinfo) {
		size_t tlen = dxh->authinfo->header.total;
		memset(dxh->authinfo, 0, tlen);
		munlock(dxh->authinfo, tlen);
		free(dxh->authinfo);
	}
	dxh->authinfo = ndai;
	return 0;
}

/* FIXME: Internal only! */
static NWDSCCODE NWDXAddConnection(NWDS_HANDLE dsh, NWCONN_HANDLE conn) {
	ncpt_mutex_lock(&nds_ring_lock);
	list_del(&conn->nds_ring);
	conn->state++;
	list_add(&conn->nds_ring, &dsh->conns);
	conn->nds_conn = dsh;
	ncpt_mutex_unlock(&nds_ring_lock);
	if (!dsh->authinfo) {
		NWDXUpdateKey(dsh, conn);
	}
	return 0;
}

NWDSCCODE NWDSAddConnection(NWDSContextHandle ctx, NWCONN_HANDLE conn) {
	return NWDXAddConnection(ctx->ds_connection, conn);
}

#define xprintf(X...)

NWDSCCODE __NWDSGetPrivateKey(
		NWCONN_HANDLE	conn,
		const nuint8*	connPublicKey,
		const nuint8	rndseed[4],
		NWObjectID	objectID,
		const nuint8	pwd[16],
		nuint8		logindata[8],
		nuint8**	privateKey,
		size_t*		privateKeyLen
) {
	nuint8 crypt1strc[40];
	Buf_T cbuf;
	Buf_T rpb;
	Buf_T rqb;
	nuint8 tempbuf[1200];
	nuint8 rqb_b[1200];
	nuint8 rpb_b[1200];
	nuint8 logindata2[8];
	nuint8* p;
	size_t bl;
	NWDSCCODE err;
	NWDSCCODE grace_err;
	nuint32 len;
	size_t n2;
	size_t n3;
	size_t i;
	nuint8 temp[8];
	nuint8 hashshuf[8];

	bl = sizeof(crypt1strc);
	err = __NWEncryptWithSK(pwd, 16, rndseed, 4, crypt1strc, &bl);
	if (err)
		goto err_exit;

	NWDSSetupBuf(&cbuf, tempbuf, sizeof(tempbuf));
	p = NWDSBufPutPtr(&cbuf, 4);
	fillrandom(p, 4);
	NWDSBufPutLE32(&cbuf, 1024);
	p = NWDSBufPutPtr(&cbuf, 1024);
	fillrandom(p, 1024);
	NWDSBufPutBuffer(&cbuf, crypt1strc, bl);

	NWDSSetupBuf(&rqb, rqb_b, sizeof(rqb_b));
	NWDSInitBuf(NULL, DSV_FINISH_LOGIN, &rqb);

	err = rsa_crypt2(connPublicKey, &cbuf, &rqb);
	if (err)
		goto err_exit;
	
	NWDSSetupBuf(&rpb, rpb_b, sizeof(rpb_b));
	if (logindata)
		err = __NWDSFinishLoginV2(conn, 0, objectID, &rqb, logindata, &rpb);
	else
		err = __NWDSFinishLoginV2(conn, 1, objectID, &rqb, logindata2, &rpb);
	if ((err != 0) && (err != NWE_PASSWORD_EXPIRED))
		goto err_exit;
	grace_err = err;
	
	err = NWDSBufGetLE32(&rpb, &len);
	if (err)
		goto err_exit;
		
	xprintf("Want len %d\n", len);
	
	p = NWDSBufGetPtr(&rpb, len);
	if (!p) {
		err = ERR_BUFFER_EMPTY;
		goto err_exit;
	}
	err = ERR_INVALID_SERVER_RESPONSE;
	if (len < 12) {
		ISRPrint("Key shorter than 12 bytes (%d)\n", len);
		goto err_exit;
	}
		
	xprintf("%04X %04X %02X\n", DVAL_LH(p, 0), DVAL_LH(p, 4), WVAL_LH(p, 8));

	if ((DVAL_LH(p, 0) != 0x00000001) ||
	    (DVAL_LH(p, 4) != 0x00060001)) {
	    	ISRPrint("Invalid key start (%08X %08X %04X)\n",
	    		 DVAL_LH(p, 0), DVAL_LH(p, 4), WVAL_LH(p, 8));
	    	goto err_exit;
	}
	n3 = WVAL_LH(p, 8);
	if (len < n3 + 12) {
		ISRPrint("Invalid key len (%d < %d)\n", len, n3 + 12);
		goto err_exit;
	}
	p += 12;
	
	nwhash1init(temp, 8);
	for (i = 10; i; i--)
		nwhash1(temp, 8, crypt1strc, 28);
	nwdecryptblock(temp, p, n3, p);
	
	nwhash1init(temp, 5);
	nwhash1(temp, 5, p, n3 - 5);
	
	xprintf("G1\n");
	
	if (memcmp(temp, p + n3 - 5, 5)) {
		ISRPrint("Wrong hash\n");
		goto err_exit;
	}
	
	xprintf("G2\n");
		
	if (memcmp(p, tempbuf, 4)) {
		ISRPrint("Wrong hash #2\n");
		goto err_exit;
	}
	
	xprintf("G3\n");
		
	n2 = DVAL_LH(p, 4);
	p += 8;
	for (i = 0; i < n2; i++)
        	p[i] ^= tempbuf[i+8];
		
	xprintf("G4: %08X %08X\n", DVAL_LH(p, 0), DVAL_LH(p, 4));

	if ((DVAL_LH(p, 0) != 0x00000001) ||
	    (DVAL_LH(p, 4) != 0x00060001)) {
	    	ISRPrint("Invalid start #2: %08X %08X\n", DVAL_LH(p, 0),
	    		DVAL_LH(p, 4));
	    	goto err_exit;
	}
		
	n3 = WVAL_LH(p, 8);
	p += 12;

	nwhash1init(hashshuf, 8);
	for (i = 10; i; i--)
		nwhash1(hashshuf, 8, pwd, 16);

	nwdecryptblock(hashshuf, p, n3, p);
	
	xprintf("G5: %08X %08X\n", DVAL_LH(p, 0), DVAL_LH(p, 4));

	if ((DVAL_LH(p, 0) != 1) ||
	    (WVAL_LH(p, 4) != 2)) {
	    	ISRPrint("Invalid start #3: %08X %08X\n", DVAL_LH(p, 0),
	    		DVAL_LH(p, 4));
	    	goto err_exit;
	}
		
	n3 = WVAL_LH(p, 6);
	
	p += 8;

	{
		char* tb;
		if (!(tb = malloc(n3 + 10))) {
			err = ENOMEM;
			goto err_exit;
		}
		DSET_LH(tb, 0, 1);
		WSET_LH(tb, 4, 3);
		WSET_LH(tb, 6, 1);
		WSET_LH(tb, 8, n3);
		memcpy(tb + 10, p, n3);
		
		xprintf("G6\n");
		
		if (!checkkey(tb)) {
			free(tb);
			ISRPrint("Post check failed\n");
			goto err_exit;
	        }
		
		xprintf("G7\n");

		if (privateKeyLen)
			*privateKeyLen = n3 + 10;
		if (privateKey) {
			*privateKey = tb;
		} else {
			memset(tb, 0, n3 + 10);
			free(tb);
		}
	}
	err = grace_err;
err_exit:
	memset(logindata2, 0, sizeof(logindata2));
	memset(tempbuf, 0, sizeof(tempbuf));
	memset(rpb_b, 0, sizeof(rpb_b));
	memset(hashshuf, 0, sizeof(hashshuf));
	memset(crypt1strc, 0, sizeof(crypt1strc));
	memset(temp, 0, sizeof(temp));
	memset(rqb_b, 0, sizeof(rqb_b));
	memset(temp, 0, sizeof(temp));
	return err; 
}

static NWDSCCODE nds_beginauth2(
		NWCONN_HANDLE conn,
		NWObjectID user_id,
		nuint8 authid[4],
		const char* s_key) {
	char *p, *pend, *n_temp, temp[8], *k1end;
	char randno[4];
	NWDSCCODE err;
	int n1, n3;
	u_int16_t n3a;
	char rpb_b[DEFAULT_MESSAGE_LEN];
	Buf_T rpb;
    	size_t k1tl;
    
	n_temp = NULL;    
	fillrandom(randno, 4);        

	NWDSSetupBuf(&rpb, rpb_b, sizeof(rpb_b));
	err = __NWDSBeginAuthenticationV0(conn, user_id, randno, authid, &rpb);
	if (err)
		goto err_exit;
	err = ERR_INVALID_SERVER_RESPONSE;
	p = rpb.curPos;
	pend = rpb.dataend;
	if (pend - p < 12) {
		ISRPrint("Invalid len: %u < 12\n", pend - p);
		goto err_exit;
	}
	if ((DVAL_LH(p, 0) != 0x00000001) ||
	    (DVAL_LH(p, 4) != 0x00020009)) {
	    	ISRPrint("Invalid start: %08X %08X\n", DVAL_LH(p, 0),
	    		DVAL_LH(p, 4));
	    	goto err_exit;
	}
	n3 = DVAL_LH(p, 8);
	if (n3 < 16) {
		ISRPrint("Invalid len: %u < 16\n", n3);
		goto err_exit;
	}
	p += 12;
	if (pend - p < n3) {
		ISRPrint("Invalid len: %u < %u\n", pend - p, n3);
		goto err_exit;
	}
	pend = p + n3;
	k1tl = DVAL_LH(p, 0);
	if (k1tl < 12) {
		ISRPrint("K1Tl %u < 12\n", k1tl);
		goto err_exit;
	}
	p += 4;
	k1end = p + k1tl;
	if ((DVAL_LH(p, 0) != 0x00000001) ||
	    (DVAL_LH(p, 4) != 0x000A0001)) {
	    	ISRPrint("Invalid start: %08X %08X\n", DVAL_LH(p, 0),
	    		DVAL_LH(p, 4));
	    	goto err_exit;
	}
	n3a = WVAL_LH(p, 8);
	p += 12;
	if (k1end - p < n3a) {
		ISRPrint("Invalid len: %u < %u\n", k1end - p, n3a);
		goto err_exit;
	}
	n1 = ((countbits_l(p, n3a) + 31) & (~31)) >> 3;
	if (n1 < 52) {
		ISRPrint("Too short key: %u < 52\n", n1);
		goto err_exit;
	}
	if (!(n_temp = malloc(n1))) {
		err = ENOMEM;
		goto err_exit;
	}
	copyfill(n_temp, n1, p, n3a);
	p = (void*)(((unsigned long)k1end + 3) & ~3);
	err = modexpkey(s_key, n_temp, n_temp, n1);
	if (err) {
		ISRPrint("modexpkey failed\n");
		goto err_exit;
	}
	err = ERR_INVALID_SERVER_RESPONSE;
	nwhash1init(temp, 7);
	nwhash1(temp + 5, 2, n_temp, 45);
	nwhash1(temp, 5, n_temp + 1, 39);
	if (memcmp(temp, n_temp + 40, 7)) {
		ISRPrint("Invalid hash\n");
		goto err_exit;
	}
	nwhash1init(temp, 8);
	for (n1 = 10; n1; n1--)
		nwhash1(temp, 8, n_temp + 1, 28);
	free(n_temp); n_temp = NULL;
	if (pend - p < 16) {
		ISRPrint("Invalid len: %u < 16\n", pend - p);
    		goto err_exit;
    	}
	if ((DVAL_LH(p, 0) != 28) ||
	    (DVAL_LH(p, 4) != 0x00000001) ||
	    (DVAL_LH(p, 8) != 0x00060001) ||
	    (DVAL_LH(p, 12) != 0x00040010)) {
	    	ISRPrint("Invalid start: %u %08X %08X %08X\n", DVAL_LH(p, 0), 
	    		DVAL_LH(p, 4), DVAL_LH(p, 8), DVAL_LH(p, 12));
	    	goto err_exit;
	}
	p += 16;
	if (pend - p < 16) {
		ISRPrint("Invalid len: %u < 16\n", pend - p);
		goto err_exit;
	}
	nwdecryptblock(temp, p, 16, p);
	nwhash1init(temp, 5);
	nwhash1(temp, 5, p, 11);
	if ((!memcmp(temp, p + 11, 5)) || (!memcmp(p, randno, 4)))
		err = 0;
	else
		ISRPrint("Invalid hash\n");
err_exit:        
	if (n_temp) free(n_temp);
	return err;
}

static char *allocfillchunk(const char *keyptr, int keylen,
		const char *chunk, int destsize) {
	char *p, *p2;
	int i;

	i = findchunk(keyptr, keylen, chunk, &p);
	if (!p) 
		return NULL;
	if (!(p2 = malloc(destsize)))
		return NULL;
	copyfill(p2, destsize, p, i);
	return p2;    
}

static NWDSCCODE nds_beginauth(
		NWCONN_HANDLE conn,
		NWObjectID user_id,
		NWDSContextHandle ctx,
		const wchar_t* servername,
		nuint8 authid[4]) {
	NWDSCCODE err;
	nuint8* s_key;
	
	if ((err = get_public_key(ctx, servername, &s_key)))
		return err;
	err = nds_beginauth2(conn, user_id, authid, s_key);
	free(s_key);
	return err;
}

static NWDSCCODE gen_auth_data(
		Buf_T* outbuf, 
		const char *u_key,
		const char *u_priv_key,
		const nuint8* authid,
		char *loginstrc,
		int loginstrc_len) {
	char *keyptr;
	size_t keylen;
	int i, j;
	int nbits, nblocksize, nbytes, nblocksize_nw;
	unsigned char nmask;
	unitptr n_mod, n_exp, n_pn, n_qn, n_dp, n_dq, n_cr, n_key, n_temp;
	unitptr n_key_dp, n_key_dq;
	unitptr up, up2;
	unitptr randbuf;
	char *p;
	char *tempbuf;
	char hashbuf[0x42];
	NWDSCCODE err;
    
	n_temp = n_mod = n_exp = n_pn = n_qn = n_dp = n_dq = n_cr = n_key = NULL;
	if (!initkey(u_key, &keyptr, &keylen)) {
		ISRPrint("Initkey failed\n");
		return ERR_INVALID_SERVER_RESPONSE;
	}
	i = findchunk(keyptr, keylen, "NN", &p);
	if (!p) {
		ISRPrint("NN chunk not found\n");
		return ERR_INVALID_SERVER_RESPONSE;
	}
	nbits = countbits_l(p, i);
	nbytes = (nbits + 7) >> 3;
	nmask = (unsigned char)(255 >> (8 - (nbits & 7)));
	/* we really want (x + 31) & ~15... I'm sorry, but Novell thinks that way */
	nblocksize_nw = ((nbits + 31) & (~15)) >> 3;

	ncpt_mutex_lock(&mpilock);
	set_precision(bytes2units(nblocksize_nw));
	nblocksize = units2bytes(global_precision);
   
	n_mod = (unitptr)allocfillchunk(keyptr, keylen, "NN", nblocksize);
	n_exp = (unitptr)allocfillchunk(keyptr, keylen, "EN", nblocksize);
	if (!initkey(u_priv_key, &keyptr, &keylen)) {
		err = ERR_INVALID_SERVER_RESPONSE;
		ISRPrint("initkey failed\n");
		goto err_exit;
	}
	n_pn = (unitptr)allocfillchunk(keyptr, keylen, "PN", nblocksize);
	n_qn = (unitptr)allocfillchunk(keyptr, keylen, "QN", nblocksize);
	n_dp = (unitptr)allocfillchunk(keyptr, keylen, "DP", nblocksize);
	n_dq = (unitptr)allocfillchunk(keyptr, keylen, "DQ", nblocksize);
	n_cr = (unitptr)allocfillchunk(keyptr, keylen, "CR", nblocksize);
	n_key = malloc(nblocksize);

	nwhash2init(hashbuf);
	nwhash2block(hashbuf, loginstrc, loginstrc_len);
	nwhash2end(hashbuf);
	copyfill(n_key, nblocksize, hashbuf, 16);

	if (!(tempbuf = malloc(loginstrc_len + 16))) {
		err = ENOMEM;
		goto err_exit;
	}
	DSET_LH(tempbuf, 0, 0);
	DSET_LH(tempbuf, 4, 0x3c);
	memcpy(tempbuf + 8, authid, 4);
	DSET_LH(tempbuf, 12, loginstrc_len);
	memcpy(tempbuf + 16, loginstrc, loginstrc_len);

	nwhash2init(hashbuf);
	nwhash2block(hashbuf, tempbuf, loginstrc_len + 16);
	free(tempbuf);

	n_temp = malloc(nblocksize * 6);
	n_key_dp = (unitptr)(((char*)n_temp) + nblocksize);
	n_key_dq = (unitptr)(((char*)n_key_dp) + nblocksize);
	mp_mult(n_temp, n_pn, n_qn);
	mp_modexp(n_key_dp, n_key, n_dp, n_pn);
	mp_modexp(n_key_dq, n_key, n_dq, n_qn);
	mp_move(n_temp, n_key_dp);
	mp_add(n_temp, n_pn);
	mp_sub(n_temp, n_key_dq);
	stage_modulus(n_pn); 
	mp_modmult(n_temp, n_temp, n_cr);
	mp_mult(n_key, n_temp, n_qn);
	mp_add(n_key, n_key_dq);

	randbuf = (unitptr)(((char*)n_key_dq) + nblocksize);
	memset(randbuf, 0, nblocksize * 3);

	p = NWDSBufPutPtr(outbuf, 12 + nblocksize_nw * 6);
	DSET_LH(p, 0, 0x00000001);
	DSET_LH(p, 4, 0x00100008);
	WSET_LH(p, 8, 3);
	WSET_LH(p, 10, nblocksize_nw * 3);
	memset(p + 12, 0, nblocksize_nw * 6);

	up = randbuf; up2 = (unitptr)(p + 12);
	for (i = 3; i; i--) {
		fillrandom((char *)up, nbytes);
		((char *)up)[nbytes - 1] &= nmask;
		if (!(j = mp_compare(up, n_mod))) {
			mp_dec(up);
		} else if (j > 0) {
			mp_sub(up, n_mod);
			mp_neg(up);
			mp_add(up, n_mod);
		}
		mp_modexp(up2, up, n_exp, n_mod);
		up = (unitptr)((char*)up + nblocksize);
		up2 = (unitptr)((char*)up2 + nblocksize_nw);
	}
	nwhash2block(hashbuf, p+12, nblocksize_nw * 3);
	nwhash2end(hashbuf);

	up = randbuf;
	for (i = 0; i < 3; i++) {
		mp_init(n_temp, WVAL_LH(hashbuf, i<<1));
		mp_modexp(up2, n_key, n_temp, n_mod);
		stage_modulus(n_mod);
		mp_modmult(up2, up2, up); 
		up = (unitptr)((char*)up + nblocksize);
		up2 = (unitptr)((char*)up2 + nblocksize_nw);
	}
	if (n_temp) {
    		mp_init0(n_temp);
		mp_init0(n_key_dp);
		mp_init0(n_key_dq);
		free(n_temp);
	}
	err = 0;
err_exit:
	memset(hashbuf, 0, sizeof(hashbuf));
	if (n_pn) { mp_init0(n_pn); free(n_pn); }
	if (n_qn) { mp_init0(n_qn); free(n_qn); }
	if (n_dp) { mp_init0(n_dp); free(n_dp); }
	if (n_dq) { mp_init0(n_dq); free(n_dq); }
	if (n_cr) { mp_init0(n_cr); free(n_cr); }
	ncpt_mutex_unlock(&mpilock);
	free(n_mod);
	free(n_exp);
	return err;
}

NWDSCCODE NWDSAuthenticateConn(
		NWDSContextHandle inctx,
		NWCONN_HANDLE conn) {
	/* it can be per NWDS_HANDLE lock... */
	static ncpt_mutex_t auth_lock = NCPT_MUTEX_INITIALIZER;
	nuint8 authid[4];
	NWDSCCODE err;
	size_t user_name_len;
	char *loginstrc;
	size_t loginstrc_len;
	nuint8 *u_key;
    	Buf_T signbuf;
	char signbuf_b[DEFAULT_MESSAGE_LEN];
	Buf_T keybuf;
	char keybuf_b[DEFAULT_MESSAGE_LEN];
	wchar_t server_name[MAX_DN_CHARS+1];
	NWObjectID user_id;
	char signkey[8];
	wchar_t* w_user_name;
	nuint8* u_priv_key;
	nuint8* logindata;
	union __NWDSAuthInfo* ndai;
	size_t ndailen;
	NWDSContextHandle ctx;
	
	if (!conn)
		return ERR_NULL_POINTER;
	ncpt_mutex_lock(&auth_lock);
	if (conn->connState & CONNECTION_AUTHENTICATED) {
		err = 0;
		goto err_exit3;
	}
	err = NWDSGetKeys(inctx, &ndai, &ndailen);
	if (err)
		goto err_exit3;
	w_user_name = (wchar_t*)(ndai->data + ndai->header.hdrlen);
	logindata = ndai->header.logindata;
	u_priv_key = ndai->data + ndai->header.hdrlen + ndai->header.name_len;
	err = __NWDSGetServerDN(conn, server_name, sizeof(server_name));
	if (err)
		goto err_exit3;
	err = NWDSDuplicateContextHandle(inctx, &ctx);
	if (err)
		goto err_exit3;
	ctx->dck.flags = DCV_XLATE_STRINGS | DCV_TYPELESS_NAMES | DCV_DEREF_ALIASES;
	ctx->priv_flags |= DCV_PRIV_AUTHENTICATING;
	err = NWDSSetContext(ctx, DCK_LOCAL_CHARSET, "WCHAR_T//");
	if (err)
    		goto err_exit2;
	/* FIXME! deref aliases? */
	err = NWDSMapNameToID(ctx, conn, (const NWDSChar*)w_user_name, &user_id);
	if (err)
		goto err_exit2;
	if ((err = nds_beginauth(conn, user_id, ctx, server_name, authid)))
		goto err_exit2;
	u_key = NULL;
	loginstrc = malloc(DEFAULT_MESSAGE_LEN);
	if (!loginstrc) {
		err = ENOMEM;
		goto err_exit2;
	}
	user_name_len = DEFAULT_MESSAGE_LEN - 22;
	err = __NWDSGetObjectDNUnicode(conn, user_id, (unicode*)(loginstrc + 22), &user_name_len);
	if (err)
		goto err_exit;
	loginstrc_len = user_name_len + 22;
	DSET_LH(loginstrc, 0, 1);
	WSET_LH(loginstrc, 4, 6);
	memcpy(loginstrc + 6, logindata, 8);
	fillrandom(loginstrc + 14, 4);
	WSET_LH(loginstrc, 18, 0);
	WSET_LH(loginstrc, 20, user_name_len);
	if ((err = get_public_key(ctx, w_user_name, &u_key)))
		goto err_exit;
	NWDSSetupBuf(&signbuf, signbuf_b, sizeof(signbuf_b));
#ifdef SIGNATURES
	if (conn->sign_wanted) {
	    	Buf_T tmp;
	
    		NWDSSetupBuf(&tmp, signkey, 8);
		NWDSBufPutSkip(&tmp, 8);
	        fillrandom(signkey, 8);
        	err = rsa_crypt(ctx, server_name, &tmp, &signbuf);
		if (err)
			goto err_exit;
	}
#endif   
	NWDSSetupBuf(&keybuf,keybuf_b, sizeof(keybuf_b));
	if ((err = gen_auth_data(&keybuf, u_key, u_priv_key, 
		authid, loginstrc, loginstrc_len)))
		goto err_exit;
	err = __NWDSFinishAuthenticationV0(conn, &signbuf, 
		loginstrc, loginstrc_len,
		&keybuf);
	memset(keybuf_b, 0, sizeof(keybuf_b));
	if (err)
		goto err_exit;
	ncp_set_private_key(conn, ndai, ndailen);
	conn->user_id = user_id;
	conn->user_id_valid = 1;
	conn->connState |= CONNECTION_AUTHENTICATED;
#ifdef SIGNATURES
	if ((err = ncp_sign_start(conn, signkey)))
		goto err_exit;
#endif        
	err = ncp_change_conn_state(conn, 1);
err_exit:
	if (loginstrc) free(loginstrc);
	if (u_key) free(u_key);
err_exit2:
	NWDSFreeContext(ctx);
err_exit3:
	ncpt_mutex_unlock(&auth_lock);
	return err;
}

long nds_login_auth(struct ncp_conn *conn, const char *user, 
 const char *pwd) {
    long err;
    wchar_t user_w[MAX_DN_CHARS+1];
    char *u_priv_key = NULL;
    wchar_t server_name[MAX_DN_CHARS+1];
    NWCONN_HANDLE userconn = NULL;
    int i;
    struct timeval tv;
    int grace_period = 0;
    NWDSContextHandle ctx = NULL;
#ifdef ERR_MSG    
    char buf[256]; /* to print username */
#endif
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);
    
    if (strlen(user) > MAX_DN_CHARS)
        return ENAMETOOLONG;

    err = NWDSCreateContextHandle(&ctx);
    if (err)
    	return err;
    ctx->dck.flags = DCV_XLATE_STRINGS | DCV_TYPELESS_NAMES | DCV_DEREF_ALIASES | DCV_CANONICALIZE_NAMES;
    ctx->priv_flags |= DCV_PRIV_AUTHENTICATING;
    err = NWDSXlateFromCtx(ctx, user_w, sizeof(user_w), user);
    if (err)
	goto err_exit;
    err = NWDSSetContext(ctx, DCK_LOCAL_CHARSET, "WCHAR_T//");
    if (err)
    	goto err_exit;
    NWDSAddConnection(ctx, conn);
    err = nds_login(ctx, (const NWDSChar*)user_w, pwd);
    if ((err == ERR_NO_SUCH_ENTRY) &&
	(user_w[0] != '.') &&
        (user_w[wcslen(user_w)-1] != '.')) {
#ifdef ERR_MSG        
        printf(_("User %s not found in current context.\n"
                 "Trying server context...\n"), user);
#endif
	err = __NWDSGetServerDN(conn, server_name, sizeof(server_name));
	if (err)
		goto err_exit;
        i = 0;
        while (server_name[i] && (server_name[i] != '.'))
            i++;
	if (wcslen(user_w)+wcslen(server_name+i)+1 > MAX_DN_CHARS) {
		err = ENAMETOOLONG;
		goto err_exit;
	}
	memcpy(user_w + wcslen(user_w), 
		server_name + i,
		(wcslen(server_name + i) + 1) * sizeof(wchar_t));
	ctx->dck.flags &= ~DCV_CANONICALIZE_NAMES;
	err = nds_login(ctx, (const NWDSChar*)user_w, pwd);
    }
    if (err) {
        if (err != NWE_PASSWORD_EXPIRED) {
#ifdef ERR_MSG
            fprintf(stderr, _("error %d logging in\n"), err);
#endif       
            goto err_exit;
       }
       grace_period = 1;
    }
    if ((err = NWDSAuthenticateConn(ctx, conn))) {
#ifdef ERR_MSG
        fprintf(stderr, _("error %d authenticating\n"), err);
#endif
        goto err_exit;
    }
    if (grace_period && (!err)) {
        err = NWE_PASSWORD_EXPIRED;
    }
err_exit:
    if (userconn)
    	NWCCCloseConn(userconn);
    if (ctx)
    	NWDSFreeContext(ctx);
    if (u_priv_key) {
    	clearkey(u_priv_key);
	free(u_priv_key);
    }
#ifdef RANDBUF
    ncpt_mutex_lock(&randbuflock);
    memset(global_randbuf, 0, RANDBUFSIZE);
    g_rndp = global_randbuf + RANDBUFSIZE;
    ncpt_mutex_unlock(&randbuflock);
#endif
    return err;
}

#ifdef DEBUG_PRINT
static void dgcdv(const unit* a, const unit* b, const unit* c, const unit* d, const unit* e) {
	unit a1[4096];
	unit a2[4096];
	unit a3[4096];

	printnumber("Z", a);
	printnumber("U", b);
	printnumber("V", c);
	printnumber("X", d);
	printnumber("Y", e);
	
	mp_mult(a1, b, d);
	mp_mult(a2, c, e);
	printnumber("UX", a1);
	printnumber("VY", a2);
	mp_add(a1, a2);
	printnumber("UX+VY", a1);
	
	mp_mod(a3, a1, d);
	printnumber("UX+VY%U", a3);
	mp_mod(a3, a1, e);
	printnumber("UX+VY%V", a3);
}
#endif

/* ctx must be in wchar_t mode */
NWDSCCODE __NWDSGetPublicKeyFromConnection(NWDSContextHandle ctx, NWCONN_HANDLE conn, nuint8 **skey) {
	wchar_t serverName[MAX_DN_CHARS+1];
	NWDSCCODE err;
	
	*skey = NULL;
	err = __NWDSGetServerDN(conn, serverName, sizeof(serverName));
	if (err)
		return err;
	err = get_public_key(ctx, serverName, skey);
	if (err)
		return err;
	return 0;
}

struct keyparam {
	unit	n_exp[bytes2units(106)];
	unit	n_mod[bytes2units(106)];
	unit	n_recip[bytes2units(106)];
	unit	n_pn[bytes2units(106 /*50*/)];
	unit	n_qn[bytes2units(106 /*50*/)];
	unit	n_dp[bytes2units(106 /*50*/)];
	unit	n_dq[bytes2units(106 /*50*/)];
	unit	n_cr[bytes2units(106)];
	int	n_recip_present;
	size_t	bytes;
	nuint8	BA;
	size_t	BL;
};

static int CreatePublicKey(const struct keyparam* kp, nuint8 *pubkey, size_t *pubkey_len) {
	nuint8* ptr;
	nuint8 hash[8];
	size_t n_exp_len;
	size_t len;
	
	n_exp_len = (countbits(kp->n_exp) + 7) / 8 + 2;
	len =     (4 + 4)			/* BV */
		+ (4 + 1) * 2			/* BC, BA */
		+ (4 + 2)			/* BL */
		+ (4 + kp->bytes * 2)		/* NN */
		+ (4 + n_exp_len)		/* EN */
		+ (4 + 8)			/* MA */
		+ (2 + 6);			/* PURSAF */
	if (kp->n_recip_present)
		len += 4 + kp->bytes * 2 + 6;	/* NI */
	if (len + 10 > *pubkey_len)
		return NWE_BUFFER_OVERFLOW;
	*pubkey_len = len + 10;

	DSET_LH(pubkey, 0, 1);
	WSET_LH(pubkey, 4, 3);
	WSET_LH(pubkey, 6, 1);
	WSET_LH(pubkey, 8, len);
	pubkey += 10;
	ptr = pubkey;

#define PUTHDR(ptr, type, len) \
	       WSET_HL(ptr, 0, (type)); \
	       WSET_LH(ptr, 2, len); \
	       ptr += 4;	
#define PUTMEM(ptr, type, len, data) \
	       PUTHDR(ptr, (type), len); \
	       memcpy(ptr, data, len); \
	       ptr += len;
#define PUTNUM(ptr, type, len, data) \
	       PUTHDR(ptr, (type), len); \
	       memcpy(ptr, data, len); \
	       ptr += len;
#define PUTBYTE(ptr, type, data) \
		PUTHDR(ptr, (type), 1); \
		BSET(ptr, 0, data); \
		ptr += 1;
#define PUTWORD(ptr, type, data) \
		PUTHDR(ptr, (type), 2); \
		WSET_LH(ptr, 0, data); \
		ptr += 2;
	PUTMEM(ptr,  0x4256, 4, "1.7b");
	PUTBYTE(ptr, 0x4243, 3);
	PUTBYTE(ptr, 0x4241, kp->BA);
	PUTWORD(ptr, 0x424C, kp->BL);
	PUTNUM(ptr,  0x4E4E, kp->bytes * 2, kp->n_mod);
	if (kp->n_recip_present) {
		PUTNUM(ptr, 0x4E49, kp->bytes * 2 + 6, kp->n_recip);
	}
	PUTNUM(ptr,  0x454E, n_exp_len, kp->n_exp);
	
	nwhash1init(hash, sizeof(hash));
	nwhash1(hash, sizeof(hash), pubkey, ptr - pubkey);
	PUTMEM(ptr,  0x4D41, sizeof(hash), hash);

	WSET_LH(ptr, 0, ptr - pubkey);
	
	memcpy(ptr + 2, "PURSAF", 6);
	return 0;
}

static int CreatePrivateKey(const struct keyparam* kp, nuint8* privkey, size_t* privkey_len) {
	nuint8* ptr;
	nuint8 hash[8];
	size_t n_exp_len;
	size_t len;
	
	n_exp_len = (countbits(kp->n_exp) + 7 ) / 8 + 2;
	len =    (4 + 4) 			/* BV */
	       + (4 + 1) * 2 			/* BC, BA */
	       + (4 + 2) 			/* BL */
	       + (4 + kp->bytes) * 5	 	/* PN, QN, DP, DQ, CR */
	       + (4 + n_exp_len) 		/* EN */
	       + (4 + kp->bytes * 2) 		/* NN */
	       + (4 + 8) 			/* MA */
	       + (2 + 6);			/* PRRSAF */
	if (len + 8 > *privkey_len)
		return NWE_BUFFER_OVERFLOW;
	*privkey_len = len + 8;

	DSET_LH(privkey, 0, 1);
	WSET_LH(privkey, 4, 2);
	WSET_LH(privkey, 6, len);
	privkey += 8;
	ptr = privkey;

	PUTMEM(ptr,  0x4256, 4, "1.7b");	
	PUTBYTE(ptr, 0x4243, 4);
	PUTBYTE(ptr, 0x4241, kp->BA);
	PUTWORD(ptr, 0x424C, kp->BL);	
	PUTNUM(ptr,  0x4E4E, kp->bytes * 2, kp->n_mod);
	PUTNUM(ptr,  0x454E, n_exp_len, kp->n_exp);
	PUTNUM(ptr,  0x504E, kp->bytes, kp->n_pn);
	PUTNUM(ptr,  0x514E, kp->bytes, kp->n_qn);
	PUTNUM(ptr,  0x4450, kp->bytes, kp->n_dp);
	PUTNUM(ptr,  0x4451, kp->bytes, kp->n_dq);
	PUTNUM(ptr,  0x4352, kp->bytes, kp->n_cr);
	
	nwhash1init(hash, sizeof(hash));
	nwhash1(hash, sizeof(hash), privkey, ptr - privkey);
	
	PUTMEM(ptr,  0x4D41, sizeof(hash), hash);
	
	WSET_LH(ptr, 0, ptr - privkey);
	
	memcpy(ptr + 2, "PRRSAF", 6);
	return 0;
}
#undef PUTWORD
#undef PUTBYTE
#undef PUTNUM
#undef PUTMEM
#undef PUTHDR

static int primes[] = { 
		  3,   5,   7, 0, 11,  13,  17,  19,  23,  29,  31,  37,  41,
		 43,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97, 101, 
		103, 107, 109, 113, 127, 131, 
		0x89, 0x8B, 0x95, 0x97, 0x9D, 0xA3, 0xA7, 0xAD, 0xB3, 0xB5, 
		0xBF, 0xC1, 0xC5, 0xC7, 0xD3, 0xDF, 0xE3, 0xE5, 0xE9, 0xEF, 
		0xF1, 0xFB, 0x101, 0x107, 0x10D, 0x10F, 0x115, 0x119, 0x11B, 
		0x125, 0x133, 0x137, 0x139, 0x13D, 0x14B, 0x151, 0x15B, 0x15D, 
		0x161, 0x167, 0x16F, 0x175, 0x17B, 0x17F, 0x185, 0x18D, 0x191, 
		0x199, 0x1A3, 0x1A5, 0x1AF, 0x1B1, 0x1B7, 0x1BB, 0x1C1, 0x1C9, 
		0x1CD, 0x1CF, 0x1D3, 0x1DF, 0x1E7, 0x1EB, 0x1F3, 0 };

static int IsPrime(const unit* n) {
	unitptr tmp;
	unitptr result;
	unitptr oneless;
	size_t len;
	size_t i;
	
	len = units2bytes(global_precision);
	tmp = alloca(len);
	result = alloca(len);
	oneless = alloca(len);
	/* n is odd, so n-1 is even... */
	mp_move(oneless, n);
	mp_clrbit(oneless, 0);
	
	for (i = 0; i < 100; i++) {
		int err;
	
		do {
			fillrandom((void*)tmp, len);
			if (mp_mod(result, tmp, n)) {
				continue;	/* return error? */
			}
		} while (testle(result, 1));
		err = mp_modexp(tmp, result, oneless, n);
		if (err) {
			continue;	/* return error? */
		}
		if (testne(tmp, 1))
			return 0;
	}
	mp_burn(oneless);
	mp_burn(result);
	mp_burn(tmp);
	return 1;
}

static int FindPrime(unitptr p, size_t bits) {
	size_t len;
	size_t old_precision;
	
	old_precision = global_precision;
	set_precision(bits2units(bits + SLOP_BITS));
	len = units2bytes(global_precision);
	
	while (1) {
		size_t i;
		int divd;
		nuint8 skip[1000];

		fillrandom((void*)p, len);
		mp_setbit(p, bits - 2);
		mp_setbit(p, bits - 1);
		for (i = units2bits(global_precision); i-- > bits; )
			mp_clrbit(p, i);
		mp_clrbit(p, 0);
	
		memset(skip, 0, sizeof(skip));
		for (divd = 3; divd < 9000; divd += 2) {
			int modd;
			int *prime;

			for (prime = primes; *prime && (*prime < divd); prime++) {
				if (divd % *prime == 0)
					goto next;
			}
			modd = mp_shortmod(p, divd);
			if (!modd)
				modd = divd;
			for (i = divd - modd; i < sizeof(skip); i += divd) {
				skip[i] = 1;
			}
next:;
		}
		for (mp_inc(p), i = 1; i < sizeof(skip); mp_inc(p), mp_inc(p), i += 2) {
			if (skip[i])
				continue;
			if (IsPrime(p)) {
				/* extend precision */
				for (i = global_precision; i < old_precision; i++) {
					p[i] = 0;
				}
				set_precision(old_precision);
				return 0;
			}
		}
	}
}

/* move this to mpilib? */
static void gcdv(unit *z, unit *u, unit *v, const unit *x, const unit *y) {
	unit	*u2;
	unit	*v2;
	unit	*z2;
	unit	*quot;
	unit	*tmp;
	unit	*tmp2;
	size_t	len = units2bytes(global_precision) * 6;
	
	u2 = alloca(len);
	v2 = u2 + global_precision;
	z2 = v2 + global_precision;
	quot = z2 + global_precision;
	tmp = quot + global_precision;
	tmp2 = tmp + global_precision;

	mp_init(u, 1);
	mp_init(v, 0);
	mp_move(z, x);
	mp_init(u2, 0);
	mp_init(v2, 1);
	mp_move(z2, y);
	while (testne(z2, 0)) {
		mp_div(tmp, quot, z, z2);
#define OSTEP(r,prev,mult) \
		mp_mult(tmp, prev, mult); \
		mp_move(tmp2, r); \
		mp_sub(tmp2, tmp); \
		mp_move(r, prev); \
		mp_move(prev, tmp2);
		OSTEP(u, u2, quot);
		OSTEP(v, v2, quot);
		OSTEP(z, z2, quot);
#undef OSTEP
	}
	if (mp_tstminus(u)) {
		mp_add(u, y);
	}
	if (mp_tstminus(v)) {
		mp_add(v, x);
	}
	memset(u2, 0, len);
}

static int ComputeKey(struct keyparam *kp) {
	unitptr  tmp;
	unitptr  tmp2;
	unitptr  tmp3;
	unitptr  n_pn1;
	unitptr  n_qn1;
	unitptr	 n_dn;
	int err;
	size_t len = units2bytes(global_precision) * 6;
	
	tmp  = alloca(len);
	tmp2 = tmp + global_precision;
	tmp3 = tmp2 + global_precision;
	n_pn1 = tmp3 + global_precision;
	n_qn1 = n_pn1 + global_precision;
	n_dn = n_qn1 + global_precision;
	
	mp_mult(kp->n_mod, kp->n_pn, kp->n_qn);
	/* Hey, why shift by 8? */
	mp_move_and_shift_left_bits(tmp, kp->n_mod, 8);	
	mp_recip(kp->n_recip, tmp);
	kp->n_recip_present = 1;
	mp_move(n_qn1, kp->n_qn);
	mp_dec(n_qn1);
	mp_move(n_pn1, kp->n_pn);
	mp_dec(n_pn1);
	mp_mult(tmp, n_pn1, n_qn1);
	gcdv(tmp2, n_dn, tmp3, kp->n_exp, tmp);
#ifdef DEBUG_PRINT
	dgcdv(tmp2, n_dn, tmp3, kp->n_exp, tmp);
#endif
	if (testne(tmp2, 1)) {
		err = ERR_SYSTEM_ERROR;
	} else {
		mp_div(kp->n_dp, tmp, n_dn, n_pn1);
		mp_div(kp->n_dq, tmp, n_dn, n_qn1);
		gcdv(tmp, tmp2, kp->n_cr, kp->n_pn, kp->n_qn);
		err = 0;
	}
	memset(tmp, 0, len);
	return err;
}

NWDSCCODE __NWGenerateKeyPair(
		size_t		key_len,
		const void*	exp,
		size_t		exp_len,
		void*		pubkey,
		size_t*		pubkey_len,
		void*		privkey,
		size_t*		privkey_len
) {
	nuint8 def_exp[] = { 1, 0, 1};
	struct keyparam kp;
	NWDSCCODE err;
	size_t bits;
	size_t words; /* netware native units */

	if (!pubkey || !pubkey_len || !privkey || !privkey_len) return -2;
	if (!key_len || !exp || !exp_len) {
		exp = def_exp;
		exp_len = sizeof(def_exp);
		key_len = 420;	/* well, at least 620 does work too */
	} else {
		if ((key_len > 760) || (key_len < 256) || (key_len & 1) || (exp_len > 16)) {
			return NWE_PARAM_INVALID;
		}
	}
	memset(&kp, 0, sizeof(kp));
	kp.BA = 48;
	kp.BL = key_len;
	memcpy(kp.n_exp, exp, exp_len);
	ncpt_mutex_lock(&mpilock);
	set_precision(bits2units(kp.BL) + SLOP_BITS);
	if ((countbits(kp.n_exp) > kp.BL) || !(kp.n_exp[0] & 1)) {
		err = NWE_PARAM_INVALID;
		goto quit;
	}
	bits = kp.BL >> 1;
	words = (bits / 16) + 1;
	kp.bytes = words * 2;
  	err = FindPrime(kp.n_pn, bits);
	if (err)
		goto quit;
	err = FindPrime(kp.n_qn, bits);
	if (err)
		goto quit;
	if (mp_compare(kp.n_pn, kp.n_qn) != 1) {
		unitptr tmp = alloca(units2bytes(global_precision));
		
		mp_move(tmp, kp.n_pn);
		mp_move(kp.n_pn, kp.n_qn);
		mp_move(kp.n_qn, tmp);
		mp_init0(tmp);
	}
	/* +6: NI has 6 additonal bytes... ?! */
	set_precision(bytes2units(kp.bytes * 2 + 6 + (SLOP_BITS + 7) / 8));
  	err = ComputeKey(&kp);
	if (err)
		goto quit;
  	err = CreatePublicKey(&kp, pubkey, pubkey_len);
	if (err)
		goto quit;
	err = CreatePrivateKey(&kp, privkey, privkey_len);
quit:;
	ncpt_mutex_unlock(&mpilock);
	memset(&kp, 0, sizeof(kp));
	return err;
}

