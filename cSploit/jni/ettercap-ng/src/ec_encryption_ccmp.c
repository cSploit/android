/*
    ettercap -- encryption functions

    Copyright (C) The Ettercap Dev Team

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <ec.h>
#include <ec_encryption.h>
#include <ec_checksum.h>
#include <ec_threads.h>

#include <openssl/aes.h>

/* globals */

#define CCMP_DECRYPT(_i, _b, _b0, _enc, _a, _len, _ctx) {  \
   /* Decrypt, with counter */                             \
   _b0[14] = (u_int8)((_i >> 8) & 0xff);                   \
   _b0[15] = (u_int8)(_i & 0xff);                          \
   AES_encrypt(_b0, _b, _ctx);         \
   XOR_BLOCK(_enc, _b, _len);          \
   /* Authentication */                \
   XOR_BLOCK(_a, _enc, _len);          \
   AES_encrypt(_a, _a, _ctx);          \
}

/* protos */

#undef B0
#undef AAD
#undef PN
#undef A

int wpa_ccmp_decrypt(u_char *mac, u_char *data, size_t len, struct wpa_sa sa);
static inline void get_PN(u_char *PN, u_char *data);
static inline void get_B0(u_char *B0, u_char *mac, u_char *PN, size_t len);
static inline void get_AAD(u_char *AAD, u_char *mac, u_char *B0);
static int ccmp_decrypt(u_char *enc, u_char *B0, u_char *B, u_char *A, u_char *mic, size_t len, AES_KEY *ctx);

/*******************************************/

/*
 * IEEE-802.11i-2004  8.3.3.1
 */
int wpa_ccmp_decrypt(u_char *mac, u_char *data, size_t len, struct wpa_sa sa)
{
   u_char mic[WPA_CCMP_TRAILER];
   u_char PN[6]; /* 48 bit Packet Number */
   size_t data_len = len - sizeof(struct wpa_header);
   u_char AAD[AES_BLOCK_SIZE*2];
   u_char B0[AES_BLOCK_SIZE], A[AES_BLOCK_SIZE], B[AES_BLOCK_SIZE];
   u_char decbuf[len];
   AES_KEY aes_ctx;

   if (len > UINT16_MAX) {
       return -ENOTHANDLED;
   }

   /* init the AES with the decryption key from SA */
   AES_set_encrypt_key(sa.decryption_key, 128, &aes_ctx);

   /* get the Packet Number */
   get_PN(PN, data);

   /* get the B0 */
   memset(B0, 0, sizeof(B0));
   get_B0(B0, mac, PN, data_len);

   /* get the Additional Authentication Data */
   memset(AAD, 0, sizeof(AAD));
   get_AAD(AAD, mac, B0);

   /* Start with the first block and AAD */
   AES_encrypt(B0, A, &aes_ctx);
   XOR_BLOCK(A, AAD, AES_BLOCK_SIZE);
   AES_encrypt(A, A, &aes_ctx);
   XOR_BLOCK(A, AAD + AES_BLOCK_SIZE, AES_BLOCK_SIZE);
   AES_encrypt(A, A, &aes_ctx);

   B0[0] &= 0x07;
   B0[14] = B0[15] = 0;
   AES_encrypt(B0, B, &aes_ctx);

   /* get the MIC trailer. it is after the end of our packet */
   memcpy(mic, data + len, WPA_CCMP_TRAILER);

   XOR_BLOCK(mic, B, WPA_CCMP_TRAILER);

   /* copy the encrypted data to the decryption buffer (skipping the wpa parameters) */
   memcpy(decbuf, data + sizeof(struct wpa_header), len);

   /* decrypt the packet */
   if (ccmp_decrypt(decbuf, B0, B, A, mic, len, &aes_ctx) != 0) {
      //DEBUG_MSG(D_VERBOSE, "WPA (CCMP) decryption failed, packet was skipped");
      return -ENOTHANDLED;
   }

   /*
    * copy the decrypted packet over the original one
    * overwriting the wpa header. this way the packet is
    * identical to a non-WPA one.
    */
   memcpy(data, decbuf, len);

   /*
    * wipe out the remaining bytes at the end of the packets
    * we have moved the data over the wpa header and the MIC was left
    * at the end of the packet.
    */
   memset(data + len - WPA_CCMP_TRAILER, 0, WPA_CCMP_TRAILER);

   return ESUCCESS;
}


/*
 * IEEE-802.11i-2004  8.3.3.2
 *
 * ----------------------------------------
 * |PN0|PN1|Reserved|KeyId|PN2|PN3|PN4|PN5|
 * ----------------------------------------
 */
static inline void get_PN(u_char *PN, u_char *data)
{
   PN[0] = data[0];
   PN[1] = data[1];
   /* skip the reserved */
   PN[2] = data[4];
   PN[3] = data[5];
   PN[4] = data[6];
   PN[5] = data[7];
}


static inline void get_B0(u_char *B0, u_char *mac, u_char *PN, size_t len)
{
   B0[0] = 0x59;
   B0[1] = 0; /* this will be set later by the callee */

   memcpy(B0 + 2, mac + 10, ETH_ADDR_LEN);

   B0[8]  = PN[5];
   B0[9]  = PN[4];
   B0[10] = PN[3];
   B0[11] = PN[2];
   B0[12] = PN[1];
   B0[13] = PN[0];

   B0[14] = ( len >> 8 ) & 0xFF;
   B0[15] = ( len & 0xFF );
}

static inline void get_AAD(u_char *AAD, u_char *mac, u_char *B0)
{
   AAD[0] = 0; /* AAD length >> 8 */
   AAD[1] = 0; /* this will be set below */
   AAD[2] = mac[0] & 0x8f;
   AAD[3] = mac[1] & 0xc7;
   /* we know 3 addresses are contiguous */
   memcpy(AAD + 4, mac + 4, 3 * ETH_ADDR_LEN);
   AAD[22] = mac[22] & 0x0F;
   AAD[23] = 0; /* all bits masked */

   /* XXX - implement the case of AP to AP 4 addresses wifi header */

   /* if WIFI_DATA | WIFI_BACON, we have a QoS Packet */
   if ( (mac[0] & (0x80 | 0x08)) == 0x88 ) {
      AAD[24] = mac[24] & 0x0f; /* just priority bits */
      AAD[25] = 0;
      B0[1] = AAD[24];
      AAD[1] = 22 + 2;
   } else {
      memset(&AAD[24], 0, 2);
      B0[1] = 0;
      AAD[1] = 22;
   }
}


static int ccmp_decrypt(u_char *enc, u_char *B0, u_char *B, u_char *A, u_char *mic, size_t len, AES_KEY *ctx)
{
   int i = 1;

   /* remove the trailer from the length */
   len -= WPA_CCMP_TRAILER;

   while (len >= AES_BLOCK_SIZE) {
      CCMP_DECRYPT(i, B, B0, enc, A, AES_BLOCK_SIZE, ctx);

      enc += AES_BLOCK_SIZE;
      len -= AES_BLOCK_SIZE;
      i++;
   }

   /* last block */
   if (len != 0) {
      CCMP_DECRYPT(i, B, B0, enc, A, len, ctx);
   }

   return memcmp(mic, A, WPA_CCMP_TRAILER);
}

/* EOF */

// vim:ts=3:expandtab

