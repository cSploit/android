/*
    setkeys.c - NWDSGenerateObjectKeyPair implementation
    Copyright (C) 2000  Petr Vandrovec

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Revision history:

	1.00  2000, April 15		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release.

	1.01  2000, April 17		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSVerifyObjectPassword.
		
	1.02  2000, April 23		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSChangeObjectPassword. Moved nds_login from ndslib
		here.
		
	1.03  2000, May 22		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed NWDSChangeObjectPassword. It generated wrong key type.

	1.04  2000, July 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		Modified nds_login to use unaliased name in NWDSSetKeys.

 */

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "ncplib_i.h"
#include "nwnet_i.h"
#include "ndslib_i.h"
#include "ndscrypt.h"

static NWDSCCODE __NWDSSetKeysV0(
		NWCONN_HANDLE	conn,
		NWObjectID	objectID,
		Buf_T*		keyPair) {
	NW_FRAGMENT rq_frag[2];
	nuint8 rq_b[12];
	size_t len;
	
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(keyPair, &len);
	rq_frag[1].fragSize = len;
	DSET_LH(rq_b, 0, 0);
	DSET_HL(rq_b, 4, objectID);
	DSET_LH(rq_b, 8, len);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = sizeof(rq_b);
	
	return NWCFragmentRequest(conn, DSV_SET_KEYS, 2, rq_frag, 0, NULL, NULL);
}

static NWDSCCODE __NWDSVerifyPasswordV1(
		NWCONN_HANDLE	conn,
		NWObjectID	objectID,
		Buf_T*		key) {
	NW_FRAGMENT rq_frag[2];
	nuint8 rq_b[12];
	size_t len;
	
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(key, &len);
	rq_frag[1].fragSize = len;
	DSET_LH(rq_b, 0, 1);
	DSET_HL(rq_b, 4, objectID);
	DSET_LH(rq_b, 8, len);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = sizeof(rq_b);
	
	return NWCFragmentRequest(conn, DSV_VERIFY_PASSWORD, 2, rq_frag, 0, NULL, NULL);
}

static NWDSCCODE __NWDSChangePasswordV0(
		NWCONN_HANDLE	conn,
		NWObjectID	objectID,
		Buf_T*		key) {
	NW_FRAGMENT rq_frag[2];
	nuint8 rq_b[12];
	size_t len;
	
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(key, &len);
	rq_frag[1].fragSize = len;
	DSET_LH(rq_b, 0, 0);
	DSET_HL(rq_b, 4, objectID);
	DSET_LH(rq_b, 8, len);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = sizeof(rq_b);
	
	return NWCFragmentRequest(conn, DSV_CHANGE_PASSWORD, 2, rq_frag, 0, NULL, NULL);
}

static NWDSCCODE __NWDSGenerateObjectKeyPairStep1(
		NWDSContextHandle	ctx, 
		const NWDSChar*		objectName,
		NWCONN_HANDLE*		conn,
		NWObjectID*		objectID,
		NWObjectID*		oldPseudoID,
		nuint8			rndseed[4],
		nuint8**		connPublicKey
) {
	NWDSCCODE err;
	NWDSContextHandle unictx;
	NWCONN_HANDLE lconn;

	*connPublicKey = NULL;

	err = NWDSResolveName2DR(ctx, objectName, DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_WRITEABLE,
		&lconn, objectID);
	if (err)
		return err;
	err = __NWDSBeginLoginV0(lconn, *objectID, oldPseudoID, rndseed);
	if (err)
		goto err_free_conn;
	err = NWDSDuplicateContextHandle(ctx, &unictx);
	if (err)
		goto err_free_conn;
	unictx->dck.flags = DCV_TYPELESS_NAMES | DCV_XLATE_STRINGS;
	err = NWDSSetContext(unictx, DCK_LOCAL_CHARSET, "WCHAR_T//");
	if (err)
		goto err_free_ctx_conn;
	err = __NWDSGetPublicKeyFromConnection(unictx, lconn, connPublicKey);
	NWDSFreeContext(unictx);
	if (err)
		goto err_free_conn;
	*conn = lconn;
	return 0;
err_free_ctx_conn:;
	NWDSFreeContext(unictx);
err_free_conn:;
	NWCCCloseConn(*conn);
	return err;
}

static NWDSCCODE __NWDSGenerateObjectKeyPairStep2(
		NWCONN_HANDLE		conn,
		NWObjectID		objectID,
		nuint8			rndseed[4],
		NWObjectID		pseudoID, 
		size_t			pwdLen,
		const nuint8*		pwdHash, 
		const nuint8*		connPublicKey
) {
	nuint8		privkey[4096];
	size_t		privkey_len;
	nuint8		pubkey[4096];
	size_t		pubkey_len;
	nuint8		ec_privkey[4096];
	size_t		ec_privkey_len;
	NWDSCCODE	err;
	Buf_T		*ib;
	Buf_T		*ob;

	pubkey_len = sizeof(pubkey);
	privkey_len = sizeof(privkey);
	/* 0, NULL, 0 == keylen, exp, exp_len */
	err = __NWGenerateKeyPair(0, NULL, 0, pubkey, &pubkey_len, privkey, &privkey_len);
	if (err)
		return err;
	ec_privkey_len = sizeof(ec_privkey);
	err = __NWEncryptWithSK(pwdHash, 16, privkey, privkey_len, ec_privkey, &ec_privkey_len);
	memset(privkey, 0, sizeof(privkey));
	if (err)
		goto err_free_pub;
	err = NWDSAllocBuf(ec_privkey_len + pubkey_len + 0x2C + 8 + 8, &ib);
	if (err)
		goto err_free_ec_pub;
	NWDSBufPutLE32(ib, 0);
	NWDSBufPut(ib, rndseed, 4);
	DSET_HL(NWDSBufPutPtr(ib, 4), 0, pseudoID);
	NWDSBufPutLE32(ib, pwdLen);
	NWDSBufPutBuffer(ib, pwdHash, 16);
	NWDSBufPutBuffer(ib, ec_privkey, ec_privkey_len);
	NWDSBufPutBuffer(ib, pubkey, pubkey_len);

	memset(ec_privkey, 0, sizeof(ec_privkey));
	memset(pubkey, 0, sizeof(pubkey));

	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &ob);
	if (err) {
		NWDSClearFreeBuf(ib);
		return err;
	}
	
	err = rsa_crypt2(connPublicKey, ib, ob);
	NWDSClearFreeBuf(ib);
	if (!err) {
		err = __NWDSSetKeysV0(conn, objectID, ob);
	}
	NWDSClearFreeBuf(ob);
	return err;
err_free_ec_pub:;
	memset(ec_privkey, 0, sizeof(ec_privkey));
err_free_pub:;
	memset(pubkey, 0, sizeof(pubkey));
	return err;
}

static NWDSCCODE __NWDSGenerateObjectKeyPairStep3(
		NWCONN_HANDLE		conn,
		nuint8*			connPublicKey
) {
	free(connPublicKey);
	NWCCCloseConn(conn);
	return 0;
}

static void __NWDSHashPasswordUpper(const nuint8* objectPassword,
				    NWObjectID    pseudoID,
				    size_t        pwdLen,
				    nuint8        pwdHash[16]
) {
	nuint8 newPwd[pwdLen + 1];
	size_t i;
	nuint8 tmpID[4];

	for (i = 0; i < pwdLen; i++)
		newPwd[i] = toupper(*objectPassword++);
	newPwd[i] = 0;

	DSET_HL(tmpID, 0, pseudoID);
	shuffle(tmpID, newPwd, pwdLen, pwdHash);
}

static void __NWDSHashPassword(const nuint8* objectPassword,
			       NWObjectID    pseudoID,
			       size_t        pwdLen,
			       nuint8        pwdHash[16]
) {
	nuint8 tmpID[4];

	DSET_HL(tmpID, 0, pseudoID);
	shuffle(tmpID, objectPassword, pwdLen, pwdHash);
}
	
static NWDSCCODE __NWDSGenerateObjectKeyPair(
		NWDSContextHandle	ctx,
		const NWDSChar*		objectName,
		const char*		objectPassword
) {
	NWCONN_HANDLE	conn;
	NWObjectID	objectID;
	NWObjectID	pseudoID;
	nuint8		rndseed[4];
	nuint8*		serverPublicKey;
	NWDSCCODE	err;
	size_t		pwdLen;
	nuint8		pwdHash[16];
	
	err = __NWDSGenerateObjectKeyPairStep1(ctx, objectName,
			&conn, &objectID, &pseudoID, rndseed,
			&serverPublicKey);
	if (err)
		return err;
	/* compute key... */
	pwdLen = strlen(objectPassword);
	/* BEWARE! other NWDS*Password functions do NOT uppercase password */
	__NWDSHashPasswordUpper(objectPassword, pseudoID, pwdLen, pwdHash);
	err = __NWDSGenerateObjectKeyPairStep2(conn, objectID,
			rndseed, pseudoID, pwdLen, pwdHash,
			serverPublicKey);
	__NWDSGenerateObjectKeyPairStep3(conn, serverPublicKey);
	return err;
}

NWDSCCODE NWDSGenerateObjectKeyPair2(
		NWDSContextHandle	ctx, 
		const NWDSChar*		objectName, 
		NWObjectID		pseudoID, 
		size_t			pwdLen,
		const nuint8*		pwdHash, 
	UNUSED(	nflag32			optionsFlag)
) {
	NWCONN_HANDLE	conn;
	NWObjectID	objectID;
	NWObjectID	pseudoID2;
	nuint8		rndseed[4];
	nuint8*		serverPublicKey;
	NWDSCCODE	err;
	
	err = __NWDSGenerateObjectKeyPairStep1(ctx, objectName,
			&conn, &objectID, &pseudoID2, rndseed,
			&serverPublicKey);
	if (err)
		return err;
	err = __NWDSGenerateObjectKeyPairStep2(conn, objectID,
			rndseed, pseudoID, pwdLen, pwdHash,
			serverPublicKey);
	__NWDSGenerateObjectKeyPairStep3(conn, serverPublicKey);
	return err;
}

NWDSCCODE NWDSGenerateObjectKeyPair(
		NWDSContextHandle ctx, 
		const NWDSChar* objectName, 
		const char* objectPassword, 
		nflag32 optionsFlag
) {
	switch (optionsFlag) {
		case 0:
		case NDS_PASSWORD:
			return __NWDSGenerateObjectKeyPair(ctx, objectName, objectPassword);
		default:
			return NWE_PARAM_INVALID;
	}
}

static NWDSCCODE __NWDSVerifyObjectPasswordStep2(
		NWCONN_HANDLE		conn,
		NWObjectID		objectID,
		nuint8			rndseed[4],
		const nuint8*		pwdHash, 
		const nuint8*		connPublicKey
) {
	nuint8*		ec_privkey;
	size_t		ec_privkey_len;
	NWDSCCODE	err;
	Buf_T		*ib;
	Buf_T		*ob;

	err = NWDSAllocBuf(64, &ib);
	if (err)
		return err;
	ec_privkey = NWDSBufPutPtrLen(ib, &ec_privkey_len);
	err = __NWEncryptWithSK(pwdHash, 16, rndseed, 4, ec_privkey, &ec_privkey_len);
	if (err)
		return err;
	NWDSBufPutSkip(ib, ec_privkey_len);
	
	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &ob);
	if (err) {
		NWDSClearFreeBuf(ib);
		return err;
	}
	
	err = rsa_crypt2(connPublicKey, ib, ob);
	NWDSClearFreeBuf(ib);
	if (!err) {
		err = __NWDSVerifyPasswordV1(conn, objectID, ob);
	}
	NWDSClearFreeBuf(ob);
	return err;
}

NWDSCCODE NWDSVerifyObjectPassword(NWDSContextHandle ctx, UNUSED(nflag32 flags), 
		const NWDSChar* objectName, const char* objectPassword
) {
	NWCONN_HANDLE	conn;
	NWObjectID	objectID;
	NWObjectID	pseudoID;
	nuint8		rndseed[4];
	nuint8*		serverPublicKey;
	NWDSCCODE	err;
	size_t		pwdLen;
	nuint8		pwdHash[16];
	
	err = __NWDSGenerateObjectKeyPairStep1(ctx, objectName,
			&conn, &objectID, &pseudoID, rndseed,
			&serverPublicKey);
	if (err)
		return err;
	/* compute key... */
	pwdLen = strlen(objectPassword);
	__NWDSHashPassword(objectPassword, pseudoID, pwdLen, pwdHash);
	err = __NWDSVerifyObjectPasswordStep2(conn, objectID, rndseed, 
			pwdHash, serverPublicKey);
	__NWDSGenerateObjectKeyPairStep3(conn, serverPublicKey);
	return err;
}


static NWDSCCODE __NWDSChangeObjectPasswordStep2(
		NWCONN_HANDLE conn,
		NWObjectID objectID,
		NWObjectID pseudoID,
		nuint8 rndseed[4],
		nuint8 *connPublicKey,
		const char* oldPassword,
		const char* newPassword
) {
	nuint8* privateKey;
	size_t privateKeyLen;
	nuint8 ec_newpwd[4096];
	size_t ec_newpwd_len;
	nuint8 oldPwdHash[16];
	nuint8 newPwdHash[16];
	NWDSCCODE err;
	NWDSCCODE gpk_err;
	size_t di;
	Buf_T *ib;
	Buf_T *ob;
	nuint8 tmpID[4];
	
	DSET_HL(tmpID, 0, pseudoID);
	shuffle(tmpID, oldPassword, strlen(oldPassword), oldPwdHash);
	shuffle(tmpID, newPassword, strlen(newPassword), newPwdHash);
	
	err = __NWDSGetPrivateKey(conn, connPublicKey, rndseed, objectID, oldPwdHash, NULL, &privateKey, &privateKeyLen);
	if (err != 0 && err != NWE_PASSWORD_EXPIRED)
		goto quit;
	if (privateKeyLen < 10) {
		err = ERR_INVALID_SERVER_RESPONSE;
		goto free_privkey;
	}
	DSET_LH(privateKey, 2, 1);
	WSET_LH(privateKey, 6, 2);

	gpk_err = err;
	ec_newpwd_len = sizeof(ec_newpwd);
	err = __NWEncryptWithSK(newPwdHash, 16, privateKey + 2, privateKeyLen - 2, ec_newpwd, &ec_newpwd_len);
	if (err)
		goto free_privkey;
	di = ec_newpwd_len + 0x34;
	
	err = NWDSAllocBuf(di + 8, &ib);
	if (err)
		goto free_privkey_ecnewpwd;
	NWDSBufPut(ib, rndseed, 4);
	NWDSBufPutBuffer(ib, oldPwdHash, 16);
	NWDSBufPutLE32(ib, strlen(newPassword));
	NWDSBufPutBuffer(ib, newPwdHash, 16);
	NWDSBufPutBuffer(ib, ec_newpwd, ec_newpwd_len);
	
	err = NWDSAllocBuf(di + 256, &ob);
	if (err)
		goto free_privkey_ecnewpwd_ib;
	err = rsa_crypt2(connPublicKey, ib, ob);
	if (err)
		goto free_privkey_ecnewpwd_ib_ob;
	err = __NWDSChangePasswordV0(conn, objectID, ob);
	if (!err)
		err = gpk_err;
free_privkey_ecnewpwd_ib_ob:;
	NWDSClearFreeBuf(ob);
free_privkey_ecnewpwd_ib:;
	NWDSClearFreeBuf(ib);
free_privkey_ecnewpwd:;
	memset(ec_newpwd, 0, sizeof(ec_newpwd));
free_privkey:;
	memset(privateKey, 0, privateKeyLen);
	free(privateKey);
quit:;
	memset(oldPwdHash, 0, sizeof(oldPwdHash));
	memset(newPwdHash, 0, sizeof(newPwdHash));
	return err;
}

static NWDSCCODE __NWDSChangeObjectPassword(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		const char* oldPassword,
		const char* newPassword) {
	NWCONN_HANDLE conn;
	NWObjectID objectID;
	NWObjectID pseudoID;
	nuint8 rndseed[4];
	nuint8* connPublicKey;
	NWDSCCODE err;
	
	err = __NWDSGenerateObjectKeyPairStep1(ctx, objectName,
			&conn, &objectID, &pseudoID, rndseed,
			&connPublicKey);
	if (err)
		return err;
	err =  __NWDSChangeObjectPasswordStep2(conn, objectID, pseudoID, rndseed, connPublicKey, oldPassword, newPassword);
	__NWDSGenerateObjectKeyPairStep3(conn, connPublicKey);
	return err;
}

NWDSCCODE NWDSChangeObjectPassword(
		NWDSContextHandle ctx,
		nflag32 flags,
		const NWDSChar* objectName,
		const char* oldPassword,
		const char* newPassword
) {
	switch (flags) {
		case 0:
		case NDS_PASSWORD:
			return __NWDSChangeObjectPassword(ctx, objectName, oldPassword, newPassword);
		default:
			return NWE_PARAM_INVALID;
	}
}


NWDSCCODE nds_login(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		const char *objectPassword) {
	NWCONN_HANDLE	conn;
	NWObjectID	objectID;
	NWObjectID	pseudoID;
	nuint8		rndseed[4];
	nuint8*		serverPublicKey;
	NWDSCCODE	err;
	size_t		pwdLen;
	nuint8		pwdHash[16];
	nuint8*		privKey;
	size_t		privKeyLen;
	nuint8		logindata[8];
	NWDSCCODE	grace_err;
	wchar_t		unaliasedName[MAX_DN_CHARS + 1];
	NWDSContextHandle wctx;

	err = __NWDSGenerateObjectKeyPairStep1(ctx, objectName,
			&conn, &objectID, &pseudoID, rndseed,
			&serverPublicKey);
	if (err)
		return err;
	err = NWDSDuplicateContextHandleInt(ctx, &wctx);
	if (err) {
		__NWDSGenerateObjectKeyPairStep3(conn, serverPublicKey);
		return err;
	}
	err = NWDSMapIDToName(wctx, conn, objectID, (NWDSChar*)unaliasedName);
	if (err) {
		NWDSFreeContext(wctx);
		__NWDSGenerateObjectKeyPairStep3(conn, serverPublicKey);
		return err;
	}
	/* compute key... */
	pwdLen = strlen(objectPassword);
	/* BEWARE! other NWDS*Password functions do NOT uppercase password */
	__NWDSHashPasswordUpper(objectPassword, pseudoID, pwdLen, pwdHash);
	grace_err = __NWDSGetPrivateKey(conn, serverPublicKey,
	    rndseed, objectID, pwdHash, logindata, &privKey, &privKeyLen);
	__NWDSGenerateObjectKeyPairStep3(conn, serverPublicKey);
	if (!grace_err || grace_err == NWE_PASSWORD_EXPIRED) {
		err = NWDSSetKeys(wctx, logindata, unaliasedName, privKey, privKeyLen);
		memset(privKey, 0, privKeyLen);
		free(privKey);
		if (err)
			goto err_exit;
	}
	err = grace_err;
err_exit:
	NWDSFreeContext(wctx);
	memset(logindata, 0, sizeof(logindata));
	return err; 
}

