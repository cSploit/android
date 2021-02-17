/*
    ncpsign.c
    Copyright (C) 1997  Arne de Bruijn <arne@knoware.nl>

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

	0.00  1997			Arne de Bruijn <arne@knoware.nl>
						Initial revision.

 */

#include "config.h"

#ifdef SIGNATURES

#include <string.h>
#include "ncpsign.h"
#include "ncplib_i.h"

#define rol32(i,c) (((((i)&0xffffffff)<<c)&0xffffffff)| \
                    (((i)&0xffffffff)>>(32-c)))
/* i386: 32-bit, little endian, handles mis-alignment */
#ifdef __i386__
#define GET_LE32(p) (*(const int *)(p))
#define PUT_LE32(p,v) { *(int *)(p)=v; }
#else
#define GET_LE32(p) DVAL_LH(p,0)
#define PUT_LE32(p,v) DSET_LH(p,0,v)
#endif

#define min(a,b) ((a)<(b)?(a):(b))

static void nwsign(const char r_data1[16], char r_data2[64], char outdata[16]) {
 int i;
 unsigned int w0,w1,w2,w3;
 static int rbit[4]={0, 2, 1, 3};
#ifdef __i386__
 unsigned int *data2=(int *)r_data2;
#else
 unsigned int data2[16];
 for (i=0;i<16;i++)
  data2[i]=GET_LE32(r_data2+(i<<2));
#endif 
 w0=GET_LE32(r_data1);
 w1=GET_LE32(r_data1+4);
 w2=GET_LE32(r_data1+8);
 w3=GET_LE32(r_data1+12);
 for (i=0;i<16;i+=4) {
  w0=rol32(w0 + ((w1 & w2) | ((~w1) & w3)) + data2[i+0],3);
  w3=rol32(w3 + ((w0 & w1) | ((~w0) & w2)) + data2[i+1],7);
  w2=rol32(w2 + ((w3 & w0) | ((~w3) & w1)) + data2[i+2],11);
  w1=rol32(w1 + ((w2 & w3) | ((~w2) & w0)) + data2[i+3],19);
 }
 for (i=0;i<4;i++) {
  w0=rol32(w0 + (((w2 | w3) & w1) | (w2 & w3)) + 0x5a827999 + data2[i+0],3);
  w3=rol32(w3 + (((w1 | w2) & w0) | (w1 & w2)) + 0x5a827999 + data2[i+4],5);
  w2=rol32(w2 + (((w0 | w1) & w3) | (w0 & w1)) + 0x5a827999 + data2[i+8],9);
  w1=rol32(w1 + (((w3 | w0) & w2) | (w3 & w0)) + 0x5a827999 + data2[i+12],13);
 }
 for (i=0;i<4;i++) {
  w0=rol32(w0 + ((w1 ^ w2) ^ w3) + 0x6ed9eba1 + data2[rbit[i]+0],3);
  w3=rol32(w3 + ((w0 ^ w1) ^ w2) + 0x6ed9eba1 + data2[rbit[i]+8],9);
  w2=rol32(w2 + ((w3 ^ w0) ^ w1) + 0x6ed9eba1 + data2[rbit[i]+4],11);
  w1=rol32(w1 + ((w2 ^ w3) ^ w0) + 0x6ed9eba1 + data2[rbit[i]+12],15);
 }
 PUT_LE32(outdata,(w0+GET_LE32(r_data1)) & 0xffffffff);
 PUT_LE32(outdata+4,(w1+GET_LE32(r_data1+4)) & 0xffffffff);
 PUT_LE32(outdata+8,(w2+GET_LE32(r_data1+8)) & 0xffffffff);
 PUT_LE32(outdata+12,(w3+GET_LE32(r_data1+12)) & 0xffffffff);
}

/*
 * Initialize packet signatures
 * The first 16 bytes of logindata are the shuffled password,
 * the last 8 bytes the encryption key as received from the server.
 */
void sign_init(const unsigned char *logindata, unsigned char *sign_root) {
	static const unsigned char initlast[16] = 
		{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
		  0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
	static const unsigned char *initdata = "Authorized NetWare Client";
	unsigned char msg[64];
	unsigned char hash[16];
 
	memcpy(msg, logindata, 24);
	memcpy(msg + 24, initdata, 25);
	memset(msg + 24 + 25, 0, 64 - 24 - 25);
 	nwsign(initlast, msg, hash);
	memcpy(sign_root, hash, 8);
}

/*
 * Make a signature for the current packet and add it at the end of the
 * packet. 
 */
void __sign_packet(struct ncp_conn *conn, const void* packet, size_t size, u_int32_t totalsize, void *signbuff) {
	unsigned char data[64];

	memcpy(data, conn->sign_data.sign_root, 8);
	*(u_int32_t*)(data + 8) = totalsize;
	if (size < 52) {
		memcpy(data+12, packet, size);
		memset(data+12+size, 0, 52-size);
	} else {
		memcpy(data+12, packet, 52);
	}
	nwsign(conn->sign_data.sign_last, data, conn->sign_data.sign_last);
	memcpy(signbuff, conn->sign_data.sign_last, 8);
}

int sign_verify_reply(struct ncp_conn *conn, const void* packet, size_t size, u_int32_t totalsize, const void *signbuff) {
	unsigned char data[64];
	unsigned char hash[16];

	memcpy(data, conn->sign_data.sign_root, 8);
	*(u_int32_t*)(data + 8) = totalsize;
	if (size < 52) {
		memcpy(data+12, packet, size);
		memset(data+12+size, 0, 52-size);
	} else {
		memcpy(data+12, packet, 52);
	}
	nwsign(conn->sign_data.sign_last, data, hash);
	return memcmp(signbuff, hash, 8);
}
#endif
