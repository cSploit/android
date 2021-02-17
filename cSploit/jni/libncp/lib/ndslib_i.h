/*
    NDS client for ncpfs
    Copyright (C) 1997  Arne de Bruijn

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
    
	1.00  2000, April 18		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release.
					
*/

#ifndef __NDSLIB_I_H__
#define __NDSLIB_I_H__

#include "nwnet_i.h"

NWDSCCODE rsa_crypt2(const nuint8* s_key, Buf_T* input,	Buf_T* output);
NWDSCCODE rsa_crypt(NWDSContextHandle ctx, const wchar_t* servername, 
		Buf_T* input, Buf_T* output);
NWDSCCODE NWDSSetKeys(NWDSContextHandle ctx, const nuint8 logindata[8],
		const wchar_t* username, const nuint8* pkey, size_t pkey_len);
NWDSCCODE NWDSGetKeys(NWDSContextHandle ctx, union __NWDSAuthInfo** pndai,
		size_t* pndailen);
/* ctx must be in wchar_t mode */
NWDSCCODE __NWDSGetPublicKeyFromConnection(NWDSContextHandle ctx, 
		NWCONN_HANDLE conn, nuint8 **skey);
NWDSCCODE __NWGenerateKeyPair(size_t key_len, const void* exp, size_t exp_len,
		void* pubkey, size_t* pubkey_len, 
		void* privkey, size_t* privkey_len);
NWDSCCODE __NWEncryptWithSK(const void* hashsrc, size_t hashsrclen, 
		const void* idata, size_t ilen, 
		void* odata, size_t* olen);
NWDSCCODE __NWDSGetPrivateKey(NWCONN_HANDLE conn, const nuint8* connPublicKey,
		const nuint8 rndseed[4], NWObjectID objectID, 
		const nuint8 pwdHash[16], nuint8 logindata[8], 
		nuint8** privateKey, size_t* privateKeyLen);
NWDSCCODE nds_login(NWDSContextHandle ctx, const NWDSChar* objectName,
		const char* objectPassword);

#endif
